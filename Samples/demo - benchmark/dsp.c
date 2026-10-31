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
** The SCU DSP: the 3 x 3 transformation of vertices by the DSP (dsp/xform.dsp, the program of the engine) compared to
** the CPU, and its results checked
*/

#include <jo/jo.h>
#include "benchmark.h"
#include "dsp_programs.h"

/** @brief Vertices transformed */
#define VERTEX_COUNT                (504)
/** @brief Vertices of a batch of the DSP: a bank of its data RAM holds 64 words, 21 vertices of 3 words */
#define VERTICES_PER_BATCH          (21)
/** @brief Data RAM of the DSP (see dsp/xform.dsp): the matrix in the bank 0, the parameters of a batch at the end of it,
 *  the table of the first words of the vertices in the bank 2 */
#define BANK_MATRIX                 (0)
#define BANK_TABLE                  (2)
#define PARAMETERS_ADDRESS          (60)
/** @brief The vertices: pseudo random coordinates (a multiplicative hash) in -4096 to 4096 */
#define RANDOM_MULTIPLIER           (2654435761u)
#define COORDINATE_RANGE            (0x20000000u)
#define COORDINATE_OFFSET           (0x10000000)

static jo_fixed                 input[VERTEX_COUNT * 3] __attribute__((aligned(16)));
static jo_fixed                 output_dsp[VERTEX_COUNT * 3] __attribute__((aligned(16)));
static jo_fixed                 output_cpu[VERTEX_COUNT * 3];
/** @brief A rotation of 30 degrees around Y with a shear (every term is used) */
static const jo_fixed           matrix[9] = {56756, 0, -32768, 1000, 65536, 300, 32768, -2000, 56756};
int                             dsp_errors;

static void                     init_vertices(void)
{
    int                         i;

    for (i = 0; i < VERTEX_COUNT * 3; ++i)
        input[i] = (jo_fixed)((i * RANDOM_MULTIPLIER) % COORDINATE_RANGE) - COORDINATE_OFFSET;
}

static void __attribute__((noinline)) transform_cpu(void)
{
    jo_fixed                    columns[3][3];
    int                         i;
    int                         k;

    for (k = 0; k < 3; ++k)
    {
        columns[k][0] = matrix[3 * k];
        columns[k][1] = matrix[3 * k + 1];
        columns[k][2] = matrix[3 * k + 2];
    }
    for (i = 0; i < VERTEX_COUNT; ++i)
    {
        output_cpu[3 * i] = jo_fixed_dot(&input[3 * i], columns[0]);
        output_cpu[3 * i + 1] = jo_fixed_dot(&input[3 * i], columns[1]);
        output_cpu[3 * i + 2] = jo_fixed_dot(&input[3 * i], columns[2]);
    }
}

/** @brief Load the program, the matrix and the table of the vertices of a batch */
static void                     transform_dsp_setup(void)
{
    unsigned int                table[2 * VERTICES_PER_BATCH];
    int                         i;

    jo_dsp_load_program(dsp_xform, sizeof(dsp_xform) / sizeof(dsp_xform[0]));
    jo_dsp_write_data(BANK_MATRIX, 0, (const unsigned int *)matrix, 9);
    for (i = 0; i < VERTICES_PER_BATCH; ++i)
        table[2 * i] = table[2 * i + 1] = 3 * i;
    jo_dsp_write_data(BANK_TABLE, 0, table, 2 * VERTICES_PER_BATCH);
}

/** @brief Transform the vertices by batches: the parameters of a batch (the addresses in words, the counts), then the
 *  program runs (it reads and writes the vertices by DMA) */
static void __attribute__((noinline)) transform_dsp(void)
{
    unsigned int                parameters[4];
    int                         i;
    int                         n;

    for (i = 0; i < VERTEX_COUNT; i += VERTICES_PER_BATCH)
    {
        n = JO_MIN(VERTEX_COUNT - i, VERTICES_PER_BATCH);
        parameters[0] = ((unsigned int)&input[3 * i]) >> 2;
        parameters[1] = ((unsigned int)&output_dsp[3 * i]) >> 2;
        parameters[2] = n - 1;
        parameters[3] = 3 * n;
        jo_dsp_write_data(BANK_MATRIX, PARAMETERS_ADDRESS, parameters, 4);
        jo_dsp_start(0);
        jo_dsp_wait();
    }
}

/** @brief Count the vertices of the DSP that differ from the CPU (the DSP wrote the memory: read uncached) */
static int                      count_errors(void)
{
    const volatile jo_fixed     *result;
    int                         errors;
    int                         i;

    result = (const volatile jo_fixed *)JO_CACHE_THROUGH(output_dsp);
    errors = 0;
    for (i = 0; i < VERTEX_COUNT * 3; ++i)
        if (result[i] != output_cpu[i])
            ++errors;
    return (errors);
}

void                            dsp_benchmarks(void)
{
    init_vertices();
    BENCHMARK_MEASURE("xf cpu 3x3", VERTEX_COUNT, transform_cpu());
    transform_dsp_setup();
    BENCHMARK_MEASURE("xf dsp 3x3", VERTEX_COUNT, transform_dsp());
    dsp_errors = count_errors();
}
