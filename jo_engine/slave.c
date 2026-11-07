/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2026, Johannes Fetz (johannesfetz@gmail.com)
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the Johannes Fetz nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL Johannes Fetz BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
** INCLUDES
*/
#include <stdbool.h>
#include "jo/conf.h"
#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/math.h"
#include "jo/tools.h"
#include "jo/core.h"
#include "jo/malloc.h"
#include "jo/list.h"
#include "jo/time.h"
#include "jo/profile.h"

#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT

/*
** The slave CPU executes the jobs of a queue in jo_slave_main(). The master CPU
** adds a job and wakes it up by writing to 0x21000000, which sets the input
** capture flag of the slave free running timer. The slave polls this on-chip
** flag, so waiting does not use the external bus.
**
** The queue holds the jobs of the user (jo_core_exec_on_slave()) and of the
** engine (__jo_core_add_slave_job(): the VDP1 commands of the previous frame are
** sorted and copied by the slave CPU while the master CPU computes the next frame).
** The master CPU never waits to add a job, unless the queue is full.
**
** The variables shared by the 2 CPUs are read and written through the
** cache-through addresses (the cache of a CPU doesn't see the writes of the other).
*/

/*
** INTERNAL MACROS
*/
# define JO_SLAVE_ENTRY_POINT               (*(volatile unsigned int *)0x06000250)
# define JO_SLAVE_STACK_POINTER             (*(volatile unsigned int *)0x060002AC)
# define JO_SLAVE_WAKE_UP                   (*(volatile unsigned short *)0x21000000)
/** @brief Any value can be written to wake up the slave CPU */
# define JO_SLAVE_WAKE_UP_VALUE             (0xffff)
/** @brief Maximum wait loop count for the slave CPU startup */
# define JO_SLAVE_START_TIMEOUT             (0x100000)
/** @brief Size of the job queue (power of 2) */
# define JO_SLAVE_QUEUE_SIZE                (8)
# define JO_SLAVE_RUNNING                   (*(volatile unsigned int *)JO_CACHE_THROUGH(&__jo_slave_running))
/** @brief Jobs added by the master CPU (written by the master CPU only) */
# define JO_SLAVE_QUEUE_HEAD                (*(volatile unsigned int *)JO_CACHE_THROUGH(&__jo_slave_queue_head))
/** @brief Jobs done by the slave CPU (written by the slave CPU only) */
# define JO_SLAVE_QUEUE_TAIL                (*(volatile unsigned int *)JO_CACHE_THROUGH(&__jo_slave_queue_tail))
/** @brief User jobs done by the slave CPU (written by the slave CPU only) */
# define JO_SLAVE_USER_DONE                 (*(volatile unsigned int *)JO_CACHE_THROUGH(&__jo_slave_user_done))
/** @brief Ticks of the free running timer the slave CPU spent waiting, its user jobs done, before the master CPU waited
 *  for it (written by the slave CPU only, see jo_core_slave_idle_time()) */
# define JO_SLAVE_IDLE_TICKS                (*(volatile unsigned int *)JO_CACHE_THROUGH(&__jo_slave_idle_ticks))
/** @brief User jobs added by the master CPU (written by the master CPU only) */
# define JO_SLAVE_USER_ADDED                (*(volatile unsigned int *)JO_CACHE_THROUGH(&__jo_slave_user_added))
/** @brief The master CPU waits in jo_core_wait_for_slave() (until it adds a new user job, written by the master CPU only) */
# define JO_SLAVE_MASTER_WAITING            (*(volatile unsigned int *)JO_CACHE_THROUGH(&__jo_slave_master_waiting))
/** @brief Microseconds per tick of the free running timers (clock / 32, see jo_time_init(): 26.8 MHz or 28.6 MHz) */
# define JO_SLAVE_TICK_MICROSECONDS(TICKS)  ((JO_TV_WIDTH == 352 || JO_TV_WIDTH == 704) ? ((TICKS) * 1000) / 895 : ((TICKS) * 1000) / 839)
# define JO_SLAVE_JOB(INDEX)                (((volatile jo_slave_job *)JO_CACHE_THROUGH(__jo_slave_queue))[(INDEX) & (JO_SLAVE_QUEUE_SIZE - 1)])

/** @brief Job of the slave CPU */
typedef struct
{
    jo_slave_job_function           function;
    void                            *argument;
}                                   jo_slave_job;

jo_list                             __slave_callbacks;
static volatile unsigned int        __jo_slave_running;
static jo_slave_job                 __jo_slave_queue[JO_SLAVE_QUEUE_SIZE];
static volatile unsigned int        __jo_slave_queue_head;
static volatile unsigned int        __jo_slave_queue_tail;
static volatile unsigned int        __jo_slave_user_done;
static volatile unsigned int        __jo_slave_idle_ticks;
static volatile unsigned int        __jo_slave_master_waiting;
/** @brief User jobs added (master CPU only, read by the slave CPU: JO_SLAVE_USER_ADDED) */
static volatile unsigned int        __jo_slave_user_added;
/** @brief Balance of the CPUs in the last frame (master CPU only, see jo_core_slave_idle_time())
 *  @remarks idle: JO_SLAVE_IDLE_TICKS at the last jo_core_wait_for_slave(), waited: ticks the master CPU waited */
static unsigned int                 __jo_slave_last_idle_ticks;
static int                          __jo_slave_idle_time;
/** @brief Microseconds the master CPU waited in the last jo_core_wait_for_slave() (see jo_core_master_wait_time()) */
static int                          __jo_slave_master_wait_time;

#ifdef JO_SLAVE_SLEEP
/*
** THE SLAVE CPU SLEEPS WHILE IT WAITS (JO_SLAVE_SLEEP, not by default)
**
** The slave CPU waits for its jobs in SLEEP (the CPU stops until an interrupt), not in a loop on the flag of its free
** running timer: an emulator (Mednafen) does not run the instructions of a sleeping CPU (a game ran 7 % faster with
** the master CPU sleeping in its wait of the VBLANK, see __jo_core_synch(); the slave CPU idle 9 to 17 ms per frame).
** The master CPU wakes up the slave CPU by a write to its FRT input capture: the input capture interrupt (vector
** JO_SLAVE_WAKE_UP_VECTOR, priority 15, the slave CPU runs at the level 14 like the sampling of the profiler) sets
** __jo_slave_woken and clears the flag. A wake-up between the test of the flag and the SLEEP is lost until the next
** write: the master CPU writes again while it waits for the slave CPU (jo_core_wait_for_slave()).
** The vectors of the slave CPU: its own table (the table of the Boot ROM stands under its stack: an interrupt during
** the loading of a level, the stack deep, jumped into the stack).
** Not by default: SLEEP is rarely used by the games, the emulators get it wrong. Yabause (and its forks) saves the
** address of the SLEEP itself at an interrupt (the CPU sleeps again after the RTE, before testing its flag: the slave
** CPU never ran its jobs, the game froze); with it the demos crashed in SSF too. Mednafen: a game emulated 4 % faster.
*/
/** @brief Vector of the input capture interrupt of the free running timer (user vectors: 64 to 127; 0x66: the sampling
 *  of the profiler) */
# define JO_SLAVE_WAKE_UP_VECTOR            (0x64)
/** @brief Set by the input capture interrupt of the slave CPU (its own CPU: no cache-through needed) */
volatile unsigned int               __jo_slave_woken __attribute__((used, externally_visible));
/** @brief The vectors of the slave CPU (a copy of the table of the Boot ROM, the wake-up handler added) */
static unsigned int                 __jo_slave_vectors[256] __attribute__((aligned(4)));

/** @brief Input capture interrupt handler of the slave CPU: the flag cleared, __jo_slave_woken set */
__asm__(
"    .align 2\n"
"    .global ___jo_slave_wake_up_handler\n"
"___jo_slave_wake_up_handler:\n"
"    mov.l r0, @-r15\n"
"    mov.l r1, @-r15\n"
"    mov.l 1f, r1\n"                    /* FTCSR: clear ICF */
"    mov.b @r1, r0\n"
"    and #0x7F, r0\n"
"    mov.b r0, @r1\n"
"    mov.l 2f, r1\n"
"    mov #1, r0\n"
"    mov.l r0, @r1\n"
"    mov.l @r15+, r1\n"
"    mov.l @r15+, r0\n"
"    rte\n"
"    nop\n"
"    .align 2\n"
"1:  .long 0xFFFFFE11\n"
"2:  .long ___jo_slave_woken\n"
);
extern void                         __jo_slave_wake_up_handler(void);

/** @brief Slave CPU: its vectors, the wake-up interrupt (called once by jo_slave_main()) */
static void                         jo_slave_sleep_init(void)
{
    const unsigned int              *bios;
    unsigned int                    k;

    __asm__ volatile ("stc vbr, %0" : "=r" (bios));
    for (JO_ZERO(k); k < 256; ++k)
        __jo_slave_vectors[k] = bios[k];
    __jo_slave_vectors[JO_SLAVE_WAKE_UP_VECTOR] = (unsigned int)__jo_slave_wake_up_handler;
    __asm__ volatile ("ldc %0, vbr" : : "r" (__jo_slave_vectors));
    // (the input capture vector: the high byte of VCRC; the low byte: the output compare, the sampling of the profiler)
    JO_SH2_VCRC = (unsigned short)((JO_SH2_VCRC & 0x00FF) | (JO_SLAVE_WAKE_UP_VECTOR << 8));
    // (the free running timer at the priority 15: the slave CPU runs at the level 14)
    JO_SH2_IPRB = (unsigned short)((JO_SH2_IPRB & 0xF0FF) | 0x0F00);
    JO_ZERO(__jo_slave_woken);
    __jo_core_set_sr(0xE0);
}
#endif

inline int			jo_core_add_slave_callback(const jo_event_callback callback)
{
    jo_node         *node;

#ifdef JO_DEBUG
    if (callback == JO_NULL)
    {
        jo_core_error("callback is null");
        return (0);
    }
#endif
    node = jo_list_add_ptr(&__slave_callbacks, callback);
    if (node == JO_NULL)
        return (0);
    return (int)node;
}

/** @brief The slave CPU runs the callbacks (set by the master CPU, see jo_core_run()): a callback removed then waits */
volatile bool                       __jo_slave_callbacks_running = false;
/** @brief The callbacks removed while the slave CPU runs them (written without the cache: by both CPUs) */
static int                          __jo_slave_callbacks_removed[8];
static unsigned int                 __jo_slave_callbacks_removed_count = 0;
# define JO_SLAVE_REMOVED_COUNT     (*(volatile unsigned int *)JO_CACHE_THROUGH(&__jo_slave_callbacks_removed_count))
# define JO_SLAVE_REMOVED(INDEX)    (((volatile int *)JO_CACHE_THROUGH(__jo_slave_callbacks_removed))[(INDEX)])

void                                __jo_slave_callbacks_done(void)
{
    unsigned int                    i;

    __jo_slave_callbacks_running = false;
    for (JO_ZERO(i); i < JO_SLAVE_REMOVED_COUNT; ++i)
        jo_list_remove(&__slave_callbacks, (jo_node *)JO_SLAVE_REMOVED(i));
    JO_SLAVE_REMOVED_COUNT = 0;
}

inline void    jo_core_remove_slave_callback(const int event_id)
{
    unsigned int                    count;

#ifdef JO_DEBUG
    if (event_id == 0)
    {
        jo_core_error("Invalid event_id (%d)", event_id);
        return;
    }
#endif
    // (the slave CPU reads the list while it runs the callbacks: the node is freed after, see __jo_slave_callbacks_done())
    if (*(volatile bool *)JO_CACHE_THROUGH(&__jo_slave_callbacks_running))
    {
        count = JO_SLAVE_REMOVED_COUNT;
        if (count < (sizeof(__jo_slave_callbacks_removed) / sizeof(__jo_slave_callbacks_removed[0])))
        {
            JO_SLAVE_REMOVED(count) = event_id;
            JO_SLAVE_REMOVED_COUNT = count + 1;
            return;
        }
        // (more than 8 at once: removed now, as before. Waiting for the slave CPU here would block a slave callback)
    }
    jo_list_remove(&__slave_callbacks, (jo_node *)event_id);
}

/** @brief Purge the cache of the current CPU
 *  @remarks Needed after the other CPU changed the shared data
 */
static  __jo_force_inline void      jo_slave_cache_purge(void)
{
    unsigned char                   ccr;

    ccr = JO_SH2_CCR;
    // Disable the cache while purging it
    JO_SH2_CCR = (ccr & ~JO_SH2_CCR_CACHE_ENABLE);
    JO_SH2_CCR = (ccr & ~JO_SH2_CCR_CACHE_ENABLE) | JO_SH2_CCR_CACHE_PURGE;
    JO_SH2_CCR = ccr;
}

/** @brief Main loop of the slave CPU
 *  @remarks Executes the jobs of the queue in order
 *  @remarks The master CPU wakes up the slave CPU by writing to JO_SLAVE_WAKE_UP
 */
static void                         jo_slave_main(void)
{
    jo_slave_job_function           function;
    void                            *argument;
    unsigned int                    tail;
    unsigned int                    start;
    bool                            counting;

    __jo_core_set_sr(JO_SH2_SR_INTERRUPT_MASK);
    JO_SH2_FRT_TIER = JO_SH2_FRT_TIER_NO_INTERRUPT;
    JO_SH2_FRT_FTCSR = 0;
    // Free running timer: clock / 32, like the master CPU (see jo_time_init())
    jo_time_poke_byte(RegisterTCR, (jo_time_peek_byte(RegisterTCR) & ~JO_TIME_M_CKS) | JO_TIME_CKS_32_MODE);
    // Purge and enable the cache
    JO_SH2_CCR = JO_SH2_CCR_CACHE_PURGE;
    JO_SH2_CCR = JO_SH2_CCR_CACHE_ENABLE;
#ifdef JO_SLAVE_SLEEP
    jo_slave_sleep_init();
#endif
    JO_SLAVE_RUNNING = 1;
    for (;;)
    {
        tail = JO_SLAVE_QUEUE_TAIL;
        if (JO_SLAVE_QUEUE_HEAD == tail)
        {
            // The time spent waiting, the user jobs done, before the master CPU waits for the slave CPU (it wakes up the
            // slave CPU then): the master CPU balances the work of the CPUs with it, see jo_core_slave_idle_time()
            start = jo_time_get_frc();
            counting = JO_SLAVE_USER_DONE == JO_SLAVE_USER_ADDED && !JO_SLAVE_MASTER_WAITING;
            while (JO_SLAVE_QUEUE_HEAD == tail)
            {
#ifndef JO_SLAVE_SLEEP
                while (!(JO_SH2_FRT_FTCSR & JO_SH2_FRT_FTCSR_ICF))
                    __jo_core_pause();
                JO_SH2_FRT_FTCSR = JO_SH2_FRT_FTCSR & ~JO_SH2_FRT_FTCSR_ICF;
#else
                // (the interrupt enabled again each time: the sampling of the profiler writes TIER, see jo_profile_sampling_install())
                JO_SH2_FRT_TIER = JO_SH2_FRT_TIER | JO_SH2_FRT_TIER_ICIE;
                JO_ZERO(__jo_slave_woken);
                // (a job added before the flag was cleared: not waited for)
                if (JO_SLAVE_QUEUE_HEAD != tail)
                    break;
                while (!__jo_slave_woken)
                    __asm__ volatile ("sleep");
#endif
                if (counting && JO_SLAVE_MASTER_WAITING)
                {
                    JO_SLAVE_IDLE_TICKS = JO_SLAVE_IDLE_TICKS + ((jo_time_get_frc() - start) & 0xFFFF);
                    counting = false;
                }
            }
            if (counting)
                JO_SLAVE_IDLE_TICKS = JO_SLAVE_IDLE_TICKS + ((jo_time_get_frc() - start) & 0xFFFF);
        }
        // The data of the job were written by the master CPU
        jo_slave_cache_purge();
        __jo_profile_slave_job();
        function = JO_SLAVE_JOB(tail).function;
        argument = JO_SLAVE_JOB(tail).argument;
        jo_profile_begin(JO_PROFILE_SLAVE);
        function(argument);
        jo_profile_end(JO_PROFILE_SLAVE);
        JO_SLAVE_QUEUE_TAIL = tail + 1;
    }
}

void                                jo_core_slave_init(void)
{
    register unsigned int           timeout;

    jo_list_init(&__slave_callbacks);
    JO_SLAVE_RUNNING = 0;
    JO_SLAVE_QUEUE_HEAD = 0;
    JO_SLAVE_QUEUE_TAIL = 0;
    JO_SLAVE_USER_DONE = 0;
    JO_SLAVE_IDLE_TICKS = 0;
    JO_SLAVE_MASTER_WAITING = 0;
    JO_SLAVE_USER_ADDED = 0;
    JO_ZERO(__jo_slave_last_idle_ticks);
    JO_ZERO(__jo_slave_idle_time);
    __jo_smpc_command(DisableSlaveCPU);
    // Boot ROM: slave entry point and stack
    JO_SLAVE_ENTRY_POINT = (unsigned int)jo_slave_main;
    JO_SLAVE_STACK_POINTER = JO_WORK_RAM_SLAVE_STACK_TOP;
    __jo_smpc_command(EnableSlaveCPU);
    // Wait until the slave CPU is ready
    for (JO_ZERO(timeout); !JO_SLAVE_RUNNING && timeout < JO_SLAVE_START_TIMEOUT; ++timeout)
        ;
}

bool                                __jo_core_is_slave_running(void)
{
    return (JO_SLAVE_RUNNING != 0);
}

void                                __jo_core_add_slave_job(const jo_slave_job_function function, void *argument)
{
    unsigned int                    head;

    // The slave CPU didn't start: the job is done now by the master CPU
    if (!JO_SLAVE_RUNNING)
    {
        function(argument);
        return;
    }
    head = JO_SLAVE_QUEUE_HEAD;
    // The queue is full: wait for the oldest job
    while (head - JO_SLAVE_QUEUE_TAIL >= JO_SLAVE_QUEUE_SIZE)
        ;
    JO_SLAVE_JOB(head).function = function;
    JO_SLAVE_JOB(head).argument = argument;
    // The job is visible before the new head (the writes through the cache-through addresses are done in order)
    JO_SLAVE_QUEUE_HEAD = head + 1;
    // Wake up the slave CPU (FRT input capture)
    JO_SLAVE_WAKE_UP = JO_SLAVE_WAKE_UP_VALUE;
}

/** @brief Execute a user callback and notify the master CPU
 *  @param param Callback (jo_slave_callback)
 */
static void                         jo_core_slave_bootstrap(void *param)
{
    ((jo_slave_callback)param)();
    JO_SLAVE_USER_DONE = JO_SLAVE_USER_DONE + 1;
}

inline void                        jo_core_wait_for_slave(void)
{
    unsigned int                    start;
    unsigned int                    waited;
    unsigned int                    idle;

    jo_profile_begin(JO_PROFILE_WAIT_SLAVE);
    // All the callbacks given to jo_core_exec_on_slave() are done (the slave CPU is woken up: it stops counting its idle time)
    start = jo_time_get_frc();
    JO_SLAVE_MASTER_WAITING = 1;
    JO_SLAVE_WAKE_UP = JO_SLAVE_WAKE_UP_VALUE;
    // (written again now and then: a wake-up lost by the slave CPU between its test and its SLEEP, see jo_slave_main())
    for (JO_ZERO(waited); JO_SLAVE_USER_DONE != JO_SLAVE_USER_ADDED; ++waited)
        if ((waited & 63) == 63)
            JO_SLAVE_WAKE_UP = JO_SLAVE_WAKE_UP_VALUE;
    waited = (jo_time_get_frc() - start) & 0xFFFF;
    // The data written by the slave CPU
    jo_slave_cache_purge();
    // The balance of the frame: the slave CPU waited for its jobs (it can do more), or the master CPU waited for it
    idle = JO_SLAVE_IDLE_TICKS;
    __jo_slave_master_wait_time = JO_SLAVE_TICK_MICROSECONDS((int)waited);
    __jo_slave_idle_time = JO_SLAVE_TICK_MICROSECONDS((int)(idle - __jo_slave_last_idle_ticks)) - __jo_slave_master_wait_time;
    __jo_slave_last_idle_ticks = idle;
    jo_profile_end(JO_PROFILE_WAIT_SLAVE);
}

inline int                         jo_core_slave_idle_time(void)
{
    return (__jo_slave_idle_time);
}

inline int                         jo_core_master_wait_time(void)
{
    return (__jo_slave_master_wait_time);
}

inline void                        jo_core_exec_on_slave(jo_slave_callback callback)
{
    JO_SLAVE_MASTER_WAITING = 0;
    JO_SLAVE_USER_ADDED = JO_SLAVE_USER_ADDED + 1;
    // (when the slave CPU didn't start, the callback is executed now on the master CPU: jo_core_wait_for_slave() doesn't hang)
    __jo_core_add_slave_job(jo_core_slave_bootstrap, (void *)callback);
}


#endif //!JO_COMPILE_WITH_DUAL_CPU_SUPPORT
