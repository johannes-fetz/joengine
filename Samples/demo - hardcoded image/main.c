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
#include "hardcoded_image.h"
#include "hardcoded_8bits_image.h"

void			    my_draw(void)
{
	jo_printf(0, 0, jo_get_last_error());

	jo_sprite_draw3D(0, 0, 0, 500);
	jo_sprite_draw3D(1, 100, 0, 500);
}

void			    load_15bits_image(void)
{
	jo_sprite_add(&SpriteB1);
}

void			    load_8bits_image(void)
{
    jo_img_8bits    img;

    img.data = spaceship_image;
    img.width = 16;
    img.height = 16;
    jo_sprite_add_8bits_image(&img);
}

void			    set_8bits_image_palette(void)
{
    // Each component value (R, G and B) goes from 0 to 255
    jo_set_palette_register(0xBB, JO_COLOR_RGB(0, 255, 0));
    jo_set_palette_register(0xCC, JO_COLOR_RGB(255, 0, 0));
}

void			    jo_main(void)
{
	jo_core_init(JO_COLOR_White);
	load_15bits_image();
	load_8bits_image();
	set_8bits_image_palette();
	jo_core_add_callback(my_draw);
	jo_core_run();
}

/*
** END OF FILE
*/
