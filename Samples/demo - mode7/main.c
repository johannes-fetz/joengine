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
#include "background_image.h"

#define SPEED_POS   (3)

static jo_img       tile;
static jo_mode7     mode7;
static int          background_pos_x = 0;
static int          floor_pos_y = 0;
static int          need_redraw_background = 0;

void			my_draw(void)
{
    if (need_redraw_background)
    {
        /* We notify that the event is handled */
        need_redraw_background = 0;
        /* We scroll the background (it's very fast) */
        jo_move_background(background_pos_x, 0);
        /* We scroll the mode7 output otherwise the floor will be truncated */
        mode7.output_left = background_pos_x;
        /* This function updates the new output position */
        jo_update_screen_output_coord_for_mode7(&mode7);
        /* We draw the floor */
        jo_do_mode7_floor(&tile, background_pos_x, floor_pos_y, &mode7);

    }
    /* TODO: Add Ken and Ryu :) */
}

void			my_gamepad(void)
{
	if (!jo_is_pad1_available())
		return ;
	if (jo_is_pad1_key_pressed(JO_KEY_LEFT))
	{
        background_pos_x -= SPEED_POS;
        if (background_pos_x < 0)
            background_pos_x = 0;
        else
            need_redraw_background = 1;
	}
	else if (jo_is_pad1_key_pressed(JO_KEY_RIGHT))
    {
  		background_pos_x += SPEED_POS;
  		if (background_pos_x > 192)
            background_pos_x = 192;
  		else
            need_redraw_background = 1;
    }
	/* Uncomment the part bellow to move the floor upward or forward */
	/*if (jo_is_pad1_key_pressed(JO_KEY_UP))
	{
        floor_pos_y -= SPEED_POS;
		need_redraw_background = 1;
	}
	else if (jo_is_pad1_key_pressed(JO_KEY_DOWN))
	{
  		floor_pos_y += SPEED_POS;
		need_redraw_background = 1;
	}*/
}

void            init_mode7_floor()
{
    /* We load an image (aka tile in this case) for the floor.
       We set data to NULL, so the buffer will be created dynamically according to the size */
    tile.data = JO_NULL;
    jo_tga_loader(&tile, JO_ROOT_DIR, "FLR.TGA", JO_COLOR_Transparent);

    /* Now we initialize the pseudo mode7 parameter of the projection */
    mode7.space_z = 50;
    mode7.horizon = 177;
    mode7.scale_x = 100;
    mode7.scale_y = 400;
    mode7.output_left = 0;
    mode7.output_top = 0;

    /* We define that the result will be directly written on the screen */
	jo_init_mode7_for_screen_output(&mode7, tile.width, tile.height);
	/* We set a flag to handle floor drawing in my_draw() */
    need_redraw_background = 1;
}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_Black);

    jo_printf(0, 0, "Use left and right to move");
    /* We draw the fixed part of the background (boat and sky).
       SpriteBG2 is defined in background_image.h */
    jo_set_background_sprite(&SpriteBG2, 0, 0);

    init_mode7_floor();

	jo_core_add_callback(my_draw);
	jo_core_add_callback(my_gamepad);
	jo_core_run();
}

/*
** END OF FILE
*/
