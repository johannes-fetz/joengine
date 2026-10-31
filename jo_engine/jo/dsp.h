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
/** @file dsp.h
 *  @author Johannes Fetz
 *
 *  @brief SCU DSP: the third processor of the Saturn (about 14 MHz, one 32 x 32 bits multiply and accumulate per cycle)
 *  @remarks The DSP has 256 instructions of program RAM and 4 banks of 64 words of data RAM. It reads and writes the high
 *           work RAM and the B-bus by DMA (not the low work RAM). It has no division
 *  @remarks Programs are written in DSP assembler and converted to a C array by Compiler/COMMON/scudsp_asm.py
 *  @remarks The 3D module can use the DSP to transform the vertices (see jo_3d_enable_dsp())
 *  @warning The data RAM and the program RAM can only be accessed by the CPU when the DSP is stopped
 */

#ifndef __JO_DSP_H__
# define __JO_DSP_H__

/** @brief Words of a data RAM bank */
# define JO_DSP_BANK_SIZE           (64)
/** @brief Words of the program RAM */
# define JO_DSP_PROGRAM_SIZE        (256)

/** @brief Program control port (PPAF) */
# define JO_DSP_PPAF                (*(volatile unsigned int *)0x25FE0080)
/** @brief Program RAM data port */
# define JO_DSP_PPD                 (*(volatile unsigned int *)0x25FE0084)
/** @brief Data RAM address port */
# define JO_DSP_PDA                 (*(volatile unsigned int *)0x25FE0088)
/** @brief Data RAM data port */
# define JO_DSP_PDD                 (*(volatile unsigned int *)0x25FE008C)
/** @brief PPAF: execute (write: start, read: running) */
# define JO_DSP_PPAF_EXECUTE        (1 << 16)
/** @brief PPAF: program counter load (the low 8 bits) */
# define JO_DSP_PPAF_LOAD           (1 << 15)
/** @brief PPAF: a DMA of the DSP is in progress */
# define JO_DSP_PPAF_DMA            (1 << 23)

/** @brief Stop the DSP and load a program
 *  @param program Instructions
 *  @param count Instruction count (256 at most)
 */
void                                jo_dsp_load_program(const unsigned int * const program, const unsigned int count);

/** @brief Write the data RAM (the DSP must be stopped)
 *  @param bank Bank (0 to 3)
 *  @param address First word (0 to 63)
 *  @param data Words
 *  @param count Word count
 */
void                                jo_dsp_write_data(const unsigned int bank, const unsigned int address, const unsigned int * const data, const unsigned int count);

/** @brief Read the data RAM (the DSP must be stopped)
 *  @param bank Bank (0 to 3)
 *  @param address First word (0 to 63)
 *  @param data Words (output)
 *  @param count Word count
 */
void                                jo_dsp_read_data(const unsigned int bank, const unsigned int address, unsigned int * const data, const unsigned int count);

/** @brief Start the program loaded by jo_dsp_load_program()
 *  @param pc First instruction
 *  @remarks The DSP runs until its END instruction, the CPU keeps working (see jo_dsp_is_running())
 */
static  __jo_force_inline void      jo_dsp_start(const unsigned int pc)
{
    JO_DSP_PPAF = JO_DSP_PPAF_EXECUTE | JO_DSP_PPAF_LOAD | (pc & 0xFF);
}

/** @brief Check if the DSP is running
 *  @return true until the END instruction (and the end of the last DMA of the DSP)
 */
static  __jo_force_inline bool      jo_dsp_is_running(void)
{
    return ((JO_DSP_PPAF & (JO_DSP_PPAF_EXECUTE | JO_DSP_PPAF_DMA)) != 0);
}

/** @brief Wait for the end of the program started by jo_dsp_start() */
static  __jo_force_inline void      jo_dsp_wait(void)
{
    while (jo_dsp_is_running())
        ;
}

#endif /* !__JO_DSP_H__ */

/*
** END OF FILE
*/
