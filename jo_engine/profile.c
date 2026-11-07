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
#include "jo/core.h"
#include "jo/tools.h"
#include "jo/math.h"
#include "jo/time.h"
#include "jo/profile.h"
#include "internal/core.h"
#include "internal/profile.h"

#ifdef JO_COMPILE_WITH_PROFILER_SUPPORT

/*
** Each CPU adds its times to its own measures (__jo_profile_cpu[0]: master CPU, [1]: slave CPU), through the
** cache-through address. The sums are never reset: every JO_PROFILE_FRAMES frames, the master CPU computes the
** averages from the differences with the previous sums. The maximums are reset by the master CPU (a maximum of
** the slave CPU found at the same time may be lost).
*/

/*
** INTERNAL MACROS
*/
/** @brief TCR: free running timer clock = CPU clock / 32 */
# define JO_PROFILE_TCR_CLOCK_32            (1)
/** @brief Measures that are counts (not times) */
# define JO_PROFILE_IS_COUNT(SLOT)          ((SLOT) == JO_PROFILE_VBLANKS || (SLOT) == JO_PROFILE_VDP1_COMMANDS || (SLOT) == JO_PROFILE_POLYGONS || (SLOT) == JO_PROFILE_3D_VERTEX_COUNT || (SLOT) == JO_PROFILE_3D_POLYGON_COUNT || (SLOT) == JO_PROFILE_3D_PIXELS)
/** @brief Characters of a name */
# define JO_PROFILE_NAME_LENGTH             (8)

/*
** GLOBALS
*/
jo_profile_cpu                              __jo_profile_cpu[2];
static unsigned int                         __jo_profile_last_sum[2][JO_PROFILE_COUNT];
static unsigned int                         __jo_profile_average[JO_PROFILE_COUNT];
static unsigned int                         __jo_profile_maximum[JO_PROFILE_COUNT];
static bool                                 __jo_profile_used[JO_PROFILE_COUNT];
static char                                 __jo_profile_names[JO_PROFILE_COUNT][JO_PROFILE_NAME_LENGTH + 1];
static unsigned int                         __jo_profile_frames;
/* The master CPU asks the slave CPU to reset its maximums (written by the master CPU, cleared by the slave CPU: through the
   cache-through address) */
static volatile bool                        __jo_profile_slave_reset;
static bool                                 __jo_profile_changed;
/* Ticks per millisecond x 1000 (the CPU clock depends on the horizontal resolution) */
static unsigned int                         __jo_profile_ticks_per_ms;
bool                                        __jo_profile_pixels = false;

void                                        jo_profile_count_pixels(const bool enabled)
{
    __jo_profile_pixels = enabled;
}
/* The VDP1 draws a new table since this time (master CPU ticks), see __jo_profile_vdp1_start() */
static volatile unsigned int                __jo_profile_vdp1_start_time;
static volatile bool                        __jo_profile_vdp1_drawing;
/** @brief Ticks of the last drawing of the VDP1 (see jo_profile_vdp1_last()) */
static volatile unsigned int                __jo_profile_vdp1_last = 0;

unsigned int                                jo_profile_vdp1_last(void)
{
    return (__jo_profile_vdp1_last);
}

static const char * const                   __jo_profile_engine_names[JO_PROFILE_FIRST_USER] =
{
    "frame", "callback", "w.slave", "slave", "sort", "copy", "w.vdp1", "vsync", "vdp1", "vbl x100", "commands", "polygons", "tested", "vertices", "3d vert", "3d poly", "3d clip", "3d light", "3d px"
};

/** @brief End of the drawing of the VDP1 (sprite draw end interrupt) */
static void __attribute__((interrupt_handler)) jo_profile_sprite_end_handler(void)
{
    if (__jo_profile_vdp1_drawing)
    {
        // (the master CPU: its cached address, like jo_profile_end())
        __jo_profile_vdp1_last = (jo_profile_ticks() - __jo_profile_vdp1_start_time) & 0xFFFF;
        __jo_profile_add(&__jo_profile_cpu[0], JO_PROFILE_VDP1_DRAW, __jo_profile_vdp1_last);
        __jo_profile_vdp1_drawing = false;
    }
    __jo_core_restore_scu_mask();
}

void                                        __jo_profile_vdp1_start(void)
{
    __jo_profile_vdp1_start_time = jo_profile_ticks();
    __jo_profile_vdp1_drawing = true;
}

void                                        __jo_profile_init(void)
{
    jo_memset(__jo_profile_cpu, 0, sizeof(__jo_profile_cpu));
    jo_memset(__jo_profile_last_sum, 0, sizeof(__jo_profile_last_sum));
    jo_memset(__jo_profile_used, 0, sizeof(__jo_profile_used));
    jo_memset(__jo_profile_names, 0, sizeof(__jo_profile_names));
    JO_ZERO(__jo_profile_frames);
    __jo_profile_changed = false;
    // Free running timer of the master CPU: clock / 32 (the slave CPU sets its own timer, see jo_slave_main())
    jo_time_poke_byte(RegisterTCR, (jo_time_peek_byte(RegisterTCR) & ~JO_TIME_M_CKS) | JO_PROFILE_TCR_CLOCK_32);
    // 28.6 MHz (352 and 704 pixels width) or 26.8 MHz, divided by 32
    __jo_profile_ticks_per_ms = (JO_TV_WIDTH == 352 || JO_TV_WIDTH == 704) ? 894886 : 838752;
    __jo_core_set_interrupt_handler(JO_VECTOR_SPRITE_END, jo_profile_sprite_end_handler);
    __jo_core_enable_scu_interrupts(JO_SCU_INT_SPRITE_END);
}

/*
** PROGRAM COUNTER SAMPLING
*/

/** @brief Ticks of the free running timer (clock / 32) between 2 samples: about 1 500 CPU cycles */
# define JO_PROFILE_SAMPLING_INTERVAL       "48"
/** @brief The last counter of a histogram (JO_PROFILE_SAMPLING_BINS - 1, for the assembly) */
# define JO_PROFILE_STR_(x)                 #x
# define JO_PROFILE_STR(x)                  JO_PROFILE_STR_(x)
# define JO_PROFILE_SAMPLING_LAST_BIN       JO_PROFILE_STR(JO_PROFILE_SAMPLING_BINS - 1)
/** @brief Vector of the output compare interrupt of the free running timer (user vectors: 64 to 127) */
# define JO_PROFILE_SAMPLING_VECTOR         (0x66)

/** @brief Histograms (one pointer per CPU, read by the handlers) */
unsigned int                                *__jo_profile_sampling_histogram[2] __attribute__((used, externally_visible));

/** @brief Output compare A interrupt handler of a CPU: counts the interrupted address, sets the next compare match
 *  @param NAME Name of the handler
 *  @param INDEX Index of the histogram pointer (0: master CPU, 1: slave CPU)
 *  @remarks The exception saved the PC of the interrupted code at @r15 and the SR at @(4, r15)
 *  @remarks The free running timer keeps running (the counter is not cleared on the compare match): the next match is
 *           FRC + interval, and the FRC is still usable by jo_profile_begin()/jo_profile_end()
 */
# define JO_PROFILE_SAMPLING_HANDLER(NAME, INDEX) \
__asm__( \
"    .text\n" \
"    .align 2\n" \
"    .global _" #NAME "\n" \
"_" #NAME ":\n" \
"    mov.l r0, @-r15\n" \
"    mov.l r1, @-r15\n" \
"    mov.l r2, @-r15\n" \
"    mov.l @(12, r15), r0\n"         /* PC of the interrupted code */ \
"    mov.l 1f, r1\n" \
"    sub r1, r0\n" \
"    shlr2 r0\n"                     /* / JO_PROFILE_SAMPLING_BIN_SIZE (16) */ \
"    shlr2 r0\n" \
"    mov.l 2f, r1\n" \
"    cmp/hi r1, r0\n"                /* beyond the code: the last counter */ \
"    bf 0f\n" \
"    mov r1, r0\n" \
"0:  shll2 r0\n" \
"    mov.l 3f, r1\n" \
"    mov.l @r1, r1\n" \
"    add r1, r0\n" \
"    mov.l @r0, r2\n" \
"    add #1, r2\n" \
"    mov.l r2, @r0\n" \
"    mov.l 4f, r1\n"                 /* FRT registers (TIER) */ \
"    mov.b @(2, r1), r0\n"           /* FRCH (the low byte is latched) */ \
"    extu.b r0, r2\n" \
"    shll8 r2\n" \
"    mov.b @(3, r1), r0\n"           /* FRCL */ \
"    extu.b r0, r0\n" \
"    or r0, r2\n" \
"    add #" JO_PROFILE_SAMPLING_INTERVAL ", r2\n" \
"    swap.b r2, r0\n" \
"    mov.b r0, @(4, r1)\n"           /* OCRAH */ \
"    mov r2, r0\n" \
"    mov.b r0, @(5, r1)\n"           /* OCRAL */ \
"    mov.b @(1, r1), r0\n"           /* FTCSR: clear OCFA */ \
"    and #0xF7, r0\n" \
"    mov.b r0, @(1, r1)\n" \
"    mov.l @r15+, r2\n" \
"    mov.l @r15+, r1\n" \
"    mov.l @r15+, r0\n" \
"    rte\n" \
"    nop\n" \
"    .align 2\n" \
"1:  .long 0x06004000\n" \
"2:  .long " JO_PROFILE_SAMPLING_LAST_BIN "\n" \
"3:  .long ___jo_profile_sampling_histogram + " #INDEX " * 4\n" \
"4:  .long 0xFFFFFE10\n" \
);

JO_PROFILE_SAMPLING_HANDLER(__jo_profile_sampling_master_handler, 0)
JO_PROFILE_SAMPLING_HANDLER(__jo_profile_sampling_slave_handler, 1)
extern void                                 __jo_profile_sampling_master_handler(void);
extern void                                 __jo_profile_sampling_slave_handler(void);

/** @brief Start the sampling on the current CPU
 *  @param handler Handler of this CPU
 */
static void                                 jo_profile_sampling_install(jo_interrupt_handler handler)
{
    unsigned int                            frc;

    JO_SH2_FRT_TIER = JO_SH2_FRT_TIER_NO_INTERRUPT;
    JO_SH2_VCRC = (JO_SH2_VCRC & 0xFF00) | JO_PROFILE_SAMPLING_VECTOR;
    // Highest priority: the samples are taken inside the sections that mask the interrupts below 15
    JO_SH2_IPRB = (JO_SH2_IPRB & 0xF0FF) | 0x0F00;
    __jo_core_set_interrupt_handler(JO_PROFILE_SAMPLING_VECTOR, handler);
    JO_SH2_FRT_TOCR = JO_SH2_FRT_TOCR_OCRA;
    frc = jo_time_get_frc() + 48;
    JO_SH2_FRT_OCRAH = (unsigned char)(frc >> 8);
    JO_SH2_FRT_OCRAL = (unsigned char)frc;
    JO_SH2_FRT_FTCSR = JO_SH2_FRT_FTCSR & ~JO_SH2_FRT_FTCSR_OCFA;
    JO_SH2_FRT_TIER = JO_SH2_FRT_TIER_NO_INTERRUPT | JO_SH2_FRT_TIER_OCIAE;
}

/** @brief Slave CPU: start its sampling (jo_core_exec_on_slave() callback)
 *  @remarks The slave CPU runs with all its interrupts masked (its blanking interrupts are useless): level 14 lets the
 *           sampling (level 15) through
 */
static void                                 jo_profile_sampling_slave_start(void)
{
    jo_profile_sampling_install(__jo_profile_sampling_slave_handler);
    __jo_core_set_sr(0xE0);
}

#ifdef JO_PROFILE_CALLS
/*
** Test (not for release): the calls of each function instead of the samples (compile with -finstrument-functions
** -DJO_PROFILE_CALLS): jo_profile_sampling_start() gives the histograms, each call of a function counts at its address
** (the inline functions too, at their copies). The slave CPU: its stack is under JO_WORK_RAM_SLAVE_STACK_TOP
*/
static unsigned int                         *__jo_profile_calls[2];

void __attribute__((no_instrument_function)) __cyg_profile_func_enter(void *function, void *site)
{
    register unsigned int                   sp __asm__("r15");
    unsigned int                            *histogram;
    unsigned int                            bin;

    (void)site;
    histogram = ((unsigned int * volatile *)JO_CACHE_THROUGH(__jo_profile_calls))[sp < JO_WORK_RAM_SLAVE_STACK_TOP ? 1 : 0];
    if (histogram == JO_NULL)
        return;
    bin = ((unsigned int)function - 0x06004000) >> 4;
    if (bin >= JO_PROFILE_SAMPLING_BINS)
        bin = JO_PROFILE_SAMPLING_BINS - 1;
    ++((volatile unsigned int *)JO_CACHE_THROUGH(histogram))[bin];
}

void __attribute__((no_instrument_function)) __cyg_profile_func_exit(void *function, void *site)
{
    (void)function;
    (void)site;
}
#endif

void                                        jo_profile_sampling_start(unsigned int *master_histogram, unsigned int *slave_histogram)
{
#ifdef JO_PROFILE_CALLS
    jo_memset(master_histogram, 0, JO_PROFILE_SAMPLING_BINS * sizeof(*master_histogram));
    if (slave_histogram != JO_NULL)
        jo_memset(slave_histogram, 0, JO_PROFILE_SAMPLING_BINS * sizeof(*slave_histogram));
    ((unsigned int * volatile *)JO_CACHE_THROUGH(__jo_profile_calls))[1] = slave_histogram;
    ((unsigned int * volatile *)JO_CACHE_THROUGH(__jo_profile_calls))[0] = master_histogram;
    return;
#endif

    jo_memset(master_histogram, 0, JO_PROFILE_SAMPLING_BINS * sizeof(*master_histogram));
    __jo_profile_sampling_histogram[0] = master_histogram;
    jo_profile_sampling_install(__jo_profile_sampling_master_handler);
    if (slave_histogram == JO_NULL)
        return;
    jo_memset(slave_histogram, 0, JO_PROFILE_SAMPLING_BINS * sizeof(*slave_histogram));
    __jo_profile_sampling_histogram[1] = slave_histogram;
    jo_core_exec_on_slave(jo_profile_sampling_slave_start);
}

/** @brief Slave CPU: stop its sampling (jo_core_exec_on_slave() callback) */
static void                                 jo_profile_sampling_slave_stop(void)
{
    JO_SH2_FRT_TIER = JO_SH2_FRT_TIER_NO_INTERRUPT;
}

void                                        jo_profile_sampling_stop(void)
{
#ifdef JO_PROFILE_CALLS
    ((unsigned int * volatile *)JO_CACHE_THROUGH(__jo_profile_calls))[0] = JO_NULL;
    ((unsigned int * volatile *)JO_CACHE_THROUGH(__jo_profile_calls))[1] = JO_NULL;
    return;
#endif
    JO_SH2_FRT_TIER = JO_SH2_FRT_TIER_NO_INTERRUPT;
    if (__jo_profile_sampling_histogram[1] != JO_NULL)
        jo_core_exec_on_slave(jo_profile_sampling_slave_stop);
}

void                                        __jo_profile_end_frame(void)
{
    volatile jo_profile_cpu                 *cpu;
    register int                            slot;
    register int                            c;
    unsigned int                            sum;
    unsigned int                            maximum;

    if (++__jo_profile_frames < JO_PROFILE_FRAMES)
        return;
    JO_ZERO(__jo_profile_frames);
    for (JO_ZERO(slot); slot < JO_PROFILE_COUNT; ++slot)
    {
        JO_ZERO(sum);
        JO_ZERO(maximum);
        for (JO_ZERO(c); c < 2; ++c)
        {
            // (the measures of the master CPU through its cache, the ones of the slave CPU without it)
            cpu = c == 0 ? (volatile jo_profile_cpu *)&__jo_profile_cpu[0] : (volatile jo_profile_cpu *)JO_CACHE_THROUGH(&__jo_profile_cpu[1]);
            sum += cpu->sum[slot] - __jo_profile_last_sum[c][slot];
            __jo_profile_last_sum[c][slot] = cpu->sum[slot];
            if (cpu->max[slot] > maximum)
                maximum = cpu->max[slot];
            // (the maximums of the master CPU reset here, the ones of the slave CPU by itself at its next job)
            if (c == 0)
                JO_ZERO(cpu->max[slot]);
        }
        if (sum != 0)
            __jo_profile_used[slot] = true;
        if (JO_PROFILE_IS_COUNT(slot))
        {
            __jo_profile_average[slot] = sum / JO_PROFILE_FRAMES;
            __jo_profile_maximum[slot] = maximum;
        }
        else
        {
            // Ticks to microseconds
            __jo_profile_average[slot] = (unsigned int)(((unsigned long long)sum * 1000000) / ((unsigned long long)__jo_profile_ticks_per_ms * JO_PROFILE_FRAMES));
            __jo_profile_maximum[slot] = (unsigned int)(((unsigned long long)maximum * 1000000) / __jo_profile_ticks_per_ms);
        }
    }
    *(volatile bool *)JO_CACHE_THROUGH(&__jo_profile_slave_reset) = true;
    __jo_profile_changed = true;
}

void                                        __jo_profile_slave_job(void)
{
    register int                            slot;

    if (!*(volatile bool *)JO_CACHE_THROUGH(&__jo_profile_slave_reset))
        return;
    for (JO_ZERO(slot); slot < JO_PROFILE_COUNT; ++slot)
        JO_ZERO(__jo_profile_cpu[1].max[slot]);
    *(volatile bool *)JO_CACHE_THROUGH(&__jo_profile_slave_reset) = false;
}

bool                                        jo_profile_get(const jo_profile_slot slot, unsigned int *average, unsigned int *maximum)
{
    if (slot < 0 || slot >= JO_PROFILE_COUNT || !__jo_profile_used[slot])
        return (false);
    *average = __jo_profile_average[slot];
    *maximum = __jo_profile_maximum[slot];
    return (true);
}

void                                        jo_profile_set_name(const jo_profile_slot slot, const char * const name)
{
    register int                            i;

    if (slot < JO_PROFILE_FIRST_USER || slot >= JO_PROFILE_COUNT)
        return;
    for (JO_ZERO(i); i < JO_PROFILE_NAME_LENGTH && name[i]; ++i)
        __jo_profile_names[slot][i] = name[i];
    __jo_profile_names[slot][i] = '\0';
}

void                                        jo_profile_print(const int x, const int y)
{
#ifdef JO_COMPILE_WITH_PRINTF_SUPPORT
    register int                            slot;
    int                                     line;
    const char                              *name;
    char                                    user_name[6];

    if (!__jo_profile_changed)
        return;
    __jo_profile_changed = false;
    line = y;
    jo_printf(x, line++, "%-8s %6s %6s", "", "avg", "max");
    for (JO_ZERO(slot); slot < JO_PROFILE_COUNT && line < y + 24; ++slot)
    {
        if (!__jo_profile_used[slot])
            continue;
        if (slot < JO_PROFILE_FIRST_USER)
            name = __jo_profile_engine_names[slot];
        else if (__jo_profile_names[slot][0])
            name = __jo_profile_names[slot];
        else
        {
            user_name[0] = 'u';
            user_name[1] = 's';
            user_name[2] = 'e';
            user_name[3] = 'r';
            user_name[4] = '0' + (slot - JO_PROFILE_FIRST_USER);
            user_name[5] = '\0';
            name = user_name;
        }
        jo_printf(x, line++, "%-8s %6d %6d", name, __jo_profile_average[slot], __jo_profile_maximum[slot]);
    }
#else
    JO_UNUSED_ARG(x);
    JO_UNUSED_ARG(y);
#endif
}

#endif /* !JO_COMPILE_WITH_PROFILER_SUPPORT */

/*
** END OF FILE
*/
