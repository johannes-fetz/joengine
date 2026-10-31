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
/** @file core.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine core function prototypes
 *  @bug No known bugs.
 */

#ifndef __JO_CORE_H__
# define __JO_CORE_H__

#ifdef JO_DEBUG

/** @brief Error handling (Internal usage)
 *  @warning MC Hammer: don't touch this
 */
extern char		__jo_last_error[JO_PRINTF_BUF_SIZE];
void    __jo_core_error(char *message, const char *function);
/** @brief The message formatted, then __jo_core_error() (Internal usage): one call at each of the hundreds of
 *  jo_core_error() (the formatting at each of them was about 4 KB of code), out of the hot code (cold) */
void    __jo_core_errorf(const char *function, const char *format, ...) __attribute__((cold, noinline));
# define jo_core_error(...)		__jo_core_errorf(__FUNCTION__, __VA_ARGS__)

#endif

/** @brief Jo Engine Version */
# define JO_ENGINE_VERSION  (2025.1)

/*
** TYPEDEFS
*/
/** @brief Function prototype for jo_core_add_callback() */
typedef void	(*jo_event_callback)(void);

/** @brief Function prototype for interrupt handlers (Internal usage) */
typedef void	(*jo_interrupt_handler)(void);

#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT

/** @brief Function prototype for jo_core_exec_on_slave() */
typedef void	(*jo_slave_callback)(void);

#endif

/** @brief Init the engine
 *  @param back_color Default background color (colors.h)
 *  @warning Must be the first function called in jo_main()
 */
void	jo_core_init(const jo_color back_color);

/** @brief Run the game
 *  @warning Must be the last function called in jo_main()
 */
void	jo_core_run(void);

/** @brief Add a callback during VBLANK interruption
 *  @param callback Function name with no parameters and no return value
 *  @return VBLANK Event Id (usefull for jo_core_remove_vblank_callback())
 */
int     jo_core_add_vblank_callback(const jo_event_callback callback);

/** @brief Remove a callback during VBLANK interruption
 *  @param vblank_event_id Value returned by jo_core_add_vblank_callback()
 */
void    jo_core_remove_vblank_callback(const int vblank_event_id);

/** @brief Add a callback in the game loop on main CPU
 *  @param callback Function name with no parameters and no return value
 *  @warning Must be called before jo_core_run()
 *  @return Event Id (usefull for jo_core_remove_callback())
 */
int     jo_core_add_callback(const jo_event_callback callback);

/** @brief Remove a callback in the game loop from on main CPU
 *  @param event_id Value returned by jo_core_add_callback()
 */
void    jo_core_remove_callback(const int event_id);

#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT

/** @brief Add a callback in the game loop on slave CPU
 *  @param callback Function name with no parameters and no return value
 *  @warning Must be called before jo_core_run()
 *  @return Event Id (usefull for jo_core_remove_slave_callback())
 */
int     jo_core_add_slave_callback(const jo_event_callback callback);

/** @brief Remove a callback in the game loop from on slave CPU
 *  @param event_id Value returned by jo_core_add_slave_callback()
 */
void    jo_core_remove_slave_callback(const int event_id);

#endif

/** @brief Add a callback when A+B+C+Start is pressed on the first control pad
 *  @param callback Function name with no parameters and no return value
 */
void    jo_core_set_restart_game_callback(const jo_event_callback callback);

/** @brief Return to the boot menu
 */
void    jo_goto_boot_menu(void);

#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT

/** @brief Execute a callback on the slave CPU
 *  @param callback Function name with no parameters and no return value
 *  @remarks The callback is added to the jobs of the slave CPU: the master CPU doesn't wait (unless 8 jobs are waiting).
 *           The slave CPU may first finish a job of the engine (the VDP1 commands of the previous frame are sorted and
 *           copied by the slave CPU, see JO_COMPILE_WITH_DUAL_CPU_SUPPORT)
 *  @remarks If the slave CPU didn't start, the callback is executed on the master CPU
 */
void    jo_core_exec_on_slave(jo_slave_callback callback);

/** @brief Wait until all the callbacks given to jo_core_exec_on_slave() are completed
 *  @remarks Then the master CPU can read the data written by the slave CPU (its cache is purged)
 */
void    jo_core_wait_for_slave(void);

/** @brief Balance of the CPUs in the last frame (between the 2 last calls of jo_core_wait_for_slave())
 *  @return Microseconds the slave CPU spent waiting for a job (it can take more work), negative: microseconds the
 *  master CPU waited for the slave CPU in jo_core_wait_for_slave() (the slave CPU has too much work)
 *  @remarks To move the drawing of some objects from a CPU to the other: the frame takes the time of the slowest CPU
 */
int     jo_core_slave_idle_time(void);

/** @brief Function of a job of the slave CPU (Internal usage) */
typedef void	(*jo_slave_job_function)(void *argument);

/** @brief Check if the slave CPU started (Internal usage)
 *  @return false if the slave CPU didn't start (the jobs are executed by the master CPU)
 */
bool    __jo_core_is_slave_running(void);

/** @brief Add a job to the queue of the slave CPU (Internal usage)
 *  @param function Function executed by the slave CPU
 *  @param argument Argument of the function
 *  @remarks The jobs are executed in order. The master CPU waits only if the queue is full.
 *           If the slave CPU didn't start, the function is executed now by the master CPU
 *  @remarks The slave CPU purges its cache before each job: the job reads the data written by the master CPU before this call
 */
void    __jo_core_add_slave_job(const jo_slave_job_function function, void *argument);

#endif

/** @brief Disable reset button
 */
static  __jo_force_inline void jo_core_disable_reset(void)
{
    jo_smpc_begin_command();
    jo_smpc_end_command(DisableReset);
}

/** @brief Enable reset button
 */
static  __jo_force_inline void jo_core_enable_reset(void)
{
    jo_smpc_begin_command();
    jo_smpc_end_command(EnableReset);
}

/** @brief Turn OFF scroll drawing processing in the monitor
 */
void    jo_core_tv_off(void);

/** @brief Turn ON scroll drawing processing in the monitor
 */
void    jo_core_tv_on(void);

/** @brief Get the refresh rate of the console
 *  @return 50 for a PAL console, 60 for a NTSC console
 *  @remarks This is the number of VBLANK per second (see jo_core_add_vblank_callback())
 */
static  __jo_force_inline int      jo_core_get_refresh_rate(void)
{
    return ((JO_VDP2_TVSTAT & JO_VDP2_TVSTAT_PAL) ? 50 : 60);
}

/** @brief Exit program and return to the Saturn multiplayer (CD player) screen
 *  @warning Works but I didn't found documentation about it
 *  @todo Investigate and give feedback to https://segaxtreme.net/threads/best-way-to-debug-saturn-bios.18644/
 */
static  __jo_force_inline void jo_core_exit_to_multiplayer(void)
{

    (**(void(**)(void))0x600026C)();
}

/** @brief Restart the Saturn
 *  @warning Doesn't work in Yabause
 */
static  __jo_force_inline void jo_core_restart_saturn(void)
{
    jo_smpc_begin_command();
    jo_smpc_end_command(SystemReset);
}

/** @brief Set scroll screen order between them
 *  @param screen1 First screen (high priority)
 *  @param args Other screens
 *  @remarks Default Jo Engine screen priority : NBG0 > SPR0 > SPR1 > RBG0 > NBG1 > NBG2 > NBG3
 */
#define                             jo_core_set_screens_order(screen1, args...) __jo_core_set_screens_order(screen1, ## args, ~0);

#ifdef JO_DEBUG

/** @brief Stop the execution until someone press Start
 */
void    jo_core_suspend(void);

/** @brief Get last error
 *  @return Formated error
 */
static  __jo_force_inline char * jo_get_last_error(void)
{
    return (__jo_last_error);
}

/** @brief Dump VDP1 registers
 */
void                jo_dump_vdp1_registers(void);

/** @brief Dump VDP2 registers
 */
void                jo_dump_vdp2_registers(void);

#endif

/** @brief Screen order (Internal usage)
 *  @warning Use jo_core_set_screens_order instead
 */
void                                __jo_core_set_screens_order(jo_scroll_screen screen1, ...);

/*
** Internal usage
*/

/** @brief Number of VBLANK since startup (Internal usage) */
extern volatile unsigned int        __jo_core_vblank_count;

/** @brief Get the number of VBLANK since startup (60 per second in NTSC, 50 in PAL)
 *  @return VBLANK count (it is incremented by the VBLANK interrupt)
 *  @remarks Measure the time of a frame or the frames per second without a timer
 */
static  __jo_force_inline unsigned int  jo_core_get_vblank_count(void)
{
    return (__jo_core_vblank_count);
}

/** @brief Wait for the end of the current frame (Internal usage)
 *  @remarks Send the VDP1 commands and wait for JO_FRAMERATE VBLANK
 */
void                                __jo_core_synch(void);

/** @brief Set an interrupt handler in the vector table (Internal usage)
 *  @param vector Interrupt vector number (JO_VECTOR_VBLANK_IN, etc)
 *  @param handler Function declared with __attribute__((interrupt_handler))
 */
void                                __jo_core_set_interrupt_handler(const unsigned int vector, jo_interrupt_handler handler);

/** @brief Unmask SCU interrupts (Internal usage)
 *  @param mask Interrupts (JO_SCU_INT_VBLANK_IN, etc)
 */
void                                __jo_core_enable_scu_interrupts(const unsigned int mask);

/** @brief Write the SCU interrupt mask again at the end of a SCU interrupt handler (Internal usage)
 *  @remarks When the CPU takes a SCU interrupt, the SCU masks all its interrupts (JO_SCU_IMS = JO_SCU_INT_ALL):
 *           without this call, no other SCU interrupt happens (the BIOS handlers do the same)
 */
void                                __jo_core_restore_scu_mask(void);

/** @brief Get CPU status register (Internal usage)
 *  @return Status register value
 */
static  __jo_force_inline unsigned int  __jo_core_get_sr(void)
{
    unsigned int                    sr;

    __asm__ volatile ("stc sr, %0" : "=r" (sr));
    return (sr);
}

/** @brief Set CPU status register (Internal usage)
 *  @param sr Status register value (JO_SH2_SR_INTERRUPT_MASK bits: interrupt mask level)
 */
static  __jo_force_inline void      __jo_core_set_sr(const unsigned int sr)
{
    __asm__ volatile ("ldc %0, sr" : : "r" (sr) : "memory");
}

/** @brief Mask all interrupts (Internal usage)
 *  @return Previous status register for __jo_core_restore_interrupts()
 */
static  __jo_force_inline unsigned int  __jo_core_disable_interrupts(void)
{
    unsigned int                    sr;

    sr = __jo_core_get_sr();
    __jo_core_set_sr(sr | JO_SH2_SR_INTERRUPT_MASK);
    return (sr);
}

/** @brief Restore interrupts masked by __jo_core_disable_interrupts() (Internal usage)
 *  @param sr Value returned by __jo_core_disable_interrupts()
 */
static  __jo_force_inline void      __jo_core_restore_interrupts(const unsigned int sr)
{
    __jo_core_set_sr(sr);
}

#endif /* !__JO_CORE_H__ */

/*
** END OF FILE
*/
