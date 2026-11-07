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

#endif /* !__JO_INTERNAL_VDP2_H__ */

/*
** END OF FILE
*/
