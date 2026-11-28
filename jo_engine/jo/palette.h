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
/** @file palette.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Palette Handling
 *  @bug No known bugs.
 */

#ifndef __JO_PALETTE_H__
# define __JO_PALETTE_H__

/** @brief Define the maximum of color entry available on each palette
 *  @warning MC Hammer: don't touch this
*/
# define JO_PALETTE_MAX_COLORS              (255)

/* (JO_PALETTE_MAX, the maximum of palettes: see jo/sega_saturn.h, the functions of the sprites and of the 3D check the
   ids of the palettes with it) */

/** @brief Create a new palette with 255 entries maximum
 *  @param output The new palette: output->data[k] is the color of the pixels of index k + 1 (the index 0 is transparent;
 *  the TGA files: their index + 1)
 *  @details See also jo_sprite_set_palette()
 */
void                jo_create_palette(jo_palette * const output);

/** @brief Write the shades of colors in a palette for the palette lighting (see jo_3d_set_palette_light()): the shade s
 *  (0 = the darkest, levels - 1 = the brightest) of the color i is the index first + s * count + i
 *  @param palette Palette (jo_create_palette())
 *  @param first Index of the darkest shade of the first color (1 to 255; a multiple of 16 for the textures of 16 colors)
 *  @param colors Colors (the color itself at the shade levels / 2, from 35 % of it at the shade 0 to 125 % at the last one)
 *  @param count Colors count (16 for a texture of 16 colors: its color bank is the shade 0)
 *  @param levels Shades of each color (2 to 32)
 *  @remarks The polygon of the color i: jo_3d_set_mesh_polygon_color(mesh, JO_PALETTE_LIGHT_COLOR(palette->id, first, i), index)
 */
void                jo_palette_set_light_shades(jo_palette * const palette, const int first, const jo_color * const colors,
                                                const int count, const int levels);

/** @brief Color code of the color i of the shades written by jo_palette_set_light_shades() (its darkest shade) */
# define JO_PALETTE_LIGHT_COLOR(PALETTE_ID, FIRST, I)   ((jo_color)((PALETTE_ID) * 256 + (FIRST) + (I)))

/** @brief Color code of a color of a palette: a polygon of this color (jo_3d_set_mesh_polygon_color()) has the color of
 *  the palette, in all the modes
 *  @param palette Palette (jo_create_palette())
 *  @param index Index of the color in the palette (1 to 255: palette->data[index - 1])
 *  @return jo_color
 */
static  __jo_force_inline jo_color  jo_palette_color(const jo_palette * const palette, const int index)
{
#ifdef JO_DEBUG
    // (-1: the palette of jo_create_palette() with the color RAM full)
    if (__JO_INDEX_IS_WRONG(palette->id, JO_PALETTE_MAX, "palette id") || __JO_INDEX_IS_WRONG(index, JO_PALETTE_MAX_COLORS + 1, "color index"))
        return (0);
#endif
    return ((jo_color)(palette->id * 256 + index));
}

/** @brief Change colors of a palette with the image of the frame: in a callback of jo_core_run(), the colors are written
 *  in the color RAM at the VBLANK where the VDP1 displays the sprites of this frame (1 or 2 frames later), at once out of
 *  jo_core_run()
 *  @param palette Palette (jo_create_palette())
 *  @param index Index of the first color (1 to 255: palette->data[index - 1])
 *  @param colors Colors
 *  @param count Colors count
 *  @remarks For the palette animations (water, fire, flashes, fades of a palette): the sprites and the scroll screens change
 *  their colors in the same field. palette->data[k] = color writes at once (before the image of the frame)
 *  @remarks Until then palette->data and jo_get_palette_register() give the old colors
 */
void                jo_palette_set_colors(const jo_palette * const palette, const int index, const jo_color * const colors,
                                          const int count);

/** @brief Copies the image palette data to CRAM
 *  @param data Data source
 *  @param cram_addr Color RAM address
 *  @param size Data size
 */
static  __jo_force_inline void    jo_palette_to_cram(unsigned short * restrict data, unsigned short * restrict cram_addr, unsigned int size)
{
    while (size-- > 0) *(cram_addr++) = *(data++);
}

/** @brief Create a new palette from existing data
 *  @param output The new palette
 *  @param data Data source
 *  @param size Data size
 *  @details See also jo_sprite_set_palette()
 */
static  __jo_force_inline void        jo_create_palette_from(jo_palette * const output, unsigned short *data, unsigned int size)
{
    jo_create_palette(output);
    // (the color RAM full: written at the address 0)
    if (output->data != JO_NULL)
        jo_palette_to_cram(data, output->data, size);
}

/** @brief Write the colors of a sub-palette of 16 colors of a palette, for the 16 colors sprites in color bank mode
 *  (jo_sprite_add_4bits_image(), jo_sprite_set_palette_4bits())
 *  @param palette Palette (jo_create_palette())
 *  @param subpalette_id Sub-palette (0 to JO_PALETTE_SUBPALETTES - 1)
 *  @param colors JO_PALETTE_SUBPALETTE_COLORS colors: the color k of the pixels k (the color 0 is not used: the pixels 0 are
 *  transparent)
 *  @remarks The color k of the sub-palette s is palette->data[16 x s + k - 1] (palette->data[k] is the color of the index
 *  k + 1). Written with the image of the frame (see jo_palette_set_colors()): a palette animation (the water, the fire, a
 *  flash) changes with the sprites of the frame
 */
static  __jo_force_inline void        jo_palette_set_subpalette(const jo_palette * const palette, const int subpalette_id,
                                                                const jo_color * const colors)
{
#ifdef JO_DEBUG
    if (palette == JO_NULL || colors == JO_NULL)
    {
        jo_core_error("palette or colors is null");
        return ;
    }
    if (__JO_INDEX_IS_WRONG(subpalette_id, JO_PALETTE_SUBPALETTES, "sub-palette id"))
        return ;
#endif
    jo_palette_set_colors(palette, JO_MULT_BY_16(subpalette_id) + 1, colors + 1, JO_PALETTE_SUBPALETTE_COLORS - 1);
}

/** @brief Set palette index color
 *  @param palette_index palette Index
 *  @param color Color
 */
void                jo_set_palette_register(const unsigned short palette_index, const jo_color color);

/** @brief Get palette color from index
 *  @param palette_index palette Index
 *  @return color Color
 */
jo_color            jo_get_palette_register(const unsigned short palette_index);

#ifdef JO_COMPILE_WITH_PRINTF_SUPPORT

/** @brief Set foreground palette color for the specific index
  * @param index Printf color index (0 to 7, see jo_set_printf_color_index())
  * @param color Color (Example: JO_COLOR_Red)
  * @remarks The printf colors are in the first 256 colors of the color RAM (reserved), so they don't conflict with the palettes created by jo_create_palette()
  */
static  __jo_force_inline void        jo_set_printf_palette_color(const unsigned short index, const unsigned short color)
{
    // Color 1 of the 16 colors palette "index"
    jo_set_palette_register(1 + JO_MULT_BY_16(index), color);
}

#endif

#endif /* !__JO_PALETTE_H__ */

/*
** END OF FILE
*/
