/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2020, Johannes Fetz (johannesfetz@gmail.com)
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
# define jo_core_error(...)		do {sprintf(__jo_last_error, __VA_ARGS__); __jo_core_error(__jo_last_error, __FUNCTION__);} while(0)

#endif

/** @brief Jo Engine Version */
# define JO_ENGINE_VERSION  (10.0)

/*
** TYPEDEFS
*/
/** @brief Function prototype for jo_core_add_callback() */
typedef void	(*jo_event_callback)(void);

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
 */
void    jo_core_exec_on_slave(jo_slave_callback callback);

/** @brief Wait until slave callback is completed
 */
void    jo_core_wait_for_slave(void);

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
static  __jo_force_inline void      jo_core_tv_off(void)
{
    slTVOff();
}

/** @brief Turn ON scroll drawing processing in the monitor
 */
static  __jo_force_inline void      jo_core_tv_on(void)
{
    slTVOn();
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

#endif /* !__JO_CORE_H__ */

/*
** END OF FILE
*/
