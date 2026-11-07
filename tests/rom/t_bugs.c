/*
** Jo Sega Saturn Engine - regression test ROM: the bugs found by the review of the engine (each check failed before its
** fix)
*/

#include "jt.h"
#include "tests.h"

#define T_REG(ADDRESS)          (jo_vdp2_get_registers()[((ADDRESS) - 0x25F80000) / 2])

static void             t_sprintf_bugs(void)
{
    char                b[64];

    jt_group("bugs.sprintf");
    jo_sprintf(b, "%+d %d", 5, 7);
    JT_EQ(jo_strcmp(b, "+5 7"), 0);
    jo_sprintf(b, "% d|%+d", 5, -5);
    JT_EQ(jo_strcmp(b, " 5|-5"), 0);
    jo_sprintf(b, "%.3d|%5.2s|%.1s", 7, "abc", "xyz");
    JT_EQ(jo_strcmp(b, "007|   ab|x"), 0);
    // (an unsupported conversion: its argument skipped, the next ones right)
    jo_sprintf(b, "%.2f %s", 1.5, "ok");
    JT_EQ(jo_strcmp(b, "%f ok"), 0);
    // A size: size - 1 characters, then the null character
    b[8] = 'Z';
    JT_EQ(jo_snprintf(b, 8, "%s", "abcdefghij"), 7);
    JT_EQ(jo_strcmp(b, "abcdefg"), 0);
    JT_EQ(b[8], 'Z');
    b[0] = 'Q';
    JT_EQ(jo_snprintf(b, 0, "%d", 123), 0);
    JT_EQ(b[0], 'Q');
    // jo_printf() of a long text: not written after the buffer of 64 bytes
    JT_EQ(jo_snprintf(b, sizeof(b), "NULL pointer: access to 0-0xFFF before PC %x, PR %x (%d times)", 0x06004A3C, 0x06004A10, 1), 63);
}

static void             t_math_bugs(void)
{
    jo_matrix           m;
    jo_vector4_fixed    v;
    jo_matrixf          look;
    jo_vectorf          from;
    jo_vectorf          to;
    jo_vectorf          up;
    jo_list             list;
    int                 i;
    int                 r;
    int                 bad;

    jt_group("bugs.math");
    JT_EQ(jo_tools_atoi(" 7"), 7);
    JT_EQ(jo_tools_atoi("+3"), 3);
    JT_EQ(jo_tools_atoi("42\r"), 42);
    JT_EQ(jo_tools_atoi("abc"), 0);
    JT_EQ(jo_tools_atoi("-12x"), -12);
    JT_EQ(jo_fixed_pow(JO_FIXED_2, -JO_FIXED_1_DIV_2), JO_FIXED_1);
    JT_EQ(jo_fixed_pow(JO_FIXED_2, -JO_FIXED_1), JO_FIXED_1_DIV_2);
    JT_EQ(JO_HAS_FLAG(0, 1 | 2), 0);
    JT_EQ(JO_HAS_FLAG(3, 1 | 2), 1);
    JT_EQ(JO_BCD_INT(0x10 | 0x02), 12);
    // jo_tanf(): the fraction of the degrees (45.5 degrees: 1.0176, not tan(45) = 1)
    JT_NEAR((int)(jo_tanf(45.5f) * 10000.0f), 10176, 30);
    for (bad = 0, i = 0; i < 200; ++i)
    {
        r = jo_random_using_multiple(10, 5);
        if (r != 5 && r != 10)
            ++bad;
    }
    JT_EQ(bad, 0);
    // In place (v == result): a rotation of 90 degrees around Z
    jo_matrix_identity(&m);
    m.m00 = 0; m.m01 = JO_FIXED_1; m.m10 = -JO_FIXED_1; m.m11 = 0;
    v.x = JO_FIXED_1; v.y = JO_FIXED_2; v.z = 0; v.w = JO_FIXED_1;
    jo_matrix_mul_vector4(&m, &v, &v);
    JT_EQ(v.x, -JO_FIXED_2);
    JT_EQ(v.y, JO_FIXED_1);
    // The camera: the translation in m32, m33 = 1
    from.x = 0; from.y = 0; from.z = 1;
    to.x = 0; to.y = 0; to.z = 0;
    up.x = 0; up.y = 1; up.z = 0;
    jo_matrixf_look_at(&from, &to, &up, &look);
    JT_NEAR((int)(look.m32 * 1000), -1000, 5);
    JT_NEAR((int)(look.m33 * 1000), 1000, 5);
    jo_list_init(&list);
    jo_list_add_ptr(&list, (void *)1);
    JT_TRUE(jo_list_at(&list, 1) == JO_NULL);
    JT_TRUE(jo_list_at(&list, -1) == list.first);
    jo_list_clear(&list);
}

static void             t_ticks_bugs(void)
{
    unsigned int        start;
    int                 i;
    int                 frc;

    jt_group("bugs.ticks");
    // Called every 200 us: the remainders under a millisecond add up (before: 0 ms after 400 ms)
    start = jo_get_ticks();
    for (i = 0; i < 2000; ++i)
    {
        frc = jo_time_get_frc();
        while (((jo_time_get_frc() - frc) & 0xFFFF) < 170)
            ;
        jo_get_ticks();
    }
    JT_TRUE(jo_get_ticks() - start >= 300);
}

static void             t_sprite_bugs(void)
{
    static unsigned short pixels[8 * 8];
    jo_img              img;
    jo_palette          palette;
    jo_color            color;
    unsigned short      *marker;
    unsigned short      saved;
    int                 last;
    int                 id;
    int                 anim;
    int                 first_anim;
    int                 i;

    jt_group("bugs.sprites");
    // jo_sprite_free_from(-1): the id of a sprite not added, nothing freed
    last = jo_get_last_sprite_id();
    jo_sprite_free_from(-1);
    JT_EQ(jo_get_last_sprite_id(), last);
    // The color calculation modes: one at a time (3 half transparency, 4 gouraud, 7 both, no 5)
    jo_sprite_enable_half_transparency();
    jo_sprite_disable_dark_filter();
    JT_EQ(__jo_sprite_attributes.effect & 7, 3);
    jo_sprite_enable_gouraud_shading();
    JT_EQ(__jo_sprite_attributes.effect & 7, 7);
    jo_sprite_enable_shadow_filter();
    JT_EQ(__jo_sprite_attributes.effect & 7, 1);
    jo_sprite_enable_gouraud_shading();
    JT_EQ(__jo_sprite_attributes.effect & 7, 4);
    jo_sprite_enable_dark_filter();
    JT_EQ(__jo_sprite_attributes.effect & 7, 6);
    jo_sprite_disable_half_transparency();
    JT_EQ(__jo_sprite_attributes.effect & 7, 6);
    jo_sprite_disable_dark_filter();
    jo_sprite_disable_gouraud_shading();
    JT_EQ(__jo_sprite_attributes.effect & 7, 0);
    // The gouraud tables of the sprites are not in the memory of the textures (0x70000 before)
    marker = (unsigned short *)(JO_VDP1_VRAM + 0x70000);
    saved = *marker;
    *marker = 0x1234;
    jo_set_gouraud_shading_brightness(10);
    JT_EQ(*marker, 0x1234);
    *marker = saved;
    JT_TRUE(JO_VDP1_USER_AREA_END_ADDR <= JO_VDP1_VRAM + JO_VDP1_CLIP_GOURAUD_TABLE_OFFSET - 2 * 32 * 8);
    // A sprite added after a free: no 3D data of the freed sprite (mipmaps)
    for (i = 0; i < 64; ++i)
        pixels[i] = JO_COLOR_RGB(i * 4, 0, 0);
    img.width = 8;
    img.height = 8;
    img.data = pixels;
    id = jo_sprite_add(&img);
    jo_sprite_free_from(id);
    JT_EQ(jo_sprite_add(&img), id);
    jo_sprite_free_from(id);
    // The animations: the last one can be replaced, no more than JO_MAX_SPRITE_ANIM
    jt_expect_errors(true);
    first_anim = jo_create_sprite_anim(0, 4, 2);
    JT_TRUE(first_anim >= 0);
    JT_EQ(jo_replace_sprite_anim(first_anim, 0, 4, 2), first_anim);
    JT_EQ(jo_replace_sprite_anim(first_anim + 1, 0, 4, 2), -1);
    JT_EQ(jo_replace_sprite_anim(-1, 0, 4, 2), -1);
    for (anim = first_anim, i = 0; i < JO_MAX_SPRITE_ANIM + 2 && anim >= 0; ++i)
        anim = jo_create_sprite_anim(0, 4, 2);
    JT_EQ(anim, -1);
    jt_expect_errors(false);
    // The palette shades: the shade levels / 2 is the color itself
    jt_group("bugs.palette");
    jo_create_palette(&palette);
    color = JO_COLOR_SATURN_RGB(20, 20, 20);
    jo_palette_set_light_shades(&palette, 16, &color, 1, 4);
    JT_EQ(palette.data[16 + 2 - 1], color);
    JT_TRUE(JO_COLOR_SATURN_GET_R(palette.data[16 - 1]) < 20);
    JT_TRUE(JO_COLOR_SATURN_GET_R(palette.data[16 + 3 - 1]) > 20);
}

static void             t_vdp2_bugs(void)
{
    volatile unsigned short *back;
    unsigned short      saved;
    unsigned short      *table;
    unsigned int        offset;

    jt_group("bugs.vdp2");
    // BGON bit 5 is RBG1: JO_BACK_SCREEN doesn't turn it on
    saved = T_REG(0x25F80020);
    jo_set_displayed_screens((jo_scroll_screen)(JO_NBG0_SCREEN | JO_NBG1_SCREEN | JO_BACK_SCREEN));
    JT_EQ(T_REG(0x25F80020) & 0x20, 0);
    jo_set_displayed_screens((jo_scroll_screen)(saved & 0x1F));
    // The back screen color (the last word of the NBG1 bitmap) is kept by the clear of the bitmap
    back = (volatile unsigned short *)0x25E3FFFE;
    saved = *back;
    *back = JO_COLOR_Blue;
    jo_clear_background(JO_COLOR_Black);
    JT_EQ(*back, JO_COLOR_Blue);
    *back = saved;
    // jo_fade_screens() of 0 frames: the last level at once
    jo_fade_screens(JO_NBG1_SCREEN, 0, -255, 0);
    JT_EQ(T_REG(0x25F80114), (unsigned short)(-255 & 0x1FF));
    jo_set_screen_color_filter_a(JO_NBG1_SCREEN, 0, 0, 0);
    jo_disable_screen_color_filter(JO_NBG1_SCREEN);
    // The VDP2 memory: blocks freed in the order of their allocation go back to the bank (the top stayed up before)
    {
        void            *a;
        void            *b;
        void            *c;

        a = jo_vdp2_malloc(JO_VDP2_RAM_USER, 4096);
        b = jo_vdp2_malloc(JO_VDP2_RAM_USER, 4096);
        jo_vdp2_free(a);
        jo_vdp2_free(b);
        c = jo_vdp2_malloc(JO_VDP2_RAM_USER, 8192);
        JT_TRUE(c == a);
        jo_vdp2_free(c);
    }
    // The map of an image whose height in cells (3) doesn't divide 64: each row of the map starts with the first cell of
    // the image (map[64] == map[0]; map[0] + 2 before, the rows were scrambled)
    {
        static unsigned char    cells[8 * 24];
        jo_img_8bits            img8;
        jo_palette              pal;
        unsigned short          *map;
        unsigned int            page;
        int                     k;

        for (k = 0; k < 8 * 24; ++k)
            cells[k] = (unsigned char)(1 + k / 64);
        img8.width = 8;
        img8.height = 24;
        img8.data = cells;
        jo_create_palette(&pal);
        jo_vdp2_set_nbg2_8bits_image(&img8, pal.id, false, false);
        page = (T_REG(0x25F80048) & 0x3F) | (((T_REG(0x25F8003C) >> 8) & 7) << 6);
        map = (unsigned short *)(0x25E00000 + page * 0x2000);
        JT_EQ(map[64], map[0]);
        JT_EQ(map[128], map[0]);
        JT_EQ(map[1], map[0] + 2);
    }
    // A circle out of the right of the screen: its lines end at the right of the screen (wrapped before)
    JT_TRUE(jo_vdp2_set_window_circle(JO_VDP2_WINDOW_0, 300, 112, 250));
    offset = (((unsigned int)T_REG(0x25F800D8) & 7) << 16 | T_REG(0x25F800DA)) * 2;
    table = (unsigned short *)(0x25E00000 + offset);
    JT_TRUE(table[112 * 2] <= table[112 * 2 + 1]);
    jo_vdp2_set_window(JO_VDP2_WINDOW_0, 0, 0, JO_TV_WIDTH, JO_TV_HEIGHT);
}

static void             t_files_bugs(void)
{
    jo_img              img;
    jo_sound            sound;
    char                *data;
    int                 len;
    unsigned int        vblank;

    jt_group("bugs.fs");
    // A directory not found: no read, and the current directory doesn't change
    jt_expect_errors(true);
    data = jo_fs_read_file_in_dir("TEST.TXT", "NOPE", &len);
    jt_expect_errors(false);
    JT_TRUE(data == JO_NULL);
    data = jo_fs_read_file("TEST.TXT", &len);
    JT_TRUE(data != JO_NULL);
    if (data != JO_NULL)
        jo_free(data);
    jt_group("bugs.tga");
    // The image ID after the header is skipped (red, then blue)
    img.data = JO_NULL;
    JT_EQ(jo_tga_loader(&img, JO_ROOT_DIR, "ID4.TGA", JO_COLOR_Transparent), JO_TGA_OK);
    if (img.data != JO_NULL)
    {
        JT_EQ(img.width, 2);
        JT_EQ(img.data[0], JO_COLOR_Red);
        JT_EQ(img.data[1], JO_COLOR_Blue);
        jo_free(img.data);
    }
    // A compressed (RLE) image is refused
    img.data = JO_NULL;
    jt_expect_errors(true);
    JT_TRUE(jo_tga_loader(&img, JO_ROOT_DIR, "RLE.TGA", JO_COLOR_Transparent) != JO_TGA_OK);
    jt_expect_errors(false);
    jt_group("bugs.wav");
    // A data size of 0xFFFFFFFF (a stream): the size of the file
    jo_memset(&sound, 0, sizeof(sound));
    JT_TRUE(jo_audio_load_wav("STRM.WAV", &sound));
    JT_EQ(sound.data_length, 100);
    if (sound.data != JO_NULL)
        jo_free(sound.data);
    // A chunk of size 0xFFFFFFF8: refused at once (the walk didn't move before)
    jo_memset(&sound, 0, sizeof(sound));
    vblank = jo_core_get_vblank_count();
    jt_expect_errors(true);
    JT_TRUE(!jo_audio_load_wav("HANG.WAV", &sound));
    jt_expect_errors(false);
    JT_TRUE(jo_core_get_vblank_count() - vblank < 30);
}

/** @brief A model file loaded (or refused) by jo_3d_create_model_from_memory() */
static bool             t_load_model(const char * const name, const bool expected, const int animations)
{
    jo_3d_model         *model;
    char                *data;
    int                 len;
    bool                ok;

    data = jo_fs_read_file(name, &len);
    if (data == JO_NULL)
        return (false);
    jt_expect_errors(!expected);
    model = jo_3d_create_model_from_memory((const unsigned char *)data, (unsigned int)len);
    jt_expect_errors(false);
    ok = (model != JO_NULL) == expected;
    if (model != JO_NULL)
    {
        ok = ok && (animations < 0 || (int)model->animation_count == animations);
        jo_3d_free_model(model);
    }
    jo_free(data);
    return (ok);
}

static void             t_model_bugs(void)
{
    unsigned int        vblank;

    jt_group("bugs.jom");
    JT_TRUE(t_load_model("GOOD.JOM", true, 0));
    // A chunk of size 0xFFFFFFF8: refused at once (the walk didn't move before)
    vblank = jo_core_get_vblank_count();
    JT_TRUE(t_load_model("HANG.JOM", false, 0));
    JT_TRUE(jo_core_get_vblank_count() - vblank < 30);
    // A vertex after the points, more polygons than the chunk holds: refused
    JT_TRUE(t_load_model("BADV.JOM", false, 0));
    JT_TRUE(t_load_model("BADC.JOM", false, 0));
}

static void             t_3d_bugs(void)
{
    jo_fixed            a[12];
    jo_fixed            b[12];
    jo_camera           camera;
    jo_3d_mesh          *mesh;
    jo_pos3D_fixed      center;
    int                 i;

    jt_group("bugs.3d");
    // A push refused (the stack is full, 20 matrices): its pop doesn't pop the matrix under it
    jt_expect_errors(true);
    for (i = 0; i < 19; ++i)
        jo_3d_push_matrix();
    jo_3d_translate_matrix(5, 0, 0);
    jo_3d_get_matrix(a);
    jo_3d_push_matrix();
    jo_3d_pop_matrix();
    jo_3d_get_matrix(b);
    for (i = 0; i < 19; ++i)
        jo_3d_pop_matrix();
    jt_expect_errors(false);
    JT_EQ(b[9], a[9]);
    // A camera that looks almost straight down: its axes are unit vectors (its right axis was not normalized)
    jo_3d_camera_init(&camera);
    jo_3d_camera_set_viewpoint(&camera, 0, -500, 0);
    jo_3d_camera_set_target(&camera, 1, 0, 0);
    jo_3d_push_matrix();
    jo_3d_camera_look_at(&camera);
    jo_3d_get_matrix(a);
    jo_3d_pop_matrix();
    JT_TRUE(JO_ABS(a[0]) + JO_ABS(a[3]) + JO_ABS(a[6]) > 60000);
    jo_3d_camera_init(&camera);
    // The sphere of a big mesh (the squared size overflowed)
    mesh = jo_3d_create_mesh(1);
    if (mesh != JO_NULL)
    {
        jo_3d_set_mesh_vertice(mesh, jo_int2fixed(-25000), 0, jo_int2fixed(-25000), 0);
        jo_3d_set_mesh_vertice(mesh, jo_int2fixed(25000), 0, jo_int2fixed(-25000), 1);
        jo_3d_set_mesh_vertice(mesh, jo_int2fixed(25000), 0, jo_int2fixed(25000), 2);
        jo_3d_set_mesh_vertice(mesh, jo_int2fixed(-25000), 0, jo_int2fixed(25000), 3);
        JT_TRUE(jo_3d_mesh_get_bounding_sphere(mesh, &center) >= 35355);
        jo_3d_free_mesh(mesh);
    }
    // jo_malloc() of a huge size (a negative number given as a size): refused, it wrapped around before
    JT_TRUE(jo_malloc(0xFFFFFFF8u) == JO_NULL);
    JT_TRUE(jo_malloc(0xFFFFFFFAu) == JO_NULL);
}

/** @brief The list of the self-removal test (t_round3_bugs()) */
static jo_list          t_removal_list;
static int              t_removal_calls;

/** @brief Remove its own node, then reuse its memory (the loop read the next node in the freed node before) */
static void             t_remove_self(jo_node *node)
{
    unsigned char       *reuse;

    ++t_removal_calls;
    jo_list_remove(&t_removal_list, node);
    reuse = (unsigned char *)jo_malloc(sizeof(jo_node));
    if (reuse != JO_NULL)
    {
        jo_memset(reuse, 0xFF, sizeof(jo_node));
        jo_free(reuse);
    }
}

static void             t_round3_bugs(void)
{
    unsigned int        vblank;
    unsigned int        start;
    int                 a;
    int                 b;
    int                 c;

    jt_group("bugs.round3");
    // jo_list_foreach(): a callback that removes its own node (a callback of jo_core_add_callback() that stops itself)
    jo_list_init(&t_removal_list);
    jo_list_add_ptr(&t_removal_list, (void *)1);
    jo_list_add_ptr(&t_removal_list, (void *)2);
    jo_list_add_ptr(&t_removal_list, (void *)3);
    t_removal_calls = 0;
    jo_list_foreach(&t_removal_list, t_remove_self);
    JT_EQ(t_removal_calls, 3);
    JT_EQ(t_removal_list.count, 0);
    // (the animations of t_sprite_bugs(): the table is full)
    for (a = JO_MAX_SPRITE_ANIM - 1; a >= 0; --a)
        jo_remove_sprite_anim(a);
    // jo_create_sprite_anim(): the place of a removed animation is used again (the table was full after many creations)
    a = jo_create_sprite_anim(0, 2, 4);
    b = jo_create_sprite_anim(0, 2, 4);
    jo_remove_sprite_anim(a);
    c = jo_create_sprite_anim(0, 2, 4);
    JT_EQ(c, a);
    jo_remove_sprite_anim(b);
    jo_remove_sprite_anim(c);
    // The reserve of gouraud tables (an 8 x 256 block, never drawn): its height over 255 lines must be accepted
    JT_TRUE(jo_3d_add_gouraud_tables(JO_NULL, 2) != 0);
    // jo_get_ticks() called after 200 ms: the timer wrapped every 78 ms, the time between 2 calls was lost
    start = jo_get_ticks();
    vblank = jo_core_get_vblank_count();
    while (jo_core_get_vblank_count() - vblank < 12)
        ;
    JT_TRUE(jo_get_ticks() - start >= 180);
}

/** @brief A static table converted at the compilation (see the round 5 checks) */
static const jo_angle   t_static_angle = JO_DEG_TO_ANGLE(270);

/** @brief Math and 3D of the fifth review: each check failed before its fix */
static void             t_round5_bugs(void)
{
    jo_pos2D            point;
    jo_pos2D            origin;
    jo_pos2D            rotated;

    jt_group("bugs.round5");
    // jo_fixed_pow() with a negative exponent: (1 / x)^n (16^-4 was 32768, (-2)^-17 was positive)
    JT_NEAR(jo_fixed_pow(jo_int2fixed(16), jo_int2fixed(-4)), 1, 2);
    JT_TRUE(jo_fixed_pow(jo_int2fixed(-2), jo_int2fixed(-17)) <= 0);
    JT_NEAR(jo_fixed_pow(jo_int2fixed(2), jo_int2fixed(-1)), JO_FIXED_1 / 2, 2);
    // JO_DEG_TO_ANGLE() in a static initializer: 270 degrees is -90 (the compiler gave 32767, 180 degrees)
    JT_EQ(t_static_angle, -16384);
    // jo_sin() of a big angle: a full turn removed first (n * 8192 overflowed)
    JT_NEAR(jo_sin(1000000), jo_sin(1000000 % 360), 2);
    JT_NEAR(jo_sin(-1000000), jo_sin(-(1000000 % 360)), 2);
    // jo_atan2_fixed() of the smallest int
    JT_NEAR(jo_atan2_fixed((jo_fixed)0x80000000, 0), -16384, 2);
    // jo_planar_rotate(): 64 bits products (40000 pixels overflowed)
    point.x = 40000;
    point.y = 0;
    origin.x = 0;
    origin.y = 0;
    jo_planar_rotate(&point, &origin, 90, &rotated);
    JT_NEAR(rotated.x, 0, 2);
    JT_NEAR(JO_ABS(rotated.y), 40000, 2);
}

/** @brief Sprites and images of the sixth review (VDP1/VDP2 part): each check failed before its fix */
static void             t_round6_c_bugs(void)
{
    static const jo_tile out_tiles[2] = {{0, 0, 8, 8}, {16, 0, 8, 8}};
    static const jo_tile two_tiles[2] = {{0, 0, 8, 8}, {8, 0, 8, 8}};
    jo_raw_img          tiles[2];
    jo_img              img;
    jo_font             *font;
    int                 base;
    int                 last;
    int                 id;

    jt_group("bugs.round6_c");
    base = jo_get_last_sprite_id();
    // jo_sprite_add_tga_tileset(): a tile out of the image (16 x 8 pixels) is refused, as in the BIN files (its pixels
    // were read after the pixels of the file), no sprite is left
    jt_expect_errors(true);
    id = jo_sprite_add_tga_tileset(JO_ROOT_DIR, "T16.TGA", JO_COLOR_Transparent, out_tiles, 2);
    jt_expect_errors(false);
    JT_EQ(id, -1);
    JT_EQ(jo_get_last_sprite_id(), base);
    if (jo_get_last_sprite_id() > base)
        jo_sprite_free_from(base + 1);
    // A font whose characters are not all in the image: not loaded
    jt_expect_errors(true);
    font = jo_font_load(JO_ROOT_DIR, "T16.TGA", JO_COLOR_Transparent, 8, 8, 0, "ABC");
    jt_expect_errors(false);
    JT_TRUE(font == JO_NULL);
    JT_EQ(jo_get_last_sprite_id(), base);
    if (font != JO_NULL)
        jo_font_free(font);
    if (jo_get_last_sprite_id() > base)
        jo_sprite_free_from(base + 1);
    // jo_tga_8bits_tileset_loader(): the same check (an error, the tiles it allocated are freed)
    tiles[0].data = JO_NULL;
    tiles[1].data = JO_NULL;
    jt_expect_errors(true);
    JT_TRUE(jo_tga_8bits_tileset_loader(JO_ROOT_DIR, "T16.TGA", JO_COLOR_Transparent, out_tiles, 2, tiles) != JO_TGA_OK);
    jt_expect_errors(false);
    JT_TRUE(tiles[0].data == JO_NULL);
    if (tiles[0].data != JO_NULL)
        jo_free(tiles[0].data);
    if (tiles[1].data != JO_NULL)
        jo_free(tiles[1].data);
    // The sprites full after the first tile: -1 and the first tile removed (the font got the ids of the next sprites)
    img.width = 8;
    img.height = 1;
    img.data = JO_NULL;
    jt_expect_errors(true);
    while (jo_get_last_sprite_id() < JO_MAX_SPRITE - 2 && jo_sprite_add(&img) >= 0)
        ;
    last = jo_get_last_sprite_id();
    id = jo_sprite_add_tga_tileset(JO_ROOT_DIR, "T16.TGA", JO_COLOR_Transparent, two_tiles, 2);
    jt_expect_errors(false);
    JT_EQ(last, JO_MAX_SPRITE - 2);
    JT_EQ(id, -1);
    JT_EQ(jo_get_last_sprite_id(), last);
    if (jo_get_last_sprite_id() > base)
        jo_sprite_free_from(base + 1);
}


#if defined(JO_COMPILE_WITH_LEVEL_SUPPORT) && defined(JO_COMPILE_WITH_3D_SUPPORT)

/** @brief A level built in memory (see t_round6_e_bugs()): its directory, its sections */
typedef struct
{
    jo_level                    level;
    jo_level_directory_entry    directory[8];
    unsigned int                data[640];
    unsigned int                used;           // Bytes of data
    char                        strings[128];
    unsigned int                strings_size;
}                               t_memory_level;

static t_memory_level           t_mem_level;

static void                     t_mem_level_init(void)
{
    jo_memset(&t_mem_level, 0, sizeof(t_mem_level));
    t_mem_level.level.data = (unsigned char *)t_mem_level.data;
    t_mem_level.level.directory = t_mem_level.directory;
    // (offset 0: "")
    t_mem_level.strings_size = 1;
}

/** @brief A string of the section STRS of the level in memory: its offset */
static unsigned int             t_mem_string(const char * const s)
{
    unsigned int                offset;

    offset = t_mem_level.strings_size;
    jo_memcpy(t_mem_level.strings + offset, s, jo_strlen(s) + 1);
    t_mem_level.strings_size += jo_strlen(s) + 1;
    return (offset);
}

/** @brief Add a section of records to the level in memory (its records copied) */
static void                     t_mem_section(const unsigned int id, const void * const records, const unsigned int record_size,
                                              const unsigned int count)
{
    jo_level_directory_entry    *e;

    e = &t_mem_level.directory[t_mem_level.level.section_count++];
    e->id = id;
    e->version = 1;
    e->offset = t_mem_level.used;
    e->size = (record_size * count + 3) & ~3u;
    e->record_size = (unsigned short)record_size;
    e->record_count = count;
    jo_memcpy((unsigned char *)t_mem_level.data + e->offset, records, record_size * count);
    t_mem_level.used += e->size;
    t_mem_level.level.size = t_mem_level.used;
}

/** @brief The strings of the level in memory (after the other sections: their offsets are known) */
static void                     t_mem_strings(void)
{
    t_mem_section(JO_LEVEL_SECTION_STRS, t_mem_level.strings, 1, t_mem_level.strings_size);
    t_mem_level.directory[t_mem_level.level.section_count - 1].record_size = 0;
    t_mem_level.directory[t_mem_level.level.section_count - 1].record_count = 0;
}

/** @brief An entity of the level in memory */
static void                     t_mem_entity(jo_level_enti_record * const r, const char * const name, const char * const cls,
                                             const int x, const int z, const int radius, const int flags)
{
    jo_memset(r, 0, sizeof(*r));
    r->name = t_mem_string(name);
    r->cls = t_mem_string(cls);
    r->model = JO_LEVEL_NO_STRING;
    r->x = x;
    r->z = z;
    r->radius = (unsigned short)radius;
    r->height = 100;
    r->flags = (unsigned short)flags;
}

/** @brief A link of the level in memory */
static void                     t_mem_link(jo_level_link_record * const l, const int event, const int action, const int target,
                                           const int delay)
{
    l->event = (unsigned short)event;
    l->action = (unsigned short)action;
    l->target = (unsigned short)target;
    l->delay = (unsigned short)delay;
    l->param = 0;
}

/** @brief The light of a point light on a mesh not seen when the light was computed (round 6) */
static void                     t_round6_e_light(void)
{
    jo_color                    colors[8];
    jo_3d_mesh                  *mesh;
    jo_pos3D_fixed              p;
    unsigned short              base;
    int                         light;
    int                         k;

    jo_3d_point_lights_clear();
    for (k = 0; k < 8; ++k)
        colors[k] = (jo_color)(0x8000 | (8 << 10) | (8 << 5) | 8);
    base = jo_3d_add_gouraud_tables(colors, 1);
    mesh = jo_3d_create_indexed_mesh(4, 1);
    JT_TRUE(base != 0 && mesh != JO_NULL);
    if (base == 0 || mesh == JO_NULL)
        return;
    jo_3d_set_mesh_vertice(mesh, jo_int2fixed(950), 0, jo_int2fixed(-50), 0);
    jo_3d_set_mesh_vertice(mesh, jo_int2fixed(1050), 0, jo_int2fixed(-50), 1);
    jo_3d_set_mesh_vertice(mesh, jo_int2fixed(1050), 0, jo_int2fixed(50), 2);
    jo_3d_set_mesh_vertice(mesh, jo_int2fixed(950), 0, jo_int2fixed(50), 3);
    for (k = 0; k < 4; ++k)
        mesh->data.polygons[0].vertices[k] = (unsigned short)k;
    mesh->data.attributes[0].draw_mode |= JO_VDP1_COLOR_CALC_GOURAUD;
    mesh->data.attributes[0].gouraud_table = base;
    JT_TRUE(jo_3d_point_lights_add_mesh(mesh));
    // A lamp without shadows (it doesn't move) over the mesh not seen at its first update: lit when the mesh is seen
    jo_3d_point_lights_set_mesh_seen(mesh, false);
    p.x = jo_int2fixed(1000);
    p.y = jo_int2fixed(-30);
    p.z = 0;
    light = jo_3d_point_light_add(&p, 255, 255, 255, 60, 1024);
    jo_3d_point_light_set_shadows(light, false);
    jo_3d_point_lights_update();
    jo_3d_point_lights_set_mesh_seen(mesh, true);
    jo_3d_point_lights_update();
    jo_dma_wait();
    JT_TRUE((((volatile jo_color *)(JO_VDP1_VRAM + JO_MULT_BY_8(base)))[0] & 31) > 8);
    jo_3d_point_lights_clear();
    jo_3d_free_mesh(mesh);
    jo_3d_free_gouraud_tables(base, 1);
}

/** @brief Entities, vehicles, weapons, storyboards of the sixth review (part e): each check failed before its fix */
static void                     t_round6_e_bugs(void)
{
    jo_level_enti_record        enti[7];
    jo_level_link_record        links[3];
    jo_level_prop_record        props[2];
    jo_level_weap_record        weap;
    jo_pos3D_fixed              player;
    jo_pos3D_fixed              origin;
    jo_pos3D_fixed              dir;
    jo_pos3D_fixed              normal;
    jo_fixed                    t;
    jo_entity                   *e;
    jo_vehicle                  *v;
    int                         k;

    jt_group("bugs.round6_e");
    // A level in memory: relays a -> b -> (c after 1000 ms, d after 10 ms), a trigger zone wider than the wake distance, a
    // long box with a small radius, a vehicle
    t_mem_level_init();
    t_mem_entity(&enti[0], "a", "relay", 0, 0, 0, 0);
    enti[0].first_link = 0;
    enti[0].link_count = 1;
    t_mem_entity(&enti[1], "b", "relay", 0, 0, 0, 0);
    enti[1].first_link = 1;
    enti[1].link_count = 2;
    t_mem_entity(&enti[2], "c", "relay", 0, 0, 0, 0);
    t_mem_entity(&enti[3], "d", "relay", 0, 0, 0, 0);
    t_mem_entity(&enti[4], "zone", "trigger", 0, 0, 800, 0);
    t_mem_entity(&enti[5], "box", "entity", 250, 0, 10, JO_LEVEL_ENTITY_FLAG_SOLID);
    enti[5].first_prop = 0;
    enti[5].prop_count = 2;
    t_mem_entity(&enti[6], "car", "vehicle", 10000, -20000, 40, 0);
    t_mem_link(&links[0], JO_LEVEL_EVENT_TRIGGER, JO_LEVEL_ACTION_TRIGGER, 1, 20);
    t_mem_link(&links[1], JO_LEVEL_EVENT_TRIGGER, JO_LEVEL_ACTION_DISABLE, 2, 1000);
    t_mem_link(&links[2], JO_LEVEL_EVENT_TRIGGER, JO_LEVEL_ACTION_DISABLE, 3, 10);
    jo_memset(props, 0, sizeof(props));
    props[0].key = t_mem_string("box_x");
    props[0].value = 200;
    props[1].key = t_mem_string("box_z");
    props[1].value = 10;
    t_mem_section(JO_LEVEL_SECTION_ENTI, enti, sizeof(enti[0]), 7);
    t_mem_section(JO_LEVEL_SECTION_LINK, links, sizeof(links[0]), 3);
    t_mem_section(JO_LEVEL_SECTION_PROP, props, sizeof(props[0]), 2);
    t_mem_strings();
    JT_EQ(jo_entities_load(&t_mem_level.level), 7);
    // A trigger zone (radius 800) with a wake distance of 300: the player 500 units from its center is in it (it slept)
    player.x = jo_int2fixed(500);
    player.y = 0;
    player.z = 0;
    jo_entities_set_wake(300, false);
    jo_entities_update(&player, 16);
    e = jo_entity_find("zone");
    JT_TRUE(e != JO_NULL && (e->flags & JO_ENTITY_FLAG_PLAYER_INSIDE) != 0);
    jo_entities_set_wake(JO_ENTITY_WAKE_DISTANCE, false);
    // A delayed action queued by a delayed action (in a slot after it): its delay counts from the next update (it was
    // done in the same update, 10 ms after the 20 ms of the first one: at the same time)
    jo_entity_send(jo_entity_find("a"), JO_LEVEL_ACTION_TRIGGER, 0, JO_NULL);
    jo_entities_update(&player, 16);
    jo_entities_update(&player, 16);
    JT_EQ(jo_entity_find("d")->flags & JO_ENTITY_FLAG_DISABLED, 0);
    jo_entities_update(&player, 16);
    JT_EQ(jo_entity_find("d")->flags & JO_ENTITY_FLAG_DISABLED, JO_ENTITY_FLAG_DISABLED);
    JT_EQ(jo_entity_find("c")->flags & JO_ENTITY_FLAG_DISABLED, 0);
    // A ray toward the end of a long box (box_x 200, radius 10, center at 250): hit at 50 before the end of the ray at 100
    origin.x = 0;
    origin.y = jo_int2fixed(-50);
    origin.z = 0;
    dir.x = JO_FIXED_1;
    dir.y = 0;
    dir.z = 0;
    t = 0;
    JT_TRUE(jo_entities_ray_hit(&origin, &dir, jo_int2fixed(100), &t, &normal) == jo_entity_find("box"));
    JT_NEAR(jo_fixed2int(t), 50, 1);
    // A vehicle after a long frame (600 ms): it goes forward (step x throttle overflowed: backward), and never over its top
    // speed (900) after frames of 8 s
    v = jo_vehicle_get(jo_entity_find("car"));
    JT_TRUE(v != JO_NULL);
    if (v != JO_NULL)
    {
        jo_vehicle_drive(jo_entity_find("car"), 256, 0, false, 600);
        JT_TRUE(v->speed > 0);
        for (k = 0; k < 4; ++k)
            jo_vehicle_drive(jo_entity_find("car"), 256, 0, false, 8000);
        JT_TRUE(v->speed > 0 && v->speed <= jo_int2fixed(900));
        v->speed = 0;
        for (k = 0; k < 4; ++k)
            jo_vehicle_drive(jo_entity_find("car"), -256, 0, false, 8000);
        JT_TRUE(v->speed < 0 && v->speed >= -jo_int2fixed(300));
    }
    jo_entities_free();
    // Weapons: a reserve over ammo_max is not cut by a pickup, a big count doesn't overflow, the reserve stays >= 0
    t_mem_level_init();
    jo_memset(&weap, 0, sizeof(weap));
    weap.name = JO_LEVEL_NO_STRING;
    weap.model = JO_LEVEL_NO_STRING;
    weap.flash = JO_LEVEL_NO_STRING;
    weap.sound_shot = JO_LEVEL_NO_STRING;
    weap.sound_reload = JO_LEVEL_NO_STRING;
    weap.magazine = 5;
    weap.ammo_start = 50;
    weap.ammo_max = 30;
    t_mem_section(JO_LEVEL_SECTION_WEAP, &weap, sizeof(weap), 1);
    JT_EQ(jo_weapons_load(&t_mem_level.level), 1);
    jo_weapons_give_ammo(0, 5);
    JT_EQ(jo_weapons_reserve(), 50);
    jo_weapons_give_ammo(0, -45);
    JT_EQ(jo_weapons_reserve(), 5);
    jo_weapons_give_ammo(0, 0x7fffffff);
    JT_EQ(jo_weapons_reserve(), 30);
    jo_weapons_give_ammo(0, -100);
    JT_EQ(jo_weapons_reserve(), 0);
    t_mem_level_init();
    JT_EQ(jo_weapons_load(&t_mem_level.level), 0);
    t_round6_e_light();
}

#endif

#ifdef JO_COMPILE_WITH_STORYBOARD_SUPPORT

void                            jo_execute_storyboards(void);

/** @brief The object of the storyboard check of t_round6_e_storyboard() */
static jo_3d_object_attributes  t_story_turn;

/** @brief A rotation and a circle without end: the angles (shorts) went on from -32768 after 32767 (8 degrees back) */
static void                     t_round6_e_storyboard(void)
{
    jo_storyboard               *s;
    jo_animation                *a;
    jo_storyboard_object_cache  *cache;

    jt_group("bugs.round6_e");
    jo_memset(&t_story_turn, 0, sizeof(t_story_turn));
    t_story_turn.rz = 32760;
    s = jo_storyboard_create(true, false);
    JT_TRUE(s != JO_NULL);
    if (s == JO_NULL)
        return;
    jo_storyboard_add_object(s, &t_story_turn);
    a = jo_storyboard_create_animation(s, 0, JO_STORYBOARD_INFINITE_DURATION);
    cache = jo_storyboard_get_object_cache(s, &t_story_turn);
    JT_TRUE(a != JO_NULL && cache != JO_NULL);
    if (a != JO_NULL && cache != JO_NULL)
    {
        a->rotation_speed = 10;
        a->sin_radius = 80;
        a->translation_speed_y = 10;
        // (32760: 0 degrees, modulo 360)
        cache->current_sin = 32760;
        jo_execute_storyboards();
        jo_execute_storyboards();
        // 80 x sin(10 degrees), 20 degrees turned
        JT_NEAR(t_story_turn.y, 13, 1);
        JT_EQ(((t_story_turn.rz % 360) + 360) % 360, 20);
    }
    jo_storyboard_destroy(s);
}

#endif

#ifdef JO_COMPILE_WITH_3D_SUPPORT

/** @brief The red of each color of the gouraud tables of the commands of a frame from first (see t_round6_a_bugs())
 *  @return The colors with a red of 31, -1: a command without a gouraud table of the clipped polygons
 */
static int              t_r6a_full_reds(const jo_vdp1_command * const commands, const int count)
{
    const volatile unsigned short *table;
    int                 full;
    int                 i;
    int                 k;

    full = 0;
    for (i = 0; i < count; ++i)
    {
        if (commands[i].grda < JO_DIV_BY_8(JO_VDP1_CLIP_GOURAUD_TABLE_OFFSET))
            return (-1);
        table = (const volatile unsigned short *)(JO_VDP1_VRAM + JO_MULT_BY_8(commands[i].grda));
        for (k = 0; k < 4; ++k)
            if ((table[k] & 0x1f) == 31)
                ++full;
    }
    return (full);
}

/** @brief A big mesh cut in parts near the camera, alone (see t_round6_a_bugs()): its parts were not counted by the budget of
 *  the subdivisions of the frame until its end (only the commands of the objects before it), the command table overflowed
 *  (the polygons after it were lost). Its polygons: a wall from 2 to 6 times the near distance, 4 parts each (a texture of
 *  16 x 16 pixels: 2 x 2 tiles), enough to fill the table; within the budget, the polygons after it are drawn as one
 */
static void             t_r6a_budget(void)
{
    jo_img              img;
    jo_3d_mesh          *mesh;
    jo_fixed            (*m)[3];
    jo_fixed            n;
    unsigned int        k;
    unsigned int        count;
    int                 id;
    int                 before;
    int                 errors;
    bool                drawn;

    // (a quarter of the commands over the budget: 4 parts each fill the table, one each fits)
    count = (unsigned int)(jo_vdp1_command_max() - jo_vdp1_command_max() * 3 / 5) * 3 / 4;
    img.width = 16;
    img.height = 16;
    img.data = (jo_color *)jo_malloc(16 * 16 * sizeof(jo_color));
    if (img.data == JO_NULL)
        return;
    // (transparent: nothing seen in the frame of the checks)
    jo_memset(img.data, 0, 16 * 16 * sizeof(jo_color));
    id = jo_sprite_add(&img);
    jo_free(img.data);
    mesh = jo_3d_create_indexed_mesh(4, count);
    if (id < 0 || mesh == JO_NULL || !jo_3d_set_texture_subdivision(id))
    {
        JT_TRUE(false);
        if (mesh != JO_NULL)
            jo_3d_free_mesh(mesh);
        if (id >= 0)
            jo_sprite_free_from(id);
        return;
    }
    n = __jo_vdp1_near_clip();
    jo_3d_set_mesh_vertice(mesh, -n, -n, 2 * n, 0);
    jo_3d_set_mesh_vertice(mesh, n, -n, 6 * n, 1);
    jo_3d_set_mesh_vertice(mesh, n, n, 6 * n, 2);
    jo_3d_set_mesh_vertice(mesh, -n, n, 2 * n, 3);
    for (k = 0; k < count; ++k)
    {
        mesh->data.polygons[k].vertices[0] = 0;
        mesh->data.polygons[k].vertices[1] = 1;
        mesh->data.polygons[k].vertices[2] = 2;
        mesh->data.polygons[k].vertices[3] = 3;
        jo_3d_set_mesh_polygon_texture(mesh, id, k);
    }
    jo_3d_push_matrix();
    m = __jo_3d_current_matrix();
    for (k = 0; k < 4; ++k)
    {
        m[k][0] = k == 0 ? JO_FIXED_1 : 0;
        m[k][1] = k == 1 ? JO_FIXED_1 : 0;
        m[k][2] = k == 2 ? JO_FIXED_1 : 0;
    }
    before = __jo_vdp1_frame_command_count();
    jt_expect_errors(true);
    drawn = __jo_3d_draw(&mesh->data);
    errors = jt_expect_errors(false);
    jo_3d_pop_matrix();
    // (the check is meaningful while the budget is not spent by the commands before)
    JT_TRUE(before < jo_vdp1_command_max() * 3 / 5);
    JT_TRUE(drawn);
    JT_EQ(errors, 0);
    jo_3d_free_mesh(mesh);
    jo_sprite_free_from(id);
}

/** @brief A small model scaled by (0.003, 0.002, 0.003) (a scale that is not uniform: its normals transformed by the
 *  cofactors of the matrix, see t_round6_a_bugs()): its polygon facing the light is lit (the cofactors were 0 after their
 *  >> 16, the polygons kept their color, the neutral level)
 */
static void             t_r6a_small_light(void)
{
    static jo_pos3D_fixed points[4] = {{-JO_MULT_BY_65536(3000), -JO_MULT_BY_65536(5000), 0},
                                       {JO_MULT_BY_65536(3000), -JO_MULT_BY_65536(5000), 0},
                                       {JO_MULT_BY_65536(3000), JO_MULT_BY_65536(5000), 0},
                                       {-JO_MULT_BY_65536(3000), JO_MULT_BY_65536(5000), 0}};
    static jo_3d_polygon polygon = {{0, 0, -JO_FIXED_1}, JO_3D_VERTICES(0, 1, 2, 3)};
    static jo_3d_polygon_attribute attribute = JO_3D_ATTRIBUTE(JO_3D_SINGLE_PLANE, JO_3D_SORT_CENTER, JO_3D_NO_TEXTURE,
                                                               0x8000 | (16 << 10) | (16 << 5) | 16, 0, JO_VDP1_COLOR_MODE_RGB,
                                                               JO_VDP1_POLYGON, JO_3D_USE_LIGHT);
    jo_3d_polygon_data  data;
    jo_vdp1_command     *commands;
    unsigned short      *keys;
    unsigned int        reserved;
    jo_fixed            (*m)[3];
    int                 before;
    int                 count;
    int                 k;

    data.points = points;
    data.point_count = 4;
    data.polygons = &polygon;
    data.polygon_count = 1;
    data.attributes = &attribute;
    jo_3d_push_matrix();
    m = __jo_3d_current_matrix();
    for (k = 0; k < 4; ++k)
    {
        m[k][0] = k == 0 ? JO_FIXED_1 : 0;
        m[k][1] = k == 1 ? JO_FIXED_1 : 0;
        m[k][2] = k == 2 ? JO_FIXED_1 : 0;
    }
    // (the light toward +Z in the view: the default one)
    jo_3d_light_fixed(0, 0, JO_FIXED_1);
    m[0][0] = JO_FIXED_1 * 3 / 1000;
    m[1][1] = JO_FIXED_1 * 2 / 1000;
    m[2][2] = JO_FIXED_1 * 3 / 1000;
    m[3][2] = JO_MULT_BY_65536(100);
    before = __jo_vdp1_frame_command_count();
    commands = __jo_vdp1_reserve_commands(0, &keys, &reserved);
    __jo_3d_draw(&data);
    count = __jo_vdp1_frame_command_count() - before;
    jo_3d_pop_matrix();
    JT_EQ(count, 1);
    // (facing the light: the brightest level, each component 31)
    if (count == 1)
        JT_EQ(commands[0].colr, 0xFFFF);
}

/** @brief Draw a mesh of one textured polygon with the current matrix set to the identity (see t_round6_a_bugs())
 *  @return Its first command, JO_NULL: no command
 */
static const jo_vdp1_command *t_r6a_draw_at_identity(const jo_3d_mesh * const mesh, int * const count)
{
    jo_vdp1_command     *commands;
    unsigned short      *keys;
    unsigned int        reserved;
    jo_fixed            (*m)[3];
    int                 before;
    int                 k;

    jo_3d_push_matrix();
    m = __jo_3d_current_matrix();
    for (k = 0; k < 4; ++k)
    {
        m[k][0] = k == 0 ? JO_FIXED_1 : 0;
        m[k][1] = k == 1 ? JO_FIXED_1 : 0;
        m[k][2] = k == 2 ? JO_FIXED_1 : 0;
    }
    before = __jo_vdp1_frame_command_count();
    commands = __jo_vdp1_reserve_commands(0, &keys, &reserved);
    __jo_3d_draw(&mesh->data);
    *count = __jo_vdp1_frame_command_count() - before;
    jo_3d_pop_matrix();
    return (*count > 0 ? commands : JO_NULL);
}

/** @brief A polygon cut in parts whose last part crosses the near plane and is clipped by it (see jo_3d_emit_near_tile()):
 *  the 4 vertices kept for it are all under the screen (the fifth one, left out, on the screen), the part is not drawn
 *  (see jo_3d_crop_part()). The words of its tile stayed in the state of the CPU: the next textured polygon (a small one, on
 *  the screen) was drawn with the tile instead of its texture (see t_round6_a_bugs()). The positions: near distance / 20
 *  units, the screen positions of the 320 x 224 mode (the part: (-20, 22, 25), (60, 60, 50), (60, 76, 40), (-20, -10, 10))
 */
static void             t_r6a_stale_tile(void)
{
    static const short  corners[4][3] = {{-100, 64, 20}, {60, 44, 60}, {60, 76, 40}, {-100, -96, -20}};
    jo_img              img;
    jo_3d_mesh          *mesh;
    jo_3d_mesh          *probe;
    const jo_vdp1_command *cmd;
    jo_fixed            unit;
    int                 id;
    int                 count;
    int                 k;

    img.width = 16;
    img.height = 16;
    img.data = (jo_color *)jo_malloc(16 * 16 * sizeof(jo_color));
    if (img.data == JO_NULL)
        return;
    jo_memset(img.data, 0, 16 * 16 * sizeof(jo_color));
    id = jo_sprite_add(&img);
    jo_free(img.data);
    mesh = jo_3d_create_indexed_mesh(4, 1);
    probe = jo_3d_create_indexed_mesh(4, 1);
    if (id < 0 || mesh == JO_NULL || probe == JO_NULL || !jo_3d_set_texture_subdivision(id))
    {
        JT_TRUE(false);
        if (probe != JO_NULL)
            jo_3d_free_mesh(probe);
        if (mesh != JO_NULL)
            jo_3d_free_mesh(mesh);
        if (id >= 0)
            jo_sprite_free_from(id);
        return;
    }
    unit = __jo_vdp1_near_clip() / 20;
    for (k = 0; k < 4; ++k)
    {
        jo_3d_set_mesh_vertice(mesh, corners[k][0] * unit, corners[k][1] * unit, corners[k][2] * unit, k);
        mesh->data.polygons[0].vertices[k] = (unsigned short)k;
        probe->data.polygons[0].vertices[k] = (unsigned short)k;
    }
    jo_3d_set_mesh_vertice(probe, -10 * unit, -10 * unit, 100 * unit, 0);
    jo_3d_set_mesh_vertice(probe, 10 * unit, -10 * unit, 100 * unit, 1);
    jo_3d_set_mesh_vertice(probe, 10 * unit, 10 * unit, 100 * unit, 2);
    jo_3d_set_mesh_vertice(probe, -10 * unit, 10 * unit, 100 * unit, 3);
    jo_3d_set_mesh_polygon_texture(mesh, id, 0);
    jo_3d_set_mesh_polygon_texture(probe, id, 0);
    mesh->data.attributes[0].plane = JO_3D_DUAL_PLANE;
    (void)t_r6a_draw_at_identity(mesh, &count);
    cmd = t_r6a_draw_at_identity(probe, &count);
    JT_EQ(count, 1);
    if (cmd != JO_NULL)
    {
        JT_EQ(cmd->srca, __jo_sprite_def[id].adr);
        JT_EQ(cmd->size, __jo_sprite_def[id].size);
    }
    jo_3d_free_mesh(probe);
    jo_3d_free_mesh(mesh);
    jo_sprite_free_from(id);
}

#endif

/** @brief 3D of the sixth review (part a): each check failed before its fix */
static void             t_round6_a_bugs(void)
{
#ifdef JO_COMPILE_WITH_3D_SUPPORT
    static jo_color     colors[4] = {0x8000 | 31, 0x8000, 0x8000 | 31, 0x8000 | 31};
    static jo_3d_polygon polygon = {{0, 0, -JO_FIXED_1}, JO_3D_VERTICES(0, 1, 2, 2)};
    static jo_3d_polygon_attribute attribute = JO_3D_ATTRIBUTE(JO_3D_DUAL_PLANE, JO_3D_SORT_CENTER, JO_3D_NO_TEXTURE, 0x8000 | 31, 0,
                                                               JO_VDP1_COLOR_MODE_RGB | JO_VDP1_COLOR_CALC_GOURAUD, JO_VDP1_POLYGON,
                                                               JO_3D_NO_OPTION);
    jo_pos3D_fixed      points[3];
    jo_3d_polygon_data  data;
    jo_vdp1_command     *commands;
    unsigned short      *keys;
    unsigned int        reserved;
    jo_fixed            (*m)[3];
    jo_fixed            n;
    int                 before;
    int                 count;
    int                 i;
#endif

    jt_group("bugs.round6_a");
#ifdef JO_COMPILE_WITH_3D_SUPPORT
    // A gouraud triangle A B C whose corner C is behind the camera (clipped): the VDP1 draws it as A B C C, the colors of
    // the edge A-C go from A to D. A, C and D red, B black: the point of the edge A-C on the near plane is red (it got
    // some of the black of B, 23 instead of 31: a seam with the triangle next to it)
    attribute.gouraud_table = jo_3d_add_gouraud_tables(colors, 1);
    n = __jo_vdp1_near_clip();
    points[0].x = -JO_MULT_BY_4(n) / 5;
    points[0].y = -JO_MULT_BY_4(n) / 5;
    points[0].z = 3 * n;
    points[1].x = JO_MULT_BY_4(n) / 5;
    points[1].y = -JO_MULT_BY_4(n) / 5;
    points[1].z = 3 * n;
    points[2].x = 0;
    points[2].y = JO_MULT_BY_4(n) / 5;
    points[2].z = -n;
    data.points = points;
    data.point_count = 3;
    data.polygons = &polygon;
    data.polygon_count = 1;
    data.attributes = &attribute;
    jo_3d_push_matrix();
    m = __jo_3d_current_matrix();
    for (i = 0; i < 4; ++i)
    {
        m[i][0] = i == 0 ? JO_FIXED_1 : 0;
        m[i][1] = i == 1 ? JO_FIXED_1 : 0;
        m[i][2] = i == 2 ? JO_FIXED_1 : 0;
    }
    before = __jo_vdp1_frame_command_count();
    commands = __jo_vdp1_reserve_commands(0, &keys, &reserved);
    __jo_3d_draw(&data);
    count = __jo_vdp1_frame_command_count() - before;
    jo_3d_pop_matrix();
    JT_EQ(count, 1);
    // (A and the point of the edge A-C red: 2 colors of the table of the clipped polygon)
    JT_EQ(t_r6a_full_reds(commands, count), 2);
    jo_3d_free_gouraud_tables(attribute.gouraud_table, 1);
    t_r6a_small_light();
    t_r6a_stale_tile();
    // (last: the commands of its mesh fill the frame)
    t_r6a_budget();
#endif
}

/** @brief The sixth review (core, memory, audio, inputs): each check failed before its fix */
static void             t_round6_d_bugs(void)
{
    void                *p;
    bool                refused;
#ifdef JO_COMPILE_WITH_AUDIO_SUPPORT
    jo_sound            copied;
    jo_sound            resident;
    int                 c;
#endif
#ifdef JO_COMPILE_WITH_KEYBOARD_SUPPORT
    jo_peripheral       saved;
    jo_peripheral_analog *pad;
    unsigned int        sr;
    jo_keyboard_special_key key;
    unsigned char       ch;
#endif

    jt_group("bugs.round6_d");
    // jo_malloc() of a negative size (-100000): the address of the end of the block wrapped around, the block was given
    // and the top of the zone went back over the blocks in use (freed at once: the top of the zone is restored)
    p = jo_malloc((unsigned int)-100000);
    refused = p == JO_NULL;
    if (p != JO_NULL)
        jo_free(p);
    JT_TRUE(refused);
    p = jo_malloc_with_behaviour((unsigned int)-100000, JO_FAST_ALLOCATION);
    refused = p == JO_NULL;
    if (p != JO_NULL)
        jo_free(p);
    JT_TRUE(refused);
#ifdef JO_COMPILE_WITH_AUDIO_SUPPORT
    // A resident sound over the copy of a sound that plays: the channel stopped (it played the new data, a strident noise)
    for (c = 0; c < JO_SOUND_MAX_CHANNEL; ++c)
        jo_audio_stop_channel(c);
    jo_audio_free_resident();
    jo_memset(&copied, 0, sizeof(copied));
    // (any memory: as big as the sound RAM, copied from its start to the area of the resident sounds; silent)
    copied.data = (char *)0x06004000;
    copied.data_length = 0x7F000;
    copied.mode = JoSoundMono16Bit;
    copied.volume = 0;
    jo_audio_play_sound_loop_on_channel(&copied, JO_SOUND_MAX_CHANNEL - 1);
    JT_TRUE(jo_audio_is_channel_playing(JO_SOUND_MAX_CHANNEL - 1));
    jo_memset(&resident, 0, sizeof(resident));
    resident.data = (char *)jo_malloc(1024);
    if (resident.data != JO_NULL)
    {
        jo_memset(resident.data, 0, 1024);
        resident.data_length = 1024;
        resident.mode = JoSoundMono16Bit;
        JT_TRUE(jo_audio_make_resident(&resident));
        JT_TRUE(!jo_audio_is_channel_playing(JO_SOUND_MAX_CHANNEL - 1));
    }
    jo_audio_stop_channel(JO_SOUND_MAX_CHANNEL - 1);
    jo_audio_free_resident();
#endif
#ifdef JO_COMPILE_WITH_KEYBOARD_SUPPORT
    // A 3D pad on the port 1 (thumb x 0x88: the make flag, y 90: Enter): no key (the error screen was closed by the pad)
    sr = __jo_core_disable_interrupts();
    saved = __jo_smpc_peripherals[0];
    pad = (jo_peripheral_analog *)&__jo_smpc_peripherals[0];
    pad->id = JO_PERIPHERAL_ID_NIGHTS_PAD;
    pad->x = 0x88;
    pad->y = 90;
    key = jo_keyboard_get_special_key();
    pad->y = 28;
    ch = jo_keyboard_get_char();
    __jo_smpc_peripherals[0] = saved;
    __jo_core_restore_interrupts(sr);
    JT_EQ(key, JO_KEYBOARD_NO_SPECIAL_KEY);
    JT_EQ(ch, 0);
#endif
}

/** @brief Write a chunk tag of a JOM file in a buffer of words (see t_round6_b_bugs()) */
static void             t_jom_tag(unsigned int * const w, const char * const tag)
{
    jo_memcpy(w, tag, 4);
}

/** @brief A JOM file in memory: an embedded texture, then a "TEXT" chunk of 16 names and a second "TEXT" of 1 name
 *  @return Size in bytes */
static unsigned int     t_jom_two_texts(unsigned int * const w)
{
    unsigned int        n;
    int                 k;

    jo_memset(w, 0, 128 * sizeof(unsigned int));
    t_jom_tag(w, "JOM1");
    w[1] = 0x00010000;
    n = 6;
    // "TIMG": 1 texture of 8 x 1 pixels (RGB)
    t_jom_tag(&w[n], "TIMG");
    w[n + 1] = 4 + 24 + 16;
    w[n + 2] = 1;
    jo_memcpy(&w[n + 3], "T6BTEX", 7);
    w[n + 7] = (8u << 16) | 1u;
    w[n + 8] = 0;
    for (k = 0; k < 4; ++k)
        w[n + 9 + k] = 0xFC00FC00u;
    n += 13;
    // "TEXT": 16 names, then "TEXT": 1 name
    t_jom_tag(&w[n], "TEXT");
    w[n + 1] = 4 + 16 * 16;
    w[n + 2] = 16;
    for (k = 0; k < 16; ++k)
        jo_memcpy(&w[n + 3 + k * 4], "T6BTEX", 7);
    n += 3 + 16 * 4;
    t_jom_tag(&w[n], "TEXT");
    w[n + 1] = 4 + 16;
    w[n + 2] = 1;
    jo_memcpy(&w[n + 3], "T6BTEX", 7);
    n += 7;
    t_jom_tag(&w[n], "END ");
    w[n + 1] = 0;
    n += 2;
    return (n * sizeof(unsigned int));
}

static unsigned int     t_jom_words[128];

/** @brief Review 6 (part b) of 3d.c and model.c: each check failed before its fix */
static void             t_round6_b_bugs(void)
{
    jo_3d_mesh          *mesh;
    jo_3d_ray_plane     *planes;
    jo_3d_model         *model;
    jo_pos3D_fixed      o;
    jo_pos3D_fixed      d;
    jo_pos3D_fixed      normal;
    jo_fixed            best;
    jo_color            lut[16];
    unsigned char       pixels[32 * 32 / 2];
    unsigned char       *probe;
    unsigned int        block;
    unsigned int        size;
    unsigned int        k;
    int                 untouched;
    int                 first;
    int                 id;
    int                 index;
    bool                hit;

    jt_group("bugs.round6_b");
    // jo_3d_create_model_in_place(): a first "TEXT" bigger than a second one (a wrong file) wrote its ids after the block of
    // the model (the block holds the ids of the last one). The block is at the end of the memory used: the memory after it
    // is filled first, then read again
    first = jo_get_last_sprite_id() + 1;
    size = t_jom_two_texts(t_jom_words);
    block = ((sizeof(jo_3d_model) + 3) & ~3u) + ((sizeof(jo_3d_mesh) + 3) & ~3u) + sizeof(int);
    untouched = -1;
    probe = (unsigned char *)jo_malloc(block + 256);
    if (probe != JO_NULL)
    {
        jo_memset(probe, 0x5A, block + 256);
        jo_free(probe);
        jt_expect_errors(true);
        model = jo_3d_create_model_in_place((const unsigned char *)t_jom_words, size);
        jt_expect_errors(false);
        if (model != JO_NULL)
            jo_3d_free_model(model);
        probe = (unsigned char *)jo_malloc(block + 256);
        if (probe != JO_NULL)
        {
            for (untouched = 1, k = block; k < block + 64; ++k)
                if (probe[k] != 0x5A)
                    untouched = 0;
            jo_free(probe);
        }
    }
    JT_EQ(untouched, 1);
    if (jo_get_last_sprite_id() >= first)
        jo_3d_model_free_textures_from(first);
    // jo_3d_mesh_ray_hit_with_planes(): a long ray (its end over 32767 units) found the wall that jo_3d_mesh_ray_hit() finds
    // (the box of the segment wrapped to the other side of the origin)
    mesh = jo_3d_create_mesh(1);
    if (mesh != JO_NULL)
    {
        jo_3d_set_mesh_vertice(mesh, jo_int2fixed(1100), jo_int2fixed(-50), jo_int2fixed(-50), 0);
        jo_3d_set_mesh_vertice(mesh, jo_int2fixed(1100), jo_int2fixed(-50), jo_int2fixed(50), 1);
        jo_3d_set_mesh_vertice(mesh, jo_int2fixed(1100), jo_int2fixed(50), jo_int2fixed(50), 2);
        jo_3d_set_mesh_vertice(mesh, jo_int2fixed(1100), jo_int2fixed(50), jo_int2fixed(-50), 3);
        jo_3d_set_mesh_normal(mesh, -JO_FIXED_1, 0, 0, 0);
        planes = jo_3d_mesh_create_ray_planes(mesh);
        o.x = jo_int2fixed(1000);
        o.y = 0;
        o.z = 0;
        d.x = JO_FIXED_1;
        d.y = 0;
        d.z = 0;
        best = jo_int2fixed(32000);
        hit = jo_3d_mesh_ray_hit(mesh, &o, &d, &best, &normal);
        JT_TRUE(hit);
        JT_NEAR(jo_fixed2int(best), 100, 1);
        best = jo_int2fixed(32000);
        hit = planes != JO_NULL && jo_3d_mesh_ray_hit_with_planes(mesh, planes, &o, &d, &best, &normal);
        JT_TRUE(hit);
        JT_NEAR(jo_fixed2int(best), 100, 1);
        if (planes != JO_NULL)
            jo_free(planes);
        jo_3d_free_mesh(mesh);
    }
    // The smaller copies of a 16 colors texture: only the colors of its table (the entries not used are 0: a dark average
    // took one of them, a transparent pixel in the copy). Red, green and blue (12) and a transparent pixel in each 2 x 2 block
    jo_memset(lut, 0, sizeof(lut));
    lut[1] = JO_COLOR_SATURN_MSB | 12;
    lut[2] = JO_COLOR_SATURN_MSB | (12 << 5);
    lut[3] = JO_COLOR_SATURN_MSB | (12 << 10);
    for (k = 0; k < sizeof(pixels); ++k)
        pixels[k] = ((k / 16) & 1) ? 0x30 : 0x12;
    first = jo_get_last_sprite_id() + 1;
    id = jo_sprite_add_4bits_lut(pixels, 32, 32, lut);
    index = -1;
    if (id >= 0 && jo_3d_set_texture_mipmaps(id) && jo_get_last_sprite_id() > id)
        index = *(volatile unsigned char *)(JO_VDP1_VRAM + JO_MULT_BY_8(__jo_sprite_def[id + 1].adr)) >> 4;
    JT_TRUE(index >= 1 && index <= 3);
    if (jo_get_last_sprite_id() >= first)
        jo_sprite_free_from(first);
}

void                    t_bugs(void)
{
    t_3d_bugs();
    t_model_bugs();
    t_sprintf_bugs();
    t_math_bugs();
    t_ticks_bugs();
    t_sprite_bugs();
    t_vdp2_bugs();
    t_files_bugs();
    t_round3_bugs();
    t_round5_bugs();
    t_round6_c_bugs();
#if defined(JO_COMPILE_WITH_LEVEL_SUPPORT) && defined(JO_COMPILE_WITH_3D_SUPPORT)
    t_round6_e_bugs();
#endif
#ifdef JO_COMPILE_WITH_STORYBOARD_SUPPORT
    t_round6_e_storyboard();
#endif
    t_round6_d_bugs();
    t_round6_a_bugs();
    t_round6_b_bugs();
}
