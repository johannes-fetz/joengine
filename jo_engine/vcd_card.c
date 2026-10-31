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

#include <jo/vcd_card.h>
#include <jo/jo.h>

#ifdef JO_COMPILE_WITH_VCD_CARD_SUPPORT

#define BIOS_IS_VCD_CARD_PRESENT (0x06000274) // contains a pointer to bios_is_vcd_card_present routine
#define BIOS_GET_VCD_CARD_ROM (0x06000298) // contains a pointer to bios_is_vcd_card_present routine
#define INDEX_SIZE_BYTES (0x800) // each index is 0x800 bytes.

typedef int (*bios_is_vcd_card_present_fn)(int index);
typedef int (*bios_get_vcd_card_rom_fn)(unsigned int index, unsigned int size, void* dest);

int jo_vcd_card_is_present(void)
{
    bios_is_vcd_card_present_fn bios_is_vcd_card_present = JO_NULL;

    // call into the BIOS to detect if the VCD card is present
    bios_is_vcd_card_present = (bios_is_vcd_card_present_fn)(*(unsigned int*)BIOS_IS_VCD_CARD_PRESENT);
    if(!bios_is_vcd_card_present)
    {
        // Failed to resolve BIOS function
        return -1;
    }

    return bios_is_vcd_card_present(0);
}

int jo_vcd_card_get_vcd_card_rom(unsigned int index, unsigned int num_indexes, void* dest, unsigned int dest_size)
{
    bios_get_vcd_card_rom_fn bios_get_vcd_card_rom = JO_NULL;

    if(!dest || !dest_size)
    {
        // invalid buffer
        return -1;
    }

    if(!num_indexes || num_indexes * INDEX_SIZE_BYTES < num_indexes)
    {
        // bad number of num_indexes
        return -2;
    }

    if(dest_size < num_indexes * INDEX_SIZE_BYTES)
    {
        // dest buffer is too small
        return -3;
    }

    bios_get_vcd_card_rom = (bios_get_vcd_card_rom_fn)(*(unsigned int*)BIOS_GET_VCD_CARD_ROM);
    if(!bios_get_vcd_card_rom)
    {
        // failed to resolve BIOS function
        return -4;
    }

    return bios_get_vcd_card_rom(index, num_indexes, dest);
}

#endif /* !JO_COMPILE_WITH_VCD_CARD_SUPPORT */

/*
** END OF FILE
*/
