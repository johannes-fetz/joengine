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
#include "internal/core.h"
#include "internal/vdp1_command_pipeline.h"
#include "internal/profile.h"

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
# define JO_VDP1_HIRES                      (__jo_screen.hires)
/** @brief Double density interlace (448 to 512 lines) */
# define JO_VDP1_INTERLACE                  (__jo_screen.interlace)
/** @brief Frame buffer lines */
# define JO_VDP1_LINES                      (__jo_screen.background_height)
/** @brief TVMR: 8 bits per pixel, 1024x256 frame buffer (hi-res modes) */
# define JO_VDP1_TVMR_HIRES                 (0x0001)
/** @brief FBCR: double density interlace (DIE | DIL) */
# define JO_VDP1_FBCR_DOUBLE_INTERLACE      (0x000C)
/** @brief FBCR: double density interlace, the even lines are drawn (DIE) */
# define JO_VDP1_FBCR_DOUBLE_INTERLACE_EVEN (0x0008)
/** @brief FBCR: manual frame buffer change mode (FCM) and change at the next VBLANK (FCT) */
# define JO_VDP1_FBCR_MANUAL                (0x0002)
# define JO_VDP1_FBCR_MANUAL_CHANGE         (0x0003)
/** @brief The frame buffers change only when the VDP1 finished drawing a new frame (JO_FRAMERATE > 1, or the dynamic
 *  frame rate JO_FRAMERATE_DYNAMIC): in the 1-cycle mode, they change at each VBLANK and the VDP1 must draw a frame in
 *  1/60 s (the end of a frame that takes longer is not drawn: on the real hardware and on the accurate emulators, the last
 *  commands like the HUD disappear). The frame buffer is cleared by a polygon at the start of each command table */
#ifdef JO_FRAMERATE_DYNAMIC
# define JO_VDP1_MANUAL_CHANGE              (!JO_VDP1_INTERLACE)
/** @brief The pace of the images (manual change, see __jo_vdp1_vblank()): not with the dynamic frame rate, the tables are
 *  taken as they come */
# define JO_VDP1_PACED                      (0)
#else
# define JO_VDP1_MANUAL_CHANGE              (JO_FRAMERATE > 1 && !JO_VDP1_INTERLACE)
/** @brief The pace of the images (manual change, JO_FRAMERATE > 1): the table of a frame is sent in the last field of
 *  the frame at the earliest (see __jo_vdp1_end_frame(): the master CPU waits for the end of its frame anyway), and the
 *  VDP1 takes it at the VBLANK that starts the next frame: each image stays JO_FRAMERATE fields, the VDP1 has the whole
 *  frame to draw it, the slave CPU never waits to copy the next one. Before: a table sent before the second VBLANK of
 *  its frame (the CPUs under 16.7 ms) was taken at that VBLANK, the next one sent after it (17 ms) at the VBLANK after:
 *  the image before stayed 3 fields, the one after 1 field (a stutter at each crossing of 16.7 ms: 37 in a 1200 frames
 *  run of the first person shooter with the VDP1 at 9 ms, 0 after). When the VDP1 is over a frame, its table is taken at
 *  the first VBLANK after (one image of 3 fields), the images go on from there, and come back to the VBLANKs that start
 *  the frames as soon as the VDP1 draws a frame in a field (one image of 1 field) */
# define JO_VDP1_PACED                      (1)
#endif
/** @brief PMOD of the clear polygon: replace, no end code and no transparent pixel check, no pre-clipping (it is the
 *  system clipping area: see JO_VDP1_PRE_CLIPPING_DISABLE) */
# define JO_VDP1_CLEAR_POLYGON_MODE         (0x00C0 | JO_VDP1_PRE_CLIPPING_DISABLE)
/** @brief PTMR: the drawing starts automatically when the frame buffer changes */
# define JO_VDP1_PTMR_AUTOMATIC_DRAW        (0x0002)
/** @brief EDSR: the drawing of the current frame is finished */
# define JO_VDP1_EDSR_CURRENT_END           (0x0002)
/** @brief TVMR: the displayed frame buffer is erased during the VBLANK (VBE), see jo_vdp1_vblank_erase() */
# define JO_VDP1_TVMR_VBLANK_ERASE          (0x0008)
/** @brief CMDCTRL: the command is skipped (jump mode: skip, next) */
# define JO_VDP1_CMD_SKIP_NEXT              (0x4000)
/** @brief The VBLANK erase (manual change): the lines kept out of the measure (see jo_vdp1_measure_vblank_erase()), the
 *  fields to wait after a change of the screen mode before the measure (the VDP2 takes the new mode at a VBLANK) */
# define JO_VDP1_VBLANK_ERASE_MARGIN        (2)
# define JO_VDP1_VBLANK_ERASE_DELAY         (3)
/** @brief The top of the clear polygon when the VBLANK erase cleared the whole frame buffer: the polygon is skipped */
# define JO_VDP1_CLEAR_SKIPPED              (-32768)
/** @brief VDP2 TVSTAT: horizontal blanking */
# define JO_VDP2_TVSTAT_HBLANK              (1 << 2)
/** @brief EWRR: erase/write area (right in 8 or 16 pixels unit, excluded: the width / unit; bottom line, included) */
# define JO_VDP1_ERASE_AREA(X_UNIT, BOTTOM)       (((__jo_screen.width / (X_UNIT)) << 9) | ((BOTTOM) - 1))
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
/** @brief The system clipping area in the local coordinates of each window, packed as the positions of the commands (right
 *  in the high half, bottom in the low half, see JO_VDP1_OUTSIDE()) for the pre-clipping of the sprites: a window with its
 *  own local coordinates (a screen of its own) has JO_VDP1_NO_PRE_CLIP_AREA (see jo_vdp1_update_pre_clip_areas()) */
static unsigned int                     __jo_vdp1_pre_clip_areas[JO_VDP1_WINDOW_COUNT];
/** @brief Area of no position: JO_VDP1_OUTSIDE() always has a sign bit */
# define JO_VDP1_NO_PRE_CLIP_AREA           (0xFFFFFFFFu)

/** @brief Compute the packed areas of the windows for the pre-clipping of the sprites (__jo_vdp1_pre_clip_areas), after a
 *  change of the system clipping area or of the local coordinates of a window */
static void                             jo_vdp1_update_pre_clip_areas(void)
{
    register int                        i;

    for (JO_ZERO(i); i < JO_VDP1_WINDOW_COUNT; ++i)
        __jo_vdp1_pre_clip_areas[i] = (__jo_vdp1_windows[i].origin_x | __jo_vdp1_windows[i].origin_y) ? JO_VDP1_NO_PRE_CLIP_AREA :
                                      ((unsigned int)__jo_vdp1_system_clip_right << 16) | (unsigned short)__jo_vdp1_system_clip_bottom;
}

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

/** @brief Changes of a window in a frame after commands were drawn in it, at most (see __jo_vdp1_keep_window(): beyond, the
 *  commands before a change take the window as it is at the end of the frame) */
# define JO_VDP1_WINDOW_CHANGES             (8)
/** @brief Headers of a frame: the windows, the windows before their changes */
# define JO_VDP1_HEADER_SLOTS               (JO_VDP1_WINDOW_COUNT + JO_VDP1_WINDOW_CHANGES)

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
    // The windows changed during the frame after commands (see __jo_vdp1_keep_window()): the commands before each change
    // keep the window as it was (a header of its own, window JO_VDP1_WINDOW_COUNT + k)
    unsigned int                        window_change_count;
    unsigned int                        window_change_end[JO_VDP1_WINDOW_CHANGES];      // Commands before the change
    unsigned short                      window_change_window[JO_VDP1_WINDOW_CHANGES];   // The window changed
    jo_vdp1_window                      window_change_state[JO_VDP1_WINDOW_CHANGES];    // The window before the change
}                                       jo_vdp1_frame;

/** @brief The windows do not overlap: the commands of each window are drawn together (see jo_3d_set_windows_apart()) */
bool                                    __jo_vdp1_windows_apart = false;

/** @brief The polygons and the distorted sprites of the next frames drawn as their outline (see jo_3d_set_wireframe()) */
bool                                    __jo_vdp1_wireframe = false;
#ifdef JO_TEST_VDP1_VBLANKS
/** @brief Test (JO_TEST_VDP1_VBLANKS, read by the game): the VBLANKs by what they found: no table and the VDP1 drawing,
 *  no table and the VDP1 idle (the CPUs are late), the DMA still copying the table, the table waiting for the VDP1, the
 *  table taken (manual change) */
unsigned int                            __jo_vdp1_test_vblanks[5];
/** @brief Test (JO_TEST_VDP1_VBLANKS): what the last VBLANK found (the index in __jo_vdp1_test_vblanks) */
int                                     __jo_vdp1_test_last_state = 0;
/** @brief Test (JO_TEST_VDP1_VBLANKS): the lines erased by the last measure of the VBLANK erase, the changes of the
 *  frame buffers with the VBLANK erase (see jo_vdp1_vblank_erase()) */
unsigned int                            __jo_vdp1_test_erase_lines = 0;
unsigned int                            __jo_vdp1_test_vblank_erases = 0;
#endif

/** @brief The frame buffer is cleared at the start of each frame (see jo_3d_set_frame_clear()) */
bool                                    __jo_vdp1_frame_clear = true;
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
# define JO_VDP1_DMA_CLEAR_COMMAND          (JO_VDP1_HEADER_COMMANDS * JO_VDP1_HEADER_SLOTS)
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
/** @brief The field the current frame of the game started at (see __jo_vdp1_frame_start(), JO_VDP1_PACED) */
static volatile unsigned int            __jo_vdp1_frame_field;
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
/* The VBLANK erase (manual change mode, see jo_vdp1_vblank_erase()): the lines of the frame buffer it clears (measured
** by jo_vdp1_measure_vblank_erase(), 0: not used), the measure to do (at the end of the frame, after the field
** __jo_vdp1_erase_measure_field), the clear polygon of the pending table (VRAM offset, 0: none), its top under the
** lines erased in the VBLANK (local coordinates, JO_VDP1_CLEAR_SKIPPED: none left) and its top on the whole screen, the
** clear polygon of the table taken when it was changed (to restore before a redraw), the VBE bit set during this VBLANK
** (JO_VDP1_SHARED) */
static volatile unsigned int            __jo_vdp1_erase_lines;
static bool                             __jo_vdp1_erase_measure = false;
static unsigned int                     __jo_vdp1_erase_measure_field;
static volatile unsigned int            __jo_vdp1_pending_clear;
static volatile short                   __jo_vdp1_pending_clear_top;
static volatile short                   __jo_vdp1_pending_clear_full;
static volatile unsigned int            __jo_vdp1_taken_clear;
static volatile short                   __jo_vdp1_taken_clear_full;
static volatile bool                    __jo_vdp1_vblank_erase_on;
/* Manual change: the VDP1 draws a table given at a VBLANK (see __jo_vdp1_vblank()) */
static bool                             __jo_vdp1_drawing = false;
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

/** @brief TVMR of the screen mode (without the VBLANK erase) */
# define JO_VDP1_TVMR_MODE                  (JO_VDP1_HIRES ? JO_VDP1_TVMR_HIRES : 0)

/** @brief The frame buffer registers of the screen mode: 16 bits per pixel (8 bits in hi-res), 1 cycle or manual change
 *  mode, double density interlace, erase area */
static void                             __jo_vdp1_set_registers(void)
{
    JO_VDP1_TVMR = JO_VDP1_TVMR_MODE;
    JO_VDP1_FBCR = JO_VDP1_INTERLACE ? JO_VDP1_FBCR_DOUBLE_INTERLACE : (JO_VDP1_MANUAL_CHANGE ? JO_VDP1_FBCR_MANUAL : 0);
    JO_VDP1_EWDR = 0;
    JO_VDP1_EWLR = 0;
    JO_VDP1_EWRR = JO_VDP1_HIRES ? JO_VDP1_ERASE_AREA(16, JO_VDP1_LINES) : JO_VDP1_ERASE_AREA(8, JO_VDP1_LINES);
    // The VBLANK erase is measured again for this mode (the lines of the VBLANK, the width)
    JO_VDP1_SHARED(__jo_vdp1_erase_lines) = 0;
    __jo_vdp1_erase_measure = JO_VDP1_MANUAL_CHANGE;
    __jo_vdp1_erase_measure_field = JO_VDP1_SHARED(__jo_vdp1_vblank_fields);
}

/** @brief Reads of the V counter before a line is given up (see jo_vdp1_wait_line(): about 6 fields) */
# define JO_VDP1_WAIT_LINE_READS            (100000)

/** @brief Wait for the start of a line (the V counter, latched by a read of EXTEN: not with a light gun, see
 *  jo_vdp1_measure_vblank_erase())
 *  @param line Line (0: the first line displayed, JO_VDP1_LINES: the first line of the VBLANK)
 *  @return false if the V counter never showed the line (a mode where it counts differently: no VBLANK erase)
 */
static bool                             jo_vdp1_wait_line(const unsigned int line)
{
    unsigned int                        reads;

    for (JO_ZERO(reads); reads < JO_VDP1_WAIT_LINE_READS; ++reads)
    {
        (void)JO_VDP2_EXTEN;
        if (JO_VDP2_VCNT != line)
            break;
    }
    for (; reads < JO_VDP1_WAIT_LINE_READS; ++reads)
    {
        (void)JO_VDP2_EXTEN;
        if (JO_VDP2_VCNT == line)
            return (true);
    }
    return (false);
}

/** @brief Measure the lines of the frame buffer that the VBLANK erase clears (manual change mode, see
 *  jo_vdp1_vblank_erase()): the erase is stopped at the end of the VBLANK (Sega: "erase the unerased part with
 *  polygons"), how far it goes depends on the lines of the VBLANK (NTSC or PAL, 224 or 240 lines), the width and the
 *  console. A mark at the end of each line of a frame buffer, displayed for a field, erased in the VBLANK: the lines
 *  without their mark are the lines erased
 *  @remarks About 4 fields, the interrupts masked (the VBE bit and the change must be written in the first line of the
 *  VBLANK). At the end of the first frame and of the first frame after a change of the screen mode: a field of the screen
 *  shows the frame buffer of the measure (black)
 */
static void                             jo_vdp1_measure_vblank_erase(void)
{
    volatile unsigned short             *fb;
    unsigned int                        sr;
    unsigned int                        line;
    unsigned int                        erased;
    unsigned int                        last;
    unsigned int                        field;

    __jo_vdp1_erase_measure = false;
    JO_VDP1_SHARED(__jo_vdp1_erase_lines) = 0;
    // (the V counter is latched by the light gun, not by the reads of EXTEN: the clear polygon only)
    if (!JO_VDP1_MANUAL_CHANGE || (JO_VDP2_EXTEN & JO_VDP2_EXTEN_EXLTEN) != 0)
        return;
    fb = (volatile unsigned short *)JO_VDP1_FB;
    // The last word of an erased line (16 bits per pixel; 8 bits in hi-res: 2 pixels per word)
    last = JO_VDP1_HIRES ? (unsigned int)__jo_screen.width / 2 - 1 : (unsigned int)__jo_screen.width - 1;
    JO_ZERO(erased);
    sr = __jo_core_disable_interrupts();
    // The table being drawn finishes (else the change at the VBLANK would stop it)
    for (field = 0; __jo_vdp1_drawing && field < 4 && (JO_VDP1_EDSR & JO_VDP1_EDSR_CURRENT_END) == 0; ++field)
        if (!jo_vdp1_wait_line(JO_VDP1_LINES))
            break;
    // No drawing at the changes of the frame buffers
    JO_VDP1_PTMR = 0;
    if (jo_vdp1_wait_line(JO_VDP1_LINES))
    {
        JO_VDP1_FBCR = JO_VDP1_FBCR_MANUAL_CHANGE;
        // (the frame buffer can't be accessed during the first line displayed)
        if (jo_vdp1_wait_line(2))
        {
            for (line = 0; line < (unsigned int)JO_VDP1_LINES; ++line)
                fb[line * 512 + last] = 0x8000;
            // The marks displayed for a field, erased in the VBLANK after it (VBE and the change: in the first line
            // of the VBLANK, VBE cleared after the VBLANK)
            if (jo_vdp1_wait_line(JO_VDP1_LINES))
            {
                JO_VDP1_FBCR = JO_VDP1_FBCR_MANUAL_CHANGE;
                if (jo_vdp1_wait_line(JO_VDP1_LINES))
                {
                    JO_VDP1_TVMR = JO_VDP1_TVMR_MODE | JO_VDP1_TVMR_VBLANK_ERASE;
                    JO_VDP1_FBCR = JO_VDP1_FBCR_MANUAL_CHANGE;
                    if (jo_vdp1_wait_line(2))
                        for (; erased < (unsigned int)JO_VDP1_LINES && fb[erased * 512 + last] == 0; ++erased)
                            ;
                    JO_VDP1_TVMR = JO_VDP1_TVMR_MODE;
                }
            }
            for (line = 0; line < (unsigned int)JO_VDP1_LINES; ++line)
                fb[line * 512 + last] = 0;
        }
    }
    JO_VDP1_PTMR = JO_VDP1_PTMR_AUTOMATIC_DRAW;
    // (the next table is taken at the next VBLANK: no drawing in progress)
    __jo_vdp1_drawing = false;
    __jo_core_restore_interrupts(sr);
    JO_VDP1_SHARED(__jo_vdp1_erase_lines) = erased > JO_VDP1_VBLANK_ERASE_MARGIN ? erased - JO_VDP1_VBLANK_ERASE_MARGIN : 0;
#ifdef JO_TEST_VDP1_VBLANKS
    __jo_vdp1_test_erase_lines = erased;
#endif
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
    {
        JO_ZERO(__jo_vdp1_frames[i].count);
        JO_ZERO(__jo_vdp1_frames[i].window_change_count);
    }
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
        __jo_vdp1_windows[i].right = __jo_screen.width - 1;
        __jo_vdp1_windows[i].bottom = __jo_screen.height - 1;
        __jo_vdp1_windows[i].center_x = __jo_screen.width_2;
        __jo_vdp1_windows[i].center_y = __jo_screen.height_2;
        __jo_vdp1_windows[i].zlimit = JO_VDP1_NO_Z_LIMIT;
        __jo_vdp1_windows[i].clip_mode = 0;
        __jo_vdp1_windows[i].origin_x = 0;
        __jo_vdp1_windows[i].origin_y = 0;
        __jo_vdp1_windows[i].screen = false;
    }
    __jo_vdp1_current_window = 0;
    jo_vdp1_update_pre_clip_areas();
    __jo_vdp1_set_registers();
    JO_VDP1_PTMR = JO_VDP1_PTMR_AUTOMATIC_DRAW;
}

void                                    __jo_vdp1_set_screen_mode(void)
{
    int                                 i;

    for (JO_ZERO(i); i < JO_VDP1_WINDOW_COUNT; ++i)
    {
        __jo_vdp1_windows[i].left = 0;
        __jo_vdp1_windows[i].top = 0;
        __jo_vdp1_windows[i].right = __jo_screen.width - 1;
        __jo_vdp1_windows[i].bottom = __jo_screen.height - 1;
        __jo_vdp1_windows[i].center_x = __jo_screen.width_2;
        __jo_vdp1_windows[i].center_y = __jo_screen.height_2;
        __jo_vdp1_windows[i].origin_x = 0;
        __jo_vdp1_windows[i].origin_y = 0;
        __jo_vdp1_windows[i].screen = false;
    }
    __jo_vdp1_system_clip_right = __jo_screen.width - 1;
    __jo_vdp1_system_clip_bottom = __jo_screen.height - 1;
    jo_vdp1_update_pre_clip_areas();
    __jo_vdp1_set_registers();
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

int                                     jo_vdp1_frame_command_count(void)
{
    return (__jo_vdp1_frame_command_count());
}

int                                     jo_vdp1_command_max(void)
{
    return (JO_VDP1_MAX_COMMANDS - JO_VDP1_HEADER_COMMANDS * 2 - 1);
}

/** @brief Allocate a command for the current frame, its link and gouraud table not written (see __jo_vdp1_add_command(): the
 *  sprites write all the words of their commands, the call and the 2 stores of 16 bits paid for the test of their
 *  pre-clipping)
 *  @param z Depth used to sort the commands (bigger = farther)
 *  @return Command or JO_NULL if the table is full
 */
static  __jo_force_inline jo_vdp1_command *jo_vdp1_new_command(const jo_fixed z)
{
    jo_vdp1_frame                       *frame;

    frame = __jo_vdp1_frame;
    if (__builtin_expect(frame->count >= JO_VDP1_MAX_COMMANDS - JO_VDP1_HEADER_COMMANDS * 2 - 1, 0))
        return (__jo_vdp1_add_command(z));
    frame->keys[frame->count] = __jo_vdp1_sort_key(z);
    frame->window[frame->count] = (unsigned short)__jo_vdp1_current_window;
    return (&frame->commands[frame->count++]);
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

    w = window < JO_VDP1_WINDOW_COUNT ? &frame->windows[window] : &frame->window_change_state[window - JO_VDP1_WINDOW_COUNT];
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
    int                                 ox;
    int                                 oy;

    // (sent after the header of the window 0: relative to its local coordinates, the top left corner of a clipped window)
    ox = frame->windows[0].origin_x;
    oy = frame->windows[0].origin_y;
    jo_memset(c, 0, sizeof(*c));
    c->ctrl = JO_VDP1_CMD_POLYGON;
    c->pmod = JO_VDP1_CLEAR_POLYGON_MODE;
    c->xa = (short)-ox;
    c->ya = (short)-oy;
    c->xb = (short)(frame->system_clip_right - ox);
    c->yb = (short)-oy;
    c->xc = (short)(frame->system_clip_right - ox);
    c->yc = (short)(frame->system_clip_bottom - oy);
    c->xd = (short)-ox;
    c->yd = (short)(frame->system_clip_bottom - oy);
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
    unsigned int                        first[JO_VDP1_HEADER_SLOTS];
    unsigned int                        sum;
    register unsigned int               i;
    unsigned int                        n;

    n = frame->count;
    for (JO_ZERO(i); i < JO_VDP1_HEADER_SLOTS; ++i)
        JO_ZERO(first[i]);
    for (JO_ZERO(i); i < n; ++i)
        ++first[frame->window[i]];
    for (JO_ZERO(i), JO_ZERO(sum); i < JO_VDP1_HEADER_SLOTS; ++i)
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

/** @brief The first command that fits in the table with the headers of its window changes (the commands sorted far to
 *  near: the farthest ones are dropped first)
 *  @param frame Frame (the windows of its commands)
 *  @param order Command indexes in the order of the sort
 *  @param n Number of commands (at least one)
 *  @param room Commands the table can take
 *  @return Index in order of the first command to draw (0: all)
 */
static unsigned int                     jo_vdp1_first_fitting(const jo_vdp1_frame * const frame, const unsigned short * const order,
                                                              unsigned int n, const unsigned int room)
{
    unsigned int                        cost;
    unsigned short                      window;
    unsigned short                      previous;

    // From the nearest: each command, and the header of each run of commands of a window
    previous = frame->window[order[n - 1]];
    cost = JO_VDP1_HEADER_COMMANDS + 1;
    while (--n > 0)
    {
        window = frame->window[order[n - 1]];
        cost += window != previous ? JO_VDP1_HEADER_COMMANDS + 1 : 1;
        if (cost > room)
            return (n);
        previous = window;
    }
    return (0);
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
    unsigned int                        clear;
    int                                 erased;
    short                               clear_top;
    short                               clear_full;

    frame = (const jo_vdp1_frame *)argument;
    JO_ZERO(clear);
    JO_ZERO(clear_top);
    JO_ZERO(clear_full);
    // (the commands of the slave CPU may still be copied by the DMA, see jo_3d_slave_copy_job())
    if (frame->wireframe)
    {
        jo_dma_wait();
        jo_vdp1_make_wireframe((jo_vdp1_frame *)argument);
    }
    // The sort only uses the work RAM: it is done while the VDP1 is still drawing
    jo_profile_begin(JO_PROFILE_VDP1_SORT);
    n = frame->count;
    // (the commands drawn before a change of their window: the window as it was, see __jo_vdp1_keep_window())
    for (i = 0; i < frame->window_change_count; ++i)
        for (j = 0; j < frame->window_change_end[i]; ++j)
            if (frame->window[j] == frame->window_change_window[i])
                ((jo_vdp1_frame *)frame)->window[j] = (unsigned short)(JO_VDP1_WINDOW_COUNT + i);
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
    for (window = 0; window < JO_VDP1_WINDOW_COUNT + (int)frame->window_change_count; ++window)
        jo_vdp1_make_header(&__jo_vdp1_dma_commands[window * JO_VDP1_HEADER_COMMANDS], frame, window);
    jo_memset(&__jo_vdp1_dma_commands[JO_VDP1_DMA_END_COMMAND], 0, sizeof(jo_vdp1_command));
    __jo_vdp1_dma_commands[JO_VDP1_DMA_END_COMMAND].ctrl = JO_VDP1_CMD_END;
    current_window = -1;
    skip = false;
    JO_ZERO(skip_key);
    if (JO_VDP1_MANUAL_CHANGE && __jo_vdp1_frame_clear)
    {
        // The frame buffer is not erased by the VDP1 (manual change mode): a polygon of the transparent color clears it
        // (not when the program covers the whole screen at each frame, see jo_3d_set_frame_clear(): the polygon costs
        // about 4 ms of VDP1 time)
        jo_vdp1_make_clear(&__jo_vdp1_dma_commands[JO_VDP1_DMA_CLEAR_COMMAND], frame);
        jo_vdp1_dma_add(dma, &entries, &__jo_vdp1_dma_commands[0], offset, JO_VDP1_HEADER_COMMANDS * sizeof(jo_vdp1_command));
        offset += JO_VDP1_HEADER_COMMANDS * sizeof(jo_vdp1_command);
        current_window = 0;
        // (when the VBLANK erase clears the frame buffer, see jo_vdp1_vblank_erase(): the polygon only covers the lines
        // under the lines erased, or it is skipped)
        clear = offset;
        erased = (int)JO_VDP1_SHARED(__jo_vdp1_erase_lines);
        clear_full = __jo_vdp1_dma_commands[JO_VDP1_DMA_CLEAR_COMMAND].ya;
        clear_top = erased > frame->system_clip_bottom ? JO_VDP1_CLEAR_SKIPPED : (short)(clear_full + erased);
        jo_vdp1_dma_add(dma, &entries, &__jo_vdp1_dma_commands[JO_VDP1_DMA_CLEAR_COMMAND], offset, sizeof(jo_vdp1_command));
        offset += sizeof(jo_vdp1_command);
    }
    // A window change costs JO_VDP1_HEADER_COMMANDS commands in the table, jo_vdp1_command_max() keeps room for 2: when
    // the commands of the windows are mixed by depth (a room seen through a door, drawn in its clipped window, around
    // the camera standing in the door), the table could overflow, and the nearest commands (the weapon, the HUD) would
    // be the ones dropped. The farthest ones are dropped instead (counted only when the frame could overflow)
    if (n > 0 && n * (JO_VDP1_HEADER_COMMANDS + 1) > (limit - offset) / sizeof(jo_vdp1_command))
    {
        j = jo_vdp1_first_fitting(frame, order, n, (limit - offset) / sizeof(jo_vdp1_command));
        order += j;
        n -= j;
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
    JO_VDP1_SHARED(__jo_vdp1_pending_clear) = clear;
    JO_VDP1_SHARED(__jo_vdp1_pending_clear_top) = clear_top;
    JO_VDP1_SHARED(__jo_vdp1_pending_clear_full) = clear_full;
    JO_VDP1_SHARED(__jo_vdp1_pending_tag) = frame->tag;
    JO_VDP1_SHARED(__jo_vdp1_pending_table) = table;
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
    // The SCU DMA reads the buffer of the frame until the end of the copy (see __jo_vdp1_end_frame())
    JO_VDP1_SHARED(__jo_vdp1_frames_done) = JO_VDP1_SHARED(__jo_vdp1_frames_done) + 1;
#endif
}

void                                    __jo_vdp1_frame_start(void)
{
    JO_VDP1_SHARED(__jo_vdp1_frame_field) = JO_VDP1_SHARED(__jo_vdp1_vblank_fields);
}

void                                    __jo_vdp1_end_frame(void)
{
    jo_vdp1_frame                       *frame;
    int                                 i;

    // The VBLANK erase measured for the screen mode (first frame, change of the mode), when the frame before is in the
    // VDP1 (see jo_vdp1_measure_vblank_erase())
    if (__jo_vdp1_erase_measure &&
        JO_VDP1_SHARED(__jo_vdp1_vblank_fields) - __jo_vdp1_erase_measure_field >= JO_VDP1_VBLANK_ERASE_DELAY)
    {
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
        while (__jo_vdp1_frames_sent != JO_VDP1_SHARED(__jo_vdp1_frames_done))
            ;
#endif
        while (JO_VDP1_SHARED(__jo_vdp1_pending_table) || jo_vdp1_dma_busy())
            ;
        jo_vdp1_measure_vblank_erase();
    }
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
    // The pace of the images (see JO_VDP1_PACED): the table is sent in the last field of the frame at the earliest, the
    // VDP1 takes it at the VBLANK that starts the next frame (the master CPU waits for that VBLANK anyway)
    // (at 60 frames per second there is no field to wait for)
#if JO_FRAMERATE > 1
    if (JO_VDP1_MANUAL_CHANGE && JO_VDP1_PACED)
        while (JO_VDP1_SHARED(__jo_vdp1_vblank_fields) - JO_VDP1_SHARED(__jo_vdp1_frame_field) < (unsigned int)(JO_FRAMERATE - 1))
            ;
#endif
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
    if (__jo_core_is_slave_running())
    {
        // The slave CPU sorts and copies the commands while the master CPU computes the next frame in the other buffer
        ++__jo_vdp1_frames_sent;
        __jo_core_add_slave_job(jo_vdp1_send_frame, frame);
        // Manual change: the CPUs one frame ahead of the VDP1 at most. When the previous frame still waits for the VDP1
        // (it draws the one before), the slave CPU waits for it in jo_vdp1_send_frame() before it copies this frame, and
        // the jobs of the next frame (the 3D of the slave CPU) wait behind: the slave CPU lost 8 to 13 ms per frame, the
        // master CPU drew alone meanwhile, and the CPUs stayed 2 frames ahead of the VDP1 (a split screen, the VDP1 at
        // 27 to 30 ms per frame: the master CPU busy 33 ms in every frame). The master CPU waits here
        // instead, until the VDP1 takes that frame: the next frame starts with the change of the frame buffers
        if (JO_VDP1_MANUAL_CHANGE)
        {
            const unsigned int          tag = frame->tag;

            while (JO_VDP1_SHARED(__jo_vdp1_pending_table) && JO_VDP1_SHARED(__jo_vdp1_pending_tag) != tag)
                ;
        }
        frame = (frame == &__jo_vdp1_frames[0]) ? &__jo_vdp1_frames[1] : &__jo_vdp1_frames[0];
        // The previous frame of this buffer must be in the VRAM (the slave CPU is usually done since a long time)
        while (__jo_vdp1_frames_sent - JO_VDP1_SHARED(__jo_vdp1_frames_done) > 1)
            ;
        // The SCU DMA may still read the buffer (or already copy the frame just sent: the wait is short)
        while (jo_vdp1_dma_busy())
            ;
        JO_ZERO(frame->count);
        JO_ZERO(frame->window_change_count);
        __jo_vdp1_frame = frame;
        return;
    }
#endif
    jo_vdp1_send_frame(frame);
    // The SCU DMA reads the buffer of the frame
    while (jo_vdp1_dma_busy())
        ;
    JO_ZERO(frame->count);
    JO_ZERO(frame->window_change_count);
}

void                                    jo_vdp1_redraw_last_frame(void)
{
    volatile jo_vdp1_command            *clear;

    // (manual change: the VDP1 draws only when the frame buffers change; 1 cycle mode: at each field already)
    if (JO_VDP1_MANUAL_CHANGE && (JO_VDP1_EDSR & JO_VDP1_EDSR_CURRENT_END) != 0)
    {
        // No VBLANK erase for this change: the clear polygon of the table covers the whole screen again
        if (JO_VDP1_SHARED(__jo_vdp1_taken_clear))
        {
            clear = JO_VDP1_COMMAND_AT(JO_VDP1_SHARED(__jo_vdp1_taken_clear));
            clear->ctrl = JO_VDP1_CMD_POLYGON;
            clear->ya = JO_VDP1_SHARED(__jo_vdp1_taken_clear_full);
            clear->yb = JO_VDP1_SHARED(__jo_vdp1_taken_clear_full);
            JO_ZERO(JO_VDP1_SHARED(__jo_vdp1_taken_clear));
        }
        JO_VDP1_FBCR = JO_VDP1_FBCR_MANUAL_CHANGE;
    }
}

/** @brief The VBLANK erase (manual change mode, called by __jo_vdp1_vblank() when it changes the frame buffers): the
 *  displayed frame buffer is erased during the VBLANK and becomes the frame buffer drawn at the end of the VBLANK. The
 *  clear polygon of the table (about 4 ms of VDP1 time in 320 x 224) only covers the lines that the VBLANK is too short
 *  to erase (measured, see jo_vdp1_measure_vblank_erase()), or it is skipped (one VRAM write). The erase of the manual
 *  erase mode (FBCR FCM = 1, FCT = 0) would need to know a field before the change that the VDP1 finishes its drawing
 *  during that field (a frame buffer erased without a change: a field without the VDP1 image)
 *  @remarks Sega: VBE is set "immediately after the V blank IN interrupt" and taken into account after one line, it is
 *  cleared after the VBLANK-OUT interrupt (else the next VBLANK erases too, see __jo_vdp1_vblank_out()). When the
 *  write came too late (the V counter after the first line of the VBLANK; with a light gun, unknown) the clear polygon
 *  covers the whole screen as before: the result is right with or without the erase
 */
static void                             jo_vdp1_vblank_erase(void)
{
    const unsigned int                  clear = JO_VDP1_SHARED(__jo_vdp1_pending_clear);
    const short                         top = JO_VDP1_SHARED(__jo_vdp1_pending_clear_top);
    const short                         full = JO_VDP1_SHARED(__jo_vdp1_pending_clear_full);
    volatile jo_vdp1_command            *cmd;

    JO_ZERO(JO_VDP1_SHARED(__jo_vdp1_taken_clear));
    if (!clear || top == full)
        return;
    JO_VDP1_TVMR = JO_VDP1_TVMR_MODE | JO_VDP1_TVMR_VBLANK_ERASE;
    JO_VDP1_SHARED(__jo_vdp1_vblank_erase_on) = true;
    // (reading EXTEN latches the counters, not with a light gun; still in the first line of the VBLANK, before its
    // HBLANK: the VDP1 takes VBE at the end of that line)
    if ((JO_VDP2_EXTEN & JO_VDP2_EXTEN_EXLTEN) != 0 || JO_VDP2_VCNT != (unsigned short)JO_VDP1_LINES ||
        (JO_VDP2_TVSTAT & JO_VDP2_TVSTAT_HBLANK) != 0)
        return;
    cmd = JO_VDP1_COMMAND_AT(clear);
    if (top == JO_VDP1_CLEAR_SKIPPED)
        cmd->ctrl = JO_VDP1_CMD_POLYGON | JO_VDP1_CMD_SKIP_NEXT;
    else
    {
        cmd->ya = top;
        cmd->yb = top;
    }
    JO_VDP1_SHARED(__jo_vdp1_taken_clear) = clear;
    JO_VDP1_SHARED(__jo_vdp1_taken_clear_full) = full;
#ifdef JO_TEST_VDP1_VBLANKS
    ++__jo_vdp1_test_vblank_erases;
#endif
}

void                                    __jo_vdp1_vblank_out(void)
{
    if (!JO_VDP1_SHARED(__jo_vdp1_vblank_erase_on))
        return;
    JO_VDP1_TVMR = JO_VDP1_TVMR_MODE;
    JO_VDP1_SHARED(__jo_vdp1_vblank_erase_on) = false;
}

void                                    __jo_vdp1_vblank(void)
{
    unsigned int                        table;

    JO_VDP1_SHARED(__jo_vdp1_vblank_fields) = JO_VDP1_SHARED(__jo_vdp1_vblank_fields) + 1;
    // Double density interlace: the VDP1 draws the lines of the next field
    if (JO_VDP1_INTERLACE)
        JO_VDP1_FBCR = (JO_VDP2_TVSTAT & JO_VDP2_TVSTAT_ODD) ? JO_VDP1_FBCR_DOUBLE_INTERLACE_EVEN : JO_VDP1_FBCR_DOUBLE_INTERLACE;
    // 1 cycle mode: the table given at the last VBLANK is displayed from this field
    if (JO_VDP1_SHARED(__jo_vdp1_next_shown_field) == JO_VDP1_SHARED(__jo_vdp1_vblank_fields))
    {
        JO_VDP1_SHARED(__jo_vdp1_shown_tag) = JO_VDP1_SHARED(__jo_vdp1_next_shown_tag);
        JO_VDP1_SHARED(__jo_vdp1_shown_field) = JO_VDP1_SHARED(__jo_vdp1_vblank_fields);
    }
    table = JO_VDP1_SHARED(__jo_vdp1_pending_table);
    // The SCU DMA still copies the table
#ifdef JO_TEST_VDP1_VBLANKS
    if (!table)
        __jo_vdp1_test_last_state = (JO_VDP1_EDSR & JO_VDP1_EDSR_CURRENT_END) == 0 ? 0 : 1;
    else if (jo_vdp1_dma_busy())
        __jo_vdp1_test_last_state = 2;
    else if (JO_VDP1_MANUAL_CHANGE && __jo_vdp1_drawing && (JO_VDP1_EDSR & JO_VDP1_EDSR_CURRENT_END) == 0)
        __jo_vdp1_test_last_state = 3;
    else
        __jo_vdp1_test_last_state = 4;
    ++__jo_vdp1_test_vblanks[__jo_vdp1_test_last_state];
#endif
    if (!table || jo_vdp1_dma_busy())
        return;
    if (JO_VDP1_MANUAL_CHANGE)
    {
        // Manual change: the new table is drawn when the VDP1 finished the current one (else the next VBLANK)
        if (__jo_vdp1_drawing && (JO_VDP1_EDSR & JO_VDP1_EDSR_CURRENT_END) == 0)
            return;
        // (first: VBE in the first line of the VBLANK)
        jo_vdp1_vblank_erase();
        jo_vdp1_write_jump(table);
        // The frame buffers change at the end of this VBLANK, then the VDP1 draws the new table: the image of the
        // previous table is displayed from this field
        JO_VDP1_FBCR = JO_VDP1_FBCR_MANUAL_CHANGE;
        if (__jo_vdp1_drawing)
        {
            JO_VDP1_SHARED(__jo_vdp1_shown_tag) = JO_VDP1_SHARED(__jo_vdp1_taken_tag);
            JO_VDP1_SHARED(__jo_vdp1_shown_field) = JO_VDP1_SHARED(__jo_vdp1_vblank_fields);
        }
        __jo_vdp1_drawing = true;
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
 *  @param mode Added to the draw mode (JO_VDP1_PRE_CLIPPING_DISABLE, see jo_vdp1_pre_clipping())
 *  @return true if textured
 *  @remarks The type is known before the command is written: it is not read back (the cache does not keep the lines
 *  written, a read of the command after it is written reads the work RAM)
 */
static  __jo_force_inline bool          jo_vdp1_write_attribute(jo_vdp1_command * const cmd, const jo_vdp1_sprite_attribute * const attr,
                                                                const unsigned short sprite_type, const unsigned short polygon_type,
                                                                const unsigned short mode)
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
    words[1] = JO_VDP1_PACK(attr->draw_mode | mode, attr->color);
    // (the gouraud bit: the modes 4, 6 and 7, gouraud with half luminance or half transparency)
    words[7] = JO_VDP1_PACK((attr->draw_mode & JO_VDP1_COLOR_CALC_GOURAUD) ? attr->gouraud_table : 0, 0);
    return (textured);
}

bool                                    __jo_vdp1_set_attribute(jo_vdp1_command *cmd, const jo_vdp1_sprite_attribute *attr)
{
    return (jo_vdp1_write_attribute(cmd, attr, 0, 0, 0));
}

/** @brief Sign bits of a packed position (JO_VDP1_PACK(x, y)) out of a packed area (see __jo_vdp1_pre_clip_areas): x or y
 *  negative, or over the right or the bottom (a borrow of y into x only comes with a y out of the area, found by its own
 *  sign). The bits of several positions are or'ed, then tested by jo_vdp1_pre_clipping()
 */
# define JO_VDP1_OUTSIDE(AREA, P)           ((P) | ((AREA) - (P)))

/** @brief The pre-clipping of a command disabled when it is inside of the system clipping area (see
 *  JO_VDP1_PRE_CLIPPING_DISABLE: the VDP1 tests each line of a command against the clipping area, about 4 cycles per line)
 *  @param draw_mode Draw mode of the command (clipped by the user clipping area: the pre-clipping is kept, the VDP1 skips
 *  the lines out of the area)
 *  @param outside The sign bits of the corners of the command (see JO_VDP1_OUTSIDE())
 *  @return JO_VDP1_PRE_CLIPPING_DISABLE or 0
 *  @remarks The image doesn't change: the pixels out of the clipping area are not drawn anyway
 *  @remarks Packed positions and areas, no branch (the user clipping bit moved to a sign bit): the first test (the min and
 *  max of the coordinates, the origin of the window added) cost 1.75 microseconds per sprite (200 sprites of the test ROM:
 *  +11 % of CPU)
 */
static  __jo_force_inline unsigned short jo_vdp1_pre_clipping(const unsigned short draw_mode, const unsigned int outside)
{
    // (JO_VDP1_WINDOW_IN: bit 10, moved to the bit 15)
    return ((unsigned short)((((outside | ((unsigned int)draw_mode << 5)) & 0x80008000u) == 0) << 11));
}

/** @brief Write the command of a rotated sprite (see jo_vdp1_draw_sprite(): out of its common path, the registers of this
 *  one made the sprites without rotation slower)
 *  @param cmd Command
 *  @param attr Sprite attribute
 *  @param cx Horizontal center (screen pixels, Q16.16)
 *  @param cy Vertical center (screen pixels, Q16.16)
 *  @param w Last pixel of the width (width - 1)
 *  @param h Last pixel of the height
 *  @param angle Rotation
 */
static __attribute__((noinline)) void   jo_vdp1_write_rotated_sprite(jo_vdp1_command * const cmd, const jo_vdp1_sprite_attribute * const attr,
                                                                     const jo_fixed cx, const jo_fixed cy, const int w, const int h,
                                                                     const jo_angle angle)
{
    jo_vdp1_command_word                *words;
    jo_fixed                            s;
    jo_fixed                            c;
    jo_fixed                            lx0;
    jo_fixed                            ly0;
    jo_fixed                            lx1;
    jo_fixed                            ly1;
    jo_fixed                            ox;
    jo_fixed                            oy;
    unsigned int                        area;
    unsigned int                        p0;
    unsigned int                        p1;
    unsigned int                        p2;
    unsigned int                        p3;

    words = JO_VDP1_COMMAND_WORDS(cmd);
    s = jo_sin_angle(angle);
    c = jo_cos_angle(angle);
    // Rotation around the integer part of the center
    ox = cx & ~JO_FIXED_DECIMAL_MASK;
    oy = cy & ~JO_FIXED_DECIMAL_MASK;
    lx0 = JO_INT_TO_FIXED(-JO_DIV_BY_2(w));
    ly0 = JO_INT_TO_FIXED(-JO_DIV_BY_2(h));
    lx1 = lx0 + JO_INT_TO_FIXED(w);
    ly1 = ly0 + JO_INT_TO_FIXED(h);
    p0 = JO_VDP1_PACK(JO_FIXED_TO_INT(ox + jo_fixed_mult(lx0, c) - jo_fixed_mult(ly0, s)),
                      JO_FIXED_TO_INT(oy + jo_fixed_mult(lx0, s) + jo_fixed_mult(ly0, c)));
    p1 = JO_VDP1_PACK(JO_FIXED_TO_INT(ox + jo_fixed_mult(lx1, c) - jo_fixed_mult(ly0, s)),
                      JO_FIXED_TO_INT(oy + jo_fixed_mult(lx1, s) + jo_fixed_mult(ly0, c)));
    p2 = JO_VDP1_PACK(JO_FIXED_TO_INT(ox + jo_fixed_mult(lx1, c) - jo_fixed_mult(ly1, s)),
                      JO_FIXED_TO_INT(oy + jo_fixed_mult(lx1, s) + jo_fixed_mult(ly1, c)));
    p3 = JO_VDP1_PACK(JO_FIXED_TO_INT(ox + jo_fixed_mult(lx0, c) - jo_fixed_mult(ly1, s)),
                      JO_FIXED_TO_INT(oy + jo_fixed_mult(lx0, s) + jo_fixed_mult(ly1, c)));
    area = __jo_vdp1_pre_clip_areas[__jo_vdp1_current_window];
    jo_vdp1_write_attribute(cmd, attr, JO_VDP1_CMD_DISTORTED_SPRITE, 0,
                            jo_vdp1_pre_clipping(attr->draw_mode,
                                                 JO_VDP1_OUTSIDE(area, p0) | JO_VDP1_OUTSIDE(area, p1) |
                                                 JO_VDP1_OUTSIDE(area, p2) | JO_VDP1_OUTSIDE(area, p3)));
    words[3] = p0;
    words[4] = p1;
    words[5] = p2;
    words[6] = p3;
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
    bool                                normal;
    unsigned int                        area;
    unsigned int                        first;
    unsigned int                        last;
    unsigned short                      mode;

    texture = &__jo_sprite_def[attr->texture_id];
    // (a negative scale is rare: out of the common path)
    if (__builtin_expect(sx < 0, 0))
        sx = -sx;
    if (__builtin_expect(sy < 0, 0))
        sy = -sy;
    // Size in pixels (rounded to the nearest pixel). Without scale (the most common sprite): the size of the texture, the
    // same without the 2 products (they paid for the test of the pre-clipping)
    normal = sx == JO_FIXED_1 && sy == JO_FIXED_1;
    if (normal)
    {
        w = texture->width;
        h = texture->height;
    }
    else
    {
        w = JO_FIXED_TO_INT(jo_fixed_mult(JO_INT_TO_FIXED(texture->width), sx) + JO_FIXED_1_DIV_2);
        h = JO_FIXED_TO_INT(jo_fixed_mult(JO_INT_TO_FIXED(texture->height), sy) + JO_FIXED_1_DIV_2);
        // A scale that leaves no pixel: nothing drawn (the VDP1 drew the corner pixel, a dot left by a sprite shrunk to 0)
        if (w <= 0 || h <= 0)
            return (true);
    }
    if ((cmd = jo_vdp1_new_command(z)) == JO_NULL)
        return (false);
    words = JO_VDP1_COMMAND_WORDS(cmd);
    x0 = JO_FIXED_TO_INT(cx) - JO_DIV_BY_2(w);
    y0 = JO_FIXED_TO_INT(cy) - JO_DIV_BY_2(h);
    // Last pixel (inclusive coordinates)
    if (w > 0)
        --w;
    if (h > 0)
        --h;
    if (angle != 0)
    {
        jo_vdp1_write_rotated_sprite(cmd, attr, cx, cy, w, h, angle);
        return (true);
    }
    // (the corners of the sprite packed: its first and last pixels)
    first = JO_VDP1_PACK(x0, y0);
    last = JO_VDP1_PACK(x0 + w, y0 + h);
    area = __jo_vdp1_pre_clip_areas[__jo_vdp1_current_window];
    mode = jo_vdp1_pre_clipping(attr->draw_mode, JO_VDP1_OUTSIDE(area, first) | JO_VDP1_OUTSIDE(area, last));
    if (normal)
        textured = jo_vdp1_write_attribute(cmd, attr, JO_VDP1_CMD_NORMAL_SPRITE, JO_VDP1_CMD_POLYGON, mode);
    else
        textured = jo_vdp1_write_attribute(cmd, attr, JO_VDP1_CMD_SCALED_SPRITE, JO_VDP1_CMD_POLYGON, mode);
    words[3] = first;
    if (textured && normal)
        return (true);
    if (textured)
    {
        words[5] = last;
        return (true);
    }
    words[4] = JO_VDP1_PACK(x0 + w, y0);
    words[5] = last;
    words[6] = JO_VDP1_PACK(x0, y0 + h);
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
    unsigned int                        area;
    unsigned int                        p0;
    unsigned int                        p1;
    unsigned int                        p2;
    unsigned int                        p3;

    if (!jo_vdp1_is_sprite_visible(z))
        return (false);
    if ((cmd = jo_vdp1_new_command(z)) == JO_NULL)
        return (false);
    cx = jo_vdp1_window_center_x();
    cy = jo_vdp1_window_center_y();
    p0 = JO_VDP1_PACK(JO_FIXED_TO_INT(cx + points[0].x), JO_FIXED_TO_INT(cy + points[0].y));
    p1 = JO_VDP1_PACK(JO_FIXED_TO_INT(cx + points[1].x), JO_FIXED_TO_INT(cy + points[1].y));
    p2 = JO_VDP1_PACK(JO_FIXED_TO_INT(cx + points[2].x), JO_FIXED_TO_INT(cy + points[2].y));
    p3 = JO_VDP1_PACK(JO_FIXED_TO_INT(cx + points[3].x), JO_FIXED_TO_INT(cy + points[3].y));
    area = __jo_vdp1_pre_clip_areas[__jo_vdp1_current_window];
    jo_vdp1_write_attribute(cmd, attr, JO_VDP1_CMD_DISTORTED_SPRITE, 0,
                            jo_vdp1_pre_clipping(attr->draw_mode,
                                                 JO_VDP1_OUTSIDE(area, p0) | JO_VDP1_OUTSIDE(area, p1) |
                                                 JO_VDP1_OUTSIDE(area, p2) | JO_VDP1_OUTSIDE(area, p3)));
    words = JO_VDP1_COMMAND_WORDS(cmd);
    words[3] = p0;
    words[4] = p1;
    words[5] = p2;
    words[6] = p3;
    return (true);
}

bool                                    __jo_vdp1_write_sprite_4p(jo_vdp1_command * const cmd, const jo_pos2D_fixed * const points, const jo_fixed z,
                                                                  const jo_vdp1_sprite_attribute * const attr, const int window)
{
    const jo_vdp1_window                *w;
    jo_vdp1_command_word                *words;
    jo_fixed                            cx;
    jo_fixed                            cy;
    unsigned int                        area;
    unsigned int                        p0;
    unsigned int                        p1;
    unsigned int                        p2;
    unsigned int                        p3;

    w = &__jo_vdp1_windows[window];
    if (z <= __jo_vdp1_near_clip() || z > w->zlimit)
        return (false);
    // (the same command as __jo_vdp1_draw_sprite_4p())
    cx = JO_MULT_BY_65536(w->center_x - w->origin_x);
    cy = JO_MULT_BY_65536(w->center_y - w->origin_y);
    p0 = JO_VDP1_PACK(JO_FIXED_TO_INT(cx + points[0].x), JO_FIXED_TO_INT(cy + points[0].y));
    p1 = JO_VDP1_PACK(JO_FIXED_TO_INT(cx + points[1].x), JO_FIXED_TO_INT(cy + points[1].y));
    p2 = JO_VDP1_PACK(JO_FIXED_TO_INT(cx + points[2].x), JO_FIXED_TO_INT(cy + points[2].y));
    p3 = JO_VDP1_PACK(JO_FIXED_TO_INT(cx + points[3].x), JO_FIXED_TO_INT(cy + points[3].y));
    area = __jo_vdp1_pre_clip_areas[window];
    jo_vdp1_write_attribute(cmd, attr, JO_VDP1_CMD_DISTORTED_SPRITE, 0,
                            jo_vdp1_pre_clipping(attr->draw_mode,
                                                 JO_VDP1_OUTSIDE(area, p0) | JO_VDP1_OUTSIDE(area, p1) |
                                                 JO_VDP1_OUTSIDE(area, p2) | JO_VDP1_OUTSIDE(area, p3)));
    words = JO_VDP1_COMMAND_WORDS(cmd);
    words[3] = p0;
    words[4] = p1;
    words[5] = p2;
    words[6] = p3;
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
    jo_vdp1_update_pre_clip_areas();
}

/** @brief Before a change of the current window: the commands drawn so far in the frame keep the window as it is (its
 *  header is written once per frame from the window at the end of the frame: a window changed in the middle of a frame,
 *  like the render to image demo that draws into a window out of the screen and then sets it to the whole screen, lost
 *  its first state)
 */
static void                             __jo_vdp1_keep_window(void)
{
    jo_vdp1_frame                       *frame;
    unsigned int                        k;

    frame = __jo_vdp1_frame;
    k = frame->window_change_count;
    // (no command yet, or none since the last change of this window: nothing to keep)
    if (frame->count == 0 || k >= JO_VDP1_WINDOW_CHANGES ||
        (k > 0 && frame->window_change_end[k - 1] == frame->count && frame->window_change_window[k - 1] == __jo_vdp1_current_window))
        return;
    frame->window_change_end[k] = frame->count;
    frame->window_change_window[k] = (unsigned short)__jo_vdp1_current_window;
    frame->window_change_state[k] = __jo_vdp1_windows[__jo_vdp1_current_window];
    frame->window_change_count = k + 1;
}

void                                    __jo_vdp1_set_window(const short left, const short top, const short right, const short bottom,
                                                             const short zlimit, const short center_x, const short center_y)
{
    jo_vdp1_window                      *w;

    w = &__jo_vdp1_windows[__jo_vdp1_current_window];
    // (the header of the window changes: the commands before keep the window as it was)
    if (w->left != left || w->top != top || w->right != right || w->bottom != bottom)
        __jo_vdp1_keep_window();
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
    jo_vdp1_update_pre_clip_areas();
}

void                                    jo_3d_set_depth_limit(const int z)
{
    __jo_vdp1_windows[__jo_vdp1_current_window].zlimit = z < 0 || z >= 0x7fff ? JO_VDP1_NO_Z_LIMIT : JO_INT_TO_FIXED((jo_fixed)z);
}

void                                    __jo_vdp1_set_window_clip(const bool clip)
{
    jo_vdp1_window                      *w;

    w = &__jo_vdp1_windows[__jo_vdp1_current_window];
    // (the origin of the header changes with a screen of its own)
    if ((clip && __jo_vdp1_windows_apart) != w->screen)
        __jo_vdp1_keep_window();
    w->clip_mode = clip ? JO_VDP1_WINDOW_IN : 0;
    w->screen = clip && __jo_vdp1_windows_apart;
    w->origin_x = w->screen ? w->left : 0;
    w->origin_y = w->screen ? w->top : 0;
    jo_vdp1_update_pre_clip_areas();
}

/*
** END OF FILE
*/
