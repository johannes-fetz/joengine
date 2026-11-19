#include <jo/jo.h>

/* Un-comment this line to show the image on NBG1 instead of a texture of the visible cube */
//#define DRAW_ON_NBG1

#define IMAGE_WIDTH             (160)
#define IMAGE_HEIGHT            (112)
#define CUBE_SIZE               (32)
#define CUBE_FACE_COUNT         (6)

static jo_camera                camera;
static jo_vertice               cube_vertices[] = JO_3D_CUBE_VERTICES(CUBE_SIZE);
static jo_3d_mesh               *outside_cube;
static jo_3d_mesh               *inside_cube;
static jo_rot3D                 outside_rotation;
static jo_rot3D                 inside_rotation;

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
    jo_3d_render_to_image_begin();
    draw_cube(outside_cube, &outside_rotation);
    jo_3d_render_to_image_end();
    draw_cube(inside_cube, &inside_rotation);
#ifdef DRAW_ON_NBG1
    jo_vdp2_set_nbg1_image(jo_3d_render_to_image_get(), 0, 0);
#endif
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
    int                         image_sprite;

    jo_core_init(JO_COLOR_Black);
    jo_3d_camera_init(&camera);
    jo_3d_render_to_image_init(IMAGE_WIDTH, IMAGE_HEIGHT);
#ifdef DRAW_ON_NBG1
    image_sprite = jo_sprite_add_tga(JO_ROOT_DIR, "BOX.TGA", JO_COLOR_Transparent);
#else
    image_sprite = jo_3d_render_to_image_add_sprite();
#endif

    inside_cube = jo_3d_create_mesh_from_vertices(CUBE_FACE_COUNT, cube_vertices);
    outside_cube = jo_3d_create_mesh_from_vertices(CUBE_FACE_COUNT, cube_vertices);
    jo_3d_set_mesh_texture(inside_cube, image_sprite);
    jo_3d_set_mesh_texture(outside_cube, jo_sprite_add_tga(JO_ROOT_DIR, "BOX2.TGA", JO_COLOR_Transparent));

    jo_core_set_restart_game_callback(jo_goto_boot_menu);
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
