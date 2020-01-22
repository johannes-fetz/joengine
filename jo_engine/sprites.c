/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2017, Johannes Fetz (johannesfetz@gmail.com)
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
# define COLMODE_256        (4)
# define COLMODE_RGB        (5)

/*
** GLOBALS
*/

jo_sprite_attributes    __jo_sprite_attributes = {0, 0, 0, 0, JO_FIXED_1, No_Window};
jo_pos3D                __jo_sprite_pos = {0, 0, 120};
jo_texture_definition   __jo_sprite_def[JO_MAX_SPRITE];
jo_picture_definition   __jo_sprite_pic[JO_MAX_SPRITE];
int                     __jo_gouraud_shading_runtime_index = -1;
int				        __jo_hash_table[JO_MAX_SPRITE];
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

int                     jo_get_last_sprite_id(void)
{
    return (__jo_sprite_id);
}

void                    jo_sprite_init(void)
{
    register int		i;

    __jo_sprite_id = -1;
    for (JO_ZERO(i); i < JO_MAX_SPRITE; ++i)
        JO_ZERO(__jo_hash_table[i]);
}

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

int				            jo_sprite_replace(const jo_img * const img, const int sprite_id)
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
    jo_dma_copy(picture->data, (void *)(JO_VDP1_VRAM + JO_MULT_BY_8(texture->adr)),
                (unsigned int)((JO_MULT_BY_4(texture->width * texture->height)) >> (picture->color_mode)));
    picture->data = (void *)(JO_VDP1_VRAM + JO_MULT_BY_8(texture->adr));
#ifdef JO_COMPILE_WITH_3D_SUPPORT
    if (__jo_sprite_quad[sprite_id] != JO_NULL)
        jo_3d_free_sprite_quad(sprite_id);
#endif // JO_COMPILE_WITH_3D_SUPPORT
    return (sprite_id);
}

static int                  __internal_jo_sprite_add(void * const data, const unsigned short width, const unsigned short height, const unsigned short cmode)
{
    jo_texture_definition   *texture;
    jo_picture_definition   *picture;

#ifdef JO_DEBUG
    if (data == JO_NULL)
    {
        jo_core_error("data is null");
        return (-1);
    }
    if ((__jo_sprite_id + 1) >= JO_MAX_SPRITE)
    {
        jo_core_error("Too many sprites");
        return (-1);
    }
#endif
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
    ++__jo_sprite_id;

    texture = &__jo_sprite_def[__jo_sprite_id];
    texture->width = width;
    texture->height = height;
    texture->adr = JO_DIV_BY_8(__jo_sprite_addr);
    texture->size = JO_MULT_BY_32(width & 0x1f8) | height;

    picture = &__jo_sprite_pic[__jo_sprite_id];
    picture->color_mode = cmode;
    picture->index = __jo_sprite_id;
    picture->data = data;

    jo_dma_copy(data,
                (void *)(JO_VDP1_VRAM + JO_MULT_BY_8(texture->adr)),
                (unsigned int)((JO_MULT_BY_4(texture->width * texture->height)) >> (cmode)));
    picture->data = (void *)(JO_VDP1_VRAM + JO_MULT_BY_8(texture->adr));
    return (__jo_sprite_id);
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
    return __internal_jo_sprite_add(img->data, img->width, img->height, COL_32K);
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
    return __internal_jo_sprite_add(img->data, img->width, img->height, COL_256);
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
    for (i = sprite_id; i < __jo_sprite_id; ++i)
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

void	    jo_set_gouraud_shading_colors(const jo_color topleft_color,
        const jo_color topright_color,
        const jo_color bottomright_color,
        const jo_color bottomleft_color)
{
    unsigned short *ptr;
    ++__jo_gouraud_shading_runtime_index;
    ptr = (unsigned short *)(void *)(JO_VDP1_VRAM + 0x70000 + JO_MULT_BY_8(__jo_gouraud_shading_runtime_index));
    *ptr = topleft_color;
    *(ptr + 1) = topright_color;
    *(ptr + 2) = bottomright_color;
    *(ptr + 3) = bottomleft_color;
}

void     jo_set_gouraud_shading_brightness(const unsigned char brightness)
{
    jo_color    color;

    color = JO_COLOR_RGB(brightness, brightness, brightness);
    jo_set_gouraud_shading_colors(color, color, color, color);
}

#if JO_COMPILE_USING_SGL
static  __jo_force_inline void __jo_set_sprite_attributes(SPR_ATTR *attr, const int sprite_id)
{
    attr->dir |= __jo_sprite_attributes.direction;
    attr->atrb |= __jo_sprite_attributes.effect | __jo_sprite_attributes.clipping;
    if (__jo_sprite_pic[sprite_id].color_mode == COL_32K)
    {
        attr->atrb |= ((COLMODE_RGB & 7) << 3);
        JO_ZERO(attr->colno);
    }
    else
    {
        attr->atrb |= ((COLMODE_256 & 7) << 3);
        attr->colno = ((Uint16)VDP2_COLRAM) >> 3;
    }
    if (__jo_sprite_attributes.effect & 4)
        attr->gstb = 0xe000 + __jo_gouraud_shading_runtime_index;
    attr->texno = sprite_id;
}
#else
static  __jo_force_inline void __jo_set_sprite_attributes(jo_vdp1_command * const cmd, const int sprite_id)
{
    cmd->ctrl |= __jo_sprite_attributes.direction;
    cmd->pmod = 0x0080 | __jo_sprite_attributes.effect | __jo_sprite_attributes.clipping;
    if (__jo_sprite_pic[sprite_id].color_mode == COL_32K)
    {
        cmd->pmod |= ((COLMODE_RGB & 7) << 3);
        JO_ZERO(cmd->colr);
    }
    else
    {
        cmd->pmod |= ((COLMODE_256 & 7) << 3);
        cmd->colr = ((Uint16)VDP2_COLRAM) >> 3;
    }
    cmd->srca = (unsigned int)(__jo_sprite_pic[sprite_id].data) >> 3;
    cmd->size = (((__jo_sprite_def[sprite_id].width >> 3) & 0x3F) << 8) |
                (__jo_sprite_def[sprite_id].height & 0xFF);
    if (__jo_sprite_attributes.effect & 4)
        cmd->grda = (JO_VDP1_VRAM + 0x70000 + JO_MULT_BY_8(__jo_gouraud_shading_runtime_index)) >> 3;
}
#endif

void                    jo_sprite_draw(const int sprite_id, const jo_pos3D * const pos, const bool centered_style_coordinates, const bool billboard)
{
#if JO_COMPILE_USING_SGL
    FIXED               sgl_pos[XYZS];
    SPR_ATTR            attr = SPR_ATTRIBUTE(0, No_Palet, No_Gouraud, ECdis, sprNoflip | FUNC_Sprite);

    sgl_pos[2] = JO_MULT_BY_65536(pos->z);
    sgl_pos[3] = __jo_sprite_attributes.fixed_scale;
    if (centered_style_coordinates)
    {
        sgl_pos[0] = JO_MULT_BY_65536(pos->x);
        sgl_pos[1] = JO_MULT_BY_65536(pos->y);
    }
    else
    {
        sgl_pos[0] = JO_MULT_BY_65536(pos->x - JO_TV_WIDTH_2 + JO_DIV_BY_2(__jo_sprite_def[sprite_id].width));
        sgl_pos[1] = JO_MULT_BY_65536(pos->y - JO_TV_HEIGHT_2 + JO_DIV_BY_2(__jo_sprite_def[sprite_id].height));
    }
    __jo_set_sprite_attributes(&attr, sprite_id);
    if (billboard)
        slPutSprite(sgl_pos, &attr, 0);
    else
        slDispSprite(sgl_pos, &attr, 0);
#else
    JO_UNUSED_ARG(billboard);
    jo_vdp1_command     *cmd;
    unsigned int        sprite_width;
    unsigned int        sprite_height;

    if (__jo_sprite_attributes.fixed_scale != JO_FIXED_1)
    {
        cmd = jo_vdp1_create_command();
        cmd->ctrl = DrawScaledSprite;
        sprite_width = JO_DIV_BY_65536(__jo_sprite_def[sprite_id].width * __jo_sprite_attributes.fixed_scale);
        sprite_height = JO_DIV_BY_65536(__jo_sprite_def[sprite_id].height * __jo_sprite_attributes.fixed_scale);
        cmd->xc = pos->x + sprite_width;
        cmd->yc = pos->y + sprite_height;
        if (centered_style_coordinates)
        {
            cmd->xc += JO_TV_WIDTH_2 - JO_DIV_BY_2(sprite_width);
            cmd->yc += JO_TV_HEIGHT_2 - JO_DIV_BY_2(sprite_height);
        }
    }
    else
    {
        cmd = jo_vdp1_create_command();
        cmd->ctrl = DrawNormalSprite;
    }
    __jo_set_sprite_attributes(cmd, sprite_id);
    cmd->jo_engine_reserved = pos->z;
    cmd->xa = pos->x;
    cmd->ya = pos->y;
    if (centered_style_coordinates)
    {
        cmd->xa += JO_TV_WIDTH_2 - JO_DIV_BY_2(__jo_sprite_def[sprite_id].width);
        cmd->ya += JO_TV_HEIGHT_2 - JO_DIV_BY_2(__jo_sprite_def[sprite_id].height);
    }
#endif
}

void                    jo_sprite_draw_rotate(const int sprite_id, const jo_pos3D * const pos, const int angle, const bool centered_style_coordinates, const bool billboard)
{
#if JO_COMPILE_USING_SGL
    FIXED               sgl_pos[XYZS];
    SPR_ATTR            attr = SPR_ATTRIBUTE(0, No_Palet, No_Gouraud, ECdis, sprNoflip | FUNC_Sprite);

    sgl_pos[2] = JO_MULT_BY_65536(pos->z);
    sgl_pos[3] = __jo_sprite_attributes.fixed_scale;
    if (centered_style_coordinates)
    {
        sgl_pos[0] = JO_MULT_BY_65536(pos->x);
        sgl_pos[1] = JO_MULT_BY_65536(pos->y);
    }
    else
    {
        sgl_pos[0] = JO_MULT_BY_65536(pos->x - JO_TV_WIDTH_2 + JO_DIV_BY_2(__jo_sprite_def[sprite_id].width));
        sgl_pos[1] = JO_MULT_BY_65536(pos->y - JO_TV_HEIGHT_2 + JO_DIV_BY_2(__jo_sprite_def[sprite_id].height));
    }
    __jo_set_sprite_attributes(&attr, sprite_id);
    if (billboard)
        slPutSprite(sgl_pos, &attr, DEGtoANG(angle));
    else
        slDispSprite(sgl_pos, &attr, DEGtoANG(angle));
#else
    JO_UNUSED_ARG(billboard);
    jo_pos2D            rotation_origin;
    jo_pos2D            vertex;
    register int        cos_theta;
    register int        sin_theta;
    unsigned int        sprite_width;
    unsigned int        sprite_height;

    if (!angle)
    {
        jo_sprite_draw(sprite_id, pos, centered_style_coordinates, billboard);
        return ;
    }
    jo_vdp1_command *cmd = jo_vdp1_create_command();
    cmd->ctrl = DrawDistortedSprite;
    __jo_set_sprite_attributes(cmd, sprite_id);
    cmd->jo_engine_reserved = pos->z;

    cos_theta = jo_cos(angle);
    sin_theta = jo_sin(angle);

    if (__jo_sprite_attributes.fixed_scale != JO_FIXED_1)
    {
        sprite_width = JO_DIV_BY_65536(__jo_sprite_def[sprite_id].width * __jo_sprite_attributes.fixed_scale);
        sprite_height = JO_DIV_BY_65536(__jo_sprite_def[sprite_id].height * __jo_sprite_attributes.fixed_scale);
    }
    else
    {
        sprite_width = __jo_sprite_def[sprite_id].width;
        sprite_height = __jo_sprite_def[sprite_id].height;
    }
    rotation_origin.x = pos->x + JO_DIV_BY_2(sprite_width);
    rotation_origin.y = pos->y + JO_DIV_BY_2(sprite_height);

    vertex.x = (pos->x) - rotation_origin.x;
    vertex.y = (pos->y) - rotation_origin.y;
    cmd->xa = (JO_DIV_BY_32768(vertex.x * cos_theta - vertex.y * sin_theta)) + rotation_origin.x;
    cmd->ya = (JO_DIV_BY_32768(vertex.y * cos_theta + vertex.x * sin_theta)) + rotation_origin.y;

    vertex.x = (pos->x + sprite_width) - rotation_origin.x;
    vertex.y = (pos->y) - rotation_origin.y;
    cmd->xb = (JO_DIV_BY_32768(vertex.x * cos_theta - vertex.y * sin_theta)) + rotation_origin.x;
    cmd->yb = (JO_DIV_BY_32768(vertex.y * cos_theta + vertex.x * sin_theta)) + rotation_origin.y;

    vertex.x = (pos->x + sprite_width) - rotation_origin.x;
    vertex.y = (pos->y + sprite_height) - rotation_origin.y;
    cmd->xc = (JO_DIV_BY_32768(vertex.x * cos_theta - vertex.y * sin_theta)) + rotation_origin.x;
    cmd->yc = (JO_DIV_BY_32768(vertex.y * cos_theta + vertex.x * sin_theta)) + rotation_origin.y;

    vertex.x = (pos->x) - rotation_origin.x;
    vertex.y = (pos->y + sprite_height) - rotation_origin.y;
    cmd->xd = (JO_DIV_BY_32768(vertex.x * cos_theta - vertex.y * sin_theta)) + rotation_origin.x;
    cmd->yd = (JO_DIV_BY_32768(vertex.y * cos_theta + vertex.x * sin_theta)) + rotation_origin.y;

    if (centered_style_coordinates)
    {
        cmd->xa += JO_TV_WIDTH_2 - JO_DIV_BY_2(sprite_width);
        cmd->ya += JO_TV_HEIGHT_2 - JO_DIV_BY_2(sprite_height);
        cmd->xb += JO_TV_WIDTH_2 - JO_DIV_BY_2(sprite_width);
        cmd->yb += JO_TV_HEIGHT_2 - JO_DIV_BY_2(sprite_height);
        cmd->xc += JO_TV_WIDTH_2 - JO_DIV_BY_2(sprite_width);
        cmd->yc += JO_TV_HEIGHT_2 - JO_DIV_BY_2(sprite_height);
        cmd->xd += JO_TV_WIDTH_2 - JO_DIV_BY_2(sprite_width);
        cmd->yd += JO_TV_HEIGHT_2 - JO_DIV_BY_2(sprite_height);
    }
#endif
}

void jo_sprite_set_clipping_area(const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height, const int depth)
{
#if JO_COMPILE_USING_SGL
    SPRITE sprite;
    sprite.CTRL = FUNC_UserClip;
    sprite.XA = width;
    sprite.YA = height;
    sprite.XC = x;
    sprite.YC = y;
    slSetSprite(&sprite, JO_MULT_BY_65536(depth));
#else
    jo_vdp1_command *cmd = jo_vdp1_create_command();
    cmd->ctrl = SetUserClipping;
    cmd->xa = width;
    cmd->ya = height;
    cmd->xc = x;
    cmd->yc = y;
    cmd->jo_engine_reserved = depth;
#endif
}

/*
** END OF FILE
*/
