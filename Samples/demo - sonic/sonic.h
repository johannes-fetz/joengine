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

#ifndef __SONIC_H__
# define __SONIC_H__

# define WORLD_MAP_ID		            (0)
# define WORLD_DEFAULT_X                (540)
# define WORLD_DEFAULT_Y                (120)

# define MAP_TILE_BLOCK_ATTR            (1)

# define SONIC_WIDTH                    (48)
# define SONIC_WIDTH_2                  (24)
# define SONIC_HEIGHT                   (56)
# define SONIC_SPIN_SPEED               (20)

/* If sonic almost touch the ground we allow the user to jump */
# define SONIC_JUMP_PER_PIXEL_TOLERANCE (20)

typedef struct
{
    int         walking_anim_id;
    int         spin_sprite_id;
    int         x;
    int         y;
    bool        flip_sonic;
    bool        spin;
    bool        can_jump;
    int         angle;
}               sonic_t;

extern sonic_t player;

void            sonic_running_animation_handling();
void            display_sonic(void);
void            load_sonic(void);

#endif /* !__SONIC_H__ */

/*
** END OF FILE
*/
