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

/*
** The water of the engine (jo/water.h): a sea with waves around the camera, crates that fall in it and float on the
** waves (jo_water_float()). The camera turns around them (pad: left and right to turn, up and down to go up and down,
** A: drop the crates again, B: another look of the water, C: another swell).
*/

#include <jo/jo.h>

/** @brief The crates: half their side, how much they sink in the water (units: 1/16 m) */
# define CRATE_COUNT        (3)
# define CRATE_HALF         (12)
# define CRATE_SINK         (9)
/** @brief Depth bias of the crates: drawn over the water around them (the part under the surface is not hidden) */
# define CRATE_BIAS         (-48)

static jo_camera            cam;
static jo_water             *sea;
static jo_3d_mesh           *crate_mesh;
static jo_water_floating    crates[CRATE_COUNT];
static jo_angle             crate_heading[CRATE_COUNT];
static jo_angle             orbit;
static int                  height = 60;
static int                  look = JO_WATER_LOOK_TROPICAL_TEAL;
static int                  swell = JO_WATER_SWELL_CHOPPY;

/** @brief A face of the crate: 4 corners (+1 / -1 for each axis, clockwise seen from outside: the top left of the texture
 *  first), its texture */
static void                 crate_face(const int face, const int corners[4][3], const int sprite)
{
    int                     k;

    for (JO_ZERO(k); k < 4; ++k)
        jo_3d_set_mesh_vertice(crate_mesh, jo_int2fixed(corners[k][0] * CRATE_HALF), jo_int2fixed(corners[k][1] * CRATE_HALF),
                               jo_int2fixed(corners[k][2] * CRATE_HALF), (unsigned int)(face * 4 + k));
    jo_3d_set_mesh_polygon_texture(crate_mesh, sprite, (unsigned int)face);
}

/** @brief The crate: a box of 6 faces (y down: the top is at y = -CRATE_HALF), its textures (cd/CRATE.TGA, CRATETOP.TGA:
 *  tools/make_crate.py), the top in the sun
 *  @return false if a texture is missing */
static bool                 create_crate(void)
{
    static const int        faces[6][4][3] = {
        {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}},       // Front (-z)
        {{1, -1, 1}, {-1, -1, 1}, {-1, 1, 1}, {1, 1, 1}},           // Back (+z)
        {{-1, -1, 1}, {-1, -1, -1}, {-1, 1, -1}, {-1, 1, 1}},       // Left (-x)
        {{1, -1, -1}, {1, -1, 1}, {1, 1, 1}, {1, 1, -1}},           // Right (+x)
        {{-1, -1, 1}, {1, -1, 1}, {1, -1, -1}, {-1, -1, -1}},       // Top (-y)
        {{-1, 1, -1}, {1, 1, -1}, {1, 1, 1}, {-1, 1, 1}}            // Bottom (+y)
    };
    int                     side;
    int                     top;
    int                     k;

    if ((side = jo_sprite_add_tga(JO_ROOT_DIR, "CRATE.TGA", JO_COLOR_Transparent)) < 0 ||
        (top = jo_sprite_add_tga(JO_ROOT_DIR, "CRATETOP.TGA", JO_COLOR_Transparent)) < 0)
        return (false);
    crate_mesh = jo_3d_create_mesh(6);
    for (JO_ZERO(k); k < 6; ++k)
        crate_face(k, faces[k], k == 4 ? top : side);
    return (true);
}

/** @brief Drop the crates from above the water around the middle */
static void                 drop_crates(void)
{
    int                     k;

    for (JO_ZERO(k); k < CRATE_COUNT; ++k)
    {
        jo_water_floating_init(&crates[k], (k - 1) * 48, (k & 1) * 40, 60 + k * 30, CRATE_HALF, CRATE_SINK);
        crate_heading[k] = (jo_angle)(k * 12000);
    }
}

/** @brief Draw the crates (the master CPU, while the slave CPU draws the water near the camera) */
static void                 draw_crates(void)
{
    int                     k;

    jo_3d_set_depth_bias(CRATE_BIAS);
    for (JO_ZERO(k); k < CRATE_COUNT; ++k)
    {
        jo_3d_push_matrix();
        {
            jo_3d_translate_matrix(crates[k].x, -jo_fixed2int(crates[k].y), crates[k].z);
            jo_3d_rotate_matrix_angle_x(crates[k].tilt_x);
            jo_3d_rotate_matrix_angle_z(crates[k].tilt_z);
            jo_3d_rotate_matrix_angle_y(crate_heading[k]);
            jo_3d_mesh_draw(crate_mesh);
        }
        jo_3d_pop_matrix();
    }
    jo_3d_set_depth_bias(0);
}

/** @brief A new look or a new swell (the pad) */
static void                 change_water(void)
{
    jo_water_config         config;
    jo_water_wave           waves[JO_WATER_MAX_WAVES];

    config = *jo_water_get_config(sea);
    jo_water_config_look(&config, (jo_water_look_kind)look);
    jo_water_set_look(sea, &config);
    jo_water_set_waves(sea, waves, jo_water_swell(waves, (jo_water_swell_kind)swell));
}

static void                 my_draw(void)
{
    int                     cam_x;
    int                     cam_z;
    int                     k;

    // The pad
    if (jo_is_pad1_key_pressed(JO_KEY_LEFT))
        orbit += 200;
    if (jo_is_pad1_key_pressed(JO_KEY_RIGHT))
        orbit -= 200;
    if (jo_is_pad1_key_pressed(JO_KEY_UP))
        height = JO_MIN(200, height + 1);
    if (jo_is_pad1_key_pressed(JO_KEY_DOWN))
        height = JO_MAX(16, height - 1);
    if (jo_is_pad1_key_down(JO_KEY_A))
        drop_crates();
    if (jo_is_pad1_key_down(JO_KEY_B))
    {
        look = (look + 1) % JO_WATER_LOOK_COUNT;
        change_water();
    }
    if (jo_is_pad1_key_down(JO_KEY_C))
    {
        swell = (swell + 1) % JO_WATER_SWELL_COUNT;
        change_water();
    }
    orbit += 20;
    // The camera: around the crates, looking at them (y down: the camera above the water is at y < 0)
    cam_x = jo_fixed2int(jo_sin_angle(orbit) * 220);
    cam_z = jo_fixed2int(jo_cos_angle(orbit) * 220);
    jo_3d_camera_set_viewpoint(&cam, cam_x, -height, cam_z);
    jo_3d_camera_set_target(&cam, 0, 0, 0);
    // The waves at the time of the frame, the crates on them, then the water and the crates drawn
    jo_water_update(sea, (int)jo_core_get_vblank_count(), cam_x, -height, cam_z, (jo_angle)(orbit + 32768));
    for (JO_ZERO(k); k < CRATE_COUNT; ++k)
        jo_water_float(sea, &crates[k]);
    jo_3d_camera_look_at(&cam);
    jo_water_draw_with(sea, draw_crates, JO_NULL);
    jo_printf(1, 1, "Water: arrows, A drop, B look, C swell");
}

void                        jo_main(void)
{
    jo_water_config         config;

    // (the sky: the color of the back screen, behind the water)
    jo_core_init(JO_COLOR_RGB(120, 180, 236));
    jo_3d_camera_init(&cam);
    // The water: the default configuration, a look (see jo_water_config_look())
    jo_water_default_config(&config);
    jo_water_config_look(&config, (jo_water_look_kind)look);
    if ((sea = jo_water_create(&config)) == JO_NULL)
        return;
    if (!create_crate())
        return;
    drop_crates();
    jo_core_add_callback(my_draw);
    jo_core_run();
}

/*
** END OF FILE
*/
