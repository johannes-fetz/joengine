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
** Warnings of the link: a call of a routine of libgcc that is slow on the Saturn is reported by the linker with the
** function that calls it ("in function ...", or the section .text.<function>). An object file with a section
** ".gnu.warning.<symbol>" makes the linker print its text for each reference to the symbol (GNU ld). Linked by
** jo_engine_makefile unless JO_LINK_WARNINGS = 0
*/

#define JO_FLOAT_WARNING(symbol)  \
    .section .gnu.warning.symbol ; \
    .ascii "float or double: emulated in software by libgcc (the SH-2 has no FPU), slow: use jo_fixed (jo/math.h)"

JO_FLOAT_WARNING(___addsf3)
JO_FLOAT_WARNING(___subsf3)
JO_FLOAT_WARNING(___mulsf3)
JO_FLOAT_WARNING(___divsf3)
JO_FLOAT_WARNING(___negsf2)
JO_FLOAT_WARNING(___cmpsf2)
JO_FLOAT_WARNING(___eqsf2)
JO_FLOAT_WARNING(___nesf2)
JO_FLOAT_WARNING(___gtsf2)
JO_FLOAT_WARNING(___gesf2)
JO_FLOAT_WARNING(___ltsf2)
JO_FLOAT_WARNING(___lesf2)
JO_FLOAT_WARNING(___unordsf2)
JO_FLOAT_WARNING(___fixsfsi)
JO_FLOAT_WARNING(___fixunssfsi)
JO_FLOAT_WARNING(___fixsfdi)
JO_FLOAT_WARNING(___fixunssfdi)
JO_FLOAT_WARNING(___floatsisf)
JO_FLOAT_WARNING(___floatunsisf)
JO_FLOAT_WARNING(___floatdisf)
JO_FLOAT_WARNING(___floatundisf)
JO_FLOAT_WARNING(___powisf2)
JO_FLOAT_WARNING(___adddf3)
JO_FLOAT_WARNING(___subdf3)
JO_FLOAT_WARNING(___muldf3)
JO_FLOAT_WARNING(___divdf3)
JO_FLOAT_WARNING(___negdf2)
JO_FLOAT_WARNING(___cmpdf2)
JO_FLOAT_WARNING(___eqdf2)
JO_FLOAT_WARNING(___nedf2)
JO_FLOAT_WARNING(___gtdf2)
JO_FLOAT_WARNING(___gedf2)
JO_FLOAT_WARNING(___ltdf2)
JO_FLOAT_WARNING(___ledf2)
JO_FLOAT_WARNING(___unorddf2)
JO_FLOAT_WARNING(___fixdfsi)
JO_FLOAT_WARNING(___fixunsdfsi)
JO_FLOAT_WARNING(___fixdfdi)
JO_FLOAT_WARNING(___fixunsdfdi)
JO_FLOAT_WARNING(___floatsidf)
JO_FLOAT_WARNING(___floatunsidf)
JO_FLOAT_WARNING(___floatdidf)
JO_FLOAT_WARNING(___floatundidf)
JO_FLOAT_WARNING(___powidf2)
JO_FLOAT_WARNING(___extendsfdf2)
JO_FLOAT_WARNING(___truncdfsf2)
