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

#ifdef JO_COMPILE_WITH_SOFTWARE_RENDERER_SUPPORT

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
#include "jo/colors.h"
#include "jo/vdp1_command_pipeline.h"
#include "jo/sprites.h"
#include "jo/fs.h"
#include "jo/math.h"
#include "jo/list.h"
#include "jo/vdp2.h"
#include "jo/vdp2_malloc.h"
#include "jo/software_renderer.h"
#include "internal/sprites.h"

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

/** @brief Forget the textures copied in RAM (see jo_sr_get_texture()): at each flush, the sprites can change (their
 *  blocks are kept for the next frame)
 *  @param gfx Surface
 *  @param release The blocks freed too
 */
static void                                 jo_sr_forget_textures(jo_software_renderer_gfx * const gfx, const bool release)
{
    int                                     k;

    for (JO_ZERO(k); k < 4; ++k)
    {
        gfx->__texture_cache_id[k] = -1;
        if (!release || gfx->__texture_cache[k] == JO_NULL)
            continue;
        jo_free(gfx->__texture_cache[k]);
        gfx->__texture_cache[k] = JO_NULL;
        JO_ZERO(gfx->__texture_cache_size[k]);
    }
}

static __jo_force_inline void               __jo_software_renderer_clear_zbuffer(jo_software_renderer_gfx * const gfx)
{
    register jo_fixed                       *ptr;
    register jo_fixed                       *end;

    ptr = gfx->depth_buffer;
    end = ptr + gfx->depth_buffer_dword_size;
    // 4 values per iteration
    while (ptr + 4 <= end)
    {
        ptr[0] = JO_FIXED_MIN;
        ptr[1] = JO_FIXED_MIN;
        ptr[2] = JO_FIXED_MIN;
        ptr[3] = JO_FIXED_MIN;
        ptr += 4;
    }
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
    jo_sr_forget_textures(gfx, true);
    if (gfx->color_buffer != JO_NULL)
        jo_free(gfx->color_buffer);
    if (gfx->depth_buffer != JO_NULL)
        jo_free(gfx->depth_buffer);
    // The sprite can only be freed if it's the last one (see jo_sprite_free_from())
    if (gfx->sprite_id >= 0 && gfx->sprite_id == jo_sprite_count() - 1)
        jo_sprite_free_from(gfx->sprite_id);
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
    // jo_software_renderer_free() can be called on error
    gfx->color_buffer = JO_NULL;
    gfx->depth_buffer = JO_NULL;
    for (gfx->__texture_cache_next = 0; gfx->__texture_cache_next < 4; ++gfx->__texture_cache_next)
    {
        gfx->__texture_cache[gfx->__texture_cache_next] = JO_NULL;
        JO_ZERO(gfx->__texture_cache_size[gfx->__texture_cache_next]);
        gfx->__texture_cache_id[gfx->__texture_cache_next] = -1;
    }
    JO_ZERO(gfx->__texture_cache_next);
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
            // (JO_NULL: the bitmap can't be allocated, see __jo_vdp2_reserve_nbg1_bitmap())
            if (nbg1_bitmap == JO_NULL)
            {
                jo_software_renderer_free(gfx);
                return (JO_NULL);
            }
            gfx->vram = nbg1_bitmap;
            gfx->vram_size.width = JO_VDP2_WIDTH;
            gfx->vram_size.height = JO_VDP2_HEIGHT;
            // (in the bitmap: a renderer of 704 pixels or 480 lines wrote after the buffer and after the bitmap)
            gfx->clipping_size.width = JO_MIN(width, JO_VDP2_WIDTH);
            gfx->clipping_size.height = JO_MIN(height, JO_VDP2_HEIGHT);
            height = gfx->clipping_size.height;
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
    jo_sr_forget_textures(gfx, false);
    if (gfx->depth_buffer == JO_NULL)
    {
        jo_dma_copy(gfx->color_buffer, gfx->vram, gfx->color_buffer_size);
        return;
    }
    // The depth buffer is cleared during the copy (the DMA controller steals cycles: the CPU keeps working). The
    // destination is in the VRAM (cache-through): no cache line to purge after the transfer
    jo_dma_copy_async(gfx->color_buffer, gfx->vram, gfx->color_buffer_size);
    __jo_software_renderer_clear_zbuffer(gfx);
    jo_dma_wait();
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
    register unsigned int                   *ptr32;
    register unsigned int                   *end32;
    unsigned int                            color32;

    ptr = gfx->color_buffer;
    end = ptr + JO_DIV_BY_2(gfx->color_buffer_size);
    // 2 pixels per 32 bits write
    if (((unsigned int)ptr & 2) && ptr < end)
        *ptr++ = color;
    color32 = ((unsigned int)color << 16) | color;
    ptr32 = (unsigned int *)ptr;
    end32 = ptr32 + JO_DIV_BY_2(end - ptr);
    while (ptr32 + 4 <= end32)
    {
        ptr32[0] = color32;
        ptr32[1] = color32;
        ptr32[2] = color32;
        ptr32[3] = color32;
        ptr32 += 4;
    }
    while (ptr32 < end32)
        *ptr32++ = color32;
    ptr = (jo_color *)ptr32;
    if (ptr < end)
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
    const bool gradient = color0 != color1;
    const bool depth = gfx->depth_buffer != JO_NULL;
    // Color gradient: jo_lerp(C0 << 16, C1 << 16, t) is exactly (C0 << 16) + t * (C1 - C0) (no rounding), so the components
    // are 32 bits accumulators instead of 6 multiplications per pixel
    jo_fixed r = jo_int2fixed(JO_COLOR_SATURN_GET_R(color0));
    jo_fixed g = jo_int2fixed(JO_COLOR_SATURN_GET_G(color0));
    jo_fixed b = jo_int2fixed(JO_COLOR_SATURN_GET_B(color0));
    const jo_fixed incr_r = incr_t * (JO_COLOR_SATURN_GET_R(color1) - JO_COLOR_SATURN_GET_R(color0));
    const jo_fixed incr_g = incr_t * (JO_COLOR_SATURN_GET_G(color1) - JO_COLOR_SATURN_GET_G(color0));
    const jo_fixed incr_b = incr_t * (JO_COLOR_SATURN_GET_B(color1) - JO_COLOR_SATURN_GET_B(color0));
    for (n = JO_DIV_BY_2(ax); i <= ax; i += JO_FIXED_1)
    {
        if (gradient)
        {
            c = JO_COLOR_SATURN_RGB(jo_fixed2int(r), jo_fixed2int(g), jo_fixed2int(b));
            r += incr_r;
            g += incr_g;
            b += incr_b;
        }
        // The depth is only computed with a depth buffer
        if (depth)
            __jo_software_renderer_draw_pixel(gfx, x0, y0, jo_lerp(z0, z1, t), c);
        else
            __jo_software_renderer_plot_pixel(gfx, x0, y0, c);
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

/** @brief Smallest depth (w) of a vertex drawn (no clipping on the near plane) */
#define JO_SR_NEAR_W                        (JO_FIXED_1 / 16)
/** @brief Largest coordinate of a vertex on the surface (pixels, fixed point): 4 times the largest surface */
#define JO_SR_GUARD_BAND                    (jo_int2fixed(2048))

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

    // (x / w first, from -1 to 1 on the screen: x * width overflowed for |x| > 102 with a width of 320)
    width = JO_DIV_BY_2(width);
    height = JO_DIV_BY_2(height);
    p0->x = jo_fixed_mult(jo_fixed_div(p0->x, p0->w), width) + width;
    p0->y = jo_fixed_mult(jo_fixed_div(p0->y, p0->w), height) + height;

    p1->x = jo_fixed_mult(jo_fixed_div(p1->x, p1->w), width) + width;
    p1->y = jo_fixed_mult(jo_fixed_div(p1->y, p1->w), height) + height;

    p2->x = jo_fixed_mult(jo_fixed_div(p2->x, p2->w), width) + width;
    p2->y = jo_fixed_mult(jo_fixed_div(p2->y, p2->w), height) + height;
}

static __jo_force_inline void               jo_swap_vertex(jo_software_renderer_vertex * const a, jo_software_renderer_vertex * const b)
{
    JO_SWAP(a->color, b->color);
    JO_SWAP(a->uv_texture_mapping.x, b->uv_texture_mapping.x);
    JO_SWAP(a->uv_texture_mapping.y, b->uv_texture_mapping.y);
    jo_vector4_swap(&a->pos, &b->pos);
}

/*
** Triangles: one pass over the rows of the triangle (no split in 2 triangles), the attributes interpolated from their
** gradients on the screen: the depth (z / w), the color of each vertex (gouraud) and the texture coordinates (u / w,
** v / w and 1 / w, perspective correct every JO_SR_SPAN pixels, linear between)
*/

/** @brief The attributes of a vertex interpolated on the triangle */
enum
{
    JO_SR_DEPTH = 0,        // z / w (Q16)
    JO_SR_RED,              // the components of the color (Q16, 0 to 31)
    JO_SR_GREEN,
    JO_SR_BLUE,
    JO_SR_U,                // u / w (Q16 texels, see JO_SR_INVERSE_W_SHIFT)
    JO_SR_V,
    JO_SR_INVERSE_W,        // 2^24 / w
    JO_SR_ATTRIBUTES
};

/** @brief 1 / w in 2^JO_SR_INVERSE_W_SHIFT / w: w is at least JO_SR_NEAR_W (1/16), 1 / w at most 2^28 */
#define JO_SR_INVERSE_W_SHIFT               (24)
/** @brief The texture coordinates divided exactly every JO_SR_SPAN pixels of a row (linear between: a division per
 *  segment instead of one per pixel) */
#define JO_SR_SPAN                          (16)
#define JO_SR_SPAN_SHIFT                    (4)

/** @brief A vertex on the surface: its position (pixels, Q16) and its attributes */
typedef struct
{
    jo_fixed                                x;
    jo_fixed                                y;
    int                                     a[JO_SR_ATTRIBUTES];
}                                           jo_sr_point;

/** @brief The texture of a triangle (texels in the VRAM of the VDP1: RGB or 16 colors with a table) */
typedef struct
{
    const jo_color                          *texels;        // The texels copied in RAM (JO_NULL: read in the VRAM)
    const unsigned short                    *rgb;           // RGB texels (JO_NULL: 16 colors)
    const unsigned char                     *nibbles;       // 16 colors: 2 texels per byte, the left one in the high bits
    jo_color                                lut[16];        // 16 colors: the table (0: transparent)
    int                                     width;
    int                                     height;
    int                                     width_mask;     // width - 1 for a power of 2, -1: the coordinates are wrapped by a modulo
    int                                     height_mask;
}                                           jo_sr_texture;

/** @brief Signed 64 bits / 32 bits by the division unit of the CPU (the quotient must fit in 32 bits)
 *  @param high High word of the dividend
 *  @param low Low word of the dividend
 *  @param divisor Divisor (not 0)
 *  @return The quotient
 */
static __jo_force_inline int                jo_sr_div64(const int high, const unsigned int low, const int divisor)
{
    JO_SH2_DVSR = divisor;
    JO_SH2_DVDNTH = high;
    JO_SH2_DVDNTL = (int)low;
    return (JO_SH2_DVDNTL);
}

/** @brief w (Q16) from 2^24 / w */
static __jo_force_inline jo_fixed           jo_sr_w(const int inverse_w)
{
    // (2^40 / (2^24 / w) = w x 2^16)
    return (inverse_w > 0 ? jo_sr_div64(1 << (40 - 32), 0, inverse_w) : JO_FIXED_MAX);
}

/** @brief A texel (repeated: the coordinates wrap around the texture)
 *  @param t Texture
 *  @param u Column (Q16 texels)
 *  @param v Row (Q16 texels)
 *  @return Its color (0: transparent)
 */
static __jo_force_inline jo_color           jo_sr_texel(const jo_sr_texture * const t, const jo_fixed u, const jo_fixed v)
{
    int                                     x;
    int                                     y;
    unsigned int                            b;

    x = jo_fixed2int(u);
    y = jo_fixed2int(v);
    if (t->width_mask >= 0)
        x &= t->width_mask;
    else if ((x %= t->width) < 0)
        x += t->width;
    if (t->height_mask >= 0)
        y &= t->height_mask;
    else if ((y %= t->height) < 0)
        y += t->height;
    if (t->texels != JO_NULL)
        return (t->texels[y * t->width + x]);
    if (t->rgb != JO_NULL)
        return (t->rgb[y * t->width + x]);
    b = t->nibbles[JO_DIV_BY_2(y * t->width + x)];
    return (t->lut[(x & 1) ? (b & 15) : (b >> 4)]);
}

#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
/** @brief Check if the code runs on the slave CPU (its stack is in the first 16 KB of the high work RAM, see
 *  __jo_profile_current_cpu()) */
static __jo_force_inline bool               jo_sr_on_slave_cpu(void)
{
    unsigned int                            sp;

    __asm__ volatile ("mov r15, %0" : "=r" (sp));
    return ((sp & 0x000FFFFF) < 0x4000);
}
#endif

/** @brief Copy the texels of a texture in RAM, in RGB (the CPU reads the VRAM of the VDP1 slowly: the textured pixels
 *  cost about twice as much). Kept until the next flush of the surface (4 textures, the oldest one replaced). On the
 *  slave CPU, only the blocks already there (jo_malloc() isn't shared by the 2 CPUs: the texels read in the VRAM)
 *  @param gfx Surface
 *  @param sprite_id Sprite
 *  @param t Texture (its texels in the VRAM; t->texels set, JO_NULL if there is no memory)
 */
static void                                 jo_sr_cache_texture(jo_software_renderer_gfx * const gfx, const int sprite_id, jo_sr_texture * const t)
{
    jo_color                                *texels;
    unsigned int                            n;
    unsigned int                            i;
    unsigned int                            b;
    int                                     k;

    for (JO_ZERO(k); k < 4; ++k)
        if (gfx->__texture_cache_id[k] == sprite_id)
        {
            t->texels = gfx->__texture_cache[k];
            return;
        }
    k = gfx->__texture_cache_next;
    gfx->__texture_cache_next = (k + 1) & 3;
    n = (unsigned int)(t->width * t->height);
    if (gfx->__texture_cache_size[k] < n)
    {
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
        if (jo_sr_on_slave_cpu())
        {
            t->texels = JO_NULL;
            return;
        }
#endif
        if (gfx->__texture_cache[k] != JO_NULL)
            jo_free(gfx->__texture_cache[k]);
        gfx->__texture_cache[k] = (jo_color *)jo_malloc(n * sizeof(jo_color));
        gfx->__texture_cache_size[k] = gfx->__texture_cache[k] != JO_NULL ? n : 0;
        gfx->__texture_cache_id[k] = -1;
        if (gfx->__texture_cache[k] == JO_NULL)
        {
            t->texels = JO_NULL;
            return;
        }
    }
    texels = gfx->__texture_cache[k];
    if (t->rgb != JO_NULL)
        jo_memcpy(texels, t->rgb, n * sizeof(jo_color));
    else
        for (JO_ZERO(i); i < n; i += 2)
        {
            b = t->nibbles[JO_DIV_BY_2(i)];
            texels[i] = t->lut[b >> 4];
            texels[i + 1] = t->lut[b & 15];
        }
    gfx->__texture_cache_id[k] = (short)sprite_id;
    t->texels = texels;
}

/** @brief The texture of a sprite (its texels in the VRAM of the VDP1, copied in RAM)
 *  @param gfx Surface
 *  @param sprite_id Sprite
 *  @param t Texture (output)
 *  @return false if the sprite can't be a texture (no sprite, colors of the VDP2 palettes)
 */
static bool                                 jo_sr_get_texture(jo_software_renderer_gfx * const gfx, const int sprite_id, jo_sr_texture * const t)
{
    const jo_texture_definition             *def;
    const jo_picture_definition             *pic;
    const unsigned short                    *lut;
    int                                     k;

    if (sprite_id < 0 || sprite_id >= JO_MAX_SPRITE)
        return (false);
    def = &__jo_sprite_def[sprite_id];
    pic = &__jo_sprite_pic[sprite_id];
    if (def->width == 0 || def->height == 0)
        return (false);
    t->width = def->width;
    t->height = def->height;
    t->width_mask = JO_IS_POW2(t->width) ? t->width - 1 : -1;
    t->height_mask = JO_IS_POW2(t->height) ? t->height - 1 : -1;
    t->rgb = JO_NULL;
    t->texels = JO_NULL;
    t->nibbles = JO_NULL;
    switch (pic->color_mode)
    {
        case JO_VDP1_TEXTURE_SIZE_SHIFT_32K_COLORS:
            t->rgb = (const unsigned short *)(JO_VDP1_VRAM + JO_MULT_BY_8(def->adr));
            jo_sr_cache_texture(gfx, sprite_id, t);
            return (true);
        case JO_VDP1_TEXTURE_SIZE_SHIFT_16_COLORS:
            // (a table of colors: the 16 colors copied once; without a table, the colors of the VDP2: not supported)
            if (__jo_sprite_lut[sprite_id] == 0)
                return (false);
            t->nibbles = (const unsigned char *)(JO_VDP1_VRAM + JO_MULT_BY_8(def->adr));
            lut = (const unsigned short *)(JO_VDP1_VRAM + JO_MULT_BY_8(__jo_sprite_lut[sprite_id]));
            for (JO_ZERO(k); k < 16; ++k)
                t->lut[k] = lut[k];
            t->lut[0] = JO_COLOR_Transparent;
            jo_sr_cache_texture(gfx, sprite_id, t);
            return (true);
        default:
            return (false);
    }
}

/** @brief A color component interpolated (Q16), between 0 and 31 (at the edges, the rounding of the gradients can go
 *  a little beyond the colors of the vertices: a component of -1 or 32 changed the other components) */
static __jo_force_inline int                jo_sr_component(const jo_fixed value)
{
    int                                     c;

    c = value >> 16;
    c &= ~(c >> 31);
    return (c > 31 ? 31 : c);
}

/** @brief The depth tests (see jo_software_renderer_depth_mode): bit 0 the pixel is drawn when its depth is smaller than
 *  the one of the buffer, bit 1 equal, bit 2 greater
 *  @param mode Depth test
 *  @return The bits
 */
static __jo_force_inline unsigned int       jo_sr_depth_mask(const jo_software_renderer_depth_mode mode)
{
    switch (mode)
    {
        case JO_SR_DEPTH_LESS: return (1);
        case JO_SR_DEPTH_LESS_OR_EQUAL: return (3);
        case JO_SR_DEPTH_GREATER_OR_EQUAL: return (6);
        case JO_SR_DEPTH_GREATER: return (4);
        case JO_SR_DEPTH_NOT_EQUAL: return (5);
        case JO_SR_DEPTH_EQUAL: return (2);
        default: return (7);
    }
}

/** @brief Check the depth of a pixel
 *  @param mask See jo_sr_depth_mask()
 *  @param z Its depth
 *  @param depth The depth in the buffer
 *  @return true if it is drawn
 */
static __jo_force_inline bool               jo_sr_depth_test(const unsigned int mask, const jo_fixed z, const jo_fixed depth)
{
    return ((mask >> (z < depth ? 0 : (z == depth ? 1 : 2))) & 1);
}

/** @brief The x of an edge on a row (Q16)
 *  @param p First vertex (the top)
 *  @param slope dx / dy (Q16)
 *  @param q Last vertex
 *  @param yc Center of the row (Q16)
 */
static __jo_force_inline jo_fixed           jo_sr_edge_x(const jo_sr_point * const p, const jo_fixed slope, const jo_sr_point * const q, const jo_fixed yc)
{
    jo_fixed                                x;

    x = p->x + jo_fixed_mult(slope, yc - p->y);
    // (between the ends of the edge: the slope of an edge almost flat is limited)
    return (JO_MIN(JO_MAX(x, JO_MIN(p->x, q->x)), JO_MAX(p->x, q->x)));
}

/** @brief dx / dy of an edge (Q16), limited to 16384 pixels per pixel */
static __jo_force_inline jo_fixed           jo_sr_edge_slope(const jo_sr_point * const p, const jo_sr_point * const q)
{
    jo_fixed                                dx;
    jo_fixed                                dy;

    dx = q->x - p->x;
    dy = q->y - p->y;
    if (dy <= 0)
        return (0);
    if ((long long)JO_ABS(dx) >= ((long long)dy << 14))
        return (dx < 0 ? -(1 << 30) : (1 << 30));
    return (jo_fixed_div(dx, dy));
}

/** @brief Fill a triangle on the surface
 *  @param gfx Surface
 *  @param p The 3 vertices on the surface
 *  @param texture Its texture (JO_NULL: the colors of its vertices)
 */
static void                                 jo_sr_fill_triangle(const jo_software_renderer_gfx * const gfx, jo_sr_point p[3],
                                                                const jo_sr_texture * const texture)
{
    jo_sr_point                             *a;
    jo_sr_point                             *b;
    jo_sr_point                             *c;
    jo_sr_point                             *tmp;
    jo_fixed                                d[JO_SR_ATTRIBUTES][2];
    jo_fixed                                v[JO_SR_ATTRIBUTES];
    long long                               det;
    long long                               n;
    int                                     dx1;
    int                                     dy1;
    int                                     dx2;
    int                                     dy2;
    int                                     k;
    int                                     first;
    int                                     count;
    jo_fixed                                slope_ac;
    jo_fixed                                slope_ab;
    jo_fixed                                slope_bc;
    jo_fixed                                xl;
    jo_fixed                                xr;
    jo_fixed                                yc;
    jo_fixed                                xc;
    int                                     y;
    int                                     y_end;
    int                                     x;
    int                                     x_end;
    const int                               width = gfx->clipping_size.width;
    const int                               pitch = gfx->vram_size.width;
    const bool                              depth = gfx->depth_buffer != JO_NULL && gfx->depth_mode_testing != JO_SR_DEPTH_IGNORE;
    const unsigned int                      depth_mask = jo_sr_depth_mask(gfx->depth_mode_testing);
    bool                                    gouraud;
    jo_color                                flat;

    // Sorted from the top to the bottom
    a = &p[0];
    b = &p[1];
    c = &p[2];
    if (b->y < a->y) { tmp = a; a = b; b = tmp; }
    if (c->y < a->y) { tmp = a; a = c; c = tmp; }
    if (c->y < b->y) { tmp = b; b = c; c = tmp; }
    // The gradients of the attributes on the surface (positions in 1/16 pixel: the products fit in 64 bits)
    dx1 = (b->x - a->x) >> 12;
    dy1 = (b->y - a->y) >> 12;
    dx2 = (c->x - a->x) >> 12;
    dy2 = (c->y - a->y) >> 12;
    det = (long long)dx1 * dy2 - (long long)dx2 * dy1;
    if (det == 0)
        return;
    for (JO_ZERO(k); k < JO_SR_ATTRIBUTES; ++k)
    {
        // (only the attributes used)
        if ((k == JO_SR_DEPTH && !depth) || (k >= JO_SR_U && texture == JO_NULL) || (k >= JO_SR_RED && k <= JO_SR_BLUE && texture != JO_NULL))
        {
            d[k][0] = d[k][1] = 0;
            continue;
        }
        n = (long long)(b->a[k] - a->a[k]) * dy2 - (long long)(c->a[k] - a->a[k]) * dy1;
        d[k][0] = (jo_fixed)((n << 4) / det);
        n = (long long)(c->a[k] - a->a[k]) * dx1 - (long long)(b->a[k] - a->a[k]) * dx2;
        d[k][1] = (jo_fixed)((n << 4) / det);
    }
    gouraud = texture == JO_NULL && (a->a[JO_SR_RED] != b->a[JO_SR_RED] || a->a[JO_SR_RED] != c->a[JO_SR_RED] ||
                                     a->a[JO_SR_GREEN] != b->a[JO_SR_GREEN] || a->a[JO_SR_GREEN] != c->a[JO_SR_GREEN] ||
                                     a->a[JO_SR_BLUE] != b->a[JO_SR_BLUE] || a->a[JO_SR_BLUE] != c->a[JO_SR_BLUE]);
    flat = JO_COLOR_SATURN_RGB(jo_fixed2int(a->a[JO_SR_RED]), jo_fixed2int(a->a[JO_SR_GREEN]), jo_fixed2int(a->a[JO_SR_BLUE]));
    slope_ac = jo_sr_edge_slope(a, c);
    slope_ab = jo_sr_edge_slope(a, b);
    slope_bc = jo_sr_edge_slope(b, c);
    // The rows whose center is inside of the triangle: ceil(y - 0.5)
    y = JO_MAX((a->y + 0x7FFF) >> 16, 0);
    y_end = JO_MIN((c->y + 0x7FFF) >> 16, gfx->clipping_size.height);
    for (; y < y_end; ++y)
    {
        jo_color                            *pixel;
        jo_fixed                            *z;

        yc = jo_int2fixed(y) + JO_FIXED_1_DIV_2;
        xl = jo_sr_edge_x(a, slope_ac, c, yc);
        xr = yc < b->y ? jo_sr_edge_x(a, slope_ab, b, yc) : jo_sr_edge_x(b, slope_bc, c, yc);
        if (xr < xl)
            JO_SWAP(xl, xr);
        x = JO_MAX((xl + 0x7FFF) >> 16, 0);
        x_end = JO_MIN((xr + 0x7FFF) >> 16, width);
        if (x >= x_end)
            continue;
        pixel = gfx->color_buffer + y * pitch + x;
        z = depth ? gfx->depth_buffer + y * pitch + x : JO_NULL;
        // A color without depth: the row filled
        if (texture == JO_NULL && !gouraud && !depth)
        {
            for (count = x_end - x; count > 0; --count)
                *pixel++ = flat;
            continue;
        }
        // The attributes at the center of the first pixel
        xc = jo_int2fixed(x) + JO_FIXED_1_DIV_2;
        for (JO_ZERO(k); k < JO_SR_ATTRIBUTES; ++k)
            v[k] = a->a[k] + (jo_fixed)(((long long)d[k][0] * (xc - a->x) + (long long)d[k][1] * (yc - a->y)) >> 16);
        if (texture == JO_NULL)
        {
            for (; x < x_end; ++x, ++pixel)
            {
                if (depth)
                {
                    if (jo_sr_depth_test(depth_mask, v[JO_SR_DEPTH], *z))
                    {
                        *pixel = gouraud ? JO_COLOR_SATURN_RGB(jo_sr_component(v[JO_SR_RED]), jo_sr_component(v[JO_SR_GREEN]),
                                                               jo_sr_component(v[JO_SR_BLUE])) : flat;
                        *z = v[JO_SR_DEPTH];
                    }
                    ++z;
                    v[JO_SR_DEPTH] += d[JO_SR_DEPTH][0];
                }
                else
                    *pixel = JO_COLOR_SATURN_RGB(jo_sr_component(v[JO_SR_RED]), jo_sr_component(v[JO_SR_GREEN]),
                                                 jo_sr_component(v[JO_SR_BLUE]));
                v[JO_SR_RED] += d[JO_SR_RED][0];
                v[JO_SR_GREEN] += d[JO_SR_GREEN][0];
                v[JO_SR_BLUE] += d[JO_SR_BLUE][0];
            }
            continue;
        }
        // Textured: u and v exact at the ends of each segment of JO_SR_SPAN pixels, linear between
        {
            jo_fixed                        w;
            jo_fixed                        u0;
            jo_fixed                        v0;
            jo_fixed                        u1;
            jo_fixed                        v1;
            jo_fixed                        du;
            jo_fixed                        dv;
            jo_color                        color;

            w = jo_sr_w(v[JO_SR_INVERSE_W]);
            u0 = jo_fixed_mult(v[JO_SR_U], w);
            v0 = jo_fixed_mult(v[JO_SR_V], w);
            for (first = x; first < x_end; first += count)
            {
                count = JO_MIN(x_end - first, JO_SR_SPAN);
                v[JO_SR_U] += d[JO_SR_U][0] * count;
                v[JO_SR_V] += d[JO_SR_V][0] * count;
                v[JO_SR_INVERSE_W] += d[JO_SR_INVERSE_W][0] * count;
                w = jo_sr_w(v[JO_SR_INVERSE_W]);
                u1 = jo_fixed_mult(v[JO_SR_U], w);
                v1 = jo_fixed_mult(v[JO_SR_V], w);
                du = count == JO_SR_SPAN ? (u1 - u0) >> JO_SR_SPAN_SHIFT : (u1 - u0) / count;
                dv = count == JO_SR_SPAN ? (v1 - v0) >> JO_SR_SPAN_SHIFT : (v1 - v0) / count;
                for (k = count; k > 0; --k, ++pixel)
                {
                    color = jo_sr_texel(texture, u0, v0);
                    u0 += du;
                    v0 += dv;
                    if (depth)
                    {
                        if (color != JO_COLOR_Transparent && jo_sr_depth_test(depth_mask, v[JO_SR_DEPTH], *z))
                        {
                            *pixel = color;
                            *z = v[JO_SR_DEPTH];
                        }
                        ++z;
                        v[JO_SR_DEPTH] += d[JO_SR_DEPTH][0];
                    }
                    else if (color != JO_COLOR_Transparent)
                        *pixel = color;
                }
                u0 = u1;
                v0 = v1;
            }
        }
    }
}

/** @brief The attributes of a vertex on the surface
 *  @param p Output
 *  @param vertex The vertex (its position projected on the surface, its w and its z kept)
 *  @param depth The depth is used
 *  @param texture The texture (JO_NULL: none)
 */
static __jo_force_inline void               jo_sr_set_point(jo_sr_point * const p, const jo_software_renderer_vertex * const vertex,
                                                            const bool depth, const jo_sr_texture * const texture)
{
    int                                     inverse_w;

    p->x = vertex->pos.x;
    p->y = vertex->pos.y;
    p->a[JO_SR_DEPTH] = depth ? jo_fixed_div(vertex->pos.z, vertex->pos.w) : 0;
    p->a[JO_SR_RED] = jo_int2fixed(JO_COLOR_SATURN_GET_R(vertex->color));
    p->a[JO_SR_GREEN] = jo_int2fixed(JO_COLOR_SATURN_GET_G(vertex->color));
    p->a[JO_SR_BLUE] = jo_int2fixed(JO_COLOR_SATURN_GET_B(vertex->color));
    if (texture == JO_NULL)
    {
        p->a[JO_SR_U] = p->a[JO_SR_V] = p->a[JO_SR_INVERSE_W] = 0;
        return;
    }
    // 2^24 / w, and the texture coordinates in texels (u = 1: the width of the texture) divided by w
    inverse_w = jo_sr_div64(1 << (40 - 32), 0, vertex->pos.w);
    p->a[JO_SR_INVERSE_W] = inverse_w;
    p->a[JO_SR_U] = (int)(((long long)(vertex->uv_texture_mapping.x * texture->width) * inverse_w) >> JO_SR_INVERSE_W_SHIFT);
    p->a[JO_SR_V] = (int)(((long long)(vertex->uv_texture_mapping.y * texture->height) * inverse_w) >> JO_SR_INVERSE_W_SHIFT);
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
    // (no clipping on the near plane: a vertex at the camera or behind it was divided by 0 or mirrored, the scanlines of
    // huge coordinates froze the game for frames. Not drawn)
    if (result.v0.pos.w < JO_SR_NEAR_W || result.v1.pos.w < JO_SR_NEAR_W || result.v2.pos.w < JO_SR_NEAR_W)
        return ;
    if (gfx->face_culling_mode != JO_SR_NO_FACE_CULLING && jo_is_triangle_face_culled(gfx->face_culling_mode, &result.v0.pos, &result.v1.pos, &result.v2.pos))
        return ;
    jo_transform_to_surface_coord(gfx, &result.v0.pos, &result.v1.pos, &result.v2.pos);
    // (far outside of the surface: the same freeze)
    if (JO_ABS(result.v0.pos.x) > JO_SR_GUARD_BAND || JO_ABS(result.v1.pos.x) > JO_SR_GUARD_BAND || JO_ABS(result.v2.pos.x) > JO_SR_GUARD_BAND ||
        JO_ABS(result.v0.pos.y) > JO_SR_GUARD_BAND || JO_ABS(result.v1.pos.y) > JO_SR_GUARD_BAND || JO_ABS(result.v2.pos.y) > JO_SR_GUARD_BAND)
        return ;

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
    {
        jo_sr_point                         points[3];
        jo_sr_texture                       texture;
        const jo_sr_texture                 *t;
        const bool                          depth = gfx->depth_buffer != JO_NULL && gfx->depth_mode_testing != JO_SR_DEPTH_IGNORE;

        // (textured: a sprite that can't be a texture, its colors)
        t = gfx->draw_mode == JO_SR_DRAW_TEXTURED && jo_sr_get_texture((jo_software_renderer_gfx *)gfx, result.sprite_id, &texture) ? &texture : JO_NULL;
        jo_sr_set_point(&points[0], &result.v0, depth, t);
        jo_sr_set_point(&points[1], &result.v1, depth, t);
        jo_sr_set_point(&points[2], &result.v2, depth, t);
        jo_sr_fill_triangle(gfx, points, t);
    }
}

#endif

/*
** END OF FILE
*/
