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
** Memory benchmarks: reads and writes of the high and low work RAM (cached and uncached), copies to the VDP1 VRAM by the
** CPU, by the DMA of the SH-2 (jo_dma_copy()) and by the DMA of the SCU (direct and indirect modes), the on-chip cache RAM
*/

#include <jo/jo.h>
#include "benchmark.h"

/** @brief VDP1 commands copied (like the command table of a frame) */
#define COMMAND_COUNT               (500)
/** @brief A VDP1 command: 32 bytes, 8 words, 16 half words */
#define COMMAND_WORDS               (8)
#define COMMAND_HALF_WORDS          (16)
#define COMMAND_BYTES               (32)
/** @brief Place of the copies in the VDP1 VRAM: after the command table of the engine, in the texture area (unused here) */
#define VDP1_TEST_AREA              (JO_VDP1_VRAM + 0x40000)
/** @brief Words read by the read tests (16 KB) */
#define READ_WORDS                  (4096)
/** @brief Place of the tests in the low work RAM (unused by the engine) */
#define LOW_WORK_RAM_TEST_AREA      ((unsigned int *)(JO_WORK_RAM_LOW + 0x10000))
/** @brief Commands written in the cache RAM tests (2 KB of cache RAM) */
#define CACHE_RAM_COMMANDS          (64)
/** @brief Source of the indirect DMA of the command i: the commands are copied in another order (like a sorted table) */
#define INDIRECT_SOURCE(I)          ((((I) * 7) % COMMAND_COUNT) * COMMAND_WORDS)

/** @brief The commands copied (high work RAM, aligned for the DMA) */
static unsigned int             commands[COMMAND_COUNT * COMMAND_WORDS] __attribute__((aligned(16)));
/** @brief Destination of the write tests */
static unsigned int             write_buffer[COMMAND_COUNT * COMMAND_WORDS] __attribute__((aligned(16)));
/** @brief Indirect mode of the SCU DMA: a table of transfers (size, destination, source), the last one flagged */
static unsigned int             indirect_table[COMMAND_COUNT * 3] __attribute__((aligned(32)));
/** @brief The sums of the read tests go here (the compiler can't remove the reads) */
volatile unsigned int           memory_read_sink;
int                             memory_copy_errors[3];

static void                     init_commands(void)
{
    int                         i;

    for (i = 0; i < COMMAND_COUNT * COMMAND_WORDS; ++i)
        commands[i] = 0x10001 * (i & 0xFF);
}

static unsigned int __attribute__((noinline)) read_sum(const unsigned int * const words, const int count)
{
    unsigned int                sum;
    int                         i;

    sum = 0;
    for (i = 0; i < count; i += 4)
        sum += words[i] + words[i + 1] + words[i + 2] + words[i + 3];
    return (sum);
}

/** @brief Write commands as 8 words each */
static void __attribute__((noinline)) write_words(unsigned int *destination, const int count)
{
    int                         i;

    for (i = 0; i < count; ++i, destination += COMMAND_WORDS)
    {
        destination[0] = i; destination[1] = i; destination[2] = i; destination[3] = i;
        destination[4] = i; destination[5] = i; destination[6] = i; destination[7] = i;
    }
}

/** @brief Write commands as 16 half words each (the way a VDP1 command is filled field by field) */
static void __attribute__((noinline)) write_half_words(unsigned short *destination, const int count)
{
    int                         i;

    for (i = 0; i < count; ++i, destination += COMMAND_HALF_WORDS)
    {
        destination[0] = i; destination[1] = i; destination[2] = i; destination[3] = i;
        destination[4] = i; destination[5] = i; destination[6] = i; destination[7] = i;
        destination[8] = i; destination[9] = i; destination[10] = i; destination[11] = i;
        destination[12] = i; destination[13] = i; destination[14] = i; destination[15] = i;
    }
}

static void __attribute__((noinline)) cpu_copy_to_vdp1(void)
{
    volatile unsigned int       *destination;
    const unsigned int          *source;
    int                         i;

    destination = (volatile unsigned int *)VDP1_TEST_AREA;
    source = commands;
    for (i = 0; i < COMMAND_COUNT; ++i, destination += COMMAND_WORDS, source += COMMAND_WORDS)
    {
        destination[0] = source[0]; destination[1] = source[1]; destination[2] = source[2]; destination[3] = source[3];
        destination[4] = source[4]; destination[5] = source[5]; destination[6] = source[6]; destination[7] = source[7];
    }
}

static void                     scu_dma_wait(void)
{
    while (JO_SCU_DMA_STATUS & JO_SCU_DMA_STATUS_BUSY(0))
        ;
}

/** @brief SCU DMA level 0, direct mode: one transfer of all the commands */
static void __attribute__((noinline)) scu_dma_copy_to_vdp1(void)
{
    JO_SCU_DMA_ENABLE(0) = 0;
    JO_SCU_DMA_READ(0) = (unsigned int)commands;
    JO_SCU_DMA_WRITE(0) = VDP1_TEST_AREA;
    JO_SCU_DMA_COUNT(0) = COMMAND_COUNT * COMMAND_BYTES;
    JO_SCU_DMA_ADD(0) = JO_SCU_DMA_ADD_READ4_WRITE2;
    JO_SCU_DMA_MODE(0) = JO_SCU_DMA_MODE_DIRECT;
    JO_SCU_DMA_ENABLE(0) = JO_SCU_DMA_ENABLE_START;
    scu_dma_wait();
}

/** @brief SCU DMA level 0, indirect mode: one transfer per command, in the order of the table (like a copy in the order
 *  of a sort) */
static void __attribute__((noinline)) scu_dma_build_indirect_table(void)
{
    int                         i;

    for (i = 0; i < COMMAND_COUNT; ++i)
    {
        indirect_table[i * 3] = COMMAND_BYTES;
        indirect_table[i * 3 + 1] = VDP1_TEST_AREA + i * COMMAND_BYTES;
        indirect_table[i * 3 + 2] = (unsigned int)&commands[INDIRECT_SOURCE(i)];
    }
    indirect_table[COMMAND_COUNT * 3 - 1] |= JO_SCU_DMA_INDIRECT_END;
}

static void __attribute__((noinline)) scu_dma_indirect_copy_to_vdp1(void)
{
    JO_SCU_DMA_ENABLE(0) = 0;
    JO_SCU_DMA_WRITE(0) = (unsigned int)indirect_table;
    JO_SCU_DMA_ADD(0) = JO_SCU_DMA_ADD_READ4_WRITE2;
    JO_SCU_DMA_MODE(0) = JO_SCU_DMA_MODE_INDIRECT;
    JO_SCU_DMA_ENABLE(0) = JO_SCU_DMA_ENABLE_START;
    scu_dma_wait();
}

static void                     clear_vdp1_test_area(void)
{
    volatile unsigned int       *destination;
    int                         i;

    destination = (volatile unsigned int *)VDP1_TEST_AREA;
    for (i = 0; i < COMMAND_COUNT * COMMAND_WORDS; ++i)
        destination[i] = 0;
}

/** @brief Check a copy of the commands in the VDP1 VRAM
 *  @return 0 if the copy is right, else the index of the first wrong half word + 1
 */
static int                      check_vdp1_copy(void)
{
    volatile unsigned short     *destination;
    const unsigned short        *source;
    int                         i;

    destination = (volatile unsigned short *)VDP1_TEST_AREA;
    source = (const unsigned short *)commands;
    for (i = 0; i < COMMAND_COUNT * COMMAND_HALF_WORDS; ++i)
        if (destination[i] != source[i])
            return (i + 1);
    return (0);
}

/** @brief Check the indirect copy (the commands in the order of the table)
 *  @return 0 if the copy is right, else the index of the first wrong command + 1
 */
static int                      check_vdp1_indirect_copy(void)
{
    volatile unsigned short     *destination;
    const unsigned short        *source;
    int                         i;
    int                         k;

    for (i = 0; i < COMMAND_COUNT; ++i)
    {
        destination = (volatile unsigned short *)(VDP1_TEST_AREA + i * COMMAND_BYTES);
        source = (const unsigned short *)&commands[INDIRECT_SOURCE(i)];
        for (k = 0; k < COMMAND_HALF_WORDS; ++k)
            if (destination[k] != source[k])
                return (i + 1);
    }
    return (0);
}

void                            memory_benchmarks(void)
{
    init_commands();
    // Reads: the high work RAM (twice: the second time, the first lines are in the cache), the low work RAM, uncached
    BENCHMARK_MEASURE("read HW 16K", READ_WORDS, memory_read_sink = read_sum(commands, READ_WORDS));
    BENCHMARK_MEASURE("read HW 16K", READ_WORDS, memory_read_sink = read_sum(commands, READ_WORDS));
    BENCHMARK_MEASURE("read LW 16K", READ_WORDS, memory_read_sink = read_sum(LOW_WORK_RAM_TEST_AREA, READ_WORDS));
    BENCHMARK_MEASURE("read HW unc", READ_WORDS, memory_read_sink = read_sum((const unsigned int *)JO_CACHE_THROUGH(commands), READ_WORDS));
    BENCHMARK_MEASURE("read LW unc", READ_WORDS, memory_read_sink = read_sum((const unsigned int *)JO_CACHE_THROUGH(LOW_WORK_RAM_TEST_AREA), READ_WORDS));
    // Writes: words and half words in the high work RAM, words in the low work RAM
    BENCHMARK_MEASURE("write 8 w", COMMAND_COUNT, write_words(write_buffer, COMMAND_COUNT));
    BENCHMARK_MEASURE("write 16 h", COMMAND_COUNT, write_half_words((unsigned short *)write_buffer, COMMAND_COUNT));
    BENCHMARK_MEASURE("write 8w LW", COMMAND_COUNT, write_words(LOW_WORK_RAM_TEST_AREA, COMMAND_COUNT));
    // Copies to the VDP1 VRAM, each one checked
    clear_vdp1_test_area();
    BENCHMARK_MEASURE("cpu->vdp1", COMMAND_COUNT, cpu_copy_to_vdp1());
    memory_copy_errors[0] = check_vdp1_copy();
    clear_vdp1_test_area();
    BENCHMARK_MEASURE("dmac->vdp1", COMMAND_COUNT, jo_dma_copy(commands, (void *)VDP1_TEST_AREA, COMMAND_COUNT * COMMAND_BYTES));
    memory_copy_errors[1] = check_vdp1_copy();
    clear_vdp1_test_area();
    BENCHMARK_MEASURE("scu->vdp1", COMMAND_COUNT, scu_dma_copy_to_vdp1());
    memory_copy_errors[2] = check_vdp1_copy();
    clear_vdp1_test_area();
    BENCHMARK_MEASURE("ind build", COMMAND_COUNT, scu_dma_build_indirect_table());
    BENCHMARK_MEASURE("scu ind", COMMAND_COUNT, scu_dma_indirect_copy_to_vdp1());
    memory_copy_errors[0] |= check_vdp1_indirect_copy() << 16;
    // The cache RAM: half of the cache becomes a 2 KB RAM on the chip, written like the high work RAM
    JO_SH2_CCR = JO_SH2_CCR_CACHE_PURGE | SH2_CCR_TWO_WAY_MODE | JO_SH2_CCR_CACHE_ENABLE;
    BENCHMARK_MEASURE("write8w onch", CACHE_RAM_COMMANDS, write_words((unsigned int *)SH2_CACHE_RAM_ADDRESS, CACHE_RAM_COMMANDS));
    BENCHMARK_MEASURE("write8w HW", CACHE_RAM_COMMANDS, write_words(write_buffer, CACHE_RAM_COMMANDS));
    JO_SH2_CCR = JO_SH2_CCR_CACHE_PURGE | JO_SH2_CCR_CACHE_ENABLE;
}
