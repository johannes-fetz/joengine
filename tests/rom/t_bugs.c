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
}
