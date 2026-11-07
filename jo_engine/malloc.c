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

static jo_memory_zone   memory_zones[JO_MALLOC_MAX_MEMORY_ZONE];

void                    jo_add_memory_zone(unsigned char *ptr, const unsigned int size_in_bytes)
{
#ifdef JO_DEBUG
    if (__jo_memory_zone_index >= JO_MALLOC_MAX_MEMORY_ZONE)
    {
        jo_core_error("Too many memory zone");
        return ;
    }
#endif
    memory_zones[__jo_memory_zone_index].begin = ptr;
    memory_zones[__jo_memory_zone_index].high = ptr;
    memory_zones[__jo_memory_zone_index].end = ptr + size_in_bytes;
    jo_memset(ptr, 0, size_in_bytes);
    ++__jo_memory_zone_index;
}

/** @brief Merge a free block with the next free blocks of its zone
 *  @param zone Memory zone
 *  @param block Free block
 *  @return false if the block was the last block of the zone (it is given back to the zone)
 */
static bool             jo_malloc_merge_free_blocks(jo_memory_zone * const zone, jo_memory_block * const block)
{
    jo_memory_block     *next;

    next = (jo_memory_block *)(((unsigned char *)block) + block->size);
    while ((unsigned char *)next < zone->high && next->zone == JO_BLOCK_FREE)
    {
        block->size += next->size;
        next = (jo_memory_block *)(((unsigned char *)block) + block->size);
    }
    if ((unsigned char *)next >= zone->high)
    {
        zone->high = (unsigned char *)block;
        return (false);
    }
    return (true);
}

/** @brief Find a free block in a zone
 *  @param zone Zone index
 *  @param n Block size (header included, multiple of 4)
 *  @param same_size Only reuse a block with the same size
 *  @return The block or JO_NULL
 *  @remarks The adjacent free blocks are merged and a bigger block is split
 */
static jo_memory_block  *jo_malloc_find_free_block(const int zone, const unsigned int n, const bool same_size)
{
    jo_memory_block     *block;
    jo_memory_block     *remaining;

    block = (jo_memory_block *)memory_zones[zone].begin;
    while ((unsigned char *)block < memory_zones[zone].high)
    {
#ifdef JO_DEBUG
        if (block->size == 0 || JO_MOD_POW2(block->size, 4) != 0)
        {
            jo_core_error("Memory corrupt: Block size is null");
            return (JO_NULL);
        }
#endif
        if (block->zone == JO_BLOCK_FREE)
        {
            if (!jo_malloc_merge_free_blocks(&memory_zones[zone], block))
                return (JO_NULL);
            if (block->size == n || (!same_size && block->size >= n))
            {
                // Split the block if the remaining part can hold another block
                if (block->size >= n + sizeof(*block) + sizeof(int))
                {
                    remaining = (jo_memory_block *)(((unsigned char *)block) + n);
                    remaining->zone = JO_BLOCK_FREE;
                    remaining->size = block->size - n;
                    block->size = n;
                }
                block->zone = zone;
                return (block);
            }
        }
        block = (jo_memory_block *)(((unsigned char *)block) + block->size);
    }
    return (JO_NULL);
}

/** @brief Allocate a block at the end of the used area of a zone
 *  @param zone Zone index
 *  @param n Block size (header included, multiple of 4)
 *  @return The block or JO_NULL if the zone is full
 */
static jo_memory_block  *jo_malloc_new_block(const int zone, const unsigned int n)
{
    jo_memory_block     *block;

    block = (jo_memory_block *)memory_zones[zone].high;
    // (the room left, not block + n: a huge n (jo_malloc(-100000)) wrapped the address around, the block was accepted
    // and the top of the zone went back over the blocks in use)
    if (n > (unsigned int)(memory_zones[zone].end - (unsigned char *)block))
        return (JO_NULL);
    memory_zones[zone].high += n;
    block->zone = zone;
    block->size = n;
    return (block);
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
    // (a huge size, a negative number given as a size: n + the header wrapped around, a block smaller than its header)
    if (n > 0xFFFFFFFFu - sizeof(*block) - 3)
        return (JO_NULL);
    n += sizeof(*block);
    while (JO_MOD_POW2(n, 4))
        ++n;
    for (JO_ZERO(zone); zone < __jo_memory_zone_index; ++zone)
    {
        // Try to reuse a freed block first
        if (behaviour != JO_FAST_ALLOCATION &&
            (block = jo_malloc_find_free_block(zone, n, behaviour == JO_MALLOC_TRY_REUSE_SAME_BLOCK_SIZE)) != JO_NULL)
            return (block + 1);
        // New block at the end of the used area
        if ((block = jo_malloc_new_block(zone, n)) != JO_NULL)
            return (block + 1);
        // The zone is full: any freed block is fine
        if ((block = jo_malloc_find_free_block(zone, n, false)) != JO_NULL)
            return (block + 1);
        // The last freed blocks may have been given back to the zone by jo_malloc_find_free_block()
        if ((block = jo_malloc_new_block(zone, n)) != JO_NULL)
            return (block + 1);
    }
    return (JO_NULL);
}

void                            jo_reduce_memory_fragmentation(void)
{
    register int                zone;
    unsigned char               *ptr;
    jo_memory_block             *block;
    jo_memory_block             *last_free_block;

    for (JO_ZERO(zone); zone < __jo_memory_zone_index; ++zone)
    {
        ptr = memory_zones[zone].begin;
        last_free_block = JO_NULL;
        while (ptr < memory_zones[zone].high)
        {
            block = (jo_memory_block *)ptr;
            if (block->zone != JO_BLOCK_FREE)
                last_free_block = JO_NULL;
            if (block->zone == JO_BLOCK_FREE && last_free_block == JO_NULL)
                last_free_block = block;
            ptr += block->size;
        }
        if (last_free_block != JO_NULL)
            memory_zones[zone].high = (unsigned char *)last_free_block;
    }
}

inline void                     jo_free(const void * const p)
{
    jo_memory_block             *block;
    jo_memory_zone              *zone;

#ifdef JO_DEBUG
    if (p == JO_NULL)
    {
        jo_core_error("Null pointer");
        return ;
    }
#endif
    block = ((jo_memory_block *)p) - 1;
#ifdef JO_DEBUG
    if (block->size == 0 || JO_MOD_POW2(block->size, 4) != 0)
    {
        jo_core_error("Bad pointer: %x", (unsigned int)p);
        return ;
    }
    if (block->zone == JO_BLOCK_FREE)
    {
        jo_core_error("Pointer already freed: %x", (unsigned int)p);
        return ;
    }
#endif
    zone = &memory_zones[block->zone];
    block->zone = JO_BLOCK_FREE;
    // Merged with the next free blocks, or given back to the zone if it's the last block
    jo_malloc_merge_free_blocks(zone, block);
}

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
                if (block->size == 0 || JO_MOD_POW2(block->size, 4) != 0)
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
            if (block->size == 0 || JO_MOD_POW2(block->size, 4) != 0)
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

/*
** END OF FILE
*/
