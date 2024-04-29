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
/** @file voxel.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Voxel renderer
 *  @bug No known bugs.
 */

#ifndef __JO_VOXEL_H__
# define __JO_VOXEL_H__

#ifdef JO_COMPILE_WITH_SOFTWARE_RENDERER_SUPPORT

/** @brief Voxel computation cache (internal engine usage)
 *  @warning MC Hammer: don't touch this
 */
typedef struct
{
    jo_fixed        plx;
    jo_fixed        ply;
    jo_fixed        px;
    jo_fixed        py;
    jo_fixed        clipping_size_width;
    jo_fixed        inv_clipping_size_width;
    jo_fixed        inv_zfar;
    int             map_width_minus_1;
}                   __jo_voxel_computation_cache;

/** @brief Voxel camera definition
 */
typedef struct
{
    jo_fixed        scale;
    jo_fixed        x;
    jo_fixed        y;
    jo_fixed        height;
    jo_fixed        horizon;
    jo_fixed        zfar;
    jo_fixed        angle;
}                   jo_voxel_camera;

/** @brief Voxel main definition
 */
typedef struct
{
    jo_software_renderer_gfx        *gfx;
    jo_palette                      terrain_pal;
    jo_palette                      height_pal;
    jo_img_8bits                    terrain_img;
    jo_img_8bits                    height_img;
    jo_voxel_camera                 camera;
    bool                            use_zbuffer;
    __jo_voxel_computation_cache    __cache;
}                                   jo_voxel;

/*
 ██████╗ ██████╗ ██████╗ ███████╗
██╔════╝██╔═══██╗██╔══██╗██╔════╝
██║     ██║   ██║██████╔╝█████╗
██║     ██║   ██║██╔══██╗██╔══╝
╚██████╗╚██████╔╝██║  ██║███████╗
 ╚═════╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝
*/

/** @brief Do computation linked to camera angle
 *  @param voxel_data Voxel
 *  @remarks Must be called if you change camera.angle manually.
 */
void            jo_voxel_do_angle_computation(jo_voxel * const voxel_data);

/** @brief Draw voxels to gfx
 *  @param voxel_data Voxel
 *  @remarks You should call jo_software_renderer_flush() at some point
 */
void            jo_voxel_redraw(jo_voxel * const voxel_data);

/** @brief Draw voxels to gfx
 *  @param voxel_data Voxel
 *  @warning Must be call before calling jo_voxel_redraw() or jo_voxel_redraw_and_flush()
 */
void            jo_voxel_run(jo_voxel * const voxel_data);

/** @brief Draw voxels and flush the buffer
 *  @param voxel_data Voxel
 *  @remarks It's just an helper method
 */
static  __jo_force_inline void      jo_voxel_redraw_and_flush(jo_voxel * const voxel_data)
{
    jo_software_renderer_clear(voxel_data->gfx, JO_COLOR_Black);
    jo_voxel_redraw(voxel_data);
    jo_software_renderer_flush(voxel_data->gfx);
}

/*
 ██████╗ █████╗ ███╗   ███╗███████╗██████╗  █████╗
██╔════╝██╔══██╗████╗ ████║██╔════╝██╔══██╗██╔══██╗
██║     ███████║██╔████╔██║█████╗  ██████╔╝███████║
██║     ██╔══██║██║╚██╔╝██║██╔══╝  ██╔══██╗██╔══██║
╚██████╗██║  ██║██║ ╚═╝ ██║███████╗██║  ██║██║  ██║
 ╚═════╝╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝
*/

/** @brief Set scale (camera.scale)
 *  @param voxel_data Voxel
 *  @param Scale fixed value (see also jo_int2fixed() or jo_float2fixed())
 */
static  __jo_force_inline void      jo_voxel_set_scale(jo_voxel * const voxel_data, const jo_fixed scale)
{
    voxel_data->camera.scale = scale;
}

/** @brief Increase view distance (camera.zfar)
 *  @param voxel_data Voxel
 *  @param distance fixed distance value (see also jo_int2fixed() or jo_float2fixed())
 */
static  __jo_force_inline void      jo_voxel_increase_view_distance(jo_voxel * const voxel_data, const jo_fixed distance)
{
    voxel_data->camera.zfar += distance;
    voxel_data->__cache.inv_zfar = jo_fixed_div(JO_FIXED_1, voxel_data->camera.zfar);
    jo_voxel_do_angle_computation(voxel_data);
}

/** @brief Decrease view distance (camera.zfar)
 *  @param voxel_data Voxel
 *  @param distance fixed distance value (see also jo_int2fixed() or jo_float2fixed())
 */
static  __jo_force_inline void      jo_voxel_decrease_view_distance(jo_voxel * const voxel_data, const jo_fixed distance)
{
    voxel_data->camera.zfar -= distance;
    voxel_data->__cache.inv_zfar = jo_fixed_div(JO_FIXED_1, voxel_data->camera.zfar);
    jo_voxel_do_angle_computation(voxel_data);
}

/** @brief Increase horizon height (camera.horizon)
 *  @param voxel_data Voxel
 *  @param distance fixed distance value (see also jo_int2fixed() or jo_float2fixed())
 */
static  __jo_force_inline void      jo_voxel_horizon_up(jo_voxel * const voxel_data, const jo_fixed distance)
{
    voxel_data->camera.horizon += distance;
}

/** @brief Decrease horizon height (camera.horizon)
 *  @param voxel_data Voxel
 *  @param distance fixed distance value (see also jo_int2fixed() or jo_float2fixed())
 */
static  __jo_force_inline void      jo_voxel_horizon_down(jo_voxel *const voxel_data, const jo_fixed distance)
{
    voxel_data->camera.horizon -= distance;
}

/** @brief Increase camera height (camera.height)
 *  @param voxel_data Voxel
 *  @param distance fixed distance value (see also jo_int2fixed() or jo_float2fixed())
 */
static  __jo_force_inline void      jo_voxel_move_up(jo_voxel *voxel_data, const jo_fixed distance)
{
    voxel_data->camera.height += distance;
}

/** @brief Decrease camera height (camera.height)
 *  @param voxel_data Voxel
 *  @param distance fixed distance value (see also jo_int2fixed() or jo_float2fixed())
 */
static  __jo_force_inline void      jo_voxel_move_down(jo_voxel * const voxel_data, const jo_fixed distance)
{
    voxel_data->camera.height -= distance;
}

/** @brief Move camera forward (camera.x & camera.y)
 *  @param voxel_data Voxel
 *  @param distance fixed distance value (see also jo_int2fixed() or jo_float2fixed())
 */
static  __jo_force_inline void      jo_voxel_move_forward(jo_voxel * const voxel_data, const jo_fixed distance)
{
    voxel_data->camera.x += jo_fixed_mult(jo_fixed_cos(voxel_data->camera.angle), distance);
    voxel_data->camera.y += jo_fixed_mult(jo_fixed_sin(voxel_data->camera.angle), distance);
}

/** @brief Move camera backward (camera.x & camera.y)
 *  @param voxel_data Voxel
 *  @param distance fixed distance value (see also jo_int2fixed() or jo_float2fixed())
 */
static  __jo_force_inline void      jo_voxel_move_backward(jo_voxel * const voxel_data, const jo_fixed distance)
{
    voxel_data->camera.x -= jo_fixed_mult(jo_fixed_cos(voxel_data->camera.angle), distance);
    voxel_data->camera.y -= jo_fixed_mult(jo_fixed_sin(voxel_data->camera.angle), distance);
}

/** @brief Rotate camera on the left (camera.angle)
 *  @param voxel_data Voxel
 *  @param angle_rad fixed rad value (see also jo_int2fixed() or jo_float2fixed())
 */
static  __jo_force_inline void      jo_voxel_rotate_left(jo_voxel * const voxel_data, const jo_fixed angle_rad)
{
    voxel_data->camera.angle -= angle_rad;
    jo_voxel_do_angle_computation(voxel_data);
}

/** @brief Rotate camera on the right (camera.angle)
 *  @param voxel_data Voxel
 *  @param angle_rad fixed rad value (see also jo_int2fixed() or jo_float2fixed())
 */
static  __jo_force_inline void      jo_voxel_rotate_right(jo_voxel * const voxel_data, const jo_fixed angle_rad)
{
    voxel_data->camera.angle += angle_rad;
    jo_voxel_do_angle_computation(voxel_data);
}

#endif // JO_COMPILE_WITH_SOFTWARE_RENDERER_SUPPORT

#endif /* !__JO_VOXEL_H__ */

/*
** END OF FILE
*/
