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

static int cursor_sprite_id = 0;
static int cursor_pos_x = 50;
static int cursor_pos_y = 50;

void                    draw_bezier_curve(void)
{
    jo_vector_fixed     p0;
    jo_vector_fixed     p1;
    jo_vector_fixed     p2;
    jo_vector_fixed     p3;
    jo_vector_fixed     p;
    jo_fixed            t;

    p0.x = jo_int2fixed(15);
    p0.y = jo_int2fixed(50);
    p0.z = JO_FIXED_0;

    p1.x = jo_int2fixed(200);
    p1.y = jo_int2fixed(100);
    p1.z = JO_FIXED_0;

    p2.x = jo_int2fixed(300);
    p2.y = jo_int2fixed(200);
    p2.z = JO_FIXED_0;

    p3.x = jo_int2fixed(150);
    p3.y = jo_int2fixed(180);
    p3.z = JO_FIXED_0;

    for (t = JO_FIXED_0; t <= JO_FIXED_1; t += 100)
    {
        p.x = JO_FIXED_0;
        p.y = JO_FIXED_0;
        p.z = JO_FIXED_0;
        jo_vector_fixed_compute_bezier_point(t, p0, p1, p2, p3, &p);
        jo_put_pixel_in_background(jo_fixed2int(p.x), jo_fixed2int(p.y), JO_COLOR_Green);
    }
}

void			my_draw(void)
{
    jo_printf(0, 0, "Press A to draw and Start to erase all");
    jo_printf(0, 1, "Press B to draw a circle");
    jo_printf(0, 2, "Press C to draw a bezier curve");
    jo_printf(0, 28, "Cursor: %d %d   ", cursor_pos_x, cursor_pos_y);

    /* jo_sprite_draw3D2 is different from jo_sprite_draw3D() because position (0,0) is at the top left corner of the screen */
    jo_sprite_draw3D2(cursor_sprite_id, cursor_pos_x, cursor_pos_y, 490);
}

void            draw_circle_at_cursor_pos(const int radius)
{
    int         angle;

    for (angle = 0; angle < 360; ++angle)
        jo_put_pixel_in_background(cursor_pos_x + jo_cos_mult(radius, angle),
                                   cursor_pos_y + jo_sin_mult(radius, angle),
                                   JO_COLOR_Blue);
}

void			my_gamepad(void)
{
	if (!jo_is_pad1_available())
		return ;
	if (jo_is_pad1_key_pressed(JO_KEY_LEFT) && cursor_pos_x > 0)
		--cursor_pos_x;
	if (jo_is_pad1_key_pressed(JO_KEY_RIGHT)/* && cursor_pos_x < JO_TV_WIDTH*/)
		++cursor_pos_x;
	if (jo_is_pad1_key_pressed(JO_KEY_UP) && cursor_pos_y > 0)
		--cursor_pos_y;
	if (jo_is_pad1_key_pressed(JO_KEY_DOWN)/* && cursor_pos_y < JO_TV_HEIGHT*/)
		++cursor_pos_y;
    if (jo_is_pad1_key_pressed(JO_KEY_A))
        jo_put_pixel_in_background(cursor_pos_x, cursor_pos_y, JO_COLOR_Red);
    if (jo_is_pad1_key_down(JO_KEY_B))
        draw_circle_at_cursor_pos(40);
    if (jo_is_pad1_key_down(JO_KEY_C))
        draw_bezier_curve();
    if (jo_is_pad1_key_pressed(JO_KEY_START))
        jo_clear_background(JO_COLOR_White);
}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_White);
    jo_set_printf_color_index(JO_COLOR_INDEX_Black);
	cursor_sprite_id = jo_sprite_add_tga(JO_ROOT_DIR, "PEN.TGA", JO_COLOR_Red);
	jo_core_add_callback(my_gamepad);
	jo_core_add_callback(my_draw);
	jo_core_run();
}

/*
** END OF FILE
*/
