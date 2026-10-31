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
/** @file math.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Math tools
 *  @bug Possible issue with jo_sin() and jo_cos() with some angle.
 */

#ifndef __JO_MATH_H__
# define __JO_MATH_H__

/*
███╗   ███╗ █████╗ ████████╗██╗  ██╗     ██████╗ ██████╗ ███╗   ██╗███████╗████████╗███████╗
████╗ ████║██╔══██╗╚══██╔══╝██║  ██║    ██╔════╝██╔═══██╗████╗  ██║██╔════╝╚══██╔══╝██╔════╝
██╔████╔██║███████║   ██║   ███████║    ██║     ██║   ██║██╔██╗ ██║███████╗   ██║   ███████╗
██║╚██╔╝██║██╔══██║   ██║   ██╔══██║    ██║     ██║   ██║██║╚██╗██║╚════██║   ██║   ╚════██║
██║ ╚═╝ ██║██║  ██║   ██║   ██║  ██║    ╚██████╗╚██████╔╝██║ ╚████║███████║   ██║   ███████║
╚═╝     ╚═╝╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝     ╚═════╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝   ╚═╝   ╚══════╝
*/

/** @brief PI value
 *  @remarks float constant: a double constant made the whole expression use the double precision, even slower than
 *  the float on the SH-2 (no FPU, both are computed by libgcc)
 */
# define JO_PI                      (3.1415927f)

/** @brief PI/2 value */
# define JO_PI_2                    (1.5707963f)

/*
███╗   ███╗██╗   ██╗██╗  ████████╗ ██╗██████╗ ██╗██╗   ██╗
████╗ ████║██║   ██║██║  ╚══██╔══╝██╔╝██╔══██╗██║██║   ██║
██╔████╔██║██║   ██║██║     ██║  ██╔╝ ██║  ██║██║██║   ██║
██║╚██╔╝██║██║   ██║██║     ██║ ██╔╝  ██║  ██║██║╚██╗ ██╔╝
██║ ╚═╝ ██║╚██████╔╝███████╗██║██╔╝   ██████╔╝██║ ╚████╔╝
╚═╝     ╚═╝ ╚═════╝ ╚══════╝╚═╝╚═╝    ╚═════╝ ╚═╝  ╚═══╝
*/

/** @brief Multiply a variable by 2
 *  @remarks faster than X * 2
 *  @param X Variable name
 */
# define JO_MULT_BY_2(X)			((X) << 1)

/** @brief Multiply a variable by 4
 *  @remarks faster than X * 4
 *  @param X Variable name
 */
# define JO_MULT_BY_4(X)			((X) << 2)

/** @brief Multiply a variable by 8
 *  @remarks faster than X * 8
 *  @param X Variable name
 */
# define JO_MULT_BY_8(X)			((X) << 3)

/** @brief Multiply a variable by 16
 *  @remarks faster than X * 16
 *  @param X Variable name
 */
# define JO_MULT_BY_16(X)			((X) << 4)

/** @brief Multiply a variable by 32
 *  @remarks faster than X * 32
 *  @param X Variable name
 */
# define JO_MULT_BY_32(X)			((X) << 5)

/** @brief Multiply a variable by 64
 *  @remarks faster than X * 64
 *  @param X Variable name
 */
# define JO_MULT_BY_64(X)			((X) << 6)

/** @brief Multiply a variable by 128
 *  @remarks faster than X * 128
 *  @param X Variable name
 */
# define JO_MULT_BY_128(X)			((X) << 7)

/** @brief Multiply a variable by 256
 *  @remarks faster than X * 256
 *  @param X Variable name
 */
# define JO_MULT_BY_256(X)			((X) << 8)

/** @brief Multiply a variable by 512
 *  @remarks faster than X * 512
 *  @param X Variable name
 */
# define JO_MULT_BY_512(X)			((X) << 9)

/** @brief Multiply a variable by 1024
 *  @remarks faster than X * 1024
 *  @param X Variable name
 */
# define JO_MULT_BY_1024(X)			((X) << 10)

/** @brief Multiply a variable by 2048
 *  @remarks faster than X * 2048
 *  @param X Variable name
 */
# define JO_MULT_BY_2048(X)			((X) << 11)

/** @brief Multiply a variable by 4096
 *  @remarks faster than X * 4096
 *  @param X Variable name
 */
# define JO_MULT_BY_4096(X)			((X) << 12)

/** @brief Multiply a variable by 32768
 *  @remarks faster than X * 32768
 *  @param X Variable name
 */
# define JO_MULT_BY_32768(X)			((X) << 15)

/** @brief Multiply a variable by 65536
 *  @remarks faster than X * 65536
 *  @param X Variable name
 */
# define JO_MULT_BY_65536(X)			((X) << 16)

/** @brief Devide a variable by 2
 *  @remarks faster than X / 2
 *  @param X Variable name
 */
# define JO_DIV_BY_2(X)				((X) >> 1)

/** @brief Devide a variable by 4
 *  @remarks faster than X / 4
 *  @param X Variable name
 */
# define JO_DIV_BY_4(X)				((X) >> 2)

/** @brief Devide a variable by 8
 *  @remarks faster than X / 8
 *  @param X Variable name
 */
# define JO_DIV_BY_8(X)				((X) >> 3)

/** @brief Devide a variable by 16
 *  @remarks faster than X / 16
 *  @param X Variable name
 */
# define JO_DIV_BY_16(X)			((X) >> 4)

/** @brief Devide a variable by 32
 *  @remarks faster than X / 32
 *  @param X Variable name
 */
# define JO_DIV_BY_32(X)			((X) >> 5)

/** @brief Devide a variable by 64
 *  @remarks faster than X / 64
 *  @param X Variable name
 */
# define JO_DIV_BY_64(X)			((X) >> 6)

/** @brief Devide a variable by 128
 *  @remarks faster than X / 128
 *  @param X Variable name
 */
# define JO_DIV_BY_128(X)			((X) >> 7)

/** @brief Devide a variable by 256
 *  @remarks faster than X / 256
 *  @param X Variable name
 */
# define JO_DIV_BY_256(X)			((X) >> 8)

/** @brief Devide a variable by 512
 *  @remarks faster than X / 512
 *  @param X Variable name
 */
# define JO_DIV_BY_512(X)			((X) >> 9)

/** @brief Devide a variable by 1024
 *  @remarks faster than X / 1024
 *  @param X Variable name
 */
# define JO_DIV_BY_1024(X)			((X) >> 10)

/** @brief Devide a variable by 32768
 *  @remarks faster than X / 32768
 *  @param X Variable name
 */
# define JO_DIV_BY_32768(X)			((X) >> 15)

/** @brief Devide a variable by 65536
 *  @remarks faster than X / 65536
 *  @param X Variable name
 */
# define JO_DIV_BY_65536(X)			((X) >> 16)

/** @brief Devide a variable by 2147483648
 *  @remarks faster than X / 2147483648
 *  @param X Variable name
 */
# define JO_DIV_BY_2147483648(X)    ((X) >> 31)

/*
████████╗ ██████╗  ██████╗ ██╗     ███████╗
╚══██╔══╝██╔═══██╗██╔═══██╗██║     ██╔════╝
   ██║   ██║   ██║██║   ██║██║     ███████╗
   ██║   ██║   ██║██║   ██║██║     ╚════██║
   ██║   ╚██████╔╝╚██████╔╝███████╗███████║
   ╚═╝    ╚═════╝  ╚═════╝ ╚══════╝╚══════╝
*/

/** @brief Set a variable to zero
 *  @remarks faster than X = 0
 *  @param X Variable name
 */
# define JO_ZERO(X)                 X ^= X

/** @brief Fast modulo of a power of 2
 *  @remarks faster than N % M
 *  @param N Number
 *  @param M Modulo
 */
# define JO_MOD_POW2(N, M)          ((N) & ((M) - 1))

/** @brief Fast check if N of a power of 2
 *  @param N Number
 */
# define JO_IS_POW2(N)              ((N) != 0 && (((N) & ((N) - 1)) == 0))

/** @brief Get the absolute value of X
 *  @param X Variable name
 */
# define JO_ABS(X)                  ((X) < 0 ? -(X) : (X))

/** @brief Get the absolute value of X
 *  @param X Variable name
 */
# define JO_FABS(X)                 ((X) < 0.0f ? -(X) : (X))

/** @brief Get the smaller value between A and B
 *  @param A First operand
 *  @param B Second operand
 */
# define JO_MIN(A, B)               (((A) < (B)) ? (A) : (B))

/** @brief Get the bigger value between A and B
 *  @param A First operand
 *  @param B Second operand
 */
# define JO_MAX(A, B)               (((A) > (B)) ? (A) : (B))

/** @brief Change the sign of X
 *  @param X Variable name
 */
# define JO_CHANGE_SIGN(X)          (-(X))

/** @brief Float minimum positive value */
# define JO_FLOAT_EPSILON           (0.00001f)

/** @brief Check if A and B is nearly the same value
 *  @param A First operand
 *  @param B Second operand
 */
# define JO_FLOAT_NEARLY_EQUALS(A, B)     ((A) <= ((B) + JO_FLOAT_EPSILON) && (A) >= ((B) - JO_FLOAT_EPSILON))

/** @brief Check if A nearly equals zero
 *  @param A operand
 */
# define JO_IS_FLOAT_NULL(A)              ((A) <= JO_FLOAT_EPSILON && (A) >= (-JO_FLOAT_EPSILON))

/** @brief Convert radians to degrees
 *  @param A operand
 */
# define JO_RAD_TO_DEG(A)                  ((A) * 57.295780f)

/** @brief Convert degrees to radians
 *  @param A operand
 */
# define JO_DEG_TO_RAD(A)                  ((A) * 0.017453292f)

/** @brief Check if A is odd
 *  @param A operand
 */
# define JO_IS_ODD(A)                       ((A) & 1)

/** @brief Swap A and B values
 *  @param A operand
 *  @param B operand
 *  @remarks With a temporary variable: the XOR swap gave 0 when A and B were the same variable
 */
# define JO_SWAP(A, B)                      { __typeof__((A) + 0) __jo_swap_tmp = (A); (A) = (B); (B) = __jo_swap_tmp; }

/** @brief Hide the value of a variable from the optimizer of the compiler (no instruction)
 *  @param VAR Variable (in a register)
 *  @remarks GCC replaces "x != 1 ? y / x : y" by a division in all the cases (y / 1 = y) and a product by a variable
 *           power of 2 by a shift by a variable amount: calls of the library of the compiler on the SH-2 (no division
 *           instruction, no barrel shifter). The variable hidden in the branch, or before the product, keeps the code written
 */
# define JO_OPAQUE(VAR)                     __asm__ ("" : "+r" (VAR))

/** @brief Compute percent ((TOTAL - FREE) x 100 / TOTAL, rounded down)
 *  @param TOTAL Total (42 949 672 at most, 0: 0 %)
 *  @param FREE Free
 *  @remarks Integers: the float version linked the float emulation of the compiler in every program (about 2 KB)
 */
# define JO_PERCENT_USED(TOTAL, FREE)       ((TOTAL) == 0 ? 0 : (int)((unsigned int)((TOTAL) - (FREE)) * 100u / (unsigned int)(TOTAL)))

/** @brief Square computation (x²)
 *  @param A operand
 */
# define JO_SQUARE(A)                       ((A) * (A))

/** @brief BCD to integer
 *  @param BCD BCD
 */
# define JO_BCD_INT(BCD)                    (((BCD & 0xF0) >> 4) * 10 + (BCD & 0x0F))

/*
██████╗ ██╗   ██╗████████╗███████╗███████╗██╗███████╗██╗     ██████╗
██╔══██╗╚██╗ ██╔╝╚══██╔══╝██╔════╝██╔════╝██║██╔════╝██║     ██╔══██╗
██████╔╝ ╚████╔╝    ██║   █████╗  █████╗  ██║█████╗  ██║     ██║  ██║
██╔══██╗  ╚██╔╝     ██║   ██╔══╝  ██╔══╝  ██║██╔══╝  ██║     ██║  ██║
██████╔╝   ██║      ██║   ███████╗██║     ██║███████╗███████╗██████╔╝
╚═════╝    ╚═╝      ╚═╝   ╚══════╝╚═╝     ╚═╝╚══════╝╚══════╝╚═════╝
*/

/** @brief Set flags in bytefield
 *  @param BYTEFIELD Bytefield
 *  @param FLAGS Flags
 */
# define JO_SET_FLAGS(BYTEFIELD, FLAGS)         BYTEFIELD = (FLAGS)

/** @brief Set all flags in bytefield
 *  @param BYTEFIELD Bytefield
 */
# define JO_SET_ALL_FLAGS(BYTEFIELD)            BYTEFIELD = (~0)

/** @brief Add flag in bytefield
 *  @param BYTEFIELD Bytefield
 *  @param FLAG Flag
 */
# define JO_ADD_FLAG(BYTEFIELD, FLAG)           BYTEFIELD |= (FLAG)

/** @brief Remove flag in bytefield
 *  @param BYTEFIELD Bytefield
 *  @param FLAG Flag
 */
# define JO_REMOVE_FLAG(BYTEFIELD, FLAG)        BYTEFIELD &= ~(FLAG)

/** @brief Remove all flags in bytefield
 *  @param BYTEFIELD Bytefield
 */
# define JO_REMOVE_ALL_FLAGS(BYTEFIELD)         BYTEFIELD = 0

/** @brief Check if flag is checked in bytefield
 *  @param BYTEFIELD Bytefield
 *  @param FLAG Flag
 */
# define JO_HAS_FLAG(BYTEFIELD, FLAG)           ((BYTEFIELD & FLAG) == FLAG)

/*
███████╗██╗██╗  ██╗███████╗██████╗      ██████╗ ██████╗ ███╗   ██╗██╗   ██╗███████╗██████╗ ████████╗██╗ ██████╗ ███╗   ██╗
██╔════╝██║╚██╗██╔╝██╔════╝██╔══██╗    ██╔════╝██╔═══██╗████╗  ██║██║   ██║██╔════╝██╔══██╗╚══██╔══╝██║██╔═══██╗████╗  ██║
█████╗  ██║ ╚███╔╝ █████╗  ██║  ██║    ██║     ██║   ██║██╔██╗ ██║██║   ██║█████╗  ██████╔╝   ██║   ██║██║   ██║██╔██╗ ██║
██╔══╝  ██║ ██╔██╗ ██╔══╝  ██║  ██║    ██║     ██║   ██║██║╚██╗██║╚██╗ ██╔╝██╔══╝  ██╔══██╗   ██║   ██║██║   ██║██║╚██╗██║
██║     ██║██╔╝ ██╗███████╗██████╔╝    ╚██████╗╚██████╔╝██║ ╚████║ ╚████╔╝ ███████╗██║  ██║   ██║   ██║╚██████╔╝██║ ╚████║
╚═╝     ╚═╝╚═╝  ╚═╝╚══════╝╚═════╝      ╚═════╝ ╚═════╝ ╚═╝  ╚═══╝  ╚═══╝  ╚══════╝╚═╝  ╚═╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝

 ▲ NOTE ABOUT FIXED NUMBER ▲
 Only values between -32767.99998 and 32767.99998 can be converted to fixed number.

*/

/** @brief Fixed floating point value for 0 */
# define JO_FIXED_0					(0)
/** @brief Fixed floating point value for 0.5 */
# define JO_FIXED_1_DIV_2           (32768)
/** @brief Fixed floating point value for 0.999 */
# define JO_FIXED_ALMOST_1          (65470)
/** @brief Fixed floating point value for 1 */
# define JO_FIXED_1					(65536)
/** @brief Fixed floating point value for 2 */
# define JO_FIXED_2                 JO_INT_TO_FIXED(2)
/** @brief Fixed floating point value for 3 */
# define JO_FIXED_3                 JO_INT_TO_FIXED(3)
/** @brief Fixed floating point value for 4 */
# define JO_FIXED_4                 JO_INT_TO_FIXED(4)
/** @brief Fixed floating point value for 8 */
# define JO_FIXED_8                 JO_INT_TO_FIXED(8)
/** @brief Fixed floating point value for 16 */
# define JO_FIXED_16                JO_INT_TO_FIXED(16)
/** @brief Fixed floating point value for 32 */
# define JO_FIXED_32                JO_INT_TO_FIXED(32)
/** @brief Fixed floating point value for 120 */
# define JO_FIXED_120				JO_INT_TO_FIXED(120)
/** @brief Fixed floating point value for 150 */
# define JO_FIXED_150				JO_INT_TO_FIXED(150)
/** @brief Fixed floating point value for 180 */
# define JO_FIXED_180				JO_INT_TO_FIXED(180)
/** @brief Fixed floating point value for 360 */
# define JO_FIXED_360				JO_INT_TO_FIXED(360)

/** @brief Fixed floating point value for -32767.99998 */
# define JO_FIXED_MIN				(-2147483647)
/** @brief Fixed floating point value for +32767.99998 */
# define JO_FIXED_MAX				(2147483647)
/** @brief Jo Fixed minimum positive value */
# define JO_FIXED_EPSILON           (1)
/** @brief Indicate an overflow error */
# define JO_FIXED_OVERFLOW          (0x80000000)
/** @brief Fixed value of PI */
# define JO_FIXED_PI                (205887)
/** @brief Fixed value of 2 PI */
# define JO_FIXED_PI_2              (411775)
/** @brief Fixed value of 180/PI */
# define JO_FIXED_180_DIV_PI        (3754936)
/** @brief Fixed value of PI/180 */
# define JO_FIXED_PI_DIV_180        (1144)
/** @brief Fixed value of PI/2 */
# define JO_FIXED_PI_DIV_2          (102944)
/** @brief Fixed value of PI/2 */
# define JO_FIXED_PI_OVER_2         JO_FIXED_PI_DIV_2
/** @brief Fixed value of 3 PI / 2 */
# define JO_FIXED_3_PI_OVER_2       (308331)
/** @brief Fixed value of 1/65536 */
# define JO_FIXED_1_DIV             (1.0f / 65536.0f)

/** @brief Convert int to jo_fixed
 *  @param X operand
 */
# define JO_INT_TO_FIXED(X)         JO_MULT_BY_65536(X)
/** @brief Convert jo_fixed to int
 *  @param X operand
 */
# define JO_FIXED_TO_INT(X)         JO_DIV_BY_65536(X)
/** @brief Decimal part mask of a jo_fixed */
# define JO_FIXED_DECIMAL_MASK      (0xFFFF)
/** @brief Convert a float to jo_fixed
 *  @param X Float (constant expression friendly, usable in static initializers)
 */
# define JO_FLOAT_TO_FIXED(X)       ((jo_fixed)(65536.0f * (X)))
/** @brief Convert a degree to jo_angle
 *  @param X Degree (constant expression friendly, usable in static initializers)
 */
# define JO_DEG_TO_ANGLE(X)         ((jo_angle)((65536.0 * (X)) / 360.0))
/** @brief Convert a radian to jo_angle
 *  @param X Radian (constant expression friendly, usable in static initializers)
 */
# define JO_RAD_TO_ANGLE(X)         ((jo_angle)((65536.0 * (X)) / (2.0 * 3.14159265358979)))

/** @brief Convert int to jo engine fixed
 *  @param x Float to convert
 *  @return Fixed value
 */
static __jo_force_inline jo_fixed       jo_int2fixed(const int x)
{
    return JO_INT_TO_FIXED(x);
}

/** @brief Convert jo engine fixed to int
 *  @param x Fixed
 *  @return Integer value
 */
static __jo_force_inline int            jo_fixed2int(const jo_fixed x)
{
    return JO_FIXED_TO_INT(x);
}

/** @brief Convert float to jo engine fixed (avoid usage of GCC Soft Float)
 *  @param x Float to convert
 *  @return Fixed value
 */
static __jo_force_inline jo_fixed       jo_float2fixed(const float x)
{
    return ((jo_fixed)(x * (float)JO_FIXED_1));
}

/** @brief Convert jo engine fixed to float
 *  @param x Fixed to convert
 *  @return Float value
 */
static __jo_force_inline float          jo_fixed2float(const jo_fixed x)
{
    return ((float)x * JO_FIXED_1_DIV);
}

/** @brief Wrap rad in [−pi pi]
 *  @param rad Fixed radian
 *  @return rad wrapped in [−pi pi]
 */
static __jo_force_inline jo_fixed       jo_fixed_wrap_to_pi(jo_fixed rad)
{
    while (rad > JO_FIXED_PI) rad -= JO_FIXED_PI_2;
    while (rad <= -JO_FIXED_PI) rad += JO_FIXED_PI_2;
    return (rad);
}

/** @brief Wrap deg in [-180 180]
 *  @param deg Fixed degree
 *  @return deg wrapped in [-180 180]
 */
static __jo_force_inline jo_fixed       jo_fixed_wrap_to_180(jo_fixed deg)
{
    while (deg > JO_FIXED_180) deg -= JO_FIXED_360;
    while (deg <= -JO_FIXED_180) deg += JO_FIXED_360;
    return (deg);
}

/** @brief x raised to the power of the integer part of y
 *  @param x First operand
 *  @param y Second operand
 *  @return x^y
 */
jo_fixed                                jo_fixed_pow(jo_fixed x, jo_fixed y);

/** @brief Multiply to fixed number
 *  @param x First operand
 *  @param y Second operand
 *  @return x * y
 *  @remarks Compiled as a single SH-2 instruction (dmuls.l) + 3 register moves
 */
static __jo_force_inline jo_fixed       jo_fixed_mult(const jo_fixed x, const jo_fixed y)
{
    return ((jo_fixed)(((long long)x * y) >> 16));
}

/** @brief Multiply to fixed number and keep the integer part: jo_fixed2int(jo_fixed_mult(x, y)), without overflow
 *  @param x First operand
 *  @param y Second operand
 *  @return Integer part of x * y (rounded down)
 *  @remarks The high word of the 64 bits product (dmuls.l + sts mach): faster than jo_fixed_mult() then a shift
 */
static __jo_force_inline int            jo_fixed_mult_to_int(const jo_fixed x, const jo_fixed y)
{
    int                                 high;

    __asm__ ("dmuls.l %1, %2\n\t"
             "sts mach, %0"
             : "=r" (high)
             : "r" (x), "r" (y)
             : "mach", "macl");
    return (high);
}

/** @brief Dot product two fixed 3D points
 *  @param ptA Point A (first operand)
 *  @param ptB	Point B (second operand)
 *  @return ptA · ptB
 *  @remarks The sum is computed on 64 bits by the SH-2 multiply and accumulate unit (mac.l), so only the result must fit in a jo_fixed
 *  @remarks It's about twice faster than 3 calls of jo_fixed_mult()
 */
static __jo_force_inline jo_fixed       jo_fixed_dot(const jo_fixed ptA[3], const jo_fixed ptB[3])
{
    const jo_fixed                      *a;
    const jo_fixed                      *b;
    jo_fixed                            high;
    jo_fixed                            low;

    a = ptA;
    b = ptB;
    __asm__ ("clrmac\n\t"
             "mac.l @%2+, @%3+\n\t"
             "mac.l @%2+, @%3+\n\t"
             "mac.l @%2+, @%3+\n\t"
             "sts mach, %0\n\t"
             "sts macl, %1\n\t"
             "xtrct %0, %1"
             : "=&r" (high), "=&r" (low), "+r" (a), "+r" (b)
             : "m" (*(const jo_fixed (*)[3])ptA), "m" (*(const jo_fixed (*)[3])ptB)
             : "mach", "macl");
    return (low);
}

/** @brief Divide fixed-point numbers (expresses dividend / divisor)
 *  @param dividend (first operand)
 *  @param divisor  (second operand)
 *  @return dividend / divisor
 *  @remarks Uses the SH-2 division unit (64 bits / 32 bits in 39 cycles). The CPU waits automatically for the result.
 *  @warning The result is undefined if divisor is 0 or if the quotient doesn't fit in a jo_fixed
 */
static __jo_force_inline jo_fixed       jo_fixed_div(const jo_fixed dividend, const jo_fixed divisor)
{
    JO_SH2_DVSR = divisor;
    // 64 bits dividend: (dividend << 16)
    JO_SH2_DVDNTH = dividend >> 16;
    // Writing the low part starts the division
    JO_SH2_DVDNTL = dividend << 16;
    return ((jo_fixed)JO_SH2_DVDNTL);
}

/** @brief Divide 2 integers (dividend / divisor, rounded toward 0 as in C)
 *  @param dividend (first operand)
 *  @param divisor  (second operand)
 *  @return dividend / divisor
 *  @remarks Uses the SH-2 division unit (32 bits / 32 bits in 39 cycles): faster than the division of the compiler
 *           (___sdivsi3, a routine of the library: about 3 times slower)
 *  @warning The result is undefined if divisor is 0
 */
static __jo_force_inline int            jo_int_div(const int dividend, const int divisor)
{
    JO_SH2_DVSR = divisor;
    // (writing the dividend starts the division. The quotient read in DVDNTL, as jo_fixed_div(): the register saved and
    // restored by the VBLANK interrupt)
    JO_SH2_DVDNT = dividend;
    return (JO_SH2_DVDNTL);
}

/** @brief Arithmetic right shift by 8 bits (x >> 8, rounded toward minus infinity) of any value
 *  @param x Value
 *  @return x >> 8
 *  @remarks The SH-2 shifts arithmetically by one bit only: GCC calls a routine of its library (___ashiftrt_r4_8). Here
 *  the logical shift (shlr8) of x or of its complement
 */
static __jo_force_inline int            jo_shift_right8(const int x)
{
    return (x < 0 ? (int)~(~(unsigned int)x >> 8) : (int)((unsigned int)x >> 8));
}

/** @brief Arithmetic right shift by 8 bits (x >> 8) of a value in [-2^23, 2^23) (a 16 bits value x 0 to 256 for example)
 *  @param x Value
 *  @return x >> 8
 *  @remarks Without a branch: the logical shift of x + 2^23 (positive), minus 2^15
 *  @warning The result is wrong out of the range
 */
static __jo_force_inline int            jo_shift_right8_small(const int x)
{
    return ((int)(((unsigned int)x + (1u << 23)) >> 8) - (1 << 15));
}

/** @brief Start a fixed-point division in the background (see jo_fixed_div_result())
 *  @param dividend (first operand)
 *  @param divisor  (second operand)
 *  @remarks The SH-2 division unit computes the quotient in 39 cycles while the CPU executes other instructions
 *  @warning Don't start another division before reading the result
 */
static __jo_force_inline void           jo_fixed_div_start(const jo_fixed dividend, const jo_fixed divisor)
{
    JO_SH2_DVSR = divisor;
    JO_SH2_DVDNTH = dividend >> 16;
    JO_SH2_DVDNTL = dividend << 16;
}

/** @brief Get the result of the division started by jo_fixed_div_start()
 *  @return dividend / divisor
 *  @remarks The CPU waits automatically if the division is not finished
 */
static __jo_force_inline jo_fixed       jo_fixed_div_result(void)
{
    return ((jo_fixed)JO_SH2_DVDNTL);
}

/** @brief Convert fixed degree to fixed radian
 *  @param deg Fixed angle in degree
 *  @return Fixed angle in radian
 */
static __jo_force_inline jo_fixed       jo_fixed_deg2rad(const jo_fixed deg)
{
    return (jo_fixed_mult(jo_fixed_wrap_to_180(deg), JO_FIXED_PI_DIV_180));
}

/** @brief Convert fixed radian to fixed degree
 *  @param rad Fixed angle in radian
 *  @return Fixed angle in degree
 */
static __jo_force_inline jo_fixed       jo_fixed_rad2deg(const jo_fixed rad)
{
    return (jo_fixed_mult(jo_fixed_wrap_to_pi(rad), JO_FIXED_180_DIV_PI));
}

/** @brief Returns the smallest (fixed) integer value greater than or equal to x
 *  @param x Fixed value
 *  @return Smallest (fixed) integer value greater than or equal to x
 */
static __jo_force_inline jo_fixed       jo_fixed_ceil(const jo_fixed x)
{
    return (x & 0xFFFF0000UL) + (x & 0x0000FFFFUL ? JO_FIXED_1 : 0);
}

/** @brief Returns the largest (fixed) integer value less than or equal to x
 *  @param x Fixed value
 *  @return largest (fixed) integer value less than or equal to x
 */
static __jo_force_inline jo_fixed       jo_fixed_floor(const jo_fixed x)
{
    return (x & 0xFFFF0000ul);
}

/*
██████╗  █████╗ ███╗   ██╗██████╗  ██████╗ ███╗   ███╗
██╔══██╗██╔══██╗████╗  ██║██╔══██╗██╔═══██╗████╗ ████║
██████╔╝███████║██╔██╗ ██║██║  ██║██║   ██║██╔████╔██║
██╔══██╗██╔══██║██║╚██╗██║██║  ██║██║   ██║██║╚██╔╝██║
██║  ██║██║  ██║██║ ╚████║██████╔╝╚██████╔╝██║ ╚═╝ ██║
╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═════╝  ╚═════╝ ╚═╝     ╚═╝
*/

/** @brief Get or set current random seed
 */
extern int                      jo_random_seed;

/** @brief Get a random number
 *  @param max maximum value
 *  @return 1 to max
 *  @remarks Park-Miller generator (16807 * seed mod 2^31 - 1) computed without division (same sequence as before)
 */
int                             jo_random(int max);

/** @brief Get a random number with a specific multiple
 *  @param max maximum value
 *  @param multiple multiple
 *  @return A multiple between 1 to max
 */
static  __jo_force_inline int	jo_random_using_multiple(int max, int multiple)
{
    return (jo_random(max) / multiple) * multiple;
}

/*
███████╗ ██████╗ ██████╗ ████████╗
██╔════╝██╔═══██╗██╔══██╗╚══██╔══╝
███████╗██║   ██║██████╔╝   ██║
╚════██║██║▄▄ ██║██╔══██╗   ██║
███████║╚██████╔╝██║  ██║   ██║
╚══════╝ ╚══▀▀═╝ ╚═╝  ╚═╝   ╚═╝
*/

/** @brief Index of the highest bit set in a value (31 - number of leading zeros)
 *  @param x Value
 *  @return 0 to 31 (0 if x is 0 or 1)
 *  @remarks Binary search in 5 steps: the SH-2 has no instruction for it
 */
static  __jo_force_inline unsigned int  jo_highest_bit(unsigned int x)
{
    unsigned int                        bit;

    bit = 0;
    if (x & 0xffff0000)
    {
        bit = 16;
        x >>= 16;
    }
    if (x & 0xff00)
    {
        bit += 8;
        x >>= 8;
    }
    if (x & 0xf0)
    {
        bit += 4;
        x >>= 4;
    }
    if (x & 0xc)
    {
        bit += 2;
        x >>= 2;
    }
    if (x & 0x2)
        ++bit;
    return (bit);
}

/** @brief Fast Square root using fixed number
 *  @param value Value
 *  @return Sqrt(value), rounded down to 1/65536 (JO_FIXED_OVERFLOW if value is negative)
 *  @remarks A table and one step of Newton's method with the division unit (see jo_sqrt())
 */
jo_fixed                        jo_fixed_sqrt(jo_fixed value);

/** @brief Fast square root
 *  @param value Value
 *  @return Integer part of Sqrt(value)
 *  @remarks A table and one step of Newton's method with the division unit (about 70 cycles)
 */
unsigned int                    jo_sqrt(unsigned int value);

/** @brief Fast Square root using floating number
 *  @param value Value
 *  @return Sqrt(value)
 *  @warning Not precise
 */
static  __jo_force_inline float jo_sqrtf(float value)
{
    // A union avoids the strict aliasing issue of *(unsigned int *)&value (undefined behavior with -O2)
    union
    {
        float           f;
        unsigned int    i;
    }                   bits;

    bits.f = value;
    bits.i += 127 << 23;
    bits.i >>= 1;
    return (bits.f);
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

/** @brief Fast Reciprocal Square root using fixed number
 *  @param value Value
 *  @return RSqrt(value)
 */
jo_fixed        jo_fixed_rsqrt(jo_fixed value);

/** @brief Fast Reciprocal Square root using floating number
 *  @param value Value
 *  @return RSqrt(value)
 */
float           jo_rsqrt(float value);

/*
███████╗ ██████╗ ██╗         ██╗███╗   ██╗████████╗███████╗██████╗  ██████╗ ██████╗
██╔════╝██╔════╝ ██║         ██║████╗  ██║╚══██╔══╝██╔════╝██╔══██╗██╔═══██╗██╔══██╗
███████╗██║  ███╗██║         ██║██╔██╗ ██║   ██║   █████╗  ██████╔╝██║   ██║██████╔╝
╚════██║██║   ██║██║         ██║██║╚██╗██║   ██║   ██╔══╝  ██╔══██╗██║   ██║██╔═══╝
███████║╚██████╔╝███████╗    ██║██║ ╚████║   ██║   ███████╗██║  ██║╚██████╔╝██║
╚══════╝ ╚═════╝ ╚══════╝    ╚═╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═╝

*/

/** @brief See https://github.com/johannes-fetz/joengine/issues/42 */
# define __JO_DEG_TO_ANGLE_MAGIC    (182)

/** @brief Convert a degree to jo_angle using floating number
 *  @param deg Degree
 *  @return jo_angle
 */
static  __jo_force_inline jo_angle     jo_DEGtoANG(const float deg)
{
    // One float multiplication (65536 / 360), not a double multiplication and a double division
    return ((jo_angle)(deg * (65536.0f / 360.0f)));
}

/** @brief Convert a degree to jo_angle using interger
 *  @param deg Degree
 *  @return jo_angle
 */
static  __jo_force_inline jo_angle     jo_DEGtoANG_int(const int deg)
{
    int                                 n;
    int                                 a;

    // deg * 65536 / 360 = deg * 8192 / 45: multiplication by 2^32 / 45 (rounded up) instead of a division.
    // Same result as the division for |deg| < 37000
    n = deg < 0 ? -deg : deg;
    a = (int)(((long long)(n * 8192) * 95443718LL) >> 32);
    return ((jo_angle)(deg < 0 ? -a : a));
}


/** @brief Convert fixed radian to jo_angle
 *  @param rad Jo Engine fixed radian
 *  @return jo_angle
 */
static  __jo_force_inline jo_angle     jo_fixed_rad2ANGLE(const jo_fixed rad)
{
    // rad * 65536 / (2 pi) without floating point: 65536 / (2 pi) = 683565276 / 2^32 (Q16.16 input)
    return ((jo_angle)(((long long)rad * 683565276LL) >> 32));
}


/** @brief Sinus table: sin(x) for x in [0, pi/2] (1024 steps, Q16.16) (Internal usage, see math.c) */
extern const jo_fixed               __jo_sin_table[1025];

/** @brief Sinus of a table index (Internal usage)
 *  @param index 0 to 4095 (4096 steps per turn)
 *  @return Q16.16 fixed number
 */
static  __jo_force_inline jo_fixed     __jo_sin_index(const unsigned int index)
{
    if (index < 1024)
        return (__jo_sin_table[index]);
    if (index < 2048)
        return (__jo_sin_table[2048 - index]);
    if (index < 3072)
        return (-__jo_sin_table[index - 2048]);
    return (-__jo_sin_table[4096 - index]);
}

/** @brief Table index of an angle (Internal usage)
 *  @param angle jo_angle
 *  @return 0 to 4095 (4096 steps per turn, rounded to the nearest)
 */
static  __jo_force_inline unsigned int __jo_angle_to_sin_index(const jo_angle angle)
{
    return (((((unsigned short)angle) + 8) >> 4) & 0xfff);
}

/** @brief Sinus of an angle (table lookup, fast)
 *  @param angle jo_angle (0x10000 = 360 degrees, it wraps: for instance (jo_angle)(degrees * 182) for an angle that grows)
 *  @return Q16.16 fixed number
 *  @remarks Inlined: a table lookup costs less than a function call
 */
static  __jo_force_inline jo_fixed     jo_sin_angle(const jo_angle angle)
{
    return (__jo_sin_index(__jo_angle_to_sin_index(angle)));
}

/** @brief Cosinus of an angle (table lookup, fast)
 *  @param angle jo_angle (0x10000 = 360 degrees, it wraps)
 *  @return Q16.16 fixed number
 */
static  __jo_force_inline jo_fixed     jo_cos_angle(const jo_angle angle)
{
    return (__jo_sin_index((__jo_angle_to_sin_index(angle) + 1024) & 0xfff));
}

/** @brief Sinus and cosinus of the same angle (table lookup, fast)
 *  @param angle jo_angle (0x10000 = 360 degrees, it wraps)
 *  @param sin_value Sinus (Q16.16 fixed number)
 *  @param cos_value Cosinus (Q16.16 fixed number)
 *  @remarks Same values as jo_sin_angle() and jo_cos_angle(), the angle is converted once
 */
static  __jo_force_inline void         jo_sincos_angle(const jo_angle angle, jo_fixed * const sin_value, jo_fixed * const cos_value)
{
    unsigned int                        index;

    index = __jo_angle_to_sin_index(angle);
    *sin_value = __jo_sin_index(index);
    *cos_value = __jo_sin_index((index + 1024) & 0xfff);
}

/** @brief Convert fixed degree to jo_angle
 *  @param deg Jo Engine fixed degree
 *  @return jo_angle
 */
static  __jo_force_inline jo_angle     jo_fixed_deg2ANGLE(const jo_fixed deg)
{
    // deg * 65536 / 360 without floating point: 1 / 360 = 11930465 / 2^32 (Q16.16 input)
    return ((jo_angle)(((long long)deg * 11930465LL) >> 32));
}

/*
███████╗██╗███╗   ██╗██╗   ██╗███████╗
██╔════╝██║████╗  ██║██║   ██║██╔════╝
███████╗██║██╔██╗ ██║██║   ██║███████╗
╚════██║██║██║╚██╗██║██║   ██║╚════██║
███████║██║██║ ╚████║╚██████╔╝███████║
╚══════╝╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚══════╝
*/

/** @brief Fast sinus computation using fixed number
 *  @param rad Fixed angle in radian
 *  @return Sin(rad)
 *  @remarks Table lookup (jo_sin_angle())
 */
static  __jo_force_inline jo_fixed	jo_fixed_sin(const jo_fixed rad)
{
    return (jo_sin_angle(jo_fixed_rad2ANGLE(rad)));
}

/** @brief Fast sinus computation
 *  @param deg Angle in degree
 *  @return Fixed Sin(deg)
 */
static  __jo_force_inline jo_fixed	jo_sin(const int deg)
{
    // Table lookup, without floating point
    return (jo_sin_angle(jo_DEGtoANG_int(deg)));
}

/** @brief Sinus computation
 *  @param deg Angle in degree
 *  @return Sin(deg) using floating number (slow)
 *  @warning slower than jo_sin() because it use floating point
 */
static  __jo_force_inline float	jo_sinf(const int deg)
{
    return (jo_fixed2float(jo_sin(deg)));
}

/** @brief Sinus computation
 *  @param rad Angle in radian
 *  @return Fixed Sin(rad)
 *  @warning slower than jo_sin() because it use floating point
 */
static  __jo_force_inline jo_fixed	jo_sin_rad(const float rad)
{
    return (jo_fixed_sin(jo_float2fixed(rad)));
}

/** @brief Sinus computation
 *  @param rad Angle in radian
 *  @return Sin(rad) using floating number (slow)
 *  @warning slower than jo_sin_rad() because it use floating point
 */
static  __jo_force_inline float	jo_sin_radf(const float rad)
{
    return (jo_fixed2float(jo_fixed_sin(jo_float2fixed(rad))));
}

/** @brief Fast sinus multiplication
 *  @param deg Angle in degree
 *  @param value Value
 *  @return value * Sin(deg)
 *  @remarks 64 bits product: value * Sin(deg) overflowed from |value| > 32767
 */
static  __jo_force_inline int	jo_sin_mult(const int value, const int deg)
{
    return (jo_fixed_mult(value, jo_sin(deg)));
}

/** @brief Fast sinus multiplication
 *  @param deg Angle in degree
 *  @param value Value
 *  @return value * Sin(deg) using floating number (slow)
 */
static  __jo_force_inline float	jo_sinf_mult(const float value, const int deg)
{
    return (value * jo_sinf(deg));
}

/*
 ██████╗ ██████╗ ███████╗██╗███╗   ██╗██╗   ██╗███████╗
██╔════╝██╔═══██╗██╔════╝██║████╗  ██║██║   ██║██╔════╝
██║     ██║   ██║███████╗██║██╔██╗ ██║██║   ██║███████╗
██║     ██║   ██║╚════██║██║██║╚██╗██║██║   ██║╚════██║
╚██████╗╚██████╔╝███████║██║██║ ╚████║╚██████╔╝███████║
 ╚═════╝ ╚═════╝ ╚══════╝╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚══════╝

*/

/** @brief Fast cosinus computation using fixed number
 *  @param rad Fixed angle in radian
 *  @return Cos(rad)
 *  @remarks Table lookup (jo_cos_angle())
 */
static  __jo_force_inline jo_fixed	jo_fixed_cos(const jo_fixed rad)
{
    return (jo_cos_angle(jo_fixed_rad2ANGLE(rad)));
}

/** @brief Fast cosinus computation
 *  @param deg Angle in degree
 *  @return Fixed Cos(deg)
 */
static  __jo_force_inline jo_fixed	jo_cos(const int deg)
{
    // Table lookup, without floating point
    return (jo_cos_angle(jo_DEGtoANG_int(deg)));
}

/** @brief Sinus and cosinus of the same angle in degree (table lookup, fast)
 *  @param deg Angle in degree
 *  @param sin_value Fixed Sin(deg)
 *  @param cos_value Fixed Cos(deg)
 *  @remarks Same values as jo_sin() and jo_cos(), the angle is converted once
 */
static  __jo_force_inline void	jo_sincos(const int deg, jo_fixed * const sin_value, jo_fixed * const cos_value)
{
    jo_sincos_angle(jo_DEGtoANG_int(deg), sin_value, cos_value);
}

/** @brief Cosinus computation
 *  @param deg Angle in degree
 *  @return Cos(deg) using floating number (slow)
 *  @warning slower than jo_cos() because it use floating point
 */
static  __jo_force_inline float	jo_cosf(const int deg)
{
    return (jo_fixed2float(jo_cos(deg)));
}

/** @brief Cosinus computation
 *  @param rad Angle in radian
 *  @return Fixed Cos(rad)
 *  @warning slower than jo_cos() because it use floating point
 */
static  __jo_force_inline jo_fixed	jo_cos_rad(const float rad)
{
    return (jo_fixed_cos(jo_float2fixed(rad)));
}

/** @brief Cosinus computation
 *  @param rad Angle in radian
 *  @return Cos(rad) using floating number (slow)
 *  @warning slower than jo_cos_rad() because it use floating point
 */
static  __jo_force_inline float	jo_cos_radf(const float rad)
{
    return (jo_fixed2float(jo_fixed_cos(jo_float2fixed(rad))));
}

/** @brief Fast cosinus multiplication
 *  @param deg Angle in degree
 *  @param value Value
 *  @return value * Cos(deg)
 *  @remarks 64 bits product: value * Cos(deg) overflowed from |value| > 32767
 */
static  __jo_force_inline int	jo_cos_mult(const int value, const int deg)
{
    return (jo_fixed_mult(value, jo_cos(deg)));
}

/** @brief Fast cosinus multiplication
 *  @param deg Angle in degree
 *  @param value Value
 *  @return value * Cos(deg) using floating number (slow)
 */
static  __jo_force_inline float	jo_cosf_mult(const float value, const int deg)
{
    return (value * jo_cosf(deg));
}

/*
████████╗ █████╗ ███╗   ██╗ ██████╗ ███████╗███╗   ██╗████████╗
╚══██╔══╝██╔══██╗████╗  ██║██╔════╝ ██╔════╝████╗  ██║╚══██╔══╝
   ██║   ███████║██╔██╗ ██║██║  ███╗█████╗  ██╔██╗ ██║   ██║
   ██║   ██╔══██║██║╚██╗██║██║   ██║██╔══╝  ██║╚██╗██║   ██║
   ██║   ██║  ██║██║ ╚████║╚██████╔╝███████╗██║ ╚████║   ██║
   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚══════╝╚═╝  ╚═══╝   ╚═╝

*/

/** @brief Fast tangent computation
 *  @param deg Angle in degree
 *  @return Fixed Tan(deg)
 */
static __jo_force_inline jo_fixed    jo_tan(const int deg)
{
    jo_fixed                         sin_value;
    jo_fixed                         cos_value;

    jo_sincos(deg, &sin_value, &cos_value);
    return jo_fixed_div(sin_value, cos_value);
}

/** @brief Tangent computation
 *  @param deg Angle in degree
 *  @return Tan(deg) using floating number (slow)
 *  @warning slower than jo_tan() because it use floating point
 */
static __jo_force_inline float    jo_tanf(const float deg)
{
    return (jo_sinf(deg) / jo_cosf(deg));
}

/** @brief Tangent computation
 *  @param rad Angle in radian
 *  @return Fixed Tan(rad)
 *  @warning slower than jo_tan() because it use floating point
 */
static __jo_force_inline jo_fixed    jo_tan_rad(const float rad)
{
    jo_fixed                         sin_value;
    jo_fixed                         cos_value;

    jo_sincos_angle(jo_fixed_rad2ANGLE(jo_float2fixed(rad)), &sin_value, &cos_value);
    return jo_fixed_div(sin_value, cos_value);
}

/** @brief Tangent computation
 *  @param rad Angle in radian
 *  @return Tan(rad) using floating number (slow)
 *  @warning slower than jo_tan_rad() because it use floating point
 */
static __jo_force_inline float  jo_tan_radf(const float rad)
{
    return (jo_sin_radf(rad) / jo_cos_radf(rad));
}

/*
 █████╗  ██████╗ ██████╗ ███████╗
██╔══██╗██╔════╝██╔═══██╗██╔════╝
███████║██║     ██║   ██║███████╗
██╔══██║██║     ██║   ██║╚════██║
██║  ██║╚██████╗╚██████╔╝███████║
╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚══════╝
*/

/** @brief Fast Arc Cosinus computation (linear approximation)
 *  @param angle Cosinus (-1 to 1)
 *  @return Acos(angle) in radian using floating number: PI for -1, PI/2 for 0, 0 for 1
 *  @warning Rough: error up to about 0.21 radian (12 degrees)
 */
static  __jo_force_inline float	jo_acos_radf(const float angle)
{
    return (1.5707963f - 1.5707963f * angle);
}

/*
 █████╗ ████████╗ █████╗ ███╗   ██╗██████╗
██╔══██╗╚══██╔══╝██╔══██╗████╗  ██║╚════██╗
███████║   ██║   ███████║██╔██╗ ██║ █████╔╝
██╔══██║   ██║   ██╔══██║██║╚██╗██║██╔═══╝
██║  ██║   ██║   ██║  ██║██║ ╚████║███████╗
╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═══╝╚══════╝

*/

/** @brief Fast ATAN2 computation in radian
 *  @param y First value
 *  @param x Second value
 *  @warning Y is the first operand (like Math.Atan2() in C#)
 *  @return ATAN2(Y, X)
 */
float                           jo_atan2f_rad(const float y, const float x);

/** @brief Fast ATAN2 computation in degree
 *  @param y First value
 *  @param x Second value
 *  @warning Y is the first operand (like Math.Atan2() in C#)
 *  @return ATAN2(Y, X)
 */
static __jo_force_inline int    jo_atan2f(const float y, const float x)
{
    return (int)JO_RAD_TO_DEG(jo_atan2f_rad(y, x));
}

/** @brief ATAN2 as a jo_angle (65536 per turn, like jo_sin_angle() and the rotations of the 3D)
 *  @param y First value
 *  @param x Second value
 *  @return Angle (-32768 to 32767)
 *  @remarks Finer than jo_atan2_degrees() (a degree is 182 units of jo_angle): a camera aimed with it does not shake
 */
static __jo_force_inline jo_angle   jo_atan2_angle(const float y, const float x)
{
    return ((jo_angle)(int)(jo_atan2f_rad(y, x) * (32768.0f / JO_PI)));
}

/** @brief ATAN2 in degree without floating point (fixed numbers)
 *  @param y First value (Q16.16)
 *  @param x Second value (Q16.16)
 *  @warning Y is the first operand (like Math.Atan2() in C#)
 *  @return ATAN2(Y, X) in degree, -179 to 180 (0 if x and y are 0). Error: about 1 degree at most (rounded down)
 *  @remarks Much faster than jo_atan2f() (the SH-2 has no FPU): a division and a polynomial. The heading of a vector
 *  (x = cos, z = sin, like jo_cos() / jo_sin()) is jo_atan2_degrees(z, x)
 */
int                             jo_atan2_degrees(const jo_fixed y, const jo_fixed x);

/** @brief Turn an angle toward another angle, by the shortest way
 *  @param angle Current angle in degree (0 to 359)
 *  @param target Target angle in degree (any value)
 *  @param max_step Largest change in degree (positive)
 *  @return New angle in degree (0 to 359)
 */
static __jo_force_inline int    jo_turn_toward(const int angle, const int target, const int max_step)
{
    int                         d;

    // Shortest difference: -180 to 179 (the modulo, a division by libgcc, only when the angles are not 0 to 359)
    d = target - angle;
    if (d >= 360 || d <= -360)
        d %= 360;
    if (d >= 180)
        d -= 360;
    else if (d < -180)
        d += 360;
    if (d > max_step)
        d = max_step;
    else if (d < -max_step)
        d = -max_step;
    d += angle;
    if (d >= 360 || d <= -360)
        d %= 360;
    return (d < 0 ? d + 360 : d);
}

/*
██╗   ██╗███████╗ ██████╗████████╗ ██████╗ ██████╗
██║   ██║██╔════╝██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗
██║   ██║█████╗  ██║        ██║   ██║   ██║██████╔╝
╚██╗ ██╔╝██╔══╝  ██║        ██║   ██║   ██║██╔══██╗
 ╚████╔╝ ███████╗╚██████╗   ██║   ╚██████╔╝██║  ██║
  ╚═══╝  ╚══════╝ ╚═════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝
*/

/*
    _______               __   _   __                __
   / ____(_)  _____  ____/ /  / | / /_  ______ ___  / /_  ___  _____
  / /_  / / |/_/ _ \/ __  /  /  |/ / / / / __ `__ \/ __ \/ _ \/ ___/
 / __/ / />  </  __/ /_/ /  / /|  / /_/ / / / / / / /_/ /  __/ /
/_/   /_/_/|_|\___/\__,_/  /_/ |_/\__,_/_/ /_/ /_/_.___/\___/_/

*/

/** @brief Compute cubic bezier curve point for vectors (using fixed numbers)
 *  @param t Value of the function on the curve [0 to JO_FIXED_1]
 *  @param p0 First vector
 *  @param p1 Second vector
 *  @param p2 Third vector
 *  @param p3 Fourth vector
 *  @param result output vector
 */
void    jo_vector_fixed_compute_bezier_point(const jo_fixed t, jo_vector_fixed p0, jo_vector_fixed p1, jo_vector_fixed p2, jo_vector_fixed p3, jo_vector_fixed *result);

/** @brief Add 2 vectors (using fixed numbers)
 *  @param a First vector
 *  @param b Second vector
 *  @param result output vector
 *  @remarks You can pass a or b address for result parameter
 */
static __jo_force_inline void jo_vector_fixed_add(const jo_vector_fixed * const a, const jo_vector_fixed * const b, jo_vector_fixed * const result)
{
    result->x = a->x + b->x;
    result->y = a->y + b->y;
    result->z = a->z + b->z;
}

/** @brief Multiply value to vector (using fixed numbers)
 *  @param a Vector
 *  @param s Constant
 *  @param result output vector
 *  @remarks You can pass a address for result parameter
 */
static __jo_force_inline void jo_vector_fixed_muls(const jo_vector_fixed * const a, const jo_fixed s, jo_vector_fixed * const result)
{
    result->x = jo_fixed_mult(a->x, s);
    result->y = jo_fixed_mult(a->y, s);
    result->z = jo_fixed_mult(a->z, s);
}

/** @brief Add 2 vectors4 (using fixed numbers)
 *  @param a First vector4
 *  @param b Second vector4
 *  @param result output vector4
 *  @remarks You can pass a or b address for result parameter
 */
static __jo_force_inline void jo_vector4_fixed_add(const jo_vector4_fixed * const a, const jo_vector4_fixed * const b, jo_vector4_fixed * const result)
{
    result->x = a->x + b->x;
    result->y = a->y + b->y;
    result->z = a->z + b->z;
    result->w = JO_FIXED_1;
}

/** @brief Substract 2 vectors4 (using fixed numbers)
 *  @param a First vector4
 *  @param b Second vector4
 *  @param result output vector4
 *  @remarks You can pass a or b address for result parameter
 */
static __jo_force_inline void jo_vector4_fixed_sub(const jo_vector4_fixed * const a, const jo_vector4_fixed * const b, jo_vector4_fixed * const result)
{
    result->x = a->x - b->x;
    result->y = a->y - b->y;
    result->z = a->z - b->z;
    result->w = JO_FIXED_1;
}

/** @brief Cross product of 2 vectors4 (using fixed numbers)
 *  @param a First vector4
 *  @param b Second vector4
 *  @param result output vector4
 */
static __jo_force_inline void jo_vector4_fixed_cross(const jo_vector4_fixed * const a, const jo_vector4_fixed * const b, jo_vector4_fixed * const result)
{
    result->x = jo_fixed_mult(a->y, b->z) - jo_fixed_mult(a->z, b->y);
    result->y = jo_fixed_mult(a->z, b->x) - jo_fixed_mult(a->x, b->z);
    result->z = jo_fixed_mult(a->x, b->y) - jo_fixed_mult(a->y, b->x);
    result->w = JO_FIXED_1;
}

/** @brief Dot product of 2 vectors4 (using fixed numbers)
 *  @param a First vector4
 *  @param b Second vector4
 *  @return Dot product
 */
static __jo_force_inline jo_fixed jo_vector4_fixed_dot(const jo_vector4_fixed * const a, const jo_vector4_fixed * const b)
{
    // x, y, z are contiguous (w is ignored)
    return (jo_fixed_dot((const jo_fixed *)a, (const jo_fixed *)b));
}

/** @brief Swap 2 vectors4 (using fixed numbers)
 *  @param a First vector4
 *  @param b Second vector4
 */
static __jo_force_inline void jo_vector4_swap(jo_vector4_fixed * const a, jo_vector4_fixed * const b)
{
    JO_SWAP(a->x, b->x);
    JO_SWAP(a->y, b->y);
    JO_SWAP(a->z, b->z);
    JO_SWAP(a->w, b->w);
}

/*
    ________            __  _                _   __                __
   / ____/ /___  ____ _/ /_(_)___  ____ _   / | / /_  ______ ___  / /_  ___  _____
  / /_  / / __ \/ __ `/ __/ / __ \/ __ `/  /  |/ / / / / __ `__ \/ __ \/ _ \/ ___/
 / __/ / / /_/ / /_/ / /_/ / / / / /_/ /  / /|  / /_/ / / / / / / /_/ /  __/ /
/_/   /_/\____/\__,_/\__/_/_/ /_/\__, /  /_/ |_/\__,_/_/ /_/ /_/_.___/\___/_/
                                /____/
*/

/** @brief Compute cubic bezier curve point for vectors (using floating numbers)
 *  @param t Value of the function on the curve [0.0f to 1.0f]
 *  @param p0 First vector
 *  @param p1 Second vector
 *  @param p2 Third vector
 *  @param p3 Fourth vector
 *  @param result output vector
 */
void    jo_vectorf_compute_bezier_point(const float t, jo_vectorf p0, jo_vectorf p1, jo_vectorf p2, jo_vectorf p3, jo_vectorf *result);

/** @brief Add 2 vectors (using floating numbers)
 *  @param a First vector
 *  @param b Second vector
 *  @param result output vector
 *  @remarks You can pass a or b address for result parameter
 */
static __jo_force_inline void jo_vectorf_add(const jo_vectorf * const a, const jo_vectorf * const b, jo_vectorf * const result)
{
    result->x = a->x + b->x;
    result->y = a->y + b->y;
    result->z = a->z + b->z;
}

/** @brief Add value to vector (using floating numbers)
 *  @param a Vector
 *  @param s Constant
 *  @param result output vector
 *  @remarks You can pass a address for result parameter
 */
static __jo_force_inline void jo_vectorf_adds(const jo_vectorf * const a, const float s, jo_vectorf * const result)
{
    result->x = a->x + s;
    result->y = a->y + s;
    result->z = a->z + s;
}

/** @brief Subtract 2 vectors (using floating numbers)
 *  @param a First vector
 *  @param b Second vector
 *  @param result output vector
 *  @remarks You can pass a or b address for result parameter
 */
static __jo_force_inline void jo_vectorf_sub(const jo_vectorf * const a, const jo_vectorf * const b, jo_vectorf * const result)
{
    result->x = a->x - b->x;
    result->y = a->y - b->y;
    result->z = a->z - b->z;
}

/** @brief Subtract value to vector (using floating numbers)
 *  @param a Vector
 *  @param s Constant
 *  @param result output vector
 *  @remarks You can pass a address for result parameter
 */
static __jo_force_inline void jo_vectorf_subs(const jo_vectorf * const a, const float s, jo_vectorf * const result)
{
    result->x = a->x - s;
    result->y = a->y - s;
    result->z = a->z - s;
}

/** @brief Multiply 2 vectors (using floating numbers)
 *  @param a First vector
 *  @param b Second vector
 *  @param result output vector
 *  @remarks You can pass a or b address for result parameter
 */
static __jo_force_inline void jo_vectorf_mul(const jo_vectorf * const a, const jo_vectorf * const b, jo_vectorf * const result)
{
    result->x = a->x * b->x;
    result->y = a->y * b->y;
    result->z = a->z * b->z;
}

/** @brief Multiply value to vector (using floating numbers)
 *  @param a Vector
 *  @param s Constant
 *  @param result output vector
 *  @remarks You can pass a address for result parameter
 */
static __jo_force_inline void jo_vectorf_muls(const jo_vectorf * const a, const float s, jo_vectorf * const result)
{
    result->x = a->x * s;
    result->y = a->y * s;
    result->z = a->z * s;
}

/** @brief Divide 2 vectors (using floating numbers)
 *  @param a First vector
 *  @param b Second vector
 *  @param result output vector
 *  @remarks You can pass a or b address for result parameter
 */
static __jo_force_inline void jo_vectorf_div(const jo_vectorf * const a, const jo_vectorf * const b, jo_vectorf * const result)
{
    result->x = a->x / b->x;
    result->y = a->y / b->y;
    result->z = a->z / b->z;
}

/** @brief Divide value to vector (using floating numbers)
 *  @param a Vector
 *  @param s Constant
 *  @param result output vector
 *  @remarks You can pass a address for result parameter
 */
static __jo_force_inline void jo_vectorf_divs(const jo_vectorf * const a, const float s, jo_vectorf * const result)
{
    float                       inv;

    // One division (libgcc on the SH-2) and 3 multiplications instead of 3 divisions
    inv = 1.0f / s;
    result->x = a->x * inv;
    result->y = a->y * inv;
    result->z = a->z * inv;
}

/** @brief Get the length of a vector (using floating numbers)
 *  @param a Vector
 *  @return Vector length
 */
static __jo_force_inline float  jo_vectorf_length(const jo_vectorf * const a)
{
    return (jo_sqrtf(JO_SQUARE(a->x) + JO_SQUARE(a->y) + JO_SQUARE(a->z)));
}

/** @brief Get the dot product of 2 vectors (using floating numbers)
 *  @param a First vector
 *  @param b Second vector
 *  @return a.b
 */
static __jo_force_inline float  jo_vectorf_dot(const jo_vectorf * const a, const jo_vectorf * const b)
{
    return (a->x * b->x + a->y * b->y + a->z * b->z);
}

/** @brief Normalize a vector (using floating numbers)
 *  @param a Vector
 *  @param result output vector
 *  @remarks You can pass a address for result parameter
 */
static __jo_force_inline void jo_vectorf_normalize(const jo_vectorf * const a, jo_vectorf * const result)
{
    float len = jo_vectorf_length(a);
    if (len > 0)
    {
        // One division (libgcc on the SH-2) and 3 multiplications instead of 3 divisions
        float inv = 1.0f / len;
        result->x = a->x * inv;
        result->y = a->y * inv;
        result->z = a->z * inv;
    }
    else
    {
        result->x = 0;
        result->y = 0;
        result->z = 0;
    }
}

/** @brief Compute projection vector (using floating numbers)
 *  @param v Vector
 *  @param onto Vector
 *  @param result output vector
 */
static __jo_force_inline void jo_vectorf_proj(const jo_vectorf * const v, const jo_vectorf * const onto, jo_vectorf * const result)
{
    jo_vectorf_muls(onto, jo_vectorf_dot(v, onto) / jo_vectorf_dot(onto, onto), result);
}

/** @brief Cross 2 vectors (using floating numbers)
 *  @param a First vector
 *  @param b Second vector
 *  @param result output vector
 *  @warning Don't pass a or b address in result parameter
 */
static __jo_force_inline void jo_vectorf_cross(const jo_vectorf * const a, const jo_vectorf * const b, jo_vectorf * const result)
{
    result->x = a->y * b->z - a->z * b->y;
    result->y = a->z * b->x - a->x * b->z;
    result->z = a->x * b->y - a->y * b->x;
}

/** @brief Get the angle between 2 vectors (using floating numbers)
 *  @param a First vector
 *  @param b Second vector
 *  @return Angle in radian using floating numbers
 */
static __jo_force_inline float jo_vectorf_angle_between_radf(const jo_vectorf * const a, const jo_vectorf * const b)
{
    // One square root: sqrt(|a|^2 * |b|^2) = |a| * |b|
    return jo_acos_radf(jo_vectorf_dot(a, b) / jo_sqrtf(jo_vectorf_dot(a, a) * jo_vectorf_dot(b, b)));
}

/*
██╗  ██╗██╗  ██╗██╗  ██╗    ███╗   ███╗ █████╗ ████████╗██████╗ ██╗██╗  ██╗
██║  ██║╚██╗██╔╝██║  ██║    ████╗ ████║██╔══██╗╚══██╔══╝██╔══██╗██║╚██╗██╔╝
███████║ ╚███╔╝ ███████║    ██╔████╔██║███████║   ██║   ██████╔╝██║ ╚███╔╝
╚════██║ ██╔██╗ ╚════██║    ██║╚██╔╝██║██╔══██║   ██║   ██╔══██╗██║ ██╔██╗
     ██║██╔╝ ██╗     ██║    ██║ ╚═╝ ██║██║  ██║   ██║   ██║  ██║██║██╔╝ ██╗
     ╚═╝╚═╝  ╚═╝     ╚═╝    ╚═╝     ╚═╝╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝╚═╝╚═╝  ╚═╝

*/

/** @brief Creates the identity matrix (using fixed numbers)
 *  @param result Result matrix
 */
static __jo_force_inline void   jo_matrix_identity(jo_matrix * const result)
{
    register int                i;

    for (i = 1; i < 15; ++i)
        JO_ZERO(result->table[i]);
    result->m00 = JO_FIXED_1;
    result->m11 = JO_FIXED_1;
    result->m22 = JO_FIXED_1;
    result->m33 = JO_FIXED_1;
}

/** @brief Creates the identity matrix (using floating numbers)
 *  @param result Result matrix
 */
static __jo_force_inline void   jo_matrixf_identity(jo_matrixf * const result)
{
    register int                i;

    for (i = 1; i < 15; ++i)
        result->table[i] = 0;
    result->m00 = 1;
    result->m11 = 1;
    result->m22 = 1;
    result->m33 = 1;
}

/** @brief Creates translation matrix (using fixed numbers)
 *  @param offset Offset vector
 *  @param result Result matrix
 */
static __jo_force_inline void jo_matrix_translation(const jo_vector_fixed * const offset, jo_matrix * const result)
{
    jo_matrix_identity(result);
    result->m30 = offset->x;
    result->m31 = offset->y;
    result->m32 = offset->z;
}

/** @brief Creates translation matrix (using floating numbers)
 *  @param offset Offset vector
 *  @param result Result matrix
 */
static __jo_force_inline void jo_matrixf_translation(const jo_vectorf * const offset, jo_matrixf * const result)
{
    jo_matrixf_identity(result);
    result->m30 = offset->x;
    result->m31 = offset->y;
    result->m32 = offset->z;
}

/** @brief Creates scaling matrix (using fixed numbers)
 *  @param scale Scale vector
 *  @param result Result matrix
 */
static __jo_force_inline void jo_matrix_scaling(const jo_vector_fixed * const scale, jo_matrix * const result)
{
    jo_matrix_identity(result);
    result->m00 = scale->x;
    result->m11 = scale->y;
    result->m22 = scale->z;
}

/** @brief Creates scaling matrix (using floating numbers)
 *  @param scale Scale vector
 *  @param result Result matrix
 */
static __jo_force_inline void jo_matrixf_scaling(const jo_vectorf * const scale, jo_matrixf * const result)
{
    jo_matrixf_identity(result);
    result->m00 = scale->x;
    result->m11 = scale->y;
    result->m22 = scale->z;
}

/** @brief Creates rotating matrix (X axis) (using fixed numbers)
 *  @param angle_in_rad Angle in radiant
 *  @param result Result matrix
 */
static __jo_force_inline void jo_matrix_rotation_x_rad(const float angle_in_rad, jo_matrix * const result)
{
    jo_matrix_identity(result);
    // The float angle is converted once (same values as jo_cos_rad() and jo_sin_rad())
    jo_sincos_angle(jo_fixed_rad2ANGLE(jo_float2fixed(angle_in_rad)), &result->m12, &result->m11);
    result->m21 = -result->m12;
    result->m22 = result->m11;
}

/** @brief Creates rotating matrix (X axis) (using floating numbers)
 *  @param angle_in_rad Angle in radiant
 *  @param result Result matrix
 */
static __jo_force_inline void jo_matrixf_rotation_x_rad(const float angle_in_rad, jo_matrixf * const result)
{
    jo_matrixf_identity(result);
    result->m11 = jo_cos_radf(angle_in_rad);
    result->m12 = jo_sin_radf(angle_in_rad);
    result->m21 = -result->m12;
    result->m22 = result->m11;
}

/** @brief Creates rotating matrix (Y axis) (using fixed numbers)
 *  @param angle_in_rad Angle in radiant
 *  @param result Result matrix
 */
static __jo_force_inline void jo_matrix_rotation_y_rad(const float angle_in_rad, jo_matrix * const result)
{
    jo_matrix_identity(result);
    // The float angle is converted once (same values as jo_cos_rad() and jo_sin_rad())
    jo_sincos_angle(jo_fixed_rad2ANGLE(jo_float2fixed(angle_in_rad)), &result->m20, &result->m00);
    result->m02 = -result->m20;
    result->m22 = result->m00;
}

/** @brief Creates rotating matrix (Y axis) (using floating numbers)
 *  @param angle_in_rad Angle in radiant
 *  @param result Result matrix
 */
static __jo_force_inline void jo_matrixf_rotation_y_rad(const float angle_in_rad, jo_matrixf * const result)
{
    jo_matrixf_identity(result);
    result->m00 = jo_cos_radf(angle_in_rad);
    result->m20 = jo_sin_radf(angle_in_rad);
    result->m02 = -result->m20;
    result->m22 = result->m00;
}

/** @brief Creates rotating matrix (Z axis) (using fixed numbers)
 *  @param angle_in_rad Angle in radiant
 *  @param result Result matrix
 */
static __jo_force_inline void jo_matrix_rotation_z_rad(const float angle_in_rad, jo_matrix * const result)
{
    jo_matrix_identity(result);
    // The float angle is converted once (same values as jo_cos_rad() and jo_sin_rad())
    jo_sincos_angle(jo_fixed_rad2ANGLE(jo_float2fixed(angle_in_rad)), &result->m01, &result->m00);
    result->m10 = -result->m01;
    result->m11 = result->m00;
}

/** @brief Creates rotating matrix (Z axis) (using floating numbers)
 *  @param angle_in_rad Angle in radiant
 *  @param result Result matrix
 */
static __jo_force_inline void jo_matrixf_rotation_z_rad(const float angle_in_rad, jo_matrixf * const result)
{
    jo_matrixf_identity(result);
    result->m00 = jo_cos_radf(angle_in_rad);
    result->m01 = jo_sin_radf(angle_in_rad);
    result->m10 = -result->m01;
    result->m11 = result->m00;
}

/** @brief Creates transpose matrix (using fixed numbers)
 *  @param matrix Input matrix
 *  @param result Result matrix
 *  @warning matrix and result parameters must not be the same address
 */
static __jo_force_inline void jo_matrix_transpose(const jo_matrix * const matrix, jo_matrix * const result)
{
    result->m00 = matrix->m00;
    result->m10 = matrix->m01;
    result->m20 = matrix->m02;
    result->m30 = matrix->m03;
    result->m01 = matrix->m10;
    result->m11 = matrix->m11;
    result->m21 = matrix->m12;
    result->m31 = matrix->m13;
    result->m02 = matrix->m20;
    result->m12 = matrix->m21;
    result->m22 = matrix->m22;
    result->m32 = matrix->m23;
    result->m03 = matrix->m30;
    result->m13 = matrix->m31;
    result->m23 = matrix->m32;
    result->m33 = matrix->m33;
}

/** @brief Creates transpose matrix (using floating numbers)
 *  @param matrix Input matrix
 *  @param result Result matrix
 *  @warning matrix and result parameters must not be the same address
 */
static __jo_force_inline void jo_matrixf_transpose(const jo_matrixf * const matrix, jo_matrixf * const result)
{
    result->m00 = matrix->m00;
    result->m10 = matrix->m01;
    result->m20 = matrix->m02;
    result->m30 = matrix->m03;
    result->m01 = matrix->m10;
    result->m11 = matrix->m11;
    result->m21 = matrix->m12;
    result->m31 = matrix->m13;
    result->m02 = matrix->m20;
    result->m12 = matrix->m21;
    result->m22 = matrix->m22;
    result->m32 = matrix->m23;
    result->m03 = matrix->m30;
    result->m13 = matrix->m31;
    result->m23 = matrix->m32;
    result->m33 = matrix->m33;
}

/** @brief Multiply a matrix by a vector4 (using fixed numbers)
 *  @param m Matrix
 *  @param v Vector4
 *  @param result Result vector4
 *  @warning v and result parameters can be the same address
 */
static __jo_force_inline void   jo_matrix_mul_vector4(const jo_matrix * const m, const jo_vector4_fixed * const v, jo_vector4_fixed * const result)
{
    result->x = jo_fixed_mult(m->m00, v->x) + jo_fixed_mult(m->m10, v->y) + jo_fixed_mult(m->m20, v->z) + jo_fixed_mult(m->m30, v->w);
    result->y = jo_fixed_mult(m->m01, v->x) + jo_fixed_mult(m->m11, v->y) + jo_fixed_mult(m->m21, v->z) + jo_fixed_mult(m->m31, v->w);
    result->z = jo_fixed_mult(m->m02, v->x) + jo_fixed_mult(m->m12, v->y) + jo_fixed_mult(m->m22, v->z) + jo_fixed_mult(m->m32, v->w);
    result->w = jo_fixed_mult(m->m03, v->x) + jo_fixed_mult(m->m13, v->y) + jo_fixed_mult(m->m23, v->z) + jo_fixed_mult(m->m33, v->w);
}

/** @brief Dot product of 2 vectors of 4 fixed numbers (Internal usage)
 *  @param ptA First vector (4 contiguous fixed numbers)
 *  @param ptB Second vector (4 contiguous fixed numbers)
 *  @return ptA · ptB
 *  @remarks Sum on 64 bits by the multiply and accumulate unit (mac.l), like jo_fixed_dot()
 */
static __jo_force_inline jo_fixed       __jo_fixed_dot4(const jo_fixed ptA[4], const jo_fixed ptB[4])
{
    const jo_fixed                      *a;
    const jo_fixed                      *b;
    jo_fixed                            high;
    jo_fixed                            low;

    a = ptA;
    b = ptB;
    __asm__ ("clrmac\n\t"
             "mac.l @%2+, @%3+\n\t"
             "mac.l @%2+, @%3+\n\t"
             "mac.l @%2+, @%3+\n\t"
             "mac.l @%2+, @%3+\n\t"
             "sts mach, %0\n\t"
             "sts macl, %1\n\t"
             "xtrct %0, %1"
             : "=&r" (high), "=&r" (low), "+r" (a), "+r" (b)
             : "m" (*(const jo_fixed (*)[4])ptA), "m" (*(const jo_fixed (*)[4])ptB)
             : "mach", "macl");
    return (low);
}

/** @brief Multiply 2 matrix (using fixed numbers)
 *  @param a First matrix
 *  @param b Second matrix
 *  @param result Result matrix
 *  @warning a, b and result parameters must not be the same address
 *  @remarks Each element is a sum of 4 products on 64 bits (mac.l). The sum was a float before: slow (no FPU) and
 *  only 24 bits of precision
 */
static __jo_force_inline void   jo_matrix_mul(const jo_matrix * const a, const jo_matrix * const b, jo_matrix * const result)
{
    jo_matrix                   at;
    register int                i;
    register int                j;

    // The columns of a become contiguous rows: result->m[i][j] = sum of a->m[k][j] * b->m[i][k]
    jo_matrix_transpose(a, &at);
    for (i = 0; i < 4; ++i)
        for (j = 0; j < 4; ++j)
            result->m[i][j] = __jo_fixed_dot4(at.m[j], b->m[i]);
}

/** @brief Multiply 2 matrix (using floating numbers)
 *  @param a First matrix
 *  @param b Second matrix
 *  @param result Result matrix
 *  @warning a, b and result parameters must not be the same address
 */
static __jo_force_inline void   jo_matrixf_mul(const jo_matrixf * const a, const jo_matrixf * const b, jo_matrixf * const result)
{
    register int                i;
    register int                j;
    register int                k;
    register float              sum;

    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            sum = 0;
            for (k = 0; k < 4; ++k)
                sum += a->m[k][j] * b->m[i][k];
            result->m[i][j] = sum;
        }
    }
}

/** @brief Creates a matrix to rotate around an axis by a given angle in radiant (using floating numbers)
 *  @param angle_in_rad Angle in radiant
 *  @param axis Axis
 *  @param result Result matrix
 */
static __jo_force_inline void   jo_matrixf_rotation(const float angle_in_rad, const jo_vectorf * const axis, jo_matrixf * const result)
{
    jo_vectorf                   normalized_axis;

    jo_vectorf_normalize(axis, &normalized_axis);
    float c = jo_cos_radf(angle_in_rad);
    float s = jo_sin_radf(angle_in_rad);
    float a = 1 - c;
    jo_matrixf_identity(result);
    result->m00 = c + normalized_axis.x * normalized_axis.x * a;
    result->m10 = normalized_axis.x * normalized_axis.y * a - normalized_axis.z * s;
    result->m20 = normalized_axis.x * normalized_axis.z * a + normalized_axis.y * s;
    result->m01 = normalized_axis.y * normalized_axis.x * a + normalized_axis.z * s;
    result->m11 = c + normalized_axis.y * normalized_axis.y * a;
    result->m21 = normalized_axis.y * normalized_axis.z * a - normalized_axis.x * s;
    result->m02 = normalized_axis.z * normalized_axis.x * a - normalized_axis.y * s;
    result->m12 = normalized_axis.z * normalized_axis.y * a + normalized_axis.x * s;
    result->m22 = c + normalized_axis.z * normalized_axis.z * a;
}

/** @brief Creates an orthographic projection matrix (using floating numbers)
 *  @param left Left (Cube)
 *  @param right Right (Cube)
 *  @param bottom Bottom (Cube)
 *  @param top Top (Cube)
 *  @param back Back (Cube)
 *  @param front Front (Cube)
 *  @param result Result matrix
 */
static __jo_force_inline void jo_matrixf_ortho(const float left, const float right, const float bottom, const float top,
                                                   const float back, const float front, jo_matrixf * const result)
{
    jo_matrixf_identity(result);
    result->m00 = 2 / (right - left);
    result->m11 = 2 / (top - bottom);
    result->m22 = 2 / (back - front);
    result->m30 = -(right + left) / (right - left);
    result->m31 = -(top + bottom) / (top - bottom);
    result->m32 = -(back + front) / (back - front);
}

/** @brief Creates a perspective projection matrix for a camera (using floating numbers)
 *  @param vertical_field_of_view_in_deg Specifies the field of view angle, in degrees, in the y direction.
 *  @param aspect_ratio Specifies the aspect ratio that determines the field of view in the x direction. The aspect ratio is the ratio of x (width) to y (height).
 *  @param near_view_distance Specifies the distance from the viewer to the near clipping plane (always positive).
 *  @param far_view_distance Specifies the distance from the viewer to the far clipping plane (always positive).
 *  @param result Result matrix
 *  @remarks Similar to gluPerspective
 */
static __jo_force_inline void jo_matrixf_perspective(const float vertical_field_of_view_in_deg, const float aspect_ratio,
                                                    const float near_view_distance, const float far_view_distance, jo_matrixf * const result)
{
    jo_matrixf_identity(result);
    result->m11 = 1.0f / jo_tan_radf(vertical_field_of_view_in_deg * (JO_PI / 360.0f));
    result->m00 = result->m11 / aspect_ratio;
    result->m22 = (far_view_distance + near_view_distance) / (near_view_distance - far_view_distance);
    result->m32 = (2 * far_view_distance * near_view_distance) / (near_view_distance - far_view_distance);
    result->m23 = -1;
    result->m33 = 0;
}

/** @brief Builds a transformation matrix for a camera (using floating numbers)
 *  @param from Viewpoint
 *  @param to target
 *  @param up Defines the direction that's upwards for the camera
 *  @param result Result matrix
 */
static __jo_force_inline void   jo_matrixf_look_at(const jo_vectorf * const from, const jo_vectorf * const to,
                                                     const jo_vectorf * const up, jo_matrixf * const result)
{
    jo_vectorf                   tmp;
    jo_vectorf                   x;
    jo_vectorf                   y;
    jo_vectorf                   z;

    jo_vectorf_sub(to, from, &tmp);
    jo_vectorf_normalize(&tmp, &tmp);
    jo_vectorf_muls(&tmp, -1, &z);
    jo_vectorf_cross(up, &z, &tmp);
    jo_vectorf_normalize(&tmp, &x);
    jo_vectorf_cross(&z, &x, &y);

    jo_matrixf_identity(result);
    result->m00 = x.x;
    result->m10 = x.y;
    result->m20 = x.z;
    result->m30 = -jo_vectorf_dot(from, &x);
    result->m01 = y.x;
    result->m11 = y.y;
    result->m21 = y.z;
    result->m31 = -jo_vectorf_dot(from, &y);
    result->m02 = z.x;
    result->m12 = z.y;
    result->m22 = z.z;
    result->m33 = -jo_vectorf_dot(from, &z);
}

/** @brief Inverts an affine transformation matrix (using floating numbers)
 *  @param matrix Matrix
 *  @param result Result matrix
 */
static __jo_force_inline void jo_matrixf_invert_affine(const jo_matrixf * const matrix, jo_matrixf * const result)
{
    float c00 = matrix->m11 * matrix->m22 - matrix->m12 * matrix->m21;
    float c10 = -(matrix->m01 * matrix->m22 - matrix->m02 * matrix->m21);
    float c20 =   matrix->m01 * matrix->m12 - matrix->m02 * matrix->m11;
    float det = matrix->m00 * c00 + matrix->m10 * c10 + matrix->m20 * c20;
    jo_matrixf_identity(result);
    if (JO_FABS(det) < JO_FLOAT_EPSILON)
        return;
    float c01 = -(matrix->m10 * matrix->m22 - matrix->m12 * matrix->m20);
    float c11 = matrix->m00 * matrix->m22 - matrix->m02 * matrix->m20;
    float c21 = -(matrix->m00 * matrix->m12 - matrix->m02 * matrix->m10);
    float c02 = matrix->m10 * matrix->m21 - matrix->m11 * matrix->m20;
    float c12 = -(matrix->m00 * matrix->m21 - matrix->m01 * matrix->m20);
    float c22 = matrix->m00 * matrix->m11 - matrix->m01 * matrix->m10;
    // One division (libgcc on the SH-2) and 9 multiplications instead of 9 divisions
    float inv_det = 1.0f / det;
    float i00 = c00 * inv_det;
    float i10 = c01 * inv_det;
    float i20 = c02 * inv_det;
    float i01 = c10 * inv_det;
    float i11 = c11 * inv_det;
    float i21 = c12 * inv_det;
    float i02 = c20 * inv_det;
    float i12 = c21 * inv_det;
    float i22 = c22 * inv_det;

    result->m00 = i00;
    result->m10 = i10;
    result->m20 = i20;
    result->m30 = -(i00 * matrix->m30 + i10 * matrix->m31 + i20 * matrix->m32);
    result->m01 = i01;
    result->m11 = i11;
    result->m21 = i21;
    result->m31 = -(i01 * matrix->m30 + i11 * matrix->m31 + i21 * matrix->m32);
    result->m02 = i02;
    result->m12 = i12;
    result->m22 = i22;
    result->m32 = -(i02 * matrix->m30 + i12 * matrix->m31 + i22 * matrix->m32);
}

/** @brief Multiplies a 4x4 matrix with a 3D vector representing a point in 3D space (using floating numbers)
 *  @param matrix Matrix
 *  @param position Position
 *  @param result Result matrix
 */
static __jo_force_inline void jo_matrixf_mul_pos(const jo_matrixf * const matrix, const jo_vectorf * const position,
                                                     jo_vectorf * const result)
{
    float w = matrix->m03 * position->x + matrix->m13 * position->y + matrix->m23 * position->z + matrix->m33;
    result->x = matrix->m00 * position->x + matrix->m10 * position->y + matrix->m20 * position->z + matrix->m30;
    result->y = matrix->m01 * position->x + matrix->m11 * position->y + matrix->m21 * position->z + matrix->m31;
    result->z = matrix->m02 * position->x + matrix->m12 * position->y + matrix->m22 * position->z + matrix->m32;
    if (w != 0 && w != 1)
    {
        // One division (libgcc on the SH-2) and 3 multiplications instead of 3 divisions
        float inv_w = 1.0f / w;
        result->x *= inv_w;
        result->y *= inv_w;
        result->z *= inv_w;
    }
}

/** @brief Multiplies a 4x4 matrix with a 3D vector representing a direction in 3D space (using floating numbers)
 *  @param matrix Matrix
 *  @param direction Direction
 *  @param result Result matrix
 */
static __jo_force_inline void jo_matrixf_mul_dir(const jo_matrixf * const matrix, const jo_vectorf * const direction,
                                                     jo_vectorf * const result)
{
    float w = matrix->m03 * direction->x + matrix->m13 * direction->y + matrix->m23 * direction->z;
    result->x = matrix->m00 * direction->x + matrix->m10 * direction->y + matrix->m20 * direction->z;
    result->y = matrix->m01 * direction->x + matrix->m11 * direction->y + matrix->m21 * direction->z;
    result->z = matrix->m02 * direction->x + matrix->m12 * direction->y + matrix->m22 * direction->z;
    if (w != 0 && w != 1)
    {
        // One division (libgcc on the SH-2) and 3 multiplications instead of 3 divisions
        float inv_w = 1.0f / w;
        result->x *= inv_w;
        result->y *= inv_w;
        result->z *= inv_w;
    }
}

/*
███╗   ███╗██╗███████╗ ██████╗
████╗ ████║██║██╔════╝██╔════╝
██╔████╔██║██║███████╗██║
██║╚██╔╝██║██║╚════██║██║
██║ ╚═╝ ██║██║███████║╚██████╗
╚═╝     ╚═╝╚═╝╚══════╝ ╚═════╝

*/

/** @brief Linear interpolation which guarantees v = v1 when t = 1
 *  @param v0 First value
 *  @param v1 Second value
 *  @param t T [0, 1]
 *  @return Interpolation between two inputs (v0, v1) for a parameter (t) in the closed unit interval [0, 1].
 */
static __jo_force_inline jo_fixed           jo_lerp(const jo_fixed v0, const jo_fixed v1, const jo_fixed t)
{
    return (jo_fixed_mult((JO_FIXED_1 - t), v0) + jo_fixed_mult(t, v1));
}

/** @brief Check if float almost equals 0
 *  @param f floating point number
 *  @return true if the float almost equals 0 otherwise false
 */
static  __jo_force_inline bool	jo_is_float_equals_zero(const float f)
{
    return (JO_ABS(f) < 0.00000001f);
}

/** @brief Fast method to get if two square intersects (HitBox processing)
 *  @param x1 Horizontal position of the first square
 *  @param y1 Vertical position of the first square
 *  @param w1 Width of the first square
 *  @param h1 Height of the first square
 *  @param x2 Horizontal position of the second square
 *  @param y2 Vertical position of the second square
 *  @param w2 Width of the second square
 *  @param h2 Height of the second square
 *  @return true if squares collides otherwise false
 */
static  __jo_force_inline bool	jo_square_intersect(const int x1, const int y1, const int w1, const int h1,
        const int x2, const int y2, const int w2, const int h2)
{
    return ((x1 < x2 + w2) && (x2 < x1 + w1)) && ((y1 < y2 + h2) && (y2 < y1 + h1));
}

/** @brief Get the greatest common divisor
 *  @remarks PGCD in French
 *  @param a First value (> 0)
 *  @param b Second value (> 0)
 *  @return GCD(a, b)
 */
int     jo_gcd(int a, int b);

/** @brief Rotate a point on the plan with a specific origin
 *  @param point Point to rotate
 *  @param origin Rotation origin
 *  @param angle Angle in degrees
 *  @param result The new point coordinate
 *  @remarks result and point parameters can be the same pointer
 */
void            jo_planar_rotate(const jo_pos2D * const point, const jo_pos2D * const origin, const int angle, jo_pos2D * const result);

/** @brief Convert jo_8_directions to angle in degree
 *  @param direction Direction
 *  @return Angle in degree
 */
static __jo_force_inline short jo_direction_to_angle(const jo_8_directions direction)
{
    switch (direction)
    {
    case LEFT:
        return (180);
    case RIGHT:
        return (0);
    case UP:
        return (270);
    case DOWN:
        return (90);
    case UP_LEFT:
        return (225);
    case UP_RIGHT:
        return (315);
    case DOWN_LEFT:
        return (135);
    case DOWN_RIGHT:
        return (45);
    default:
        return (0);
    }
}

#endif /* !__JO_MATH_H__ */

/*
** END OF FILE
*/

