/*
** The vectors of the 68000 and the start of the sound driver (see driver.c)
*/
    .section .vectors, "a"
    .long   0x00000B00              /* Initial stack pointer (JO_DRIVER_CODE_END) */
    .long   _start                  /* Initial program counter */
    .rept   62
    .long   _trap                   /* The other exceptions: stop there */
    .endr

    .text
    .globl  _start
_start:
    move.w  #0x2700, %sr            /* No interrupt */
    jmp     driver_main
_trap:
    bra.s   _trap
