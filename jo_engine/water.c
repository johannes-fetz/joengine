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

#include "jo/jo.h"

#if defined(JO_COMPILE_WITH_3D_SUPPORT) && defined(JO_COMPILE_WITH_DUAL_CPU_SUPPORT)

#include "internal/water.h"
#include "water_patterns.h"

/** @brief The patterns of the foam and of the bottom (used once, when the water is made): built in (water_patterns.h,
 *  tools/make_water_patterns.py) or in a level file (jo_water_config.patterns_file: the section of the game 'foam'): u16 the
 *  size of the pattern (JO_WATER_PATTERN_SIZE), u16 0, the foam pattern (4 bits), the half pattern (half the size), the
 *  bottom pattern */
# define FOAM_SECTION       JO_LEVEL_ID('f', 'o', 'a', 'm')
# define FOAM_PATTERN_SIZE  (JO_WATER_PATTERN_SIZE)
# define SKY_REFLECTION_SIZE    (JO_WATER_SKY_REFLECTION_SIZE)

/*
** The levels of the grid (a clipmap): square grids of quads around the camera, each one around the previous one (a hole in
** it where the previous one is, not drawn), its quads as big or bigger. Each level follows the camera by whole cells of
** the next level: its vertices are on the lines of vertices of the next level, its hole moves by whole cells in it (the
** quads drawn are chosen each frame). On the edge of a level, a vertex is put on the line between 2 vertices of the next
** level (height and color): no crack, no step of light.
**
** The waves: a wave is drawn on the levels with at least min_cells_per_wave quads per wavelength (fewer: the vertices
** miss the crests); it fades out toward the edge of the last of them (the same function of the position in all the
** levels: the vertices at the same place have the same height). Heights: a table per wave (amplitude x sine, 256 steps),
** the phase goes up by the same step along a row of vertices: a lookup and an addition per vertex and per wave.
**
** The 2 CPUs: the rows of vertices of all the levels one after the other; the slave CPU computes the rows before a split,
** the master CPU the others (the heights, then the light). The split moves by a row each frame toward the balance.
**
** The gouraud tables: made in RAM, copied to the VRAM by the DMA (2 sets per level: the VDP1 draws the previous frame
** with the other one; 2 arrays of attributes, one per set: nothing to change per quad).
*/

/** @brief The levels with the foam texture (quads of 8 and 16 units: tiles of 16 x 16 texels, the pattern every 64 units) */
# define FOAM_TEXTURE_LEVELS    (2)
# define FOAM_TILE          (16)
/** @brief Tiles on a side of the pattern of the level 0 (the level 1: half) */
# define FOAM_TILES         (8)
/** @brief Color lookup tables of the foam texture: more and more foam (a quad with less foam than the first one: no
 *  texture) */
# define FOAM_LUTS          (8)
/** @brief Foam of the vertices of a quad (the most) for each table: below FOAM_LUT_MIN no texture */
# define FOAM_LUT_MIN       (24)
/** @brief The bottom seen in the shallow water (see config.bottom): the levels with its texture (the first one, quads of 8
 *  units: tiles of 16 x 16 texels, the pattern every 64 units; farther, the light of the vertices only: the textured quads of
 *  the level 1 cost 1 to 2 ms of the master CPU near the shore), tiles on a side of the pattern of the level 0, its color
 *  lookup tables (from the shore to bottom_depth) */
# define BOTTOM_LEVELS      (1)
# define BOTTOM_TILES       (8)
# define BOTTOM_LUTS        (8)
/** @brief Places of the band of light of the bottom (the 15 values of its pattern, see prepare_bottom()) */
# define BOTTOM_PHASES      (15)
/** @brief The texture of the bottom up to this distance in front of the camera (units): farther, a quad of the level 1 is a
 *  thin line on the screen (the engine draws it by its slow path, its texture is not seen) */
# define BOTTOM_FAR         (160)
/** @brief The foam grid: FOAM_GRID x FOAM_GRID cells of 1 << FOAM_CELL_SHIFT units around the camera (a cell of a point:
 *  its position modulo the grid; the region of 1024 units of the point in the cell checks it) */
# define FOAM_GRID_SHIFT    (7)
# define FOAM_GRID          (1 << FOAM_GRID_SHIFT)
# define FOAM_CELL_SHIFT    (3)
# define FOAM_REGION_SHIFT  (FOAM_GRID_SHIFT + FOAM_CELL_SHIFT)
/** @brief Cells of the depth map per side of a block (see jo_water_set_depth_map()) */
# define DEPTH_BLOCK        (4)
/** @brief Levels where the vertices take the foam of the grid (quads of 8 and 16 units), the reflections of the clouds */
# define FOAM_LEVELS        (2)
# define REFLECTION_LEVELS  (2)
/** @brief Vertices computed inside the hole of a level (heights; the light: 1 less): the colors of the edge of the level
 *  inside it go to the colors of this one, they use them */
# define HOLE_MARGIN        (3)
/** @brief Rows of the edge of a level where its colors go to the colors of the next level */
# define BLEND_ROWS         (2)
# define MAX_SPLASHES       (8)
# define MAX_FRONTS         (8)
/** @brief Values of the shape of a front (see front_at(): a table instead of 2 sines per vertex) */
# define FRONT_SHAPE        (256)
/** @brief A splash lower than this is gone (jo_fixed units) */
# define SPLASH_MIN_AMPLITUDE   (JO_FIXED_1 / 4)
/** @brief Brightest gouraud (16 + 10: the whites of the texture are at 21 at most) */
# define GOURAUD_MAX        (26)
/** @brief Above it the light grows half as fast (no flat area where the light is cut at GOURAUD_MAX) */
# define GOURAUD_KNEE       (21)
/** @brief White of the spot, the glints and the reflections at most (steps) */
# define WHITE_MAX          (20)
/** @brief White of the foam at most (steps, with the rest) */
# define FOAM_WHITE_MAX     (26)
/** @brief Fresnel: N.V from NEAR_VY (deep) to NEAR_VY + 1 / NEAR_SCALE (shallow) */
# define NEAR_VY            (JO_FIXED_1 / 50)
# define NEAR_SCALE         (3)
/** @brief Light of the slopes: (N.S - S.y) x SLOPE_LIGHT steps */
# define SLOPE_LIGHT        (28)
/** @brief The reflections of the clouds: the direction reflected by the surface (R) on a plane of clouds above, R.x / R.y
 *  and R.z / R.y x 2048 / REFLECTION_SPREAD cells of the map of the clouds (__jo_water_sky_reflection, see tools/make_water_patterns.py); the
 *  position of the vertex / 2^REFLECTION_DRIFT cells (they pass under the camera) */
# define REFLECTION_SPREAD  (256)
# define REFLECTION_DRIFT   (5)
/** @brief Distance to the samples of jo_water_sample() (units, on each side) */
# define SAMPLE_STEP        (4)

/** @brief A cell of the foam grid (see jo_water_foam()): the frame of the stamp, its life (frames / 4), the region of 1024
 *  units of the cell (4 bits of x and z) */
typedef struct
{
    unsigned short          time;
    unsigned char           life;
    unsigned char           region;
}                           foam_cell;

/** @brief A wave of the swell, ready to compute */
typedef struct
{
    int                     kx;             // Phase per unit along x and z (jo_angle: 0x10000 = a wavelength)
    int                     kz;
    int                     w;              // Phase per frame
    jo_fixed                amplitude;      // Height x the swell
    jo_fixed                slope_x;        // Its slope between 2 points SAMPLE_STEP around, x the cosine of its phase: the
    jo_fixed                slope_z;        // amplitude x sin(kx x SAMPLE_STEP) / SAMPLE_STEP (see jo_water_sample())
    int                     last_level;     // The last level that draws it
    jo_fixed                table[256];     // Amplitude x sine
}                           wave_data;

/** @brief A level of the grid */
typedef struct
{
    int                     cells;
    int                     cell;
    int                     hole_i;         // First quad of the hole (the level inside it), -1: no hole (the first level)
    int                     hole_j;
    int                     hole_cells;     // Side of the hole (quads)
    int                     ratio;          // Side of a quad of the next level / side of a quad of this one
    jo_fixed                inv_ratio;
    int                     period;         // Updated every period frames (see update_cell)
    bool                    update;         // Updated in this frame
    int                     set;            // The set of gouraud tables of the last update
    int                     origin_x;
    int                     origin_z;
    bool                    along_z;        // The order of the vertices of the quads (see set_quad_order())
    jo_3d_mesh              *mesh;
    jo_pos3D_fixed          *points;
    jo_fixed                *heights;
    jo_color                *colors;
    unsigned char           *foam;          // Foam of the vertices (0 to 255, see FOAM_LEVELS)
    unsigned char           *depth;         // Depth of the vertices (0 the shore to 255, see jo_water_set_depth_map())
    unsigned char           *quad_i;        // Column and row of each quad (no division per quad)
    unsigned char           *quad_j;
    unsigned short          gouraud[2];
    jo_3d_polygon_attribute *attributes[2]; // The attributes of the quads with the gouraud tables of each set
    unsigned int            *tables;        // The gouraud tables made in RAM
    // The rows of the edge where the colors go to the colors of the next level (see prepare_blend())
    unsigned short          *blend_vertex;  // Vertex of this level
    unsigned short          *blend_parent;  // Vertex of the next level before it
    unsigned char           *blend_code;    // Row from the edge (bits 0-1), fraction along x (bits 2-4) and z (bits 5-7)
    int                     blend_count;
    int                     blend_hole_i;   // The hole of the next level when it was prepared
    int                     blend_hole_j;
    jo_fixed                caustics_x[JO_WATER_MAX_CELLS + 1];
    jo_fixed                caustics_z[JO_WATER_MAX_CELLS + 1];
    jo_fixed                caustics_xz[2 * JO_WATER_MAX_CELLS + 1];
    // The vertices of each row in the field of view (with a margin): from span_first to span_end (excluded), the others
    // are not computed (behind the camera, on the sides)
    unsigned char           span_first[JO_WATER_MAX_CELLS + 1];
    unsigned char           span_end[JO_WATER_MAX_CELLS + 1];
}                           level_data;

typedef struct
{
    int                     x;
    int                     z;
    int                     start;
    jo_fixed                height;
    jo_fixed                amplitude;      // Height at the time of the frame (lower with the time)
}                           splash_data;

typedef struct
{
    int                     x;
    int                     z;
    jo_fixed                dx;             // Direction (unit vector)
    jo_fixed                dz;
    int                     half_width;
    int                     start;
    jo_fixed                height;
    jo_fixed                amplitude;      // Height at the time of the frame (lower at the end of its life)
    jo_fixed                inv_side;       // 1 / (half_width / 2): the side where it gets lower
}                           front_data;

/** @brief The 2 CPUs: the slave CPU computes the rows before rows (share of the rows of the levels updated in the frame:
 *  the levels updated every 2 or 4 frames change the work of each frame) */
typedef struct
{
    jo_fixed                share;
    int                     rows;
    int                     total;
}                           cpu_split;

struct                      s_jo_water
{
    jo_water_config         config;
    jo_water_level          level_sizes[JO_WATER_MAX_LEVELS];
    jo_water_wave           wave_list[JO_WATER_MAX_WAVES];
    int                     level_count;
    level_data              levels[JO_WATER_MAX_LEVELS];
    int                     wave_count;
    wave_data               waves[JO_WATER_MAX_WAVES];
    jo_fixed                swell;
    // The waves of the events: the last level that draws them; phase per unit of their wave and of their envelope
    // (no division per vertex)
    int                     splash_k;
    int                     splash_envelope_k;
    int                     front_k;
    int                     front_envelope_k;
    // The shape of a front over its 2 wavelengths (FRONT_SHAPE values: the envelope x the wave, jo_fixed), the index of a
    // distance behind its front (x front_index >> 16)
    jo_fixed                front_shape[FRONT_SHAPE];
    int                     front_index;
    int                     splash_level;
    int                     front_level;
    splash_data             splashes[MAX_SPLASHES];
    int                     next_splash;
    int                     live_splashes[MAX_SPLASHES];
    int                     live_splash_count;
    front_data              fronts[MAX_FRONTS];
    int                     next_front;
    front_data              *live_fronts[MAX_FRONTS];
    int                     live_front_count;
    // The frame
    int                     frame;
    int                     updates;
    int                     cam_x;
    int                     cam_y;          // (y up)
    int                     view_tan;       // Tangent of half the field of view x 16 (+ 10 %)
    int                     cam_z;
    jo_fixed                forward_x;
    jo_fixed                forward_z;
    // The bounds of the levels (fade of the waves)
    int                     bound_x[JO_WATER_MAX_LEVELS];
    int                     bound_z[JO_WATER_MAX_LEVELS];
    int                     bound_side[JO_WATER_MAX_LEVELS];
    // The sun
    jo_fixed                sun_x;          // Toward the sun (unit vector, y up)
    jo_fixed                sun_y;
    jo_fixed                sun_z;
    jo_fixed                sun_hx;         // Toward the sun on the sea (unit vector)
    jo_fixed                sun_hz;
    int                     spot_distance;
    jo_fixed                spot_inv_radius;
    // (1 << 24) / the shallow depth rounded up, 0 to divide (see vertex_color(): the same as the division up to 255)
    unsigned int            shallow_scale;
    int                     spot_x;
    int                     spot_z;
    // The foam texture: the tiles of the pattern (the quads A-D along x, then along z: turned), the color lookup tables
    // (VRAM address / 8), the attributes of a quad without and with the texture (see set_polygons())
    int                     foam_tiles[FOAM_TEXTURE_LEVELS][2][FOAM_TILES * FOAM_TILES];
    unsigned short          foam_luts[FOAM_LUTS];
    jo_3d_polygon_attribute plain;
    jo_3d_polygon_attribute foamy;
    // The bottom seen in the shallow water: the tiles of its pattern (as the foam), its color lookup tables, the attributes
    // of a quad with its texture
    int                     bottom_tiles[BOTTOM_LEVELS][2][BOTTOM_TILES * BOTTOM_TILES];
    unsigned short          bottom_luts[BOTTOM_LUTS];
    unsigned short          bottom_cache[BOTTOM_PHASES][BOTTOM_LUTS][16] __attribute__((aligned(4)));
    int                     bottom_phase;   // The place of the band of light in the VRAM (-1: none yet)
    jo_3d_polygon_attribute bottomy;
    // The depth map (see jo_water_set_depth_map()); its blocks of DEPTH_BLOCK x DEPTH_BLOCK cells with shallow water (a row
    // of vertices out of them: deep, no lookup)
    unsigned char           depth_blocks[256];
    int                     depth_block_count;
    const unsigned char     *depth_map;
    int                     depth_size;
    int                     depth_shift;
    int                     depth_x;
    int                     depth_z;
    foam_cell               *foam_grid;     // FOAM_GRID x FOAM_GRID cells
    bool                    foam_live;      // Foam stamped in the grid (in the last minutes)
    bool                    foam_filled;    // All the tables of the foam made once (see animate_foam())
    int                     foam_frame;     // The frame of the last stamp
    int                     grain;
    // The skirt
    jo_3d_mesh              *skirt;
    int                     skirt_segments;
    jo_pos3D_fixed          *skirt_points;  // Shared by its quads (2 rings)
    unsigned short          skirt_gouraud[2];
    int                     skirt_set;
    // The 2 CPUs
    cpu_split               height_split;
    cpu_split               light_split;
    int                     slave_draw_levels;  // The levels drawn by the slave CPU
};

/** @brief The water of the jobs of the slave CPU (they take no argument) */
static jo_water             *current;

void                        (*__jo_water_objects_frame)(const jo_water * const w, const __jo_water_objects_step step) = JO_NULL;

/*
** Tables
*/

/** @brief 1 / length of a vector without square root nor division: length ~ max + 11/32 mid + 1/4 min (4 % at most) */
# define INVERSE_STEP_SHIFT (2)
# define INVERSE_COUNT      (2048)
static jo_fixed             inverses[INVERSE_COUNT];
/** @brief 1 / sqrt(1 + s), s = nx^2 + nz^2 from 0 to 4 in 256 steps (the length of the normal (-nx, 1, -nz)) */
static jo_fixed             normal_inverses[256];
/** @brief Sine (256 steps per period) */
static jo_fixed             sines[256];
/** @brief 2048 / k: R.x / R.y in cells of the reflections (R.y in 1/64) */
static unsigned short       reflection_inverses[65];
/** @brief 65536 / (life x 4) (the fade of a cell) */
static unsigned short       foam_inverses[256];
static bool                 tables_ready;

static void                 init_tables(void)
{
    int                     k;

    if (tables_ready)
        return;
    tables_ready = true;
    inverses[0] = JO_FIXED_1;
    for (k = 1; k < INVERSE_COUNT; ++k)
        inverses[k] = JO_FIXED_1 / (k << INVERSE_STEP_SHIFT);
    // (1 / sqrt(1 + k / 64): once)
    for (JO_ZERO(k); k < 256; ++k)
    {
        normal_inverses[k] = jo_fixed_div(JO_FIXED_1, (jo_fixed)(jo_sqrt((unsigned int)(64 + k)) * 65536 / 8));
        sines[k] = jo_sin_angle((jo_angle)(k << 8));
        foam_inverses[k] = (unsigned short)(k == 0 ? 0 : JO_MIN(65535, 16384 / k));
    }
    for (JO_ZERO(k); k <= 64; ++k)
        reflection_inverses[k] = (unsigned short)(2048 * 64 / REFLECTION_SPREAD / JO_MAX(2, k));
}

static  __jo_force_inline jo_fixed  inverse_length(int a, int b, int c)
{
    int                     t;

    a = JO_ABS(a);
    b = JO_ABS(b);
    c = JO_ABS(c);
    if (a < b)
    {
        t = a;
        a = b;
        b = t;
    }
    if (b < c)
    {
        t = b;
        b = c;
        c = t;
        if (a < b)
        {
            t = a;
            a = b;
            b = t;
        }
    }
    // (positive values: logical shifts, the SH-2 has no arithmetic shift of several bits)
    t = (int)(((unsigned int)a + ((unsigned int)(b * 11) >> 5) + ((unsigned int)c >> 2)) >> INVERSE_STEP_SHIFT);
    return (inverses[JO_MIN(t, INVERSE_COUNT - 1)]);
}

/** @brief Sine of a jo_angle (256 steps per period) */
static  __jo_force_inline jo_fixed  fast_sin(const int angle)
{
    return (sines[((unsigned int)angle >> 8) & 255]);
}

/** @brief A value rounded down to a multiple of step (negative values too) */
static int                  snap(const int value, const int step)
{
    return (value >= 0 ? value / step * step : -((step - 1 - value) / step * step));
}

/*
** Configuration
*/

static const jo_water_level default_levels[] =
{
    {8, 128}, {16, 256}, {32, 384}, {64, 768}
};


void                        jo_water_default_config(jo_water_config * const config)
{
    jo_memset(config, 0, sizeof(*config));
    config->levels = default_levels;
    config->level_count = (int)(sizeof(default_levels) / sizeof(*default_levels));
    config->ahead = 84;
    config->update_cell = 16;
    config->min_cells_per_wave = 4;
    config->fade = 160;
    config->skirt = 16000;
    config->slave_levels = 1;
    // (the swell: JO_WATER_SWELL_CHOPPY, made by jo_water_create(), see jo_water_swell())
    config->waves = JO_NULL;
    JO_ZERO(config->wave_count);
    jo_water_config_look(config, JO_WATER_LOOK_DEFAULT);
    config->spot = 4;
    config->spot_distance = 64;
    config->caustics = 2;
    config->reflection = 0;
    config->foam_white = 6;
    config->foam_color = JO_COLOR_RGB(168, 176, 176);
    config->shore.r = 4;
    config->shore.g = 4;
    config->shore.b = 0;
    config->shallow_depth = 160;
    config->surf = 0;
    config->textured = false;
    config->grain = false;
    config->splash_length = 96;
    config->splash_speed = 3;
    config->splash_life = 360;
    config->front_length = 192;
    config->front_speed = 3;
    config->front_life = 480;
}

const jo_water_config       *jo_water_get_config(const jo_water * const w)
{
    return (&w->config);
}

jo_color                    jo_water_seen_color(const jo_water * const w, const jo_color object, const int amount)
{
    const jo_water_config   *c;
    int                     seen[3];
    int                     k;
    int                     o;
    jo_color                color;

    c = &w->config;
    seen[0] = c->shallow.r;
    seen[1] = c->shallow.g;
    seen[2] = c->shallow.b;
    color = 0x8000;
    for (JO_ZERO(k); k < 3; ++k)
    {
        seen[k] = JO_MAX(5, JO_MIN(21, ((c->color >> (k * 5)) & 31) + seen[k] - 16));
        o = (object >> (k * 5)) & 31;
        color |= (jo_color)((o + (seen[k] - o) * amount / 256) << (k * 5));
    }
    return (color);
}

/** @brief The last level that draws a wave of this length */
static int                  last_level_for(const jo_water * const w, const int length)
{
    int                     k;
    int                     last;

    JO_ZERO(last);
    for (JO_ZERO(k); k < w->level_count; ++k)
        if (w->levels[k].cell * w->config.min_cells_per_wave <= length)
            last = k;
    return (last);
}

/** @brief The tables of the waves (amplitude x swell x sine) */
static void                 build_waves(jo_water * const w)
{
    wave_data               *d;
    const jo_water_wave     *s;
    int                     i;
    int                     k;
    jo_angle                direction;

    for (JO_ZERO(i); i < w->wave_count; ++i)
    {
        s = &w->wave_list[i];
        d = &w->waves[i];
        direction = (jo_angle)(s->direction * 65536 / 360);
        // (phase per unit: 65536 / length along the direction, sin x 65536 / length)
        d->kx = jo_sin_angle(direction) / JO_MAX(1, s->length);
        d->kz = jo_cos_angle(direction) / JO_MAX(1, s->length);
        d->w = 65536 / JO_MAX(1, s->period);
        d->amplitude = jo_fixed_mult(s->height, w->swell);
        d->slope_x = jo_fixed_mult(d->amplitude, jo_sin_angle((jo_angle)(d->kx * SAMPLE_STEP))) / SAMPLE_STEP;
        d->slope_z = jo_fixed_mult(d->amplitude, jo_sin_angle((jo_angle)(d->kz * SAMPLE_STEP))) / SAMPLE_STEP;
        d->last_level = last_level_for(w, s->length);
        for (JO_ZERO(k); k < 256; ++k)
            d->table[k] = jo_fixed_mult(d->amplitude, sines[k]);
    }
}

void                        jo_water_set_waves(jo_water * const w, const jo_water_wave * const waves, const int count)
{
    // (a negative count: no wave, it copied a huge size)
    w->wave_count = JO_MAX(0, JO_MIN(count, JO_WATER_MAX_WAVES));
    jo_memcpy(w->wave_list, waves, (unsigned int)w->wave_count * sizeof(jo_water_wave));
    w->config.waves = w->wave_list;
    w->config.wave_count = w->wave_count;
    build_waves(w);
}

void                        jo_water_set_swell(jo_water * const w, const jo_fixed scale)
{
    w->swell = scale;
    build_waves(w);
}

void                        jo_water_set_sun(jo_water * const w, const jo_angle azimuth, const jo_angle elevation)
{
    jo_fixed                c;

    c = jo_cos_angle(elevation);
    w->sun_x = jo_fixed_mult(c, jo_sin_angle(azimuth));
    w->sun_z = jo_fixed_mult(c, jo_cos_angle(azimuth));
    w->sun_y = jo_sin_angle(elevation);
    w->sun_hx = jo_sin_angle(azimuth);
    w->sun_hz = jo_cos_angle(azimuth);
    // The spot: farther when the sun is low (as its reflection on the sea), bigger when farther
    w->spot_distance = JO_MIN(1400, jo_fixed2int(jo_fixed_div(c, JO_MAX(w->sun_y, 4096)) * w->config.spot_distance));
    w->spot_inv_radius = JO_FIXED_1 / (192 + w->spot_distance / 2);
}

/*
** Events: splashes (a ring of waves) and wave fronts (a straight wave)
*/

void                        jo_water_drop(jo_water * const w, const int x, const int z, const int height)
{
    splash_data             *s;

    s = &w->splashes[w->next_splash];
    w->next_splash = (w->next_splash + 1) % MAX_SPLASHES;
    s->x = x;
    s->z = z;
    s->start = w->frame;
    s->height = jo_int2fixed(height);
}

/*
** Floating objects (see jo_water_float()): gravity in the air; in the water a spring toward the surface under the object
** (FLOAT_SPRING of the distance a frame), damped (FLOAT_DAMPING of the speed); the tilt: the slopes of the surface (a small
** slope in radians is about its tangent, x RADIAN_TO_ANGLE)
*/
# define FLOAT_GRAVITY      (JO_FIXED_1 / 5)
# define FLOAT_SPRING       (JO_FIXED_1 / 12)
# define FLOAT_DAMPING      (JO_FIXED_1 / 7)
# define RADIAN_TO_ANGLE    (10430)

void                        jo_water_floating_init(jo_water_floating * const f, const int x, const int z, const int height, const int half,
                                                   const int sink)
{
    f->x = x;
    f->z = z;
    f->y = jo_int2fixed(height);
    JO_ZERO(f->vy);
    f->half = half;
    f->sink = sink;
    JO_ZERO(f->tilt_x);
    JO_ZERO(f->tilt_z);
    f->in_water = false;
}

void                        jo_water_float(jo_water * const w, jo_water_floating * const f)
{
    jo_water_point          point;
    jo_fixed                speed;

    jo_water_sample(w, f->x, f->z, &point);
    if (!f->in_water)
    {
        f->vy -= FLOAT_GRAVITY;
        f->y += f->vy;
        if (f->y - jo_int2fixed(f->half) <= point.height)
        {
            // (the splash: waves as high as the speed of the fall, the object goes under a bit)
            f->in_water = true;
            speed = -f->vy;
            jo_water_drop(w, f->x, f->z, JO_MIN(18, jo_fixed2int(speed) * 3 + 4));
            f->vy = -speed / 4;
        }
        return;
    }
    f->vy += jo_fixed_mult(point.height + jo_int2fixed(f->half - f->sink) - f->y, FLOAT_SPRING) - jo_fixed_mult(f->vy, FLOAT_DAMPING);
    f->y += f->vy;
    f->tilt_x = (jo_angle)jo_fixed_mult(point.slope_z, RADIAN_TO_ANGLE);
    f->tilt_z = (jo_angle)-jo_fixed_mult(point.slope_x, RADIAN_TO_ANGLE);
}

void                        jo_water_wave_front(jo_water * const w, const int x, const int z, const jo_angle direction, const int height,
                                             const int width)
{
    front_data              *f;

    f = &w->fronts[w->next_front];
    w->next_front = (w->next_front + 1) % MAX_FRONTS;
    f->x = x;
    f->z = z;
    f->dx = jo_sin_angle(direction);
    f->dz = jo_cos_angle(direction);
    f->half_width = width / 2;
    f->inv_side = JO_FIXED_1 / JO_MAX(1, width / 4);
    f->start = w->frame;
    f->height = jo_int2fixed(height);
}

/** @brief The splashes and the fronts alive at the time of the frame */
static void                 find_live_events(jo_water * const w)
{
    const jo_water_config   *c;
    front_data              *f;
    int                     k;
    int                     age;

    c = &w->config;
    JO_ZERO(w->live_splash_count);
    for (JO_ZERO(k); k < MAX_SPLASHES; ++k)
    {
        age = w->frame - w->splashes[k].start;
        if (w->splashes[k].height == 0 || age <= 0 || age >= c->splash_life)
            continue;
        // Lower with the time (spread on a longer circle, gone at the end of its life)
        w->splashes[k].amplitude = w->splashes[k].height / c->splash_life * (c->splash_life - age) /
                                   (c->splash_length + age * c->splash_speed / 2) * c->splash_length;
        // (a ring too low to be seen: not computed any more, its circle covers most of the grid)
        if (w->splashes[k].amplitude < SPLASH_MIN_AMPLITUDE)
            continue;
        w->live_splashes[w->live_splash_count++] = k;
    }
    JO_ZERO(w->live_front_count);
    for (JO_ZERO(k); k < MAX_FRONTS; ++k)
    {
        f = &w->fronts[k];
        age = w->frame - f->start;
        if (f->height == 0 || age <= 0 || age >= c->front_life)
            continue;
        // It grows, then fades out: a half sine over its life (a swell that rises out at sea and dies on the shore)
        f->amplitude = jo_fixed_mult(f->height, jo_sin_angle((jo_angle)(age * 32768 / c->front_life)));
        w->live_fronts[w->live_front_count++] = f;
    }
}

/** @brief Height of the splashes at a point (jo_fixed) */
static jo_fixed             splash_height(const jo_water * const w, const int x, const int z)
{
    const jo_water_config   *c;
    const splash_data       *s;
    int                     front;
    int                     back;
    int                     age;
    int                     dx;
    int                     dz;
    int                     r2;
    int                     behind;
    int                     k;
    int                     length;
    jo_fixed                h;
    jo_fixed                a;

    c = &w->config;
    length = c->splash_length;
    JO_ZERO(h);
    for (JO_ZERO(k); k < w->live_splash_count; ++k)
    {
        s = &w->splashes[w->live_splashes[k]];
        age = w->frame - s->start;
        front = age * c->splash_speed;
        back = JO_MAX(0, front - length * 2);
        dx = x - s->x;
        dz = z - s->z;
        if (JO_ABS(dx) > front || JO_ABS(dz) > front)
            continue;
        r2 = dx * dx + dz * dz;
        if (r2 > front * front || r2 < back * back)
            continue;
        behind = front - (int)jo_sqrt((unsigned int)r2);
        // 2 rings, smooth at the front and at the back (a half sine over them)
        a = jo_fixed_mult(s->amplitude, jo_sin_angle((jo_angle)(behind * w->splash_envelope_k)));
        h += jo_fixed_mult(a, jo_sin_angle((jo_angle)(behind * w->splash_k)));
    }
    return (h);
}

/** @brief Add the splashes to a row of vertices: only the vertices between the circles of each splash (a ring far from
 *  its center covers few vertices of a row, its circle most of the grid)
 *  @param end First vertex of the hole of the row (vertices: no hole)
 *  @param start First vertex after the hole
 */
static void                 add_splashes_row(const jo_water * const w, jo_fixed * const row, const int count, const int x,
                                             const int z, const int cell, const int end, const int start)
{
    const jo_water_config   *c;
    const splash_data       *s;
    int                     k;
    int                     i;
    int                     last;
    int                     age;
    int                     front;
    int                     back;
    int                     dz;
    int                     dx;
    int                     r2;
    int                     outer;
    int                     inner_first;
    int                     inner_last;
    int                     inverse;
    int                     behind;
    jo_fixed                a;

    c = &w->config;
    // (vertex of a position: x * inverse >> 16, no division per splash)
    inverse = JO_FIXED_1 / cell;
    for (JO_ZERO(k); k < w->live_splash_count; ++k)
    {
        s = &w->splashes[w->live_splashes[k]];
        age = w->frame - s->start;
        front = age * c->splash_speed;
        back = JO_MAX(0, front - c->splash_length * 2);
        dz = z - s->z;
        if (JO_ABS(dz) > front)
            continue;
        // The vertices inside the outer circle, minus the ones inside the inner circle
        outer = (int)jo_sqrt((unsigned int)(front * front - dz * dz));
        dx = s->x - outer - x;
        i = dx <= 0 ? 0 : (dx * inverse + JO_FIXED_1 - 1) >> 16;
        dx = s->x + outer - x;
        if (dx < 0)
            continue;
        last = JO_MIN(count - 1, (dx * inverse) >> 16);
        inner_first = count;
        inner_last = count;
        if (JO_ABS(dz) < back)
        {
            outer = (int)jo_sqrt((unsigned int)(back * back - dz * dz));
            dx = s->x - outer - x;
            inner_first = dx <= 0 ? 0 : (dx * inverse + JO_FIXED_1 - 1) >> 16;
            dx = s->x + outer - x;
            inner_last = dx < 0 ? -1 : (dx * inverse) >> 16;
        }
        for (; i <= last; ++i)
        {
            if (i >= inner_first && i <= inner_last)
            {
                i = inner_last;
                continue;
            }
            if (i >= end && i < start)
            {
                i = start - 1;
                continue;
            }
            dx = x + i * cell - s->x;
            r2 = dx * dx + dz * dz;
            if (r2 > front * front || r2 < back * back)
                continue;
            behind = front - (int)jo_sqrt((unsigned int)r2);
            // 2 rings, smooth at the front and at the back (a half sine over them)
            a = jo_fixed_mult(s->amplitude, jo_sin_angle((jo_angle)(behind * w->splash_envelope_k)));
            row[i] += jo_fixed_mult(a, jo_sin_angle((jo_angle)(behind * w->splash_k)));
        }
    }
}

/** @brief Height of a wave front at a point: distance behind its front, across it (jo_fixed) */
static  __jo_force_inline jo_fixed  front_at(const jo_water * const w, const front_data * const f, const int behind, const int across)
{
    jo_fixed                a;
    int                     length;

    length = w->config.front_length;
    if (behind <= 0 || behind >= length * 2 || across >= f->half_width)
        return (0);
    // Lower on its sides; 2 crests, smooth at the front and at the back (a half sine over them: front_shape)
    a = f->amplitude;
    if (across > f->half_width / 2)
        a = jo_fixed_mult(a, (f->half_width - across) * f->inv_side);
    (void)length;
    return (jo_fixed_mult(a, w->front_shape[(behind * w->front_index) >> 16]));
}

static jo_fixed             front_height(const jo_water * const w, const int x, const int z)
{
    const front_data        *f;
    int                     k;
    jo_fixed                h;

    JO_ZERO(h);
    for (JO_ZERO(k); k < w->live_front_count; ++k)
    {
        f = w->live_fronts[k];
        h += front_at(w, f, (w->frame - f->start) * w->config.front_speed - jo_fixed2int((x - f->x) * f->dx + (z - f->z) * f->dz),
                      JO_ABS(jo_fixed2int((x - f->x) * f->dz - (z - f->z) * f->dx)));
    }
    return (h);
}

/** @brief Add the wave fronts to a row of vertices (the distances along and across a front go up by the same step: no
 *  product per vertex) */
static void                 add_fronts_row(const jo_water * const w, jo_fixed * const row, const int first, const int count,
                                           const int x0, const int z, const int cell)
{
    const front_data        *f;
    int                     k;
    int                     i;
    int                     run;
    jo_fixed                along;
    jo_fixed                side;
    jo_fixed                along_step;
    jo_fixed                side_step;
    int                     a0;
    int                     a1;
    int                     s0;
    int                     s1;
    int                     length;
    int                     x;

    // (the vertices first to count - 1 only: in the field of view)
    if (first >= count)
        return;
    x = x0 + first * cell;
    length = w->config.front_length;
    for (JO_ZERO(k); k < w->live_front_count; ++k)
    {
        f = w->live_fronts[k];
        run = (w->frame - f->start) * w->config.front_speed;
        along = (x - f->x) * f->dx + (z - f->z) * f->dz;
        side = (x - f->x) * f->dz - (z - f->z) * f->dx;
        along_step = cell * f->dx;
        side_step = cell * f->dz;
        // (the row out of the wave: along and side change linearly, their values at both ends tell)
        a0 = run - jo_fixed2int(along);
        a1 = run - jo_fixed2int(along + along_step * (count - 1 - first));
        if ((a0 <= 0 && a1 <= 0) || (a0 >= 2 * length && a1 >= 2 * length))
            continue;
        s0 = jo_fixed2int(side);
        s1 = jo_fixed2int(side + side_step * (count - 1 - first));
        if ((s0 >= f->half_width && s1 >= f->half_width) || (s0 <= -f->half_width && s1 <= -f->half_width))
            continue;
        for (i = first; i < count; ++i, along += along_step, side += side_step)
            row[i] += front_at(w, f, run - jo_fixed2int(along), JO_ABS(jo_fixed2int(side)));
    }
}

/*
** Foam: a grid of cells of 8 units around the camera (modulo the grid: no copy when the camera moves), each one the time
** and the life of its last stamp (no update per frame: the foam of a cell is computed from its age)
*/

/** @brief Region of 1024 units of a point (4 bits of x and z) */
static  __jo_force_inline int  foam_region(const int x, const int z)
{
    return (((x >> FOAM_REGION_SHIFT) & 15) | (((z >> FOAM_REGION_SHIFT) & 15) << 4));
}

static  __jo_force_inline foam_cell  *foam_cell_at(foam_cell * const grid, const int x, const int z)
{
    return (&grid[(((z >> FOAM_CELL_SHIFT) & (FOAM_GRID - 1)) << FOAM_GRID_SHIFT) | ((x >> FOAM_CELL_SHIFT) & (FOAM_GRID - 1))]);
}

/** @brief Foam of a cell at a time (0 to 255: 255 at the stamp, 0 at the end of its life) */
static  __jo_force_inline int  foam_of(const foam_cell * const c, const int frame, const int region)
{
    int                     age;

    if (c->life == 0 || c->region != region)
        return (0);
    age = (unsigned short)(frame - c->time);
    if (age >= c->life * 4)
        return (0);
    return (255 - ((age * foam_inverses[c->life]) >> 8));
}

void                        jo_water_set_depth_map(jo_water * const w, const unsigned char * const map, const int size, const int shift,
                                                const int x, const int z)
{
    int                     i;
    int                     j;
    int                     n;

    w->depth_map = map;
    w->depth_size = size;
    w->depth_shift = shift;
    w->depth_x = x;
    w->depth_z = z;
    // The blocks with shallow water (or next to it: the interpolation reads the next cells)
    w->depth_block_count = JO_MIN(16, size / DEPTH_BLOCK);
    jo_memset(w->depth_blocks, 0, sizeof(w->depth_blocks));
    if (map == JO_NULL)
        return;
    for (JO_ZERO(j); j < size; ++j)
        for (JO_ZERO(i); i < size; ++i)
            if (map[j * size + i] < 255)
                for (n = 0; n < 9; ++n)
                {
                    // (the block of the cell and the blocks of its neighbors)
                    if ((i + n % 3 - 1) / DEPTH_BLOCK < w->depth_block_count && (j + n / 3 - 1) / DEPTH_BLOCK < w->depth_block_count &&
                        i + n % 3 - 1 >= 0 && j + n / 3 - 1 >= 0)
                        w->depth_blocks[((j + n / 3 - 1) / DEPTH_BLOCK) * 16 + (i + n % 3 - 1) / DEPTH_BLOCK] = 1;
                }
}

/** @brief A row of vertices (z, from x0 to x1) crosses shallow water (see depth_blocks) */
static bool                 shallow_row(const jo_water * const w, const int z, const int x0, const int x1)
{
    int                     bj;
    int                     b0;
    int                     b1;
    int                     block;

    block = DEPTH_BLOCK << w->depth_shift;
    bj = (z - w->depth_z) / block;
    if (z < w->depth_z || bj >= w->depth_block_count)
        return (false);
    b0 = JO_MAX(0, (x0 - w->depth_x) / block);
    b1 = JO_MIN(w->depth_block_count - 1, (x1 - w->depth_x) / block);
    for (; b0 <= b1; ++b0)
        if (w->depth_blocks[bj * 16 + b0])
            return (true);
    return (false);
}

/** @brief Depth of the water at a point (0 to 255, the map interpolated between its cells) */
static int                  depth_at(const jo_water * const w, const int x, const int z)
{
    const unsigned char     *m;
    int                     u;
    int                     v;
    int                     i;
    int                     j;
    int                     fu;
    int                     fv;
    int                     top;
    int                     bottom;
    int                     n;

    // (the cell and the fraction in it, 8 bits)
    u = ((x - w->depth_x) << 8) >> w->depth_shift;
    v = ((z - w->depth_z) << 8) >> w->depth_shift;
    i = u >> 8;
    j = v >> 8;
    n = w->depth_size;
    if (i < 0 || j < 0 || i >= n - 1 || j >= n - 1)
        return (255);
    fu = u & 255;
    fv = v & 255;
    m = w->depth_map + j * n + i;
    top = m[0] + (((m[1] - m[0]) * fu) >> 8);
    bottom = m[n] + (((m[n + 1] - m[n]) * fu) >> 8);
    return (top + (((bottom - top) * fv) >> 8));
}

int                         jo_water_foam_at(const jo_water * const w, const int x, const int z)
{
    if (!w->foam_live)
        return (0);
    return (foam_of(foam_cell_at(w->foam_grid, x, z), w->frame, foam_region(x, z)));
}

/** @brief Stamp the foam on a cell (the foam already there that lives longer: kept) */
static  __jo_force_inline void  stamp_foam(jo_water * const w, const int cx, const int cz, const int l)
{
    foam_cell               *c;
    int                     region;
    int                     left;
    int                     px;
    int                     pz;

    px = cx << FOAM_CELL_SHIFT;
    pz = cz << FOAM_CELL_SHIFT;
    c = foam_cell_at(w->foam_grid, px, pz);
    region = foam_region(px, pz);
    if (c->life != 0 && c->region == region)
    {
        left = c->life * 4 - (unsigned short)(w->frame - c->time);
        if (left > l * 4)
            return;
    }
    c->time = (unsigned short)w->frame;
    c->life = (unsigned char)l;
    c->region = (unsigned char)region;
}

void                        jo_water_foam(jo_water * const w, const int x, const int z, const int radius, const int life)
{
    unsigned int            inverse;
    int                     longest;
    int                     divisor;
    int                     r;
    int                     cx;
    int                     cz;
    int                     i;
    int                     j;
    int                     d2;
    int                     l;

    r = JO_MAX(radius, 1 << FOAM_CELL_SHIFT) >> FOAM_CELL_SHIFT;
    cx = x >> FOAM_CELL_SHIFT;
    cz = z >> FOAM_CELL_SHIFT;
    w->foam_live = true;
    w->foam_frame = w->frame;
    if (life <= 0)
        return;
    // (the edge lives half as long, a soft edge as it fades: life - life x d2 / (2 r^2 + 1) at the distance d2. The division
    // by a product with the inverse of 2 r^2 + 1, rounded up: exact while life x d2 x (2 r^2 + 1) < 2^32, up to 32 cells.
    // Longer than 2047 frames, the foam is 255 everywhere: the same as 2047)
    longest = JO_MIN(life, 2047);
    divisor = 2 * r * r + 1;
    inverse = r <= 32 ? 0xFFFFFFFFu / (unsigned int)divisor + 1 : 0;
    // (a quarter of the disc: the foam of a distance computed once for the 4 cells at this distance)
    for (JO_ZERO(j); j <= r; ++j)
        for (JO_ZERO(i); i <= r; ++i)
        {
            d2 = i * i + j * j;
            if (d2 > r * r)
                break;
            l = JO_MIN(255, (longest - (inverse != 0 ? (int)(((unsigned long long)(unsigned int)(longest * d2) * inverse) >> 32) :
                                                       longest * d2 / divisor)) >> 2);
            if (l <= 0)
                break;
            stamp_foam(w, cx + i, cz + j, l);
            if (i != 0)
                stamp_foam(w, cx - i, cz + j, l);
            if (j != 0)
            {
                stamp_foam(w, cx + i, cz - j, l);
                if (i != 0)
                    stamp_foam(w, cx - i, cz - j, l);
            }
        }
}

/*
** The surface for the objects
*/

jo_fixed                    jo_water_drawn_height(const jo_water * const w, const int x, const int z)
{
    const level_data        *g;
    const jo_fixed          *h;
    jo_fixed                fx;
    jo_fixed                fz;
    jo_fixed                top;
    jo_fixed                bottom;
    int                     vertices;
    int                     i;
    int                     j;
    int                     k;

    for (JO_ZERO(k); k < w->level_count; ++k)
    {
        g = &w->levels[k];
        i = x - g->origin_x;
        j = z - g->origin_z;
        if (i < 0 || j < 0 || i >= g->cells * g->cell || j >= g->cells * g->cell)
            continue;
        vertices = g->cells + 1;
        fx = jo_int2fixed(i % g->cell) / g->cell;
        fz = jo_int2fixed(j % g->cell) / g->cell;
        h = &g->heights[(j / g->cell) * vertices + i / g->cell];
        top = h[0] + jo_fixed_mult(h[1] - h[0], fx);
        bottom = h[vertices] + jo_fixed_mult(h[vertices + 1] - h[vertices], fx);
        return (top + jo_fixed_mult(bottom - top, fz));
    }
    return (jo_water_wave_height(w, x, z));
}

int                         jo_water_quad_side(const jo_water * const w, const int x, const int z)
{
    const level_data        *g;
    int                     k;

    for (JO_ZERO(k); k < w->level_count; ++k)
    {
        g = &w->levels[k];
        if (x >= g->origin_x && z >= g->origin_z && x < g->origin_x + g->cells * g->cell && z < g->origin_z + g->cells * g->cell)
            return (g->cell);
    }
    return (w->levels[w->level_count - 1].cell);
}

jo_fixed                    jo_water_wave_height(const jo_water * const w, const int x, const int z)
{
    const wave_data         *d;
    jo_fixed                h;
    int                     i;

    JO_ZERO(h);
    for (JO_ZERO(i); i < w->wave_count; ++i)
    {
        d = &w->waves[i];
        h += jo_fixed_mult(d->amplitude, jo_sin_angle((jo_angle)(x * d->kx + z * d->kz + w->frame * d->w)));
    }
    return (h);
}

jo_fixed                    jo_water_height(const jo_water * const w, const int x, const int z)
{
    return (jo_water_wave_height(w, x, z) + splash_height(w, x, z) + front_height(w, x, z));
}

void                        jo_water_sample(const jo_water * const w, const int x, const int z, jo_water_point * const point)
{
    const wave_data         *d;
    jo_fixed                height;
    jo_fixed                slope_x;
    jo_fixed                slope_z;
    jo_fixed                s;
    jo_fixed                c;
    int                     i;

    // The waves: the height and the slopes at once. The slope between 2 points SAMPLE_STEP around (sin(a + b) - sin(a - b)
    // = 2 cos(a) sin(b)): the cosine of the phase x a factor of the wave (see build_waves()), a sine and a cosine per wave
    // instead of 5 sines
    JO_ZERO(height);
    JO_ZERO(slope_x);
    JO_ZERO(slope_z);
    for (JO_ZERO(i); i < w->wave_count; ++i)
    {
        d = &w->waves[i];
        jo_sincos_angle((jo_angle)(x * d->kx + z * d->kz + w->frame * d->w), &s, &c);
        height += jo_fixed_mult(d->amplitude, s);
        slope_x += jo_fixed_mult(d->slope_x, c);
        slope_z += jo_fixed_mult(d->slope_z, c);
    }
    // The splashes and the wave fronts: the slopes from 4 heights around the point
    if (w->live_splash_count > 0 || w->live_front_count > 0)
    {
        height += splash_height(w, x, z) + front_height(w, x, z);
        slope_x += (splash_height(w, x + SAMPLE_STEP, z) + front_height(w, x + SAMPLE_STEP, z) - splash_height(w, x - SAMPLE_STEP, z) -
                    front_height(w, x - SAMPLE_STEP, z)) / (2 * SAMPLE_STEP);
        slope_z += (splash_height(w, x, z + SAMPLE_STEP) + front_height(w, x, z + SAMPLE_STEP) - splash_height(w, x, z - SAMPLE_STEP) -
                    front_height(w, x, z - SAMPLE_STEP)) / (2 * SAMPLE_STEP);
    }
    point->height = height;
    point->slope_x = slope_x;
    point->slope_z = slope_z;
}

/*
** The grid
*/

/*
** The VDP1 draws a quad line by line from its side A-D to its side B-C, the gouraud along the lines: its rounding makes
** streaks along the lines. The lines go away from the camera: the streaks are across the screen, as the waves.
*/

/** @brief Order of the vertices of the quads of a level
 *  @param along_z The lines of the VDP1 along z (A-D along x), else along x
 */
static void                 set_quad_order(level_data * const g, const bool along_z)
{
    int                     vertices;
    int                     q;
    int                     n;
    unsigned short          *v;

    if (g->along_z == along_z)
        return;
    g->along_z = along_z;
    vertices = g->cells + 1;
    for (JO_ZERO(q); q < (int)g->mesh->data.polygon_count; ++q)
    {
        n = g->quad_j[q] * vertices + g->quad_i[q];
        v = g->mesh->data.polygons[q].vertices;
        if (along_z)
        {
            v[0] = (unsigned short)(n + 1);
            v[1] = (unsigned short)(n + vertices + 1);
            v[2] = (unsigned short)(n + vertices);
            v[3] = (unsigned short)n;
        }
        else
        {
            v[0] = (unsigned short)n;
            v[1] = (unsigned short)(n + 1);
            v[2] = (unsigned short)(n + vertices + 1);
            v[3] = (unsigned short)(n + vertices);
        }
    }
}

/** @brief The attributes of the quads (both sets): the color (or the grain); the foam texture is put on the quads with
 *  foam at each update (level_tables()) */
static void                 set_polygons(jo_water * const w, level_data * const g)
{
    unsigned int            q;
    int                     s;

    for (JO_ZERO(s); s < 2; ++s)
    {
        g->mesh->data.attributes = g->attributes[s];
        for (JO_ZERO(q); q < g->mesh->data.polygon_count; ++q)
        {
            if (w->config.grain && w->grain >= 0)
                jo_3d_set_mesh_polygon_texture(g->mesh, w->grain, q);
            else
                jo_3d_set_mesh_polygon_color(g->mesh, w->config.color, q);
            g->attributes[s][q].plane = JO_3D_DUAL_PLANE;
            g->attributes[s][q].draw_mode |= JO_VDP1_COLOR_CALC_GOURAUD;
            g->attributes[s][q].gouraud_table = (unsigned short)(g->gouraud[s] + q);
        }
    }
    g->mesh->data.attributes = g->attributes[g->set];
    // The attributes of a quad without and with the foam texture (copied to the quads in level_tables())
    w->plain = g->attributes[0][0];
    w->foamy = w->plain;
    w->bottomy = w->plain;
    if (w->foam_tiles[0][0][0] >= 0)
    {
        jo_3d_set_mesh_polygon_texture(g->mesh, w->foam_tiles[0][0][0], 0);
        w->foamy = g->mesh->data.attributes[0];
        w->foamy.plane = JO_3D_DUAL_PLANE;
        w->foamy.draw_mode |= JO_VDP1_COLOR_CALC_GOURAUD;
        g->mesh->data.attributes[0] = w->plain;
    }
    if (w->bottom_tiles[0][0][0] > 0)
    {
        jo_3d_set_mesh_polygon_texture(g->mesh, w->bottom_tiles[0][0][0], 0);
        w->bottomy = g->mesh->data.attributes[0];
        w->bottomy.plane = JO_3D_DUAL_PLANE;
        w->bottomy.draw_mode |= JO_VDP1_COLOR_CALC_GOURAUD;
        g->mesh->data.attributes[0] = w->plain;
    }
}

void                        jo_water_set_textured(jo_water * const w, const bool textured)
{
    int                     k;

    w->config.textured = textured;
    for (JO_ZERO(k); k < w->level_count; ++k)
        set_polygons(w, &w->levels[k]);
}

static void                 prepare_bottom(jo_water * const w);

void                        jo_water_set_look(jo_water * const w, const jo_water_config * const look)
{
    jo_water_config         *c;

    c = &w->config;
    c->color = look->color;
    c->shallow = look->shallow;
    c->deep = look->deep;
    c->sky = look->sky;
    c->slope_light_min = look->slope_light_min;
    c->slope_light_max = look->slope_light_max;
    c->glint = look->glint;
    c->spot = look->spot;
    c->spot_distance = look->spot_distance;
    c->caustics = look->caustics;
    c->reflection = look->reflection;
    c->foam_white = look->foam_white;
    c->foam_color = look->foam_color;
    c->shore = look->shore;
    c->shallow_depth = look->shallow_depth;
    c->surf = look->surf;
    c->bottom = look->bottom;
    c->bottom_depth = look->bottom_depth;
    c->see_through = look->see_through;
    if (w->bottom_tiles[0][0][0] > 0)
        prepare_bottom(w);
    c->grain = look->grain;
    jo_water_set_textured(w, c->textured);
}

/** @brief A value of the foam pattern (4 bits, 2 per byte)
 *  @param pattern foam_pattern or foam_pattern_half
 *  @param size Its side (pixels)
 */
static  __jo_force_inline int  pattern_at(const unsigned char * const pattern, const int size, const int x, const int y)
{
    unsigned char           b;

    b = pattern[(y * size + x) >> 1];
    return (x & 1 ? b & 15 : b >> 4);
}

/** @brief The foam texture: the tiles of the pattern for the levels 0 and 1 (a quad of 8 units: a tile of 16 x 16 texels,
 *  of 16 units: the pattern at half size), each one twice: A-D along x (the texture along x and z), A-D along z (the
 *  texture turned: u along z, v along -x, see set_quad_order()); the color lookup tables (a block of 4 tables)
 */
static bool                 create_foam_textures(jo_water * const w, const unsigned char * const foam_pattern,
                                                 const unsigned char * const foam_pattern_half)
{
    unsigned char           pixels[FOAM_TILE * FOAM_TILE / 2];
    unsigned char           blank[FOAM_TILE * FOAM_TILE / 2 * ((FOAM_LUTS + 3) / 4)];
    jo_color                lut[16];
    const unsigned char     *pattern;
    int                     level;
    int                     tiles;
    int                     size;
    int                     turned;
    int                     a;
    int                     b;
    int                     u;
    int                     v;
    int                     p;
    int                     id;
    int                     k;

    for (JO_ZERO(u); u < 16; ++u)
        lut[u] = w->config.color;
    for (JO_ZERO(level); level < FOAM_TEXTURE_LEVELS; ++level)
    {
        // (only the sets of the levels: quads of 8 units for the set 0, of 16 for the set 1)
        for (JO_ZERO(k); k < w->level_count && w->level_sizes[k].cell != (8 << level); ++k)
            ;
        if (k == w->level_count)
            continue;
        pattern = level == 0 ? foam_pattern : foam_pattern_half;
        size = level == 0 ? FOAM_PATTERN_SIZE : FOAM_PATTERN_SIZE / 2;
        tiles = size / FOAM_TILE;
        for (JO_ZERO(turned); turned < 2; ++turned)
            for (JO_ZERO(b); b < tiles; ++b)
                for (JO_ZERO(a); a < tiles; ++a)
                {
                    // (the tile of the column a (x) and the row b (z) of the pattern)
                    for (JO_ZERO(v); v < FOAM_TILE; ++v)
                        for (JO_ZERO(u); u < FOAM_TILE; ++u)
                        {
                            p = turned ? pattern_at(pattern, size, a * FOAM_TILE + FOAM_TILE - 1 - v, b * FOAM_TILE + u) :
                                         pattern_at(pattern, size, a * FOAM_TILE + u, b * FOAM_TILE + v);
                            if (u & 1)
                                pixels[(v * FOAM_TILE + u) >> 1] |= (unsigned char)p;
                            else
                                pixels[(v * FOAM_TILE + u) >> 1] = (unsigned char)(p << 4);
                        }
                    if ((w->foam_tiles[level][turned][b * tiles + a] = jo_sprite_add_4bits_lut(pixels, FOAM_TILE, FOAM_TILE, lut)) < 0)
                        return (false);
                }
    }
    // The tables: a block of 16 x 16 texels (4 x 32 bytes) or more, its pixels are the tables (see animate_foam())
    jo_memset(blank, 0, sizeof(blank));
    if ((id = jo_sprite_add_4bits_lut(blank, FOAM_TILE, FOAM_TILE * ((FOAM_LUTS + 3) / 4), lut)) < 0)
        return (false);
    for (JO_ZERO(u); u < FOAM_LUTS; ++u)
        w->foam_luts[u] = (unsigned short)(__jo_sprite_def[id].adr + u * 4);
    return (true);
}

/** @brief The color lookup tables of the bottom for each place of the band of light (see animate_bottom()): the color of
 *  the water, a bit darker or lighter along the lines of the pattern (its values: a phase, see tools/make_foam.py), a band of
 *  light at a phase that moves with the time (the light of the waves on the sand); the same mean (the color of the water: the
 *  light of the vertices makes the shallow water lighter); less and less from the shore to bottom_depth. The channels under
 *  31 - 10 (the VDP1 doesn't saturate the gouraud). Made once (computed at each frame, 1 ms of the master CPU) */
static void                 prepare_bottom(jo_water * const w)
{
    // (the band of light: (cos of the phase from its middle)^4 x 16, 0 under 0; its mean over the 15 values: 3)
    static const signed char band[BOTTOM_PHASES] = {16, 11, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 11};
    // (the lines: cos of the phase x 8)
    static const signed char lines[BOTTOM_PHASES] = {8, 7, 5, 2, -1, -4, -6, -8, -8, -6, -4, -1, 2, 5, 7};
    unsigned short          *lut;
    int                     color[3];
    int                     moved;
    int                     k;
    int                     v;
    int                     c;
    int                     light;
    int                     strength;
    int                     channel;
    unsigned short          value;

    color[0] = w->config.color & 31;
    color[1] = (w->config.color >> 5) & 31;
    color[2] = (w->config.color >> 10) & 31;
    for (JO_ZERO(moved); moved < BOTTOM_PHASES; ++moved)
        for (JO_ZERO(k); k < BOTTOM_LUTS; ++k)
        {
            lut = w->bottom_cache[moved][k];
            // (the contrast: bottom at the shore, less with the depth: the middle of the band of depth of the table)
            strength = w->config.bottom * (2 * BOTTOM_LUTS - 2 * k - 1);
            lut[0] = w->config.color;
            for (v = 1; v < 16; ++v)
            {
                // (x 16 x 2 x BOTTOM_LUTS: the light in steps x 256)
                light = (band[(v - 1 + BOTTOM_PHASES - moved) % BOTTOM_PHASES] - 3) * 2 + lines[v - 1];
                light = light * strength * 256 / (16 * 2 * BOTTOM_LUTS);
                value = 0x8000;
                for (JO_ZERO(channel); channel < 3; ++channel)
                {
                    // (the blue less: the light of the sand is yellow)
                    c = color[channel] + ((channel == 2 ? light / 2 : light) >> 8);
                    c = JO_MAX(0, JO_MIN(31 - (GOURAUD_MAX - 16), c));
                    value |= (unsigned short)(c << (channel * 5));
                }
                lut[v] = value;
            }
        }
    w->bottom_phase = -1;
}

/** @brief The band of light of the bottom moves by a value every 4 frames: its tables copied to the VRAM (one after the other
 *  there, see create_bottom_textures(); by words of 32 bits: the bus of the VRAM is slow) */
static void                 animate_bottom(jo_water * const w)
{
    volatile unsigned int   *vram;
    const unsigned int      *cache;
    int                     moved;
    int                     k;

    moved = (w->frame >> 2) % BOTTOM_PHASES;
    if (moved == w->bottom_phase)
        return;
    w->bottom_phase = moved;
    vram = (volatile unsigned int *)(JO_VDP1_VRAM + w->bottom_luts[0] * 8);
    cache = (const unsigned int *)w->bottom_cache[moved];
    for (JO_ZERO(k); k < BOTTOM_LUTS * 8; ++k)
        vram[k] = cache[k];
}

/** @brief The texture of the bottom: the tiles of its pattern for the levels 0 to 3 (bottom_pattern_k: 128 >> k texels for
 *  64 units, the quads 8 << k units: tiles of 16 x 16 texels), along x and along z (turned, as the foam); the color lookup
 *  tables (see animate_bottom()) */
static bool                 create_bottom_textures(jo_water * const w, const unsigned char * const bottom_pattern_0)
{
    const unsigned char     *patterns[BOTTOM_LEVELS] = {bottom_pattern_0};
    unsigned char           pixels[FOAM_TILE * FOAM_TILE / 2];
    unsigned char           blank[FOAM_TILE * FOAM_TILE / 2 * ((BOTTOM_LUTS + 3) / 4)];
    jo_color                lut[16];
    int                     level;
    int                     tiles;
    int                     size;
    int                     turned;
    int                     a;
    int                     b;
    int                     u;
    int                     v;
    int                     p;
    int                     id;

    for (JO_ZERO(u); u < 16; ++u)
        lut[u] = w->config.color;
    for (JO_ZERO(level); level < BOTTOM_LEVELS; ++level)
    {
        size = FOAM_PATTERN_SIZE >> level;
        tiles = size / FOAM_TILE;
        for (JO_ZERO(turned); turned < 2; ++turned)
            for (JO_ZERO(b); b < tiles; ++b)
                for (JO_ZERO(a); a < tiles; ++a)
                {
                    for (JO_ZERO(v); v < FOAM_TILE; ++v)
                        for (JO_ZERO(u); u < FOAM_TILE; ++u)
                        {
                            p = turned ? pattern_at(patterns[level], size, a * FOAM_TILE + FOAM_TILE - 1 - v, b * FOAM_TILE + u) :
                                         pattern_at(patterns[level], size, a * FOAM_TILE + u, b * FOAM_TILE + v);
                            if (u & 1)
                                pixels[(v * FOAM_TILE + u) >> 1] |= (unsigned char)p;
                            else
                                pixels[(v * FOAM_TILE + u) >> 1] = (unsigned char)(p << 4);
                        }
                    if ((w->bottom_tiles[level][turned][b * tiles + a] = jo_sprite_add_4bits_lut(pixels, FOAM_TILE, FOAM_TILE, lut)) < 0)
                        return (false);
                }
    }
    jo_memset(blank, 0, sizeof(blank));
    if ((id = jo_sprite_add_4bits_lut(blank, FOAM_TILE, FOAM_TILE * ((BOTTOM_LUTS + 3) / 4), lut)) < 0)
        return (false);
    for (JO_ZERO(u); u < BOTTOM_LUTS; ++u)
        w->bottom_luts[u] = (unsigned short)(__jo_sprite_def[id].adr + u * 4);
    prepare_bottom(w);
    return (true);
}

/** @brief A mesh of quads that share their points (its own 4 points per quad given back), JO_NULL if out of memory (or no
 *  points) */
static jo_3d_mesh           *create_shared_mesh(const int quads, jo_pos3D_fixed * const points, const int point_count)
{
    jo_3d_mesh              *mesh;

    if (points == JO_NULL || (mesh = jo_3d_create_mesh((unsigned int)quads)) == JO_NULL)
        return (JO_NULL);
    jo_free(mesh->data.points);
    mesh->data.points = points;
    mesh->data.point_count = (unsigned int)point_count;
    return (mesh);
}

/** @brief Free a block if there is one (a water not fully created) */
static void                 free_block(const void * const p)
{
    if (p != JO_NULL)
        jo_free(p);
}

/** @brief Free a mesh of create_shared_mesh() with its points, or its points alone (the mesh not created) */
static void                 free_shared_mesh(const jo_3d_mesh * const mesh, const jo_pos3D_fixed * const points)
{
    if (mesh != JO_NULL)
        jo_3d_free_mesh(mesh);
    else
        free_block(points);
}

static bool                 create_level(level_data * const g, const int cells, const int cell)
{
    int                     vertices;
    int                     quads;
    int                     ring;
    int                     i;
    int                     j;
    int                     q;

    g->cells = cells;
    g->cell = cell;
    g->hole_i = -1;
    g->hole_j = -1;
    g->blend_hole_i = -1000;
    vertices = cells + 1;
    quads = cells * cells;
    ring = 4 * BLEND_ROWS * vertices;
    g->points = (jo_pos3D_fixed *)jo_malloc((unsigned int)(vertices * vertices) * sizeof(jo_pos3D_fixed));
    g->heights = (jo_fixed *)jo_malloc((unsigned int)(vertices * vertices) * sizeof(jo_fixed));
    g->colors = (jo_color *)jo_malloc((unsigned int)(vertices * vertices) * sizeof(jo_color));
    g->foam = (unsigned char *)jo_malloc((unsigned int)(vertices * vertices));
    g->depth = (unsigned char *)jo_malloc((unsigned int)(vertices * vertices));
    g->quad_i = (unsigned char *)jo_malloc((unsigned int)quads);
    g->quad_j = (unsigned char *)jo_malloc((unsigned int)quads);
    g->tables = (unsigned int *)jo_malloc((unsigned int)quads * 8);
    g->attributes[1] = (jo_3d_polygon_attribute *)jo_malloc((unsigned int)quads * sizeof(jo_3d_polygon_attribute));
    g->blend_vertex = (unsigned short *)jo_malloc((unsigned int)ring * sizeof(unsigned short));
    g->blend_parent = (unsigned short *)jo_malloc((unsigned int)ring * sizeof(unsigned short));
    g->blend_code = (unsigned char *)jo_malloc((unsigned int)ring);
    if (g->points == JO_NULL || g->heights == JO_NULL || g->colors == JO_NULL || g->foam == JO_NULL || g->depth == JO_NULL ||
        g->quad_i == JO_NULL ||
        g->quad_j == JO_NULL || g->tables == JO_NULL || g->attributes[1] == JO_NULL || g->blend_vertex == JO_NULL ||
        g->blend_parent == JO_NULL || g->blend_code == JO_NULL ||
        (g->mesh = create_shared_mesh(quads, g->points, vertices * vertices)) == JO_NULL)
        return (false);
    // The quads share the vertices of the level, line by line (the quads of the hole are skipped when drawn)
    for (JO_ZERO(j); j < vertices; ++j)
        for (JO_ZERO(i); i < vertices; ++i)
        {
            g->points[j * vertices + i].x = jo_int2fixed(i * cell);
            g->points[j * vertices + i].z = jo_int2fixed(j * cell);
            JO_ZERO(g->points[j * vertices + i].y);
            JO_ZERO(g->heights[j * vertices + i]);
            JO_ZERO(g->foam[j * vertices + i]);
            g->depth[j * vertices + i] = 255;
        }
    JO_ZERO(q);
    for (JO_ZERO(j); j < cells; ++j)
        for (JO_ZERO(i); i < cells; ++i, ++q)
        {
            g->quad_i[q] = (unsigned char)i;
            g->quad_j[q] = (unsigned char)j;
        }
    g->along_z = true;
    set_quad_order(g, false);
    g->gouraud[0] = jo_3d_add_gouraud_tables(JO_NULL, (unsigned int)quads);
    g->gouraud[1] = jo_3d_add_gouraud_tables(JO_NULL, (unsigned int)quads);
    // (2 arrays of attributes: their gouraud tables don't change, the mesh takes one or the other)
    g->attributes[0] = g->mesh->data.attributes;
    jo_memcpy(g->attributes[1], g->attributes[0], (unsigned int)quads * sizeof(jo_3d_polygon_attribute));
    return (g->gouraud[0] != 0 && g->gouraud[1] != 0);
}

/** @brief Free a level (its gouraud tables apart: see free_water()) */
static void                 free_level(level_data * const g)
{
    // (the mesh with its own attributes: it may have the second array, see set_polygons())
    if (g->mesh != JO_NULL && g->attributes[0] != JO_NULL)
        g->mesh->data.attributes = g->attributes[0];
    free_shared_mesh(g->mesh, g->points);
    free_block(g->attributes[1]);
    free_block(g->heights);
    free_block(g->colors);
    free_block(g->foam);
    free_block(g->depth);
    free_block(g->quad_i);
    free_block(g->quad_j);
    free_block(g->tables);
    free_block(g->blend_vertex);
    free_block(g->blend_parent);
    free_block(g->blend_code);
}

/*
** The skirt: flat quads around the last level up to config.skirt units from its center (from high up, the end of the
** grid was seen). A ring of 4 x skirt_segments quads, its inner vertices on the edge of the last level (no crack).
*/

/** @brief A point of a square ring, counterclockwise from the corner (0, 0) */
static void                 ring_point(const jo_water * const w, jo_pos3D_fixed * const p, const int k, const int low, const int side)
{
    int                     s;
    int                     t;

    s = k / w->skirt_segments;
    t = (k % w->skirt_segments) * side / w->skirt_segments;
    p->y = 0;
    p->x = jo_int2fixed(low + (s == 0 ? t : s == 1 ? side : s == 2 ? side - t : 0));
    p->z = jo_int2fixed(low + (s == 0 ? 0 : s == 1 ? t : s == 2 ? side : side - t));
}

/** @brief Vertex of the last level at the point k of the inner ring of the skirt */
static int                  ring_vertex(const jo_water * const w, const int k)
{
    int                     s;
    int                     c;
    int                     i;
    int                     j;
    int                     n;

    n = w->skirt_segments;
    s = k / n;
    c = k % n;
    i = s == 0 ? c : s == 1 ? n : s == 2 ? n - c : 0;
    j = s == 0 ? 0 : s == 1 ? c : s == 2 ? n : n - c;
    return (j * (n + 1) + i);
}

static bool                 create_skirt(jo_water * const w)
{
    int                     side;
    int                     quads;
    int                     k;
    jo_3d_polygon           *polygon;

    w->skirt_segments = w->levels[w->level_count - 1].cells;
    quads = 4 * w->skirt_segments;
    w->skirt_points = (jo_pos3D_fixed *)jo_malloc((unsigned int)(2 * quads) * sizeof(jo_pos3D_fixed));
    if ((w->skirt = create_shared_mesh(quads, w->skirt_points, 2 * quads)) == JO_NULL)
        return (false);
    side = w->level_sizes[w->level_count - 1].side;
    for (JO_ZERO(k); k < quads; ++k)
    {
        ring_point(w, &w->skirt_points[k], k, 0, side);
        ring_point(w, &w->skirt_points[quads + k], k, side / 2 - w->config.skirt, 2 * w->config.skirt);
    }
    for (JO_ZERO(k); k < quads; ++k)
    {
        polygon = &w->skirt->data.polygons[k];
        polygon->vertices[0] = (unsigned short)k;
        polygon->vertices[1] = (unsigned short)((k + 1) % quads);
        polygon->vertices[2] = (unsigned short)(quads + (k + 1) % quads);
        polygon->vertices[3] = (unsigned short)(quads + k);
        jo_3d_set_mesh_polygon_color(w->skirt, w->config.color, (unsigned int)k);
        w->skirt->data.attributes[k].plane = JO_3D_DUAL_PLANE;
        w->skirt->data.attributes[k].draw_mode |= JO_VDP1_COLOR_CALC_GOURAUD;
    }
    w->skirt_gouraud[0] = jo_3d_add_gouraud_tables(JO_NULL, (unsigned int)quads);
    w->skirt_gouraud[1] = jo_3d_add_gouraud_tables(JO_NULL, (unsigned int)quads);
    return (w->skirt_gouraud[0] != 0 && w->skirt_gouraud[1] != 0);
}

/** @brief Free the memory of a water and its gouraud tables (see jo_water_free()) */
static void                 free_water(jo_water * const w)
{
    int                     quads;
    int                     k;

    // (the gouraud tables in the reverse order of their adding: given back when they are the last ones, see
    // jo_3d_free_gouraud_tables())
    jo_3d_free_gouraud_tables(w->skirt_gouraud[1], (unsigned int)(4 * w->skirt_segments));
    jo_3d_free_gouraud_tables(w->skirt_gouraud[0], (unsigned int)(4 * w->skirt_segments));
    free_block(w->foam_grid);
    free_shared_mesh(w->skirt, w->skirt_points);
    for (k = w->level_count - 1; k >= 0; --k)
    {
        quads = w->levels[k].cells * w->levels[k].cells;
        jo_3d_free_gouraud_tables(w->levels[k].gouraud[1], (unsigned int)quads);
        jo_3d_free_gouraud_tables(w->levels[k].gouraud[0], (unsigned int)quads);
        free_level(&w->levels[k]);
    }
    if (current == w)
        current = JO_NULL;
    jo_free(w);
}

/** @brief jo_water_create() failed: the water freed, the sprites it added given back (all the sprites since its start) */
static jo_water             *create_failed(jo_water * const w, const int first_sprite)
{
    free_water(w);
    jo_sprite_free_from(first_sprite);
    return (JO_NULL);
}

jo_water                    *jo_water_create(const jo_water_config * const config)
{
    jo_water                *w;
    level_data              *g;
    int                     first_sprite;
    int                     k;
    int                     cell;
    int                     side;

    init_tables();
    if ((w = (jo_water *)jo_malloc(sizeof(jo_water))) == JO_NULL)
        return (JO_NULL);
    jo_memset(w, 0, sizeof(jo_water));
    first_sprite = jo_sprite_count();
    current = w;
    w->config = *config;
    w->level_count = JO_MAX(0, JO_MIN(config->level_count, JO_WATER_MAX_LEVELS));
    jo_memcpy(w->level_sizes, config->levels, (unsigned int)w->level_count * sizeof(jo_water_level));
    w->config.levels = w->level_sizes;
    w->config.level_count = w->level_count;
    // (no level: the skirt around the last level and the heights out of the levels read before the array of the levels)
    if (w->level_count == 0)
    {
#ifdef JO_DEBUG
        jo_core_error("water: no level");
#endif
        return (create_failed(w, first_sprite));
    }
    w->swell = JO_FIXED_1;
    w->grain = -1;
    // The tiles of the foam and of the bottom from their patterns: built in, or from a level file (freed once they are made)
    {
        const unsigned char *patterns;
        bool                made;
#ifdef JO_COMPILE_WITH_FS_SUPPORT
        jo_level            *file;

        file = JO_NULL;
        if (config->patterns_file != JO_NULL)
        {
            if ((file = jo_level_load(config->patterns_file)) == JO_NULL)
                return (create_failed(w, first_sprite));
            patterns = (const unsigned char *)jo_level_section_data(file, FOAM_SECTION);
        }
        else
#endif
            patterns = __jo_water_patterns;
        made = patterns != JO_NULL && *(const unsigned short *)patterns == FOAM_PATTERN_SIZE &&
               create_foam_textures(w, patterns + 4, patterns + 4 + FOAM_PATTERN_SIZE * FOAM_PATTERN_SIZE / 2) &&
               (config->bottom <= 0 || create_bottom_textures(w, patterns + 4 + FOAM_PATTERN_SIZE * FOAM_PATTERN_SIZE / 2 +
                                                                 FOAM_PATTERN_SIZE * FOAM_PATTERN_SIZE / 8));
#ifdef JO_COMPILE_WITH_FS_SUPPORT
        if (file != JO_NULL)
            jo_level_free(file);
#endif
        if (!made)
            return (create_failed(w, first_sprite));
    }
// (a file of the CD: the program built without the file system had a compilation error)
#if defined(JO_COMPILE_WITH_TGA_SUPPORT) && defined(JO_COMPILE_WITH_FS_SUPPORT)
    if (config->grain)
        w->grain = jo_sprite_add_tga(JO_ROOT_DIR, "GRAIN.TGA", JO_COLOR_Transparent);
#endif
    for (JO_ZERO(k); k < w->level_count; ++k)
    {
        cell = w->level_sizes[k].cell;
        side = w->level_sizes[k].side;
        // (the configuration, see jo_water_level)
        if (cell <= 0 || side % cell != 0 || side / cell > JO_WATER_MAX_CELLS ||
            (k > 0 && (cell % w->level_sizes[k - 1].cell != 0 || w->level_sizes[k - 1].side % cell != 0 ||
                       side < w->level_sizes[k - 1].side + 4 * cell)))
        {
            // (no message in a release build: it stayed on the screen of the game, and jo_printf() needs the printf module)
#ifdef JO_DEBUG
            jo_core_error("water: wrong level %d", k);
#endif
            return (create_failed(w, first_sprite));
        }
        g = &w->levels[k];
        if (!create_level(g, side / cell, cell))
            return (create_failed(w, first_sprite));
        g->period = cell <= config->update_cell ? 1 : cell <= 2 * config->update_cell ? 2 : 4;
        if (k == 0)
            continue;
        w->levels[k - 1].ratio = cell / w->level_sizes[k - 1].cell;
        w->levels[k - 1].inv_ratio = JO_FIXED_1 / w->levels[k - 1].ratio;
        g->hole_cells = w->level_sizes[k - 1].side / cell;
    }
    if (!create_skirt(w))
        return (create_failed(w, first_sprite));
    if ((w->foam_grid = (foam_cell *)jo_malloc(FOAM_GRID * FOAM_GRID * sizeof(foam_cell))) == JO_NULL)
        return (create_failed(w, first_sprite));
    jo_memset(w->foam_grid, 0, FOAM_GRID * FOAM_GRID * sizeof(foam_cell));
    // (the events: a short life, 3 quads per wavelength are enough)
    w->splash_level = last_level_for(w, config->splash_length * config->min_cells_per_wave / 3);
    w->splash_k = 65536 / JO_MAX(1, config->splash_length);
    w->splash_envelope_k = 16384 / JO_MAX(1, config->splash_length);
    w->front_k = 65536 / JO_MAX(1, config->front_length);
    w->front_envelope_k = 16384 / JO_MAX(1, config->front_length);
    // (behind: 0 to 2 wavelengths, the index 0 to FRONT_SHAPE - 1)
    w->front_index = (FRONT_SHAPE << 16) / JO_MAX(1, 2 * config->front_length);
    for (JO_ZERO(k); k < FRONT_SHAPE; ++k)
    {
        int                 behind;

        behind = (2 * config->front_length * k + config->front_length) / FRONT_SHAPE;
        w->front_shape[k] = jo_fixed_mult(jo_sin_angle((jo_angle)(behind * w->front_envelope_k)),
                                          jo_sin_angle((jo_angle)(behind * w->front_k)));
    }
    w->front_level = last_level_for(w, config->front_length * config->min_cells_per_wave / 3);
    if (config->waves != JO_NULL)
        jo_water_set_waves(w, config->waves, config->wave_count);
    else
    {
        jo_water_wave       waves[JO_WATER_MAX_WAVES];

        jo_water_set_waves(w, waves, jo_water_swell(waves, JO_WATER_SWELL_CHOPPY));
    }
    jo_water_set_sun(w, 0, JO_DEG_TO_ANGLE(30));
    jo_water_set_textured(w, config->textured);
    // The 2 CPUs: half of the rows each at the start
    w->height_split.share = JO_FIXED_1 / 2;
    w->light_split.share = JO_FIXED_1 / 2;
    w->slave_draw_levels = JO_MAX(0, JO_MIN(config->slave_levels, w->level_count - 1));
    return (w);
}

void                        jo_water_free(jo_water * const w)
{
    if (w == JO_NULL)
        return;
    if (__jo_water_objects_frame != JO_NULL)
        __jo_water_objects_frame(w, __JO_WATER_OBJECTS_FREE);
    free_water(w);
}

/*
** Heights
*/

/** @brief A wave on the vertices from to to (excluded) of a row
 *  @param set Set the heights (else add the wave)
 */
static  __jo_force_inline void  wave_span(jo_fixed * const row, const bool set, const jo_fixed * const table,
                                          unsigned int phase, const unsigned int step, const int from, const int to)
{
    int                     i;

    // (the phase: a jo_angle in the low 16 bits, the step of the table in bits 8 to 15)
    phase += (unsigned int)from * step;
    if (set)
        for (i = from; i < to; ++i, phase += step)
            row[i] = table[(phase >> 8) & 255];
    else
        for (i = from; i < to; ++i, phase += step)
            row[i] += table[(phase >> 8) & 255];
}

/** @brief Fade of the waves of a level at a point: 1 inside, 0 on its edge (jo_fixed)
 *  @param edge_z Distance to the edge along z (units)
 */
static  __jo_force_inline jo_fixed  fade_at(const jo_water * const w, const int level, const int x, const int edge_z)
{
    int                     edge;

    edge = JO_MIN(edge_z, JO_MIN(x - w->bound_x[level], w->bound_x[level] + w->bound_side[level] - x));
    if (edge >= w->config.fade)
        return (JO_FIXED_1);
    return (JO_MAX(0, edge) * (JO_FIXED_1 / w->config.fade));
}

/** @brief Heights of rows of a level (on either CPU: it only writes the heights of these rows, not those inside its hole)
 *  @param k The level
 *  @param first First row
 *  @param last After the last row
 */
static void                 level_heights(const int k, const int first, const int last)
{
    jo_water                *w;
    level_data              *g;
    jo_fixed                rows[JO_WATER_MAX_LEVELS][JO_WATER_MAX_CELLS + 1];
    bool                    used[JO_WATER_MAX_LEVELS];
    int                     phases[JO_WATER_MAX_WAVES];
    int                     vertices;
    int                     i;
    int                     j;
    int                     l;
    int                     x;
    int                     z;
    int                     start;
    int                     end;
    int                     edge_z;
    int                     events;
    int                     lo;
    int                     hi;
    unsigned int            step;
    const wave_data         *d;
    jo_fixed                *row;

    w = current;
    g = &w->levels[k];
    vertices = g->cells + 1;
    for (JO_ZERO(i); i < w->wave_count; ++i)
    {
        d = &w->waves[i];
        phases[i] = g->origin_x * d->kx + (g->origin_z + first * g->cell) * d->kz + w->frame * d->w;
    }
    // The events (splashes, fronts): with the waves of their last level
    events = JO_MAX(w->live_splash_count > 0 ? w->splash_level : -1, w->live_front_count > 0 ? w->front_level : -1);
    for (j = first; j < last; ++j)
    {
        row = g->heights + j * vertices;
        z = g->origin_z + j * g->cell;
        // The vertices of the row: 2 spans in the rows of the hole (its inside is not used; HOLE_MARGIN vertices inside it
        // are kept: the light of the edge of the level inside it uses them)
        end = vertices;
        start = vertices;
        if (g->hole_i >= 0 && j > g->hole_j + HOLE_MARGIN && j < g->hole_j + g->hole_cells - HOLE_MARGIN &&
            g->hole_cells > 2 * HOLE_MARGIN)
        {
            end = g->hole_i + HOLE_MARGIN + 1;
            start = g->hole_i + g->hole_cells - HOLE_MARGIN;
        }
        // (only the vertices in the field of view: lo to hi)
        lo = g->span_first[j];
        hi = g->span_end[j];
        // The waves, one row per group (the waves with the same last level: the same fade)
        for (JO_ZERO(l); l < w->level_count; ++l)
            used[l] = false;
        for (JO_ZERO(i); i < w->wave_count; ++i)
        {
            d = &w->waves[i];
            if (d->last_level < k)
                continue;
            step = (unsigned int)(d->kx * g->cell);
            wave_span(rows[d->last_level], !used[d->last_level], d->table, (unsigned int)phases[i], step, lo, JO_MIN(end, hi));
            wave_span(rows[d->last_level], !used[d->last_level], d->table, (unsigned int)phases[i], step, JO_MAX(start, lo), hi);
            used[d->last_level] = true;
            phases[i] += d->kz * g->cell;
        }
        for (JO_ZERO(i); i < w->wave_count; ++i)
            if (w->waves[i].last_level < k)
                phases[i] += w->waves[i].kz * g->cell;
        if (events >= k)
        {
            if (!used[events])
            {
                for (JO_ZERO(i); i < vertices; ++i)
                    JO_ZERO(rows[events][i]);
                used[events] = true;
            }
            if (w->live_front_count > 0 && w->front_level >= k)
                add_fronts_row(w, rows[w->front_level], lo, hi, g->origin_x, z, g->cell);
            if (w->live_splash_count > 0 && w->splash_level >= k)
                add_splashes_row(w, rows[w->splash_level], vertices, g->origin_x, z, g->cell, end, start);
        }
        // The sum of the groups, each one faded toward the edge of its last level (not the last level: the skirt)
        for (i = lo; i < hi; ++i)
            JO_ZERO(row[i]);
        for (JO_ZERO(l); l < w->level_count; ++l)
        {
            if (!used[l])
                continue;
            if (l == w->level_count - 1)
            {
                for (i = lo; i < hi; ++i)
                    row[i] += rows[l][i];
                continue;
            }
            edge_z = JO_MIN(z - w->bound_z[l], w->bound_z[l] + w->bound_side[l] - z);
            for (i = lo; i < hi; ++i)
            {
                if (i == end)
                    i = start;
                if (i >= hi)
                    break;
                x = g->origin_x + i * g->cell;
                row[i] += jo_fixed_mult(rows[l][i], fade_at(w, l, x, edge_z));
            }
        }
    }
}

/** @brief Position of a vertex of a level on the next level
 *  @param i Column (or row) of the vertex in its level
 *  @param hole First column (or row) of the hole of the next level
 *  @param a Column (or row) of the vertex of the next level before it (output)
 *  @return Fraction of the way to the next one (0 to ratio - 1)
 */
static  __jo_force_inline int  on_parent(const level_data * const g, const int i, const int hole, int * const a)
{
    int                     f;

    // (i / ratio without a division: the ratio is small, a few subtractions)
    *a = hole;
    f = i;
    while (f >= g->ratio)
    {
        f -= g->ratio;
        ++*a;
    }
    return (f);
}

/** @brief Put the heights of the edge of a level on the next level (no crack) */
static void                 stitch_level(jo_water * const w, const int k)
{
    level_data              *g;
    const level_data        *p;
    int                     vertices;
    int                     e;
    int                     s;
    int                     n;
    int                     i;
    int                     j;
    int                     a;
    int                     b;
    int                     f;
    int                     pa;
    int                     pb;

    g = &w->levels[k];
    p = &w->levels[k + 1];
    vertices = g->cells + 1;
    for (JO_ZERO(s); s < 4; ++s)
        for (JO_ZERO(e); e < vertices; ++e)
        {
            // The 4 sides: top, bottom, left, right; along a side, between 2 vertices of the next level
            i = s < 2 ? e : (s == 2 ? 0 : g->cells);
            j = s < 2 ? (s == 0 ? 0 : g->cells) : e;
            n = j * vertices + i;
            if (s < 2)
            {
                f = on_parent(g, i, p->hole_i, &a);
                on_parent(g, j, p->hole_j, &b);
                pa = b * (p->cells + 1) + a;
                pb = pa + (f ? 1 : 0);
            }
            else
            {
                on_parent(g, i, p->hole_i, &a);
                f = on_parent(g, j, p->hole_j, &b);
                pa = b * (p->cells + 1) + a;
                pb = pa + (f ? p->cells + 1 : 0);
            }
            g->heights[n] = p->heights[pa] + jo_fixed_mult(p->heights[pb] - p->heights[pa], f * g->inv_ratio);
        }
}

/*
** The edge of a level: its colors go to the colors of the next level (its normals are from bigger quads), the same color
** on the edge: no line where the levels meet. The vertices and their place on the next level are listed once (when the
** level moves in the next one), then a few means of colors per vertex.
*/

/** @brief Mean of 2 colors (the 3 channels at once, without carry between them) */
static  __jo_force_inline jo_color  mean_color(const jo_color a, const jo_color b)
{
    return ((jo_color)(0x8000 | ((((a & 0x7BDE) + (b & 0x7BDE)) >> 1) + (a & b & 0x0421))));
}

/** @brief The list of the vertices of the edge of a level and their place on the next level */
static void                 prepare_blend(jo_water * const w, const int k)
{
    level_data              *g;
    const level_data        *p;
    int                     vertices;
    int                     i;
    int                     j;
    int                     e;
    int                     a;
    int                     b;
    int                     fi;
    int                     fj;

    g = &w->levels[k];
    p = &w->levels[k + 1];
    if (g->blend_hole_i == p->hole_i && g->blend_hole_j == p->hole_j)
        return;
    g->blend_hole_i = p->hole_i;
    g->blend_hole_j = p->hole_j;
    vertices = g->cells + 1;
    JO_ZERO(g->blend_count);
    for (JO_ZERO(j); j < vertices; ++j)
        for (JO_ZERO(i); i < vertices; ++i)
        {
            e = JO_MIN(JO_MIN(i, j), JO_MIN(g->cells - i, g->cells - j));
            if (e >= BLEND_ROWS)
            {
                // (the inside: to the other side of the row)
                if (i == BLEND_ROWS)
                    i = g->cells - BLEND_ROWS;
                continue;
            }
            fi = on_parent(g, i, p->hole_i, &a);
            fj = on_parent(g, j, p->hole_j, &b);
            g->blend_vertex[g->blend_count] = (unsigned short)(j * vertices + i);
            g->blend_parent[g->blend_count] = (unsigned short)(b * (p->cells + 1) + a);
            g->blend_code[g->blend_count] = (unsigned char)(e | (fi << 2) | (fj << 5));
            ++g->blend_count;
        }
}

static void                 blend_edge(jo_water * const w, const int k)
{
    level_data              *g;
    const level_data        *p;
    const jo_color          *pc;
    int                     n;
    int                     v;
    int                     fi;
    int                     fj;
    int                     code;
    jo_color                c;
    jo_color                own;

    g = &w->levels[k];
    p = &w->levels[k + 1];
    prepare_blend(w, k);
    v = p->cells + 1;
    for (JO_ZERO(n); n < g->blend_count; ++n)
    {
        code = g->blend_code[n];
        fi = (code >> 2) & 7;
        fj = code >> 5;
        pc = p->colors + g->blend_parent[n];
        // The color of the next level there (between its vertices; a ratio of 2: means)
        c = pc[0];
        if (g->ratio == 2)
        {
            if (fi)
                c = mean_color(c, pc[1]);
            if (fj)
                c = mean_color(c, fi ? mean_color(pc[v], pc[v + 1]) : pc[v]);
        }
        else
        {
            if (fi)
                c = jo_color_mix(c, pc[1], (fi * g->inv_ratio) >> 8);
            if (fj)
                c = jo_color_mix(c, fi ? jo_color_mix(pc[v], pc[v + 1], (fi * g->inv_ratio) >> 8) : pc[v], (fj * g->inv_ratio) >> 8);
        }
        // (on the edge: its color; then half of it)
        own = g->colors[g->blend_vertex[n]];
        g->colors[g->blend_vertex[n]] = (code & 3) == 0 ? c : mean_color(c, own);
    }
}

/*
** Light of a vertex (gouraud: 16 = the color as it is, per channel)
**
** N: normal of the surface (from the slopes), V: toward the camera, S: toward the sun.
** - Fresnel: clear (shallow) where the camera looks into the water (N.V high), deep at a low angle, the sky reflected
** - the light of the sun on the slopes (N.S), the spot of the sun (round, around the point of the sea toward the sun), the
**   glints of the slopes toward the sun (no (R.S)^n: with a low sun, a narrow strip toward the horizon)
** - the caustics: moving light near the camera
** - the reflections of the clouds: R (V reflected by the surface) on a plane of clouds above (a map of white streaks): the
**   waves bend them, they slide with the camera (the white streaks of a sunny sea)
** - the foam: white, on the shadow side too
*/

static  __jo_force_inline int  soft_top(const int light)
{
    return (light > GOURAUD_KNEE ? GOURAUD_KNEE + ((light - GOURAUD_KNEE) >> 1) : light);
}

/** @brief The caustics of a level: one sine per column, row and diagonal */
static void                 prepare_caustics(jo_water * const w, level_data * const g)
{
    int                     t;
    int                     k;
    int                     a;

    // (slow and soft: wavelengths of 360, 480 and 610 units)
    t = w->frame;
    a = w->config.caustics;
    for (JO_ZERO(k); k <= g->cells; ++k)
    {
        g->caustics_x[k] = fast_sin((g->origin_x + k * g->cell + t * 2) * 182) * a / 3;
        g->caustics_z[k] = fast_sin((g->origin_z + k * g->cell - t * 2) * 137) * a / 3;
    }
    for (JO_ZERO(k); k <= 2 * g->cells; ++k)
        g->caustics_xz[k] = fast_sin((g->origin_x + g->origin_z + k * g->cell + t * 3) * 107) * a / 3;
}

/** @brief Gouraud of a vertex
 *  @param x Vertex (units)
 *  @param h Height (jo_fixed)
 *  @param z Vertex (units)
 *  @param nx Slope along x (jo_fixed: the normal is (-nx, 1, -nz))
 *  @param nz Slope along z
 *  @param light Caustics (steps, jo_fixed)
 *  @param foam 0 to 255
 *  @param depth 0 (the shore) to 255 (deep)
 *  @param reflect The reflections of the clouds (the levels near the camera)
 */
static jo_color             vertex_color(const jo_water * const w, const int x, const jo_fixed h, const int z, const jo_fixed nx,
                                         const jo_fixed nz, const jo_fixed light, int foam, const int depth, const bool reflect)
{
    const jo_water_config   *c;
    int                     dx;
    int                     dy;
    int                     dz;
    jo_fixed                inv;
    jo_fixed                inv_n;
    jo_fixed                vx;
    jo_fixed                vy;
    jo_fixed                vz;
    jo_fixed                nv;
    jo_fixed                ns;
    jo_fixed                glint;
    jo_fixed                spot;
    jo_fixed                ux;
    jo_fixed                uz;
    jo_fixed                near;
    jo_fixed                rx;
    jo_fixed                ry;
    jo_fixed                rz;
    int                     clouds;
    int                     shallow;
    int                     base;
    int                     sky;
    int                     white;
    int                     r;
    int                     g;
    int                     b;

    c = &w->config;
    // V: toward the camera (unit vector, approximate length)
    dx = w->cam_x - x;
    dy = w->cam_y - jo_fixed2int(h);
    dz = w->cam_z - z;
    inv = inverse_length(dx, dy, dz);
    vx = dx * inv;
    vy = dy * inv;
    vz = dz * inv;
    // N.V, N.S, N made a unit vector (steep slopes: the light stays in its range)
    inv_n = normal_inverses[JO_MIN(255, (unsigned int)(jo_fixed_mult(nx, nx) + jo_fixed_mult(nz, nz)) >> 10)];
    nv = jo_fixed_mult(vy - jo_fixed_mult(nx, vx) - jo_fixed_mult(nz, vz), inv_n);
    ns = jo_fixed_mult(w->sun_y - jo_fixed_mult(nx, w->sun_x) - jo_fixed_mult(nz, w->sun_z), inv_n);
    // Spot: (1 - (d / radius)^2)^2 around its center
    JO_ZERO(spot);
    ux = (x - w->spot_x) * w->spot_inv_radius;
    uz = (z - w->spot_z) * w->spot_inv_radius;
    if (JO_ABS(ux) < JO_FIXED_1 && JO_ABS(uz) < JO_FIXED_1)
    {
        spot = JO_FIXED_1 - jo_fixed_mult(ux, ux) - jo_fixed_mult(uz, uz);
        spot = spot > 0 ? jo_fixed_mult(spot, spot) : 0;
    }
    // Glints: the slopes toward the sun, more in the spot
    glint = -jo_fixed_mult(nx, w->sun_x) - jo_fixed_mult(nz, w->sun_z);
    glint = glint > 0 ? jo_fixed_mult(glint * c->glint, JO_FIXED_1 / 4 + spot) >> 16 : 0;
    // Fresnel (jo_fixed x small integer: (f * n) >> 16, no 64 bits product)
    near = JO_MAX(0, JO_MIN(JO_FIXED_1, (nv - NEAR_VY) * NEAR_SCALE));
    r = c->deep.r + ((near * (c->shallow.r - c->deep.r)) >> 16);
    g = c->deep.g + ((near * (c->shallow.g - c->deep.g)) >> 16);
    b = c->deep.b + ((near * (c->shallow.b - c->deep.b)) >> 16);
    // The slopes (limited: a high wave is steep), the caustics near the camera
    base = JO_MAX(c->slope_light_min, JO_MIN(c->slope_light_max, ((ns - w->sun_y) * SLOPE_LIGHT) >> 16)) +
           (((near >> 8) * (light >> 8)) >> 16);
    // The sky at a low angle: pale blue
    sky = ((JO_FIXED_1 - JO_MAX(0, JO_MIN(JO_FIXED_1, nv))) * c->sky) >> 16;
    // The clouds: R = 2 (N.V) N - V, on a plane above: R.x / R.y, R.z / R.y in the map of the clouds (and the position: they
    // slide under the camera); more at a low angle (Fresnel)
    JO_ZERO(clouds);
    if (reflect && c->reflection > 0)
    {
        ry = jo_fixed_mult(nv, inv_n) << 1;
        rx = -jo_fixed_mult(ry, nx) - vx;
        rz = -jo_fixed_mult(ry, nz) - vz;
        ry -= vy;
        ry = reflection_inverses[JO_MAX(0, JO_MIN(64, ry >> 10))];
        clouds = __jo_water_sky_reflection[((((rz * ry) >> 16) + (z >> REFLECTION_DRIFT)) & (SKY_REFLECTION_SIZE - 1)) * SKY_REFLECTION_SIZE +
                                ((((rx * ry) >> 16) + (x >> REFLECTION_DRIFT)) & (SKY_REFLECTION_SIZE - 1))];
        clouds = (clouds * c->reflection * (JO_FIXED_1 / 4 + JO_FIXED_1 - JO_MAX(0, JO_MIN(JO_FIXED_1, nv)))) >> 24;
    }
    white = JO_MIN(WHITE_MAX, ((spot * c->spot) >> 16) + glint + clouds);
    // The shallow water: lighter (the sand under it), the waves break on the shore (bands of foam that come in)
    if (depth < c->shallow_depth)
    {
        shallow = w->shallow_scale != 0 ? (int)(((unsigned int)(c->shallow_depth - depth) * w->shallow_scale) >> 16) :
                                          (c->shallow_depth - depth) * 256 / c->shallow_depth;
        r += (c->shore.r * shallow) >> 8;
        g += (c->shore.g * shallow) >> 8;
        b += (c->shore.b * shallow) >> 8;
        if (c->surf > 0 && depth < 64)
        {
            shallow = fast_sin(depth * 1024 + w->frame * 180) + JO_FIXED_1 / 3;
            if (shallow > 0)
                foam = JO_MAX(foam, (((shallow >> 8) * (64 - depth)) * c->surf) >> 14);
        }
    }
    // Foam: white on all the slopes (the shadow of a slope lighter)
    if (foam > 0)
    {
        white = JO_MIN(FOAM_WHITE_MAX, white + ((foam * c->foam_white) >> 8));
        if (base < 0)
            base = (base * (256 - foam)) >> 8;
    }
    r = soft_top(r + base + sky / 3 + white);
    g = soft_top(g + base + sky * 2 / 3 + white);
    b = soft_top(b + base + sky + white);
    // (no light above GOURAUD_MAX: the whites of the texture + the gouraud stay under 31)
    r = JO_MAX(2, JO_MIN(GOURAUD_MAX, r));
    g = JO_MAX(2, JO_MIN(GOURAUD_MAX, g));
    b = JO_MAX(2, JO_MIN(GOURAUD_MAX, b));
    return ((jo_color)(0x8000 | (b << 10) | (g << 5) | r));
}

/** @brief Light of the vertices of rows of a level (on either CPU: it only writes the colors of these rows)
 *  @param k The level (its caustics prepared)
 *  @param first First row
 *  @param last After the last row
 */
static void                 level_light(const int k, const int first, const int last)
{
    const jo_water          *w;
    level_data              *g;
    int                     vertices;
    int                     i;
    int                     j;
    int                     n;
    int                     inside;
    bool                    caustics;
    bool                    hole_row;
    bool                    foamy;
    foam_cell               *grid;
    bool                    shallow;
    jo_fixed                inv_2cells;
    jo_fixed                sx;
    jo_fixed                sz;
    int                     x;
    int                     z;
    int                     foam;
    int                     depth;

    w = current;
    g = (level_data *)&w->levels[k];
    vertices = g->cells + 1;
    inv_2cells = JO_FIXED_1 / (2 * g->cell);
    caustics = w->config.caustics > 0;
    foamy = w->foam_live && k < FOAM_LEVELS;
    grid = w->foam_grid;
    JO_ZERO(foam);
    depth = 255;
    // (the vertices deep in the hole: not used)
    inside = HOLE_MARGIN - 1;
    for (j = first; j < last; ++j)
    {
        // (the levels updated at each frame: half of their rows, the other half at the next frame; the light changes
        // slowly, the heights move at each frame. The foam of the other rows at each frame: a quad that takes the foam
        // texture from its vertices blinked, the foam of one of its rows a frame old)
        if (g->period == 1 && w->updates > 1 && ((j ^ w->updates) & 1))
        {
            if (foamy)
            {
                hole_row = g->hole_i >= 0 && j > g->hole_j + inside && j < g->hole_j + g->hole_cells - inside;
                z = g->origin_z + j * g->cell;
                for (i = g->span_first[j]; i < g->span_end[j]; ++i)
                {
                    if (hole_row && i > g->hole_i + inside && i < g->hole_i + g->hole_cells - inside)
                        continue;
                    x = g->origin_x + i * g->cell;
                    g->foam[j * vertices + i] = (unsigned char)foam_of(foam_cell_at(grid, x, z), w->frame, foam_region(x, z));
                }
            }
            continue;
        }
        // (the depth only in the rows with shallow water)
        shallow = w->depth_map != JO_NULL && g->span_first[j] < g->span_end[j] &&
                  shallow_row(w, g->origin_z + j * g->cell, g->origin_x + g->span_first[j] * g->cell,
                              g->origin_x + (g->span_end[j] - 1) * g->cell);
        depth = 255;
        hole_row = g->hole_i >= 0 && j > g->hole_j + inside && j < g->hole_j + g->hole_cells - inside;
        // (only the vertices in the field of view)
        for (i = g->span_first[j]; i < g->span_end[j]; ++i)
        {
            if (hole_row && i > g->hole_i + inside && i < g->hole_i + g->hole_cells - inside)
                continue;
            n = j * vertices + i;
            x = g->origin_x + i * g->cell;
            z = g->origin_z + j * g->cell;
            if (foamy)
                foam = foam_of(foam_cell_at(grid, x, z), w->frame, foam_region(x, z));
            if (shallow)
                depth = depth_at(w, x, z);
            // (the surf: the foam of the vertex, see vertex_color(); the texture only for the foam of a boat)
            g->foam[n] = (unsigned char)foam;
            g->depth[n] = (unsigned char)depth;
            // Slopes: height differences over 2 cells (1 on the edge), by a product (a division is a call on the SH-2)
            sx = g->heights[j * vertices + JO_MIN(i + 1, g->cells)] - g->heights[j * vertices + JO_MAX(i - 1, 0)];
            sz = g->heights[JO_MIN(j + 1, g->cells) * vertices + i] - g->heights[JO_MAX(j - 1, 0) * vertices + i];
            g->colors[n] = vertex_color(w, x, g->heights[n], z, jo_fixed_mult(sx, inv_2cells), jo_fixed_mult(sz, inv_2cells),
                                        caustics ? g->caustics_x[i] + g->caustics_z[j] + g->caustics_xz[i + j] : 0, foam, depth,
                                        k < REFLECTION_LEVELS);
        }
    }
}

/** @brief The edge of a level, its gouraud tables (to the VRAM by the DMA) and the texture of its quads (the master CPU,
 *  after the light of the vertices of all the levels)
 *  @param k The level (the next one is done: the edge takes its colors)
 */
static void                 level_tables(jo_water * const w, const int k)
{
    level_data              *g;
    int                     vertices;
    int                     i;
    int                     j;
    int                     n;
    int                     q;
    int                     f;
    int                     d;
    int                     tiles;
    int                     shift;
    int                     ti;
    int                     tj;
    int                     bottom_tiles;
    int                     bottom_depth;
    int                     bottom_far;
    int                     see_through;
    int                     x0;
    int                     z0;
    const int               *ids;
    const int               *bottom_ids;
    const unsigned char     *foam;
    const unsigned char     *depth;
    jo_3d_polygon_attribute *a;
    unsigned int            *table;
    const unsigned short    *v;

    g = &w->levels[k];
    vertices = g->cells + 1;
    if (k < w->level_count - 1)
        blend_edge(w, k);
    for (JO_ZERO(n); n < vertices * vertices; ++n)
        g->points[n].y = -g->heights[n];
    set_quad_order(g, JO_ABS(w->forward_z) > JO_ABS(w->forward_x));
    // (the other set: the VDP1 may draw the previous frame with the set of the last update)
    g->set ^= 1;
    g->mesh->data.attributes = g->attributes[g->set];
    table = g->tables;
    // The foam texture: the quads with foam (the most of their vertices) take the tile of the pattern at their place and the
    // color lookup table of their foam; the others the color (the quads that had foam at the last update of this set too)
    foam = g->foam;
    // (the place of the quad in the pattern: the cell of its corner (i, j) in the world, modulo the tiles)
    // (the tiles of the size of the quads: quads of 8 units, the set 0, of 16, the set 1)
    for (shift = 3; (8 << (shift - 3)) < g->cell; ++shift)
        ;
    ti = g->origin_x >> shift;
    tj = g->origin_z >> shift;
    if (g->cell == (8 << (shift - 3)) && shift - 3 < FOAM_TEXTURE_LEVELS && w->config.textured && w->foam_live)
    {
        tiles = FOAM_TILES >> (shift - 3);
        ids = w->foam_tiles[shift - 3][g->along_z ? 1 : 0];
    }
    else
    {
        foam = JO_NULL;
        JO_ZERO(tiles);
        ids = JO_NULL;
    }
    // The bottom seen in the shallow water: the quads without foam, all their vertices less deep than bottom_depth (only in
    // the rows with shallow water: the others are deep), take the tile of the bottom at their place and the color lookup
    // table of their depth
    depth = g->depth;
    bottom_depth = w->config.bottom_depth;
    see_through = w->depth_map != JO_NULL ? w->config.see_through : 0;
    if (k < BOTTOM_LEVELS && g->cell == 8 && w->config.textured && w->config.bottom > 0 && w->depth_map != JO_NULL &&
        w->bottom_tiles[0][0][0] > 0)
    {
        bottom_tiles = BOTTOM_TILES >> k;
        bottom_ids = w->bottom_tiles[k][g->along_z ? 1 : 0];
        // (the distance of a quad in front of the camera: its corner (i, j), x / 8 units: the products stay small)
        bottom_far = (BOTTOM_FAR + g->cell) * (65536 / 8);
        x0 = (g->origin_x - w->cam_x) / 8;
        z0 = (g->origin_z - w->cam_z) / 8;
    }
    else
    {
        depth = JO_NULL;
        JO_ZERO(bottom_tiles);
        JO_ZERO(bottom_far);
        JO_ZERO(x0);
        JO_ZERO(z0);
        bottom_ids = JO_NULL;
    }
    for (JO_ZERO(q); q < (int)g->mesh->data.polygon_count; ++q)
    {
        v = g->mesh->data.polygons[q].vertices;
        // (the colors of the vertices A, B, C, D)
        table[q * 2] = ((unsigned int)g->colors[v[0]] << 16) | g->colors[v[1]];
        table[q * 2 + 1] = ((unsigned int)g->colors[v[2]] << 16) | g->colors[v[3]];
        a = &g->mesh->data.attributes[q];
        // (the foam of the quad: between the mean and the most of its vertices, a softer edge than the most)
        if (foam == JO_NULL)
            JO_ZERO(f);
        else
        {
            f = JO_MAX(JO_MAX(foam[v[0]], foam[v[1]]), JO_MAX(foam[v[2]], foam[v[3]]));
            f = (f * 4 + foam[v[0]] + foam[v[1]] + foam[v[2]] + foam[v[3]]) >> 3;
        }
        if (f >= FOAM_LUT_MIN)
        {
            i = g->quad_i[q];
            j = g->quad_j[q];
            *a = w->foamy;
            a->texture_id = (unsigned short)ids[((tj + j) & (tiles - 1)) * tiles + ((ti + i) & (tiles - 1))];
            a->color = w->foam_luts[JO_MIN(FOAM_LUTS - 1, (f - FOAM_LUT_MIN) * FOAM_LUTS / (256 - FOAM_LUT_MIN))];
            a->gouraud_table = (unsigned short)(g->gouraud[g->set] + q);
        }
        else if (depth != JO_NULL &&
                 (d = JO_MAX(JO_MAX(depth[v[0]], depth[v[1]]), JO_MAX(depth[v[2]], depth[v[3]]))) < bottom_depth &&
                 (x0 + g->quad_i[q] * (g->cell / 8)) * w->forward_x + (z0 + g->quad_j[q] * (g->cell / 8)) * w->forward_z < bottom_far)
        {
            i = g->quad_i[q];
            j = g->quad_j[q];
            *a = w->bottomy;
            a->texture_id = (unsigned short)bottom_ids[((tj + j) & (bottom_tiles - 1)) * bottom_tiles + ((ti + i) & (bottom_tiles - 1))];
            // (the table of the mean depth of the quad; less contrast near the edges of the level: no line between the
            // texture and the quads of the next level)
            d = (d + depth[v[0]] + depth[v[1]] + depth[v[2]] + depth[v[3]]) / 5;
            d = JO_MIN(BOTTOM_LUTS - 1, d * BOTTOM_LUTS / bottom_depth);
            n = JO_MIN(JO_MIN(i, j), JO_MIN(g->cells - 1 - i, g->cells - 1 - j));
            d = JO_MAX(d, BOTTOM_LUTS - 1 - 2 * n);
            a->color = w->bottom_luts[d];
            a->gouraud_table = (unsigned short)(g->gouraud[g->set] + q);
        }
        else if (a->texture_id != w->plain.texture_id)
        {
            *a = w->plain;
            a->gouraud_table = (unsigned short)(g->gouraud[g->set] + q);
        }
        // The shallow water half transparent (see config.see_through): the VDP1 mixes it with the RGB pixels drawn behind
        // it (the island under the water), not with the others (the frame buffer cleared, the palette pixels)
        if (see_through > 0)
        {
            d = JO_MIN(JO_MIN(g->depth[v[0]], g->depth[v[1]]), JO_MIN(g->depth[v[2]], g->depth[v[3]]));
            a->draw_mode = (unsigned short)((a->draw_mode & ~JO_VDP1_COLOR_CALC_MASK) |
                                            (d < see_through ? JO_VDP1_COLOR_CALC_GOURAUD | JO_VDP1_COLOR_CALC_HALF_TRANSPARENT :
                                                               JO_VDP1_COLOR_CALC_GOURAUD));
        }
    }
    // (the end of the previous copy first)
    jo_dma_wait();
    jo_dma_copy_async(g->tables, (void *)(JO_VDP1_VRAM + g->gouraud[g->set] * 8), g->mesh->data.polygon_count * 8);
}

/** @brief Light of the skirt: the edge of the last level, the calm sea outside (with the last level) */
static void                 skirt_light(jo_water * const w)
{
    jo_color                colors[8 * JO_WATER_MAX_CELLS];
    const level_data        *last;
    volatile unsigned int   *table;
    int                     quads;
    int                     k;
    int                     a;
    int                     b;

    last = &w->levels[w->level_count - 1];
    quads = 4 * w->skirt_segments;
    for (JO_ZERO(k); k < quads; ++k)
    {
        a = ring_vertex(w, k);
        w->skirt_points[k].y = last->points[a].y;
        colors[k] = last->colors[a];
    }
    for (k = quads; k < 2 * quads; ++k)
        colors[k] = vertex_color(w, last->origin_x + jo_fixed2int(w->skirt_points[k].x), 0,
                                 last->origin_z + jo_fixed2int(w->skirt_points[k].z), 0, 0, 0, 0,
                                 w->depth_map != JO_NULL ? depth_at(w, last->origin_x + jo_fixed2int(w->skirt_points[k].x),
                                                                    last->origin_z + jo_fixed2int(w->skirt_points[k].z)) : 255,
                                 false);
    w->skirt_set ^= 1;
    table = (volatile unsigned int *)(JO_VDP1_VRAM + w->skirt_gouraud[w->skirt_set] * 8);
    for (JO_ZERO(k); k < quads; ++k)
    {
        a = k;
        b = (k + 1) % quads;
        table[k * 2] = ((unsigned int)colors[a] << 16) | colors[b];
        table[k * 2 + 1] = ((unsigned int)colors[quads + b] << 16) | colors[quads + a];
        if (w->config.grain && w->grain >= 0)
            jo_3d_set_mesh_polygon_texture(w->skirt, w->grain, (unsigned int)k);
        else
            jo_3d_set_mesh_polygon_color(w->skirt, w->config.color, (unsigned int)k);
        w->skirt->data.attributes[k].draw_mode |= JO_VDP1_COLOR_CALC_GOURAUD;
        w->skirt->data.attributes[k].gouraud_table = (unsigned short)(w->skirt_gouraud[w->skirt_set] + k);
    }
}

/*
** The frame
*/

/** @brief Run a function on the rows from first to last (excluded) of the levels updated in this frame (one after the
 *  other) */
static void                 run_rows(const jo_water * const w, const int first, const int last,
                                     void (*function)(const int, const int, const int))
{
    int                     k;
    int                     base;
    int                     rows;
    int                     a;
    int                     b;

    JO_ZERO(base);
    for (JO_ZERO(k); k < w->level_count && base < last; ++k)
    {
        if (!w->levels[k].update)
            continue;
        rows = w->levels[k].cells + 1;
        a = JO_MAX(first - base, 0);
        b = JO_MIN(last - base, rows);
        if (a < b)
            function(k, a, b);
        base += rows;
    }
}

/** @brief The rows of the slave CPU in this frame */
static void                 split_rows(const jo_water * const w, cpu_split * const s)
{
    int                     k;

    JO_ZERO(s->total);
    for (JO_ZERO(k); k < w->level_count; ++k)
        if (w->levels[k].update)
            s->total += w->levels[k].cells + 1;
    s->rows = (s->total * s->share) >> 16;
}

/** @brief Move the share of the slave CPU toward the balance (after jo_core_wait_for_slave()) */
static void                 balance(cpu_split * const s)
{
    int                     wait;

    // (microseconds the master CPU waited: more than 300, the slave CPU has too much; less than 100, it can take more)
    wait = jo_core_master_wait_time();
    if (wait > 300)
        s->share = JO_MAX(0, s->share - JO_FIXED_1 / 128);
    else if (wait < 100)
        s->share = JO_MIN(JO_FIXED_1, s->share + JO_FIXED_1 / 128);
}

/** @brief The jobs of the slave CPU: the first rows */
static void                 slave_heights(void)
{
    run_rows(current, 0, current->height_split.rows, level_heights);
}

static void                 slave_light(void)
{
    run_rows(current, 0, current->light_split.rows, level_light);
}

/** @brief The vertices of each row of a level in the field of view (the test of level_ranges() with a margin of one more
 *  quad, and one more vertex on each side: the slopes of the light), see span_first */
static void                 level_spans(const jo_water * const w, level_data * const g)
{
    int                     i;
    int                     j;
    int                     first;
    int                     last;
    int                     dx;
    int                     dz;
    int                     margin;
    jo_fixed                along;
    jo_fixed                side;
    jo_fixed                ax;
    jo_fixed                sx;

    margin = 3 * g->cell + JO_ABS(w->cam_y);
    ax = g->cell * w->forward_x;
    sx = g->cell * w->forward_z;
    for (JO_ZERO(j); j <= g->cells; ++j)
    {
        dx = g->origin_x - w->cam_x;
        dz = g->origin_z + j * g->cell - w->cam_z;
        along = dx * w->forward_x + dz * w->forward_z;
        side = dx * w->forward_z - dz * w->forward_x;
        first = -1;
        last = -1;
        for (JO_ZERO(i); i <= g->cells; ++i, along += ax, side += sx)
            if (jo_fixed2int(along) > -margin && JO_ABS(jo_fixed2int(side)) < jo_fixed2int(along) * w->view_tan / 16 + margin)
            {
                if (first < 0)
                    first = i;
                last = i;
            }
        if (first < 0)
        {
            JO_ZERO(g->span_first[j]);
            JO_ZERO(g->span_end[j]);
            continue;
        }
        g->span_first[j] = (unsigned char)JO_MAX(0, first - 1);
        g->span_end[j] = (unsigned char)JO_MIN(g->cells + 1, last + 2);
    }
}

/** @brief Place the levels around a point in front of the camera; the levels updated in this frame */
static void                 place_levels(jo_water * const w)
{
    level_data              *g;
    int                     k;
    int                     cx;
    int                     cz;
    int                     x;
    int                     z;
    int                     step;
    bool                    moved;
    bool                    inner_moved;

    cx = w->cam_x + jo_fixed2int(w->forward_x * w->config.ahead);
    cz = w->cam_z + jo_fixed2int(w->forward_z * w->config.ahead);
    inner_moved = false;
    for (JO_ZERO(k); k < w->level_count; ++k)
    {
        g = &w->levels[k];
        // (by whole cells of the next level: the vertices on its lines)
        step = k < w->level_count - 1 ? w->levels[k + 1].cell : 2 * g->cell;
        x = snap(cx - g->cells * g->cell / 2, step);
        z = snap(cz - g->cells * g->cell / 2, step);
        // Updated in this frame: its turn, it moved, the level inside it moved (its hole: new quads), the first frame
        // (the turns of the levels at the same frames: a level updated, the levels inside it too, their edges stay on it)
        moved = x != g->origin_x || z != g->origin_z;
        g->update = w->updates == 0 || (w->updates & (g->period - 1)) == 0 || moved || inner_moved;
        inner_moved = moved;
        g->origin_x = x;
        g->origin_z = z;
        if (k > 0)
        {
            g->hole_i = (w->levels[k - 1].origin_x - x) / g->cell;
            g->hole_j = (w->levels[k - 1].origin_z - z) / g->cell;
        }
        w->bound_x[k] = x;
        w->bound_z[k] = z;
        w->bound_side[k] = g->cells * g->cell;
        level_spans(w, g);
    }
}

void                        jo_water_update(jo_water * const w, const int frame, const int x, const int y, const int z, const jo_angle heading)
{
    int                     k;

    current = w;
    w->frame = frame;
    // (no stamp for longer than the longest foam: no foam left)
    if (w->foam_live && frame - w->foam_frame > 1100)
        w->foam_live = false;
    w->cam_x = x;
    w->cam_y = -y;
    // (the field of view of the camera: the half width of the screen / the projection distance)
    w->view_tan = jo_fixed2int(jo_fixed_div(jo_int2fixed(jo_core_get_screen_width() * 16 * 11 / 20), jo_3d_get_projection_distance())) + 1;
    w->cam_z = z;
    w->forward_x = jo_sin_angle(heading);
    w->forward_z = jo_cos_angle(heading);
    w->spot_x = x + jo_fixed2int(w->sun_hx * w->spot_distance);
    w->spot_z = z + jo_fixed2int(w->sun_hz * w->spot_distance);
    w->shallow_scale = w->config.shallow_depth > 0 && w->config.shallow_depth <= 255 ?
                       (0x1000000u + (unsigned int)w->config.shallow_depth - 1) / (unsigned int)w->config.shallow_depth : 0;
    place_levels(w);
    find_live_events(w);
    // Heights: the first rows on the slave CPU, the others on the master CPU; then the edges on the next levels
    jo_profile_begin(JO_PROFILE_USER(0));
    split_rows(w, &w->height_split);
    jo_core_exec_on_slave(slave_heights);
    run_rows(w, w->height_split.rows, w->height_split.total, level_heights);
    jo_core_wait_for_slave();
    balance(&w->height_split);
    for (k = w->level_count - 2; k >= 0; --k)
        if (w->levels[k].update)
            stitch_level(w, k);
    jo_profile_end(JO_PROFILE_USER(0));
    // Light: the same (the edges and the gouraud tables: jo_water_draw())
    jo_profile_begin(JO_PROFILE_USER(1));
    if (w->config.caustics > 0)
        for (JO_ZERO(k); k < w->level_count; ++k)
            if (w->levels[k].update)
                prepare_caustics(w, &w->levels[k]);
    split_rows(w, &w->light_split);
    jo_core_exec_on_slave(slave_light);
    run_rows(w, w->light_split.rows, w->light_split.total, level_light);
    jo_core_wait_for_slave();
    balance(&w->light_split);
    ++w->updates;
    jo_profile_end(JO_PROFILE_USER(1));
}

/** @brief The quads of a level in the field of view, without its hole (the engine transforms only the points of the
 *  quads drawn: the quads behind the camera and on the sides cost nothing)
 *  @return Number of ranges
 */
static int                  level_ranges(const jo_water * const w, const level_data * const g, jo_3d_polygon_range * const ranges)
{
    int                     count;
    int                     i;
    int                     j;
    int                     first;
    int                     dx;
    int                     dz;
    int                     along;
    int                     side;
    int                     margin;
    jo_fixed                ax;
    jo_fixed                sx;

    JO_ZERO(count);
    // (the center of a quad: in the view if along > -margin and |side| < along x view_tan + margin; along a row, x goes
    // up by a cell: the products go up by the same steps)
    // (looking down from high up, the bottom of the screen shows the sea under the camera: a margin as big as the height)
    margin = 2 * g->cell + JO_ABS(w->cam_y);
    ax = g->cell * w->forward_x;
    sx = g->cell * w->forward_z;
    for (JO_ZERO(j); j < g->cells; ++j)
    {
        dx = g->origin_x + g->cell / 2 - w->cam_x;
        dz = g->origin_z + j * g->cell + g->cell / 2 - w->cam_z;
        along = dx * w->forward_x + dz * w->forward_z;
        side = dx * w->forward_z - dz * w->forward_x;
        first = -1;
        for (JO_ZERO(i); i <= g->cells; ++i, along += ax, side += sx)
        {
            if (i < g->cells && !(g->hole_i >= 0 && j >= g->hole_j && j < g->hole_j + g->hole_cells && i >= g->hole_i &&
                                  i < g->hole_i + g->hole_cells) &&
                jo_fixed2int(along) > -margin && JO_ABS(jo_fixed2int(side)) < jo_fixed2int(along) * w->view_tan / 16 + margin)
            {
                if (first < 0)
                    first = i;
                continue;
            }
            if (first >= 0)
                jo_3d_add_polygon_range(ranges, &count, j * g->cells + first, i - first);
            first = -1;
        }
    }
    return (count);
}

/** @brief Draw a level on the master CPU */
static void                 level_draw(const level_data * const g)
{
    jo_3d_polygon_range     ranges[3 * JO_WATER_MAX_CELLS];
    int                     count;

    count = level_ranges(current, g, ranges);
    jo_3d_push_matrix();
    {
        jo_3d_translate_matrix(g->origin_x, 0, g->origin_z);
        jo_3d_mesh_draw_ranges(g->mesh, g->mesh->data.point_count, ranges, count);
    }
    jo_3d_pop_matrix();
}

/** @brief The color lookup tables of the foam texture: the color of the water, the white of the foam above a threshold
 *  (more foam: a lower threshold, more white pixels), a soft step of 2 values; each value moves a bit around its
 *  threshold with the time (the bubbles come and go). A quarter of the tables at each update (each one every 4 updates: the
 *  bubbles come and go as well, 0.3 ms of the master CPU less)
 */
static void                 animate_foam(jo_water * const w)
{
    static const short      coverage[FOAM_LUTS] = {14, 30, 52, 80, 112, 150, 192, 232};  // White pixels (/ 256) of each table
    volatile unsigned short *lut;
    jo_color                water_color;
    jo_color                foam_color;
    int                     k;
    int                     v;
    int                     threshold;
    int                     d;
    int                     first;
    int                     last;

    water_color = w->config.color;
    foam_color = w->config.foam_color;
    // (the first time all of them: the tables not made yet were the zeros of the block, transparent, a hole in the water
    // where a boat first made foam: the black screen or the sprites behind it, under the boat at the start)
    first = w->foam_filled ? (w->updates & 3) * (FOAM_LUTS / 4) : 0;
    last = w->foam_filled ? ((w->updates & 3) + 1) * (FOAM_LUTS / 4) : FOAM_LUTS;
    w->foam_filled = true;
    for (k = first; k < last; ++k)
    {
        lut = (volatile unsigned short *)(JO_VDP1_VRAM + w->foam_luts[k] * 8);
        // (the threshold in 1/256 of a value: the values 1 to 15, coverage x 15 of them white)
        threshold = 16 * 256 - coverage[k] * 15;
        lut[0] = water_color;
        for (v = 1; v < 16; ++v)
        {
            d = v * 256 - threshold + (fast_sin(w->frame * 300 + v * 9973 + k * 20000) >> 9);
            if (d >= 256)
                lut[v] = foam_color;
            else if (d <= -256)
                lut[v] = water_color;
            else
                lut[v] = jo_color_mix(water_color, foam_color, (d + 256) >> 1);
        }
    }
}

void                        __jo_water_camera(const jo_water * const w, int * const x, int * const y, int * const z, int * const view_tan)
{
    *x = w->cam_x;
    *y = w->cam_y;
    *z = w->cam_z;
    *view_tan = w->view_tan;
}

/** @brief The objects drawn by the slave CPU after the water (see jo_water_draw_with()) */
static void                 (*slave_objects)(void);

/** @brief The job of the slave CPU: the edges and the gouraud tables of the first levels (the most quads), then draw them */
static void                 slave_draw(void)
{
    jo_3d_polygon_range     ranges[3 * JO_WATER_MAX_CELLS];
    const level_data        *g;
    int                     count;
    int                     k;

    for (k = current->slave_draw_levels - 1; k >= 0; --k)
        if (current->levels[k].update)
            level_tables(current, k);
    jo_dma_wait();
    for (JO_ZERO(k); k < current->slave_draw_levels; ++k)
    {
        g = &current->levels[k];
        count = level_ranges(current, g, ranges);
        jo_3d_slave_push_matrix();
        jo_3d_slave_translate_matrix_fixed(jo_int2fixed(g->origin_x), 0, jo_int2fixed(g->origin_z));
        jo_3d_slave_mesh_draw_ranges(g->mesh, g->mesh->data.point_count, ranges, count);
        jo_3d_slave_pop_matrix();
    }
    // (the objects under the water: after the water, see water_objects.c)
    if (__jo_water_objects_frame != JO_NULL)
        __jo_water_objects_frame(current, __JO_WATER_OBJECTS_DRAW_ON_SLAVE);
    if (slave_objects != JO_NULL)
        slave_objects();
}

void                        jo_water_draw(jo_water * const w)
{
    jo_water_draw_with(w, JO_NULL, JO_NULL);
}

void                        jo_water_draw_with(jo_water * const w, void (*objects)(void), void (*objects_on_slave)(void))
{
    int                     k;

    current = w;
    jo_profile_begin(JO_PROFILE_USER(2));
    if (w->config.textured && w->foam_live)
        animate_foam(w);
    if (w->config.textured && w->config.bottom > 0 && w->bottom_tiles[0][0][0] > 0)
        animate_bottom(w);
    // The edges and the gouraud tables (from the outside: an edge takes the colors of the next level): the last levels
    // and the skirt on the master CPU, then the first levels on the slave CPU (it draws them) while the master CPU draws
    // the others
    for (k = w->level_count - 1; k >= w->slave_draw_levels; --k)
        if (w->levels[k].update)
        {
            level_tables(w, k);
            if (k == w->level_count - 1)
                skirt_light(w);
        }
    slave_objects = objects_on_slave;
    // (the view of the objects under the water: the matrix of the camera)
    if (__jo_water_objects_frame != JO_NULL)
        __jo_water_objects_frame(w, __JO_WATER_OBJECTS_VIEW);
    jo_3d_slave_begin();
    jo_core_exec_on_slave(slave_draw);
    jo_dma_wait();
    jo_3d_push_matrix();
    {
        jo_3d_translate_matrix(w->levels[w->level_count - 1].origin_x, 0, w->levels[w->level_count - 1].origin_z);
        jo_3d_mesh_draw(w->skirt);
    }
    jo_3d_pop_matrix();
    for (k = w->level_count - 1; k >= w->slave_draw_levels; --k)
        level_draw(&w->levels[k]);
    // (the master CPU draws the objects while the slave CPU draws the first levels)
    if (objects != JO_NULL)
    {
        jo_profile_end(JO_PROFILE_USER(2));
        objects();
        jo_profile_begin(JO_PROFILE_USER(2));
    }
    jo_core_wait_for_slave();
    jo_3d_slave_end();
    // (the objects above the water: given again in the next frame)
    if (__jo_water_objects_frame != JO_NULL)
        __jo_water_objects_frame(w, __JO_WATER_OBJECTS_END_FRAME);
    jo_profile_end(JO_PROFILE_USER(2));
}

#endif /* JO_COMPILE_WITH_3D_SUPPORT && JO_COMPILE_WITH_DUAL_CPU_SUPPORT */

/*
** END OF FILE
*/
