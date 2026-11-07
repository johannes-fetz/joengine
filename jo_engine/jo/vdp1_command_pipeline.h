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
/** @file vdp1_command_pipeline.h
 *  @author Johannes Fetz
 *
 *  @brief VDP1 command table handling
 *  @bug No known bugs.
 */

#ifndef __JO_VDP1_COMMAND_PIPELINE_H__
# define __JO_VDP1_COMMAND_PIPELINE_H__

/*
** VDP1 VRAM layout:
**
** 0x00000 : jump command to the table to draw
** 0x00020 : command table A
** 0x08000 : command table B
** 0x10000 : textures (JO_VDP1_TEXTURE_DEF_BASE_ADDRESS)
** 0x7E4F8 : gouraud tables of the sprites, 2 sets of 32 (JO_VDP1_USER_AREA_END_ADDR, jo_set_gouraud_shading_colors())
** 0x7E700 : gouraud tables of the clipped and subdivided 3D polygons
** 0x7FF00 : gouraud tables used for 3D lighting
*/

/** @brief Maximum commands per frame (sprites, polygons, the headers of the windows). Each one costs 36 bytes of
 *  RAM per command buffer (2 buffers with the second CPU) + 4 bytes for the sort; the 3D of the slave CPU can write
 *  JO_VDP1_MAX_COMMANDS - 40 commands
 *  @remarks Smaller in the makefile to save RAM (JO_VDP1_MAX_COMMANDS = 256: 30 KB less per buffer). 1000 at most (the
 *  command tables of the VRAM)
 */
#ifndef JO_VDP1_MAX_COMMANDS
# define JO_VDP1_MAX_COMMANDS               (1000)
#endif

/** @brief VDP1 command (32 bytes) */
typedef struct
{
    unsigned short          ctrl;
    unsigned short          link;
    unsigned short          pmod;
    unsigned short          colr;
    unsigned short          srca;
    unsigned short          size;
    short                   xa;
    short                   ya;
    short                   xb;
    short                   yb;
    short                   xc;
    short                   yc;
    short                   xd;
    short                   yd;
    unsigned short          grda;
    unsigned short          reserved;
}                           jo_vdp1_command;

/** @brief VDP1 command types (CMDCTRL) */
# define JO_VDP1_CMD_NORMAL_SPRITE          (0x0000)
# define JO_VDP1_CMD_SCALED_SPRITE          (0x0001)
# define JO_VDP1_CMD_DISTORTED_SPRITE       (0x0002)
# define JO_VDP1_CMD_POLYGON                (0x0004)
# define JO_VDP1_CMD_POLYLINE               (0x0005)
# define JO_VDP1_CMD_LINE                   (0x0006)
# define JO_VDP1_CMD_USER_CLIP              (0x0008)
# define JO_VDP1_CMD_SYSTEM_CLIP            (0x0009)
# define JO_VDP1_CMD_LOCAL_COORD            (0x000A)
# define JO_VDP1_CMD_SKIP_ASSIGN            (0x5000)
# define JO_VDP1_CMD_END                    (0x8000)

/*
** Drawing function (low bits of the "direction" attribute)
*/
/** @brief Normal sprite */
# define JO_VDP1_FUNCTION_SPRITE            (1)
/** @brief Textured polygon (distorted sprite) */
# define JO_VDP1_FUNCTION_TEXTURE           (2)
/** @brief Polygon */
# define JO_VDP1_FUNCTION_POLYGON           (4)
/** @brief Polyline */
# define JO_VDP1_FUNCTION_POLYLINE          (5)
/** @brief Line */
# define JO_VDP1_FUNCTION_LINE              (6)
/** @brief User clipping area */
# define JO_VDP1_FUNCTION_USER_CLIP         (8)
/** @brief System clipping area */
# define JO_VDP1_FUNCTION_SYSTEM_CLIP       (9)
/** @brief Local coordinates */
# define JO_VDP1_FUNCTION_LOCAL_COORDINATES (10)

/** @brief Drawing function mask of the direction attribute */
# define JO_VDP1_DIRECTION_FUNCTION_MASK    (0x0f)
/** @brief Flip mask of the direction attribute (CMDCTRL layout) */
# define JO_VDP1_DIRECTION_FLIP_MASK        (0x30)
/** @brief Horizontal flip bit of the direction attribute (CMDCTRL layout) */
# define JO_VDP1_H_FLIP                     (0x10)

/*
** Draw mode (CMDPMOD register layout)
*/
/** @brief Write the MSB of the frame buffer (shadow) */
# define JO_VDP1_MSB_ON                     (1 << 15)
/** @brief High speed shrink */
# define JO_VDP1_HIGH_SPEED_SHRINK          (1 << 12)
/** @brief No clipping window */
# define JO_VDP1_NO_WINDOW                  (0)
/** @brief Draw inside the user clipping area */
# define JO_VDP1_WINDOW_IN                  (2 << 9)
/** @brief Draw outside the user clipping area */
# define JO_VDP1_WINDOW_OUT                 (3 << 9)
/** @brief Mesh (screen door) */
# define JO_VDP1_MESH                       (1 << 8)
/** @brief No mesh */
# define JO_VDP1_NO_MESH                    (0)
/** @brief Disable end code (texture) */
# define JO_VDP1_END_CODE_DISABLE           (1 << 7)
/** @brief End code of the RGB textures: the VDP1 stops drawing a line after 2 end codes (see jo_sprite_add_end_codes()) */
# define JO_VDP1_RGB_END_CODE               (0x7FFF)
/** @brief Disable transparent pixel (texture) */
# define JO_VDP1_TRANSPARENT_PIXEL_DISABLE  (1 << 6)
/** @brief Texture color mode: 16 colors (color bank) */
# define JO_VDP1_COLOR_MODE_16_BANK         (0 << 3)
/** @brief Texture color mode: 16 colors (lookup table) */
# define JO_VDP1_COLOR_MODE_16_LOOKUP       (1 << 3)
/** @brief Texture color mode: 64 colors (color bank) */
# define JO_VDP1_COLOR_MODE_64_BANK         (2 << 3)
/** @brief Texture color mode: 128 colors (color bank) */
# define JO_VDP1_COLOR_MODE_128_BANK        (3 << 3)
/** @brief Texture color mode: 256 colors (color bank) */
# define JO_VDP1_COLOR_MODE_256_BANK        (4 << 3)
/** @brief Texture color mode: RGB (32768 colors) */
# define JO_VDP1_COLOR_MODE_RGB             (5 << 3)
/** @brief Color mode mask */
# define JO_VDP1_COLOR_MODE_MASK            (7 << 3)
/** @brief Color calculation: replace */
# define JO_VDP1_COLOR_CALC_REPLACE         (0)
/** @brief Color calculation: shadow */
# define JO_VDP1_COLOR_CALC_SHADOW          (1)
/** @brief Color calculation: half luminance */
# define JO_VDP1_COLOR_CALC_HALF_LUMINANCE  (2)
/** @brief Color calculation: half transparent */
# define JO_VDP1_COLOR_CALC_HALF_TRANSPARENT (3)
/** @brief Color calculation: gouraud shading */
# define JO_VDP1_COLOR_CALC_GOURAUD         (4)
/** @brief Color calculation mask */
# define JO_VDP1_COLOR_CALC_MASK            (7)
/** @brief No gouraud table */
# define JO_VDP1_NO_GOURAUD                 (0)
/** @brief No palette (color) */
# define JO_VDP1_NO_PALETTE                 (0)

/*
** Texture size: bytes = width * height * 4 >> shift
*/
/** @brief Texture size shift for 16 colors textures (4 bits per pixel) */
# define JO_VDP1_TEXTURE_SIZE_SHIFT_16_COLORS       (3)
/** @brief Texture size shift for 64, 128 and 256 colors textures (8 bits per pixel) */
# define JO_VDP1_TEXTURE_SIZE_SHIFT_256_COLORS      (2)
/** @brief Texture size shift for RGB textures (16 bits per pixel) */
# define JO_VDP1_TEXTURE_SIZE_SHIFT_32K_COLORS      (1)

/** @brief Option (bits 16-23 of the direction attribute): the polygon is textured */
# define JO_VDP1_OPTION_USE_TEXTURE         (1 << 2)

/*
** Direction attribute: drawing function | flip | (options << 16) | (draw mode bits << 24)
*/
/** @brief Textured sprite without flip */
# define JO_VDP1_SPRITE_NO_FLIP             ((0) | JO_VDP1_FUNCTION_TEXTURE | (JO_VDP1_OPTION_USE_TEXTURE << 16))
/** @brief Textured sprite with horizontal flip */
# define JO_VDP1_SPRITE_H_FLIP              ((1 << 4) | JO_VDP1_FUNCTION_TEXTURE | (JO_VDP1_OPTION_USE_TEXTURE << 16))
/** @brief Textured sprite with vertical flip */
# define JO_VDP1_SPRITE_V_FLIP              ((1 << 5) | JO_VDP1_FUNCTION_TEXTURE | (JO_VDP1_OPTION_USE_TEXTURE << 16))
/** @brief Textured sprite with horizontal and vertical flip */
# define JO_VDP1_SPRITE_HV_FLIP             ((3 << 4) | JO_VDP1_FUNCTION_TEXTURE | (JO_VDP1_OPTION_USE_TEXTURE << 16))
/** @brief Flat color polygon */
# define JO_VDP1_POLYGON                    (JO_VDP1_FUNCTION_POLYGON | ((JO_VDP1_END_CODE_DISABLE | JO_VDP1_TRANSPARENT_PIXEL_DISABLE) << 24))
/** @brief Flat color polyline */
# define JO_VDP1_POLYLINE                   (JO_VDP1_FUNCTION_POLYLINE | ((JO_VDP1_END_CODE_DISABLE | JO_VDP1_TRANSPARENT_PIXEL_DISABLE) << 24))
/** @brief Flat color line */
# define JO_VDP1_LINE                       (JO_VDP1_FUNCTION_LINE | ((JO_VDP1_END_CODE_DISABLE | JO_VDP1_TRANSPARENT_PIXEL_DISABLE) << 24))

/** @brief Command bits of a direction attribute (drawing function and flip)
 *  @param DIRECTION JO_VDP1_SPRITE_NO_FLIP, JO_VDP1_POLYGON, etc
 */
# define JO_VDP1_DIRECTION_COMMAND(DIRECTION)   ((DIRECTION) & (JO_VDP1_DIRECTION_FUNCTION_MASK | JO_VDP1_DIRECTION_FLIP_MASK))
/** @brief Options of a direction attribute (JO_VDP1_OPTION_USE_TEXTURE)
 *  @param DIRECTION JO_VDP1_SPRITE_NO_FLIP, JO_VDP1_POLYGON, etc
 */
# define JO_VDP1_DIRECTION_OPTIONS(DIRECTION)   (((DIRECTION) >> 16) & 0x1c)
/** @brief Draw mode bits of a direction attribute (JO_VDP1_END_CODE_DISABLE, JO_VDP1_TRANSPARENT_PIXEL_DISABLE)
 *  @param DIRECTION JO_VDP1_SPRITE_NO_FLIP, JO_VDP1_POLYGON, etc
 */
# define JO_VDP1_DIRECTION_DRAW_MODE(DIRECTION) (((DIRECTION) >> 24) & (JO_VDP1_END_CODE_DISABLE | JO_VDP1_TRANSPARENT_PIXEL_DISABLE))

/** @brief Sprite attribute */
typedef struct
{
    unsigned short          texture_id;     // Sprite id (__jo_sprite_def index)
    unsigned short          draw_mode;      // JO_VDP1_COLOR_MODE_xxx | JO_VDP1_COLOR_CALC_xxx | ...
    unsigned short          color;          // RGB color or palette
    unsigned short          gouraud_table;  // Gouraud table address / 8
    unsigned short          direction;      // JO_VDP1_FUNCTION_xxx | flip
}                           jo_vdp1_sprite_attribute;

/** @brief Sprite attribute initializer
 *  @param TEXTURE Sprite id
 *  @param COLOR RGB color or palette
 *  @param GOURAUD Gouraud table address / 8 (JO_VDP1_NO_GOURAUD)
 *  @param DRAW_MODE JO_VDP1_COLOR_MODE_xxx | JO_VDP1_COLOR_CALC_xxx | ...
 *  @param DIRECTION JO_VDP1_SPRITE_NO_FLIP, JO_VDP1_POLYGON, etc
 */
# define JO_VDP1_SPRITE_ATTRIBUTE(TEXTURE, COLOR, GOURAUD, DRAW_MODE, DIRECTION) \
    {(TEXTURE), (DRAW_MODE) | JO_VDP1_DIRECTION_DRAW_MODE(DIRECTION), (COLOR), (GOURAUD), JO_VDP1_DIRECTION_COMMAND(DIRECTION)}

/** @brief Sprite position (internal usage) */
typedef struct
{
    jo_fixed                x;
    jo_fixed                y;
    jo_fixed                z;
    jo_fixed                scale_x;        // Scale (negative value: perspective scale)
    jo_fixed                scale_y;        // Vertical scale (only for __jo_vdp1_draw_sprite_hv())
}                           jo_vdp1_sprite_position;

/** @brief Gouraud tables used for 3D lighting (32 brightness levels, 16 is neutral) */
# define JO_VDP1_LIGHT_TABLE_OFFSET         (0x7FF00)
/** @brief Gouraud tables of the parts of the 3D polygons clipped by the near plane or subdivided near the camera
 *  (2 sets: one per frame) */
# define JO_VDP1_CLIP_GOURAUD_TABLE_OFFSET  (0x7E700)
/** @brief Gouraud tables of the clipped and subdivided 3D polygons (8 bytes each) */
# define JO_VDP1_CLIP_GOURAUD_TABLE_COUNT   (768)

/** @brief 3D window (internal usage) */
typedef struct
{
    short                   left;
    short                   top;
    short                   right;
    short                   bottom;
    short                   center_x;
    short                   center_y;
    jo_fixed                   zlimit;
    unsigned short          clip_mode;      // Draw mode bits of the 3D polygons: JO_VDP1_WINDOW_IN (clipped to the window), 0
    short                   origin_x;       // The local coordinates of the window (its commands are relative to them): its top
    short                   origin_y;       // left corner when it is a screen of its own (see screen), else 0
    bool                    screen;         // Its rectangle is its screen (clipped, windows apart: see jo_3d_window_clip())
}                           jo_vdp1_window;

/** @brief 3D windows (internal usage) */
/** @brief Number of 3D windows: near (default), far, and a second far window (see jo_3d_select_window()) */
# define JO_VDP1_WINDOW_COUNT       (3)
extern jo_vdp1_window       __jo_vdp1_windows[JO_VDP1_WINDOW_COUNT];

/** @brief Current 3D window (internal usage) */
extern int                  __jo_vdp1_current_window;

/** @brief Projection distance in Q16.16 (internal usage, see jo_3d_perspective_angle()) */
extern jo_fixed                __jo_vdp1_projection_distance;

/** @brief Near clipping divisor (internal usage, see jo_3d_display_level()) */
extern unsigned short       __jo_vdp1_display_level;

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

/** @brief Draw the last frame again (from a VBLANK callback while the main loop doesn't run: a loading). A texture of
 *  this frame changed by the callback is seen (the sand of an hourglass)
 *  @remarks The manual change of the frame buffers (JO_FRAMERATE > 1) draws a frame only once: the frame buffers change
 *  at the end of this VBLANK if the VDP1 finished the frame. Don't call it while the main loop sends frames
 */
void                        jo_vdp1_redraw_last_frame(void);

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

/** @brief Biggest depth key */
# define JO_VDP1_MAX_KEY            (0xffff)

/** @brief Depth added to the keys of the commands (see jo_3d_set_depth_bias()) */
extern int                  __jo_vdp1_depth_bias;

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

/** @brief Near clipping distance (internal usage, see __jo_vdp1_near_clip()) */
extern jo_fixed             __jo_vdp1_near_z;

/** @brief Wireframe mode (internal usage, see jo_3d_set_wireframe()) */
extern bool                 __jo_vdp1_wireframe;
/** @brief The windows drawn one after the other (internal usage, see jo_3d_set_windows_apart()) */
extern bool                 __jo_vdp1_windows_apart;

/** @brief Compute the near clipping distance (internal usage)
 *  @remarks Must be called when __jo_vdp1_projection_distance or __jo_vdp1_display_level change
 */
static  __jo_force_inline void __jo_vdp1_update_near_clip(void)
{
    // Variable shift: computed once instead of for each sprite (the SH-2 can only shift by 1, 2, 8 or 16 bits)
    __jo_vdp1_near_z = __jo_vdp1_projection_distance >> (__jo_vdp1_display_level ? __jo_vdp1_display_level : 1);
}

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

/** @brief Set the current 3D window (internal usage)
 *  @param left Left clipping position (screen pixels)
 *  @param top Top clipping position (screen pixels)
 *  @param right Right clipping position (screen pixels)
 *  @param bottom Bottom clipping position (screen pixels)
 *  @param zlimit Far clipping distance
 *  @param center_x Horizontal position of the projection center (screen pixels)
 *  @param center_y Vertical position of the projection center (screen pixels)
 *  @remarks Select the window with __jo_vdp1_select_window() first
 */
void                        __jo_vdp1_set_window(const short left, const short top, const short right, const short bottom,
                                                 const short zlimit, const short center_x, const short center_y);

/** @brief Depth limit of the current window (the far clipping distance of jo_3d_window()): the polygons beyond it are not
 *  drawn (all their vertices farther, in the view space). A wall that fills the screen: nothing behind it is seen, the
 *  polygons of the room behind it cost VDP1 time for nothing (up to 50 ms against a wall)
 *  @param z Depth limit (world units), -1: none (the default)
 *  @remarks Set it for the frame (the slave CPU draws in the same window); the mirrors use the far windows
 */
void                        jo_3d_set_depth_limit(const int z);

/** @brief The frame buffer is cleared at the start of each frame (see jo_3d_set_frame_clear()) */
extern bool                 __jo_vdp1_frame_clear;

/** @brief Clear the frame buffer at the start of each frame (the default), or not
 *  @param enabled false: the frame buffer is not cleared, what the frame does not draw keeps the pixels of the frame
 *  drawn 2 frames before (the frame buffers alternate)
 *  @remarks With JO_FRAMERATE > 1 (the frame buffers change when the VDP1 finished a frame) the frame buffer is cleared
 *  by a polygon of the transparent color that costs the VDP1 about 4 ms: a program that covers the whole screen at each
 *  frame (the rooms of a first person game) does not need it. The last pixel of a line between 2 polygons is the one
 *  of the older frame instead of the background. With JO_FRAMERATE 1 the VDP1 erases the frame buffer itself
 *  @remarks From the next frame
 */
static  __jo_force_inline void jo_3d_set_frame_clear(const bool enabled)
{
    __jo_vdp1_frame_clear = enabled;
}

/** @brief The 3D polygons of the current window clipped to its rectangle (internal usage, see jo_3d_window_clip()) */
void                        __jo_vdp1_set_window_clip(const bool clip);

/** @brief Select the 3D window used by the next commands (internal usage)
 *  @param window 0 (near window), 1 (far window) or 2 (second far window)
 */
static  __jo_force_inline void __jo_vdp1_select_window(const int window)
{
    __jo_vdp1_current_window = (window >= 0 && window < JO_VDP1_WINDOW_COUNT) ? window : 0;
}

/** @brief VDP1 commands of the last frame (the polygons, their parts, the sprites: see jo_vdp1_command_max()) */
int                         jo_vdp1_command_count(void);

/** @brief VDP1 commands of the frame being made by the master CPU, read without the cache (internal usage: the budget of
 *  the subdivisions, see 3d.c; the commands of the slave CPU are added at jo_3d_slave_end()) */
int                         __jo_vdp1_frame_command_count(void);

/** @brief Most VDP1 commands in a frame (the next ones are not drawn) */
int                         jo_vdp1_command_max(void);

#endif /* !__JO_VDP1_COMMAND_PIPELINE_H__ */

/*
** END OF FILE
*/
