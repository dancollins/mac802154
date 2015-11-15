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

#include "ws_pktbuf.h"


#if 1
#undef WS_DEBUG
#define WS_DEBUG(...)
#endif


struct ws_pktbuf_t
{
    /* Pointer to the first valid location for data in the buffer. We
     * store this pktbuf at the very front of the data, so we need to
     * make sure we don't run over ourself!
     */
    uint8_t *start;

    /* Pointer to the last valid location for data in the buffer. */
    uint8_t *end;

    /* Pointer to the first octet in the buffer. If we reserve space,
     * this pointer will move forwards. As data is added it will move
     * backwards until it hits the buf pointer. Removing data from the
     * front will make this pointer move forwards.
     */
    uint8_t *data_start;

    /* Pointer to the last octet in the buffer. As data is added it
     * will move forwards. Removing data from the end will make this
     * pointer move backwards
     */
    uint8_t *data_end;

    /* This holds the maximum capacity of the buffer. The size, in
     * octets, as allocated by malloc.
     */
    uint32_t size;
};


ws_pktbuf_t *
ws_pktbuf_create(uint32_t len)
{
    ws_pktbuf_t *p = (ws_pktbuf_t *)MALLOC(sizeof(ws_pktbuf_t) + len);
    if(p == NULL)
    {
        WS_ERROR("failed to allocate pktbuf\n");
        return NULL;
    }

    p->start = (uint8_t *)(p + sizeof(ws_pktbuf_t));
    p->data_start = p->start;
    p->data_end = p->start;
    p->end = p->start + len;
    p->size = len;

    WS_DEBUG("created packet (pkt=%p)\n", p);

    return p;
}


void
ws_pktbuf_destroy(ws_pktbuf_t *p)
{
    ASSERT(p != NULL, "freeing NULL pktbuf\n");
    WS_DEBUG("freeing packet (pkt=%p)\n", p);
    FREE(p);
}


void
ws_pktbuf_reset(ws_pktbuf_t *p)
{
    ASSERT(p != NULL, "resetting NULL pktbuf\n");
    p->data_start = p->start;
    p->data_end = p->start;
}


uint32_t
ws_pktbuf_reserve(ws_pktbuf_t *p, uint32_t len)
{
    uint32_t free_space;

    ASSERT(p != NULL, "reserving from NULL pktbuf\n");
    ASSERT(p->start == p->end, "reserving from a non-empty pktbuf\n");

    /* Make sure we can reserve that much data */
    free_space = p->size - (uint32_t)(p->data_end - p->data_start);
    if (free_space < len)
        return 0;

    /* Reserve the data */
    p->data_start += len;
    p->data_end = p->data_start;

    return len;
}


uint32_t
ws_pktbuf_add_to_front(ws_pktbuf_t *p, uint8_t *data, uint32_t len)
{
    uint32_t free_space;

    ASSERT(p != NULL, "adding to NULL pktbuf\n");

    /* Make sure we can add that much data */
    free_space = (uint32_t)(p->data_start - p->start);
    if (free_space < len)
        return 0;

    /* Add the data */
    p->data_start -= len;
    memcpy(p->data_start, data, len);

    return len;
}


uint32_t
ws_pktbuf_add_to_end(ws_pktbuf_t *p, uint8_t *data, uint32_t len)
{
    uint32_t free_space;

    ASSERT(p != NULL, "adding to NULL pktbuf\n");

    /* Make sure we can add that much data */
    free_space = (uint32_t)(p->end - p->data_end);
    if (free_space < len)
        return 0;

    /* Add the data */
    memcpy(p->data_end, data, len);
    p->data_end += len;

    return len;
}


uint32_t
ws_pktbuf_increment_front(ws_pktbuf_t *p, uint32_t len)
{
    uint32_t free_space;

    ASSERT(p != NULL, "adding to NULL pktbuf\n");

    /* Make sure we can add that much data */
    free_space = (uint32_t)(p->data_start - p->start);
    if (free_space < len)
        return 0;

    /* Increment the data pointer */
    p->data_start -= len;

    return len;
}


uint32_t
ws_pktbuf_increment_end(ws_pktbuf_t *p, uint32_t len)
{
    uint32_t free_space;

    ASSERT(p != NULL, "adding to NULL pktbuf\n");

    /* Make sure we can add that much data */
    free_space = (uint32_t)(p->end - p->data_end);
    if (free_space < len)
        return 0;

    /* Increment the data pointer */
    p->data_end += len;

    return len;
}


uint32_t
ws_pktbuf_remove_from_front(ws_pktbuf_t *p, uint32_t len)
{
    uint32_t used_space;

    ASSERT(p != NULL, "removing from NULL pktbuf\n");

    /* Make sure we can remove that much data */
    used_space = (uint32_t)(p->data_end - p->data_start);
    if (used_space < len)
        return 0;

    /* Remove the data */
    p->data_start += len;

    return len;
}


uint32_t
ws_pktbuf_remove_from_end(ws_pktbuf_t *p, uint32_t len)
{
    uint32_t used_space;

    ASSERT(p != NULL, "removing from NULL pktbuf\n");

    /* Make sure we can remove that much data */
    used_space = (uint32_t)(p->data_end - p->data_start);
    if (used_space < len)
        return 0;

    /* Remove the data */
    p->data_end -= len;

    return len;
}


uint8_t *
ws_pktbuf_get_data(ws_pktbuf_t *p)
{
    ASSERT(p != NULL, "getting data from NULL pktbuf\n");
    return p->data_start;
}


uint32_t
ws_pktbuf_get_len(ws_pktbuf_t *p)
{
    ASSERT(p != NULL, "getting length from NULL pktbuf\n");
    return (uint32_t)(p->data_end - p->data_start);
}


uint32_t
ws_pktbuf_get_free_space_at_front(ws_pktbuf_t *p)
{
    ASSERT(p != NULL, "getting space from NULL pktbuf\n");
    return (uint32_t)(p->data_start - p->start);
}


uint32_t
ws_pktbuf_get_free_space_at_end(ws_pktbuf_t *p)
{
    ASSERT(p != NULL, "getting space from NULL pktbuf\n");
    return (uint32_t)(p->end - p->data_end);
}
