/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2024, Johannes Fetz (johannesfetz@gmail.com)
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
typedef enum
{
    JO_TRIANGLE_FLAT_BOTTOM,
    JO_TRIANGLE_FLAT_TOP
}                                           __jo_triangle_type;

static __jo_force_inline void               __jo_software_renderer_clear_zbuffer(jo_software_renderer_gfx * const gfx)
{
    register jo_fixed                       *ptr;
    register jo_fixed                       *end;

    ptr = gfx->depth_buffer;
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

jo_software_renderer_gfx                    *jo_software_renderer_create_ex(unsigned short width, unsigned short height, const jo_scroll_screen screen, const bool enable_zbuffer)
{
    jo_img                                  img;
    jo_software_renderer_gfx                *gfx;

    gfx = (jo_software_renderer_gfx *)jo_malloc(sizeof(*gfx));
    if (gfx == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory (GFX)");
#endif
        return (JO_NULL);
    }
    img.height = height;
    img.width = width;
    img.data = JO_NULL;
    gfx->sprite_id = -1;
    gfx->face_culling_mode = JO_SR_NO_FACE_CULLING;
    gfx->draw_mode = JO_SR_DRAW_WIREFRAME;
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
        jo_core_error("Out of memory (Color Buffer)");
#endif
        return (JO_NULL);
    }
    if (enable_zbuffer)
    {
        gfx->depth_mode_testing = JO_SR_DEPTH_GREATER_OR_EQUAL;
        gfx->depth_buffer_dword_size = (sizeof(*gfx->depth_buffer) * gfx->vram_size.width * height) / sizeof(jo_fixed);
        gfx->depth_buffer = jo_malloc(gfx->depth_buffer_dword_size * sizeof(*gfx->depth_buffer));
        if (gfx->depth_buffer == JO_NULL)
        {
            jo_software_renderer_free(gfx);
    #ifdef JO_DEBUG
            jo_core_error("Out of memory (Z-buffer)");
    #endif
            return (JO_NULL);
        }
        __jo_software_renderer_clear_zbuffer(gfx);
    }
    else
    {
        JO_ZERO(gfx->depth_buffer_dword_size);
        gfx->depth_buffer = JO_NULL;
        gfx->depth_mode_testing = JO_SR_DEPTH_IGNORE;
    }
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
    if (gfx->depth_buffer != JO_NULL)
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

void                                        jo_software_renderer_clear(const jo_software_renderer_gfx * const gfx, const jo_color color)
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

static __jo_force_inline void               __jo_software_renderer_plot_pixel(const jo_software_renderer_gfx * const gfx, const jo_fixed x, const jo_fixed y, const jo_color color)
{
    if (!__jo_software_renderer_pixel_clipping(gfx, x, y))
        return;
    gfx->color_buffer[jo_fixed2int(x) + jo_fixed2int(y) * gfx->vram_size.width] = color;
}

void                                        jo_software_renderer_draw_pixel2D(const jo_software_renderer_gfx * const gfx, const jo_fixed x, const jo_fixed y, const jo_color color)
{
    __jo_software_renderer_plot_pixel(gfx, x, y, color);
}

static __jo_force_inline void               __jo_software_renderer_draw_pixel(const jo_software_renderer_gfx * const gfx, const jo_fixed x, const jo_fixed y, const jo_fixed z, const jo_color color)
{
    unsigned int                            idx;

    if (!__jo_software_renderer_pixel_clipping(gfx, x, y))
        return;
    idx = jo_fixed2int(x) + jo_fixed2int(y) * gfx->vram_size.width;
    if (gfx->depth_buffer == JO_NULL)
    {
        gfx->color_buffer[idx] = color;
        return;
    }
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

void                                        jo_software_renderer_draw_pixel3D(const jo_software_renderer_gfx * const gfx, const jo_fixed x, const jo_fixed y, const jo_fixed z, const jo_color color)
{
    __jo_software_renderer_draw_pixel(gfx, x, y, z, color);
}

void                                        jo_software_renderer_draw_line3D(const jo_software_renderer_gfx * const gfx,
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

static __jo_force_inline bool               jo_is_triangle_degenerated(const jo_vector4_fixed * const p0,
                                                                       const jo_vector4_fixed * const p1,
                                                                       const jo_vector4_fixed * const p2)
{
    return ((p0->x == p1->x && p0->x == p2->x) || (p0->y == p1->y && p0->y == p2->y));
}

static __jo_force_inline bool               jo_is_triangle_offscreen(const jo_vector4_fixed * const p0,
                                                                     const jo_vector4_fixed * const p1,
                                                                     const jo_vector4_fixed * const p2)
{
    if ((p0->x < -p0->w && p1->x < -p1->w && p2->x < -p2->w) ||
        (p0->x > p0->w && p1->x > p1->w && p2->x > p2->w))
            return (true);
    if ((p0->y < -p0->w && p1->y < -p1->w && p2->y < -p2->w) ||
        (p0->y > p0->w && p1->y > p1->w && p2->y > p2->w))
            return (true);
    if ((p0->z < 0 && p1->z <0 && p2->z < 0) ||
        (p0->z > p0->w && p1->z > p1->w && p2->z > p2->w))
            return (true);
    return (false);
}

static __jo_force_inline bool               jo_is_triangle_face_culled(const jo_software_renderer_face_culling_mode face_culling_mode,
                                                                       const jo_vector4_fixed * const p0,
                                                                       const jo_vector4_fixed * const p1,
                                                                       const jo_vector4_fixed * const p2)
{
    jo_vector4_fixed                        d1;
    jo_vector4_fixed                        d2;
    jo_vector4_fixed                        n;
    jo_fixed                                dp;

    jo_vector4_fixed_sub(p1, p0, &d1);
    jo_vector4_fixed_sub(p2, p0, &d2);
    jo_vector4_fixed_cross(&d1, &d2, &n);
    dp = jo_vector4_fixed_dot(p0, &n);
    switch (face_culling_mode)
    {
        case JO_SR_BACK_FACE_CULLING:
            return (dp < 0);
        case JO_SR_FRONT_FACE_CULLING:
            return (dp >= 0);
        default:
            return false;
    }
}

static __jo_force_inline void               jo_transform_to_surface_coord(const jo_software_renderer_gfx * const gfx,
                                                                         jo_vector4_fixed * const p0,
                                                                         jo_vector4_fixed * const p1,
                                                                         jo_vector4_fixed * const p2)
{
    jo_fixed                                width;
    jo_fixed                                height;

    width = jo_int2fixed(gfx->clipping_size.width);
    height = jo_int2fixed(gfx->clipping_size.height);

    p0->x = jo_fixed_div(jo_fixed_mult(p0->x, width), jo_fixed_mult(JO_FIXED_2, p0->w)) + jo_fixed_div(width, JO_FIXED_2);
    p0->y = jo_fixed_div(jo_fixed_mult(p0->y, height), jo_fixed_mult(JO_FIXED_2, p0->w)) + jo_fixed_div(height, JO_FIXED_2);

    p1->x = jo_fixed_div(jo_fixed_mult(p1->x, width), jo_fixed_mult(JO_FIXED_2, p1->w)) + jo_fixed_div(width, JO_FIXED_2);
    p1->y = jo_fixed_div(jo_fixed_mult(p1->y, height), jo_fixed_mult(JO_FIXED_2, p1->w)) + jo_fixed_div(height, JO_FIXED_2);

    p2->x = jo_fixed_div(jo_fixed_mult(p2->x, width), jo_fixed_mult(JO_FIXED_2, p2->w)) + jo_fixed_div(width, JO_FIXED_2);
    p2->y = jo_fixed_div(jo_fixed_mult(p2->y, height), jo_fixed_mult(JO_FIXED_2, p2->w)) + jo_fixed_div(height, JO_FIXED_2);
}

static __jo_force_inline void               jo_swap_vertex(jo_software_renderer_vertex * const a, jo_software_renderer_vertex * const b)
{
    JO_SWAP(a->color, b->color);
    JO_SWAP(a->uv_texture_mapping.x, b->uv_texture_mapping.x);
    JO_SWAP(a->uv_texture_mapping.y, b->uv_texture_mapping.y);
    jo_vector4_swap(&a->pos, &b->pos);
}

static __jo_force_inline void               __jo_software_renderer_draw_textured_triangle(const jo_software_renderer_gfx * const gfx,
                                                                                      const jo_software_renderer_triangle * const triangle,
                                                                                      const __jo_triangle_type triangle_type)
{
    JO_UNUSED_ARG(gfx);
    JO_UNUSED_ARG(triangle);
    JO_UNUSED_ARG(triangle_type);
    jo_core_error("Texture mapped triangle is not implemented yet");
}

static __jo_force_inline void               __jo_software_renderer_draw_fill_triangle(const jo_software_renderer_gfx * const gfx,
                                                                                      const jo_software_renderer_triangle * const triangle,
                                                                                      const __jo_triangle_type triangle_type)
{
    jo_fixed y, invDy, dxLeft, dxRight, xLeft, xRight, prestep, numScanlines, yDir;
    jo_fixed startInvZ, endInvZ, invZ0, invZ1, invZ2, invY02, currLine, x0, x1, r1;

    if (triangle_type == JO_TRIANGLE_FLAT_BOTTOM)
    {
        if ((triangle->v2.pos.y - triangle->v0.pos.y) < JO_FIXED_1)
            return ;
        invDy = jo_fixed_div(JO_FIXED_1, (triangle->v2.pos.y - triangle->v0.pos.y));
        yDir = JO_FIXED_1;
        numScanlines = jo_fixed_ceil(triangle->v2.pos.y) - jo_fixed_ceil(triangle->v0.pos.y);
        prestep = jo_fixed_ceil(triangle->v0.pos.y) - triangle->v0.pos.y;
    }
    else
    {
        if ((triangle->v0.pos.y - triangle->v2.pos.y) < JO_FIXED_1)
            return ;
        invDy = jo_fixed_div(JO_FIXED_1, (triangle->v0.pos.y - triangle->v2.pos.y));
        yDir = -JO_FIXED_1;
        numScanlines = jo_fixed_ceil(triangle->v0.pos.y) - jo_fixed_ceil(triangle->v2.pos.y);
        prestep = jo_fixed_ceil(triangle->v2.pos.y) - triangle->v2.pos.y;
    }
    dxLeft = jo_fixed_mult((triangle->v2.pos.x - triangle->v0.pos.x), invDy);
    dxRight = jo_fixed_mult((triangle->v1.pos.x - triangle->v0.pos.x), invDy);
    xLeft = triangle->v0.pos.x + jo_fixed_mult(dxLeft, prestep);
    xRight = triangle->v0.pos.x + jo_fixed_mult(dxRight, prestep);

    if (gfx->depth_mode_testing != JO_SR_DEPTH_IGNORE)
    {
        invZ0 = jo_fixed_div(JO_FIXED_1, triangle->v0.pos.z);
        invZ1 = jo_fixed_div(JO_FIXED_1, triangle->v1.pos.z);
        invZ2 = jo_fixed_div(JO_FIXED_1, triangle->v2.pos.z);
        invY02 = jo_fixed_div(JO_FIXED_1, (triangle->v0.pos.y - triangle->v2.pos.y));
        for (JO_ZERO(currLine), y = jo_fixed_ceil(triangle->v0.pos.y); currLine <= numScanlines; y += yDir)
        {
            x0 = jo_fixed_ceil(xLeft);
            x1 = jo_fixed_ceil(xRight);
            r1 = jo_fixed_mult(triangle->v0.pos.y - y, invY02);
            startInvZ = jo_lerp(invZ0, invZ2, r1);
            endInvZ = jo_lerp(invZ0, invZ1, r1);
            // TODO use all vertex colors
            jo_software_renderer_draw_line3D(gfx, x0, y, jo_fixed_div(JO_FIXED_1, startInvZ), x1, y, jo_fixed_div(JO_FIXED_1, endInvZ), triangle->v0.color, triangle->v1.color);
            currLine += JO_FIXED_ALMOST_1;
            if (currLine < numScanlines)
            {
                xLeft  += dxLeft;
                xRight += dxRight;
            }
        }
    }
    else
    {
        for (JO_ZERO(currLine), y = jo_fixed_ceil(triangle->v0.pos.y); currLine <= numScanlines; y += yDir)
        {
            // TODO use all vertex colors
            jo_software_renderer_draw_line3D(gfx, jo_fixed_ceil(xLeft), y, JO_FIXED_1, jo_fixed_ceil(xRight), y, JO_FIXED_1, triangle->v0.color, triangle->v1.color);
            currLine += JO_FIXED_ALMOST_1;
            if (currLine < numScanlines)
            {
                xLeft  += dxLeft;
                xRight += dxRight;
            }
        }
    }
}

static __jo_force_inline void               __jo_software_renderer_draw_triangle_type(const jo_software_renderer_gfx * const gfx,
                                                                                      const jo_software_renderer_triangle * const triangle,
                                                                                      const __jo_triangle_type triangle_type)
{
    switch (gfx->draw_mode)
    {
        case JO_SR_DRAW_TEXTURED:
            __jo_software_renderer_draw_textured_triangle(gfx, triangle, triangle_type);
            return ;
        case JO_SR_DRAW_FLAT:
        default:
            __jo_software_renderer_draw_fill_triangle(gfx, triangle, triangle_type);
            return;
    }
}

static __jo_force_inline void               __jo_software_renderer_draw_regular_triangle(const jo_software_renderer_gfx * const gfx,
                                                                                       jo_software_renderer_triangle * triangle)
{
    jo_software_renderer_vertex             v3;
    jo_software_renderer_vertex             original_v1;
    jo_vector4_fixed                        diff;
    jo_vector4_fixed                        diff2;
    jo_fixed                                ratioU;
    jo_fixed                                ratioV;
    jo_fixed                                invV0Z;
    jo_fixed                                invV1Z;

    v3 = triangle->v2;
    v3.pos.x = triangle->v0.pos.x + jo_fixed_div(jo_fixed_mult((triangle->v1.pos.x - triangle->v0.pos.x), (triangle->v2.pos.y - triangle->v0.pos.y)), (triangle->v1.pos.y - triangle->v0.pos.y));
    JO_ZERO(v3.pos.z);
    jo_vector4_fixed_sub(&triangle->v1.pos, &triangle->v0.pos, &diff);
    jo_vector4_fixed_sub(&v3.pos, &triangle->v0.pos, &diff2);
    ratioU = diff.x != 0 ? jo_fixed_div(diff2.x, diff.x) : JO_FIXED_1;
    ratioV = diff.y != 0 ? jo_fixed_div(diff2.y, diff.y) : JO_FIXED_1;

    if (gfx->draw_mode == JO_SR_DRAW_TEXTURED || gfx->depth_mode_testing != JO_SR_DEPTH_IGNORE)
    {
        invV0Z = jo_fixed_div(JO_FIXED_1, triangle->v0.pos.z);
        invV1Z = jo_fixed_div(JO_FIXED_1, triangle->v1.pos.z);

        if (triangle->v0.pos.x - triangle->v1.pos.x)
            v3.pos.z = jo_fixed_div(JO_FIXED_1, jo_lerp(invV1Z, invV0Z, jo_fixed_div((v3.pos.x - triangle->v1.pos.x), (triangle->v0.pos.x - triangle->v1.pos.x))));
        else
            v3.pos.z = triangle->v0.pos.z;

        if (gfx->draw_mode == JO_SR_DRAW_TEXTURED)
        {
            v3.uv_texture_mapping.x = jo_fixed_mult(v3.pos.z, jo_lerp(jo_fixed_mult(triangle->v0.uv_texture_mapping.x, invV0Z), jo_fixed_mult(triangle->v1.uv_texture_mapping.x, invV1Z), ratioU));
            v3.uv_texture_mapping.y = jo_fixed_mult(v3.pos.z, jo_lerp(jo_fixed_mult(triangle->v0.uv_texture_mapping.y, invV0Z), jo_fixed_mult(triangle->v1.uv_texture_mapping.y, invV1Z), ratioV));
        }
    }
    if (v3.pos.x < triangle->v2.pos.x)
        jo_swap_vertex(&v3, &triangle->v2);
    original_v1 = triangle->v1;
    if (!jo_is_triangle_degenerated(&triangle->v0.pos, &v3.pos, &triangle->v2.pos))
    {
        triangle->v1 = v3;
        __jo_software_renderer_draw_triangle_type(gfx, triangle, JO_TRIANGLE_FLAT_BOTTOM);
    }
    if (!jo_is_triangle_degenerated(&original_v1.pos, &v3.pos, &triangle->v2.pos))
    {
        triangle->v0 = original_v1;
        triangle->v1 = v3;
        __jo_software_renderer_draw_triangle_type(gfx, triangle, JO_TRIANGLE_FLAT_TOP);
    }
}

void                                        jo_software_renderer_draw_triangle(const jo_software_renderer_gfx * const gfx,
                                                                               const jo_software_renderer_triangle * const triangle,
                                                                               const jo_matrix * const transform_matrix)
{
    jo_software_renderer_triangle           result;

    result.sprite_id = triangle->sprite_id;
    result.v0.color = triangle->v0.color;
    result.v1.color = triangle->v1.color;
    result.v2.color = triangle->v2.color;
    result.v0.uv_texture_mapping = triangle->v0.uv_texture_mapping;
    result.v1.uv_texture_mapping = triangle->v1.uv_texture_mapping;
    result.v2.uv_texture_mapping = triangle->v2.uv_texture_mapping;

    jo_matrix_mul_vector4(transform_matrix, &triangle->v0.pos, &result.v0.pos);
    jo_matrix_mul_vector4(transform_matrix, &triangle->v1.pos, &result.v1.pos);
    jo_matrix_mul_vector4(transform_matrix, &triangle->v2.pos, &result.v2.pos);

    if (jo_is_triangle_offscreen(&result.v0.pos, &result.v1.pos, &result.v2.pos))
        return ;
    if (gfx->face_culling_mode != JO_SR_NO_FACE_CULLING && jo_is_triangle_face_culled(gfx->face_culling_mode, &result.v0.pos, &result.v1.pos, &result.v2.pos))
        return ;
    jo_transform_to_surface_coord(gfx, &result.v0.pos, &result.v1.pos, &result.v2.pos);

    if (result.v2.pos.y > result.v1.pos.y)
        jo_swap_vertex(&result.v1, &result.v2);
    if (result.v0.pos.y > result.v1.pos.y)
        jo_swap_vertex(&result.v0, &result.v1);
    if (result.v0.pos.y > result.v2.pos.y)
        jo_swap_vertex(&result.v0, &result.v2);

    if (jo_is_triangle_degenerated(&result.v0.pos, &result.v1.pos, &result.v2.pos))
        return ;
    if (gfx->draw_mode & JO_SR_DRAW_WIREFRAME)
    {
        jo_software_renderer_draw_triangle_wireframe(gfx, &result);
        return ;
    }
    __jo_software_renderer_draw_regular_triangle(gfx, &result);
}

#endif

/*
** END OF FILE
*/
