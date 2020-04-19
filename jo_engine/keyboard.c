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

/*
** INCLUDES
*/
#include <stdbool.h>
#include "jo/sgl_prototypes.h"
#include "jo/conf.h"
#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/keyboard.h"

#ifdef JO_COMPILE_WITH_KEYBOARD_SUPPORT

/* REGULAR QWERTY KEYBOARD*/

jo_keyboard_special_key   jo_keyboard_get_special_key(void)
{
    PerKeyBoard                                     *keyboard;

	keyboard = (PerKeyBoard *)&Smpc_Peripheral[0];
	if ((keyboard->cond & PER_KBD_MK) == 0)
        return JO_KEYBOARD_NO_SPECIAL_KEY;
    switch (keyboard->code)
    {
        case 118:
            return (JO_KEYBOARD_ESCAPE);
        case 134:
            return (JO_KEYBOARD_LEFT);
        case 141:
            return (JO_KEYBOARD_RIGHT);
        case 137:
            return (JO_KEYBOARD_UP);
        case 138:
            return (JO_KEYBOARD_DOWN);
        case 90:
        case 25:
            return (JO_KEYBOARD_ENTER);
        case 102:
            return (JO_KEYBOARD_BACKSPACE);
        case 133:
            return (JO_KEYBOARD_DELETE);
        case 135:
            return (JO_KEYBOARD_HOME);
        case 136:
            return (JO_KEYBOARD_END);
        case 5:
            return (JO_KEYBOARD_F1);
        case 6:
            return (JO_KEYBOARD_F2);
        case 4:
            return (JO_KEYBOARD_F3);
        case 12:
            return (JO_KEYBOARD_F4);
        case 3:
            return (JO_KEYBOARD_F5);
        case 11:
            return (JO_KEYBOARD_F6);
        case 131:
            return (JO_KEYBOARD_F7);
        case 10:
            return (JO_KEYBOARD_F8);
        case 1:
            return (JO_KEYBOARD_F9);
        case 9:
            return (JO_KEYBOARD_F10);
        case 120:
            return (JO_KEYBOARD_F11);
        case 7:
            return (JO_KEYBOARD_F12);
        default:
            return (JO_KEYBOARD_NO_SPECIAL_KEY);
    }
}

char __internal_keyboard_normal_mapping[__JO_KEYBOARD_MAPPING_SIZE] =
{
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,/*10*/
    0,
    0,
    0,
    '`',
    0,
    0,
    0,
    0,
    0,
    0,/*20*/
    'q',
    '1',
    0,
    0,
    0,
    'z',
    's',
    'a',
    'w',
    '2',/*30*/
    0,
    0,
    'c',
    'x',
    'd',
    'e',
    '4',
    '3',
    0,
    0,/*40*/
    ' ',
    'v',
    'f',
    't',
    'r',
    '5',
    0,
    0,
    'n',
    'b',/*50*/
    'h',
    'g',
    'y',
    '6',
    0,
    0,
    0,
    'm',
    'j',
    'u',/*60*/
    '7',
    '8',
    0,
    0,
    ',',
    'k',
    'i',
    'o',
    '0',
    '9',/*70*/
    0,
    0,
    '.',
    '/',
    'l',
    ';',
    'p',
    '-',
    0,
    0,/*80*/
    0,
    '\'',
    0,
    '[',
    '=',
    0,
    0,
    0,
    0,
    0,/*90*/
    ']',
    0,
    '\\',
    0,
    0,
    0,
    0,
    0,
    0,
    0,/*100*/
    0,
    0,
    0,
    0,
    '1',
    0,
    '4',
    '7',
    0,
    0,/*110*/
    0,
    '0',
    '.',
    '2',
    '5',
    '6',
    '8',
    0,
    0,
    0,/*120*/
    0,
    '3',
    0,
    0,
    '9',
    0,
    0
};

char __internal_keyboard_caps_lock_mapping[__JO_KEYBOARD_MAPPING_SIZE] =
{
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,/*10*/
    0,
    0,
    0,
    '~',
    0,
    0,
    0,
    0,
    0,
    0,/*20*/
    'Q',
    '!',
    0,
    0,
    0,
    'Z',
    'S',
    'A',
    'W',
    '@',/*30*/
    0,
    0,
    'C',
    'X',
    'D',
    'E',
    '$',
    '#',
    0,
    0,/*40*/
    ' ',
    'V',
    'F',
    'T',
    'R',
    '%',
    0,
    0,
    'N',
    'B',/*50*/
    'H',
    'G',
    'Y',
    '^',
    0,
    0,
    0,
    'M',
    'J',
    'U',/*60*/
    '&',
    '*',
    0,
    0,
    '<',
    'K',
    'I',
    'O',
    ')',
    '(',/*70*/
    0,
    0,
    '>',
    '?',
    'L',
    ':',
    'P',
    '_',
    0,
    0,/*80*/
    0,
    '"',
    0,
    '{',
    '+',
    0,
    0,
    0,
    0,
    0,/*90*/
    '}',
    0,
    '|',
    0,
    0,
    0,
    0,
    0,
    0,
    0,/*100*/
    0,
    0,
    0,
    0,
    '1',
    0,
    '4',
    '7',
    0,
    0,/*110*/
    0,
    '0',
    '.',
    '2',
    '5',
    '6',
    '8',
    0,
    0,
    0,/*120*/
    0,
    '3',
    0,
    0,
    '9',
    0,
    0
};

#endif /* !JO_COMPILE_WITH_KEYBOARD_SUPPORT */

/*
** END OF FILE
*/
