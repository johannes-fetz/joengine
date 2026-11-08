/*
** Jo Sega Saturn Engine - regression test ROM: state of the engine and of the hardware after the API calls (screen
** modes, VDP2 registers, palettes, sprites, files)
*/

#include "jt.h"
#include "tests.h"

#define T_VDP2_REGS             (0x120 / 2)
#define T_REG(ADDRESS)          (jo_vdp2_get_registers()[((ADDRESS) - 0x25F80000) / 2])
#define T_TVMD                  (0x25F80000)
#define T_BGON                  (0x25F80020)
#define T_MZCTL                 (0x25F80022)
#define T_SFCODE                (0x25F80026)
#define T_CHCTLA                (0x25F80028)
#define T_PNCN0                 (0x25F80030)
#define T_WCTLA                 (0x25F800D0)
#define T_SPCTL                 (0x25F800E0)
#define T_SDCTL                 (0x25F800E2)
#define T_CRAOFB                (0x25F800E6)
#define T_CCCTL                 (0x25F800EC)
#define T_CCRNA                 (0x25F80108)
#define T_LCTAU                 (0x25F800A8)
#define T_LNCLEN                (0x25F800E8)
#define T_CCRLB                 (0x25F8010E)

static unsigned short           t_base[T_VDP2_REGS];

static void                     t_snapshot(void)
{
    jo_memcpy(t_base, jo_vdp2_get_registers(), sizeof(t_base));
}

/** @brief Record the registers changed since t_snapshot() (pairs: index, value) and return their count */
static int                      t_diff(const char * const name)
{
    unsigned short              pairs[2 * 48];
    const unsigned short        *regs;
    int                         count;
    int                         i;

    regs = jo_vdp2_get_registers();
    for (count = 0, i = 0; i < T_VDP2_REGS; ++i)
    {
        if (regs[i] == t_base[i])
            continue;
        if (count < 48)
        {
            pairs[2 * count] = (unsigned short)i;
            pairs[2 * count + 1] = regs[i];
        }
        ++count;
    }
    jt_blob(name, pairs, 4 * JO_MIN(count, 48));
    return (count);
}

/*
** Screen modes
*/

static void                     t_screen_modes(void)
{
    static const short          modes[][2] = {{320, 224}, {320, 240}, {640, 224}, {640, 448}, {320, 448}, {640, 480},
                                              {352, 224}, {352, 240}, {704, 240}, {704, 448}, {704, 480}, {352, 480}};
    char                        name[40];
    int                         base_width;
    int                         base_height;
    bool                        clock_352;
    bool                        expected;
    bool                        ok;
    int                         i;
    int                         w;
    int                         h;

    jt_group("screen.modes");
    base_width = jo_core_get_screen_width();
    base_height = jo_core_get_screen_height();
    JT_EQ(base_width, JO_TV_WIDTH);
    JT_EQ(base_height, JO_TV_HEIGHT);
    clock_352 = base_width == 352 || base_width == 704;
    JT_EQ(__jo_screen.clock_352, clock_352);
    JT_TRUE(!jo_core_set_screen_resolution(400, 224));
    JT_TRUE(!jo_core_set_screen_resolution(320, 300));
    JT_EQ(jo_core_get_screen_width(), base_width);
    for (i = 0; i < JT_COUNT(modes); ++i)
    {
        w = modes[i][0];
        h = modes[i][1];
        expected = (w == 352 || w == 704) == clock_352;
        ok = jo_core_set_screen_resolution(w, h);
        JT_EQ(ok, expected);
        if (!ok)
            continue;
        JT_EQ(jo_core_get_screen_width(), w);
        JT_EQ(jo_core_get_screen_height(), h);
        JT_EQ(__jo_screen.width_2, w / 2);
        JT_EQ(__jo_screen.height_2, h / 2);
        JT_EQ(__jo_screen.hires, w > 352);
        JT_EQ(__jo_screen.interlace, h > 256);
        JT_EQ(__jo_screen.background_width, w > 352 ? w / 2 : w);
        JT_EQ(__jo_screen.background_height, h > 256 ? h / 2 : h);
        // TVMD: HRESO bit 1 = 640/704, bit 0 = 352/704, LSMD (bits 7-6) = 3 in double interlace
        JT_EQ((T_REG(T_TVMD) >> 1) & 1, w > 352);
        JT_EQ(T_REG(T_TVMD) & 1, w == 352 || w == 704);
        JT_EQ((T_REG(T_TVMD) >> 6) & 3, h > 256 ? 3 : 0);
        jo_sprintf(name, "screen.%dx%d.tvmd", w, h);
        jt_value(name, T_REG(T_TVMD));
        jo_sprintf(name, "screen.%dx%d.spctl", w, h);
        jt_value(name, T_REG(T_SPCTL));
        jo_sprintf(name, "screen.%dx%d.regs", w, h);
        jt_value(name, jt_hash(JT_HASH_INIT, jo_vdp2_get_registers(), 2 * T_VDP2_REGS));
    }
    // Single interlace (LSMD = 2): the modes of 224 and 240 lines only, the double interlace of the 448 and 480 lines first
    JT_TRUE(jo_core_set_screen_resolution(clock_352 ? 352 : 320, 240));
    jo_core_set_screen_single_interlace(true);
    JT_EQ((T_REG(T_TVMD) >> 6) & 3, 2);
    JT_TRUE(jo_core_set_screen_resolution(clock_352 ? 704 : 640, 480));
    JT_EQ((T_REG(T_TVMD) >> 6) & 3, 3);
    jo_core_set_screen_single_interlace(false);
    JT_TRUE(jo_core_set_screen_resolution(clock_352 ? 352 : 320, 240));
    JT_EQ((T_REG(T_TVMD) >> 6) & 3, 0);
    JT_TRUE(jo_core_set_screen_resolution(base_width, base_height));
    JT_EQ(jo_core_get_screen_width(), base_width);
    JT_EQ(jo_core_get_screen_height(), base_height);
}

/*
** VDP2 effects: the registers changed, then the registers back to their values when the effect is disabled
*/

static void                     t_vdp2_effects(void)
{
    jt_group("vdp2.window");
    t_snapshot();
    jo_vdp2_set_window(JO_VDP2_WINDOW_0, 16, 24, 100, 60);
    jo_vdp2_enable_window(JO_NBG1_SCREEN | JO_SPRITE_SCREEN, JO_VDP2_WINDOW_0, JO_VDP2_WINDOW_SHOW_INSIDE);
    JT_TRUE(t_diff("vdp2.window.enable") > 0);
    jo_vdp2_disable_window(JO_NBG1_SCREEN | JO_SPRITE_SCREEN);
    jo_vdp2_set_window(JO_VDP2_WINDOW_0, 0, 0, JO_TV_WIDTH, JO_TV_HEIGHT);
    t_diff("vdp2.window.disable");
    JT_TRUE(jo_vdp2_set_window_circle(JO_VDP2_WINDOW_1, 160, 112, 50));
    jo_vdp2_enable_window(JO_NBG1_SCREEN, JO_VDP2_WINDOW_1, JO_VDP2_WINDOW_SHOW_OUTSIDE);
    t_diff("vdp2.window.circle");
    jo_vdp2_disable_window(JO_NBG1_SCREEN);

    jt_group("vdp2.blend");
    t_snapshot();
    jo_vdp2_enable_blend(JO_NBG1_SCREEN, 16);
    JT_EQ((T_REG(T_CCCTL) >> 1) & 1, 1);
    JT_EQ((T_REG(T_CCRNA) >> 8) & 31, 16);
    t_diff("vdp2.blend.nbg1");
    jo_vdp2_set_blend_mode(JO_VDP2_BLEND_ADD | JO_VDP2_BLEND_EXTENDED);
    t_diff("vdp2.blend.add_extended");
    jo_vdp2_set_blend_mode(JO_VDP2_BLEND_RATIO);
    jo_vdp2_disable_blend(JO_NBG1_SCREEN);
    JT_EQ((T_REG(T_CCCTL) >> 1) & 1, 0);
    // (no blur in the 640 and 704 pixels modes)
    JT_EQ(jo_vdp2_enable_blur(JO_NBG1_SCREEN), !__jo_screen.hires);
    t_diff("vdp2.blur");
    jo_vdp2_disable_blur();

    jt_group("vdp2.shadow");
    t_snapshot();
    jo_vdp2_enable_shadow(JO_NBG1_SCREEN | JO_RBG0_SCREEN);
    JT_EQ((T_REG(T_SDCTL) >> 1) & 1, 1);
    JT_EQ((T_REG(T_SDCTL) >> 4) & 1, 1);
    t_diff("vdp2.shadow.enable");
    jo_vdp2_disable_shadow();
    JT_EQ(T_REG(T_SDCTL) & 0x3F, 0);
    JT_EQ(t_diff("vdp2.shadow.disable"), 0);

    jt_group("vdp2.mosaic");
    t_snapshot();
    jo_vdp2_enable_mosaic(JO_NBG1_SCREEN, 4, 2);
    JT_EQ((T_REG(T_MZCTL) >> 1) & 1, 1);
    JT_EQ((T_REG(T_MZCTL) >> 8) & 15, 3);
    JT_EQ((T_REG(T_MZCTL) >> 12) & 15, 1);
    t_diff("vdp2.mosaic.enable");
    jo_vdp2_disable_mosaic();
    JT_EQ(T_REG(T_MZCTL) & 0x1F, 0);

    jt_group("vdp2.special");
    t_snapshot();
    jo_vdp2_set_special_colors(0x0C);
    jo_vdp2_enable_special_priority(JO_NBG1_SCREEN);
    jo_vdp2_enable_special_blend(JO_NBG1_SCREEN);
    t_diff("vdp2.special.enable");
    jo_vdp2_disable_special_priority(JO_NBG1_SCREEN);
    jo_vdp2_disable_special_blend(JO_NBG1_SCREEN);
    jo_vdp2_set_special_colors(0);
    t_diff("vdp2.special.disable");

    jt_group("vdp2.sprites");
    t_snapshot();
    jo_vdp2_set_sprite_priorities(7, 5, 3, 1);
    JT_EQ(T_REG(0x25F800F0), 0x0507);
    JT_EQ(T_REG(0x25F800F2), 0x0103);
    t_diff("vdp2.sprite_priorities");
    jo_vdp2_set_sprite_priorities(t_base[0xF0 / 2] & 7, (t_base[0xF0 / 2] >> 8) & 7, t_base[0xF2 / 2] & 7, (t_base[0xF2 / 2] >> 8) & 7);
    JT_EQ(t_diff("vdp2.sprite_priorities.restore"), 0);
    t_snapshot();
    jo_set_screen_color_filter_a(JO_NBG1_SCREEN, 10, -10, 20);
    t_diff("vdp2.color_filter_a");
    jo_set_screen_color_filter_a(JO_NBG1_SCREEN, 0, 0, 0);

    jt_group("vdp2.line_color");
    t_snapshot();
    {
        unsigned short          colors[JO_TV_HEIGHT];
        int                     line;

        for (line = 0; line < JO_TV_HEIGHT; ++line)
            colors[line] = JO_VDP2_LINE_COLOR(1, line & 63);
        JT_TRUE(jo_vdp2_set_line_colors(colors, JO_TV_HEIGHT));
        JT_EQ(T_REG(T_LCTAU) >> 15, 1);
        // (the 2 tables: the second call takes the other one)
        JT_TRUE(jo_vdp2_set_line_colors(colors, 1));
        JT_EQ(T_REG(T_LCTAU) >> 15, 1);
        JT_TRUE(jo_vdp2_set_line_color(JO_VDP2_LINE_COLOR(1, 5)));
        JT_EQ(T_REG(T_LCTAU) >> 15, 0);
    }
    jo_vdp2_enable_line_color(JO_RBG0_SCREEN | JO_NBG1_SCREEN | JO_SPRITE_SCREEN);
    JT_EQ(T_REG(T_LNCLEN) & 0x3F, 0x32);
    jo_vdp2_disable_line_color(JO_NBG1_SCREEN | JO_SPRITE_SCREEN);
    JT_EQ(T_REG(T_LNCLEN) & 0x3F, 0x10);
    jo_vdp2_disable_line_color(JO_RBG0_SCREEN);
    JT_EQ(T_REG(T_LNCLEN) & 0x3F, 0);
    jo_vdp2_enable_blend(JO_LNCL_SCREEN, 9);
    JT_EQ(T_REG(T_CCRLB) & 31, 9);
    JT_EQ((T_REG(T_CCRLB) >> 8) & 31, 9);
    jo_vdp2_enable_blend(JO_LNCL_SCREEN, (unsigned char)(t_base[(T_CCRLB - 0x25F80000) / 2] & 31));
    jo_vdp2_disable_blend(JO_LNCL_SCREEN);
    JT_EQ((T_REG(T_CCCTL) >> 5) & 1, 0);
}

/*
** 3D: rotations and perspective in jo_angle (fractions of degree)
*/

static bool                     t_same_matrix(const jo_fixed * const a, const jo_fixed * const b)
{
    int                         i;

    for (i = 0; i < 12; ++i)
        if (a[i] != b[i])
            return (false);
    return (true);
}

static void                     t_3d_angles(void)
{
    jo_fixed                    a[12];
    jo_fixed                    b[12];
    jo_fixed                    distance;

    jt_group("3d.angles");
    jo_3d_push_matrix();
    jo_3d_rotate_matrix(30, -45, 100);
    jo_3d_get_matrix(a);
    jo_3d_pop_matrix();
    jo_3d_push_matrix();
    jo_3d_rotate_matrix_angle(jo_DEGtoANG_int(30), jo_DEGtoANG_int(-45), jo_DEGtoANG_int(100));
    jo_3d_get_matrix(b);
    jo_3d_pop_matrix();
    JT_TRUE(t_same_matrix(a, b));
    jo_3d_push_matrix();
    jo_3d_rotate_matrix_y(-45);
    jo_3d_get_matrix(a);
    jo_3d_pop_matrix();
    jo_3d_push_matrix();
    jo_3d_rotate_matrix_angle_y(jo_DEGtoANG_int(-45));
    jo_3d_get_matrix(b);
    jo_3d_pop_matrix();
    JT_TRUE(t_same_matrix(a, b));
    // (half a degree more: another matrix)
    jo_3d_push_matrix();
    jo_3d_rotate_matrix_angle_y((jo_angle)(jo_DEGtoANG_int(-45) + JO_DEG_TO_ANGLE(0.5)));
    jo_3d_get_matrix(b);
    jo_3d_pop_matrix();
    JT_TRUE(!t_same_matrix(a, b));
    jo_3d_perspective_angle(60);
    distance = jo_3d_get_projection_distance();
    jo_3d_set_perspective(jo_DEGtoANG_int(60));
    JT_EQ(jo_3d_get_projection_distance(), distance);
    jo_3d_set_perspective(JO_DEG_TO_ANGLE(60.5));
    JT_TRUE(jo_3d_get_projection_distance() < distance);
    jo_3d_perspective_angle(JO_3D_DEFAULT_PERSPECTIVE_ANGLE);
}

/*
** Palettes
*/

static void                     t_palettes(void)
{
    static const jo_color       colors[3] = {JO_COLOR_RGB(255, 0, 0), JO_COLOR_RGB(0, 255, 128), JO_COLOR_RGB(128, 128, 255)};
    jo_palette                  palette;
    unsigned short              cram[64];
    int                         i;
    bool                        darker;

    jt_group("palette");
    jo_create_palette(&palette);
    JT_TRUE(palette.data != JO_NULL);
    jo_palette_set_light_shades(&palette, 1, colors, 3, 8);
    // Shade 0 is the darkest, the shades get lighter
    for (darker = true, i = 1; i < 8; ++i)
        darker = darker && JO_COLOR_SATURN_GET_R(palette.data[i * 3]) >= JO_COLOR_SATURN_GET_R(palette.data[(i - 1) * 3]);
    JT_TRUE(darker);
    JT_EQ(JO_PALETTE_LIGHT_COLOR(palette.id, 1, 2), palette.id * 256 + 3);
    for (i = 0; i < 64; ++i)
        cram[i] = palette.data[i];
    jt_value("palette.light_shades", jt_hash(JT_HASH_INIT, cram, 2 * 24));
    jo_set_palette_register(1, JO_COLOR_RGB(8, 16, 24));
    JT_EQ(jo_get_palette_register(1), JO_COLOR_RGB(8, 16, 24));
}

/*
** Sprites
*/

static void                     t_sprites(void)
{
    static unsigned short       pixels[16 * 8];
    jo_img                      img;
    int                         first;
    int                         second;
    int                         usage;
    int                         i;

    jt_group("sprites");
    for (i = 0; i < 16 * 8; ++i)
        pixels[i] = JO_COLOR_RGB(i * 2, 255 - i, 128);
    img.width = 16;
    img.height = 8;
    img.data = pixels;
    usage = jo_sprite_usage_percent();
    first = jo_sprite_add(&img);
    second = jo_sprite_add(&img);
    JT_TRUE(first >= 0);
    JT_EQ(second, first + 1);
    JT_EQ(jo_sprite_get_width(first), 16);
    JT_EQ(jo_sprite_get_height(first), 8);
    JT_EQ(jo_get_last_sprite_id(), second);
    jo_sprite_free_from(first);
    JT_EQ(jo_get_last_sprite_id(), first - 1);
    JT_EQ(jo_sprite_usage_percent(), usage);
}

/*
** Colored shapes: jo_sprite_draw_rectangle() and jo_sprite_draw_color_4p_fixed() (the VDP1 commands of the frame)
*/

/** @brief The commands written by a call (the next commands of the frame, see __jo_vdp1_reserve_commands())
 *  @param keys Output: their sort keys
 *  @return The first one
 */
static const jo_vdp1_command    *t_shape_next(unsigned short ** const keys)
{
    unsigned int                reserved;

    return (__jo_vdp1_reserve_commands(0, keys, &reserved));
}

static void                     t_sprite_shapes(void)
{
    // (low on the screen: the barcode of the reports is at its top; drawn during the frame of the checks only)
    static const jo_pos2D_fixed quad[4] = {{JO_MULT_BY_65536(40), JO_MULT_BY_65536(150)}, {JO_MULT_BY_65536(70), JO_MULT_BY_65536(152)},
                                           {JO_MULT_BY_65536(66), JO_MULT_BY_65536(180)}, {JO_MULT_BY_65536(44), JO_MULT_BY_65536(178)}};
    const jo_vdp1_command       *cmd;
    unsigned short              *keys;
    jo_pos2D_fixed              centered[4];
    int                         before;
    int                         cx;
    int                         cy;
    int                         i;

    jt_group("sprites.shapes");
    // (the center of the default window: the center of the screen, see jo_sprite_draw3D())
    cx = __jo_vdp1_windows[__jo_vdp1_current_window].center_x - __jo_vdp1_windows[__jo_vdp1_current_window].origin_x;
    cy = __jo_vdp1_windows[__jo_vdp1_current_window].center_y - __jo_vdp1_windows[__jo_vdp1_current_window].origin_y;
    JT_EQ(cx, jo_core_get_screen_width() / 2);
    JT_EQ(cy, jo_core_get_screen_height() / 2);
    // A rectangle: one polygon, its corners the first and the last pixel (VDP1 coordinates are inclusive)
    before = __jo_vdp1_frame_command_count();
    cmd = t_shape_next(&keys);
    jo_sprite_draw_rectangle(10, 160, 30, 5, JO_COLOR_RGB(255, 0, 0), 500);
    JT_EQ(__jo_vdp1_frame_command_count() - before, 1);
    JT_EQ(cmd->ctrl, JO_VDP1_CMD_POLYGON);
    JT_EQ(cmd->link, 0);
    // (on the screen: the pre-clipping off, see JO_VDP1_PRE_CLIPPING_DISABLE)
    JT_EQ(cmd->pmod, JO_VDP1_END_CODE_DISABLE | JO_VDP1_TRANSPARENT_PIXEL_DISABLE | JO_VDP1_COLOR_MODE_RGB | JO_VDP1_PRE_CLIPPING_DISABLE);
    JT_EQ(cmd->colr, JO_COLOR_RGB(255, 0, 0));
    JT_EQ(cmd->grda, 0);
    JT_EQ(cmd->xa, 10);
    JT_EQ(cmd->ya, 160);
    JT_EQ(cmd->xb, 39);
    JT_EQ(cmd->yb, 160);
    JT_EQ(cmd->xc, 39);
    JT_EQ(cmd->yc, 164);
    JT_EQ(cmd->xd, 10);
    JT_EQ(cmd->yd, 164);
    JT_EQ(keys[0], 500);
    // One pixel; nothing for an empty size
    cmd = t_shape_next(&keys);
    jo_sprite_draw_rectangle(5, 170, 1, 1, JO_COLOR_White, 500);
    JT_TRUE(cmd->xa == 5 && cmd->ya == 170 && cmd->xc == 5 && cmd->yc == 170);
    before = __jo_vdp1_frame_command_count();
    jo_sprite_draw_rectangle(5, 170, 0, 10, JO_COLOR_White, 500);
    jo_sprite_draw_rectangle(5, 170, 10, -1, JO_COLOR_White, 500);
    // Like the sprites: not drawn nearer than the near clipping distance
    jo_sprite_draw_rectangle(5, 170, 10, 10, JO_COLOR_White, JO_FIXED_TO_INT(__jo_vdp1_near_clip()));
    JT_EQ(__jo_vdp1_frame_command_count() - before, 0);
    // The effects of the sprites: half transparency, gouraud shading (its table), clipping; the depth bias
    cmd = t_shape_next(&keys);
    jo_sprite_enable_half_transparency();
    jo_sprite_enable_clipping(false);
    jo_3d_set_depth_bias(-100);
    jo_sprite_draw_rectangle(10, 160, 30, 5, JO_COLOR_Blue, 300);
    jo_3d_set_depth_bias(0);
    jo_sprite_disable_clipping();
    jo_sprite_disable_half_transparency();
    JT_EQ(cmd->pmod, JO_VDP1_END_CODE_DISABLE | JO_VDP1_TRANSPARENT_PIXEL_DISABLE | JO_VDP1_COLOR_MODE_RGB |
                     JO_VDP1_COLOR_CALC_HALF_TRANSPARENT | JO_VDP1_WINDOW_IN);
    JT_EQ(keys[0], 200);
    cmd = t_shape_next(&keys);
    jo_set_gouraud_shading_colors(JO_COLOR_Red, JO_COLOR_Green, JO_COLOR_Blue, JO_COLOR_White);
    jo_sprite_enable_gouraud_shading();
    jo_sprite_draw_rectangle(10, 160, 30, 5, JO_COLOR_Gray, 500);
    jo_sprite_disable_gouraud_shading();
    JT_EQ(cmd->pmod & JO_VDP1_COLOR_CALC_MASK, JO_VDP1_COLOR_CALC_GOURAUD);
    JT_TRUE(cmd->grda != 0);
    JT_EQ(*(volatile unsigned short *)(JO_VDP1_VRAM + JO_MULT_BY_8(cmd->grda)), JO_COLOR_Red);
    JT_EQ(*(volatile unsigned short *)(JO_VDP1_VRAM + JO_MULT_BY_8(cmd->grda) + 6), JO_COLOR_White);
    // The state of the sprites is back: a plain rectangle again
    cmd = t_shape_next(&keys);
    jo_sprite_draw_rectangle(10, 160, 30, 5, JO_COLOR_Red, 500);
    // (on the screen: the pre-clipping off, see JO_VDP1_PRE_CLIPPING_DISABLE)
    JT_EQ(cmd->pmod, JO_VDP1_END_CODE_DISABLE | JO_VDP1_TRANSPARENT_PIXEL_DISABLE | JO_VDP1_COLOR_MODE_RGB | JO_VDP1_PRE_CLIPPING_DISABLE);
    // A quad on 4 points from the top left of the screen, then the same from the center: the same command
    cmd = t_shape_next(&keys);
    jo_sprite_draw_color_4p_fixed(quad, JO_COLOR_Yellow, JO_MULT_BY_65536(400), false);
    JT_EQ(cmd->ctrl, JO_VDP1_CMD_POLYGON);
    JT_EQ(cmd->colr, JO_COLOR_Yellow);
    JT_TRUE(cmd->xa == 40 && cmd->ya == 150 && cmd->xb == 70 && cmd->yb == 152);
    JT_TRUE(cmd->xc == 66 && cmd->yc == 180 && cmd->xd == 44 && cmd->yd == 178);
    JT_EQ(keys[0], 400);
    for (i = 0; i < 4; ++i)
    {
        centered[i].x = quad[i].x - JO_MULT_BY_65536(cx);
        centered[i].y = quad[i].y - JO_MULT_BY_65536(cy);
    }
    cmd = t_shape_next(&keys);
    jo_sprite_draw_color_4p_fixed(centered, JO_COLOR_Yellow, JO_MULT_BY_65536(400), true);
    JT_TRUE(cmd->xa == 40 && cmd->ya == 150 && cmd->xb == 70 && cmd->yb == 152);
    JT_TRUE(cmd->xc == 66 && cmd->yc == 180 && cmd->xd == 44 && cmd->yd == 178);
}

/*
** VDP2 memory
*/

static void                     t_vdp2_malloc(void)
{
    void                        *a;
    void                        *b;

    jt_group("vdp2_malloc");
    a = jo_vdp2_malloc(JO_VDP2_RAM_USER, 1024);
    JT_TRUE(a != JO_NULL);
    jo_vdp2_free(a);
    a = jo_vdp2_malloc(JO_VDP2_RAM_CELL_NBG2, 4096);
    b = jo_vdp2_malloc(JO_VDP2_RAM_CELL_NBG2, 4096);
    JT_TRUE(a != JO_NULL && b != JO_NULL);
    JT_TRUE((unsigned char *)b >= (unsigned char *)a + 4096 || (unsigned char *)b + 4096 <= (unsigned char *)a);
    jo_vdp2_free(b);
    jo_vdp2_free(a);
    b = jo_vdp2_malloc(JO_VDP2_RAM_CELL_NBG2, 4096);
    JT_TRUE(b == a);
    jo_vdp2_free(b);
}

/*
** Files (cd/TEST.TXT)
*/

static void                     t_files(void)
{
    char                        *data;
    int                         len;
    int                         bad;
    int                         i;

    jt_group("fs");
    data = jo_fs_read_file("TEST.TXT", &len);
    JT_TRUE(data != JO_NULL);
    if (data == JO_NULL)
        return ;
    JT_EQ(len, 44);
    JT_EQ(jo_memcmp(data, "The quick brown fox jumps over the lazy dog\n", 44), 0);
    jt_value("fs.test_txt", jt_hash(JT_HASH_INIT, data, len));
    jo_free(data);
    // A file of a sub directory, its size not a multiple of the sector size (2048 bytes)
    data = jo_fs_read_file_in_dir("PATTERN.BIN", "DATA", &len);
    JT_TRUE(data != JO_NULL);
    if (data == JO_NULL)
        return ;
    JT_EQ(len, 5001);
    for (bad = 0, i = 0; i < 5001; ++i)
        if ((unsigned char)data[i] != (unsigned char)((i * 7 + (i >> 8)) & 255))
            ++bad;
    JT_EQ(bad, 0);
    jo_free(data);
    // The root directory again
    data = jo_fs_read_file("TEST.TXT", &len);
    JT_TRUE(data != JO_NULL && len == 44);
    if (data != JO_NULL)
        jo_free(data);
}

void                            t_state(void)
{
    t_screen_modes();
    t_vdp2_effects();
    t_3d_angles();
    t_palettes();
    t_sprites();
    t_sprite_shapes();
    t_vdp2_malloc();
    t_files();
}
