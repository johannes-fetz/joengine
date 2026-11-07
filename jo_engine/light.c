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
** Point lights: see jo/light.h
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
#include "jo/math.h"
#include "jo/list.h"
#include "jo/3d.h"
#include "jo/light.h"
#include "internal/3d.h"

#ifdef JO_COMPILE_WITH_3D_SUPPORT

/** @brief The light stops at this many radiuses */
#define JO_POINT_LIGHT_RANGE                (4)
/** @brief The corners nearer than this to a light are not in its shadow (world units): the model of its lamp */
#define JO_POINT_LIGHT_LAMP_SIZE            (12)
/** @brief Soft shadows: the light comes from 5 points (the light and 4 points around it at this distance, world units):
 *  a corner sees some of them at the edge of a shadow (a shadow made of whole quads showed their edges) */
#define JO_POINT_LIGHT_SOFT_SIZE            (28)
#define JO_POINT_LIGHT_SOFT_RAYS            (5)
/** @brief The rays of the shadows (JO_POINT_LIGHT_SOFT_RAYS, 1 for jo_3d_point_lights_sample(): each frame) */
static int                          __jo_point_light_rays = JO_POINT_LIGHT_SOFT_RAYS;

/** @brief A polygon lit by at least one light: its gouraud table, its mesh, its colors as read, the sum of the lights
 *  (gouraud units x 65536 for each component, the lights at the intensity applied to its mesh, see jo_point_light.applied:
 *  the light of a light is linear in its intensity, its change is added exactly) */
typedef struct
{
    unsigned short                  table;
    unsigned char                   mesh;
    jo_color                        colors[4];
    int                             sum[4][3];
}                                   jo_point_light_polygon;

/** @brief The light of a light on a polygon (gouraud units x 64 at the intensity 256, color white) */
typedef struct
{
    unsigned short                  polygon;        // Index in __jo_point_light_polygons
    unsigned short                  gain[4];
}                                   jo_point_light_entry;

typedef struct
{
    bool                            used;
    bool                            enabled;
    bool                            computed;       // The entries are up to date with the position
    bool                            partial;        // Without shadows: meshes in its reach skipped (not seen), computed when seen
    jo_pos3D_fixed                  position;
    int                             r;
    int                             g;
    int                             b;
    int                             radius;
    int                             strength;
    int                             intensity;
    bool                            spot;           // A cone (else all around)
    bool                            shadows;        // The walls stop its light (see jo_3d_point_lights_set_occlusion())
    bool                            unsampled;      // Not seen by jo_3d_point_lights_sample() (see jo_3d_point_light_set_sampled())
    int                             direction[3];   // Of the cone (unit vector x 256)
    int                             cos_inner;      // Cosine of the half angle of the full light, of the cone (x 256)
    int                             cos_outer;
    jo_point_light_entry            *entries;       // By mesh (see mesh_first)
    int                             entry_count;
    unsigned short                  mesh_first[JO_MAX_POINT_LIGHT_MESHES];      // The entries of each mesh
    unsigned short                  mesh_entries[JO_MAX_POINT_LIGHT_MESHES];
    short                           applied[JO_MAX_POINT_LIGHT_MESHES];         // Intensity in the sums of the polygons of each mesh
}                                   jo_point_light;

static jo_point_light               __jo_point_lights[JO_MAX_POINT_LIGHTS];
static const jo_3d_mesh             *__jo_point_light_meshes[JO_MAX_POINT_LIGHT_MESHES];
/** @brief Bounding spheres of the meshes (world units): the meshes far from a light are not checked */
static jo_pos3D_fixed               __jo_point_light_mesh_center[JO_MAX_POINT_LIGHT_MESHES];
static int                          __jo_point_light_mesh_radius[JO_MAX_POINT_LIGHT_MESHES];
/** @brief The first polygon of each mesh in __jo_point_light_polygon_of (index of its polygon, 0xffff: not lit yet) */
static unsigned short               *__jo_point_light_polygon_of[JO_MAX_POINT_LIGHT_MESHES];
static int                          __jo_point_light_mesh_count = 0;
/** @brief The meshes seen (see jo_3d_point_lights_set_mesh_seen()): the light of the others is applied when they are seen */
static bool                         __jo_point_light_mesh_seen[JO_MAX_POINT_LIGHT_MESHES];
/** @brief The gouraud tables of each mesh (one block: its first table, their number), a copy in the work RAM written by
 *  the CPU and copied into the VRAM by the DMA controller (the CPU waits a long time for the VRAM while the VDP1 draws),
 *  the tables changed in this update (from dirty_first to dirty_last) */
static unsigned short               __jo_point_light_mesh_table_first[JO_MAX_POINT_LIGHT_MESHES];
static unsigned short               __jo_point_light_mesh_table_count[JO_MAX_POINT_LIGHT_MESHES];
static jo_color                     *__jo_point_light_mesh_tables[JO_MAX_POINT_LIGHT_MESHES];
static int                          __jo_point_light_mesh_dirty_first[JO_MAX_POINT_LIGHT_MESHES];
static int                          __jo_point_light_mesh_dirty_last[JO_MAX_POINT_LIGHT_MESHES];
static jo_point_light_polygon       *__jo_point_light_polygons = JO_NULL;
static int                          __jo_point_light_polygon_count = 0;
static int                          __jo_point_light_polygon_capacity = 0;
static bool                         (*__jo_point_light_hit)(const jo_pos3D_fixed * const, const jo_pos3D_fixed * const, const jo_fixed) = JO_NULL;
static bool                         __jo_point_lights_on = true;

/** @brief Cache of the light of jo_3d_point_lights_sample() (before the intensity: the rays of the shadows are costly; an
 *  object that doesn't move gets it again): direct mapped, emptied when a light or a mesh changes */
#define JO_POINT_LIGHT_SAMPLE_CACHE         (128)
typedef struct
{
    unsigned int                    generation;     // __jo_point_light_generation when computed (0: empty)
    int                             light;
    jo_pos3D_fixed                  position;
    int                             gain;
    unsigned char                   recent;         // In the first entry of an index: the entry used last (0 or 1)
}                                   jo_point_light_sample_entry;
static jo_point_light_sample_entry  __jo_point_light_samples[JO_POINT_LIGHT_SAMPLE_CACHE];
static unsigned int                 __jo_point_light_generation = 1;

/** @brief Check if an entry of the cache of jo_3d_point_lights_sample() has the light of a place */
static  __jo_force_inline bool      jo_point_light_sample_hit(const jo_point_light_sample_entry * const e, const int light,
                                                              const jo_pos3D_fixed * const position)
{
    return (e->generation == __jo_point_light_generation && e->light == light && e->position.x == position->x &&
            e->position.y == position->y && e->position.z == position->z);
}

/** @brief The lights or the meshes changed: the cache of jo_3d_point_lights_sample() is not valid anymore */
static  __jo_force_inline void      jo_point_light_changed(void)
{
    ++__jo_point_light_generation;
}

/** @brief Intensity of a light now (0: off) */
static int                          jo_point_light_intensity(const jo_point_light * const l)
{
    return (__jo_point_lights_on && l->used && l->enabled ? l->intensity : 0);
}

/** @brief Write the colors of a polygon (its colors plus its lights) in the copy of the tables of its mesh */
static void                         jo_point_light_write_polygon(const jo_point_light_polygon * const p)
{
    jo_color                        *table;
    int                             index;
    int                             j;
    int                             c;
    int                             r;
    int                             g;
    int                             b;

    index = p->table - __jo_point_light_mesh_table_first[p->mesh];
    table = &__jo_point_light_mesh_tables[p->mesh][index * 4];
    for (j = 0; j < 4; ++j)
    {
        c = p->colors[j];
        // (the sums are positive: logical shifts)
        r = (c & 31) + (int)((unsigned int)p->sum[j][0] >> 16);
        g = ((c >> 5) & 31) + (int)((unsigned int)p->sum[j][1] >> 16);
        b = ((c >> 10) & 31) + (int)((unsigned int)p->sum[j][2] >> 16);
        table[j] = (jo_color)(0x8000 | (JO_MIN(b, 31) << 10) | (JO_MIN(g, 31) << 5) | JO_MIN(r, 31));
    }
    if (index < __jo_point_light_mesh_dirty_first[p->mesh])
        __jo_point_light_mesh_dirty_first[p->mesh] = index;
    if (index > __jo_point_light_mesh_dirty_last[p->mesh])
        __jo_point_light_mesh_dirty_last[p->mesh] = index;
}

/** @brief The light of a light on the polygons of a mesh at another intensity: the difference is added to their sums (the
 *  light of a corner is the gain x the color / 64 (rounded down) x the intensity: linear in the intensity, the sums stay
 *  exact)
 *  @param l Light
 *  @param m Mesh
 *  @param intensity Intensity (0: its light is removed)
 */
static void                         jo_point_light_apply(jo_point_light * const l, const int m, const int intensity)
{
    jo_point_light_polygon          *p;
    const jo_point_light_entry      *e;
    int                             before;
    int                             i;
    int                             j;
    int                             delta;
    unsigned int                    gain;
    unsigned int                    r;
    unsigned int                    g;
    unsigned int                    b;

    before = l->applied[m];
    if (before == intensity)
        return;
    l->applied[m] = (short)intensity;
    delta = intensity - before;
    r = (unsigned int)l->r;
    g = (unsigned int)l->g;
    b = (unsigned int)l->b;
    for (i = 0, e = &l->entries[l->mesh_first[m]]; i < l->mesh_entries[m]; ++i, ++e)
    {
        p = &__jo_point_light_polygons[e->polygon];
        for (j = 0; j < 4; ++j)
        {
            // (gain: gouraud units x 64 at the intensity 256; x the color / 64, x the intensity: gouraud units x 65536.
            // A logical shift: an arithmetic shift is a call of the library)
            gain = e->gain[j];
            p->sum[j][0] += (int)((gain * r) >> 6) * delta;
            p->sum[j][1] += (int)((gain * g) >> 6) * delta;
            p->sum[j][2] += (int)((gain * b) >> 6) * delta;
        }
        jo_point_light_write_polygon(p);
    }
}

/** @brief Remove the light of a light from all the meshes (before its color or its light on the polygons change) */
static void                         jo_point_light_remove(jo_point_light * const l)
{
    int                             m;

    if (l->entries == JO_NULL)
        return;
    for (m = 0; m < __jo_point_light_mesh_count; ++m)
        jo_point_light_apply(l, m, 0);
}

int                                 jo_3d_point_light_add(const jo_pos3D_fixed * const position, const unsigned char r, const unsigned char g,
                                                          const unsigned char b, const int radius, const int strength)
{
    int                             k;
    jo_point_light                  *l;

    for (k = 0, l = __jo_point_lights; k < JO_MAX_POINT_LIGHTS; ++k, ++l)
        if (!l->used)
        {
            jo_memset(l, 0, sizeof(*l));
            l->used = true;
            l->enabled = true;
            l->position = *position;
            l->r = r;
            l->g = g;
            l->b = b;
            l->radius = JO_MAX(radius, 1);
            l->strength = strength;
            l->intensity = 256;
            l->shadows = true;
            jo_point_light_changed();
            return (k);
        }
    return (-1);
}

static jo_point_light               *jo_point_light_get(const int id)
{
    if (id < 0 || id >= JO_MAX_POINT_LIGHTS || !__jo_point_lights[id].used)
        return (JO_NULL);
    return (&__jo_point_lights[id]);
}

void                                jo_3d_point_light_set_intensity(const int id, const int intensity)
{
    jo_point_light                  *l;

    if ((l = jo_point_light_get(id)) == JO_NULL || l->intensity == intensity)
        return;
    l->intensity = JO_MAX(0, JO_MIN(intensity, 256));
}

void                                jo_3d_point_light_set_color(const int id, const unsigned char r, const unsigned char g, const unsigned char b)
{
    jo_point_light                  *l;

    if ((l = jo_point_light_get(id)) == JO_NULL || (l->r == r && l->g == g && l->b == b))
        return;
    // (the light with the old color is removed: the update adds it with the new one)
    jo_point_light_remove(l);
    l->r = r;
    l->g = g;
    l->b = b;
}

void                                jo_3d_point_light_set_position(const int id, const jo_pos3D_fixed * const position)
{
    jo_point_light                  *l;

    if ((l = jo_point_light_get(id)) == JO_NULL)
        return;
    l->position = *position;
    l->computed = false;
    jo_point_light_changed();
}

void                                jo_3d_point_light_set_spot(const int id, const jo_pos3D_fixed * const direction, const int angle)
{
    jo_point_light                  *l;
    int                             half;

    if ((l = jo_point_light_get(id)) == JO_NULL)
        return;
    l->spot = direction != JO_NULL && angle > 0 && angle < 180;
    if (l->spot)
    {
        l->direction[0] = direction->x >> 8;
        l->direction[1] = direction->y >> 8;
        l->direction[2] = direction->z >> 8;
        half = angle / 2;
        l->cos_outer = jo_fixed2int(jo_cos(half) * 256);
        l->cos_inner = jo_fixed2int(jo_cos(half * 3 / 4) * 256);
        if (l->cos_inner <= l->cos_outer)
            l->cos_inner = l->cos_outer + 1;
    }
    l->computed = false;
    jo_point_light_changed();
}

void                                jo_3d_point_light_set_shadows(const int id, const bool shadows)
{
    jo_point_light                  *l;

    if ((l = jo_point_light_get(id)) == JO_NULL || l->shadows == shadows)
        return;
    l->shadows = shadows;
    l->computed = false;
    jo_point_light_changed();
}

void                                jo_3d_point_light_set_sampled(const int id, const bool sampled)
{
    jo_point_light                  *l;

    if ((l = jo_point_light_get(id)) == JO_NULL)
        return;
    l->unsampled = !sampled;
    jo_point_light_changed();
}

void                                jo_3d_point_light_enable(const int id, const bool enabled)
{
    jo_point_light                  *l;

    if ((l = jo_point_light_get(id)) == JO_NULL || l->enabled == enabled)
        return;
    l->enabled = enabled;
}

void                                jo_3d_point_lights_enable(const bool enabled)
{
    if (__jo_point_lights_on == enabled)
        return;
    __jo_point_lights_on = enabled;
}

void                                jo_3d_point_lights_set_occlusion(bool (*hit)(const jo_pos3D_fixed * const origin, const jo_pos3D_fixed * const dir,
                                                                                 const jo_fixed max_t))
{
    int                             k;

    __jo_point_light_hit = hit;
    jo_point_light_changed();
    for (k = 0; k < JO_MAX_POINT_LIGHTS; ++k)
        __jo_point_lights[k].computed = false;
}

/** @brief A gouraud table per polygon of a mesh (see jo_3d_point_lights_add_mesh()): the polygons that share a table get one each
 *  @return false if out of memory
 */
static bool                         jo_point_light_own_tables(jo_3d_mesh * const mesh)
{
    jo_3d_polygon_attribute         *a;
    unsigned char                   *seen;
    jo_color                        *colors;
    int                             first;
    int                             last;
    int                             used;
    int                             j;
    int                             k;
    bool                            shared;
    unsigned short                  base;

    first = 0xffff;
    last = -1;
    used = 0;
    for (k = 0, a = mesh->data.attributes; k < (int)mesh->data.polygon_count; ++k, ++a)
    {
        if (!(a->draw_mode & JO_VDP1_COLOR_CALC_GOURAUD) || a->gouraud_table == 0)
            continue;
        first = JO_MIN(first, (int)a->gouraud_table);
        last = JO_MAX(last, (int)a->gouraud_table);
        ++used;
    }
    if (used == 0)
        return (true);
    // (more polygons than tables: shared, else a table seen twice)
    shared = used > last - first + 1;
    if (!shared)
    {
        seen = (unsigned char *)jo_malloc(last - first + 1);
        if (seen == JO_NULL)
            return (false);
        jo_memset(seen, 0, last - first + 1);
        for (k = 0, a = mesh->data.attributes; k < (int)mesh->data.polygon_count && !shared; ++k, ++a)
        {
            if (!(a->draw_mode & JO_VDP1_COLOR_CALC_GOURAUD) || a->gouraud_table == 0)
                continue;
            shared = seen[a->gouraud_table - first] != 0;
            seen[a->gouraud_table - first] = 1;
        }
        jo_free(seen);
    }
    if (!shared)
        return (true);
    // The colors of the table of each polygon, in a new block (an odd count: 2 tables per line of the sprite)
    colors = (jo_color *)jo_malloc(((used + 1) & ~1) * 4 * (int)sizeof(jo_color));
    if (colors == JO_NULL)
        return (false);
    for (k = 0, j = 0, a = mesh->data.attributes; k < (int)mesh->data.polygon_count; ++k, ++a)
        if ((a->draw_mode & JO_VDP1_COLOR_CALC_GOURAUD) && a->gouraud_table != 0)
            jo_memcpy(&colors[4 * j++], (const void *)(JO_VDP1_VRAM + JO_MULT_BY_8(a->gouraud_table)), 4 * sizeof(jo_color));
    if (used & 1)
        jo_memset(&colors[4 * used], 0, 4 * sizeof(jo_color));
    base = jo_3d_add_gouraud_tables(colors, (unsigned int)used);
    jo_free(colors);
    if (base == 0)
        return (false);
    for (k = 0, j = 0, a = mesh->data.attributes; k < (int)mesh->data.polygon_count; ++k, ++a)
        if ((a->draw_mode & JO_VDP1_COLOR_CALC_GOURAUD) && a->gouraud_table != 0)
            a->gouraud_table = (unsigned short)(base + j++);
    __jo_3d_forget_tint(&mesh->data);
    return (true);
}

bool                                jo_3d_point_lights_add_mesh(jo_3d_mesh * const mesh)
{
    int                             k;
    int                             m;
    int                             first;
    int                             last;

    if (mesh == JO_NULL || __jo_point_light_mesh_count >= JO_MAX_POINT_LIGHT_MESHES || !jo_point_light_own_tables(mesh))
        return (false);
    __jo_point_light_polygon_of[__jo_point_light_mesh_count] = (unsigned short *)jo_malloc((int)mesh->data.polygon_count * (int)sizeof(unsigned short));
    if (__jo_point_light_polygon_of[__jo_point_light_mesh_count] == JO_NULL)
        return (false);
    for (k = 0; k < (int)mesh->data.polygon_count; ++k)
        __jo_point_light_polygon_of[__jo_point_light_mesh_count][k] = 0xffff;
    // The block of its gouraud tables, copied in the work RAM
    first = 0xffff;
    last = -1;
    for (k = 0; k < (int)mesh->data.polygon_count; ++k)
    {
        if (!(mesh->data.attributes[k].draw_mode & JO_VDP1_COLOR_CALC_GOURAUD) || mesh->data.attributes[k].gouraud_table == 0)
            continue;
        first = JO_MIN(first, (int)mesh->data.attributes[k].gouraud_table);
        last = JO_MAX(last, (int)mesh->data.attributes[k].gouraud_table);
    }
    m = __jo_point_light_mesh_count;
    __jo_point_light_mesh_tables[m] = JO_NULL;
    if (last >= first)
    {
        __jo_point_light_mesh_tables[m] = (jo_color *)jo_malloc((last - first + 1) * 4 * (int)sizeof(jo_color));
        if (__jo_point_light_mesh_tables[m] == JO_NULL)
        {
            jo_free(__jo_point_light_polygon_of[m]);
            return (false);
        }
        jo_memcpy(__jo_point_light_mesh_tables[m], (const void *)(JO_VDP1_VRAM + JO_MULT_BY_8(first)), (unsigned int)((last - first + 1) * 4 * (int)sizeof(jo_color)));
    }
    __jo_point_light_mesh_table_first[m] = (unsigned short)(last >= first ? first : 0);
    __jo_point_light_mesh_table_count[m] = (unsigned short)(last >= first ? last - first + 1 : 0);
    __jo_point_light_mesh_dirty_first[m] = 0x7fffffff;
    __jo_point_light_mesh_dirty_last[m] = -1;
    __jo_point_light_mesh_seen[m] = true;
    __jo_point_light_mesh_radius[m] = jo_3d_mesh_get_bounding_sphere(mesh, &__jo_point_light_mesh_center[m]);
    __jo_point_light_meshes[__jo_point_light_mesh_count++] = mesh;
    jo_point_light_changed();
    for (k = 0; k < JO_MAX_POINT_LIGHTS; ++k)
    {
        __jo_point_lights[k].applied[m] = 0;
        __jo_point_lights[k].mesh_entries[m] = 0;
        __jo_point_lights[k].computed = false;
    }
    return (true);
}

void                                jo_3d_point_lights_set_mesh_seen(const jo_3d_mesh * const mesh, const bool seen)
{
    int                             m;
    int                             k;

    for (m = 0; m < __jo_point_light_mesh_count; ++m)
        if (__jo_point_light_meshes[m] == mesh)
        {
            // (seen again: the lights without shadows that skipped it are computed again (a lamp that doesn't move never
            // lit the rooms not seen at its first update))
            if (seen && !__jo_point_light_mesh_seen[m])
                for (k = 0; k < JO_MAX_POINT_LIGHTS; ++k)
                    if (__jo_point_lights[k].used && __jo_point_lights[k].partial)
                        __jo_point_lights[k].computed = false;
            __jo_point_light_mesh_seen[m] = seen;
            return;
        }
}

/** @brief Unit normal of the plane of 3 points (world, Q16.16): (b - a) x (c - a) */
static void                         jo_point_light_normal(const jo_pos3D_fixed * const a, const jo_pos3D_fixed * const b,
                                                          const jo_pos3D_fixed * const c, int * const n)
{
    int                             u[3];
    int                             v[3];
    int                             w[3];
    int                             length;
    int                             shift;

    // (the sides in fixed point, halved: the whole units only gave a normal 25 degrees off for a side of 1.9 units, the
    // floor normal for sides under 1 unit)
    u[0] = (b->x >> 1) - (a->x >> 1);
    u[1] = (b->y >> 1) - (a->y >> 1);
    u[2] = (b->z >> 1) - (a->z >> 1);
    v[0] = (c->x >> 1) - (a->x >> 1);
    v[1] = (c->y >> 1) - (a->y >> 1);
    v[2] = (c->z >> 1) - (a->z >> 1);
    // (the sides scaled down first, the squares stay in 32 bits)
    for (shift = 0; JO_MAX(JO_MAX(JO_ABS(u[0]), JO_ABS(u[1])), JO_MAX(JO_ABS(u[2]), JO_MAX(JO_ABS(v[0]), JO_MAX(JO_ABS(v[1]), JO_ABS(v[2]))))) > 128; ++shift)
    {
        u[0] >>= 1; u[1] >>= 1; u[2] >>= 1;
        v[0] >>= 1; v[1] >>= 1; v[2] >>= 1;
    }
    w[0] = u[1] * v[2] - u[2] * v[1];
    w[1] = u[2] * v[0] - u[0] * v[2];
    w[2] = u[0] * v[1] - u[1] * v[0];
    length = (int)jo_sqrt((unsigned int)(w[0] * w[0] + w[1] * w[1] + w[2] * w[2]));
    if (length == 0)
    {
        n[0] = 0;
        n[1] = -256;
        n[2] = 0;
        return;
    }
    // (x 256)
    n[0] = w[0] * 256 / length;
    n[1] = w[1] * 256 / length;
    n[2] = w[2] * 256 / length;
}

/** @brief The light of a light on a corner (gouraud units x 64 at the intensity 256), 0: not lit */
static int                          jo_point_light_gain(const jo_point_light * const l, const jo_pos3D_fixed * const p, const int * const n,
                                                        signed char * const seen_cache)
{
    static const signed char        soft[JO_POINT_LIGHT_SOFT_RAYS][2] = {{0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    jo_pos3D_fixed                  d;
    jo_pos3D_fixed                  origin;
    int                             dx;
    int                             dy;
    int                             dz;
    int                             sx;
    int                             sz;
    int                             sd;
    int                             dist;
    int                             facing;
    int                             range;
    int                             cone;
    int                             seen;
    int                             k;
    int                             c;
    int                             q;
    unsigned int                    rr;
    unsigned int                    dd;
    unsigned int                    shift;

    range = l->radius * JO_POINT_LIGHT_RANGE;
    dx = jo_fixed2int(p->x - l->position.x);
    dy = jo_fixed2int(p->y - l->position.y);
    dz = jo_fixed2int(p->z - l->position.z);
    if (JO_ABS(dx) > range || JO_ABS(dy) > range || JO_ABS(dz) > range)
        return (0);
    dist = (int)jo_sqrt((unsigned int)(dx * dx + dy * dy + dz * dz));
    if (dist > range)
        return (0);
    // The lamp (a bulb around the light): all its light, it shines
    if (dist <= JO_POINT_LIGHT_LAMP_SIZE)
        return ((int)((long long)l->strength * 256 / 64));
    // A spot: in its cone (full light in the inner cone, less to its edge)
    cone = 256;
    if (l->spot && dist > 0)
    {
        c = jo_int_div(l->direction[0] * dx + l->direction[1] * dy + l->direction[2] * dz, dist);
        if (c <= l->cos_outer)
            return (0);
        if (c < l->cos_inner)
            cone = jo_int_div((c - l->cos_outer) * 256, l->cos_inner - l->cos_outer);
    }
    // The face turned toward the light (both sides), a little light at a grazing angle (0.15)
    facing = dist > 0 ? jo_int_div(JO_ABS(n[0] * dx + n[1] * dy + n[2] * dz), dist) : 256;
    facing = JO_MAX(facing, 38);
    // A wall between the light and the corner (a little before the corner: its own polygon is not a wall), from the 5
    // points of the light (soft shadows). The corners near the light (its lamp: a bulb around it) are always lit
    seen = JO_POINT_LIGHT_SOFT_RAYS;
    // (the rays of a corner already shot for another polygon of the mesh: the same result, see jo_point_light_compute())
    if (seen_cache != JO_NULL && *seen_cache >= 0)
    {
        seen = *seen_cache;
        if (seen == 0)
            return (0);
    }
    else if (__jo_point_light_hit != JO_NULL && l->shadows && dist > JO_POINT_LIGHT_LAMP_SIZE)
    {
        seen = 0;
        for (k = 0; k < __jo_point_light_rays; ++k)
        {
            sx = dx - soft[k][0] * JO_POINT_LIGHT_SOFT_SIZE;
            sz = dz - soft[k][1] * JO_POINT_LIGHT_SOFT_SIZE;
            sd = (int)jo_sqrt((unsigned int)(sx * sx + dy * dy + sz * sz));
            if (sd <= 6)
                continue;
            origin.x = l->position.x + jo_int2fixed(soft[k][0] * JO_POINT_LIGHT_SOFT_SIZE);
            origin.y = l->position.y;
            origin.z = l->position.z + jo_int2fixed(soft[k][1] * JO_POINT_LIGHT_SOFT_SIZE);
            d.x = jo_int_div(jo_int2fixed(sx), sd);
            d.y = jo_int_div(jo_int2fixed(dy), sd);
            d.z = jo_int_div(jo_int2fixed(sz), sd);
            if (!__jo_point_light_hit(&origin, &d, jo_int2fixed(sd - 6)))
                ++seen;
        }
        seen = jo_int_div(seen * JO_POINT_LIGHT_SOFT_RAYS, __jo_point_light_rays);
        if (seen_cache != JO_NULL)
            *seen_cache = (signed char)seen;
        if (seen == 0)
            return (0);
    }
    // strength x facing x radius^2 / (radius^2 + distance^2): 16 gouraud units (x 64) for JO_POINT_LIGHT_STRENGTH_DEFAULT in
    // front of the light (facing 256): strength x facing / 64. (32 bits: radius^2 / (radius^2 + distance^2) x 4096 first,
    // a 64 bits division costs a lot: the flashes of the shots are computed again at each shot)
    // (radius^2 x 4096 overflows from a radius of 1024: the 2 terms shifted together, the same ratio)
    rr = (unsigned int)(l->radius * l->radius);
    dd = (unsigned int)(dist * dist);
    shift = rr > 0xFFFFF || dd > 0x7FFFFFFF - rr ? 10 : 0;
    q = (int)(((rr >> shift) << 12) / JO_MAX(1u, (rr >> shift) + (dd >> shift)));
    // (a positive strength: logical shifts, the same result; an arithmetic shift by more than 2 bits is a call of the library
    // of the compiler, 3 calls for each corner)
    if (l->strength >= 0)
        return ((int)((((((unsigned int)(l->strength * facing) >> 8) * (unsigned int)cone) >> 8) * (unsigned int)q) >> 10) * seen /
                JO_POINT_LIGHT_SOFT_RAYS);
    return ((((((l->strength * facing) >> 8) * cone) >> 8) * q >> 10) * seen / JO_POINT_LIGHT_SOFT_RAYS);
}

/** @brief Check if a light can light a corner: false when jo_point_light_gain() is surely 0 (out of reach of the light,
 *  behind a spot), without the square root and the normal of the polygon (the same tests: the result is the same)
 *  @param l Light
 *  @param p Corner (world, Q16.16)
 *  @param range Reach of the light (world units)
 */
static  __jo_force_inline bool      jo_point_light_may_light(const jo_point_light * const l, const jo_pos3D_fixed * const p, const int range)
{
    int                             dx;
    int                             dy;
    int                             dz;

    dx = jo_fixed2int(p->x - l->position.x);
    dy = jo_fixed2int(p->y - l->position.y);
    dz = jo_fixed2int(p->z - l->position.z);
    if (JO_ABS(dx) > range || JO_ABS(dy) > range || JO_ABS(dz) > range)
        return (false);
    // (a spot: a corner behind its cone (cos <= 0 <= the cosine of its edge), out of its lamp (gain: the lamp is lit first))
    if (l->spot && l->cos_outer >= 0 && l->direction[0] * dx + l->direction[1] * dy + l->direction[2] * dz <= 0 &&
        JO_MAX(JO_MAX(JO_ABS(dx), JO_ABS(dy)), JO_ABS(dz)) > JO_POINT_LIGHT_LAMP_SIZE)
        return (false);
    return (true);
}

/** @brief The polygon lit of a mesh (added to the list when it is lit for the first time), -1 if out of memory */
static int                          jo_point_light_polygon_index(const int mesh, const int k)
{
    jo_point_light_polygon          *p;
    const jo_color                  *table;
    int                             j;
    int                             capacity;

    if (__jo_point_light_polygon_of[mesh][k] != 0xffff)
        return (__jo_point_light_polygon_of[mesh][k]);
    if (__jo_point_light_polygon_count >= 0xffff)
        return (-1);
    if (__jo_point_light_polygon_count >= __jo_point_light_polygon_capacity)
    {
        capacity = __jo_point_light_polygon_capacity + 128;
        p = (jo_point_light_polygon *)jo_malloc(capacity * (int)sizeof(*p));
        if (p == JO_NULL)
            return (-1);
        if (__jo_point_light_polygons != JO_NULL)
        {
            jo_memcpy(p, __jo_point_light_polygons, (unsigned int)(__jo_point_light_polygon_count * (int)sizeof(*p)));
            jo_free(__jo_point_light_polygons);
        }
        __jo_point_light_polygons = p;
        __jo_point_light_polygon_capacity = capacity;
    }
    p = &__jo_point_light_polygons[__jo_point_light_polygon_count];
    p->table = __jo_point_light_meshes[mesh]->data.attributes[k].gouraud_table;
    p->mesh = (unsigned char)mesh;
    // (its colors as they are in the copy of the tables: no light on it yet)
    table = &__jo_point_light_mesh_tables[mesh][(p->table - __jo_point_light_mesh_table_first[mesh]) * 4];
    for (j = 0; j < 4; ++j)
        p->colors[j] = table[j];
    jo_memset(p->sum, 0, sizeof(p->sum));
    __jo_point_light_polygon_of[mesh][k] = (unsigned short)__jo_point_light_polygon_count;
    return (__jo_point_light_polygon_count++);
}

/** @brief Compute the light of a light on the corners of the polygons of the meshes */
static void                         jo_point_light_compute(jo_point_light * const l)
{
    const jo_3d_mesh                *mesh;
    const jo_pos3D_fixed            *v[4];
    jo_point_light_entry            entry;
    jo_point_light_entry            *entries;
    int                             capacity;
    int                             n[3];
    int                             m;
    int                             k;
    int                             j;
    int                             index;
    int                             lit;
    int                             gain;
    int                             reach;
    int                             range;
    signed char                     *seen;

    if (l->entries != JO_NULL)
        jo_free(l->entries);
    l->entries = JO_NULL;
    l->entry_count = 0;
    l->computed = true;
    l->partial = false;
    capacity = 0;
    range = l->radius * JO_POINT_LIGHT_RANGE;
    // (all the meshes first: out of memory, the loop stops and the next meshes kept the entries of the last compute)
    for (m = 0; m < __jo_point_light_mesh_count; ++m)
    {
        l->applied[m] = 0;
        l->mesh_first[m] = 0;
        l->mesh_entries[m] = 0;
    }
    for (m = 0; m < __jo_point_light_mesh_count; ++m)
    {
        l->applied[m] = 0;
        l->mesh_first[m] = (unsigned short)l->entry_count;
        l->mesh_entries[m] = 0;
        mesh = __jo_point_light_meshes[m];
        // (the meshes out of reach of the light)
        reach = __jo_point_light_mesh_radius[m] + l->radius * JO_POINT_LIGHT_RANGE;
        if (JO_ABS(jo_fixed2int(__jo_point_light_mesh_center[m].x - l->position.x)) > reach ||
            JO_ABS(jo_fixed2int(__jo_point_light_mesh_center[m].y - l->position.y)) > reach ||
            JO_ABS(jo_fixed2int(__jo_point_light_mesh_center[m].z - l->position.z)) > reach)
            continue;
        // (a light without shadows moves at each frame, a flash: only on the meshes seen. The others when they are seen, see
        // jo_3d_point_lights_set_mesh_seen())
        if (!l->shadows && !__jo_point_light_mesh_seen[m])
        {
            l->partial = true;
            continue;
        }
        // The rays of the shadows of each corner, shot once (a corner is shared by about 4 polygons): -1, not shot yet
        seen = l->shadows && __jo_point_light_hit != JO_NULL ? (signed char *)jo_malloc(mesh->data.point_count) : JO_NULL;
        if (seen != JO_NULL)
            jo_memset(seen, -1, mesh->data.point_count);
        for (k = 0; k < (int)mesh->data.polygon_count; ++k)
        {
            if (!(mesh->data.attributes[k].draw_mode & JO_VDP1_COLOR_CALC_GOURAUD) || mesh->data.attributes[k].gouraud_table == 0)
                continue;
            for (j = 0; j < 4; ++j)
                v[j] = &mesh->data.points[mesh->data.polygons[k].vertices[j]];
            // (a polygon that the light can't reach: no normal, no corner computed. A light that moves at each frame (a torch)
            // reaches a small part of a big mesh)
            if (!jo_point_light_may_light(l, v[0], range) && !jo_point_light_may_light(l, v[1], range) &&
                !jo_point_light_may_light(l, v[2], range) && !jo_point_light_may_light(l, v[3], range))
                continue;
            jo_point_light_normal(v[0], v[1], v[3], n);
            lit = 0;
            for (j = 0; j < 4; ++j)
            {
                gain = jo_point_light_gain(l, v[j], n, seen != JO_NULL ? &seen[mesh->data.polygons[k].vertices[j]] : JO_NULL);
                entry.gain[j] = (unsigned short)JO_MIN(gain, 0xffff);
                lit |= gain;
            }
            if (!lit || (index = jo_point_light_polygon_index(m, k)) < 0)
                continue;
            entry.polygon = (unsigned short)index;
            if (l->entry_count >= capacity)
            {
                capacity += 128;
                entries = (jo_point_light_entry *)jo_malloc(capacity * (int)sizeof(*entries));
                if (entries == JO_NULL)
                {
                    if (seen != JO_NULL)
                        jo_free(seen);
                    return;
                }
                if (l->entries != JO_NULL)
                {
                    jo_memcpy(entries, l->entries, (unsigned int)(l->entry_count * (int)sizeof(*entries)));
                    jo_free(l->entries);
                }
                l->entries = entries;
            }
            l->entries[l->entry_count++] = entry;
            ++l->mesh_entries[m];
        }
        if (seen != JO_NULL)
            jo_free(seen);
    }
}

void                                jo_3d_point_lights_update(void)
{
    jo_point_light                  *l;
    int                             k;
    int                             m;
    int                             first;
    int                             last;

    // (the copies of the last update are done: the tables in the work RAM can change)
    jo_dma_wait();
    // The lights added or moved: their old light removed, their light computed (on the meshes seen below)
    for (k = 0, l = __jo_point_lights; k < JO_MAX_POINT_LIGHTS; ++k, ++l)
        if (l->used && !l->computed)
        {
            jo_point_light_remove(l);
            jo_point_light_compute(l);
        }
    // The meshes seen: the lights at their intensity now (only the changes are computed)
    for (m = 0; m < __jo_point_light_mesh_count; ++m)
        if (__jo_point_light_mesh_seen[m])
            for (k = 0, l = __jo_point_lights; k < JO_MAX_POINT_LIGHTS; ++k, ++l)
                if (l->entries != JO_NULL)
                    jo_point_light_apply(l, m, jo_point_light_intensity(l));
    // The tables changed: copied into the VRAM by the DMA controller (the CPU goes on)
    for (m = 0; m < __jo_point_light_mesh_count; ++m)
    {
        first = __jo_point_light_mesh_dirty_first[m];
        last = __jo_point_light_mesh_dirty_last[m];
        if (last < first)
            continue;
        jo_dma_copy_async(&__jo_point_light_mesh_tables[m][first * 4],
                          (void *)(JO_VDP1_VRAM + JO_MULT_BY_8(__jo_point_light_mesh_table_first[m] + first)),
                          (unsigned int)((last - first + 1) * 4 * (int)sizeof(jo_color)));
        __jo_point_light_mesh_dirty_first[m] = 0x7fffffff;
        __jo_point_light_mesh_dirty_last[m] = -1;
    }
}

void                                jo_3d_point_lights_sample(const jo_pos3D_fixed * const position, int * const r, int * const g, int * const b)
{
    const jo_point_light            *l;
    jo_point_light_sample_entry     *e;
    int                             n[3];
    int                             dx;
    int                             dy;
    int                             dz;
    int                             length;
    int                             gain;
    int                             k;

    *r = 0;
    *g = 0;
    *b = 0;
    for (k = 0, l = __jo_point_lights; k < JO_MAX_POINT_LIGHTS; ++k, ++l)
    {
        if (jo_point_light_intensity(l) <= 0 || l->unsampled)
            continue;
        // (out of reach: before the square root and the ray of the light)
        dx = jo_fixed2int(position->x - l->position.x);
        dy = jo_fixed2int(position->y - l->position.y);
        dz = jo_fixed2int(position->z - l->position.z);
        length = l->radius * JO_POINT_LIGHT_RANGE;
        if (JO_ABS(dx) > length || JO_ABS(dy) > length || JO_ABS(dz) > length)
            continue;
        // (the same place as before: the cache, see jo_point_light_sample_entry. 2 entries per index: 2 objects whose places
        // have the same index don't compute their light again and again)
        e = &__jo_point_light_samples[(unsigned int)((position->x >> 16) * 7 + (position->y >> 16) * 13 + (position->z >> 16) * 31 + k * 17) &
                                      (JO_POINT_LIGHT_SAMPLE_CACHE - 2)];
        if (jo_point_light_sample_hit(&e[0], k, position))
        {
            gain = e[0].gain;
            e[0].recent = 0;
        }
        else if (jo_point_light_sample_hit(&e[1], k, position))
        {
            gain = e[1].gain;
            e[0].recent = 1;
        }
        else
        {
            // A face turned toward the light, then 0.7 of its light: an object has faces lit and faces in the dark
            length = (int)jo_sqrt((unsigned int)(dx * dx + dy * dy + dz * dz));
            if (length <= 0)
                length = 1;
            n[0] = dx * 256 / length;
            n[1] = dy * 256 / length;
            n[2] = dz * 256 / length;
            __jo_point_light_rays = 1;
            gain = (jo_point_light_gain(l, position, n, JO_NULL) * 180) >> 8;
            __jo_point_light_rays = JO_POINT_LIGHT_SOFT_RAYS;
            // (in the entry not used last)
            e[0].recent = (unsigned char)(1 - e[0].recent);
            e += e[0].recent;
            e->generation = __jo_point_light_generation;
            e->light = k;
            e->position = *position;
            e->gain = gain;
        }
        if (gain <= 0)
            continue;
        // (gouraud units x 64 at the intensity 256, x the color / 64: gouraud units x 256)
        // (all positive: logical shifts, an arithmetic shift is a call of the library)
        gain = (int)((unsigned int)(gain * jo_point_light_intensity(l)) >> 8);
        *r += (int)((unsigned int)(gain * l->r) >> 6);
        *g += (int)((unsigned int)(gain * l->g) >> 6);
        *b += (int)((unsigned int)(gain * l->b) >> 6);
    }
    *r = (int)((unsigned int)*r >> 8);
    *g = (int)((unsigned int)*g >> 8);
    *b = (int)((unsigned int)*b >> 8);
}

void                                jo_3d_point_lights_clear(void)
{
    volatile jo_color               *table;
    jo_point_light_polygon          *p;
    int                             k;
    int                             j;

    // (the copy of jo_3d_point_lights_update() ended first: it would write the lit tables again after them)
    jo_dma_wait();
    // The colors of the gouraud tables as they were
    for (k = 0, p = __jo_point_light_polygons; k < __jo_point_light_polygon_count; ++k, ++p)
    {
        table = (volatile jo_color *)(JO_VDP1_VRAM + JO_MULT_BY_8(p->table));
        for (j = 0; j < 4; ++j)
            table[j] = p->colors[j];
    }
    for (k = 0; k < JO_MAX_POINT_LIGHTS; ++k)
    {
        if (__jo_point_lights[k].entries != JO_NULL)
            jo_free(__jo_point_lights[k].entries);
        __jo_point_lights[k].entries = JO_NULL;
        __jo_point_lights[k].used = false;
    }
    jo_point_light_changed();
    jo_dma_wait();
    for (k = 0; k < __jo_point_light_mesh_count; ++k)
    {
        jo_free(__jo_point_light_polygon_of[k]);
        if (__jo_point_light_mesh_tables[k] != JO_NULL)
            jo_free(__jo_point_light_mesh_tables[k]);
        __jo_point_light_mesh_tables[k] = JO_NULL;
    }
    __jo_point_light_mesh_count = 0;
    if (__jo_point_light_polygons != JO_NULL)
        jo_free(__jo_point_light_polygons);
    __jo_point_light_polygons = JO_NULL;
    __jo_point_light_polygon_count = 0;
    __jo_point_light_polygon_capacity = 0;
    __jo_point_light_hit = JO_NULL;
}

#endif /* !JO_COMPILE_WITH_3D_SUPPORT */

/*
** END OF FILE
*/
