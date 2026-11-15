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

/** @file water.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine water: a sea with waves around the camera (see below), objects seen through it, floating hulls
 *  @remarks With the 3D and the 2 CPUs (JO_COMPILE_WITH_3D_MODULE, the dual CPU support: on by default); in the program
 *  only when the game uses it (the link drops what is not called: its built-in patterns too, see
 *  tools/make_water_patterns.py)
 *  @bug No known bugs.
 */

#ifndef __JO_WATER_H__
# define __JO_WATER_H__

#if defined(JO_COMPILE_WITH_3D_SUPPORT) && defined(JO_COMPILE_WITH_DUAL_CPU_SUPPORT)

/*
** Water: a sea with waves around the camera, drawn by the VDP1 (quads with a light per vertex), seen through (the VDP2
** mixes the water with the screen behind it).
**
** - the swell: a sum of waves (jo_water_wave: length, direction, height, period), changed at any time (jo_water_set_waves(),
**   jo_water_set_swell()): calm or rough sea, wind from any direction
** - the surface: jo_water_sample() gives the height and the slopes at any point, for the objects on the water (a jet ski, a
**   buoy, a crate): the same waves as the drawn surface
** - events: jo_water_drop() (a ring of waves where an object falls), jo_water_wave_front() (a straight wave that goes away)
** - the grid: levels of quads around the camera, small quads near it, bigger and bigger farther (jo_water_level), then a flat
**   skirt to the horizon. The first levels are computed by the slave CPU, the far levels every 2 or 4 frames
** - objects under the water: any model seen through the water, sorted with it (jo_water_add_model(), see below), a floating
**   hull with its part in the water seen through it (jo_water_add_dip())
**
** Units: 1 unit = 1/16 m. Y goes down (the calm surface is at y = 0, a crest at y < 0); the heights are up (> 0: a crest).
** Time: in frames at 60 per second (the frame given to jo_water_update(): jo_core_get_vblank_count()).
**
** Usage:
**     jo_water_config config;
**     jo_water_default_config(&config);
**     config.levels = my_levels; ...              (change what you need)
**     jo_water *sea = jo_water_create(&config);
**     each frame: jo_water_update(sea, frame, camera position, heading); then after jo_3d_camera_look_at(): jo_water_draw(sea)
*/

/** @brief Waves at most in the swell */
# define JO_WATER_MAX_WAVES     (8)
/** @brief Levels at most */
# define JO_WATER_MAX_LEVELS    (6)
/** @brief Quads on a side of a level at most */
# define JO_WATER_MAX_CELLS     (24)

/** @brief A wave of the swell (a sine along a direction) */
typedef struct
{
    int                     length;         // Wavelength (units). At least 4 quads of a level: on bigger quads, it is not drawn
    int                     direction;      // Where it goes (degrees, 0: toward +z, 90: toward +x). Not along the grid (0, 90...): crests along lines of vertices make bands
    jo_fixed                height;         // Height of the crests (jo_fixed, units)
    int                     period;         // Time of a period (frames), its speed: length / period
}                           jo_water_wave;

/** @brief A level of the grid */
typedef struct
{
    int                     cell;           // Side of its quads (units): the same as the previous level or a multiple of it
    int                     side;           // Side of the level (units): a multiple of its quads and of the quads of the next level,
                                            // at least the side of the previous level + 4 quads of the next level
}                           jo_water_level;

/** @brief Gouraud steps per channel (16: the color as it is) */
typedef struct
{
    signed char             r;
    signed char             g;
    signed char             b;
}                           jo_water_steps;

/** @brief Configuration of the water (jo_water_default_config(), then change what you need) */
typedef struct
{
    /* The grid */
    const jo_water_level    *levels;        // From the camera (copied)
    int                     level_count;    // JO_WATER_MAX_LEVELS at most
    int                     ahead;          // Center of the levels in front of the camera (units)
    int                     update_cell;    // Levels with bigger quads: updated every 2 frames, bigger than 2 x: every 4 frames
    int                     min_cells_per_wave; // A wave is drawn on the levels with at least this many quads per wavelength
    int                     fade;           // The waves fade out toward the edge of the last level that draws them (units)
    int                     skirt;          // The flat sea around the levels: up to this distance (units, 32767 at most)
    int                     slave_levels;   // The first levels drawn by the slave CPU (see jo_water_draw_with(); the others and the
                                            // objects by the master CPU)
    /* The swell */
    const jo_water_wave     *waves;         // (copied; JO_NULL: the standard swell JO_WATER_SWELL_CHOPPY, see jo_water_swell())
    int                     wave_count;     // JO_WATER_MAX_WAVES at most
    /* The look */
    jo_color                color;          // Color of the water (the light of the vertices changes it)
    jo_water_steps          shallow;        // Light where the camera looks into the water (the face of a wave toward the camera)
    jo_water_steps          deep;           // Light at a low angle (far away, the back of a wave)
    int                     sky;            // Reflection of the sky at a low angle (steps, pale blue)
    int                     slope_light_min;// Light of the slopes away from the sun (steps, < 0)
    int                     slope_light_max;// Light of the slopes toward the sun (steps)
    int                     glint;          // White glints of the slopes toward the sun (steps for a slope of 1)
    int                     spot;           // Spot of the sun on the sea (steps at its center)
    int                     spot_distance;  // Distance of the spot from the camera with a sun 45 degrees high (units)
    int                     caustics;       // Moving light of the waves near the camera (steps, 0: none)
    int                     reflection;     // White streaks of the clouds reflected by the waves (steps at most, 0: none)
    int                     foam_white;     // Light of the foam (steps at full foam, see jo_water_foam())
    jo_color                foam_color;     // White of the foam texture (the light of the vertices is added: under 31 - 10 per
                                            // channel, the VDP1 doesn't saturate the gouraud)
    jo_water_steps          shore;          // Light added in the shallow water (see jo_water_set_depth_map(): at depth 0)
    int                     shallow_depth;  // Depth (0 to 255) where the shallow water starts
    int                     surf;           // Foam of the waves on the shore (0 to 255 at depth 0, 0: none)
    int                     bottom;         // The bottom seen in the shallow water: a texture on the quads near the shore (with
                                            // textured), its lines of light that move (steps at depth 0, 0: none; set
                                            // before jo_water_create(): its tiles are made there)
    int                     bottom_depth;   // Depth (0 to 255) where the texture of the bottom stops
    int                     see_through;    // The water shallower than this (0 to 255, 0: none) is half transparent: the VDP1
                                            // mixes it with the RGB polygons drawn behind it (the bottom near the shore,
                                            // sorted behind the water), not with the rest (the VDP2 mixes the water with
                                            // its bottom of the sea)
    bool                    textured;       // The foam texture on the quads near the camera where there is foam (else the light of
                                            // the vertices only)
    bool                    grain;          // Without the texture: a light grain (hides the steps of the gouraud): GRAIN.TGA at the
                                            // root of the CD (with the TGA module)
    const char              *patterns_file; // JO_NULL: the built-in patterns of the foam and of the bottom (in the program, see
                                            // tools/make_water_patterns.py); else a level file with them (the section 'foam':
                                            // tools/make_water_patterns.py --jol), read when the water is made
    /* The events */
    int                     splash_length;  // Wavelength of the rings of jo_water_drop() (units)
    int                     splash_speed;   // Units per frame
    int                     splash_life;    // Frames
    int                     front_length;   // Wavelength of jo_water_wave_front() (units)
    int                     front_speed;    // Units per frame
    int                     front_life;     // Frames
}                           jo_water_config;

/** @brief Height and slopes of the surface at a point (see jo_water_sample()) */
typedef struct
{
    jo_fixed                height;         // > 0: above the calm sea (the surface is at y = -height)
    jo_fixed                slope_x;        // Height change per unit along x (jo_fixed, the normal is (-slope_x, 1, -slope_z))
    jo_fixed                slope_z;        // Along z
}                           jo_water_point;

/** @brief The water (see jo_water_create()) */
typedef struct s_jo_water      jo_water;

/** @brief A configuration that works: 4 levels (quads of 8, 16, 32, 64 units), the swell JO_WATER_SWELL_CHOPPY, the look
 *  JO_WATER_LOOK_DEFAULT
 *  @param config Configuration (output)
 */
void        jo_water_default_config(jo_water_config * const config);

/*
** Standard looks of the water and standard swells: inline helpers (no table: a constant look or swell costs only its
** values in the code where it is used, nothing when it is not used).
**
**     jo_water_default_config(&config);
**     jo_water_config_look(&config, JO_WATER_LOOK_TROPICAL_TEAL);
**     config.wave_count = jo_water_swell(waves, JO_WATER_SWELL_GENTLE);      (jo_water_wave waves[JO_WATER_MAX_WAVES]: copied by
**     config.waves = waves;                                              jo_water_create())
**     sea = jo_water_create(&config);
**     jo_water_blend_with_bottom(jo_water_look_blend(JO_WATER_LOOK_TROPICAL_TEAL));
*/

/** @brief The standard looks of the water (its color and the light of its waves, see jo_water_config_look()) */
typedef enum
{
    JO_WATER_LOOK_DEFAULT,             // Clear blue (jo_water_default_config())
    JO_WATER_LOOK_MINT_LAGOON,         // Pale green lagoon over white sand
    JO_WATER_LOOK_TROPICAL_TEAL,       // Blue green of the tropics
    JO_WATER_LOOK_MERMAID_BLUE,        // Bright blue lagoon
    JO_WATER_LOOK_OCEAN_BLUE,          // The open sea
    JO_WATER_LOOK_FRENCH_BLUE,         // Deep saturated blue
    JO_WATER_LOOK_DEEP_CONTRAST,       // Dark blue, strong light on the waves
    JO_WATER_LOOK_BRIGHT_TURQUOISE,    // Bright turquoise, dark slopes
    JO_WATER_LOOK_CARIBBEAN,           // Light blue, very clear
    JO_WATER_LOOK_DEEP_PACIFIC,        // Deep navy blue
    JO_WATER_LOOK_SUNNY_BEACH,         // Turquoise of a sunny beach
    JO_WATER_LOOK_COUNT
}                           jo_water_look_kind;

/** @brief Set the values of a look (see jo_water_config_look()) */
static  __jo_force_inline void  __water_set_look(jo_water_config * const config, const jo_color color, const signed char shallow_r,
                                                 const signed char shallow_g, const signed char shallow_b, const signed char deep_r,
                                                 const signed char deep_g, const signed char deep_b, const int sky,
                                                 const int slope_light_min, const int slope_light_max, const int glint)
{
    config->color = color;
    config->shallow.r = shallow_r;
    config->shallow.g = shallow_g;
    config->shallow.b = shallow_b;
    config->deep.r = deep_r;
    config->deep.g = deep_g;
    config->deep.b = deep_b;
    config->sky = sky;
    config->slope_light_min = slope_light_min;
    config->slope_light_max = slope_light_max;
    config->glint = glint;
}

/** @brief Set a standard look in a configuration: color, shallow, deep, sky, slope_light_min, slope_light_max, glint (the
 *  rest as it is; for a water already made: then jo_water_set_look())
 *  @param config Configuration (jo_water_default_config() first)
 *  @param look The look
 */
static  __jo_force_inline void  jo_water_config_look(jo_water_config * const config, const jo_water_look_kind look)
{
    switch (look)
    {
        case JO_WATER_LOOK_MINT_LAGOON:
            __water_set_look(config, JO_COLOR_RGB(96, 196, 176), 4, 12, 10, 0, 4, 8, 4, -11, 6, 40);
            break;
        case JO_WATER_LOOK_TROPICAL_TEAL:
            __water_set_look(config, JO_COLOR_RGB(40, 168, 168), 6, 16, 16, 0, 4, 12, 5, -12, 7, 44);
            break;
        case JO_WATER_LOOK_MERMAID_BLUE:
            __water_set_look(config, JO_COLOR_RGB(32, 148, 192), 6, 16, 18, 0, 2, 12, 5, -12, 7, 44);
            break;
        case JO_WATER_LOOK_OCEAN_BLUE:
            __water_set_look(config, JO_COLOR_RGB(24, 108, 200), 6, 14, 18, 0, 0, 10, 6, -12, 8, 48);
            break;
        case JO_WATER_LOOK_FRENCH_BLUE:
            __water_set_look(config, JO_COLOR_RGB(32, 72, 200), 8, 12, 20, 0, 0, 8, 7, -12, 8, 48);
            break;
        case JO_WATER_LOOK_DEEP_CONTRAST:
            __water_set_look(config, JO_COLOR_RGB(24, 88, 168), 6, 18, 22, 0, 2, 10, 6, -14, 10, 60);
            break;
        case JO_WATER_LOOK_BRIGHT_TURQUOISE:
            __water_set_look(config, JO_COLOR_RGB(40, 176, 200), 4, 14, 14, 0, 0, 8, 4, -15, 8, 50);
            break;
        case JO_WATER_LOOK_CARIBBEAN:
            __water_set_look(config, JO_COLOR_RGB(88, 184, 216), 4, 12, 12, 0, 2, 6, 6, -12, 6, 40);
            break;
        case JO_WATER_LOOK_DEEP_PACIFIC:
            __water_set_look(config, JO_COLOR_RGB(16, 64, 128), 8, 18, 24, 2, 4, 14, 7, -10, 10, 56);
            break;
        case JO_WATER_LOOK_SUNNY_BEACH:
            __water_set_look(config, JO_COLOR_RGB(40, 172, 184), 5, 16, 15, 1, 6, 10, 6, -9, 7, 36);
            break;
        default:
            __water_set_look(config, JO_COLOR_RGB(64, 136, 168), 6, 19, 22, 6, 8, 18, 6, -7, 5, 40);
            break;
    }
}

/** @brief The mix of the water with the bottom of the sea that goes with a standard look (see jo_water_blend_with_bottom())
 *  @param look The look
 *  @return Of the bottom: 0 (the water only) to 31 (almost only the bottom)
 */
static  __jo_force_inline int   jo_water_look_blend(const jo_water_look_kind look)
{
    switch (look)
    {
        case JO_WATER_LOOK_MINT_LAGOON:
        case JO_WATER_LOOK_TROPICAL_TEAL:
        case JO_WATER_LOOK_MERMAID_BLUE:
            return (10);
        case JO_WATER_LOOK_OCEAN_BLUE:
        case JO_WATER_LOOK_FRENCH_BLUE:
        case JO_WATER_LOOK_BRIGHT_TURQUOISE:
            return (8);
        case JO_WATER_LOOK_DEEP_CONTRAST:
        case JO_WATER_LOOK_DEEP_PACIFIC:
            return (6);
        case JO_WATER_LOOK_CARIBBEAN:
            return (14);
        case JO_WATER_LOOK_SUNNY_BEACH:
            return (7);
        default:
            return (12);
    }
}

/** @brief The water seen through: the VDP2 mixes the RGB pixels of the sprites (the water, the objects under it, see
 *  jo_water_add_model()) with the screen behind them (the bottom of the sea: RBG0 for instance); the pixels of the palette stay
 *  opaque (the objects above the water: their colors in the color RAM with the MSB at 0)
 *  @param blend Of the bottom: 0 (the water only) to 31 (almost only the bottom), see jo_water_look_blend()
 *  @remarks jo_vdp2_set_sprite_blend_ratios() after it for other ratios of palette pixels (its ratio 0: blend)
 */
static  __jo_force_inline void  jo_water_blend_with_bottom(const int blend)
{
    jo_vdp2_set_blend_mode(JO_VDP2_BLEND_RATIO);
    jo_vdp2_blend_rgb_sprites_only(true);
    jo_vdp2_enable_blend(JO_SPRITE_SCREEN, (unsigned char)blend);
}

/** @brief The standard swells (see jo_water_swell()): waves of 130 to 700 units, for quads of 16 to 64 units near the camera
 *  (jo_water_config.levels: a wave is drawn on the levels with min_cells_per_wave quads per wavelength) */
typedef enum
{
    JO_WATER_SWELL_GENTLE,             // A sunny bay: a long low swell, small waves (crests of 1 to 3 units)
    JO_WATER_SWELL_CHOPPY,             // A rougher sea (the swell of jo_water_default_config(): crests of 2 to 9 units)
    JO_WATER_SWELL_STORM,              // A storm (crests of 3 to 16 units)
    JO_WATER_SWELL_COUNT
}                           jo_water_swell_kind;

/** @brief Set a wave (see jo_water_swell()) */
static  __jo_force_inline void  __water_set_wave(jo_water_wave * const wave, const int length, const int direction, const jo_fixed height,
                                                 const int period)
{
    wave->length = length;
    wave->direction = direction;
    wave->height = height;
    wave->period = period;
}

/** @brief A standard swell (its directions not along the grid of the water)
 *  @param waves The waves (output: JO_WATER_MAX_WAVES; jo_water_create() and jo_water_set_waves() copy them)
 *  @param swell The swell
 *  @return The number of waves
 */
static  __jo_force_inline int   jo_water_swell(jo_water_wave * const waves, const jo_water_swell_kind swell)
{
    switch (swell)
    {
        case JO_WATER_SWELL_GENTLE:
            __water_set_wave(&waves[0], 640, 12, 3 << 16, 320);
            __water_set_wave(&waves[1], 350, 56, (3 << 16) / 2, 230);
            __water_set_wave(&waves[2], 190, -36, 1 << 16, 160);
            __water_set_wave(&waves[3], 150, 78, (3 << 16) / 4, 120);
            return (4);
        case JO_WATER_SWELL_STORM:
            __water_set_wave(&waves[0], 700, -20, 16 << 16, 280);
            __water_set_wave(&waves[1], 420, 25, 9 << 16, 200);
            __water_set_wave(&waves[2], 240, -60, 5 << 16, 150);
            __water_set_wave(&waves[3], 160, 70, 4 << 16, 110);
            __water_set_wave(&waves[4], 130, -15, 3 << 16, 90);
            return (5);
        default:
            __water_set_wave(&waves[0], 600, 10, 9 << 16, 300);
            __water_set_wave(&waves[1], 330, 52, 4 << 16, 220);
            __water_set_wave(&waves[2], 180, -38, 2 << 16, 160);
            __water_set_wave(&waves[3], 150, 74, 3 << 16, 110);
            __water_set_wave(&waves[4], 140, -66, (5 << 16) / 2, 100);
            return (5);
    }
}


/** @brief Create the water: load the textures, create the grid
 *  @param config Configuration (copied)
 *  @return JO_NULL if the configuration is wrong (a message on the screen), a texture is missing or out of memory
 *  @remarks One water at a time (the jobs of the slave CPU)
 */
jo_water    *jo_water_create(const jo_water_config * const config);

/** @brief Move the waves to the time of the frame and the grid around the camera, compute the heights and the light
 *  @param frame Time (frames, 60 per second: jo_core_get_vblank_count(), the waves move at the same speed at any frame rate)
 *  @param x Camera position (units)
 *  @param y Camera position (units, y down)
 *  @param z Camera position (units)
 *  @param heading Direction of the camera (jo_angle, 0 = toward +z)
 */
void        jo_water_update(jo_water * const w, const int frame, const int x, const int y, const int z, const jo_angle heading);

/** @brief Draw the water (after jo_3d_camera_look_at()) */
void        jo_water_draw(jo_water * const w);

/** @brief Draw the water and the objects on it: the master CPU draws some objects while the slave CPU draws the water near
 *  the camera, then other objects (the time of the drawing on both CPUs: the slave CPU computes fewer waves, see the
 *  balance of jo_water_update())
 *  @param objects Draws objects on the master CPU (jo_3d_mesh_draw()...), JO_NULL: none
 *  @param objects_on_slave Draws objects on the slave CPU after the water (jo_3d_slave_mesh_draw()..., the matrix of the
 *  camera; the slave CPU uses the light set before jo_water_draw_with()), JO_NULL: none
 */
void        jo_water_draw_with(jo_water * const w, void (*objects)(void), void (*objects_on_slave)(void));

/** @brief Side of the quads of the water drawn at a point (the finest level that covers it, see jo_water_config.levels): an
 *  object seen through the water (under it) is sorted nearer than the point where it is seen by about half of it (a quad
 *  is sorted at its center)
 *  @param x Position (units)
 *  @param z Position (units)
 *  @return Side (units); the side of the last level outside of the levels
 */
int         jo_water_quad_side(const jo_water * const w, const int x, const int z);

/** @brief Height of the surface at a point (the waves, the splashes, the wave fronts at the time of jo_water_update())
 *  @param x Position (units)
 *  @param z Position (units)
 *  @return Height (jo_fixed units, > 0 above the calm sea)
 */
jo_fixed    jo_water_height(const jo_water * const w, const int x, const int z);

/** @brief Height of the surface as it is drawn at a point (the vertices of the finest level that covers it, interpolated):
 *  cheap (no sine), for many points (along a line of sight); the waves (jo_water_wave_height()) outside of the levels
 *  @param x Position (units)
 *  @param z Position (units)
 *  @return Height (jo_fixed units, > 0 above the calm sea)
 *  @remarks After jo_water_update(); only the vertices in the view are updated (see the span culling): a point out of the
 *  view of the camera may give an old height
 */
jo_fixed    jo_water_drawn_height(const jo_water * const w, const int x, const int z);

/** @brief Height of the waves at a point, without the splashes and the wave fronts (faster than jo_water_height(): many
 *  points, for instance along a line of sight)
 *  @param x Position (units)
 *  @param z Position (units)
 *  @return Height (jo_fixed units, > 0 above the calm sea)
 */
jo_fixed    jo_water_wave_height(const jo_water * const w, const int x, const int z);

/** @brief Height and slopes of the surface at a point: for an object on the water (its height, its tilt)
 *  @param x Position (units)
 *  @param z Position (units)
 *  @param point Height and slopes (output)
 */
void        jo_water_sample(const jo_water * const w, const int x, const int z, jo_water_point * const point);

/** @brief Change the swell (a calm sea, a storm, the wind turns)
 *  @param waves Waves (copied)
 *  @param count JO_WATER_MAX_WAVES at most
 */
void        jo_water_set_waves(jo_water * const w, const jo_water_wave * const waves, const int count);

/** @brief Strength of the swell: the heights of the waves x scale (a storm that grows, a calm that comes)
 *  @param scale jo_fixed (JO_FIXED_1: the heights of the waves as they are)
 */
void        jo_water_set_swell(jo_water * const w, const jo_fixed scale);

/** @brief An object falls in the water: a ring of waves goes away from the point (near the camera only)
 *  @param x Position (units)
 *  @param z Position (units)
 *  @param height Height of the first wave (units)
 */
void        jo_water_drop(jo_water * const w, const int x, const int z, const int height);

/** @brief A straight wave (2 crests, the second one the highest) that goes from a point in a direction: it grows, then
 *  fades out (a half sine over config.front_life frames; near the camera only)
 *  @param x Start (units)
 *  @param z Start (units)
 *  @param direction Direction (jo_angle, 0 = toward +z)
 *  @param height Height of the crests (units)
 *  @param width Width of the wave (units)
 */
void        jo_water_wave_front(jo_water * const w, const int x, const int z, const jo_angle direction, const int height, const int width);

/** @brief Foam on the water (a trail, a wake, a splash): a disc of the foam grid (cells of 8 units around the camera,
 *  1024 units: the foam out of it is lost), white at once, it fades out in life frames (the foam already there and longer
 *  is kept). The quads near the camera get the foam texture, more of it with more foam
 *  @param x Center (units)
 *  @param z Center (units)
 *  @param radius Units (8 at least: a cell)
 *  @param life Frames (60 per second, 1020 at most); the edge of the disc lives half as long (a soft edge)
 */
void        jo_water_foam(jo_water * const w, const int x, const int z, const int radius, const int life);

/** @brief The depth of the water around (a map of bytes: 0 the shore, 255 deep): the shallow water is lighter (config.shore),
 *  the waves break on the shore (config.surf)
 *  @param map size x size bytes, the row z then the column x (JO_NULL: no map, deep everywhere)
 *  @param size Cells on a side (a power of 2)
 *  @param shift A cell is 1 << shift units
 *  @param x Corner of the map (units)
 *  @param z Corner of the map (units)
 *  @remarks Out of the map: deep
 */
void        jo_water_set_depth_map(jo_water * const w, const unsigned char * const map, const int size, const int shift, const int x,
                                const int z);

/** @brief Foam at a point (0 to 255, see jo_water_foam()) */
int         jo_water_foam_at(const jo_water * const w, const int x, const int z);

/** @brief Position of the sun (the light, the glints, the spot)
 *  @param azimuth Direction (jo_angle, 0 = toward +z)
 *  @param elevation Height above the horizon (jo_angle)
 */
void        jo_water_set_sun(jo_water * const w, const jo_angle azimuth, const jo_angle elevation);

/** @brief The foam texture or not (see jo_water_config.textured) */
void        jo_water_set_textured(jo_water * const w, const bool textured);

/** @brief The configuration of the water (read only: the look can be changed with jo_water_set_look()) */
const jo_water_config  *jo_water_get_config(const jo_water * const w);

/** @brief The color of the water seen from above (its color with the light of config.shallow: the face of a wave toward the
 *  camera), 5 to 21 steps per channel (a gouraud of 16 +/- 5 stays in 0 to 31): the color of an object under the water is
 *  mixed with it
 *  @param object The color of the object
 *  @param amount Of the water (/ 256)
 */
jo_color    jo_water_seen_color(const jo_water * const w, const jo_color object, const int amount);

/** @brief Change the look (the colors and the light: color, shallow, deep, sky, slope_light_min/max, glint, spot,
 *  spot_distance, caustics, reflection, foam_white, foam_color, shore, shallow_depth, surf, bottom, bottom_depth,
 *  see_through, grain of the configuration)
 */
void        jo_water_set_look(jo_water * const w, const jo_water_config * const look);

/*
** Objects under the water: any mesh of the world seen through the water (a rock, a wreck, a fish, the legs of a pier, the
** part of an object under the surface). The VDP2 mixes the water with the bottom of the sea, not with the polygons drawn
** before it: an object under the water is drawn over the water, in its colors seen through the water (mixed with the
** color of the water seen from above, RGB: the VDP2 mixes them with the bottom like the water). The water module sorts
** each object where the line of sight goes into the water in front of it (the waves as drawn): in front of the water that
** covers it on the screen, behind the objects above the water in front of it (see jo_water_add_above()), the objects under
** the water that cover each other on the screen from the farthest. Its polygons keep their own order (their sort, as
** made: any shape). Drawn by the slave CPU after the water (see jo_water_draw_with()), a draw per object.
**
** Usage:
**     jo_3d_model *rocks = jo_3d_load_model("ROCKS.JOM");    (a group per rock: OBJ o or g)
**     jo_water_add_model(sea, rocks, 192, 1500, JO_NULL);        (once: the colors of the model become its colors seen
**                                                              through the water, draw it only this way)
**     each frame, before jo_water_draw_with(): jo_water_add_above(sea, jet ski...), jo_water_place() / jo_water_show() /
**     jo_water_reshape() of the objects that move
**
** A floating object (a hull): see jo_water_add_dip().
*/

/** @brief An object under the water (see jo_water_add_model()) */
typedef struct s_jo_water_object   jo_water_object;

/** @brief Objects under the water at most */
# define JO_WATER_MAX_OBJECTS   (48)
/** @brief Objects above the water given in a frame at most (see jo_water_add_above()) */
# define JO_WATER_MAX_ABOVE     (24)

/** @brief Put a model under the water: an object per group of the model (OBJ o or g: a rock, the foot of a pillar, a part of
 *  a wreck; the whole model if it has no group). Its colors become its colors seen through the water: RGB colors, colors
 *  of the palette, textures of 16 colors (a color lookup table of 32 bytes each, the pixels are not copied) and RGB
 *  textures (copied); its gouraud shading stays, not lit (the light of the sun under the water is diffuse)
 *  @param model Model (jo_3d_load_model(); its points in the world, or in the space of each object: see jo_water_place())
 *  @param amount Of the color of the water seen from above in its colors (/ 256: 0 its own colors)
 *  @param far Not drawn farther from the camera (units: |dx| + |dz|)
 *  @param objects The objects, a group each (output, JO_NULL: not needed)
 *  @return The number of objects, -1 if too many (JO_WATER_MAX_OBJECTS) or out of memory
 *  @remarks The mesh of the model is changed: draw it only through the jo_water
 */
int         jo_water_add_model(jo_water * const w, jo_3d_model * const model, const int amount, const int far, jo_water_object ** const objects);

/** @brief Put polygons of a mesh under the water (see jo_water_add_model()): a mesh made by the program, its points changed in
 *  any frame (see jo_water_reshape())
 *  @param mesh Mesh
 *  @param first First polygon of the object
 *  @param count Polygons
 *  @param amount Of the color of the water (/ 256)
 *  @param far Not drawn farther from the camera (units: |dx| + |dz|)
 *  @return JO_NULL if too many objects or out of memory
 */
jo_water_object *jo_water_add_mesh(jo_water * const w, jo_3d_mesh * const mesh, const int first, const int count, const int amount,
                                const int far);

/** @brief Place the space of an object (the space of the points of its mesh): the world by default
 *  @param x Position (units)
 *  @param y Position (units, y down)
 *  @param z Position (units)
 *  @param heading Turned around y (jo_angle, as jo_3d_rotate_matrix_angle_y())
 */
void        jo_water_place(jo_water_object * const o, const int x, const int y, const int z, const jo_angle heading);

/** @brief Show or hide an object, hide some of its polygons
 *  @param shown Drawn (when it is in the view and near enough)
 *  @param hidden Polygons not drawn: bit k for the polygon k of the object (the first 32)
 */
void        jo_water_show(jo_water_object * const o, const bool shown, const unsigned int hidden);

/** @brief The points of the mesh of an object changed: its size and its depth again (after jo_water_show(): only the
 *  polygons drawn) */
void        jo_water_reshape(jo_water_object * const o);

/** @brief Change the distance where an object is not drawn any more (units: |dx| + |dz|) */
void        jo_water_set_far(jo_water_object * const o, const int far);

/** @brief An object above the water in front of which the objects under the water stay (the jet ski, a buoy, a boat):
 *  an object under the water that it covers on the screen is sorted behind it. Call it each frame before
 *  jo_water_draw_with() (forgotten after it)
 *  @param x Its center (units)
 *  @param y Its center (units, y down)
 *  @param z Its center (units)
 *  @param radius Its size (units)
 *  @param key_offset Its farthest sort key from the depth of its center (units: its depth bias, its size)
 */
void        jo_water_add_above(jo_water * const w, const int x, const int y, const int z, const int radius, const int key_offset);

/** @brief An object on the water whose polygons under the surface are seen through the water (the hull of a boat, of a
 *  jet ski: see jo_water_add_dip()) */
typedef struct s_jo_water_dip  jo_water_dip;

/** @brief Depth levels of a jo_water_dip at most */
# define JO_WATER_DIP_LEVELS    (4)

/** @brief Prepare a floating object: its polygons under the surface take their colors seen through the water, deeper:
 *  more of the water (see jo_water_dip_mesh()); its own attributes are kept for the polygons above the jo_water
 *  @param mesh Mesh (its polygons as drawn above the water)
 *  @param levels Depth levels (JO_WATER_DIP_LEVELS at most)
 *  @param amounts Of the color of the water at each level (/ 256)
 *  @param depths Under the surface where each level starts (jo_fixed units of the mesh, the first one usually 0)
 *  @param palette_offset Added to the colors of the palette before they are read (a mesh lit by the palette light: its
 *         colors are its darkest shade, see jo_3d_set_palette_light(): + shade x stride, the shade seen under the water)
 *  @return JO_NULL if out of memory
 */
jo_water_dip   *jo_water_add_dip(jo_water * const w, jo_3d_mesh * const mesh, const int levels, const short * const amounts,
                           const jo_fixed * const depths, const int palette_offset);

/** @brief Set the colors of the polygons of a floating object from the depth of their middles under the surface (call it
 *  before it is drawn)
 *  @param surface The surface in the space of the mesh (jo_fixed units, y down: the height of the object - the height of
 *         the water)
 *  @param pitch Its pitch (jo_angle: turned around x after its roll, as drawn)
 *  @param roll Its roll (jo_angle: turned around z first)
 */
void        jo_water_dip_mesh(jo_water_dip * const d, const jo_fixed surface, const jo_angle pitch, const jo_angle roll);

#endif /* JO_COMPILE_WITH_3D_SUPPORT && JO_COMPILE_WITH_DUAL_CPU_SUPPORT */

#endif /* !__JO_WATER_H__ */

/*
** END OF FILE
*/
