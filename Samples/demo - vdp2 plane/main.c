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

/*
** ▲ This demo shows how to use VDP2 planes without the traditional PAL/CEL/MAP files!
** You just need an 8 bits TGA image (255 colors maximum) with a 90° clockwise rotation (open SKY.TGA and you will understand)
*/

jo_pos3Df                   pos;
jo_rot3Df                   rot;
jo_palette                  image_pal;

void                        my_input(void)
{
    static int              movement_speed = 0;
    static float            angle_increment = 0.0f;

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
		angle_increment += 1.5;
	else if (jo_is_pad1_key_pressed(JO_KEY_LEFT))
        angle_increment -= 1.5;

    if (jo_is_pad1_key_pressed(JO_KEY_L))
        pos.z += 1.0;
	else if (jo_is_pad1_key_pressed(JO_KEY_R))
        pos.z -= 1.0;

    rot.ry = -JO_DEG_TO_RAD(angle_increment);
    rot.rz += JO_DEG_TO_RAD(angle_increment) / 2.0;
    angle_increment = angle_increment * 4.0 / 5.0;

    pos.x -= movement_speed * jo_sin_radf(rot.rz) / 10.0;
	pos.y -= movement_speed * jo_cos_radf(rot.rz) / 10.0;
}

void                            draw_3d_planes(void)
{
    // FLOOR
    jo_3d_push_matrix();
	{
		jo_3d_rotate_matrix_rad(rot.rx, rot.ry, rot.rz);
		jo_3d_translate_matrixf(pos.x, pos.y, pos.z);
        jo_background_3d_plane_a_draw(true);
	}
	jo_3d_pop_matrix();
    //SKY
	jo_3d_push_matrix();
	{
	    jo_3d_translate_matrix(rot.rz * 100, 110 - 112 * jo_cos_radf(rot.rx), 100);
        jo_3d_rotate_matrix_rad_z(-rot.ry);
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

    pos.x = 800.0;
	pos.y = 800.0;
	pos.z = -35.0;

	rot.rx = JO_DEG_TO_RAD(90.0);
	rot.ry = JO_DEG_TO_RAD(0.0);
	rot.rz = JO_DEG_TO_RAD(45.0);

	jo_core_add_callback(my_input);
	jo_core_add_callback(draw_3d_planes);

	jo_core_run();
}

/*
 ** END OF FILE
 */
