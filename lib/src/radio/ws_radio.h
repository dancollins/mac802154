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

#ifndef _WS_RADIO_H
#define _WS_RADIO_H


#include "wsn.h"


/**
 * Maximum packet length
 */
#define WS_RADIO_MAX_PACKET_LEN (127)

/**
 * Octets used by checksum
 */
#define WS_RADIO_CHECKSUM_LEN (2)

/**
 * IEEE 802.15.4 channel definitions
 */
#define WS_RADIO_MIN_CHANNEL (11)
#define WS_RADIO_MAX_CHANNEL (26)
#define WS_RADIO_CHANNEL_SPACING (5)

/**
 * Symbols per slot - see IEEE 802.15.4-2011 section 6.4.2
 */
#define WS_RADIO_SLOT_DURATION (60)


/**
 * Used to pass data received by the radio to the packet scheduler
 * \param data the memory containing the data. This is freed when the
 *             callback returns.
 * \param len number of octets in the memory buffer
 */
typedef void (*ws_radio_rx_callback_t)(const uint8_t *data, uint8_t len);


/**
 * Used to inform the packet scheduler whenever the slot timer interrupts.
 */
typedef void (*ws_radio_timer_callback_t)(void);


/**
 * Prepare the radio for use. This must be called before any other
 * radio function
 */
extern void
ws_radio_init(void);


/**
 * Set the IEEE 802.15.4 channel
 * \param channel a number between 11 and 26 specifying the new channel
 */
extern void
ws_radio_set_channel(uint8_t channel);


/**
 * Set a callback to receive data from the radio
 * \param cb the function to call when data is received
 */
extern void
ws_radio_set_rx_callback(ws_radio_rx_callback_t cb);


/**
 * Used to turn the radio on and off. When the radio is off, it should draw
 * as little power as possible.
 * \param on true to turn the radio on
 */
extern void
ws_radio_set_power(bool on);


/**
 * Perform a clear channel assessment. If the radio is off, this will always
 * return false to indicate the channel is not clear.
 * \return true if the channel is clear
 */
extern bool
ws_radio_cca(void);


/**
 * Copy data into the TX FIFO to be sent when \see ws_radio_transmit is called
 * \param pkt pktbuf containing data to be sent
 */
extern void
ws_radio_prepare(ws_pktbuf_t *pkt);


/**
 * Send the data in the TX FIFO
 */
extern void
ws_radio_transmit(void);


/**
 * Query the state of the radio TX buffer
 * \return true if there is data in the TX buffer
 */
extern bool
ws_radio_tx_has_data(void);


/**
 * Clear the TX FIFO
 */
extern void
ws_radio_tx_clear(void);


/**
 * Disable any interrupts that could cause the receive handler to be called.
 */
extern void
ws_radio_enter_critical(void);


/**
 * Enable any interrupts that were disabled during the critical section
 */
extern void
ws_radio_exit_critical(void);


/**
 * Inform the radio of the current PAN ID. This allows the radio to filter
 * out packets in hardware or in software.
 * \param pan_id the new PAN ID
 */
extern void
ws_radio_set_pan_id(uint16_t pan_id);


/**
 * Inform the radio of the current short address. This allows the radio to
 * filter out packets in hardware or in software.
 * \param short_addr the new short address
 */
extern void
ws_radio_set_short_address(uint16_t short_addr);


/**
 * Inform the radio of the current extended address. This allows the radio to
 * filter out packets in hardware or in software.
 * \param extended_addr the new extended address
 */
extern void
ws_radio_set_extended_address(uint8_t *extended_addr);

/*
 * Radio Timer
 */
/**
 * Prepare a timer used to generate interrupts every slot.
 * \param cb Function to call at each slot interrupt. This is called from
 *           an interrupt context.
 */
extern void
ws_radio_timer_init(ws_radio_timer_callback_t cb);


/**
 * Used to synchronise with the coordinator. Sets the slot timer interrupt
 * to occur at the next slot
 */
extern void
ws_radio_timer_syncronise(void);


/**
 * Reconfigure the slot interrupt timing
 */
extern void
ws_radio_timer_set_superframe_order(uint8_t superframe_order);


/**
 * Enable interrupts at each slot
 */
extern void
ws_radio_timer_enable_interrupts(void);


/**
 * Disable interrupts at each slot
 */
extern void
ws_radio_timer_disable_interrupts(void);


/**
 * Get the current time, in symbols, of the radio timer
 * \return the current time measured in symbols
 */
extern uint32_t
ws_radio_timer_get_time();


#endif /* _WS_RADIO_H */
