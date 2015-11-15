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

#include "ws_ringbuf.h"


#define min(a, b) a > b ? b : a


void
ws_ringbuf_init(ws_ringbuf_t *rb, uint8_t *buf, uint32_t size)
{
    rb->buffer = buf;
    rb->capacity = size;
    rb->head = 0;
    rb->tail = 0;
    rb->length = 0;
}


uint32_t
ws_ringbuf_write(ws_ringbuf_t *rb, const uint8_t *data, uint32_t len)
{
    uint32_t freespace;
    int i;

    /* Calculate how much space there is in the buffer */
    freespace = rb->capacity - rb->length;

    /* Constrain length to the space in the buffer */
    len = min(len, freespace);

    /* Copy the data into the buffer */
    for (i = 0; i < len; i++)
    {
        rb->buffer[rb->head] = data[i];
        rb->head = (rb->head + 1) % rb->capacity;
    }

    /* Update the buffer length */
    rb->length += len;

    return len;
}


uint32_t
ws_ringbuf_push(ws_ringbuf_t *rb, uint8_t data)
{
    uint32_t freespace;

    /* Make sure there's space */
    if ((rb->capacity - rb->length) == 0)
    {
        return 0;
    }

    /* Copy the data into the buffer */
    rb->buffer[rb->head] = data;
    rb->head = (rb->head + 1) % rb->capacity;

    /* Update the buffer length */
    rb->length++;

    return 1;
}


uint32_t
ws_ringbuf_read(ws_ringbuf_t *rb, uint8_t *data, uint32_t len)
{
    int i;

    /* Can't read from an empty buffer */
    if (!ws_ringbuf_has_data(rb))
    {
        return 0;
    }

    /* Calculate how many bytes we can read */
    len = min(len, rb->length);

    /* Copy data from the buffer */
    for (i = 0; i < len; i++)
    {
        data[i] = rb->buffer[rb->tail];
        rb->tail = (rb->tail + 1) % rb->capacity;
    }

    /* Update the buffer length */
    rb->length -= len;

    return len;
}


uint32_t
ws_ringbuf_pop(ws_ringbuf_t *rb, uint8_t *data)
{
    /* Can't read from an empty buffer */
    if (!ws_ringbuf_has_data(rb))
    {
        return 0;
    }

    /* Copy data from the buffer */
    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % rb->capacity;

    /* Update the buffer length */
    rb->length--;

    return 1;
}


void
ws_ringbuf_flush(ws_ringbuf_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
    rb->length = 0;
    rb->buffer[0] = 0;
}


uint32_t
ws_ringbuf_get_len(ws_ringbuf_t *rb)
{
    return rb->length;
}


uint32_t
ws_ringbuf_get_space(ws_ringbuf_t *rb)
{
    return rb->capacity - rb->length;
}


bool
ws_ringbuf_has_data(ws_ringbuf_t *rb)
{
    if (rb->length)
        return true;

    return false;
}


bool
ws_ringbuf_is_full(ws_ringbuf_t *rb)
{
    return (rb->length == rb->capacity);
}
