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
#include "jo/sprites.h"
#include "jo/effects.h"

#ifdef JO_COMPILE_WITH_EFFECTS_SUPPORT

void        jo_effect_laser(int x0, int y0, int x1, int y1, int pertubation, int sprite_id)
{
    int                         dx;
    int                         sx;
    int                         dy;
    int                         sy;
    int                         err;
    int                         e2;

    x0 -= JO_DIV_BY_2(pertubation);
    y0 -= JO_DIV_BY_2(pertubation);
    x1 -= JO_DIV_BY_2(pertubation);
    y1 -= JO_DIV_BY_2(pertubation);
    dx = JO_ABS(x1 - x0);
    sx = x0 < x1 ? 1 : -1;
    dy = JO_ABS(y1 - y0);
    sy = y0 < y1 ? 1 : -1;
    err = JO_DIV_BY_2(dx > dy ? dx : -dy);
    while (42)
    {
        if (pertubation > 1)
            jo_sprite_draw3D(sprite_id, x0 + jo_random(pertubation), y0 + jo_random(pertubation), 600);
        else
            jo_sprite_draw3D(sprite_id, x0, y0, 600);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = err;
        if (e2 > -dx)
        {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy)
        {
            err += dx;
            y0 += sy;
        }
    }
}

#endif /* !JO_COMPILE_WITH_EFFECTS_SUPPORT */

/*
** END OF FILE
*/
