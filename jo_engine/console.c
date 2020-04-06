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
#include "jo/console_builtins.h"
#include "jo/console.h"

#ifdef JO_COMPILE_WITH_CONSOLE_SUPPORT

# define __JO_CONSOLE_CURSOR_BLINKING_FRAME_START   (20)
# define __JO_CONSOLE_TOTAL_FRAME                   (40)
# define __JO_CONSOLE_HISTORY_SIZE                  (5)
# define __JO_CONSOLE_MAX_INPUT_LENGTH              (32)
# define __JO_CONSOLE_MAX_COMMAND_ARGS              (6)

static jo_list                      __jo_history[__JO_CONSOLE_HISTORY_SIZE];
static int                          __jo_history_count = 0;
static jo_console_command_callback  __jo_command_callback = JO_NULL;

void        jo_console_register_command_handling(const jo_console_command_callback callback)
{
    __jo_command_callback = callback;
}

static __jo_force_inline void             __jo_console_handle_command(int argc, char **argv, const int vertical_output, bool *exit)
{
    jo_set_printf_color_index(0);
    if (jo_string_equals(argv[0], "exit"))
    {
        *exit = true;
        return ;
    }
    if (__jo_console_builtins_command_handler(argc, argv, vertical_output, exit))
        return ;
    if (__jo_command_callback == JO_NULL || !(__jo_command_callback(argc, argv, vertical_output, exit)))
    {
        jo_printf_with_color(0, vertical_output, JO_COLOR_INDEX_Red, "Unknown command: %s", argv[0]);
        return;
    }
}

static __jo_force_inline void           __jo_console_draw_prompt(const char * const prompt, const bool blink_cursor, const int y, const jo_list * const line, const int cursor)
{
    jo_node                             *tmp;
    int                                 i;
    int                                 prompt_length;

    prompt_length = jo_strlen(prompt) + 1;
    jo_printf_with_color(0, y, JO_COLOR_INDEX_Purple, prompt);
    i = prompt_length;
    for (tmp = line->first; tmp != JO_NULL; tmp = tmp->next)
    {
        if (blink_cursor && i == (cursor + prompt_length))
            jo_printf_with_color(i++, y, JO_COLOR_INDEX_Green, "%c", tmp->data.c == ' ' ? '_' : tmp->data.c);
        else
            jo_printf_with_color(i++, y, JO_COLOR_INDEX_White, "%c", tmp->data.c);
    }
    jo_printf_with_color(i, y, JO_COLOR_INDEX_White, "  ");
    if (blink_cursor && cursor == line->count)
        jo_printf_with_color(prompt_length + cursor, y, JO_COLOR_INDEX_Green, "_");
}

static __jo_force_inline void           __jo_console_parse_command(jo_list * const prompt, bool *exit)
{
    jo_node                             *tmp;
    char                                argv[__JO_CONSOLE_MAX_COMMAND_ARGS][__JO_CONSOLE_MAX_INPUT_LENGTH + 1];
    char                                *table[__JO_CONSOLE_MAX_COMMAND_ARGS];
    int                                 argc;
    int                                 pos;

    JO_ZERO(argc);
    tmp = prompt->first;
    while (tmp != JO_NULL && argc < __JO_CONSOLE_MAX_COMMAND_ARGS)
    {
        for (;tmp != JO_NULL && jo_tools_is_whitespace(tmp->data.c); tmp = tmp->next)
            continue;
        JO_ZERO(pos);
        for (;tmp != JO_NULL && !jo_tools_is_whitespace(tmp->data.c); tmp = tmp->next)
            argv[argc][pos++] = tmp->data.c;
        argv[argc][pos] = '\0';
        if (pos > 0)
            ++argc;
    }
    if (argc > 0)
    {
        for (JO_ZERO(pos); pos < argc; ++pos)
           table[pos] = &argv[pos][0];
        __jo_console_handle_command(argc, table, __jo_history_count + 2, exit);
    }
}

static __jo_force_inline int            __jo_console_input_up_key(jo_list * const prompt, int cursor, int *history_autocomplete_index)
{
    if (__jo_history_count <= 0)
        return (cursor);
    if (*history_autocomplete_index >= __jo_history_count || *history_autocomplete_index <= 0)
        *history_autocomplete_index = __jo_history_count - 1;
    else
        --(*history_autocomplete_index);
    jo_list_clear(prompt);
    jo_list_append(&__jo_history[*history_autocomplete_index], prompt);
    jo_clear_screen_line(__jo_history_count);
    return (prompt->count);
}

static __jo_force_inline int                __jo_console_input_down_key(jo_list * const prompt, int cursor, int *history_autocomplete_index)
{
    if (__jo_history_count <= 0)
        return (cursor);
    if ((*history_autocomplete_index + 1) >= __jo_history_count)
        *history_autocomplete_index = 0;
    else
        ++(*history_autocomplete_index);
    jo_list_clear(prompt);
    jo_list_append(&__jo_history[*history_autocomplete_index], prompt);
    jo_clear_screen_line(__jo_history_count);
    return (prompt->count);
}

static __jo_force_inline int                __jo_console_input_enter_key(jo_list * const prompt, int cursor, bool *exit)
{
    int                                     i;

    if (prompt->count <= 0)
        return (cursor);
    if (__jo_history_count >= __JO_CONSOLE_HISTORY_SIZE)
    {
        jo_list_clear(&__jo_history[0]);
        for (i = 1; i < __JO_CONSOLE_HISTORY_SIZE; ++i)
        {
            __jo_history[i - 1].count = __jo_history[i].count;
            __jo_history[i - 1].first = __jo_history[i].first;
            __jo_history[i - 1].last = __jo_history[i].last;
        }
        --__jo_history_count;
    }
    jo_list_init(&__jo_history[__jo_history_count]);
    jo_list_append(prompt, &__jo_history[__jo_history_count]);
    ++__jo_history_count;
    jo_clear_screen();
    __jo_console_parse_command(prompt, exit);
    jo_list_clear(prompt);
    return (0);
}

static __jo_force_inline int                __jo_console_input_backspace_key(jo_list * const prompt, int cursor)
{
    if (cursor > 0)
    {
        jo_list_remove_at(prompt, cursor - 1);
        return (cursor - 1);
    }
    return (cursor);
}

static __jo_force_inline int                __jo_console_input_delete_key(jo_list * const prompt, int cursor)
{
    if (cursor < prompt->count && prompt->count > 0)
    {
        jo_list_remove_at(prompt, cursor);
        if (cursor > prompt->count)
            cursor = prompt->count;
    }
    return (cursor);
}

static __jo_force_inline int                __jo_console_input_left_key(int cursor)
{
    if (cursor > 0)
        return (cursor - 1);
    return (cursor);
}

static __jo_force_inline int                __jo_console_input_right_key(jo_list * const prompt, int cursor)
{
    if (cursor < prompt->count)
        return (cursor + 1);
    return (cursor);
}

static __jo_force_inline int                __jo_console_input_end_key(jo_list * const prompt)
{
    return (prompt->count);
}

static __jo_force_inline int                __jo_console_input_home_key(void)
{
    return (0);
}

static __jo_force_inline int                __jo_console_input_handling(jo_list * const prompt, int cursor, int *history_autocomplete_index, bool *exit)
{
    jo_list_data                            data;

    data.c = jo_keyboard_get_char();
    if (data.c)
    {
        if (prompt->count >= __JO_CONSOLE_MAX_INPUT_LENGTH)
            return (cursor);
        jo_list_insert_at(prompt, data, cursor);
        return (cursor + 1);
    }
    switch (jo_keyboard_get_special_key())
    {
        case JO_KEYBOARD_END:
            return (__jo_console_input_end_key(prompt));
        case JO_KEYBOARD_HOME:
            return (__jo_console_input_home_key());
        case JO_KEYBOARD_UP:
            return (__jo_console_input_up_key(prompt, cursor, history_autocomplete_index));
        case JO_KEYBOARD_DOWN:
            return (__jo_console_input_down_key(prompt, cursor, history_autocomplete_index));
        case JO_KEYBOARD_ENTER:
            return (__jo_console_input_enter_key(prompt, cursor, exit));
        case JO_KEYBOARD_BACKSPACE:
            return (__jo_console_input_backspace_key(prompt, cursor));
        case JO_KEYBOARD_DELETE:
            return (__jo_console_input_delete_key(prompt, cursor));
        case JO_KEYBOARD_LEFT:
            return (__jo_console_input_left_key(cursor));
        case JO_KEYBOARD_RIGHT:
            return (__jo_console_input_right_key(prompt, cursor));
        default:
            return (cursor);
    }
}

static __jo_force_inline void           __jo_console_draw_history(void)
{
    jo_node                             *tmp;
    int                                 i;
    int                                 x;

    for (JO_ZERO(i); i < __jo_history_count; ++i)
    {
        jo_printf_with_color(0, i, JO_COLOR_INDEX_Yellow, "~$");
        x = 3;
        for (tmp = __jo_history[i].first; tmp != JO_NULL; tmp = tmp->next)
            jo_printf_with_color(x++, i, JO_COLOR_INDEX_White, "%c", tmp->data.c);
    }
}

void                    jo_console_show_dialog(void)
{
    int                 frame;
    jo_list             prompt;
    int                 history_autocomplete_index;
    int                 cursor;
    bool                exit;

    exit = false;
    history_autocomplete_index = __JO_CONSOLE_HISTORY_SIZE;
    jo_list_init(&prompt);
    JO_ZERO(cursor);
    jo_clear_screen();
    jo_printf_with_color(6, __jo_history_count + 2, JO_COLOR_INDEX_Green, "Jo Engine Console v1.0");
    jo_printf_with_color(0, __jo_history_count + 4, JO_COLOR_INDEX_White, "Tape \"exit\" to close the console.");
    for (JO_ZERO(frame); !exit; ++frame)
    {
        cursor = __jo_console_input_handling(&prompt, cursor, &history_autocomplete_index, &exit);
        __jo_console_draw_history();
        __jo_console_draw_prompt("~$", frame >= __JO_CONSOLE_CURSOR_BLINKING_FRAME_START, __jo_history_count, &prompt, cursor);
        if (frame > __JO_CONSOLE_TOTAL_FRAME)
            JO_ZERO(frame);
#if JO_COMPILE_USING_SGL
        slSynch();
#else
        jo_input_update();
        jo_wait_vblank_out();
        jo_wait_vblank_in();
#endif
    }
    jo_set_printf_color_index(0);
    jo_list_clear(&prompt);
    jo_clear_screen();
}

#endif /* !JO_COMPILE_WITH_CONSOLE_SUPPORT */

/*
** END OF FILE
*/
