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

#ifndef _WS_LIST_H
#define _WS_LIST_H

#include "wsn.h"


#define ws_list_get_data(list_node, type, list_name) \
    ((type *)((uint32_t *)(list_node) - offsetof(type, list_name)))


extern void
ws_list_init(ws_list_t *list);


extern void
ws_list_add_after(ws_list_t *list, ws_list_t *new);


extern void
ws_list_add_before(ws_list_t *list, ws_list_t *new);


extern void
ws_list_add_sorted(ws_list_t *list, ws_list_t *new,
                   int (*cmp)(ws_list_t *a, ws_list_t *b));


extern void
ws_list_remove(ws_list_t *del);


extern bool
ws_list_is_first(ws_list_t *head, ws_list_t *n);


extern bool
ws_list_is_last(ws_list_t *head, ws_list_t *n);


extern bool
ws_list_is_empty(ws_list_t *list);


extern uint32_t
ws_list_count(ws_list_t *list);


#endif
