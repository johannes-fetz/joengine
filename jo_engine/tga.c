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
#include "jo/palette.h"
#include "jo/image.h"
#include "jo/tga.h"
#include "jo/colors.h"
#include "jo/sprites.h"
#include "jo/colors.h"
#include "jo/math.h"
#include "jo/vdp2.h"

#ifdef JO_COMPILE_WITH_TGA_SUPPORT

/*
** INTERNAL MACROS
*/
#define TGA_IMAGEDESCRIPTOR_ROW_TOP                 (32)

#define JO_TGA_8_BITS                               (8)
#define JO_TGA_24_BITS                              (24)
#define JO_TGA_32_BITS                              (32)
#define JO_TGA_SUPPORTED_FORMAT(M)			        ((M) == JO_TGA_8_BITS || (M) == JO_TGA_24_BITS || (M) == JO_TGA_32_BITS)

#define JO_TGA_CONVERT_COLOR(TGA, IDX)		        JO_COLOR_SATURN(*((unsigned char *)(TGA + IDX)))

#define JO_TGA_24BITS_GET_PIXEL(TGA, X, Y, WIDTH)	JO_COLOR_SATURN_RGB(JO_TGA_CONVERT_COLOR(TGA, (((X) * 3) + 2) + ((Y) * (WIDTH * 3))), \
                                                                JO_TGA_CONVERT_COLOR(TGA, (((X) * 3) + 1) + ((Y) * (WIDTH * 3))), \
                                                                JO_TGA_CONVERT_COLOR(TGA, (((X) * 3)) + ((Y) * (WIDTH * 3))))

#define JO_TGA_32BITS_GET_PIXEL(TGA, X, Y, WIDTH)	JO_COLOR_SATURN_RGB(JO_TGA_CONVERT_COLOR(TGA, (JO_MULT_BY_4(X) + 2) + ((Y) * JO_MULT_BY_4(WIDTH))), \
                                                                JO_TGA_CONVERT_COLOR(TGA, (JO_MULT_BY_4(X) + 1) + ((Y) * JO_MULT_BY_4(WIDTH))), \
                                                                JO_TGA_CONVERT_COLOR(TGA, (JO_MULT_BY_4(X) + 0) + ((Y) * JO_MULT_BY_4(WIDTH))))

#ifdef JO_COMPILE_WITH_SPRITE_HASHTABLE
extern int      __jo_hash_table[JO_MAX_SPRITE];
#endif

static jo_tga_palette_handling_callback __jo_tga_palette_handling = JO_NULL;

typedef struct
{
    unsigned char   id_length;
    unsigned char   color_map_type;
    unsigned char   image_type;
    short           color_map_origin;
    short           color_map_length;
    unsigned char   color_map_entry_depth;
    short           origin_x;
    short           origin_y;
    short           width;
    short           height;
    unsigned char   image_pixel_size;
    unsigned char   image_descriptor;

} __attribute__((packed)) __jo_tga_header;

void                                        jo_set_tga_palette_handling(jo_tga_palette_handling_callback callback)
{
    __jo_tga_palette_handling = callback;
}

static  __jo_force_inline jo_color          jo_tga_get_pixel(const char * const restrict stream, const int x, const int y, const int width, const int bits)
{
    switch (bits)
    {
    case JO_TGA_8_BITS:
        return (jo_color)(*(stream + (x + (y * width))) + 1);
    case JO_TGA_32_BITS:
        if (JO_TGA_CONVERT_COLOR(stream, (JO_MULT_BY_4(x) + 3) + ((y) * JO_MULT_BY_4(width))) <= 0)
            return (JO_COLOR_Transparent);
        return (JO_TGA_32BITS_GET_PIXEL(stream, x, y, width));
    default:
        return (JO_TGA_24BITS_GET_PIXEL(stream, x, y, width));
    }
}

t_tga_error_code                            __jo_tga_load(jo_raw_img *img, const char * const sub_dir, const char * const filename, char **restrict stream, int *bits)
{
    __jo_tga_header                         *header;

#ifdef JO_COMPILE_WITH_FS_SUPPORT
    if (*stream == JO_NULL)
        *stream = jo_fs_read_file_in_dir(filename, sub_dir, JO_NULL);
#else
    JO_UNUSED_ARG(sub_dir);
    if (*stream == JO_NULL)
        return (JO_TGA_FILE_NOT_FOUND);
#endif /* !JO_COMPILE_WITH_FS_SUPPORT */
    if (*stream == JO_NULL)
        return (JO_TGA_UNSUPPORTED_FORMAT);
    header = (__jo_tga_header *)*stream;
    img->width = jo_swap_endian_short(header->width);
    img->height = jo_swap_endian_short(header->height);
    *bits = header->image_pixel_size;
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
        img->data = jo_malloc_with_behaviour(img->height * img->width * (*bits == JO_TGA_8_BITS ? sizeof(unsigned char) : sizeof(jo_color)), JO_MALLOC_TRY_REUSE_BLOCK);
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

static void                             __jo_tga_read_contents(jo_raw_img *img, char * restrict stream, const jo_color transparent_color, const int bits)
{
    register int		                idx;
    register int		                x;
    register int		                y;
    register int                        delta;
    register bool                       row_top;
    __jo_tga_header                     *header;

    header = (__jo_tga_header *)stream;
    row_top = (header->image_descriptor & TGA_IMAGEDESCRIPTOR_ROW_TOP) == TGA_IMAGEDESCRIPTOR_ROW_TOP;
    stream += sizeof(*header); /* Jump header */
    if (bits == JO_TGA_8_BITS)
    {
        jo_palette                      *palette;
        x = jo_swap_endian_short(header->color_map_length);
        if (__jo_tga_palette_handling != JO_NULL && (palette = __jo_tga_palette_handling()) != JO_NULL)
        {
            for (idx = 0; idx < x && idx < JO_PALETTE_MAX_COLORS; ++idx)
            {
                jo_color c = JO_COLOR_SATURN_RGB(JO_TGA_CONVERT_COLOR(stream, 2), JO_TGA_CONVERT_COLOR(stream, 1), JO_TGA_CONVERT_COLOR(stream, 0));
                palette->data[idx] = c;
                stream += 3;
            }
            stream += 3;
        }
        else
            stream += x * JO_DIV_BY_8(header->color_map_entry_depth);
    }
    for (JO_ZERO(y); y < img->height; ++y)
    {
        for (JO_ZERO(x); x < img->width; ++x)
        {
            delta = row_top ? y : (img->height - 1 - y);
            idx = x + delta * img->width;
            if (bits == JO_TGA_8_BITS)
            {
                ((jo_img_8bits *)img)->data[idx] = jo_tga_get_pixel(stream, x, y, img->width, bits);
                if (transparent_color != JO_COLOR_Transparent && ((jo_img_8bits *)img)->data[idx] == (unsigned char)transparent_color)
                    JO_ZERO(((jo_img_8bits *)img)->data[idx]);
            }
            else
            {
                ((jo_img *)img)->data[idx] = jo_tga_get_pixel(stream, x, y, img->width, bits);
                if (transparent_color != JO_COLOR_Transparent && ((jo_img *)img)->data[idx] == transparent_color)
                    ((jo_img *)img)->data[idx] = JO_COLOR_Transparent;
            }
        }
    }
}

t_tga_error_code		__jo_tga_any_loader_from_stream(jo_raw_img *img, char *stream, const jo_color transparent_color, int *bits)
{
    t_tga_error_code	code;

    if ((code = __jo_tga_load((jo_raw_img *)img, JO_NULL, JO_NULL, &stream, bits)) != JO_TGA_OK)
        return (code);
    __jo_tga_read_contents((jo_raw_img *)img, stream, transparent_color, *bits);
    return (code);
}

t_tga_error_code		jo_tga_8bits_loader_from_stream(jo_img_8bits *img, char *stream, const int transparent_color_index_in_palette)
{
    int                 bits;

    return (__jo_tga_any_loader_from_stream((jo_raw_img *)img, stream, transparent_color_index_in_palette, &bits));
}

t_tga_error_code		jo_tga_loader_from_stream(jo_img *img, char *stream, const jo_color transparent_color)
{
    int                 bits;

    return (__jo_tga_any_loader_from_stream((jo_raw_img *)img, stream, transparent_color, &bits));
}

#ifdef JO_COMPILE_WITH_FS_SUPPORT

t_tga_error_code		    __jo_tga_any_loader(jo_raw_img *img, const char * const sub_dir, const char * const filename, const jo_color transparent_color, int *bits)
{
    char                    *stream;
    t_tga_error_code	    code;

    stream = JO_NULL;
    if ((code = __jo_tga_load(img, sub_dir, filename, &stream, bits)) != JO_TGA_OK)
        return (code);
    __jo_tga_read_contents(img, stream, transparent_color, *bits);
    jo_free(stream);
    return (code);
}

t_tga_error_code		    jo_tga_8bits_loader(jo_img_8bits *img, const char * const sub_dir, const char * const filename, const int transparent_color_index_in_palette)
{
    int                     bits;

    return (__jo_tga_any_loader((jo_raw_img *)img, sub_dir, filename, transparent_color_index_in_palette, &bits));
}

t_tga_error_code		    jo_tga_loader(jo_img *img, const char * const sub_dir, const char * const filename, const jo_color transparent_color)
{
    int                     bits;

    return (__jo_tga_any_loader((jo_raw_img *)img, sub_dir, filename, transparent_color, &bits));
}

int		                    jo_sprite_add_tga_tileset(const char * const sub_dir, const char * const filename, const jo_color transparent_color, const jo_tile * const tileset, const unsigned int tile_count)
{
    char                    *stream;
    char                    *stream_begin;
    jo_raw_img              full_image;
    jo_raw_img              tile_image;
    register int		    x;
    register int		    y;
    register int		    idx;
    register unsigned int   i;
    int						first_id;
    int                     bits;
    int                     delta;
    bool                    row_top;

    stream = JO_NULL;
    full_image.data = (unsigned short *)-1;/* Disable allocation in __jo_tga_load() */
    if (__jo_tga_load(&full_image, sub_dir, filename, &stream, &bits) != JO_TGA_OK)
        return (-1);
    stream_begin = stream;
    row_top = (((__jo_tga_header *)stream)->image_descriptor & TGA_IMAGEDESCRIPTOR_ROW_TOP) == TGA_IMAGEDESCRIPTOR_ROW_TOP;
    stream += sizeof(__jo_tga_header); /* Jump header */
    first_id = -1;
    for (JO_ZERO(i); i < tile_count; ++i)
    {
#ifdef JO_DEBUG
        if (JO_MOD_POW2(tileset[i].width, 8) != 0)
        {
            jo_core_error("%s: Tile width must be multiple of 8", filename);
            jo_free(stream_begin);
            return (-1);
        }
#endif
        tile_image.width = tileset[i].width;
        tile_image.height = tileset[i].height;
        tile_image.data = jo_malloc_with_behaviour(tile_image.height * tile_image.width * (bits == JO_TGA_8_BITS ? sizeof(unsigned char) : sizeof(jo_color)), JO_FAST_ALLOCATION);
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
                delta = row_top ? (tile_image.height - 1 - y) : y;
                idx = x + delta * tile_image.width;

                if (bits == JO_TGA_8_BITS)
                {
                    ((jo_img_8bits *)&tile_image)->data[idx] = jo_tga_get_pixel(stream, x + tileset[i].x, (full_image.height - (y + tileset[i].y) - 1), full_image.width, bits);
                    if (transparent_color != JO_COLOR_Transparent && ((jo_img_8bits *)&tile_image)->data[idx] == (unsigned char)transparent_color)
                        JO_ZERO(((jo_img_8bits *)&tile_image)->data[idx]);
                }
                else
                {
                    ((jo_img *)&tile_image)->data[idx] = jo_tga_get_pixel(stream, x + tileset[i].x, (full_image.height - (y + tileset[i].y) - 1), full_image.width, bits);
                    if (transparent_color != JO_COLOR_Transparent && ((jo_img *)&tile_image)->data[idx] == transparent_color)
                        ((jo_img *)&tile_image)->data[idx] = JO_COLOR_Transparent;
                }
            }
        }
        x = (bits == JO_TGA_8_BITS ? jo_sprite_add_8bits_image((jo_img_8bits *)&tile_image) : jo_sprite_add((jo_img *)&tile_image));
        jo_free(tile_image.data);
        if (first_id < 0)
            first_id = x;
    }
    jo_free(stream_begin);
    return (first_id);
}

#endif /* !JO_COMPILE_WITH_FS_SUPPORT */

int						jo_sprite_add_tga_from_stream(char *stream, const jo_color transparent_color)
{
    jo_raw_img          img;
    int					id;
    int                 bits;

#ifdef JO_DEBUG
    if (stream == JO_NULL)
    {
        jo_core_error("stream is null");
        return (-1);
    }
#endif
    img.data = JO_NULL;
    if (__jo_tga_any_loader_from_stream(&img, stream, transparent_color, &bits) != JO_TGA_OK)
        return (-1);
#ifdef JO_DEBUG
    if (JO_MOD_POW2(img.width, 8) != 0)
    {
        jo_core_error("Image width must be a multiple of 8");
        jo_free(img.data);
        return (-1);
    }
#endif
    id = (bits == JO_TGA_8_BITS ? jo_sprite_add_8bits_image((jo_img_8bits *)&img) : jo_sprite_add((jo_img *)&img));
    jo_free(img.data);
    return (id);
}

#ifdef JO_COMPILE_WITH_FS_SUPPORT

int						jo_sprite_add_tga(const char * const sub_dir, const char * const filename, const jo_color transparent_color)
{
    jo_raw_img          img;
    int					id;
    int                 bits;

#ifdef JO_DEBUG
    if (filename == JO_NULL)
    {
        jo_core_error("filename is null");
        return (-1);
    }
#endif
    img.data = JO_NULL;
    if (__jo_tga_any_loader(&img, sub_dir, filename, transparent_color, &bits) != JO_TGA_OK)
        return (-1);
#ifdef JO_DEBUG
    if (JO_MOD_POW2(img.width, 8) != 0)
    {
        jo_core_error("%s: Image width must be a multiple of 8", filename);
        jo_free(img.data);
        return (-1);
    }
#endif
    id = (bits == JO_TGA_8_BITS ? jo_sprite_add_8bits_image((jo_img_8bits *)&img) : jo_sprite_add((jo_img *)&img));
    jo_free(img.data);
#ifdef JO_COMPILE_WITH_SPRITE_HASHTABLE
    if (id >= 0)
        __jo_hash_table[id] = jo_4_char_hash(filename);
#endif
    return (id);
}

#endif /* !JO_COMPILE_WITH_FS_SUPPORT */

#endif /* !JO_COMPILE_WITH_TGA_SUPPORT */

/*
** END OF FILE
*/
