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
/** @file internal/sprites.h
 *  @author Johannes Fetz
 *
 *  @brief Sprites: internal functions of the engine (see jo/sprites.h)
 *  @warning Not part of the API: only for the source files of the engine (included after jo/sprites.h). The games
 *  include jo/jo.h, which doesn't include this file
 *  @bug No known bugs.
 */

#ifndef __JO_INTERNAL_SPRITES_H__
# define __JO_INTERNAL_SPRITES_H__

/** @brief (internal engine usage) Color lookup table of a 16 colors texture (VRAM address / 8), 0: none */
extern unsigned short           __jo_sprite_lut[JO_MAX_SPRITE];

/** @brief (internal engine usage) The sprite has VDP1 end codes (see jo_sprite_add_end_codes()) */
extern bool                     __jo_sprite_end_codes[JO_MAX_SPRITE];

/** @brief (internal engine usage) Gouraud tables of jo_set_gouraud_shading_colors(): after the textures
 *  (JO_VDP1_USER_AREA_END_ADDR), 2 sets of JO_SPRITE_GOURAUD_TABLES tables (the VDP1 draws a frame while the CPU prepares
 *  the next one: another set) */
# define JO_SPRITE_GOURAUD_TABLES       (32)
# define JO_SPRITE_GOURAUD_OFFSET       (JO_VDP1_USER_AREA_END_ADDR - JO_VDP1_VRAM)

/** @brief (internal engine usage) The last table of jo_set_gouraud_shading_colors() in the frame, -1: none */
extern int                      __jo_gouraud_shading_runtime_index;

/** @brief (internal engine usage) The set of the tables of jo_set_gouraud_shading_colors() in the frame (0 or 1) */
extern int                      __jo_gouraud_shading_set;

/** @brief Fill the attribute of a sprite (internal engine usage: the sprites, the fast path of the VDP1 for
 *  jo_sprite_draw3D())
 *  @param attr Attribute (output)
 *  @param sprite_id Sprite Id
 *  @param state Sprite attributes (__jo_sprite_attributes, or the copy of the slave CPU)
 *  @remarks Same as JO_VDP1_SPRITE_ATTRIBUTE(0, JO_VDP1_NO_PALETTE, JO_VDP1_NO_GOURAUD, JO_VDP1_END_CODE_DISABLE, JO_VDP1_SPRITE_NO_FLIP | JO_VDP1_FUNCTION_SPRITE)
 *           and the sprite attributes of the state, field by field (an initialized local structure is copied with memcpy())
 */
static  __jo_force_inline void   __jo_sprite_attributes_of_state(jo_vdp1_sprite_attribute *attr, const int sprite_id,
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
    // (a table given by jo_sprite_set_gouraud_table(), else the last one of jo_set_gouraud_shading_colors(), else (none in
    // this frame) the neutral table of the 3D light)
    if (state->effect & 4)
        attr->gouraud_table = state->gouraud_shading_index != 0 ? (unsigned short)state->gouraud_shading_index :
                              __jo_gouraud_shading_runtime_index >= 0 ?
                              (unsigned short)(JO_DIV_BY_8(JO_SPRITE_GOURAUD_OFFSET) + __jo_gouraud_shading_set * JO_SPRITE_GOURAUD_TABLES +
                                               __jo_gouraud_shading_runtime_index) :
                              (unsigned short)(JO_DIV_BY_8(JO_VDP1_LIGHT_TABLE_OFFSET) + 16);
    attr->texture_id = sprite_id;
}

/** @brief Add a 16 colors texture that uses the color lookup table of another one (internal engine usage: smaller copies,
 *  tiles, see 3d.c)
 *  @param pixels 2 pixels per byte
 *  @param width Width (multiple of 8)
 *  @param height Height
 *  @param lut Table (VRAM address / 8, see __jo_sprite_lut)
 *  @return Sprite Id or -1 if failed
 */
int     __jo_sprite_add_4bits(const void * const pixels, const unsigned short width, const unsigned short height, const unsigned short lut);

/** @brief Replace a sprite (internal usage)
 *  @param img Pointer to a image struct
 *  @param sprite_id Sprite ID to replace
 *  @param async true: the copy is done by the DMA controller in the background (see jo_dma_wait())
 *  @return sprite_id
 */
int     __jo_sprite_replace(const jo_img * const img, const int sprite_id, const bool async);

#endif /* !__JO_INTERNAL_SPRITES_H__ */

/*
** END OF FILE
*/
