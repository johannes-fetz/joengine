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
** The 3D pipeline: the time per polygon of jo_3d_mesh_draw() with a grid of quads (colored, textured, textured with
** mipmaps, seen from the back, outside of the screen, cut by the near plane), with small meshes, and the vertices alone;
** by the CPU and with the SCU DSP (jo_3d_enable_dsp()). One test per frame: the VDP1 commands of a frame are limited
*/

#include <jo/jo.h>
#include "benchmark.h"

/** @brief The grid: 16 x 16 quads of 10 units */
#define GRID_SIZE                   (16)
#define SMALL_GRID_SIZE             (4)
#define QUAD_SIZE                   (10)
/** @brief Small meshes drawn in one test */
#define SMALL_MESH_COUNT            (16)
/** @brief The mesh of points: 504 vertices (24 columns, 8 units apart) and one polygon: the transformation alone */
#define POINT_COUNT                 (504)
#define POINT_COLUMNS               (24)
#define POINT_SPACING               (8)
/** @brief The texture: 32 x 32 pixels, a checkerboard */
#define TEXTURE_SIZE                (32)
/** @brief Where the meshes are drawn: in front of the camera, outside of the screen (to the right), cut by the near plane */
#define DEPTH                       (400)
#define NEAR_DEPTH                  (40)
#define OFFSCREEN_X                 (3000)
/** @brief The mesh turned to show its back faces (degrees) */
#define BACK_ANGLE                  (180)

static jo_3d_mesh               *mesh_color;
static jo_3d_mesh               *mesh_texture;
static jo_3d_mesh               *mesh_mipmap;
static jo_3d_mesh               *mesh_small;
static jo_3d_mesh               *mesh_points;

/** @brief A grid of quads facing the camera */
static jo_3d_mesh               *make_grid(const int size)
{
    jo_3d_mesh                  *mesh;
    int                         i;
    int                         j;
    int                         k;

    mesh = jo_3d_create_indexed_mesh((size + 1) * (size + 1), size * size);
    for (j = 0; j <= size; ++j)
        for (i = 0; i <= size; ++i)
        {
            k = j * (size + 1) + i;
            mesh->data.points[k].x = jo_int2fixed((i - size / 2) * QUAD_SIZE);
            mesh->data.points[k].y = jo_int2fixed((j - size / 2) * QUAD_SIZE);
            mesh->data.points[k].z = 0;
        }
    for (j = 0; j < size; ++j)
        for (i = 0; i < size; ++i)
        {
            k = j * size + i;
            mesh->data.polygons[k].vertices[0] = j * (size + 1) + i;
            mesh->data.polygons[k].vertices[1] = j * (size + 1) + i + 1;
            mesh->data.polygons[k].vertices[2] = (j + 1) * (size + 1) + i + 1;
            mesh->data.polygons[k].vertices[3] = (j + 1) * (size + 1) + i;
            mesh->data.polygons[k].normal.z = -JO_FIXED_1;
        }
    return (mesh);
}

/** @brief A mesh of points and one polygon: only the transformation of the vertices costs */
static jo_3d_mesh               *make_points(void)
{
    jo_3d_mesh                  *mesh;
    int                         k;

    mesh = jo_3d_create_indexed_mesh(POINT_COUNT, 1);
    for (k = 0; k < POINT_COUNT; ++k)
    {
        mesh->data.points[k].x = jo_int2fixed((k % POINT_COLUMNS) * POINT_SPACING - POINT_COLUMNS * POINT_SPACING / 2);
        mesh->data.points[k].y = jo_int2fixed((k / POINT_COLUMNS) * POINT_SPACING - (POINT_COUNT / POINT_COLUMNS) * POINT_SPACING / 2);
        mesh->data.points[k].z = 0;
    }
    return (mesh);
}

/** @brief A red and yellow checkerboard */
static int                      make_texture(void)
{
    static jo_color             pixels[TEXTURE_SIZE * TEXTURE_SIZE];
    jo_img                      image;
    int                         i;

    for (i = 0; i < TEXTURE_SIZE * TEXTURE_SIZE; ++i)
        pixels[i] = ((i ^ (i / TEXTURE_SIZE)) & 4) ? JO_COLOR_Red : JO_COLOR_Yellow;
    image.width = TEXTURE_SIZE;
    image.height = TEXTURE_SIZE;
    image.data = pixels;
    return (jo_sprite_add(&image));
}

void                            polygon_benchmarks_init(void)
{
    int                         texture;
    int                         texture_mipmap;

    texture = make_texture();
    texture_mipmap = make_texture();
    jo_3d_set_texture_mipmaps(texture_mipmap);
    mesh_color = make_grid(GRID_SIZE);
    jo_3d_set_mesh_color(mesh_color, JO_COLOR_Green);
    mesh_texture = make_grid(GRID_SIZE);
    jo_3d_set_mesh_texture(mesh_texture, texture);
    mesh_mipmap = make_grid(GRID_SIZE);
    jo_3d_set_mesh_texture(mesh_mipmap, texture_mipmap);
    mesh_small = make_grid(SMALL_GRID_SIZE);
    jo_3d_set_mesh_color(mesh_small, JO_COLOR_Green);
    mesh_points = make_points();
    jo_3d_set_mesh_color(mesh_points, JO_COLOR_Green);
}

/** @brief Draw a mesh at a place and measure it
 *  @param items Polygons (or vertices) of the mesh, for the result
 */
static void                     draw_mesh(jo_3d_mesh * const mesh, const char * const name, const int x, const int z, const int angle_y,
                                          const unsigned int items)
{
    unsigned int                t0;

    jo_3d_push_matrix();
    {
        jo_3d_translate_matrix(x, 0, z);
        if (angle_y != 0)
            jo_3d_rotate_matrix_y(angle_y);
        t0 = jo_profile_ticks();
        jo_3d_mesh_draw(mesh);
        benchmark_add(name, (jo_profile_ticks() - t0) & 0xFFFF, items);
    }
    jo_3d_pop_matrix();
}

/** @brief Draw the small mesh several times at a place and measure it */
static void                     draw_small_meshes(const char * const name, const int x)
{
    unsigned int                t0;
    int                         k;

    jo_3d_push_matrix();
    {
        jo_3d_translate_matrix(x, 0, DEPTH);
        t0 = jo_profile_ticks();
        for (k = 0; k < SMALL_MESH_COUNT; ++k)
            jo_3d_mesh_draw(mesh_small);
        benchmark_add(name, (jo_profile_ticks() - t0) & 0xFFFF, SMALL_MESH_COUNT * SMALL_GRID_SIZE * SMALL_GRID_SIZE);
    }
    jo_3d_pop_matrix();
}

bool                            polygon_benchmark_step(const int step)
{
    const unsigned int          grid_polygons = GRID_SIZE * GRID_SIZE;

    switch (step)
    {
        // The CPU
        case 0: draw_mesh(mesh_color, "poly color", 0, DEPTH, 0, grid_polygons); break;
        case 1: draw_mesh(mesh_texture, "poly tex", 0, DEPTH, 0, grid_polygons); break;
        case 2: draw_mesh(mesh_mipmap, "poly mipmap", 0, DEPTH, 0, grid_polygons); break;
        case 3: draw_mesh(mesh_texture, "poly back", 0, DEPTH, BACK_ANGLE, grid_polygons); break;
        case 4: draw_mesh(mesh_texture, "poly offscr", OFFSCREEN_X, DEPTH, 0, grid_polygons); break;
        case 5: draw_mesh(mesh_texture, "poly near", 0, NEAR_DEPTH, 0, grid_polygons); break;
        case 6: draw_small_meshes("small x16", 0); break;
        case 7: draw_small_meshes("small offs", OFFSCREEN_X); break;
        case 8: draw_mesh(mesh_points, "vert cpu", 0, DEPTH, 0, POINT_COUNT); break;
        // The SCU DSP transforms the vertices
        case 9: jo_3d_enable_dsp(false); draw_mesh(mesh_points, "vert dsp", 0, DEPTH, 0, POINT_COUNT); break;
        case 10: draw_mesh(mesh_color, "dsp color", 0, DEPTH, 0, grid_polygons); break;
        case 11: draw_mesh(mesh_texture, "dsp offscr", OFFSCREEN_X, DEPTH, 0, grid_polygons); break;
        case 12: draw_mesh(mesh_texture, "dsp back", 0, DEPTH, BACK_ANGLE, grid_polygons); jo_3d_disable_dsp(); break;
        default: return (false);
    }
    return (true);
}
