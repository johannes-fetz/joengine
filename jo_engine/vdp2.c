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
#include "jo/malloc.h"
#include "jo/fs.h"
#include "jo/image.h"
#include "jo/colors.h"
#include "jo/vdp1_command_pipeline.h"
#include "jo/sprites.h"
#include "jo/vdp2.h"
#include "jo/math.h"
#include "jo/palette.h"
#include "jo/vdp2_malloc.h"
#include "jo/list.h"
#include "jo/3d.h"
#include "internal/core.h"
#include "internal/vdp2_malloc.h"
#include "internal/vdp2.h"

#ifdef JO_COMPILE_WITH_PRINTF_SUPPORT
void                                jo_init_nbg0_printf(void);
#endif
void                                jo_vdp2_malloc_init(void);

//NBG0
#ifndef JO_COMPILE_WITH_PRINTF_SUPPORT
static unsigned short               *nbg0_map = JO_NULL;
static unsigned char                *nbg0_cell = JO_NULL;
#endif
static int                          *nbg0_scroll_table = JO_NULL;
//NBG1
jo_color                            *nbg1_bitmap = JO_NULL;
bool                                __jo_vdp2_nbg1_bitmap_reserved = false;
static unsigned short               *nbg1_map = JO_NULL;
static unsigned char                *nbg1_cell = JO_NULL;
static int                          *nbg1_scroll_table = JO_NULL;
static int                          *nbg1_vertical_cell_scroll_table = JO_NULL;
//NBG2
static unsigned short               *nbg2_map = JO_NULL;
static unsigned char                *nbg2_cell = JO_NULL;
#ifndef JO_COMPILE_WITH_PRINTF_SUPPORT
static char                         *nbg2_font_str = JO_NULL;
static unsigned short               nbg2_font_paloff = 0;
static int                          nbg2_font_map_offset = 0;
#endif
//NBG3
static unsigned short               *nbg3_map = JO_NULL;
static unsigned char                *nbg3_cell = JO_NULL;
#ifndef JO_COMPILE_WITH_PRINTF_SUPPORT
static char                         *nbg3_font_str = JO_NULL;
static unsigned short               nbg3_font_paloff = 0;
static int                          nbg3_font_map_offset = 0;
#endif
//RBG0
static unsigned char                *rbg0_cell_a = JO_NULL;
static unsigned char                *rbg0_cell_b = JO_NULL;
static unsigned short               *rbg0_map_a = JO_NULL;
static unsigned short               *rbg0_map_a_border = JO_NULL;
static unsigned short               *rbg0_map_b = JO_NULL;
static void                         *rbg0_ktable = JO_NULL;
static void                         *rbg0_rtable = JO_NULL;
/* Over mode of the plane B set with its image (see jo_vdp2_draw_rbg0_plane_b_flat()) */
static unsigned short               rbg0_over_mode_b = 0;
//OTHER
static unsigned int                 screen_flags = 0;
static void                         *back_color = JO_NULL;

/*
██████╗ ███████╗ ██████╗ ██╗███████╗████████╗███████╗██████╗ ███████╗
██╔══██╗██╔════╝██╔════╝ ██║██╔════╝╚══██╔══╝██╔════╝██╔══██╗██╔════╝
██████╔╝█████╗  ██║  ███╗██║███████╗   ██║   █████╗  ██████╔╝███████╗
██╔══██╗██╔══╝  ██║   ██║██║╚════██║   ██║   ██╔══╝  ██╔══██╗╚════██║
██║  ██║███████╗╚██████╔╝██║███████║   ██║   ███████╗██║  ██║███████║
╚═╝  ╚═╝╚══════╝ ╚═════╝ ╚═╝╚══════╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚══════╝

Every register write goes to a RAM copy of the VDP2 registers. Modified
registers are copied to the hardware during the next VBLANK, so changes are
applied atomically between two frames.
*/

# define JO_VDP2_REG_COUNT              ((JO_VDP2_LAST_REG - JO_VDP2_REG) / 2 + 1)
# define JO_VDP2_REG_INDEX(REG)         ((((unsigned int)&(REG)) - JO_VDP2_REG) >> 1)
# define JO_VDP2_SET(REG, VALUE)        jo_vdp2_shadow_set(JO_VDP2_REG_INDEX(REG), (VALUE))
# define JO_VDP2_GET(REG)               (__jo_vdp2_shadow[JO_VDP2_REG_INDEX(REG)])
# define JO_VDP2_SET_BITS(REG, MASK, VALUE) jo_vdp2_shadow_set_bits(JO_VDP2_REG_INDEX(REG), (MASK), (VALUE))

/*
** Register fields
*/

/** @brief VRAM offset of an address (VDP2 VRAM is 512 KB) */
# define JO_VDP2_VRAM_OFFSET_MASK           (0x7FFFF)
/** @brief VDP2 VRAM size */
# define JO_VDP2_VRAM_SIZE                  (0x80000)
/** @brief VDP2 color RAM size */
# define JO_VDP2_CRAM_SIZE                  (0x1000)

/** @brief TVMD: display enabled */
# define JO_VDP2_TVMD_DISPLAY               (1 << 15)
/** @brief TVMD: the border uses the back screen color */
# define JO_VDP2_TVMD_BORDER_BACK_COLOR     (1 << 8)
/** @brief TVMD: double density interlace */
# define JO_VDP2_TVMD_DOUBLE_DENSITY        (3 << 6)
/** @brief TVMD: single density interlace */
# define JO_VDP2_TVMD_SINGLE_DENSITY        (2 << 6)
/** @brief TVMD: vertical resolution shift */
# define JO_VDP2_TVMD_VRESO_SHIFT           (4)

/** @brief RAMCTL: color RAM mode 1 (2048 colors, RGB 555) */
# define JO_VDP2_RAMCTL_CRAM_MODE_1         (1 << 12)
/** @brief RAMCTL: VRAM bank A divided in A0 and A1 */
# define JO_VDP2_RAMCTL_SPLIT_BANK_A        (1 << 8)
/** @brief RAMCTL: VRAM bank B divided in B0 and B1 */
# define JO_VDP2_RAMCTL_SPLIT_BANK_B        (1 << 9)
/** @brief RAMCTL: rotation data of a VRAM bank (bank: 0=A0, 1=A1, 2=B0, 3=B1) */
# define JO_VDP2_RAMCTL_ROTATION_DATA(BANK, DATA)  ((DATA) << JO_MULT_BY_2(BANK))
/** @brief RAMCTL rotation data: coefficient table */
# define JO_VDP2_ROTATION_DATA_COEFFICIENT  (1)
/** @brief RAMCTL rotation data: pattern name table */
# define JO_VDP2_ROTATION_DATA_PATTERN_NAME (2)
/** @brief RAMCTL rotation data: character pattern */
# define JO_VDP2_ROTATION_DATA_CHARACTER    (3)
/** @brief RAMCTL rotation data mask of a bank */
# define JO_VDP2_ROTATION_DATA_MASK         (3)

/** @brief BGON / CCCTL / MZCTL / CLOFEN / CLOFSL: NBG0 to NBG3 and RBG0 bits */
# define JO_VDP2_SCROLL_SCREENS_MASK        (0x001f)
/** @brief BGON: displayed screens bits (NBG0 to NBG3, RBG0 and RBG1) */
# define JO_VDP2_BGON_DISPLAY_MASK          (0x003f)
/** @brief BGON: transparent pixel disabled bits shift */
# define JO_VDP2_BGON_TRANSPARENCY_SHIFT    (8)
/** @brief CCCTL / CLOFEN / CLOFSL: scroll screens, back screen and sprites bits */
# define JO_VDP2_ALL_SCREENS_MASK           (0x007f)

/** @brief CHCTLA: NBG0 bits */
# define JO_VDP2_CHCTLA_NBG0_MASK           (0x00ff)
/** @brief CHCTLA: NBG1 bits */
# define JO_VDP2_CHCTLA_NBG1_MASK           (0xff00)
/** @brief CHCTLB: NBG2 bits */
# define JO_VDP2_CHCTLB_NBG2_MASK           (0x000f)
/** @brief CHCTLB: NBG3 bits */
# define JO_VDP2_CHCTLB_NBG3_MASK           (0x00f0)
/** @brief CHCTLB: RBG0 bits */
# define JO_VDP2_CHCTLB_RBG0_MASK           (0xff00)
/** @brief CHCTLB: RBG0 bitmap enabled */
# define JO_VDP2_CHCTLB_RBG0_BITMAP         (1 << 9)
/** @brief Bitmap size (JO_VDP2_SIZE): bitmap enabled and size bits */
# define JO_VDP2_BITMAP_SIZE_MASK           (0x0e)
/** @brief Bitmap size: 1024 pixels width */
# define JO_VDP2_BITMAP_WIDTH_1024          (1 << 3)
/** @brief Bitmap size: 512 lines height */
# define JO_VDP2_BITMAP_HEIGHT_512          (1 << 2)

/** @brief Page size (bytes) with 1 word pattern name data */
# define JO_VDP2_PAGE_SIZE_1WORD            (0x2000)
/** @brief Page size (bytes) with 2 words pattern name data */
# define JO_VDP2_PAGE_SIZE_2WORDS           (0x4000)
/** @brief Character number bits of a 1 word pattern name (12 bits mode) */
# define JO_VDP2_CHARACTER_NUMBER_MASK      (0x0fff)
/** @brief Supplementary character number bits (10 bits mode) */
# define JO_VDP2_SUPPLEMENTARY_CHARACTER_MASK (0x1f)
/** @brief Map register: plane address bits */
# define JO_VDP2_MAP_ADDRESS_MASK           (0x3f)
/** @brief Map offset register (MPOFN, MPOFR): offset bits of a screen */
# define JO_VDP2_MAP_OFFSET_MASK            (0x7)

/** @brief PLSZ: plane size bits of a screen */
# define JO_VDP2_PLANE_SIZE_MASK            (0x3)
/** @brief PLSZ: rotation parameter A over mode shift */
# define JO_VDP2_PLSZ_RA_OVER_SHIFT         (10)
/** @brief PLSZ: rotation parameter B plane size shift */
# define JO_VDP2_PLSZ_RB_PLANE_SIZE_SHIFT   (12)
/** @brief PLSZ: rotation parameter B over mode shift */
# define JO_VDP2_PLSZ_RB_OVER_SHIFT         (14)
/** @brief Over mode bits */
# define JO_VDP2_OVER_MODE_MASK             (0x3)

/** @brief ZMCTL: zoom reduction to 1/2 enabled */
# define JO_VDP2_ZOOM_REDUCTION_HALF        (1)
/** @brief ZMCTL: zoom reduction to 1/4 enabled */
# define JO_VDP2_ZOOM_REDUCTION_QUARTER     (2)
/** @brief ZMCTL: zoom reduction bits of a screen */
# define JO_VDP2_ZOOM_REDUCTION_MASK        (0x3)
/** @brief ZMCTL: NBG1 bits shift */
# define JO_VDP2_ZMCTL_NBG1_SHIFT           (8)
/** @brief Zoom and scroll: integer part bits */
# define JO_VDP2_ZOOM_INTEGER_MASK          (0x7)
/** @brief Scroll: integer part bits */
# define JO_VDP2_SCROLL_INTEGER_MASK        (0x7ff)
/** @brief Zoom and scroll: fractional part bits (8 bits) */
# define JO_VDP2_FRACTION_MASK              (0xff00)

/** @brief Table address (LSTA, RPTA, BKTA...): upper part bits */
# define JO_VDP2_TABLE_ADDRESS_HIGH_MASK    (0x7)
/** @brief Table address (BKTA, LCTA): lower part bits */
# define JO_VDP2_TABLE_ADDRESS_LOW_16BITS_MASK  (0xffff)
/** @brief BKTAU: one back screen color per line */
# define JO_VDP2_BKTAU_LINE_MODE        (1 << 15)
/** @brief Table address (LSTA, RPTA): lower part bits (word aligned on 2 words) */
# define JO_VDP2_TABLE_ADDRESS_LOW_MASK     (0xfffe)

/** @brief Priority registers: priority of the first screen (low byte) */
# define JO_VDP2_PRIORITY_LOW_MASK          (0x0007)
/** @brief Priority registers: priority of the second screen (high byte) */
# define JO_VDP2_PRIORITY_HIGH_MASK         (0x0700)
/** @brief Priority registers: both priorities */
# define JO_VDP2_PRIORITY_BOTH_MASK         (JO_VDP2_PRIORITY_LOW_MASK | JO_VDP2_PRIORITY_HIGH_MASK)
/** @brief Priority registers value
 *  @param HIGH Priority of the second screen (0 to 7)
 *  @param LOW Priority of the first screen (0 to 7)
 */
# define JO_VDP2_PRIORITY_PAIR(HIGH, LOW)   (((HIGH) << 8) | (LOW))

/** @brief Color calculation rate registers: rate of the first screen (low byte) */
# define JO_VDP2_COLOR_RATE_LOW_MASK        (0x001f)
/** @brief Color calculation rate registers: rate of the second screen (high byte) */
# define JO_VDP2_COLOR_RATE_HIGH_MASK       (0x1f00)
/** @brief CCCTL: color calculation mode bits (0 = ratio mode, top image ratio) */
# define JO_VDP2_CCCTL_MODE_MASK            (0x0700)
/** @brief CCCTL: color calculation mode: the colors are added (not a ratio) */
# define JO_VDP2_CCCTL_ADD                  (0x0100)

/** @brief SPCTL: sprite type 3 */
# define JO_VDP2_SPCTL_TYPE_3               (3)
/** @brief SPCTL: sprite type F (8 bits frame buffer of the hi-res modes: 256 colors, priority register 0) */
# define JO_VDP2_SPCTL_TYPE_F               (0xF)
/** @brief SPCTL: sprite type bits */
# define JO_VDP2_SPCTL_TYPE_MASK            (0xF)
/** @brief SPCTL: palette and RGB sprites mixed */
# define JO_VDP2_SPCTL_MIXED_COLOR          (1 << 5)
/** @brief SPCTL: color calculation condition bits */
# define JO_VDP2_SPCTL_COLOR_CALC_MASK      (0x3700)
/** @brief SPCTL: color calculation when the sprite priority is <= 7 (all sprites) */
# define JO_VDP2_SPCTL_COLOR_CALC_ALL       (7 << 8)
/** @brief SPCTL: color calculation when the MSB of the color is 1 (the RGB pixels only) */
# define JO_VDP2_SPCTL_COLOR_CALC_MSB       (3 << 12)

/** @brief SCRCTL: NBG0 line scroll bits (the bit 0, the vertical cell scroll, is set apart) */
# define JO_VDP2_SCRCTL_NBG0_MASK           (0x003e)
/** @brief SCRCTL: NBG0 vertical cell scroll bit (shifted by JO_VDP2_SCRCTL_NBG1_SHIFT for NBG1) */
# define JO_VDP2_SCRCTL_VERTICAL_CELL_SCROLL (0x0001)
/** @brief SCRCTL: NBG1 bits shift */
# define JO_VDP2_SCRCTL_NBG1_SHIFT          (8)

/** @brief MZCTL: mosaic size bits */
# define JO_VDP2_MOSAIC_SIZE_MASK           (0xff00)
/** @brief MZCTL: mosaic size value
 *  @param X Horizontal size (1 to 16)
 *  @param Y Vertical size (1 to 16)
 */
# define JO_VDP2_MOSAIC_SIZE(X, Y)          ((((Y) - 1) << 12) | (((X) - 1) << 8))

/** @brief Color offset: value bits (signed 9 bits) */
# define JO_VDP2_COLOR_OFFSET_MASK          (0x1ff)

/** @brief Window end position (whole screen, hardware default) */
# define JO_VDP2_WINDOW_END_X               (0x27e)
/** @brief Window end position (whole screen, hardware default) */
# define JO_VDP2_WINDOW_END_Y               (0x1be)

static unsigned short               __jo_vdp2_shadow[JO_VDP2_REG_COUNT];
static volatile unsigned int        __jo_vdp2_dirty[(JO_VDP2_REG_COUNT + 31) / 32];
static unsigned short               __jo_vdp2_tvmd;

/** @brief Set a VDP2 register (applied during the next VBLANK)
 *  @param index Register index (JO_VDP2_REG_INDEX())
 *  @param value Register value
 */
static void                         jo_vdp2_shadow_set(const unsigned int index, const unsigned short value)
{
    unsigned int                    sr;

    sr = __jo_core_disable_interrupts();
    __jo_vdp2_shadow[index] = value;
    __jo_vdp2_dirty[index >> 5] |= (1u << (index & 31));
    __jo_core_restore_interrupts(sr);
}

/** @brief Set some bits of a VDP2 register (applied during the next VBLANK)
 *  @param index Register index (JO_VDP2_REG_INDEX())
 *  @param mask Bits to change
 *  @param value New value of the bits
 */
static void                         jo_vdp2_shadow_set_bits(const unsigned int index, const unsigned short mask, const unsigned short value)
{
    unsigned int                    sr;

    // (the read and the write with the interrupts masked: a VBLANK callback changing the same register in between)
    sr = __jo_core_disable_interrupts();
    __jo_vdp2_shadow[index] = (__jo_vdp2_shadow[index] & ~mask) | (value & mask);
    __jo_vdp2_dirty[index >> 5] |= (1u << (index & 31));
    __jo_core_restore_interrupts(sr);
}

/** @brief Copy the modified registers to the VDP2
 *  @remarks Called during VBLANK-IN
 */
static void                         jo_vdp2_commit(void)
{
    register unsigned int           w;
    register unsigned int           bits;
    register unsigned int           index;

    for (JO_ZERO(w); w < (sizeof(__jo_vdp2_dirty) / sizeof(*__jo_vdp2_dirty)); ++w)
    {
        bits = __jo_vdp2_dirty[w];
        if (!bits)
            continue;
        JO_ZERO(__jo_vdp2_dirty[w]);
        for (index = JO_MULT_BY_32(w); bits; ++index, bits >>= 1)
        {
            if (bits & 1)
                ((volatile unsigned short *)JO_VDP2_REG)[index] = __jo_vdp2_shadow[index];
        }
    }
}

/** @brief Copy the modified registers to the VDP2 immediately */
static void                         jo_vdp2_flush_now(void)
{
    unsigned int                    sr;

    sr = __jo_core_disable_interrupts();
    jo_vdp2_commit();
    __jo_core_restore_interrupts(sr);
}

/*
███████╗ ██████╗██████╗  ██████╗ ██╗     ██╗         ███████╗ ██████╗██████╗ ███████╗███████╗███╗   ██╗███████╗
██╔════╝██╔════╝██╔══██╗██╔═══██╗██║     ██║         ██╔════╝██╔════╝██╔══██╗██╔════╝██╔════╝████╗  ██║██╔════╝
███████╗██║     ██████╔╝██║   ██║██║     ██║         ███████╗██║     ██████╔╝█████╗  █████╗  ██╔██╗ ██║███████╗
╚════██║██║     ██╔══██╗██║   ██║██║     ██║         ╚════██║██║     ██╔══██╗██╔══╝  ██╔══╝  ██║╚██╗██║╚════██║
███████║╚██████╗██║  ██║╚██████╔╝███████╗███████╗    ███████║╚██████╗██║  ██║███████╗███████╗██║ ╚████║███████║
╚══════╝ ╚═════╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚══════╝    ╚══════╝ ╚═════╝╚═╝  ╚═╝╚══════╝╚══════╝╚═╝  ╚═══╝╚══════╝
     CONFIGURATION (used to compute map addresses and VRAM cycle patterns)
*/

# define JO_VDP2_SCREEN_NBG0            (0)
# define JO_VDP2_SCREEN_NBG1            (1)
# define JO_VDP2_SCREEN_NBG2            (2)
# define JO_VDP2_SCREEN_NBG3            (3)
# define JO_VDP2_SCREEN_RBG0            (4)
# define JO_VDP2_SCREEN_COUNT           (5)

/* Character color count */
# define JO_VDP2_COLOR_16               (0)
# define JO_VDP2_COLOR_256              (1)
# define JO_VDP2_COLOR_2048             (2)
# define JO_VDP2_COLOR_32768            (3)
# define JO_VDP2_COLOR_16M              (4)

/* Pattern name data */
# define JO_VDP2_PATTERN_1WORD          (0x8000)
# define JO_VDP2_PATTERN_12BIT          (0x4000)

/* Rotation parameter mode (RPMD) */
# define JO_VDP2_ROTATION_PARAMETER_A   (0)
# define JO_VDP2_ROTATION_COEFFICIENT   (2)

/* Over mode: what is displayed outside of the plane */
# define JO_VDP2_OVER_MODE_REPEAT       (0)
# define JO_VDP2_OVER_MODE_CHARACTER    (1)
# define JO_VDP2_OVER_MODE_TRANSPARENT  (3)

/* Line scroll mode (SCRCTL): horizontal line scroll, 1 line */
# define JO_VDP2_LINE_SCROLL_HORIZONTAL (0x02)
/** @brief SCRCTL: NBG0 vertical line scroll bit (shifted by JO_VDP2_SCRCTL_NBG1_SHIFT for NBG1) */
# define JO_VDP2_LINE_SCROLL_VERTICAL   (0x04)

typedef struct
{
    bool                            bitmap;
    unsigned char                   color;          // JO_VDP2_COLOR_xxx
    unsigned char                   char_size;      // 0: 1x1 cell, 1: 2x2 cells
    unsigned char                   plane_size;     // 0: 1x1 page, 1: 2x1 pages, 3: 2x2 pages
    unsigned short                  pattern;        // JO_VDP2_PATTERN_xxx
    unsigned int                    cell;           // Character data VRAM offset
    unsigned int                    map[4];         // Plane VRAM offsets
    unsigned int                    bitmap_addr;    // Bitmap VRAM offset
    unsigned short                  bitmap_size;    // See JO_VDP2_SIZE
    jo_fixed                           zoom_x;
    jo_fixed                           zoom_y;
}                                   jo_vdp2_screen;

static jo_vdp2_screen               __jo_vdp2_screens[JO_VDP2_SCREEN_COUNT];
static unsigned int                 __jo_vdp2_displayed_screens;
/* The NBG1 registers hold the configuration of the NBG1 bitmap (see jo_vdp2_use_nbg1_bitmap()) */
static bool                         __jo_vdp2_nbg1_bitmap_registers = false;
/** @brief The NBG1 bitmap has 256 colors at the resolution of the screen (see jo_vdp2_set_nbg1_8bits_bitmap()) */
static bool                         __jo_vdp2_nbg1_native = false;
/** @brief BMPNA and BMPNB: palette number of a 256 colors bitmap (x 256 colors) */
# define JO_VDP2_BMPN_PALETTE_MASK      (0x0007)
/** @brief Sprite type of the 640 and 704 pixels modes (see jo_vdp2_set_hires_sprite_mode()) */
static unsigned short               __jo_vdp2_hires_sprite_type = 0xF;
static void                         jo_vdp2_use_nbg1_bitmap(void);
static bool                         jo_vdp2_nbg1_bitmap_ready(void);
static void                         jo_vdp2_display_nbg1(void);
static unsigned int                 __jo_vdp2_rotation_map[2];
/** @brief Map of the 15 planes around the plane A of the rotation parameter A (see jo_vdp2_set_rbg0_plane_a_8bits_island()),
 *  0: the 16 planes show the map of the image */
static unsigned int                 __jo_vdp2_rotation_island_map = 0;

/** @brief Get the VRAM offset of an address
 *  @param addr Address in VDP2 VRAM
 *  @return Offset from the beginning of the VRAM
 */
static  __jo_force_inline unsigned int  jo_vdp2_vram_offset(const void *addr)
{
    return (((unsigned int)addr) & JO_VDP2_VRAM_OFFSET_MASK);
}

/** @brief Get the size of a page
 *  @param s Screen configuration
 *  @return Page size in bytes
 */
static unsigned int                 jo_vdp2_page_size(const jo_vdp2_screen *s)
{
    unsigned int                    size;

    size = (s->pattern & JO_VDP2_PATTERN_1WORD) ? JO_VDP2_PAGE_SIZE_1WORD : JO_VDP2_PAGE_SIZE_2WORDS;
    if (s->char_size)
        size >>= 2;
    return (size);
}

/** @brief Get the supplementary character number bits of a screen
 *  @param s Screen configuration
 *  @return Bits for the pattern name control register
 *  @remarks Gives the upper bits of the character number from the cell address
 */
static unsigned short               jo_vdp2_supplementary_character(const jo_vdp2_screen *s)
{
    unsigned int                    character;

    character = s->cell >> 5;
    if (s->pattern & JO_VDP2_PATTERN_12BIT)
        return (((character >> 12) & 0x7) << 2);
    return ((character >> 10) & JO_VDP2_SUPPLEMENTARY_CHARACTER_MASK);
}

/** @brief Screens whose special function codes change the priority (see jo_vdp2_enable_special_priority()) */
static unsigned short               __jo_vdp2_special_priority_screens = 0;
/** @brief Screens whose special function codes enable the color calculation (see jo_vdp2_enable_special_blend()) */
static unsigned short               __jo_vdp2_special_blend_screens = 0;
/** @brief PNCN: special priority bit of the characters (1 word pattern names) */
# define JO_VDP2_PNCN_SPECIAL_PRIORITY      (1 << 9)
/** @brief PNCN: special color calculation bit of the characters (1 word pattern names) */
# define JO_VDP2_PNCN_SPECIAL_BLEND         (1 << 8)

/** @brief Update the pattern name control register of a screen
 *  @param n Screen (JO_VDP2_SCREEN_xxx)
 */
static void                         jo_vdp2_update_pattern_register(const int n)
{
    jo_vdp2_screen                  *s;
    static const unsigned int       regs[JO_VDP2_SCREEN_COUNT] =
    {
        JO_VDP2_REG_INDEX(JO_VDP2_PNCN0), JO_VDP2_REG_INDEX(JO_VDP2_PNCN1), JO_VDP2_REG_INDEX(JO_VDP2_PNCN2),
        JO_VDP2_REG_INDEX(JO_VDP2_PNCN3), JO_VDP2_REG_INDEX(JO_VDP2_PNCR)
    };

    s = &__jo_vdp2_screens[n];
    jo_vdp2_shadow_set(regs[n], (s->pattern & (JO_VDP2_PATTERN_1WORD | JO_VDP2_PATTERN_12BIT)) | jo_vdp2_supplementary_character(s) |
                                ((__jo_vdp2_special_priority_screens & (1 << n)) ? JO_VDP2_PNCN_SPECIAL_PRIORITY : 0) |
                                ((__jo_vdp2_special_blend_screens & (1 << n)) ? JO_VDP2_PNCN_SPECIAL_BLEND : 0));
}

/** @brief Update the map registers of a normal scroll screen
 *  @param n Screen (JO_VDP2_SCREEN_NBG0 to JO_VDP2_SCREEN_NBG3)
 *  @remarks Does nothing for RBG0 (see jo_vdp2_update_rotation_map_registers())
 */
static void                         jo_vdp2_update_map_registers(const int n)
{
    jo_vdp2_screen                  *s;
    unsigned int                    p[4];
    register unsigned int           i;
    unsigned int                    offset;
    unsigned int                    index;

    if (n >= JO_VDP2_SCREEN_RBG0)
        return;
    if (n == JO_VDP2_SCREEN_NBG1)
        __jo_vdp2_nbg1_bitmap_registers = false;
    s = &__jo_vdp2_screens[n];
    for (JO_ZERO(i); i < 4; ++i)
        p[i] = (s->map[i] / jo_vdp2_page_size(s));
    offset = (p[0] >> 6) & 7;
    // MPABNx and MPCDNx registers are consecutive for each screen
    index = JO_VDP2_REG_INDEX(JO_VDP2_MPABN0) + JO_MULT_BY_2(n);
    jo_vdp2_shadow_set(index, ((p[1] & JO_VDP2_MAP_ADDRESS_MASK) << 8) | (p[0] & JO_VDP2_MAP_ADDRESS_MASK));
    jo_vdp2_shadow_set(index + 1, ((p[3] & JO_VDP2_MAP_ADDRESS_MASK) << 8) | (p[2] & JO_VDP2_MAP_ADDRESS_MASK));
    JO_VDP2_SET_BITS(JO_VDP2_MPOFN, JO_VDP2_MAP_OFFSET_MASK << JO_MULT_BY_4(n), offset << JO_MULT_BY_4(n));
}

/** @brief Update the map registers of the rotation parameters A and B */
static void                         jo_vdp2_update_rotation_map_registers(void)
{
    register unsigned int           i;
    unsigned int                    unit;
    unsigned int                    pa;
    unsigned int                    pb;
    unsigned int                    border;

    unit = jo_vdp2_page_size(&__jo_vdp2_screens[JO_VDP2_SCREEN_RBG0]);
    pa = (__jo_vdp2_rotation_map[0] / unit) & JO_VDP2_MAP_ADDRESS_MASK;
    pb = (__jo_vdp2_rotation_map[1] / unit) & JO_VDP2_MAP_ADDRESS_MASK;
    border = __jo_vdp2_rotation_island_map ? (__jo_vdp2_rotation_island_map / unit) & JO_VDP2_MAP_ADDRESS_MASK : pa;
    // 16 planes for each rotation parameter (MPABRA to MPOPRA, MPABRB to MPOPRB)
    for (JO_ZERO(i); i < 8; ++i)
    {
        jo_vdp2_shadow_set(JO_VDP2_REG_INDEX(JO_VDP2_MPABRA) + i, (border << 8) | (i ? border : pa));
        jo_vdp2_shadow_set(JO_VDP2_REG_INDEX(JO_VDP2_MPABRB) + i, (pb << 8) | pb);
    }
    JO_VDP2_SET_BITS(JO_VDP2_MPOFR, JO_VDP2_MAP_OFFSET_MASK, (__jo_vdp2_rotation_map[0] / unit) >> 6);
    JO_VDP2_SET_BITS(JO_VDP2_MPOFR, JO_VDP2_MAP_OFFSET_MASK << 4, ((__jo_vdp2_rotation_map[1] / unit) >> 6) << 4);
}

/** @brief Update the character control registers of a screen
 *  @param n Screen (JO_VDP2_SCREEN_xxx)
 */
static void                         jo_vdp2_update_character_registers(const int n)
{
    jo_vdp2_screen                  *s;
    unsigned short                  a;
    unsigned short                  b;

    s = &__jo_vdp2_screens[n];
    a = JO_VDP2_GET(JO_VDP2_CHCTLA);
    b = JO_VDP2_GET(JO_VDP2_CHCTLB);
    switch (n)
    {
        case JO_VDP2_SCREEN_NBG0:
            a &= ~JO_VDP2_CHCTLA_NBG0_MASK;
            a |= (s->color & 7) << 4;
            if (s->bitmap)
                a |= s->bitmap_size & JO_VDP2_BITMAP_SIZE_MASK;
            else
                a |= s->char_size & 1;
            break;
        case JO_VDP2_SCREEN_NBG1:
            a &= ~JO_VDP2_CHCTLA_NBG1_MASK;
            a |= (s->color & 3) << 12;
            if (s->bitmap)
                a |= (s->bitmap_size & JO_VDP2_BITMAP_SIZE_MASK) << 8;
            else
                a |= (s->char_size & 1) << 8;
            break;
        case JO_VDP2_SCREEN_NBG2:
            b &= ~JO_VDP2_CHCTLB_NBG2_MASK;
            b |= ((s->color & 1) << 1) | (s->char_size & 1);
            break;
        case JO_VDP2_SCREEN_NBG3:
            b &= ~JO_VDP2_CHCTLB_NBG3_MASK;
            b |= ((s->color & 1) << 5) | ((s->char_size & 1) << 4);
            break;
        case JO_VDP2_SCREEN_RBG0:
            b &= ~JO_VDP2_CHCTLB_RBG0_MASK;
            b |= (s->color & 7) << 12;
            if (s->bitmap)
                b |= ((s->bitmap_size & JO_VDP2_BITMAP_HEIGHT_512) << 8) | JO_VDP2_CHCTLB_RBG0_BITMAP;
            else
                b |= (s->char_size & 1) << 8;
            break;
    }
    JO_VDP2_SET(JO_VDP2_CHCTLA, a);
    JO_VDP2_SET(JO_VDP2_CHCTLB, b);
}

/*
** VRAM cycle patterns
**
** Each VRAM bank gives 8 access slots per pixel group (T0 to T7). Every enabled
** normal scroll screen needs pattern name reads (cell mode) and character or
** bitmap reads in the banks where its data lives. Banks used by the rotation
** scroll (RAMCTL.RDBS) are not driven by the cycle pattern registers.
*/
# define JO_VDP2_CYCLE_PATTERN_NAME(N)  (0x0 + (N))
# define JO_VDP2_CYCLE_CHARACTER(N)     (0x4 + (N))
# define JO_VDP2_CYCLE_VERTICAL_CELL_SCROLL(N) (0xC + (N))
# define JO_VDP2_CYCLE_CPU              (0xE)
# define JO_VDP2_CYCLE_NONE             (0xF)

/** @brief Get the VRAM bank of an offset
 *  @param offset VRAM offset
 *  @return Bank index (0 = A0, 1 = A1, 2 = B0, 3 = B1)
 *  @remarks When a bank is not divided, the first half is returned
 */
static unsigned int                 jo_vdp2_bank_of(const unsigned int offset)
{
    unsigned short                  ramctl;
    unsigned int                    bank;

    ramctl = JO_VDP2_GET(JO_VDP2_RAMCTL);
    bank = (offset >> 17) & 3;
    // Bank index: A0=0, A1=1, B0=2, B1=3. When a bank pair is not split, use the first half
    if (bank < 2 && !(ramctl & JO_VDP2_RAMCTL_SPLIT_BANK_A))
        return (0);
    if (bank >= 2 && !(ramctl & JO_VDP2_RAMCTL_SPLIT_BANK_B))
        return (2);
    return (bank);
}

/** @brief Get the VRAM accesses needed to read the characters of a screen
 *  @param s Screen configuration
 *  @return Number of access slots (more colors or a reduction need more accesses)
 */
static unsigned int                 jo_vdp2_character_access_count(const jo_vdp2_screen *s)
{
    static const unsigned char      counts[5] = {1, 2, 4, 4, 8};
    unsigned int                    count;

    count = counts[s->color > 4 ? 4 : s->color];
    // Reduction needs more accesses
    if ((s->zoom_x >> __jo_screen.hires) > JO_FIXED_2)
        count <<= 2;
    else if ((s->zoom_x >> __jo_screen.hires) > JO_FIXED_1)
        count <<= 1;
    return (count);
}

/** @brief Check if a VRAM bank is used by the rotation scroll screen
 *  @param bank Bank index (0 = A0, 1 = A1, 2 = B0, 3 = B1)
 *  @return true if RBG0 is displayed and reads this bank
 */
static bool                         jo_vdp2_is_bank_used_by_rotation(const unsigned int bank)
{
    if (!(__jo_vdp2_displayed_screens & JO_RBG0_SCREEN))
        return (false);
    return (((JO_VDP2_GET(JO_VDP2_RAMCTL) >> JO_MULT_BY_2(bank)) & 3) != 0);
}

/** @brief Compute the VRAM cycle patterns of the displayed normal scroll screens
 *  @remarks Must be called after any change of the screens configuration
 */
static void                         jo_vdp2_update_cycle_patterns(void)
{
    static const unsigned char      bitmap_bpp4[5] = {1, 2, 4, 4, 8};
    unsigned char                   slots[4][8];
    unsigned char                   used[4];
    register unsigned int           n;
    register unsigned int           i;
    unsigned int                    count;
    unsigned int                    first;
    unsigned int                    bank;
    unsigned int                    pattern_name_slot;
    unsigned int                    banks_of_bitmap;
    jo_vdp2_screen                  *s;

    for (JO_ZERO(n); n < 4; ++n)
    {
        JO_ZERO(used[n]);
        for (JO_ZERO(i); i < 8; ++i)
            slots[n][i] = (i < 4) ? JO_VDP2_CYCLE_NONE : JO_VDP2_CYCLE_CPU;
    }
    for (JO_ZERO(n); n < 4; ++n)
    {
        if (!(__jo_vdp2_displayed_screens & (1 << n)))
            continue;
        s = &__jo_vdp2_screens[n];
        JO_ZERO(pattern_name_slot);
        if (s->bitmap)
        {
            count = jo_vdp2_character_access_count(s);
            if (count > 4)
                count = 4;
            // A bitmap can span several banks
            banks_of_bitmap = (((s->bitmap_size & JO_VDP2_BITMAP_WIDTH_1024) ? 1024 : 512) * ((s->bitmap_size & JO_VDP2_BITMAP_HEIGHT_512) ? 512 : 256) *
                              bitmap_bpp4[s->color > 4 ? 4 : s->color]) / 2;
            banks_of_bitmap = (banks_of_bitmap + JO_VDP2_BANK_SIZE - 1) / JO_VDP2_BANK_SIZE;
            for (JO_ZERO(i); i < banks_of_bitmap; ++i)
            {
                bank = jo_vdp2_bank_of(s->bitmap_addr + (i << 17));
                if (jo_vdp2_is_bank_used_by_rotation(bank))
                    continue;
                for (first = used[bank]; first < 4 && first < used[bank] + count; ++first)
                    slots[bank][first] = JO_VDP2_CYCLE_CHARACTER(n);
                used[bank] = first;
            }
            continue;
        }
        // Pattern name read
        for (JO_ZERO(i); i < 4; ++i)
        {
            if (!s->map[i] && i)
                continue;
            bank = jo_vdp2_bank_of(s->map[i]);
            if (jo_vdp2_is_bank_used_by_rotation(bank))
                continue;
            if (slots[bank][0] == JO_VDP2_CYCLE_PATTERN_NAME(n) || slots[bank][1] == JO_VDP2_CYCLE_PATTERN_NAME(n) ||
                slots[bank][2] == JO_VDP2_CYCLE_PATTERN_NAME(n) || slots[bank][3] == JO_VDP2_CYCLE_PATTERN_NAME(n))
                continue;
            if (used[bank] < 4)
            {
                pattern_name_slot = used[bank];
                slots[bank][used[bank]++] = JO_VDP2_CYCLE_PATTERN_NAME(n);
            }
        }
        // Character pattern reads (after the pattern name read)
        bank = jo_vdp2_bank_of(s->cell);
        if (jo_vdp2_is_bank_used_by_rotation(bank))
            continue;
        count = jo_vdp2_character_access_count(s);
        first = used[bank] > pattern_name_slot ? used[bank] : pattern_name_slot;
        for (JO_ZERO(i); i < count && first < 4; ++i, ++first)
            slots[bank][first] = JO_VDP2_CYCLE_CHARACTER(n);
        if (first > used[bank])
            used[bank] = first;
        // T0 to T3 full (4 screens in one bank, like the 2 glow layers with the map of NBG1): the characters in T4 to T7,
        // taken from the CPU. In normal resolution the character reads of a pattern name read at Tn can be from Tn+4 to
        // T7 (VDP2 manual, chapter 3: T0 -> T4-T7, T1 -> T5-T7, T2 -> T6-T7, T3 -> T7); in high resolution never. Without
        // a slot the screen was not displayed at all (the second glow, NBG3, never seen in the demo rally)
        if (i < count && !__jo_screen.hires)
        {
            for (first = pattern_name_slot + 4; i < count && first < 8; ++first)
            {
                if (slots[bank][first] != JO_VDP2_CYCLE_CPU)
                    continue;
                slots[bank][first] = JO_VDP2_CYCLE_CHARACTER(n);
                ++i;
            }
        }
    }
    // The vertical cell scroll table of NBG1: one read in T0 to T2 of the bank of the table (the effect is enabled only
    // with this slot, in the same VBLANK)
    JO_ZERO(n);
    if (nbg1_vertical_cell_scroll_table != JO_NULL && (__jo_vdp2_displayed_screens & (1 << JO_VDP2_SCREEN_NBG1)))
    {
        bank = jo_vdp2_bank_of(jo_vdp2_vram_offset(nbg1_vertical_cell_scroll_table));
        if (!jo_vdp2_is_bank_used_by_rotation(bank) && used[bank] < 3)
        {
            slots[bank][used[bank]++] = JO_VDP2_CYCLE_VERTICAL_CELL_SCROLL(JO_VDP2_SCREEN_NBG1);
            n = JO_VDP2_SCRCTL_VERTICAL_CELL_SCROLL << JO_VDP2_SCRCTL_NBG1_SHIFT;
        }
    }
    JO_VDP2_SET_BITS(JO_VDP2_SCRCTL, JO_VDP2_SCRCTL_VERTICAL_CELL_SCROLL << JO_VDP2_SCRCTL_NBG1_SHIFT, n);
    for (JO_ZERO(n); n < 4; ++n)
    {
        jo_vdp2_shadow_set(JO_VDP2_REG_INDEX(JO_VDP2_CYCA0L) + JO_MULT_BY_2(n),
                           (slots[n][0] << 12) | (slots[n][1] << 8) | (slots[n][2] << 4) | slots[n][3]);
        jo_vdp2_shadow_set(JO_VDP2_REG_INDEX(JO_VDP2_CYCA0U) + JO_MULT_BY_2(n),
                           (slots[n][4] << 12) | (slots[n][5] << 8) | (slots[n][6] << 4) | slots[n][7]);
    }
}

/** @brief Set the displayed screens
 *  @param flags Screens (JO_NBG0_SCREEN, etc)
 */
static void                         jo_vdp2_set_displayed_screens(const unsigned int flags)
{
    // (bit 5 of BGON is RBG1, not the back screen: JO_BACK_SCREEN, (1 << 5) in the flags, turned RBG1 on)
    __jo_vdp2_displayed_screens = flags & JO_VDP2_SCROLL_SCREENS_MASK;
    JO_VDP2_SET_BITS(JO_VDP2_BGON, JO_VDP2_BGON_DISPLAY_MASK, (unsigned short)__jo_vdp2_displayed_screens);
    jo_vdp2_update_cycle_patterns();
}

/** @brief Set a screen in cell mode
 *  @param n Screen (JO_VDP2_SCREEN_xxx)
 *  @param color Color count (JO_VDP2_COLOR_xxx)
 *  @param char_size Character size (0: 1x1 cell, 1: 2x2 cells)
 */
static void                         jo_vdp2_set_character_mode(const int n, const unsigned char color, const unsigned char char_size)
{
    __jo_vdp2_screens[n].bitmap = false;
    __jo_vdp2_screens[n].color = color;
    __jo_vdp2_screens[n].char_size = char_size;
    jo_vdp2_update_character_registers(n);
    jo_vdp2_update_pattern_register(n);
    if (n < JO_VDP2_SCREEN_RBG0)
        jo_vdp2_update_map_registers(n);
    else
        jo_vdp2_update_rotation_map_registers();
    jo_vdp2_update_cycle_patterns();
}

/** @brief Set the character pattern table of a screen
 *  @param n Screen (JO_VDP2_SCREEN_xxx)
 *  @param cell Address of the characters in VRAM
 *  @param pattern Pattern name data mode (JO_VDP2_PATTERN_xxx)
 */
static void                         jo_vdp2_set_cell(const int n, const void *cell, const unsigned short pattern)
{
    __jo_vdp2_screens[n].cell = jo_vdp2_vram_offset(cell);
    __jo_vdp2_screens[n].pattern = pattern;
    jo_vdp2_update_pattern_register(n);
    if (n < JO_VDP2_SCREEN_RBG0)
        jo_vdp2_update_map_registers(n);
    else
        jo_vdp2_update_rotation_map_registers();
    jo_vdp2_update_cycle_patterns();
}

/** @brief Set the plane size of a screen
 *  @param n Screen (JO_VDP2_SCREEN_xxx)
 *  @param size 0: 1x1 page, 1: 2x1 pages, 3: 2x2 pages
 */
static void                         jo_vdp2_set_plane_size(const int n, const unsigned char size)
{
    __jo_vdp2_screens[n].plane_size = size & 3;
    JO_VDP2_SET_BITS(JO_VDP2_PLSZ, JO_VDP2_PLANE_SIZE_MASK << JO_MULT_BY_2(n), (size & JO_VDP2_PLANE_SIZE_MASK) << JO_MULT_BY_2(n));
    if (n < JO_VDP2_SCREEN_RBG0)
        jo_vdp2_update_map_registers(n);
    else
        jo_vdp2_update_rotation_map_registers();
}

/** @brief Use the same map for the 4 planes of a normal scroll screen
 *  @param n Screen (JO_VDP2_SCREEN_NBG0 to JO_VDP2_SCREEN_NBG3)
 *  @param map Address of the map in VRAM
 */
static void                         jo_vdp2_set_map(const int n, const void *map)
{
    jo_vdp2_screen                  *s;

    s = &__jo_vdp2_screens[n];
    s->map[0] = jo_vdp2_vram_offset(map);
    s->map[1] = s->map[0];
    s->map[2] = s->map[0];
    s->map[3] = s->map[0];
    jo_vdp2_update_map_registers(n);
    jo_vdp2_update_cycle_patterns();
}

/** @brief Use the same map for the 16 planes of a rotation parameter
 *  @param parameter 0: rotation parameter A, 1: rotation parameter B
 *  @param map Address of the map in VRAM
 */
static void                         jo_vdp2_set_rotation_map(const int parameter, const void *map)
{
    __jo_vdp2_rotation_map[parameter] = jo_vdp2_vram_offset(map);
    jo_vdp2_update_rotation_map_registers();
}

/** @brief Set a screen in bitmap mode
 *  @param n Screen (JO_VDP2_SCREEN_NBG0 or JO_VDP2_SCREEN_NBG1)
 *  @param color Color count (JO_VDP2_COLOR_xxx)
 *  @param size Bitmap size (see JO_VDP2_SIZE)
 *  @param addr Address of the bitmap in VRAM
 */
static void                         jo_vdp2_set_bitmap(const int n, const unsigned char color, const unsigned short size, const void *addr)
{
    jo_vdp2_screen                  *s;

    s = &__jo_vdp2_screens[n];
    s->bitmap = true;
    s->color = color;
    s->bitmap_size = size;
    s->bitmap_addr = jo_vdp2_vram_offset(addr);
    jo_vdp2_update_character_registers(n);
    JO_VDP2_SET_BITS(JO_VDP2_MPOFN, JO_VDP2_MAP_OFFSET_MASK << JO_MULT_BY_4(n), ((s->bitmap_addr / JO_VDP2_BANK_SIZE) & JO_VDP2_MAP_OFFSET_MASK) << JO_MULT_BY_4(n));
    jo_vdp2_update_cycle_patterns();
    if (n == JO_VDP2_SCREEN_NBG1)
        __jo_vdp2_nbg1_bitmap_registers = true;
}

/** @brief Set the back screen color
 *  @param addr Address of the back screen color table in VRAM
 *  @param color Color
 */
static void                         jo_vdp2_set_back_color(void *addr, const jo_color color)
{
    unsigned int                    offset;

    *(volatile unsigned short *)(JO_VDP2_VRAM | jo_vdp2_vram_offset(addr)) = color;
    offset = jo_vdp2_vram_offset(addr) >> 1;
    JO_VDP2_SET(JO_VDP2_BKTAU, (offset >> 16) & 0x7);
    JO_VDP2_SET(JO_VDP2_BKTAL, offset & JO_VDP2_TABLE_ADDRESS_LOW_16BITS_MASK);
}

/** @brief Set the zoom of a normal scroll screen
 *  @param n Screen (JO_VDP2_SCREEN_NBG0 or JO_VDP2_SCREEN_NBG1)
 *  @param x Horizontal coordinate increment (JO_FIXED_1 = no zoom, JO_FIXED_2 = reduction to 1/2)
 *  @param y Vertical coordinate increment
 *  @remarks A reduction needs more VRAM accesses (the cycle patterns are updated)
 */
static void                         jo_vdp2_set_zoom(const int n, jo_fixed x, jo_fixed y)
{
    unsigned short                  ctl;
    jo_fixed                           max;

    __jo_vdp2_screens[n].zoom_x = x;
    __jo_vdp2_screens[n].zoom_y = y;
    if (n == JO_VDP2_SCREEN_NBG1 && __jo_vdp2_nbg1_native)
        x >>= __jo_screen.hires && __jo_screen.interlace;
    else
    {
        x >>= __jo_screen.hires;
        y >>= __jo_screen.interlace;
    }
    max = x > y ? x : y;
    // Reduction above 1:1 needs the reduction enable bits (1/2 or 1/4)
    JO_ZERO(ctl);
    if (max > JO_FIXED_2)
        ctl = 2;
    else if (max > JO_FIXED_1)
        ctl = 1;
    if (n == JO_VDP2_SCREEN_NBG0)
    {
        JO_VDP2_SET(JO_VDP2_ZMXIN0, (x >> 16) & 0x7);
        JO_VDP2_SET(JO_VDP2_ZMXDN0, x & JO_VDP2_FRACTION_MASK);
        JO_VDP2_SET(JO_VDP2_ZMYIN0, (y >> 16) & 0x7);
        JO_VDP2_SET(JO_VDP2_ZMYDN0, y & JO_VDP2_FRACTION_MASK);
        JO_VDP2_SET_BITS(JO_VDP2_ZMCTL, JO_VDP2_ZOOM_REDUCTION_MASK, ctl);
    }
    else
    {
        JO_VDP2_SET(JO_VDP2_ZMXIN1, (x >> 16) & 0x7);
        JO_VDP2_SET(JO_VDP2_ZMXDN1, x & JO_VDP2_FRACTION_MASK);
        JO_VDP2_SET(JO_VDP2_ZMYIN1, (y >> 16) & 0x7);
        JO_VDP2_SET(JO_VDP2_ZMYDN1, y & JO_VDP2_FRACTION_MASK);
        JO_VDP2_SET_BITS(JO_VDP2_ZMCTL, JO_VDP2_ZOOM_REDUCTION_MASK << JO_VDP2_ZMCTL_NBG1_SHIFT, ctl << JO_VDP2_ZMCTL_NBG1_SHIFT);
    }
    jo_vdp2_update_cycle_patterns();
}

/** @brief Set the line scroll table address of a screen
 *  @param n Screen (JO_VDP2_SCREEN_NBG0 or JO_VDP2_SCREEN_NBG1)
 *  @param addr Address of the table in VRAM
 */
static void                         jo_vdp2_set_line_scroll_table(const int n, const void *addr)
{
    unsigned int                    offset;

    offset = jo_vdp2_vram_offset(addr) >> 1;
    if (n == JO_VDP2_SCREEN_NBG0)
    {
        JO_VDP2_SET(JO_VDP2_LSTA0U, (offset >> 16) & 0x7);
        JO_VDP2_SET(JO_VDP2_LSTA0L, offset & JO_VDP2_TABLE_ADDRESS_LOW_MASK);
    }
    else
    {
        JO_VDP2_SET(JO_VDP2_LSTA1U, (offset >> 16) & 0x7);
        JO_VDP2_SET(JO_VDP2_LSTA1L, offset & JO_VDP2_TABLE_ADDRESS_LOW_MASK);
    }
}

/** @brief Set the priority of a screen (Internal usage)
 *  @param screen Screen (JO_NBG0_SCREEN, etc). JO_SPRITE_SCREEN sets the 8 sprite priorities
 *  @param priority Priority (0 = hidden, 7 = top)
 */
void                                __jo_vdp2_set_priority(const jo_scroll_screen screen, const unsigned short priority)
{
    switch (screen)
    {
        case JO_NBG0_SCREEN: JO_VDP2_SET_BITS(JO_VDP2_PRINA, JO_VDP2_PRIORITY_LOW_MASK, priority); break;
        case JO_NBG1_SCREEN: JO_VDP2_SET_BITS(JO_VDP2_PRINA, JO_VDP2_PRIORITY_HIGH_MASK, priority << 8); break;
        case JO_NBG2_SCREEN: JO_VDP2_SET_BITS(JO_VDP2_PRINB, JO_VDP2_PRIORITY_LOW_MASK, priority); break;
        case JO_NBG3_SCREEN: JO_VDP2_SET_BITS(JO_VDP2_PRINB, JO_VDP2_PRIORITY_HIGH_MASK, priority << 8); break;
        case JO_RBG0_SCREEN: JO_VDP2_SET_BITS(JO_VDP2_PRIR, JO_VDP2_PRIORITY_LOW_MASK, priority); break;
        case JO_SPRITE_SCREEN:
            // Same priority for the 8 sprite priority registers
            JO_VDP2_SET_BITS(JO_VDP2_PRISA, JO_VDP2_PRIORITY_BOTH_MASK, JO_VDP2_PRIORITY_PAIR(priority, priority));
            JO_VDP2_SET_BITS(JO_VDP2_PRISB, JO_VDP2_PRIORITY_BOTH_MASK, JO_VDP2_PRIORITY_PAIR(priority, priority));
            JO_VDP2_SET_BITS(JO_VDP2_PRISC, JO_VDP2_PRIORITY_BOTH_MASK, JO_VDP2_PRIORITY_PAIR(priority, priority));
            JO_VDP2_SET_BITS(JO_VDP2_PRISD, JO_VDP2_PRIORITY_BOTH_MASK, JO_VDP2_PRIORITY_PAIR(priority, priority));
            break;
        default: break;
    }
}

/*
████████╗██╗   ██╗
╚══██╔══╝██║   ██║
   ██║   ██║   ██║
   ██║   ╚██╗ ██╔╝
   ██║    ╚████╔╝
   ╚═╝     ╚═══╝
*/

# define JO_VDP2_TV_HRESO               (__jo_screen.width == 352 ? 1 : __jo_screen.width == 640 ? 2 : __jo_screen.width == 704 ? 3 : 0)
# define JO_VDP2_TV_VRESO               (__jo_screen.background_height == 240 ? 1 : __jo_screen.background_height == 256 ? 2 : 0)

/** @brief The TV mode of the screen (display off), the type of the sprites (8 bits in hi-res) and the zoom of NBG0 and NBG1 */
/** @brief The palette of the sprites in hi-res (see jo_core_set_screen_palette()) */
static int                          __jo_vdp2_screen_palette = 0;

static void                         jo_vdp2_set_tv_mode(void)
{
    __jo_vdp2_tvmd = JO_VDP2_TVMD_BORDER_BACK_COLOR | (JO_VDP2_TV_VRESO << JO_VDP2_TVMD_VRESO_SHIFT) | JO_VDP2_TV_HRESO;
    if (__jo_screen.interlace)
        __jo_vdp2_tvmd |= JO_VDP2_TVMD_DOUBLE_DENSITY;
    else if (__jo_screen.single_interlace)
        __jo_vdp2_tvmd |= JO_VDP2_TVMD_SINGLE_DENSITY;
    if (__jo_screen.hires)
        JO_VDP2_SET_BITS(JO_VDP2_SPCTL, JO_VDP2_SPCTL_MIXED_COLOR | JO_VDP2_SPCTL_TYPE_MASK, __jo_vdp2_hires_sprite_type);
    else
        JO_VDP2_SET_BITS(JO_VDP2_SPCTL, JO_VDP2_SPCTL_MIXED_COLOR | JO_VDP2_SPCTL_TYPE_MASK, JO_VDP2_SPCTL_MIXED_COLOR | JO_VDP2_SPCTL_TYPE_3);
    JO_VDP2_SET_BITS(JO_VDP2_CRAOFB, JO_VDP2_CRAOFB_SPRITE_OFFSET_MASK, JO_VDP2_CRAOFB_SPRITE_OFFSET(__jo_screen.hires ? __jo_vdp2_screen_palette : 0));
    jo_vdp2_set_zoom(JO_VDP2_SCREEN_NBG0, __jo_vdp2_screens[JO_VDP2_SCREEN_NBG0].zoom_x, __jo_vdp2_screens[JO_VDP2_SCREEN_NBG0].zoom_y);
    jo_vdp2_set_zoom(JO_VDP2_SCREEN_NBG1, __jo_vdp2_screens[JO_VDP2_SCREEN_NBG1].zoom_x, __jo_vdp2_screens[JO_VDP2_SCREEN_NBG1].zoom_y);
}

/** @brief Turn the display on or off (Internal usage)
 *  @param on true to turn the display on
 */
void                                __jo_vdp2_set_tv_display(const bool on)
{
    JO_VDP2_SET(JO_VDP2_TVMD, on ? (__jo_vdp2_tvmd | JO_VDP2_TVMD_DISPLAY) : __jo_vdp2_tvmd);
}

/*
██████╗  ██████╗ ████████╗ █████╗ ████████╗██╗ ██████╗ ███╗   ██╗
██╔══██╗██╔═══██╗╚══██╔══╝██╔══██╗╚══██╔══╝██║██╔═══██╗████╗  ██║
██████╔╝██║   ██║   ██║   ███████║   ██║   ██║██║   ██║██╔██╗ ██║
██╔══██╗██║   ██║   ██║   ██╔══██║   ██║   ██║██║   ██║██║╚██╗██║
██║  ██║╚██████╔╝   ██║   ██║  ██║   ██║   ██║╚██████╔╝██║ ╚████║
╚═╝  ╚═╝ ╚═════╝    ╚═╝   ╚═╝  ╚═╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝

The plane drawn is the local XY plane of the current 3D matrix. For a screen
pixel (h, v) relative to the screen center, the ray d = (h, v, f) (f is the
projection distance) hits the plane at:
    plane(x, y) = R^-1 d * (t.z / (R^-1 d).z) - t.xy     (t = R^-1 * translation)
The VDP2 computes X = k * (A h + B v - C f) + Xp, so the numerators go into
the rotation parameters A..F and the 1 / depth part goes into the coefficient
table: k(KA) = 128 / (e - 16384) where e is the table entry, and KA (linear
in h and v) is set through KAst, dKAst and dKAx. Entries below 16386 are
marked transparent (coefficient MSB): with the coefficient parameter mode the
VDP2 then displays the parameter B plane instead (used for skies above the horizon).
*/

# define JO_VDP2_RPARAM_SIZE            (0x80)
# define JO_VDP2_KTABLE_ENTRIES         (32640)
# define JO_VDP2_KTABLE_ORIGIN          (16384)
# define JO_VDP2_KTABLE_SCALE           (128)
/** @brief Coefficient table entry: transparent pixel (MSB) */
# define JO_VDP2_KTABLE_TRANSPARENT     (0xFF000000)
/** @brief Coefficient table entry: coefficient bits (signed 8.16) */
# define JO_VDP2_KTABLE_VALUE_MASK      (0x00FFFFFF)
/** @brief KTAOF: coefficient table address offset bits */
# define JO_VDP2_KTAOF_MASK             (0x7)
/** @brief KTAOF value: coefficient table address offset (in 256 KB) */
# define JO_VDP2_KTABLE_ADDRESS_OFFSET(VRAM_OFFSET)  (((VRAM_OFFSET) >> 18) & JO_VDP2_KTAOF_MASK)
/** @brief Rotation parameter values: sign extended, the 6 low bits are not used */
# define JO_VDP2_ROTATION_VALUE_MASK    (0xFFFFFFC0)
/** @brief KTCTL: coefficient table enabled */
# define JO_VDP2_KTCTL_ENABLE           (1 << 0)
/** @brief KTCTL value of a rotation parameter (coefficient table enabled, 2 words coefficients, coefficient mode 0)
 *  @remarks The coefficients are not used as line color screen data: the line color of the transparent lines
 *           (above the horizon) was displayed
 */
# define JO_VDP2_KTCTL_VALUE            (JO_VDP2_KTCTL_ENABLE)
/** @brief KTCTL bits of a rotation parameter (the bits of the parameter B are shifted by 8) */
# define JO_VDP2_KTCTL_PARAMETER_MASK   (0x1F)
/** @brief Size of a plane of the rotation scroll screen in pixels (plane size 1 x 1 page of 64 x 64 cells) */
# define JO_VDP2_PLANE_PIXELS           (512)

typedef struct
{
    unsigned int                    xst;
    unsigned int                    yst;
    unsigned int                    zst;
    unsigned int                    dxst;
    unsigned int                    dyst;
    unsigned int                    dx;
    unsigned int                    dy;
    unsigned int                    matrix[6];
    unsigned short                  px;
    unsigned short                  py;
    unsigned short                  pz;
    unsigned short                  pad0;
    unsigned short                  cx;
    unsigned short                  cy;
    unsigned short                  cz;
    unsigned short                  pad1;
    unsigned int                    mx;
    unsigned int                    my;
    unsigned int                    kx;
    unsigned int                    ky;
    unsigned int                    kast;
    unsigned int                    dkast;
    unsigned int                    dkax;
    unsigned int                    pad2[8];
}                                   jo_vdp2_rotation_parameter;

static jo_vdp2_rotation_parameter   __jo_vdp2_rparams[2];
static volatile bool                __jo_vdp2_rparams_dirty = false;
static unsigned int                 __jo_vdp2_rparams_offset = 0;
static unsigned int                 __jo_vdp2_ktable_offset[2] = {0, 0};

/** @brief Format a value for the rotation parameter table
 *  @param v Fixed number
 *  @return Value with the 6 low bits cleared
 */
static  __jo_force_inline unsigned int  jo_vdp2_rotation_value(const jo_fixed v)
{
    return (((unsigned int)v) & JO_VDP2_ROTATION_VALUE_MASK);
}

/** @brief Set a rotation parameter to the identity (no rotation, no scaling)
 *  @param p Rotation parameter
 *  @param ktable_offset VRAM offset of the coefficient table
 */
static void                         jo_vdp2_set_default_rotation_parameter(jo_vdp2_rotation_parameter *p, const unsigned int ktable_offset)
{
    jo_memset(p, 0, sizeof(*p));
    // The rotation screens are computed for 1 pixel out of 2 in hi-res and for the lines of a field in double density
    // interlace: the screen steps are in pixels of the screen
    p->dyst = __jo_screen.interlace ? JO_FIXED_2 : JO_FIXED_1;
    p->dx = __jo_screen.hires ? JO_FIXED_2 : JO_FIXED_1;
    p->matrix[0] = JO_FIXED_1;
    p->matrix[4] = JO_FIXED_1;
    p->px = p->cx = __jo_screen.width_2;
    p->py = p->cy = __jo_screen.height_2;
    p->pz = p->cz = __jo_vdp1_projection_distance >> 16;
    p->kx = JO_FIXED_1;
    p->ky = JO_FIXED_1;
    p->kast = (ktable_offset >> 2) << 16;
}

/** @brief Copy the modified rotation parameters to the VRAM
 *  @remarks Called during VBLANK-IN
 */
static void                         jo_vdp2_rotation_vblank(void)
{
    if (!__jo_vdp2_rparams_dirty)
        return;
    __jo_vdp2_rparams_dirty = false;
    jo_memcpy((void *)(JO_VDP2_VRAM + __jo_vdp2_rparams_offset), &__jo_vdp2_rparams[0], JO_VDP2_RPARAM_SIZE);
    jo_memcpy((void *)(JO_VDP2_VRAM + __jo_vdp2_rparams_offset + JO_VDP2_RPARAM_SIZE), &__jo_vdp2_rparams[1], JO_VDP2_RPARAM_SIZE);
}

/** @brief Initialize the rotation parameter table
 *  @param addr Address of the table in VRAM (0x80 bytes per parameter)
 */
static void                         jo_vdp2_init_rotation_parameters(void *addr)
{
    unsigned int                    offset;

    __jo_vdp2_rparams_offset = jo_vdp2_vram_offset(addr);
    jo_vdp2_set_default_rotation_parameter(&__jo_vdp2_rparams[0], __jo_vdp2_ktable_offset[0]);
    jo_vdp2_set_default_rotation_parameter(&__jo_vdp2_rparams[1], __jo_vdp2_ktable_offset[1]);
    jo_memcpy((void *)(JO_VDP2_VRAM + __jo_vdp2_rparams_offset), &__jo_vdp2_rparams[0], JO_VDP2_RPARAM_SIZE);
    jo_memcpy((void *)(JO_VDP2_VRAM + __jo_vdp2_rparams_offset + JO_VDP2_RPARAM_SIZE), &__jo_vdp2_rparams[1], JO_VDP2_RPARAM_SIZE);
    offset = __jo_vdp2_rparams_offset >> 1;
    JO_VDP2_SET(JO_VDP2_RPTAU, (offset >> 16) & 0x7);
    JO_VDP2_SET(JO_VDP2_RPTAL, offset & JO_VDP2_TABLE_ADDRESS_LOW_MASK);
}

/** @brief First visible entry of the coefficient table (see jo_vdp2_set_rbg0_far_limit()) */
static unsigned int                 __jo_vdp2_ktable_first_entry = JO_VDP2_KTABLE_ORIGIN + 2;

/** @brief Fill entries of the coefficient table used for the perspective of the rotation scroll screen
 *  @param addr Address of the table in VRAM
 *  @param from First entry
 *  @param to After the last entry
 *  @remarks Entry e gives the coefficient 128 / (e - 16384), entries below 16386 are transparent
 */
static void                         jo_vdp2_make_coefficient_entries(void *addr, const unsigned int from, const unsigned int to)
{
    volatile unsigned int           *table;
    register unsigned int           e;
    unsigned int                    d;
    unsigned int                    q;
    int                             r;

    table = (volatile unsigned int *)(JO_VDP2_VRAM + jo_vdp2_vram_offset(addr));
    for (e = from; e < to && e < __jo_vdp2_ktable_first_entry; ++e)
        table[e] = JO_VDP2_KTABLE_TRANSPARENT;
    // The quotient K / d (K = 128 in 16.16) of a divisor from the quotient q and the remainder r of the divisor before:
    // K = q (d - 1) + r = q d + (r - q), the quotient goes down by K / d^2 (under 3 from d = 2048) when r - q is negative.
    // The division unit (39 cycles) for the first entry and the small divisors: the table took 43 ms
    JO_ZERO(q);
    JO_ZERO(r);
    for (; e < to; ++e)
    {
        d = e - JO_VDP2_KTABLE_ORIGIN;
        if (d < 2048 || q == 0)
        {
            // 32 bits / 32 bits with the division unit (the values are positive)
            JO_SH2_DVSR = d;
            JO_SH2_DVDNT = JO_INT_TO_FIXED(JO_VDP2_KTABLE_SCALE);
            q = (unsigned int)JO_SH2_DVDNTL;
            r = (int)(JO_INT_TO_FIXED(JO_VDP2_KTABLE_SCALE) - q * d);
        }
        else
        {
            r -= (int)q;
            while (r < 0)
            {
                --q;
                r += (int)d;
            }
        }
        table[e] = q & JO_VDP2_KTABLE_VALUE_MASK;
    }
}

/** @brief Fill the whole coefficient table (see jo_vdp2_make_coefficient_entries())
 *  @param addr Address of the table in VRAM
 */
static void                         jo_vdp2_make_coefficient_table(void *addr)
{
    jo_vdp2_make_coefficient_entries(addr, 0, JO_VDP2_KTABLE_ENTRIES);
}

/** @brief Use the coefficient table for both rotation parameters
 *  @param addr Address of the table in VRAM
 */
static void                         jo_vdp2_set_coefficient_table(void *addr)
{
    __jo_vdp2_ktable_offset[0] = jo_vdp2_vram_offset(addr);
    __jo_vdp2_ktable_offset[1] = __jo_vdp2_ktable_offset[0];
    JO_VDP2_SET(JO_VDP2_KTCTL, (JO_VDP2_KTCTL_VALUE << 8) | JO_VDP2_KTCTL_VALUE);
    JO_VDP2_SET_BITS(JO_VDP2_KTAOF, JO_VDP2_KTAOF_MASK | (JO_VDP2_KTAOF_MASK << 8),
                     (JO_VDP2_KTABLE_ADDRESS_OFFSET(__jo_vdp2_ktable_offset[1]) << 8) | JO_VDP2_KTABLE_ADDRESS_OFFSET(__jo_vdp2_ktable_offset[0]));
}

/** @brief 64 bits signed division with the SH-2 division unit
 *  @param n Dividend
 *  @param d Divisor
 *  @return n / d (rounded toward zero, like the C division)
 *  @remarks The division unit is used when the quotient fits in 32 bits (39 cycles), otherwise the libgcc (__divdi3, slow)
 */
static long long                    jo_vdp2_div64(const long long n, const long long d)
{
    long long                       an;
    long long                       ad;

    an = n < 0 ? -n : n;
    ad = d < 0 ? -d : d;
    // |n| < |d| * 2^31: the quotient fits in 32 bits
    if (d != 0 && ad <= 0x7fffffffLL && (an >> 31) < ad)
    {
        JO_SH2_DVSR = (int)d;
        JO_SH2_DVDNTH = (int)(n >> 32);
        JO_SH2_DVDNTL = (int)n;
        return ((long long)(int)JO_SH2_DVDNTL);
    }
    return (n / d);
}

/** @brief Invert the 3x3 part of a matrix
 *  @param m Matrix (12 values, axes then translation)
 *  @param inv Inverted matrix (output)
 *  @return false if the matrix cannot be inverted
 */
static bool                         jo_vdp2_invert_matrix(const jo_fixed *m, jo_fixed inv[3][3])
{
    // m: column major (m[0..2] = image of X, m[3..5] = Y, m[6..8] = Z) -> matrix M[r][c] = m[c * 3 + r]
    long long                       a[3][3];
    long long                       det;
    register int                    r;
    register int                    c;

    for (JO_ZERO(r); r < 3; ++r)
        for (JO_ZERO(c); c < 3; ++c)
            a[r][c] = m[c * 3 + r];
    det = (a[0][0] * ((a[1][1] * a[2][2] - a[1][2] * a[2][1]) >> 16)
         - a[0][1] * ((a[1][0] * a[2][2] - a[1][2] * a[2][0]) >> 16)
         + a[0][2] * ((a[1][0] * a[2][1] - a[1][1] * a[2][0]) >> 16)) >> 16;
    if (det == 0)
        return (false);
    inv[0][0] = (jo_fixed)jo_vdp2_div64(((a[1][1] * a[2][2] - a[1][2] * a[2][1]) >> 16) << 16, det);
    inv[0][1] = (jo_fixed)jo_vdp2_div64(((a[0][2] * a[2][1] - a[0][1] * a[2][2]) >> 16) << 16, det);
    inv[0][2] = (jo_fixed)jo_vdp2_div64(((a[0][1] * a[1][2] - a[0][2] * a[1][1]) >> 16) << 16, det);
    inv[1][0] = (jo_fixed)jo_vdp2_div64(((a[1][2] * a[2][0] - a[1][0] * a[2][2]) >> 16) << 16, det);
    inv[1][1] = (jo_fixed)jo_vdp2_div64(((a[0][0] * a[2][2] - a[0][2] * a[2][0]) >> 16) << 16, det);
    inv[1][2] = (jo_fixed)jo_vdp2_div64(((a[0][2] * a[1][0] - a[0][0] * a[1][2]) >> 16) << 16, det);
    inv[2][0] = (jo_fixed)jo_vdp2_div64(((a[1][0] * a[2][1] - a[1][1] * a[2][0]) >> 16) << 16, det);
    inv[2][1] = (jo_fixed)jo_vdp2_div64(((a[0][1] * a[2][0] - a[0][0] * a[2][1]) >> 16) << 16, det);
    inv[2][2] = (jo_fixed)jo_vdp2_div64(((a[0][0] * a[1][1] - a[0][1] * a[1][0]) >> 16) << 16, det);
    return (true);
}

/** @brief Compute a rotation parameter from the current 3D matrix
 *  @param parameter 0: rotation parameter A, 1: rotation parameter B
 *  @param use_scroll_format_matrix Convert the matrix to the scroll format
 *  @remarks The plane drawn is the local XY plane of the current matrix
 *  @remarks The table is copied to the VRAM during the next VBLANK
 */
static void                         jo_vdp2_set_rotation_matrix(const int parameter, const bool use_scroll_format_matrix)
{
    jo_fixed                           m[12];
    jo_fixed                           inv[3][3];
    jo_fixed                           t[3];
    jo_fixed                           scale;
    jo_fixed                           f;
    jo_fixed                           cx;
    jo_fixed                           cy;
    long long                       q;
    long long                       ka;
    jo_vdp2_rotation_parameter      *p;
    register int                    i;
    unsigned int                    kt;

#ifdef JO_COMPILE_WITH_3D_SUPPORT
    jo_3d_get_matrix(m);
#else
    jo_memset(m, 0, sizeof(m));
    m[0] = JO_FIXED_1;
    m[4] = JO_FIXED_1;
    m[8] = JO_FIXED_1;
#endif
    if (!jo_vdp2_invert_matrix(m, inv))
    {
#ifdef JO_DEBUG
        jo_core_error("The current matrix can't be inverted (null scale?)");
#endif
        return;
    }
    // Plane local translation
    for (JO_ZERO(i); i < 3; ++i)
        t[i] = jo_fixed_mult(inv[i][0], m[9]) + jo_fixed_mult(inv[i][1], m[10]) + jo_fixed_mult(inv[i][2], m[11]);
    if (t[2] == 0)
        t[2] = 1;
    scale = use_scroll_format_matrix ? JO_FIXED_4 : JO_FIXED_2;
    f = __jo_vdp1_projection_distance;
    cx = JO_MULT_BY_65536(__jo_screen.width_2);
    cy = JO_MULT_BY_65536(__jo_screen.height_2);
    p = &__jo_vdp2_rparams[parameter];
    kt = __jo_vdp2_ktable_offset[parameter];
    jo_vdp2_set_default_rotation_parameter(p, kt);
    if (use_scroll_format_matrix)
    {
        p->matrix[0] = jo_vdp2_rotation_value(jo_fixed_mult(scale, inv[0][0]));
        p->matrix[1] = jo_vdp2_rotation_value(jo_fixed_mult(scale, inv[0][1]));
        p->matrix[2] = jo_vdp2_rotation_value(-jo_fixed_mult(scale, inv[0][2]));
        p->matrix[3] = jo_vdp2_rotation_value(jo_fixed_mult(scale, inv[1][0]));
        p->matrix[4] = jo_vdp2_rotation_value(jo_fixed_mult(scale, inv[1][1]));
        p->matrix[5] = jo_vdp2_rotation_value(-jo_fixed_mult(scale, inv[1][2]));
        p->mx = jo_vdp2_rotation_value(-t[0] - cx);
        p->my = jo_vdp2_rotation_value(-t[1] - cy);
    }
    else
    {
        // The matrix is used as is (screen space): rows of the matrix, translation not rotated
        p->matrix[0] = jo_vdp2_rotation_value(jo_fixed_mult(scale, m[0]));
        p->matrix[1] = jo_vdp2_rotation_value(jo_fixed_mult(scale, m[3]));
        p->matrix[2] = jo_vdp2_rotation_value(-jo_fixed_mult(scale, m[6]));
        p->matrix[3] = jo_vdp2_rotation_value(jo_fixed_mult(scale, m[1]));
        p->matrix[4] = jo_vdp2_rotation_value(jo_fixed_mult(scale, m[4]));
        p->matrix[5] = jo_vdp2_rotation_value(-jo_fixed_mult(scale, m[7]));
        p->mx = jo_vdp2_rotation_value(m[9] - cx);
        p->my = jo_vdp2_rotation_value(m[10] - cy);
    }
    // Coefficient table address: e = origin + 128 * scale * (row2 . d) / t.z,
    // with d = (h - cx, v - cy, f) for the first pixel (h = 0, v = 0)
    q = (long long)JO_VDP2_KTABLE_SCALE * scale;
    ka = (long long)jo_fixed_mult(inv[2][0], -cx) + jo_fixed_mult(inv[2][1], -cy) + jo_fixed_mult(inv[2][2], f);
    ka = jo_vdp2_div64(((q * ka) >> 16) << 16, t[2]);
    ka += ((long long)((kt >> 2) + JO_VDP2_KTABLE_ORIGIN)) << 16;
    p->kast = ((unsigned int)ka) & JO_VDP2_ROTATION_VALUE_MASK;
    p->dkast = jo_vdp2_rotation_value((jo_fixed)jo_vdp2_div64((((q * inv[2][1]) >> 16)) << (16 + __jo_screen.interlace), t[2]));
    p->dkax = jo_vdp2_rotation_value((jo_fixed)jo_vdp2_div64((((q * inv[2][0]) >> 16)) << (16 + __jo_screen.hires), t[2]));
    __jo_vdp2_rparams_dirty = true;
}

/*
██╗   ██╗██████╗ ██╗      █████╗ ███╗   ██╗██╗  ██╗
██║   ██║██╔══██╗██║     ██╔══██╗████╗  ██║██║ ██╔╝
██║   ██║██████╔╝██║     ███████║██╔██╗ ██║█████╔╝
╚██╗ ██╔╝██╔══██╗██║     ██╔══██║██║╚██╗██║██╔═██╗
 ╚████╔╝ ██████╔╝███████╗██║  ██║██║ ╚████║██║  ██╗
  ╚═══╝  ╚═════╝ ╚══════╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝
*/

/** @brief Fields displayed black after a change of the screen mode (the frame buffer of the VDP1 has still the previous
 *  format) */
static volatile int                 __jo_vdp2_hidden_fields = 0;

/** @brief VDP2 VBLANK-IN processing (Internal usage)
 *  @remarks Applies the registers and the rotation parameters
 */
/** @brief Set the latch of the HV counters by the external signal (Internal usage, the light guns: see smpc.c)
 *  @param enabled true: HCNT and VCNT are latched by the light sensor of a gun (TVSTAT.EXLTFG), false: by the reads of EXTEN
 *  @remarks Applied at the next VBLANK
 */
void                                __jo_vdp2_set_external_latch(const bool enabled)
{
    if (((JO_VDP2_GET(JO_VDP2_EXTEN) & JO_VDP2_EXTEN_EXLTEN) != 0) != enabled)
        JO_VDP2_SET_BITS(JO_VDP2_EXTEN, JO_VDP2_EXTEN_EXLTEN, enabled ? JO_VDP2_EXTEN_EXLTEN : 0);
}

void                                __jo_vdp2_vblank(void)
{
    if (__jo_vdp2_hidden_fields > 0 && --__jo_vdp2_hidden_fields == 0)
        __jo_vdp2_set_tv_display(true);
    jo_vdp2_commit();
    jo_vdp2_rotation_vblank();
}

/*
██╗███╗   ██╗██╗████████╗
██║████╗  ██║██║╚══██╔══╝
██║██╔██╗ ██║██║   ██║
██║██║╚██╗██║██║   ██║
██║██║ ╚████║██║   ██║
╚═╝╚═╝  ╚═══╝╚═╝   ╚═╝
*/

/** @brief Initialize the VDP2: clear the VRAM and the color RAM, set the default registers (Internal usage)
 *  @remarks The display is off after this call
 */
void                                __jo_vdp2_init_registers(void)
{
    register unsigned int           i;

    // Display off while we initialize the video hardware
    JO_VDP2_TVMD = 0;
    // Clear VRAM and color RAM (by the CPU: the DMA controller took 50 ms instead of 30 ms)
    for (JO_ZERO(i); i < JO_VDP2_VRAM_SIZE; i += 4)
        *(volatile unsigned int *)(JO_VDP2_VRAM + i) = 0;
    for (JO_ZERO(i); i < JO_VDP2_CRAM_SIZE; i += 4)
        *(volatile unsigned int *)(JO_VDP2_CRAM + i) = 0;
    for (JO_ZERO(i); i < JO_VDP2_REG_COUNT; ++i)
        JO_ZERO(__jo_vdp2_shadow[i]);
    for (JO_ZERO(i); i < JO_VDP2_SCREEN_COUNT; ++i)
    {
        jo_memset(&__jo_vdp2_screens[i], 0, sizeof(__jo_vdp2_screens[i]));
        __jo_vdp2_screens[i].pattern = JO_VDP2_PATTERN_1WORD | JO_VDP2_PATTERN_12BIT;
        __jo_vdp2_screens[i].zoom_x = JO_FIXED_1;
        __jo_vdp2_screens[i].zoom_y = JO_FIXED_1;
    }
    JO_ZERO(__jo_vdp2_rotation_map[0]);
    JO_ZERO(__jo_vdp2_rotation_map[1]);
    JO_ZERO(__jo_vdp2_displayed_screens);
    for (JO_ZERO(i); i < (sizeof(__jo_vdp2_dirty) / sizeof(*__jo_vdp2_dirty)); ++i)
        __jo_vdp2_dirty[i] = ~0u;
    // Color RAM mode 1 (2048 colors, RGB 555), VRAM banks A and B split.
    // Rotation scroll data banks: A0 = character, A1 = coefficient table, B0 = pattern name
    JO_VDP2_SET(JO_VDP2_RAMCTL, JO_VDP2_RAMCTL_CRAM_MODE_1 | JO_VDP2_RAMCTL_SPLIT_BANK_A | JO_VDP2_RAMCTL_SPLIT_BANK_B |
                                JO_VDP2_RAMCTL_ROTATION_DATA(0, JO_VDP2_ROTATION_DATA_CHARACTER) |
                                JO_VDP2_RAMCTL_ROTATION_DATA(1, JO_VDP2_ROTATION_DATA_COEFFICIENT) |
                                JO_VDP2_RAMCTL_ROTATION_DATA(2, JO_VDP2_ROTATION_DATA_PATTERN_NAME));
    JO_VDP2_SET(JO_VDP2_EXTEN, 0);
    JO_VDP2_SET(JO_VDP2_VRSIZE, 0);
    // Zoom 1:1
    JO_VDP2_SET(JO_VDP2_ZMXIN0, 1);
    JO_VDP2_SET(JO_VDP2_ZMYIN0, 1);
    JO_VDP2_SET(JO_VDP2_ZMXIN1, 1);
    JO_VDP2_SET(JO_VDP2_ZMYIN1, 1);
    // Window 0 and 1 cover the whole screen
    JO_VDP2_SET(JO_VDP2_WPEX0, JO_VDP2_WINDOW_END_X);
    JO_VDP2_SET(JO_VDP2_WPEY0, JO_VDP2_WINDOW_END_Y);
    JO_VDP2_SET(JO_VDP2_WPEX1, JO_VDP2_WINDOW_END_X);
    JO_VDP2_SET(JO_VDP2_WPEY1, JO_VDP2_WINDOW_END_Y);
    // Sprite type 3, mixed palette / RGB sprites
    JO_VDP2_SET(JO_VDP2_SPCTL, JO_VDP2_SPCTL_MIXED_COLOR | JO_VDP2_SPCTL_TYPE_3);
    // Default priorities: NBG0 > sprites > RBG0 > NBG1 > NBG2 > NBG3
    JO_VDP2_SET(JO_VDP2_PRISA, JO_VDP2_PRIORITY_PAIR(5, 6));
    JO_VDP2_SET(JO_VDP2_PRISB, JO_VDP2_PRIORITY_PAIR(5, 5));
    JO_VDP2_SET(JO_VDP2_PRISC, JO_VDP2_PRIORITY_PAIR(5, 5));
    JO_VDP2_SET(JO_VDP2_PRISD, JO_VDP2_PRIORITY_PAIR(5, 5));
    JO_VDP2_SET(JO_VDP2_PRINA, JO_VDP2_PRIORITY_PAIR(3, 7));
    JO_VDP2_SET(JO_VDP2_PRINB, JO_VDP2_PRIORITY_PAIR(1, 2));
    JO_VDP2_SET(JO_VDP2_PRIR, JO_VDP2_PRIORITY_PAIR(0, 4));
    // All screens: 1 word pattern name data, 12 bits character number
    for (JO_ZERO(i); i < JO_VDP2_SCREEN_COUNT; ++i)
        jo_vdp2_update_pattern_register(i);
    // RBG0 256 colors
    __jo_vdp2_screens[JO_VDP2_SCREEN_RBG0].color = JO_VDP2_COLOR_256;
    jo_vdp2_update_character_registers(JO_VDP2_SCREEN_RBG0);
    jo_vdp2_update_cycle_patterns();
    // TV mode (display off). BDCLMD: border uses the back screen color
    jo_vdp2_set_tv_mode();
    JO_VDP2_SET(JO_VDP2_TVMD, __jo_vdp2_tvmd);
    jo_vdp2_flush_now();
}


#ifdef JO_COMPILE_WITH_PRINTF_SUPPORT

/*
████████╗███████╗██╗  ██╗████████╗
╚══██╔══╝██╔════╝╚██╗██╔╝╚══██╔══╝
   ██║   █████╗   ╚███╔╝    ██║
   ██║   ██╔══╝   ██╔██╗    ██║
   ██║   ███████╗██╔╝ ██╗   ██║
   ╚═╝   ╚══════╝╚═╝  ╚═╝   ╚═╝

jo_printf() output on NBG0: 64x64 cells map, 8x8 pixels font, 16 colors cells.
Each glyph pixel uses color 1 of the current 16 colors palette (jo_set_printf_color_index()).
The 16 colors palettes 0 to 7 are in the first 256 colors of the color RAM, which is not used by jo_create_palette().
*/

/** @brief Glyph pixel color (in the 16 colors palette) */
# define JO_VDP2_PRINTF_GLYPH_COLOR     (1)
/** @brief Outline pixel color (in the 16 colors palette, see jo_set_printf_outline()) */
# define JO_VDP2_PRINTF_OUTLINE_COLOR   (2)

# define JO_VDP2_PRINTF_MAP             ((volatile unsigned short *)(JO_VDP2_VRAM + JO_VDP2_PRINTF_MAP_OFFSET))
# define JO_VDP2_PRINTF_CHARACTER_BASE  ((JO_VDP2_PRINTF_CELL_OFFSET >> 5) & JO_VDP2_CHARACTER_NUMBER_MASK)

unsigned short                      __jo_vdp2_printf_palette = 0;

/* 8x8 font, ASCII 0 to 127 */
static const unsigned char          __jo_vdp2_printf_font[128 * 8] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x81, 0xA5, 0x81, 0xBD, 0x99, 0x81, 0x7E,
    0x7E, 0xFF, 0xDB, 0xFF, 0xC3, 0xE7, 0xFF, 0x7E, 0x6C, 0xFE, 0xFE, 0xFE, 0x7C, 0x38, 0x10, 0x00,
    0x10, 0x38, 0x7C, 0xFE, 0x7C, 0x38, 0x10, 0x00, 0x38, 0x7C, 0x38, 0xFE, 0xFE, 0x7C, 0x38, 0x7C,
    0x10, 0x10, 0x38, 0x7C, 0xFE, 0x7C, 0x38, 0x7C, 0x00, 0x00, 0x18, 0x3C, 0x3C, 0x18, 0x00, 0x00,
    0xFF, 0xFF, 0xE7, 0xC3, 0xC3, 0xE7, 0xFF, 0xFF, 0x00, 0x3C, 0x66, 0x42, 0x42, 0x66, 0x3C, 0x00,
    0xFF, 0xC3, 0x99, 0xBD, 0xBD, 0x99, 0xC3, 0xFF, 0x0F, 0x07, 0x0F, 0x7D, 0xCC, 0xCC, 0xCC, 0x78,
    0x3C, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x7E, 0x18, 0x3F, 0x33, 0x3F, 0x30, 0x30, 0x70, 0xF0, 0xE0,
    0x7F, 0x63, 0x7F, 0x63, 0x63, 0x67, 0xE6, 0xC0, 0x99, 0x5A, 0x3C, 0xE7, 0xE7, 0x3C, 0x5A, 0x99,
    0x80, 0xE0, 0xF8, 0xFE, 0xF8, 0xE0, 0x80, 0x00, 0x02, 0x0E, 0x3E, 0xFE, 0x3E, 0x0E, 0x02, 0x00,
    0x18, 0x3C, 0x7E, 0x18, 0x18, 0x7E, 0x3C, 0x18, 0x66, 0x66, 0x66, 0x66, 0x66, 0x00, 0x66, 0x00,
    0x7F, 0xDB, 0xDB, 0x7B, 0x1B, 0x1B, 0x1B, 0x00, 0x3E, 0x63, 0x38, 0x6C, 0x6C, 0x38, 0xCC, 0x78,
    0x00, 0x00, 0x00, 0x00, 0x7E, 0x7E, 0x7E, 0x00, 0x18, 0x3C, 0x7E, 0x18, 0x7E, 0x3C, 0x18, 0xFF,
    0x18, 0x3C, 0x7E, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x3C, 0x18, 0x00,
    0x00, 0x18, 0x0C, 0xFE, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x30, 0x60, 0xFE, 0x60, 0x30, 0x00, 0x00,
    0x00, 0x00, 0xC0, 0xC0, 0xC0, 0xFE, 0x00, 0x00, 0x00, 0x24, 0x66, 0xFF, 0x66, 0x24, 0x00, 0x00,
    0x00, 0x18, 0x3C, 0x7E, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x7E, 0x3C, 0x18, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x78, 0x78, 0x78, 0x30, 0x00, 0x30, 0x00,
    0x6C, 0x6C, 0x6C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6C, 0x6C, 0xFE, 0x6C, 0xFE, 0x6C, 0x6C, 0x00,
    0x30, 0x7C, 0xC0, 0x78, 0x0C, 0xF8, 0x30, 0x00, 0x00, 0xC6, 0xCC, 0x18, 0x30, 0x66, 0xC6, 0x00,
    0x38, 0x6C, 0x38, 0x76, 0xDC, 0xCC, 0x76, 0x00, 0x60, 0x60, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x18, 0x30, 0x60, 0x60, 0x60, 0x30, 0x18, 0x00, 0x60, 0x30, 0x18, 0x18, 0x18, 0x30, 0x60, 0x00,
    0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00, 0x00, 0x30, 0x30, 0xFC, 0x30, 0x30, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x60, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x00, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x80, 0x00,
    0x7C, 0xC6, 0xCE, 0xDE, 0xF6, 0xE6, 0x7C, 0x00, 0x30, 0x70, 0x30, 0x30, 0x30, 0x30, 0xFC, 0x00,
    0x78, 0xCC, 0x0C, 0x38, 0x60, 0xCC, 0xFC, 0x00, 0x78, 0xCC, 0x0C, 0x38, 0x0C, 0xCC, 0x78, 0x00,
    0x1C, 0x3C, 0x6C, 0xCC, 0xFE, 0x0C, 0x1E, 0x00, 0xFC, 0xC0, 0xF8, 0x0C, 0x0C, 0xCC, 0x78, 0x00,
    0x38, 0x60, 0xC0, 0xF8, 0xCC, 0xCC, 0x78, 0x00, 0xFC, 0xCC, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00,
    0x78, 0xCC, 0xCC, 0x78, 0xCC, 0xCC, 0x78, 0x00, 0x78, 0xCC, 0xCC, 0x7C, 0x0C, 0x18, 0x70, 0x00,
    0x00, 0x30, 0x30, 0x00, 0x00, 0x30, 0x30, 0x00, 0x00, 0x30, 0x30, 0x00, 0x00, 0x30, 0x30, 0x60,
    0x18, 0x30, 0x60, 0xC0, 0x60, 0x30, 0x18, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x00, 0xFC, 0x00, 0x00,
    0x60, 0x30, 0x18, 0x0C, 0x18, 0x30, 0x60, 0x00, 0x78, 0xCC, 0x0C, 0x18, 0x30, 0x00, 0x30, 0x00,
    0x7C, 0xC6, 0xDE, 0xDE, 0xDE, 0xC0, 0x78, 0x00, 0x30, 0x78, 0xCC, 0xCC, 0xFC, 0xCC, 0xCC, 0x00,
    0xFC, 0x66, 0x66, 0x7C, 0x66, 0x66, 0xFC, 0x00, 0x3C, 0x66, 0xC0, 0xC0, 0xC0, 0x66, 0x3C, 0x00,
    0xF8, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0xF8, 0x00, 0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x7E, 0x00,
    0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x60, 0x00, 0x3C, 0x66, 0xC0, 0xC0, 0xCE, 0x66, 0x3E, 0x00,
    0xCC, 0xCC, 0xCC, 0xFC, 0xCC, 0xCC, 0xCC, 0x00, 0x78, 0x30, 0x30, 0x30, 0x30, 0x30, 0x78, 0x00,
    0x1E, 0x0C, 0x0C, 0x0C, 0xCC, 0xCC, 0x78, 0x00, 0xE6, 0x66, 0x6C, 0x78, 0x6C, 0x66, 0xE6, 0x00,
    0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00, 0xC6, 0xEE, 0xFE, 0xFE, 0xD6, 0xC6, 0xC6, 0x00,
    0xC6, 0xE6, 0xF6, 0xDE, 0xCE, 0xC6, 0xC6, 0x00, 0x38, 0x6C, 0xC6, 0xC6, 0xC6, 0x6C, 0x38, 0x00,
    0xFC, 0x66, 0x66, 0x7C, 0x60, 0x60, 0xF0, 0x00, 0x78, 0xCC, 0xCC, 0xCC, 0xDC, 0x78, 0x1C, 0x00,
    0xFC, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0xE6, 0x00, 0x78, 0xCC, 0xE0, 0x70, 0x1C, 0xCC, 0x78, 0x00,
    0xFC, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xFC, 0x00,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x78, 0x30, 0x00, 0xC6, 0xC6, 0xC6, 0xD6, 0xFE, 0xEE, 0xC6, 0x00,
    0xC6, 0xC6, 0x6C, 0x38, 0x38, 0x6C, 0xC6, 0x00, 0xCC, 0xCC, 0xCC, 0x78, 0x30, 0x30, 0x78, 0x00,
    0xFE, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xFE, 0x00, 0x78, 0x60, 0x60, 0x60, 0x60, 0x60, 0x78, 0x00,
    0xC0, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x02, 0x00, 0x78, 0x18, 0x18, 0x18, 0x18, 0x18, 0x78, 0x00,
    0x10, 0x38, 0x6C, 0xC6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
    0x30, 0x30, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x0C, 0x7C, 0xCC, 0x76, 0x00,
    0xE0, 0x60, 0x60, 0x7C, 0x66, 0x66, 0xDC, 0x00, 0x00, 0x00, 0x78, 0xCC, 0xC0, 0xCC, 0x78, 0x00,
    0x1C, 0x0C, 0x0C, 0x7C, 0xCC, 0xCC, 0x76, 0x00, 0x00, 0x00, 0x78, 0xCC, 0xFC, 0xC0, 0x78, 0x00,
    0x38, 0x6C, 0x60, 0xF0, 0x60, 0x60, 0xF0, 0x00, 0x00, 0x00, 0x76, 0xCC, 0xCC, 0x7C, 0x0C, 0xF8,
    0xE0, 0x60, 0x6C, 0x76, 0x66, 0x66, 0xE6, 0x00, 0x30, 0x00, 0x70, 0x30, 0x30, 0x30, 0x78, 0x00,
    0x0C, 0x00, 0x0C, 0x0C, 0x0C, 0xCC, 0xCC, 0x78, 0xE0, 0x60, 0x66, 0x6C, 0x78, 0x6C, 0xE6, 0x00,
    0x70, 0x30, 0x30, 0x30, 0x30, 0x30, 0x78, 0x00, 0x00, 0x00, 0xCC, 0xFE, 0xFE, 0xD6, 0xC6, 0x00,
    0x00, 0x00, 0xF8, 0xCC, 0xCC, 0xCC, 0xCC, 0x00, 0x00, 0x00, 0x78, 0xCC, 0xCC, 0xCC, 0x78, 0x00,
    0x00, 0x00, 0xDC, 0x66, 0x66, 0x7C, 0x60, 0xF0, 0x00, 0x00, 0x76, 0xCC, 0xCC, 0x7C, 0x0C, 0x1E,
    0x00, 0x00, 0xDC, 0x76, 0x66, 0x60, 0xF0, 0x00, 0x00, 0x00, 0x7C, 0xC0, 0x78, 0x0C, 0xF8, 0x00,
    0x10, 0x30, 0x7C, 0x30, 0x30, 0x34, 0x18, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC, 0xCC, 0x76, 0x00,
    0x00, 0x00, 0xCC, 0xCC, 0xCC, 0x78, 0x30, 0x00, 0x00, 0x00, 0xC6, 0xD6, 0xFE, 0xFE, 0x6C, 0x00,
    0x00, 0x00, 0xC6, 0x6C, 0x38, 0x6C, 0xC6, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC, 0x7C, 0x0C, 0xF8,
    0x00, 0x00, 0xFC, 0x98, 0x30, 0x64, 0xFC, 0x00, 0x1C, 0x30, 0x30, 0xE0, 0x30, 0x30, 0x1C, 0x00,
    0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00, 0xE0, 0x30, 0x30, 0x1C, 0x30, 0x30, 0xE0, 0x00,
    0x76, 0xDC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x38, 0x6C, 0xC6, 0xC6, 0xFE, 0x00

};

/** @brief The color of a pixel of a printf character (0: transparent, the glyph color, the outline color)
 *  @param c Character
 *  @param x Column (0: left)
 *  @param y Row (0: top)
 *  @param outline The pixels around the glyph in the outline color (inside the 8x8 cell)
 */
/** @brief A row of a printf character: its 8 pixels of 4 bits (0: transparent, the glyph color, the outline color), the left
 *  pixel in the high bits
 *  @param c Character
 *  @param y Row (0: top)
 *  @param outline The pixels around the glyph (the 8 pixels around each pixel of the glyph, inside the 8x8 cell) in the
 *  outline color
 */
static unsigned int                 jo_vdp2_printf_row(const unsigned int c, const int y, const bool outline)
{
    const unsigned char             *glyph;
    unsigned int                    bits;
    unsigned int                    around;
    unsigned int                    row;
    register int                    x;

    if (c >= 128)
        return (0);
    glyph = &__jo_vdp2_printf_font[JO_MULT_BY_8(c)];
    bits = glyph[y];
    JO_ZERO(around);
    if (outline)
    {
        around = bits | (y > 0 ? glyph[y - 1] : 0) | (y < 7 ? glyph[y + 1] : 0);
        around = (around | (around << 1) | (around >> 1)) & ~bits & 0xFF;
    }
    JO_ZERO(row);
    for (JO_ZERO(x); x < 8; ++x)
        row = (row << 4) | ((bits & (0x80 >> x)) ? JO_VDP2_PRINTF_GLYPH_COLOR : (around & (0x80 >> x)) ? JO_VDP2_PRINTF_OUTLINE_COLOR : 0);
    return (row);
}

/** @brief Write the characters of the printf font in VRAM
 *  @param outline With the outline (see jo_set_printf_outline())
 *  @remarks A row of a character per write (by pixel and by byte: 29 ms at the start of each program, 68 ms with the outline)
 */
static void                         jo_vdp2_write_printf_font(const bool outline)
{
    volatile unsigned int           *cell;
    register unsigned int           c;
    register int                    y;

    // Characters: 256 cells of 8x8 pixels at 4 bits per pixel (2 pixels per byte, left pixel in the high nibble)
    cell = (volatile unsigned int *)(JO_VDP2_VRAM + JO_VDP2_PRINTF_CELL_OFFSET);
    for (JO_ZERO(c); c < 256; ++c)
        for (JO_ZERO(y); y < 8; ++y)
            *cell++ = jo_vdp2_printf_row(c, y, outline);
}

void                                jo_set_printf_outline(const bool enabled, const jo_color color)
{
    register int                    k;

    // (the outline color in the 8 palettes of the printf: the color 2 of each one)
    for (JO_ZERO(k); k < 8; ++k)
        jo_set_palette_register((unsigned short)(JO_VDP2_PRINTF_OUTLINE_COLOR + JO_MULT_BY_16(k)), color);
    jo_vdp2_write_printf_font(enabled);
}

/** @brief Load the printf font and set NBG0 for jo_printf() */
static void                         jo_vdp2_init_printf_font(void)
{
    register unsigned int           c;

    jo_vdp2_write_printf_font(false);
    // Map filled with spaces (a 16 colors cell is 32 bytes: 1 character number)
    for (JO_ZERO(c); c < 64 * 64; ++c)
        JO_VDP2_PRINTF_MAP[c] = JO_VDP2_PRINTF_CHARACTER_BASE + ' ';
    JO_ZERO(__jo_vdp2_printf_palette);
    jo_vdp2_set_character_mode(JO_VDP2_SCREEN_NBG0, JO_VDP2_COLOR_16, 0);
    jo_vdp2_set_plane_size(JO_VDP2_SCREEN_NBG0, 0);
    jo_vdp2_set_cell(JO_VDP2_SCREEN_NBG0, (void *)(JO_VDP2_VRAM + JO_VDP2_PRINTF_CELL_OFFSET), JO_VDP2_PATTERN_1WORD | JO_VDP2_PATTERN_12BIT);
    jo_vdp2_set_map(JO_VDP2_SCREEN_NBG0, (void *)(JO_VDP2_VRAM + JO_VDP2_PRINTF_MAP_OFFSET));
}

/** @brief Print a string on NBG0 (Internal usage)
 *  @param x Horizontal position (cells)
 *  @param y Vertical position (cells)
 *  @param str String
 *  @remarks The text continues on the next line at the end of the line
 */
void                                __jo_vdp2_print(const int x, const int y, const char * str)
{
    volatile unsigned short         *map;
    volatile unsigned short         *end;

    map = JO_VDP2_PRINTF_MAP + JO_MULT_BY_64(y & 63) + (x & 63);
    end = JO_VDP2_PRINTF_MAP + 64 * 64;
    while (*str && map < end)
    {
        *map++ = __jo_vdp2_printf_palette | (JO_VDP2_PRINTF_CHARACTER_BASE + (unsigned char)*str);
        ++str;
    }
}

#endif // JO_COMPILE_WITH_PRINTF_SUPPORT


void                                jo_set_default_background_color(const jo_color background_color)
{
    if (back_color == JO_NULL)
        back_color = jo_vdp2_malloc_autosize(JO_VDP2_RAM_BACK_COLOR);
    jo_vdp2_set_back_color(back_color, background_color);
}

/** @brief Back screen color tables (one color per line), the table displayed changes at each call */
static jo_color                     *back_color_lines[2] = {JO_NULL, JO_NULL};
static int                          back_color_lines_index = 0;

void                                jo_set_background_line_colors(const jo_color * const colors)
{
    jo_color                        *table;
    unsigned int                    offset;

#ifdef JO_DEBUG
    if (colors == JO_NULL)
    {
        jo_core_error("colors is null");
        return ;
    }
#endif
    // 2 tables: the table displayed is not changed during the display
    back_color_lines_index ^= 1;
    if (back_color_lines[back_color_lines_index] == JO_NULL &&
        (back_color_lines[back_color_lines_index] = jo_vdp2_malloc(JO_VDP2_LINE_COLOR_TABLE, JO_TV_HEIGHT * sizeof(jo_color))) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of VDP2 memory");
#endif
        return ;
    }
    table = back_color_lines[back_color_lines_index];
    jo_dma_copy(colors, (void *)(JO_VDP2_VRAM | jo_vdp2_vram_offset(table)), JO_TV_HEIGHT * sizeof(jo_color));
    offset = jo_vdp2_vram_offset(table) >> 1;
    // Bit 15: one color per line (applied during the next VBLANK)
    JO_VDP2_SET(JO_VDP2_BKTAU, JO_VDP2_BKTAU_LINE_MODE | ((offset >> 16) & 0x7));
    JO_VDP2_SET(JO_VDP2_BKTAL, offset & JO_VDP2_TABLE_ADDRESS_LOW_16BITS_MASK);
}

void                            __jo_init_vdp2(const jo_color background_color)
{
    jo_vdp2_malloc_init();
    //NBG0
#ifdef JO_COMPILE_WITH_PRINTF_SUPPORT
    jo_init_nbg0_printf();
    JO_SET_FLAGS(screen_flags, JO_NBG0_SCREEN);
#else
    JO_ZERO(screen_flags);
#endif
    //NBG1: its bitmap at its first use (see __jo_vdp2_reserve_nbg1_bitmap()), or now as before
    __jo_vdp2_nbg1_bitmap_reserved = false;
#ifdef JO_COMPATIBILITY_NBG1_BITMAP_AT_INIT
    nbg1_bitmap = jo_vdp2_malloc(JO_VDP2_RAM_BITMAP_NBG1, JO_VDP2_WIDTH * JO_VDP2_HEIGHT * sizeof(*nbg1_bitmap));
    jo_vdp2_set_bitmap(JO_VDP2_SCREEN_NBG1, JO_VDP2_COLOR_32768, JO_VDP2_SIZE, nbg1_bitmap);
    JO_ADD_FLAG(screen_flags, JO_NBG1_SCREEN);
    __jo_vdp2_nbg1_bitmap_reserved = true;
#endif
    //OTHER
    jo_set_default_background_color(background_color);
    jo_vdp2_set_displayed_screens(screen_flags);
}

static  __jo_force_inline void  __jo_switch_to_bitmap_mode(void)
{
#ifndef JO_COMPILE_WITH_PRINTF_SUPPORT
    if (nbg0_cell != JO_NULL)
    {
        jo_vdp2_free(nbg0_cell);
        nbg0_cell = JO_NULL;
    }
    if (nbg0_map != JO_NULL)
    {
        jo_vdp2_free(nbg0_map);
        nbg0_map = JO_NULL;
    }
#endif
    if (nbg1_cell != JO_NULL)
    {
        jo_vdp2_free(nbg1_cell);
        nbg1_cell = JO_NULL;
    }
    if (nbg1_map != JO_NULL)
    {
        jo_vdp2_free(nbg1_map);
        nbg1_map = JO_NULL;
    }
    if (rbg0_cell_b != JO_NULL)
    {
        jo_vdp2_free(rbg0_cell_b);
        rbg0_cell_b = JO_NULL;
    }
    if (rbg0_cell_a != JO_NULL)
    {
        jo_vdp2_free(rbg0_cell_a);
        rbg0_cell_a = JO_NULL;
    }
    if (rbg0_map_a != JO_NULL)
    {
        jo_vdp2_free(rbg0_map_a);
        rbg0_map_a = JO_NULL;
    }
    if (rbg0_map_a_border != JO_NULL)
    {
        jo_vdp2_free(rbg0_map_a_border);
        rbg0_map_a_border = JO_NULL;
        __jo_vdp2_rotation_island_map = 0;
    }
    if (rbg0_map_b != JO_NULL)
    {
        jo_vdp2_free(rbg0_map_b);
        rbg0_map_b = JO_NULL;
    }
}

static  __jo_force_inline void  __jo_switch_to_8bits_mode(void)
{
    if (nbg1_bitmap != JO_NULL)
    {
        jo_vdp2_free(nbg1_bitmap);
        nbg1_bitmap = JO_NULL;
        // Reserved again (at the start of the bank A0, cleared) before the next drawing in it
        __jo_vdp2_nbg1_bitmap_reserved = false;
    }
}

/*
 ██████╗███████╗██╗             ██╗    ███╗   ███╗ █████╗ ██████╗
██╔════╝██╔════╝██║            ██╔╝    ████╗ ████║██╔══██╗██╔══██╗
██║     █████╗  ██║           ██╔╝     ██╔████╔██║███████║██████╔╝
██║     ██╔══╝  ██║          ██╔╝      ██║╚██╔╝██║██╔══██║██╔═══╝
╚██████╗███████╗███████╗    ██╔╝       ██║ ╚═╝ ██║██║  ██║██║
 ╚═════╝╚══════╝╚══════╝    ╚═╝        ╚═╝     ╚═╝╚═╝  ╚═╝╚═╝
*/

#define CELL_WIDTH              (8)
#define CELL_HEIGHT             (8)
#define CELL_SIZE               (CELL_WIDTH * CELL_HEIGHT)
#define CELL_COUNT_PER_ROW      (64)
#define MAP_LENGTH              (512 * 8)

void                            jo_img_to_vdp2_cells(const jo_img_8bits * const img, const bool vertical_flip, unsigned char *cell)
{
    const unsigned char         *data;
    const unsigned char         *p;
    unsigned int                *words;
    register int                x;
    register int                y;
    register int                c;
    register int                i;
    int                         width;
    int                         height;
    int                         step;

#ifdef JO_DEBUG
        if (img == JO_NULL)
        {
            jo_core_error("img is null");
            return ;
        }
        if (JO_MOD_POW2(img->width, 8) != 0)
        {
            jo_core_error("Image width must be multiple of 8");
            return ;
        }
        if (JO_MOD_POW2(img->height, 8) != 0)
        {
            jo_core_error("Image height must be multiple of 8");
            return ;
        }
#endif
    // Each group of 8 bytes of a cell: a column of the image, 8 rows (the image turned: its columns are the rows of the
    // screen), the columns from the right without vertical flip. Written by 4 pixels (the VRAM: a write of 32 bits costs a
    // write of 8 bits, 148 ms for 512 x 256 pixels written by bytes) when the cells are aligned on 4 bytes
    data = img->data;
    width = img->width;
    height = img->height;
    step = vertical_flip ? 1 : -1;
    words = (unsigned int *)cell;
    for (JO_ZERO(x); x < width; x += CELL_WIDTH)
    {
        for (JO_ZERO(y); y < height; y += CELL_HEIGHT)
        {
            p = data + y * width + (vertical_flip ? x : width - x - 1);
            for (JO_ZERO(c); c < CELL_WIDTH; ++c, p += step)
            {
                if (((unsigned int)cell & 3) == 0)
                {
                    *words++ = ((unsigned int)p[0] << 24) | ((unsigned int)p[width] << 16) | ((unsigned int)p[2 * width] << 8) | p[3 * width];
                    *words++ = ((unsigned int)p[4 * width] << 24) | ((unsigned int)p[5 * width] << 16) | ((unsigned int)p[6 * width] << 8) |
                               p[7 * width];
                }
                else
                    for (JO_ZERO(i); i < CELL_HEIGHT; ++i)
                        cell[(x * height + y * CELL_WIDTH + c * CELL_HEIGHT) + i] = p[i * width];
            }
        }
    }
}


/** @brief Fill the map of an image (repeated in the 64 x 64 cells of the page)
 *  @param img Image (turned clockwise: its columns are the rows of the screen)
 *  @param map Map (64 x 64 entries of 1 word)
 *  @param palette_id Palette
 *  @param map_offset Character number of the first cell of the image
 *  @param blank_cell Without repeat: the cell after the image (index from the first cell), shown out of the image; -1:
 *  the image is repeated
 */
static void                     __jo_create_map_blank(const jo_img_8bits * const img, unsigned short *map, const unsigned short palette_id, const int map_offset, const int blank_cell)
{
    register int                x;
    register int                y;
    register int                x2;
    register int                y2;
    register int                i;
    unsigned short              paloff;

    paloff = JO_MULT_BY_4096(palette_id);
    y = JO_DIV_BY_8(img->height);
    x = JO_DIV_BY_8(img->width);
    JO_ZERO(y2);
    JO_ZERO(x2);
    for (JO_ZERO(i); i < MAP_LENGTH; ++i)
    {
        if (i != 0 && JO_MOD_POW2(i, CELL_COUNT_PER_ROW) == 0)
        {
            ++x2;
            // (each row of the map starts with the first cell of a row of the image: the images whose cells count doesn't
            // divide 64 were displayed scrambled)
            JO_ZERO(y2);
        }
        if (x2 >= x)
            JO_ZERO(x2);
        if (blank_cell >= 0 && (i / CELL_COUNT_PER_ROW >= x || JO_MOD_POW2(i, CELL_COUNT_PER_ROW) >= y))
            *map = (JO_MULT_BY_2(blank_cell) | paloff) + map_offset;
        else
            *map = (JO_MULT_BY_2(y2 + x2 * y) | paloff) + map_offset;
        ++map;
        ++y2;
        if (y2 >= y)
            JO_ZERO(y2);
    }
}

static  __jo_force_inline void  __jo_create_map(const jo_img_8bits * const img, unsigned short *map, const unsigned short palette_id, const int map_offset)
{
    __jo_create_map_blank(img, map, palette_id, map_offset, -1);
}


/*
██████╗ ██████╗  ██████╗  ██████╗
██╔══██╗██╔══██╗██╔════╝ ██╔═████╗
██████╔╝██████╔╝██║  ███╗██║██╔██║
██╔══██╗██╔══██╗██║   ██║████╔╝██║
██║  ██║██████╔╝╚██████╔╝╚██████╔╝
╚═╝  ╚═╝╚═════╝  ╚═════╝  ╚═════╝
*/


void                            jo_vdp2_replace_rbg0_plane_a_8bits_image(jo_img_8bits *img, bool vertical_flip)
{
    jo_img_to_vdp2_cells(img, vertical_flip, rbg0_cell_a);
}

void                            jo_vdp2_replace_rbg0_plane_b_8bits_image(jo_img_8bits *img, bool vertical_flip)
{
    jo_img_to_vdp2_cells(img, vertical_flip, rbg0_cell_b);
}


void                            jo_vdp2_set_rbg0_plane_a_8bits_image(jo_img_8bits *img, int palette_id, bool repeat, bool vertical_flip)
{
    __jo_vdp2_rotation_island_map = 0;
    if (rbg0_map_a == JO_NULL)
        rbg0_map_a = (unsigned short *)jo_vdp2_malloc(JO_VDP2_RAM_MAP_RBG0, JO_VDP2_MAP_SIZE);
    jo_vdp2_set_rotation_map(0, rbg0_map_a);
    JO_VDP2_SET_BITS(JO_VDP2_PLSZ, JO_VDP2_OVER_MODE_MASK << JO_VDP2_PLSZ_RA_OVER_SHIFT,
                     (repeat ? JO_VDP2_OVER_MODE_REPEAT : JO_VDP2_OVER_MODE_TRANSPARENT) << JO_VDP2_PLSZ_RA_OVER_SHIFT);
    // The previous image is replaced (the image size can change)
    if (rbg0_cell_a != JO_NULL)
        jo_vdp2_free(rbg0_cell_a);
    // (the bitmap of NBG1 is freed first: the cells of the rotation screen go to the bank A0, not after it)
    __jo_switch_to_8bits_mode();
    rbg0_cell_a = (unsigned char *)jo_vdp2_malloc(JO_VDP2_RAM_CELL_RBG0, img->width * img->height);
    if (rbg0_map_a == JO_NULL || rbg0_cell_a == JO_NULL)
        return ;
    jo_img_to_vdp2_cells(img, vertical_flip, rbg0_cell_a);
	__jo_create_map(img, rbg0_map_a, palette_id, JO_VDP2_CELL_TO_MAP_OFFSET(rbg0_cell_a));
}

void                            jo_vdp2_set_rbg0_plane_a_8bits_island(jo_img_8bits *img, int palette_id, bool vertical_flip)
{
    unsigned short                  border;
    int                             i;

    jo_vdp2_set_rbg0_plane_a_8bits_image(img, palette_id, true, vertical_flip);
    if (rbg0_map_a == JO_NULL || rbg0_cell_a == JO_NULL)
        return;
    __jo_create_map_blank(img, rbg0_map_a, palette_id, JO_VDP2_CELL_TO_MAP_OFFSET(rbg0_cell_a), 0);
    border = (unsigned short)(JO_MULT_BY_4096(palette_id) + JO_VDP2_CELL_TO_MAP_OFFSET(rbg0_cell_a));
    // The 15 other planes: the border cell only (in the same 512 KB as the map of the image, see MPOFR)
    if (rbg0_map_a_border == JO_NULL)
        rbg0_map_a_border = (unsigned short *)jo_vdp2_malloc(JO_VDP2_RAM_MAP_RBG0, JO_VDP2_MAP_SIZE);
    if (rbg0_map_a_border != JO_NULL)
    {
        for (i = 0; i < MAP_LENGTH; ++i)
            rbg0_map_a_border[i] = border;
        __jo_vdp2_rotation_island_map = jo_vdp2_vram_offset(rbg0_map_a_border);
        jo_vdp2_update_rotation_map_registers();
    }
    JO_VDP2_SET(JO_VDP2_OVPNRA, border);
    JO_VDP2_SET_BITS(JO_VDP2_PLSZ, JO_VDP2_OVER_MODE_MASK << JO_VDP2_PLSZ_RA_OVER_SHIFT,
                     JO_VDP2_OVER_MODE_CHARACTER << JO_VDP2_PLSZ_RA_OVER_SHIFT);
}

void                            jo_vdp2_set_rbg0_plane_b_8bits_image(jo_img_8bits *img, int palette_id, bool repeat, bool vertical_flip)
{
    if (rbg0_map_b == JO_NULL)
        rbg0_map_b = (unsigned short *)jo_vdp2_malloc(JO_VDP2_RAM_MAP_RBG0, JO_VDP2_MAP_SIZE);
    jo_vdp2_set_rotation_map(1, rbg0_map_b);
    rbg0_over_mode_b = repeat ? JO_VDP2_OVER_MODE_REPEAT : JO_VDP2_OVER_MODE_TRANSPARENT;
    JO_VDP2_SET_BITS(JO_VDP2_PLSZ, JO_VDP2_OVER_MODE_MASK << JO_VDP2_PLSZ_RB_OVER_SHIFT, rbg0_over_mode_b << JO_VDP2_PLSZ_RB_OVER_SHIFT);
    // The previous image is replaced (the image size can change)
    if (rbg0_cell_b != JO_NULL)
        jo_vdp2_free(rbg0_cell_b);
    // (the bitmap of NBG1 is freed first: the cells of the rotation screen go to the bank A0, not after it)
    __jo_switch_to_8bits_mode();
    rbg0_cell_b = (unsigned char *)jo_vdp2_malloc(JO_VDP2_RAM_CELL_RBG0, img->width * img->height);
    if (rbg0_map_b == JO_NULL || rbg0_cell_b == JO_NULL)
        return ;
    jo_img_to_vdp2_cells(img, vertical_flip, rbg0_cell_b);
	__jo_create_map(img, rbg0_map_b, palette_id, JO_VDP2_CELL_TO_MAP_OFFSET(rbg0_cell_b));
    // Plane B is displayed where the coefficient of plane A is transparent
    JO_VDP2_SET(JO_VDP2_RPMD, JO_VDP2_ROTATION_COEFFICIENT);
}

void                            jo_vdp2_set_rbg0_far_limit(const int max_zoom_out)
{
    unsigned int                first;
    unsigned int                previous;

    // Coefficient 128 / (e - origin) > max_zoom_out: transparent
    first = JO_VDP2_KTABLE_ORIGIN + (max_zoom_out > 0 ? JO_MAX(2, (JO_VDP2_KTABLE_SCALE + max_zoom_out - 1) / max_zoom_out) : 2);
    if (first == __jo_vdp2_ktable_first_entry)
        return ;
    previous = __jo_vdp2_ktable_first_entry;
    __jo_vdp2_ktable_first_entry = first;
    // Only the entries between the previous and the new limit change (the rest of the table is the same)
    if (rbg0_ktable != JO_NULL)
        jo_vdp2_make_coefficient_entries(rbg0_ktable, JO_MIN(previous, first), JO_MAX(previous, first));
}

void                            jo_vdp2_enable_rbg0(void)
{
    __jo_switch_to_8bits_mode();

    if (rbg0_rtable == JO_NULL)
    {
        rbg0_rtable = jo_vdp2_malloc_autosize(JO_VDP2_RAM_RTABLE);
        if (rbg0_rtable == JO_NULL)
            return;
    }

    jo_vdp2_init_rotation_parameters(rbg0_rtable);

    if (rbg0_ktable == JO_NULL)
    {
        rbg0_ktable = jo_vdp2_malloc_autosize(JO_VDP2_RAM_KTABLE);
        if (rbg0_ktable == JO_NULL)
            return;
    }

    jo_vdp2_make_coefficient_table(rbg0_ktable);
    jo_vdp2_set_coefficient_table(rbg0_ktable);
    jo_vdp2_set_character_mode(JO_VDP2_SCREEN_RBG0, JO_VDP2_COLOR_256, 0);
    // Rotation parameter A only: where plane A is transparent (above the horizon), the screens below are visible.
    // The plane B is used when an image is set (jo_vdp2_set_rbg0_plane_b_8bits_image(), before or after this call)
    JO_VDP2_SET(JO_VDP2_RPMD, rbg0_cell_b != JO_NULL ? JO_VDP2_ROTATION_COEFFICIENT : JO_VDP2_ROTATION_PARAMETER_A);
    jo_vdp2_set_plane_size(JO_VDP2_SCREEN_RBG0, 0);
    // Plane size of the rotation parameter B
    JO_VDP2_SET_BITS(JO_VDP2_PLSZ, JO_VDP2_PLANE_SIZE_MASK << JO_VDP2_PLSZ_RB_PLANE_SIZE_SHIFT, 0);
    jo_vdp2_update_rotation_map_registers();

    // The NBG1 bitmap (512 x 256 pixels from the bank A0) overlaps the coefficient table (bank A1):
    // it is hidden, otherwise the table is displayed where the plane is transparent (above the horizon)
    if (__jo_vdp2_screens[JO_VDP2_SCREEN_NBG1].bitmap)
        JO_REMOVE_FLAG(screen_flags, JO_NBG1_SCREEN);
    JO_ADD_FLAG(screen_flags, JO_RBG0_SCREEN);
    jo_vdp2_set_displayed_screens(screen_flags);
}

void                            jo_vdp2_disable_rbg0(void)
{
    JO_REMOVE_FLAG(screen_flags, JO_RBG0_SCREEN);
    jo_vdp2_set_displayed_screens(screen_flags);
    JO_VDP2_SET(JO_VDP2_RPMD, JO_VDP2_ROTATION_PARAMETER_A);

    // (the VDP2 reads the tables until the registers are written: now, not at the next VBLANK)
    jo_vdp2_flush_now();
    if (rbg0_ktable != JO_NULL)
    {
        jo_vdp2_free(rbg0_ktable);
        rbg0_ktable = JO_NULL;
    }

    if (rbg0_rtable != JO_NULL)
    {
        jo_vdp2_free(rbg0_rtable);
        rbg0_rtable = JO_NULL;
    }
}


/*
███╗   ██╗██████╗  ██████╗  ██████╗
████╗  ██║██╔══██╗██╔════╝ ██╔═████╗
██╔██╗ ██║██████╔╝██║  ███╗██║██╔██║
██║╚██╗██║██╔══██╗██║   ██║████╔╝██║
██║ ╚████║██████╔╝╚██████╔╝╚██████╔╝
╚═╝  ╚═══╝╚═════╝  ╚═════╝  ╚═════╝
*/

#ifdef JO_COMPILE_WITH_PRINTF_SUPPORT
void                     jo_init_nbg0_printf(void)
{
    jo_vdp2_init_printf_font();
    jo_set_printf_palette_color(JO_COLOR_INDEX_White, JO_COLOR_White);
    jo_set_printf_palette_color(JO_COLOR_INDEX_Black, JO_COLOR_Black);
    jo_set_printf_palette_color(JO_COLOR_INDEX_Red, JO_COLOR_Red);
    jo_set_printf_palette_color(JO_COLOR_INDEX_Green, JO_COLOR_Green);
    jo_set_printf_palette_color(JO_COLOR_INDEX_Yellow, JO_COLOR_Yellow);
    jo_set_printf_palette_color(JO_COLOR_INDEX_Blue, JO_COLOR_Blue);
    jo_set_printf_palette_color(JO_COLOR_INDEX_Purple, JO_COLOR_Purple);
    jo_set_printf_color_index(0);
}
#else
void			                jo_vdp2_set_nbg0_8bits_image(jo_img_8bits *img, int palette_id, bool vertical_flip, bool enabled)
{
    __jo_switch_to_8bits_mode();
    if (nbg0_cell != JO_NULL)
    {
        jo_vdp2_free(nbg0_cell);
        nbg0_cell = JO_NULL;
    }
    jo_vdp2_set_plane_size(JO_VDP2_SCREEN_NBG0, 0);
    jo_vdp2_set_character_mode(JO_VDP2_SCREEN_NBG0, JO_VDP2_COLOR_256, 0);
    if (nbg0_map == JO_NULL)
        nbg0_map = (unsigned short *)jo_vdp2_malloc(JO_VDP2_RAM_MAP_NBG0, JO_VDP2_MAP_SIZE);
    jo_vdp2_set_map(JO_VDP2_SCREEN_NBG0, nbg0_map);
    nbg0_cell = (unsigned char *)jo_vdp2_malloc(JO_VDP2_RAM_CELL_NBG0, img->width * img->height);
    if (nbg0_map == JO_NULL || nbg0_cell == JO_NULL)
        return ;
    jo_vdp2_set_cell(JO_VDP2_SCREEN_NBG0, nbg0_cell, JO_VDP2_PATTERN_1WORD | JO_VDP2_PATTERN_12BIT);
    jo_img_to_vdp2_cells(img, vertical_flip, nbg0_cell);
	__jo_create_map(img, nbg0_map, palette_id, JO_VDP2_CELL_TO_MAP_OFFSET(nbg0_cell));
	if (enabled)
    {
        JO_ADD_FLAG(screen_flags, JO_NBG0_SCREEN);
        jo_vdp2_set_displayed_screens(screen_flags);
    }
}
#endif

void                            jo_vdp2_disable_nbg0_line_scroll(void)
{
    // The VDP2 must stop reading the table before it is freed (the registers: written now, not at the next VBLANK)
    JO_VDP2_SET_BITS(JO_VDP2_SCRCTL, JO_VDP2_SCRCTL_NBG0_MASK, 0);
    if (nbg0_scroll_table != JO_NULL)
    {
        jo_vdp2_flush_now();
        jo_vdp2_free(nbg0_scroll_table);
        nbg0_scroll_table = JO_NULL;
    }
}


void                            jo_vdp2_compute_nbg0_line_scroll(unsigned short offset)
{
    if (nbg0_scroll_table == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Call jo_vdp2_enable_nbg0_line_scroll() first");
#endif
        return ;
    }
    jo_vdp2_set_line_scroll_table(JO_VDP2_SCREEN_NBG0, nbg0_scroll_table + offset);
}

int                             *jo_vdp2_enable_nbg0_line_scroll(void)
{
    __jo_switch_to_8bits_mode();
    if (nbg0_scroll_table == JO_NULL)
        nbg0_scroll_table = jo_vdp2_malloc(JO_VDP2_RAM_LINE_SCROLL, JO_NBG0_SCROLL_TABLE_SIZE * sizeof(int));
    if (nbg0_scroll_table == JO_NULL)
        return (JO_NULL);
    JO_VDP2_SET_BITS(JO_VDP2_SCRCTL, JO_VDP2_SCRCTL_NBG0_MASK, JO_VDP2_LINE_SCROLL_HORIZONTAL);
    jo_vdp2_set_line_scroll_table(JO_VDP2_SCREEN_NBG0, nbg0_scroll_table);
    return (nbg0_scroll_table);
}


/*
███╗   ██╗██████╗  ██████╗  ██╗
████╗  ██║██╔══██╗██╔════╝ ███║
██╔██╗ ██║██████╔╝██║  ███╗╚██║
██║╚██╗██║██╔══██╗██║   ██║ ██║
██║ ╚████║██████╔╝╚██████╔╝ ██║
╚═╝  ╚═══╝╚═════╝  ╚═════╝  ╚═╝
*/

void                            jo_vdp2_draw_bitmap_nbg1_line(int x0, int y0, int x1, int y1, const jo_color color)
{
    int                         dx;
    int                         sx;
    int                         dy;
    int                         sy;
    int                         err;
    int                         e2;

    if (!jo_vdp2_nbg1_bitmap_ready())
        return;
    dx = JO_ABS(x1 - x0);
    sx = x0 < x1 ? 1 : -1;
    dy = JO_ABS(y1 - y0);
    sy = y0 < y1 ? 1 : -1;
    if (dx == dy)
        JO_ZERO(err);
    else
        err = JO_DIV_BY_2(dx > dy ? dx : -dy);
    for (;;)
    {
        jo_vdp2_put_pixel_bitmap_nbg1(x0, y0, color);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = err;
        if (e2 > -dx)
        {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy)
        {
            err += dx;
            y0 += sy;
        }
    }
    jo_vdp2_display_nbg1();
}

void                            jo_vdp2_clear_bitmap_nbg1(const jo_color color)
{
    unsigned short              back;

    if (!jo_vdp2_nbg1_bitmap_ready())
        return;
    // The whole bitmap (JO_VDP2_WIDTH x JO_VDP2_HEIGHT pixels) is filled by the DMA controller (2 pixels per transfer), but
    // its last pixel: the back screen color is there (see JO_VDP2_RAM_BACK_COLOR)
    back = *(volatile unsigned short *)(JO_VDP2_VRAM_A1 + JO_VDP2_BANK_SIZE - 2);
    jo_dma_fill32((void *)JO_VDP2_VRAM_A0, ((unsigned int)color << 16) | color, JO_VDP2_WIDTH * JO_VDP2_HEIGHT * sizeof(color));
    *(volatile unsigned short *)(JO_VDP2_VRAM_A1 + JO_VDP2_BANK_SIZE - 2) = back;
    jo_vdp2_display_nbg1();
}


/** @brief Configure NBG1 as the 32768 colors bitmap (nbg1_bitmap)
 *  @remarks Nothing is done if the registers already hold this configuration
 *           (the cycle patterns are computed again by every function that changes the screens)
 */
static void                     jo_vdp2_use_nbg1_bitmap(void)
{
    jo_vdp2_screen              *s;

    s = &__jo_vdp2_screens[JO_VDP2_SCREEN_NBG1];
    if (__jo_vdp2_nbg1_bitmap_registers && s->bitmap && s->color == JO_VDP2_COLOR_32768 &&
        s->bitmap_size == JO_VDP2_SIZE && s->bitmap_addr == jo_vdp2_vram_offset(nbg1_bitmap))
        return;
    jo_vdp2_set_bitmap(JO_VDP2_SCREEN_NBG1, JO_VDP2_COLOR_32768, JO_VDP2_SIZE, nbg1_bitmap);
    if (__jo_vdp2_nbg1_native)
    {
        __jo_vdp2_nbg1_native = false;
        jo_vdp2_set_zoom(JO_VDP2_SCREEN_NBG1, s->zoom_x, s->zoom_y);
    }
}

/** @brief Layout of the 256 colors NBG1 bitmap: 1024 x 256 pixels, 512 x 512 pixels in the 448 to 512 lines modes */
# define JO_VDP2_NATIVE_BITMAP_SIZE     (__jo_screen.interlace ? 0x06 : 0x0A)

void                            jo_vdp2_set_nbg1_16bits_bitmap(void)
{
    if (!jo_vdp2_nbg1_bitmap_ready())
        return;
    jo_vdp2_use_nbg1_bitmap();
    jo_vdp2_display_nbg1();
}

int                             jo_vdp2_get_nbg1_8bits_bitmap_width(void)
{
    return (__jo_screen.interlace ? 512 : 1024);
}

int                             jo_vdp2_get_nbg1_8bits_bitmap_height(void)
{
    return (__jo_screen.interlace ? 512 : 256);
}

/** @brief Registers of the 256 colors NBG1 bitmap for the current screen mode */
static void                     jo_vdp2_use_nbg1_native_bitmap(void)
{
    jo_vdp2_screen              *s;

    s = &__jo_vdp2_screens[JO_VDP2_SCREEN_NBG1];
    __jo_vdp2_nbg1_native = true;
    jo_vdp2_set_bitmap(JO_VDP2_SCREEN_NBG1, JO_VDP2_COLOR_256, JO_VDP2_NATIVE_BITMAP_SIZE, nbg1_bitmap);
    jo_vdp2_set_zoom(JO_VDP2_SCREEN_NBG1, s->zoom_x, s->zoom_y);
}

bool                            jo_vdp2_set_nbg1_8bits_bitmap(const int palette_id)
{
    if (!jo_vdp2_nbg1_bitmap_ready())
        return (false);
    JO_VDP2_SET_BITS(JO_VDP2_BMPNA, JO_VDP2_BMPN_PALETTE_MASK << 8, (palette_id & JO_VDP2_BMPN_PALETTE_MASK) << 8);
    jo_vdp2_use_nbg1_native_bitmap();
    jo_vdp2_display_nbg1();
    return (true);
}

void                            jo_vdp2_clear_bitmap_nbg1_8bits(const unsigned char index)
{
    unsigned short              back;

    if (!__jo_vdp2_nbg1_native)
        return;
    back = *(volatile unsigned short *)(JO_VDP2_VRAM_A1 + JO_VDP2_BANK_SIZE - 2);
    jo_dma_fill32((void *)JO_VDP2_VRAM_A0, index * 0x01010101u, JO_VDP2_WIDTH * JO_VDP2_HEIGHT * sizeof(jo_color));
    *(volatile unsigned short *)(JO_VDP2_VRAM_A1 + JO_VDP2_BANK_SIZE - 2) = back;
}

void                            jo_vdp2_put_pixel_bitmap_nbg1_8bits(const int x, const int y, const unsigned char index)
{
    if (!__jo_vdp2_nbg1_native || (unsigned int)x >= (unsigned int)jo_vdp2_get_nbg1_8bits_bitmap_width() ||
        (unsigned int)y >= (unsigned int)jo_vdp2_get_nbg1_8bits_bitmap_height())
        return;
    ((volatile unsigned char *)JO_VDP2_VRAM_A0)[y * jo_vdp2_get_nbg1_8bits_bitmap_width() + x] = index;
}

void                            jo_vdp2_draw_bitmap_nbg1_8bits_image(const jo_img_8bits * const img, const int x, const int y)
{
    volatile unsigned char      *dst;
    const unsigned char         *src;
    int                         width;
    int                         w;
    int                         h;
    int                         i;
    int                         j;

    if (!__jo_vdp2_nbg1_native || x < 0 || y < 0)
        return;
    width = jo_vdp2_get_nbg1_8bits_bitmap_width();
    w = JO_MIN(img->width, width - x);
    h = JO_MIN(img->height, jo_vdp2_get_nbg1_8bits_bitmap_height() - y);
    for (JO_ZERO(j); j < h; ++j)
    {
        dst = (volatile unsigned char *)JO_VDP2_VRAM_A0 + (y + j) * width + x;
        src = img->data + j * img->width;
        for (JO_ZERO(i); i < w; ++i)
            dst[i] = src[i];
    }
}

bool                            __jo_vdp2_reserve_nbg1_bitmap(void)
{
    unsigned short              back;

    if (__jo_vdp2_nbg1_bitmap_reserved)
        return (true);
    // The bitmap replaces the 8 bits image of NBG1 (see jo_vdp2_set_nbg1_8bits_image())
    if (nbg1_cell != JO_NULL)
    {
        jo_vdp2_free(nbg1_cell);
        nbg1_cell = JO_NULL;
    }
    if (nbg1_map != JO_NULL)
    {
        jo_vdp2_free(nbg1_map);
        nbg1_map = JO_NULL;
    }
    // At the start of the bank A0 (as when jo_core_init() allocated it: the drawing functions write there).
    // Checked before the allocation: it is also used by the error screen, so it must not raise an error itself
    if (!__jo_vdp2_a0_start_is_free(JO_VDP2_WIDTH * JO_VDP2_HEIGHT * sizeof(*nbg1_bitmap)))
        return (false);
    nbg1_bitmap = jo_vdp2_malloc(JO_VDP2_RAM_BITMAP_NBG1, JO_VDP2_WIDTH * JO_VDP2_HEIGHT * sizeof(*nbg1_bitmap));
    if (nbg1_bitmap == JO_NULL)
        return (false);
    __jo_vdp2_nbg1_bitmap_reserved = true;
    // Transparent, as the VRAM at the start (the bank may have held the cells of a screen freed since), but the back screen
    // color in the last pixel
    back = *(volatile unsigned short *)(JO_VDP2_VRAM_A1 + JO_VDP2_BANK_SIZE - 2);
    jo_dma_fill32((void *)JO_VDP2_VRAM_A0, 0, JO_VDP2_WIDTH * JO_VDP2_HEIGHT * sizeof(*nbg1_bitmap));
    *(volatile unsigned short *)(JO_VDP2_VRAM_A1 + JO_VDP2_BANK_SIZE - 2) = back;
    jo_vdp2_set_bitmap(JO_VDP2_SCREEN_NBG1, JO_VDP2_COLOR_32768, JO_VDP2_SIZE, nbg1_bitmap);
    JO_ADD_FLAG(screen_flags, JO_NBG1_SCREEN);
    jo_vdp2_set_displayed_screens(screen_flags);
    return (true);
}

/** @brief The NBG1 bitmap before a drawing in it (see __jo_vdp2_reserve_nbg1_bitmap())
 *  @return false if it can't be allocated
 */
static bool                     jo_vdp2_nbg1_bitmap_ready(void)
{
    if (__jo_vdp2_reserve_nbg1_bitmap())
        return (true);
#ifdef JO_DEBUG
    jo_core_error("NBG1 bitmap: VDP2 bank A0 is used");
#endif
    return (false);
}

/** @brief Display NBG1 (nothing is done if it is already displayed) */
static void                     jo_vdp2_display_nbg1(void)
{
    JO_ADD_FLAG(screen_flags, JO_NBG1_SCREEN);
    if ((screen_flags & JO_VDP2_BGON_DISPLAY_MASK) != __jo_vdp2_displayed_screens)
        jo_vdp2_set_displayed_screens(screen_flags);
}

/** @brief Prepare the NBG1 bitmap for a direct access
 *  @return NBG1 bitmap (JO_VDP2_WIDTH x JO_VDP2_HEIGHT pixels)
 *  @remarks Called for each tile when a map is drawn in the NBG1 bitmap: the screen is not configured again if not needed
 */
static jo_color                 *jo_vdp2_get_nbg1_bitmap(void)
{
    __jo_switch_to_bitmap_mode();
    if (!jo_vdp2_nbg1_bitmap_ready())
        return (JO_NULL);
    jo_vdp2_use_nbg1_bitmap();
    jo_vdp2_display_nbg1();
    return (nbg1_bitmap);
}

/** @brief Copy a line in the NBG1 bitmap (the line wraps around the right border)
 *  @param row First pixel of the bitmap row
 *  @param x Horizontal position (any value, wrapped around JO_VDP2_WIDTH)
 *  @param src Pixels (JO_NULL to fill with color)
 *  @param color Fill color (if src is JO_NULL)
 *  @param len Pixel count (<= JO_VDP2_WIDTH)
 */
static void                     jo_vdp2_nbg1_bitmap_line(jo_color *row, int x, const jo_color *src, const jo_color color, int len)
{
    register int                i;
    int                         part;

    x &= (JO_VDP2_WIDTH - 1);
    while (len > 0)
    {
        part = JO_MIN(len, JO_VDP2_WIDTH - x);
        if (src != JO_NULL)
        {
            jo_memcpy(row + x, src, part * sizeof(*src));
            src += part;
        }
        else
            for (JO_ZERO(i); i < part; ++i)
                row[x + i] = color;
        len -= part;
        JO_ZERO(x);
    }
}

void                            __jo_vdp2_nbg1_bitmap_fill(const int x, const int y, int width, int height, const jo_color color)
{
    jo_color                    *bitmap;
    register int                i;

    bitmap = jo_vdp2_get_nbg1_bitmap();
    if (bitmap == JO_NULL)
        return;
    width = JO_MIN(width, JO_VDP2_WIDTH);
    height = JO_MIN(height, JO_VDP2_HEIGHT);
    for (JO_ZERO(i); i < height; ++i)
        jo_vdp2_nbg1_bitmap_line(bitmap + ((y + i) & (JO_VDP2_HEIGHT - 1)) * JO_VDP2_WIDTH, x, JO_NULL, color, width);
}

void                            __jo_vdp2_nbg1_bitmap_blit(const jo_img * const img, const int src_x, const int src_y, int width, int height, const int x, const int y)
{
    jo_color                    *bitmap;
    register int                i;

    if (img->data == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("img->data is null");
#endif
        return ;
    }
    bitmap = jo_vdp2_get_nbg1_bitmap();
    if (bitmap == JO_NULL)
        return;
    width = JO_MIN(width, JO_VDP2_WIDTH);
    height = JO_MIN(height, JO_VDP2_HEIGHT);
    for (JO_ZERO(i); i < height; ++i)
        jo_vdp2_nbg1_bitmap_line(bitmap + ((y + i) & (JO_VDP2_HEIGHT - 1)) * JO_VDP2_WIDTH, x,
                                 img->data + (src_y + i) * img->width + src_x, 0, width);
}

void			                jo_vdp2_set_nbg1_8bits_image(jo_img_8bits *img, int palette_id, bool vertical_flip)
{
    jo_vdp2_set_nbg1_8bits_image_repeat(img, palette_id, vertical_flip, true);
}

/** @brief The first empty cell (all its pixels transparent) of an image in VRAM
 *  @param cell Cells of the image
 *  @param count Number of cells
 *  @return Index of the cell, -1 if there is none
 */
static int                      jo_vdp2_find_empty_cell(const unsigned char *cell, const int count)
{
    const unsigned int          *p;
    register int                k;
    register int                i;

    for (JO_ZERO(k); k < count; ++k)
    {
        p = (const unsigned int *)(cell + k * CELL_SIZE);
        for (JO_ZERO(i); i < CELL_SIZE / 4 && p[i] == 0; ++i)
            ;
        if (i == CELL_SIZE / 4)
            return (k);
    }
    return (-1);
}

void			                jo_vdp2_set_nbg1_8bits_image_repeat(jo_img_8bits *img, int palette_id, bool vertical_flip, bool repeat)
{
    unsigned int                size;

    __jo_switch_to_8bits_mode();
    if (nbg1_cell != JO_NULL)
    {
        jo_vdp2_free(nbg1_cell);
        nbg1_cell = JO_NULL;
    }
    jo_vdp2_set_plane_size(JO_VDP2_SCREEN_NBG1, 0);
    jo_vdp2_set_character_mode(JO_VDP2_SCREEN_NBG1, JO_VDP2_COLOR_256, 0);
    if (nbg1_map == JO_NULL)
        nbg1_map = (unsigned short *)jo_vdp2_malloc(JO_VDP2_RAM_MAP_NBG1, JO_VDP2_MAP_SIZE);
    jo_vdp2_set_map(JO_VDP2_SCREEN_NBG1, nbg1_map);
    size = img->width * img->height;
    nbg1_cell = (unsigned char *)jo_vdp2_malloc(JO_VDP2_RAM_CELL_NBG1, size);
    if (nbg1_map == JO_NULL || nbg1_cell == JO_NULL)
        return ;
    jo_vdp2_set_cell(JO_VDP2_SCREEN_NBG1, nbg1_cell, JO_VDP2_PATTERN_1WORD | JO_VDP2_PATTERN_12BIT);
    jo_img_to_vdp2_cells(img, vertical_flip, nbg1_cell);
    // (without repeat: an empty cell of the image out of the image)
    __jo_create_map_blank(img, nbg1_map, palette_id, JO_VDP2_CELL_TO_MAP_OFFSET(nbg1_cell),
                          repeat ? -1 : jo_vdp2_find_empty_cell(nbg1_cell, (int)(size / CELL_SIZE)));
    JO_ADD_FLAG(screen_flags, JO_NBG1_SCREEN);
    jo_vdp2_set_displayed_screens(screen_flags);
}


void			                jo_vdp2_set_nbg1_image(const jo_img *const img, short left, short top)
{
    register int                y;
    int                         right;
    int                         len;
    int                         height;
    register jo_color           *vram_ptr;
    register jo_color           *img_ptr;

    if (left >= JO_VDP2_WIDTH || left <= -img->width)
        return ;
    if (top >= JO_VDP2_HEIGHT || top <= -img->height)
        return;
    __jo_switch_to_bitmap_mode();
    if (!jo_vdp2_nbg1_bitmap_ready())
        return;
    jo_vdp2_use_nbg1_bitmap();
    if (img->data == JO_NULL)
        return ;
    img_ptr = img->data;
    JO_ZERO(y);
    if (top != 0)
    {
        if (top < 0)
        {
            y = -top;
            img_ptr += img->width * y;
            JO_ZERO(top);
        }
        vram_ptr = nbg1_bitmap + (JO_VDP2_WIDTH * top);
    }
    else
        vram_ptr = nbg1_bitmap;
    // Last image row + 1 (clipped to the bottom of the bitmap)
    height = JO_MIN(img->height, y + JO_VDP2_HEIGHT - top);
    for (; y < height; ++y)
    {
        // (clipped to the width of the bitmap: an image wider than it wrote the next rows, then the bank B0)
        if (left > 0)
        {
            right = left + img->width;
            len = right > JO_VDP2_WIDTH ? JO_VDP2_WIDTH - left : img->width;
            if (len > 0)
                jo_dma_copy(img_ptr, vram_ptr + left, len * sizeof(*img_ptr));
            vram_ptr += JO_VDP2_WIDTH;
        }
        else if (left < 0)
        {
            len = JO_MIN(img->width + left, JO_VDP2_WIDTH);
            if (len > 0)
                jo_dma_copy(img_ptr - left, vram_ptr, len * sizeof(*img_ptr));
            vram_ptr += JO_VDP2_WIDTH;
        }
        else
        {
            jo_dma_copy(img_ptr, vram_ptr, JO_MIN(img->width, JO_VDP2_WIDTH) * sizeof(*img_ptr));
            vram_ptr += JO_VDP2_WIDTH;
        }
        img_ptr += img->width;
    }
    jo_vdp2_display_nbg1();
}


void                            jo_vdp2_disable_nbg1_line_scroll(void)
{
    // The VDP2 must stop reading the table before it is freed (the registers: written now, not at the next VBLANK)
    JO_VDP2_SET_BITS(JO_VDP2_SCRCTL, JO_VDP2_SCRCTL_NBG0_MASK << JO_VDP2_SCRCTL_NBG1_SHIFT, 0);
    if (nbg1_scroll_table != JO_NULL)
    {
        jo_vdp2_flush_now();
        jo_vdp2_free(nbg1_scroll_table);
        nbg1_scroll_table = JO_NULL;
    }
}

void                            jo_vdp2_compute_nbg1_line_scroll(unsigned short offset)
{
    if (nbg1_scroll_table == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Call jo_vdp2_enable_nbg1_line_scroll() first");
#endif
        return ;
    }
    jo_vdp2_set_line_scroll_table(JO_VDP2_SCREEN_NBG1, nbg1_scroll_table + offset);
}

int                             *jo_vdp2_enable_nbg1_line_scroll_xy(void)
{
    if (nbg1_scroll_table != JO_NULL)
        jo_vdp2_disable_nbg1_line_scroll();
    nbg1_scroll_table = jo_vdp2_malloc(JO_VDP2_RAM_LINE_SCROLL, JO_NBG1_SCROLL_XY_TABLE_LINES * 2 * sizeof(int));
    if (nbg1_scroll_table == JO_NULL)
        return (JO_NULL);
    jo_memset(nbg1_scroll_table, 0, JO_NBG1_SCROLL_XY_TABLE_LINES * 2 * sizeof(int));
    JO_VDP2_SET_BITS(JO_VDP2_SCRCTL, JO_VDP2_SCRCTL_NBG0_MASK << JO_VDP2_SCRCTL_NBG1_SHIFT,
                     (JO_VDP2_LINE_SCROLL_HORIZONTAL | JO_VDP2_LINE_SCROLL_VERTICAL) << JO_VDP2_SCRCTL_NBG1_SHIFT);
    jo_vdp2_set_line_scroll_table(JO_VDP2_SCREEN_NBG1, nbg1_scroll_table);
    jo_vdp2_update_cycle_patterns();
    return (nbg1_scroll_table);
}

int                             *jo_vdp2_enable_nbg1_line_scroll(void)
{
    __jo_switch_to_8bits_mode();
    if (nbg1_scroll_table == JO_NULL)
        nbg1_scroll_table = jo_vdp2_malloc(JO_VDP2_RAM_LINE_SCROLL, JO_NBG1_SCROLL_TABLE_SIZE * sizeof(int));
    if (nbg1_scroll_table == JO_NULL)
        return (JO_NULL);
    JO_VDP2_SET_BITS(JO_VDP2_SCRCTL, JO_VDP2_SCRCTL_NBG0_MASK << JO_VDP2_SCRCTL_NBG1_SHIFT, JO_VDP2_LINE_SCROLL_HORIZONTAL << JO_VDP2_SCRCTL_NBG1_SHIFT);
    jo_vdp2_set_line_scroll_table(JO_VDP2_SCREEN_NBG1, nbg1_scroll_table);
    return (nbg1_scroll_table);
}

int                             *jo_vdp2_enable_nbg1_vertical_cell_scroll(void)
{
    unsigned int                offset;

    if (nbg1_vertical_cell_scroll_table == JO_NULL)
    {
        nbg1_vertical_cell_scroll_table = jo_vdp2_malloc(JO_VDP2_RAM_LINE_SCROLL, JO_NBG1_VERTICAL_CELL_SCROLL_TABLE_SIZE * sizeof(int));
        if (nbg1_vertical_cell_scroll_table == JO_NULL)
            return (JO_NULL);
        jo_memset(nbg1_vertical_cell_scroll_table, 0, JO_NBG1_VERTICAL_CELL_SCROLL_TABLE_SIZE * sizeof(int));
    }
    // (the same format as the line scroll table address)
    offset = jo_vdp2_vram_offset(nbg1_vertical_cell_scroll_table) >> 1;
    JO_VDP2_SET(JO_VDP2_VCSTAU, (offset >> 16) & 0x7);
    JO_VDP2_SET(JO_VDP2_VCSTAL, offset & JO_VDP2_TABLE_ADDRESS_LOW_MASK);
    jo_vdp2_update_cycle_patterns();
    return (nbg1_vertical_cell_scroll_table);
}

void                            jo_vdp2_disable_nbg1_vertical_cell_scroll(void)
{
    void                        *table;

    if (nbg1_vertical_cell_scroll_table == JO_NULL)
        return;
    // (the effect and its slot are removed, written now: the VDP2 stops reading the table before it is freed)
    table = nbg1_vertical_cell_scroll_table;
    nbg1_vertical_cell_scroll_table = JO_NULL;
    jo_vdp2_update_cycle_patterns();
    jo_vdp2_flush_now();
    jo_vdp2_free(table);
}


void			                __jo_set_nbg2_8bits_image(jo_img_8bits *img, bool vertical_flip)
{
    if (nbg2_cell != JO_NULL)
    {
        jo_vdp2_free(nbg2_cell);
        nbg2_cell = JO_NULL;
    }
    jo_vdp2_set_plane_size(JO_VDP2_SCREEN_NBG2, 0);
    jo_vdp2_set_character_mode(JO_VDP2_SCREEN_NBG2, JO_VDP2_COLOR_256, 0);
    if (nbg2_map == JO_NULL)
        nbg2_map = (unsigned short *)jo_vdp2_malloc(JO_VDP2_RAM_MAP_NBG2, JO_VDP2_MAP_SIZE);
    jo_vdp2_set_map(JO_VDP2_SCREEN_NBG2, nbg2_map);
    nbg2_cell = (unsigned char *)jo_vdp2_malloc(JO_VDP2_RAM_CELL_NBG2, img->width * img->height);
    if (nbg2_map == JO_NULL || nbg2_cell == JO_NULL)
        return ;
    jo_vdp2_set_cell(JO_VDP2_SCREEN_NBG2, nbg2_cell, JO_VDP2_PATTERN_1WORD | JO_VDP2_PATTERN_12BIT);
    jo_img_to_vdp2_cells(img, vertical_flip, nbg2_cell);
}

void			                jo_vdp2_set_nbg2_8bits_image(jo_img_8bits *img, int palette_id, bool vertical_flip, bool enabled)
{
    __jo_set_nbg2_8bits_image(img, vertical_flip);
	__jo_create_map(img, nbg2_map, palette_id, JO_VDP2_CELL_TO_MAP_OFFSET(nbg2_cell));
	if (enabled)
    {
        JO_ADD_FLAG(screen_flags, JO_NBG2_SCREEN);
        jo_vdp2_set_displayed_screens(screen_flags);
    }
}

void			                __jo_set_nbg3_8bits_image(jo_img_8bits *img, bool vertical_flip)
{
    if (nbg3_cell != JO_NULL)
    {
        jo_vdp2_free(nbg3_cell);
        nbg3_cell = JO_NULL;
    }
    jo_vdp2_set_plane_size(JO_VDP2_SCREEN_NBG3, 0);
    jo_vdp2_set_character_mode(JO_VDP2_SCREEN_NBG3, JO_VDP2_COLOR_256, 0);
    if (nbg3_map == JO_NULL)
        nbg3_map = (unsigned short *)jo_vdp2_malloc(JO_VDP2_RAM_MAP_NBG3, JO_VDP2_MAP_SIZE);
    jo_vdp2_set_map(JO_VDP2_SCREEN_NBG3, nbg3_map);
    nbg3_cell = (unsigned char *)jo_vdp2_malloc(JO_VDP2_RAM_CELL_NBG3, img->width * img->height);
    if (nbg3_map == JO_NULL || nbg3_cell == JO_NULL)
        return ;
    jo_vdp2_set_cell(JO_VDP2_SCREEN_NBG3, nbg3_cell, JO_VDP2_PATTERN_1WORD | JO_VDP2_PATTERN_12BIT);
    jo_img_to_vdp2_cells(img, vertical_flip, nbg3_cell);
}

void			                jo_vdp2_set_nbg3_8bits_image(jo_img_8bits *img, int palette_id, bool vertical_flip, bool enabled)
{
    __jo_set_nbg3_8bits_image(img, vertical_flip);
	__jo_create_map(img, nbg3_map, palette_id, JO_VDP2_CELL_TO_MAP_OFFSET(nbg3_cell));
	if (enabled)
    {
        JO_ADD_FLAG(screen_flags, JO_NBG3_SCREEN);
        jo_vdp2_set_displayed_screens(screen_flags);
    }
}

#ifndef JO_COMPILE_WITH_PRINTF_SUPPORT

/*
███╗   ██╗██████╗  ██████╗ ██████╗
████╗  ██║██╔══██╗██╔════╝ ╚════██╗
██╔██╗ ██║██████╔╝██║  ███╗ █████╔╝
██║╚██╗██║██╔══██╗██║   ██║██╔═══╝
██║ ╚████║██████╔╝╚██████╔╝███████╗
╚═╝  ╚═══╝╚═════╝  ╚═════╝ ╚══════╝
*/

void                            jo_nbg2_clear(void)
{
    register int                i;

#ifdef JO_DEBUG
    if (nbg2_map == JO_NULL)
    {
        jo_core_error("jo_set_nbg2_8bits_font() required");
        return ;
    }
#endif
    for (JO_ZERO(i); i < MAP_LENGTH; ++i)
        nbg2_map[i] = nbg2_font_paloff + nbg2_font_map_offset;
}


void                            jo_nbg2_print(int x, int y, char * str)
{
    register int                c;
    register unsigned short     cell;

#ifdef JO_DEBUG
    if (nbg2_font_str == JO_NULL)
    {
        jo_core_error("jo_set_nbg2_8bits_font() required");
        return ;
    }
#endif
    x += JO_MULT_BY_64(y);
    // (in the map of 64 x 64 cells, as __jo_vdp2_print(): a text at a line over 63 or near the end of the map was written
    // after the map in the VRAM, over the data after it)
    while (*str != '\0' && x < MAP_LENGTH)
    {
        JO_ZERO(cell);
        for (JO_ZERO(c); nbg2_font_str[c] != '\0'; ++c)
        {
            if (nbg2_font_str[c] == *str)
            {
                cell = c;
                break;
            }
        }
        if (x >= 0)
            nbg2_map[x] = (JO_MULT_BY_2(cell) | nbg2_font_paloff) + nbg2_font_map_offset;
        ++str;
        ++x;
    }
}

void			                jo_vdp2_set_nbg2_8bits_font(jo_img_8bits *img, char *mapping, int palette_id, bool vertical_flip, bool enabled)
{
#ifdef JO_DEBUG
    if (img->width != 8)
    {
        jo_core_error("img width must be 8");
        return ;
    }
#endif
    nbg2_font_str = mapping;
    nbg2_font_paloff = JO_MULT_BY_4096(palette_id);
    __jo_set_nbg2_8bits_image(img, vertical_flip);
    nbg2_font_map_offset = JO_VDP2_CELL_TO_MAP_OFFSET(nbg2_cell);
    jo_nbg2_clear();
    if (enabled)
    {
        JO_ADD_FLAG(screen_flags, JO_NBG2_SCREEN);
        jo_vdp2_set_displayed_screens(screen_flags);
    }
}


/*
███╗   ██╗██████╗  ██████╗ ██████╗
████╗  ██║██╔══██╗██╔════╝ ╚════██╗
██╔██╗ ██║██████╔╝██║  ███╗ █████╔╝
██║╚██╗██║██╔══██╗██║   ██║ ╚═══██╗
██║ ╚████║██████╔╝╚██████╔╝██████╔╝
╚═╝  ╚═══╝╚═════╝  ╚═════╝ ╚═════╝
*/

void                            jo_nbg3_clear(void)
{
    register int                i;

#ifdef JO_DEBUG
    if (nbg3_map == JO_NULL)
    {
        jo_core_error("jo_set_nbg3_8bits_font() required");
        return ;
    }
#endif
    for (JO_ZERO(i); i < MAP_LENGTH; ++i)
        nbg3_map[i] = nbg3_font_paloff + nbg3_font_map_offset;
}


void                            jo_nbg3_print(int x, int y, char * str)
{
    register int                c;
    register unsigned short     cell;

#ifdef JO_DEBUG
    if (nbg3_font_str == JO_NULL)
    {
        jo_core_error("jo_set_nbg3_8bits_font() required");
        return ;
    }
#endif
    x += JO_MULT_BY_64(y);
    // (in the map of 64 x 64 cells, as __jo_vdp2_print(): a text at a line over 63 or near the end of the map was written
    // after the map in the VRAM, over the data after it)
    while (*str != '\0' && x < MAP_LENGTH)
    {
        JO_ZERO(cell);
        for (JO_ZERO(c); nbg3_font_str[c] != '\0'; ++c)
        {
            if (nbg3_font_str[c] == *str)
            {
                cell = c;
                break;
            }
        }
        if (x >= 0)
            nbg3_map[x] = (JO_MULT_BY_2(cell) | nbg3_font_paloff) + nbg3_font_map_offset;
        ++str;
        ++x;
    }
}

void			                jo_vdp2_set_nbg3_8bits_font(jo_img_8bits *img, char *mapping, int palette_id, bool vertical_flip, bool enabled)
{
#ifdef JO_DEBUG
    if (img->width != 8)
    {
        jo_core_error("img width must be 8");
        return ;
    }
#endif
    nbg3_font_str = mapping;
    nbg3_font_paloff = JO_MULT_BY_4096(palette_id);
    __jo_set_nbg3_8bits_image(img, vertical_flip);
    nbg3_font_map_offset = JO_VDP2_CELL_TO_MAP_OFFSET(nbg3_cell);
    jo_nbg3_clear();
    if (enabled)
    {
        JO_ADD_FLAG(screen_flags, JO_NBG3_SCREEN);
        jo_vdp2_set_displayed_screens(screen_flags);
    }
}


#endif

/*
███████╗ ██████╗██████╗  ██████╗ ██╗     ██╗         ███████╗ ██████╗██████╗ ███████╗███████╗███╗   ██╗███████╗
██╔════╝██╔════╝██╔══██╗██╔═══██╗██║     ██║         ██╔════╝██╔════╝██╔══██╗██╔════╝██╔════╝████╗  ██║██╔════╝
███████╗██║     ██████╔╝██║   ██║██║     ██║         ███████╗██║     ██████╔╝█████╗  █████╗  ██╔██╗ ██║███████╗
╚════██║██║     ██╔══██╗██║   ██║██║     ██║         ╚════██║██║     ██╔══██╗██╔══╝  ██╔══╝  ██║╚██╗██║╚════██║
███████║╚██████╗██║  ██║╚██████╔╝███████╗███████╗    ███████║╚██████╗██║  ██║███████╗███████╗██║ ╚████║███████║
╚══════╝ ╚═════╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚══════╝    ╚══════╝ ╚═════╝╚═╝  ╚═╝╚══════╝╚══════╝╚═╝  ╚═══╝╚══════╝
*/


/*
** Glow layers (see glow.c): NBG2 and NBG3, 16 colors characters added to the image below them (the sprites: the 3D view)
*/

/** @brief The registers changed by the glow layers, as they were before (restored by __jo_vdp2_glow_layers_hide()) */
static bool                         __jo_vdp2_glow_layers_shown = false;
static unsigned short               __jo_vdp2_glow_saved_ramctl;
static unsigned short               __jo_vdp2_glow_saved_ccctl;
static unsigned short               __jo_vdp2_glow_saved_prinb;
static unsigned short               __jo_vdp2_glow_saved_craofa;
static bool                         __jo_vdp2_glow_saved_rbg0;

/** @brief Show the glow layers (internal usage, see glow.c)
 *  @param cells Characters (16 colors, in the VRAM bank B0)
 *  @param map2 Map of NBG2 (1 page of 64 x 64 cells, in the bank B0)
 *  @param map3 Map of NBG3, JO_NULL: NBG2 only (NBG3 stays as it is)
 *  @param cram_block Color RAM block of 256 colors of their palettes (0 to 7)
 *  @remarks The rotation screen is hidden (its pattern names are in the bank B0, read by the glow layers instead): the
 *           glows are for the places where the 3D planes are not seen (inside)
 */
void                                __jo_vdp2_glow_layers_show(const void *cells, const void *map2, const void *map3, const int cram_block)
{
    unsigned short                  layers;
    int                             n;

    if (__jo_vdp2_glow_layers_shown)
        return;
    layers = map3 != JO_NULL ? (JO_NBG2_SCREEN | JO_NBG3_SCREEN) : JO_NBG2_SCREEN;
    __jo_vdp2_glow_layers_shown = true;
    __jo_vdp2_glow_saved_ramctl = JO_VDP2_GET(JO_VDP2_RAMCTL);
    __jo_vdp2_glow_saved_ccctl = JO_VDP2_GET(JO_VDP2_CCCTL);
    __jo_vdp2_glow_saved_prinb = JO_VDP2_GET(JO_VDP2_PRINB);
    __jo_vdp2_glow_saved_craofa = JO_VDP2_GET(JO_VDP2_CRAOFA);
    __jo_vdp2_glow_saved_rbg0 = (screen_flags & JO_RBG0_SCREEN) != 0;
    // The bank B0 is not read by the rotation screen (its pattern names): the cycle patterns give its slots to NBG2, NBG3
    JO_REMOVE_FLAG(screen_flags, JO_RBG0_SCREEN);
    JO_VDP2_SET(JO_VDP2_RAMCTL, __jo_vdp2_glow_saved_ramctl & ~JO_VDP2_RAMCTL_ROTATION_DATA(2, 3));
    for (n = JO_VDP2_SCREEN_NBG2; n <= (map3 != JO_NULL ? JO_VDP2_SCREEN_NBG3 : JO_VDP2_SCREEN_NBG2); ++n)
    {
        jo_vdp2_set_character_mode(n, JO_VDP2_COLOR_16, 0);
        jo_vdp2_set_cell(n, cells, JO_VDP2_PATTERN_1WORD | JO_VDP2_PATTERN_12BIT);
        jo_vdp2_set_plane_size(n, 0);
        jo_vdp2_set_map(n, n == JO_VDP2_SCREEN_NBG2 ? map2 : map3);
    }
    // Over the sprites (priority 6), under the text (NBG0, same priority: NBG0 first)
    JO_VDP2_SET(JO_VDP2_PRINB, JO_VDP2_PRIORITY_PAIR(7, 7));
    // Their colors in the block of the color RAM
    JO_VDP2_SET(JO_VDP2_CRAOFA, (__jo_vdp2_glow_saved_craofa & 0x00FF) | ((cram_block & 7) << 8) | ((cram_block & 7) << 12));
    // Color calculation of the layers: added to the image under them
    JO_VDP2_SET(JO_VDP2_CCCTL, (__jo_vdp2_glow_saved_ccctl & ~(JO_VDP2_CCCTL_MODE_MASK | layers)) | JO_VDP2_CCCTL_ADD | layers);
    JO_ADD_FLAG(screen_flags, layers);
    jo_vdp2_set_displayed_screens(screen_flags);
}

/** @brief Hide the glow layers (internal usage, see glow.c): the rotation screen is displayed again */
void                                __jo_vdp2_glow_layers_hide(void)
{
    if (!__jo_vdp2_glow_layers_shown)
        return;
    __jo_vdp2_glow_layers_shown = false;
    JO_REMOVE_FLAG(screen_flags, JO_NBG2_SCREEN | JO_NBG3_SCREEN);
    // (the windows of the glows, see glow.c)
    JO_VDP2_WCTLB = 0;
    JO_VDP2_SET(JO_VDP2_CCCTL, __jo_vdp2_glow_saved_ccctl);
    JO_VDP2_SET(JO_VDP2_PRINB, __jo_vdp2_glow_saved_prinb);
    JO_VDP2_SET(JO_VDP2_CRAOFA, __jo_vdp2_glow_saved_craofa);
    JO_VDP2_SET(JO_VDP2_RAMCTL, __jo_vdp2_glow_saved_ramctl);
    if (__jo_vdp2_glow_saved_rbg0)
        JO_ADD_FLAG(screen_flags, JO_RBG0_SCREEN);
    jo_vdp2_set_displayed_screens(screen_flags);
}

void        jo_set_displayed_screens(const jo_scroll_screen scroll_screen_flags)
{
    screen_flags = (unsigned int)scroll_screen_flags;
    jo_vdp2_set_displayed_screens(screen_flags);
}

void        jo_disable_all_screens_transparency(void)
{
    // Color calculation ratio mode, top image ratio
    JO_VDP2_SET_BITS(JO_VDP2_CCCTL, JO_VDP2_CCCTL_MODE_MASK, 0);
    JO_VDP2_SET_BITS(JO_VDP2_CCCTL, JO_VDP2_ALL_SCREENS_MASK, 0);
    JO_VDP2_SET_BITS(JO_VDP2_BGON, JO_VDP2_SCROLL_SCREENS_MASK << JO_VDP2_BGON_TRANSPARENCY_SHIFT, 0);
}

void        jo_enable_screen_transparency(const jo_scroll_screen screen, const unsigned short transparency_level)
{
    // Color calculation ratio mode, top image ratio
    JO_VDP2_SET_BITS(JO_VDP2_CCCTL, JO_VDP2_CCCTL_MODE_MASK, 0);
    JO_VDP2_SET_BITS(JO_VDP2_CCCTL, JO_VDP2_ALL_SCREENS_MASK, screen & JO_VDP2_ALL_SCREENS_MASK);
    // Color calculation for all sprite priorities (priority number <= 7)
    if (screen & JO_SPRITE_SCREEN)
        JO_VDP2_SET_BITS(JO_VDP2_SPCTL, JO_VDP2_SPCTL_COLOR_CALC_MASK, JO_VDP2_SPCTL_COLOR_CALC_ALL);
    switch (screen)
    {
        case JO_NBG0_SCREEN: JO_VDP2_SET_BITS(JO_VDP2_CCRNA, JO_VDP2_COLOR_RATE_LOW_MASK, transparency_level); break;
        case JO_NBG1_SCREEN: JO_VDP2_SET_BITS(JO_VDP2_CCRNA, JO_VDP2_COLOR_RATE_HIGH_MASK, transparency_level << 8); break;
        case JO_NBG2_SCREEN: JO_VDP2_SET_BITS(JO_VDP2_CCRNB, JO_VDP2_COLOR_RATE_LOW_MASK, transparency_level); break;
        case JO_NBG3_SCREEN: JO_VDP2_SET_BITS(JO_VDP2_CCRNB, JO_VDP2_COLOR_RATE_HIGH_MASK, transparency_level << 8); break;
        case JO_RBG0_SCREEN: JO_VDP2_SET_BITS(JO_VDP2_CCRR, JO_VDP2_COLOR_RATE_LOW_MASK, transparency_level); break;
        case JO_BACK_SCREEN: JO_VDP2_SET_BITS(JO_VDP2_CCRLB, JO_VDP2_COLOR_RATE_HIGH_MASK, transparency_level << 8); break;
        case JO_SPRITE_SCREEN:
            JO_VDP2_SET_BITS(JO_VDP2_CCRSA, JO_VDP2_COLOR_RATE_LOW_MASK | JO_VDP2_COLOR_RATE_HIGH_MASK, (transparency_level << 8) | transparency_level);
            JO_VDP2_SET_BITS(JO_VDP2_CCRSB, JO_VDP2_COLOR_RATE_LOW_MASK | JO_VDP2_COLOR_RATE_HIGH_MASK, (transparency_level << 8) | transparency_level);
            JO_VDP2_SET_BITS(JO_VDP2_CCRSC, JO_VDP2_COLOR_RATE_LOW_MASK | JO_VDP2_COLOR_RATE_HIGH_MASK, (transparency_level << 8) | transparency_level);
            JO_VDP2_SET_BITS(JO_VDP2_CCRSD, JO_VDP2_COLOR_RATE_LOW_MASK | JO_VDP2_COLOR_RATE_HIGH_MASK, (transparency_level << 8) | transparency_level);
            break;
        default: break;
    }
    JO_VDP2_SET_BITS(JO_VDP2_BGON, JO_VDP2_SCROLL_SCREENS_MASK << JO_VDP2_BGON_TRANSPARENCY_SHIFT,
                     (screen & JO_VDP2_SCROLL_SCREENS_MASK) << JO_VDP2_BGON_TRANSPARENCY_SHIFT);
}

/** @brief Priority of the additive sprites (see jo_enable_additive_sprites()): over the rotation screen (4), under the RGB
 *  sprites (6); the priority registers 1 and 2 (5 too) are not used by the engine */
# define JO_VDP2_ADDITIVE_SPRITE_PRIORITY   (5)
/** @brief SPCTL: color calculation of the sprites whose priority is equal to the condition number */
# define JO_VDP2_SPCTL_COLOR_CALC_EQUAL     (1 << 12)

void        jo_enable_additive_sprites(void)
{
    // The register 3 of the sprites (the color bank JO_VDP2_ADDITIVE_SPRITE_BANK)
    JO_VDP2_SET_BITS(JO_VDP2_PRISB, JO_VDP2_PRIORITY_HIGH_MASK, JO_VDP2_PRIORITY_PAIR(JO_VDP2_ADDITIVE_SPRITE_PRIORITY, 0));
    // The color calculation of the sprites of this priority only, mode "add"
    JO_VDP2_SET_BITS(JO_VDP2_SPCTL, JO_VDP2_SPCTL_COLOR_CALC_MASK, JO_VDP2_SPCTL_COLOR_CALC_EQUAL | (JO_VDP2_ADDITIVE_SPRITE_PRIORITY << 8));
    JO_VDP2_SET_BITS(JO_VDP2_CCCTL, JO_VDP2_CCCTL_MODE_MASK | JO_SPRITE_SCREEN, JO_VDP2_CCCTL_ADD | JO_SPRITE_SCREEN);
}

void        jo_disable_additive_sprites(void)
{
    JO_VDP2_SET_BITS(JO_VDP2_CCCTL, JO_VDP2_CCCTL_MODE_MASK | JO_SPRITE_SCREEN, 0);
    JO_VDP2_SET_BITS(JO_VDP2_SPCTL, JO_VDP2_SPCTL_COLOR_CALC_MASK, 0);
}

/*
███╗   ███╗ ██████╗ ██╗   ██╗███████╗     ██╗    ███████╗ ██████╗  ██████╗ ███╗   ███╗
████╗ ████║██╔═══██╗██║   ██║██╔════╝    ██╔╝    ╚══███╔╝██╔═══██╗██╔═══██╗████╗ ████║
██╔████╔██║██║   ██║██║   ██║█████╗     ██╔╝       ███╔╝ ██║   ██║██║   ██║██╔████╔██║
██║╚██╔╝██║██║   ██║╚██╗ ██╔╝██╔══╝    ██╔╝       ███╔╝  ██║   ██║██║   ██║██║╚██╔╝██║
██║ ╚═╝ ██║╚██████╔╝ ╚████╔╝ ███████╗ ██╔╝       ███████╗╚██████╔╝╚██████╔╝██║ ╚═╝ ██║
╚═╝     ╚═╝ ╚═════╝   ╚═══╝  ╚══════╝ ╚═╝        ╚══════╝ ╚═════╝  ╚═════╝ ╚═╝     ╚═╝
*/

void	                        jo_vdp2_move_nbg0(const int x, const int y)
{
    JO_VDP2_SET(JO_VDP2_SCXIN0, x & JO_VDP2_SCROLL_INTEGER_MASK);
    JO_VDP2_SET(JO_VDP2_SCXDN0, 0);
    JO_VDP2_SET(JO_VDP2_SCYIN0, y & JO_VDP2_SCROLL_INTEGER_MASK);
    JO_VDP2_SET(JO_VDP2_SCYDN0, 0);
}

void	                        jo_vdp2_move_nbg1(const int x, const int y)
{
    JO_VDP2_SET(JO_VDP2_SCXIN1, x & JO_VDP2_SCROLL_INTEGER_MASK);
    JO_VDP2_SET(JO_VDP2_SCXDN1, 0);
    JO_VDP2_SET(JO_VDP2_SCYIN1, y & JO_VDP2_SCROLL_INTEGER_MASK);
    JO_VDP2_SET(JO_VDP2_SCYDN1, 0);
}

void	                        jo_vdp2_zoom_nbg0f(const float width_factor, const float height_factor)
{
    jo_vdp2_set_zoom(JO_VDP2_SCREEN_NBG0, JO_FLOAT_TO_FIXED(width_factor), JO_FLOAT_TO_FIXED(height_factor));
}

void	                        jo_vdp2_zoom_nbg1f(const float width_factor, const float height_factor)
{
    jo_vdp2_set_zoom(JO_VDP2_SCREEN_NBG1, JO_FLOAT_TO_FIXED(width_factor), JO_FLOAT_TO_FIXED(height_factor));
}

void                            jo_vdp2_draw_rbg0_plane_a(const bool use_scroll_format_matrix)
{
    jo_vdp2_set_rotation_matrix(0, use_scroll_format_matrix);
}

void                            jo_vdp2_draw_rbg0_plane_b(const bool use_scroll_format_matrix)
{
    // In perspective: the coefficient table and the over mode of the image (see jo_vdp2_draw_rbg0_plane_b_flat())
    JO_VDP2_SET_BITS(JO_VDP2_KTCTL, JO_VDP2_KTCTL_PARAMETER_MASK << 8, JO_VDP2_KTCTL_VALUE << 8);
    JO_VDP2_SET_BITS(JO_VDP2_PLSZ, JO_VDP2_OVER_MODE_MASK << JO_VDP2_PLSZ_RB_OVER_SHIFT, rbg0_over_mode_b << JO_VDP2_PLSZ_RB_OVER_SHIFT);
    jo_vdp2_set_rotation_matrix(1, use_scroll_format_matrix);
}

void                            jo_vdp2_draw_rbg0_plane_b_flat(const int x, const int y)
{
    jo_vdp2_rotation_parameter  *p;

    // No coefficient table: no perspective (and no transparent line)
    JO_VDP2_SET_BITS(JO_VDP2_KTCTL, JO_VDP2_KTCTL_PARAMETER_MASK << 8, 0);
    // Outside of the 4 x 4 planes (above the image): the top left character of the image
    if (rbg0_map_b != JO_NULL)
        JO_VDP2_SET(JO_VDP2_OVPNRB, rbg0_map_b[0]);
    JO_VDP2_SET_BITS(JO_VDP2_PLSZ, JO_VDP2_OVER_MODE_MASK << JO_VDP2_PLSZ_RB_OVER_SHIFT, JO_VDP2_OVER_MODE_CHARACTER << JO_VDP2_PLSZ_RB_OVER_SHIFT);
    // Identity: the pixel (h, v) of the screen shows the point (h + mx, v + my) of the plane. The plane is repeated every
    // 512 pixels horizontally (the image too): mx stays in the second plane of the 4 planes of a row
    p = &__jo_vdp2_rparams[1];
    jo_vdp2_set_default_rotation_parameter(p, __jo_vdp2_ktable_offset[1]);
    p->mx = jo_vdp2_rotation_value(JO_MULT_BY_65536(JO_MOD_POW2(x, JO_VDP2_PLANE_PIXELS) + JO_VDP2_PLANE_PIXELS));
    p->my = jo_vdp2_rotation_value(JO_MULT_BY_65536(y));
    __jo_vdp2_rparams_dirty = true;
}

/*
███╗   ███╗ ██████╗ ███████╗ █████╗ ██╗ ██████╗
████╗ ████║██╔═══██╗╚══███╔╝██╔══██╗██║██╔════╝
██╔████╔██║██║   ██║  ███╔╝ ███████║██║██║
██║╚██╔╝██║██║   ██║ ███╔╝  ██╔══██║██║██║
██║ ╚═╝ ██║╚██████╔╝███████╗██║  ██║██║╚██████╗
╚═╝     ╚═╝ ╚═════╝ ╚══════╝╚═╝  ╚═╝╚═╝ ╚═════╝
*/

void                            jo_enable_screen_mozaic(const jo_scroll_screen screens, short x, short y)
{
    if (x < 1)
        x = 1;
    else if (x > 16)
        x = 16;
    if (y < 1)
        y = 1;
    else if (y > 16)
        y = 16;
    JO_VDP2_SET_BITS(JO_VDP2_MZCTL, JO_VDP2_MOSAIC_SIZE_MASK, JO_VDP2_MOSAIC_SIZE(x, y));
    JO_VDP2_SET_BITS(JO_VDP2_MZCTL, JO_VDP2_SCROLL_SCREENS_MASK, screens & JO_VDP2_SCROLL_SCREENS_MASK);
}

void                            jo_disable_all_screen_mozaic(void)
{
    JO_VDP2_SET_BITS(JO_VDP2_MZCTL, JO_VDP2_MOSAIC_SIZE_MASK | JO_VDP2_SCROLL_SCREENS_MASK, 0);
}

/*
 ██████╗ ██████╗ ██╗      ██████╗ ██████╗     ███████╗██╗██╗  ████████╗███████╗██████╗
██╔════╝██╔═══██╗██║     ██╔═══██╗██╔══██╗    ██╔════╝██║██║  ╚══██╔══╝██╔════╝██╔══██╗
██║     ██║   ██║██║     ██║   ██║██████╔╝    █████╗  ██║██║     ██║   █████╗  ██████╔╝
██║     ██║   ██║██║     ██║   ██║██╔══██╗    ██╔══╝  ██║██║     ██║   ██╔══╝  ██╔══██╗
╚██████╗╚██████╔╝███████╗╚██████╔╝██║  ██║    ██║     ██║███████╗██║   ███████╗██║  ██║
 ╚═════╝ ╚═════╝ ╚══════╝ ╚═════╝ ╚═╝  ╚═╝    ╚═╝     ╚═╝╚══════╝╚═╝   ╚══════╝╚═╝  ╚═╝
*/

void                            jo_set_screen_color_filter_a(const jo_scroll_screen screens, const short r, const short g, const short b)
{
    JO_VDP2_SET(JO_VDP2_COAR, r & JO_VDP2_COLOR_OFFSET_MASK);
    JO_VDP2_SET(JO_VDP2_COAG, g & JO_VDP2_COLOR_OFFSET_MASK);
    JO_VDP2_SET(JO_VDP2_COAB, b & JO_VDP2_COLOR_OFFSET_MASK);
    // Use color offset A and enable color offset
    JO_VDP2_SET_BITS(JO_VDP2_CLOFSL, screens & JO_VDP2_ALL_SCREENS_MASK, 0);
    JO_VDP2_SET_BITS(JO_VDP2_CLOFEN, screens & JO_VDP2_ALL_SCREENS_MASK, screens & JO_VDP2_ALL_SCREENS_MASK);
}

void                            jo_set_screen_color_filter_b(const jo_scroll_screen screens, const short r, const short g, const short b)
{
    JO_VDP2_SET(JO_VDP2_COBR, r & JO_VDP2_COLOR_OFFSET_MASK);
    JO_VDP2_SET(JO_VDP2_COBG, g & JO_VDP2_COLOR_OFFSET_MASK);
    JO_VDP2_SET(JO_VDP2_COBB, b & JO_VDP2_COLOR_OFFSET_MASK);
    // Use color offset B and enable color offset
    JO_VDP2_SET_BITS(JO_VDP2_CLOFSL, screens & JO_VDP2_ALL_SCREENS_MASK, screens & JO_VDP2_ALL_SCREENS_MASK);
    JO_VDP2_SET_BITS(JO_VDP2_CLOFEN, screens & JO_VDP2_ALL_SCREENS_MASK, screens & JO_VDP2_ALL_SCREENS_MASK);
}

void                            jo_disable_screen_color_filter(const jo_scroll_screen screens)
{
    JO_VDP2_SET_BITS(JO_VDP2_CLOFEN, screens & JO_VDP2_ALL_SCREENS_MASK, 0);
}

void                            jo_fade_screens(const jo_scroll_screen screens, const short from, const short to, const int frames)
{
    unsigned int                vblank;
    int                         k;
    short                       level;

    for (k = 0; k <= frames; ++k)
    {
        // (0 frames: to at once)
        level = frames <= 0 ? to : (short)(from + (to - from) * k / frames);
        jo_set_screen_color_filter_a(screens, level, level, level);
        vblank = jo_core_get_vblank_count();
        while (jo_core_get_vblank_count() == vblank)
            ;
    }
}


/*
** END OF FILE
*/

/*
** Windows, color calculation and shadows
*/

/** @brief WCTLA to WCTLC: the bits of a window for a screen (area: 1 = the outside of the window, enable) */
# define JO_VDP2_WINDOW_OUTSIDE_BIT(W)      (1 << ((W) << 1))
# define JO_VDP2_WINDOW_ENABLE_BIT(W)       (2 << ((W) << 1))
# define JO_VDP2_WINDOW_BITS(W)             (3 << ((W) << 1))
/** @brief LWTAnU: line window enable */
# define JO_VDP2_LINE_WINDOW_ENABLE         (1 << 15)
/** @brief Window coordinates (10 bits) */
# define JO_VDP2_WINDOW_COORDINATE_MASK     (0x3FF)
/** @brief SDCTL: the screens that receive the shadows */
# define JO_VDP2_SDCTL_SCREENS_MASK         (0x003F)
/** @brief CCCTL: the screens with a color calculation */
# define JO_VDP2_CCCTL_SCREENS_MASK         (0x007F)
/** @brief CCCTL: extended color calculation (EXCCEN) */
# define JO_VDP2_CCCTL_EXTENDED             (1 << 10)
/** @brief CCCTL: gradation calculation (BOKEN) */
# define JO_VDP2_CCCTL_BLUR                 (1 << 15)
/** @brief CCCTL: screen of the gradation calculation (BOKN) */
# define JO_VDP2_CCCTL_BLUR_SHIFT           (12)
# define JO_VDP2_CCCTL_BLUR_MASK            (0xF000)
/** @brief RAMCTL: color RAM mode bits */
# define JO_VDP2_RAMCTL_CRAM_MODE_MASK      (3 << 12)
/** @brief SFPRMD and SFCCMD: special function by dot (special function codes) */
# define JO_VDP2_SPECIAL_BY_DOT             (2)
/** @brief Screens with special functions: NBG0 to NBG3 and RBG0 */
# define JO_VDP2_SPECIAL_SCREENS_MASK       (0x001F)
/** @brief BMPNA and BMPNB: special priority and special color calculation bits of a bitmap */
# define JO_VDP2_BMPN_SPECIAL_PRIORITY_SHIFT (5)
# define JO_VDP2_BMPN_SPECIAL_BLEND_SHIFT   (4)
# define JO_VDP2_BMPN_SPECIAL_MASK          (0x0030)
/** @brief Lines of a line window table (the most lines of a screen) */
# define JO_VDP2_LINE_WINDOW_LINES          (512)

static unsigned short                   *__jo_vdp2_line_window[2] = {JO_NULL, JO_NULL};

/** @brief A horizontal coordinate of a window: in the normal resolutions (320 and 352 pixels), the bit 0 is not used */
static  __jo_force_inline unsigned short jo_vdp2_window_x(const int x)
{
    return ((unsigned short)((__jo_screen.hires ? x : x << 1) & JO_VDP2_WINDOW_COORDINATE_MASK));
}

/** @brief The window control register and the shift of a screen
 *  @return false if the screen has no window
 */
static bool                             jo_vdp2_window_control(const jo_scroll_screen screen, volatile unsigned short **reg, int *shift)
{
    switch (screen)
    {
        case JO_NBG0_SCREEN: *reg = &JO_VDP2_WCTLA; *shift = 0; return (true);
        case JO_NBG1_SCREEN: *reg = &JO_VDP2_WCTLA; *shift = 8; return (true);
        case JO_NBG2_SCREEN: *reg = &JO_VDP2_WCTLB; *shift = 0; return (true);
        case JO_NBG3_SCREEN: *reg = &JO_VDP2_WCTLB; *shift = 8; return (true);
        case JO_RBG0_SCREEN: *reg = &JO_VDP2_WCTLC; *shift = 0; return (true);
        case JO_SPRITE_SCREEN: *reg = &JO_VDP2_WCTLC; *shift = 8; return (true);
        default: return (false);
    }
}

static void                             jo_vdp2_set_window_registers(const int window, const int left, const int top, const int right, const int bottom)
{
    if (window == JO_VDP2_WINDOW_0)
    {
        JO_VDP2_SET(JO_VDP2_WPSX0, jo_vdp2_window_x(left));
        JO_VDP2_SET(JO_VDP2_WPSY0, (unsigned short)(top & JO_VDP2_WINDOW_COORDINATE_MASK));
        JO_VDP2_SET(JO_VDP2_WPEX0, jo_vdp2_window_x(right));
        JO_VDP2_SET(JO_VDP2_WPEY0, (unsigned short)(bottom & JO_VDP2_WINDOW_COORDINATE_MASK));
    }
    else
    {
        JO_VDP2_SET(JO_VDP2_WPSX1, jo_vdp2_window_x(left));
        JO_VDP2_SET(JO_VDP2_WPSY1, (unsigned short)(top & JO_VDP2_WINDOW_COORDINATE_MASK));
        JO_VDP2_SET(JO_VDP2_WPEX1, jo_vdp2_window_x(right));
        JO_VDP2_SET(JO_VDP2_WPEY1, (unsigned short)(bottom & JO_VDP2_WINDOW_COORDINATE_MASK));
    }
}

void                                    jo_vdp2_set_window(const int window, int x, int y, int width, int height)
{
    if (window != JO_VDP2_WINDOW_0 && window != JO_VDP2_WINDOW_1)
        return;
    if (x < 0)
    {
        width += x;
        JO_ZERO(x);
    }
    if (y < 0)
    {
        height += y;
        JO_ZERO(y);
    }
    if (window == JO_VDP2_WINDOW_0)
        JO_VDP2_SET(JO_VDP2_LWTA0U, 0);
    else
        JO_VDP2_SET(JO_VDP2_LWTA1U, 0);
    if (x + width > __jo_screen.width)
        width = __jo_screen.width - x;
    if (y + height > __jo_screen.height)
        height = __jo_screen.height - y;
    if (width <= 0 || height <= 0)
        jo_vdp2_set_window_registers(window, 1, 1, 0, 0);
    else
        jo_vdp2_set_window_registers(window, x, y, x + width - 1, y + height - 1);
}

bool                                    jo_vdp2_set_window_lines(const int window, const int top, const int count, const short * const left, const short * const right)
{
    unsigned short                      *table;
    unsigned int                        offset;
    int                                 line;
    int                                 i;

    if (window != JO_VDP2_WINDOW_0 && window != JO_VDP2_WINDOW_1)
        return (false);
    if ((table = __jo_vdp2_line_window[window]) == JO_NULL &&
        (table = __jo_vdp2_line_window[window] = (unsigned short *)jo_vdp2_malloc(JO_VDP2_LINE_COLOR_TABLE, JO_VDP2_LINE_WINDOW_LINES * 4)) == JO_NULL)
        return (false);
    for (JO_ZERO(line); line < JO_VDP2_LINE_WINDOW_LINES; ++line)
    {
        i = line - top;
        // (in the screen: a right end after the screen was wrapped by the 10 bits of the coordinate, a line out of the
        // screen at the left was shown as a column of 1 pixel)
        if (i >= 0 && i < count && left[i] <= right[i] && right[i] >= 0 && left[i] < __jo_screen.width)
        {
            table[line * 2] = jo_vdp2_window_x(JO_MAX(left[i], 0));
            table[line * 2 + 1] = jo_vdp2_window_x(JO_MIN(right[i], __jo_screen.width - 1));
        }
        else
        {
            table[line * 2] = JO_VDP2_WINDOW_COORDINATE_MASK;
            table[line * 2 + 1] = JO_VDP2_WINDOW_COORDINATE_MASK;
        }
    }
    jo_vdp2_set_window_registers(window, 0, 0, __jo_screen.width - 1, __jo_screen.height - 1);
    offset = jo_vdp2_vram_offset(table) >> 1;
    if (window == JO_VDP2_WINDOW_0)
    {
        JO_VDP2_SET(JO_VDP2_LWTA0U, JO_VDP2_LINE_WINDOW_ENABLE | ((offset >> 16) & JO_VDP2_TABLE_ADDRESS_HIGH_MASK));
        JO_VDP2_SET(JO_VDP2_LWTA0L, offset & JO_VDP2_TABLE_ADDRESS_LOW_MASK);
    }
    else
    {
        JO_VDP2_SET(JO_VDP2_LWTA1U, JO_VDP2_LINE_WINDOW_ENABLE | ((offset >> 16) & JO_VDP2_TABLE_ADDRESS_HIGH_MASK));
        JO_VDP2_SET(JO_VDP2_LWTA1L, offset & JO_VDP2_TABLE_ADDRESS_LOW_MASK);
    }
    return (true);
}

bool                                    jo_vdp2_set_window_circle(const int window, const int x, const int y, const int radius)
{
    short                               left[JO_VDP2_LINE_WINDOW_LINES];
    short                               right[JO_VDP2_LINE_WINDOW_LINES];
    int                                 count;
    int                                 dy;
    int                                 dx;
    int                                 i;

    count = JO_MIN(2 * radius + 1, JO_VDP2_LINE_WINDOW_LINES);
    for (JO_ZERO(i); i < count; ++i)
    {
        dy = i - radius;
        dx = (int)jo_sqrt((unsigned int)(radius * radius - dy * dy));
        left[i] = (short)(x - dx);
        right[i] = (short)(x + dx);
    }
    return (jo_vdp2_set_window_lines(window, y - radius, count, left, right));
}

void                                    jo_vdp2_enable_window(const jo_scroll_screen screens, const int window, const jo_vdp2_window_area area)
{
    volatile unsigned short             *reg;
    int                                 shift;
    unsigned int                        screen;

    if (window != JO_VDP2_WINDOW_0 && window != JO_VDP2_WINDOW_1)
        return;
    for (screen = JO_NBG0_SCREEN; screen <= JO_SPRITE_SCREEN; screen <<= 1)
        if ((screens & screen) && jo_vdp2_window_control((jo_scroll_screen)screen, &reg, &shift))
            jo_vdp2_shadow_set_bits(JO_VDP2_REG_INDEX(*reg), JO_VDP2_WINDOW_BITS(window) << shift,
                                    (JO_VDP2_WINDOW_ENABLE_BIT(window) | (area == JO_VDP2_WINDOW_SHOW_INSIDE ? JO_VDP2_WINDOW_OUTSIDE_BIT(window) : 0)) << shift);
}

void                                    jo_vdp2_disable_window(const jo_scroll_screen screens)
{
    volatile unsigned short             *reg;
    int                                 shift;
    unsigned int                        screen;

    for (screen = JO_NBG0_SCREEN; screen <= JO_SPRITE_SCREEN; screen <<= 1)
        if ((screens & screen) && jo_vdp2_window_control((jo_scroll_screen)screen, &reg, &shift))
            jo_vdp2_shadow_set_bits(JO_VDP2_REG_INDEX(*reg), (JO_VDP2_WINDOW_BITS(JO_VDP2_WINDOW_0) | JO_VDP2_WINDOW_BITS(JO_VDP2_WINDOW_1)) << shift, 0);
}

void                                    jo_vdp2_set_blend_mode(const jo_vdp2_blend_mode mode)
{
    JO_VDP2_SET_BITS(JO_VDP2_CCCTL, JO_VDP2_CCCTL_MODE_MASK, ((mode & JO_VDP2_BLEND_ADD) ? JO_VDP2_CCCTL_ADD : 0) |
                                                         ((mode & JO_VDP2_BLEND_EXTENDED) ? JO_VDP2_CCCTL_EXTENDED : 0));
}

bool                                    jo_vdp2_enable_blur(const jo_scroll_screen screen)
{
    unsigned short                      source;

    // (no blur in the 640 and 704 pixels modes: the color RAM keeps its 2048 colors)
    if (__jo_screen.hires)
        return (false);
    switch (screen)
    {
        case JO_SPRITE_SCREEN: source = 0; break;
        case JO_RBG0_SCREEN: source = 1; break;
        case JO_NBG0_SCREEN: source = 2; break;
        case JO_NBG1_SCREEN: source = 4; break;
        case JO_NBG2_SCREEN: source = 5; break;
        case JO_NBG3_SCREEN: source = 6; break;
        default: return (false);
    }
    JO_VDP2_SET_BITS(JO_VDP2_RAMCTL, JO_VDP2_RAMCTL_CRAM_MODE_MASK, 0);
    JO_VDP2_SET_BITS(JO_VDP2_CCCTL, JO_VDP2_CCCTL_BLUR_MASK, JO_VDP2_CCCTL_BLUR | (source << JO_VDP2_CCCTL_BLUR_SHIFT));
    return (true);
}

void                                    jo_vdp2_disable_blur(void)
{
    JO_VDP2_SET_BITS(JO_VDP2_CCCTL, JO_VDP2_CCCTL_BLUR_MASK, 0);
    JO_VDP2_SET_BITS(JO_VDP2_RAMCTL, JO_VDP2_RAMCTL_CRAM_MODE_MASK, JO_VDP2_RAMCTL_CRAM_MODE_1);
}

void                                    jo_vdp2_set_special_colors(const unsigned char codes)
{
    JO_VDP2_SET(JO_VDP2_SFSEL, 0);
    JO_VDP2_SET(JO_VDP2_SFCODE, codes);
}

/** @brief The special bits of the characters and of the bitmaps, and the special function modes (by dot) of the screens */
static void                             jo_vdp2_update_special_functions(void)
{
    unsigned short                      priority;
    unsigned short                      blend;
    unsigned short                      bitmap;
    int                                 n;

    JO_ZERO(priority);
    JO_ZERO(blend);
    for (JO_ZERO(n); n < JO_VDP2_SCREEN_COUNT; ++n)
    {
        if (__jo_vdp2_special_priority_screens & (1 << n))
            priority |= JO_VDP2_SPECIAL_BY_DOT << JO_MULT_BY_2(n);
        if (__jo_vdp2_special_blend_screens & (1 << n))
            blend |= JO_VDP2_SPECIAL_BY_DOT << JO_MULT_BY_2(n);
        jo_vdp2_update_pattern_register(n);
    }
    JO_VDP2_SET(JO_VDP2_SFPRMD, priority);
    JO_VDP2_SET(JO_VDP2_SFCCMD, blend);
    JO_ZERO(bitmap);
    for (JO_ZERO(n); n < 2; ++n)
        bitmap |= (((__jo_vdp2_special_priority_screens >> n) & 1) << JO_VDP2_BMPN_SPECIAL_PRIORITY_SHIFT |
                   ((__jo_vdp2_special_blend_screens >> n) & 1) << JO_VDP2_BMPN_SPECIAL_BLEND_SHIFT) << JO_MULT_BY_8(n);
    JO_VDP2_SET_BITS(JO_VDP2_BMPNA, JO_VDP2_BMPN_SPECIAL_MASK | (JO_VDP2_BMPN_SPECIAL_MASK << 8), bitmap);
    JO_VDP2_SET_BITS(JO_VDP2_BMPNB, JO_VDP2_BMPN_SPECIAL_MASK,
                     ((__jo_vdp2_special_priority_screens >> JO_VDP2_SCREEN_RBG0) & 1) << JO_VDP2_BMPN_SPECIAL_PRIORITY_SHIFT |
                     ((__jo_vdp2_special_blend_screens >> JO_VDP2_SCREEN_RBG0) & 1) << JO_VDP2_BMPN_SPECIAL_BLEND_SHIFT);
}

void                                    jo_vdp2_enable_special_priority(const jo_scroll_screen screens)
{
    __jo_vdp2_special_priority_screens |= (unsigned short)(screens & JO_VDP2_SPECIAL_SCREENS_MASK);
    jo_vdp2_update_special_functions();
}

void                                    jo_vdp2_disable_special_priority(const jo_scroll_screen screens)
{
    __jo_vdp2_special_priority_screens &= (unsigned short)~screens;
    jo_vdp2_update_special_functions();
}

void                                    jo_vdp2_enable_special_blend(const jo_scroll_screen screens)
{
    __jo_vdp2_special_blend_screens |= (unsigned short)(screens & JO_VDP2_SPECIAL_SCREENS_MASK);
    jo_vdp2_update_special_functions();
}

void                                    jo_vdp2_disable_special_blend(const jo_scroll_screen screens)
{
    __jo_vdp2_special_blend_screens &= (unsigned short)~screens;
    jo_vdp2_update_special_functions();
}

/** @brief Condition of the color calculation of the sprites (jo_vdp2_blend_rgb_sprites_only()) */
static unsigned short                   __jo_vdp2_sprite_blend_condition = JO_VDP2_SPCTL_COLOR_CALC_ALL;

void                                    jo_vdp2_blend_rgb_sprites_only(const bool rgb_only)
{
    __jo_vdp2_sprite_blend_condition = rgb_only ? JO_VDP2_SPCTL_COLOR_CALC_MSB : JO_VDP2_SPCTL_COLOR_CALC_ALL;
    if (JO_VDP2_GET(JO_VDP2_CCCTL) & JO_SPRITE_SCREEN)
        JO_VDP2_SET_BITS(JO_VDP2_SPCTL, JO_VDP2_SPCTL_COLOR_CALC_MASK, __jo_vdp2_sprite_blend_condition);
}

void                                    jo_vdp2_enable_blend(const jo_scroll_screen screens, unsigned char ratio)
{
    unsigned short                      both;

    if (ratio > JO_VDP2_BLEND_RATIO_MAX)
        ratio = JO_VDP2_BLEND_RATIO_MAX;
    both = (unsigned short)((ratio << 8) | ratio);
    if (screens & JO_NBG0_SCREEN)
        JO_VDP2_SET_BITS(JO_VDP2_CCRNA, JO_VDP2_COLOR_RATE_LOW_MASK, ratio);
    if (screens & JO_NBG1_SCREEN)
        JO_VDP2_SET_BITS(JO_VDP2_CCRNA, JO_VDP2_COLOR_RATE_HIGH_MASK, ratio << 8);
    if (screens & JO_NBG2_SCREEN)
        JO_VDP2_SET_BITS(JO_VDP2_CCRNB, JO_VDP2_COLOR_RATE_LOW_MASK, ratio);
    if (screens & JO_NBG3_SCREEN)
        JO_VDP2_SET_BITS(JO_VDP2_CCRNB, JO_VDP2_COLOR_RATE_HIGH_MASK, ratio << 8);
    if (screens & JO_RBG0_SCREEN)
        JO_VDP2_SET_BITS(JO_VDP2_CCRR, JO_VDP2_COLOR_RATE_LOW_MASK, ratio);
    // (JO_BACK_SCREEN is also JO_LNCL_SCREEN: the ratio of the back screen and of the line color screen)
    if (screens & JO_BACK_SCREEN)
        JO_VDP2_SET(JO_VDP2_CCRLB, both);
    if (screens & JO_SPRITE_SCREEN)
    {
        JO_VDP2_SET_BITS(JO_VDP2_SPCTL, JO_VDP2_SPCTL_COLOR_CALC_MASK, __jo_vdp2_sprite_blend_condition);
        JO_VDP2_SET(JO_VDP2_CCRSA, both);
        JO_VDP2_SET(JO_VDP2_CCRSB, both);
        JO_VDP2_SET(JO_VDP2_CCRSC, both);
        JO_VDP2_SET(JO_VDP2_CCRSD, both);
    }
    JO_VDP2_SET_BITS(JO_VDP2_CCCTL, screens & JO_VDP2_CCCTL_SCREENS_MASK, screens & JO_VDP2_CCCTL_SCREENS_MASK);
}

void                                    jo_vdp2_disable_blend(const jo_scroll_screen screens)
{
    JO_VDP2_SET_BITS(JO_VDP2_CCCTL, screens & JO_VDP2_CCCTL_SCREENS_MASK, 0);
    if (screens & JO_SPRITE_SCREEN)
        JO_VDP2_SET_BITS(JO_VDP2_SPCTL, JO_VDP2_SPCTL_COLOR_CALC_MASK, 0);
}

/*
** Line color screen
*/

/** @brief LCTAU: one color per line (0: one color for the whole screen) */
# define JO_VDP2_LCTAU_LINE_MODE            (1 << 15)
/** @brief LNCLEN: the line color screen inserted under NBG0 to NBG3 and RBG0 (bits 0 to 4) and the sprites (bit 5) */
# define JO_VDP2_LNCLEN_SCREENS_MASK        (0x001F)
# define JO_VDP2_LNCLEN_SPRITE              (1 << 5)
/** @brief A line color: an index of the color RAM (11 bits) */
# define JO_VDP2_LINE_COLOR_MASK            (0x07FF)

/** @brief Line color tables (2: the table displayed is not changed during the display), and the single color */
static unsigned short                   *__jo_vdp2_line_colors[2] = {JO_NULL, JO_NULL};
static int                              __jo_vdp2_line_colors_index = 0;
static unsigned short                   *__jo_vdp2_line_color = JO_NULL;

static void                             jo_vdp2_set_line_color_table(const unsigned short * const table, const bool per_line)
{
    unsigned int                        offset;

    offset = jo_vdp2_vram_offset(table) >> 1;
    JO_VDP2_SET(JO_VDP2_LCTAU, (per_line ? JO_VDP2_LCTAU_LINE_MODE : 0) | ((offset >> 16) & JO_VDP2_TABLE_ADDRESS_HIGH_MASK));
    JO_VDP2_SET(JO_VDP2_LCTAL, offset & JO_VDP2_TABLE_ADDRESS_LOW_16BITS_MASK);
}

bool                                    jo_vdp2_set_line_colors(const unsigned short * const colors, const int count)
{
    unsigned short                      *table;
    int                                 line;

#ifdef JO_DEBUG
    if (colors == JO_NULL || count <= 0)
    {
        jo_core_error("colors is null or count <= 0");
        return (false);
    }
#endif
    __jo_vdp2_line_colors_index ^= 1;
    if ((table = __jo_vdp2_line_colors[__jo_vdp2_line_colors_index]) == JO_NULL &&
        (table = __jo_vdp2_line_colors[__jo_vdp2_line_colors_index] =
         (unsigned short *)jo_vdp2_malloc(JO_VDP2_LINE_COLOR_TABLE, JO_VDP2_LINE_COLOR_LINES * sizeof(unsigned short))) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of VDP2 memory");
#endif
        return (false);
    }
    // (the lines after count: the last color)
    for (JO_ZERO(line); line < JO_VDP2_LINE_COLOR_LINES; ++line)
        table[line] = colors[line < count ? line : count - 1] & JO_VDP2_LINE_COLOR_MASK;
    jo_vdp2_set_line_color_table(table, true);
    return (true);
}

bool                                    jo_vdp2_set_line_color(const unsigned short color)
{
    if (__jo_vdp2_line_color == JO_NULL &&
        (__jo_vdp2_line_color = (unsigned short *)jo_vdp2_malloc(JO_VDP2_LINE_COLOR_TABLE, sizeof(unsigned short))) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of VDP2 memory");
#endif
        return (false);
    }
    *__jo_vdp2_line_color = color & JO_VDP2_LINE_COLOR_MASK;
    jo_vdp2_set_line_color_table(__jo_vdp2_line_color, false);
    return (true);
}

static unsigned short                   jo_vdp2_line_color_bits(const jo_scroll_screen screens)
{
    return ((unsigned short)((screens & JO_VDP2_LNCLEN_SCREENS_MASK) | (screens & JO_SPRITE_SCREEN ? JO_VDP2_LNCLEN_SPRITE : 0)));
}

void                                    jo_vdp2_enable_line_color(const jo_scroll_screen screens)
{
    unsigned short                      bits;

    bits = jo_vdp2_line_color_bits(screens);
    JO_VDP2_SET_BITS(JO_VDP2_LNCLEN, bits, bits);
}

void                                    jo_vdp2_disable_line_color(const jo_scroll_screen screens)
{
    JO_VDP2_SET_BITS(JO_VDP2_LNCLEN, jo_vdp2_line_color_bits(screens), 0);
}

void                                    jo_vdp2_enable_shadow(const jo_scroll_screen screens)
{
    JO_VDP2_SET_BITS(JO_VDP2_SDCTL, JO_VDP2_SDCTL_SCREENS_MASK, screens & JO_VDP2_SDCTL_SCREENS_MASK);
}

void                                    jo_vdp2_disable_shadow(void)
{
    JO_VDP2_SET_BITS(JO_VDP2_SDCTL, JO_VDP2_SDCTL_SCREENS_MASK, 0);
}

void                                    jo_vdp2_enable_mosaic(const jo_scroll_screen screens, const int width, const int height)
{
    jo_enable_screen_mozaic(screens, (short)width, (short)height);
}

void                                    jo_vdp2_disable_mosaic(void)
{
    jo_disable_all_screen_mozaic();
}

const unsigned short                    *jo_vdp2_get_registers(void)
{
    return (__jo_vdp2_shadow);
}

void                                    jo_vdp2_set_hires_sprite_mode(const jo_vdp2_hires_sprite_mode mode)
{
    static const unsigned char          types[3] = {0xF, 0xE, 0xD};

    __jo_vdp2_hires_sprite_type = types[mode];
    jo_vdp2_set_tv_mode();
}

void                                    jo_vdp2_set_sprite_priorities(const unsigned char p0, const unsigned char p1,
                                                                      const unsigned char p2, const unsigned char p3)
{
    JO_VDP2_SET(JO_VDP2_PRISA, JO_VDP2_PRIORITY_PAIR(p1, p0));
    JO_VDP2_SET(JO_VDP2_PRISB, JO_VDP2_PRIORITY_PAIR(p3, p2));
}

void                                    jo_vdp2_set_sprite_blend_ratios(const unsigned char r0, const unsigned char r1,
                                                                        const unsigned char r2, const unsigned char r3)
{
    JO_VDP2_SET(JO_VDP2_CCRSA, (unsigned short)((JO_MIN(r1, JO_VDP2_BLEND_RATIO_MAX) << 8) | JO_MIN(r0, JO_VDP2_BLEND_RATIO_MAX)));
    JO_VDP2_SET(JO_VDP2_CCRSB, (unsigned short)((JO_MIN(r3, JO_VDP2_BLEND_RATIO_MAX) << 8) | JO_MIN(r2, JO_VDP2_BLEND_RATIO_MAX)));
}

void                                    __jo_vdp2_set_screen_mode(void)
{
    bool                                on;

    on = (JO_VDP2_GET(JO_VDP2_TVMD) & JO_VDP2_TVMD_DISPLAY) != 0 || __jo_vdp2_hidden_fields > 0;
    jo_vdp2_set_tv_mode();
    __jo_vdp2_set_tv_display(false);
    if (on)
        __jo_vdp2_hidden_fields = 4;
    if (__jo_vdp2_nbg1_native)
        jo_vdp2_use_nbg1_native_bitmap();
    jo_vdp2_set_window(JO_VDP2_WINDOW_0, 0, 0, __jo_screen.width, __jo_screen.height);
    jo_vdp2_set_window(JO_VDP2_WINDOW_1, 0, 0, __jo_screen.width, __jo_screen.height);
}

void                                    __jo_vdp2_update_tv_mode(void)
{
    bool                                on;

    on = (JO_VDP2_GET(JO_VDP2_TVMD) & JO_VDP2_TVMD_DISPLAY) != 0;
    jo_vdp2_set_tv_mode();
    __jo_vdp2_set_tv_display(on);
}

void                                    jo_core_set_screen_palette(const int palette_id)
{
    __jo_vdp2_screen_palette = palette_id;
    jo_vdp2_set_tv_mode();
}
