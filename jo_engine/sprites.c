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
#include "jo/vdp1_command_pipeline.h"
#include "jo/colors.h"
#include "jo/malloc.h"
#include "jo/fs.h"
#include "jo/image.h"
#include "jo/sprites.h"
#include "jo/math.h"
#include "jo/list.h"
#include "jo/3d.h"

/*
** MACROS
*/

/*
** GLOBALS
*/

jo_sprite_attributes    __jo_sprite_attributes = {0, 0, 0, 0, JO_NO_ZOOM, JO_NO_ZOOM, JO_VDP1_NO_WINDOW};
jo_pos3D                __jo_sprite_pos = {0, 0, 120};
jo_texture_definition   __jo_sprite_def[JO_MAX_SPRITE] __attribute__((used, externally_visible));
jo_picture_definition   __jo_sprite_pic[JO_MAX_SPRITE];
/* The sprite has VDP1 end codes (see jo_sprite_add_end_codes()) */
/* (read by the assembly polygon loop of 3d.c: kept with its name by the link time optimization) */
bool                    __jo_sprite_end_codes[JO_MAX_SPRITE] __attribute__((used, externally_visible));
/* Color lookup table of a 16 colors texture (VRAM address / 8, see jo_sprite_add_4bits_lut()), 0: none */
unsigned short          __jo_sprite_lut[JO_MAX_SPRITE];
/* Mean color of the pixels of a texture with smaller copies (set by jo_3d_set_texture_mipmaps() in 3d.c), 0: unknown. A
   polygon with the texture seen as a thin line is drawn with it (see jo_3d_emit_sliver()) */
unsigned short          __jo_sprite_mean_color[JO_MAX_SPRITE];
int                     __jo_gouraud_shading_runtime_index = -1;
#ifdef JO_COMPILE_WITH_SPRITE_HASHTABLE
int				        __jo_hash_table[JO_MAX_SPRITE];
#endif
static int				__jo_sprite_addr = 0;
static int				__jo_sprite_id = -1;

static  __jo_force_inline unsigned int	__jo_get_next_sprite_address(const unsigned int sprite_address, const unsigned int sprite_width,
        const unsigned int sprite_height, const unsigned int sprite_color_mode)
{
    return ((sprite_address) + (((((sprite_width) * (sprite_height) * 4) >> (sprite_color_mode)) + 0x1f) & 0x7ffe0));
}

int                         jo_sprite_usage_percent(void)
{
    jo_texture_definition   *texture;
    int                     mem_free;

    if (!__jo_sprite_addr)
        return (0);
    texture = &__jo_sprite_def[__jo_sprite_id];
    mem_free = JO_VDP1_USER_AREA_END_ADDR - (JO_VDP1_VRAM + __jo_get_next_sprite_address(JO_MULT_BY_8(texture->adr),
               texture->width,
               texture->height,
               __jo_sprite_pic[__jo_sprite_id].color_mode));
    return (JO_PERCENT_USED(JO_VDP1_USER_AREA_SIZE, mem_free));
}

unsigned int                jo_sprite_free_memory(void)
{
    jo_texture_definition   *texture;
    unsigned int            next;

    if (!__jo_sprite_addr)
        return (JO_VDP1_USER_AREA_END_ADDR - (JO_VDP1_VRAM + JO_VDP1_TEXTURE_DEF_BASE_ADDRESS));
    texture = &__jo_sprite_def[__jo_sprite_id];
    next = JO_VDP1_VRAM + __jo_get_next_sprite_address(JO_MULT_BY_8(texture->adr), texture->width, texture->height,
                                                       __jo_sprite_pic[__jo_sprite_id].color_mode);
    return (next < JO_VDP1_USER_AREA_END_ADDR ? JO_VDP1_USER_AREA_END_ADDR - next : 0);
}

int                     jo_get_last_sprite_id(void)
{
    return (__jo_sprite_id);
}

void                    jo_sprite_init(void)
{
#ifdef JO_COMPILE_WITH_SPRITE_HASHTABLE
    register int		i;
#endif

#ifdef JO_DEBUG
    if (!JO_IS_ARRAY_INSIDE_USER_RAM_AREA(__jo_sprite_def) ||
# ifdef JO_COMPILE_WITH_SPRITE_HASHTABLE
        !JO_IS_ARRAY_INSIDE_USER_RAM_AREA(__jo_hash_table) ||
# endif
        !JO_IS_ARRAY_INSIDE_USER_RAM_AREA(__jo_sprite_pic))
    {
        jo_core_error("Please reduce JO_GLOBAL_MEMORY_SIZE_FOR_MALLOC");
        jo_goto_boot_menu();
        return ;
    }
#endif
    __jo_sprite_id = -1;
#ifdef JO_COMPILE_WITH_SPRITE_HASHTABLE
    for (JO_ZERO(i); i < JO_MAX_SPRITE; ++i)
        JO_ZERO(__jo_hash_table[i]);
#endif
}

#ifdef JO_COMPILE_WITH_SPRITE_HASHTABLE
int				jo_sprite_name2id(const char * const restrict filename)
{
    register int		i;
    int					hash;

#ifdef JO_DEBUG
    if (filename == JO_NULL)
    {
        jo_core_error("filename is null");
        return (-1);
    }
#endif
    hash = jo_4_char_hash(filename);
    for (JO_ZERO(i); i <= __jo_sprite_id; ++i)
        if (__jo_hash_table[i] == hash)
            return (i);
    return (-1);
}
#endif

int				            __jo_sprite_replace(const jo_img * const img, const int sprite_id, const bool async)
{
    jo_texture_definition   *texture;
    jo_picture_definition   *picture;

    texture = &__jo_sprite_def[sprite_id];
    picture = &__jo_sprite_pic[sprite_id];
#ifdef JO_DEBUG
    if (img == JO_NULL)
    {
        jo_core_error("img is null");
        return (sprite_id);
    }
    if (sprite_id > __jo_sprite_id)
    {
        jo_core_error("Invalid sprite_id");
        return (sprite_id);
    }
    if (texture->width != img->width)
    {
        jo_core_error("Width doesn't match (%d != %d)", (int)texture->width, (int)img->width);
        return (sprite_id);
    }
    if (texture->height != img->height)
    {
        jo_core_error("Height doesn't match (%d != %d)", (int)texture->height, (int)img->height);
        return (sprite_id);
    }
#endif
    picture->data = img->data;
    if (async)
        jo_dma_copy_async(picture->data, (void *)(JO_VDP1_VRAM + JO_MULT_BY_8(texture->adr)),
                          (unsigned int)((JO_MULT_BY_4(texture->width * texture->height)) >> (picture->color_mode)));
    else
        jo_dma_copy(picture->data, (void *)(JO_VDP1_VRAM + JO_MULT_BY_8(texture->adr)),
                    (unsigned int)((JO_MULT_BY_4(texture->width * texture->height)) >> (picture->color_mode)));
    picture->data = (void *)(JO_VDP1_VRAM + JO_MULT_BY_8(texture->adr));
    __jo_sprite_end_codes[sprite_id] = false;
    __jo_sprite_mean_color[sprite_id] = 0;
#ifdef JO_COMPILE_WITH_3D_SUPPORT
    if (__jo_sprite_quad[sprite_id] != JO_NULL)
        jo_3d_free_sprite_quad(sprite_id);
#endif // JO_COMPILE_WITH_3D_SUPPORT
    return (sprite_id);
}

int				            jo_sprite_replace(const jo_img * const img, const int sprite_id)
{
    return (__jo_sprite_replace(img, sprite_id, false));
}

/** @brief Add a sprite in the VRAM after the last one
 *  @param prefix Bytes kept free before the pixels (a multiple of 32: the color lookup table of a 16 colors texture, see
 *  jo_sprite_add_4bits_lut()), freed with the sprite
 */
static int                  __internal_jo_sprite_add_prefix(void * const data, const unsigned short width, const unsigned short height,
                                                            const unsigned short cmode, const unsigned int prefix)
{
    jo_texture_definition   *texture;
    jo_picture_definition   *picture;

    if ((__jo_sprite_id + 1) >= JO_MAX_SPRITE)
    {
#ifdef JO_DEBUG
        jo_core_error("Too many sprites");
#endif
        return (-1);
    }
    if (!__jo_sprite_addr)
        __jo_sprite_addr = JO_VDP1_TEXTURE_DEF_BASE_ADDRESS;
    else
    {
        texture = &__jo_sprite_def[__jo_sprite_id];
        picture = &__jo_sprite_pic[__jo_sprite_id];
        __jo_sprite_addr = __jo_get_next_sprite_address(JO_MULT_BY_8(texture->adr),
                           texture->width,
                           texture->height,
                           picture->color_mode);
    }
    // Not enough VDP1 VRAM for the texture
    if (JO_VDP1_VRAM + __jo_get_next_sprite_address(__jo_sprite_addr + prefix, width, height, cmode) > JO_VDP1_USER_AREA_END_ADDR)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of sprite memory (%dx%d)", width, height);
#endif
        if (__jo_sprite_id < 0)
            JO_ZERO(__jo_sprite_addr);
        return (-1);
    }
    ++__jo_sprite_id;

    texture = &__jo_sprite_def[__jo_sprite_id];
    texture->width = width;
    texture->height = height;
    texture->adr = JO_DIV_BY_8(__jo_sprite_addr + prefix);
    texture->size = JO_MULT_BY_32(width & 0x1f8) | height;

    picture = &__jo_sprite_pic[__jo_sprite_id];
    picture->color_mode = cmode;
    picture->index = __jo_sprite_id;
    __jo_sprite_end_codes[__jo_sprite_id] = false;
    __jo_sprite_lut[__jo_sprite_id] = 0;
    __jo_sprite_mean_color[__jo_sprite_id] = 0;
    picture->data = (void *)(JO_VDP1_VRAM + JO_MULT_BY_8(texture->adr));

    if (data != JO_NULL)
        jo_dma_copy(data, picture->data, (unsigned int)((JO_MULT_BY_4(width * height)) >> cmode));

    return (__jo_sprite_id);
}

static  __jo_force_inline int __internal_jo_sprite_add(void * const data, const unsigned short width, const unsigned short height,
                                                       const unsigned short cmode)
{
    return (__internal_jo_sprite_add_prefix(data, width, height, cmode, 0));
}

int				jo_sprite_add(const jo_img * const img)
{
#ifdef JO_DEBUG
    if (img == JO_NULL)
    {
        jo_core_error("img is null");
        return (-1);
    }
#endif
    return __internal_jo_sprite_add(img->data, img->width, img->height, JO_VDP1_TEXTURE_SIZE_SHIFT_32K_COLORS);
}

int                         __jo_sprite_add_4bits(const void * const pixels, const unsigned short width, const unsigned short height,
                                                  const unsigned short lut)
{
    int                     id;

    id = __internal_jo_sprite_add((void *)pixels, width, height, JO_VDP1_TEXTURE_SIZE_SHIFT_16_COLORS);
    if (id >= 0)
        __jo_sprite_lut[id] = lut;
    return (id);
}

int                         jo_sprite_add_4bits_lut(const unsigned char * const pixels, const unsigned short width, const unsigned short height,
                                                    const jo_color * const lut)
{
    int                     id;

#ifdef JO_DEBUG
    if (pixels == JO_NULL || lut == JO_NULL)
    {
        jo_core_error("pixels or lut is null");
        return (-1);
    }
#endif
    // The table: 32 bytes just before the pixels, in the place of the sprite (one sprite for both: the tables used a sprite
    // of their own, half of the sprites of a game in 16 colors)
    id = __internal_jo_sprite_add_prefix((void *)pixels, width, height, JO_VDP1_TEXTURE_SIZE_SHIFT_16_COLORS, 32);
    if (id < 0)
        return (-1);
    __jo_sprite_lut[id] = __jo_sprite_def[id].adr - JO_DIV_BY_8(32);
    jo_dma_copy((void *)lut, (void *)(JO_VDP1_VRAM + JO_MULT_BY_8(__jo_sprite_lut[id])), 32);
    return (id);
}

int                         jo_sprite_add_image_4bits_lut(const jo_img * const img)
{
    jo_color                lut[16];
    unsigned char           *pixels;
    jo_color                color;
    int                     colors;
    int                     count;
    int                     index;
    int                     i;
    int                     k;
    int                     id;

#ifdef JO_DEBUG
    if (img == JO_NULL || img->data == JO_NULL)
    {
        jo_core_error("img is null");
        return (-1);
    }
#endif
    if (JO_MOD_POW2(img->width, 8) != 0)
        return (-1);
    // The table: the colors of the image (0: transparent, the entry 0 of the table)
    lut[0] = JO_COLOR_Transparent;
    colors = 1;
    count = img->width * img->height;
    for (JO_ZERO(i); i < count; ++i)
    {
        color = img->data[i];
        if (color == JO_COLOR_Transparent)
            continue;
        for (k = 1; k < colors && lut[k] != color; ++k)
            ;
        if (k < colors)
            continue;
        // (more than 14 colors: the image must stay in RGB. The index 15 is never used: it is the end code of the VDP1
        // in 16 colors mode when the end codes are enabled, 2 of them on a line of the texture stop the line)
        if (colors >= 15)
            return (-1);
        lut[colors++] = color;
    }
    for (k = colors; k < 16; ++k)
        lut[k] = JO_COLOR_Transparent;
    pixels = (unsigned char *)jo_malloc(JO_DIV_BY_2(count));
    if (pixels == JO_NULL)
        return (-1);
    // 2 pixels per byte, the left pixel in the high bits
    for (JO_ZERO(i); i < count; ++i)
    {
        color = img->data[i];
        JO_ZERO(index);
        if (color != JO_COLOR_Transparent)
            for (index = 1; lut[index] != color; ++index)
                ;
        if (JO_MOD_POW2(i, 2) == 0)
            pixels[JO_DIV_BY_2(i)] = (unsigned char)(index << 4);
        else
            pixels[JO_DIV_BY_2(i)] |= (unsigned char)index;
    }
    id = jo_sprite_add_4bits_lut(pixels, img->width, img->height, lut);
    jo_free(pixels);
    return (id);
}

int				            jo_sprite_add_8bits_image(const jo_img_8bits * const img)
{
#ifdef JO_DEBUG
    if (img == JO_NULL)
    {
        jo_core_error("img is null");
        return (-1);
    }
#endif
    return __internal_jo_sprite_add(img->data, img->width, img->height, JO_VDP1_TEXTURE_SIZE_SHIFT_256_COLORS);
}

bool                        jo_sprite_add_end_codes(const int sprite_id)
{
    jo_color                *line;
    int                     width;
    int                     y;
    int                     x;
    int                     last;

    if (sprite_id < 0 || sprite_id > __jo_sprite_id || __jo_sprite_pic[sprite_id].color_mode != JO_VDP1_TEXTURE_SIZE_SHIFT_32K_COLORS)
        return (false);
    if (__jo_sprite_end_codes[sprite_id])
        return (true);
    // The texture may still be copied in VRAM by the DMA controller (see __jo_sprite_replace())
    jo_dma_wait();
    width = __jo_sprite_def[sprite_id].width;
    line = (jo_color *)(JO_VDP1_VRAM + JO_MULT_BY_8(__jo_sprite_def[sprite_id].adr));
    for (JO_ZERO(y); y < __jo_sprite_def[sprite_id].height; ++y, line += width)
    {
        for (last = width - 1; last >= 0 && line[last] == JO_COLOR_Transparent; --last)
            ;
        // The VDP1 stops drawing the line after 2 end codes (a transparent line: at its start)
        for (x = last + 1; x <= last + 2 && x < width; ++x)
            line[x] = JO_VDP1_RGB_END_CODE;
    }
    __jo_sprite_end_codes[sprite_id] = true;
    return (true);
}

void                        jo_sprite_free_from(const int sprite_id)
{
    jo_texture_definition   *texture;

    texture = &__jo_sprite_def[sprite_id];
#ifdef JO_COMPILE_WITH_3D_SUPPORT
    register int    i;
#endif // JO_COMPILE_WITH_3D_SUPPORT
    if (sprite_id > __jo_sprite_id)
        return ;
#ifdef JO_COMPILE_WITH_3D_SUPPORT
    __jo_3d_gouraud_pool_freed(sprite_id);
    // The last sprite is included
    for (i = sprite_id; i <= __jo_sprite_id; ++i)
    {
        if (__jo_sprite_quad[i] != JO_NULL)
            jo_3d_free_sprite_quad(i);
    }
#endif // JO_COMPILE_WITH_3D_SUPPORT
    if (sprite_id == 0)
        JO_ZERO(__jo_sprite_addr);
    else
        __jo_sprite_addr = __jo_get_next_sprite_address(JO_MULT_BY_8(texture->adr),
                           texture->width,
                           texture->height,
                           __jo_sprite_pic[sprite_id].color_mode);
    __jo_sprite_id = sprite_id - 1;
}

void	            jo_set_gouraud_shading_colors(const jo_color topleft_color,
                                            const jo_color topright_color,
                                            const jo_color bottomright_color,
                                            const jo_color bottomleft_color)
{
    jo_color        *ptr;

    ++__jo_gouraud_shading_runtime_index;
    ptr = (jo_color *)(JO_VDP1_VRAM + 0x70000 + JO_MULT_BY_8(__jo_gouraud_shading_runtime_index));
    *ptr = topleft_color;
    *(ptr + 1) = topright_color;
    *(ptr + 2) = bottomright_color;
    *(ptr + 3) = bottomleft_color;
}

void                jo_set_gouraud_shading_brightness(const unsigned char brightness)
{
    jo_color        color;

    color = JO_COLOR_SATURN_RGB(brightness, brightness, brightness);
    jo_set_gouraud_shading_colors(color, color, color, color);
}

/** @brief Fill the attribute of a sprite
 *  @param attr Attribute (output)
 *  @param sprite_id Sprite Id
 *  @param state Sprite attributes (__jo_sprite_attributes, or the copy of the slave CPU)
 *  @remarks Same as JO_VDP1_SPRITE_ATTRIBUTE(0, JO_VDP1_NO_PALETTE, JO_VDP1_NO_GOURAUD, JO_VDP1_END_CODE_DISABLE, JO_VDP1_SPRITE_NO_FLIP | JO_VDP1_FUNCTION_SPRITE)
 *           and the sprite attributes of the state, field by field (an initialized local structure is copied with memcpy())
 */
static  __jo_force_inline void jo_sprite_attributes_of_state(jo_vdp1_sprite_attribute *attr, const int sprite_id,
                                                             const jo_sprite_attributes * const state)
{
    attr->direction = JO_VDP1_DIRECTION_COMMAND(JO_VDP1_SPRITE_NO_FLIP | JO_VDP1_FUNCTION_SPRITE) | state->direction;
    attr->draw_mode = JO_VDP1_DIRECTION_DRAW_MODE(JO_VDP1_SPRITE_NO_FLIP | JO_VDP1_FUNCTION_SPRITE) |
                      state->effect | state->clipping;
    // The VDP1 reads the lines of a sprite flipped horizontally from their end: it would stop at the end codes
    if (!__jo_sprite_end_codes[sprite_id] || (attr->direction & JO_VDP1_H_FLIP))
        attr->draw_mode |= JO_VDP1_END_CODE_DISABLE;
    attr->gouraud_table = JO_VDP1_NO_GOURAUD;
    if (__jo_sprite_pic[sprite_id].color_mode == JO_VDP1_TEXTURE_SIZE_SHIFT_32K_COLORS)
    {
        attr->draw_mode |= JO_VDP1_COLOR_MODE_RGB;
        JO_ZERO(attr->color);
    }
    else if (__jo_sprite_pic[sprite_id].color_mode == JO_VDP1_TEXTURE_SIZE_SHIFT_16_COLORS)
    {
        // 16 colors and their RGB colors in a lookup table: drawn like an RGB texture (gouraud, half transparency)
        attr->draw_mode |= JO_VDP1_COLOR_MODE_16_LOOKUP;
        attr->color = __jo_sprite_lut[sprite_id];
    }
    else
    {
        attr->draw_mode |= JO_VDP1_COLOR_MODE_256_BANK;
        attr->color = state->color_table_index;
    }
    // (a table given by jo_sprite_set_gouraud_table(), else the last one of jo_set_gouraud_shading_colors())
    if (state->effect & 4)
        attr->gouraud_table = state->gouraud_shading_index != 0 ? (unsigned short)state->gouraud_shading_index :
                              0xe000 + __jo_gouraud_shading_runtime_index;
    attr->texture_id = sprite_id;
}

void                    jo_sprite_draw_rotated_fixed(const int sprite_id, const jo_fixed x, const jo_fixed y, const jo_fixed z,
                                                     const jo_fixed half_width, const jo_fixed half_height, const int angle)
{
    jo_pos2D_fixed      p[4];
    jo_fixed            wc;
    jo_fixed            ws;
    jo_fixed            hc;
    jo_fixed            hs;

    wc = jo_fixed_mult(half_width, jo_cos(angle));
    ws = jo_fixed_mult(half_width, jo_sin(angle));
    hc = jo_fixed_mult(half_height, jo_cos(angle));
    hs = jo_fixed_mult(half_height, jo_sin(angle));
    // Corners (-w, -h), (w, -h), (w, h), (-w, h) turned: (u cos - v sin, u sin + v cos)
    p[0].x = x - wc + hs;
    p[0].y = y - ws - hc;
    p[1].x = x + wc + hs;
    p[1].y = y + ws - hc;
    p[2].x = x + wc - hs;
    p[2].y = y + ws + hc;
    p[3].x = x - wc - hs;
    p[3].y = y - ws + hc;
    jo_sprite_draw_4p_fixed(sprite_id, p, z, true);
}

/** @brief Fill the attribute of a sprite with the current sprite attributes (see jo_sprite_attributes_of_state()) */
static  __jo_force_inline void __jo_set_sprite_attributes(jo_vdp1_sprite_attribute *attr, const int sprite_id)
{
    jo_sprite_attributes_of_state(attr, sprite_id, &__jo_sprite_attributes);
}

void                    __jo_sprite_attributes_of(jo_vdp1_sprite_attribute *attr, const int sprite_id, const jo_sprite_attributes * const state)
{
    jo_sprite_attributes_of_state(attr, sprite_id, state);
}

void                    jo_sprite_draw_4p_fixed(const int sprite_id, const jo_pos2D_fixed * const four_points, const jo_fixed z, const bool centered_style_coordinates)
{
    jo_pos2D_fixed      points[4];
    jo_pos2D_fixed      delta;
    register int        i;
    jo_vdp1_sprite_attribute attr;

    if (sprite_id < 0)
        return;
    __jo_set_sprite_attributes(&attr, sprite_id);
    // (centered: the points as they are, no copy; the standing sprites of the 3D, hundreds a frame)
    if (centered_style_coordinates)
    {
        __jo_vdp1_draw_sprite_4p(four_points, z, &attr);
        return;
    }
    delta.x = jo_int2fixed(JO_TV_WIDTH_2 + JO_DIV_BY_2(__jo_sprite_def[sprite_id].width));
    delta.y = jo_int2fixed(JO_TV_HEIGHT_2 + JO_DIV_BY_2(__jo_sprite_def[sprite_id].height));
    for (JO_ZERO(i); i < 4; ++i)
    {
        points[i].x = four_points[i].x - delta.x;
        points[i].y = four_points[i].y - delta.y;
    }
    __jo_vdp1_draw_sprite_4p(points, z, &attr);
}

/** @brief Compute the position of a sprite for the VDP1
 *  @param sprite_id Sprite Id
 *  @param pos Position
 *  @param centered_style_coordinates true: (0, 0) is the center of the screen, false: (0, 0) is the top left corner
 *  @param output VDP1 sprite position
 */
static void             jo_sprite_compute_position(const int sprite_id, const jo_pos3D * const pos, const bool centered_style_coordinates, jo_vdp1_sprite_position *output)
{
    output->z = jo_int2fixed(pos->z);
    output->scale_x = __jo_sprite_attributes.fixed_scale_x;
    output->scale_y = __jo_sprite_attributes.fixed_scale_y;
    if (centered_style_coordinates)
    {
        output->x = jo_int2fixed(pos->x);
        output->y = jo_int2fixed(pos->y);
    }
    else
    {
        output->x = jo_int2fixed(pos->x - JO_TV_WIDTH_2 + JO_DIV_BY_2(__jo_sprite_def[sprite_id].width));
        output->y = jo_int2fixed(pos->y - JO_TV_HEIGHT_2 + JO_DIV_BY_2(__jo_sprite_def[sprite_id].height));
    }
}

void                    jo_sprite_draw(const int sprite_id, const jo_pos3D * const pos, const bool centered_style_coordinates, const bool billboard)
{
    jo_sprite_draw_rotate(sprite_id, pos, 0, centered_style_coordinates, billboard);
}

void                    jo_sprite_draw_rotate(const int sprite_id, const jo_pos3D * const pos, const int angle, const bool centered_style_coordinates, const bool billboard)
{
    jo_vdp1_sprite_position position;
    jo_angle                vdp1_angle;
    jo_vdp1_sprite_attribute attr;

    // No sprite (for instance jo_get_video_sprite() when the movie is stopped)
    if (sprite_id < 0)
        return;
    jo_sprite_compute_position(sprite_id, pos, centered_style_coordinates, &position);
    __jo_set_sprite_attributes(&attr, sprite_id);
    vdp1_angle = jo_DEGtoANG_int(angle);
    if (billboard)
        __jo_vdp1_draw_sprite_3d(&position, &attr, vdp1_angle);
    else if (position.scale_x != position.scale_y)
        __jo_vdp1_draw_sprite_hv(&position, &attr, vdp1_angle);
    else
        __jo_vdp1_draw_sprite(&position, &attr, vdp1_angle);
}

void                    jo_sprite_set_clipping_area(const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height, const int depth)
{
    jo_vdp1_command     *cmd;

    if ((cmd = __jo_vdp1_add_command(JO_MULT_BY_65536(depth))) == JO_NULL)
        return;
    cmd->ctrl = JO_VDP1_CMD_USER_CLIP;
    cmd->xa = x;
    cmd->ya = y;
    // Inclusive coordinates
    cmd->xc = x + width - 1;
    cmd->yc = y + height - 1;
}

/*
** END OF FILE
*/
