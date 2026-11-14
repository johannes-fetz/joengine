/*
 ** Jo Sega Saturn Engine
 ** Copyright (c) 2012-2026, Johannes Fetz (johannesfetz@gmail.com)
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

/*
** ▲ This demo shows how to use VDP2 planes without the traditional PAL/CEL/MAP files!
** You just need an 8 bits TGA image (255 colors maximum) with a 90° clockwise rotation (open SKY.TGA and you will understand)
*/

/** @brief The camera (fixed numbers, the angles in radian: no float computed) */
jo_pos3D_fixed              pos;
struct
{
    jo_fixed                rx;
    jo_fixed                ry;
    jo_fixed                rz;
}                           rot;
jo_palette                  image_pal;

void                        my_input(void)
{
    static int              movement_speed = 0;
    static jo_fixed         angle_increment = 0;

	if (jo_is_pad1_key_pressed(JO_KEY_UP))
	{
		if ((movement_speed += 10) > 60)
			movement_speed = 60;
	}
	else if (jo_is_pad1_key_pressed(JO_KEY_DOWN))
	{
		if ((movement_speed -= 10) < -60)
			movement_speed = -60;
	}
	else if (movement_speed > 5)
		movement_speed -= 5;
	else if (movement_speed < -5)
		movement_speed += 5;
	else
		movement_speed = 0;

	if (jo_is_pad1_key_pressed(JO_KEY_RIGHT))
		angle_increment += jo_float2fixed(1.5f);
	else if (jo_is_pad1_key_pressed(JO_KEY_LEFT))
        angle_increment -= jo_float2fixed(1.5f);

    if (jo_is_pad1_key_pressed(JO_KEY_L))
        pos.z += JO_FIXED_1;
	else if (jo_is_pad1_key_pressed(JO_KEY_R))
        pos.z -= JO_FIXED_1;

    rot.ry = -jo_fixed_deg2rad(angle_increment);
    rot.rz += jo_fixed_deg2rad(angle_increment) / 2;
    angle_increment = angle_increment * 4 / 5;

    pos.x -= movement_speed * jo_fixed_sin(rot.rz) / 10;
	pos.y -= movement_speed * jo_fixed_cos(rot.rz) / 10;
}

void                            draw_3d_planes(void)
{
    // FLOOR
    jo_3d_push_matrix();
	{
		jo_3d_rotate_matrix_angle(jo_fixed_rad2ANGLE(rot.rx), jo_fixed_rad2ANGLE(rot.ry), jo_fixed_rad2ANGLE(rot.rz));
		jo_3d_translate_matrix_fixed(pos.x, pos.y, pos.z);
        jo_background_3d_plane_a_draw(true);
	}
	jo_3d_pop_matrix();
    //SKY
	jo_3d_push_matrix();
	{
	    jo_3d_translate_matrix(jo_fixed_mult_to_int(rot.rz, jo_int2fixed(100)), 110 - jo_fixed_mult_to_int(jo_fixed_cos(rot.rx), jo_int2fixed(112)), 100);
        jo_3d_rotate_matrix_angle_z(jo_fixed_rad2ANGLE(-rot.ry));
        jo_background_3d_plane_b_draw(false);
	}
	jo_3d_pop_matrix();
}

jo_palette          *my_tga_palette_handling(void)
{
    // We create a new palette for each image. It's not optimal but OK for a demo.
    jo_create_palette(&image_pal);
    return (&image_pal);
}

void                init_3d_planes(void)
{
    jo_img_8bits    img;

    jo_core_tv_off();

    jo_enable_background_3d_plane(JO_COLOR_Black);

    // FLOOR
    img.data = JO_NULL;
    jo_tga_8bits_loader(&img, JO_ROOT_DIR, "FLOOR.TGA", 0);
    jo_background_3d_plane_a_img(&img, image_pal.id, true, true);
    jo_free_img(&img);

    //SKY
    img.data = JO_NULL;
    jo_tga_8bits_loader(&img, JO_ROOT_DIR, "SKY.TGA", 0);
    jo_background_3d_plane_b_img(&img, image_pal.id, true, false);
    jo_free_img(&img);

    jo_core_tv_on();
}

void                jo_main(void)
{
	jo_core_init(JO_COLOR_Black);
    jo_set_tga_palette_handling(my_tga_palette_handling);

    init_3d_planes();

    pos.x = jo_int2fixed(800);
	pos.y = jo_int2fixed(800);
	pos.z = jo_int2fixed(-35);

	rot.rx = JO_FIXED_PI_DIV_2;
	rot.ry = 0;
	rot.rz = JO_FIXED_PI_DIV_2 / 2;

	jo_core_add_callback(my_input);
	jo_core_add_callback(draw_3d_planes);

	jo_core_run();
}

/*
 ** END OF FILE
 */
