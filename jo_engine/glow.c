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
** VRAM (bank B0 of the VDP2, one block): the map of NBG2, the map of NBG3 (1 page of 64 x 64 cells, 8 KB, aligned on 8
** KB), then the characters (16 colors, 32 bytes per cell): an empty cell, then a disc (with its rays) for each size
** (its cells line by line). The disc of a layer is in the 16 x 16 cells at the top left of its map (the other cells are
** empty): its scroll puts its middle on the lamp. The plane (512 x 512 pixels) repeats: the other copies are out of the
** screen.
** Levels of light 1 to 15 (0: transparent), the light of each level in the palette of the layer (its color x intensity)
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

#ifdef JO_COMPILE_WITH_3D_SUPPORT

/** @brief VDP2 layers (NBG2, NBG3): glows displayed at the same time */
#define JO_GLOW_LAYERS                      (2)
/** @brief Sizes of the discs (pixels, multiples of 8) */
#define JO_GLOW_SIZES                       (12)
static const unsigned char                  __jo_glow_sizes[JO_GLOW_SIZES] = {16, 24, 32, 40, 48, 56, 64, 72, 80, 96, 112, 128};
/** @brief Cells of the biggest disc (16 x 16): the block of the map that holds the disc */
#define JO_GLOW_BLOCK_CELLS                 (16)
/** @brief Cells in a line of the map (1 page) */
#define JO_GLOW_MAP_WIDTH                   (64)
/** @brief Size of a map (1 page of 64 x 64 cells, 1 word per cell) */
#define JO_GLOW_MAP_BYTES                   (8192)
/** @brief Light levels (16 colors characters: 1 to 15, 0 is transparent) */
#define JO_GLOW_LEVELS                      (15)
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

typedef struct
{
    bool                            used;
    bool                            seen;           // No wall between the camera and the lamp (last ray)
    bool                            tested;         // A ray was cast since the glow was added or moved
    jo_pos3D_fixed                  position;
    jo_pos3D_fixed                  tested_from;    // The camera of the last ray (the same camera: the same result)
    unsigned char                   r;
    unsigned char                   g;
    unsigned char                   b;
    short                           radius;
    short                           intensity;
    short                           fade;           // 0 (hidden) to 256 (seen)
    char                            layer;          // Layer of the last frame, -1: none
}                                   jo_glow;

/** @brief What a layer displays in a frame */
typedef struct
{
    short                           scroll_x;
    short                           scroll_y;
    char                            size;           // Index in __jo_glow_sizes, -1: nothing
    unsigned short                  colors[JO_GLOW_LEVELS];
    jo_3d_rect                      rect;           // The disc is seen only in this rectangle (its window)
}                                   jo_glow_layer_state;

typedef struct
{
    unsigned int                    tag;            // Frame (see __jo_vdp1_frame_tag())
    jo_glow_layer_state             layers[JO_GLOW_LAYERS];
}                                   jo_glow_state;

void                                __jo_vdp2_glow_layers_show(const void *cells, const void *map2, const void *map3, const int cram_block);
void                                __jo_vdp2_glow_layers_hide(void);

static jo_glow                      __jo_glows[JO_3D_GLOW_MAX];
static jo_glow_state                __jo_glow_states[JO_GLOW_STATES];
static bool                         __jo_glows_enabled = false;
static int                          __jo_glow_vblank_event = -1;
static unsigned char                *__jo_glow_vram = JO_NULL;
static volatile unsigned short      *__jo_glow_maps[JO_GLOW_LAYERS];
static unsigned char                *__jo_glow_cells;
/** @brief Character number of the first cell of each disc, of the empty cell */
static unsigned short               __jo_glow_first_char[JO_GLOW_SIZES];
static unsigned short               __jo_glow_empty_char;
/** @brief Color RAM: block of 256 colors, first color of the palette of each layer */
static int                          __jo_glow_cram_block = -1;
static volatile unsigned short      *__jo_glow_palettes[JO_GLOW_LAYERS];
/** @brief What the layers display now (written during the VBLANK) */
static char                         __jo_glow_displayed_size[JO_GLOW_LAYERS] = {-1, -1};
static unsigned short               __jo_glow_displayed_colors[JO_GLOW_LAYERS][JO_GLOW_LEVELS];
static bool                         (*__jo_glow_hit)(const jo_pos3D_fixed * const origin, const jo_pos3D_fixed * const dir, const jo_fixed max_t) = JO_NULL;
static int                          __jo_glow_ray_cursor = 0;
static bool                         (*__jo_glow_clip)(const jo_pos3D_fixed * const position, jo_3d_rect * const rect) = JO_NULL;
/** @brief Window control of NBG2 and NBG3 (WCTLB): the outside of the window 0 (NBG2), of the window 1 (NBG3) is transparent */
#define JO_GLOW_WINDOWS                     ((0x3 << 0) | (0xC << 8))

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
    return ((jo_fixed_mult(fall, width) * light) >> 8);
}

/** @brief Light of a pixel of a glow: a disc (1 - d^2)^4 (d: distance to the middle / radius, 0 at the edge) and 8 rays (4 long
 *  ones on the axes, 4 shorter ones on the diagonals)
 *  @param dx Distance to the middle along X (half pixels: the middle is between two pixels)
 *  @param dy Distance to the middle along Y (half pixels)
 *  @param size Size of the disc
 *  @return Light (Q16, 0 to 1)
 */
static jo_fixed                     jo_glow_light(const int dx, const int dy, const int size)
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
    // The rays on the axes (to the edge of the square)
    ray = jo_glow_ray(JO_MIN(dx, dy), JO_MAX(dx, dy), size, __jo_glow_ray_light);
    // The rays on the diagonals (x 1 / sqrt(2): the distances along the diagonals)
    u = ((dx + dy) * 181) >> 8;
    v = (JO_ABS(dx - dy) * 181) >> 8;
    ray = JO_MAX(ray, jo_glow_ray(JO_MIN(u, v), JO_MAX(u, v), size * 3 / 4, __jo_glow_diagonal_ray_light));
    return (JO_MIN(q + ray, JO_FIXED_1));
}

/** @brief Level of a pixel of a glow, with an ordered dither between two levels (no ring)
 *  @param x Pixel
 *  @param y Pixel
 *  @param q Its light (see jo_glow_light())
 *  @return Level (0 to 15)
 */
static int                          jo_glow_level(const int x, const int y, const jo_fixed q)
{
    static const unsigned char      bayer[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};
    int                             level;

    level = (q * JO_GLOW_LEVELS + (bayer[(y & 3) * 4 + (x & 3)] << 12) + (1 << 11)) >> 16;
    return (JO_MIN(level, JO_GLOW_LEVELS));
}

/** @brief Write the characters of the discs (16 colors: 4 bits per pixel, the left pixel in the high bits)
 *  @return false if the VDP2 memory is full
 */
static bool                         jo_glow_build_vram(void)
{
    volatile unsigned short         *dst;
    unsigned char                   *cell;
    unsigned int                    cells;
    unsigned int                    start;
    int                             k;
    int                             n;
    int                             cx;
    int                             cy;
    int                             y;
    int                             x;
    unsigned short                  w;
    jo_fixed                        *light;
    jo_fixed                        q;
    int                             half;
    int                             size;
    int                             dx;
    int                             dy;
    int                             i;

    if (__jo_glow_vram != JO_NULL)
        return (true);
    // The empty cell and the discs
    cells = 1;
    for (k = 0; k < JO_GLOW_SIZES; ++k)
        cells += (__jo_glow_sizes[k] / 8) * (__jo_glow_sizes[k] / 8);
    // (2 maps aligned on their size: up to a map more)
    __jo_glow_vram = (unsigned char *)jo_vdp2_malloc(JO_VDP2_RAM_MAP_NBG1, 3 * JO_GLOW_MAP_BYTES + cells * 32);
    if (__jo_glow_vram == JO_NULL)
        return (false);
    start = ((unsigned int)__jo_glow_vram + JO_GLOW_MAP_BYTES - 1) & ~(JO_GLOW_MAP_BYTES - 1);
    __jo_glow_maps[0] = (volatile unsigned short *)start;
    __jo_glow_maps[1] = (volatile unsigned short *)(start + JO_GLOW_MAP_BYTES);
    __jo_glow_cells = (unsigned char *)(start + 2 * JO_GLOW_MAP_BYTES);
    // The empty cell
    dst = (volatile unsigned short *)__jo_glow_cells;
    for (x = 0; x < 16; ++x)
        dst[x] = 0;
    __jo_glow_empty_char = jo_glow_character(__jo_glow_cells);
    cell = __jo_glow_cells + 32;
    // The light of a quarter of the largest disc (the 4 quarters are the same but for the dither: 4 times less computed,
    // the start of the glows took about 0.6 s)
    half = __jo_glow_sizes[JO_GLOW_SIZES - 1] / 2;
    light = (jo_fixed *)jo_malloc((unsigned int)(half * half) * sizeof(*light));
    for (k = 0; k < JO_GLOW_SIZES; ++k)
    {
        size = __jo_glow_sizes[k];
        half = size / 2;
        if (light != JO_NULL)
            for (y = 0; y < half; ++y)
                for (x = 0; x < half; ++x)
                    light[y * half + x] = jo_glow_light(2 * x + 1, 2 * y + 1, size);
        n = size / 8;
        __jo_glow_first_char[k] = jo_glow_character(cell);
        for (cy = 0; cy < n; ++cy)
            for (cx = 0; cx < n; ++cx, cell += 32)
            {
                dst = (volatile unsigned short *)cell;
                for (y = 0; y < 8; ++y)
                    for (x = 0; x < 8; x += 4)
                    {
                        for (JO_ZERO(i), JO_ZERO(w); i < 4; ++i)
                        {
                            // (the distances to the middle, half pixels: |2 x + 1 - size|, odd)
                            dx = JO_ABS(2 * (cx * 8 + x + i) + 1 - size);
                            dy = JO_ABS(2 * (cy * 8 + y) + 1 - size);
                            q = light != JO_NULL ? light[(dy >> 1) * half + (dx >> 1)] : jo_glow_light(dx, dy, size);
                            w = (unsigned short)((w << 4) | jo_glow_level(cx * 8 + x + i, cy * 8 + y, q));
                        }
                        *dst++ = w;
                    }
            }
    }
    if (light != JO_NULL)
        jo_free(light);
    // The maps: empty
    for (k = 0; k < JO_GLOW_LAYERS; ++k)
    {
        for (x = 0; x < JO_GLOW_MAP_WIDTH * JO_GLOW_MAP_WIDTH; ++x)
            __jo_glow_maps[k][x] = __jo_glow_empty_char;
        __jo_glow_displayed_size[k] = -1;
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
    // (a palette of the engine starts at the second color of a block: the palettes 1 and 2 of 16 colors of the block)
    index = ((unsigned int)p - JO_VDP2_CRAM) / sizeof(unsigned short);
    __jo_glow_cram_block = (int)(index >> 8);
    for (k = 0; k < JO_GLOW_LAYERS; ++k)
    {
        __jo_glow_palettes[k] = (volatile unsigned short *)(JO_VDP2_CRAM + ((__jo_glow_cram_block << 8) + (k + 1) * 16) * sizeof(unsigned short));
        for (i = 0; i < 16; ++i)
            __jo_glow_palettes[k][i] = 0;
        for (i = 0; i < JO_GLOW_LEVELS; ++i)
            __jo_glow_displayed_colors[k][i] = 0;
    }
    return (true);
}

/*
** VBLANK: the layers show the glows of the frame displayed
*/

/** @brief Put a disc in the map of a layer (the block at the top left)
 *  @param layer Layer
 *  @param size Index of the size, -1: nothing
 */
static void                         jo_glow_write_map(const int layer, const int size)
{
    volatile unsigned short         *map;
    unsigned short                  palette;
    unsigned short                  c;
    int                             n;
    int                             x;
    int                             y;

    map = __jo_glow_maps[layer];
    palette = (unsigned short)((layer + 1) << 12);
    n = size >= 0 ? __jo_glow_sizes[size] / 8 : 0;
    c = size >= 0 ? __jo_glow_first_char[size] : 0;
    for (y = 0; y < JO_GLOW_BLOCK_CELLS; ++y, map += JO_GLOW_MAP_WIDTH)
        for (x = 0; x < JO_GLOW_BLOCK_CELLS; ++x)
            map[x] = (x < n && y < n) ? (unsigned short)(palette | (c + y * n + x)) : __jo_glow_empty_char;
}

static void                         jo_glow_vblank(void)
{
    const jo_glow_state             *state;
    const jo_glow_layer_state       *l;
    unsigned int                    tag;
    int                             k;
    int                             i;

    if (!__jo_glows_enabled || !__jo_vdp1_displayed_frame_changed(&tag))
        return;
    state = &__jo_glow_states[tag % JO_GLOW_STATES];
    if (state->tag != tag)
        return;
    for (k = 0, l = state->layers; k < JO_GLOW_LAYERS; ++k, ++l)
    {
        if (l->size != __jo_glow_displayed_size[k])
        {
            jo_glow_write_map(k, l->size);
            __jo_glow_displayed_size[k] = l->size;
        }
        if (l->size < 0)
            continue;
        // (integer scroll of NBG2 and NBG3; the window: X in half pixels, Y in lines)
        if (k == 0)
        {
            JO_VDP2_SCXN2 = (unsigned short)(l->scroll_x & 0x7FF);
            JO_VDP2_SCYN2 = (unsigned short)(l->scroll_y & 0x7FF);
            JO_VDP2_WPSX0 = (unsigned short)(l->rect.left << !__jo_screen.hires);
            JO_VDP2_WPSY0 = (unsigned short)l->rect.top;
            JO_VDP2_WPEX0 = (unsigned short)(l->rect.right << !__jo_screen.hires);
            JO_VDP2_WPEY0 = (unsigned short)l->rect.bottom;
        }
        else
        {
            JO_VDP2_SCXN3 = (unsigned short)(l->scroll_x & 0x7FF);
            JO_VDP2_SCYN3 = (unsigned short)(l->scroll_y & 0x7FF);
            JO_VDP2_WPSX1 = (unsigned short)(l->rect.left << !__jo_screen.hires);
            JO_VDP2_WPSY1 = (unsigned short)l->rect.top;
            JO_VDP2_WPEX1 = (unsigned short)(l->rect.right << !__jo_screen.hires);
            JO_VDP2_WPEY1 = (unsigned short)l->rect.bottom;
        }
        JO_VDP2_WCTLB = JO_GLOW_WINDOWS;
        for (i = 0; i < JO_GLOW_LEVELS; ++i)
        {
            if (l->colors[i] == __jo_glow_displayed_colors[k][i])
                continue;
            __jo_glow_displayed_colors[k][i] = l->colors[i];
            __jo_glow_palettes[k][i + 1] = l->colors[i];
        }
    }
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
        glow->position = *position;
        glow->r = r;
        glow->g = g;
        glow->b = b;
        glow->radius = (short)radius;
        glow->intensity = (short)JO_MIN(JO_MAX(intensity, 0), 256);
        glow->layer = -1;
        return (k);
    }
    return (-1);
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
    for (k = 0; k < JO_GLOW_LAYERS; ++k)
    {
        jo_glow_write_map(k, -1);
        __jo_glow_displayed_size[k] = -1;
    }
    if (__jo_glow_vblank_event < 0)
        __jo_glow_vblank_event = jo_core_add_vblank_callback(jo_glow_vblank);
    __jo_vdp2_glow_layers_show(__jo_glow_cells, (const void *)__jo_glow_maps[0], (const void *)__jo_glow_maps[1], __jo_glow_cram_block);
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

    glow->tested = true;
    glow->tested_from = *camera;
    if (__jo_glow_hit == JO_NULL)
    {
        glow->seen = true;
        return;
    }
    dx = jo_fixed2int(glow->position.x - camera->x);
    dy = jo_fixed2int(glow->position.y - camera->y);
    dz = jo_fixed2int(glow->position.z - camera->z);
    length = (int)jo_sqrt((unsigned int)(dx * dx + dy * dy + dz * dz));
    if (length <= JO_GLOW_LAMP_SIZE)
    {
        glow->seen = true;
        return;
    }
    dir.x = jo_int2fixed(dx) / length;
    dir.y = jo_int2fixed(dy) / length;
    dir.z = jo_int2fixed(dz) / length;
    glow->seen = !__jo_glow_hit(camera, &dir, jo_int2fixed(length - JO_GLOW_LAMP_SIZE));
}

void                                jo_3d_glows_draw(const jo_pos3D_fixed * const camera)
{
    jo_glow_state                   *state;
    jo_glow_layer_state             *l;
    jo_glow                         *glow;
    jo_glow                         *best[JO_GLOW_LAYERS];
    int                             best_score[JO_GLOW_LAYERS];
    jo_pos3D_fixed                  v;
    jo_fixed                        m[4][3];
    jo_fixed                        columns[3][3];
    jo_fixed                        k;
    int                             sx[JO_3D_GLOW_MAX];
    int                             sy[JO_3D_GLOW_MAX];
    int                             diameter[JO_3D_GLOW_MAX];
    int                             score[JO_3D_GLOW_MAX];
    bool                            on_screen[JO_3D_GLOW_MAX];
    jo_3d_rect                      rect[JO_3D_GLOW_MAX];
    unsigned int                    tag;
    int                             i;
    int                             j;
    int                             s;
    int                             rays;
    int                             light;
    int                             size;

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
        if (v.z > JO_FIXED_16)
        {
            k = jo_fixed_div(jo_3d_get_projection_distance(), v.z);
            sx[i] = jo_fixed2int(jo_fixed_mult(v.x, k));
            sy[i] = jo_fixed2int(jo_fixed_mult(v.y, k));
            diameter[i] = jo_fixed2int(jo_fixed_mult(jo_int2fixed(glow->radius * 2), k));
            s = JO_MIN(diameter[i], 128) / 2;
            on_screen[i] = sx[i] + s > -__jo_screen.width_2 && sx[i] - s < __jo_screen.width_2 && sy[i] + s > -__jo_screen.height_2 && sy[i] - s < __jo_screen.height_2;
            // Seen only where the place of the lamp is seen (the rectangle of a door: the walls around it hide its glow)
            rect[i].left = 0;
            rect[i].top = 0;
            rect[i].right = __jo_screen.width - 1;
            rect[i].bottom = __jo_screen.height - 1;
            if (on_screen[i] && __jo_glow_clip != JO_NULL)
            {
                on_screen[i] = __jo_glow_clip(&glow->position, &rect[i]);
                // (the disc and the rectangle)
                on_screen[i] = on_screen[i] && sx[i] + __jo_screen.width_2 + s > rect[i].left && sx[i] + __jo_screen.width_2 - s < rect[i].right &&
                               sy[i] + __jo_screen.height_2 + s > rect[i].top && sy[i] + __jo_screen.height_2 - s < rect[i].bottom;
            }
        }
    }
    // The rays (the glows on the screen not tested first, then in turn)
    for (rays = 0, j = 0; j < JO_3D_GLOW_MAX && rays < JO_GLOW_RAYS_PER_FRAME; ++j)
        if (on_screen[j] && !__jo_glows[j].tested)
        {
            jo_glow_test(&__jo_glows[j], camera);
            ++rays;
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
    // Fade, score (the light seen: intensity x area, up to the biggest disc)
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
        s = JO_MAX(JO_MIN(diameter[i], 128), 4);
        score[i] = (glow->intensity * glow->fade >> 8) * s;
    }
    // The best glows (a glow keeps its layer)
    for (j = 0; j < JO_GLOW_LAYERS; ++j)
    {
        best[j] = JO_NULL;
        best_score[j] = 0;
    }
    for (i = 0; i < JO_3D_GLOW_MAX; ++i)
    {
        if (score[i] <= 0)
            continue;
        for (j = 0; j < JO_GLOW_LAYERS; ++j)
        {
            if (score[i] > best_score[j])
            {
                for (s = JO_GLOW_LAYERS - 1; s > j; --s)
                {
                    best[s] = best[s - 1];
                    best_score[s] = best_score[s - 1];
                }
                best[j] = &__jo_glows[i];
                best_score[j] = score[i];
                break;
            }
        }
    }
    if (best[0] != JO_NULL && best[1] != JO_NULL && (best[0]->layer == 1 || best[1]->layer == 0))
    {
        glow = best[0];
        best[0] = best[1];
        best[1] = glow;
    }
    for (i = 0; i < JO_3D_GLOW_MAX; ++i)
        __jo_glows[i].layer = -1;
    // The state of the layers for this frame
    state = &__jo_glow_states[tag % JO_GLOW_STATES];
    for (j = 0, l = state->layers; j < JO_GLOW_LAYERS; ++j, ++l)
    {
        glow = best[j];
        l->size = -1;
        if (glow == JO_NULL)
            continue;
        glow->layer = (char)j;
        i = (int)(glow - __jo_glows);
        // The nearest size; smaller than the smallest disc: less light
        size = 0;
        for (s = 1; s < JO_GLOW_SIZES; ++s)
            if (JO_ABS(__jo_glow_sizes[s] - diameter[i]) < JO_ABS(__jo_glow_sizes[size] - diameter[i]))
                size = s;
        light = glow->intensity * glow->fade >> 8;
        if (diameter[i] < __jo_glow_sizes[0])
            light = light * diameter[i] * diameter[i] / (__jo_glow_sizes[0] * __jo_glow_sizes[0]);
        if (light <= 0)
            continue;
        l->size = (char)size;
        l->rect = rect[i];
        // The middle of the disc on the lamp
        l->scroll_x = (short)(__jo_glow_sizes[size] / 2 - (__jo_screen.width_2 + sx[i]));
        l->scroll_y = (short)(__jo_glow_sizes[size] / 2 - (__jo_screen.height_2 + sy[i]));
        // (the light of a level: (level / 15)^2: a soft halo, the light falls quickly away from the lamp. Linear, the halo
        // was a wide bright disc with a visible edge)
        for (s = 0; s < JO_GLOW_LEVELS; ++s)
            l->colors[s] = JO_COLOR_RGB(glow->r * light * (s + 1) * (s + 1) / (256 * JO_GLOW_LEVELS * JO_GLOW_LEVELS),
                                        glow->g * light * (s + 1) * (s + 1) / (256 * JO_GLOW_LEVELS * JO_GLOW_LEVELS),
                                        glow->b * light * (s + 1) * (s + 1) / (256 * JO_GLOW_LEVELS * JO_GLOW_LEVELS));
    }
    state->tag = tag;
}

#endif /* !JO_COMPILE_WITH_3D_SUPPORT */

/*
** END OF FILE
*/
