/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2020, Johannes Fetz (johannesfetz@gmail.com)
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
/** @file vdp1_command_pipeline.h
 *  @author Johannes Fetz
 *
 *  @brief VDP1 command table handling
 *  @bug No known bugs.
 */

#ifndef __JO_VDP1_COMMAND_PIPELINE_H__
# define __JO_VDP1_COMMAND_PIPELINE_H__

#if !JO_COMPILE_USING_SGL

/** @brief (internal engine usage)
 *  @warning MC Hammer: don't touch this
 */
extern unsigned int                                 __jo_vdp1_command_index;

/** @brief VDP1 commands */
typedef enum  __attribute__ ((__packed__))
{
    DrawNormalSprite = 0x0,
    DrawScaledSprite = 0x1,
    DrawDistortedSprite = 0x2,
    DrawPolygon = 0x4,
    DrawPolylines = 0x5,
    DrawLine = 0x6,
    SetUserClipping = 0x8,
    SetSystemClipping = 0x9,
    SetLocalCoordinates = 0xA,
    DrawEnd = 0x8000,
    __force_unsigned_short__ = 0xFFFFU
}
jo_vdp1_command_control;

/** @brief 32 byte VDP1 command table entry */
typedef struct
{
    jo_vdp1_command_control     ctrl;
    unsigned short              link;
    unsigned short              pmod;
    unsigned short              colr;
    unsigned short              srca;
    unsigned short              size;
    short                       xa;
    short                       ya;
    short                       xb;
    short                       yb;
    short                       xc;
    short                       yc;
    short                       xd;
    short                       yd;
    unsigned short              grda;
    short                       jo_engine_reserved;
}                               jo_vdp1_command;

static  __jo_force_inline jo_vdp1_command   *jo_vdp1_get_command_entry_from_link(unsigned short link)
{
    return (link ? (jo_vdp1_command *)(JO_VDP1_VRAM + (unsigned int)link * 0x8) : JO_NULL);
}
static  __jo_force_inline unsigned short   jo_vdp1_command_index_to_link(int command_index)
{
    return ((JO_VDP1_VRAM + JO_MULT_BY_32(command_index)) / 0x8);
}
static  __jo_force_inline jo_vdp1_command   *jo_vdp1_get_command_entry(int command_index)
{
    return (jo_vdp1_command *)(JO_VDP1_VRAM + JO_MULT_BY_32(command_index));
}
static  __jo_force_inline unsigned short   jo_vdp1_command_to_link(jo_vdp1_command *command)
{
    return (command ? (((unsigned int)command - (unsigned int)JO_VDP1_VRAM) / 0x8) : 0);
}

jo_vdp1_command*                jo_vdp1_create_command(void);

#endif

#endif /* !__JO_VDP1_COMMAND_PIPELINE_H__ */

/*
** END OF FILE
*/
