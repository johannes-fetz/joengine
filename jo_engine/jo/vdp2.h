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
/** @file vdp2.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine VDP2 definition and tools
 *  @bug No known bugs.
 */

 /*
    ▲ IMPORTANT NOTE ABOUT VDP2 USAGE (extract from ST-058-R2-060194.pdf p61):
    Depending on the color count of NBG0 and NBG1, the scroll screen that cannot be displayed will appear.

    - When NBG0 is set as 2048 or 32768 colors, NBG2 can not longer be displayed.
    - When NBG0 is set at 16,770,000 colors, NBG1 to NBG3 can not longer be displayed.
    - When NBG1 is set at 2048 or 32768 colors, NBG3 can longer be displayed.
 */

#ifndef __JO_VDP2_H__
# define __JO_VDP2_H__

/** @brief sprintf prototypes
 *  @param str String
 *  @param format Format string
 *  @param ... Arguments
 *  @return If successful, the total number of characters written is returned excluding the null-character appended at the end of the string, otherwise a negative number is returned in case of failure.
 */
int				sprintf(char* str, const char* format, ...);

/** @brief Internal sprintf buffer
 *  @warning MC Hammer: don't touch this
 */
extern char		        __jo_sprintf_buf[JO_PRINTF_BUF_SIZE];

/** @brief Set displayed screens
 *  @param scroll_screen_flags Scroll screens (You can pass multiple value using pipe(|). Example: JO_NBG1_SCREEN|JO_NBG2_SCREEN)
 */
void            jo_set_displayed_screens(const jo_scroll_screen scroll_screen_flags);

/** @brief Set default background color
 *  @param background_color Color
 */
void            jo_set_default_background_color(const jo_color background_color);

/*
███╗   ██╗██████╗  ██████╗ ██████╗     ██╗██████╗     ███████╗ ██████╗ ███╗   ██╗████████╗
████╗  ██║██╔══██╗██╔════╝ ╚════██╗   ██╔╝╚════██╗    ██╔════╝██╔═══██╗████╗  ██║╚══██╔══╝
██╔██╗ ██║██████╔╝██║  ███╗ █████╔╝  ██╔╝  █████╔╝    █████╗  ██║   ██║██╔██╗ ██║   ██║
██║╚██╗██║██╔══██╗██║   ██║██╔═══╝  ██╔╝   ╚═══██╗    ██╔══╝  ██║   ██║██║╚██╗██║   ██║
██║ ╚████║██████╔╝╚██████╔╝███████╗██╔╝   ██████╔╝    ██║     ╚██████╔╝██║ ╚████║   ██║
╚═╝  ╚═══╝╚═════╝  ╚═════╝ ╚══════╝╚═╝    ╚═════╝     ╚═╝      ╚═════╝ ╚═╝  ╚═══╝   ╚═╝
*/

#ifndef JO_COMPILE_WITH_PRINTF_SUPPORT

/** @brief NBG2 Print implementation
 *  @param x Horizontal position from top left screen corner
 *  @param y Vertical position from top left screen corner
 *  @param str String
 */
void    jo_nbg2_print(int x, int y, char * str);

/** @brief NBG2 printf implementation (jo_set_nbg2_8bits_font must be called before)
 *  @param X Horizontal position from top left screen corner
 *  @param Y Vertical position from top left screen corner
 *  @param ... Arguments
 */
# define jo_nbg2_printf(X, Y, ...)		do {sprintf(__jo_sprintf_buf, __VA_ARGS__); jo_nbg2_print(X, Y, __jo_sprintf_buf); } while(0)

/** @brief Set 8 bits NBG2 font image
 *  @param img 8 bits 255 colors max image. (height must be a multiple of 8). Width must be 8.
 *  @param mapping Image pattern : " 0123456789ABCDEFGH...". The first character must be an empty space.
 *  @param palette_id palette id from TGA (see also jo_palette)
 *  @param vertical_flip Flip image vertically
 *  @param enabled Display NBG2 now
 *  @warning Image need to be clockwised rotated (right) because of an optimisation
 */
void    jo_vdp2_set_nbg2_8bits_font(jo_img_8bits *img, char *mapping, int palette_id, bool vertical_flip, bool enabled);

/** @brief Clear NBG2
 */
void    jo_nbg2_clear(void);

/** @brief NBG3 Print implementation
 *  @param x Horizontal position from top left screen corner
 *  @param y Vertical position from top left screen corner
 *  @param str String
 */
void    jo_nbg3_print(int x, int y, char * str);

/** @brief NBG3 printf implementation (jo_set_nbg3_8bits_font must be called before)
 *  @param X Horizontal position from top left screen corner
 *  @param Y Vertical position from top left screen corner
 *  @param ... Arguments
 */
# define jo_nbg3_printf(X, Y, ...)		do {sprintf(__jo_sprintf_buf, __VA_ARGS__); jo_nbg3_print(X, Y, __jo_sprintf_buf); } while(0)

/** @brief Set 8 bits NBG3 font image
 *  @param img 8 bits 255 colors max image. (height must be a multiple of 8). Width must be 8
 *  @param mapping Image pattern : "0123456789ABCDEFGH...". The first character must be an empty space.
 *  @param palette_id palette id from TGA (see also jo_palette)
 *  @param vertical_flip Flip image vertically
 *  @param enabled Display NBG2 now
 *  @warning Image need to be clockwised rotated (right) because of an optimisation
 */
void    jo_vdp2_set_nbg3_8bits_font(jo_img_8bits *img, char *mapping, int palette_id, bool vertical_flip, bool enabled);

/** @brief Clear NBG3
 */
void    jo_nbg3_clear(void);

#endif

/*
██████╗ ██████╗  █████╗ ██╗    ██╗    ██╗███╗   ███╗ █████╗  ██████╗ ███████╗
██╔══██╗██╔══██╗██╔══██╗██║    ██║    ██║████╗ ████║██╔══██╗██╔════╝ ██╔════╝
██║  ██║██████╔╝███████║██║ █╗ ██║    ██║██╔████╔██║███████║██║  ███╗█████╗
██║  ██║██╔══██╗██╔══██║██║███╗██║    ██║██║╚██╔╝██║██╔══██║██║   ██║██╔══╝
██████╔╝██║  ██║██║  ██║╚███╔███╔╝    ██║██║ ╚═╝ ██║██║  ██║╚██████╔╝███████╗
╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝ ╚══╝╚══╝     ╚═╝╚═╝     ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝

*/

/** @brief Add 8 bits NBG1 image
 *  @param img 8 bits 255 colors max image. (Width AND height must be a multiple of 8)
 *  @param palette_id palette id from TGA (see also jo_palette)
 *  @param vertical_flip Flip image vertically
 *  @warning Image need to be clockwised rotated (right) because of an optimisation
 */
void    jo_vdp2_set_nbg1_8bits_image(jo_img_8bits *img, int palette_id, bool vertical_flip);

#ifndef JO_COMPILE_WITH_PRINTF_SUPPORT
/** @brief Set 8 bits NBG0 image
 *  @param img 8 bits 255 colors max image. (Width AND height must be a multiple of 8)
 *  @param palette_id palette id from TGA (see also jo_palette)
 *  @param vertical_flip Flip image vertically
 *  @param enabled Display NBG0 now
 *  @warning Image need to be clockwised rotated (right) because of an optimisation
 */
void    jo_vdp2_set_nbg0_8bits_image(jo_img_8bits *img, int palette_id, bool vertical_flip, bool enabled);
#endif

/** @brief Set 8 bits NBG2 image
 *  @param img 8 bits 255 colors max image. (Width AND height must be a multiple of 8)
 *  @param palette_id palette id from TGA (see also jo_palette)
 *  @param vertical_flip Flip image vertically
 *  @param enabled Display NBG2 now
 *  @warning Image need to be clockwised rotated (right) because of an optimisation
 */
void    jo_vdp2_set_nbg2_8bits_image(jo_img_8bits *img, int palette_id, bool vertical_flip, bool enabled);

/** @brief Set 8 bits NBG3 image
 *  @param img 8 bits 255 colors max image. (Width AND height must be a multiple of 8)
 *  @param palette_id palette id from TGA (see also jo_palette)
 *  @param vertical_flip Flip image vertically
 *  @param enabled Display NBG3 now
 *  @warning Image need to be clockwised rotated (right) because of an optimisation
 */
void    jo_vdp2_set_nbg3_8bits_image(jo_img_8bits *img, int palette_id, bool vertical_flip, bool enabled);

/** @brief Set NBG1 bitmap image
 *  @param img Pointer to an image struct
 *  @param left Left location
 *  @param top Top location
 */
void	jo_vdp2_set_nbg1_image(const jo_img * const img, const unsigned short left, const unsigned short top);

/** @brief Clear NBG1 bitmap
 *  @param color Clear color
 */
void    jo_vdp2_clear_bitmap_nbg1(const jo_color color);

/*
███╗   ███╗ ██████╗ ██╗   ██╗███████╗       ██╗       ███████╗ ██████╗  ██████╗ ███╗   ███╗
████╗ ████║██╔═══██╗██║   ██║██╔════╝       ██║       ╚══███╔╝██╔═══██╗██╔═══██╗████╗ ████║
██╔████╔██║██║   ██║██║   ██║█████╗      ████████╗      ███╔╝ ██║   ██║██║   ██║██╔████╔██║
██║╚██╔╝██║██║   ██║╚██╗ ██╔╝██╔══╝      ██╔═██╔═╝     ███╔╝  ██║   ██║██║   ██║██║╚██╔╝██║
██║ ╚═╝ ██║╚██████╔╝ ╚████╔╝ ███████╗    ██████║      ███████╗╚██████╔╝╚██████╔╝██║ ╚═╝ ██║
╚═╝     ╚═╝ ╚═════╝   ╚═══╝  ╚══════╝    ╚═════╝      ╚══════╝ ╚═════╝  ╚═════╝ ╚═╝     ╚═╝

*/

/** @brief Move NBG1 (scrolling)
 *  @param x horizontal location
 *  @param y vertical location
 */
static  __jo_force_inline void	jo_vdp2_move_nbg1(const int x, const int y)
{
#if JO_COMPILE_USING_SGL
    slScrPosNbg1(JO_MULT_BY_65536(x), JO_MULT_BY_65536(y));
#else
    JO_VDP2_SCXIN1 = x;
    JO_VDP2_SCYIN1 = y;
#endif
}

/** @brief Zoom NBG1 width and height independently
 *  @param width_factor Width zoom factor
 *  @param height_factor Height zoom factor
 */
static  __jo_force_inline void	jo_vdp2_zoom_nbg1f(const float width_factor, const float height_factor)
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

/** @brief Zoom NBG1
 *  @param factor Zoom factor
 */
static  __jo_force_inline void	jo_vdp2_zoom_nbg1(const float factor)
{
    jo_vdp2_zoom_nbg1f(factor, factor);
}

/** @brief Move NBG0 (scrolling)
 *  @param x horizontal location
 *  @param y vertical location
 */
static  __jo_force_inline void	jo_vdp2_move_nbg0(const int x, const int y)
{
#if JO_COMPILE_USING_SGL
    slScrPosNbg0(JO_MULT_BY_65536(x), JO_MULT_BY_65536(y));
#else
    JO_VDP2_SCXIN0 = x;
    JO_VDP2_SCYIN0 = y;
#endif
}

/** @brief Zoom NBG0 width and height independently
 *  @param width_factor Width zoom factor
 *  @param height_factor Height zoom factor
 */
static  __jo_force_inline void	jo_vdp2_zoom_nbg0f(const float width_factor, const float height_factor)
{
#if JO_COMPILE_USING_SGL
    slZoomNbg0(toFIXED(width_factor), toFIXED(height_factor));
#else
    int fixed_width = jo_float2fixed(width_factor);
    int fixed_height = jo_float2fixed(height_factor);
    JO_VDP2_ZMXIN0 = JO_DIV_BY_32768(fixed_width);
    JO_VDP2_ZMYIN0 = JO_DIV_BY_32768(fixed_height);
    JO_VDP2_ZMXDN0 = JO_MOD_POW2(fixed_width, 32768) * 2; /*TODO : fix x2 */
    JO_VDP2_ZMYDN0 = JO_MOD_POW2(fixed_height, 32768) * 2;
#endif
}

/** @brief Zoom NBG0
 *  @param factor Zoom factor
 */
static  __jo_force_inline void	jo_vdp2_zoom_nbg0(const float factor)
{
    jo_vdp2_zoom_nbg0f(factor, factor);
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

/** @brief Draw a NBG1 line using Bresenham's line algorithm
 *  @param x0 horizontal location of the beginning of the line
 *  @param y0 vertical location of the beginning of the line
 *  @param x1 horizontal location of the end of the line
 *  @param y1 vertical location of the end of the line
 *  @param color Color (ex: JO_COLOR_Red)
 */
void        jo_vdp2_draw_bitmap_nbg1_line(int x0, int y0, int x1, int y1, const jo_color color);

/** @brief Draw a square on NBG1
 *  @param x Square horizontal location
 *  @param y Square vertical location
 *  @param width Square width
 *  @param height Square height
 *  @param color Color (ex: JO_COLOR_Red)
 */
static  __jo_force_inline void	jo_vdp2_draw_bitmap_nbg1_square(const int x, const int y, const short width, const short height, const jo_color color)
{
    jo_vdp2_draw_bitmap_nbg1_line(x, y, x + width, y, color);
    jo_vdp2_draw_bitmap_nbg1_line(x + width, y + height, x + width, y, color);
    jo_vdp2_draw_bitmap_nbg1_line(x, y + height, x + width, y + height, color);
    jo_vdp2_draw_bitmap_nbg1_line(x, y + height, x, y, color);
}

/** @brief Put pixel in NBG1 using color
 *  @param x horizontal location
 *  @param y vertical location
 *  @param color Color (ex: JO_COLOR_Red)
 */
static  __jo_force_inline void	jo_vdp2_put_pixel_bitmap_nbg1(const int x, const int y, const jo_color color)
{
    *(((unsigned short *)VDP2_VRAM_A0) + x + y * JO_VDP2_WIDTH) = color;
}

/** @brief Put pixel in NBG1 using composite color
 *  @param x horizontal location
 *  @param y vertical location
 *  @param r Red color component
 *  @param g Green color component
 *  @param b Blue color component
 */
static  __jo_force_inline void	jo_vdp2_put_pixel_bitmap_nbg1_rgb(const int x, const int y, unsigned char r, unsigned char g, unsigned char b)
{
    jo_vdp2_put_pixel_bitmap_nbg1(x, y, C_RGB(r, g, b));
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
 */
void                            jo_vdp2_enable_rbg0(void);

/** @brief Disable 3D planes
 */
void                            jo_vdp2_disable_rbg0(void);

/** @brief Setup plane A
 *  @param img 8 bits 255 colors max image. (Width AND height must be a multiple of 8)
 *  @param palette_id palette id from TGA (see also jo_palette)
 *  @param repeat Repeat the plane infinitely.
 *  @param vertical_flip Flip image vertically
 *  @warning Image need to be clockwised rotated (right) because of an optimisation
 */
void                            jo_vdp2_set_rbg0_plane_a_8bits_image(jo_img_8bits *img, int palette_id, bool repeat, bool vertical_flip);

/** @brief Setup plane B
 *  @param img 8 bits 255 colors max image. (Width AND height must be a multiple of 8)
 *  @param palette_id palette id from TGA (see also jo_palette)
 *  @param repeat Repeat the plane infinitely.
 *  @param vertical_flip Flip image vertically
 *  @warning Image need to be clockwised rotated (right) because of an optimisation
 */
void                            jo_vdp2_set_rbg0_plane_b_8bits_image(jo_img_8bits *img, int palette_id, bool repeat, bool vertical_flip);

/** @brief Replace plane A image
 *  @param img 8 bits 255 colors max image. (Width AND height must be a multiple of 8)
 *  @param vertical_flip Flip image vertically
 *  @warning jo_vdp2_set_rbg0_plane_a_8bits_image() MUST be called before
 *  @warning Image need to be clockwised rotated (right) because of an optimisation and the previous palette is preserved
 */
void                            jo_vdp2_replace_rbg0_plane_a_8bits_image(jo_img_8bits *img, bool vertical_flip);

/** @brief Replace plane B image
 *  @param img 8 bits 255 colors max image. (Width AND height must be a multiple of 8)
 *  @param vertical_flip Flip image vertically
 *  @warning jo_vdp2_set_rbg0_plane_b_8bits_image() MUST be called before
 *  @warning Image need to be clockwised rotated (right) because of an optimisation and the previous palette is preserved
 */
void                            jo_vdp2_replace_rbg0_plane_b_8bits_image(jo_img_8bits *img, bool vertical_flip);

#if JO_COMPILE_USING_SGL

/** @brief Draw plane A
  * @param use_scroll_format_matrix Convert current matrix to scroll format matrix
 */
static  __jo_force_inline void  jo_vdp2_draw_rbg0_plane_a(const bool use_scroll_format_matrix)
{
    slCurRpara(RA); if (use_scroll_format_matrix) slScrMatConv(); slScrMatSet();
}

/** @brief Draw plane A
  * @param use_scroll_format_matrix Convert current matrix to scroll format matrix
 */
static  __jo_force_inline void  jo_vdp2_draw_rbg0_plane_b(const bool use_scroll_format_matrix)
{
    slCurRpara(RB); if (use_scroll_format_matrix) slScrMatConv(); slScrMatSet();
}

#endif

/*
███╗   ███╗ ██████╗ ███████╗ █████╗ ██╗ ██████╗
████╗ ████║██╔═══██╗╚══███╔╝██╔══██╗██║██╔════╝
██╔████╔██║██║   ██║  ███╔╝ ███████║██║██║
██║╚██╔╝██║██║   ██║ ███╔╝  ██╔══██║██║██║
██║ ╚═╝ ██║╚██████╔╝███████╗██║  ██║██║╚██████╗
╚═╝     ╚═╝ ╚═════╝ ╚══════╝╚═╝  ╚═╝╚═╝ ╚═════╝
*/

#if JO_COMPILE_USING_SGL

/** @brief Enable mozaic effect for scroll screen
 *  @param screens Scroll screens (You can pass multiple value using pipe(|). Example: JO_NBG1_SCREEN|JO_NBG2_SCREEN)
 *  @param x Horizontal mozaic size [1-16]
 *  @param y Vertical mozaic size [1-16]
 *  @warning Only JO_NBG0_SCREEN, JO_NBG1_SCREEN, JO_NBG2_SCREEN, JO_NBG3_SCREEN and JO_RBG0_SCREEN supports this feature
 */
static  __jo_force_inline void      jo_enable_screen_mozaic(const jo_scroll_screen screens, const short x, const short y)
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

▲ ONLY WORKS IF BACKGROUND IMAGE IS LOADED THROUH jo_vdp2_set_nbg1_8bits_image() ▲
  OTHERWISE IT MAY GLITCH
*/

/** @brief 512 entries. One offset for each line because the background size is a 512x512 pixels by default.
 */
# define JO_NBG1_SCROLL_TABLE_SIZE       (512)

/** @brief Enable NBG1 horizontal line scroll effect
 *  @return An array of 512 entries. One offset for each line because the background size is a 512x512 pixels by default.
 *  @warning Only works with 8 bits background image
 */
int                                 *jo_vdp2_enable_nbg1_line_scroll(void);

/** @brief Disable NBG1 horizontal line scroll effect
 */
void                                jo_vdp2_disable_nbg1_line_scroll(void);

/** @brief Compute NBG1 horizontal line scroll effect using specific offset
 *  @param offset Offset in scroll table returned by jo_vdp2_enable_nbg1_line_scroll()
 */
void                                jo_vdp2_compute_nbg1_line_scroll(unsigned short offset);

/** @brief 512 entries. One offset for each line because the background size is a 512x512 pixels by default.
 */
# define JO_NBG0_SCROLL_TABLE_SIZE       (512)

/** @brief Enable NBG0 horizontal line scroll effect
 *  @return An array of 512 entries. One offset for each line because the background size is a 512x512 pixels by default.
 *  @warning Only works with 8 bits background image
 */
int                                 *jo_vdp2_enable_nbg0_line_scroll(void);

/** @brief Disable NBG0 horizontal line scroll effect
 */
void                                jo_vdp2_disable_nbg0_line_scroll(void);

/** @brief Compute NBG0 horizontal line scroll effect using specific offset
 *  @param offset Offset in scroll table returned by jo_vdp2_enable_nbg1_line_scroll()
 */
void                                jo_vdp2_compute_nbg0_line_scroll(unsigned short offset);

/*
████████╗██████╗  █████╗ ███╗   ██╗███████╗██████╗  █████╗ ██████╗ ███████╗███╗   ██╗ ██████╗██╗   ██╗
╚══██╔══╝██╔══██╗██╔══██╗████╗  ██║██╔════╝██╔══██╗██╔══██╗██╔══██╗██╔════╝████╗  ██║██╔════╝╚██╗ ██╔╝
   ██║   ██████╔╝███████║██╔██╗ ██║███████╗██████╔╝███████║██████╔╝█████╗  ██╔██╗ ██║██║      ╚████╔╝
   ██║   ██╔══██╗██╔══██║██║╚██╗██║╚════██║██╔═══╝ ██╔══██║██╔══██╗██╔══╝  ██║╚██╗██║██║       ╚██╔╝
   ██║   ██║  ██║██║  ██║██║ ╚████║███████║██║     ██║  ██║██║  ██║███████╗██║ ╚████║╚██████╗   ██║
   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚══════╝╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═══╝ ╚═════╝   ╚═╝

*/

/** @brief Disable all screens transparency
 */
void      jo_disable_all_screens_transparency(void);

/** @brief Enable screen transparency
 *  @param screen Scroll screen
 *  @param transparency_level Transparency Level 0 (fully visible) to 31 (almost invisible)
 */
void      jo_enable_screen_transparency(const jo_scroll_screen screen, const unsigned short transparency_level);

#endif /* !__JO_VDP2_H__ */

/*
** END OF FILE
*/
