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
/** @file physics.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine File Physics function prototypes
 *  @bug No known bugs.
 *  @warning Under development
 *  @todo Fixed point math
 *  @todo Megaman, Contra Physics
 */

#ifndef __JO_PHYSICS_H__
# define __JO_PHYSICS_H__

typedef struct
{
    bool    is_in_air;
    float   air_acceleration_strength;
    float   acceleration_strength;
    float   friction;
    float   deceleration_strength;
    float   max_speed;
    float   max_speed_y;
    float   braking_animation_speed;
    float   gravity;
    float   jump_speed_y;
    float   speed;
    float   speed_y;
}           jo_sidescroller_physics_params;

/*
** PHYSICS COMPUTATIONS
*/

/** @brief Friction computation
 *  @param physics Sidescroller physics properties
 */
static  __jo_force_inline void              jo_physics_apply_friction(jo_sidescroller_physics_params *physics)
{
    physics->speed -= JO_MIN(JO_ABS(physics->speed), physics->friction) * (physics->speed > 0 ? 1 : -1);
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
    physics->speed_y += physics->gravity;
    if (physics->speed_y > physics->max_speed_y)
        physics->speed_y = physics->max_speed_y;
}

/** @brief Acceleration computation when going to the right
 *  @param physics Sidescroller physics properties
 */
static  __jo_force_inline void            jo_physics_accelerate_right(jo_sidescroller_physics_params *physics)
{
    physics->speed += (physics->is_in_air ? physics->air_acceleration_strength : physics->acceleration_strength);
    physics->speed = JO_MIN(physics->speed, physics->max_speed);
}

/** @brief Acceleration computation when going to the left
 *  @param physics Sidescroller physics properties
 */
static  __jo_force_inline void            jo_physics_accelerate_left(jo_sidescroller_physics_params *physics)
{
    physics->speed -= (physics->is_in_air ? physics->air_acceleration_strength : physics->acceleration_strength);
    physics->speed = JO_MAX(physics->speed, JO_CHANGE_SIGN(physics->max_speed));
}

/** @brief Deceleration computation when going to the right
 *  @param physics Sidescroller physics properties
 */
static  __jo_force_inline void            jo_physics_decelerate_left(jo_sidescroller_physics_params *physics)
{
    physics->speed -=  physics->deceleration_strength;
    physics->speed = JO_MAX(0, physics->speed);
}

/** @brief Deceleration computation when going to the left
 *  @param physics Sidescroller physics properties
 */
static  __jo_force_inline void            jo_physics_decelerate_right(jo_sidescroller_physics_params *physics)
{
    physics->speed +=  physics->deceleration_strength;
    physics->speed = JO_MIN(physics->speed, 0);
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
    return (jo_is_float_equals_zero(physics->speed));
}

/** @brief Check if we almost doesn't move
 *  @param physics Sidescroller physics properties
 *  @param tolerance Tolerance
 *  @return true if we almost doesn't move otherwise false
 */
static  __jo_force_inline bool            jo_physics_is_almost_standing(jo_sidescroller_physics_params *physics, float tolerance)
{
    return (JO_ABS(physics->speed) < tolerance);
}

/** @brief Check if we going on the right
 *  @param physics Sidescroller physics properties
 *  @return true if we going on the right otherwise false
 */
static  __jo_force_inline bool            jo_physics_is_going_on_the_right(jo_sidescroller_physics_params *physics)
{
    return (physics->speed > 0.0f);
}

/** @brief Check if we going on the left
 *  @param physics Sidescroller physics properties
 *  @return true if we going on the left otherwise false
 */
static  __jo_force_inline bool            jo_physics_is_going_on_the_left(jo_sidescroller_physics_params *physics)
{
    return (physics->speed < 0.0f);
}

/** @brief Check if we should brake depending on the direction
 *  @param physics Sidescroller physics properties
 *  @return true if we should brake depending on the direction otherwise false
 */
static  __jo_force_inline bool            jo_physics_should_brake(jo_sidescroller_physics_params *physics)
{
    if (jo_physics_is_going_on_the_left(physics))
        return (physics->speed >= physics->braking_animation_speed);
    return (physics->speed <= JO_CHANGE_SIGN(physics->braking_animation_speed));
}

/*
** HELPER
*/

/** @brief Init physics engine for a Sonic like game
 *  @param physics Sidescroller physics properties
 */
static  __jo_force_inline void            jo_physics_init_for_sonic(jo_sidescroller_physics_params *physics)
{
    physics->air_acceleration_strength = 0.09375f;
    physics->acceleration_strength = 0.046875f;
    physics->friction = 0.046875f;
    physics->deceleration_strength = 0.5f;
    physics->max_speed = 6.0f;
    physics->max_speed_y = 16.0f;
    physics->braking_animation_speed = 4.5f;
    physics->gravity = 0.21875f;
    physics->jump_speed_y = -7.0f;
    physics->speed = 0.0f;
    physics->speed_y = 0.0f;
    physics->is_in_air = false;
};

#endif /* !__JO_PHYSICS_H__ */

/*
** END OF FILE
*/
