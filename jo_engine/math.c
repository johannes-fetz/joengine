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
#include "jo/tools.h"
#include "jo/math.h"

/*
** FIXED Q16.16 Number
*/

/*
** Thanks Ponut64 for optimized this method !
**
** Note: -O2 optimisation is temporarily disabled for this function because it cause unexpected behaviour
**
** History:
**
**  - https://github.com/johannes-fetz/joengine/pull/15
**  - https://github.com/johannes-fetz/joengine/pull/17
**  - https://github.com/johannes-fetz/joengine/pull/19
**
*/
#pragma GCC push_options
#pragma GCC optimize ("Os")

jo_fixed                jo_fixed_mult(jo_fixed x, jo_fixed y)
{
	register jo_fixed rtval;
	asm(
	"dmuls.l %[d1],%[d2];" // `dmuls.l % , %` -- dmuls.l being double-word multplication, inputs as longword from variable register (%)
	"sts MACH,r1;"		// `sts` - store system register MACH at explicit general-purpose register r1
	"sts MACL,%[out];"	// `sts` - store system register MACL at variable register (%)
	"xtrct r1,%[out];" 	// `xtrct gpr0,gpr1` - extract the lower 16-bits of gpr0 (r1) and upper 16-bits of gpr1 (%), result to gpr1 (%)
    :    [out] "=r" (rtval)       		 //OUT
    :    [d1] "r" (x), [d2] "r" (y)    //IN
	:		"r1"						//CLOBBERS
	);
	return rtval;
}


jo_fixed	jo_fixed_dot(jo_fixed ptA[3], jo_fixed ptB[3]) //This can cause illegal instruction execution... I wonder why... fxm does not
{
	register jo_fixed rtval;
	asm(
		"clrmac;"
		"mac.l @%[ptr1]+,@%[ptr2]+;"
		"mac.l @%[ptr1]+,@%[ptr2]+;"
		"mac.l @%[ptr1]+,@%[ptr2]+;" //Expresses (ptA[X] * ptB[X]) + (ptA[Y] * ptB[Y]) + (ptA[Z] * ptB[Z])
		"sts MACH,r1;"
		"sts MACL,%[ox];"
		"xtrct r1,%[ox];"			//Extracts to fixed-point
		: 	[ox] "=r" (rtval)											//OUT
		:	[ptr1] "r" (ptA) , [ptr2] "r" (ptB)							//IN
		:	"r1"														//CLOBBERS
	);
	return rtval;
}


jo_fixed	jo_fixed_div(jo_fixed dividend, jo_fixed divisor)
{
	
const int * DVSR = ( int*)0xFFFFFF00;
const int * DVDNTH = ( int*)0xFFFFFF10;
const int * DVDNTL = ( int*)0xFFFFFF14;

/*
SH7604 Note
Saturn special CPU has a division unit. We use it here.
Our assembler is not aware of its existence, so we must address it via pointers.
When a value is placed in DVDNTL register, (64 bit / 32 bit) division begins,
 with DVDNTH and DVDNTL represening the high and low 32 bits of the 64 bit dividend.
The divisor register (DVSR) is just 32-bit.

Now, this *should* take 39 cycles to complete. It appears the SH7604 will just wait if you try and access it early.
But check with real hardware first, you know?
*/

register jo_fixed quotient;
	asm(
	"mov.l %[dvs], @%[dvsr];"
	"mov %[dvd], r1;" //Move the dividend to a general-purpose register, to prevent weird misreading of data.
	"shlr16 r1;"
	"mov.l r1, @%[nth];" //Expresses "*DVDNTH = dividend>>16"
	"mov %[dvd], r1;" 
	"shll16 r1;"
	"mov.l r1, @%[ntl];" //Expresses *DVDNTL = dividend<<16";
	"mov.l @%[ntl], %[out];" //Get result.
		: 	[out] "=r" (quotient)											//OUT
		:	[dvs] "r" (divisor) , [dvd] "r" (dividend) ,					//IN
			[dvsr] "r" (DVSR) ,	[nth] "r" (DVDNTH) , [ntl] "r" (DVDNTL)		//IN
		:	"r1"															//CLOBBERS
	);
	return quotient;
}


#pragma GCC pop_options

/* Taylor series approximation */
jo_fixed                jo_fixed_sin(jo_fixed rad)
{
    jo_fixed            result;
    jo_fixed            x2;

    rad = jo_fixed_wrap_to_pi(rad);
    x2 = jo_fixed_mult(rad ,rad);

    result = rad;
    rad = jo_fixed_mult(rad, x2);
    result -= (rad / 6);
    rad = jo_fixed_mult(rad, x2);
    result += (rad / 120);
    rad = jo_fixed_mult(rad, x2);
    result -= (rad / 5040);
    rad = jo_fixed_mult(rad, x2);
    result += (rad / 362880);
    rad = jo_fixed_mult(rad, x2);
    result -= (rad / 39916800);

    return (result);
}

/*
** OTHER
*/

unsigned int        jo_sqrt(unsigned int value)
{
    unsigned int    start;
    unsigned int    end;
    unsigned int    res;
    unsigned int    mid;

    if (value == 0 || value == 1)
        return (value);
    JO_ZERO(res);
    start = 1;
    end = value;
    while (start <= end)
    {
        mid = JO_DIV_BY_2(start + end);
        if (mid * mid == value)
            return (mid);
        if (mid * mid < value)
        {
            start = mid + 1;
            res = mid;
        }
        else
            end = mid - 1;
    }
    return (res);
}

int     jo_gcd(int a, int b)
{
#ifdef JO_DEBUG
    if (a <= 0)
    {
        jo_core_error("a <= 0 in jo_gcd()");
        return (-1);
    }
    if (b <= 0)
    {
        jo_core_error("b <= 0 in jo_gcd()");
        return (-1);
    }
#endif
    while (a != b)
    {
        if (a > b)
            a -= b;
        else
            b -= a;
    }
    return (a);
}

float       jo_atan2f_rad(const float y, const float x)
{
    float   atan;
    float   z;

    if (JO_IS_FLOAT_NULL(x))
    {
        if (y > 0.0f)
            return (JO_PI_2);
        if (JO_IS_FLOAT_NULL(y))
            return 0.0f;
        return (-JO_PI_2);
    }
    z = y / x;
    if (JO_FABS(z) < 1.0f)
    {
        atan = z / (1.0f + 0.28f * z * z);
        if (x < 0.0f)
        {
            if (y < 0.0f)
                return (atan - JO_PI);
            return (atan + JO_PI);
        }
    }
    else
    {
        atan = JO_PI_2 - z / (z * z + 0.28f);
        if (y < 0.0f)
            return (atan - JO_PI);
    }
    return (atan);
}

void                        jo_planar_rotate(const jo_pos2D * const point, const jo_pos2D * const origin, const int angle, jo_pos2D * const result)
{
    register int            dx;
    register int            dy;
    register jo_fixed       cos_theta;
    register jo_fixed       sin_theta;

    dx = point->x - origin->x;
    dy = point->y - origin->y;
    cos_theta = jo_cos(angle);
    sin_theta = jo_sin(angle);
    result->x = (jo_fixed2int(dx * cos_theta) - jo_fixed2int(dy * sin_theta)) + origin->x;
    result->y = (jo_fixed2int(dy * cos_theta) + jo_fixed2int(dx * sin_theta)) + origin->y;
}

/* single phase linear congruential random integer generator */

#define JO_RANDOM_M             (2147483647)
#define JO_RANDOM_A             (16807)
#define JO_RANDOM_Q             (JO_RANDOM_M / JO_RANDOM_A)
#define JO_RANDOM_R             (JO_RANDOM_M % JO_RANDOM_A)

int                             jo_random_seed = 1;

int                             jo_random(int max)
{
    jo_random_seed = JO_RANDOM_A * (jo_random_seed % JO_RANDOM_Q) - JO_RANDOM_R * (jo_random_seed / JO_RANDOM_Q);
    if (jo_random_seed <= 0)
        jo_random_seed += JO_RANDOM_M;
    return jo_random_seed % max + 1;
}

/*
** END OF FILE
*/
