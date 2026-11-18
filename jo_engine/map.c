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
#include "jo/colors.h"
#include "jo/math.h"
#include "jo/vdp1_command_pipeline.h"
#include "jo/sprites.h"
#include "jo/tools.h"
#include "jo/malloc.h"
#include "jo/vdp2.h"
#include "jo/fs.h"
#include "jo/hitbox.h"
#include "jo/map.h"
#include "jo/sprite_animator.h"
#include "jo/list.h"
#include "jo/3d.h"
#include "internal/core.h"
#include "internal/tools.h"
#include "internal/vdp2.h"

#define JO_MAP_PARSER_BUF_SIZE	(8)

extern jo_texture_definition    __jo_sprite_def[JO_MAX_SPRITE];
extern jo_picture_definition    __jo_sprite_pic[JO_MAX_SPRITE];

/** @brief Map tile structure (16 bytes: the depth is the one of the layer, see gl_map_depth_fixed; a tile was 28 bytes with
 *  a copy of it and a position only used by jo_map_draw())
 */
typedef struct
{
    short			x;
    short			y;
    short			real_x;
    short			real_y;
    short			width;
    short			height;

#if JO_MAX_SPRITE > 255
    unsigned short
#else
    unsigned char
#endif
    sprite_or_anim_id;
    unsigned char   attribute;
    bool            is_animated;
}					jo_map_tile;

static jo_map_tile                      *gl_map[JO_MAP_MAX_LAYER];
static unsigned short                   gl_map_tile_count[JO_MAP_MAX_LAYER];
static unsigned short                   gl_map_max_tile_count[JO_MAP_MAX_LAYER];
static int                              gl_map_depth_fixed[JO_MAP_MAX_LAYER];

/*
** Index of the tiles by their horizontal position (a layer of JO_MAP_INDEX_MIN_TILES tiles or more): the tiles whose
** horizontal range meets the one of the screen (jo_map_draw()), of a hitbox (the collisions) or of a region of the
** background are found by a binary search, not by a loop over all the tiles of the layer (29 cycles a tile: a map of 4000
** tiles took 4 ms per frame in jo_map_draw(), the same in each collision test, even when no tile was seen).
** The results are the same: the tiles are drawn, and the first one found, in their order in the layer.
** Built at the first use after tiles are added, sorted again after tiles are moved (jo_map_move_tiles_by_attribute());
** without memory for it, the loops over all the tiles as before.
*/

/** @brief Fewer tiles in a layer: the loops over all of them (no index) */
#ifndef JO_MAP_INDEX_MIN_TILES
# define JO_MAP_INDEX_MIN_TILES                 (64)
#endif

/** @brief The tiles of a layer sorted by real_x (then by their number), JO_NULL: no index */
static unsigned short                   *gl_map_index[JO_MAP_MAX_LAYER];
/** @brief A bit per tile: the tiles found (drawn in their order), cleared after */
static unsigned int                     *gl_map_marks[JO_MAP_MAX_LAYER];
/** @brief Tiles in the index (gl_map_tile_count[] when it was built: tiles added after it, built again) */
static unsigned short                   gl_map_index_count[JO_MAP_MAX_LAYER];
/** @brief Width of the widest tile of the layer (a tile meets a range if it starts less than this before it) */
static short                            gl_map_max_width[JO_MAP_MAX_LAYER];
/** @brief false: tiles moved horizontally (the index is sorted again) */
static bool                             gl_map_index_sorted[JO_MAP_MAX_LAYER];
/** @brief true: no memory for the index of the layer (not asked again at each use, until the layer is created again) */
static bool                             gl_map_index_refused[JO_MAP_MAX_LAYER];

/** @brief Free the index of a layer */
static void                             jo_map_forget_index(const unsigned int layer)
{
    if (gl_map_index[layer] != JO_NULL)
        jo_free(gl_map_index[layer]);
    if (gl_map_marks[layer] != JO_NULL)
        jo_free(gl_map_marks[layer]);
    gl_map_index[layer] = JO_NULL;
    gl_map_marks[layer] = JO_NULL;
    JO_ZERO(gl_map_index_count[layer]);
    gl_map_index_refused[layer] = false;
}

/** @brief Sort the index of a layer by real_x, then by the number of the tile (insertion: a few tiles moved by a few pixels
 *  cost a loop over the index)
 *  @param layer Layer
 *  @param count Tiles in the index
 */
static void                             jo_map_sort_index(const unsigned int layer, const unsigned int count)
{
    unsigned short                      *index;
    const jo_map_tile                   *tiles;
    unsigned int                        k;
    unsigned int                        j;
    unsigned short                      t;
    int                                 x;

    index = gl_map_index[layer];
    tiles = gl_map[layer];
    for (k = 1; k < count; ++k)
    {
        t = index[k];
        x = tiles[t].real_x;
        for (j = k; j > 0 && (tiles[index[j - 1]].real_x > x || (tiles[index[j - 1]].real_x == x && index[j - 1] > t)); --j)
            index[j] = index[j - 1];
        index[j] = t;
    }
}

/** @brief Build the index of a layer: the tiles sorted by real_x by 2 passes of a radix sort of 8 bits (stable, from the
 *  order of the tiles: the same order as jo_map_sort_index())
 *  @param layer Layer
 *  @param count Tiles of the layer
 */
static void                             jo_map_build_index(const unsigned int layer, const unsigned int count)
{
    static unsigned short               counts[256];
    unsigned short                      *index;
    unsigned short                      *tmp;
    const jo_map_tile                   *tiles;
    unsigned int                        key;
    unsigned int                        sum;
    unsigned int                        k;
    unsigned int                        n;
    int                                 width;

    index = gl_map_index[layer];
    tiles = gl_map[layer];
    width = 0;
    for (k = 0; k < count; ++k)
        width = JO_MAX(width, tiles[k].width);
    gl_map_max_width[layer] = (short)width;
    tmp = (unsigned short *)jo_malloc_with_behaviour(count * sizeof(*tmp), JO_MALLOC_TRY_REUSE_BLOCK);
    if (tmp == JO_NULL)
    {
        for (k = 0; k < count; ++k)
            index[k] = (unsigned short)k;
        jo_map_sort_index(layer, count);
        return;
    }
    // Low byte of real_x + 32768 (an unsigned key in the order of real_x)
    for (k = 0; k < 256; ++k)
        JO_ZERO(counts[k]);
    for (k = 0; k < count; ++k)
        ++counts[((unsigned int)(tiles[k].real_x + 32768)) & 0xFF];
    for (k = 0, sum = 0; k < 256; ++k)
    {
        n = counts[k];
        counts[k] = (unsigned short)sum;
        sum += n;
    }
    for (k = 0; k < count; ++k)
    {
        key = ((unsigned int)(tiles[k].real_x + 32768)) & 0xFF;
        tmp[counts[key]++] = (unsigned short)k;
    }
    for (k = 0; k < 256; ++k)
        JO_ZERO(counts[k]);
    for (k = 0; k < count; ++k)
        ++counts[((unsigned int)(tiles[k].real_x + 32768)) >> 8];
    for (k = 0, sum = 0; k < 256; ++k)
    {
        n = counts[k];
        counts[k] = (unsigned short)sum;
        sum += n;
    }
    for (k = 0; k < count; ++k)
    {
        key = ((unsigned int)(tiles[tmp[k]].real_x + 32768)) >> 8;
        index[counts[key]++] = tmp[k];
    }
    jo_free(tmp);
}

/** @brief The index of a layer, built or sorted again if the tiles changed
 *  @param layer Layer
 *  @return false: no index (few tiles, or no memory for it): loop over all the tiles
 */
static bool                             jo_map_get_index(const unsigned int layer)
{
    const unsigned int                  count = gl_map_tile_count[layer];

    if (count < JO_MAP_INDEX_MIN_TILES)
        return (false);
    // (the index is allocated, sorted and marked by the master CPU only, outside of an interrupt)
    if (__jo_core_on_slave_cpu() || (__jo_core_get_sr() & JO_SH2_SR_INTERRUPT_MASK) != 0)
        return (false);
    if (gl_map_index[layer] == JO_NULL)
    {
        if (gl_map_index_refused[layer])
            return (false);
        gl_map_index[layer] = (unsigned short *)jo_malloc(gl_map_max_tile_count[layer] * sizeof(unsigned short));
        gl_map_marks[layer] = (unsigned int *)jo_malloc(JO_DIV_BY_32(gl_map_max_tile_count[layer] + 31) * sizeof(unsigned int));
        if (gl_map_index[layer] == JO_NULL || gl_map_marks[layer] == JO_NULL)
        {
            jo_map_forget_index(layer);
            gl_map_index_refused[layer] = true;
            return (false);
        }
        jo_memset(gl_map_marks[layer], 0, JO_DIV_BY_32(gl_map_max_tile_count[layer] + 31) * sizeof(unsigned int));
        JO_ZERO(gl_map_index_count[layer]);
    }
    if (gl_map_index_count[layer] != count)
    {
        jo_map_build_index(layer, count);
        gl_map_index_count[layer] = (unsigned short)count;
        gl_map_index_sorted[layer] = true;
    }
    else if (!gl_map_index_sorted[layer])
    {
        jo_map_sort_index(layer, count);
        gl_map_index_sorted[layer] = true;
    }
    return (true);
}

/** @brief The first tile of the index that can meet a range starting at x: real_x + the widest tile > x
 *  @param layer Layer (its index ready)
 *  @param x Start of the range
 *  @return Position in the index
 */
static unsigned int                     jo_map_index_start(const unsigned int layer, const int x)
{
    const unsigned short                *index;
    const jo_map_tile                   *tiles;
    unsigned int                        lo;
    unsigned int                        hi;
    unsigned int                        mid;
    int                                 from;

    index = gl_map_index[layer];
    tiles = gl_map[layer];
    from = x - gl_map_max_width[layer];
    for (lo = 0, hi = gl_map_index_count[layer]; lo < hi;)
    {
        mid = (lo + hi) >> 1;
        if (tiles[index[mid]].real_x <= from)
            lo = mid + 1;
        else
            hi = mid;
    }
    return (lo);
}

/** @brief The number of the lowest bit of a word: [(word & -word) x 0x077CB531 >> 27] (a de Bruijn sequence) */
static const unsigned char              jo_map_lowest_bit[32] =
{
    0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
};

/** @brief Mark the tiles of a layer that meet a rectangle (see gl_map_marks: the index is ready)
 *  @param layer Layer
 *  @param x Rectangle (the same test as jo_square_intersect())
 *  @param y Rectangle
 *  @param w Rectangle
 *  @param h Rectangle
 *  @param first The smallest number of a tile marked
 *  @param last The biggest number of a tile marked (-1: none)
 */
static void                             jo_map_mark_tiles(const unsigned int layer, const int x, const int y, const int w, const int h,
                                                          int * const first, int * const last)
{
    const unsigned short                *index;
    const jo_map_tile                   *tiles;
    const jo_map_tile                   *tile;
    unsigned int                        *marks;
    unsigned int                        count;
    unsigned int                        lo;
    unsigned int                        t;
    int                                 low;
    int                                 high;

    index = gl_map_index[layer];
    tiles = gl_map[layer];
    marks = gl_map_marks[layer];
    count = gl_map_index_count[layer];
    low = 0x7FFFFFFF;
    high = -1;
    for (lo = jo_map_index_start(layer, x); lo < count; ++lo)
    {
        t = index[lo];
        tile = &tiles[t];
        if (tile->real_x >= x + w)
            break;
        if (jo_square_intersect(tile->real_x, tile->real_y, tile->width, tile->height, x, y, w, h))
        {
            marks[t >> 5] |= __jo_bits[t & 31];
            low = JO_MIN(low, (int)t);
            high = JO_MAX(high, (int)t);
        }
    }
    *first = low;
    *last = high;
}

/** @brief The next tile marked by jo_map_mark_tiles() (its mark cleared), in the order of the layer
 *  @param layer Layer
 *  @param from The number after the previous tile (the first one marked at the first call)
 *  @param last The last tile marked
 *  @return The number of the tile, -1: no more
 */
static int                              jo_map_next_marked(const unsigned int layer, const int from, const int last)
{
    unsigned int                        *marks;
    unsigned int                        word;
    unsigned int                        k;

    marks = gl_map_marks[layer];
    // (the tiles are taken in their order and each mark is cleared: no mark before from, the next tile is the lowest bit of
    // the first word that is not empty)
    for (k = (unsigned int)from >> 5; (int)(k << 5) <= last; ++k)
    {
        word = marks[k];
        if (word == 0)
            continue;
        marks[k] = word & (word - 1);
        return ((int)(k << 5) + jo_map_lowest_bit[((word & -word) * 0x077CB531u) >> 27]);
    }
    return (-1);
}

/** @brief The first tile of a layer (in its order) that meets a rectangle
 *  @param layer Layer
 *  @param x Rectangle (see jo_square_intersect())
 *  @param y Rectangle
 *  @param w Rectangle
 *  @param h Rectangle
 *  @return The number of the tile, -1: none
 */
static int                              jo_map_first_tile(const unsigned int layer, const int x, const int y, const int w, const int h)
{
    const unsigned short                *index;
    const jo_map_tile                   *tile;
    unsigned int                        count;
    unsigned int                        lo;
    int                                 first;
    int                                 i;

    // (an index up to date is only read: the collisions can be tested by the slave CPU or in an interrupt)
    jo_map_get_index(layer);
    count = gl_map_index_count[layer];
    if (gl_map_index[layer] != JO_NULL && count == gl_map_tile_count[layer] && gl_map_index_sorted[layer])
    {
        index = gl_map_index[layer];
        first = -1;
        for (lo = jo_map_index_start(layer, x); lo < count; ++lo)
        {
            i = index[lo];
            tile = &gl_map[layer][i];
            if (tile->real_x >= x + w)
                break;
            if ((first < 0 || i < first) && jo_square_intersect(tile->real_x, tile->real_y, tile->width, tile->height, x, y, w, h))
                first = i;
        }
        return (first);
    }
    for (JO_ZERO(i); i < gl_map_tile_count[layer]; ++i)
    {
        tile = &gl_map[layer][i];
        if (jo_square_intersect(tile->real_x, tile->real_y, tile->width, tile->height, x, y, w, h))
            return (i);
    }
    return (-1);
}

/** @brief A pixel of a tile is free (transparent): the color 0 of a texture of 16 or 256 colors, JO_COLOR_Transparent of
 *  an RGB texture (a texture of 16 colors was read as RGB pixels: wrong pixels, past its end)
 *  @param pic The picture of the tile
 *  @param x Pixel in the tile
 *  @param y Pixel in the tile
 *  @param width Width of the tile
 */
static bool                             jo_map_pixel_is_free(const jo_picture_definition * const pic, const int x, const int y,
                                                             const int width)
{
    unsigned int                        k;
    unsigned char                       pair;

    k = (unsigned int)(x + y * width);
    if (pic->color_mode == JO_VDP1_TEXTURE_SIZE_SHIFT_256_COLORS)
        return (((const unsigned char *)pic->data)[k] == 0);
    if (pic->color_mode == JO_VDP1_TEXTURE_SIZE_SHIFT_16_COLORS)
    {
        // (2 pixels per byte, the first one in the high bits)
        pair = ((const unsigned char *)pic->data)[k >> 1];
        return (((k & 1) ? (pair & 15) : (pair >> 4)) == 0);
    }
    return (((const jo_color *)pic->data)[k] == JO_COLOR_Transparent);
}

int                                     jo_map_per_pixel_vertical_collision(const unsigned int layer, int x, int y, unsigned char * restrict attribute)
{
    register jo_map_tile                *current_tile;
    register int                        i;
    register int                        distance;
    jo_picture_definition               *current_pic;

#ifdef JO_DEBUG
    if (__JO_INDEX_IS_WRONG(layer, JO_MAP_MAX_LAYER, "layer"))
        return (JO_MAP_NO_COLLISION);
#endif
    for (i = jo_map_first_tile(layer, x, y, 1, 1); i >= 0 && i < gl_map_tile_count[layer]; ++i)
    {
        current_tile = &gl_map[layer][i];
        if (jo_square_intersect(current_tile->real_x, current_tile->real_y, current_tile->width, current_tile->height, x, y, 1, 1))
        {
            if (attribute != JO_NULL)
                *attribute = current_tile->attribute;
            current_pic = &__jo_sprite_pic[current_tile->is_animated ? jo_get_anim_sprite(current_tile->sprite_or_anim_id) : current_tile->sprite_or_anim_id];
            x -= current_tile->real_x;
            y -= current_tile->real_y;
            if (jo_map_pixel_is_free(current_pic, x, y, current_tile->width))
            {
                for (distance = 1; y + distance < current_tile->height; ++distance)
                {
                    if (!jo_map_pixel_is_free(current_pic, x, y + distance, current_tile->width))
                        return distance;
                }
                return (JO_MAP_NO_COLLISION);
            }
            // Inside: up to the first solid pixel of the column (the pixel above is free; the top row of the tile when
            // the column is solid up to it: the result was one pixel short there, and a solid top row read the row
            // before the tile)
            distance = y;
            while (y > 0 && !jo_map_pixel_is_free(current_pic, x, y - 1, current_tile->width))
                --y;
            return (y - distance);
        }
    }
    return (JO_MAP_NO_COLLISION);
}

int                                     jo_map_hitbox_detection_custom_boundaries(const unsigned int layer, const int x, const int y, const int w, const int h)
{
    int                                 i;

#ifdef JO_DEBUG
    if (__JO_INDEX_IS_WRONG(layer, JO_MAP_MAX_LAYER, "layer"))
        return (JO_MAP_NO_COLLISION);
#endif
    i = jo_map_first_tile(layer, x, y, w, h);
    return (i >= 0 ? gl_map[layer][i].attribute : JO_MAP_NO_COLLISION);
}

bool					                    jo_map_create(const unsigned int layer, const unsigned short max_tile_count, const short depth)
{
#ifdef JO_DEBUG
    if (layer >= JO_MAP_MAX_LAYER)
    {
        jo_core_error("layer (%d) is too high (max=%d)", layer, JO_MAP_MAX_LAYER - 1);
        return (false);
    }
#endif
    // The layer can be created again: the previous tiles are freed
    jo_map_free(layer);
    gl_map_depth_fixed[layer] = depth;
    gl_map[layer] = (jo_map_tile *)jo_malloc(max_tile_count * sizeof(jo_map_tile));
    if (gl_map[layer] == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory");
#endif
        return (false);
    }
    gl_map_max_tile_count[layer] = max_tile_count;
    return (true);
}

static void             __internal_jo_map_add_tile(const unsigned int layer, const short x, const short y,
#if JO_MAX_SPRITE > 255
        const unsigned short sprite_or_anim_id,
#else
        const unsigned char sprite_or_anim_id,
#endif
        const bool is_animated,
        const unsigned char attribute)
{
    int					                sprite_id;
    register jo_map_tile                *new_tile;

    if (gl_map_tile_count[layer] >= gl_map_max_tile_count[layer])
    {
#ifdef JO_DEBUG
        jo_core_error("Too many tiles in layer %d (max=%d)", layer, gl_map_max_tile_count[layer]);
#endif
        return ;
    }
    // (an id of a damaged map file ("@N") must not read after the tables of the animations or of the sprites)
    if (sprite_or_anim_id >= (is_animated ? JO_MAX_SPRITE_ANIM : JO_MAX_SPRITE))
    {
#ifdef JO_DEBUG
        jo_core_error("Tile: wrong %s id %d", is_animated ? "animation" : "sprite", sprite_or_anim_id);
#endif
        return ;
    }
    new_tile = &gl_map[layer][gl_map_tile_count[layer]];
    new_tile->sprite_or_anim_id = sprite_or_anim_id;
    new_tile->real_x = x;
    new_tile->real_y = y;
    if (is_animated)
        sprite_id = jo_get_anim_sprite(sprite_or_anim_id);
    else
        sprite_id = sprite_or_anim_id;
    new_tile->x = x - __jo_screen.width_2 + JO_DIV_BY_2(__jo_sprite_def[sprite_id].width);
    new_tile->y = y - __jo_screen.height_2 + JO_DIV_BY_2(__jo_sprite_def[sprite_id].height);
    new_tile->width = __jo_sprite_def[sprite_id].width;
    new_tile->height = __jo_sprite_def[sprite_id].height;
    new_tile->is_animated = is_animated;
    new_tile->attribute = attribute;
    ++gl_map_tile_count[layer];
}

void                        jo_map_add_tile(const unsigned int layer, const short x, const short y,
#if JO_MAX_SPRITE > 255
        const unsigned short sprite_id,
#else
        const unsigned char sprite_id,
#endif
        const unsigned char attribute)
{
#ifdef JO_DEBUG
    if (__JO_INDEX_IS_WRONG(layer, JO_MAP_MAX_LAYER, "layer") || __JO_SPRITE_IS_MISSING(sprite_id))
        return;
#endif
    __internal_jo_map_add_tile(layer, x, y, sprite_id, false, attribute);
}

void                        jo_map_add_animated_tile(const unsigned int layer, const short x, const short y, const unsigned char anim_id, const unsigned char attribute)
{
#ifdef JO_DEBUG
    if (__JO_INDEX_IS_WRONG(layer, JO_MAP_MAX_LAYER, "layer") || __JO_SPRITE_ANIM_IS_WRONG(anim_id))
        return;
#endif
    __internal_jo_map_add_tile(layer, x, y, anim_id, true, attribute);
}

void						jo_map_draw_background(const unsigned int layer, const short x, const short y)
{
    register jo_map_tile    *current_tile;
    register int            i;
    jo_img                  img;

#ifdef JO_DEBUG
    if (__JO_INDEX_IS_WRONG(layer, JO_MAP_MAX_LAYER, "layer"))
        return;
#endif
    for (JO_ZERO(i); i < gl_map_tile_count[layer]; ++i)
    {
        current_tile = &gl_map[layer][i];
        img.width = current_tile->width;
        img.height =  current_tile->height;
        // (an animated tile: its animation id, the sprite of its current frame, as jo_map_draw_background_region())
        img.data = __jo_sprite_pic[current_tile->is_animated ? jo_get_anim_sprite(current_tile->sprite_or_anim_id) : current_tile->sprite_or_anim_id].data;
        jo_vdp2_set_nbg1_image(&img,
                                 x + current_tile->real_x,
                                 y + current_tile->real_y);
    }
}

/** @brief Draw a region of a layer in the NBG1 bitmap (the bitmap wraps around like the NBG1 scroll)
 *  @param layer layer level (between 0 and JO_MAP_MAX_LAYER)
 *  @param x Horizontal position of the region in the map
 *  @param y Vertical position of the region in the map
 *  @param width Region width
 *  @param height Region height
 *  @param clear_color Color of the pixels without tile
 */
static void                 jo_map_draw_background_region(const unsigned int layer, const int x, const int y, const int width, const int height, const jo_color clear_color)
{
    register jo_map_tile    *current_tile;
    register int            i;
    jo_img                  img;
    int                     left;
    int                     top;
    int                     right;
    int                     bottom;
    int                     first;
    int                     last;
    bool                    indexed;

    if (width <= 0 || height <= 0)
        return ;
    __jo_vdp2_nbg1_bitmap_fill(x, y, width, height, clear_color);
    indexed = jo_map_get_index(layer);
    if (indexed)
    {
        jo_map_mark_tiles(layer, x, y, width, height, &first, &last);
        i = jo_map_next_marked(layer, first, last);
    }
    else
        JO_ZERO(i);
    for (; indexed ? i >= 0 : i < gl_map_tile_count[layer]; i = indexed ? jo_map_next_marked(layer, i + 1, last) : i + 1)
    {
        current_tile = &gl_map[layer][i];
        // Intersection between the tile and the region
        left = JO_MAX(x, current_tile->real_x);
        top = JO_MAX(y, current_tile->real_y);
        right = JO_MIN(x + width, current_tile->real_x + current_tile->width);
        bottom = JO_MIN(y + height, current_tile->real_y + current_tile->height);
        if (left >= right || top >= bottom)
            continue;
        img.width = current_tile->width;
        img.height = current_tile->height;
        img.data = __jo_sprite_pic[current_tile->is_animated ? jo_get_anim_sprite(current_tile->sprite_or_anim_id) : current_tile->sprite_or_anim_id].data;
        __jo_vdp2_nbg1_bitmap_blit(&img, left - current_tile->real_x, top - current_tile->real_y, right - left, bottom - top, left, top);
    }
}

/** @brief What jo_map_scroll_background() drew (a map freed or created again: drawn again) */
static bool                 __jo_map_scroll_drawn = false;

void						jo_map_scroll_background(const unsigned int layer, const short x, const short y, const jo_color clear_color)
{
    static unsigned int     drawn_layer;
    static jo_color         drawn_clear_color;
    static int              view_x;
    static int              view_y;
    int                     new_x;
    int                     new_y;
    int                     dx;
    int                     dy;

#ifdef JO_DEBUG
    if (__JO_INDEX_IS_WRONG(layer, JO_MAP_MAX_LAYER, "layer"))
        return;
#endif
    // Top-left corner of the screen in the map
    new_x = -x;
    new_y = -y;
    dx = new_x - view_x;
    dy = new_y - view_y;
    if (!__jo_map_scroll_drawn || drawn_layer != layer || drawn_clear_color != clear_color || JO_ABS(dx) > JO_VDP2_WIDTH - __jo_screen.background_width || JO_ABS(dy) > JO_VDP2_HEIGHT - __jo_screen.background_height)
    {
        // First call or too big move: draw the whole screen
        jo_map_draw_background_region(layer, new_x, new_y, __jo_screen.background_width, __jo_screen.background_height, clear_color);
        __jo_map_scroll_drawn = true;
        drawn_layer = layer;
        drawn_clear_color = clear_color;
    }
    else
    {
        // Only the new columns and lines (outside of the screen until the scroll registers change at the next VBLANK)
        if (dx > 0)
            jo_map_draw_background_region(layer, view_x + __jo_screen.background_width, new_y, dx, __jo_screen.background_height, clear_color);
        else if (dx < 0)
            jo_map_draw_background_region(layer, new_x, new_y, -dx, __jo_screen.background_height, clear_color);
        if (dy > 0)
            jo_map_draw_background_region(layer, new_x, view_y + __jo_screen.background_height, __jo_screen.background_width, dy, clear_color);
        else if (dy < 0)
            jo_map_draw_background_region(layer, new_x, new_y, __jo_screen.background_width, -dy, clear_color);
    }
    view_x = new_x;
    view_y = new_y;
    jo_vdp2_move_nbg1(new_x & (JO_VDP2_WIDTH - 1), new_y & (JO_VDP2_HEIGHT - 1));
}

#ifdef JO_COMPILE_WITH_3D_SUPPORT
void						jo_map_3d_draw(const unsigned int layer, const int x, const int y, const int z)
{
    register int		    i;
    register jo_map_tile    *current_tile;

#ifdef JO_DEBUG
    if (__JO_INDEX_IS_WRONG(layer, JO_MAP_MAX_LAYER, "layer"))
        return;
#endif
    for (JO_ZERO(i); i < gl_map_tile_count[layer]; ++i)
    {
        current_tile = &gl_map[layer][i];
        if (current_tile->is_animated)
            jo_3d_draw_sprite_at(jo_get_anim_sprite(current_tile->sprite_or_anim_id), current_tile->x - x, current_tile->y - y, z);
        else
            jo_3d_draw_sprite_at(current_tile->sprite_or_anim_id, current_tile->x - x, current_tile->y - y, z);
    }
}
#endif /* !JO_COMPILE_WITH_3D_SUPPORT */

void						jo_map_draw(const unsigned int layer, const short screen_x, const short screen_y)
{
    register int		    i;
    register jo_map_tile    *current_tile;
    int                     first;
    int                     last;
    bool                    indexed;

#ifdef JO_DEBUG
    if (__JO_INDEX_IS_WRONG(layer, JO_MAP_MAX_LAYER, "layer"))
        return;
#endif
    indexed = jo_map_get_index(layer);
    if (indexed)
    {
        jo_map_mark_tiles(layer, screen_x, screen_y, __jo_screen.width, __jo_screen.height, &first, &last);
        i = jo_map_next_marked(layer, first, last);
    }
    else
        JO_ZERO(i);
    for (; indexed ? i >= 0 : i < gl_map_tile_count[layer]; i = indexed ? jo_map_next_marked(layer, i + 1, last) : i + 1)
    {
        current_tile = &gl_map[layer][i];
        if (jo_square_intersect(current_tile->real_x, current_tile->real_y, current_tile->width, current_tile->height, screen_x, screen_y, __jo_screen.width, __jo_screen.height))
        {
            jo_sprite_draw3D(current_tile->is_animated ? jo_get_anim_sprite(current_tile->sprite_or_anim_id) :
                             current_tile->sprite_or_anim_id, current_tile->x - screen_x, current_tile->y - screen_y,
                             gl_map_depth_fixed[layer]);
        }
    }
}

void					jo_map_free(const unsigned int layer)
{
#ifdef JO_DEBUG
    if (__JO_INDEX_IS_WRONG(layer, JO_MAP_MAX_LAYER, "layer"))
        return;
#endif
    jo_map_forget_index(layer);
    if (gl_map[layer] != JO_NULL)
        jo_free(gl_map[layer]);
    gl_map[layer] = JO_NULL;
    JO_ZERO(gl_map_tile_count[layer]);
    JO_ZERO(gl_map_max_tile_count[layer]);
    __jo_map_scroll_drawn = false;
}

#ifdef JO_COMPILE_WITH_FS_SUPPORT

bool                    jo_map_load_from_file(const unsigned int layer, const short depth, const char *const sub_dir, const char *const filename)
{
    char*				stream;
    char                *stream_begin;
    char*				tmp;
    int					tile_count;
    char				sprite[JO_MAX_FILENAME_LENGTH];
    char				x[JO_MAP_PARSER_BUF_SIZE];
    char				y[JO_MAP_PARSER_BUF_SIZE];
    char				attribute[JO_MAP_PARSER_BUF_SIZE];
    unsigned char       attribute_value;
    register int		i;
#ifdef JO_COMPILE_WITH_SPRITE_HASHTABLE
    int                 sprite_id;
#endif

    if ((stream = jo_fs_read_file_in_dir(filename, sub_dir, 0)) == JO_NULL)
        return (false);
    stream_begin = stream;
    // One tile per line (the last line may not end with a newline)
    JO_ZERO(tile_count);
    for (tmp = stream; *tmp; ++tmp)
        if (*tmp == '\n' || tmp[1] == '\0')
            ++tile_count;
    if (tile_count <= 0)
    {
        jo_free(stream_begin);
        return (false);
    }
    if (!jo_map_create(layer, tile_count, depth))
    {
        jo_free(stream_begin);
        return (false);
    }
    while (*stream)
    {
        JO_ZERO(attribute_value);
        while (*stream && jo_tools_is_whitespace(*stream))
            ++stream;
        if (!*stream)
        {
            jo_free(stream_begin);
            return (true);
        }
        for (JO_ZERO(i); *stream && !jo_tools_is_whitespace(*stream); ++i)
        {
            if (i >= JO_MAX_FILENAME_LENGTH - 1)
            {
#ifdef JO_DEBUG
                jo_core_error("Filename too long: %s", stream);
#endif
                jo_free(stream_begin);
                jo_map_free(layer);
                return (false);
            }
            sprite[i] = *stream++;
        }
        JO_ZERO(sprite[i]);
        while (*stream && jo_tools_is_whitespace(*stream))
            ++stream;
        for (JO_ZERO(i); *stream && !jo_tools_is_whitespace(*stream); ++i)
        {
            if (i >= JO_MAP_PARSER_BUF_SIZE - 1)
            {
#ifdef JO_DEBUG
                jo_core_error("X coord too big");
#endif
                jo_free(stream_begin);
                jo_map_free(layer);
                return (false);
            }
            x[i] = *stream++;
        }
        JO_ZERO(x[i]);
        while (*stream && jo_tools_is_whitespace(*stream))
            ++stream;
        for (JO_ZERO(i); *stream && !jo_tools_is_whitespace(*stream); ++i)
        {
            if (i >= JO_MAP_PARSER_BUF_SIZE - 1)
            {
#ifdef JO_DEBUG
                jo_core_error("Y coord too big");
#endif
                jo_free(stream_begin);
                jo_map_free(layer);
                return (false);
            }
            y[i] = *stream++;
        }
        JO_ZERO(y[i]);
        // (the spaces and the tabs after Y, not the end of the line: the next line was read as the attribute)
        while (*stream == ' ' || *stream == '\t')
            ++stream;
        if (*stream && *stream != '\r' && *stream != '\n')
        {
            for (JO_ZERO(i); *stream && !jo_tools_is_whitespace(*stream); ++i)
            {
                if (i >= JO_MAP_PARSER_BUF_SIZE - 1)
                {
#ifdef JO_DEBUG
                    jo_core_error("Attribute too big");
#endif
                    jo_free(stream_begin);
                    jo_map_free(layer);
                    return (false);
                }
                attribute[i] = *stream++;
            }
            JO_ZERO(attribute[i]);
            attribute_value = (unsigned char)jo_tools_atoi(attribute);
        }
        // (the id checked before it is cut to the type of the tiles)
        if (sprite[0] == '@')
            __internal_jo_map_add_tile(layer, jo_tools_atoi(x), jo_tools_atoi(y),
                                       (unsigned int)jo_tools_atoi(sprite + 1) < JO_MAX_SPRITE_ANIM ? jo_tools_atoi(sprite + 1) : JO_MAX_SPRITE_ANIM,
                                       true, attribute_value);
#ifdef JO_COMPILE_WITH_SPRITE_HASHTABLE
        else if ((sprite_id = jo_sprite_name2id(sprite)) >= 0)
            __internal_jo_map_add_tile(layer, jo_tools_atoi(x), jo_tools_atoi(y), sprite_id, false, attribute_value);
#ifdef JO_DEBUG
        else
            jo_core_error("%s: Sprite not found", sprite);
#endif
#elif defined(JO_DEBUG)
        // (the tile was dropped without a word: the map drew nothing)
        else
            jo_core_error("%s: a sprite name needs JO_COMPILE_WITH_SPRITE_HASHTABLE", sprite);
#endif
    }
    jo_free(stream_begin);
    return (true);
}

#endif /* !JO_COMPILE_WITH_FS_SUPPORT */

void                        jo_map_move_tiles_by_attribute(const unsigned int layer, const unsigned char attribute_filter, const short incr_x, const short incr_y)
{
    register int            i;
    register jo_map_tile    *current_tile;

#ifdef JO_DEBUG
    if (__JO_INDEX_IS_WRONG(layer, JO_MAP_MAX_LAYER, "layer"))
        return;
#endif
    for (JO_ZERO(i); i < gl_map_tile_count[layer]; ++i)
    {
        current_tile = &gl_map[layer][i];
        if (current_tile->attribute != attribute_filter)
            continue;
        if (incr_y != 0)
        {
            current_tile->y += incr_y;
            current_tile->real_y += incr_y;
        }
        if (incr_x != 0)
        {
            current_tile->x += incr_x;
            current_tile->real_x += incr_x;
            gl_map_index_sorted[layer] = false;
        }
    }
}

/*
** END OF FILE
*/
