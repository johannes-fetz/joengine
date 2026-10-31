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
**  - jo_3d_create_sphere() to create a sphere programmatically (with normals for the lighting)
**  - A camera turning around a target (like Burning Rangers or Sonic Jam with the L and R buttons)
**
** L / R        : Turn the camera around the target
** UP / DOWN    : Move the camera closer / farther
** A / B        : Move the camera up / down
*/

# define SPHERE_COUNT       (6)
# define SPHERE_DISTANCE    (90)
# define MIN_DISTANCE       (80)
# define MAX_DISTANCE       (400)

static jo_camera            cam;
static jo_3d_mesh           *sphere;
static jo_3d_mesh           *small_sphere;
/* Camera position around the target */
static int                  camera_angle = 0;
static int                  camera_distance = 220;
static int                  camera_height = -60;

static void                 update_camera(void)
{
    // The camera turns on a circle around the target (0, 0, 0). Y goes down, so a negative height is above the ground
    jo_3d_camera_set_viewpoint(&cam,
                               jo_sin_mult(camera_distance, camera_angle),
                               camera_height,
                               -jo_cos_mult(camera_distance, camera_angle));
    jo_3d_camera_set_target(&cam, 0, 0, 0);
}

void			            my_draw(void)
{
    static short            spin = 0;
    int                     i;

    jo_printf(10, 1, "*Camera & sphere demo*");
    jo_printf(1, 25, "L/R: turn  UP/DOWN: zoom  A/B: height");

    update_camera();
    jo_3d_camera_look_at(&cam);
    // Light going down (Y goes down) and toward the back: call it after jo_3d_camera_look_at() so it stays in world space
    jo_3d_light(0, 1, 0);

    // Big sphere in the center
    jo_3d_push_matrix();
    {
        jo_3d_rotate_matrix_y(spin);
        jo_3d_mesh_draw(sphere);
    }
    jo_3d_pop_matrix();

    // Small spheres around
    for (i = 0; i < SPHERE_COUNT; ++i)
    {
        jo_3d_push_matrix();
        {
            jo_3d_translate_matrix(jo_sin_mult(SPHERE_DISTANCE, i * (360 / SPHERE_COUNT)), 0, jo_cos_mult(SPHERE_DISTANCE, i * (360 / SPHERE_COUNT)));
            jo_3d_mesh_draw(small_sphere);
        }
        jo_3d_pop_matrix();
    }
    ++spin;

    jo_printf(1, 27, "Angle:%3d  Distance:%3d  Height:%4d ", camera_angle, camera_distance, camera_height);
    jo_printf(1, 28, "Polygons: %d / %d  ", jo_3d_get_displayed_polygon_count(), jo_3d_get_polygon_count());
}

void			            my_gamepad(void)
{
    if (!jo_is_pad1_available())
        return ;
    if (jo_is_pad1_key_pressed(JO_KEY_L))
        camera_angle -= 2;
    if (jo_is_pad1_key_pressed(JO_KEY_R))
        camera_angle += 2;
    if (camera_angle < 0)
        camera_angle += 360;
    else if (camera_angle >= 360)
        camera_angle -= 360;
    if (jo_is_pad1_key_pressed(JO_KEY_UP) && camera_distance > MIN_DISTANCE)
        camera_distance -= 2;
    if (jo_is_pad1_key_pressed(JO_KEY_DOWN) && camera_distance < MAX_DISTANCE)
        camera_distance += 2;
    if (jo_is_pad1_key_pressed(JO_KEY_A))
        camera_height -= 2;
    if (jo_is_pad1_key_pressed(JO_KEY_B))
        camera_height += 2;
}

void			            jo_main(void)
{
    int                     i;

    jo_core_init(JO_COLOR_Black);
    jo_3d_camera_init(&cam);

    // 12 slices x 8 stacks = 96 polygons
    sphere = jo_3d_create_sphere(40, 12, 8);
    // Lighter spheres for the small ones: 8 x 6 = 48 polygons
    small_sphere = jo_3d_create_sphere(15, 8, 6);
    if (sphere == JO_NULL || small_sphere == JO_NULL)
        return ;
    jo_3d_set_mesh_color(sphere, JO_COLOR_Red);
    jo_3d_set_mesh_light(sphere, true);
    // The polygons of a mesh can have different colors
    for (i = 0; i < 8 * 6; ++i)
        jo_3d_set_mesh_polygon_color(small_sphere, (i & 1) ? JO_COLOR_Yellow : JO_COLOR_Green, i);
    jo_3d_set_mesh_light(small_sphere, true);

    jo_core_add_callback(my_gamepad);
    jo_core_add_callback(my_draw);
    jo_core_run();
}

/*
** END OF FILE
*/
