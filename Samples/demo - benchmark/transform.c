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
** Transformation and projection of vertices by the CPU: the loop of the engine written in C (matrix, translation,
** projection by the division unit, the division of a vertex running while the next one is transformed), from the high
** and the low work RAM, with the on-chip cache RAM
*/

#include <jo/jo.h>
#include "benchmark.h"

/** @brief Vertices transformed */
#define VERTEX_COUNT                (512)
/** @brief Vertices transformed by the cache RAM tests (2 KB of cache RAM: 128 vertices of 16 bytes) */
#define CACHE_RAM_VERTEX_COUNT      (128)
/** @brief The vertices: a grid of 32 x 16 points, 10 units apart, 400 units in front of the camera */
#define GRID_WIDTH                  (32)
#define GRID_SPACING                (10)
#define GRID_DEPTH                  (400)
/** @brief Rotation of the test matrix (degrees around Y) and its translation (units along Z) */
#define MATRIX_ANGLE                (30)
#define MATRIX_TRANSLATION_Z        (100)
/** @brief Projection: the distance of the screen and its center (320 x 224) */
#define PROJECTION_DISTANCE         (160)
#define SCREEN_CENTER_X             (JO_TV_WIDTH / 2)
#define SCREEN_CENTER_Y             (JO_TV_HEIGHT / 2)
/** @brief Outcodes of a projected vertex: outside of the screen horizontally, vertically */
#define CODE_OUTSIDE_X              (1 << 0)
#define CODE_OUTSIDE_Y              (1 << 2)
/** @brief Place of the vertices in the low work RAM (unused by the engine) */
#define LOW_WORK_RAM_VERTICES       ((jo_pos3D_fixed *)JO_WORK_RAM_LOW)

/** @brief A transformed vertex: its screen position, its depth and its outcode */
typedef struct
{
    int                         x;
    int                         y;
    jo_fixed                    z;
    unsigned int                code;
}                               transformed_vertex;

static jo_pos3D_fixed           vertices[VERTEX_COUNT];
static transformed_vertex       output[VERTEX_COUNT];
/** @brief The matrix: 3 axes and the translation */
static jo_fixed                 matrix[4][3];

static void                     init_vertices(void)
{
    jo_pos3D_fixed              *low_vertices;
    int                         i;

    low_vertices = LOW_WORK_RAM_VERTICES;
    for (i = 0; i < VERTEX_COUNT; ++i)
    {
        vertices[i].x = jo_int2fixed((i % GRID_WIDTH) * GRID_SPACING - GRID_WIDTH * GRID_SPACING / 2);
        vertices[i].y = jo_int2fixed((i / GRID_WIDTH) * GRID_SPACING - (VERTEX_COUNT / GRID_WIDTH) * GRID_SPACING / 2);
        vertices[i].z = jo_int2fixed(GRID_DEPTH + (i & 7));
        low_vertices[i] = vertices[i];
    }
    // Rotation around Y and translation
    matrix[0][0] = jo_cos(MATRIX_ANGLE); matrix[0][1] = 0; matrix[0][2] = -jo_sin(MATRIX_ANGLE);
    matrix[1][0] = 0; matrix[1][1] = JO_FIXED_1; matrix[1][2] = 0;
    matrix[2][0] = jo_sin(MATRIX_ANGLE); matrix[2][1] = 0; matrix[2][2] = jo_cos(MATRIX_ANGLE);
    matrix[3][0] = 0; matrix[3][1] = 0; matrix[3][2] = jo_int2fixed(MATRIX_TRANSLATION_Z);
}

/** @brief The columns of the matrix: contiguous for jo_fixed_dot() (the multiply and accumulate unit) */
static void                     get_columns(jo_fixed columns[3][3])
{
    int                         i;

    for (i = 0; i < 3; ++i)
    {
        columns[i][0] = matrix[0][i];
        columns[i][1] = matrix[1][i];
        columns[i][2] = matrix[2][i];
    }
}

/** @brief Project a vertex with the quotient of the division unit: k = projection distance / z */
static __jo_force_inline void   project(transformed_vertex * const vertex, const jo_fixed x, const jo_fixed y)
{
    jo_fixed                    k;

    k = jo_fixed_div_result();
    vertex->x = SCREEN_CENTER_X + (jo_fixed_mult(x, k) >> 16);
    vertex->y = SCREEN_CENTER_Y + (jo_fixed_mult(y, k) >> 16);
    vertex->code = ((unsigned int)vertex->x >= JO_TV_WIDTH ? CODE_OUTSIDE_X : 0) | ((unsigned int)vertex->y >= JO_TV_HEIGHT ? CODE_OUTSIDE_Y : 0);
}

/** @brief Transform and project the vertices like the engine (without the near plane): the division of a vertex runs
 *  in the division unit while the next vertex is transformed */
static void __attribute__((noinline)) transform_and_project(const jo_pos3D_fixed *in, transformed_vertex *out, const int count)
{
    jo_fixed                    columns[3][3];
    transformed_vertex          *pending;
    jo_fixed                    pending_x;
    jo_fixed                    pending_y;
    jo_fixed                    x;
    jo_fixed                    y;
    jo_fixed                    z;
    int                         i;

    get_columns(columns);
    pending = JO_NULL;
    pending_x = 0;
    pending_y = 0;
    for (i = 0; i < count; ++i, ++in, ++out)
    {
        x = jo_fixed_dot((const jo_fixed *)in, columns[0]) + matrix[3][0];
        y = jo_fixed_dot((const jo_fixed *)in, columns[1]) + matrix[3][1];
        z = jo_fixed_dot((const jo_fixed *)in, columns[2]) + matrix[3][2];
        out->z = z;
        if (pending != JO_NULL)
            project(pending, pending_x, pending_y);
        jo_fixed_div_start(jo_int2fixed(PROJECTION_DISTANCE), z);
        pending = out;
        pending_x = x;
        pending_y = y;
    }
    if (pending != JO_NULL)
        project(pending, pending_x, pending_y);
}

/** @brief The matrix only (no projection) */
static void __attribute__((noinline)) transform_only(const jo_pos3D_fixed *in, transformed_vertex *out, const int count)
{
    jo_fixed                    columns[3][3];
    int                         i;

    get_columns(columns);
    for (i = 0; i < count; ++i, ++in, ++out)
    {
        out->x = jo_fixed_dot((const jo_fixed *)in, columns[0]) + matrix[3][0];
        out->y = jo_fixed_dot((const jo_fixed *)in, columns[1]) + matrix[3][1];
        out->z = jo_fixed_dot((const jo_fixed *)in, columns[2]) + matrix[3][2];
    }
}

/** @brief The divisions only, overlapped like the engine (the depths of the vertices transformed before; | 1 unit: never 0) */
static void __attribute__((noinline)) divide_only(transformed_vertex *out, const int count)
{
    int                         i;

    jo_fixed_div_start(jo_int2fixed(PROJECTION_DISTANCE), out[0].z | JO_FIXED_1);
    for (i = 1; i < count; ++i)
    {
        out[i - 1].code = (unsigned int)jo_fixed_div_result();
        jo_fixed_div_start(jo_int2fixed(PROJECTION_DISTANCE), out[i].z | JO_FIXED_1);
    }
    out[count - 1].code = (unsigned int)jo_fixed_div_result();
}

void                            transform_benchmarks(void)
{
    init_vertices();
    // (once before the measures: the code and the vertices in the cache)
    transform_and_project(vertices, output, VERTEX_COUNT);
    BENCHMARK_MEASURE("xform HWRAM", VERTEX_COUNT, transform_and_project(vertices, output, VERTEX_COUNT));
    BENCHMARK_MEASURE("xform LWRAM", VERTEX_COUNT, transform_and_project(LOW_WORK_RAM_VERTICES, output, VERTEX_COUNT));
    BENCHMARK_MEASURE("matrix only", VERTEX_COUNT, transform_only(vertices, output, VERTEX_COUNT));
    BENCHMARK_MEASURE("div only", VERTEX_COUNT, divide_only(output, VERTEX_COUNT));
    // The output in the on-chip cache RAM (half of the cache), then in the high work RAM with a 2 KB cache, then with the 4 KB cache
    JO_SH2_CCR = JO_SH2_CCR_CACHE_PURGE | SH2_CCR_TWO_WAY_MODE | JO_SH2_CCR_CACHE_ENABLE;
    BENCHMARK_MEASURE("xform onchip", CACHE_RAM_VERTEX_COUNT, transform_and_project(vertices, (transformed_vertex *)SH2_CACHE_RAM_ADDRESS, CACHE_RAM_VERTEX_COUNT));
    BENCHMARK_MEASURE("xform HW 2KB", CACHE_RAM_VERTEX_COUNT, transform_and_project(vertices, output, CACHE_RAM_VERTEX_COUNT));
    JO_SH2_CCR = JO_SH2_CCR_CACHE_PURGE | JO_SH2_CCR_CACHE_ENABLE;
    BENCHMARK_MEASURE("xform HW 4KB", CACHE_RAM_VERTEX_COUNT, transform_and_project(vertices, output, CACHE_RAM_VERTEX_COUNT));
}
