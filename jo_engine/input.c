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

/*
** INCLUDES
*/
#include <stdbool.h>
#include "jo/conf.h"
#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/core.h"
#include "jo/math.h"
#include "jo/tools.h"
#include "jo/malloc.h"
#include "jo/input.h"

# define JO_INPUT_SPIN_WAIT     (16)

jo_peripheral                      jo_inputs[JO_INPUT_MAX_DEVICE];

#define __JO_SIZEOF_6_PerDigital_SIZE_DIV_SIZEOF_INT        (36)
#define __JO_SECOND_MULTITAP_OFFSET                         (54)

/*
** The peripherals are read at each VBLANK, the game reads jo_inputs once per frame (JO_FRAMERATE VBLANK): the keys pushed
** or released since the start of the frame are accumulated, else jo_is_pad1_key_down() would miss the keys pushed during
** the other VBLANK. Each push or pull is reported to the first frame that starts after it (and to the current frame).
*/
static unsigned short           jo_input_push[JO_INPUT_MAX_DEVICE];
static unsigned short           jo_input_pull[JO_INPUT_MAX_DEVICE];
static unsigned short           jo_input_reported_push[JO_INPUT_MAX_DEVICE];
static unsigned short           jo_input_reported_pull[JO_INPUT_MAX_DEVICE];

/** @brief Start of a frame (Internal usage, called by __jo_core_synch()): forget the keys already reported to the previous frame */
void                            __jo_input_frame_start(void)
{
    register unsigned int       i;
    unsigned int                sr;

    sr = __jo_core_disable_interrupts();
    for (JO_ZERO(i); i < JO_INPUT_MAX_DEVICE; ++i)
    {
        jo_input_push[i] &= ~jo_input_reported_push[i];
        jo_input_pull[i] &= ~jo_input_reported_pull[i];
        jo_input_reported_push[i] = jo_input_push[i];
        jo_input_reported_pull[i] = jo_input_pull[i];
        jo_inputs[i].push = (unsigned short)~jo_input_push[i];
        jo_inputs[i].pull = (unsigned short)~jo_input_pull[i];
    }
    __jo_core_restore_interrupts(sr);
}

void                            jo_get_inputs_vblank(void)
{
    register unsigned int       i;
    unsigned int                *src;
    unsigned int                *dest;

    //We get the first 6 inputs
    src = (unsigned int *)__jo_smpc_peripherals;
    dest = (unsigned int *)&jo_inputs[0];
    for (JO_ZERO(i); i < __JO_SIZEOF_6_PerDigital_SIZE_DIV_SIZEOF_INT; ++i)
        *dest++ = *src++;
    //Then, we get the last 6 inputs for the second multitap
    //Thanks Slinga: https://github.com/johannes-fetz/joengine/issues/23
    src += __JO_SECOND_MULTITAP_OFFSET;
    for (JO_ZERO(i); i < __JO_SIZEOF_6_PerDigital_SIZE_DIV_SIZEOF_INT; ++i)
        *dest++ = *src++;
    // Keys pushed or released since the start of the frame (active low in jo_inputs): kept for the next frame. The frame
    // sees the keys reported at its start only (a key seen after a VBLANK in the middle of a frame was seen again by the
    // next frame: a pause toggled twice)
    for (JO_ZERO(i); i < JO_INPUT_MAX_DEVICE; ++i)
    {
        jo_input_push[i] |= (unsigned short)~jo_inputs[i].push;
        jo_input_pull[i] |= (unsigned short)~jo_inputs[i].pull;
        jo_inputs[i].push = (unsigned short)~jo_input_reported_push[i];
        jo_inputs[i].pull = (unsigned short)~jo_input_reported_pull[i];
    }
}

jo_gamepad_type                 jo_get_input_type(const int port)
{
    switch (jo_inputs[port].id)
    {
    case JO_PERIPHERAL_ID_NOT_CONNECTED:
        return (JoNotConnectedGamepad);
    case JO_PERIPHERAL_ID_STANDARD_PAD:
        return (JoRegularGamepad);
    case JO_PERIPHERAL_ID_MOUSE:
        return (JoRegularMouse);
    case JO_PERIPHERAL_ID_SHUTTLE_MOUSE:
        return (JoShuttleMouse);
    case JO_PERIPHERAL_ID_KEYBOARD:
        return (JoRegularKeyboard);
    case JO_PERIPHERAL_ID_NIGHTS_PAD:
        return (JoNightsPad);
    case JO_PERIPHERAL_ID_MISSION_STICK:
        return (JoMissionStick);
    case JO_PERIPHERAL_ID_RACING_WHEEL:
        return (JoRacingWheel);
    default:
        return (JoUnsupportedGamepad);
    }
}

int                             jo_get_input_count(void)
{
    register unsigned int       i;
    register unsigned int       count;

    for (JO_ZERO(i), JO_ZERO(count); i < JO_INPUT_MAX_DEVICE; ++i)
    {
        if (jo_get_input_type(i) != JoNotConnectedGamepad)
            ++count;
    }
    return (count);
}

int                             jo_get_player_input_port(const int player)
{
    register int                i;
    int                         n;

    for (JO_ZERO(i), JO_ZERO(n); i < JO_INPUT_MAX_DEVICE; ++i)
        if (jo_inputs[i].id != JO_PERIPHERAL_ID_NOT_CONNECTED && n++ == player)
            return (i);
    return (-1);
}

unsigned char                   jo_get_input_axis(const int port, const jo_input_axis axis)
{
    // TODO: Implement other analog devices
    switch (jo_get_input_type(port))
    {
    case JoRacingWheel:
        switch (axis)
        {
        case JoAxis1:
            return ((jo_peripheral_analog *)(&jo_inputs[port]))->x;      // Wheel turn axis
        default:
            break;
        }
        break;

    case JoMissionStick:
        switch (axis)
        {
        case JoAxis1:
            return ((jo_peripheral_analog *)(&jo_inputs[port]))->x;      // Stick X axis
        case JoAxis2:
            return ((jo_peripheral_analog *)(&jo_inputs[port]))->y;      // Stick Y axis
        case JoAxis3:
            return ((jo_peripheral_analog *)(&jo_inputs[port]))->z;      // Throttle
        default:
            break;
        }
        break;

    case JoNightsPad:
        switch (axis)
        {
        case JoAxis1:
            return ((jo_peripheral_analog *)(&jo_inputs[port]))->x;      // Thumb X axis
        case JoAxis2:
            return ((jo_peripheral_analog *)(&jo_inputs[port]))->y;      // Thumb Y axis
        case JoAxis3:
            return ((jo_peripheral_analog *)(&jo_inputs[port]))->z;      // Right trigger
        case JoAxis4:
            return ((jo_peripheral_analog *)(&jo_inputs[port]))->w; // Left trigger

        default:
            break;
        }
        break;

    default:
        break;
    }

    return 0;
}

/*
** END OF FILE
*/
