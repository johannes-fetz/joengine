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

#include "internal/3d.h"
#include "internal/water.h"

/*
** Objects under the water (see jo/water.h). The sort key of an object: where the line of sight to the point of its bottom
** nearest to the camera goes into the water (over a calm sea at its distance x the height of the camera / (the height of
** the camera + its depth); a crest in front of that point takes it nearer: the waves as drawn at SIGHT_STEPS points of the
** line of sight), in front of that point by 3/4 of a quad of the water there (a quad is sorted at its center) and 1/12.
** Its keys in a band of UNDER_BAND units: the object drawn scaled around the camera (its points nearer by a scale, as far
** on the screen: the same image; its depths, its keys smaller by the scale), its farthest key at its sort key (a depth bias):
** its polygons keep their order, the object stays in front of the water that covers it and between the objects around it
** (one key per object: its polygons in the wrong order; its own depths: its nearest polygons in front of a jet ski over
** it). The scale keeps its points farther than the near plane (else they are cut). Then:
**   - an object under the water that covers a farther one on the screen: in front of it
**   - in front of it, the objects above the water that cover it on the screen, and UNDER_MIN_KEY: the object pushed back
**     (from the nearest: the farther objects under the water it covers behind it again)
** The size of an object: the sphere around its points (the center of their box, half its diagonal)
*/

/** @brief Points of a line of sight where the waves are looked for */
# define SIGHT_STEPS        (6)
/** @brief The sort keys of the objects under the water at least (units): a key under 0 is 0 for the VDP1 sort (their
 *  order lost) */
# define UNDER_MIN_KEY      (8)
/** @brief The keys of an object under the water in a band of this size (units, see above): its polygons in their order to
 *  about its depth / UNDER_BAND units */
# define UNDER_BAND         (24)
/** @brief Color lookup tables seen through the water at most, made in blocks of LUT_BLOCK (a sprite of 16 x LUT_BLOCK RGB
 *  pixels: a table per row of 32 bytes) */
# define SEEN_LUTS          (64)
# define LUT_BLOCK          (16)
/** @brief RGB textures seen through the water at most (a copy each) */
# define SEEN_TEXTURES      (16)
/** @brief Ranges of polygons of an object drawn at most (its hidden polygons between them) */
# define OBJECT_RANGES      (17)

struct                      s_jo_water_object
{
    jo_3d_mesh              *mesh;
    unsigned short          first;          // Its polygons in the mesh
    unsigned short          count;
    unsigned int            hidden;         // Polygons not drawn (bit k: the polygon first + k)
    int                     x;              // Its space in the world (units, y down)
    int                     y;
    int                     z;
    jo_angle                heading;
    jo_fixed                sin;            // Of its heading
    jo_fixed                cos;
    int                     center[3];      // Center of its points in its space (units)
    int                     radius;         // Units, 0: no polygon
    int                     bottom;         // Its deepest point in its space (units, y down)
    int                     far;            // Not drawn farther from the camera (|dx| + |dz|, units)
    bool                    shown;
};

/** @brief An object drawn in the frame: its center in the view (units), its size, its scale around the camera, the band of
 *  its keys (units), its farthest key, the depth bias of its polygons */
typedef struct
{
    jo_water_object         *object;
    int                     view[3];
    int                     radius;
    jo_fixed                scale;
    int                     band;
    int                     top;
    int                     bias;
    int                     free_bias;      // Of its polygons that no object above the water covers
    unsigned int            covered;        // Its polygons covered by an object above the water (bit k: the polygon first + k)
}                           seen_object;

/** @brief An object above the water in the frame: its center (the world, then the view: units), its size, its farthest
 *  sort key from the depth of its center */
typedef struct
{
    int                     place[3];
    int                     view[3];
    int                     radius;
    int                     key;
}                           over_object;

/** @brief A color lookup table seen through the water: its source (a table: VRAM / 8; or the first color of a bank of the
 *  palette), the amount of water, the table (VRAM / 8) */
typedef struct
{
    unsigned short          source;
    bool                    bank;
    short                   amount;
    unsigned short          lut;
}                           seen_lut;

/** @brief An RGB texture seen through the water: its source, the amount of water, the copy */
typedef struct
{
    short                   source;
    short                   amount;
    short                   copy;
}                           seen_texture;

static jo_water_object      under_objects[JO_WATER_MAX_OBJECTS];
static int                  under_count;
static seen_object          in_view[JO_WATER_MAX_OBJECTS];
static int                  in_view_count;
static over_object          over_water[JO_WATER_MAX_ABOVE];
static int                  over_count;
/** @brief The view of the frame (the matrix of the camera): a point of the world (units) is at view_origin + x view_axis[0]
 *  + y view_axis[1] + z view_axis[2] (jo_fixed units) */
static jo_fixed             view_origin[3];
static jo_fixed             view_axis[3][3];
/** @brief The camera in the world (jo_fixed units: the point at the origin of the view) */
static jo_fixed             eye[3];
static seen_lut             seen_luts[SEEN_LUTS];
static int                  seen_lut_count;
static unsigned short       lut_next;       // The next table of the block (VRAM / 8)
static int                  lut_free;       // Tables left in the block
static seen_texture         seen_textures[SEEN_TEXTURES];
static int                  seen_texture_count;
/** @brief The camera of the water in the frame (see __jo_water_camera()): units (y up), the tangent of half the field of view x
 *  16 */
static int                  cam_x;
static int                  cam_y;
static int                  cam_z;
static int                  view_tan;

/** @brief Read the view from the matrix of the camera (the current matrix of the master CPU) */
static void                 read_view(void)
{
    jo_pos3D_fixed          p;
    jo_pos3D_fixed          v;
    int                     k;

    p.x = p.y = p.z = 0;
    jo_3d_transform_point(&p, &v);
    view_origin[0] = v.x;
    view_origin[1] = v.y;
    view_origin[2] = v.z;
    for (JO_ZERO(k); k < 3; ++k)
    {
        // (64 units along each axis: the precision of the axis)
        p.x = k == 0 ? jo_int2fixed(64) : 0;
        p.y = k == 1 ? jo_int2fixed(64) : 0;
        p.z = k == 2 ? jo_int2fixed(64) : 0;
        jo_3d_transform_point(&p, &v);
        view_axis[k][0] = (v.x - view_origin[0]) / 64;
        view_axis[k][1] = (v.y - view_origin[1]) / 64;
        view_axis[k][2] = (v.z - view_origin[2]) / 64;
    }
    // (the camera: the axes are the rows of the rotation of the view)
    for (JO_ZERO(k); k < 3; ++k)
        eye[k] = -(jo_fixed_mult(view_axis[k][0], view_origin[0]) + jo_fixed_mult(view_axis[k][1], view_origin[1]) +
                   jo_fixed_mult(view_axis[k][2], view_origin[2]));
}

/** @brief A point of the world in the view (units) */
static void                 to_view(const int x, const int y, const int z, int view[3])
{
    int                     k;

    for (JO_ZERO(k); k < 3; ++k)
        view[k] = jo_fixed2int(view_origin[k] + view_axis[0][k] * x + view_axis[1][k] * y + view_axis[2][k] * z);
}

/** @brief 2 spheres of the view cover each other on the screen (their directions closer than their sizes, across and up) */
static bool                 overlap(const int a[3], const int ra, const int b[3], const int rb)
{
    int                     za;
    int                     zb;
    int                     reach;

    za = JO_MAX(1, a[2]);
    zb = JO_MAX(1, b[2]);
    reach = ra * zb + rb * za;
    return (JO_ABS(a[0] * zb - b[0] * za) < reach && JO_ABS(a[1] * zb - b[1] * za) < reach);
}

/** @brief The sort key of an object under the water at most (see above; units)
 *  @param x Its center in the world (units)
 *  @param z
 *  @param radius Its size (units)
 *  @param depth Its deepest point under the calm sea (units)
 */
static int                  sight_key(const jo_water * const w, const int x, const int z, const int radius, const int depth)
{
    jo_fixed                sight;
    jo_fixed                t0;
    jo_fixed                t;
    jo_fixed                prev;
    jo_fixed                d;
    int                     view[3];
    int                     height;
    int                     bottom;
    int                     distance;
    int                     dx;
    int                     dz;
    int                     fx;
    int                     fz;
    int                     key;
    int                     k;

    // The point of its bottom nearest to the camera (under the camera: there)
    dx = x - cam_x;
    dz = z - cam_z;
    distance = (int)jo_sqrt((unsigned int)(dx * dx + dz * dz));
    fx = distance > radius ? x - dx * radius / distance : cam_x;
    fz = distance > radius ? z - dz * radius / distance : cam_z;
    height = JO_MAX(8, cam_y);
    bottom = JO_MAX(1, depth);
    // Where the line of sight goes into the water: t of the way (calm: t0)
    t0 = jo_int2fixed(height) / (height + bottom);
    t = t0;
    JO_ZERO(prev);
    for (k = 1; k <= SIGHT_STEPS; ++k)
    {
        d = t0 * k / SIGHT_STEPS;
        sight = jo_int2fixed(height) - (height + bottom) * d;
        if (jo_water_drawn_height(w, cam_x + jo_fixed2int((fx - cam_x) * d), cam_z + jo_fixed2int((fz - cam_z) * d)) >= sight)
        {
            t = (prev + d) / 2;
            break;
        }
        prev = d;
    }
    // (the depths in the view grow along the line of sight from the camera: t of the depth of the point)
    to_view(fx, bottom, fz, view);
    key = jo_fixed2int(view[2] * t);
    return (key - key / 12 - 4 -
            jo_water_quad_side(w, cam_x + jo_fixed2int((fx - cam_x) * t), cam_z + jo_fixed2int((fz - cam_z) * t)) * 3 / 4);
}

/** @brief The polygons of an object under the water (the first 32) that an object above the water nearer than them covers
 *  on the screen (the box of each polygon around its center) */
static unsigned int         covered_polygons(const jo_water_object * const o, const over_object * const a)
{
    const jo_3d_polygon     *polygon;
    const jo_pos3D_fixed    *p;
    unsigned int            mask;
    int                     world[4][3];
    int                     view[3];
    int                     center[3];
    int                     size;
    int                     count;
    int                     k;
    int                     i;
    int                     j;

    JO_ZERO(mask);
    count = JO_MIN((int)o->count, 32);
    for (JO_ZERO(k); k < count; ++k)
    {
        polygon = &o->mesh->data.polygons[o->first + k];
        center[0] = center[1] = center[2] = 0;
        for (JO_ZERO(i); i < 4; ++i)
        {
            p = &o->mesh->data.points[polygon->vertices[i]];
            world[i][0] = o->x + jo_fixed2int(jo_fixed_mult(o->cos, p->x) + jo_fixed_mult(o->sin, p->z));
            world[i][1] = o->y + jo_fixed2int(p->y);
            world[i][2] = o->z + jo_fixed2int(jo_fixed_mult(o->cos, p->z) - jo_fixed_mult(o->sin, p->x));
            for (JO_ZERO(j); j < 3; ++j)
                center[j] += world[i][j];
        }
        for (JO_ZERO(j); j < 3; ++j)
            center[j] /= 4;
        JO_ZERO(size);
        for (JO_ZERO(i); i < 4; ++i)
            for (JO_ZERO(j); j < 3; ++j)
                size = JO_MAX(size, JO_ABS(world[i][j] - center[j]));
        // (a margin: the box of the polygon on the screen from the box around its center)
        size += size / 4 + 2;
        to_view(center[0], center[1], center[2], view);
        if (a->view[2] < view[2] + size && overlap(view, size, a->view, a->radius))
            mask |= 1u << k;
    }
    return (mask);
}

/** @brief The objects drawn in the frame, their scales and their depth biases (see above): by the slave CPU before it draws
 *  them (the view read by the master CPU) */
static void                 prepare_objects(const jo_water * const w)
{
    const jo_water_object   *o;
    seen_object             *s;
    const seen_object       *n;
    seen_object             t;
    jo_fixed                near_scale;
    unsigned int            covered;
    int                     free_top;
    int                     near;
    int                     c[3];
    int                     i;
    int                     j;
    int                     k;
    int                     need;

    // (the points of the objects scaled stay this far: the near plane of the engine, a margin)
    near = jo_fixed2int(__jo_vdp1_near_z) + 4;
    __jo_water_camera(w, &cam_x, &cam_y, &cam_z, &view_tan);
    JO_ZERO(in_view_count);
    for (JO_ZERO(k); k < under_count; ++k)
    {
        o = &under_objects[k];
        if (!o->shown || o->radius <= 0)
            continue;
        // (its center in the world)
        c[0] = o->x + jo_fixed2int(o->cos * o->center[0] + o->sin * o->center[2]);
        c[1] = o->y + o->center[1];
        c[2] = o->z + jo_fixed2int(o->cos * o->center[2] - o->sin * o->center[0]);
        if (JO_ABS(c[0] - cam_x) + JO_ABS(c[2] - cam_z) > o->far)
            continue;
        s = &t;
        to_view(c[0], c[1], c[2], s->view);
        // (behind the camera, beside the view)
        if (s->view[2] + o->radius <= 0 || JO_ABS(s->view[0]) - o->radius > JO_MAX(0, s->view[2]) * view_tan / 16)
            continue;
        s->object = (jo_water_object *)o;
        s->radius = o->radius;
        // Its keys in UNDER_BAND units (its nearest point farther than the near plane), the farthest one where it is seen
        // through the water
        near_scale = s->view[2] - o->radius > near ? jo_int2fixed(near) / (s->view[2] - o->radius) : JO_FIXED_1;
        s->scale = JO_MIN(JO_FIXED_1, JO_MAX(near_scale, jo_int2fixed(UNDER_BAND) / (2 * o->radius)));
        s->band = jo_fixed2int(2 * o->radius * s->scale) + 1;
        s->top = sight_key(w, c[0], c[2], o->radius, o->y + o->bottom);
        // (from the farthest: by the depth of its center)
        for (i = in_view_count; i > 0 && in_view[i - 1].view[2] < s->view[2]; --i)
            in_view[i] = in_view[i - 1];
        in_view[i] = t;
        ++in_view_count;
    }
    // In front of the farther objects under the water that it covers
    for (i = 1; i < in_view_count; ++i)
    {
        s = &in_view[i];
        for (JO_ZERO(j); j < i; ++j)
        {
            n = &in_view[j];
            if (overlap(s->view, s->radius, n->view, n->radius))
                s->top = JO_MIN(s->top, n->top - n->band - 1);
        }
    }
    // Behind the objects above the water that cover it and UNDER_MIN_KEY, behind the nearer objects under the water that cover
    // it (from the nearest); its depth bias: its farthest key at the top of its band
    for (JO_ZERO(k); k < over_count; ++k)
        to_view(over_water[k].place[0], over_water[k].place[1], over_water[k].place[2], over_water[k].view);
    // (pushed back, only its polygons that an object above the water covers: pushed behind a jet ski, a rock near the
    // camera went behind the water in front of the jet ski, all of it)
    for (i = in_view_count - 1; i >= 0; --i)
    {
        s = &in_view[i];
        need = UNDER_MIN_KEY;
        for (j = i + 1; j < in_view_count; ++j)
        {
            n = &in_view[j];
            if (overlap(s->view, s->radius, n->view, n->radius))
                need = JO_MAX(need, n->top + 1);
        }
        free_top = JO_MAX(s->top, need + s->band);
        JO_ZERO(s->covered);
        for (JO_ZERO(k); k < over_count; ++k)
            if (over_water[k].view[2] < s->view[2] + s->radius && overlap(s->view, s->radius, over_water[k].view, over_water[k].radius))
            {
                covered = covered_polygons(s->object, &over_water[k]);
                if (covered != 0 || s->object->count > 32)
                {
                    s->covered |= covered;
                    need = JO_MAX(need, over_water[k].view[2] + over_water[k].key + 1);
                }
            }
        // (more than 32 polygons: all of them pushed back as before)
        if (s->object->count > 32 && s->covered != 0)
            s->covered = 0xFFFFFFFFu;
        s->top = JO_MAX(s->top, need + s->band);
        s->bias = s->top - jo_fixed2int((s->view[2] + s->radius) * s->scale);
        s->free_bias = (s->covered == 0 ? s->top : free_top) - jo_fixed2int((s->view[2] + s->radius) * s->scale);
    }
}

/** @brief Draw the objects under the water (the slave CPU, after the water) */
static void                 draw_objects_on_slave(const jo_water * const w)
{
    jo_3d_polygon_range     ranges[OBJECT_RANGES];
    const jo_water_object   *o;
    int                     n;
    int                     k;
    unsigned int            skip;
    int                     pass;
    int                     i;

    prepare_objects(w);
    for (JO_ZERO(i); i < in_view_count * 2; ++i)
    {
        // (2 passes per object: its polygons not covered by an object above the water, then the covered ones, pushed back)
        pass = i & 1;
        o = in_view[i >> 1].object;
        skip = o->hidden | (pass == 0 ? in_view[i >> 1].covered : ~in_view[i >> 1].covered);
        if (pass == 1 && in_view[i >> 1].covered == 0)
            continue;
        // (its polygons drawn: the ranges between the hidden ones)
        JO_ZERO(n);
        for (JO_ZERO(k); k < o->count; ++k)
        {
            if (k < 32 && (skip & (1u << k)))
                continue;
            if (k >= 32 && pass == 0 && in_view[i >> 1].covered == 0xFFFFFFFFu)
                continue;
            if (k >= 32 && pass == 1 && in_view[i >> 1].covered != 0xFFFFFFFFu)
                continue;
            if (n > 0 && ranges[n - 1].first + ranges[n - 1].count == o->first + k)
                ++ranges[n - 1].count;
            else if (n < OBJECT_RANGES)
            {
                ranges[n].first = (unsigned short)(o->first + k);
                ranges[n].count = 1;
                ++n;
            }
        }
        if (n == 0)
            continue;
        jo_3d_slave_push_matrix();
        // (scaled around the camera: the same image, its depths smaller, see prepare_objects())
        if (in_view[i >> 1].scale < JO_FIXED_1)
        {
            jo_3d_slave_translate_matrix_fixed(eye[0], eye[1], eye[2]);
            jo_3d_slave_set_scale_fixed(in_view[i >> 1].scale, in_view[i >> 1].scale, in_view[i >> 1].scale);
            jo_3d_slave_translate_matrix_fixed(jo_int2fixed(o->x) - eye[0], jo_int2fixed(o->y) - eye[1], jo_int2fixed(o->z) - eye[2]);
        }
        else if (o->x != 0 || o->y != 0 || o->z != 0)
            jo_3d_slave_translate_matrix_fixed(jo_int2fixed(o->x), jo_int2fixed(o->y), jo_int2fixed(o->z));
        if (o->heading != 0)
            jo_3d_slave_rotate(JO_3D_AXIS_Y, o->heading);
        jo_3d_slave_set_depth_bias(pass == 0 ? in_view[i >> 1].free_bias : in_view[i >> 1].bias);
        jo_3d_slave_mesh_draw_ranges(o->mesh, o->mesh->data.point_count, ranges, n);
        jo_3d_slave_pop_matrix();
    }
    jo_3d_slave_set_depth_bias(0);
}

/** @brief A color lookup table seen through the water (VRAM / 8), 0 if there is no more room
 *  @param source A table (VRAM / 8), or the first color of a bank of the palette
 *  @param bank The source is a bank of the palette (its color 0: transparent)
 */
static unsigned short       seen_table(const jo_water * const w, const unsigned short source, const bool bank, const int amount)
{
    volatile jo_color       *lut;
    const volatile jo_color *from;
    jo_img                  block;
    int                     id;
    int                     k;

    for (JO_ZERO(k); k < seen_lut_count; ++k)
        if (seen_luts[k].source == source && seen_luts[k].bank == bank && seen_luts[k].amount == amount)
            return (seen_luts[k].lut);
    if (seen_lut_count >= SEEN_LUTS)
        return (0);
    // A block of tables (a sprite of 16 x LUT_BLOCK RGB pixels: a table per row)
    if (lut_free == 0)
    {
        block.width = 16;
        block.height = LUT_BLOCK;
        if ((block.data = (jo_color *)jo_malloc(16 * LUT_BLOCK * sizeof(jo_color))) == JO_NULL)
            return (0);
        jo_memset(block.data, 0, 16 * LUT_BLOCK * sizeof(jo_color));
        id = jo_sprite_add(&block);
        jo_free(block.data);
        if (id < 0)
            return (0);
        lut_next = __jo_sprite_def[id].adr;
        lut_free = LUT_BLOCK;
    }
    lut = (volatile jo_color *)(JO_VDP1_VRAM + lut_next * 8);
    from = (const volatile jo_color *)(JO_VDP1_VRAM + source * 8);
    lut[0] = bank ? 0 : from[0];
    for (k = 1; k < 16; ++k)
        lut[k] = jo_water_seen_color(w, bank ? jo_get_palette_register((unsigned short)(source + k)) : from[k], amount);
    seen_luts[seen_lut_count].source = source;
    seen_luts[seen_lut_count].bank = bank;
    seen_luts[seen_lut_count].amount = (short)amount;
    seen_luts[seen_lut_count].lut = lut_next;
    ++seen_lut_count;
    lut_next += 32 / 8;
    --lut_free;
    return (seen_luts[seen_lut_count - 1].lut);
}

/** @brief A copy of an RGB texture seen through the water (its transparent pixels and its end codes kept), -1 if there is
 *  no more room */
static int                  seen_rgb_texture(const jo_water * const w, const int sprite, const int amount)
{
    const volatile jo_color *from;
    jo_img                  img;
    jo_color                c;
    int                     size;
    int                     k;

    for (JO_ZERO(k); k < seen_texture_count; ++k)
        if (seen_textures[k].source == sprite && seen_textures[k].amount == amount)
            return (seen_textures[k].copy);
    if (seen_texture_count >= SEEN_TEXTURES)
        return (-1);
    img.width = (unsigned short)jo_sprite_get_width(sprite);
    img.height = (unsigned short)jo_sprite_get_height(sprite);
    size = img.width * img.height;
    if ((img.data = (jo_color *)jo_malloc((unsigned int)size * sizeof(jo_color))) == JO_NULL)
        return (-1);
    from = (const volatile jo_color *)(JO_VDP1_VRAM + __jo_sprite_def[sprite].adr * 8);
    for (JO_ZERO(k); k < size; ++k)
    {
        c = from[k];
        img.data[k] = c == JO_COLOR_Transparent || (c & 0x7FFF) == 0x7FFF ? c : jo_water_seen_color(w, c, amount);
    }
    k = jo_sprite_add(&img);
    jo_free(img.data);
    if (k < 0)
        return (-1);
    seen_textures[seen_texture_count].source = (short)sprite;
    seen_textures[seen_texture_count].amount = (short)amount;
    seen_textures[seen_texture_count].copy = (short)k;
    ++seen_texture_count;
    return (k);
}

/** @brief Forget the colors seen through the water in the sprites freed (see __jo_3d_water_sprites_freed): the tables and
 *  the copies there (the next table was written in the VRAM of the sprite added at their place), and the ones made from a
 *  table or a texture freed (a new one at its place took the old colors)
 *  @param sprite_id First sprite freed
 */
static void                 water_sprites_freed(const int sprite_id)
{
    unsigned int            adr;
    int                     k;
    int                     n;

    if (sprite_id < 0 || sprite_id > jo_get_last_sprite_id())
        return;
    // (the sprites are in the VRAM in their order: the ones freed from this address)
    adr = __jo_sprite_def[sprite_id].adr;
    if (lut_next >= adr)
        JO_ZERO(lut_free);
    JO_ZERO(n);
    for (JO_ZERO(k); k < seen_lut_count; ++k)
        if (seen_luts[k].lut < adr && (seen_luts[k].bank || seen_luts[k].source < adr))
            seen_luts[n++] = seen_luts[k];
    seen_lut_count = n;
    JO_ZERO(n);
    for (JO_ZERO(k); k < seen_texture_count; ++k)
        if (seen_textures[k].copy < sprite_id && seen_textures[k].source < sprite_id)
            seen_textures[n++] = seen_textures[k];
    seen_texture_count = n;
}

/** @brief An attribute seen through the water: its colors mixed with the color of the water, not lit (its gouraud kept)
 *  @param palette_offset Added to its colors of the palette */
static void                 seen_attribute(const jo_water * const w, jo_3d_polygon_attribute * const a, const int amount, const int palette_offset)
{
    unsigned short          lut;
    int                     id;

    __jo_3d_water_sprites_freed = water_sprites_freed;
    a->sort &= (unsigned char)~JO_3D_USE_LIGHT;
    if (!(a->sort & JO_3D_USE_TEXTURE))
    {
        // (an RGB color or a color of the palette)
        a->color = jo_water_seen_color(w, (a->color & 0x8000) ? a->color : jo_get_palette_register((unsigned short)(a->color + palette_offset)),
                                    amount);
        return;
    }
    switch (a->draw_mode & JO_VDP1_COLOR_MODE_MASK)
    {
        case JO_VDP1_COLOR_MODE_16_LOOKUP:
            if ((lut = seen_table(w, a->color, false, amount)) != 0)
                a->color = lut;
            break;
        case JO_VDP1_COLOR_MODE_16_BANK:
            // (its colors in a table: RGB)
            if ((lut = seen_table(w, (unsigned short)(a->color + palette_offset), true, amount)) != 0)
            {
                a->color = lut;
                a->draw_mode = (unsigned short)((a->draw_mode & ~JO_VDP1_COLOR_MODE_MASK) | JO_VDP1_COLOR_MODE_16_LOOKUP);
            }
            break;
        case JO_VDP1_COLOR_MODE_RGB:
            if ((id = seen_rgb_texture(w, a->texture_id, amount)) >= 0)
                a->texture_id = (unsigned short)id;
            break;
        default:
            // (64, 128, 256 colors of the palette: as they are)
            break;
    }
}

/** @brief The objects at each step of a frame of the water (see internal/water.h) */
static void                 objects_frame(const jo_water * const w, const __jo_water_objects_step step)
{
    switch (step)
    {
    case __JO_WATER_OBJECTS_VIEW:
        if (under_count > 0)
            read_view();
        break;
    case __JO_WATER_OBJECTS_DRAW_ON_SLAVE:
        if (under_count > 0)
            draw_objects_on_slave(w);
        break;
    case __JO_WATER_OBJECTS_FREE:
        // (their sprites: freed with the textures of the water, see jo_water_free())
        JO_ZERO(under_count);
        JO_ZERO(in_view_count);
        JO_ZERO(over_count);
        JO_ZERO(seen_lut_count);
        JO_ZERO(lut_free);
        JO_ZERO(seen_texture_count);
        break;
    default:
        JO_ZERO(over_count);
        break;
    }
}

jo_water_object             *jo_water_add_mesh(jo_water * const w, jo_3d_mesh * const mesh, const int first, const int count, const int amount,
                                            const int far)
{
    jo_water_object         *o;
    int                     k;

    __jo_water_objects_frame = objects_frame;
    if (mesh == JO_NULL || under_count >= JO_WATER_MAX_OBJECTS || first < 0 || count <= 0 || first + count > (int)mesh->data.polygon_count)
        return (JO_NULL);
    o = &under_objects[under_count++];
    o->mesh = mesh;
    o->first = (unsigned short)first;
    o->count = (unsigned short)count;
    JO_ZERO(o->hidden);
    o->far = far;
    o->shown = true;
    jo_water_place(o, 0, 0, 0, 0);
    for (JO_ZERO(k); k < count; ++k)
        seen_attribute(w, &mesh->data.attributes[first + k], amount, 0);
    jo_water_reshape(o);
    return (o);
}

int                         jo_water_add_model(jo_water * const w, jo_3d_model * const model, const int amount, const int far,
                                            jo_water_object ** const objects)
{
    jo_water_object         *o;
    int                     count;
    int                     k;

    if (model == JO_NULL || model->mesh == JO_NULL)
        return (-1);
    count = model->group_count > 0 ? (int)model->group_count : 1;
    for (JO_ZERO(k); k < count; ++k)
    {
        o = model->group_count > 0 ?
            jo_water_add_mesh(w, model->mesh, model->groups[k].first_polygon, model->groups[k].polygon_count, amount, far) :
            jo_water_add_mesh(w, model->mesh, 0, (int)model->mesh->data.polygon_count, amount, far);
        if (o == JO_NULL)
            return (-1);
        if (objects != JO_NULL)
            objects[k] = o;
    }
    return (count);
}

void                        jo_water_place(jo_water_object * const o, const int x, const int y, const int z, const jo_angle heading)
{
    o->x = x;
    o->y = y;
    o->z = z;
    o->heading = heading;
    o->sin = jo_sin_angle(heading);
    o->cos = jo_cos_angle(heading);
}

void                        jo_water_show(jo_water_object * const o, const bool shown, const unsigned int hidden)
{
    o->shown = shown;
    o->hidden = hidden;
}

void                        jo_water_set_far(jo_water_object * const o, const int far)
{
    o->far = far;
}

void                        jo_water_reshape(jo_water_object * const o)
{
    const jo_pos3D_fixed    *p;
    jo_fixed                low[3];
    jo_fixed                high[3];
    int                     size[3];
    int                     k;
    int                     i;

    low[0] = low[1] = low[2] = 0x7FFFFFFF;
    high[0] = high[1] = high[2] = -0x7FFFFFFF;
    for (JO_ZERO(k); k < o->count; ++k)
    {
        if (k < 32 && (o->hidden & (1u << k)))
            continue;
        for (JO_ZERO(i); i < 4; ++i)
        {
            p = &o->mesh->data.points[o->mesh->data.polygons[o->first + k].vertices[i]];
            low[0] = JO_MIN(low[0], p->x);
            low[1] = JO_MIN(low[1], p->y);
            low[2] = JO_MIN(low[2], p->z);
            high[0] = JO_MAX(high[0], p->x);
            high[1] = JO_MAX(high[1], p->y);
            high[2] = JO_MAX(high[2], p->z);
        }
    }
    if (low[0] > high[0])
    {
        JO_ZERO(o->radius);
        return;
    }
    for (JO_ZERO(k); k < 3; ++k)
    {
        o->center[k] = jo_fixed2int((low[k] >> 1) + (high[k] >> 1));
        size[k] = jo_fixed2int(high[k] - low[k]) + 1;
    }
    o->radius = (int)jo_sqrt((unsigned int)(size[0] * size[0] + size[1] * size[1] + size[2] * size[2])) / 2 + 1;
    o->bottom = jo_fixed2int(high[1]) + 1;
}

void                        jo_water_add_above(jo_water * const w, const int x, const int y, const int z, const int radius, const int key_offset)
{
    JO_UNUSED_ARG(w);
    __jo_water_objects_frame = objects_frame;
    if (over_count >= JO_WATER_MAX_ABOVE)
        return;
    over_water[over_count].place[0] = x;
    over_water[over_count].place[1] = y;
    over_water[over_count].place[2] = z;
    over_water[over_count].radius = radius;
    over_water[over_count].key = key_offset;
    ++over_count;
}

struct                      s_jo_water_dip
{
    jo_3d_mesh              *mesh;
    int                     count;          // Polygons
    int                     levels;
    jo_fixed                depths[JO_WATER_DIP_LEVELS];
    jo_3d_polygon_attribute *attributes;    // As made, then the attributes of each level (count each)
    jo_pos3D_fixed          *middles;       // The middle of each polygon (its 4 points)
};

jo_water_dip                *jo_water_add_dip(jo_water * const w, jo_3d_mesh * const mesh, const int levels, const short * const amounts,
                                           const jo_fixed * const depths, const int palette_offset)
{
    jo_water_dip            *d;
    const jo_pos3D_fixed    *p;
    int                     count;
    int                     k;
    int                     i;

    if (mesh == JO_NULL || levels < 1 || levels > JO_WATER_DIP_LEVELS)
        return (JO_NULL);
    count = (int)mesh->data.polygon_count;
    if ((d = (jo_water_dip *)jo_malloc(sizeof(jo_water_dip))) == JO_NULL)
        return (JO_NULL);
    d->attributes = (jo_3d_polygon_attribute *)jo_malloc((unsigned int)(count * (levels + 1)) * sizeof(jo_3d_polygon_attribute));
    d->middles = (jo_pos3D_fixed *)jo_malloc((unsigned int)count * sizeof(jo_pos3D_fixed));
    if (d->attributes == JO_NULL || d->middles == JO_NULL)
    {
        jo_water_free_dip(d);
        return (JO_NULL);
    }
    d->mesh = mesh;
    d->count = count;
    d->levels = levels;
    for (JO_ZERO(k); k < levels; ++k)
        d->depths[k] = depths[k];
    for (JO_ZERO(k); k < count; ++k)
    {
        d->attributes[k] = mesh->data.attributes[k];
        for (JO_ZERO(i); i < levels; ++i)
        {
            d->attributes[(i + 1) * count + k] = mesh->data.attributes[k];
            seen_attribute(w, &d->attributes[(i + 1) * count + k], amounts[i], palette_offset);
        }
        d->middles[k].x = d->middles[k].y = d->middles[k].z = 0;
        for (JO_ZERO(i); i < 4; ++i)
        {
            p = &mesh->data.points[mesh->data.polygons[k].vertices[i]];
            d->middles[k].x += p->x / 4;
            d->middles[k].y += p->y / 4;
            d->middles[k].z += p->z / 4;
        }
    }
    return (d);
}

void                        jo_water_dip_mesh(jo_water_dip * const d, const jo_fixed surface, const jo_angle pitch, const jo_angle roll)
{
    const jo_pos3D_fixed    *m;
    jo_fixed                sp;
    jo_fixed                cp;
    jo_fixed                sr;
    jo_fixed                cr;
    jo_fixed                depth;
    int                     level;
    int                     k;

    sp = jo_sin_angle(pitch);
    cp = jo_cos_angle(pitch);
    sr = jo_sin_angle(roll);
    cr = jo_cos_angle(roll);
    for (JO_ZERO(k); k < d->count; ++k)
    {
        // (the depth of its middle: rolled, then pitched, as drawn)
        m = &d->middles[k];
        depth = jo_fixed_mult(jo_fixed_mult(m->x, sr) + jo_fixed_mult(m->y, cr), cp) - jo_fixed_mult(m->z, sp) - surface;
        for (JO_ZERO(level); level < d->levels && depth >= d->depths[level]; ++level)
            ;
        d->mesh->data.attributes[k] = d->attributes[level * d->count + k];
    }
}

void                        jo_water_free_dip(jo_water_dip * const d)
{
    if (d == JO_NULL)
        return;
    if (d->attributes != JO_NULL)
        jo_free(d->attributes);
    if (d->middles != JO_NULL)
        jo_free(d->middles);
    jo_free(d);
}

#endif /* JO_COMPILE_WITH_3D_SUPPORT && JO_COMPILE_WITH_DUAL_CPU_SUPPORT */

/*
** END OF FILE
*/
