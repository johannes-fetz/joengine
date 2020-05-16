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
/** @file hitbox.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Hitbox and tools
 */

#ifndef __JO_HITBOX_H__
# define __JO_HITBOX_H__

/** @brief Fast method to get if two sprites intersects (HitBox processing)
 *  @param sprite_id1 Sprite Id of the first sprite
 *  @param x1 Horizontal position of the first sprite
 *  @param y1 Vertical position of the first sprite
 *  @param sprite_id2 Sprite Id of the second sprite
 *  @param x2 Horizontal position of the second sprite
 *  @param y2 Vertical position of the second sprite
 *  @return true if sprites collides otherwise false
 */
static  __jo_force_inline bool	jo_hitbox_detection(const int sprite_id1, const int x1, const int y1, const int sprite_id2, const int x2, const int y2)
{
    return jo_square_intersect(x1 - JO_DIV_BY_2(__jo_sprite_def[sprite_id1].width),
                               y1 - JO_DIV_BY_2(__jo_sprite_def[sprite_id1].height),
                               __jo_sprite_def[sprite_id1].width,
                               __jo_sprite_def[sprite_id1].height,
                               x2 - JO_DIV_BY_2(__jo_sprite_def[sprite_id2].width),
                               y2 - JO_DIV_BY_2(__jo_sprite_def[sprite_id2].height),
                               __jo_sprite_def[sprite_id2].width,
                               __jo_sprite_def[sprite_id2].height);

}

/** @brief Fast method to get if the sprite intersects with the box (HitBox processing)
 *  @param sprite_id1 Sprite Id of the first sprite
 *  @param x1 Horizontal position of the first sprite
 *  @param y1 Vertical position of the first sprite
 *  @param x2 Horizontal position of the second box
 *  @param y2 Vertical position of the second box
 *  @param w2 Width of the second box
 *  @param h2 Height of the second box
 *  @return true if sprites collides otherwise false
 */
static  __jo_force_inline bool	jo_hitbox_detection_custom_boundaries(const int sprite_id1, const int x1, const int y1, const int x2, const int y2, const int w2, const int h2)
{
    return jo_square_intersect(x1 - JO_DIV_BY_2(__jo_sprite_def[sprite_id1].width),
                               y1 - JO_DIV_BY_2(__jo_sprite_def[sprite_id1].height),
                               __jo_sprite_def[sprite_id1].width,
                               __jo_sprite_def[sprite_id1].height,
                               x2 - JO_DIV_BY_2(w2),
                               y2 - JO_DIV_BY_2(h2),
                               w2,
                               h2);

}

#endif /* !__JO_HITBOX_H__ */

/*
** END OF FILE
*/
