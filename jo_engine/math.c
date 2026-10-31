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
** INCLUDES
*/
#include <stdbool.h>
#include "jo/conf.h"
#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/time.h"
#include "jo/core.h"
#include "jo/tools.h"
#include "jo/math.h"

/*
** Sinus table: sin(x) for x in [0, pi/2] (1024 steps, Q16.16)
** jo_sin_angle(), jo_cos_angle() and jo_sincos_angle() are inlined (see math.h)
*/
const jo_fixed                  __jo_sin_table[1025] =
{
    0x00000, 0x00065, 0x000C9, 0x0012E, 0x00192, 0x001F7, 0x0025B, 0x002C0,
    0x00324, 0x00389, 0x003ED, 0x00452, 0x004B6, 0x0051B, 0x0057F, 0x005E4,
    0x00648, 0x006AD, 0x00711, 0x00776, 0x007DA, 0x0083F, 0x008A3, 0x00908,
    0x0096C, 0x009D1, 0x00A35, 0x00A9A, 0x00AFE, 0x00B62, 0x00BC7, 0x00C2B,
    0x00C90, 0x00CF4, 0x00D59, 0x00DBD, 0x00E21, 0x00E86, 0x00EEA, 0x00F4E,
    0x00FB3, 0x01017, 0x0107B, 0x010E0, 0x01144, 0x011A8, 0x0120D, 0x01271,
    0x012D5, 0x01339, 0x0139E, 0x01402, 0x01466, 0x014CA, 0x0152E, 0x01593,
    0x015F7, 0x0165B, 0x016BF, 0x01723, 0x01787, 0x017EB, 0x01850, 0x018B4,
    0x01918, 0x0197C, 0x019E0, 0x01A44, 0x01AA8, 0x01B0C, 0x01B70, 0x01BD4,
    0x01C38, 0x01C9B, 0x01CFF, 0x01D63, 0x01DC7, 0x01E2B, 0x01E8F, 0x01EF3,
    0x01F56, 0x01FBA, 0x0201E, 0x02082, 0x020E5, 0x02149, 0x021AD, 0x02210,
    0x02274, 0x022D7, 0x0233B, 0x0239F, 0x02402, 0x02466, 0x024C9, 0x0252D,
    0x02590, 0x025F4, 0x02657, 0x026BA, 0x0271E, 0x02781, 0x027E4, 0x02848,
    0x028AB, 0x0290E, 0x02971, 0x029D5, 0x02A38, 0x02A9B, 0x02AFE, 0x02B61,
    0x02BC4, 0x02C27, 0x02C8A, 0x02CED, 0x02D50, 0x02DB3, 0x02E16, 0x02E79,
    0x02EDC, 0x02F3F, 0x02FA1, 0x03004, 0x03067, 0x030CA, 0x0312C, 0x0318F,
    0x031F1, 0x03254, 0x032B7, 0x03319, 0x0337C, 0x033DE, 0x03440, 0x034A3,
    0x03505, 0x03568, 0x035CA, 0x0362C, 0x0368E, 0x036F1, 0x03753, 0x037B5,
    0x03817, 0x03879, 0x038DB, 0x0393D, 0x0399F, 0x03A01, 0x03A63, 0x03AC5,
    0x03B27, 0x03B88, 0x03BEA, 0x03C4C, 0x03CAE, 0x03D0F, 0x03D71, 0x03DD2,
    0x03E34, 0x03E95, 0x03EF7, 0x03F58, 0x03FBA, 0x0401B, 0x0407C, 0x040DE,
    0x0413F, 0x041A0, 0x04201, 0x04262, 0x042C3, 0x04324, 0x04385, 0x043E6,
    0x04447, 0x044A8, 0x04509, 0x0456A, 0x045CB, 0x0462B, 0x0468C, 0x046EC,
    0x0474D, 0x047AE, 0x0480E, 0x0486F, 0x048CF, 0x0492F, 0x04990, 0x049F0,
    0x04A50, 0x04AB0, 0x04B10, 0x04B71, 0x04BD1, 0x04C31, 0x04C90, 0x04CF0,
    0x04D50, 0x04DB0, 0x04E10, 0x04E70, 0x04ECF, 0x04F2F, 0x04F8E, 0x04FEE,
    0x0504D, 0x050AD, 0x0510C, 0x0516C, 0x051CB, 0x0522A, 0x05289, 0x052E8,
    0x05348, 0x053A7, 0x05406, 0x05464, 0x054C3, 0x05522, 0x05581, 0x055E0,
    0x0563E, 0x0569D, 0x056FC, 0x0575A, 0x057B9, 0x05817, 0x05875, 0x058D4,
    0x05932, 0x05990, 0x059EE, 0x05A4C, 0x05AAA, 0x05B08, 0x05B66, 0x05BC4,
    0x05C22, 0x05C80, 0x05CDE, 0x05D3B, 0x05D99, 0x05DF6, 0x05E54, 0x05EB1,
    0x05F0F, 0x05F6C, 0x05FC9, 0x06026, 0x06084, 0x060E1, 0x0613E, 0x0619B,
    0x061F8, 0x06254, 0x062B1, 0x0630E, 0x0636B, 0x063C7, 0x06424, 0x06480,
    0x064DD, 0x06539, 0x06595, 0x065F2, 0x0664E, 0x066AA, 0x06706, 0x06762,
    0x067BE, 0x0681A, 0x06876, 0x068D1, 0x0692D, 0x06989, 0x069E4, 0x06A40,
    0x06A9B, 0x06AF6, 0x06B52, 0x06BAD, 0x06C08, 0x06C63, 0x06CBE, 0x06D19,
    0x06D74, 0x06DCF, 0x06E2A, 0x06E85, 0x06EDF, 0x06F3A, 0x06F94, 0x06FEF,
    0x07049, 0x070A3, 0x070FE, 0x07158, 0x071B2, 0x0720C, 0x07266, 0x072C0,
    0x0731A, 0x07373, 0x073CD, 0x07427, 0x07480, 0x074DA, 0x07533, 0x0758D,
    0x075E6, 0x0763F, 0x07698, 0x076F1, 0x0774A, 0x077A3, 0x077FC, 0x07855,
    0x078AD, 0x07906, 0x0795F, 0x079B7, 0x07A10, 0x07A68, 0x07AC0, 0x07B18,
    0x07B70, 0x07BC8, 0x07C20, 0x07C78, 0x07CD0, 0x07D28, 0x07D7F, 0x07DD7,
    0x07E2F, 0x07E86, 0x07EDD, 0x07F35, 0x07F8C, 0x07FE3, 0x0803A, 0x08091,
    0x080E8, 0x0813F, 0x08195, 0x081EC, 0x08243, 0x08299, 0x082F0, 0x08346,
    0x0839C, 0x083F2, 0x08449, 0x0849F, 0x084F5, 0x0854A, 0x085A0, 0x085F6,
    0x0864C, 0x086A1, 0x086F7, 0x0874C, 0x087A1, 0x087F6, 0x0884C, 0x088A1,
    0x088F6, 0x0894A, 0x0899F, 0x089F4, 0x08A49, 0x08A9D, 0x08AF2, 0x08B46,
    0x08B9A, 0x08BEF, 0x08C43, 0x08C97, 0x08CEB, 0x08D3F, 0x08D93, 0x08DE6,
    0x08E3A, 0x08E8D, 0x08EE1, 0x08F34, 0x08F88, 0x08FDB, 0x0902E, 0x09081,
    0x090D4, 0x09127, 0x09179, 0x091CC, 0x0921F, 0x09271, 0x092C4, 0x09316,
    0x09368, 0x093BA, 0x0940C, 0x0945E, 0x094B0, 0x09502, 0x09554, 0x095A5,
    0x095F7, 0x09648, 0x0969A, 0x096EB, 0x0973C, 0x0978D, 0x097DE, 0x0982F,
    0x09880, 0x098D0, 0x09921, 0x09972, 0x099C2, 0x09A12, 0x09A63, 0x09AB3,
    0x09B03, 0x09B53, 0x09BA3, 0x09BF2, 0x09C42, 0x09C92, 0x09CE1, 0x09D31,
    0x09D80, 0x09DCF, 0x09E1E, 0x09E6D, 0x09EBC, 0x09F0B, 0x09F5A, 0x09FA8,
    0x09FF7, 0x0A045, 0x0A094, 0x0A0E2, 0x0A130, 0x0A17E, 0x0A1CC, 0x0A21A,
    0x0A268, 0x0A2B5, 0x0A303, 0x0A350, 0x0A39E, 0x0A3EB, 0x0A438, 0x0A485,
    0x0A4D2, 0x0A51F, 0x0A56C, 0x0A5B8, 0x0A605, 0x0A652, 0x0A69E, 0x0A6EA,
    0x0A736, 0x0A782, 0x0A7CE, 0x0A81A, 0x0A866, 0x0A8B2, 0x0A8FD, 0x0A949,
    0x0A994, 0x0A9DF, 0x0AA2A, 0x0AA76, 0x0AAC1, 0x0AB0B, 0x0AB56, 0x0ABA1,
    0x0ABEB, 0x0AC36, 0x0AC80, 0x0ACCA, 0x0AD14, 0x0AD5E, 0x0ADA8, 0x0ADF2,
    0x0AE3C, 0x0AE85, 0x0AECF, 0x0AF18, 0x0AF62, 0x0AFAB, 0x0AFF4, 0x0B03D,
    0x0B086, 0x0B0CE, 0x0B117, 0x0B160, 0x0B1A8, 0x0B1F0, 0x0B239, 0x0B281,
    0x0B2C9, 0x0B311, 0x0B358, 0x0B3A0, 0x0B3E8, 0x0B42F, 0x0B477, 0x0B4BE,
    0x0B505, 0x0B54C, 0x0B593, 0x0B5DA, 0x0B620, 0x0B667, 0x0B6AD, 0x0B6F4,
    0x0B73A, 0x0B780, 0x0B7C6, 0x0B80C, 0x0B852, 0x0B898, 0x0B8DD, 0x0B923,
    0x0B968, 0x0B9AE, 0x0B9F3, 0x0BA38, 0x0BA7D, 0x0BAC1, 0x0BB06, 0x0BB4B,
    0x0BB8F, 0x0BBD4, 0x0BC18, 0x0BC5C, 0x0BCA0, 0x0BCE4, 0x0BD28, 0x0BD6B,
    0x0BDAF, 0x0BDF2, 0x0BE36, 0x0BE79, 0x0BEBC, 0x0BEFF, 0x0BF42, 0x0BF85,
    0x0BFC7, 0x0C00A, 0x0C04C, 0x0C08F, 0x0C0D1, 0x0C113, 0x0C155, 0x0C197,
    0x0C1D8, 0x0C21A, 0x0C25C, 0x0C29D, 0x0C2DE, 0x0C31F, 0x0C360, 0x0C3A1,
    0x0C3E2, 0x0C423, 0x0C463, 0x0C4A4, 0x0C4E4, 0x0C524, 0x0C564, 0x0C5A4,
    0x0C5E4, 0x0C624, 0x0C663, 0x0C6A3, 0x0C6E2, 0x0C721, 0x0C761, 0x0C7A0,
    0x0C7DE, 0x0C81D, 0x0C85C, 0x0C89A, 0x0C8D9, 0x0C917, 0x0C955, 0x0C993,
    0x0C9D1, 0x0CA0F, 0x0CA4D, 0x0CA8A, 0x0CAC7, 0x0CB05, 0x0CB42, 0x0CB7F,
    0x0CBBC, 0x0CBF9, 0x0CC35, 0x0CC72, 0x0CCAE, 0x0CCEB, 0x0CD27, 0x0CD63,
    0x0CD9F, 0x0CDDB, 0x0CE17, 0x0CE52, 0x0CE8E, 0x0CEC9, 0x0CF04, 0x0CF3F,
    0x0CF7A, 0x0CFB5, 0x0CFF0, 0x0D02A, 0x0D065, 0x0D09F, 0x0D0D9, 0x0D113,
    0x0D14D, 0x0D187, 0x0D1C1, 0x0D1FA, 0x0D234, 0x0D26D, 0x0D2A6, 0x0D2DF,
    0x0D318, 0x0D351, 0x0D38A, 0x0D3C2, 0x0D3FB, 0x0D433, 0x0D46B, 0x0D4A3,
    0x0D4DB, 0x0D513, 0x0D54B, 0x0D582, 0x0D5BA, 0x0D5F1, 0x0D628, 0x0D65F,
    0x0D696, 0x0D6CD, 0x0D703, 0x0D73A, 0x0D770, 0x0D7A6, 0x0D7DC, 0x0D812,
    0x0D848, 0x0D87E, 0x0D8B4, 0x0D8E9, 0x0D91E, 0x0D954, 0x0D989, 0x0D9BE,
    0x0D9F2, 0x0DA27, 0x0DA5C, 0x0DA90, 0x0DAC4, 0x0DAF8, 0x0DB2C, 0x0DB60,
    0x0DB94, 0x0DBC8, 0x0DBFB, 0x0DC2F, 0x0DC62, 0x0DC95, 0x0DCC8, 0x0DCFB,
    0x0DD2D, 0x0DD60, 0x0DD92, 0x0DDC5, 0x0DDF7, 0x0DE29, 0x0DE5B, 0x0DE8C,
    0x0DEBE, 0x0DEF0, 0x0DF21, 0x0DF52, 0x0DF83, 0x0DFB4, 0x0DFE5, 0x0E016,
    0x0E046, 0x0E077, 0x0E0A7, 0x0E0D7, 0x0E107, 0x0E137, 0x0E167, 0x0E196,
    0x0E1C6, 0x0E1F5, 0x0E224, 0x0E253, 0x0E282, 0x0E2B1, 0x0E2DF, 0x0E30E,
    0x0E33C, 0x0E36B, 0x0E399, 0x0E3C7, 0x0E3F4, 0x0E422, 0x0E450, 0x0E47D,
    0x0E4AA, 0x0E4D7, 0x0E504, 0x0E531, 0x0E55E, 0x0E58B, 0x0E5B7, 0x0E5E3,
    0x0E610, 0x0E63C, 0x0E667, 0x0E693, 0x0E6BF, 0x0E6EA, 0x0E716, 0x0E741,
    0x0E76C, 0x0E797, 0x0E7C2, 0x0E7EC, 0x0E817, 0x0E841, 0x0E86B, 0x0E895,
    0x0E8BF, 0x0E8E9, 0x0E913, 0x0E93C, 0x0E966, 0x0E98F, 0x0E9B8, 0x0E9E1,
    0x0EA0A, 0x0EA32, 0x0EA5B, 0x0EA83, 0x0EAAB, 0x0EAD4, 0x0EAFC, 0x0EB23,
    0x0EB4B, 0x0EB73, 0x0EB9A, 0x0EBC1, 0x0EBE8, 0x0EC0F, 0x0EC36, 0x0EC5D,
    0x0EC83, 0x0ECAA, 0x0ECD0, 0x0ECF6, 0x0ED1C, 0x0ED42, 0x0ED68, 0x0ED8D,
    0x0EDB3, 0x0EDD8, 0x0EDFD, 0x0EE22, 0x0EE47, 0x0EE6B, 0x0EE90, 0x0EEB4,
    0x0EED9, 0x0EEFD, 0x0EF21, 0x0EF45, 0x0EF68, 0x0EF8C, 0x0EFAF, 0x0EFD2,
    0x0EFF5, 0x0F018, 0x0F03B, 0x0F05E, 0x0F080, 0x0F0A3, 0x0F0C5, 0x0F0E7,
    0x0F109, 0x0F12B, 0x0F14C, 0x0F16E, 0x0F18F, 0x0F1B1, 0x0F1D2, 0x0F1F3,
    0x0F213, 0x0F234, 0x0F254, 0x0F275, 0x0F295, 0x0F2B5, 0x0F2D5, 0x0F2F5,
    0x0F314, 0x0F334, 0x0F353, 0x0F372, 0x0F391, 0x0F3B0, 0x0F3CF, 0x0F3ED,
    0x0F40C, 0x0F42A, 0x0F448, 0x0F466, 0x0F484, 0x0F4A2, 0x0F4BF, 0x0F4DD,
    0x0F4FA, 0x0F517, 0x0F534, 0x0F551, 0x0F56E, 0x0F58A, 0x0F5A6, 0x0F5C3,
    0x0F5DF, 0x0F5FB, 0x0F616, 0x0F632, 0x0F64E, 0x0F669, 0x0F684, 0x0F69F,
    0x0F6BA, 0x0F6D5, 0x0F6EF, 0x0F70A, 0x0F724, 0x0F73E, 0x0F758, 0x0F772,
    0x0F78C, 0x0F7A5, 0x0F7BF, 0x0F7D8, 0x0F7F1, 0x0F80A, 0x0F823, 0x0F83B,
    0x0F854, 0x0F86C, 0x0F885, 0x0F89D, 0x0F8B4, 0x0F8CC, 0x0F8E4, 0x0F8FB,
    0x0F913, 0x0F92A, 0x0F941, 0x0F958, 0x0F96E, 0x0F985, 0x0F99B, 0x0F9B2,
    0x0F9C8, 0x0F9DE, 0x0F9F3, 0x0FA09, 0x0FA1F, 0x0FA34, 0x0FA49, 0x0FA5E,
    0x0FA73, 0x0FA88, 0x0FA9C, 0x0FAB1, 0x0FAC5, 0x0FAD9, 0x0FAED, 0x0FB01,
    0x0FB15, 0x0FB28, 0x0FB3C, 0x0FB4F, 0x0FB62, 0x0FB75, 0x0FB88, 0x0FB9A,
    0x0FBAD, 0x0FBBF, 0x0FBD1, 0x0FBE3, 0x0FBF5, 0x0FC07, 0x0FC18, 0x0FC2A,
    0x0FC3B, 0x0FC4C, 0x0FC5D, 0x0FC6E, 0x0FC7F, 0x0FC8F, 0x0FCA0, 0x0FCB0,
    0x0FCC0, 0x0FCD0, 0x0FCDF, 0x0FCEF, 0x0FCFE, 0x0FD0E, 0x0FD1D, 0x0FD2C,
    0x0FD3B, 0x0FD49, 0x0FD58, 0x0FD66, 0x0FD74, 0x0FD83, 0x0FD90, 0x0FD9E,
    0x0FDAC, 0x0FDB9, 0x0FDC7, 0x0FDD4, 0x0FDE1, 0x0FDEE, 0x0FDFA, 0x0FE07,
    0x0FE13, 0x0FE1F, 0x0FE2B, 0x0FE37, 0x0FE43, 0x0FE4F, 0x0FE5A, 0x0FE66,
    0x0FE71, 0x0FE7C, 0x0FE87, 0x0FE91, 0x0FE9C, 0x0FEA6, 0x0FEB0, 0x0FEBA,
    0x0FEC4, 0x0FECE, 0x0FED8, 0x0FEE1, 0x0FEEB, 0x0FEF4, 0x0FEFD, 0x0FF06,
    0x0FF0E, 0x0FF17, 0x0FF1F, 0x0FF28, 0x0FF30, 0x0FF38, 0x0FF3F, 0x0FF47,
    0x0FF4E, 0x0FF56, 0x0FF5D, 0x0FF64, 0x0FF6B, 0x0FF71, 0x0FF78, 0x0FF7E,
    0x0FF85, 0x0FF8B, 0x0FF91, 0x0FF96, 0x0FF9C, 0x0FFA2, 0x0FFA7, 0x0FFAC,
    0x0FFB1, 0x0FFB6, 0x0FFBB, 0x0FFBF, 0x0FFC4, 0x0FFC8, 0x0FFCC, 0x0FFD0,
    0x0FFD4, 0x0FFD7, 0x0FFDB, 0x0FFDE, 0x0FFE1, 0x0FFE4, 0x0FFE7, 0x0FFEA,
    0x0FFEC, 0x0FFEF, 0x0FFF1, 0x0FFF3, 0x0FFF5, 0x0FFF7, 0x0FFF8, 0x0FFFA,
    0x0FFFB, 0x0FFFC, 0x0FFFD, 0x0FFFE, 0x0FFFF, 0x0FFFF, 0x10000, 0x10000,
    0x10000,
};

/*
** FIXED Q16.16 Number
*/

/*
** jo_fixed_mult(), jo_fixed_dot() and jo_fixed_div() are inlined (see math.h)
** Thanks Ponut64 for the first optimized versions!
**
**  - https://github.com/johannes-fetz/joengine/pull/15
**  - https://github.com/johannes-fetz/joengine/pull/17
**  - https://github.com/johannes-fetz/joengine/pull/19
*/

jo_fixed            jo_fixed_pow(jo_fixed x, jo_fixed y)
{
    jo_fixed        result;
    int             n;
    bool            negative;

    // Only the integer part of y is used
    n = JO_DIV_BY_65536(y);
    negative = n < 0;
    if (negative)
        n = -n;
    // Exponentiation by squaring
    for (result = JO_FIXED_1; n > 0; n >>= 1)
    {
        if (n & 1)
            result = jo_fixed_mult(result, x);
        x = jo_fixed_mult(x, x);
    }
    if (negative)
        return (jo_fixed_div(JO_FIXED_1, result));
    return (result);
}

/*
** OTHER
*/

int                 jo_gcd(int a, int b)
{
    unsigned int    u;
    unsigned int    v;
    unsigned int    t;
    int             shift;

#ifdef JO_DEBUG
    if (a <= 0)
    {
        jo_core_error("a <= 0 in jo_gcd()");
        return (-1);
    }
    if (b <= 0)
    {
        jo_core_error("b <= 0 in jo_gcd()");
        return (-1);
    }
#endif
    // Binary GCD (Stein): shifts and subtractions only. The subtraction version looped max(a, b) / min(a, b) times
    // (1000000 times for jo_gcd(1, 1000000)) and never ended with 0 or a negative value
    u = (unsigned int)JO_ABS(a);
    v = (unsigned int)JO_ABS(b);
    if (u == 0)
        return ((int)v);
    if (v == 0)
        return ((int)u);
    for (shift = 0; ((u | v) & 1) == 0; ++shift)
    {
        u >>= 1;
        v >>= 1;
    }
    while ((u & 1) == 0)
        u >>= 1;
    do
    {
        while ((v & 1) == 0)
            v >>= 1;
        if (u > v)
        {
            t = u;
            u = v;
            v = t;
        }
        v -= u;
    } while (v != 0);
    return ((int)(u << shift));
}

/** @brief Angle (degree, 0 to 45) of a slope (0 to 1): polynomial approximation of atan(t)
 *  @param t Slope (Q16.16)
 *  @return Angle in degree (Q16.16)
 */
static jo_fixed                 jo_atan_degrees_0_45(const jo_fixed t)
{
    // atan(t) ~ 45 t - t (t - 1) (14.02 + 3.80 t)
    return (45 * t - jo_fixed_mult(jo_fixed_mult(t, t - JO_FIXED_1), 918815 + jo_fixed_mult(249037, t)));
}

int                             jo_atan2_degrees(const jo_fixed y, const jo_fixed x)
{
    jo_fixed                    ax;
    jo_fixed                    ay;
    int                         a;

    ax = JO_ABS(x);
    ay = JO_ABS(y);
    if (ax == 0 && ay == 0)
        return (0);
    // First octant: the slope is 0 to 1
    if (ax >= ay)
        a = jo_fixed2int(jo_atan_degrees_0_45(jo_fixed_div(ay, ax)));
    else
        a = 90 - jo_fixed2int(jo_atan_degrees_0_45(jo_fixed_div(ax, ay)));
    // Quadrant (0 to 359), then -179 to 180
    if (x < 0)
        a = y < 0 ? 180 + a : 180 - a;
    else if (y < 0)
        a = 360 - a;
    // a is -1 to 361: no modulo (a division by libgcc)
    if (a >= 360)
        a -= 360;
    return (a > 180 ? a - 360 : a);
}

float       jo_atan2f_rad(const float y, const float x)
{
    float   atan;
    float   z;

    if (JO_IS_FLOAT_NULL(x))
    {
        if (y > 0.0f)
            return (JO_PI_2);
        if (JO_IS_FLOAT_NULL(y))
            return 0.0f;
        return (-JO_PI_2);
    }
    z = y / x;
    if (JO_FABS(z) < 1.0f)
    {
        atan = z / (1.0f + 0.28f * z * z);
        if (x < 0.0f)
        {
            if (y < 0.0f)
                return (atan - JO_PI);
            return (atan + JO_PI);
        }
    }
    else
    {
        atan = JO_PI_2 - z / (z * z + 0.28f);
        if (y < 0.0f)
            return (atan - JO_PI);
    }
    return (atan);
}

void                        jo_planar_rotate(const jo_pos2D * const point, const jo_pos2D * const origin, const int angle, jo_pos2D * const result)
{
    register int            dx;
    register int            dy;
    jo_fixed                cos_theta;
    jo_fixed                sin_theta;

    dx = point->x - origin->x;
    dy = point->y - origin->y;
    jo_sincos(angle, &sin_theta, &cos_theta);
    result->x = (jo_fixed2int(dx * cos_theta) - jo_fixed2int(dy * sin_theta)) + origin->x;
    result->y = (jo_fixed2int(dy * cos_theta) + jo_fixed2int(dx * sin_theta)) + origin->y;
}

/*
██████╗  █████╗ ███╗   ██╗██████╗  ██████╗ ███╗   ███╗
██╔══██╗██╔══██╗████╗  ██║██╔══██╗██╔═══██╗████╗ ████║
██████╔╝███████║██╔██╗ ██║██║  ██║██║   ██║██╔████╔██║
██╔══██╗██╔══██║██║╚██╗██║██║  ██║██║   ██║██║╚██╔╝██║
██║  ██║██║  ██║██║ ╚████║██████╔╝╚██████╔╝██║ ╚═╝ ██║
╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═════╝  ╚═════╝ ╚═╝     ╚═╝
*/

/* single phase linear congruential random integer generator */

#define JO_RANDOM_M             (2147483647)
#define JO_RANDOM_A             (16807)
#define JO_RANDOM_Q             (JO_RANDOM_M / JO_RANDOM_A)
#define JO_RANDOM_R             (JO_RANDOM_M % JO_RANDOM_A)

int                             jo_random_seed = 1;

int                             jo_random(int max)
{
    unsigned long long          product;
    unsigned int                seed;

    // seed = 16807 * seed mod (2^31 - 1) without division (Carta): the product has 46 bits, 2^31 = 1 mod (2^31 - 1),
    // so the high part (bits 31 and up) is added to the low part (bits 0 to 30). Same sequence as Schrage's method
    // (2 divisions and 2 modulos by libgcc)
    seed = (unsigned int)jo_random_seed;
    if (seed == 0 || seed >= JO_RANDOM_M)
    {
        // Seed out of 1 to 2^31 - 2 (0, negative, 2^31 - 1, set by the program): Schrage's method as before
        jo_random_seed = JO_RANDOM_A * (jo_random_seed % JO_RANDOM_Q) - JO_RANDOM_R * (jo_random_seed / JO_RANDOM_Q);
        if (jo_random_seed <= 0)
            jo_random_seed += JO_RANDOM_M;
    }
    else
    {
        product = (unsigned long long)seed * JO_RANDOM_A;
        seed = (unsigned int)(product & JO_RANDOM_M) + (unsigned int)(product >> 31);
        if (seed >= JO_RANDOM_M)
            seed -= JO_RANDOM_M;
        jo_random_seed = (int)seed;
    }
    // Power of 2: a mask instead of a modulo (same result, the seed is positive)
    if (max > 0 && (max & (max - 1)) == 0)
        return (jo_random_seed & (max - 1)) + 1;
    // (the modulo by the division unit, see jo_int_div(): a % b = a - (a / b) x b in C. Not ___modsi3 of the library)
    return jo_random_seed - jo_int_div(jo_random_seed, max) * max + 1;
}

/*
██╗   ██╗███████╗ ██████╗████████╗ ██████╗ ██████╗
██║   ██║██╔════╝██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗
██║   ██║█████╗  ██║        ██║   ██║   ██║██████╔╝
╚██╗ ██╔╝██╔══╝  ██║        ██║   ██║   ██║██╔══██╗
 ╚████╔╝ ███████╗╚██████╗   ██║   ╚██████╔╝██║  ██║
  ╚═══╝  ╚══════╝ ╚═════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝
*/

void            jo_vector_fixed_compute_bezier_point(const jo_fixed t, jo_vector_fixed p0, jo_vector_fixed p1, jo_vector_fixed p2, jo_vector_fixed p3, jo_vector_fixed *result)
{
    jo_fixed    tt = jo_fixed_mult(t, t);
    jo_fixed    u = JO_FIXED_1 - t;
    jo_fixed    uu = jo_fixed_mult(u, u);

    // first term
    jo_vector_fixed_muls(&p0, jo_fixed_mult(uu, u), result);
    // second term (3 * x: not a multiplication by the fixed number 3.0)
    jo_vector_fixed_muls(&p1, 3 * jo_fixed_mult(uu, t), &p1);
    jo_vector_fixed_add(result, &p1, result);
    // third term
    jo_vector_fixed_muls(&p2, 3 * jo_fixed_mult(u, tt), &p2);
    jo_vector_fixed_add(result, &p2, result);
    // fourth term
    jo_vector_fixed_muls(&p3, jo_fixed_mult(tt, t), &p3);
    jo_vector_fixed_add(result, &p3, result);
}

void            jo_vectorf_compute_bezier_point(const float t, jo_vectorf p0, jo_vectorf p1, jo_vectorf p2, jo_vectorf p3, jo_vectorf *result)
{
    float       tt = t * t;
    float       u = 1.0f - t;
    float       uu = u * u;

    // first term
    jo_vectorf_muls(&p0, uu * u, result);
    // second term
    jo_vectorf_muls(&p1, 3.0f * uu * t, &p1);
    jo_vectorf_add(result, &p1, result);
    // third term
    jo_vectorf_muls(&p2, 3.0f * u * tt, &p2);
    jo_vectorf_add(result, &p2, result);
    // fourth term
    jo_vectorf_muls(&p3, tt * t, &p3);
    jo_vectorf_add(result, &p3, result);
}

/*
███████╗ ██████╗ ██████╗ ████████╗
██╔════╝██╔═══██╗██╔══██╗╚══██╔══╝
███████╗██║   ██║██████╔╝   ██║
╚════██║██║▄▄ ██║██╔══██╗   ██║
███████║╚██████╔╝██║  ██║   ██║
╚══════╝ ╚══▀▀═╝ ╚═╝  ╚═╝   ╚═╝
*/

/*
** Square roots: the value is shifted left by an even count of bits into [2^30, 2^32) (4 constant shifts: the SH-2 has no
** variable shift), a first root is read in a table (linear interpolation between 2 entries), one step of Newton's method
** with the division unit gives the exact integer part (or 1 more: one check by a multiplication), shifted back. About
** 3 times faster than the digit by digit method (up to 32 iterations). Checked against the exact integer part of the
** root for all the 2^32 values (jo_sqrt) and all the 2^31 positive values (jo_fixed_sqrt).
*/

/** @brief sqrt(64 + i) * 2048, rounded (the root of the 8 high bits of a value in [2^30, 2^32)) */
static const unsigned short             jo_sqrt_table[193] =
{
    16384, 16512, 16638, 16764, 16888, 17012, 17135, 17257, 17378, 17498, 17618, 17736, 17854, 17971, 18087, 18203,
    18318, 18432, 18545, 18658, 18770, 18882, 18992, 19102, 19212, 19321, 19429, 19537, 19644, 19750, 19856, 19961,
    20066, 20170, 20274, 20377, 20480, 20582, 20684, 20785, 20886, 20986, 21085, 21185, 21283, 21382, 21480, 21577,
    21674, 21771, 21867, 21962, 22058, 22153, 22247, 22341, 22435, 22528, 22621, 22713, 22806, 22897, 22989, 23080,
    23170, 23261, 23351, 23440, 23530, 23619, 23707, 23796, 23884, 23971, 24059, 24146, 24232, 24319, 24405, 24491,
    24576, 24661, 24746, 24831, 24915, 24999, 25083, 25166, 25249, 25332, 25415, 25497, 25580, 25661, 25743, 25824,
    25905, 25986, 26067, 26147, 26227, 26307, 26387, 26466, 26545, 26624, 26703, 26781, 26859, 26937, 27015, 27092,
    27170, 27247, 27324, 27400, 27477, 27553, 27629, 27705, 27780, 27856, 27931, 28006, 28081, 28155, 28230, 28304,
    28378, 28452, 28525, 28599, 28672, 28745, 28818, 28891, 28963, 29035, 29108, 29180, 29251, 29323, 29394, 29466,
    29537, 29608, 29678, 29749, 29819, 29890, 29960, 30030, 30099, 30169, 30238, 30308, 30377, 30446, 30515, 30583,
    30652, 30720, 30788, 30856, 30924, 30992, 31059, 31127, 31194, 31261, 31328, 31395, 31462, 31529, 31595, 31661,
    31727, 31794, 31859, 31925, 31991, 32056, 32122, 32187, 32252, 32317, 32382, 32446, 32511, 32575, 32640, 32704,
    32768
};

/** @brief Shift a value into [2^30, 2^32) by an even count of bits
 *  @param value Value (not 0, output: shifted)
 *  @return Count of pairs of bits (the root is shifted back by this count of bits)
 */
static  __jo_force_inline unsigned int  jo_sqrt_normalize(unsigned int * const value)
{
    unsigned int                        v;
    unsigned int                        n;

    v = *value;
    JO_ZERO(n);
    if (v < (1u << 16))
    {
        v <<= 16;
        n += 8;
    }
    if (v < (1u << 24))
    {
        v <<= 8;
        n += 4;
    }
    if (v < (1u << 28))
    {
        v <<= 4;
        n += 2;
    }
    if (v < (1u << 30))
    {
        v <<= 2;
        n += 1;
    }
    *value = v;
    return (n);
}

/** @brief First root of a value in [2^30, 2^32): sqrt(v / 2^24) * 2^19, about 2^-14 near (linear interpolation)
 *  @param v Value
 */
static  __jo_force_inline unsigned int  jo_sqrt_first_root(const unsigned int v)
{
    const unsigned short                *t;
    unsigned int                        f;

    t = jo_sqrt_table + ((v >> 24) - 64);
    f = (v >> 16) & 0xFF;
    return ((unsigned int)t[0] * (256 - f) + (unsigned int)t[1] * f);
}

/** @brief Shift a root back (see jo_sqrt_normalize()): the integer part of the root of the value
 *  @param root Integer part of the root of the shifted value
 *  @param n Count of pairs of bits of the shift
 */
static  __jo_force_inline unsigned int  jo_sqrt_denormalize(unsigned int root, const unsigned int n)
{
    if (n & 8)
        root >>= 8;
    if (n & 4)
        root >>= 4;
    if (n & 2)
        root >>= 2;
    if (n & 1)
        root >>= 1;
    return (root);
}

jo_fixed                jo_fixed_sqrt(jo_fixed value)
{
    unsigned int        v;
    unsigned int        n;
    unsigned int        y;
    unsigned long long  square;

    if (value <= 0)
        return (value < 0 ? JO_FIXED_OVERFLOW : 0);
    v = (unsigned int)value;
    n = jo_sqrt_normalize(&v);
    // Root of v * 2^16 (48 bits, Q16.16 result): first root * 2^1, then y = (y + v * 2^16 / y) / 2 (64 / 32 bits division,
    // the quotient is under 2^26)
    y = jo_sqrt_first_root(v) << 1;
    JO_SH2_DVSR = (int)y;
    JO_SH2_DVDNTH = (int)(v >> 16);
    JO_SH2_DVDNTL = (int)(v << 16);
    y = (y + (unsigned int)JO_SH2_DVDNTL) >> 1;
    // (y >= the integer part of the root, 1 more at most)
    square = (unsigned long long)y * y;
    if ((unsigned int)(square >> 32) > (v >> 16) ||
        ((unsigned int)(square >> 32) == (v >> 16) && (unsigned int)square > (v << 16)))
        --y;
    return ((jo_fixed)jo_sqrt_denormalize(y, n));
}

unsigned int        jo_sqrt(unsigned int value)
{
    unsigned int    v;
    unsigned int    n;
    unsigned int    y;

    if (value == 0)
        return (0);
    v = value;
    n = jo_sqrt_normalize(&v);
    // Root of v (16 bits): first root / 2^7, then y = (y + v / y) / 2 (v as a 64 bits dividend: the division unit is signed)
    y = jo_sqrt_first_root(v) >> 7;
    JO_SH2_DVSR = (int)y;
    JO_SH2_DVDNTH = 0;
    JO_SH2_DVDNTL = (int)v;
    y = (y + (unsigned int)JO_SH2_DVDNTL) >> 1;
    // (y >= the integer part of the root, 1 more at most: y * y overflows only when y = 65536)
    if (y > 0xFFFF || y * y > v)
        --y;
    return (jo_sqrt_denormalize(y, n));
}

/*
██╗███╗   ██╗██╗   ██╗███████╗██████╗ ███████╗███████╗    ███████╗ ██████╗ ██████╗ ████████╗
██║████╗  ██║██║   ██║██╔════╝██╔══██╗██╔════╝██╔════╝    ██╔════╝██╔═══██╗██╔══██╗╚══██╔══╝
██║██╔██╗ ██║██║   ██║█████╗  ██████╔╝███████╗█████╗      ███████╗██║   ██║██████╔╝   ██║
██║██║╚██╗██║╚██╗ ██╔╝██╔══╝  ██╔══██╗╚════██║██╔══╝      ╚════██║██║▄▄ ██║██╔══██╗   ██║
██║██║ ╚████║ ╚████╔╝ ███████╗██║  ██║███████║███████╗    ███████║╚██████╔╝██║  ██║   ██║
╚═╝╚═╝  ╚═══╝  ╚═══╝  ╚══════╝╚═╝  ╚═╝╚══════╝╚══════╝    ╚══════╝ ╚══▀▀═╝ ╚═╝  ╚═╝   ╚═╝
AKA RECIPROCAL SQUARE ROOT
*/

/*
** Based on trenki2 implementation and adapted for the Jo Engine
*/

jo_fixed        jo_fixed_rsqrt(jo_fixed value)
{
    static const unsigned short rsq_tab[] =
    {
        /* domain 0.5 .. 1.0-1/16 */
		0xb504, 0xaaaa, 0xa1e8, 0x9a5f, 0x93cd, 0x8e00, 0x88d6, 0x8432,
    };
    jo_fixed    result;
    int         i;
    int         exp;

    if (value == 0)
        return (JO_FIXED_MAX);
    if (value == JO_FIXED_1)
        return (value);
    // Number of leading zeros
    exp = 31 - (int)jo_highest_bit((unsigned int)value);
    result = rsq_tab[(value >> (28 - exp)) & 0x7] << 1;
	exp -= 16;
    if (exp <= 0)
		result >>= -exp >> 1;
    else
		result <<= (exp >> 1) + (exp & 1);
    if (exp & 1)
        result = jo_fixed_mult(result, rsq_tab[0]);
    /* newton-raphson : x = x/2*(3-(a*x)*x) */
    i = 0;
    do
    {
		result = jo_fixed_mult((result >> 1), (JO_FIXED_1 * 3 - jo_fixed_mult(jo_fixed_mult(value, result), result)));
    } while(++i < 3);
    return (result);
}

/*
** Wikipedia implementation : https://en.wikipedia.org/wiki/Fast_inverse_square_root
*/
float               jo_rsqrt(float value)
{
    // A union avoids the strict aliasing issue of *(int *)&y (undefined behavior with -O2)
    union
    {
        float       f;
        int         i;
    }               bits;
    float           x2;
    float           y;
    const float     threehalfs = 1.5f;

    x2 = value * 0.5f;
    bits.f = value;
    bits.i = 0x5f3759df - (bits.i >> 1);
    y = bits.f;
    y = y * (threehalfs - (x2 * y * y));
    y = y * (threehalfs - (x2 * y * y));
    return (y);
}

/*
** END OF FILE
*/
