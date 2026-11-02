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

/** @brief Set a color for each line of the back screen (visible where no scroll screen is displayed)
 *  @param colors JO_TV_HEIGHT colors (the first one is the top of the screen)
 *  @remarks Useful for a sky gradient. The colors are copied: the function can be called every frame.
 *  @remarks jo_set_default_background_color() goes back to a single color
 */
void            jo_set_background_line_colors(const jo_color * const colors);

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
# define jo_nbg2_printf(X, Y, ...)		do {jo_sprintf(__jo_sprintf_buf, __VA_ARGS__); jo_nbg2_print(X, Y, __jo_sprintf_buf); } while(0)

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
# define jo_nbg3_printf(X, Y, ...)		do {jo_sprintf(__jo_sprintf_buf, __VA_ARGS__); jo_nbg3_print(X, Y, __jo_sprintf_buf); } while(0)

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

/** @brief Add 8 bits NBG1 image, repeated or not
 *  @param img 8 bits 255 colors max image. (Width AND height must be a multiple of 8, 512 x 512 at most)
 *  @param palette_id palette id from TGA (see also jo_palette)
 *  @param vertical_flip Flip image vertically
 *  @param repeat true: the image is repeated in the 512 x 512 pixels of the screen; false: transparent out of the image
 *  (shows an empty cell of the image there: repeated when the image has no 8 x 8 transparent cell)
 *  @warning Image need to be clockwised rotated (right) because of an optimisation
 */
void    jo_vdp2_set_nbg1_8bits_image_repeat(jo_img_8bits *img, int palette_id, bool vertical_flip, bool repeat);

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
void	jo_vdp2_set_nbg1_image(const jo_img * const img, short left, short top);

/** @brief The NBG1 bitmap is in the VRAM (Internal usage, see __jo_vdp2_reserve_nbg1_bitmap()) */
extern bool __jo_vdp2_nbg1_bitmap_reserved;

/** @brief Allocate and display the NBG1 bitmap at its first use (Internal usage)
 *  @return false if it can't be at the start of the bank A0 (the bank is used by other screens: line scroll tables, cells
 *  of NBG1 or RBG0). The drawing functions of the bitmap call it
 *  @remarks The bitmap takes 256 KB of the VDP2 VRAM (the banks A0 and A1): a program that never draws in it doesn't
 *  allocate it. JO_COMPATIBILITY_NBG1_BITMAP_AT_INIT = 1 in the makefile: allocated and displayed by jo_core_init() (the
 *  old behavior)
 */
bool    __jo_vdp2_reserve_nbg1_bitmap(void);

/** @brief Clear NBG1 bitmap
 *  @param color Clear color
 */
void    jo_vdp2_clear_bitmap_nbg1(const jo_color color);

/** @brief Fill a rectangle in the NBG1 bitmap (Internal usage)
 *  @param x Horizontal position in the bitmap (wrapped around JO_VDP2_WIDTH)
 *  @param y Vertical position in the bitmap (wrapped around JO_VDP2_HEIGHT)
 *  @param width Rectangle width
 *  @param height Rectangle height
 *  @param color Fill color
 *  @remarks The rectangle wraps around the borders of the bitmap like the NBG1 scroll
 */
void    __jo_vdp2_nbg1_bitmap_fill(const int x, const int y, int width, int height, const jo_color color);

/** @brief Copy a part of an image in the NBG1 bitmap (Internal usage)
 *  @param img Image
 *  @param src_x Horizontal position of the part in the image
 *  @param src_y Vertical position of the part in the image
 *  @param width Part width (must be inside the image)
 *  @param height Part height (must be inside the image)
 *  @param x Horizontal position in the bitmap (wrapped around JO_VDP2_WIDTH)
 *  @param y Vertical position in the bitmap (wrapped around JO_VDP2_HEIGHT)
 *  @remarks The part wraps around the borders of the bitmap like the NBG1 scroll
 */
void    __jo_vdp2_nbg1_bitmap_blit(const jo_img * const img, const int src_x, const int src_y, int width, int height, const int x, const int y);

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
void	jo_vdp2_move_nbg1(const int x, const int y);

/** @brief Zoom NBG1 width and height independently
 *  @param width_factor Width zoom factor
 *  @param height_factor Height zoom factor
 */
void	jo_vdp2_zoom_nbg1f(const float width_factor, const float height_factor);

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
void	jo_vdp2_move_nbg0(const int x, const int y);

/** @brief Zoom NBG0 width and height independently
 *  @param width_factor Width zoom factor
 *  @param height_factor Height zoom factor
 */
void	jo_vdp2_zoom_nbg0f(const float width_factor, const float height_factor);

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
    // (the bitmap at its first use, see __jo_vdp2_reserve_nbg1_bitmap())
    if (!__jo_vdp2_nbg1_bitmap_reserved && !__jo_vdp2_reserve_nbg1_bitmap())
        return;
    *(((unsigned short *)JO_VDP2_VRAM_A0) + x + y * JO_VDP2_WIDTH) = color;
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
    jo_vdp2_put_pixel_bitmap_nbg1(x, y, JO_COLOR_SATURN_RGB(r, g, b));
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

/** @brief Hide the farthest part of the 3D planes (the back screen is visible instead)
 *  @param max_zoom_out Biggest reduction of the texture (0: no limit, 16: the plane stops where 16 texels are in 1 pixel)
 *  @remarks Far away, a plane is very reduced and looks noisy: with a back screen color like the horizon, it fades in the distance
 */
void                            jo_vdp2_set_rbg0_far_limit(const int max_zoom_out);

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


/** @brief Draw plane A
  * @param use_scroll_format_matrix Convert current matrix to scroll format matrix
 */
void    jo_vdp2_draw_rbg0_plane_a(const bool use_scroll_format_matrix);

/** @brief Draw plane B
  * @param use_scroll_format_matrix Convert current matrix to scroll format matrix
 */
void    jo_vdp2_draw_rbg0_plane_b(const bool use_scroll_format_matrix);

/** @brief Draw plane B as a flat image, without perspective: a sky behind the plane A (sea, ground)
 *  @param x Horizontal position in the image of the left of the screen (pixels, the image is repeated horizontally)
 *  @param y Vertical position in the image of the top of the screen (pixels)
 *  @remarks The plane B is displayed where the plane A is transparent: above its horizon and after its far limit
 *  (see jo_vdp2_set_rbg0_far_limit()). Above the image (y < 0), the top left cell (8 x 8 pixels) of the image is repeated:
 *  the top of the image must be one color (the top of the sky). Below the image, the image is repeated (hidden by the plane A)
 *  @remarks The image (jo_vdp2_set_rbg0_plane_b_8bits_image()) must be as wide as the plane (512 pixels) or a divisor of it
 *  @warning Image need to be clockwised rotated (right) because of an optimisation
 */
void    jo_vdp2_draw_rbg0_plane_b_flat(const int x, const int y);


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
void    jo_enable_screen_mozaic(const jo_scroll_screen screens, short x, short y);

/** @brief Disable mozaic effect for all scroll screen
 */
void    jo_disable_all_screen_mozaic(void);


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

/** @brief Lines of the table of jo_vdp2_enable_nbg1_line_scroll_xy() */
# define JO_NBG1_SCROLL_XY_TABLE_LINES   (256)

/** @brief Enable the NBG1 horizontal and vertical line scroll: each line of the screen shows its own part of the image
 *  (a split screen: each view its own sky)
 *  @return A table of JO_NBG1_SCROLL_XY_TABLE_LINES x 2 entries in VRAM (16.16, the fraction on 8 bits): for each line of the
 *  screen from the top, the horizontal scroll, then the vertical one: the line of the image shown on this line of the screen
 *  (not an offset: 0 on every line shows the first line of the image everywhere; measured in Mednafen); JO_NULL when there
 *  is no memory
 *  @remarks Replaces the table of jo_vdp2_enable_nbg1_line_scroll(); jo_vdp2_disable_nbg1_line_scroll() stops it. Not with
 *  the vertical cell scroll of NBG1
 */
int                                 *jo_vdp2_enable_nbg1_line_scroll_xy(void);

/** @brief Compute NBG1 horizontal line scroll effect using specific offset
 *  @param offset Offset in scroll table returned by jo_vdp2_enable_nbg1_line_scroll()
 */
void                                jo_vdp2_compute_nbg1_line_scroll(unsigned short offset);

/** @brief Entries of the NBG1 vertical cell scroll table: one for each column of 8 pixels on the screen (and one more
 *  when the screen is scrolled by a fraction of a column)
 */
# define JO_NBG1_VERTICAL_CELL_SCROLL_TABLE_SIZE (64)

/** @brief Enable the NBG1 vertical cell scroll: each column of 8 pixels of the screen is moved vertically by its own offset
 *  (added to the vertical scroll of the screen)
 *  @return An array of JO_NBG1_VERTICAL_CELL_SCROLL_TABLE_SIZE jo_fixed offsets in VRAM (from the left of the screen,
 *  all 0 at first), JO_NULL when there is no memory
 *  @remarks Needs a free VRAM access slot (T0 to T2) in the bank of the table: without it, the offsets are ignored
 *  @remarks Works with a bitmap or a cell screen, without the line scroll of NBG1
 */
int                                 *jo_vdp2_enable_nbg1_vertical_cell_scroll(void);

/** @brief Disable the NBG1 vertical cell scroll and free its table
 */
void                                jo_vdp2_disable_nbg1_vertical_cell_scroll(void);

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

/** @brief Color bank bits of an additive sprite (see jo_enable_additive_sprites(), jo_sprite_set_additive_palette()): the
 *  priority register 3 of the sprites (sprite type 3: bits 14 and 13 of a palette pixel)
 */
# define JO_VDP2_ADDITIVE_SPRITE_BANK       (0x6000)

/** @brief The light of the additive sprites is added to the image under them (the VDP2 color calculation "add", like the
 *  glows, see jo/glow.h): a halo without edge (the sun, a flare) drawn by the VDP1. An additive sprite is a palette
 *  sprite (8 bits) drawn with jo_sprite_set_additive_palette(): its pixels have the priority register 3 of the sprites,
 *  the only one with the color calculation. Black adds nothing, the colors saturate to white
 *  @remarks The color calculation mode is the same for all the screens: no transparency of a screen (see
 *           jo_enable_screen_transparency()) at the same time
 */
void      jo_enable_additive_sprites(void);

/** @brief The additive sprites are opaque again (see jo_enable_additive_sprites()) */
void      jo_disable_additive_sprites(void);

#endif /* !__JO_VDP2_H__ */

/*
** END OF FILE
*/
