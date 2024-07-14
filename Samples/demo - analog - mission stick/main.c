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

#define BOX_SIZE (128)

static int cross_sprite = 0;
static int throttle_sprite = 0;
static int throttle_bar_sprite = 0;

jo_fixed axis_x = 0;
jo_fixed axis_y = 0;
jo_fixed throttle = 0;

void			my_draw(void)
{
	if (jo_get_input_type(0) == JoMissionStick)
	{
		jo_printf(3, 4, "Connected: YES");
	}
	else
	{
		jo_printf(3, 4, "Connected: NO ");
	}

	// Draw the trigger bar background
	jo_sprite_draw3D(throttle_bar_sprite, -140, 0, 500);

	// Draw thumb stick cross
	jo_sprite_draw3D(cross_sprite, JO_DIV_BY_65536(axis_x), JO_DIV_BY_65536(axis_y), 500);

	// Draw left and right trigger
	jo_sprite_draw3D(throttle_sprite, -140, -JO_DIV_BY_65536(throttle) + 64, 500);
}

void			my_gamepad(void)
{
	if (jo_get_input_type(0) == JoMissionStick)
	{
		// X and Y are centered at 128
		char axis_x_raw = jo_get_input_axis(0, JoAxis1) - 0x80;
		char axis_y_raw = jo_get_input_axis(0, JoAxis2) - 0x80;

		// Left and right trigger start at 0 and go to 255
		unsigned char throttle_raw = jo_get_input_axis(0, JoAxis3);

		// Scale input to the size of the box on screen
		axis_x = jo_fixed_mult(JO_MULT_BY_256(axis_x_raw), JO_MULT_BY_65536(BOX_SIZE));
		axis_y = jo_fixed_mult(JO_MULT_BY_256(axis_y_raw), JO_MULT_BY_65536(BOX_SIZE));
		throttle = jo_fixed_mult(JO_MULT_BY_256(throttle_raw), JO_MULT_BY_65536(BOX_SIZE));
	}
}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_Black);

	jo_printf(12, 2, "Demo - Analog");

	cross_sprite = jo_sprite_add_tga(JO_ROOT_DIR, "CROSS.TGA", JO_COLOR_Green);
	throttle_sprite = jo_sprite_add_tga(JO_ROOT_DIR, "TRIG.TGA", JO_COLOR_Green);
	throttle_bar_sprite = jo_sprite_add_tga(JO_ROOT_DIR, "TRIGBAR.TGA", JO_COLOR_Green);

	jo_core_add_callback(my_gamepad);
	jo_core_add_callback(my_draw);

	jo_core_run();
}

/*
** END OF FILE
*/
