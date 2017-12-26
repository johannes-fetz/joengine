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
#include "jo/malloc.h"
#include "jo/fs.h"
#include "jo/image.h"
#include "jo/tga.h"
#include "jo/sprites.h"
#include "jo/colors.h"
#include "jo/math.h"

#ifdef JO_COMPILE_WITH_TGA_SUPPORT

/*
** INTERNAL MACROS
*/
#define         JO_TGA_24_BITS                              (24)
#define         JO_TGA_32_BITS                              (32)
#define			JO_TGA_SUPPORTED_FORMAT(M)			        ((M) == JO_TGA_24_BITS || (M) == JO_TGA_32_BITS)

#define			JO_TGA_CONVERT_COLOR(TGA, IDX)		        (JO_DIV_BY_8((*((unsigned char *)(TGA + IDX)))) & 0x1f)

#define         JO_TGA_RGB(R, G, B)		                    (0x8000 | ((B)<<10) | ((G)<<5) | (R))

#define			JO_TGA_24BITS_GET_PIXEL(TGA, X, Y, WIDTH)	JO_TGA_RGB(JO_TGA_CONVERT_COLOR(TGA, (((X) * 3) + 2) + ((Y) * (WIDTH * 3))), \
                                                                JO_TGA_CONVERT_COLOR(TGA, (((X) * 3) + 1) + ((Y) * (WIDTH * 3))), \
                                                                JO_TGA_CONVERT_COLOR(TGA, (((X) * 3)) + ((Y) * (WIDTH * 3))))

#define			JO_TGA_32BITS_GET_PIXEL(TGA, X, Y, WIDTH)	JO_TGA_RGB(JO_TGA_CONVERT_COLOR(TGA, (JO_MULT_BY_4(X) + 2) + ((Y) * JO_MULT_BY_4(WIDTH))), \
                                                                JO_TGA_CONVERT_COLOR(TGA, (JO_MULT_BY_4(X) + 1) + ((Y) * JO_MULT_BY_4(WIDTH))), \
                                                                JO_TGA_CONVERT_COLOR(TGA, (JO_MULT_BY_4(X) + 0) + ((Y) * JO_MULT_BY_4(WIDTH))))

extern int      __jo_hash_table[JO_MAX_SPRITE];

static  __jo_force_inline jo_color         jo_tga_get_pixel(const char * const stream, const int x, const int y, const int width, const int bits)
{
    switch (bits)
    {
    case JO_TGA_32_BITS:
        if (JO_TGA_CONVERT_COLOR(stream, (JO_MULT_BY_4(x) + 3) + ((y) * JO_MULT_BY_4(width))) <= 0)
            return (JO_COLOR_Transparent);
        return (JO_TGA_32BITS_GET_PIXEL(stream, x, y, width));
    default:
        return (JO_TGA_24BITS_GET_PIXEL(stream, x, y, width));
    }
}

t_tga_error_code	__jo_tga_load(jo_img *img, const char * const sub_dir, const char * const filename, char **stream, int *bits)
{
    if (*stream == JO_NULL)
        *stream = jo_fs_read_file_in_dir(filename, sub_dir, JO_NULL);
    if (*stream == JO_NULL)
        return (JO_TGA_UNSUPPORTED_FORMAT);
    img->width = jo_swap_endian_ushort( *((unsigned short *)(*stream + 12)));
    img->height = jo_swap_endian_ushort( *((unsigned short *)(*stream + 14)));
    *bits = (int)(*stream)[16];
    if (!JO_TGA_SUPPORTED_FORMAT(*bits))
    {
#ifdef JO_DEBUG
        jo_core_error("%s: Unsupported %d bits TGA", filename, *bits);
#endif
        if (filename != JO_NULL)
            jo_free(*stream);
        return (JO_TGA_UNSUPPORTED_FORMAT);
    }
    if (img->data == JO_NULL)
        img->data = (unsigned short *)jo_malloc_with_behaviour(img->height * img->width * sizeof(*img->data), JO_MALLOC_TRY_REUSE_BLOCK);
    if (img->data == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("%s: Out of memory", filename);
#endif
        if (filename != JO_NULL)
            jo_free(*stream);
        return (JO_TGA_OUT_OF_MEMORY);
    }
    return (JO_TGA_OK);
}

static void             jo_tga_read_contents(jo_img *img, char * stream, const jo_color transparent_color, const int bits)
{
    register int		idx;
    register int		x;
    register int		y;

    stream += 18; /* Jump header */
    for (JO_ZERO(y); y < img->height; ++y)
    {
        for (JO_ZERO(x); x < img->width; ++x)
        {
            idx = x + (img->height - y - 1) * img->width;
            img->data[idx] = jo_tga_get_pixel(stream, x, y, img->width, bits);
            if (transparent_color != JO_COLOR_Transparent && img->data[idx] == transparent_color)
                img->data[idx] = JO_COLOR_Transparent;
        }
    }
}

t_tga_error_code		jo_tga_loader_from_stream(jo_img *img, char *stream, const jo_color transparent_color)
{
    t_tga_error_code	code;
    int                 bits;

    if ((code = __jo_tga_load(img, JO_NULL, JO_NULL, &stream, &bits)) != JO_TGA_OK)
        return (code);
    jo_tga_read_contents(img, stream, transparent_color, bits);
    return (code);
}

t_tga_error_code		    jo_tga_loader(jo_img *img, const char * const sub_dir, const char * const filename, const jo_color transparent_color)
{
    char                    *stream;
    t_tga_error_code	    code;
    int                     bits;

    stream = JO_NULL;
    if ((code = __jo_tga_load(img, sub_dir, filename, &stream, &bits)) != JO_TGA_OK)
        return (code);
    jo_tga_read_contents(img, stream, transparent_color, bits);
    jo_free(stream);
    return (code);
}

int		                    jo_sprite_add_tga_tileset(const char * const sub_dir, const char * const filename, const jo_color transparent_color, const jo_tile * const tileset, const unsigned int tile_count)
{
    char                    *stream;
    char                    *stream_begin;
    jo_img                  full_image;
    jo_img                  tile_image;
    register int		    x;
    register int		    y;
    register int		    idx;
    register unsigned int   i;
    int						first_id;
    int                     bits;

    stream = JO_NULL;
    full_image.data = (unsigned short *)-1;/* Disable allocation in __jo_tga_load() */
    if (__jo_tga_load(&full_image, sub_dir, filename, &stream, &bits) != JO_TGA_OK)
        return (-1);
    stream_begin = stream;
    stream += 18; /* Jump header */
    first_id = -1;
    for (JO_ZERO(i); i < tile_count; ++i)
    {
#ifdef JO_DEBUG
        if ((tileset[i].width % 8) != 0)
        {
            jo_core_error("%s: Tile width must be multiple of 8", filename);
            jo_free(stream_begin);
            return (-1);
        }
#endif
        tile_image.width = tileset[i].width;
        tile_image.height = tileset[i].height;
        tile_image.data = (unsigned short *)jo_malloc(tile_image.height * tile_image.width * sizeof(*tile_image.data));
        if (tile_image.data == JO_NULL)
        {
#ifdef JO_DEBUG
            jo_core_error("%s: Out of memory", filename);
#endif
            jo_free(stream_begin);
            return (-1);
        }
        for (JO_ZERO(y); y < tile_image.height; ++y)
        {
            for (JO_ZERO(x); x < tile_image.width; ++x)
            {
                idx = x + y * tile_image.width;
                tile_image.data[idx] = jo_tga_get_pixel(stream, x + tileset[i].x, (full_image.height - (y + tileset[i].y) - 1), full_image.width, bits);
                if (transparent_color != JO_COLOR_Transparent && tile_image.data[idx] == transparent_color)
                    tile_image.data[idx] = JO_COLOR_Transparent;
            }
        }
        x = jo_sprite_add(&tile_image);
        jo_free_img(&tile_image);
        if (first_id < 0)
            first_id = x;
    }
    jo_free(stream_begin);
    return (first_id);
}

int						jo_sprite_add_tga_from_stream(char *stream, const jo_color transparent_color)
{
    jo_img				img;
    int					id;

#ifdef JO_DEBUG
    if (stream == JO_NULL)
    {
        jo_core_error("stream is null");
        return (-1);
    }
#endif
    img.data = JO_NULL;
    if (jo_tga_loader_from_stream(&img, stream, transparent_color) != JO_TGA_OK)
        return (-1);
#ifdef JO_DEBUG
    if ((img.width % 8) != 0)
    {
        jo_core_error("Image width must be a multiple of 8");
        jo_free(img.data);
        return (-1);
    }
#endif
    id = jo_sprite_add(&img);
    jo_free_img(&img);
    return (id);
}

int						jo_sprite_add_tga(const char * const sub_dir, const char * const filename, const jo_color transparent_color)
{
    jo_img				img;
    int					id;

#ifdef JO_DEBUG
    if (filename == JO_NULL)
    {
        jo_core_error("filename is null");
        return (-1);
    }
#endif
    img.data = JO_NULL;
    if (jo_tga_loader(&img, sub_dir, filename, transparent_color) != JO_TGA_OK)
        return (-1);
#ifdef JO_DEBUG
    if ((img.width % 8) != 0)
    {
        jo_core_error("%s: Image width must be a multiple of 8", filename);
        jo_free(img.data);
        return (-1);
    }
#endif
    id = jo_sprite_add(&img);
    jo_free_img(&img);
    if (id >= 0)
        __jo_hash_table[id] = jo_4_char_hash(filename);
    return (id);
}

#endif /* !JO_COMPILE_WITH_TGA_SUPPORT */

/*
** END OF FILE
*/
