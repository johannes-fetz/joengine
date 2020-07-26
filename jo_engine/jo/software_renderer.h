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
/** @file software_renderer.h
 *  @author Johannes Fetz
 *
 *  @brief Fast Triangle based software renderer using fixed number
 *  @bug No known bugs.
 *  @todo Lights, Texture mapping, 8 bits mode, etc.
 */

#ifndef __JO_SOFTWARE_RENDERER_H__
# define __JO_SOFTWARE_RENDERER_H__

#ifdef JO_COMPILE_WITH_SOFTWARE_RENDERER_SUPPORT

/*
 ██████╗ ██████╗ ██████╗ ███████╗
██╔════╝██╔═══██╗██╔══██╗██╔════╝
██║     ██║   ██║██████╔╝█████╗
██║     ██║   ██║██╔══██╗██╔══╝
╚██████╗╚██████╔╝██║  ██║███████╗
 ╚═════╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝
*/

/*
   Jo Engine Software Rendering XYZ axes

           Z-
          /
         /
        0------> X+ (width)
       /|
      / |
     Z+ V
        Y+ (height)
*/

/** @brief Z-BUFFER Computation mode */
typedef enum
{
    JO_SR_DEPTH_IGNORE                      = (1 << 0),
    JO_SR_DEPTH_LESS                        = (1 << 1),
    JO_SR_DEPTH_LESS_OR_EQUAL               = (1 << 2),
    JO_SR_DEPTH_GREATER_OR_EQUAL            = (1 << 3),
    JO_SR_DEPTH_GREATER                     = (1 << 4),
    JO_SR_DEPTH_NOT_EQUAL                   = (1 << 5),
    JO_SR_DEPTH_EQUAL                       = (1 << 6)
}                                           jo_software_renderer_depth_mode;

/** @brief This is the main struct that handle everything for drawing */
typedef struct
{
    jo_size                                 clipping_size;
    int                                     sprite_id;
    jo_fixed                                *depth_buffer;
    unsigned int                            depth_buffer_dword_size;
    jo_color                                *color_buffer;
    unsigned int                            color_buffer_size;
    void                                    *vram;
    jo_size                                 vram_size;
    jo_software_renderer_depth_mode         depth_mode_testing;
}                                           jo_software_renderer_gfx;

/** @brief Create a full-color rendering surface
 *  @param width Surface width
 *  @param height Surface height
 *  @param screen Drawing destination. Only JO_SPRITE_SCREEN and JO_NBG1_SCREEN are supported today
 *  @return Software Rendering Graphics
 */
jo_software_renderer_gfx                    *jo_software_renderer_create(unsigned short width, unsigned short height, const jo_scroll_screen screen);

/** @brief Free the rendering surface
 *  @param gfx Software Rendering Graphics
 *  @warning This doesn't free the sprite behind (JO_SPRITE_SCREEN) but you can replace it with jo_sprite_replace()
 */
void                                        jo_software_renderer_free(jo_software_renderer_gfx * const gfx);

/** @brief Copy the buffer into VDP1/2 VRAM
 *  @param gfx Software Rendering Graphics
 */
void                                        jo_software_renderer_flush(jo_software_renderer_gfx * const gfx);

/*
██████╗  █████╗ ███████╗██╗ ██████╗    ██████╗ ██████╗  █████╗ ██╗    ██╗██╗███╗   ██╗ ██████╗
██╔══██╗██╔══██╗██╔════╝██║██╔════╝    ██╔══██╗██╔══██╗██╔══██╗██║    ██║██║████╗  ██║██╔════╝
██████╔╝███████║███████╗██║██║         ██║  ██║██████╔╝███████║██║ █╗ ██║██║██╔██╗ ██║██║  ███╗
██╔══██╗██╔══██║╚════██║██║██║         ██║  ██║██╔══██╗██╔══██║██║███╗██║██║██║╚██╗██║██║   ██║
██████╔╝██║  ██║███████║██║╚██████╗    ██████╔╝██║  ██║██║  ██║╚███╔███╔╝██║██║ ╚████║╚██████╔╝
╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝ ╚═════╝    ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝ ╚══╝╚══╝ ╚═╝╚═╝  ╚═══╝ ╚═════╝
*/

/** @brief Clear the buffer with the given color
 *  @param gfx Software Rendering Graphics
 *  @param color Color
 */
void                                        jo_software_renderer_clear(jo_software_renderer_gfx * const gfx, const jo_color color);

/** @brief Draw a pixel without handling the Z-Buffer
 *  @param gfx Software Rendering Graphics
 *  @param x X Fixed position
 *  @param y Y Fixed position
 *  @param color Pixel color
 */
void                                        jo_software_renderer_draw_pixel2D(jo_software_renderer_gfx * const gfx, const jo_fixed x, const jo_fixed y, const jo_color color);

/** @brief Draw a pixel
 *  @param gfx Software Rendering Graphics
 *  @param x X Fixed position
 *  @param y Y Fixed position
 *  @param z Z Fixed position
 *  @param color Pixel color
 */
void                                        jo_software_renderer_draw_pixel3D(jo_software_renderer_gfx * const gfx, const jo_fixed x, const jo_fixed y, const jo_fixed z, const jo_color color);

/** @brief Draw a line
 *  @param gfx Software Rendering Graphics
 *  @param x0 X0 Fixed position (point A)
 *  @param y0 Y0 Fixed position (point A)
 *  @param z0 Z0 Fixed position (point A)
 *  @param x1 X2 Fixed position (point B)
 *  @param y1 Y2 Fixed position (point B)
 *  @param z1 Z2 Fixed position (point B)
 *  @param color0 First color (point A)
 *  @param color1 Second color (point B)
 */
void                                        jo_software_renderer_draw_line3D(jo_software_renderer_gfx * const gfx,
                                                                           jo_fixed x0, jo_fixed y0, jo_fixed z0,
                                                                           jo_fixed x1, jo_fixed y1, jo_fixed z1,
                                                                           const jo_color color0, const jo_color color1);

/*
████████╗██████╗ ██╗ █████╗ ███╗   ██╗ ██████╗ ██╗     ███████╗
╚══██╔══╝██╔══██╗██║██╔══██╗████╗  ██║██╔════╝ ██║     ██╔════╝
   ██║   ██████╔╝██║███████║██╔██╗ ██║██║  ███╗██║     █████╗
   ██║   ██╔══██╗██║██╔══██║██║╚██╗██║██║   ██║██║     ██╔══╝
   ██║   ██║  ██║██║██║  ██║██║ ╚████║╚██████╔╝███████╗███████╗
   ╚═╝   ╚═╝  ╚═╝╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚══════╝╚══════╝

    A ___ C
     \   /
      \ /
       B
*/

/** @brief Triangle vertice definition */
typedef struct
{
    jo_color                                color;
    jo_vector4_fixed                        pos;
}                                           jo_software_renderer_triangle_vertice;

/** @brief Triangle definition */
typedef struct
{
    jo_software_renderer_triangle_vertice   a;
    jo_software_renderer_triangle_vertice   b;
    jo_software_renderer_triangle_vertice   c;
}                                           jo_software_renderer_triangle;

/** @brief Draw a triangle in wireframe
 *  @param gfx Software Rendering Graphics
 *  @param triangle Triangle
 */
static __jo_force_inline void               jo_software_renderer_draw_triangle_wireframe(jo_software_renderer_gfx * const gfx, const jo_software_renderer_triangle * const triangle)
{
    jo_software_renderer_draw_line3D(gfx,
                                     triangle->a.pos.x, triangle->a.pos.y, triangle->a.pos.z,
                                     triangle->b.pos.x, triangle->b.pos.y, triangle->b.pos.z,
                                     triangle->a.color, triangle->b.color);
    jo_software_renderer_draw_line3D(gfx,
                                     triangle->b.pos.x, triangle->b.pos.y, triangle->b.pos.z,
                                     triangle->c.pos.x, triangle->c.pos.y, triangle->c.pos.z,
                                     triangle->b.color, triangle->c.color);
    jo_software_renderer_draw_line3D(gfx,
                                     triangle->c.pos.x, triangle->c.pos.y, triangle->c.pos.z,
                                     triangle->a.pos.x, triangle->a.pos.y, triangle->a.pos.z,
                                     triangle->c.color, triangle->a.color);
}

#endif // JO_COMPILE_WITH_SOFTWARE_RENDERER_SUPPORT

#endif /* !__JO_SOFTWARE_RENDERER_H__ */

/*
** END OF FILE
*/
