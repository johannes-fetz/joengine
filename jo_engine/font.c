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
#include "jo/font.h"
#include "jo/image.h"
#include "jo/tga.h"
#include "jo/malloc.h"
#include "jo/math.h"
#include "jo/sprites.h"

jo_font             *jo_font_load(const char * const sub_dir, const char * const filename, const jo_color transparent_color, const int letter_width, const int letter_height, const int letter_spacing, const char * const mapping)
{
    int             char_count;
    int             first_sprite_id;
    jo_tile         *font_tileset;
    jo_font         *font;
    int             x;
    int             y;
    int             i;

#ifdef JO_DEBUG
    if (filename == JO_NULL)
    {
        jo_core_error("filename is null");
        return (JO_NULL);
    }
    if (mapping == JO_NULL)
    {
        jo_core_error("mapping is null");
        return (JO_NULL);
    }
#endif
    font = (jo_font *)jo_malloc_with_behaviour(sizeof(*font), JO_MALLOC_TRY_REUSE_BLOCK);
    if (font == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory #1");
#endif
        return (JO_NULL);
    }
    char_count = jo_strlen(mapping);
    font_tileset = (jo_tile *)jo_malloc_with_behaviour(sizeof(*font_tileset) * char_count, JO_MALLOC_TRY_REUSE_BLOCK);
    if (font_tileset == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory #2");
#endif
        jo_free(font);
        return (JO_NULL);
    }
    font->spacing = letter_width + letter_spacing;
    font->start_ascii = 127;
    JO_ZERO(font->end_ascii);
    font->z_index = 100;
    JO_ZERO(i);
    for (JO_ZERO(y); i < char_count; y += letter_height)
    {
        for (JO_ZERO(x); i < char_count; ++i, x += letter_width)
        {
            if (mapping[i] == '\n')
            {
                --char_count;
                break;
            }
            if (mapping[i] < font->start_ascii)
                font->start_ascii = mapping[i];
            if (mapping[i] > font->end_ascii)
                font->end_ascii = mapping[i];
            font_tileset[i].width = letter_width;
            font_tileset[i].height = letter_height;
            font_tileset[i].y = y;
            font_tileset[i].x = x;
        }
    }
#ifdef JO_COMPILE_WITH_TGA_SUPPORT
    if (jo_endwith(filename, ".BIN"))
#endif
        first_sprite_id = jo_sprite_add_bin_tileset(sub_dir, filename, transparent_color, font_tileset, char_count);
#ifdef JO_COMPILE_WITH_TGA_SUPPORT
    else
        first_sprite_id = jo_sprite_add_tga_tileset(sub_dir, filename, transparent_color, font_tileset, char_count);
#endif
    font->lookup_table = (char *)jo_malloc_with_behaviour(sizeof(*font->lookup_table) * (font->end_ascii - font->start_ascii + 1), JO_MALLOC_TRY_REUSE_BLOCK);
    if (font->lookup_table == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory #3");
#endif
        jo_free(font);
        jo_free(font_tileset);
        return (JO_NULL);
    }
    for (JO_ZERO(i); i < char_count; ++i)
        font->lookup_table[i] = first_sprite_id;
    for (JO_ZERO(i); i < char_count; ++i)
        font->lookup_table[(mapping[i] -  font->start_ascii)] = first_sprite_id + i;
    jo_free(font_tileset);
    return (font);
}

void            jo_font_print(const jo_font * const font, int x, int y, float scale, char *str)
{
    int         spacing;

    if (font == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("font is null");
#endif
        return ;
    }
    if (str == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("str is null");
#endif
        return ;
    }
    jo_sprite_change_sprite_scale(scale);
    spacing = font->spacing * scale;
    while (*str)
    {
        if (*str == '\t')
            x += JO_MULT_BY_4(spacing);
        else if (*str != ' ' && *str >= font->start_ascii && *str <= font->end_ascii)
            jo_sprite_draw3D2(font->lookup_table[*str - font->start_ascii], x, y, font->z_index);
        ++str;
        x += spacing;
    }
    jo_sprite_restore_sprite_scale();
}

void            jo_font_print_centered(const jo_font * const font, int x, int y, float scale, char *str)
{
    int         spacing;
    int         len;

    if (font == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("font is null");
#endif
        return ;
    }
    if (str == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("str is null");
#endif
        return ;
    }
    len = jo_strlen(str);
    jo_sprite_change_sprite_scale(scale);
    spacing = font->spacing * scale;
    x -= JO_DIV_BY_2((int)((len - 1) * font->spacing * scale));
    while (*str)
    {
        if (*str == '\t')
            x += JO_MULT_BY_4(spacing);
        else if (*str != ' ' && *str >= font->start_ascii && *str <= font->end_ascii)
            jo_sprite_draw3D(font->lookup_table[*str - font->start_ascii], x, y, font->z_index);
        ++str;
        x += spacing;
    }
    jo_sprite_restore_sprite_scale();
}

/*
** END OF FILE
*/
