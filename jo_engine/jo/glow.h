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
/** @file glow.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine glows: the halo of light around a lamp (a soft disc of its color, without edge, with or without a
 *  few thin rays of light; a lit rectangle; the outline of a rectangle, a neon), added to the image
 *
 *  - The VDP1 blends at 50 % only: a half transparent sprite shows its edge (and darkens the dark walls around it). The
 *    glows are drawn by the VDP2 instead: NBG2 over the sprite layer, with the color calculation "add": the color of a
 *    glow is added to the 3D view (0 at its edge: no edge, like the light of a real lamp in a camera)
 *  - Up to 15 glows on the screen at the same time (the brightest seen; a glow fades in and out), each one its size, its
 *    color and its shape (see jo_3d_glow_set_shape()): the map of the layer is a canvas, each glow puts the cells of its
 *    shape there (16 to 128 pixels, 15 levels of light) with its palette; where 2 glows overlap their light is added.
 *    The cells are computed once (no CPU time per pixel but for the overlaps); a glow is placed within 2 to 8 pixels,
 *    the brightest one to the pixel (the scroll of the layer). NBG3 stays free
 *  - The layers change when the image of the frame is displayed (not when the frame is computed): the glow stays on its
 *    lamp when the camera turns
 *  - They use the VRAM bank B0 of the VDP2: the rotation screen (the 3D planes, see background.h) is hidden while the
 *    glows are on (for the places where the 3D planes are not seen: inside a building, underground...)
 *  - A wall between the camera and a lamp hides its glow (see jo_3d_glows_set_occlusion())
 *
 *  Usage: jo_3d_glows_enable(true), jo_3d_glow_add() for each lamp, then jo_3d_glows_draw() at each frame after the
 *  camera (its matrix is the current matrix)
 *  @bug No known bugs.
 */

#ifndef __JO_GLOW_H__
# define __JO_GLOW_H__

#ifdef JO_COMPILE_WITH_3D_SUPPORT

/** @brief Max glows (lamps) */
# define JO_3D_GLOW_MAX                     (48)

/** @brief Shapes of the glows (see jo_3d_glow_set_shape())
 */
typedef enum
{
    JO_3D_GLOW_STAR = 0,        // A soft disc and 8 thin rays (see jo_3d_glows_set_rays()): the default
    JO_3D_GLOW_DISC = 1,        // A soft disc, no rays (a car light, a small lamp)
    JO_3D_GLOW_RECT = 2,        // A lit rectangle, its light falling around it (a sign, a window)
    JO_3D_GLOW_FRAME = 3        // The outline of a rectangle, its light on both sides of the line (a neon tube)
}                                   jo_3d_glow_shape;

/** @brief Add a glow
 *  @param position Position (world)
 *  @param r Red (0 to 255): the color added at its middle with the intensity 256
 *  @param g Green
 *  @param b Blue
 *  @param radius Radius (world units): its size on the screen changes with its distance (16 to 128 pixels)
 *  @param intensity Intensity (0: off, 256: its color)
 *  @return Id of the glow, -1 if there are too many
 */
int                                 jo_3d_glow_add(const jo_pos3D_fixed * const position, const unsigned char r, const unsigned char g,
                                                   const unsigned char b, const int radius, const int intensity);

/** @brief Change the shape of a glow
 *  @param id Id of the glow
 *  @param shape Shape (JO_3D_GLOW_STAR when it is added)
 *  @param width Width of the rectangle (RECT, FRAME: world units, facing the camera; its radius is the width of the light
 *         around it, 8, 16 or 24 pixels on the screen)
 *  @param height Height of the rectangle
 *  @remarks A rectangle is placed on the cells of the layer (8 pixels), the discs within 2 to 4 pixels (the glow seen the
 *           most to the pixel)
 */
void                                jo_3d_glow_set_shape(const int id, const jo_3d_glow_shape shape, const int width, const int height);

/** @brief Place a glow to the pixel: the scroll of the layer puts the grid of the cells on it (the glow seen the most
 *  otherwise); for a rectangle that must stay on its lamps (the rear lamps of a car: a rectangle is placed on the
 *  cells, 8 pixels, it jumped from one cell to the next). The first anchored glow seen wins
 *  @param id Id of the glow
 *  @param anchor true: placed to the pixel
 */
void                                jo_3d_glow_set_anchor(const int id, const bool anchor);

/** @brief Change the size of a glow
 *  @param id Id of the glow
 *  @param radius Radius (world units, see jo_3d_glow_add()); a rectangle: the width of the light around it
 */
void                                jo_3d_glow_set_radius(const int id, const int radius);

/** @brief Change the intensity of a glow (a lamp that flickers, an alarm)
 *  @param id Id of the glow
 *  @param intensity Intensity (0: off, 256: its color)
 */
void                                jo_3d_glow_set_intensity(const int id, const int intensity);

/** @brief Move a glow
 *  @param id Id of the glow
 *  @param position Position (world)
 */
void                                jo_3d_glow_set_position(const int id, const jo_pos3D_fixed * const position);

/** @brief Change the color of a glow
 *  @param id Id of the glow
 *  @param r Red (0 to 255)
 *  @param g Green
 *  @param b Blue
 */
void                                jo_3d_glow_set_color(const int id, const unsigned char r, const unsigned char g, const unsigned char b);

/** @brief Remove all the glows */
void                                jo_3d_glows_clear(void);

/** @brief The light of the rays of the glows (the star around the disc), before jo_3d_glows_enable() (its characters
 *  are built then)
 *  @param light Light of the 4 rays on the axes (0 to 256, in the middle of the glow; 150 by default, 0: no rays)
 *  @param diagonal_light Light of the 4 shorter rays on the diagonals (100 by default)
 */
void                                jo_3d_glows_set_rays(const int light, const int diagonal_light);

/** @brief Turn the glows on or off: the VDP2 layers (see the remarks of this file: the 3D planes are hidden while they are on)
 *  @param enabled true: the glows are displayed (the first call builds their characters in the VRAM)
 *  @return false if the VDP2 memory is full (the glows stay off)
 */
bool                                jo_3d_glows_enable(const bool enabled);

/** @brief The walls hide the glows behind them (a ray from the camera to the lamp, a few each frame)
 *  @param hit The ray from origin along dir (unit vector) hits something before max_t (Q16.16), JO_NULL: always seen
 */
void                                jo_3d_glows_set_occlusion(bool (*hit)(const jo_pos3D_fixed * const origin, const jo_pos3D_fixed * const dir,
                                                                          const jo_fixed max_t));

/** @brief The glows are seen only in a rectangle of the screen: the rectangle where the place of the lamp is seen (a lamp
 *  in a room seen through a door: its glow stays in the door, see jo_cells_view_rect_at()). The cells of the glow out of
 *  the rectangle are not shown (to 8 pixels)
 *  @param clip Gives the rectangle where a point is seen, false if it is not seen; JO_NULL: the whole screen
 */
void                                jo_3d_glows_set_clip(bool (*clip)(const jo_pos3D_fixed * const position, jo_3d_rect * const rect));

/** @brief Place the glows seen from the camera (once per frame, after the camera: its matrix is the current matrix)
 *  @param camera Position of the camera (world): the rays of the occlusion
 *  @remarks The layers change when the image of this frame is displayed
 */
void                                jo_3d_glows_draw(const jo_pos3D_fixed * const camera);

#endif /* !JO_COMPILE_WITH_3D_SUPPORT */

#endif /* !__JO_GLOW_H__ */

/*
** END OF FILE
*/
