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
** ▲ NOTE: JO_COMPILE_WITH_PRINTF_MODULE must be set to 0 in your makefile
** When the default jo_printf in NBG0 is disabled you have full access to NBG0/2/3
**
** This demo also works with NBG3.
*/

#define TRANSPARENT_COLOR_INDEX_IN_FONT_PALETTE     (2)

jo_palette              font_palette;

void                    load_nbg2_font(void)
{
    jo_img_8bits        img;

    img.data = NULL;
    jo_tga_8bits_loader(&img, JO_ROOT_DIR, "FONT.TGA", TRANSPARENT_COLOR_INDEX_IN_FONT_PALETTE);
    jo_vdp2_set_nbg2_8bits_font(&img, " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ!\"?=%&',.()*+-/", font_palette.id, false, true);
    jo_free_img(&img);
}

jo_palette              *my_tga_palette_handling(void)
{
    jo_create_palette(&font_palette);
    return (&font_palette);
}

void			jo_main(void)
{
    jo_core_init(JO_COLOR_White);
    jo_set_tga_palette_handling(my_tga_palette_handling);
    // We put NBG2 in top of all scroll screen
    jo_core_set_screens_order(JO_NBG2_SCREEN, JO_SPRITE_SCREEN, JO_NBG0_SCREEN, JO_RBG0_SCREEN, JO_NBG1_SCREEN);
    load_nbg2_font();
    jo_nbg2_printf(5, 5, "IT WORKS !!!");
	jo_core_run();
}

/*
** END OF FILE
*/
