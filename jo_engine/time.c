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
** INCLUDES
*/
#include <stdbool.h>
#include "jo/conf.h"
#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/time.h"
#include "jo/core.h"
#include "jo/time.h"
#include "internal/core.h"

void                                    jo_time_init(unsigned char mode)
{
    unsigned char                       tcr;

    tcr = jo_time_peek_byte(RegisterTCR) & ~JO_TIME_M_CKS;
    if ((mode & JO_TIME_M_CKS) != JO_TIME_M_CKS)
    {
        tcr |= (mode & JO_TIME_M_CKS);
        jo_time_poke_byte(RegisterTCR, tcr);
    }
    jo_time_set_frc(0);
}

/** @brief Seconds (16.16) of a clock of the CPU x 2^40: the microseconds of a clock (0.037470726 and 0.035164835 in PAL,
 *  0.037210548 and 0.03492059 in NTSC, 26.8 and 28.6 MHz) x 65536 / 1000000 x 2^40 (computed by floats before: the floats
 *  of the library of the compiler at each call of jo_fixed_point_time()) */
static  __jo_force_inline unsigned int  __jo_time_get_clock_seconds(void)
{
    return (((*(unsigned short *)0x25f80004 & 0x1) == 0x1) ?
             ((jo_time_peek_int(RegisterSysClock) == 0) ? 2700050362u : 2533893405u) :
             ((jo_time_peek_int(RegisterSysClock) == 0) ? 2681302562u : 2516293698u));
}

//	Determine if clock is on 1/8, 1/32, or 1/128 of count
static  __jo_force_inline unsigned int  __jo_time_get_clock_mode(void)
{
    return (8 << ((jo_time_peek_byte(RegisterTCR) & 3) << 1));
}

/** @brief Microseconds of clocks of the CPU (the microseconds of a clock, see __jo_time_get_clock_seconds(), x 2^20: without float, this runs at
 *  each VBLANK, see __jo_time_vblank()) */
static  __jo_force_inline unsigned int  __jo_time_clocks_to_mcr(const unsigned int clocks)
{
    unsigned int                        speed;

    speed = ((*(unsigned short *)0x25f80004 & 0x1) == 0x1) ?
            ((jo_time_peek_int(RegisterSysClock) == 0) ? 39291 : 36873) :
            ((jo_time_peek_int(RegisterSysClock) == 0) ? 39018 : 36617);
    // (64 bits product: dmulu.l)
    return ((unsigned int)(((unsigned long long)clocks * speed) >> 20));
}

static  __jo_force_inline int           __jo_time_frame_count_to_mcr(int count)
{
    return ((int)__jo_time_clocks_to_mcr((unsigned int)count * __jo_time_get_clock_mode()));
}

/** @brief Free running timer ticks since the previous call
 *  @param previous Value of the timer at the previous call (updated)
 *  @return Ticks
 *  @remarks The timer is not reset: it is also used by the profiler (see jo_profile_begin())
 */
static  __jo_force_inline int           jo_time_elapsed_frc(unsigned short *previous)
{
    unsigned short                      now;
    unsigned short                      elapsed;

    now = (unsigned short)jo_time_get_frc();
    elapsed = now - *previous;
    *previous = now;
    return (elapsed);
}

static unsigned int                     __jo_time_ticks = 0;
static unsigned int                     __jo_time_microseconds = 0;
static unsigned short                   __jo_time_previous = 0;

/** @brief Count the time since the last call (the free running counter wraps after about 78 ms: counted at each VBLANK
 *  too, jo_get_ticks() called once a second lost most of the time) */
static void                             __jo_time_update_ticks(void)
{
    // (the microseconds under a millisecond are kept for the next call: called every 500 us, it advanced never)
    __jo_time_microseconds += __jo_time_frame_count_to_mcr(jo_time_elapsed_frc(&__jo_time_previous));
    __jo_time_ticks += __jo_time_microseconds / 1000;
    __jo_time_microseconds %= 1000;
}

void                                    __jo_time_vblank(void)
{
    __jo_time_update_ticks();
}

unsigned int                            jo_get_ticks(void)
{
    unsigned int                        sr;
    unsigned int                        ticks;

    // (the VBLANK interrupt counts too)
    sr = __jo_core_disable_interrupts();
    __jo_time_update_ticks();
    ticks = __jo_time_ticks;
    __jo_core_restore_interrupts(sr);
    return (ticks);
}

/*
  Contributed by Ponut64: https://github.com/johannes-fetz/joengine/pull/21 & Emerald Nova
*/

static jo_fixed oldtime = 0;
jo_fixed time_in_seconds = 0;
jo_fixed delta_time = 0;
static unsigned short previous_frc = 0;

void                                    jo_fixed_point_time(void)
{
	oldtime = time_in_seconds;
	jo_fixed time_add = (jo_fixed)(((unsigned long long)((unsigned int)jo_time_elapsed_frc(&previous_frc) * __jo_time_get_clock_mode()) *
                                    __jo_time_get_clock_seconds()) >> 40);
	time_in_seconds += time_add;
	delta_time = time_in_seconds - oldtime;
}

/*
** END OF FILE
*/
