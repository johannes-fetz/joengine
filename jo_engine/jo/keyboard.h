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
/** @file keyboard.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Keyboard function prototypes
 *  @bug No known bugs.
 */

#ifndef __JO_KEYBOARD_H__
# define __JO_KEYBOARD_H__

#ifdef JO_COMPILE_WITH_KEYBOARD_SUPPORT

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
** Internals
*/

/** @brief Keyboard mapping (internal engine usage)
 *  @warning MC Hammer: don't touch this
 */
extern char __internal_keyboard_normal_mapping[];

/** @brief Keyboard mapping (internal engine usage)
 *  @warning MC Hammer: don't touch this
 */
extern char __internal_keyboard_caps_lock_mapping[];

/** @brief Keyboard mapping size (internal engine usage)
 *  @warning MC Hammer: don't touch this
 */
# define __JO_KEYBOARD_MAPPING_SIZE     (128)

/*
** Special Keys
*/
/** @brief Keyboard special keys
 */
typedef enum
{
    JO_KEYBOARD_NO_SPECIAL_KEY = 0,
    JO_KEYBOARD_RIGHT,
    JO_KEYBOARD_LEFT,
    JO_KEYBOARD_DOWN,
    JO_KEYBOARD_UP,
    JO_KEYBOARD_ESCAPE,
    JO_KEYBOARD_ENTER,
    JO_KEYBOARD_BACKSPACE,
    JO_KEYBOARD_TAB,
    JO_KEYBOARD_DELETE,
    JO_KEYBOARD_HOME,
    JO_KEYBOARD_END,
    JO_KEYBOARD_F1,
    JO_KEYBOARD_F2,
    JO_KEYBOARD_F3,
    JO_KEYBOARD_F4,
    JO_KEYBOARD_F5,
    JO_KEYBOARD_F6,
    JO_KEYBOARD_F7,
    JO_KEYBOARD_F8,
    JO_KEYBOARD_F9,
    JO_KEYBOARD_F10,
    JO_KEYBOARD_F11,
    JO_KEYBOARD_F12
}           jo_keyboard_special_key;

jo_keyboard_special_key   jo_keyboard_get_special_key(void);

static  __jo_force_inline unsigned char             jo_keyboard_get_char(void)
{
    PerKeyBoard                                     *keyboard;

	keyboard = (PerKeyBoard *)&Smpc_Peripheral[0];
	if ((keyboard->cond & PER_KBD_MK) == 0 || keyboard->code >= __JO_KEYBOARD_MAPPING_SIZE)
        return (0);
	if (keyboard->cond & PER_KBD_CL)
        return (__internal_keyboard_caps_lock_mapping[keyboard->code]);
    return (__internal_keyboard_normal_mapping[keyboard->code]);
}

#endif /* !JO_COMPILE_WITH_KEYBOARD_SUPPORT */

#endif /* !__JO_KEYBOARD_H__ */

/*
** END OF FILE
*/
