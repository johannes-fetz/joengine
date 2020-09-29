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

/*
** INCLUDES
*/
#include <stdbool.h>
#include <stdarg.h>
#include "jo/sgl_prototypes.h"
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
extern void                                 jo_main(void);
void                                        __jo_init_vdp2(const jo_color back_color);
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
extern jo_list                              __slave_callbacks;
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
void                                        jo_input_update(void);
void                                        jo_input_init(void);
#ifdef JO_COMPILE_WITH_FS_SUPPORT
int						                    jo_fs_init(void);
#endif
void                                        jo_fs_do_background_jobs(void);
void                                        jo_sprite_init(void);
void                                        jo_time_init(unsigned char mode);
void                                        jo_get_inputs_vblank(void);

#ifdef JO_DEBUG
char                                        __jo_last_error[JO_PRINTF_BUF_SIZE];
#endif

static jo_list                             __vblank_callbacks;
static jo_list                             __callbacks;
static jo_event_callback                   __jo_restart_game_callback = JO_NULL;

#ifdef JO_COMPILE_WITH_RAM_CARD_SUPPORT

#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
extern jo_list                              __slave_callbacks;
void                                        jo_core_slave_init(void);
#endif // JO_COMPILE_WITH_DUAL_CPU_SUPPORT

static  __jo_force_inline void              jo_enable_extended_ram_cartridge_support(void)
{
    *((int *)0x257efffe) = 1;
}

static  __jo_force_inline void              jo_set_a_bus_register(void)
{
    *((int *)0x25fe0080) = 0x23301ff0;
    *((int *)0x25fe0033) = 0x00000013;
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

#if !JO_COMPILE_USING_SGL

static __jo_force_inline void jo_wait_vblank_out(void)
{
    while ((JO_VDP2_TVSTAT & 8) == 8);
}

static __jo_force_inline void jo_wait_vblank_in(void)
{
    while ((JO_VDP2_TVSTAT & 8) == 0);
}

#endif

static bool                 jo_init_memory(void)
{
    static unsigned char    global_memory[JO_GLOBAL_MEMORY_SIZE_FOR_MALLOC];

    jo_add_memory_zone(global_memory, sizeof(global_memory));
#ifdef JO_COMPILE_WITH_RAM_CARD_SUPPORT
    if (jo_get_extended_ram_cartridge_type() == CART_8MBits)
    {
        jo_enable_extended_ram_cartridge_support();
        jo_set_a_bus_register();
        jo_add_memory_zone((unsigned char *)0x2247ffff, 0x180000);
        jo_add_memory_zone((unsigned char *)0x2267ffff, 0x180000);
    }
    else if (jo_get_extended_ram_cartridge_type() == CART_32MBits)
    {
        jo_enable_extended_ram_cartridge_support();
        jo_set_a_bus_register();
        jo_add_memory_zone((unsigned char *)0x2247ffff, 0x180000);
        jo_add_memory_zone((unsigned char *)0x2267ffff, 0x180000);
        jo_add_memory_zone((unsigned char *)0x2287ffff, 0x180000);
        jo_add_memory_zone((unsigned char *)0x22a7ffff, 0x180000);
        jo_add_memory_zone((unsigned char *)0x22c7ffff, 0x180000);
        jo_add_memory_zone((unsigned char *)0x22e7ffff, 0x180000);
        jo_add_memory_zone((unsigned char *)0x2307ffff, 0x180000);
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

static void         jo_core_init_vdp(const jo_color back_color)
{
#if JO_COMPILE_USING_SGL
    slInitSystem(JO_TV_RES, (TEXTURE *)__jo_sprite_def, JO_FRAMERATE/*Real frame rate = (60 / JO_FRAMERATE)*/);
    slTVOff();
    __jo_init_vdp2(back_color);
#else
    slInitSystem(0, 0, 0); // TODO fix nosgl.linker script

    jo_memset((void *)JO_VDP1_REG, 0, 0x16);
    jo_memset((void *)JO_VDP2_REG, 0, 0x11E);
    jo_memset((void *)JO_VDP2_VRAM, 0, 0x40000);
    jo_memset((void *)JO_VDP2_CRAM, 0, 0x0800);

    JO_VDP1_EWRR = (((JO_TV_WIDTH >> 3) << 9) | JO_TV_HEIGHT);
    JO_VDP1_EDSR = 0x3;
    JO_VDP1_PTMR = 0x2;
    JO_VDP1_MODR = 0x1100;
#if defined(JO_NTSC_VERSION)
    JO_VDP2_HCNT = 0x275;
#else
    JO_VDP2_HCNT = 0x26B;
#endif
    JO_VDP2_VCNT = 0x36;
    JO_VDP2_RAMCTL = 0x1327;

    JO_VDP2_CYCA0L = 0x5555;
    JO_VDP2_CYCA0U = 0xFEEE;
    JO_VDP2_CYCA1L = 0x5555;
    JO_VDP2_CYCA1U = 0xFEEE;
    JO_VDP2_CYCB0L = 0xFFFF;
    JO_VDP2_CYCB0U = 0xEEEE;
    JO_VDP2_CYCB1L = 0x44F;
    JO_VDP2_CYCB1U = 0xEEEE;

    JO_VDP2_BGON = 0x3;

    JO_VDP2_CHCTLA = 0x3210;
    JO_VDP2_CHCTLB = 0x1000;

    JO_VDP2_PNCN0 = 0x800C;
    JO_VDP2_PNCN1 = 0xC00E;
    JO_VDP2_PNCN2 = 0xC000;
    JO_VDP2_PNCN3 = 0xC000;
    JO_VDP2_PNCR = 0xC000;

    JO_VDP2_MPABN0 = 0x3B3B;
    JO_VDP2_MPCDN0 = 0x3B3B;
    JO_VDP2_MPABN1 = 0x3939;
    JO_VDP2_MPCDN1 = 0x3939;

    JO_VDP2_MPABRA = 0x2020;
    JO_VDP2_MPCDRA = 0x2020;
    JO_VDP2_MPEFRA = 0x2020;
    JO_VDP2_MPGHRA = 0x2020;
    JO_VDP2_MPIJRA = 0x2020;
    JO_VDP2_MPKLRA = 0x2020;
    JO_VDP2_MPMNRA = 0x2020;
    JO_VDP2_MPOPRA = 0x2020;

    JO_VDP2_MPABRB = 0x2828;
    JO_VDP2_MPCDRB = 0x2828;
    JO_VDP2_MPEFRB = 0x2828;
    JO_VDP2_MPGHRB = 0x2838;
    JO_VDP2_MPIJRB = 0x2828;
    JO_VDP2_MPKLRB = 0x2828;
    JO_VDP2_MPMNRB = 0x2828;
    JO_VDP2_MPOPRB = 0x2828;

    JO_VDP2_ZMXIN0 = 0x1;
    JO_VDP2_ZMYIN0 = 0x1;

    JO_VDP2_ZMXIN1 = 0x1;
    JO_VDP2_ZMYIN1 = 0x1;

    JO_VDP2_BKTAU = 0x12F1;
    JO_VDP2_BKTAL = 0xFFFF;

    JO_VDP2_RPTAU = 0x1;
    JO_VDP2_RPTAL = 0xFF80;

    JO_VDP2_WPEX0 = 0x27E;
    JO_VDP2_WPEY0 = 0x1BE;

    JO_VDP2_WPEX1 = 0x27E;
    JO_VDP2_WPEY1 = 0x1BE;

    JO_VDP2_SPCTL = 0x23;

    JO_VDP2_PRISA = 0x506;
    JO_VDP2_PRISB = 0x505;
    JO_VDP2_PRISC = 0x505;
    JO_VDP2_PRISD = 0x505;
    JO_VDP2_PRINA = 0x607;
    JO_VDP2_PRINB = 0x102;
    JO_VDP2_PRIR = 0x4;

    jo_vdp2_clear_bitmap_nbg1(back_color);
    jo_set_vdp2_4k_default_color_palette();
    jo_set_printf_color_index(0);
#endif
}

void                    __jo_core_set_screens_order(jo_scroll_screen screen1, ...)
{
#if JO_COMPILE_USING_SGL
    va_list             va;
    int                 priority;
    jo_scroll_screen    screen;

    va_start(va, screen1);
    priority = 7;
    screen = screen1;
    do
    {
        switch (screen)
        {
            case JO_NBG0_SCREEN: slPriorityNbg0(priority); break;
            case JO_NBG1_SCREEN: slPriorityNbg1(priority); break;
            case JO_NBG2_SCREEN: slPriorityNbg2(priority); break;
            case JO_NBG3_SCREEN: slPriorityNbg3(priority); break;
            case JO_RBG0_SCREEN: slPriorityRbg0(priority); break;
            case JO_SPRITE_SCREEN:
                slPrioritySpr0(priority);
                slPrioritySpr1(priority);
                slPrioritySpr2(priority);
                slPrioritySpr3(priority);
                slPrioritySpr4(priority);
                slPrioritySpr5(priority);
                slPrioritySpr6(priority);
                slPrioritySpr7(priority);
                break;
            default: break;
        }
        screen = va_arg(va, jo_scroll_screen);
        --priority;
    } while (screen != ((jo_scroll_screen)~0));
    va_end(va);
#else
    JO_UNUSED_ARG(screen1);
    //TODO
#endif // JO_COMPILE_USING_SGL
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
    jo_list_init(&__vblank_callbacks);
    jo_list_init(&__callbacks);
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
    jo_core_slave_init();
#endif
    jo_core_init_vdp(back_color);
    jo_core_add_vblank_callback(jo_get_inputs_vblank);
#if !JO_COMPILE_USING_SGL
    jo_input_init();
#endif
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
#if JO_COMPILE_USING_SGL
    slTVOn();
#else
    jo_vdp1_buffer_init();
# if defined(JO_NTSC_VERSION)
    JO_VDP2_TVMD = 0x8110;
#else
    JO_VDP2_TVMD = 0x8120;
#endif
#endif
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
    // random seed initialization
    jo_getdate(&dt);
    jo_random_seed = JO_MULT_BY_4096(dt.second) +
                    JO_MULT_BY_1024(dt.minute) +
                    JO_MULT_BY_65536(dt.hour) +
                    JO_MULT_BY_2048(dt.week) -
                    JO_MULT_BY_32(dt.year) -
                    JO_MULT_BY_256(dt.month) -
                    JO_MULT_BY_16(dt.day);
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
#if JO_COMPILE_USING_SGL
    if (__vblank_callbacks.count == 1)
        slIntFunction(__jo_vblank_callbacks);
#endif // JO_COMPILE_USING_SGL
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
#if JO_COMPILE_USING_SGL
    if (__vblank_callbacks.count == 0)
        slIntFunction(JO_NULL);
#endif // JO_COMPILE_USING_SGL
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
#if JO_COMPILE_USING_SGL
    SYS_Exit(0);
#endif
}

#ifdef JO_DEBUG

void            __jo_core_error(char *message, const char *function)
{
#ifdef JO_COMPILE_WITH_PRINTF_SUPPORT
    int         usage;

    jo_vdp2_clear_bitmap_nbg1(JO_COLOR_Blue);
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
    jo_printf_with_color(0, 23, JO_COLOR_INDEX_Red, message);
    jo_set_printf_color_index(0);
    jo_printf(2, 27, "Press [START] to continue...");
    jo_core_suspend();
    jo_clear_screen();
#else
    JO_UNUSED_ARG(message);
    JO_UNUSED_ARG(function);
    jo_vdp2_clear_bitmap_nbg1(JO_COLOR_Red);
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
#if JO_COMPILE_USING_SGL
        slSynch();
#else
        jo_input_update();
        jo_wait_vblank_out();
        jo_wait_vblank_in();
#endif
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

void			        jo_core_run(void)
{
    for (;;)
    {
#if !JO_COMPILE_USING_SGL
        jo_vdp1_buffer_reset();
#endif

#ifdef JO_COMPILE_WITH_3D_SUPPORT
    #if JO_COMPILE_USING_SGL
        slUnitMatrix(0);
    #else
        /* TODO slUnitMatrix replacement */
    #endif
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
            jo_core_exec_on_slave(jo_slave_callbacks);
#endif
        if (jo_is_pad1_key_pressed(JO_KEY_A) && jo_is_pad1_key_pressed(JO_KEY_B) &&
                jo_is_pad1_key_pressed(JO_KEY_C) && jo_is_pad1_key_pressed(JO_KEY_START) && __jo_restart_game_callback != JO_NULL)
            __jo_restart_game_callback();
#ifdef JO_COMPILE_WITH_FS_SUPPORT
        if (__jo_fs_background_job_count)
            jo_fs_do_background_jobs();
#endif
        jo_list_foreach(&__callbacks, __jo_call_event);
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
        if (__slave_callbacks.count)
            jo_core_wait_for_slave();
#endif

#if JO_COMPILE_USING_SGL
        slSynch();
#else
        jo_vdp1_flush();
        jo_wait_vblank_out();
        __jo_vblank_callbacks();
        jo_wait_vblank_in();
        jo_input_update();
#endif
        __jo_gouraud_shading_runtime_index = -1;
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
	jo_memset(&_bstart, 0, &_bend - &_bstart);
    jo_memset((void *)JO_WORK_RAM_SYSTEM_WORK, 0, JO_WORK_RAM_SYSTEM_WORK_SIZE);
	jo_main();
	return (0);
}


/*
** END OF FILE
*/
