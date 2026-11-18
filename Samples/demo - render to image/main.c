#include <jo/jo.h>

/* Un-comment this line to show the image on NBG1 instead of a texture of the visible cube */
//#define DRAW_ON_NBG1

/* The image: a part of the VDP1 frame buffer beyond the screen (x 352 to 511), drawn by the far 3D window */
#define IMAGE_WIDTH             (160)
#define IMAGE_HEIGHT            (112)
#define IMAGE_LEFT              (JO_VDP1_FB_WIDTH - IMAGE_WIDTH)
#define IMAGE_BYTES_PER_LINE    (IMAGE_WIDTH * sizeof(jo_color))
#define FB_BYTES_PER_LINE       (JO_VDP1_FB_WIDTH * sizeof(jo_color))

#define CUBE_SIZE               (32)
#define CUBE_FACE_COUNT         (6)
#define Z_LIMIT                 (200)

#define IMAGE_HALF_WIDTH        (IMAGE_WIDTH / 2 * JO_FIXED_1)
#define IMAGE_HALF_HEIGHT       (IMAGE_HEIGHT / 2 * JO_FIXED_1)

#define IMAGE_SPRITE_ID         (0)
#define BOX_SPRITE_ID           (1)

static jo_img                   image;
static jo_camera                camera;
static jo_vertice               cube_vertices[] = JO_3D_CUBE_VERTICES(CUBE_SIZE);
static jo_3d_mesh               *outside_cube;
static jo_3d_mesh               *inside_cube;
static jo_rot3D                 outside_rotation;
static jo_rot3D                 inside_rotation;

/** @brief Copy the image drawn by the VDP1 beyond the screen into the texture (or NBG1), at each VBLANK */
static void                     copy_image(void)
{
    int                         y;

    /* Waits for the end of the drawing of the VDP1: a frame longer than one VBLANK reads the frame drawn before */
    while (!JO_VDP1_EDSR)
        ;
    for (y = 0; y < IMAGE_HEIGHT; ++y)
        jo_dma_copy((const char *)JO_VDP1_FB + y * FB_BYTES_PER_LINE + IMAGE_LEFT * sizeof(jo_color),
                    image.data + y * IMAGE_WIDTH, IMAGE_BYTES_PER_LINE);
#ifdef DRAW_ON_NBG1
    jo_vdp2_set_nbg1_image(&image, 0, 0);
#else
    jo_sprite_replace(&image, IMAGE_SPRITE_ID);
#endif
}

/** @brief Clear the image (the background of the far window): the VBLANK erase clears only the screen */
static void                     clear_image(void)
{
    static const jo_pos2D_fixed corners[] =
    {
        { -IMAGE_HALF_WIDTH, -IMAGE_HALF_HEIGHT },
        { IMAGE_HALF_WIDTH, -IMAGE_HALF_HEIGHT },
        { IMAGE_HALF_WIDTH, IMAGE_HALF_HEIGHT },
        { -IMAGE_HALF_WIDTH, IMAGE_HALF_HEIGHT }
    };

    jo_sprite_draw_color_4p_fixed(corners, JO_COLOR_Transparent, jo_int2fixed(Z_LIMIT), true);
}

static void                     draw_cube(jo_3d_mesh * const cube, const jo_rot3D * const rotation)
{
    jo_3d_push_matrix();
    {
        jo_3d_rotate_matrix(rotation->rx, rotation->ry, rotation->rz);
        jo_3d_mesh_draw(cube);
    }
    jo_3d_pop_matrix();
}

static void                     demo_draw(void)
{
    jo_3d_camera_look_at(&camera);

    jo_3d_select_window(JO_3D_FAR_WINDOW);
    jo_3d_window(IMAGE_LEFT, 0, JO_VDP1_FB_WIDTH - 1, IMAGE_HEIGHT - 1, Z_LIMIT,
                 IMAGE_LEFT + IMAGE_WIDTH / 2, IMAGE_HEIGHT / 2);
    jo_3d_window_clip(true);
    clear_image();
    draw_cube(outside_cube, &outside_rotation);

    jo_3d_select_window(JO_3D_NEAR_WINDOW);
    draw_cube(inside_cube, &inside_rotation);
}

static void                     demo_logic(void)
{
    ++outside_rotation.rx;
    --outside_rotation.ry;
    ++outside_rotation.rz;
    inside_rotation.rx -= 2;
    ++inside_rotation.ry;
    inside_rotation.rz += 2;
}

void                            jo_main(void)
{
    jo_core_init(JO_COLOR_Black);
    /* The VDP1 draws beyond the screen too: the whole width of the frame buffer */
    jo_sprite_set_system_clipping_area(JO_VDP1_FB_WIDTH, JO_TV_HEIGHT);

    image.width = IMAGE_WIDTH;
    image.height = IMAGE_HEIGHT;
    image.data = jo_malloc(IMAGE_WIDTH * IMAGE_HEIGHT * sizeof(jo_color));
    jo_3d_camera_init(&camera);

#ifdef DRAW_ON_NBG1
    jo_sprite_add_tga(JO_ROOT_DIR, "BOX.TGA", JO_COLOR_Transparent);
#else
    jo_sprite_add(&image);
#endif
    jo_sprite_add_tga(JO_ROOT_DIR, "BOX2.TGA", JO_COLOR_Transparent);

    inside_cube = jo_3d_create_mesh_from_vertices(CUBE_FACE_COUNT, cube_vertices);
    outside_cube = jo_3d_create_mesh_from_vertices(CUBE_FACE_COUNT, cube_vertices);
    jo_3d_set_mesh_texture(inside_cube, IMAGE_SPRITE_ID);
    jo_3d_set_mesh_texture(outside_cube, BOX_SPRITE_ID);

    jo_core_set_restart_game_callback(jo_goto_boot_menu);
    jo_core_add_vblank_callback(copy_image);
    jo_core_add_callback(demo_draw);
    jo_core_add_callback(demo_logic);

    jo_printf(9, 1, "demo - render to image");
    jo_printf(1, 3, "This demo shows VDP1 rendering");
    jo_printf(1, 4, "a rotating cube offscreen and then");
    jo_printf(1, 5, "putting the rendered image as a");
    jo_printf(1, 6, "texture on another rotating cube.");
    jo_set_printf_color_index(JO_COLOR_INDEX_Red);
    jo_printf(1, 22, "Demo made by Reye");
    jo_set_printf_color_index(JO_COLOR_INDEX_Yellow);
    jo_printf(1, 24, "Special thanks to XL2, mrkotfw");
    jo_printf(1, 25, "and hitomi2500 for help.");
    jo_set_printf_color_index(JO_COLOR_INDEX_White);

    jo_core_run();
}
