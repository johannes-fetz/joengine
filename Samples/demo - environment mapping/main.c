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

#include <jo/jo.h>

/*
** This demo shows:
**  - jo_3d_set_mesh_environment(): an environment map (a "matcap"), the reflection of the surroundings on a mesh. Each
**    polygon shows the tile of the picture of the direction of its normal in the view: the reflection moves on the
**    object when it turns
**  - The picture (ENV.TGA, 192 x 192, 6 x 6 tiles) is a chrome ball seen from the front with the Jo Engine logo
**  - The same mesh drawn twice (the environment map is set once per mesh)
**
** D-PAD        : Turn the sphere
** A / B        : Move the camera closer / farther
** START        : Stop / restart the automatic rotation
*/

# define SPHERE_RADIUS          (50)
# define SPHERE_SLICES          (16)
# define SPHERE_STACKS          (12)
# define ENVIRONMENT_TILES      (6)
# define MIN_DISTANCE           (110)
# define MAX_DISTANCE           (400)

static jo_camera                cam;
static jo_3d_mesh               *sphere;
static int                      logo_id;
static int                      angle_x = 0;
static int                      angle_y = 0;
static int                      orbit = 0;
static int                      camera_distance = 180;
static bool                     auto_rotation = true;

void                            my_draw(void)
{
    jo_printf(9, 1, "*Environment mapping demo*");
    jo_printf(1, 26, "D-PAD: turn  A/B: zoom  START: spin");

    jo_3d_camera_set_viewpoint(&cam, 0, 0, -camera_distance);
    jo_3d_camera_look_at(&cam);

    // The big sphere: the reflection of the logo moves when it turns
    jo_3d_push_matrix();
    {
        jo_3d_rotate_matrix_x(angle_x);
        jo_3d_rotate_matrix_y(angle_y);
        jo_3d_mesh_draw(sphere);
    }
    jo_3d_pop_matrix();

    // The same mesh, smaller, turning around the big one: it has its own reflection (the direction of its normals in the view)
    jo_3d_push_matrix();
    {
        jo_3d_translate_matrix(jo_sin_mult(95, orbit), jo_cos_mult(25, orbit), jo_cos_mult(95, orbit));
        jo_3d_set_scale_fixed(JO_FIXED_1_DIV_2 - (JO_FIXED_1 >> 3), JO_FIXED_1_DIV_2 - (JO_FIXED_1 >> 3), JO_FIXED_1_DIV_2 - (JO_FIXED_1 >> 3));
        jo_3d_rotate_matrix_y(-2 * orbit);
        jo_3d_mesh_draw(sphere);
    }
    jo_3d_pop_matrix();

    // The logo in the corner (2D sprite, in front of the 3D)
    jo_sprite_draw3D(logo_id, -92, -60, 100);

    if (auto_rotation)
    {
        angle_y = (angle_y + 1) % 360;
        angle_x = (angle_x + 359) % 360;
    }
    orbit = (orbit + 2) % 360;
    jo_printf(1, 28, "Polygons: %d / %d  ", jo_3d_get_displayed_polygon_count(), jo_3d_get_polygon_count());
}

void                            my_gamepad(void)
{
    if (!jo_is_pad1_available())
        return ;
    if (jo_is_pad1_key_pressed(JO_KEY_LEFT))
        angle_y = (angle_y + 358) % 360;
    if (jo_is_pad1_key_pressed(JO_KEY_RIGHT))
        angle_y = (angle_y + 2) % 360;
    if (jo_is_pad1_key_pressed(JO_KEY_UP))
        angle_x = (angle_x + 2) % 360;
    if (jo_is_pad1_key_pressed(JO_KEY_DOWN))
        angle_x = (angle_x + 358) % 360;
    if (jo_is_pad1_key_pressed(JO_KEY_A) && camera_distance > MIN_DISTANCE)
        camera_distance -= 2;
    if (jo_is_pad1_key_pressed(JO_KEY_B) && camera_distance < MAX_DISTANCE)
        camera_distance += 2;
    if (jo_is_pad1_key_down(JO_KEY_START))
        auto_rotation = !auto_rotation;
}

void                            jo_main(void)
{
    unsigned int                i;
    int                         env_id;

    jo_core_init(JO_COLOR_RGB(16, 16, 32));
    jo_3d_camera_init(&cam);

    logo_id = jo_sprite_add_tga(JO_ROOT_DIR, "LOGO.TGA", JO_COLOR_Transparent);
    // The picture of the surroundings (RGB, its width / tiles a multiple of 8 pixels: 192 / 6 = 32)
    env_id = jo_sprite_add_tga(JO_ROOT_DIR, "ENV.TGA", JO_COLOR_Transparent);
    sphere = jo_3d_create_sphere(SPHERE_RADIUS, SPHERE_SLICES, SPHERE_STACKS);
    if (logo_id < 0 || env_id < 0 || sphere == JO_NULL)
        return ;
    // The picture cut in 6 x 6 tiles (the normals of the polygons are computed), then each polygon shows its tile
    if (!jo_3d_set_mesh_environment(sphere, env_id, ENVIRONMENT_TILES))
        return ;
    for (i = 0; i < sphere->data.polygon_count; ++i)
        jo_3d_set_mesh_polygon_environment(sphere, i);

    jo_core_add_callback(my_gamepad);
    jo_core_add_callback(my_draw);
    jo_core_run();
}

/*
** END OF FILE
*/
