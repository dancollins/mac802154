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


mac_t mac;


void
mac_mlme_send_beacon_request(void)
{
    ws_pktbuf_t *pkt = ws_pktbuf_create(WS_RADIO_MAX_PACKET_LEN);

    mac_fcf_t *fcf = (mac_fcf_t *)ws_pktbuf_get_data(pkt);
    uint8_t *ptr = fcf->data;
    ws_mac_addr_t dest;

    memset(fcf, 0, sizeof(mac_fcf_t));
    fcf->frame_type = MAC_FRAME_TYPE_MAC;
    fcf->security_enabled = 0;
    fcf->frame_pending = 0;
    fcf->ack_req = 0;
    fcf->frame_version = WS_MAC_MAX_FRAME_VERSION;

    /* Sequence number */
    *ptr++ = mac.sqn++;

    /* Add a broadcast destination address */
    dest.type = WS_MAC_ADDR_TYPE_SHORT;
    dest.pan_id = WS_MAC_BROADCAST_ADDR;
    dest.short_addr = WS_MAC_BROADCAST_ADDR;
    WS_DEBUG("appending address\n");
    ptr += mac_frame_append_address(fcf, &dest, NULL);

    *ptr++ = MAC_COMMAND_BEACON_REQUEST;

    ws_pktbuf_increment_end(pkt, (uint32_t)(ptr - (uint8_t *)fcf));

    mac_packet_scheduler_send_data(pkt);
}


void
mac_mlme_send_association_request(ws_mac_addr_t *dest)
{
    ws_pktbuf_t *pkt = ws_pktbuf_create(WS_RADIO_MAX_PACKET_LEN);

    mac_fcf_t *fcf = (mac_fcf_t *)ws_pktbuf_get_data(pkt);
    uint8_t *ptr = fcf->data;
    ws_mac_addr_t src;
    mac_capability_info_t *info;

    memset(fcf, 0, sizeof(mac_fcf_t));
    fcf->frame_type = MAC_FRAME_TYPE_MAC;
    fcf->security_enabled = 0;
    fcf->frame_pending = 0;
    fcf->ack_req = 1;
    fcf->frame_version = WS_MAC_MAX_FRAME_VERSION;

    /* Sequence number */
    *ptr++ = mac.sqn++;

    /* Packet is directed to the coordinator from our extended address */
    src.type = WS_MAC_ADDR_TYPE_EXTENDED;
    src.pan_id = dest->pan_id;
    memcpy(&src.extended_addr, mac.extended_address,
           WS_MAC_ADDR_TYPE_EXTENDED_LEN);
    WS_DEBUG("appending address\n");
    ptr += mac_frame_append_address(fcf, dest, &src);

    *ptr++ = MAC_COMMAND_ASSOCIATION_REQUEST;

    info = (mac_capability_info_t *)ptr;
    ptr = info->data;

    /* TODO: Don't hard code our capabilities..! */
    memset(info, 0, sizeof(mac_capability_info_t));
    info->device_type = MAC_DEVICE_TYPE_FFD;
    info->power_source = 0;
    info->rx_when_idle = 1;
    info->security_capable = 0;
    info->allocate_addr = 1;

    ws_pktbuf_increment_end(pkt, (uint32_t)(ptr - (uint8_t *)fcf));

    mac_packet_scheduler_send_data(pkt);
}


void
mac_mlme_send_data_request(ws_mac_addr_t *dest)
{
    ws_pktbuf_t *pkt = ws_pktbuf_create(WS_RADIO_MAX_PACKET_LEN);

    mac_fcf_t *fcf = (mac_fcf_t *)ws_pktbuf_get_data(pkt);
    uint8_t *ptr = fcf->data;
    ws_mac_addr_t src;
    mac_capability_info_t *info;

    memset(fcf, 0, sizeof(mac_fcf_t));
    fcf->frame_type = MAC_FRAME_TYPE_MAC;
    fcf->security_enabled = 0;
    fcf->frame_pending = 0;
    fcf->ack_req = 1;
    fcf->frame_version = WS_MAC_MAX_FRAME_VERSION;

    /* Sequence number */
    *ptr++ = mac.sqn++;

    /* Packet is directed to the coordinator from our extended address */
    src.type = WS_MAC_ADDR_TYPE_EXTENDED;
    src.pan_id = dest->pan_id;
    memcpy(&src.extended_addr, mac.extended_address,
           WS_MAC_ADDR_TYPE_EXTENDED_LEN);
    WS_DEBUG("appending address\n");
    ptr += mac_frame_append_address(fcf, dest, &src);

    *ptr++ = MAC_COMMAND_DATA_REQUEST;

    ws_pktbuf_increment_end(pkt, (uint32_t)(ptr - (uint8_t *)fcf));

    mac_packet_scheduler_send_data(pkt);
}


static void
handle_mac_packet(ws_pktbuf_t *pkt)
{
    mac_fcf_t *fcf = (mac_fcf_t *)ws_pktbuf_get_data(pkt);
    uint8_t *ptr;
    ws_mac_addr_t src;

    ASSERT(fcf->frame_type == MAC_FRAME_TYPE_MAC,
           "invalid frame (type=%u)\n", fcf->frame_type);

    /* Extract the source address */
    ptr = mac_frame_extract_address(fcf, NULL, &src);

    /* TODO: Security */
    if (fcf->security_enabled)
    {
        WS_ERROR("Security is unsupported\n");
        return;
    }

    switch (*ptr)
    {
    case MAC_COMMAND_ASSOCIATION_REQUEST:
        WS_DEBUG("Got association request!\n");
        break;

    case MAC_COMMAND_DISASSOC_REQUEST:
        WS_DEBUG("Got disassociation request!\n");
        break;

    case MAC_COMMAND_DATA_REQUEST:
        WS_DEBUG("Got data request!\n");
        break;

    case MAC_COMMAND_PAN_ID_COLLECT_INFO:
        WS_DEBUG("Got PAN ID collect info!\n");
        break;

    case MAC_COMMAND_ORPHAN_NOTIFICATION:
        WS_DEBUG("Got orphan notification!\n");
        break;

    case MAC_COMMAND_BEACON_REQUEST:
        WS_DEBUG("Got beacon request!\n");
        break;

    case MAC_COMMAND_COORDINATOR_REALIGN:
        WS_DEBUG("Got coordinator realignment!\n");
        break;

    case MAC_COMMAND_GTS_REQUEST:
        WS_DEBUG("Got GTS request!\n");
        break;

    default:
        WS_DEBUG("Ignoring MAC command (%u)\n", *ptr);
        break;
    }
}


/* TODO: merge this into the packet scheduler. It's not needed twice..! */
void
mac_mlme_handle_packet(ws_pktbuf_t *pkt)
{
    mac_fcf_t *fcf = (mac_fcf_t *)ws_pktbuf_get_data(pkt);
    uint32_t len = ws_pktbuf_get_len(pkt);

    switch (mac.state)
    {
    default:
        WS_ERROR("Unhandled frame (type=%u) in state (%u)\n",
                 fcf->frame_type, mac.state);
        break;
    }

    WS_DEBUG("dest\n");
    ws_pktbuf_destroy(pkt);
}


void
mac_mlme_handle_status(uint8_t sqn, mac_tx_status_t status)
{

}


uint8_t
mac_mlme_get_sqn(void)
{
    return mac.sqn++;
}


/*
 * Public API
 */
void
ws_mac_init(uint8_t *extended_address)
{
    memset(&mac, 0, sizeof(mac));

    /* PIB */
    memcpy(&mac.extended_address, extended_address,
           WS_MAC_ADDR_TYPE_EXTENDED_LEN);
    mac.beacon_order = 15;
    mac.pan_id = 0xffff;
    mac.short_address = 0xffff;
    mac.superframe_order = 15;
    mac.responseWaitTime = 32;
    mac.coord_short_address = 0xffff;
    mac.batt_life_extension = false;
    mac.min_backoff_exponent = 3;
    mac.max_backoff_exponent = 5;
    mac.max_csma_backoffs = 4;
    mac.sqn = WS_GET_RANDOM8();
    ws_list_init(&mac.device_list);
    mac.max_frame_retries = 3;

    mac.current_channel = 11;

    mac.state = MAC_STATE_IDLE;

    /* Set up the radio */
    ws_radio_init();
    ws_radio_set_power(true);

    ws_radio_set_pan_id(0xffff);
    ws_radio_set_short_address(0xffff);
    ws_radio_set_extended_address(extended_address);

    /* Set up the mac components */
    mac_packet_scheduler_init();
    mac_mcps_init();
    mac_coordinator_init();
}


ws_mac_start_status_t
ws_mac_mlme_start(uint16_t pan_id, uint8_t channel,
                  uint8_t beacon_order, uint8_t superframe_order,
                  bool pan_coordinator,
                  ws_mac_coordinator_associate_callback_t associate_callback)
{
    /* Check the input parameters */
    if (channel < WS_RADIO_MIN_CHANNEL || channel > WS_RADIO_MAX_CHANNEL)
        return WS_MAC_START_INVALID_PARAMETER;

    if (beacon_order > 16 || superframe_order > 16)
        return WS_MAC_START_INVALID_PARAMETER;

    if (pan_id == 0xffff)
        return WS_MAC_START_INVALID_PARAMETER;

    if (mac.short_address == 0xffff)
        return WS_MAC_START_NO_SHORT_ADDRESS;

    WS_DEBUG("START\n");

    ws_radio_enter_critical();

    /* PIB */
    mac.pan_id = pan_id;
    mac.beacon_order = beacon_order;
    mac.superframe_order = superframe_order;

    mac.state = MAC_STATE_COORDINATING;
    mac.is_pan_coordinator = pan_coordinator;

    /* Configure the radio */
    mac.current_channel = channel;
    ws_radio_set_channel(mac.current_channel);
    ws_radio_set_pan_id(pan_id);

    /* Packet scheduler will start sending timed beacons */
    ws_radio_timer_set_superframe_order(superframe_order);
    ws_radio_timer_enable_interrupts();
    mac_packet_scheduler_sync();

    ws_radio_exit_critical();

    mac_coordinator_register_associate_callback(associate_callback);

    return WS_MAC_START_SUCCESS;
}


uint16_t
ws_mac_mlme_get_short_address(void)
{
    return mac.short_address;
}


void
ws_mac_mlme_set_short_address(uint16_t addr)
{
    mac.short_address = addr;
    ws_radio_set_short_address(addr);
}


uint8_t *
ws_mac_mlme_get_extended_address(void)
{
    return mac.extended_address;
}


uint16_t
ws_mac_mlme_get_pan_id(void)
{
    return mac.pan_id;
}


void
ws_mac_mlme_get_address(ws_mac_addr_t *addr)
{
    addr->pan_id = mac.pan_id;
    if (mac.short_address < 0xfffe)
    {
        addr->type = WS_MAC_ADDR_TYPE_SHORT;
        addr->short_addr = mac.short_address;
    }
    else
    {
        addr->type = WS_MAC_ADDR_TYPE_EXTENDED;
        memcpy(&addr->extended_addr, &mac.extended_address,
            WS_MAC_ADDR_TYPE_EXTENDED_LEN);
    }
}


