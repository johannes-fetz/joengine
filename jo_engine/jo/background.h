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
/** @file background.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Background definition and tools
 *  @bug No known bugs.
 */

#ifndef __JO_BACKGROUND_H__
# define __JO_BACKGROUND_H__

/*
** Background (VDP2)
*/

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
void    jo_set_background_8bits_sprite(jo_img_8bits *img, int palette_id, bool vertical_flip);

/** @brief Add background sprite
 *  @param img Pointer to an image struct
 *  @param left Left location
 *  @param top Top location
 */
void	jo_set_background_sprite(const jo_img * const img, const unsigned short left, const unsigned short top);

/** @brief Clear the background
 *  @param color Clear color
 */
void    jo_clear_background(const jo_color color);

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
#if JO_COMPILE_USING_SGL
    slScrPosNbg1(JO_MULT_BY_65536(x), JO_MULT_BY_65536(y));
#else
    JO_VDP2_SCXIN1 = x;
    JO_VDP2_SCYIN1 = y;
#endif
}

/** @brief Zoom background width and height independently
 *  @param width_factor Width zoom factor
 *  @param height_factor Height zoom factor
 */
static  __jo_force_inline void	jo_zoom_background2(const float width_factor, const float height_factor)
{
#if JO_COMPILE_USING_SGL
    slZoomNbg1(toFIXED(width_factor), toFIXED(height_factor));
#else
    int fixed_width = jo_float2fixed(width_factor);
    int fixed_height = jo_float2fixed(height_factor);
    JO_VDP2_ZMXIN1 = JO_DIV_BY_32768(fixed_width);
    JO_VDP2_ZMYIN1 = JO_DIV_BY_32768(fixed_height);
    JO_VDP2_ZMXDN1 = JO_MOD_POW2(fixed_width, 32768) * 2; /*TODO : fix x2 */
    JO_VDP2_ZMYDN1 = JO_MOD_POW2(fixed_height, 32768) * 2;
#endif
}

/** @brief Zoom background
 *  @param factor Zoom factor
 */
static  __jo_force_inline void	jo_zoom_background(const float factor)
{
    jo_zoom_background2(factor, factor);
}

/*
██████╗  █████╗ ███████╗██╗ ██████╗    ██████╗ ██████╗  █████╗ ██╗    ██╗██╗███╗   ██╗ ██████╗
██╔══██╗██╔══██╗██╔════╝██║██╔════╝    ██╔══██╗██╔══██╗██╔══██╗██║    ██║██║████╗  ██║██╔════╝
██████╔╝███████║███████╗██║██║         ██║  ██║██████╔╝███████║██║ █╗ ██║██║██╔██╗ ██║██║  ███╗
██╔══██╗██╔══██║╚════██║██║██║         ██║  ██║██╔══██╗██╔══██║██║███╗██║██║██║╚██╗██║██║   ██║
██████╔╝██║  ██║███████║██║╚██████╗    ██████╔╝██║  ██║██║  ██║╚███╔███╔╝██║██║ ╚████║╚██████╔╝
╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝ ╚═════╝    ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝ ╚══╝╚══╝ ╚═╝╚═╝  ╚═══╝ ╚═════╝
*/

/** @brief Draw a line using Bresenham's line algorithm
 *  @param x0 horizontal location of the beginning of the line
 *  @param y0 vertical location of the beginning of the line
 *  @param x1 horizontal location of the end of the line
 *  @param y1 vertical location of the end of the line
 *  @param color Color (ex: JO_COLOR_Red)
 */
void        jo_draw_background_line(int x0, int y0, int x1, int y1, const jo_color color);

/** @brief Draw a square on the background
 *  @param x Square horizontal location
 *  @param y Square vertical location
 *  @param width Square width
 *  @param height Square height
 *  @param color Color (ex: JO_COLOR_Red)
 */
static  __jo_force_inline void	jo_draw_background_square(const int x, const int y, const short width, const short height, const jo_color color)
{
    jo_draw_background_line(x, y, x + width, y, color);
    jo_draw_background_line(x + width, y + height, x + width, y, color);
    jo_draw_background_line(x, y + height, x + width, y + height, color);
    jo_draw_background_line(x, y + height, x, y, color);
}

/** @brief Put pixel in background using color
 *  @param x horizontal location
 *  @param y vertical location
 *  @param color Color (ex: JO_COLOR_Red)
 */
static  __jo_force_inline void	jo_put_pixel_in_background(const int x, const int y, const jo_color color)
{
    *(((unsigned short *)VDP2_VRAM_A0) + x + y * JO_VDP2_WIDTH) = color;
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
    jo_put_pixel_in_background(x, y, C_RGB(r, g, b));
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
void                            jo_enable_background_3d_plane(jo_color background_color);

/** @brief Reset 3D planes if you wants to change setup A ou B
 */
void                            jo_reset_background_3d_plane(void);

/** @brief Disable 3D planes
 *  @param background_color Color (ex: JO_COLOR_Red)
 */
void                            jo_disable_background_3d_plane(jo_color background_color);

/** @brief Setup plane A
 *  @param img 8 bits 255 colors max image. (Width AND height must be a multiple of 8)
 *  @param palette_id palette id from TGA (see also jo_palette)
 *  @param repeat Repeat the plane infinitely.
 *  @param vertical_flip Flip image vertically
 *  @warning Image need to be clockwised rotated (right) because of an optimisation
 */
void                            jo_background_3d_plane_a_img(jo_img_8bits *img, int palette_id, bool repeat, bool vertical_flip);

/** @brief Setup plane B
 *  @param img 8 bits 255 colors max image. (Width AND height must be a multiple of 8)
 *  @param palette_id palette id from TGA (see also jo_palette)
 *  @param repeat Repeat the plane infinitely.
 *  @param vertical_flip Flip image vertically
 *  @warning Image need to be clockwised rotated (right) because of an optimisation
 */
void                            jo_background_3d_plane_b_img(jo_img_8bits *img, int palette_id, bool repeat, bool vertical_flip);

/** @brief Draw plane A
  * @param use_scroll_format_matrix Convert current matrix to scroll format matrix
 */
static  __jo_force_inline void  jo_background_3d_plane_a_draw(const bool use_scroll_format_matrix)
{
    slCurRpara(RA); if (use_scroll_format_matrix) slScrMatConv(); slScrMatSet();
}

/** @brief Draw plane A
  * @param use_scroll_format_matrix Convert current matrix to scroll format matrix
 */
static  __jo_force_inline void  jo_background_3d_plane_b_draw(const bool use_scroll_format_matrix)
{
    slCurRpara(RB); if (use_scroll_format_matrix) slScrMatConv(); slScrMatSet();
}

/*
███╗   ███╗ ██████╗ ███████╗ █████╗ ██╗ ██████╗
████╗ ████║██╔═══██╗╚══███╔╝██╔══██╗██║██╔════╝
██╔████╔██║██║   ██║  ███╔╝ ███████║██║██║
██║╚██╔╝██║██║   ██║ ███╔╝  ██╔══██║██║██║
██║ ╚═╝ ██║╚██████╔╝███████╗██║  ██║██║╚██████╗
╚═╝     ╚═╝ ╚═════╝ ╚══════╝╚═╝  ╚═╝╚═╝ ╚═════╝
*/

/** @brief Enable mozaic effect for scroll screen
 *  @param screens Scroll screens (You can pass multiple value using pipe(|). Example: JO_NBG1_SCREEN|JO_NBG2_SCREEN)
 *  @param x Horizontal mozaic size [1-16]
 *  @param y Vertical mozaic size [1-16]
 *  @warning Only JO_NBG0_SCREEN, JO_NBG1_SCREEN, JO_NBG2_SCREEN, JO_NBG3_SCREEN and JO_RBG0_SCREEN supports this feature
 */
static  __jo_force_inline void      jo_enable_screen_mozaic(jo_scroll_screen screens, short x, short y)
{
    slScrMosSize(x, y);
    slScrMosaicOn(screens);
}

/** @brief Disable mozaic effect for all scroll screen
 */
static  __jo_force_inline void      jo_disable_all_screen_mozaic(void)
{
    slScrMosSize(1, 1);
    slScrMosaicOn(0);
}

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
# define JO_SCROLL_TABLE_SIZE       (512)

/** @brief Enable horizontal line scroll effect
 *  @return An array of 512 entries. One offset for each line because the background size is a 512x512 pixels by default.
 *  @warning Only works with 8 bits background image
 */
int                                 *jo_enable_background_horizontal_line_scroll(void);

/** @brief Disable horizontal line scroll effect
 */
void                                jo_disable_background_horizontal_line_scroll(void);

/** @brief Compute horizontal line scroll effect using specific offset
 *  @param offset Offset in scroll table returned by jo_enable_background_horizontal_line_scroll()
 */
void                                jo_compute_background_horizontal_line_scroll(unsigned short offset);

#endif /* !__JO_BACKGROUND_H__ */

/*
** END OF FILE
*/
