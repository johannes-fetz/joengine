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

static int mouse_sprite_id = 0;
static int cursor_sprite_id = 0;
static int arrow_sprite_id = 0;
static jo_gamepad_type gamepad_type = JoNotConnectedGamepad;

static int mouse_x = 0;
static int mouse_y = 0;
static int mouse_pos_x = 0;
static int mouse_pos_y = 0;
static int prev_pos_x = 0;
static int prev_pos_y = 0;
static int mouse_delta_x = 0;
static int mouse_delta_y = 0;

void			my_draw(void)
{
    jo_printf(15, 1, "*Mouse demo*");
    jo_sprite_draw3D(mouse_sprite_id, 0, -30, 500);

    if(gamepad_type == JoRegularMouse)
    {
        jo_printf(3, 22, "Mouse: JoRegularMouse");
    }
    else if(gamepad_type == JoShuttleMouse)
    {
        jo_printf(3, 22, "Mouse: JoShuttleMouse");
    }
    else
    {
        jo_printf(3, 22, "Mouse: Not Connected ");
        return;
    }

    jo_printf(3, 23, "Mouse X: %d Delta: %d Pos: %d            ", mouse_pos_x, mouse_delta_x, mouse_x);
    jo_printf(3, 24, "Mouse Y: %d Delta: %d Pos: %d            ", mouse_pos_y, mouse_delta_y, mouse_y);

    jo_sprite_draw3D(cursor_sprite_id, mouse_x, mouse_y, 500);
}

void			my_gamepad(void)
{
    gamepad_type = jo_get_input_type(0);

    // Saturn supports two mouse types
    if(gamepad_type != JoRegularMouse && gamepad_type != JoShuttleMouse)
    {
        // no mouse in controller port
        return;
    }

    //
    // read mouse X, Y position
    //
    mouse_pos_x = jo_get_mouse_pos_x(0);
    mouse_pos_y = jo_get_mouse_pos_y(0);

    // compare them to last frame to calculate the change
    mouse_delta_x = mouse_pos_x - prev_pos_x;
    mouse_delta_y = mouse_pos_y - prev_pos_y;

    // move mouse sprite
    mouse_x += mouse_delta_x;
    mouse_y += mouse_delta_y;

    prev_pos_x = mouse_pos_x;
    prev_pos_y = mouse_pos_y;

    // bound/wrap the mouse sprite
    if(mouse_x < -160)
        mouse_x = 160;
    if(mouse_x > 160)
        mouse_x = -160;
    if(mouse_y < -120)
        mouse_y = 120;
    if(mouse_y > 120)
        mouse_y = -120;

    //
    // the mouse only has 4 buttons (start, a, b, c)
    //

    if (jo_is_input_key_pressed(0, JO_KEY_START))
        jo_sprite_draw3D(arrow_sprite_id, 0, -40, 450);
    if (jo_is_input_key_pressed(0, JO_KEY_A))
        jo_sprite_draw3D(arrow_sprite_id, -20, -64, 450);
    if (jo_is_input_key_pressed(0, JO_KEY_B))
        jo_sprite_draw3D(arrow_sprite_id, 0, -62, 450);
    if (jo_is_input_key_pressed(0, JO_KEY_C))
        jo_sprite_draw3D(arrow_sprite_id, 20, -60, 450);
}

void			jo_main(void)
{
    jo_core_init(JO_COLOR_Black);
    mouse_sprite_id = jo_sprite_add_tga(JO_ROOT_DIR, "MOUSE.TGA", JO_COLOR_Transparent);
    cursor_sprite_id = jo_sprite_add_tga(JO_ROOT_DIR, "CURSOR.TGA", JO_COLOR_Transparent);
    arrow_sprite_id = jo_sprite_add_tga(JO_ROOT_DIR, "ARW.TGA", JO_COLOR_Green);
    jo_core_add_callback(my_gamepad);
    jo_core_add_callback(my_draw);
    jo_core_run();
}

/*
** END OF FILE
*/
