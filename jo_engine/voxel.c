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

/** @brief The color of each index of the terrain (the palette in the color RAM read once per redraw: a read of the color RAM
 *  for each sample waited for the bus of the VDP2. The index 0, without a color in the palette (transparent): black, it read
 *  the color before the palette) */
static jo_color             __jo_voxel_colors[256];

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
    const jo_color          *terrain_pal = __jo_voxel_colors;
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
            const jo_color color = terrain_pal[terrain_data[mapoffset]];
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

/** @brief camera.height - the height of each index of the height map (the palette read once, not for each sample) */
static jo_fixed             __jo_voxel_above[256];

/** @brief The columns drawn by a CPU (see __jo_voxel_redraw()) */
typedef struct
{
    const jo_voxel          *voxel_data;
    int                     first;
    int                     end;
    int                     count;
    bool                    clear;
}                           jo_voxel_columns;

/** @brief The columns of the slave CPU (read through its cache: it is purged before each job) */
static jo_voxel_columns     __jo_voxel_slave_columns;

/** @brief Draw the columns from c->first to c->end (excluded)
 *  @param c Columns
 */
static void                 jo_voxel_draw_columns(const jo_voxel_columns * const c)
{
    const jo_voxel * const  voxel_data = c->voxel_data;
    jo_color * const        color_buffer = voxel_data->gfx->color_buffer;
    const int               pitch = voxel_data->gfx->vram_size.width;
    jo_fixed                deltax;
    jo_fixed                deltay;
    jo_color                *pixel;
    int                     maxheight;
    int                     x;
    int                     y;

    deltax = voxel_data->__cache.deltax_0 + c->first * voxel_data->__cache.deltax_incr;
    deltay = voxel_data->__cache.deltay_0 + c->first * voxel_data->__cache.deltay_incr;
    for (x = c->first; x < c->end; ++x)
    {
        if (c->count <= JO_VOXEL_SCALE_TABLE_SIZE)
            maxheight = jo_voxel_draw_column(voxel_data, x, voxel_data->camera.x, voxel_data->camera.y, deltax, deltay, __jo_voxel_above,
                                             __jo_voxel_scales, c->count, c->clear);
        else
            maxheight = jo_voxel_draw_column(voxel_data, x, voxel_data->camera.x, voxel_data->camera.y, deltax, deltay, __jo_voxel_above,
                                             JO_NULL, c->count, c->clear);
        // (instead of clearing the whole buffer first: the pixels of the column above the terrain)
        if (c->clear)
            for (JO_ZERO(y), pixel = color_buffer + x; y < maxheight; ++y, pixel += pitch)
                *pixel = JO_COLOR_Black;
        deltax += voxel_data->__cache.deltax_incr;
        deltay += voxel_data->__cache.deltay_incr;
    }
}

#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
/** @brief Check if the code runs on the slave CPU (its stack is in the first 16 KB of the high work RAM, see
 *  __jo_profile_current_cpu()) */
static  __jo_force_inline bool  jo_voxel_on_slave_cpu(void)
{
    unsigned int                sp;

    __asm__ volatile ("mov r15, %0" : "=r" (sp));
    return ((sp & 0x000FFFFF) < 0x4000);
}

/** @brief The columns of the slave CPU (jo_core_exec_on_slave() callback) */
static void                 jo_voxel_slave_draw_columns(void)
{
    jo_voxel_draw_columns(&__jo_voxel_slave_columns);
}
#endif

void                        __jo_voxel_redraw(const jo_voxel * const voxel_data, const bool clear)
{
    jo_software_renderer_gfx * const gfx = voxel_data->gfx;
    const int               width = gfx->clipping_size.width;
    const int               height = gfx->clipping_size.height;
    const int               pitch = gfx->vram_size.width;
    jo_color * const        color_buffer = gfx->color_buffer;
    const jo_color          *height_pal = voxel_data->height_pal.data;
    const jo_color          *terrain_pal = voxel_data->terrain_pal.data;
    const jo_fixed          camera_height = voxel_data->camera.height;
    const jo_fixed          camera_scale = voxel_data->camera.scale;
    const jo_fixed          zfar = voxel_data->camera.zfar;
    jo_voxel_columns        columns;
    jo_fixed                z;
    jo_color                *pixel;
    int                     count;
    int                     x;
    int                     y;

    // (the index 0: no color in the palette (transparent), the height 0: it read the color before the palette)
    __jo_voxel_above[0] = camera_height;
    __jo_voxel_colors[0] = JO_COLOR_Black;
    for (x = 1; x < 256; ++x)
    {
        // JO_COLOR_SATURN_GET_R: we use the red component arbitrary because it's a grayscale image
        __jo_voxel_above[x] = camera_height - jo_int2fixed(JO_COLOR_SATURN_GET_R(height_pal[x - 1]));
        __jo_voxel_colors[x] = terrain_pal[x - 1];
    }
    // The samples: z = 1, 2, 3... while z < zfar
    for (JO_ZERO(count), z = JO_FIXED_1; z < zfar; z += JO_FIXED_1)
        ++count;
    // camera.scale / z for each distance: a product for each sample instead of a division
    if (count <= JO_VOXEL_SCALE_TABLE_SIZE)
        for (JO_ZERO(x); x < count; ++x)
            __jo_voxel_scales[x] = jo_fixed_div(camera_scale, jo_int2fixed(x + 1));
    columns.voxel_data = voxel_data;
    columns.count = count;
    columns.clear = clear;
    JO_ZERO(columns.first);
    columns.end = width;
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
    // The columns shared by the 2 CPUs (each column on its own: the same picture), the left half on the slave CPU (the
    // voxel demo: 21 frames per second on the master CPU alone, 34 with half of the columns on each CPU; 9/16 on the slave
    // CPU: 32). Not when the redraw is a job of the slave CPU itself
    if (!jo_voxel_on_slave_cpu() && width >= 16)
    {
        __jo_voxel_slave_columns = columns;
        __jo_voxel_slave_columns.end = JO_DIV_BY_2(width) & ~7;
        columns.first = __jo_voxel_slave_columns.end;
        jo_core_exec_on_slave(jo_voxel_slave_draw_columns);
        jo_voxel_draw_columns(&columns);
        jo_core_wait_for_slave();
    }
    else
#endif
        jo_voxel_draw_columns(&columns);
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
