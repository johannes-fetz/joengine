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

/*
** Jo Engine multitap demo contributed by Slinga of the Sega Saturn Multitap
** Task Force (SSMTF)
*/

#include <jo/jo.h>

static int gamepad_sprite_id;
static int arrow_sprite_id;
static int x_sprite_id;

void			my_draw(void)
{
	// multitap 1
	jo_print(3, 3, "P1");
	jo_sprite_draw3D(gamepad_sprite_id, -72, -100, 500);
	jo_print(3, 8, "P2");
	jo_sprite_draw3D(gamepad_sprite_id, -72, -60, 500);
	jo_print(3, 13, "P3");
	jo_sprite_draw3D(gamepad_sprite_id, -72, -20, 500);
	jo_print(3, 18, "P4");
	jo_sprite_draw3D(gamepad_sprite_id, -72, 20, 500);
	jo_print(3, 23, "P5");
	jo_sprite_draw3D(gamepad_sprite_id, -72, 60, 500);
	jo_print(3, 28, "P6");
	jo_sprite_draw3D(gamepad_sprite_id, -72, 100, 500);

	// multitap 2
	jo_print(21, 3, "P7");
	jo_sprite_draw3D(gamepad_sprite_id, 72, -100, 500);
	jo_print(21, 8, "P8");
	jo_sprite_draw3D(gamepad_sprite_id, 72, -60, 500);
	jo_print(21, 13, "P9");
	jo_sprite_draw3D(gamepad_sprite_id, 72, -20, 500);
	jo_print(21, 18, "P10");
	jo_sprite_draw3D(gamepad_sprite_id, 72, 20, 500);
	jo_print(21, 23, "P11");
	jo_sprite_draw3D(gamepad_sprite_id, 72, 60, 500);
	jo_print(21, 28, "P12");
	jo_sprite_draw3D(gamepad_sprite_id, 72, 100, 500);
}

void			my_gamepad(void)
{
	// loop through all 12 controllers
	// requires the use of two 6-player multitaps for full 12 player support
	// controllerID is the controller port and can be from 0-11
	// - ports 0-5 are on the multitap plugged into the first controller port
	// - ports 6-11 are on the multitap plugged into the second controller port
	for(int controllerID = 0; controllerID < JO_INPUT_MAX_DEVICE; controllerID++)
	{		
		int sprite_x_pos;
		int sprite_y_pos;
		
		// sprite offsets
		if(controllerID < 6)
		{
			sprite_x_pos = -72;
			sprite_y_pos = -100 + (controllerID*40);
		}
		else
		{
			sprite_x_pos = 72;
			sprite_y_pos = -100 + ((controllerID-6)*40);
		}

		if (!jo_is_input_available(controllerID))
		{
			// if the controller was not found, print a giant X
		    jo_sprite_draw3D(x_sprite_id, sprite_x_pos, sprite_y_pos, 450);
			continue;
		}

		// Note: You can also use jo_is_input_key_* for direction
		switch (jo_get_input_direction_pressed(controllerID))
		{
		    case LEFT: jo_sprite_draw3D(arrow_sprite_id, sprite_x_pos - 25, sprite_y_pos - 5, 450); break;
		    case RIGHT: jo_sprite_draw3D(arrow_sprite_id, sprite_x_pos- 13, sprite_y_pos - 5, 450); break;
		    case UP: jo_sprite_draw3D(arrow_sprite_id, sprite_x_pos - 19, sprite_y_pos - 10, 450); break;
		    case DOWN: jo_sprite_draw3D(arrow_sprite_id, sprite_x_pos - 19, sprite_y_pos + 1, 450); break;
		    case UP_LEFT: jo_sprite_draw3D(arrow_sprite_id, sprite_x_pos - 22, sprite_y_pos - 8, 450); break;
		    case UP_RIGHT: jo_sprite_draw3D(arrow_sprite_id, sprite_x_pos - 16, sprite_y_pos - 8, 450); break;
		    case DOWN_LEFT: jo_sprite_draw3D(arrow_sprite_id, sprite_x_pos - 22, sprite_y_pos - 1, 450); break;
		    case DOWN_RIGHT: jo_sprite_draw3D(arrow_sprite_id, sprite_x_pos - 16, sprite_y_pos - 1, 450); break;
		    case NONE: break;
		}

		if (jo_is_input_key_pressed(controllerID, JO_KEY_START))
		    jo_sprite_draw3D(arrow_sprite_id, sprite_x_pos, sprite_y_pos, 450);

		if (jo_is_input_key_pressed(controllerID, JO_KEY_L))
		    jo_sprite_draw3D(arrow_sprite_id, sprite_x_pos - 18, sprite_y_pos - 20, 450);
		if (jo_is_input_key_pressed(controllerID, JO_KEY_R))
		    jo_sprite_draw3D(arrow_sprite_id, sprite_x_pos + 18, sprite_y_pos - 20, 450);

		if (jo_is_input_key_pressed(controllerID, JO_KEY_A))
		    jo_sprite_draw3D(arrow_sprite_id, sprite_x_pos + 12, sprite_y_pos + 1, 450);
		if (jo_is_input_key_pressed(controllerID, JO_KEY_B))
		    jo_sprite_draw3D(arrow_sprite_id, sprite_x_pos + 19, sprite_y_pos - 4, 450);
		if (jo_is_input_key_pressed(controllerID, JO_KEY_C))
		    jo_sprite_draw3D(arrow_sprite_id, sprite_x_pos + 27, sprite_y_pos - 7, 450);

		if (jo_is_input_key_pressed(controllerID, JO_KEY_X))
		    jo_sprite_draw3D(arrow_sprite_id, sprite_x_pos + 10, sprite_y_pos - 8, 450);
		if (jo_is_input_key_pressed(controllerID, JO_KEY_Y))
		    jo_sprite_draw3D(arrow_sprite_id, sprite_x_pos + 16, sprite_y_pos - 12, 450);
		if (jo_is_input_key_pressed(controllerID, JO_KEY_Z))
		    jo_sprite_draw3D(arrow_sprite_id, sprite_x_pos + 22, sprite_y_pos - 14, 450);

	}
}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_White);
	gamepad_sprite_id = jo_sprite_add_tga(JO_ROOT_DIR, "PAD.TGA", JO_COLOR_Green);
	arrow_sprite_id = jo_sprite_add_tga(JO_ROOT_DIR, "ARW.TGA", JO_COLOR_Green);
	x_sprite_id = jo_sprite_add_tga(JO_ROOT_DIR, "X.TGA", JO_COLOR_Green);
	jo_core_add_callback(my_gamepad);
	jo_core_add_callback(my_draw);
	jo_core_run();
}

/*
** END OF FILE
*/
