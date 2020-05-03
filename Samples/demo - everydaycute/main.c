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

static int cats_anim_id = 0;
static int star_sprite_id = 0;
static int star_rotation = 0;

void			    my_draw(void)
{
	jo_sprite_draw3D_and_rotate(star_sprite_id, -80, -60, 400, star_rotation);
	jo_sprite_draw3D_and_rotate(star_sprite_id, -80, 80, 400, star_rotation);
	jo_sprite_draw3D_and_rotate(star_sprite_id, 80, -60, 400, star_rotation);
	jo_sprite_draw3D_and_rotate(star_sprite_id, 80, 80, 400, star_rotation);
	star_rotation += 3;

	jo_sprite_draw3D(jo_get_anim_sprite(cats_anim_id), 0, 0, 490);
    jo_printf(12, 21, "EverydayCute.com");
}

void			create_cats_animation()
{
	int first_sprite_id = jo_sprite_add_image_pack("EDC", "EDC.TEX", JO_COLOR_Transparent);
	cats_anim_id = jo_create_sprite_anim(first_sprite_id, 4, 10);
	jo_start_sprite_anim_loop(cats_anim_id);
}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_White);
	jo_set_printf_color_index(JO_COLOR_INDEX_Black);
	create_cats_animation();
	star_sprite_id = jo_sprite_add_tga(JO_ROOT_DIR, "STAR.TGA", JO_COLOR_Green);
	jo_core_add_callback(my_draw);
	jo_core_run();
}

/*
** END OF FILE
*/
