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

#ifndef _WS_PKTBUF_H
#define _WS_PKTBUF_H


#include "wsn.h"


extern ws_pktbuf_t *
ws_pktbuf_create(uint32_t len);


extern void
ws_pktbuf_destroy(ws_pktbuf_t *p);


extern void
ws_pktbuf_reset(ws_pktbuf_t *p);


extern uint32_t
ws_pktbuf_reserve(ws_pktbuf_t *p, uint32_t len);


extern uint32_t
ws_pktbuf_add_to_front(ws_pktbuf_t *p, uint8_t *data, uint32_t len);


extern uint32_t
ws_pktbuf_add_to_end(ws_pktbuf_t *p, uint8_t *data, uint32_t len);


extern uint32_t
ws_pktbuf_increment_front(ws_pktbuf_t *p, uint32_t len);


extern uint32_t
ws_pktbuf_increment_end(ws_pktbuf_t *p, uint32_t len);


extern uint32_t
ws_pktbuf_remove_from_front(ws_pktbuf_t *p, uint32_t len);


extern uint32_t
ws_pktbuf_remove_from_end(ws_pktbuf_t *p, uint32_t len);


extern uint8_t *
ws_pktbuf_get_data(ws_pktbuf_t *p);


extern uint32_t
ws_pktbuf_get_len(ws_pktbuf_t *p);


extern uint32_t
ws_pktbuf_get_free_space_at_start(ws_pktbuf_t *p);


extern uint32_t
ws_pktbuf_get_free_space_at_end(ws_pktbuf_t *p);


#endif /* _WS_PKTBUF_H */
