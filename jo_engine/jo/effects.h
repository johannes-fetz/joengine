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
/** @file effects.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Function prototypes special effects (Laser beam, fire, smoke, flash, shake, etc)
 *  @bug No known bugs.
 */

#ifndef __JO_EFFECTS_H__
# define __JO_EFFECTS_H__

#ifdef JO_COMPILE_WITH_EFFECTS_SUPPORT

/*
██╗  ██╗ ██████╗ ███╗   ███╗██╗███╗   ██╗ ██████╗     ██╗      █████╗ ███████╗███████╗██████╗
██║  ██║██╔═══██╗████╗ ████║██║████╗  ██║██╔════╝     ██║     ██╔══██╗██╔════╝██╔════╝██╔══██╗
███████║██║   ██║██╔████╔██║██║██╔██╗ ██║██║  ███╗    ██║     ███████║███████╗█████╗  ██████╔╝
██╔══██║██║   ██║██║╚██╔╝██║██║██║╚██╗██║██║   ██║    ██║     ██╔══██║╚════██║██╔══╝  ██╔══██╗
██║  ██║╚██████╔╝██║ ╚═╝ ██║██║██║ ╚████║╚██████╔╝    ███████╗██║  ██║███████║███████╗██║  ██║
╚═╝  ╚═╝ ╚═════╝ ╚═╝     ╚═╝╚═╝╚═╝  ╚═══╝ ╚═════╝     ╚══════╝╚═╝  ╚═╝╚══════╝╚══════╝╚═╝  ╚═╝
*/

/** @brief Homing laser effect definition
 */
typedef struct
{
    jo_vector_fixed     source;
    bool                use_intermediate_point_a;
    jo_vector_fixed     intermediate_point_a;
    bool                use_intermediate_point_b;
    jo_vector_fixed     intermediate_point_b;
    jo_vector_fixed     target;
    jo_fixed            length;
    jo_fixed            horizontal_thickness;
    jo_fixed            vertical_thickness;
    jo_fixed            section_step;
    int                 sprite_id;
    bool                center_sprites_according_to_thickness;
}                       jo_homing_laser;

/** @brief Init jo_homing_laser struct
 *  @param homing_laser Homing laser definition
 *  @param sprite_id Sprite ID
 *  @param thickness Horizontal Laser thickness (1 to 256)
 *  @param z Default z index (500 for example)
 *  @param nb_sections Number of section in the curve aka smoothness (4 to 64 and must be a multiple of 2).
 *  @remarks nb_sections=20 Should be enough for most case
 */
void    jo_init_homing_laser(jo_homing_laser * const homing_laser, const int sprite_id, const unsigned short thickness,
                             const int z, const unsigned short nb_sections);

/** @brief Draw homing laser effect
 *  @param homing_laser Homing laser definition
 *  @remarks source, intermediate_point_a, intermediate_point_b, target and length must be set before this call
 */
void    jo_draw_homing_laser(jo_homing_laser * const homing_laser);

#ifdef JO_COMPILE_WITH_3D_SUPPORT

/*
** Effects of the 3D world: animated sprites (fire, smoke, dust, sparks...), flash of the screen, shake of the camera.
** Cheap on the Saturn: the sprites are opaque with end codes (half transparency costs a lot of VDP1 time), the flash is
** the color offset of the VDP2 (free), the shake is an offset of the camera given to the game.
** The explosions of the entities use them (see entity.h)
*/

/** @brief Effects at the same time (the oldest one is reused) */
# define JO_EFFECT_MAX                      (32)
/** @brief Frames of the animation of an effect at most */
# define JO_EFFECT_MAX_FRAMES               (4)

/** @brief The sprite is mixed with what is behind (VDP1 half transparency: costly, for small sprites only) */
# define JO_EFFECT_HALF_TRANSPARENT         (0x01)
/** @brief The sprite stands on its position (else it is centered on it) */
# define JO_EFFECT_STANDING                 (0x02)
/** @brief The sprite is flipped at random (the effects don't look the same) */
# define JO_EFFECT_RANDOM_FLIP              (0x04)

/** @brief Definition of an effect (shared by the effects of the same kind) */
typedef struct
{
    short                           sprites[JO_EFFECT_MAX_FRAMES];  // Frames of the animation (played once during the life)
    unsigned char                   frame_count;
    unsigned char                   life;           // Frames of the game
    unsigned char                   flags;          // JO_EFFECT_xxx
    unsigned char                   __pad;
    short                           size_start;     // Half size at the start and at the end (world units)
    short                           size_end;
    short                           rise;           // Speed up (1/16 world units per frame of the game)
    short                           __pad2;
}                                   jo_effect_def;

/** @brief Add an effect
 *  @param def Definition (it must stay in memory)
 *  @param position Center (or foot: JO_EFFECT_STANDING), world space
 *  @param delay Frames before the effect starts (an explosion is a group of fireballs that start one after the other)
 */
void                                jo_effect_add(const jo_effect_def * const def, const jo_pos3D_fixed * const position, const int delay);

/** @brief Remove all the effects, the flash and the shake */
void                                jo_effects_clear(void);

/** @brief Animate the effects, the flash and the shake: once per frame of the game */
void                                jo_effects_update(void);

/** @brief Draw the effects (in the world: the camera matrix is the current matrix, see jo_3d_set_depth_bias())
 *  @return Number of VDP1 commands (sprites) drawn
 */
int                                 jo_effects_draw(void);

/** @brief Flash of the screen: the color offset of the VDP2 is added to all the screens, it fades to 0
 *  @param r Red (0 to 255: added to the colors)
 *  @param g Green
 *  @param b Blue
 *  @param frames Frames of the game until it is gone
 *  @remarks The registers are written during the VBLANK (no line of the screen with half of the flash). A brighter flash
 *           than the current one replaces it
 */
void                                jo_effect_flash(const int r, const int g, const int b, const int frames);

/** @brief Fade of the screen to black (or back): the color offset of the VDP2 (see jo_effect_flash())
 *  @param darkness 0 (no fade) to 255 (black)
 *  @param frames Frames of the game to reach it (see jo_effects_update()), 0: now
 *  @remarks A change of level: fade to black, load, fade back. jo_effects_clear() doesn't change the fade
 */
void                                jo_effect_fade(const int darkness, const int frames);

/** @brief Check if the fade is not finished (see jo_effect_fade()) */
bool                                jo_effect_is_fading(void);

/** @brief Shake of the camera
 *  @param amount Amplitude (world units). A stronger shake than the current one replaces it
 *  @param frames Frames of the game until it is gone
 */
void                                jo_effect_shake(const int amount, const int frames);

/** @brief Offset of the camera of the shake (add it to the position of the eyes)
 *  @param offset Offset (output, world units Q16.16), 0 when nothing shakes
 */
void                                jo_effect_get_shake(jo_pos3D_fixed * const offset);

#endif /* !JO_COMPILE_WITH_3D_SUPPORT */

#endif /* !JO_COMPILE_WITH_EFFECTS_SUPPORT */

#endif /* !__JO_EFFECTS_H__ */

/*
** END OF FILE
*/
