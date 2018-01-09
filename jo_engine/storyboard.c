/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2017, Johannes Fetz (johannesfetz@gmail.com)
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
#include "jo/tools.h"
#include "jo/malloc.h"
#include "jo/math.h"
#include "jo/list.h"
#include "jo/storyboard.h"

#ifdef JO_COMPILE_WITH_STORYBOARD_SUPPORT

jo_list                             __storyboards;

void                                jo_init_storyboards(void)
{
    jo_list_init(&__storyboards);
    jo_list_set_allocation_behaviour(&__storyboards, JO_MALLOC_TRY_REUSE_SAME_BLOCK_SIZE);
}

jo_storyboard                       *jo_storyboard_create(const bool autoplay, const bool repeat)
{
    jo_storyboard                   *storyboard  = (jo_storyboard *)jo_malloc_with_behaviour(sizeof(*storyboard), JO_MALLOC_TRY_REUSE_SAME_BLOCK_SIZE);

    storyboard->pause = !autoplay;
    storyboard->repeat = repeat;
    jo_list_init(&storyboard->objects);
    jo_list_init(&storyboard->animations);
    jo_list_add_ptr(&__storyboards, storyboard);
    return storyboard;
}

void                                jo_storyboard_destroy(jo_storyboard *storyboard)
{
    jo_list_free_and_clear(&storyboard->objects);
    jo_list_free_and_clear(&storyboard->animations);
    jo_list_remove_first_value(&__storyboards, (jo_list_data)(void *)storyboard);
}

static void                         animate_object(jo_animation *animation, jo_storyboard_object_cache *object)
{
    if (animation->rotation_speed)
        object->user->rz += animation->rotation_speed;
    if (animation->cos_radius)
    {
        object->user->x = jo_cos_mult(animation->cos_radius, object->current_cos);
        if (object->dx)
            object->user->x += JO_DIV_BY_8(object->dx);
        object->current_cos += animation->translation_speed_x;
    }
    else if (animation->translation_speed_x)
    {
        if (animation->direction == 0)
            object->user->x += animation->translation_speed_x;
        else if (animation->direction == 180)
            object->user->x -= animation->translation_speed_x;
        else if (animation->direction != 90 && animation->direction != 270)
        {
            object->dx += jo_cos_mult(2 + animation->translation_speed_x, animation->direction);
            object->user->x = JO_DIV_BY_8(object->dx);
        }
    }
    if (animation->sin_radius)
    {
        object->user->y = jo_sin_mult(animation->sin_radius, object->current_sin);
        if (object->dy)
            object->user->y += JO_DIV_BY_8(object->dy);
        object->current_sin += animation->translation_speed_y;
    }
    else if (animation->translation_speed_y)
    {
        if (animation->direction == 90)
            object->user->y += animation->translation_speed_y;
        else if (animation->direction == 270)
            object->user->y -= animation->translation_speed_y;
        else if (animation->direction != 0 && animation->direction != 180)
        {
            object->dy += jo_sin_mult(2 + animation->translation_speed_y, animation->direction);
            object->user->y = JO_DIV_BY_8(object->dy);
        }
    }
}

static void                         jo_reset_storyboard(jo_storyboard *storyboard)
{
    jo_node                         *tmp;

    for (tmp = storyboard->animations.first; tmp != JO_NULL; tmp = tmp->next)
        ((jo_animation *)tmp->data.ptr)->disabled = false;
}

static void                         do_storyboard(jo_storyboard *storyboard)
{
    jo_node                         *animation_node;
    jo_node                         *object_node;
    jo_animation                    *animation;

    for (animation_node = storyboard->animations.first; animation_node != JO_NULL; animation_node = animation_node->next)
    {
        animation = (jo_animation *)animation_node->data.ptr;
        if (animation->disabled)
            continue;
        if (animation->frame >= animation->frame_skip)
        {
            JO_ZERO(animation->frame);
            for (object_node = storyboard->objects.first; object_node != JO_NULL; object_node = object_node->next)
                animate_object(animation, (jo_storyboard_object_cache *)object_node->data.ptr);
            if (animation->limit)
            {
                ++animation->count;
                if (animation->count >= animation->limit)
                {
                    JO_ZERO(animation->count);
                    // TODO RESET
                    animation->disabled = true;
                }
            }
        }
        else
            ++animation->frame;
        return ;
    }
    if (storyboard->repeat)
        jo_reset_storyboard(storyboard);
}

void                                jo_execute_storyboards(void)
{
    jo_node                         *storyboard_node;
    jo_storyboard                   *storyboard;

    for (storyboard_node = __storyboards.first; storyboard_node != JO_NULL; storyboard_node = storyboard_node->next)
    {
        storyboard = (jo_storyboard *)storyboard_node->data.ptr;
        if (!storyboard->pause && storyboard->objects.count)
            do_storyboard(storyboard);
    }
}

jo_animation                        *jo_storyboard_create_animation(jo_storyboard *storyboard,
                                                                    const unsigned char frame_skip,
                                                                    const unsigned int duration)
{
    jo_animation                    *tmp = (jo_animation *)jo_malloc_with_behaviour(sizeof(*tmp), JO_MALLOC_TRY_REUSE_SAME_BLOCK_SIZE);

    jo_memset(tmp, 0, sizeof(*tmp));
    tmp->frame_skip = frame_skip;
    tmp->limit = duration;
    jo_list_add_ptr(&storyboard->animations, tmp);
    return tmp;
}

#endif /* !JO_COMPILE_WITH_STORYBOARD_SUPPORT */

/*
** END OF FILE
*/
