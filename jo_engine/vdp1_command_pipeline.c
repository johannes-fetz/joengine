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
#include "jo/time.h"
#include "jo/core.h"
#include "jo/math.h"
#include "jo/tools.h"
#include "jo/malloc.h"
#include "jo/list.h"
#include "jo/colors.h"
#include "jo/vdp1_command_pipeline.h"
#include "jo/image.h"
#include "jo/sprites.h"
#include "jo/3d.h"
#include "jo/profile.h"

/*
** Commands are collected in work RAM during the frame with a depth value.
** At the end of the frame they are sorted from far to near and copied into one
** of the two command tables of the VDP1 VRAM. The first command of the VRAM is a
** "skip and jump" to the table to draw: during VBLANK we only change this jump,
** so the VDP1 always reads a complete table.
**
** With the dual CPU support, the sort and the copy are a job of the slave CPU
** (__jo_core_add_slave_job()): the master CPU fills the other command buffer with
** the next frame meanwhile. The table is displayed at the first VBLANK after the copy,
** like when the master CPU copies it.
*/

/*
** INTERNAL MACROS
*/
/** @brief VRAM offset of the command table A */
# define JO_VDP1_TABLE_A_OFFSET             (0x00020)
/** @brief VRAM offset of the command table B */
# define JO_VDP1_TABLE_B_OFFSET             (0x08000)
/* (JO_VDP1_MAX_COMMANDS: see jo/vdp1_command_pipeline.h. The command tables A and B of the VRAM hold 1023 commands) */
_Static_assert(JO_VDP1_MAX_COMMANDS >= 64 && JO_VDP1_MAX_COMMANDS <= 1000, "JO_VDP1_MAX_COMMANDS: 64 to 1000");
/* (bigger tables (table B from 0x0A800 or 0x0C000, the textures after them, JO_VDP1_TEXTURE_DEF_BASE_ADDRESS): the
** a big level fills the VDP1 VRAM, the textures moved by 32 KB don't fit any more ("Out of sprite memory"). The tables
** stay in the first 64 KB) */
/** @brief Commands written before each window: system clipping, user clipping and local coordinates */
# define JO_VDP1_HEADER_COMMANDS            (3)
/** @brief Command at a VRAM offset */
# define JO_VDP1_COMMAND_AT(OFFSET)         ((volatile jo_vdp1_command *)(JO_VDP1_VRAM + (OFFSET)))
/** @brief 32 bits word of a command (may alias a jo_vdp1_command) */
typedef unsigned int                        __attribute__((may_alias)) jo_vdp1_word;
/** @brief Hi-res mode (640 or 704 pixels width) */
# define JO_VDP1_HIRES                      (JO_TV_WIDTH > 352)
/** @brief Double density interlace (480 lines) */
# define JO_VDP1_INTERLACE                  (JO_TV_HEIGHT > 256)
/** @brief Frame buffer lines */
# define JO_VDP1_LINES                      (JO_VDP1_INTERLACE ? JO_TV_HEIGHT_2 : JO_TV_HEIGHT)
/** @brief TVMR: 8 bits per pixel, 1024x256 frame buffer (hi-res modes) */
# define JO_VDP1_TVMR_HIRES                 (0x0001)
/** @brief FBCR: double density interlace (DIE | DIL) */
# define JO_VDP1_FBCR_DOUBLE_INTERLACE      (0x000C)
/** @brief FBCR: manual frame buffer change mode (FCM) and change at the next VBLANK (FCT) */
# define JO_VDP1_FBCR_MANUAL                (0x0002)
# define JO_VDP1_FBCR_MANUAL_CHANGE         (0x0003)
/** @brief The frame buffers change only when the VDP1 finished drawing a new frame (JO_FRAMERATE > 1, or the dynamic
 *  frame rate JO_FRAMERATE_DYNAMIC): in the 1-cycle mode, they change at each VBLANK and the VDP1 must draw a frame in
 *  1/60 s (the end of a frame that takes longer is not drawn: on the real hardware and on the accurate emulators, the last
 *  commands like the HUD disappear). The frame buffer is cleared by a polygon at the start of each command table */
#ifdef JO_FRAMERATE_DYNAMIC
# define JO_VDP1_MANUAL_CHANGE              (!JO_VDP1_INTERLACE)
#else
# define JO_VDP1_MANUAL_CHANGE              (JO_FRAMERATE > 1 && !JO_VDP1_INTERLACE)
#endif
/** @brief PMOD of the clear polygon: replace, no end code and no transparent pixel check */
# define JO_VDP1_CLEAR_POLYGON_MODE         (0x00C0)
/** @brief PTMR: the drawing starts automatically when the frame buffer changes */
# define JO_VDP1_PTMR_AUTOMATIC_DRAW        (0x0002)
/** @brief EDSR: the drawing of the current frame is finished */
# define JO_VDP1_EDSR_CURRENT_END           (0x0002)
/** @brief EWRR: erase/write area (right in 8 or 16 pixels unit, excluded: the width / unit; bottom line, included) */
# define JO_VDP1_ERASE_AREA(X_UNIT, BOTTOM)       (((JO_TV_WIDTH / (X_UNIT)) << 9) | ((BOTTOM) - 1))
/** @brief Default Z limit of a window (no limit) */
# define JO_VDP1_NO_Z_LIMIT                 (0x7fff0000)
/** @brief Command link: the VRAM address divided by 8 */
# define JO_VDP1_LINK(OFFSET)               ((OFFSET) >> 3)
/** @brief Radix sort: 256 buckets per byte */
# define JO_VDP1_RADIX                      (256)
/** @brief Light table count (one per brightness level) */
# define JO_VDP1_LIGHT_LEVEL_COUNT          (32)
/** @brief Size of a gouraud table (4 colors) */
# define JO_VDP1_GOURAUD_TABLE_SIZE         (8)

/*
** GLOBALS
*/
extern jo_texture_definition            __jo_sprite_def[];
jo_vdp1_window                          __jo_vdp1_windows[JO_VDP1_WINDOW_COUNT];
int                                     __jo_vdp1_current_window = 0;
jo_fixed                                __jo_vdp1_projection_distance = JO_MULT_BY_65536(JO_TV_WIDTH_2);
unsigned short                          __jo_vdp1_display_level = 1;
jo_fixed                                __jo_vdp1_near_z = JO_MULT_BY_65536(JO_TV_WIDTH_2) >> 1;
short                                   __jo_vdp1_system_clip_right = JO_TV_WIDTH - 1;
short                                   __jo_vdp1_system_clip_bottom = JO_TV_HEIGHT - 1;

/* Depth added to the keys of the commands (see jo_3d_set_depth_bias()) */
int                                     __jo_vdp1_depth_bias = 0;
/** @brief Command buffers: the master CPU fills one while the slave CPU sorts and copies the other one into the VRAM */
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
# define JO_VDP1_FRAME_BUFFERS              (2)
#else
# define JO_VDP1_FRAME_BUFFERS              (1)
#endif
/** @brief Variable shared by the CPUs: read and written through its cache-through address */
# define JO_VDP1_SHARED(VARIABLE)           (*(volatile __typeof__(VARIABLE) *)JO_CACHE_THROUGH(&(VARIABLE)))

/** @brief Commands of a frame: filled by the master CPU, sorted and copied into the VRAM at the end of the frame */
typedef struct
{
    jo_vdp1_command                     commands[JO_VDP1_MAX_COMMANDS] __attribute__((aligned(4)));
    unsigned short                      keys[JO_VDP1_MAX_COMMANDS];
    unsigned short                      window[JO_VDP1_MAX_COMMANDS];
    unsigned int                        count;
    jo_vdp1_window                      windows[JO_VDP1_WINDOW_COUNT]; // The windows at the end of the frame
    short                               system_clip_right;      // The system clipping at the end of the frame
    short                               system_clip_bottom;
    unsigned int                        tag;                    // Number of the frame (see __jo_vdp1_frame_tag())
    bool                                wireframe;              // The polygons drawn as their outline (see jo_3d_set_wireframe())
    bool                                windows_apart;          // The windows drawn one after the other (see jo_3d_set_windows_apart())
}                                       jo_vdp1_frame;

/** @brief The windows do not overlap: the commands of each window are drawn together (see jo_3d_set_windows_apart()) */
bool                                    __jo_vdp1_windows_apart = false;

/** @brief The polygons and the distorted sprites of the next frames drawn as their outline (see jo_3d_set_wireframe()) */
bool                                    __jo_vdp1_wireframe = false;
/** @brief Color of the outlines in wireframe mode (RGB) */
# define JO_VDP1_WIREFRAME_COLOR            (0x8000 | (8 << 10) | (31 << 5) | 8)

/** @brief The polygons and the distorted sprites of a frame changed into polylines (their outline, one color): the 3D
 *  objects and the standing sprites in wireframe, the normal and scaled sprites (the HUD) stay as they are
 *  @param frame Frame (before its copy into the VRAM)
 */
static void                             jo_vdp1_make_wireframe(jo_vdp1_frame * const frame)
{
    jo_vdp1_command                     *cmd;
    unsigned int                        k;
    unsigned short                      function;

    for (JO_ZERO(k), cmd = frame->commands; k < frame->count; ++k, ++cmd)
    {
        function = cmd->ctrl & 0xF;
        if (function != JO_VDP1_CMD_DISTORTED_SPRITE && function != JO_VDP1_CMD_POLYGON)
            continue;
        cmd->ctrl = (cmd->ctrl & ~0xF) | JO_VDP1_CMD_POLYLINE;
        // RGB color, no texture, no gouraud shading, the clipping of the window kept
        cmd->pmod = (cmd->pmod & (3 << 9)) | JO_VDP1_END_CODE_DISABLE | JO_VDP1_TRANSPARENT_PIXEL_DISABLE | JO_VDP1_COLOR_MODE_RGB;
        cmd->colr = JO_VDP1_WIREFRAME_COLOR;
        cmd->grda = 0;
    }
}

static jo_vdp1_frame                    __jo_vdp1_frames[JO_VDP1_FRAME_BUFFERS] JO_WORK_RAM_ENGINE_WORK_SECTION;
/* Frame filled by the master CPU */
static jo_vdp1_frame                    *__jo_vdp1_frame = &__jo_vdp1_frames[0];
static unsigned short                   __jo_vdp1_sort_a[JO_VDP1_MAX_COMMANDS] JO_WORK_RAM_ENGINE_WORK_SECTION;
static unsigned short                   __jo_vdp1_sort_b[JO_VDP1_MAX_COMMANDS] JO_WORK_RAM_ENGINE_WORK_SECTION;
/* Copy of the command tables into the VRAM: SCU DMA level (the SCU DMA can't read the low work RAM) */
# define JO_VDP1_SCU_DMA_LEVEL              (0)
/** @brief Commands added to the frames: the headers of the windows, the clear polygon and the end command */
# define JO_VDP1_DMA_CLEAR_COMMAND          (JO_VDP1_HEADER_COMMANDS * JO_VDP1_WINDOW_COUNT)
# define JO_VDP1_DMA_END_COMMAND            (JO_VDP1_DMA_CLEAR_COMMAND + 1)
static jo_vdp1_command                  __jo_vdp1_dma_commands[JO_VDP1_DMA_END_COMMAND + 1] __attribute__((aligned(16))) JO_WORK_RAM_ENGINE_WORK_SECTION;
/** @brief Indirect table of the SCU DMA: byte count, VRAM address, source address (one entry per command at most, see
 *  jo_vdp1_dma_add()). The table is aligned on a boundary of its size (rounded up to a power of 2) */
# define JO_VDP1_DMA_ENTRIES                ((JO_VDP1_TEXTURE_DEF_BASE_ADDRESS - JO_VDP1_TABLE_B_OFFSET) / 32 + 2)
# define JO_VDP1_DMA_TABLE_SIZE             (JO_VDP1_DMA_ENTRIES * 3 * sizeof(unsigned int))
/** @brief Alignment of the indirect table (bigger command tables: a bigger alignment, the SCU DMA reads wrong entries else) */
# define JO_VDP1_DMA_TABLE_ALIGNMENT        (JO_VDP1_DMA_TABLE_SIZE <= 8192 ? 8192 : (JO_VDP1_DMA_TABLE_SIZE <= 16384 ? 16384 : 32768))
_Static_assert(JO_VDP1_DMA_TABLE_SIZE <= 32768, "Indirect table of the SCU DMA too big");
static unsigned int                     __jo_vdp1_dma_table[JO_VDP1_DMA_ENTRIES * 3] __attribute__((aligned(JO_VDP1_DMA_TABLE_ALIGNMENT))) JO_WORK_RAM_ENGINE_WORK_ALIGNED_SECTION;
/* Radix sort counters (not on the stack: the stack of the slave CPU is small) */
/* (16 bits: a count is JO_VDP1_MAX_COMMANDS at most, read by mov.w with its sign: under 32768) */
_Static_assert(JO_VDP1_MAX_COMMANDS < 32768, "radix counters of 16 bits");
static unsigned short                   __jo_vdp1_radix_low[JO_VDP1_RADIX] JO_WORK_RAM_ENGINE_WORK_SECTION;
static unsigned short                   __jo_vdp1_radix_high[JO_VDP1_RADIX] JO_WORK_RAM_ENGINE_WORK_SECTION;

/* Offset of the table to display, 0 if none (JO_VDP1_SHARED) */
static volatile unsigned int            __jo_vdp1_pending_table;
/* Table to fill next (JO_VDP1_SHARED) */
static volatile unsigned int            __jo_vdp1_next_table;
/* VBLANK of the last table change, VBLANK count (JO_VDP1_SHARED) */
static volatile unsigned int            __jo_vdp1_flip_field;
static volatile unsigned int            __jo_vdp1_vblank_fields;
/* Frame numbers (see __jo_vdp1_frame_tag()): the frame built (master CPU), the table pending, the last table given to
** the VDP1, the frame displayed from the field __jo_vdp1_shown_field (and the next one in 1 cycle mode) (JO_VDP1_SHARED) */
static unsigned int                     __jo_vdp1_building_tag;
static volatile unsigned int            __jo_vdp1_pending_tag;
static volatile unsigned int            __jo_vdp1_taken_tag;
static volatile unsigned int            __jo_vdp1_shown_tag;
static volatile unsigned int            __jo_vdp1_shown_field;
static volatile unsigned int            __jo_vdp1_next_shown_tag;
static volatile unsigned int            __jo_vdp1_next_shown_field;
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
/* Frames given to the slave CPU (master CPU only), frames copied into the VRAM by the slave CPU (JO_VDP1_SHARED) */
static unsigned int                     __jo_vdp1_frames_sent;
static volatile unsigned int            __jo_vdp1_frames_done;
#endif

/** @brief Write an end command
 *  @param offset VRAM offset
 */
static void                             jo_vdp1_write_end(const unsigned int offset)
{
    JO_VDP1_COMMAND_AT(offset)->ctrl = JO_VDP1_CMD_END;
}

/** @brief Change the first command of the VRAM: jump to a command table
 *  @param table VRAM offset of the table
 */
static void                             jo_vdp1_write_jump(const unsigned int table)
{
    volatile jo_vdp1_command            *head;

    head = JO_VDP1_COMMAND_AT(0);
    head->link = JO_VDP1_LINK(table);
    head->ctrl = JO_VDP1_CMD_SKIP_ASSIGN;
}

void                                    __jo_vdp1_init(void)
{
    register unsigned int               i;
    unsigned short                      g;
    volatile unsigned short             *table;

    // Gouraud tables for lighting: level 0 (dark) to 31 (bright), 16 is neutral
    for (JO_ZERO(i); i < JO_VDP1_LIGHT_LEVEL_COUNT; ++i)
    {
        g = JO_COLOR_SATURN_RGB(i, i, i) & ~JO_COLOR_SATURN_MSB;
        table = (volatile unsigned short *)(JO_VDP1_VRAM + JO_VDP1_LIGHT_TABLE_OFFSET + i * JO_VDP1_GOURAUD_TABLE_SIZE);
        table[0] = g;
        table[1] = g;
        table[2] = g;
        table[3] = g;
    }
    // Empty tables
    jo_vdp1_write_end(JO_VDP1_TABLE_A_OFFSET);
    jo_vdp1_write_end(JO_VDP1_TABLE_B_OFFSET);
    jo_vdp1_write_jump(JO_VDP1_TABLE_A_OFFSET);
    JO_VDP1_SHARED(__jo_vdp1_next_table) = JO_VDP1_TABLE_B_OFFSET;
    JO_VDP1_SHARED(__jo_vdp1_pending_table) = 0;
    JO_VDP1_SHARED(__jo_vdp1_flip_field) = 0;
    JO_VDP1_SHARED(__jo_vdp1_vblank_fields) = 0;
    __jo_vdp1_building_tag = 1;
    JO_VDP1_SHARED(__jo_vdp1_pending_tag) = 0;
    JO_VDP1_SHARED(__jo_vdp1_taken_tag) = 0;
    JO_VDP1_SHARED(__jo_vdp1_shown_tag) = 0;
    JO_VDP1_SHARED(__jo_vdp1_shown_field) = ~0u;
    JO_VDP1_SHARED(__jo_vdp1_next_shown_field) = ~0u;
    for (JO_ZERO(i); i < JO_VDP1_FRAME_BUFFERS; ++i)
        JO_ZERO(__jo_vdp1_frames[i].count);
    __jo_vdp1_frame = &__jo_vdp1_frames[0];
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
    JO_ZERO(__jo_vdp1_frames_sent);
    JO_ZERO(JO_VDP1_SHARED(__jo_vdp1_frames_done));
#endif
    // Default window: whole screen
    for (JO_ZERO(i); i < JO_VDP1_WINDOW_COUNT; ++i)
    {
        __jo_vdp1_windows[i].left = 0;
        __jo_vdp1_windows[i].top = 0;
        __jo_vdp1_windows[i].right = JO_TV_WIDTH - 1;
        __jo_vdp1_windows[i].bottom = JO_TV_HEIGHT - 1;
        __jo_vdp1_windows[i].center_x = JO_TV_WIDTH_2;
        __jo_vdp1_windows[i].center_y = JO_TV_HEIGHT_2;
        __jo_vdp1_windows[i].zlimit = JO_VDP1_NO_Z_LIMIT;
        __jo_vdp1_windows[i].clip_mode = 0;
        __jo_vdp1_windows[i].origin_x = 0;
        __jo_vdp1_windows[i].origin_y = 0;
        __jo_vdp1_windows[i].screen = false;
    }
    __jo_vdp1_current_window = 0;
    // 16 bits frame buffer (8 bits in hi-res), 1 cycle mode
    JO_VDP1_TVMR = JO_VDP1_HIRES ? JO_VDP1_TVMR_HIRES : 0;
    JO_VDP1_FBCR = JO_VDP1_INTERLACE ? JO_VDP1_FBCR_DOUBLE_INTERLACE : (JO_VDP1_MANUAL_CHANGE ? JO_VDP1_FBCR_MANUAL : 0);
    // Frame buffer erase: transparent color on the whole screen
    JO_VDP1_EWDR = 0;
    JO_VDP1_EWLR = 0;
    JO_VDP1_EWRR = JO_VDP1_HIRES ? JO_VDP1_ERASE_AREA(16, JO_VDP1_LINES) : JO_VDP1_ERASE_AREA(8, JO_VDP1_LINES);
    JO_VDP1_PTMR = JO_VDP1_PTMR_AUTOMATIC_DRAW;
}

/** @brief Commands of the last frame sent (see jo_vdp1_command_count()) */
static int                              __jo_vdp1_last_count = 0;

int                                     jo_vdp1_command_count(void)
{
    return (__jo_vdp1_last_count);
}

int                                     __jo_vdp1_frame_command_count(void)
{
    jo_vdp1_frame                       *frame;

    // (read without the cache: the slave CPU calls it too, see jo_3d_subdivision_level())
    frame = *(jo_vdp1_frame * volatile *)JO_CACHE_THROUGH(&__jo_vdp1_frame);
    return ((int)*(volatile unsigned int *)JO_CACHE_THROUGH(&frame->count));
}

int                                     jo_vdp1_command_max(void)
{
    return (JO_VDP1_MAX_COMMANDS - JO_VDP1_HEADER_COMMANDS * 2 - 1);
}

jo_vdp1_command                         *__jo_vdp1_add_command(const jo_fixed z)
{
    jo_vdp1_command                     *cmd;
    jo_vdp1_frame                       *frame;

    frame = __jo_vdp1_frame;
    if (frame->count >= JO_VDP1_MAX_COMMANDS - JO_VDP1_HEADER_COMMANDS * 2 - 1)
    {
#ifdef JO_DEBUG
        // Reported once: the next commands of the frame are also dropped
        static bool                     reported = false;

        if (!reported)
        {
            reported = true;
            jo_core_error("Too many VDP1 commands in a frame (max=%d)", JO_VDP1_MAX_COMMANDS - JO_VDP1_HEADER_COMMANDS * 2 - 1);
        }
#endif
        return (JO_NULL);
    }
    frame->keys[frame->count] = __jo_vdp1_sort_key(z);
    frame->window[frame->count] = (unsigned short)__jo_vdp1_current_window;
    cmd = &frame->commands[frame->count++];
    cmd->link = 0;
    cmd->grda = 0;
    return (cmd);
}

jo_vdp1_command                         *__jo_vdp1_reserve_commands(const unsigned int count, unsigned short **keys, unsigned int *reserved)
{
    unsigned int                        available;
    jo_vdp1_frame                       *frame;

    frame = __jo_vdp1_frame;
    available = JO_VDP1_MAX_COMMANDS - JO_VDP1_HEADER_COMMANDS * 2 - 1 - frame->count;
    *reserved = count < available ? count : available;
    *keys = &frame->keys[frame->count];
    return (&frame->commands[frame->count]);
}

unsigned short                          *__jo_vdp1_commit_commands_without_window(const unsigned int used)
{
    jo_vdp1_frame                       *frame;
    unsigned short                      *window;

    frame = __jo_vdp1_frame;
    window = &frame->window[frame->count];
    frame->count += used;
    return (window);
}

void                                    __jo_vdp1_commit_commands(const unsigned int used)
{
    register unsigned int               i;
    unsigned short                      window;
    jo_vdp1_frame                       *frame;

    frame = __jo_vdp1_frame;
    window = (unsigned short)__jo_vdp1_current_window;
    for (JO_ZERO(i); i < used; ++i)
        frame->window[frame->count + i] = window;
    frame->count += used;
}

/** @brief Prepare the clipping and local coordinate commands of a window (copied into the VRAM by the SCU DMA)
 *  @param c 3 commands
 *  @param frame Frame (its windows and system clipping)
 *  @param window Window index
 */
static void                             jo_vdp1_make_header(jo_vdp1_command * const c, const jo_vdp1_frame * const frame, const int window)
{
    const jo_vdp1_window                *w;

    w = &frame->windows[window];
    jo_memset(c, 0, JO_VDP1_HEADER_COMMANDS * sizeof(jo_vdp1_command));
    c[0].ctrl = JO_VDP1_CMD_SYSTEM_CLIP;
    c[0].xc = frame->system_clip_right;
    c[0].yc = frame->system_clip_bottom;
    c[1].ctrl = JO_VDP1_CMD_USER_CLIP;
    c[1].xa = w->left;
    c[1].ya = w->top;
    c[1].xc = w->right;
    c[1].yc = w->bottom;
    c[2].ctrl = JO_VDP1_CMD_LOCAL_COORD;
    // (a clipped window: its commands are relative to its top left corner; the clipping stays in screen coordinates)
    c[2].xa = w->origin_x;
    c[2].ya = w->origin_y;
}

/** @brief Prepare a polygon of the transparent color on the whole frame buffer (manual change mode)
 *  @param c Command
 *  @param frame Frame (its system clipping)
 */
static void                             jo_vdp1_make_clear(jo_vdp1_command * const c, const jo_vdp1_frame * const frame)
{
    jo_memset(c, 0, sizeof(*c));
    c->ctrl = JO_VDP1_CMD_POLYGON;
    c->pmod = JO_VDP1_CLEAR_POLYGON_MODE;
    c->xb = frame->system_clip_right;
    c->xc = frame->system_clip_right;
    c->yc = frame->system_clip_bottom;
    c->yd = frame->system_clip_bottom;
}

/*
** Radix sort and indirect DMA table in assembly (the C code is kept with JO_VDP1_SORT_IN_C)
*/

/** @brief Count the low and the high bytes of the keys
 *  @param keys First key
 *  @param end End of the keys (at least one key)
 *  @param low Counters of the low bytes (256)
 *  @param high Counters of the high bytes (256)
 */
extern void                             __jo_vdp1_radix_count(const unsigned short *keys, const unsigned short *end, unsigned short *low,
                                                              unsigned short *high);
/** @brief First pass of the sort: out[low[key & 255]++] = index of the key
 *  @param keys First key
 *  @param end End of the keys (at least one key)
 *  @param low Positions of the low bytes (prefix sums)
 *  @param out Command indexes (output)
 */
extern void                             __jo_vdp1_radix_scatter_low(const unsigned short *keys, const unsigned short *end, unsigned short *low,
                                                                    unsigned short *out);
/** @brief Second pass of the sort: out[high[keys[i] >> 8]++] = i for each index i of the first pass
 *  @param keys Keys
 *  @param in Command indexes of the first pass
 *  @param end End of the indexes (at least one)
 *  @param high Positions of the high bytes (prefix sums)
 *  @param out Command indexes (output)
 */
extern void                             __jo_vdp1_radix_scatter_high(const unsigned short *keys, const unsigned short *in, const unsigned short *end,
                                                                     unsigned short *high, unsigned short *out);
__asm__(
"    .text\n"
"    .align 2\n"
"    .global ___jo_vdp1_radix_count\n"
"___jo_vdp1_radix_count:\n"
"1:  mov.w @r4+, r0\n"
"    extu.w r0, r1\n"
"    extu.b r0, r0\n"
"    add r0, r0\n"
"    mov.w @(r0, r6), r2\n"
"    add #1, r2\n"
"    mov.w r2, @(r0, r6)\n"
"    shlr8 r1\n"
"    add r1, r1\n"
"    mov r1, r0\n"
"    mov.w @(r0, r7), r2\n"
"    add #1, r2\n"
"    cmp/hs r5, r4\n"
"    bf.s 1b\n"
"    mov.w r2, @(r0, r7)\n"
"    rts\n"
"    nop\n"
"    .align 2\n"
"    .global ___jo_vdp1_radix_scatter_low\n"
"___jo_vdp1_radix_scatter_low:\n"
"    mov r4, r3\n"                      /* first key: the index is (key pointer - keys) / 2 */
"1:  mov r4, r1\n"
"    mov.w @r4+, r0\n"
"    sub r3, r1\n"
"    extu.b r0, r0\n"
"    add r0, r0\n"
"    mov.w @(r0, r6), r2\n"
"    shlr r1\n"
"    add #1, r2\n"
"    mov.w r2, @(r0, r6)\n"
"    add #-1, r2\n"
"    add r2, r2\n"
"    mov r2, r0\n"
"    cmp/hs r5, r4\n"
"    bf.s 1b\n"
"    mov.w r1, @(r0, r7)\n"
"    rts\n"
"    nop\n"
"    .align 2\n"
"    .global ___jo_vdp1_radix_scatter_high\n"
"___jo_vdp1_radix_scatter_high:\n"
"    mov.l @r15, r3\n"                  /* out (5th argument) */
"1:  mov.w @r5+, r0\n"
"    extu.w r0, r1\n"
"    add r0, r0\n"
"    mov.w @(r0, r4), r0\n"
"    extu.w r0, r0\n"
"    shlr8 r0\n"
"    add r0, r0\n"
"    mov.w @(r0, r7), r2\n"
"    add #1, r2\n"
"    mov.w r2, @(r0, r7)\n"
"    add #-1, r2\n"
"    add r2, r2\n"
"    mov r2, r0\n"
"    cmp/hs r6, r5\n"
"    bf.s 1b\n"
"    mov.w r1, @(r0, r3)\n"
"    rts\n"
"    nop\n"
);

/** @brief Sort the commands of a frame by depth (2 passes radix sort, stable)
 *  @param frame Frame
 *  @return Command indexes from the farthest to the nearest (same depth: submission order)
 *  @remarks A pass is skipped when all the keys have the same byte (for instance the high byte in 2D)
 */
static unsigned short                   *jo_vdp1_radix_sort(const jo_vdp1_frame * const frame)
{
    unsigned short                      *low;
    unsigned short                      *high;
    const unsigned short                *keys;
    register unsigned int               i;
    unsigned int                        n;
    unsigned int                        sum;
    unsigned int                        tmp;
#ifdef JO_VDP1_SORT_IN_C
    unsigned short                      key;
#endif
    unsigned short                      *order;

    low = __jo_vdp1_radix_low;
    high = __jo_vdp1_radix_high;
    keys = frame->keys;
    n = frame->count;
    jo_memset(low, 0, sizeof(__jo_vdp1_radix_low));
    jo_memset(high, 0, sizeof(__jo_vdp1_radix_high));
#ifdef JO_VDP1_SORT_IN_C
    for (JO_ZERO(i); i < n; ++i)
    {
        key = keys[i];
        ++low[key & (JO_VDP1_RADIX - 1)];
        ++high[JO_DIV_BY_256(key)];
    }
#else
    if (n > 0)
        __jo_vdp1_radix_count(keys, keys + n, low, high);
#endif
    // Low byte (buckets in descending order: the farthest first)
    order = __jo_vdp1_sort_a;
    if (n > 0 && low[keys[0] & (JO_VDP1_RADIX - 1)] == n)
    {
        for (JO_ZERO(i); i < n; ++i)
            __jo_vdp1_sort_a[i] = i;
    }
    else
    {
        for (i = JO_VDP1_RADIX, JO_ZERO(sum); i-- > 0;)
        {
            tmp = low[i];
            low[i] = (unsigned short)sum;
            sum += tmp;
        }
#ifdef JO_VDP1_SORT_IN_C
        for (JO_ZERO(i); i < n; ++i)
            __jo_vdp1_sort_a[low[keys[i] & (JO_VDP1_RADIX - 1)]++] = i;
#else
        __jo_vdp1_radix_scatter_low(keys, keys + n, low, __jo_vdp1_sort_a);
#endif
    }
    // High byte
    if (n > 0 && high[JO_DIV_BY_256(keys[0])] == n)
        return (order);
    for (i = JO_VDP1_RADIX, JO_ZERO(sum); i-- > 0;)
    {
        tmp = high[i];
        high[i] = (unsigned short)sum;
        sum += tmp;
    }
#ifdef JO_VDP1_SORT_IN_C
    for (JO_ZERO(i); i < n; ++i)
        __jo_vdp1_sort_b[high[JO_DIV_BY_256(keys[__jo_vdp1_sort_a[i]])]++] = __jo_vdp1_sort_a[i];
#else
    __jo_vdp1_radix_scatter_high(keys, __jo_vdp1_sort_a, __jo_vdp1_sort_a + n, high, __jo_vdp1_sort_b);
#endif
    return (__jo_vdp1_sort_b);
}

/** @brief The sorted commands grouped by window (window 0 first), in the order of the sort in each window: a header per
 *  window (the windows of a split screen do not overlap: mixed by depth, a header was inserted at each change of window,
 *  hundreds per frame)
 *  @param frame Frame
 *  @param order The sorted commands (see jo_vdp1_radix_sort())
 *  @return The grouped commands (__jo_vdp1_sort_a: the sort does not use it any more)
 */
static unsigned short                   *jo_vdp1_group_windows(const jo_vdp1_frame * const frame, const unsigned short * const order)
{
    unsigned int                        first[JO_VDP1_WINDOW_COUNT];
    unsigned int                        sum;
    register unsigned int               i;
    unsigned int                        n;

    n = frame->count;
    for (JO_ZERO(i); i < JO_VDP1_WINDOW_COUNT; ++i)
        JO_ZERO(first[i]);
    for (JO_ZERO(i); i < n; ++i)
        ++first[frame->window[i]];
    for (JO_ZERO(i), JO_ZERO(sum); i < JO_VDP1_WINDOW_COUNT; ++i)
    {
        n = first[i];
        first[i] = sum;
        sum += n;
    }
    n = frame->count;
    for (JO_ZERO(i); i < n; ++i)
        __jo_vdp1_sort_a[first[frame->window[order[i]]]++] = order[i];
    return (__jo_vdp1_sort_a);
}

/** @brief Add a transfer to the indirect table of the SCU DMA (the transfers of consecutive sources are merged)
 *  @param table Table
 *  @param entries Entries of the table (updated)
 *  @param src Source (high work RAM)
 *  @param offset VRAM offset of the destination
 *  @param size Size in bytes
 */
static  __jo_force_inline void          jo_vdp1_dma_add(unsigned int * const table, unsigned int * const entries, const void * const src,
                                                        const unsigned int offset, const unsigned int size)
{
    unsigned int                        *last;

    if (*entries > 0)
    {
        last = &table[(*entries - 1) * 3];
        // The destinations are always consecutive
        if (last[2] + last[0] == (unsigned int)src)
        {
            last[0] += size;
            return;
        }
    }
    last = &table[*entries * 3];
    last[0] = size;
    last[1] = JO_VDP1_VRAM + offset;
    last[2] = (unsigned int)src;
    ++*entries;
}

/** @brief Parameters of __jo_vdp1_build_dma() (the offsets are used by the assembly) */
typedef struct
{
    const unsigned short                *keys;          // 0
    const unsigned short                *windows;       // 4
    const jo_vdp1_command               *commands;      // 8
    unsigned int                        *table;         // 12: indirect table
    unsigned int                        offset;         // 16: VRAM offset of the next command (input and output)
    unsigned int                        limit;          // 20: end of the table (the end command excluded)
    int                                 window;         // 24: current window (input and output, -1: none)
    const jo_vdp1_command               *headers;       // 28: headers of the windows (JO_VDP1_HEADER_COMMANDS each)
    unsigned int                        *last;          // 32: last entry of the table (input and output)
    int                                 size;           // 36: its byte count, not written yet (-1: no entry)
    unsigned int                        vram;           // 40: VDP1 VRAM address
    const unsigned short                *end;           // 44: (end of the indexes, set by the assembly)
}                                       jo_vdp1_dma_build;

/** @brief Add the commands to the indirect table, from the farthest (see jo_vdp1_send_frame(), the C loop)
 *  @param order Command indexes in the order of the sort
 *  @param end End of the indexes (at least one)
 *  @param b Parameters
 *  @remarks The byte count of the last entry stays in b->size (the transfers of consecutive sources are merged)
 */
extern void                             __jo_vdp1_build_dma(const unsigned short *order, const unsigned short *end, jo_vdp1_dma_build * const b);

/* DMA_ENTRY: transfer of r1 (source) for r2 bytes at the offset r13: merged with the last entry when the source follows it */
__asm__(
"    .macro DMA_ENTRY\n"
"    cmp/eq r1, r11\n"
"    bf 1f\n"
"    add r2, r12\n"
"    bra 2f\n"
"    add r2, r11\n"
"1:  cmp/pz r12\n"                       /* the byte count of the last entry */
"    bf 3f\n"
"    mov.l r12, @r10\n"
"3:  add #12, r10\n"
"    mov r2, r12\n"
"    mov.l @(40, r6), r0\n"
"    add r13, r0\n"
"    mov.l r0, @(4, r10)\n"
"    mov.l r1, @(8, r10)\n"
"    mov r1, r11\n"
"    add r2, r11\n"
"2:  add r2, r13\n"
"    .endm\n"
"    .text\n"
"    .align 2\n"
"    .global ___jo_vdp1_build_dma\n"
"___jo_vdp1_build_dma:\n"
"    mov.l r8, @-r15\n"
"    mov.l r9, @-r15\n"
"    mov.l r10, @-r15\n"
"    mov.l r11, @-r15\n"
"    mov.l r12, @-r15\n"
"    mov.l r13, @-r15\n"
"    mov.l r14, @-r15\n"
"    mov.l r5, @(44, r6)\n"
"    mov.l @(0, r6), r7\n"                /* keys */
"    mov.l @(4, r6), r8\n"                /* windows */
"    mov.l @(8, r6), r9\n"                /* commands */
"    mov.l @(32, r6), r10\n"              /* last entry */
"    mov.l @(36, r6), r12\n"              /* its byte count */
"    mov.l @(16, r6), r13\n"              /* offset */
"    mov.l @(24, r6), r14\n"              /* current window */
"    mov #-1, r3\n"                       /* key of the commands dropped (the table is full), -1: none */
"    mov #-1, r11\n"                      /* end of the source of the last entry (-1: none, it isn't merged) */
"    cmp/pz r12\n"
"    bf 5f\n"
"    mov.l @(8, r10), r11\n"
"    add r12, r11\n"
"5:  mov.w @r4+, r0\n"                   /* command index */
"    extu.w r0, r0\n"
"    cmp/pz r3\n"                         /* (the key is read only after a drop: a random read less a command) */
"    bf.s 4f\n"
"    add r0, r0\n"
"    mov.w @(r0, r7), r1\n"
"    extu.w r1, r1\n"
"    cmp/eq r1, r3\n"                     /* the other commands of the depth dropped */
"    bt 9f\n"
"    mov #-1, r3\n"
"4:  mov.w @(r0, r8), r2\n"               /* window */
"    extu.w r2, r2\n"
"    cmp/eq r2, r14\n"
"    bt 6f\n"
     /* the window changes: its header (room for the header and one command) */
"    mov r13, r5\n"
"    add #127, r5\n"
"    add #1, r5\n"
"    mov.l @(20, r6), r3\n"
"    cmp/hi r3, r5\n"
"    bf.s 7f\n"
"    mov #-1, r3\n"
"    mov.w @(r0, r7), r3\n"              /* (dropped: its key) */
"    bra 9f\n"
"    extu.w r3, r3\n"
"7:  mov r2, r14\n"
"    mov.l r14, @(24, r6)\n"
"    mov r0, r5\n"                        /* (index x 2 saved) */
"    mov r2, r1\n"                        /* header: headers + window x 96 */
"    shll2 r1\n"
"    shll2 r1\n"
"    shll r1\n"
"    mov r1, r2\n"
"    add r1, r1\n"
"    add r2, r1\n"
"    mov.l @(28, r6), r2\n"
"    add r2, r1\n"
"    mov #96, r2\n"
"    DMA_ENTRY\n"
"    mov r5, r0\n"
     /* the command (room for it) */
"6:  mov r13, r5\n"
"    add #32, r5\n"
"    mov.l @(20, r6), r2\n"
"    cmp/hi r2, r5\n"
"    bf 8f\n"
"    mov.w @(r0, r7), r3\n"              /* (dropped: its key) */
"    bra 9f\n"
"    extu.w r3, r3\n"
"8:  shll2 r0\n"                         /* commands + index x 32 */
"    shll2 r0\n"
"    mov r0, r1\n"
"    add r9, r1\n"
"    mov #32, r2\n"
"    DMA_ENTRY\n"
"9:  mov.l @(44, r6), r2\n"
"    cmp/hs r2, r4\n"
"    bf 5b\n"
"    mov.l r13, @(16, r6)\n"
"    mov.l r10, @(32, r6)\n"
"    mov.l r12, @(36, r6)\n"
"    mov.l @r15+, r14\n"
"    mov.l @r15+, r13\n"
"    mov.l @r15+, r12\n"
"    mov.l @r15+, r11\n"
"    mov.l @r15+, r10\n"
"    mov.l @r15+, r9\n"
"    rts\n"
"    mov.l @r15+, r8\n"
);

/** @brief Check if the SCU DMA copies a command table into the VRAM
 *  @return true until the end of the copy
 */
static  __jo_force_inline bool          jo_vdp1_dma_busy(void)
{
    return ((JO_SCU_DMA_STATUS & JO_SCU_DMA_STATUS_BUSY(JO_VDP1_SCU_DMA_LEVEL)) != 0);
}

/** @brief Sort the commands of a frame and copy them into the VDP1 VRAM: the table is displayed at the next VBLANK after the copy
 *  @param argument Frame (jo_vdp1_frame)
 *  @remarks Job of the slave CPU (see __jo_vdp1_end_frame()), or called by the master CPU
 *  @remarks The copy is done by the SCU DMA (indirect mode: one transfer per run of commands in the order of the sort): it
 *           is about 5 times faster than a copy by the CPU and the CPU doesn't wait for its end
 */
static void                             jo_vdp1_send_frame(void *argument)
{
    const jo_vdp1_frame                 *frame;
    unsigned int                        offset;
    unsigned int                        table;
    unsigned int                        n;
    unsigned int                        field;
    unsigned int                        limit;
    unsigned int                        entries;
    unsigned int                        *dma;
    register unsigned int               i;
    register unsigned int               j;
    int                                 window;
    int                                 current_window;
    unsigned short                      key;
    unsigned short                      skip_key;
    bool                                skip;
    unsigned short                      *order;

    frame = (const jo_vdp1_frame *)argument;
    if (frame->wireframe)
        jo_vdp1_make_wireframe((jo_vdp1_frame *)argument);
    // The sort only uses the work RAM: it is done while the VDP1 is still drawing
    jo_profile_begin(JO_PROFILE_VDP1_SORT);
    n = frame->count;
    order = jo_vdp1_radix_sort(frame);
    if (frame->windows_apart)
        order = jo_vdp1_group_windows(frame, order);
    jo_profile_end(JO_PROFILE_VDP1_SORT);
    jo_profile_count(JO_PROFILE_VDP1_COMMANDS, n);
    jo_profile_begin(JO_PROFILE_VDP1_WAIT);
    // The previous table must be given to the VDP1 (at a VBLANK) before this one (and the SCU DMA must have copied it:
    // the indirect table and the headers are used again)
    while (JO_VDP1_SHARED(__jo_vdp1_pending_table) || jo_vdp1_dma_busy())
        ;
    // 1-cycle mode: the table we are about to fill may still be read by the VDP1 if the jump to the
    // other table was changed after the VDP1 started this field: wait for the end of
    // the current drawing (or for the next field).
    // Manual change mode: the jump changes only when the VDP1 doesn't draw (see __jo_vdp1_vblank()), the VDP1 never
    // reads this table again
    if (!JO_VDP1_MANUAL_CHANGE)
    {
        field = JO_VDP1_SHARED(__jo_vdp1_flip_field);
        while ((JO_VDP1_EDSR & JO_VDP1_EDSR_CURRENT_END) == 0 && JO_VDP1_SHARED(__jo_vdp1_vblank_fields) == field)
            ;
    }
    jo_profile_end(JO_PROFILE_VDP1_WAIT);
    jo_profile_begin(JO_PROFILE_VDP1_COPY);
    table = JO_VDP1_SHARED(__jo_vdp1_next_table);
    offset = table;
    dma = __jo_vdp1_dma_table;
    JO_ZERO(entries);
    // Keep room for the end command
    limit = ((table == JO_VDP1_TABLE_A_OFFSET) ? JO_VDP1_TABLE_B_OFFSET : JO_VDP1_TEXTURE_DEF_BASE_ADDRESS) - sizeof(jo_vdp1_command);
    // Headers of the windows and end command
    for (window = 0; window < JO_VDP1_WINDOW_COUNT; ++window)
        jo_vdp1_make_header(&__jo_vdp1_dma_commands[window * JO_VDP1_HEADER_COMMANDS], frame, window);
    jo_memset(&__jo_vdp1_dma_commands[JO_VDP1_DMA_END_COMMAND], 0, sizeof(jo_vdp1_command));
    __jo_vdp1_dma_commands[JO_VDP1_DMA_END_COMMAND].ctrl = JO_VDP1_CMD_END;
    current_window = -1;
    skip = false;
    JO_ZERO(skip_key);
    if (JO_VDP1_MANUAL_CHANGE)
    {
        // The frame buffer is not erased by the VDP1 (manual change mode): a polygon of the transparent color clears it
        jo_vdp1_make_clear(&__jo_vdp1_dma_commands[JO_VDP1_DMA_CLEAR_COMMAND], frame);
        jo_vdp1_dma_add(dma, &entries, &__jo_vdp1_dma_commands[0], offset, JO_VDP1_HEADER_COMMANDS * sizeof(jo_vdp1_command));
        offset += JO_VDP1_HEADER_COMMANDS * sizeof(jo_vdp1_command);
        current_window = 0;
        jo_vdp1_dma_add(dma, &entries, &__jo_vdp1_dma_commands[JO_VDP1_DMA_CLEAR_COMMAND], offset, sizeof(jo_vdp1_command));
        offset += sizeof(jo_vdp1_command);
    }
    // Far to near. Commands with the same depth are drawn in submission order
#ifndef JO_VDP1_SORT_IN_C
    if (n > 0)
    {
        jo_vdp1_dma_build                build;

        build.keys = frame->keys;
        build.windows = frame->window;
        build.commands = frame->commands;
        build.table = dma;
        build.offset = offset;
        build.limit = limit;
        build.window = current_window;
        build.headers = __jo_vdp1_dma_commands;
        build.vram = JO_VDP1_VRAM;
        // The last entry (merged with the next transfer if the sources follow each other)
        if (entries > 0)
        {
            build.last = &dma[(entries - 1) * 3];
            build.size = (int)build.last[0];
        }
        else
        {
            // No entry yet: the "last entry" is the place before the table (the builder goes 12 bytes forward before it
            // writes an entry, and it doesn't read this one: its byte count is -1). Computed as an address: a pointer
            // before an array is not valid C, the compiler warned about it
            build.last = (unsigned int *)((unsigned int)dma - 3 * sizeof(unsigned int));
            build.size = -1;
        }
        __jo_vdp1_build_dma(order, order + n, &build);
        if (build.size >= 0)
        {
            build.last[0] = (unsigned int)build.size;
            entries = (unsigned int)(build.last - dma) / 3 + 1;
        }
        offset = build.offset;
        current_window = build.window;
        n = 0;
    }
#endif
    for (JO_ZERO(j); j < n; ++j)
    {
        i = order[j];
        key = frame->keys[i];
        // The table is full: the other commands with the same depth are dropped
        if (skip && key == skip_key)
            continue;
        skip = false;
        window = frame->window[i];
        if (window != current_window)
        {
            // The window changes many times when the commands of the 2 windows are mixed by depth
            if (offset + (JO_VDP1_HEADER_COMMANDS + 1) * sizeof(jo_vdp1_command) > limit)
            {
                skip = true;
                skip_key = key;
                continue;
            }
            jo_vdp1_dma_add(dma, &entries, &__jo_vdp1_dma_commands[window * JO_VDP1_HEADER_COMMANDS], offset,
                            JO_VDP1_HEADER_COMMANDS * sizeof(jo_vdp1_command));
            offset += JO_VDP1_HEADER_COMMANDS * sizeof(jo_vdp1_command);
            current_window = window;
        }
        if (offset + sizeof(jo_vdp1_command) > limit)
        {
            skip = true;
            skip_key = key;
            continue;
        }
        jo_vdp1_dma_add(dma, &entries, &frame->commands[i], offset, sizeof(jo_vdp1_command));
        offset += sizeof(jo_vdp1_command);
    }
    if (current_window < 0)
    {
        jo_vdp1_dma_add(dma, &entries, &__jo_vdp1_dma_commands[0], offset, JO_VDP1_HEADER_COMMANDS * sizeof(jo_vdp1_command));
        offset += JO_VDP1_HEADER_COMMANDS * sizeof(jo_vdp1_command);
    }
    jo_vdp1_dma_add(dma, &entries, &__jo_vdp1_dma_commands[JO_VDP1_DMA_END_COMMAND], offset, sizeof(jo_vdp1_command));
    dma[(entries - 1) * 3 + 2] |= JO_SCU_DMA_INDIRECT_END;
    // The SCU DMA reads the work RAM (the cache of the CPU writes through: the memory is up to date; the commands of
    // the slave CPU copied by the DMA controller of this CPU during the sort, see jo_3d_slave_copy_job(): finished)
    jo_dma_wait();
    JO_SCU_DMA_ENABLE(JO_VDP1_SCU_DMA_LEVEL) = 0;
    JO_SCU_DMA_WRITE(JO_VDP1_SCU_DMA_LEVEL) = (unsigned int)dma;
    JO_SCU_DMA_ADD(JO_VDP1_SCU_DMA_LEVEL) = JO_SCU_DMA_ADD_READ4_WRITE2;
    JO_SCU_DMA_MODE(JO_VDP1_SCU_DMA_LEVEL) = JO_SCU_DMA_MODE_INDIRECT;
    JO_SCU_DMA_ENABLE(JO_VDP1_SCU_DMA_LEVEL) = JO_SCU_DMA_ENABLE_START;
    JO_VDP1_SHARED(__jo_vdp1_next_table) = (table == JO_VDP1_TABLE_A_OFFSET) ? JO_VDP1_TABLE_B_OFFSET : JO_VDP1_TABLE_A_OFFSET;
    jo_profile_end(JO_PROFILE_VDP1_COPY);
    // Displayed at the first VBLANK after the end of the copy (see __jo_vdp1_vblank())
    JO_VDP1_SHARED(__jo_vdp1_pending_tag) = frame->tag;
    JO_VDP1_SHARED(__jo_vdp1_pending_table) = table;
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
    // The SCU DMA reads the buffer of the frame until the end of the copy (see __jo_vdp1_end_frame())
    JO_VDP1_SHARED(__jo_vdp1_frames_done) = JO_VDP1_SHARED(__jo_vdp1_frames_done) + 1;
#endif
}

void                                    __jo_vdp1_end_frame(void)
{
    jo_vdp1_frame                       *frame;
    int                                 i;

    frame = __jo_vdp1_frame;
    frame->tag = __jo_vdp1_building_tag++;
    // A sprite may still be copied in VRAM by the DMA controller (see __jo_sprite_replace())
    jo_dma_wait();
    // The windows and the system clipping of the frame: the next frame can change them
    for (i = 0; i < JO_VDP1_WINDOW_COUNT; ++i)
        frame->windows[i] = __jo_vdp1_windows[i];
    frame->system_clip_right = __jo_vdp1_system_clip_right;
    frame->system_clip_bottom = __jo_vdp1_system_clip_bottom;
    frame->wireframe = __jo_vdp1_wireframe;
    frame->windows_apart = __jo_vdp1_windows_apart;
    __jo_vdp1_last_count = frame->count;
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
    if (__jo_core_is_slave_running())
    {
        // The slave CPU sorts and copies the commands while the master CPU computes the next frame in the other buffer
        ++__jo_vdp1_frames_sent;
        __jo_core_add_slave_job(jo_vdp1_send_frame, frame);
        frame = (frame == &__jo_vdp1_frames[0]) ? &__jo_vdp1_frames[1] : &__jo_vdp1_frames[0];
        // The previous frame of this buffer must be in the VRAM (the slave CPU is usually done since a long time)
        while (__jo_vdp1_frames_sent - JO_VDP1_SHARED(__jo_vdp1_frames_done) > 1)
            ;
        // The SCU DMA may still read the buffer (or already copy the frame just sent: the wait is short)
        while (jo_vdp1_dma_busy())
            ;
        JO_ZERO(frame->count);
        __jo_vdp1_frame = frame;
        return;
    }
#endif
    jo_vdp1_send_frame(frame);
    // The SCU DMA reads the buffer of the frame
    while (jo_vdp1_dma_busy())
        ;
    JO_ZERO(frame->count);
}

void                                    jo_vdp1_redraw_last_frame(void)
{
    // (manual change: the VDP1 draws only when the frame buffers change; 1 cycle mode: at each field already)
    if (JO_VDP1_MANUAL_CHANGE && (JO_VDP1_EDSR & JO_VDP1_EDSR_CURRENT_END) != 0)
        JO_VDP1_FBCR = JO_VDP1_FBCR_MANUAL_CHANGE;
}

void                                    __jo_vdp1_vblank(void)
{
    static bool                         drawing = false;
    unsigned int                        table;

    JO_VDP1_SHARED(__jo_vdp1_vblank_fields) = JO_VDP1_SHARED(__jo_vdp1_vblank_fields) + 1;
    // 1 cycle mode: the table given at the last VBLANK is displayed from this field
    if (JO_VDP1_SHARED(__jo_vdp1_next_shown_field) == JO_VDP1_SHARED(__jo_vdp1_vblank_fields))
    {
        JO_VDP1_SHARED(__jo_vdp1_shown_tag) = JO_VDP1_SHARED(__jo_vdp1_next_shown_tag);
        JO_VDP1_SHARED(__jo_vdp1_shown_field) = JO_VDP1_SHARED(__jo_vdp1_vblank_fields);
    }
    table = JO_VDP1_SHARED(__jo_vdp1_pending_table);
    // The SCU DMA still copies the table
    if (!table || jo_vdp1_dma_busy())
        return;
    if (JO_VDP1_MANUAL_CHANGE)
    {
        // Manual change: the new table is drawn when the VDP1 finished the current one (else the next VBLANK)
        if (drawing && (JO_VDP1_EDSR & JO_VDP1_EDSR_CURRENT_END) == 0)
            return;
        jo_vdp1_write_jump(table);
        // The frame buffers change at the end of this VBLANK, then the VDP1 draws the new table: the image of the
        // previous table is displayed from this field
        JO_VDP1_FBCR = JO_VDP1_FBCR_MANUAL_CHANGE;
        if (drawing)
        {
            JO_VDP1_SHARED(__jo_vdp1_shown_tag) = JO_VDP1_SHARED(__jo_vdp1_taken_tag);
            JO_VDP1_SHARED(__jo_vdp1_shown_field) = JO_VDP1_SHARED(__jo_vdp1_vblank_fields);
        }
        drawing = true;
    }
    else
    {
        jo_vdp1_write_jump(table);
        // The buffers change at each VBLANK: the table is displayed from the next field
        JO_VDP1_SHARED(__jo_vdp1_next_shown_tag) = JO_VDP1_SHARED(__jo_vdp1_pending_tag);
        JO_VDP1_SHARED(__jo_vdp1_next_shown_field) = JO_VDP1_SHARED(__jo_vdp1_vblank_fields) + 1;
    }
    JO_VDP1_SHARED(__jo_vdp1_taken_tag) = JO_VDP1_SHARED(__jo_vdp1_pending_tag);
#ifdef JO_COMPILE_WITH_PROFILER_SUPPORT
    __jo_profile_vdp1_start();
#endif
    JO_VDP1_SHARED(__jo_vdp1_flip_field) = JO_VDP1_SHARED(__jo_vdp1_vblank_fields);
    JO_VDP1_SHARED(__jo_vdp1_pending_table) = 0;
}

unsigned int                            __jo_vdp1_frame_tag(void)
{
    return (__jo_vdp1_building_tag);
}

bool                                    __jo_vdp1_displayed_frame_changed(unsigned int * const tag)
{
    if (JO_VDP1_SHARED(__jo_vdp1_shown_field) != JO_VDP1_SHARED(__jo_vdp1_vblank_fields))
        return (false);
    *tag = JO_VDP1_SHARED(__jo_vdp1_shown_tag);
    return (true);
}

/** @brief A command as 8 words of 32 bits (ctrl | link, pmod | colr, srca | size, xa | ya, xb | yb, xc | yc, xd | yd,
 *  grda | reserved): the command table is in work RAM, the cache writes through, a 32 bits write costs a 16 bits write */
typedef unsigned int __attribute__((may_alias))  jo_vdp1_command_word;

# define JO_VDP1_COMMAND_WORDS(CMD)         ((jo_vdp1_command_word *)(CMD))
/** @brief Two 16 bits fields in a word (the high one first: big endian) */
# define JO_VDP1_PACK(HIGH, LOW)            (((unsigned int)(unsigned short)(HIGH) << 16) | (unsigned int)(unsigned short)(LOW))

/** @brief Write the attribute of a sprite in a command (see __jo_vdp1_set_attribute()), its link 0
 *  @param cmd Command
 *  @param attr Attribute
 *  @param sprite_type Added to the type of the command if it is textured (JO_VDP1_CMD_xxx_SPRITE)
 *  @param polygon_type Added to the type of the command if it is not
 *  @return true if textured
 *  @remarks The type is known before the command is written: it is not read back (the cache does not keep the lines
 *  written, a read of the command after it is written reads the work RAM)
 */
static  __jo_force_inline bool          jo_vdp1_write_attribute(jo_vdp1_command * const cmd, const jo_vdp1_sprite_attribute * const attr,
                                                                const unsigned short sprite_type, const unsigned short polygon_type)
{
    jo_vdp1_command_word                *words;
    jo_texture_definition               *texture;
    unsigned int                        ctrl;
    bool                                textured;

    words = JO_VDP1_COMMAND_WORDS(cmd);
    textured = false;
    switch (attr->direction & JO_VDP1_DIRECTION_FUNCTION_MASK)
    {
        case JO_VDP1_FUNCTION_POLYGON:
            ctrl = JO_VDP1_CMD_POLYGON | polygon_type;
            break;
        case JO_VDP1_FUNCTION_POLYLINE:
            ctrl = JO_VDP1_CMD_POLYLINE | polygon_type;
            break;
        case JO_VDP1_FUNCTION_LINE:
            ctrl = JO_VDP1_CMD_LINE | polygon_type;
            break;
        default:
            texture = &__jo_sprite_def[attr->texture_id];
            ctrl = (attr->direction & JO_VDP1_DIRECTION_FLIP_MASK) | sprite_type;
            words[2] = JO_VDP1_PACK(texture->adr, texture->size);
            textured = true;
            break;
    }
    words[0] = JO_VDP1_PACK(ctrl, 0);
    words[1] = JO_VDP1_PACK(attr->draw_mode, attr->color);
    words[7] = JO_VDP1_PACK(((attr->draw_mode & JO_VDP1_COLOR_CALC_MASK) == JO_VDP1_COLOR_CALC_GOURAUD) ? attr->gouraud_table : 0, 0);
    return (textured);
}

bool                                    __jo_vdp1_set_attribute(jo_vdp1_command *cmd, const jo_vdp1_sprite_attribute *attr)
{
    return (jo_vdp1_write_attribute(cmd, attr, 0, 0));
}

/*
** Sprites
*/

/** @brief Check if a sprite is between the near clipping plane and the far limit of the current window
 *  @param z Depth
 *  @return true if visible
 */
static  __jo_force_inline bool          jo_vdp1_is_sprite_visible(const jo_fixed z)
{
    return (z > __jo_vdp1_near_clip() && z <= __jo_vdp1_windows[__jo_vdp1_current_window].zlimit);
}

/** @brief Horizontal center of the current window
 *  @return Q16.16 coordinate
 */
static  __jo_force_inline jo_fixed      jo_vdp1_window_center_x(void)
{
    return (JO_MULT_BY_65536(__jo_vdp1_windows[__jo_vdp1_current_window].center_x - __jo_vdp1_windows[__jo_vdp1_current_window].origin_x));
}

/** @brief Vertical center of the current window
 *  @return Q16.16 coordinate
 */
static  __jo_force_inline jo_fixed      jo_vdp1_window_center_y(void)
{
    return (JO_MULT_BY_65536(__jo_vdp1_windows[__jo_vdp1_current_window].center_y - __jo_vdp1_windows[__jo_vdp1_current_window].origin_y));
}

/** @brief Perspective scale at a depth
 *  @param z Depth
 *  @return projection distance / z
 */
static  __jo_force_inline jo_fixed      jo_vdp1_perspective_scale(const jo_fixed z)
{
    if (z <= 0)
        return (0);
    return (jo_fixed_div(__jo_vdp1_projection_distance, z));
}

/** @brief Draw a sprite centered on (cx, cy)
 *  @param cx Horizontal center (screen pixels, Q16.16)
 *  @param cy Vertical center (screen pixels, Q16.16)
 *  @param z Depth
 *  @param sx Horizontal scale
 *  @param sy Vertical scale
 *  @param angle Rotation
 *  @param attr Sprite attribute
 *  @return false if the command table is full
 *  @remarks The sprite covers (width * scale) x (height * scale) pixels (VDP1 coordinates are inclusive)
 *  @remarks A sprite without scale and rotation is drawn with a normal sprite command (faster and pixel perfect)
 */
static bool                             jo_vdp1_draw_sprite(const jo_fixed cx, const jo_fixed cy, const jo_fixed z, jo_fixed sx, jo_fixed sy,
                                                            const jo_angle angle, const jo_vdp1_sprite_attribute *attr)
{
    jo_vdp1_command                     *cmd;
    jo_vdp1_command_word                *words;
    jo_texture_definition               *texture;
    int                                 w;
    int                                 h;
    int                                 x0;
    int                                 y0;
    bool                                textured;
    jo_fixed                            s;
    jo_fixed                            c;
    jo_fixed                            lx0;
    jo_fixed                            ly0;
    jo_fixed                            lx1;
    jo_fixed                            ly1;
    jo_fixed                            ox;
    jo_fixed                            oy;

    if ((cmd = __jo_vdp1_add_command(z)) == JO_NULL)
        return (false);
    words = JO_VDP1_COMMAND_WORDS(cmd);
    texture = &__jo_sprite_def[attr->texture_id];
    if (sx < 0)
        sx = -sx;
    if (sy < 0)
        sy = -sy;
    // Size in pixels (rounded to the nearest pixel)
    w = JO_FIXED_TO_INT(jo_fixed_mult(JO_INT_TO_FIXED(texture->width), sx) + JO_FIXED_1_DIV_2);
    h = JO_FIXED_TO_INT(jo_fixed_mult(JO_INT_TO_FIXED(texture->height), sy) + JO_FIXED_1_DIV_2);
    x0 = JO_FIXED_TO_INT(cx) - JO_DIV_BY_2(w);
    y0 = JO_FIXED_TO_INT(cy) - JO_DIV_BY_2(h);
    // Last pixel (inclusive coordinates)
    if (w > 0)
        --w;
    if (h > 0)
        --h;
    if (angle == 0)
    {
        if (sx == JO_FIXED_1 && sy == JO_FIXED_1)
            textured = jo_vdp1_write_attribute(cmd, attr, JO_VDP1_CMD_NORMAL_SPRITE, JO_VDP1_CMD_POLYGON);
        else
            textured = jo_vdp1_write_attribute(cmd, attr, JO_VDP1_CMD_SCALED_SPRITE, JO_VDP1_CMD_POLYGON);
        if (textured && sx == JO_FIXED_1 && sy == JO_FIXED_1)
            words[3] = JO_VDP1_PACK(x0, y0);
        else if (textured)
        {
            words[3] = JO_VDP1_PACK(x0, y0);
            words[5] = JO_VDP1_PACK(x0 + w, y0 + h);
        }
        else
        {
            words[3] = JO_VDP1_PACK(x0, y0);
            words[4] = JO_VDP1_PACK(x0 + w, y0);
            words[5] = JO_VDP1_PACK(x0 + w, y0 + h);
            words[6] = JO_VDP1_PACK(x0, y0 + h);
        }
        return (true);
    }
    jo_vdp1_write_attribute(cmd, attr, JO_VDP1_CMD_DISTORTED_SPRITE, 0);
    s = jo_sin_angle(angle);
    c = jo_cos_angle(angle);
    // Rotation around the integer part of the center
    ox = cx & ~JO_FIXED_DECIMAL_MASK;
    oy = cy & ~JO_FIXED_DECIMAL_MASK;
    lx0 = JO_INT_TO_FIXED(-JO_DIV_BY_2(w));
    ly0 = JO_INT_TO_FIXED(-JO_DIV_BY_2(h));
    lx1 = lx0 + JO_INT_TO_FIXED(w);
    ly1 = ly0 + JO_INT_TO_FIXED(h);
    words[3] = JO_VDP1_PACK(JO_FIXED_TO_INT(ox + jo_fixed_mult(lx0, c) - jo_fixed_mult(ly0, s)),
                            JO_FIXED_TO_INT(oy + jo_fixed_mult(lx0, s) + jo_fixed_mult(ly0, c)));
    words[4] = JO_VDP1_PACK(JO_FIXED_TO_INT(ox + jo_fixed_mult(lx1, c) - jo_fixed_mult(ly0, s)),
                            JO_FIXED_TO_INT(oy + jo_fixed_mult(lx1, s) + jo_fixed_mult(ly0, c)));
    words[5] = JO_VDP1_PACK(JO_FIXED_TO_INT(ox + jo_fixed_mult(lx1, c) - jo_fixed_mult(ly1, s)),
                            JO_FIXED_TO_INT(oy + jo_fixed_mult(lx1, s) + jo_fixed_mult(ly1, c)));
    words[6] = JO_VDP1_PACK(JO_FIXED_TO_INT(ox + jo_fixed_mult(lx0, c) - jo_fixed_mult(ly1, s)),
                            JO_FIXED_TO_INT(oy + jo_fixed_mult(lx0, s) + jo_fixed_mult(ly1, c)));
    return (true);
}

bool                                    __jo_vdp1_draw_sprite(const jo_vdp1_sprite_position * const pos, const jo_vdp1_sprite_attribute * const attr, const jo_angle angle)
{
    jo_fixed                            scale;

    if (!jo_vdp1_is_sprite_visible(pos->z))
        return (false);
    scale = pos->scale_x;
    if (scale < 0)
        scale = jo_fixed_mult(-scale, jo_vdp1_perspective_scale(pos->z));
    return (jo_vdp1_draw_sprite(jo_vdp1_window_center_x() + pos->x, jo_vdp1_window_center_y() + pos->y, pos->z, scale, scale, angle, attr));
}

bool                                    __jo_vdp1_draw_sprite_hv(const jo_vdp1_sprite_position * const pos, const jo_vdp1_sprite_attribute * const attr, const jo_angle angle)
{
    jo_fixed                            sx;
    jo_fixed                            sy;
    jo_fixed                            ps;

    if (!jo_vdp1_is_sprite_visible(pos->z))
        return (false);
    sx = pos->scale_x;
    sy = pos->scale_y;
    // (the perspective scale: one division for both)
    if ((sx | sy) < 0)
    {
        ps = jo_vdp1_perspective_scale(pos->z);
        if (sx < 0)
            sx = jo_fixed_mult(-sx, ps);
        if (sy < 0)
            sy = jo_fixed_mult(-sy, ps);
    }
    return (jo_vdp1_draw_sprite(jo_vdp1_window_center_x() + pos->x, jo_vdp1_window_center_y() + pos->y, pos->z, sx, sy, angle, attr));
}

bool                                    __jo_vdp1_draw_sprite_4p(const jo_pos2D_fixed * const points, const jo_fixed z, const jo_vdp1_sprite_attribute * const attr)
{
    jo_vdp1_command                     *cmd;
    jo_vdp1_command_word                *words;
    jo_fixed                            cx;
    jo_fixed                            cy;

    if (!jo_vdp1_is_sprite_visible(z))
        return (false);
    if ((cmd = __jo_vdp1_add_command(z)) == JO_NULL)
        return (false);
    jo_vdp1_write_attribute(cmd, attr, JO_VDP1_CMD_DISTORTED_SPRITE, 0);
    words = JO_VDP1_COMMAND_WORDS(cmd);
    cx = jo_vdp1_window_center_x();
    cy = jo_vdp1_window_center_y();
    words[3] = JO_VDP1_PACK(JO_FIXED_TO_INT(cx + points[0].x), JO_FIXED_TO_INT(cy + points[0].y));
    words[4] = JO_VDP1_PACK(JO_FIXED_TO_INT(cx + points[1].x), JO_FIXED_TO_INT(cy + points[1].y));
    words[5] = JO_VDP1_PACK(JO_FIXED_TO_INT(cx + points[2].x), JO_FIXED_TO_INT(cy + points[2].y));
    words[6] = JO_VDP1_PACK(JO_FIXED_TO_INT(cx + points[3].x), JO_FIXED_TO_INT(cy + points[3].y));
    return (true);
}

bool                                    __jo_vdp1_write_sprite_4p(jo_vdp1_command * const cmd, const jo_pos2D_fixed * const points, const jo_fixed z,
                                                                  const jo_vdp1_sprite_attribute * const attr, const int window)
{
    const jo_vdp1_window                *w;
    jo_fixed                            cx;
    jo_fixed                            cy;

    w = &__jo_vdp1_windows[window];
    if (z <= __jo_vdp1_near_clip() || z > w->zlimit)
        return (false);
    // (the same command as __jo_vdp1_draw_sprite_4p())
    cmd->link = 0;
    cmd->grda = 0;
    if (__jo_vdp1_set_attribute(cmd, attr))
        cmd->ctrl |= JO_VDP1_CMD_DISTORTED_SPRITE;
    cx = JO_MULT_BY_65536(w->center_x - w->origin_x);
    cy = JO_MULT_BY_65536(w->center_y - w->origin_y);
    cmd->xa = JO_FIXED_TO_INT(cx + points[0].x);
    cmd->ya = JO_FIXED_TO_INT(cy + points[0].y);
    cmd->xb = JO_FIXED_TO_INT(cx + points[1].x);
    cmd->yb = JO_FIXED_TO_INT(cy + points[1].y);
    cmd->xc = JO_FIXED_TO_INT(cx + points[2].x);
    cmd->yc = JO_FIXED_TO_INT(cy + points[2].y);
    cmd->xd = JO_FIXED_TO_INT(cx + points[3].x);
    cmd->yd = JO_FIXED_TO_INT(cy + points[3].y);
    return (true);
}

bool                                    __jo_vdp1_draw_sprite_3d(const jo_vdp1_sprite_position * const pos, const jo_vdp1_sprite_attribute * const attr, const jo_angle angle)
{
    jo_pos3D_fixed                      view;
    jo_fixed                            scale;
    jo_fixed                            ps;

#ifdef JO_COMPILE_WITH_3D_SUPPORT
    jo_3d_transform_point((const jo_pos3D_fixed *)pos, &view);
#else
    view.x = pos->x;
    view.y = pos->y;
    view.z = pos->z;
#endif
    if (!jo_vdp1_is_sprite_visible(view.z))
        return (false);
    ps = jo_vdp1_perspective_scale(view.z);
    scale = pos->scale_x;
    if (scale < 0)
        scale = -scale;
    scale = jo_fixed_mult(scale, ps);
    return (jo_vdp1_draw_sprite(jo_vdp1_window_center_x() + jo_fixed_mult(view.x, ps),
                                jo_vdp1_window_center_y() + jo_fixed_mult(view.y, ps),
                                view.z, scale, scale, angle, attr));
}

void                                    jo_sprite_set_system_clipping_area(const unsigned int width, const unsigned int height)
{
#ifdef JO_DEBUG
    if (width == 0 || height == 0)
    {
        jo_core_error("Invalid clipping area size (%dx%d)", width, height);
        return;
    }
#endif
    __jo_vdp1_system_clip_right = width - 1;
    __jo_vdp1_system_clip_bottom = height - 1;
}

void                                    __jo_vdp1_set_window(const short left, const short top, const short right, const short bottom,
                                                             const short zlimit, const short center_x, const short center_y)
{
    jo_vdp1_window                      *w;

    w = &__jo_vdp1_windows[__jo_vdp1_current_window];
    w->left = left;
    w->top = top;
    w->right = right;
    w->bottom = bottom;
    w->zlimit = JO_INT_TO_FIXED((jo_fixed)zlimit);
    w->center_x = center_x;
    w->center_y = center_y;
    // (a clipped window of windows apart: its own screen, see jo_3d_window_clip())
    w->screen = w->clip_mode != 0 && __jo_vdp1_windows_apart;
    w->origin_x = w->screen ? left : 0;
    w->origin_y = w->screen ? top : 0;
}

void                                    __jo_vdp1_set_window_clip(const bool clip)
{
    jo_vdp1_window                      *w;

    w = &__jo_vdp1_windows[__jo_vdp1_current_window];
    w->clip_mode = clip ? JO_VDP1_WINDOW_IN : 0;
    w->screen = clip && __jo_vdp1_windows_apart;
    w->origin_x = w->screen ? w->left : 0;
    w->origin_y = w->screen ? w->top : 0;
}

/*
** END OF FILE
*/
