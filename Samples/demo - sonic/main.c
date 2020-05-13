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
#include "sonic.h"

/*
** SPECIAL NOTE: It's not the Sonic that I'm working on, but you can now write your own :)
*/

jo_sidescroller_physics_params  physics;
static int map_pos_x = WORLD_DEFAULT_X;
static int map_pos_y = WORLD_DEFAULT_Y;

void            reset_demo(void)
{
    map_pos_x = WORLD_DEFAULT_X;
    map_pos_y = WORLD_DEFAULT_Y;
    player.x = 160;
    player.y = 70;
    player.angle = 0;
    player.flip_sonic = false;
    player.spin = false;
    player.can_jump = true;
}

bool          has_vertical_collision(void)
{
    player.can_jump = false;
    if (physics.speed_y < 0.0f)
    {
        physics.is_in_air = true;
        return false;
    }
    int dist = jo_map_per_pixel_vertical_collision(WORLD_MAP_ID, map_pos_x + player.x + SONIC_WIDTH_2, map_pos_y + player.y + SONIC_HEIGHT, JO_NULL);
    if (dist == JO_MAP_NO_COLLISION || dist > 0)
    {
        if (dist != JO_MAP_NO_COLLISION && dist < SONIC_JUMP_PER_PIXEL_TOLERANCE)
            player.can_jump = true;
        physics.is_in_air = true;
        return false;
    }
    if (dist < 0 && jo_is_float_equals_zero(physics.speed_y))
        player.y += dist;
    player.can_jump = true;
    physics.is_in_air = false;
	return true;
}

static inline bool      has_horizontal_collision(void)
{
    int         next_pixel;

    next_pixel = jo_physics_is_going_on_the_right(&physics) ? player.x + 4 :
                 jo_physics_is_going_on_the_left(&physics) ? player.x - 4 :
                 player.x;
    int attr = jo_map_hitbox_detection_custom_boundaries(WORLD_MAP_ID, map_pos_x + next_pixel + SONIC_WIDTH_2, map_pos_y + player.y, 4, 20);
    if (attr == JO_MAP_NO_COLLISION)
        return false;
    if (attr != MAP_TILE_BLOCK_ATTR)
        return false;
    return true;
}

void     sonic_collision_handling(void)
{
    if (has_vertical_collision())
        physics.speed_y = 0.0f;
    else
    {
        jo_physics_apply_gravity(&physics);
        player.y += physics.speed_y;
    }
    if (has_horizontal_collision())
        physics.speed = 0.0f;
    else if (physics.speed > 0.0f)
        map_pos_x += physics.speed < 1.0f ? 1.0f : physics.speed;
    else if (physics.speed < 0.0f)
        map_pos_x += physics.speed > -1.0f ? -1.0f : physics.speed;
}

static inline void     camera_handling(int prev_y)
{
    int         delta;

    delta = JO_ABS(player.y - prev_y);
    if (player.y > 100)
    {
        map_pos_y += delta;
        player.y -= delta;
    }
    else if (player.y < 50)
    {
        map_pos_y -= delta;
        player.y += delta;
    }
}

static inline void     my_draw(void)
{
    int         prev_y;

    jo_map_draw(WORLD_MAP_ID, map_pos_x, map_pos_y);
    prev_y = player.y;
    sonic_collision_handling();
    camera_handling(prev_y);
    if (!physics.is_in_air)
        sonic_running_animation_handling();
    display_sonic();
}

static inline void     make_sonic_jump(void)
{
    player.can_jump = false;
    player.spin = true;
    jo_physics_jump(&physics);
}

static inline void     my_input(void)
{
    if (physics.is_in_air)
    {
        if (jo_is_pad1_key_pressed(JO_KEY_DOWN))
            player.spin = true;
        if (jo_is_pad1_key_pressed(JO_KEY_LEFT))
        {
            player.flip_sonic = true;
            jo_physics_accelerate_left(&physics);
        }
        else if (jo_is_pad1_key_pressed(JO_KEY_RIGHT))
        {
            player.flip_sonic = false;
            jo_physics_accelerate_right(&physics);
        }
    }
    else
    {
        if (jo_is_pad1_key_pressed(JO_KEY_LEFT))
        {
            player.flip_sonic = true;
            if (jo_physics_is_going_on_the_right(&physics) || jo_physics_should_brake(&physics))
                jo_physics_decelerate_left(&physics);
            else
                jo_physics_accelerate_left(&physics);
        }
        else if (jo_is_pad1_key_pressed(JO_KEY_RIGHT))
        {
            player.flip_sonic = false;
            if (jo_physics_is_going_on_the_left(&physics) || jo_physics_should_brake(&physics))
                jo_physics_decelerate_right(&physics);
            else
                jo_physics_accelerate_right(&physics);
        }
        else
            jo_physics_apply_friction(&physics);
    }
    if (jo_is_pad1_key_down(JO_KEY_START))
        reset_demo();
    if (player.can_jump && jo_is_pad1_key_down(JO_KEY_A))
        make_sonic_jump();
}

void			load_map(void)
{
    jo_sprite_add_image_pack("BLK", "BLK.TEX", JO_COLOR_Red);
	jo_map_load_from_file(WORLD_MAP_ID, 500, "MAP", "DEMO2.MAP");
}

void			load_background(void)
{
    jo_img      bg;

    bg.data = JO_NULL;
    jo_bin_loader(&bg, "BG", "BG.BIN", JO_COLOR_Transparent);
    jo_set_background_sprite(&bg, 0, 0);
    jo_free_img(&bg);
}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_Black);
    load_background();
	load_map();
    load_sonic();
    jo_physics_init_for_sonic(&physics);
    reset_demo();
    jo_printf(10, 0, "* Tiny Sonic Demo *");
	jo_core_add_callback(my_draw);
	jo_core_add_callback(my_input);
	jo_core_run();
}

/*
** END OF FILE
*/
