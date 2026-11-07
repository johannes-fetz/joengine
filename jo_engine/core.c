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

__jo_force_inline void __jo_call_event(jo_node *node)
{
    ((jo_event_callback)node->data.ptr)();
}

#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
static void jo_slave_callbacks(void)
{
    jo_list_foreach(&__slave_callbacks, __jo_call_event);
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

/** @brief SCU interrupt mask (1: masked) */
static unsigned int                         jo_core_scu_mask = JO_SCU_INT_ALL;

void                                        __jo_core_enable_scu_interrupts(const unsigned int mask)
{
    unsigned int                            sr;

    sr = __jo_core_disable_interrupts();
    jo_core_scu_mask = (jo_core_scu_mask & ~mask) & JO_SCU_INT_ALL;
    // The BIOS copy first: an interrupt handled by the BIOS writes it to the SCU when it returns
    JO_BIOS_SCU_IMS = jo_core_scu_mask;
    JO_SCU_IMS = jo_core_scu_mask;
    __jo_core_restore_interrupts(sr);
}

void                                        __jo_core_restore_scu_mask(void)
{
    JO_SCU_IMS = jo_core_scu_mask;
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

    // Reading the quotient waits for the end of a division in progress
    quotient = JO_SH2_DVDNTL;
    divisor = JO_SH2_DVSR;
    high = JO_SH2_DVDNTH;
    ++__jo_core_vblank_count;
    __jo_time_vblank();
    __jo_vdp2_vblank();
    __jo_vdp1_vblank();
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
    __jo_core_enable_scu_interrupts(JO_SCU_INT_VBLANK_IN | JO_SCU_INT_SMPC);
    JO_ZERO(__jo_core_vblank_count);
    JO_ZERO(__jo_core_last_synch_vblank);
    __jo_core_set_sr(JO_SH2_SR_INTERRUPTS_ON);
}

void                                        __jo_core_synch(void)
{
    unsigned int                            target;

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

bool                                        jo_core_set_screen_resolution(const int width, const int height)
{
    bool                                    pal;

    pal = (JO_VDP2_TVSTAT & JO_VDP2_TVSTAT_PAL) != 0;
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
    jo_core_add_vblank_callback(jo_get_inputs_vblank);
#if defined(JO_COMPILE_WITH_AUDIO_SUPPORT)
    jo_audio_init();
    jo_audio_set_volume(JO_DEFAULT_AUDIO_VOLUME);
#endif
#ifdef JO_COMPILE_WITH_3D_SUPPORT
    jo_3d_init();
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

void            __jo_core_errorf(const char *function, const char *format, ...)
{
    va_list     args;

    va_start(args, format);
    jo_vsnprintf(__jo_last_error, sizeof(__jo_last_error), format, args);
    va_end(args);
    __jo_core_error(__jo_last_error, function);
}

static jo_core_error_handler                 __jo_core_error_handler = JO_NULL;

void            jo_core_set_error_handler(const jo_core_error_handler handler)
{
    __jo_core_error_handler = handler;
}

void            __jo_core_error(char *message, const char *function)
{
#ifdef JO_COMPILE_WITH_PRINTF_SUPPORT
    int         usage;
#endif

    if (__jo_core_error_handler != JO_NULL)
    {
        __jo_core_error_handler(message, function);
        return;
    }
#ifdef JO_COMPILE_WITH_PRINTF_SUPPORT

    // (the NBG1 bitmap can't be allocated, see __jo_vdp2_reserve_nbg1_bitmap(): the back color, the text stays on NBG0)
    if (__jo_vdp2_reserve_nbg1_bitmap())
        jo_vdp2_clear_bitmap_nbg1(JO_COLOR_Blue);
    else
        jo_set_default_background_color(JO_COLOR_Blue);
    jo_set_printf_color_index(0);
    jo_printf(0, 13, "   >>> Jo Engine error handler <<<");
    usage = jo_memory_usage_percent();
    if (usage > 75)
        jo_printf_with_color(0, 17, JO_COLOR_INDEX_Red, "Dynamic memory usage: %d%%  ", usage);
    else if (usage > 33)
        jo_printf_with_color(0, 17, JO_COLOR_INDEX_Yellow, "Dynamic memory usage: %d%%  ", usage);
    else
        jo_printf_with_color(0, 17, JO_COLOR_INDEX_Green, "Dynamic memory usage: %d%%  ", usage);
    usage = jo_sprite_usage_percent();
    if (usage > 75)
        jo_printf_with_color(0, 18, JO_COLOR_INDEX_Red, "Sprite memory usage:  %d%%  ", usage);
    else if (usage > 33)
        jo_printf_with_color(0, 18, JO_COLOR_INDEX_Yellow, "Sprite memory usage:  %d%%  ", usage);
    else
        jo_printf_with_color(0, 18, JO_COLOR_INDEX_Green, "Sprite memory usage:  %d%%  ", usage);
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
