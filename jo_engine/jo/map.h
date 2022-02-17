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
/** @file map.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine MAP file (sprite map)
 *  @bug No known bugs.
 */

#ifndef __JO_MAP_H__
# define __JO_MAP_H__

/** @brief Value returned by jo_map_hitbox_detection_custom_boundaries();
    @warning  Don't change this value
*/
# define    JO_MAP_NO_COLLISION     (-2147483647)

/** @brief Method to get vertical distance between our point (x, y) and the surface of the map (example: hill)
 *  @param layer layer level (between 0 and JO_MAP_MAX_LAYER)
 *  @param x Horizontal position of the point (based on the screen !, so 0 is the left the screen)
 *  @param y Vertical position of the point (based on the screen !, so 0 is the top the screen)
 *  @param attribute Tile attribute
 *  @remarks        (x,y)
 *  @remarks    #     |      #
 *  @remarks    ###   |     ##
 *  @remarks    ##### v     ##
 *  @remarks    #######   ####
 *  @remarks    ##############
 *  @return distance from the surface of the map otherwise JO_MAP_NO_COLLISION
 *  @warning if distance > 0 then you don't touch the ground but your are on the sprite
 *  @warning if distance <= 0 then you are under the surface of the sprite
 *  @warning Don't use this method if you don't have transparent pixel or if the surface is concave
 */
int         jo_map_per_pixel_vertical_collision(const unsigned int layer, int x, int y, unsigned char *attribute);

/** @brief Fast method to get if a square intersects with the map (HitBox processing) based on screen coord
 *  @param layer layer level (between 0 and JO_MAP_MAX_LAYER)
 *  @param x Horizontal position of square (based on the screen !, so 0 is the left the screen)
 *  @param y Vertical position of square (based on the screen !, so 0 is the top the screen)
 *  @param w Width of the square
 *  @param h Height of the square
 *  @return The attribute of the tile if square collides otherwise JO_MAP_NO_COLLISION
 */
int         jo_map_hitbox_detection_custom_boundaries(const unsigned int layer, const int x, const int y, const int w, const int h);

#ifdef JO_COMPILE_WITH_FS_SUPPORT

/** @brief Load a sprite map on a specific layer using MAP file format
 *  @param layer layer level (between 0 and JO_MAP_MAX_LAYER)
 *  @param depth Z-index
 *  @param sub_dir Sub directory name (use JO_ROOT_DIR if the file is on the root directory)
 *  @param filename Filename (upper case and shorter as possible like "A.MAP")
 *  @remarks  Sample file with 4 blocks. Each tile has his own texture:
 *  @remarks  IMAGE_FILENAME	X	Y   ATTRIBUTE(optional)
 *  @remarks  1.TGA	0	0
 *  @remarks  2.TGA	160	0
 *  @remarks  3.TGA	0	120
 *  @remarks  4.TGA	160	120
 *  @remarks use JoMapEditor to create map file
 *  @return true if successful otherwise false (use jo_get_last_error())
 */
bool            jo_map_load_from_file(const unsigned int layer, const short depth, const char * const sub_dir, const char * const filename);

#endif /* !JO_COMPILE_WITH_FS_SUPPORT */

/** @brief Create a new sprite map
 *  @param layer layer level (between 0 and JO_MAP_MAX_LAYER)
 *  @param max_tile_count Maximum tile count in the entire map
 *  @param depth Z index (ex 500)
 *  @return true if successful otherwise false (use jo_get_last_error())
 */
bool             jo_map_create(const unsigned int layer, const unsigned short max_tile_count, const short depth);

/** @brief Add a tile on the specific layer
 *  @param layer layer level (between 0 and JO_MAP_MAX_LAYER)
 *  @param x horizontal position on the map
 *  @param y vertical position on the map
 *  @param sprite_id Sprite ID
 *  @param attribute User attribute (it's for you)
 */
void            jo_map_add_tile(const unsigned int layer, const short x, const short y,
#if JO_MAX_SPRITE > 255
                                const unsigned short sprite_id,
#else
                                const unsigned char sprite_id,
#endif
                                const unsigned char attribute);

/** @brief Add an animated tile on the specific layer
 *  @param layer layer level (between 0 and JO_MAP_MAX_LAYER)
 *  @param x horizontal position on the map
 *  @param y vertical position on the map
 *  @param anim_id Animation ID
 *  @param attribute User attribute (it's for you)
 */
void            jo_map_add_animated_tile(const unsigned int layer, const short x, const short y, const unsigned char anim_id, const unsigned char attribute);

/** @brief Destroy the specific layer
 *  @param layer layer level (between 0 and JO_MAP_MAX_LAYER)
 */
void			jo_map_free(const unsigned int layer);

/** @brief Display the specific layer
 *  @param layer layer level (between 0 and JO_MAP_MAX_LAYER)
 *  @param screen_x relative horizontal position between the screen and the map
 *  @param screen_y relative vertical position between the screen and the map
 */
void			jo_map_draw(const unsigned int layer, const short screen_x, const short screen_y);

/** @brief Draw the specific layer on the background (NBG1)
 *  @param layer layer level (between 0 and JO_MAP_MAX_LAYER)
 *  @param x relative horizontal position between the background image and the map
 *  @param y relative vertical position between the background image and the map
 *  @warning Only works with full color images (not 8 bits)
 */
void            jo_map_draw_background(const unsigned int layer, const short x, const short y);

/** @brief Move specific tiles by their attributes
 *  @param layer layer level (between 0 and JO_MAP_MAX_LAYER)
 *  @param attribute_filter Tile attribute value (filtering)
 *  @param incr_x Horizontal position incrementation
 *  @param incr_y Vertical position incrementation
 */
void            jo_map_move_tiles_by_attribute(const unsigned int layer, const unsigned char attribute_filter, const short incr_x, const short incr_y);

#ifdef JO_COMPILE_WITH_3D_SUPPORT
void            jo_map_3d_draw(const unsigned int layer, const int x, const int y, const int z);
#endif // JO_COMPILE_WITH_3D_SUPPORT

#endif /* !__JO_MAP_H__ */

/*
** END OF FILE
*/
