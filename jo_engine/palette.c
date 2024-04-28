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
#include "jo/fs.h"
#include "jo/image.h"
#include "jo/math.h"
#include "jo/colors.h"
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
    output->id = __jo_current_palette_id;
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
