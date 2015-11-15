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

typedef struct
{
    ws_mac_scan_type_t type;
    ws_mac_scan_callback_t cb;
    uint32_t channel_duration;
    uint16_t channels;
    uint8_t channel;
    ws_list_t scan_results;
} scan_state_t;

static scan_state_t scan_state;


WS_TIMER_DECLARE(scan_timer);


/* if a >  b return  1
 * if a == b return  0
 * if a <  b return -1 */
/* TODO: Standard states that a beacon is unique only if the PAN ID, channel
 * and source address are unique. This is only testing PAN and channel */
static int
compare_scan_result(ws_list_t *a, ws_list_t *b)
{
    ws_mac_scan_result_t *this =
        ws_list_get_data(a, ws_mac_scan_result_t, list);
    ws_mac_scan_result_t *that =
        ws_list_get_data(b, ws_mac_scan_result_t, list);

    if (this->pan_desc.addr.pan_id < that->pan_desc.addr.pan_id)
        return -1;

    if (this->pan_desc.addr.pan_id == that->pan_desc.addr.pan_id)
    {
        if (this->pan_desc.channel == that->pan_desc.channel)
            return 0;

        if (this->pan_desc.channel < that->pan_desc.channel)
            return -1;
    }

    return 1;
}


static void
clear_scan_results(void)
{
    ws_list_t *ptr = scan_state.scan_results.next;
    ws_mac_scan_result_t *res;

    /* List isn't initialised, so it must be empty */
    if (ptr == NULL)
    {
        WS_WARN("clearing uninitialised list\n");
        return;
    }

    while (ptr != &scan_state.scan_results)
    {
        res = ws_list_get_data(ptr, ws_mac_scan_result_t, list);
        ptr = ptr->next;

        ws_list_remove(&res->list);
        FREE(res);
    }
}


static void
scan_timer(void)
{
    /* Figure out the next channel to scan */
    scan_state.channels >>= 1;
    scan_state.channel++;

    while (!(scan_state.channels & 1) &&
           scan_state.channel <= WS_RADIO_MAX_CHANNEL)
    {
        scan_state.channel++;
        scan_state.channels >>= 1;
    }

    if (scan_state.channels == 0)
    {
        /* Scan is complete! */
        WS_DEBUG("Scan complete\n");

        mac.state = MAC_STATE_IDLE;

        scan_state.cb(WS_MAC_SCAN_SUCCESS,
                      scan_state.type,
                      &scan_state.scan_results);
        clear_scan_results();
    }
    else
    {
        /* Scan the next channel */
        WS_DEBUG("Scanning channel %u\n", scan_state.channel);
        ws_radio_set_channel(scan_state.channel);

        /* Send a beacon request if this is an active scan */
        if (scan_state.type == WS_MAC_SCAN_TYPE_ACTIVE)
            mac_mlme_send_beacon_request();

        WS_TIMER_SET(scan_timer, scan_state.channel_duration);
    }
}


/* TODO: Depending on macAutoRequest, we also need to pass the frame up the
 * stack. In all cases, we want to pass the beacon data payload up the stack */
void
mac_mlme_scan_handle_packet(ws_pktbuf_t *pkt)
{
    mac_fcf_t *fcf = (mac_fcf_t *)ws_pktbuf_get_data(pkt);
    uint8_t *ptr;
    mac_superframe_spec_t *spec;

    ws_mac_scan_result_t *res;
    ws_mac_beacon_superframe_spec_t *superframe_spec;

    ws_list_t *head;

    ASSERT(fcf->frame_type == MAC_FRAME_TYPE_BEACON,
           "invalid frame received in scan state\n");

    res = (ws_mac_scan_result_t *)MALLOC(sizeof(ws_mac_scan_result_t));
    if (res == NULL)
    {
        WS_ERROR("Failed to allocate memory for scan result\n");
        goto cleanup;
    }

    /* Sync up the slot timer */
    mac_packet_scheduler_sync();

    /* Extract the source address */
    ptr = mac_frame_extract_address(fcf, NULL, &res->pan_desc.addr);

    /* TODO: Security */
    if (fcf->security_enabled)
    {
        WS_ERROR("Security is unsupported\n");
        FREE(res);
        goto cleanup;
    }

    res->pan_desc.channel = scan_state.channel;
    /* TODO: The bytes to calculate this are sitting at the end of the
     * pktbuf. We just need to use them */
    res->pan_desc.link_quality = 0x00;

    /* Pull the superframe specification */
    spec = (mac_superframe_spec_t *)ptr;
    res->pan_desc.superframe_spec.beacon_order = spec->beacon_order;
    res->pan_desc.superframe_spec.superframe_order = spec->superframe_order;
    res->pan_desc.superframe_spec.final_cap_slot = spec->final_cap_slot;
    res->pan_desc.superframe_spec.batt_life_ext = spec->ble;
    res->pan_desc.superframe_spec.pan_coordinator = spec->pan_coordinator;
    res->pan_desc.superframe_spec.association_permit = spec->association_permit;

    WS_DEBUG("Found beacon (ch=%u, id=0x%04x, bo=%u, so=%u)\n",
             res->pan_desc.channel,
             res->pan_desc.addr.pan_id,
             res->pan_desc.superframe_spec.beacon_order,
             res->pan_desc.superframe_spec.superframe_order);

    ws_list_add_sorted(&scan_state.scan_results, &res->list,
                       compare_scan_result);

cleanup:
    WS_DEBUG("dest\n");
    ws_pktbuf_destroy(pkt);
}


void
ws_mac_mlme_scan(ws_mac_scan_type_t type, uint16_t channels,
                 uint8_t duration, ws_mac_scan_callback_t cb)
{
    if (mac.state == MAC_STATE_SCANNING)
    {
        cb(WS_MAC_SCAN_IN_PROGRESS, type, NULL);
        return;
    }

    if (channels == 0)
    {
        cb(WS_MAC_SCAN_INVALID_PARAMETER, type, NULL);
        return;
    }

    if (type == WS_MAC_SCAN_TYPE_ED || type == WS_MAC_SCAN_TYPE_ORPHAN)
    {
        WS_WARN("Scan type not implemented yet!\n");
        return;
    }

    WS_DEBUG("SCAN\n");

    ws_radio_enter_critical();

    mac.state = MAC_STATE_SCANNING;

    /* Stop the radio from filtering out packets */
    ws_radio_set_pan_id(0xffff);

    memset(&scan_state, 0, sizeof(scan_state));
    scan_state.type = type;
    scan_state.channels = channels;
    scan_state.cb = cb;

    ws_list_init(&scan_state.scan_results);

    /* Figure out which channel to start scanning on */
    scan_state.channel = WS_RADIO_MIN_CHANNEL;
    while (!(scan_state.channels & 1) &&
           scan_state.channel <= WS_RADIO_MAX_CHANNEL)
    {
        WS_DEBUG("Skipping channel %u\n", scan_state.channel);
        scan_state.channel++;
        scan_state.channels >>= 1;
    }

    /* Calculate the scan duration per channel */
    scan_state.channel_duration = (1 << duration) + 1;
    scan_state.channel_duration *= WS_RADIO_SLOT_DURATION;

    WS_DEBUG("Scanning channel %u\n", scan_state.channel);
    ws_radio_set_channel(scan_state.channel);

    ws_radio_exit_critical();

    if (type == WS_MAC_SCAN_TYPE_ACTIVE)
        mac_mlme_send_beacon_request();

    WS_TIMER_SET(scan_timer, scan_state.channel_duration);
}
