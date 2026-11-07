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
 *  @remarks With JO_DEBUG, a NULL pointer trap of the master CPU (its user break controller) stops each data access to
 *  the first 4 KB of the memory after this call: the error screen shows the PC (the instruction after the access) and the
 *  PR (return address of its function), to find in the map file. The emulators (Mednafen, Yabause) do not emulate it
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
 *  @remarks Called while the slave CPU runs its callbacks (from a callback of either CPU), the callback is removed
 *  after they are done
 */
void    jo_core_remove_slave_callback(const int event_id);

#endif

/** @brief Add a callback when A+B+C+Start is pressed on the first control pad (called at each frame while they are pressed)
 *  @param callback Function name with no parameters and no return value (JO_NULL: back to the multiplayer screen of the
 *  BIOS, see jo_core_exit_to_multiplayer())
 */
void    jo_core_set_restart_game_callback(const jo_event_callback callback);

/** @brief Add a callback when the tray of the CD is opened
 *  @param callback Function name with no parameters and no return value (JO_NULL, by default: back to the multiplayer
 *  screen of the BIOS, see jo_core_exit_to_multiplayer())
 */
void    jo_core_set_cd_tray_open_callback(const jo_event_callback callback);

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

/** @brief The time the master CPU waited for the slave CPU in the last jo_core_wait_for_slave()
 *  @return Microseconds (0: the slave CPU was done first)
 *  @remarks With several jobs of the slave CPU per frame (a split screen: a view after the other), the balance of each
 *  one: jo_core_slave_idle_time() also counts the time the slave CPU waited before the job (the master CPU prepared it)
 */
int     jo_core_master_wait_time(void);

/** @brief Function of a job of the slave CPU (Internal usage) */
typedef void	(*jo_slave_job_function)(void *argument);

#endif

/** @brief Disable reset button
 *  @remarks The calls are nested: the reset button is enabled again when each call has its jo_core_enable_reset(). The backup
 *  functions disable it while they write (a reset would corrupt the backup)
 */
static  __jo_force_inline void jo_core_disable_reset(void)
{
    __jo_smpc_set_reset_button(false);
}

/** @brief Enable reset button (see jo_core_disable_reset())
 */
static  __jo_force_inline void jo_core_enable_reset(void)
{
    __jo_smpc_set_reset_button(true);
}

/** @brief Turn OFF scroll drawing processing in the monitor
 */
void    jo_core_tv_off(void);

/** @brief Turn ON scroll drawing processing in the monitor
 */
void    jo_core_tv_on(void);

/** @brief The screen now (Internal usage, see jo_core_set_screen_resolution()) */
typedef struct
{
    short   width;
    short   height;
    short   width_2;
    short   height_2;
    /** @brief The size of the screen in pixels of NBG0 and NBG1 (jo_printf(), the NBG1 bitmap): they are zoomed 2 times
     *  in the hi-res and interlace modes */
    short   background_width;
    short   background_height;
    /** @brief 640 or 704 pixels: the frame buffer of the VDP1 has 8 bits per pixel */
    bool    hires;
    /** @brief 448 to 512 lines: double density interlace */
    bool    interlace;
    /** @brief 352 or 704 pixels: the system clock is 28.6 MHz (26.8 MHz otherwise) */
    bool    clock_352;
    /** @brief Single density interlace (see jo_core_set_screen_single_interlace()) */
    bool    single_interlace;
}           __jo_screen_mode;

/** @brief The screen now (Internal usage) */
extern __jo_screen_mode     __jo_screen;

/** @brief Change the resolution of the screen while the game runs
 *  @param width 320, 352, 640 or 704 pixels
 *  @param height 224 or 240 lines (256 in PAL), or 448 or 480 (512 in PAL): double density interlace
 *  @return true if succeed, false if the resolution is not supported
 *  @remarks The text (jo_printf()) and the NBG1 bitmap keep their size: they are zoomed 2 times in the 640 and 704 pixels
 *  modes and in the interlace modes. The sprites and the 3D are drawn with all the pixels of the screen
 *  @remarks In the 640 and 704 pixels modes, the VDP1 draws 8 bits per pixel: only the palette sprites (8 bits images) and
 *  the colors of a palette (see jo_core_set_screen_palette()) are displayed, not the RGB colors
 *  @remarks The 3D has one projection distance for the 2 axes (from the width): in the modes 640 x 224 / 240 and 320 x 448 /
 *  480, whose pixels are not square, the 3D is stretched 2 times vertically or horizontally
 *  @remarks 320 and 640 pixels use the 26.8 MHz clock, 352 and 704 pixels the 28.6 MHz clock: the clock is chosen at the
 *  start by JO_SCREEN_WIDTH (makefile) and the width can only change inside its family (false otherwise)
 *  @remarks The windows (jo_3d_window()) and the VDP2 windows (jo_vdp2_set_window()) are reset to the whole screen
 */
bool    jo_core_set_screen_resolution(const int width, const int height);

/** @brief Single density interlace: the image of 224 to 256 lines is displayed in the 2 fields, the second one half a
 *  line lower (the look of the games that use it on a CRT: no dark lines between the lines, a slight flicker)
 *  @param enabled true to enable it, false for the progressive display (default)
 *  @remarks Only in the modes of 224 to 256 lines: the 448 to 512 lines modes are always double density interlace
 */
void    jo_core_set_screen_single_interlace(const bool enabled);

/** @brief Get the width of the screen
 *  @return Width in pixels (JO_TV_WIDTH at the start, see jo_core_set_screen_resolution())
 */
static  __jo_force_inline int      jo_core_get_screen_width(void)
{
    return (__jo_screen.width);
}

/** @brief Get the height of the screen
 *  @return Height in lines (JO_TV_HEIGHT at the start, see jo_core_set_screen_resolution())
 */
static  __jo_force_inline int      jo_core_get_screen_height(void)
{
    return (__jo_screen.height);
}

/** @brief Choose the palette of the sprites in the 640 and 704 pixels modes (the frame buffer of the VDP1 has 8 bits per
 *  pixel: one palette of 256 colors for all the sprites)
 *  @param palette_id Palette (jo_create_palette())
 *  @remarks A polygon of color palette_id * 256 + k (k from 1 to 255) has the color k of the palette in all the modes
 */
void    jo_core_set_screen_palette(const int palette_id);

/** @brief Get the refresh rate of the console
 *  @return 50 for a PAL console, 60 for a NTSC console
 *  @remarks This is the number of VBLANK per second (see jo_core_add_vblank_callback())
 */
static  __jo_force_inline int      jo_core_get_refresh_rate(void)
{
    return ((JO_VDP2_TVSTAT & JO_VDP2_TVSTAT_PAL) ? 50 : 60);
}

/** @brief Exit program and return to the Saturn multiplayer (CD player) screen
 *  @remarks Routine of the Boot ROM (SYS_EXECDMP). Called when the tray of the CD is opened and on A+B+C+Start by default
 *  (see jo_core_set_cd_tray_open_callback() and jo_core_set_restart_game_callback())
 */
static  __jo_force_inline void jo_core_exit_to_multiplayer(void)
{
    // (the NULL pointer trap of JO_DEBUG off: the Boot ROM reads its own data, see jo_core_init())
    *(volatile unsigned short *)0xFFFFFF48 = 0;
    (**(void(**)(void))0x600026C)();
}

/** @brief Restart the Saturn
 *  @warning Doesn't work in Yabause
 */
static  __jo_force_inline void jo_core_restart_saturn(void)
{
    // (after the INTBACK of the frame: a command sent during an INTBACK is lost)
    __jo_smpc_command(SystemReset);
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

/** @brief Function called by jo_core_error() (see jo_core_set_error_handler())
 *  @param message The error
 *  @param function The function of the error
 */
typedef void    (*jo_core_error_handler)(const char *message, const char *function);

/** @brief Call a function instead of the error screen (JO_DEBUG): automated tests count the errors and go on
 *  @param handler Function, JO_NULL: the error screen (the default)
 */
void    jo_core_set_error_handler(const jo_core_error_handler handler);

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

#else

/** @brief Get last error (without JO_DEBUG, the errors are not recorded)
 *  @return An empty string
 */
static  __jo_force_inline char * jo_get_last_error(void)
{
    return ("");
}

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

/** @brief Frames per second shown (Internal usage, see jo_core_get_fps()) */
extern int                          __jo_core_fps;

/** @brief Count of late frames (Internal usage, see jo_core_get_late_frame_count()) */
extern unsigned int                 __jo_core_late_frames;

/** @brief Get the frames per second really shown, measured over the last 2 seconds
 *  @return Frames per second (60 / JO_FRAMERATE in NTSC when no frame is late), 0 during the first 2 seconds
 *  @remarks jo_printf(0, 27, "%d FPS  ", jo_core_get_fps());
 */
static  __jo_force_inline int       jo_core_get_fps(void)
{
    return (__jo_core_fps);
}

/** @brief Get the count of the late frames since startup
 *  @return Frames that took more VBLANK than JO_FRAMERATE (the game missed the end of a frame: it slows down)
 *  @remarks The first frame is not counted (the loading before it)
 */
static  __jo_force_inline unsigned int  jo_core_get_late_frame_count(void)
{
    return (__jo_core_late_frames);
}

#endif /* !__JO_CORE_H__ */

/*
** END OF FILE
*/
