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
/** @file internal/vdp2.h
 *  @author Johannes Fetz
 *
 *  @brief VDP2: internal functions of the engine (see jo/vdp2.h)
 *  @warning Not part of the API: only for the source files of the engine (included after jo/vdp2.h). The games
 *  include jo/jo.h, which doesn't include this file
 *  @bug No known bugs.
 */

#ifndef __JO_INTERNAL_VDP2_H__
# define __JO_INTERNAL_VDP2_H__

/** @brief Fill a rectangle in the NBG1 bitmap (Internal usage)
 *  @param x Horizontal position in the bitmap (wrapped around JO_VDP2_WIDTH)
 *  @param y Vertical position in the bitmap (wrapped around JO_VDP2_HEIGHT)
 *  @param width Rectangle width
 *  @param height Rectangle height
 *  @param color Fill color
 *  @remarks The rectangle wraps around the borders of the bitmap like the NBG1 scroll
 */
void    __jo_vdp2_nbg1_bitmap_fill(const int x, const int y, int width, int height, const jo_color color);

/** @brief Copy a part of an image in the NBG1 bitmap (Internal usage)
 *  @param img Image
 *  @param src_x Horizontal position of the part in the image
 *  @param src_y Vertical position of the part in the image
 *  @param width Part width (must be inside the image)
 *  @param height Part height (must be inside the image)
 *  @param x Horizontal position in the bitmap (wrapped around JO_VDP2_WIDTH)
 *  @param y Vertical position in the bitmap (wrapped around JO_VDP2_HEIGHT)
 *  @remarks The part wraps around the borders of the bitmap like the NBG1 scroll
 */
void    __jo_vdp2_nbg1_bitmap_blit(const jo_img * const img, const int src_x, const int src_y, int width, int height, const int x, const int y);

/** @brief End of a frame of the game (Internal usage, called by __jo_core_synch() before __jo_vdp1_end_frame()): the
 *  scroll registers and the rotation parameters set by the frame are kept with its number (__jo_vdp1_frame_tag()) and
 *  applied at the VBLANK where the VDP1 displays its image (see __jo_vdp1_displayed_frame_changed())
 */
void            __jo_vdp2_end_frame(void);

/** @brief Number of the last frame whose VDP2 state was applied (Internal usage, the test ROM)
 *  @return Frame number (see __jo_vdp1_frame_tag())
 */
unsigned int    __jo_vdp2_applied_frame_tag(void);

/** @brief Set the priority of a screen (Internal usage, see jo_core_set_screens_order())
 *  @param screen Screen (JO_SPRITE_SCREEN: the 8 priority registers of the sprites)
 *  @param priority 0 (hidden) to 7
 */
void            __jo_vdp2_set_priority(const jo_scroll_screen screen, const unsigned short priority);

/** @brief Highest priority of a screen (0: hidden) */
# define JO_VDP2_MAX_PRIORITY       (7)

/** @brief Priority of a screen (Internal usage)
 *  @param screen Screen (JO_SPRITE_SCREEN: the priority register 0 of the sprites)
 *  @return 0 (hidden) to JO_VDP2_MAX_PRIORITY
 */
unsigned short  __jo_vdp2_get_priority(const jo_scroll_screen screen);

/** @brief Set NBG0 or NBG1 as a bitmap of 32768 colors (512 x 256), transparent, displayed (Internal usage: the
 *  transparency layer, see jo/transparency.h)
 *  @param screen JO_NBG0_SCREEN or JO_NBG1_SCREEN
 *  @param rows Rows used (the next ones are not displayed: the memory after them can hold other blocks)
 *  @return The bitmap in the VRAM (the start of a bank: NBG1 its bitmap at the start of A0 if it is free, else the
 *  first bank free after the blocks allocated, A1 or B0), JO_NULL if the screen shows an image or if no bank is free
 *  @remarks NBG0 with the text of jo_printf(): the text is hidden until __jo_vdp2_free_rgb_bitmap()
 */
void            *__jo_vdp2_set_rgb_bitmap(const jo_scroll_screen screen, const unsigned int rows);

/** @brief Give back a bitmap of __jo_vdp2_set_rgb_bitmap() (Internal usage): NBG0 shows the text of jo_printf() again
 *  (empty); the bitmap of NBG1 in A0 stays (the drawing functions of NBG1 use it)
 *  @param screen JO_NBG0_SCREEN or JO_NBG1_SCREEN
 *  @param bitmap The bitmap (JO_NULL: nothing)
 */
void            __jo_vdp2_free_rgb_bitmap(const jo_scroll_screen screen, void * const bitmap);

#endif /* !__JO_INTERNAL_VDP2_H__ */

/*
** END OF FILE
*/
