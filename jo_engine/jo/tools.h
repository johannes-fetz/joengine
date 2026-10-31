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
/** @file tools.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Tools (printf, etc,)
 *  @bug No known bugs.
 */

#ifndef __JO_TOOLS_H__
# define __JO_TOOLS_H__

/*
** INCLUDES
*/
#include "math.h"
#include <stdarg.h>

/** @brief Fixed value of JO_TV_WIDTH */
#define JO_FIXED_TV_WIDTH   JO_INT_TO_FIXED(JO_TV_WIDTH)
/** @brief Fixed value of JO_TV_HEIGHT */
#define JO_FIXED_TV_HEIGHT  JO_INT_TO_FIXED(JO_TV_HEIGHT)

/** @brief Swap Short endian
  * @param value Little or Big endian short
  * @return Endian swapped short
  */
static  __jo_force_inline short        jo_swap_endian_short(short value)
{
    return ((((value) >> 8) & 0xff) | (((value) & 0xff) << 8));
}

/** @brief Swap Unsigned short endian
  * @param value Little or Big endian unsigned short
  * @return Endian swapped unsigned short
  */
static  __jo_force_inline unsigned short        jo_swap_endian_ushort(unsigned short value)
{
    return ((((value) >> 8) & 0xff) | (((value) & 0xff) << 8));
}

/** @brief Swap unsigned int endian
  * @param value Little or Big endian unsigned int
  * @return Endian swapped unsigned int
  */
static  __jo_force_inline unsigned int        jo_swap_endian_uint(unsigned int value)
{
    return ((((value) & 0xff000000) >> 24) | (((value) & 0x00ff0000) >>  8) | (((value) & 0x0000ff00) <<  8) | (((value) & 0x000000ff) << 24));
}

/** @brief Swap int endian
  * @param value Little or Big endian int
  * @return Endian swapped int
  */
static  __jo_force_inline int        jo_swap_endian_int(int value)
{
    return ((((value) & 0xff000000) >> 24) | (((value) & 0x00ff0000) >>  8) | (((value) & 0x0000ff00) <<  8) | (((value) & 0x000000ff) << 24));
}

/** @brief Read a big endian unsigned short (16 bits) from any address (the bytes of a file, aligned or not)
  * @param p Address of the first byte
  * @return Value
  */
static  __jo_force_inline unsigned short        jo_read_big_endian_ushort(const unsigned char * const p)
{
    return ((unsigned short)((p[0] << 8) | p[1]));
}

/** @brief Read a big endian unsigned int (32 bits) from any address (the bytes of a file, aligned or not)
  * @param p Address of the first byte
  * @return Value
  */
static  __jo_force_inline unsigned int        jo_read_big_endian_uint(const unsigned char * const p)
{
    return (((unsigned int)p[0] << 24) | ((unsigned int)p[1] << 16) | ((unsigned int)p[2] << 8) | p[3]);
}

/** @brief sprintf prototypes
 *  @param str String
 *  @param format Format string
 *  @param ... Arguments
 *  @return If successful, the total number of characters written is returned excluding the null-character appended at the end of the string, otherwise a negative number is returned in case of failure.
 */
int				sprintf(char* str, const char* format, ...);

/** @brief A small sprintf of the engine (jo_printf()...): %d %i %u %x %X %c %s %%, the flags - and 0, a width. Faster and
 *  smaller than the one of the C library (which handles the floats: %f is not supported here, written as it is)
 *  @param str String (big enough)
 *  @param format Format string
 *  @param ... Arguments
 *  @return The number of characters written (without the null character)
 */
int                 jo_sprintf(char * const str, const char * const format, ...);

/** @brief jo_sprintf() with a list of arguments
 *  @param str String (big enough)
 *  @param format Format string
 *  @param args Arguments
 *  @return The number of characters written (without the null character)
 */
int                 jo_vsprintf(char * const str, const char * format, va_list args);

#ifdef JO_DEBUG

#define jo_printf_debug(X, Y, fmt, args...) \
        jo_printf(X, Y, "%s(%s)#%d : " fmt , \
                __FILE__, __FUNCTION__, __LINE__, ## args)

#endif

/** @brief Internal sprintf buffer
 *  @warning MC Hammer: don't touch this
 */
extern char		        __jo_sprintf_buf[JO_PRINTF_BUF_SIZE];

#ifdef JO_COMPILE_WITH_PRINTF_SUPPORT


/** @brief Print implementation
 *  @param x Horizontal position from top left screen corner
 *  @param y Vertical position from top left screen corner
 *  @param str String
 */
void jo_print(int x, int y, char * str);

/** @brief printf implementation
 *  @param X Horizontal position from top left screen corner
 *  @param Y Vertical position from top left screen corner
 *  @param ... Arguments
 */
# define jo_printf(X, Y, ...)		do {jo_sprintf(__jo_sprintf_buf, __VA_ARGS__); jo_print(X, Y, __jo_sprintf_buf); } while(0)

/** @brief printf implementation
 *  @param X Horizontal position from top left screen corner
 *  @param Y Vertical position from top left screen corner
 *  @param COLOR_INDEX Color palette index
 *  @param ... Arguments
 */
# define jo_printf_with_color(X, Y, COLOR_INDEX, ...)  do { jo_set_printf_color_index(COLOR_INDEX); jo_printf(X, Y, __VA_ARGS__); } while(0)

/** @brief Printf palette (Internal usage) */
extern unsigned short   __jo_vdp2_printf_palette;

/** @brief Set printf foreground color index
  * @param index palette color index
  */
static  __jo_force_inline void        jo_set_printf_color_index(const unsigned char index)
{
    __jo_vdp2_printf_palette = (unsigned short)((index & 7) << 12);
}

/** @brief Clear screen (usefull for printf)
 *  @warning Try to not use it by adding some whitespace at the end of jo_printf
 */
void            jo_clear_screen(void);

/** @brief Clear single screen line (usefull for printf)
 *  @param y Vertical line index
 *  @warning Try to not use it by adding some whitespace at the end of jo_printf
 */
void            jo_clear_screen_line(const int y);

#endif // JO_COMPILE_WITH_PRINTF_SUPPORT

/** @brief Copies size bytes from the object pointed to by src to the object pointed to by dst
 *  @param dst Destination
 *  @param src Data source
 *  @param size Number of bytes to copy
 *  @remarks No return value in this implementation
 */
void            jo_memcpy(void * const restrict dst, const void * const restrict src, unsigned int size);

/** @brief Start a copy with the DMA controller of the current CPU (the function doesn't wait for the end of the copy)
  * @param src Data source
  * @param dest Destination
  * @param size Size in bytes
  * @remarks The CPU keeps working during the copy (the DMA controller steals bus cycles).
  *          Each CPU (master and slave) has its own DMA controller.
  * @remarks Small or unaligned (odd address or size) copies are done by the CPU with jo_memcpy() before returning.
  * @warning Don't change the source and don't read the destination before jo_dma_wait()
  */
void            jo_dma_copy_async(const void * const src, void * const dest, const unsigned int size);

/** @brief Wait for the end of the copy started by jo_dma_copy_async() on the current CPU
  * @remarks Returns immediately if no copy is in progress
  */
void            jo_dma_wait(void);

/** @brief Copy with the DMA controller of the current CPU and wait for the end of the copy
  * @param src Data source
  * @param dest Destination
  * @param size Size in bytes
  * @remarks Faster than jo_memcpy() for big copies, especially to VRAM or sound RAM.
  *          Small or unaligned (odd address or size) copies use jo_memcpy().
  */
void            jo_dma_copy(const void * const src, void * const dest, const unsigned int size);

/** @brief Fill memory with a 32 bits value using the DMA controller of the current CPU (waits for the end)
  * @param dest Destination (4 bytes aligned)
  * @param value Value
  * @param size Size in bytes (multiple of 4)
  */
void            jo_dma_fill32(void * const dest, const unsigned int value, const unsigned int size);

/** @brief Read a 32 bits I/O port into memory with the DMA controller of the current CPU (Internal usage)
  * @param port Port address (the address doesn't change)
  * @param dest Destination (4 bytes aligned)
  * @param size Size in bytes (multiple of 4)
  */
void            __jo_dma_read_port32(const volatile void * const port, void * const dest, const unsigned int size);

/** @brief NULL implementation
 */
# define JO_NULL                            ((void *)0)

/** @brief Mark function argument as unused
 */
# define JO_UNUSED_ARG(ARG)                     ((void)ARG)

/** @brief Check if the character is a whitespace
 *  @param c variable of char type
 *  @return true if the character is a whitespace otherwise false
 */
static  __jo_force_inline bool        jo_tools_is_whitespace(const char c)
{
    return c == '\n' || c == '\r' || c == '\t'  || c == ' ';
}

/** @brief Create an hash code based on the first four character of the string
 *  @param str String
 *  @return the hashcode (integer)
 */
static  __jo_force_inline int        jo_4_char_hash(const char * const str)
{
    return (int)str[0] | (((int)str[1]) << 8) | (((int)str[2]) << 16) | (((int)str[3]) << 24);
}

/** @brief DateTime struct
 */
typedef struct
{
    unsigned char   day;
    unsigned char   month;
    unsigned short  year;
    unsigned char   hour;
    unsigned char   week;
    unsigned char   minute;
    unsigned char   second;
}                   jo_datetime;

/** @brief Language type
 */
typedef enum
{
    English = 0,
    Deutsch = 1,
    French = 2,
    Espanol = 3,
    Italiano = 4,
    Japanese = 5
}                   jo_language;

/** @brief Horizontal move
 */
typedef enum
{
    None,
    Left,
    Right
}                   jo_horizontal_move;

/** @brief Get Sega saturn current language
 *  @return The language
 */
jo_language     jo_get_current_language(void);

/** @brief get current date and time
 *  @param now result
 */
void            jo_getdate(jo_datetime *now);

/** @brief atoi implementation
 *  @param str This is the string representation of an integral number
 *  @return This function returns the converted integral number as an int value. If no valid conversion could be performed, it returns zero
 */
int			    jo_tools_atoi(const char * restrict str);

/** @brief strlen implementation
 *  @param str This is the string
 *  @return This function returns the length of str
 */
int				jo_strlen(const char * restrict str);

/** @brief strcmp implementation
 *  @param p1 First string
 *  @param p2 Second string
 *  @return Compare p1 and p2, returning less than, equal to or greater than zero if p1 is lexicographically less than, equal to or greater than p2.
 */
int             jo_strcmp(const char * restrict p1, const char * restrict p2);

/** @brief Determine if two string equals
 *  @param p1 First string
 *  @param p2 Second string
 *  @return True if p1 and p2 is the same
 */
static  __jo_force_inline bool        jo_string_equals(const char * restrict p1, const char * restrict p2)
{
    return (jo_strcmp(p1, p2) == 0);
}

/** @brief determine if str end with a specific string
 *  @param str This is the string
 *  @param end The ending string
 *  @return This function returns true if str end with "end" param otherwise false
 */
bool            jo_endwith(const char * restrict str, const char * restrict end);

/** @brief Sets the first num bytes of the block of memory pointed by ptr to the specified value (interpreted as an unsigned char)
 *  @param ptr Pointer to the block of memory to fill.
 *  @param value Value to be set. The value is passed as an int, but the function fills the block of memory using the unsigned char conversion of this value.
 *  @param num Number of bytes to be set to the value.
 *  @remarks No return value in this implementation
 */
void            jo_memset(const void * const restrict ptr, const int value, unsigned int num);

/** @brief memcmp implementation
 *  @param p1 First block of memory
 *  @param p2 Second block of memory
 *  @param num Number of bytes to compare
 *  @return 0 if the num first bytes are the same, else the difference of the first different bytes (as unsigned char)
 */
int             jo_memcmp(const void * const p1, const void * const p2, unsigned int num);

/** @brief Size of the data compressed by Compiler/COMMON/jo_lz.py (the 4 bytes at its start, big endian)
 *  @param src Compressed data
 *  @return Size of the data once decompressed (bytes)
 */
static  __jo_force_inline unsigned int  jo_lz_size(const unsigned char * const src)
{
    return (((unsigned int)src[0] << 24) | ((unsigned int)src[1] << 16) | ((unsigned int)src[2] << 8) | src[3]);
}

/** @brief Decompress data compressed by Compiler/COMMON/jo_lz.py (LZSS: a byte of 8 flags, 1 a byte as it is, 0 a copy of 3 to 18
 *  bytes from 1 to 4096 bytes before: 12 bits of distance, 4 bits of length)
 *  @param src Compressed data (the size first, see jo_lz_size())
 *  @param dst Decompressed data (jo_lz_size() bytes)
 *  @return Size of the data once decompressed (bytes)
 *  @remarks About 4 times smaller in RAM than a model of a car with its textures: kept in RAM, read from the CD once
 *  (the CD audio goes on)
 */
unsigned int    jo_lz_decompress(const unsigned char * const src, unsigned char * const dst);

/** @brief Little pause possible
 */
static  __jo_force_inline void        jo_yield(void)
{
    asm("nop");
}

/** @brief Causes "a thread" to wait the number of times defined by the iterations parameter.
 *  @param iterations defines how long "a thread" is to wait
 */
static  __jo_force_inline void        jo_spin_wait(int iterations)
{
    while (iterations--) jo_yield();
}

/** @brief Copies the image map data to VRAM
 *  @param data Data source
 *  @param vram_addr VRAM address
 *  @param suui SUUI
 *  @param suuj SUUJ
 *  @param palnum Palette number
 *  @param mapoff MapOff
 */
void    jo_map_to_vram(unsigned short * restrict data, unsigned short * restrict vram_addr,
                        unsigned short suuj, unsigned short suui, unsigned short palnum, unsigned int mapoff);

/** @brief Copies the image cel data to VRAM
 *  @param data Data source
 *  @param vram_addr VRAM address
 *  @param size Data size
 */
static  __jo_force_inline void    jo_cell_to_vram(unsigned char * restrict data, unsigned char * restrict vram_addr, unsigned int size)
{
    while (size-- > 0) *(vram_addr++) = *(data++);
}

#endif /* !__JO_TOOLS_H__ */

/*
** END OF FILE
*/
