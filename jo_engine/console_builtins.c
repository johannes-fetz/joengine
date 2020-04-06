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

/*
** INCLUDES
*/
#include <stdbool.h>
#include "jo/sgl_prototypes.h"
#include "jo/conf.h"
#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/core.h"
#include "jo/math.h"
#include "jo/input.h"
#include "jo/keyboard.h"
#include "jo/colors.h"
#include "jo/tools.h"
#include "jo/malloc.h"
#include "jo/list.h"
#include "jo/map.h"
#include "jo/fs.h"
#include "jo/image.h"
#include "jo/sprites.h"
#include "jo/background.h"
#include "jo/console_builtins.h"

#ifdef JO_COMPILE_WITH_CONSOLE_SUPPORT
#ifndef JO_COMPILE_NO_CONSOLE_BUILTINS

static jo_color                       __jo_console_builtins_parse_color(const char * const str)
{
    if (jo_string_equals(str, "white"))
        return (JO_COLOR_White);
    if (jo_string_equals(str, "black"))
        return (JO_COLOR_Black);
    if (jo_string_equals(str, "red"))
        return (JO_COLOR_Red);
    if (jo_string_equals(str, "green"))
        return (JO_COLOR_Green);
    if (jo_string_equals(str, "yellow"))
        return (JO_COLOR_Yellow);
    if (jo_string_equals(str, "blue"))
        return (JO_COLOR_Blue);
    if (jo_string_equals(str, "purple"))
        return (JO_COLOR_Purple);
    if (jo_string_equals(str, "cyan"))
        return (JO_COLOR_Cyan);
    if (jo_string_equals(str, "gray"))
        return (JO_COLOR_Gray);
    if (jo_string_equals(str, "transparent"))
        return (JO_COLOR_Transparent);
    return (JO_COLOR_MediumPurple);
}

static __jo_force_inline void               __jo_console_builtins_background(int argc, char **argv, int vertical_output)
{
    if (argc < 2)
    {
        jo_printf_with_color(0, vertical_output, JO_COLOR_INDEX_Red, "Too few argument (%d < 2). Usage:", argc);
        jo_printf_with_color(0, vertical_output + 1, JO_COLOR_INDEX_Red, "Usage: bkg <colorname>");
        return ;
    }
    if (argc > 2)
    {
        jo_printf_with_color(0, vertical_output, JO_COLOR_INDEX_Red, "Too many argument (%d > 2). Usage:", argc);
        jo_printf_with_color(0, vertical_output + 1, JO_COLOR_INDEX_Red, "Usage: bkg <colorname>");
        return ;
    }
    jo_clear_background(__jo_console_builtins_parse_color(argv[1]));
    jo_printf_with_color(0, vertical_output, JO_COLOR_INDEX_Green, "Done.");
}

#ifdef JO_COMPILE_WITH_FS_SUPPORT

static __jo_force_inline void               __jo_console_builtins_load_sprite(int argc, char **argv, int vertical_output)
{
    int                                     result;
    char                                    *subdir;
    jo_color                                color;

    if (argc < 3)
    {
        jo_printf_with_color(0, vertical_output, JO_COLOR_INDEX_Red, "Too few argument (%d < 3). Usage:", argc);
        jo_printf_with_color(0, vertical_output + 1, JO_COLOR_INDEX_Red, "Usage: load sprite <filename>");
        jo_printf_with_color(0, vertical_output + 2, JO_COLOR_INDEX_Red, "[transparent_color_name] [subdir]");
        return ;
    }
    if (argc > 5)
    {
        jo_printf_with_color(0, vertical_output, JO_COLOR_INDEX_Red, "Too many argument (%d > 5). Usage:", argc);
        jo_printf_with_color(0, vertical_output + 1, JO_COLOR_INDEX_Red, "Usage: load sprite <filename>");
        jo_printf_with_color(0, vertical_output + 2, JO_COLOR_INDEX_Red, "[transparent_color_name] [subdir]");
        return ;
    }
    subdir = argc >= 5 ? argv[4] : (char *)JO_CURRENT_DIR;
    color = argc >= 4 ? __jo_console_builtins_parse_color(argv[3]) : JO_COLOR_Transparent;
    if (jo_endwith(argv[2], ".BIN"))
        result = jo_sprite_add_bin(subdir, argv[2], color);
#ifdef JO_COMPILE_WITH_TGA_SUPPORT
    else
        result = jo_sprite_add_tga(subdir, argv[2], color);
#endif
    if (result >= 0)
        jo_printf_with_color(0, vertical_output, JO_COLOR_INDEX_Green, "Sprite ID = %d", result);
    else
        jo_printf_with_color(0, vertical_output, JO_COLOR_INDEX_Red, "Error!");
}

static __jo_force_inline void               __jo_console_builtins_load_map(int argc, char **argv, int vertical_output)
{
    bool                                    succeeded;

    if (argc < 5)
    {
        jo_printf_with_color(0, vertical_output, JO_COLOR_INDEX_Red, "Too few argument (%d < 5). Usage:", argc);
        jo_printf_with_color(0, vertical_output + 1, JO_COLOR_INDEX_Red, "Usage: load map <layer> <zindex>");
        jo_printf_with_color(0, vertical_output + 2, JO_COLOR_INDEX_Red, "<filename> [subdir]");
        return ;
    }
    if (argc > 6)
    {
        jo_printf_with_color(0, vertical_output, JO_COLOR_INDEX_Red, "Too many argument (%d > 6). Usage:", argc);
        jo_printf_with_color(0, vertical_output + 1, JO_COLOR_INDEX_Red, "Usage: load map <layer> <zindex>");
        jo_printf_with_color(0, vertical_output + 2, JO_COLOR_INDEX_Red, "<filename> [subdir]");
        return ;
    }
    if (argc > 5)
        succeeded = jo_map_load_from_file(jo_tools_atoi(argv[2]), jo_tools_atoi(argv[3]), argv[5], argv[4]);
    else
        succeeded = jo_map_load_from_file(jo_tools_atoi(argv[2]), jo_tools_atoi(argv[3]), JO_CURRENT_DIR, argv[4]);
    if (succeeded)
        jo_printf_with_color(0, vertical_output, JO_COLOR_INDEX_Green, "Done.");
    else
        jo_printf_with_color(0, vertical_output, JO_COLOR_INDEX_Red, "Error!");
}

static __jo_force_inline void               __jo_console_builtins_load_entry_point(int argc, char **argv, int vertical_output)
{
    if (argc < 2)
    {
        jo_printf_with_color(0, vertical_output, JO_COLOR_INDEX_Red, "Too few argument for: load");
        return ;
    }
    if (jo_string_equals(argv[1], "map"))
    {
        __jo_console_builtins_load_map(argc, argv, vertical_output);
        return ;
    }
    if (jo_string_equals(argv[1], "sprite"))
    {
        __jo_console_builtins_load_sprite(argc, argv, vertical_output);
        return ;
    }
    jo_printf_with_color(0, vertical_output, JO_COLOR_INDEX_Red, "Usage: load (map) ...");
}

#endif

bool    __jo_console_builtins_command_handler(int argc, char **argv, int vertical_output, bool *exit)
{
    JO_UNUSED_ARG(exit);
    if (jo_string_equals(argv[0], "bkg"))
    {
        __jo_console_builtins_background(argc, argv, vertical_output);
        return (true);
    }
#ifdef JO_COMPILE_WITH_FS_SUPPORT
    if (jo_string_equals(argv[0], "load"))
    {
        __jo_console_builtins_load_entry_point(argc, argv, vertical_output);
        return (true);
    }
#endif
    return (false);
}

#endif /* JO_COMPILE_NO_CONSOLE_BUILTINS */
#endif /* !JO_COMPILE_WITH_CONSOLE_SUPPORT */

/*
** END OF FILE
*/
