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
/** @file input.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Gamepad functions
 *  @bug No known bugs.
 *  @todo analog gamepad, wheel, gun
 */

#ifndef __JO_INPUT_H__
# define __JO_INPUT_H__

# define JO_INPUT_MAX_DEVICE            (12)

// This device ID is not present in SGL header files
#define	PER_ID_NightsPad	0x16	/* Nights 3D pad */

#if JO_COMPILE_USING_SGL
extern PerDigital                       jo_inputs[JO_INPUT_MAX_DEVICE];
# define __JO_KEY_PRESSED(PORT, KEY)    ((jo_inputs[PORT].data & KEY) == 0)
#else
# define __JO_KEY_PRESSED(PORT, KEY)    ((jo_inputs[PORT].pressed & KEY) == KEY)
#endif

#if !JO_COMPILE_USING_SGL

/** @brief Input structure (internal engine usage)
 *  @warning MC Hammer: don't touch this
 */
typedef struct
{
    unsigned char               id;
    unsigned short              pressed;
    unsigned short              on_keydown;
    unsigned short              on_keyup;
}                               jo_input;

/** @brief Input status (internal engine usage)
 *  @warning MC Hammer: don't touch this
 */
extern jo_input                        jo_inputs[JO_INPUT_MAX_DEVICE];

#endif

/** @brief Input keys
 */
#if JO_COMPILE_USING_SGL
typedef enum
{
    JO_KEY_RIGHT = 1 << 15,
    JO_KEY_LEFT	= 1 << 14,
    JO_KEY_DOWN = 1 << 13,
    JO_KEY_UP = 1 << 12,
    JO_KEY_START = 1 << 11,
    JO_KEY_A = 1 << 10,
    JO_KEY_B = 1 << 8,
    JO_KEY_C = 1 << 9,
    JO_KEY_X = 1 << 6,
    JO_KEY_Y = 1 << 5,
    JO_KEY_Z = 1 << 4,
    JO_KEY_L = 1 << 3,
    JO_KEY_R = 1 << 7,

    JO_MOUSE_START_BUTTON = 1 << 11,
    JO_MOUSE_LEFT_BUTTON = 1 << 10,
    JO_MOUSE_MIDDLE_BUTTON = 1 << 8,
    JO_MOUSE_RIGHT_BUTTON = 1 << 9
}           jo_gamepad_keys;
#else
typedef enum
{
    JO_KEY_RIGHT = 1 << 7,
    JO_KEY_LEFT	= 1 << 6,
    JO_KEY_DOWN = 1 << 5,
    JO_KEY_UP = 1 << 4,
    JO_KEY_START = 1 << 11,
    JO_KEY_A = 1 << 10,
    JO_KEY_B = 1 << 8,
    JO_KEY_C = 1 << 9,
    JO_KEY_X = 1 << 2,
    JO_KEY_Y = 1 << 1,
    JO_KEY_Z = 1 << 0,
    JO_KEY_L = 1 << 15,
    JO_KEY_R = 1 << 3,

    JO_MOUSE_START_BUTTON = 1 << 11,
    JO_MOUSE_LEFT_BUTTON = 1 << 10,
    JO_MOUSE_MIDDLE_BUTTON = 1 << 8,
    JO_MOUSE_RIGHT_BUTTON = 1 << 9
}           jo_gamepad_keys;
#endif

/** @brief Input type
 */
typedef enum
{
    JoNotConnectedGamepad = -1,
    JoUnsupportedGamepad = 0,
    JoRegularGamepad = 1,
    JoRegularMouse = 2,
    JoShuttleMouse = 3,
    JoRegularKeyboard = 4,

	/** @brief Nights 3D pad, Axis1 and Axis2 are analog stick, Axis3 is right trigger and Axis4 is left trigger
	 */
	JoNightsPad = 5
}           jo_gamepad_type;

/** @brief Input axis type
 */
typedef enum
{
    JoAxis1 = 0,
    JoAxis2 = 1,
    JoAxis3 = 2,
    JoAxis4 = 3,
    JoAxis5 = 4,
    JoAxis6 = 5,
}           jo_input_axis;	

/** @brief Get input type for the given port
 *  @param port Gamepad port
 *  @return Input type (Pad, Mouse, etc)
 */
jo_gamepad_type                 jo_get_input_type(const int port);

/** @brief Check if connected input is analog
 *  @param port input port
 *  @return true if connected controller has analog axies
 */
static __jo_force_inline bool	jo_is_input_analog(const int port)
{
	// TODO: Add other analog devices (wheel, mission stick)
	return (jo_inputs[port].id == PER_ID_NightsPad);
}

/** @brief Check if the input is available for the given port
 *  @param port Gamepad port
 *  @return true if the input is available otherwise false
 */
static  __jo_force_inline bool	jo_is_input_available(const int port)
{
    return (jo_inputs[port].id != PER_ID_NotConnect);
}

/** @brief Get the number of input available
 *  @return Input count
 */
int                             jo_get_input_count(void);

/** @brief Get analog axis value
 *  @param port Gamepad port
 *  @param axis Analog axis
 *  @return Analog axis value
 */
unsigned char                   jo_get_input_axis(const int port, const jo_input_axis axis);

/** @brief Check if the key is pressed for the given port
 *  @param port Gamepad port
 *  @param key Key value
 *  @return true if the key is pressed otherwise false
 */
static  __jo_force_inline bool	jo_is_input_key_pressed(const int port, const jo_gamepad_keys key)
{
    return __JO_KEY_PRESSED(port, key);
}

/** @brief Get the direction pressed for the given port
 *  @param port Gamepad port
 *  @return jo_8_directions
 */
static  __jo_force_inline jo_8_directions	jo_get_input_direction_pressed(const int port)
{
    if (__JO_KEY_PRESSED(port, JO_KEY_UP))
    {
        if (__JO_KEY_PRESSED(port, JO_KEY_LEFT))
            return (UP_LEFT);
        if (__JO_KEY_PRESSED(port, JO_KEY_RIGHT))
            return (UP_RIGHT);
        return (UP);
    }
    if (__JO_KEY_PRESSED(port, JO_KEY_DOWN))
    {
        if (__JO_KEY_PRESSED(port, JO_KEY_LEFT))
            return (DOWN_LEFT);
        if (__JO_KEY_PRESSED(port, JO_KEY_RIGHT))
            return (DOWN_RIGHT);
        return (DOWN);
    }
    if (__JO_KEY_PRESSED(port, JO_KEY_LEFT))
        return (LEFT);
    if (__JO_KEY_PRESSED(port, JO_KEY_RIGHT))
        return (RIGHT);
    return (NONE);
}

/** @brief Get raw input data for the given port
 *  @param port Gamepad port
 *  @return Raw input data
 */
static  __jo_force_inline unsigned short	jo_get_raw_input_key_pressed(const int port)
{
#if JO_COMPILE_USING_SGL
    return ((unsigned short)jo_inputs[port].data);
#else
    return ((unsigned short)jo_inputs[port].pressed);
#endif
}

/** @brief Check if the key is down for the given port
 *  @param port Gamepad port
 *  @param key Key value
 *  @return true if the key is down otherwise false
 */
static  __jo_force_inline bool	jo_is_input_key_down(const int port, const jo_gamepad_keys key)
{
#if JO_COMPILE_USING_SGL
    return ((jo_inputs[port].push & key) == 0);
#else
    return ((jo_inputs[port].on_keydown & key) == key);
#endif
}

#if !JO_COMPILE_USING_SGL

/** @brief Check if the key is up for the given port
 *  @param port Gamepad port
 *  @param key Key value
 *  @return true if the key is up otherwise false
 */
static  __jo_force_inline bool	jo_is_input_key_up(const int port, const jo_gamepad_keys key)
{
    return ((jo_inputs[port].on_keyup & key) == key);
}
#endif

#if JO_COMPILE_USING_SGL

/** @brief Get mouse X position
 *  @param port Gamepad port
 *  @return X coordinate
 */
static  __jo_force_inline int	jo_get_mouse_pos_x(const int port)
{
    return (JO_DIV_BY_65536(jo_inputs[port].dummy2[0]));
}

/** @brief Get mouse Y position
 *  @param port Gamepad port
 *  @return Y coordinate
 */
static  __jo_force_inline int	jo_get_mouse_pos_y(const int port)
{
    return (JO_DIV_BY_65536((JO_MULT_BY_65536(jo_inputs[port].dummy2[0]))));
}

#endif

/*
** HELPER
*/

/** @brief Check if the first gamepad is plugged
 *  @return true if the first gamepad is plugged otherwise false
 */
static  __jo_force_inline bool	jo_is_pad1_available(void)
{
    return jo_is_input_available(0);
}

/** @brief Check if the second gamepad is plugged
 *  @return true if the second gamepad is plugged otherwise false
 */
static  __jo_force_inline bool	jo_is_pad2_available(void)
{
    return jo_is_input_available(1);
}

/** @brief Check if the key is pressed on the first gamepad
 *  @param key Key value
 *  @return true if the key is pressed on the first gamepad otherwise false
 */
static  __jo_force_inline bool	jo_is_pad1_key_pressed(jo_gamepad_keys key)
{
    return (jo_is_input_key_pressed(0, key));
}

/** @brief Check if the key is down on the first gamepad
 *  @param key Key value
 *  @return true if the key is down on the first gamepad otherwise false
 */
static  __jo_force_inline bool	jo_is_pad1_key_down(jo_gamepad_keys key)
{
    return (jo_is_input_key_down(0, key));
}

/** @brief Check if the key is pressed on the second gamepad
 *  @param key Key value
 *  @return true if the key is pressed on the second gamepad otherwise false
 */
static  __jo_force_inline bool	jo_is_pad2_key_pressed(jo_gamepad_keys key)
{
    return (jo_is_input_key_pressed(1, key));
}

/** @brief Check if the key is down on the second gamepad
 *  @param key Key value
 *  @return true if the key is down on the second gamepad otherwise false
 */
static  __jo_force_inline bool	jo_is_pad2_key_down(jo_gamepad_keys key)
{
    return (jo_is_input_key_down(1, key));
}

#endif /* !__JO_INPUT_H__ */

/*
** END OF FILE
*/
