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

static int wheel_sprite = 0;

jo_fixed axis_x = 0;

void			my_draw(void)
{
	if (jo_get_input_type(0) == JoRacingWheel)
	{
		jo_printf(3, 4, "Connected: YES");
	}
	else
	{
		jo_printf(3, 4, "Connected: NO ");
	}

	// Draw thumb stick cross
	jo_sprite_draw3D_and_rotate(wheel_sprite, 0, 0, 500, axis_x);

}

void			my_gamepad(void)
{
	if (jo_get_input_type(0) == JoRacingWheel)
	{
		// X and Y are centered at 128
		char axis_x_raw = jo_get_input_axis(0, JoAxis1) - 0x80;

		// Scale input to the 90°deg rotation
		axis_x = JO_DIV_BY_65536(jo_fixed_mult(JO_MULT_BY_256(axis_x_raw), JO_MULT_BY_65536(90)));
	}
}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_Black);

	jo_printf(12, 2, "Demo - Analog");

	wheel_sprite = jo_sprite_add_tga(JO_ROOT_DIR, "WHEEL.TGA", JO_COLOR_Green);

	jo_core_add_callback(my_gamepad);
	jo_core_add_callback(my_draw);

	jo_core_run();
}

/*
** END OF FILE
*/
