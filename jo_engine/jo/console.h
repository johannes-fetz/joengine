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
/** @file console.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Console function prototypes
 *  @bug No known bugs.
 */

#ifndef __JO_CONSOLE_H__
# define __JO_CONSOLE_H__

#ifdef JO_COMPILE_WITH_CONSOLE_SUPPORT

/*
** HOW TO USE YOUR KEYBOARD WITH MEDNAFEN
**
** 1) Edit \Emulators\mednafen\mednafen.cfg and search "ss.input.port1" and set the value to "keyboard" instead of "gamepad"
** 2) When the emulator is running press [Ctrl]+[Shift]+[Menu] to toggle keyboard emulation in Mednafen.
** 3) When your game runs press [Esc]+[Down] with your keyboard or [Start]+[Down] with your gamepad to display the console.
**
** Full documentation here: https://mednafen.github.io/documentation/ss.html
*/

/*
** TYPEDEFS
*/
/** @brief Function prototype for jo_console_register_command_handling()
 *  @return true if the command is known
 *  @param argc Argument count
 *  @param argv Arguments
 *  @param vertical_output Indicate where you can start writing to the console using jo_printf(0, vertical_output, "my output")
 *  @param exit Set the value behind the pointer to true to exit console
*/
typedef bool	(*jo_console_command_callback)(int argc, char **argv, int vertical_output, bool *exit);

void        jo_console_register_command_handling(const jo_console_command_callback callback);
void        jo_console_show_dialog(void);

#endif /* !JO_COMPILE_WITH_CONSOLE_SUPPORT */

#endif /* !__JO_CONSOLE_H__ */

/*
** END OF FILE
*/
