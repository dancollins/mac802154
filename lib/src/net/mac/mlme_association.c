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


/* TODO: Ugly hack! */
#define DEVICE_NUM (2)
#if DEVICE_NUM == 1
static uint8_t aes_key[] = {
    0xbd, 0xf4, 0xbc, 0xed, 0x02, 0x12, 0x4b, 0x41,
    0x36, 0x41, 0x80, 0x42, 0x57, 0xfe, 0xef, 0xdd,
};
#elif DEVICE_NUM == 2
static uint8_t aes_key[] = {
    0xe8, 0xa7, 0xfb, 0xeb, 0xa7, 0x1f, 0xcc, 0x1c,
    0xaa, 0x63, 0x2e, 0x1a, 0x47, 0xbd, 0x74, 0xbb,
};
#endif


typedef enum
{
    STATE_START,
    STATE_ASSOC_REQ_SENT,
    STATE_ASSOC_REQ_ACKED,
    STATE_DATA_REQ_SENT,
    STATE_WAIT_ASSOC_RESP,
} association_state_t;


typedef struct
{
    association_state_t state;
    ws_mac_addr_t coord_addr;
    uint8_t last_sqn;
    ws_mac_association_callback_t cb;
} association_t;

static association_t assoc;


/** Time to wait for a message reponse from the coordinator */
/* TODO: This should be a function of the beacon interval */
#define MSG_TIMEOUT (1000)


WS_TIMER_DECLARE(association_timer);

static void
association_timer(void)
{
    switch (assoc.state)
    {
    case STATE_START:
        assoc.state = STATE_ASSOC_REQ_SENT;
        assoc.last_sqn = mac.sqn;
        mac_mlme_send_association_request(&assoc.coord_addr);
        break;

    case STATE_ASSOC_REQ_SENT:
        WS_DEBUG("Association timed out with no response\n");
        WS_ERROR("Failed to associate\n");

        if (assoc.cb != NULL)
            assoc.cb(WS_MAC_ASSOCIATION_NO_ACK, 0xffff);
        break;

    case STATE_ASSOC_REQ_ACKED:
        assoc.state = STATE_DATA_REQ_SENT;
        assoc.last_sqn = mac.sqn;
        mac_mlme_send_data_request(&assoc.coord_addr);
        break;

    case STATE_DATA_REQ_SENT:
        WS_DEBUG("Association timed out with no response\n");
        WS_ERROR("Failed to associate\n");

        if (assoc.cb != NULL)
            assoc.cb(WS_MAC_ASSOCIATION_NO_ACK, 0xffff);
        break;

    case STATE_WAIT_ASSOC_RESP:
        WS_ERROR("Timed out waiting for association response\n");
        if (assoc.cb != NULL)
            assoc.cb(WS_MAC_ASSOCIATION_NO_DATA, 0xffff);
    }
}


static bool
check_address(ws_mac_addr_t *addr)
{
    if (addr->pan_id != assoc.coord_addr.pan_id)
        return false;

    if (addr->type != assoc.coord_addr.type)
        return false;

    switch (addr->type)
    {
    case WS_MAC_ADDR_TYPE_NONE:
        return true;

    case WS_MAC_ADDR_TYPE_SHORT:
        return addr->short_addr == assoc.coord_addr.short_addr;

    case WS_MAC_ADDR_TYPE_EXTENDED:
        return memcmp(addr->extended_addr, &assoc.coord_addr.extended_addr,
                      WS_MAC_ADDR_TYPE_EXTENDED_LEN) == 0;

    default:
        WS_WARN("Unknown address type (%u)\n", addr->type);
        return false;
    }
}


/* TODO: The memory management is a bit ugly here, as I've left it up to
 * the end point for data to clean up. However, this function is an end
 * point in most cases, but not all (it is not a beacon end point).
 * Would be be the case that we could make the pointer point to null?
 * *pkt = NULL in the pktbuf code and then we can check in functions like
 * this one to see if the packet has been cleaned up. If it hasn't, we can
 * clean it up ourself. If it has, very good.
 */
void
mac_mlme_association_handle_packet(ws_pktbuf_t *pkt)
{
    mac_fcf_t *fcf = (mac_fcf_t *)ws_pktbuf_get_data(pkt);
    uint8_t sqn;
    ws_mac_addr_t src;
    uint8_t *ptr;

    mac_device_t *dev;

    sqn = *fcf->data;

    WS_DEBUG("frame type: %u\n", fcf->frame_type);

    switch (fcf->frame_type)
    {
    case MAC_FRAME_TYPE_BEACON:
        mac_frame_extract_address(fcf, NULL, &src);
        if (!check_address(&src))
        {
            WS_DEBUG("Ignoring beacon from (%04x, %04x)\n",
                     src.pan_id, src.short_addr);
            goto cleanup;
        }

        mac_packet_scheduler_sync();
        mac_coordinator_handle_packet(pkt);
        pkt = NULL;
        break;

    case MAC_FRAME_TYPE_MAC:
        if (assoc.state == STATE_WAIT_ASSOC_RESP)
        {
            ptr = mac_frame_extract_address(fcf, NULL, &src);

            if (*ptr != MAC_COMMAND_ASSOCIATION_RESPONSE)
            {
                WS_DEBUG("Ignoring MAC frame (type=%u) in state (%u)\n",
                         *ptr, assoc.state);
                goto cleanup;
            }
            *ptr++;

            if (src.type != WS_MAC_ADDR_TYPE_EXTENDED)
            {
                WS_ERROR("Invalid association response!\n");

                if (assoc.cb != NULL)
                    assoc.cb(WS_MAC_ASSOCIATION_NO_DATA, 0xffff);

                goto cleanup;
            }

            /* Save the coordinator address */
            mac.pan_id = assoc.coord_addr.pan_id;
            mac.coord_short_address = assoc.coord_addr.short_addr;
            memcpy(&mac.coord_extended_address, &src.extended_addr,
                   WS_MAC_ADDR_TYPE_EXTENDED_LEN);

            /* Save our new short address. Even if association fails,
             * this value will be valid (0xffff on failure) */
            memcpy(&mac.short_address, ptr, 2);
            ptr += 2;
            ws_radio_set_short_address(mac.short_address);

            /* Check the association status */
            if (*ptr != WS_MAC_ASSOCIATION_SUCCESS)
            {
                WS_ERROR("Failed to association with error (%02x)\n", *ptr);

                if (assoc.cb != NULL)
                    assoc.cb(*ptr, 0xffff);

                mac.state = MAC_STATE_IDLE;
                goto cleanup;
            }

            WS_DEBUG("Associated to (%04x, %04x) with address (%04x)\n",
                     assoc.coord_addr.pan_id, assoc.coord_addr.short_addr,
                     mac.short_address);

            WS_TIMER_CANCEL(association_timer);

            dev = (mac_device_t *)MALLOC(sizeof(mac_device_t));
            if (dev == NULL)
            {
                WS_ERROR("failed to create device\n");
                mac.short_address = 0xffff;
                mac.state = MAC_STATE_IDLE;

                if (assoc.cb != NULL)
                    assoc.cb(*ptr, 0xffff);
                goto cleanup;
            }

            ws_list_init(&dev->key_list);
            memcpy(dev->addr.extended_addr, mac.coord_extended_address,
                   WS_MAC_ADDR_TYPE_EXTENDED_LEN);
            dev->addr.pan_id = mac.pan_id;
            dev->addr.short_addr = mac.coord_short_address;

            ws_list_add_after(&mac.device_list, &dev->list);

            /* TODO: Decide exactly how we want to manage keys */
            mac_device_set_key(dev, 0, mac.own_key);

            mac.state = MAC_STATE_ASSOCIATED;

            if (assoc.cb != NULL)
                assoc.cb(WS_MAC_ASSOCIATION_SUCCESS, mac.short_address);
        }
        else
        {
            WS_DEBUG("Ignoring mac frame in association state (%u)\n",
                     assoc.state);
        }
    }

cleanup:
    if (pkt != NULL)
    {
        WS_DEBUG("dest\n");
        ws_pktbuf_destroy(pkt);
    }
}


void
mac_mlme_association_handle_status(uint8_t sqn, mac_tx_status_t status)
{
    UNUSED(sqn);

    switch (assoc.state)
    {
    case STATE_ASSOC_REQ_SENT:
        WS_DEBUG("assoc req status (%u)\n", status);
        WS_TIMER_CANCEL(association_timer);
        if (status == MAC_TX_STATUS_SUCCESS)
        {
            assoc.state = STATE_ASSOC_REQ_ACKED;
            WS_TIMER_SET(association_timer,
                         mac.responseWaitTime * 60);
        }
        else
        {
            WS_ERROR("Failed to associate\n");

            if (assoc.cb != NULL)
                assoc.cb(WS_MAC_ASSOCIATION_NO_ACK, 0xffff);
        }
        break;

    case STATE_DATA_REQ_SENT:
        WS_DEBUG("data req status (%u)\n", status);
        WS_TIMER_CANCEL(association_timer);
        if (status == MAC_TX_STATUS_SUCCESS)
        {
            WS_DEBUG("Waiting for association response\n");
            assoc.state = STATE_WAIT_ASSOC_RESP;
            WS_TIMER_SET(association_timer, MSG_TIMEOUT);
        }
        else
        {
            WS_ERROR("Failed to associate\n");

            if (assoc.cb != NULL)
                assoc.cb(WS_MAC_ASSOCIATION_NO_ACK, 0xffff);
        }
        break;

    default:
        WS_WARN("ignoring tx status in state (%u)\n", status);
        break;
    }
}


void
ws_mac_mlme_associate(ws_mac_pan_descriptor_t *pan,
                      ws_mac_association_callback_t cb)
{
    if (cb == NULL)
    {
        WS_WARN("NULL callback!\n");
    }

    WS_DEBUG("Associating with (%u, %04x)\n",
             pan->channel, pan->addr.pan_id);

    mac.state = MAC_STATE_ASSOCIATING;

    /* Prepare the association state */
    memset(&assoc, 0, sizeof(assoc));
    memcpy(&assoc.coord_addr, &pan->addr, sizeof(ws_mac_addr_t));
    assoc.state = STATE_START;
    assoc.cb = cb;

    ws_radio_set_channel(pan->channel);
    ws_radio_set_pan_id(pan->addr.pan_id);
    ws_radio_timer_enable_interrupts();
    ws_radio_timer_set_superframe_order(pan->superframe_spec.superframe_order);

    WS_TIMER_SET_NOW(association_timer);
}
