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
#include "jo/conf.h"
#include "jo/types.h"
#include "jo/dsp.h"

void                                jo_dsp_load_program(const unsigned int * const program, const unsigned int count)
{
    register unsigned int           i;

    // Stop, then program counter = 0: each write to the program port increments it
    JO_DSP_PPAF = 0;
    JO_DSP_PPAF = JO_DSP_PPAF_LOAD;
    for (i = 0; i < count && i < JO_DSP_PROGRAM_SIZE; ++i)
        JO_DSP_PPD = program[i];
}

void                                jo_dsp_write_data(const unsigned int bank, const unsigned int address, const unsigned int * const data, const unsigned int count)
{
    register unsigned int           i;

    // The address port increments after each access of the data port
    JO_DSP_PDA = ((bank & 3) << 6) | (address & 0x3F);
    for (i = 0; i < count; ++i)
        JO_DSP_PDD = data[i];
}

void                                jo_dsp_read_data(const unsigned int bank, const unsigned int address, unsigned int * const data, const unsigned int count)
{
    register unsigned int           i;

    JO_DSP_PDA = ((bank & 3) << 6) | (address & 0x3F);
    for (i = 0; i < count; ++i)
        data[i] = JO_DSP_PDD;
}

/*
** END OF FILE
*/
