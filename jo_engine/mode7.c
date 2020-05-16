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
#include "jo/image.h"
#include "jo/math.h"
#include "jo/mode7.h"

#ifdef JO_COMPILE_WITH_PSEUDO_MODE7_SUPPORT

static unsigned short * __mode7_buffer = JO_NULL;

static bool     jo_common_init_mode7(jo_mode7 * const params, const unsigned short tile_width, const unsigned short tile_height)
{
    int         screen_y;
    int         fconst_zy;

    if (__mode7_buffer == JO_NULL)
        __mode7_buffer = jo_malloc(1024 * sizeof(*__mode7_buffer));
    if (__mode7_buffer == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory #1");
#endif
        return (false);
    }
    params->__cache.tile_mask_x = tile_width - 1;
    params->__cache.tile_mask_y = tile_height - 1;
    params->__cache.output_width_sizeof = params->__cache.output_width * sizeof(*params->__cache.output);
    params->__cache.output_width_div2 = JO_DIV_BY_2(params->__cache.output_width);
    if (params->__cache.distance == JO_NULL)
        params->__cache.distance = jo_malloc(params->__cache.output_height * sizeof(*params->__cache.distance));
    if (params->__cache.distance == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory #2");
#endif
        return (false);
    }
    if (params->__cache.line_dy == JO_NULL)
        params->__cache.line_dy = jo_malloc(params->__cache.output_height * sizeof(*params->__cache.line_dy));
    if (params->__cache.line_dy == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory #3");
#endif
        return (false);
    }
    fconst_zy = JO_MULT_BY_1024(params->space_z * params->scale_y);
    for (screen_y = params->horizon; screen_y < params->__cache.output_height; ++screen_y)
    {
        if (screen_y == 0)
            params->__cache.distance[screen_y] = fconst_zy;
        else
            params->__cache.distance[screen_y] = fconst_zy / screen_y;
        params->__cache.line_dy[screen_y] = params->__cache.distance[screen_y] / params->scale_x;
    }
    return (true);
}

inline void     jo_update_screen_output_coord_for_mode7(jo_mode7 * const params)
{
    params->__cache.output_y_start = params->horizon + params->output_top;
    params->__cache.output = ((unsigned short *)VDP2_VRAM_A0 + params->__cache.output_y_start * JO_VDP2_WIDTH) + params->output_left;
}

bool        jo_init_mode7_for_screen_output(jo_mode7 * const params, const unsigned short tile_width, const unsigned short tile_height)
{
    params->__cache.output_width = JO_TV_WIDTH;
    params->__cache.output_height = JO_TV_HEIGHT;
    params->__cache.output_y_start = params->horizon + params->output_top;
    params->__cache.output = ((unsigned short *)VDP2_VRAM_A0 + params->__cache.output_y_start * JO_VDP2_WIDTH) + params->output_left;
    params->__cache.output_line_width = JO_VDP2_WIDTH;
    return (jo_common_init_mode7(params, tile_width, tile_height));
}

bool        jo_init_mode7_for_image_output(jo_mode7 *const params, const unsigned short tile_width, const unsigned short tile_height, jo_img * const output_image)
{
    params->__cache.output_width = output_image->width;
    params->__cache.output_height = output_image->height;
    params->__cache.output_line_width = output_image->width;
    if (output_image->data == JO_NULL)
        output_image->data = jo_malloc(output_image->width * output_image->height * sizeof(*output_image->data));
    if (output_image->data == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory");
#endif
        return (false);
    }
    params->__cache.output_y_start = params->horizon + params->output_top;
    params->__cache.output = output_image->data + params->__cache.output_y_start * output_image->width;
    return (jo_common_init_mode7(params, tile_width, tile_height));
}

void        jo_free_mode7(jo_mode7 * const params)
{
    if (params->__cache.line_dy != JO_NULL)
    {
        jo_free(params->__cache.line_dy);
        params->__cache.line_dy = JO_NULL;
    }
    if (params->__cache.distance != JO_NULL)
    {
        jo_free(params->__cache.distance);
        params->__cache.distance = JO_NULL;
    }
}

void                jo_do_mode7_floor(const jo_img * const tile, const int cam_y, const int cam_x, const jo_mode7 * const params)
{
    register int    output_y;
    int             fspace_y;
    register int    i;
    unsigned short  *tile_ptr;
    unsigned short  *dst;
    const int       cam_y_fixed_value = JO_MULT_BY_1024(cam_y);

    dst = params->__cache.output;
    for (output_y = params->__cache.output_y_start; output_y < params->__cache.output_height; ++output_y)
    {
        fspace_y = cam_y_fixed_value - params->__cache.output_width_div2 * params->__cache.line_dy[output_y];
        tile_ptr = tile->data + ((cam_x + JO_DIV_BY_1024(params->__cache.distance[output_y])) & params->__cache.tile_mask_x);
        for (JO_ZERO(i); i < params->__cache.output_width; ++i)
        {
            __mode7_buffer[i] = *(tile_ptr + ((JO_DIV_BY_1024(fspace_y) & params->__cache.tile_mask_y) * tile->width));
            fspace_y += params->__cache.line_dy[output_y];
        }
        jo_dma_copy(__mode7_buffer, dst, params->__cache.output_width_sizeof);
        dst += params->__cache.output_line_width;
    }
}

#endif /* !JO_COMPILE_WITH_PSEUDO_MODE7_SUPPORT */

/*
** END OF FILE
*/
