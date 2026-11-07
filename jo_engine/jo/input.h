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
/** @file input.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Gamepad functions
 *  @bug No known bugs.
 */

#ifndef __JO_INPUT_H__
# define __JO_INPUT_H__

# define JO_INPUT_MAX_DEVICE            (12)

/** @brief Index in jo_inputs of the first peripheral of each port (6 peripherals per port, see jo_get_inputs_vblank()) */
# define JO_INPUT_PORT1_FIRST_DEVICE    (0)
# define JO_INPUT_PORT2_FIRST_DEVICE    (6)

extern jo_peripheral                       jo_inputs[JO_INPUT_MAX_DEVICE];
# define __JO_KEY_PRESSED(PORT, KEY)    ((jo_inputs[PORT].data & KEY) == 0)


/** @brief Input keys
 */
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
    JO_MOUSE_RIGHT_BUTTON = 1 << 9,

    /** @brief Trigger of a light gun (the same bit as JO_KEY_A: a gun works with the code written for a pad) */
    JO_GUN_TRIGGER = 1 << 10,
    /** @brief Start button of a light gun (the same bit as JO_KEY_START) */
    JO_GUN_START_BUTTON = 1 << 11
}           jo_gamepad_keys;

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
	JoNightsPad = 5,
    
	/** @brief Mission stick, Axis1 and Axis2 are the analog stick, Axis3 is throttle
	 */
	JoMissionStick = 6,
    
	/** @brief Racing wheel, Axis1 is the wheel
	 */
	JoRacingWheel = 7,

    /** @brief Light gun (Virtua Gun, Stunner): JO_GUN_TRIGGER, JO_GUN_START_BUTTON and jo_get_input_gun_position()
     */
    JoLightGun = 8
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
	return (jo_inputs[port].id == JO_PERIPHERAL_ID_NIGHTS_PAD);
}

/** @brief Check if the input is available for the given port
 *  @param port Gamepad port
 *  @return true if the input is available otherwise false
 */
static  __jo_force_inline bool	jo_is_input_available(const int port)
{
    return (jo_inputs[port].id != JO_PERIPHERAL_ID_NOT_CONNECTED);
}

/** @brief Get the number of input available
 *  @return Input count
 */
int                             jo_get_input_count(void);

/** @brief The port (the index of jo_inputs) of the gamepad of a player: the devices of the port 1 are 0 to 5 (a multitap),
 *  those of the port 2 are 6 to 11; 2 gamepads without multitap: 0 and 6
 *  @param player 0: the first connected device, 1: the second one...
 *  @return The port, -1 if there are not so many devices
 */
int                             jo_get_player_input_port(const int player);

/** @brief Get analog axis value
 *  @param port Gamepad port
 *  @param axis Analog axis
 *  @return Analog axis value
 */
unsigned char                   jo_get_input_axis(const int port, const jo_input_axis axis);

/** @brief Get a centered analog axis (a stick or a wheel): 0 in the middle
 *  @param port Gamepad port
 *  @param axis Analog axis: JoAxis1 and JoAxis2 (the stick of the Nights pad and of the mission stick), JoAxis1 (the
 *  racing wheel)
 *  @return -128 (left, up) to 127 (right, down), 0 if the device has no such centered axis (not connected, a trigger,
 *  the throttle: see jo_get_input_axis(), 0 to 255)
 *  @remarks jo_get_input_axis() gives 0 to 255 with the middle at 128 (128 is subtracted here)
 */
int                             jo_get_input_axis_centered(const int port, const jo_input_axis axis);

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
    return ((unsigned short)jo_inputs[port].data);
}

/** @brief Check if the key is down for the given port
 *  @param port Gamepad port
 *  @param key Key value
 *  @return true if the key is down otherwise false
 */
static  __jo_force_inline bool	jo_is_input_key_down(const int port, const jo_gamepad_keys key)
{
    return ((jo_inputs[port].push & key) == 0);
}



/** @brief Get mouse X position
 *  @param port Gamepad port
 *  @return X coordinate
 */
static  __jo_force_inline int	jo_get_mouse_pos_x(const int port)
{
    return (((jo_peripheral_pointer *)&jo_inputs[port])->x);
}

/** @brief Get mouse Y position
 *  @param port Gamepad port
 *  @return Y coordinate
 */
static  __jo_force_inline int	jo_get_mouse_pos_y(const int port)
{
    return (((jo_peripheral_pointer *)&jo_inputs[port])->y);
}


/*
** LIGHT GUN
*/

/** @brief Get the position on the screen where the light gun points
 *  @param port Gamepad port: 0 (port 1) or JO_INPUT_PORT2_FIRST_DEVICE (port 2), the gun can't be used behind a multitap
 *  @param x Horizontal position in pixels of the screen (0 to jo_core_get_screen_width() - 1, from the left), -1 when false
 *  is returned
 *  @param y Vertical position in lines of the screen (0 to jo_core_get_screen_height() - 1, from the top), -1 when false is
 *  returned
 *  @return true if the light sensor of the gun saw the screen during the last frame, false if it points out of the screen,
 *  at a dark area of the screen, or if there is no gun on the port
 *  @remarks The gun is found automatically (jo_get_input_type() returns JoLightGun). The trigger and the start button are
 *  read like the keys of a pad: jo_is_input_key_down(port, JO_GUN_TRIGGER), jo_is_pad1_key_pressed(JO_GUN_START_BUTTON)...
 *  @remarks How it works: the light sensor sees the beam of the CRT draw the pixel, the VDP2 keeps the position of the
 *  beam at this time (HV counter latch). Only a CRT TV works (no LCD TV, no upscaler): a gun sees nothing on the other
 *  TVs. The gun sees a bright area only: when the trigger is pulled at a dark area (false is returned), draw a white
 *  screen for 2 frames: the position comes 2 frames after the trigger (see the sample "demo - light gun")
 *  @remarks With 2 guns, each one gets a position every other field (the VDP2 has one latch)
 *  @remarks Each gun and each TV is a little different: see jo_calibrate_input_gun()
 *  @code
 *  int     x;
 *  int     y;
 *
 *  if (jo_get_input_type(0) == JoLightGun && jo_is_input_key_down(0, JO_GUN_TRIGGER) && jo_get_input_gun_position(0, &x, &y))
 *      jo_sprite_draw3D(hit_sprite_id, x - JO_TV_WIDTH_2, y - JO_TV_HEIGHT_2, 100);
 *  @endcode
 */
bool                            jo_get_input_gun_position(const int port, int * const x, int * const y);

/** @brief Calibrate a light gun: the player shoots at a target, the position read is moved onto the target
 *  @param port Gamepad port: 0 (port 1) or JO_INPUT_PORT2_FIRST_DEVICE (port 2)
 *  @param target_x Horizontal position of the target (pixels of the screen, from the left)
 *  @param target_y Vertical position of the target (pixels of the screen, from the top)
 *  @return true if the gun saw the screen during the last frame (the next positions are corrected), false otherwise
 *  (nothing changes)
 *  @remarks Sega's advice: draw a cross at the center of the screen, ask the player to aim at it and to pull the trigger,
 *  then call jo_calibrate_input_gun(port, JO_TV_WIDTH_2, JO_TV_HEIGHT_2). The correction of each port is kept until the
 *  next calibration (calibrate again after jo_core_set_screen_resolution())
 *  @remarks Before the calibration, the position is corrected by the delay of the gun measured in Mednafen (29 dots in the
 *  320 and 640 pixel modes, 31 dots in the 352 and 704 pixel modes)
 */
bool                            jo_calibrate_input_gun(const int port, const int target_x, const int target_y);

/*
** HELPER
*/

/** @brief Check if the first gamepad is plugged
 *  @return true if the first gamepad is plugged otherwise false
 */
static  __jo_force_inline bool	jo_is_pad1_available(void)
{
    return jo_is_input_available(JO_INPUT_PORT1_FIRST_DEVICE);
}

/** @brief Check if the second gamepad is plugged (the second device: on the port 2, or on a multitap of the port 1)
 *  @return true if the second gamepad is plugged otherwise false
 */
static  __jo_force_inline bool	jo_is_pad2_available(void)
{
    return (jo_get_player_input_port(1) >= 0);
}

/** @brief Check if the key is pressed on the first gamepad
 *  @param key Key value
 *  @return true if the key is pressed on the first gamepad otherwise false
 */
static  __jo_force_inline bool	jo_is_pad1_key_pressed(jo_gamepad_keys key)
{
    return (jo_is_input_key_pressed(JO_INPUT_PORT1_FIRST_DEVICE, key));
}

/** @brief Check if the key is down on the first gamepad
 *  @param key Key value
 *  @return true if the key is down on the first gamepad otherwise false
 */
static  __jo_force_inline bool	jo_is_pad1_key_down(jo_gamepad_keys key)
{
    return (jo_is_input_key_down(JO_INPUT_PORT1_FIRST_DEVICE, key));
}

/** @brief Check if the key is pressed on the second gamepad
 *  @param key Key value
 *  @return true if the key is pressed on the second gamepad otherwise false
 */
static  __jo_force_inline bool	jo_is_pad2_key_pressed(jo_gamepad_keys key)
{
    int                         port;

    port = jo_get_player_input_port(1);
    return (port >= 0 && jo_is_input_key_pressed(port, key));
}

/** @brief Check if the key is down on the second gamepad
 *  @param key Key value
 *  @return true if the key is down on the second gamepad otherwise false
 */
static  __jo_force_inline bool	jo_is_pad2_key_down(jo_gamepad_keys key)
{
    int                         port;

    port = jo_get_player_input_port(1);
    return (port >= 0 && jo_is_input_key_down(port, key));
}

#endif /* !__JO_INPUT_H__ */

/*
** END OF FILE
*/
