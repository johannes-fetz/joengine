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
/** @file console_builtins.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Internal Console commands
 *  @bug No known bugs.
 */

#ifndef __JO_CONSOLE_BUILTINS_H__
# define __JO_CONSOLE_BUILTINS_H__

#ifdef JO_COMPILE_WITH_CONSOLE_SUPPORT
#ifndef JO_COMPILE_NO_CONSOLE_BUILTINS

/*
** Available builtins commands:
**
** bkg <colorname>
** - colorname: (Optional) white|black|red|green|yellow|blue|purple|cyan|gray|transparent
**              Default: transparent
**
** load sprite <filename> [transparent_color_name] [subdir]
** - filename: Case sensitive filename
** - transparent_color_name: (Optional) white|black|red|green|yellow|blue|purple|cyan|gray|transparent
**              Default: transparent
** - subdir: (Optional) Case sensitive subdir name
**              Default: Current directory
**
** load map <layer> <zindex> <filename> [subdir]
** - layer: Layer value beetween 0 and JO_MAP_MAX_LAYER
** - zindex: Z-Index (signed short)
** - filename: Case sensitive filename
** - subdir: (Optional) Case sensitive subdir name
**              Default: Current directory
**
** exit
*/

bool    __jo_console_builtins_command_handler(int argc, char **argv, int vertical_output, bool *exit);

#endif /* JO_COMPILE_NO_CONSOLE_BUILTINS */
#endif /* !JO_COMPILE_WITH_CONSOLE_SUPPORT */

#endif /* !__JO_CONSOLE_BUILTINS_H__ */

/*
** END OF FILE
*/
