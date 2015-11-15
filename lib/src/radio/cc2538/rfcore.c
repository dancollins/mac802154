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

#include "rfcore.h"

#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_ints.h"

#include "hw_rfcore_ffsm.h"
#include "hw_rfcore_sfr.h"
#include "hw_rfcore_xreg.h"
#include "hw_ana_regs.h"

#include "sys_ctrl.h"
#include "interrupt.h"

#include "bsp_led.h"


#if 0
#undef WS_DEBUG
#define WS_DEBUG(...)
#endif

#if 0
#undef WS_ERROR
#define WS_ERROR(...)
#endif


struct
{
    ws_radio_rx_callback_t cb;
    bool is_on;
} radio_state;


/* Data gets received into here before being passed to the next higher layer */
static uint8_t rx_buf[128];


/*
 * Command Strobe/CSMA-CA Processor
 */
typedef enum
{
    CSP_OPCODE_ISRXON       = 0xe3,
    CSP_OPCODE_ISTXON       = 0xe9,
    CSP_OPCODE_ISTXONCCA    = 0xea,
    CSP_OPCODE_ISSAMPLECCA  = 0xeb,
    CSP_OPCODE_ISRXOFF      = 0xef,
    CSP_OPCODE_ISFLUSHRX    = 0xed,
    CSP_OPCODE_ISFLUSHTX    = 0xee,
} csp_opcode_t;

static inline void
csp_run_instruction(csp_opcode_t inst)
{
    HWREG(RFCORE_SFR_RFST) = inst;
}


/*
 * Interrupts
 */
static void
rf_isr(void)
{
    int i;
    uint8_t *buf, len;
    uint32_t flags;

    flags = HWREG(RFCORE_SFR_RFIRQF0);

    if (flags & RFCORE_SFR_RFIRQF0_FIFOP)
    {
        len = HWREG(RFCORE_XREG_RXFIFOCNT);

        /* Pull the data from the radio */
        for (i = 0; i < len; i++)
        {
            rx_buf[i] = (uint8_t)HWREG(RFCORE_SFR_RFDATA);
        }

        /* Pass the data up the stack */
        if (radio_state.cb != NULL)
        {
            radio_state.cb(rx_buf, len);
        }

        /* Clear the flag */
        HWREG(RFCORE_SFR_RFIRQF0) &= ~RFCORE_SFR_RFIRQF0_FIFOP;
    }
}


static void
rf_err_isr(void)
{
    uint32_t flags;

    flags = HWREG(RFCORE_SFR_RFERRF);

    if (flags & RFCORE_SFR_RFERRF_STROBEERR ||
        flags & RFCORE_SFR_RFERRF_RXUNDERF ||
        flags & RFCORE_SFR_RFERRF_NLOCK)
    {
        ASSERT(false, "Serious RF error (%08lx)\n", flags);
    }
    else if (flags & RFCORE_SFR_RFERRF_RXOVERF ||
             flags & RFCORE_SFR_RFERRF_RXABO)
    {
        ASSERT(false, "RX overflow or abort (%08lx)\n", flags);
    }
    else if (flags & RFCORE_SFR_RFERRF_TXOVERF)
    {
        ASSERT(false, "TX buffer overloaded (%08lx)\n", flags);
    }
    else if (flags & RFCORE_SFR_RFERRF_TXUNDERF)
    {
        csp_run_instruction(CSP_OPCODE_ISFLUSHTX);
    }
    else
    {
        ASSERT(false, "Unhandled RF error (%08lx)\n", flags);
    }
}


/*
 * Public API
 */
void
ws_radio_init(void)
{
    WS_DEBUG("init\n");

    /* Reset state */
    memset(&radio_state, 0, sizeof(radio_state));

    /* Enable the clock */
    /* TODO: Power saving. We're basically leaving the radio on all the time */
    SysCtrlPeripheralReset(SYS_CTRL_PERIPH_RFC);
    SysCtrlPeripheralEnable(SYS_CTRL_PERIPH_RFC);
    SysCtrlPeripheralSleepEnable(SYS_CTRL_PERIPH_RFC);
    SysCtrlPeripheralDeepSleepEnable(SYS_CTRL_PERIPH_RFC);

    /* Configure CCA */
    HWREG(RFCORE_XREG_CCACTRL0) = CC2538_RFCORE_CCA_THRES;

    /* User Manual 23.15 - TX and RX settings */
    HWREG(RFCORE_XREG_AGCCTRL1) = CC2538_RFCORE_AGC_TARGET;
    HWREG(RFCORE_XREG_TXFILTCFG) = CC2538_RFCORE_TX_AA_FILTER;
    HWREG(ANA_REGS_BASE + ANA_REGS_O_IVCTRL) = CC2538_RFCORE_BIAS_CURRENT;
    HWREG(RFCORE_XREG_FSCAL1) = CC2538_RFCORE_FREQ_CAL;

    /* Enable auto CRC calculation (hardware) */
    HWREG(RFCORE_XREG_FRMCTRL0) = RFCORE_XREG_FRMCTRL0_AUTOCRC;

    /* Enable auto ACK */
    HWREG(RFCORE_XREG_FRMCTRL0) |= RFCORE_XREG_FRMCTRL0_AUTOACK;

    /* Configure the FIFOP signal to trigger when there are one or more
     * complete frames in the RX FIFO */
    HWREG(RFCORE_XREG_FIFOPCTRL) = WS_RADIO_MAX_PACKET_LEN;

    /* Set the TX output power */
    HWREG(RFCORE_XREG_TXPOWER) = CC2538_RFCORE_TX_POWER;

    /* Interrupt wth FIFOP signal */
    HWREG(RFCORE_XREG_RFIRQM0) = 0x04;
    IntRegister(INT_RFCORERTX, &rf_isr);
    IntEnable(INT_RFCORERTX);

    /* Interrupt with all RF ERROR signals */
    HWREG(RFCORE_XREG_RFERRM) = 0x7f;
    IntRegister(INT_RFCOREERR, &rf_err_isr);
    IntEnable(INT_RFCOREERR);

    /* Configure the frame filtering */
    HWREG(RFCORE_XREG_FRMFILT0) &= ~RFCORE_XREG_FRMFILT0_MAX_FRAME_VERSION_M;
    HWREG(RFCORE_XREG_FRMFILT0) |= WS_MAC_MAX_FRAME_VERSION <<
        RFCORE_XREG_FRMFILT0_MAX_FRAME_VERSION_S;
    /* Don't bother filtering out the source addresses */
    HWREG(RFCORE_XREG_SRCMATCH) &= ~RFCORE_XREG_SRCMATCH_SRC_MATCH_EN;
}


void
ws_radio_set_channel(uint8_t channel)
{
    ASSERT(channel >= WS_RADIO_MIN_CHANNEL &&
           channel <= WS_RADIO_MAX_CHANNEL,
           "Invalid radio channel %u\n", channel);

    HWREG(RFCORE_XREG_FREQCTRL) = channel;

    csp_run_instruction(CSP_OPCODE_ISRXOFF);
    csp_run_instruction(CSP_OPCODE_ISFLUSHTX);
    csp_run_instruction(CSP_OPCODE_ISFLUSHRX);
    csp_run_instruction(CSP_OPCODE_ISRXON);
}


void
ws_radio_set_power(bool on)
{
    if (on)
    {
        csp_run_instruction(CSP_OPCODE_ISRXON);
    }
    else
    {
        csp_run_instruction(CSP_OPCODE_ISRXOFF);
    }

    radio_state.is_on = on;
}


void
ws_radio_set_rx_callback(ws_radio_rx_callback_t cb)
{
    radio_state.cb = cb;
}


bool
ws_radio_cca(void)
{
#if 0
    if (!radio_state.is_on)
        return false;
#else
    ASSERT(radio_state.is_on, "Radio is turned off, so can't perform CCA\n");
#endif

    csp_run_instruction(CSP_OPCODE_ISSAMPLECCA);

    if (HWREG(RFCORE_XREG_FSMSTAT1) & RFCORE_XREG_FSMSTAT1_SAMPLED_CCA)
        return true;

    return false;
}


void
ws_radio_prepare(ws_pktbuf_t *pkt)
{
    int i;
    uint32_t len = ws_pktbuf_get_len(pkt);
    uint8_t *data = ws_pktbuf_get_data(pkt);

    ASSERT(len <= WS_RADIO_MAX_PACKET_LEN, "invalid packet size: %u\n", len);

    /* Copy the PHY len field */
    HWREG(RFCORE_SFR_RFDATA) = len + WS_RADIO_CHECKSUM_LEN;

    /* Copy the data into the buffer */
    for (i = 0; i < len; i++)
    {
        HWREG(RFCORE_SFR_RFDATA) = data[i];
    }
}


void
ws_radio_transmit(void)
{
    ASSERT(radio_state.is_on, "Radio is not powered!\n");

    /* There's no data to be sent */
    if (HWREG(RFCORE_XREG_TXFIFOCNT) == 0)
        return;

    /* Turn on the radio and wait for the data to be sent */
    csp_run_instruction(CSP_OPCODE_ISTXON);

    /* TODO: If the radio doesn't turn on we'll get stuck here! */
    while (!(HWREG(RFCORE_XREG_FSMSTAT1) & RFCORE_XREG_FSMSTAT1_TX_ACTIVE))
        ;
    while (HWREG(RFCORE_XREG_FSMSTAT1) & RFCORE_XREG_FSMSTAT1_TX_ACTIVE)
        ;

    /* Higher layer will handle re-transmissions */
    csp_run_instruction(CSP_OPCODE_ISFLUSHTX);

    /* Return to RX mode */
    csp_run_instruction(CSP_OPCODE_ISRXON);
}


bool
ws_radio_tx_has_data(void)
{
    uint32_t fifo_len = HWREG(RFCORE_XREG_TXFIFOCNT);
    return fifo_len > 0;
}

void
ws_radio_tx_clear(void)
{
    csp_run_instruction(CSP_OPCODE_ISFLUSHTX);
}


void
ws_radio_enter_critical(void)
{
    IntDisable(INT_RFCORERTX);
}


void
ws_radio_exit_critical(void)
{
    IntEnable(INT_RFCORERTX);
}


void
ws_radio_set_pan_id(uint16_t pan_id)
{
    HWREG(RFCORE_FFSM_PAN_ID1) = (pan_id >> 8) & 0xff;
    HWREG(RFCORE_FFSM_PAN_ID0) = pan_id & 0xff;
}


void
ws_radio_set_short_address(uint16_t short_addr)
{
    HWREG(RFCORE_FFSM_SHORT_ADDR1) = (short_addr >> 8) & 0xff;
    HWREG(RFCORE_FFSM_SHORT_ADDR0) = short_addr & 0xff;
}


void
ws_radio_set_extended_address(uint8_t *extended_addr)
{
    HWREG(RFCORE_FFSM_EXT_ADDR7) = extended_addr[7] & 0xff;
    HWREG(RFCORE_FFSM_EXT_ADDR6) = extended_addr[6] & 0xff;
    HWREG(RFCORE_FFSM_EXT_ADDR5) = extended_addr[5] & 0xff;
    HWREG(RFCORE_FFSM_EXT_ADDR4) = extended_addr[4] & 0xff;
    HWREG(RFCORE_FFSM_EXT_ADDR3) = extended_addr[3] & 0xff;
    HWREG(RFCORE_FFSM_EXT_ADDR2) = extended_addr[2] & 0xff;
    HWREG(RFCORE_FFSM_EXT_ADDR1) = extended_addr[1] & 0xff;
    HWREG(RFCORE_FFSM_EXT_ADDR0) = extended_addr[0] & 0xff;
}
