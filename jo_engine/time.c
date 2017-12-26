/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2017, Johannes Fetz (johannesfetz@gmail.com)
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

/*
** END OF FILE
*/
