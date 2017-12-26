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
#include "jo/fs.h"
#include "jo/image.h"
#include "jo/sprites.h"
#include "jo/background.h"
#include "jo/math.h"

void                            jo_draw_background_line(int x0, int y0, int x1, int y1, const jo_color color)
{
    int                         dx;
    int                         sx;
    int                         dy;
    int                         sy;
    int                         err;
    int                         e2;

    dx = JO_ABS(x1 - x0);
    sx = x0 < x1 ? 1 : -1;
    dy = JO_ABS(y1 - y0);
    sy = y0 < y1 ? 1 : -1;
    if (dx == dy)
        JO_ZERO(err);
    else
        err = JO_DIV_BY_2(dx > dy ? dx : -dy);
    for (;;)
    {
        jo_put_pixel_in_background(x0, y0, color);
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

void                            jo_clear_background(const jo_color color)
{
    register jo_color           *vram_ptr;
    register int                i;
    jo_color                    buf[JO_VDP2_WIDTH];

    for (JO_ZERO(i); i < JO_VDP2_WIDTH; ++i)
        buf[i] = color;
    vram_ptr = (jo_color *)VDP2_VRAM_A0;
    for (JO_ZERO(i); i < JO_VDP2_HEIGHT; ++i)
    {
        jo_dma_copy(buf, vram_ptr, JO_VDP2_WIDTH * sizeof(color));
        vram_ptr += JO_VDP2_WIDTH;
    }
}

void			                jo_set_background_sprite(const jo_img *const img, const unsigned short left, const unsigned short top)
{
    register int                y;
    register jo_color           *vram_ptr;
    register jo_color           *img_ptr;

    if (top)
        vram_ptr = ((jo_color *)VDP2_VRAM_A0) + (JO_VDP2_WIDTH * top);
    else
        vram_ptr = (jo_color *)VDP2_VRAM_A0;
    img_ptr = img->data;
    for (JO_ZERO(y); y < img->height; ++y)
    {
        if (left)
            vram_ptr += left;
        jo_dma_copy(img_ptr, vram_ptr, img->width * sizeof(*img_ptr));
        if (left)
            vram_ptr += JO_VDP2_WIDTH - left;
        else
            vram_ptr += JO_VDP2_WIDTH;
        img_ptr += img->width;
    }
}

/*
** END OF FILE
*/
