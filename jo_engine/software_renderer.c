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

#ifdef JO_COMPILE_WITH_SOFTWARE_RENDERER_SUPPORT

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
#include "jo/colors.h"
#include "jo/sprites.h"
#include "jo/fs.h"
#include "jo/math.h"
#include "jo/list.h"
#include "jo/vdp2.h"
#include "jo/vdp2_malloc.h"
#include "jo/software_renderer.h"

/*
 ██████╗ ██████╗ ██████╗ ███████╗
██╔════╝██╔═══██╗██╔══██╗██╔════╝
██║     ██║   ██║██████╔╝█████╗
██║     ██║   ██║██╔══██╗██╔══╝
╚██████╗╚██████╔╝██║  ██║███████╗
 ╚═════╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝
*/

extern jo_picture_definition                __jo_sprite_pic[JO_MAX_SPRITE];
extern jo_color                             *nbg1_bitmap;

static __jo_force_inline void               __jo_software_renderer_clear_zbuffer(jo_software_renderer_gfx * const gfx)
{
    register int                            *ptr;
    register int                            *end;

    ptr = (int *)gfx->depth_buffer;
    end = ptr + gfx->depth_buffer_dword_size;
    while (ptr < end)
        *ptr++ = JO_FIXED_MIN;
}

void                                        jo_software_renderer_free(jo_software_renderer_gfx * const gfx)
{
#ifdef JO_DEBUG
    if (gfx == JO_NULL)
    {
        jo_core_error("gfx is null");
        return ;
    }
#endif
    if (gfx->color_buffer != JO_NULL)
        jo_free(gfx->color_buffer);
    if (gfx->depth_buffer != JO_NULL)
        jo_free(gfx->depth_buffer);
    jo_free(gfx);
}

jo_software_renderer_gfx                    *jo_software_renderer_create(unsigned short width, unsigned short height, const jo_scroll_screen screen)
{
    jo_img                                  img;
    jo_software_renderer_gfx                *gfx;

    gfx = (jo_software_renderer_gfx *)jo_malloc(sizeof(*gfx));
    if (gfx == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory");
#endif
        return (JO_NULL);
    }
    img.height = height;
    img.width = width;
    img.data = JO_NULL;
    gfx->depth_mode_testing = JO_SR_DEPTH_GREATER_OR_EQUAL;
    gfx->sprite_id = -1;
    gfx->clipping_size.width = width;
    gfx->clipping_size.height = height;
    switch (screen)
    {
        case JO_SPRITE_SCREEN:
            if ((gfx->sprite_id = jo_sprite_add(&img)) < 0)
            {
                jo_software_renderer_free(gfx);
                return (JO_NULL);
            }
            gfx->vram = __jo_sprite_pic[gfx->sprite_id].data;
            gfx->vram_size.width = width;
            gfx->vram_size.height = height;
            break;
        case JO_NBG1_SCREEN:
            jo_vdp2_set_nbg1_image(&img, 0, 0);
            gfx->vram = nbg1_bitmap;
            gfx->vram_size.width = JO_VDP2_WIDTH;
            gfx->vram_size.height = JO_VDP2_HEIGHT;
            break;
        default:
            jo_software_renderer_free(gfx);
#ifdef JO_DEBUG
            jo_core_error("Unsupported screen");
#endif
            return (JO_NULL);
    }
    gfx->color_buffer_size = sizeof(jo_color) * gfx->vram_size.width * height;
    gfx->color_buffer = (jo_color *)jo_malloc(gfx->color_buffer_size);
    if (gfx->color_buffer == JO_NULL)
    {
        jo_software_renderer_free(gfx);
#ifdef JO_DEBUG
        jo_core_error("Out of memory");
#endif
        return (JO_NULL);
    }
    gfx->depth_buffer_dword_size = (sizeof(*gfx->depth_buffer) * gfx->vram_size.width * height) / sizeof(int);
    gfx->depth_buffer = jo_malloc(gfx->depth_buffer_dword_size * sizeof(int));
    if (gfx->depth_buffer == JO_NULL)
    {
        jo_software_renderer_free(gfx);
#ifdef JO_DEBUG
        jo_core_error("Out of memory");
#endif
        return (JO_NULL);
    }
    __jo_software_renderer_clear_zbuffer(gfx);
    return (gfx);
}

void                                        jo_software_renderer_flush(jo_software_renderer_gfx * const gfx)
{
#ifdef JO_DEBUG
    if (gfx == JO_NULL)
    {
        jo_core_error("gfx is null");
        return ;
    }
#endif
    jo_dma_copy(gfx->color_buffer, gfx->vram, gfx->color_buffer_size);
    __jo_software_renderer_clear_zbuffer(gfx);
}

/*
██████╗  █████╗ ███████╗██╗ ██████╗    ██████╗ ██████╗  █████╗ ██╗    ██╗██╗███╗   ██╗ ██████╗
██╔══██╗██╔══██╗██╔════╝██║██╔════╝    ██╔══██╗██╔══██╗██╔══██╗██║    ██║██║████╗  ██║██╔════╝
██████╔╝███████║███████╗██║██║         ██║  ██║██████╔╝███████║██║ █╗ ██║██║██╔██╗ ██║██║  ███╗
██╔══██╗██╔══██║╚════██║██║██║         ██║  ██║██╔══██╗██╔══██║██║███╗██║██║██║╚██╗██║██║   ██║
██████╔╝██║  ██║███████║██║╚██████╗    ██████╔╝██║  ██║██║  ██║╚███╔███╔╝██║██║ ╚████║╚██████╔╝
╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝ ╚═════╝    ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝ ╚══╝╚══╝ ╚═╝╚═╝  ╚═══╝ ╚═════╝
*/

void                                        jo_software_renderer_clear(jo_software_renderer_gfx * const gfx, const jo_color color)
{
    register jo_color                       *ptr;
    register jo_color                       *end;

    ptr = gfx->color_buffer;
    for (end = ptr + JO_DIV_BY_2(gfx->color_buffer_size); ptr < end; ++ptr)
        *ptr = color;
}

static __jo_force_inline bool               __jo_software_renderer_pixel_clipping(const jo_software_renderer_gfx * const gfx, const jo_fixed x, const jo_fixed y)
{
    return (x >= 0 && y >= 0 && x < jo_int2fixed(gfx->clipping_size.width) && y < jo_int2fixed(gfx->clipping_size.height));
}

static __jo_force_inline void               __jo_software_renderer_plot_pixel(jo_software_renderer_gfx * const gfx, const jo_fixed x, const jo_fixed y, const jo_color color)
{
    if (!__jo_software_renderer_pixel_clipping(gfx, x, y))
        return;
    gfx->color_buffer[jo_fixed2int(x) + jo_fixed2int(y) * gfx->vram_size.width] = color;
}

void                                        jo_software_renderer_draw_pixel2D(jo_software_renderer_gfx * const gfx, const jo_fixed x, const jo_fixed y, const jo_color color)
{
    __jo_software_renderer_plot_pixel(gfx, x, y, color);
}

static __jo_force_inline void               __jo_software_renderer_draw_pixel(jo_software_renderer_gfx * const gfx, const jo_fixed x, const jo_fixed y, const jo_fixed z, const jo_color color)
{
    unsigned int                            idx;

    if (!__jo_software_renderer_pixel_clipping(gfx, x, y))
        return;
    idx = jo_fixed2int(x) + jo_fixed2int(y) * gfx->vram_size.width;
    switch (gfx->depth_mode_testing)
    {
        case JO_SR_DEPTH_LESS: if (z >= gfx->depth_buffer[idx]) return ; break;
        case JO_SR_DEPTH_LESS_OR_EQUAL: if (z > gfx->depth_buffer[idx]) return ; break;
        case JO_SR_DEPTH_GREATER_OR_EQUAL: if (z < gfx->depth_buffer[idx]) return ; break;
        case JO_SR_DEPTH_GREATER: if (z <= gfx->depth_buffer[idx]) return ; break;
        case JO_SR_DEPTH_NOT_EQUAL: if (z == gfx->depth_buffer[idx]) return ; break;
        case JO_SR_DEPTH_EQUAL: if (z != gfx->depth_buffer[idx]) return ; break;
        default:
            break;
    }
    gfx->color_buffer[idx] = color;
    gfx->depth_buffer[idx] = z;
}

void                                        jo_software_renderer_draw_pixel3D(jo_software_renderer_gfx * const gfx, const jo_fixed x, const jo_fixed y, const jo_fixed z, const jo_color color)
{
    __jo_software_renderer_draw_pixel(gfx, x, y, z, color);
}

void                                        jo_software_renderer_draw_line3D(jo_software_renderer_gfx * const gfx,
                                                                           jo_fixed x0, jo_fixed y0, jo_fixed z0,
                                                                           jo_fixed x1, jo_fixed y1, jo_fixed z1,
                                                                           const jo_color color0, const jo_color color1)
{
    jo_color c = color0;
    jo_fixed x = x1 - x0;
    jo_fixed y = y1 - y0;
    jo_fixed ax = JO_ABS(x), ay = JO_ABS(y);
    jo_fixed dx1 = x < JO_FIXED_0 ? -JO_FIXED_1 : x > JO_FIXED_0 ? JO_FIXED_1 : JO_FIXED_0;
    jo_fixed dy1 = y < JO_FIXED_0 ? -JO_FIXED_1 : y > JO_FIXED_0 ? JO_FIXED_1 : JO_FIXED_0;
    jo_fixed dx2 = dx1, dy2 = JO_FIXED_0;
    jo_fixed n, i = JO_FIXED_0;
    if (ax <= ay)
    {
        JO_SWAP(ax, ay);
        JO_ZERO(dx2);
        dy2 = dy1;
    }
    jo_fixed incr_t = jo_fixed_div(JO_FIXED_1, ax);
    jo_fixed t = JO_FIXED_0;
    for (n = JO_DIV_BY_2(ax); i <= ax; i += JO_FIXED_1)
    {
        if (color0 != color1)
        {
            c = JO_COLOR_SATURN_RGB(jo_fixed2int(jo_lerp(jo_int2fixed(JO_COLOR_SATURN_GET_R(color0)), jo_int2fixed(JO_COLOR_SATURN_GET_R(color1)), t)),
                             jo_fixed2int(jo_lerp(jo_int2fixed(JO_COLOR_SATURN_GET_G(color0)), jo_int2fixed(JO_COLOR_SATURN_GET_G(color1)), t)),
                             jo_fixed2int(jo_lerp(jo_int2fixed(JO_COLOR_SATURN_GET_B(color0)), jo_int2fixed(JO_COLOR_SATURN_GET_B(color1)), t)));
        }
        __jo_software_renderer_draw_pixel(gfx, x0, y0, jo_lerp(z0, z1, t), c);
        n += ay;
        x0 += n >= ax ? dx1 : dx2;
        y0 += n >= ax ? dy1 : dy2;
        n -= n >= ax ? ax : JO_FIXED_0;
        t += incr_t;
    }
}

/*
████████╗██████╗ ██╗ █████╗ ███╗   ██╗ ██████╗ ██╗     ███████╗
╚══██╔══╝██╔══██╗██║██╔══██╗████╗  ██║██╔════╝ ██║     ██╔════╝
   ██║   ██████╔╝██║███████║██╔██╗ ██║██║  ███╗██║     █████╗
   ██║   ██╔══██╗██║██╔══██║██║╚██╗██║██║   ██║██║     ██╔══╝
   ██║   ██║  ██║██║██║  ██║██║ ╚████║╚██████╔╝███████╗███████╗
   ╚═╝   ╚═╝  ╚═╝╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚══════╝╚══════╝
*/

#endif

/*
** END OF FILE
*/
