/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2024, Johannes Fetz (johannesfetz@gmail.com)
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
#include "jo/sgl_prototypes.h"
#include "jo/conf.h"
#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/core.h"
#include "jo/colors.h"
#include "jo/math.h"
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
            if (current_pic->color_mode == COL_256)
            {
                img_data_8bits = ((unsigned char *)current_pic->data);
                if (jo_sprite_get_pixel_palette_index(img_data_8bits, x, y, current_tile->width) == 0)
                {
                    for (distance = 1; distance < current_tile->height; ++distance)
                    {
                        if (jo_sprite_get_pixel_palette_index(img_data_8bits, x, y + distance, current_tile->width) != 0)
                            return distance;
                    }
                    return (JO_MAP_NO_COLLISION);
                }
                else
                {
                    distance = y;
                    do
                    {
                        --y;
                        if (jo_sprite_get_pixel_palette_index(img_data_8bits, x, y, current_tile->width) == 0)
                            break;

                    }
                    while (y > 0);
                    return (y - distance + 1);
                }
            }
            else
            {
                img_data = ((unsigned short *)current_pic->data);
                if (jo_sprite_is_pixel_transparent(img_data, x, y, current_tile->width))
                {
                    for (distance = 1; distance < current_tile->height; ++distance)
                    {
                        if (!jo_sprite_is_pixel_transparent(img_data, x, y + distance, current_tile->width))
                            return distance;
                    }
                    return (JO_MAP_NO_COLLISION);
                }
                else
                {
                    distance = y;
                    do
                    {
                        --y;
                        if (jo_sprite_is_pixel_transparent(img_data, x, y, current_tile->width))
                            break;

                    }
                    while (y > 0);
                    return (y - distance + 1);
                }
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
    gl_map_depth_fixed[layer] = depth;
    JO_ZERO(gl_map_tile_count[layer]);
    gl_map[layer] = (jo_map_tile *)jo_malloc(max_tile_count * sizeof(jo_map_tile));
    if (gl_map[layer] == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory");
#endif
        return (false);
    }
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
    new_tile->x = x - JO_TV_WIDTH_2 + JO_DIV_BY_2(__jo_sprite_def[sprite_id].width);
    new_tile->y = y - JO_TV_HEIGHT_2 + JO_DIV_BY_2(__jo_sprite_def[sprite_id].height);
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
        img.data = __jo_sprite_pic[current_tile->sprite_or_anim_id].data;
        jo_vdp2_set_nbg1_image(&img,
                                 x + current_tile->real_x,
                                 y + current_tile->real_y);
    }
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
        current_tile->is_visible_on_screen = jo_square_intersect(current_tile->real_x, current_tile->real_y, current_tile->width, current_tile->height, screen_x, screen_y, JO_TV_WIDTH, JO_TV_HEIGHT);
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
    jo_free(gl_map[layer]);
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

    if ((stream = jo_fs_read_file_in_dir(filename, sub_dir, 0)) == JO_NULL)
        return (false);
    stream_begin = stream;
    JO_ZERO(tile_count);
    for (tmp = stream; *tmp; ++tmp)
        if (*tmp == '\n')
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
#ifdef JO_DEBUG
            if (i >= JO_MAX_FILENAME_LENGTH)
            {
                jo_core_error("Filename too long: %s", stream);
                jo_free(stream_begin);
                return (0);
            }
#endif
            sprite[i] = *stream++;
        }
        JO_ZERO(sprite[i]);
        while (*stream && jo_tools_is_whitespace(*stream))
            ++stream;
        for (JO_ZERO(i); *stream && !jo_tools_is_whitespace(*stream); ++i)
        {
#ifdef JO_DEBUG
            if (i >= JO_MAP_PARSER_BUF_SIZE)
            {
                jo_core_error("X coord too big");
                jo_free(stream);
                return (0);
            }
#endif
            x[i] = *stream++;
        }
        JO_ZERO(x[i]);
        while (*stream && jo_tools_is_whitespace(*stream))
            ++stream;
        for (JO_ZERO(i); *stream && !jo_tools_is_whitespace(*stream); ++i)
        {
#ifdef JO_DEBUG
            if (i >= JO_MAP_PARSER_BUF_SIZE)
            {
                jo_core_error("Y coord too big");
                jo_free(stream_begin);
                return (0);
            }
#endif
            y[i] = *stream++;
        }
        JO_ZERO(y[i]);
        if (*stream && *stream != '\r' && *stream != '\n')
        {
            while (*stream && jo_tools_is_whitespace(*stream))
                ++stream;
            for (JO_ZERO(i); *stream && !jo_tools_is_whitespace(*stream); ++i)
            {
#ifdef JO_DEBUG
                if (i >= JO_MAP_PARSER_BUF_SIZE)
                {
                    jo_core_error("Attribute too big");
                    jo_free(stream_begin);
                    return (0);
                }
#endif
                attribute[i] = *stream++;
            }
            JO_ZERO(attribute[i]);
            attribute_value = (unsigned char)jo_tools_atoi(attribute);
        }
        if (sprite[0] == '@')
            __internal_jo_map_add_tile(layer, jo_tools_atoi(x), jo_tools_atoi(y), jo_tools_atoi(sprite + 1), true, attribute_value);
#ifdef JO_COMPILE_WITH_SPRITE_HASHTABLE
        else
            __internal_jo_map_add_tile(layer, jo_tools_atoi(x), jo_tools_atoi(y), jo_sprite_name2id(sprite), false, attribute_value);
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
