/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2020, Johannes Fetz (johannesfetz@gmail.com)
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

int                     source_sprite_id;
int                     target_sprite_id;
jo_homing_laser         laser;

static void             compute_random_target(void)
{
    laser.target.x = jo_int2fixed(jo_random(280) - 140);
    laser.intermediate_point_a.x = (laser.source.x + laser.target.x) / 2;
}

static void             my_draw(void)
{
    jo_sprite_draw3D(target_sprite_id, jo_fixed2int(laser.target.x), jo_fixed2int(laser.target.y), 400);
    jo_sprite_draw3D(source_sprite_id, jo_fixed2int(laser.source.x), jo_fixed2int(laser.source.y) - 18, 400);

    // You can use any effect on sprite additionally like jo_sprite_enable_screen_doors_filter();
    jo_draw_homing_laser(&laser);

    laser.length += jo_float2fixed(0.05); // Laser speed
    if (laser.length > JO_FIXED_1)
    {
        laser.length = 0;
        compute_random_target();
    }
}

void                    jo_main(void)
{
    int                 laser_sprite_id;

	jo_core_init(JO_COLOR_Black);
    jo_printf(10, 28, "*Homing laser demo*");

    source_sprite_id = jo_sprite_add_tga("TEX", "SHIP.TGA", JO_COLOR_Blue);
    target_sprite_id = jo_sprite_add_tga("TEX", "EN.TGA", JO_COLOR_Blue);
    laser_sprite_id = jo_sprite_add_tga("TEX", "BER.TGA", JO_COLOR_Transparent);

    jo_init_homing_laser(&laser, laser_sprite_id, 8, 500, 20);
    laser.use_intermediate_point_b = false; // We only use 3 positions for this demo (quadratic bezier curve)
    laser.source.x = jo_int2fixed(0);
    laser.source.y = jo_int2fixed(20);
    laser.intermediate_point_a.y = jo_int2fixed(120);
    laser.target.y = jo_int2fixed(-110);

    compute_random_target();

	jo_core_add_callback(my_draw);
	jo_core_run();
}

/*
** END OF FILE
*/
