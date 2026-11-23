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
#include <stdarg.h>
#include "jo/conf.h"
#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/math.h"
#include "jo/tools.h"
#include "jo/vdp1_command_pipeline.h"
#include "jo/core.h"
#include "jo/time.h"
#include "jo/audio.h"
#include "jo/colors.h"
#include "jo/palette.h"
#include "jo/sprites.h"
#include "jo/input.h"
#include "jo/console.h"
#include "jo/malloc.h"
#include "jo/video.h"
#include "jo/list.h"
#include "jo/vdp2.h"
#include "jo/storyboard.h"
#include "jo/keyboard.h"
#include "jo/3d.h"
#include "jo/profile.h"
#include "internal/smpc.h"
#include "internal/core.h"
#include "internal/vdp1_command_pipeline.h"
#include "internal/profile.h"

/*
** DEFINITIONS
*/

extern jo_texture_definition                __jo_sprite_def[];
extern int                                  __jo_gouraud_shading_runtime_index;
#ifdef JO_COMPILE_WITH_FS_SUPPORT
extern unsigned int                         __jo_fs_background_job_count;
#endif
extern unsigned int                         _bstart;
extern unsigned int                         _bend;
/* End of the engine work area (see jo.linker) */
extern unsigned int                         _jo_work_end;
extern void                                 jo_main(void);
void                                        __jo_init_vdp2(const jo_color back_color);
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
extern jo_list                              __slave_callbacks;
extern volatile bool                        __jo_slave_callbacks_running;
void                                        __jo_slave_callbacks_done(void);
void                                        jo_core_slave_init(void);
void                                        jo_core_exec_on_slave(jo_slave_callback callback);
#endif
#ifdef JO_COMPILE_WITH_STORYBOARD_SUPPORT
extern jo_list                              __storyboards;
void                                        jo_execute_storyboards(void);
void                                        jo_init_storyboards(void);
#endif
#ifdef JO_COMPILE_WITH_VIDEO_SUPPORT
bool                                        jo_video_init(void);
#endif
#ifdef JO_COMPILE_WITH_AUDIO_SUPPORT
void                                        jo_audio_init(void);
#endif
#ifdef JO_COMPILE_WITH_3D_SUPPORT
void                                        jo_3d_init(void);
#endif
void                                        jo_vdp1_buffer_init(void);
void                                        jo_vdp1_buffer_reset(void);
void                                        jo_vdp1_flush(void);
void                                        jo_input_init(void);
#ifdef JO_COMPILE_WITH_FS_SUPPORT
int						                    jo_fs_init(void);
#endif
void                                        jo_fs_do_background_jobs(void);
void                                        jo_sprite_init(void);
void                                        jo_time_init(unsigned char mode);
void                                        jo_get_inputs_vblank(void);
void                                        __jo_input_frame_start(void);
void                                        __jo_smpc_init(void);
void                                        __jo_smpc_vblank_in(void);
void                                        __jo_vdp2_init_registers(void);
void                                        __jo_vdp2_vblank(void);
void                                        __jo_vdp2_end_frame(void);
void                                        __jo_time_vblank(void);
void                                        __jo_vdp2_set_tv_display(const bool on);
void                                        __jo_vdp2_set_priority(const jo_scroll_screen screen, const unsigned short priority);
#ifdef JO_COMPILE_WITH_3D_SUPPORT
void                                        __jo_3d_end_frame(void);
void                                        __jo_3d_reset_matrix(void);
#endif

#ifdef JO_DEBUG
char                                        __jo_last_error[JO_PRINTF_BUF_SIZE];
#endif

/** @brief Number of VBLANK since startup */
volatile unsigned int                       __jo_core_vblank_count = 0;
static unsigned int                         __jo_core_last_synch_vblank = 0;
volatile bool                               __jo_core_frame_open = false;
int                                         __jo_core_fps = 0;
unsigned int                                __jo_core_late_frames = 0;
static unsigned int                         __jo_core_fps_frames = 0;
static unsigned int                         __jo_core_fps_vblank = 0;
static jo_list                             __vblank_callbacks;
static jo_list                             __callbacks;
static jo_event_callback                   __jo_restart_game_callback = JO_NULL;
__jo_screen_mode                            __jo_screen =
{
    JO_TV_WIDTH, JO_TV_HEIGHT, JO_TV_WIDTH_2, JO_TV_HEIGHT_2,
    JO_TV_WIDTH > 352 ? JO_TV_WIDTH_2 : JO_TV_WIDTH, JO_TV_HEIGHT > 256 ? JO_TV_HEIGHT_2 : JO_TV_HEIGHT,
    JO_TV_WIDTH > 352, JO_TV_HEIGHT > 256, JO_TV_WIDTH == 352 || JO_TV_WIDTH == 704, false
};
void                                        __jo_vdp1_set_screen_mode(void);
void                                        __jo_vdp2_set_screen_mode(void);
void                                        __jo_vdp2_update_tv_mode(void);
#ifdef JO_COMPILE_WITH_3D_SUPPORT
void                                        __jo_3d_set_screen_mode(void);
#endif
/** @brief The music streamed from the CD (see jo_audio_play_music()): its samples at each VBLANK, its reads at each frame */
void                                        (*__jo_core_music_vblank)(void) = JO_NULL;
void                                        (*__jo_core_music_update)(void) = JO_NULL;
static jo_event_callback                   __jo_cd_tray_open_callback = JO_NULL;

/** @brief CD block: HIRQ register and its "disc changed" flag (set when the tray opens), CR1 (drive status in its high byte) */
# define JO_CORE_CD_HIRQ                    (*(volatile unsigned short *)0x25890008)
# define JO_CORE_CD_HIRQ_DCHG               (0x0020)
# define JO_CORE_CD_CR1                     (*(volatile unsigned short *)0x25890018)
/** @brief Drive status: tray open, no disc */
# define JO_CORE_CD_STATUS_OPEN             (0x06)
# define JO_CORE_CD_STATUS_NO_DISC          (0x07)

#ifdef JO_DEBUG

/*
** NULL pointer trap: the user break controller (UBC) of the master SH-2 stops each data access to the first 4 KB of the
** memory (a NULL pointer, or a small offset from it: a field of a NULL structure)
*/
# define JO_SH2_UBC_BARAH                   (*(volatile unsigned short *)0xFFFFFF40)
# define JO_SH2_UBC_BARAL                   (*(volatile unsigned short *)0xFFFFFF42)
# define JO_SH2_UBC_BAMRAH                  (*(volatile unsigned short *)0xFFFFFF44)
# define JO_SH2_UBC_BAMRAL                  (*(volatile unsigned short *)0xFFFFFF46)
# define JO_SH2_UBC_BBRA                    (*(volatile unsigned short *)0xFFFFFF48)
# define JO_SH2_UBC_BBRB                    (*(volatile unsigned short *)0xFFFFFF68)
# define JO_SH2_UBC_BRCR                    (*(volatile unsigned short *)0xFFFFFF78)
/** @brief Break on CPU cycles (01), data accesses (10), reads and writes (11), any size (00) */
# define JO_SH2_UBC_BBR_CPU_DATA_READ_WRITE (0x006C)
/** @brief Address mask: the 12 low bits are ignored (0x00000000 to 0x00000FFF) */
# define JO_SH2_UBC_NULL_MASK               (0x0FFF)
/** @brief Vector of the user break interrupt */
# define JO_VECTOR_USER_BREAK               (12)

/** @brief Accesses trapped (0), and the first one: PC (1, the instruction after the access) and PR (2, return address of its
 *  function) */
volatile unsigned int                       __jo_core_null_access[3] __attribute__((used, externally_visible)) = {0, 0, 0};

/** @brief User break interrupt: counts the access, keeps the PC and the PR of the first one, then the program goes on (the
 *  error is shown at the next frame, see jo_core_check_null_access()) */
void                                        __jo_core_user_break_handler(void);
__asm__(
"    .text\n"
"    .align 2\n"
"    .global ___jo_core_user_break_handler\n"
"___jo_core_user_break_handler:\n"
"    mov.l r0, @-r15\n"
"    mov.l r1, @-r15\n"
"    mov.l r2, @-r15\n"
"    mov.l 1f, r1\n"                   /* count */
"    mov.l @r1, r0\n"
"    add #1, r0\n"
"    mov.l r0, @r1\n"
"    cmp/eq #1, r0\n"
"    bf 3f\n"
"    mov.l @(12, r15), r0\n"           /* PC pushed by the exception (above r2, r1, r0) */
"    mov.l r0, @(4, r1)\n"
"    sts pr, r0\n"
"    mov.l r0, @(8, r1)\n"
"3:\n"
"    mov.l 4f, r1\n"                   /* BRCR: clear the condition match flags (bits 15 and 14) */
"    mov.w @r1, r0\n"
"    mov.w 5f, r2\n"
"    and r2, r0\n"
"    mov.w r0, @r1\n"
"    mov.l @r15+, r2\n"
"    mov.l @r15+, r1\n"
"    mov.l @r15+, r0\n"
"    rte\n"
"    nop\n"
"    .align 2\n"
"1:  .long ___jo_core_null_access\n"
"4:  .long 0xFFFFFF78\n"
"5:  .word 0x3FFF\n"
"    .align 2\n"
);

#ifdef JO_CORE_EXCEPTION_SCREEN
/** @brief The handlers of the CPU exceptions of the master CPU (see __jo_core_exception_screen()) */
static void                                 jo_core_init_exceptions(void);
#ifdef JO_CORE_WATCHDOG
/** @brief No frame ended for this time: the program is stuck (a long loading without a frame must be shorter) */
# ifndef JO_CORE_WATCHDOG_SECONDS
#  define JO_CORE_WATCHDOG_SECONDS          (15)
# endif
static void                                 jo_core_watchdog_screen(const void * const corrupt, const unsigned int * const previous) __attribute__((noreturn, noinline, cold));
#endif
#endif

/** @brief Start the NULL pointer trap of the master CPU */
static void                                 jo_core_init_null_trap(void)
{
    __jo_core_set_interrupt_handler(JO_VECTOR_USER_BREAK, (jo_interrupt_handler)__jo_core_user_break_handler);
    JO_SH2_UBC_BBRA = 0;
    JO_SH2_UBC_BBRB = 0;
    JO_SH2_UBC_BRCR = 0;
    JO_SH2_UBC_BARAH = 0;
    JO_SH2_UBC_BARAL = 0;
    JO_SH2_UBC_BAMRAH = 0;
    JO_SH2_UBC_BAMRAL = JO_SH2_UBC_NULL_MASK;
    JO_SH2_UBC_BBRA = JO_SH2_UBC_BBR_CPU_DATA_READ_WRITE;
}

/** @brief An access to the first 4 KB of the memory since the start: the error screen */
static void                                 jo_core_check_null_access(void)
{
    static bool                             shown = false;

    if (__jo_core_null_access[0] == 0 || shown)
        return;
    shown = true;
    jo_core_error("NULL pointer: access to 0-0xFFF before PC %x, PR %x (%d times)",
                  __jo_core_null_access[1], __jo_core_null_access[2], __jo_core_null_access[0]);
}

#endif

#ifdef JO_COMPILE_WITH_RAM_CARD_SUPPORT

#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
extern jo_list                              __slave_callbacks;
void                                        jo_core_slave_init(void);
#endif // JO_COMPILE_WITH_DUAL_CPU_SUPPORT

static  __jo_force_inline void              jo_enable_extended_ram_cartridge_support(void)
{
    JO_RAM_CARTRIDGE_ENABLE = 1;
}

static  __jo_force_inline void              jo_set_a_bus_register(void)
{
    JO_SCU_ASR0 = JO_RAM_CARTRIDGE_ASR0;
    JO_SCU_AREF = JO_RAM_CARTRIDGE_AREF;
}

#endif

/** @brief Call the callback of a node (a function: jo_list_foreach() is not inlined, it calls it by its address) */
static void __jo_call_event(jo_node *node)
{
    ((jo_event_callback)node->data.ptr)();
}

#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
static void jo_slave_callbacks(void)
{
    jo_node     *node;

    // (not jo_list_foreach(): the slave CPU would write its cursor in the list, the cache of the master CPU could keep it
    // and jo_list_remove() follow it to the stack of the slave CPU. No node is freed while the slave CPU runs them, see
    // jo_core_remove_slave_callback())
    for (node = __slave_callbacks.first; node != JO_NULL; node = node->next)
        __jo_call_event(node);
}
#endif

static void __jo_vblank_callbacks(void)
{
    jo_list_foreach(&__vblank_callbacks, __jo_call_event);
}


/** @brief Minimum size of a free area of the high work RAM given to jo_malloc() */
#define JO_MIN_FREE_HIGH_WORK_RAM_ZONE  (1024)
/** @brief Alignment of the free areas of the high work RAM given to jo_malloc() (cache line) */
#define JO_FREE_HIGH_WORK_RAM_ALIGN     (16)

/** @brief Give a free area of the high work RAM to jo_malloc()
 *  @param begin Start of the area
 *  @param end End of the area (excluded)
 */
static void                 jo_add_free_high_work_ram_zone(unsigned int begin, const unsigned int end)
{
    begin = (begin + JO_FREE_HIGH_WORK_RAM_ALIGN - 1) & ~(JO_FREE_HIGH_WORK_RAM_ALIGN - 1);
    if (end > begin && end - begin >= JO_MIN_FREE_HIGH_WORK_RAM_ZONE)
        jo_add_memory_zone((unsigned char *)begin, end - begin);
}

static bool                 jo_init_memory(void)
{
    static unsigned char    global_memory[JO_GLOBAL_MEMORY_SIZE_FOR_MALLOC];

    jo_add_memory_zone(global_memory, sizeof(global_memory));
    // The high work RAM not used by the program (after .bss) and by the engine (after the engine work area, before the stack):
    // jo_malloc() uses the high work RAM before the slower memories (low work RAM, RAM cartridge)
    jo_add_free_high_work_ram_zone((unsigned int)&_bend, JO_WORK_RAM_USER_PROGRAM_AND_DATA_END);
#ifdef JO_STACK_GUARD_SIZE
    // (tests: a painted area under the stack of the master CPU, not given to jo_malloc(), checked by the game)
    jo_add_free_high_work_ram_zone((unsigned int)&_jo_work_end, JO_WORK_RAM_STACK_BOTTOM - JO_STACK_GUARD_SIZE);
    {
        volatile unsigned int   *w;

        for (w = (volatile unsigned int *)(JO_WORK_RAM_STACK_BOTTOM - JO_STACK_GUARD_SIZE); (unsigned int)w < JO_WORK_RAM_STACK_BOTTOM; ++w)
            *w = 0xDEADBEEF;
    }
#else
    jo_add_free_high_work_ram_zone((unsigned int)&_jo_work_end, JO_WORK_RAM_STACK_BOTTOM);
#endif
#ifdef JO_COMPILE_WITH_RAM_CARD_SUPPORT
    if (jo_get_extended_ram_cartridge_type() == CART_8MBits)
    {
        jo_enable_extended_ram_cartridge_support();
        jo_set_a_bus_register();
        jo_add_memory_zone((unsigned char *)JO_RAM_CARTRIDGE_ADDRESS, JO_RAM_CARTRIDGE_8MBITS_BANK_SIZE);
        jo_add_memory_zone((unsigned char *)JO_RAM_CARTRIDGE_8MBITS_BANK2_ADDRESS, JO_RAM_CARTRIDGE_8MBITS_BANK_SIZE);
    }
    else if (jo_get_extended_ram_cartridge_type() == CART_32MBits)
    {
        jo_enable_extended_ram_cartridge_support();
        jo_set_a_bus_register();
        jo_add_memory_zone((unsigned char *)JO_RAM_CARTRIDGE_ADDRESS, JO_RAM_CARTRIDGE_32MBITS_SIZE);
    }
#endif
#ifdef JO_DEBUG
    // We check if the Work RAM for malloc is on the user RAM area
    // It's not perfect but better than nothing
    if (!JO_IS_ARRAY_INSIDE_USER_RAM_AREA(global_memory))
        return (false);
#endif
    return (true);
}

/*
** INTERRUPTS
*/

/** @brief Get the vector base register
 *  @return Address of the interrupt vector table
 */
static  __jo_force_inline unsigned int      jo_core_get_vbr(void)
{
    unsigned int                            vbr;

    __asm__ volatile ("stc vbr, %0" : "=r" (vbr));
    return (vbr);
}

void                                        __jo_core_set_interrupt_handler(const unsigned int vector, jo_interrupt_handler handler)
{
    *(volatile unsigned int *)(jo_core_get_vbr() + JO_MULT_BY_4(vector)) = (unsigned int)handler;
}

/*
** The SCU interrupt mask (1: masked) is the copy of the BIOS (JO_BIOS_SCU_IMS): the code that enables an interrupt through
** the BIOS (SYS_CHGSCUIM) changes it too. A mask of the engine of its own masked it again at the next VBLANK: the backup
** library of an optical drive emulator (its code in the cartridge) then waited forever for its interrupt (the game froze at
** the start, at the reading of its save, or when it saved)
*/

void                                        __jo_core_enable_scu_interrupts(const unsigned int mask)
{
    unsigned int                            sr;
    unsigned int                            ims;

    sr = __jo_core_disable_interrupts();
    ims = (JO_BIOS_SCU_IMS & ~mask) & JO_SCU_INT_ALL;
    // The BIOS copy first: an interrupt handled by the BIOS writes it to the SCU when it returns
    JO_BIOS_SCU_IMS = ims;
    JO_SCU_IMS = ims;
    __jo_core_restore_interrupts(sr);
}

void                                        __jo_core_restore_scu_mask(void)
{
    JO_SCU_IMS = JO_BIOS_SCU_IMS;
}

/** @brief VBLANK-IN interrupt handler
 *  @remarks Applies the VDP2 registers, displays the last VDP1 command table, calls the VBLANK callbacks and requests the peripheral data
 *  @remarks The division unit is saved and restored: the interrupted code can be waiting for a quotient (jo_fixed_div_start())
 *           and the VBLANK callbacks can use jo_fixed_div()
 */
static void __attribute__((interrupt_handler)) jo_core_vblank_in_handler(void)
{
    int                                     divisor;
    int                                     high;
    int                                     quotient;

#if defined(JO_CORE_EXCEPTION_SCREEN) && defined(JO_COMPILE_WITH_DUAL_CPU_SUPPORT)
    // A CPU exception of the slave CPU stopped it: its error screen (the master CPU can be waiting for one of its jobs)
    if (*(volatile unsigned int *)JO_CACHE_THROUGH(&__jo_core_slave_exception.vector) != 0)
        __jo_core_show_exception((const volatile __jo_core_exception_registers *)JO_CACHE_THROUGH(&__jo_core_slave_exception));
#endif
#if defined(JO_CORE_EXCEPTION_SCREEN) && defined(JO_CORE_WATCHDOG)
    if (__jo_core_vblank_count - __jo_core_last_synch_vblank > JO_CORE_WATCHDOG_SECONDS * 60)
        jo_core_watchdog_screen(JO_NULL, JO_NULL);
#endif
    // Reading the quotient waits for the end of a division in progress
    quotient = JO_SH2_DVDNTL;
    divisor = JO_SH2_DVSR;
    high = JO_SH2_DVDNTH;
    ++__jo_core_vblank_count;
    // (first: the light guns read TVSTAT, other reads would clear the flag of the latch)
    __jo_smpc_gun_vblank();
    // (then the VDP1: the VBLANK erase of the manual change mode is requested in the first line of the VBLANK)
    __jo_vdp1_vblank();
    __jo_time_vblank();
    __jo_vdp2_vblank();
    if (__jo_core_music_vblank != JO_NULL)
        __jo_core_music_vblank();
    if (__vblank_callbacks.count)
        __jo_vblank_callbacks();
    // Requests the SMPC status and the peripheral data (like the Sega libraries, during the VBLANK)
    __jo_smpc_vblank_in();
    // The quotient can only be written by a division: quotient / 1
    JO_SH2_DVSR = 1;
    JO_SH2_DVDNTH = quotient < 0 ? -1 : 0;
    JO_SH2_DVDNTL = quotient;
    // Wait for the end of the division
    quotient = JO_SH2_DVDNTL;
    // Remainder (or high part of a dividend not yet complete) and divisor
    JO_SH2_DVDNTH = high;
    JO_SH2_DVSR = divisor;
    // The SCU masked its interrupts when this one was taken
    __jo_core_restore_scu_mask();
}

/** @brief VBLANK-OUT interrupt handler
 *  @remarks Ends the VBLANK erase of the VDP1 (see __jo_vdp1_vblank_out())
 */
static void __attribute__((interrupt_handler)) jo_core_vblank_out_handler(void)
{
    __jo_vdp1_vblank_out();
    // The SCU masked its interrupts when this one was taken
    __jo_core_restore_scu_mask();
}

/*
** SYSTEM
*/

/** @brief Initialize the hardware (VDP1, VDP2, SMPC) and the interrupts
 *  @remarks The display stays off (see jo_core_tv_on())
 */
static void                                 jo_core_init_system(void)
{
    __jo_core_set_sr(JO_SH2_SR_INTERRUPT_MASK);
    JO_BIOS_SCU_IMS = JO_SCU_INT_ALL;
    JO_SCU_IMS = JO_SCU_INT_ALL;
    __jo_vdp2_init_registers();
    __jo_vdp1_init();
    __jo_smpc_init();
    // (the "disc changed" flag of the boot: only a tray opened from now on, see jo_core_check_system_keys())
    JO_CORE_CD_HIRQ = (unsigned short)~JO_CORE_CD_HIRQ_DCHG;
    __jo_core_set_interrupt_handler(JO_VECTOR_VBLANK_IN, jo_core_vblank_in_handler);
    __jo_core_set_interrupt_handler(JO_VECTOR_VBLANK_OUT, jo_core_vblank_out_handler);
    __jo_core_enable_scu_interrupts(JO_SCU_INT_VBLANK_IN | JO_SCU_INT_VBLANK_OUT | JO_SCU_INT_SMPC);
    JO_ZERO(__jo_core_vblank_count);
    JO_ZERO(__jo_core_last_synch_vblank);
    __jo_core_set_sr(JO_SH2_SR_INTERRUPTS_ON);
}

void                                        __jo_core_synch(void)
{
    unsigned int                            target;
#if defined(JO_CORE_EXCEPTION_SCREEN) && defined(JO_CORE_WATCHDOG)
    unsigned int                            previous[4];
    const void                              *corrupt;

    // The memory checked at each frame: the frame where a buffer was written past its end
    if ((corrupt = __jo_memory_check(previous)) != JO_NULL)
        jo_core_watchdog_screen(corrupt, previous);
#endif

    // (the VDP2 scroll and rotation parameters of the frame: with the number of the frame, before it changes)
    __jo_vdp2_end_frame();
    __jo_vdp1_end_frame();
#ifdef JO_COMPILE_WITH_3D_SUPPORT
    jo_profile_count(JO_PROFILE_POLYGONS, jo_3d_get_displayed_polygon_count());
    __jo_3d_end_frame();
#endif
    // Real frame rate = (60 / JO_FRAMERATE)
    jo_profile_begin(JO_PROFILE_VSYNC);
    target = __jo_core_last_synch_vblank + (JO_FRAMERATE < 1 ? 1 : JO_FRAMERATE);
    while ((int)(__jo_core_vblank_count - target) < 0 || __jo_core_vblank_count == __jo_core_last_synch_vblank)
    {
#ifdef JO_CORE_SYNCH_SLEEP
        // (JO_CORE_SYNCH_SLEEP, not by default) The CPU sleeps until the next interrupt (the VBLANK, the SMPC just after
        // it): the same wait, without the loop (an emulator does not run the instructions of the loop: Mednafen emulates
        // the game 7 % faster). Not by default: the emulators get SLEEP wrong (see JO_SLAVE_SLEEP in slave.c)
        __asm__ volatile ("sleep");
#else
        __jo_core_pause();
#endif
    }
    jo_profile_end(JO_PROFILE_VSYNC);
    jo_profile_count(JO_PROFILE_VBLANKS, 100 * (__jo_core_vblank_count - __jo_core_last_synch_vblank));
    // The frames really shown (see jo_core_get_fps()): a frame is late when it took more VBLANK than JO_FRAMERATE (not
    // the first one: the time of the loading before it)
    if (__jo_core_fps_vblank == 0)
        __jo_core_fps_vblank = __jo_core_vblank_count;
    else
    {
        if (__jo_core_vblank_count - __jo_core_last_synch_vblank > (JO_FRAMERATE < 1 ? 1 : JO_FRAMERATE))
            ++__jo_core_late_frames;
        ++__jo_core_fps_frames;
        // (every 2 seconds: 120 VBLANK in NTSC, 100 in PAL. Rounded)
        if (__jo_core_vblank_count - __jo_core_fps_vblank >= 2 * (unsigned int)jo_core_get_refresh_rate())
        {
            __jo_core_fps = (int)((__jo_core_fps_frames * (unsigned int)jo_core_get_refresh_rate() * 2 + (__jo_core_vblank_count - __jo_core_fps_vblank)) /
                                  (2 * (__jo_core_vblank_count - __jo_core_fps_vblank)));
            JO_ZERO(__jo_core_fps_frames);
            __jo_core_fps_vblank = __jo_core_vblank_count;
        }
    }
    __jo_core_last_synch_vblank = __jo_core_vblank_count;
    __jo_vdp1_frame_start();
    // The inputs of the new frame
    __jo_input_frame_start();
}

void                                        jo_core_tv_off(void)
{
    __jo_vdp2_set_tv_display(false);
}

void                                        jo_core_tv_on(void)
{
    __jo_vdp2_set_tv_display(true);
}

static void         jo_core_init_vdp(const jo_color back_color)
{
    jo_core_init_system();
    jo_core_tv_off();
    __jo_init_vdp2(back_color);
}

bool                                        jo_core_set_screen_resolution(const int width, const int asked_height)
{
    bool                                    pal;
    int                                     height;

    pal = (JO_VDP2_TVSTAT & JO_VDP2_TVSTAT_PAL) != 0;
    // The 256 and 512 lines of PAL on a 60 Hz console (a PAL program on a NTSC console or emulator): 240 and 480 lines, the
    // closest ones (the mode stayed unchanged: the menus in 640 x 448 kept for the 3D of a game)
    height = asked_height;
    if (!pal && (height == 256 || height == 512))
        height = height == 256 ? 240 : 480;
    if ((width != 320 && width != 352 && width != 640 && width != 704) ||
        (height != 224 && height != 240 && height != 448 && height != 480 && (!pal || (height != 256 && height != 512))))
        return (false);
    if ((width == 352 || width == 704) != __jo_screen.clock_352)
        return (false);
    __jo_screen.width = (short)width;
    __jo_screen.height = (short)height;
    __jo_screen.width_2 = (short)(width >> 1);
    __jo_screen.height_2 = (short)(height >> 1);
    __jo_screen.hires = width > 352;
    __jo_screen.interlace = height > 256;
    __jo_screen.background_width = (short)(__jo_screen.hires ? width >> 1 : width);
    __jo_screen.background_height = (short)(__jo_screen.interlace ? height >> 1 : height);
    __jo_vdp1_set_screen_mode();
    __jo_vdp2_set_screen_mode();
#ifdef JO_COMPILE_WITH_3D_SUPPORT
    __jo_3d_set_screen_mode();
#endif
    return (true);
}

void                    jo_core_set_screen_single_interlace(const bool enabled)
{
    __jo_screen.single_interlace = enabled;
    __jo_vdp2_update_tv_mode();
}

void                    __jo_core_set_screens_order(jo_scroll_screen screen1, ...)
{
    va_list             va;
    int                 priority;
    jo_scroll_screen    screen;

    va_start(va, screen1);
    priority = 7;
    screen = screen1;
    do
    {
        __jo_vdp2_set_priority(screen, priority);
        screen = va_arg(va, jo_scroll_screen);
        --priority;
    } while (screen != ((jo_scroll_screen)~0));
    va_end(va);
}

void			    jo_core_init(const jo_color back_color)
{
#ifdef JO_DEBUG
    bool            is_memory_ok;
#endif
    jo_datetime     dt;

#ifdef JO_DEBUG
    JO_ZERO(__jo_last_error[0]);
    is_memory_ok =
#endif
    jo_init_memory();
#if JO_TV_WIDTH == 352 || JO_TV_WIDTH == 704
    // 352 and 704 pixels width modes need the 28.6 MHz clock. The change resets the VDPs, the SCU and the slave CPU,
    // and the SMPC sends an NMI to the master CPU: the routine of the Boot ROM does it (SYS_CHGSYSCK), before the
    // slave CPU is started and before the registers of the VDPs are written
    JO_BIOS_CHANGE_SYSTEM_CLOCK(JO_BIOS_SYSTEM_CLOCK_352);
#endif
    jo_list_init(&__vblank_callbacks);
    jo_list_init(&__callbacks);
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
    jo_core_slave_init();
#endif
    jo_core_init_vdp(back_color);
#ifdef JO_CORE_EXCEPTION_SCREEN
    // (after the VDP2: the error screen of a CPU exception prints)
    jo_core_init_exceptions();
#endif
    jo_core_add_vblank_callback(jo_get_inputs_vblank);
#if defined(JO_COMPILE_WITH_AUDIO_SUPPORT)
    jo_audio_init();
    jo_audio_set_volume(JO_DEFAULT_AUDIO_VOLUME);
#endif
#ifdef JO_COMPILE_WITH_3D_SUPPORT
    jo_3d_init();
#endif
#if JO_TV_HEIGHT == 256 || JO_TV_HEIGHT == 512
    // (a PAL program on a 60 Hz console: 240 or 480 lines, see jo_core_set_screen_resolution())
    if ((JO_VDP2_TVSTAT & JO_VDP2_TVSTAT_PAL) == 0)
        jo_core_set_screen_resolution(JO_TV_WIDTH, JO_TV_HEIGHT);
#endif
#ifdef JO_COMPILE_WITH_STORYBOARD_SUPPORT
    jo_init_storyboards();
#endif
    jo_core_tv_on();
#ifdef JO_DEBUG
    if (!is_memory_ok)
        jo_core_error("Please reduce JO_GLOBAL_MEMORY_SIZE_FOR_MALLOC");
#endif
#ifdef JO_COMPILE_WITH_FS_SUPPORT
    if (jo_fs_init() == 0)
    {
#ifdef JO_DEBUG
        jo_core_error("Fail to initialize filesystem");
#endif
    }
#endif
#if defined(JO_COMPILE_WITH_VIDEO_SUPPORT)
    if (!jo_video_init())
    {
#ifdef JO_DEBUG
        jo_core_error("Fail to initialize video");
#endif
    }
#endif
    jo_sprite_init();
    jo_time_init(JO_TIME_CKS_32_MODE);
#if defined(JO_CORE_EXCEPTION_SCREEN) && defined(JO_CORE_WATCHDOG)
    __jo_core_watchdog_install(false);
#endif
#ifdef JO_COMPILE_WITH_PROFILER_SUPPORT
    __jo_profile_init();
#endif
    // random seed initialization
    jo_getdate(&dt);
    jo_random_seed = JO_MULT_BY_4096(dt.second) +
                    JO_MULT_BY_1024(dt.minute) +
                    JO_MULT_BY_65536(dt.hour) +
                    JO_MULT_BY_2048(dt.week) -
                    JO_MULT_BY_32(dt.year) -
                    JO_MULT_BY_256(dt.month) -
                    JO_MULT_BY_16(dt.day);
#ifdef JO_DEBUG
    // (after the initialization: the code of the Boot ROM reads its own data in the first 4 KB)
    jo_core_init_null_trap();
#endif
}

inline int			jo_core_add_vblank_callback(const jo_event_callback callback)
{
    jo_node         *node;

#ifdef JO_DEBUG
    if (callback == JO_NULL)
    {
        jo_core_error("callback is null");
        return (0);
    }
#endif
    node = jo_list_add_ptr(&__vblank_callbacks, callback);
    if (node == JO_NULL)
        return (0);
    return (int)node;
}

inline void    jo_core_remove_vblank_callback(const int vblank_event_id)
{
#ifdef JO_DEBUG
    if (vblank_event_id == 0)
    {
        jo_core_error("Invalid vblank_event_id (%d)", vblank_event_id);
        return;
    }
#endif
    jo_list_remove(&__vblank_callbacks, (jo_node *)vblank_event_id);
}

inline int			jo_core_add_callback(const jo_event_callback callback)
{
    jo_node         *node;

#ifdef JO_DEBUG
    if (callback == JO_NULL)
    {
        jo_core_error("callback is null");
        return (0);
    }
#endif
    node = jo_list_add_ptr(&__callbacks, callback);
    if (node == JO_NULL)
        return (0);
    return (int)node;
}

inline void    jo_core_remove_callback(const int event_id)
{
#ifdef JO_DEBUG
    if (event_id == 0)
    {
        jo_core_error("Invalid event_id (%d)", event_id);
        return;
    }
#endif
    jo_list_remove(&__callbacks, (jo_node *)event_id);
}

void            jo_goto_boot_menu(void)
{
    // Back to the boot ROM (multiplayer screen). After the INTBACK of the frame (see __jo_smpc_command(): a command sent
    // during an INTBACK is lost), then the interrupts masked until the reset
    __jo_smpc_command(SystemReset);
    __jo_core_set_sr(JO_SH2_SR_INTERRUPT_MASK);
    for (;;)
        ;
}

#ifdef JO_DEBUG

/** @brief The first error of the slave CPU or of an interrupt (or of code with the interrupts masked), shown by the master
 *  CPU at the next frame (see jo_core_check_held_error()): the error screen waits for the next frames and for the pad, it
 *  never ended there (the VBLANK interrupt can't come, the slave CPU doesn't draw the screen) */
static char                                 __jo_core_held_error[JO_PRINTF_BUF_SIZE];
static const char                           *__jo_core_held_error_function;
/** @brief Errors held since the last one shown (written by both CPUs: read and written through the cache-through address) */
static int                                  __jo_core_held_error_count = 0;

/** @brief Check if an error can't be shown now (see __jo_core_held_error) */
static  __jo_force_inline bool              jo_core_error_must_wait(void)
{
    return (__jo_core_on_slave_cpu() || (__jo_core_get_sr() & JO_SH2_SR_INTERRUPT_MASK) != 0);
}

/** @brief Show the error held (see jo_core_check_held_error()) */
static void __attribute__((cold, noinline)) jo_core_check_held_error_now(void)
{
    volatile int                            *count;
    int                                     n;

    count = (volatile int *)JO_CACHE_THROUGH(&__jo_core_held_error_count);
    n = *count;
    // (the message written by the other CPU: read through the cache-through address)
    jo_memcpy(__jo_last_error, (const void *)JO_CACHE_THROUGH(__jo_core_held_error), sizeof(__jo_last_error));
    *count = 0;
    if (n > 1)
        jo_snprintf(__jo_last_error + jo_strlen(__jo_last_error), sizeof(__jo_last_error) - jo_strlen(__jo_last_error),
                    " (+%d)", n - 1);
    __jo_core_error(__jo_last_error, *(const char * volatile *)JO_CACHE_THROUGH(&__jo_core_held_error_function));
}

/** @brief The error of the slave CPU or of an interrupt, at the start of a frame of the master CPU */
static  __jo_force_inline void              jo_core_check_held_error(void)
{
    if (*(volatile int *)JO_CACHE_THROUGH(&__jo_core_held_error_count) != 0)
        jo_core_check_held_error_now();
}

void            __jo_core_errorf(const char *function, const char *format, ...)
{
    va_list     args;
    volatile int *count;

    va_start(args, format);
    if (jo_core_error_must_wait())
    {
        // The first one kept (the others counted), shown by the master CPU at its next frame
        count = (volatile int *)JO_CACHE_THROUGH(&__jo_core_held_error_count);
        if (*count == 0)
        {
            jo_vsnprintf((char *)JO_CACHE_THROUGH(__jo_core_held_error), sizeof(__jo_core_held_error), format, args);
            *(const char * volatile *)JO_CACHE_THROUGH(&__jo_core_held_error_function) = function;
        }
        ++*count;
        va_end(args);
        return;
    }
    jo_vsnprintf(__jo_last_error, sizeof(__jo_last_error), format, args);
    va_end(args);
    __jo_core_error(__jo_last_error, function);
}

static jo_core_error_handler                 __jo_core_error_handler = JO_NULL;

void            jo_core_set_error_handler(const jo_core_error_handler handler)
{
    __jo_core_error_handler = handler;
}

#ifdef JO_COMPILE_WITH_PRINTF_SUPPORT

/** @brief The background of the error screens: the NBG1 bitmap filled with a color
 *  @param color Color
 *  @remarks The NBG1 bitmap can't be allocated (see __jo_vdp2_reserve_nbg1_bitmap()): the back screen color, the text stays
 *           on NBG0
 */
static void                                 jo_core_error_background(const jo_color color)
{
    if (__jo_vdp2_reserve_nbg1_bitmap())
        jo_vdp2_clear_bitmap_nbg1(color);
    else
        jo_set_default_background_color(color);
}

/** @brief The memory used, on 2 lines of the error screens
 *  @param y Line of the first one
 */
static void                                 jo_core_error_show_usage(const int y)
{
    int                                     usage;

    usage = jo_memory_usage_percent();
    if (usage > 75)
        jo_printf_with_color(0, y, JO_COLOR_INDEX_Red, "Dynamic memory usage: %d%%  ", usage);
    else if (usage > 33)
        jo_printf_with_color(0, y, JO_COLOR_INDEX_Yellow, "Dynamic memory usage: %d%%  ", usage);
    else
        jo_printf_with_color(0, y, JO_COLOR_INDEX_Green, "Dynamic memory usage: %d%%  ", usage);
    usage = jo_sprite_usage_percent();
    if (usage > 75)
        jo_printf_with_color(0, y + 1, JO_COLOR_INDEX_Red, "Sprite memory usage:  %d%%  ", usage);
    else if (usage > 33)
        jo_printf_with_color(0, y + 1, JO_COLOR_INDEX_Yellow, "Sprite memory usage:  %d%%  ", usage);
    else
        jo_printf_with_color(0, y + 1, JO_COLOR_INDEX_Green, "Sprite memory usage:  %d%%  ", usage);
}

#endif // JO_COMPILE_WITH_PRINTF_SUPPORT

void            __jo_core_error(char *message, const char *function)
{
    if (__jo_core_error_handler != JO_NULL)
    {
        __jo_core_error_handler(message, function);
        return;
    }
#ifdef JO_COMPILE_WITH_PRINTF_SUPPORT

    jo_core_error_background(JO_COLOR_Blue);
    jo_set_printf_color_index(0);
    jo_printf(0, 13, "   >>> Jo Engine error handler <<<");
    jo_core_error_show_usage(17);
    jo_printf_with_color(0, 21, JO_COLOR_INDEX_Red, "In %s():", function);
    jo_printf_with_color(0, 23, JO_COLOR_INDEX_Red, "%s", message);
    jo_set_printf_color_index(0);
    jo_printf(2, 27, "Press [START] to continue...");
    jo_core_suspend();
    jo_clear_screen();
#else
    JO_UNUSED_ARG(message);
    JO_UNUSED_ARG(function);
    if (__jo_vdp2_reserve_nbg1_bitmap())
        jo_vdp2_clear_bitmap_nbg1(JO_COLOR_Red);
    else
        jo_set_default_background_color(JO_COLOR_Red);
    jo_core_suspend();
#endif // JO_COMPILE_WITH_PRINTF_SUPPORT
}

#ifdef JO_CORE_EXCEPTION_SCREEN

/*
** CPU EXCEPTIONS (JO_DEBUG, with the printf module): an illegal instruction, an illegal instruction after a delayed branch (slot), an address
** error of the CPU (a 16 or 32 bits access to an odd or unaligned address, a forbidden area) or of its DMA controller.
** The handler of each CPU saves its registers (__jo_core_exception_registers), then:
**   - master CPU: the error screen, the interrupts masked, on the top of its stack (the program can't go on: the frames
**     of the stack are left, an exception in an interrupt left the SCU interrupts masked);
**   - slave CPU: it stops (the interrupts masked), the master CPU shows its registers at the next VBLANK (it can be
**     waiting for a job of the slave CPU that never ends).
** The error screen doesn't wait for the VBLANK interrupt nor for the slave CPU: it polls the VBLANK to apply the
** registers of the VDP2, and the sprites are hidden (the VDP1 doesn't draw a frame anymore: its last one is left).
** Not given to the handler of jo_core_set_error_handler(): the program stops there, the console must be reset.
*/

/** @brief Vectors of the CPU exceptions */
# define JO_VECTOR_ILLEGAL_INSTRUCTION      (4)
# define JO_VECTOR_ILLEGAL_SLOT             (6)
# define JO_VECTOR_CPU_ADDRESS_ERROR        (9)
# define JO_VECTOR_DMA_ADDRESS_ERROR        (10)
/** @brief TVSTAT: in the VBLANK */
# define JO_CORE_TVSTAT_VBLANK              (1 << 3)

/** @brief The registers of the CPUs at their CPU exception (see __jo_core_exception_registers) */
__jo_core_exception_registers               __jo_core_master_exception __attribute__((used, externally_visible, aligned(16)));
__jo_core_exception_registers               __jo_core_slave_exception __attribute__((used, externally_visible, aligned(16)));

/** @brief Text of a number in the assembly */
# define JO_CORE_ASM_STR2(x)                #x
# define JO_CORE_ASM_STR(x)                 JO_CORE_ASM_STR2(x)

/** @brief The handlers of the CPU exceptions, one per vector and per CPU (the vector in R0, then the part of the CPU).
 *  The CPU pushed SR, then PC (it doesn't change the interrupt mask). The registers are saved from the end of the
 *  structure (pre-decrement, the vector last), at its cache-through address (offsets: see __jo_core_exception_registers,
 *  96 bytes)
 */
__asm__(
"    .text\n"
"    .align 2\n"
"    .global ___jo_core_master_exception_4\n"
"___jo_core_master_exception_4:\n"
"    mov.l r0, @-r15\n"
"    bra ___jo_core_master_exception_save\n"
"    mov #4, r0\n"
"    .global ___jo_core_master_exception_6\n"
"___jo_core_master_exception_6:\n"
"    mov.l r0, @-r15\n"
"    bra ___jo_core_master_exception_save\n"
"    mov #6, r0\n"
"    .global ___jo_core_master_exception_9\n"
"___jo_core_master_exception_9:\n"
"    mov.l r0, @-r15\n"
"    bra ___jo_core_master_exception_save\n"
"    mov #9, r0\n"
"    .global ___jo_core_master_exception_10\n"
"___jo_core_master_exception_10:\n"
"    mov.l r0, @-r15\n"
"    bra ___jo_core_master_exception_save\n"
"    mov #10, r0\n"
"___jo_core_master_exception_save:\n"
"    mov.l r1, @-r15\n"
"    mov.l r0, @-r15\n"                /* the stack: vector, R1, R0, PC, SR */
"    mov.l 1f, r1\n"
"    sts.l macl, @-r1\n"
"    sts.l mach, @-r1\n"
"    stc.l vbr, @-r1\n"
"    stc.l gbr, @-r1\n"
"    sts.l pr, @-r1\n"                 /* (before the bsr) */
"    bsr ___jo_core_exception_save\n"
"    nop\n"
"    mov.l 2f, r0\n"                   /* __jo_core_show_exception(registers) */
"    jmp @r0\n"
"    mov r1, r4\n"
"    .align 2\n"
"1:  .long ___jo_core_master_exception + 0x20000060\n"
"2:  .long ___jo_core_show_exception\n"
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
"\n"
"    .global ___jo_core_slave_exception_4\n"
"___jo_core_slave_exception_4:\n"
"    mov.l r0, @-r15\n"
"    bra ___jo_core_slave_exception_save\n"
"    mov #4, r0\n"
"    .global ___jo_core_slave_exception_6\n"
"___jo_core_slave_exception_6:\n"
"    mov.l r0, @-r15\n"
"    bra ___jo_core_slave_exception_save\n"
"    mov #6, r0\n"
"    .global ___jo_core_slave_exception_9\n"
"___jo_core_slave_exception_9:\n"
"    mov.l r0, @-r15\n"
"    bra ___jo_core_slave_exception_save\n"
"    mov #9, r0\n"
"    .global ___jo_core_slave_exception_10\n"
"___jo_core_slave_exception_10:\n"
"    mov.l r0, @-r15\n"
"    bra ___jo_core_slave_exception_save\n"
"    mov #10, r0\n"
"___jo_core_slave_exception_save:\n"
"    mov.l r1, @-r15\n"
"    mov.l r0, @-r15\n"
"    mov.l 3f, r1\n"
"    sts.l macl, @-r1\n"
"    sts.l mach, @-r1\n"
"    stc.l vbr, @-r1\n"
"    stc.l gbr, @-r1\n"
"    sts.l pr, @-r1\n"
"    bsr ___jo_core_exception_save\n"
"    nop\n"
"    mov.l 4f, r0\n"                   /* the interrupts masked, the slave CPU stops there */
"    ldc r0, sr\n"
"5:  bra 5b\n"
"    nop\n"
"    .align 2\n"
"3:  .long ___jo_core_slave_exception + 0x20000060\n"
"4:  .long 0x000000F0\n"
#endif
"\n"
/* The rest of the registers. R1: the address of the PR in the structure, the stack: vector, R1, R0, PC, SR. Returns with
   R1 at the start of the structure */
"___jo_core_exception_save:\n"
"    mov.l @(16, r15), r0\n"           /* SR */
"    mov.l r0, @-r1\n"
"    mov.l @(12, r15), r0\n"           /* PC */
"    mov.l r0, @-r1\n"
"    mov r15, r0\n"                    /* R15 before the exception */
"    add #20, r0\n"
"    mov.l r0, @-r1\n"
"    mov.l r14, @-r1\n"
"    mov.l r13, @-r1\n"
"    mov.l r12, @-r1\n"
"    mov.l r11, @-r1\n"
"    mov.l r10, @-r1\n"
"    mov.l r9, @-r1\n"
"    mov.l r8, @-r1\n"
"    mov.l r7, @-r1\n"
"    mov.l r6, @-r1\n"
"    mov.l r5, @-r1\n"
"    mov.l r4, @-r1\n"
"    mov.l r3, @-r1\n"
"    mov.l r2, @-r1\n"
"    mov.l @(4, r15), r0\n"            /* R1 */
"    mov.l r0, @-r1\n"
"    mov.l @(8, r15), r0\n"            /* R0 */
"    mov.l r0, @-r1\n"
"    mov.l @r15, r0\n"                 /* the vector, last */
"    rts\n"
"    mov.l r0, @-r1\n"
"\n"
/* __jo_core_show_exception(registers): the interrupts masked, the stack pointer on the top of the stack of the master
   CPU (the frames of the stack are left), then __jo_core_exception_screen(registers) (R4 kept) */
"    .global ___jo_core_show_exception\n"
"___jo_core_show_exception:\n"
"    mov.l 7f, r0\n"
"    ldc r0, sr\n"
"    mov.l 8f, r15\n"
"    mov.l 9f, r0\n"
"    jmp @r0\n"
"    nop\n"
"    .align 2\n"
"7:  .long 0x000000F0\n"
"8:  .long " JO_CORE_ASM_STR(JO_WORK_RAM_STACK_TOP) "\n"
"9:  .long ___jo_core_exception_screen\n"
);

/** @brief The handlers of the CPU exceptions (assembly above) */
void                                        __jo_core_master_exception_4(void);
void                                        __jo_core_master_exception_6(void);
void                                        __jo_core_master_exception_9(void);
void                                        __jo_core_master_exception_10(void);
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
void                                        __jo_core_slave_exception_4(void);
void                                        __jo_core_slave_exception_6(void);
void                                        __jo_core_slave_exception_9(void);
void                                        __jo_core_slave_exception_10(void);
#endif

/** @brief The handlers of the CPU exceptions of the master CPU, in its vector table */
static void                                 jo_core_init_exceptions(void)
{
    __jo_core_set_interrupt_handler(JO_VECTOR_ILLEGAL_INSTRUCTION, __jo_core_master_exception_4);
    __jo_core_set_interrupt_handler(JO_VECTOR_ILLEGAL_SLOT, __jo_core_master_exception_6);
    __jo_core_set_interrupt_handler(JO_VECTOR_CPU_ADDRESS_ERROR, __jo_core_master_exception_9);
    __jo_core_set_interrupt_handler(JO_VECTOR_DMA_ADDRESS_ERROR, __jo_core_master_exception_10);
}

#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
void                                        __jo_core_init_slave_exceptions(void)
{
    __jo_core_set_interrupt_handler(JO_VECTOR_ILLEGAL_INSTRUCTION, __jo_core_slave_exception_4);
    __jo_core_set_interrupt_handler(JO_VECTOR_ILLEGAL_SLOT, __jo_core_slave_exception_6);
    __jo_core_set_interrupt_handler(JO_VECTOR_CPU_ADDRESS_ERROR, __jo_core_slave_exception_9);
    __jo_core_set_interrupt_handler(JO_VECTOR_DMA_ADDRESS_ERROR, __jo_core_slave_exception_10);
}
#endif

/** @brief Check if the code around an address can be read (the Boot ROM, the work RAMs)
 *  @param addr Address
 *  @return true if it can
 */
static bool                                 jo_core_exception_readable(const unsigned int addr)
{
    unsigned int                            a;

    // (the cached or the cache-through address of the Boot ROM, of the low and of the high work RAM)
    if ((addr & 1) || (addr & 0xD0000000) != 0)
        return (false);
    a = addr & 0x1FFFFFFF;
    if (a < 4)
        return (false);
    a -= 4;
    return (a + 8 <= 0x00080000 || (a >= JO_WORK_RAM_LOW && a + 8 <= JO_WORK_RAM_LOW + 0x100000) ||
            (a >= JO_WORK_RAM_HIGH && a + 8 <= JO_WORK_RAM_HIGH + 0x100000));
}

/** @brief The error screen of a CPU exception (see __jo_core_show_exception())
 *  @param registers The registers (cache-through address)
 */
void __attribute__((used, externally_visible, noreturn, noinline, cold))
                                            __jo_core_exception_screen(const volatile __jo_core_exception_registers *registers)
{
    static int                              shown = 0;
    static const char * const               names[] = {"Illegal instruction", "", "Illegal instruction after a branch",
                                                       "", "", "CPU address error (an unaligned access?)", "DMA address error"};
    const volatile unsigned short           *code;
    unsigned int                            pc;
    int                                     i;

    if (shown++)
        for (;;)
            ;
    // The transfer of the DMA controller stopped by an address error never ends (the DMA of the VDP2 below waits for it)
    JO_SH2_CHCR0 = 0;
    jo_core_error_background(JO_COLOR_Blue);
    jo_clear_screen();
    jo_set_printf_color_index(JO_COLOR_INDEX_White);
    jo_printf(0, 1, "   >>> Jo Engine error handler <<<");
    jo_printf_with_color(0, 3, JO_COLOR_INDEX_Red, "CPU exception of the %s CPU:",
                         (unsigned int)registers == JO_CACHE_THROUGH(&__jo_core_slave_exception) ? "slave" : "master");
    jo_printf_with_color(0, 4, JO_COLOR_INDEX_Red, "%s", registers->vector >= 4 && registers->vector <= 10 ?
                         names[registers->vector - 4] : "?");
    jo_set_printf_color_index(JO_COLOR_INDEX_White);
    jo_printf(0, 6, "PC   %08X  PR   %08X", registers->pc, registers->pr);
    jo_printf(0, 7, "SR   %08X  GBR  %08X", registers->sr, registers->gbr);
    jo_printf(0, 8, "MACH %08X  MACL %08X", registers->mach, registers->macl);
    jo_printf(0, 9, "VBR  %08X", registers->vbr);
    for (i = 0; i < 16; i += 3)
    {
        if (i < 15)
            jo_printf(0, 11 + i / 3, "R%-2d %08X R%-2d %08X R%-2d %08X", i, registers->r[i], i + 1, registers->r[i + 1],
                      i + 2, registers->r[i + 2]);
        else
            jo_printf(0, 11 + i / 3, "R%-2d %08X", i, registers->r[i]);
    }
    pc = registers->pc;
    if (jo_core_exception_readable(pc))
    {
        code = (const volatile unsigned short *)JO_CACHE_THROUGH((pc & 0x1FFFFFFF) - 4);
        jo_printf(0, 18, "Code at PC-4: %04X %04X %04X %04X", code[0], code[1], code[2], code[3]);
    }
    jo_printf(0, 20, "Find PC and PR in game.map");
    jo_printf(0, 25, "Program stopped: reset the console");
    // (last: the memory blocks are read, they can be damaged)
    jo_core_error_show_usage(22);
    jo_set_printf_color_index(JO_COLOR_INDEX_White);
    // The sprites hidden (priority 0), the display on, applied at each VBLANK (polled: the interrupts are masked)
    __jo_vdp2_set_priority(JO_SPRITE_SCREEN, 0);
    __jo_vdp2_set_tv_display(true);
    for (;;)
    {
        while (JO_VDP2_TVSTAT & JO_CORE_TVSTAT_VBLANK)
            ;
        while (!(JO_VDP2_TVSTAT & JO_CORE_TVSTAT_VBLANK))
            ;
        __jo_vdp2_vblank();
    }
}

#ifdef JO_CORE_WATCHDOG

/*
** WATCHDOG (-DJO_CORE_WATCHDOG with JO_DEBUG, a diagnostic of a program that hangs on a real console): the output compare
** A interrupt of the free running timer of each CPU (level 15: also in the sections that mask the other interrupts)
** saves the PC, PR and SR of the interrupted code about 50 times per second. When no frame ended for
** JO_CORE_WATCHDOG_SECONDS, the VBLANK interrupt shows them: the loop where each CPU waits (see game.map)
*/

/** @brief The vector and the interval (ticks of the free running timer, clock / 32: about 20 ms) */
# define JO_CORE_WATCHDOG_VECTOR            (0x66)

/** @brief The samples kept per CPU (power of 2) */
# define JO_CORE_WATCHDOG_SAMPLES           (8)

/** @brief The last samples of a CPU (offsets used by the handlers: 80 bytes) */
typedef struct
{
    unsigned int                            count;
    struct
    {
        unsigned int                        pc;
        unsigned int                        pr;
    }                                       sample[JO_CORE_WATCHDOG_SAMPLES];
    unsigned int                            padding[3];
}                                           jo_core_watchdog_samples;

/** @brief The samples of the master CPU and of the slave CPU (written through the cache-through address) */
jo_core_watchdog_samples                    __jo_core_watchdog_samples[2] __attribute__((used, externally_visible, aligned(16)));

/** @brief The handler of a CPU: saves the PC and the PR of the interrupted code in the next sample, sets the next compare
 *  match. The stack: R2, R1, R0, PC, SR */
# define JO_CORE_WATCHDOG_HANDLER(NAME, INDEX) \
__asm__( \
"    .text\n" \
"    .align 2\n" \
"    .global _" #NAME "\n" \
"_" #NAME ":\n" \
"    mov.l r0, @-r15\n" \
"    mov.l r1, @-r15\n" \
"    mov.l r2, @-r15\n" \
"    mov.l 1f, r1\n" \
"    mov.l @r1, r0\n"                /* count */ \
"    add #1, r0\n" \
"    mov.l r0, @r1\n" \
"    and #7, r0\n" \
"    shll2 r0\n" \
"    shll r0\n" \
"    add #4, r0\n" \
"    add r1, r0\n"                   /* the sample */ \
"    mov.l @(12, r15), r2\n"         /* PC */ \
"    mov.l r2, @r0\n" \
"    sts pr, r2\n" \
"    mov.l r2, @(4, r0)\n" \
"    mov.l 4f, r1\n"                 /* FRT registers (TIER) */ \
"    mov.b @(2, r1), r0\n" \
"    extu.b r0, r2\n" \
"    shll8 r2\n" \
"    mov.b @(3, r1), r0\n" \
"    extu.b r0, r0\n" \
"    or r0, r2\n" \
"    mov.l 2f, r0\n" \
"    add r0, r2\n" \
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
"1:  .long ___jo_core_watchdog_samples + " #INDEX " * 80 + 0x20000000\n" \
"2:  .long 0x4000\n" \
"4:  .long 0xFFFFFE10\n" \
);

JO_CORE_WATCHDOG_HANDLER(__jo_core_watchdog_master_handler, 0)
JO_CORE_WATCHDOG_HANDLER(__jo_core_watchdog_slave_handler, 1)
extern void                                 __jo_core_watchdog_master_handler(void);
extern void                                 __jo_core_watchdog_slave_handler(void);

void                                        __jo_core_watchdog_install(const bool slave)
{
    unsigned int                            frc;

    JO_SH2_FRT_TIER = JO_SH2_FRT_TIER_NO_INTERRUPT;
    JO_SH2_VCRC = (JO_SH2_VCRC & 0xFF00) | JO_CORE_WATCHDOG_VECTOR;
    JO_SH2_IPRB = (JO_SH2_IPRB & 0xF0FF) | 0x0F00;
    __jo_core_set_interrupt_handler(JO_CORE_WATCHDOG_VECTOR, slave ? __jo_core_watchdog_slave_handler : __jo_core_watchdog_master_handler);
    JO_SH2_FRT_TOCR = JO_SH2_FRT_TOCR_OCRA;
    frc = jo_time_get_frc() + 48;
    JO_SH2_FRT_OCRAH = (unsigned char)(frc >> 8);
    JO_SH2_FRT_OCRAL = (unsigned char)frc;
    JO_SH2_FRT_FTCSR = JO_SH2_FRT_FTCSR & ~JO_SH2_FRT_FTCSR_OCFA;
    JO_SH2_FRT_TIER = JO_SH2_FRT_TIER_NO_INTERRUPT | JO_SH2_FRT_TIER_OCIAE;
    // (the slave CPU runs with its interrupts masked: level 14 lets the watchdog through)
    if (slave)
        __jo_core_set_sr(0x000000E0);
}

/** @brief Show the samples of a CPU, the oldest first
 *  @param samples The samples (a copy)
 *  @param y Line of the first one
 */
static void                                 jo_core_watchdog_show(const jo_core_watchdog_samples *samples, const int y)
{
    int                                     i;
    int                                     n;

    n = (int)JO_MIN(samples->count, (unsigned int)JO_CORE_WATCHDOG_SAMPLES);
    for (i = 0; i < n; ++i)
    {
        const unsigned int                  k = (samples->count - (unsigned int)n + (unsigned int)i + 1) & (JO_CORE_WATCHDOG_SAMPLES - 1);

        jo_printf(i & 1 ? 20 : 0, y + i / 2, "%08X %08X", samples->sample[k].pc, samples->sample[k].pr);
    }
}

/** @brief The screen of the watchdog: the last samples of the CPUs (PC and PR), then the program stops (polls the VBLANK) */
static void                                 jo_core_watchdog_screen(const void * const corrupt, const unsigned int * const previous)
{
    jo_core_watchdog_samples                copy[2];
    const unsigned int                      *header;

    __jo_core_set_sr(JO_SH2_SR_INTERRUPT_MASK);
    // (the samples of the moment: the slave CPU keeps running)
    jo_memcpy(copy, (const void *)JO_CACHE_THROUGH(__jo_core_watchdog_samples), sizeof(copy));
    JO_SH2_CHCR0 = 0;
    jo_core_error_background(JO_COLOR_Blue);
    jo_clear_screen();
    jo_set_printf_color_index(JO_COLOR_INDEX_White);
    jo_printf(0, 1, "   >>> Jo Engine watchdog <<<");
    if (corrupt == JO_NULL)
        jo_printf_with_color(0, 3, JO_COLOR_INDEX_Red, "No frame for %d seconds", JO_CORE_WATCHDOG_SECONDS);
    else
    {
        // (the header of the broken block: zone (16 bits), size, caller; the block before it, its buffer written too far)
        header = (const unsigned int *)((unsigned int)corrupt & ~3u);
        jo_printf_with_color(0, 2, JO_COLOR_INDEX_Red, "Memory broken at %08X:", (unsigned int)corrupt);
        jo_printf_with_color(0, 3, JO_COLOR_INDEX_Red, "%08X %08X %08X", header[0], header[1], header[2]);
        jo_printf_with_color(0, 4, JO_COLOR_INDEX_Red, "Before: %08X size %X by %08X", previous[0], previous[1], previous[2]);
    }
    jo_set_printf_color_index(JO_COLOR_INDEX_White);
    jo_printf(0, 5, "Master CPU (%d samples), PC PR:", copy[0].count);
    jo_core_watchdog_show(&copy[0], 6);
    jo_printf(0, 11, "Slave CPU (%d samples), PC PR:", copy[1].count);
    jo_core_watchdog_show(&copy[1], 12);
    jo_printf(0, 17, "VBLANK %d, last frame %d", __jo_core_vblank_count, __jo_core_last_synch_vblank);
    if (corrupt != JO_NULL)
        jo_printf(0, 19, "(the block before: %s)", previous[3] == 0xFFFF ? "free" : "in use");
    jo_printf(0, 18, "Find PC and PR in game.map");
    jo_printf(0, 25, "Program stopped: reset the console");
    jo_core_error_show_usage(22);
    jo_set_printf_color_index(JO_COLOR_INDEX_White);
    __jo_vdp2_set_priority(JO_SPRITE_SCREEN, 0);
    __jo_vdp2_set_tv_display(true);
    for (;;)
    {
        while (JO_VDP2_TVSTAT & JO_CORE_TVSTAT_VBLANK)
            ;
        while (!(JO_VDP2_TVSTAT & JO_CORE_TVSTAT_VBLANK))
            ;
        __jo_vdp2_vblank();
    }
}

#endif // JO_CORE_WATCHDOG

#endif // JO_CORE_EXCEPTION_SCREEN

#endif

#ifdef JO_DEBUG
void            jo_core_suspend(void)
{
    int         wait_cursor;
    int         frame;

    for (JO_ZERO(frame), JO_ZERO(wait_cursor);; ++frame)
    {
#ifdef JO_COMPILE_WITH_PRINTF_SUPPORT
        if (frame > 4)
        {
            switch (wait_cursor)
            {
            case 0:
                jo_printf(0, 27, "-");
                break;
            case 1:
                jo_printf(0, 27, "/");
                break;
            case 2:
                jo_printf(0, 27, "I");
                break;
            }
            if (wait_cursor == 2)
                JO_ZERO(wait_cursor);
            else
                ++wait_cursor;
            JO_ZERO(frame);
        }
#endif // JO_COMPILE_WITH_PRINTF_SUPPORT
        __jo_core_synch();
        if (jo_is_pad1_available() && (jo_is_pad1_key_down(JO_KEY_START)
#ifdef JO_COMPILE_WITH_KEYBOARD_SUPPORT
            || (jo_keyboard_get_special_key() == JO_KEYBOARD_ENTER)
#endif // JO_COMPILE_WITH_KEYBOARD_SUPPORT
                                       ))
        {
#ifdef JO_COMPILE_WITH_PRINTF_SUPPORT
            jo_printf(0, 27, " ");
#endif // JO_COMPILE_WITH_PRINTF_SUPPORT
            return ;
        }
    }
}
#endif

void    jo_core_set_restart_game_callback(const jo_event_callback callback)
{
    __jo_restart_game_callback = callback;
}

void    jo_core_set_cd_tray_open_callback(const jo_event_callback callback)
{
    __jo_cd_tray_open_callback = callback;
}

/** @brief The tray of the CD opened, A+B+C+Start on the first pad: the callback of the game, or the multiplayer (CD player)
 *  screen of the BIOS (what Sega asked of the games)
 */
/** @brief Frames A+B+C+Start are held before the restart (see jo_core_check_system_keys()) */
# define JO_CORE_RESET_KEYS_FRAMES          (4)
static int                                  __jo_core_reset_keys_frames = 0;
/** @brief A disc was in the drive, the opening of the tray was reported (see jo_core_check_system_keys()) */
static bool                                 __jo_core_cd_disc_seen = false;
static bool                                 __jo_core_cd_tray_reported = false;

static void                                 jo_core_check_system_keys(void)
{
    unsigned short                          status;

    // (the drive status of the last report of the CD block, periodic or the answer to a command: CR1 high byte, 0xff for a
    // rejected command)
    status = JO_CORE_CD_CR1 >> 8;
    status = status == 0xff ? 0 : (status & 0x0f);
    if (status == JO_CORE_CD_STATUS_OPEN || status == JO_CORE_CD_STATUS_NO_DISC || (JO_CORE_CD_HIRQ & JO_CORE_CD_HIRQ_DCHG))
    {
        JO_CORE_CD_HIRQ = (unsigned short)~JO_CORE_CD_HIRQ_DCHG;
        // Once when the tray opens (not at each frame while it is open), and only after a disc was seen: a program started
        // without a disc (a cartridge, a program loaded in the memory) does not leave at its first frame
        if (__jo_core_cd_disc_seen && !__jo_core_cd_tray_reported)
        {
            __jo_core_cd_tray_reported = true;
            if (__jo_cd_tray_open_callback != JO_NULL)
                __jo_cd_tray_open_callback();
            else
                jo_core_exit_to_multiplayer();
        }
    }
    else if (status != 0)
    {
        __jo_core_cd_disc_seen = true;
        __jo_core_cd_tray_reported = false;
    }
    // (held during JO_CORE_RESET_KEYS_FRAMES frames: the first reports of the pad after the boot can read all the keys
    // pressed for 1 or 2 frames)
    if (jo_is_pad1_available() && jo_is_pad1_key_pressed(JO_KEY_A) && jo_is_pad1_key_pressed(JO_KEY_B) &&
            jo_is_pad1_key_pressed(JO_KEY_C) && jo_is_pad1_key_pressed(JO_KEY_START))
    {
        // (once: the callback restarted the game at each frame while the keys were held)
        if (__jo_core_reset_keys_frames < JO_CORE_RESET_KEYS_FRAMES)
            ++__jo_core_reset_keys_frames;
        else if (__jo_core_reset_keys_frames == JO_CORE_RESET_KEYS_FRAMES)
        {
            ++__jo_core_reset_keys_frames;
            if (__jo_restart_game_callback != JO_NULL)
                __jo_restart_game_callback();
            else
                jo_core_exit_to_multiplayer();
        }
    }
    else
        JO_ZERO(__jo_core_reset_keys_frames);
}

void			        jo_core_run(void)
{
    jo_profile_begin(JO_PROFILE_FRAME);
    for (;;)
    {
        // The callbacks of a frame: their VDP2 scroll and rotation parameters are applied with its image (see
        // __jo_vdp2_end_frame())
        __jo_core_frame_open = true;
#ifdef JO_TEST_PCPROF_FRAME
        {
            static int pcprof_frame;
# ifndef JO_TEST_PCPROF_MASTER
#  define JO_TEST_PCPROF_MASTER 0x002E0000
# endif
# ifndef JO_TEST_PCPROF_SLAVE
#  define JO_TEST_PCPROF_SLAVE  0x002F0000
# endif
            if (++pcprof_frame == JO_TEST_PCPROF_FRAME)
                jo_profile_sampling_start((unsigned int *)JO_TEST_PCPROF_MASTER,
# ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
                                          (unsigned int *)JO_TEST_PCPROF_SLAVE);
# else
                                          JO_NULL);
# endif
        }
#endif

#ifdef JO_COMPILE_WITH_3D_SUPPORT
        __jo_3d_reset_matrix();
#endif

#ifdef JO_COMPILE_WITH_CONSOLE_SUPPORT
    if (jo_is_pad1_available() && jo_is_pad1_key_pressed(JO_KEY_DOWN) && jo_is_pad1_key_pressed(JO_KEY_START))
        jo_console_show_dialog();
#endif

#ifdef JO_COMPILE_WITH_STORYBOARD_SUPPORT
        if (__storyboards.count)
            jo_execute_storyboards();
#endif

#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
        if (__slave_callbacks.count)
        {
            *(volatile bool *)JO_CACHE_THROUGH(&__jo_slave_callbacks_running) = true;
            jo_core_exec_on_slave(jo_slave_callbacks);
        }
#endif
        jo_core_check_system_keys();
#ifdef JO_DEBUG
        jo_core_check_null_access();
        jo_core_check_held_error();
#endif
#ifdef JO_COMPILE_WITH_FS_SUPPORT
        if (__jo_core_music_update != JO_NULL)
            __jo_core_music_update();
        if (__jo_fs_background_job_count)
            jo_fs_do_background_jobs();
#endif
        jo_profile_begin(JO_PROFILE_CALLBACKS);
        jo_list_foreach(&__callbacks, __jo_call_event);
        jo_profile_end(JO_PROFILE_CALLBACKS);
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
        if (*(volatile bool *)JO_CACHE_THROUGH(&__jo_slave_callbacks_running))
        {
            jo_core_wait_for_slave();
            __jo_slave_callbacks_done();
        }
#endif

        __jo_core_frame_open = false;
        __jo_core_synch();
        __jo_gouraud_shading_runtime_index = -1;
#ifdef JO_COMPILE_WITH_PROFILER_SUPPORT
        jo_profile_end(JO_PROFILE_FRAME);
        jo_profile_begin(JO_PROFILE_FRAME);
        __jo_profile_end_frame();
#endif
    }
    jo_goto_boot_menu();
}

#ifdef JO_DEBUG

void                jo_dump_vdp1_registers(void)
{
    unsigned short  *ptr;

    for (ptr = (unsigned short *)JO_VDP1_REG; ptr <= (unsigned short *)JO_VDP1_LAST_REG; ++ptr)
        jo_core_error("VDP1: 0x%X = 0x%X ", (int)ptr, *ptr);
}

void                jo_dump_vdp2_registers(void)
{
    unsigned short  *ptr;

    for (ptr = (unsigned short *)JO_VDP2_REG; ptr <= (unsigned short *)JO_VDP2_LAST_REG; ++ptr)
        jo_core_error("VDP2: 0x%X = 0x%X ", (int)ptr, *ptr);
}

#endif

int     main(void)
{
	// (the .bss is cleared by start.S)
	jo_main();
	return (0);
}


/*
** END OF FILE
*/
