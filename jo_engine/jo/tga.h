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
/** @file tga.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Truevision Targa format support
 *  @bug jo_tga_tileset_loader is not tested
 */

#ifndef __JO_TGA_H__
# define __JO_TGA_H__

#ifdef JO_COMPILE_WITH_TGA_SUPPORT

/** @brief tga error code */
typedef enum
{
    JO_TGA_OK = 0,
    JO_TGA_FILE_NOT_FOUND = 1,
    JO_TGA_UNSUPPORTED_FORMAT = 2,
    JO_TGA_OUT_OF_MEMORY = 3
}					t_tga_error_code;

/** @brief Load a TGA image
 *  @param img Image (set data to NULL for dynamic allocation)
 *  @param sub_dir Sub directory name (use JO_ROOT_DIR if the file is on the root directory)
 *  @param filename Filename (upper case and shorter as possible like "A.TGA")
 *  @param transparent_color Transparent color (see colors.h). Use JO_COLOR_Transparent by default
 *  @return t_tga_error_code
 */
t_tga_error_code		jo_tga_loader(jo_img *img, const char * const sub_dir, const char *const filename, const jo_color transparent_color);

/** @brief Load a TGA image from stream
 *  @param img Image (set data to NULL for dynamic allocation)
 *  @param stream Raw TGA contents
 *  @param transparent_color Transparent color (see colors.h). Use JO_COLOR_Transparent by default
 *  @return t_tga_error_code
 */
t_tga_error_code		jo_tga_loader_from_stream(jo_img *img, char *stream, const jo_color transparent_color);

/** @brief Add a sprite from a TGA file
 *  @param sub_dir Sub directory name (use JO_ROOT_DIR if the file is on the root directory)
 *  @param filename Filename (upper case and shorter as possible like "A.TGA")
 *  @param transparent_color Transparent color (see colors.h). Use JO_COLOR_Transparent by default
 *  @return Sprite Id or -1 if failed
 */
int		jo_sprite_add_tga(const char * const sub_dir, const char * const filename, const jo_color transparent_color);

/** @brief Add a sprite from a TGA stream
 *  @param stream Raw TGA contents
 *  @param transparent_color Transparent color (see colors.h). Use JO_COLOR_Transparent by default
 *  @return Sprite Id or -1 if failed
 */
int     jo_sprite_add_tga_from_stream(char *stream, const jo_color transparent_color);

/** @brief Add tileset sprites from a TGA file
 *  @param sub_dir Sub directory name (use JO_ROOT_DIR if the file is on the root directory)
 *  @param filename Filename (upper case and shorter as possible like "A.TGA")
 *  @param transparent_color Transparent color (see colors.h). Use JO_COLOR_Transparent by default
 *  @param tileset Tileset definition
 *  @param tile_count Number of tile in the entire image
 *  @return Sprite Id of the first image or -1 if failed
 */
int		jo_sprite_add_tga_tileset(const char * const sub_dir, const char * const filename, const jo_color transparent_color, const jo_tile * const tileset, const unsigned int tile_count);

#endif /* !JO_COMPILE_WITH_TGA_SUPPORT */

#endif /* !__JO_TGA_H__ */

/*
** END OF FILE
*/
