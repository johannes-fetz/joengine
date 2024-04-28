/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2024, Johannes Fetz (johannesfetz@gmail.com)
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
#include "jo/colors.h"
#include "jo/sprites.h"
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

#endif /* !JO_COMPILE_WITH_EFFECTS_SUPPORT */

/*
** END OF FILE
*/
