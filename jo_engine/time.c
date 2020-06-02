/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2020, Johannes Fetz (johannesfetz@gmail.com)
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
#include "jo/sgl_prototypes.h"
#include "jo/conf.h"
#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/time.h"
#include "jo/core.h"
#include "jo/time.h"

void        jo_time_init(unsigned char mode)
{
    unsigned char tcr;

    tcr = jo_time_peek_byte(RegisterTCR) & ~JO_TIME_M_CKS;
    if ((mode & JO_TIME_M_CKS) != JO_TIME_M_CKS)
    {
        tcr |= (mode & JO_TIME_M_CKS);
        jo_time_poke_byte(RegisterTCR, tcr);
    }
    jo_time_set_frc(0);
}

static  __jo_force_inline int jo_time_frame_count_to_mcr(int count)
{
    return ((((*(Uint16 *)0x25f80004 & 0x1) == 0x1) ?
             ((jo_time_get_sys_clock_value() == 0) ? (float)0.037470726 : (float)0.035164835 ) :
             ((jo_time_get_sys_clock_value() == 0) ? (float)0.037210548 : (float)0.03492059 ))
            * (count) * (8 << ((jo_time_peek_byte(RegisterTCR) & JO_TIME_M_CKS) << 1)));
}

unsigned int                jo_get_ticks(void)
{
    static unsigned int     ticks = 0;

    ticks += jo_time_frame_count_to_mcr(jo_time_get_frc()) / 1000;
    jo_time_set_frc(0);
    return (ticks);
}

/**Work by Emerald Nova**/
// Pointer to SH2 Registers, found by Johannes Fetz, contributed by Ponut64
// High Free Running Counter Register (FCR), counts up to 255, then iterates FCR Low
Uint8 * SH2FRCHigh = (Uint8 *)0xfffffe12;
// Low Free Running Counter Register (FCR), increases every time FCR high reaches 256
Uint8 * SH2FRCLow = (Uint8 *)0xfffffe13;
// Time Control Register (TCR)
Uint8 * SH2TCR = (Uint8 *)0xfffffe16;
// System clock
unsigned int * SysClockReg = (unsigned int*)0x6000324;
//	Time tracking in seconds
jo_fixed time_in_seconds = 0;
jo_fixed oldtime = 0;
jo_fixed delta_time = 0;

//	JO Engine sourced timer adaptation
//	System variable "Time" will count up in seconds if the function "timer" is run.
//	System variable "delta_time" will express the delta time between frames, in fixed-point seconds.
void jo_fixed_point_time(void)
{
	//	Set old time for iteration
	oldtime = time_in_seconds;
	
	// Get System Clock Value
	unsigned int SysClock = (*(unsigned int*)0x6000324);
	
	
	jo_fixed time_add =
		(
		//	Clock speed from framerate based on resolution register and NSTC vs PAL
		(((*(Uint16 *)0x25f80004 & 0x1) == 0x1) ?
			((SysClock == 0) ? (float)0.037470726 : (float)0.035164835 ) :
			((SysClock == 0) ? (float)0.037210548 : (float)0.03492059 ))
			*
		//	Get Free Running Counter value
		((*(unsigned char *)SH2FRCHigh) << 8 | (*(unsigned char *)SH2FRCLow))
		*
		//	Determine if clock is on 1/8, 1/32, or 1/128 of count
		(8 << (((*(unsigned char *)RegisterTCR) & 3) << 1)) /
		//	Set to s
		1000000) * (65536.0);

	//	Set new time_in_seconds
	time_in_seconds += time_add;
	delta_time = time_in_seconds - oldtime;
	
	//	Reset FRC's
	(*(unsigned char *)SH2FRCHigh) = 0 >> 8;
	(*(unsigned char *)SH2FRCLow) = 0;
}


/*
** END OF FILE
*/
