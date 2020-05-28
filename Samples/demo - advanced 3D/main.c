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

/***************************************************************************************************\
** Special Note : The 3D part on Jo Engine is still in development. So, some glitch may occur ;)
**                Btw, texture mapping for triangle base mesh (not quads) is experimental.
\***************************************************************************************************/

#include <jo/jo.h>
#include "model.h"

static jo_camera    cam;
static float        rx, ry;
static bool         use_screen_doors;
static bool         use_light = true;
static bool         use_texture = true;
static unsigned int selected_polygon_index = 0;

void			    my_draw(void)
{
    jo_printf(12, 1, "*3D Mesh Demo*");
    jo_3d_camera_look_at(&cam);
    jo_3d_push_matrix();
    {
        jo_3d_rotate_matrix_rad_x(rx);
        jo_3d_rotate_matrix_rad_y(ry);
        /* Use Jo Map Editor (v5.0 or superior) to convert mesh for the Sega Saturn */
        display_rock1_mesh();
    }
    jo_3d_pop_matrix();
    jo_printf(0, 24, "[PAD] Rotate mesh");
    jo_printf(0, 25, "[L/R] select polygon (index=%d)  ", selected_polygon_index);
    jo_printf(0, 26, "[C] to toggle texture (%s) ", use_texture ? "on" : "off");
    jo_printf(0, 27, "[B] to toggle light (%s) ", use_light ? "on" : "off");
    jo_printf(0, 28, "[A] to toggle screen doors (%s) ", use_screen_doors ? "on" : "off");
}

void            apply_effect(void)
{
    jo_3d_set_mesh_screen_doors(&MeshCube, use_screen_doors);
    jo_3d_set_mesh_light(&MeshCube, use_light);
    if (use_texture)
        jo_3d_set_mesh_texture(&MeshCube, 0);
    else
        jo_3d_set_mesh_color(&MeshCube, JO_COLOR_Gray);
    jo_3d_set_mesh_polygon_color(&MeshCube, JO_COLOR_Red, selected_polygon_index);
}

void			my_gamepad(void)
{
    if (jo_is_pad1_key_pressed(JO_KEY_UP))
        rx -= 0.05f;
    else if (jo_is_pad1_key_pressed(JO_KEY_DOWN))
        rx += 0.05f;
    else if (jo_is_pad1_key_pressed(JO_KEY_LEFT))
        ry += 0.05f;
    else if (jo_is_pad1_key_pressed(JO_KEY_RIGHT))
        ry -= 0.05f;
    if (jo_is_pad1_key_down(JO_KEY_A))
    {
        use_screen_doors ^= true;
        apply_effect();
    }
    if (jo_is_pad1_key_down(JO_KEY_B))
    {
        use_light ^= true;
        apply_effect();
    }
    if (jo_is_pad1_key_down(JO_KEY_C))
    {
        use_texture ^= true;
        apply_effect();
    }
    if (jo_is_pad1_key_down(JO_KEY_L))
    {
        if (selected_polygon_index > 0)
            --selected_polygon_index;
        else
            selected_polygon_index = jo_3d_get_mesh_polygon_count(&MeshCube) - 1;
        apply_effect();
    }
    if (jo_is_pad1_key_down(JO_KEY_R))
    {
        ++selected_polygon_index;
        if (selected_polygon_index >= jo_3d_get_mesh_polygon_count(&MeshCube))
            selected_polygon_index = 0;
        apply_effect();
    }
}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_Blue);
    jo_sprite_add_tga(JO_ROOT_DIR, "ROCK.TGA", JO_COLOR_Transparent);
    jo_3d_camera_init(&cam);
    jo_core_add_callback(my_gamepad);
	jo_core_add_callback(my_draw);
	apply_effect();
	jo_core_run();
}

/*
** END OF FILE
*/
