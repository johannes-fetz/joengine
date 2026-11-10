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
#include "internal/vdp2.h"

#define JO_MAP_PARSER_BUF_SIZE	(8)

extern jo_texture_definition    __jo_sprite_def[JO_MAX_SPRITE];
extern jo_picture_definition    __jo_sprite_pic[JO_MAX_SPRITE];

/** @brief Map tile structure */
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
    bool            is_visible_on_screen;
    jo_pos3D        pos;
}					jo_map_tile;

static jo_map_tile                      *gl_map[JO_MAP_MAX_LAYER];
static unsigned short                   gl_map_tile_count[JO_MAP_MAX_LAYER];
static unsigned short                   gl_map_max_tile_count[JO_MAP_MAX_LAYER];
static int                              gl_map_depth_fixed[JO_MAP_MAX_LAYER];

int                                     jo_map_per_pixel_vertical_collision(const unsigned int layer, int x, int y, unsigned char * restrict attribute)
{
    register jo_map_tile                *current_tile;
    register int                        i;
    register int                        distance;
    unsigned short                      *img_data;
    unsigned char                       *img_data_8bits;
    jo_picture_definition               *current_pic;

    for (JO_ZERO(i); i < gl_map_tile_count[layer]; ++i)
    {
        current_tile = &gl_map[layer][i];
        if (jo_square_intersect(current_tile->real_x, current_tile->real_y, current_tile->width, current_tile->height, x, y, 1, 1))
        {
            if (attribute != JO_NULL)
                *attribute = current_tile->attribute;
            current_pic = &__jo_sprite_pic[current_tile->is_animated ? jo_get_anim_sprite(current_tile->sprite_or_anim_id) : current_tile->sprite_or_anim_id];
            x -= current_tile->real_x;
            y -= current_tile->real_y;
            if (current_pic->color_mode == JO_VDP1_TEXTURE_SIZE_SHIFT_256_COLORS)
            {
                img_data_8bits = ((unsigned char *)current_pic->data);
                if (jo_sprite_get_pixel_palette_index(img_data_8bits, x, y, current_tile->width) == 0)
                {
                    for (distance = 1; y + distance < current_tile->height; ++distance)
                    {
                        if (jo_sprite_get_pixel_palette_index(img_data_8bits, x, y + distance, current_tile->width) != 0)
                            return distance;
                    }
                    return (JO_MAP_NO_COLLISION);
                }
                // Inside: up to the first solid pixel of the column (the pixel above is free; the top row of the tile when
                // the column is solid up to it: the result was one pixel short there, and a solid top row read the row
                // before the tile)
                distance = y;
                while (y > 0 && jo_sprite_get_pixel_palette_index(img_data_8bits, x, y - 1, current_tile->width) != 0)
                    --y;
                return (y - distance);
            }
            else
            {
                img_data = ((unsigned short *)current_pic->data);
                if (jo_sprite_is_pixel_transparent(img_data, x, y, current_tile->width))
                {
                    for (distance = 1; y + distance < current_tile->height; ++distance)
                    {
                        if (!jo_sprite_is_pixel_transparent(img_data, x, y + distance, current_tile->width))
                            return distance;
                    }
                    return (JO_MAP_NO_COLLISION);
                }
                // (see above)
                distance = y;
                while (y > 0 && !jo_sprite_is_pixel_transparent(img_data, x, y - 1, current_tile->width))
                    --y;
                return (y - distance);
            }
        }
    }
    return (JO_MAP_NO_COLLISION);
}

int                                     jo_map_hitbox_detection_custom_boundaries(const unsigned int layer, const int x, const int y, const int w, const int h)
{
    register jo_map_tile                *current_tile;
    register int                        i;

    for (JO_ZERO(i); i < gl_map_tile_count[layer]; ++i)
    {
        current_tile = &gl_map[layer][i];
        if (jo_square_intersect(current_tile->real_x, current_tile->real_y, current_tile->width, current_tile->height, x, y, w, h))
            return (current_tile->attribute);
    }
    return (JO_MAP_NO_COLLISION);
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
    new_tile->pos.z = gl_map_depth_fixed[layer];
    new_tile->is_visible_on_screen = false;
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
    __internal_jo_map_add_tile(layer, x, y, sprite_id, false, attribute);
}

void                        jo_map_add_animated_tile(const unsigned int layer, const short x, const short y, const unsigned char anim_id, const unsigned char attribute)
{
    __internal_jo_map_add_tile(layer, x, y, anim_id, true, attribute);
}

void						jo_map_draw_background(const unsigned int layer, const short x, const short y)
{
    register jo_map_tile    *current_tile;
    register int            i;
    jo_img                  img;

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

    if (width <= 0 || height <= 0)
        return ;
    __jo_vdp2_nbg1_bitmap_fill(x, y, width, height, clear_color);
    for (JO_ZERO(i); i < gl_map_tile_count[layer]; ++i)
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

    for (JO_ZERO(i); i < gl_map_tile_count[layer]; ++i)
    {
        current_tile = &gl_map[layer][i];
        current_tile->is_visible_on_screen = jo_square_intersect(current_tile->real_x, current_tile->real_y, current_tile->width, current_tile->height, screen_x, screen_y, __jo_screen.width, __jo_screen.height);
        if (current_tile->is_visible_on_screen)
        {
            current_tile->pos.x = current_tile->x - screen_x;
            current_tile->pos.y = current_tile->y - screen_y;
            if (current_tile->is_animated)
                jo_sprite_draw(jo_get_anim_sprite(current_tile->sprite_or_anim_id), &current_tile->pos, true, false);
            else
                jo_sprite_draw(current_tile->sprite_or_anim_id, &current_tile->pos, true, false);
        }
    }
}

void					jo_map_free(const unsigned int layer)
{
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
        }
    }
}

/*
** END OF FILE
*/
