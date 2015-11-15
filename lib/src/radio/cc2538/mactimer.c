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

#include "mactimer.h"

#include "hw_rfcore_sfr.h"
#include "hw_ints.h"

#include "interrupt.h"


typedef struct
{
    ws_radio_timer_callback_t timer_cb;
    uint8_t superframe_order;
} mactimer_t;

static mactimer_t mactimer;


static void
mactimer_handler(void)
{
    uint32_t flags = HWREG(RFCORE_SFR_MTIRQF);

    if (flags & RFCORE_SFR_MTIRQF_MACTIMER_OVF_COMPARE1F)
    {
        if (mactimer.timer_cb != NULL)
            mactimer.timer_cb();

        ws_radio_timer_syncronise();

        /* Clear the flag */
        HWREG(RFCORE_SFR_MTIRQF) &= ~RFCORE_SFR_MTIRQF_MACTIMER_OVF_COMPARE1F;
    }
}


void
ws_radio_timer_init(ws_radio_timer_callback_t cb)
{
    mactimer.timer_cb = cb;
    mactimer.superframe_order = 15;

    /* Reset the MAC timer */
    HWREG(RFCORE_SFR_MTCTRL) &= ~RFCORE_SFR_MTCTRL_RUN;
    HWREG(RFCORE_SFR_MTMSEL) = CC2538_MACTIMER_SEL_OVF_CTR |
        CC2538_MACTIMER_SEL_CTR;
    HWREG(RFCORE_SFR_MTM0) = 0;
    HWREG(RFCORE_SFR_MTM1) = 0;
    HWREG(RFCORE_SFR_MTMOVF0) = 0;
    HWREG(RFCORE_SFR_MTMOVF1) = 0;
    HWREG(RFCORE_SFR_MTMOVF2) = 0;

    /* Start the timer asynchronously */
    HWREG(RFCORE_SFR_MTCTRL) &= ~RFCORE_SFR_MTCTRL_SYNC;

    /* Configure the MAC timer to overflow every symbol
     * Symbol rate        = 62.5 ksym/s
     * Symbol period      = 16us
     * Timer resolution   = 31ns
     * Ticks required     = 16us / 31ns
     *                    = 516.13
     *                    = 516
     *                   => 15.996us
     */
    HWREG(RFCORE_SFR_MTMSEL) = CC2538_MACTIMER_SEL_PER;
    /* 516 ticks => load with 515 (0x203) */
    HWREG(RFCORE_SFR_MTM0) = 0x03;
    HWREG(RFCORE_SFR_MTM1) = 0x02;

    /* Interrupt on overflow counter compare 1 */
    HWREG(RFCORE_SFR_MTIRQM) |= RFCORE_SFR_MTIRQM_MACTIMER_OVF_COMPARE1M;

    IntRegister(INT_MACTIMR, &mactimer_handler);

    ws_radio_timer_syncronise();

    HWREG(RFCORE_SFR_MTCTRL) |= RFCORE_SFR_MTCTRL_RUN;
}


void
ws_radio_timer_syncronise(void)
{
    uint32_t time;

    HWREG(RFCORE_SFR_MTMSEL) = CC2538_MACTIMER_SEL_OVF_CTR;
    time = HWREG(RFCORE_SFR_MTMOVF0);
    time |= HWREG(RFCORE_SFR_MTMOVF1) << 8;
    time |= HWREG(RFCORE_SFR_MTMOVF2) << 16;

    time += (WS_RADIO_SLOT_DURATION << mactimer.superframe_order);

    HWREG(RFCORE_SFR_MTMSEL) = CC2538_MACTIMER_SEL_OVF_CMP1;
    HWREG(RFCORE_SFR_MTMOVF0) = time & 0xff;
    HWREG(RFCORE_SFR_MTMOVF1) = (time >> 8) & 0xff;
    HWREG(RFCORE_SFR_MTMOVF2) = (time >> 16) & 0xff;
}


void
ws_radio_timer_set_superframe_order(uint8_t superframe_order)
{
    mactimer.superframe_order = superframe_order;
}


void
ws_radio_timer_enable_interrupts(void)
{
    WS_DEBUG("enable interrupts\n");
    IntEnable(INT_MACTIMR);
}


void
ws_radio_timer_disable_interrupts(void)
{
    IntDisable(INT_MACTIMR);
}


uint32_t
ws_radio_timer_get_time()
{
    uint32_t time;

    HWREG(RFCORE_SFR_MTMSEL) = CC2538_MACTIMER_SEL_OVF_CTR;
    time = HWREG(RFCORE_SFR_MTMOVF0);
    time |= HWREG(RFCORE_SFR_MTMOVF1) << 8;
    time |= HWREG(RFCORE_SFR_MTMOVF2) << 16;

    return time;
}

