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
** Fighters: 704 x 480 (double density interlace) at 60 FPS, like the fighting games of the Saturn.
**
** - The VDP2 draws the arena: the ring and the water (rotation screen, plane A in perspective), the sky and the mountains
**   (plane B, flat), the shadows of the fighters (shadow polygons over the plane A) and the flash of the hits (color offset)
** - The VDP1 draws only the 2 fighters and the sparks: palette polygons (the frame buffer has 8 bits per pixel in 640 and
**   704 pixels), lit by the palette lighting (16 shades of each color). The master CPU draws the red fighter, the slave CPU
**   the white one
**
** Pad 1: LEFT / RIGHT to walk, DOWN to guard, A to punch, B to kick. Without the pad, the 2 fighters play alone.
*/

#include <jo/jo.h>

#define SHADES              (16)
#define SHADES_FIRST        (16)
#define RING_HALF           (240)
#define RING_Y              (-24)
#define ARENA_SIZE          (256)
#define ARENA_SCALE         (2)
#define RING_TILE           (24)
#define WATER_FIRST         (40)
#define WATER_COLORS        (8)
#define SKY_WIDTH           (512)
#define SKY_HEIGHT          (128)
#define SKY_HORIZON         (116)
#define HEALTH              (100)
#define SPARK_FRAMES        (10)
#define MAX_QUADS           (48)
#define FIGHTER_LIMIT       (RING_HALF - 95)

typedef enum
{
    MAT_SKIN,
    MAT_HAIR,
    MAT_GI_RED,
    MAT_TRIM_RED,
    MAT_PANTS_BLUE,
    MAT_GI_WHITE,
    MAT_TRIM_WHITE,
    MAT_PANTS_BLACK,
    MAT_BELT,
    MAT_WRAP,
    MAT_SPARK,
    MAT_SPARK_CORE,
    MAT_CURB,
    MAT_COUNT
}                           material;

static const jo_color       material_colors[MAT_COUNT] =
{
    JO_COLOR_RGB(224, 168, 124), JO_COLOR_RGB(36, 26, 20), JO_COLOR_RGB(196, 28, 28), JO_COLOR_RGB(232, 200, 64),
    JO_COLOR_RGB(40, 52, 128), JO_COLOR_RGB(236, 236, 228), JO_COLOR_RGB(64, 64, 72), JO_COLOR_RGB(30, 30, 36),
    JO_COLOR_RGB(20, 20, 20), JO_COLOR_RGB(232, 224, 208), JO_COLOR_RGB(255, 200, 48), JO_COLOR_RGB(255, 255, 216),
    JO_COLOR_RGB(150, 136, 116)
};

typedef enum
{
    PART_PELVIS,
    PART_TORSO,
    PART_HEAD,
    PART_UPPER_ARM,
    PART_FOREARM,
    PART_FIST,
    PART_THIGH,
    PART_SHIN,
    PART_FOOT,
    PART_COUNT
}                           part;

typedef enum
{
    STATE_IDLE,
    STATE_WALK,
    STATE_PUNCH,
    STATE_KICK,
    STATE_GUARD,
    STATE_HIT,
    STATE_KO
}                           fighter_state;

typedef struct
{
    short                   shoulder;
    short                   spread;
    short                   elbow;
}                           arm_pose;

typedef struct
{
    short                   hip;
    short                   spread;
    short                   knee;
}                           leg_pose;

typedef struct
{
    short                   lean;
    short                   twist;
    short                   head;
    short                   fall;
    arm_pose                arm[2];
    leg_pose                leg[2];
}                           pose;

typedef struct
{
    jo_3d_mesh              *mesh[PART_COUNT];
    int                     x;
    int                     dir;
    int                     walk;
    fighter_state           state;
    int                     frame;
    int                     health;
    int                     shown_health;
    bool                    hit_done;
    int                     think;
    int                     wins;
}                           fighter;

/** @brief The matrix operations of a CPU: the master CPU draws a fighter, the slave CPU the other one */
typedef struct
{
    void                    (*push)(void);
    void                    (*pop)(void);
    void                    (*translate)(const int x, const int y, const int z);
    void                    (*rotate)(const int x, const int y, const int z);
    void                    (*draw)(jo_3d_mesh *mesh);
}                           renderer;

typedef struct
{
    jo_3d_mesh              *mesh;
    int                     count;
}                           builder;

static jo_camera            camera;
static jo_palette           palette;
static jo_palette           arena_palette;
static fighter              fighters[2];
static jo_3d_mesh           *shadow;
static jo_3d_mesh           *spark;
static jo_3d_mesh           *curb;
static unsigned char        arena_pixels[ARENA_SIZE * ARENA_SIZE];
static unsigned char        sky_pixels[SKY_WIDTH * SKY_HEIGHT];
static jo_color             water_colors[WATER_COLORS];
static int                  frame_count = 0;
static int                  round_frames = 0;
static int                  ko_frames = 0;
static int                  spark_frames = 0;
static int                  spark_x;
static int                  spark_y;
static int                  shake = 0;
static int                  flash = 0;
static bool                 player = false;

static jo_color             color_of(const material m)
{
    return (JO_PALETTE_LIGHT_COLOR(palette.id, SHADES_FIRST, m));
}

/*
** Meshes: rings of 6 or 8 sides (the limbs, the body, the head) and boxes (the fists, the feet). Seen from the outside, the
** vertices of a quad turn clockwise on the screen (single plane polygons)
*/

static void                 add_quad(builder *b, const jo_fixed v[4][3], const jo_fixed nx, const jo_fixed ny, const jo_fixed nz,
                                     const material m)
{
    int                     i;

    if (b->count >= MAX_QUADS)
        return;
    for (JO_ZERO(i); i < 4; ++i)
        jo_3d_set_mesh_vertice(b->mesh, v[i][0], v[i][1], v[i][2], b->count * 4 + i);
    jo_3d_set_mesh_normal(b->mesh, nx, ny, nz, b->count);
    jo_3d_set_mesh_polygon_color(b->mesh, color_of(m), b->count);
    b->mesh->data.attributes[b->count].plane = JO_3D_SINGLE_PLANE;
    ++b->count;
}

static void                 ring_point(jo_fixed v[3], const int i, const int sides, const int y, const int rx, const int rz)
{
    int                     a;

    a = 360 * i / sides + 180 / sides;
    v[0] = jo_fixed_mult(jo_int2fixed(rx), jo_sin(a));
    v[1] = jo_int2fixed(y);
    v[2] = jo_fixed_mult(jo_int2fixed(rz), jo_cos(a));
}

/** @brief Side quads of a ring from y0 (radii rx0, rz0) to y1 (rx1, rz1), the front quad (+Z) of the material front */
static void                 add_ring(builder *b, const int sides, const int y0, const int rx0, const int rz0, const int y1,
                                     const int rx1, const int rz1, const material m, const material front, const material back)
{
    jo_fixed                v[4][3];
    int                     i;
    int                     a;

    for (JO_ZERO(i); i < sides; ++i)
    {
        ring_point(v[0], i + 1, sides, y0, rx0, rz0);
        ring_point(v[1], i, sides, y0, rx0, rz0);
        ring_point(v[2], i, sides, y1, rx1, rz1);
        ring_point(v[3], i + 1, sides, y1, rx1, rz1);
        a = 360 * i / sides + 360 / sides;
        add_quad(b, (const jo_fixed (*)[3])v, jo_sin(a), 0, jo_cos(a),
                 i == sides - 1 ? front : i == sides / 2 - 1 ? back : m);
    }
}

/** @brief A cap of a ring (top: seen from above, bottom: seen from below) */
static void                 add_cap(builder *b, const int sides, const int y, const int rx, const int rz, const bool top, const material m)
{
    static const unsigned char  octagon[3][4] = {{0, 1, 2, 3}, {0, 3, 4, 7}, {4, 5, 6, 7}};
    static const unsigned char  hexagon[2][4] = {{0, 1, 2, 3}, {0, 3, 4, 5}};
    jo_fixed                    v[4][3];
    int                         q;
    int                         i;
    int                         k;

    for (JO_ZERO(q); q < (sides == 8 ? 3 : 2); ++q)
    {
        for (JO_ZERO(i); i < 4; ++i)
        {
            k = sides == 8 ? octagon[q][top ? i : 3 - i] : hexagon[q][top ? i : 3 - i];
            ring_point(v[i], k, sides, y, rx, rz);
        }
        add_quad(b, (const jo_fixed (*)[3])v, 0, top ? -JO_FIXED_1 : JO_FIXED_1, 0, m);
    }
}

/** @brief A ring of 4 sides whose corners are exactly at (+-h, +-h) (add_ring() rounds the radius of its corners): from y0
 *  (half side h0) to y1 (half side h1) */
static void                 add_square_ring(builder *b, const int y0, const int h0, const int y1, const int h1, const material m)
{
    static const signed char    sx[4] = {1, 1, -1, -1};
    static const signed char    sz[4] = {1, -1, -1, 1};
    jo_fixed                    v[4][3];
    int                         i;
    int                         n;
    int                         a;

    for (JO_ZERO(i); i < 4; ++i)
    {
        n = (i + 1) & 3;
        v[0][0] = jo_int2fixed(sx[n] * h0); v[0][1] = jo_int2fixed(y0); v[0][2] = jo_int2fixed(sz[n] * h0);
        v[1][0] = jo_int2fixed(sx[i] * h0); v[1][1] = jo_int2fixed(y0); v[1][2] = jo_int2fixed(sz[i] * h0);
        v[2][0] = jo_int2fixed(sx[i] * h1); v[2][1] = jo_int2fixed(y1); v[2][2] = jo_int2fixed(sz[i] * h1);
        v[3][0] = jo_int2fixed(sx[n] * h1); v[3][1] = jo_int2fixed(y1); v[3][2] = jo_int2fixed(sz[n] * h1);
        a = 90 * i + 90;
        add_quad(b, (const jo_fixed (*)[3])v, jo_sin(a), 0, jo_cos(a), m);
    }
}

/** @brief Turn the last quads of a mesh by 180 degrees around Y (x and z negated: the winding stays the same) */
static void                 turn_quads(builder *b, const int first)
{
    int                     i;

    for (i = first * 4; i < b->count * 4; ++i)
    {
        b->mesh->data.points[i].x = -b->mesh->data.points[i].x;
        b->mesh->data.points[i].z = -b->mesh->data.points[i].z;
    }
    for (i = first; i < b->count; ++i)
    {
        b->mesh->data.polygons[i].normal.x = -b->mesh->data.polygons[i].normal.x;
        b->mesh->data.polygons[i].normal.z = -b->mesh->data.polygons[i].normal.z;
    }
}

static void                 add_box(builder *b, const int x0, const int y0, const int z0, const int x1, const int y1, const int z1,
                                    const int front_shrink, const material m)
{
    static const unsigned char  faces[6][4] = {{0, 1, 3, 2}, {5, 4, 6, 7}, {4, 0, 2, 6}, {1, 5, 7, 3}, {4, 5, 1, 0}, {2, 3, 7, 6}};
    static const signed char    normals[6][3] = {{0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}};
    jo_fixed                    p[8][3];
    jo_fixed                    v[4][3];
    int                         f;
    int                         i;

    for (JO_ZERO(i); i < 8; ++i)
    {
        p[i][0] = jo_int2fixed((i & 1) ? x1 - ((i & 4) ? front_shrink : 0) : x0 + ((i & 4) ? front_shrink : 0));
        p[i][1] = jo_int2fixed((i & 2) ? y1 : y0 + ((i & 4) ? front_shrink : 0));
        p[i][2] = jo_int2fixed((i & 4) ? z1 : z0);
    }
    for (JO_ZERO(f); f < 6; ++f)
    {
        for (JO_ZERO(i); i < 4; ++i)
            jo_memcpy(v[i], p[faces[f][i]], sizeof(v[i]));
        add_quad(b, (const jo_fixed (*)[3])v, jo_int2fixed(normals[f][0]), jo_int2fixed(normals[f][1]), jo_int2fixed(normals[f][2]), m);
    }
}

/** @brief An eye or the mouth: a small dark quad just in front of the face (z = 11) */
static void                 add_face_detail(builder *b, const int x0, const int y0, const int x1, const int y1)
{
    jo_fixed                v[4][3];

    v[0][0] = jo_int2fixed(x1); v[0][1] = jo_int2fixed(y0);
    v[1][0] = jo_int2fixed(x0); v[1][1] = jo_int2fixed(y0);
    v[2][0] = jo_int2fixed(x0); v[2][1] = jo_int2fixed(y1);
    v[3][0] = jo_int2fixed(x1); v[3][1] = jo_int2fixed(y1);
    v[0][2] = v[1][2] = v[2][2] = v[3][2] = jo_float2fixed(10.6f);
    add_quad(b, (const jo_fixed (*)[3])v, 0, 0, JO_FIXED_1, MAT_HAIR);
}

static void                 begin_mesh(builder *b)
{
    b->mesh = jo_3d_create_mesh(MAX_QUADS);
    b->count = 0;
}

static jo_3d_mesh           *end_mesh(builder *b, const bool lit)
{
    b->mesh->data.polygon_count = (unsigned int)b->count;
    b->mesh->data.point_count = (unsigned int)b->count * 4;
    if (lit)
        jo_3d_set_mesh_light(b->mesh, true);
    return (b->mesh);
}

static void                 create_fighter(fighter *f, const material gi, const material trim, const material pants, const int x)
{
    builder                 b;

    begin_mesh(&b);
    add_ring(&b, 8, -9, 16, 11, -3, 16, 11, MAT_BELT, MAT_BELT, MAT_BELT);
    add_ring(&b, 8, -3, 15, 10, 9, 14, 10, pants, pants, pants);
    add_cap(&b, 8, 9, 14, 10, false, pants);
    f->mesh[PART_PELVIS] = end_mesh(&b, true);
    begin_mesh(&b);
    add_ring(&b, 8, -52, 21, 12, -30, 19, 12, gi, MAT_SKIN, gi);
    add_ring(&b, 8, -30, 19, 12, -9, 16, 11, gi, trim, gi);
    // (under the top of the belt: no gap at the waist when the torso leans or twists)
    add_ring(&b, 8, -9, 16, 11, -1, 13, 9, gi, trim, gi);
    add_cap(&b, 8, -52, 21, 12, true, gi);
    add_ring(&b, 6, -58, 5, 5, -51, 6, 6, MAT_SKIN, MAT_SKIN, MAT_SKIN);
    f->mesh[PART_TORSO] = end_mesh(&b, true);
    begin_mesh(&b);
    add_ring(&b, 8, -25, 10, 11, -14, 10, 11, MAT_HAIR, MAT_SKIN, MAT_HAIR);
    add_ring(&b, 8, -14, 10, 11, -2, 8, 9, MAT_SKIN, MAT_SKIN, MAT_HAIR);
    add_ring(&b, 8, -2, 8, 9, 2, 5, 6, MAT_SKIN, MAT_SKIN, MAT_SKIN);
    add_cap(&b, 8, -25, 10, 11, true, MAT_HAIR);
    add_face_detail(&b, -6, -16, -1, -14);
    add_face_detail(&b, 1, -16, 6, -14);
    add_face_detail(&b, -3, -7, 3, -6);
    f->mesh[PART_HEAD] = end_mesh(&b, true);
    begin_mesh(&b);
    add_ring(&b, 8, 0, 8, 8, 28, 6, 6, gi, gi, gi);
    add_cap(&b, 8, 0, 8, 8, true, gi);
    add_cap(&b, 8, 28, 6, 6, false, gi);
    f->mesh[PART_UPPER_ARM] = end_mesh(&b, true);
    begin_mesh(&b);
    // (the forearm starts inside the end of the upper arm, closed: no hole at the elbow when it bends)
    add_ring(&b, 6, -6, 5, 5, 0, 6, 6, MAT_SKIN, MAT_SKIN, MAT_SKIN);
    add_cap(&b, 6, -6, 5, 5, true, MAT_SKIN);
    add_ring(&b, 6, 0, 6, 6, 24, 5, 5, MAT_SKIN, MAT_SKIN, MAT_SKIN);
    add_ring(&b, 6, 18, 6, 6, 24, 6, 6, MAT_WRAP, MAT_WRAP, MAT_WRAP);
    f->mesh[PART_FOREARM] = end_mesh(&b, true);
    begin_mesh(&b);
    add_box(&b, -6, 23, -6, 6, 34, 7, 1, MAT_WRAP);
    f->mesh[PART_FIST] = end_mesh(&b, true);
    begin_mesh(&b);
    // (the thigh starts inside the pelvis and is closed at the knee: no hole at the hip or the knee when they bend)
    add_ring(&b, 8, -8, 8, 8, 0, 10, 10, pants, pants, pants);
    add_ring(&b, 8, 0, 10, 10, 44, 8, 8, pants, pants, pants);
    add_cap(&b, 8, -8, 8, 8, true, pants);
    add_cap(&b, 8, 44, 8, 8, false, pants);
    f->mesh[PART_THIGH] = end_mesh(&b, true);
    begin_mesh(&b);
    add_ring(&b, 8, -6, 7, 7, 0, 8, 8, pants, pants, pants);
    add_cap(&b, 8, -6, 7, 7, true, pants);
    add_ring(&b, 8, 0, 8, 8, 34, 8, 8, pants, pants, pants);
    add_ring(&b, 6, 34, 6, 6, 42, 5, 5, MAT_SKIN, MAT_SKIN, MAT_SKIN);
    f->mesh[PART_SHIN] = end_mesh(&b, true);
    begin_mesh(&b);
    // (the toes in front of the fighter: the box is built with its toes on +Z, turned)
    add_box(&b, -6, 0, -7, 6, 7, 18, 2, MAT_SKIN);
    turn_quads(&b, 0);
    f->mesh[PART_FOOT] = end_mesh(&b, true);
    f->x = x;
    f->dir = x < 0 ? 1 : -1;
    f->health = f->shown_health = HEALTH;
    f->state = STATE_IDLE;
    f->frame = 0;
}

static void                 create_effects(void)
{
    builder                 b;
    jo_fixed                v[4][3];
    int                     i;
    int                     a;

    // Shadow under a fighter: an octagon of the shadow color (the VDP2 darkens the ring under it)
    begin_mesh(&b);
    add_cap(&b, 8, 0, 34, 22, true, MAT_SKIN);
    for (JO_ZERO(i); i < b.count; ++i)
        jo_3d_set_mesh_polygon_color(b.mesh, JO_VDP2_SHADOW_COLOR, i);
    shadow = end_mesh(&b, false);
    // Spark: 8 rays around a core, facing the camera (dual plane)
    begin_mesh(&b);
    for (JO_ZERO(i); i < 8; ++i)
    {
        a = i * 45 + (i & 1) * 10;
        v[0][0] = jo_fixed_mult(jo_int2fixed(3), jo_cos(a + 90)); v[0][1] = jo_fixed_mult(jo_int2fixed(3), jo_sin(a + 90));
        v[1][0] = jo_fixed_mult(jo_int2fixed((i & 1) ? 14 : 22), jo_cos(a)); v[1][1] = jo_fixed_mult(jo_int2fixed((i & 1) ? 14 : 22), jo_sin(a));
        v[2][0] = v[1][0]; v[2][1] = v[1][1];
        v[3][0] = -v[0][0]; v[3][1] = -v[0][1];
        v[0][2] = v[1][2] = v[2][2] = v[3][2] = 0;
        add_quad(&b, (const jo_fixed (*)[3])v, 0, 0, -JO_FIXED_1, (i & 1) ? MAT_SPARK : MAT_SPARK_CORE);
        b.mesh->data.attributes[b.count - 1].plane = JO_3D_DUAL_PLANE;
        jo_3d_set_mesh_polygon_color(b.mesh, (jo_color)(color_of((i & 1) ? MAT_SPARK : MAT_SPARK_CORE) + (SHADES - 1) * MAT_COUNT), b.count - 1);
    }
    spark = end_mesh(&b, false);
    // The side of the ring above the water: its top over the border of the floor (the edge of the VDP2 plane is drawn about
    // 1 unit off: a line of water was seen between the floor and the side), sloped (seen from the inside of the ring, it
    // faces away: not drawn)
    begin_mesh(&b);
    add_square_ring(&b, RING_Y, RING_HALF - 3, RING_Y + 14, RING_HALF + 2, MAT_CURB);
    for (JO_ZERO(i); i < b.count; ++i)
        b.mesh->data.attributes[i].sort = (b.mesh->data.attributes[i].sort & ~JO_3D_SORT_MASK) | JO_3D_SORT_MAX;
    curb = end_mesh(&b, true);
}

/*
** VDP2: the ring and the water (plane A, 1 pixel = 2 units), the sky (plane B)
*/

static void                 create_arena(void)
{
    jo_img_8bits            arena = {ARENA_SIZE, ARENA_SIZE, arena_pixels};
    jo_img_8bits            sky = {SKY_HEIGHT, SKY_WIDTH, sky_pixels};
    int                     x;
    int                     y;
    int                     tx;
    int                     ty;
    int                     ring;
    int                     edge;
    int                     mountain;
    int                     far_mountain;
    unsigned char           c;

    jo_create_palette(&arena_palette);
    // 1-24: stones (3 tones x 8 shades), 25-27: grout and edge, 40-47: water, 64-95: sky, 96-99: mountains
    for (JO_ZERO(x); x < 8; ++x)
    {
        arena_palette.data[x] = JO_COLOR_RGB(196 - x * 7, 186 - x * 7, 164 - x * 7);
        arena_palette.data[8 + x] = JO_COLOR_RGB(176 - x * 7, 160 - x * 7, 132 - x * 7);
        arena_palette.data[16 + x] = JO_COLOR_RGB(158 - x * 6, 150 - x * 6, 140 - x * 6);
    }
    arena_palette.data[24] = JO_COLOR_RGB(84, 76, 66);
    arena_palette.data[25] = JO_COLOR_RGB(120, 108, 92);
    arena_palette.data[26] = JO_COLOR_RGB(60, 54, 48);
    for (JO_ZERO(x); x < WATER_COLORS; ++x)
    {
        water_colors[x] = JO_COLOR_RGB(28 + jo_sin_mult(14, x * 45) + 14, 92 + jo_sin_mult(22, x * 45) + 22, 132 + jo_sin_mult(24, x * 45) + 24);
        arena_palette.data[WATER_FIRST - 1 + x] = water_colors[x];
    }
    for (JO_ZERO(x); x < 32; ++x)
        arena_palette.data[63 + x] = JO_COLOR_RGB(64 + x * 4, 112 + x * 3, 200 + x);
    arena_palette.data[95] = JO_COLOR_RGB(176, 196, 216);
    arena_palette.data[96] = JO_COLOR_RGB(120, 140, 168);
    arena_palette.data[97] = JO_COLOR_RGB(84, 100, 124);
    arena_palette.data[98] = JO_COLOR_RGB(156, 172, 196);
    ring = RING_HALF / ARENA_SCALE;
    for (JO_ZERO(y); y < ARENA_SIZE; ++y)
        for (JO_ZERO(x); x < ARENA_SIZE; ++x)
        {
            tx = x - ARENA_SIZE / 2;
            ty = y - ARENA_SIZE / 2;
            edge = JO_MAX(JO_ABS(tx), JO_ABS(ty));
            if (edge >= ring)
                c = (unsigned char)(WATER_FIRST + JO_MOD_POW2((x * 3 + jo_sin_mult(4, y * 11) + y + jo_sin_mult(3, x * 7)) >> 2, WATER_COLORS));
            else if (edge >= ring - 3)
                c = (unsigned char)(edge == ring - 3 ? 27 : 26);
            else if ((tx + ring) % RING_TILE == 0 || (ty + ring) % RING_TILE == 0)
                c = 25;
            else
            {
                c = (unsigned char)(1 + 8 * (((tx + ring) / RING_TILE * 7 + (ty + ring) / RING_TILE * 3) % 3) + jo_random(3) - 1 +
                                    ((tx + ring) % RING_TILE + (ty + ring) % RING_TILE < 4 ? 0 : 2) +
                                    ((tx + ring) % RING_TILE + (ty + ring) % RING_TILE > RING_TILE * 2 - 6 ? 3 : 0));
            }
            arena_pixels[y * ARENA_SIZE + x] = c;
        }
    // The image of the sky is rotated clockwise: its line x is the column x of the sky, from the bottom to the top
    for (JO_ZERO(x); x < SKY_WIDTH; ++x)
    {
        far_mountain = SKY_HORIZON - 30 - (jo_sin_mult(14, x * 360 / SKY_WIDTH * 2 + 70) + jo_sin_mult(9, x * 360 / SKY_WIDTH * 5) +
                                            jo_cos_mult(4, x * 360 / SKY_WIDTH * 17));
        mountain = SKY_HORIZON - 12 - (jo_sin_mult(16, x * 360 / SKY_WIDTH * 3) + jo_sin_mult(8, x * 360 / SKY_WIDTH * 7 + 40) +
                                       jo_cos_mult(4, x * 360 / SKY_WIDTH * 13) + 8);
        for (JO_ZERO(y); y < SKY_HEIGHT; ++y)
        {
            if (y >= SKY_HORIZON)
                c = WATER_FIRST + 2;
            else if (y >= mountain)
                c = (unsigned char)(y - mountain < 2 ? 99 : 98);
            else if (y >= far_mountain)
                c = (unsigned char)(y - far_mountain < 2 ? 99 : 97);
            else
                c = (unsigned char)(64 + JO_MIN(31, y * 32 / (SKY_HORIZON - 20)));
            sky_pixels[x * SKY_HEIGHT + (SKY_HEIGHT - 1 - y)] = c;
        }
    }
    jo_vdp2_enable_rbg0();
    jo_vdp2_set_rbg0_plane_a_8bits_island(&arena, arena_palette.id, false);
    jo_vdp2_set_rbg0_plane_b_8bits_image(&sky, arena_palette.id, true, false);
    jo_vdp2_set_rbg0_far_limit(4);
    jo_vdp2_enable_shadow(JO_RBG0_SCREEN);
}

/** @brief The waves of the water: its colors turn in the palette */
static void                 animate_water(void)
{
    int                     i;

    if (JO_MOD_POW2(frame_count, 8))
        return;
    for (JO_ZERO(i); i < WATER_COLORS; ++i)
        arena_palette.data[WATER_FIRST - 1 + i] = water_colors[JO_MOD_POW2(i + frame_count / 8, WATER_COLORS)];
}

/*
** Poses
*/

static int                  ease(const int t, const int duration)
{
    return (JO_MIN(t, duration) * 256 / duration);
}

static void                 make_pose(const fighter *f, pose *p)
{
    int                     t;
    int                     k;
    int                     bob;

    jo_memset(p, 0, sizeof(*p));
    bob = jo_sin_mult(2, f->frame * 6);
    p->lean = 8 + bob;
    p->arm[0].shoulder = 60; p->arm[0].spread = 10; p->arm[0].elbow = 110;
    p->arm[1].shoulder = 40; p->arm[1].spread = 14; p->arm[1].elbow = 120;
    p->leg[0].hip = 22; p->leg[0].spread = 4; p->leg[0].knee = 30;
    p->leg[1].hip = -16; p->leg[1].spread = 6; p->leg[1].knee = 22;
    t = f->frame;
    switch (f->state)
    {
        case STATE_WALK:
            k = jo_sin_mult(14, t * 12);
            p->leg[0].hip += (short)(k * f->walk);
            p->leg[1].hip -= (short)(k * f->walk);
            p->leg[0].knee += (short)JO_ABS(k);
            break;
        case STATE_PUNCH:
            k = t < 6 ? ease(t, 6) : t < 10 ? 256 : 256 - ease(t - 10, 8);
            p->arm[0].shoulder = (short)(60 + k * 30 / 256);
            p->arm[0].elbow = (short)(110 - k * 105 / 256);
            p->arm[0].spread = (short)(10 - k * 8 / 256);
            p->twist = (short)(-k * 22 / 256);
            p->lean = (short)(8 + k * 8 / 256);
            break;
        case STATE_KICK:
            if (t < 10)
            {
                k = ease(t, 10);
                p->leg[0].hip = (short)(22 + k * 58 / 256);
                p->leg[0].knee = (short)(30 + k * 70 / 256);
            }
            else if (t < 17)
            {
                k = ease(t - 10, 5);
                p->leg[0].hip = (short)(80 + k * 15 / 256);
                p->leg[0].knee = (short)(100 - k * 92 / 256);
            }
            else
            {
                k = ease(t - 17, 13);
                p->leg[0].hip = (short)(95 - k * 73 / 256);
                p->leg[0].knee = (short)(8 + k * 22 / 256);
            }
            k = t < 17 ? ease(t, 8) : 256 - ease(t - 17, 13);
            p->lean = (short)(8 - k * 24 / 256);
            p->leg[1].knee = (short)(22 + k * 10 / 256);
            p->arm[0].shoulder = (short)(60 - k * 30 / 256);
            break;
        case STATE_GUARD:
            p->arm[0].shoulder = 85; p->arm[0].elbow = 130; p->arm[0].spread = 2;
            p->arm[1].shoulder = 80; p->arm[1].elbow = 135; p->arm[1].spread = 4;
            p->lean = 14;
            p->leg[0].knee = 44; p->leg[1].knee = 36;
            break;
        case STATE_HIT:
            k = t < 4 ? ease(t, 4) : 256 - ease(t - 4, 16);
            p->lean = (short)(8 - k * 30 / 256);
            p->head = (short)(-k * 25 / 256);
            p->arm[0].shoulder = (short)(60 - k * 50 / 256);
            p->arm[1].shoulder = (short)(40 - k * 40 / 256);
            break;
        case STATE_KO:
            k = ease(t, 40);
            p->fall = (short)(-k * 88 / 256);
            p->lean = (short)(8 - k * 20 / 256);
            p->arm[0].shoulder = p->arm[1].shoulder = (short)(60 - k * 150 / 256);
            p->arm[0].spread = p->arm[1].spread = (short)(10 + k * 30 / 256);
            p->leg[0].hip = (short)(22 - k * 12 / 256);
            p->leg[1].hip = (short)(-16 + k * 26 / 256);
            p->leg[0].knee = p->leg[1].knee = (short)(30 - k * 20 / 256);
            break;
        default:
            break;
    }
}

static int                  leg_height(const leg_pose *l)
{
    return (jo_cos_mult(44, l->hip) + jo_cos_mult(42, l->hip - l->knee) + 7);
}

/*
** Drawing (the same code on the 2 CPUs)
*/

static void                 master_push(void) { jo_3d_push_matrix(); }
static void                 master_pop(void) { jo_3d_pop_matrix(); }
static void                 master_translate(const int x, const int y, const int z) { jo_3d_translate_matrix(x, y, z); }
static void                 master_rotate(const int x, const int y, const int z) { jo_3d_rotate_matrix((short)x, (short)y, (short)z); }
static void                 master_draw(jo_3d_mesh *mesh) { jo_3d_mesh_draw(mesh); }
static void                 slave_push(void) { jo_3d_slave_push_matrix(); }
static void                 slave_pop(void) { jo_3d_slave_pop_matrix(); }
static void                 slave_translate(const int x, const int y, const int z)
{
    jo_3d_slave_translate_matrix_fixed(jo_int2fixed(x), jo_int2fixed(y), jo_int2fixed(z));
}
static void                 slave_rotate(const int x, const int y, const int z) { jo_3d_slave_rotate_matrix(x, y, z); }
static void                 slave_draw(jo_3d_mesh *mesh) { jo_3d_slave_mesh_draw(mesh); }

static const renderer       master_renderer = {master_push, master_pop, master_translate, master_rotate, master_draw};
static const renderer       slave_renderer = {slave_push, slave_pop, slave_translate, slave_rotate, slave_draw};

static void                 draw_arm(const renderer *r, const fighter *f, const arm_pose *a, const int side)
{
    r->push();
    r->translate(side * 25, -47, 0);
    r->rotate(-a->shoulder, 0, 0);
    r->rotate(0, 0, side * a->spread);
    r->draw(f->mesh[PART_UPPER_ARM]);
    r->translate(0, 27, 0);
    r->rotate(-a->elbow, 0, 0);
    r->draw(f->mesh[PART_FOREARM]);
    r->draw(f->mesh[PART_FIST]);
    r->pop();
}

static void                 draw_leg(const renderer *r, const fighter *f, const leg_pose *l, const int side)
{
    r->push();
    r->translate(side * 9, 6, 0);
    r->rotate(-l->hip, 0, 0);
    r->rotate(0, 0, side * l->spread);
    r->draw(f->mesh[PART_THIGH]);
    r->translate(0, 43, 0);
    r->rotate(l->knee, 0, 0);
    r->draw(f->mesh[PART_SHIN]);
    r->translate(0, 41, 0);
    r->rotate(l->hip - l->knee, 0, 0);
    r->draw(f->mesh[PART_FOOT]);
    r->pop();
}

static void                 draw_fighter(const renderer *r, const fighter *f)
{
    pose                    p;
    int                     hip;

    make_pose(f, &p);
    hip = JO_MAX(leg_height(&p.leg[0]), leg_height(&p.leg[1])) + 6;
    if (f->state == STATE_KO)
        hip = hip - (hip - 14) * ease(f->frame, 40) / 256;
    r->push();
    r->translate(f->x, RING_Y - hip, 0);
    r->rotate(0, f->dir > 0 ? -90 : 90, 0);
    r->rotate(p.fall, 0, 0);
    r->draw(f->mesh[PART_PELVIS]);
    r->push();
    r->rotate(0, p.twist, 0);
    r->rotate(-p.lean, 0, 0);
    r->draw(f->mesh[PART_TORSO]);
    r->push();
    r->translate(0, -57, 0);
    r->rotate(-p.head, 0, 0);
    r->draw(f->mesh[PART_HEAD]);
    r->pop();
    draw_arm(r, f, &p.arm[0], 1);
    draw_arm(r, f, &p.arm[1], -1);
    r->pop();
    draw_leg(r, f, &p.leg[0], 1);
    draw_leg(r, f, &p.leg[1], -1);
    r->pop();
}

static void                 slave_draw_fighter(void)
{
    draw_fighter(&slave_renderer, &fighters[1]);
}

static void                 draw_effects(void)
{
    int                     i;
    int                     s;

    // The side of the ring and the shadows first (far behind the fighters in the sort)
    jo_3d_set_depth_bias(4000);
    jo_3d_mesh_draw(curb);
    for (JO_ZERO(i); i < 2; ++i)
    {
        jo_3d_push_matrix();
        jo_3d_translate_matrix(fighters[i].x + (fighters[i].state == STATE_KO ? -fighters[i].dir * 40 * ease(fighters[i].frame, 40) / 256 : 0),
                               RING_Y, 0);
        jo_3d_mesh_draw(shadow);
        jo_3d_pop_matrix();
    }
    jo_3d_set_depth_bias(0);
    if (spark_frames <= 0)
        return;
    s = 1 + (SPARK_FRAMES - spark_frames) / 2;
    jo_3d_set_depth_bias(-60);
    jo_3d_push_matrix();
    jo_3d_translate_matrix(spark_x, spark_y, -20);
    jo_3d_rotate_matrix(0, 0, (short)(spark_frames * 17));
    jo_3d_set_scale(s, s, s);
    jo_3d_mesh_draw(spark);
    jo_3d_pop_matrix();
    jo_3d_set_depth_bias(0);
}

/*
** Fight
*/

static void                 set_state(fighter *f, const fighter_state state)
{
    f->state = state;
    f->frame = 0;
    f->hit_done = false;
}

static bool                 is_busy(const fighter *f)
{
    return ((f->state == STATE_PUNCH && f->frame < 18) || (f->state == STATE_KICK && f->frame < 30) ||
            (f->state == STATE_HIT && f->frame < 20) || f->state == STATE_KO);
}

static void                 attack(fighter *f, fighter *target, const int first, const int last, const int reach, const int damage)
{
    if (f->hit_done || f->frame < first || f->frame > last || JO_ABS(target->x - f->x) > reach || target->state == STATE_KO)
        return;
    f->hit_done = true;
    spark_frames = SPARK_FRAMES;
    spark_x = target->x - f->dir * 14;
    spark_y = RING_Y - (reach > 80 ? 80 : 118);
    if (target->state == STATE_GUARD)
    {
        target->x += f->dir * 6;
        return;
    }
    shake = damage;
    flash = 4;
    target->health -= damage;
    if (target->health <= 0)
    {
        target->health = 0;
        set_state(target, STATE_KO);
        ++f->wins;
        ko_frames = 0;
    }
    else
        set_state(target, STATE_HIT);
}

static void                 think(fighter *f, const fighter *other, int *move, bool *punch, bool *kick, bool *guard)
{
    int                     dist;

    dist = JO_ABS(other->x - f->x);
    if (--f->think > 0)
    {
        *move = f->walk;
        *guard = f->state == STATE_GUARD;
        return;
    }
    f->think = 7 + jo_random(16);
    if ((other->state == STATE_PUNCH || other->state == STATE_KICK) && dist < 110 && jo_random(3) == 1)
        *guard = true;
    else if (dist > 100)
        *move = 1;
    else if (dist < 50)
        *move = -1;
    else if (jo_random(3) == 1)
        *kick = true;
    else if (jo_random(2) == 1)
        *punch = true;
    else
        *move = jo_random(3) - 2;
}

static void                 update_fighter(fighter *f, fighter *other, const bool pad)
{
    int                     move;
    bool                    punch;
    bool                    kick;
    bool                    guard;

    ++f->frame;
    f->dir = other->x > f->x ? 1 : -1;
    if (f->state == STATE_PUNCH)
        attack(f, other, 5, 9, 72, 7);
    else if (f->state == STATE_KICK)
        attack(f, other, 9, 16, 98, 13);
    else if (f->state == STATE_HIT && f->frame < 10)
        f->x -= f->dir * 3;
    if (is_busy(f) || ko_frames > 0)
        return;
    move = 0;
    punch = kick = guard = false;
    if (pad)
    {
        move = jo_is_pad1_key_pressed(JO_KEY_RIGHT) ? f->dir : jo_is_pad1_key_pressed(JO_KEY_LEFT) ? -f->dir : 0;
        guard = jo_is_pad1_key_pressed(JO_KEY_DOWN);
        punch = jo_is_pad1_key_down(JO_KEY_A);
        kick = jo_is_pad1_key_down(JO_KEY_B);
    }
    else
        think(f, other, &move, &punch, &kick, &guard);
    if (punch)
        set_state(f, STATE_PUNCH);
    else if (kick)
        set_state(f, STATE_KICK);
    else if (guard)
    {
        if (f->state != STATE_GUARD)
            set_state(f, STATE_GUARD);
    }
    else if (move)
    {
        if (f->state != STATE_WALK)
            set_state(f, STATE_WALK);
        f->walk = move;
        if (move < 0 || JO_ABS(other->x - f->x) > 60)
            f->x += f->dir * move * 2;
    }
    else if (f->state != STATE_IDLE)
        set_state(f, STATE_IDLE);
}

static void                 new_round(void)
{
    int                     i;

    for (JO_ZERO(i); i < 2; ++i)
    {
        fighters[i].x = i ? 70 : -70;
        fighters[i].health = fighters[i].shown_health = HEALTH;
        set_state(&fighters[i], STATE_IDLE);
    }
    ko_frames = 0;
    round_frames = 0;
}

/*
** Head-up display: the health bars (VDP1 rectangles in front of everything, the red part shows the last damages)
*/

#define HUD_Z               (500)

/** @brief A rectangle of the head-up display from x0 to x1 (the pixels of both included, 2 pixels at least) */
static void                 draw_hud_rect(const int x0, int x1, const int y0, const int y1, const material m)
{
    if (x1 <= x0)
        x1 = x0 + 1;
    // (a color of the palette of the screen in 704 x 480: the brightest shades of the material)
    jo_sprite_draw_rectangle(x0, y0, x1 - x0 + 1, y1 - y0 + 1, (jo_color)(color_of(m) + (SHADES - 3) * MAT_COUNT), HUD_Z);
}

static void                 draw_hud(void)
{
    int                     i;
    int                     x;
    int                     w;

    // In front of the 3D: their sort key is 0 (the depth bias added to HUD_Z), the 3D polygons are farther than the
    // near plane; at the same depth, drawn in the order of the calls (the frame, the red part, the yellow part)
    jo_3d_set_depth_bias(-HUD_Z);
    for (JO_ZERO(i); i < 2; ++i)
    {
        if (fighters[i].shown_health > fighters[i].health)
            --fighters[i].shown_health;
        else
            fighters[i].shown_health = fighters[i].health;
        x = i ? 386 : 38;
        w = 280;
        draw_hud_rect(x - 3, x + w + 3, 32, 54, MAT_BELT);
        if (i)
        {
            draw_hud_rect(x + w - w * fighters[i].shown_health / HEALTH, x + w, 35, 51, MAT_GI_RED);
            draw_hud_rect(x + w - w * fighters[i].health / HEALTH, x + w, 35, 51, MAT_SPARK);
        }
        else
        {
            draw_hud_rect(x, x + w * fighters[i].shown_health / HEALTH, 35, 51, MAT_GI_RED);
            draw_hud_rect(x, x + w * fighters[i].health / HEALTH, 35, 51, MAT_SPARK);
        }
    }
    jo_3d_set_depth_bias(0);
}

/*
** Main loop
*/

static void                 update_camera(void)
{
    int                     mid;
    int                     dist;
    int                     a;
    int                     cx;
    int                     cz;
    int                     horizon;
    int                     height;
    int                     sx;
    int                     sy;

    mid = (fighters[0].x + fighters[1].x) / 2;
    dist = 175 + JO_ABS(fighters[1].x - fighters[0].x) * 2 / 3;
    if (ko_frames > 0)
        dist -= JO_MIN(ko_frames, 60) * 50 / 60;
    a = jo_sin_mult(26, frame_count / 4);
    height = 92 + jo_sin_mult(10, frame_count / 3);
    sx = shake ? jo_random(shake) - 1 - shake / 2 : 0;
    sy = shake ? jo_random(shake) - 1 - shake / 2 : 0;
    cx = mid + jo_sin_mult(dist, a);
    cz = -jo_cos_mult(dist, a);
    jo_3d_camera_set_viewpoint(&camera, cx + sx, RING_Y - height + sy, cz);
    jo_3d_camera_set_target(&camera, mid + sx, RING_Y - 78 + sy, 0);
    jo_3d_camera_look_at(&camera);
    jo_3d_light_fixed(jo_float2fixed(0.42f), jo_float2fixed(0.72f), jo_float2fixed(0.55f));
    // The sky: its horizon on the horizon of the camera, its mountains turn with the camera
    horizon = jo_core_get_screen_height() / 2 - jo_fixed2int(jo_fixed_mult(jo_3d_get_projection_distance(),
                                                                             jo_fixed_div(jo_int2fixed(height - 78), jo_int2fixed(dist))));
    jo_vdp2_draw_rbg0_plane_b_flat(-a * jo_core_get_screen_width() * 4 / 360 - mid, SKY_HORIZON - horizon);
}

static void                 draw_arena(void)
{
    jo_3d_push_matrix();
    {
        jo_3d_translate_matrix(0, RING_Y, 0);
        jo_3d_rotate_matrix(90, 0, 0);
        jo_3d_set_scale(ARENA_SCALE, ARENA_SCALE, ARENA_SCALE);
        jo_3d_translate_matrix(-ARENA_SIZE / 2, -ARENA_SIZE / 2, 0);
        jo_vdp2_draw_rbg0_plane_a(true);
    }
    jo_3d_pop_matrix();
}

static void                 game_loop(void)
{
    ++frame_count;
    ++round_frames;
    if (jo_is_pad1_available() && (jo_is_pad1_key_pressed(JO_KEY_A) || jo_is_pad1_key_pressed(JO_KEY_B) ||
                                   jo_is_pad1_key_pressed(JO_KEY_LEFT) || jo_is_pad1_key_pressed(JO_KEY_RIGHT)))
        player = true;
    update_fighter(&fighters[0], &fighters[1], player);
    update_fighter(&fighters[1], &fighters[0], false);
    // On the ring: walking back, pushed by a hit or a guard (a fighter knocked out falls backward: his body stays on it)
    fighters[0].x = JO_CLAMP(fighters[0].x, -FIGHTER_LIMIT, FIGHTER_LIMIT);
    fighters[1].x = JO_CLAMP(fighters[1].x, -FIGHTER_LIMIT, FIGHTER_LIMIT);
    if (fighters[0].state == STATE_KO || fighters[1].state == STATE_KO)
        if (++ko_frames > 180)
            new_round();
    if (spark_frames > 0)
        --spark_frames;
    if (shake > 0)
        shake = shake * 3 / 4;
    if (flash > 0)
        jo_set_screen_color_filter_a(JO_RBG0_SCREEN | JO_SPRITE_SCREEN, (short)(flash * 24), (short)(flash * 20), (short)(flash * 12));
    else if (flash == 0)
        jo_disable_screen_color_filter(JO_RBG0_SCREEN | JO_SPRITE_SCREEN);
    --flash;
    animate_water();
    draw_hud();
    update_camera();
    draw_arena();
    jo_3d_slave_begin();
    jo_core_exec_on_slave(slave_draw_fighter);
    draw_fighter(&master_renderer, &fighters[0]);
    draw_effects();
    jo_core_wait_for_slave();
    jo_3d_slave_end();
    jo_printf(3, 4, "RED %d", fighters[0].wins);
    jo_printf(34, 4, "%d WHITE", fighters[1].wins);
    jo_printf(19, 6, ko_frames > 0 ? "K.O.  " : round_frames < 90 ? "FIGHT!" : "      ");
    jo_printf(2, 27, "%d FPS  %s  LATE %d   ", jo_core_get_fps(), player ? "PLAYER 1" : "DEMO", jo_core_get_late_frame_count());
#ifdef JO_COMPILE_WITH_PROFILER_SUPPORT
    jo_profile_print(1, 8);
#endif
}

void                        jo_main(void)
{
    jo_core_init(JO_COLOR_RGB(150, 176, 204));
    jo_create_palette(&palette);
    jo_palette_set_light_shades(&palette, SHADES_FIRST, material_colors, MAT_COUNT, SHADES);
    jo_3d_set_palette_light(SHADES, MAT_COUNT);
    jo_core_set_screen_palette(palette.id);
    create_arena();
    create_fighter(&fighters[0], MAT_GI_RED, MAT_TRIM_RED, MAT_PANTS_BLUE, -70);
    create_fighter(&fighters[1], MAT_GI_WHITE, MAT_TRIM_WHITE, MAT_PANTS_BLACK, 70);
    create_effects();
    jo_3d_camera_init(&camera);
    new_round();
    jo_core_add_callback(game_loop);
    jo_core_run();
}
