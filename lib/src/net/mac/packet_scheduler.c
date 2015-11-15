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

#include "mac_private.h"

#include "bsp_led.h"

#include "hw_memmap.h"
#include "gpio.h"

#if 1
#undef WS_DEBUG
#define WS_DEBUG(...)
#endif

/*#define GPIO_DEBUG 1*/


/* Debug GPIO pins */
#define DEBUG_GPIO_HANDLER_PORT (GPIO_B_BASE)
#define DEBUG_GPIO_HANDLER_RX (GPIO_PIN_4)
#define DEBUG_GPIO_HANDLER_TIMER (GPIO_PIN_5)
#define DEBUG_GPIO_OTHER_PORT (GPIO_A_BASE)
#define DEBUG_GPIO_OTHER_CSMA (GPIO_PIN_2)
#define DEBUG_GPIO_OTHER_TX_IN_FLIGHT (GPIO_PIN_4)
#define DEBUG_GPIO_OTHER_SYNC (GPIO_PIN_5)


/**
 * Transmitter state machine.
 * IDLE:    transmitter is waiting for packets to send
 * SENDING: transmitter has copied outgoing packet into radio FIFO, and
 *          is waiting for the packet to be transmitted
 * SENT:    packet was transmitted (CSMA-CA succeeded)
 */
typedef enum
{
    PACKET_SCHEDULER_TX_STATE_IDLE,
    PACKET_SCHEDULER_TX_STATE_SENDING,
    PACKET_SCHEDULER_TX_STATE_SENT,
} packet_scheduler_tx_state_t;


/**
 * Wrapper to allow us to store packets in a list
 */
typedef struct
{
    ws_list_t list;
    ws_pktbuf_t *pkt;
} packet_scheduler_queue_t;


/**
 * Packet scheduler state
 */
#define INCOMING_RB_LEN (1024)
typedef struct
{
    /* Receiver */
    ws_ringbuf_t rx_data;
    uint8_t rx_data_buf[INCOMING_RB_LEN];
    bool rx_data_dropped;

    /* Transmitter */
    packet_scheduler_tx_state_t tx_state;
    ws_list_t tx_data;
    ws_pktbuf_t *tx_in_flight;
    uint32_t tx_in_flight_timestamp;
    uint8_t tx_in_flight_retries;

    uint16_t slot_count;
    bool csma_active;
} packet_scheduler_state_t;

static packet_scheduler_state_t ps_state;


/**
 * Timers
 */
WS_TIMER_DECLARE(packet_scheduler_timer);
WS_TIMER_DECLARE(csma_timer);


/* -----------------------------------------------------------------------
 *  Interrupt Handlers
 * -----------------------------------------------------------------------
 */
static void
handle_radio_rx_interrupt(const uint8_t *data, uint8_t len)
{
#ifdef GPIO_DEBUG
    GPIOPinWrite(DEBUG_GPIO_HANDLER_PORT, DEBUG_GPIO_HANDLER_RX,
                 DEBUG_GPIO_HANDLER_RX);
#endif

    /* Try to save the received data */
    uint32_t max_len = ws_ringbuf_get_space(&ps_state.rx_data);
    if (len > max_len)
    {
        ps_state.rx_data_dropped = true;
    }
    else
    {
        ws_ringbuf_write(&ps_state.rx_data, data, (uint32_t)len);
    }

    WS_TIMER_SET_NOW(packet_scheduler_timer);

#ifdef GPIO_DEBUG
    GPIOPinWrite(DEBUG_GPIO_HANDLER_PORT, DEBUG_GPIO_HANDLER_RX, 0);
#endif
}


static void
handle_radio_timer_interrupt(void)
{
    ws_pktbuf_t *pkt;

#ifdef GPIO_DEBUG
    GPIOPinWrite(DEBUG_GPIO_HANDLER_PORT, DEBUG_GPIO_HANDLER_TIMER,
                 DEBUG_GPIO_HANDLER_TIMER);
#endif

    /* TODO:
     * This does not allow for GTS, or inactive periods within the superframe.
     * Currently, all 15 slots are CAP. This should be improved.
     */

    if (mac.state == MAC_STATE_COORDINATING)
    {
        if (ps_state.slot_count >= (1 << mac.beacon_order))
        {
            ps_state.slot_count = 0;

#ifdef GPIO_DEBUG
            GPIOPinWrite(DEBUG_GPIO_OTHER_PORT, DEBUG_GPIO_OTHER_SYNC,
                         DEBUG_GPIO_OTHER_SYNC);
#endif

            /* If there's data left in the transmitter, then we want to
             * dump it out to send a beacon. The packet scheduler will
             * manage a retransmission. */
            if (ws_radio_tx_has_data())
            {
                ws_radio_tx_clear();
            }

            pkt = mac_coordinator_request_beacon();
            ws_radio_prepare(pkt);
            ws_radio_transmit();
        }
        else
        {
            ps_state.slot_count++;

#ifdef GPIO_DEBUG
            if (ps_state.slot_count >= 16)
            {
                GPIOPinWrite(DEBUG_GPIO_OTHER_PORT, DEBUG_GPIO_OTHER_SYNC, 0);
            }
#endif

            if (ps_state.slot_count < 15 &&
                ws_radio_tx_has_data() && !ps_state.csma_active)
                WS_TIMER_SET_NOW(csma_timer);
        }
    }
    else
    {
#ifdef GPIO_DEBUG
        if (ps_state.slot_count >= 16)
        {
            GPIOPinWrite(DEBUG_GPIO_OTHER_PORT, DEBUG_GPIO_OTHER_SYNC,
                         0);
        }
#endif

        if (ps_state.slot_count < 15 && ps_state.slot_count > 0)
        {
            if (ws_radio_tx_has_data() && !ps_state.csma_active)
                WS_TIMER_SET_NOW(csma_timer);
        }

        /* TODO: We should detect syncronisation loss here. */
        ps_state.slot_count++;

        if (ps_state.slot_count > 100)
            ps_state.slot_count = 100;
    }

#ifdef GPIO_DEBUG
    GPIOPinWrite(DEBUG_GPIO_HANDLER_PORT, DEBUG_GPIO_HANDLER_TIMER, 0);
#endif
}


/* -----------------------------------------------------------------------
 *  Dispatching packets / status
 * -----------------------------------------------------------------------
 */
static void
dispatch_packet(ws_pktbuf_t *pkt)
{
    mac_fcf_t *fcf = (mac_fcf_t *)ws_pktbuf_get_data(pkt);

    switch (fcf->frame_type)
    {
    case MAC_FRAME_TYPE_BEACON:
        mac_coordinator_handle_packet(pkt);
        break;

    case MAC_FRAME_TYPE_DATA:
        mac_mcps_handle_packet(pkt);
        break;

    case MAC_FRAME_TYPE_MAC:
        switch (mac.state)
        {
        case MAC_STATE_ASSOCIATING:
            mac_mlme_association_handle_packet(pkt);
            break;

        case MAC_STATE_COORDINATING:
            mac_coordinator_handle_packet(pkt);
            break;

        default:
            mac_mlme_handle_packet(pkt);
            break;
        }
        break;

    default:
        WS_WARN("unhandled frame (type=%u) in dispatcher\n", fcf->frame_type);
        break;
    }
}


static void
dispatch_status(ws_pktbuf_t *pkt, mac_tx_status_t status)
{
    mac_fcf_t *fcf = (mac_fcf_t *)ws_pktbuf_get_data(pkt);

    switch (fcf->frame_type)
    {
    case MAC_FRAME_TYPE_BEACON:
        mac_coordinator_handle_status(fcf->data[0], status);
        break;

    case MAC_FRAME_TYPE_DATA:
        mac_mcps_handle_status(fcf->data[0], status);
        break;

    case MAC_FRAME_TYPE_MAC:
        switch (mac.state)
        {
        case MAC_STATE_ASSOCIATING:
            mac_mlme_association_handle_status(fcf->data[0], status);
            break;

        case MAC_STATE_COORDINATING:
            mac_coordinator_handle_status(fcf->data[0], status);
            break;

        default:
            mac_mlme_handle_status(fcf->data[0], status);
            break;
        }
        break;

    default:
        WS_WARN("unhandled frame (type=%u,sqn=%u) in dispatcher\n",
                fcf->frame_type, fcf->data[0]);
        break;
    }
}


static void
clean_tx_state(void)
{
    WS_DEBUG("cleaning tx state\n");

    if (ps_state.tx_in_flight != NULL)
    {
        WS_DEBUG("dest\n");
        ws_pktbuf_destroy(ps_state.tx_in_flight);
        ps_state.tx_in_flight = NULL;
    }

    ps_state.tx_state = PACKET_SCHEDULER_TX_STATE_IDLE;
    ws_radio_tx_clear();
}


/* -----------------------------------------------------------------------
 *  Background Task
 * -----------------------------------------------------------------------
 */
static void
packet_scheduler_timer(void)
{
    ws_pktbuf_t *pkt;
    uint8_t *buf;
    uint8_t phy_len;
    mac_fcf_t *fcf, *in_flight_fcf;
    uint32_t delta, time;

    /*
     * Incoming Data
     */
    if (ps_state.rx_data_dropped)
    {
        /* TODO: Keep track of this / do something with this warning */
        WS_WARN("received frame has been dropped\n");
        ps_state.rx_data_dropped = false;
    }

    while (ws_ringbuf_has_data(&ps_state.rx_data))
    {
        /* Load a frame from the ring buffer into a pktbuf */
        pkt = ws_pktbuf_create(WS_RADIO_MAX_PACKET_LEN);
        if (pkt == NULL)
        {
            WS_ERROR("failed to allocate memory for received data\n");
            break;
        }

        WS_DEBUG("created packet (pkt=%p)\n", pkt);

        buf = ws_pktbuf_get_data(pkt);
        ws_ringbuf_pop(&ps_state.rx_data, &phy_len);
        ws_ringbuf_read(&ps_state.rx_data, buf, phy_len);
        ws_pktbuf_increment_end(pkt, phy_len);

        /* Remove the FCS from the end */
        ws_pktbuf_remove_from_end(pkt, 2);

        WS_DEBUG("received a packet (len=%u, state=%u)\n",
                 phy_len, mac.state);

        fcf = (mac_fcf_t *)ws_pktbuf_get_data(pkt);

        /* Packets will get passed to the appropriate upper MAC layer based
         * on a number of conditions:
         *
         * ACK packets get compared to the current in_flight packet.
         * If they match, we can clean up the in_flight state, and alert the
         * layer above.
         *
         * mac.state = IDLE
         *  ignore everything
         * mac.state = SCANNING
         *  beacon frames go to mlme_scan and everything else is ignored
         * mac.state = ASSOCIATING
         *  data is ignored, everything else falls to standard case
         *
         * fcf.type = BEACON => coordinator
         * fcf.type = DATA   => MCPS
         * fcf.type = MAC    => MLME
         */
        if (fcf->frame_type == MAC_FRAME_TYPE_ACK)
        {
            if (ps_state.tx_in_flight != NULL)
            {
                in_flight_fcf =
                    (mac_fcf_t *)ws_pktbuf_get_data(ps_state.tx_in_flight);
                if (fcf->data[0] == in_flight_fcf->data[0])
                {
                    WS_DEBUG("received ACK for (sqn=%u)\n", fcf->data[0]);

                    dispatch_status(ps_state.tx_in_flight,
                                    MAC_TX_STATUS_SUCCESS);

                    clean_tx_state();
                }
                else
                {
                    WS_DEBUG("ignoring ACK (sqn=%u) frame expected (%u)\n",
                             fcf->data[0], in_flight_fcf->data[0]);
                }
            }
            else
            {
                WS_DEBUG("ignoring ACK (sqn=%u) frame\n", fcf->data[0]);
            }

            WS_DEBUG("dest\n");
            ws_pktbuf_destroy(pkt);
        }
        else
        {
            switch (mac.state)
            {
            case MAC_STATE_IDLE:
                WS_DEBUG("ignoring (type=%u) in IDLE state\n",
                         fcf->frame_type);
                break;

            case MAC_STATE_SCANNING:
                if (fcf->frame_type == MAC_FRAME_TYPE_BEACON)
                {
                    mac_mlme_scan_handle_packet(pkt);
                }
                else
                {
                    WS_DEBUG("ignoring (type=%u) in SCANNING state\n",
                             fcf->frame_type);
                }
                break;

            case MAC_STATE_ASSOCIATING:
                if (fcf->frame_type == MAC_FRAME_TYPE_BEACON ||
                    fcf->frame_type == MAC_FRAME_TYPE_MAC)
                {
                    mac_mlme_association_handle_packet(pkt);
                }
                else
                {
                    WS_DEBUG("ignoring (type=%u) in ASSOCIATING state\n",
                             fcf->frame_type);
                }
                break;

            default:
                dispatch_packet(pkt);
                break;
            }
        }
    }

    /*
     * Outgoing Data
     */
    switch (ps_state.tx_state)
    {
    case PACKET_SCHEDULER_TX_STATE_IDLE:
    {
        packet_scheduler_queue_t *data;

        if (!ws_list_is_empty(&ps_state.tx_data))
        {
            ps_state.tx_state = PACKET_SCHEDULER_TX_STATE_SENDING;

            ASSERT(ps_state.tx_in_flight == NULL,
                   "have data in_flight in IDLE state!\n");

            /* Pull the next pktbuf out of the list */
            data = ws_list_get_data(ps_state.tx_data.next,
                                   packet_scheduler_queue_t, list);
            ws_list_remove(&data->list);
            pkt = data->pkt;
            FREE(data);

            fcf = (mac_fcf_t *)ws_pktbuf_get_data(pkt);

            WS_DEBUG("adding frame (type=%u) to TX FIFO\n", fcf->frame_type);
            WS_DEBUG("frame: %r\n",
                     ws_pktbuf_get_data(pkt),
                     ws_pktbuf_get_len(pkt));

            /* Copy the packet to the RF FIFO */
            ws_radio_prepare(pkt);

#ifdef GPIO_DEBUG
            GPIOPinWrite(DEBUG_GPIO_OTHER_PORT,
                         DEBUG_GPIO_OTHER_TX_IN_FLIGHT,
                         DEBUG_GPIO_OTHER_TX_IN_FLIGHT);
#endif

            if (fcf->ack_req)
            {
                /* If acknowledgement is requested, we'll prepare the in flight
                 * state */
                ps_state.tx_in_flight = pkt;
                ps_state.tx_in_flight_timestamp = ws_radio_timer_get_time();
                ps_state.tx_in_flight_retries = 0;

                WS_DEBUG("acknowledgement requested\n");
            }
            else
            {
                /* Otherwise we'll tidy up the packet */
                WS_DEBUG("dest\n");
                ws_pktbuf_destroy(pkt);
                pkt = NULL;

                WS_DEBUG("acknowledgement not requested\n");
            }
        }
        break;
    }

    case PACKET_SCHEDULER_TX_STATE_SENDING:
        /* TODO: This should be a function of the current beacon interval.
         * This is roughly calculated for a BO of 5 */
        time = ps_state.tx_in_flight_timestamp + 4000;
        time &= 0xffffff;
        delta = ws_radio_timer_get_time() - time;

        if (delta < 0x800000)
        {
            /* If the packet has not left the radio in a reasonable amount of
             * time then we alert the next higher layer with the failure. */
            WS_ERROR("failed to transmit packet (%u/%u)\n",
                     ps_state.tx_in_flight_retries, mac.max_frame_retries);
            if (ps_state.tx_in_flight_retries < mac.max_frame_retries)
            {
                ws_radio_prepare(ps_state.tx_in_flight);
                ps_state.tx_in_flight_timestamp = ws_radio_timer_get_time();
                ps_state.tx_in_flight_retries++;
                ps_state.tx_state = PACKET_SCHEDULER_TX_STATE_SENDING;

#ifdef GPIO_DEBUG
                GPIOPinWrite(DEBUG_GPIO_OTHER_PORT,
                             DEBUG_GPIO_OTHER_TX_IN_FLIGHT,
                             DEBUG_GPIO_OTHER_TX_IN_FLIGHT);
#endif
            }
            else
            {
#ifdef GPIO_DEBUG
                GPIOPinWrite(DEBUG_GPIO_OTHER_PORT,
                         DEBUG_GPIO_OTHER_TX_IN_FLIGHT, 0);
#endif

                dispatch_status(ps_state.tx_in_flight,
                                MAC_TX_STATUS_NOT_SENT);

                clean_tx_state();
            }
        }
        break;

    case PACKET_SCHEDULER_TX_STATE_SENT:
        /* TODO: This should be a function of the SIFS PIB attribute.
         * This is roughly a slot period, which is too long. */
        time = ps_state.tx_in_flight_timestamp + 60;
        time &= 0xffffff;
        delta = ws_radio_timer_get_time() - time;

        if (delta < 0x800000)
        {
            /* If the ACK message was not received in a reasonable amount of
             * time then we should schedule a retransmission */
            WS_ERROR("No ACK received (%u/%u)\n",
                     ps_state.tx_in_flight_retries, mac.max_frame_retries);
            if (ps_state.tx_in_flight_retries < mac.max_frame_retries)
            {
                ws_radio_prepare(ps_state.tx_in_flight);
                ps_state.tx_in_flight_timestamp = ws_radio_timer_get_time();
                ps_state.tx_in_flight_retries++;
                ps_state.tx_state = PACKET_SCHEDULER_TX_STATE_SENDING;

#ifdef GPIO_DEBUG
                GPIOPinWrite(DEBUG_GPIO_OTHER_PORT,
                             DEBUG_GPIO_OTHER_TX_IN_FLIGHT,
                             DEBUG_GPIO_OTHER_TX_IN_FLIGHT);
#endif
            }
            else
            {
                WS_ERROR("failed to send within (max_retry=%u) retries\n",
                         mac.max_frame_retries);

                dispatch_status(ps_state.tx_in_flight,
                                MAC_TX_STATUS_NO_ACK);

                clean_tx_state();
            }
        }
        break;
    }
}


/* -----------------------------------------------------------------------
 *  CSMA-CA
 * -----------------------------------------------------------------------
 */
static bool
csma_contend_for_access(void)
{
    uint8_t cw = MAC_CW_0;

    /* Ensure the channel is clear for the duration of the contention
     * window */
    while (cw--)
    {
        if (!ws_radio_cca())
            return false;
    }

    return true;
}

static void
csma_timer(void)
{
    uint8_t n_backoffs = 0;
    uint8_t backoff_exponent;
    uint8_t rand;
    uint32_t backoff_delay;

#ifdef GPIO_DEBUG
    GPIOPinWrite(DEBUG_GPIO_OTHER_PORT, DEBUG_GPIO_OTHER_CSMA,
                 DEBUG_GPIO_OTHER_CSMA);
#endif

    ps_state.csma_active = true;

    if (mac.batt_life_extension)
    {
        WS_WARN("battery life extension is unsupported\n");
        return;
    }
    else
    {
        backoff_exponent = mac.min_backoff_exponent;
    }

    while (n_backoffs < mac.max_csma_backoffs)
    {
        /* Calculate the backoff delay */
        backoff_delay = (uint32_t)WS_GET_RANDOM8();
        backoff_delay >>= (8 - backoff_exponent);
        backoff_delay *= UNIT_BACKOFF_PERIOD;

        backoff_delay += ws_radio_timer_get_time();

        /* Symbols are shorter than our system timer, and this delay isn't
         * going to be very long so we'll just busy loop */
        while (ws_radio_timer_get_time() < backoff_delay)
            ;

        /* Try to obtain the channel */
        if (csma_contend_for_access())
        {
            ws_radio_transmit();
            ps_state.csma_active = false;
#ifdef GPIO_DEBUG
            GPIOPinWrite(DEBUG_GPIO_OTHER_PORT,
                         DEBUG_GPIO_OTHER_TX_IN_FLIGHT, 0);
            GPIOPinWrite(DEBUG_GPIO_OTHER_PORT, DEBUG_GPIO_OTHER_CSMA, 0);
#endif

            if (ps_state.tx_in_flight != NULL)
            {
                ps_state.tx_state = PACKET_SCHEDULER_TX_STATE_SENT;
            }
            else
            {
                /* TODO: If we could inform the upper layer that the packet
                 * has left the radio, that would be good. Might require an
                 * ack_req boolean so we can store the in_flight pktbuf in
                 * both cases. */
                ps_state.tx_state = PACKET_SCHEDULER_TX_STATE_IDLE;
            }

            WS_DEBUG("transmitted frame\n");
            return;
        }

        n_backoffs++;
    }

    /* CSMA failed */
    WS_DEBUG("csma failed!\n");
    ps_state.csma_active = false;

#ifdef GPIO_DEBUG
    GPIOPinWrite(DEBUG_GPIO_OTHER_PORT, DEBUG_GPIO_OTHER_CSMA, 0);
#endif
}


/* -----------------------------------------------------------------------
 *  MAC API
 * -----------------------------------------------------------------------
 */
void
mac_packet_scheduler_init(void)
{
    WS_DEBUG("init\n");

    memset(&ps_state, 0, sizeof(packet_scheduler_state_t));

    ws_ringbuf_init(&ps_state.rx_data, ps_state.rx_data_buf, INCOMING_RB_LEN);
    ws_list_init(&ps_state.tx_data);

    ws_radio_set_rx_callback(handle_radio_rx_interrupt);
    ws_radio_timer_init(handle_radio_timer_interrupt);

    /* Prepare debug GPIO. These are used in combination with a logic
     * analyser to show the packet scheduler timing */
#ifdef GPIO_DEBUG
    GPIODirModeSet(DEBUG_GPIO_OTHER_PORT,
                   DEBUG_GPIO_OTHER_CSMA | DEBUG_GPIO_OTHER_TX_IN_FLIGHT |
                   DEBUG_GPIO_OTHER_SYNC,
                   GPIO_DIR_MODE_OUT);
    GPIODirModeSet(DEBUG_GPIO_HANDLER_PORT,
                   DEBUG_GPIO_HANDLER_RX | DEBUG_GPIO_HANDLER_TIMER,
                   GPIO_DIR_MODE_OUT);

    GPIOPinWrite(DEBUG_GPIO_OTHER_PORT,
                 DEBUG_GPIO_OTHER_CSMA | DEBUG_GPIO_OTHER_TX_IN_FLIGHT |
                 DEBUG_GPIO_OTHER_SYNC,
                 0x00);
    GPIOPinWrite(DEBUG_GPIO_HANDLER_PORT,
                 DEBUG_GPIO_HANDLER_RX | DEBUG_GPIO_HANDLER_TIMER,
                 0x00);
#endif
}


void
mac_packet_scheduler_clear_receiver(void)
{
    WS_DEBUG("clearing received data\n");

    ws_radio_enter_critical();
    ws_ringbuf_flush(&ps_state.rx_data);
    ws_radio_exit_critical();
}


void
mac_packet_scheduler_sync(void)
{
    ps_state.slot_count = 0;
    ws_radio_timer_syncronise();

#ifdef GPIO_DEBUG
    GPIOPinWrite(DEBUG_GPIO_OTHER_PORT, DEBUG_GPIO_OTHER_SYNC,
                 DEBUG_GPIO_OTHER_SYNC);
#endif
}


void
mac_packet_scheduler_send_data(ws_pktbuf_t *pkt)
{
    WS_DEBUG("send data\n");

    packet_scheduler_queue_t *data =
        (packet_scheduler_queue_t *)MALLOC(sizeof(packet_scheduler_queue_t));
    if (data == NULL)
    {
        WS_ERROR("failed to allocate memory for tx data\n");
        return;
    }

    WS_DEBUG("frame: %r\n",
                     ws_pktbuf_get_data(pkt),
                     ws_pktbuf_get_len(pkt));

    data->pkt = pkt;

    ws_list_add_before(&ps_state.tx_data, &data->list);

    WS_DEBUG("pending list size (count=%u)\n",
             ws_list_count(&ps_state.tx_data));

    WS_TIMER_SET_NOW(packet_scheduler_timer);
}
