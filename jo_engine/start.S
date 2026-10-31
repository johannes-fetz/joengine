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
** Program entry point (the IP.BIN jumps here: first read address 0x06004000)
*/

/* Status register value: all interrupts masked */
#define JO_SR_INTERRUPT_MASK        0xF0
/* Cache control register: purge, then enable (4-way, instruction + data) */
#define JO_CCR_CACHE_PURGE          0x10
#define JO_CCR_CACHE_ENABLE         0x01
#define JO_SH2_CCR_ADDRESS          0xFFFFFE92
/* Master CPU stack (top), see JO_WORK_RAM_STACK_TOP */
#define JO_MASTER_STACK_TOP         0x060FFC00

        .section JO_START, "ax"
        .align  2
        .global _jo_start
_jo_start:
        /* Mask all interrupts while we set things up */
        mov     #JO_SR_INTERRUPT_MASK, r0
        extu.b  r0, r0
        ldc     r0, sr

        /* Master CPU stack */
        mov.l   .stack_top, r15

        /* Purge and enable the cache (4-way, instruction + data) */
        mov.l   .ccr, r1
        mov     #JO_CCR_CACHE_PURGE, r0
        mov.b   r0, @r1
        mov     #JO_CCR_CACHE_ENABLE, r0
        mov.b   r0, @r1

        /* Clear .bss */
        mov.l   .bss_start, r1
        mov.l   .bss_end, r2
        mov     #0, r0
1:
        cmp/hs  r2, r1
        bt      2f
        mov.l   r0, @r1
        bra     1b
        add     #4, r1
2:
        /* Clear the engine work area */
        mov.l   .work_start, r1
        mov.l   .work_end, r2
3:
        cmp/hs  r2, r1
        bt      4f
        mov.l   r0, @r1
        bra     3b
        add     #4, r1
4:
        /* Call main() */
        mov.l   .main, r0
        jsr     @r0
        nop
5:
        bra     5b
        nop

        .align  2
.stack_top:     .long   JO_MASTER_STACK_TOP
.ccr:           .long   JO_SH2_CCR_ADDRESS
.bss_start:     .long   __bstart
.bss_end:       .long   __bend
.work_start:    .long   __jo_work_start
.work_end:      .long   __jo_work_end
.main:          .long   _main

/*
** END OF FILE
*/
