/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2017, Johannes Fetz (johannesfetz@gmail.com)
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
 __      _______  _____    __
 \ \    / /  __ \|  __ \  /_ |
  \ \  / /| |  | | |__) |  | |
   \ \/ / | |  | |  ___/   | |
    \  /  | |__| | |       | |
     \/   |_____/|_|       |_|
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
# define JO_VDP1_USER_AREA_SIZE             (0x71D38)
/** @brief VDP1 user area end address */
# define JO_VDP1_USER_AREA_END_ADDR         (0x25C7FEF8)
/** @brief VDP1 Sprite base address */
# define JO_VDP1_TEXTURE_DEF_BASE_ADDRESS   (0x10000)

/*
 __      _______  _____    ___
 \ \    / /  __ \|  __ \  |__ \
  \ \  / /| |  | | |__) |    ) |
   \ \/ / | |  | |  ___/    / /
    \  /  | |__| | |       / /_
     \/   |_____/|_|      |____|
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

# define JO_VDP2_NBG1_CEL_ADR       (JO_VDP2_VRAM_B1 + 0x02000)
# define JO_VDP2_NBG1_MAP_ADR       (JO_VDP2_VRAM_B1 + 0x12000)

/* 180002 - r/w - EXTEN - External Signal Enable Register
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    | EXLTEN   | EXSYEN   |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    | DASEL    | EXBGEN   |
       \----------|----------|----------|----------|----------|----------|----------|---------*/
# define JO_VDP2_TVMD        (*(volatile unsigned short *)0x25F80000) // 0x8110 0x8120
# define JO_VDP2_EXTEN       (*(volatile unsigned short *)0x25F80002) // 0x0
# define JO_VDP2_TVSTAT      (*(volatile unsigned short *)0x25F80004) // N/A

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

#endif /* !__JO_SEGA_SATURN_H__ */

/*
** END OF FILE
*/
