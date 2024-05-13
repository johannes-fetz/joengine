/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2024, Johannes Fetz (johannesfetz@gmail.com)
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

static int map_pos_x = 0;
static int map_pos_y = 0;

#define WORLD_MAP_ID		(0)

void			my_draw(void)
{
    jo_vdp2_clear_bitmap_nbg1(JO_COLOR_Black);
    jo_vdp2_map_draw_nbg1(WORLD_MAP_ID, map_pos_x, map_pos_y);
    //You can also use jo_vdp2_move_nbg1(), jo_enable_screen_mozaic(), etc after
}

void			my_gamepad(void)
{
	if (!jo_is_pad1_available())
		return ;
	if (jo_is_pad1_key_pressed(JO_KEY_LEFT))
		map_pos_x += 2;
	if (jo_is_pad1_key_pressed(JO_KEY_RIGHT))
		map_pos_x -= 2;
	if (jo_is_pad1_key_pressed(JO_KEY_UP))
		map_pos_y += 2;
	if (jo_is_pad1_key_pressed(JO_KEY_DOWN))
		map_pos_y -= 2;
}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_Black);
	jo_sprite_add_image_pack("BLK", "BLK.TEX", JO_COLOR_Red);
	jo_map_load_from_file(WORLD_MAP_ID, 450, "MAP", "DEMO.MAP");

	jo_core_add_callback(my_gamepad);
	jo_core_add_callback(my_draw);
	jo_core_run();
}

/*
** END OF FILE
*/
