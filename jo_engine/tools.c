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
#include <stdarg.h>
#include "jo/conf.h"
#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/time.h"
#include "jo/core.h"
#include "jo/tools.h"
#include "jo/math.h"
#include "jo/colors.h"
#include "jo/image.h"
#include "jo/vdp2.h"

/*
** GLOBALS
*/
char			            __jo_sprintf_buf[JO_PRINTF_BUF_SIZE];

/** @brief Write an unsigned number in a base (10 or 16), backward from the end of a buffer
 *  @return The first character
 */
static char                 *__jo_format_unsigned(unsigned int value, const unsigned int base, const char * const digits, char *end)
{
    // (constant divisors: a multiplication, no call of the division of the C library)
    if (base == 16)
    {
        do
        {
            *--end = digits[value & 15];
            value >>= 4;
        } while (value != 0);
        return (end);
    }
    do
    {
        *--end = digits[value % 10u];
        value /= 10u;
    } while (value != 0);
    return (end);
}

int                         jo_vsnprintf(char * const str, const unsigned int size, const char * format, va_list args)
{
    char                    number[12];
    char                    *out;
    char                    *limit;
    const char              *text;
    int                     length;
    int                     width;
    int                     precision;
    int                     zeros;
    int                     value;
    bool                    left;
    bool                    zero;
    char                    sign;
    char                    plus;
    char                    c;

// (the characters after size - 1 are not written: the count is the length of the string written)
# define JO_SPRINTF_PUT(C)  do { if (out < limit) *out++ = (C); } while (0)

    out = str;
    limit = size == 0 ? str : str + size - 1;
    for (; *format != '\0'; ++format)
    {
        if (*format != '%')
        {
            JO_SPRINTF_PUT(*format);
            continue;
        }
        ++format;
        left = false;
        zero = false;
        plus = '\0';
        for (;; ++format)
        {
            if (*format == '-')
                left = true;
            else if (*format == '0')
                zero = true;
            else if (*format == '+')
                plus = '+';
            else if (*format == ' ' && plus == '\0')
                plus = ' ';
            else
                break;
        }
        for (JO_ZERO(width); *format >= '0' && *format <= '9'; ++format)
            width = width * 10 + *format - '0';
        // Precision: the minimum of digits of a number, the maximum of characters of a string
        precision = -1;
        if (*format == '.')
        {
            for (JO_ZERO(precision), ++format; *format >= '0' && *format <= '9'; ++format)
                precision = precision * 10 + *format - '0';
        }
        // (the long type is an int: the l and h modifiers change nothing)
        while (*format == 'l' || *format == 'h')
            ++format;
        sign = '\0';
        c = *format;
        switch (c)
        {
            case 'd':
            case 'i':
                value = va_arg(args, int);
                sign = value < 0 ? '-' : plus;
                text = __jo_format_unsigned(value < 0 ? 0u - (unsigned int)value : (unsigned int)value, 10, "0123456789", number + sizeof(number));
                break;
            case 'u':
                text = __jo_format_unsigned(va_arg(args, unsigned int), 10, "0123456789", number + sizeof(number));
                break;
            case 'x':
                text = __jo_format_unsigned(va_arg(args, unsigned int), 16, "0123456789abcdef", number + sizeof(number));
                break;
            case 'X':
            case 'p':
                text = __jo_format_unsigned(va_arg(args, unsigned int), 16, "0123456789ABCDEF", number + sizeof(number));
                break;
            case 'c':
                number[0] = (char)va_arg(args, int);
                text = number;
                zero = false;
                break;
            case 's':
                text = va_arg(args, const char *);
                if (text == JO_NULL)
                    text = "(null)";
                zero = false;
                break;
            case '%':
                JO_SPRINTF_PUT('%');
                continue;
            case '\0':
                --format;
                continue;
            case 'f':
            case 'F':
            case 'e':
            case 'E':
            case 'g':
            case 'G':
                // (not supported: written as it is, its argument skipped, the next ones are right)
                (void)va_arg(args, double);
                JO_SPRINTF_PUT('%');
                JO_SPRINTF_PUT(c);
                continue;
            default:
                // (not supported: written as it is)
                JO_SPRINTF_PUT('%');
                JO_SPRINTF_PUT(c);
                continue;
        }
        JO_ZERO(zeros);
        if (c == 'c')
            length = 1;
        else if (c == 's')
        {
            length = (int)jo_strlen(text);
            if (precision >= 0 && length > precision)
                length = precision;
        }
        else
        {
            length = (int)(number + sizeof(number) - text);
            if (precision >= 0)
            {
                // (a precision: no padding with 0 by the width)
                zero = false;
                if (precision > length)
                    zeros = precision - length;
            }
        }
        width -= length + zeros + (sign != '\0' ? 1 : 0);
        if (sign != '\0' && zero)
            JO_SPRINTF_PUT(sign);
        if (!left)
            for (; width > 0; --width)
                JO_SPRINTF_PUT(zero ? '0' : ' ');
        if (sign != '\0' && !zero)
            JO_SPRINTF_PUT(sign);
        for (; zeros > 0; --zeros)
            JO_SPRINTF_PUT('0');
        for (; length > 0; --length)
            JO_SPRINTF_PUT(*text++);
        for (; width > 0; --width)
            JO_SPRINTF_PUT(' ');
    }
    if (size != 0)
        *out = '\0';
# undef JO_SPRINTF_PUT
    return ((int)(out - str));
}

int                         jo_vsprintf(char * const str, const char * format, va_list args)
{
    return (jo_vsnprintf(str, 0x7FFFFFFFu, format, args));
}

int                         jo_snprintf(char * const str, const unsigned int size, const char * const format, ...)
{
    va_list                 args;
    int                     length;

    va_start(args, format);
    length = jo_vsnprintf(str, size, format, args);
    va_end(args);
    return (length);
}

int                         jo_sprintf(char * const str, const char * const format, ...)
{
    va_list                 args;
    int                     length;

    va_start(args, format);
    length = jo_vsprintf(str, format, args);
    va_end(args);
    return (length);
}

#ifdef JO_COMPILE_WITH_PRINTF_SUPPORT

void            __jo_vdp2_print(const int x, const int y, const char * str);

void            jo_print(int x, int y, char * str)
{
#ifdef JO_DEBUG
    if (str == JO_NULL)
    {
        jo_core_error("str is null");
        return;
    }
#endif
    __jo_vdp2_print(x, y, str);
}

void                    jo_clear_screen_line(const int y)
{
    __jo_vdp2_print(0, y, "                                            ");
}

void                    jo_clear_screen(void)
{
    register int        y;

    for (JO_ZERO(y); y < JO_DIV_BY_8(__jo_screen.background_height); ++y)
        jo_clear_screen_line(y);
}

#endif // JO_COMPILE_WITH_PRINTF_SUPPORT


/** @brief Convert a BCD value to an integer
 *  @param bcd Binary coded decimal value (0x1234 = 1234)
 *  @return Integer value
 */
static unsigned int             jo_bcd_to_int(unsigned int bcd)
{
    unsigned int                result;
    unsigned int                factor;

    for (JO_ZERO(result), factor = 1; bcd; bcd >>= 4, factor *= 10)
        result += (bcd & 0xf) * factor;
    return (result);
}

void                    jo_getdate(jo_datetime *now)
{
#ifdef JO_DEBUG
   if (now == JO_NULL)
   {
	 	jo_core_error("now is null");
	 	return;
   }
#endif
    // The SMPC status is refreshed every frame
    now->year = (unsigned short)jo_bcd_to_int(__jo_smpc_status.year);
    now->month = (unsigned char)(__jo_smpc_status.month & 0x0f);
    now->week = (unsigned char)(__jo_smpc_status.month >> 4);
    now->day = (unsigned char)jo_bcd_to_int(__jo_smpc_status.day);
    now->hour = (unsigned char)jo_bcd_to_int(__jo_smpc_status.hour);
    now->minute = (unsigned char)jo_bcd_to_int(__jo_smpc_status.minute);
    now->second = (unsigned char)jo_bcd_to_int(__jo_smpc_status.second);
}


jo_language     jo_get_current_language(void)
{
    unsigned int    language;

    // The language is stored in the SMPC battery-backed memory
    language = (__jo_smpc_status.smpc_memory >> 24) & 0x0f;
    if (language > Japanese)
        return (English);
    return ((jo_language)language);
}

/*
** This is a fast unsafe(doesn't check JO_NULL param, etc.) implementation of atoi
*/
inline int				jo_tools_atoi(const char * restrict str)
{
    int					val;
    bool		        is_negative;

    JO_ZERO(val);
    is_negative = false;
#ifdef JO_DEBUG
    if (str == JO_NULL)
    {
        jo_core_error("str is null");
        return (0);
    }
#endif
    // (the spaces before, a sign, then the digits until another character)
    while (jo_tools_is_whitespace(*str))
        ++str;
    if (*str == '-' || *str == '+')
        is_negative = *str++ == '-';
    while (*str >= '0' && *str <= '9')
        val = val * 10 + (*str++ - '0');
    if (is_negative)
        return (-val);
    return (val);
}

inline int				jo_strlen(const char * restrict str)
{
    char                *begin;

#ifdef JO_DEBUG
    if (str == JO_NULL)
    {
        jo_core_error("str is null");
        return (0);
    }
#endif
    for (begin = (char *)str; *str; ++str)
        ;
    return (str - begin);
}

inline bool             jo_endwith(const char * restrict str, const char * restrict end)
{
    int                 str_len;
    int                 end_len;
    register int        i;

#ifdef JO_DEBUG
    if (str == JO_NULL)
    {
        jo_core_error("str is null");
        return (0);
    }
    if (end == JO_NULL)
    {
        jo_core_error("end is null");
        return (0);
    }
#endif
    str_len = jo_strlen(str);
    end_len = jo_strlen(end);
    if (str_len < end_len)
        return (false);
    str += (str_len - end_len);
    for (JO_ZERO(i); i < end_len; ++i)
    {
        if (str[i] != end[i])
            return (false);
    }
    return (true);
}

unsigned int                        jo_lz_decompress(const unsigned char * const src, unsigned char * const dst)
{
    register const unsigned char    *s;
    register unsigned char          *d;
    const unsigned char             *m;
    unsigned char                   *end;
    unsigned int                    size;
    unsigned int                    flags;
    unsigned int                    bits;
    unsigned int                    v;
    unsigned int                    n;

    size = jo_lz_size(src);
    s = src + 4;
    d = dst;
    end = dst + size;
    JO_ZERO(flags);
    JO_ZERO(bits);
    while (d < end)
    {
        if (bits == 0)
        {
            flags = *s++;
            bits = 8;
        }
        if (flags & 0x80)
            *d++ = *s++;
        else
        {
            v = ((unsigned int)s[0] << 8) | s[1];
            s += 2;
            m = d - ((v >> 4) + 1);
            for (n = (v & 15) + 3; n > 0 && d < end; --n)
                *d++ = *m++;
        }
        flags <<= 1;
        --bits;
    }
    return (size);
}

int                                 jo_memcmp(const void * const p1, const void * const p2, unsigned int num)
{
    register const unsigned char    *s1 = (const unsigned char *)p1;
    register const unsigned char    *s2 = (const unsigned char *)p2;

    for (; num > 0; --num, ++s1, ++s2)
        if (*s1 != *s2)
            return (*s1 - *s2);
    return (0);
}

int                                 jo_strcmp(const char * restrict p1, const char * restrict p2)
{
    register const unsigned char    *s1 = (const unsigned char *)p1;
    register const unsigned char    *s2 = (const unsigned char *)p2;
    unsigned char                   c1;
    unsigned char                   c2;

    do
    {
        c1 = (unsigned char)*s1++;
        c2 = (unsigned char)*s2++;
        if (!c1)
            return (c1 - c2);
    }
    while (c1 == c2);
    return (c1 - c2);
}

void                        jo_memset(const void * const restrict ptr, const int value, unsigned int num)
{
    register unsigned int   * restrict p;
    register unsigned char  * restrict pp;
    unsigned int            tail;
    unsigned int            x;
    unsigned char           xx;

    if(num == 0)
        return;

    x = value & 0xff;
    xx = value & 0xff;
    pp = (unsigned char*)ptr;
    // (num must not wrap around when it is smaller than the misalignment)
    while (((unsigned int)pp & 3) && num != 0)
    {
        *pp++ = xx;
        --num;
    }
    p = (unsigned int*)pp;
    tail = num & 3;
    x |= x << 8;
    x |= x << 16;
    num >>= 2;
    // 4 words per iteration (fewer loop instructions)
    for (; num >= 4; num -= 4)
    {
        p[0] = x;
        p[1] = x;
        p[2] = x;
        p[3] = x;
        p += 4;
    }
    while (num--)
        *p++ = x;
    pp = (unsigned char*)p;
    while (tail--)
        *pp++ = xx;
}

void                            jo_memcpy(void * const restrict dst, const void * const restrict src, unsigned int size)
{
    register unsigned char          *d8;
    register const unsigned char    *s8;
    register unsigned int           *d32;
    register const unsigned int     *s32;
    register unsigned short         *d16;
    register const unsigned short   *s16;

    d8 = (unsigned char *)dst;
    s8 = (const unsigned char *)src;
    // Same misalignment: the first bytes are copied one by one, then the 32 bits (or 16 bits) copy is used
    if (((((unsigned int)d8) ^ ((unsigned int)s8)) & 3) == 0)
    {
        while ((((unsigned int)d8) & 3) && size)
        {
            *d8++ = *s8++;
            --size;
        }
    }
    else if (((((unsigned int)d8) ^ ((unsigned int)s8)) & 1) == 0 && (((unsigned int)d8) & 1) && size)
    {
        *d8++ = *s8++;
        --size;
    }
    if (((((unsigned int)d8) | ((unsigned int)s8)) & 3) == 0)
    {
        d32 = (unsigned int *)d8;
        s32 = (const unsigned int *)s8;
        for (; size >= 16; size -= 16)
        {
            d32[0] = s32[0];
            d32[1] = s32[1];
            d32[2] = s32[2];
            d32[3] = s32[3];
            d32 += 4;
            s32 += 4;
        }
        for (; size >= 4; size -= 4)
            *d32++ = *s32++;
        d8 = (unsigned char *)d32;
        s8 = (const unsigned char *)s32;
    }
    else if (((((unsigned int)d8) | ((unsigned int)s8)) & 1) == 0)
    {
        d16 = (unsigned short *)d8;
        s16 = (const unsigned short *)s8;
        // (unrolled like the 32 bits copy: the keys of the commands of the slave CPU, for instance)
        for (; size >= 8; size -= 8)
        {
            d16[0] = s16[0];
            d16[1] = s16[1];
            d16[2] = s16[2];
            d16[3] = s16[3];
            d16 += 4;
            s16 += 4;
        }
        for (; size >= 2; size -= 2)
            *d16++ = *s16++;
        d8 = (unsigned char *)d16;
        s8 = (const unsigned char *)s16;
    }
    while (size--)
        *d8++ = *s8++;
}

/*
** DMA (DMA controller of the SH-2)
*/

/** @brief Smallest copy done by the DMA controller (the CPU is faster for smaller copies) */
# define JO_DMA_MIN_SIZE                (64)

/** @brief Check if a copy is in progress on the DMA channel 0 of the current CPU
 *  @return true if the copy is not finished
 */
static  __jo_force_inline bool  jo_dma_busy(void)
{
    return ((JO_SH2_CHCR0 & (JO_SH2_CHCR_ENABLE | JO_SH2_CHCR_TRANSFER_END)) == JO_SH2_CHCR_ENABLE);
}

/** @brief Invalidate the cache lines of a memory area written by the DMA controller
 *  @param addr Address
 *  @param size Size in bytes
 *  @remarks The cache is write-through: the lines are only invalidated (the cache-through areas are ignored)
 */
static void                     jo_dma_purge_cache(const void * const addr, const unsigned int size)
{
    register unsigned int       a;
    unsigned int                end;

    a = (unsigned int)addr;
    // Only the cache area (0x00000000 to 0x1FFFFFFF) uses the cache
    if (a >= 0x20000000)
        return;
    end = a + size;
    for (a &= ~(JO_SH2_CACHE_LINE_SIZE - 1); a < end; a += JO_SH2_CACHE_LINE_SIZE)
        *(volatile unsigned int *)(JO_SH2_CACHE_PURGE_AREA | a) = 0;
}

/** @brief Start a transfer on the DMA channel 0 of the current CPU
 *  @param src Source
 *  @param dest Destination
 *  @param size Size in bytes
 *  @param src_mode JO_SH2_CHCR_SRC_INCREMENT or 0 (the source address doesn't change)
 *  @return false if the transfer is too small or unaligned (nothing is done)
 */
static bool                     jo_dma_start(const void * const src, void * const dest, const unsigned int size, const unsigned int src_mode)
{
    unsigned int                align;
    unsigned int                unit;
    unsigned int                chcr;
    unsigned int                sr;

    if (size < JO_DMA_MIN_SIZE)
        return (false);
    align = ((unsigned int)src) | ((unsigned int)dest) | size;
    if ((align & 3) == 0)
    {
        unit = 4;
        chcr = JO_SH2_CHCR_SIZE_32;
    }
    else if ((align & 1) == 0)
    {
        unit = 2;
        chcr = JO_SH2_CHCR_SIZE_16;
    }
    else
        return (false);
    // The lines of the destination must be loaded again from the memory after the transfer
    jo_dma_purge_cache(dest, size);
    // The previous transfer must be finished (an interrupt handler can also use the channel)
    for (;;)
    {
        jo_dma_wait();
        sr = __jo_core_disable_interrupts();
        if (!jo_dma_busy())
            break;
        __jo_core_restore_interrupts(sr);
    }
    // TE has been read by jo_dma_busy(): writing 0 clears it
    JO_SH2_CHCR0 = 0;
    JO_SH2_SAR0 = (unsigned int)src;
    JO_SH2_DAR0 = (unsigned int)dest;
    // (a shift: the division was a call of the library of the compiler)
    JO_SH2_TCR0 = unit == 4 ? JO_DIV_BY_4(size) : JO_DIV_BY_2(size);
    // Clears the address error and NMI flags (read, then write 0) and enables the DMA
    if (JO_SH2_DMAOR != JO_SH2_DMAOR_ENABLE)
        JO_SH2_DMAOR = JO_SH2_DMAOR_ENABLE;
    JO_SH2_CHCR0 = JO_SH2_CHCR_DEST_INCREMENT | src_mode | chcr | JO_SH2_CHCR_AUTO_REQUEST | JO_SH2_CHCR_ENABLE;
    __jo_core_restore_interrupts(sr);
    return (true);
}

void                            jo_dma_wait(void)
{
    while (jo_dma_busy())
        ;
}

void                            jo_dma_copy_async(const void * const src, void * const dest, const unsigned int size)
{
    if (jo_dma_start(src, dest, size, JO_SH2_CHCR_SRC_INCREMENT))
        return;
    // Small or unaligned: the CPU copies it (after the previous transfer, to keep the order)
    jo_dma_wait();
    jo_memcpy(dest, src, size);
}

void                            jo_dma_copy(const void * const src, void * const dest, const unsigned int size)
{
    if (!jo_dma_start(src, dest, size, JO_SH2_CHCR_SRC_INCREMENT))
    {
        jo_dma_wait();
        jo_memcpy(dest, src, size);
        return;
    }
    jo_dma_wait();
    // The CPU may have loaded a line shared with the destination during the transfer
    jo_dma_purge_cache(dest, size);
}

void                            jo_dma_fill32(void * const dest, const unsigned int value, const unsigned int size)
{
    volatile unsigned int       source;
    register unsigned int       *ptr;
    register unsigned int       i;

    // The DMA controller reads the value in memory (the cache is write-through)
    source = value;
    if (jo_dma_start((const void *)&source, dest, size, 0))
    {
        jo_dma_wait();
        jo_dma_purge_cache(dest, size);
        return;
    }
    ptr = (unsigned int *)dest;
    for (JO_ZERO(i); i < JO_DIV_BY_4(size); ++i)
        ptr[i] = value;
}

void                            __jo_dma_read_port32(const volatile void * const port, void * const dest, const unsigned int size)
{
    register volatile unsigned int  *src;
    register unsigned int       *ptr;
    register unsigned int       i;

    if (jo_dma_start((const void *)port, dest, size, 0))
    {
        jo_dma_wait();
        jo_dma_purge_cache(dest, size);
        return;
    }
    src = (volatile unsigned int *)port;
    ptr = (unsigned int *)dest;
    for (JO_ZERO(i); i < JO_DIV_BY_4(size); ++i)
        ptr[i] = *src;
}

void                    jo_map_to_vram(unsigned short * restrict data, unsigned short * restrict vram_addr,
                                       unsigned short suuj, unsigned short suui, unsigned short palnum, unsigned int mapoff)
{
    unsigned short      j;
    unsigned short      paloff;

    paloff = palnum << 12;
    while (suui-- > 0)
    {
        for (JO_ZERO(j); j < suuj; ++j, ++data)
            *vram_addr++ = (*data | paloff) + mapoff;
        vram_addr += (64 - suuj);
    }
}

/*
** END OF FILE
*/
