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
** Cells and portals: see jo/cell.h
*/

#include <stdbool.h>
#include "jo/conf.h"

#if defined(JO_COMPILE_WITH_LEVEL_SUPPORT) && defined(JO_COMPILE_WITH_3D_SUPPORT)

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
#include "jo/image.h"
#include "jo/tga.h"
#include "jo/3d.h"
#include "jo/model.h"
#include "jo/light.h"
#include "jo/level.h"
#include "jo/cell.h"

/** @brief The reflected room in a mirror is not drawn farther (world units) */
#define JO_CELL_MIRROR_FAR                  (4000)
/** @brief Depth added to the reflected room: drawn before all the other polygons (sorted among themselves). The view
 *  covers it except through the mirror: a hole in a wall, or a half transparent floor (a polished floor) */
#define JO_CELL_MIRROR_DEPTH_BIAS           (16384)
/** @brief A portal nearer than this (world units, Q16.16) is not projected: the rectangle of the cell before it is kept */
#define JO_CELL_PORTAL_NEAR                 (JO_FIXED_8)
/** @brief The cells farther are not drawn (world units, Q16.16) */
#define JO_CELL_FAR                         (jo_int2fixed(8000))
/** @brief The camera is in a doorway when it is nearer than this to the plane of the portal (world units): the cell on the
 *  other side is seen in the whole rectangle of the cell before it (the portal is cut by the near plane) */
#define JO_CELL_DOORWAY                     (16)
/** @brief Depth added to the cells seen through a portal for each portal (see jo_cells_draw_prepared()): the walls of a cell
 *  hide the cells behind them (the VDP1 doesn't clip a cell to the rectangle of its portal: the painter's sort could draw
 *  the walls of the next room over the walls of the room of the camera) */
#define JO_CELL_LEVEL_BIAS                  (2048)

/** @brief A cell to draw and the rectangle of the screen where it is seen */
typedef struct
{
    short                           cell;
    short                           level;          // Portals from the cell of the camera
    short                           left;
    short                           top;
    short                           right;
    short                           bottom;
}                                   __jo_cell_view;

static jo_cell                      __jo_cells[JO_CELL_MAX];
static jo_3d_model                  *__jo_cell_models[JO_CELL_MAX];
/** @brief Box of the model of each cell (world units: x, y, z min, then max): the rays that don't go through it are not tested
 *  against its polygons (see jo_cells_ray_hit()) */
static short                        __jo_cell_box[JO_CELL_MAX][6];
/** @brief The planes of the polygons of the model of each cell (the rays are faster), JO_NULL: none */
static jo_3d_ray_plane              *__jo_cell_ray_planes[JO_CELL_MAX];
static jo_3d_model                  *__jo_cell_reflection_models[JO_CELL_MAX];
static int                          __jo_cell_count = 0;
/** @brief The last cells found at points (see __jo_cells_find_object()): the entities (the doors, the furniture) ask for
 *  the same points every frame (their depth bias), about 130 a frame in a level full of doors. Sets of 4 entries (the last one
 *  found first), emptied when the cells change */
# define JO_CELL_FIND_SETS_SHIFT            (7)
# define JO_CELL_FIND_SETS                  (1 << JO_CELL_FIND_SETS_SHIFT)
# define JO_CELL_FIND_WAYS                  (4)
/* (8 bytes: X and Z from the corner of the box around the cells, Y in 16 bits; the other points are not cached. Aligned
** on 4 bytes: a copy of an entry is 2 moves of 32 bits, aligned on 2 it was a call of memcpy()) */
typedef struct
{
    unsigned short                  x;
    unsigned short                  z;
    short                           y;
    short                           cell;
}                                   __attribute__((aligned(4))) __jo_cell_find_entry;
/* (in the heap while cells are loaded: 4 KB, none in a game without cells) */
static __jo_cell_find_entry         *__jo_cell_find_cache = JO_NULL;
/* Generation of the cells: changed when they are loaded or freed (see jo_cells_generation()), never 0 */
static unsigned short               __jo_cell_generation = 1;

/** @brief Next generation of the cells (the cells found before are stale) */
static void                         __jo_cell_next_generation(void)
{
    if (++__jo_cell_generation == 0)
        __jo_cell_generation = 1;
}
/* The box around the boxes of all the cells (X and Z, whole units): no cell out of it */
static int                          __jo_cells_x0 = 0;
static int                          __jo_cells_z0 = 0;
static int                          __jo_cells_x1 = -1;
static int                          __jo_cells_z1 = -1;
/** @brief The box where the player can walk of each cell in the world (whole units), around the turned boxes: the search of
 *  the cell of a point reads these 16 bytes, not the cell (the cells did not fit in the CPU cache) */
static int                          __jo_cell_walk_box[JO_CELL_MAX][4];
/** @brief A grid on the box around the cells (32 x 32 squares at most): the cells whose walk box meets each square, in
 *  their order (the search of the cell of a point reads a few boxes, not all of them: the entities ask for the cells at
 *  each step). first: the first entry of each square (one more at the end), then the entries (indexes of the cells) */
# define JO_CELL_GRID_SIZE                  (32)
static unsigned short               *__jo_cell_grid_first = JO_NULL;
static unsigned char                *__jo_cell_grid_cells;
static int                          __jo_cell_grid_shift;
/** @brief 2^(16 - __jo_cell_grid_shift): the square of a position is (d x it) >> 16, the same as d >> the shift (d: 0 to
 *  65535 units). A shift by a variable count was a routine of the library of the compiler (___ashrsi3, a loop) */
static unsigned int                 __jo_cell_grid_scale;
static int                          __jo_cell_grid_width;
/** @brief All the cells in their order (no grid: not enough memory) */
static unsigned char                __jo_cell_all[JO_CELL_MAX];
static void                         __jo_cell_build_grid(void);
static void                         __jo_cell_free_grid(void);

static void                         __jo_cell_find_cache_clear(void)
{
    int                             k;

    if (__jo_cell_find_cache == JO_NULL)
        return;
    for (JO_ZERO(k); k < JO_CELL_FIND_SETS * JO_CELL_FIND_WAYS; ++k)
        __jo_cell_find_cache[k].cell = -2;
}
static jo_cell_portal               __jo_cell_portals[JO_CELL_MAX_PORTALS];
/* The portals closed (a closed door: the cell behind it is not drawn, see jo_cells_close_portals()) */
static unsigned char                __jo_cell_portal_closed[JO_CELL_MAX_PORTALS];
// The back of the mirrors in the walls (a dark glass): drawn when the mirror is not (seen through a door), else its hole in
// the wall showed the background
static jo_3d_mesh                   *__jo_cell_mirror_backs[JO_CELL_MAX_PORTALS];
static int                          __jo_cell_portal_count = 0;
static unsigned char                __jo_cell_drawn[JO_CELL_MAX];
// The rectangle of the screen where each drawn cell is seen (see jo_cells_seen_through(), jo_cells_view_rect_at())
static __jo_cell_view               __jo_cell_from_outside[JO_CELL_MAX];
/** @brief The cell is seen from the outside through a window (or from its doorway: the whole screen) in this frame */
static bool                         __jo_cell_through_window[JO_CELL_MAX];
// The cells to draw (see jo_cells_prepare()): their meshes
static jo_3d_mesh                   *__jo_cell_meshes[JO_CELL_MAX_DRAWN];
/*
** The chunks of the model of a cell: runs of polygons (in the order of the model: the walls, the furniture are made
** together) with their bounding spheres. A cell seen through a door draws only the chunks seen in the rectangle of the
** door (a room behind 2 doors drew all of its polygons: too many VDP1 commands in the frame)
*/
/** @brief Polygons of a chunk (more when the next polygons are sorted with the previous one: a decal) */
#define JO_CELL_CHUNK_POLYGONS              (24)
/** @brief Ranges of polygons drawn for a mesh (the chunks seen, the ones next to each other together) */
#define JO_CELL_MAX_RANGES                  (48)

typedef struct
{
    unsigned short                  first;
    unsigned short                  count;
    jo_pos3D_fixed                  center;
    int                             radius;
}                                   __jo_cell_chunk;

static __jo_cell_chunk              *__jo_cell_chunks[JO_CELL_MAX];
static int                          __jo_cell_chunk_count[JO_CELL_MAX];
static void                         __jo_cell_make_chunks(const int cell, const jo_3d_mesh * const mesh);
/** @brief The ranges of polygons to draw of each mesh to draw (-1: all of them) */
static jo_3d_polygon_range          __jo_cell_mesh_ranges[JO_CELL_MAX_DRAWN][JO_CELL_MAX_RANGES];
static int                          __jo_cell_mesh_range_count[JO_CELL_MAX_DRAWN];
static unsigned char                __jo_cell_mesh_levels[JO_CELL_MAX_DRAWN];
static int                          __jo_cell_mesh_count = 0;
// Depth bias of the cells of the camera (see jo_cells_prepare())
static int                          __jo_cell_depth_bias = 0;
// The mirrors seen from the camera (see jo_cells_draw_mirror(), 2 at most: a polished floor and a mirror on a wall): their
// cells (-1: none), their rectangles on the screen, their planes. The mirror drawn now (see jo_cells_mirror_is_floor())
# define JO_CELL_MAX_MIRRORS                (2)
static int                          __jo_cell_mirror[JO_CELL_MAX_MIRRORS] = {-1, -1};
static int                          __jo_cell_mirror_count = 0;
static int                          __jo_cell_mirror_drawn = 0;
// The outside seen from the camera (see jo_cells_outside_view()): -1 the camera is outside, 0 not seen, 1 seen in the rectangle
static int                          __jo_cell_outside = -1;
static __jo_cell_view               __jo_cell_outside_view;
static __jo_cell_view               __jo_cell_mirror_view[JO_CELL_MAX_MIRRORS];
static jo_pos3D_fixed               __jo_cell_mirror_normal[JO_CELL_MAX_MIRRORS];
static jo_fixed                     __jo_cell_mirror_distance[JO_CELL_MAX_MIRRORS];

static bool                         __jo_cell_portal_plane(const jo_cell_portal * const p, jo_pos3D_fixed * const n, jo_fixed * const d);

void                                jo_cells_free(void)
{
    int                             k;

    for (k = 0; k < __jo_cell_count; ++k)
    {
        if (__jo_cell_ray_planes[k] != JO_NULL)
            jo_free(__jo_cell_ray_planes[k]);
        __jo_cell_ray_planes[k] = JO_NULL;
        if (__jo_cell_models[k] != JO_NULL)
            jo_3d_free_model(__jo_cell_models[k]);
        if (__jo_cell_reflection_models[k] != JO_NULL)
            jo_3d_free_model(__jo_cell_reflection_models[k]);
    }
    for (k = 0; k < __jo_cell_portal_count; ++k)
        if (__jo_cell_mirror_backs[k] != JO_NULL)
        {
            jo_3d_free_mesh(__jo_cell_mirror_backs[k]);
            __jo_cell_mirror_backs[k] = JO_NULL;
        }
    __jo_cell_count = 0;
    __jo_cell_portal_count = 0;
    __jo_cell_free_grid();
    __jo_cell_next_generation();
    if (__jo_cell_find_cache != JO_NULL)
        jo_free(__jo_cell_find_cache);
    __jo_cell_find_cache = JO_NULL;
    __jo_cells_x0 = 0;
    __jo_cells_z0 = 0;
    __jo_cells_x1 = -1;
    __jo_cells_z1 = -1;
    jo_memset(__jo_cell_portal_closed, 0, sizeof(__jo_cell_portal_closed));
}

/** @brief The box of a model (world units, with a margin of 1)
 *  @param mesh Model
 *  @param box x, y, z min, then x, y, z max
 */
static void                         __jo_cell_compute_box(const jo_3d_mesh * const mesh, short * const box)
{
    const jo_fixed                  *v;
    int                             lo[3];
    int                             hi[3];
    int                             j;
    int                             k;

    for (k = 0; k < 3; ++k)
    {
        lo[k] = 0x7fff;
        hi[k] = -0x7fff;
    }
    for (j = 0; j < (int)mesh->data.point_count; ++j)
    {
        v = (const jo_fixed *)&mesh->data.points[j];
        for (k = 0; k < 3; ++k)
        {
            lo[k] = JO_MIN(lo[k], jo_fixed2int(v[k]) - 1);
            hi[k] = JO_MAX(hi[k], jo_fixed2int(v[k]) + 1);
        }
    }
    for (k = 0; k < 3; ++k)
    {
        box[k] = (short)JO_MAX(lo[k], -0x7fff);
        box[k + 3] = (short)JO_MIN(hi[k], 0x7fff);
    }
}

/** @brief The box in the world around the box where the player can walk of a cell (see __jo_cell_walk_box) */
static void                         __jo_cell_compute_walk_box(const jo_cell * const c, int box[4])
{
    int                             hx;
    int                             hz;
    int                             cx;
    int                             cz;
    int                             half;

    if (c->yaw == 0)
    {
        box[0] = c->x0;
        box[1] = c->z0;
        box[2] = c->x1;
        box[3] = c->z1;
        return;
    }
    // (turned: any turn of the box is in its center + or - the 2 half sizes; + 2: the rounding of __jo_cell_local())
    cx = (c->x0 + c->x1) / 2;
    cz = (c->z0 + c->z1) / 2;
    hx = (c->x1 - c->x0) / 2 + 1;
    hz = (c->z1 - c->z0) / 2 + 1;
    half = hx + hz + 2;
    box[0] = cx - half;
    box[1] = cz - half;
    box[2] = cx + half;
    box[3] = cz + half;
}

int                                 jo_cells_load(jo_level * const level)
{
    const jo_level_cell_record      *cr;
    const jo_level_prtl_record      *pr;
    unsigned int                    count;
    unsigned int                    portal_count;
    unsigned int                    k;
    int                             j;
    jo_cell                         *c;
    jo_cell_portal                  *p;

    jo_cells_free();
    pr = JO_LEVEL_RECORDS(level, PRTL, prtl, &portal_count);
    __jo_cell_portal_count = (int)JO_MIN(portal_count, JO_CELL_MAX_PORTALS);
    for (k = 0, p = __jo_cell_portals; k < (unsigned int)__jo_cell_portal_count; ++k, ++p, ++pr)
    {
        for (j = 0; j < 4; ++j)
        {
            p->points[j].x = jo_int2fixed(pr->points[j * 3]);
            p->points[j].y = jo_int2fixed(pr->points[j * 3 + 1]);
            p->points[j].z = jo_int2fixed(pr->points[j * 3 + 2]);
        }
        p->cell = pr->cell;
        p->kind = pr->kind;
    }
    // The backs of the mirrors in the walls (not the polished floors: a half transparent floor over nothing is drawn as it is)
    for (k = 0, p = __jo_cell_portals; k < (unsigned int)__jo_cell_portal_count; ++k, ++p)
    {
        jo_pos3D_fixed              n;
        jo_fixed                    d;

        __jo_cell_mirror_backs[k] = JO_NULL;
        if (p->kind != JO_LEVEL_PORTAL_MIRROR || !__jo_cell_portal_plane(p, &n, &d) || JO_ABS(n.y) > JO_FIXED_1 / 2)
            continue;
        __jo_cell_mirror_backs[k] = jo_3d_create_indexed_mesh(4, 1);
        if (__jo_cell_mirror_backs[k] == JO_NULL)
            continue;
        for (j = 0; j < 4; ++j)
        {
            __jo_cell_mirror_backs[k]->data.points[j] = p->points[j];
            __jo_cell_mirror_backs[k]->data.polygons[0].vertices[j] = (unsigned short)j;
        }
        jo_3d_set_mesh_polygon_color(__jo_cell_mirror_backs[k], JO_COLOR_RGB(52, 60, 66), 0);
        __jo_cell_mirror_backs[k]->data.attributes[0].plane = JO_3D_DUAL_PLANE;
    }
    cr = JO_LEVEL_RECORDS(level, CELL, cell, &count);
    if (count > JO_CELL_MAX)
    {
#ifdef JO_DEBUG
        jo_core_error("Too many cells: %d (max=%d)", count, JO_CELL_MAX);
#endif
        count = JO_CELL_MAX;
    }
    for (k = 0, c = __jo_cells; k < count; ++k, ++c, ++cr)
    {
        jo_memset(c, 0, sizeof(*c));
        __jo_cell_models[k] = cr->model != JO_LEVEL_NO_STRING ? __jo_3d_load_model_for_lights(jo_level_string(level, cr->model)) : JO_NULL;
        if (__jo_cell_models[k] != JO_NULL)
        {
            c->mesh = __jo_cell_models[k]->mesh;
            c->radius = jo_3d_mesh_get_bounding_sphere(c->mesh, &c->center);
            __jo_cell_compute_box(c->mesh, __jo_cell_box[k]);
            __jo_cell_ray_planes[k] = jo_3d_mesh_create_ray_planes(c->mesh);
            __jo_cell_make_chunks(k, c->mesh);
        }
        else
            __jo_cell_ray_planes[k] = JO_NULL;
        __jo_cell_reflection_models[k] = cr->reflection != JO_LEVEL_NO_STRING ? jo_3d_load_model(jo_level_string(level, cr->reflection)) : JO_NULL;
        if (__jo_cell_reflection_models[k] != JO_NULL)
            c->reflection = __jo_cell_reflection_models[k]->mesh;
        c->x0 = cr->x0;
        c->z0 = cr->z0;
        c->x1 = cr->x1;
        c->z1 = cr->z1;
        c->floor0 = cr->floor0;
        c->floor1 = cr->floor1;
        c->slope = cr->slope;
        c->yaw = cr->yaw;
        c->cos = jo_cos(cr->yaw);
        c->sin = jo_sin(cr->yaw);
        __jo_cell_compute_walk_box(c, __jo_cell_walk_box[k]);
        // (portals outside of the section: none)
        if (cr->first_portal + cr->portal_count <= __jo_cell_portal_count)
        {
            c->first_portal = cr->first_portal;
            c->portal_count = cr->portal_count;
        }
    }
    __jo_cell_count = (int)count;
    __jo_cells_x0 = 0x7fffffff;
    __jo_cells_z0 = 0x7fffffff;
    __jo_cells_x1 = -0x7fffffff;
    __jo_cells_z1 = -0x7fffffff;
    for (JO_ZERO(k); k < count; ++k)
    {
        __jo_cells_x0 = JO_MIN(__jo_cells_x0, __jo_cell_walk_box[k][0]);
        __jo_cells_z0 = JO_MIN(__jo_cells_z0, __jo_cell_walk_box[k][1]);
        __jo_cells_x1 = JO_MAX(__jo_cells_x1, __jo_cell_walk_box[k][2]);
        __jo_cells_z1 = JO_MAX(__jo_cells_z1, __jo_cell_walk_box[k][3]);
    }
    __jo_cell_build_grid();
    if (__jo_cell_find_cache == JO_NULL)
        __jo_cell_find_cache = (__jo_cell_find_entry *)jo_malloc(JO_CELL_FIND_SETS * JO_CELL_FIND_WAYS * sizeof(__jo_cell_find_entry));
    __jo_cell_find_cache_clear();
    __jo_cell_next_generation();
    return (__jo_cell_count);
}

int                                 jo_cells_count(void)
{
    return (__jo_cell_count);
}

bool                                jo_cells_have_exits(void)
{
    int                             k;

    for (k = 0; k < __jo_cell_portal_count; ++k)
        if ((__jo_cell_portals[k].kind == JO_LEVEL_PORTAL_DOOR || __jo_cell_portals[k].kind == JO_LEVEL_PORTAL_WINDOW) &&
            __jo_cell_portals[k].cell == JO_LEVEL_NO_INDEX)
            return (true);
    return (false);
}

const jo_cell                       *jo_cells_get(const int index)
{
    return (index >= 0 && index < __jo_cell_count ? &__jo_cells[index] : JO_NULL);
}

/** @brief A position in the box of a cell before its rotation (see jo_cell.yaw)
 *  @param x Position (world, output: in the box)
 *  @param z Position
 */
static void                         __jo_cell_local(const jo_cell * const cell, int * const x, int * const z)
{
    int                             cx;
    int                             cz;
    int                             dx;
    int                             dz;

    if (cell->yaw == 0)
        return;
    cx = (cell->x0 + cell->x1) / 2;
    cz = (cell->z0 + cell->z1) / 2;
    dx = *x - cx;
    dz = *z - cz;
    // The inverse of the rotation of the box (the same convention as jo_3d_rotate_matrix_y())
    *x = cx + jo_fixed2int(dx * cell->cos - dz * cell->sin);
    *z = cz + jo_fixed2int(dx * cell->sin + dz * cell->cos);
}

int                                 jo_cell_floor(const jo_cell * const cell, const int wx, const int wz)
{
    int                             x;
    int                             z;

    x = wx;
    z = wz;
    __jo_cell_local(cell, &x, &z);
    if (cell->slope == JO_LEVEL_CELL_SLOPE_X && cell->x1 > cell->x0)
        return (cell->floor0 + (cell->floor1 - cell->floor0) * (JO_MIN(JO_MAX(x, cell->x0), cell->x1) - cell->x0) / (cell->x1 - cell->x0));
    if (cell->slope == JO_LEVEL_CELL_SLOPE_Z && cell->z1 > cell->z0)
        return (cell->floor0 + (cell->floor1 - cell->floor0) * (JO_MIN(JO_MAX(z, cell->z0), cell->z1) - cell->z0) / (cell->z1 - cell->z0));
    return (cell->floor0);
}

/** @brief Build the grid of the cells (see __jo_cell_grid_first), after their walk boxes and the box around them */
static void                         __jo_cell_build_grid(void)
{
    int                             k;
    int                             x;
    int                             z;
    int                             height;
    int                             total;
    unsigned short                  *first;
    const int                       *box;

    for (JO_ZERO(k); k < JO_CELL_MAX; ++k)
        __jo_cell_all[k] = (unsigned char)k;
    // (no grid beyond 65535 units: see __jo_cell_grid_scale. All the cells are read)
    if (__jo_cell_count == 0 || __jo_cells_x1 - __jo_cells_x0 > 65535 || __jo_cells_z1 - __jo_cells_z0 > 65535)
        return;
    for (__jo_cell_grid_shift = 0; ((__jo_cells_x1 - __jo_cells_x0) >> __jo_cell_grid_shift) >= JO_CELL_GRID_SIZE ||
                                   ((__jo_cells_z1 - __jo_cells_z0) >> __jo_cell_grid_shift) >= JO_CELL_GRID_SIZE; ++__jo_cell_grid_shift)
        ;
    __jo_cell_grid_scale = (unsigned int)65536 >> __jo_cell_grid_shift;
    __jo_cell_grid_width = ((__jo_cells_x1 - __jo_cells_x0) >> __jo_cell_grid_shift) + 1;
    height = ((__jo_cells_z1 - __jo_cells_z0) >> __jo_cell_grid_shift) + 1;
    // The entries of each square (counted in the next square: the first of each square after the sum)
    first = (unsigned short *)jo_malloc((__jo_cell_grid_width * height + 2) * (int)sizeof(unsigned short));
    if (first == JO_NULL)
        return;
    jo_memset(first, 0, (unsigned int)((__jo_cell_grid_width * height + 2) * (int)sizeof(unsigned short)));
    for (k = 0, box = __jo_cell_walk_box[0]; k < __jo_cell_count; ++k, box += 4)
        for (z = (box[1] - __jo_cells_z0) >> __jo_cell_grid_shift; z <= (box[3] - __jo_cells_z0) >> __jo_cell_grid_shift; ++z)
            for (x = (box[0] - __jo_cells_x0) >> __jo_cell_grid_shift; x <= (box[2] - __jo_cells_x0) >> __jo_cell_grid_shift; ++x)
                ++first[z * __jo_cell_grid_width + x + 2];
    for (k = 2, total = 0; k < __jo_cell_grid_width * height + 2; ++k)
    {
        total += first[k];
        first[k] = (unsigned short)total;
    }
    __jo_cell_grid_cells = (unsigned char *)jo_malloc(JO_MAX(total, 1));
    if (__jo_cell_grid_cells == JO_NULL)
    {
        jo_free(first);
        return;
    }
    // (first[square + 1]: the next entry of the square while they are written, then its first entry once they are)
    for (k = 0, box = __jo_cell_walk_box[0]; k < __jo_cell_count; ++k, box += 4)
        for (z = (box[1] - __jo_cells_z0) >> __jo_cell_grid_shift; z <= (box[3] - __jo_cells_z0) >> __jo_cell_grid_shift; ++z)
            for (x = (box[0] - __jo_cells_x0) >> __jo_cell_grid_shift; x <= (box[2] - __jo_cells_x0) >> __jo_cell_grid_shift; ++x)
                __jo_cell_grid_cells[first[z * __jo_cell_grid_width + x + 1]++] = (unsigned char)k;
    __jo_cell_grid_first = first;
}

/** @brief Free the grid of the cells */
static void                         __jo_cell_free_grid(void)
{
    if (__jo_cell_grid_first == JO_NULL)
        return;
    jo_free(__jo_cell_grid_cells);
    jo_free(__jo_cell_grid_first);
    __jo_cell_grid_first = JO_NULL;
}

/** @brief The cells whose walk box may hold a point, in their order: all the cells whose walk box holds it are there
 *  @param ix Point (whole units)
 *  @param iz Point
 *  @param end End of the cells (output)
 *  @return First cell
 */
static  __jo_force_inline const unsigned char *__jo_cell_candidates(const int ix, const int iz, const unsigned char ** const end)
{
    int                             square;

    if (__jo_cell_grid_first == JO_NULL)
    {
        *end = __jo_cell_all + __jo_cell_count;
        return (__jo_cell_all);
    }
    // (out of the box around the cells: in no cell)
    if (ix < __jo_cells_x0 || ix > __jo_cells_x1 || iz < __jo_cells_z0 || iz > __jo_cells_z1)
    {
        *end = __jo_cell_all;
        return (__jo_cell_all);
    }
    square = (int)(((unsigned int)(iz - __jo_cells_z0) * __jo_cell_grid_scale) >> 16) * __jo_cell_grid_width +
             (int)(((unsigned int)(ix - __jo_cells_x0) * __jo_cell_grid_scale) >> 16);
    *end = __jo_cell_grid_cells + __jo_cell_grid_first[square + 1];
    return (__jo_cell_grid_cells + __jo_cell_grid_first[square]);
}

/** @brief Cell of a position whose box has a circle around the position (the highest floor under the feet + step)
 *  @return Index of the cell, -1 if none
 */
static int                          __jo_cells_find(const jo_fixed x, const jo_fixed y, const jo_fixed z, const int radius, const int step,
                                                    int * const floor)
{
    int                             k;
    int                             ix;
    int                             iz;
    int                             iy;
    int                             f;
    int                             best;
    int                             best_floor;
    int                             lx;
    int                             lz;
    const int                       *box;
    const jo_cell                   *c;
    const unsigned char             *cells;
    const unsigned char             *end;

    ix = jo_fixed2int(x);
    iy = jo_fixed2int(y);
    iz = jo_fixed2int(z);
    best = -1;
    best_floor = 0x7fffffff;
    for (cells = __jo_cell_candidates(ix, iz, &end); cells < end; ++cells)
    {
        k = *cells;
        c = &__jo_cells[k];
        box = __jo_cell_walk_box[k];
        if (ix - radius < box[0] || ix + radius > box[2] || iz - radius < box[1] || iz + radius > box[3])
            continue;
        lx = ix;
        lz = iz;
        __jo_cell_local(c, &lx, &lz);
        if (lx - radius < c->x0 || lx + radius > c->x1 || lz - radius < c->z0 || lz + radius > c->z1)
            continue;
        // Saturn Y: the floor is under the feet when it is bigger (or a step over them)
        f = jo_cell_floor(c, ix, iz);
        if (f < iy - step || f >= best_floor)
            continue;
        best = k;
        best_floor = f;
    }
    if (floor != JO_NULL)
        *floor = best_floor;
    return (best);
}

/** @brief __jo_cells_find() with the step 8, else with any step, in one pass (the cell of an object)
 *  @return Index of the cell, -1 if none
 */
static int                          __jo_cells_find_any(const jo_fixed x, const jo_fixed y, const jo_fixed z, int * const floor)
{
    int                             k;
    int                             ix;
    int                             iz;
    int                             iy;
    int                             f;
    int                             best;
    int                             best_floor;
    int                             any;
    int                             any_floor;
    int                             lx;
    int                             lz;
    const int                       *box;
    const jo_cell                   *c;
    const unsigned char             *cells;
    const unsigned char             *end;

    ix = jo_fixed2int(x);
    iy = jo_fixed2int(y);
    iz = jo_fixed2int(z);
    best = -1;
    best_floor = 0x7fffffff;
    any = -1;
    any_floor = 0x7fffffff;
    for (cells = __jo_cell_candidates(ix, iz, &end); cells < end; ++cells)
    {
        k = *cells;
        c = &__jo_cells[k];
        box = __jo_cell_walk_box[k];
        if (ix < box[0] || ix > box[2] || iz < box[1] || iz > box[3])
            continue;
        lx = ix;
        lz = iz;
        __jo_cell_local(c, &lx, &lz);
        if (lx < c->x0 || lx > c->x1 || lz < c->z0 || lz > c->z1)
            continue;
        f = jo_cell_floor(c, ix, iz);
        if (f >= iy - 0x7fff && f < any_floor)
        {
            any = k;
            any_floor = f;
        }
        if (f >= iy - 8 && f < best_floor)
        {
            best = k;
            best_floor = f;
        }
    }
    if (best < 0)
    {
        best = any;
        best_floor = any_floor;
    }
    *floor = best_floor;
    return (best);
}

int                                 jo_cells_find(const jo_fixed x, const jo_fixed y, const jo_fixed z, const int step)
{
    return (__jo_cells_find(x, y, z, 0, step, JO_NULL));
}

/** @brief The cell of an object (its depth bias, its rectangle of the screen): the cell whose floor is under it (or a little
 *  over it), else any cell around it. The cells one over the other (a duct over a room): with any floor, the highest one
 *  was taken, the seat of a toilet under the duct was in the duct (sorted with it, behind the walls of the room) */
/** @brief Check if a point is in the bounding sphere of the model of a cell (whole units: no overflow of the squares) */
static bool                         __jo_cell_sphere_contains(const jo_cell * const c, const int x, const int y, const int z)
{
    int                             dx;
    int                             dy;
    int                             dz;

    if (c->mesh == JO_NULL)
        return (false);
    dx = JO_ABS(x - jo_fixed2int(c->center.x));
    dy = JO_ABS(y - jo_fixed2int(c->center.y));
    dz = JO_ABS(z - jo_fixed2int(c->center.z));
    if (dx > c->radius || dy > c->radius || dz > c->radius)
        return (false);
    return (dx * dx + dy * dy + dz * dz <= c->radius * c->radius);
}

/** @brief The room on the side of a doorway (a cell without model between 2 rooms) where a point is: the point in the
 *  doorway past the box of the room (the landing of stairs, the box of their slope starts after it) was in the doorway
 *  only, its portals behind the camera: black. The portals are vertical planes along X or Z
 *  @return The room, else the doorway */
static int                          __jo_cells_doorway_room(const int doorway, const int x, const int z)
{
    const jo_cell                   *c;
    const jo_cell                   *room;
    const jo_cell_portal            *p;
    int                             k;
    int                             px;
    int                             pz;
    int                             side;
    int                             room_side;
    int                             best;
    int                             best_distance;
    int                             distance;

    c = &__jo_cells[doorway];
    best = doorway;
    best_distance = 0x7fffffff;
    for (k = 0, p = &__jo_cell_portals[c->first_portal]; k < c->portal_count; ++k, ++p)
    {
        if (p->cell >= __jo_cell_count || __jo_cells[p->cell].mesh == JO_NULL)
            continue;
        room = &__jo_cells[p->cell];
        px = jo_fixed2int(p->points[0].x);
        pz = jo_fixed2int(p->points[0].z);
        // (the plane of the portal: along Z when its points have the same X)
        if (p->points[0].x == p->points[1].x && p->points[0].x == p->points[2].x)
        {
            side = x - px;
            room_side = jo_fixed2int(room->center.x) - px;
            distance = JO_ABS(side);
        }
        else
        {
            side = z - pz;
            room_side = jo_fixed2int(room->center.z) - pz;
            distance = JO_ABS(side);
        }
        if ((side >= 0) != (room_side >= 0) || distance >= best_distance)
            continue;
        best = p->cell;
        best_distance = distance;
    }
    return (best);
}

static int                          __jo_cells_find_object_uncached(const jo_fixed x, const jo_fixed y, const jo_fixed z)
{
    int                             cell;
    int                             floor;
    int                             k;
    int                             ix;
    int                             iy;
    int                             iz;
    int                             lx;
    int                             lz;
    int                             best_radius;
    const int                       *box;
    const jo_cell                   *c;
    const unsigned char             *cells;
    const unsigned char             *end;

    cell = __jo_cells_find_any(x, y, z, &floor);
    if (cell < 0)
        return (cell);
    // Cells one over the other with the same floor (the end of a duct over its hole, its floor the one of the room: the
    // agent falls in it): the one whose model is around the point, the smallest one. The camera falling through the hole
    // is in the end of the duct (the room did not see the duct from above its ceiling: black), the camera in the room
    // under it is in the room (the end of the duct does not see the room around: black)
    ix = jo_fixed2int(x);
    iy = jo_fixed2int(y);
    iz = jo_fixed2int(z);
    best_radius = __jo_cell_sphere_contains(&__jo_cells[cell], ix, iy, iz) ? __jo_cells[cell].radius : 0x7fffffff;
    for (cells = __jo_cell_candidates(ix, iz, &end); cells < end; ++cells)
    {
        k = *cells;
        c = &__jo_cells[k];
        box = __jo_cell_walk_box[k];
        if (ix < box[0] || ix > box[2] || iz < box[1] || iz > box[3] || k == cell || c->radius >= best_radius)
            continue;
        lx = ix;
        lz = iz;
        __jo_cell_local(c, &lx, &lz);
        if (lx < c->x0 || lx > c->x1 || lz < c->z0 || lz > c->z1 || jo_cell_floor(c, ix, iz) != floor)
            continue;
        if (!__jo_cell_sphere_contains(c, ix, iy, iz))
            continue;
        cell = k;
        best_radius = c->radius;
    }
    if (__jo_cells[cell].mesh == JO_NULL)
        cell = __jo_cells_doorway_room(cell, ix, iz);
    return (cell);
}

/** @brief The cell of an object at a point (see __jo_cells_find_object_uncached()), the last ones in a cache */
static int                          __jo_cells_find_object(const jo_fixed x, const jo_fixed y, const jo_fixed z)
{
    __jo_cell_find_entry            *entry;
    __jo_cell_find_entry            hit;
    int                             k;
    int                             ix;
    int                             iy;
    int                             iz;

    ix = jo_fixed2int(x);
    iy = jo_fixed2int(y);
    iz = jo_fixed2int(z);
    // (out of the box around the cells: in no cell)
    if (ix < __jo_cells_x0 || ix > __jo_cells_x1 || iz < __jo_cells_z0 || iz > __jo_cells_z1)
        return (-1);
    ix -= __jo_cells_x0;
    iz -= __jo_cells_z0;
    if (__jo_cell_find_cache == JO_NULL || ix > 0xffff || iz > 0xffff || (unsigned int)(iy + 0x8000) > 0xffff)
        return (__jo_cells_find_object_uncached(x, y, z));
    // (a multiplicative hash, the top bits: the points of the level are on a grid, a sum of small multiples left most of
    // the entries unused. The ways of a set: the last one found first)
    entry = &__jo_cell_find_cache[(((unsigned int)ix * 0x9E3779B1u + (unsigned int)iy * 0x85EBCA77u + (unsigned int)iz * 0xC2B2AE3Du) >>
                                   (32 - JO_CELL_FIND_SETS_SHIFT)) * JO_CELL_FIND_WAYS];
    for (JO_ZERO(k); k < JO_CELL_FIND_WAYS; ++k)
        if (entry[k].x == ix && entry[k].z == iz && entry[k].y == iy && entry[k].cell != -2)
        {
            hit = entry[k];
            for (; k > 0; --k)
                entry[k] = entry[k - 1];
            entry[0] = hit;
            return (hit.cell);
        }
    for (k = JO_CELL_FIND_WAYS - 1; k > 0; --k)
        entry[k] = entry[k - 1];
    entry[0].x = (unsigned short)ix;
    entry[0].z = (unsigned short)iz;
    entry[0].y = (short)iy;
    entry[0].cell = (short)__jo_cells_find_object_uncached(x, y, z);
    return (entry[0].cell);
}

bool                                jo_cells_floor(const jo_fixed x, const jo_fixed y, const jo_fixed z, const int step, int * const floor)
{
    return (__jo_cells_find(x, y, z, 0, step, floor) >= 0);
}

bool                                jo_cells_is_walkable(const jo_fixed x, const jo_fixed y, const jo_fixed z, const int radius, const int step)
{
    return (__jo_cells_find(x, y, z, radius, step, JO_NULL) >= 0);
}

/*
** Drawing
*/

/** @brief Rectangle of the screen where a portal is seen, in the rectangle of the cell before it
 *  @param portal Portal
 *  @param from Rectangle of the cell before it
 *  @param to Rectangle of the portal (output)
 *  @return false if the portal is not seen
 */
static bool                         __jo_cell_portal_view(const jo_cell_portal * const portal, const __jo_cell_view * const from,
                                                          __jo_cell_view * const to)
{
    jo_pos3D_fixed                  v[4];
    jo_pos3D_fixed                  points[8];
    jo_pos3D_fixed                  *a;
    jo_pos3D_fixed                  *b;
    jo_fixed                        k;
    jo_fixed                        t;
    jo_fixed                        pd;
    int                             sx;
    int                             sy;
    int                             left;
    int                             top;
    int                             right;
    int                             bottom;
    int                             count;
    int                             j;

    pd = jo_3d_get_projection_distance();
    for (j = 0; j < 4; ++j)
        jo_3d_transform_point(&portal->points[j], &v[j]);
    // The quad clipped by the near plane: its corners in front of the camera, the points of its edges on the near plane
    count = 0;
    for (j = 0; j < 4; ++j)
    {
        a = &v[j];
        b = &v[(j + 1) & 3];
        if (a->z >= JO_CELL_PORTAL_NEAR)
            points[count++] = *a;
        if ((a->z >= JO_CELL_PORTAL_NEAR) != (b->z >= JO_CELL_PORTAL_NEAR))
        {
            t = jo_fixed_div(JO_CELL_PORTAL_NEAR - a->z, b->z - a->z);
            points[count].x = a->x + jo_fixed_mult(b->x - a->x, t);
            points[count].y = a->y + jo_fixed_mult(b->y - a->y, t);
            points[count].z = JO_CELL_PORTAL_NEAR;
            ++count;
        }
    }
    if (count == 0)
        return (false);
    left = 0x7fff;
    top = 0x7fff;
    right = -0x7fff;
    bottom = -0x7fff;
    for (j = 0; j < count; ++j)
    {
        k = jo_fixed_div(pd, points[j].z);
        // (far outside of the screen: the rectangle of the cell before it is the limit)
        sx = JO_TV_WIDTH_2 + JO_MIN(JO_MAX(jo_fixed2int(jo_fixed_mult(points[j].x, k)), -10000), 10000);
        sy = JO_TV_HEIGHT_2 + JO_MIN(JO_MAX(jo_fixed2int(jo_fixed_mult(points[j].y, k)), -10000), 10000);
        left = JO_MIN(left, sx);
        right = JO_MAX(right, sx);
        top = JO_MIN(top, sy);
        bottom = JO_MAX(bottom, sy);
    }
    to->left = (short)JO_MAX(left, from->left);
    to->top = (short)JO_MAX(top, from->top);
    to->right = (short)JO_MIN(right, from->right);
    to->bottom = (short)JO_MIN(bottom, from->bottom);
    return (to->left <= to->right && to->top <= to->bottom);
}

/** @brief The chunks of the model of a cell (see __jo_cell_chunk): runs of polygons, their bounding spheres */
static void                         __jo_cell_make_chunks(const int cell, const jo_3d_mesh * const mesh)
{
    const jo_3d_polygon_data        *d;
    __jo_cell_chunk                 *ch;
    int                             n;
    int                             first;
    int                             end;
    int                             i;
    int                             v;
    int                             lo[3];
    int                             hi[3];
    int                             p[3];
    int                             a;
    int                             r;

    __jo_cell_chunks[cell] = JO_NULL;
    __jo_cell_chunk_count[cell] = 0;
    d = &mesh->data;
    if (d->polygon_count <= JO_CELL_CHUNK_POLYGONS)
        return;
    n = (int)(d->polygon_count + JO_CELL_CHUNK_POLYGONS - 1) / JO_CELL_CHUNK_POLYGONS;
    __jo_cell_chunks[cell] = (__jo_cell_chunk *)jo_malloc(n * sizeof(__jo_cell_chunk));
    if (__jo_cell_chunks[cell] == JO_NULL)
        return;
    for (first = 0; first < (int)d->polygon_count && __jo_cell_chunk_count[cell] < n; first = end)
    {
        end = JO_MIN(first + JO_CELL_CHUNK_POLYGONS, (int)d->polygon_count);
        // (a polygon sorted with the previous one stays with it)
        while (end < (int)d->polygon_count && (d->attributes[end].sort & JO_3D_SORT_MASK) == JO_3D_SORT_PREVIOUS)
            ++end;
        lo[0] = lo[1] = lo[2] = 0x7fffffff;
        hi[0] = hi[1] = hi[2] = -0x7fffffff;
        for (i = first; i < end; ++i)
            for (v = 0; v < 4; ++v)
            {
                p[0] = jo_fixed2int(d->points[d->polygons[i].vertices[v]].x);
                p[1] = jo_fixed2int(d->points[d->polygons[i].vertices[v]].y);
                p[2] = jo_fixed2int(d->points[d->polygons[i].vertices[v]].z);
                for (a = 0; a < 3; ++a)
                {
                    lo[a] = JO_MIN(lo[a], p[a]);
                    hi[a] = JO_MAX(hi[a], p[a]);
                }
            }
        ch = &__jo_cell_chunks[cell][__jo_cell_chunk_count[cell]++];
        ch->first = (unsigned short)first;
        ch->count = (unsigned short)(end - first);
        ch->center.x = jo_int2fixed((lo[0] + hi[0]) / 2);
        ch->center.y = jo_int2fixed((lo[1] + hi[1]) / 2);
        ch->center.z = jo_int2fixed((lo[2] + hi[2]) / 2);
        // (the half diagonal of the box, 2 more: the points are rounded)
        r = (int)jo_sqrt((unsigned int)((hi[0] - lo[0]) * (hi[0] - lo[0]) + (hi[1] - lo[1]) * (hi[1] - lo[1]) +
                                        (hi[2] - lo[2]) * (hi[2] - lo[2]))) / 2 + 2;
        ch->radius = r;
    }
}

/** @brief The ranges of polygons of the model of a cell seen in a rectangle of the screen (the chunks in it)
 *  @return Number of ranges, -1: all the polygons (no chunks, or too many ranges)
 */
static int                          __jo_cell_visible_ranges(const int cell, const __jo_cell_view * const view, jo_3d_polygon_range * const ranges)
{
    const __jo_cell_chunk           *ch;
    jo_3d_rect                      rect;
    jo_3d_rect                      saved;
    bool                            had;
    jo_pos3D_fixed                  v;
    jo_fixed                        m[4][3];
    jo_fixed                        columns[3][3];
    int                             count;
    int                             k;

    if (__jo_cell_chunks[cell] == JO_NULL)
        return (-1);
    // (the rectangle of the program is kept: the intersection with the one of the cell, then it is set again)
    had = jo_3d_get_view_rect(&saved);
    rect.left = view->left;
    rect.top = view->top;
    rect.right = view->right;
    rect.bottom = view->bottom;
    if (had)
    {
        rect.left = JO_MAX(rect.left, saved.left);
        rect.top = JO_MAX(rect.top, saved.top);
        rect.right = JO_MIN(rect.right, saved.right);
        rect.bottom = JO_MIN(rect.bottom, saved.bottom);
    }
    jo_3d_set_view_rect(&rect);
    // (the columns of the matrix once for all the chunks: the same products as jo_3d_transform_point())
    jo_3d_get_matrix((jo_fixed *)m);
    for (k = 0; k < 3; ++k)
    {
        columns[k][0] = m[0][k];
        columns[k][1] = m[1][k];
        columns[k][2] = m[2][k];
    }
    count = 0;
    for (k = 0, ch = __jo_cell_chunks[cell]; k < __jo_cell_chunk_count[cell]; ++k, ++ch)
    {
        v.x = jo_fixed_dot((const jo_fixed *)&ch->center, columns[0]) + m[3][0];
        v.y = jo_fixed_dot((const jo_fixed *)&ch->center, columns[1]) + m[3][1];
        v.z = jo_fixed_dot((const jo_fixed *)&ch->center, columns[2]) + m[3][2];
        if (!jo_3d_is_sphere_visible(&v, jo_int2fixed(ch->radius), JO_CELL_FAR))
            continue;
        // (next to the previous range: together)
        if (count > 0 && ranges[count - 1].first + ranges[count - 1].count == ch->first)
        {
            ranges[count - 1].count += ch->count;
            continue;
        }
        if (count >= JO_CELL_MAX_RANGES)
        {
            jo_3d_set_view_rect(had ? &saved : JO_NULL);
            return (-1);
        }
        ranges[count].first = ch->first;
        ranges[count].count = ch->count;
        ++count;
    }
    jo_3d_set_view_rect(had ? &saved : JO_NULL);
    return (count);
}

/** @brief Add the mesh of a cell to the meshes to draw (when its bounding sphere is in the view)
 *  @param level Portals from the cell of the camera
 */
static void                         __jo_cell_add_mesh(const jo_cell * const c, const int level, const __jo_cell_view * const view)
{
    jo_pos3D_fixed                  v;

    if (c->mesh == JO_NULL || __jo_cell_mesh_count >= JO_CELL_MAX_DRAWN)
        return;
    jo_3d_transform_point(&c->center, &v);
    if (jo_3d_is_sphere_visible(&v, jo_int2fixed(c->radius), JO_CELL_FAR))
    {
        // (the chunks of the model seen in its rectangle; JO_NULL: all of it)
        __jo_cell_mesh_range_count[__jo_cell_mesh_count] = view != JO_NULL ?
                                                            __jo_cell_visible_ranges((int)(c - __jo_cells), view, __jo_cell_mesh_ranges[__jo_cell_mesh_count]) : -1;
        __jo_cell_mesh_levels[__jo_cell_mesh_count] = (unsigned char)JO_MIN(level, 15);
        __jo_cell_meshes[__jo_cell_mesh_count++] = c->mesh;
    }
}

/** @brief Check if the camera is in the opening of a portal (near its plane, inside its corners) */
/** @brief Distance from the outside under which a cell is seen through its windows (see jo_cells_set_window_distance()) */
static jo_fixed                     __jo_cell_window_distance = 0;

void                                jo_cells_set_window_distance(const int distance)
{
    __jo_cell_window_distance = jo_int2fixed(distance);
}

/** @brief Check if the camera is near a window (its center, horizontal distance, see jo_cells_set_window_distance())
 *  @param p Portal
 *  @param camera Camera
 *  @return true if the cell is seen through the window from the outside
 */
static bool                         __jo_cell_is_near_window(const jo_cell_portal * const p, const jo_pos3D_fixed * const camera)
{
    int                             dx;
    int                             dz;
    int                             d;

    if (__jo_cell_window_distance <= 0)
        return (false);
    // (whole units: the squares of the distances overflow the fixed point numbers)
    dx = jo_fixed2int(JO_DIV_BY_4(p->points[0].x) + JO_DIV_BY_4(p->points[1].x) + JO_DIV_BY_4(p->points[2].x) + JO_DIV_BY_4(p->points[3].x) - camera->x);
    dz = jo_fixed2int(JO_DIV_BY_4(p->points[0].z) + JO_DIV_BY_4(p->points[1].z) + JO_DIV_BY_4(p->points[2].z) + JO_DIV_BY_4(p->points[3].z) - camera->z);
    d = jo_fixed2int(__jo_cell_window_distance);
    return (JO_ABS(dx) < d && JO_ABS(dz) < d && dx * dx + dz * dz < d * d);
}

static bool                         __jo_cell_is_in_doorway(const jo_cell_portal * const p, const jo_pos3D_fixed * const camera)
{
    jo_fixed                        lo[3];
    jo_fixed                        hi[3];
    const jo_fixed                  *c;
    int                             j;
    int                             k;

    c = (const jo_fixed *)camera;
    // (the portals are planar: their box is thin along their normal)
    for (k = 0; k < 3; ++k)
    {
        lo[k] = ((const jo_fixed *)&p->points[0])[k];
        hi[k] = lo[k];
        for (j = 1; j < 4; ++j)
        {
            lo[k] = JO_MIN(lo[k], ((const jo_fixed *)&p->points[j])[k]);
            hi[k] = JO_MAX(hi[k], ((const jo_fixed *)&p->points[j])[k]);
        }
        if (c[k] < lo[k] - jo_int2fixed(JO_CELL_DOORWAY) || c[k] > hi[k] + jo_int2fixed(JO_CELL_DOORWAY))
            return (false);
    }
    return (true);
}

/** @brief Plane of a mirror: its normal (unit vector) and its distance (n.p = d)
 *  @return false if the portal is flat (no area)
 */
static bool                         __jo_cell_portal_plane(const jo_cell_portal * const p, jo_pos3D_fixed * const n, jo_fixed * const d)
{
    int                             ax;
    int                             ay;
    int                             az;
    int                             bx;
    int                             by;
    int                             bz;
    int                             nx;
    int                             ny;
    int                             nz;
    int                             length;

    // (world units)
    ax = jo_fixed2int(p->points[1].x - p->points[0].x);
    ay = jo_fixed2int(p->points[1].y - p->points[0].y);
    az = jo_fixed2int(p->points[1].z - p->points[0].z);
    bx = jo_fixed2int(p->points[3].x - p->points[0].x);
    by = jo_fixed2int(p->points[3].y - p->points[0].y);
    bz = jo_fixed2int(p->points[3].z - p->points[0].z);
    nx = ay * bz - az * by;
    ny = az * bx - ax * bz;
    nz = ax * by - ay * bx;
    // (a big portal: a floor. The squares must not overflow)
    while (JO_ABS(nx) > 16384 || JO_ABS(ny) > 16384 || JO_ABS(nz) > 16384)
    {
        nx /= 2;
        ny /= 2;
        nz /= 2;
    }
    length = (int)jo_sqrt((unsigned int)(nx * nx + ny * ny + nz * nz));
    if (length == 0)
        return (false);
    n->x = jo_int2fixed(nx) / length;
    n->y = jo_int2fixed(ny) / length;
    n->z = jo_int2fixed(nz) / length;
    *d = jo_fixed_mult(n->x, p->points[0].x) + jo_fixed_mult(n->y, p->points[0].y) + jo_fixed_mult(n->z, p->points[0].z);
    return (true);
}

/** @brief The mirror of a cell seen from the camera (the first one found), see jo_cells_draw_mirror()
 *  @param cell Cell
 *  @param view Rectangle of the screen where the cell is seen (the whole screen for the cell of the camera, the rectangle
 *  of its door for a cell seen through a door)
 */
static void                         __jo_cell_find_mirror(const int cell, const __jo_cell_view * const view)
{
    const jo_cell                   *c;
    const jo_cell_portal            *p;
    int                             k;
    int                             m;

    m = __jo_cell_mirror_count;
    // (a cell seen through 2 doors: its mirror once)
    if (m >= JO_CELL_MAX_MIRRORS || (m > 0 && __jo_cell_mirror[0] == cell))
        return;
    c = &__jo_cells[cell];
    for (k = 0, p = &__jo_cell_portals[c->first_portal]; k < c->portal_count; ++k, ++p)
    {
        if (p->kind != JO_LEVEL_PORTAL_MIRROR)
            continue;
        if (!__jo_cell_portal_view(p, view, &__jo_cell_mirror_view[m]))
            continue;
        if (!__jo_cell_portal_plane(p, &__jo_cell_mirror_normal[m], &__jo_cell_mirror_distance[m]))
            continue;
        // (a polished floor seen through a door has its reflection too, in the rectangle of the door: without it, the half
        // transparent floor was drawn as it is over nothing, it shone only from inside its room)
        __jo_cell_mirror[m] = cell;
        ++__jo_cell_mirror_count;
        return;
    }
}

/** @brief The point lights compute the changes of their light only on the cells drawn (see jo_3d_point_lights_set_mesh_seen()) */
static void                         __jo_cells_tell_lights(void)
{
    int                             k;

    for (k = 0; k < __jo_cell_count; ++k)
        if (__jo_cells[k].mesh != JO_NULL)
            jo_3d_point_lights_set_mesh_seen(__jo_cells[k].mesh, __jo_cell_drawn[k] != 0);
}

/** @brief The views waiting in jo_cells_prepare() (a cell seen through several portals is visited again when its rectangle
 *  grows) */
#define JO_CELL_PREPARE_STACK               (48)

int                                 jo_cells_prepare(const jo_pos3D_fixed * const camera)
{
    __jo_cell_view                  stack[JO_CELL_PREPARE_STACK];
    short                           order[JO_CELL_MAX_DRAWN];
    __jo_cell_view                  view;
    __jo_cell_view                  next;
    const jo_cell                   *c;
    const jo_cell_portal            *p;
    int                             count;
    int                             drawn;
    int                             start;
    int                             k;
    bool                            seen;

    jo_memset(__jo_cell_drawn, 0, sizeof(__jo_cell_drawn));
    __jo_cell_mesh_count = 0;
    __jo_cell_depth_bias = jo_3d_get_depth_bias();
    __jo_cell_mirror[0] = -1;
    __jo_cell_mirror[1] = -1;
    __jo_cell_mirror_count = 0;
    __jo_cell_outside = -1;
    // (the cell whose floor is under the camera: under a duct, the camera is in the room, not in the duct)
    start = __jo_cells_find_object(camera->x, camera->y, camera->z);
    // Outside of the cells: the cells with a door to the outside that is seen (the cells without door to the outside: all)
    if (start < 0)
    {
        count = 0;
        for (k = 0; k < __jo_cell_count; ++k)
        {
            c = &__jo_cells[k];
            // (a cell without a door to the outside: drawn)
            __jo_cell_drawn[k] = 1;
            view.left = 0;
            view.top = 0;
            view.right = JO_TV_WIDTH - 1;
            view.bottom = JO_TV_HEIGHT - 1;
            __jo_cell_from_outside[k] = view;
            __jo_cell_through_window[k] = false;
            seen = false;
            for (drawn = 0, p = &__jo_cell_portals[c->first_portal]; drawn < c->portal_count; ++drawn, ++p)
            {
                if ((p->kind != JO_LEVEL_PORTAL_DOOR && p->kind != JO_LEVEL_PORTAL_WINDOW) || p->cell != JO_LEVEL_NO_INDEX)
                    continue;
                // A door (or a window) to the outside: the cell is seen through them (the union of their rectangles), a
                // window only when the camera is near it
                if (!seen)
                    __jo_cell_drawn[k] = 0;
                seen = true;
                if (p->kind == JO_LEVEL_PORTAL_WINDOW && !__jo_cell_is_near_window(p, camera))
                    continue;
                if (__jo_cell_is_in_doorway(p, camera))
                {
                    __jo_cell_drawn[k] = 1;
                    __jo_cell_from_outside[k] = view;
                    __jo_cell_through_window[k] = true;
                    break;
                }
                if (!__jo_cell_portal_view(p, &view, &next))
                    continue;
                if (p->kind == JO_LEVEL_PORTAL_WINDOW)
                    __jo_cell_through_window[k] = true;
                if (__jo_cell_drawn[k])
                {
                    __jo_cell_from_outside[k].left = JO_MIN(__jo_cell_from_outside[k].left, next.left);
                    __jo_cell_from_outside[k].top = JO_MIN(__jo_cell_from_outside[k].top, next.top);
                    __jo_cell_from_outside[k].right = JO_MAX(__jo_cell_from_outside[k].right, next.right);
                    __jo_cell_from_outside[k].bottom = JO_MAX(__jo_cell_from_outside[k].bottom, next.bottom);
                }
                else
                {
                    __jo_cell_drawn[k] = 1;
                    __jo_cell_from_outside[k] = next;
                }
            }
            if (__jo_cell_drawn[k])
            {
                __jo_cell_add_mesh(c, 0, JO_NULL);
                ++count;
            }
        }
        __jo_cells_tell_lights();
        return (count);
    }
    __jo_cell_outside = 0;
    // The cell of the camera, then the cells seen through the portals. A cell seen through several portals (a big room
    // seen through the openings of a walkway, of stairs and of a landing) is seen in all their rectangles: the first one
    // found was used, the room was cut to the rectangle of the smallest opening. Its rectangle grows (the union), the
    // cells behind it are visited again; its model is added at the end, with its whole rectangle
    stack[0].cell = (short)start;
    stack[0].level = 0;
    stack[0].left = 0;
    stack[0].top = 0;
    stack[0].right = JO_TV_WIDTH - 1;
    stack[0].bottom = JO_TV_HEIGHT - 1;
    count = 1;
    drawn = 0;
    while (count > 0)
    {
        view = stack[--count];
        if (__jo_cell_drawn[view.cell])
        {
            __jo_cell_view          *known;

            known = &__jo_cell_from_outside[view.cell];
            if (view.left >= known->left && view.top >= known->top && view.right <= known->right && view.bottom <= known->bottom)
                continue;
            known->left = JO_MIN(known->left, view.left);
            known->top = JO_MIN(known->top, view.top);
            known->right = JO_MAX(known->right, view.right);
            known->bottom = JO_MAX(known->bottom, view.bottom);
            view.left = known->left;
            view.top = known->top;
            view.right = known->right;
            view.bottom = known->bottom;
            view.level = known->level;
        }
        else
        {
            if (drawn >= JO_CELL_MAX_DRAWN)
                continue;
            // (1 + the portals from the cell of the camera: see jo_cells_depth_bias_at())
            __jo_cell_drawn[view.cell] = (unsigned char)(1 + JO_MIN(view.level, 15));
            __jo_cell_from_outside[view.cell] = view;
            order[drawn++] = view.cell;
        }
        c = &__jo_cells[view.cell];
        for (k = 0, p = &__jo_cell_portals[c->first_portal]; k < c->portal_count; ++k, ++p)
        {
            // (a closed door: nothing is seen through it)
            if (__jo_cell_portal_closed[c->first_portal + k])
                continue;
            // A door (or a window) to the outside: the rectangle where the outside is seen grows (the camera in the
            // doorway: the portal is cut by the near plane, the outside is seen in the whole rectangle)
            if ((p->kind == JO_LEVEL_PORTAL_DOOR || p->kind == JO_LEVEL_PORTAL_WINDOW) && p->cell == JO_LEVEL_NO_INDEX)
            {
                if (__jo_cell_is_in_doorway(p, camera))
                    next = view;
                if (__jo_cell_is_in_doorway(p, camera) || __jo_cell_portal_view(p, &view, &next))
                {
                    if (__jo_cell_outside == 0)
                        __jo_cell_outside_view = next;
                    else
                    {
                        __jo_cell_outside_view.left = JO_MIN(__jo_cell_outside_view.left, next.left);
                        __jo_cell_outside_view.top = JO_MIN(__jo_cell_outside_view.top, next.top);
                        __jo_cell_outside_view.right = JO_MAX(__jo_cell_outside_view.right, next.right);
                        __jo_cell_outside_view.bottom = JO_MAX(__jo_cell_outside_view.bottom, next.bottom);
                    }
                    __jo_cell_outside = 1;
                }
                continue;
            }
            // (the cell of the camera is seen whole: no way back to it)
            if (p->kind != JO_LEVEL_PORTAL_DOOR || p->cell >= __jo_cell_count || p->cell == start || count >= JO_CELL_PREPARE_STACK)
                continue;
            // The camera in the doorway: the portal is cut by the near plane, the next cell is seen in the whole rectangle
            if (__jo_cell_is_in_doorway(p, camera))
                next = view;
            else if (!__jo_cell_portal_view(p, &view, &next))
                continue;
            next.cell = (short)p->cell;
            next.level = (short)(view.level + 1);
            stack[count++] = next;
        }
    }
    // The models of the cells (their whole rectangles), the mirrors of the first cells found (the cell of the camera first)
    for (k = 0; k < drawn; ++k)
    {
        c = &__jo_cells[order[k]];
        __jo_cell_add_mesh(c, __jo_cell_from_outside[order[k]].level, &__jo_cell_from_outside[order[k]]);
        __jo_cell_find_mirror(order[k], &__jo_cell_from_outside[order[k]]);
    }
    __jo_cells_tell_lights();
    return (drawn);
}

void                                jo_cells_draw_prepared(const bool slave)
{
    jo_cells_draw_prepared_part(slave, -1, 0);
}

void                                jo_cells_draw_prepared_part(const bool slave, const int part, const int handicap)
{
    int                             k;
    int                             load[2];
    int                             bin;

    load[0] = 0;
    load[1] = handicap;
    for (k = 0; k < __jo_cell_mesh_count; ++k)
    {
        // (the same split on both CPUs: each mesh to the part with fewer polygons)
        bin = load[0] <= load[1] ? 0 : 1;
        load[bin] += (int)__jo_cell_meshes[k]->data.polygon_count;
        if (part >= 0 && bin != part)
            continue;
        // The cells seen through more portals: behind (see JO_CELL_LEVEL_BIAS)
        if (__jo_cell_mesh_range_count[k] == 0)
            continue;
        if (slave)
        {
            jo_3d_slave_set_depth_bias(__jo_cell_depth_bias + __jo_cell_mesh_levels[k] * JO_CELL_LEVEL_BIAS);
            if (__jo_cell_mesh_range_count[k] < 0)
                jo_3d_slave_mesh_draw(__jo_cell_meshes[k]);
            else
                jo_3d_slave_mesh_draw_ranges(__jo_cell_meshes[k], __jo_cell_meshes[k]->data.point_count, __jo_cell_mesh_ranges[k],
                                             __jo_cell_mesh_range_count[k]);
        }
        else
        {
            jo_3d_set_depth_bias(__jo_cell_depth_bias + __jo_cell_mesh_levels[k] * JO_CELL_LEVEL_BIAS);
            if (__jo_cell_mesh_range_count[k] < 0)
                jo_3d_mesh_draw(__jo_cell_meshes[k]);
            else
                jo_3d_mesh_draw_ranges(__jo_cell_meshes[k], __jo_cell_meshes[k]->data.point_count, __jo_cell_mesh_ranges[k],
                                       __jo_cell_mesh_range_count[k]);
        }
    }
    if (slave)
        jo_3d_slave_set_depth_bias(__jo_cell_depth_bias);
    else
        jo_3d_set_depth_bias(__jo_cell_depth_bias);
}

void                                jo_cells_draw_prepared_share(const bool slave, const int part, const int share)
{
    jo_3d_polygon_range             range;
    jo_3d_mesh                      *mesh;
    int                             total;
    int                             cut;
    int                             first;
    int                             n;
    int                             k;

    for (k = 0, total = 0; k < __jo_cell_mesh_count; ++k)
        total += (int)__jo_cell_meshes[k]->data.polygon_count;
    // (the same cut on both CPUs: the polygons before it are the part 0)
    cut = total * JO_MAX(0, JO_MIN(share, 1024)) / 1024;
    for (k = 0, first = 0; k < __jo_cell_mesh_count; ++k, first += n)
    {
        mesh = __jo_cell_meshes[k];
        n = (int)mesh->data.polygon_count;
        // The polygons of the mesh in the part: from range.first, range.count
        if (part == 0)
        {
            range.first = 0;
            range.count = (unsigned short)JO_MAX(0, JO_MIN(n, cut - first));
        }
        else
        {
            range.first = (unsigned short)JO_MAX(0, JO_MIN(n, cut - first));
            range.count = (unsigned short)(n - range.first);
        }
        // (a polygon sorted with the previous one (a decal) stays in the part of the previous one)
        if (part == 0 && range.count > 0)
            while (range.first + range.count < n && (mesh->data.attributes[range.first + range.count].sort & JO_3D_SORT_MASK) == JO_3D_SORT_PREVIOUS)
                ++range.count;
        if (part == 1 && range.first > 0)
            while (range.count > 0 && (mesh->data.attributes[range.first].sort & JO_3D_SORT_MASK) == JO_3D_SORT_PREVIOUS)
            {
                ++range.first;
                --range.count;
            }
        if (range.count == 0 || __jo_cell_mesh_range_count[k] == 0)
            continue;
        if (__jo_cell_mesh_range_count[k] > 0)
        {
            // The chunks seen (see __jo_cell_visible_ranges()) in the part
            jo_3d_polygon_range     clipped[JO_CELL_MAX_RANGES];
            int                     c;
            int                     j;
            int                     a;
            int                     b;

            for (j = 0, c = 0; j < __jo_cell_mesh_range_count[k]; ++j)
            {
                a = JO_MAX((int)__jo_cell_mesh_ranges[k][j].first, (int)range.first);
                b = JO_MIN((int)__jo_cell_mesh_ranges[k][j].first + (int)__jo_cell_mesh_ranges[k][j].count, (int)range.first + (int)range.count);
                if (b <= a)
                    continue;
                clipped[c].first = (unsigned short)a;
                clipped[c].count = (unsigned short)(b - a);
                ++c;
            }
            if (c == 0)
                continue;
            if (slave)
            {
                jo_3d_slave_set_depth_bias(__jo_cell_depth_bias + __jo_cell_mesh_levels[k] * JO_CELL_LEVEL_BIAS);
                jo_3d_slave_mesh_draw_ranges(mesh, mesh->data.point_count, clipped, c);
            }
            else
            {
                jo_3d_set_depth_bias(__jo_cell_depth_bias + __jo_cell_mesh_levels[k] * JO_CELL_LEVEL_BIAS);
                jo_3d_mesh_draw_ranges(mesh, mesh->data.point_count, clipped, c);
            }
            continue;
        }
        if (slave)
        {
            jo_3d_slave_set_depth_bias(__jo_cell_depth_bias + __jo_cell_mesh_levels[k] * JO_CELL_LEVEL_BIAS);
            if (range.count == n)
                jo_3d_slave_mesh_draw(mesh);
            else
                jo_3d_slave_mesh_draw_ranges(mesh, mesh->data.point_count, &range, 1);
        }
        else
        {
            jo_3d_set_depth_bias(__jo_cell_depth_bias + __jo_cell_mesh_levels[k] * JO_CELL_LEVEL_BIAS);
            if (range.count == n)
                jo_3d_mesh_draw(mesh);
            else
                jo_3d_mesh_draw_ranges(mesh, mesh->data.point_count, &range, 1);
        }
    }
    if (slave)
        jo_3d_slave_set_depth_bias(__jo_cell_depth_bias);
    else
        jo_3d_set_depth_bias(__jo_cell_depth_bias);
}

/** @brief Share of the polygons of the cells drawn by the master CPU (0 to 1024), its limits (see jo_cells_balance_share()) */
static int                          __jo_cell_share = JO_CELLS_DEFAULT_SHARE;
static int                          __jo_cell_share_min = 0;
static int                          __jo_cell_share_max = JO_CELLS_DEFAULT_MAX_SHARE;

#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT

/** @brief The job of the slave CPU: its part of the cells */
static void                         __jo_cells_slave_part(void)
{
    // (written by the master CPU before the job: read without the cache)
    jo_cells_draw_prepared_share(true, 1, *(volatile int *)JO_CACHE_THROUGH(&__jo_cell_share));
}

#endif

void                                jo_cells_draw_prepared_on_both_cpus(void)
{
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
    jo_core_exec_on_slave(__jo_cells_slave_part);
    jo_cells_draw_prepared_share(false, 0, __jo_cell_share);
#else
    jo_cells_draw_prepared(false);
#endif
}

void                                jo_cells_balance_share(void)
{
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
    int                             step;

    // (1 / 1024 of the polygons per 100 microseconds the slave CPU waited, 40 / 1024 at most per frame)
    step = JO_MAX(-40, JO_MIN(40, jo_core_slave_idle_time() / 100));
    if (step > 0 && jo_3d_slave_get_command_count() > jo_3d_slave_get_max_commands() * 3 / 4)
        step = -16;
    __jo_cell_share = JO_MAX(__jo_cell_share_min, JO_MIN(__jo_cell_share_max, __jo_cell_share - step));
#endif
}

void                                jo_cells_set_share_limits(const int min, const int max)
{
    __jo_cell_share_min = JO_MAX(0, JO_MIN(min, 1024));
    __jo_cell_share_max = JO_MAX(__jo_cell_share_min, JO_MIN(max, 1024));
    __jo_cell_share = JO_MAX(__jo_cell_share_min, JO_MIN(__jo_cell_share_max, __jo_cell_share));
}

int                                 jo_cells_get_share(void)
{
    return (__jo_cell_share);
}

int                                 jo_cells_draw(const jo_pos3D_fixed * const camera)
{
    int                             count;

    count = jo_cells_prepare(camera);
    jo_cells_draw_prepared(false);
    jo_cells_draw_mirror(JO_NULL);
    return (count);
}

/** @brief The backs of the mirrors in the walls of the cells drawn, but the mirror reflected (see __jo_cell_mirror_backs) */
static void                         __jo_cells_draw_mirror_backs(void)
{
    const jo_cell_portal            *p;
    int                             bias;
    int                             k;
    int                             j;

    bias = jo_3d_get_depth_bias();
    // (behind the walls around them: they don't hide the reflection either, it is drawn over them)
    jo_3d_set_depth_bias(bias + JO_CELL_MIRROR_DEPTH_BIAS);
    for (k = 0; k < __jo_cell_count; ++k)
    {
        if (!__jo_cell_drawn[k] || k == __jo_cell_mirror[0] || k == __jo_cell_mirror[1])
            continue;
        for (j = 0, p = &__jo_cell_portals[__jo_cells[k].first_portal]; j < __jo_cells[k].portal_count; ++j, ++p)
            if (__jo_cell_mirror_backs[__jo_cells[k].first_portal + j] != JO_NULL)
                jo_3d_mesh_draw(__jo_cell_mirror_backs[__jo_cells[k].first_portal + j]);
    }
    jo_3d_set_depth_bias(bias);
}

/** @brief Draw a mirror seen from the camera: the room reflected, in a far window clipped to the mirror
 *  @param m Index of the mirror (see __jo_cell_mirror)
 *  @param draw_more See jo_cells_draw_mirror()
 */
static void                         __jo_cells_draw_one_mirror(const int m, void (*draw_more)(void))
{
    const jo_cell                   *c;
    const jo_cell                   *n;
    const jo_cell_portal            *p;
    const jo_cell_portal            *q;
    const __jo_cell_view            *v;
    int                             k;
    int                             j;
    int                             bias;

    c = &__jo_cells[__jo_cell_mirror[m]];
    v = &__jo_cell_mirror_view[m];
    __jo_cell_mirror_drawn = m;
    // A far window (one per mirror): the rectangle of the mirror on the screen, the VDP1 clips the polygons to it
    jo_3d_select_window(m == 0 ? JO_3D_FAR_WINDOW : JO_3D_FAR_WINDOW_2);
    jo_3d_window(v->left, v->top, v->right, v->bottom, JO_CELL_MIRROR_FAR, JO_TV_WIDTH_2, JO_TV_HEIGHT_2);
    jo_3d_window_clip(true);
    // The camera reflected by the plane of the mirror: the room is behind the mirror (farther: sorted behind the wall
    // around it, which has a hole for the mirror)
    bias = jo_3d_get_depth_bias();
    jo_3d_set_depth_bias(bias + JO_CELL_MIRROR_DEPTH_BIAS);
    jo_3d_push_matrix();
    {
        jo_3d_reflect_matrix(&__jo_cell_mirror_normal[m], __jo_cell_mirror_distance[m]);
        jo_3d_set_mirror(true);
        // A simpler model of the cell for its mirrors: alone (the cells next to it are not seen in a polished floor)
        if (c->reflection != JO_NULL)
            jo_3d_mesh_draw(c->reflection);
        else if (c->mesh != JO_NULL)
            jo_3d_mesh_draw(c->mesh);
        // The cells next to it (seen through its doors in the mirror, else the background is seen there). The doors are
        // small cells without a mesh: the cells on their other side
        for (k = 0, p = &__jo_cell_portals[c->first_portal]; c->reflection == JO_NULL && k < c->portal_count; ++k, ++p)
        {
            if (p->kind != JO_LEVEL_PORTAL_DOOR || p->cell >= __jo_cell_count)
                continue;
            n = &__jo_cells[p->cell];
            if (n->mesh != JO_NULL)
                jo_3d_mesh_draw(n->mesh);
            for (j = 0, q = &__jo_cell_portals[n->first_portal]; n->mesh == JO_NULL && j < n->portal_count; ++j, ++q)
                if (q->kind == JO_LEVEL_PORTAL_DOOR && q->cell < __jo_cell_count && q->cell != __jo_cell_mirror[m] &&
                    __jo_cells[q->cell].mesh != JO_NULL)
                    jo_3d_mesh_draw(__jo_cells[q->cell].mesh);
        }
        if (draw_more != JO_NULL)
            draw_more();
        jo_3d_set_mirror(false);
    }
    jo_3d_pop_matrix();
    jo_3d_set_depth_bias(bias);
    jo_3d_window_clip(false);
    jo_3d_select_window(JO_3D_NEAR_WINDOW);
}

bool                                jo_cells_draw_mirror(void (*draw_more)(void))
{
    int                             m;

    __jo_cells_draw_mirror_backs();
    for (m = 0; m < __jo_cell_mirror_count; ++m)
        __jo_cells_draw_one_mirror(m, draw_more);
    return (__jo_cell_mirror_count > 0);
}

bool                                jo_cells_mirror_is_floor(void)
{
    // (the mirror drawn now, see jo_cells_draw_mirror(). The normal is vertical: a floor, or a ceiling)
    return (__jo_cell_mirror_count > 0 && JO_ABS(__jo_cell_mirror_normal[__jo_cell_mirror_drawn].y) > JO_FIXED_1 / 2);
}

bool                                jo_cells_seen_through(const int index, jo_3d_rect * const rect)
{
    if (__jo_cell_outside >= 0 || index < 0 || index >= __jo_cell_count || !__jo_cell_drawn[index])
        return (false);
    rect->left = __jo_cell_from_outside[index].left;
    rect->top = __jo_cell_from_outside[index].top;
    rect->right = __jo_cell_from_outside[index].right;
    rect->bottom = __jo_cell_from_outside[index].bottom;
    return (true);
}

bool                                jo_cells_view_rect_at(const jo_pos3D_fixed * const position, jo_3d_rect * const rect)
{
    int                             cell;

    rect->left = 0;
    rect->top = 0;
    rect->right = JO_TV_WIDTH - 1;
    rect->bottom = JO_TV_HEIGHT - 1;
    if (__jo_cell_count == 0)
        return (true);
    cell = __jo_cells_find_object(position->x, position->y, position->z);
    // Outside of the cells: seen from the outside (the whole screen), or through the doors of the cell of the camera
    if (cell < 0)
        return (jo_cells_outside_view(rect) != 0);
    if (!__jo_cell_drawn[cell])
        return (false);
    rect->left = __jo_cell_from_outside[cell].left;
    rect->top = __jo_cell_from_outside[cell].top;
    rect->right = __jo_cell_from_outside[cell].right;
    rect->bottom = __jo_cell_from_outside[cell].bottom;
    return (true);
}

int                                 jo_cells_outside_view(jo_3d_rect * const rect)
{
    if (__jo_cell_outside == 1)
    {
        rect->left = __jo_cell_outside_view.left;
        rect->top = __jo_cell_outside_view.top;
        rect->right = __jo_cell_outside_view.right;
        rect->bottom = __jo_cell_outside_view.bottom;
    }
    return (__jo_cell_outside);
}

bool                                jo_cells_seen_through_window(const int index)
{
    return (__jo_cell_outside < 0 && index >= 0 && index < __jo_cell_count && __jo_cell_drawn[index] &&
            __jo_cell_through_window[index]);
}

int                                 jo_cells_close_portals(const jo_fixed x, const jo_fixed z, const int radius, const bool closed)
{
    const jo_cell_portal            *p;
    int                             k;
    int                             n;
    int                             cx;
    int                             cz;

    for (k = 0, n = 0, p = __jo_cell_portals; k < __jo_cell_portal_count; ++k, ++p)
    {
        if (p->kind != JO_LEVEL_PORTAL_DOOR)
            continue;
        // (the middle of the portal on the ground)
        cx = jo_fixed2int(JO_DIV_BY_4(p->points[0].x) + JO_DIV_BY_4(p->points[1].x) + JO_DIV_BY_4(p->points[2].x) + JO_DIV_BY_4(p->points[3].x));
        cz = jo_fixed2int(JO_DIV_BY_4(p->points[0].z) + JO_DIV_BY_4(p->points[1].z) + JO_DIV_BY_4(p->points[2].z) + JO_DIV_BY_4(p->points[3].z));
        if (JO_ABS(cx - jo_fixed2int(x)) > radius || JO_ABS(cz - jo_fixed2int(z)) > radius)
            continue;
        __jo_cell_portal_closed[k] = closed ? 1 : 0;
        ++n;
    }
    return (n);
}

bool                                jo_cells_is_drawn(const int index)
{
    return (index < 0 || index >= __jo_cell_count || __jo_cell_drawn[index] != 0);
}

unsigned short                      jo_cells_generation(void)
{
    return (__jo_cell_generation);
}

int                                 jo_cells_object_cell(const jo_fixed x, const jo_fixed y, const jo_fixed z)
{
    if (__jo_cell_count == 0)
        return (-1);
    return (__jo_cells_find_object(x, y, z));
}

int                                 jo_cells_depth_bias_of(const int cell)
{
    if (cell < 0 || cell >= __jo_cell_count)
        return (0);
    if (__jo_cell_drawn[cell] == 0)
        return (-1);
    return ((__jo_cell_drawn[cell] - 1) * JO_CELL_LEVEL_BIAS);
}

int                                 jo_cells_depth_bias_at(const jo_fixed x, const jo_fixed y, const jo_fixed z)
{
    return (jo_cells_depth_bias_of(jo_cells_object_cell(x, y, z)));
}

bool                                jo_cells_ray_hit(const jo_pos3D_fixed * const origin, const jo_pos3D_fixed * const dir, const jo_fixed max_t,
                                                     jo_fixed * const t, jo_pos3D_fixed * const normal)
{
    int                             k;
    jo_pos3D_fixed                  oc;
    jo_fixed                        along;
    jo_fixed                        best;
    long long                       miss;
    bool                            hit;
    const jo_cell                   *c;
    const short                     *box;
    jo_pos3D_fixed                  end;
    int                             lo[3];
    int                             hi[3];
    int                             lx;
    int                             lz;
    int                             line_o;
    int                             hi_x;
    int                             hi_z;

    best = max_t;
    hit = false;
    // The line of the ray seen from above: its normal (lx, lz) Q12, the cells whose box is all on one side of it, farther than
    // 8 units, are skipped (see jo_3d_mesh_ray_hit_with_planes(): the same test with the boxes of its polygons)
    lx = -(dir->z >> 4);
    lz = dir->x >> 4;
    line_o = lx * jo_fixed2int(origin->x) + lz * jo_fixed2int(origin->z);
    hi_x = lx > 0 ? 3 : 0;
    hi_z = lz > 0 ? 5 : 2;
    // The box of the segment (world units): the cells whose box doesn't meet it are not tested
    end.x = origin->x + jo_fixed_mult(dir->x, max_t);
    end.y = origin->y + jo_fixed_mult(dir->y, max_t);
    end.z = origin->z + jo_fixed_mult(dir->z, max_t);
    lo[0] = jo_fixed2int(JO_MIN(origin->x, end.x)) - 1;
    lo[1] = jo_fixed2int(JO_MIN(origin->y, end.y)) - 1;
    lo[2] = jo_fixed2int(JO_MIN(origin->z, end.z)) - 1;
    hi[0] = jo_fixed2int(JO_MAX(origin->x, end.x)) + 1;
    hi[1] = jo_fixed2int(JO_MAX(origin->y, end.y)) + 1;
    hi[2] = jo_fixed2int(JO_MAX(origin->z, end.z)) + 1;
    for (k = 0, c = __jo_cells; k < __jo_cell_count; ++k, ++c)
    {
        if (c->mesh == JO_NULL)
            continue;
        box = __jo_cell_box[k];
        if (hi[0] < box[0] || hi[1] < box[1] || hi[2] < box[2] || lo[0] > box[3] || lo[1] > box[4] || lo[2] > box[5] ||
            lx * box[hi_x] + lz * box[hi_z] - line_o < -8 * 4096 || lx * box[3 - hi_x] + lz * box[7 - hi_z] - line_o > 8 * 4096)
            continue;
        // The ray must go through the bounding sphere (world units: the squares fit in 64 bits)
        oc.x = jo_fixed2int(c->center.x - origin->x);
        oc.y = jo_fixed2int(c->center.y - origin->y);
        oc.z = jo_fixed2int(c->center.z - origin->z);
        along = (jo_fixed)(((long long)oc.x * dir->x + (long long)oc.y * dir->y + (long long)oc.z * dir->z) >> 16);
        miss = (long long)oc.x * oc.x + (long long)oc.y * oc.y + (long long)oc.z * oc.z - (long long)along * along;
        if (miss > (long long)c->radius * c->radius || along + c->radius < 0 || jo_int2fixed(along - c->radius) > best)
            continue;
        if (jo_3d_mesh_ray_hit_with_planes(c->mesh, __jo_cell_ray_planes[k], origin, dir, &best, normal))
            hit = true;
    }
    *t = best;
    return (hit);
}

/** @brief A wall of the cells between 2 points (the shadows of the point lights) */
static bool                         __jo_cells_light_hit(const jo_pos3D_fixed * const origin, const jo_pos3D_fixed * const dir, const jo_fixed max_t)
{
    jo_pos3D_fixed                  normal;
    jo_fixed                        t;

    return (jo_cells_ray_hit(origin, dir, max_t, &t, &normal));
}

int                                 jo_cells_add_point_light_meshes(const bool shadows)
{
    int                             k;
    int                             count;

    count = 0;
    for (k = 0; k < __jo_cell_count; ++k)
        if (__jo_cells[k].mesh != JO_NULL && jo_3d_point_lights_add_mesh(__jo_cells[k].mesh))
            ++count;
    jo_3d_point_lights_set_occlusion(shadows ? __jo_cells_light_hit : JO_NULL);
    return (count);
}

#endif /* !JO_COMPILE_WITH_LEVEL_SUPPORT && JO_COMPILE_WITH_3D_SUPPORT */

/*
** END OF FILE
*/
