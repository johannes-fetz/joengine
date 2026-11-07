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
/** @file internal/core.h
 *  @author Johannes Fetz
 *
 *  @brief Core: internal functions of the engine (see jo/core.h)
 *  @warning Not part of the API: only for the source files of the engine (included after jo/core.h). The games
 *  include jo/jo.h, which doesn't include this file
 *  @bug No known bugs.
 */

#ifndef __JO_INTERNAL_CORE_H__
# define __JO_INTERNAL_CORE_H__

#ifdef JO_DEBUG

void    __jo_core_error(char *message, const char *function);

#endif /* !JO_DEBUG */

/** @brief A pause in a busy wait loop (Internal usage): a division of the CPU (39 cycles), waited for
 *  @remarks Used where a CPU has nothing to do (the slave CPU waiting for a job, the master CPU for the VBLANK): an
 *           emulator runs 3 instructions for the 39 cycles instead of about 15 of a loop on the flag (Mednafen emulates
 *           the wait of the division unit: a game emulated 186 -> 193 % of the real time), and no bus access.
 *           Not in the waits of the master CPU for the VDP1 (__jo_vdp1_end_frame()): a view of a level went from 30 to
 *           25 frames per second (measured, not explained)
 *  @warning The division unit is changed: never between the start of a division and the read of its quotient (the
 *           interrupts save and restore it)
 */
static  __jo_force_inline void      __jo_core_pause(void)
{
    JO_SH2_DVSR = 1;
    // (writing the dividend starts the division, reading the quotient waits for its end)
    JO_SH2_DVDNT = 0;
    (void)JO_SH2_DVDNT;
}

#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT

/** @brief Check if the slave CPU started (Internal usage)
 *  @return false if the slave CPU didn't start (the jobs are executed by the master CPU)
 */
bool    __jo_core_is_slave_running(void);

/** @brief Add a job to the queue of the slave CPU (Internal usage)
 *  @param function Function executed by the slave CPU
 *  @param argument Argument of the function
 *  @remarks The jobs are executed in order. The master CPU waits only if the queue is full.
 *           If the slave CPU didn't start, the function is executed now by the master CPU
 *  @remarks The slave CPU purges its cache before each job: the job reads the data written by the master CPU before this call
 */
void    __jo_core_add_slave_job(const jo_slave_job_function function, void *argument);

#endif /* !JO_COMPILE_WITH_DUAL_CPU_SUPPORT */

/** @brief Wait for the end of the current frame (Internal usage)
 *  @remarks Send the VDP1 commands and wait for JO_FRAMERATE VBLANK
 */
void                                __jo_core_synch(void);

/** @brief Set an interrupt handler in the vector table (Internal usage)
 *  @param vector Interrupt vector number (JO_VECTOR_VBLANK_IN, etc)
 *  @param handler Function declared with __attribute__((interrupt_handler))
 */
void                                __jo_core_set_interrupt_handler(const unsigned int vector, jo_interrupt_handler handler);

/** @brief Unmask SCU interrupts (Internal usage)
 *  @param mask Interrupts (JO_SCU_INT_VBLANK_IN, etc)
 */
void                                __jo_core_enable_scu_interrupts(const unsigned int mask);

/** @brief Write the SCU interrupt mask again at the end of a SCU interrupt handler (Internal usage)
 *  @remarks When the CPU takes a SCU interrupt, the SCU masks all its interrupts (JO_SCU_IMS = JO_SCU_INT_ALL):
 *           without this call, no other SCU interrupt happens (the BIOS handlers do the same)
 */
void                                __jo_core_restore_scu_mask(void);

/** @brief Get CPU status register (Internal usage)
 *  @return Status register value
 */
static  __jo_force_inline unsigned int  __jo_core_get_sr(void)
{
    unsigned int                    sr;

    __asm__ volatile ("stc sr, %0" : "=r" (sr));
    return (sr);
}

/** @brief Set CPU status register (Internal usage)
 *  @param sr Status register value (JO_SH2_SR_INTERRUPT_MASK bits: interrupt mask level)
 */
static  __jo_force_inline void      __jo_core_set_sr(const unsigned int sr)
{
    __asm__ volatile ("ldc %0, sr" : : "r" (sr) : "memory");
}

/** @brief Mask all interrupts (Internal usage)
 *  @return Previous status register for __jo_core_restore_interrupts()
 */
static  __jo_force_inline unsigned int  __jo_core_disable_interrupts(void)
{
    unsigned int                    sr;

    sr = __jo_core_get_sr();
    __jo_core_set_sr(sr | JO_SH2_SR_INTERRUPT_MASK);
    return (sr);
}

/** @brief Restore interrupts masked by __jo_core_disable_interrupts() (Internal usage)
 *  @param sr Value returned by __jo_core_disable_interrupts()
 */
static  __jo_force_inline void      __jo_core_restore_interrupts(const unsigned int sr)
{
    __jo_core_set_sr(sr);
}

#endif /* !__JO_INTERNAL_CORE_H__ */

/*
** END OF FILE
*/
