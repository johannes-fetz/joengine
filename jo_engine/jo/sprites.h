/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2020, Johannes Fetz (johannesfetz@gmail.com)
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
/** @file sprites.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Sprite definition and tools
 *  @bug No known bugs.
 */

#ifndef __JO_SPRITES_H__
# define __JO_SPRITES_H__

/*
** INCLUDES
*/
#include "types.h"
#include "image.h"
#include "tga.h"
#include "math.h"
#include "colors.h"

/** @brief Value used to keep the original scale */
# define JO_NO_ZOOM             (65535)

/** @brief Current displayed sprite attribute (internal engine usage)
 *  @warning MC Hammer: don't touch this
 */
extern jo_sprite_attributes    __jo_sprite_attributes;
/** @brief Current displayed sprite position (internal engine usage)
 *  @warning MC Hammer: don't touch this
 */
extern jo_pos3D                     __jo_sprite_pos;
/** @brief (internal engine usage)
 *  @warning MC Hammer: don't touch this
 */
extern jo_texture_definition        __jo_sprite_def[JO_MAX_SPRITE];
/** @brief (internal engine usage)
 *  @warning MC Hammer: don't touch this
 */
extern jo_picture_definition        __jo_sprite_pic[JO_MAX_SPRITE];
/** @brief (internal engine usage)
 *  @warning MC Hammer: don't touch this
 */
void    jo_sprite_draw(const int sprite_id, const jo_pos3D * const pos, const bool centered_style_coordinates, const bool billboard);
/** @brief (internal engine usage)
 *  @warning MC Hammer: don't touch this
 */
void    jo_sprite_draw_rotate(const int sprite_id, const jo_pos3D * const pos, const int angle, const bool centered_style_coordinates, const bool billboard);

/*
** INTERNAL
*/
/** @brief Internal usage don't use it */
static  __jo_force_inline void	__internal_jo_sprite_set_position3D(const int x, const int y, const int z)
{
    __jo_sprite_pos.x = x;
    __jo_sprite_pos.y = y;
    __jo_sprite_pos.z = z;
}

/** @brief Get last Sprite Id
 *  @return Last Sprite Id
 */
int     jo_get_last_sprite_id(void);

/** @brief Add a sprite
 *  @param img Pointer to a 15 bits image struct
 *  @return Sprite Id or -1 if failed
 */
int		jo_sprite_add(const jo_img * const img);

/** @brief Add a 8 bits sprite
 *  @param img Pointer to a 8 bits bits image struct
 *  @return Sprite Id or -1 if failed
 *  @warning 255 colors maximum not 256 !
 */
int     jo_sprite_add_8bits_image(const jo_img_8bits * const img);

/** @brief Replace a sprite
 *  @param img Pointer to a image struct
 *  @param sprite_id Sprite ID to replace
 *  @return sprite_id
 */
int     jo_sprite_replace(const jo_img * const img, const int sprite_id);

/** @brief Free all sprites from the given sprite_id
 *  @param sprite_id Sprite ID to replace
 *  @warning Don't use freed sprites after this call or the game may crash unexpectedly
 */
void    jo_sprite_free_from(const int sprite_id);

/** @brief Free all sprites
 *  @warning Don't use any sprites after this call or the game may crash unexpectedly
 */
static  __jo_force_inline void	jo_sprite_free_all(void)
{
    jo_sprite_free_from(0);
}

# ifdef JO_COMPILE_WITH_SPRITE_HASHTABLE
/** @brief Retrive the Sprite Id from filename based on the four first character in the filename
 *  @param filename Filename (upper case and shorter as possible like "A.TEX")
 *  @warning Slow, use carefully
 *  @return Sprite Id of the first image or -1 if not found
 */
int		jo_sprite_name2id(const char *const filename);
#endif
/*
** Sprite clipping
*/
/** @brief Activate sprite clipping for every sprite displayed after this call
 * @param outside Clip everything inside clipping area
 */
static  __jo_force_inline void	jo_sprite_enable_clipping(bool outside)
{
    __jo_sprite_attributes.clipping = outside ? Window_Out : Window_In;
}

/** @brief Disable sprite clipping for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_disable_clipping(void)
{
    __jo_sprite_attributes.clipping = No_Window;
}

/*
** Screen doors filter (old school transparency)
*/
/** @brief Activate screen doors filter for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_enable_screen_doors_filter(void)
{
    __jo_sprite_attributes.effect |= 256;
}

/** @brief Disable screen doors filter for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_disable_screen_doors_filter(void)
{
    __jo_sprite_attributes.effect &= ~(256);
}


/** @brief Activate shadow filter for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_enable_shadow_filter(void)
{
    __jo_sprite_attributes.effect |= 1;
}

/** @brief Disable shadow filter for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_disable_shadow_filter(void)
{
    __jo_sprite_attributes.effect &= ~(1);
}

/*
** Dark filter
*/
/** @brief Activate dark filter for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_enable_dark_filter(void)
{
    __jo_sprite_attributes.effect |= 2;
}

/** @brief Disable dark filter for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_disable_dark_filter(void)
{
    __jo_sprite_attributes.effect &= ~(2);
}

/*
** Half transparency
*/
/** @brief Activate half transparency for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_enable_half_transparency(void)
{
    __jo_sprite_attributes.effect |= 3;
}

/** @brief Disable half transparency for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_disable_half_transparency(void)
{
    __jo_sprite_attributes.effect &= ~(3);
}

/*
** Gouraud Shading
*/
/** @brief Value used to keep the original color for jo_set_gouraud_shading_colors() */
# define JO_NO_GOURAUD_COLOR            (0xc210)

/** @brief Change Gouraud Shading colors
 *  @param topleft_color Top left corner color in sprite
 *  @param topright_color Top right corner color in sprite
 *  @param bottomright_color bottom right corner color in sprite
 *  @param bottomleft_color bottom left corner color in sprite
 */
void	jo_set_gouraud_shading_colors(const jo_color topleft_color,
                                      const jo_color topright_color,
                                      const jo_color bottomright_color,
                                      const jo_color bottomleft_color);

/** @brief Activate Gouraud Shading for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_enable_gouraud_shading(void)
{
    __jo_sprite_attributes.effect |= 4;
}

/** @brief Disable Gouraud Shading for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_disable_gouraud_shading(void)
{
    __jo_sprite_attributes.effect &= ~(4);
}

/** @brief Default brightness for jo_set_gouraud_shading_brightness() */
# define JO_DEFAULT_BRIGHTNESS      (16)

/** @brief Maximum brightness value for jo_set_gouraud_shading_brightness() */
# define JO_MAX_BRIGHTNESS          (31)

/** @brief Change Gouraud Shading brightness
 *  @param brightness Brightness from 0 to 31
 */
void     jo_set_gouraud_shading_brightness(const unsigned char brightness);

/*
** Vertical flip
*/
/** @brief Activate vertical flip for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_enable_vertical_flip(void)
{
    __jo_sprite_attributes.direction |= 32;
}

/** @brief Disable vertical flip for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_disable_vertical_flip(void)
{
    __jo_sprite_attributes.direction &= ~(32);
}

/*
** Horizontal flip
*/
/** @brief Activate horizontal flip for every sprite displayed after this call */
static  __jo_force_inline void	jo_sprite_enable_horizontal_flip(void)
{
    __jo_sprite_attributes.direction |= 16;
}

/** @brief Disable horizontal flip for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_disable_horizontal_flip(void)
{
    __jo_sprite_attributes.direction &= ~(16);
}

/*
** Palette
*/
/** @brief Change palette for every 8 bits sprite displayed after this call
 *  @param palette_id Palette id from jo_create_palette()
 */
static  __jo_force_inline void	jo_sprite_set_palette(int palette_id)
{
    __jo_sprite_attributes.color_table_index = JO_MULT_BY_256(palette_id);
}

/*
** Change sprite scale
*/
/** @brief Change scale for every sprite displayed after this call
 *  @param scale Sprite scale
 */
static  __jo_force_inline void	jo_sprite_change_sprite_scale(const float scale)
{
    __jo_sprite_attributes.fixed_scale = (int)(scale * 65535.f);
}

/** @brief Restore default scale for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_restore_sprite_scale(void)
{
    __jo_sprite_attributes.fixed_scale = JO_NO_ZOOM;
}

/** @brief Set user clipping area
 * @param x Horizontal coord from top left corner
 * @param y Vertical coord from top left corner
 * @param width Width of the clipping rectangle
 * @param height Height of the clipping rectangle
 * @param depth Specifies depth up to which sprites get clipped
 */
void jo_sprite_set_clipping_area(const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height, const int depth);

/** @brief Display a sprite in 3 dimensional space
 *  @param sprite_id Sprite Id
 *  @param x Horizontal position from the center of the screen
 *  @param y Vertical position from the center of the screen
 *  @param z Z index (depth)
 *  @param angle Angle (Z axe)
 */
static  __jo_force_inline void	jo_sprite_draw3D_and_rotate(const int sprite_id, const int x, const int y, const int z, const int angle)
{
    __internal_jo_sprite_set_position3D(x, y, z);
    jo_sprite_draw_rotate(sprite_id, &__jo_sprite_pos, angle, true, false);
}

/** @brief Display a sprite in 3 dimensional space
 *  @param sprite_id Sprite Id
 *  @param x Horizontal position from the center of the screen
 *  @param y Vertical position from the center of the screen
 *  @param z Z index (depth))
 */
static  __jo_force_inline void	jo_sprite_draw3D(const int sprite_id, const int x, const int y, const int z)
{
    __internal_jo_sprite_set_position3D(x, y, z);
    jo_sprite_draw(sprite_id, &__jo_sprite_pos, true, false);
}

/** @brief Display a sprite in 3 dimensional space
 *  @param sprite_id Sprite Id returned by jo_sprite_add(), jo_sprite_add_tga() or jo_sprite_add_image_pack()
 *  @param x Horizontal position from the top left corner
 *  @param y Vertical position from the top left corner
 *  @param z Z index (depth)
 *  @param angle Angle (Z axe)
 */
static  __jo_force_inline void	jo_sprite_draw3D_and_rotate2(const int sprite_id, const int x, const int y, const int z, const int angle)
{
    __internal_jo_sprite_set_position3D(x, y, z);
    jo_sprite_draw_rotate(sprite_id, &__jo_sprite_pos, angle, false, false);
}

/** @brief Display a sprite in 3 dimensional space
 *  @param sprite_id Sprite Id returned by jo_sprite_add(), jo_sprite_add_tga() or jo_sprite_add_image_pack()
 *  @param x Horizontal position from the top left corner
 *  @param y Vertical position from the top left corner
 *  @param z Z index (depth)
 */
static  __jo_force_inline void	jo_sprite_draw3D2(const int sprite_id, const int x, const int y, const int z)
{
    __internal_jo_sprite_set_position3D(x, y, z);
    jo_sprite_draw(sprite_id, &__jo_sprite_pos, false, false);
}

/*
** Some helper to get pixel color in texture
*/
/** @brief Get a specific pixel color from image bytes
 *  @param data Bytes (raw image data)
 *  @param x Horizontal ccord in the image
 *  @param y Vertical ccord in the image
 *  @param image_width Image width (in pixels)
 */
static  __jo_force_inline jo_color        jo_sprite_get_pixel_color(const jo_color * const data, const unsigned int x, const unsigned int y, const unsigned int image_width)
{
    return data[x + y * image_width];
}

/** @brief Get if a specific pixel is transparent (JO_COLOR_Transparent) in image bytes
 *  @param data Bytes (raw image data)
 *  @param x Horizontal ccord in the image
 *  @param y Vertical ccord in the image
 *  @return true if the pixel is transparent otherwise false
 *  @param image_width Image width (in pixels)
 */
static  __jo_force_inline bool        jo_sprite_is_pixel_transparent(const jo_color * const data, const unsigned int x, const unsigned int y, const unsigned int image_width)
{
    return jo_sprite_get_pixel_color(data, x, y, image_width) == JO_COLOR_Transparent;
}

/** @brief Get sprite width
 *  @param sprite_id Sprite Id returned by jo_sprite_add(), jo_sprite_add_tga() or jo_sprite_add_image_pack()
 *  @return Sprite width
 */
static  __jo_force_inline int         jo_sprite_get_width(const int sprite_id)
{
    return (__jo_sprite_def[sprite_id].width);
}

/** @brief Get sprite height
 *  @param sprite_id Sprite Id returned by jo_sprite_add(), jo_sprite_add_tga() or jo_sprite_add_image_pack()
 *  @return Sprite height
 */
static  __jo_force_inline int         jo_sprite_get_height(const int sprite_id)
{
    return (__jo_sprite_def[sprite_id].height);
}

/** @brief Get sprite count
 *  @return Count
 */
static  __jo_force_inline int         jo_sprite_count(void)
{
    return (jo_get_last_sprite_id() + 1);
}

/** @brief Get sprite memory usage
 *  @return Sprite memory usage percent
 *  @remarks jo_printf(0, 0, "Sprite memory usage: %d%%  ", jo_sprite_usage_percent());
 */
int                                   jo_sprite_usage_percent(void);

#endif /* !__JO_SPRITES_H__ */

/*
** END OF FILE
*/
