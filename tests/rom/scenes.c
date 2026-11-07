/*
** Jo Sega Saturn Engine - regression test ROM: the scenes (pictures compared with the golden images) and the measures
** (CPU and VDP1 time compared with the budgets). Everything is static: the same picture at each frame
*/

#include "jt.h"
#include "tests.h"

#define T_SPRITE_SIZE           (32)

#define T_SHADES_FIRST          (32)
#define T_SHADES                (8)

static unsigned short           t_pixels[T_SPRITE_SIZE * T_SPRITE_SIZE];
static unsigned char            t_pixels8[T_SPRITE_SIZE * T_SPRITE_SIZE];
static int                      t_sprite_rgb;
static int                      t_sprite_8bits;
static int                      t_sprite;
static jo_palette               t_palette;
static jo_vertice               t_palette_vertices[] = JO_3D_CUBE_VERTICES(32);
static jo_3d_quad               t_palette_cube[6];
static jo_camera                t_camera;
static jo_vertice               t_cube_vertices[] = JO_3D_CUBE_VERTICES(32);
static jo_3d_quad               t_cube[6];
static jo_vertice               t_textured_vertices[] = JO_3D_CUBE_VERTICES(32);
static jo_3d_quad               t_textured[6];
static unsigned int             t_seed;

static int                      t_rand(const int max)
{
    t_seed = t_seed * 1103515245u + 12345u;
    return ((int)((t_seed >> 16) % (unsigned int)max));
}

void                            t_scenes_init(void)
{
    static const jo_color       faces[6] = {JO_COLOR_Red, JO_COLOR_Green, JO_COLOR_Blue, JO_COLOR_Yellow, JO_COLOR_Cyan, JO_COLOR_Purple};
    jo_img                      img;
    jo_img_8bits                img8;
    int                         x;
    int                         y;
    int                         i;

    for (y = 0; y < T_SPRITE_SIZE; ++y)
        for (x = 0; x < T_SPRITE_SIZE; ++x)
            t_pixels[y * T_SPRITE_SIZE + x] = ((x >> 3) ^ (y >> 3)) & 1 ? JO_COLOR_RGB(x * 8, 255 - y * 8, 64) : JO_COLOR_RGB(240, 240, 240);
    // Transparent corner
    for (y = 0; y < 6; ++y)
        for (x = 0; x < 6 - y; ++x)
            t_pixels[y * T_SPRITE_SIZE + x] = JO_COLOR_Transparent;
    img.width = T_SPRITE_SIZE;
    img.height = T_SPRITE_SIZE;
    img.data = t_pixels;
    t_sprite_rgb = jo_sprite_add(&img);
    // The same sprite with a palette (the only sprites of the 640 and 704 pixels modes): colors 1 to 15, 0 transparent
    jo_create_palette(&t_palette);
    // (data[0]: the color 1)
    for (i = 1; i < 15; ++i)
        t_palette.data[i - 1] = JO_COLOR_RGB((i * 53) & 255, (i * 97) & 255, (i * 151) & 255);
    t_palette.data[15 - 1] = JO_COLOR_RGB(240, 240, 240);
    jo_palette_set_light_shades(&t_palette, T_SHADES_FIRST, faces, 6, T_SHADES);
    jo_core_set_screen_palette(t_palette.id);
    jo_3d_set_palette_light(T_SHADES, 6);
    for (y = 0; y < T_SPRITE_SIZE; ++y)
        for (x = 0; x < T_SPRITE_SIZE; ++x)
            t_pixels8[y * T_SPRITE_SIZE + x] = ((x >> 3) ^ (y >> 3)) & 1 ? (unsigned char)(1 + ((x + y) >> 2) % 14) : 15;
    for (y = 0; y < 6; ++y)
        for (x = 0; x < 6 - y; ++x)
            t_pixels8[y * T_SPRITE_SIZE + x] = 0;
    img8.width = T_SPRITE_SIZE;
    img8.height = T_SPRITE_SIZE;
    img8.data = t_pixels8;
    t_sprite_8bits = jo_sprite_add_8bits_image(&img8);
    t_sprite = t_sprite_rgb;
    jo_3d_camera_init(&t_camera);
    jo_3d_create_cube(t_cube, t_cube_vertices);
    jo_3d_create_cube(t_textured, t_textured_vertices);
    jo_3d_create_cube(t_palette_cube, t_palette_vertices);
    for (i = 0; i < 6; ++i)
    {
        jo_3d_set_color(&t_cube[i], faces[i]);
        jo_3d_set_light(&t_cube[i], true);
        jo_3d_set_texture(&t_textured[i], t_sprite_rgb);
        jo_3d_set_color(&t_palette_cube[i], JO_PALETTE_LIGHT_COLOR(t_palette.id, T_SHADES_FIRST, i));
        jo_3d_set_light(&t_palette_cube[i], true);
    }
}

/** @brief The sprites of the scenes: with a palette in the 640 and 704 pixels modes (no RGB sprite in these modes) */
static void                     t_select_sprite(void)
{
    t_sprite = __jo_screen.hires ? t_sprite_8bits : t_sprite_rgb;
    jo_sprite_set_palette(t_palette.id);
}

static void                     t_draw_cube(jo_3d_quad * const cube, const int x, const int y, const int z, const short rx, const short ry)
{
    int                         i;

    jo_3d_push_matrix();
    {
        jo_3d_translate_matrix(x, y, z);
        jo_3d_rotate_matrix(rx, ry, 0);
        for (i = 0; i < 6; ++i)
            jo_3d_draw(&cube[i]);
    }
    jo_3d_pop_matrix();
}

static void                     t_nbg1_pattern(void)
{
    int                         x;
    int                         y;
    int                         w;
    int                         h;

    w = __jo_screen.background_width;
    h = __jo_screen.background_height;
    for (y = 0; y < h; ++y)
        jo_vdp2_draw_bitmap_nbg1_line(0, y, w - 1, y, JO_COLOR_RGB(y * 255 / h, 40, 255 - y * 255 / h));
    for (x = 0; x < w; x += 16)
        jo_vdp2_draw_bitmap_nbg1_line(x, 0, x, h - 1, JO_COLOR_White);
    jo_vdp2_draw_bitmap_nbg1_line(0, 0, w - 1, h - 1, JO_COLOR_Yellow);
    jo_vdp2_draw_bitmap_nbg1_line(w - 1, 0, 0, h - 1, JO_COLOR_Yellow);
    jo_vdp2_draw_bitmap_nbg1_square(w / 4, h / 4, w / 2, h / 2, JO_COLOR_Green);
}

/*
** Scenes
*/

static void                     s_sprites_draw(void)
{
    int                         i;

    t_select_sprite();
    for (i = 0; i < 8; ++i)
        jo_sprite_draw3D(t_sprite, -140 + i * 36, -80, 500);
    for (i = 0; i < 6; ++i)
        jo_sprite_draw3D_and_rotate(t_sprite, -120 + i * 48, -20, 500, i * 15);
    jo_sprite_change_sprite_scale(2.0f);
    jo_sprite_draw3D(t_sprite, -100, 50, 500);
    jo_sprite_change_sprite_scale(0.5f);
    jo_sprite_draw3D(t_sprite, -40, 50, 500);
    jo_sprite_change_sprite_scale(1.0f);
    jo_sprite_enable_horizontal_flip();
    jo_sprite_draw3D(t_sprite, 0, 50, 500);
    jo_sprite_disable_horizontal_flip();
    jo_sprite_enable_vertical_flip();
    jo_sprite_draw3D(t_sprite, 40, 50, 500);
    jo_sprite_disable_vertical_flip();
    jo_sprite_enable_half_transparency();
    jo_sprite_draw3D(t_sprite, 80, 50, 400);
    jo_sprite_draw3D(t_sprite, 95, 60, 450);
    jo_sprite_disable_half_transparency();
    jo_sprite_enable_shadow_filter();
    jo_sprite_draw3D(t_sprite, 130, 50, 500);
    jo_sprite_disable_shadow_filter();
}

/** @brief The colored shapes (jo_sprite_draw_rectangle(), jo_sprite_draw_color_4p_fixed()): flat, sorted with the sprites,
 *  with the effects of the sprites (half transparency, gouraud, screen doors, shadow; RGB colors, the palette of the screen
 *  in the 640 and 704 pixels modes)
 */
static void                     s_shapes_draw(void)
{
    static const jo_pos2D_fixed triangle[4] = {{0, -JO_MULT_BY_65536(90)}, {JO_MULT_BY_65536(40), -JO_MULT_BY_65536(30)},
                                               {-JO_MULT_BY_65536(40), -JO_MULT_BY_65536(30)}, {-JO_MULT_BY_65536(40), -JO_MULT_BY_65536(30)}};
    static const jo_color       rgb[6] = {JO_COLOR_Red, JO_COLOR_Green, JO_COLOR_Blue, JO_COLOR_Yellow, JO_COLOR_Cyan, JO_COLOR_Purple};
    jo_color                    colors[6];
    int                         w;
    int                         h;
    int                         i;

    w = jo_core_get_screen_width();
    h = jo_core_get_screen_height();
    for (i = 0; i < 6; ++i)
        colors[i] = __jo_screen.hires ? JO_PALETTE_LIGHT_COLOR(t_palette.id, T_SHADES_FIRST, i) : rgb[i];
    t_select_sprite();
    // A frame around the screen (1 pixel lines) and bars of each color (1, 2... 6 pixels high)
    jo_sprite_draw_rectangle(0, 0, w, 1, colors[5], 600);
    jo_sprite_draw_rectangle(0, h - 1, w, 1, colors[5], 600);
    jo_sprite_draw_rectangle(0, 0, 1, h, colors[5], 600);
    jo_sprite_draw_rectangle(w - 1, 0, 1, h, colors[5], 600);
    for (i = 0; i < 6; ++i)
        jo_sprite_draw_rectangle(8 + i * 20, 24, 16, i + 1, colors[i], 500);
    // The sort: a rectangle behind a sprite (farther), another in front of it (nearer), the same depth in the order of the calls
    jo_sprite_draw_rectangle(w / 2 - 40, h / 2 - 8, 80, 16, colors[0], 600);
    jo_sprite_draw3D(t_sprite, 0, 0, 500);
    jo_sprite_draw_rectangle(w / 2 - 4, h / 2 - 30, 8, 60, colors[1], 400);
    jo_sprite_draw_rectangle(w / 2 + 50, h / 2 - 20, 20, 20, colors[2], 450);
    jo_sprite_draw_rectangle(w / 2 + 55, h / 2 - 15, 20, 20, colors[3], 450);
    // A triangle from the center of the screen
    jo_sprite_draw_color_4p_fixed(triangle, colors[4], JO_MULT_BY_65536(500), true);
    // The effects over a sprite: half transparency, screen doors, shadow, gouraud shading
    jo_sprite_draw3D(t_sprite, -100, 60, 500);
    jo_sprite_enable_half_transparency();
    jo_sprite_draw_rectangle(w / 2 - 116, h / 2 + 50, 32, 20, colors[0], 400);
    jo_sprite_disable_half_transparency();
    jo_sprite_draw3D(t_sprite, -40, 60, 500);
    jo_sprite_enable_screen_doors_filter();
    jo_sprite_draw_rectangle(w / 2 - 56, h / 2 + 50, 32, 20, colors[2], 400);
    jo_sprite_disable_screen_doors_filter();
    jo_sprite_draw3D(t_sprite, 20, 60, 500);
    jo_sprite_enable_shadow_filter();
    jo_sprite_draw_rectangle(w / 2 + 4, h / 2 + 50, 32, 20, JO_COLOR_Black, 400);
    jo_sprite_disable_shadow_filter();
    jo_set_gouraud_shading_colors(JO_COLOR_RGB(255, 0, 0), JO_COLOR_RGB(0, 255, 0), JO_COLOR_RGB(0, 0, 255), JO_COLOR_RGB(128, 128, 128));
    jo_sprite_enable_gouraud_shading();
    jo_sprite_draw_rectangle(w / 2 + 64, h / 2 + 44, 48, 32, JO_COLOR_RGB(128, 128, 128), 500);
    jo_sprite_disable_gouraud_shading();
}

static void                     s_cube_draw(void)
{
    jo_3d_camera_look_at(&t_camera);
    jo_3d_light(1, 1, 1);
    t_draw_cube(__jo_screen.hires ? t_palette_cube : t_cube, -60, 0, 0, 30, 45);
    t_draw_cube(__jo_screen.hires ? t_palette_cube : t_textured, 60, 0, 0, -20, 30);
}

static void                     s_palette_light_draw(void)
{
    int                         i;

    jo_3d_camera_look_at(&t_camera);
    jo_3d_light(1, 1, 1);
    for (i = 0; i < 4; ++i)
        t_draw_cube(t_palette_cube, i * 64 - 96, (i & 1) * 40 - 20, 40, (short)(i * 25 + 10), (short)(i * 40 + 20));
}

static void                     s_nbg1_setup(void)
{
    t_nbg1_pattern();
}

static void                     s_mosaic_setup(void)
{
    t_nbg1_pattern();
    jo_vdp2_enable_mosaic(JO_NBG1_SCREEN, 4, 4);
}

static void                     s_mosaic_cleanup(void)
{
    jo_vdp2_disable_mosaic();
    jt_restore_nbg1();
}

static void                     s_window_setup(void)
{
    t_nbg1_pattern();
    jo_vdp2_set_window_circle(JO_VDP2_WINDOW_0, jo_core_get_screen_width() / 2, jo_core_get_screen_height() / 2, 60);
    jo_vdp2_enable_window(JO_NBG1_SCREEN | JO_SPRITE_SCREEN, JO_VDP2_WINDOW_0, JO_VDP2_WINDOW_SHOW_INSIDE);
}

static void                     s_window_cleanup(void)
{
    jo_vdp2_disable_window(JO_NBG1_SCREEN | JO_SPRITE_SCREEN);
    jt_restore_nbg1();
}

static void                     s_filter_setup(void)
{
    t_nbg1_pattern();
    jo_set_screen_color_filter_a(JO_NBG1_SCREEN | JO_SPRITE_SCREEN, 60, -40, -40);
}

static void                     s_filter_cleanup(void)
{
    jo_set_screen_color_filter_a(JO_NBG1_SCREEN | JO_SPRITE_SCREEN, 0, 0, 0);
    jt_restore_nbg1();
}

static void                     s_blend_setup(void)
{
    t_nbg1_pattern();
    jo_vdp2_enable_blend(JO_SPRITE_SCREEN, 16);
}

static void                     s_blend_cleanup(void)
{
    jo_vdp2_disable_blend(JO_SPRITE_SCREEN);
    jt_restore_nbg1();
}

static void                     s_text_draw(void)
{
    int                         i;

    for (i = 0; i < 8; ++i)
        jo_printf_with_color(2, 3 + i, i, "COLOR %d: THE QUICK BROWN FOX 0123456789", i);
    jo_set_printf_color_index(0);
}

static void                     s_text_cleanup(void)
{
    jo_clear_screen();
}

static void                     s_hires_setup(void)
{
    if (!__jo_screen.hires)
        jo_core_set_screen_resolution(640, 448);
    else
        jo_core_set_screen_resolution(704, 240);
    t_nbg1_pattern();
}

static void                     s_hires_cleanup(void)
{
    jo_core_set_screen_resolution(JO_TV_WIDTH, JO_TV_HEIGHT);
    jt_restore_nbg1();
}

static void                     s_hires_draw(void)
{
    s_sprites_draw();
    s_cube_draw();
}

/*
** Measures (the averages of the profiler over the last 32 frames)
*/

static void                     t_measure(const char * const name)
{
    static const struct { jo_profile_slot slot; const char *name; } slots[] =
    {
        {JO_PROFILE_FRAME, "frame"}, {JO_PROFILE_CALLBACKS, "callbacks"}, {JO_PROFILE_VDP1_DRAW, "vdp1_draw"},
        {JO_PROFILE_3D_VERTICES, "3d_vertices"}, {JO_PROFILE_3D_POLYGONS, "3d_polygons"}, {JO_PROFILE_VDP1_COMMANDS, "vdp1_commands"}
    };
    unsigned int                average;
    unsigned int                maximum;
    char                        text[64];
    int                         i;

    for (i = 0; i < JT_COUNT(slots); ++i)
    {
        if (!jo_profile_get(slots[i].slot, &average, &maximum))
            continue;
        jo_sprintf(text, "perf.%s.%s", name, slots[i].name);
        jt_metric(text, average);
    }
}

static void                     b_sprites_setup(void)
{
    t_seed = 1;
    t_select_sprite();
}

static void                     b_sprites_draw(void)
{
    int                         i;
    unsigned int                seed;

    seed = t_seed;
    for (i = 0; i < 200; ++i)
        jo_sprite_draw3D(t_sprite, t_rand(300) - 150, t_rand(200) - 100, 500);
    t_seed = seed;
}

static void                     b_sprites_measure(void)
{
    t_measure("sprites_200");
}

static void                     b_cubes_draw(void)
{
    int                         i;

    jo_3d_camera_look_at(&t_camera);
    for (i = 0; i < 40; ++i)
        t_draw_cube(__jo_screen.hires ? t_palette_cube : (i & 1) ? t_textured : t_cube, (i % 8) * 36 - 126, (i / 8) * 40 - 80, 150, (short)(i * 9), (short)(i * 14));
}

static void                     b_cubes_measure(void)
{
    t_measure("cubes_40");
}

static void                     b_nbg1_draw(void)
{
    t_nbg1_pattern();
}

static void                     b_nbg1_measure(void)
{
    t_measure("nbg1_pattern");
}

static void                     b_text_draw(void)
{
    int                         i;

    for (i = 0; i < 12; ++i)
        jo_printf(1, 14 + i, "LINE %d %d %x %s", i, i * 1234, i * 0xABCD, "TEXT");
}

static void                     b_text_measure(void)
{
    t_measure("printf_12");
    jo_clear_screen();
}

static void                     b_empty_measure(void)
{
    t_measure("empty");
}

const jt_step                   t_scene_steps[] =
{
    {JT_STEP_BENCH, "empty", JO_NULL, JO_NULL, b_empty_measure, JO_NULL, 64},
    {JT_STEP_BENCH, "sprites_200", b_sprites_setup, b_sprites_draw, b_sprites_measure, JO_NULL, 64},
    {JT_STEP_BENCH, "cubes_40", JO_NULL, b_cubes_draw, b_cubes_measure, JO_NULL, 64},
    {JT_STEP_BENCH, "nbg1_pattern", JO_NULL, b_nbg1_draw, b_nbg1_measure, jt_restore_nbg1, 64},
    {JT_STEP_BENCH, "printf_12", JO_NULL, b_text_draw, b_text_measure, JO_NULL, 64},
    {JT_STEP_REPORT, "perf", JO_NULL, JO_NULL, JO_NULL, JO_NULL, 0},
    {JT_STEP_SCENE, "sprites", JO_NULL, s_sprites_draw, JO_NULL, JO_NULL, 0},
    {JT_STEP_SCENE, "cubes", JO_NULL, s_cube_draw, JO_NULL, JO_NULL, 0},
    {JT_STEP_SCENE, "palette_light", JO_NULL, s_palette_light_draw, JO_NULL, JO_NULL, 0},
    {JT_STEP_SCENE, "nbg1_bitmap", s_nbg1_setup, s_sprites_draw, JO_NULL, jt_restore_nbg1, 0},
    {JT_STEP_SCENE, "nbg1_mosaic", s_mosaic_setup, JO_NULL, JO_NULL, s_mosaic_cleanup, 0},
    {JT_STEP_SCENE, "window_circle", s_window_setup, s_sprites_draw, JO_NULL, s_window_cleanup, 0},
    {JT_STEP_SCENE, "color_filter", s_filter_setup, s_sprites_draw, JO_NULL, s_filter_cleanup, 0},
    {JT_STEP_SCENE, "sprite_blend", s_blend_setup, s_sprites_draw, JO_NULL, s_blend_cleanup, 0},
    {JT_STEP_SCENE, "shapes", JO_NULL, s_shapes_draw, JO_NULL, JO_NULL, 0},
    {JT_STEP_SCENE, "printf_colors", JO_NULL, s_text_draw, JO_NULL, s_text_cleanup, 0},
    {JT_STEP_SCENE, "hires", s_hires_setup, s_hires_draw, JO_NULL, s_hires_cleanup, 0},
};

const int                       t_scene_step_count = JT_COUNT(t_scene_steps);
