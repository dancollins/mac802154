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

#ifndef _WS_OS_H
#define _WS_OS_H

/* NOTE: This abstraction layer between the WSN library and an operating
 * system is lacking! This is due to a move from a proprietary OS that did
 * not permit linking with an Open Source project. The intention is to
 * add a very simplistic OS based on software timers and also use libc's
 * malloc and printf functions.
 */


#include "wsn.h"


#define PRINTF(...)
#define VPRINTF(fmt, arg)
#define FFLUSH(f)

#define WS_INFO(...) \
    PRINTF("INF %s:%d ", __FILE__, __LINE__);\
    PRINTF(__VA_ARGS__)
#define WS_WARN(...) \
    PRINTF("WRN %s:%d ", __FILE__, __LINE__);\
    PRINTF(__VA_ARGS__)
#define WS_ERROR(...) \
    PRINTF("ERR %s:%d ", __FILE__, __LINE__);\
    PRINTF(__VA_ARGS__)
#define WS_DEBUG(...) \
    PRINTF("DBG %s:%d ", __FILE__, __LINE__);\
    PRINTF(__VA_ARGS__)

#define MALLOC(x) (NULL)
#define FREE(x)

/* TODO: Abstract the BSP stuff into a cleaner interface */
#define ASSERT(cond, ...) \
    do\
    {\
        if (!(cond))\
        {\
            ws_assert(__LINE__, __FILE__, __VA_ARGS__);\
        }\
    }\
    while(0)

#define WS_TIMER_DECLARE(id)
#define WS_TIMER_SET(id, time)
#define WS_TIMER_SET_NOW(id)
#define WS_TIMER_CANCEL(id)

#define WS_TIME_GET_NOW() (0)

#define WS_GET_RANDOM8() (0)

#define ENTER_CRITICAL() ws_enter_critical()
#define EXIT_CRITICAL() ws_exit_critical()


extern void
ws_assert(int line, char *file, char *fmt, ...);


extern void
ws_enter_critical(void);


extern void
ws_exit_critical(void);


#endif /* _WS_OS_H */
