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
#include "hardcoded_image.h"
#include "hardcoded_8bits_image.h"
#include "hardcoded_4bits_image.h"

/* The 4 bits spaceship drawn in SHIP_VARIANTS colors: one sub-palette of 16 colors each (the sub-palette 0 is used by
   the 8 bits spaceship: the colors 1 and 2 of my_palette) */
#define SHIP_VARIANTS       (4)
#define SHIP_FIRST_SUBPALETTE (1)
#define SHIP_SPACING        (40)
#define SHIP_ROW_Y          (60)
#define SHIP_BLINK_FRAMES   (32)

jo_palette          my_palette;
int                 spaceship_4bits;
int                 frame;

static const jo_color ship_colors[SHIP_VARIANTS][JO_PALETTE_SUBPALETTE_COLORS] =
{
    {JO_COLOR_Transparent, JO_COLOR_Blue, JO_COLOR_Yellow},
    {JO_COLOR_Transparent, JO_COLOR_Purple, JO_COLOR_White},
    {JO_COLOR_Transparent, JO_COLOR_DarkGreen, JO_COLOR_Red},
    {JO_COLOR_Transparent, JO_COLOR_Black, JO_COLOR_Yellow},
};

static const jo_color ship_blink_colors[JO_PALETTE_SUBPALETTE_COLORS] =
{
    JO_COLOR_Transparent, JO_COLOR_Black, JO_COLOR_Black
};

void			    my_draw(void)
{
	jo_printf(0, 0, jo_get_last_error());

	jo_sprite_draw3D(0, 0, 0, 500);

	jo_sprite_set_palette(my_palette.id); // set the current palette
	jo_sprite_draw3D(1, 100, 0, 500);

    // The same 4 bits sprite in other colors: its sub-palette chosen before each draw (no copy of its pixels)
    for (int k = 0; k < SHIP_VARIANTS; ++k)
    {
        jo_sprite_set_palette_4bits(my_palette.id, SHIP_FIRST_SUBPALETTE + k);
        jo_sprite_draw3D(spaceship_4bits, (k - SHIP_VARIANTS / 2) * SHIP_SPACING + SHIP_SPACING / 2, SHIP_ROW_Y, 500);
    }
    // A palette animation: the eyes of the last one blink, only its colors in the color RAM are written
    ++frame;
    jo_palette_set_subpalette(&my_palette, SHIP_FIRST_SUBPALETTE + SHIP_VARIANTS - 1,
                              (frame / SHIP_BLINK_FRAMES) % 2 ? ship_blink_colors : ship_colors[SHIP_VARIANTS - 1]);
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

void			    load_4bits_image(void)
{
    jo_img_4bits    img;

    img.data = spaceship_4bits_image;
    img.width = 16;
    img.height = 16;
    spaceship_4bits = jo_sprite_add_4bits_image(&img);
    for (int k = 0; k < SHIP_VARIANTS; ++k)
        jo_palette_set_subpalette(&my_palette, SHIP_FIRST_SUBPALETTE + k, ship_colors[k]);
}

void			    jo_main(void)
{
	jo_core_init(JO_COLOR_White);

    jo_create_palette(&my_palette);
    my_palette.data[0] = JO_COLOR_RGB(0, 255, 0); // 0x01 in spaceship_image
    my_palette.data[1] = JO_COLOR_RGB(255, 0, 0); // 0x02 in spaceship_image

	load_15bits_image();
	load_8bits_image();
	load_4bits_image();
	jo_core_add_callback(my_draw);
	jo_core_run();
}

/*
** END OF FILE
*/
