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

/** @brief Image types: 1 color mapped, 2 true color, 3 gray; + 8: compressed (RLE) */
#define JO_TGA_RLE                                  (8)

#ifdef JO_COMPILE_WITH_SPRITE_HASHTABLE
extern int      __jo_hash_table[JO_MAX_SPRITE];
#endif

static jo_tga_palette_handling_callback __jo_tga_palette_handling = JO_NULL;
static jo_palette *__jo_tga_default_palette = JO_NULL;
/** @brief End of the file loaded by __jo_tga_load() (JO_NULL: a stream of the caller, its length is not known) */
static const unsigned char *__jo_tga_end;

/** @brief Reader of the pixels of a compressed (RLE) TGA image: packets of a header byte (bit 7: a run of one pixel
 *  repeated, else raw pixels; bits 0 to 6: count - 1) then the pixels. A packet can go on in the next row */
typedef struct
{
    const unsigned char *src;
    unsigned int        count;
    bool                run;
}                       jo_tga_rle;

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

/** @brief Read pixels of a row of a TGA image (the transparent color replaced)
 *  @param dest Output (jo_color, unsigned char for 8 bits)
 *  @param src First pixel in the file
 *  @param count Pixel count
 *  @param bits Bits per pixel of the file
 *  @param transparent_color Color (index) replaced by the transparent one (JO_COLOR_Transparent: none)
 *  @remarks A row at a time: a function per pixel computed its place (2 products) and tested its format (8 bits: the index
 *  + 1, 0 is transparent on the Saturn and 1 the index 0 of the palette; the index 255 is 0, so always transparent; a
 *  transparent index of a 8 bits TGA has the same + 1: JO_TGA_TRANSPARENT_INDEX())
 */
static void                                 jo_tga_read_row(void * const dest, const unsigned char *src, const int count, const int bits,
                                                            const jo_color transparent_color)
{
    unsigned char                           *d8;
    jo_color                                *d;
    jo_color                                c;
    register int                            x;

    if (bits == JO_TGA_8_BITS)
    {
        d8 = (unsigned char *)dest;
        for (JO_ZERO(x); x < count; ++x)
        {
            d8[x] = (unsigned char)(src[x] + 1);
            if (transparent_color != JO_COLOR_Transparent && d8[x] == (unsigned char)transparent_color)
                JO_ZERO(d8[x]);
        }
        return;
    }
    d = (jo_color *)dest;
    // (B, G, R: JO_COLOR_SATURN_RGB() of the 5 high bits of each, by masks: the SH-2 shifts by 1, 2, 8 or 16 bits only)
    if (bits == JO_TGA_32_BITS)
    {
        for (JO_ZERO(x); x < count; ++x, src += 4)
        {
            // (an alpha under 8 is transparent)
            c = src[3] < 8 ? JO_COLOR_Transparent :
                (jo_color)(0x8000 | ((src[0] & 0xF8) << 7) | ((src[1] & 0xF8) << 2) | (src[2] >> 3));
            if (c == transparent_color)
                c = JO_COLOR_Transparent;
            d[x] = c;
        }
        return;
    }
    for (JO_ZERO(x); x < count; ++x, src += 3)
    {
        c = (jo_color)(0x8000 | ((src[0] & 0xF8) << 7) | ((src[1] & 0xF8) << 2) | (src[2] >> 3));
        if (c == transparent_color)
            c = JO_COLOR_Transparent;
        d[x] = c;
    }
}

/** @brief Unpack the next pixels of a compressed (RLE) TGA image, as in a raw image
 *  @param r Reader
 *  @param dest Output (the bytes of the pixels of the file)
 *  @param pixels Pixel count
 *  @param size Bytes per pixel
 *  @remarks A damaged or short file: the pixels after the end of the file are 0
 */
static void                                 jo_tga_rle_unpack(jo_tga_rle * const r, unsigned char *dest, unsigned int pixels, const int size)
{
    unsigned int                            n;
    int                                     k;

    while (pixels > 0)
    {
        if (r->count == 0)
        {
            if (__jo_tga_end != JO_NULL && r->src + 1 + size > __jo_tga_end)
            {
                jo_memset(dest, 0, pixels * size);
                return;
            }
            // (the header byte read once, in a variable: with 2 reads, GCC 8.2 read the count after the header, one byte
            // too far (the write of r->run made it read the byte again, from the pointer already moved on))
            k = *r->src++;
            r->run = (k & 0x80) != 0;
            r->count = (unsigned int)(k & 0x7F) + 1;
        }
        n = JO_MIN(r->count, pixels);
        r->count -= n;
        pixels -= n;
        if (r->run)
        {
            for (; n > 0; --n)
                for (k = 0; k < size; ++k)
                    *dest++ = r->src[k];
            if (r->count == 0)
                r->src += size;
            continue;
        }
        if (__jo_tga_end != JO_NULL && r->src + n * size > __jo_tga_end)
        {
            jo_memset(dest, 0, (n + pixels) * size);
            return;
        }
        jo_memcpy(dest, r->src, n * size);
        dest += n * size;
        r->src += n * size;
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
    __jo_tga_end = len > 0 ? (const unsigned char *)*stream + len : JO_NULL;
    // (1 color mapped, 2 true color, 3 gray; 9 to 11: the same compressed (RLE), see jo_tga_rle_unpack())
    // (a file shorter than its header says, or than a header: the pixels were read after its end. Compressed: the pixels
    // stop at the end of the file)
    if (!JO_TGA_SUPPORTED_FORMAT(*bits) || (header->image_type & ~JO_TGA_RLE) < 1 || (header->image_type & ~JO_TGA_RLE) > 3 ||
        (len > 0 && ((unsigned int)len < sizeof(*header) ||
                     (unsigned long long)len < sizeof(*header) + header->id_length +
                     (*bits == JO_TGA_8_BITS ? (unsigned long long)(unsigned short)jo_swap_endian_short(header->color_map_length) * JO_DIV_BY_8(header->color_map_entry_depth + 1) : 0) +
                     ((header->image_type & JO_TGA_RLE) ? 0 : (unsigned long long)img->width * img->height * JO_DIV_BY_8(*bits)))))
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
    unsigned char                       *row;
    register int		                idx;
    register int		                y;
    register int                        delta;
    bool                                row_top;
    jo_tga_rle                          rle;

    row = JO_NULL;
    rle.src = JO_NULL;
    JO_ZERO(rle.count);
    rle.run = false;
    image = __jo_tga_read_header(stream, &row_top, bits);
    // Compressed: each row unpacked in a buffer (out of memory: the image is transparent)
    if (((__jo_tga_header *)stream)->image_type & JO_TGA_RLE)
    {
        if ((row = (unsigned char *)jo_malloc(img->width * JO_DIV_BY_8(bits))) == JO_NULL)
        {
            jo_memset(img->data, 0, img->width * img->height * (bits == JO_TGA_8_BITS ? 1 : sizeof(jo_color)));
            return;
        }
        rle.src = (const unsigned char *)image;
        JO_ZERO(rle.count);
    }
    for (JO_ZERO(y); y < img->height; ++y)
    {
        delta = row_top ? y : (img->height - 1 - y);
        idx = delta * img->width;
        if (row != JO_NULL)
            jo_tga_rle_unpack(&rle, row, img->width, JO_DIV_BY_8(bits));
        jo_tga_read_row(bits == JO_TGA_8_BITS ? (void *)(((jo_img_8bits *)img)->data + idx) : (void *)(((jo_img *)img)->data + idx),
                        row != JO_NULL ? row : (const unsigned char *)image + y * img->width * JO_DIV_BY_8(bits), img->width, bits, transparent_color);
    }
    if (row != JO_NULL)
        jo_free(row);
}

#ifdef JO_COMPILE_WITH_FS_SUPPORT
/** @brief The pixels of a tileset (the tiles are read anywhere in the image): a compressed image unpacked
 *  @param file The file (freed when unpacked)
 *  @param pixels The pixels in the file (after the header and the palette)
 *  @param img Size of the image
 *  @param bits Bits per pixel
 *  @param block The block to free with the pixels (output: the file, or the unpacked pixels)
 *  @return The pixels, as in a raw image (JO_NULL: out of memory, the file freed)
 */
static char                             *jo_tga_tileset_pixels(char * const file, char * const pixels, const jo_raw_img * const img,
                                                               const int bits, char ** const block)
{
    jo_tga_rle                          rle;
    char                                *unpacked;

    *block = file;
    if (!(((__jo_tga_header *)file)->image_type & JO_TGA_RLE))
        return (pixels);
    unpacked = (char *)jo_malloc(img->width * img->height * JO_DIV_BY_8(bits));
    if (unpacked != JO_NULL)
    {
        rle.src = (const unsigned char *)pixels;
        JO_ZERO(rle.count);
        jo_tga_rle_unpack(&rle, (unsigned char *)unpacked, img->width * img->height, JO_DIV_BY_8(bits));
    }
    jo_free(file);
    *block = unpacked;
    return (unpacked);
}
#endif

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
    stream = jo_tga_tileset_pixels(stream, __jo_tga_read_header(stream, &row_top, bits), &full_image, bits, &stream_begin);
    if (stream == JO_NULL)
        return (JO_TGA_OUT_OF_MEMORY);
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
        // (a row at a time, see jo_tga_read_row(); the row of the file of the row y of the tile)
        for (JO_ZERO(y); y < output_tiles[i].height; ++y)
        {
            delta = row_top ? (y + tileset[i].y) : (full_image.height - (y + tileset[i].y) - 1);
            idx = y * output_tiles[i].width;
            jo_tga_read_row(bits == JO_TGA_8_BITS ? (void *)(((jo_img_8bits *)&output_tiles[i])->data + idx) : (void *)(((jo_img *)&output_tiles[i])->data + idx),
                            (const unsigned char *)stream + (delta * full_image.width + tileset[i].x) * JO_DIV_BY_8(bits),
                            output_tiles[i].width, bits, transparent_color);
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
    stream = jo_tga_tileset_pixels(stream, __jo_tga_read_header(stream, &row_top, bits), &full_image, bits, &stream_begin);
    if (stream == JO_NULL)
        return (-1);
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
        // (a row at a time, see jo_tga_read_row(); the row of the file of the row y of the tile)
        for (JO_ZERO(y); y < tile_image.height; ++y)
        {
            delta = row_top ? (y + tileset[i].y) : (full_image.height - (y + tileset[i].y) - 1);
            idx = y * tile_image.width;
            jo_tga_read_row(bits == JO_TGA_8_BITS ? (void *)(((jo_img_8bits *)&tile_image)->data + idx) : (void *)(((jo_img *)&tile_image)->data + idx),
                            (const unsigned char *)stream + (delta * full_image.width + tileset[i].x) * JO_DIV_BY_8(bits),
                            tile_image.width, bits, transparent_color);
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
