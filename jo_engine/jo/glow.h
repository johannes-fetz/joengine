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
 *  @brief Jo Engine glows: the halo of light around a lamp (a soft disc of its color, without edge, and a few thin rays
 *  of light, added to the image)
 *
 *  - The VDP1 blends at 50 % only: a half transparent sprite shows its edge (and darkens the dark walls around it). The
 *    glows are drawn by the VDP2 instead: NBG2 and NBG3 over the sprite layer, with the color calculation "add": the
 *    color of a glow is added to the 3D view (0 at its edge: no edge, like the light of a real lamp in a camera)
 *  - 2 glows on the screen at the same time (the 2 brightest seen; a glow fades in and out). Each layer shows one disc
 *    (16 to 128 pixels, 15 levels of light), moved by its scroll and colored by its palette: no CPU time per pixel
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
 *  in a room seen through a door: its glow stays in the door, see jo_cells_view_rect_at()). A window of the VDP2 cuts the
 *  disc to the rectangle
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
