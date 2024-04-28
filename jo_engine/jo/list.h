/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2024, Johannes Fetz (johannesfetz@gmail.com)
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
/** @file list.h
 *  @author Johannes Fetz
 *
 *  @brief Jo Engine Linked list helper
 *  @bug No known bugs.
 */

#ifndef __JO_LIST_H__
# define __JO_LIST_H__

/** @brief Node type */
typedef struct __jo_node jo_node;

/** @brief Callback for jo_list_foreach() */
typedef void (*jo_node_callback)(jo_node *node);

/** @brief Callback for jo_list_any() */
typedef bool (*jo_node_any_callback)(jo_node *node, void *extra);

/** @brief Node data (4 bytes) */
typedef union
{
    char            c;
    unsigned char   uc;
    struct
    {
        short       x;
        short       y;
    }               coord;
    char            str[4];
    bool            booleans[4];
    void            *ptr;
    char            *ch_arr;
    int             integer;
}                   jo_list_data;

/** @brief Node struct */
struct              __jo_node
{
    jo_list_data    data;
    jo_node         *prev;
    jo_node         *next;
};

/** @brief List struct */
typedef struct
{
    int                     count;
    jo_node                 *first;
    jo_node                 *last;
    jo_malloc_behaviour     allocation_behaviour;
}                           jo_list;

/** @brief Init a list
 *  @param list List pointer
 */
void	                            jo_list_init(jo_list * const list);

/** @brief Insert an item on the list at specific index
 *  @param list List
 *  @param data Item to insert
 *  @param index Node index
 *  @return Created node on the list
 */
jo_node                             *jo_list_insert_at(jo_list * const list, const jo_list_data data, const int index);

/** @brief Add an item on the list
 *  @param list List
 *  @param data item to add
 *  @return Created node on the list
 */
jo_node                             *jo_list_add(jo_list * const list, const jo_list_data data);

/** @brief Add a pointer on the list
 *  @param list List
 *  @param ptr Pointer to add <=> jo_list_data:ptr
 *  @return Created node on the list
 */
static  __jo_force_inline jo_node   *jo_list_add_ptr(jo_list * const list, void *ptr)
{
    return (jo_list_add(list, (jo_list_data)ptr));
}

/** @brief Remove an item on the list
 *  @param list List
 *  @param node_to_delete Node to remove
 */
void                                jo_list_remove(jo_list * const list, const jo_node * const node_to_delete);

/** @brief Get node at index
 *  @remarks Returns first or last node if out of bounds
 *  @param list List
 *  @param index Node index
 */
static  __jo_force_inline jo_node   *jo_list_at(jo_list * const list, int index)
{
    jo_node *tmp;
    for (tmp = list->first; tmp != JO_NULL && index > 0; tmp = tmp->next)
        --index;
    return tmp;
}

/** @brief Remove an item on the list from index
 *  @param list List
 *  @param index Node index
 */
static  __jo_force_inline void      jo_list_remove_at(jo_list * const list, const int index)
{
    jo_list_remove(list, jo_list_at(list, index));
}

/** @brief Free node pointer and remove the item from the list
 *  @param list List
 *  @param node_to_delete Node to remove
 */
static  __jo_force_inline void	    jo_list_free_and_remove(jo_list * const list, const jo_node * const node_to_delete)
{
    if (node_to_delete->data.ptr != JO_NULL) jo_free(node_to_delete->data.ptr);
    jo_list_remove(list, node_to_delete);
}

/** @brief Free node pointers and remove all item
 *  @param list List
 */
static  __jo_force_inline void	    jo_list_free_and_clear(jo_list * const list)
{
    while (list->first != JO_NULL)
    {
        if (list->first->data.ptr != JO_NULL) jo_free(list->first->data.ptr);
        jo_list_remove(list, list->first);
    }
}

/** @brief Free and remove the last item from the list
 *  @param list List
 */
static  __jo_force_inline void      jo_list_free_and_remove_last(jo_list * const list)
{
    jo_list_free_and_remove(list, list->last);
}

/** @brief Free and remove the last item from the list
 *  @param list List
 */
static  __jo_force_inline void      jo_list_remove_last(jo_list * const list)
{
    jo_list_remove(list, list->last);
}

/** @brief Free and remove the first item from the list
 *  @param list List
 */
static  __jo_force_inline void      jo_list_free_and_remove_first(jo_list * const list)
{
    jo_list_free_and_remove(list, list->first);
}

/** @brief Free and remove the first item from the list
 *  @param list List
 */
static  __jo_force_inline void      jo_list_remove_first(jo_list * const list)
{
    jo_list_remove(list, list->first);
}

/** @brief Remove all item
 *  @param list List
 */
static  __jo_force_inline void	    jo_list_clear(jo_list * const list)
{
    while (list->first != JO_NULL) jo_list_remove(list, list->first);
}

/** @brief Get first item
 *  @param list List
 */
static  __jo_force_inline jo_node   *jo_list_first(jo_list * const list)
{
    return (list->first);
}

/** @brief Get last item
 *  @param list List
 */
static  __jo_force_inline jo_node   *jo_list_last(jo_list * const list)
{
    return list->last;
}

/** @brief Iterate on the list
 *  @param list List
 *  @param callback callback for each node
 */
static  __jo_force_inline void	    jo_list_foreach(jo_list * const list, jo_node_callback callback)
{
    jo_node *tmp;
    for (tmp = list->first; tmp != JO_NULL; tmp = tmp->next) callback(tmp);
}

/** @brief Find if any element of the list satisfy the condition (callback)
 *  @param list List
 *  @param extra Extra data passed to the callback
 *  @param callback callback for each node
 *  @return true if any element satisfy the condition otherwise false
 */
static  __jo_force_inline bool	    jo_list_any(jo_list * const list, jo_node_any_callback callback, void *extra)
{
    jo_node *tmp;
    for (tmp = list->first; tmp != JO_NULL; tmp = tmp->next) if (callback(tmp, extra)) return true;
    return false;
}

/** @brief Set list memory allocation behaviour
 *  @param list List pointer
 *  @param behaviour Allocation behaviour
 */
static  __jo_force_inline void      jo_list_set_allocation_behaviour(jo_list * const list, jo_malloc_behaviour behaviour)
{
    list->allocation_behaviour = behaviour;
}

/** @brief Remove the first item on the list that match DATA
 *  @param list List
 *  @param data Node DATA
 */
static  __jo_force_inline bool	    jo_list_remove_first_value(jo_list * const list, jo_list_data data)
{
    jo_node *tmp;
    for (tmp = list->first; tmp != JO_NULL; tmp = tmp->next)
        if (tmp->data.integer == data.integer)
        {
            jo_list_remove(list, tmp);
            return (true);
        }
    return false;
}

/** @brief Remove all items on the list that match DATA
 *  @param list List
 *  @param data Node DATA
 */
static  __jo_force_inline void	    jo_list_remove_all_value(jo_list * const list, jo_list_data data)
{
    while (jo_list_remove_first_value(list, data))
        ;
}

/** @brief Append all items on the list to output list
 *  @param list List
 *  @param output Output list
 */
static  __jo_force_inline void	    jo_list_append(const jo_list * const list, jo_list * const output)
{
    jo_node *tmp;
    for (tmp = list->first; tmp != JO_NULL; tmp = tmp->next)
        jo_list_add(output, tmp->data);
}

#endif /* !__JO_LIST_H__ */

/*
** END OF FILE
*/
