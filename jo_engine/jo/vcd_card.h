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
/** @file vcd_card.h
 *  @author Slinga, based on CyberWarriorX's work
 *
 *  @brief WIP VCD Card support
 *  @bug Only supports detecting card, no support for playing videos
 */

#ifndef __JO_VCD_CARD_H__
#define __JO_VCD_CARD_H__

#ifdef JO_COMPILE_WITH_VCD_CARD_SUPPORT

 /** @brief Checks whether or not the VCD Card is inserted in the system
  *  @return 0 if the VCD Card is present
  */
int jo_vcd_card_is_present(void);

 /** @brief Advanced feature to read the firmware of the VCD Card
  *  @param index Which index to start with
  *  @param num_indexes How many indexes to copy. Each index is 800 bytes
  *  @param dest On success, the firmware gets written here
  *  @param dest_size Size in bytes of dest
  *  @return On success, the number of bytes written. On failure < 0 is returned.
  */
int jo_vcd_card_get_vcd_card_rom(unsigned int index, unsigned int num_indexes, void* dest, unsigned int dest_size);

#endif /* !JO_COMPILE_WITH_VCD_CARD_SUPPORT */

#endif /* !__JO_VCD_CARDL_H__ */

/*
** END OF FILE
*/
