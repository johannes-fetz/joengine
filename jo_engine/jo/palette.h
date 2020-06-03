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

/** @brief Define the maximum of palette
 *  @warning MC Hammer: don't touch this
*/
# define JO_PALETTE_MAX                     (8)

/** @brief Create a new palette with 255 entries maximum
 *  @param output The new palette
 *  @details See also jo_sprite_set_palette_id()
 */
void                jo_create_palette(jo_palette * const output);

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
 *  @details See also jo_sprite_set_palette_id()
 */
static  __jo_force_inline void        jo_create_palette_from(jo_palette * const output, unsigned short *data, unsigned int size)
{
    jo_create_palette(output);
    jo_palette_to_cram(data, output->data, size);
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
  * @param index Palette index (0 and 1 are reserved)
  * @param color Color (Example: JO_COLOR_Red)
  */
static  __jo_force_inline void        jo_set_printf_palette_color(const unsigned short index, const unsigned short color)
{
    jo_set_palette_register(1 + JO_MULT_BY_256(index), color);
}

#endif

#endif /* !__JO_PALETTE_H__ */

/*
** END OF FILE
*/
