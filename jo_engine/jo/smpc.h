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
/** @file smpc.h
 *  @author Johannes Fetz
 *
 *  @brief System Management & Peripheral Control
 *  @remarks The SMPC (System Management & Peripheral Control) is a Hitachi 4-bit MCU with built-in program ROM.
 *  @remarks The actual part number is HD404920FS, but the chip is branded with a Sega custom part number of 315-5744.
 *  @bug No known bugs.
 */

#ifndef __JO_SMPC_H__
# define __JO_SMPC_H__

/** @brief SMPC Commands */
typedef enum
{
    /** @brief Resets and enables the SH-2 Master CPU. */
    EnableMasterCPU = 0x0,
    /** @brief Resets and enables the SH-2 Slave CPU. */
    EnableSlaveCPU = 0x2,
    /** @brief Disables the SH-2 Slave CPU. */
    DisableSlaveCPU = 0x3,
    /** @brief Resets and enables the Motorola C68K (sound) CPU. */
    EnableSoundCPU = 0x6,
    /** @brief Disables the Motorola C68K (sound) CPU. */
    DisableSoundCPU = 0x7,
    /** @brief Resets and enables the CD Block. */
    EnableCD = 0x8,
    /** @brief Disables the CD Block. */
    DisableCD = 0x9,
    /** @brief Resets and enables Netlink execution. */
    EnableNetlink = 0xA,
    /** @brief Disables Netlink execution. */
    DisableNetlink = 0xB,
    /** @brief 	Resets the System. */
    SystemReset = 0xD,
    /** @brief Changes the system clockspeed */
    ChangeSystemClockSpeed352 = 0xE,
    /** @brief Changes the system clockspeed */
    ChangeSystemClockSpeed320 = 0xF,
    /** @brief Fetches the SMPC status and peripheral data. */
    FetchStatusAndPeripheralData = 0x10,
    /** @brief Sets the date and time for the RTC */
    SetRTCDateAndTime = 0x16,
    /** @brief Sets the 4-byte battery-backed memory contained on the SMPC(which is used by the bios for language settings, etc. */
    SetSMPCMemory = 0x17,
    /** @brief Sends an NMI request to the Master SH2 */
    SendNMIRequestToMasterCPU = 0x18,
    /** @brief Enables NMI requests to be sent when the Reset button is pressed. */
    EnableReset = 0x19,
    /** @brief Disables NMI requests to be sent when the Reset button is pressed. */
    DisableReset = 0x1A
}               jo_smpc_command;

typedef enum
{
    /** @brief The command that's supposed to be issued by the SMPC */
    CommandRegister = 0x2010001f,
    /** @brief Status Register */
    StatusRegister = 0x20100061,
    /** @brief Status Flag. Shows the status of the SMPC command. Normally you set this to 1 when issuing out a command, and then the SMPC clears it when it's finished. */
    StatusFlag = 0x20100063,
    /** @brief Input registers for command issuing. Whatever data the SMPC needs for processing command goes here. */
    InputRegister0 = 0x20100001,
    InputRegister1 = 0x20100003,
    InputRegister2 = 0x20100005,
    InputRegister3 = 0x20100007,
    InputRegister4 = 0x20100009,
    InputRegister5 = 0x2010000b,
    InputRegister6 = 0x2010000d,
    /** @brief Ouput Register for the command. If it supports it, it'll output any return data here */
    OutputRegister0 = 0x20100021,
    OutputRegister1 = 0x20100023,
    OutputRegister2 = 0x20100025,
    OutputRegister3 = 0x20100027,
    OutputRegister4 = 0x20100029,
    OutputRegister5 = 0x2010002b,
    OutputRegister6 = 0x2010002d,
    OutputRegister7 = 0x2010002f,
    OutputRegister8 = 0x20100031,
    OutputRegister9 = 0x20100033,
    OutputRegister10 = 0x20100035,
    OutputRegister11 = 0x20100037,
    OutputRegister12 = 0x20100039,
    OutputRegister13 = 0x2010003b,
    OutputRegister14 = 0x2010003d,
    OutputRegister15 = 0x2010003f,
    OutputRegister16 = 0x20100041,
    OutputRegister17 = 0x20100043,
    OutputRegister18 = 0x20100045,
    OutputRegister19 = 0x20100047,
    OutputRegister20 = 0x20100049,
    OutputRegister21 = 0x2010004b,
    OutputRegister22 = 0x2010004d,
    OutputRegister23 = 0x2010004f,
    OutputRegister24 = 0x20100051,
    OutputRegister25 = 0x20100053,
    OutputRegister26 = 0x20100055,
    OutputRegister27 = 0x20100057,
    OutputRegister28 = 0x20100059,
    OutputRegister29 = 0x2010005b,
    OutputRegister30 = 0x2010005d,
    OutputRegister31 = 0x2010005f,
    PortDataRegister1 = 0x20100075,
    PortDataRegister2 = 0x20100077,
    DataDirectionRegister1 = 0x20100079,
    DataDirectionRegister2 = 0x2010007b,
    InputOutputSelectRegister = 0x2010007d,
    ExternalLatchEnableRegister = 0x2010007f
}               jo_smpc_memory_address;

static  __jo_force_inline void jo_smpc_write_byte(jo_smpc_memory_address addr, unsigned char data)
{
    (*(volatile unsigned char *)(addr)) = data;
}
static  __jo_force_inline unsigned char jo_smpc_read_byte(jo_smpc_memory_address addr)
{
    return (*(volatile unsigned char *)(addr));
}
static  __jo_force_inline void jo_smpc_wait(void)
{
    while ((jo_smpc_read_byte(StatusFlag) & 0x1) == 0x1);
}
static  __jo_force_inline void jo_smpc_begin_command(void)
{
    jo_smpc_wait();
    jo_smpc_write_byte(StatusFlag, 1);
}
static  __jo_force_inline void jo_smpc_end_command(jo_smpc_command command)
{
    jo_smpc_write_byte(CommandRegister, command);
    jo_smpc_wait();
}

#endif /* !__JO_SMPC_H__ */

/*
** END OF FILE
*/
