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


#if 1
#undef WS_DEBUG
#define WS_DEBUG(...)
#endif


/* Macro to access the private coordinator data associated with a device */
#define CDATA(dev) ((device_coord_data_t *)dev->coord_data)


typedef enum
{
    DEVICE_STATE_ASSOCIATING,
    DEVICE_STATE_ASSOCIATED
} device_state_t;


/* Additional data stored per device specific to the coordinator role */
typedef struct
{
    ws_pktbuf_t *pending_data;
    device_state_t state;
    mac_device_type_t type;
} device_coord_data_t;


typedef struct
{
    uint8_t beacon_sqn;
    ws_mac_beacon_rx_callback_t rx_cb;
    ws_mac_coordinator_associate_callback_t associate_cb;
    ws_pktbuf_t *beacon;
} coordinator_t;

static coordinator_t coord;


static void
prepare_association_response(ws_mac_addr_t *dest, mac_device_t *dev,
                             ws_mac_association_status_t stat)
{
    ws_pktbuf_t *pkt = ws_pktbuf_create(WS_RADIO_MAX_PACKET_LEN);
    if (pkt == NULL)
    {
        WS_ERROR("failed to allocate pktbuf for association response\n");
        return;
    }

    mac_fcf_t *fcf = (mac_fcf_t *)ws_pktbuf_get_data(pkt);
    uint8_t *ptr = fcf->data;
    ws_mac_addr_t src;
    mac_capability_info_t *info = NULL;

    memset(fcf, 0, sizeof(mac_fcf_t));
    fcf->frame_type = MAC_FRAME_TYPE_MAC;
    fcf->security_enabled = 0;
    fcf->frame_pending = 0;
    fcf->ack_req = 1;
    fcf->frame_version = WS_MAC_MAX_FRAME_VERSION;

    /* Sequence number */
    dev->last_sqn = mac_mlme_get_sqn();
    *ptr++ = dev->last_sqn;

    /* Packet is directed to the device from our extended address */
    src.type = WS_MAC_ADDR_TYPE_EXTENDED;
    src.pan_id = dest->pan_id;
    memcpy(&src.extended_addr, mac.extended_address,
           WS_MAC_ADDR_TYPE_EXTENDED_LEN);
    WS_DEBUG("appending address\n");
    ptr += mac_frame_append_address(fcf, dest, &src);

    *ptr++ = MAC_COMMAND_ASSOCIATION_RESPONSE;

    memcpy(ptr, &dev->addr.short_addr, 2);
    ptr += 2;

    *ptr++ = stat;

    ws_pktbuf_increment_end(pkt, (uint32_t)(ptr - (uint8_t *)fcf));

    WS_DEBUG("pending packet (%p) for device (%04x)\n", pkt,
             dev->addr.short_addr);
    CDATA(dev)->pending_data = pkt;
}


static void
handle_association_request(mac_fcf_t *fcf, ws_mac_addr_t *src)
{
    mac_device_t *dev = NULL;
    ws_list_t *ptr;

    WS_DEBUG("Got association request!\n");

    /* TODO: If we're not offering association, then we want to respond
     * with access denied */

    if (src->type != WS_MAC_ADDR_TYPE_EXTENDED)
    {
        WS_ERROR("Invalid source address type for association request\n");
        return;
    }

    dev = mac_device_get_by_extended(&src->extended_addr[0]);
    if (dev != NULL)
    {
        /* A device we've seen before is trying to re-associate. We'll clear
         * any pending data and prepare a new association response. */
        if (CDATA(dev)->pending_data != NULL)
        {
            WS_DEBUG("dest\n");
            ws_pktbuf_destroy(CDATA(dev)->pending_data);
            CDATA(dev)->pending_data = NULL;
        }
    }
    else
    {
        /* A new device is trying to associate */
        dev = mac_coordinator_create_device(src);
        if (dev == NULL)
        {
            WS_ERROR("Failed to create device\n");
            return;
        }

        dev->addr.type = WS_MAC_ADDR_TYPE_SHORT;
        dev->addr.pan_id = src->pan_id;
        /* TODO: Randomly assign addresses that aren't taken */
        dev->addr.short_addr = ws_list_count(&mac.device_list) + 1;

        ws_list_add_after(&mac.device_list, &dev->list);

        WS_DEBUG("Added new device (%04x)\n", dev->addr.short_addr);
    }

    WS_DEBUG("device associating: %p\n", dev);
    CDATA(dev)->state = DEVICE_STATE_ASSOCIATING;

    prepare_association_response(src, dev,
                                 WS_MAC_ASSOCIATION_SUCCESS);

    /* TODO: Add a timeout to the new device so we can free some memory if
     * the device never requests the data */
}


static void
handle_data_request(mac_fcf_t *fcf, ws_mac_addr_t *src)
{
    mac_device_t *dev = NULL;
    ws_list_t *ptr;

    WS_DEBUG("got data request\n");

    if (src->pan_id != mac.pan_id)
    {
        WS_WARN("Ignoring data request from (pan=%04x)\n", src->pan_id);
        return;
    }

    if (ws_list_is_empty(&mac.device_list))
    {
        WS_DEBUG("No associated devices\n");
        return;
    }

    WS_DEBUG("finding device with address: ");
    mac_frame_print_address(src);
    PRINTF("\n");

    dev = mac_device_get_by_addr(src);
    if (dev != NULL && CDATA(dev)->pending_data != NULL)
    {
        WS_DEBUG("device: %p\n", dev);
        WS_DEBUG("sending data to: ");
        mac_frame_print_address(src);
        PRINTF("\n");
        WS_DEBUG("(pkt=%p)\n", CDATA(dev)->pending_data);
        mac_packet_scheduler_send_data(CDATA(dev)->pending_data);
        CDATA(dev)->pending_data = NULL;
    }
    else
    {
        WS_WARN("dev = %p\n", dev);
        if (dev != NULL)
        {
            WS_WARN("dev->pending_data = %p\n", CDATA(dev)->pending_data);
        }
    }
}


void
mac_coordinator_init(void)
{
    memset(&coord, 0, sizeof(coord));
    coord.beacon = ws_pktbuf_create(WS_RADIO_MAX_PACKET_LEN);
    WS_DEBUG("coodinator beacon (ptr=%p)\n", coord.beacon);
}


void
mac_coordinator_handle_packet(ws_pktbuf_t *pkt)
{
    mac_fcf_t *fcf = (mac_fcf_t *)ws_pktbuf_get_data(pkt);
    uint8_t *ptr;
    ws_mac_addr_t src;

    uint16_t saddr;
    uint8_t sqn = fcf->data[0];

    mac_device_t *dev;
    ws_list_t *lptr;

    ptr = mac_frame_extract_address(fcf, NULL, &src);

    mac_superframe_spec_t *spec;
    mac_gts_spec_t *gts_spec;
    mac_pending_addr_t *pending_addr;

    WS_DEBUG("frame type: %u\n", fcf->frame_type);

    switch (fcf->frame_type)
    {
    case MAC_FRAME_TYPE_BEACON:
        if (mac.state == MAC_STATE_COORDINATING)
        {
            /* TODO: Figure out if we need to realign */
            WS_WARN("Beacon frame detected\n");
        }
        else if (mac.state == MAC_STATE_ASSOCIATED)
        {
            if (src.type == WS_MAC_ADDR_TYPE_SHORT &&
                src.short_addr == mac.coord_short_address)
            {
                /* This came from our coordinator, so we need to sync */
                mac_packet_scheduler_sync();

                spec = (mac_superframe_spec_t *)ptr;
                gts_spec = (mac_gts_spec_t *)spec->data;
                pending_addr = (mac_pending_addr_t *)gts_spec->data;

                /* We should also check for pending addresses so see if we
                 * need to request data */
                if (pending_addr->short_addr_count > 0)
                {
                    ptr = pending_addr->data;
                    while (pending_addr->short_addr_count--)
                    {
                        memcpy(&saddr, ptr, 2);
                        ptr += 2;
                        if (saddr == mac.short_address)
                        {
                            mac_mlme_send_data_request(&src);
                        }
                    }
                }
            }
            else if (src.type == WS_MAC_ADDR_TYPE_EXTENDED)
            {
                WS_WARN("TODO: Handle extended ADDR beacon frames\n");
            }
            else
            {
                WS_DEBUG("ignoring beacon from unknown coordinator (0x%04x)\n",
                    src.short_addr);
            }
        }

        /* Pass the frame up */
        break;

    case MAC_FRAME_TYPE_MAC:
        switch (*ptr)
        {
        case MAC_COMMAND_ASSOCIATION_REQUEST:
            handle_association_request(fcf, &src);
            break;

        case MAC_COMMAND_DATA_REQUEST:
            handle_data_request(fcf, &src);
            break;

        case MAC_COMMAND_BEACON_REQUEST:
            WS_DEBUG("Got beacon request\n");
            /* TODO: If our beacon order is 16 then we should send a beacon */
            break;

        default:
            WS_DEBUG("Ignoring MAC frame (cmd=%02x)\n", *ptr);
            break;
        }
        break;
    }

    WS_DEBUG("dest\n");
    ws_pktbuf_destroy(pkt);
}


void
mac_coordinator_handle_status(uint8_t sqn, mac_tx_status_t status)
{
    mac_device_t *dev;
    ws_list_t *lptr;

    WS_DEBUG("got status (%u) for sqn (%u)\n", status, sqn);

    for (lptr = mac.device_list.next;
         lptr != &mac.device_list;
         lptr = lptr->next)
    {
        dev = ws_list_get_data(lptr, mac_device_t, list);
        if (dev->last_sqn == sqn)
        {
            if (CDATA(dev)->state == DEVICE_STATE_ASSOCIATING)
            {
                WS_DEBUG("device associated: %p\n", dev);
                CDATA(dev)->state = DEVICE_STATE_ASSOCIATED;

                if (coord.associate_cb != NULL)
                    coord.associate_cb(&dev->addr);
            }
            break;
        }
    }
}


void
mac_coordinator_send_data(ws_pktbuf_t *pkt)
{
    /* This input packet will be ready to go, we just need to read the
     * address and pend it for the right device */
    mac_fcf_t *fcf;
    mac_device_t *dev;
    ws_mac_addr_t dest;

    WS_DEBUG("sending data (pkt=%p\n", pkt);

    fcf = (mac_fcf_t *)ws_pktbuf_get_data(pkt);
    mac_frame_extract_address(fcf, &dest, NULL);
    dev = mac_device_get_by_addr(&dest);

    WS_DEBUG("device: %p\n", dev);

    if (dev != NULL && CDATA(dev)->state == DEVICE_STATE_ASSOCIATED)
    {
        if (CDATA(dev)->pending_data != NULL)
        {
            /* TODO: This warning assumes the device has a short address */
            WS_WARN("Overwritting previous data for device: %04x\n",
                    dest.short_addr);
            WS_DEBUG("dest\n");
            ws_pktbuf_destroy(CDATA(dev)->pending_data);
            CDATA(dev)->pending_data = NULL;
        }

        WS_DEBUG("pending data for: ");
        mac_frame_print_address(&dest);
        PRINTF("\n");

        CDATA(dev)->pending_data = pkt;

        WS_DEBUG("data: %r\n",
                 ws_pktbuf_get_data(CDATA(dev)->pending_data),
                 ws_pktbuf_get_len(CDATA(dev)->pending_data));
    }
    else
    {
        WS_WARN("can't send to device not currently associated\n");
    }
}


ws_pktbuf_t *
mac_coordinator_request_beacon(void)
{
    mac_fcf_t *fcf = NULL;
    uint8_t *ptr = NULL;
    ws_mac_addr_t src;

    mac_superframe_spec_t *spec = NULL;
    mac_gts_spec_t *gts_spec = NULL;
    mac_pending_addr_t *pending_addr = NULL;

    ws_list_t *lptr;
    mac_device_t *dev;
    uint8_t pending_count = 0;

    ws_pktbuf_reset(coord.beacon);

    ptr = ws_pktbuf_get_data(coord.beacon);
    ASSERT(ptr != NULL, "corrupted beacon pktbuf!\n");

    memset(ptr, 0, WS_RADIO_MAX_PACKET_LEN);

    fcf = (mac_fcf_t *)ptr;
    ptr = fcf->data;

    fcf->frame_type = MAC_FRAME_TYPE_BEACON;
    fcf->security_enabled = 0;
    fcf->frame_pending = 0;
    fcf->ack_req = 0;
    fcf->frame_version = WS_MAC_MAX_FRAME_VERSION;

    /* Sequence number */
    *ptr++ = coord.beacon_sqn++;

    /* Add our source address */
    ws_mac_mlme_get_address(&src);
    ptr += mac_frame_append_address(fcf, NULL, &src);

    /* Beacon header */
    spec = (mac_superframe_spec_t *)ptr;
    ptr = spec->data;
    spec->beacon_order = mac.beacon_order;
    spec->superframe_order = mac.superframe_order;
    /* TODO: This is part of GTS */
    spec->final_cap_slot = 15;
    spec->ble = 0;
    spec->pan_coordinator = mac.is_pan_coordinator;
    /* TODO: This is a MAC PIB attribute */
    spec->association_permit = 0;

    /* GTS descriptor */
    gts_spec = (mac_gts_spec_t *)ptr;
    ptr = gts_spec->data;
    gts_spec->descriptor_count = 0;
    gts_spec->gts_permit = 0;

    /* Pending addresses */
    pending_addr = (mac_pending_addr_t *)ptr;
    ptr = pending_addr->data;

    for (lptr = mac.device_list.next;
         lptr != &mac.device_list;
         lptr = lptr->next)
    {
        dev = ws_list_get_data(lptr, mac_device_t, list);
        if (CDATA(dev)->pending_data != NULL)
        {
            pending_count++;
            memcpy(ptr, &dev->addr.short_addr, 2);
            ptr += 2;
        }

        /* Maximum number of pending addresses */
        if (pending_count == 7)
            break;
    }

    pending_addr->short_addr_count = pending_count;
    pending_addr->extended_addr_count = 0;

    /* TODO: Add beacon payload if present (and if there's room!) */

    ws_pktbuf_increment_end(coord.beacon, (uint32_t)(ptr - (uint8_t *)fcf));

    return coord.beacon;
}


mac_device_t *
mac_coordinator_create_device(ws_mac_addr_t *ext_addr)
{
    mac_device_t *dev;

    WS_DEBUG("Creating new coordinator device\n");

    if (ext_addr->type != WS_MAC_ADDR_TYPE_EXTENDED)
    {
        WS_ERROR("unable to create device without extended address!\n");
        return NULL;
    }

    dev = (mac_device_t *)MALLOC(sizeof(mac_device_t) +
                                     sizeof(device_coord_data_t));
    if (dev == NULL)
    {
        WS_ERROR("Failed to allocate memory for device\n");
        return NULL;
    }

    ws_list_init(&dev->key_list);

    dev->addr.type = WS_MAC_ADDR_TYPE_EXTENDED;
    memcpy(dev->addr.extended_addr, ext_addr->extended_addr,
           WS_MAC_ADDR_TYPE_EXTENDED_LEN);

    return dev;
}


void
mac_coordinator_register_associate_callback(ws_mac_coordinator_associate_callback_t cb)
{
    coord.associate_cb = cb;
}


void
ws_mac_coordinator_register_callback(ws_mac_beacon_rx_callback_t cb)
{
    coord.rx_cb = cb;
}


void
ws_mac_coordinator_add_data(const uint8_t *data, uint8_t len)
{
    /* Add data to the end of a beacon frame. When the packet scheduler
     * asks us for a beacon, we'll put this data in if it fits. We'll only
     * copy it into the beacon once, but it might take a couple of attempts
     * if we're messing with GTS maintenence */
    (void)data;
    (void)len;
}
