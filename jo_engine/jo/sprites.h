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
/** @file sprites.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Sprite definition and tools
 *  @bug No known bugs.
 */

#ifndef __JO_SPRITES_H__
# define __JO_SPRITES_H__

/*
██╗███╗   ██╗████████╗███████╗██████╗ ███╗   ██╗ █████╗ ██╗
██║████╗  ██║╚══██╔══╝██╔════╝██╔══██╗████╗  ██║██╔══██╗██║
██║██╔██╗ ██║   ██║   █████╗  ██████╔╝██╔██╗ ██║███████║██║
██║██║╚██╗██║   ██║   ██╔══╝  ██╔══██╗██║╚██╗██║██╔══██║██║
██║██║ ╚████║   ██║   ███████╗██║  ██║██║ ╚████║██║  ██║███████╗
╚═╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝╚══════╝
▲ DO NOT USE THESE FUNCTIONS ▲
*/

/** @brief Current displayed sprite attribute (internal engine usage)
 *  @warning MC Hammer: don't touch this
 */
extern jo_sprite_attributes    __jo_sprite_attributes;
/** @brief (internal engine usage)
 *  @warning MC Hammer: don't touch this
 */
extern jo_texture_definition        __jo_sprite_def[JO_MAX_SPRITE];
/** @brief (internal engine usage)
 *  @warning MC Hammer: don't touch this
 */
extern jo_picture_definition   __jo_sprite_pic[JO_MAX_SPRITE];
/** @brief (internal engine usage) Color lookup table of a 16 colors texture (VRAM address / 8), 0: none */
extern unsigned short           __jo_sprite_lut[JO_MAX_SPRITE];
/** @brief (internal engine usage)
 *  @warning MC Hammer: don't touch this
 */
void    jo_sprite_draw(const int sprite_id, const jo_pos3D * const pos, const bool centered_style_coordinates, const bool billboard);
/** @brief (internal engine usage)
 *  @warning MC Hammer: don't touch this
 */
void    jo_sprite_draw_rotate(const int sprite_id, const jo_pos3D * const pos, const int angle, const bool centered_style_coordinates, const bool billboard);
/** @brief Internal usage don't use it */
static  __jo_force_inline void	__internal_jo_sprite_set_position3D(const int x, const int y, const int z)
{
    extern jo_pos3D __jo_sprite_pos;
    __jo_sprite_pos.x = x;
    __jo_sprite_pos.y = y;
    __jo_sprite_pos.z = z;
}

/*
 █████╗ ██████╗ ██████╗     ██╗██████╗ ███████╗██████╗ ██╗      █████╗  ██████╗███████╗    ██╗███████╗██████╗ ███████╗███████╗
██╔══██╗██╔══██╗██╔══██╗   ██╔╝██╔══██╗██╔════╝██╔══██╗██║     ██╔══██╗██╔════╝██╔════╝   ██╔╝██╔════╝██╔══██╗██╔════╝██╔════╝
███████║██║  ██║██║  ██║  ██╔╝ ██████╔╝█████╗  ██████╔╝██║     ███████║██║     █████╗    ██╔╝ █████╗  ██████╔╝█████╗  █████╗
██╔══██║██║  ██║██║  ██║ ██╔╝  ██╔══██╗██╔══╝  ██╔═══╝ ██║     ██╔══██║██║     ██╔══╝   ██╔╝  ██╔══╝  ██╔══██╗██╔══╝  ██╔══╝
██║  ██║██████╔╝██████╔╝██╔╝   ██║  ██║███████╗██║     ███████╗██║  ██║╚██████╗███████╗██╔╝   ██║     ██║  ██║███████╗███████╗
╚═╝  ╚═╝╚═════╝ ╚═════╝ ╚═╝    ╚═╝  ╚═╝╚══════╝╚═╝     ╚══════╝╚═╝  ╚═╝ ╚═════╝╚══════╝╚═╝    ╚═╝     ╚═╝  ╚═╝╚══════╝╚══════╝
*/

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

/** @brief Add a 16 colors texture with its color lookup table: 4 bits per pixel (4 times less VRAM than an RGB texture),
 *  drawn like an RGB texture (the colors of the table are RGB: gouraud shading, half transparency, lights)
 *  @param pixels 2 pixels per byte (the left pixel in the high bits), the color 0 is transparent
 *  @param width Width (multiple of 8)
 *  @param height Height
 *  @param lut The 16 colors (RGB 555 of the Saturn, the color 0 is not used: transparent)
 *  @return Sprite Id or -1 if failed
 *  @remarks The table is kept in the VRAM just before the pixels, in the place of the texture (no sprite of its own)
 */
int     jo_sprite_add_4bits_lut(const unsigned char * const pixels, const unsigned short width, const unsigned short height,
                                const jo_color * const lut);

/** @brief Add an RGB image of 14 colors at most (the index 15 is the end code of the VDP1) as a 16 colors texture with its color lookup table (see
 *  jo_sprite_add_4bits_lut()): the same pixels for 4 times less VRAM (a grey detail texture tinted by the gouraud shading
 *  has a few levels of grey)
 *  @param img Image (RGB, the width a multiple of 8, JO_COLOR_Transparent: transparent)
 *  @return Sprite Id, -1 if the image has more than 14 colors (add it with jo_sprite_add()) or if failed
 */
int     jo_sprite_add_image_4bits_lut(const jo_img * const img);

/** @brief Add a 16 colors texture that uses the color lookup table of another one (internal engine usage: smaller copies,
 *  tiles, see 3d.c)
 *  @param pixels 2 pixels per byte
 *  @param width Width (multiple of 8)
 *  @param height Height
 *  @param lut Table (VRAM address / 8, see __jo_sprite_lut)
 *  @return Sprite Id or -1 if failed
 */
int     __jo_sprite_add_4bits(const void * const pixels, const unsigned short width, const unsigned short height, const unsigned short lut);

/** @brief Replace a sprite
 *  @param img Pointer to a image struct
 *  @param sprite_id Sprite ID to replace
 *  @return sprite_id
 */
int     jo_sprite_replace(const jo_img * const img, const int sprite_id);

/** @brief Replace a sprite (internal usage)
 *  @param img Pointer to a image struct
 *  @param sprite_id Sprite ID to replace
 *  @param async true: the copy is done by the DMA controller in the background (see jo_dma_wait())
 *  @return sprite_id
 */
int     __jo_sprite_replace(const jo_img * const img, const int sprite_id, const bool async);

/** @brief Write the VDP1 end codes after the last visible pixel of each line of a sprite (RGB sprites)
 *  @param sprite_id Sprite Id
 *  @return false if the sprite is not a RGB sprite
 *  @remarks The VDP1 stops drawing a line of a sprite after 2 end codes: the transparent pixels at the right of
 *  the lines (and the transparent lines) are not drawn, the sprite is drawn faster. Useful for the sprites with many
 *  transparent pixels (plants, clouds, smoke). The VDP1 draws all the pixels of a sprite flipped horizontally
 *  (it reads the lines from their end). Call it before jo_3d_set_texture_mipmaps()
 *  @warning The end code of the RGB sprites is the color 0x7FFF (white without the MSB): not used by the images
 */
bool    jo_sprite_add_end_codes(const int sprite_id);

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
████████╗██████╗  █████╗ ███╗   ██╗███████╗██████╗  █████╗ ██████╗ ███████╗███╗   ██╗ ██████╗██╗   ██╗
╚══██╔══╝██╔══██╗██╔══██╗████╗  ██║██╔════╝██╔══██╗██╔══██╗██╔══██╗██╔════╝████╗  ██║██╔════╝╚██╗ ██╔╝
   ██║   ██████╔╝███████║██╔██╗ ██║███████╗██████╔╝███████║██████╔╝█████╗  ██╔██╗ ██║██║      ╚████╔╝
   ██║   ██╔══██╗██╔══██║██║╚██╗██║╚════██║██╔═══╝ ██╔══██║██╔══██╗██╔══╝  ██║╚██╗██║██║       ╚██╔╝
   ██║   ██║  ██║██║  ██║██║ ╚████║███████║██║     ██║  ██║██║  ██║███████╗██║ ╚████║╚██████╗   ██║
   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚══════╝╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═══╝ ╚═════╝   ╚═╝
*/

/*
** Screen doors filter (old school transparency)
*/

/** @brief Activate screen doors filter for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_enable_screen_doors_filter(void)
{
    JO_ADD_FLAG(__jo_sprite_attributes.effect, 0x100);
}

/** @brief Disable screen doors filter for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_disable_screen_doors_filter(void)
{
    JO_REMOVE_FLAG(__jo_sprite_attributes.effect, 0x100);
}

/*
** Half transparency
*/

/** @brief Activate half transparency for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_enable_half_transparency(void)
{
    JO_ADD_FLAG(__jo_sprite_attributes.effect, 0x3);
}

/** @brief Disable half transparency for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_disable_half_transparency(void)
{
    JO_REMOVE_FLAG(__jo_sprite_attributes.effect, 0x3);
}

/*
███████╗██╗  ██╗ █████╗ ██████╗  ██████╗ ██╗    ██╗
██╔════╝██║  ██║██╔══██╗██╔══██╗██╔═══██╗██║    ██║
███████╗███████║███████║██║  ██║██║   ██║██║ █╗ ██║
╚════██║██╔══██║██╔══██║██║  ██║██║   ██║██║███╗██║
███████║██║  ██║██║  ██║██████╔╝╚██████╔╝╚███╔███╔╝
╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═════╝  ╚═════╝  ╚══╝╚══╝
*/

/** @brief Activate shadow filter for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_enable_shadow_filter(void)
{
    JO_ADD_FLAG(__jo_sprite_attributes.effect, 0x1);
}

/** @brief Disable shadow filter for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_disable_shadow_filter(void)
{
    JO_REMOVE_FLAG(__jo_sprite_attributes.effect, 0x1);
}

/*
** Dark filter
*/

/** @brief Activate dark filter for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_enable_dark_filter(void)
{
    JO_ADD_FLAG(__jo_sprite_attributes.effect, 0x2);
}

/** @brief Disable dark filter for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_disable_dark_filter(void)
{
    JO_REMOVE_FLAG(__jo_sprite_attributes.effect, 0x2);
}

/*
 ██████╗  ██████╗ ██╗   ██╗██████╗  █████╗ ██╗   ██╗██████╗     ███████╗██╗  ██╗ █████╗ ██████╗ ██╗███╗   ██╗ ██████╗
██╔════╝ ██╔═══██╗██║   ██║██╔══██╗██╔══██╗██║   ██║██╔══██╗    ██╔════╝██║  ██║██╔══██╗██╔══██╗██║████╗  ██║██╔════╝
██║  ███╗██║   ██║██║   ██║██████╔╝███████║██║   ██║██║  ██║    ███████╗███████║███████║██║  ██║██║██╔██╗ ██║██║  ███╗
██║   ██║██║   ██║██║   ██║██╔══██╗██╔══██║██║   ██║██║  ██║    ╚════██║██╔══██║██╔══██║██║  ██║██║██║╚██╗██║██║   ██║
╚██████╔╝╚██████╔╝╚██████╔╝██║  ██║██║  ██║╚██████╔╝██████╔╝    ███████║██║  ██║██║  ██║██████╔╝██║██║ ╚████║╚██████╔╝
 ╚═════╝  ╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝     ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═════╝ ╚═╝╚═╝  ╚═══╝ ╚═════╝
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
    JO_ADD_FLAG(__jo_sprite_attributes.effect, 0x4);
}

/** @brief Disable Gouraud Shading for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_disable_gouraud_shading(void)
{
    JO_REMOVE_FLAG(__jo_sprite_attributes.effect, 0x4);
}

/** @brief Gouraud table of the sprites displayed after this call with jo_sprite_enable_gouraud_shading() (instead of the last
 *  table of jo_set_gouraud_shading_colors(): the tables given by jo_3d_add_gouraud_tables() are kept from a frame to the next)
 *  @param table Address / 8 of the table (see jo_3d_add_gouraud_tables()), 0: the last table of jo_set_gouraud_shading_colors()
 */
static  __jo_force_inline void	jo_sprite_set_gouraud_table(const unsigned short table)
{
    __jo_sprite_attributes.gouraud_shading_index = table;
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
██╗  ██╗ ██████╗ ██████╗ ██╗███████╗    ██╗██╗   ██╗███████╗██████╗ ████████╗    ███████╗██╗     ██╗██████╗
██║  ██║██╔═══██╗██╔══██╗██║╚══███╔╝   ██╔╝██║   ██║██╔════╝██╔══██╗╚══██╔══╝    ██╔════╝██║     ██║██╔══██╗
███████║██║   ██║██████╔╝██║  ███╔╝   ██╔╝ ██║   ██║█████╗  ██████╔╝   ██║       █████╗  ██║     ██║██████╔╝
██╔══██║██║   ██║██╔══██╗██║ ███╔╝   ██╔╝  ╚██╗ ██╔╝██╔══╝  ██╔══██╗   ██║       ██╔══╝  ██║     ██║██╔═══╝
██║  ██║╚██████╔╝██║  ██║██║███████╗██╔╝    ╚████╔╝ ███████╗██║  ██║   ██║       ██║     ███████╗██║██║
╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═╝╚═╝╚══════╝╚═╝      ╚═══╝  ╚══════╝╚═╝  ╚═╝   ╚═╝       ╚═╝     ╚══════╝╚═╝╚═╝
*/

/** @brief Activate vertical flip for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_enable_vertical_flip(void)
{
    JO_ADD_FLAG(__jo_sprite_attributes.direction, 0x20);
}

/** @brief Disable vertical flip for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_disable_vertical_flip(void)
{
    JO_REMOVE_FLAG(__jo_sprite_attributes.direction, 0x20);
}

/** @brief Activate horizontal flip for every sprite displayed after this call */
static  __jo_force_inline void	jo_sprite_enable_horizontal_flip(void)
{
    JO_ADD_FLAG(__jo_sprite_attributes.direction, 0x10);
}

/** @brief Disable horizontal flip for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_disable_horizontal_flip(void)
{
    JO_REMOVE_FLAG(__jo_sprite_attributes.direction, 0x10);
}

/*
███████╗ ██████╗ █████╗ ██╗     ███████╗
██╔════╝██╔════╝██╔══██╗██║     ██╔════╝
███████╗██║     ███████║██║     █████╗
╚════██║██║     ██╔══██║██║     ██╔══╝
███████║╚██████╗██║  ██║███████╗███████╗
╚══════╝ ╚═════╝╚═╝  ╚═╝╚══════╝╚══════╝
*/

/** @brief Value used to keep the original scale (1.0 in jo_fixed)
 *  @remarks Sprites without scale and rotation are drawn pixel perfect with the VDP1 normal sprite command
 */
# define JO_NO_ZOOM             (65536)

/** @brief Change scale for every sprite displayed after this call
 *  @param scale Sprite scale
 */
static  __jo_force_inline void	jo_sprite_change_sprite_scale(const float scale)
{
    __jo_sprite_attributes.fixed_scale_x = jo_float2fixed(scale);
    __jo_sprite_attributes.fixed_scale_y = __jo_sprite_attributes.fixed_scale_x;
}

/** @brief Change scale for every sprite displayed after this call
 *  @param scale_x Horizontal sprite scale
 *  @param scale_y Vertical sprite scale
 */
static  __jo_force_inline void	jo_sprite_change_sprite_scale_xy(const float scale_x, const float scale_y)
{
    __jo_sprite_attributes.fixed_scale_x = jo_float2fixed(scale_x);
    __jo_sprite_attributes.fixed_scale_y = jo_float2fixed(scale_y);
}

/** @brief Change scale for every sprite displayed after this call using fixed number
 *  @param scale_x Horizontal sprite scale
 *  @param scale_y Vertical sprite scale
 */
static  __jo_force_inline void	jo_sprite_change_sprite_scale_xy_fixed(const jo_fixed scale_x, const jo_fixed scale_y)
{
    __jo_sprite_attributes.fixed_scale_x = scale_x;
    __jo_sprite_attributes.fixed_scale_y = scale_y;
}

/** @brief Restore default scale for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_restore_sprite_scale(void)
{
    __jo_sprite_attributes.fixed_scale_x = JO_NO_ZOOM;
    __jo_sprite_attributes.fixed_scale_y = JO_NO_ZOOM;
}

/*
██████╗ ██████╗  █████╗ ██╗    ██╗
██╔══██╗██╔══██╗██╔══██╗██║    ██║
██║  ██║██████╔╝███████║██║ █╗ ██║
██║  ██║██╔══██╗██╔══██║██║███╗██║
██████╔╝██║  ██║██║  ██║╚███╔███╔╝
╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝ ╚══╝╚══╝
*/

/** @brief Change palette for every 8 bits sprite displayed after this call
 *  @param palette_id Palette id from jo_create_palette()
 */
static  __jo_force_inline void	jo_sprite_set_palette(int palette_id)
{
    __jo_sprite_attributes.color_table_index = JO_MULT_BY_256(palette_id);
}

/** @brief Change palette for every 8 bits sprite displayed after this call: their light is added to the image under them
 *  (see jo_enable_additive_sprites())
 *  @param palette_id Palette id from jo_create_palette()
 */
static  __jo_force_inline void	jo_sprite_set_additive_palette(int palette_id)
{
    // (bits 14 and 13: the priority register 3 of the sprites, see JO_VDP2_ADDITIVE_SPRITE_BANK in jo/vdp2.h)
    __jo_sprite_attributes.color_table_index = JO_MULT_BY_256(palette_id) | 0x6000;
}

/** @brief Display a sprite in 3 dimensional space
 *  @param sprite_id Sprite Id
 *  @param x Horizontal position from the center of the screen
 *  @param y Vertical position from the center of the screen
 *  @param z Z index (depth)
 *  @param angle Angle (Z axe)
 */
static  __jo_force_inline void	jo_sprite_draw3D_and_rotate(const int sprite_id, const int x, const int y, const int z, const int angle)
{
    extern jo_pos3D __jo_sprite_pos;
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
    extern jo_pos3D __jo_sprite_pos;
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
    extern jo_pos3D __jo_sprite_pos;
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
    extern jo_pos3D __jo_sprite_pos;
    __internal_jo_sprite_set_position3D(x, y, z);
    jo_sprite_draw(sprite_id, &__jo_sprite_pos, false, false);
}

void        jo_sprite_draw_4p_fixed(const int sprite_id, const jo_pos2D_fixed * const four_points, const jo_fixed z, const bool centered_style_coordinates);

/** @brief Display a sprite turned on the screen (any angle: the 4 corners are computed, the sprite is distorted by the VDP1)
 *  @param sprite_id Sprite Id returned by jo_sprite_add(), jo_sprite_add_tga() or jo_sprite_add_image_pack()
 *  @param x Horizontal position of the center (pixels from the center of the screen, Q16.16)
 *  @param y Vertical position of the center (pixels from the center of the screen, Q16.16)
 *  @param z Z index (depth, Q16.16)
 *  @param half_width Half of the width on the screen (pixels, Q16.16)
 *  @param half_height Half of the height on the screen (pixels, Q16.16)
 *  @param angle Angle in degree (clockwise on the screen)
 */
void        jo_sprite_draw_rotated_fixed(const int sprite_id, const jo_fixed x, const jo_fixed y, const jo_fixed z,
                                         const jo_fixed half_width, const jo_fixed half_height, const int angle);

/*
 ██████╗██╗     ██╗██████╗ ██████╗ ██╗███╗   ██╗ ██████╗
██╔════╝██║     ██║██╔══██╗██╔══██╗██║████╗  ██║██╔════╝
██║     ██║     ██║██████╔╝██████╔╝██║██╔██╗ ██║██║  ███╗
██║     ██║     ██║██╔═══╝ ██╔═══╝ ██║██║╚██╗██║██║   ██║
╚██████╗███████╗██║██║     ██║     ██║██║ ╚████║╚██████╔╝
 ╚═════╝╚══════╝╚═╝╚═╝     ╚═╝     ╚═╝╚═╝  ╚═══╝ ╚═════╝
*/

/** @brief Activate sprite clipping for every sprite displayed after this call
 * @param outside Clip everything inside clipping area
 */
static  __jo_force_inline void	jo_sprite_enable_clipping(bool outside)
{
    __jo_sprite_attributes.clipping = outside ? JO_VDP1_WINDOW_OUT : JO_VDP1_WINDOW_IN;
}

/** @brief Disable sprite clipping for every sprite displayed after this call
 */
static  __jo_force_inline void	jo_sprite_disable_clipping(void)
{
    __jo_sprite_attributes.clipping = JO_VDP1_NO_WINDOW;
}


/** @brief Set user clipping area
 * @param x Horizontal coord from top left corner
 * @param y Vertical coord from top left corner
 * @param width Width of the clipping rectangle
 * @param height Height of the clipping rectangle
 * @param depth Specifies depth up to which sprites get clipped
 */
void jo_sprite_set_clipping_area(const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height, const int depth);

/** @brief Set the VDP1 drawing area (system clipping)
 *  @param width Width of the drawing area (JO_TV_WIDTH by default)
 *  @param height Height of the drawing area (JO_TV_HEIGHT by default)
 *  @remarks Useful to draw outside of the visible area of the frame buffer (512 pixels width)
 */
void jo_sprite_set_system_clipping_area(const unsigned int width, const unsigned int height);

/*
 ██████╗ ███████╗████████╗    ███████╗██████╗ ██████╗ ██╗████████╗███████╗    ██╗███╗   ██╗███████╗ ██████╗
██╔════╝ ██╔════╝╚══██╔══╝    ██╔════╝██╔══██╗██╔══██╗██║╚══██╔══╝██╔════╝    ██║████╗  ██║██╔════╝██╔═══██╗
██║  ███╗█████╗     ██║       ███████╗██████╔╝██████╔╝██║   ██║   █████╗      ██║██╔██╗ ██║█████╗  ██║   ██║
██║   ██║██╔══╝     ██║       ╚════██║██╔═══╝ ██╔══██╗██║   ██║   ██╔══╝      ██║██║╚██╗██║██╔══╝  ██║   ██║
╚██████╔╝███████╗   ██║       ███████║██║     ██║  ██║██║   ██║   ███████╗    ██║██║ ╚████║██║     ╚██████╔╝
 ╚═════╝ ╚══════╝   ╚═╝       ╚══════╝╚═╝     ╚═╝  ╚═╝╚═╝   ╚═╝   ╚══════╝    ╚═╝╚═╝  ╚═══╝╚═╝      ╚═════╝
*/

/** @brief Get a specific pixel color from image bytes
 *  @param data Bytes (raw image data)
 *  @param x Horizontal ccord in the image
 *  @param y Vertical ccord in the image
 *  @param image_width Image width (in pixels)
 *  @warning This method doesn't work for 8 bits image
 */
static  __jo_force_inline jo_color        jo_sprite_get_pixel_color(const jo_color * const data, const unsigned int x, const unsigned int y, const unsigned int image_width)
{
    return (data[x + y * image_width]);
}

/** @brief Get a specific pixel palette index from image bytes
 *  @param data Bytes (raw image data)
 *  @param x Horizontal ccord in the image
 *  @param y Vertical ccord in the image
 *  @param image_width Image width (in pixels)
 *  @warning This method only works for 8 bits image
 */
static  __jo_force_inline unsigned char     jo_sprite_get_pixel_palette_index(const unsigned char * const data, const unsigned int x, const unsigned int y, const unsigned int image_width)
{
    return (data[x + y * image_width]);
}

/** @brief Get if a specific pixel is transparent (JO_COLOR_Transparent) in image bytes
 *  @param data Bytes (raw image data)
 *  @param x Horizontal ccord in the image
 *  @param y Vertical ccord in the image
 *  @return true if the pixel is transparent otherwise false
 *  @param image_width Image width (in pixels)
 *  @warning This method doesn't work for 8 bits image
 */
static  __jo_force_inline bool        jo_sprite_is_pixel_transparent(const jo_color * const data, const unsigned int x, const unsigned int y, const unsigned int image_width)
{
    return (jo_sprite_get_pixel_color(data, x, y, image_width) == JO_COLOR_Transparent);
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
static  __jo_force_inline int           jo_sprite_get_height(const int sprite_id)
{
    return (__jo_sprite_def[sprite_id].height);
}

/** @brief Get sprite raw image contents
 *  @param sprite_id Sprite Id returned by jo_sprite_add(), jo_sprite_add_tga() or jo_sprite_add_image_pack()
 *  @return Raw image contents
 */
static  __jo_force_inline void          *jo_sprite_get_raw_data(const int sprite_id)
{
    return (__jo_sprite_pic[sprite_id].data);
}

/*
███████╗████████╗ █████╗ ████████╗██╗███████╗████████╗██╗ ██████╗███████╗
██╔════╝╚══██╔══╝██╔══██╗╚══██╔══╝██║██╔════╝╚══██╔══╝██║██╔════╝██╔════╝
███████╗   ██║   ███████║   ██║   ██║███████╗   ██║   ██║██║     ███████╗
╚════██║   ██║   ██╔══██║   ██║   ██║╚════██║   ██║   ██║██║     ╚════██║
███████║   ██║   ██║  ██║   ██║   ██║███████║   ██║   ██║╚██████╗███████║
╚══════╝   ╚═╝   ╚═╝  ╚═╝   ╚═╝   ╚═╝╚══════╝   ╚═╝   ╚═╝ ╚═════╝╚══════╝
*/

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

/** @brief Get the free sprite memory (VDP1 VRAM)
 *  @return Free bytes for the next sprites
 */
unsigned int                          jo_sprite_free_memory(void);

#endif /* !__JO_SPRITES_H__ */

/*
** END OF FILE
*/
