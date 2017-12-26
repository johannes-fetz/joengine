/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2013, Johannes Fetz (johannesfetz@gmail.com)
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
static int map_pos_y = -160;
static int hitbox_sprite_id;

#define WORLD_MAP_ID		(0)
#define HITBOX_POS_X        (JO_TV_WIDTH_2)
#define HITBOX_POS_Y        (JO_TV_HEIGHT_2)
#define HITBOX_WIDTH        (32)
#define HITBOX_HEIGHT       (32)

void     collision_detection(void)
{
    jo_clear_screen();
    /*
        Per-pixel vertical collision from the top-left corner of our box
    */
    jo_printf_with_color(0, 0, JO_COLOR_INDEX_White, "Top-left corner per-pixel collision:");
    int dist = jo_map_per_pixel_vertical_collision(WORLD_MAP_ID, map_pos_x + HITBOX_POS_X, map_pos_y + HITBOX_POS_Y, JO_NULL);
    if (dist != JO_MAP_NO_COLLISION)
    {
        if (dist >= 0)
            jo_printf_with_color(1, 1, JO_COLOR_INDEX_Yellow, "> Distance from ground: %d  ", dist);
        else
            jo_printf_with_color(1, 1, JO_COLOR_INDEX_Green, "> Distance from surface: %d  ", dist);
    }
    else
        jo_printf_with_color(1, 1, JO_COLOR_INDEX_Red, "> No hit");

    /*
        Detect collision between our red square and any tile on the map
        Warning: It's not per-pixel collision so sometimes you will see "Hit!" but the square don't visually touch the map.
                 Just remember that tiles are square too and this method doesn't handle transparent pixels.
                 Btw, I'll add another one for per-pixel collision soon :D
     */
    jo_printf_with_color(0, 3, JO_COLOR_INDEX_White, "Hitbox collision:");
	if (jo_map_hitbox_detection_custom_boundaries(WORLD_MAP_ID, map_pos_x + HITBOX_POS_X, map_pos_y + HITBOX_POS_Y, HITBOX_WIDTH, HITBOX_HEIGHT) != JO_MAP_NO_COLLISION)
        jo_printf_with_color(1, 4, JO_COLOR_INDEX_Green, "> Hit!");
    else
        jo_printf_with_color(1, 4, JO_COLOR_INDEX_Red, "> No hit");
}

void			my_draw(void)
{
	jo_map_draw(WORLD_MAP_ID, map_pos_x, map_pos_y);
    jo_sprite_draw3D2(hitbox_sprite_id, HITBOX_POS_X, HITBOX_POS_Y, 400);
    collision_detection();
}

void			my_gamepad(void)
{
	if (!jo_is_pad1_available())
		return ;
	if (jo_is_pad1_key_pressed(JO_KEY_LEFT))
		map_pos_x -= 2;
	if (jo_is_pad1_key_pressed(JO_KEY_RIGHT))
		map_pos_x += 2;
	if (jo_is_pad1_key_pressed(JO_KEY_UP))
		map_pos_y -= 2;
	if (jo_is_pad1_key_pressed(JO_KEY_DOWN))
		map_pos_y += 2;
}

void            load_background()
{
    jo_img      bg;

    bg.data = JO_NULL;
    jo_tga_loader(&bg, "BG", "BG.TGA", JO_COLOR_Transparent);
    jo_set_background_sprite(&bg, 0, 0);
    jo_free_img(&bg);
}

void			load_sprites()
{
	jo_sprite_add_image_pack("BLK", "BLK.TEX", JO_COLOR_Red);
	hitbox_sprite_id = jo_sprite_add_tga(JO_ROOT_DIR, "HITBOX.TGA", JO_COLOR_Transparent);
}

void			load_map()
{
	jo_map_load_from_file(WORLD_MAP_ID, 450, "MAP", "DEMO.MAP");
}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_Black);
	load_sprites();
	load_map();
    load_background();
	jo_core_add_callback(my_gamepad);
	jo_core_add_callback(my_draw);
	jo_core_run();
}

/*
** END OF FILE
*/
