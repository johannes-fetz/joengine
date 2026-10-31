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
** Based on Sebastian Macke implementation
** https://github.com/s-macke/VoxelSpace
**
** Note that Novalogic patent has expired
** https://patents.google.com/patent/US6020893
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
#include "jo/colors.h"
#include "jo/vdp1_command_pipeline.h"
#include "jo/sprites.h"
#include "jo/fs.h"
#include "jo/math.h"
#include "jo/list.h"
#include "jo/vdp2.h"
#include "jo/vdp2_malloc.h"
#include "jo/software_renderer.h"
#include "jo/voxel.h"

void            jo_voxel_do_angle_computation(jo_voxel * const voxel_data)
{
    const jo_fixed sinangle_zfar = jo_fixed_mult(jo_fixed_sin(voxel_data->camera.angle), voxel_data->camera.zfar);
    const jo_fixed cosangle_zfar = jo_fixed_mult(jo_fixed_cos(voxel_data->camera.angle), voxel_data->camera.zfar);
    const jo_fixed plx = cosangle_zfar + sinangle_zfar;
    const jo_fixed ply = sinangle_zfar - cosangle_zfar;
    const jo_fixed prx = cosangle_zfar - sinangle_zfar;
    const jo_fixed pry = sinangle_zfar + cosangle_zfar;
    const jo_fixed px = jo_fixed_mult((prx - plx), voxel_data->__cache.inv_clipping_size_width);
    const jo_fixed py = jo_fixed_mult((pry - ply), voxel_data->__cache.inv_clipping_size_width);
    voxel_data->__cache.deltax_0 = jo_fixed_mult(plx, voxel_data->__cache.inv_zfar);
    voxel_data->__cache.deltax_incr = jo_fixed_mult(px, voxel_data->__cache.inv_zfar);
    voxel_data->__cache.deltay_0 = jo_fixed_mult(ply, voxel_data->__cache.inv_zfar);
    voxel_data->__cache.deltay_incr = jo_fixed_mult(py, voxel_data->__cache.inv_zfar);
}

/** @brief Size of the table of the scales by distance (see __jo_voxel_redraw()): the farther samples divide */
# define JO_VOXEL_SCALE_TABLE_SIZE  (512)

/** @brief camera.scale / z for each distance z of the samples (1, 2, 3...), filled by each redraw */
static jo_fixed             __jo_voxel_scales[JO_VOXEL_SCALE_TABLE_SIZE];

/** @brief Check the depth of a pixel (see __jo_software_renderer_draw_pixel())
 *  @param mode Depth test
 *  @param z Depth of the pixel
 *  @param depth Depth in the buffer
 *  @return true if the pixel is drawn
 */
static __jo_force_inline bool   jo_voxel_depth_test(const jo_software_renderer_depth_mode mode, const jo_fixed z, const jo_fixed depth)
{
    switch (mode)
    {
        case JO_SR_DEPTH_LESS: return (z < depth);
        case JO_SR_DEPTH_LESS_OR_EQUAL: return (z <= depth);
        case JO_SR_DEPTH_GREATER_OR_EQUAL: return (z >= depth);
        case JO_SR_DEPTH_GREATER: return (z > depth);
        case JO_SR_DEPTH_NOT_EQUAL: return (z != depth);
        case JO_SR_DEPTH_EQUAL: return (z == depth);
        default: return (true);
    }
}

/** @brief Draw the terrain in a column of the buffer
 *  @param scales camera.scale / z for each sample (JO_NULL: divided for each sample)
 *  @return The height of the column above the terrain
 *  @remarks Inlined twice: with the table of the scales and with the division
 */
static __jo_force_inline int    jo_voxel_draw_column(const jo_voxel * const voxel_data, const int x, jo_fixed rx, jo_fixed ry,
                                                     const jo_fixed deltax, const jo_fixed deltay, const jo_fixed * const above,
                                                     const jo_fixed * const scales, const int count, const bool clear)
{
    jo_software_renderer_gfx * const gfx = voxel_data->gfx;
    const int               pitch = gfx->vram_size.width;
    jo_color * const        color_buffer = gfx->color_buffer;
    jo_fixed * const        depth_buffer = voxel_data->use_zbuffer ? gfx->depth_buffer : JO_NULL;
    const int               mapwidth_minus_1 = voxel_data->__cache.map_width_minus_1;
    const int               terrain_width = voxel_data->terrain_img.width;
    const unsigned char     *height_data = voxel_data->height_img.data;
    const unsigned char     *terrain_data = voxel_data->terrain_img.data;
    const jo_color          *terrain_pal = voxel_data->terrain_pal.data;
    const jo_fixed          camera_scale = voxel_data->camera.scale;
    const jo_fixed          camera_horizon = voxel_data->camera.horizon;
    int                     maxheight;
    int                     k;

    maxheight = gfx->clipping_size.height;
    for (JO_ZERO(k); k < count; ++k)
    {
        rx += deltax;
        ry += deltay;
        const int mapoffset = ((terrain_width * (jo_fixed2int(ry) & mapwidth_minus_1)) + (jo_fixed2int(rx) & mapwidth_minus_1));
        const jo_fixed h = above[height_data[mapoffset]];
        const int projheight = jo_fixed2int((scales != JO_NULL ? jo_fixed_mult(h, scales[k]) :
                                             jo_fixed_mult(jo_fixed_div(h, jo_int2fixed(k + 1)), camera_scale)) + camera_horizon);
        if (projheight < maxheight)
        {
            const jo_color color = terrain_pal[terrain_data[mapoffset] - 1];
            int y = (projheight >= 0) ? projheight : 0;
            jo_color *pixel = color_buffer + x + y * pitch;
            if (depth_buffer != JO_NULL)
            {
                // (the pixels of the column are inside the clipping rectangle: no clipping)
                const jo_software_renderer_depth_mode depth_mode = gfx->depth_mode_testing;
                const jo_fixed z = jo_int2fixed(k + 1);
                jo_fixed *depth = depth_buffer + x + y * pitch;
                for (; y < maxheight; ++y, pixel += pitch, depth += pitch)
                {
                    if (!jo_voxel_depth_test(depth_mode, z, *depth))
                    {
                        // (hidden: the pixel of a cleared buffer, each pixel of the column is visited once)
                        if (clear)
                            *pixel = JO_COLOR_Black;
                        continue;
                    }
                    *pixel = color;
                    *depth = z;
                }
            }
            else
            {
                for (; y < maxheight; ++y, pixel += pitch)
                    *pixel = color;
            }
            maxheight = projheight;
            // The column is full: the farther samples are hidden
            if (maxheight <= 0)
                break;
        }
    }
    return (maxheight);
}

void                        __jo_voxel_redraw(const jo_voxel * const voxel_data, const bool clear)
{
    jo_software_renderer_gfx * const gfx = voxel_data->gfx;
    const int               width = gfx->clipping_size.width;
    const int               height = gfx->clipping_size.height;
    const int               pitch = gfx->vram_size.width;
    jo_color * const        color_buffer = gfx->color_buffer;
    const jo_color          *height_pal = voxel_data->height_pal.data;
    const jo_fixed          camera_height = voxel_data->camera.height;
    const jo_fixed          camera_scale = voxel_data->camera.scale;
    const jo_fixed          zfar = voxel_data->camera.zfar;
    jo_fixed                deltax = voxel_data->__cache.deltax_0;
    jo_fixed                deltay = voxel_data->__cache.deltay_0;
    // camera.height - height of each index of the height map (the palette is read once, not for each sample)
    jo_fixed                above[256];
    jo_fixed                z;
    jo_color                *pixel;
    int                     count;
    int                     maxheight;
    int                     x;
    int                     y;

    for (JO_ZERO(x); x < 256; ++x)
        // JO_COLOR_SATURN_GET_R: we use the red component arbitrary because it's a grayscale image
        above[x] = camera_height - jo_int2fixed(JO_COLOR_SATURN_GET_R(height_pal[x - 1]));
    // The samples: z = 1, 2, 3... while z < zfar
    for (JO_ZERO(count), z = JO_FIXED_1; z < zfar; z += JO_FIXED_1)
        ++count;
    // camera.scale / z for each distance: a product for each sample instead of a division
    if (count <= JO_VOXEL_SCALE_TABLE_SIZE)
        for (JO_ZERO(x); x < count; ++x)
            __jo_voxel_scales[x] = jo_fixed_div(camera_scale, jo_int2fixed(x + 1));
    for (JO_ZERO(x); x < width; ++x)
    {
        if (count <= JO_VOXEL_SCALE_TABLE_SIZE)
            maxheight = jo_voxel_draw_column(voxel_data, x, voxel_data->camera.x, voxel_data->camera.y, deltax, deltay, above,
                                             __jo_voxel_scales, count, clear);
        else
            maxheight = jo_voxel_draw_column(voxel_data, x, voxel_data->camera.x, voxel_data->camera.y, deltax, deltay, above,
                                             JO_NULL, count, clear);
        // (instead of clearing the whole buffer first: the pixels of the column above the terrain)
        if (clear)
            for (JO_ZERO(y), pixel = color_buffer + x; y < maxheight; ++y, pixel += pitch)
                *pixel = JO_COLOR_Black;
        deltax += voxel_data->__cache.deltax_incr;
        deltay += voxel_data->__cache.deltay_incr;
    }
    // (the rest of the buffer, outside of the clipping rectangle: cleared like jo_software_renderer_clear() does)
    if (clear && pitch > width)
        for (JO_ZERO(y); y < height; ++y)
            for (x = width, pixel = color_buffer + y * pitch; x < pitch; ++x)
                pixel[x] = JO_COLOR_Black;
}

void                        jo_voxel_redraw(const jo_voxel * const voxel_data)
{
    __jo_voxel_redraw(voxel_data, false);
}

void            jo_voxel_run(jo_voxel * const voxel_data)
{
#ifdef JO_DEBUG
    if (voxel_data == JO_NULL)
    {
        jo_core_error("voxel_data is null");
        return ;
    }
    if (voxel_data->gfx == JO_NULL)
    {
        jo_core_error("gfx is not set");
        return ;
    }
    if (!JO_IS_POW2(voxel_data->terrain_img.width))
    {
        jo_core_error("terrain size must be a power of 2");
        return ;
    }
#endif

    voxel_data->__cache.inv_clipping_size_width = jo_fixed_div(JO_FIXED_1, jo_int2fixed(voxel_data->gfx->clipping_size.width));
    voxel_data->__cache.map_width_minus_1 = voxel_data->terrain_img.width - 1;
    voxel_data->__cache.inv_zfar = jo_fixed_div(JO_FIXED_1, voxel_data->camera.zfar);
    jo_voxel_do_angle_computation(voxel_data);
    jo_voxel_redraw_and_flush(voxel_data);
}

#endif

/*
** END OF FILE
*/
