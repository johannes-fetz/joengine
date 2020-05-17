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

static jo_palette      palette1;
static jo_palette      palette2;

void			my_draw(void)
{
    // Here you can specify the palette to use for 8 bits sprites.
    jo_sprite_set_palette(palette1.id);
	jo_sprite_draw3D(0, 0, 0, 500);

    jo_sprite_set_palette(palette2.id); // Here we use our custom palette
	jo_sprite_draw3D(1, 100, 0, 500);

    // Uncomment the code below and see what's happens ^^
	//palette2.data[1]++;
}

// Each time a 8 bits TGA is loaded, this function is called. (jo_set_tga_palette_handling)
jo_palette      *my_tga_palette_handling(void)
{
    // Here you can handle palettes as you want.
    // You can create a new one or pass an existing.
    // jo_create_palette() also give you an ID for your palette (palette1.id)
    jo_create_palette(&palette1);
    // The palette returned will be filled using color map inside TGA file
    // If you return JO_NULL, the palette inside the TGA is ignored
    // ▲ Note : Only 8 palettes are available. So try to mutualize them as much as you can
    return (&palette1);
}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_Black);

	// This method is used to register you palette handler
	jo_set_tga_palette_handling(my_tga_palette_handling);
	// Load a 8 bits ▲ 255 colors (MAX) NOT 256 or it will glitch! ▲ TGA
	// Instead of JO_COLOR_Transparent, you can pass an index on your palette
	jo_sprite_add_tga("TEX", "SON8.TGA", JO_COLOR_Transparent);

	// You can disable TGA palette loading by simply passing JO_NULL to jo_set_tga_palette_handling()
	// Of course you can use an existing palette using jo_sprite_set_palette_id()
	jo_set_tga_palette_handling(JO_NULL);
	// Note: instead of JO_COLOR_Transparent you can pass palette color index like 0x1 or 0x2 in our case.
	jo_sprite_add_tga("TEX", "BRK8.TGA", JO_COLOR_Transparent);

	// Here we create a custom palette for the second sprite
	jo_create_palette(&palette2);
	palette2.data[0] = JO_COLOR_RGB(255, 0, 0); // You can change values at any time (see line 44)
    palette2.data[1] = JO_COLOR_RGB(255, 127, 39);

	jo_core_add_callback(my_draw);
	jo_core_run();
}

/*
** END OF FILE
*/
