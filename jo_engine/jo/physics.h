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
/** @file physics.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine File Physics function prototypes
 *  @bug No known bugs.
 *  @warning Under development
 *  @todo Megaman, Contra Physics
 */

#ifndef __JO_PHYSICS_H__
# define __JO_PHYSICS_H__

/** @brief The physics of a side scroller: speeds in pixels per frame, accelerations in pixels per frame per frame (jo_fixed:
 *  no float, the SH-2 has no FPU) */
typedef struct
{
    bool        is_in_air;
    jo_fixed    air_acceleration_strength;
    jo_fixed    acceleration_strength;
    jo_fixed    friction;
    jo_fixed    deceleration_strength;
    jo_fixed    max_speed;
    jo_fixed    max_speed_y;
    jo_fixed    braking_animation_speed;
    jo_fixed    gravity;
    jo_fixed    jump_speed_y;
    jo_fixed    speed;
    jo_fixed    speed_y;            // (y down: < 0 going up)
}               jo_sidescroller_physics_params;

/*
** PHYSICS COMPUTATIONS
*/

/** @brief Friction computation: the speed goes to 0 by friction
 *  @param physics Sidescroller physics properties
 */
static  __jo_force_inline void              jo_physics_apply_friction(jo_sidescroller_physics_params *physics)
{
    if (physics->speed > 0)
        physics->speed -= JO_MIN(physics->speed, physics->friction);
    else
        physics->speed += JO_MIN(-physics->speed, physics->friction);
}

/** @brief Jump computation
 *  @param physics Sidescroller physics properties
 */
static  __jo_force_inline void            jo_physics_jump(jo_sidescroller_physics_params *physics)
{
    physics->speed_y = physics->jump_speed_y;
}

/** @brief Gravity computation
 *  @param physics Sidescroller physics properties
 */
static  __jo_force_inline void            jo_physics_apply_gravity(jo_sidescroller_physics_params *physics)
{
    physics->speed_y = JO_MIN(physics->speed_y + physics->gravity, physics->max_speed_y);
}

/** @brief Acceleration computation when going to the right
 *  @param physics Sidescroller physics properties
 */
static  __jo_force_inline void            jo_physics_accelerate_right(jo_sidescroller_physics_params *physics)
{
    physics->speed = JO_MIN(physics->speed + (physics->is_in_air ? physics->air_acceleration_strength : physics->acceleration_strength),
                            physics->max_speed);
}

/** @brief Acceleration computation when going to the left
 *  @param physics Sidescroller physics properties
 */
static  __jo_force_inline void            jo_physics_accelerate_left(jo_sidescroller_physics_params *physics)
{
    physics->speed = JO_MAX(physics->speed - (physics->is_in_air ? physics->air_acceleration_strength : physics->acceleration_strength),
                            -physics->max_speed);
}

/** @brief Deceleration computation when going to the right (pushed to the left): the speed goes down to 0
 *  @param physics Sidescroller physics properties
 */
static  __jo_force_inline void            jo_physics_decelerate_left(jo_sidescroller_physics_params *physics)
{
    physics->speed = JO_MAX(0, physics->speed - physics->deceleration_strength);
}

/** @brief Deceleration computation when going to the left (pushed to the right): the speed goes up to 0
 *  @param physics Sidescroller physics properties
 */
static  __jo_force_inline void            jo_physics_decelerate_right(jo_sidescroller_physics_params *physics)
{
    physics->speed = JO_MIN(physics->speed + physics->deceleration_strength, 0);
}

/*
** PHYSICS STATES TEST
*/

/** @brief Check if we doesn't move
 *  @param physics Sidescroller physics properties
 *  @return true if we doesn't move otherwise false
 */
static  __jo_force_inline bool            jo_physics_is_standing(jo_sidescroller_physics_params *physics)
{
    return (physics->speed == 0);
}

/** @brief Check if we almost doesn't move
 *  @param physics Sidescroller physics properties
 *  @param tolerance Tolerance (jo_fixed)
 *  @return true if we almost doesn't move (|speed| < tolerance) otherwise false
 */
static  __jo_force_inline bool            jo_physics_is_almost_standing(jo_sidescroller_physics_params *physics, const jo_fixed tolerance)
{
    return (JO_ABS(physics->speed) < tolerance);
}

/** @brief Check if we going on the right
 *  @param physics Sidescroller physics properties
 *  @return true if we going on the right otherwise false
 */
static  __jo_force_inline bool            jo_physics_is_going_on_the_right(jo_sidescroller_physics_params *physics)
{
    return (physics->speed > 0);
}

/** @brief Check if we going on the left
 *  @param physics Sidescroller physics properties
 *  @return true if we going on the left otherwise false
 */
static  __jo_force_inline bool            jo_physics_is_going_on_the_left(jo_sidescroller_physics_params *physics)
{
    return (physics->speed < 0);
}

/** @brief Check if we go fast enough to brake with an animation (a skid when the player pushes the other way)
 *  @param physics Sidescroller physics properties
 *  @return true if |speed| >= braking_animation_speed, to the left or to the right, otherwise false
 */
static  __jo_force_inline bool            jo_physics_should_brake(jo_sidescroller_physics_params *physics)
{
    return (JO_ABS(physics->speed) >= physics->braking_animation_speed);
}

/*
** HELPER
*/

/** @brief Init physics engine for a Sonic like game
 *  @param physics Sidescroller physics properties
 */
static  __jo_force_inline void            jo_physics_init_for_sonic(jo_sidescroller_physics_params *physics)
{
    physics->air_acceleration_strength = JO_FIXED_1 * 3 / 32;      // 0.09375
    physics->acceleration_strength = JO_FIXED_1 * 3 / 64;          // 0.046875
    physics->friction = JO_FIXED_1 * 3 / 64;
    physics->deceleration_strength = JO_FIXED_1_DIV_2;
    physics->max_speed = JO_INT_TO_FIXED(6);
    physics->max_speed_y = JO_FIXED_16;
    physics->braking_animation_speed = JO_FIXED_1 * 9 / 2;         // 4.5
    physics->gravity = JO_FIXED_1 * 7 / 32;                        // 0.21875
    physics->jump_speed_y = -JO_INT_TO_FIXED(7);
    physics->speed = JO_FIXED_0;
    physics->speed_y = JO_FIXED_0;
    physics->is_in_air = false;
}

#endif /* !__JO_PHYSICS_H__ */

/*
** END OF FILE
*/
