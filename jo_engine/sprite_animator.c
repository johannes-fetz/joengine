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
#include "jo/math.h"
#include "jo/sprite_animator.h"

jo_sprite_anim		__jo_sprite_anim_tab[JO_MAX_SPRITE_ANIM];
static int			__jo_sprite_anim_id = -1;
static char			__jo_sprite_anim_callback_event_id = 0;

static void			__jo_internal_frame_animator(void)
{
    register int	i;

    for (JO_ZERO(i); i <= __jo_sprite_anim_id; ++i)
    {
        if (__jo_sprite_anim_tab[i].action == JO_SPRITE_ANIM_STOP)
            continue;
        if (__jo_sprite_anim_tab[i].frame_skip >= __jo_sprite_anim_tab[i].frame_rate)
        {
            JO_ZERO(__jo_sprite_anim_tab[i].frame_skip);
            if ((__jo_sprite_anim_tab[i].cur_frame + 1) >= __jo_sprite_anim_tab[i].frame_count)
            {
                if (__jo_sprite_anim_tab[i].action == JO_SPRITE_ANIM_STOP_AT_LAST_FRAME)
                    __jo_sprite_anim_tab[i].action = JO_SPRITE_ANIM_STOP;
                else
                    JO_ZERO(__jo_sprite_anim_tab[i].cur_frame);
            }
            else
                ++__jo_sprite_anim_tab[i].cur_frame;
        }
        else
            ++__jo_sprite_anim_tab[i].frame_skip;
    }
}

int			jo_replace_sprite_anim(const int at, const unsigned short sprite_id, const unsigned short frame_count, const unsigned char frame_rate)
{
#ifdef JO_DEBUG
    if (at >= __jo_sprite_anim_id)
    {
        jo_core_error("at (%d) is too high (max=%d)", (int)at, (int)__jo_sprite_anim_id);
        return (-1);
    }
#endif
    __jo_sprite_anim_tab[at].action = JO_SPRITE_ANIM_STOP;
    JO_ZERO(__jo_sprite_anim_tab[at].frame_skip);
    JO_ZERO(__jo_sprite_anim_tab[at].cur_frame);
    __jo_sprite_anim_tab[at].frame_count = frame_count;
    __jo_sprite_anim_tab[at].frame0_sprite_id = sprite_id;
    __jo_sprite_anim_tab[at].frame_rate = frame_rate;
    if (!__jo_sprite_anim_callback_event_id)
    {
        __jo_sprite_anim_callback_event_id = jo_core_add_callback(__jo_internal_frame_animator);
        if (!__jo_sprite_anim_callback_event_id)
        {
#ifdef JO_DEBUG
            jo_core_error("Event creation failed");
#endif
            return (-1);
        }
    }
    return (at);
}

int			jo_create_sprite_anim(const unsigned short sprite_id, const unsigned short frame_count, const unsigned char frame_rate)
{
    ++__jo_sprite_anim_id;
#ifdef JO_DEBUG
    if (__jo_sprite_anim_id >= JO_MAX_SPRITE_ANIM)
    {
        --__jo_sprite_anim_id;
        jo_core_error("Too many animation");
        return (-1);
    }
#endif
    __jo_sprite_anim_tab[__jo_sprite_anim_id].action = JO_SPRITE_ANIM_STOP;
    JO_ZERO(__jo_sprite_anim_tab[__jo_sprite_anim_id].frame_skip);
    JO_ZERO(__jo_sprite_anim_tab[__jo_sprite_anim_id].cur_frame);
    __jo_sprite_anim_tab[__jo_sprite_anim_id].frame_count = frame_count;
    __jo_sprite_anim_tab[__jo_sprite_anim_id].frame0_sprite_id = sprite_id;
    __jo_sprite_anim_tab[__jo_sprite_anim_id].frame_rate = frame_rate;
    if (!__jo_sprite_anim_callback_event_id)
    {
        __jo_sprite_anim_callback_event_id = jo_core_add_callback(__jo_internal_frame_animator);
        if (!__jo_sprite_anim_callback_event_id)
        {
            --__jo_sprite_anim_id;
#ifdef JO_DEBUG
            jo_core_error("Event creation failed");
#endif
            return (-1);
        }
    }
    return (__jo_sprite_anim_id);
}

void        jo_clear_all_sprite_anim(void)
{
    __jo_sprite_anim_id = -1;
    jo_core_remove_callback(__jo_sprite_anim_callback_event_id);
    JO_ZERO(__jo_sprite_anim_callback_event_id);
}

/*
** END OF FILE
*/
