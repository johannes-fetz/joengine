/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2024, Johannes Fetz (johannesfetz@gmail.com)
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
/** @file time.h
 *  @author Johannes Fetz
 *
 *  @brief Time Management
 *  @bug No known bugs.
 */

#ifndef __JO_TIME_H__
# define __JO_TIME_H__

# define JO_TIME_CKS_32_MODE        (1)
# define JO_TIME_M_CKS              (3)

typedef enum
{
    /** @brief High Free Running Counter Register (FCR), counts up to 255, then iterates FCR Low */
    RegisterHighFRC = 0xfffffe12,
    /** @brief Low Free Running Counter Register (FCR), increases every time FCR high reaches 256 */
    RegisterLowFRC = 0xfffffe13,
    /** @brief Time Control Register (TCR) */
    RegisterTCR = 0xfffffe16,
    /** @brief System clock register */
    RegisterSysClock = 0x6000324
}               jo_time_memory_address;

static  __jo_force_inline void jo_time_poke_byte(jo_time_memory_address addr, unsigned char data)
{
    (*(volatile unsigned char *)(addr)) = data;
}

static  __jo_force_inline unsigned char jo_time_peek_byte(jo_time_memory_address addr)
{
    return (*(volatile unsigned char *)(addr));
}

static  __jo_force_inline void jo_time_poke_int(jo_time_memory_address addr, unsigned int data)
{
    (*(volatile unsigned int *)(addr)) = data;
}

static  __jo_force_inline unsigned int jo_time_peek_int(jo_time_memory_address addr)
{
    return (*(volatile unsigned int *)(addr));
}

/** @brief Get Free Running Counter value */
static  __jo_force_inline int jo_time_get_frc(void)
{
    return (jo_time_peek_byte(RegisterHighFRC) << 8 | jo_time_peek_byte(RegisterLowFRC));
}

static  __jo_force_inline void jo_time_set_frc(unsigned char reg)
{
    jo_time_poke_byte(RegisterHighFRC, reg >> 8);
    jo_time_poke_byte(RegisterLowFRC, reg);
}

/** @brief get ticks count
 *  @return ticks count from jo_core_run()
 */
unsigned int    jo_get_ticks(void);

/*
████████╗██╗███╗   ███╗███████╗    ████████╗██████╗  █████╗  ██████╗██╗  ██╗██╗███╗   ██╗ ██████╗
╚══██╔══╝██║████╗ ████║██╔════╝    ╚══██╔══╝██╔══██╗██╔══██╗██╔════╝██║ ██╔╝██║████╗  ██║██╔════╝
   ██║   ██║██╔████╔██║█████╗         ██║   ██████╔╝███████║██║     █████╔╝ ██║██╔██╗ ██║██║  ███╗
   ██║   ██║██║╚██╔╝██║██╔══╝         ██║   ██╔══██╗██╔══██║██║     ██╔═██╗ ██║██║╚██╗██║██║   ██║
   ██║   ██║██║ ╚═╝ ██║███████╗       ██║   ██║  ██║██║  ██║╚██████╗██║  ██╗██║██║ ╚████║╚██████╔╝
   ╚═╝   ╚═╝╚═╝     ╚═╝╚══════╝       ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝╚═╝╚═╝  ╚═══╝ ╚═════╝

██╗███╗   ██╗    ███████╗██╗██╗  ██╗███████╗██████╗     ███████╗███████╗ ██████╗ ██████╗ ███╗   ██╗██████╗ ███████╗
██║████╗  ██║    ██╔════╝██║╚██╗██╔╝██╔════╝██╔══██╗    ██╔════╝██╔════╝██╔════╝██╔═══██╗████╗  ██║██╔══██╗██╔════╝
██║██╔██╗ ██║    █████╗  ██║ ╚███╔╝ █████╗  ██║  ██║    ███████╗█████╗  ██║     ██║   ██║██╔██╗ ██║██║  ██║███████╗
██║██║╚██╗██║    ██╔══╝  ██║ ██╔██╗ ██╔══╝  ██║  ██║    ╚════██║██╔══╝  ██║     ██║   ██║██║╚██╗██║██║  ██║╚════██║
██║██║ ╚████║    ██║     ██║██╔╝ ██╗███████╗██████╔╝    ███████║███████╗╚██████╗╚██████╔╝██║ ╚████║██████╔╝███████║
╚═╝╚═╝  ╚═══╝    ╚═╝     ╚═╝╚═╝  ╚═╝╚══════╝╚═════╝     ╚══════╝╚══════╝ ╚═════╝ ╚═════╝ ╚═╝  ╚═══╝╚═════╝ ╚══════╝

Contributed by Ponut64: https://github.com/johannes-fetz/joengine/pull/21 & Emerald Nova
*/

/** @brief Will increment fixed-point 1 every second since starting jo_fixed_point_time() */
extern jo_fixed     time_in_seconds;

/** @brief Represent the delta time (in fixed-point seconds) between each frame */
extern jo_fixed     delta_time;

/** @brief get ticks count
 *	@warning run only once per frame!
 */
void			    jo_fixed_point_time(void);

#endif /* !__JO_TIME_H__ */

/*
** END OF FILE
*/
