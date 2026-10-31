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
/** @file backup.h
 *  @author Slinga with help from KnightOfDragon, Reye, Purist, and Antime
 *
 *  @brief Jo Engine serial link (Taisen cable) featurs function prototypes
 *  @bug Only tested on US and Jap Saturns. Not tested on PAL. No known bugs.
 */

#ifndef __JO_SERIAL_H__
# define __JO_SERIAL_H__

# ifdef JO_COMPILE_WITH_SERIAL_SUPPORT

// serial registers
#define SERIAL_REG_SMR (*(volatile unsigned char*)(0xFFFFFE00)) // serial mode register
#define SERIAL_REG_BRR (*(volatile unsigned char*)(0xFFFFFE01)) // bit rate register
#define SERIAL_REG_SCR (*(volatile unsigned char*)(0xFFFFFE02)) // serial control register
#define SERIAL_REG_TDR (*(volatile unsigned char*)(0xFFFFFE03)) // transmit data register
#define SERIAL_REG_SSR (*(volatile unsigned char*)(0xFFFFFE04)) // serial status register
#define SERIAL_REG_RDR (*(volatile unsigned char*)(0xFFFFFE05)) // receive data register

// serial register bit fields
#define SERIAL_BIT_TDRE     (1<<7)
#define SERIAL_BIT_RDRF     (1<<6)

#define SERIAL_BIT_TE       (1<<5)
#define SERIAL_BIT_RE       (1<<4)

#define ORER                (1<<5)
#define FER                 (1<<4)
#define PER                 (1<<3)

// prototype for serial error callbacks
typedef void (*jo_serial_error_callback)(void);

void jo_serial_async_init(void);
void jo_serial_register_error_callbacks(jo_serial_error_callback orer, jo_serial_error_callback fer, jo_serial_error_callback per);
int jo_serial_send_byte(unsigned char data);
int jo_serial_recv_byte(unsigned char* data);

#endif /* !JO_COMPILE_WITH_SERIAL_SUPPORT */

#endif /* !__JO_SERIAL_H__ */

/*
** END OF FILE
*/