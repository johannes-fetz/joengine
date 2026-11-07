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
/** @file internal/tools.h
 *  @author Johannes Fetz
 *
 *  @brief Tools: internal functions of the engine (see jo/tools.h)
 *  @warning Not part of the API: only for the source files of the engine (included after jo/tools.h). The games
 *  include jo/jo.h, which doesn't include this file
 *  @bug No known bugs.
 */

#ifndef __JO_INTERNAL_TOOLS_H__
# define __JO_INTERNAL_TOOLS_H__

/** @brief Read a 32 bits I/O port into memory with the DMA controller of the current CPU (Internal usage)
  * @param port Port address (the address doesn't change)
  * @param dest Destination (4 bytes aligned)
  * @param size Size in bytes (multiple of 4)
  */
void            __jo_dma_read_port32(const volatile void * const port, void * const dest, const unsigned int size);

#endif /* !__JO_INTERNAL_TOOLS_H__ */

/*
** END OF FILE
*/
