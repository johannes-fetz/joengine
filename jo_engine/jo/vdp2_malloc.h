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
/** @file vdp2_malloc.h
 *  @author Johannes Fetz
 *
 *  @brief VDP2 memory allocation (cell, map, rotation, scroll, etc)
 *  @bug No known bugs.
 */

#ifndef __JO_VDP2_MALLOC_H__
# define __JO_VDP2_MALLOC_H__

/*
** MACROS
*/
# define JO_VDP2_KTABLE_SIZE                    (0x1FE00)
# define JO_VDP2_ROTATION_TABLE_SIZE            (0x00080)
# define JO_VDP2_MAP_SIZE                       (8192)
# define JO_VDP2_CELL_TO_MAP_OFFSET(CELL_PTR)   JO_DIV_BY_32(JO_MOD_POW2((int)CELL_PTR, JO_VDP2_BANK_SIZE))

typedef enum
{
    /** @brief For user */
    JO_VDP2_RAM_USER,
    /** @brief MAP (for sl1MapRA, slMapNbg1, etc) */
    JO_VDP2_RAM_MAP_NBG0,
    JO_VDP2_RAM_MAP_NBG1,
    JO_VDP2_RAM_MAP_NBG2,
    JO_VDP2_RAM_MAP_NBG3,
    JO_VDP2_RAM_MAP_RBG0,
    /** @brief KTABLE (for slMakeKtable) */
    JO_VDP2_RAM_KTABLE,
    /** @brief Rotation table (for slRparaInitSet) */
    JO_VDP2_RAM_RTABLE,
    /** @brief CELL (for slPageRbg0, slPageNbg1, etc) */
    JO_VDP2_RAM_CELL_NBG0,
    JO_VDP2_RAM_CELL_NBG1,
    JO_VDP2_RAM_CELL_NBG2,
    JO_VDP2_RAM_CELL_NBG3,
    JO_VDP2_RAM_CELL_RBG0,
    /** @brief Back color (for slBack1ColSet) */
    JO_VDP2_RAM_BACK_COLOR,
    /** @brief Line Scroll table (for slLineScrollTable0 and slLineScrollTable1) */
    JO_VDP2_RAM_LINE_SCROLL,
    /** @brief CELL (for slBitMapNbg0, slBitMapNbg1, etc) */
    JO_VDP2_RAM_BITMAP_NBG0,
    JO_VDP2_RAM_BITMAP_NBG1,
    /** @brief Color RAM for Palette, etc */
    JO_VDP2_RAM_COLOR
}                               jo_vdp2_ram_usage;

/** @brief Dynamic VDP2 memory allocator
 *  @param usage VDP2 memory segment usage
 *  @param n Segment size
 *  @return a pointer to the allocated memory or NULL if failed
 */
void                            *jo_vdp2_malloc(const jo_vdp2_ram_usage usage, const unsigned int n);

/** @brief Dynamic VDP2 memory allocator for known size usage (JO_VDP2_RAM_RTABLE, JO_VDP2_RAM_KTABLE, JO_VDP2_RAM_COLOR)
 *  @param usage VDP2 memory segment usage
 *  @return a pointer to the allocated memory or NULL if failed
 */
static  __jo_force_inline void  *jo_vdp2_malloc_autosize(const jo_vdp2_ram_usage usage)
{
    return (jo_vdp2_malloc(usage, ~0));
}

/** @brief function frees VDP2 memory space pointed to by p
 *  @param p pointer that must have been returned by a previous call to jo_vdp2_malloc()
 */
void                            jo_vdp2_free(const void * const p);

#endif /* !__JO_VDP2_MALLOC_H__ */

/*
** END OF FILE
*/
