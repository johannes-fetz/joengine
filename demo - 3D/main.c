/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2017, Johannes Fetz (johannesfetz@gmail.com)
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

#include <jo/jo.h>

jo_camera           cam;
jo_vertice          cube_vertices[] = JO_3D_CUBE_VERTICES(32);
jo_3d_quad          cube_quads[6];
jo_vertice          plane_vertices[] = JO_3D_PLANE_VERTICES(104);
jo_3d_quad          plane;

void                create_cube(void)
{
    int             i;

    jo_3d_create_cube(cube_quads, cube_vertices);
    for (i = 0; i < 6; ++i)
        jo_3d_set_texture(&cube_quads[i], 0);
}

void                create_plane(void)
{
    jo_3d_create_plane(&plane, plane_vertices);
    jo_3d_set_texture(&plane, 1);
}

void                create_ghost(void)
{
    jo_3d_set_transparency(jo_3d_get_sprite_quad(2), true);
}

void                draw_ghost(void)
{
    static bool     sens = true;
    static int      distance = -40;

    jo_3d_push_matrix();
    {
        jo_3d_translate_matrix_y(10);
        jo_3d_translate_matrix_z(distance);
        jo_3d_draw_sprite(2);
    }
    jo_3d_pop_matrix();
    if (sens)
    {
        --distance;
        if (distance < -55)
            sens ^= true;
    }
    else
    {
        ++distance;
        if (distance > -40)
            sens ^= true;
    }
}

void                draw_floor(void)
{
    jo_3d_push_matrix();
    {
        jo_3d_rotate_matrix_x(-65);
        jo_3d_draw(&plane);
    }
    jo_3d_pop_matrix();
}

void                draw_cube(void)
{
    static short    rx, ry, rz;

    jo_3d_push_matrix();
    {
        jo_3d_rotate_matrix(rx, ry, rz);
        rx += 3;
        ry += 2;
        rz += 1;
        jo_3d_draw_array(cube_quads, 6);
    }
    jo_3d_pop_matrix();
}

void			    my_draw(void)
{
    jo_printf(12, 1, "*Simple 3D demo*");

    jo_3d_camera_look_at(&cam);

    draw_floor();
    draw_ghost();
    draw_cube();

    jo_printf(0, 28, "Polygon count: %d  ", jo_3d_get_polygon_count());
}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_Black);

    jo_3d_camera_init(&cam);

    jo_sprite_add_tga(JO_ROOT_DIR, "BOX.TGA", JO_COLOR_Transparent);
    jo_sprite_add_tga(JO_ROOT_DIR, "FLR.TGA", JO_COLOR_Transparent);
    jo_sprite_add_tga(JO_ROOT_DIR, "G.TGA", JO_COLOR_Green);

    create_cube();
    create_ghost();
    create_plane();

	jo_core_add_callback(my_draw);
	jo_core_run();
}

/*
** END OF FILE
*/
