/*
 * Copyright (c) 2015, Dan Collins
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "ws_list.h"


void
ws_list_init(ws_list_t *list)
{
    list->next = list;
    list->prev = list;
}


void
ws_list_add_after(ws_list_t *list, ws_list_t *new)
{
    new->prev = list;
    new->next = list->next;
    list->next->prev = new;
    list->next = new;
}


void
ws_list_add_before(ws_list_t *list, ws_list_t *new)
{
    new->prev = list->prev;
    new->next = list;
    list->prev->next = new;
    list->prev = new;
}


void
ws_list_add_sorted(ws_list_t *list, ws_list_t *new,
                   int (*cmp)(ws_list_t *a, ws_list_t *b))
{
    ws_list_t *ptr = list->next;

    /* If the list is empty, then we'll just add at the front */
    if (list == list->next)
    {
        ws_list_add_after(list, new);
        return;
    }

    /* If the timer is smaller than the first element, add to the head */
    if (cmp(new, list) < 1)
    {
        ws_list_add_after(list, new);
        return;
    }

    /* If the timer is bigger than the last element, add to the tail*/
    if (cmp(new, list->prev) > 0)
    {
        ws_list_add_before(list, new);
        return;
    }

    /* Scan the list until we find an element larger than the new element. We
     * are certain the new element is larger than the head and smaller than
     * the tail which means there is a place for it */
    do
    {
        if (cmp(new, ptr) < 1)
        {
            ws_list_add_before(ptr, new);
            return;
        }
        ptr = ptr->next;
    } while (ptr != list);
}


void
ws_list_remove(ws_list_t *del)
{
    del->prev->next = del->next;
    del->next->prev = del->prev;
    del->next = NULL;
    del->prev = NULL;
}


bool
ws_list_is_first(ws_list_t *head, ws_list_t *n)
{
    return (head->next == n);
}


bool
ws_list_is_last(ws_list_t *head, ws_list_t *n)
{
    return (head->prev == n);
}


bool
ws_list_is_empty(ws_list_t *list)
{
    return (list->next == list);
}


uint32_t
ws_list_count(ws_list_t *list)
{
    ws_list_t *ptr = list->next;
    uint32_t count = 0;

    while (ptr != list)
    {
        count++;
        ptr = ptr->next;
    }

    return count;
}
