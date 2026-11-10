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

/*
** INCLUDES
*/
#include <stdbool.h>
#include "jo/conf.h"
#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/core.h"
#include "jo/tools.h"
#include "jo/malloc.h"
#include "jo/colors.h"
#include "jo/vdp1_command_pipeline.h"
#include "jo/sprites.h"
#include "jo/fs.h"
#include "jo/math.h"
#include "jo/list.h"
#include "jo/3d.h"
#include "jo/profile.h"
#include "jo/dsp.h"
#include "dsp_xform.h"
#include "internal/core.h"
#include "internal/vdp1_command_pipeline.h"
#include "internal/sprites.h"
#include "internal/3d.h"
#include "internal/profile.h"

#ifdef JO_COMPILE_WITH_3D_SUPPORT

extern jo_picture_definition        __jo_sprite_pic[JO_MAX_SPRITE];
extern jo_texture_definition        __jo_sprite_def[];
extern bool                         __jo_sprite_end_codes[];
extern unsigned short               __jo_sprite_mean_color[];
extern jo_sprite_attributes         __jo_sprite_attributes;
jo_3d_quad                          *__jo_sprite_quad[JO_MAX_SPRITE];

/*
** Coordinate system: X to the right, Y down, Z into the screen.
** A matrix stores the image of the X, Y and Z axes (m[0], m[1], m[2]) and the
** translation (m[3]): out = x * m[0] + y * m[1] + z * m[2] + m[3].
*/

/*
** INTERNAL MACROS
*/
# define JO_3D_MATRIX_STACK_DEPTH           (20)
# define JO_3D_MAX_VERTICES                 (2048)
# define JO_3D_CURRENT_MATRIX               (__jo_3d_matrix_stack[__jo_3d_matrix_level])
/** @brief Lighting: neutral brightness level (gouraud table index) */
# define JO_3D_LIGHT_NEUTRAL_LEVEL          (16)
/** @brief Lighting: maximum brightness level */
# define JO_3D_LIGHT_MAX_LEVEL              (31)
/** @brief RGB component mask (5 bits) */
# define JO_3D_RGB_COMPONENT_MASK           (0x1f)
/** @brief Scale tolerance to consider that a matrix has a uniform scale (squared, Q16.16) */
# define JO_3D_UNIFORM_SCALE_TOLERANCE      (0x100)
/** @brief Clipping: margin around the screen used to clip the polygons that are clipped anyway (pixels). The VDP1 also spends time on
 *  the pixels outside of the screen: a clipped polygon is cut at the edges of the screen */
# define JO_3D_CLIP_MARGIN                  (2)
/** @brief Clipping: a vertex is "very far" outside of the screen when |x| or |y| > z * 2^shift (view space) */
# define JO_3D_FAR_OUTSIDE_SHIFT            (7)
/** @brief Screen position (pixels from the left or the top) beyond which a vertex is very far outside of the screen: its view
 *  space position is kept (the assembly loop checks the projected position, see __jo_3d_transform_loop()) */
# define JO_3D_FAR_LIMIT                    (16384)
/** @brief Clipping: maximum vertices of a clipped polygon (4 + 1 per plane) */
/** @brief Distortion of a texture (pixels, see jo_3d_subdivision_level()): more, the polygon is drawn in 2 x 2 parts
 *  (4 x 4 parts above 4 times more) */
# define JO_3D_SUBDIVISION_ERROR            (8)
# define JO_3D_CLIP_MAX_VERTICES            (12)
/** @brief A textured polygon seen as a thin line (the ground at a grazing angle: its 4 corners nearly on a line of the screen)
 *  is drawn as a polygon of the mean color of its texture (known for the textures with smaller copies, see
 *  jo_3d_set_texture_mipmaps()), as thin as it (see jo_3d_emit_sliver()). The VDP1 drew its lines from the edge A-D to the
 *  edge B-C: for such a polygon both edges lie along the line, each of the lines went along the whole line (about its
 *  length squared pixels: 22 000 pixels for 400 seen on a hilly ground, 10 to 20 % of the pixels of the frame; measured
 *  in Mednafen: 0.4 ms of VDP1 time per frame, 0.8 ms at the 95th percentile, for 0.5 ms of each CPU, the test of
 *  the assembly loop). The extent of the polygon on the screen (pixels, along the diagonals) and the width under which it
 *  is a line (pixels: twice its area / the extent, the mean width of the strip; wider, flat bands were seen on the hills).
 *  Test: JO_TEST_NO_SLIVER draws them as they are (the test of the assembly loop stays) */
# define JO_3D_SLIVER_EXTENT                (64)
# define JO_3D_SLIVER_WIDTH                 (4)
/** @brief A part of a subdivided polygon partly out of the screen that is at least this big on the screen (pixels) is cut
 *  along the lines of its tile where it leaves the screen (see jo_3d_crop_part()): the VDP1 spends its time on the pixels
 *  out of the screen too (the parts of a road under the bottom of the screen were a fourth of the pixels of a frame;
 *  Mednafen doesn't count them). The margin (pixels) kept out of the screen. Test: -DJO_3D_CROP_EXTENT=9999
 *  for no cut, to compare on the console */
# ifndef JO_3D_CROP_EXTENT
#  define JO_3D_CROP_EXTENT                 (96)
# endif
# define JO_3D_CROP_MARGIN                  (4)
/** @brief Clipping: sets of gouraud tables of the clipped polygons, used by the frames in turn. The VDP1 draws the previous frame
 *  (and with the dual CPU support, the slave CPU copies the previous frame while the VDP1 draws the frame before it) */
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
# define JO_3D_CLIP_GOURAUD_SETS            (3)
#else
# define JO_3D_CLIP_GOURAUD_SETS            (2)
#endif
/** @brief Clipping: gouraud tables of the parts of the clipped polygons, per frame */
# define JO_3D_CLIP_GOURAUD_TABLES          (JO_VDP1_CLIP_GOURAUD_TABLE_COUNT / JO_3D_CLIP_GOURAUD_SETS)
/** @brief All the commands available in the frame */
# define JO_3D_RESERVE_ALL_COMMANDS         (0xFFFF)
/** @brief The center of the projection of a window in its local coordinates (see jo_3d_window_clip()) */
# define JO_3D_WINDOW_CX(W)                 ((W)->center_x - (W)->origin_x)
# define JO_3D_WINDOW_CY(W)                 ((W)->center_y - (W)->origin_y)
/** @brief Outcode of a vertex: left of the screen */
# define JO_3D_CODE_LEFT                    (1 << 0)
/** @brief Outcode of a vertex: right of the screen */
# define JO_3D_CODE_RIGHT                   (1 << 1)
/** @brief Outcode of a vertex: above the screen */
# define JO_3D_CODE_TOP                     (1 << 2)
/** @brief Outcode of a vertex: below the screen */
# define JO_3D_CODE_BOTTOM                  (1 << 3)
/** @brief Outcode of a vertex: outside of the screen */
# define JO_3D_CODE_SCREEN                  (JO_3D_CODE_LEFT | JO_3D_CODE_RIGHT | JO_3D_CODE_TOP | JO_3D_CODE_BOTTOM)
/** @brief Outcode of a vertex: outside of the guard band (the polygon is clipped) */
# define JO_3D_CODE_GUARD                   (1 << 4)
/** @brief Outcode of a vertex: behind the near plane (the polygon is clipped) */
# define JO_3D_CODE_NEAR                    (1 << 5)
/** @brief Outcode of a vertex: x and y are the view space position (not projected) */
# define JO_3D_CODE_VIEW                    (1 << 6)

/** @brief 32 bits word of a VDP1 command (may alias a jo_vdp1_command) */
typedef unsigned int                        __attribute__((may_alias)) jo_3d_command_word;

/** @brief Matrix: 3 axes and the translation (internal usage) */
typedef jo_fixed                            jo_3d_matrix[4][3];

/** @brief Outcode bits of a transformed vertex */
# define JO_3D_CODE_MASK                    (0x7F)

/** @brief Copy a matrix (12 words: without the checks and the call of jo_memcpy(), a matrix is pushed for each object)
 *  @param dst Destination
 *  @param src Source
 */
static  __jo_force_inline void              jo_3d_copy_matrix(jo_fixed (* const dst)[3], const jo_fixed (* const src)[3])
{
    register int                            i;

    for (JO_ZERO(i); i < 4; ++i)
    {
        dst[i][0] = src[i][0];
        dst[i][1] = src[i][1];
        dst[i][2] = src[i][2];
    }
}

/** @brief Transformed vertex (internal usage): 8 bytes, 2 per cache line. The screen position is packed as in a VDP1
 *  command, the outcode is in the low bits of the depth (the depth loses 1/512 unit). The view space position of a vertex
 *  behind the near plane or very far outside of the screen (JO_3D_CODE_VIEW) is in the parallel array of the CPU
 *  (jo_3d_cpu_context.view, same index), or given by the caller for the vertices built by the subdivision */
typedef struct
{
    int                                     xy;     // Screen position: x in the high 16 bits, y in the low 16 bits
    jo_fixed                                zc;     // View space depth (bits 31 to 7) and outcode JO_3D_CODE_xxx (bits 6 to 0)
}                                           jo_3d_vertex;

/** @brief View space position of a vertex behind the near plane or very far outside of the screen (internal usage) */
typedef struct
{
    jo_fixed                                x;
    jo_fixed                                y;
}                                           jo_3d_view_position;

/** @brief Screen position of a transformed vertex */
# define JO_3D_VERTEX_X(V)                  ((V)->xy >> 16)
# define JO_3D_VERTEX_Y(V)                  ((int)(short)(V)->xy)
/** @brief Depth of a transformed vertex (its outcode removed) */
# define JO_3D_VERTEX_Z(V)                  ((V)->zc & ~JO_3D_CODE_MASK)
/** @brief Outcode of a transformed vertex */
# define JO_3D_VERTEX_CODE(V)               ((unsigned int)(V)->zc & JO_3D_CODE_MASK)
/** @brief Screen position packed as in a VDP1 command */
# define JO_3D_PACK_XY(X, Y)                (((int)(X) << 16) | ((int)(Y) & 0xFFFF))

/** @brief Set a transformed vertex
 *  @param v Vertex
 *  @param x Screen position
 *  @param y Screen position
 *  @param z Depth
 *  @param code Outcode
 */
static  __jo_force_inline void              jo_3d_set_vertex(jo_3d_vertex * const v, const int x, const int y, const jo_fixed z, const unsigned int code)
{
    v->xy = JO_3D_PACK_XY(x, y);
    v->zc = (z & ~JO_3D_CODE_MASK) | (jo_fixed)code;
}

/** @brief View space position of a vertex of a CPU (JO_3D_CODE_VIEW), from its index */
# define JO_3D_VIEW_POSITION(CTX, V)        (&(CTX)->view[(V) - (CTX)->vertices])

/*
** GLOBALS
*/
static jo_3d_matrix                         __jo_3d_matrix_stack[JO_3D_MATRIX_STACK_DEPTH];
static int                                  __jo_3d_matrix_level = 0;
/** @brief jo_3d_push_matrix() calls refused (the stack is full): their jo_3d_pop_matrix() don't pop */
static int                                  __jo_3d_matrix_overflow = 0;
/* Light direction in view space */
static jo_pos3D_fixed                       __jo_3d_light_direction;

/* Transformed vertices of the current object: screen position and view space depth, and the view space positions of the
   vertices behind the near plane or very far outside of the screen */
static jo_3d_vertex                         __jo_3d_vertices[JO_3D_MAX_VERTICES] JO_WORK_RAM_ENGINE_WORK_SECTION;
static jo_3d_view_position                  __jo_3d_view[JO_3D_MAX_VERTICES] JO_WORK_RAM_ENGINE_WORK_SECTION;

/** @brief State of the drawing of the 3D polygons by a CPU: the master CPU (jo_3d_mesh_draw(), etc) or the slave CPU
 *  (jo_3d_slave_mesh_draw()) draw at the same time with their own state (internal usage)
 */
typedef struct
{
    jo_3d_vertex                            *vertices;          // Transformed vertices of the current object
    unsigned int                            max_vertices;
    jo_3d_view_position                     *view;              // View space positions of the vertices with JO_3D_CODE_VIEW
    jo_fixed                                (*matrix)[3];       // Current matrix
    jo_fixed                                previous_polygon_z;
    jo_fixed                                forced_key;         // See jo_3d_emit_subdivided_polygon(), -1: none
    int                                     *depth_bias;
    unsigned int                            clip_table_first;   // Gouraud tables of the clipped and subdivided polygons: the master
    unsigned int                            clip_table_count;   // CPU takes them from the first of the set of the frame up, the slave
    unsigned int                            clip_table_max;     // CPU from the last down (see jo_3d_clip_table())
    unsigned short                          *polygon_count;     // Statistics
    unsigned short                          *vertice_count;
    unsigned short                          *displayed_polygon_count;
    bool                                    slave;              // The commands go to the slave command table (see jo_3d_slave_end())
    bool                                    mirror;             // A reflected view: the front faces are counterclockwise (see jo_3d_set_mirror())
    unsigned short                          clip_mode;          // Draw mode bits of the window (see jo_3d_window_clip())
    const jo_3d_part                        *parts;             // The points in parts with their own matrices (see
    int                                     part_count;         // __jo_3d_draw_parts()), JO_NULL: the current matrix
    int                                     front_shift;        // See jo_3d_set_front_layer(), -1: the normal sort
    const int                               *guard_band;        // See jo_3d_set_guard_band() (the slave CPU: its copy, the
    const jo_pos3D_fixed                    *light_direction;   // master CPU changes them while the slave CPU draws)
    unsigned int                            texture_words;      // Address and size of a tile of the texture (see jo_3d_emit_tile()),
                                                                // 0: the texture of the polygon
    unsigned int                            pixels;             // Pixels drawn by the VDP1 for the object (see JO_PROFILE_3D_PIXELS)
    unsigned short                          gouraud_offset;     // Added to the gouraud tables of the object: its tinted copy (see jo_3d_set_tint())
    int                                     environment;        // The environment map of the object (see jo_3d_set_mesh_environment()), -1: none
    unsigned short                          sliver_color;       // The polygon is drawn as a polygon of this color (see jo_3d_emit_sliver()), 0: none
    const jo_pos3D_fixed                    *points;            // Points of the current object (model space) and their count: the
    unsigned int                            point_count;        // exact view space positions of its vertices (see jo_3d_vertex_view())
}                                           jo_3d_cpu_context;

/** @brief Levels of tiles of a texture (2 x 2, 4 x 4) */
# define JO_3D_TILE_LEVELS                  (2)
/** @brief Levels of tiles of a texture with its finer tiles (8 x 8, 16 x 16 in one sprite, see jo_3d_set_texture_subdivision_fine()) */
# define JO_3D_TILE_FINE_LEVELS             (4)
/** @brief Commands of the frame over which the polygons are not cut for the perspective any more (see
 *  jo_3d_subdivision_level()) */
# define JO_3D_SUBDIVISION_BUDGET           (jo_vdp1_command_max() * 3 / 5)
/** @brief Size on the screen under which a texture copy of a size is used: 11/16 of the next bigger copy (see jo_3d_mipmap()) */
# define JO_3D_MIPMAP_LIMIT(SIZE)           (JO_MULT_BY_2(SIZE) - JO_DIV_BY_2(SIZE) - JO_DIV_BY_8(SIZE))

/** @brief Tiles of a texture for the subdivision of the polygons near the camera (see jo_3d_set_texture_subdivision()) */
typedef struct
{
    unsigned char                           levels;     // 0: none, 1: 2 x 2 tiles, 2: and 4 x 4 tiles, 4: and the finer tiles
                                                        // (JO_3D_TILE_FINE_LEVELS); JO_3D_TILES_NEAR_ONLY
    unsigned char                           error;      // Distortion over which a polygon is drawn in parts (pixels), 0:
                                                        // JO_3D_SUBDIVISION_ERROR (see jo_3d_set_texture_subdivision_error())
    unsigned short                          sprite[JO_3D_TILE_LEVELS + 1]; // The 2 x 2 tiles, the 4 x 4 tiles: a sprite per level, its
                                                        // tiles one under the other (a sprite per tile would use too many sprites);
                                                        // the 8 x 8 then the 16 x 16 tiles in one sprite (see jo_3d_tile_level())
    jo_fixed                                depth;      // Only the polygons whose nearest vertex is nearer than this are drawn in
                                                        // parts, 0: all (see jo_3d_set_texture_subdivision_depth())
}                                           jo_3d_texture_tiles;

/** @brief Flag of jo_3d_texture_tiles.levels: only the polygons that cross the near plane (or go far outside of the screen)
 *  are drawn in parts (see jo_3d_set_texture_subdivision_near_only()) */
# define JO_3D_TILES_NEAR_ONLY              (0x80)
/** @brief Levels of tiles of a texture (without the flag) */
# define JO_3D_TILE_LEVEL_COUNT(T)          ((T)->levels & 7)
/** @brief The last sprite of the tiles of a texture (the finer levels share one) */
# define JO_3D_TILE_LAST_SPRITE(T)          ((T)->sprite[JO_MIN(JO_3D_TILE_LEVEL_COUNT(T), JO_3D_TILE_LEVELS + 1) - 1])

/* (read by the assembly polygon loop: kept with their names by the link time optimization) */
jo_3d_texture_tiles                         __jo_3d_tiles[JO_MAX_SPRITE] __attribute__((used, externally_visible));

// (the assembly polygon loop reads levels at sprite x 12)
_Static_assert(sizeof(jo_3d_texture_tiles) == 12, "__jo_3d_polygon_loop: size of jo_3d_texture_tiles");
// (the assembly polygon loop uses these values, see its label 50)
_Static_assert(JO_3D_SUBDIVISION_ERROR == 8 && JO_3D_TILES_NEAR_ONLY == 128 && (JO_3D_CODE_SCREEN | JO_3D_CODE_VIEW) == 79 &&
               JO_3D_CODE_MASK == 127 && JO_3D_SLIVER_EXTENT == 64 && JO_3D_SLIVER_WIDTH == 4, "__jo_3d_polygon_loop: constants");
_Static_assert(__builtin_offsetof(jo_3d_texture_tiles, depth) == 8, "__jo_3d_polygon_loop: offset of depth");
_Static_assert(__builtin_offsetof(jo_3d_polygon_attribute, draw_mode) == 4 && JO_VDP1_COLOR_CALC_HALF_TRANSPARENT == 3,
               "__jo_3d_polygon_loop: the draw mode of the attribute (half transparent)");
_Static_assert(__builtin_offsetof(jo_3d_texture_tiles, error) == 1, "jo_3d_texture_tiles: the error in the padding after levels");

/** @brief The rectangle of the screen where the objects can be seen (see jo_3d_set_view_rect()): the slopes of its 4 planes
 *  (left, right, top, bottom: x = k * z) and their margins (sqrt(1 + k * k), about) */
typedef struct
{
    bool                                    enabled;
    bool                                    empty;
    jo_fixed                                k[4];
    jo_fixed                                margin[4];
}                                           jo_3d_view_rect_planes;

static jo_3d_view_rect_planes               __jo_3d_view_rect = {false, false, {0, 0, 0, 0}, {0, 0, 0, 0}};
/** @brief Its copy for the slave CPU (see jo_3d_slave_begin(): the master CPU can change it while the slave CPU draws, for
 *  the rear-view mirror for instance) */
static jo_3d_view_rect_planes               __jo_3d_slave_view_rect = {false, false, {0, 0, 0, 0}, {0, 0, 0, 0}};

/** @brief The side planes of the screen for jo_3d_is_sphere_visible(): their slopes (|x| = kx * z, |y| = ky * z) and margins,
 *  computed again only when the projection distance changes (a copy per CPU: each CPU writes only its own) */
typedef struct
{
    jo_fixed                                projection_distance;    // (0: not computed yet)
    jo_fixed                                kx;
    jo_fixed                                ky;
    jo_fixed                                margin_x;
    jo_fixed                                margin_y;
    int                                     height_2;               // (the height changes alone: 224 to 240 lines)
}                                           jo_3d_screen_planes;

static jo_3d_screen_planes                  __jo_3d_screen_planes = {0, 0, 0, 0, 0, 0};
static jo_3d_screen_planes                  __jo_3d_slave_screen_planes = {0, 0, 0, 0, 0, 0};

/** @brief Smaller copies of a texture (see jo_3d_set_texture_mipmaps()) */
typedef struct
{
    unsigned char                           levels;     // 0: none, 1: half size, 2: half and quarter size
    unsigned short                          first;      // Sprite of the half size (the quarter size is the next sprite)
    unsigned short                          width[2];   // The copy of a level is used up to these sizes on the screen
    unsigned short                          height[2];  // (see __jo_3d_mipmap_select(), computed once)
    int                                     area[2];
}                                           jo_3d_texture_mipmaps;

jo_3d_texture_mipmaps                       __jo_3d_mipmaps[JO_MAX_SPRITE] __attribute__((used, externally_visible));
// (the assembly polygon loop reads the copies at sprite x 20: the smaller copies were never used when the structure grew)
_Static_assert(sizeof(jo_3d_texture_mipmaps) == 20, "__jo_3d_polygon_loop: size of jo_3d_texture_mipmaps");
_Static_assert(__builtin_offsetof(jo_3d_texture_mipmaps, first) == 2 && __builtin_offsetof(jo_3d_texture_mipmaps, width) == 4 &&
               __builtin_offsetof(jo_3d_texture_mipmaps, height) == 8 && __builtin_offsetof(jo_3d_texture_mipmaps, area) == 12,
               "__jo_3d_polygon_loop: offsets of jo_3d_texture_mipmaps");

/** @brief Flags of a texture for the assembly polygon loop (see jo_3d_update_texture_flags()): one byte per texture, the
 *  tiles and the smaller copies are read only when the texture has them (the 2 tables of 12 and 20 bytes per texture don't
 *  stay in the 4 KB cache) */
# define JO_3D_TEXTURE_HAS_TILES            (1)
# define JO_3D_TEXTURE_HAS_MIPMAPS          (2)
unsigned char                               __jo_3d_texture_flags[JO_MAX_SPRITE] __attribute__((used, externally_visible));

/** @brief Compute the flags of a texture again (after a change of its tiles or of its smaller copies)
 *  @param sprite_id Texture
 */
static void                                 jo_3d_update_texture_flags(const int sprite_id)
{
    __jo_3d_texture_flags[sprite_id] = (unsigned char)((__jo_3d_tiles[sprite_id].levels != 0 ? JO_3D_TEXTURE_HAS_TILES : 0) |
                                                       (__jo_3d_mipmaps[sprite_id].levels != 0 ? JO_3D_TEXTURE_HAS_MIPMAPS : 0));
}

/*
** Environment maps (see jo_3d_set_mesh_environment()): the polygons of a mesh show a tile of a picture of the surroundings,
** the tile of the direction of their normal in the view (a reflection: it turns with the camera, not with the object)
*/

# define JO_3D_ENVIRONMENT_MESHES           (8)

typedef struct
{
    const void                              *key;               // The polygons of the mesh, JO_NULL: free
    int                                     sprite;             // The tiles (one under the other)
    int                                     tiles;              // Tiles on a side
    int                                     first_sprite;       // Its first sprite (forgotten when it is freed)
    unsigned int                            tile_step;          // The address of a tile after the previous one (/ 8)
    unsigned int                            tile_size;          // The width / 8 and the height of a tile (low word of the
                                                                // texture words of a command)
}                                           jo_3d_environment;

static jo_3d_environment                    __jo_3d_environments[JO_3D_ENVIRONMENT_MESHES];
/** @brief The number of environment maps (0: jo_3d_environment_of() does not search, it is called for each object) */
static int                                  __jo_3d_environment_count = 0;

/** @brief Count the environment maps again (after one is added or freed) */
static void                                 jo_3d_count_environments(void)
{
    int                                     k;

    JO_ZERO(__jo_3d_environment_count);
    for (JO_ZERO(k); k < JO_3D_ENVIRONMENT_MESHES; ++k)
        if (__jo_3d_environments[k].key != JO_NULL)
            ++__jo_3d_environment_count;
}

/** @brief The environment map of a mesh, -1: none */
static int                                  jo_3d_environment_of(const jo_3d_polygon_data * const pdata)
{
    int                                     k;

    if (__jo_3d_environment_count == 0)
        return (-1);
    for (JO_ZERO(k); k < JO_3D_ENVIRONMENT_MESHES; ++k)
        if (__jo_3d_environments[k].key == pdata)
            return (k);
    return (-1);
}


/*
** Tint (see jo_3d_set_tint()): the objects drawn with a tint use a copy of their gouraud tables with the tint added. Each
** mesh keeps JO_3D_TINT_VARIANTS copies (the tints used last): a copy is written again only when a new tint is needed
*/

# define JO_3D_TINT_MESHES                  (16)
# define JO_3D_TINT_VARIANTS                (4)

typedef struct
{
    const void                              *key;               // The polygons of the mesh (jo_3d_polygon_data), JO_NULL: free
    unsigned short                          first;              // Its gouraud tables: first, count
    unsigned short                          count;
    jo_color                                *colors;            // Their colors (work RAM)
    int                                     first_sprite;       // The sprite of the first copy (forgotten when it is freed)
    unsigned short                          table[JO_3D_TINT_VARIANTS];    // The copies
    int                                     tint[JO_3D_TINT_VARIANTS];     // Their tints (packed, -1: none yet)
    unsigned int                            used[JO_3D_TINT_VARIANTS];     // Last use (LRU)
}                                           jo_3d_tint_cache;

static jo_3d_tint_cache                     __jo_3d_tints[JO_3D_TINT_MESHES];
static int                                  __jo_3d_tint = 0;
static int                                  __jo_3d_slave_tint = 0;
static unsigned int                         __jo_3d_tint_clock = 0;
/** @brief The frame of the tints (see jo_3d_tint_offset()): a copy used in this frame is not written again in it (the
 *  slave CPU draws with the copies the master CPU prepared, see jo_3d_slave_set_tint()) */
static unsigned int                         __jo_3d_tint_frame_first_clock = 0;
/** @brief Difference of tint (gouraud units, each component) under which a copy is used for another tint (the characters of
 *  a room have nearly the same tint: fewer copies written) */
# define JO_3D_TINT_NEAR                    (3)

/** @brief A tint packed (see jo_3d_set_tint()) */
static  __jo_force_inline int               jo_3d_pack_tint(const int r, const int g, const int b)
{
    // (steps of 2 gouraud units: fewer copies)
    return (((JO_MIN(JO_MAX(r, -31), 31) >> 1) & 0x1f) | (((JO_MIN(JO_MAX(g, -31), 31) >> 1) & 0x1f) << 5) |
            (((JO_MIN(JO_MAX(b, -31), 31) >> 1) & 0x1f) << 10));
}

void                                        jo_3d_set_tint(const int r, const int g, const int b)
{
    __jo_3d_tint = jo_3d_pack_tint(r, g, b);
}

unsigned short                              jo_3d_get_tint(void)
{
    return ((unsigned short)__jo_3d_tint);
}

void                                        jo_3d_set_packed_tint(const unsigned short tint)
{
    __jo_3d_tint = tint;
}

void                                        jo_3d_slave_set_tint(const unsigned short tint)
{
    __jo_3d_slave_tint = tint;
}

static void                                 __jo_3d_tint_new_frame(void)
{
    __jo_3d_tint_frame_first_clock = __jo_3d_tint_clock + 1;
}

/** @brief A component of a packed tint (signed, gouraud units) */
static  __jo_force_inline int               jo_3d_tint_component(const int tint, const int shift)
{
    int                                     v;

    v = (tint >> shift) & 0x1f;
    return ((v >= 16 ? v - 32 : v) * 2);
}

/** @brief Write a tinted copy of the gouraud tables of a mesh
 *  @remarks Not inlined: its tables on the stack only while a copy is written (not in the frame of jo_3d_draw_polygons(), the
 *           short stack of the slave CPU)
 */
static __attribute__((noinline)) bool       jo_3d_write_tint(const jo_3d_tint_cache * const c, const int variant, const int tint)
{
    jo_color                                *colors;
    const jo_color                          *in;
    const jo_color                          *end;
    jo_color                                *out;
    unsigned short                          red[32];
    unsigned short                          green[32];
    unsigned short                          blue[32];
    unsigned int                            color;
    int                                     k;
    int                                     tr;
    int                                     tg;
    int                                     tb;

    colors = (jo_color *)jo_malloc(c->count * 4 * sizeof(jo_color));
    if (colors == JO_NULL)
        return (false);
    tr = jo_3d_tint_component(tint, 0);
    tg = jo_3d_tint_component(tint, 5);
    tb = jo_3d_tint_component(tint, 10);
    // Each component tinted and clamped once (32 values): a color is 3 reads of these tables (the tables of a character: about
    // 600 colors, written again at each new tint of a character hit)
    for (JO_ZERO(k); k < 32; ++k)
    {
        red[k] = (unsigned short)(JO_MIN(JO_MAX(k + tr, 0), 31) | 0x8000);
        green[k] = (unsigned short)(JO_MIN(JO_MAX(k + tg, 0), 31) << 5);
        blue[k] = (unsigned short)(JO_MIN(JO_MAX(k + tb, 0), 31) << 10);
    }
    for (in = c->colors, end = in + c->count * 4, out = colors; in < end; ++in, ++out)
    {
        color = *in;
        *out = (jo_color)(red[color & 0x1f] | green[(color >> 5) & 0x1f] | blue[(color >> 10) & 0x1f]);
    }
    jo_dma_copy(colors, (void *)(JO_VDP1_VRAM + JO_MULT_BY_8(c->table[variant])), c->count * 4 * sizeof(jo_color));
    jo_free(colors);
    return (true);
}

/** @brief Difference of 2 packed tints: the biggest of their components (gouraud units) */
static  __jo_force_inline int               jo_3d_tint_distance(const int a, const int b)
{
    int                                     d;
    int                                     e;

    d = JO_ABS(jo_3d_tint_component(a, 0) - jo_3d_tint_component(b, 0));
    e = JO_ABS(jo_3d_tint_component(a, 5) - jo_3d_tint_component(b, 5));
    d = JO_MAX(d, e);
    e = JO_ABS(jo_3d_tint_component(a, 10) - jo_3d_tint_component(b, 10));
    return (JO_MAX(d, e));
}

/** @brief The copy of the tables of a mesh nearest to a tint (see jo_3d_tint_offset())
 *  @return The copy, -1: none near enough (JO_3D_TINT_NEAR)
 */
static  __jo_force_inline int               jo_3d_tint_nearest(const jo_3d_tint_cache * const c, const int tint)
{
    int                                     v;
    int                                     best;
    int                                     distance;
    int                                     d;

    best = -1;
    distance = JO_3D_TINT_NEAR + 1;
    for (JO_ZERO(v); v < JO_3D_TINT_VARIANTS; ++v)
    {
        if (c->tint[v] < 0)
            continue;
        d = jo_3d_tint_distance(c->tint[v], tint);
        if (d < distance)
        {
            distance = d;
            best = v;
        }
    }
    return (best);
}

/** @brief The offset to the tinted copy of the gouraud tables of a mesh for the tint of the slave CPU (0: no tint, or no
 *  copy): the copies prepared by the master CPU (see jo_3d_slave_set_tint()), read only, the nearest one */
static unsigned short                       jo_3d_slave_tint_offset(const jo_3d_polygon_data * const pdata)
{
    const jo_3d_tint_cache                  *c;
    int                                     k;
    int                                     v;

    if (__jo_3d_slave_tint == 0)
        return (0);
    for (k = 0, c = __jo_3d_tints; k < JO_3D_TINT_MESHES; ++k, ++c)
        if (c->key == pdata)
        {
            v = jo_3d_tint_nearest(c, __jo_3d_slave_tint);
            return (v < 0 ? 0 : (unsigned short)(c->table[v] - c->first));
        }
    return (0);
}

/** @brief The offset to the tinted copy of the gouraud tables of a mesh for the current tint (0: no tint, or no copy)
 *  @remarks A copy used in this frame (see __jo_3d_tint_new_frame()) is not written again: the slave CPU draws with it
 *  (jo_3d_slave_tint_offset()). The copies written in this frame are all used: the nearest one (JO_3D_TINT_NEAR), or none
 */
static unsigned short                       jo_3d_tint_offset(const jo_3d_polygon_data * const pdata)
{
    jo_3d_tint_cache                        *c;
    jo_3d_tint_cache                        *free_cache;
    int                                     first;
    int                                     last;
    int                                     k;
    int                                     v;
    int                                     oldest;

    if (__jo_3d_tint == 0)
        return (0);
    free_cache = JO_NULL;
    for (k = 0, c = __jo_3d_tints; k < JO_3D_TINT_MESHES; ++k, ++c)
    {
        if (c->key == pdata)
            break;
        if (c->key == JO_NULL && free_cache == JO_NULL)
            free_cache = c;
    }
    if (k == JO_3D_TINT_MESHES)
    {
        // A new mesh: its tables (one block), their colors, its copies
        if ((c = free_cache) == JO_NULL)
            return (0);
        first = 0xffff;
        last = -1;
        for (JO_ZERO(k); k < (int)pdata->polygon_count; ++k)
        {
            if (!(pdata->attributes[k].draw_mode & JO_VDP1_COLOR_CALC_GOURAUD) || pdata->attributes[k].gouraud_table == 0)
                continue;
            first = JO_MIN(first, (int)pdata->attributes[k].gouraud_table);
            last = JO_MAX(last, (int)pdata->attributes[k].gouraud_table);
        }
        if (last < first || last - first + 1 > 1024)
            return (0);
        // (no sprite left: the mesh is drawn without its tint, the game goes on)
        if (jo_get_last_sprite_id() + 2 > JO_MAX_SPRITE)
            return (0);
        jo_memset(c, 0, sizeof(*c));
        c->first = (unsigned short)first;
        c->count = (unsigned short)(last - first + 1);
        c->colors = (jo_color *)jo_malloc(c->count * 4 * sizeof(jo_color));
        if (c->colors == JO_NULL)
            return (0);
        jo_memcpy(c->colors, (const void *)(JO_VDP1_VRAM + JO_MULT_BY_8(first)), c->count * 4 * sizeof(jo_color));
        c->first_sprite = jo_get_last_sprite_id() + 1;
        // (the copies in one block of the VRAM: one sprite for the mesh, not one per copy; each new mesh tinted (a character hit,
        // its dead model) took 4 sprites in the middle of the game, the sprites of the level
        // were used up. A block of tables is never drawn: its height over 255 lines doesn't matter. Each copy is
        // an even count of tables: 2 tables per line of 8 pixels)
        {
            unsigned short                  base;
            unsigned short                  stride;

            stride = (unsigned short)((c->count + 1) & ~1);
            base = jo_3d_add_gouraud_tables(JO_NULL, (unsigned int)stride * JO_3D_TINT_VARIANTS);
            if (base == 0)
            {
                jo_free(c->colors);
                c->colors = JO_NULL;
                return (0);
            }
            for (JO_ZERO(v); v < JO_3D_TINT_VARIANTS; ++v)
            {
                c->table[v] = (unsigned short)(base + v * stride);
                c->tint[v] = -1;
            }
        }
        c->key = pdata;
    }
    ++__jo_3d_tint_clock;
    oldest = -1;
    for (JO_ZERO(v); v < JO_3D_TINT_VARIANTS; ++v)
    {
        if (c->tint[v] == __jo_3d_tint)
        {
            c->used[v] = __jo_3d_tint_clock;
            return ((unsigned short)(c->table[v] - c->first));
        }
        // (a copy used in this frame is kept)
        if (c->tint[v] >= 0 && c->used[v] >= __jo_3d_tint_frame_first_clock)
            continue;
        if (oldest < 0 || c->used[v] < c->used[oldest])
            oldest = v;
    }
    if (oldest < 0)
    {
        // (all the copies are used in this frame: the nearest one)
        v = jo_3d_tint_nearest(c, __jo_3d_tint);
        if (v < 0)
            return (0);
        c->used[v] = __jo_3d_tint_clock;
        return ((unsigned short)(c->table[v] - c->first));
    }
    // A new tint: the copy used the longest time ago
    // (out of memory: the copy is not written, not marked as this tint)
    if (!jo_3d_write_tint(c, oldest, __jo_3d_tint))
        return (0);
    c->tint[oldest] = __jo_3d_tint;
    c->used[oldest] = __jo_3d_tint_clock;
    return ((unsigned short)(c->table[oldest] - c->first));
}

void                                        jo_3d_prepare_tint(const jo_3d_mesh * const mesh)
{
    if (mesh != JO_NULL)
        jo_3d_tint_offset(&mesh->data);
}

/** @brief The sprites that fade with the distance, at most (see jo_3d_set_sprite_fade()) */
# define JO_3D_FADE_SPRITES                 (16)

typedef struct
{
    short                                   sprite;     // Sprite (its smaller copies too)
    unsigned short                          bank;       // Color code of its color 0 (multiple of 16)
}                                           jo_3d_fade_sprite;

static jo_3d_fade_sprite                    __jo_3d_fade_sprites[JO_3D_FADE_SPRITES];
static int                                  __jo_3d_fade_sprite_count = 0;

void                                        __jo_3d_forget_sprites_from(const int sprite_id)
{
    int                                     i;

    // (the tiles and the smaller copies are sprites added after their texture: a texture kept whose copies are freed loses
    // them too)
    for (JO_ZERO(i); i < JO_MAX_SPRITE; ++i)
    {
        // (the sprites of the last level are the last ones: one of them freed, the tiles are lost)
        if (__jo_3d_tiles[i].levels != 0 && (i >= sprite_id || (int)JO_3D_TILE_LAST_SPRITE(&__jo_3d_tiles[i]) >= sprite_id))
        {
            __jo_3d_tiles[i].levels = 0;
            __jo_3d_tiles[i].error = 0;
            __jo_3d_tiles[i].depth = 0;
        }
        if (__jo_3d_mipmaps[i].levels != 0 && (i >= sprite_id || (int)__jo_3d_mipmaps[i].first + __jo_3d_mipmaps[i].levels - 1 >= sprite_id))
            __jo_3d_mipmaps[i].levels = 0;
        jo_3d_update_texture_flags(i);
    }
    // The sprites that fade (see jo_3d_set_sprite_fade())
    for (i = __jo_3d_fade_sprite_count - 1; i >= 0; --i)
        if (__jo_3d_fade_sprites[i].sprite >= sprite_id)
            __jo_3d_fade_sprites[i] = __jo_3d_fade_sprites[--__jo_3d_fade_sprite_count];
    // The environment maps
    for (JO_ZERO(i); i < JO_3D_ENVIRONMENT_MESHES; ++i)
        if (__jo_3d_environments[i].key != JO_NULL && __jo_3d_environments[i].first_sprite >= sprite_id)
            __jo_3d_environments[i].key = JO_NULL;
    jo_3d_count_environments();
    // The tinted copies (and the meshes: their tables are sprites added before them)
    for (JO_ZERO(i); i < JO_3D_TINT_MESHES; ++i)
        if (__jo_3d_tints[i].key != JO_NULL && __jo_3d_tints[i].first_sprite >= sprite_id)
        {
            if (__jo_3d_tints[i].colors != JO_NULL)
                jo_free(__jo_3d_tints[i].colors);
            jo_memset(&__jo_3d_tints[i], 0, sizeof(__jo_3d_tints[i]));
        }
}

void                                        __jo_3d_forget_tint(const jo_3d_polygon_data * const pdata)
{
    int                                     i;
    int                                     sprite;

    // (the copies are given back when they are the last sprite: else they stay in the VRAM, a new mesh in the cache gets
    // new copies)
    for (JO_ZERO(i); i < JO_3D_TINT_MESHES; ++i)
        if (__jo_3d_tints[i].key == pdata)
        {
            sprite = __jo_3d_tints[i].first_sprite;
            if (__jo_3d_tints[i].colors != JO_NULL)
                jo_free(__jo_3d_tints[i].colors);
            jo_memset(&__jo_3d_tints[i], 0, sizeof(__jo_3d_tints[i]));
            if (sprite > 0 && sprite == jo_get_last_sprite_id())
                jo_sprite_free_from(sprite);
        }
}

void                                        __jo_3d_forget_environment(const jo_3d_polygon_data * const pdata)
{
    int                                     k;
    int                                     sprite;

    k = jo_3d_environment_of(pdata);
    if (k < 0)
        return;
    sprite = __jo_3d_environments[k].first_sprite;
    __jo_3d_environments[k].key = JO_NULL;
    jo_3d_count_environments();
    // (its tiles given back when they are the last sprite)
    if (sprite > 0 && sprite == jo_get_last_sprite_id())
        jo_sprite_free_from(sprite);
}

/* Gouraud tables of the clipped polygons: first table of the set of the frame (see JO_3D_CLIP_GOURAUD_SETS) */
static unsigned int                         __jo_3d_clip_table_base = 0;

/* Clipping: the polygons are clipped when they go further than this distance outside of the screen (see jo_3d_set_guard_band()) */
int                                         __jo_3d_guard_band = JO_3D_DEFAULT_GUARD_BAND;
/** @brief See jo_3d_set_fewer_parts() (read by both CPUs: through the cache-through address) */
static volatile bool                        __jo_3d_fewer_parts = false;
#ifdef JO_TEST_SPAN
/* Test (JO_TEST_SPAN, read by the game): the vertices transformed by the master CPU, its polygons, the points of their meshes */
unsigned int                                __jo_3d_test_span[3];
#endif

/* Statistics of the current frame */
unsigned short                              __jo_3d_polygon_count = 0;
unsigned short                              __jo_3d_vertice_count = 0;
unsigned short                              __jo_3d_displayed_polygon_count = 0;

/* Gouraud tables of the clipped polygons used in the frame by the master CPU and by the slave CPU: they share the set of the
** frame (see jo_3d_clip_table()), read through the cache by the other CPU */
static volatile unsigned int                __jo_3d_clip_tables_used[2] = {0, 0};
/** @brief The gouraud tables of the slave CPU taken for this frame (by its first job: a frame can have several jobs of
 *  the slave CPU, the views of a split screen, whose tables are all drawn with the frame) */
static bool                                 __jo_3d_slave_tables_taken = false;
/** @brief A copy of the commands of the slave CPU was started this frame (by its DMA controller, see jo_3d_slave_end()) */
static bool                                 __jo_3d_slave_copy_started = false;

/* State of the master CPU */
static jo_3d_cpu_context                    __jo_3d_master = {__jo_3d_vertices, JO_3D_MAX_VERTICES, __jo_3d_view, JO_NULL, 0, -1, &__jo_vdp1_depth_bias, 0, 0,
                                                              JO_3D_CLIP_GOURAUD_TABLES,
                                                              &__jo_3d_polygon_count, &__jo_3d_vertice_count, &__jo_3d_displayed_polygon_count, false,
                                                              false, 0, JO_NULL, 0, -1, &__jo_3d_guard_band, &__jo_3d_light_direction, 0, 0, 0, -1, 0,
                                                              JO_NULL, 0};

/** @brief Reset the statistics of the frame (Internal usage) */
void                                        __jo_3d_end_frame(void)
{
    __jo_3d_tint_new_frame();
    JO_ZERO(__jo_3d_polygon_count);
    JO_ZERO(__jo_3d_vertice_count);
    JO_ZERO(__jo_3d_displayed_polygon_count);
    JO_ZERO(__jo_3d_master.clip_table_count);
    *(volatile unsigned int *)JO_CACHE_THROUGH(&__jo_3d_clip_tables_used[0]) = 0;
    // (the tables of the slave CPU too: a frame without jo_3d_slave_begin() lost them)
    *(volatile unsigned int *)JO_CACHE_THROUGH(&__jo_3d_clip_tables_used[1]) = 0;
    __jo_3d_clip_table_base += JO_3D_CLIP_GOURAUD_TABLES;
    if (__jo_3d_clip_table_base >= JO_3D_CLIP_GOURAUD_TABLES * JO_3D_CLIP_GOURAUD_SETS)
        JO_ZERO(__jo_3d_clip_table_base);
    __jo_3d_master.clip_table_first = __jo_3d_clip_table_base;
    // (the tables of the slave CPU: taken again by the first jo_3d_slave_begin() of the next frame)
    __jo_3d_slave_tables_taken = false;
    __jo_3d_slave_copy_started = false;
}

/** @brief Squared length of a vector
 *  @param v Vector
 *  @return |v|² in Q16.16 (64 bits)
 */
static  __jo_force_inline long long         jo_3d_squared_length(const jo_pos3D_fixed * const v)
{
    // (the sum unsigned: 3 squares of components over 26755.0 overflowed the signed 64 bits)
    return ((long long)(((unsigned long long)((long long)v->x * v->x) + (unsigned long long)((long long)v->y * v->y) +
                         (unsigned long long)((long long)v->z * v->z)) >> 16));
}

/** @brief Component of a normalized vector: x * inverse / 2^shift, rounded toward zero like a division
 *  @param x Component
 *  @param inverse 2^(shift + 16) / length, rounded up
 *  @param shift 22 or 30 (constant: the SH-2 has no variable shift)
 *  @return x / length (Q16.16)
 */
static  __jo_force_inline jo_fixed          jo_3d_normalize_component(const jo_fixed x, const jo_fixed inverse, const int shift)
{
    long long                               product;

    product = (long long)x * inverse;
    if (product < 0)
        product += ((long long)1 << shift) - 1;
    return ((jo_fixed)(product >> shift));
}

/** @brief Normalize a vector
 *  @param v Vector (input and output)
 *  @remarks A null vector is not changed
 */
static void                                 jo_3d_normalize(jo_pos3D_fixed *v)
{
    long long                               len2;
    jo_fixed                                len;
    jo_fixed                                inverse;

    len2 = jo_3d_squared_length(v);
    if (len2 < JO_FIXED_1)
    {
        // A vector shorter than 1.0: its squared length has less than 16 bits, the square root was up to 41 % off (the right
        // axis of a camera that looks almost straight down stretched the picture). 16 times longer, the same direction (x 256
        // could jump over the range [1.0, 181.0) and loop with the >> 8 below)
        if ((v->x != 0 || v->y != 0 || v->z != 0) && JO_ABS(v->x) < 0x10000 && JO_ABS(v->y) < 0x10000 && JO_ABS(v->z) < 0x10000)
        {
            v->x <<= 4;
            v->y <<= 4;
            v->z <<= 4;
            jo_3d_normalize(v);
        }
        return;
    }
    if (len2 > JO_FIXED_MAX)
    {
        v->x >>= 8;
        v->y >>= 8;
        v->z >>= 8;
        jo_3d_normalize(v);
        return;
    }
    if ((len = jo_fixed_sqrt((jo_fixed)len2)) == 0)
        return;
    // One division and 3 multiplications instead of 3 divisions (39 cycles each). len < 182.0 (len2 <= JO_FIXED_MAX).
    // The inverse has 22 to 30 significant bits: the result is the same as the division, rarely 1/65536 lower or higher
    if (len >= JO_FIXED_1)
    {
        // inverse = 2^46 / len, rounded up (2^22 to 2^30)
        JO_SH2_DVSR = len;
        JO_SH2_DVDNTH = 1 << 14;
        JO_SH2_DVDNTL = len - 1;
        inverse = (jo_fixed)JO_SH2_DVDNTL;
        v->x = jo_3d_normalize_component(v->x, inverse, 30);
        v->y = jo_3d_normalize_component(v->y, inverse, 30);
        v->z = jo_3d_normalize_component(v->z, inverse, 30);
    }
    else if (len >= 256)
    {
        // inverse = 2^38 / len, rounded up (2^22 to 2^30)
        JO_SH2_DVSR = len;
        JO_SH2_DVDNTH = 1 << 6;
        JO_SH2_DVDNTL = len - 1;
        inverse = (jo_fixed)JO_SH2_DVDNTL;
        v->x = jo_3d_normalize_component(v->x, inverse, 22);
        v->y = jo_3d_normalize_component(v->y, inverse, 22);
        v->z = jo_3d_normalize_component(v->z, inverse, 22);
    }
    else
    {
        // Very short vector (length < 1/256): the inverse would not fit in 32 bits
        v->x = jo_fixed_div(v->x, len);
        v->y = jo_fixed_div(v->y, len);
        v->z = jo_fixed_div(v->z, len);
    }
}

/*
** Matrix
*/

/** @brief Set a matrix to the identity
 *  @param m Matrix
 */
static void                                 jo_3d_set_identity(jo_3d_matrix m)
{
    m[0][0] = JO_FIXED_1;
    m[0][1] = JO_FIXED_0;
    m[0][2] = JO_FIXED_0;
    m[1][0] = JO_FIXED_0;
    m[1][1] = JO_FIXED_1;
    m[1][2] = JO_FIXED_0;
    m[2][0] = JO_FIXED_0;
    m[2][1] = JO_FIXED_0;
    m[2][2] = JO_FIXED_1;
    m[3][0] = JO_FIXED_0;
    m[3][1] = JO_FIXED_0;
    m[3][2] = JO_FIXED_0;
}

/** @brief Set the current matrix to the identity (Internal usage) */
void                                        __jo_3d_reset_matrix(void)
{
    jo_3d_set_identity(JO_3D_CURRENT_MATRIX);
}

void                                        jo_3d_push_matrix(void)
{
#ifdef JO_DEBUG
    if (__jo_3d_matrix_level >= JO_3D_MATRIX_STACK_DEPTH - 1)
        jo_core_error("Too many jo_3d_push_matrix()");
#endif
    if (__jo_3d_matrix_level >= JO_3D_MATRIX_STACK_DEPTH - 1)
    {
        ++__jo_3d_matrix_overflow;
        return;
    }
    jo_3d_copy_matrix(__jo_3d_matrix_stack[__jo_3d_matrix_level + 1], (const jo_fixed (*)[3])__jo_3d_matrix_stack[__jo_3d_matrix_level]);
    ++__jo_3d_matrix_level;
}

void                                        jo_3d_pop_matrix(void)
{
#ifdef JO_DEBUG
    if (__jo_3d_matrix_level <= 0 && __jo_3d_matrix_overflow == 0)
    {
        jo_core_error("Too many jo_3d_pop_matrix()");
        return;
    }
#endif
    // (the pop of a refused push: the level stays, the stack stays balanced)
    if (__jo_3d_matrix_overflow > 0)
    {
        --__jo_3d_matrix_overflow;
        return;
    }
    if (__jo_3d_matrix_level <= 0)
        return;
    --__jo_3d_matrix_level;
}

/** @brief Copy the 3 axes of a matrix by column (contiguous values for jo_fixed_dot())
 *  @param m Matrix
 *  @param columns Output: columns[i] = { m[0][i], m[1][i], m[2][i] }
 */
static  __jo_force_inline void              jo_3d_get_columns(jo_fixed (* const m)[3], jo_fixed columns[3][3])
{
    register int                            i;

    for (JO_ZERO(i); i < 3; ++i)
    {
        columns[i][0] = m[0][i];
        columns[i][1] = m[1][i];
        columns[i][2] = m[2][i];
    }
}

/** @brief Transform a vector by the 3 axes of a matrix (without the translation)
 *  @param columns Columns of the matrix (see jo_3d_get_columns())
 *  @param in Vector
 *  @param out Transformed vector (output)
 */
static  __jo_force_inline void              jo_3d_transform_vector(jo_fixed columns[3][3], const jo_fixed * const in, jo_fixed * const out)
{
    out[0] = jo_fixed_dot(in, columns[0]);
    out[1] = jo_fixed_dot(in, columns[1]);
    out[2] = jo_fixed_dot(in, columns[2]);
}

/** @brief Translate a matrix (see __jo_3d_translate())
 *  @remarks The sums of the products on 64 bits, then the middle 32 bits: the result of jo_fixed_dot() (the multiply and
 *  accumulate unit), without the copy of the columns of the matrix
 */
static void                                 jo_3d_matrix_translate(jo_fixed (* const m)[3], const jo_fixed x, const jo_fixed y, const jo_fixed z)
{
    register int                            i;

    for (JO_ZERO(i); i < 3; ++i)
        m[3][i] += (jo_fixed)(((long long)x * m[0][i] + (long long)y * m[1][i] + (long long)z * m[2][i]) >> 16);
}

void                                        __jo_3d_translate(const jo_fixed x, const jo_fixed y, const jo_fixed z)
{
    jo_3d_matrix_translate(JO_3D_CURRENT_MATRIX, x, y, z);
}

/** @brief Scale a matrix (see __jo_3d_scale()) */
static void                                 jo_3d_matrix_scale(jo_fixed (* const m)[3], const jo_fixed x, const jo_fixed y, const jo_fixed z)
{
    // (a scale of 1, jo_3d_restore_scale() for instance: the products would not change the matrix)
    if (x == JO_FIXED_1 && y == JO_FIXED_1 && z == JO_FIXED_1)
        return;
    m[0][0] = jo_fixed_mult(m[0][0], x);
    m[0][1] = jo_fixed_mult(m[0][1], x);
    m[0][2] = jo_fixed_mult(m[0][2], x);
    m[1][0] = jo_fixed_mult(m[1][0], y);
    m[1][1] = jo_fixed_mult(m[1][1], y);
    m[1][2] = jo_fixed_mult(m[1][2], y);
    m[2][0] = jo_fixed_mult(m[2][0], z);
    m[2][1] = jo_fixed_mult(m[2][1], z);
    m[2][2] = jo_fixed_mult(m[2][2], z);
}

void                                        __jo_3d_scale(const jo_fixed x, const jo_fixed y, const jo_fixed z)
{
    jo_3d_matrix_scale(JO_3D_CURRENT_MATRIX, x, y, z);
}

/** @brief Rotate the rows a and b of a matrix (see jo_3d_matrix_rotate(): inlined with constant rows, the bones of a skeleton) */
static  __jo_force_inline void              jo_3d_matrix_rotate_rows(jo_fixed (* const m)[3], const int a, const int b, const jo_angle angle)
{
    jo_fixed                                s;
    jo_fixed                                c;
    jo_fixed                                va;
    jo_fixed                                vb;
    register int                            i;

    s = jo_sin_angle(angle);
    c = jo_cos_angle(angle);
    for (JO_ZERO(i); i < 3; ++i)
    {
        va = m[a][i];
        vb = m[b][i];
        m[a][i] = jo_fixed_mult(va, c) + jo_fixed_mult(vb, s);
        m[b][i] = jo_fixed_mult(vb, c) - jo_fixed_mult(va, s);
    }
}

/** @brief Rotate a matrix (see __jo_3d_rotate()) */
static void                                 jo_3d_matrix_rotate(jo_fixed (* const m)[3], const int axis, const jo_angle angle)
{
    if (angle == 0)
        return;
    // Rotation in the plane of the 2 other axes (X: Y->Z, Y: Z->X, Z: X->Y)
    jo_3d_matrix_rotate_rows(m, (axis + 1) % 3, (axis + 2) % 3, angle);
}

void                                        __jo_3d_rotate(const int axis, const jo_angle angle)
{
    jo_3d_matrix_rotate(JO_3D_CURRENT_MATRIX, axis, angle);
}

void                                        __jo_3d_bone_matrix(jo_fixed (* const out)[3], const jo_fixed (* const parent)[3],
                                                                const jo_pos3D_fixed * const pivot, const jo_angle * const angles)
{
    jo_3d_copy_matrix(out, parent);
    // Around the joint: around X, then Y, then Z of the parent (the matrix is multiplied on the right: Z first)
    if ((angles[0] | angles[1] | angles[2]) == 0)
        return;
    jo_3d_matrix_translate(out, pivot->x, pivot->y, pivot->z);
    // (the rows of jo_3d_matrix_rotate(): Z turns X->Y, Y turns Z->X, X turns Y->Z)
    if (angles[2] != 0)
        jo_3d_matrix_rotate_rows(out, 0, 1, angles[2]);
    if (angles[1] != 0)
        jo_3d_matrix_rotate_rows(out, 2, 0, angles[1]);
    if (angles[0] != 0)
        jo_3d_matrix_rotate_rows(out, 1, 2, angles[0]);
    jo_3d_matrix_translate(out, -pivot->x, -pivot->y, -pivot->z);
}

jo_fixed                                    (*__jo_3d_current_matrix(void))[3]
{
    return (JO_3D_CURRENT_MATRIX);
}

void                                        jo_3d_transform_point_by(const jo_fixed (* const m)[3], const jo_pos3D_fixed * const in,
                                                                     jo_pos3D_fixed * const out)
{
    jo_fixed                                columns[3][3];

    jo_3d_get_columns((jo_fixed (*)[3])m, columns);
    jo_3d_transform_vector(columns, (const jo_fixed *)in, (jo_fixed *)out);
    out->x += m[3][0];
    out->y += m[3][1];
    out->z += m[3][2];
}

void                                        jo_3d_transform_point(const jo_pos3D_fixed * const in, jo_pos3D_fixed *out)
{
    jo_fixed                                (*m)[3];
    jo_fixed                                columns[3][3];

    m = JO_3D_CURRENT_MATRIX;
    jo_3d_get_columns(m, columns);
    jo_3d_transform_vector(columns, (const jo_fixed *)in, (jo_fixed *)out);
    out->x += m[3][0];
    out->y += m[3][1];
    out->z += m[3][2];
}

void                                        jo_3d_get_matrix(jo_fixed *m)
{
    // (12 words: copied here, not by the call of jo_memcpy() (each bone of a skeleton, see jo_3d_model_pose_bone()))
    jo_3d_copy_matrix((jo_fixed (*)[3])m, (const jo_fixed (*)[3])JO_3D_CURRENT_MATRIX);
}

static void                                 jo_3d_multiply_current_matrix(jo_3d_matrix m);

void                                        jo_3d_reflect_matrix(const jo_pos3D_fixed * const normal, const jo_fixed distance)
{
    jo_3d_matrix                            r;
    jo_fixed                                columns[3][3];
    jo_fixed                                offset[3];
    const jo_fixed                          *n;
    int                                     i;
    int                                     j;

    // Reflection through the plane n.p = d: p' = p - 2 (n.p - d) n, the matrix I - 2 n n^T and the translation 2 d n
    n = (const jo_fixed *)normal;
    for (i = 0; i < 3; ++i)
    {
        for (j = 0; j < 3; ++j)
            r[i][j] = (i == j ? JO_FIXED_1 : 0) - 2 * jo_fixed_mult(n[i], n[j]);
        r[3][i] = jo_fixed_mult(distance, n[i]);
    }
    // (the translation d n added twice in the view space: 2 d n overflowed for a plane at 16384 units or more from the
    // origin, the reflection of a mirror of a big level was at a wrong place. The camera is near the plane: its
    // translation added first, each half fits)
    jo_3d_get_columns(JO_3D_CURRENT_MATRIX, columns);
    jo_3d_transform_vector(columns, r[3], offset);
    jo_3d_multiply_current_matrix(r);
    JO_3D_CURRENT_MATRIX[3][0] += offset[0];
    JO_3D_CURRENT_MATRIX[3][1] += offset[1];
    JO_3D_CURRENT_MATRIX[3][2] += offset[2];
}

void                                        jo_3d_set_mirror(const bool mirror)
{
    __jo_3d_master.mirror = mirror;
}

/** @brief Multiply the current matrix by m (current = current * m)
 *  @param m Matrix
 */
static void                                 jo_3d_multiply_current_matrix(jo_3d_matrix m)
{
    jo_fixed                                (*c)[3];
    jo_fixed                                columns[3][3];
    jo_fixed                                translation[3];
    register int                            row;

    c = JO_3D_CURRENT_MATRIX;
    // The axes are copied, so the current matrix can be overwritten row by row
    jo_3d_get_columns(c, columns);
    translation[0] = c[3][0];
    translation[1] = c[3][1];
    translation[2] = c[3][2];
    for (JO_ZERO(row); row < 4; ++row)
        jo_3d_transform_vector(columns, m[row], c[row]);
    c[3][0] += translation[0];
    c[3][1] += translation[1];
    c[3][2] += translation[2];
}

void                                        __jo_3d_look_at(const jo_fixed * const camera, const jo_fixed * const target, const jo_angle angle)
{
    jo_pos3D_fixed                          f;
    jo_pos3D_fixed                          r;
    jo_pos3D_fixed                          u;
    jo_pos3D_fixed                          up;
    jo_3d_matrix                            v;
    jo_3d_matrix                            roll;
    jo_fixed                                s;
    jo_fixed                                c;

    f.x = target[0] - camera[0];
    f.y = target[1] - camera[1];
    f.z = target[2] - camera[2];
    jo_3d_normalize(&f);
    // World "up" is -Y (Y goes down)
    up.x = JO_FIXED_0;
    up.y = -JO_FIXED_1;
    up.z = JO_FIXED_0;
    if (f.x == 0 && f.z == 0)
    {
        up.y = JO_FIXED_0;
        up.z = JO_FIXED_1;
    }
    // r = f x up
    r.x = jo_fixed_mult(f.y, up.z) - jo_fixed_mult(f.z, up.y);
    r.y = jo_fixed_mult(f.z, up.x) - jo_fixed_mult(f.x, up.z);
    r.z = jo_fixed_mult(f.x, up.y) - jo_fixed_mult(f.y, up.x);
    jo_3d_normalize(&r);
    // u = f x r
    u.x = jo_fixed_mult(f.y, r.z) - jo_fixed_mult(f.z, r.y);
    u.y = jo_fixed_mult(f.z, r.x) - jo_fixed_mult(f.x, r.z);
    u.z = jo_fixed_mult(f.x, r.y) - jo_fixed_mult(f.y, r.x);
    // View matrix: rows r, u, f
    v[0][0] = r.x;
    v[0][1] = u.x;
    v[0][2] = f.x;
    v[1][0] = r.y;
    v[1][1] = u.y;
    v[1][2] = f.y;
    v[2][0] = r.z;
    v[2][1] = u.z;
    v[2][2] = f.z;
    v[3][0] = -jo_fixed_dot((const jo_fixed *)&r, camera);
    v[3][1] = -jo_fixed_dot((const jo_fixed *)&u, camera);
    v[3][2] = -jo_fixed_dot((const jo_fixed *)&f, camera);
    if (angle)
    {
        // Roll around the view axis
        jo_3d_set_identity(roll);
        s = jo_sin_angle(angle);
        c = jo_cos_angle(angle);
        roll[0][0] = c;
        roll[0][1] = s;
        roll[1][0] = -s;
        roll[1][1] = c;
        jo_3d_multiply_current_matrix(roll);
    }
    jo_3d_multiply_current_matrix(v);
}

/*
** Projection & light
*/

/** @brief The angle of view (see __jo_3d_set_screen_mode()) */
static jo_angle                             __jo_3d_perspective_angle = JO_DEG_TO_ANGLE(JO_3D_DEFAULT_PERSPECTIVE_ANGLE);

void                                        __jo_3d_set_perspective(const jo_angle angle)
{
    jo_fixed                                s;
    jo_fixed                                c;

    __jo_3d_perspective_angle = angle;
    s = jo_sin_angle((jo_angle)(angle >> 1));
    c = jo_cos_angle((jo_angle)(angle >> 1));
    if (s <= 0)
        s = 1;
    // Distance between the eye and the screen: (screen width / 2) / tan(angle / 2)
    // (the divider of the CPU instead of a 64 bits division: the same quotient, (width / 2) x c < 2^31)
    __jo_vdp1_projection_distance = jo_fixed_div(__jo_screen.width_2 * c, s);
    __jo_vdp1_update_near_clip();
}

void                                        __jo_3d_set_screen_mode(void)
{
    __jo_3d_set_perspective(__jo_3d_perspective_angle);
}

void                                        __jo_3d_light(const jo_pos3D_fixed * const light)
{
    jo_fixed                                columns[3][3];

    // Light direction in view space. The vector is not normalized (it should be a unit vector)
    jo_3d_get_columns(JO_3D_CURRENT_MATRIX, columns);
    jo_3d_transform_vector(columns, (const jo_fixed *)light, (jo_fixed *)&__jo_3d_light_direction);
}

/*
** Polygons
*/

/** @brief Lighting context for the polygons of one object (computed once by __jo_3d_draw()) */
typedef struct
{
    bool                                    ready;
    bool                                    uniform_scale;
    jo_pos3D_fixed                          light;          // Light direction in model space (scaled)
    jo_fixed                                inverse_scale;  // 1 / scale of the matrix
    jo_fixed                                columns[3][3];  // Columns of the matrix (see jo_3d_get_columns())
    jo_fixed                                (*matrix)[3];   // Matrix of the object
    const jo_pos3D_fixed                    *direction;     // Light direction in view space (see jo_3d_cpu_context)
}                                           jo_3d_light_context;

/** @brief Prepare the lighting of an object
 *  @param context Output
 *  @remarks n' = M n, so n'.l = n.(M^T l): the light is transformed once in model space.
 *  @remarks When the matrix has a uniform scale s, |n'| = s |n| so there is no normalization per polygon.
 */
static __attribute__((noinline)) void       jo_3d_prepare_light(jo_3d_light_context *context, jo_fixed (* const matrix)[3])
{
    jo_fixed                                (*m)[3];
    long long                               s0;
    long long                               s1;
    long long                               s2;
    long long                               d01;
    long long                               d02;
    long long                               d12;
    long long                               tolerance;
    long long                               c[3][3];
    long long                               biggest;
    jo_fixed                                scale;
    int                                     i;
    int                                     j;

    m = matrix;
    context->ready = true;
    jo_3d_get_columns(m, context->columns);
    s0 = jo_3d_squared_length((const jo_pos3D_fixed *)m[0]);
    s1 = jo_3d_squared_length((const jo_pos3D_fixed *)m[1]);
    s2 = jo_3d_squared_length((const jo_pos3D_fixed *)m[2]);
    d01 = ((long long)m[0][0] * m[1][0] + (long long)m[0][1] * m[1][1] + (long long)m[0][2] * m[1][2]) >> 16;
    d02 = ((long long)m[0][0] * m[2][0] + (long long)m[0][1] * m[2][1] + (long long)m[0][2] * m[2][2]) >> 16;
    d12 = ((long long)m[1][0] * m[2][0] + (long long)m[1][1] * m[2][1] + (long long)m[1][2] * m[2][2]) >> 16;
    // (relative to the scale: 1/256 of |row 0|², the absolute tolerance of a scale of 1. A small flattened model (0.06,
    // 0.03, 0.06) passed as uniform, its normals were lit as the ones of a sphere)
    tolerance = JO_MAX(s0 >> 8, 1);
    context->uniform_scale = s0 > 0 && s0 <= JO_FIXED_MAX &&
                             JO_ABS(s1 - s0) <= tolerance && JO_ABS(s2 - s0) <= tolerance &&
                             JO_ABS(d01) <= tolerance && JO_ABS(d02) <= tolerance && JO_ABS(d12) <= tolerance;
    if (!context->uniform_scale)
    {
        // A scale that is not uniform: a normal is transformed by the inverse of the transposed matrix, not by the matrix
        // (a slope of a model flattened 4 times was lit 12 levels off). The cofactors (rows 1 x 2, 2 x 0, 0 x 1): the same
        // directions, the length doesn't matter (normalized after), the sign of the determinant kept
        // (the products kept whole (Q32): shifted >> 16, the cofactors of a small model scaled by (0.003, 0.002, 0.003) were
        // all 0, its polygons were not lit; below 0.06 their directions were rounded)
        for (JO_ZERO(i); i < 3; ++i)
        {
            const jo_fixed *a = m[(i + 1) % 3];
            const jo_fixed *b = m[(i + 2) % 3];

            c[i][0] = (long long)a[1] * b[2] - (long long)a[2] * b[1];
            c[i][1] = (long long)a[2] * b[0] - (long long)a[0] * b[2];
            c[i][2] = (long long)a[0] * b[1] - (long long)a[1] * b[0];
        }
        // (in 32 bits: halved while too big, the directions don't change)
        for (biggest = 0, i = 0; i < 3; ++i)
            for (JO_ZERO(j); j < 3; ++j)
                biggest = JO_MAX(biggest, JO_ABS(c[i][j]));
        for (; biggest >= (1 << 24); biggest >>= 1)
            for (JO_ZERO(i); i < 3; ++i)
                for (JO_ZERO(j); j < 3; ++j)
                    c[i][j] >>= 1;
        // (after: the products of the 64 bits cofactors overflowed)
        if ((c[0][0] * m[0][0] + c[0][1] * m[0][1] + c[0][2] * m[0][2]) < 0)
            for (JO_ZERO(i); i < 3; ++i)
                for (JO_ZERO(j); j < 3; ++j)
                    c[i][j] = -c[i][j];
        // (the columns: n' = n.x c[0] + n.y c[1] + n.z c[2], like the rows of the matrix with jo_3d_transform_vector())
        for (JO_ZERO(i); i < 3; ++i)
            for (JO_ZERO(j); j < 3; ++j)
                context->columns[j][i] = (jo_fixed)c[i][j];
        return;
    }
    scale = jo_fixed_sqrt((jo_fixed)s0);
    if (scale == 0)
    {
        context->uniform_scale = false;
        return;
    }
    context->inverse_scale = jo_fixed_div(JO_FIXED_1, scale);
    context->light.x = jo_fixed_dot(m[0], (const jo_fixed *)context->direction);
    context->light.y = jo_fixed_dot(m[1], (const jo_fixed *)context->direction);
    context->light.z = jo_fixed_dot(m[2], (const jo_fixed *)context->direction);
}

/** @brief Lighting: minimum length of a normal that is not normalized for the uniform scale path (see jo_3d_light_intensity_generic()): 1/16 (Q16.16) */
# define JO_3D_LIGHT_MIN_NORMAL_LENGTH      (4096)

/** @brief Brightness of a polygon when the matrix has no uniform scale or the normal is not a unit vector
 *  @param context Lighting context of the object
 *  @param normal Normal of the polygon (model space)
 *  @return Cosine between the view space normal and the light direction (Q16.16)
 *  @remarks Not inlined: the fast path of jo_3d_apply_light() needs no stack
 *  @remarks Uniform scale: (n . l) / (scale |n|), a square root and a division (the normals of the models exported by Jo
 *           Map Editor are not unit vectors: 117 of the 192 normals of a rock model). The same result as
 *           the normal transformed and normalized within 0.0006 (0.0025 for the normal transformed), measured on PC. The very
 *           short or very long normals are transformed and normalized
 */
static __attribute__((noinline)) jo_fixed   jo_3d_light_intensity_generic(jo_3d_light_context * const context, const jo_pos3D_fixed * const normal)
{
    jo_pos3D_fixed                          n;
    long long                               len2;
    jo_fixed                                len;

    if (context->uniform_scale)
    {
        len2 = jo_3d_squared_length(normal);
        if (len2 > 0 && len2 <= JO_FIXED_MAX && (len = jo_fixed_sqrt((jo_fixed)len2)) >= JO_3D_LIGHT_MIN_NORMAL_LENGTH)
            return (jo_fixed_div(jo_fixed_mult(jo_fixed_dot((const jo_fixed *)normal, (const jo_fixed *)&context->light),
                                               context->inverse_scale), len));
    }
    jo_3d_transform_vector(context->columns, (const jo_fixed *)normal, (jo_fixed *)&n);
    jo_3d_normalize(&n);
    return (jo_fixed_dot((const jo_fixed *)&n, (const jo_fixed *)context->direction));
}

/** @brief Check if a normal is a unit vector: |(|n|² >> 16) - 1| <= JO_3D_UNIFORM_SCALE_TOLERANCE (Q16.16)
 *  @param normal Normal
 *  @return true if it is
 *  @remarks |n|² summed on 64 bits by the MAC unit: |n|² in [(1 - t) << 16, ((1 + t) << 16) + 0xFFFF], so
 *           |n|² + 2^24 - 2^32 in [0, 0x0201FFFF] (t = 0x100). Same result as jo_3d_squared_length()
 */
static  __jo_force_inline bool              jo_3d_is_unit_normal(const jo_pos3D_fixed * const normal)
{
    const jo_fixed                          *a;
    const jo_fixed                          *b;
    unsigned int                            high;
    unsigned int                            low;

    a = (const jo_fixed *)normal;
    b = a;
    __asm__ ("clrmac\n\t"
             "mac.l @%2+, @%3+\n\t"
             "mac.l @%2+, @%3+\n\t"
             "mac.l @%2+, @%3+\n\t"
             "sts mach, %0\n\t"
             "sts macl, %1"
             : "=r" (high), "=r" (low), "+r" (a), "+r" (b)
             :
             : "mach", "macl", "memory");
    low += 0x01000000u;
    high += low < 0x01000000u;
    return (high == 1 && low <= 0x0201FFFFu);
}

/** @brief Color component lit: jo_3d_light_clamp[component + level] = component + level - 16, between 0 and 31 */
static const unsigned char                  jo_3d_light_clamp[JO_3D_RGB_COMPONENT_MASK + JO_3D_LIGHT_MAX_LEVEL + 1] =
{
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31
};

/** @brief Shades of the palette lighting (see jo_3d_set_palette_light()): 0 = off */
static int                                  __jo_3d_palette_light_levels = 0;
/** @brief Color codes between 2 shades of a color (see jo_3d_set_palette_light()) */
static int                                  __jo_3d_palette_light_stride = 0;

void                                        jo_3d_set_palette_light(const int levels, const int stride)
{
    __jo_3d_palette_light_levels = levels > 0 && levels <= JO_3D_LIGHT_MAX_LEVEL + 1 ? levels : 0;
    __jo_3d_palette_light_stride = stride;
}

/** @brief Apply the lighting to a VDP1 command
 *  @param cmd VDP1 command
 *  @param context Lighting context of the object
 *  @param polygon Polygon
 *  @param attribute Polygon attribute
 *  @remarks Fast path (uniform scale, unit normal): cosine = (n . l) / scale, l = the light in model space (see jo_3d_prepare_light())
 */
static  __jo_force_inline void              jo_3d_apply_light(jo_vdp1_command *cmd, jo_3d_light_context *context,
                                                              const jo_3d_polygon * const polygon, const jo_3d_polygon_attribute * const attribute)
{
    const unsigned char                     *clamp;
    jo_fixed                                d;
    int                                     level;
    int                                     color;
    int                                     mode;

    if (!context->ready)
        jo_3d_prepare_light(context, context->matrix);
    if (context->uniform_scale && jo_3d_is_unit_normal(&polygon->normal))
        d = jo_fixed_mult(jo_fixed_dot((const jo_fixed *)&polygon->normal, (const jo_fixed *)&context->light), context->inverse_scale);
    else
        d = jo_3d_light_intensity_generic(context, &polygon->normal);
    // Facing the light: brighter (up to +15), facing away: darker (down to -16)
    if (d > JO_FIXED_1)
        d = JO_FIXED_1;
    else if (d < -JO_FIXED_1)
        d = -JO_FIXED_1;
    // Textured polygons are lit on both sides
    if ((attribute->sort & JO_3D_USE_TEXTURE) && d > 0)
        d = -d;
    // d in [-1, 1]: (-16 d +/- 1) >> 16 in [-16, 15], so level in [0, JO_3D_LIGHT_MAX_LEVEL]
    level = JO_3D_LIGHT_NEUTRAL_LEVEL;
    if (d)
        level += (-JO_MULT_BY_16(d) + (d > 0 ? 1 : -1)) >> 16;
    if (__jo_3d_palette_light_levels && (attribute->sort & JO_3D_USE_TEXTURE ?
        (attribute->draw_mode & JO_VDP1_COLOR_MODE_MASK) == JO_VDP1_COLOR_MODE_16_BANK : !(attribute->color & JO_COLOR_SATURN_MSB)))
    {
        cmd->colr = (unsigned short)(attribute->color + ((level * __jo_3d_palette_light_levels) >> 5) * __jo_3d_palette_light_stride);
        return;
    }
    if (attribute->sort & JO_3D_USE_TEXTURE)
    {
        // Gouraud shading with the same brightness for the 4 vertices (RGB frame buffer only)
        if (__jo_screen.hires)
            return;
        // Gouraud with half transparency (7) or half luminance (6) kept, the shadow (1) is not lit (1 + gouraud is
        // prohibited)
        mode = cmd->pmod & JO_VDP1_COLOR_CALC_MASK;
        if (mode == 1)
            return;
        // (the half transparency or luminance of a gouraud polygon too: 7 and 6 became 4)
        mode &= 3;
        cmd->pmod = (cmd->pmod & ~JO_VDP1_COLOR_CALC_MASK) | JO_VDP1_COLOR_CALC_GOURAUD | (mode == 2 || mode == 3 ? mode : 0);
        cmd->grda = JO_DIV_BY_8(JO_VDP1_LIGHT_TABLE_OFFSET) + level;
        return;
    }
    // A palette color without the palette lighting (jo_3d_set_palette_light()): not lit (its index is not a RGB color)
    if (!(attribute->color & JO_COLOR_SATURN_MSB))
        return;
    // Untextured polygon: change the color itself (each component + level - 16, between 0 and 31)
    clamp = jo_3d_light_clamp + level;
    color = attribute->color;
    cmd->colr = (color & JO_COLOR_SATURN_MSB) |
                ((int)clamp[JO_COLOR_SATURN_GET_B(color)] << 10) |
                ((int)clamp[JO_COLOR_SATURN_GET_G(color)] << 5) |
                clamp[JO_COLOR_SATURN_GET_R(color)];
}

/** @brief Check if a polygon is lit by jo_3d_apply_light() (the light changes its color or its gouraud table)
 *  @param attribute Polygon attribute
 *  @return true if it is lit
 */
static  __jo_force_inline bool              jo_3d_is_lit(const jo_3d_polygon_attribute * const attribute)
{
    return ((attribute->sort & JO_3D_USE_LIGHT) && !(attribute->sort & JO_3D_USE_PALETTE) &&
            ((attribute->draw_mode & JO_VDP1_COLOR_MODE_MASK) == JO_VDP1_COLOR_MODE_RGB ||
             (attribute->draw_mode & JO_VDP1_COLOR_MODE_MASK) == JO_VDP1_COLOR_MODE_16_LOOKUP ||
             (__jo_3d_palette_light_levels && (attribute->draw_mode & JO_VDP1_COLOR_MODE_MASK) == JO_VDP1_COLOR_MODE_16_BANK)));
}

/** @brief Outcode of a vertex: position against the screen, the guard band and the near plane
 *  @param x Screen position
 *  @param y Screen position
 *  @param right Right of the screen (system clipping)
 *  @param bottom Bottom of the screen (system clipping)
 *  @param guard Guard band (pixels, see jo_3d_set_guard_band())
 *  @return JO_3D_CODE_xxx flags
 */
static  __jo_force_inline unsigned int      jo_3d_screen_code(const int x, const int y, const int right, const int bottom, const int guard)
{
    unsigned int                            code;

    JO_ZERO(code);
    // One unsigned comparison for each axis when the vertex is on the screen (most common case)
    if ((unsigned int)x > (unsigned int)right)
    {
        if (x < 0)
            code = x < -guard ? JO_3D_CODE_LEFT | JO_3D_CODE_GUARD : JO_3D_CODE_LEFT;
        else
            code = x > right + guard ? JO_3D_CODE_RIGHT | JO_3D_CODE_GUARD : JO_3D_CODE_RIGHT;
    }
    if ((unsigned int)y > (unsigned int)bottom)
    {
        if (y < 0)
            code |= y < -guard ? JO_3D_CODE_TOP | JO_3D_CODE_GUARD : JO_3D_CODE_TOP;
        else
            code |= y > bottom + guard ? JO_3D_CODE_BOTTOM | JO_3D_CODE_GUARD : JO_3D_CODE_BOTTOM;
    }
    return (code);
}

/** @brief Check if a vertex is very far outside of the screen: |x| or |y| > z * 2^JO_3D_FAR_OUTSIDE_SHIFT (its projection could overflow)
 *  @remarks When z >= 2^(31 - JO_3D_FAR_OUTSIDE_SHIFT - 1), x and y (32767 units at most) can't be so far
 */
static  __jo_force_inline bool              jo_3d_is_far_outside(const jo_fixed x, const jo_fixed y, const jo_fixed z)
{
    unsigned int                            limit;

    if (z >= (1 << (31 - JO_3D_FAR_OUTSIDE_SHIFT - 1)))
        return (false);
    limit = (unsigned int)z << JO_3D_FAR_OUTSIDE_SHIFT;
    // |x| > limit <=> x + limit is not in [0, 2 * limit]
    return ((unsigned int)(x + limit) > JO_MULT_BY_2(limit) || (unsigned int)(y + limit) > JO_MULT_BY_2(limit));
}

/** @brief The sides of the view volume a point of the view space is outside of (the planes through the camera and the edges
 *  of the screen: JO_3D_CODE_LEFT...), behind the near plane or very far outside of the screen
 *  @param pd Projection distance (pixels, Q16.16)
 *  @remarks 64 bits products: in 32 bits (x x pixels, z x pixels) they overflowed at 186 units from the camera (a long
 *  wall next to the camera: its vertices behind it got wrong sides, the polygon was rejected, a piece of it missing)
 */
static  __jo_force_inline unsigned int      jo_3d_side_code(const jo_fixed x, const jo_fixed y, const jo_fixed z, const int cx,
                                                            const int cy, const int right, const int bottom, const jo_fixed pd)
{
    long long                               px;
    long long                               py;
    unsigned int                            code;

    px = (long long)x * pd;
    py = (long long)y * pd;
    // (z x 65536 x pixels: one 32 x 32 bits product each, the pixels x 65536 fit in 32 bits)
    JO_ZERO(code);
    if (px + (long long)z * JO_MULT_BY_65536(cx) < 0)
        code |= JO_3D_CODE_LEFT;
    if ((long long)z * JO_MULT_BY_65536(right - cx) - px < 0)
        code |= JO_3D_CODE_RIGHT;
    if (py + (long long)z * JO_MULT_BY_65536(cy) < 0)
        code |= JO_3D_CODE_TOP;
    if ((long long)z * JO_MULT_BY_65536(bottom - cy) - py < 0)
        code |= JO_3D_CODE_BOTTOM;
    return (code);
}

/*
** Transformation of the vertices by the SCU DSP (see jo_3d_enable_dsp()): the DSP computes the 3 x 3 part of the matrix
** for a batch of vertices while the CPU projects the previous batch with the assembly loop (__jo_3d_project_loop(): it adds
** the translation). The DSP reads the vertices in the high work RAM by DMA and writes the results in 2 buffers of the
** high work RAM, used in turn.
** The results are the same as the CPU (48 bits products, bits 47 to 16 kept).
*/

/** @brief Vertices of a batch of the DSP (bank 1: 3 words per vertex, 63 words at most) */
# define JO_3D_DSP_BATCH                    (21)
/** @brief Data RAM words of the parameters of a batch (bank 0, see jo_engine/dsp/xform.dsp) */
# define JO_3D_DSP_PARAMS                   (60)
/** @brief Vertex count under which the CPU transforms the vertices (the DSP has an overhead) */
# define JO_3D_DSP_MIN_VERTICES             (12)
/** @brief CPU using the DSP: -1 none, 0 master CPU, 1 slave CPU */
static int                                  __jo_3d_dsp_cpu = -1;
/** @brief Words of a buffer of results: a multiple of the cache line (each buffer starts on a line: its lines are purged
 *  from its start, see jo_3d_transform_vertices_dsp(). With 63 words, the buffer 1 started 4 bytes before a line: the
 *  purge missed its last line and old results were projected, a far vertex of a terrain for example) */
# define JO_3D_DSP_RESULT_WORDS             (((JO_3D_DSP_BATCH * 3 * sizeof(jo_fixed) + JO_SH2_CACHE_LINE_SIZE - 1) & ~(JO_SH2_CACHE_LINE_SIZE - 1)) / sizeof(jo_fixed))
static jo_fixed                             __jo_3d_dsp_results[2][JO_3D_DSP_RESULT_WORDS] __attribute__((aligned(16))) JO_WORK_RAM_ENGINE_WORK_SECTION;

void                                        jo_3d_enable_dsp(const bool slave_cpu)
{
    unsigned int                            table[2 * JO_3D_DSP_BATCH];
    register unsigned int                   i;

    jo_dsp_load_program(dsp_xform, sizeof(dsp_xform) / sizeof(*dsp_xform));
    // Bank 2: first word of each vertex in the bank 1, twice (see jo_engine/dsp/xform.dsp)
    for (JO_ZERO(i); i < JO_3D_DSP_BATCH; ++i)
        table[2 * i] = table[2 * i + 1] = 3 * i;
    jo_dsp_write_data(2, 0, table, 2 * JO_3D_DSP_BATCH);
    __jo_3d_dsp_cpu = slave_cpu ? 1 : 0;
}

void                                        jo_3d_disable_dsp(void)
{
    __jo_3d_dsp_cpu = -1;
}

/** @brief Start the transformation of a batch of vertices by the DSP
 *  @param in Vertices (high work RAM)
 *  @param out Results
 *  @param count Vertex count (JO_3D_DSP_BATCH at most)
 */
static  __jo_force_inline void              jo_3d_dsp_start_batch(const jo_pos3D_fixed * const in, jo_fixed * const out, const unsigned int count)
{
    unsigned int                            params[4];

    params[0] = ((unsigned int)in & ~JO_CACHE_THROUGH(0)) >> 2;
    params[1] = ((unsigned int)out & ~JO_CACHE_THROUGH(0)) >> 2;
    params[2] = count - 1;
    params[3] = 3 * count;
    jo_dsp_write_data(0, JO_3D_DSP_PARAMS, params, 4);
    jo_dsp_start(0);
}

/** @brief Check if the DSP can read an address (high work RAM, cached or cache-through)
 *  @param ptr Address
 *  @return true if the DSP can read it by DMA
 */
static  __jo_force_inline bool              jo_3d_dsp_can_read(const void * const ptr)
{
    return ((((unsigned int)ptr) & 0xDFF00000) == JO_WORK_RAM_HIGH);
}

/*
** Transformation and projection of the vertices in assembly (the common path: a vertex in front of the near plane and
** not very far outside of the screen).
**
** Registers of the loop: r8 input vertex, r9 output vertex, r10 columns of the matrix, r14 parameters,
** r11/r12 view space x/y of the pending vertex (its division runs), r13 pending output vertex, r7 division unit.
** The pending vertex is finished after the multiplications of the next vertex (the division unit needs 39 cycles).
** A vertex of the slow path (near or far outside) is projected by __jo_3d_project_slow() and the pending vertex
** becomes a scratch vertex of the stack frame (the stale quotient is written there).
*/

/** @brief Parameters of __jo_3d_transform_loop() (the offsets are used by the assembly) */
typedef struct
{
    jo_fixed                                tx;         // 0: translation
    jo_fixed                                ty;         // 4
    jo_fixed                                tz;         // 8
    jo_fixed                                near_z;     // 12
    jo_fixed                                pd_high;    // 16: projection distance << 16, as a 64 bits dividend
    jo_fixed                                pd_low;     // 20
    int                                     cx;         // 24
    int                                     cy;         // 28
    int                                     right;      // 32
    int                                     bottom;     // 36
    int                                     guard;      // 40
    const jo_pos3D_fixed                    *end;       // 44: end of the input vertices
    int                                     far;        // 48: screen position beyond which a vertex is very far outside
    int                                     right_guard;// 52: right + guard
    int                                     bottom_guard;// 56: bottom + guard
    int                                     view_delta; // 60: bytes from a vertex to its view space position
    jo_fixed                                pd;         // projection distance (the slow path)
}                                           jo_3d_transform_params;

/** @brief Project a vertex behind the near plane
 *  @param k Parameters
 *  @param v Vertex (output)
 *  @param x View space position
 *  @param y View space position
 *  @param z View space depth
 *  @remarks Called by the assembly loop: not static (not inlined, kept by the link time optimization)
 */
void __attribute__((noinline, used, externally_visible)) __jo_3d_project_slow(const jo_3d_transform_params * const k, jo_3d_vertex * const v,
                                                                 const jo_fixed x, const jo_fixed y, const jo_fixed z)
{
    jo_3d_view_position                     *view;

    view = (jo_3d_view_position *)((char *)v + k->view_delta);
    view->x = x;
    view->y = y;
    jo_3d_set_vertex(v, 0, 0, z, JO_3D_CODE_NEAR | JO_3D_CODE_VIEW | jo_3d_side_code(x, y, z, k->cx, k->cy, k->right, k->bottom, k->pd));
}

/** @brief Transform and project the vertices (assembly, see above)
 *  @param columns Columns of the matrix (see jo_3d_get_columns())
 *  @param in Vertices (model space)
 *  @param out Vertices (output)
 *  @param k Parameters (k->end: end of the input vertices, at least one vertex)
 */
extern void                                 __jo_3d_transform_loop(const jo_fixed columns[9], const jo_pos3D_fixed *in, jo_3d_vertex *out,
                                                                   const jo_3d_transform_params * const k);

/* FINISH: project the pending vertex (r11, r12, r7: view space x, y and z, r13: output): the quotient of the division
   unit, the screen position, its outcode (JO_3D_CODE_LEFT 1, RIGHT 2, TOP 4, BOTTOM 8, GUARD 16, see jo_3d_screen_code()),
   then the packed vertex (very far outside of the screen: the view space position in the parallel array, VIEW 64) */
__asm__(
"    .macro FINISH\n"
"    mov.l 7f, r6\n"                     /* division unit */
"    mov.l @(20, r6), r0\n"              /* DVDNTL: the quotient (the CPU waits for the end of the division) */
"    dmuls.l r11, r0\n"
"    mov.l @(24, r14), r6\n"             /* cx */
"    sts mach, r4\n"
"    dmuls.l r12, r0\n"
"    add r6, r4\n"
"    mov.l @(28, r14), r6\n"             /* cy */
"    sts mach, r5\n"
"    add r6, r5\n"
"    mov.l @(32, r14), r6\n"             /* right */
"    mov #0, r0\n"
"    cmp/hi r6, r4\n"                    /* outside horizontally (x < 0 or x > right) */
"    bf 1f\n"
"    mov.l @(40, r14), r6\n"             /* guard band */
"    cmp/pz r4\n"
"    bf 2f\n"
"    mov.l @(52, r14), r0\n"
"    cmp/gt r0, r4\n"                    /* x > right + guard: outside of the guard band */
"    movt r0\n"
"    shll2 r0\n"
"    shll2 r0\n"
"    bra 1f\n"
"    or #2, r0\n"
"2:  neg r6, r6\n"
"    cmp/gt r4, r6\n"                    /* x < -guard */
"    movt r0\n"
"    shll2 r0\n"
"    shll2 r0\n"
"    or #1, r0\n"
"1:  mov.l @(36, r14), r6\n"             /* bottom */
"    cmp/hi r6, r5\n"
"    bf 3f\n"
"    mov.l @(40, r14), r6\n"
"    cmp/pz r5\n"
"    bf 4f\n"
"    mov.l @(56, r14), r6\n"
"    cmp/gt r6, r5\n"                    /* y > bottom + guard */
"    movt r6\n"
"    shll2 r6\n"
"    shll2 r6\n"
"    or r6, r0\n"
"    bra 3f\n"
"    or #8, r0\n"
"4:  neg r6, r6\n"
"    cmp/gt r5, r6\n"
"    movt r6\n"
"    shll2 r6\n"
"    shll2 r6\n"
"    or r6, r0\n"
"    or #4, r0\n"
"3:  tst #16, r0\n"                     /* outside of the guard band: maybe very far (|x| or |y| > the far limit) */
"    bt 11f\n"
"    mov.l @(48, r14), r6\n"             /* far limit */
"    cmp/gt r6, r4\n"
"    bt 13f\n"
"    neg r6, r6\n"
"    cmp/gt r4, r6\n"
"    bt 13f\n"
"    neg r6, r6\n"
"    cmp/gt r6, r5\n"
"    bt 13f\n"
"    neg r6, r6\n"
"    cmp/gt r5, r6\n"
"    bf 11f\n"
"13: mov.l @(60, r14), r6\n"             /* very far outside: the view space position in the parallel array */
"    add r13, r6\n"
"    mov.l r11, @r6\n"
"    mov.l r12, @(4, r6)\n"
"    or #64, r0\n"
"11: shll16 r4\n"                        /* the packed vertex */
"    extu.w r5, r5\n"
"    or r5, r4\n"
"    mov.l r4, @r13\n"
"    mov #-128, r6\n"
"    and r6, r7\n"
"    or r0, r7\n"
"    mov.l r7, @(4, r13)\n"
"    .endm\n"
"    .macro TRANSFORM_LOOP name, rotate\n"
"    .text\n"
"    .align 2\n"
"    .global \\name\n"
"\\name:\n"
"    mov.l r8, @-r15\n"
"    mov.l r9, @-r15\n"
"    mov.l r10, @-r15\n"
"    mov.l r11, @-r15\n"
"    mov.l r12, @-r15\n"
"    mov.l r13, @-r15\n"
"    mov.l r14, @-r15\n"
"    sts.l pr, @-r15\n"
"    add #-16, r15\n"                    /* scratch vertex and view position */
"    mov r4, r10\n"
"    mov r5, r8\n"
"    mov r6, r9\n"
"    mov r7, r14\n"
"    mov #0, r11\n"
"    mov #0, r12\n"
"    mov #0, r7\n"
"    mov r15, r13\n"
"5:\n"
"    .if \\rotate\n"
     /* x, y, z = columns . vertex (bits 47 to 16 of the 64 bits sums) */
"    clrmac\n"
"    mac.l @r8+, @r10+\n"
"    mac.l @r8+, @r10+\n"
"    mac.l @r8+, @r10+\n"
"    add #-12, r8\n"
"    sts mach, r0\n"
"    sts macl, r1\n"
"    clrmac\n"
"    mac.l @r8+, @r10+\n"
"    mac.l @r8+, @r10+\n"
"    mac.l @r8+, @r10+\n"
"    xtrct r0, r1\n"
"    add #-12, r8\n"
"    sts mach, r0\n"
"    sts macl, r2\n"
"    clrmac\n"
"    mac.l @r8+, @r10+\n"
"    mac.l @r8+, @r10+\n"
"    mac.l @r8+, @r10+\n"
"    xtrct r0, r2\n"
"    add #-36, r10\n"
"    sts mach, r0\n"
"    sts macl, r3\n"
"    xtrct r0, r3\n"
"    .else\n"
     /* x, y, z: the vertex rotated by the DSP (r10 unused) */
"    mov.l @r8+, r1\n"
"    mov.l @r8+, r2\n"
"    mov.l @r8+, r3\n"
"    .endif\n"
"    mov.l @(0, r14), r0\n"
"    add r0, r1\n"
"    mov.l @(4, r14), r0\n"
"    add r0, r2\n"
"    mov.l @(8, r14), r0\n"
"    add r0, r3\n"
"    FINISH\n"
     /* behind the near plane: the slow path */
"    mov.l @(12, r14), r0\n"
"    cmp/ge r3, r0\n"
"    bt 8f\n"
     /* k = pd / z in the background: the vertex is pending */
"    mov.l 7f, r6\n"
"    mov.l r3, @(0, r6)\n"               /* DVSR */
"    mov.l @(16, r14), r0\n"
"    mov.l r0, @(16, r6)\n"              /* DVDNTH */
"    mov.l @(20, r14), r0\n"
"    mov.l r0, @(20, r6)\n"              /* DVDNTL: starts the division */
"    mov r1, r11\n"
"    mov r2, r12\n"
"    mov r3, r7\n"
"    mov r9, r13\n"
"9:\n"
"    add #8, r9\n"
"    mov.l @(44, r14), r0\n"
"    cmp/hs r0, r8\n"
"    bf 5b\n"
     /* the last pending vertex */
"    FINISH\n"
"    add #16, r15\n"
"    lds.l @r15+, pr\n"
"    mov.l @r15+, r14\n"
"    mov.l @r15+, r13\n"
"    mov.l @r15+, r12\n"
"    mov.l @r15+, r11\n"
"    mov.l @r15+, r10\n"
"    mov.l @r15+, r9\n"
"    rts\n"
"    mov.l @r15+, r8\n"
"8:\n"
     /* slow path: __jo_3d_project_slow(k, v, x, y, z), then the scratch vertex is pending */
"    mov r14, r4\n"
"    mov r9, r5\n"
"    mov r1, r6\n"
"    mov r2, r7\n"
"    mov.l 10f, r0\n"
"    jsr @r0\n"
"    mov.l r3, @-r15\n"
"    add #4, r15\n"
     /* (the scratch vertex is finished with the view space x and y of the vertex before: if it was very far outside, its
        view space position was written at the scratch vertex + view_delta, under the stack (the blocks of jo_malloc()
        there were broken). 0, 0: the center of the screen, never very far) */
"    mov #0, r11\n"
"    mov #0, r12\n"
"    bra 9b\n"
"    mov r15, r13\n"
"    .align 2\n"
"7:  .long 0xFFFFFF00\n"
"10: .long ___jo_3d_project_slow\n"
"    .endm\n"
"    TRANSFORM_LOOP ___jo_3d_transform_loop, 1\n"
"    TRANSFORM_LOOP ___jo_3d_project_loop, 0\n"
);

/** @brief Project vertices rotated by the DSP (assembly, see __jo_3d_transform_loop(): the same loop without the rotation)
 *  @param unused (the columns of __jo_3d_transform_loop())
 *  @param in Vertices rotated by the DSP (x, y, z, without the translation)
 *  @param out Vertices (output)
 *  @param k Parameters (k->end: end of the rotated vertices, at least one vertex)
 */
extern void                                 __jo_3d_project_loop(const jo_fixed *unused, const jo_fixed *in, jo_3d_vertex *out,
                                                                 const jo_3d_transform_params * const k);

/** @brief Transform the vertices with the DSP and project them (see jo_3d_transform_vertices()): the DSP rotates a batch
 *  while the CPU projects the previous one with the assembly loop (the results are the ones of the CPU)
 *  @param k Parameters of the assembly loop (k->end is set for each batch)
 */
static __attribute__((noinline)) void      jo_3d_transform_vertices_dsp(jo_3d_cpu_context * const ctx, const jo_pos3D_fixed *in, const unsigned int count,
                                                                         jo_3d_transform_params * const k)
{
    jo_fixed                                (*m)[3];
    unsigned int                            matrix[9];
    unsigned int                            first;
    unsigned int                            n;
    unsigned int                            next;
    unsigned int                            buffer;
    register unsigned int                   i;
    const jo_fixed                          *r;

    m = ctx->matrix;
    // Rows of the 3 x 3 part: output i = x * m[0][i] + y * m[1][i] + z * m[2][i]
    for (JO_ZERO(i); i < 3; ++i)
    {
        matrix[3 * i] = m[0][i];
        matrix[3 * i + 1] = m[1][i];
        matrix[3 * i + 2] = m[2][i];
    }
    // The DSP may still finish the last batch of the previous object
    jo_dsp_wait();
    jo_dsp_write_data(0, 0, matrix, 9);
    n = JO_MIN(count, JO_3D_DSP_BATCH);
    JO_ZERO(buffer);
    jo_3d_dsp_start_batch(in, __jo_3d_dsp_results[0], n);
    for (JO_ZERO(first); first < count; first = next, buffer ^= 1)
    {
        next = first + n;
        jo_dsp_wait();
        // The next batch is transformed while the CPU projects this one
        if (next < count)
            jo_3d_dsp_start_batch(in + next, __jo_3d_dsp_results[buffer ^ 1], JO_MIN(count - next, JO_3D_DSP_BATCH));
        // The DMA of the DSP wrote the memory: the cache may hold old values
        r = __jo_3d_dsp_results[buffer];
        for (JO_ZERO(i); i < n * 3 * sizeof(jo_fixed); i += JO_SH2_CACHE_LINE_SIZE)
            *(volatile unsigned int *)(JO_SH2_CACHE_PURGE_AREA | ((unsigned int)r + i)) = 0;
        k->end = (const jo_pos3D_fixed *)(r + 3 * n);
        __jo_3d_project_loop(JO_NULL, r, ctx->vertices + first, k);
        n = JO_MIN(count - next, JO_3D_DSP_BATCH);
    }
}

/** @brief Transform the vertices of an object in view space and project them on the screen
 *  @param in Vertices (model space)
 *  @param count Vertex count
 *  @param near_z Near clipping distance (the vertices nearer are not projected)
 *  @param cx Horizontal center of the projection (screen pixels)
 *  @param cy Vertical center of the projection (screen pixels)
 *  @param right Right of the screen (system clipping)
 *  @param bottom Bottom of the screen (system clipping)
 *  @remarks The perspective division of a vertex is computed by the division unit while the next vertex is transformed
 *  @remarks A vertex behind the near plane, or very far outside of the screen, keeps its view space position
 *  @remarks The DSP transforms the vertices when it is enabled for this CPU (see jo_3d_enable_dsp())
 */
static __attribute__((noinline)) void      jo_3d_transform_vertices(jo_3d_cpu_context * const ctx, const jo_pos3D_fixed *in, const unsigned int count,
                                                                     const jo_fixed near_z, const int cx, const int cy, const int right, const int bottom)
{
    jo_3d_transform_params                  k;
    jo_fixed                                (*m)[3];
    jo_fixed                                columns[3][3];
    jo_fixed                                pd;
    int                                     guard;

    if (count == 0)
        return;
    pd = __jo_vdp1_projection_distance;
    guard = *ctx->guard_band;
    k.near_z = near_z;
    k.pd = pd;
    k.pd_high = pd >> 16;
    k.pd_low = pd << 16;
    k.cx = cx;
    k.cy = cy;
    k.right = right;
    k.bottom = bottom;
    k.guard = guard;
    k.end = in + count;
    k.far = JO_3D_FAR_LIMIT;
    k.right_guard = right + guard;
    k.bottom_guard = bottom + guard;
    k.view_delta = (int)((char *)ctx->view - (char *)ctx->vertices);
    m = ctx->matrix;
    k.tx = m[3][0];
    k.ty = m[3][1];
    k.tz = m[3][2];
    if (__jo_3d_dsp_cpu == (ctx->slave ? 1 : 0) && count >= JO_3D_DSP_MIN_VERTICES && jo_3d_dsp_can_read(in))
    {
        jo_3d_transform_vertices_dsp(ctx, in, count, &k);
        return;
    }
    // Columns of the matrix are contiguous for jo_fixed_dot() (multiply and accumulate unit)
    jo_3d_get_columns(m, columns);
    __jo_3d_transform_loop((const jo_fixed *)columns, in, ctx->vertices, &k);
}

/** @brief Transform and project the vertices of the parts of an object, each one with its matrix (the bones of a
 *  skeleton, see jo_3d_transform_vertices()): the parameters of the assembly loop are set once, then the matrix and the
 *  vertices of each part (about 20 parts for a character: the setup of each call cost about a tenth)
 *  @param in Vertices of the object (model space)
 *  @param count Vertices of the object (the parts beyond are skipped)
 */
static __attribute__((noinline)) void      jo_3d_transform_parts(jo_3d_cpu_context * const ctx, const jo_pos3D_fixed * const in,
                                                                  const unsigned int count, const jo_fixed near_z, const int cx,
                                                                  const int cy, const int right, const int bottom)
{
    jo_3d_transform_params                  k;
    jo_fixed                                columns[3][3];
    jo_fixed                                (*m)[3];
    jo_3d_vertex                            *base;
    jo_3d_view_position                     *view;
    jo_fixed                                (*matrix)[3];
    const jo_3d_part                        *part;
    int                                     i;
    jo_fixed                                pd;
    int                                     guard;

    base = ctx->vertices;
    view = ctx->view;
    matrix = ctx->matrix;
    pd = __jo_vdp1_projection_distance;
    guard = *ctx->guard_band;
    k.near_z = near_z;
    k.pd = pd;
    k.pd_high = pd >> 16;
    k.pd_low = pd << 16;
    k.cx = cx;
    k.cy = cy;
    k.right = right;
    k.bottom = bottom;
    k.guard = guard;
    k.far = JO_3D_FAR_LIMIT;
    k.right_guard = right + guard;
    k.bottom_guard = bottom + guard;
    // (the vertices and their view space positions move together: the same distance for all the parts)
    k.view_delta = (int)((char *)view - (char *)base);
    for (JO_ZERO(i), part = ctx->parts; i < ctx->part_count; ++i, ++part)
    {
        if (part->first + part->count > count || part->count == 0)
            continue;
        ctx->vertices = base + part->first;
        ctx->view = view + part->first;
        // (the DSP: the general function)
        if (__jo_3d_dsp_cpu == (ctx->slave ? 1 : 0))
        {
            ctx->matrix = part->matrix;
            jo_3d_transform_vertices(ctx, in + part->first, part->count, near_z, cx, cy, right, bottom);
            continue;
        }
        m = part->matrix;
        jo_3d_get_columns(m, columns);
        k.tx = m[3][0];
        k.ty = m[3][1];
        k.tz = m[3][2];
        k.end = in + part->first + part->count;
        __jo_3d_transform_loop((const jo_fixed *)columns, in + part->first, ctx->vertices, &k);
    }
    ctx->vertices = base;
    ctx->view = view;
    ctx->matrix = matrix;
}

/** @brief Vertex of a polygon during the clipping (view space) */
typedef struct
{
    jo_fixed                                x;
    jo_fixed                                y;
    jo_fixed                                z;
    jo_fixed                                u;      // Position in the original polygon (0 to 1), to interpolate the gouraud shading
    jo_fixed                                v;
}                                           jo_3d_clip_vertex;

/** @brief Clipping context of an object (computed once by __jo_3d_draw(), when a polygon must be clipped) */
typedef struct
{
    bool                                    ready;
    jo_fixed                                inverse_pd;     // 1 / projection distance
    jo_fixed                                planes[4];      // Edges of the screen: left, right, top, bottom (|x| or |y| / z at the limit)
}                                           jo_3d_clip_context;

/** @brief Read the 4 colors of a gouraud table in the VDP1 VRAM: 2 32 bits reads (a read of the VRAM is slow, the table
 *  is 8 bytes aligned)
 *  @param table_address Gouraud table address / 8
 *  @param table The 4 colors (A, B, C, D)
 */
static __jo_force_inline void               jo_3d_read_gouraud_table(const unsigned short table_address, unsigned short table[4])
{
    const volatile unsigned int             *source;
    unsigned int                            ab;
    unsigned int                            cd;

    source = (const volatile unsigned int *)(JO_VDP1_VRAM + JO_MULT_BY_8(table_address));
    ab = source[0];
    cd = source[1];
    table[0] = (unsigned short)(ab >> 16);
    table[1] = (unsigned short)ab;
    table[2] = (unsigned short)(cd >> 16);
    table[3] = (unsigned short)cd;
}

/** @brief View space coordinate of a projected vertex: (screen - center) * z / projection distance
 *  @param d Screen position - center of the window (a short: |d| < 2^16)
 *  @param z Depth
 *  @param inverse_pd 1 / projection distance (see jo_3d_clip_context)
 *  @return d * z * inverse_pd >> 16, exactly
 *  @remarks d * inverse_pd fits in 32 bits when |inverse_pd| < 2^15 (projection distance > 2): one 32 x 32 -> 64 bits product
 *  (dmuls.l) instead of a 64 x 64 bits product, the same result (the exact product fits in 64 bits in both cases)
 */
static __jo_force_inline jo_fixed           jo_3d_unproject(const int d, const jo_fixed z, const jo_fixed inverse_pd)
{
    if (inverse_pd < 32768 && inverse_pd > -32768)
        return ((jo_fixed)(((long long)(d * inverse_pd) * z) >> 16));
    return ((jo_fixed)((((long long)d * z) * inverse_pd) >> 16));
}

/** @brief View space position of a vertex of the current object (a polygon clipped or drawn in parts): kept for the vertices
 *  behind the near plane or very far outside of the screen (JO_3D_CODE_VIEW), the others transformed again from the points
 *  of the object (taken back from their rounded screen positions, the error was half a pixel x the depth / the projection
 *  distance: 1.5 units for a corner 500 units away, 12 pixels where the polygon is cut by the near plane; the line of the
 *  cut of a big floor 2 to 5 pixels short, a wedge of background under it, tests/harness/glitch.py), else from the screen
 *  @param ctx CPU context
 *  @param v Vertex
 *  @param w Window (projection center)
 *  @param inverse_pd 1 / projection distance
 *  @param out View space position (x, y, z)
 */
static void                                 jo_3d_vertex_view(const jo_3d_cpu_context * const ctx, const jo_3d_vertex * const v,
                                                              const jo_vdp1_window * const w, const jo_fixed inverse_pd, jo_fixed out[3])
{
    const jo_3d_view_position               *position;
    const jo_3d_part                        *part;
    jo_fixed                                (*m)[3];
    jo_pos3D_fixed                          p;
    unsigned int                            index;
    int                                     i;

    out[2] = JO_3D_VERTEX_Z(v);
    if (v->zc & JO_3D_CODE_VIEW)
    {
        position = JO_3D_VIEW_POSITION(ctx, v);
        out[0] = position->x;
        out[1] = position->y;
        return;
    }
    // (a vertex of the object: its index in the array of the CPU; the matrix of its part (the bones of a skeleton))
    index = (unsigned int)((const char *)v - (const char *)ctx->vertices) / sizeof(jo_3d_vertex);
    if (ctx->points != JO_NULL && v >= ctx->vertices && index < ctx->point_count)
    {
        m = ctx->matrix;
        if (ctx->parts != JO_NULL)
            for (JO_ZERO(i), part = ctx->parts; i < ctx->part_count; ++i, ++part)
                if (index - part->first < part->count)
                {
                    m = part->matrix;
                    break;
                }
        jo_3d_transform_point_by((const jo_fixed (*)[3])m, &ctx->points[index], &p);
        out[0] = p.x;
        out[1] = p.y;
        return;
    }
    out[0] = jo_3d_unproject(JO_3D_VERTEX_X(v) - JO_3D_WINDOW_CX(w), out[2], inverse_pd);
    out[1] = jo_3d_unproject(JO_3D_VERTEX_Y(v) - JO_3D_WINDOW_CY(w), out[2], inverse_pd);
}

/** @brief Clip a polygon against one plane (Sutherland-Hodgman)
 *  @param in Input polygon
 *  @param n Input vertex count
 *  @param out Output polygon
 *  @param plane 0: near, 1: left, 2: right, 3: top, 4: bottom
 *  @param near_z Near clipping distance
 *  @param context Clipping context
 *  @return Output vertex count, -1 if the polygon is inside (it is not copied)
 */
static int                                  jo_3d_clip_plane(const jo_3d_clip_vertex * const in, const int n, jo_3d_clip_vertex * const out,
                                                             const int plane, const jo_fixed near_z, const jo_3d_clip_context * const context)
{
    register int                            i;
    int                                     count;
    jo_fixed                                d[JO_3D_CLIP_MAX_VERTICES];
    const jo_3d_clip_vertex                 *a;
    const jo_3d_clip_vertex                 *b;
    jo_fixed                                t;
    jo_fixed                                da;
    jo_fixed                                db;
    jo_fixed                                dt;
    const jo_3d_clip_vertex                 *tmp;
    int                                     outside;
    int                                     next;
    jo_fixed                                k;

    // Signed distance to the plane (positive inside): the plane chosen once, not for each vertex. Divided by 8: the
    // distances and their differences of 2 vertices far from the camera (more than 16384 units, on both sides)
    // overflowed, the point of the edge on the plane was wrong (spikes on a sky box or a huge ground). The signs and
    // the ratios don't change
    switch (plane)
    {
        case 0:
            for (JO_ZERO(i); i < n; ++i)
                d[i] = (in[i].z >> 3) - (near_z >> 3);
            break;
        case 1:
            k = context->planes[0];
            for (JO_ZERO(i); i < n; ++i)
                d[i] = (in[i].x >> 3) + (jo_fixed_mult(in[i].z, k) >> 3);
            break;
        case 2:
            k = context->planes[1];
            for (JO_ZERO(i); i < n; ++i)
                d[i] = (jo_fixed_mult(in[i].z, k) >> 3) - (in[i].x >> 3);
            break;
        case 3:
            k = context->planes[2];
            for (JO_ZERO(i); i < n; ++i)
                d[i] = (in[i].y >> 3) + (jo_fixed_mult(in[i].z, k) >> 3);
            break;
        default:
            k = context->planes[3];
            for (JO_ZERO(i); i < n; ++i)
                d[i] = (jo_fixed_mult(in[i].z, k) >> 3) - (in[i].y >> 3);
            break;
    }
    JO_ZERO(outside);
    for (JO_ZERO(i); i < n; ++i)
        if (d[i] < 0)
            ++outside;
    // All inside: the polygon doesn't change (most planes)
    if (outside == 0)
        return (-1);
    JO_ZERO(count);
    for (JO_ZERO(i); i < n; ++i)
    {
        next = i + 1 < n ? i + 1 : 0;
        a = &in[i];
        b = &in[next];
        // Very unlikely (concave polygon): the vertices that don't fit are ignored
        if (count >= JO_3D_CLIP_MAX_VERTICES - 1)
            break;
        if (d[i] >= 0)
            out[count++] = *a;
        if ((d[i] >= 0) == (d[next] >= 0))
            continue;
        // Intersection of the edge with the plane, always computed from the inside vertex: a neighbor polygon that
        // shares the edge (in the other direction) gets exactly the same point (no gap between them after the rounding)
        da = d[i];
        db = d[next];
        if (da < 0)
        {
            tmp = a;
            a = b;
            b = tmp;
            dt = da;
            da = db;
            db = dt;
        }
        t = jo_fixed_div(da, da - db);
        // (the differences of 2 far vertices halved: they overflowed too)
        out[count].x = a->x + (jo_fixed_mult((b->x >> 1) - (a->x >> 1), t) << 1);
        out[count].y = a->y + (jo_fixed_mult((b->y >> 1) - (a->y >> 1), t) << 1);
        out[count].z = plane == 0 ? near_z : a->z + (jo_fixed_mult((b->z >> 1) - (a->z >> 1), t) << 1);
        out[count].u = a->u + jo_fixed_mult(b->u - a->u, t);
        out[count].v = a->v + jo_fixed_mult(b->v - a->v, t);
        ++count;
    }
    return (count);
}

/** @brief Clip a polygon with the near plane and the edges of the screen
 *  @param ctx CPU context (the view space positions of its vertices)
 *  @param in Vertices of the polygon
 *  @param view View space positions of the vertices (JO_NULL: vertices of the CPU, see jo_3d_cpu_context.view)
 *  @param codes OR of the codes of the vertices
 *  @param near_z Near clipping distance
 *  @param w Window (projection center)
 *  @param right Right of the screen (system clipping)
 *  @param bottom Bottom of the screen (system clipping)
 *  @param context Clipping context of the object
 *  @param out Clipped polygon (screen positions, u and v)
 *  @return Vertex count of the clipped polygon (0 if nothing is visible)
 *  @remarks The VDP1 maps the whole texture on each part of the polygon: the texture is distorted (not the gouraud shading)
 */
static int                                  jo_3d_clip_polygon(const jo_3d_cpu_context * const ctx, jo_3d_vertex * const in[4],
                                                               const jo_fixed (* const view)[3], const unsigned int codes, const jo_fixed near_z,
                                                               const jo_vdp1_window * const w, const int right, const int bottom,
                                                               jo_3d_clip_context * const context, jo_3d_clip_vertex out[JO_3D_CLIP_MAX_VERTICES])
{
    jo_fixed                                exact[3];
    register int                            i;
    int                                     n;
    int                                     count;
    int                                     plane;
    jo_3d_clip_vertex                       buffer[JO_3D_CLIP_MAX_VERTICES];
    jo_3d_clip_vertex                       *src;
    jo_3d_clip_vertex                       *dst;
    jo_3d_clip_vertex                       *tmp;
    jo_fixed                                pd;
    jo_fixed                                k;
    unsigned int                            sides;
    unsigned int                            side;
    static const jo_fixed                   u[4] = {0, JO_FIXED_1, JO_FIXED_1, 0};
    static const jo_fixed                   v[4] = {0, 0, JO_FIXED_1, JO_FIXED_1};

    pd = __jo_vdp1_projection_distance;
    if (!context->ready)
    {
        context->ready = true;
        context->inverse_pd = jo_fixed_div(JO_FIXED_1, pd);
        context->planes[0] = jo_fixed_div(JO_MULT_BY_65536(JO_3D_WINDOW_CX(w) + JO_3D_CLIP_MARGIN), pd);
        context->planes[1] = jo_fixed_div(JO_MULT_BY_65536(right + JO_3D_CLIP_MARGIN - JO_3D_WINDOW_CX(w)), pd);
        context->planes[2] = jo_fixed_div(JO_MULT_BY_65536(JO_3D_WINDOW_CY(w) + JO_3D_CLIP_MARGIN), pd);
        context->planes[3] = jo_fixed_div(JO_MULT_BY_65536(bottom + JO_3D_CLIP_MARGIN - JO_3D_WINDOW_CY(w)), pd);
    }
    // View space positions: (screen - center) * z / projection distance for the projected vertices.
    // A triangle (the last 2 vertices are the same) has 3 vertices
    n = in[3] == in[2] ? 3 : 4;
    // Planes to check: the sides of the screen the vertices are outside of (the vertices behind the near plane or very far
    // outside too: the sides of the view volume, planes through the camera and the edges of the screen, exact, see
    // jo_3d_side_code(); all the sides before: a polygon of the ground cut by the near plane was clipped by the 5 planes)
    JO_ZERO(sides);
    for (JO_ZERO(i); i < n; ++i)
    {
        sides |= JO_3D_VERTEX_CODE(in[i]) & JO_3D_CODE_SCREEN;
        if (view != JO_NULL)
        {
            out[i].x = view[i][0];
            out[i].y = view[i][1];
            out[i].z = view[i][2];
        }
        else
        {
            jo_3d_vertex_view(ctx, in[i], w, context->inverse_pd, exact);
            out[i].x = exact[0];
            out[i].y = exact[1];
            out[i].z = exact[2];
        }
        out[i].u = u[i];
        out[i].v = v[i];
    }
    // Near plane, then the edges of the screen (the vertices of the near plane are usually outside of the screen)
    src = out;
    dst = buffer;
    // (the bit of the side of the plane: a shift by a variable is a call of the library of the compiler)
    for (JO_ZERO(plane), side = JO_3D_CODE_LEFT; plane < 5 && n > 0; ++plane)
    {
        if (plane > 1)
            side = JO_MULT_BY_2(side);
        if (plane == 0 ? !(codes & JO_3D_CODE_NEAR) : !(sides & side))
            continue;
        count = jo_3d_clip_plane(src, n, dst, plane, near_z, context);
        if (count < 0)
            continue;
        n = count;
        tmp = src;
        src = dst;
        dst = tmp;
    }
    if (n < 3)
        return (0);
    if (src != out)
        for (JO_ZERO(i); i < n; ++i)
            out[i] = src[i];
    // Projection (the vertices are on the screen): the division of the next vertex is computed during the projection
    jo_fixed_div_start(pd, out[0].z);
    for (JO_ZERO(i); i < n; ++i)
    {
        k = jo_fixed_div_result();
        if (i + 1 < n)
            jo_fixed_div_start(pd, out[i + 1].z);
        out[i].x = JO_3D_WINDOW_CX(w) + jo_fixed_mult_to_int(out[i].x, k);
        out[i].y = JO_3D_WINDOW_CY(w) + jo_fixed_mult_to_int(out[i].y, k);
    }
    return (n);
}

/** @brief Gouraud color of a point of a polygon (bilinear interpolation of the colors of the 4 vertices)
 *  @param colors Colors of the vertices (gouraud table)
 *  @param u Position (0 to 1) from the vertex 0 to the vertex 1
 *  @param v Position (0 to 1) from the vertex 0 to the vertex 3
 *  @return RGB 555 color
 */
static unsigned short                       jo_3d_gouraud_color(const unsigned short * const colors, const jo_fixed u, const jo_fixed v)
{
    int                                     wu;
    int                                     wv;
    int                                     w[4];
    int                                     r;
    int                                     g;
    int                                     b;
    register int                            i;

    // A vertex of the original polygon (most vertices of a clipped polygon)
    if ((u == 0 || u == JO_FIXED_1) && (v == 0 || v == JO_FIXED_1))
        return (colors[v == 0 ? (u == 0 ? 0 : 1) : (u == 0 ? 3 : 2)]);
    // Weights of the 4 corners (8 bits u and v, the sum is 65536). u and v can be a little outside of 0 to 1 (rounding of the intersections)
    // (unsigned shifts: a signed shift by 8 is a call of the library of the compiler)
    wu = u <= 0 ? 0 : (u >= JO_FIXED_1 ? 256 : (int)((unsigned int)u >> 8));
    wv = v <= 0 ? 0 : (v >= JO_FIXED_1 ? 256 : (int)((unsigned int)v >> 8));
    w[0] = (256 - wu) * (256 - wv);
    w[1] = wu * (256 - wv);
    w[2] = wu * wv;
    w[3] = (256 - wu) * wv;
    // Constant shifts only (the SH-2 has no barrel shifter)
    r = JO_DIV_BY_2(JO_FIXED_1);
    g = JO_DIV_BY_2(JO_FIXED_1);
    b = JO_DIV_BY_2(JO_FIXED_1);
    for (JO_ZERO(i); i < 4; ++i)
    {
        r += (colors[i] & JO_3D_RGB_COMPONENT_MASK) * w[i];
        g += ((colors[i] >> 5) & JO_3D_RGB_COMPONENT_MASK) * w[i];
        b += ((colors[i] >> 10) & JO_3D_RGB_COMPONENT_MASK) * w[i];
    }
    return ((colors[0] & JO_COLOR_SATURN_MSB) | (r >> 16) | ((g >> 16) << 5) | ((b >> 16) << 10));
}

/** @brief Texture to use for a size on the screen: the VDP1 reads all the texels of a texture, even when it is drawn
 *  small (far away). The smallest copy of the texture that has at least as many texels as pixels on the screen is used
 *  @param sprite_id Texture
 *  @param width Size on the screen (pixels) along the width of the texture
 *  @param height Size on the screen (pixels) along the height of the texture
 *  @return Sprite to use
 */
int __attribute__((noinline, used, externally_visible)) __jo_3d_mipmap_select(const int sprite_id, const int width, const int height)
{
    const jo_3d_texture_mipmaps             *mip;

    mip = &__jo_3d_mipmaps[sprite_id];
    if (mip->levels == 0)
        return (sprite_id);
    // (the limits: see jo_3d_mipmap_limits())
    if (width * height > mip->area[0] || width > mip->width[0] || height > mip->height[0])
        return (sprite_id);
    if (mip->levels < 2 || width * height > mip->area[1] || width > mip->width[1] || height > mip->height[1])
        return (mip->first);
    return (mip->first + 1);
}

/** @brief Compute the sizes on the screen up to which the smaller copies of a texture are used
 *  @param mip Smaller copies
 *  @param texture The texture
 *  @remarks The VDP1 reads a texture entirely even drawn smaller (its cost: max(lines, height) x max(pixels per line, width)):
 *  a smaller copy is used as soon as the polygon has fewer pixels than 11/16 x 11/16 of it (its area: a polygon narrow and
 *  tall takes the copy too, the texture reduced along its width by the VDP1 looked darker, the last texels written
 *  winning), as long as the copy is not magnified more than 2.5 times along a side. The copy is magnified rather than the
 *  texture reduced (as much texture read for the same pixels, and the pattern flickers)
 */
static void                                 jo_3d_mipmap_limits(jo_3d_texture_mipmaps * const mip, const jo_texture_definition * const texture)
{
    mip->area[0] = JO_3D_MIPMAP_LIMIT(JO_DIV_BY_2(texture->width)) * JO_3D_MIPMAP_LIMIT(JO_DIV_BY_2(texture->height));
    mip->area[1] = JO_3D_MIPMAP_LIMIT(JO_DIV_BY_4(texture->width)) * JO_3D_MIPMAP_LIMIT(JO_DIV_BY_4(texture->height));
    // 2 x size > 2.5 x the size of the texture, 4 x size > 2.5 x...
    mip->width[0] = (unsigned short)JO_DIV_BY_2(JO_MULT_BY_2(texture->width) + JO_DIV_BY_2(texture->width));
    mip->height[0] = (unsigned short)JO_DIV_BY_2(JO_MULT_BY_2(texture->height) + JO_DIV_BY_2(texture->height));
    mip->width[1] = (unsigned short)JO_DIV_BY_4(JO_MULT_BY_2(texture->width) + JO_DIV_BY_2(texture->width));
    mip->height[1] = (unsigned short)JO_DIV_BY_4(JO_MULT_BY_2(texture->height) + JO_DIV_BY_2(texture->height));
}

/** @brief Texture to use for a size on the screen (see __jo_3d_mipmap_select()) */
# define jo_3d_mipmap(SPRITE_ID, WIDTH, HEIGHT) __jo_3d_mipmap_select(SPRITE_ID, WIDTH, HEIGHT)

/** @brief A gouraud table of the set of the frame for a part of a clipped polygon: the master CPU takes them from the first up,
 *  the slave CPU from the last down (the CPU that draws more polygons near the camera takes more of them)
 *  @param ctx CPU
 *  @return Index of the table (in the tables of JO_VDP1_CLIP_GOURAUD_TABLE_OFFSET), -1 if they are all used (the table of
 *          the whole polygon instead)
 */
static  __jo_force_inline int               jo_3d_clip_table(jo_3d_cpu_context * const ctx)
{
    volatile unsigned int                   *used;
    int                                     cpu;

    cpu = ctx->slave ? 1 : 0;
    used = (volatile unsigned int *)JO_CACHE_THROUGH(__jo_3d_clip_tables_used);
    // (the count of the other CPU read through the cache: it may take one more meanwhile, the last tables are kept free)
    if (ctx->clip_table_count + used[1 - cpu] + 1 >= ctx->clip_table_max)
        return (-1);
    used[cpu] = ++ctx->clip_table_count;
    return ((int)(cpu ? ctx->clip_table_first - (ctx->clip_table_count - 1) : ctx->clip_table_first + ctx->clip_table_count - 1));
}

/** @brief Length of an edge for the VDP1: the longest of |dx| and |dy| (the pixels of a line, the lines of a side)
 *  @param xa First vertex (screen position)
 *  @param ya First vertex
 *  @param xb Second vertex
 *  @param yb Second vertex
 *  @return Length
 */
static  __jo_force_inline unsigned int      jo_3d_vdp1_edge_length(const int xa, const int ya, const int xb, const int yb)
{
    int                                     dx;
    int                                     dy;

    dx = JO_ABS(xb - xa);
    dy = JO_ABS(yb - ya);
    return ((unsigned int)JO_MAX(dx, dy));
}

/** @brief Check if the VDP1 draws an untextured quad A B C D faster with its vertices rotated by one (B C D A)
 *  @return true if the rotated quad is faster
 *  @remarks The VDP1 draws max(|AD|, |BC|) + 1 lines from the edge A-D to the edge B-C: |AB| + 1 pixels for the first one,
 *           |DC| + 1 for the last one, about |B + C - A - D| / 2 + 1 for the middle one. A line costs about 12 cycles plus 1 per
 *           pixel (Mednafen, vdp1.c): about (max(|AD|, |BC|) + 1) x (6 x 13 + |AB| + |DC| + 2 x |B + C - A - D|) / 6 cycles
 *           (Simpson's rule over the lines: the quads that are not parallelograms too). The rotated quad draws its lines from
 *           the edge B-A to the edge C-D. The image of an untextured polygon doesn't change (the gouraud colors follow their
 *           vertices)
 *  @remarks Unsigned: inside of the guard band there is no overflow, farther a wrong choice only costs time
 *  @remarks The same test in __jo_3d_polygon_loop()
 */
static  __jo_force_inline bool              jo_3d_polygon_rotation_is_faster(const int x0, const int y0, const int x1, const int y1,
                                                                             const int x2, const int y2, const int x3, const int y3)
{
    unsigned int                            ab;
    unsigned int                            bc;
    unsigned int                            cd;
    unsigned int                            da;
    unsigned int                            middle;
    unsigned int                            rotated_middle;

    ab = jo_3d_vdp1_edge_length(x0, y0, x1, y1);
    bc = jo_3d_vdp1_edge_length(x1, y1, x2, y2);
    cd = jo_3d_vdp1_edge_length(x2, y2, x3, y3);
    da = jo_3d_vdp1_edge_length(x3, y3, x0, y0);
    // |B + C - A - D| and |C + D - A - B|: twice the middle lines
    middle = jo_3d_vdp1_edge_length(x0 + x3, y0 + y3, x1 + x2, y1 + y2);
    rotated_middle = jo_3d_vdp1_edge_length(x0 + x1, y0 + y1, x2 + x3, y2 + y3);
    return ((JO_MAX(ab, cd) + 1) * (78 + bc + da + JO_MULT_BY_2(rotated_middle)) <
            (JO_MAX(da, bc) + 1) * (78 + ab + cd + JO_MULT_BY_2(middle)));
}

/** @brief Add a polygon to the VDP1 command table
 *  @param colors Gouraud colors of the 4 vertices (a gouraud table is allocated), or JO_NULL to use the table of the polygon
 *  @return false if the command table is full
 *  @remarks An untextured polygon is drawn with its vertices rotated by one when the VDP1 draws it faster (see
 *           jo_3d_polygon_rotation_is_faster())
 */
static  __jo_force_inline bool              jo_3d_emit_polygon(jo_3d_cpu_context * const ctx, jo_3d_vertex *v0, jo_3d_vertex *v1, jo_3d_vertex *v2, jo_3d_vertex *v3,
                                                               const unsigned short * const colors,
                                                               const jo_3d_polygon * const p, const jo_3d_polygon_attribute * const a,
                                                               jo_vdp1_command * const commands, unsigned short * const keys,
                                                               const unsigned int reserved, unsigned int * const used, jo_3d_light_context * const light)
{
    jo_3d_vertex                            *tmp;
    jo_fixed                                key;
    int                                     sort_key;
    jo_vdp1_command                         *cmd;
    unsigned int                            function;
    int                                     cross;
    int                                     tri;
    bool                                    mirrored;
    unsigned int                            rotation;
    jo_texture_definition                   *texture;
    volatile unsigned int                   *table;
    int                                     index;
    jo_3d_command_word                      *words;
    unsigned int                            ctrl;
    unsigned int                            pmod;
    unsigned int                            grda;
    unsigned int                            texture_words;
    unsigned int                            color;
    int                                     x0;
    int                                     y0;
    int                                     x1;
    int                                     y1;
    int                                     x2;
    int                                     y2;
    int                                     x3;
    int                                     y3;

    x0 = JO_3D_VERTEX_X(v0);
    y0 = JO_3D_VERTEX_Y(v0);
    x1 = JO_3D_VERTEX_X(v1);
    y1 = JO_3D_VERTEX_Y(v1);
    x2 = JO_3D_VERTEX_X(v2);
    y2 = JO_3D_VERTEX_Y(v2);
    x3 = JO_3D_VERTEX_X(v3);
    y3 = JO_3D_VERTEX_Y(v3);
    // Facing (screen space winding): front faces are clockwise.
    // Cross product of the diagonals: twice the signed area of the whole quad (also valid for a triangle: v3 = v2).
    // Unlike the area of the first triangle, it is right for a non planar quad seen at a grazing angle.
    cross = (x2 - x0) * (y3 - y1) - (x3 - x1) * (y2 - y0);
    // A reflected view (a mirror): the winding of the front faces is reversed
    if (ctx->mirror)
        cross = -cross;
    mirrored = false;
    // (a single plane quad is skipped when its 2 triangles face away: a quad of the ground is not flat, seen at a grazing angle
    // one of its triangles can face the camera, see __jo_3d_polygon_loop(). The area of the quad is the sum of the areas of
    // the triangles 0 1 2 and 0 2 3)
    if (cross < 0 && a->plane == JO_3D_SINGLE_PLANE)
    {
        tri = (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
        if (ctx->mirror)
            tri = -tri;
        if (tri >= 0 || cross >= tri)
            cross = 0;
    }
    if (cross < 0)
    {
        if (a->plane == JO_3D_SINGLE_PLANE)
            return (true);
        // Back face of a dual plane polygon: mirror the vertex order
        mirrored = true;
        tmp = v0;
        v0 = v1;
        v1 = tmp;
        tmp = v2;
        v2 = v3;
        v3 = tmp;
        cross = x0;
        x0 = x1;
        x1 = cross;
        cross = y0;
        y0 = y1;
        y1 = cross;
        cross = x2;
        x2 = x3;
        x3 = cross;
        cross = y2;
        y2 = y3;
        y3 = cross;
    }
    // (the outcodes in the low bits of the depths are removed from the key)
    switch (ctx->forced_key >= 0 ? JO_3D_SORT_MASK + 1 : (a->sort & JO_3D_SORT_MASK))
    {
        case JO_3D_SORT_MIN:
            key = JO_MIN(JO_MIN(v0->zc, v1->zc), JO_MIN(v2->zc, v3->zc)) & ~JO_3D_CODE_MASK;
            break;
        case JO_3D_SORT_MAX:
            key = JO_MAX(JO_MAX(v0->zc, v1->zc), JO_MAX(v2->zc, v3->zc)) & ~JO_3D_CODE_MASK;
            break;
        case JO_3D_SORT_PREVIOUS:
            key = ctx->previous_polygon_z;
            break;
        case JO_3D_SORT_MASK + 1:
            key = ctx->forced_key;
            break;
        default:
            key = (JO_DIV_BY_4(v0->zc) + JO_DIV_BY_4(v1->zc) + JO_DIV_BY_4(v2->zc) + JO_DIV_BY_4(v3->zc)) & ~JO_3D_CODE_MASK;
            break;
    }
    // A polygon that crosses the near plane (a side of a vehicle next to the camera): its points behind the camera have
    // negative depths. Sorted with them (JO_3D_SORT_MIN), it was drawn over the weapon of the player (in front of the
    // camera): the key is the near plane at least
    if (key < __jo_vdp1_near_z)
        key = __jo_vdp1_near_z;
    ctx->previous_polygon_z = key;
    if (*used >= reserved)
    {
        // The command table is full (__jo_vdp1_add_command() reports the error, the slave CPU drops the polygons)
        if (!ctx->slave)
        {
            __jo_vdp1_commit_commands(*used);
            __jo_vdp1_add_command(key);
        }
        return (false);
    }
    // Sort key with the depth bias of the CPU (see __jo_vdp1_sort_key())
    sort_key = JO_FIXED_TO_INT(key) + *ctx->depth_bias;
    // The front layer (a rear-view mirror): in front of all the other polygons, sorted among themselves
    if (ctx->front_shift >= 0)
        sort_key = (JO_MAX(JO_FIXED_TO_INT(key), 0) >> ctx->front_shift) + 1;
    keys[*used] = sort_key < 0 ? 0 : (sort_key > JO_VDP1_MAX_KEY ? JO_VDP1_MAX_KEY : (unsigned short)sort_key);
    cmd = &commands[(*used)++];
    ++*ctx->displayed_polygon_count;
    function = a->direction & JO_VDP1_DIRECTION_FUNCTION_MASK;
    pmod = a->draw_mode | ctx->clip_mode;
    // (the pre-clipping disabled for a polygon inside of the screen, see jo_3d_pre_clipping(): a window without clipping,
    // the system clipping area. The sign bits of the packed positions (x, y) and of the packed distances to the right and
    // bottom sides: a borrow of y into x only comes with a y out of the area, found by its own sign. The test of the 8
    // coordinates cost 1 microsecond per polygon to the environment map demo, all its polygons in the C code)
    if (ctx->clip_mode == 0)
    {
        const unsigned int                  area = ((unsigned int)__jo_vdp1_system_clip_right << 16) | (unsigned short)__jo_vdp1_system_clip_bottom;
        const unsigned int                  p0 = (unsigned int)v0->xy;
        const unsigned int                  p1 = (unsigned int)v1->xy;
        const unsigned int                  p2 = (unsigned int)v2->xy;
        const unsigned int                  p3 = (unsigned int)v3->xy;

        if (((p0 | p1 | p2 | p3 | (area - p0) | (area - p1) | (area - p2) | (area - p3)) & 0x80008000u) == 0)
            pmod |= JO_VDP1_PRE_CLIPPING_DISABLE;
    }
    // Gouraud shading alone or with half luminance / half transparency (modes 4, 6 and 7)
    grda = (pmod & JO_VDP1_COLOR_CALC_GOURAUD) ? (a->gouraud_table != 0 ? (unsigned short)(a->gouraud_table + ctx->gouraud_offset) : 0) : 0;
    // An untextured polygon: its vertices rotated by one when the VDP1 draws it with fewer lines (0: not rotated, 1: B C D A)
    JO_ZERO(rotation);
    if (!(a->sort & JO_3D_USE_TEXTURE) && function != JO_VDP1_FUNCTION_POLYLINE && function != JO_VDP1_FUNCTION_LINE &&
        jo_3d_polygon_rotation_is_faster(x0, y0, x1, y1, x2, y2, x3, y3))
        rotation = 1;
    // Part of a clipped polygon: gouraud table of the part (the colors follow the vertices when they are mirrored or rotated)
    if (colors != JO_NULL && (index = jo_3d_clip_table(ctx)) >= 0)
    {
        unsigned short                      vertex_colors[4];

        vertex_colors[0] = colors[mirrored ? 1 : 0];
        vertex_colors[1] = colors[mirrored ? 0 : 1];
        vertex_colors[2] = colors[mirrored ? 3 : 2];
        vertex_colors[3] = colors[mirrored ? 2 : 3];
        // (2 32 bits writes: an access to the VRAM is slow, the table is 8 bytes aligned)
        table = (volatile unsigned int *)(JO_VDP1_VRAM + JO_VDP1_CLIP_GOURAUD_TABLE_OFFSET + JO_MULT_BY_8(index));
        table[0] = ((unsigned int)vertex_colors[rotation] << 16) | vertex_colors[rotation + 1];
        table[1] = ((unsigned int)vertex_colors[rotation + 2] << 16) | vertex_colors[(rotation + 3) & 3];
        grda = JO_DIV_BY_8(JO_VDP1_CLIP_GOURAUD_TABLE_OFFSET) + index;
    }
    // The table of the polygon (its colors in the order of its vertices): not rotated
    else if (pmod & JO_VDP1_COLOR_CALC_GOURAUD)
        JO_ZERO(rotation);
    JO_ZERO(texture_words);
    color = a->color;
    if (ctx->sliver_color != 0)
    {
        // A textured polygon seen as a thin line: a polygon of the mean color of its texture (see jo_3d_emit_sliver(); the
        // color mode of its texture replaced: a polygon with the mode of a 16 colors texture was drawn with a wrong color)
        ctrl = JO_VDP1_CMD_POLYGON;
        color = ctx->sliver_color;
        pmod = (pmod & ~JO_VDP1_COLOR_MODE_MASK) | JO_VDP1_COLOR_MODE_RGB | JO_VDP1_END_CODE_DISABLE | JO_VDP1_TRANSPARENT_PIXEL_DISABLE;
    }
    else if ((a->sort & JO_3D_USE_TEXTURE) && ctx->texture_words != 0)
    {
        // A tile of the texture (a part of a subdivided polygon, see jo_3d_emit_tile())
        ctrl = JO_VDP1_CMD_DISTORTED_SPRITE | (a->direction & JO_VDP1_DIRECTION_FLIP_MASK);
        texture_words = ctx->texture_words;
        if ((a->direction & JO_VDP1_H_FLIP) && __jo_sprite_end_codes[a->texture_id])
            pmod |= JO_VDP1_END_CODE_DISABLE;
    }
    else if (a->sort & JO_3D_USE_TEXTURE)
    {
        // Far away: a smaller copy of the texture (size on the screen: the longest of the 2 edges of each direction)
        texture = &__jo_sprite_def[__jo_3d_mipmaps[a->texture_id].levels == 0 ? a->texture_id :
                                   jo_3d_mipmap(a->texture_id,
                                                JO_MAX(JO_MAX(JO_ABS(x1 - x0), JO_ABS(y1 - y0)), JO_MAX(JO_ABS(x2 - x3), JO_ABS(y2 - y3))),
                                                JO_MAX(JO_MAX(JO_ABS(x3 - x0), JO_ABS(y3 - y0)), JO_MAX(JO_ABS(x2 - x1), JO_ABS(y2 - y1))))];
        ctrl = JO_VDP1_CMD_DISTORTED_SPRITE | (a->direction & JO_VDP1_DIRECTION_FLIP_MASK);
        texture_words = ((unsigned int)texture->adr << 16) | texture->size;
        // The VDP1 reads the lines of a texture flipped horizontally from their end: it would stop at the end codes
        if ((a->direction & JO_VDP1_H_FLIP) && __jo_sprite_end_codes[a->texture_id])
            pmod |= JO_VDP1_END_CODE_DISABLE;
    }
    else if (function == JO_VDP1_FUNCTION_POLYLINE)
        ctrl = JO_VDP1_CMD_POLYLINE;
    else if (function == JO_VDP1_FUNCTION_LINE)
        ctrl = JO_VDP1_CMD_LINE;
    else
        ctrl = JO_VDP1_CMD_POLYGON;
    // 8 stores of 32 bits instead of 16 stores of 16 bits (the cache writes through: each store is a bus access)
    words = (jo_3d_command_word *)cmd;
    words[0] = (unsigned int)ctrl << 16;
    words[1] = ((unsigned int)pmod << 16) | color;
    words[2] = texture_words;
    if (rotation)
    {
        words[3] = (unsigned int)v1->xy;
        words[4] = (unsigned int)v2->xy;
        words[5] = (unsigned int)v3->xy;
        words[6] = (unsigned int)v0->xy;
    }
    else
    {
        words[3] = (unsigned int)v0->xy;
        words[4] = (unsigned int)v1->xy;
        words[5] = (unsigned int)v2->xy;
        words[6] = (unsigned int)v3->xy;
    }
    words[7] = (unsigned int)grda << 16;
#ifdef JO_COMPILE_WITH_PROFILER_SUPPORT
    if (__jo_profile_pixels)
    {
        // The pixels the VDP1 draws: about max(|AD|, |BC|) lines of max(|AB|, |DC|) pixels, at least the texture
        int                                 lines;
        int                                 width;

        lines = JO_MAX(JO_MAX(JO_ABS(x3 - x0), JO_ABS(y3 - y0)), JO_MAX(JO_ABS(x2 - x1), JO_ABS(y2 - y1)));
        width = JO_MAX(JO_MAX(JO_ABS(x1 - x0), JO_ABS(y1 - y0)), JO_MAX(JO_ABS(x2 - x3), JO_ABS(y2 - y3)));
        ctx->pixels += (JO_MAX(lines, (int)(texture_words & 0xFF)) + 1) * (JO_MAX(width, (int)JO_MULT_BY_8((texture_words >> 8) & 0x3F)) + 1);
    }
#endif
    if (jo_3d_is_lit(a))
    {
        jo_profile_begin(JO_PROFILE_3D_LIGHT);
        jo_3d_apply_light(cmd, light, p, a);
        jo_profile_end(JO_PROFILE_3D_LIGHT);
    }
    return (true);
}

/** @brief jo_3d_emit_polygon() not inlined: all its callers (the parts of the polygons drawn in parts and of the clipped
 *  polygons, the near tiles, the general path). Each copy of it is about 2.3 KB of code: inlined in the parts of the
 *  subdivided polygons (4 copies, 11 KB) the work RAM of a big game overflowed, and a scene was 0.3 ms slower on the
 *  master CPU (the cache of the CPU is 4 KB) */
static __attribute__((noinline)) bool       jo_3d_emit_polygon_call(jo_3d_cpu_context * const ctx, jo_3d_vertex *v0, jo_3d_vertex *v1,
                                                                    jo_3d_vertex *v2, jo_3d_vertex *v3, const unsigned short * const colors,
                                                                    const jo_3d_polygon * const p, const jo_3d_polygon_attribute * const a,
                                                                    jo_vdp1_command * const commands, unsigned short * const keys,
                                                                    const unsigned int reserved, unsigned int * const used,
                                                                    jo_3d_light_context * const light)
{
    return (jo_3d_emit_polygon(ctx, v0, v1, v2, v3, colors, p, a, commands, keys, reserved, used, light));
}

/** @brief Vertex order of a triangle drawn by the VDP1 as a quad (the last vertex is repeated)
 *  @param v Vertices
 *  @param a First vertex (clockwise)
 *  @param b Second vertex
 *  @param c Third vertex
 *  @param idx Vertex order (output)
 *  @remarks The VDP1 draws lines from the edge A-D to the edge B-C, about max(A-D, B-C) lines as long as A-B: the
 *  shortest edge is used as A-B (the quad A, B, C, C costs about longest edge x shortest edge / 2 pixels)
 */
static void                                 jo_3d_triangle_order(const jo_3d_vertex * const v, const int a, const int b, const int c, int idx[4])
{
    int                                     ab;
    int                                     bc;
    int                                     ca;

    // Manhattan lengths are enough to compare the edges
    ab = JO_ABS(JO_3D_VERTEX_X(&v[b]) - JO_3D_VERTEX_X(&v[a])) + JO_ABS(JO_3D_VERTEX_Y(&v[b]) - JO_3D_VERTEX_Y(&v[a]));
    bc = JO_ABS(JO_3D_VERTEX_X(&v[c]) - JO_3D_VERTEX_X(&v[b])) + JO_ABS(JO_3D_VERTEX_Y(&v[c]) - JO_3D_VERTEX_Y(&v[b]));
    ca = JO_ABS(JO_3D_VERTEX_X(&v[a]) - JO_3D_VERTEX_X(&v[c])) + JO_ABS(JO_3D_VERTEX_Y(&v[a]) - JO_3D_VERTEX_Y(&v[c]));
    // Rotation of (a, b, c): the winding doesn't change
    if (ab <= bc && ab <= ca)
    {
        idx[0] = a;
        idx[1] = b;
        idx[2] = c;
    }
    else if (bc <= ca)
    {
        idx[0] = b;
        idx[1] = c;
        idx[2] = a;
    }
    else
    {
        idx[0] = c;
        idx[1] = a;
        idx[2] = b;
    }
    idx[3] = idx[2];
}

/** @brief Check if the color of a polygon depends on the frame buffer under it (half transparency, with or without the
 *  gouraud shading; shadow): its parts must not overlap, the overlap would be seen
 *  @param draw_mode Draw mode (CMDPMOD)
 */
static  __jo_force_inline bool              jo_3d_reads_frame_buffer(const unsigned int draw_mode)
{
    unsigned int                            mode;

    mode = draw_mode & JO_VDP1_COLOR_CALC_MASK;
    return (mode == JO_VDP1_COLOR_CALC_SHADOW || (mode & JO_VDP1_COLOR_CALC_HALF_TRANSPARENT) == JO_VDP1_COLOR_CALC_HALF_TRANSPARENT);
}

/** @brief Turn the vertices of a clipped textured polygon so that its first vertex is the one of the corner A of the texture
 *  (u = 0, v = 0): the VDP1 maps the top left of the texture on the first vertex of a part. The clipping starts the polygon
 *  at any vertex: the texture of a picture near the camera (clipped by the edges of the screen) turned by 90 degrees
 *  @param clipped Vertices (u and v: their position in the original polygon)
 *  @param n Vertex count (4 or more)
 *  @remarks 4 vertices: the turn that puts the 4 vertices nearest to the 4 corners A, B, C, D. More: the vertex nearest to A
 *  first (the parts of the fan map the whole texture: their orientation is the one of the polygon)
 */
static void                                 jo_3d_orient_clipped_polygon(jo_3d_clip_vertex * const clipped, const int n)
{
    static const jo_fixed                   corner_u[4] = {0, JO_FIXED_1, JO_FIXED_1, 0};
    static const jo_fixed                   corner_v[4] = {0, 0, JO_FIXED_1, JO_FIXED_1};
    jo_3d_clip_vertex                       turned[JO_3D_CLIP_MAX_VERTICES];
    jo_fixed                                cost;
    jo_fixed                                best_cost;
    int                                     best;
    int                                     r;
    register int                            i;

    best = 0;
    best_cost = JO_FIXED_MAX;
    for (JO_ZERO(r); r < n; ++r)
    {
        if (n == 4)
            for (JO_ZERO(cost), JO_ZERO(i); i < 4; ++i)
                cost += JO_ABS(clipped[(r + i) & 3].u - corner_u[i]) + JO_ABS(clipped[(r + i) & 3].v - corner_v[i]);
        else
            cost = JO_ABS(clipped[r].u) + JO_ABS(clipped[r].v);
        if (cost < best_cost)
        {
            best_cost = cost;
            best = r;
        }
    }
    if (best == 0)
        return;
    for (JO_ZERO(i); i < n; ++i)
        turned[i] = clipped[best + i < n ? best + i : best + i - n];
    for (JO_ZERO(i); i < n; ++i)
        clipped[i] = turned[i];
}

/** @brief Place of a point of a clipped triangle in the quad A B C C drawn by the VDP1, along its line (for its gouraud colors,
 *  see jo_3d_emit_clipped_polygon()). The clipping interpolates the (u, v) of the corners A (0, 0), B (1, 0) and C (1, 1)
 *  along the edges: u = b + c and v = c for the point b B + c C + (1 - b - c) A. The VDP1 draws the lines of the quad from
 *  the edge A-D (A-C) to the edge B-C: the point is at b / (1 - c) of its line
 *  @param u Position of the point given by the clipping
 *  @param v Position of the point given by the clipping (from the edge A-B to C)
 *  @return Position along the line of the VDP1 (0: the edge A-C, 1: the edge B-C)
 */
static  __jo_force_inline jo_fixed          jo_3d_triangle_u(const jo_fixed u, const jo_fixed v)
{
    // (C itself: its color, see jo_3d_gouraud_color())
    if (v >= JO_FIXED_1)
        return (JO_FIXED_1);
    if (u <= v)
        return (0);
    return (jo_fixed_div(u - v, JO_FIXED_1 - v));
}

/** @brief Draw a polygon that crosses the near plane or goes far outside of the screen: it is clipped, then drawn in parts of 4 vertices
 *  @param view View space positions of the vertices (JO_NULL: vertices of the CPU)
 *  @param corner_colors Gouraud colors of the 4 vertices, JO_NULL: the gouraud table of the polygon
 *  @return false if the command table is full
 */
static bool                                 jo_3d_emit_clipped_polygon(jo_3d_cpu_context * const ctx, jo_3d_vertex * const v[4],
                                                                       const jo_fixed (* const view)[3], const unsigned int codes,
                                                                       const jo_fixed near_z,
                                                                       const jo_vdp1_window * const w, const int right, const int bottom,
                                                                       const jo_3d_polygon * const p, const jo_3d_polygon_attribute * const a,
                                                                       const unsigned short * const corner_colors,
                                                                       jo_vdp1_command * const commands, unsigned short * const keys,
                                                                       const unsigned int reserved, unsigned int * const used,
                                                                       jo_3d_light_context * const light, jo_3d_clip_context * const context)
{
    register int                            i;
    int                                     n;
    int                                     first;
    jo_3d_clip_vertex                       clipped[JO_3D_CLIP_MAX_VERTICES];
    jo_3d_vertex                            screen[JO_3D_CLIP_MAX_VERTICES];
    unsigned short                          table[4];
    unsigned short                          colors[JO_3D_CLIP_MAX_VERTICES];
    unsigned short                          part[4];
    bool                                    gouraud;
    unsigned int                            all;
    int                                     idx[4];
    int                                     k;

    n = jo_3d_clip_polygon(ctx, v, view, codes, near_z, w, right, bottom, context, clipped);
    if (n == 0)
        return (true);
    // (a texture: its top left corner on the first vertex)
    if ((a->sort & JO_3D_USE_TEXTURE) && n >= 4)
        jo_3d_orient_clipped_polygon(clipped, n);
    // Outside of the screen
    all = ~0u;
    for (JO_ZERO(i); i < n; ++i)
    {
        jo_3d_set_vertex(&screen[i], clipped[i].x, clipped[i].y, clipped[i].z, 0);
        // A corner of the polygon kept by the clipping (projected): its position on the screen as it is, the same as for the
        // polygons next to it (projected again from the view space, it was rounded differently: a line of background
        // between them)
        if ((clipped[i].u == 0 || clipped[i].u == JO_FIXED_1) && (clipped[i].v == 0 || clipped[i].v == JO_FIXED_1))
        {
            k = clipped[i].v == 0 ? (clipped[i].u == 0 ? 0 : 1) : (clipped[i].u == 0 ? 3 : 2);
            if (!(v[k]->zc & (JO_3D_CODE_NEAR | JO_3D_CODE_VIEW)))
                screen[i].xy = v[k]->xy;
        }
        all &= jo_3d_screen_code(JO_3D_VERTEX_X(&screen[i]), JO_3D_VERTEX_Y(&screen[i]), right, bottom, 0);
    }
    if (all & JO_3D_CODE_SCREEN)
        return (true);
    // Gouraud shading interpolated at each vertex
    gouraud = (a->draw_mode & JO_VDP1_COLOR_CALC_GOURAUD) != 0;
    if (gouraud)
    {
        // The colors of the corners: the gouraud table of the polygon, or the colors of a part of a subdivided polygon
        if (corner_colors != JO_NULL)
            for (JO_ZERO(i); i < 4; ++i)
                table[i] = corner_colors[i];
        else
            jo_3d_read_gouraud_table((unsigned short)(a->gouraud_table + ctx->gouraud_offset), table);
        // The same color for the 4 vertices (flat shading): nothing to interpolate
        if (table[0] == table[1] && table[0] == table[2] && table[0] == table[3])
            for (JO_ZERO(i); i < n; ++i)
                colors[i] = table[0];
        else if (corner_colors == JO_NULL && v[3] == v[2])
            // (a triangle: the VDP1 draws it as A B C C, the colors of a point are the ones of its place in that quad, see
            // jo_3d_triangle_u(); the bilinear colors of (u, v) put some of the color of B on the edge A-C, a seam with the
            // triangle next to it)
            for (JO_ZERO(i); i < n; ++i)
                colors[i] = jo_3d_gouraud_color(table, jo_3d_triangle_u(clipped[i].u, clipped[i].v), clipped[i].v);
        else
            for (JO_ZERO(i); i < n; ++i)
                colors[i] = jo_3d_gouraud_color(table, clipped[i].u, clipped[i].v);
    }
    // Fan: (0, 1, 2, 3), (0, 3, 4, 5), (0, 5, 6, 7)... (the last part can be a triangle)
    for (first = 1; first + 1 < n; first += 2)
    {
        if (first + 2 < n)
        {
            idx[0] = 0;
            idx[1] = first;
            idx[2] = first + 1;
            idx[3] = first + 2;
        }
        else
            jo_3d_triangle_order(screen, 0, first, first + 1, idx);
        if (gouraud)
            for (JO_ZERO(i); i < 4; ++i)
                part[i] = colors[idx[i]];
        // (not grown: its corners kept by the clipping are the ones of the polygons next to it (see above), its new vertices
        // are computed from the same edge by them (see jo_3d_clip_plane()), and its parts share their vertices. Grown by 1
        // pixel, the parts overlapped)
        if (!jo_3d_emit_polygon_call(ctx, &screen[idx[0]], &screen[idx[1]], &screen[idx[2]], &screen[idx[3]],
                                gouraud ? part : JO_NULL, p, a, commands, keys, reserved, used, light))
            return (false);
    }
    // The polygon after this one sorted with it (JO_3D_SORT_PREVIOUS: a picture on a wall) is drawn after all the parts of
    // the fan: the key of the nearest vertex (see jo_3d_emit_subdivided_polygon())
    if (ctx->forced_key < 0 && n > 4)
    {
        k = screen[0].zc;
        for (i = 1; i < n; ++i)
            k = JO_MIN(k, screen[i].zc);
        ctx->previous_polygon_z = JO_MAX(__jo_vdp1_near_z, k & ~JO_3D_CODE_MASK);
    }
    return (true);
}

/** @brief Outcode of a point of the view space that is not projected (behind the near plane or very far outside of the
 *  screen): the sides of the screen (the planes through the eye and the edges of the screen) it is outside of, as for a
 *  projected point (see __jo_3d_project_slow()). A part of a subdivided polygon outside of the same side is not drawn: with
 *  JO_3D_CODE_NEAR and JO_3D_CODE_VIEW only, a part beside the screen that crosses the near plane went through all the
 *  clipping to draw nothing
 *  @remarks 64 bits: the point can be very far
 */
static unsigned int                         jo_3d_view_side_code(const jo_fixed x, const jo_fixed y, const jo_fixed z,
                                                                 const jo_vdp1_window * const w, const int right, const int bottom)
{
    return (jo_3d_side_code(x, y, z, JO_3D_WINDOW_CX(w), JO_3D_WINDOW_CY(w), right, bottom, __jo_vdp1_projection_distance));
}

/** @brief Project points of the view space (see jo_3d_transform_vertices()): the division of the next point is done in the
 *  background while a point is projected
 *  @param v Vertices (output)
 *  @param points View space positions
 *  @param count Point count
 *  @remarks The view space position of a point behind the near plane or very far outside is not kept: the caller has it
 */
static void                                 jo_3d_project_points(jo_3d_vertex * const v, const jo_fixed (* const points)[3], const int count,
                                                                 const jo_fixed near_z, const jo_vdp1_window * const w, const int right,
                                                                 const int bottom, const int guard)
{
    jo_fixed                                k;
    int                                     sx;
    int                                     sy;
    int                                     i;
    int                                     next;

    // (the next point that is projected: its division is started)
    for (next = 0; next < count; ++next)
        if (points[next][2] > near_z && !jo_3d_is_far_outside(points[next][0], points[next][1], points[next][2]))
            break;
    if (next < count)
        jo_fixed_div_start(__jo_vdp1_projection_distance, points[next][2]);
    for (JO_ZERO(i); i < count; ++i)
    {
        if (i != next)
        {
            // Behind the near plane or very far outside of the screen: the caller keeps the view space position (the part is
            // clipped), the sides of the screen it is outside of
            jo_3d_set_vertex(&v[i], 0, 0, points[i][2], (points[i][2] <= near_z ? JO_3D_CODE_NEAR : JO_3D_CODE_GUARD) | JO_3D_CODE_VIEW |
                             jo_3d_view_side_code(points[i][0], points[i][1], points[i][2], w, right, bottom));
            continue;
        }
        k = jo_fixed_div_result();
        for (++next; next < count; ++next)
            if (points[next][2] > near_z && !jo_3d_is_far_outside(points[next][0], points[next][1], points[next][2]))
                break;
        if (next < count)
            jo_fixed_div_start(__jo_vdp1_projection_distance, points[next][2]);
        sx = JO_3D_WINDOW_CX(w) + jo_fixed_mult_to_int(points[i][0], k);
        sy = JO_3D_WINDOW_CY(w) + jo_fixed_mult_to_int(points[i][1], k);
        jo_3d_set_vertex(&v[i], sx, sy, points[i][2], jo_3d_screen_code(sx, sy, right, bottom, guard));
    }
}

/** @brief Check if a polygon on the screen is big and mostly out of the screen (its bounding box: 2048 pixels at least, more
 *  than a third out of the screen)
 *  @return true if it is
 */
static  __jo_force_inline bool              jo_3d_is_mostly_out_of_screen(const int xmin, const int ymin, const int xmax, const int ymax,
                                                                          const int right, const int bottom)
{
    int                                     area;
    int                                     inside_width;
    int                                     inside_height;

    // (the coordinates are 16 bits: no overflow)
    area = (xmax - xmin) * (ymax - ymin);
    if (area < 2048)
        return (false);
    inside_width = JO_MIN(xmax, right) - JO_MAX(xmin, 0);
    inside_height = JO_MIN(ymax, bottom) - JO_MAX(ymin, 0);
    if (inside_width <= 0 || inside_height <= 0)
        return (true);
    return (inside_width * inside_height * 3 < area * 2);
}

/* (defined with the 3D of the slave CPU below: its commands of the frame) */
static unsigned int                         __jo_3d_slave_command_count;

/** @brief VDP1 commands of the frame made by both CPUs so far (the master CPU: its table, the slave CPU: its commands not
 *  added yet, see jo_3d_slave_end()), read without the cache: the budget of the subdivisions is shared by the 2 CPUs (each
 *  one counting only its own commands, they overflowed the table together)
 *  @param ctx The CPU that draws
 *  @param used Its commands so far (see jo_3d_draw_polygons(): the master CPU, the ones of the current object, added to the
 *  frame at its end; the slave CPU, the ones of its job, the current object too)
 *  @remarks With the commands of the current object: a big mesh near the camera (a room, a ground) was cut in parts without
 *  limit, its commands not counted until its end; the table overflowed, the polygons after were not drawn
 */
static  __jo_force_inline int               jo_3d_frame_command_total(const jo_3d_cpu_context * const ctx, const unsigned int used)
{
    return (__jo_vdp1_frame_command_count() + (int)used +
            (ctx->slave ? 0 : (int)*(volatile unsigned int *)JO_CACHE_THROUGH(&__jo_3d_slave_command_count)));
}

#ifdef JO_TEST_PATH_COUNTS
volatile unsigned int                       __jo_3d_sub_reason = 0;
# define JO_SUB_REASON(R)                   (__jo_3d_sub_reason = (R))
#else
# define JO_SUB_REASON(R)
#endif

/** @brief Check if a textured polygon must be drawn in parts, without the budget of the VDP1 commands (see
 *  jo_3d_subdivision_level()), not a polygon that crosses the near plane: the VDP1 maps the texture without perspective, the error
 *  (pixels, in the middle of the polygon) is about size on the screen x (zmax - zmin) / (zmax + zmin) / 4
 *  @param tiles The tiles of its texture (levels != 0)
 *  @return Parts per side (2 or 4), 0: drawn as one polygon
 */
static  __jo_force_inline int               jo_3d_subdivision_level_unbudgeted(jo_3d_vertex * const v[4], const unsigned int any,
                                                                               const jo_3d_polygon_attribute * const a,
                                                                               const jo_3d_texture_tiles * const tiles)
{
    jo_fixed                                zmin;
    jo_fixed                                zmax;
    int                                     limit;
    int                                     xmin;
    int                                     xmax;
    int                                     ymin;
    int                                     ymax;
    int                                     size;
    int                                     error;
    int                                     x[4];
    int                                     y[4];
    int                                     k;

    // A polygon that goes very far outside of the screen (beyond the guard band): in parts too, the parts outside are not
    // drawn and the parts cut by the edges of the screen are small (cut as one polygon, each part of the fan got the whole
    // texture: the texture broke along the cuts). All the textures with tiles, not only JO_3D_TILES_NEAR_ONLY: a big wall
    // facing the camera 25 units away (no perspective to correct) was cut by the edges of the screen and got its whole
    // texture on the screen (tests/harness/glitch.py, the room)
    // (fewer parts, see jo_3d_set_fewer_parts(): clipped by the CPU at the edges of the screen)
    if (any & JO_3D_CODE_GUARD)
        return (*(volatile bool *)JO_CACHE_THROUGH(&__jo_3d_fewer_parts) ? 0 : (JO_3D_TILE_LEVEL_COUNT(tiles) >= 2 ? 4 : 2));
    // Size on the screen (the vertices are projected)
    // (each position read once)
    for (k = 0; k < 4; ++k)
    {
        x[k] = JO_3D_VERTEX_X(v[k]);
        y[k] = JO_3D_VERTEX_Y(v[k]);
    }
    xmin = JO_MIN(JO_MIN(x[0], x[1]), JO_MIN(x[2], x[3]));
    xmax = JO_MAX(JO_MAX(x[0], x[1]), JO_MAX(x[2], x[3]));
    ymin = JO_MIN(JO_MIN(y[0], y[1]), JO_MIN(y[2], y[3]));
    ymax = JO_MAX(JO_MAX(y[0], y[1]), JO_MAX(y[2], y[3]));
    // A big half transparent polygon mostly out of the screen: in parts, the parts out of the screen are not drawn (the VDP1
    // spends a lot of time on each half transparent pixel, out of the screen too: it reads the frame buffer)
    // The same for a big polygon of a model cut in small quads when it was made (a wall next to the camera: its parts out of the
    // screen cost VDP1 time too, 3 times more in a stairwell)
    // (not with fewer parts, see jo_3d_set_fewer_parts(): the VDP1 draws their pixels out of the screen)
    if (!*(volatile bool *)JO_CACHE_THROUGH(&__jo_3d_fewer_parts) && (any & JO_3D_CODE_SCREEN) &&
        ((a->draw_mode & JO_VDP1_COLOR_CALC_HALF_TRANSPARENT) == JO_VDP1_COLOR_CALC_HALF_TRANSPARENT ||
                                      (tiles->levels & JO_3D_TILES_NEAR_ONLY)) &&
        jo_3d_is_mostly_out_of_screen(xmin, ymin, xmax, ymax, __jo_vdp1_system_clip_right, __jo_vdp1_system_clip_bottom))
        // (2 x 2 parts for an opaque polygon: the parts cost CPU time)
        return (JO_3D_TILE_LEVEL_COUNT(tiles) < 2 ||
                (a->draw_mode & JO_VDP1_COLOR_CALC_HALF_TRANSPARENT) != JO_VDP1_COLOR_CALC_HALF_TRANSPARENT ? 2 : 4);
    // (the models cut in small quads when they were made: their quads are not cut, see jo_3d_set_texture_subdivision_near_only())
    if (tiles->levels & JO_3D_TILES_NEAR_ONLY)
    {
        JO_SUB_REASON(1);
        return (0);
    }
    // (fewer parts: not for the perspective of the texture)
    if (*(volatile bool *)JO_CACHE_THROUGH(&__jo_3d_fewer_parts))
        return (0);
    size = JO_MAX(xmax - xmin, ymax - ymin);
    // (the distortion allowed: the one of the texture, else the default)
    limit = tiles->error != 0 ? tiles->error : JO_3D_SUBDIVISION_ERROR;
    if (size < JO_MULT_BY_4(limit))
    {
        JO_SUB_REASON(2);
        return (0);
    }
    zmin = JO_MIN(JO_MIN(JO_3D_VERTEX_Z(v[0]), JO_3D_VERTEX_Z(v[1])), JO_MIN(JO_3D_VERTEX_Z(v[2]), JO_3D_VERTEX_Z(v[3])));
    // (the ground: only next to the camera, where its texture slides the most; farther, the parts cost CPU time for little)
    if (tiles->depth != 0 && zmin > tiles->depth)
    {
        JO_SUB_REASON(3);
        return (0);
    }
    zmax = JO_MAX(JO_MAX(JO_3D_VERTEX_Z(v[0]), JO_3D_VERTEX_Z(v[1])), JO_MAX(JO_3D_VERTEX_Z(v[2]), JO_3D_VERTEX_Z(v[3])));
    if (zmax - zmin < JO_DIV_BY_8(zmin))
    {
        JO_SUB_REASON(4);
        return (0);
    }
    // (the ratio is less than 1: no overflow)
    error = JO_DIV_BY_4(jo_fixed2int(jo_fixed_mult(jo_int2fixed(JO_MIN(size, 16000)), jo_fixed_div(JO_DIV_BY_2(zmax - zmin), JO_DIV_BY_2(zmax) + JO_DIV_BY_2(zmin)))));
    if (error > JO_MULT_BY_4(limit) && JO_3D_TILE_LEVEL_COUNT(tiles) >= 2)
        return (4);
    JO_SUB_REASON(error > limit ? 0 : 5);
    return (error > limit ? 2 : 0);
}

/** @brief Check if a textured polygon must be drawn in parts (see jo_3d_subdivision_level_unbudgeted())
 *  @return Parts per side (2 or 4), 0: drawn as one polygon
 *  @remarks The budget of the VDP1 commands: the frame is nearly full (a room with many polygons near the camera,
 *  characters), the polygons are not cut any more for the perspective (a texture slides a little), the others still fit
 *  (the commands after the last one were not drawn: polygons missing). Read only for a polygon to cut (3 reads of the
 *  counts of the 2 CPUs through their cache-through addresses for each polygon)
 */
static  __jo_force_inline int               jo_3d_subdivision_level(const jo_3d_cpu_context * const ctx, const unsigned int used,
                                                                    jo_3d_vertex * const v[4], const unsigned int any,
                                                                    const jo_3d_polygon_attribute * const a)
{
    const jo_3d_texture_tiles               *tiles;
    int                                     level;

    if (!(a->sort & JO_3D_USE_TEXTURE))
    {
        JO_SUB_REASON(6);
        return (0);
    }
    tiles = &__jo_3d_tiles[a->texture_id];
    if (tiles->levels == 0)
    {
        JO_SUB_REASON(7);
        return (0);
    }
    // (the parts that cross the near plane are drawn in smaller parts, see jo_3d_emit_tile(); the frame nearly full: 2 x 2
    // parts, they still get their part of the texture)
    if (any & (JO_3D_CODE_NEAR | JO_3D_CODE_VIEW))
        return (JO_3D_TILE_LEVEL_COUNT(tiles) >= 2 && jo_3d_frame_command_total(ctx, used) <= JO_3D_SUBDIVISION_BUDGET ? 4 : 2);
    level = jo_3d_subdivision_level_unbudgeted(v, any, a, tiles);
    return (level != 0 && jo_3d_frame_command_total(ctx, used) > JO_3D_SUBDIVISION_BUDGET ? 0 : level);
}

/** @brief Common data of the parts of a subdivided polygon (see jo_3d_emit_subdivided_polygon()) */
typedef struct
{
    jo_3d_cpu_context                       *ctx;
    jo_fixed                                near_z;
    const jo_vdp1_window                    *w;
    int                                     right;
    int                                     bottom;
    const jo_3d_polygon                     *p;
    const jo_3d_polygon_attribute           *a;
    const jo_3d_texture_tiles               *tiles;
    bool                                    mirrored;
    bool                                    triangle;           // The polygon is a triangle (its last 2 vertices are the same)
    bool                                    flip_x;             // The tiles are read from the right (mirrored or flipped)
    bool                                    flip_y;             // The tiles are read from the bottom (flipped)
    unsigned int                            tile_words[JO_3D_TILE_FINE_LEVELS]; // Texture words of the command of the first tile of each
    unsigned int                            tile_step[JO_3D_TILE_FINE_LEVELS];  // level (address and size, see jo_3d_emit_tile_part()),
                                                                // and from a tile to the next
    jo_vdp1_command                         *commands;
    unsigned short                          *keys;
    unsigned int                            reserved;
    unsigned int                            *used;
    jo_3d_light_context                     *light;
    jo_3d_clip_context                      *context;
}                                           jo_3d_subdivision;

/** @brief Largest distance to the center of the screen of the vertices of a tile drawn by jo_3d_emit_near_tile() (pixels): the
 *  VDP1 spends time on the pixels outside of the screen */
# define JO_3D_NEAR_TILE_LIMIT              (__jo_screen.width)
/** @brief A point of the clipping plane of jo_3d_emit_near_tile() nearer than this to the screen (pixels) is seen */
# define JO_3D_NEAR_TILE_MARGIN             (8)
/** @brief Largest distance to the center of the screen of a corner of a tile placed by jo_3d_extrapolate_near_tile() (pixels):
 *  the guard band (the VDP1 draws the whole tile, its pixels out of the screen too. With 1024, a tile of a big wall 6 units
 *  from the camera had a corner 700 pixels under the screen: the VDP1 was still drawing it at the end of the frame, the
 *  nearest parts drawn after it were lost, holes in the wall (tests/harness/glitch.py, the room, a save state of Mednafen))
 */
# define JO_3D_NEAR_TILE_EXTRAPOLATION_LIMIT (JO_3D_MAX_GUARD_BAND)
/** @brief Largest distance to the center of the screen of a corner of a tile used to place the other corners by
 *  jo_3d_extrapolate_near_tile() (pixels) */
# define JO_3D_NEAR_TILE_ANCHOR_LIMIT       (__jo_screen.width)
/** @brief Smallest part of an edge of a tile in front of the clipping plane for jo_3d_extrapolate_near_tile() (1 / 4: the
 *  edge is drawn 4 times longer than its part on the screen at most) */
# define JO_3D_NEAR_TILE_MIN_PART           (JO_FIXED_1 / 4)

/** @brief Place on the screen the corners of a tile behind the near plane or far outside of the screen (see
 *  jo_3d_emit_near_tile()). The anchors: the corners in front of the plane near the screen (JO_3D_NEAR_TILE_ANCHOR_LIMIT,
 *  else JO_3D_NEAR_TILE_EXTRAPOLATION_LIMIT). Along each edge from an anchor, the point where the edge leaves the screen
 *  is found (P, a part t of the edge), the other corner goes to the anchor + (P - the anchor) / t. The projection of
 *  the edge is a line: the corner is on it, beyond the edge of the screen, and the texture is right up to the edge of
 *  the screen (no squeezed or turned texture). A corner between 2 anchors: the middle of the 2 places, the corner
 *  opposite to the only anchor: a parallelogram. The whole tile is drawn
 *  @param s Subdivided polygon
 *  @param in The 4 corners (view space)
 *  @param screen The 4 corners on the screen (output)
 *  @return false if a corner would be too far from the screen (the tile is drawn another way)
 */
static bool                                 jo_3d_extrapolate_near_tile(const jo_3d_subdivision * const s, const jo_3d_clip_vertex in[4],
                                                                        jo_3d_vertex screen[4])
{
    jo_fixed                                k;
    jo_fixed                                t;
    jo_fixed                                tc;
    long long                               fa;
    long long                               fb;
    jo_fixed                                dx;
    jo_fixed                                dy;
    jo_fixed                                dz;
    jo_fixed                                ex;
    jo_fixed                                ey;
    jo_fixed                                ez;
    jo_fixed                                sx[4];      // (Q16: the corners placed from the exact positions, rounded at the end;
    jo_fixed                                sy[4];      // rounded first, the error grew by 1 / t: the edges 3 to 4 pixels away)
    jo_fixed                                fixed_limit;
    int                                     count[4];
    int                                     seen;
    int                                     i;
    int                                     j;
    int                                     n;
    int                                     side;
    jo_fixed                                clip_z;
    bool                                    anchor[4];
    int                                     limit;

    clip_z = s->near_z;
    // (the screen positions are Q16: below 2^15 pixels)
    fixed_limit = jo_int2fixed(JO_3D_NEAR_TILE_EXTRAPOLATION_LIMIT);
    // The anchors: the corners in front of the near plane near the screen, else farther
    for (limit = JO_3D_NEAR_TILE_ANCHOR_LIMIT; ; limit = JO_3D_NEAR_TILE_EXTRAPOLATION_LIMIT)
    {
        JO_ZERO(seen);
        for (JO_ZERO(i); i < 4; ++i)
        {
            sx[i] = 0;
            sy[i] = 0;
            count[i] = 0;
            anchor[i] = false;
            if (in[i].z > clip_z)
            {
                k = jo_fixed_div(__jo_vdp1_projection_distance, in[i].z);
                // (far from the screen: x * k overflows a jo_fixed, the corner is no anchor)
                if (JO_ABS(jo_fixed_mult_to_int(in[i].x, k)) > limit || JO_ABS(jo_fixed_mult_to_int(in[i].y, k)) > limit)
                    continue;
                sx[i] = jo_fixed_mult(in[i].x, k);
                sy[i] = jo_fixed_mult(in[i].y, k);
                anchor[i] = true;
                ++seen;
            }
        }
        if (seen > 0 || limit == JO_3D_NEAR_TILE_EXTRAPOLATION_LIMIT)
            break;
    }
    // (no anchor, or no other corner: drawn another way; the other corners: one after the other, opposite corners would
    // mean a twisted tile)
    if (seen == 0 || seen == 4 || (seen == 2 && anchor[0] == anchor[2]))
        return (false);
    // The other corners: where they are on the screen when they are in front of the near plane (far outside of the
    // screen, but known: a corner placed along its edges instead was too near, the tile hung in the middle of the
    // screen), else next to an anchor along their edge
    for (JO_ZERO(i); i < 4; ++i)
    {
        if (anchor[i])
            continue;
        // (a little behind the near plane, near the screen: where it is on the screen too. Placed along its edges from 2
        // anchors (the middle of 2 places, each one right along one edge only), its edges were not the ones of the tiles
        // next to it, which place it from one anchor: a pinhole between them where the ground is cut by the near plane,
        // tests/harness/glitch.py; it is the same corner for all of them this way)
        if (in[i].z <= clip_z && in[i].z > JO_DIV_BY_2(clip_z))
        {
            k = jo_fixed_div(__jo_vdp1_projection_distance, in[i].z);
            if (JO_ABS(jo_fixed_mult_to_int(in[i].x, k)) <= JO_3D_NEAR_TILE_ANCHOR_LIMIT &&
                JO_ABS(jo_fixed_mult_to_int(in[i].y, k)) <= JO_3D_NEAR_TILE_ANCHOR_LIMIT)
            {
                sx[i] = jo_fixed_mult(in[i].x, k);
                sy[i] = jo_fixed_mult(in[i].y, k);
                count[i] = 1;
                continue;
            }
        }
        if (in[i].z > clip_z)
        {
            k = jo_fixed_div(__jo_vdp1_projection_distance, in[i].z);
            if (JO_ABS(jo_fixed_mult_to_int(in[i].x, k)) > JO_3D_NEAR_TILE_EXTRAPOLATION_LIMIT ||
                JO_ABS(jo_fixed_mult_to_int(in[i].y, k)) > JO_3D_NEAR_TILE_EXTRAPOLATION_LIMIT)
                return (false);
            sx[i] = jo_fixed_mult(in[i].x, k);
            sy[i] = jo_fixed_mult(in[i].y, k);
            count[i] = 1;
            continue;
        }
        for (side = 1; side <= 3; side += 2)
        {
            n = (i + side) & 3;
            if (!anchor[n])
                continue;
            dx = in[i].x - in[n].x;
            dy = in[i].y - in[n].y;
            dz = in[i].z - in[n].z;
            // The point where the edge leaves the screen: the first of the 5 planes of the screen (left, right, top,
            // bottom, near) crossed from inside to outside. A plane: f(t) <= 0 inside, f linear along the edge
            // (the anchor must be inside: on the screen)
            t = JO_FIXED_1;
            for (JO_ZERO(j); j < 5; ++j)
            {
                switch (j)
                {
                    case 0:
                        fa = (long long)in[n].x * __jo_vdp1_projection_distance - (long long)JO_MULT_BY_65536(__jo_screen.width_2 + JO_3D_NEAR_TILE_MARGIN) * in[n].z;
                        fb = (long long)in[i].x * __jo_vdp1_projection_distance - (long long)JO_MULT_BY_65536(__jo_screen.width_2 + JO_3D_NEAR_TILE_MARGIN) * in[i].z;
                        break;
                    case 1:
                        fa = -(long long)in[n].x * __jo_vdp1_projection_distance - (long long)JO_MULT_BY_65536(__jo_screen.width_2 + JO_3D_NEAR_TILE_MARGIN) * in[n].z;
                        fb = -(long long)in[i].x * __jo_vdp1_projection_distance - (long long)JO_MULT_BY_65536(__jo_screen.width_2 + JO_3D_NEAR_TILE_MARGIN) * in[i].z;
                        break;
                    case 2:
                        fa = (long long)in[n].y * __jo_vdp1_projection_distance - (long long)JO_MULT_BY_65536(__jo_screen.height_2 + JO_3D_NEAR_TILE_MARGIN) * in[n].z;
                        fb = (long long)in[i].y * __jo_vdp1_projection_distance - (long long)JO_MULT_BY_65536(__jo_screen.height_2 + JO_3D_NEAR_TILE_MARGIN) * in[i].z;
                        break;
                    case 3:
                        fa = -(long long)in[n].y * __jo_vdp1_projection_distance - (long long)JO_MULT_BY_65536(__jo_screen.height_2 + JO_3D_NEAR_TILE_MARGIN) * in[n].z;
                        fb = -(long long)in[i].y * __jo_vdp1_projection_distance - (long long)JO_MULT_BY_65536(__jo_screen.height_2 + JO_3D_NEAR_TILE_MARGIN) * in[i].z;
                        break;
                    default:
                        fa = clip_z - in[n].z;
                        fb = clip_z - in[i].z;
                        break;
                }
                if (fa > 0)
                    return (false);
                if (fb > 0)
                {
                    // (the crossing: fa + tc * (fb - fa) = 0, 0 <= tc <= 1. The divider of the CPU (the 64 bits division
                    // of the compiler is slow): both terms made smaller than 2^31 first, the ratio stays the same)
                    fa = -fa;
                    fb += fa;
                    while (fb > 0x7FFFFFFF)
                    {
                        fa >>= 8;
                        fb >>= 8;
                    }
                    tc = jo_fixed_div((jo_fixed)fa, (jo_fixed)fb);
                    t = JO_MIN(t, tc);
                }
            }
            if (t < JO_3D_NEAR_TILE_MIN_PART)
                return (false);
            ex = in[n].x + jo_fixed_mult(dx, t);
            ey = in[n].y + jo_fixed_mult(dy, t);
            ez = in[n].z + jo_fixed_mult(dz, t);
            k = jo_fixed_div(__jo_vdp1_projection_distance, ez);
            // (the point P is near the screen (the margin): P - the anchor is below 2^11 pixels, / t (>= 1 / 4) below 2^13)
            sx[i] += sx[n] + jo_fixed_div(jo_fixed_mult(ex, k) - sx[n], t);
            sy[i] += sy[n] + jo_fixed_div(jo_fixed_mult(ey, k) - sy[n], t);
            ++count[i];
        }
        if (count[i] == 2)
        {
            sx[i] = JO_DIV_BY_2(sx[i]);
            sy[i] = JO_DIV_BY_2(sy[i]);
        }
    }
    // The corner opposite to the only corner seen
    if (seen == 1)
        for (JO_ZERO(i); i < 4; ++i)
            if (!anchor[i] && count[i] == 0)
            {
                sx[i] = sx[(i + 1) & 3] + sx[(i + 3) & 3] - sx[(i + 2) & 3];
                sy[i] = sy[(i + 1) & 3] + sy[(i + 3) & 3] - sy[(i + 2) & 3];
            }
    for (JO_ZERO(i); i < 4; ++i)
    {
        if (JO_ABS(sx[i]) > fixed_limit || JO_ABS(sy[i]) > fixed_limit)
            return (false);
        // (an anchor: rounded down as the projected points (see jo_3d_project_points()), the vertex of the parts next to it;
        // the others to the nearest pixel)
        if (anchor[i])
            jo_3d_set_vertex(&screen[i], JO_3D_WINDOW_CX(s->w) + jo_fixed2int(sx[i]), JO_3D_WINDOW_CY(s->w) + jo_fixed2int(sy[i]),
                             JO_MAX(in[i].z, s->near_z), 0);
        else
            jo_3d_set_vertex(&screen[i], JO_3D_WINDOW_CX(s->w) + jo_fixed2int(sx[i] + JO_FIXED_1_DIV_2),
                             JO_3D_WINDOW_CY(s->w) + jo_fixed2int(sy[i] + JO_FIXED_1_DIV_2), JO_MAX(in[i].z, s->near_z), 0);
    }
    return (true);
}


/** @brief Bytes of a line of a texture: 2 per pixel (RGB), 2 pixels per byte (16 colors) */
static  __jo_force_inline int               jo_3d_tile_line_bytes(const jo_texture_definition * const texture)
{
    return (__jo_sprite_pic[texture - __jo_sprite_def].color_mode == JO_VDP1_TEXTURE_SIZE_SHIFT_16_COLORS ?
            JO_DIV_BY_2(texture->width) : JO_MULT_BY_2(texture->width));
}

/** @brief Tiles per side of a level (no shift by a variable amount: the SH-2 has no barrel shifter) */
static  __jo_force_inline int               jo_3d_tiles_per_side(const int level)
{
    return (level == 0 ? 2 : (level == 1 ? 4 : (level == 2 ? 8 : 16)));
}

/** @brief The sprite of the tiles of a level: its tiles one under the other; the finer levels (see
 *  jo_3d_set_texture_subdivision_fine()) in one sprite, the 8 x 8 tiles then the 16 x 16 tiles (half as high: the first one is
 *  the tile 128 of the sprite at their height)
 *  @param tiles The tiles of the texture
 *  @param level Level (0: 2 x 2 tiles, 1: 4 x 4, 2: 8 x 8, 3: 16 x 16)
 *  @param th Output: the lines of a tile
 *  @param first Output: the index of the first tile of the level in the sprite (tiles of th lines)
 *  @return The sprite
 */
static const jo_texture_definition          *jo_3d_tile_level(const jo_3d_texture_tiles * const tiles, const int level, int * const th,
                                                              int * const first)
{
    const jo_texture_definition             *texture;

    texture = &__jo_sprite_def[tiles->sprite[JO_MIN(level, JO_3D_TILE_LEVELS)]];
    *first = 0;
    // (the sprite: n x n tiles of th lines; the finer one 64 tiles of th8 lines and 256 of th8 / 2: 192 x th8)
    if (level == 0)
        *th = texture->height >> 2;
    else if (level == 1)
        *th = texture->height >> 4;
    else if (level == 2)
        *th = texture->height / 192;
    else
    {
        *th = texture->height / 384;
        *first = 128;
    }
    return (texture);
}
/** @brief A part of a subdivided polygon 1 pixel bigger on the screen on the edges of the polygon only: each of these edges
 *  moves 1 pixel away from the middle of the part (along X or Y: the main direction of its normal). The edges between 2
 *  parts are not moved (the same vertices: the parts don't overlap); an edge of the polygon has new vertices that are not
 *  exactly on the edge of the polygon next to it (a line of background was seen between them)
 *  @param in Corners (A, B, C, D)
 *  @param out Corners moved (the depth and the outcode kept)
 *  @param borders Edges moved: bit k for the edge from the corner k to the corner k + 1
 */
/** @brief One edge of jo_3d_grow_borders(): from the corner k to the corner n (constants once inlined: no shift by a variable
 *  amount, the SH-2 has no barrel shifter) */
static  __jo_force_inline void              jo_3d_grow_edge(const int x[4], const int y[4], int dx[4], int dy[4], const int cx, const int cy,
                                                            const int k, const int n)
{
    int                                     ex;
    int                                     ey;

    // The normal of the edge toward the outside: the side of the middle of the edge, seen from the middle of the part
    // (4 x the distances: no division)
    ex = y[n] - y[k];
    ey = x[k] - x[n];
    if (ex * (JO_MULT_BY_2(x[k] + x[n]) - cx) + ey * (JO_MULT_BY_2(y[k] + y[n]) - cy) < 0)
    {
        ex = -ex;
        ey = -ey;
    }
    if (ex == 0 && ey == 0)
        return;
    if (JO_ABS(ex) >= JO_ABS(ey))
        dx[k] = dx[n] = ex > 0 ? 1 : -1;
    else
        dy[k] = dy[n] = ey > 0 ? 1 : -1;
}

static void                                 jo_3d_grow_borders(jo_3d_vertex * const in[4], jo_3d_vertex out[4], const unsigned int borders)
{
    int                                     x[4];
    int                                     y[4];
    int                                     dx[4];
    int                                     dy[4];
    int                                     cx;
    int                                     cy;

    // (no loops: the arrays are indexed by constants only, GCC keeps them in registers instead of the stack: a store each,
    // the cache writes through)
    x[0] = (short)(in[0]->xy >> 16);
    y[0] = (short)(in[0]->xy & 0xffff);
    x[1] = (short)(in[1]->xy >> 16);
    y[1] = (short)(in[1]->xy & 0xffff);
    x[2] = (short)(in[2]->xy >> 16);
    y[2] = (short)(in[2]->xy & 0xffff);
    x[3] = (short)(in[3]->xy >> 16);
    y[3] = (short)(in[3]->xy & 0xffff);
    cx = x[0] + x[1] + x[2] + x[3];
    cy = y[0] + y[1] + y[2] + y[3];
    dx[0] = dx[1] = dx[2] = dx[3] = 0;
    dy[0] = dy[1] = dy[2] = dy[3] = 0;
    // (in the order of the edges: a corner moved by 2 edges keeps the move of the last one)
    if (borders & 1u)
        jo_3d_grow_edge(x, y, dx, dy, cx, cy, 0, 1);
    if (borders & 2u)
        jo_3d_grow_edge(x, y, dx, dy, cx, cy, 1, 2);
    if (borders & 4u)
        jo_3d_grow_edge(x, y, dx, dy, cx, cy, 2, 3);
    if (borders & 8u)
        jo_3d_grow_edge(x, y, dx, dy, cx, cy, 3, 0);
    out[0].xy = (int)(((unsigned int)((x[0] + dx[0]) & 0xffff) << 16) | (unsigned int)((y[0] + dy[0]) & 0xffff));
    out[0].zc = in[0]->zc;
    out[1].xy = (int)(((unsigned int)((x[1] + dx[1]) & 0xffff) << 16) | (unsigned int)((y[1] + dy[1]) & 0xffff));
    out[1].zc = in[1]->zc;
    out[2].xy = (int)(((unsigned int)((x[2] + dx[2]) & 0xffff) << 16) | (unsigned int)((y[2] + dy[2]) & 0xffff));
    out[2].zc = in[2]->zc;
    out[3].xy = (int)(((unsigned int)((x[3] + dx[3]) & 0xffff) << 16) | (unsigned int)((y[3] + dy[3]) & 0xffff));
    out[3].zc = in[3]->zc;
}

/** @brief The ends of the lines of a part of a subdivided polygon (its edges A-D and B-C: the VDP1 draws the lines from
 *  one to the other) pulled in to this distance (pixels) outside of the screen when one of them is farther out and the
 *  other on the screen (see jo_3d_pull_in_part()). Test: -DJO_3D_PULL_IN_LIMIT=9999 for no pull */
# ifndef JO_3D_PULL_IN_LIMIT
#  define JO_3D_PULL_IN_LIMIT               (64)
# endif

/** @brief Move a corner of a part along an edge to a line of the screen (see jo_3d_pull_in_part())
 *  @param out The corner out of the screen (moved)
 *  @param in The other end of the edge
 *  @param along The axis of the move (0: X, the line is x = at, 1: Y)
 *  @param at The line
 */
static void                                 jo_3d_pull_in_corner(jo_3d_vertex * const out, const jo_3d_vertex * const in, const int along,
                                                                 const int at)
{
    int                                     ox;
    int                                     oy;
    int                                     ix;
    int                                     iy;
    int                                     d;
    jo_fixed                                t;

    ox = JO_3D_VERTEX_X(out);
    oy = JO_3D_VERTEX_Y(out);
    ix = JO_3D_VERTEX_X(in);
    iy = JO_3D_VERTEX_Y(in);
    d = along == 0 ? ix - ox : iy - oy;
    if (d == 0)
        return;
    // (the part of the edge from the corner out to the line, Q16: the corners are at most 2 x the guard band apart)
    t = jo_fixed_div(jo_int2fixed(at - (along == 0 ? ox : oy)), jo_int2fixed(d));
    if (t <= 0 || t >= JO_FIXED_1)
        return;
    jo_3d_set_vertex(out, ox + jo_fixed2int(jo_fixed_mult(jo_int2fixed(ix - ox), t)), oy + jo_fixed2int(jo_fixed_mult(jo_int2fixed(iy - oy), t)),
                     out->zc & ~JO_3D_CODE_MASK, 0);
}

/** @brief Pull in the ends of the lines of a big part of a subdivided polygon that go far out of the screen: when the
 *  edge A-D (or B-C) is beyond a side of the screen by more than JO_3D_PULL_IN_LIMIT and the edge B-C (or A-D) is not,
 *  its 2 corners are moved along the edges A-B and D-C (or B-A and C-D) to that distance from the side. The VDP1
 *  draws the lines from A-D to B-C and spends its time on their pixels out of the screen (a wall next to the camera seen
 *  at a grazing angle: 2 parts of 16 x 16 texels with lines of 480 pixels for 73 on the screen, 25 ms of the frame);
 *  the texture is stretched over the lines kept (the texels of the part out of the screen are drawn on the screen too:
 *  the part is near the camera at a grazing angle, its texels are already stretched)
 *  @param corners The 4 corners projected (A: top left, B: top right, C: bottom right, D: bottom left), moved
 *  @param right Right of the screen (pixels)
 *  @param bottom Bottom of the screen
 */
static void                                 jo_3d_pull_in_part(jo_3d_vertex * const corners[4], const int right, const int bottom)
{
    int                                     x[4];
    int                                     y[4];
    int                                     k;
    int                                     side;
    int                                     at;
    int                                     along;
    bool                                    out[4];

    for (JO_ZERO(k); k < 4; ++k)
    {
        x[k] = JO_3D_VERTEX_X(corners[k]);
        y[k] = JO_3D_VERTEX_Y(corners[k]);
    }
    for (side = 0; side < 4; ++side)
    {
        switch (side)
        {
            case 0:
                along = 0;
                at = -JO_3D_PULL_IN_LIMIT;
                for (JO_ZERO(k); k < 4; ++k)
                    out[k] = x[k] < at;
                break;
            case 1:
                along = 0;
                at = right + JO_3D_PULL_IN_LIMIT;
                for (JO_ZERO(k); k < 4; ++k)
                    out[k] = x[k] > at;
                break;
            case 2:
                along = 1;
                at = -JO_3D_PULL_IN_LIMIT;
                for (JO_ZERO(k); k < 4; ++k)
                    out[k] = y[k] < at;
                break;
            default:
                along = 1;
                at = bottom + JO_3D_PULL_IN_LIMIT;
                for (JO_ZERO(k); k < 4; ++k)
                    out[k] = y[k] > at;
                break;
        }
        // (A-D out, B-C in: A along A-B, D along D-C; B-C out, A-D in: B along B-A, C along C-D)
        if (out[0] && out[3] && !out[1] && !out[2])
        {
            jo_3d_pull_in_corner(corners[0], corners[1], along, at);
            jo_3d_pull_in_corner(corners[3], corners[2], along, at);
        }
        else if (out[1] && out[2] && !out[0] && !out[3])
        {
            jo_3d_pull_in_corner(corners[1], corners[0], along, at);
            jo_3d_pull_in_corner(corners[2], corners[3], along, at);
        }
        else
            continue;
        for (JO_ZERO(k); k < 4; ++k)
        {
            x[k] = JO_3D_VERTEX_X(corners[k]);
            y[k] = JO_3D_VERTEX_Y(corners[k]);
        }
    }
}

/** @brief Check if jo_3d_pull_in_part() would pull in the ends of the lines of a part (an edge A-D or B-C beyond a side of the
 *  screen by more than JO_3D_PULL_IN_LIMIT, the other one not)
 *  @param corners The 4 corners projected (A: top left, B: top right, C: bottom right, D: bottom left)
 *  @param right Right of the screen (pixels)
 *  @param bottom Bottom of the screen
 */
static bool                                 jo_3d_part_is_pulled_in(jo_3d_vertex * const corners[4], const int right, const int bottom)
{
    unsigned int                            out[4];
    int                                     k;
    int                                     x;
    int                                     y;

    // (bit 0: left, 1: right, 2: top, 3: bottom)
    for (JO_ZERO(k); k < 4; ++k)
    {
        x = JO_3D_VERTEX_X(corners[k]);
        y = JO_3D_VERTEX_Y(corners[k]);
        out[k] = (x < -JO_3D_PULL_IN_LIMIT ? 1u : 0u) | (x > right + JO_3D_PULL_IN_LIMIT ? 2u : 0u) |
                 (y < -JO_3D_PULL_IN_LIMIT ? 4u : 0u) | (y > bottom + JO_3D_PULL_IN_LIMIT ? 8u : 0u);
    }
    return (((out[0] & out[3]) & ~(out[1] | out[2])) != 0 || ((out[1] & out[2]) & ~(out[0] | out[3])) != 0);
}

/** @brief Check if a part of a subdivided polygon is big on the screen (see JO_3D_CROP_EXTENT) */
static  __jo_force_inline bool              jo_3d_part_is_big(jo_3d_vertex * const corners[4])
{
    int                                     x[4];
    int                                     y[4];
    int                                     k;

    for (JO_ZERO(k); k < 4; ++k)
    {
        x[k] = JO_3D_VERTEX_X(corners[k]);
        y[k] = JO_3D_VERTEX_Y(corners[k]);
    }
    return (JO_MAX(JO_MAX(x[0], x[1]), JO_MAX(x[2], x[3])) - JO_MIN(JO_MIN(x[0], x[1]), JO_MIN(x[2], x[3])) >= JO_3D_CROP_EXTENT ||
            JO_MAX(JO_MAX(y[0], y[1]), JO_MAX(y[2], y[3])) - JO_MIN(JO_MIN(y[0], y[1]), JO_MIN(y[2], y[3])) >= JO_3D_CROP_EXTENT);
}

/** @brief The range of a part along the lines of its tile that may be on the screen (see jo_3d_crop_part()): outside of it,
 *  the lines of the part (from a point of the edge A-D to the point of the edge B-C at the same fraction) are beyond the
 *  same edge of the screen (with JO_3D_CROP_MARGIN), the whole line out of the screen. An edge of the screen cuts the
 *  edge A-D at f(t) = 0, f linear along it (f <= 0 inside), the edge B-C at g(t) = 0: the lines of the part are beyond
 *  the edge for the t where both f and g are positive, a range with an end at 0 (the start of the part) or at 1 (its end)
 *  @param s Subdivided polygon
 *  @param corners The 4 corners projected (A: top left, B: top right, C: bottom right, D: bottom left)
 *  @param range Output: the range [t0, t1] kept (Q16, 0: the first line of the tile, JO_FIXED_1: its end)
 *  @return false if the range is empty
 *  @remarks A part with both its edges A-D and B-C out of the screen can cover the screen (its lines cross it from one side
 *  to the other): only the lines with both ends beyond the same edge are left out
 */
static bool                                 jo_3d_part_visible_range(const jo_3d_subdivision * const s, jo_3d_vertex * const corners[4],
                                                                     jo_fixed range[2])
{
    int                                     x[4];
    int                                     y[4];
    int                                     f[4];
    jo_fixed                                lo;
    jo_fixed                                hi;
    jo_fixed                                t;
    int                                     j;
    int                                     k;

    for (JO_ZERO(k); k < 4; ++k)
    {
        x[k] = JO_3D_VERTEX_X(corners[k]);
        y[k] = JO_3D_VERTEX_Y(corners[k]);
    }
    range[0] = 0;
    range[1] = JO_FIXED_1;
    for (JO_ZERO(j); j < 4; ++j)
    {
        // (f[0], f[1]: the edge A-D at its start and its end, f[2], f[3]: the edge B-C)
        switch (j)
        {
            case 0:
                f[0] = -x[0] - JO_3D_CROP_MARGIN;
                f[1] = -x[3] - JO_3D_CROP_MARGIN;
                f[2] = -x[1] - JO_3D_CROP_MARGIN;
                f[3] = -x[2] - JO_3D_CROP_MARGIN;
                break;
            case 1:
                f[0] = x[0] - s->right - JO_3D_CROP_MARGIN;
                f[1] = x[3] - s->right - JO_3D_CROP_MARGIN;
                f[2] = x[1] - s->right - JO_3D_CROP_MARGIN;
                f[3] = x[2] - s->right - JO_3D_CROP_MARGIN;
                break;
            case 2:
                f[0] = -y[0] - JO_3D_CROP_MARGIN;
                f[1] = -y[3] - JO_3D_CROP_MARGIN;
                f[2] = -y[1] - JO_3D_CROP_MARGIN;
                f[3] = -y[2] - JO_3D_CROP_MARGIN;
                break;
            default:
                f[0] = y[0] - s->bottom - JO_3D_CROP_MARGIN;
                f[1] = y[3] - s->bottom - JO_3D_CROP_MARGIN;
                f[2] = y[1] - s->bottom - JO_3D_CROP_MARGIN;
                f[3] = y[2] - s->bottom - JO_3D_CROP_MARGIN;
                break;
        }
        // The t where both edges are beyond the edge of the screen: [lo, hi]
        lo = 0;
        hi = JO_FIXED_1;
        for (JO_ZERO(k); k < 4 && lo <= hi; k += 2)
        {
            if (f[k] <= 0 && f[k + 1] <= 0)
                hi = -1;    // (never beyond)
            else if (f[k] > 0 && f[k + 1] <= 0)
            {
                // (beyond at the start: [0, t[)
                t = jo_fixed_div(jo_int2fixed(f[k]), jo_int2fixed(f[k] - f[k + 1]));
                hi = JO_MIN(hi, t);
            }
            else if (f[k] <= 0 && f[k + 1] > 0)
            {
                // (beyond at the end: ]t, 1])
                t = jo_fixed_div(jo_int2fixed(-f[k]), jo_int2fixed(f[k + 1] - f[k]));
                lo = JO_MAX(lo, t);
            }
        }
        if (lo > hi)
            continue;
        if (lo == 0 && hi == JO_FIXED_1)
            return (false);
        if (lo == 0)
            range[0] = JO_MAX(range[0], hi);
        if (hi == JO_FIXED_1)
            range[1] = JO_MIN(range[1], lo);
    }
    return (range[0] < range[1]);
}

/** @brief The lines of a tile kept for a part cut where it leaves the screen (see jo_3d_crop_part())
 *  @param s Subdivided polygon (the tiles read from the bottom: the other end of the tile)
 *  @param th Lines of the tile
 *  @param range The visible range of the part along the lines of the tile (input), the range of the lines kept (output: the
 *  cut on a line of the tile, the texture is not distorted)
 *  @param lines Output: the first line kept and the one after the last
 *  @return false if the whole tile is kept (no cut)
 */
static bool                                 jo_3d_crop_rows(const jo_3d_subdivision * const s, const int th, jo_fixed range[2], int lines[2])
{
    int                                     first;
    int                                     last;

    first = (int)(((long long)range[0] * th) >> 16);
    last = (int)((((long long)range[1] * th) + 0xFFFF) >> 16);
    first = JO_MAX(0, JO_MIN(first, th - 1));
    last = JO_MAX(first + 1, JO_MIN(last, th));
    // (less than a fourth of the lines removed: not worth the CPU time of the cut)
    if (JO_MULT_BY_4(last - first) > 3 * th)
        return (false);
    range[0] = jo_fixed_div(jo_int2fixed(first), jo_int2fixed(th));
    range[1] = jo_fixed_div(jo_int2fixed(last), jo_int2fixed(th));
    if (s->flip_y)
    {
        lines[0] = th - last;
        lines[1] = th - first;
    }
    else
    {
        lines[0] = first;
        lines[1] = last;
    }
    return (true);
}

/** @brief Draw a part of a subdivided polygon cut along the lines of its tile: the texture words of its lines
 *  @param texture Tiles of the level of the part (one under the other in a sprite)
 *  @param row First line of the tile (or of its lines seen) in the sprite
 *  @param lines The first line kept and the one after the last (see jo_3d_crop_rows())
 */
static  __jo_force_inline unsigned int      jo_3d_cropped_texture_words(const jo_texture_definition * const texture, const int row,
                                                                        const int lines[2])
{
    int                                     line_bytes;
    int                                     first;

    line_bytes = jo_3d_tile_line_bytes(texture);
    first = lines[0];
    // (a line starts at a multiple of 8 bytes, see jo_3d_emit_near_tile())
    if (JO_MOD_POW2((row + first) * line_bytes, 8) != 0)
        --first;
    return (((unsigned int)(texture->adr + JO_DIV_BY_8((row + first) * line_bytes)) << 16) |
            ((unsigned int)JO_DIV_BY_8(texture->width) << 8) | (unsigned int)(lines[1] - first));
}

/** @brief Gouraud colors of the corners of a part cut along the lines of its tile (see jo_3d_crop_part())
 *  @param colors Gouraud colors of the 4 corners of the part
 *  @param range The range kept
 *  @param out Output: the colors of the 4 corners of the piece
 */
static void                                 jo_3d_cropped_colors(const unsigned short * const colors, const jo_fixed range[2], unsigned short out[4])
{
    out[0] = jo_3d_gouraud_color(colors, 0, range[0]);
    out[1] = jo_3d_gouraud_color(colors, JO_FIXED_1, range[0]);
    out[2] = jo_3d_gouraud_color(colors, JO_FIXED_1, range[1]);
    out[3] = jo_3d_gouraud_color(colors, 0, range[1]);
}

/** @brief Draw a part of a subdivided polygon partly out of the screen cut along the lines of its tile where it leaves
 *  the screen (see JO_3D_CROP_EXTENT): the piece out of the screen is not drawn. The VDP1 draws the lines of a part from
 *  its edge A-D to its edge B-C with the lines of the tile spread evenly on the screen (no perspective): the piece gets
 *  its lines of the tile, cut on the screen at the same fractions, the pixels drawn are the same (the VDP1 can start a
 *  texture at any line, see jo_3d_emit_near_tile())
 *  @param s Subdivided polygon
 *  @param corners The 4 corners projected (A: top left, B: top right, C: bottom right, D: bottom left)
 *  @param colors Gouraud colors of the 4 corners, JO_NULL: none
 *  @param texture Tiles of the level of the part (one under the other in a sprite)
 *  @param row First line of the tile (or of its lines seen) in the sprite
 *  @param th Its lines
 *  @param borders Edges of the part on the edges of the polygon (see jo_3d_grow_borders()), 0: none
 *  @param result Output: false if the command table is full
 *  @return false if the part is not cut (drawn by the caller)
 */
static bool                                 jo_3d_crop_part(const jo_3d_subdivision * const s, jo_3d_vertex * const corners[4],
                                                            const unsigned short * const colors, const jo_texture_definition * const texture,
                                                            const int row, const int th, const unsigned int borders, bool * const result)
{
    jo_fixed                                range[2];
    jo_3d_vertex                            piece[4];
    jo_3d_vertex                            grown[4];
    jo_3d_vertex                            *v[4];
    unsigned short                          piece_colors[4];
    int                                     lines[2];
    int                                     k;
    int                                     n;

    // (a cut along the lines of the tile removes something only when an end of the tile (its first line: A and B, or its
    // last line: C and D) is out of the screen)
    if (!((jo_3d_screen_code(JO_3D_VERTEX_X(corners[0]), JO_3D_VERTEX_Y(corners[0]), s->right, s->bottom, 0) &
           jo_3d_screen_code(JO_3D_VERTEX_X(corners[1]), JO_3D_VERTEX_Y(corners[1]), s->right, s->bottom, 0) & JO_3D_CODE_SCREEN) ||
          (jo_3d_screen_code(JO_3D_VERTEX_X(corners[2]), JO_3D_VERTEX_Y(corners[2]), s->right, s->bottom, 0) &
           jo_3d_screen_code(JO_3D_VERTEX_X(corners[3]), JO_3D_VERTEX_Y(corners[3]), s->right, s->bottom, 0) & JO_3D_CODE_SCREEN)))
        return (false);
    // The range along the lines of the tile that may be on the screen
    if (!jo_3d_part_visible_range(s, corners, range))
    {
        // (out of the screen: nothing to draw. The words of the tile set by the caller forgotten, as after a piece drawn:
        // the tile of a part that crosses the near plane (see jo_3d_emit_near_tile()) stayed, the next textured polygons of
        // the CPU were drawn with it, the next objects by the slow C loop)
        s->ctx->texture_words = 0;
        *result = true;
        return (true);
    }
    if (!jo_3d_crop_rows(s, th, range, lines))
        return (false);
    // The piece: along A-D and B-C on the screen (the depths too: the sort key)
    for (JO_ZERO(k); k < 4; ++k)
    {
        n = k < 2 ? k : 1 - (k & 1);    // (0, 1, 1, 0: the start of the edge)
        jo_3d_set_vertex(&piece[k],
                         JO_3D_VERTEX_X(corners[n]) + jo_fixed2int(jo_fixed_mult(jo_int2fixed(JO_3D_VERTEX_X(corners[3 - n]) - JO_3D_VERTEX_X(corners[n])), range[k < 2 ? 0 : 1])),
                         JO_3D_VERTEX_Y(corners[n]) + jo_fixed2int(jo_fixed_mult(jo_int2fixed(JO_3D_VERTEX_Y(corners[3 - n]) - JO_3D_VERTEX_Y(corners[n])), range[k < 2 ? 0 : 1])),
                         corners[n]->zc + jo_fixed_mult(corners[3 - n]->zc - corners[n]->zc, range[k < 2 ? 0 : 1]), 0);
        v[k] = &piece[k];
    }
    s->ctx->texture_words = jo_3d_cropped_texture_words(texture, row, lines);
    if (colors != JO_NULL)
        jo_3d_cropped_colors(colors, range, piece_colors);
    // (the edges of the polygon that are kept: the cuts are out of the screen)
    if (borders != 0)
    {
        jo_3d_grow_borders(v, grown, borders & ((range[0] > 0 ? 0u : 1u) | 2u | (range[1] < JO_FIXED_1 ? 0u : 4u) | 8u));
        for (JO_ZERO(k); k < 4; ++k)
            v[k] = &grown[k];
    }
    *result = jo_3d_emit_polygon_call(s->ctx, v[0], v[1], v[2], v[3], colors != JO_NULL ? piece_colors : JO_NULL, s->p, s->a,
                                      s->commands, s->keys, s->reserved, s->used, s->light);
    s->ctx->texture_words = 0;
    return (true);
}

/** @brief Check if projected points are all out of the screen beyond the same edge (nothing to draw)
 *  @param screen The points
 *  @param count Point count
 *  @param right Right edge of the screen (see jo_3d_screen_code())
 *  @param bottom Bottom edge
 */
static bool                                 jo_3d_all_outside(const jo_3d_vertex * const screen, const int count, const int right, const int bottom)
{
    unsigned int                            all;
    int                                     i;

    all = JO_3D_CODE_SCREEN;
    for (JO_ZERO(i); i < count && all != 0; ++i)
        all &= jo_3d_screen_code(JO_3D_VERTEX_X(&screen[i]), JO_3D_VERTEX_Y(&screen[i]), right, bottom, 0);
    return (all != 0);
}

/** @brief Check if projected points are all on the screen */
static bool                                 jo_3d_all_inside(const jo_3d_vertex * const screen, const int count, const int right, const int bottom)
{
    int                                     i;

    for (JO_ZERO(i); i < count; ++i)
        if (jo_3d_screen_code(JO_3D_VERTEX_X(&screen[i]), JO_3D_VERTEX_Y(&screen[i]), right, bottom, 0) & JO_3D_CODE_SCREEN)
            return (false);
    return (true);
}

/** @brief Pixels a point of the part of a tile seen may be outside of the quad drawn for it (see jo_3d_quad_covers()) */
# define JO_3D_NEAR_TILE_COVER_TOLERANCE    (2)

/** @brief Clip a polygon of the screen by the edges of the screen (Sutherland-Hodgman: left, right, top, bottom)
 *  @param x Vertices (screen positions, 12 entries, modified)
 *  @param y Vertices
 *  @param n Vertex count
 *  @param right Right of the screen
 *  @param bottom Bottom of the screen
 *  @param cx Work space (12 entries)
 *  @param cy Work space
 *  @param out_x Output: the vertices of the clipped polygon (x or cx)
 *  @param out_y Output: (y or cy)
 *  @return The vertex count of the clipped polygon (less than 3: nothing on the screen)
 */
static int                                  jo_3d_clip_to_screen(int x[12], int y[12], int n, const int right, const int bottom,
                                                                 int cx[12], int cy[12], int **out_x, int **out_y)
{
    int                                     *sx;
    int                                     *sy;
    int                                     *dx;
    int                                     *dy;
    int                                     *tmp;
    int                                     m;
    int                                     i;
    int                                     j;
    int                                     plane;
    long long                               da;
    long long                               db;

    sx = x;
    sy = y;
    dx = cx;
    dy = cy;
    for (JO_ZERO(plane); plane < 4 && n >= 3; ++plane)
    {
        JO_ZERO(m);
        for (JO_ZERO(i); i < n && m < 11; ++i)
        {
            j = i + 1 < n ? i + 1 : 0;
            switch (plane)
            {
                case 0:
                    da = sx[i];
                    db = sx[j];
                    break;
                case 1:
                    da = right - sx[i];
                    db = right - sx[j];
                    break;
                case 2:
                    da = sy[i];
                    db = sy[j];
                    break;
                default:
                    da = bottom - sy[i];
                    db = bottom - sy[j];
                    break;
            }
            if (da >= 0)
            {
                dx[m] = sx[i];
                dy[m++] = sy[i];
            }
            if ((da >= 0) != (db >= 0))
            {
                dx[m] = sx[i] + (int)((long long)(sx[j] - sx[i]) * da / (da - db));
                dy[m++] = sy[i] + (int)((long long)(sy[j] - sy[i]) * da / (da - db));
            }
        }
        n = m;
        tmp = sx;
        sx = dx;
        dx = tmp;
        tmp = sy;
        sy = dy;
        dy = tmp;
    }
    *out_x = sx;
    *out_y = sy;
    return (n);
}

/** @brief Check that a quad drawn for a tile through the near plane covers the part of the tile that is seen: a polygon
 *  of the screen clipped by the edges of the screen, each of its vertices inside of the quad
 *  @param x Vertices of the polygon (screen positions, 12 entries: the clipping uses them; modified)
 *  @param y Vertices of the polygon
 *  @param n Vertex count (3 to 5)
 *  @param right Right of the screen
 *  @param bottom Bottom of the screen
 *  @param quad The quad (screen positions)
 *  @return true if the part seen is inside of the quad (a convex quad, either winding)
 *  @remarks Only for the tiles through the near plane (a few per frame)
 */
static bool                                 jo_3d_quad_covers(int x[12], int y[12], int n, const int right, const int bottom,
                                                              const jo_3d_vertex * const quad[4])
{
    int                                     cx[12];
    int                                     cy[12];
    int                                     *sx;
    int                                     *sy;
    int                                     i;
    int                                     j;
    int                                     ex;
    int                                     ey;
    long long                               c;
    long long                               area;

    n = jo_3d_clip_to_screen(x, y, n, right, bottom, cx, cy, &sx, &sy);
    // Nothing seen
    if (n < 3)
        return (true);
    // Inside of the 4 edges of the quad (its winding: clockwise or mirrored)
    area = (long long)(JO_3D_VERTEX_X(quad[2]) - JO_3D_VERTEX_X(quad[0])) * (JO_3D_VERTEX_Y(quad[3]) - JO_3D_VERTEX_Y(quad[1])) -
           (long long)(JO_3D_VERTEX_X(quad[3]) - JO_3D_VERTEX_X(quad[1])) * (JO_3D_VERTEX_Y(quad[2]) - JO_3D_VERTEX_Y(quad[0]));
    if (area == 0)
        return (false);
    for (JO_ZERO(j); j < 4; ++j)
    {
        ex = JO_3D_VERTEX_X(quad[(j + 1) & 3]) - JO_3D_VERTEX_X(quad[j]);
        ey = JO_3D_VERTEX_Y(quad[(j + 1) & 3]) - JO_3D_VERTEX_Y(quad[j]);
        for (JO_ZERO(i); i < n; ++i)
        {
            c = (long long)ex * (sy[i] - JO_3D_VERTEX_Y(quad[j])) - (long long)ey * (sx[i] - JO_3D_VERTEX_X(quad[j]));
            if (area < 0)
                c = -c;
            if (c < -(long long)JO_3D_NEAR_TILE_COVER_TOLERANCE * JO_MAX(JO_ABS(ex), JO_ABS(ey)))
                return (false);
        }
    }
    return (true);
}

/** @brief Check that the quad placed by jo_3d_extrapolate_near_tile() covers the part of the tile that is seen: the tile
 *  clipped by the near plane and by the edges of the screen. The corners behind the plane are placed from one or two
 *  anchors along the edges, which is right only along these edges: a corner opposite to the only anchor (a parallelogram)
 *  or between two anchors is approximate. A part of a wall 6 units from the camera was left uncovered (the background seen
 *  through the wall) and its tile drawn beside it (tests/harness/glitch.py, the room)
 *  @param s Subdivided polygon
 *  @param in The 4 corners (view space)
 *  @param screen The quad placed (screen positions)
 *  @return true if every vertex of the part seen is inside the quad
 */
static bool                                 jo_3d_near_tile_covers(const jo_3d_subdivision * const s, const jo_3d_clip_vertex in[4],
                                                                   const jo_3d_vertex screen[4])
{
    const jo_3d_vertex                      *quad[4];
    int                                     x[12];
    int                                     y[12];
    int                                     n;
    int                                     i;
    jo_fixed                                k;
    jo_fixed                                t;
    jo_fixed                                px;
    jo_fixed                                py;
    const jo_3d_clip_vertex                 *a;
    const jo_3d_clip_vertex                 *b;

    // The tile clipped by the near plane, projected
    JO_ZERO(n);
    for (JO_ZERO(i); i < 4; ++i)
    {
        a = &in[i];
        b = &in[(i + 1) & 3];
        if (a->z >= s->near_z)
        {
            k = jo_fixed_div(__jo_vdp1_projection_distance, a->z);
            x[n] = JO_3D_WINDOW_CX(s->w) + jo_fixed_mult_to_int(a->x, k);
            y[n++] = JO_3D_WINDOW_CY(s->w) + jo_fixed_mult_to_int(a->y, k);
        }
        if ((a->z >= s->near_z) != (b->z >= s->near_z))
        {
            t = jo_fixed_div(s->near_z - a->z, b->z - a->z);
            px = a->x + jo_fixed_mult(b->x - a->x, t);
            py = a->y + jo_fixed_mult(b->y - a->y, t);
            k = jo_fixed_div(__jo_vdp1_projection_distance, s->near_z);
            x[n] = JO_3D_WINDOW_CX(s->w) + jo_fixed_mult_to_int(px, k);
            y[n++] = JO_3D_WINDOW_CY(s->w) + jo_fixed_mult_to_int(py, k);
        }
    }
    for (JO_ZERO(i); i < 4; ++i)
        quad[i] = &screen[i];
    return (jo_3d_quad_covers(x, y, n, s->right, s->bottom, quad));
}

/** @brief Draw a tile that crosses the near plane as one quad, clipped by a plane parallel to the near plane only. A clipped
 *  polygon drawn in parts maps the whole tile on each part: the texture would turn near the camera. Each corner of the tile
 *  goes to the vertex of the clipped polygon nearest to it (in the tile), the lines of the tile that are cut are not drawn
 *  (the VDP1 can start a texture at any line). The clipping plane is moved away from the camera until the tile stays near
 *  the screen (JO_3D_NEAR_TILE_LIMIT), the VDP1 doesn't spend time far outside of the screen. When a point of the moved
 *  plane is on the screen, the part removed would be seen: the tile is clipped by the edges of the screen instead
 *  @param s Subdivided polygon
 *  @param view View space positions of the 4 corners
 *  @param colors Gouraud colors of the 4 corners, JO_NULL: none
 *  @param texture Sprite of the tiles of the level (one under the other)
 *  @param tile Index of the tile in the sprite
 *  @param th Height of a tile
 *  @return 1: drawn, 0: the command table is full, -1: too big on the screen (the tile must be clipped by the edges of the screen)
 */
static int                                  jo_3d_emit_near_tile(const jo_3d_subdivision * const s, const jo_fixed view[4][3],
                                                                 const unsigned short * const colors, const jo_texture_definition * const texture,
                                                                 const int tile, const int th)
{
    static const jo_fixed                   corner_u[4] = {0, JO_FIXED_1, JO_FIXED_1, 0};
    static const jo_fixed                   corner_v[4] = {0, 0, JO_FIXED_1, JO_FIXED_1};
    jo_3d_clip_vertex                       in[4];
    jo_3d_clip_vertex                       out[5];
    jo_3d_vertex                            screen[5];
    jo_fixed                                cropped_v[5];
    jo_fixed                                k;
    jo_fixed                                t;
    jo_fixed                                vmin;
    jo_fixed                                vmax;
    const jo_3d_clip_vertex                 *a;
    const jo_3d_clip_vertex                 *b;
    unsigned short                          part[4];
    int                                     idx[4];
    int                                     n;
    int                                     i;
    int                                     j;
    int                                     x;
    int                                     y;
    int                                     d;
    int                                     best;
    int                                     first;
    int                                     last;
    int                                     pass;
    jo_fixed                                clip_z;
    jo_fixed                                needed_z;
    long long                               needed;
    bool                                    flip;
    bool                                    result;
    int                                     near_x[12];
    int                                     near_y[12];
    int                                     near_n;

    for (JO_ZERO(i); i < 4; ++i)
    {
        in[i].x = view[i][0];
        in[i].y = view[i][1];
        in[i].z = view[i][2];
        in[i].u = corner_u[i];
        in[i].v = corner_v[i];
    }
    // The whole tile, its corners behind the plane placed along its edges (when the quad covers what is seen of the tile)
    if (jo_3d_extrapolate_near_tile(s, in, screen) && jo_3d_near_tile_covers(s, in, screen))
    {
        // (out of the screen: the corners placed along the edges of a part of the road under the camera were all under
        // the bottom of the screen, the VDP1 spent its time on the pixels out of the screen)
        if (jo_3d_all_outside(screen, 4, s->right, s->bottom))
            return (1);
        // (big and partly out of the screen: the ends of its lines pulled in (see jo_3d_pull_in_part()), cut where it
        // leaves the screen, see JO_3D_CROP_EXTENT)
        {
            jo_3d_vertex                    *corners[4];

            for (JO_ZERO(i); i < 4; ++i)
                corners[i] = &screen[i];
            if (jo_3d_part_is_big(corners) && !jo_3d_all_inside(screen, 4, s->right, s->bottom))
            {
                jo_3d_pull_in_part(corners, s->right, s->bottom);
                if (jo_3d_crop_part(s, corners, colors, texture, tile * th, th, 0, &result))
                    return (result ? 1 : 0);
            }
        }
        s->ctx->texture_words = ((unsigned int)(texture->adr + JO_DIV_BY_8(tile * th * jo_3d_tile_line_bytes(texture))) << 16) |
                                ((unsigned int)JO_DIV_BY_8(texture->width) << 8) | (unsigned int)th;
        result = jo_3d_emit_polygon_call(s->ctx, &screen[0], &screen[1], &screen[2], &screen[3], colors, s->p, s->a, s->commands, s->keys,
                                    s->reserved, s->used, s->light);
        s->ctx->texture_words = 0;
        return (result ? 1 : 0);
    }
    clip_z = s->near_z;
    JO_ZERO(near_n);
    for (JO_ZERO(pass); ; ++pass)
    {
        // The clipping plane
        JO_ZERO(n);
        for (JO_ZERO(i); i < 4; ++i)
        {
            a = &in[i];
            b = &in[(i + 1) & 3];
            if (a->z >= clip_z)
                out[n++] = *a;
            if ((a->z >= clip_z) != (b->z >= clip_z))
            {
                t = jo_fixed_div(clip_z - a->z, b->z - a->z);
                out[n].x = a->x + jo_fixed_mult(b->x - a->x, t);
                out[n].y = a->y + jo_fixed_mult(b->y - a->y, t);
                out[n].z = clip_z;
                out[n].u = a->u + jo_fixed_mult(b->u - a->u, t);
                out[n].v = a->v + jo_fixed_mult(b->v - a->v, t);
                ++n;
            }
        }
        if (n < 3)
            return (1);
        // The depth where the points of the plane stay near the screen
        JO_ZERO(needed_z);
        // (the division of the next point is computed by the divider of the CPU while a point is projected: its result
        // is read before a return, the divider is free after this loop)
        jo_fixed_div_start(__jo_vdp1_projection_distance, out[0].z);
        for (JO_ZERO(i); i < n; ++i)
        {
            k = jo_fixed_div_result();
            if (i + 1 < n)
                jo_fixed_div_start(__jo_vdp1_projection_distance, out[i + 1].z);
            x = jo_fixed_mult_to_int(out[i].x, k);
            y = jo_fixed_mult_to_int(out[i].y, k);
            // (the plane moved away: a point of the plane on the screen means that the part of the tile removed is seen,
            // the edges of the screen clip it instead)
            if (pass > 0 && out[i].z == clip_z && JO_ABS(x) <= __jo_screen.width_2 + JO_3D_NEAR_TILE_MARGIN &&
                JO_ABS(y) <= __jo_screen.height_2 + JO_3D_NEAR_TILE_MARGIN)
            {
                if (i + 1 < n)
                    (void)jo_fixed_div_result();
                return (-1);
            }
            if (JO_ABS(x) > JO_3D_NEAR_TILE_LIMIT || JO_ABS(y) > JO_3D_NEAR_TILE_LIMIT)
            {
                // A vertex of the tile (not of the plane): far outside of the screen, the edges of the screen clip it
                if (out[i].z != clip_z || pass >= 3)
                {
                    if (i + 1 < n)
                        (void)jo_fixed_div_result();
                    return (-1);
                }
                // (64 bits: the product overflowed above 205 world units to the side with the default view, the tile was
                // drawn with a vertex far outside of the screen)
                needed = ((long long)JO_MAX(JO_ABS(out[i].x), JO_ABS(out[i].y)) * __jo_vdp1_projection_distance) >> 16;
                if (needed > 0x7FFF0000)
                    needed = 0x7FFF0000;
                needed_z = JO_MAX(needed_z, (jo_fixed)needed / JO_3D_NEAR_TILE_LIMIT + JO_FIXED_1);
            }
            jo_3d_set_vertex(&screen[i], JO_3D_WINDOW_CX(s->w) + x, JO_3D_WINDOW_CY(s->w) + y, out[i].z, 0);
            // (the tile clipped by the near plane itself: what must be covered when the plane is moved, see below)
            if (pass == 0)
            {
                near_x[i] = JO_3D_WINDOW_CX(s->w) + x;
                near_y[i] = JO_3D_WINDOW_CY(s->w) + y;
            }
        }
        if (pass == 0)
            near_n = n;
        if (needed_z == 0)
            break;
        clip_z = needed_z;
    }
    // (the plane moved away: the part of the tile removed (between the near plane and the plane) may be seen even when no
    // point of the plane is on the screen: the floor 10 units under the camera, its edge across the bottom of the screen,
    // a wedge of background; a wall next to the camera, the screen inside of the part removed, the background seen through
    // the wall (tests/harness/glitch.py). Nothing drawn: the part clipped by the near plane must not be on the screen)
    if (jo_3d_all_outside(screen, n, s->right, s->bottom))
    {
        if (pass > 0)
        {
            int                             cx[12];
            int                             cy[12];
            int                             *px;
            int                             *py;

            if (jo_3d_clip_to_screen(near_x, near_y, near_n, s->right, s->bottom, cx, cy, &px, &py) >= 3)
                return (-1);
        }
        return (1);
    }
    // The lines of the tile that are seen (in the texture: the vertical flip)
    flip = (s->a->direction & 0x20) != 0;
    vmin = out[0].v;
    vmax = out[0].v;
    for (i = 1; i < n; ++i)
    {
        vmin = JO_MIN(vmin, out[i].v);
        vmax = JO_MAX(vmax, out[i].v);
    }
    if (flip)
    {
        t = vmin;
        vmin = JO_FIXED_1 - vmax;
        vmax = JO_FIXED_1 - t;
    }
    first = JO_MAX(0, (int)(((long long)vmin * th) >> 16));
    last = JO_MIN(th, (int)((((long long)vmax * th) + 0xFFFF) >> 16));
    if (last <= first)
    {
        if (first >= th)
            first = th - 1;
        last = first + 1;
    }
    // (a line of 4 bytes, 16 colors 8 pixels wide: from an even line, the addresses of the VDP1 are multiples of 8 bytes; the
    // tile starts at a multiple of 8 bytes, see jo_3d_set_texture_subdivision_4bits())
    if (JO_MOD_POW2((tile * th + first) * jo_3d_tile_line_bytes(texture), 8) != 0)
        --first;
    // (the width is a multiple of 8 pixels: a line starts at a multiple of 4 bytes, 16 colors, or 16 bytes, RGB)
    s->ctx->texture_words = ((unsigned int)(texture->adr + JO_DIV_BY_8((tile * th + first) * jo_3d_tile_line_bytes(texture))) << 16) |
                            ((unsigned int)JO_DIV_BY_8(texture->width) << 8) | (unsigned int)(last - first);
    for (JO_ZERO(i); i < n; ++i)
    {
        t = flip ? JO_FIXED_1 - out[i].v : out[i].v;
        // (t * th < 2^31: the texture is smaller than 32768 lines. The divider of the CPU: (x << 16) / (d << 16) = x / d)
        t = jo_fixed_div(t * th - JO_MULT_BY_65536(first), JO_MULT_BY_65536(last - first));
        cropped_v[i] = flip ? JO_FIXED_1 - t : t;
    }
    // Each corner of the tile: the nearest vertex
    for (JO_ZERO(j); j < 4; ++j)
    {
        idx[j] = 0;
        best = 0x7FFFFFFF;
        for (JO_ZERO(i); i < n; ++i)
        {
            d = JO_ABS(out[i].u - corner_u[j]) + JO_ABS(cropped_v[i] - corner_v[j]);
            if (d < best)
            {
                best = d;
                idx[j] = i;
            }
        }
        if (colors != JO_NULL)
            part[j] = jo_3d_gouraud_color(colors, out[idx[j]].u, out[idx[j]].v);
    }
    // (the 4 vertices chosen must go around the clipped polygon in its order, each once: a twisted quad (two corners on
    // the same vertex, or the order crossed) covered what is around the tile with its texture. A part cut at a corner
    // by the near plane has 5 vertices, the one left out is a point of the plane: the piece lost is not seen)
    for (JO_ZERO(j), d = 0; j < 4; ++j)
    {
        // (n vertices at most apart: no modulo, it is 4 divisions of the library of the compiler)
        i = idx[(j + 1) & 3] - idx[j];
        if (i < 0)
            i += n;
        if (i == 0)
            return (-1);
        d += i;
    }
    if (d != n)
        return (-1);
    // (the quad must cover what is seen of the tile clipped by the near plane: a part cut at a corner by the near plane has
    // 5 vertices, the one left out was on the screen (the floor 10 units under the camera: a wedge of background between the
    // floor and the bottom of the screen); the plane moved away removes a part that may be seen, see above.
    // tests/harness/glitch.py)
    if (n > 4 || pass > 0)
    {
        int                                 px[12];
        int                                 py[12];
        const jo_3d_vertex                  *quad[4];

        if (pass > 0)
            for (JO_ZERO(i); i < near_n; ++i)
            {
                px[i] = near_x[i];
                py[i] = near_y[i];
            }
        else
            for (JO_ZERO(i); i < n; ++i)
            {
                px[i] = JO_3D_VERTEX_X(&screen[i]);
                py[i] = JO_3D_VERTEX_Y(&screen[i]);
            }
        for (JO_ZERO(j); j < 4; ++j)
            quad[j] = &screen[idx[j]];
        if (!jo_3d_quad_covers(px, py, pass > 0 ? near_n : n, s->right, s->bottom, quad))
            return (-1);
    }
    // (big and partly out of the screen: cut where it leaves the screen, see JO_3D_CROP_EXTENT)
    {
        jo_3d_vertex                        quad[4];
        jo_3d_vertex                        *corners[4];

        for (JO_ZERO(j); j < 4; ++j)
        {
            quad[j] = screen[idx[j]];
            corners[j] = &quad[j];
        }
        if (jo_3d_part_is_big(corners) && !jo_3d_all_inside(quad, 4, s->right, s->bottom) &&
            jo_3d_crop_part(s, corners, colors != JO_NULL ? part : JO_NULL, texture, tile * th + first, last - first, 0, &result))
            return (result ? 1 : 0);
    }
    result = jo_3d_emit_polygon_call(s->ctx, &screen[idx[0]], &screen[idx[1]], &screen[idx[2]], &screen[idx[3]], colors != JO_NULL ? part : JO_NULL,
                                s->p, s->a, s->commands, s->keys, s->reserved, s->used, s->light);
    s->ctx->texture_words = 0;
    return (result ? 1 : 0);
}

/** @brief Draw a part of a subdivided polygon with its tile of the texture. A part that crosses the near plane is drawn in
 *  2 x 2 smaller parts (the next level of tiles): the clipping by the near plane maps the whole tile on each piece of the
 *  part, only a small tile can be distorted this way (else the texture turns and twists near the camera)
 *  @param s Subdivided polygon
 *  @param view View space positions of the 4 corners (A: top left, B: top right, C: bottom right, D: bottom left)
 *  @param corners The 4 corners projected
 *  @param colors Gouraud colors of the 4 corners, JO_NULL: none
 *  @param level Level of the tiles (0: 2 x 2 tiles, 1: 4 x 4 tiles, 2 and 3: the finer tiles, see jo_3d_tile_level())
 *  @param ti Column of the tile (in the polygon)
 *  @param tj Line of the tile
 *  @param parent_edges Edges of the part on the edges of the part it was cut from (bit k: from the corner k to k + 1), 0: none
 *  @return false if the command table is full
 */
/** @brief The last level of jo_3d_emit_tile(): one part, with its tile of the texture (inlined in jo_3d_emit_subdivided_polygon()
 *  for the parts that don't cross the near plane: no call of the recursive jo_3d_emit_tile(), its big stack frame; the
 *  command itself by jo_3d_emit_polygon_call())
 *  @param any The outcodes of the corners (or)
 */
static  __jo_force_inline bool              jo_3d_emit_tile_part(const jo_3d_subdivision * const s, const jo_fixed view[4][3],
                                                                 jo_3d_vertex * const corners[4], const unsigned short * const colors,
                                                                 const int level, const int ti, const int tj, const unsigned int parent_edges,
                                                                 const unsigned int any)
{
    const jo_texture_definition             *texture;
    jo_3d_vertex                            grown[4];
    jo_3d_vertex                            pulled[4];
    jo_3d_vertex                            *pc[4];
    unsigned int                            borders;
    bool                                    result;
    int                                     n;
    int                                     th;
    int                                     tx;
    int                                     ty;
    int                                     k;

    // Tile of the part: the back face of a dual plane polygon is mirrored (see jo_3d_emit_polygon()), the flips
    // (horizontal: 0x10, vertical: 0x20, see JO_VDP1_DIRECTION_FLIP_MASK)
    n = jo_3d_tiles_per_side(level);
    tx = s->flip_x ? n - 1 - ti : ti;
    ty = s->flip_y ? n - 1 - tj : tj;
    // The last level crosses the near plane or goes far outside of the screen: one quad (see jo_3d_emit_near_tile())
    if ((any & (JO_3D_CODE_NEAR | JO_3D_CODE_GUARD)) && !s->triangle)
    {
        // (the tiles of the level are one under the other in a sprite)
        texture = jo_3d_tile_level(s->tiles, level, &th, &k);
        k = jo_3d_emit_near_tile(s, view, colors, texture, k + ty * n + tx, th);
        if (k >= 0)
            return (k != 0);
    }
    s->ctx->texture_words = s->tile_words[level] + (unsigned int)(ty * n + tx) * s->tile_step[level];
    if (!(any & (JO_3D_CODE_NEAR | JO_3D_CODE_GUARD)))
    {
        // (1 pixel more on each side: the edges of the parts are rounded on the screen, the polygon next to the cut
        // polygon (a straight edge) left lines of 1 pixel between them, the background was seen)
        // (not the half transparent polygons: the overlap would be seen)
        // (on the edges of the polygon and the edges of a smaller part on the edges of its parent (T-junctions with the parts
        // next to it) only, see jo_3d_grow_borders(); not the half transparent polygons: the overlap would be seen)
        borders = (tj == 0 ? 1u : 0u) | (ti == n - 1 ? 2u : 0u) | (tj == n - 1 ? 4u : 0u) | (ti == 0 ? 8u : 0u) | parent_edges;
        if (jo_3d_reads_frame_buffer(s->a->draw_mode))
            JO_ZERO(borders);
        // A big part partly out of the screen: cut along the lines of its tile where it leaves the screen (see
        // JO_3D_CROP_EXTENT)
        for (k = 0; k < 4; ++k)
            pc[k] = corners[k];
        if ((any & JO_3D_CODE_SCREEN) && !s->triangle && jo_3d_part_is_big(corners))
        {
            // (the ends of its lines far out of the screen pulled in: its own copies of the corners, the parts next to
            // it keep theirs, see jo_3d_pull_in_part())
            for (k = 0; k < 4; ++k)
            {
                pulled[k] = *corners[k];
                pc[k] = &pulled[k];
            }
            jo_3d_pull_in_part(pc, s->right, s->bottom);
            texture = jo_3d_tile_level(s->tiles, level, &th, &k);
            if (jo_3d_crop_part(s, pc, colors, texture, (k + ty * n + tx) * th, th, borders, &result))
                return (result);
        }
        if (borders == 0)
            result = jo_3d_emit_polygon_call(s->ctx, pc[0], pc[1], pc[2], pc[3], colors, s->p, s->a, s->commands,
                                        s->keys, s->reserved, s->used, s->light);
        else
        {
            jo_3d_grow_borders(pc, grown, borders);
            result = jo_3d_emit_polygon_call(s->ctx, &grown[0], &grown[1], &grown[2], &grown[3], colors, s->p, s->a, s->commands,
                                        s->keys, s->reserved, s->used, s->light);
        }
    }
    else
        result = jo_3d_emit_clipped_polygon(s->ctx, corners, view, any, s->near_z, s->w, s->right, s->bottom, s->p, s->a, colors,
                                            s->commands, s->keys, s->reserved, s->used, s->light, s->context);
    s->ctx->texture_words = 0;
    return (result);
}

/** @brief Check if a part of the last level of the usual tiles is cut into the finer tiles (see jo_3d_emit_tile()): big and its
 *  lines pulled in with its texture (see jo_3d_pull_in_part(): the texture squeezed by up to a tile, 8 texels of a 4 x 4
 *  tile, a wall 25 units in front of the camera, tests/harness/glitch.py), or distorted by the perspective as a polygon cut
 *  in parts (see jo_3d_subdivision_level_unbudgeted(): a part of 8 x 8 texels of a magnified texture is still big); not
 *  when the frame is nearly full (see JO_3D_SUBDIVISION_BUDGET)
 */
static bool                                 jo_3d_fine_part(const jo_3d_subdivision * const s, jo_3d_vertex * const corners[4],
                                                            const unsigned int any)
{
    int                                     x[4];
    int                                     y[4];
    int                                     size;
    int                                     limit;
    jo_fixed                                zmin;
    jo_fixed                                zmax;
    int                                     k;

    // (the budget last: 3 reads without the cache)
    if ((any & JO_3D_CODE_SCREEN) && jo_3d_part_is_big(corners) && jo_3d_part_is_pulled_in(corners, s->right, s->bottom))
        return (jo_3d_frame_command_total(s->ctx, *s->used) <= JO_3D_SUBDIVISION_BUDGET);
    for (JO_ZERO(k); k < 4; ++k)
    {
        x[k] = JO_3D_VERTEX_X(corners[k]);
        y[k] = JO_3D_VERTEX_Y(corners[k]);
    }
    size = JO_MAX(JO_MAX(JO_MAX(x[0], x[1]), JO_MAX(x[2], x[3])) - JO_MIN(JO_MIN(x[0], x[1]), JO_MIN(x[2], x[3])),
                  JO_MAX(JO_MAX(y[0], y[1]), JO_MAX(y[2], y[3])) - JO_MIN(JO_MIN(y[0], y[1]), JO_MIN(y[2], y[3])));
    limit = s->tiles->error != 0 ? s->tiles->error : JO_3D_SUBDIVISION_ERROR;
    if (size < JO_MULT_BY_4(limit))
        return (false);
    zmin = JO_MIN(JO_MIN(JO_3D_VERTEX_Z(corners[0]), JO_3D_VERTEX_Z(corners[1])), JO_MIN(JO_3D_VERTEX_Z(corners[2]), JO_3D_VERTEX_Z(corners[3])));
    zmax = JO_MAX(JO_MAX(JO_3D_VERTEX_Z(corners[0]), JO_3D_VERTEX_Z(corners[1])), JO_MAX(JO_3D_VERTEX_Z(corners[2]), JO_3D_VERTEX_Z(corners[3])));
    // (the ratio is less than 1: no overflow)
    if (JO_DIV_BY_4(jo_fixed2int(jo_fixed_mult(jo_int2fixed(JO_MIN(size, 16000)),
                                               jo_fixed_div(JO_DIV_BY_2(zmax - zmin), JO_DIV_BY_2(zmax) + JO_DIV_BY_2(zmin))))) <= limit)
        return (false);
    return (jo_3d_frame_command_total(s->ctx, *s->used) <= JO_3D_SUBDIVISION_BUDGET);
}

static bool                                 jo_3d_emit_tile(const jo_3d_subdivision * const s, const jo_fixed view[4][3],
                                                            jo_3d_vertex * const corners[4], const unsigned short * const colors,
                                                            const int level, const int ti, const int tj, const unsigned int parent_edges)
{
    jo_fixed                                points[3][3][3];
    jo_3d_vertex                            projected[3][3];
    unsigned short                          point_colors[3][3];
    unsigned short                          part_colors[4];
    jo_fixed                                child[4][3];
    jo_3d_vertex                            *child_corners[4];
    unsigned int                            all;
    unsigned int                            any;
    int                                     i;
    int                                     j;
    int                                     k;

    all = (unsigned int)(corners[0]->zc & corners[1]->zc & corners[2]->zc & corners[3]->zc) & JO_3D_CODE_MASK;
    if (all & (JO_3D_CODE_SCREEN | JO_3D_CODE_NEAR))
        return (true);
    any = (unsigned int)(corners[0]->zc | corners[1]->zc | corners[2]->zc | corners[3]->zc) & JO_3D_CODE_MASK;
    // (a part that crosses the near plane or goes far outside of the screen: smaller parts, the last level is drawn
    // as one quad whose texture is approximate, see jo_3d_emit_near_tile(): the error stays small. The finer tiles (see
    // jo_3d_set_texture_subdivision_fine()): also a big part whose lines would be pulled in with its texture or distorted
    // by the perspective, see jo_3d_fine_part())
    if (level + 1 < JO_3D_TILE_LEVEL_COUNT(s->tiles) &&
        ((any & (JO_3D_CODE_NEAR | JO_3D_CODE_GUARD)) ||
         (level + 1 >= JO_3D_TILE_LEVELS && jo_3d_fine_part(s, corners, any))))
    {
        // The 3 x 3 points of the 4 smaller parts (the middles of the edges and of the part)
        for (JO_ZERO(k); k < 3; ++k)
        {
            points[0][0][k] = view[0][k];
            points[0][2][k] = view[1][k];
            points[2][2][k] = view[2][k];
            points[2][0][k] = view[3][k];
            points[0][1][k] = JO_DIV_BY_2(view[0][k]) + JO_DIV_BY_2(view[1][k]);
            points[2][1][k] = JO_DIV_BY_2(view[3][k]) + JO_DIV_BY_2(view[2][k]);
            points[1][0][k] = JO_DIV_BY_2(view[0][k]) + JO_DIV_BY_2(view[3][k]);
            points[1][2][k] = JO_DIV_BY_2(view[1][k]) + JO_DIV_BY_2(view[2][k]);
            points[1][1][k] = JO_DIV_BY_2(points[0][1][k]) + JO_DIV_BY_2(points[2][1][k]);
        }
        jo_3d_project_points(&projected[0][0], (const jo_fixed (*)[3])points, 9, s->near_z, s->w, s->right, s->bottom,
                             *s->ctx->guard_band);
        if (colors != JO_NULL)
            for (JO_ZERO(j); j < 3; ++j)
                for (JO_ZERO(i); i < 3; ++i)
                    point_colors[j][i] = jo_3d_gouraud_color(colors, JO_FIXED_1 * i / 2, JO_FIXED_1 * j / 2);
        for (JO_ZERO(j); j < 2; ++j)
            for (JO_ZERO(i); i < 2; ++i)
            {
                // A part outside of a side of the screen or behind the near plane: nothing to draw (the test at the start of
                // this function, without the call)
                if ((unsigned int)(projected[j][i].zc & projected[j][i + 1].zc & projected[j + 1][i + 1].zc & projected[j + 1][i].zc) &
                    JO_3D_CODE_MASK & (JO_3D_CODE_SCREEN | JO_3D_CODE_NEAR))
                    continue;
                for (JO_ZERO(k); k < 3; ++k)
                {
                    child[0][k] = points[j][i][k];
                    child[1][k] = points[j][i + 1][k];
                    child[2][k] = points[j + 1][i + 1][k];
                    child[3][k] = points[j + 1][i][k];
                }
                child_corners[0] = &projected[j][i];
                child_corners[1] = &projected[j][i + 1];
                child_corners[2] = &projected[j + 1][i + 1];
                child_corners[3] = &projected[j + 1][i];
                if (colors != JO_NULL)
                {
                    part_colors[0] = point_colors[j][i];
                    part_colors[1] = point_colors[j][i + 1];
                    part_colors[2] = point_colors[j + 1][i + 1];
                    part_colors[3] = point_colors[j + 1][i];
                }
                // (the edges of the part on the edges of its parent: the parts next to it are not cut, see below)
                if (!jo_3d_emit_tile(s, (const jo_fixed (*)[3])child, child_corners, colors != JO_NULL ? part_colors : JO_NULL,
                                     level + 1, JO_MULT_BY_2(ti) + i, JO_MULT_BY_2(tj) + j,
                                     (j == 0 ? 1u : 0u) | (i == 1 ? 2u : 0u) | (j == 1 ? 4u : 0u) | (i == 0 ? 8u : 0u)))
                    return (false);
            }
        return (true);
    }
    return (jo_3d_emit_tile_part(s, view, corners, colors, level, ti, tj, parent_edges, any));
}

/** @brief Gouraud colors of a row of the points of a subdivided polygon (see jo_3d_emit_subdivided_polygon())
 *  @param row Output: the colors of the n + 1 points of the row
 *  @param table Gouraud colors of the 4 vertices (A: top left, B: top right, C: bottom right, D: bottom left)
 *  @param flat The same color for the 4 vertices
 *  @param t Position of the row (0: the edge A B, 1: the edge D C)
 *  @param step Distance between 2 points (1 / n)
 *  @param n Parts per side
 *  @param j Row
 *  @remarks The 2 edges (vertices 0 to 3 and 1 to 2) at t, then between them at u, a sum: the color of the point i (8 bits
 *  weights) is (left x (256 - wu) + right x wu + 1 / 2) >> 16 = (left x 256 + 1 / 2 + i x (right - left) x wu step) >> 16 (the
 *  same result as jo_3d_gouraud_color(): the bilinear weights are the products of these, no multiplication a point)
 *  @remarks Not inlined: in the big jo_3d_emit_subdivided_polygon(), its sums were kept on the stack (a store to the memory
 *  each, the cache writes through)
 */
static  __attribute__((noinline)) void      jo_3d_subdivision_row_colors(unsigned short * const row, const unsigned short * const table,
                                                                         const bool flat, const jo_fixed t, const jo_fixed step,
                                                                         const int n, const int j)
{
    unsigned int                            msb;
    int                                     left0;
    int                                     left1;
    int                                     left2;
    int                                     delta0;
    int                                     delta1;
    int                                     delta2;
    int                                     wu;
    int                                     wv;
    int                                     i;

    if (flat)
    {
        for (JO_ZERO(i); i <= n; ++i)
            row[i] = table[0];
        return;
    }
    // (constant shifts: the SH-2 has no barrel shifter)
    wv = (int)((unsigned int)t >> 8);
    wu = (int)((unsigned int)step >> 8);
    left0 = (table[0] & JO_3D_RGB_COMPONENT_MASK) * (256 - wv) + (table[3] & JO_3D_RGB_COMPONENT_MASK) * wv;
    left1 = ((table[0] >> 5) & JO_3D_RGB_COMPONENT_MASK) * (256 - wv) + ((table[3] >> 5) & JO_3D_RGB_COMPONENT_MASK) * wv;
    left2 = ((table[0] >> 10) & JO_3D_RGB_COMPONENT_MASK) * (256 - wv) + ((table[3] >> 10) & JO_3D_RGB_COMPONENT_MASK) * wv;
    delta0 = ((table[1] & JO_3D_RGB_COMPONENT_MASK) * (256 - wv) + (table[2] & JO_3D_RGB_COMPONENT_MASK) * wv - left0) * wu;
    delta1 = ((((table[1] >> 5) & JO_3D_RGB_COMPONENT_MASK) * (256 - wv) + ((table[2] >> 5) & JO_3D_RGB_COMPONENT_MASK) * wv) - left1) * wu;
    delta2 = ((((table[1] >> 10) & JO_3D_RGB_COMPONENT_MASK) * (256 - wv) + ((table[2] >> 10) & JO_3D_RGB_COMPONENT_MASK) * wv) - left2) * wu;
    left0 = JO_MULT_BY_256(left0) + JO_DIV_BY_2(JO_FIXED_1);
    left1 = JO_MULT_BY_256(left1) + JO_DIV_BY_2(JO_FIXED_1);
    left2 = JO_MULT_BY_256(left2) + JO_DIV_BY_2(JO_FIXED_1);
    msb = table[0] & JO_COLOR_SATURN_MSB;
    for (JO_ZERO(i); i <= n; ++i)
    {
        row[i] = (unsigned short)(msb | (unsigned int)(left0 >> 16) | ((unsigned int)(left1 >> 16) << 5) | ((unsigned int)(left2 >> 16) << 10));
        left0 += delta0;
        left1 += delta1;
        left2 += delta2;
    }
    // (a corner: its color as it is, see jo_3d_gouraud_color())
    if (j == 0)
    {
        row[0] = table[0];
        row[n] = table[1];
    }
    else if (j == n)
    {
        row[0] = table[3];
        row[n] = table[2];
    }
}

/** @brief Draw a textured polygon near the camera in n x n parts, each part with its tile of the texture: the distortion
 *  of the texture (the VDP1 maps it without perspective) and of the clipping by the near plane stays small, the parts
 *  are sorted with their own depth
 *  @return false if the command table is full
 */
#ifdef JO_TEST_SUBDIVISION_COUNT
int                                         __jo_test_subdivision_commands[2];
#endif

static bool                                 jo_3d_emit_subdivided_polygon(jo_3d_cpu_context * const ctx, jo_3d_vertex * const v[4], const int n,
                                                                          const jo_fixed near_z,
                                                                          const jo_vdp1_window * const w, const int right, const int bottom,
                                                                          const jo_3d_polygon * const p, const jo_3d_polygon_attribute * const a,
                                                                          jo_vdp1_command * const commands, unsigned short * const keys,
                                                                          const unsigned int reserved, unsigned int * const used,
                                                                          jo_3d_light_context * const light, jo_3d_clip_context * const context)
{
    jo_3d_vertex                            grid[5][5];
    jo_fixed                                grid_view[5][5][3];
    jo_fixed                                corner_view[4][3];
    jo_3d_subdivision                       s;
    unsigned short                          colors[5][5];
    unsigned short                          table[4];
    unsigned short                          part_colors[4];
    jo_fixed                                view[4][3];
    jo_fixed                                step;
    jo_fixed                                u;
    jo_fixed                                t;
    jo_fixed                                top[3];
    jo_fixed                                bottom_edge[3];
    long long                               facing;
    jo_fixed                                d1[3];
    jo_fixed                                d2[3];
    jo_3d_vertex                            *corners[4];
    const jo_3d_vertex                      *corner_first;
    const jo_3d_vertex                      *corner_last;
    int                                     first;
    int                                     last;
    bool                                    gouraud;
    bool                                    flat;
    bool                                    mirrored;
    bool                                    result;
    bool                                    fine;
    unsigned int                            any;
    const jo_texture_definition             *texture;
    int                                     th;
    int                                     i;
    int                                     j;
    int                                     k;

    // View space positions of the 4 vertices
    if (!context->ready)
    {
        context->ready = true;
        context->inverse_pd = jo_fixed_div(JO_FIXED_1, __jo_vdp1_projection_distance);
        context->planes[0] = jo_fixed_div(JO_MULT_BY_65536(JO_3D_WINDOW_CX(w) + JO_3D_CLIP_MARGIN), __jo_vdp1_projection_distance);
        context->planes[1] = jo_fixed_div(JO_MULT_BY_65536(right + JO_3D_CLIP_MARGIN - JO_3D_WINDOW_CX(w)), __jo_vdp1_projection_distance);
        context->planes[2] = jo_fixed_div(JO_MULT_BY_65536(JO_3D_WINDOW_CY(w) + JO_3D_CLIP_MARGIN), __jo_vdp1_projection_distance);
        context->planes[3] = jo_fixed_div(JO_MULT_BY_65536(bottom + JO_3D_CLIP_MARGIN - JO_3D_WINDOW_CY(w)), __jo_vdp1_projection_distance);
    }
    for (JO_ZERO(k); k < 4; ++k)
        jo_3d_vertex_view(ctx, v[k], w, context->inverse_pd, view[k]);
    // Outside of the view: the 4 vertices beyond the same side of the pyramid of the view (behind the camera too: the
    // planes go through the camera). A polygon that crosses the near plane under the camera (the ground) is not drawn in
    // parts for nothing (each part costs about 25 microseconds)
    {
        unsigned int                        all_sides;
        unsigned int                        sides;

        all_sides = 0xF;
        for (JO_ZERO(k); k < 4 && all_sides != 0; ++k)
        {
            JO_ZERO(sides);
            if (view[k][0] < -jo_fixed_mult(view[k][2], context->planes[0]))
                sides |= 1;
            if (view[k][0] > jo_fixed_mult(view[k][2], context->planes[1]))
                sides |= 2;
            if (view[k][1] < -jo_fixed_mult(view[k][2], context->planes[2]))
                sides |= 4;
            if (view[k][1] > jo_fixed_mult(view[k][2], context->planes[3]))
                sides |= 8;
            all_sides &= sides;
        }
        if (all_sides != 0)
            return (true);
    }
    // Facing: the normal (cross product of the diagonals) toward the camera (front faces are clockwise on the screen)
    for (JO_ZERO(k); k < 3; ++k)
    {
        // (shifted before the difference: 2 corners far on both sides of the camera overflowed)
        d1[k] = (view[2][k] >> 4) - (view[0][k] >> 4);
        d2[k] = (view[3][k] >> 4) - (view[1][k] >> 4);
    }
    // (a big polygon: its diagonals smaller, the product overflowed 64 bits above about 1300 world units for the size and
    // the distance. A small polygon keeps all the bits: a back face of a few units was seen as a front face)
    for (JO_ZERO(k); k < 3 && (JO_ABS(d1[0]) | JO_ABS(d1[1]) | JO_ABS(d1[2]) | JO_ABS(d2[0]) | JO_ABS(d2[1]) | JO_ABS(d2[2])) >= (1 << 18); ++k)
    {
        d1[0] >>= 4; d1[1] >>= 4; d1[2] >>= 4;
        d2[0] >>= 4; d2[1] >>= 4; d2[2] >>= 4;
    }
    facing = (long long)((long long)d1[1] * d2[2] - (long long)d1[2] * d2[1]) * (view[0][0] >> 8) +
             (long long)((long long)d1[2] * d2[0] - (long long)d1[0] * d2[2]) * (view[0][1] >> 8) +
             (long long)((long long)d1[0] * d2[1] - (long long)d1[1] * d2[0]) * (view[0][2] >> 8);
    mirrored = ctx->mirror ? facing > 0 : facing < 0;
    if (mirrored && a->plane == JO_3D_SINGLE_PLANE)
        return (true);
    // Gouraud colors of the corners
    gouraud = (a->draw_mode & JO_VDP1_COLOR_CALC_GOURAUD) != 0;
    if (gouraud)
    {
        jo_3d_read_gouraud_table((unsigned short)(a->gouraud_table + ctx->gouraud_offset), table);
    }
    // The same color for the 4 vertices: nothing to interpolate
    flat = !gouraud || (table[0] == table[1] && table[0] == table[2] && table[0] == table[3]);
    // Points of the grid: bilinear interpolation of the 4 vertices (A: top left, B: top right, C: bottom right, D: bottom left)
    step = n == 4 ? JO_FIXED_1 / 4 : JO_FIXED_1 / 2;
    for (JO_ZERO(j); j <= n; ++j)
    {
        t = step * j;
        for (JO_ZERO(k); k < 3; ++k)
        {
            top[k] = view[0][k] + jo_fixed_mult(view[3][k] - view[0][k], t);
            bottom_edge[k] = view[1][k] + jo_fixed_mult(view[2][k] - view[1][k], t);
        }
        for (JO_ZERO(i); i <= n; ++i)
        {
            u = step * i;
            grid_view[j][i][0] = top[0] + jo_fixed_mult(bottom_edge[0] - top[0], u);
            grid_view[j][i][1] = top[1] + jo_fixed_mult(bottom_edge[1] - top[1], u);
            grid_view[j][i][2] = top[2] + jo_fixed_mult(bottom_edge[2] - top[2], u);
        }
        if (gouraud)
            jo_3d_subdivision_row_colors(colors[j], table, flat, t, step, n, j);
        // The corners of the polygon (projected): their positions on the screen as they are, the same as for the polygons next
        // to it (projected again from the view space, they were rounded differently: a corner of a part was 2 pixels away from
        // the corner of the next polygon, a hole between them), not projected again
        first = 0;
        last = n;
        if (j == 0 || j == n)
        {
            corner_first = v[j == 0 ? 0 : 3];
            corner_last = v[j == 0 ? 1 : 2];
            if (!(corner_first->zc & (JO_3D_CODE_NEAR | JO_3D_CODE_VIEW)))
            {
                grid[j][0] = *corner_first;
                first = 1;
            }
            if (!(corner_last->zc & (JO_3D_CODE_NEAR | JO_3D_CODE_VIEW)))
            {
                grid[j][n] = *corner_last;
                last = n - 1;
            }
        }
        jo_3d_project_points(&grid[j][first], (const jo_fixed (*)[3])&grid_view[j][first], last - first + 1, near_z, w, right, bottom,
                             *ctx->guard_band);
    }
    // Sort key: the key of the whole polygon for JO_3D_SORT_MIN and JO_3D_SORT_MAX (for instance the ground under a wall)
    switch (a->sort & JO_3D_SORT_MASK)
    {
        case JO_3D_SORT_MIN:
            ctx->forced_key = JO_MAX(0, JO_MIN(JO_MIN(v[0]->zc, v[1]->zc), JO_MIN(v[2]->zc, v[3]->zc)) & ~JO_3D_CODE_MASK);
            break;
        case JO_3D_SORT_MAX:
            ctx->forced_key = JO_MAX(JO_MAX(v[0]->zc, v[1]->zc), JO_MAX(v[2]->zc, v[3]->zc)) & ~JO_3D_CODE_MASK;
            break;
        default:
            ctx->forced_key = -1;
            break;
    }
    s.ctx = ctx;
    s.near_z = near_z;
    s.w = w;
    s.right = right;
    s.bottom = bottom;
    s.p = p;
    s.a = a;
    s.tiles = &__jo_3d_tiles[a->texture_id];
    s.mirrored = mirrored;
    s.triangle = v[3] == v[2];
    // Tiles: the back face of a dual plane polygon is mirrored (see jo_3d_emit_polygon()), the flips (horizontal: 0x10,
    // vertical: 0x20, see JO_VDP1_DIRECTION_FLIP_MASK)
    s.flip_x = mirrored != ((a->direction & 0x10) != 0);
    s.flip_y = (a->direction & 0x20) != 0;
    // The tiles of a level are one under the other in a sprite (RGB: 2 bytes per pixel, 16 colors: 2 pixels per byte), their
    // sizes are multiples of 8 bytes (see jo_3d_set_texture_subdivision()): tile k at the address of the sprite + k x the size
    // of a tile / 8 (JO_3D_TILE_FINE_LEVELS levels at most; the parts of n == 2 that cross the near plane are cut again: level 1,
    // and the finer levels for the parts that cross it, see jo_3d_tile_level())
    for (JO_ZERO(k); k < JO_3D_TILE_LEVEL_COUNT(s.tiles); ++k)
    {
        texture = jo_3d_tile_level(s.tiles, k, &th, &i);
        s.tile_step[k] = (unsigned int)JO_DIV_BY_8(th * jo_3d_tile_line_bytes(texture)) << 16;
        s.tile_words[k] = (((unsigned int)texture->adr << 16) | ((unsigned int)JO_DIV_BY_8(texture->width) << 8) | (unsigned int)th) +
                          (unsigned int)i * s.tile_step[k];
    }
    fine = JO_3D_TILE_LEVEL_COUNT(s.tiles) > JO_3D_TILE_LEVELS;
    s.commands = commands;
    s.keys = keys;
    s.reserved = reserved;
    s.used = used;
    s.light = light;
    s.context = context;
    result = true;
#ifdef JO_TEST_SUBDIVISION_COUNT
    k = (int)*used;
#endif
    for (JO_ZERO(j); j < n && result; ++j)
        for (JO_ZERO(i); i < n && result; ++i)
        {
            corners[0] = &grid[j][i];
            corners[1] = &grid[j][i + 1];
            corners[2] = &grid[j + 1][i + 1];
            corners[3] = &grid[j + 1][i];
            // A part outside of a side of the screen or behind the near plane: nothing to draw (the test of jo_3d_emit_tile(),
            // without its call)
            if ((unsigned int)(corners[0]->zc & corners[1]->zc & corners[2]->zc & corners[3]->zc) & JO_3D_CODE_MASK &
                (JO_3D_CODE_SCREEN | JO_3D_CODE_NEAR))
                continue;
            // (the positions in the view space are read only by a part that crosses the near plane or goes far outside of
            // the screen, see jo_3d_emit_tile())
            any = (unsigned int)(corners[0]->zc | corners[1]->zc | corners[2]->zc | corners[3]->zc) & JO_3D_CODE_MASK;
            if (gouraud)
            {
                part_colors[0] = colors[j][i];
                part_colors[1] = colors[j][i + 1];
                part_colors[2] = colors[j + 1][i + 1];
                part_colors[3] = colors[j + 1][i];
            }
            // (most parts: drawn here, no call of the recursive jo_3d_emit_tile(); the finer tiles: the parts cut again too,
            // see jo_3d_fine_part())
            if (!(any & (JO_3D_CODE_NEAR | JO_3D_CODE_GUARD)) && (!fine || n != 4 || !jo_3d_fine_part(&s, corners, any)))
            {
                result = jo_3d_emit_tile_part(&s, (const jo_fixed (*)[3])corner_view, corners, gouraud ? part_colors : JO_NULL, n == 4 ? 1 : 0,
                                              i, j, 0, any);
                continue;
            }
            for (JO_ZERO(k); k < 3; ++k)
            {
                corner_view[0][k] = grid_view[j][i][k];
                corner_view[1][k] = grid_view[j][i + 1][k];
                corner_view[2][k] = grid_view[j + 1][i + 1][k];
                corner_view[3][k] = grid_view[j + 1][i][k];
            }
            result = jo_3d_emit_tile(&s, (const jo_fixed (*)[3])corner_view, corners, gouraud ? part_colors : JO_NULL, n == 4 ? 1 : 0, i, j, 0);
        }
    // The polygon after this one sorted with it (JO_3D_SORT_PREVIOUS: a picture on a wall) is drawn after all its parts: the
    // key of its nearest vertex (the parts are sorted on their centers: all farther. With the key of the last part, the
    // parts nearer than it were drawn over the picture)
    if (ctx->forced_key < 0)
        ctx->previous_polygon_z = JO_MAX(__jo_vdp1_near_z, JO_MIN(JO_MIN(v[0]->zc, v[1]->zc), JO_MIN(v[2]->zc, v[3]->zc)) & ~JO_3D_CODE_MASK);
    ctx->forced_key = -1;
#ifdef JO_TEST_SUBDIVISION_COUNT
    // Test (not for release): the commands of the parts of each CPU in the frame (the polygons drawn in parts: 1 command
    // each else)
    *(volatile int *)JO_CACHE_THROUGH(&__jo_test_subdivision_commands[ctx->slave ? 1 : 0]) += (int)*used - k - 1;
#endif
    return (result);
}

/** @brief The tiles of a 16 colors texture (see jo_3d_set_texture_subdivision()): 2 pixels per byte, the same table */
static bool                                 jo_3d_set_texture_subdivision_4bits(const int sprite_id, jo_3d_texture_tiles * const tiles,
                                                                            const jo_texture_definition * const texture,
                                                                            const unsigned char * const pixels)
{
    unsigned char                           *data;
    int                                     n;
    int                                     level;
    int                                     i;
    int                                     j;
    int                                     y;
    int                                     id;
    int                                     tw;
    int                                     th;

    for (level = 0, n = 2; level < JO_3D_TILE_LEVELS; ++level, n *= 2)
    {
        // The width of a texture is a multiple of 8 pixels, and a tile starts at a multiple of 8 bytes (the unit of the
        // addresses of the VDP1); a part of a tile of 8 pixels (4 bytes a line) starts at an even line (see
        // jo_3d_emit_near_tile(); 16 pixels at least before: the doors of a level, 16 pixels wide strips,
        // had no tiles, cut by the near plane they got their whole texture: the texture squashed)
        tw = texture->width / n;
        th = texture->height / n;
        // (8 pixels only for the first level: a texture of 16 pixels; a wider one keeps its levels of 16 pixels at least,
        // the sprites and the VRAM of the tiles are counted)
        if (JO_MOD_POW2(tw, level == 0 ? 8 : 16) != 0 || tw == 0 || th == 0 || JO_MOD_POW2(th * tw / 2, 8) != 0)
            break;
        // The tiles one under the other: tile (i, j) from the line (j * n + i) * th
        data = (unsigned char *)jo_malloc(tw * th * n * n / 2);
        if (data == JO_NULL)
            break;
        for (JO_ZERO(j); j < n; ++j)
            for (JO_ZERO(i); i < n; ++i)
                for (JO_ZERO(y); y < th; ++y)
                    jo_memcpy(data + ((j * n + i) * th + y) * (tw / 2), pixels + (j * th + y) * (texture->width / 2) + i * (tw / 2), tw / 2);
        id = __jo_sprite_add_4bits(data, tw, th * n * n, __jo_sprite_lut[sprite_id]);
        jo_free(data);
        if (id < 0)
            break;
        tiles->sprite[level] = (unsigned short)id;
        tiles->levels = level + 1;
    }
    return (tiles->levels != 0);
}

/** @brief Make the tiles of a texture (see jo_3d_set_texture_subdivision()) */
static bool                                 jo_3d_make_texture_subdivision(const int sprite_id)
{
    jo_3d_texture_tiles                     *tiles;
    const jo_texture_definition             *texture;
    const unsigned short                    *pixels;
    jo_img                                  img;
    int                                     n;
    int                                     level;
    int                                     i;
    int                                     j;
    int                                     y;
    int                                     id;
    int                                     tw;
    int                                     th;

    if (sprite_id < 0 || sprite_id >= JO_MAX_SPRITE)
        return (false);
    tiles = &__jo_3d_tiles[sprite_id];
    if (tiles->levels != 0)
        return (true);
    // RGB and 16 colors textures (the tiles are copies of parts of the texture in the VRAM, a 16 colors tile uses the color
    // lookup table of the texture)
    if (__jo_sprite_pic[sprite_id].color_mode != JO_VDP1_TEXTURE_SIZE_SHIFT_32K_COLORS &&
        __jo_sprite_pic[sprite_id].color_mode != JO_VDP1_TEXTURE_SIZE_SHIFT_16_COLORS)
        return (false);
    texture = &__jo_sprite_def[sprite_id];
    pixels = (const unsigned short *)(JO_VDP1_VRAM + JO_MULT_BY_8(texture->adr));
    if (__jo_sprite_pic[sprite_id].color_mode == JO_VDP1_TEXTURE_SIZE_SHIFT_16_COLORS)
        return (jo_3d_set_texture_subdivision_4bits(sprite_id, tiles, texture, (const unsigned char *)pixels));
    for (level = 0, n = 2; level < JO_3D_TILE_LEVELS; ++level, n *= 2)
    {
        // The width of a texture is a multiple of 8 pixels
        tw = texture->width / n;
        th = texture->height / n;
        if (JO_MOD_POW2(tw, 8) != 0 || tw == 0 || th == 0)
            break;
        // The tiles one under the other: tile (i, j) from the line (j * n + i) * th
        img.width = tw;
        img.height = th * n * n;
        img.data = (jo_color *)jo_malloc(tw * th * n * n * sizeof(jo_color));
        if (img.data == JO_NULL)
            break;
        for (JO_ZERO(j); j < n; ++j)
            for (JO_ZERO(i); i < n; ++i)
                for (JO_ZERO(y); y < th; ++y)
                    jo_memcpy(img.data + ((j * n + i) * th + y) * tw, pixels + (j * th + y) * texture->width + i * tw, tw * sizeof(jo_color));
        id = jo_sprite_add(&img);
        jo_free(img.data);
        if (id < 0)
            break;
        tiles->sprite[level] = (unsigned short)id;
        tiles->levels = level + 1;
    }
    return (tiles->levels != 0);
}

bool                                        jo_3d_set_texture_subdivision_fine(const int sprite_id)
{
    jo_3d_texture_tiles                     *tiles;
    const jo_texture_definition             *texture;
    const unsigned char                     *pixels;
    unsigned char                           *data;
    jo_img                                  img;
    bool                                    four_bits;
    int                                     fw;
    int                                     th8;
    int                                     height;
    int                                     level;
    int                                     n;
    int                                     tw;
    int                                     th;
    int                                     row;
    int                                     col;
    int                                     c;
    int                                     i;
    int                                     j;
    int                                     x;
    int                                     y;
    int                                     id;

    if (sprite_id < 0 || sprite_id >= JO_MAX_SPRITE || !jo_3d_make_texture_subdivision(sprite_id))
        return (false);
    // (the tiles made here: the polygon loop must know it, see jo_3d_set_texture_subdivision())
    jo_3d_update_texture_flags(sprite_id);
    tiles = &__jo_3d_tiles[sprite_id];
    if (JO_3D_TILE_LEVEL_COUNT(tiles) != JO_3D_TILE_LEVELS)
        return (JO_3D_TILE_LEVEL_COUNT(tiles) == JO_3D_TILE_FINE_LEVELS);
    texture = &__jo_sprite_def[sprite_id];
    four_bits = __jo_sprite_pic[sprite_id].color_mode == JO_VDP1_TEXTURE_SIZE_SHIFT_16_COLORS;
    // The width of the tiles in the sprite: the one of the 8 x 8 tiles, 8 pixels at least (16 for 16 colors: a line of 8
    // bytes, the unit of the addresses of the VDP1, see jo_3d_emit_near_tile()); a narrower tile has each texel repeated
    // (the VDP1 spreads the width of the tile over the line: the same place on the screen)
    fw = JO_MAX(texture->width / 8, four_bits ? 16 : 8);
    if (JO_MOD_POW2(texture->width, 16) != 0 || JO_MOD_POW2(texture->height, 16) != 0 || fw % (texture->width / 8) != 0 ||
        fw % (texture->width / 16) != 0)
        return (false);
    th8 = texture->height / 8;
    height = 64 * th8 + 256 * (th8 / 2);
    data = (unsigned char *)jo_malloc(four_bits ? fw * height / 2 : fw * height * (int)sizeof(jo_color));
    if (data == JO_NULL)
        return (false);
    pixels = (const unsigned char *)(JO_VDP1_VRAM + JO_MULT_BY_8(texture->adr));
    // The 8 x 8 tiles then the 16 x 16 tiles, each one under the other: tile (i, j) of a level from the line
    // first + (j * n + i) * th
    for (row = 0, level = 2; level < JO_3D_TILE_FINE_LEVELS; ++level)
    {
        n = jo_3d_tiles_per_side(level);
        tw = texture->width / n;
        th = texture->height / n;
        for (JO_ZERO(j); j < n; ++j)
            for (JO_ZERO(i); i < n; ++i)
                for (JO_ZERO(y); y < th; ++y, ++row)
                    for (JO_ZERO(x); x < fw; ++x)
                    {
                        col = i * tw + x / (fw / tw);
                        if (four_bits)
                        {
                            // (the first pixel of a byte in its high 4 bits)
                            c = pixels[((j * th + y) * texture->width + col) >> 1];
                            c = (col & 1) ? (c & 0xF) : (c >> 4);
                            if (x & 1)
                                data[(row * fw + x) >> 1] |= (unsigned char)c;
                            else
                                data[(row * fw + x) >> 1] = (unsigned char)(c << 4);
                        }
                        else
                            ((jo_color *)data)[row * fw + x] = ((const jo_color *)pixels)[(j * th + y) * texture->width + col];
                    }
    }
    if (four_bits)
        id = __jo_sprite_add_4bits(data, fw, height, __jo_sprite_lut[sprite_id]);
    else
    {
        img.width = fw;
        img.height = height;
        img.data = (jo_color *)data;
        id = jo_sprite_add(&img);
    }
    jo_free(data);
    if (id < 0)
        return (false);
    tiles->sprite[JO_3D_TILE_LEVELS] = (unsigned short)id;
    tiles->levels = (unsigned char)((tiles->levels & JO_3D_TILES_NEAR_ONLY) | JO_3D_TILE_FINE_LEVELS);
    return (true);
}

bool                                        jo_3d_set_texture_subdivision(const int sprite_id)
{
    bool                                    result;

    if (sprite_id < 0 || sprite_id >= JO_MAX_SPRITE)
        return (false);
    result = jo_3d_make_texture_subdivision(sprite_id);
    // (all the polygons drawn in parts, also for a texture made "near only" before: its tiles are kept)
    __jo_3d_tiles[sprite_id].levels &= (unsigned char)~JO_3D_TILES_NEAR_ONLY;
    jo_3d_update_texture_flags(sprite_id);
    return (result);
}

void                                        jo_3d_set_texture_subdivision_depth(const int sprite_id, const jo_fixed depth)
{
    if (sprite_id >= 0 && sprite_id < JO_MAX_SPRITE)
        __jo_3d_tiles[sprite_id].depth = depth;
}

void                                        jo_3d_set_fewer_parts(const bool enabled)
{
    *(volatile bool *)JO_CACHE_THROUGH(&__jo_3d_fewer_parts) = enabled;
}

void                                        jo_3d_set_texture_subdivision_error(const int sprite_id, const int pixels)
{
    if (sprite_id >= 0 && sprite_id < JO_MAX_SPRITE)
        __jo_3d_tiles[sprite_id].error = (unsigned char)JO_MIN(JO_MAX(pixels, 0), 255);
}

bool                                        jo_3d_set_texture_subdivision_near_only(const int sprite_id)
{
    if (sprite_id < 0 || sprite_id >= JO_MAX_SPRITE)
        return (false);
    if (!jo_3d_make_texture_subdivision(sprite_id))
        return (false);
    __jo_3d_tiles[sprite_id].levels |= JO_3D_TILES_NEAR_ONLY;
    jo_3d_update_texture_flags(sprite_id);
    return (true);
}

/*
** Loop of the polygons of an object: the assembly loop __jo_3d_polygon_loop() tests the polygons and emits the common ones
** (all the vertices projected, no subdivision, front face, rejected back face or back face of a dual plane polygon that is not
** half transparent, sorted by their depths, a texture with its smaller copies or a color, without JO_3D_USE_LIGHT). The others
** go through __jo_3d_draw_polygon_slow() (C). The C loop is used when the CPU context needs it: mirror, front layer, forced
** sort key and tile of a part (subdivided polygons), tint, environment map, pixel count of the profiler.
*/

/** @brief State of the slow path of the polygon loop */
typedef struct
{
    jo_3d_cpu_context                       *ctx;
    jo_fixed                                near_z;
    const jo_vdp1_window                    *w;
    int                                     right;
    int                                     bottom;
    jo_3d_light_context                     *light;
    jo_3d_clip_context                      *clip;
    const jo_3d_polygon                     *polygons;      // The polygons of the object (their index in the messages)
    jo_fixed                                env_axes[2][3]; // The X and Y axes of the view in model space (environment map)
    bool                                    env_ready;
}                                           jo_3d_draw_state;

/** @brief Parameters of __jo_3d_polygon_loop() (the offsets are used by the assembly) */
typedef struct
{
    jo_3d_vertex                            *vertices;      // 0: transformed vertices of the CPU
    jo_fixed                                zlimit;         // 4: depth limit of the window
    unsigned int                            count;          // 8: vertex count (the indexes are checked)
    jo_vdp1_command                         *commands;      // 12
    unsigned short                          *keys;          // 16
    unsigned int                            *used;          // 20
    unsigned int                            reserved;       // 24
    int                                     bias;           // 28: depth bias of the CPU
    unsigned int                            clip_mode;      // 32: draw mode bits of the window
    unsigned short                          *displayed;     // 36: displayed polygon count of the CPU
    jo_fixed                                *previous_z;    // 40: depth of the previous polygon of the CPU (JO_3D_SORT_PREVIOUS)
    const jo_3d_polygon                     *end;           // 44: end of the polygons of the range
    jo_3d_draw_state                        *state;         // 48: state of the slow path
    unsigned int                            gouraud_offset; // 52: offset to the tinted copy of the gouraud tables (a tint, 0: none)
    unsigned int                            no_subdivision; // 56: the budget of the subdivisions of the frame is spent (see
                                                            // JO_3D_SUBDIVISION_BUDGET): the polygons are not cut, fast path
    unsigned int                            pre_clipping;   // 60: JO_VDP1_PRE_CLIPPING_DISABLE for the polygons inside of the
                                                            // screen (no JO_3D_CODE_SCREEN), 0: never (see jo_3d_pre_clipping())
    short                                   window_left;    // 64: the rectangle of a clipped window (clip_mode != 0) in the
    short                                   window_right;   // 66: local coordinates of the window: a polygon out of it is
    short                                   window_top;     // 68: not drawn (see jo_3d_is_outside_window())
    short                                   window_bottom;  // 70
}                                           jo_3d_polygon_loop;
_Static_assert(__builtin_offsetof(jo_3d_polygon_loop, gouraud_offset) == 52, "__jo_3d_polygon_loop: offset of gouraud_offset");
_Static_assert(__builtin_offsetof(jo_3d_polygon_loop, no_subdivision) == 56, "__jo_3d_polygon_loop: offset of no_subdivision");
_Static_assert(__builtin_offsetof(jo_3d_polygon_loop, window_left) == 64 && __builtin_offsetof(jo_3d_polygon_loop, window_bottom) == 70,
               "__jo_3d_polygon_loop: offset of the window");
_Static_assert(__builtin_offsetof(jo_3d_polygon_loop, pre_clipping) == 60, "__jo_3d_polygon_loop: offset of pre_clipping");

/** @brief Check if a polygon (its 4 vertices projected) is outside of the rectangle of a window
 *  @param v The 4 vertices
 *  @param w The window
 *  @return true if all the vertices are on the same side out of the rectangle
 */
static  __jo_force_inline bool              jo_3d_is_outside_window(jo_3d_vertex * const v[4], const jo_vdp1_window * const w)
{
    int                                     x[4];
    int                                     y[4];
    register int                            i;

    for (JO_ZERO(i); i < 4; ++i)
    {
        x[i] = JO_3D_VERTEX_X(v[i]);
        y[i] = JO_3D_VERTEX_Y(v[i]);
    }
    // (the vertices are relative to the local coordinates of the window)
    return (JO_MAX(JO_MAX(x[0], x[1]), JO_MAX(x[2], x[3])) < w->left - w->origin_x ||
            JO_MIN(JO_MIN(x[0], x[1]), JO_MIN(x[2], x[3])) > w->right - w->origin_x ||
            JO_MAX(JO_MAX(y[0], y[1]), JO_MAX(y[2], y[3])) < w->top - w->origin_y ||
            JO_MIN(JO_MIN(y[0], y[1]), JO_MIN(y[2], y[3])) > w->bottom - w->origin_y);
}

/** @brief The polygons of a window drawn with the pre-clipping of the VDP1 disabled when they are inside of the screen (their 4
 *  vertices: the VDP1 tests each line against the clipping area before it draws it, about 4 cycles per line for nothing;
 *  measured in Mednafen: 7 % of the drawing time of the fighters demo)
 *  @param w Window
 *  @return JO_VDP1_PRE_CLIPPING_DISABLE, or 0: a clipped window that is not a screen of its own (the pre-clipping of the
 *  VDP1 skips the lines of a polygon out of its rectangle: kept)
 *  @remarks The image doesn't change: the pixels out of the clipping area are not drawn anyway
 *  @remarks The assembly loop (the outcodes of the vertices, against the screen of the window); the C code tests the screen
 *  positions against the system clipping area, in the windows without clipping (see jo_3d_emit_polygon())
 */
static  __jo_force_inline unsigned int      jo_3d_pre_clipping(const jo_vdp1_window * const w)
{
    return (w->clip_mode != 0 && !w->screen ? 0 : JO_VDP1_PRE_CLIPPING_DISABLE);
}

#ifdef JO_TEST_PATH_COUNTS
/** @brief Test (JO_TEST_PATH_COUNTS, read by the game and printed with the profile): the polygons through the general path,
 *  by CPU (0: master, 4: slave) and reason (0: none, drawn as one polygon by the C code, 1: drawn in parts, 2: clipped,
 *  3: the commands of the parts); [1][0] and [5][0]: the polygons of the C loop (a mirror, a tint, an environment map...).
 *  All the textured polygons with tiles beyond their subdivision depth were in [.][0] (the assembly loop sent them to the
 *  slow path by mistake): 260 per frame, about 3 ms on each CPU */
volatile unsigned int                       __jo_3d_path_counts[8][4];
volatile unsigned int                       __jo_3d_path_reasons[8];
#endif

#ifndef JO_TEST_NO_SLIVER
/** @brief Check if a textured polygon is seen as a thin line (see jo_3d_emit_sliver())
 *  @param v The 4 vertices (projected, inside of the guard band)
 *  @param cross Cross product of the diagonals of its screen positions (the area x 2, front face: positive)
 *  @return true if its extent is at least JO_3D_SLIVER_EXTENT and its mean width under JO_3D_SLIVER_WIDTH
 *  @remarks The assembly loop sends such polygons to jo_3d_draw_polygon_general() (see __jo_3d_polygon_loop(), the label
 *  13; it leaves out the thin lines up and down the screen): the extent is the longest diagonal (the chebyshev length)
 */
static  __jo_force_inline bool              jo_3d_is_sliver(jo_3d_vertex * const v[4], const int cross)
{
    int                                     extent;

    extent = JO_MAX(JO_MAX(JO_ABS(JO_3D_VERTEX_X(v[2]) - JO_3D_VERTEX_X(v[0])), JO_ABS(JO_3D_VERTEX_Y(v[2]) - JO_3D_VERTEX_Y(v[0]))),
                    JO_MAX(JO_ABS(JO_3D_VERTEX_X(v[3]) - JO_3D_VERTEX_X(v[1])), JO_ABS(JO_3D_VERTEX_Y(v[3]) - JO_3D_VERTEX_Y(v[1]))));
    return (extent >= JO_3D_SLIVER_EXTENT && cross < JO_MULT_BY_2(JO_3D_SLIVER_WIDTH) * extent);
}

/** @brief Draw a textured polygon seen as a thin line (see JO_3D_SLIVER_EXTENT) as a polygon of the mean color of its
 *  texture: the strip along the line that holds its 4 corners, the VDP1 draws it in a few lines as long as the strip
 *  (the textured polygon cost about the square of its length)
 *  @param k Parameters of the loop
 *  @param v The 4 vertices (projected, inside of the guard band)
 *  @param p Polygon
 *  @param a Its attribute (its gouraud table is kept: the light)
 *  @return false if the command table is full
 */
static bool                                 jo_3d_emit_sliver(const jo_3d_polygon_loop * const k, jo_3d_vertex * const v[4],
                                                              const jo_3d_polygon * const p, const jo_3d_polygon_attribute * const a)
{
    jo_3d_draw_state                        *s;
    jo_3d_vertex                            strip[4];
    int                                     along[4];
    int                                     across[4];
    int                                     first;
    int                                     last;
    int                                     offset;
    int                                     low;
    int                                     high;
    int                                     i;
    bool                                    vertical;
    bool                                    result;

    s = k->state;
    // The line: along the X axis of the screen, or the Y axis when the polygon is taller than wide
    vertical = JO_MAX(JO_ABS(JO_3D_VERTEX_Y(v[2]) - JO_3D_VERTEX_Y(v[0])), JO_ABS(JO_3D_VERTEX_Y(v[3]) - JO_3D_VERTEX_Y(v[1]))) >
               JO_MAX(JO_ABS(JO_3D_VERTEX_X(v[2]) - JO_3D_VERTEX_X(v[0])), JO_ABS(JO_3D_VERTEX_X(v[3]) - JO_3D_VERTEX_X(v[1])));
    for (JO_ZERO(i); i < 4; ++i)
    {
        along[i] = vertical ? JO_3D_VERTEX_Y(v[i]) : JO_3D_VERTEX_X(v[i]);
        across[i] = vertical ? JO_3D_VERTEX_X(v[i]) : JO_3D_VERTEX_Y(v[i]);
    }
    // Its ends: the first and the last corners along the axis
    JO_ZERO(first);
    JO_ZERO(last);
    for (i = 1; i < 4; ++i)
    {
        if (along[i] < along[first])
            first = i;
        if (along[i] > along[last])
            last = i;
    }
    // The 2 other corners: their distances across the line from one end to the other, the strip holds them (the Q16
    // ratio: the corners are at most 2 x the guard band apart, the division is exact enough)
    JO_ZERO(low);
    JO_ZERO(high);
    for (JO_ZERO(i); i < 4; ++i)
    {
        if (i == first || i == last)
            continue;
        offset = across[i] - across[first] -
                 jo_fixed2int(jo_fixed_mult(jo_int2fixed(across[last] - across[first]),
                                            jo_fixed_div(jo_int2fixed(along[i] - along[first]), jo_int2fixed(along[last] - along[first]))));
        low = JO_MIN(low, offset);
        high = JO_MAX(high, offset);
    }
    // The strip, clockwise on the screen (a front face): from A-D (its start) to B-C (its end), the VDP1 draws it in
    // high - low + 1 lines
    if (vertical)
    {
        jo_3d_set_vertex(&strip[0], across[first] + high, along[first], v[first]->zc, 0);
        jo_3d_set_vertex(&strip[1], across[last] + high, along[last], v[last]->zc, 0);
        jo_3d_set_vertex(&strip[2], across[last] + low, along[last], v[last]->zc, 0);
        jo_3d_set_vertex(&strip[3], across[first] + low, along[first], v[first]->zc, 0);
    }
    else
    {
        jo_3d_set_vertex(&strip[0], along[first], across[first] + low, v[first]->zc, 0);
        jo_3d_set_vertex(&strip[1], along[last], across[last] + low, v[last]->zc, 0);
        jo_3d_set_vertex(&strip[2], along[last], across[last] + high, v[last]->zc, 0);
        jo_3d_set_vertex(&strip[3], along[first], across[first] + high, v[first]->zc, 0);
    }
    // (the sort key of the polygon itself: its 4 depths, see jo_3d_emit_polygon(); the strip has the depths of its ends
    // twice)
    s->ctx->sliver_color = __jo_sprite_mean_color[a->texture_id];
    result = jo_3d_emit_polygon_call(s->ctx, &strip[0], &strip[1], &strip[2], &strip[3], JO_NULL, p, a, k->commands, k->keys,
                                     k->reserved, k->used, s->light);
    s->ctx->sliver_color = 0;
    return (result);
}
#endif

/** @brief Draw one polygon: the general case (see __jo_3d_polygon_loop())
 *  @param k Parameters of the loop
 *  @param p Polygon
 *  @param a Its attribute
 *  @return false if the command table is full (or an index is invalid)
 *  @remarks Called by the assembly loop: not static (not inlined, kept by the link time optimization)
 */
static bool                                 jo_3d_draw_polygon_general(const jo_3d_polygon_loop * const k, const jo_3d_polygon * const p,
                                                                       const jo_3d_polygon_attribute * const a)
{
    jo_3d_draw_state                        *s;
    jo_3d_cpu_context                       *ctx;
    jo_3d_vertex                            *v[4];
    unsigned int                            all;
    unsigned int                            any;
    int                                     level;
    bool                                    result;

    s = k->state;
    ctx = s->ctx;
    if (p->vertices[0] >= k->count || p->vertices[1] >= k->count || p->vertices[2] >= k->count || p->vertices[3] >= k->count)
    {
#ifdef JO_DEBUG
        if (!ctx->slave)
            jo_core_error("Polygon %d: invalid vertex index %d %d %d %d (vertex count=%d)", (int)(p - s->polygons), p->vertices[0],
                          p->vertices[1], p->vertices[2], p->vertices[3], k->count);
#endif
        return (false);
    }
    v[0] = &k->vertices[p->vertices[0]];
    v[1] = &k->vertices[p->vertices[1]];
    v[2] = &k->vertices[p->vertices[2]];
    v[3] = &k->vertices[p->vertices[3]];
    // (the outcodes are in the low bits of the depths: they don't matter in the comparison)
    all = (unsigned int)(v[0]->zc & v[1]->zc & v[2]->zc & v[3]->zc);
    // Outside of the screen, all behind the near plane, or too far
    if ((all & (JO_3D_CODE_SCREEN | JO_3D_CODE_NEAR)) ||
        JO_MIN(JO_MIN(v[0]->zc, v[1]->zc), JO_MIN(v[2]->zc, v[3]->zc)) > k->zlimit)
        return (true);
    any = (unsigned int)(v[0]->zc | v[1]->zc | v[2]->zc | v[3]->zc) & JO_3D_CODE_MASK;
    // Clipped to the rectangle of the window (a mirror): outside of it, not drawn (the VDP1 would go through all its pixels)
    if (k->clip_mode != 0 && !(any & (JO_3D_CODE_NEAR | JO_3D_CODE_VIEW)) && jo_3d_is_outside_window(v, s->w))
        return (true);
    // Near the camera: drawn in parts (see jo_3d_set_texture_subdivision())
    level = jo_3d_subdivision_level(ctx, *k->used, v, any, a);
#ifdef JO_TEST_PATH_COUNTS
    ++__jo_3d_path_counts[ctx->slave ? 4 : 0][level > 0 ? 1 : ((any & (JO_3D_CODE_NEAR | JO_3D_CODE_GUARD)) ? 2 : 0)];
    // (the polygons drawn as one by the general path, by the test of jo_3d_subdivision_level() that said no, master CPU)
    if (!ctx->slave && level == 0 && !(any & (JO_3D_CODE_NEAR | JO_3D_CODE_GUARD)))
        ++__jo_3d_path_reasons[__jo_3d_sub_reason & 7];
#endif
    if (level > 0)
    {
        jo_profile_begin(JO_PROFILE_3D_CLIP);
#ifdef JO_TEST_PATH_COUNTS
        __jo_3d_path_counts[ctx->slave ? 4 : 0][3] -= *k->used;
#endif
        result = jo_3d_emit_subdivided_polygon(ctx, v, level, s->near_z, s->w, s->right, s->bottom, p, a, k->commands, k->keys, k->reserved,
                                               k->used, s->light, s->clip);
#ifdef JO_TEST_PATH_COUNTS
        __jo_3d_path_counts[ctx->slave ? 4 : 0][3] += *k->used;
#endif
        jo_profile_end(JO_PROFILE_3D_CLIP);
        return (result);
    }
    if (!(any & (JO_3D_CODE_NEAR | JO_3D_CODE_GUARD)))
    {
        // Seen as a thin line: a polygon of the mean color of its texture (see jo_3d_emit_sliver())
        // (JO_TEST_NO_SLIVER: drawn as they are, to compare on the console; JO_3D_CROP_EXTENT=9999: no cut of the parts)
#ifndef JO_TEST_NO_SLIVER
        if ((a->sort & JO_3D_USE_TEXTURE) && __jo_sprite_mean_color[a->texture_id] != 0 && !ctx->mirror && a->plane == JO_3D_SINGLE_PLANE)
        {
            int                             cross;

            cross = (JO_3D_VERTEX_X(v[2]) - JO_3D_VERTEX_X(v[0])) * (JO_3D_VERTEX_Y(v[3]) - JO_3D_VERTEX_Y(v[1])) -
                    (JO_3D_VERTEX_X(v[3]) - JO_3D_VERTEX_X(v[1])) * (JO_3D_VERTEX_Y(v[2]) - JO_3D_VERTEX_Y(v[0]));
            if (cross >= 0 && jo_3d_is_sliver(v, cross))
                return (jo_3d_emit_sliver(k, v, p, a));
        }
#endif
        return (jo_3d_emit_polygon_call(ctx, v[0], v[1], v[2], v[3], JO_NULL, p, a, k->commands, k->keys, k->reserved, k->used, s->light));
    }
    // Crossing the near plane or far outside of the screen: clipped
    jo_profile_begin(JO_PROFILE_3D_CLIP);
    result = jo_3d_emit_clipped_polygon(ctx, v, JO_NULL, any, s->near_z, s->w, s->right, s->bottom, p, a, JO_NULL, k->commands, k->keys,
                                        k->reserved, k->used, s->light, s->clip);
    jo_profile_end(JO_PROFILE_3D_CLIP);
    return (result);
}

/** @brief The tile of the environment map of a polygon (the words of the texture of the command, see jo_3d_emit_polygon()) */
static unsigned int                         jo_3d_environment_tile(jo_3d_draw_state * const s, const jo_3d_polygon * const p)
{
    const jo_3d_environment                 *e;
    jo_fixed                                (*m)[3];
    jo_fixed                                length;
    jo_fixed                                u;
    jo_fixed                                v;
    int                                     i;
    int                                     j;
    int                                     k;

    e = &__jo_3d_environments[s->ctx->environment];
    // (no tile: not an environment map. A program without them: the compiler saw a division by 0 in this function and called
    // abort() there, it linked the signals of the C library and their data, about 2.5 KB)
    if (e->tiles <= 0)
        return (0);
    // The X and Y axes of the view in model space (the rows of the rotation, normalized: the scale of the object)
    if (!s->env_ready)
    {
        m = s->ctx->matrix;
        for (JO_ZERO(k); k < 2; ++k)
        {
            s->env_axes[k][0] = m[0][k];
            s->env_axes[k][1] = m[1][k];
            s->env_axes[k][2] = m[2][k];
            length = jo_fixed_sqrt(jo_3d_squared_length((const jo_pos3D_fixed *)s->env_axes[k]));
            if (length > 0)
            {
                s->env_axes[k][0] = jo_fixed_div(s->env_axes[k][0], length);
                s->env_axes[k][1] = jo_fixed_div(s->env_axes[k][1], length);
                s->env_axes[k][2] = jo_fixed_div(s->env_axes[k][2], length);
            }
        }
        s->env_ready = true;
    }
    // The normal in the view: -1 to 1 on X (left to right) and Y (top to bottom): the tile (i, j)
    u = jo_fixed_dot((const jo_fixed *)&p->normal, s->env_axes[0]);
    v = jo_fixed_dot((const jo_fixed *)&p->normal, s->env_axes[1]);
    i = (int)(((long long)(u + JO_FIXED_1) * e->tiles) >> 17);
    j = (int)(((long long)(v + JO_FIXED_1) * e->tiles) >> 17);
    i = JO_MIN(JO_MAX(i, 0), e->tiles - 1);
    j = JO_MIN(JO_MAX(j, 0), e->tiles - 1);
    // (the step and the size of a tile: computed once, a division per polygon was 6 % of the time of the CPU in the
    // environment mapping demo)
    return (((__jo_sprite_def[e->sprite].adr + (unsigned int)(j * e->tiles + i) * e->tile_step) << 16) | e->tile_size);
}

/** @brief The normal of each polygon of a mesh (unit vectors: the cross product of its diagonals) */
static void                                 jo_3d_compute_mesh_normals(jo_3d_mesh * const mesh)
{
    jo_3d_polygon                           *p;
    const jo_pos3D_fixed                    *v[4];
    long long                               a[3];
    long long                               b[3];
    long long                               n[3];
    long long                               length;
    unsigned int                            k;
    int                                     c;

    for (JO_ZERO(k), p = mesh->data.polygons; k < mesh->data.polygon_count; ++k, ++p)
    {
        for (JO_ZERO(c); c < 4; ++c)
            v[c] = &mesh->data.points[p->vertices[c]];
        a[0] = v[2]->x - v[0]->x;
        a[1] = v[2]->y - v[0]->y;
        a[2] = v[2]->z - v[0]->z;
        b[0] = v[3]->x - v[1]->x;
        b[1] = v[3]->y - v[1]->y;
        b[2] = v[3]->z - v[1]->z;
        // (b x a: toward the outside of a polygon that is clockwise seen from the outside, the front face; a x b pointed inside,
        // the reflection was upside down)
        n[0] = b[1] * a[2] - b[2] * a[1];
        n[1] = b[2] * a[0] - b[0] * a[2];
        n[2] = b[0] * a[1] - b[1] * a[0];
        // (smaller until the squares fit in 32 bits: only the direction is kept)
        while (JO_MAX(JO_MAX(JO_ABS(n[0]), JO_ABS(n[1])), JO_ABS(n[2])) > 0x7fff)
        {
            n[0] >>= 1;
            n[1] >>= 1;
            n[2] >>= 1;
        }
        // (bigger when small: the precision of the square root)
        while (JO_MAX(JO_MAX(JO_ABS(n[0]), JO_ABS(n[1])), JO_ABS(n[2])) < 0x2000 && (n[0] | n[1] | n[2]) != 0)
        {
            n[0] <<= 1;
            n[1] <<= 1;
            n[2] <<= 1;
        }
        length = (long long)jo_sqrt((unsigned int)(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]));
        if (length == 0)
        {
            p->normal.x = 0;
            p->normal.y = -JO_FIXED_1;
            p->normal.z = 0;
            continue;
        }
        p->normal.x = (jo_fixed)((n[0] << 16) / length);
        p->normal.y = (jo_fixed)((n[1] << 16) / length);
        p->normal.z = (jo_fixed)((n[2] << 16) / length);
    }
}

bool                                        jo_3d_set_mesh_environment(jo_3d_mesh * const mesh, const int sprite_id, const int tiles)
{
    jo_3d_environment                       *e;
    const jo_texture_definition             *texture;
    const jo_color                          *pixels;
    jo_img                                  img;
    int                                     k;
    int                                     i;
    int                                     j;
    int                                     y;
    int                                     tw;
    int                                     th;
    int                                     id;

    if (mesh == JO_NULL || sprite_id < 0 || sprite_id >= JO_MAX_SPRITE || tiles < 1 ||
        __jo_sprite_pic[sprite_id].color_mode != JO_VDP1_TEXTURE_SIZE_SHIFT_32K_COLORS)
        return (false);
    texture = &__jo_sprite_def[sprite_id];
    tw = texture->width / tiles;
    th = texture->height / tiles;
    if (JO_MOD_POW2(tw, 8) != 0 || tw == 0 || th == 0 || th * tiles * tiles > 0xffff)
        return (false);
    k = jo_3d_environment_of(&mesh->data);
    // (set again: the tiles of the last map given back when they are the last sprite, else they were lost)
    if (k >= 0 && __jo_3d_environments[k].first_sprite > 0 && __jo_3d_environments[k].first_sprite == jo_get_last_sprite_id())
    {
        jo_sprite_free_from(__jo_3d_environments[k].first_sprite);
        k = -1;
    }
    if (k < 0)
        for (JO_ZERO(k); k < JO_3D_ENVIRONMENT_MESHES && __jo_3d_environments[k].key != JO_NULL; ++k)
            ;
    if (k >= JO_3D_ENVIRONMENT_MESHES)
        return (false);
    e = &__jo_3d_environments[k];
    // The tiles one under the other (tile (i, j) from the line (j * tiles + i) * th, see jo_3d_environment_tile())
    pixels = (const jo_color *)(JO_VDP1_VRAM + JO_MULT_BY_8(texture->adr));
    img.width = tw;
    img.height = th * tiles * tiles;
    img.data = (jo_color *)jo_malloc(tw * th * tiles * tiles * sizeof(jo_color));
    if (img.data == JO_NULL)
        return (false);
    for (JO_ZERO(j); j < tiles; ++j)
        for (JO_ZERO(i); i < tiles; ++i)
            for (JO_ZERO(y); y < th; ++y)
                jo_memcpy(img.data + ((j * tiles + i) * th + y) * tw, pixels + (j * th + y) * texture->width + i * tw, tw * sizeof(jo_color));
    id = jo_sprite_add(&img);
    jo_free(img.data);
    if (id < 0)
        return (false);
    e->key = &mesh->data;
    e->sprite = id;
    e->tiles = tiles;
    e->first_sprite = id;
    // (tw is a multiple of 8: the step of the tiles in units of 8 bytes is exact)
    e->tile_step = (unsigned int)JO_DIV_BY_8(th * JO_MULT_BY_2(tw));
    e->tile_size = ((unsigned int)JO_DIV_BY_8(tw) << 8) | (unsigned int)th;
    jo_3d_count_environments();
    // The normals of the polygons: the tiles
    jo_3d_compute_mesh_normals(mesh);
    return (true);
}

bool                                        jo_3d_set_mesh_polygon_environment(jo_3d_mesh * const mesh, const unsigned int index)
{
    jo_3d_polygon_attribute                 *a;
    int                                     k;

    k = mesh != JO_NULL ? jo_3d_environment_of(&mesh->data) : -1;
    if (k < 0 || index >= mesh->data.polygon_count)
        return (false);
    // Textured by a tile (an RGB texture: the gouraud table of the polygon tints it)
    a = &mesh->data.attributes[index];
    a->texture_id = (unsigned short)__jo_3d_environments[k].sprite;
    JO_ADD_FLAG(a->sort, JO_3D_USE_TEXTURE);
    a->direction = JO_VDP1_DIRECTION_COMMAND(JO_VDP1_SPRITE_NO_FLIP);
    a->draw_mode = (a->draw_mode & ~JO_VDP1_COLOR_MODE_MASK) | JO_VDP1_COLOR_MODE_RGB;
    return (true);
}

/** @brief A polygon with the tile of the environment map upright on the screen (see __jo_3d_draw_polygon_slow())
 *  @param k Parameters of the loop
 *  @param p Polygon
 *  @param a Its attribute
 *  @param upright Its copy with the vertices rotated (output)
 *  @return The polygon to draw: p, or upright
 *  @remarks The VDP1 maps the top left corner of the tile on the first vertex and the top edge on the first edge: without
 *  the rotation, the tile turned with the polygon (the reflection of a sphere seen from above, upside down). The first edge is
 *  the one that goes the most to the right on the screen (the polygons are clockwise: the next edges go down then left)
 *  @remarks Not rotated: the polygons with a gouraud table (its colors are in the order of the vertices), the polygons that
 *  are not all projected (behind the near plane, very far outside: their screen positions are not known)
 */
static const jo_3d_polygon                  *jo_3d_environment_upright(const jo_3d_polygon_loop * const k, const jo_3d_polygon * const p,
                                                                       const jo_3d_polygon_attribute * const a, jo_3d_polygon * const upright)
{
    const jo_3d_vertex                      *v[4];
    int                                     x[4];
    int                                     y[4];
    int                                     best;
    int                                     score;
    int                                     first;
    int                                     i;

    if (a->draw_mode & JO_VDP1_COLOR_CALC_GOURAUD)
        return (p);
    for (JO_ZERO(i); i < 4; ++i)
    {
        if (p->vertices[i] >= k->count)
            return (p);
        v[i] = &k->vertices[p->vertices[i]];
        if (v[i]->zc & (JO_3D_CODE_NEAR | JO_3D_CODE_VIEW))
            return (p);
        x[i] = JO_3D_VERTEX_X(v[i]);
        y[i] = JO_3D_VERTEX_Y(v[i]);
    }
    // The edge closest to the direction of the X axis of the screen: dx - |dy| the biggest
    JO_ZERO(first);
    best = (x[1] - x[0]) - JO_ABS(y[1] - y[0]);
    for (i = 1; i < 4; ++i)
    {
        score = (x[(i + 1) & 3] - x[i]) - JO_ABS(y[(i + 1) & 3] - y[i]);
        if (score > best)
        {
            best = score;
            first = i;
        }
    }
    if (first == 0)
        return (p);
    *upright = *p;
    for (JO_ZERO(i); i < 4; ++i)
        upright->vertices[i] = p->vertices[(i + first) & 3];
    return (upright);
}

/** @brief Draw one polygon: the general case (see __jo_3d_polygon_loop()), with the tile of the environment map
 *  @param k Parameters of the loop
 *  @param p Polygon
 *  @param a Its attribute
 *  @return false if the command table is full (or an index is invalid)
 *  @remarks Called by the assembly loop: not static (not inlined, kept by the link time optimization)
 */
bool __attribute__((noinline, used, externally_visible)) __jo_3d_draw_polygon_slow(const jo_3d_polygon_loop * const k, const jo_3d_polygon * const p,
                                                                      const jo_3d_polygon_attribute * const a)
{
    jo_3d_cpu_context                       *ctx;
    jo_3d_polygon                           upright;
    bool                                    result;

    ctx = k->state->ctx;
    if (ctx->environment < 0 || !(a->sort & JO_3D_USE_TEXTURE) || a->texture_id != __jo_3d_environments[ctx->environment].sprite)
        return (jo_3d_draw_polygon_general(k, p, a));
    // A polygon that would not be drawn: skipped before its tile (the back faces of a closed mesh, half of its polygons: the
    // tile, the upright vertices and the general path cost about 3 times the drawing of the front faces in the environment
    // mapping demo). Back face: both of its triangles face away, for both of its diagonals (the vertices of the upright copy
    // are rotated: the general path splits it along one or the other)
    if (a->plane == JO_3D_SINGLE_PLANE && !ctx->mirror && p->vertices[0] < k->count && p->vertices[1] < k->count &&
        p->vertices[2] < k->count && p->vertices[3] < k->count)
    {
        const jo_3d_vertex                  *v[4];
        int                                 x[4];
        int                                 y[4];
        int                                 cross;
        int                                 tri;
        int                                 i;

        for (JO_ZERO(i); i < 4; ++i)
            v[i] = &k->vertices[p->vertices[i]];
        if ((v[0]->zc & v[1]->zc & v[2]->zc & v[3]->zc) & (JO_3D_CODE_SCREEN | JO_3D_CODE_NEAR))
            return (true);
        if (((v[0]->zc | v[1]->zc | v[2]->zc | v[3]->zc) & JO_3D_CODE_MASK) == 0)
        {
            for (JO_ZERO(i); i < 4; ++i)
            {
                x[i] = JO_3D_VERTEX_X(v[i]);
                y[i] = JO_3D_VERTEX_Y(v[i]);
            }
            // (the cross product of the diagonals: the sum of the areas x 2 of the triangles of either diagonal, clockwise,
            // front, positive; then the triangles 0 1 2 and 1 2 3: the others are the sum minus them)
            cross = (x[2] - x[0]) * (y[3] - y[1]) - (x[3] - x[1]) * (y[2] - y[0]);
            if (cross < 0)
            {
                tri = (x[1] - x[0]) * (y[2] - y[0]) - (y[1] - y[0]) * (x[2] - x[0]);
                if (tri < 0 && cross - tri < 0)
                {
                    tri = (x[2] - x[1]) * (y[3] - y[1]) - (y[2] - y[1]) * (x[3] - x[1]);
                    if (tri < 0 && cross - tri < 0)
                        return (true);
                }
            }
        }
    }
    ctx->texture_words = jo_3d_environment_tile(k->state, p);
    // The tile kept upright on the screen (a reflection doesn't turn with the polygon): its vertices rotated
    result = jo_3d_draw_polygon_general(k, jo_3d_environment_upright(k, p, a, &upright), a);
    ctx->texture_words = 0;
    return (result);
}

/** @brief Light the command of a polygon written by the assembly loop (as jo_3d_emit_polygon() does after the command)
 *  @param k Parameters of the loop
 *  @param cmd The command
 *  @param p Polygon (JO_3D_USE_LIGHT)
 *  @param a Its attribute
 *  @remarks Called by the assembly loop: not static (not inlined, kept by the link time optimization)
 */
void __attribute__((noinline, used, externally_visible)) __jo_3d_light_command(const jo_3d_polygon_loop * const k, jo_vdp1_command * const cmd,
                                                                  const jo_3d_polygon * const p, const jo_3d_polygon_attribute * const a)
{
    if (!jo_3d_is_lit(a))
        return;
    jo_profile_begin(JO_PROFILE_3D_LIGHT);
    jo_3d_apply_light(cmd, k->state->light, p, a);
    jo_profile_end(JO_PROFILE_3D_LIGHT);
}

/** @brief Loop of the polygons of a range (assembly, see above)
 *  @param k Parameters
 *  @param p First polygon
 *  @param a Its attribute
 *  @return false if the command table is full
 */
extern bool                                 __jo_3d_polygon_loop(const jo_3d_polygon_loop * const k, const jo_3d_polygon *p,
                                                                 const jo_3d_polygon_attribute *a);

__asm__(
     /* JO3DMIPABSMAX: acc = max(acc, |r0|). JO3DMIPEDGE: the longest of |dx| and |dy| of the edge p q in acc (r0, r3 used).
        (the names of the macros of gas ignore the case: not the name of an instruction) */
"    .macro JO3DMIPABSMAX acc\n"
"    cmp/pz r0\n"
"    bt 30f\n"
"    neg r0, r0\n"
"30: cmp/gt \\acc, r0\n"
"    bf 31f\n"
"    mov r0, \\acc\n"
"31:\n"
"    .endm\n"
"    .macro JO3DMIPEDGE p, q, acc\n"
"    mov \\q, r0\n"
"    sub \\p, r0\n"
"    exts.w r0, r0\n"
"    JO3DMIPABSMAX \\acc\n"
"    swap.w \\q, r0\n"
"    swap.w \\p, r3\n"
"    sub r3, r0\n"
"    exts.w r0, r0\n"
"    JO3DMIPABSMAX \\acc\n"
"    .endm\n"
     /* JO3DMIDDLE: acc += 2 x the longest of |dx| and |dy| of (q - p) + (r - s), the screen positions packed (r0, r3 and the
        words 4 and 8 of the stack used) */
"    .macro JO3DMIDDLE p, q, r, s, acc\n"
"    mov \\q, r0\n"
"    sub \\p, r0\n"
"    exts.w r0, r0\n"
"    mov \\r, r3\n"
"    sub \\s, r3\n"
"    exts.w r3, r3\n"
"    add r3, r0\n"
"    cmp/pz r0\n"
"    bt 35f\n"
"    neg r0, r0\n"
"35: mov.l r0, @(4, r15)\n"
"    swap.w \\r, r0\n"
"    swap.w \\s, r3\n"
"    sub r3, r0\n"
"    exts.w r0, r0\n"
"    mov.l r0, @(8, r15)\n"
"    swap.w \\q, r0\n"
"    swap.w \\p, r3\n"
"    sub r3, r0\n"
"    exts.w r0, r0\n"
"    mov.l @(8, r15), r3\n"
"    add r3, r0\n"
"    cmp/pz r0\n"
"    bt 36f\n"
"    neg r0, r0\n"
"36: mov.l @(4, r15), r3\n"
"    cmp/gt r3, r0\n"
"    bf 37f\n"
"    mov r0, r3\n"
"37: add r3, r3\n"
"    add r3, \\acc\n"
"    .endm\n"
     /* JO3DRANGE v, x: the range [r2, r3] grows with the x (1) or y (0) of the screen position of the vertex v (r0 used) */
"    .macro JO3DRANGE v, x\n"
"    mov.l @\\v, r0\n"
"    .if \\x\n"
"    swap.w r0, r0\n"
"    .endif\n"
"    exts.w r0, r0\n"
"    cmp/gt r0, r2\n"
"    bf 38f\n"
"    mov r0, r2\n"
"38: cmp/gt r3, r0\n"
"    bf 39f\n"
"    mov r0, r3\n"
"39:\n"
"    .endm\n"
     /* JO3DZRANGE v: the range [r2, r3] grows with the depth of the vertex v without its outcode (r11: the mask, r0 used) */
"    .macro JO3DZRANGE v\n"
"    mov.l @(4, \\v), r0\n"
"    and r11, r0\n"
"    cmp/gt r0, r2\n"
"    bf 38f\n"
"    mov r0, r2\n"
"38: cmp/gt r3, r0\n"
"    bf 39f\n"
"    mov r0, r3\n"
"39:\n"
"    .endm\n"
"    .text\n"
"    .align 2\n"
"    .global ___jo_3d_polygon_loop\n"
"___jo_3d_polygon_loop:\n"
"    mov.l r8, @-r15\n"
"    mov.l r9, @-r15\n"
"    mov.l r10, @-r15\n"
"    mov.l r11, @-r15\n"
"    mov.l r12, @-r15\n"
"    mov.l r13, @-r15\n"
"    mov.l r14, @-r15\n"
"    sts.l pr, @-r15\n"
"    add #-16, r15\n"                    /* 0: zmin, 4 and 8: JO3DMIDDLE, 12: draw mode bits (pre-clipping, end code) */
"    mov r4, r14\n"
"    mov r5, r8\n"
"    mov r6, r9\n"
"    mov.l @(0, r14), r10\n"
"    bra 11f\n"
"    nop\n"
     /* (the slow path from the checks of the indexes: beyond the reach of a conditional branch, 256 bytes) */
"54: bra 8f\n"
"    nop\n"
     /* --- a polygon: r8 polygon, r9 attribute. The 4 vertices (r4 to r7): indexes checked, 8 bytes each. The indexes are
        read 2 by 2 (offset 12 of a polygon: aligned on 4 bytes, the first index in the high half) */
"5:  mov.l @(12, r8), r4\n"
"    mov.l @(16, r8), r6\n"
"    mov.l @(8, r14), r0\n"
"    extu.w r4, r5\n"
"    shlr16 r4\n"
"    extu.w r6, r7\n"
"    shlr16 r6\n"
"    cmp/hs r0, r4\n"
"    bt 54b\n"
"    cmp/hs r0, r5\n"
"    bt 54b\n"
"    cmp/hs r0, r6\n"
"    bt 54b\n"
"    cmp/hs r0, r7\n"
"    bt 54b\n"
"    shll2 r4\n"
"    add r4, r4\n"
"    add r10, r4\n"
"    shll2 r5\n"
"    add r5, r5\n"
"    add r10, r5\n"
"    shll2 r6\n"
"    add r6, r6\n"
"    add r10, r6\n"
"    shll2 r7\n"
"    add r7, r7\n"
"    add r10, r7\n"
     /* depths and outcodes: all outside of the screen or behind the near plane, all too far */
"    mov.l @(4, r4), r1\n"
"    mov.l @(4, r5), r2\n"
"    mov.l @(4, r6), r3\n"
"    mov.l @(4, r7), r13\n"
"    mov r1, r0\n"
"    and r2, r0\n"
"    and r3, r0\n"
"    and r13, r0\n"
"    tst #47, r0\n"
"    bf 9f\n"
"    mov r1, r0\n"
"    cmp/gt r0, r2\n"
"    bt 1f\n"
"    mov r2, r0\n"
"1:  cmp/gt r0, r3\n"
"    bt 1f\n"
"    mov r3, r0\n"
"1:  cmp/gt r0, r13\n"
"    bt 1f\n"
"    mov r13, r0\n"
"1:  mov.l @(4, r14), r11\n"
"    cmp/gt r11, r0\n"
"    bt 9f\n"
"    mov.l r0, @r15\n"
     /* crossing the near plane or very far outside: the slow path */
"    mov r1, r0\n"
"    or r2, r0\n"
"    or r3, r0\n"
"    or r13, r0\n"
"    tst #48, r0\n"
"    bf 8f\n"
"    mov.l r0, @(12, r15)\n"             /* (the outcodes of the 4 vertices: the pre-clipping, see the label 2 below) */
     /* --- facing: cross product of the diagonals of the screen positions. Inside of the guard band (no JO_3D_CODE_GUARD),
        the differences of the coordinates fit in 16 bits: products of the low halves (muls.w), the coordinates are
        subtracted in the words without sign extension (the low half of a difference only depends on the low halves) */
"    mov.l @r4, r1\n"
"    mov.l @r5, r2\n"
"    mov.l @r6, r3\n"
"    mov.l @r7, r13\n"
"    swap.w r3, r11\n"
"    swap.w r1, r0\n"
"    sub r0, r11\n"                     /* x2 - x0 */
"    mov r13, r12\n"
"    sub r2, r12\n"                     /* y3 - y1 */
"    muls.w r11, r12\n"
"    swap.w r13, r11\n"
"    swap.w r2, r0\n"
"    sub r0, r11\n"                     /* x3 - x1 */
"    mov r3, r12\n"
"    sub r1, r12\n"                     /* y2 - y0 */
"    sts macl, r0\n"
"    muls.w r11, r12\n"
"    sts macl, r11\n"
"    sub r11, r0\n"
"    cmp/pz r0\n"
"    bf 13f\n"
     /* --- front face (r0: the area x 2). A polygon seen as a thin line (see jo_3d_emit_sliver(), the test of
        jo_3d_is_sliver(): the extent is the longest diagonal, in chebyshev length): slow path. Most polygons leave at the
        first tests (10 cycles): the first diagonal less than JO_3D_SLIVER_EXTENT wide (both diagonals of a thin line are
        about as long as it; a thin line up and down the screen is not found: rare), then the area over
        2 x JO_3D_SLIVER_WIDTH x 512 (an extent of 512 at most inside the guard band) */
"    swap.w r3, r12\n"
"    swap.w r1, r11\n"
"    sub r11, r12\n"                    /* x2 - x0 */
"    exts.w r12, r12\n"
"    add #64, r12\n"
"    mov #64, r11\n"
"    add r11, r11\n"
"    cmp/hi r11, r12\n"                 /* |x2 - x0| > 64 */
"    bf 3f\n"
"    mov #16, r11\n"
"    shll8 r11\n"                       /* 4096 */
"    cmp/ge r11, r0\n"
"    bt 3f\n"
"    mov r3, r11\n"
"    sub r1, r11\n"                     /* y2 - y0 */
"    muls.w r11, r11\n"
"    swap.w r3, r12\n"
"    swap.w r1, r11\n"
"    sub r11, r12\n"                    /* x2 - x0 */
"    sts macl, r11\n"
"    muls.w r12, r12\n"
"    sts macl, r12\n"
"    cmp/gt r11, r12\n"
"    bf 1f\n"
"    mov r12, r11\n"
"1:  mov #16, r12\n"
"    shll8 r12\n"                       /* 4096: JO_3D_SLIVER_EXTENT squared */
"    cmp/ge r12, r11\n"
"    bf 3f\n"
"    mov r13, r12\n"
"    sub r2, r12\n"                     /* y3 - y1 */
"    muls.w r12, r12\n"
"    sts macl, r12\n"
"    cmp/gt r11, r12\n"
"    bf 1f\n"
"    mov r12, r11\n"
"1:  swap.w r13, r12\n"
"    swap.w r2, r3\n"
"    sub r3, r12\n"                     /* x3 - x1 */
"    muls.w r12, r12\n"
"    sts macl, r12\n"
"    cmp/gt r11, r12\n"
"    bf 1f\n"
"    mov r12, r11\n"
     /* r11: the extent squared. A thin line: (area x 2)² < (2 x JO_3D_SLIVER_WIDTH)² x extent² = 64 x extent² */
"1:  muls.w r0, r0\n"
"    mov r11, r12\n"
"    shll2 r12\n"
"    shll2 r12\n"
"    shll2 r12\n"
"    sts macl, r0\n"
"    cmp/gt r0, r12\n"
"    bt 8f\n"
"    bra 3f\n"
"    nop\n"
"13: mov r0, r12\n"                     /* (the cross product of the diagonals: the area of the quad x 2) */
"    mov.b @r9, r0\n"                   /* back face: a single plane polygon is skipped, a dual plane one is mirrored */
"    tst r0, r0\n"
"    bf 12f\n"
     /* (a single plane quad is skipped when its 2 triangles face away: a quad of the ground is not flat, seen at a grazing
        angle one of its triangles can face the camera, the quad was skipped and the sea was seen through the ground.
        The area of the quad is the sum of the areas of the triangles 0 1 2 and 0 2 3) */
"    swap.w r2, r11\n"
"    swap.w r1, r0\n"
"    sub r0, r11\n"                     /* x1 - x0 */
"    mov r3, r13\n"
"    sub r1, r13\n"                     /* y2 - y0 */
"    muls.w r11, r13\n"
"    mov r2, r11\n"
"    sub r1, r11\n"                     /* y1 - y0 */
"    swap.w r3, r13\n"
"    sub r0, r13\n"                     /* x2 - x0 */
"    sts macl, r0\n"
"    muls.w r11, r13\n"
"    sts macl, r11\n"
"    sub r11, r0\n"                     /* area of the triangle 0 1 2 x 2 */
"    cmp/pz r0\n"
"    bt 3f\n"
"    cmp/ge r0, r12\n"                  /* the triangle 0 2 3 faces the camera: area of the quad >= area of 0 1 2 */
"    bt 3f\n"
     /* --- next polygon (the branches of the loop meet here: in reach of the short branches) */
"9:  add #20, r8\n"
"    add #12, r9\n"
"11: mov.l @(44, r14), r0\n"
"    cmp/hs r0, r8\n"
     /* (the next polygon: the start of the loop is beyond the reach of a conditional branch; r0 is read again there) */
"    bt/s 10f\n"
"    mov #1, r0\n"
"    bra 5b\n"
"    nop\n"
     /* --- back face of a dual plane polygon: drawn mirrored, the vertices 0 and 1, 2 and 3 swapped (as jo_3d_emit_polygon()
        does: the same command). Half transparent: slow path (a big one mostly out of the screen is drawn in parts, see
        jo_3d_subdivision_level()). The dual plane back faces were the first cause of the slow path */
"12: mov.w @(4, r9), r0\n"
"    and #3, r0\n"
"    cmp/eq #3, r0\n"
"    bt 8f\n"
"    mov r4, r0\n"
"    mov r5, r4\n"
"    mov r0, r5\n"
"    mov r6, r0\n"
"    mov r7, r6\n"
"    bra 3f\n"
"    mov r0, r7\n"
     /* --- slow path (the polygon is not done yet: r8, r9 are still on it) */
"53:\n"
"8:  mov r14, r4\n"
"    mov r8, r5\n"
"    mov r9, r6\n"
"    mov.l 26f, r0\n"
"    jsr @r0\n"
"    nop\n"
"    tst r0, r0\n"
"    bf 9b\n"
"    mov #0, r0\n"
"10: add #16, r15\n"
"    lds.l @r15+, pr\n"
"    mov.l @r15+, r14\n"
"    mov.l @r15+, r13\n"
"    mov.l @r15+, r12\n"
"    mov.l @r15+, r11\n"
"    mov.l @r15+, r10\n"
"    mov.l @r15+, r9\n"
"    rts\n"
"    mov.l @r15+, r8\n"
     /* --- the sort flags stay in r12 (read once). A lit polygon (JO_3D_USE_LIGHT) is lit after its command (label 42) */
"3:  mov.b @(1, r9), r0\n"
"    mov r0, r12\n"
     /* a textured polygon with tiles is drawn in parts when its depths are spread (see jo_3d_subdivision_level()): slow path */
"    tst #4, r0\n"
"    bt 2f\n"
"    mov.w @(2, r9), r0\n"
"    extu.w r0, r1\n"
"    mov.l 27f, r3\n"                   /* the flags of the texture: tiles (JO_3D_TEXTURE_HAS_TILES) */
"    mov r1, r0\n"
"    mov.b @(r0, r3), r0\n"
"    tst #1, r0\n"
"    bt 2f\n"
"    mov r1, r0\n"                      /* (sprite x 12: the size of jo_3d_texture_tiles) */
"    add r0, r0\n"
"    add r1, r0\n"
"    add r0, r0\n"
"    add r0, r0\n"
"    mov.l 20f, r1\n"
"    add r0, r1\n"                      /* r1: the tiles of the texture (jo_3d_texture_tiles) */
     /* (the polygons farther than the depth of the tiles are not drawn in parts, see jo_3d_set_texture_subdivision_depth():
      * fast path (the label 2: the sort key). 0: all the polygons. They went to the slow path (the label 8) before:
      * about 260 polygons per frame of a big scene emitted by the C code, about 3 ms on each CPU) */
"    mov.l @(8, r1), r0\n"
"    tst r0, r0\n"
"    bt 1f\n"
"    mov.l @r15, r1\n"                  /* minimum depth */
"    cmp/gt r0, r1\n"
"    bt 2f\n"
     /* (maximum depth (r1), only when it is needed: here and for JO_3D_SORT_MAX) */
"1:  mov.l @(4, r4), r1\n"
"    mov.l @(4, r5), r0\n"
"    cmp/gt r1, r0\n"
"    bf 1f\n"
"    mov r0, r1\n"
"1:  mov.l @(4, r6), r0\n"
"    cmp/gt r1, r0\n"
"    bf 1f\n"
"    mov r0, r1\n"
"1:  mov.l @(4, r7), r0\n"
"    cmp/gt r1, r0\n"
"    bf 1f\n"
"    mov r0, r1\n"
"1:  mov.l @r15, r0\n"
"    sub r0, r1\n"
"    shlr2 r0\n"                        /* (the depths are positive: no JO_3D_CODE_NEAR) */
"    shlr r0\n"
"    cmp/gt r1, r0\n"
"    bt 2f\n"
"    bra 50f\n"                         /* (spread: drawn in parts or not, see the label 50) */
"    nop\n"
     /* --- sort key: the depths (JO_3D_SORT_PREVIOUS: slow path) */
"52:\n"
"2:  mov r12, r0\n"
"    and #3, r0\n"
"    cmp/eq #0, r0\n"
"    bt 8b\n"
"    cmp/eq #1, r0\n"
"    bf 1f\n"
"    bra 4f\n"
"    mov.l @r15, r1\n"                  /* minimum */
"1:  cmp/eq #2, r0\n"
"    bf 1f\n"
"    mov.l @(4, r4), r1\n"              /* maximum */
"    mov.l @(4, r5), r0\n"
"    cmp/gt r1, r0\n"
"    bf 2f\n"
"    mov r0, r1\n"
"2:  mov.l @(4, r6), r0\n"
"    cmp/gt r1, r0\n"
"    bf 2f\n"
"    mov r0, r1\n"
"2:  mov.l @(4, r7), r0\n"
"    cmp/gt r1, r0\n"
"    bf 4f\n"
"    bra 4f\n"
"    mov r0, r1\n"
"1:  mov.l @(4, r4), r1\n"              /* average (the depths are positive) */
"    shlr2 r1\n"
"    mov.l @(4, r5), r0\n"
"    shlr2 r0\n"
"    add r0, r1\n"
"    mov.l @(4, r6), r0\n"
"    shlr2 r0\n"
"    add r0, r1\n"
"    mov.l @(4, r7), r0\n"
"    shlr2 r0\n"
"    add r0, r1\n"
"4:  mov #-128, r0\n"
"    and r0, r1\n"
"    mov.l @(40, r14), r0\n"
"    mov.l r1, @r0\n"
     /* an untextured polygon that is not a polygon (lines): slow path */
"    mov r12, r0\n"
"    tst #4, r0\n"
"    bf 1f\n"
"    mov.w @(10, r9), r0\n"
"    and #15, r0\n"
"    cmp/eq #4, r0\n"
"    bf 8b\n"
"1:  swap.w r1, r0\n"
"    exts.w r0, r0\n"
"    mov.l @(28, r14), r1\n"
"    add r1, r0\n"
"    cmp/pz r0\n"
"    bt 1f\n"
"    mov #0, r0\n"
"1:  mov.l 21f, r1\n"
"    cmp/gt r1, r0\n"
"    bf 1f\n"
"    mov r1, r0\n"
     /* --- the command: the table is full (slow path, it reports the error) */
"1:  mov.l @(20, r14), r2\n"
"    mov.l @r2, r3\n"
"    mov.l @(24, r14), r11\n"
"    cmp/hs r11, r3\n"
"    bt 8b\n"
"    mov.l @(16, r14), r11\n"
"    mov r3, r13\n"
"    add r13, r13\n"
"    add r13, r11\n"
"    mov.w r0, @r11\n"                  /* keys[used] */
"    mov.l @(12, r14), r13\n"
"    mov r3, r11\n"
"    shll2 r11\n"
"    shll2 r11\n"
"    shll r11\n"
"    add r11, r13\n"                    /* the command */
"    add #1, r3\n"
"    mov.l r3, @r2\n"
"    mov.l @(36, r14), r2\n"
"    mov.w @r2, r0\n"
"    add #1, r0\n"
"    mov.w r0, @r2\n"
"    mov.l @r4, r0\n"
"    mov.l r0, @(12, r13)\n"
"    mov.l @r5, r0\n"
"    mov.l r0, @(16, r13)\n"
"    mov.l @r6, r0\n"
"    mov.l r0, @(20, r13)\n"
"    mov.l @r7, r0\n"
"    mov.l r0, @(24, r13)\n"
"    mov.w @(4, r9), r0\n"              /* gouraud table with the gouraud shading (a tint: the offset to the tinted copy) */
"    tst #4, r0\n"
"    bt 1f\n"
"    mov.w @(8, r9), r0\n"
"    tst r0, r0\n"
"    bt 2f\n"
"    mov.l @(52, r14), r1\n"
"    add r1, r0\n"
"    bra 2f\n"
"    shll16 r0\n"
"1:  mov #0, r0\n"
"2:  mov.l r0, @(28, r13)\n"
     /* (the draw mode bits of the polygon in the word 12 of the stack: the pre-clipping disabled when its 4 vertices are
        inside of the screen (no JO_3D_CODE_SCREEN in their outcodes, see jo_3d_pre_clipping()), the end code flag added
        to them below) */
"    mov.l @(12, r15), r0\n"
"    mov.l @(60, r14), r1\n"
"    tst #15, r0\n"
"    movt r0\n"
"    neg r0, r0\n"
"    and r1, r0\n"
"    mov.l r0, @(12, r15)\n"
"    mov r12, r0\n"
"    tst #4, r0\n"
"    bf 33f\n"                        /* (7: out of reach of a conditional branch) */
"    bra 7f\n"
"    nop\n"
     /* texture: distorted sprite with the flips, the end codes disabled by a horizontal flip, the smaller copy for the size */
"33: mov.w @(10, r9), r0\n"
"    extu.w r0, r12\n"
"    and #48, r0\n"
"    or #2, r0\n"
"    shll16 r0\n"
"    mov.l r0, @r13\n"
"    mov.w @(2, r9), r0\n"
"    extu.w r0, r1\n"
"    mov r12, r0\n"
"    tst #16, r0\n"
"    bt 1f\n"
"    mov.l 22f, r2\n"
"    mov r1, r0\n"
"    mov.b @(r0, r2), r0\n"
"    tst r0, r0\n"
"    bt 1f\n"
"    mov.l @(12, r15), r0\n"
"    or #128, r0\n"
"    mov.l r0, @(12, r15)\n"
"1:  mov.l 27f, r2\n"                   /* the flags of the texture: smaller copies (JO_3D_TEXTURE_HAS_MIPMAPS) */
"    mov r1, r0\n"
"    mov.b @(r0, r2), r0\n"
"    tst #2, r0\n"
"    bf 32f\n"
"    bra 2f\n"
"    nop\n"
     /* the size on the screen (as __jo_3d_mipmap_select()): r11 the longest of the edges AB and DC, r12 of AD and BC (the
        longest of |dx| and |dy|). The screen positions (x in the high half, y in the low half) are inside of the guard
        band: the differences fit in 16 bits. The vertices (r4 to r7) are not used after */
"32: mov.l @r4, r4\n"
"    mov.l @r5, r5\n"
"    mov.l @r6, r6\n"
"    mov.l @r7, r7\n"
"    mov #0, r11\n"
"    mov #0, r12\n"
"    JO3DMIPEDGE r4, r5, r11\n"
"    JO3DMIPEDGE r7, r6, r11\n"
"    JO3DMIPEDGE r4, r7, r12\n"
"    JO3DMIPEDGE r5, r6, r12\n"
     /* __jo_3d_mipmaps[sprite]: 20 bytes per texture (levels 0, first 2, width 4, height 8, area 12) */
"    mov.l 23f, r2\n"
"    mov r1, r0\n"
"    shll2 r0\n"
"    mov r0, r3\n"
"    shll2 r0\n"
"    add r3, r0\n"
"    add r0, r2\n"
"    mul.l r11, r12\n"
"    sts macl, r3\n"                   /* the area */
"    mov.l @(12, r2), r0\n"            /* bigger than the half size: the texture */
"    cmp/gt r0, r3\n"
"    bt 2f\n"
"    mov.w @(4, r2), r0\n"
"    extu.w r0, r0\n"
"    cmp/gt r0, r11\n"
"    bt 2f\n"
"    mov.w @(8, r2), r0\n"
"    extu.w r0, r0\n"
"    cmp/gt r0, r12\n"
"    bt 2f\n"
"    mov.w @(2, r2), r0\n"             /* the half size */
"    extu.w r0, r1\n"
"    mov.b @r2, r0\n"
"    cmp/eq #1, r0\n"
"    bt 2f\n"
"    mov.l @(16, r2), r0\n"
"    cmp/gt r0, r3\n"
"    bt 2f\n"
"    mov.w @(6, r2), r0\n"
"    extu.w r0, r0\n"
"    cmp/gt r0, r11\n"
"    bt 2f\n"
"    mov.w @(10, r2), r0\n"
"    extu.w r0, r0\n"
"    cmp/gt r0, r12\n"
"    bt 2f\n"
"    add #1, r1\n"                     /* the quarter size */
"2:  mov.l 25f, r2\n"                   /* texture address and size */
"    mov r1, r3\n"
"    shll2 r3\n"
"    add r3, r3\n"
"    add r2, r3\n"
"    mov.w @(4, r3), r0\n"
"    shll16 r0\n"
"    mov r0, r2\n"
"    mov.w @(6, r3), r0\n"
"    extu.w r0, r0\n"
"    or r0, r2\n"
"    bra 1f\n"
"    mov.l r2, @(8, r13)\n"
     /* polygon: no texture. Without a gouraud table (the table of the polygon: the colors in the order of its vertices), its
        vertices are rotated by one when the VDP1 draws it with fewer lines, as jo_3d_polygon_rotation_is_faster(): the lines
        max(DA, BC) + 1 (r12) and max(AB, CD) + 1 of the rotated polygon (r11), the costs x 6 (78 + AB + CD + 2 x |B + C - A - D|)
        x r12 (r3) and (78 + BC + DA + 2 x |C + D - A - B|) x r11 (r0) */
"7:  mov.w @(4, r9), r0\n"
"    tst #4, r0\n"
"    bf 34f\n"
"    bra 40f\n"                       /* (out of line: the literals above stay in reach) */
"    nop\n"
"34: mov #4, r0\n"
"    shll16 r0\n"
"    mov.l r0, @r13\n"
"    mov #0, r0\n"
"    mov.l r0, @(8, r13)\n"
     /* draw mode with the clipping of the window and the end code flag, color */
"1:  mov.w @(4, r9), r0\n"
"    extu.w r0, r11\n"
"    mov.l @(32, r14), r0\n"
"    or r0, r11\n"
"    mov.l @(12, r15), r0\n"
"    or r0, r11\n"
"    shll16 r11\n"
"    mov.w @(6, r9), r0\n"
"    extu.w r0, r0\n"
"    or r0, r11\n"
"    mov.l r11, @(4, r13)\n"
"    mov.b @(1, r9), r0\n"
"    tst #8, r0\n"
"    bf 42f\n"
"    bra 9b\n"
"    nop\n"
     /* lit polygon: __jo_3d_light_command(k, command, polygon, attribute) changes its color or its gouraud table */
"42: mov r14, r4\n"
"    mov r13, r5\n"
"    mov r8, r6\n"
"    mov r9, r7\n"
"    mov.l 28f, r0\n"
"    jsr @r0\n"
"    nop\n"
"    bra 9b\n"
"    nop\n"
"    .align 2\n"
"20: .long ___jo_3d_tiles\n"
"21: .long 0xFFFF\n"
"22: .long ___jo_sprite_end_codes\n"
"23: .long ___jo_3d_mipmaps\n"
"27: .long ___jo_3d_texture_flags\n"
"25: .long ___jo_sprite_def\n"
"26: .long ___jo_3d_draw_polygon_slow\n"
"28: .long ___jo_3d_light_command\n"
     /* --- an untextured polygon: its vertices rotated by one when the VDP1 draws it faster (see the label 7) */
"40: mov.l @r4, r4\n"
"    mov.l @r5, r5\n"
"    mov.l @r6, r6\n"
"    mov.l @r7, r7\n"
"    mov #0, r1\n"
"    JO3DMIPEDGE r4, r5, r1\n"           /* AB */
"    mov #0, r11\n"
"    JO3DMIPEDGE r6, r7, r11\n"          /* CD */
"    mov #0, r2\n"
"    JO3DMIPEDGE r5, r6, r2\n"           /* BC */
"    mov #0, r12\n"
"    JO3DMIPEDGE r7, r4, r12\n"          /* DA */
"    mov r1, r0\n"
"    cmp/gt r0, r11\n"
"    bf 1f\n"
"    mov r11, r0\n"
"1:  add r11, r1\n"                     /* AB + CD */
"    add #1, r0\n"
"    mov r0, r11\n"                     /* lines of the rotated polygon */
"    mov r12, r0\n"
"    cmp/gt r0, r2\n"
"    bf 1f\n"
"    mov r2, r0\n"
"1:  add r12, r2\n"                     /* BC + DA */
"    add #1, r0\n"
"    mov r0, r12\n"                     /* lines of the polygon */
"    JO3DMIDDLE r4, r5, r6, r7, r1\n"    /* + 2 x |(B - A) + (C - D)| */
"    JO3DMIDDLE r5, r6, r7, r4, r2\n"    /* + 2 x |(C - B) + (D - A)| */
"    add #78, r1\n"
"    mul.l r12, r1\n"
"    sts macl, r3\n"
"    add #78, r2\n"
"    mul.l r11, r2\n"
"    sts macl, r0\n"
"    cmp/hs r3, r0\n"
"    bt 41f\n"
"    mov.l r5, @(12, r13)\n"
"    mov.l r6, @(16, r13)\n"
"    mov.l r7, @(20, r13)\n"
"    mov.l r4, @(24, r13)\n"
"41: bra 34b\n"
"    nop\n"
     /* --- a textured polygon with tiles whose depths are spread: not drawn in parts in these cases, as
        jo_3d_subdivision_level() (else the slow path): no vertex out of the screen (JO_3D_CODE_SCREEN, JO_3D_CODE_VIEW), the
        window without clipping (jo_3d_draw_polygon_general() skips the polygons out of its rectangle), and the tiles only for
        the near plane (JO_3D_TILES_NEAR_ONLY), or the size on the screen under 4 x the distortion allowed, or a distortion
        surely under it: size x (zmax - zmin) < 4 x (distortion + 1) x (zmax + zmin - 2) (the distortion computed by
        jo_3d_subdivision_level_unbudgeted() is at most size x (zmax - zmin) / (zmax + zmin - 2) / 4, the halves of the depths
        rounded down). About 1 polygon out of 3 sent to the slow path in a level, then drawn as one polygon */
"50: mov.l @(32, r14), r0\n"
"    tst r0, r0\n"
"    bt 56f\n"
     /* a clipped window (a room seen through a door): out of its rectangle, not drawn (see jo_3d_is_outside_window());
      * inside, tested like the polygons of the other windows (they all went to the slow path, which drew most of them as
      * one polygon) */
"    mov.l @r4, r0\n"
"    swap.w r0, r0\n"
"    exts.w r0, r2\n"
"    mov r2, r3\n"
"    JO3DRANGE r5, 1\n"
"    JO3DRANGE r6, 1\n"
"    JO3DRANGE r7, 1\n"                /* r2: xmin, r3: xmax */
"    mov r14, r1\n"
"    add #64, r1\n"
"    mov.w @r1+, r0\n"                 /* the left of the window */
"    cmp/gt r3, r0\n"
"    bt 60f\n"
"    mov.w @r1+, r0\n"                 /* its right */
"    cmp/gt r0, r2\n"
"    bt 60f\n"
"    mov.l @r4, r0\n"
"    exts.w r0, r2\n"
"    mov r2, r3\n"
"    JO3DRANGE r5, 0\n"
"    JO3DRANGE r6, 0\n"
"    JO3DRANGE r7, 0\n"                /* r2: ymin, r3: ymax */
"    mov.w @r1+, r0\n"                 /* its top */
"    cmp/gt r3, r0\n"
"    bt 60f\n"
"    mov.w @r1+, r0\n"                 /* its bottom */
"    cmp/gt r0, r2\n"
"    bf 56f\n"
"60: bra 9b\n"                         /* (out of the window: the next polygon) */
"    nop\n"
"56:\n"
"    mov.l @(4, r4), r0\n"
"    mov.l @(4, r5), r1\n"
"    or r1, r0\n"
"    mov.l @(4, r6), r1\n"
"    or r1, r0\n"
"    mov.l @(4, r7), r1\n"
"    or r1, r0\n"
"    tst #79, r0\n"
"    bt 57f\n"
     /* (a vertex out of the screen (in the guard band: projected): the slow path only for a half transparent polygon or
      * the tiles of a model cut when it was made (drawn in parts when mostly out of the screen, see
      * jo_3d_subdivision_level_unbudgeted()); the others are tested here like the ones on the screen (they all went to
      * the slow path, the C code found most of them drawn as one polygon) */
"    mov.w @(4, r9), r0\n"               /* draw mode: JO_VDP1_COLOR_CALC_HALF_TRANSPARENT (3) */
"    and #3, r0\n"
"    cmp/eq #3, r0\n"
"    bf 57f\n"
"    bra 54f\n"
"    nop\n"
"57:\n"
     /* (the budget of the subdivisions of the frame spent: drawn as one, the fast path, see jo_3d_subdivision_level()) */
"    mov.l @(56, r14), r0\n"
"    tst r0, r0\n"
"    bt 60f\n"                           /* (55 is out of reach of a conditional branch) */
"    bra 55f\n"
"    nop\n"
"60:\n"
"    mov.w @(2, r9), r0\n"               /* the tiles of the texture (sprite x 12) */
"    extu.w r0, r1\n"
"    mov r1, r0\n"
"    add r0, r0\n"
"    add r1, r0\n"
"    add r0, r0\n"
"    add r0, r0\n"
"    mov.l 51f, r1\n"
"    add r0, r1\n"
"    mov.b @r1, r0\n"
"    tst #128, r0\n"
"    bt 58f\n"
     /* (tiles of a model cut when it was made: never drawn in parts on the screen, in parts when mostly out of it: a vertex
      * out of the screen and its box on the screen of 2048 pixels or more (see jo_3d_is_mostly_out_of_screen(); smaller,
      * the C code drew it as one polygon: the bars of a railing along the view, partly out of the screen)) */
"    mov.l @(4, r4), r0\n"
"    mov.l @(4, r5), r2\n"
"    or r2, r0\n"
"    mov.l @(4, r6), r2\n"
"    or r2, r0\n"
"    mov.l @(4, r7), r2\n"
"    or r2, r0\n"
"    tst #79, r0\n"
"    bf 61f\n"
"    bra 55f\n"
"    nop\n"
"61:\n"
"    mov.l @r4, r0\n"
"    swap.w r0, r0\n"
"    exts.w r0, r2\n"
"    mov r2, r3\n"
"    JO3DRANGE r5, 1\n"
"    JO3DRANGE r6, 1\n"
"    JO3DRANGE r7, 1\n"
"    sub r2, r3\n"
"    mov r3, r1\n"                      /* the width of the box */
"    mov.l @r4, r0\n"
"    exts.w r0, r2\n"
"    mov r2, r3\n"
"    JO3DRANGE r5, 0\n"
"    JO3DRANGE r6, 0\n"
"    JO3DRANGE r7, 0\n"
"    sub r2, r3\n"                      /* its height */
"    mulu.w r1, r3\n"
"    mov #8, r1\n"
"    shll8 r1\n"                        /* 2048 */
"    sts macl, r0\n"
"    cmp/hs r1, r0\n"
"    bt 54f\n"
"    bra 55f\n"
"    nop\n"
"58:\n"
"    mov.b @(1, r1), r0\n"               /* the distortion allowed (0: JO_3D_SUBDIVISION_ERROR) */
"    extu.b r0, r0\n"
"    tst r0, r0\n"
"    bf 1f\n"
"    mov #8, r0\n"
"1:  add #1, r0\n"
"    shll2 r0\n"
"    mov r0, r13\n"                      /* 4 x (distortion + 1) */
"    add #-4, r0\n"
"    mov r0, r11\n"                      /* 4 x distortion */
     /* the size on the screen: the longest of the ranges of x and y (r1) */
"    mov.l @r4, r0\n"
"    swap.w r0, r0\n"
"    exts.w r0, r2\n"
"    mov r2, r3\n"
"    JO3DRANGE r5, 1\n"
"    JO3DRANGE r6, 1\n"
"    JO3DRANGE r7, 1\n"
"    sub r2, r3\n"
"    mov r3, r1\n"
"    mov.l @r4, r0\n"
"    exts.w r0, r2\n"
"    mov r2, r3\n"
"    JO3DRANGE r5, 0\n"
"    JO3DRANGE r6, 0\n"
"    JO3DRANGE r7, 0\n"
"    sub r2, r3\n"
"    cmp/gt r1, r3\n"
"    bf 1f\n"
"    mov r3, r1\n"
"1:  cmp/gt r1, r11\n"
"    bt 55f\n"
     /* the depths without their outcodes: zmin (r2), zmax (r3) */
"    mov #-128, r11\n"
"    mov.l @(4, r4), r2\n"
"    and r11, r2\n"
"    mov r2, r3\n"
"    JO3DZRANGE r5\n"
"    JO3DZRANGE r6\n"
"    JO3DZRANGE r7\n"
"    mov r3, r0\n"
"    sub r2, r0\n"
"    dmulu.l r1, r0\n"                   /* size x (zmax - zmin): r11, r0 */
"    add r2, r3\n"
"    add #-2, r3\n"
"    sts mach, r11\n"
"    sts macl, r0\n"
"    dmulu.l r13, r3\n"                  /* 4 x (distortion + 1) x (zmax + zmin - 2): r2, r3 */
"    sts mach, r2\n"
"    sts macl, r3\n"
"    cmp/hs r2, r11\n"
"    bf 55f\n"
"    cmp/eq r2, r11\n"
"    bf 54f\n"
"    cmp/hs r3, r0\n"
"    bf 55f\n"
"54: bra 53b\n"
"    nop\n"
"55: bra 52b\n"
"    nop\n"
"    .align 2\n"
"51: .long ___jo_3d_tiles\n"
);

/** @brief Draw the polygons of an object with the state of a CPU
 *  @param ctx State of the CPU (its matrix is used)
 *  @param commands Reserved commands
 *  @param keys Depth keys of the reserved commands
 *  @param reserved Reserved command count
 *  @param used Commands used (input and output)
 *  @return false if the command table is full
 */
/** @brief Transform the points of an object and emit its polygons
 *  @param ctx CPU context
 *  @param pdata Polygon data
 *  @param point_count Number of points to transform (the first ones of pdata)
 *  @param ranges Ranges of polygons to draw (JO_NULL: all the polygons)
 *  @param range_count Number of ranges
 *  @param window Window
 *  @param commands Commands
 *  @param keys Sort keys of the commands
 *  @param reserved Number of commands available
 *  @param used Number of commands used (updated)
 *  @return false if the command table is full
 */

static bool                                 jo_3d_draw_polygons(jo_3d_cpu_context * const ctx, const jo_3d_polygon_data * const pdata,
                                                                const unsigned int first_point, const unsigned int point_count,
                                                                const jo_3d_polygon_range * const ranges, const int range_count,
                                                                const int window, jo_vdp1_command * const commands,
                                                                unsigned short * const keys, const unsigned int reserved,
                                                                unsigned int * const used)
{
    register unsigned int                   i;
    unsigned int                            count;
    unsigned int                            polygon_count;
    jo_3d_polygon_range                     all_polygons;
    const jo_3d_polygon_range               *range;
    const jo_3d_polygon_range               *last_range;
    jo_vdp1_window                          *w;
    jo_fixed                                near_z;
    jo_fixed                                zlimit;
    int                                     right;
    int                                     bottom;
    const jo_3d_polygon                     *p;
    const jo_3d_polygon_attribute           *a;
    jo_3d_light_context                     light;
    jo_3d_clip_context                      clip;
    jo_3d_draw_state                        state;
    jo_3d_polygon_loop                      loop;
    bool                                    fast;

    count = point_count;
    if (count > ctx->max_vertices)
    {
#ifdef JO_DEBUG
        if (!ctx->slave)
            jo_core_error("Too many vertices (%d), max=%d", count, ctx->max_vertices);
#endif
        count = ctx->max_vertices;
    }
    w = &__jo_vdp1_windows[window];
    ctx->clip_mode = w->clip_mode;
    near_z = __jo_vdp1_near_clip();
    zlimit = w->zlimit;
    // (a clipped window of windows apart is a screen of its own: its rectangle, see jo_3d_window_clip())
    right = w->screen ? w->right - w->left : __jo_vdp1_system_clip_right;
    bottom = w->screen ? w->bottom - w->top : __jo_vdp1_system_clip_bottom;
    if (ranges == JO_NULL)
    {
        all_polygons.first = 0;
        all_polygons.count = (unsigned short)pdata->polygon_count;
        range = &all_polygons;
        last_range = range + 1;
    }
    else
    {
        range = ranges;
        last_range = ranges + range_count;
    }
    JO_ZERO(polygon_count);
    for (i = 0; range + i < last_range; ++i)
        polygon_count += range[i].count;
    *ctx->vertice_count += count;
    *ctx->polygon_count += polygon_count;
    // (the exact view space positions of the vertices of the polygons clipped or drawn in parts, see jo_3d_vertex_view())
    ctx->points = pdata->points;
    ctx->point_count = count;
    light.ready = false;
    light.matrix = ctx->matrix;
    light.direction = ctx->light_direction;
    clip.ready = false;
    // Computed when a polygon must be clipped (initialized for the compiler: -Wmaybe-uninitialized)
    clip.inverse_pd = 0;
    clip.planes[0] = 0;
    clip.planes[1] = 0;
    clip.planes[2] = 0;
    clip.planes[3] = 0;
    jo_profile_begin(JO_PROFILE_3D_VERTICES);
    if (ctx->parts == JO_NULL && (first_point > 0 || (ranges != JO_NULL && count > 0 && JO_MULT_BY_8(polygon_count) <= pdata->polygon_count * 7 &&
        point_count >= pdata->point_count)))
    {
        // A few polygons of the mesh (half of them at most: a part of a cell, the share of a CPU): only the vertices from the
        // first one they use to the last one (the vertices of a model are in the order of its polygons: the 2 CPUs drew
        // parts of the same rooms, each one transformed all their vertices). (More polygons: the search costs more than
        // the vertices it saves, the chunks of a terrain. Fewer points given by the caller: it knows the points
        // of its ranges, not searched: the coarse meshes of a ground, the search saved no vertex and read
        // all their polygons in the slow RAM)
        unsigned int                        lo;
        unsigned int                        hi;
        unsigned int                        v;
        const jo_3d_polygon                 *q;
        const jo_3d_polygon                 *q_end;
        jo_3d_vertex                        *base;
        jo_3d_view_position                 *view;

        lo = count;
        JO_ZERO(hi);
        // (the first point given by the caller: the points of its ranges known (the chunks of the cells, see
        // __jo_3d_draw_ranges_from()), not searched: the search read the 4 vertex indexes of each polygon, out of the cache,
        // 2 % of the time of each CPU in the first person shooter demo)
        if (first_point > 0)
        {
            lo = first_point;
            hi = count - 1;
        }
        else
        for (JO_ZERO(i); range + i < last_range; ++i)
            for (q = pdata->polygons + range[i].first, q_end = q + range[i].count; q < q_end; ++q)
            {
                v = q->vertices[0];
                lo = JO_MIN(lo, v);
                hi = JO_MAX(hi, v);
                v = q->vertices[1];
                lo = JO_MIN(lo, v);
                hi = JO_MAX(hi, v);
                v = q->vertices[2];
                lo = JO_MIN(lo, v);
                hi = JO_MAX(hi, v);
                v = q->vertices[3];
                lo = JO_MIN(lo, v);
                hi = JO_MAX(hi, v);
            }
        hi = JO_MIN(hi, count - 1);
#ifdef JO_TEST_SPAN
        if (!ctx->slave)
        {
            __jo_3d_test_span[0] += lo <= hi ? hi - lo + 1 : 0;
            __jo_3d_test_span[1] += polygon_count;
            __jo_3d_test_span[2] += count;
        }
#endif
        if (lo <= hi)
        {
            base = ctx->vertices;
            view = ctx->view;
            ctx->vertices = base + lo;
            ctx->view = view + lo;
            jo_3d_transform_vertices(ctx, pdata->points + lo, hi - lo + 1, near_z, JO_3D_WINDOW_CX(w), JO_3D_WINDOW_CY(w), right, bottom);
            ctx->vertices = base;
            ctx->view = view;
        }
    }
    else if (ctx->parts == JO_NULL)
    {
#ifdef JO_TEST_SPAN
        if (!ctx->slave)
        {
            __jo_3d_test_span[0] += count;
            __jo_3d_test_span[1] += polygon_count;
            __jo_3d_test_span[2] += count;
        }
#endif
        jo_3d_transform_vertices(ctx, pdata->points, count, near_z, JO_3D_WINDOW_CX(w), JO_3D_WINDOW_CY(w), right, bottom);
    }
    else
        // Each part with its matrix (the bones of a skeleton): its vertices at their place in the buffer, their view space
        // positions too (the parallel array: moved with the vertices, the positions of the vertices of a part behind the
        // near plane or far outside of the screen went at the start of the array, read at their index by the clipping)
        jo_3d_transform_parts(ctx, pdata->points, count, near_z, JO_3D_WINDOW_CX(w), JO_3D_WINDOW_CY(w), right, bottom);
    jo_profile_end(JO_PROFILE_3D_VERTICES);
    jo_profile_count(JO_PROFILE_3D_VERTEX_COUNT, count);
    jo_profile_count(JO_PROFILE_3D_POLYGON_COUNT, polygon_count);
    jo_profile_begin(JO_PROFILE_3D_POLYGONS);
    // Polygons (a clipped polygon can use several commands): the common ones by the assembly loop, the others by
    // __jo_3d_draw_polygon_slow()
    state.ctx = ctx;
    state.near_z = near_z;
    state.w = w;
    state.right = right;
    state.bottom = bottom;
    state.light = &light;
    state.clip = &clip;
    state.polygons = pdata->polygons;
    state.env_ready = false;
    loop.vertices = ctx->vertices;
    loop.zlimit = zlimit;
    loop.count = count;
    loop.commands = commands;
    loop.keys = keys;
    loop.used = used;
    loop.reserved = reserved;
    loop.bias = *ctx->depth_bias;
    loop.clip_mode = ctx->clip_mode;
    loop.displayed = ctx->displayed_polygon_count;
    loop.previous_z = &ctx->previous_polygon_z;
    loop.state = &state;
    // (a tint: the tinted copy of the gouraud tables of the object, see jo_3d_set_tint(); both loops add the offset. The
    // tinted objects went through the C loop: the characters of a lit room, 19 microseconds per polygon instead of 5)
    ctx->gouraud_offset = ctx->slave ? jo_3d_slave_tint_offset(pdata) : jo_3d_tint_offset(pdata);
    loop.gouraud_offset = ctx->gouraud_offset;
    // (the budget of the subdivisions spent (a room with many polygons near the camera, characters): the polygons that may
    // need parts for the perspective stay in the assembly loop, drawn as one (the C code of the slow path found that for
    // each one of them, see jo_3d_subdivision_level()))
    loop.no_subdivision = jo_3d_frame_command_total(ctx, *used) > JO_3D_SUBDIVISION_BUDGET ? 1 : 0;
    // (a clipped window (a room seen through a door): the assembly loop drops the polygons out of its rectangle itself.
    // All the polygons of such a window whose depths are spread went through the slow path, drawn as one polygon by the
    // C code: 24 per frame from the walkway over a big room, 33 in a laboratory seen through its door; the master CPU
    // went from 25.5 to 23.8 ms and the slave CPU from 21.7 to 20.1 ms per frame in front of that door)
    loop.window_left = (short)(w->left - w->origin_x);
    loop.window_right = (short)(w->right - w->origin_x);
    loop.window_top = (short)(w->top - w->origin_y);
    loop.window_bottom = (short)(w->bottom - w->origin_y);
    loop.pre_clipping = jo_3d_pre_clipping(w);
    // (an environment map: the tile of each polygon is chosen by the C loop)
    ctx->environment = jo_3d_environment_of(pdata);
    fast = !ctx->mirror && ctx->front_shift < 0 && ctx->forced_key < 0 && ctx->texture_words == 0 && ctx->environment < 0;
#ifdef JO_COMPILE_WITH_PROFILER_SUPPORT
    if (__jo_profile_pixels)
        fast = false;
#endif
    for (; range < last_range; ++range)
    {
        p = pdata->polygons + range->first;
        a = pdata->attributes + range->first;
        loop.end = p + range->count;
        if (fast)
        {
            if (!__jo_3d_polygon_loop(&loop, p, a))
            {
                ctx->gouraud_offset = 0;
                ctx->environment = -1;
                jo_profile_end(JO_PROFILE_3D_POLYGONS);
                return (false);
            }
            continue;
        }
#ifdef JO_TEST_PATH_COUNTS
        // (the C loop: the polygons of the range)
        __jo_3d_path_counts[ctx->slave ? 5 : 1][0] += range->count;
#endif
        for (; p < loop.end; ++p, ++a)
            if (!__jo_3d_draw_polygon_slow(&loop, p, a))
            {
                ctx->gouraud_offset = 0;
                ctx->environment = -1;
                jo_profile_end(JO_PROFILE_3D_POLYGONS);
                return (false);
            }
    }
    ctx->gouraud_offset = 0;
    ctx->environment = -1;
    jo_profile_end(JO_PROFILE_3D_POLYGONS);
#ifdef JO_COMPILE_WITH_PROFILER_SUPPORT
    jo_profile_count(JO_PROFILE_3D_PIXELS, ctx->pixels);
    JO_ZERO(ctx->pixels);
#endif
    return (true);
}

/** @brief Average of 2 x 2 pixels of a texture (RGB 555): transparent when 2 pixels or more are transparent */
static jo_color                             jo_3d_average_pixels(const jo_color a, const jo_color b, const jo_color c, const jo_color d)
{
    const jo_color                          pixels[4] = {a, b, c, d};
    int                                     r;
    int                                     g;
    int                                     bl;
    int                                     n;
    int                                     k;

    JO_ZERO(r);
    JO_ZERO(g);
    JO_ZERO(bl);
    JO_ZERO(n);
    for (JO_ZERO(k); k < 4; ++k)
    {
        if (pixels[k] == JO_COLOR_Transparent || pixels[k] == JO_VDP1_RGB_END_CODE)
            continue;
        r += pixels[k] & 0x1f;
        g += (pixels[k] >> 5) & 0x1f;
        bl += (pixels[k] >> 10) & 0x1f;
        ++n;
    }
    if (n < 3)
        return (n == 2 ? (jo_color)(JO_COLOR_SATURN_MSB | (r >> 1) | ((g >> 1) << 5) | ((bl >> 1) << 10)) : JO_COLOR_Transparent);
    // (no division: / 4 is a shift, / 3 a product, exact up to 93 = 3 x 31)
    if (n == 4)
        return ((jo_color)(JO_COLOR_SATURN_MSB | (r >> 2) | ((g >> 2) << 5) | ((bl >> 2) << 10)));
    return ((jo_color)(JO_COLOR_SATURN_MSB | ((r * 0x5556) >> 16) | (((g * 0x5556) >> 16) << 5) | (((bl * 0x5556) >> 16) << 10)));
}

/** @brief The color of the table nearest to a color (1 to 15: the color 0 is transparent)
 *  @remarks Only the RGB colors of the table (their bit 15 set): the entries not used by the texture are 0 (see
 *  jo_sprite_add_image_4bits_lut()), the nearest one to a dark average was taken, a transparent pixel in the copy */
static int                                  jo_3d_nearest_lut_color(const jo_color * const lut, const jo_color c)
{
    int                                     best;
    int                                     best_distance;
    int                                     distance;
    int                                     k;
    int                                     dr;
    int                                     dg;
    int                                     db;

    best = 1;
    best_distance = 0x7fffffff;
    for (k = 1; k < 16; ++k)
    {
        if (!(lut[k] & JO_COLOR_SATURN_MSB))
            continue;
        dr = (int)(c & 0x1f) - (int)(lut[k] & 0x1f);
        dg = (int)((c >> 5) & 0x1f) - (int)((lut[k] >> 5) & 0x1f);
        db = (int)((c >> 10) & 0x1f) - (int)((lut[k] >> 10) & 0x1f);
        distance = dr * dr + dg * dg + db * db;
        if (distance < best_distance)
        {
            best_distance = distance;
            best = k;
        }
    }
    return (best);
}

/** @brief The nearest colors of the table already found (see jo_3d_set_texture_mipmaps_4bits()): a power of 2 */
# define JO_3D_LUT_MEMO                     (64)

/** @brief Mean color of the pixels of a texture that are not transparent (see __jo_sprite_mean_color, jo_3d_emit_sliver())
 *  @param pixels RGB pixels
 *  @param count Pixel count
 *  @return The color (its high bit set), 0: all transparent
 */
static unsigned short                       jo_3d_mean_color(const jo_color * const pixels, const int count)
{
    unsigned int                            sum[3];
    unsigned int                            n;
    unsigned int                            color;
    int                                     i;

    JO_ZERO(sum[0]);
    JO_ZERO(sum[1]);
    JO_ZERO(sum[2]);
    JO_ZERO(n);
    for (JO_ZERO(i); i < count; ++i)
    {
        color = (unsigned int)pixels[i];
        // (the transparent color and the end code of the VDP1)
        if (color == JO_COLOR_Transparent || color == JO_VDP1_RGB_END_CODE)
            continue;
        sum[0] += color & 0x1F;
        sum[1] += (color >> 5) & 0x1F;
        sum[2] += (color >> 10) & 0x1F;
        ++n;
    }
    if (n == 0)
        return (0);
    return ((unsigned short)(0x8000 | ((sum[2] / n) << 10) | ((sum[1] / n) << 5) | (sum[0] / n)));
}

/** @brief The smaller copies of a 16 colors texture (see jo_3d_set_texture_mipmaps()): the average of 2 x 2 pixels (their
 *  colors in the table), the nearest color of the table */
static bool                                 jo_3d_set_texture_mipmaps_4bits(const int sprite_id, jo_3d_texture_mipmaps * const mip,
                                                                        const jo_texture_definition * const texture)
{
    jo_color                                lut[16];
    jo_color                                *levels[3];
    unsigned char                           *packed;
    const unsigned char                     *source;
    const jo_color                          *s;
    jo_color                                c;
    int                                     w;
    int                                     h;
    int                                     level;
    int                                     x;
    int                                     y;
    int                                     id;
    int                                     index;
    unsigned int                            slot;
    jo_color                                memo_color[JO_3D_LUT_MEMO];
    unsigned char                           memo_index[JO_3D_LUT_MEMO];

    // (no color is 0: the opaque colors have their bit 15 set, the transparent one is not searched)
    jo_memset(memo_color, 0, sizeof(memo_color));
    jo_memcpy(lut, (const void *)(JO_VDP1_VRAM + JO_MULT_BY_8(__jo_sprite_lut[sprite_id])), sizeof(lut));
    source = (const unsigned char *)(JO_VDP1_VRAM + JO_MULT_BY_8(texture->adr));
    w = texture->width;
    h = texture->height;
    // The texture in RGB (the colors of the table, 0: transparent)
    levels[0] = (jo_color *)jo_malloc(w * h * sizeof(jo_color));
    levels[1] = JO_NULL;
    levels[2] = JO_NULL;
    if (levels[0] == JO_NULL)
        return (false);
    for (JO_ZERO(y); y < h; ++y)
        for (JO_ZERO(x); x < w; ++x)
        {
            index = (source[(y * w + x) >> 1] >> ((x & 1) ? 0 : 4)) & 0xf;
            levels[0][y * w + x] = index == 0 ? JO_COLOR_Transparent : lut[index];
        }
    __jo_sprite_mean_color[sprite_id] = jo_3d_mean_color(levels[0], w * h);
    for (level = 1; level <= 2; ++level)
    {
        if (JO_MOD_POW2(w / 2, 8) != 0 || h < 2)
            break;
        levels[level] = (jo_color *)jo_malloc((w / 2) * (h / 2) * sizeof(jo_color));
        packed = (unsigned char *)jo_malloc((w / 2) * (h / 2) / 2);
        if (levels[level] == JO_NULL || packed == JO_NULL)
        {
            if (packed != JO_NULL)
                jo_free(packed);
            break;
        }
        jo_memset(packed, 0, (w / 2) * (h / 2) / 2);
        for (JO_ZERO(y); y < h / 2; ++y)
            for (JO_ZERO(x); x < w / 2; ++x)
            {
                s = levels[level - 1] + (y * 2) * w + x * 2;
                c = jo_3d_average_pixels(s[0], s[1], s[w], s[w + 1]);
                levels[level][y * (w / 2) + x] = c;
                if (c == JO_COLOR_Transparent)
                    JO_ZERO(index);
                else
                {
                    // (the averages repeat: the nearest color of each one found once, the search of the table took 0.5 s of
                    // the loading of a level)
                    slot = (c ^ (c >> 6) ^ (c >> 11)) & (JO_3D_LUT_MEMO - 1);
                    if (memo_color[slot] != c)
                    {
                        memo_color[slot] = c;
                        memo_index[slot] = (unsigned char)jo_3d_nearest_lut_color(lut, c);
                    }
                    index = memo_index[slot];
                }
                packed[(y * (w / 2) + x) >> 1] |= (unsigned char)(index << ((x & 1) ? 0 : 4));
            }
        w /= 2;
        h /= 2;
        id = __jo_sprite_add_4bits(packed, (unsigned short)w, (unsigned short)h, __jo_sprite_lut[sprite_id]);
        jo_free(packed);
        if (level == 1)
            mip->first = id;
        // The 2 copies must follow each other
        if (id < 0 || id != mip->first + level - 1)
            break;
        mip->levels = level;
    }
    for (level = 0; level <= 2; ++level)
        if (levels[level] != JO_NULL)
            jo_free(levels[level]);
    jo_3d_mipmap_limits(mip, texture);
    return (mip->levels != 0);
}

/** @brief Make the smaller copies of a texture (see jo_3d_set_texture_mipmaps()) */
static bool                                 jo_3d_make_texture_mipmaps(const int sprite_id)
{
    jo_3d_texture_mipmaps                   *mip;
    const jo_texture_definition             *texture;
    const jo_color                          *source;
    jo_color                                *levels[3];
    jo_img                                  img;
    int                                     w;
    int                                     h;
    int                                     level;
    int                                     x;
    int                                     y;
    int                                     id;
    const jo_color                          *s;

    if (sprite_id < 0 || sprite_id >= JO_MAX_SPRITE)
        return (false);
    mip = &__jo_3d_mipmaps[sprite_id];
    if (mip->levels != 0)
        return (true);
    texture = &__jo_sprite_def[sprite_id];
    // A 16 colors texture: its copies with the same table
    if (__jo_sprite_pic[sprite_id].color_mode == JO_VDP1_TEXTURE_SIZE_SHIFT_16_COLORS)
        return (jo_3d_set_texture_mipmaps_4bits(sprite_id, mip, texture));
    // RGB textures
    if (__jo_sprite_pic[sprite_id].color_mode != JO_VDP1_TEXTURE_SIZE_SHIFT_32K_COLORS)
        return (false);
    source = (const jo_color *)(JO_VDP1_VRAM + JO_MULT_BY_8(texture->adr));
    levels[0] = (jo_color *)source;
    levels[1] = JO_NULL;
    levels[2] = JO_NULL;
    w = texture->width;
    h = texture->height;
    __jo_sprite_mean_color[sprite_id] = jo_3d_mean_color(source, w * h);
    for (level = 1; level <= 2; ++level)
    {
        // The width of a texture is a multiple of 8 pixels
        if (JO_MOD_POW2(w / 2, 8) != 0 || h < 2)
            break;
        levels[level] = (jo_color *)jo_malloc((w / 2) * (h / 2) * sizeof(jo_color));
        if (levels[level] == JO_NULL)
            break;
        for (JO_ZERO(y); y < h / 2; ++y)
            for (JO_ZERO(x); x < w / 2; ++x)
            {
                s = levels[level - 1] + (y * 2) * w + x * 2;
                levels[level][y * (w / 2) + x] = jo_3d_average_pixels(s[0], s[1], s[w], s[w + 1]);
            }
        w /= 2;
        h /= 2;
        img.width = w;
        img.height = h;
        img.data = levels[level];
        id = jo_sprite_add(&img);
        if (level == 1)
            mip->first = id;
        // The 2 copies must follow each other
        if (id < 0 || id != mip->first + level - 1)
            break;
        if (__jo_sprite_end_codes[sprite_id])
            jo_sprite_add_end_codes(id);
        mip->levels = level;
    }
    if (levels[1] != JO_NULL)
        jo_free(levels[1]);
    if (levels[2] != JO_NULL)
        jo_free(levels[2]);
    jo_3d_mipmap_limits(mip, texture);
    return (mip->levels != 0);
}

bool                                        jo_3d_set_texture_mipmaps(const int sprite_id)
{
    bool                                    result;

    if (sprite_id < 0 || sprite_id >= JO_MAX_SPRITE)
        return (false);
    result = jo_3d_make_texture_mipmaps(sprite_id);
    jo_3d_update_texture_flags(sprite_id);
    return (result);
}

void                                        jo_3d_set_front_layer(const int shift)
{
    __jo_3d_master.front_shift = shift;
}

bool                                        __jo_3d_draw_parts(const jo_3d_polygon_data * const pdata, const jo_3d_part * const parts,
                                                               const int part_count)
{
    bool                                    result;

    __jo_3d_master.parts = parts;
    __jo_3d_master.part_count = part_count;
    result = __jo_3d_draw_ranges(pdata, pdata->point_count, JO_NULL, 0);
    __jo_3d_master.parts = JO_NULL;
    __jo_3d_master.part_count = 0;
    return (result);
}


bool                                        __jo_3d_draw_ranges(const jo_3d_polygon_data * const pdata, const unsigned int point_count,
                                                                const jo_3d_polygon_range * const ranges, const int range_count)
{
    return (__jo_3d_draw_ranges_from(pdata, 0, point_count, ranges, range_count));
}

bool                                        __jo_3d_draw_ranges_from(const jo_3d_polygon_data * const pdata, const unsigned int first_point,
                                                                     const unsigned int point_count,
                                                                     const jo_3d_polygon_range * const ranges, const int range_count)
{
    jo_vdp1_command                         *commands;
    unsigned short                          *keys;
    unsigned int                            reserved;
    unsigned int                            used;
    bool                                    result;

    __jo_3d_master.matrix = JO_3D_CURRENT_MATRIX;
    commands = __jo_vdp1_reserve_commands(JO_3D_RESERVE_ALL_COMMANDS, &keys, &reserved);
    JO_ZERO(used);
    result = jo_3d_draw_polygons(&__jo_3d_master, pdata, first_point, point_count, ranges, range_count, __jo_vdp1_current_window, commands, keys,
                                 reserved, &used);
    // (when the command table is full, the commands were committed by jo_3d_emit_polygon())
    if (result)
        __jo_vdp1_commit_commands(used);
    return (result);
}

/** @brief The foot of a standing sprite in the view space, its depth first: the division of the projection is done by the
 *  division unit while x and y are computed (see jo_3d_standing_sprite_points())
 *  @param m Matrix
 *  @param in Foot (world)
 *  @param v Output: the foot in the view space
 *  @param k Output: the projection distance / the depth (when it is not too near)
 *  @return false if it is too near (or behind the camera): k is not computed
 */
static  __jo_force_inline bool              jo_3d_standing_sprite_foot(jo_fixed (* const m)[3], const jo_pos3D_fixed * const in,
                                                                       jo_pos3D_fixed * const v, jo_fixed * const k)
{
    jo_fixed                                columns[3][3];

    jo_3d_get_columns(m, columns);
    v->z = jo_fixed_dot((const jo_fixed *)in, columns[2]) + m[3][2];
    if (v->z < JO_FIXED_8)
        return (false);
    jo_fixed_div_start(__jo_vdp1_projection_distance, v->z);
    v->x = jo_fixed_dot((const jo_fixed *)in, columns[0]) + m[3][0];
    v->y = jo_fixed_dot((const jo_fixed *)in, columns[1]) + m[3][1];
    *k = jo_fixed_div_result();
    return (true);
}

/** @brief A product of the projection of a standing sprite, bounded (a foot near the camera but far to the side gave a
 *  product over 32767 that wrapped: the test of the screen passed, a sprite was drawn at a wrong place)
 */
static  __jo_force_inline jo_fixed          jo_3d_standing_sprite_mult(const jo_fixed a, const jo_fixed b)
{
    long long                               r;

    r = ((long long)a * b) >> 16;
    return ((jo_fixed)(r > 0x3FFFFFFF ? 0x3FFFFFFF : r < -0x3FFFFFFF ? -0x3FFFFFFF : r));
}

/** @brief Corners of a standing sprite on the screen (see jo_3d_draw_standing_sprite())
 *  @param v Position of the foot in the view space
 *  @param k The projection distance / its depth (see jo_3d_standing_sprite_foot())
 *  @param points Output: A top left, B top right, C bottom right, D bottom left (from the center of the window)
 *  @param width_on_screen Output: width on the screen (whole pixels, for the smaller copy of the texture)
 *  @param height_on_screen Output: height on the screen (whole pixels)
 *  @return false if it is not drawn (behind the camera, outside of the screen or too near)
 */
static  __jo_force_inline bool              jo_3d_standing_sprite_points(const jo_pos3D_fixed * const v, const jo_fixed k, const jo_fixed width,
                                                                         const jo_fixed height, const jo_fixed sway, const bool flip,
                                                                         jo_pos2D_fixed * const points, int * const width_on_screen,
                                                                         int * const height_on_screen)
{
    jo_fixed                                sx;
    jo_fixed                                sy;
    jo_fixed                                w;
    jo_fixed                                h;
    jo_fixed                                s;

    // Projection of the foot, size on the screen
    h = jo_3d_standing_sprite_mult(height, k);
    w = jo_3d_standing_sprite_mult(width, k) >> 1;
    sx = jo_3d_standing_sprite_mult(v->x, k);
    sy = jo_3d_standing_sprite_mult(v->y, k);
    // Outside of the screen
    if (sx + w < -jo_int2fixed(__jo_screen.width_2) || sx - w > jo_int2fixed(__jo_screen.width_2) ||
        sy < -jo_int2fixed(__jo_screen.height_2) || sy - h > jo_int2fixed(__jo_screen.height_2))
        return (false);
    // The camera is in the sprite: a huge sprite would hide the whole view (and cost a lot of VDP1 time): not drawn.
    // (a plant near the camera stays: it disappeared when the player came close to it)
    if (h > jo_int2fixed(__jo_screen.height * 2) || w > jo_int2fixed(__jo_screen.width))
        return (false);
    s = jo_fixed_mult(sway, k);
    // A: top left, B: top right, C: bottom right, D: bottom left (the sway moves the top)
    points[flip ? 1 : 0].x = sx - w + s;
    points[flip ? 1 : 0].y = sy - h;
    points[flip ? 0 : 1].x = sx + w + s;
    points[flip ? 0 : 1].y = sy - h;
    points[flip ? 3 : 2].x = sx + w;
    points[flip ? 3 : 2].y = sy;
    points[flip ? 2 : 3].x = sx - w;
    points[flip ? 2 : 3].y = sy;
    *width_on_screen = jo_fixed2int(w) * 2;
    *height_on_screen = jo_fixed2int(h);
    return (true);
}

/*
** Fade of the far standing sprites (see jo_3d_set_sprite_fade()): their 16 colors in a bank of the color RAM, the color
** calculation bits of the sprites (bits 12 and 11 of a palette pixel, sprite type 3) give the ratio of their mix with the
** screens of the VDP2 under them, by distance. The VDP2 mixes each pixel once, at the display: no edge, no line drawn twice
*/

/** @brief The distances of the fade: its start and the step of a level (jo_fixed, the step 0: no fade) */
typedef struct
{
    jo_fixed                                start;
    jo_fixed                                levels_per_unit;
}                                           jo_3d_fade;

static jo_3d_fade                           __jo_3d_fade = {0, 0};

/** @brief The fade of a range of distances (see jo_3d_set_fade_distances()) */
static void                                 jo_3d_fade_set(jo_3d_fade * const fade, const jo_fixed start, const jo_fixed end)
{
    fade->start = start;
    // (JO_3D_FADE_LEVELS levels from start to end: the level of a depth is 1 + its distance after start times this)
    fade->levels_per_unit = end > start ? jo_fixed_div(jo_int2fixed(JO_3D_FADE_LEVELS), end - start) : 0;
}

void                                        jo_3d_set_fade_distances(const jo_fixed start, const jo_fixed end)
{
    jo_3d_fade_set(&__jo_3d_fade, start, end);
}

bool                                        jo_3d_set_sprite_fade(const int sprite_id, const jo_palette * const palette, const int bank)
{
    int                                     k;
    int                                     n;
    const unsigned short                    *lut;

    if (sprite_id < 0 || sprite_id >= JO_MAX_SPRITE)
        return (false);
    // (the sprite forgotten: its bank 0)
    for (JO_ZERO(k); k < __jo_3d_fade_sprite_count && __jo_3d_fade_sprites[k].sprite != sprite_id; ++k)
        ;
    if (palette == JO_NULL || palette->data == JO_NULL || bank <= 0)
    {
        if (k < __jo_3d_fade_sprite_count)
            __jo_3d_fade_sprites[k] = __jo_3d_fade_sprites[--__jo_3d_fade_sprite_count];
        return (true);
    }
    // A 16 colors texture with its table only (its colors copied in the bank)
    if (__jo_sprite_pic[sprite_id].color_mode != JO_VDP1_TEXTURE_SIZE_SHIFT_16_COLORS || __jo_sprite_lut[sprite_id] == 0 ||
        bank >= 16 || (k >= __jo_3d_fade_sprite_count && __jo_3d_fade_sprite_count >= JO_3D_FADE_SPRITES))
        return (false);
    // (the color c of the bank is the color index bank * 16 + c of the palette: its data[bank * 16 + c - 1], see
    // jo_create_palette(); the color 0 is transparent, not written)
    lut = (const unsigned short *)(JO_VDP1_VRAM + JO_MULT_BY_8(__jo_sprite_lut[sprite_id]));
    for (n = 1; n < 16; ++n)
        palette->data[bank * 16 + n - 1] = lut[n];
    if (k >= __jo_3d_fade_sprite_count)
        ++__jo_3d_fade_sprite_count;
    __jo_3d_fade_sprites[k].sprite = (short)sprite_id;
    __jo_3d_fade_sprites[k].bank = (unsigned short)(palette->id * 256 + bank * 16);
    return (true);
}

/** @brief The attribute of a standing sprite that fades: its colors in their bank, the level of its depth (see
 *  jo_3d_set_sprite_fade())
 *  @param sprite_id The sprite (not its smaller copy: the attribute has its texture)
 *  @param z Depth of the sprite
 */
static  __jo_force_inline void              jo_3d_fade_attribute(jo_vdp1_sprite_attribute * const attr, const jo_3d_fade * const fade,
                                                                 const int sprite_id, const jo_fixed z)
{
    int                                     k;
    int                                     level;

    if (fade->levels_per_unit == 0 || z <= fade->start || __jo_3d_fade_sprite_count == 0 || __jo_screen.hires)
        return;
    for (JO_ZERO(k); k < __jo_3d_fade_sprite_count; ++k)
        if (__jo_3d_fade_sprites[k].sprite == sprite_id)
            break;
    if (k >= __jo_3d_fade_sprite_count)
        return;
    level = 1 + jo_fixed2int(jo_fixed_mult(z - fade->start, fade->levels_per_unit));
    if (level > JO_3D_FADE_LEVELS)
        level = JO_3D_FADE_LEVELS;
    // (a palette pixel: no gouraud, the VDP1 shades the RGB pixels only)
    attr->draw_mode = (unsigned short)((attr->draw_mode & ~(JO_VDP1_COLOR_MODE_MASK | JO_VDP1_COLOR_CALC_GOURAUD)) |
                                       JO_VDP1_COLOR_MODE_16_BANK);
    attr->color = (unsigned short)(__jo_3d_fade_sprites[k].bank | (level << JO_3D_FADE_LEVEL_SHIFT));
}

#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT

/*
** 3D on the slave CPU: the slave CPU draws objects with its own state (matrix, transformed vertices, commands), at the
** same time as the master CPU. Its commands are added to the frame by jo_3d_slave_end()
*/

/* (the commands of the slave CPU are added to the frame of the master CPU: 40 commands of the frame stay for the master
** CPU, 960 by default) */
# define JO_3D_SLAVE_MAX_COMMANDS           (JO_VDP1_MAX_COMMANDS - 40)
# define JO_3D_SLAVE_MAX_VERTICES           (1024)

static jo_vdp1_command                      __jo_3d_slave_commands[JO_3D_SLAVE_MAX_COMMANDS] __attribute__((aligned(4)));
static unsigned short                       __jo_3d_slave_keys[JO_3D_SLAVE_MAX_COMMANDS] __attribute__((aligned(4)));
static unsigned int                         __jo_3d_slave_last_command_count;
static jo_3d_vertex                         __jo_3d_slave_vertices[JO_3D_SLAVE_MAX_VERTICES];
static jo_3d_view_position                  __jo_3d_slave_view[JO_3D_SLAVE_MAX_VERTICES];
# define JO_3D_SLAVE_MATRIX_STACK_DEPTH     (4)
static jo_3d_matrix                         __jo_3d_slave_matrix[JO_3D_SLAVE_MATRIX_STACK_DEPTH];
static int                                  __jo_3d_slave_matrix_level;
static int                                  __jo_3d_slave_depth_bias;
static int                                  __jo_3d_slave_guard_band;
static jo_pos3D_fixed                       __jo_3d_slave_light_direction;
static int                                  __jo_3d_slave_window;
static unsigned short                       __jo_3d_slave_polygon_count;
static unsigned short                       __jo_3d_slave_vertice_count;
static unsigned short                       __jo_3d_slave_displayed_polygon_count;
/** @brief The sprite attributes of the master CPU at jo_3d_slave_begin() (it changes them while the slave CPU draws) */
static jo_sprite_attributes                 __jo_3d_slave_sprite_attributes;
/** @brief The fade of the standing sprites of the slave CPU (the one of the master CPU at jo_3d_slave_begin()) */
static jo_3d_fade                           __jo_3d_slave_fade;
static jo_3d_cpu_context                    __jo_3d_slave = {__jo_3d_slave_vertices, JO_3D_SLAVE_MAX_VERTICES, __jo_3d_slave_view, __jo_3d_slave_matrix[0], 0, -1,
                                                             &__jo_3d_slave_depth_bias, 0, 0, JO_3D_CLIP_GOURAUD_TABLES,
                                                             &__jo_3d_slave_polygon_count, &__jo_3d_slave_vertice_count,
                                                             &__jo_3d_slave_displayed_polygon_count, true, false, 0, JO_NULL, 0, -1,
                                                             &__jo_3d_slave_guard_band, &__jo_3d_slave_light_direction, 0, 0, 0, -1, 0,
                                                             JO_NULL, 0};

/** @brief A job of the slave CPU: the end of the copy of its commands by its DMA controller (the next job draws in the
 *  same table) */
static void                                 jo_3d_slave_wait_copy_job(void *argument)
{
    (void)argument;
    jo_dma_wait();
}

void                                        jo_3d_slave_begin(void)
{
    // (a second job of the slave CPU in the frame (a split screen): its commands of the first job are still copied by its
    // DMA controller; its next drawings overwrite them: it waits for the end of the copy first)
    if (__jo_3d_slave_copy_started)
        __jo_core_add_slave_job(jo_3d_slave_wait_copy_job, JO_NULL);
    jo_memcpy(__jo_3d_slave_matrix[0], JO_3D_CURRENT_MATRIX, sizeof(jo_3d_matrix));
    __jo_3d_slave_matrix_level = 0;
    __jo_3d_slave.matrix = __jo_3d_slave_matrix[0];
    __jo_3d_slave_depth_bias = __jo_vdp1_depth_bias;
    // (the master CPU changes them while the slave CPU draws: for instance the guard band of the weapon)
    __jo_3d_slave_guard_band = __jo_3d_guard_band;
    __jo_3d_slave_light_direction = __jo_3d_light_direction;
    __jo_3d_slave_window = __jo_vdp1_current_window;
    __jo_3d_slave_view_rect = __jo_3d_view_rect;
    __jo_3d_slave_sprite_attributes = __jo_sprite_attributes;
    __jo_3d_slave_fade = __jo_3d_fade;
    __jo_3d_slave.previous_polygon_z = 0;
    __jo_3d_slave.forced_key = -1;
    __jo_3d_slave_tint = 0;
    // The last gouraud tables of the set of this frame (the first job of the frame; the next ones take the tables after)
    if (!__jo_3d_slave_tables_taken)
    {
        __jo_3d_slave_tables_taken = true;
        __jo_3d_slave.clip_table_first = __jo_3d_clip_table_base + JO_3D_CLIP_GOURAUD_TABLES - 1;
        __jo_3d_slave.clip_table_count = 0;
        *(volatile unsigned int *)JO_CACHE_THROUGH(&__jo_3d_clip_tables_used[1]) = 0;
    }
    JO_ZERO(__jo_3d_slave_command_count);
    JO_ZERO(__jo_3d_slave_polygon_count);
    JO_ZERO(__jo_3d_slave_vertice_count);
    JO_ZERO(__jo_3d_slave_displayed_polygon_count);
}

jo_fixed                                    (*__jo_3d_slave_current_matrix(void))[3]
{
    return (__jo_3d_slave.matrix);
}

void                                        jo_3d_slave_select_window(const int window)
{
    __jo_3d_slave_window = window;
}

void                                        jo_3d_slave_set_depth_bias(const int bias)
{
    __jo_3d_slave_depth_bias = bias;
}

void                                        jo_3d_slave_set_fade_distances(const jo_fixed start, const jo_fixed end)
{
    jo_3d_fade_set(&__jo_3d_slave_fade, start, end);
}

void                                        jo_3d_slave_set_guard_band(const int pixels)
{
    __jo_3d_slave_guard_band = pixels < 0 ? 0 : (pixels > JO_3D_MAX_GUARD_BAND ? JO_3D_MAX_GUARD_BAND : pixels);
}

void                                        jo_3d_slave_transform_point(const jo_pos3D_fixed * const in, jo_pos3D_fixed *out)
{
    jo_fixed                                columns[3][3];

    jo_3d_get_columns(__jo_3d_slave.matrix, columns);
    jo_3d_transform_vector(columns, (const jo_fixed *)in, (jo_fixed *)out);
    out->x += __jo_3d_slave.matrix[3][0];
    out->y += __jo_3d_slave.matrix[3][1];
    out->z += __jo_3d_slave.matrix[3][2];
}

bool                                        jo_3d_slave_push_matrix(void)
{
    if (__jo_3d_slave_matrix_level + 1 >= JO_3D_SLAVE_MATRIX_STACK_DEPTH)
        return (false);
    ++__jo_3d_slave_matrix_level;
    jo_3d_copy_matrix(__jo_3d_slave_matrix[__jo_3d_slave_matrix_level], (const jo_fixed (*)[3])__jo_3d_slave_matrix[__jo_3d_slave_matrix_level - 1]);
    __jo_3d_slave.matrix = __jo_3d_slave_matrix[__jo_3d_slave_matrix_level];
    return (true);
}

void                                        jo_3d_slave_pop_matrix(void)
{
    if (__jo_3d_slave_matrix_level <= 0)
        return;
    --__jo_3d_slave_matrix_level;
    __jo_3d_slave.matrix = __jo_3d_slave_matrix[__jo_3d_slave_matrix_level];
}

void                                        jo_3d_slave_translate_matrix_fixed(const jo_fixed x, const jo_fixed y, const jo_fixed z)
{
    jo_3d_matrix_translate(__jo_3d_slave.matrix, x, y, z);
}

void                                        jo_3d_slave_rotate(const int axis, const jo_angle angle)
{
    jo_3d_matrix_rotate(__jo_3d_slave.matrix, axis, angle);
}

void                                        jo_3d_slave_rotate_matrix(const int x, const int y, const int z)
{
    if (x)
        jo_3d_matrix_rotate(__jo_3d_slave.matrix, JO_3D_AXIS_X, jo_DEGtoANG_int(x));
    if (y)
        jo_3d_matrix_rotate(__jo_3d_slave.matrix, JO_3D_AXIS_Y, jo_DEGtoANG_int(y));
    if (z)
        jo_3d_matrix_rotate(__jo_3d_slave.matrix, JO_3D_AXIS_Z, jo_DEGtoANG_int(z));
}

void                                        jo_3d_slave_set_scale_fixed(const jo_fixed x, const jo_fixed y, const jo_fixed z)
{
    jo_3d_matrix_scale(__jo_3d_slave.matrix, x, y, z);
}

bool                                        jo_3d_slave_mesh_draw_ranges(const jo_3d_mesh * const mesh, const unsigned int point_count,
                                                                         const jo_3d_polygon_range * const ranges, const int range_count)
{
    return (__jo_3d_slave_draw_ranges_from(mesh, 0, point_count, ranges, range_count));
}

bool                                        __jo_3d_slave_draw_ranges_from(const jo_3d_mesh * const mesh, const unsigned int first_point,
                                                                           const unsigned int point_count,
                                                                           const jo_3d_polygon_range * const ranges, const int range_count)
{
    unsigned int                            used;
    bool                                    result;

    used = __jo_3d_slave_command_count;
    result = jo_3d_draw_polygons(&__jo_3d_slave, &mesh->data, first_point, point_count, ranges, range_count, __jo_3d_slave_window,
                                 __jo_3d_slave_commands, __jo_3d_slave_keys, JO_3D_SLAVE_MAX_COMMANDS, &used);
    __jo_3d_slave_command_count = used;
    return (result);
}

bool                                        jo_3d_slave_draw_standing_sprite(const int sprite_id, const jo_pos3D_fixed * const foot,
                                                                             const jo_fixed width, const jo_fixed height,
                                                                             const jo_fixed sway, const bool flip)
{
    jo_pos3D_fixed                          v;
    jo_pos2D_fixed                          points[4];
    jo_vdp1_sprite_attribute                attr;
    jo_fixed                                k;
    int                                     w;
    int                                     h;
    int                                     key;

    if (sprite_id < 0 || sprite_id >= JO_MAX_SPRITE || __jo_3d_slave_command_count >= JO_3D_SLAVE_MAX_COMMANDS)
        return (false);
    if (!jo_3d_standing_sprite_foot(__jo_3d_slave.matrix, foot, &v, &k) ||
        !jo_3d_standing_sprite_points(&v, k, width, height, sway, flip, points, &w, &h))
        return (false);
    // Far away: a smaller copy of the texture. The command of jo_3d_draw_standing_sprite() (the sprite attributes of the
    // master CPU at jo_3d_slave_begin()), with the window and the depth bias of the slave CPU
    __jo_sprite_attributes_of(&attr, jo_3d_mipmap(sprite_id, w, h), &__jo_3d_slave_sprite_attributes);
    jo_3d_fade_attribute(&attr, &__jo_3d_slave_fade, sprite_id, v.z);
    if (!__jo_vdp1_write_sprite_4p(&__jo_3d_slave_commands[__jo_3d_slave_command_count], points, v.z, &attr, __jo_3d_slave_window))
        return (false);
    key = JO_FIXED_TO_INT(v.z) + __jo_3d_slave_depth_bias;
    __jo_3d_slave_keys[__jo_3d_slave_command_count++] = key < 0 ? 0 : (key > JO_VDP1_MAX_KEY ? JO_VDP1_MAX_KEY : (unsigned short)key);
    return (true);
}

bool                                        __jo_3d_slave_draw_parts(const jo_3d_polygon_data * const pdata, const jo_3d_part * const parts,
                                                                     const int part_count)
{
    unsigned int                            used;
    bool                                    result;

    __jo_3d_slave.parts = parts;
    __jo_3d_slave.part_count = part_count;
    used = __jo_3d_slave_command_count;
    result = jo_3d_draw_polygons(&__jo_3d_slave, pdata, 0, pdata->point_count, JO_NULL, 0, __jo_3d_slave_window,
                                 __jo_3d_slave_commands, __jo_3d_slave_keys, JO_3D_SLAVE_MAX_COMMANDS, &used);
    __jo_3d_slave_command_count = used;
    __jo_3d_slave.parts = JO_NULL;
    __jo_3d_slave.part_count = 0;
    return (result);
}

int                                         jo_3d_slave_get_current_command_count(void)
{
    // (also read by the master CPU while the slave CPU draws: without the cache)
    return ((int)*(volatile unsigned int *)JO_CACHE_THROUGH(&__jo_3d_slave_command_count));
}

/** @brief The commands of the slave CPU to copy in the frame (see jo_3d_slave_end()) */
typedef struct
{
    jo_vdp1_command                         *commands;
    unsigned short                          *keys;
    unsigned short                          *windows;
    unsigned int                            count;
    unsigned short                          window;
}                                           jo_3d_slave_copy;

/** @brief The copies waiting (as many as the jobs of the slave CPU: an entry is used again after its job is done) */
static jo_3d_slave_copy                     __jo_3d_slave_copies[8];
static unsigned int                         __jo_3d_slave_copy_index = 0;

/** @brief Copy the commands of the slave CPU in the frame (a job of the slave CPU: the master CPU doesn't wait for the copy, the
 *  jobs are done in order: before the frame is sent, and before the slave CPU draws again in its command table)
 *  @param argument Copy (jo_3d_slave_copy)
 */
static void                                 jo_3d_slave_copy_job(void *argument)
{
    const jo_3d_slave_copy                  *c;
    register unsigned int                   i;

    c = (const jo_3d_slave_copy *)argument;
    // The commands are copied by the DMA controller of the slave CPU while the slave CPU sorts the frame (its next
    // job, which needs the keys only): jo_memcpy() took 1 ms of the slave CPU for 400 commands. jo_vdp1_send_frame()
    // waits for the end of the copy before the SCU DMA reads the commands. The slave CPU draws again in its table
    // only after the frame is sent: the source is not changed before the end of the copy.
    jo_dma_copy_async(__jo_3d_slave_commands, c->commands, c->count * sizeof(jo_vdp1_command));
    jo_memcpy(c->keys, __jo_3d_slave_keys, c->count * sizeof(unsigned short));
    for (JO_ZERO(i); i < c->count; ++i)
        c->windows[i] = c->window;
}

/** @brief VDP1 commands kept for what is drawn after jo_3d_slave_end() (see jo_3d_slave_set_kept_commands()) */
static int                                  __jo_3d_slave_kept_commands = 0;

void                                        jo_3d_slave_set_kept_commands(const int count)
{
    __jo_3d_slave_kept_commands = JO_MAX(count, 0);
}

void                                        jo_3d_slave_end(void)
{
    int                                     room;
    jo_vdp1_command                         *commands;
    unsigned short                          *keys;
    unsigned int                            reserved;
    jo_3d_slave_copy                        *c;

    // (the counts of the slave CPU even without command: all its polygons hidden)
    __jo_3d_polygon_count += __jo_3d_slave_polygon_count;
    __jo_3d_vertice_count += __jo_3d_slave_vertice_count;
    __jo_3d_displayed_polygon_count += __jo_3d_slave_displayed_polygon_count;
    __jo_3d_slave_last_command_count = __jo_3d_slave_command_count;
    // (added once: a second jo_3d_slave_end() adds nothing)
    JO_ZERO(__jo_3d_slave_polygon_count);
    JO_ZERO(__jo_3d_slave_vertice_count);
    JO_ZERO(__jo_3d_slave_displayed_polygon_count);
    if (__jo_3d_slave_command_count == 0)
        return;
    // (not the commands kept for what is drawn after, see jo_3d_slave_set_kept_commands())
    room = jo_vdp1_command_max() - __jo_vdp1_frame_command_count() - __jo_3d_slave_kept_commands;
    commands = __jo_vdp1_reserve_commands((unsigned int)JO_MAX(0, JO_MIN((int)__jo_3d_slave_command_count, room)), &keys, &reserved);
    // The slave CPU copies its commands (with the window of the slave CPU): about 1 ms of the master CPU for 500 commands
    c = &__jo_3d_slave_copies[__jo_3d_slave_copy_index];
    __jo_3d_slave_copy_index = (__jo_3d_slave_copy_index + 1) & 7;
    c->commands = commands;
    c->keys = keys;
    c->count = reserved;
    c->window = (unsigned short)__jo_3d_slave_window;
    c->windows = __jo_vdp1_commit_commands_without_window(reserved);
    if (reserved > 0)
    {
        __jo_core_add_slave_job(jo_3d_slave_copy_job, c);
        __jo_3d_slave_copy_started = true;
    }
    JO_ZERO(__jo_3d_slave_command_count);
}

int                                         jo_3d_slave_get_command_count(void)
{
    return ((int)__jo_3d_slave_last_command_count);
}

int                                         jo_3d_slave_get_max_commands(void)
{
    return (JO_3D_SLAVE_MAX_COMMANDS);
}

int                                         jo_3d_slave_get_max_vertices(void)
{
    return (JO_3D_SLAVE_MAX_VERTICES);
}

#endif /* !JO_COMPILE_WITH_DUAL_CPU_SUPPORT */

void                                        jo_3d_init(void)
{
    register int		                    i;

    __jo_3d_matrix_level = 0;
    __jo_3d_matrix_overflow = 0;
    jo_3d_set_identity(JO_3D_CURRENT_MATRIX);
    __jo_3d_set_perspective(JO_DEG_TO_ANGLE(JO_3D_DEFAULT_PERSPECTIVE_ANGLE));
    __jo_3d_light_direction.x = JO_FIXED_0;
    __jo_3d_light_direction.y = JO_FIXED_0;
    __jo_3d_light_direction.z = JO_FIXED_1;
    JO_ZERO(__jo_3d_master.previous_polygon_z);
    jo_3d_display_level(JO_3D_DEFAULT_DISPLAY_LEVEL);
    for (JO_ZERO(i); i < JO_MAX_SPRITE; ++i)
    {
        __jo_sprite_quad[i] = JO_NULL;
        __jo_3d_tiles[i].levels = 0;
        __jo_3d_tiles[i].error = 0;
        __jo_3d_tiles[i].depth = 0;
        __jo_3d_mipmaps[i].levels = 0;
        __jo_3d_texture_flags[i] = 0;
    }
}

/** @brief Convert integer vertices to fixed numbers
 *  @param vertices Vertices (input and output)
 *  @param count Vertex count
 */
static  __jo_force_inline void      __jo_to_fixed_vertices(jo_vertice * const vertices, int count)
{
    while (--count >= 0)
    {
        vertices[count].x <<= 16;
        vertices[count].y <<= 16;
        vertices[count].z <<= 16;
    }
}

void                        jo_3d_create_triangle(jo_3d_quad * const quad, jo_vertice * const vertices)
{
    static jo_3d_polygon          polygon[] = { { {JO_FIXED_0, JO_FIXED_1, JO_FIXED_0}, JO_3D_VERTICES(0, 1, 2, 2) } };

    __jo_to_fixed_vertices(vertices, 3);
    quad->data.point_count = 3;
    quad->data.points = (jo_pos3D_fixed *)vertices;
    quad->data.polygon_count = 1;
    quad->data.polygons = polygon;
    quad->data.attributes = &quad->attribute;
    quad->data.attributes->plane = JO_3D_DUAL_PLANE;
    JO_ZERO(quad->data.attributes->gouraud_table);
}

void                        jo_3d_create_plane(jo_3d_quad * const quad, jo_vertice * const vertices)
{
    static jo_3d_polygon          polygon[] = { { {JO_FIXED_0, JO_FIXED_1, JO_FIXED_0}, JO_3D_VERTICES(0, 1, 2, 3) } };

    __jo_to_fixed_vertices(vertices, 4);
    quad->data.point_count = 4;
    quad->data.points = (jo_pos3D_fixed *)vertices;
    quad->data.polygon_count = 1;
    quad->data.polygons = polygon;
    quad->data.attributes = &quad->attribute;
    quad->data.attributes->plane = JO_3D_DUAL_PLANE;
    JO_ZERO(quad->data.attributes->gouraud_table);
}

/** @brief Allocate a mesh
 *  @param point_count Vertex count
 *  @param polygon_count Polygon count
 *  @return Mesh (polygons and points not initialized, attributes: dual plane white polygons) or JO_NULL if out of memory
 */
static jo_3d_mesh           *jo_3d_allocate_mesh(const unsigned int point_count, const unsigned int polygon_count)
{
    unsigned int            i;
    jo_3d_mesh              *mesh;

    if ((mesh = (jo_3d_mesh *)jo_malloc_with_behaviour(sizeof(*mesh), JO_MALLOC_TRY_REUSE_SAME_BLOCK_SIZE)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory");
#endif
        return (JO_NULL);
    }
    mesh->data.point_count = point_count;
    mesh->data.polygon_count = polygon_count;
    mesh->data.points = (jo_pos3D_fixed *)jo_malloc_with_behaviour(point_count * sizeof(*mesh->data.points), JO_MALLOC_TRY_REUSE_SAME_BLOCK_SIZE);
    mesh->data.polygons = (jo_3d_polygon *)jo_malloc_with_behaviour(polygon_count * sizeof(*mesh->data.polygons), JO_MALLOC_TRY_REUSE_SAME_BLOCK_SIZE);
    mesh->data.attributes = (jo_3d_polygon_attribute *)jo_malloc_with_behaviour(polygon_count * sizeof(*mesh->data.attributes), JO_MALLOC_TRY_REUSE_SAME_BLOCK_SIZE);
    if (mesh->data.points == JO_NULL || mesh->data.polygons == JO_NULL || mesh->data.attributes == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory");
#endif
        if (mesh->data.points != JO_NULL)
            jo_free(mesh->data.points);
        if (mesh->data.polygons != JO_NULL)
            jo_free(mesh->data.polygons);
        if (mesh->data.attributes != JO_NULL)
            jo_free(mesh->data.attributes);
        jo_free(mesh);
        return (JO_NULL);
    }
    jo_memset(mesh->data.attributes, 0, polygon_count * sizeof(*mesh->data.attributes));
    for (JO_ZERO(i); i < polygon_count; ++i)
    {
        mesh->data.attributes[i].plane = JO_3D_DUAL_PLANE;
        jo_3d_set_mesh_polygon_color(mesh, JO_COLOR_White, i);
    }
    return (mesh);
}

jo_3d_mesh                  *jo_3d_create_mesh_from_vertices_and_normals(const unsigned int quad_count, jo_vertice * const vertices, jo_vector * const normals)
{
    unsigned int            i;
    unsigned int            j;
    jo_3d_mesh              *mesh;

    if ((mesh = jo_3d_allocate_mesh(JO_MULT_BY_4(quad_count), quad_count)) == JO_NULL)
        return (JO_NULL);
    if (vertices != JO_NULL)
    {
        for (JO_ZERO(i); i < mesh->data.point_count; ++i)
        {
            mesh->data.points[i].x = jo_int2fixed(vertices[i].x);
            mesh->data.points[i].y = jo_int2fixed(vertices[i].y);
            mesh->data.points[i].z = jo_int2fixed(vertices[i].z);
        }
    }
    else
        jo_memset(mesh->data.points, 0, mesh->data.point_count * sizeof(*mesh->data.points));
    for (JO_ZERO(i); i < quad_count; ++i)
    {
        if (normals != JO_NULL)
        {
            mesh->data.polygons[i].normal.x = jo_int2fixed(normals[i].x);
            mesh->data.polygons[i].normal.y = jo_int2fixed(normals[i].y);
            mesh->data.polygons[i].normal.z = jo_int2fixed(normals[i].z);
        }
        else
        {
            mesh->data.polygons[i].normal.x = JO_FIXED_0;
            mesh->data.polygons[i].normal.y = JO_FIXED_1;
            mesh->data.polygons[i].normal.z = JO_FIXED_0;
        }
        // 4 vertices per quad
        for (JO_ZERO(j); j < 4; ++j)
            mesh->data.polygons[i].vertices[j] = JO_MULT_BY_4(i) + j;
    }
    return (mesh);
}

/** @brief Index of a vertex of a sphere created by jo_3d_create_sphere()
 *  @param stack Stack index (0: top pole, stacks: bottom pole)
 *  @param slice Slice index
 *  @param slices Slice count
 *  @param stacks Stack count
 *  @return Vertex index
 */
static  __jo_force_inline unsigned short    jo_3d_sphere_vertex_index(const unsigned int stack, const unsigned int slice, const unsigned int slices, const unsigned int stacks)
{
    if (stack == 0)
        return (0);
    if (stack >= stacks)
        return ((stacks - 1) * slices + 1);
    return (1 + (stack - 1) * slices + (slice % slices));
}

/** @brief Direction of a point of a sphere (unit vector, Y goes down)
 *  @param latitude Angle from the top pole (0x8000 = 180 degrees)
 *  @param longitude Angle around the Y axis (0x10000 = 360 degrees)
 *  @param output Unit vector
 */
static void                 jo_3d_sphere_direction(const jo_angle latitude, const jo_angle longitude, jo_pos3D_fixed *output)
{
    jo_fixed                s;

    s = jo_sin_angle(latitude);
    output->x = jo_fixed_mult(s, jo_cos_angle(longitude));
    output->y = -jo_cos_angle(latitude);
    output->z = jo_fixed_mult(s, jo_sin_angle(longitude));
}

jo_3d_mesh                  *jo_3d_create_sphere(const int radius, const unsigned int slices, const unsigned int stacks)
{
    jo_3d_mesh              *mesh;
    jo_pos3D_fixed          *p;
    jo_3d_polygon           *polygon;
    unsigned int            stack;
    unsigned int            slice;
    jo_fixed                r;

#ifdef JO_DEBUG
    if (slices < 3 || stacks < 2)
    {
        jo_core_error("A sphere needs at least 3 slices and 2 stacks");
        return (JO_NULL);
    }
#endif
    if (slices < 3 || stacks < 2)
        return (JO_NULL);
    // Vertices are shared: the 2 poles and the (stacks - 1) rings
    if ((mesh = jo_3d_allocate_mesh((stacks - 1) * slices + 2, slices * stacks)) == JO_NULL)
        return (JO_NULL);
    r = jo_int2fixed(radius);
    for (JO_ZERO(stack); stack <= stacks; ++stack)
    {
        for (JO_ZERO(slice); slice < slices; ++slice)
        {
            p = &mesh->data.points[jo_3d_sphere_vertex_index(stack, slice, slices, stacks)];
            jo_3d_sphere_direction((jo_angle)((stack * 0x8000) / stacks), (jo_angle)((slice * 0x10000) / slices), p);
            p->x = jo_fixed_mult(p->x, r);
            p->y = jo_fixed_mult(p->y, r);
            p->z = jo_fixed_mult(p->z, r);
            // Only one vertex for each pole
            if (stack == 0 || stack == stacks)
                break;
        }
    }
    polygon = mesh->data.polygons;
    for (JO_ZERO(stack); stack < stacks; ++stack)
    {
        for (JO_ZERO(slice); slice < slices; ++slice, ++polygon)
        {
            // Clockwise seen from the outside (the quads near the poles are triangles)
            polygon->vertices[0] = jo_3d_sphere_vertex_index(stack, slice, slices, stacks);
            polygon->vertices[1] = jo_3d_sphere_vertex_index(stack, slice + 1, slices, stacks);
            polygon->vertices[2] = jo_3d_sphere_vertex_index(stack + 1, slice + 1, slices, stacks);
            polygon->vertices[3] = jo_3d_sphere_vertex_index(stack + 1, slice, slices, stacks);
            // Normal at the center of the quad
            jo_3d_sphere_direction((jo_angle)(((2 * stack + 1) * 0x8000) / (2 * stacks)), (jo_angle)(((2 * slice + 1) * 0x10000) / (2 * slices)), &polygon->normal);
        }
    }
    // Closed shape: the back faces are never visible
    for (JO_ZERO(slice); slice < mesh->data.polygon_count; ++slice)
        mesh->data.attributes[slice].plane = JO_3D_SINGLE_PLANE;
    return (mesh);
}

void                        jo_3d_free_mesh(const jo_3d_mesh * const mesh)
{
#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
#endif
    // (the caches of the tints and of the environment maps: a new mesh at the same address got the copies of the freed
    // one, 16 meshes freed and the tints did not work anymore)
    __jo_3d_forget_tint(&mesh->data);
    __jo_3d_forget_environment(&mesh->data);
    jo_free(mesh->data.points);
    jo_free(mesh->data.polygons);
    jo_free(mesh->data.attributes);
    jo_free(mesh);
}

void                        jo_3d_create_cube(jo_3d_quad * const array, jo_vertice * const vertices)
{
    jo_3d_create_plane(array, vertices);
    jo_3d_create_plane(array + 1, vertices + 4);
    jo_3d_create_plane(array + 2, vertices + 8);
    jo_3d_create_plane(array + 3, vertices + 12);
    jo_3d_create_plane(array + 4, vertices + 16);
    jo_3d_create_plane(array + 5, vertices + 20);
}

/** @brief Draw mode options kept when the texture, the color or the wireframe mode of a polygon changes
 *  @param attribute Attribute of the polygon
 *  @param textured The new polygon is textured (the high speed shrink only applies to the textures)
 *  @return The screen doors, the half transparency and the high speed shrink of the polygon (lost before: only the screen
 *          doors were kept, a jo_3d_set_mesh_transparency() before a jo_3d_set_mesh_texture() was lost)
 */
static unsigned short       jo_3d_kept_draw_mode(const jo_3d_polygon_attribute * const attribute, const bool textured)
{
    unsigned short          kept;

    kept = attribute->draw_mode & JO_VDP1_MESH;
    if ((attribute->draw_mode & 7) == JO_VDP1_COLOR_CALC_HALF_TRANSPARENT)
        kept |= JO_VDP1_COLOR_CALC_HALF_TRANSPARENT;
    if (textured)
        kept |= attribute->draw_mode & JO_VDP1_HIGH_SPEED_SHRINK;
    return (kept);
}

void                        jo_3d_set_mesh_polygon_transparency(jo_3d_mesh * const mesh, const bool enabled, const unsigned int index)
{
#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
    if (index >= mesh->data.polygon_count)
    {
        jo_core_error("index (%d) is too high (max=%d)", (int)index, (int)mesh->data.polygon_count);
        return ;
    }
#endif
    if (enabled)
        JO_ADD_FLAG(mesh->data.attributes[index].draw_mode, JO_VDP1_COLOR_CALC_HALF_TRANSPARENT);
    else
        JO_REMOVE_FLAG(mesh->data.attributes[index].draw_mode, JO_VDP1_COLOR_CALC_HALF_TRANSPARENT);
}


void                        jo_3d_set_mesh_transparency(jo_3d_mesh * const mesh, const bool enabled)
{
    unsigned int            i;

#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
#endif
    for (JO_ZERO(i); i < mesh->data.polygon_count; ++i)
        jo_3d_set_mesh_polygon_transparency(mesh, enabled, i);
}

void                        jo_3d_set_mesh_polygon_screen_doors(jo_3d_mesh * const mesh, const bool enabled, const unsigned int index)
{
#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
    if (index >= mesh->data.polygon_count)
    {
        jo_core_error("index (%d) is too high (max=%d)", (int)index, (int)mesh->data.polygon_count);
        return ;
    }
#endif
    if (enabled)
        JO_ADD_FLAG(mesh->data.attributes[index].draw_mode, JO_VDP1_MESH);
    else
        JO_REMOVE_FLAG(mesh->data.attributes[index].draw_mode, JO_VDP1_MESH);
}

void                        jo_3d_set_mesh_polygon_high_speed_shrink(jo_3d_mesh * const mesh, const bool enabled, const unsigned int index)
{
#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
    if (index >= mesh->data.polygon_count)
    {
        jo_core_error("index (%d) is too high (max=%d)", (int)index, (int)mesh->data.polygon_count);
        return ;
    }
#endif
    if (enabled)
        JO_ADD_FLAG(mesh->data.attributes[index].draw_mode, JO_VDP1_HIGH_SPEED_SHRINK);
    else
        JO_REMOVE_FLAG(mesh->data.attributes[index].draw_mode, JO_VDP1_HIGH_SPEED_SHRINK);
}

void                        jo_3d_set_mesh_high_speed_shrink(jo_3d_mesh * const mesh, const bool enabled)
{
    unsigned int            i;

#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
#endif
    for (JO_ZERO(i); i < mesh->data.polygon_count; ++i)
        jo_3d_set_mesh_polygon_high_speed_shrink(mesh, enabled, i);
}

void                        jo_3d_set_mesh_screen_doors(jo_3d_mesh * const mesh, const bool enabled)
{
    unsigned int            i;

#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
#endif
    for (JO_ZERO(i); i < mesh->data.polygon_count; ++i)
        jo_3d_set_mesh_polygon_screen_doors(mesh, enabled, i);
}

void                        jo_3d_set_mesh_polygon_light(jo_3d_mesh * const mesh, const bool enabled, const unsigned int index)
{
#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
    if (index >= mesh->data.polygon_count)
    {
        jo_core_error("index (%d) is too high (max=%d)", (int)index, (int)mesh->data.polygon_count);
        return ;
    }
#endif
    if (enabled)
        JO_ADD_FLAG(mesh->data.attributes[index].sort, JO_3D_USE_LIGHT);
    else
        JO_REMOVE_FLAG(mesh->data.attributes[index].sort, JO_3D_USE_LIGHT);
}

void                        jo_3d_set_mesh_light(jo_3d_mesh * const mesh, const bool enabled)
{
    unsigned int            i;

#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
#endif
    for (JO_ZERO(i); i < mesh->data.polygon_count; ++i)
        jo_3d_set_mesh_polygon_light(mesh, enabled, i);
}

void                        jo_3d_set_mesh_polygon_texture(jo_3d_mesh * const mesh, const int sprite_id, const unsigned int index)
{
    bool                    use_light;
    unsigned short          kept;

#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
    if (index >= mesh->data.polygon_count)
    {
        jo_core_error("index (%d) is too high (max=%d)", (int)index, (int)mesh->data.polygon_count);
        return ;
    }
#endif
    use_light = (mesh->data.attributes[index].sort & JO_3D_USE_LIGHT) != 0;
    kept = jo_3d_kept_draw_mode(&mesh->data.attributes[index], true);
    mesh->data.attributes[index].sort = JO_3D_SORT_CENTER | JO_VDP1_DIRECTION_OPTIONS(JO_VDP1_SPRITE_NO_FLIP) | JO_3D_NO_OPTION;
    if (use_light)
        JO_ADD_FLAG(mesh->data.attributes[index].sort, JO_3D_USE_LIGHT);
    mesh->data.attributes[index].texture_id = sprite_id;
    if (__jo_sprite_pic[sprite_id].color_mode == JO_VDP1_TEXTURE_SIZE_SHIFT_256_COLORS)
    {
        // (without the end codes, as the sprites: the color 255 is a color of the palette, see below)
        mesh->data.attributes[index].color = (unsigned short)__jo_sprite_attributes.color_table_index;
        mesh->data.attributes[index].draw_mode = (JO_VDP1_COLOR_MODE_256_BANK | JO_VDP1_NO_GOURAUD | JO_VDP1_END_CODE_DISABLE) |
                                                 JO_VDP1_DIRECTION_DRAW_MODE(JO_VDP1_SPRITE_NO_FLIP);
        JO_ADD_FLAG(mesh->data.attributes[index].sort, JO_3D_USE_PALETTE);
    }
    else if (__jo_sprite_pic[sprite_id].color_mode == JO_VDP1_TEXTURE_SIZE_SHIFT_16_COLORS)
    {
        // 16 colors and their color lookup table: RGB colors (gouraud, half transparency, light like an RGB texture). Without
        // the end codes, as the sprites (see jo_sprite_add_end_codes(): RGB only): the color 15 of the table is a color (the
        // JOM files use 15 colors), with the end codes 2 of them stopped the line, the rest of the line was not drawn
        mesh->data.attributes[index].color = __jo_sprite_lut[sprite_id];
        mesh->data.attributes[index].draw_mode = (JO_VDP1_COLOR_MODE_16_LOOKUP | JO_VDP1_NO_GOURAUD | JO_VDP1_END_CODE_DISABLE) |
                                                 JO_VDP1_DIRECTION_DRAW_MODE(JO_VDP1_SPRITE_NO_FLIP);
    }
    else
    {
        mesh->data.attributes[index].color = JO_VDP1_NO_PALETTE;
        mesh->data.attributes[index].draw_mode = (JO_VDP1_COLOR_MODE_RGB | JO_VDP1_NO_GOURAUD) | JO_VDP1_DIRECTION_DRAW_MODE(JO_VDP1_SPRITE_NO_FLIP);
    }
    JO_ADD_FLAG(mesh->data.attributes[index].draw_mode, kept);
    mesh->data.attributes[index].direction = JO_VDP1_DIRECTION_COMMAND(JO_VDP1_SPRITE_NO_FLIP);
}

void                        jo_3d_set_mesh_texture(jo_3d_mesh * const mesh, const int sprite_id)
{
    unsigned int            i;

#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
#endif
    for (JO_ZERO(i); i < mesh->data.polygon_count; ++i)
        jo_3d_set_mesh_polygon_texture(mesh, sprite_id, i);
}

void                        jo_3d_set_mesh_polygon_wireframe(jo_3d_mesh * const mesh, const unsigned int index, bool wireframe)
{
    // (the color of the polygon, its light, screen doors and half transparency kept, see jo_3d_set_mesh_polygon_color_ex())
    jo_3d_set_mesh_polygon_color_ex(mesh, mesh->data.attributes[index].color, index, wireframe);
}

void                        jo_3d_set_mesh_polygon_color_ex(jo_3d_mesh * const mesh, const jo_color color, const unsigned int index, bool wireframe)
{
    bool                    use_light;
    unsigned short          kept;

#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
    if (index >= mesh->data.polygon_count)
    {
        jo_core_error("index (%d) is too high (max=%d)", (int)index, (int)mesh->data.polygon_count);
        return ;
    }
#endif
    use_light = (mesh->data.attributes[index].sort & JO_3D_USE_LIGHT) != 0;
    kept = jo_3d_kept_draw_mode(&mesh->data.attributes[index], false);
    mesh->data.attributes[index].sort = JO_3D_SORT_CENTER | JO_VDP1_DIRECTION_OPTIONS(wireframe ? JO_VDP1_POLYLINE : JO_VDP1_POLYGON) | JO_3D_NO_OPTION;
    if (use_light)
        JO_ADD_FLAG(mesh->data.attributes[index].sort, JO_3D_USE_LIGHT);
    mesh->data.attributes[index].texture_id = JO_3D_NO_TEXTURE;
    mesh->data.attributes[index].draw_mode = (JO_VDP1_COLOR_MODE_RGB | JO_VDP1_NO_GOURAUD) | JO_VDP1_DIRECTION_DRAW_MODE(wireframe ? JO_VDP1_POLYLINE : JO_VDP1_POLYGON);
    JO_ADD_FLAG(mesh->data.attributes[index].draw_mode, kept);
    mesh->data.attributes[index].color = color;
    mesh->data.attributes[index].direction = JO_VDP1_DIRECTION_COMMAND(wireframe ? JO_VDP1_POLYLINE : JO_VDP1_POLYGON);
}

void                        jo_3d_set_mesh_color(jo_3d_mesh * const mesh, const jo_color color)
{
    jo_3d_set_mesh_color_ex(mesh, color, false);
}

void                        jo_3d_set_mesh_color_ex(jo_3d_mesh * const mesh, const jo_color color, bool wireframe)
{
    unsigned int            i;

#ifdef JO_DEBUG
    if (mesh == JO_NULL)
    {
        jo_core_error("mesh is null");
        return ;
    }
#endif
    for (JO_ZERO(i); i < mesh->data.polygon_count; ++i)
        jo_3d_set_mesh_polygon_color_ex(mesh, color, i, wireframe);
}

void                jo_3d_free_sprite_quad(int sprite_id)
{
#ifdef JO_DEBUG
    if (sprite_id < 0 || sprite_id >= JO_MAX_SPRITE)
    {
        jo_core_error("Invalid sprite_id (%d)", sprite_id);
        return ;
    }
    if (__jo_sprite_quad[sprite_id] == JO_NULL)
    {
        jo_core_error("No sprite quad for sprite_id %d", sprite_id);
        return ;
    }
#endif
    if (__jo_sprite_quad[sprite_id] == JO_NULL)
        return ;
    jo_free(__jo_sprite_quad[sprite_id]->data.points);
    jo_free(__jo_sprite_quad[sprite_id]);
    __jo_sprite_quad[sprite_id] = JO_NULL;
}

bool                jo_3d_create_sprite_quad(const int sprite_id)
{
    int             img_width;
    int             img_height;
    jo_vertice      *vertices;

    // (created already: kept, a second quad lost the first one)
    if (sprite_id < 0 || sprite_id >= JO_MAX_SPRITE)
        return (false);
    if (__jo_sprite_quad[sprite_id] != JO_NULL)
        return (true);
    if ((__jo_sprite_quad[sprite_id] = (jo_3d_quad *)jo_malloc_with_behaviour(sizeof(*__jo_sprite_quad[sprite_id]), JO_FAST_ALLOCATION)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory #1");
#endif
        return (false);
    }
    if ((vertices = (jo_vertice *)jo_malloc_with_behaviour(4 * sizeof(*vertices), JO_FAST_ALLOCATION)) == JO_NULL)
    {
        jo_free(__jo_sprite_quad[sprite_id]);
        __jo_sprite_quad[sprite_id] = JO_NULL;
#ifdef JO_DEBUG
        jo_core_error("Out of memory #2");
#endif
        return (false);
    }
    jo_3d_create_plane(__jo_sprite_quad[sprite_id], vertices);
    img_width = JO_DIV_BY_2(jo_sprite_get_width(sprite_id));
    img_height = JO_DIV_BY_2(jo_sprite_get_height(sprite_id));
    vertices[0].x = JO_MULT_BY_65536(-img_width);
    vertices[1].x = JO_MULT_BY_65536(img_width);
    vertices[2].x = JO_MULT_BY_65536(img_width);
    vertices[3].x = JO_MULT_BY_65536(-img_width);
    vertices[0].y = JO_MULT_BY_65536(-img_height);
    vertices[1].y = JO_MULT_BY_65536(-img_height);
    vertices[2].y = JO_MULT_BY_65536(img_height);
    vertices[3].y = JO_MULT_BY_65536(img_height);
    JO_ZERO(vertices[0].z);
    JO_ZERO(vertices[1].z);
    JO_ZERO(vertices[2].z);
    JO_ZERO(vertices[3].z);
    jo_3d_set_texture(__jo_sprite_quad[sprite_id], sprite_id);
    jo_3d_set_screen_doors(__jo_sprite_quad[sprite_id], false);
    jo_3d_set_light(__jo_sprite_quad[sprite_id], false);
    return (true);
}

int                                         jo_3d_mesh_get_bounding_sphere(const jo_3d_mesh * const mesh, jo_pos3D_fixed * const center)
{
    int                                     lo[3];
    int                                     hi[3];
    const jo_fixed                          *v;
    unsigned int                            j;
    int                                     k;

    if (mesh->data.point_count == 0)
    {
        center->x = 0;
        center->y = 0;
        center->z = 0;
        return (0);
    }
    for (k = 0; k < 3; ++k)
    {
        lo[k] = 0x7fffffff;
        hi[k] = -0x7fffffff;
    }
    for (j = 0; j < mesh->data.point_count; ++j)
    {
        v = (const jo_fixed *)&mesh->data.points[j];
        for (k = 0; k < 3; ++k)
        {
            lo[k] = JO_MIN(lo[k], jo_fixed2int(v[k]));
            hi[k] = JO_MAX(hi[k], jo_fixed2int(v[k]));
        }
    }
    center->x = jo_int2fixed((lo[0] + hi[0]) / 2);
    center->y = jo_int2fixed((lo[1] + hi[1]) / 2);
    center->z = jo_int2fixed((lo[2] + hi[2]) / 2);
    // (the half sizes: 3 x 32767^2 fits in 32 bits, the whole sizes overflowed for the meshes bigger than about 37000)
    // (unsigned: the sum of the 3 squares goes over 2^31)
    return (jo_sqrt((unsigned int)JO_SQUARE((hi[0] - lo[0] + 1) / 2) + (unsigned int)JO_SQUARE((hi[1] - lo[1] + 1) / 2) +
                    (unsigned int)JO_SQUARE((hi[2] - lo[2] + 1) / 2)) + 2);
}

/** @brief The box of a ray segment (whole units, a margin of 1)
 *  @param o Origin
 *  @param d Direction (unit vector)
 *  @param length Length (Q16.16)
 *  @param box x, y, z min, then max
 */
static  __jo_force_inline void              jo_3d_ray_segment_box(const jo_pos3D_fixed * const o, const jo_pos3D_fixed * const d,
                                                                  const jo_fixed length, int * const box)
{
    int                                     e[3];

    // (the end in whole units, 64 bits: a long ray (the length of a ray without limit, JO_FIXED_MAX) went over 32767 units
    // and wrapped, the box was on the other side of the origin and the polygons in front of it were not hit. Once per hit)
    e[0] = (int)((((long long)o->x << 16) + (long long)d->x * length) >> 32);
    e[1] = (int)((((long long)o->y << 16) + (long long)d->y * length) >> 32);
    e[2] = (int)((((long long)o->z << 16) + (long long)d->z * length) >> 32);
    box[0] = JO_MIN(jo_fixed2int(o->x), e[0]) - 1;
    box[1] = JO_MIN(jo_fixed2int(o->y), e[1]) - 1;
    box[2] = JO_MIN(jo_fixed2int(o->z), e[2]) - 1;
    box[3] = JO_MAX(jo_fixed2int(o->x), e[0]) + 1;
    box[4] = JO_MAX(jo_fixed2int(o->y), e[1]) + 1;
    box[5] = JO_MAX(jo_fixed2int(o->z), e[2]) + 1;
}

/** @brief Polygons per group of the planes for the rays (a box for each group, see jo_3d_mesh_create_ray_planes()), a
 *  power of 2 */
# define JO_3D_RAY_PLANE_GROUP              (8)
# define JO_3D_RAY_PLANE_GROUPS(COUNT)      (((COUNT) + JO_3D_RAY_PLANE_GROUP - 1) / JO_3D_RAY_PLANE_GROUP)
/** @brief The box of a group (whole units: x, y, z min, then max), after the planes of the polygons */
typedef struct
{
    short                                   box[6];
}                                           jo_3d_ray_group;
# define JO_3D_RAY_GROUPS(PLANES, COUNT)    ((const jo_3d_ray_group *)((PLANES) + (COUNT)))
/** @brief Polygons per big group (a box for each, after the boxes of the groups): a long ray across a room skips most of a
 *  big mesh with them (the loop on the groups of 8 polygons cost about 0.1 ms for a mesh of 3000 polygons) */
# define JO_3D_RAY_PLANE_BIG_GROUP          (64)
# define JO_3D_RAY_PLANE_BIG_GROUPS(COUNT)  (((COUNT) + JO_3D_RAY_PLANE_BIG_GROUP - 1) / JO_3D_RAY_PLANE_BIG_GROUP)
# define JO_3D_RAY_BIG_GROUPS(PLANES, COUNT) (JO_3D_RAY_GROUPS(PLANES, COUNT) + JO_3D_RAY_PLANE_GROUPS(COUNT))

/** @brief What a box must meet to be tested by a ray (see jo_3d_ray_next_polygon()): the box of the segment, the line seen
 *  from above (its normal Q12, its offset, the byte offsets of the corners of a box farthest on each side of it) */
typedef struct
{
    int                                     segment[6];
    int                                     lx;
    int                                     lz;
    int                                     line_o;
    int                                     hi_x;
    int                                     hi_z;
    int                                     lo_x;
    int                                     lo_z;
}                                           jo_3d_ray_filter;

/** @brief A box (whole units: x, y, z min, then max) can't hold a hit of the ray: out of the box of the segment, or all on one
 *  side of the line of the ray seen from above, farther than 8 units (the point of a hit is a few units at most from the line) */
static  __jo_force_inline bool              jo_3d_ray_box_out(const short * const box, const jo_3d_ray_filter * const f)
{
    return (f->segment[3] < box[0] || f->segment[4] < box[1] || f->segment[5] < box[2] ||
            f->segment[0] > box[3] || f->segment[1] > box[4] || f->segment[2] > box[5] ||
            f->lx * *(const short *)((const char *)box + f->hi_x) + f->lz * *(const short *)((const char *)box + f->hi_z) - f->line_o < -8 * 4096 ||
            f->lx * *(const short *)((const char *)box + f->lo_x) + f->lz * *(const short *)((const char *)box + f->lo_z) - f->line_o > 8 * 4096);
}

/** @brief The next polygon whose box can hold a hit of the ray (the boxes of the big groups, of the groups, then of the
 *  polygons: the most polygons stop here. A small function: its values stay in the registers)
 *  @param planes The planes of the polygons of the mesh (then the boxes of the groups and of the big groups)
 *  @param k First polygon
 *  @param count Polygons of the mesh
 *  @param f Filter
 *  @return The polygon, count: none
 */
static unsigned int __attribute__((noinline)) jo_3d_ray_next_polygon(const jo_3d_ray_plane * const planes, unsigned int k,
                                                                    const unsigned int count, const jo_3d_ray_filter * const f)
{
    const jo_3d_ray_group                   *groups;
    const jo_3d_ray_group                   *big;
    const jo_3d_ray_plane                   *q;
    const jo_3d_ray_plane                   *q_end;
    unsigned int                            end;

    groups = JO_3D_RAY_GROUPS(planes, count);
    big = JO_3D_RAY_BIG_GROUPS(planes, count);
    while (k < count)
    {
        if ((k & (JO_3D_RAY_PLANE_BIG_GROUP - 1)) == 0 && jo_3d_ray_box_out(big[k / JO_3D_RAY_PLANE_BIG_GROUP].box, f))
        {
            k += JO_3D_RAY_PLANE_BIG_GROUP;
            continue;
        }
        // (the polygons of a group are near each other: the order of the model)
        if ((k & (JO_3D_RAY_PLANE_GROUP - 1)) == 0 && jo_3d_ray_box_out(groups[k / JO_3D_RAY_PLANE_GROUP].box, f))
        {
            k += JO_3D_RAY_PLANE_GROUP;
            continue;
        }
        // The polygons to the end of the group (a pointer, not an index: the loop stays in the registers)
        end = JO_MIN((k | (JO_3D_RAY_PLANE_GROUP - 1)) + 1, count);
        for (q = planes + k, q_end = planes + end; q < q_end; ++q)
            if (!jo_3d_ray_box_out(q->box, f))
                return ((unsigned int)(q - planes));
        k = end;
    }
    return (count);
}

/** @brief First polygon of a mesh hit by a ray (see jo_3d_mesh_ray_hit()), with the planes of its polygons or without them
 *  @param planes The planes of the polygons (see jo_3d_mesh_create_ray_planes()), used if with_planes */
static  __jo_force_inline bool              jo_3d_mesh_ray_hit_in(const jo_3d_mesh * const mesh, const jo_3d_ray_plane * const planes,
                                                                  const bool with_planes, const bool with_line, const jo_pos3D_fixed * const o,
                                                                  const jo_pos3D_fixed * const d, jo_fixed * const best,
                                                                  jo_pos3D_fixed * const normal)
{
    const jo_3d_polygon                     *p;
    const jo_pos3D_fixed                    *points;
    const jo_pos3D_fixed                    *a;
    const jo_pos3D_fixed                    *b;
    int                                     ox;
    int                                     oy;
    int                                     oz;
    int                                     dx;
    int                                     dy;
    int                                     dz;
    int                                     nx;
    int                                     ny;
    int                                     nz;
    int                                     qx;
    int                                     qy;
    int                                     qz;
    int                                     ex;
    int                                     ey;
    int                                     ez;
    int                                     wx;
    int                                     wy;
    int                                     wz;
    int                                     den;
    int                                     num;
    int                                     side;
    int                                     limit;
    jo_fixed                                t;
    unsigned int                            k;
    int                                     j;
    int                                     sign;
    bool                                    hit;
    jo_3d_ray_filter                        f;
    unsigned int                            count;

    hit = false;
    points = mesh->data.points;
    ox = jo_fixed2int(o->x);
    oy = jo_fixed2int(o->y);
    oz = jo_fixed2int(o->z);
    dx = d->x >> 2;
    dy = d->y >> 2;
    dz = d->z >> 2;
    // (with the planes: the box of the segment, smaller at each hit, see jo_3d_ray_segment_box())
    if (with_planes)
        jo_3d_ray_segment_box(o, d, *best, f.segment);
    // (with the planes: the line of the ray seen from above, its normal (lx, lz) Q12: a box all on one side of it, farther
    // than 8 units, is skipped (a long ray across a room: its box meets most groups). The point of a hit is a few units at
    // most from the line (rounded): the same result. The corners of a box farthest on each side (byte offsets). Without
    // the line: a normal 0, every box meets it)
    if (with_line)
    {
        f.lx = -(d->z >> 4);
        f.lz = d->x >> 4;
    }
    else
    {
        JO_ZERO(f.lx);
        JO_ZERO(f.lz);
    }
    f.line_o = f.lx * ox + f.lz * oz;
    f.hi_x = f.lx > 0 ? 3 * 2 : 0;
    f.hi_z = f.lz > 0 ? 5 * 2 : 2 * 2;
    f.lo_x = 3 * 2 - f.hi_x;
    f.lo_z = 7 * 2 - f.hi_z;
    count = mesh->data.polygon_count;
    for (k = 0, p = mesh->data.polygons; k < count; ++k, ++p)
    {
        // (with the planes: the boxes of the big groups, of the groups and of the polygons that don't meet the segment, see
        // jo_3d_ray_next_polygon(). The same result: the boxes of the polygons are in the boxes of their groups)
        if (with_planes)
        {
            k = jo_3d_ray_next_polygon(planes, k, count, &f);
            if (k >= count)
                break;
            p = mesh->data.polygons + k;
        }
        // Plane of the polygon: t = n.(a - o) / n.d. Normal and direction Q14. Only the front faces (the walls have a
        // polygon on each side: the inside and the outside)
        if (with_planes)
        {
            nx = planes[k].nx;
            ny = planes[k].ny;
            nz = planes[k].nz;
        }
        else
        {
            nx = p->normal.x >> 2;
            ny = p->normal.y >> 2;
            nz = p->normal.z >> 2;
        }
        // ((sum >> 14) > -16, without the shift: a shift of 14 bits is a call on the SH-2, most polygons stop here)
        den = nx * dx + ny * dy + nz * dz;
        if (den >= -16 * 16384)
            continue;
        // (den < 0: its arithmetic shift is the complement of the logical shift of its complement, without the call of the
        // library of the compiler for a shift of 14 bits)
        den = (int)~((unsigned int)~den >> 14);
        // (with the planes: n.a - n.o, the same 32 bits result)
        if (with_planes)
            num = planes[k].d - (nx * ox + ny * oy + nz * oz);
        else
        {
            a = &points[p->vertices[0]];
            num = nx * (jo_fixed2int(a->x) - ox) + ny * (jo_fixed2int(a->y) - oy) + nz * (jo_fixed2int(a->z) - oz);
        }
        // 0 < t < best (world units: num / den), the division only for the polygons in front
        if ((num > 0) != (den > 0) || num == 0)
            continue;
        // (the whole units of best, plus one: a quick test before the division; then t < best exactly. With the whole
        // units only, a polygon less than 1 unit in front of the hit found before was not taken: the hit depended on the
        // order of the polygons)
        limit = (jo_fixed2int(*best) + 1) * (den > 0 ? den : -den);
        if ((num > 0 ? num : -num) >= limit)
            continue;
        t = jo_fixed_div(num, den);
        if (t >= *best)
            continue;
        qx = ox + jo_fixed2int(jo_fixed_mult(d->x, t));
        qy = oy + jo_fixed2int(jo_fixed_mult(d->y, t));
        qz = oz + jo_fixed2int(jo_fixed_mult(d->z, t));
        // (with the planes: out of the box of the polygon, out of the polygon. Most polygons of a wall are in the same plane)
        if (with_planes && (qx < planes[k].box[0] || qy < planes[k].box[1] || qz < planes[k].box[2] || qx > planes[k].box[3] ||
                            qy > planes[k].box[4] || qz > planes[k].box[5]))
            continue;
        // Inside of the quad (convex): the point is on the same side of the 4 edges (an edge of length 0 for a triangle).
        // (e x w).n: e x w / 16 (edges of 4000 units at most), normal Q8
        sign = 0;
        for (j = 0; j < 4; ++j)
        {
            a = &points[p->vertices[j]];
            b = &points[p->vertices[(j + 1) & 3]];
            ex = jo_fixed2int(b->x - a->x);
            ey = jo_fixed2int(b->y - a->y);
            ez = jo_fixed2int(b->z - a->z);
            wx = qx - jo_fixed2int(a->x);
            wy = qy - jo_fixed2int(a->y);
            wz = qz - jo_fixed2int(a->z);
            side = ((ey * wz - ez * wy) >> 4) * (nx >> 6) + ((ez * wx - ex * wz) >> 4) * (ny >> 6) + ((ex * wy - ey * wx) >> 4) * (nz >> 6);
            if (side == 0)
                continue;
            if (sign == 0)
                sign = side > 0 ? 1 : -1;
            else if ((side > 0 ? 1 : -1) != sign)
                break;
        }
        if (j < 4)
            continue;
        *best = t;
        *normal = p->normal;
        hit = true;
        if (with_planes)
            jo_3d_ray_segment_box(o, d, t, f.segment);
    }
    return (hit);
}

bool                                        jo_3d_mesh_ray_hit(const jo_3d_mesh * const mesh, const jo_pos3D_fixed * const o, const jo_pos3D_fixed * const d,
                                                               jo_fixed * const best, jo_pos3D_fixed * const normal)
{
    return (jo_3d_mesh_ray_hit_in(mesh, JO_NULL, false, false, o, d, best, normal));
}

#ifdef JO_TEST_RAY_COMPARE
int                                         __jo_test_ray_count = 0;
int                                         __jo_test_ray_mismatch = 0;
#endif

bool                                        jo_3d_mesh_ray_hit_with_planes(const jo_3d_mesh * const mesh, const jo_3d_ray_plane * const planes,
                                                                           const jo_pos3D_fixed * const o, const jo_pos3D_fixed * const d,
                                                                           jo_fixed * const best, jo_pos3D_fixed * const normal)
{
    if (planes == JO_NULL)
        return (jo_3d_mesh_ray_hit_in(mesh, JO_NULL, false, false, o, d, best, normal));
#ifdef JO_TEST_RAY_COMPARE
    // Test (not for release): the same result without the test of the line of the ray (see jo_3d_mesh_ray_hit_in())
    {
        jo_fixed                            best2;
        jo_pos3D_fixed                      normal2;
        bool                                hit2;
        bool                                hit;

        best2 = *best;
        normal2 = *normal;
        hit2 = jo_3d_mesh_ray_hit_in(mesh, planes, true, false, o, d, &best2, &normal2);
        hit = jo_3d_mesh_ray_hit_in(mesh, planes, true, true, o, d, best, normal);
        ++__jo_test_ray_count;
        if (hit != hit2 || *best != best2 || normal->x != normal2.x || normal->y != normal2.y || normal->z != normal2.z)
            ++__jo_test_ray_mismatch;
        return (hit);
    }
#else
    return (jo_3d_mesh_ray_hit_in(mesh, planes, true, true, o, d, best, normal));
#endif
}

jo_3d_ray_plane                             *jo_3d_mesh_create_ray_planes(const jo_3d_mesh * const mesh)
{
    jo_3d_ray_plane                         *planes;
    jo_3d_ray_group                         *group;
    const jo_3d_polygon                     *p;
    const jo_pos3D_fixed                    *a;
    const jo_fixed                          *v;
    unsigned int                            k;
    int                                     i;
    int                                     j;

    if (mesh->data.polygon_count == 0)
        return (JO_NULL);
    // (after the planes: the boxes of the groups of JO_3D_RAY_PLANE_GROUP polygons, then of JO_3D_RAY_PLANE_BIG_GROUP polygons,
    // see jo_3d_mesh_ray_hit_in())
    planes = (jo_3d_ray_plane *)jo_malloc(mesh->data.polygon_count * sizeof(jo_3d_ray_plane) +
                                          (JO_3D_RAY_PLANE_GROUPS(mesh->data.polygon_count) +
                                           JO_3D_RAY_PLANE_BIG_GROUPS(mesh->data.polygon_count)) * sizeof(jo_3d_ray_group));
    if (planes == JO_NULL)
        return (JO_NULL);
    for (k = 0, p = mesh->data.polygons; k < mesh->data.polygon_count; ++k, ++p)
    {
        // (as in jo_3d_mesh_ray_hit_in(): the normal Q14, n.a with the whole units of a vertex)
        planes[k].nx = (short)(p->normal.x >> 2);
        planes[k].ny = (short)(p->normal.y >> 2);
        planes[k].nz = (short)(p->normal.z >> 2);
        planes[k].unused = 0;
        a = &mesh->data.points[p->vertices[0]];
        planes[k].d = planes[k].nx * jo_fixed2int(a->x) + planes[k].ny * jo_fixed2int(a->y) + planes[k].nz * jo_fixed2int(a->z);
        for (j = 0; j < 3; ++j)
        {
            planes[k].box[j] = 0x7fff;
            planes[k].box[j + 3] = -0x7fff;
        }
        for (i = 0; i < 4; ++i)
        {
            v = (const jo_fixed *)&mesh->data.points[p->vertices[i]];
            for (j = 0; j < 3; ++j)
            {
                planes[k].box[j] = (short)JO_MAX(JO_MIN(planes[k].box[j], jo_fixed2int(v[j]) - 1), -0x7fff);
                planes[k].box[j + 3] = (short)JO_MIN(JO_MAX(planes[k].box[j + 3], jo_fixed2int(v[j]) + 1), 0x7fff);
            }
        }
    }
    // The box of each group: the boxes of its polygons together
    for (k = 0; k < JO_3D_RAY_PLANE_GROUPS(mesh->data.polygon_count); ++k)
    {
        group = (jo_3d_ray_group *)&JO_3D_RAY_GROUPS(planes, mesh->data.polygon_count)[k];
        for (j = 0; j < 3; ++j)
        {
            group->box[j] = 0x7fff;
            group->box[j + 3] = -0x7fff;
        }
        for (i = (int)k * JO_3D_RAY_PLANE_GROUP; i < (int)mesh->data.polygon_count && i < (int)(k + 1) * JO_3D_RAY_PLANE_GROUP; ++i)
            for (j = 0; j < 3; ++j)
            {
                group->box[j] = JO_MIN(group->box[j], planes[i].box[j]);
                group->box[j + 3] = JO_MAX(group->box[j + 3], planes[i].box[j + 3]);
            }
    }
    // The box of each big group: the boxes of its groups together
    for (k = 0; k < JO_3D_RAY_PLANE_BIG_GROUPS(mesh->data.polygon_count); ++k)
    {
        group = (jo_3d_ray_group *)&JO_3D_RAY_BIG_GROUPS(planes, mesh->data.polygon_count)[k];
        for (j = 0; j < 3; ++j)
        {
            group->box[j] = 0x7fff;
            group->box[j + 3] = -0x7fff;
        }
        for (i = (int)k * (JO_3D_RAY_PLANE_BIG_GROUP / JO_3D_RAY_PLANE_GROUP);
             i < (int)JO_3D_RAY_PLANE_GROUPS(mesh->data.polygon_count) && i < (int)(k + 1) * (JO_3D_RAY_PLANE_BIG_GROUP / JO_3D_RAY_PLANE_GROUP); ++i)
            for (j = 0; j < 3; ++j)
            {
                group->box[j] = JO_MIN(group->box[j], JO_3D_RAY_GROUPS(planes, mesh->data.polygon_count)[i].box[j]);
                group->box[j + 3] = JO_MAX(group->box[j + 3], JO_3D_RAY_GROUPS(planes, mesh->data.polygon_count)[i].box[j + 3]);
            }
    }
    return (planes);
}

/** @brief The rectangle of jo_3d_set_view_rect() (see jo_3d_get_view_rect()) */
static jo_3d_rect                           __jo_3d_view_rect_value;

bool                                        jo_3d_get_view_rect(jo_3d_rect * const rect)
{
    if (!__jo_3d_view_rect.enabled)
        return (false);
    *rect = __jo_3d_view_rect_value;
    return (true);
}

void                                        jo_3d_set_view_rect(const jo_3d_rect * const rect)
{
    int                                     k;
    int                                     edges[4];

    __jo_3d_view_rect.enabled = rect != JO_NULL;
    if (rect == JO_NULL)
        return;
    __jo_3d_view_rect_value = *rect;
    __jo_3d_view_rect.empty = rect->left > rect->right || rect->top > rect->bottom;
    edges[0] = rect->left - __jo_screen.width_2;
    edges[1] = rect->right + 1 - __jo_screen.width_2;
    edges[2] = rect->top - __jo_screen.height_2;
    edges[3] = rect->bottom + 1 - __jo_screen.height_2;
    for (k = 0; k < 4; ++k)
    {
        __jo_3d_view_rect.k[k] = jo_fixed_div(jo_int2fixed(edges[k]), __jo_vdp1_projection_distance);
        __jo_3d_view_rect.margin[k] = JO_FIXED_1 + jo_fixed_mult(__jo_3d_view_rect.k[k], __jo_3d_view_rect.k[k]) / 2;
    }
}

/** @brief Check if a sphere is outside of the rectangle of jo_3d_set_view_rect()
 *  @param r The rectangle (of the master CPU, or its copy for the slave CPU)
 *  @param center Center (view space)
 *  @param radius Radius
 */
static bool                                 jo_3d_is_sphere_out_of_view_rect(const jo_3d_view_rect_planes * const r, const jo_pos3D_fixed * const center,
                                                                             const jo_fixed radius)
{
    // The camera in the sphere (the building of the camera): seen
    if (center->z - radius < JO_FIXED_8)
        return (false);
    if (r->empty)
        return (true);
    // The 4 planes through the camera and the edges of the rectangle: x = k * z (y = k * z)
    if (center->x - jo_fixed_mult(center->z, r->k[0]) < -jo_fixed_mult(radius, r->margin[0]) ||
        center->x - jo_fixed_mult(center->z, r->k[1]) > jo_fixed_mult(radius, r->margin[1]) ||
        center->y - jo_fixed_mult(center->z, r->k[2]) < -jo_fixed_mult(radius, r->margin[2]) ||
        center->y - jo_fixed_mult(center->z, r->k[3]) > jo_fixed_mult(radius, r->margin[3]))
        return (true);
    return (false);
}

/** @brief Check if a sphere can be seen (see jo_3d_is_sphere_visible())
 *  @param r The rectangle of jo_3d_set_view_rect() (of the master CPU, or its copy for the slave CPU)
 */
static bool                                 jo_3d_is_sphere_visible_in(const jo_3d_view_rect_planes * const r, jo_3d_screen_planes * const s,
                                                                       const jo_pos3D_fixed * const center, const jo_fixed radius,
                                                                       const jo_fixed max_distance)
{
    if (center->z + radius < JO_FIXED_8 || center->z - radius > max_distance)
        return (false);
    // Side planes |x| = kx * z: the distance to a plane is (|x| - kx * z) / sqrt(1 + kx * kx), and sqrt(1 + a) <= 1 + a / 2
    if (s->projection_distance != __jo_vdp1_projection_distance || s->height_2 != __jo_screen.height_2)
    {
        s->projection_distance = __jo_vdp1_projection_distance;
        s->height_2 = __jo_screen.height_2;
        s->kx = jo_fixed_div(jo_int2fixed(__jo_screen.width_2), s->projection_distance);
        s->ky = jo_fixed_div(jo_int2fixed(__jo_screen.height_2), s->projection_distance);
        s->margin_x = JO_FIXED_1 + jo_fixed_mult(s->kx, s->kx) / 2;
        s->margin_y = JO_FIXED_1 + jo_fixed_mult(s->ky, s->ky) / 2;
    }
    if (JO_ABS(center->x) - jo_fixed_mult(center->z, s->kx) > jo_fixed_mult(radius, s->margin_x))
        return (false);
    // Top and bottom planes |y| = ky * z
    if (JO_ABS(center->y) - jo_fixed_mult(center->z, s->ky) > jo_fixed_mult(radius, s->margin_y))
        return (false);
    // The rectangle where the objects can be seen (the door of the building of the camera)
    if (r->enabled && jo_3d_is_sphere_out_of_view_rect(r, center, radius))
        return (false);
    return (true);
}

bool                                        jo_3d_is_sphere_visible(const jo_pos3D_fixed * const center, const jo_fixed radius, const jo_fixed max_distance)
{
    return (jo_3d_is_sphere_visible_in(&__jo_3d_view_rect, &__jo_3d_screen_planes, center, radius, max_distance));
}

bool                                        jo_3d_slave_is_sphere_visible(const jo_pos3D_fixed * const center, const jo_fixed radius, const jo_fixed max_distance)
{
    jo_3d_view_rect_planes                  r;

    // (written by the master CPU: read without the cache of the slave CPU, it may keep the copy of a previous frame; the
    // whole rectangle only when it is enabled, it is not most of the time)
    r.enabled = ((const volatile jo_3d_view_rect_planes *)JO_CACHE_THROUGH(&__jo_3d_slave_view_rect))->enabled;
    if (r.enabled)
        r = *(const volatile jo_3d_view_rect_planes *)JO_CACHE_THROUGH(&__jo_3d_slave_view_rect);
    return (jo_3d_is_sphere_visible_in(&r, &__jo_3d_slave_screen_planes, center, radius, max_distance));
}

bool                                        jo_3d_draw_standing_sprite(const int sprite_id, const jo_pos3D_fixed * const foot, const jo_fixed width,
                                                                       const jo_fixed height, const jo_fixed sway, const bool flip)
{
    jo_pos3D_fixed                          v;
    jo_pos2D_fixed                          points[4];
    jo_vdp1_sprite_attribute                attr;
    jo_fixed                                k;
    int                                     w;
    int                                     h;

    // (as the slave CPU version: jo_3d_mipmap() reads the tables of the sprite)
    if (sprite_id < 0 || sprite_id >= JO_MAX_SPRITE)
        return (false);
    if (!jo_3d_standing_sprite_foot(JO_3D_CURRENT_MATRIX, foot, &v, &k) ||
        !jo_3d_standing_sprite_points(&v, k, width, height, sway, flip, points, &w, &h))
        return (false);
    // Far away: a smaller copy of the texture (the command of jo_sprite_draw_4p_fixed(), its colors in their bank when it fades)
    __jo_sprite_attributes_of(&attr, jo_3d_mipmap(sprite_id, w, h), &__jo_sprite_attributes);
    jo_3d_fade_attribute(&attr, &__jo_3d_fade, sprite_id, v.z);
    __jo_vdp1_draw_sprite_4p(points, v.z, &attr);
    return (true);
}

jo_3d_mesh                                  *jo_3d_create_indexed_mesh(const unsigned int point_count, const unsigned int polygon_count)
{
    jo_3d_mesh                              *mesh;

    mesh = (jo_3d_mesh *)jo_malloc(sizeof(*mesh));
    if (mesh == JO_NULL)
        return (JO_NULL);
    jo_memset(mesh, 0, sizeof(*mesh));
    mesh->data.point_count = point_count;
    mesh->data.points = (jo_pos3D_fixed *)jo_malloc(point_count * sizeof(*mesh->data.points));
    mesh->data.polygon_count = polygon_count;
    mesh->data.polygons = (jo_3d_polygon *)jo_malloc(polygon_count * sizeof(*mesh->data.polygons));
    mesh->data.attributes = (jo_3d_polygon_attribute *)jo_malloc(polygon_count * sizeof(*mesh->data.attributes));
    if (mesh->data.points == JO_NULL || mesh->data.polygons == JO_NULL || mesh->data.attributes == JO_NULL)
    {
        // (only the blocks that were allocated: jo_free(JO_NULL) would corrupt the heap without JO_DEBUG)
        if (mesh->data.points != JO_NULL)
            jo_free(mesh->data.points);
        if (mesh->data.polygons != JO_NULL)
            jo_free(mesh->data.polygons);
        if (mesh->data.attributes != JO_NULL)
            jo_free(mesh->data.attributes);
        jo_free(mesh);
        return (JO_NULL);
    }
    jo_memset(mesh->data.points, 0, point_count * sizeof(*mesh->data.points));
    jo_memset(mesh->data.polygons, 0, polygon_count * sizeof(*mesh->data.polygons));
    jo_memset(mesh->data.attributes, 0, polygon_count * sizeof(*mesh->data.attributes));
    return (mesh);
}

/** @brief The pool of the small sets of gouraud tables: one sprite of JO_3D_GOURAUD_POOL_LINES lines shared by the
 *  models (a sprite per model used up the sprites: 1000 small models, 1600 sprites) */
#define JO_3D_GOURAUD_POOL_LINES                (256)
static int                                  __jo_3d_gouraud_pool_sprite = -1;
static unsigned int                         __jo_3d_gouraud_pool_used = 0;

void                                        __jo_3d_gouraud_pool_freed(const int sprite_id)
{
    if (sprite_id <= __jo_3d_gouraud_pool_sprite)
        __jo_3d_gouraud_pool_sprite = -1;
}

void                                        jo_3d_free_gouraud_tables(const unsigned short adr, const unsigned int count)
{
    unsigned int                            lines;
    unsigned int                            base;
    int                                     id;

    if (adr == 0 || count == 0)
        return;
    lines = (count + 1) / 2;
    // In the shared sprite: the last set given back (the sets of a level freed in the reverse order of their loading)
    if (__jo_3d_gouraud_pool_sprite >= 0 && lines <= JO_3D_GOURAUD_POOL_LINES / 4)
    {
        base = __jo_sprite_def[__jo_3d_gouraud_pool_sprite].adr;
        if (adr >= base && adr < base + JO_3D_GOURAUD_POOL_LINES * 2)
        {
            if (adr + lines * 2 == base + __jo_3d_gouraud_pool_used * 2)
                __jo_3d_gouraud_pool_used -= lines;
            return;
        }
    }
    // Its own sprite: given back when it is the last one
    id = jo_get_last_sprite_id();
    if (id > 0 && id != __jo_3d_gouraud_pool_sprite && __jo_sprite_def[id].adr == adr)
        jo_sprite_free_from(id);
}

unsigned short                              jo_3d_add_gouraud_tables(const jo_color * const tables, const unsigned int count)
{
    jo_img                                  img;
    jo_color                                *zero;
    int                                     id;
    unsigned int                            lines;
    unsigned short                          adr;

    // A sprite of 8 pixels per line holds 2 tables per line
    lines = (count + 1) / 2;
    if (tables != JO_NULL && lines <= JO_3D_GOURAUD_POOL_LINES / 4)
    {
        // A small set: in the shared sprite (a new one when it is full)
        if (__jo_3d_gouraud_pool_sprite < 0 || __jo_3d_gouraud_pool_used + lines > JO_3D_GOURAUD_POOL_LINES)
        {
            if (jo_3d_add_gouraud_tables(JO_NULL, JO_3D_GOURAUD_POOL_LINES * 2) == 0)
                return (0);
            __jo_3d_gouraud_pool_sprite = jo_get_last_sprite_id();
            __jo_3d_gouraud_pool_used = 0;
        }
        adr = (unsigned short)(__jo_sprite_def[__jo_3d_gouraud_pool_sprite].adr + __jo_3d_gouraud_pool_used * 2);
        jo_memcpy((void *)(JO_VDP1_VRAM + JO_MULT_BY_8(adr)), tables, count * 4 * sizeof(jo_color));
        __jo_3d_gouraud_pool_used += lines;
        return (adr);
    }
    zero = JO_NULL;
    // (no tables, or an odd count: a copy with a full last line, the sprite reads 8 colors per line, 4 after the tables)
    if (tables == JO_NULL || (count & 1))
    {
        zero = (jo_color *)jo_malloc(((count + 1) / 2) * 8 * sizeof(jo_color));
        if (zero == JO_NULL)
            return (0);
        jo_memset(zero, 0, ((count + 1) / 2) * 8 * sizeof(jo_color));
        if (tables != JO_NULL)
            jo_memcpy(zero, tables, count * 4 * sizeof(jo_color));
    }
    img.width = 8;
    img.height = lines;
    img.data = zero != JO_NULL ? zero : (jo_color *)tables;
    id = jo_sprite_add(&img);
    if (zero != JO_NULL)
        jo_free(zero);
    if (id < 0)
        return (0);
    return (__jo_sprite_def[id].adr);
}

#endif /* !JO_COMPILE_WITH_3D_SUPPORT */

/*
** END OF FILE
*/
