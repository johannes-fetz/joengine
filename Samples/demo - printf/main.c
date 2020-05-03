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

static jo_font      *my_font;

void			    my_draw(void)
{
    jo_datetime     now;

    /* See jo/colors.h to know palette color indexes */
    jo_set_printf_color_index(JO_COLOR_INDEX_Red);
    jo_getdate(&now);
    jo_printf(0, 0, "Date: %d/%d/%d    ", now.day, now.month, now.year);
    jo_printf(0, 1, "Time: %d:%d:%d    ", now.hour, now.minute, now.second);
    jo_set_printf_color_index(JO_COLOR_INDEX_Green);
    jo_printf(0, 3, "It works !!!");
    jo_set_printf_color_index(JO_COLOR_INDEX_Blue);
    jo_printf(0, 4, "Hex value of 42: %x", 42);
    // Or you can use jo_printf_with_color()
    jo_printf_with_color(0, 5, JO_COLOR_INDEX_Purple, "Dynamic memory usage: %d%%  ", jo_memory_usage_percent());
    // Use jo_font_printf if you want to use top-left screen coordinates
    jo_font_printf_centered(my_font, 0, 0, 1.0f, "SCORE = %d",  42);



}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_White);
    my_font = jo_font_load(JO_ROOT_DIR, "FONT.TGA", JO_COLOR_Green, 8, 8, 2, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ!\"?=%&',.()*+-/");
	jo_core_add_callback(my_draw);
	jo_core_run();
}

/*
** END OF FILE
*/
