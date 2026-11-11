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
** Glows: see jo/glow.h
**
** One VDP2 layer (NBG2) over the sprites, its color added to them, shows all the glows of a frame: its map is a canvas of
** cells (64 x 64 cells, the screen at its top left, scrolled by 0 to 7 pixels). Each glow seen puts the cells of its shape
** there, with its own palette (16 colors: the palettes 1 to 15 of a block of the color RAM, its color x its intensity at
** each level of light): up to 15 glows at the same time, each one its size, its color and its shape.
**
** VRAM (bank B0 of the VDP2, one block): the map (8 KB, aligned on 8 KB), then the characters (16 colors, 32 bytes per
** cell): an empty cell, the discs with their rays (STAR, each size), the soft discs (DISC, each size; the small ones also
** moved by 4 pixels in X, Y or both, in a box of one more cell: they are placed within 2 pixels), the pieces of the
** rectangles (RECT, FRAME: corners, edges and middle for 3 widths of light, put together around a rectangle of cells),
** then the pool of the cells where glows overlap (their light added, computed for each frame).
** Levels of light 1 to 15 (0: transparent); the light of a level is (level / 15)^2 x the color of the glow.
** The glow seen the most (its light x its size) is placed to the pixel: the scroll of the layer puts the grid of the cells
** on it; the others are within 2 pixels (the small discs) or 4 pixels.
*/

#include <stdbool.h>
#include "jo/conf.h"
#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/core.h"
#include "jo/tools.h"
#include "jo/malloc.h"
#include "jo/colors.h"
#include "jo/vdp1_command_pipeline.h"
#include "jo/sprites.h"
#include "jo/math.h"
#include "jo/list.h"
#include "jo/3d.h"
#include "jo/vdp2_malloc.h"
#include "jo/glow.h"
#include "internal/vdp1_command_pipeline.h"

#ifdef JO_COMPILE_WITH_3D_SUPPORT

/** @brief Sizes of the discs (pixels, multiples of 8) */
#define JO_GLOW_SIZES                       (12)
static const unsigned char                  __jo_glow_sizes[JO_GLOW_SIZES] = {16, 24, 32, 40, 48, 56, 64, 72, 80, 96, 112, 128};
/** @brief The soft discs up to this size index (48 pixels) also exist moved by 4 pixels (X, Y, both) */
#define JO_GLOW_PHASED_SIZES                (5)
/** @brief Widths of the light around a rectangle (cells: 8, 16, 24 pixels) */
#define JO_GLOW_MARGINS                     (3)
/** @brief Cells in a line of the map (1 page) */
#define JO_GLOW_MAP_WIDTH                   (64)
/** @brief Size of the map (1 page of 64 x 64 cells, 1 word per cell) */
#define JO_GLOW_MAP_BYTES                   (8192)
/** @brief Light levels (16 colors characters: 1 to 15, 0 is transparent) */
#define JO_GLOW_LEVELS                      (15)
/** @brief Glows shown at the same time: the palettes 1 to 15 of the block of the color RAM */
#define JO_GLOW_SHOWN                       (15)
/** @brief Cells where glows overlap, for each frame (their light computed) */
#define JO_GLOW_POOL                        (96)
/** @brief Frames remembered (the frames computed before their image is displayed) */
#define JO_GLOW_STATES                      (4)
/** @brief Change of the fade of a glow at each frame (0 to 256): it appears and disappears in 4 frames */
#define JO_GLOW_FADE_STEP                   (64)
/** @brief Rays of the occlusion at each frame */
#define JO_GLOW_RAYS_PER_FRAME              (2)
/** @brief Width of the rays of light of a glow (half pixels: the light falls to 0 at this distance from the ray) */
#define JO_GLOW_RAY_WIDTH                   (3)
/** @brief Light of the rays on the axes, on the diagonals (0 to 256, in the middle of the glow), see jo_3d_glows_set_rays() */
#define JO_GLOW_RAY_LIGHT                   (150)
#define JO_GLOW_DIAGONAL_RAY_LIGHT          (100)
static int                          __jo_glow_ray_light = JO_GLOW_RAY_LIGHT;
static int                          __jo_glow_diagonal_ray_light = JO_GLOW_DIAGONAL_RAY_LIGHT;
/** @brief The ray stops before the lamp (its model) */
#define JO_GLOW_LAMP_SIZE                   (16)
/** @brief The cells of the screen remembered for the overlaps (a cell more on each side) */
#define JO_GLOW_GRID_WIDTH                  (64)
#define JO_GLOW_GRID_HEIGHT                 (34)
/** @brief Light inside a filled rectangle (0 to 1, Q16): the light around it falls from there */
#define JO_GLOW_RECT_LIGHT                  (JO_FIXED_1 * 55 / 100)

/** @brief Kinds of the items of a frame */
#define JO_GLOW_ITEM_STAMP                  (0)
#define JO_GLOW_ITEM_RECT                   (1)
#define JO_GLOW_ITEM_FRAME                  (2)

typedef struct
{
    bool                            used;
    bool                            seen;           // No wall between the camera and the lamp (last ray)
    bool                            tested;         // A ray was cast since the glow was added or moved
    unsigned char                   shape;          // jo_3d_glow_shape
    bool                            anchor;         // Placed to the pixel (the scroll of the layer), see jo_3d_glow_set_anchor()
    jo_pos3D_fixed                  position;
    jo_pos3D_fixed                  tested_from;    // The camera of the last ray (the same camera: the same result)
    unsigned char                   r;
    unsigned char                   g;
    unsigned char                   b;
    short                           radius;
    short                           width;          // The rectangles (world units)
    short                           height;
    short                           intensity;
    short                           fade;           // 0 (hidden) to 256 (seen)
    short                           placed_x;       // The top left pixel of its star in the frame before (see
    short                           placed_y;       // jo_glow_sticky_cell())
    bool                            placed;
}                                   jo_glow;

/** @brief A glow in the cells of a frame: a box of cells (screen cells, the grid moved by the scroll of the frame) */
typedef struct
{
    short                           cx;             // Cell of the top left of the box
    short                           cy;
    unsigned char                   w;              // Size of the box (cells)
    unsigned char                   h;
    unsigned char                   kind;           // JO_GLOW_ITEM_xxx
    unsigned char                   palette;        // 1 to 15
    unsigned short                  first;          // First character (the stamp, the pieces of the rectangle)
    unsigned char                   inner_w;        // Rectangle (cells)
    unsigned char                   inner_h;
    unsigned char                   margin;         // Width of the light (cells)
    short                           clip_x0;        // The cells seen (the clip of the glow)
    short                           clip_y0;
    short                           clip_x1;
    short                           clip_y1;
}                                   jo_glow_item;

/** @brief What the layer displays in a frame */
typedef struct
{
    unsigned int                    tag;            // Frame (see __jo_vdp1_frame_tag())
    unsigned char                   fine_x;         // Scroll (0 to 7): the grid of the cells on the glow seen the most
    unsigned char                   fine_y;
    unsigned char                   count;
    unsigned char                   overlap_count;
    jo_glow_item                    items[JO_GLOW_SHOWN];
    unsigned short                  colors[JO_GLOW_SHOWN][JO_GLOW_LEVELS];
    unsigned short                  overlap_cell[JO_GLOW_POOL];     // Index in the map
    unsigned short                  overlap_name[JO_GLOW_POOL];     // Pattern name
}                                   jo_glow_state;

/** @brief The cells written in the map (cleared at the next change) */
typedef struct
{
    unsigned char                   count;
    unsigned char                   overlap_count;
    short                           box[JO_GLOW_SHOWN][4];
    unsigned short                  overlap_cell[JO_GLOW_POOL];
}                                   jo_glow_displayed;

void                                __jo_vdp2_glow_layers_show(const void *cells, const void *map2, const void *map3, const int cram_block);
void                                __jo_vdp2_glow_layers_hide(void);

static jo_glow                      __jo_glows[JO_3D_GLOW_MAX];
static jo_glow_state                __jo_glow_states[JO_GLOW_STATES];
static jo_glow_displayed            __jo_glow_displayed;
static bool                         __jo_glows_enabled = false;
static int                          __jo_glow_vblank_event = -1;
static unsigned char                *__jo_glow_vram = JO_NULL;
static volatile unsigned short      *__jo_glow_map;
static unsigned char                *__jo_glow_cells;
static unsigned char                *__jo_glow_pool;
/** @brief Character number of the first cell of each disc (STAR), of each soft disc and its moves (DISC: 0, X, Y, X and Y;
 *  0: none, the STAR disc instead), of the pieces of the rectangles of each width (0: none), of the empty cell */
static unsigned short               __jo_glow_star_char[JO_GLOW_SIZES];
static unsigned short               __jo_glow_disc_char[JO_GLOW_SIZES][4];
static unsigned short               __jo_glow_rect_char[JO_GLOW_MARGINS];
static unsigned short               __jo_glow_frame_char[JO_GLOW_MARGINS];
static unsigned short               __jo_glow_empty_char;
/** @brief The overlap of each cell of the screen in the frame computed: its index in the overlaps + 1 (0: none), and
 *  in the high byte the last glow added there + 1 */
static unsigned short               __jo_glow_grid[JO_GLOW_GRID_HEIGHT][JO_GLOW_GRID_WIDTH];
/** @brief Color RAM: block of 256 colors, the palettes 1 to 15 */
static int                          __jo_glow_cram_block = -1;
static volatile unsigned short      *__jo_glow_palettes;
static unsigned short               __jo_glow_displayed_colors[JO_GLOW_SHOWN][JO_GLOW_LEVELS];
/** @brief The light of 2 levels added: sqrt(a^2 + b^2), 15 at most */
static unsigned char                __jo_glow_add_levels[16][16];
static bool                         (*__jo_glow_hit)(const jo_pos3D_fixed * const origin, const jo_pos3D_fixed * const dir, const jo_fixed max_t) = JO_NULL;
static int                          __jo_glow_ray_cursor = 0;
static bool                         (*__jo_glow_clip)(const jo_pos3D_fixed * const position, jo_3d_rect * const rect) = JO_NULL;

/*
** VRAM
*/

/** @brief Character number of a cell (1 word pattern names, 12 bits character numbers: the upper bits are the same for all
 *  the cells of the bank, see jo_vdp2_supplementary_character())
 *  @param cell Address of the cell
 *  @return Character number
 */
static  __jo_force_inline unsigned short    jo_glow_character(const unsigned char * const cell)
{
    return ((((unsigned int)cell & 0x7FFFF) >> 5) & 0xFFF);
}

/** @brief Address of a character
 *  @param c Character number
 */
static  __jo_force_inline unsigned short    *jo_glow_character_address(const unsigned short c)
{
    return ((unsigned short *)(((unsigned int)__jo_glow_cells & ~0x1FFFF) | ((unsigned int)c << 5)));
}

/** @brief Light of a ray of a glow at a pixel (Q16): thin, the light falls along it (to 0 at its end)
 *  @param across Distance to the line of the ray (half pixels)
 *  @param along Distance along the ray (half pixels)
 *  @param length Length of the ray (half pixels)
 *  @param light Light of the ray in the middle (0 to 256)
 */
static jo_fixed                     jo_glow_ray(const int across, const int along, const int length, const int light)
{
    jo_fixed                        fall;
    jo_fixed                        width;

    if (along >= length || across >= JO_GLOW_RAY_WIDTH)
        return (0);
    // (the divider of the CPU: the same quotient, 0 <= along < length)
    fall = JO_FIXED_1 - jo_fixed_div(along, length);
    fall = jo_fixed_mult(fall, fall);
    width = JO_FIXED_1 - (jo_fixed)((across << 16) / JO_GLOW_RAY_WIDTH);
    // (positive: a logical shift, an arithmetic shift is a call of the library of the compiler)
    return ((jo_fixed)((unsigned int)(jo_fixed_mult(fall, width) * light) >> 8));
}

/** @brief Light of a pixel of a glow: a disc (1 - d^2)^4 (d: distance to the middle / radius, 0 at the edge) and, for a
 *  STAR, 8 rays (4 long ones on the axes, 4 shorter ones on the diagonals)
 *  @param dx Distance to the middle along X (half pixels: the middle is between two pixels)
 *  @param dy Distance to the middle along Y (half pixels)
 *  @param size Size of the disc
 *  @param rays With its rays
 *  @return Light (Q16, 0 to 1)
 */
static jo_fixed                     jo_glow_light(const int dx, const int dy, const int size, const bool rays)
{
    int                             u;
    int                             v;
    jo_fixed                        q;
    jo_fixed                        ray;

    // 1 - d^2 (Q16)
    // (the divider of the CPU instead of a 64 bits division for each pixel: the same quotient)
    q = JO_FIXED_1 - jo_fixed_div(dx * dx + dy * dy, size * size);
    if (q > 0)
    {
        q = jo_fixed_mult(q, q);
        q = jo_fixed_mult(q, q);
    }
    else
        q = 0;
    if (!rays)
        return (q);
    // The rays on the axes (to the edge of the square)
    ray = jo_glow_ray(JO_MIN(dx, dy), JO_MAX(dx, dy), size, __jo_glow_ray_light);
    // The rays on the diagonals (x 1 / sqrt(2): the distances along the diagonals)
    // (positive: logical shifts)
    u = (int)((unsigned int)((dx + dy) * 181) >> 8);
    v = (int)((unsigned int)(JO_ABS(dx - dy) * 181) >> 8);
    ray = JO_MAX(ray, jo_glow_ray(JO_MIN(u, v), JO_MAX(u, v), size * 3 / 4, __jo_glow_diagonal_ray_light));
    return (JO_MIN(q + ray, JO_FIXED_1));
}

/** @brief The light falling away from a rectangle or a line (Q16): (1 - d)^2, 0 beyond the width
 *  @param d Distance (half pixels)
 *  @param width Width of the light (half pixels)
 */
static jo_fixed                     jo_glow_fall(const int d, const int width)
{
    jo_fixed                        q;

    if (d >= width)
        return (0);
    q = JO_FIXED_1 - jo_fixed_div(d, width);
    return (jo_fixed_mult(q, q));
}

/** @brief The ordered dither of the levels (4 x 4 pixels, x 4096) */
static const unsigned short         __jo_glow_bayer[16] =
{
    0 << 12, 8 << 12, 2 << 12, 10 << 12, 12 << 12, 4 << 12, 14 << 12, 6 << 12,
    3 << 12, 11 << 12, 1 << 12, 9 << 12, 15 << 12, 7 << 12, 13 << 12, 5 << 12
};

/** @brief The light of a pixel ready for jo_glow_level(): x the levels, rounded
 *  @param q Its light (see jo_glow_light(), 0 to 1)
 */
static  __jo_force_inline unsigned int jo_glow_level_base(const jo_fixed q)
{
    return ((unsigned int)q * JO_GLOW_LEVELS + (1 << 11));
}

/** @brief Level of a pixel of a glow, with an ordered dither between two levels (no ring)
 *  @param dither The dither of the pixel (__jo_glow_bayer[(y & 3) * 4 + (x & 3)])
 *  @param base Its light (see jo_glow_level_base())
 *  @return Level (0 to 15)
 */
static  __jo_force_inline unsigned int jo_glow_level(const unsigned int dither, const unsigned int base)
{
    unsigned int                    level;

    level = (base + dither) >> 16;
    return (JO_MIN(level, JO_GLOW_LEVELS));
}

/** @brief The light of a quarter of a disc (Q16), by the distances to its middle (half pixels, odd: index (d >> 1)): the
 *  same for all its moves (computed once for each size; the start of the glows took 0.6 s without it). JO_NULL: computed
 *  for each pixel (no memory) */
static jo_fixed                     *__jo_glow_lut = JO_NULL;
static int                          __jo_glow_lut_half = 0;

/** @brief The light of a quarter of a disc (see __jo_glow_lut)
 *  @param size Size of the disc
 *  @param rays With its rays
 */
static void                         jo_glow_build_lut(const int size, const bool rays)
{
    int                             half;
    int                             x;
    int                             y;

    half = size / 2;
    if (__jo_glow_lut == JO_NULL)
        __jo_glow_lut = (jo_fixed *)jo_malloc((unsigned int)(64 * 64) * sizeof(jo_fixed));
    __jo_glow_lut_half = 0;
    if (__jo_glow_lut == JO_NULL)
        return;
    // (the same on both sides of the diagonal: jo_glow_light() of (dx, dy) and of (dy, dx) are the same)
    for (y = 0; y < half; ++y)
        for (x = 0; x < half; ++x)
            __jo_glow_lut[y * half + x] = x < y ? __jo_glow_lut[x * half + y] : jo_glow_light(2 * x + 1, 2 * y + 1, size, rays);
    __jo_glow_lut_half = half;
}

/** @brief The light of a pixel of a piece of the characters (see jo_glow_write_block())
 *  @param kind 0: STAR disc, 1: DISC, 2: corner of a rectangle, 3: edge of a rectangle (Y), 4: middle of a rectangle,
 *         5: corner of a frame, 6: edge of a frame (Y)
 *  @param px Pixel in the block (X)
 *  @param py Pixel in the block (Y)
 *  @param a First parameter: the size of the disc, the width of the light (pixels)
 *  @param ox Middle of the disc, corner of the rectangle in the block (half pixels)
 *  @param oy
 */
static jo_fixed                     jo_glow_piece_light(const int kind, const int px, const int py, const int a, const int ox, const int oy)
{
    int                             dx;
    int                             dy;
    int                             d;

    dx = 2 * px + 1 - ox;
    dy = 2 * py + 1 - oy;
    switch (kind)
    {
        case 0:
        case 1:
            dx = JO_ABS(dx);
            dy = JO_ABS(dy);
            if (__jo_glow_lut_half == a / 2)
                return ((dx >> 1) < __jo_glow_lut_half && (dy >> 1) < __jo_glow_lut_half ?
                        __jo_glow_lut[(dy >> 1) * __jo_glow_lut_half + (dx >> 1)] : 0);
            return (jo_glow_light(dx, dy, a, kind == 0));
        case 2:
            // (the corner at (ox, oy), the rectangle toward +X +Y: the pixels at -X -Y)
            d = (int)jo_sqrt((unsigned int)(dx * dx + dy * dy));
            return (jo_fixed_mult(JO_GLOW_RECT_LIGHT, jo_glow_fall(d, 2 * a)));
        case 3:
            return (jo_fixed_mult(JO_GLOW_RECT_LIGHT, jo_glow_fall(JO_ABS(dy), 2 * a)));
        case 4:
            return (JO_GLOW_RECT_LIGHT);
        case 5:
            // (the 2 lines from the corner: toward +X along Y = oy, toward +Y along X = ox)
            if (dx >= 0 && dy >= 0)
                d = JO_MIN(dx, dy);
            else if (dx < 0 && dy < 0)
                d = (int)jo_sqrt((unsigned int)(dx * dx + dy * dy));
            else
                d = dx < 0 ? -dx : -dy;
            return (jo_glow_fall(d, 2 * a));
        default:
            return (jo_glow_fall(JO_ABS(dy), 2 * a));
    }
}

/** @brief Write a block of cells (16 colors: 4 bits per pixel, the left pixel in the high bits; the cells line by line)
 *  @param cell First cell
 *  @param w Width of the block (cells)
 *  @param h Height (cells)
 *  @param kind See jo_glow_piece_light()
 *  @param a See jo_glow_piece_light()
 *  @param ox See jo_glow_piece_light()
 *  @param oy See jo_glow_piece_light()
 *  @param flip_x The block is the mirror in X of the computed one, flip_y in Y (the other corners, edges)
 *  @param flip_y
 *  @param transpose X and Y swapped (the edges along Y from the edges along X)
 *  @return The cell after the block
 */
static unsigned char                *jo_glow_write_block(unsigned char *cell, const int w, const int h, const int kind, const int a,
                                                         const int ox, const int oy, const bool flip_x, const bool flip_y,
                                                         const bool transpose)
{
    volatile unsigned short         *dst;
    const unsigned short            *dither;
    unsigned int                    word;
    int                             cx;
    int                             cy;
    int                             x;
    int                             y;
    int                             i;
    int                             px;
    int                             py;
    int                             qx;
    int                             qy;

    for (cy = 0; cy < h; ++cy)
        for (cx = 0; cx < w; ++cx, cell += 32)
        {
            dst = (volatile unsigned short *)cell;
            for (y = 0; y < 8; ++y)
            {
                py = cy * 8 + y;
                dither = __jo_glow_bayer + (py & 3) * 4;
                for (x = 0; x < 8; x += 4)
                {
                    for (JO_ZERO(i), JO_ZERO(word); i < 4; ++i)
                    {
                        px = cx * 8 + x + i;
                        qx = flip_x ? w * 8 - 1 - px : px;
                        qy = flip_y ? h * 8 - 1 - py : py;
                        word = (word << 4) | jo_glow_level(dither[px & 3], jo_glow_level_base(transpose ?
                                                           jo_glow_piece_light(kind, qy, qx, a, ox, oy) :
                                                           jo_glow_piece_light(kind, qx, qy, a, ox, oy)));
                    }
                    *dst++ = (unsigned short)word;
                }
            }
        }
    return (cell);
}

/** @brief The cells of a rectangle or a frame (see jo_glow_cell_name()): 4 corners, the edges (top, bottom, left, right),
 *  the middle (filled rectangle)
 *  @param cell First cell
 *  @param c Width of the light (cells)
 *  @param frame A frame (its corners and edges 2 c cells: the light on both sides of the line)
 *  @return The cell after them
 */
static unsigned char                *jo_glow_write_rect(unsigned char *cell, const int c, const bool frame)
{
    int                             n;
    int                             o;
    int                             kc;
    int                             ke;

    n = frame ? 2 * c : c;
    // (the corner of the rectangle in the block of the top left corner: half pixels)
    o = 2 * 8 * c;
    kc = frame ? 5 : 2;
    ke = frame ? 6 : 3;
    // The corners: top left, top right, bottom left, bottom right
    cell = jo_glow_write_block(cell, n, n, kc, 8 * c, o, o, false, false, false);
    cell = jo_glow_write_block(cell, n, n, kc, 8 * c, o, o, true, false, false);
    cell = jo_glow_write_block(cell, n, n, kc, 8 * c, o, o, false, true, false);
    cell = jo_glow_write_block(cell, n, n, kc, 8 * c, o, o, true, true, false);
    // The edges: top (a column of n cells), bottom, left (a line of n cells), right
    cell = jo_glow_write_block(cell, 1, n, ke, 8 * c, 0, o, false, false, false);
    cell = jo_glow_write_block(cell, 1, n, ke, 8 * c, 0, o, false, true, false);
    cell = jo_glow_write_block(cell, n, 1, ke, 8 * c, 0, o, false, false, true);
    cell = jo_glow_write_block(cell, n, 1, ke, 8 * c, 0, o, true, false, true);
    // The middle
    if (!frame)
        cell = jo_glow_write_block(cell, 1, 1, 4, 8 * c, 0, 0, false, false, false);
    return (cell);
}

/** @brief Cells of the pieces of a rectangle
 *  @param c Width of the light (cells)
 *  @param frame A frame
 */
static  __jo_force_inline unsigned int jo_glow_rect_cells(const int c, const bool frame)
{
    int                             n;

    n = frame ? 2 * c : c;
    return ((unsigned int)(4 * n * n + 4 * n + (frame ? 0 : 1)));
}

/** @brief Write the characters (the cells of the discs: computed for a quarter of the disc, the same light in the 4)
 *  @param with_discs With the soft discs (DISC)
 *  @param with_moves With the soft discs moved by 4 pixels
 *  @return false if the VDP2 memory is full
 */
static bool                         jo_glow_build_vram(void)
{
    volatile unsigned short         *dst;
    unsigned char                   *cell;
    unsigned int                    cells;
    unsigned int                    base_cells;
    unsigned int                    disc_cells;
    unsigned int                    move_cells;
    unsigned int                    start;
    int                             k;
    int                             n;
    int                             m;
    int                             tier;
    int                             x;
    int                             y;

    if (__jo_glow_vram != JO_NULL)
        return (true);
    // The cells: the empty cell, the STAR discs, the rectangles, the pool for each frame; then the DISC discs and their
    // moves if the memory allows (else a DISC is drawn with the STAR disc)
    base_cells = 1 + JO_GLOW_STATES * JO_GLOW_POOL;
    disc_cells = 0;
    move_cells = 0;
    for (k = 0; k < JO_GLOW_SIZES; ++k)
    {
        n = __jo_glow_sizes[k] / 8;
        base_cells += n * n;
        disc_cells += n * n;
        if (k < JO_GLOW_PHASED_SIZES)
            move_cells += 2 * (n + 1) * n + (n + 1) * (n + 1);
    }
    for (k = 0; k < JO_GLOW_MARGINS; ++k)
        base_cells += jo_glow_rect_cells(k + 1, false) + jo_glow_rect_cells(k + 1, true);
    // (the map aligned on its size: up to a map more)
    for (tier = 0; tier < 3; ++tier)
    {
        cells = base_cells + (tier < 2 ? disc_cells : 0) + (tier < 1 ? move_cells : 0);
        __jo_glow_vram = (unsigned char *)jo_vdp2_malloc(JO_VDP2_RAM_MAP_NBG1, 2 * JO_GLOW_MAP_BYTES + cells * 32);
        if (__jo_glow_vram != JO_NULL)
            break;
    }
    if (__jo_glow_vram == JO_NULL)
        return (false);
    start = ((unsigned int)__jo_glow_vram + JO_GLOW_MAP_BYTES - 1) & ~(JO_GLOW_MAP_BYTES - 1);
    __jo_glow_map = (volatile unsigned short *)start;
    __jo_glow_cells = (unsigned char *)(start + JO_GLOW_MAP_BYTES);
    // The empty cell
    dst = (volatile unsigned short *)__jo_glow_cells;
    for (x = 0; x < 16; ++x)
        dst[x] = 0;
    __jo_glow_empty_char = jo_glow_character(__jo_glow_cells);
    cell = __jo_glow_cells + 32;
    // The discs with their rays
    for (k = 0; k < JO_GLOW_SIZES; ++k)
    {
        n = __jo_glow_sizes[k] / 8;
        __jo_glow_star_char[k] = jo_glow_character(cell);
        jo_glow_build_lut(__jo_glow_sizes[k], true);
        cell = jo_glow_write_block(cell, n, n, 0, __jo_glow_sizes[k], 8 * n, 8 * n, false, false, false);
    }
    // The soft discs, moved by 4 pixels in X, in Y, in both (a cell more in that direction)
    for (k = 0; k < JO_GLOW_SIZES; ++k)
    {
        n = __jo_glow_sizes[k] / 8;
        for (m = 0; m < 4; ++m)
            __jo_glow_disc_char[k][m] = 0;
        if (tier >= 2)
            continue;
        jo_glow_build_lut(__jo_glow_sizes[k], false);
        for (m = 0; m < 4; ++m)
        {
            if (m > 0 && (tier >= 1 || k >= JO_GLOW_PHASED_SIZES))
                break;
            __jo_glow_disc_char[k][m] = jo_glow_character(cell);
            cell = jo_glow_write_block(cell, n + (m & 1), n + (m >> 1), 1, __jo_glow_sizes[k], 8 * n + (m & 1) * 8,
                                       8 * n + (m >> 1) * 8, false, false, false);
        }
    }
    if (__jo_glow_lut != JO_NULL)
    {
        jo_free(__jo_glow_lut);
        __jo_glow_lut = JO_NULL;
    }
    __jo_glow_lut_half = 0;
    // The rectangles and the frames
    for (k = 0; k < JO_GLOW_MARGINS; ++k)
    {
        __jo_glow_rect_char[k] = jo_glow_character(cell);
        cell = jo_glow_write_rect(cell, k + 1, false);
        __jo_glow_frame_char[k] = jo_glow_character(cell);
        cell = jo_glow_write_rect(cell, k + 1, true);
    }
    // The pool of the overlaps
    __jo_glow_pool = cell;
    // The map: empty
    for (x = 0; x < JO_GLOW_MAP_WIDTH * JO_GLOW_MAP_WIDTH; ++x)
        __jo_glow_map[x] = __jo_glow_empty_char;
    __jo_glow_displayed.count = 0;
    __jo_glow_displayed.overlap_count = 0;
    // The light of 2 levels added
    for (y = 0; y < 16; ++y)
        for (x = 0; x < 16; ++x)
        {
            n = (int)jo_sqrt((unsigned int)(x * x + y * y) << 8);
            n = (n + 8) >> 4;
            __jo_glow_add_levels[y][x] = (unsigned char)JO_MIN(n, JO_GLOW_LEVELS);
        }
    return (true);
}

/** @brief A block of the color RAM for the palettes (once: the color RAM is not freed)
 *  @return false if the color RAM is full
 */
static bool                         jo_glow_alloc_palettes(void)
{
    unsigned short                  *p;
    unsigned int                    index;
    int                             k;
    int                             i;

    if (__jo_glow_cram_block >= 0)
        return (true);
    p = (unsigned short *)jo_vdp2_malloc_autosize(JO_VDP2_RAM_COLOR);
    if (p == JO_NULL)
        return (false);
    // (a palette of the engine starts at the second color of a block: the palettes 1 to 15 of 16 colors of the block)
    index = ((unsigned int)p - JO_VDP2_CRAM) / sizeof(unsigned short);
    __jo_glow_cram_block = (int)(index >> 8);
    __jo_glow_palettes = (volatile unsigned short *)(JO_VDP2_CRAM + (__jo_glow_cram_block << 8) * sizeof(unsigned short));
    for (i = 16; i < 256; ++i)
        __jo_glow_palettes[i] = 0;
    for (k = 0; k < JO_GLOW_SHOWN; ++k)
        for (i = 0; i < JO_GLOW_LEVELS; ++i)
            __jo_glow_displayed_colors[k][i] = 0;
    return (true);
}

/*
** The cells of a glow
*/

/** @brief Pattern name of a cell of a glow
 *  @param item Glow
 *  @param x Cell in its box (0 to w - 1)
 *  @param y Cell in its box (0 to h - 1)
 *  @return Pattern name, 0: no cell (the middle of a frame)
 */
static unsigned short               jo_glow_cell_name(const jo_glow_item * const item, const int x, const int y)
{
    unsigned int                    c;
    int                             n;
    int                             cn;
    int                             iw;
    int                             ih;
    int                             rx;
    int                             ry;
    int                             part_x;         // 0: left, 1: middle, 2: right
    int                             part_y;

    if (item->kind == JO_GLOW_ITEM_STAMP)
        return ((unsigned short)((item->palette << 12) | (item->first + y * item->w + x)));
    cn = item->margin;
    iw = item->inner_w;
    ih = item->inner_h;
    if (item->kind == JO_GLOW_ITEM_RECT)
    {
        n = cn;
        part_x = x < n ? 0 : (x >= n + iw ? 2 : 1);
        part_y = y < n ? 0 : (y >= n + ih ? 2 : 1);
        rx = part_x == 2 ? x - n - iw : x;
        ry = part_y == 2 ? y - n - ih : y;
    }
    else
    {
        // (a frame: the light on both sides of its line, 2 c cells; the box is the inner rectangle and c cells around)
        n = 2 * cn;
        if (x < n && (x < (iw + n) / 2 || iw + n - x > n))
            part_x = 0;
        else if (x >= iw)
            part_x = 2;
        else
            part_x = 1;
        if (y < n && (y < (ih + n) / 2 || ih + n - y > n))
            part_y = 0;
        else if (y >= ih)
            part_y = 2;
        else
            part_y = 1;
        rx = part_x == 2 ? x - iw : x;
        ry = part_y == 2 ? y - ih : y;
        if (part_x == 1 && part_y == 1)
            return (0);
    }
    c = item->first;
    if (part_x != 1 && part_y != 1)
        c += (unsigned int)(((part_y == 2) * 2 + (part_x == 2)) * n * n + ry * n + rx);
    else if (part_x == 1 && part_y != 1)
        c += (unsigned int)(4 * n * n + (part_y == 2) * n + ry);
    else if (part_y == 1 && part_x != 1)
        c += (unsigned int)(4 * n * n + 2 * n + (part_x == 2) * n + rx);
    else
        c += (unsigned int)(4 * n * n + 4 * n);
    return ((unsigned short)((item->palette << 12) | c));
}

/** @brief Put the light of a cell over another one (the light added), in a cell of the pool
 *  @param dst Cell of the pool
 *  @param a Pattern name of the cell under it (its character)
 *  @param b Pattern name of the cell over it
 */
static void                         jo_glow_add_cells(unsigned short *dst, const unsigned short a, const unsigned short b)
{
    const unsigned short            *pa;
    const unsigned short            *pb;
    unsigned int                    wa;
    unsigned int                    wb;
    unsigned int                    w;
    int                             i;
    unsigned int                    last_a;
    unsigned int                    last_b;
    unsigned int                    last_w;

    pa = jo_glow_character_address(a & 0xFFF);
    pb = jo_glow_character_address(b & 0xFFF);
    // (4 pixels of a row (a word): nothing under or over it, the other one as it is; the same 2 words as the last row
    // (the middle of a rectangle, the rows of a disc near its middle), the same sum. The images are the same; the sum
    // of the 4 pixels, with a shift by a variable, was the hottest line of the glows in a profile of the master CPU)
    last_a = 0x10000;
    JO_ZERO(last_b);
    JO_ZERO(last_w);
    for (i = 0; i < 16; ++i)
    {
        wa = pa[i];
        wb = pb[i];
        if (wa == 0)
            w = wb;
        else if (wb == 0)
            w = wa;
        else if (wa == last_a && wb == last_b)
            w = last_w;
        else
        {
            // (the shifts by constants: a shift by a variable is a call of the library on the SH-2)
            w = ((unsigned int)__jo_glow_add_levels[wa >> 12][wb >> 12] << 12) |
                ((unsigned int)__jo_glow_add_levels[(wa >> 8) & 15][(wb >> 8) & 15] << 8) |
                ((unsigned int)__jo_glow_add_levels[(wa >> 4) & 15][(wb >> 4) & 15] << 4) |
                __jo_glow_add_levels[wa & 15][wb & 15];
            last_a = wa;
            last_b = wb;
            last_w = w;
        }
        dst[i] = (unsigned short)w;
    }
}

/** @brief The cells seen of a glow (its box, its clip, the screen and a cell around it)
 *  @param item Glow
 *  @param box x0, y0, x1, y1 (cells, inclusive)
 */
static void                         jo_glow_seen_box(const jo_glow_item * const item, int * const box)
{
    box[0] = JO_MAX(JO_MAX(item->cx, item->clip_x0), -1);
    box[1] = JO_MAX(JO_MAX(item->cy, item->clip_y0), -1);
    box[2] = JO_MIN(JO_MIN(item->cx + item->w - 1, item->clip_x1), JO_MIN((__jo_screen.width >> 3) + 1, JO_GLOW_GRID_WIDTH - 2));
    box[3] = JO_MIN(JO_MIN(item->cy + item->h - 1, item->clip_y1), JO_MIN((__jo_screen.height >> 3) + 1, JO_GLOW_GRID_HEIGHT - 2));
}

/** @brief The cells of a glow where its light is strong: the box of a rectangle, the middle half of a disc (its edge and
 *  the ends of its rays are faint: there the brighter glow is drawn over the other one)
 *  @param item Glow
 *  @param box x0, y0, x1, y1 (cells, inclusive)
 */
static void                         jo_glow_core_box(const jo_glow_item * const item, int * const box)
{
    int                             m;

    jo_glow_seen_box(item, box);
    if (item->kind != JO_GLOW_ITEM_STAMP)
        return;
    m = item->w >> 2;
    box[0] = JO_MAX(box[0], item->cx + m);
    box[2] = JO_MIN(box[2], item->cx + item->w - 1 - m);
    m = item->h >> 2;
    box[1] = JO_MAX(box[1], item->cy + m);
    box[3] = JO_MIN(box[3], item->cy + item->h - 1 - m);
}

/** @brief The overlaps of the glows of a frame: in the cells where the strong light of a glow (see jo_glow_core_box())
 *  meets the strong light of a brighter one (before it), their light added in a cell of the pool (the palette of the
 *  brighter one); 3 glows or more: added in the same cell. Elsewhere and when the pool is full the brighter glow is
 *  drawn over the other one (the VBLANK writes the dimmer glows first). (All the cells of the boxes: 15 to 20 ms of the
 *  master CPU in a tunnel, 12 discs of 96 to 128 pixels; the characters are read in the VRAM)
 *  @param state The frame
 *  @param pool The pool of this frame
 */
static void                         jo_glow_overlaps(jo_glow_state * const state, unsigned char * const pool)
{
    const jo_glow_item              *a;
    const jo_glow_item              *b;
    unsigned short                  *dst;
    unsigned short                  under;
    unsigned short                  over;
    int                             box_a[4];
    int                             box_b[4];
    int                             i;
    int                             j;
    int                             x;
    int                             y;
    int                             x0;
    int                             y0;
    int                             x1;
    int                             y1;
    int                             e;

    state->overlap_count = 0;
    if (state->count < 2)
        return;
    for (y = 0; y < JO_GLOW_GRID_HEIGHT; ++y)
        jo_memset(__jo_glow_grid[y], 0, sizeof(__jo_glow_grid[y]));
    for (j = 1, b = state->items + 1; j < state->count && state->overlap_count < JO_GLOW_POOL; ++j, ++b)
    {
        jo_glow_core_box(b, box_b);
        for (i = 0, a = state->items; i < j; ++i, ++a)
        {
            jo_glow_core_box(a, box_a);
            x0 = JO_MAX(box_a[0], box_b[0]);
            y0 = JO_MAX(box_a[1], box_b[1]);
            x1 = JO_MIN(box_a[2], box_b[2]);
            y1 = JO_MIN(box_a[3], box_b[3]);
            for (y = y0; y <= y1; ++y)
                for (x = x0; x <= x1; ++x)
                {
                    over = jo_glow_cell_name(b, x - b->cx, y - b->cy);
                    if (over == 0)
                        continue;
                    e = __jo_glow_grid[y + 1][x + 1] & 0xFF;
                    if (e > 0)
                    {
                        // (a third glow there: added to the cell of the overlap once)
                        if ((__jo_glow_grid[y + 1][x + 1] >> 8) == j + 1)
                            continue;
                        dst = (unsigned short *)(pool + (e - 1) * 32);
                        jo_glow_add_cells(dst, state->overlap_name[e - 1], over);
                        __jo_glow_grid[y + 1][x + 1] = (unsigned short)(((j + 1) << 8) | e);
                        continue;
                    }
                    if (state->overlap_count >= JO_GLOW_POOL)
                        return;
                    under = jo_glow_cell_name(a, x - a->cx, y - a->cy);
                    if (under == 0)
                        continue;
                    dst = (unsigned short *)(pool + state->overlap_count * 32);
                    jo_glow_add_cells(dst, under, over);
                    state->overlap_cell[state->overlap_count] = (unsigned short)((y & 63) * JO_GLOW_MAP_WIDTH + (x & 63));
                    // (the palette of the brighter glow: a)
                    state->overlap_name[state->overlap_count] = (unsigned short)((under & 0xF000) |
                                                                                 jo_glow_character((const unsigned char *)dst));
                    ++state->overlap_count;
                    __jo_glow_grid[y + 1][x + 1] = (unsigned short)(((j + 1) << 8) | state->overlap_count);
                }
        }
    }
}

/*
** VBLANK: the layer shows the glows of the frame displayed
*/

static void                         jo_glow_vblank(void)
{
    const jo_glow_state             *state;
    const jo_glow_item              *item;
    volatile unsigned short         *map;
    unsigned short                  name;
    unsigned int                    tag;
    int                             k;
    int                             i;
    int                             x;
    int                             y;
    int                             box[4];

    if (!__jo_glows_enabled || !__jo_vdp1_displayed_frame_changed(&tag))
        return;
    state = &__jo_glow_states[tag % JO_GLOW_STATES];
    if (state->tag != tag)
        return;
    map = __jo_glow_map;
    // The cells of the frame before: empty
    for (k = 0; k < __jo_glow_displayed.count; ++k)
        for (y = __jo_glow_displayed.box[k][1]; y <= __jo_glow_displayed.box[k][3]; ++y)
            for (x = __jo_glow_displayed.box[k][0]; x <= __jo_glow_displayed.box[k][2]; ++x)
                map[(y & 63) * JO_GLOW_MAP_WIDTH + (x & 63)] = __jo_glow_empty_char;
    for (k = 0; k < __jo_glow_displayed.overlap_count; ++k)
        map[__jo_glow_displayed.overlap_cell[k]] = __jo_glow_empty_char;
    // The scroll: the grid of the cells on the glow seen the most
    JO_VDP2_SCXN2 = (unsigned short)((-(int)state->fine_x) & 0x7FF);
    JO_VDP2_SCYN2 = (unsigned short)((-(int)state->fine_y) & 0x7FF);
    // The glows, the dimmer ones first (the brighter one over them where the pool of the overlaps is full), then their
    // overlaps
    for (k = state->count - 1; k >= 0; --k)
    {
        item = &state->items[k];
        jo_glow_seen_box(item, box);
        __jo_glow_displayed.box[k][0] = (short)box[0];
        __jo_glow_displayed.box[k][1] = (short)box[1];
        __jo_glow_displayed.box[k][2] = (short)box[2];
        __jo_glow_displayed.box[k][3] = (short)box[3];
        if (item->kind == JO_GLOW_ITEM_STAMP)
        {
            // (a disc: its cells line by line)
            for (y = box[1]; y <= box[3]; ++y)
            {
                name = (unsigned short)((item->palette << 12) | (item->first + (y - item->cy) * item->w + box[0] - item->cx));
                for (x = box[0]; x <= box[2]; ++x, ++name)
                    map[(y & 63) * JO_GLOW_MAP_WIDTH + (x & 63)] = name;
            }
        }
        else
            for (y = box[1]; y <= box[3]; ++y)
                for (x = box[0]; x <= box[2]; ++x)
                {
                    name = jo_glow_cell_name(item, x - item->cx, y - item->cy);
                    if (name != 0)
                        map[(y & 63) * JO_GLOW_MAP_WIDTH + (x & 63)] = name;
                }
        // The palette of the glow
        for (i = 0; i < JO_GLOW_LEVELS; ++i)
        {
            if (state->colors[k][i] == __jo_glow_displayed_colors[k][i])
                continue;
            __jo_glow_displayed_colors[k][i] = state->colors[k][i];
            __jo_glow_palettes[(k + 1) * 16 + i + 1] = state->colors[k][i];
        }
    }
    __jo_glow_displayed.count = state->count;
    for (k = 0; k < state->overlap_count; ++k)
    {
        map[state->overlap_cell[k]] = state->overlap_name[k];
        __jo_glow_displayed.overlap_cell[k] = state->overlap_cell[k];
    }
    __jo_glow_displayed.overlap_count = state->overlap_count;
}

/*
** API
*/

int                                 jo_3d_glow_add(const jo_pos3D_fixed * const position, const unsigned char r, const unsigned char g,
                                                   const unsigned char b, const int radius, const int intensity)
{
    jo_glow                         *glow;
    int                             k;

    for (k = 0, glow = __jo_glows; k < JO_3D_GLOW_MAX; ++k, ++glow)
    {
        if (glow->used)
            continue;
        jo_memset(glow, 0, sizeof(*glow));
        glow->used = true;
        glow->shape = JO_3D_GLOW_STAR;
        glow->anchor = false;
        glow->position = *position;
        glow->r = r;
        glow->g = g;
        glow->b = b;
        glow->radius = (short)radius;
        glow->intensity = (short)JO_MIN(JO_MAX(intensity, 0), 256);
        return (k);
    }
    return (-1);
}

void                                jo_3d_glow_set_shape(const int id, const jo_3d_glow_shape shape, const int width, const int height)
{
    if (id < 0 || id >= JO_3D_GLOW_MAX)
        return;
    __jo_glows[id].shape = (unsigned char)shape;
    __jo_glows[id].width = (short)JO_MAX(width, 0);
    __jo_glows[id].height = (short)JO_MAX(height, 0);
}

void                                jo_3d_glow_set_anchor(const int id, const bool anchor)
{
    if (id >= 0 && id < JO_3D_GLOW_MAX)
        __jo_glows[id].anchor = anchor;
}

void                                jo_3d_glow_set_radius(const int id, const int radius)
{
    if (id >= 0 && id < JO_3D_GLOW_MAX)
        __jo_glows[id].radius = (short)radius;
}

void                                jo_3d_glow_set_intensity(const int id, const int intensity)
{
    if (id >= 0 && id < JO_3D_GLOW_MAX)
        __jo_glows[id].intensity = (short)JO_MIN(JO_MAX(intensity, 0), 256);
}

void                                jo_3d_glow_set_position(const int id, const jo_pos3D_fixed * const position)
{
    if (id < 0 || id >= JO_3D_GLOW_MAX)
        return;
    __jo_glows[id].position = *position;
    __jo_glows[id].tested = false;
}

void                                jo_3d_glow_set_color(const int id, const unsigned char r, const unsigned char g, const unsigned char b)
{
    if (id < 0 || id >= JO_3D_GLOW_MAX)
        return;
    __jo_glows[id].r = r;
    __jo_glows[id].g = g;
    __jo_glows[id].b = b;
}

void                                jo_3d_glows_clear(void)
{
    jo_memset(__jo_glows, 0, sizeof(__jo_glows));
}

void                                jo_3d_glows_set_occlusion(bool (*hit)(const jo_pos3D_fixed * const origin, const jo_pos3D_fixed * const dir,
                                                                          const jo_fixed max_t))
{
    __jo_glow_hit = hit;
}

void                                jo_3d_glows_set_clip(bool (*clip)(const jo_pos3D_fixed * const position, jo_3d_rect * const rect))
{
    __jo_glow_clip = clip;
}

void                                jo_3d_glows_set_rays(const int light, const int diagonal_light)
{
    __jo_glow_ray_light = JO_MAX(0, JO_MIN(light, 256));
    __jo_glow_diagonal_ray_light = JO_MAX(0, JO_MIN(diagonal_light, 256));
}

bool                                jo_3d_glows_enable(const bool enabled)
{
    int                             k;
    int                             i;

    if (enabled == __jo_glows_enabled)
        return (true);
    if (!enabled)
    {
        __jo_glows_enabled = false;
        __jo_vdp2_glow_layers_hide();
        return (true);
    }
    if (!jo_glow_alloc_palettes() || !jo_glow_build_vram())
        return (false);
    for (k = 0; k < JO_GLOW_STATES; ++k)
        __jo_glow_states[k].tag = ~0u;
    // Nothing displayed until the first frame of the glows
    for (k = 0; k < JO_GLOW_MAP_WIDTH * JO_GLOW_MAP_WIDTH; ++k)
        __jo_glow_map[k] = __jo_glow_empty_char;
    __jo_glow_displayed.count = 0;
    __jo_glow_displayed.overlap_count = 0;
    for (k = 0; k < JO_GLOW_SHOWN; ++k)
        for (i = 0; i < JO_GLOW_LEVELS; ++i)
            __jo_glow_displayed_colors[k][i] = 0xFFFF;
    if (__jo_glow_vblank_event < 0)
        __jo_glow_vblank_event = jo_core_add_vblank_callback(jo_glow_vblank);
    // (one layer: NBG2; NBG3 stays free)
    __jo_vdp2_glow_layers_show(__jo_glow_cells, (const void *)__jo_glow_map, JO_NULL, __jo_glow_cram_block);
    __jo_glows_enabled = true;
    return (true);
}

/** @brief Cast the ray from the camera to a lamp
 *  @param glow Glow
 *  @param camera Camera (world)
 */
static void                         jo_glow_test(jo_glow * const glow, const jo_pos3D_fixed * const camera)
{
    jo_pos3D_fixed                  dir;
    int                             length;
    int                             dx;
    int                             dy;
    int                             dz;
    int                             scale;
    int                             scaled;

    glow->tested = true;
    glow->tested_from = *camera;
    if (__jo_glow_hit == JO_NULL)
    {
        glow->seen = true;
        return;
    }
    // (a far lamp in a big level: the differences halved until the sum of their squares and jo_int2fixed() fit, the sum
    // overflowed above about 26000 world units, the ray went in another direction)
    dx = jo_fixed2int(glow->position.x - camera->x);
    dy = jo_fixed2int(glow->position.y - camera->y);
    dz = jo_fixed2int(glow->position.z - camera->z);
    for (JO_ZERO(scale); JO_ABS(dx) > 16383 || JO_ABS(dy) > 16383 || JO_ABS(dz) > 16383; ++scale)
    {
        dx >>= 1;
        dy >>= 1;
        dz >>= 1;
    }
    scaled = (int)jo_sqrt((unsigned int)(dx * dx) + (unsigned int)(dy * dy) + (unsigned int)(dz * dz));
    length = scaled << scale;
    if (length <= JO_GLOW_LAMP_SIZE || scaled == 0)
    {
        glow->seen = true;
        return;
    }
    dir.x = jo_int2fixed(dx) / scaled;
    dir.y = jo_int2fixed(dy) / scaled;
    dir.z = jo_int2fixed(dz) / scaled;
    glow->seen = !__jo_glow_hit(camera, &dir, jo_int2fixed(JO_MIN(length - JO_GLOW_LAMP_SIZE, 32767)));
}

/** @brief The nearest size of disc
 *  @param diameter Diameter on the screen (pixels)
 *  @return Index in __jo_glow_sizes
 */
static int                          jo_glow_nearest_size(const int diameter)
{
    int                             size;
    int                             s;

    size = 0;
    for (s = 1; s < JO_GLOW_SIZES; ++s)
        if (JO_ABS(__jo_glow_sizes[s] - diameter) < JO_ABS(__jo_glow_sizes[size] - diameter))
            size = s;
    return (size);
}

/** @brief A cell and the pixel in it (floor), the grid moved by fine pixels */
static  __jo_force_inline int       jo_glow_cell_of(const int pixel, const int fine, int * const rest)
{
    int                             p;

    p = pixel - fine + 1024;
    *rest = p & 7;
    return ((p >> 3) - 128);
}

/** @brief The cell of a star (8 pixels: its place within 4 pixels): the nearest one, or the one of the frame before while
 *  it is within JO_GLOW_STICKY pixels of the place of the lamp (a lamp that moves by one pixel, the camera shaking with the
 *  car standing, the grid moved by the anchored glow, jumped by 8 pixels between 2 cells at each frame: far away, a star of
 *  16 pixels jumped by half its size)
 *  @param pixel The top left pixel of the star (exact)
 *  @param fine The scroll of the grid
 *  @param placed The top left pixel of the star in the frame before (-32768: none)
 *  @return Cell
 */
#define JO_GLOW_STICKY                      (6)

static int                          jo_glow_sticky_cell(const int pixel, const int fine, const int placed)
{
    int                             rest;
    int                             m;
    int                             k;

    m = jo_glow_cell_of(pixel, fine, &rest);
    m += rest >= 4;
    if (placed == -32768)
        return (m);
    // (the cell of the grid nearest to the place of the frame before)
    k = jo_glow_cell_of(placed, fine, &rest);
    k += rest >= 4;
    if (k != m && JO_ABS(k * 8 + fine - pixel) <= JO_GLOW_STICKY)
        return (k);
    return (m);
}

void                                jo_3d_glows_draw(const jo_pos3D_fixed * const camera)
{
    jo_glow_state                   *state;
    jo_glow_item                    *item;
    jo_glow                         *glow;
    jo_pos3D_fixed                  v;
    jo_fixed                        m[4][3];
    jo_fixed                        columns[3][3];
    jo_fixed                        k;
    unsigned char                   *pool;
    short                           sx[JO_3D_GLOW_MAX];
    short                           sy[JO_3D_GLOW_MAX];
    short                           diameter[JO_3D_GLOW_MAX];     // Disc: its diameter; rectangle: the width of its light
    short                           half_w[JO_3D_GLOW_MAX];       // Rectangle: its half size on the screen
    short                           half_h[JO_3D_GLOW_MAX];
    int                             score[JO_3D_GLOW_MAX];
    bool                            on_screen[JO_3D_GLOW_MAX];
    jo_3d_rect                      rect[JO_3D_GLOW_MAX];
    unsigned char                   order[JO_GLOW_SHOWN];
    int                             light[JO_GLOW_SHOWN];
    unsigned int                    tag;
    int                             count;
    int                             i;
    int                             j;
    int                             s;
    int                             rays;
    int                             size;
    int                             reach;
    int                             left;
    int                             top;
    int                             rx;
    int                             ry;
    int                             mx;
    int                             my;
    int                             c;

    if (!__jo_glows_enabled)
        return;
    tag = __jo_vdp1_frame_tag();
    // (the columns of the matrix once for all the glows: the same products as jo_3d_transform_point())
    jo_3d_get_matrix((jo_fixed *)m);
    for (i = 0; i < 3; ++i)
    {
        columns[i][0] = m[0][i];
        columns[i][1] = m[1][i];
        columns[i][2] = m[2][i];
    }
    // Where each glow is on the screen
    for (i = 0, glow = __jo_glows; i < JO_3D_GLOW_MAX; ++i, ++glow)
    {
        on_screen[i] = false;
        score[i] = 0;
        if (!glow->used)
            continue;
        v.x = jo_fixed_dot((const jo_fixed *)&glow->position, columns[0]) + m[3][0];
        v.y = jo_fixed_dot((const jo_fixed *)&glow->position, columns[1]) + m[3][1];
        v.z = jo_fixed_dot((const jo_fixed *)&glow->position, columns[2]) + m[3][2];
        if (v.z <= JO_FIXED_16)
            continue;
        k = jo_fixed_div(jo_3d_get_projection_distance(), v.z);
        sx[i] = (short)jo_fixed2int(jo_fixed_mult(v.x, k));
        sy[i] = (short)jo_fixed2int(jo_fixed_mult(v.y, k));
        diameter[i] = (short)JO_MIN(jo_fixed2int(jo_fixed_mult(jo_int2fixed(glow->radius * 2), k)), 1024);
        half_w[i] = 0;
        half_h[i] = 0;
        reach = JO_MIN(diameter[i], 128) / 2;
        if (glow->shape >= JO_3D_GLOW_RECT)
        {
            half_w[i] = (short)JO_MIN(jo_fixed2int(jo_fixed_mult(jo_int2fixed(glow->width), k)) / 2, 512);
            half_h[i] = (short)JO_MIN(jo_fixed2int(jo_fixed_mult(jo_int2fixed(glow->height), k)) / 2, 512);
            reach = 24 + JO_MAX(half_w[i], half_h[i]);
        }
        on_screen[i] = sx[i] + reach > -__jo_screen.width_2 && sx[i] - reach < __jo_screen.width_2 &&
                       sy[i] + reach > -__jo_screen.height_2 && sy[i] - reach < __jo_screen.height_2;
        // Seen only where the place of the lamp is seen (the rectangle of a door: the walls around it hide its glow)
        rect[i].left = 0;
        rect[i].top = 0;
        rect[i].right = __jo_screen.width - 1;
        rect[i].bottom = __jo_screen.height - 1;
        if (on_screen[i] && __jo_glow_clip != JO_NULL)
        {
            on_screen[i] = __jo_glow_clip(&glow->position, &rect[i]);
            // (the glow and the rectangle)
            on_screen[i] = on_screen[i] && sx[i] + __jo_screen.width_2 + reach > rect[i].left && sx[i] + __jo_screen.width_2 - reach < rect[i].right &&
                           sy[i] + __jo_screen.height_2 + reach > rect[i].top && sy[i] + __jo_screen.height_2 - reach < rect[i].bottom;
        }
    }
    // The rays (the glows on the screen not tested first, from the one after the last tested: the glows moved at each
    // frame (the lights of a car) took all the rays, the others were never tested; then in turn). No occlusion: all
    // seen
    for (rays = 0, j = 0; j < JO_3D_GLOW_MAX && rays < JO_GLOW_RAYS_PER_FRAME; ++j)
    {
        i = (__jo_glow_ray_cursor + 1 + j) % JO_3D_GLOW_MAX;
        if (on_screen[i] && (!__jo_glows[i].tested || __jo_glow_hit == JO_NULL))
        {
            jo_glow_test(&__jo_glows[i], camera);
            if (__jo_glow_hit != JO_NULL)
                ++rays;
        }
    }
    for (j = 0; j < JO_3D_GLOW_MAX && rays < JO_GLOW_RAYS_PER_FRAME; ++j)
    {
        __jo_glow_ray_cursor = (__jo_glow_ray_cursor + 1) % JO_3D_GLOW_MAX;
        glow = &__jo_glows[__jo_glow_ray_cursor];
        // (the camera didn't move since the last ray of this glow: the same result, the ray for another glow)
        if (on_screen[__jo_glow_ray_cursor] && (glow->tested_from.x != camera->x || glow->tested_from.y != camera->y ||
                                                glow->tested_from.z != camera->z))
        {
            jo_glow_test(&__jo_glows[__jo_glow_ray_cursor], camera);
            ++rays;
        }
    }
    // Fade, score (the light seen: intensity x size), the brightest ones (JO_GLOW_SHOWN at most)
    count = 0;
    for (i = 0, glow = __jo_glows; i < JO_3D_GLOW_MAX; ++i, ++glow)
    {
        if (!glow->used)
            continue;
        if (on_screen[i] && glow->seen && glow->intensity > 0)
            glow->fade = (short)JO_MIN(glow->fade + JO_GLOW_FADE_STEP, 256);
        else
            glow->fade = (short)JO_MAX(glow->fade - JO_GLOW_FADE_STEP, 0);
        if (!on_screen[i] || glow->fade <= 0)
            continue;
        s = glow->shape >= JO_3D_GLOW_RECT ? JO_MIN(2 * JO_MAX(half_w[i], half_h[i]) + diameter[i], 128) : JO_MIN(diameter[i], 128);
        score[i] = (glow->intensity * glow->fade >> 8) * JO_MAX(s, 4);
        // (an anchored glow first: the grid of the cells on it, placed to the pixel)
        if (glow->anchor)
            score[i] += 0x10000000;
        // (sorted, the brightest first)
        if (count == JO_GLOW_SHOWN && score[order[JO_GLOW_SHOWN - 1]] >= score[i])
            continue;
        for (j = JO_MIN(count, JO_GLOW_SHOWN - 1); j > 0 && score[order[j - 1]] < score[i]; --j)
            order[j] = order[j - 1];
        order[j] = (unsigned char)i;
        count = JO_MIN(count + 1, JO_GLOW_SHOWN);
    }
    // The frame: the grid of the cells on the brightest glow, then each glow in the cells
    state = &__jo_glow_states[tag % JO_GLOW_STATES];
    pool = __jo_glow_pool + (tag % JO_GLOW_STATES) * JO_GLOW_POOL * 32;
    state->count = 0;
    state->overlap_count = 0;
    state->fine_x = 0;
    state->fine_y = 0;
    for (j = 0; j < count; ++j)
    {
        i = order[j];
        glow = &__jo_glows[i];
        item = &state->items[state->count];
        light[state->count] = glow->intensity * glow->fade >> 8;
        rx = sx[i] + __jo_screen.width_2;
        ry = sy[i] + __jo_screen.height_2;
        if (glow->shape >= JO_3D_GLOW_RECT)
        {
            // A rectangle: its cells, the width of its light (the nearest of the pieces: 8, 16, 24 pixels)
            c = JO_MIN(JO_MAX((diameter[i] / 2 + 4) / 8, 1), JO_GLOW_MARGINS);
            if (diameter[i] < 8)
                light[state->count] = light[state->count] * JO_MAX(diameter[i], 1) / 8;
            left = rx - half_w[i];
            top = ry - half_h[i];
            if (state->count == 0)
            {
                state->fine_x = (unsigned char)(left & 7);
                state->fine_y = (unsigned char)(top & 7);
            }
            mx = jo_glow_cell_of(left, state->fine_x, &rx);
            my = jo_glow_cell_of(top, state->fine_y, &ry);
            mx += rx >= 4;
            my += ry >= 4;
            item->inner_w = (unsigned char)JO_MIN(JO_MAX((2 * half_w[i] + 4) / 8, 1), 64);
            item->inner_h = (unsigned char)JO_MIN(JO_MAX((2 * half_h[i] + 4) / 8, 1), 64);
            item->margin = (unsigned char)c;
            if (glow->shape == JO_3D_GLOW_RECT)
            {
                item->kind = JO_GLOW_ITEM_RECT;
                item->first = __jo_glow_rect_char[c - 1];
                item->cx = (short)(mx - c);
                item->cy = (short)(my - c);
                item->w = (unsigned char)JO_MIN(item->inner_w + 2 * c, 255);
                item->h = (unsigned char)JO_MIN(item->inner_h + 2 * c, 255);
            }
            else
            {
                item->kind = JO_GLOW_ITEM_FRAME;
                item->first = __jo_glow_frame_char[c - 1];
                item->cx = (short)(mx - c);
                item->cy = (short)(my - c);
                item->w = (unsigned char)JO_MIN(item->inner_w + 2 * c, 255);
                item->h = (unsigned char)JO_MIN(item->inner_h + 2 * c, 255);
                // (the frame: its inner size is the box less the light on one side, see jo_glow_cell_name())
                item->inner_w = item->w > 2 * c ? (unsigned char)(item->w - 2 * c) : 0;
                item->inner_h = item->h > 2 * c ? (unsigned char)(item->h - 2 * c) : 0;
            }
        }
        else
        {
            // A disc: the nearest size; smaller than the smallest disc: less light
            size = jo_glow_nearest_size(diameter[i]);
            if (diameter[i] < __jo_glow_sizes[0])
                light[state->count] = light[state->count] * diameter[i] * diameter[i] / (__jo_glow_sizes[0] * __jo_glow_sizes[0]);
            s = __jo_glow_sizes[size];
            left = rx - s / 2;
            top = ry - s / 2;
            if (state->count == 0)
            {
                state->fine_x = (unsigned char)(left & 7);
                state->fine_y = (unsigned char)(top & 7);
            }
            mx = jo_glow_cell_of(left, state->fine_x, &rx);
            my = jo_glow_cell_of(top, state->fine_y, &ry);
            item->kind = JO_GLOW_ITEM_STAMP;
            item->w = (unsigned char)(s / 8);
            item->h = (unsigned char)(s / 8);
            if (glow->shape == JO_3D_GLOW_DISC && __jo_glow_disc_char[size][0] != 0)
            {
                // (moved by 4 pixels when it is nearer: the box a cell bigger)
                c = 0;
                if (__jo_glow_disc_char[size][1] != 0)
                {
                    if (rx >= 2 && rx < 6)
                        c |= 1;
                    if (ry >= 2 && ry < 6)
                        c |= 2;
                }
                mx += rx >= ((c & 1) ? 8 : 4);
                my += ry >= ((c & 2) ? 8 : 4);
                item->first = __jo_glow_disc_char[size][c];
                item->w += c & 1;
                item->h += c >> 1;
            }
            else
            {
                // (a star: in the cell of the frame before while it is near enough, see jo_glow_sticky_cell())
                mx = jo_glow_sticky_cell(left, state->fine_x, glow->placed ? glow->placed_x : -32768);
                my = jo_glow_sticky_cell(top, state->fine_y, glow->placed ? glow->placed_y : -32768);
                glow->placed_x = (short)(mx * 8 + state->fine_x);
                glow->placed_y = (short)(my * 8 + state->fine_y);
                glow->placed = true;
                item->first = __jo_glow_star_char[size];
            }
            item->cx = (short)mx;
            item->cy = (short)my;
        }
        if (light[state->count] <= 0)
            continue;
        item->palette = (unsigned char)(state->count + 1);
        // The cells seen (the clip of the glow, moved like the grid)
        item->clip_x0 = (short)jo_glow_cell_of(rect[i].left, state->fine_x, &rx);
        item->clip_y0 = (short)jo_glow_cell_of(rect[i].top, state->fine_y, &ry);
        item->clip_x1 = (short)jo_glow_cell_of(rect[i].right, state->fine_x, &rx);
        item->clip_y1 = (short)jo_glow_cell_of(rect[i].bottom, state->fine_y, &ry);
        // (the light of a level: (level / 15)^2: a soft halo, the light falls quickly away from the lamp. Linear, the halo
        // was a wide bright disc with a visible edge)
        s = light[state->count];
        for (c = 0; c < JO_GLOW_LEVELS; ++c)
            state->colors[state->count][c] = JO_COLOR_RGB(glow->r * s * (c + 1) * (c + 1) / (256 * JO_GLOW_LEVELS * JO_GLOW_LEVELS),
                                                          glow->g * s * (c + 1) * (c + 1) / (256 * JO_GLOW_LEVELS * JO_GLOW_LEVELS),
                                                          glow->b * s * (c + 1) * (c + 1) / (256 * JO_GLOW_LEVELS * JO_GLOW_LEVELS));
        ++state->count;
    }
    jo_glow_overlaps(state, pool);
    state->tag = tag;
}

#endif /* !JO_COMPILE_WITH_3D_SUPPORT */

/*
** END OF FILE
*/
