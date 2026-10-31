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
** Benchmarks of the Sega Saturn with the Jo Engine
**
** Micro benchmarks of the hardware and of the engine: the memory, the DMA, the transformation of the vertices by the
** CPU and by the SCU DSP, the 3D pipeline. The results are in nanoseconds per item (vertex, 32 bits word, VDP1 command,
** polygon).
**
** UP / DOWN    : scroll the results
*/

#include <jo/jo.h>
#include "benchmark.h"

/** @brief Frames of the demo: the tests of the CPU run in one frame, then one 3D test per frame, then the results */
typedef enum
{
    PHASE_START,            // The screen is initialized
    PHASE_CPU_TESTS,        // Memory, transformation and DSP tests
    PHASE_POLYGON_TESTS,    // One test per frame
    PHASE_RESULTS           // The results stay on the screen
}                               demo_phase;

/** @brief Frames skipped before the tests: the first frames of the engine are not representative */
#define WARM_UP_FRAMES              (2)

static benchmark_result         results[BENCHMARK_MAX_RESULTS];
static int                      result_count = 0;
static demo_phase               phase = PHASE_START;
static int                      frame = 0;
static int                      polygon_step = 0;
static int                      first_shown = 0;

void                            benchmark_add(const char * const name, const unsigned int ticks, const unsigned int items)
{
    benchmark_result            *result;

    if (result_count >= BENCHMARK_MAX_RESULTS)
        return;
    result = &results[result_count++];
    result->name = name;
    result->ticks = ticks;
    result->items = items;
}

const benchmark_result          *benchmark_get(const int index)
{
    return (&results[index]);
}

int                             benchmark_count(void)
{
    return (result_count);
}

/** @brief Nanoseconds per item of a result */
static unsigned int             nanoseconds_per_item(const benchmark_result * const result)
{
    if (result->items == 0)
        return (0);
    return ((unsigned int)(((unsigned long long)result->ticks * BENCHMARK_NS_PER_TICK_X100) / (100 * result->items)));
}

static void                     print_results(void)
{
    int                         i;
    int                         line;
    const benchmark_result      *result;

    jo_printf(0, 0, "test         ticks items  ns/item");
    for (line = 0; line < BENCHMARK_LINES_PER_PAGE; ++line)
    {
        i = first_shown + line;
        if (i < result_count)
        {
            result = benchmark_get(i);
            jo_printf(0, 1 + line, "%-12s %5d %5d %8d", result->name, result->ticks, result->items, nanoseconds_per_item(result));
        }
        else
            jo_printf(0, 1 + line, "                                  ");
    }
    jo_printf(0, 27, "copy errors %d %d %d  dsp errors %d", memory_copy_errors[0], memory_copy_errors[1], memory_copy_errors[2], dsp_errors);
}

/** @brief UP and DOWN scroll the results */
static void                     scroll_results(void)
{
    int                         last_first;

    last_first = JO_MAX(0, result_count - BENCHMARK_LINES_PER_PAGE);
    if (jo_is_pad1_key_down(JO_KEY_UP) && first_shown > 0)
        --first_shown;
    if (jo_is_pad1_key_down(JO_KEY_DOWN) && first_shown < last_first)
        ++first_shown;
}

static void                     my_draw(void)
{
    ++frame;
    switch (phase)
    {
        case PHASE_START:
            if (frame > WARM_UP_FRAMES)
                phase = PHASE_CPU_TESTS;
            break;
        case PHASE_CPU_TESTS:
            memory_benchmarks();
            transform_benchmarks();
            dsp_benchmarks();
            phase = PHASE_POLYGON_TESTS;
            break;
        case PHASE_POLYGON_TESTS:
            if (!polygon_benchmark_step(polygon_step++))
            {
                phase = PHASE_RESULTS;
                first_shown = JO_MAX(0, result_count - BENCHMARK_LINES_PER_PAGE);
            }
            break;
        case PHASE_RESULTS:
            scroll_results();
            print_results();
            break;
    }
}

void                            jo_main(void)
{
    jo_core_init(JO_COLOR_Black);
    polygon_benchmarks_init();
    jo_core_add_callback(my_draw);
    jo_core_run();
}
