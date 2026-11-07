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
#include "jo/fs.h"
#include "jo/image.h"
#include "jo/math.h"
#include "jo/colors.h"
#include "jo/vdp1_command_pipeline.h"
#include "jo/sprites.h"
#include "jo/palette.h"
#include "jo/vdp2_malloc.h"

extern int          __jo_current_palette_id;

void                jo_create_palette(jo_palette * const output)
{
#ifdef JO_DEBUG
    if (output == JO_NULL)
    {
        jo_core_error("output is null");
        return ;
    }
#endif
    output->data = jo_vdp2_malloc_autosize(JO_VDP2_RAM_COLOR);
    // (the color RAM full: no id, the sprites used another palette)
    output->id = output->data != JO_NULL ? __jo_current_palette_id : -1;
}

void        jo_set_palette_register(const unsigned short palette_index, const jo_color color)
{
#ifdef JO_DEBUG
    if (palette_index < 1)
    {
        jo_core_error("palette_index too low");
        return ;
    }
    if (palette_index >= 2048)
    {
        jo_core_error("palette_index too high");
        return ;
    }
#endif
    *(((jo_color *)(JO_VDP2_CRAM)) + palette_index) = color;
}

jo_color    jo_get_palette_register(const unsigned short palette_index)
{
    return *(((jo_color *)(JO_VDP2_CRAM)) + palette_index);
}

/*
** END OF FILE
*/

void                jo_palette_set_light_shades(jo_palette * const palette, const int first, const jo_color * const colors,
                                                const int count, const int levels)
{
    int             s;
    int             i;
    int             index;
    int             level;
    int             r;
    int             g;
    int             b;

    for (JO_ZERO(s); s < levels; ++s)
    {
        // Light level of the shade (0 to 31, 16 = the color itself): from 35 % of the color to 125 %. The shade levels / 2
        // is the level 16 (the light level 16 of jo_3d_apply_light() takes it): the color itself
        level = s * 32 / levels;
        level = level < 16 ? 90 + level * 166 / 16 : 256 + (level - 16) * 64 / 16;
        for (JO_ZERO(i); i < count; ++i)
        {
            index = first + s * count + i;
            if (index < 1 || index > JO_PALETTE_MAX_COLORS)
                continue;
            r = (JO_COLOR_SATURN_GET_R(colors[i]) * level) >> 8;
            g = (JO_COLOR_SATURN_GET_G(colors[i]) * level) >> 8;
            b = (JO_COLOR_SATURN_GET_B(colors[i]) * level) >> 8;
            palette->data[index - 1] = JO_COLOR_SATURN_RGB(r < 0 ? 0 : r > 31 ? 31 : r, g < 0 ? 0 : g > 31 ? 31 : g,
                                                           b < 0 ? 0 : b > 31 ? 31 : b);
        }
    }
}
