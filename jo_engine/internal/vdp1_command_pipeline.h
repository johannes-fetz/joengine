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
/** @file internal/vdp1_command_pipeline.h
 *  @author Johannes Fetz
 *
 *  @brief VDP1 command pipeline: internal functions of the engine (see jo/vdp1_command_pipeline.h)
 *  @warning Not part of the API: only for the source files of the engine (included after jo/vdp1_command_pipeline.h). The games
 *  include jo/jo.h, which doesn't include this file
 *  @bug No known bugs.
 */

#ifndef __JO_INTERNAL_VDP1_COMMAND_PIPELINE_H__
# define __JO_INTERNAL_VDP1_COMMAND_PIPELINE_H__

/** @brief 3D windows (internal usage) */
extern jo_vdp1_window       __jo_vdp1_windows[JO_VDP1_WINDOW_COUNT];

/** @brief Initialize the VDP1 (internal usage) */
void                        __jo_vdp1_init(void);

/** @brief Sort the commands of the frame and copy them into the VDP1 VRAM (internal usage) */
void                        __jo_vdp1_end_frame(void);
/** @brief A frame of the game starts (internal usage, the master CPU after the VBLANK it waited for): the table of the
 *  frame is sent in its last field at the earliest (JO_FRAMERATE > 1, see __jo_vdp1_end_frame()) */
void                        __jo_vdp1_frame_start(void);

/** @brief Display the last complete command table (internal usage, called during VBLANK-IN) */
void                        __jo_vdp1_vblank(void);

/** @brief Number of the frame being built (internal usage): the VDP2 changes that must match the image of this frame are
 *  kept with this number and applied when it is displayed (see __jo_vdp1_displayed_frame_changed())
 *  @return Frame number (it increases at each frame)
 */
unsigned int                __jo_vdp1_frame_tag(void);

/** @brief Called during the VBLANK, after __jo_vdp1_vblank() (internal usage, a VBLANK callback): the image of an other
 *  frame is displayed from this field
 *  @param tag Output: number of the frame displayed (see __jo_vdp1_frame_tag())
 *  @return true if the frame displayed changes at this VBLANK
 */
bool                        __jo_vdp1_displayed_frame_changed(unsigned int * const tag);

/** @brief Allocate a command for the current frame (internal usage)
 *  @param z Depth used to sort the commands (bigger = farther)
 *  @return Command or JO_NULL if the table is full
 */
jo_vdp1_command             *__jo_vdp1_add_command(const jo_fixed z);

/** @brief Reserve commands for the current frame (internal usage)
 *  @param count Wanted command count
 *  @param keys Output: depth keys of the reserved commands (see __jo_vdp1_sort_key())
 *  @param reserved Output: reserved command count (less than count if the table is full)
 *  @return First reserved command
 *  @remarks The commands are added by __jo_vdp1_commit_commands(). link must be set to 0.
 */
jo_vdp1_command             *__jo_vdp1_reserve_commands(const unsigned int count, unsigned short **keys, unsigned int *reserved);

/** @brief Add the first commands reserved by __jo_vdp1_reserve_commands() to the frame, their windows written by the caller
 *  (internal usage: the slave CPU copies its commands, see jo_3d_slave_end())
 *  @param used Command count
 *  @return The windows of the commands (to write)
 */
unsigned short              *__jo_vdp1_commit_commands_without_window(const unsigned int used);

/** @brief Add the first commands reserved by __jo_vdp1_reserve_commands() to the frame (internal usage)
 *  @param used Command count
 */
void                        __jo_vdp1_commit_commands(const unsigned int used);

/** @brief Depth key of a command (internal usage)
 *  @param z Depth (bigger = farther)
 *  @return Key used to sort the commands
 */
static  __jo_force_inline unsigned short __jo_vdp1_sort_key(const jo_fixed z)
{
    int                     key;

    key = JO_FIXED_TO_INT(z) + __jo_vdp1_depth_bias;
    if (key < 0)
        return (0);
    if (key > JO_VDP1_MAX_KEY)
        return (JO_VDP1_MAX_KEY);
    return ((unsigned short)key);
}

/** @brief Right of the system clipping area (internal usage) */
extern short                __jo_vdp1_system_clip_right;

/** @brief Bottom of the system clipping area (internal usage) */
extern short                __jo_vdp1_system_clip_bottom;

/** @brief Fill the drawing part of a command from a sprite attribute (internal usage)
 *  @param cmd VDP1 command
 *  @param attr Sprite attribute
 *  @return true if the command is textured
 *  @remarks The coordinates are not set
 */
bool                        __jo_vdp1_set_attribute(jo_vdp1_command *cmd, const jo_vdp1_sprite_attribute *attr);

/** @brief Near clipping distance (internal usage)
 *  @return Projection distance divided by 2^display level (Q16.16)
 *  @remarks Polygons and sprites nearer than this distance are not displayed
 */
static  __jo_force_inline jo_fixed __jo_vdp1_near_clip(void)
{
    return (__jo_vdp1_near_z);
}

/** @brief Draw a sprite on the screen (internal usage)
 *  @param pos Position relative to the center of the current window, depth and scale (scale_x, negative scale: perspective scale)
 *  @param attr Sprite attribute
 *  @param angle Rotation
 *  @return false if the sprite is not visible or if the command table is full
 */
bool                        __jo_vdp1_draw_sprite(const jo_vdp1_sprite_position * const pos, const jo_vdp1_sprite_attribute * const attr, const jo_angle angle);

/** @brief Draw a sprite on the screen with different horizontal and vertical scale (internal usage)
 *  @param pos Position relative to the center of the current window, depth and scales (negative scale: perspective scale)
 *  @param attr Sprite attribute
 *  @param angle Rotation
 *  @return false if the sprite is not visible or if the command table is full
 */
bool                        __jo_vdp1_draw_sprite_hv(const jo_vdp1_sprite_position * const pos, const jo_vdp1_sprite_attribute * const attr, const jo_angle angle);

/** @brief Draw a sprite or a polygon using 4 points (internal usage)
 *  @param points 4 points relative to the center of the current window (clockwise from the top left corner)
 *  @param z Depth
 *  @param attr Sprite attribute
 *  @return false if the sprite is not visible or if the command table is full
 */
bool                        __jo_vdp1_draw_sprite_4p(const jo_pos2D_fixed * const points, const jo_fixed z, const jo_vdp1_sprite_attribute * const attr);

/** @brief Write the command of a sprite using 4 points in a command table of the caller (internal usage: the slave CPU, see
 *  jo_3d_slave_draw_standing_sprite())
 *  @param cmd Command (its sort key is written by the caller)
 *  @param points 4 points relative to the center of the window (clockwise from the top left corner)
 *  @param z Depth
 *  @param attr Sprite attribute
 *  @param window Window (its center and its far limit)
 *  @return false if the sprite is not visible (the command is not written)
 */
bool                        __jo_vdp1_write_sprite_4p(jo_vdp1_command * const cmd, const jo_pos2D_fixed * const points, const jo_fixed z,
                                                      const jo_vdp1_sprite_attribute * const attr, const int window);

/** @brief Attribute of a sprite with some sprite attributes (internal usage: the slave CPU uses its copy of them, see
 *  jo_3d_slave_begin())
 *  @param attr Attribute (output)
 *  @param sprite_id Sprite Id
 *  @param state Sprite attributes
 */
void                        __jo_sprite_attributes_of(jo_vdp1_sprite_attribute *attr, const int sprite_id, const jo_sprite_attributes * const state);

/** @brief Draw a sprite in the 3D space using the current matrix (internal usage)
 *  @param pos Position in the 3D space and scale (scale_x)
 *  @param attr Sprite attribute
 *  @param angle Rotation
 *  @return false if the sprite is not visible or if the command table is full
 *  @remarks The sprite always faces the camera (billboard)
 */
bool                        __jo_vdp1_draw_sprite_3d(const jo_vdp1_sprite_position * const pos, const jo_vdp1_sprite_attribute * const attr, const jo_angle angle);

/** @brief VDP1 commands of the frame being made by the master CPU, read without the cache (internal usage: the budget of
 *  the subdivisions, see 3d.c; the commands of the slave CPU are added at jo_3d_slave_end()) */
int                         __jo_vdp1_frame_command_count(void);

#endif /* !__JO_INTERNAL_VDP1_COMMAND_PIPELINE_H__ */

/*
** END OF FILE
*/
