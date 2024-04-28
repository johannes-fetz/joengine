/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2024, Johannes Fetz (johannesfetz@gmail.com)
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
/** @file background.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Background definition and tools
 *  @warning ▲ IF YOUR ARE FAMILIAR WITH SEGA SATURN HARDWARE, USE vdp2.h FUNCTIONS INSTEAD ▲
 *  @warning IT'S JUST A WRAPPER FOR THOSE WHO DON'T CARE ABOUT SCREEN SCROLL AND VDP2 USAGE
 *  @bug No known bugs.
 */

#ifndef __JO_BACKGROUND_H__
# define __JO_BACKGROUND_H__

/*
██████╗ ██████╗  █████╗ ██╗    ██╗    ██╗███╗   ███╗ █████╗  ██████╗ ███████╗
██╔══██╗██╔══██╗██╔══██╗██║    ██║    ██║████╗ ████║██╔══██╗██╔════╝ ██╔════╝
██║  ██║██████╔╝███████║██║ █╗ ██║    ██║██╔████╔██║███████║██║  ███╗█████╗
██║  ██║██╔══██╗██╔══██║██║███╗██║    ██║██║╚██╔╝██║██╔══██║██║   ██║██╔══╝
██████╔╝██║  ██║██║  ██║╚███╔███╔╝    ██║██║ ╚═╝ ██║██║  ██║╚██████╔╝███████╗
╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝ ╚══╝╚══╝     ╚═╝╚═╝     ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝

*/

/** @brief Add 8 bits background sprite
 *  @param img 8 bits 255 colors max image. (Width AND height must be a multiple of 8)
 *  @param palette_id palette id from TGA (see also jo_palette)
 *  @param vertical_flip Flip image vertically
 *  @warning Image need to be clockwised rotated (right) because of an optimisation
 */
static  __jo_force_inline void    jo_set_background_8bits_sprite(jo_img_8bits *img, int palette_id, bool vertical_flip)
{
    jo_vdp2_set_nbg1_8bits_image(img, palette_id, vertical_flip);
}

/** @brief Add background sprite
 *  @param img Pointer to an image struct
 *  @param left Left location
 *  @param top Top location
 */
static  __jo_force_inline void	jo_set_background_sprite(const jo_img * const img, const unsigned short left, const unsigned short top)
{
    jo_vdp2_set_nbg1_image(img, left, top);
}

/** @brief Clear the background
 *  @param color Clear color
 */
static  __jo_force_inline void    jo_clear_background(const jo_color color)
{
    jo_vdp2_clear_bitmap_nbg1(color);
}

/*
███╗   ███╗ ██████╗ ██╗   ██╗███████╗       ██╗       ███████╗ ██████╗  ██████╗ ███╗   ███╗
████╗ ████║██╔═══██╗██║   ██║██╔════╝       ██║       ╚══███╔╝██╔═══██╗██╔═══██╗████╗ ████║
██╔████╔██║██║   ██║██║   ██║█████╗      ████████╗      ███╔╝ ██║   ██║██║   ██║██╔████╔██║
██║╚██╔╝██║██║   ██║╚██╗ ██╔╝██╔══╝      ██╔═██╔═╝     ███╔╝  ██║   ██║██║   ██║██║╚██╔╝██║
██║ ╚═╝ ██║╚██████╔╝ ╚████╔╝ ███████╗    ██████║      ███████╗╚██████╔╝╚██████╔╝██║ ╚═╝ ██║
╚═╝     ╚═╝ ╚═════╝   ╚═══╝  ╚══════╝    ╚═════╝      ╚══════╝ ╚═════╝  ╚═════╝ ╚═╝     ╚═╝

*/

/** @brief Move background (scrolling)
 *  @param x horizontal location
 *  @param y vertical location
 */
static  __jo_force_inline void	jo_move_background(const int x, const int y)
{
    jo_vdp2_move_nbg1(x, y);
}

/** @brief Zoom background width and height independently
 *  @param width_factor Width zoom factor
 *  @param height_factor Height zoom factor
 */
static  __jo_force_inline void	jo_zoom_background2(const float width_factor, const float height_factor)
{
    jo_vdp2_zoom_nbg1f(width_factor, height_factor);
}

/** @brief Zoom background
 *  @param factor Zoom factor
 */
static  __jo_force_inline void	jo_zoom_background(const float factor)
{
    jo_vdp2_zoom_nbg1f(factor, factor);
}

/*
██████╗  █████╗ ███████╗██╗ ██████╗    ██████╗ ██████╗  █████╗ ██╗    ██╗██╗███╗   ██╗ ██████╗
██╔══██╗██╔══██╗██╔════╝██║██╔════╝    ██╔══██╗██╔══██╗██╔══██╗██║    ██║██║████╗  ██║██╔════╝
██████╔╝███████║███████╗██║██║         ██║  ██║██████╔╝███████║██║ █╗ ██║██║██╔██╗ ██║██║  ███╗
██╔══██╗██╔══██║╚════██║██║██║         ██║  ██║██╔══██╗██╔══██║██║███╗██║██║██║╚██╗██║██║   ██║
██████╔╝██║  ██║███████║██║╚██████╗    ██████╔╝██║  ██║██║  ██║╚███╔███╔╝██║██║ ╚████║╚██████╔╝
╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝ ╚═════╝    ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝ ╚══╝╚══╝ ╚═╝╚═╝  ╚═══╝ ╚═════╝
*
** ▲ ONLY WORKS IN BITMAP MODE
*/

/** @brief Draw a line using Bresenham's line algorithm
 *  @param x0 horizontal location of the beginning of the line
 *  @param y0 vertical location of the beginning of the line
 *  @param x1 horizontal location of the end of the line
 *  @param y1 vertical location of the end of the line
 *  @param color Color (ex: JO_COLOR_Red)
 */
static  __jo_force_inline void        jo_draw_background_line(int x0, int y0, int x1, int y1, const jo_color color)
{
    jo_vdp2_draw_bitmap_nbg1_line(x0, y0, x1, y1, color);
}

/** @brief Draw a square on the background
 *  @param x Square horizontal location
 *  @param y Square vertical location
 *  @param width Square width
 *  @param height Square height
 *  @param color Color (ex: JO_COLOR_Red)
 */
static  __jo_force_inline void	jo_draw_background_square(const int x, const int y, const short width, const short height, const jo_color color)
{
    jo_vdp2_draw_bitmap_nbg1_square(x, y, width, height, color);
}

/** @brief Put pixel in background using color
 *  @param x horizontal location
 *  @param y vertical location
 *  @param color Color (ex: JO_COLOR_Red)
 */
static  __jo_force_inline void	jo_put_pixel_in_background(const int x, const int y, const jo_color color)
{
    jo_vdp2_put_pixel_bitmap_nbg1(x, y, color);
}

/** @brief Put pixel in background using composite color
 *  @param x horizontal location
 *  @param y vertical location
 *  @param r Red color component
 *  @param g Green color component
 *  @param b Blue color component
 */
static  __jo_force_inline void	jo_put_pixel_in_background_rgb(const int x, const int y, unsigned char r, unsigned char g, unsigned char b)
{
    jo_vdp2_put_pixel_bitmap_nbg1_rgb(x, y, r, g, b);
}

/*
██████╗ ██████╗     ██████╗ ██╗      █████╗ ███╗   ██╗███████╗███████╗
╚════██╗██╔══██╗    ██╔══██╗██║     ██╔══██╗████╗  ██║██╔════╝██╔════╝
 █████╔╝██║  ██║    ██████╔╝██║     ███████║██╔██╗ ██║█████╗  ███████╗
 ╚═══██╗██║  ██║    ██╔═══╝ ██║     ██╔══██║██║╚██╗██║██╔══╝  ╚════██║
██████╔╝██████╔╝    ██║     ███████╗██║  ██║██║ ╚████║███████╗███████║
╚═════╝ ╚═════╝     ╚═╝     ╚══════╝╚═╝  ╚═╝╚═╝  ╚═══╝╚══════╝╚══════╝
** - Two planes are available: A and B
** - Each plane is 512 x 512 size
*/

/** @brief Enable 3D planes
 *  @param background_color Color (ex: JO_COLOR_Red)
 */
static  __jo_force_inline void                            jo_enable_background_3d_plane(jo_color background_color)
{
    jo_vdp2_enable_rbg0();
    jo_set_default_background_color(background_color);
}

/** @brief Disable 3D planes
 *  @param background_color Color (ex: JO_COLOR_Red)
 */
static  __jo_force_inline void                            jo_disable_background_3d_plane(jo_color background_color)
{
    jo_vdp2_disable_rbg0();
    jo_set_default_background_color(background_color);
}

/** @brief Setup plane A
 *  @param img 8 bits 255 colors max image. (Width AND height must be a multiple of 8)
 *  @param palette_id palette id from TGA (see also jo_palette)
 *  @param repeat Repeat the plane infinitely.
 *  @param vertical_flip Flip image vertically
 *  @warning Image need to be clockwised rotated (right) because of an optimisation
 */
static  __jo_force_inline void                            jo_background_3d_plane_a_img(jo_img_8bits *img, int palette_id, bool repeat, bool vertical_flip)
{
    jo_vdp2_set_rbg0_plane_a_8bits_image(img, palette_id, repeat, vertical_flip);
}

/** @brief Setup plane B
 *  @param img 8 bits 255 colors max image. (Width AND height must be a multiple of 8)
 *  @param palette_id palette id from TGA (see also jo_palette)
 *  @param repeat Repeat the plane infinitely.
 *  @param vertical_flip Flip image vertically
 *  @warning Image need to be clockwised rotated (right) because of an optimisation
 */
static  __jo_force_inline void                            jo_background_3d_plane_b_img(jo_img_8bits *img, int palette_id, bool repeat, bool vertical_flip)
{
    jo_vdp2_set_rbg0_plane_b_8bits_image(img, palette_id, repeat, vertical_flip);
}

#if JO_COMPILE_USING_SGL

/** @brief Draw plane A
  * @param use_scroll_format_matrix Convert current matrix to scroll format matrix
 */
static  __jo_force_inline void  jo_background_3d_plane_a_draw(const bool use_scroll_format_matrix)
{
    jo_vdp2_draw_rbg0_plane_a(use_scroll_format_matrix);
}

/** @brief Draw plane A
  * @param use_scroll_format_matrix Convert current matrix to scroll format matrix
 */
static  __jo_force_inline void  jo_background_3d_plane_b_draw(const bool use_scroll_format_matrix)
{
    jo_vdp2_draw_rbg0_plane_b(use_scroll_format_matrix);
}

#endif

/*
██╗  ██╗ ██████╗ ██████╗ ██╗███████╗ ██████╗ ███╗   ██╗████████╗ █████╗ ██╗
██║  ██║██╔═══██╗██╔══██╗██║╚══███╔╝██╔═══██╗████╗  ██║╚══██╔══╝██╔══██╗██║
███████║██║   ██║██████╔╝██║  ███╔╝ ██║   ██║██╔██╗ ██║   ██║   ███████║██║
██╔══██║██║   ██║██╔══██╗██║ ███╔╝  ██║   ██║██║╚██╗██║   ██║   ██╔══██║██║
██║  ██║╚██████╔╝██║  ██║██║███████╗╚██████╔╝██║ ╚████║   ██║   ██║  ██║███████╗
╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═╝╚═╝╚══════╝ ╚═════╝ ╚═╝  ╚═══╝   ╚═╝   ╚═╝  ╚═╝╚══════╝

██╗     ██╗███╗   ██╗███████╗    ███████╗ ██████╗██████╗  ██████╗ ██╗     ██╗
██║     ██║████╗  ██║██╔════╝    ██╔════╝██╔════╝██╔══██╗██╔═══██╗██║     ██║
██║     ██║██╔██╗ ██║█████╗      ███████╗██║     ██████╔╝██║   ██║██║     ██║
██║     ██║██║╚██╗██║██╔══╝      ╚════██║██║     ██╔══██╗██║   ██║██║     ██║
███████╗██║██║ ╚████║███████╗    ███████║╚██████╗██║  ██║╚██████╔╝███████╗███████╗
╚══════╝╚═╝╚═╝  ╚═══╝╚══════╝    ╚══════╝ ╚═════╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚══════╝

▲ ONLY WORKS IF BACKGROUND IMAGE IS LOADED THROUH jo_set_background_8bits_sprite() ▲
  OTHERWISE IT MAY GLITCH
*/

/** @brief 512 entries. One offset for each line because the background size is a 512x512 pixels by default.
 */
# define JO_SCROLL_TABLE_SIZE       JO_NBG1_SCROLL_TABLE_SIZE

/** @brief Enable horizontal line scroll effect
 *  @return An array of 512 entries. One offset for each line because the background size is a 512x512 pixels by default.
 *  @warning Only works with 8 bits background image
 */
static  __jo_force_inline int                                 *jo_enable_background_horizontal_line_scroll(void)
{
    return (jo_vdp2_enable_nbg1_line_scroll());
}

/** @brief Disable horizontal line scroll effect
 */
static  __jo_force_inline void                                jo_disable_background_horizontal_line_scroll(void)
{
    jo_vdp2_disable_nbg1_line_scroll();
}

/** @brief Compute horizontal line scroll effect using specific offset
 *  @param offset Offset in scroll table returned by jo_enable_background_horizontal_line_scroll()
 */
static  __jo_force_inline void                                jo_compute_background_horizontal_line_scroll(unsigned short offset)
{
    jo_vdp2_compute_nbg1_line_scroll(offset);
}

#endif /* !__JO_BACKGROUND_H__ */

/*
** END OF FILE
*/
