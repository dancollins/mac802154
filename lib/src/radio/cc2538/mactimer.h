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

#ifndef _CC2538_MACTIMER_H
#define _CC2538_MACTIMER_H


#include "wsn.h"


#define CC2538_MACTIMER_SEL_OVF_CTR  (0x00)
#define CC2538_MACTIMER_SEL_OVF_CAP  (0x10)
#define CC2538_MACTIMER_SEL_OVF_PER  (0x20)
#define CC2538_MACTIMER_SEL_OVF_CMP1 (0x30)
#define CC2538_MACTIMER_SEL_OVF_CMP2 (0x40)

#define CC2538_MACTIMER_SEL_CTR  (0x0)
#define CC2538_MACTIMER_SEL_CAP  (0x1)
#define CC2538_MACTIMER_SEL_PER  (0x2)
#define CC2538_MACTIMER_SEL_CMP1 (0x3)
#define CC2538_MACTIMER_SEL_CMP2 (0x4)


#endif /* _CC2538_MACTIMER_H */
