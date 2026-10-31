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

#include <jo/serial.h>
#include <jo/jo.h>

# ifdef JO_COMPILE_WITH_SERIAL_SUPPORT

static int g_serial_initialized = 0;

// error callback functions
static jo_serial_error_callback g_orer = JO_NULL;
static jo_serial_error_callback g_fer = JO_NULL;
static jo_serial_error_callback g_per = JO_NULL;

void jo_serial_async_init(void)
{
    if(g_serial_initialized == 1)
    {
        // already initialized
        return;
    }
    
    SERIAL_REG_SCR = 0;
    SERIAL_REG_SMR = 0;
    SERIAL_REG_BRR =  0; 
 
    // delay for at least one bit
    for(unsigned int i = 0; i < 64; i++)
    {
        asm("nop");
    }
    
    SERIAL_REG_SCR |= (SERIAL_BIT_TE|SERIAL_BIT_RE);

    g_serial_initialized = 1;

    return;
}

void jo_serial_register_error_callbacks(jo_serial_error_callback orer, jo_serial_error_callback fer, jo_serial_error_callback per)
{
    if(orer)
    {
        g_orer = orer;
    }

    if(fer)
    {
        g_fer = fer;
    }

    if(per)
    {
        g_per = per;
    }

    return;
}

int jo_serial_send_byte(unsigned char data)
{
    if(!g_serial_initialized)
    {
        return -1;
    }

    if(!(SERIAL_REG_SSR & SERIAL_BIT_TDRE))
    {
        // not ready to send
        return -2;
    }
   
    SERIAL_REG_TDR = data;
    SERIAL_REG_SSR &= ~SERIAL_BIT_TDRE;

    return 0;
}

int jo_serial_recv_byte(unsigned char* data)
{
    if(!g_serial_initialized)
    {
        return -1;
    }

    //
    // check for errors
    //
    
    if(SERIAL_REG_SSR & ORER)
    {
        SERIAL_REG_SSR &= ~ORER;

        if(g_orer)
        {
            g_orer();
        }
    }

    if(SERIAL_REG_SSR & FER)
    {
        SERIAL_REG_SSR &= ~FER;

        if(g_fer)
        {
            g_fer();
        }
    }

    if(SERIAL_REG_SSR & PER)
    {
        SERIAL_REG_SSR &= ~PER;

        if(g_per)
        {
            g_per();
        }
    }

    if(!(SERIAL_REG_SSR & SERIAL_BIT_RDRF))
    {
        // not ready to send
        return -2;
    }   

    *data = (SERIAL_REG_RDR);

    SERIAL_REG_SSR &= ~SERIAL_BIT_RDRF;
    
    return 0;
}

#endif /* !JO_COMPILE_WITH_SERIAL_SUPPORT */

/*
** END OF FILE
*/
