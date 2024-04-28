/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2024, Johannes Fetz (johannesfetz@gmail.com)
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
#include "jo/smpc.h"
#include "jo/core.h"
#include "jo/math.h"
#include "jo/tools.h"
#include "jo/malloc.h"
#include "jo/input.h"

# define JO_INPUT_SPIN_WAIT     (16)

#if !JO_COMPILE_USING_SGL
jo_input                        jo_inputs[JO_INPUT_MAX_DEVICE];

void                            jo_input_init(void)
{
    jo_memset(&jo_inputs, 0, sizeof(jo_inputs));
    jo_smpc_write_byte(DataDirectionRegister1, 0x60);
    jo_smpc_write_byte(DataDirectionRegister2, 0x60);
    jo_smpc_write_byte(InputOutputSelectRegister, 0x03);
    jo_smpc_write_byte(ExternalLatchEnableRegister, 0x00);
}

static unsigned short       jo_read_input(const jo_smpc_memory_address port)
{
    unsigned short          temp;

    jo_smpc_write_byte(port, 0x60);
    jo_spin_wait(JO_INPUT_SPIN_WAIT);
    temp = (jo_smpc_read_byte(port) & 0x08) << 12;

    jo_smpc_write_byte(port, 0x40);
    jo_spin_wait(JO_INPUT_SPIN_WAIT);
    temp |= (jo_smpc_read_byte(port) & 0x0F) << 8;

    jo_smpc_write_byte(port, 0x20);
    jo_spin_wait(JO_INPUT_SPIN_WAIT);
    temp |= (jo_smpc_read_byte(port) & 0x0F) << 4;

    jo_smpc_write_byte(port, 0x00);
    jo_spin_wait(JO_INPUT_SPIN_WAIT);
    temp |= (jo_smpc_read_byte(port) & 0x0F) << 0;

    return (temp ^ 0x8FFF);
}

void                            jo_get_inputs_vblank(void)
{
    unsigned short              previous;

    previous = jo_inputs[0].pressed;
    jo_inputs[0].id = PER_ID_StnPad; /* TODO */
    jo_inputs[0].pressed = jo_read_input(PortDataRegister1);
    jo_inputs[0].on_keydown = (jo_inputs[0].pressed ^ previous) & jo_inputs[0].pressed;
    jo_inputs[0].on_keyup = (previous ^ jo_inputs[0].pressed) & previous;

    previous = jo_inputs[1].pressed;
    jo_inputs[1].id = PER_ID_StnPad; /* TODO */
    jo_inputs[1].pressed = jo_read_input(PortDataRegister2);
    jo_inputs[1].on_keydown = (jo_inputs[1].pressed ^ previous) & jo_inputs[1].pressed;
    jo_inputs[1].on_keyup = (previous ^ jo_inputs[1].pressed) & previous;
}
#else
PerDigital                      jo_inputs[JO_INPUT_MAX_DEVICE];

#define __JO_SIZEOF_6_PerDigital_SIZE_DIV_SIZEOF_INT        (36)
#define __JO_SECOND_MULTITAP_OFFSET                         (54)

void                            jo_get_inputs_vblank(void)
{
    register unsigned int       i;
    unsigned int                *src;
    unsigned int                *dest;

    //We get the first 6 inputs
    src = (unsigned int *)Smpc_Peripheral;
    dest = (unsigned int *)&jo_inputs[0];
    for (JO_ZERO(i); i < __JO_SIZEOF_6_PerDigital_SIZE_DIV_SIZEOF_INT; ++i)
        *dest++ = *src++;
    //Then, we get the last 6 inputs for the second multitap
    //Thanks Slinga: https://github.com/johannes-fetz/joengine/issues/23
    src += __JO_SECOND_MULTITAP_OFFSET;
    for (JO_ZERO(i); i < __JO_SIZEOF_6_PerDigital_SIZE_DIV_SIZEOF_INT; ++i)
        *dest++ = *src++;
}
#endif

jo_gamepad_type                 jo_get_input_type(const int port)
{
    switch (jo_inputs[port].id)
    {
    case PER_ID_NotConnect:
        return (JoNotConnectedGamepad);
    case PER_ID_StnPad:
        return (JoRegularGamepad);
    case PER_ID_StnMouse:
        return (JoRegularMouse);
    case PER_ID_ShuttleMouse:
        return (JoShuttleMouse);
    case PER_ID_StnKeyBoard:
        return (JoRegularKeyboard);
    case PER_ID_NightsPad:
        return (JoNightsPad);
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

unsigned char                   jo_get_input_axis(const int port, const jo_input_axis axis)
{
    // TODO: Implement other analog devices
    switch (jo_get_input_type(port))
    {
    case JoNightsPad:
        switch (axis)
        {
        case JoAxis1:
            return ((PerAnalog *)(&jo_inputs[port]))->x;      // Thumb X axis
        case JoAxis2:
            return ((PerAnalog *)(&jo_inputs[port]))->y;      // Thumb Y axis
        case JoAxis3:
            return ((PerAnalog *)(&jo_inputs[port]))->z;      // Right trigger
        case JoAxis4:
            return ((PerAnalog *)(&jo_inputs[port]))->dummy1; // Left trigger

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
