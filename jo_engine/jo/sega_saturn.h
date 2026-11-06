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
/** @file sega_saturn.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Sega Saturn constants (addresses, memory sizes, etc)
 *  @bug No known bugs.
 */

#ifndef __JO_SEGA_SATURN_H__
# define __JO_SEGA_SATURN_H__

/*

-------------------------------------------------|-----------------------------|------------------------------
|  Function        |  Normal Scroll Screen                                     |  Rotation Scroll Screen     |
|                  |-----------------------------|-----------------------------|------------------------------
|                  | NBG0         | NBG1         | NBG2         | NBG3         | RBG0         | RBG1         |
-------------------------------------------------|-----------------------------|------------------------------
| Character Colour | 16 colours   | 16 colours   | 16 colours   | 16 colours   | 16 colours   | 16 colours   |
| Count            | 256 " "      | 256 " "      | 256 " "      | 256 " "      | 256 " "      | 256 " "      |
|                  | 2048 " "     | 2048 " "     |              |              | 2048 " "     | 2048 " "     |
|                  | 32768 " "    | 32768 " "    |              |              | 32768 " "    | 32768 " "    |
|                  | 16770000 " " |              |              |              | 16770000 " " | 16770000 " " |
-------------------------------------------------|-----------------------------|------------------------------
| Character Size   | 1x1 Cells , 2x2 Cells                                                                   |
-------------------------------------------------|-----------------------------|------------------------------
| Pattern Name     | 1 word , 2 words                                                                        |
| Data Size        |                                                                                         |
-------------------------------------------------|-----------------------------|------------------------------
| Plane Size       | 1 H x 1 V 1 Pages ; 2 H x 1 V 1 Pages ; 2 H x 2 V Pages (I don't understand ... )       |
-------------------------------------------------|-----------------------------|------------------------------
| Plane Count      | 4                                                         | 16                          |
-------------------------------------------------|-----------------------------|------------------------------
| Bitmap Possible  | Yes                         | No                          | Yes          | No           |
-------------------------------------------------|-----------------------------|------------------------------
| Bitmap Size      | 512 x 256                   | N/A                         | 512x256      | N/A          |
|                  | 512 x 512                   |                             | 512x512      |              |
|                  | 1024 x 256                  |                             |              |              |
|                  | 1024 x 512                  |                             |              |              |
-------------------------------------------------|-----------------------------|------------------------------
| Scale            | 0.25 x - 256 x              | None                        | Any ?                       |
-------------------------------------------------|-----------------------------|------------------------------
| Rotation         | No                                                        | Yes                         |
-------------------------------------------------|-----------------------------|-----------------------------|
| Linescroll       | Yes                         | No                                                        |
-------------------------------------------------|-----------------------------|------------------------------
| Column Scroll    | Yes                         | No                                                        |
-------------------------------------------------|-----------------------------|------------------------------
| Mosaic           | Yes                                                       | Horizontal Only             |
-------------------------------------------------|-----------------------------|------------------------------
*/

/*
██╗   ██╗██████╗ ██████╗  ██╗
██║   ██║██╔══██╗██╔══██╗███║
██║   ██║██║  ██║██████╔╝╚██║
╚██╗ ██╔╝██║  ██║██╔═══╝  ██║
 ╚████╔╝ ██████╔╝██║      ██║
  ╚═══╝  ╚═════╝ ╚═╝      ╚═╝

*/

/** @brief VDP1 VRAM (512 kilobytes) */
# define JO_VDP1_VRAM               (0x25C00000)
/** @brief VDP1 Frame buffer (256 kilobytes) */
# define JO_VDP1_FB                 (0x25C80000)
/** @brief VDP1 I/O register address */
# define JO_VDP1_REG                (0x25D00000)
/** @brief Last VDP1 I/O register address */
# define JO_VDP1_LAST_REG                (0x25D00016)

/** @brief TV mode selection */
# define JO_VDP1_TVMR               (*(volatile unsigned short *)0x25D00000) // 0x0
/** @brief Controls the frame buffer */
# define JO_VDP1_FBCR               (*(volatile unsigned short *)0x25D00002) // 0x0
/** @brief Plot trigger */
# define JO_VDP1_PTMR               (*(volatile unsigned short *)0x25D00004) // 0x2
/** @brief Erase/write color */
# define JO_VDP1_EWDR               (*(volatile unsigned short *)0x25D00006) // 0x0
/** @brief Erase/write upper-left position */
# define JO_VDP1_EWLR               (*(volatile unsigned short *)0x25D00008) // 0x0
/** @brief Erase/write lower-right position */
# define JO_VDP1_EWRR               (*(volatile unsigned short *)0x25D0000A) // 0x58FF
/** @brief Force draw end */
# define JO_VDP1_ENDR               (*(volatile unsigned short *)0x25D0000C) // 0x0
/** @brief Transfer end status */
# define JO_VDP1_EDSR               (*(volatile unsigned short *)0x25D00010) // 0x3
/** @brief Return the last command table processed */
# define JO_VDP1_LOPR               (*(volatile unsigned short *)0x25D00012) // N/A
/** @brief Return the current command table processed */
# define JO_VDP1_COPR               (*(volatile unsigned short *)0x25D00014) // N/A
/** @brief Mode status */
# define JO_VDP1_MODR               (*(volatile unsigned short *)0x25D00016) // 0x1100
/** @brief VDP1 user area size */
# define JO_VDP1_USER_AREA_SIZE             (0x70538)
/** @brief VDP1 user area end address */
# define JO_VDP1_USER_AREA_END_ADDR         (0x25C7E6F8) // Before the gouraud tables of the 3D (JO_VDP1_CLIP_GOURAUD_TABLE_OFFSET)
/** @brief VDP1 Sprite base address */
# define JO_VDP1_TEXTURE_DEF_BASE_ADDRESS   (0x10000)

/*
██╗   ██╗██████╗ ██████╗ ██████╗
██║   ██║██╔══██╗██╔══██╗╚════██╗
██║   ██║██║  ██║██████╔╝ █████╔╝
╚██╗ ██╔╝██║  ██║██╔═══╝ ██╔═══╝
 ╚████╔╝ ██████╔╝██║     ███████╗
  ╚═══╝  ╚═════╝ ╚═╝     ╚══════╝

*/
/** @brief VDP2 I/O register address */
# define JO_VDP2_REG                (0x25F80000)
/** @brief Last VDP2 I/O register address */
# define JO_VDP2_LAST_REG           (0x25F8011E)
/** @brief VDP2 VRAM (512 kilobytes) = A0 + A1 + B0 + B1 */
# define JO_VDP2_VRAM               (0x25E00000)
/** @brief VDP2 VRAM Bank A0 (128 kilobytes) */
# define JO_VDP2_VRAM_A0            (JO_VDP2_VRAM)
/** @brief VDP2 VRAM Bank A1 (128 kilobytes) */
# define JO_VDP2_VRAM_A1            (0x25E20000)
/** @brief VDP2 VRAM Bank B0 (128 kilobytes) */
# define JO_VDP2_VRAM_B0            (0x25E40000)
/** @brief VDP2 VRAM Bank B1 (128 kilobytes) */
# define JO_VDP2_VRAM_B1            (0x25E60000)
/** @brief VDP2 Color RAM (4 kilobytes) */
# define JO_VDP2_CRAM               (0x25F00000)
/** @brief VDP2 VRAM Bank size (128 kilobytes) */
# define JO_VDP2_BANK_SIZE          (0x20000)

# define JO_VDP2_NBG0_CEL_ADR       (JO_VDP2_VRAM_A1)
# define JO_VDP2_NBG0_MAP_ADR       (JO_VDP2_VRAM_B1)
# define JO_VDP2_NBG1_CEL_ADR       (JO_VDP2_VRAM_A0)
# define JO_VDP2_NBG1_MAP_ADR       (JO_VDP2_VRAM_B0)


/* 180002 - r/w - EXTEN - External Signal Enable Register
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    | EXLTEN   | EXSYEN   |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    | DASEL    | EXBGEN   |
       \----------|----------|----------|----------|----------|----------|----------|---------*/
# define JO_VDP2_TVMD        (*(volatile unsigned short *)0x25F80000) // 0x8110 0x8120
# define JO_VDP2_EXTEN       (*(volatile unsigned short *)0x25F80002) // 0x0
# define JO_VDP2_TVSTAT      (*(volatile unsigned short *)0x25F80004) // N/A
/** @brief TVSTAT: PAL console (50 Hz) */
# define JO_VDP2_TVSTAT_PAL  (1 << 0)
/** @brief TVSTAT: odd field of an interlace mode */
# define JO_VDP2_TVSTAT_ODD  (1 << 1)

/* 180006 - r/w - VRSIZE - VRAM Size
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | VRAMSZ   |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    | VER3     | VER2     | VER1     | VER0     |
       \----------|----------|----------|----------|----------|----------|----------|---------*/
# define JO_VDP2_VRSIZE      (*(volatile unsigned short *)0x25F80006) // 0x0

/* 180008 - r/o - HCNT - H-Counter
  bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
        |    --    |    --    |    --    |    --    |    --    |    --    | HCT9     | HCT8     |
        |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
        | HCT7     | HCT6     | HCT5     | HCT4     | HCT3     | HCT2     | HCT1     | HCT0     |
        \----------|----------|----------|----------|----------|----------|----------|---------*/
# define JO_VDP2_HCNT        (*(volatile unsigned short *)0x25F80008) // 0x275 0x26B

/* 18000A - r/o - VCNT - V-Counter
  bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
        |    --    |    --    |    --    |    --    |    --    |    --    | VCT9     | VCT8     |
        |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
        | VCT7     | VCT6     | VCT5     | VCT4     | VCT3     | VCT2     | VCT1     | VCT0     |
        \----------|----------|----------|----------|----------|----------|----------|---------*/
# define JO_VDP2_VCNT        (*(volatile unsigned short *)0x25F8000A) // 0x36

/* 18000E - r/w - RAMCTL - RAM Control
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |  CRKTE   |    --    | CRMD1    | CRMD0    |    --    |    --    | VRBMD    | VRAMD    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | RDBSB11  | RDBSB10  | RDBSB01  | RDBSB00  | RDBSA11  | RDBSA10  | RDBSA01  | RDBSA00  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/
# define JO_VDP2_RAMCTL      (*(volatile unsigned short *)0x25F8000E) // 0x1327

# define JO_VDP2_CYCA0L      (*(volatile unsigned short *)0x25F80010) // 0x5555
# define JO_VDP2_CYCA0U      (*(volatile unsigned short *)0x25F80012) // 0xFEEE
# define JO_VDP2_CYCA1L      (*(volatile unsigned short *)0x25F80014) // 0x5555
# define JO_VDP2_CYCA1U      (*(volatile unsigned short *)0x25F80016) // 0xFEEE
# define JO_VDP2_CYCB0L      (*(volatile unsigned short *)0x25F80018) // 0xFFFF
# define JO_VDP2_CYCB0U      (*(volatile unsigned short *)0x25F8001A) // 0xEEEE
# define JO_VDP2_CYCB1L      (*(volatile unsigned short *)0x25F8001C) // 0x44F
# define JO_VDP2_CYCB1U      (*(volatile unsigned short *)0x25F8001E) // 0xEEEE

/* 180020 - r/w - BGON - SCREEN DISPLAY ENABLE

  this register allows each tilemap to be enabled or disabled and also which layers are solid

  bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
        |    --    |    --    |    --    | R0TPON   | N3TPON   | N2TPON   | N1TPON   | N0TPON   |
        |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
        |    --    |    --    | R1ON     | R0ON     | N3ON     | N2ON     | N1ON     | N0ON     |
        \----------|----------|----------|----------|----------|----------|----------|---------*/
# define JO_VDP2_BGON        (*(volatile unsigned short *)0x25F80020) // 0x3

/*
180022 - MZCTL - Mosaic Control
bit->  /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/
# define JO_VDP2_MZCTL       (*(volatile unsigned short *)0x25F80022) // 0x0
# define JO_VDP2_SFSEL       (*(volatile unsigned short *)0x25F80024) // 0x0
# define JO_VDP2_SFCODE      (*(volatile unsigned short *)0x25F80026) // 0x0
# define JO_VDP2_CHCTLA      (*(volatile unsigned short *)0x25F80028) // 0x3210
# define JO_VDP2_CHCTLB      (*(volatile unsigned short *)0x25F8002A) // 0x1000
/*
18002C - BMPNA - Bitmap Palette Number (NBG0, NBG1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/
# define JO_VDP2_BMPNA       (*(volatile unsigned short *)0x25F8002C) // 0x0

/* 18002E - Bitmap Palette Number (RBG0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/
# define JO_VDP2_BMPNB       (*(volatile unsigned short *)0x25F8002E) // 0x0

/* 180030 - PNCN0 - Pattern Name Control (NBG0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | N0PNB    | N0CNSM   |    --    |    --    |    --    |    --    | N0SPR    | N0SCC    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | N0SPLT6  | N0SPLT5  | N0SPLT4  | N0SPCN4  | N0SPCN3  | N0SPCN2  | N0SPCN1  | N0SPCN0  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/
# define JO_VDP2_PNCN0       (*(volatile unsigned short *)0x25F80030) // 0x800C
# define JO_VDP2_PNCN1       (*(volatile unsigned short *)0x25F80032) // 0xC00E
# define JO_VDP2_PNCN2       (*(volatile unsigned short *)0x25F80034) // 0xC000
# define JO_VDP2_PNCN3       (*(volatile unsigned short *)0x25F80036) // 0xC000
# define JO_VDP2_PNCR        (*(volatile unsigned short *)0x25F80038) // 0xC000
# define JO_VDP2_PLSZ        (*(volatile unsigned short *)0x25F8003A) // 0x0
# define JO_VDP2_MPOFN       (*(volatile unsigned short *)0x25F8003C) // 0x0
# define JO_VDP2_MPOFR       (*(volatile unsigned short *)0x25F8003E) // 0x0

# define JO_VDP2_MPABN0      (*(volatile unsigned short *)0x25F80040) // 0x3B3B
# define JO_VDP2_MPCDN0      (*(volatile unsigned short *)0x25F80042) // 0x3B3B
# define JO_VDP2_MPABN1      (*(volatile unsigned short *)0x25F80044) // 0x3939
# define JO_VDP2_MPCDN1      (*(volatile unsigned short *)0x25F80046) // 0x3939
# define JO_VDP2_MPABN2      (*(volatile unsigned short *)0x25F80048) // 0x0
# define JO_VDP2_MPCDN2      (*(volatile unsigned short *)0x25F8004A) // 0x0
# define JO_VDP2_MPABN3      (*(volatile unsigned short *)0x25F8004C) // 0x0
# define JO_VDP2_MPCDN3      (*(volatile unsigned short *)0x25F8004E) // 0x0

# define JO_VDP2_MPABRA      (*(volatile unsigned short *)0x25F80050) // 0x2020
# define JO_VDP2_MPCDRA      (*(volatile unsigned short *)0x25F80052) // 0x2020
# define JO_VDP2_MPEFRA      (*(volatile unsigned short *)0x25F80054) // 0x2020
# define JO_VDP2_MPGHRA      (*(volatile unsigned short *)0x25F80056) // 0x2020
# define JO_VDP2_MPIJRA      (*(volatile unsigned short *)0x25F80058) // 0x2020
# define JO_VDP2_MPKLRA      (*(volatile unsigned short *)0x25F8005A) // 0x2020
# define JO_VDP2_MPMNRA      (*(volatile unsigned short *)0x25F8005C) // 0x2020
# define JO_VDP2_MPOPRA      (*(volatile unsigned short *)0x25F8005E) // 0x2020

# define JO_VDP2_MPABRB      (*(volatile unsigned short *)0x25F80060) // 0x2828
# define JO_VDP2_MPCDRB      (*(volatile unsigned short *)0x25F80062) // 0x2828
# define JO_VDP2_MPEFRB      (*(volatile unsigned short *)0x25F80064) // 0x2828
# define JO_VDP2_MPGHRB      (*(volatile unsigned short *)0x25F80066) // 0x2838
# define JO_VDP2_MPIJRB      (*(volatile unsigned short *)0x25F80068) // 0x2828
# define JO_VDP2_MPKLRB      (*(volatile unsigned short *)0x25F8006A) // 0x2828
# define JO_VDP2_MPMNRB      (*(volatile unsigned short *)0x25F8006C) // 0x2828
# define JO_VDP2_MPOPRB      (*(volatile unsigned short *)0x25F8006E) // 0x2828

# define JO_VDP2_SCXIN0      (*(volatile unsigned short *)0x25F80070) // 0x0
# define JO_VDP2_SCXDN0      (*(volatile unsigned short *)0x25F80072) // 0x0
# define JO_VDP2_SCYIN0      (*(volatile unsigned short *)0x25F80074) // 0x0
# define JO_VDP2_SCYDN0      (*(volatile unsigned short *)0x25F80076) // 0x0
# define JO_VDP2_ZMXIN0      (*(volatile unsigned short *)0x25F80078) // 0x1
# define JO_VDP2_ZMXDN0      (*(volatile unsigned short *)0x25F8007A) // 0x0
# define JO_VDP2_ZMYIN0      (*(volatile unsigned short *)0x25F8007C) // 0x1
# define JO_VDP2_ZMYDN0      (*(volatile unsigned short *)0x25F8007E) // 0x0

# define JO_VDP2_SCXIN1      (*(volatile unsigned short *)0x25F80080) // 0x0
# define JO_VDP2_SCXDN1      (*(volatile unsigned short *)0x25F80082) // 0x0
# define JO_VDP2_SCYIN1      (*(volatile unsigned short *)0x25F80084) // 0x0
# define JO_VDP2_SCYDN1      (*(volatile unsigned short *)0x25F80086) // 0x0
# define JO_VDP2_ZMXIN1      (*(volatile unsigned short *)0x25F80088) // 0x1
# define JO_VDP2_ZMXDN1      (*(volatile unsigned short *)0x25F8008A) // 0x0
# define JO_VDP2_ZMYIN1      (*(volatile unsigned short *)0x25F8008C) // 0x1
# define JO_VDP2_ZMYDN1      (*(volatile unsigned short *)0x25F8008E) // 0x0

# define JO_VDP2_SCXN2       (*(volatile unsigned short *)0x25F80090) // 0x0
# define JO_VDP2_SCYN2       (*(volatile unsigned short *)0x25F80092) // 0x0
# define JO_VDP2_SCXN3       (*(volatile unsigned short *)0x25F80094) // 0x0
# define JO_VDP2_SCYN3       (*(volatile unsigned short *)0x25F80096) // 0x0
# define JO_VDP2_ZMCTL       (*(volatile unsigned short *)0x25F80098) // 0x0
# define JO_VDP2_SCRCTL      (*(volatile unsigned short *)0x25F8009A) // 0x0
# define JO_VDP2_VCSTAU      (*(volatile unsigned short *)0x25F8009C) // 0x0
# define JO_VDP2_VCSTAL      (*(volatile unsigned short *)0x25F8009E) // 0x0

# define JO_VDP2_LSTA0U      (*(volatile unsigned short *)0x25F800A0) // 0x0
# define JO_VDP2_LSTA0L      (*(volatile unsigned short *)0x25F800A2) // 0x0
# define JO_VDP2_LSTA1U      (*(volatile unsigned short *)0x25F800A4) // 0x0
# define JO_VDP2_LSTA1L      (*(volatile unsigned short *)0x25F800A6) // 0x0
# define JO_VDP2_LCTAU       (*(volatile unsigned short *)0x25F800A8) // 0x0
# define JO_VDP2_LCTAL       (*(volatile unsigned short *)0x25F800AA) // 0x0
# define JO_VDP2_BKTAU       (*(volatile unsigned short *)0x25F800AC) // 0x12F1
# define JO_VDP2_BKTAL       (*(volatile unsigned short *)0x25F800AE) // 0xFFFF

# define JO_VDP2_RPMD        (*(volatile unsigned short *)0x25F800B0) // 0x0
# define JO_VDP2_RPRCTL      (*(volatile unsigned short *)0x25F800B2) // 0x0
# define JO_VDP2_KTCTL       (*(volatile unsigned short *)0x25F800B4) // 0x0
# define JO_VDP2_KTAOF       (*(volatile unsigned short *)0x25F800B6) // 0x0
# define JO_VDP2_OVPNRA      (*(volatile unsigned short *)0x25F800B8) // 0x0
# define JO_VDP2_OVPNRB      (*(volatile unsigned short *)0x25F800BA) // 0x0
# define JO_VDP2_RPTAU       (*(volatile unsigned short *)0x25F800BC) // 0x1
# define JO_VDP2_RPTAL       (*(volatile unsigned short *)0x25F800BE) // 0xFF80

# define JO_VDP2_WPSX0       (*(volatile unsigned short *)0x25F800C0) // 0x0
# define JO_VDP2_WPSY0       (*(volatile unsigned short *)0x25F800C2) // 0x0
# define JO_VDP2_WPEX0       (*(volatile unsigned short *)0x25F800C4) // 0x27E  SCRW-1
# define JO_VDP2_WPEY0       (*(volatile unsigned short *)0x25F800C6) // 0x1BE  SCRH-1
# define JO_VDP2_WPSX1       (*(volatile unsigned short *)0x25F800C8) // 0x0
# define JO_VDP2_WPSY1       (*(volatile unsigned short *)0x25F800CA) // 0x0
# define JO_VDP2_WPEX1       (*(volatile unsigned short *)0x25F800CC) // 0x27E  SCRW-1
# define JO_VDP2_WPEY1       (*(volatile unsigned short *)0x25F800CE) // 0x1BE  SCRH-1

# define JO_VDP2_WCTLA       (*(volatile unsigned short *)0x25F800D0) // 0x0
# define JO_VDP2_WCTLB       (*(volatile unsigned short *)0x25F800D2) // 0x0
# define JO_VDP2_WCTLC       (*(volatile unsigned short *)0x25F800D4) // 0x0
# define JO_VDP2_WCTLD       (*(volatile unsigned short *)0x25F800D6) // 0x0
# define JO_VDP2_LWTA0U      (*(volatile unsigned short *)0x25F800D8) // 0x0
# define JO_VDP2_LWTA0L      (*(volatile unsigned short *)0x25F800DA) // 0x0
# define JO_VDP2_LWTA1U      (*(volatile unsigned short *)0x25F800DC) // 0x0
# define JO_VDP2_LWTA1L      (*(volatile unsigned short *)0x25F800DE) // 0x0

# define JO_VDP2_SPCTL       (*(volatile unsigned short *)0x25F800E0) // 0x23
# define JO_VDP2_SDCTL       (*(volatile unsigned short *)0x25F800E2) // 0x0
# define JO_VDP2_CRAOFA      (*(volatile unsigned short *)0x25F800E4) // 0x0
# define JO_VDP2_CRAOFB      (*(volatile unsigned short *)0x25F800E6) // 0x0
/** @brief CRAOFB: sprite color RAM address offset bits */
# define JO_VDP2_CRAOFB_SPRITE_OFFSET_MASK  (0x00F0)
/** @brief CRAOFB: sprite color RAM address offset value (0 to 7) */
# define JO_VDP2_CRAOFB_SPRITE_OFFSET(N)    (((N) & 7) << 4)
# define JO_VDP2_LNCLEN      (*(volatile unsigned short *)0x25F800E8) // 0x0
# define JO_VDP2_SFPRMD      (*(volatile unsigned short *)0x25F800EA) // 0x0
# define JO_VDP2_CCCTL       (*(volatile unsigned short *)0x25F800EC) // 0x0
# define JO_VDP2_SFCCMD      (*(volatile unsigned short *)0x25F800EE) // 0x0

# define JO_VDP2_PRISA       (*(volatile unsigned short *)0x25F800F0) // 0x506
# define JO_VDP2_PRISB       (*(volatile unsigned short *)0x25F800F2) // 0x505
# define JO_VDP2_PRISC       (*(volatile unsigned short *)0x25F800F4) // 0x505
# define JO_VDP2_PRISD       (*(volatile unsigned short *)0x25F800F6) // 0x505
# define JO_VDP2_PRINA       (*(volatile unsigned short *)0x25F800F8) // 0x607
# define JO_VDP2_PRINB       (*(volatile unsigned short *)0x25F800FA) // 0x102
# define JO_VDP2_PRIR        (*(volatile unsigned short *)0x25F800FC) // 0x4
# define JO_VDP2_RESERVE     (*(volatile unsigned short *)0x25F800FE) // 0x0

# define JO_VDP2_CCRSA       (*(volatile unsigned short *)0x25F80100) // 0x0
# define JO_VDP2_CCRSB       (*(volatile unsigned short *)0x25F80102) // 0x0
# define JO_VDP2_CCRSC       (*(volatile unsigned short *)0x25F80104) // 0x0
# define JO_VDP2_CCRSD       (*(volatile unsigned short *)0x25F80106) // 0x0
# define JO_VDP2_CCRNA       (*(volatile unsigned short *)0x25F80108) // 0x0
# define JO_VDP2_CCRNB       (*(volatile unsigned short *)0x25F8010A) // 0x0
# define JO_VDP2_CCRR        (*(volatile unsigned short *)0x25F8010C) // 0x0
# define JO_VDP2_CCRLB       (*(volatile unsigned short *)0x25F8010E) // 0x0

# define JO_VDP2_CLOFEN      (*(volatile unsigned short *)0x25F80110) // 0x0
# define JO_VDP2_CLOFSL      (*(volatile unsigned short *)0x25F80112) // 0x0
# define JO_VDP2_COAR        (*(volatile unsigned short *)0x25F80114) // 0x0
# define JO_VDP2_COAG        (*(volatile unsigned short *)0x25F80116) // 0x0
# define JO_VDP2_COAB        (*(volatile unsigned short *)0x25F80118) // 0x0
# define JO_VDP2_COBR        (*(volatile unsigned short *)0x25F8011A) // 0x0
# define JO_VDP2_COBG        (*(volatile unsigned short *)0x25F8011C) // 0x0
# define JO_VDP2_COBB        (*(volatile unsigned short *)0x25F8011E) // 0x0

/*
██╗    ██╗ ██████╗ ██████╗ ██╗  ██╗    ██████╗  █████╗ ███╗   ███╗
██║    ██║██╔═══██╗██╔══██╗██║ ██╔╝    ██╔══██╗██╔══██╗████╗ ████║
██║ █╗ ██║██║   ██║██████╔╝█████╔╝     ██████╔╝███████║██╔████╔██║
██║███╗██║██║   ██║██╔══██╗██╔═██╗     ██╔══██╗██╔══██║██║╚██╔╝██║
╚███╔███╔╝╚██████╔╝██║  ██║██║  ██╗    ██║  ██║██║  ██║██║ ╚═╝ ██║
 ╚══╝╚══╝  ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝    ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝     ╚═╝

*/

/** @brief High work RAM (1 MB): the fastest memory of the CPUs */
# define JO_WORK_RAM_HIGH                       (0x06000000)
/** @brief Low work RAM (1 MB): read much slower by the CPUs, no SCU DMA */
# define JO_WORK_RAM_LOW                        (0x00200000)
/** @brief Size of the high or low work RAM */
# define JO_WORK_RAM_SIZE                       (0x100000)
/** @brief ROM BIOS functions */
# define JO_WORK_RAM_BOOT_ROM                   (0x06000000)
/** @brief Slave CPU stack */
# define JO_WORK_RAM_SLAVE_CPU_STACK_AREA       (0x06000800)
/** @brief Master CPU stack (used by the boot ROM) */
# define JO_WORK_RAM_MASTER_CPU_DEFAULT_STACK   (0x06001000)
/** @brief IP.BIN and boot ROM work area */
# define JO_WORK_RAM_UNKNOWN_USAGE              (0x06002000)
/** @brief User area */
# define JO_WORK_RAM_USER_PROGRAM_AND_DATA      (0x06004000)
/** @brief Start of the engine work area (VDP1 command lists, 3D buffers, etc): after the program and its data (see jo.linker) */
extern unsigned int                             _jo_work_start;
# define JO_WORK_RAM_ENGINE_WORK                ((unsigned int)&_jo_work_start)
/** @brief End of the user area (the free high work RAM after the engine work area is given to jo_malloc()) */
# define JO_WORK_RAM_USER_PROGRAM_AND_DATA_END  (JO_WORK_RAM_ENGINE_WORK)
/** @brief Master CPU stack (top) */
# define JO_WORK_RAM_STACK_TOP                  (0x060FFC00)
/** @brief Master CPU stack (bottom, 16 KB, see jo.linker) */
# define JO_WORK_RAM_STACK_BOTTOM               (0x060FBC00)
/** @brief Slave CPU stack (top) */
# define JO_WORK_RAM_SLAVE_STACK_TOP            (0x06004000)

/** @brief Extended RAM cartridge identifier */
# define JO_RAM_CARTRIDGE_ID                    (*(volatile unsigned char *)0x24FFFFFF)
/** @brief Extended RAM cartridge identifier: 1 MB (8 Mbits) */
# define JO_RAM_CARTRIDGE_ID_8MBITS             (0x5a)
/** @brief Extended RAM cartridge identifier: 4 MB (32 Mbits) */
# define JO_RAM_CARTRIDGE_ID_32MBITS            (0x5c)
/** @brief Extended RAM cartridge: write 1 to enable the RAM (16 bits write) */
# define JO_RAM_CARTRIDGE_ENABLE                (*(volatile unsigned short *)0x257EFFFE)
/** @brief Extended RAM cartridge: RAM address */
# define JO_RAM_CARTRIDGE_ADDRESS               (0x22400000)
/** @brief Extended RAM cartridge 1 MB: second bank address */
# define JO_RAM_CARTRIDGE_8MBITS_BANK2_ADDRESS  (0x22600000)
/** @brief Extended RAM cartridge 1 MB: size of a bank */
# define JO_RAM_CARTRIDGE_8MBITS_BANK_SIZE      (0x80000)
/** @brief Extended RAM cartridge 4 MB: size */
# define JO_RAM_CARTRIDGE_32MBITS_SIZE          (0x400000)
/** @brief A-bus timings for the extended RAM cartridge (JO_SCU_ASR0) */
# define JO_RAM_CARTRIDGE_ASR0                  (0x23301FF0)
/** @brief A-bus refresh for the extended RAM cartridge (JO_SCU_AREF) */
# define JO_RAM_CARTRIDGE_AREF                  (0x00000013)

/** @brief Put a variable in the engine work area (not cleared by the program loader, cleared at startup) */
# define JO_WORK_RAM_ENGINE_WORK_SECTION        __attribute__((section("JO_WORK")))
/** @brief Engine work area, for a buffer aligned on a big boundary (placed first: no hole in the work area) */
# define JO_WORK_RAM_ENGINE_WORK_ALIGNED_SECTION __attribute__((section("JO_WORK_ALIGNED")))

/*
███████╗ ██████╗██╗   ██╗
██╔════╝██╔════╝██║   ██║
███████╗██║     ██║   ██║
╚════██║██║     ██║   ██║
███████║╚██████╗╚██████╔╝
╚══════╝ ╚═════╝ ╚═════╝
*/

/** @brief SCU interrupt mask register */
# define JO_SCU_IMS                 (*(volatile unsigned int *)0x25FE00A0)
/** @brief Copy of the SCU interrupt mask kept by the BIOS (SYS_GETSCUIM): the interrupt handlers of the BIOS write it to
 *  JO_SCU_IMS when they return, it must always be equal to the mask written to JO_SCU_IMS */
# define JO_BIOS_SCU_IMS            (*(volatile unsigned int *)0x06000348)
/** @brief Boot ROM routine that changes the system clock (SYS_CHGSYSCK): 0 = 26.8 MHz (320 and 640 pixels width), 1 = 28.6 MHz
 *  (352 and 704 pixels width). The VDPs, the SCU and the slave CPU are reset */
# define JO_BIOS_CHANGE_SYSTEM_CLOCK(CLOCK) ((**(void (**)(unsigned int))0x06000320)(CLOCK))
/** @brief System clock of the 320 and 640 pixels width modes (see JO_BIOS_CHANGE_SYSTEM_CLOCK()) */
# define JO_BIOS_SYSTEM_CLOCK_320   (0)
/** @brief System clock of the 352 and 704 pixels width modes (see JO_BIOS_CHANGE_SYSTEM_CLOCK()) */
# define JO_BIOS_SYSTEM_CLOCK_352   (1)
/** @brief SCU interrupt status register */
# define JO_SCU_IST                 (*(volatile unsigned int *)0x25FE00A4)
/** @brief SCU A-bus set register 0 (CS0 and CS1 access timings) */
# define JO_SCU_ASR0                (*(volatile unsigned int *)0x25FE00B0)
/** @brief SCU A-bus refresh register */
# define JO_SCU_AREF                (*(volatile unsigned int *)0x25FE00B8)
/** @brief SCU DMA level N (0 to 2): read address, write address (or indirect table), byte count, address add, enable, mode */
# define JO_SCU_DMA_READ(N)         (*(volatile unsigned int *)(0x25FE0000 + (N) * 0x20))
# define JO_SCU_DMA_WRITE(N)        (*(volatile unsigned int *)(0x25FE0004 + (N) * 0x20))
# define JO_SCU_DMA_COUNT(N)        (*(volatile unsigned int *)(0x25FE0008 + (N) * 0x20))
# define JO_SCU_DMA_ADD(N)          (*(volatile unsigned int *)(0x25FE000C + (N) * 0x20))
# define JO_SCU_DMA_ENABLE(N)       (*(volatile unsigned int *)(0x25FE0010 + (N) * 0x20))
# define JO_SCU_DMA_MODE(N)         (*(volatile unsigned int *)(0x25FE0014 + (N) * 0x20))
/** @brief SCU DMA status register */
# define JO_SCU_DMA_STATUS          (*(volatile unsigned int *)0x25FE007C)
/** @brief SCU DMA status: level N in operation */
# define JO_SCU_DMA_STATUS_BUSY(N)  (1 << (4 + (N) * 4))
/** @brief SCU DMA add: read address + 4, write address + 2 (B-bus: VDP1, VDP2, sound) */
# define JO_SCU_DMA_ADD_READ4_WRITE2    (0x101)
/** @brief SCU DMA enable: enabled and started */
# define JO_SCU_DMA_ENABLE_START    (0x101)
/** @brief SCU DMA mode: direct mode, started by JO_SCU_DMA_ENABLE_START */
# define JO_SCU_DMA_MODE_DIRECT     (0x00000007)
/** @brief SCU DMA mode: indirect mode (table of transfers: byte count, write address, read address | 0x80000000 for the last one) */
# define JO_SCU_DMA_MODE_INDIRECT   (0x01000007)
/** @brief SCU DMA indirect table: end of the table (read address of the last transfer) */
# define JO_SCU_DMA_INDIRECT_END    (0x80000000)

/** @brief SCU interrupt mask bits */
# define JO_SCU_INT_VBLANK_IN       (1 << 0)
# define JO_SCU_INT_VBLANK_OUT      (1 << 1)
# define JO_SCU_INT_HBLANK_IN       (1 << 2)
# define JO_SCU_INT_TIMER0          (1 << 3)
# define JO_SCU_INT_TIMER1          (1 << 4)
# define JO_SCU_INT_SOUND           (1 << 6)
# define JO_SCU_INT_SMPC            (1 << 7)
# define JO_SCU_INT_SPRITE_END      (1 << 13)
# define JO_SCU_INT_ALL             (0x0000BFFF)

/** @brief SH-2 interrupt vectors used by the SCU */
# define JO_VECTOR_VBLANK_IN        (0x40)
# define JO_VECTOR_VBLANK_OUT       (0x41)
# define JO_VECTOR_HBLANK_IN        (0x42)
# define JO_VECTOR_TIMER0           (0x43)
# define JO_VECTOR_TIMER1           (0x44)
# define JO_VECTOR_SOUND            (0x46)
# define JO_VECTOR_SMPC             (0x47)
# define JO_VECTOR_SPRITE_END       (0x4D)

/*
███████╗██╗  ██╗    ██████╗
██╔════╝██║  ██║    ╚════██╗
███████╗███████║     █████╔╝
╚════██║██╔══██║    ██╔═══╝
███████║██║  ██║    ███████╗
╚══════╝╚═╝  ╚═╝    ╚══════╝
*/

/** @brief Cache-through mirror of an address (to share data between the two CPU) */
# define JO_CACHE_THROUGH(ADDR)     ((unsigned int)(ADDR) | 0x20000000)
/** @brief Cache control register */
# define JO_SH2_CCR                 (*(volatile unsigned char *)0xFFFFFE92)
/** @brief CCR: cache enabled */
# define JO_SH2_CCR_CACHE_ENABLE    (1 << 0)
/** @brief CCR: cache purge (all entries invalidated) */
# define JO_SH2_CCR_CACHE_PURGE     (1 << 4)
/** @brief Status register: interrupt mask bits (level 15 = all interrupts masked) */
# define JO_SH2_SR_INTERRUPT_MASK   (0xF0)
/** @brief Status register: all interrupts enabled */
# define JO_SH2_SR_INTERRUPTS_ON    (0x00)
/** @brief Free running timer: interrupt enable register */
# define JO_SH2_FRT_TIER            (*(volatile unsigned char *)0xFFFFFE10)
/** @brief Free running timer: control/status register */
# define JO_SH2_FRT_FTCSR           (*(volatile unsigned char *)0xFFFFFE11)
/** @brief TIER: all FRT interrupts disabled (bit 0 is always 1) */
# define JO_SH2_FRT_TIER_NO_INTERRUPT   (1 << 0)
/** @brief FTCSR: input capture flag */
# define JO_SH2_FRT_FTCSR_ICF       (1 << 7)
/** @brief FTCSR: output compare flag A */
# define JO_SH2_FRT_FTCSR_OCFA      (1 << 3)
/** @brief TIER: input capture interrupt enabled */
# define JO_SH2_FRT_TIER_ICIE       (1 << 7)
/** @brief TIER: output compare interrupt A enabled */
# define JO_SH2_FRT_TIER_OCIAE      (1 << 3)
/** @brief Free running timer: output compare register A (high byte, see JO_SH2_FRT_TOCR)  */
# define JO_SH2_FRT_OCRAH           (*(volatile unsigned char *)0xFFFFFE14)
/** @brief Free running timer: output compare register A (low byte, written last) */
# define JO_SH2_FRT_OCRAL           (*(volatile unsigned char *)0xFFFFFE15)
/** @brief Free running timer: timer output compare control register */
# define JO_SH2_FRT_TOCR            (*(volatile unsigned char *)0xFFFFFE17)
/** @brief TOCR: output compare register A selected (bits 7 to 5 are always 1) */
# define JO_SH2_FRT_TOCR_OCRA       (0xE0)
/** @brief Interrupt controller: interrupt priority level setting register B (bits 11 to 8: free running timer) */
# define JO_SH2_IPRB                (*(volatile unsigned short *)0xFFFFFE60)
/** @brief Interrupt controller: vector number setting register C (bits 6 to 0: free running timer output compare) */
# define JO_SH2_VCRC                (*(volatile unsigned short *)0xFFFFFE66)
/** @brief Division unit: divisor register */
# define JO_SH2_DVSR                (*(volatile int *)0xFFFFFF00)
/** @brief Division unit: dividend register (32 bits division) */
# define JO_SH2_DVDNT               (*(volatile int *)0xFFFFFF04)
/** @brief Division unit: dividend register high (64 bits division) */
# define JO_SH2_DVDNTH              (*(volatile int *)0xFFFFFF10)
/** @brief Division unit: dividend register low (64 bits division, writing starts the division, the quotient is read here) */
# define JO_SH2_DVDNTL              (*(volatile int *)0xFFFFFF14)
/** @brief DMA controller channel 0: source address */
# define JO_SH2_SAR0                (*(volatile unsigned int *)0xFFFFFF80)
/** @brief DMA controller channel 0: destination address */
# define JO_SH2_DAR0                (*(volatile unsigned int *)0xFFFFFF84)
/** @brief DMA controller channel 0: transfer count (in transfer units) */
# define JO_SH2_TCR0                (*(volatile unsigned int *)0xFFFFFF88)
/** @brief DMA controller channel 0: channel control register */
# define JO_SH2_CHCR0               (*(volatile unsigned int *)0xFFFFFF8C)
/** @brief DMA controller: operation register */
# define JO_SH2_DMAOR               (*(volatile unsigned int *)0xFFFFFFB0)
/** @brief CHCR: destination address incremented */
# define JO_SH2_CHCR_DEST_INCREMENT (1 << 14)
/** @brief CHCR: source address incremented */
# define JO_SH2_CHCR_SRC_INCREMENT  (1 << 12)
/** @brief CHCR: transfer unit (byte) */
# define JO_SH2_CHCR_SIZE_8         (0 << 10)
/** @brief CHCR: transfer unit (word) */
# define JO_SH2_CHCR_SIZE_16        (1 << 10)
/** @brief CHCR: transfer unit (long word) */
# define JO_SH2_CHCR_SIZE_32        (2 << 10)
/** @brief CHCR: auto request (the transfer starts as soon as the channel is enabled, cycle steal mode) */
# define JO_SH2_CHCR_AUTO_REQUEST   (1 << 9)
/** @brief CHCR: transfer end flag */
# define JO_SH2_CHCR_TRANSFER_END   (1 << 1)
/** @brief CHCR: channel enabled */
# define JO_SH2_CHCR_ENABLE         (1 << 0)
/** @brief DMAOR: DMA enabled for all the channels */
# define JO_SH2_DMAOR_ENABLE        (1 << 0)
/** @brief Cache: associative purge of the line of an address (write 0 at JO_SH2_CACHE_PURGE_AREA | address) */
# define JO_SH2_CACHE_PURGE_AREA    (0x40000000)
/** @brief Cache: line size */
# define JO_SH2_CACHE_LINE_SIZE     (16)

/*
███████╗ ██████╗███████╗██████╗
██╔════╝██╔════╝██╔════╝██╔══██╗
███████╗██║     ███████╗██████╔╝
╚════██║██║     ╚════██║██╔═══╝
███████║╚██████╗███████║██║
╚══════╝ ╚═════╝╚══════╝╚═╝
*/

/** @brief Sound RAM (512 kilobytes) */
# define JO_SOUND_RAM               (0x25A00000)
/** @brief SCSP slot register */
# define JO_SCSP_SLOT(SLOT, OFFSET) (*(volatile unsigned short *)(0x25B00000 + ((SLOT) << 5) + (OFFSET)))
/** @brief SCSP common control register */
# define JO_SCSP_COMMON(OFFSET)     (*(volatile unsigned short *)(0x25B00400 + (OFFSET)))

/** @brief SCSP slot count */
# define JO_SCSP_SLOT_COUNT                 (32)

/*
** SCSP slot registers (offset in bytes)
*/
/** @brief Key on, loop control, 8 bits PCM, start address (high bits) */
# define JO_SCSP_SLOT_CONTROL               (0x00)
/** @brief Start address (low 16 bits) */
# define JO_SCSP_SLOT_START_ADDRESS         (0x02)
/** @brief Loop start address (in samples) */
# define JO_SCSP_SLOT_LOOP_START            (0x04)
/** @brief Loop end address (in samples) */
# define JO_SCSP_SLOT_LOOP_END              (0x06)
/** @brief Envelope: decay 2 rate, decay 1 rate, attack rate */
# define JO_SCSP_SLOT_ENVELOPE_ATTACK       (0x08)
/** @brief Envelope: key rate scaling, decay level, release rate */
# define JO_SCSP_SLOT_ENVELOPE_RELEASE      (0x0A)
/** @brief Total level (attenuation) */
# define JO_SCSP_SLOT_TOTAL_LEVEL           (0x0C)
/** @brief Modulation */
# define JO_SCSP_SLOT_MODULATION            (0x0E)
/** @brief Pitch (octave and frequency number) */
# define JO_SCSP_SLOT_PITCH                 (0x10)
/** @brief LFO */
# define JO_SCSP_SLOT_LFO                   (0x12)
/** @brief DSP input */
# define JO_SCSP_SLOT_DSP_INPUT             (0x14)
/** @brief Direct and effect send levels and pans */
# define JO_SCSP_SLOT_MIXER                 (0x16)

/*
** JO_SCSP_SLOT_CONTROL fields
*/
/** @brief Execute key on/off for all slots */
# define JO_SCSP_KEY_ON_EXECUTE             (1 << 12)
/** @brief Key on */
# define JO_SCSP_KEY_ON                     (1 << 11)
/** @brief Bits not related to the key on/off */
# define JO_SCSP_KEY_MASK                   (0x07FF)
/** @brief Normal loop */
# define JO_SCSP_LOOP_NORMAL                (1 << 5)
/** @brief 8 bits PCM (16 bits otherwise) */
# define JO_SCSP_PCM_8BITS                  (1 << 4)
/** @brief Start address high bits (address >> 16) */
# define JO_SCSP_START_ADDRESS_HIGH(ADDR)   (((ADDR) >> 16) & 0xF)
/** @brief Start address low bits */
# define JO_SCSP_START_ADDRESS_LOW(ADDR)    ((ADDR) & 0xFFFF)

/** @brief Sound source: the noise generator (SSCTL = 1) instead of the sound RAM */
# define JO_SCSP_SOURCE_NOISE               (1 << 7)

/** @brief Envelope register 0x08: decay 2 rate (bits 15-11), decay 1 rate (bits 10-6), attack rate (bits 4-0) */
# define JO_SCSP_ENVELOPE_ATTACK(AR, D1R, D2R) ((((D2R) & 0x1F) << 11) | (((D1R) & 0x1F) << 6) | ((AR) & 0x1F))
/** @brief Envelope register 0x0A: no key rate scaling (KRS = 0xF: the rates don't depend on the pitch), decay level 0,
 *  release rate */
# define JO_SCSP_ENVELOPE_RELEASE(RR)       ((0xF << 10) | ((RR) & 0x1F))
/** @brief Modulation register: modulation level (MDL, 0 to 15: 5 and more modulate), the 2 inputs (MDXSL, MDYSL: the
 *  stack of the outputs of the slots: 32 + n is the slot + n, one sample before) */
# define JO_SCSP_MODULATION(MDL, X, Y)      ((((MDL) & 0xF) << 12) | (((X) & 0x3F) << 6) | ((Y) & 0x3F))
/** @brief LFO register: frequency (LFOF: 0 to 31, 0.17 to 96 Hz), the waveform and the depth of the amplitude LFO */
# define JO_SCSP_LFO(LFOF, ALFOWS, ALFOS)   ((((LFOF) & 0x1F) << 10) | (((ALFOWS) & 3) << 3) | ((ALFOS) & 7))
/** @brief LFO waveform: triangle */
# define JO_SCSP_LFO_TRIANGLE               (2)
/** @brief DSP input register: the mix of the DSP the slot goes to (ISEL, 0 to 15) and its level (IMXL: 0 none, 1 to 7:
 *  -36 to 0 dB) */
# define JO_SCSP_DSP_SEND(ISEL, IMXL)       ((((ISEL) & 0xF) << 3) | ((IMXL) & 7))

/** @brief Envelope: attack rate maximum (no attack) */
# define JO_SCSP_ATTACK_RATE_MAX            (0x1F)
/** @brief Envelope: release rate maximum (immediate release) */
# define JO_SCSP_RELEASE_RATE_MAX           (0x1F)

/** @brief Pitch register value
 *  @param OCT Octave (-8 to 7)
 *  @param FNS Frequency number (0 to 1023): frequency = 44100 * 2^OCT * (1 + FNS / 1024)
 */
# define JO_SCSP_PITCH(OCT, FNS)            ((((OCT) & 0xF) << 11) | ((FNS) & 0x3FF))
/** @brief Maximum frequency number */
# define JO_SCSP_FNS_MAX                    (1023)
/** @brief Frequency number count per octave */
# define JO_SCSP_FNS_PER_OCTAVE             (1024)
/** @brief Sampling rate at octave 0 and frequency number 0 (Hz) */
# define JO_SCSP_BASE_SAMPLING_RATE         (44100)
/** @brief Total level register value (attenuation) from a volume
 *  @param VOLUME 0 (silent) to 127 (max)
 */
# define JO_SCSP_TOTAL_LEVEL(VOLUME)        (((127 - ((VOLUME) & 127)) << 1) & 0xFF)

/** @brief Pitch register value for a sampling rate (Internal usage)
 *  @param rate Sampling rate in Hz (0 means JO_SCSP_BASE_SAMPLING_RATE)
 *  @return Octave and frequency number (JO_SCSP_PITCH())
 *  @remarks frequency = 44100 * 2^OCT * (1 + FNS / 1024)
 */
static  __jo_force_inline unsigned short __jo_scsp_pitch(unsigned int rate)
{
    int                 oct;
    unsigned int        base;
    unsigned int        fns;

    if (rate == 0)
        rate = JO_SCSP_BASE_SAMPLING_RATE;
    oct = 0;
    base = JO_SCSP_BASE_SAMPLING_RATE;
    while (rate < base && oct > -8)
    {
        base >>= 1;
        --oct;
    }
    while (rate >= (base << 1) && oct < 7)
    {
        base <<= 1;
        ++oct;
    }
    fns = ((rate - base) * JO_SCSP_FNS_PER_OCTAVE) / base;
    if (fns > JO_SCSP_FNS_MAX)
        fns = JO_SCSP_FNS_MAX;
    return ((unsigned short)JO_SCSP_PITCH(oct, fns));
}

/*
** JO_SCSP_SLOT_MIXER fields
*/
/** @brief Direct send level (0 to 7) */
# define JO_SCSP_DIRECT_LEVEL(LEVEL)        ((LEVEL) << 13)
/** @brief Direct pan (5 bits, see JO_SCSP_PAN_xxx) */
# define JO_SCSP_DIRECT_PAN(PAN)            (((PAN) & 0x1F) << 8)
/** @brief Effect send level (0 to 7) */
# define JO_SCSP_EFFECT_LEVEL(LEVEL)        ((LEVEL) << 5)
/** @brief Effect pan (5 bits, see JO_SCSP_PAN_xxx) */
# define JO_SCSP_EFFECT_PAN(PAN)            ((PAN) & 0x1F)
/** @brief Pan: center */
# define JO_SCSP_PAN_CENTER                 (0x00)
/** @brief Pan: left side flag (bits 3-0: attenuation of the other side) */
# define JO_SCSP_PAN_LEFT_SIDE              (0x10)
/** @brief Pan: full right */
# define JO_SCSP_PAN_RIGHT                  (0x0F)
/** @brief Pan: full left */
# define JO_SCSP_PAN_LEFT                   (0x1F)
/** @brief Maximum send level */
# define JO_SCSP_LEVEL_MAX                  (7)

/** @brief Slot used for the CD audio left channel (EXTS input) */
# define JO_SCSP_CDDA_LEFT_SLOT             (16)
/** @brief Slot used for the CD audio right channel (EXTS input) */
# define JO_SCSP_CDDA_RIGHT_SLOT            (17)

/*
** SCSP common registers
*/
/** @brief Master volume register (MVOL bits 0-3, 16 bits DAC, 4 Mbits sound RAM) */
# define JO_SCSP_COMMON_MASTER_VOLUME       (0x00)
/** @brief Ring buffer of the DSP: its length (bits 8-7: 8K, 16K, 32K, 64K words) and its address (bits 6-0: 8 KB units) */
# define JO_SCSP_COMMON_RING_BUFFER         (0x02)
/** @brief Monitor register: the slot monitored (MSLC, bits 15-11, written) and the 4096 samples block it plays (CA, bits
 *  10-7, read) */
# define JO_SCSP_COMMON_MONITOR             (0x08)
/** @brief The slot monitored (JO_SCSP_COMMON_MONITOR) */
# define JO_SCSP_MONITOR_SLOT(SLOT)         ((unsigned short)((SLOT) << 11))
/** @brief The 4096 samples block played by the slot monitored (JO_SCSP_COMMON_MONITOR) */
# define JO_SCSP_MONITOR_BLOCK(VALUE)       (((VALUE) >> 7) & 0xF)
/** @brief The registers of the DSP of the SCSP: coefficients (0x700), memory addresses (0x780), program (0x800, 128
 *  steps of 4 words) */
# define JO_SCSP_DSP_AREA                   (0x25B00700)
# define JO_SCSP_DSP_AREA_SIZE              (0x500)
/** @brief The coefficients of the DSP (64 words: 13 bits, bits 15-3), offset in JO_SCSP_DSP_AREA */
# define JO_SCSP_DSP_COEF                   (0x000)
/** @brief The memory addresses of the DSP (32 words), offset in JO_SCSP_DSP_AREA */
# define JO_SCSP_DSP_ADDRESS                (0x080)
/** @brief The program of the DSP (128 steps of 4 words), offset in JO_SCSP_DSP_AREA */
# define JO_SCSP_DSP_PROGRAM                (0x100)
/** @brief 0 in the float format of the ring buffer of the DSP (exponent 12; 0x0000 is +0.5) */
# define JO_SCSP_DSP_FLOAT_ZERO             (0x6000)
/** @brief The ring buffer register (JO_SCSP_COMMON_RING_BUFFER)
 *  @param WORDS Length: 8192, 16384, 32768 or 65536 words
 *  @param ADDR Address in the sound RAM (8 KB aligned)
 */
# define JO_SCSP_RING_BUFFER(WORDS, ADDR)   ((((WORDS) >= 65536 ? 3 : (WORDS) >= 32768 ? 2 : (WORDS) >= 16384 ? 1 : 0) << 7) | (((ADDR) >> 13) & 0x7F))
/** @brief Register 0x400: the sound RAM of 4 Mbits (512 KB). Without it the SCSP addresses 2 Mbits (256 KB): the slots and
 *  the DSP read and write the upper half in the lower half (Yabause, Kronos: the sounds placed above 256 KB (the resident
 *  ones), and the samples under the ring buffer of the DSP, were silent) */
# define JO_SCSP_MEMORY_4MBITS              (0x0200)
/** @brief Master volume bits */
# define JO_SCSP_MASTER_VOLUME_MASK         (0x000F)
/** @brief Maximum master volume */
# define JO_SCSP_MASTER_VOLUME_MAX          (0x000F)

/*
██╗  ██╗███████╗██╗     ██████╗ ███████╗██████╗ ███████╗
██║  ██║██╔════╝██║     ██╔══██╗██╔════╝██╔══██╗██╔════╝
███████║█████╗  ██║     ██████╔╝█████╗  ██████╔╝███████╗
██╔══██║██╔══╝  ██║     ██╔═══╝ ██╔══╝  ██╔══██╗╚════██║
██║  ██║███████╗███████╗██║     ███████╗██║  ██║███████║
╚═╝  ╚═╝╚══════╝╚══════╝╚═╝     ╚══════╝╚═╝  ╚═╝╚══════╝

*/

/** @brief Check if the pointer is inside user area WORK RAM
 *  @param PTR Pointer
 */
# define JO_IS_PTR_INSIDE_USER_RAM_AREA(PTR)        (((unsigned int)(PTR)) >= JO_WORK_RAM_USER_PROGRAM_AND_DATA && \
                                                    ((unsigned int)(PTR)) < JO_WORK_RAM_USER_PROGRAM_AND_DATA_END)

/** @brief Check if the array is inside user area WORK RAM
 *  @param ARRAY An array like char[256]
 */
# define JO_IS_ARRAY_INSIDE_USER_RAM_AREA(ARRAY)    (JO_IS_PTR_INSIDE_USER_RAM_AREA((unsigned char *)ARRAY) && \
                                                    JO_IS_PTR_INSIDE_USER_RAM_AREA(((unsigned char *)ARRAY) + sizeof(ARRAY)))

#endif /* !__JO_SEGA_SATURN_H__ */

/*
** END OF FILE
*/
