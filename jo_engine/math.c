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

jo_fixed            jo_fixed_pow(jo_fixed x, jo_fixed y)
{
    jo_fixed        result;
    jo_fixed        n;

    n = JO_DIV_BY_65536(y);
    if (n == 0)
        return (JO_FIXED_1);
    if (n == JO_FIXED_1)
        return (x);
    for (result = JO_FIXED_1; n >= 0; --n)
    {
        result = jo_fixed_mult(x, result);
        if (result == (jo_fixed)JO_FIXED_OVERFLOW)
            return (result);
    }
    return (result);
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
	"exts.w r1, r1;" //Sign extension in case value is negative
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

/*
██████╗  █████╗ ███╗   ██╗██████╗  ██████╗ ███╗   ███╗
██╔══██╗██╔══██╗████╗  ██║██╔══██╗██╔═══██╗████╗ ████║
██████╔╝███████║██╔██╗ ██║██║  ██║██║   ██║██╔████╔██║
██╔══██╗██╔══██║██║╚██╗██║██║  ██║██║   ██║██║╚██╔╝██║
██║  ██║██║  ██║██║ ╚████║██████╔╝╚██████╔╝██║ ╚═╝ ██║
╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═════╝  ╚═════╝ ╚═╝     ╚═╝
*/

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
██╗   ██╗███████╗ ██████╗████████╗ ██████╗ ██████╗
██║   ██║██╔════╝██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗
██║   ██║█████╗  ██║        ██║   ██║   ██║██████╔╝
╚██╗ ██╔╝██╔══╝  ██║        ██║   ██║   ██║██╔══██╗
 ╚████╔╝ ███████╗╚██████╗   ██║   ╚██████╔╝██║  ██║
  ╚═══╝  ╚══════╝ ╚═════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝
*/

void            jo_vector_fixed_compute_bezier_point(const jo_fixed t, jo_vector_fixed p0, jo_vector_fixed p1, jo_vector_fixed p2, jo_vector_fixed p3, jo_vector_fixed *result)
{
    jo_fixed    tt = jo_fixed_mult(t, t);
    jo_fixed    u = JO_FIXED_1 - t;
    jo_fixed    uu = jo_fixed_mult(u, u);

    // first term
    jo_vector_fixed_muls(&p0, jo_fixed_mult(uu, u), result);
    // second term
    jo_vector_fixed_muls(&p1, jo_fixed_mult(196608, jo_fixed_mult(uu, t)), &p1);
    jo_vector_fixed_add(result, &p1, result);
    // third term
    jo_vector_fixed_muls(&p2, jo_fixed_mult(196608, jo_fixed_mult(u, tt)), &p2);
    jo_vector_fixed_add(result, &p2, result);
    // fourth term
    jo_vector_fixed_muls(&p3, jo_fixed_mult(tt, t), &p3);
    jo_vector_fixed_add(result, &p3, result);
}

void            jo_vectorf_compute_bezier_point(const float t, jo_vectorf p0, jo_vectorf p1, jo_vectorf p2, jo_vectorf p3, jo_vectorf *result)
{
    float       tt = t * t;
    float       u = 1.0f - t;
    float       uu = u * u;

    // first term
    jo_vectorf_muls(&p0, uu * u, result);
    // second term
    jo_vectorf_muls(&p1, 3 * uu * t, &p1);
    jo_vectorf_add(result, &p1, result);
    // third term
    jo_vectorf_muls(&p2, 3 * u * tt, &p2);
    jo_vectorf_add(result, &p2, result);
    // fourth term
    jo_vectorf_muls(&p3, tt * t, &p3);
    jo_vectorf_add(result, &p3, result);
}

/*
███████╗ ██████╗ ██████╗ ████████╗
██╔════╝██╔═══██╗██╔══██╗╚══██╔══╝
███████╗██║   ██║██████╔╝   ██║
╚════██║██║▄▄ ██║██╔══██╗   ██║
███████║╚██████╔╝██║  ██║   ██║
╚══════╝ ╚══▀▀═╝ ╚═╝  ╚═╝   ╚═╝
*/

/*
** Based on http://en.wikipedia.org/wiki/Methods_of_computing_square_roots
*/
jo_fixed                jo_fixed_sqrt(jo_fixed value)
{
    unsigned int        bit;
    unsigned char       n;
    unsigned int        result;

    if (value < 0)
        return (JO_FIXED_OVERFLOW);
    bit = (value & 0xFFF00000)? (unsigned int)1 << 30 : (unsigned int)1 << 18;
    while (bit > (unsigned int)value)
        bit >>= 2;
    JO_ZERO(result);
    for (n = 0u; n < 2u; ++n)
    {
        while (bit)
        {
            if (result + bit < (unsigned int)value)
            {
                value -= result + bit;
                result = (result >> 1) + bit;
            }
            else
                result = (result >> 1);
            bit >>= 2;
        }
        if (n != 0u)
            continue;
        if (value > 65535)
        {
            value -= result;
            value = (value << 16) - JO_FIXED_1_DIV_2;
            result = (result << 16) + JO_FIXED_1_DIV_2;
        }
        else
        {
            value <<= 16;
            result <<= 16;
        }
        bit = (1 << 14);
    }
    return ((jo_fixed)result);
}

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

/*
██╗███╗   ██╗██╗   ██╗███████╗██████╗ ███████╗███████╗    ███████╗ ██████╗ ██████╗ ████████╗
██║████╗  ██║██║   ██║██╔════╝██╔══██╗██╔════╝██╔════╝    ██╔════╝██╔═══██╗██╔══██╗╚══██╔══╝
██║██╔██╗ ██║██║   ██║█████╗  ██████╔╝███████╗█████╗      ███████╗██║   ██║██████╔╝   ██║
██║██║╚██╗██║╚██╗ ██╔╝██╔══╝  ██╔══██╗╚════██║██╔══╝      ╚════██║██║▄▄ ██║██╔══██╗   ██║
██║██║ ╚████║ ╚████╔╝ ███████╗██║  ██║███████║███████╗    ███████║╚██████╔╝██║  ██║   ██║
╚═╝╚═╝  ╚═══╝  ╚═══╝  ╚══════╝╚═╝  ╚═╝╚══════╝╚══════╝    ╚══════╝ ╚══▀▀═╝ ╚═╝  ╚═╝   ╚═╝
AKA RECIPROCAL SQUARE ROOT
*/

/*
** Based on trenki2 implementation and adapted for the Jo Engine
*/

static __jo_force_inline unsigned int   __jo_fixed_inv_sqrt_leading_zeros(unsigned int x)
{
    unsigned int                        exp = 31;

    if (x & 0xffff0000)
    {
        exp -= 16;
        x >>= 16;
    }
    if (x & 0xff00)
    {
        exp -= 8;
        x >>= 8;
    }
    if (x & 0xf0)
    {
        exp -= 4;
        x >>= 4;
    }
    if (x & 0xc)
    {
        exp -= 2;
        x >>= 2;
    }
    if (x & 0x2)
        exp -= 1;
    return (exp);
}

jo_fixed        jo_fixed_rsqrt(jo_fixed value)
{
    static const unsigned short rsq_tab[] =
    {
        /* domain 0.5 .. 1.0-1/16 */
		0xb504, 0xaaaa, 0xa1e8, 0x9a5f, 0x93cd, 0x8e00, 0x88d6, 0x8432,
    };
    jo_fixed    result;
    int         i;
    int         exp;

    if (value == 0)
        return (JO_FIXED_MAX);
    if (value == JO_FIXED_1)
        return (value);
    exp = __jo_fixed_inv_sqrt_leading_zeros(value);
    result = rsq_tab[(value >> (28 - exp)) & 0x7] << 1;
	exp -= 16;
    if (exp <= 0)
		result >>= -exp >> 1;
    else
		result <<= (exp >> 1) + (exp & 1);
    if (exp & 1)
        result = jo_fixed_mult(result, rsq_tab[0]);
    /* newton-raphson : x = x/2*(3-(a*x)*x) */
    i = 0;
    do
    {
		result = jo_fixed_mult((result >> 1), (JO_FIXED_1 * 3 - jo_fixed_mult(jo_fixed_mult(value, result), result)));
    } while(++i < 3);
    return (result);
}

/*
** Wikipedia implementation : https://en.wikipedia.org/wiki/Fast_inverse_square_root
*/
float               jo_rsqrt(float value)
{
    int             i;
    float           x2;
    float           y;
    const float     threehalfs = 1.5f;

    x2 = value * 0.5f;
    y = value;
    i = *(int *)&y;
    i = 0x5f3759df - (i >> 1);
    y = *(float *)&i;
    y = y * (threehalfs - (x2 * y * y));
    y = y * (threehalfs - (x2 * y * y));
    return (y);
}

/*
** END OF FILE
*/
