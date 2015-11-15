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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "wsn.h"


typedef struct {
    ws_list_t l;
    int val;
} my_node_t;


ws_list_t list;


static my_node_t *
create_node(int val)
{
    my_node_t *n = (my_node_t *)malloc(sizeof(my_node_t));
    if (n == NULL)
    {
        return NULL;
    }

    n->val = val;

    return n;
}


static void
free_list(void)
{
    ws_list_t *ptr;
    my_node_t *n;

    ptr = list.next;
    while (ptr != &list)
    {
        n = ws_list_get_data(ptr, my_node_t, l);
        ptr = ptr->next;
        ws_list_remove(&n->l);
        free(n);
    }
}


bool
list_add_elements(void)
{
    int i;
    ws_list_t *ptr;
    my_node_t *n;
    bool ret;

    ws_list_init(&list);

    for (i = 0; i < 20; i++)
    {
        n = create_node(i);
        if (n == NULL)
        {
            fprintf(stderr, "failed to allocate memory for list node!\n");
            ret = false;
            goto cleanup;
        }

        ws_list_add_before(&list, &n->l);
    }

    i = 0;
    ptr = list.next;
    while (ptr != &list)
    {
        n = ws_list_get_data(ptr, my_node_t, l);

        if (n->val != i)
        {
            ret = false;
            goto cleanup;
        }

        i++;
        ptr = ptr->next;
    }

    ret = true;

cleanup:
    free_list();
    return ret;
}


static int
cmp(ws_list_t *a, ws_list_t *b)
{
    my_node_t *this = ws_list_get_data(a, my_node_t, l);
    my_node_t *that = ws_list_get_data(b, my_node_t, l);

    return (this->val < that->val) ?
        -1 : (this->val > that->val);
}

bool
list_add_elements_sorted(void)
{
    int i;
    ws_list_t *ptr;
    my_node_t *n;

    bool ret;

    int add[20] = {9, 8, 7, 1, 2, 5, 4, 3, 10, 11,
                   13, 14, 15, 18, 12, 20, 19, 6, 16, 17};

    ws_list_init(&list);

    for (i = 0; i < 20; i++)
    {
        n = create_node(add[i]);
        if (n == NULL)
        {
            fprintf(stderr, "failed to allocate memory for list node!\n");
            ret = false;
            goto cleanup;
        }

        ws_list_add_sorted(&list, &n->l, cmp);
    }

    i = 0;
    ptr = list.next;
    while (ptr != &list)
    {
        n = ws_list_get_data(ptr, my_node_t, l);
        ptr = ptr->next;

        i++;
    }

    i = 1;
    ptr = list.next;
    while (ptr != &list)
    {
        n = ws_list_get_data(ptr, my_node_t, l);

        if (n->val != i)
        {
            ret = false;
            goto cleanup;
        }

        i++;
        ptr = ptr->next;
    }

    ret = true;

cleanup:
    free_list();
    return ret;
}


bool
list_remove_elements(void)
{
    int i;
    ws_list_t *ptr;
    my_node_t *n;

    bool ret;

    int remove[10] = {1, 2, 6, 8, 10, 11, 15, 16, 17, 18};
    int expected[10] = {0, 3, 4, 5, 7, 9, 12, 13, 14, 19};

    ws_list_init(&list);

    for (i = 0; i < 20; i++)
    {
        n = create_node(i);
        if (n == NULL)
        {
            fprintf(stderr, "failed to allocate memory for list node!\n");
            ret = false;
            goto cleanup;
        }

        ws_list_add_before(&list, &n->l);
    }

    i = 0;
    ptr = list.next;
    while (ptr != &list)
    {
        n = ws_list_get_data(ptr, my_node_t, l);
        ptr = ptr->next;

        if (n->val == remove[i])
        {
            i++;
            ws_list_remove(&n->l);

            free(n);
        }
    }

    i = 0;
    ptr = list.next;
    while (ptr != &list)
    {
        n = ws_list_get_data(ptr, my_node_t, l);

        if (n->val != expected[i])
        {
            ret = false;
            goto cleanup;
        }

        i++;
        ptr = ptr->next;
    }

    ret = true;

cleanup:
    free_list();
    return ret;
}


bool
list_count_elements(void)
{
    int i;
    my_node_t *n;
    bool ret;

    ws_list_init(&list);

    for (i = 0; i < 20; i++)
    {
        n = create_node(i);
        if (n == NULL)
        {
            fprintf(stderr, "failed to allocate memory for list node!\n");
            ret = false;
            goto cleanup;
        }

        ws_list_add_before(&list, &n->l);
    }

    ret = ws_list_count(&list) == 20;

cleanup:
    free_list();
    return ret;
}


bool
list_test_empty(void)
{
    my_node_t *n;
    bool ret;

    ws_list_init(&list);

    if (!ws_list_is_empty(&list))
    {
        ret = false;
        goto cleanup;
    }

    n = create_node(1);
    ws_list_add_after(&list, &n->l);

    ret = !ws_list_is_empty(&list);

cleanup:
    free_list();
    return ret;
}


bool
list_test_first(void)
{
    int i;
    my_node_t *n, *first = NULL;
    bool ret;

    ws_list_init(&list);

    n = create_node(0);
    first = n;

    ws_list_add_after(&list, &n->l);

    for (i = 1; i < 10; i++)
    {
        n = create_node(i);
        ws_list_add_before(&list, &n->l);
    }

    ret = ws_list_is_first(&list, &first->l);

    free_list();
    return ret;
}


bool
list_test_last(void)
{
    int i;
    my_node_t *n, *last = NULL;
    bool ret;

    ws_list_init(&list);

    n = create_node(0);
    last = n;

    ws_list_add_before(&list, &n->l);

    for (i = 1; i < 10; i++)
    {
        n = create_node(i);
        ws_list_add_after(&list, &n->l);
    }

    ret = ws_list_is_last(&list, &last->l);

    free_list();
    return ret;
}
