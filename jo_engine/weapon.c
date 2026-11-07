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
** Weapons of the player: see jo/weapon.h
*/

#include <stdbool.h>
#include "jo/conf.h"

#ifdef JO_COMPILE_WITH_LEVEL_SUPPORT

#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/core.h"
#include "jo/tools.h"
#include "jo/malloc.h"
#include "jo/math.h"
#include "jo/level.h"
#include "jo/weapon.h"

static jo_weapon_def                __jo_weapon_defs[JO_WEAPON_MAX];
static int                          __jo_weapon_count = 0;
static char                         *__jo_weapon_strings = JO_NULL;
static unsigned int                 __jo_weapon_strings_size = 0;
// State: the magazines, the reserves of the types of ammunition, the current weapon
static int                          __jo_weapon_ammo[JO_WEAPON_MAX];
static int                          __jo_weapon_reserve[JO_WEAPON_MAX_AMMO_TYPES];
static int                          __jo_weapon_current = 0;
static int                          __jo_weapon_next = -1;      // The weapon taken after the current one went down
static int                          __jo_weapon_switch_timer = 0;
static int                          __jo_weapon_switch_time = 0;
static bool                         __jo_weapon_lowering = false;
static int                          __jo_weapon_cooldown = 0;
static int                          __jo_weapon_reload_timer = 0;
static unsigned int                 __jo_weapon_last_vblank = 0;
static bool                         __jo_weapon_started = false;

/*
** The times of the definitions are in frames of the game (JO_FRAMERATE VBLANK each); the timers count VBLANKs: when a
** frame takes longer (the frame rate goes down) they go down by the VBLANKs it took. An automatic weapon keeps its rate
** of fire (the time left over goes to the next shot), the reload its length (the shots and the sounds no longer stutter)
*/
/** @brief A time of a definition (frames of the game) in VBLANKs */
# define JO_WEAPON_VBLANKS(FRAMES)          ((FRAMES) * JO_FRAMERATE)
/** @brief The longest time counted between 2 updates (a pause, a loading): VBLANKs */
# define JO_WEAPON_MAX_STEP                 (4 * JO_FRAMERATE)

static const char                   *__jo_weapon_string(const unsigned int offset)
{
    if (__jo_weapon_strings == JO_NULL || offset >= __jo_weapon_strings_size)
        return ("");
    return (__jo_weapon_strings + offset);
}

int                                 jo_weapons_load(jo_level * const level)
{
    const jo_level_weap_record      *r;
    const jo_level_directory_entry  *strs;
    jo_weapon_def                   *d;
    unsigned int                    count;
    unsigned int                    k;

    if (__jo_weapon_strings != JO_NULL)
        jo_free(__jo_weapon_strings);
    __jo_weapon_strings = JO_NULL;
    __jo_weapon_strings_size = 0;
    // The strings of the level stay after it is freed (the names of the files)
    strs = jo_level_find_section(level, JO_LEVEL_SECTION_STRS);
    if (strs != JO_NULL)
    {
        __jo_weapon_strings = (char *)jo_malloc(strs->size);
        if (__jo_weapon_strings != JO_NULL)
        {
            jo_memcpy(__jo_weapon_strings, jo_level_section_data(level, JO_LEVEL_SECTION_STRS), strs->size);
            __jo_weapon_strings_size = strs->size;
        }
    }
    r = JO_LEVEL_RECORDS(level, WEAP, weap, &count);
    count = JO_MIN(count, JO_WEAPON_MAX);
    jo_memset(__jo_weapon_reserve, 0, sizeof(__jo_weapon_reserve));
    for (k = 0, d = __jo_weapon_defs; k < count; ++k, ++d, ++r)
    {
        d->name = __jo_weapon_string(r->name);
        d->model = __jo_weapon_string(r->model);
        d->flash = __jo_weapon_string(r->flash);
        d->sound_shot = __jo_weapon_string(r->sound_shot);
        d->sound_reload = __jo_weapon_string(r->sound_reload);
        d->x = r->x;
        d->y = r->y;
        d->z = r->z;
        d->muzzle_y = r->muzzle_y;
        d->muzzle_z = r->muzzle_z;
        d->convergence = JO_MAX(r->convergence, 1);
        d->automatic = r->automatic;
        d->fire_delay = r->fire_delay;
        d->damage = r->damage;
        d->spread = r->spread;
        d->recoil = r->recoil;
        d->kick = r->kick;
        d->magazine = r->magazine;
        d->ammo_type = JO_MIN(r->ammo_type, JO_WEAPON_MAX_AMMO_TYPES - 1);
        d->ammo_start = r->ammo_start;
        d->ammo_max = r->ammo_max;
        d->reload_time = JO_MAX(r->reload_time, 2);
        d->switch_time = JO_MAX(r->switch_time, 2);
        d->flash_size = r->flash_size;
        d->casing = r->casing;
        d->shot_volume = r->shot_volume;
        // Full magazines, the reserve of the type: the biggest one of its weapons
        __jo_weapon_ammo[k] = d->magazine;
        __jo_weapon_reserve[d->ammo_type] = JO_MAX(__jo_weapon_reserve[d->ammo_type], d->ammo_start);
    }
    __jo_weapon_count = (int)count;
    __jo_weapon_current = 0;
    __jo_weapon_next = -1;
    __jo_weapon_switch_timer = 0;
    __jo_weapon_cooldown = 0;
    __jo_weapon_reload_timer = 0;
    __jo_weapon_started = false;
    return (__jo_weapon_count);
}

int                                 jo_weapons_count(void)
{
    return (__jo_weapon_count);
}

const jo_weapon_def                 *jo_weapons_def(const int index)
{
    return (index >= 0 && index < __jo_weapon_count ? &__jo_weapon_defs[index] : JO_NULL);
}

int                                 jo_weapons_current(void)
{
    return (__jo_weapon_current);
}

int                                 jo_weapons_ammo(void)
{
    return (__jo_weapon_count > 0 ? __jo_weapon_ammo[__jo_weapon_current] : 0);
}

int                                 jo_weapons_reserve(void)
{
    return (__jo_weapon_count > 0 ? __jo_weapon_reserve[__jo_weapon_defs[__jo_weapon_current].ammo_type] : 0);
}

int                                 jo_weapons_reload_progress(void)
{
    const jo_weapon_def             *d;

    if (__jo_weapon_reload_timer <= 0 || __jo_weapon_count == 0)
        return (0);
    d = &__jo_weapon_defs[__jo_weapon_current];
    return (256 - __jo_weapon_reload_timer * 255 / JO_WEAPON_VBLANKS(d->reload_time));
}

int                                 jo_weapons_lowered(void)
{
    if (__jo_weapon_switch_timer <= 0)
        return (0);
    // Down: from 0 to 256, up: from 256 to 0
    if (__jo_weapon_lowering)
        return (256 - __jo_weapon_switch_timer * 256 / __jo_weapon_switch_time);
    return (__jo_weapon_switch_timer * 256 / __jo_weapon_switch_time);
}

void                                jo_weapons_give_ammo(const int ammo_type, const int count)
{
    int                             k;
    int                             max;
    int                             reserve;

    if (ammo_type < 0 || ammo_type >= JO_WEAPON_MAX_AMMO_TYPES)
        return;
    max = 0;
    for (k = 0; k < __jo_weapon_count; ++k)
        if (__jo_weapon_defs[k].ammo_type == ammo_type)
            max = JO_MAX(max, __jo_weapon_defs[k].ammo_max);
    // (a reserve over the most (an ammo_start over ammo_max) is not cut by a pickup; reserve + count overflowed with a big
    // count (all the ammunition): the reserve became negative)
    reserve = __jo_weapon_reserve[ammo_type];
    if (count > 0)
    {
        if (reserve < max)
            __jo_weapon_reserve[ammo_type] = count >= max - reserve ? max : reserve + count;
    }
    else if (count < 0)
        __jo_weapon_reserve[ammo_type] = count <= -reserve ? 0 : reserve + count;
}

void                                jo_weapons_select(const int index)
{
    if (index < 0 || index >= __jo_weapon_count || index == __jo_weapon_current || __jo_weapon_switch_timer > 0)
        return;
    __jo_weapon_next = index;
    __jo_weapon_lowering = true;
    __jo_weapon_switch_time = JO_WEAPON_VBLANKS(JO_MAX(__jo_weapon_defs[__jo_weapon_current].switch_time / 2, 1));
    __jo_weapon_switch_timer = __jo_weapon_switch_time;
    __jo_weapon_reload_timer = 0;
}

unsigned int                        jo_weapons_update(const bool fire_held, const bool fire_pressed, const bool reload_pressed,
                                                      const bool switch_pressed)
{
    const jo_weapon_def             *d;
    unsigned int                    events;
    unsigned int                    now;
    int                             *reserve;
    int                             step;
    int                             before;
    int                             n;

    if (__jo_weapon_count == 0)
        return (0);
    // The VBLANKs since the last update (a frame of the game: JO_FRAMERATE, more when the frame rate goes down)
    now = jo_core_get_vblank_count();
    step = __jo_weapon_started ? (int)(now - __jo_weapon_last_vblank) : JO_FRAMERATE;
    step = JO_MAX(1, JO_MIN(step, JO_WEAPON_MAX_STEP));
    __jo_weapon_last_vblank = now;
    __jo_weapon_started = true;
    events = 0;
    d = &__jo_weapon_defs[__jo_weapon_current];
    reserve = &__jo_weapon_reserve[d->ammo_type];
    // (below 0: the time left over since the shot was possible, given to the next shot while the trigger is held)
    __jo_weapon_cooldown -= step;
    if (__jo_weapon_cooldown < -step)
        __jo_weapon_cooldown = -step;
    // Change of weapon: the current one goes down (half of its time), the next one comes up (half of its time). A reload
    // is stopped
    if (__jo_weapon_switch_timer > 0)
    {
        __jo_weapon_switch_timer -= step;
        if (__jo_weapon_switch_timer > 0)
            return (0);
        __jo_weapon_switch_timer = 0;
        if (!__jo_weapon_lowering)
            return (0);
        __jo_weapon_current = __jo_weapon_next;
        __jo_weapon_lowering = false;
        __jo_weapon_switch_time = JO_WEAPON_VBLANKS(JO_MAX(__jo_weapon_defs[__jo_weapon_current].switch_time / 2, 1));
        __jo_weapon_switch_timer = __jo_weapon_switch_time;
        return (JO_WEAPON_EVENT_SWITCH);
    }
    if (switch_pressed && __jo_weapon_count > 1)
    {
        jo_weapons_select((__jo_weapon_current + 1) % __jo_weapon_count);
        return (0);
    }
    // Reload
    if (__jo_weapon_reload_timer > 0)
    {
        before = __jo_weapon_reload_timer;
        __jo_weapon_reload_timer = JO_MAX(0, __jo_weapon_reload_timer - step);
        if (before > JO_WEAPON_VBLANKS(d->reload_time) / 2 && __jo_weapon_reload_timer <= JO_WEAPON_VBLANKS(d->reload_time) / 2)
            events |= JO_WEAPON_EVENT_RELOAD_HALF;
        if (__jo_weapon_reload_timer == 0)
        {
            n = JO_MIN(d->magazine - __jo_weapon_ammo[__jo_weapon_current], *reserve);
            __jo_weapon_ammo[__jo_weapon_current] += n;
            *reserve -= n;
            events |= JO_WEAPON_EVENT_RELOAD_DONE;
        }
        return (events);
    }
    // Fire: held for an automatic weapon, one shot per press for the others
    if ((d->automatic ? fire_held : fire_pressed) && __jo_weapon_cooldown <= 0)
    {
        if (__jo_weapon_ammo[__jo_weapon_current] > 0)
        {
            --__jo_weapon_ammo[__jo_weapon_current];
            __jo_weapon_cooldown += JO_WEAPON_VBLANKS(d->fire_delay);
            events |= JO_WEAPON_EVENT_FIRE;
        }
        else if (*reserve > 0)
        {
            __jo_weapon_reload_timer = JO_WEAPON_VBLANKS(d->reload_time);
            events |= JO_WEAPON_EVENT_RELOAD;
        }
        else if (fire_pressed)
            events |= JO_WEAPON_EVENT_EMPTY;
    }
    // (the time left over is only for the next shot of a burst)
    if (!fire_held && __jo_weapon_cooldown < 0)
        __jo_weapon_cooldown = 0;
    if (reload_pressed && __jo_weapon_reload_timer == 0 && __jo_weapon_ammo[__jo_weapon_current] < d->magazine && *reserve > 0)
    {
        __jo_weapon_reload_timer = JO_WEAPON_VBLANKS(d->reload_time);
        events |= JO_WEAPON_EVENT_RELOAD;
    }
    return (events);
}

#endif /* !JO_COMPILE_WITH_LEVEL_SUPPORT */

/*
** END OF FILE
*/
