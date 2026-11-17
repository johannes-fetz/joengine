/*
** Transparency test: the transparency layer of the engine (jo/transparency.h), like Burning Rangers
**
** The transparent objects are drawn between jo_3d_transparency_begin() and jo_3d_transparency_end(), with the opaque
** objects that can be in front of them as masks (jo_3d_transparency_mask()). The layer is mixed once with everything
** behind it, 3D of the VDP1 included (no stripes like the VDP1 half transparency, no checkerboard like the mesh).
**
** Modes (A):
**   1. Water of one color, ratio (half and half)
**   2. Water with a texture and a gouraud gradient (lighter on the edges, darker in the middle), ratio
**   3. Additive: flames (texture and gouraud fading to black at the top) and halos of lamps on the columns (the water: mesh)
**   4. VDP1 half transparency (for comparison)
**   5. Mesh (for comparison)
**
** B: rotation on/off   left/right: turn   up/down: distance   C: masks on/off (to see why they are needed)
** Y: width of the layer (160 or 192 pixels: steps of 2 or 1.7 pixels on its edges)
*/
#include <jo/jo.h>
#include "textures.h"

#define MODE_LAYER_FLAT     (0)     /* water: one color */
#define MODE_LAYER_TEXTURED (1)     /* water: texture + gouraud */
#define MODE_LAYER_ADDITIVE (2)     /* flames and halos, additive (the water: mesh) */
#define MODE_HALF_TRANSPARENCY (3)
#define MODE_MESH           (4)
#define MODE_COUNT          (5)

static const char *mode_names[MODE_COUNT] = { "LAYER: WATER, ONE COLOR      ", "LAYER: WATER, TEXTURE+GOURAUD",
                                              "LAYER: ADDITIVE FLAMES, HALOS", "VDP1 HALF TRANSPARENCY       ",
                                              "MESH (SCREEN DOORS)          " };

static  __jo_force_inline bool uses_layer(const int m)
{
    return (m <= MODE_LAYER_ADDITIVE);
}

/*
** Scene: a floor of flagstones, 2 pools (tiled bottom and walls), 4 sandstone columns with a lamp on top, water, flames
*/
#define TILE                (64)
#define GRID                (6)         /* tiles from -GRID to GRID - 1 */
#define POOL_DEPTH          (56)
#define WATER_Y             (0)         /* the water surface: at the level of the floor (nothing of the floor in front of it) */
#define COLUMN_HALF         (16)
#define COLUMN_HEIGHT       (176)
/* The halos: above the top of the columns, drawn in front of their own column (its mask) */
#define HALO_HEIGHT         (COLUMN_HEIGHT + 14)
#define HALO_SCALE          (2)
#define HALO_DEPTH_BIAS     (-48)

typedef struct
{
    int     tx0;
    int     tz0;
    int     tx1;
    int     tz1;
}           pool_rect;

/* In tiles, [tx0, tx1[ x [tz0, tz1[ */
static const pool_rect pools[2] = { { -4, -2, -1, 2 }, { 1, -2, 4, 2 } };

#define COLUMN_COUNT        (4)
static const int columns[COLUMN_COUNT][2] = { { -64, -160 }, { 64, -160 }, { -64, 160 }, { 64, 160 } };

/* Flames (on the floor, between the columns and behind the pools) */
#define FLAME_COUNT         (4)
#define FLAME_HALF_WIDTH    (28)
#define FLAME_HEIGHT        (110)
/* The images of a flame (4 sprites added one after the other: the sprite animator of the engine plays them), the game
   frames between 2 images (the animator waits FLAME_FRAME_RATE frames, then shows the next one: 10 images per second at 30
   frames per second) */
#define FLAME_FRAMES        (4)
#define FLAME_FRAME_RATE    (2)
static const int flames[FLAME_COUNT][2] = { { 0, -80 }, { 0, 80 }, { -224, 160 }, { 224, 160 } };

/* The quads of each mesh (room for them: the floor and the pools, then the columns (4 sides each), then the water) */
#define MAX_QUADS           (400)
#define COLUMN_SIDES        (4)
#define MAX_WATER_QUADS     (64)

/** @brief Vertices of a mesh being built (4 for each quad) */
typedef struct
{
    jo_vertice              *vertices;
    int                     count;                  /* quads */
    int                     capacity;               /* quads */
}                           quad_list;

/* Gouraud of the textured water: lighter along the edges of the pools (16 is neutral) */
#define WATER_GOURAUD       JO_COLOR_SATURN_RGB(9, 11, 13)
#define WATER_EDGE_GOURAUD  JO_COLOR_SATURN_RGB(18, 20, 18)

#define QUAD_LIST(ARRAY)    { ARRAY, 0, sizeof(ARRAY) / sizeof(jo_vertice) / JO_3D_QUAD_VERTEX_COUNT }

static jo_vertice           floor_vertices[MAX_QUADS * JO_3D_QUAD_VERTEX_COUNT];
static quad_list            floor_quads = QUAD_LIST(floor_vertices);
static jo_vertice           pool_vertices[MAX_QUADS * JO_3D_QUAD_VERTEX_COUNT];
static quad_list            pool_quads = QUAD_LIST(pool_vertices);
static jo_vertice           column_vertices[COLUMN_COUNT * COLUMN_SIDES * JO_3D_QUAD_VERTEX_COUNT];
static quad_list            column_quads = QUAD_LIST(column_vertices);
static jo_vertice           water_vertices[MAX_WATER_QUADS * JO_3D_QUAD_VERTEX_COUNT];
static quad_list            water_quads = QUAD_LIST(water_vertices);

static jo_3d_mesh           *floor_mesh;
static jo_3d_mesh           *pool_mesh;
static jo_3d_mesh           *column_mesh;
static jo_3d_mesh           *water_mesh;            /* layer: one color */
static jo_3d_mesh           *water_textured_mesh;   /* layer: texture + gouraud */
static jo_3d_mesh           *water_half_mesh;       /* VDP1 half transparency */
static jo_3d_mesh           *water_mesh_mesh;       /* screen doors */
static int                  flame_first;            /* layer, additive: standing sprites (they face the camera): the first image */
static int                  flame_anim;             /* their animation (jo_create_sprite_anim()) */
static unsigned short       flame_gouraud;          /* their gouraud table: neutral at the bottom, darker at the top */
static int                  halo_tex;

static jo_camera            cam;
static int                  angle = 30;
static int                  distance = 420;
static bool                 rotate = true;
#ifndef DEMO_START_MODE
# define DEMO_START_MODE    MODE_LAYER_FLAT
#endif
static int                  mode = DEMO_START_MODE;
static bool                 masks = true;

/** @brief Add a quad: center, "right" and "down" half vectors as seen from its front (clockwise, Y down) */
/** @brief Add a quad to a mesh being built: its center, its half edges right and down (see jo_3d_set_quad_vertices()) */
static void                 add_quad(quad_list * const list, int cx, int cy, int cz, int rx, int ry, int rz, int dx, int dy, int dz)
{
    const jo_vertice        center = { cx, cy, cz };
    const jo_vertice        right = { rx, ry, rz };
    const jo_vertice        down = { dx, dy, dz };

    if (list->count >= list->capacity)
    {
#ifdef JO_DEBUG
        jo_core_error("Too many quads");
#endif
        return;
    }
    jo_3d_set_quad_vertices(list->vertices + list->count * JO_3D_QUAD_VERTEX_COUNT, &center, &right, &down);
    ++list->count;
}

static bool                 in_pool(int tx, int tz)
{
    int                     i;

    for (i = 0; i < 2; ++i)
        if (tx >= pools[i].tx0 && tx < pools[i].tx1 && tz >= pools[i].tz0 && tz < pools[i].tz1)
            return (true);
    return (false);
}

static void                 build_scene(void)
{
    int                     tx;
    int                     tz;
    int                     i;
    int                     h = TILE / 2;
    int                     wall_cy = WATER_Y / 2 + POOL_DEPTH / 2;
    int                     wall_h = POOL_DEPTH / 2;

    // Floor: facing up (the top of the texture toward +Z)
    for (tz = -GRID; tz < GRID; ++tz)
        for (tx = -GRID; tx < GRID; ++tx)
            if (!in_pool(tx, tz))
                add_quad(&floor_quads, tx * TILE + h, 0, tz * TILE + h, h, 0, 0, 0, 0, -h);
    for (i = 0; i < 2; ++i)
    {
        const pool_rect     *p = &pools[i];

        for (tz = p->tz0; tz < p->tz1; ++tz)
            for (tx = p->tx0; tx < p->tx1; ++tx)
            {
                // Bottom
                add_quad(&pool_quads, tx * TILE + h, POOL_DEPTH, tz * TILE + h, h, 0, 0, 0, 0, -h);
                // Water (one quad per tile)
                add_quad(&water_quads, tx * TILE + h, WATER_Y, tz * TILE + h, h, 0, 0, 0, 0, -h);
            }
        // Walls, facing the inside of the pool (from the floor to the bottom)
        for (tx = p->tx0; tx < p->tx1; ++tx)
        {
            // Wall at z = tz0 (seen from +Z: right = -X)
            add_quad(&pool_quads, tx * TILE + h, wall_cy - WATER_Y / 2, p->tz0 * TILE, -h, 0, 0, 0, wall_h + WATER_Y / 2, 0);
            // Wall at z = tz1 (seen from -Z: right = +X)
            add_quad(&pool_quads, tx * TILE + h, wall_cy - WATER_Y / 2, p->tz1 * TILE, h, 0, 0, 0, wall_h + WATER_Y / 2, 0);
        }
        for (tz = p->tz0; tz < p->tz1; ++tz)
        {
            // Wall at x = tx0 (seen from +X: right = +Z)
            add_quad(&pool_quads, p->tx0 * TILE, wall_cy - WATER_Y / 2, tz * TILE + h, 0, 0, h, 0, wall_h + WATER_Y / 2, 0);
            // Wall at x = tx1 (seen from -X: right = -Z)
            add_quad(&pool_quads, p->tx1 * TILE, wall_cy - WATER_Y / 2, tz * TILE + h, 0, 0, -h, 0, wall_h + WATER_Y / 2, 0);
        }
    }
    // Columns: 4 sides facing out (no top: always seen from below their top)
    for (i = 0; i < COLUMN_COUNT; ++i)
    {
        int                 x = columns[i][0];
        int                 z = columns[i][1];
        int                 cy = -COLUMN_HEIGHT / 2;
        int                 hh = COLUMN_HEIGHT / 2;

        add_quad(&column_quads, x, cy, z - COLUMN_HALF, COLUMN_HALF, 0, 0, 0, hh, 0);   // front (-Z)
        add_quad(&column_quads, x, cy, z + COLUMN_HALF, -COLUMN_HALF, 0, 0, 0, hh, 0);  // back (+Z)
        add_quad(&column_quads, x - COLUMN_HALF, cy, z, 0, 0, -COLUMN_HALF, 0, hh, 0);  // left (-X)
        add_quad(&column_quads, x + COLUMN_HALF, cy, z, 0, 0, COLUMN_HALF, 0, hh, 0);   // right (+X)
    }
}

/** @brief A vertex of the water on the edge of its pool */
static bool                 on_pool_edge(const jo_vertice * const v)
{
    int                     i;

    for (i = 0; i < 2; ++i)
    {
        const pool_rect     *p = &pools[i];

        if (v->x < p->tx0 * TILE || v->x > p->tx1 * TILE || v->z < p->tz0 * TILE || v->z > p->tz1 * TILE)
            continue;
        if (v->x == p->tx0 * TILE || v->x == p->tx1 * TILE || v->z == p->tz0 * TILE || v->z == p->tz1 * TILE)
            return (true);
    }
    return (false);
}

static void                 create_meshes(void)
{
    static jo_color         water_tables[MAX_WATER_QUADS * JO_3D_QUAD_VERTEX_COUNT];
    static const jo_color   flame_table[4] = { JO_COLOR_SATURN_RGB(6, 6, 6), JO_COLOR_SATURN_RGB(6, 6, 6),
                                               JO_COLOR_SATURN_RGB(16, 16, 16), JO_COLOR_SATURN_RGB(16, 16, 16) };
    static jo_color         *flame_images[FLAME_FRAMES] = { tex_flame0, tex_flame1, tex_flame2, tex_flame3 };
    int                     i;
    int                     k;

    floor_mesh = jo_3d_create_mesh_from_vertices(floor_quads.count, floor_vertices);
    jo_3d_set_mesh_texture(floor_mesh, jo_sprite_add_rgb(tex_floor, TEX_FLOOR_W, TEX_FLOOR_H));
    pool_mesh = jo_3d_create_mesh_from_vertices(pool_quads.count, pool_vertices);
    jo_3d_set_mesh_texture(pool_mesh, jo_sprite_add_rgb(tex_pool, TEX_POOL_W, TEX_POOL_H));
    column_mesh = jo_3d_create_mesh_from_vertices(column_quads.count, column_vertices);
    jo_3d_set_mesh_texture(column_mesh, jo_sprite_add_rgb(tex_bricks, TEX_BRICKS_W, TEX_BRICKS_H));

    // Water: one color (drawn opaque in the layer: the VDP2 mixes it)
    water_mesh = jo_3d_create_mesh_from_vertices(water_quads.count, water_vertices);
    jo_3d_set_mesh_color(water_mesh, JO_COLOR_RGB(40, 130, 255));
    // Water: texture + gouraud, lighter and greener on the edges (shallow), darker in the middle (deep)
    water_textured_mesh = jo_3d_create_mesh_from_vertices(water_quads.count, water_vertices);
    jo_3d_set_mesh_texture(water_textured_mesh, jo_sprite_add_rgb(tex_water, TEX_WATER_W, TEX_WATER_H));
    for (i = 0; i < water_quads.count; ++i)
        for (k = 0; k < JO_3D_QUAD_VERTEX_COUNT; ++k)
        {
            const int       v = i * JO_3D_QUAD_VERTEX_COUNT + k;

            water_tables[v] = on_pool_edge(&water_vertices[v]) ? WATER_EDGE_GOURAUD : WATER_GOURAUD;
        }
    jo_3d_set_mesh_gouraud(water_textured_mesh, water_tables);
    // Water: for comparison
    water_half_mesh = jo_3d_create_mesh_from_vertices(water_quads.count, water_vertices);
    jo_3d_set_mesh_color(water_half_mesh, JO_COLOR_RGB(40, 130, 255));
    jo_3d_set_mesh_transparency(water_half_mesh, true);
    water_mesh_mesh = jo_3d_create_mesh_from_vertices(water_quads.count, water_vertices);
    jo_3d_set_mesh_color(water_mesh_mesh, JO_COLOR_RGB(40, 130, 255));
    jo_3d_set_mesh_screen_doors(water_mesh_mesh, true);

    // Flames: FLAME_FRAMES images (consecutive sprites, played in a loop by the sprite animator); the gouraud of the sprites
    // (corners: top left, top right, bottom right, bottom left) darker at the top (additive: the flame fades out)
    flame_first = jo_sprite_add_rgb(flame_images[0], TEX_FLAME0_W, TEX_FLAME0_H);
    for (i = 1; i < FLAME_FRAMES; ++i)
        jo_sprite_add_rgb(flame_images[i], TEX_FLAME0_W, TEX_FLAME0_H);
    flame_anim = jo_create_sprite_anim(flame_first, FLAME_FRAMES, FLAME_FRAME_RATE);
    jo_start_sprite_anim_loop(flame_anim);
    flame_gouraud = jo_3d_add_gouraud_tables(flame_table, 1);

    halo_tex = jo_sprite_add_rgb(tex_halo, TEX_HALO_W, TEX_HALO_H);
}

/** @brief The flames: animated standing sprites (they face the camera), with their gouraud table. One animation for the 4
 *  flames, each one an image later than the previous one (they don't flicker together) */
static void                 draw_flames(void)
{
    const int               frame = jo_get_sprite_anim_frame(flame_anim);
    jo_pos3D_fixed          foot;
    int                     i;

    jo_sprite_set_gouraud_table(flame_gouraud);
    jo_sprite_enable_gouraud_shading();
    for (i = 0; i < FLAME_COUNT; ++i)
    {
        foot.x = jo_int2fixed(flames[i][0]);
        foot.y = 0;
        foot.z = jo_int2fixed(flames[i][1]);
        jo_3d_draw_standing_sprite(flame_first + (frame + i) % FLAME_FRAMES, &foot, jo_int2fixed(FLAME_HALF_WIDTH * 2), jo_int2fixed(FLAME_HEIGHT), 0, false);
    }
    jo_sprite_disable_gouraud_shading();
    jo_sprite_set_gouraud_table(0);
}

static void                 draw_halos(void)
{
    int                     bias = jo_3d_get_depth_bias();
    int                     i;

    // In front of the mask of their own column
    jo_3d_set_depth_bias(bias + HALO_DEPTH_BIAS);
    for (i = 0; i < COLUMN_COUNT; ++i)
        jo_3d_draw_scaled_billboard(halo_tex, columns[i][0], -HALO_HEIGHT, columns[i][1], HALO_SCALE);
    jo_3d_set_depth_bias(bias);
}

/*
** Frame
*/

static void                 draw(void)
{
    int                     w;
    int                     h;

    jo_3d_camera_set_viewpoint(&cam, jo_sin_mult(distance, angle), -distance * 2 / 5, -jo_cos_mult(distance, angle));
    jo_3d_camera_set_target(&cam, 0, 20, 0);
    jo_3d_camera_look_at(&cam);

    // The opaque scene
    jo_3d_mesh_draw(floor_mesh);
    jo_3d_mesh_draw(pool_mesh);
    jo_3d_mesh_draw(column_mesh);

    if (uses_layer(mode))
    {
        // The transparent objects, and the opaque objects that can be in front of them as masks
        jo_3d_transparency_begin();
        if (mode == MODE_LAYER_FLAT)
            jo_3d_mesh_draw(water_mesh);
        else if (mode == MODE_LAYER_TEXTURED)
            jo_3d_mesh_draw(water_textured_mesh);
        else
        {
            draw_flames();
            draw_halos();
        }
        if (masks)
            jo_3d_transparency_mask(column_mesh);
        jo_3d_transparency_end();
    }
    if (mode == MODE_LAYER_ADDITIVE || mode == MODE_MESH)
        jo_3d_mesh_draw(water_mesh_mesh);
    else if (mode == MODE_HALF_TRANSPARENCY)
        jo_3d_mesh_draw(water_half_mesh);

    jo_3d_transparency_get_size(&w, &h);
    jo_printf(1, 1, "%d. %s", mode + 1, mode_names[mode]);
    jo_printf(1, 2, "FPS %2d  POLYS %3d  MASKS %s ", jo_core_get_fps(), jo_3d_get_displayed_polygon_count(), masks ? "ON " : "OFF");
    jo_printf(1, 3, "LAYER %dX%d  ", w, h);
}

static void                 input(void)
{
    int                     w;

    if (jo_is_pad1_key_down(JO_KEY_A))
    {
        mode = (mode + 1) % MODE_COUNT;
        // Additive for the flames and the halos, ratio for the water (the same mode for all the VDP2 screens)
        if (mode == MODE_LAYER_ADDITIVE)
            jo_3d_transparency_set_blend(JO_VDP2_BLEND_ADD, 0);
        else
            jo_3d_transparency_set_blend(JO_VDP2_BLEND_RATIO, JO_3D_TRANSPARENCY_DEFAULT_RATIO);
    }
    if (jo_is_pad1_key_down(JO_KEY_B))
        rotate = !rotate;
    if (jo_is_pad1_key_down(JO_KEY_C))
        masks = !masks;
    if (jo_is_pad1_key_down(JO_KEY_Y))
    {
        jo_3d_transparency_get_size(&w, JO_NULL);
        jo_3d_transparency_init(w == JO_3D_TRANSPARENCY_MAX_WIDTH ? JO_3D_TRANSPARENCY_DEFAULT_WIDTH : JO_3D_TRANSPARENCY_MAX_WIDTH,
                                JO_NBG1_SCREEN);
    }
    if (jo_is_pad1_key_pressed(JO_KEY_LEFT))
        angle -= 2;
    if (jo_is_pad1_key_pressed(JO_KEY_RIGHT))
        angle += 2;
    if (jo_is_pad1_key_pressed(JO_KEY_UP) && distance > 200)
        distance -= 4;
    if (jo_is_pad1_key_pressed(JO_KEY_DOWN) && distance < 700)
        distance += 4;
    if (rotate)
        ++angle;
    if (angle >= 360)
        angle -= 360;
    if (angle < 0)
        angle += 360;
}

void                        jo_main(void)
{
    jo_core_init(JO_COLOR_RGB(20, 18, 30));
    jo_3d_camera_init(&cam);
    build_scene();
    create_meshes();
    jo_3d_transparency_init(0, JO_NBG1_SCREEN);
    if (mode == MODE_LAYER_ADDITIVE)
        jo_3d_transparency_set_blend(JO_VDP2_BLEND_ADD, 0);
    jo_core_add_callback(input);
    jo_core_add_callback(draw);
    jo_core_set_restart_game_callback(jo_goto_boot_menu);
    jo_core_run();
}
