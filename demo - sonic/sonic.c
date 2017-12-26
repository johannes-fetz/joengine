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

sonic_t player;
extern jo_sidescroller_physics_params  physics;

static const jo_tile WalkingSonicTiles[] =
{
    {SONIC_WIDTH * 0, 0, SONIC_WIDTH, SONIC_HEIGHT},
    {SONIC_WIDTH * 1, 0, SONIC_WIDTH, SONIC_HEIGHT},
    {SONIC_WIDTH * 2, 0, SONIC_WIDTH, SONIC_HEIGHT},
    {SONIC_WIDTH * 3, 0, SONIC_WIDTH, SONIC_HEIGHT},
    {SONIC_WIDTH * 4, 0, SONIC_WIDTH, SONIC_HEIGHT},
    {SONIC_WIDTH * 5, 0, SONIC_WIDTH, SONIC_HEIGHT},
    {SONIC_WIDTH * 6, 0, SONIC_WIDTH, SONIC_HEIGHT},
    {SONIC_WIDTH * 7, 0, SONIC_WIDTH, SONIC_HEIGHT},
};

inline void     sonic_running_animation_handling(void)
{
    int         speed_step;

    if (jo_physics_is_standing(&physics))
    {
        if (!jo_is_sprite_anim_stopped(player.walking_anim_id))
            jo_reset_sprite_anim(player.walking_anim_id);
    }
    else
    {
        if (jo_is_sprite_anim_stopped(player.walking_anim_id))
            jo_start_sprite_anim_loop(player.walking_anim_id);
        speed_step = (int)JO_ABS(physics.speed);
        if (speed_step >= 6)
            jo_set_sprite_anim_frame_rate(player.walking_anim_id, 3);
        else if (speed_step >= 5)
            jo_set_sprite_anim_frame_rate(player.walking_anim_id, 4);
        else if (speed_step >= 4)
            jo_set_sprite_anim_frame_rate(player.walking_anim_id, 5);
        else if (speed_step >= 3)
            jo_set_sprite_anim_frame_rate(player.walking_anim_id, 6);
        else if (speed_step >= 2)
            jo_set_sprite_anim_frame_rate(player.walking_anim_id, 7);
        else if (speed_step >= 1)
            jo_set_sprite_anim_frame_rate(player.walking_anim_id, 8);
        else
            jo_set_sprite_anim_frame_rate(player.walking_anim_id, 9);
    }
}

inline void     display_sonic(void)
{
    if (!physics.is_in_air)
    {
        player.spin = false;
        player.angle = 0;
    }
    if (player.flip_sonic)
        jo_sprite_enable_horizontal_flip();
    if (player.spin)
    {
        jo_sprite_draw3D_and_rotate2(player.spin_sprite_id, player.x, player.y, 450, player.angle);
        if (player.flip_sonic)
            player.angle -= SONIC_SPIN_SPEED;
        else
            player.angle += SONIC_SPIN_SPEED;
    }
    else
        jo_sprite_draw3D2(jo_get_anim_sprite(player.walking_anim_id), player.x, player.y, 450);
    if (player.flip_sonic)
        jo_sprite_disable_horizontal_flip();
}

void            load_sonic(void)
{
    int         sprite_id;

    sprite_id = jo_sprite_add_bin_tileset(JO_ROOT_DIR, "SNC.BIN", JO_COLOR_Green, WalkingSonicTiles, JO_TILE_COUNT(WalkingSonicTiles));
	player.walking_anim_id = jo_create_sprite_anim(sprite_id, JO_TILE_COUNT(WalkingSonicTiles), 4);
    player.spin_sprite_id = jo_sprite_add_bin(JO_ROOT_DIR, "SNC3.BIN", JO_COLOR_Green);
}

/*
** END OF FILE
*/
