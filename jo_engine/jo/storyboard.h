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
/** @file storyboard.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Storyboard Animation function prototypes
 *  @bug No known bugs.
 */

#ifndef __JO_STORYBOARD_H__
# define __JO_STORYBOARD_H__

#ifdef JO_COMPILE_WITH_STORYBOARD_SUPPORT

/** @brief Specify that the animation is infinite
  * @remarks For jo_storyboard_create_animation() and other helper
  */
# define JO_STORYBOARD_INFINITE_DURATION    (0)

/** @brief Internal object cache
 *  @warning MC Hammer: don't touch this
 */
typedef struct
{
    jo_3d_object_attributes     *user;
    int                         dx;
    int                         dy;
    short                       current_cos;
    short                       current_sin;
}                               jo_storyboard_object_cache;

/** @brief Object animation definition */
typedef struct
{
    bool                disabled;
    unsigned int        count;
    unsigned short      frame;
    unsigned char       frame_skip;
    unsigned int        limit;
    char                rotation_speed;
    short               cos_radius;
    short               sin_radius;
    short               direction;
    char                translation_speed_x;
    char                translation_speed_y;
}                       jo_animation;

/** @brief Storyboard definition
  * @remarks A storyboard could contains multiple animation on multiple object
  */
typedef struct
{
    jo_list             objects;
    jo_list             animations;
    bool                repeat;
    bool                pause;
}                       jo_storyboard;

/*
** STORYBOARD AND ANIMATION FACTORY
*/

/** @brief Create an empty storyboard
  * @param autoplay Play the storyboard immediately
  * @param repeat Play the storyboard in loop (each animation must have a limited duration)
  * @return An empty storyboard
  * @warning repeat only works if all animation inside storyboard have a limited duration
  * @see jo_storyboard_destroy()
  */
jo_storyboard                               *jo_storyboard_create(const bool autoplay, const bool repeat);

/** @brief Destroy the storyboard
  * @param storyboard Storyboard to destroy
  * @see jo_storyboard_create()
  */
void                                        jo_storyboard_destroy(jo_storyboard *const storyboard);

/** @brief Create an empty animation
  * @param storyboard Storyboard
  * @param frame_skip Number of frame to skip (slowdown animation but preserve CPU)
  * @param duration Duration of animation (you can pass JO_STORYBOARD_INFINITE_DURATION for infinite)
  * @return An empty animation
  * @see jo_storyboard_destroy_animation()
  * @see jo_storyboard_create()
  */
jo_animation                                *jo_storyboard_create_animation(jo_storyboard * const storyboard, const unsigned char frame_skip, const unsigned int duration);

/** @brief Destroy animation
  * @param storyboard Storyboard which contains the animation
  * @param animation Animation to destroy
  * @see jo_storyboard_create_animation()
  */
static  __jo_force_inline void              jo_storyboard_destroy_animation(jo_storyboard * const storyboard, const jo_animation * const animation)
{
    jo_list_remove_first_value(&storyboard->animations, (jo_list_data)(void *)animation);
}

/*
** STORYBOARD CONTROL
*/

/** @brief Storyboard repeat control
  * @param storyboard Storyboard
  * @param repeat Repeat
  */
static  __jo_force_inline void      jo_storyboard_repeat(jo_storyboard * const storyboard, bool repeat)
{
    storyboard->repeat = repeat;
}

/** @brief Suspend all storyboard animation
  * @param storyboard Storyboard
  */
static  __jo_force_inline void      jo_storyboard_suspend(jo_storyboard * const storyboard)
{
    storyboard->pause = true;
}

/** @brief Resume all suspended storyboard animation
  * @param storyboard Storyboard
  */
static  __jo_force_inline void      jo_storyboard_resume(jo_storyboard * const storyboard)
{
    storyboard->pause = false;
}

/** @brief Play storyboard animation
  * @param storyboard Storyboard
  */
static  __jo_force_inline void      jo_storyboard_play(jo_storyboard * const storyboard)
{
    storyboard->pause = false;
}

/** @brief Switch between resume and suspend
  * @param storyboard Storyboard
  */
static  __jo_force_inline void      jo_storyboard_toggle(jo_storyboard * const storyboard)
{
    storyboard->pause ^= true;
}

/*
** ADD/REMOVE OBJECT IN STORYBOARD
*/

/** @brief Add object in storyboard
  * @param storyboard Storyboard
  * @param object Pointer to an object (jo_pos2D, jo_pos3D, jo_2d_object_attributes or jo_3d_object_attributes)
  * @remarks You can pass any structure (even an array) if you respect the order of attributes
  */
static  __jo_force_inline void              jo_storyboard_add_object(jo_storyboard *const storyboard, const void * const object)
{
    jo_storyboard_object_cache              *tmp = (jo_storyboard_object_cache *)jo_malloc_with_behaviour(sizeof(*tmp), JO_MALLOC_TRY_REUSE_SAME_BLOCK_SIZE);

    tmp->user = (jo_3d_object_attributes *)object;
    tmp->dx = JO_MULT_BY_8(tmp->user->x);
    tmp->dy = JO_MULT_BY_8(tmp->user->y);
    JO_ZERO(tmp->current_cos);
    JO_ZERO(tmp->current_sin);
    jo_list_add_ptr(&storyboard->objects, tmp);
}

/** @brief Remove object from storyboard
  * @param storyboard Storyboard
  * @param object Pointer to an object (should be the same passed in jo_storyboard_add_object())
  */
static  __jo_force_inline void                  jo_storyboard_remove_object(jo_storyboard *const storyboard, const void * const object)
{
    jo_node *tmp;
    for (tmp = storyboard->objects.first; tmp != JO_NULL; tmp = tmp->next)
        if (((jo_storyboard_object_cache *)tmp->data.ptr)->user == object)
        {
            jo_list_remove(&storyboard->objects, tmp);
            return ;
        }
}

/*
** HELPER
*/

/** @brief Get object storyboard cache
  * @param storyboard Storyboard
  * @param object Pointer to an object (should be the same passed in jo_storyboard_add_object())
  * @return Object storyboard cache
  */
static  __jo_force_inline jo_storyboard_object_cache        *jo_storyboard_get_object_cache(jo_storyboard *const storyboard, const void * const object)
{
    jo_node *tmp;
    for (tmp = storyboard->objects.first; tmp != JO_NULL; tmp = tmp->next)
        if (((jo_storyboard_object_cache *)tmp->data.ptr)->user == object)
            return ((jo_storyboard_object_cache *)tmp->data.ptr);
    return (JO_NULL);
}

/** @brief Reset object position
  * @param storyboard Storyboard
  * @param object Pointer to an object (should be the same passed in jo_storyboard_add_object())
  * @remarks Must be called if you move the object manually (x, y, z)
  */
static  __jo_force_inline void                  jo_storyboard_reset_object_position(jo_storyboard * const storyboard, const void * const object)
{
    jo_storyboard_object_cache                  *cache = jo_storyboard_get_object_cache(storyboard, object);

    if (cache == JO_NULL)
        return;
    cache->dx = JO_MULT_BY_8(cache->user->x);
    cache->dy = JO_MULT_BY_8(cache->user->y);
}

/** @brief Create a new storyboard for the object
  * @param autoplay Play the storyboard immediately
  * @param repeat Play the storyboard in loop (each animation must have a limited duration)
  * @param object Pointer to an object (jo_pos2D, jo_pos3D, jo_2d_object_attributes or jo_3d_object_attributes)
  * @remarks You can pass any structure (even an array) if you respect the order of attributes
  * @return The storyboard
  */
static  __jo_force_inline jo_storyboard         *jo_storyboard_create_for_object(const bool autoplay, const bool repeat, const void * const object)
{
    jo_storyboard                               *storyboard = jo_storyboard_create(autoplay, repeat);

    jo_storyboard_add_object(storyboard, object);
    return storyboard;
}

/*
** QUICK ANIMATION FACTORY
*/

/** @brief Create a rotation animation
  * @param storyboard Storyboard
  * @param speed Rotation speed
  * @param duration Duration of animation (you can pass JO_STORYBOARD_INFINITE_DURATION for infinite)
  * @remarks If speed = 1 and duration = 90, the object will make a smooth clockwise rotation of 90°
  * @remarks If speed is positive, the rotation is clockwise otherwise counterclockwise
  * @return The animation
  */
static  __jo_force_inline jo_animation          *jo_storyboard_create_rotation_animation(jo_storyboard * const storyboard, const char speed, const unsigned int duration)
{
    jo_animation                                *animation = jo_storyboard_create_animation(storyboard, 0, duration);

    animation->rotation_speed = speed;
    return animation;
}

/** @brief Create a translation animation
  * @param storyboard Storyboard
  * @param angle Vector direction
  * @param speed Translation speed
  * @param duration Duration of animation (you can pass JO_STORYBOARD_INFINITE_DURATION for infinite)
  * @return The animation
  */
static  __jo_force_inline jo_animation          *jo_storyboard_create_translation_animation(jo_storyboard *const storyboard, const short angle, const char speed, const unsigned int duration)
{
    jo_animation                                *animation = jo_storyboard_create_animation(storyboard, 0, duration);

    animation->direction = angle;
    animation->translation_speed_x = speed;
    animation->translation_speed_y = speed;
    return (animation);
}

/** @brief Create a translation animation using jo_8_directions enum
  * @param storyboard Storyboard
  * @param direction Vector direction
  * @param speed Translation speed
  * @param duration Duration of animation (you can pass JO_STORYBOARD_INFINITE_DURATION for infinite)
  * @return The animation
  */
static  __jo_force_inline jo_animation          *jo_storyboard_create_translation_animation_using_direction(jo_storyboard * const storyboard, const jo_8_directions direction, const char speed, const unsigned int duration)
{
    return (jo_storyboard_create_translation_animation(storyboard, jo_direction_to_angle(direction), speed, duration));
}

/** @brief Create a circle rotation animation
  * @param storyboard Storyboard
  * @param radius Circle radius
  * @param speed Translation speed
  * @param duration Duration of animation (you can pass JO_STORYBOARD_INFINITE_DURATION for infinite)
  * @return The animation
  */
static  __jo_force_inline jo_animation          *jo_storyboard_create_circle_animation(jo_storyboard * const storyboard, const short radius, const char speed, const unsigned int duration)
{
    jo_animation                                *animation = jo_storyboard_create_animation(storyboard, 0, duration);

    animation->sin_radius = radius;
    animation->cos_radius = radius;
    animation->translation_speed_x = speed;
    animation->translation_speed_y = speed;
    return (animation);
}

/*
** QUICK STORYBOARD FACTORY FOR OBJECT
*/

/** @brief Create a rotation animation storyboard for a specific object
  * @param object Pointer to an object (jo_pos2D, jo_pos3D, jo_2d_object_attributes or jo_3d_object_attributes)
  * @param speed Rotation speed
  * @param duration Duration of animation (you can pass JO_STORYBOARD_INFINITE_DURATION for infinite)
  * @remarks If speed = 1 and duration = 90, the object will make a smooth clockwise rotation of 90°
  * @remarks If speed is positive, the rotation is clockwise otherwise counterclockwise
  * @return The storyboard
  */
static  __jo_force_inline jo_storyboard         *jo_storyboard_rotate_object(const void * const object, const char speed, const unsigned int duration)
{
    jo_storyboard                               *storyboard = jo_storyboard_create_for_object(true, false, object);

    jo_storyboard_create_rotation_animation(storyboard, speed, duration);
    return (storyboard);
}

/** @brief Create a translation animation storyboard for a specific object
  * @param object Pointer to an object (jo_pos2D, jo_pos3D, jo_2d_object_attributes or jo_3d_object_attributes)
  * @param angle Vector direction
  * @param speed Rotation speed
  * @param duration Duration of animation (you can pass JO_STORYBOARD_INFINITE_DURATION for infinite)
  * @return The storyboard
  */
static  __jo_force_inline jo_storyboard         *jo_storyboard_move_object(const void * const object, const short angle, const char speed, const unsigned int duration)
{
    jo_storyboard                               *storyboard = jo_storyboard_create_for_object(true, false, object);

    jo_storyboard_create_translation_animation(storyboard, angle, speed, duration);
    return (storyboard);
}

/** @brief Create a translation animation storyboard for a specific object using jo_8_directions enum
  * @param object Pointer to an object (jo_pos2D, jo_pos3D, jo_2d_object_attributes or jo_3d_object_attributes)
  * @param direction Vector direction
  * @param speed Rotation speed
  * @param duration Duration of animation (you can pass JO_STORYBOARD_INFINITE_DURATION for infinite)
  * @return The storyboard
  */
static  __jo_force_inline jo_storyboard         *jo_storyboard_move_object_in_direction(const void * const object, const jo_8_directions direction, const char speed, const unsigned int duration)
{
    jo_storyboard                               *storyboard = jo_storyboard_create_for_object(true, false, object);

    jo_storyboard_create_translation_animation_using_direction(storyboard, direction, speed, duration);
    return (storyboard);
}

/** @brief Create a circle rotation storyboard for a specific object
  * @param object Pointer to an object (jo_pos2D, jo_pos3D, jo_2d_object_attributes or jo_3d_object_attributes)
  * @param radius Circle radius
  * @param speed Translation speed
  * @param duration Duration of animation (you can pass JO_STORYBOARD_INFINITE_DURATION for infinite)
  * @return The storyboard
  */
static  __jo_force_inline jo_storyboard         *jo_storyboard_move_object_in_circle(const void * const object, const short radius, const char speed, const unsigned int duration)
{
    jo_storyboard                               *storyboard = jo_storyboard_create_for_object(true, false, object);

    jo_storyboard_create_circle_animation(storyboard, radius, speed, duration);
    return storyboard;
}

#endif /* !JO_COMPILE_WITH_STORYBOARD_SUPPORT */

#endif /* !__JO_STORYBOARD_H__ */

/*
** END OF FILE
*/
