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
#include "jo/tools.h"
#include "jo/math.h"
#include "jo/malloc.h"
#include "jo/smpc.h"
#include "jo/core.h"

#define JO_BLOCK_FREE           (~0)

typedef struct
{
    short           zone;
    unsigned int    size;
}                   jo_memory_block;

static int __jo_memory_zone_index = 0;

typedef struct
{
    unsigned char       *begin;
    unsigned char       *high;
    unsigned char       *end;
}                       jo_memory_zone;

static jo_memory_zone   memory_zones[9/*MAIN RAM + 32MB RAM Extension*/];

void                jo_add_memory_zone(unsigned char *ptr, const unsigned int size_in_bytes)
{
    register int    *tmp;
    register int    *end;

    memory_zones[__jo_memory_zone_index].begin = ptr;
    memory_zones[__jo_memory_zone_index].high = ptr;
    memory_zones[__jo_memory_zone_index].end = ptr + size_in_bytes;

    // Clear memory 4 bytes at the same time
    end = (int *)memory_zones[__jo_memory_zone_index].end;
    for (tmp = (int *)ptr; tmp < end; ++tmp)
        JO_ZERO(*tmp);
    ++__jo_memory_zone_index;
}

void	                *jo_malloc_with_behaviour(unsigned int n, const jo_malloc_behaviour behaviour)
{
    register int        zone;
    jo_memory_block     *block;

#ifdef JO_DEBUG
    if (n == 0)
    {
        jo_core_error("n must be greater than zero");
        return (JO_NULL);
    }
#endif
    n += sizeof(*block);
    while (n % 4)
        ++n;
    for (JO_ZERO(zone); zone < __jo_memory_zone_index; ++zone)
    {
        if (behaviour == JO_FAST_ALLOCATION)
        {
malloc_new_segment:
            block = (jo_memory_block *)memory_zones[zone].high;
            if (((unsigned char *)block + n) <= memory_zones[zone].end)
            {
                memory_zones[zone].high += n;
                goto malloc_new_block;
            }
            if (behaviour == JO_MALLOC_TRY_REUSE_BLOCK)
                continue;
        }
        block = (jo_memory_block *)memory_zones[zone].begin;
        while (((unsigned char *)block + n) <= memory_zones[zone].high)
        {
            if (block->zone == JO_BLOCK_FREE)
            {
#ifdef JO_DEBUG
                if (block->size == 0 || (block->size % 4) != 0)
                {
                    jo_core_error("Memory corrupt: Block size is null");
                    goto malloc_new_block;
                }
#endif
                if (behaviour == JO_MALLOC_TRY_REUSE_SAME_BLOCK_SIZE)
                {
                    if (block->size == n)
                    {
                        block->zone = zone;
                        return (block + 1);
                    }
                }
                else if (block->size >= n)
                {
                    block->zone = zone;
                    return (block + 1);
                }
            }
            block = (jo_memory_block *)(((unsigned char *)block) + block->size);
        }
        if (behaviour != JO_FAST_ALLOCATION)
            goto malloc_new_segment;
    }
    return JO_NULL;
malloc_new_block:
    block->zone = zone;
    block->size = n;
    return (block + 1);
}

inline void                    jo_free(const void * const p)
{
    jo_memory_block             *block;

#ifdef JO_DEBUG
    if (p == JO_NULL)
    {
        jo_core_error("Null pointer");
        return ;
    }
#endif
    block = ((jo_memory_block *)p) - 1;
#ifdef JO_DEBUG
    if (block->size == 0 || (block->size % 4) != 0)
    {
        jo_core_error("Bad pointer: %x", (unsigned int)p);
        return ;
    }
#endif
    if ((((unsigned char *)block) + block->size) == memory_zones[block->zone].high)
        memory_zones[block->zone].high = (unsigned char *)block;
    else
        block->zone = JO_BLOCK_FREE;
}

#ifdef JO_DEBUG

int                     jo_memory_usage_percent(void)
{
    register int        zone;
    unsigned int        mem_total;
    unsigned int        mem_free;
    jo_memory_block     *block;

    JO_ZERO(mem_total);
    JO_ZERO(mem_free);
    for (JO_ZERO(zone); zone < __jo_memory_zone_index; ++zone)
    {
        mem_total += memory_zones[zone].end - memory_zones[zone].begin;
        if (memory_zones[zone].high < memory_zones[zone].end)
            mem_free += memory_zones[zone].end - memory_zones[zone].high;
        block = (jo_memory_block *)memory_zones[zone].begin;
        while ((unsigned char *)block < memory_zones[zone].high)
        {
            if (block->zone == JO_BLOCK_FREE)
            {
#ifdef JO_DEBUG
                if (block->size == 0 || (block->size % 4) != 0)
                {
                    jo_core_error("Memory corrupt: Block size is null");
                    break;
                }
#endif
                mem_free += block->size;
            }
            block = (jo_memory_block *)(((unsigned char *)block) + block->size);
        }
    }
    return (JO_PERCENT_USED(mem_total, mem_free));
}

int                     jo_memory_fragmentation(void)
{
    register int        zone;
    unsigned int        fragment;
    jo_memory_block     *block;

    JO_ZERO(fragment);
    for (JO_ZERO(zone); zone < __jo_memory_zone_index; ++zone)
    {
        block = (jo_memory_block *)memory_zones[zone].begin;
        while ((unsigned char *)block < memory_zones[zone].high)
        {
#ifdef JO_DEBUG
            if (block->size == 0 || (block->size % 4) != 0)
            {
                jo_core_error("Memory corrupt: Block size is null");
                break;
            }
#endif
            block = (jo_memory_block *)(((unsigned char *)block) + block->size);
            ++fragment;
        }
    }
    return fragment;
}

#endif

/*
** END OF FILE
*/
