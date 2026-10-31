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
** INCLUDES
*/
#include <stdbool.h>
#include "jo/conf.h"
#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/core.h"
#include "jo/tools.h"
#include "jo/malloc.h"
#include "jo/math.h"
#include "jo/colors.h"
#include "jo/vdp1_command_pipeline.h"
#include "jo/sprites.h"
#include "jo/fs.h"
#include "jo/list.h"
#include "jo/image.h"
#include "jo/tga.h"
#include "jo/3d.h"
#include "jo/effects.h"

#ifdef JO_COMPILE_WITH_EFFECTS_SUPPORT

void                    jo_init_homing_laser(jo_homing_laser * const homing_laser, const int sprite_id, const unsigned short thickness, const int z, const unsigned short nb_sections)
{
#ifdef JO_DEBUG
    if (thickness < 1)
    {
        jo_core_error("thickness < 1");
        return ;
    }
    if (nb_sections < 4)
    {
        jo_core_error("nb_sections < 4");
        return ;
    }
    if (nb_sections > 64)
    {
        jo_core_error("nb_sections > 64");
        return ;
    }
    if (JO_MOD_POW2(nb_sections, 2) != 0)
    {
        jo_core_error("nb_sections must be a multiple of 2");
        return ;
    }
#endif

    JO_ZERO(homing_laser->source.x);
    JO_ZERO(homing_laser->source.y);
    homing_laser->source.z = jo_int2fixed(z);

    JO_ZERO(homing_laser->intermediate_point_a.x);
    JO_ZERO(homing_laser->intermediate_point_a.y);
    homing_laser->intermediate_point_a.z = homing_laser->source.z;
    homing_laser->use_intermediate_point_a = true;

    JO_ZERO(homing_laser->intermediate_point_b.x);
    JO_ZERO(homing_laser->intermediate_point_b.y);
    homing_laser->intermediate_point_b.z = homing_laser->source.z;
    homing_laser->use_intermediate_point_b = true;

    JO_ZERO(homing_laser->target.x);
    JO_ZERO(homing_laser->target.y);
    homing_laser->target.z = homing_laser->source.z;

    JO_ZERO(homing_laser->length);
    homing_laser->sprite_id = sprite_id;
    homing_laser->horizontal_thickness = jo_int2fixed(thickness);
    JO_ZERO(homing_laser->vertical_thickness);
    homing_laser->section_step = JO_FIXED_1 / nb_sections;
    homing_laser->center_sprites_according_to_thickness = true;
}

void                    jo_draw_homing_laser(jo_homing_laser * const homing_laser)
{
    jo_pos2D_fixed      sp[4];
    jo_fixed            t;
    jo_fixed            horizontal_thickness_div2;
    jo_fixed            vertical_thickness_div2;

    if (homing_laser->length <= 0)
        return ;
    sp[0].x = homing_laser->source.x;
    sp[0].y = homing_laser->source.y;
    sp[1].x = homing_laser->source.x;
    sp[1].y = homing_laser->source.y;
    horizontal_thickness_div2 = JO_DIV_BY_2(homing_laser->horizontal_thickness);
    vertical_thickness_div2 = JO_DIV_BY_2(homing_laser->vertical_thickness);
    for (JO_ZERO(t); t <= homing_laser->length && t <= JO_FIXED_1; t += homing_laser->section_step)
    {
        jo_vector_fixed_compute_bezier_point(t,
                                             homing_laser->source,
                                             homing_laser->use_intermediate_point_a ? homing_laser->intermediate_point_a : homing_laser->source,
                                             homing_laser->use_intermediate_point_b ? homing_laser->intermediate_point_b : homing_laser->target,
                                             homing_laser->target,
                                             (jo_vector_fixed *)&sp[2]/* Dirty optimization ^^ */);

        sp[3].x = sp[2].x + homing_laser->horizontal_thickness;
        sp[3].y = sp[2].y + homing_laser->vertical_thickness;

        if (homing_laser->center_sprites_according_to_thickness)
        {
            sp[2].x -= horizontal_thickness_div2;
            sp[3].x -= horizontal_thickness_div2;
            sp[2].y -= vertical_thickness_div2;
            sp[3].y -= vertical_thickness_div2;
        }

        jo_sprite_draw_4p_fixed(homing_laser->sprite_id, sp, homing_laser->source.z, true);

        JO_SWAP(sp[0].x, sp[3].x);
        JO_SWAP(sp[0].y, sp[3].y);
        JO_SWAP(sp[1].x, sp[2].x);
        JO_SWAP(sp[1].y, sp[2].y);
    }
}

#ifdef JO_COMPILE_WITH_3D_SUPPORT

/*
** Effects of the 3D world (see effects.h): fire, smoke, flash of the screen, shake of the camera
*/

typedef struct
{
    const jo_effect_def             *def;           // JO_NULL: free
    jo_pos3D_fixed                  position;
    short                           age;            // Frames (< 0: not started)
    bool                            flip;
}                                   __jo_effect;

static __jo_effect                  __jo_effects[JO_EFFECT_MAX];
static int                          __jo_effect_next = 0;
static unsigned int                 __jo_effect_seed = 0x2545f491;
// Flash: color offset of the VDP2 (written during the VBLANK)
static int                          __jo_flash_color[3];
static int                          __jo_flash_frames = 0;
static int                          __jo_flash_age = 0;
static volatile short               __jo_flash_value[3];
static volatile bool                __jo_flash_dirty = false;
static bool                         __jo_flash_on = false;
static int                          __jo_flash_event = -1;
// Fade to black (0: none, 255: black), subtracted from the flash
static int                          __jo_fade = 0;
static int                          __jo_fade_target = 0;
static int                          __jo_fade_step = 0;
static void                         __jo_effect_update_offset(void);
// Shake
static int                          __jo_shake_amount = 0;
static int                          __jo_shake_frames = 0;
static int                          __jo_shake_age = 0;
static jo_pos3D_fixed               __jo_shake_offset;

static int                          __jo_effect_random(const int range)
{
    __jo_effect_seed = __jo_effect_seed * 1103515245 + 12345;
    return ((int)((__jo_effect_seed >> 16) & 0x7fff) % range);
}

void                                jo_effect_add(const jo_effect_def * const def, const jo_pos3D_fixed * const position, const int delay)
{
    __jo_effect                     *e;

    e = &__jo_effects[__jo_effect_next];
    __jo_effect_next = (__jo_effect_next + 1) % JO_EFFECT_MAX;
    e->def = def;
    e->position = *position;
    e->age = (short)-delay;
    e->flip = (def->flags & JO_EFFECT_RANDOM_FLIP) && __jo_effect_random(2) == 0;
}

/** @brief The color offset of the flash: written during the VBLANK (see jo_effect_flash()) */
static void                         __jo_effect_flash_vblank(void)
{
    if (!__jo_flash_dirty)
        return;
    __jo_flash_dirty = false;
    if (__jo_flash_value[0] == 0 && __jo_flash_value[1] == 0 && __jo_flash_value[2] == 0)
    {
        jo_disable_screen_color_filter(JO_ALL_SCROLL_SCREEN);
        __jo_flash_on = false;
    }
    else
    {
        jo_set_screen_color_filter_a(JO_ALL_SCROLL_SCREEN, __jo_flash_value[0], __jo_flash_value[1], __jo_flash_value[2]);
        __jo_flash_on = true;
    }
}

void                                jo_effects_clear(void)
{
    int                             k;

    for (k = 0; k < JO_EFFECT_MAX; ++k)
        __jo_effects[k].def = JO_NULL;
    __jo_shake_frames = 0;
    __jo_shake_offset.x = 0;
    __jo_shake_offset.y = 0;
    __jo_shake_offset.z = 0;
    // (the fade stays: the screen stays black while a level is loaded)
    if (__jo_flash_frames > 0 || __jo_flash_on)
    {
        __jo_flash_frames = 0;
        __jo_effect_update_offset();
    }
}

/** @brief Color offset of the flash and of the fade (written during the next VBLANK) */
static void                         __jo_effect_update_offset(void)
{
    int                             k;
    int                             flash;

    for (k = 0; k < 3; ++k)
    {
        flash = __jo_flash_frames > 0 ? __jo_flash_color[k] * (__jo_flash_frames - __jo_flash_age) / __jo_flash_frames : 0;
        __jo_flash_value[k] = (short)JO_MAX(flash - __jo_fade, -255);
    }
    __jo_flash_dirty = true;
    if (__jo_flash_event < 0)
        __jo_flash_event = jo_core_add_vblank_callback(__jo_effect_flash_vblank);
}

void                                jo_effect_fade(const int darkness, const int frames)
{
    __jo_fade_target = JO_MIN(JO_MAX(darkness, 0), 255);
    if (frames <= 0)
    {
        __jo_fade = __jo_fade_target;
        __jo_effect_update_offset();
        return;
    }
    __jo_fade_step = JO_MAX(JO_ABS(__jo_fade_target - __jo_fade) / frames, 1);
}

bool                                jo_effect_is_fading(void)
{
    return (__jo_fade != __jo_fade_target);
}

void                                jo_effect_flash(const int r, const int g, const int b, const int frames)
{
    int                             left;

    // A brighter flash than what is left of the current one replaces it
    left = __jo_flash_frames > 0 ? (__jo_flash_color[0] + __jo_flash_color[1] + __jo_flash_color[2]) *
           (__jo_flash_frames - __jo_flash_age) / __jo_flash_frames : 0;
    if (frames <= 0 || r + g + b < left)
        return;
    __jo_flash_color[0] = JO_MIN(r, 255);
    __jo_flash_color[1] = JO_MIN(g, 255);
    __jo_flash_color[2] = JO_MIN(b, 255);
    __jo_flash_frames = frames;
    __jo_flash_age = 0;
    if (__jo_flash_event < 0)
        __jo_flash_event = jo_core_add_vblank_callback(__jo_effect_flash_vblank);
}

void                                jo_effect_shake(const int amount, const int frames)
{
    int                             left;

    left = __jo_shake_frames > 0 ? __jo_shake_amount * (__jo_shake_frames - __jo_shake_age) / __jo_shake_frames : 0;
    if (frames <= 0 || amount < left)
        return;
    __jo_shake_amount = amount;
    __jo_shake_frames = frames;
    __jo_shake_age = 0;
}

void                                jo_effect_get_shake(jo_pos3D_fixed * const offset)
{
    *offset = __jo_shake_offset;
}

void                                jo_effects_update(void)
{
    int                             k;
    int                             a;
    __jo_effect                     *e;

    for (k = 0, e = __jo_effects; k < JO_EFFECT_MAX; ++k, ++e)
    {
        if (e->def == JO_NULL)
            continue;
        if (++e->age >= e->def->life)
            e->def = JO_NULL;
        else if (e->age > 0)
            e->position.y -= e->def->rise << 12;
    }
    // Flash: fades linearly
    if (__jo_flash_frames > 0)
    {
        ++__jo_flash_age;
        if (__jo_flash_age >= __jo_flash_frames)
            __jo_flash_frames = 0;
        __jo_effect_update_offset();
    }
    // Fade to black, or back
    if (__jo_fade != __jo_fade_target)
    {
        __jo_fade = __jo_fade < __jo_fade_target ? JO_MIN(__jo_fade + __jo_fade_step, __jo_fade_target) :
                    JO_MAX(__jo_fade - __jo_fade_step, __jo_fade_target);
        __jo_effect_update_offset();
    }
    // Shake: a random offset that gets smaller
    if (__jo_shake_frames > 0)
    {
        ++__jo_shake_age;
        if (__jo_shake_age >= __jo_shake_frames)
        {
            __jo_shake_frames = 0;
            __jo_shake_offset.x = 0;
            __jo_shake_offset.y = 0;
            __jo_shake_offset.z = 0;
        }
        else
        {
            a = __jo_shake_amount * (__jo_shake_frames - __jo_shake_age) * 2 / __jo_shake_frames + 1;
            __jo_shake_offset.x = jo_int2fixed(__jo_effect_random(a) - a / 2);
            __jo_shake_offset.y = jo_int2fixed(__jo_effect_random(a) - a / 2);
            __jo_shake_offset.z = jo_int2fixed(__jo_effect_random(a) - a / 2);
        }
    }
}

int                                 jo_effects_draw(void)
{
    int                             k;
    int                             frame;
    int                             count;
    jo_fixed                        size;
    jo_pos3D_fixed                  foot;
    const jo_effect_def             *def;
    __jo_effect                     *e;

    count = 0;
    for (k = 0, e = __jo_effects; k < JO_EFFECT_MAX; ++k, ++e)
    {
        def = e->def;
        if (def == JO_NULL || e->age < 0)
            continue;
        frame = e->age * def->frame_count / def->life;
        size = jo_int2fixed(def->size_start) + jo_int2fixed(def->size_end - def->size_start) * e->age / def->life;
        foot = e->position;
        if (!(def->flags & JO_EFFECT_STANDING))
            foot.y += size;
        if (def->flags & JO_EFFECT_HALF_TRANSPARENT)
            jo_sprite_enable_half_transparency();
        if (jo_3d_draw_standing_sprite(def->sprites[frame], &foot, size * 2, size * 2, 0, e->flip))
            ++count;
        if (def->flags & JO_EFFECT_HALF_TRANSPARENT)
            jo_sprite_disable_half_transparency();
    }
    return (count);
}

#endif /* !JO_COMPILE_WITH_3D_SUPPORT */

#endif /* !JO_COMPILE_WITH_EFFECTS_SUPPORT */

/*
** END OF FILE
*/
