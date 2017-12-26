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

static int brightness = 0;

void            compute_brightness(void)
{
    static int  brightness_direction = 1;

    if (brightness_direction)
    {
        if (brightness >= (JO_MAX_BRIGHTNESS - 1))
            brightness_direction = 0;
        else
            ++brightness;
    }
    else
    {
        if (brightness <= 0)
            brightness_direction = 1;
        else
            --brightness;
    }
}

void			my_draw(void)
{
    jo_printf(4, 6, "Normal");
	jo_sprite_draw3D2(0, 5, 5, 500);

    jo_sprite_enable_gouraud_shading();

    jo_printf(14, 6, "White/black");
    jo_printf(16, 7, "Gouraud");
    jo_set_gouraud_shading_colors(JO_COLOR_White, JO_COLOR_White, JO_COLOR_Black, JO_COLOR_Black);
	jo_sprite_draw3D2(0, 106, 5, 500);

    jo_printf(27, 6, "Multicolor");
    jo_printf(29, 7, "Gouraud");
    jo_set_gouraud_shading_colors(JO_COLOR_Red, JO_COLOR_Green, JO_COLOR_Blue, JO_COLOR_White);
	jo_sprite_draw3D2(0, 207, 5, 500);

    jo_printf(3, 18, "Top left");
    jo_printf(3, 19, "Gouraud");
    jo_set_gouraud_shading_colors(JO_COLOR_Red, JO_NO_GOURAUD_COLOR, JO_NO_GOURAUD_COLOR, JO_NO_GOURAUD_COLOR);
	jo_sprite_draw3D2(0, 5, 106, 500);

    jo_printf(18, 18, "Full");
    jo_printf(16, 19, "Gouraud");
    jo_set_gouraud_shading_colors(JO_COLOR_Yellow, JO_COLOR_Yellow, JO_COLOR_Yellow, JO_COLOR_Yellow);
	jo_sprite_draw3D2(0, 106, 106, 500);

    jo_printf(27, 18, "Brightness");
    jo_printf(29, 19, "Gouraud");
    jo_set_gouraud_shading_brightness(brightness);
	jo_sprite_draw3D2(0, 207, 106, 500);

	jo_sprite_disable_gouraud_shading();
	compute_brightness();
}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_Black);
	jo_sprite_add_tga("TEX", "WALL.TGA", JO_COLOR_Transparent);
	jo_core_add_callback(my_draw);
	jo_core_run();
}

/*
** END OF FILE
*/
