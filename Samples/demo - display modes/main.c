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
** Display modes: every resolution of the clock family (320/640 or 352/704 pixels, set JO_SCREEN_WIDTH = 352 in the
** makefile for the second one) and the VDP2 effects.
**
** The cube is lit by the palette lighting (the only lighting of the 640 and 704 pixels modes): 8 shades of its colors in
** its palette. Its first 3 faces use the shades at 192: in "sprite priority" they go behind the checker (640 and 704 pixels).
**
** Special colors: the yellow stripes (colors 2 and 3 of their palette) of NBG2 are over the NBG1 bitmap or transparent.
**
** A: next resolution    B: next effect    C: automatic tour on/off    L/R: blend ratio
*/

#include <jo/jo.h>

#ifndef DEMO_TOUR_SECONDS
# define DEMO_TOUR_SECONDS  (3)
#endif

typedef enum
{
    EFFECT_NONE,
    EFFECT_WINDOW,
    EFFECT_CIRCLE,
    EFFECT_BLEND,
    EFFECT_ADD,
    EFFECT_SHADOW,
    EFFECT_MOSAIC,
    EFFECT_EXTENDED,
    EFFECT_BLUR,
    EFFECT_SPECIAL_PRIORITY,
    EFFECT_SPECIAL_BLEND,
    EFFECT_NATIVE_BITMAP,
    EFFECT_SPRITE_PRIORITY,
    EFFECT_COUNT
}                       demo_effect;

static const char       *effect_names[EFFECT_COUNT] =
{
    "none", "window", "circle window", "blend (ratio)", "blend (add)", "shadow", "mosaic", "blend 3 screens", "blur",
    "special priority", "special blend", "8 bits bitmap", "sprite priority"
};

# define SHADES             (8)
# define SHADES_FIRST       (16)
# define SHADES_BEHIND      (192)

# define MODE_COUNT         (16)

static const short      modes[2][MODE_COUNT][3] =
{
    {{320, 224, 0}, {320, 240, 0}, {320, 240, 1}, {320, 448, 0}, {320, 480, 0}, {640, 224, 0}, {640, 240, 0}, {640, 240, 1},
     {640, 448, 0}, {640, 480, 0}, {320, 256, 0}, {320, 256, 1}, {320, 512, 0}, {640, 256, 0}, {640, 256, 1}, {640, 512, 0}},
    {{352, 224, 0}, {352, 240, 0}, {352, 240, 1}, {352, 448, 0}, {352, 480, 0}, {704, 224, 0}, {704, 240, 0}, {704, 240, 1},
     {704, 448, 0}, {704, 480, 0}, {352, 256, 0}, {352, 256, 1}, {352, 512, 0}, {704, 256, 0}, {704, 256, 1}, {704, 512, 0}}
};

static jo_camera        camera;
static jo_palette       palette;
static jo_palette       stripes_palette;
static jo_palette       bitmap_palette;
static bool             native_bitmap = false;
static jo_3d_mesh       *cube;
static jo_3d_mesh       *shadow;
static int              resolution;
static int              effect = EFFECT_NONE;
static int              blend_ratio = 16;
static bool             tour = true;
static int              tour_frames = 0;
static int              angle = 0;

static void             draw_background(void);
static void             draw_native_bitmap(void);

static void             set_effect(const int e)
{
    int                 w;
    int                 h;

    effect = e;
    w = jo_core_get_screen_width();
    h = jo_core_get_screen_height();
    jo_vdp2_disable_window(JO_ALL_SCROLL_SCREEN | JO_SPRITE_SCREEN);
    jo_vdp2_disable_blend(JO_ALL_SCROLL_SCREEN | JO_SPRITE_SCREEN);
    jo_vdp2_set_blend_mode(JO_VDP2_BLEND_RATIO);
    jo_vdp2_disable_shadow();
    jo_vdp2_disable_mosaic();
    jo_vdp2_disable_blur();
    jo_vdp2_disable_special_priority(JO_NBG2_SCREEN);
    jo_vdp2_disable_special_blend(JO_NBG2_SCREEN);
    jo_vdp2_set_hires_sprite_mode(JO_VDP2_HIRES_SPRITES_BLEND);
    jo_vdp2_set_sprite_priorities(6, 6, 6, 6);
    if (e == EFFECT_NATIVE_BITMAP)
        draw_native_bitmap();
    else if (native_bitmap)
        draw_background();
    jo_set_displayed_screens(e >= EFFECT_EXTENDED && e <= EFFECT_SPECIAL_BLEND && e != EFFECT_BLUR ?
                             JO_NBG0_SCREEN | JO_NBG1_SCREEN | JO_NBG2_SCREEN : JO_NBG0_SCREEN | JO_NBG1_SCREEN);
    switch (e)
    {
        case EFFECT_WINDOW:
            jo_vdp2_set_window(JO_VDP2_WINDOW_0, w / 8, h / 4, w * 3 / 4, h / 2);
            jo_vdp2_enable_window(JO_NBG1_SCREEN, JO_VDP2_WINDOW_0, JO_VDP2_WINDOW_SHOW_INSIDE);
            break;
        case EFFECT_CIRCLE:
            jo_vdp2_set_window_circle(JO_VDP2_WINDOW_1, w / 2, h / 2, h / 3);
            jo_vdp2_enable_window(JO_NBG1_SCREEN | JO_SPRITE_SCREEN, JO_VDP2_WINDOW_1, JO_VDP2_WINDOW_SHOW_OUTSIDE);
            break;
        case EFFECT_BLEND:
            jo_vdp2_enable_blend(JO_SPRITE_SCREEN, blend_ratio);
            break;
        case EFFECT_ADD:
            jo_vdp2_set_blend_mode(JO_VDP2_BLEND_ADD);
            jo_vdp2_enable_blend(JO_SPRITE_SCREEN, 0);
            break;
        case EFFECT_SHADOW:
            jo_vdp2_enable_shadow(JO_NBG1_SCREEN | JO_BACK_SCREEN);
            break;
        case EFFECT_MOSAIC:
            jo_vdp2_enable_mosaic(JO_NBG1_SCREEN, 8, 8);
            break;
        case EFFECT_EXTENDED:
            jo_vdp2_set_blend_mode(JO_VDP2_BLEND_RATIO | JO_VDP2_BLEND_EXTENDED);
            jo_vdp2_enable_blend(JO_SPRITE_SCREEN | JO_NBG2_SCREEN, blend_ratio);
            break;
        case EFFECT_BLUR:
            jo_vdp2_enable_blend(JO_SPRITE_SCREEN, blend_ratio);
            jo_vdp2_enable_blur(JO_NBG1_SCREEN);
            break;
        case EFFECT_SPECIAL_PRIORITY:
            jo_vdp2_enable_special_priority(JO_NBG2_SCREEN);
            break;
        case EFFECT_SPECIAL_BLEND:
            jo_vdp2_enable_blend(JO_NBG2_SCREEN, blend_ratio);
            jo_vdp2_enable_special_blend(JO_NBG2_SCREEN);
            break;
        case EFFECT_SPRITE_PRIORITY:
            jo_vdp2_set_hires_sprite_mode(JO_VDP2_HIRES_SPRITES_PRIORITY);
            jo_vdp2_set_sprite_priorities(6, 6, 6, 3);
            break;
    }
}

static void             set_resolution(int index)
{
    const short         (*family)[3];

    family = modes[JO_TV_WIDTH == 352 || JO_TV_WIDTH == 704];
    for (index %= MODE_COUNT; !jo_core_set_screen_resolution(family[index][0], family[index][1]); index = (index + 1) % MODE_COUNT)
        ;
    jo_core_set_screen_single_interlace(family[index][2]);
    resolution = index;
    jo_clear_screen();
    set_effect(effect);
}

static void             draw_cube(void)
{
    jo_3d_camera_look_at(&camera);
    jo_3d_light(0, 0, 1);
    jo_3d_push_matrix();
    {
        jo_3d_rotate_matrix(angle, angle * 2, 0);
        jo_3d_mesh_draw(cube);
    }
    jo_3d_pop_matrix();
    jo_3d_push_matrix();
    {
        jo_3d_translate_matrix(70, 40, 0);
        jo_3d_mesh_draw(shadow);
    }
    jo_3d_pop_matrix();
}

static void             read_inputs(void)
{
    if (jo_is_pad1_key_down(JO_KEY_A))
        set_resolution(resolution + 1);
    if (jo_is_pad1_key_down(JO_KEY_B))
        set_effect((effect + 1) % EFFECT_COUNT);
    if (jo_is_pad1_key_down(JO_KEY_C))
        tour = !tour;
    if (jo_is_pad1_key_down(JO_KEY_L) && blend_ratio > 0)
    {
        --blend_ratio;
        set_effect(effect);
    }
    if (jo_is_pad1_key_down(JO_KEY_R) && blend_ratio < JO_VDP2_BLEND_RATIO_MAX)
    {
        ++blend_ratio;
        set_effect(effect);
    }
}

static void             demo_loop(void)
{
    read_inputs();
    if (tour && ++tour_frames >= DEMO_TOUR_SECONDS * jo_core_get_refresh_rate())
    {
        tour_frames = 0;
        if (effect + 1 < EFFECT_COUNT)
            set_effect(effect + 1);
        else
        {
            set_effect(EFFECT_NONE);
            set_resolution(resolution + 1);
        }
    }
    angle += 2;
    draw_cube();
    jo_printf(1, 1, "%dx%d %s      ", jo_core_get_screen_width(), jo_core_get_screen_height(),
              jo_core_get_screen_height() > 256 ? "double interlace" :
              modes[JO_TV_WIDTH == 352 || JO_TV_WIDTH == 704][resolution][2] ? "single interlace" : "progressive");
    jo_printf(1, 2, "effect: %s                ", effect_names[effect]);
    jo_printf(1, 3, "blend ratio %d  tour %s  ", blend_ratio, tour ? "on " : "off");
    jo_printf(1, 26, "A:mode B:effect C:tour L/R");
}

static jo_3d_mesh       *create_cube(const int size)
{
    static const signed char    faces[6][4][3] =
    {
        {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}},
        {{1, -1, 1}, {-1, -1, 1}, {-1, 1, 1}, {1, 1, 1}},
        {{-1, -1, 1}, {-1, -1, -1}, {-1, 1, -1}, {-1, 1, 1}},
        {{1, -1, -1}, {1, -1, 1}, {1, 1, 1}, {1, 1, -1}},
        {{-1, -1, 1}, {1, -1, 1}, {1, -1, -1}, {-1, -1, -1}},
        {{-1, 1, -1}, {1, 1, -1}, {1, 1, 1}, {-1, 1, 1}}
    };
    jo_3d_mesh          *mesh;
    int                 f;
    int                 v;

    mesh = jo_3d_create_mesh(6);
    for (JO_ZERO(f); f < 6; ++f)
    {
        for (JO_ZERO(v); v < 4; ++v)
            jo_3d_set_mesh_vertice(mesh, jo_int2fixed(faces[f][v][0] * size), jo_int2fixed(faces[f][v][1] * size),
                                   jo_int2fixed(faces[f][v][2] * size), f * 4 + v);
        jo_3d_set_mesh_polygon_color(mesh, JO_PALETTE_LIGHT_COLOR(palette.id, f < 3 ? SHADES_BEHIND : SHADES_FIRST, f), f);
    }
    jo_3d_set_mesh_light(mesh, true);
    return (mesh);
}

static void             create_stripes(void)
{
    static unsigned char    pixels[32 * 32];
    static jo_img_8bits     img = {32, 32, pixels};
    static const jo_color   colors[7] =
    {
        JO_COLOR_MediumRed, JO_COLOR_Yellow, JO_COLOR_Yellow, JO_COLOR_MediumGreen, JO_COLOR_MediumCyan,
        JO_COLOR_MediumPurple, JO_COLOR_White
    };
    int                     x;
    int                     y;

    jo_create_palette(&stripes_palette);
    for (JO_ZERO(x); x < 7; ++x)
        stripes_palette.data[x] = colors[x];
    for (JO_ZERO(y); y < 32; ++y)
        for (JO_ZERO(x); x < 32; ++x)
            pixels[y * 32 + x] = (unsigned char)(((x + y) >> 2) & 7);
    jo_vdp2_set_nbg2_8bits_image(&img, stripes_palette.id, false, false);
    jo_vdp2_set_special_colors(JO_VDP2_SPECIAL_COLOR(2));
    jo_core_set_screens_order(JO_NBG0_SCREEN, JO_SPRITE_SCREEN, JO_NBG2_SCREEN, JO_NBG1_SCREEN);
}

static void             draw_background(void)
{
    int                 x;
    int                 y;

    native_bitmap = false;
    jo_vdp2_set_nbg1_16bits_bitmap();
    for (JO_ZERO(y); y < JO_VDP2_HEIGHT; ++y)
        for (JO_ZERO(x); x < JO_VDP2_WIDTH - 1; ++x)
            jo_vdp2_put_pixel_bitmap_nbg1(x, y, ((x ^ y) & 32) ? JO_COLOR_DarkBlue : JO_COLOR_Gray);
}

static void             draw_native_bitmap(void)
{
    static unsigned char    line[1024];
    static jo_img_8bits     img = {1024, 1, line};
    int                     x;
    int                     y;

    if (!jo_vdp2_set_nbg1_8bits_bitmap(bitmap_palette.id))
        return;
    native_bitmap = true;
    img.width = (short)jo_vdp2_get_nbg1_8bits_bitmap_width();
    for (JO_ZERO(y); y < jo_vdp2_get_nbg1_8bits_bitmap_height() - 1; ++y)
    {
        for (JO_ZERO(x); x < img.width; ++x)
            line[x] = (unsigned char)((x & 63) == 0 || (y & 63) == 0 ? 3 : (((x >> 1) ^ (y >> 1)) & 1) + 1);
        jo_vdp2_draw_bitmap_nbg1_8bits_image(&img, 0, y);
    }
}

void                    jo_main(void)
{
    static const jo_color   colors[6] =
    {
        JO_COLOR_Red, JO_COLOR_Green, JO_COLOR_Yellow, JO_COLOR_Cyan, JO_COLOR_Purple, JO_COLOR_White
    };

    jo_core_init(JO_COLOR_Black);
    for (resolution = 0; modes[JO_TV_WIDTH == 352 || JO_TV_WIDTH == 704][resolution][0] != JO_TV_WIDTH ||
                         modes[JO_TV_WIDTH == 352 || JO_TV_WIDTH == 704][resolution][1] != JO_TV_HEIGHT; ++resolution)
        ;
    jo_create_palette(&palette);
    jo_palette_set_light_shades(&palette, SHADES_FIRST, colors, 6, SHADES);
    jo_palette_set_light_shades(&palette, SHADES_BEHIND, colors, 6, SHADES);
    jo_3d_set_palette_light(SHADES, 6);
    jo_core_set_screen_palette(palette.id);
    jo_create_palette(&bitmap_palette);
    bitmap_palette.data[0] = JO_COLOR_DarkBlue;
    bitmap_palette.data[1] = JO_COLOR_Gray;
    bitmap_palette.data[2] = JO_COLOR_White;
    jo_3d_camera_init(&camera);
    cube = create_cube(40);
    shadow = jo_3d_create_mesh(1);
    jo_3d_set_mesh_vertice(shadow, jo_int2fixed(-50), jo_int2fixed(-30), 0, 0);
    jo_3d_set_mesh_vertice(shadow, jo_int2fixed(50), jo_int2fixed(-30), 0, 1);
    jo_3d_set_mesh_vertice(shadow, jo_int2fixed(50), jo_int2fixed(30), 0, 2);
    jo_3d_set_mesh_vertice(shadow, jo_int2fixed(-50), jo_int2fixed(30), 0, 3);
    jo_3d_set_mesh_polygon_color(shadow, JO_VDP2_SHADOW_COLOR, 0);
    draw_background();
    create_stripes();
    jo_core_add_callback(demo_loop);
    jo_core_run();
}
