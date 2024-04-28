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
#include "jo/voxel.h"

void            jo_voxel_do_angle_computation(jo_voxel * const voxel_data)
{
    jo_fixed sinangle_zfar = jo_fixed_mult(jo_fixed_sin(voxel_data->camera.angle), voxel_data->camera.zfar);
    jo_fixed cosangle_zfar = jo_fixed_mult(jo_fixed_cos(voxel_data->camera.angle), voxel_data->camera.zfar);
    voxel_data->__cache.plx = cosangle_zfar + sinangle_zfar;
    voxel_data->__cache.ply = sinangle_zfar - cosangle_zfar;
    jo_fixed prx = cosangle_zfar - sinangle_zfar;
    jo_fixed pry = sinangle_zfar + cosangle_zfar;
    voxel_data->__cache.px = jo_fixed_div((prx - voxel_data->__cache.plx), jo_int2fixed(voxel_data->gfx->clipping_size.width));
    voxel_data->__cache.py = jo_fixed_div((pry - voxel_data->__cache.ply), jo_int2fixed(voxel_data->gfx->clipping_size.width));
}

void            jo_voxel_redraw(jo_voxel * const voxel_data)
{
    for (jo_fixed x = JO_FIXED_0; x < jo_int2fixed(voxel_data->gfx->clipping_size.width); x += JO_FIXED_1)
    {
        jo_fixed deltax = jo_fixed_div(voxel_data->__cache.plx + jo_fixed_mult(voxel_data->__cache.px, x), voxel_data->camera.zfar);
        jo_fixed deltay = jo_fixed_div(voxel_data->__cache.ply + jo_fixed_mult(voxel_data->__cache.py, x), voxel_data->camera.zfar);
        jo_fixed rx = voxel_data->camera.x;
        jo_fixed ry = voxel_data->camera.y;
        int maxheight = voxel_data->gfx->clipping_size.height;
        int int_x = jo_fixed2int(x);
        for (jo_fixed z = JO_FIXED_1; z < voxel_data->camera.zfar; z += JO_FIXED_1)
        {
            rx += deltax;
            ry += deltay;
            int mapoffset = ((voxel_data->terrain_img.width * (jo_fixed2int(ry) & voxel_data->__cache.map_width_minus_1)) +
                             (jo_fixed2int(rx) & voxel_data->__cache.map_width_minus_1));
            // JO_COLOR_SATURN_GET_R: we use the red component arbitrary because it's a grayscale image
            jo_fixed c = jo_int2fixed(JO_COLOR_SATURN_GET_R(voxel_data->height_pal.data[voxel_data->height_img.data[mapoffset] - 1]));
            int projheight = jo_fixed2int(jo_fixed_mult(jo_fixed_div(voxel_data->camera.height - c, z), voxel_data->camera.scale) + voxel_data->camera.horizon);
            if (projheight < maxheight)
            {
                jo_color color = voxel_data->terrain_pal.data[voxel_data->terrain_img.data[mapoffset] - 1];
                for (int y = projheight; y < maxheight; ++y)
                {
                    if (y >= 0)
                    {
                        if (voxel_data->use_zbuffer)
                            jo_software_renderer_draw_pixel3D(voxel_data->gfx, x, jo_int2fixed(y), z, color);
                        else
                            voxel_data->gfx->color_buffer[int_x + y * voxel_data->gfx->vram_size.width] = color;
                    }

                }
                maxheight = projheight;
            }
        }
    }
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
    if (voxel_data->terrain_img.width != 8 &&
        voxel_data->terrain_img.width != 16 &&
        voxel_data->terrain_img.width != 32 &&
        voxel_data->terrain_img.width != 64 &&
        voxel_data->terrain_img.width != 128 &&
        voxel_data->terrain_img.width != 256 &&
        voxel_data->terrain_img.width != 512)
    {
        jo_core_error("terrain size must be a power of 2");
        return ;
    }
#endif

    voxel_data->__cache.map_width_minus_1 = voxel_data->terrain_img.width - 1;
    jo_voxel_do_angle_computation(voxel_data);
    jo_voxel_redraw_and_flush(voxel_data);
}

#endif

/*
** END OF FILE
*/
