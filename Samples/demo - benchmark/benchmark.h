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
** Benchmarks of the Sega Saturn with the Jo Engine: the results table shared by the tests
*/

#ifndef __BENCHMARK_H__
# define __BENCHMARK_H__

/** @brief Maximum number of results */
# define BENCHMARK_MAX_RESULTS          (40)
/** @brief Results shown on the screen at once (the lines under the title) */
# define BENCHMARK_LINES_PER_PAGE       (26)
/** @brief Cache control register: two-way mode, the ways 0 and 1 (2 KB) become a RAM at SH2_CACHE_RAM_ADDRESS, the ways 2
 *  and 3 stay a 2 KB cache (SH7604 hardware manual, CCR bit TW). The tests restore the 4 KB cache after */
# define SH2_CCR_TWO_WAY_MODE           (1 << 3)
# define SH2_CACHE_RAM_ADDRESS          (0xC0000000)
/** @brief A tick of jo_profile_ticks() is 32 clocks of the CPU (26.8466 MHz): 1191.96 ns, in hundredths of ns */
# define BENCHMARK_NS_PER_TICK_X100     (119196)

/** @brief One result: the time of a test and the number of items it processed (the screen shows the ns per item) */
typedef struct
{
    const char                  *name;      // 12 characters at most (one line of the screen)
    unsigned int                ticks;      // Ticks of jo_profile_ticks()
    unsigned int                items;      // Vertices, words, commands, polygons... processed
}                               benchmark_result;

/** @brief Add a result */
void                            benchmark_add(const char * const name, const unsigned int ticks, const unsigned int items);
/** @brief Results added so far */
const benchmark_result          *benchmark_get(const int index);

/** @brief Measure a statement: the ticks of jo_profile_ticks() around it (16 bits: a test must take less than 78 ms) */
# define BENCHMARK_MEASURE(NAME, ITEMS, STATEMENT) \
    do { unsigned int __t0 = jo_profile_ticks(); STATEMENT; benchmark_add(NAME, (jo_profile_ticks() - __t0) & 0xFFFF, ITEMS); } while (0)

/*
** The tests (one file each)
*/

/** @brief Memory: reads and writes of the work RAM, copies to the VDP1 VRAM (CPU, SH-2 DMA, SCU DMA) */
void                            memory_benchmarks(void);
/** @brief The copies to the VDP1 VRAM are checked: 0 when the copy is right, else the index of the first wrong word + 1 */
extern int                      memory_copy_errors[3];

/** @brief Transformation and projection of vertices by the CPU (the loop of the engine, written in C) */
void                            transform_benchmarks(void);

/** @brief Transformation of vertices by the SCU DSP, compared to the CPU */
void                            dsp_benchmarks(void);
/** @brief Vertices transformed by the DSP that differ from the CPU (0 expected) */
extern int                      dsp_errors;

/** @brief The 3D pipeline: jo_3d_mesh_draw() with different meshes (one test per frame: the VDP1 commands of a frame are limited) */
void                            polygon_benchmarks_init(void);
/** @brief Run the test of a frame
 *  @param step Test index, from 0
 *  @return false when there is no more test
 */
bool                            polygon_benchmark_step(const int step);

#endif /* !__BENCHMARK_H__ */
