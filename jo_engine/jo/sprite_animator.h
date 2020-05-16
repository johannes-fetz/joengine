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
/** @file sprite_animator.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Sprite Animator
 *  @bug No known bugs.
 */

#ifndef __JO_SPRITE_ANIMATOR_H__
# define __JO_SPRITE_ANIMATOR_H__

/** @brief Animation action */
typedef enum
{
    JO_SPRITE_ANIM_START,
    JO_SPRITE_ANIM_START_REVERSE,
    JO_SPRITE_ANIM_STOP,
    JO_SPRITE_ANIM_STOP_AT_LAST_FRAME,
}					jo_sprite_anim_action;

/** @brief Internal animation struct */
typedef struct
{
    unsigned short					frame0_sprite_id;
    unsigned short					frame_count;
    unsigned short					cur_frame;
    unsigned char					frame_rate;
    unsigned char					frame_skip;
    jo_sprite_anim_action			action;
}									jo_sprite_anim;

/** @brief Internal animation table
 *  @warning MC Hammer: don't touch this
 */
extern jo_sprite_anim		__jo_sprite_anim_tab[JO_MAX_SPRITE_ANIM];

/** @brief Get animation frame rate
 *  @param anim_id Id returned by jo_create_sprite_anim()
 *  @return animation frame index (from 0 to "number of sprite in animation")
 */
static  __jo_force_inline unsigned char	jo_get_sprite_anim_frame_rate(const int anim_id)
{
    return __jo_sprite_anim_tab[anim_id].frame_rate;
}

/** @brief Get animation frame count
 *  @param anim_id Id returned by jo_create_sprite_anim()
 *  @return animation frame index (from 0 to "number of sprite in animation")
 */
static  __jo_force_inline unsigned short	jo_get_sprite_anim_frame_count(const int anim_id)
{
    return __jo_sprite_anim_tab[anim_id].frame_count;
}

/** @brief Get current animation frame
 *  @param anim_id Id returned by jo_create_sprite_anim()
 *  @return animation frame index (from 0 to "number of sprite in animation")
 */
static  __jo_force_inline unsigned short	jo_get_sprite_anim_frame(const int anim_id)
{
    return __jo_sprite_anim_tab[anim_id].cur_frame;
}

/** @brief Set animation frame
 *  @param anim_id Id returned by jo_create_sprite_anim()
 *  @param frame Frame of animation to be displayed, where 0 is the first frame.
 */
static __jo_force_inline void	jo_set_sprite_anim_frame(const int anim_id, const unsigned char frame)
{
    __jo_sprite_anim_tab[anim_id].cur_frame = frame;
}

/** @brief Start animation loop
 *  @param anim_id Id returned by jo_create_sprite_anim()
 */
static  __jo_force_inline void	jo_start_sprite_anim_loop(const int anim_id)
{
    __jo_sprite_anim_tab[anim_id].action = JO_SPRITE_ANIM_START;
}

/** @brief Determine if the animation is stopped
 *  @param anim_id Id returned by jo_create_sprite_anim()
 *  @return true if the animation is stopped otherwise false
 */
static  __jo_force_inline bool	jo_is_sprite_anim_stopped(const int anim_id)
{
    return __jo_sprite_anim_tab[anim_id].action == JO_SPRITE_ANIM_STOP;
}

/** @brief Stop animation
 *  @param anim_id Id returned by jo_create_sprite_anim()
 */
static  __jo_force_inline void	jo_stop_sprite_anim(const int anim_id)
{
    __jo_sprite_anim_tab[anim_id].action = JO_SPRITE_ANIM_STOP;
}

/** @brief Start animation and stop it at the last frame
 *  @param anim_id Id returned by jo_create_sprite_anim()
 */
static  __jo_force_inline void	jo_start_sprite_anim(const int anim_id)
{
    __jo_sprite_anim_tab[anim_id].action = JO_SPRITE_ANIM_STOP_AT_LAST_FRAME;
}

/** @brief Stop animation and reset the animation to the first frame
 *  @param anim_id Id returned by jo_create_sprite_anim()
 */
static  __jo_force_inline void	jo_reset_sprite_anim(const int anim_id)
{
    jo_stop_sprite_anim(anim_id);
    JO_ZERO(__jo_sprite_anim_tab[anim_id].frame_skip);
    JO_ZERO(__jo_sprite_anim_tab[anim_id].cur_frame);
}

/** @brief Restart animation and stop it at the last frame
 *  @param anim_id Id returned by jo_create_sprite_anim()
 */
static  __jo_force_inline void	jo_restart_sprite_anim(const int anim_id)
{
    jo_reset_sprite_anim(anim_id);
    jo_start_sprite_anim(anim_id);
}

/** @brief Get the sprite to display according to the animation (current frame)
 *  @param anim_id Id returned by jo_create_sprite_anim()
 */
static  __jo_force_inline int	jo_get_anim_sprite(const int anim_id)
{
    return __jo_sprite_anim_tab[anim_id].frame0_sprite_id + __jo_sprite_anim_tab[anim_id].cur_frame;
}

/** @brief Get the sprite to display according to the animation in reverse (current frame)
 *  @param anim_id Id returned by jo_create_sprite_anim()
 */
static  __jo_force_inline int	jo_get_anim_sprite_reverse(const int anim_id)
{
    return __jo_sprite_anim_tab[anim_id].frame0_sprite_id + __jo_sprite_anim_tab[anim_id].frame_count - __jo_sprite_anim_tab[anim_id].cur_frame - 1;
}

/** @brief Set animation framerate
 *  @param anim_id Id returned by jo_create_sprite_anim()
 *  @param framerate Frame rate (linked to the global framerate)
 */
static  __jo_force_inline int	jo_set_sprite_anim_frame_rate(const int anim_id, const unsigned char framerate)
{
    return __jo_sprite_anim_tab[anim_id].frame_rate = framerate;
}

/** @brief Replace an existing animation
 *  @param at Id returned by jo_create_sprite_anim()
 *  @param sprite_id First sprite id returned by jo_sprite_add() or similar functions
 *  @param frame_count Number of frame in the animation (number of continious sprite)
 *  @param frame_rate Framerate (linked to the global framerate)
 *  @return -1 if failed or the "at" param
 */
int			jo_replace_sprite_anim(const int at, const unsigned short sprite_id, const unsigned short frame_count, const unsigned char frame_rate);

/** @brief Create an animation
 *  @param sprite_id First sprite id returned by jo_sprite_add() or similar functions
 *  @param frame_count Number of frame in the animation (number of continious sprite)
 *  @param frame_rate Framerate (linked to the global framerate)
 *  @return -1 if failed or Animation Id (ANIM_ID)
 */
int			jo_create_sprite_anim(const unsigned short sprite_id, const unsigned short frame_count, const unsigned char frame_rate);

/** @brief Clear all sprite animation
 */
void        jo_clear_all_sprite_anim(void);

#endif /* !__JO_SPRITE_ANIMATOR_H__ */

/*
** END OF FILE
*/
