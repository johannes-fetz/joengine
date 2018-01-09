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
/** @file malloc.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Malloc
 *  @bug No known bugs.
 */

#ifndef __JO_MALLOC_H__
# define __JO_MALLOC_H__

/** @brief Extended RAM cartridge type */
typedef enum
{
    /** @brief No cartridge */
    CART_NONE,
    /** @brief 1 Mb cartridge */
    CART_8MBits,
    /** @brief 4 Mb cartridge */
    CART_32MBits
}               jo_extended_ram_cartridge_type;

/** @brief Malloc behaviour
 * JO_MALLOC_TRY_REUSE_SAME_BLOCK_SIZE
 *     |          ^
 *     |          |
 *   Speed    efficiency
 *     |          |
 *     V          |
 * JO_MALLOC_TRY_REUSE_BLOCK
 *     |          ^
 *     |          |
 *   Speed    efficiency
 *     |          |
 *     V          |
 *   JO_FAST_ALLOCATION
 */
typedef enum
{
    /** @brief Default behaviour */
    JO_FAST_ALLOCATION,
    /** @brief A little bit slower but reduce memory fragmentation */
    JO_MALLOC_TRY_REUSE_BLOCK,
    /** @brief A little bit slower than JO_MALLOC_TRY_REUSE_BLOCK but reduce memory usage */
    JO_MALLOC_TRY_REUSE_SAME_BLOCK_SIZE
}       jo_malloc_behaviour;

/** @brief dynamic memory allocator with specific behaviour
 *  @param n Segment size
 *  @param behaviour Allocation Behaviour
 *  @return a pointer to the allocated memory or NULL if failed
 */
void	        *jo_malloc_with_behaviour(unsigned int n, const jo_malloc_behaviour behaviour);

/** @brief dynamic memory allocator
 *  @param n Segment size
 *  @return a pointer to the allocated memory or NULL if failed
 */
static  __jo_force_inline void	*jo_malloc(unsigned int n)
{
    return jo_malloc_with_behaviour(n, JO_FAST_ALLOCATION);
}

/** @brief function frees the memory space pointed to by ptr, which must have been returned by a previous call to jo_malloc()
 */
void	        jo_free(const void * const p);

#ifdef JO_COMPILE_WITH_RAM_CARD_SUPPORT

/** @brief Get the extended RAM cartridge type if available
 *  @return extended RAM cartridge type
 */
static  __jo_force_inline jo_extended_ram_cartridge_type      jo_get_extended_ram_cartridge_type(void)
{
    return *((char *)0x24FFFFFF) == 0x5a ? CART_8MBits : *((char *)0x24FFFFFF) == 0x5c ? CART_32MBits : CART_NONE;
}

#endif

#ifdef JO_DEBUG

/** @brief Get memory usage percent
 *  @return memory usage percent
 *  @remarks jo_printf(0, 0, "Dynamic memory usage: %d%%  ", jo_memory_usage_percent());
 */
int             jo_memory_usage_percent(void);

/** @brief Get memory fragmentation (Number of fragment in memory)
 *  @return Memory fragmentation
 *  @remarks jo_printf(0, 0, "Dynamic memory fragmentation: %d%%  ", jo_memory_fragmentation());
 */
int             jo_memory_fragmentation(void);

#endif

#endif /* !__JO_MALLOC_H__ */

/*
** END OF FILE
*/
