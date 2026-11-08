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

/*
** INCLUDES
*/
#include <stdbool.h>
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
#include "jo/vdp1_command_pipeline.h"
#include "jo/sprites.h"
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
static jo_palette *__jo_tga_default_palette = JO_NULL;

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

void                                        jo_set_tga_default_palette(jo_palette *palette)
{
    __jo_tga_default_palette = palette;
}

void                                        jo_set_tga_palette_handling(jo_tga_palette_handling_callback callback)
{
    __jo_tga_palette_handling = callback;
}

static  __jo_force_inline jo_color          jo_tga_get_pixel(const char * const restrict stream, const int x, const int y, const int width, const int bits)
{
    switch (bits)
    {
    case JO_TGA_8_BITS:
        // (+1 because 0 is transparent on the Saturn: 1 is the index 0 of the palette. The index 255 is 0 in the 8 bits
        // images, so always transparent; a transparent color of a 8 bits TGA has the same + 1: JO_TGA_TRANSPARENT_INDEX())
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
    int                                     len;

    // (the length of the file read here: 0 for a stream of the caller, its length is not known)
    JO_ZERO(len);
#ifdef JO_COMPILE_WITH_FS_SUPPORT
    if (*stream == JO_NULL)
        *stream = jo_fs_read_file_in_dir(filename, sub_dir, &len);
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
    // (not compressed: 1 color mapped, 2 true color, 3 gray. The RLE images (9 to 11) are not supported: read as raw, the
    // pixels were wrong and read after the end of the file)
    // (a file shorter than its header says, or than a header: the pixels were read after its end)
    if (!JO_TGA_SUPPORTED_FORMAT(*bits) || header->image_type < 1 || header->image_type > 3 ||
        (len > 0 && ((unsigned int)len < sizeof(*header) ||
                     (unsigned long long)len < sizeof(*header) + header->id_length +
                     (*bits == JO_TGA_8_BITS ? (unsigned long long)(unsigned short)jo_swap_endian_short(header->color_map_length) * JO_DIV_BY_8(header->color_map_entry_depth + 1) : 0) +
                     (unsigned long long)img->width * img->height * JO_DIV_BY_8(*bits))))
    {
#ifdef JO_DEBUG
        jo_core_error("%s: Unsupported TGA (%d bits, type %d)", filename, *bits, header->image_type);
#endif
        if (filename != JO_NULL)
            jo_free(*stream);
        return (JO_TGA_UNSUPPORTED_FORMAT);
    }
    // (the size in 64 bits: a damaged header must not wrap it to a small block, then filled with the whole picture)
    if (img->data == JO_NULL &&
        (unsigned long long)img->height * img->width * (*bits == JO_TGA_8_BITS ? sizeof(unsigned char) : sizeof(jo_color)) < 0x01000000ull)
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

static char                             *__jo_tga_read_header(char * restrict stream, bool *row_top, const int bits)
{
    __jo_tga_header                     *header;
    register int		                idx;
    register int                        palette_length;
    register int                        entry_size;

    header = (__jo_tga_header *)stream;
    *row_top = (header->image_descriptor & TGA_IMAGEDESCRIPTOR_ROW_TOP) == TGA_IMAGEDESCRIPTOR_ROW_TOP;
    // (the image ID after the header: its length in the header)
    stream += sizeof(*header) + header->id_length;
    if (bits == JO_TGA_8_BITS)
    {
        jo_palette                      *palette;
        unsigned short                  color;

        palette = __jo_tga_default_palette;
        palette_length = jo_swap_endian_short(header->color_map_length);
        // (the size of a color of the palette: 3 bytes for 24 bits, 4 for 32 bits, 2 for 15 or 16 bits. The colors were
        // read as 3 bytes: the other sizes gave wrong colors and moved the pixels)
        entry_size = JO_DIV_BY_8(header->color_map_entry_depth + 1);
        // palette->data is JO_NULL when the color RAM is full (see jo_create_palette())
        if ((palette != JO_NULL || (__jo_tga_palette_handling != JO_NULL && (palette = __jo_tga_palette_handling()) != JO_NULL)) && palette->data != JO_NULL)
        {
            for (JO_ZERO(idx); idx < palette_length && idx < JO_PALETTE_MAX_COLORS; ++idx)
            {
                if (entry_size == 2)
                {
                    // (A RRRRR GGGGG BBBBB, little endian)
                    color = (unsigned short)((unsigned char)stream[0] | ((unsigned char)stream[1] << 8));
                    palette->data[idx] = JO_COLOR_SATURN_RGB((color >> 10) & 0x1f, (color >> 5) & 0x1f, color & 0x1f);
                }
                else
                    palette->data[idx] = JO_COLOR_SATURN_RGB(JO_TGA_CONVERT_COLOR(stream, 2), JO_TGA_CONVERT_COLOR(stream, 1), JO_TGA_CONVERT_COLOR(stream, 0));
                stream += entry_size;
            }
            // (the colors not read: the 256th one of a palette of 256 colors, the palette has 255 colors and the transparent
            // color 0. Not one more: the pixels of a palette of less than 255 colors were moved by 3 bytes)
            stream += (palette_length - idx) * entry_size;
        }
        else
            stream += palette_length * entry_size;
    }
    return (stream);
}

static void                             __jo_tga_read_contents(jo_raw_img *img, char * restrict stream, const jo_color transparent_color, const int bits)
{
    char                                *image;
    register int		                idx;
    register int		                x;
    register int		                y;
    register int                        delta;
    bool                                row_top;

    image = __jo_tga_read_header(stream, &row_top, bits);
    for (JO_ZERO(y); y < img->height; ++y)
    {
        for (JO_ZERO(x); x < img->width; ++x)
        {
            delta = row_top ? y : (img->height - 1 - y);
            idx = x + delta * img->width;
            if (bits == JO_TGA_8_BITS)
            {
                ((jo_img_8bits *)img)->data[idx] = jo_tga_get_pixel(image, x, y, img->width, bits);
                if (transparent_color != JO_COLOR_Transparent && ((jo_img_8bits *)img)->data[idx] == (unsigned char)transparent_color)
                    JO_ZERO(((jo_img_8bits *)img)->data[idx]);
            }
            else
            {
                ((jo_img *)img)->data[idx] = jo_tga_get_pixel(image, x, y, img->width, bits);
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

bool                        jo_set_background_tga(const char * const sub_dir, const char * const filename, const jo_color transparent_color)
{
    jo_img                  img;

    img.data = JO_NULL;
    // (the pixels are allocated after the checks of the file: nothing to free when it fails)
    if (jo_tga_loader(&img, sub_dir, filename, transparent_color) != JO_TGA_OK)
        return (false);
    jo_vdp2_set_nbg1_image(&img, 0, 0);
    jo_free_img(&img);
    return (true);
}

/** @brief Free the tiles allocated by jo_tga_8bits_tileset_loader() when an error occurs
 *  @param output_tiles Tiles
 *  @param allocated Bitset of the tiles allocated by jo_tga_8bits_tileset_loader()
 *  @param count Number of tiles to check
 *  @remarks Tiles allocated by the caller are not freed
 */
static void                 jo_tga_free_allocated_tiles(jo_raw_img *output_tiles, const unsigned char * const allocated, const unsigned int count)
{
    register unsigned int   i;

    for (JO_ZERO(i); i < count; ++i)
    {
        if (allocated[JO_DIV_BY_8(i)] & (1 << JO_MOD_POW2(i, 8)))
        {
            jo_free(output_tiles[i].data);
            output_tiles[i].data = JO_NULL;
        }
    }
}

t_tga_error_code            jo_tga_8bits_tileset_loader(const char * const sub_dir, const char * const filename, const jo_color transparent_color, const jo_tile * const tileset, const unsigned int tile_count, jo_raw_img *output_tiles)
{
    t_tga_error_code        response;
    char                    *stream;
    char                    *stream_begin;
    jo_raw_img              full_image;
    register int		    x;
    register int		    y;
    register int		    idx;
    register unsigned int   i;
    int                     bits;
    int                     delta;
    bool                    row_top;
    unsigned char           allocated[JO_DIV_BY_8(tile_count) + 1];

    jo_memset(allocated, 0, sizeof(allocated));
    stream = JO_NULL;
    full_image.data = (unsigned short *)-1;/* Disable allocation in __jo_tga_load() */
    response = __jo_tga_load(&full_image, sub_dir, filename, &stream, &bits);
    if (response != JO_TGA_OK)
        return (response);
    stream_begin = stream;
    stream = __jo_tga_read_header(stream, &row_top, bits);
    for (JO_ZERO(i); i < tile_count; ++i)
    {
#ifdef JO_DEBUG
        if (JO_MOD_POW2(tileset[i].width, 8) != 0)
        {
            jo_core_error("%s: Tile width must be multiple of 8", filename);
            jo_tga_free_allocated_tiles(output_tiles, allocated, i);
            jo_free(stream_begin);
            return (JO_TGA_UNSUPPORTED_FORMAT);
        }
#endif
        // (a tile out of the image: read before or after the pixels of the file, as jo_sprite_add_bin_tileset())
        if (tileset[i].x < 0 || tileset[i].y < 0 || tileset[i].x + tileset[i].width > full_image.width ||
            tileset[i].y + tileset[i].height > full_image.height)
        {
#ifdef JO_DEBUG
            jo_core_error("%s: tile %d out of the image", filename, i);
#endif
            jo_tga_free_allocated_tiles(output_tiles, allocated, i);
            jo_free(stream_begin);
            return (JO_TGA_UNSUPPORTED_FORMAT);
        }
        output_tiles[i].width = tileset[i].width;
        output_tiles[i].height = tileset[i].height;
        if (output_tiles[i].data == JO_NULL)
        {
            output_tiles[i].data = jo_malloc_with_behaviour(output_tiles[i].height * output_tiles[i].width * (bits == JO_TGA_8_BITS ? sizeof(unsigned char) : sizeof(jo_color)), JO_FAST_ALLOCATION);
            if (output_tiles[i].data == JO_NULL)
            {
#ifdef JO_DEBUG
                jo_core_error("%s: Out of memory", filename);
#endif
                jo_tga_free_allocated_tiles(output_tiles, allocated, i);
                jo_free(stream_begin);
                return (JO_TGA_OUT_OF_MEMORY);
            }
            allocated[JO_DIV_BY_8(i)] |= (1 << JO_MOD_POW2(i, 8));
        }
        for (JO_ZERO(y); y < output_tiles[i].height; ++y)
        {
            for (JO_ZERO(x); x < output_tiles[i].width; ++x)
            {
                delta = y; // (the source row follows the order of the rows of the file)
                idx = x + delta * output_tiles[i].width;
                if (bits == JO_TGA_8_BITS)
                {
                    ((jo_img_8bits *)&output_tiles[i])->data[idx] = jo_tga_get_pixel(stream, x + tileset[i].x, (row_top ? (y + tileset[i].y) : (full_image.height - (y + tileset[i].y) - 1)), full_image.width, bits);
                    if (transparent_color != JO_COLOR_Transparent && ((jo_img_8bits *)&output_tiles[i])->data[idx] == (unsigned char)transparent_color)
                        JO_ZERO(((jo_img_8bits *)&output_tiles[i])->data[idx]);
                }
                else
                {
                    ((jo_img *)&output_tiles[i])->data[idx] = jo_tga_get_pixel(stream, x + tileset[i].x, (row_top ? (y + tileset[i].y) : (full_image.height - (y + tileset[i].y) - 1)), full_image.width, bits);
                    if (transparent_color != JO_COLOR_Transparent && ((jo_img *)&output_tiles[i])->data[idx] == transparent_color)
                        ((jo_img *)&output_tiles[i])->data[idx] = JO_COLOR_Transparent;
                }
            }
        }
    }
    jo_free(stream_begin);
    return (JO_TGA_OK);
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
    stream = __jo_tga_read_header(stream, &row_top, bits);
    first_id = -1;
    for (JO_ZERO(i); i < tile_count; ++i)
    {
#ifdef JO_DEBUG
        if (JO_MOD_POW2(tileset[i].width, 8) != 0)
        {
            jo_core_error("%s: Tile width must be multiple of 8", filename);
            if (first_id >= 0)
                jo_sprite_free_from(first_id);
            jo_free(stream_begin);
            return (-1);
        }
#endif
        // (a tile out of the image: read before or after the pixels of the file, as jo_sprite_add_bin_tileset())
        if (tileset[i].x < 0 || tileset[i].y < 0 || tileset[i].x + tileset[i].width > full_image.width ||
            tileset[i].y + tileset[i].height > full_image.height)
        {
#ifdef JO_DEBUG
            jo_core_error("%s: tile %d out of the image", filename, i);
#endif
            if (first_id >= 0)
                jo_sprite_free_from(first_id);
            jo_free(stream_begin);
            return (-1);
        }
        tile_image.width = tileset[i].width;
        tile_image.height = tileset[i].height;
        tile_image.data = jo_malloc_with_behaviour(tile_image.height * tile_image.width * (bits == JO_TGA_8_BITS ? sizeof(unsigned char) : sizeof(jo_color)), JO_FAST_ALLOCATION);
        if (tile_image.data == JO_NULL)
        {
#ifdef JO_DEBUG
            jo_core_error("%s: Out of memory", filename);
#endif
            // Remove the tiles already added
            if (first_id >= 0)
                jo_sprite_free_from(first_id);
            jo_free(stream_begin);
            return (-1);
        }
        for (JO_ZERO(y); y < tile_image.height; ++y)
        {
            for (JO_ZERO(x); x < tile_image.width; ++x)
            {
                delta = y; // (the source row follows the order of the rows of the file)
                idx = x + delta * tile_image.width;
                if (bits == JO_TGA_8_BITS)
                {
                    ((jo_img_8bits *)&tile_image)->data[idx] = jo_tga_get_pixel(stream, x + tileset[i].x, (row_top ? (y + tileset[i].y) : (full_image.height - (y + tileset[i].y) - 1)), full_image.width, bits);
                    if (transparent_color != JO_COLOR_Transparent && ((jo_img_8bits *)&tile_image)->data[idx] == (unsigned char)transparent_color)
                        JO_ZERO(((jo_img_8bits *)&tile_image)->data[idx]);
                }
                else
                {
                    ((jo_img *)&tile_image)->data[idx] = jo_tga_get_pixel(stream, x + tileset[i].x, (row_top ? (y + tileset[i].y) : (full_image.height - (y + tileset[i].y) - 1)), full_image.width, bits);
                    if (transparent_color != JO_COLOR_Transparent && ((jo_img *)&tile_image)->data[idx] == transparent_color)
                        ((jo_img *)&tile_image)->data[idx] = JO_COLOR_Transparent;
                }
            }
        }
        x = (bits == JO_TGA_8_BITS ? jo_sprite_add_8bits_image((jo_img_8bits *)&tile_image) : jo_sprite_add((jo_img *)&tile_image));
        jo_free(tile_image.data);
        // (the sprites full: the ids after the last one added were given to the font, the tiles were missing)
        if (x < 0)
        {
            if (first_id >= 0)
                jo_sprite_free_from(first_id);
            jo_free(stream_begin);
            return (-1);
        }
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
