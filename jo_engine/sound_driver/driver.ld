/* The sound driver of the 68000: from the address 0 of the sound RAM, below the mailbox (protocol.h) */
OUTPUT_FORMAT("elf32-m68k")
ENTRY(_start)
SECTIONS
{
    . = 0;
    .vectors : { KEEP(*(.vectors)) }
    .text : { *(.text) *(.text.*) }
    .rodata : { *(.rodata) *(.rodata.*) }
    .data : { *(.data) *(.data.*) }
    .bss : { *(.bss) *(.bss.*) *(COMMON) }
    __driver_end = .;
    /DISCARD/ : { *(.comment) *(.eh_frame) *(.note*) }
}
/* (128 bytes of stack below the mailbox at 0x0B00) */
ASSERT(__driver_end <= 0x0A80, "The sound driver is too big: it overlaps its stack (0x0A80 - 0x0B00)")
