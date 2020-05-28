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

# define ANGLE_STEP         (JO_FIXED_PI / 128)
# define WAVE_AMPLITUDE     (32)
# define WAVE_SPEED         (4)
# define WAVE_SCROLL_RANGE  (256)

jo_palette                  image_pal;

void			            my_draw(void)
{
    static unsigned short   offset = 0;

    jo_compute_background_horizontal_line_scroll(JO_MOD_POW2(offset, WAVE_SCROLL_RANGE));
    offset += WAVE_SPEED;
}

void                    load_background_image()
{
    jo_img_8bits        img;

    img.data = NULL;
    jo_tga_8bits_loader(&img, "BG", "BG.TGA", 0);
    jo_set_background_8bits_sprite(&img, image_pal.id, false);
    jo_free_img(&img);
}

jo_palette          *my_tga_palette_handling(void)
{
    jo_create_palette(&image_pal);
    return (&image_pal);
}

void                init_horizontal_line_scroll(void)
{
    int             *scroll_table;
    int             i;
    jo_fixed        angle;

    scroll_table = jo_enable_background_horizontal_line_scroll();
    angle = 0;
	for(i = 0 ; i < JO_SCROLL_TABLE_SIZE ; ++i)
    {
		scroll_table[i] = jo_fixed_sin(angle) * WAVE_AMPLITUDE;
		angle += ANGLE_STEP;
    }
}

void			    jo_main(void)
{
	jo_core_init(JO_COLOR_Black);
    jo_core_tv_off();
    jo_set_tga_palette_handling(my_tga_palette_handling);
    load_background_image();
    init_horizontal_line_scroll();
    jo_core_tv_on();
	jo_core_add_callback(my_draw);
	jo_core_run();
}

/*
** END OF FILE
*/
