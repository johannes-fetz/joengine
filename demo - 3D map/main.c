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

#define WORLD_MAP_ID		(0)

jo_camera           cam;

void			    my_draw(void)
{
    static short    rx, ry, rz;

    jo_printf(8, 1, "*Draw map & sprite in 3D*");
    jo_3d_camera_look_at(&cam);
    jo_3d_push_matrix();
    {
        jo_3d_rotate_matrix(rx, ry, rz);
        rx += 3;
        ry += 2;
        rz += 1;
        jo_map_3d_draw(WORLD_MAP_ID, 0, 0, 0);
        jo_3d_rotate_matrix_x(90);
        jo_3d_draw_sprite_at(1, 0, -44, 0);
    }
    jo_3d_pop_matrix();
}

void			load_sprites()
{
	jo_sprite_add_tga("TEX", "BRK.TGA", JO_COLOR_Transparent);
	jo_sprite_add_tga("TEX", "COY.TGA", JO_COLOR_Green);
}

void			load_map()
{
	jo_map_load_from_file(WORLD_MAP_ID, 450, "MAP", "WALL.MAP");
}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_Blue);
    jo_3d_camera_init(&cam);
    jo_3d_camera_set_viewpoint(&cam, 0, 0, -250);
	load_sprites();
	load_map();
	jo_core_add_callback(my_draw);
	jo_core_run();
}

/*
** END OF FILE
*/
