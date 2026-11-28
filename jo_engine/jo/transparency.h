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
/** @file transparency.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine transparency layer: real transparency for 3D polygons and sprites, mixed once with everything
 *  behind them (the 3D of the VDP1 included), like Burning Rangers
 *
 *  - The VDP1 half transparency reads the frame buffer and draws lines that overlap (stripes, slow), the mesh is a
 *    checkerboard, the VDP2 mixes a sprite with the VDP2 screens under it only (not with the 3D behind it). Here the
 *    transparent objects are drawn by the VDP1 at a lower resolution in the part of the frame buffer that is not
 *    displayed (x 352 to 511 by default), copied into a 32768 colors bitmap of NBG1 or NBG0 and shown by the VDP2 zoomed
 *    to the screen, above the sprites, mixed with them
 *  - Everything that the VDP1 draws works in the layer: textures, gouraud, colors, billboards
 *    (jo_3d_draw_scaled_billboard()). The opacity is the same for all the pixels of the layer (one ratio, see
 *    jo_3d_transparency_set_blend()). For gradients of opacity (halo, glow, fire, light beams), the additive mode: a darker
 *    color is a weaker effect, black is none
 *  - The opaque objects that can be in front of a transparent object are drawn again in the layer as masks
 *    (jo_3d_transparency_mask()): they erase it there
 *  - The copy is done by the SCU DMA (levels 1 and 2): into the work RAM as soon as the VDP1 has finished the frame,
 *    then into the VDP2 at the VBLANK where the frame is displayed (faster than the beam). The CPUs don't wait. A frame
 *    without transparent object: the layer is emptied, nothing is drawn
 *  - Resolution: the layer is 160 pixels wide (steps of 2 pixels on its edges in 320 x 240) or up to 192 (the whole width
 *    free in the frame buffer beside a screen of 320 pixels: steps of 1.7 pixels), see jo_3d_transparency_init(). Screens
 *    of 320 or 352 pixels, 224 to 256 lines (not the high resolution, not interlaced)
 *  - Needs the manual change of the frame buffers: a frame rate of 2 or more (JO_FRAMERATE > 1 in the makefile, or
 *    jo_core_set_framerate(2) while the program runs) or JO_FRAMERATE_DYNAMIC in the makefile. Uses a
 *    VDP2 screen (NBG1 or NBG0: see jo_3d_transparency_init()) with a 32768 colors bitmap at the start of a
 *    free bank of the VRAM (NBG1: its bitmap of the banks A0 and A1 if free), its color calculation, and the levels 1 and
 *    2 of the SCU DMA
 *  - The blend mode (ratio or additive) is the same for all the VDP2 screens (see jo_vdp2_set_blend_mode())
 *  - Not with the render to image (see jo/render_to_image.h): they use the same part of the frame buffer
 *
 *  Usage:
 *  @code
 *  jo_3d_transparency_init(0, JO_NBG1_SCREEN);           // once, after jo_core_init()
 *  // each frame, after the opaque 3D:
 *  jo_3d_transparency_begin();
 *  jo_3d_mesh_draw(water);                               // the transparent objects
 *  jo_3d_transparency_mask(column);                      // the opaque objects that can be in front of them
 *  jo_3d_transparency_end();
 *  @endcode
 */

#ifndef __JO_TRANSPARENCY_H__
# define __JO_TRANSPARENCY_H__

#ifdef JO_COMPILE_WITH_3D_SUPPORT

/** @brief Default width of the layer in the frame buffer (pixels): x 352 to 511 */
# define JO_3D_TRANSPARENCY_DEFAULT_WIDTH   (160)
/** @brief Maximum width of the layer: the whole free width of the frame buffer (x 320 to 511) */
# define JO_3D_TRANSPARENCY_MAX_WIDTH       (192)
/** @brief Maximum lines of the layer */
# define JO_3D_TRANSPARENCY_MAX_HEIGHT      (160)
/** @brief Default blend ratio (JO_VDP2_BLEND_RATIO mode): 0 opaque to 31 almost invisible, 16 half and half */
# define JO_3D_TRANSPARENCY_DEFAULT_RATIO   (14)

/** @brief Start the transparency layer (or change its width or its screen)
 *  @param width Width of the layer in the frame buffer: 0 (JO_3D_TRANSPARENCY_DEFAULT_WIDTH) to
 *  JO_3D_TRANSPARENCY_MAX_WIDTH, at most the free width beside the screen (192 pixels in 320, 160 in 352). Wider: finer
 *  steps on its edges, a longer copy (160 x 120: 38 KB, 192 x 143: 55 KB)
 *  @param screen VDP2 screen of the layer: JO_NBG1_SCREEN or JO_NBG0_SCREEN. It must not show an image (its cells or its
 *  map: the layer is refused). NBG0 with the text of jo_printf(): the text is hidden while the layer runs, given back
 *  (empty) by jo_3d_transparency_stop(). Its bitmap takes a free bank of the VRAM (A0, A1 or B0; 120 KB in 320 x 240 at
 *  the width 160)
 *  @return false if it can't run (an error in JO_DEBUG): automatic change of the frame buffers (a frame rate of 1), a
 *  resolution that is not compatible (640 or 704 pixels: the frame buffer has 8 bits per pixel; interlaced, more than 256
 *  lines), screen not available (it shows an image, or no free bank of the VRAM), out of high work RAM (one block: the
 *  copy of the layer and the tables of the SCU DMA, 41 KB at 160 x 120, 59 KB at 192 x 144; given back by
 *  jo_3d_transparency_stop())
 *  @remarks Resolutions: 320 or 352 x 224, 240 or 256. Default blend: JO_VDP2_BLEND_RATIO,
 *  JO_3D_TRANSPARENCY_DEFAULT_RATIO. The screen of the layer is set above the sprites. After a change of the resolution
 *  of the screen, the layer follows it at the next jo_3d_transparency_begin() (it stops if the new resolution is not
 *  compatible)
 */
bool    jo_3d_transparency_init(const int width, const jo_scroll_screen screen);

/** @brief Stop the transparency layer: its bitmap given back (NBG0: the text of jo_printf() again), the SCU DMA levels 1
 *  and 2 free, the memory given back, the drawing of the VDP1 clipped to the screen again (not to the frame buffer) */
void    jo_3d_transparency_stop(void);

/** @brief How the layer is mixed with the image under it
 *  @param mode JO_VDP2_BLEND_RATIO (transparency: water, glass, smoke) or JO_VDP2_BLEND_ADD (light: halo, glow, fire;
 *  black adds nothing). The same for all the VDP2 screens
 *  @param ratio JO_VDP2_BLEND_RATIO: 0 (opaque) to 31 (almost invisible), 16 half and half
 */
void    jo_3d_transparency_set_blend(const jo_vdp2_blend_mode mode, const unsigned char ratio);

/** @brief Start drawing in the layer: the next polygons and billboards of the master CPU go into it (until
 *  jo_3d_transparency_end()). The first call of a frame clears the layer
 *  @remarks Draw the transparent objects and the masks with the same camera as the rest of the frame. Selects the far 3D
 *  window of the layer (JO_3D_LAYER_WINDOW, clipped to the layer: the sprites too) and scales the projection;
 *  jo_3d_transparency_end() selects the window selected before again (the 3D of the screen must not be drawn into the
 *  layer)
 */
void    jo_3d_transparency_begin(void);

/** @brief End drawing in the layer (see jo_3d_transparency_begin())
 */
void    jo_3d_transparency_end(void);

/** @brief Clip the next drawings in the layer to a rectangle of the screen (between jo_3d_transparency_begin() and
 *  jo_3d_transparency_end()): an object seen through a door only in the door (see jo_cells_view_rect_at())
 *  @param rect Rectangle of the screen (pixels, the edges in it), JO_NULL: the whole layer (the default at each
 *  jo_3d_transparency_begin())
 *  @remarks A change of the rectangle after drawings takes one of the changes of a window in a frame of the VDP1 (8):
 *  group the objects by rectangle
 */
void    jo_3d_transparency_set_clip(const jo_3d_rect * const rect);

/** @brief Draw an opaque mesh as a mask in the layer (between jo_3d_transparency_begin() and jo_3d_transparency_end()):
 *  its polygons erase the transparent objects behind them (sorted with them by depth)
 *  @param mesh Opaque mesh, drawn with the same matrix as on the screen (its whole polygons: a transparent texel of its
 *  texture masks too)
 */
void    jo_3d_transparency_mask(jo_3d_mesh * const mesh);

/** @brief Start drawing masks in the layer (between jo_3d_transparency_begin() and jo_3d_transparency_end()): the meshes
 *  and the models drawn by the master CPU until jo_3d_transparency_mask_end() (posed models and characters included, see
 *  jo_3d_model_draw_pose()) erase what is behind them in the layer, sorted with it by depth: an opaque object in front
 *  of a transparent one (a character in front of a window, the weapon of a first person game in front of a glow). The
 *  16 colors sprites (jo_sprite_add_4bits_lut()) too: their pixels (a sprite of another format is drawn as it is)
 *  @remarks Draw them with the same camera, depth bias and pose as on the screen. A mask costs its polygons again (the
 *  VDP1 draws them in the layer, small): only the objects that can be in front of the transparent ones
 */
void    jo_3d_transparency_mask_begin(void);

/** @brief End drawing masks (see jo_3d_transparency_mask_begin()); jo_3d_transparency_end() ends them too */
void    jo_3d_transparency_mask_end(void);

/** @brief Something is drawn in the layer in this frame (jo_3d_transparency_begin() was called): the masks of the opaque
 *  objects are needed only then
 *  @return true if the layer is used in the frame being built
 */
bool    jo_3d_transparency_is_used(void);

/** @brief The layer is running (see jo_3d_transparency_init())
 *  @return true if running
 */
bool    jo_3d_transparency_is_running(void);

/** @brief Size of the layer in the frame buffer (see jo_3d_transparency_init())
 *  @param width Pixels (out, can be JO_NULL)
 *  @param height Lines (out, can be JO_NULL)
 */
void    jo_3d_transparency_get_size(int * const width, int * const height);

/** @brief Scale of the layer: its pixels for a pixel of the screen (0.5 for a layer of 160 pixels beside a screen of 320)
 *  @return Scale (Q16)
 *  @remarks A sprite drawn in the layer in 2D (centered coordinates, between jo_3d_transparency_begin() and
 *  jo_3d_transparency_end()): its position and its size on the screen x the scale
 */
jo_fixed    jo_3d_transparency_get_scale(void);

#endif /* !JO_COMPILE_WITH_3D_SUPPORT */

#endif /* !__JO_TRANSPARENCY_H__ */

/*
** END OF FILE
*/
