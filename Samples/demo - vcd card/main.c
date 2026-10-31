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
#include <jo/vcd_card.h>

int g_sprite_vcd_card = 0;
int g_sprite_x = 0;
int g_vcd_present = 0;

void my_draw(void);

void			jo_main(void)
{
	jo_core_init(JO_COLOR_Black);

	//
	// So far demo only detects the presence of the VCD Card
	// Additional features planned!
	//

	// returns 0 if VCD Card is present
	g_vcd_present = jo_vcd_card_is_present();

	jo_core_add_callback(my_draw);

	g_sprite_vcd_card = jo_sprite_add_tga(JO_ROOT_DIR, "VCD_CARD.TGA", JO_COLOR_Green);
	g_sprite_x = jo_sprite_add_tga(JO_ROOT_DIR, "X.TGA", JO_COLOR_Black);

	jo_core_run();
}

// drawing callback
void			my_draw(void)
{
	jo_printf(7, 2, "VCD Card (Movie Card) Demo");
	jo_sprite_draw3D(g_sprite_vcd_card, 0,-35, 500);

	jo_printf(4, 19, "Status: ");

	if(g_vcd_present == 0)
	{
		jo_printf(12, 19, "Detected");
	}
	else
	{
		jo_sprite_draw3D(g_sprite_x, 0,-35, 500);
		jo_printf(12, 19, "Not Detected");
	}
}

/*
** END OF FILE
*/
