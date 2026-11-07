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
** Vehicles: see jo/vehicle.h
*/

#include <stdbool.h>
#include "jo/conf.h"

#if defined(JO_COMPILE_WITH_LEVEL_SUPPORT) && defined(JO_COMPILE_WITH_3D_SUPPORT)

#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/core.h"
#include "jo/tools.h"
#include "jo/malloc.h"
#include "jo/colors.h"
#include "jo/vdp1_command_pipeline.h"
#include "jo/sprites.h"
#include "jo/fs.h"
#include "jo/math.h"
#include "jo/list.h"
#include "jo/image.h"
#include "jo/tga.h"
#include "jo/3d.h"
#include "jo/model.h"
#include "jo/audio.h"
#include "jo/level.h"
#include "jo/effects.h"
#include "jo/entity.h"
#include "jo/vehicle.h"

/** @brief The front wheels turn this fast (degrees per second), and come back */
#define JO_VEHICLE_STEER_SPEED              (140)
/** @brief Drag of the engine when the throttle is released (world units per second per second) */
#define JO_VEHICLE_DRAG                     (140)
/** @brief A wheel can't go up more than this (world units): a wall, a step */
#define JO_VEHICLE_MAX_STEP                 (34)

static jo_vehicle_ground_callback   __jo_vehicle_ground = JO_NULL;

void                                jo_vehicles_set_ground_callback(const jo_vehicle_ground_callback callback)
{
    __jo_vehicle_ground = callback;
}

jo_vehicle                          *jo_vehicle_get(jo_entity * const entity)
{
    if (entity == JO_NULL || entity->data == JO_NULL || entity->cls == JO_NULL || entity->cls->spawn != __jo_vehicle_class.spawn)
        return (JO_NULL);
    return ((jo_vehicle *)entity->data);
}

/** @brief Height of the ground (Y up), false where it can't go */
static bool                         __jo_vehicle_ground_at(const jo_fixed x, const jo_fixed z, int * const height)
{
    if (__jo_vehicle_ground == JO_NULL)
    {
        *height = 0;
        return (true);
    }
    return (__jo_vehicle_ground(x, z, height));
}

void                                jo_vehicle_to_world(const jo_entity * const entity, const jo_pos3D_fixed * const local,
                                                        jo_pos3D_fixed * const world)
{
    const jo_vehicle                *v;
    jo_fixed                        c;
    jo_fixed                        s;
    jo_fixed                        x;
    jo_fixed                        y;
    jo_fixed                        z;
    jo_fixed                        t;

    v = (const jo_vehicle *)entity->data;
    x = local->x;
    y = local->y;
    z = local->z;
    // The same order as the drawing: the roll (Z), the pitch (X), then the heading (Y)
    c = jo_cos(v->roll >> 4);
    s = jo_sin(v->roll >> 4);
    t = jo_fixed_mult(x, c) - jo_fixed_mult(y, s);
    y = jo_fixed_mult(x, s) + jo_fixed_mult(y, c);
    x = t;
    c = jo_cos(v->pitch >> 4);
    s = jo_sin(v->pitch >> 4);
    t = jo_fixed_mult(y, c) - jo_fixed_mult(z, s);
    z = jo_fixed_mult(y, s) + jo_fixed_mult(z, c);
    y = t;
    c = jo_cos(entity->yaw);
    s = jo_sin(entity->yaw);
    world->x = entity->position.x + jo_fixed_mult(x, c) + jo_fixed_mult(z, s);
    world->y = entity->position.y + y;
    world->z = entity->position.z - jo_fixed_mult(x, s) + jo_fixed_mult(z, c);
}

/** @brief The bumpers are this far in front of the front wheels and behind the rear wheels (world units) */
#define JO_VEHICLE_OVERHANG                 (20)
/** @brief The body of a vehicle sticks out of its wheels on the sides (world units, the fenders) */
#define JO_VEHICLE_SIDE                     (8)

/** @brief The 4 wheels on the ground at a place: their heights (Y up: front left, front right, rear left, rear right)
 *  @return false if a wheel can't go there
 */
static bool                         __jo_vehicle_wheels(const jo_entity * const entity, const jo_vehicle * const v, const jo_fixed x,
                                                        const jo_fixed z, const int yaw, int heights[4])
{
    jo_fixed                        c;
    jo_fixed                        s;
    jo_fixed                        hx;
    jo_fixed                        hz;
    int                             k;
    int                             current;
    int                             current_body;

    c = jo_cos(yaw);
    s = jo_sin(yaw);
    current = jo_fixed2int(-entity->position.y);
    for (k = 0; k < 4; ++k)
    {
        // (the model looks toward +Z: the front wheels at +wheelbase / 2, the left wheels at -track / 2)
        hx = jo_int2fixed((k & 1) ? v->track / 2 : -v->track / 2);
        hz = jo_int2fixed((k & 2) ? -v->wheelbase / 2 : v->wheelbase / 2);
        if (!__jo_vehicle_ground_at(x + jo_fixed_mult(hx, c) + jo_fixed_mult(hz, s), z - jo_fixed_mult(hx, s) + jo_fixed_mult(hz, c),
                                    &heights[k]) || heights[k] > current + JO_VEHICLE_MAX_STEP)
            return (false);
    }
    // The body between the wheels: the middle of the bumpers and of the sides (a trunk between the front wheels stops
    // the vehicle too)
    for (k = 0; k < 4; ++k)
    {
        hx = jo_int2fixed(k == 2 ? -v->track / 2 : (k == 3 ? v->track / 2 : 0));
        hz = jo_int2fixed(k == 0 ? v->wheelbase / 2 + JO_VEHICLE_OVERHANG : (k == 1 ? -v->wheelbase / 2 - JO_VEHICLE_OVERHANG : 0));
        if (!__jo_vehicle_ground_at(x + jo_fixed_mult(hx, c) + jo_fixed_mult(hz, s), z - jo_fixed_mult(hx, s) + jo_fixed_mult(hz, c),
                                    &current_body))
            return (false);
    }
    return (true);
}

void                                jo_vehicle_drive(jo_entity * const entity, const int throttle, const int steering, const bool brake,
                                                     const int elapsed)
{
    jo_vehicle                      *v;
    jo_entity                       *other;
    jo_fixed                        step;
    jo_fixed                        target;
    jo_fixed                        distance;
    jo_fixed                        nx;
    jo_fixed                        nz;
    jo_fixed                        mean;
    int                             heights[4];
    int                             d;
    int                             yaw;
    int                             damage;

    v = jo_vehicle_get(entity);
    if (v == JO_NULL || elapsed <= 0)
        return;
    v->hit = false;
    // The front wheels: toward the steering, back to the middle
    target = steering * v->max_steer / 256;
    d = JO_VEHICLE_STEER_SPEED * elapsed / 1000;
    if (v->steer < target)
        v->steer = JO_MIN(v->steer + d, target);
    else if (v->steer > target)
        v->steer = JO_MAX(v->steer - d, target);
    // The speed: the engine (less near the top speed), the brakes, the drag
    step = jo_int2fixed(v->acceleration) / 1000 * elapsed;
    if (brake || (throttle > 0 && v->speed < 0) || (throttle < 0 && v->speed > 0))
    {
        step = jo_int2fixed(v->braking) / 1000 * elapsed;
        v->speed = v->speed > 0 ? JO_MAX(v->speed - step, 0) : JO_MIN(v->speed + step, 0);
    }
    else if (throttle > 0)
        v->speed += jo_fixed_mult(step * throttle / 256, JO_FIXED_1 - v->speed / v->max_speed);
    else if (throttle < 0)
        v->speed += jo_fixed_mult(step * throttle / 256, JO_FIXED_1 + v->speed / (v->max_speed / 3));
    else
    {
        step = jo_int2fixed(JO_VEHICLE_DRAG) / 1000 * elapsed;
        v->speed = v->speed > 0 ? JO_MAX(v->speed - step, 0) : JO_MIN(v->speed + step, 0);
    }
    // The distance of this frame, the turn: the wheelbase and the angle of the front wheels (the turn radius is
    // wheelbase / tan(steer))
    distance = v->speed / 1000 * elapsed;
    yaw = entity->yaw;
    if (v->steer != 0 && distance != 0)
    {
        // (radians to degrees: 57.3; the fraction of a degree is kept in entity->state)
        target = jo_fixed_div(jo_fixed_mult(distance, jo_fixed_div(jo_sin(v->steer), jo_cos(v->steer))), jo_int2fixed(v->wheelbase)) * 573 / 10;
        d = (int)((entity->state + target) >> 16);
        entity->state = (entity->state + target) - jo_int2fixed(d);
        yaw = (yaw + d + 360) % 360;
    }
    nx = entity->position.x + jo_fixed_mult(jo_sin(yaw), distance);
    nz = entity->position.z + jo_fixed_mult(jo_cos(yaw), distance);
    // The solid entities in front (or behind): it stops, a hit at speed damages them
    other = JO_NULL;
    if (distance != 0)
    {
        step = jo_int2fixed(distance > 0 ? v->wheelbase / 2 + 34 : -v->wheelbase / 2 - 34);
        other = jo_entities_get_blocking(nx + jo_fixed_mult(jo_sin(yaw), step), nz + jo_fixed_mult(jo_cos(yaw), step), v->track / 2, entity);
    }
    if (other != JO_NULL || !__jo_vehicle_wheels(entity, v, nx, nz, yaw, heights))
    {
        if (other != JO_NULL && JO_ABS(v->speed) > jo_int2fixed(v->max_speed / 6))
        {
            damage = v->impact * JO_ABS(jo_fixed2int(v->speed)) / v->max_speed;
            jo_entity_damage(other, damage, entity);
        }
        // A bounce
        v->speed = -v->speed / 4;
        v->hit = true;
        if (!__jo_vehicle_wheels(entity, v, entity->position.x, entity->position.z, entity->yaw, heights))
            return;
    }
    else
    {
        entity->position.x = nx;
        entity->position.z = nz;
        entity->yaw = yaw;
    }
    // The body on its 4 wheels: the mean height, the pitch (front - rear) and the roll (left - right)
    mean = jo_int2fixed(heights[0] + heights[1] + heights[2] + heights[3]) / 4;
    target = jo_atan2_degrees(jo_int2fixed(heights[0] + heights[1] - heights[2] - heights[3]) / 2, jo_int2fixed(v->wheelbase)) * 16;
    v->pitch += (target - v->pitch) / 2;
    target = jo_atan2_degrees(jo_int2fixed(heights[0] + heights[2] - heights[1] - heights[3]) / 2, jo_int2fixed(v->track)) * 16;
    v->roll += (target - v->roll) / 2;
    // Suspension: the body follows the ground with a little delay (a bump lifts it)
    v->lift_speed += (mean - (-entity->position.y + v->lift)) / 4;
    v->lift_speed = v->lift_speed * 3 / 5;
    v->lift += v->lift_speed;
    if (v->lift > JO_FIXED_1 * 6)
        v->lift = JO_FIXED_1 * 6;
    else if (v->lift < -JO_FIXED_1 * 6)
        v->lift = -JO_FIXED_1 * 6;
    entity->position.y = -mean;
    // The wheels turn: the distance / the radius (radians to degrees: 57.3 in fixed point, distance * 573 overflowed
    // above 57 world units per update)
    v->wheel_angle = (v->wheel_angle + (jo_fixed2int(jo_fixed_mult(distance, 3755213)) / v->wheel_radius) % 360 + 360) % 360;
    entity->center_dy = -jo_int2fixed(30);
    entity->center.x = entity->position.x;
    entity->center.z = entity->position.z;
    entity->center.y = entity->position.y - jo_int2fixed(30);
}

static void                         __jo_vehicle_spawn(jo_entity * const entity)
{
    static const char * const       wheels[4] = {"wheel_fl", "wheel_fr", "wheel_rl", "wheel_rr"};
    jo_vehicle                      *v;
    int                             heights[4];
    int                             k;

    jo_actor_spawn(entity);
    v = (jo_vehicle *)entity->data;
    if (v == JO_NULL)
        return;
    for (k = 0; k < 4; ++k)
        v->wheel_bones[k] = (short)(v->actor.model != JO_NULL ? jo_3d_model_find_bone(v->actor.model, wheels[k]) : -1);
    v->steer_bones[0] = (short)(v->actor.model != JO_NULL ? jo_3d_model_find_bone(v->actor.model, "steer_fl") : -1);
    v->steer_bones[1] = (short)(v->actor.model != JO_NULL ? jo_3d_model_find_bone(v->actor.model, "steer_fr") : -1);
    v->steering_bone = (short)(v->actor.model != JO_NULL ? jo_3d_model_find_bone(v->actor.model, "steering") : -1);
    v->steering_tilt_bone = (short)(v->actor.model != JO_NULL ? jo_3d_model_find_bone(v->actor.model, "steering_tilt") : -1);
    v->cockpit = jo_entity_get_model(jo_entity_get_string(entity, "cockpit_model", ""));
    if (v->cockpit != JO_NULL && v->actor.model != JO_NULL && v->cockpit->bone_count != v->actor.model->bone_count)
        v->cockpit = JO_NULL;
    v->wheelbase = JO_MAX(jo_entity_get_int(entity, "wheelbase", 106), 16);
    v->track = JO_MAX(jo_entity_get_int(entity, "track", 78), 16);
    // The collisions of the player with the vehicle: its box (longer than wide), not the cylinder (the player went
    // through its front and its back)
    entity->box_x = (short)(v->track / 2 + JO_VEHICLE_SIDE);
    entity->box_z = (short)(v->wheelbase / 2 + JO_VEHICLE_OVERHANG);
    v->wheel_radius = JO_MAX(jo_entity_get_int(entity, "wheel_radius", 19), 4);
    v->max_steer = jo_entity_get_int(entity, "max_steer", 30);
    // (the steering wheel of the model is flat, facing the driver: tilted by the parent bone of its bone, it turns around
    // its column)
    v->steering_tilt = (short)jo_entity_get_int(entity, "steering_tilt", 0);
    v->max_speed = JO_MAX(jo_entity_get_int(entity, "max_speed", 900), 60);
    v->acceleration = jo_entity_get_int(entity, "acceleration", 280);
    v->braking = jo_entity_get_int(entity, "braking", 800);
    v->impact = jo_entity_get_int(entity, "impact", 150);
    // (the fraction of a degree of its yaw is in entity->state, Q16.16)
    entity->state = 0;
    // On the ground (its wheels, its pitch and its roll)
    if (__jo_vehicle_ground_at(entity->position.x, entity->position.z, &k))
        entity->position.y = jo_int2fixed(-k);
    if (__jo_vehicle_wheels(entity, v, entity->position.x, entity->position.z, entity->yaw, heights))
        jo_vehicle_drive(entity, 0, 0, true, 1);
}

static void                         __jo_vehicle_draw(jo_entity * const entity)
{
    jo_vehicle                      *v;
    jo_3d_model                     *model;
    jo_3d_pose                      pose;
    int                             distance;
    int                             k;

    v = (jo_vehicle *)entity->data;
    if (v == JO_NULL || v->actor.model == JO_NULL)
        return;
    distance = v->driven ? 0 : jo_entity_view_distance(entity);
    if (distance < 0)
        return;
    model = v->actor.far_model != JO_NULL && distance > entity->far_distance ? v->actor.far_model : v->actor.model;
    // (the driver sees only the front of the cockpit)
    if (v->driven && v->cockpit != JO_NULL)
        model = v->cockpit;
    jo_memset(&pose, 0, sizeof(pose));
    // The wheels turn around X (forward: the top goes toward +Z), the front ones steer around Y, the steering wheel
    // around its axis
    for (k = 0; k < 4; ++k)
        if (v->wheel_bones[k] >= 0)
            pose.angles[v->wheel_bones[k]][0] = (jo_angle)(-v->wheel_angle * 65536 / 360);
    for (k = 0; k < 2; ++k)
        if (v->steer_bones[k] >= 0)
            pose.angles[v->steer_bones[k]][1] = (jo_angle)(v->steer * 65536 / 360);
    if (v->steering_bone >= 0)
        pose.angles[v->steering_bone][2] = (jo_angle)(v->steer * 3 * 65536 / 360);
    if (v->steering_tilt_bone >= 0)
        pose.angles[v->steering_tilt_bone][0] = (jo_angle)(v->steering_tilt * 65536 / 360);
    pose.root.y = -v->lift;
    jo_3d_push_matrix();
    {
        jo_3d_translate_matrix_fixed(entity->position.x, entity->position.y, entity->position.z);
        __jo_3d_rotate(JO_3D_AXIS_Y, (jo_angle)(entity->yaw * 65536 / 360 + (entity->state >> 7) * 65536 / (360 << 9)));
        if (v->pitch != 0)
            __jo_3d_rotate(JO_3D_AXIS_X, (jo_angle)(v->pitch * 4096 / 360));
        if (v->roll != 0)
            __jo_3d_rotate(JO_3D_AXIS_Z, (jo_angle)(v->roll * 4096 / 360));
        jo_3d_model_draw_pose(model, &pose, JO_NULL, JO_NULL);
    }
    jo_3d_pop_matrix();
}

const jo_entity_class               __jo_vehicle_class = {"vehicle", sizeof(jo_vehicle), __jo_vehicle_spawn, JO_NULL, JO_NULL,
                                                          JO_NULL, __jo_vehicle_draw, JO_NULL, JO_NULL};

#endif /* !JO_COMPILE_WITH_LEVEL_SUPPORT && JO_COMPILE_WITH_3D_SUPPORT */

/*
** END OF FILE
*/
