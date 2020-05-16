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
/** @file font.h
 *  @author Johannes Fetz
 *  @bug Some glitch when changing scale in jo_font_printf_centered()
 *  @brief Jo Engine Simple ASCII Font & print support
 */

#ifndef __JO_FONT_H__
# define __JO_FONT_H__

typedef struct
{
    char        start_ascii;
    char        end_ascii;
    char        *lookup_table;
    char        spacing;
    int         z_index;
}               jo_font;

/** @brief Print arguments using a specific font (using top-left screen coordinates)
 *  @param F The font (returned by jo_font_load())
 *  @param X Horizontal location from the left border of the screen
 *  @param Y Vertical location from the top border of the screen
 *  @param S Font scale
 */
# define jo_font_printf(F, X, Y, S, ...)		        do {sprintf(__jo_sprintf_buf, __VA_ARGS__); jo_font_print((F), (X), (Y), (S), __jo_sprintf_buf);} while(0)

/** @brief Print arguments using a specific font (using screen center coordinates)
 *  @param F The font (returned by jo_font_load())
 *  @param X Horizontal location from the center of the screen
 *  @param Y Vertical location from the center of the screen
 *  @param S Font scale
 */
# define jo_font_printf_centered(F, X, Y, S, ...)		do {sprintf(__jo_sprintf_buf, __VA_ARGS__); jo_font_print_centered((F), (X), (Y), (S), __jo_sprintf_buf);} while(0)

#ifdef JO_COMPILE_WITH_FS_SUPPORT

/** @brief Load a font
 *  @param sub_dir Sub directory name (use JO_ROOT_DIR if the file is on the root directory)
 *  @param filename Filename (upper case and shorter as possible like "FONT.TGA")
 *  @param transparent_color Transparent color (see colors.h). Use JO_COLOR_Transparent by default
 *  @param letter_width Letter width (must be a multiple of 8)
 *  @param letter_height Letter height
 *  @param letter_spacing Letter spacing
 *  @param mapping Image pattern : "0123456789ABCDEFGH..." (use \n to go to the next image line)
 *  @return Font object
 */
jo_font         *jo_font_load(const char * const sub_dir, const char * const filename, const jo_color transparent_color, const int letter_width, const int letter_height, const int letter_spacing, const char * const mapping);

#endif

/** @brief Print a string using a specific font (using top-left screen coordinates)
 *  @param font The font (returned by jo_font_load())
 *  @param x Horizontal location from the left border of the screen
 *  @param y Vertical location from the top border of the screen
 *  @param scale Font scale
 *  @param str The string
 */
void            jo_font_print(const jo_font * const font, int x, int y, float scale, char * str);

/** @brief Print a string using a specific font (using screen center coordinates)
 *  @param font The font (returned by jo_font_load())
 *  @param x Horizontal location from the center of the screen
 *  @param y Vertical location from the center of the screen
 *  @param scale Font scale
 *  @param str The string
 */
void            jo_font_print_centered(const jo_font * const font, int x, int y, float scale, char * str);

#endif /* !__JO_FONT_H__ */

/*
** END OF FILE
*/
