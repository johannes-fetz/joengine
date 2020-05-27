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

static int gamepad_sprite_id;
static int arrow_sprite_id;

void			my_draw(void)
{
	jo_sprite_draw3D(gamepad_sprite_id, 0, 0, 500);
}

void			my_gamepad(void)
{
	if (!jo_is_input_available(0))
    {
        jo_set_screen_color_filter_a(JO_SPRITE_SCREEN, 255, 0, 0); // All sprite in red if the controller in not plugged in.
        return ;
    }
    else
        jo_disable_all_screen_color_filter();

    // Note: You can also use jo_is_input_key_* for direction
    switch (jo_get_input_direction_pressed(0))
    {
        case LEFT: jo_sprite_draw3D(arrow_sprite_id, -75, -3, 450); break;
        case RIGHT: jo_sprite_draw3D(arrow_sprite_id, -45, -3, 450); break;
        case UP: jo_sprite_draw3D(arrow_sprite_id, -60, -15, 450); break;
        case DOWN: jo_sprite_draw3D(arrow_sprite_id, -60, 12, 450); break;
        case UP_LEFT: jo_sprite_draw3D(arrow_sprite_id, -72, -12, 450); break;
        case UP_RIGHT: jo_sprite_draw3D(arrow_sprite_id, -48, -12, 450); break;
        case DOWN_LEFT: jo_sprite_draw3D(arrow_sprite_id, -72, 9, 450); break;
        case DOWN_RIGHT: jo_sprite_draw3D(arrow_sprite_id, -48, 9, 450); break;
        case NONE: break;
    }
    if (jo_is_input_key_pressed(0, JO_KEY_START))
        jo_sprite_draw3D(arrow_sprite_id, 0, 17, 450);

    if (jo_is_input_key_pressed(0, JO_KEY_L))
        jo_sprite_draw3D(arrow_sprite_id, -50, -60, 450);
    if (jo_is_input_key_pressed(0, JO_KEY_R))
        jo_sprite_draw3D(arrow_sprite_id, 50, -60, 450);

    if (jo_is_input_key_pressed(0, JO_KEY_A))
        jo_sprite_draw3D(arrow_sprite_id, 41, 20, 450);
    if (jo_is_input_key_pressed(0, JO_KEY_B))
        jo_sprite_draw3D(arrow_sprite_id, 63, 4, 450);
    if (jo_is_input_key_pressed(0, JO_KEY_C))
        jo_sprite_draw3D(arrow_sprite_id, 88, -4, 450);

    if (jo_is_input_key_pressed(0, JO_KEY_X))
        jo_sprite_draw3D(arrow_sprite_id, 33, -7, 450);
    if (jo_is_input_key_pressed(0, JO_KEY_Y))
        jo_sprite_draw3D(arrow_sprite_id, 51, -20, 450);
    if (jo_is_input_key_pressed(0, JO_KEY_Z))
        jo_sprite_draw3D(arrow_sprite_id, 72, -28, 450);
}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_White);
	gamepad_sprite_id = jo_sprite_add_tga(JO_ROOT_DIR, "PAD.TGA", JO_COLOR_Green);
	arrow_sprite_id = jo_sprite_add_tga(JO_ROOT_DIR, "ARW.TGA", JO_COLOR_Green);
	jo_core_add_callback(my_gamepad);
	jo_core_add_callback(my_draw);
	jo_core_run();
}

/*
** END OF FILE
*/
