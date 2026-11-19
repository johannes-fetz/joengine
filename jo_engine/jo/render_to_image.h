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
/** @file render_to_image.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine render to image: the VDP1 draws 3D polygons and sprites into an image instead of the screen (a
 *  monitor, a rear-view mirror, a portrait in a menu, a texture made at run time)
 *
 *  - The image is a rectangle of the frame buffer that is not displayed (its right: x 352 to 511 for an image of 160
 *    pixels), drawn by the off screen 3D window with the same camera and the same projection as the screen (a pixel of
 *    the image is a pixel of the screen)
 *  - When the VDP1 has finished a frame, the level 1 of the SCU DMA copies the rectangle into the image (high work RAM:
 *    the CPUs don't wait); at each VBLANK the image goes into the texture of its sprite (see
 *    jo_3d_render_to_image_add_sprite()). The texture shows the image of the frame drawn before
 *  - The background of the image is transparent (color 0), cleared at the first jo_3d_render_to_image_begin() of each
 *    frame. A frame without jo_3d_render_to_image_begin(): the image keeps what was drawn before
 *  - Screens of 320 or 352 pixels, 224 to 256 lines (not the high resolution, not interlaced). Not with the transparency
 *    layer (see jo/transparency.h): they use the same part of the frame buffer and the same level of the SCU DMA
 *
 *  Usage:
 *  @code
 *  jo_3d_render_to_image_init(160, 112);                 // once, after jo_core_init()
 *  monitor = jo_3d_render_to_image_add_sprite();         // the texture that shows the image
 *  jo_3d_set_mesh_texture(screen_mesh, monitor);
 *  // each frame:
 *  jo_3d_render_to_image_begin();
 *  jo_3d_mesh_draw(robot);                               // drawn into the image
 *  jo_3d_render_to_image_end();
 *  jo_3d_mesh_draw(screen_mesh);                         // drawn on the screen, with the image as texture
 *  @endcode
 */

#ifndef __JO_RENDER_TO_IMAGE_H__
# define __JO_RENDER_TO_IMAGE_H__

#ifdef JO_COMPILE_WITH_3D_SUPPORT

/** @brief Start the render to image (or change its size)
 *  @param width Width of the image (pixels, even): at most the free width of the frame buffer beside the screen (192
 *  in 320, 160 in 352)
 *  @param height Height of the image (lines): at most the height of the screen
 *  @return false if it can't run (an error in JO_DEBUG): size not possible, high resolution or interlaced screen, the
 *  transparency layer running, out of high work RAM (the image and the table of the SCU DMA: 36 KB for 160 x 112)
 *  @remarks The VDP1 draws up to the right of the frame buffer (system clipping), the windows of the screen stay on the
 *  screen. A size changed: the sprite of jo_3d_render_to_image_add_sprite() is forgotten (add a new one)
 */
bool            jo_3d_render_to_image_init(const int width, const int height);

/** @brief Add a sprite of the size of the image: its texture receives the image at each VBLANK
 *  @return Sprite Id (the same one at each call), -1 if the render to image is not running
 *  @remarks A copy of the image per VBLANK (width x height x 2 bytes, by the DMA). Free it after
 *  jo_3d_render_to_image_stop()
 */
int             jo_3d_render_to_image_add_sprite(void);

/** @brief The image (32768 colors, the background transparent): the frame drawn before, updated when the VDP1 finishes a
 *  frame (show it on NBG1 with jo_vdp2_set_nbg1_image(), read its pixels)
 *  @return The image, JO_NULL if the render to image is not running
 */
const jo_img    *jo_3d_render_to_image_get(void);

/** @brief Start drawing into the image: the next polygons and sprites of the master CPU go into it (until
 *  jo_3d_render_to_image_end()). The first call of a frame clears the image
 *  @remarks Selects the off screen 3D window (JO_3D_LAYER_WINDOW, clipped to the image, the sprites too), the center of
 *  the projection at the center of the image; jo_3d_render_to_image_end() selects the window selected before again
 */
void            jo_3d_render_to_image_begin(void);

/** @brief End drawing into the image (see jo_3d_render_to_image_begin()) */
void            jo_3d_render_to_image_end(void);

/** @brief Stop the render to image: the memory and the level 1 of the SCU DMA given back, the drawing of the VDP1 clipped
 *  to the screen again (the sprite of jo_3d_render_to_image_add_sprite() stays: free it if needed) */
void            jo_3d_render_to_image_stop(void);

#endif /* !JO_COMPILE_WITH_3D_SUPPORT */

#endif /* !__JO_RENDER_TO_IMAGE_H__ */

/*
** END OF FILE
*/
