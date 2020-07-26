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
#include "jo/tools.h"
#include "jo/malloc.h"
#include "jo/math.h"
#include "jo/list.h"

void	jo_list_init(jo_list * const list)
{
    JO_ZERO(list->count);
    list->last = JO_NULL;
    list->first = JO_NULL;
    list->allocation_behaviour = JO_MALLOC_TRY_REUSE_SAME_BLOCK_SIZE;
}

jo_node             *jo_list_insert_at(jo_list * const list, const jo_list_data data, const int index)
{
    jo_node         *next_node;
    jo_node         *node;

    if (index >= list->count)
        return jo_list_add(list, data);
    next_node = jo_list_at(list, index);
    if (next_node == JO_NULL)
        return jo_list_add(list, data);
    node = (jo_node *)jo_malloc_with_behaviour(sizeof(jo_node), list->allocation_behaviour);
    if (node == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory");
#endif
        return (node);
    }
    ++list->count;
    node->data = data;
    if (next_node->prev != JO_NULL)
    {
        next_node->prev->next = node;
        node->prev = next_node->prev;
    }
    else
    {
        node->prev = JO_NULL;
        list->first = node;
    }
    node->next = next_node;
    next_node->prev = node;
    return (node);
}

jo_node             *jo_list_add(jo_list * const list, const jo_list_data data)
{
    jo_node         *node;

    node = (jo_node *)jo_malloc_with_behaviour(sizeof(jo_node), list->allocation_behaviour);
    if (node == JO_NULL)
    {
#ifdef JO_DEBUG
        jo_core_error("Out of memory");
#endif
        return (node);
    }
    node->data = data;
    if (list->last == JO_NULL)
    {
        node->prev = JO_NULL;
        node->next = JO_NULL;
        list->first = node;
        list->last = node;
    }
    else
    {
        node->next = JO_NULL;
        list->last->next = node;
        node->prev = list->last;
        list->last = node;
    }
    ++list->count;
    return (node);
}

void            jo_list_remove(jo_list * const list, const jo_node * const node_to_delete)
{
    jo_node     *prev;
    jo_node     *next;

    prev = node_to_delete->prev;
    next = node_to_delete->next;
    if (prev != JO_NULL)
    {
        if (next != JO_NULL)
        {
            prev->next = next;
            next->prev = prev;
        }
        else
        {
            prev->next = JO_NULL;
            list->last = prev;
        }
    }
    else
    {
        if (next != JO_NULL)
        {
            next->prev = JO_NULL;
            list->first = next;
        }
        else
        {
            list->first = JO_NULL;
            list->last = JO_NULL;
        }
    }
    jo_free(node_to_delete);
    --list->count;
}

/*
** END OF FILE
*/
