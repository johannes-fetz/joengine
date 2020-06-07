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
#include "jo/malloc.h"
#include "jo/fs.h"
#include "jo/image.h"
#include "jo/sprites.h"
#include "jo/vdp2.h"
#include "jo/math.h"
#include "jo/palette.h"
#include "jo/vdp2_malloc.h"

typedef struct
{
    unsigned char               *ptr;
    unsigned int                size;
    bool                        is_free;
}                               jo_vdp2_memory_segment;

// CRAM
# define CRAM_PALETTE_SIZE      (256)
int                             __jo_current_palette_id = 0;
static jo_color                 *__jo_cram = ((jo_color *)JO_VDP2_CRAM) + CRAM_PALETTE_SIZE + 1; // The first palette is reserved
// A0
# define A0_SEGMENT_COUNT       (6)
static unsigned char            *__jo_a0 = (unsigned char *)VDP2_VRAM_A0;
static jo_vdp2_memory_segment   __jo_a0_segments[6];
// A1
# define KTABLE_SIZE                    (0x1FE00)
static const unsigned char      *__jo_rbg0_k_table = (unsigned char *)VDP2_VRAM_A1;
# define ROTATION_TABLE_SIZE            (0x00080)
static const unsigned char      *__jo_rbg0_r_table = (unsigned char *)(VDP2_VRAM_A1 + KTABLE_SIZE);
//# define REMAING_SPACE_IN_A1            (0x1FC)
//static unsigned char            *__jo_remaing_spaceA1 = (unsigned char *)(VDP2_VRAM_A1 + KTABLE_SIZE + ROTATION_TABLE_SIZE);
static const unsigned char      *__jo_back_color = (unsigned char *)(VDP2_VRAM_A1 + 0x1fffe); // The two last bytes in A1 Bank is for background color
// B0
# define B0_SEGMENT_COUNT       (6)
static unsigned char            *__jo_b0 = (unsigned char *)VDP2_VRAM_B0;
static jo_vdp2_memory_segment   __jo_b0_segments[6];
// B1
# define B1_SEGMENT_COUNT       (12)
static unsigned char            *__jo_b1 = (unsigned char *)VDP2_VRAM_B1;
static jo_vdp2_memory_segment   __jo_b1_segments[12];

void                            jo_vdp2_malloc_init(void)
{
    jo_memset(__jo_a0_segments, 0, sizeof(__jo_a0_segments));
    jo_memset(__jo_b0_segments, 0, sizeof(__jo_b0_segments));
    jo_memset(__jo_b1_segments, 0, sizeof(__jo_b1_segments));
}

inline void                     jo_vdp2_free(const void * const p)
{
    register unsigned int       i;

    for (JO_ZERO(i); i < A0_SEGMENT_COUNT; ++i)
    {
        if (__jo_a0_segments[i].ptr == p)
        {
            __jo_a0_segments[i].is_free = true;
            if ((__jo_a0_segments[i].ptr + __jo_a0_segments[i].size) == __jo_a0)
            {
                __jo_a0 -= __jo_a0_segments[i].size;
                JO_ZERO(__jo_a0_segments[i].size);
            }
            return;
        }
    }
    for (JO_ZERO(i); i < B0_SEGMENT_COUNT; ++i)
    {
        if (__jo_b0_segments[i].ptr == p)
        {
            __jo_b0_segments[i].is_free = true;
            if ((__jo_b0_segments[i].ptr + __jo_b0_segments[i].size) == __jo_b0)
            {
                __jo_b0 -= __jo_b0_segments[i].size;
               JO_ZERO(__jo_b0_segments[i].size);
            }
            return;
        }
    }
    for (JO_ZERO(i); i < B1_SEGMENT_COUNT; ++i)
    {
        if (__jo_b1_segments[i].ptr == p)
        {
            __jo_b1_segments[i].is_free = true;
            if ((__jo_b1_segments[i].ptr + __jo_b1_segments[i].size) == __jo_b1)
            {
                __jo_b1 -= __jo_b1_segments[i].size;
                JO_ZERO(__jo_b1_segments[i].size);
            }
            return;
        }
    }
}

void                            *jo_vdp2_malloc(const jo_vdp2_ram_usage usage, const unsigned int n)
{
    register unsigned int       i;
    void                        *ptr;

    switch (usage)
    {
        case JO_VDP2_RAM_BACK_COLOR:
            return ((void*)__jo_back_color);
        case JO_VDP2_RAM_RTABLE:
            return ((void*)__jo_rbg0_r_table);
        case JO_VDP2_RAM_KTABLE:
            return ((void*)__jo_rbg0_k_table);
        case JO_VDP2_RAM_MAP2:
        case JO_VDP2_RAM_CELL2:
            for (JO_ZERO(i); i < B1_SEGMENT_COUNT; ++i)
            {
                if (__jo_b1_segments[i].size == 0)
                {
                    __jo_b1_segments[i].ptr = __jo_b1;
                    __jo_b1 += n;
                    __jo_b1_segments[i].size = n;
                    __jo_b1_segments[i].is_free = false;
                    return (__jo_b1_segments[i].ptr);
                }
                if (__jo_b1_segments[i].is_free && __jo_b1_segments[i].size >= n)
                {
                    __jo_b1_segments[i].is_free = false;
                    return (__jo_b1_segments[i].ptr);
                }
            }
            jo_core_error("Out of VDP2 memory");
            return (JO_NULL);
        case JO_VDP2_RAM_MAP:
            for (JO_ZERO(i); i < B0_SEGMENT_COUNT; ++i)
            {
                if (__jo_b0_segments[i].size == 0)
                {
                    __jo_b0_segments[i].ptr = __jo_b0;
                    __jo_b0 += n;
                    __jo_b0_segments[i].size = n;
                    __jo_b0_segments[i].is_free = false;
                    return (__jo_b0_segments[i].ptr);
                }
                if (__jo_b0_segments[i].is_free && __jo_b0_segments[i].size >= n)
                {
                    __jo_b0_segments[i].is_free = false;
                    return (__jo_b0_segments[i].ptr);
                }
            }
            jo_core_error("Out of VDP2 memory");
            return (JO_NULL);
        case JO_VDP2_RAM_BITMAP:
        case JO_VDP2_RAM_LINE_SCROLL:
        case JO_VDP2_RAM_CELL:
            for (JO_ZERO(i); i < A0_SEGMENT_COUNT; ++i)
            {
                if (__jo_a0_segments[i].size == 0)
                {
                    __jo_a0_segments[i].ptr = __jo_a0;
                    __jo_a0 += n;
                    __jo_a0_segments[i].size = n;
                    __jo_a0_segments[i].is_free = false;
                    return (__jo_a0_segments[i].ptr);
                }
                if (__jo_a0_segments[i].is_free && __jo_a0_segments[i].size >= n)
                {
                    __jo_a0_segments[i].is_free = false;
                    return (__jo_a0_segments[i].ptr);
                }
            }
            jo_core_error("Out of VDP2 memory");
            return (JO_NULL);
        case JO_VDP2_RAM_COLOR:
#ifdef JO_DEBUG
            if ((__jo_current_palette_id + 1) >= JO_PALETTE_MAX)
            {
                jo_core_error("Out of VDP2 memory");
                return (JO_NULL);
            }
#endif
            ++__jo_current_palette_id;
            ptr = __jo_cram;
            __jo_cram += CRAM_PALETTE_SIZE;
            return (ptr);
        default:
            jo_core_error("Unsupported usage %d", usage);
            return (JO_NULL);
    }
}
