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
#include "jo/time.h"
#include "jo/core.h"
#include "jo/math.h"
#include "jo/tools.h"
#include "jo/malloc.h"
#include "jo/list.h"
#include "jo/vdp1_command_pipeline.h"

#if !JO_COMPILE_USING_SGL

#define JO_COMMAND_TABLE_COUNT   (16)
#define JO_COMMAND_TABLE_SIZE   (sizeof(jo_vdp1_command) * JO_COMMAND_TABLE_COUNT)

/*
** GLOBALS
*/

static jo_list                  __jo_vdp1_buffer;
static unsigned int             __jo_vdp1_current_table_size = 0;
static unsigned int             __jo_vdp1_max_runtime_tables = 0;

static jo_vdp1_command          *__jo_create_new_command_table(void)
{
    jo_vdp1_command             *command_table;

    if ((command_table = jo_malloc_with_behaviour(JO_COMMAND_TABLE_SIZE, JO_MALLOC_TRY_REUSE_SAME_BLOCK_SIZE)) == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory");
#endif
        return JO_NULL;
    }
    jo_list_add_ptr(&__jo_vdp1_buffer, command_table);
    jo_memset(command_table, 0xFFFF, JO_COMMAND_TABLE_SIZE);
    JO_ZERO(__jo_vdp1_current_table_size);
    return command_table;
}

void                            jo_vdp1_buffer_init(void)
{
    jo_list_init(&__jo_vdp1_buffer);
    __jo_create_new_command_table();
}

jo_vdp1_command*                jo_vdp1_create_command(void)
{
    jo_vdp1_command             *command_table;

    if (__jo_vdp1_current_table_size >= JO_COMMAND_TABLE_COUNT)
        command_table = __jo_create_new_command_table();
    else
        command_table = (jo_vdp1_command *)__jo_vdp1_buffer.last->data.ptr;
    return &command_table[__jo_vdp1_current_table_size++];
}

void                            jo_vdp1_buffer_reset(void)
{
    jo_vdp1_command             *command_table;

    JO_ZERO(__jo_vdp1_current_table_size);
    while (__jo_vdp1_buffer.count > 1)
        jo_list_free_and_remove_last(&__jo_vdp1_buffer);
    // system clipping
    command_table = jo_vdp1_create_command();
    jo_memset(command_table, 0xFFFF, JO_COMMAND_TABLE_SIZE);
    command_table->ctrl = SetSystemClipping;
    command_table->xc = JO_TV_WIDTH;
    command_table->yc = JO_TV_HEIGHT;
    // user clipping
    command_table = jo_vdp1_create_command();
    command_table->ctrl = SetUserClipping;
    JO_ZERO(command_table->xa);
    JO_ZERO(command_table->ya);
    command_table->xc = JO_TV_WIDTH;
    command_table->yc = JO_TV_HEIGHT;
    // local coordinates
    command_table = jo_vdp1_create_command();
    command_table->ctrl = SetLocalCoordinates;
    JO_ZERO(command_table->xc);
    JO_ZERO(command_table->yc);
}

/*static void                     __jo_vdp1_sort_commands(void)
{
    bool                        done;
    jo_node                     *n;
#if JO_COMMAND_TABLE_COUNT > (1)
    int                         i;
#endif
    do
    {
        done = true;
        for (n = __jo_vdp1_buffer.first; n->next != JO_NULL; n = n->next)
        {
#if JO_COMMAND_TABLE_COUNT > (1)

            for (i = 0; i < JO_COMMAND_TABLE_COUNT; ++i)
            {

            }
#else
            if (((jo_vdp1_command *)n->data.ptr)->jo_engine_reserved < ((jo_vdp1_command *)n->next->data.ptr)->jo_engine_reserved)
            {
                done = false;
                JO_SWAP(n->data.integer, n->next->data.integer);
            }
#endif
        }
    }
    while (!done);
}*/

void                            jo_vdp1_flush(void)
{
    jo_node                     *node;
    unsigned char               *vdp1_vram_addr;
    int                         fill;

    //__jo_vdp1_sort_commands();
    vdp1_vram_addr = (unsigned char *)JO_VDP1_VRAM;
    for (node = __jo_vdp1_buffer.first; node != JO_NULL; node = node->next)
    {
        jo_dma_copy(node->data.ptr, vdp1_vram_addr, JO_COMMAND_TABLE_SIZE);
        vdp1_vram_addr += JO_COMMAND_TABLE_SIZE;
    }
    if (__jo_vdp1_max_runtime_tables)
    {
        fill = __jo_vdp1_max_runtime_tables - __jo_vdp1_buffer.count;
        if (fill > 0)
            jo_memset(vdp1_vram_addr, 0xFFFF, JO_COMMAND_TABLE_SIZE * fill);
    }
    __jo_vdp1_max_runtime_tables = __jo_vdp1_buffer.count;
}

#endif

/*
** END OF FILE
*/
