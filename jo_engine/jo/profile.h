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
/** @file profile.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine profiler: time spent by the CPUs and the VDP1 in each frame
 *  @remarks Compile with JO_COMPILE_WITH_PROFILER_MODULE = 1 in the makefile. Without it, the functions of this file do nothing
 *           (no cost)
 *  @remarks The times are measured with the free running timer of each CPU (1 tick = 32 CPU clocks, about 1.2 us).
 *           An interval must be shorter than 65535 ticks (about 78 ms)
 *  @remarks The averages and the maximums are computed over JO_PROFILE_FRAMES frames
 *
 *  Example:
 *  @code
 *  static void my_draw(void)
 *  {
 *      jo_profile_begin(JO_PROFILE_USER(0));
 *      draw_terrain();
 *      jo_profile_end(JO_PROFILE_USER(0));
 *      jo_profile_print(1, 3);
 *  }
 *  ...
 *  jo_profile_set_name(JO_PROFILE_USER(0), "terrain");
 *  @endcode
 */

#ifndef __JO_PROFILE_H__
# define __JO_PROFILE_H__

/** @brief Measures of the engine and of the program (jo_profile_begin(), jo_profile_end(), jo_profile_count()) */
typedef enum
{
    /** @brief Frame: from the start of the callbacks of a frame to the start of the next one */
    JO_PROFILE_FRAME = 0,
    /** @brief Callbacks of the master CPU (jo_core_add_callback()) */
    JO_PROFILE_CALLBACKS,
    /** @brief Master CPU waiting for the slave CPU (jo_core_wait_for_slave()) */
    JO_PROFILE_WAIT_SLAVE,
    /** @brief Slave CPU busy (all its jobs: the callbacks of jo_core_exec_on_slave() and the engine jobs) */
    JO_PROFILE_SLAVE,
    /** @brief Sort of the VDP1 commands of a frame */
    JO_PROFILE_VDP1_SORT,
    /** @brief Copy of the VDP1 commands of a frame into the VDP1 VRAM */
    JO_PROFILE_VDP1_COPY,
    /** @brief Wait for the VDP1 before the copy (previous table not displayed yet, or still drawn) */
    JO_PROFILE_VDP1_WAIT,
    /** @brief Master CPU waiting for the VBLANK at the end of the frame (JO_FRAMERATE) */
    JO_PROFILE_VSYNC,
    /** @brief VDP1: from the display of a new command table to the end of its drawing */
    JO_PROFILE_VDP1_DRAW,
    /** @brief VBLANK per frame (count, x 100) */
    JO_PROFILE_VBLANKS,
    /** @brief VDP1 commands per frame (count) */
    JO_PROFILE_VDP1_COMMANDS,
    /** @brief 3D polygons displayed per frame (count) */
    JO_PROFILE_POLYGONS,
    /** @brief 3D polygons tested per frame, visible or not (count, both CPUs) */
    JO_PROFILE_3D_POLYGON_COUNT,
    /** @brief 3D vertices transformed per frame (count, both CPUs) */
    JO_PROFILE_3D_VERTEX_COUNT,
    /** @brief 3D: transformation and projection of the vertices (both CPUs) */
    JO_PROFILE_3D_VERTICES,
    /** @brief 3D: polygons (visibility, clipping, lighting, VDP1 commands), both CPUs */
    JO_PROFILE_3D_POLYGONS,
    /** @brief 3D: polygons near the camera (subdivided) or crossing the near plane (clipped), both CPUs (included in JO_PROFILE_3D_POLYGONS) */
    JO_PROFILE_3D_CLIP,
    /** @brief 3D: lighting of the polygons (JO_3D_USE_LIGHT), both CPUs (included in JO_PROFILE_3D_POLYGONS) */
    JO_PROFILE_3D_LIGHT,
    /** @brief 3D: estimate of the pixels the VDP1 draws for the polygons of a frame (count, both CPUs): the VDP1 draws
     *  about max(lines, texture height) x max(pixels per line, texture width) pixels for a polygon, even smaller on the
     *  screen, even hidden by the next polygons or outside of the screen (about 28 pixels per microsecond) */
    JO_PROFILE_3D_PIXELS,
    /** @brief First measure of the program (see JO_PROFILE_USER()) */
    JO_PROFILE_FIRST_USER,
    /** @brief Measures count */
    JO_PROFILE_COUNT = JO_PROFILE_FIRST_USER + 8
}                                   jo_profile_slot;

/** @brief Measure of the program
 *  @param N 0 to 7
 */
# define JO_PROFILE_USER(N)         ((jo_profile_slot)(JO_PROFILE_FIRST_USER + (N)))

/** @brief Frames of the averages and of the maximums */
# define JO_PROFILE_FRAMES          (32)

#ifdef JO_COMPILE_WITH_PROFILER_SUPPORT

/** @brief Measures of the master CPU and of the slave CPU (Internal usage) */
typedef struct
{
    unsigned int                    sum[JO_PROFILE_COUNT];      // Never reset (the differences are computed)
    unsigned int                    max[JO_PROFILE_COUNT];
    unsigned short                  start[JO_PROFILE_COUNT];    // See jo_profile_begin()
}                                   jo_profile_cpu;

/** @brief Measures of each CPU (Internal usage) */
extern jo_profile_cpu               __jo_profile_cpu[2];

/** @brief Get the free running timer of the current CPU
 *  @return Ticks (32 CPU clocks)
 */
static  __jo_force_inline unsigned int  jo_profile_ticks(void)
{
    unsigned int                    sr;
    unsigned int                    high;
    unsigned int                    low;

    // The high byte must be read first (the low byte is latched in the only temporary register of the timer: interrupts
    // masked, the sampling handler and the VBLANK read the timer)
    __asm__ volatile ("stc sr, %0" : "=r" (sr) : : "memory");
    __asm__ volatile ("ldc %0, sr" : : "r" (sr | 0xF0) : "memory");
    high = *(volatile unsigned char *)0xFFFFFE12;
    low = *(volatile unsigned char *)0xFFFFFE13;
    __asm__ volatile ("ldc %0, sr" : : "r" (sr) : "memory");
    return ((high << 8) | low);
}

/** @brief Measures of the current CPU (Internal usage)
 *  @remarks The stack of the slave CPU is in the first 16 KB of the high work RAM
 *  @remarks Through the cache: only this CPU writes its sums, its maximums and its starts (the cache is write-through: the
 *  master CPU reads the measures of the slave CPU through the cache-through address, see __jo_profile_end_frame()). Each CPU
 *  resets its own maximums (the slave CPU at the start of its next job, see __jo_profile_slave_job())
 */
static  __jo_force_inline jo_profile_cpu *__jo_profile_current_cpu(void)
{
    unsigned int                    sp;

    __asm__ volatile ("mov r15, %0" : "=r" (sp));
    return (&__jo_profile_cpu[(sp & 0x000FFFFF) < 0x4000 ? 1 : 0]);
}

/** @brief Add a value to a measure (Internal usage)
 *  @param cpu The measures of the current CPU (its cached address) or a cache-through address
 */
static  __jo_force_inline void      __jo_profile_add(jo_profile_cpu * const cpu, const jo_profile_slot slot, const unsigned int value)
{
    // (through the cache: a read without the cache cost about 10 cycles for each measure, several per object drawn)
    cpu->sum[slot] += value;
    if (value > cpu->max[slot])
        cpu->max[slot] = value;
}

/** @brief Start a measure on the current CPU
 *  @param slot Measure (JO_PROFILE_USER(N) for the program)
 *  @remarks Each CPU has its own measures: the 2 CPUs can measure the same slot at the same time (the times are added)
 */
static  __jo_force_inline void      jo_profile_begin(const jo_profile_slot slot)
{
    __jo_profile_current_cpu()->start[slot] = (unsigned short)jo_profile_ticks();
}

/** @brief End a measure started by jo_profile_begin() on the same CPU
 *  @param slot Measure
 *  @remarks The measure can be done several times in a frame: the times are added
 */
static  __jo_force_inline void      jo_profile_end(const jo_profile_slot slot)
{
    jo_profile_cpu                  *cpu;

    cpu = __jo_profile_current_cpu();
    __jo_profile_add(cpu, slot, (jo_profile_ticks() - cpu->start[slot]) & 0xFFFF);
}

/** @brief Add a count to a measure (commands, polygons, etc)
 *  @param slot Measure
 *  @param value Count
 */
static  __jo_force_inline void      jo_profile_count(const jo_profile_slot slot, const unsigned int value)
{
    __jo_profile_add(__jo_profile_current_cpu(), slot, value);
}

/** @brief Get the average and the maximum of a measure
 *  @param slot Measure
 *  @param average Average per frame (microseconds, or count for the counts)
 *  @param maximum Maximum of a measure (microseconds, or count)
 *  @return false if the measure was never done
 *  @remarks The values change every JO_PROFILE_FRAMES frames
 */
bool                                jo_profile_get(const jo_profile_slot slot, unsigned int *average, unsigned int *maximum);

/** @brief Ticks of the last drawing of the VDP1 (from the frame buffer change to the end of the drawing, see
 *  jo_profile_ticks(); JO_PROFILE_VDP1_DRAW sums the drawings as they end: read between 2 frames, the sum of a frame
 *  held 0 or 2 drawings, a frame of 20 ms looked like a frame of 42 ms)
 *  @return Ticks (0: none yet)
 */
unsigned int                        jo_profile_vdp1_last(void);

/** @brief Count the pixels drawn by the VDP1 for the 3D polygons (JO_PROFILE_3D_PIXELS)
 *  @param enabled true to count them (about 2 microseconds more per polygon), false by default
 */
void                                jo_profile_count_pixels(const bool enabled);
/** @brief The pixels of the polygons are counted (see jo_profile_count_pixels()) */
extern bool                         __jo_profile_pixels;

/** @brief Give a name to a measure of the program (shown by jo_profile_print())
 *  @param slot Measure (JO_PROFILE_USER(N))
 *  @param name Name (8 characters at most are shown)
 */
void                                jo_profile_set_name(const jo_profile_slot slot, const char * const name);

/** @brief Print the measures with jo_printf(): average and maximum per frame
 *  @param x Column
 *  @param y Line (one line per measure, 21 lines at most)
 *  @remarks The text changes every JO_PROFILE_FRAMES frames (jo_printf() is slow: about 0.25 ms per line)
 *  @remarks Requires the printf support
 */
void                                jo_profile_print(const int x, const int y);

/** @brief Program counter sampling: first address of the code */
# define JO_PROFILE_SAMPLING_CODE_START     (0x06004000)
/** @brief Program counter sampling: bytes of code per counter */
# define JO_PROFILE_SAMPLING_BIN_SIZE       (16)
/** @brief Program counter sampling: counters per histogram (the last one counts the addresses beyond the code; fewer for
 *  a program with less code and little memory left for the histograms) */
# ifndef JO_PROFILE_SAMPLING_BINS
#  define JO_PROFILE_SAMPLING_BINS          (16384)
# endif

/** @brief Stop the sampling of the program counter of the 2 CPUs (the histograms stay as they are: a measure over a
 *  known part of the program, the same from one run to the next) */
void                                        jo_profile_sampling_stop(void);

/** @brief Start sampling the program counter of the 2 CPUs: every JO_PROFILE_SAMPLING_INTERVAL ticks of its free running timer
 *  (about 17 000 times per second), each CPU adds 1 to the counter of the code address it executes
 *  @param master_histogram JO_PROFILE_SAMPLING_BINS counters for the master CPU (zeroed here)
 *  @param slave_histogram JO_PROFILE_SAMPLING_BINS counters for the slave CPU (zeroed here, JO_NULL: not sampled)
 *  @remarks The histograms are read in an emulator save state (tools/pcprof_report.py), they cost about 3 % of CPU
 *  @remarks The VBLANK handler of the master CPU is not sampled (same interrupt level)
 *  @remarks The slave CPU is sampled from its next job (the master CPU gives it the handler with jo_core_exec_on_slave())
 */
void                                jo_profile_sampling_start(unsigned int *master_histogram, unsigned int *slave_histogram);

/** @brief A job of the slave CPU starts (Internal usage, called by the slave CPU): its maximums are reset if the master CPU
 *  asked for it (see __jo_profile_end_frame()) */
void                                __jo_profile_slave_job(void);

/** @brief Initialize the profiler (Internal usage, called by jo_core_init()) */
void                                __jo_profile_init(void);

/** @brief End of a frame (Internal usage, called by jo_core_run()) */
void                                __jo_profile_end_frame(void);

/** @brief The VDP1 starts drawing a new command table (Internal usage, called during the VBLANK) */
void                                __jo_profile_vdp1_start(void);

#else

# define jo_profile_begin(SLOT)                 do {} while (0)
# define jo_profile_end(SLOT)                   do {} while (0)
# define jo_profile_count(SLOT, VALUE)          do {} while (0)
# define jo_profile_set_name(SLOT, NAME)        do {} while (0)
# define jo_profile_print(X, Y)                 do {} while (0)
# define jo_profile_count_pixels(ENABLED)       do {} while (0)
# define jo_profile_sampling_start(M, S)        do {} while (0)
# define __jo_profile_slave_job()               do {} while (0)

#endif /* !JO_COMPILE_WITH_PROFILER_SUPPORT */

#endif /* !__JO_PROFILE_H__ */

/*
** END OF FILE
*/
