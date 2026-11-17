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
** Vertex animation (morph targets): a character who walks
**
** The skin of the character bends at the knees, the elbows, the hips and the shoulders: each frame of the walk has the
** position of each point (a skeleton moves rigid parts, see jo_3d_model_draw_pose()). The engine interpolates the points
** between 2 frames (jo_3d_model_draw_morph()). The light is baked in the vertex colors of the model (smooth gouraud
** shading); with lit materials (jo_light), the normals of the polygons would follow the frames.
**
** The character, its walk and the ground are made by models/make_walker.py (Python 3, numpy and Pillow): a mesh skinned to a
** skeleton, one OBJ per frame (a 3D software exports the same: Blender, "Animation" in the OBJ exporter), then converted by
** the converter of the Jo Engine:
**
**     python3 models/make_walker.py
**     python3 ../../Compiler/COMMON/jo_model.py models/walker.obj -o cd/WALKER.JOM --scale 40 --embed-textures --morph models/walk.json
**     python3 ../../Compiler/COMMON/jo_model.py models/ground.obj -o cd/GROUND.JOM --scale 40 --embed-textures
**
** models/walk.json lists the frames of the animation "walk" (see Compiler/COMMON/jo_model.py). walker.h: the distance walked
** in a cycle, the size of a tile of the ground. The character stays at the origin, the ground moves under him at the speed of the walk (his feet on the ground
** stay still); the ground repeats itself every tile, so it moves by less than a tile.
**
** Pad: left and right turn the camera around the character, up and down move it nearer or farther, A pauses the walk, B
** shows the pose of the OBJ (no animation)
*/

#include <jo/jo.h>
#include "walker.h"

/* The camera: its distance to the character, its height, the height it looks at (the chest), its first angle (the
   character seen from the front left) */
#define CAMERA_DISTANCE             (95)
#define CAMERA_DISTANCE_MIN         (60)
#define CAMERA_DISTANCE_MAX         (260)
#define CAMERA_DISTANCE_STEP        (2)
#define CAMERA_HEIGHT               (58)
#define CAMERA_TARGET_HEIGHT        (46)
#define CAMERA_START_ANGLE          (JO_DEG_TO_ANGLE(150))
#define CAMERA_TURN_STEP            (JO_DEG_TO_ANGLE(1))

static jo_camera                    cam;
static jo_3d_model                  *walker;
static jo_3d_model                  *ground;
static int                          walk;               /* Index of the animation "walk" */
static int                          walk_ticks;         /* Time in the animation (1/60 s) */
static jo_fixed                     ground_offset;      /* Where the ground is under the character (0 to a tile) */
static jo_fixed                     ground_per_tick;
static jo_angle                     camera_angle = CAMERA_START_ANGLE;
static int                          camera_distance = CAMERA_DISTANCE;
static unsigned int                 last_vblank;
static bool                         paused;
static bool                         obj_pose;

/* Ticks of the animations (1/60 s) since the last frame, PAL or NTSC */
static int                          elapsed_ticks(void)
{
    static int                      remainder;
    unsigned int                    now;
    int                             ticks;

    now = jo_core_get_vblank_count();
    remainder += (int)(now - last_vblank) * JO_3D_ANIMATION_TICKS_PER_SECOND;
    last_vblank = now;
    ticks = remainder / jo_core_get_refresh_rate();
    remainder -= ticks * jo_core_get_refresh_rate();
    return (ticks);
}

static void                         update(void)
{
    int                             ticks;

    ticks = elapsed_ticks();
    if (jo_is_pad1_key_pressed(JO_KEY_LEFT))
        camera_angle -= CAMERA_TURN_STEP;
    if (jo_is_pad1_key_pressed(JO_KEY_RIGHT))
        camera_angle += CAMERA_TURN_STEP;
    if (jo_is_pad1_key_pressed(JO_KEY_UP) && camera_distance > CAMERA_DISTANCE_MIN)
        camera_distance -= CAMERA_DISTANCE_STEP;
    if (jo_is_pad1_key_pressed(JO_KEY_DOWN) && camera_distance < CAMERA_DISTANCE_MAX)
        camera_distance += CAMERA_DISTANCE_STEP;
    if (jo_is_pad1_key_down(JO_KEY_A))
        paused = !paused;
    if (jo_is_pad1_key_down(JO_KEY_B))
        obj_pose = !obj_pose;
    if (paused || obj_pose)
        return;
    // (the time stays in the cycle: the animation loops)
    walk_ticks = (walk_ticks + ticks) % jo_3d_model_morph_duration(walker, walk);
    // The character walks toward -Z (the front of the model): the ground moves toward +Z, by less than a tile
    ground_offset += ground_per_tick * ticks;
    while (ground_offset >= WALKER_GROUND_TILE)
        ground_offset -= WALKER_GROUND_TILE;
}

static void                         my_draw(void)
{
    update();
    jo_3d_camera_set_viewpoint(&cam, jo_fixed2int(camera_distance * jo_sin_angle(camera_angle)), -CAMERA_HEIGHT,
                               jo_fixed2int(camera_distance * jo_cos_angle(camera_angle)));
    jo_3d_camera_set_target(&cam, 0, -CAMERA_TARGET_HEIGHT, 0);
    jo_3d_camera_look_at(&cam);
    jo_3d_push_matrix();
    {
        jo_3d_translate_matrix_fixed(0, 0, ground_offset);
        jo_3d_model_draw(ground);
    }
    jo_3d_pop_matrix();
    jo_3d_model_draw_morph(walker, obj_pose ? -1 : walk, walk_ticks);
    jo_printf(9, 1, "*Vertex animation demo*");
    jo_printf(0, 25, "WALKER.JOM: %d points, %d polygons  ", walker->mesh->data.point_count, walker->mesh->data.polygon_count);
    jo_printf(0, 26, "Frame %d / %d%s    ", walk_ticks / walker->morphs[walk].frame_ticks, walker->morphs[walk].frame_count,
              obj_pose ? " (pose of the OBJ)" : paused ? " (paused)" : "");
    jo_printf(0, 27, "%d FPS  ", jo_core_get_fps());
}

void                                jo_main(void)
{
    jo_core_init(JO_COLOR_RGB(150, 186, 228));
    jo_3d_camera_init(&cam);
    ground = jo_3d_load_model("GROUND.JOM");
    walker = jo_3d_load_model("WALKER.JOM");
    walk = jo_3d_model_find_morph(walker, "walk");
    // The distance walked in a tick: the distance of a cycle / the ticks of a cycle
    ground_per_tick = WALKER_CYCLE_DISTANCE / jo_3d_model_morph_duration(walker, walk);
    last_vblank = jo_core_get_vblank_count();
    jo_core_add_callback(my_draw);
    jo_core_run();
}

/*
** END OF FILE
*/
