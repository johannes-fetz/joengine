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
** HOW TO USE YOUR KEYBOARD WITH MEDNAFEN
**
** 1) Edit \Emulators\mednafen\mednafen.cfg and search "ss.input.port1" and set the value to "keyboard" instead of "gamepad"
** 2) When the emulator is running press [Ctrl]+[Shift]+[Menu] to toggle keyboard emulation in Mednafen.
** 3) When your game runs press [Esc]+[Down] with your keyboard or [Start]+[Down] with your gamepad to display the console.
**
** Full documentation here: https://mednafen.github.io/documentation/ss.html
**
** ▲ Debug console is in work in progress ▲
** Soon you will be able to load / replace sprite at runtime and many other things with native commands.
*/

/*
 * Here you can handle your own commands on the console to help you to develop your game easily.
*/
bool            my_command_handler(int argc, char **argv, int vertical_output, bool *exit)
{
    if (jo_string_equals(argv[0], "test"))
    {
        jo_printf(0, vertical_output, "It works !!!");
        return (true);
    }
    if (jo_string_equals(argv[0], "quit"))
    {
        *exit = true; // You can hide the console at anytime by set this boolean to true;
        return (true);
    }
    return (false);
}

void			my_draw(void)
{
	jo_sprite_draw3D(0, 0, 0, 500);
}

void			jo_main(void)
{
	jo_core_init(JO_COLOR_Black);
	jo_sprite_add_tga("TEX", "SONIC.TGA", JO_COLOR_Transparent);
	jo_core_add_callback(my_draw);
	// Note: JO_COMPILE_WITH_CONSOLE_SUPPORT = 1 must be set on your project Makefile
	jo_console_register_command_handling(my_command_handler);
	jo_core_run();
}

/*
** END OF FILE
*/
