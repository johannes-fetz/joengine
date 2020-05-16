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
/** @file mode7.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Mode7
 *  @bug No known bugs.
 */

#ifndef __JO_MODE7_H__
# define __JO_MODE7_H__

#ifdef JO_COMPILE_WITH_PSEUDO_MODE7_SUPPORT

/** @brief mode7 internal computation cache struct */
typedef struct
{
    jo_color                        *output;
    unsigned short                  output_width;
    unsigned short                  output_width_sizeof;
    unsigned short                  output_line_width;
    unsigned short                  output_height;
    unsigned short                  output_width_div2;
    int                             tile_mask_x;
    int                             tile_mask_y;
    int                             *distance;
    int                             *line_dy;
    int                             output_y_start;
}					                jo_mode7_computation_cache;

/** @brief mode7 struct */
typedef struct
{
    unsigned short                  output_top;
    unsigned short                  output_left;
    int                             space_z;
    int                             horizon;
    int                             scale_x;
    int                             scale_y;
    jo_mode7_computation_cache      __cache;
}					                jo_mode7;

/** @brief Initialize mode 7 effect for direct screen output and pre compute projection
 *  @param params mode7 structure
 *  @param tile_width Tile width
 *  @param tile_height Tile height
 *  @return true if successful otherwise false (use jo_get_last_error())
 */
bool     jo_init_mode7_for_screen_output(jo_mode7 * const params, const unsigned short tile_width, const unsigned short tile_height);

/** @brief Update mode 7 computation if output_left or output_right params changed
 *  @param params mode7 structure
 */
void    jo_update_screen_output_coord_for_mode7(jo_mode7 * const params);

/** @brief Initialize mode 7 effect for image output and pre compute projection
 *  @param params mode7 structure
 *  @param tile_width Tile width
 *  @param tile_height Tile height
 *  @param output_image Output image (data can be allocated before)
 *  @return true if successful otherwise false (use jo_get_last_error())
 */
bool     jo_init_mode7_for_image_output(jo_mode7 *params, const unsigned short tile_width, const unsigned short tile_height, jo_img * const output_image);

/** @brief Free mode 7 computation
 *  @param params mode7 structure
 */
void    jo_free_mode7(jo_mode7 * const params);

/** @brief Draw projection according to the initialized output
 *  @param tile Tile image
 *  @param cam_x Camera horizontal position
 *  @param cam_y Camera depth (Z index) position
 *  @param params mode7 structure
 */
void    jo_do_mode7_floor(const jo_img * const tile, const int cam_x, const int cam_y, const jo_mode7 * const params);

#endif /* !JO_COMPILE_WITH_PSEUDO_MODE7_SUPPORT */

#endif /* !__JO_MODE7_H__ */

/*
** END OF FILE
*/
