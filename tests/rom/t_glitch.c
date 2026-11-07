/*
** Jo Sega Saturn Engine - regression test ROM: the anti-glitch views of the 3D (checked by tests/harness/glitch.py)
**
** Each view is a scene (JT_STEP_SCENE) of G_FRAMES frames: a known geometry seen from a known camera, over a magenta
** background (the NBG1 bitmap behind the sprites) that no texture uses. The harness computes what each frame must show
** (rays through the room and the ground, the projection of the quads, the texel of each pixel) and reports the holes,
** the cracks, the deformations, the texture mapping and the colors. The views are in t_glitch_views.h (written by
** glitch.py --write-views).
**
** Scenes: a closed room seen from inside (6 faces of 8 x 8 quads sharing their vertices, or 6 big quads, textured or
** flat), a ground of 24 x 24 quads (flat checker, textured with subdivision, with mipmaps, flipped with mipmaps), one quad.
** The big quads (the room of 6 quads, the quad) are textured with the finer tiles (jo_3d_set_texture_subdivision_fine()).
** The texture is a gradient: red = column, green = line, blue = 31 - column (every texel has its color).
*/

#include "jt.h"
#include "tests.h"

/** @brief Scenes of the views (see glitch.py SCENES) */
enum
{
    G_SCENE_ROOM_GRID,          // 8 x 8 quads per face, the gradient with subdivision
    G_SCENE_ROOM_FLAT,          // 8 x 8 quads per face, a flat color per face
    G_SCENE_ROOM_BIG,           // 1 quad per face, the gradient with subdivision
    G_SCENE_ROOM_BIG_PLAIN,     // 1 quad per face, the gradient without subdivision
    G_SCENE_GROUND_FLAT,        // 24 x 24 quads, flat checker
    G_SCENE_GROUND_TEX,         // the gradient with subdivision
    G_SCENE_GROUND_MIP,         // the gradient with mipmaps
    G_SCENE_GROUND_MIP_FLIP,    // the gradient with mipmaps, flipped horizontally (FLIP_H)
    G_SCENE_QUAD,               // one quad of g_quads
    G_SCENE_COUNT
};

/** @brief Texture or color of the quad of a view */
enum
{
    G_MODE_NONE,
    G_MODE_FLAT,                // flat color (g_flat_colors[8])
    G_MODE_PLAIN,               // the gradient without subdivision
    G_MODE_SUB,                 // the gradient with subdivision
    G_MODE_MIP                  // the gradient with mipmaps
};

/** @brief Flags of a view */
#define G_F_QUICK               (1)     // also in the quick run (before the report of the checks)
#define G_F_SLAVE               (2)     // drawn by the slave CPU (the same picture as the master CPU)
#define G_F_DEFECT              (4)     // a known defect drawn: the checks must fail (see g_draw())
#define G_F_HULL                (8)     // a twisted quad or seen edge on: only the pixels out of its convex hull
#define G_F_CONTROL             (16)    // a known bad case (no subdivision through the near plane): the checks must fail

typedef struct
{
    const char                  *name;
    unsigned char               scene;
    unsigned char               mode;
    unsigned short              flags;
    jo_fixed                    cam[3];
    jo_fixed                    target[3];
    short                       roll;
    short                       quad;
}                               g_view;

#define G_F(X)                  JO_FLOAT_TO_FIXED(X)
#define G_COLOR(R, G, B)        {(R), (G), (B)}
#define G_QUAD(X0, Y0, Z0, X1, Y1, Z1, X2, Y2, Z2, X3, Y3, Z3) \
    {{G_F(X0), G_F(Y0), G_F(Z0)}, {G_F(X1), G_F(Y1), G_F(Z1)}, {G_F(X2), G_F(Y2), G_F(Z2)}, {G_F(X3), G_F(Y3), G_F(Z3)}}
#define G_VIEW(NAME, SCENE, MODE, FLAGS, CX, CY, CZ, TX, TY, TZ, ROLL, QUAD) \
    {(NAME), G_SCENE_##SCENE, G_MODE_##MODE, (FLAGS), {G_F(CX), G_F(CY), G_F(CZ)}, {G_F(TX), G_F(TY), G_F(TZ)}, (ROLL), (QUAD)}

#include "t_glitch_views.h"

/** @brief The background: no texture of the views uses it */
#define G_BACKGROUND            JO_COLOR_RGB(255, 0, 255)

/** @brief The last frames of a view: nothing drawn (see g_draw()) */
#define G_TAIL_FRAMES           (3)

/** @brief Ground: the first column of quads not moved by G_SCENE_GROUND_FLAT + G_F_DEFECT (a crack along x = 0) */
#define G_CRACK_COLUMN          (12)

static jo_3d_mesh               *g_room[2];         // 8 x 8, 1 x 1 per face
static jo_3d_mesh               g_room_variants[G_SCENE_COUNT];
static jo_3d_mesh               *g_ground;
static jo_3d_mesh               g_ground_variants[G_SCENE_COUNT];
static jo_3d_mesh               *g_quad;
static jo_3d_mesh               *g_slave_mesh;
static int                      g_first_sprite = -1;
static int                      g_tex_plain;
static int                      g_tex_sub;
static int                      g_tex_mip;
static int                      g_tex_fine;         // With the finer tiles (the big quads: a magnified texture)
static unsigned short           g_pixels[G_TEX * G_TEX];
static int                      g_current;
static int                      g_frame;            // Frames of the view drawn
static int                      g_quick_left;
static int                      g_full_left;

/** @brief The memory of the meshes: one block (allocated and freed around the views: the measures of the memory and its
 *  fragments see no difference) */
#define G_POINTS                (6 * (G_ROOM_N + 1) * (G_ROOM_N + 1) + 6 * 4 + (G_GROUND_N + 1) * (G_GROUND_N + 1) + 4)
#define G_POLYGONS              (6 * G_ROOM_N * G_ROOM_N + 6 + G_GROUND_N * G_GROUND_N + 1)
#define G_ATTRIBUTES            (G_POLYGONS + 6 * G_ROOM_N * G_ROOM_N + 6 + 3 * G_GROUND_N * G_GROUND_N)
#define G_HEAP_SIZE             (G_POINTS * sizeof(jo_pos3D_fixed) + G_POLYGONS * sizeof(jo_3d_polygon) + \
                                 G_ATTRIBUTES * sizeof(jo_3d_polygon_attribute) + 64)
static char                     *g_heap;
static unsigned int             g_heap_used;
static jo_3d_mesh               g_meshes[4];
static int                      g_mesh_count;

static void                     *g_alloc(const unsigned int size)
{
    void                        *p;

    if (g_heap == JO_NULL || g_heap_used + size > G_HEAP_SIZE)
        return (JO_NULL);
    p = g_heap + g_heap_used;
    g_heap_used += (size + 3) & ~3u;
    jo_memset(p, 0, size);
    return (p);
}

/** @brief A mesh of the block (see jo_3d_create_indexed_mesh(): the polygons single plane, sorted on their center) */
static jo_3d_mesh               *g_new_mesh(const unsigned int point_count, const unsigned int polygon_count)
{
    jo_3d_mesh                  *mesh;

    mesh = &g_meshes[g_mesh_count++];
    mesh->data.point_count = point_count;
    mesh->data.polygon_count = polygon_count;
    mesh->data.points = (jo_pos3D_fixed *)g_alloc(point_count * sizeof(jo_pos3D_fixed));
    mesh->data.polygons = (jo_3d_polygon *)g_alloc(polygon_count * sizeof(jo_3d_polygon));
    mesh->data.attributes = (jo_3d_polygon_attribute *)g_alloc(polygon_count * sizeof(jo_3d_polygon_attribute));
    if (mesh->data.points == JO_NULL || mesh->data.polygons == JO_NULL || mesh->data.attributes == JO_NULL)
        return (JO_NULL);
    return (mesh);
}

static jo_color                 g_flat(const int i)
{
    return (JO_COLOR_RGB(g_flat_colors[i][0], g_flat_colors[i][1], g_flat_colors[i][2]));
}

/** @brief A copy of a mesh with its own attributes (the points and the polygons shared) */
static bool                     g_variant(jo_3d_mesh * const out, const jo_3d_mesh * const mesh)
{
    *out = *mesh;
    out->data.attributes = (jo_3d_polygon_attribute *)g_alloc(mesh->data.polygon_count * sizeof(jo_3d_polygon_attribute));
    if (out->data.attributes == JO_NULL)
        return (false);
    jo_memcpy(out->data.attributes, mesh->data.attributes, mesh->data.polygon_count * sizeof(jo_3d_polygon_attribute));
    return (true);
}

/** @brief The room: the faces of a box seen from inside, n x n quads per face (the corners of the face: glitch.py
 *  _scene_geometry(), the same order) */
static jo_3d_mesh               *g_make_room(const int n)
{
    static const int            faces[6][3][3] =
    {
        {{-1, -1, 1}, {1, 0, 0}, {0, 1, 0}}, {{1, -1, -1}, {-1, 0, 0}, {0, 1, 0}}, {{1, -1, 1}, {0, 0, -1}, {0, 1, 0}},
        {{-1, -1, -1}, {0, 0, 1}, {0, 1, 0}}, {{-1, 1, 1}, {1, 0, 0}, {0, 0, -1}}, {{-1, -1, -1}, {1, 0, 0}, {0, 0, 1}}
    };
    jo_3d_mesh                  *mesh;
    jo_3d_polygon               *p;
    int                         f;
    int                         i;
    int                         j;
    int                         k;
    int                         base;
    int                         step;

    mesh = g_new_mesh(6 * (n + 1) * (n + 1), 6 * n * n);
    if (mesh == JO_NULL)
        return (JO_NULL);
    step = 2 * G_ROOM_HALF / n;
    for (f = 0; f < 6; ++f)
    {
        base = f * (n + 1) * (n + 1);
        for (j = 0; j <= n; ++j)
            for (i = 0; i <= n; ++i)
                for (k = 0; k < 3; ++k)
                    ((jo_fixed *)&mesh->data.points[base + j * (n + 1) + i])[k] =
                        jo_int2fixed(faces[f][0][k] * G_ROOM_HALF + faces[f][1][k] * i * step + faces[f][2][k] * j * step);
        for (j = 0; j < n; ++j)
            for (i = 0; i < n; ++i)
            {
                p = &mesh->data.polygons[(f * n + j) * n + i];
                p->vertices[0] = base + j * (n + 1) + i;
                p->vertices[1] = base + j * (n + 1) + i + 1;
                p->vertices[2] = base + (j + 1) * (n + 1) + i + 1;
                p->vertices[3] = base + (j + 1) * (n + 1) + i;
                jo_3d_set_mesh_polygon_texture(mesh, g_tex_sub, (f * n + j) * n + i);
            }
    }
    return (mesh);
}

/** @brief The ground at y = 0, n x n quads, seen from above (y < 0) */
static jo_3d_mesh               *g_make_ground(void)
{
    jo_3d_mesh                  *mesh;
    jo_3d_polygon               *p;
    int                         i;
    int                         j;
    int                         n;
    int                         step;

    n = G_GROUND_N;
    mesh = g_new_mesh((n + 1) * (n + 1), n * n);
    if (mesh == JO_NULL)
        return (JO_NULL);
    step = 2 * G_GROUND_HALF / n;
    for (j = 0; j <= n; ++j)
        for (i = 0; i <= n; ++i)
        {
            mesh->data.points[j * (n + 1) + i].x = jo_int2fixed(-G_GROUND_HALF + i * step);
            mesh->data.points[j * (n + 1) + i].y = 0;
            mesh->data.points[j * (n + 1) + i].z = jo_int2fixed(G_GROUND_HALF - j * step);
        }
    for (j = 0; j < n; ++j)
        for (i = 0; i < n; ++i)
        {
            p = &mesh->data.polygons[j * n + i];
            p->vertices[0] = j * (n + 1) + i;
            p->vertices[1] = j * (n + 1) + i + 1;
            p->vertices[2] = (j + 1) * (n + 1) + i + 1;
            p->vertices[3] = (j + 1) * (n + 1) + i;
            jo_3d_set_mesh_polygon_color(mesh, g_flat(6 + ((i + j) & 1)), j * n + i);
        }
    return (mesh);
}

static void                     g_set_texture(jo_3d_mesh * const mesh, const int sprite, const bool flip)
{
    unsigned int                i;

    for (i = 0; i < mesh->data.polygon_count; ++i)
    {
        jo_3d_set_mesh_polygon_texture(mesh, sprite, i);
        if (flip)
            mesh->data.attributes[i].direction |= JO_VDP1_H_FLIP;
    }
}

/** @brief The textures and the meshes (created at the first view of a run, freed after its last view: the memory measures and
 *  the leak checks don't see them) */
static void                     g_create(void)
{
    jo_img                      img;
    int                         x;
    int                         y;
    int                         f;
    int                         i;
    int                         n;

    for (y = 0; y < G_TEX; ++y)
        for (x = 0; x < G_TEX; ++x)
            g_pixels[y * G_TEX + x] = JO_COLOR_RGB(x << 3, y << 3, (31 - x) << 3);
    img.width = G_TEX;
    img.height = G_TEX;
    img.data = g_pixels;
    g_tex_plain = jo_sprite_add(&img);
    g_first_sprite = g_tex_plain;
    g_tex_sub = jo_sprite_add(&img);
    g_tex_mip = jo_sprite_add(&img);
    g_tex_fine = jo_sprite_add(&img);
    JT_TRUE(jo_3d_set_texture_subdivision(g_tex_sub));
    JT_TRUE(jo_3d_set_texture_mipmaps(g_tex_mip));
    JT_TRUE(jo_3d_set_texture_subdivision_fine(g_tex_fine));
    g_heap = (char *)jo_malloc(G_HEAP_SIZE);
    g_heap_used = 0;
    g_mesh_count = 0;
    g_room[0] = g_make_room(G_ROOM_N);
    g_room[1] = g_make_room(1);
    g_ground = g_make_ground();
    g_quad = g_new_mesh(4, 1);
    if (g_room[0] == JO_NULL || g_room[1] == JO_NULL || g_ground == JO_NULL || g_quad == JO_NULL)
    {
        jt_info("glitch: out of memory");
        return;
    }
    g_quad->data.polygons[0].vertices[0] = 0;
    g_quad->data.polygons[0].vertices[1] = 1;
    g_quad->data.polygons[0].vertices[2] = 2;
    g_quad->data.polygons[0].vertices[3] = 3;
    g_room_variants[G_SCENE_ROOM_GRID] = *g_room[0];
    g_room_variants[G_SCENE_ROOM_BIG] = *g_room[1];
    g_variant(&g_room_variants[G_SCENE_ROOM_FLAT], g_room[0]);
    n = G_ROOM_N * G_ROOM_N;
    for (f = 0; f < 6; ++f)
        for (i = 0; i < n; ++i)
            jo_3d_set_mesh_polygon_color(&g_room_variants[G_SCENE_ROOM_FLAT], g_flat(f), f * n + i);
    // (the big quads: 12.5 units per texel, 95 pixels per texel 21 units away: the finer tiles, see
    // jo_3d_set_texture_subdivision_fine())
    g_set_texture(&g_room_variants[G_SCENE_ROOM_BIG], g_tex_fine, false);
    g_variant(&g_room_variants[G_SCENE_ROOM_BIG_PLAIN], g_room[1]);
    g_set_texture(&g_room_variants[G_SCENE_ROOM_BIG_PLAIN], g_tex_plain, false);
    g_ground_variants[G_SCENE_GROUND_FLAT] = *g_ground;
    g_variant(&g_ground_variants[G_SCENE_GROUND_TEX], g_ground);
    g_set_texture(&g_ground_variants[G_SCENE_GROUND_TEX], g_tex_sub, false);
    g_variant(&g_ground_variants[G_SCENE_GROUND_MIP], g_ground);
    g_set_texture(&g_ground_variants[G_SCENE_GROUND_MIP], g_tex_mip, false);
    g_variant(&g_ground_variants[G_SCENE_GROUND_MIP_FLIP], g_ground);
    g_set_texture(&g_ground_variants[G_SCENE_GROUND_MIP_FLIP], g_tex_mip, true);
}

static void                     g_free(void)
{
    jo_memset(g_room_variants, 0, sizeof(g_room_variants));
    jo_memset(g_ground_variants, 0, sizeof(g_ground_variants));
    if (g_heap != JO_NULL)
        jo_free(g_heap);
    g_heap = JO_NULL;
    g_quad = JO_NULL;
    g_ground = JO_NULL;
    g_room[0] = JO_NULL;
    g_room[1] = JO_NULL;
    if (g_first_sprite >= 0)
        jo_3d_model_free_textures_from(g_first_sprite);
    g_first_sprite = -1;
}

/** @brief The mesh of a view (the quad: its corners and its texture set) */
static jo_3d_mesh               *g_mesh(const g_view * const v)
{
    jo_3d_mesh                  *mesh;
    int                         i;

    switch (v->scene)
    {
        case G_SCENE_ROOM_GRID:
        case G_SCENE_ROOM_FLAT:
        case G_SCENE_ROOM_BIG:
        case G_SCENE_ROOM_BIG_PLAIN:
            return (&g_room_variants[v->scene]);
        case G_SCENE_GROUND_MIP_FLIP:
            // (the known defect: drawn not flipped, the check expects it flipped)
            if (v->flags & G_F_DEFECT)
                return (&g_ground_variants[G_SCENE_GROUND_MIP]);
            return (&g_ground_variants[v->scene]);
        case G_SCENE_GROUND_FLAT:
        case G_SCENE_GROUND_TEX:
        case G_SCENE_GROUND_MIP:
            return (&g_ground_variants[v->scene]);
        default:
            break;
    }
    mesh = g_quad;
    for (i = 0; i < 4; ++i)
    {
        mesh->data.points[i].x = g_quads[v->quad][i][0];
        mesh->data.points[i].y = g_quads[v->quad][i][1];
        mesh->data.points[i].z = g_quads[v->quad][i][2];
    }
    // (the known defect: a corner moved by 8 units)
    if (v->flags & G_F_DEFECT)
    {
        mesh->data.points[2].x += JO_MULT_BY_65536(8);
        mesh->data.points[2].y += JO_MULT_BY_65536(8);
    }
    if (v->mode == G_MODE_FLAT)
        jo_3d_set_mesh_polygon_color(mesh, g_flat(8), 0);
    else
        jo_3d_set_mesh_polygon_texture(mesh, v->mode == G_MODE_SUB ? g_tex_fine : (v->mode == G_MODE_MIP ? g_tex_mip : g_tex_plain), 0);
    return (mesh);
}

static void                     g_draw_mesh(jo_3d_mesh * const mesh, const g_view * const v)
{
    jo_3d_polygon_range         ranges[2];
    jo_3d_polygon_range         halves[G_GROUND_N];
    int                         n;

    if ((v->flags & G_F_DEFECT) && v->scene == G_SCENE_ROOM_GRID)
    {
        // The known defect: a quad of the front wall missing (the face z = +H, its row 3, its column 4)
        n = 3 * G_ROOM_N + 4;
        ranges[0].first = 0;
        ranges[0].count = (unsigned short)n;
        ranges[1].first = (unsigned short)(n + 1);
        ranges[1].count = (unsigned short)(mesh->data.polygon_count - n - 1);
        jo_3d_mesh_draw_ranges(mesh, mesh->data.point_count, ranges, 2);
    }
    else if ((v->flags & G_F_DEFECT) && v->scene == G_SCENE_GROUND_FLAT)
    {
        // The known defect: a crack, the left half of the ground (its columns before G_CRACK_COLUMN) moved by 2 units
        for (n = 0; n < G_GROUND_N; ++n)
        {
            halves[n].first = (unsigned short)(n * G_GROUND_N + G_CRACK_COLUMN);
            halves[n].count = (unsigned short)(G_GROUND_N - G_CRACK_COLUMN);
        }
        jo_3d_mesh_draw_ranges(mesh, mesh->data.point_count, halves, G_GROUND_N);
        jo_3d_push_matrix();
        jo_3d_translate_matrix(-2, 0, 0);
        for (n = 0; n < G_GROUND_N; ++n)
        {
            halves[n].first = (unsigned short)(n * G_GROUND_N);
            halves[n].count = (unsigned short)G_CRACK_COLUMN;
        }
        jo_3d_mesh_draw_ranges(mesh, mesh->data.point_count, halves, G_GROUND_N);
        jo_3d_pop_matrix();
    }
    else
        jo_3d_mesh_draw(mesh);
}

static void                     g_slave_draw(void)
{
    jo_3d_slave_mesh_draw(g_slave_mesh);
}

static void                     g_draw(void)
{
    const g_view                *v;
    jo_camera                   cam;
    jo_3d_mesh                  *mesh;

    v = &g_views[g_current];
    if (g_quad == JO_NULL)
        return;
    // The last frames of the view: nothing drawn, a black background (the next marker is not hidden by the polygons of
    // the view still shown, and the harness knows the frames of the view: no black in them, see glitch.py)
    if (++g_frame > G_FRAMES - G_TAIL_FRAMES)
    {
        if (g_frame == G_FRAMES - G_TAIL_FRAMES + 1)
            jo_clear_background(JO_COLOR_Black);
        return;
    }
    cam.viewpoint[0] = v->cam[0];
    cam.viewpoint[1] = v->cam[1];
    cam.viewpoint[2] = v->cam[2];
    cam.target[0] = v->target[0];
    cam.target[1] = v->target[1];
    cam.target[2] = v->target[2];
    jo_3d_camera_set_z_angle(&cam, v->roll);
    jo_3d_camera_look_at(&cam);
    mesh = g_mesh(v);
    if (v->flags & G_F_SLAVE)
    {
        g_slave_mesh = mesh;
        jo_3d_slave_begin();
        jo_core_exec_on_slave(g_slave_draw);
        jo_core_wait_for_slave();
        jo_3d_slave_end();
    }
    else
        g_draw_mesh(mesh, v);
}

static void                     g_setup(void)
{
    const jt_step               *step;

    // (the first view of a run: the textures and the meshes)
    if (g_quad == JO_NULL && g_first_sprite < 0)
        g_create();
    // (the view of the step: its name)
    step = jt_current_step();
    for (g_current = 0; g_current < JT_COUNT(g_views) - 1 && jo_strcmp(g_views[g_current].name, step->name) != 0; ++g_current)
        ;
    g_frame = 0;
    jo_clear_background(G_BACKGROUND);
}

static void                     g_cleanup(void)
{
    bool                        last;

    if (g_views[g_current].flags & G_F_QUICK)
        last = --g_quick_left == 0;
    else
        last = --g_full_left == 0;
    if (last)
    {
        g_free();
        jt_restore_nbg1();
    }
}

int                             t_glitch_steps(const bool quick, jt_step * const steps, const int max)
{
    int                         count;
    int                         i;

    // (the views need the 320 or 352 pixels modes: RGB textures. Not in the variants of the long fuzz: the same views as the
    // other variants)
#ifdef T_FUZZ_LONG
    return (0);
#endif
    if (__jo_screen.hires)
        return (0);
    if (quick)
    {
        // The projection of the views (glitch.py: PD, NEAR, CX, CY)
        jt_group("glitch");
        JT_EQ(jo_3d_get_projection_distance(), JO_MULT_BY_65536(160));
        JT_EQ(__jo_vdp1_near_clip(), JO_MULT_BY_65536(20));
        JT_EQ(__jo_screen.width, 320);
        JT_EQ(__jo_screen.height, 240);
    }
    count = 0;
    for (i = 0; i < JT_COUNT(g_views) && count < max; ++i)
    {
        if (((g_views[i].flags & G_F_QUICK) != 0) != quick)
            continue;
        if (quick)
            ++g_quick_left;
        else
            ++g_full_left;
        steps[count++] = (jt_step){JT_STEP_SCENE, g_views[i].name, g_setup, g_draw, JO_NULL, g_cleanup, G_FRAMES};
    }
    return (count);
}
