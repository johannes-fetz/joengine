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
#include "jo/image.h"
#include "jo/math.h"
#include "jo/colors.h"
#include "jo/sprites.h"

extern int      __jo_hash_table[JO_MAX_SPRITE];

#ifdef JO_COMPILE_WITH_FS_SUPPORT

unsigned short      *__jo_bin_load(jo_img *img, const char * const sub_dir, const char * const filename)
{
    unsigned short  *stream;

    stream = (unsigned short *)jo_fs_read_file_in_dir(filename, sub_dir, JO_NULL);
    if (stream == JO_NULL)
        return (JO_NULL);
    img->width = jo_swap_endian_ushort(*stream);
    img->height = jo_swap_endian_ushort(*(stream + 1));
    if (img->data == JO_NULL)
        img->data = stream;
    if (img->data == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory");
#endif
        jo_free(stream);
        return (JO_NULL);
    }
    return (stream);
}

#endif /* !JO_COMPILE_WITH_FS_SUPPORT */

void	                jo_free_img(jo_img * const img)
{
#ifdef JO_DEBUG
    if (img == JO_NULL)
    {
        jo_core_error("img is null");
        return ;
    }
#endif
    jo_free(img->data);
    img->data = JO_NULL;
}

bool                    jo_bin_loader_from_stream(jo_img *img, char *stream, const jo_color transparent_color)
{
    register int        i;
    register int        size;

    img->width = jo_swap_endian_ushort(*stream);
    img->height = jo_swap_endian_ushort(*(stream + 1));
    if (img->data == JO_NULL)
        img->data = (unsigned short *)jo_malloc(img->height * img->width * sizeof(*img->data));
    if (img->data == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory");
#endif
        return (false);
    }
    stream += 2; /* Jump header */
    size = img->width * img->height;
    for (JO_ZERO(i); i < size; ++i)
    {
        img->data[i] = jo_swap_endian_ushort(*(stream + i));
        if (transparent_color != JO_COLOR_Transparent && img->data[i] == transparent_color)
            img->data[i] = JO_COLOR_Transparent;
    }
    return (true);
}

#ifdef JO_COMPILE_WITH_FS_SUPPORT

bool                    jo_bin_loader(jo_img *img, const char * const sub_dir, const char *const filename, const jo_color transparent_color)
{
    register int        i;
    register int        size;
    unsigned short      *stream;
    unsigned short      *stream_begin;

    stream = __jo_bin_load(img, sub_dir, filename);
    if (stream == JO_NULL)
        return (false);
    stream_begin = stream;
    stream += 2; /* Jump header */
    size = img->width * img->height;
    for (JO_ZERO(i); i < size; ++i)
    {
        img->data[i] = jo_swap_endian_ushort(*(stream + i));
        if (transparent_color != JO_COLOR_Transparent && img->data[i] == transparent_color)
            img->data[i] = JO_COLOR_Transparent;
    }
    if (stream_begin != img->data)
        jo_free(stream_begin);
    return (true);
}

#endif /* !JO_COMPILE_WITH_FS_SUPPORT */

void	                jo_replace_color(const jo_img * const img, const jo_color src_color, const jo_color dest_color)
{
    register int        i;
    register int        end;
#ifdef JO_DEBUG
    if (img == JO_NULL)
    {
        jo_core_error("img is null");
        return ;
    }
#endif
    end = img->width * img->height;
    for (JO_ZERO(i); i < end; ++i)
        if (img->data[i] == src_color)
            img->data[i] = dest_color;
}

#ifdef JO_COMPILE_WITH_FS_SUPPORT

int		                    jo_sprite_add_bin_tileset(const char * const sub_dir, const char * const filename, const jo_color transparent_color, const jo_tile * const tileset, const unsigned int tile_count)
{
    unsigned short          *stream;
    unsigned short          *stream_begin;
    jo_img                  full_image;
    jo_img                  tile_image;
    register int		    x;
    register int		    y;
    register int		    idx;
    register unsigned int   i;
    int						first_id;

#ifdef JO_DEBUG
    if (filename == JO_NULL)
    {
        jo_core_error("filename is null");
        return (-1);
    }
#endif
    full_image.data = (unsigned short *)-1;/* Disable allocation in __jo_bin_load() */
    stream = __jo_bin_load(&full_image, sub_dir, filename);
    if (stream == JO_NULL)
        return (-1);
    stream_begin = stream;
    stream += 2; /* Jump header */
    first_id = -1;
    for (JO_ZERO(i); i < tile_count; ++i)
    {
#ifdef JO_DEBUG
        if (JO_MOD_POW2(tileset[i].width, 8) != 0)
        {
            jo_core_error("%s: Image width must be a multiple of 8", filename);
            jo_free(stream_begin);
            return (-1);
        }
#endif
        tile_image.width = tileset[i].width;
        tile_image.height = tileset[i].height;
        tile_image.data = (unsigned short *)jo_malloc_with_behaviour(tile_image.height * tile_image.width * sizeof(*tile_image.data), JO_MALLOC_TRY_REUSE_BLOCK);
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
                tile_image.data[idx] = jo_swap_endian_ushort(stream[(x + tileset[i].x) + (y + tileset[i].y) * full_image.width]);
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

int						jo_sprite_add_bin(const char * const sub_dir, const char * const filename, const jo_color transparent_color)
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
    if (!jo_bin_loader(&img, sub_dir, filename, transparent_color))
        return (-1);
#ifdef JO_DEBUG
    if (JO_MOD_POW2(img.width, 8) != 0)
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

#endif /* !JO_COMPILE_WITH_FS_SUPPORT */

int						jo_sprite_add_bin_from_stream(char *stream, const jo_color transparent_color)
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
    if (!jo_bin_loader_from_stream(&img, stream, transparent_color))
        return (-1);
#ifdef JO_DEBUG
    if (JO_MOD_POW2(img.width, 8) != 0)
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

#ifdef JO_COMPILE_WITH_FS_SUPPORT

int							jo_sprite_add_image_pack(const char * const sub_dir, const char * const filename, const jo_color transparent_color)
{
    char                    *stream_begin;
    char                    *stream;
    register int			i;
    int						first_id;
    int						res;
    int						count;
    char                    filenames[JO_MAX_FILE_IN_IMAGE_PACK][JO_MAX_FILENAME_LENGTH];

    first_id = -1;
    JO_ZERO(count);
    JO_ZERO(res);
    if ((stream = jo_fs_read_file_in_dir(filename, sub_dir, 0)) == JO_NULL)
        return (-1);
    stream_begin = stream;
    while (*stream)
    {
        while (*stream && jo_tools_is_whitespace(*stream))
            ++stream;
        if (!*stream)
            break;
#ifdef JO_DEBUG
        if (count >= JO_MAX_FILE_IN_IMAGE_PACK)
        {
            jo_core_error("Please increase JO_MAX_FILE_IN_IMAGE_PACK");
            jo_free(stream_begin);
            return (-1);
        }
#endif
        for (JO_ZERO(i); *stream && !jo_tools_is_whitespace(*stream); ++i)
            filenames[count][i] = *stream++;
        JO_ZERO(filenames[count][i]);
        ++count;
    }
    for (JO_ZERO(i); i < count; ++i)
    {
        if (jo_endwith(filenames[i], ".BIN"))
            res = jo_sprite_add_bin(sub_dir, filenames[i], transparent_color);
#ifdef JO_COMPILE_WITH_TGA_SUPPORT
        else
            res = jo_sprite_add_tga(sub_dir, filenames[i], transparent_color);
#endif
        if (first_id < 0)
            first_id = res;
    }
    jo_free(stream_begin);
    return (first_id);
}

#endif /* !JO_COMPILE_WITH_FS_SUPPORT */

/*
** END OF FILE
*/
