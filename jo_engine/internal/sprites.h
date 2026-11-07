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
/** @file internal/sprites.h
 *  @author Johannes Fetz
 *
 *  @brief Sprites: internal functions of the engine (see jo/sprites.h)
 *  @warning Not part of the API: only for the source files of the engine (included after jo/sprites.h). The games
 *  include jo/jo.h, which doesn't include this file
 *  @bug No known bugs.
 */

#ifndef __JO_INTERNAL_SPRITES_H__
# define __JO_INTERNAL_SPRITES_H__

/** @brief (internal engine usage) Color lookup table of a 16 colors texture (VRAM address / 8), 0: none */
extern unsigned short           __jo_sprite_lut[JO_MAX_SPRITE];

/** @brief Add a 16 colors texture that uses the color lookup table of another one (internal engine usage: smaller copies,
 *  tiles, see 3d.c)
 *  @param pixels 2 pixels per byte
 *  @param width Width (multiple of 8)
 *  @param height Height
 *  @param lut Table (VRAM address / 8, see __jo_sprite_lut)
 *  @return Sprite Id or -1 if failed
 */
int     __jo_sprite_add_4bits(const void * const pixels, const unsigned short width, const unsigned short height, const unsigned short lut);

/** @brief Replace a sprite (internal usage)
 *  @param img Pointer to a image struct
 *  @param sprite_id Sprite ID to replace
 *  @param async true: the copy is done by the DMA controller in the background (see jo_dma_wait())
 *  @return sprite_id
 */
int     __jo_sprite_replace(const jo_img * const img, const int sprite_id, const bool async);

#endif /* !__JO_INTERNAL_SPRITES_H__ */

/*
** END OF FILE
*/
