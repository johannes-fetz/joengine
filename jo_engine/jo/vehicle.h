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
/** @file vehicle.h
 *  @author Johannes Fetz
 *
 *  @brief Vehicles: entities of the class vehicle (a jeep, a truck), driven by the player. An actor (a model with a
 *  rigid skeleton, see jo_3d_model_draw_pose()): the bones of the wheels turn, the bones of the front wheels and of
 *  the steering wheel steer. A simple physics: the speed (engine, brakes, drag), the steering (the turn radius comes
 *  from the wheelbase), the 4 wheels on the ground (the height, the pitch and the roll of the body follow the ground
 *  under them, with a little suspension), the collisions (the ground says where it can't go, the solid entities stop
 *  it: a hit at speed damages them)
 *  @remarks The ground of the game is given by a callback (jo_vehicles_set_ground_callback())
 */

#ifndef __JO_VEHICLE_H__
# define __JO_VEHICLE_H__

#if defined(JO_COMPILE_WITH_LEVEL_SUPPORT) && defined(JO_COMPILE_WITH_3D_SUPPORT)

# include "entity.h"

/** @brief Height of the ground for the vehicles (given by the game)
 *  @param x Point (world space)
 *  @param z Point
 *  @param height Height of the ground (world units, Y up: the opposite of the Saturn Y), output
 *  @return false if a vehicle can't go there (deep water, a wall)
 */
typedef bool                        (*jo_vehicle_ground_callback)(const jo_fixed x, const jo_fixed z, int * const height);

/** @brief Data of a vehicle (the data of the class vehicle: an actor first) */
typedef struct
{
    jo_actor                        actor;
    jo_fixed                        speed;          // World units per second (< 0: backward)
    int                             steer;          // Angle of the front wheels (degrees, > 0: to the right)
    int                             pitch;          // The front up (degrees x 16)
    int                             roll;           // The left side up (degrees x 16)
    int                             wheel_angle;    // Turn of the wheels (degrees)
    jo_fixed                        lift;           // Suspension: the body above the mean of the wheels (Q16.16)
    jo_fixed                        lift_speed;
    short                           wheel_bones[4]; // Front left, front right, rear left, rear right (-1: none)
    short                           steer_bones[2]; // Front left, front right
    short                           steering_bone;  // The steering wheel (turns around its column, see steering_tilt)
    short                           steering_tilt_bone; // The parent of the steering wheel: its tilt (-1: none)
    short                           steering_tilt;  // The tilt toward the driver (degrees, around X): the model of the wheel is flat
    int                             wheelbase;      // Properties (world units, degrees, world units per second)
    int                             track;
    int                             wheel_radius;
    int                             max_steer;
    int                             max_speed;
    int                             acceleration;
    int                             braking;
    int                             impact;         // Damage of a hit at the full speed
    jo_3d_model                     *cockpit;       // Drawn when the player drives (property cockpit_model: what the
                                                    // driver sees, the same skeleton), JO_NULL: the model
    bool                            driven;         // The player drives it
    bool                            hit;            // It hit something (this frame)
}                                   jo_vehicle;

/** @brief Give the ground to the vehicles
 *  @param callback Ground (JO_NULL: flat ground at 0, everywhere)
 */
void                                jo_vehicles_set_ground_callback(const jo_vehicle_ground_callback callback);

/** @brief Drive a vehicle (each frame while the player is in it)
 *  @param entity Entity of the class vehicle
 *  @param throttle -256 (backward) to 256 (forward), 0: the engine is idle (it slows down)
 *  @param steering -256 (left) to 256 (right)
 *  @param brake The brakes
 *  @param elapsed Milliseconds
 */
void                                jo_vehicle_drive(jo_entity * const entity, const int throttle, const int steering, const bool brake,
                                                     const int elapsed);

/** @brief A point of the vehicle in the world (the seat of the driver, the door...)
 *  @param entity Entity of the class vehicle
 *  @param local Point in the space of the model (Saturn axes: Y down, the vehicle looks toward +Z)
 *  @param world Point in the world (output): turned by the yaw, the pitch and the roll of the vehicle
 */
void                                jo_vehicle_to_world(const jo_entity * const entity, const jo_pos3D_fixed * const local,
                                                        jo_pos3D_fixed * const world);

/** @brief The class vehicle (Internal usage: registered by jo_entities_load()) */
extern const jo_entity_class        __jo_vehicle_class;

/** @brief Get the vehicle data of an entity
 *  @param entity Entity
 *  @return Its data, JO_NULL if it is not a vehicle
 */
jo_vehicle                          *jo_vehicle_get(jo_entity * const entity);

#endif /* !JO_COMPILE_WITH_LEVEL_SUPPORT && JO_COMPILE_WITH_3D_SUPPORT */

#endif /* !__JO_VEHICLE_H__ */

/*
** END OF FILE
*/
