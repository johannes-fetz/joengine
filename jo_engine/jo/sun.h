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
/** @file sun.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine sun: the sun in the sky of a 3D view (its halo and its core, sprites behind everything) and its lens
 *  flare (pieces on the line from the sun through the middle of the screen, in front of everything)
 *
 *  - The sun is where its direction (azimuth, elevation) is seen from the camera: the camera turns around the vertical
 *    axis (its heading) and looks down (the tangent of the angle). Out of the view: nothing drawn
 *  - The halo: strips of sprites drawn from the top (128 x 32 each by default), the core (32 x 32) over its middle; the
 *    lens flare: 3 pieces (a ring, a disc, a hexagon), each one drawn at several places and sizes (see jo_sun_flare)
 *  - The sprites are made by the game: Compiler/COMMON/jo_level/sun.py makes them in 16 colors (the section TX16 of a
 *    level file), see-through with the color calculation of the VDP2 (the blend ratios of the sprites, see
 *    jo_vdp2_set_sprite_blend_ratios()): the halo is mixed with the sky behind it. Under the horizon (over the water, the
 *    ground) the pieces of the lens flare can be drawn half transparent by the VDP1 instead (jo_sun.flare_rgb[]: the
 *    same pixels in RGB)
 *  - The lens flare fades in when the sun is near the middle of the screen and not hidden (the game says if something is
 *    between the camera and the sun: see jo_sun_update()), out when it goes away, a step per frame
 *
 *  Usage: jo_sun_init(), then set the sprites (jo_sun.halo[], halo_strips, core, flare[]); at each frame
 *  jo_sun_update() after the camera is known, jo_sun_draw() in the frame of the 3D, jo_sun_draw_flare() after the rest
 *  @bug No known bugs.
 */

#ifndef __JO_SUN_H__
# define __JO_SUN_H__

#ifdef JO_COMPILE_WITH_3D_SUPPORT

/** @brief Strips of the halo at most */
# define JO_SUN_MAX_HALO_STRIPS             (8)
/** @brief Pieces of the lens flare (sprites): a ring, a disc, a hexagon (see jo_level/sun.py: FLARE_PIECES) */
# define JO_SUN_FLARE_PIECES                (3)
/** @brief The full level of the lens flare (see jo_sun.flare_level) */
# define JO_SUN_FLARE_FULL                  (16)

/** @brief A piece of the lens flare drawn */
typedef struct
{
    short                           place;  // On the line from the middle of the screen (0) to the sun (256); < 0: beyond the middle
    short                           size;   // Its scale at the full level of the flare (1/8 of the sprite)
    short                           piece;  // Its sprite (0 to JO_SUN_FLARE_PIECES - 1: jo_sun.flare[])
}                                   jo_sun_flare;

/** @brief The sun (see jo_sun_init(): the default values) */
typedef struct
{
    jo_angle                        azimuth;        // Its direction: the heading toward it (as the heading of the camera)
    jo_angle                        elevation;      // Above the horizon
    int                             halo[JO_SUN_MAX_HALO_STRIPS];  // Sprites of the halo, from the top
    int                             halo_strips;    // Strips of the halo (0: no halo)
    int                             halo_half;      // Half the size of the halo (pixels: 64)
    int                             strip_height;   // Rows of a strip (pixels: 32)
    int                             core;           // Sprite of the core (-1: none)
    int                             core_half;      // Half the size of the core (pixels: 16)
    int                             flare[JO_SUN_FLARE_PIECES];        // Sprites of the lens flare (-1: none)
    int                             flare_rgb[JO_SUN_FLARE_PIECES];    // Under the horizon: drawn half transparent (-1: flare[])
    const jo_sun_flare              *flares;        // The pieces drawn (default: 7 pieces)
    int                             flare_count;
    int                             z;              // Depth of the sun (sprites) and its depth bias (behind everything)
    int                             bias;
    int                             flare_z;        // Depth of the lens flare and its depth bias (in front of everything)
    int                             flare_bias;
    /* Computed by jo_sun_update() */
    bool                            seen;           // On the screen
    int                             x;              // Its middle on the screen (pixels)
    int                             y;
    int                             horizon_y;      // The horizon on the screen (pixels)
    int                             flare_level;    // The lens flare: 0 to JO_SUN_FLARE_FULL
}                                   jo_sun;

/** @brief The default values of a sun: no sprite, the default lens flare
 *  @param sun Sun
 *  @param azimuth The heading toward it
 *  @param elevation Above the horizon
 */
void                                jo_sun_init(jo_sun * const sun, const jo_angle azimuth, const jo_angle elevation);

/** @brief Place the sun on the screen, fade the lens flare (once a frame)
 *  @param sun Sun
 *  @param heading Heading of the camera
 *  @param look_down How much the camera looks down (the tangent of the angle, jo_fixed)
 *  @param hidden Something between the camera and the sun (no lens flare)? Called only when the lens flare could be seen
 *  (JO_NULL: never hidden)
 */
void                                jo_sun_update(jo_sun * const sun, const jo_angle heading, const jo_fixed look_down,
                                                  bool (*hidden)(void));

/** @brief Draw the sun (in the frame of the 3D: behind everything) */
void                                jo_sun_draw(const jo_sun * const sun);

/** @brief Draw the lens flare (in front of the rest: before the HUD) */
void                                jo_sun_draw_flare(const jo_sun * const sun);

#endif /* JO_COMPILE_WITH_3D_SUPPORT */

#endif /* !__JO_SUN_H__ */

/*
** END OF FILE
*/
