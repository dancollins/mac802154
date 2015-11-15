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

#ifndef _WS_RINGBUF_H
#define _WS_RINGBUF_H

#include "wsn.h"


/**
 * Initialises a ring buffer
 * \param rb the ring buffer structure
 * \param data a block of memory to use
 * \param size the size, in octets, of the memory block
 */
extern void
ws_ringbuf_init(ws_ringbuf_t *rb, uint8_t *buf, uint32_t size);


/**
 * Writes data to a ring buffer
 * \param rb the ring buffer structure
 * \param data the data to write
 * \param len the size, in octets, of the data to write
 * \return the number of octets written
 */
extern uint32_t
ws_ringbuf_write(ws_ringbuf_t *rb, const uint8_t *data, uint32_t len);


/**
 * Put a single byte into the buffer
 * \param rb the ring buffer structure
 * \param data the byte to add
 * \return the number of bytes written (1 or 0)
 */
extern uint32_t
ws_ringbuf_push(ws_ringbuf_t *rb, uint8_t data);


/**
 * Reads data from a ring buffer
 * \param rb the ring buffer structure
 * \param data memory to store the read data
 * \param len the size, in octets, of the data to read
 * \return the number of octets read
 */
extern uint32_t
ws_ringbuf_read(ws_ringbuf_t *rb, uint8_t *data, uint32_t len);


/**
 * Read a single byte from the buffer
 * \param rb the ring buffer structure
 * \param data memory to store the byte
 * \return the number of bytes read (1 or 0)
 */
extern uint32_t
ws_ringbuf_pop(ws_ringbuf_t *rb, uint8_t *data);


/**
 * Drops all data in the buffer
 * \param rb the ring buffer structure
 */
extern void
ws_ringbuf_flush(ws_ringbuf_t *rb);


/**
 * Get the number of octets held in the buffer
 * \param rb the ring buffer structure
 * \return the number of octets held in the buffer
 */
extern uint32_t
ws_ringbuf_get_len(ws_ringbuf_t *rb);


/**
 * Get the number of octets free in the buffer
 * \param rb the ring buffer structure
 * \return the number of octets free in the buffer
 */
extern uint32_t
ws_ringbuf_get_space(ws_ringbuf_t *rb);


/**
 * Test if the buffer contains data
 * \param rb the ring buffer structure
 * \return true if the buffer contains data
 */
extern bool
ws_ringbuf_has_data(ws_ringbuf_t *rb);


/**
 * Test if the ringbuf is full
 * \param rb the ring buffer structure
 * \return true if the ringbuffer is full
 */
extern bool
ws_ringbuf_is_full(ws_ringbuf_t *rb);


#endif /* _WS_RINGBUF_H */
