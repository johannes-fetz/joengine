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
#include "jo/vdp2.h"
#include "jo/palette.h"
#include "jo/vdp2_malloc.h"
#include "internal/vdp2_malloc.h"

typedef struct
{
    unsigned char               *ptr;           // Start of the block
    unsigned int                size;           // Size of the block
    unsigned char               *data;          // Returned by jo_vdp2_malloc() (aligned in the block for a map)
    bool                        is_free;
}                               jo_vdp2_memory_segment;

// CRAM
# define CRAM_PALETTE_SIZE      (256)
int                             __jo_current_palette_id = 0;
// The first palette is reserved. The data of a palette starts at the second color of its block of 256: the index 0 of a
// pixel is transparent, the pixel k shows the color data[k - 1] (the TGA files: their index + 1, see jo_tga_get_pixel())
static jo_color                 *__jo_cram = ((jo_color *)JO_VDP2_CRAM) + CRAM_PALETTE_SIZE + 1;
// A0
# define A0_SEGMENT_COUNT       (6)
static unsigned char            *__jo_a0 = (unsigned char *)JO_VDP2_VRAM_A0;
static jo_vdp2_memory_segment   __jo_a0_segments[6];
// A1
# define KTABLE_SIZE            (0x1FE00)
static const unsigned char      *__jo_rbg0_k_table = (unsigned char *)JO_VDP2_VRAM_A1;
# define ROTATION_TABLE_SIZE    (0x00080)
static const unsigned char      *__jo_rbg0_r_table = (unsigned char *)(JO_VDP2_VRAM_A1 + KTABLE_SIZE);
//# define REMAING_SPACE_IN_A1  (0x1FC)
//static unsigned char          *__jo_remaing_spaceA1 = (unsigned char *)(JO_VDP2_VRAM_A1 + KTABLE_SIZE + ROTATION_TABLE_SIZE);
static const unsigned char      *__jo_back_color = (unsigned char *)(JO_VDP2_VRAM_A1 + 0x1fffe); // The two last bytes in A1 Bank is for background color
// B0
# define B0_SEGMENT_COUNT       (6)
static unsigned char            *__jo_b0 = (unsigned char *)JO_VDP2_VRAM_B0;
#ifdef JO_DEBUG
/** @brief No error message when the memory is full (__jo_vdp2_malloc_try()) */
static bool                     __jo_vdp2_quiet = false;
#endif
static jo_vdp2_memory_segment   __jo_b0_segments[6];
// B1
# define B1_SEGMENT_COUNT       (12)
#ifdef JO_COMPILE_WITH_PRINTF_SUPPORT
/** @brief The font and the map of jo_printf() stay in the bank B1: the blocks start after the font and skip the map */
# define JO_VDP2_B1_START               ((unsigned char *)(JO_VDP2_VRAM + JO_VDP2_PRINTF_CELL_OFFSET + JO_VDP2_PRINTF_CELL_SIZE))
# define JO_VDP2_B1_HOLE_START          ((unsigned char *)(JO_VDP2_VRAM + JO_VDP2_PRINTF_MAP_OFFSET))
# define JO_VDP2_B1_HOLE_END            (JO_VDP2_B1_HOLE_START + JO_VDP2_MAP_SIZE)
#else
# define JO_VDP2_B1_START               ((unsigned char *)JO_VDP2_VRAM_B1)
#endif
static unsigned char            *__jo_b1 = JO_VDP2_B1_START;
static jo_vdp2_memory_segment   __jo_b1_segments[12];

void                            jo_vdp2_malloc_init(void)
{
    jo_memset(__jo_a0_segments, 0, sizeof(__jo_a0_segments));
    jo_memset(__jo_b0_segments, 0, sizeof(__jo_b0_segments));
    jo_memset(__jo_b1_segments, 0, sizeof(__jo_b1_segments));
}

bool                            __jo_vdp2_a0_start_is_free(const unsigned int n)
{
    // jo_vdp2_malloc() returns the start of the bank A0 for the first segment, when it is unused or free and large enough
    if (__jo_a0_segments[0].size == 0)
        return (__jo_a0 == (unsigned char *)JO_VDP2_VRAM_A0);
    return (__jo_a0_segments[0].is_free && __jo_a0_segments[0].size >= n);
}

/** @brief After a free: the top of a bank goes down over the free blocks at its top (freed in any order: only the last
 *  block went back before, the bank leaked)
 *  @param segments Blocks of the bank
 *  @param count Blocks count
 *  @param top The end of the used part of the bank
 */
static void                     jo_vdp2_release_top(jo_vdp2_memory_segment * const segments, const unsigned int count, unsigned char **top)
{
    register unsigned int       i;
    bool                        found;

    do
    {
        found = false;
        for (JO_ZERO(i); i < count; ++i)
        {
            if (segments[i].size != 0 && segments[i].is_free && segments[i].ptr + segments[i].size == *top)
            {
                *top = segments[i].ptr;
                JO_ZERO(segments[i].size);
                found = true;
            }
        }
    } while (found);
}

/** @brief Copy an entry of the blocks (field by field: a copy of the structure called the block copy of the library of the
 *  compiler, 1.2 KB more in each program) */
static  __jo_force_inline void  jo_vdp2_copy_segment(jo_vdp2_memory_segment * const dest, const jo_vdp2_memory_segment * const src)
{
    dest->ptr = src->ptr;
    dest->size = src->size;
    dest->data = src->data;
    dest->is_free = src->is_free;
}

/** @brief Merge a free block with the free block that follows it in the bank (the blocks are in the order of their
 *  addresses: a new block is at the top of the bank, in the first unused entry, and only the blocks at the top are
 *  released), the entries after it moved down
 *  @param segments Blocks of the bank
 *  @param count Blocks count
 *  @param i The free block
 *  @remarks 2 images freed side by side make room for a bigger one (a free block was only reused for a block of its size)
 */
static void                     jo_vdp2_merge_next(jo_vdp2_memory_segment * const segments, const unsigned int count, const unsigned int i)
{
    register unsigned int       k;

    if (i + 1 >= count || segments[i + 1].size == 0 || !segments[i + 1].is_free ||
        segments[i].ptr + segments[i].size != segments[i + 1].ptr)
        return;
    segments[i].size += segments[i + 1].size;
    for (k = i + 1; k + 1 < count; ++k)
        jo_vdp2_copy_segment(&segments[k], &segments[k + 1]);
    // (an entry of size 0 is unused: jo_vdp2_free_in() skips it)
    JO_ZERO(segments[count - 1].size);
}

/** @brief Reuse a free block: the rest of the block after the end of the data stays free (a block of its own when an entry
 *  is unused: the entries after it moved up)
 *  @param segments Blocks of the bank
 *  @param count Blocks count
 *  @param i The free block
 *  @param data The start of the data in the block
 *  @param end The end of the data
 *  @return data
 *  @remarks Without the rest kept, the free blocks merged (see jo_vdp2_merge_next()) failed more allocations than before
 */
static void                     *jo_vdp2_reuse(jo_vdp2_memory_segment * const segments, const unsigned int count, const unsigned int i,
                                               unsigned char * const data, unsigned char * const end)
{
    register unsigned int       k;

    segments[i].data = data;
    segments[i].is_free = false;
    if (end < segments[i].ptr + segments[i].size && segments[count - 1].size == 0)
    {
        for (k = count - 1; k > i + 1; --k)
            jo_vdp2_copy_segment(&segments[k], &segments[k - 1]);
        segments[i + 1].ptr = end;
        segments[i + 1].size = (unsigned int)(segments[i].ptr + segments[i].size - end);
        segments[i + 1].is_free = true;
        segments[i].size = (unsigned int)(end - segments[i].ptr);
    }
    return (data);
}

/** @brief Free a block of a bank
 *  @param segments Blocks of the bank
 *  @param count Blocks count
 *  @param top The end of the used part of the bank
 *  @param p The block (see jo_vdp2_malloc())
 *  @return false if the block is not in this bank
 */
static bool                     jo_vdp2_free_in(jo_vdp2_memory_segment * const segments, const unsigned int count, unsigned char **top,
                                                const void * const p)
{
    register unsigned int       i;

    for (JO_ZERO(i); i < count; ++i)
    {
        if (segments[i].size != 0 && segments[i].data == p && !segments[i].is_free)
        {
            segments[i].is_free = true;
            // With the free blocks around it
            jo_vdp2_merge_next(segments, count, i);
            if (i > 0 && segments[i - 1].is_free)
                jo_vdp2_merge_next(segments, count, i - 1);
            jo_vdp2_release_top(segments, count, top);
            return (true);
        }
    }
    return (false);
}

inline void                     jo_vdp2_free(const void * const p)
{
    if (p == JO_NULL)
        return;
    if (!jo_vdp2_free_in(__jo_a0_segments, A0_SEGMENT_COUNT, &__jo_a0, p) &&
        !jo_vdp2_free_in(__jo_b0_segments, B0_SEGMENT_COUNT, &__jo_b0, p))
        jo_vdp2_free_in(__jo_b1_segments, B1_SEGMENT_COUNT, &__jo_b1, p);
}

/** @brief The start of a new block: a map is aligned on its size (the map registers hold the number of the page: an
 *  unaligned map would be displayed from the start of its page)
 *  @param top The end of the used part of the bank
 *  @param usage The usage of the block
 *  @return The start of the block
 */
static  __jo_force_inline unsigned char *jo_vdp2_block_start(unsigned char *top, const jo_vdp2_ram_usage usage)
{
    switch (usage)
    {
        case JO_VDP2_RAM_MAP_NBG0:
        case JO_VDP2_RAM_MAP_NBG1:
        case JO_VDP2_RAM_MAP_NBG2:
        case JO_VDP2_RAM_MAP_NBG3:
        case JO_VDP2_RAM_MAP_RBG0:
            return ((unsigned char *)(((unsigned int)top + JO_VDP2_MAP_SIZE - 1) & ~(JO_VDP2_MAP_SIZE - 1)));
        default:
            return (top);
    }
}

/** @brief A block in the bank B1 (the maps and the cells of NBG0, NBG2 and NBG3, the tables of the windows and of the line
 *  color screen, the blocks of the game): after the font of jo_printf(), around its map
 *  @param usage The usage of the block
 *  @param n Size in bytes
 *  @return The block, JO_NULL if the bank is full
 */
static void                     *jo_vdp2_malloc_b1(const jo_vdp2_ram_usage usage, const unsigned int n)
{
    register unsigned int       i;
    unsigned char               *start;

    for (JO_ZERO(i); i < B1_SEGMENT_COUNT; ++i)
    {
        if (__jo_b1_segments[i].size == 0)
        {
            // (the end of the bank: the next bank holds other data, see the top of this file; the padding before
            // an aligned map belongs to the block: freed with it)
            start = jo_vdp2_block_start(__jo_b1, usage);
#ifdef JO_COMPILE_WITH_PRINTF_SUPPORT
            if (start < JO_VDP2_B1_HOLE_END && start + n > JO_VDP2_B1_HOLE_START)
                start = jo_vdp2_block_start(JO_VDP2_B1_HOLE_END, usage);
#endif
            if (start + n > (unsigned char *)(JO_VDP2_VRAM + 0x80000))
                return (JO_NULL);
            __jo_b1_segments[i].ptr = __jo_b1;
            __jo_b1_segments[i].size = (unsigned int)(start + n - __jo_b1);
            __jo_b1_segments[i].data = start;
            __jo_b1 = start + n;
            __jo_b1_segments[i].is_free = false;
            return (start);
        }
        start = jo_vdp2_block_start(__jo_b1_segments[i].ptr, usage);
#ifdef JO_COMPILE_WITH_PRINTF_SUPPORT
        // (a free block over the map of jo_printf(): after it, as a new block)
        if (start < JO_VDP2_B1_HOLE_END && start + n > JO_VDP2_B1_HOLE_START)
            start = jo_vdp2_block_start(JO_VDP2_B1_HOLE_END, usage);
#endif
        if (__jo_b1_segments[i].is_free && start + n <= __jo_b1_segments[i].ptr + __jo_b1_segments[i].size)
            return (jo_vdp2_reuse(__jo_b1_segments, B1_SEGMENT_COUNT, i, start, start + n));
    }
    return (JO_NULL);
}

/** @brief A block in the bank B0 (the maps of NBG1 and RBG0)
 *  @param usage The usage of the block
 *  @param n Size in bytes
 *  @return The block, JO_NULL if the bank is full
 */
static void                     *jo_vdp2_malloc_b0(const jo_vdp2_ram_usage usage, const unsigned int n)
{
    register unsigned int       i;
    unsigned char               *start;

    for (JO_ZERO(i); i < B0_SEGMENT_COUNT; ++i)
    {
        if (__jo_b0_segments[i].size == 0)
        {
            // (the end of the bank: the next bank holds other data, see the top of this file; the padding before
            // an aligned map belongs to the block: freed with it)
            start = jo_vdp2_block_start(__jo_b0, usage);
            if (start + n > (unsigned char *)JO_VDP2_VRAM_B1)
                return (JO_NULL);
            __jo_b0_segments[i].ptr = __jo_b0;
            __jo_b0_segments[i].size = (unsigned int)(start + n - __jo_b0);
            __jo_b0_segments[i].data = start;
            __jo_b0 = start + n;
            __jo_b0_segments[i].is_free = false;
            return (start);
        }
        start = jo_vdp2_block_start(__jo_b0_segments[i].ptr, usage);
        if (__jo_b0_segments[i].is_free && start + n <= __jo_b0_segments[i].ptr + __jo_b0_segments[i].size)
            return (jo_vdp2_reuse(__jo_b0_segments, B0_SEGMENT_COUNT, i, start, start + n));
    }
    return (JO_NULL);
}

/** @brief A block in the banks A0 and A1 (the bitmaps of NBG0 and NBG1, the cells of NBG1 and RBG0, the line scroll tables)
 *  @param usage The usage of the block
 *  @param n Size in bytes
 *  @return The block, JO_NULL if the banks are full
 */
static void                     *jo_vdp2_malloc_a0(const jo_vdp2_ram_usage usage, const unsigned int n)
{
    register unsigned int       i;

    for (JO_ZERO(i); i < A0_SEGMENT_COUNT; ++i)
    {
        if (__jo_a0_segments[i].size == 0)
        {
            // (the end of the bank: the cells of the rotation screen stop before its coefficient table (A1), a bitmap
            // can use A0 and A1)
            if (__jo_a0 + n > (unsigned char *)(usage == JO_VDP2_RAM_CELL_RBG0 ? JO_VDP2_VRAM_A1 : JO_VDP2_VRAM_B0))
                return (JO_NULL);
            __jo_a0_segments[i].ptr = __jo_a0;
            __jo_a0_segments[i].data = __jo_a0;
            __jo_a0 += n;
            __jo_a0_segments[i].size = n;
            __jo_a0_segments[i].is_free = false;
            return (__jo_a0_segments[i].ptr);
        }
        // (the cells of the rotation screen: the same end as a new block, a freed bitmap of A0 and A1 reused for them went
        // over its coefficient table. The other blocks are all before B0)
        if (__jo_a0_segments[i].is_free && __jo_a0_segments[i].size >= n &&
            (usage != JO_VDP2_RAM_CELL_RBG0 || __jo_a0_segments[i].ptr + n <= (unsigned char *)JO_VDP2_VRAM_A1))
            return (jo_vdp2_reuse(__jo_a0_segments, A0_SEGMENT_COUNT, i, __jo_a0_segments[i].ptr, __jo_a0_segments[i].ptr + n));
    }
    return (JO_NULL);
}

/** @brief The bank B0 can take the blocks of the other banks when they are full: its maps (NBG1 in cells, RBG0) are small,
 *  and a bitmap of NBG1 (A0 and A1) leaves it free. Not with the rotation screen (its banks are read by it only) */
static  __jo_force_inline bool  jo_vdp2_b0_can_help(void)
{
    return (!(__jo_vdp2_displayed_screens & JO_RBG0_SCREEN));
}

void                            *jo_vdp2_malloc(const jo_vdp2_ram_usage usage, unsigned int n)
{
    void                        *ptr;

    // (a size bigger than the whole VRAM must not wrap the tests of the ends of the banks: start + n)
    if (n > 0x80000 && usage != JO_VDP2_RAM_BACK_COLOR && usage != JO_VDP2_RAM_RTABLE && usage != JO_VDP2_RAM_KTABLE &&
        usage != JO_VDP2_RAM_COLOR)
    {
#ifdef JO_DEBUG
        if (!__jo_vdp2_quiet)
            jo_core_error("Out of VDP2 memory");
#endif
        return (JO_NULL);
    }
    switch (usage)
    {
        case JO_VDP2_RAM_BACK_COLOR:
            return ((void*)__jo_back_color);
        case JO_VDP2_RAM_RTABLE:
            return ((void*)__jo_rbg0_r_table);
        case JO_VDP2_RAM_KTABLE:
            return ((void*)__jo_rbg0_k_table);
        case JO_VDP2_RAM_USER:
        case JO_VDP2_RAM_MAP_NBG0:
        case JO_VDP2_RAM_MAP_NBG2:
        case JO_VDP2_RAM_MAP_NBG3:
        case JO_VDP2_RAM_CELL_NBG0:
        case JO_VDP2_RAM_CELL_NBG2:
        case JO_VDP2_RAM_CELL_NBG3:
        case JO_VDP2_LINE_COLOR_TABLE:
            // (B1 full, with the 2 layers of 256 colors of NBG2 and NBG3: the rest in B0)
            if ((ptr = jo_vdp2_malloc_b1(usage, n)) != JO_NULL || (jo_vdp2_b0_can_help() && (ptr = jo_vdp2_malloc_b0(usage, n)) != JO_NULL))
                return (ptr);
            break;
        case JO_VDP2_RAM_MAP_NBG1:
        case JO_VDP2_RAM_MAP_RBG0:
            if ((ptr = jo_vdp2_malloc_b0(usage, n)) != JO_NULL)
                return (ptr);
            break;
        case JO_VDP2_RAM_LINE_SCROLL:
            // (the banks A0 and A1 full with the bitmap of NBG1 in 256 colors (jo_vdp2_set_nbg1_8bits_bitmap()): the table in
            // B0 or B1, read by the VDP2 in the horizontal blanking from any bank)
            if ((ptr = jo_vdp2_malloc_a0(usage, n)) != JO_NULL || (jo_vdp2_b0_can_help() && (ptr = jo_vdp2_malloc_b0(usage, n)) != JO_NULL) ||
                (ptr = jo_vdp2_malloc_b1(usage, n)) != JO_NULL)
                return (ptr);
            break;
        case JO_VDP2_RAM_BITMAP_NBG0:
        case JO_VDP2_RAM_BITMAP_NBG1:
        case JO_VDP2_RAM_CELL_NBG1:
        case JO_VDP2_RAM_CELL_RBG0:
            if ((ptr = jo_vdp2_malloc_a0(usage, n)) != JO_NULL)
                return (ptr);
            break;
        case JO_VDP2_RAM_COLOR:
            // The color RAM has JO_PALETTE_MAX palettes of 256 colors
            if ((__jo_current_palette_id + 1) >= JO_PALETTE_MAX)
                break;
            ++__jo_current_palette_id;
            ptr = __jo_cram;
            __jo_cram += CRAM_PALETTE_SIZE;
            return (ptr);
        default:
#ifdef JO_DEBUG
            jo_core_error("Unsupported usage %d", usage);
#endif
            return (JO_NULL);
    }
#ifdef JO_DEBUG
    if (!__jo_vdp2_quiet)
        jo_core_error("Out of VDP2 memory");
#endif
    return (JO_NULL);
}

/** @brief A new block at the start of a bank of a pool (see __jo_vdp2_malloc_bank())
 *  @param segments Blocks of the pool
 *  @param count Block count of the pool
 *  @param top Top of the pool
 *  @param end End of the pool
 *  @param n Size in bytes
 *  @return The block, JO_NULL if it doesn't fit
 */
static void                     *jo_vdp2_malloc_bank_in(jo_vdp2_memory_segment * const segments, const unsigned int count,
                                                        unsigned char ** const top, const unsigned char * const end,
                                                        const unsigned int n)
{
    register unsigned int       i;
    unsigned char               *start;

    start = (unsigned char *)(((unsigned int)*top + JO_VDP2_BANK_SIZE - 1) & ~(JO_VDP2_BANK_SIZE - 1));
    if (start + n > end)
        return (JO_NULL);
    for (JO_ZERO(i); i < count; ++i)
    {
        if (segments[i].size != 0)
            continue;
        // (the space before the start of the bank: in the block, given back with it)
        segments[i].ptr = *top;
        segments[i].data = start;
        segments[i].size = (unsigned int)(start + n - *top);
        segments[i].is_free = false;
        *top = start + n;
        return (start);
    }
    return (JO_NULL);
}

void                            *__jo_vdp2_malloc_bank(const unsigned int n)
{
    void                        *ptr;

    // The banks A0 and A1 (the pool of the bitmaps), then B0 (the pool of the maps of NBG1 and RBG0); not B1 (the text)
    if ((ptr = jo_vdp2_malloc_bank_in(__jo_a0_segments, A0_SEGMENT_COUNT, &__jo_a0, (unsigned char *)JO_VDP2_VRAM_B0, n)) != JO_NULL)
        return (ptr);
    return (jo_vdp2_malloc_bank_in(__jo_b0_segments, B0_SEGMENT_COUNT, &__jo_b0, (unsigned char *)JO_VDP2_VRAM_B1, n));
}

void                            *__jo_vdp2_malloc_near(const jo_vdp2_ram_usage usage, const unsigned int n, const void * const near)
{
    void                        *ptr;

    if (near != JO_NULL && (const unsigned char *)near >= (const unsigned char *)JO_VDP2_VRAM_B0 &&
        (const unsigned char *)near < (const unsigned char *)JO_VDP2_VRAM_B1 && jo_vdp2_b0_can_help() &&
        (ptr = jo_vdp2_malloc_b0(usage, n)) != JO_NULL)
        return (ptr);
    return (jo_vdp2_malloc(usage, n));
}

void                            *__jo_vdp2_malloc_try(const jo_vdp2_ram_usage usage, const unsigned int n)
{
#ifdef JO_DEBUG
    void                        *ptr;

    __jo_vdp2_quiet = true;
    ptr = jo_vdp2_malloc(usage, n);
    __jo_vdp2_quiet = false;
    return (ptr);
#else
    return (jo_vdp2_malloc(usage, n));
#endif
}
