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


static ws_mac_mcps_rx_callback_t rx_cb;
static ws_mac_mcps_confirm_callback_t confirm_cb;


static void
dispatch_packet(ws_pktbuf_t *pkt)
{
    WS_DEBUG("dispatching packet to be sent (ptr=%p,len=%u)\n",
             pkt, ws_pktbuf_get_len(pkt));
    switch (mac.state)
    {
    case MAC_STATE_COORDINATING:
        mac_coordinator_send_data(pkt);
        break;

    case MAC_STATE_ASSOCIATED:
        mac_packet_scheduler_send_data(pkt);
        break;

    default:
        WS_ERROR("unable to dispatch packet in state (%u)\n",
                 mac.state);
        WS_DEBUG("dest\n");
        ws_pktbuf_destroy(pkt);
        break;
    }
}


static void
pass_up_packet(ws_pktbuf_t *pkt)
{
    mac_fcf_t *fcf = (mac_fcf_t *)ws_pktbuf_get_data(pkt);
    uint8_t phy_len = ws_pktbuf_get_len(pkt);
    uint8_t *ptr;
    ws_mac_addr_t src;

    ASSERT(rx_cb != NULL, "receive callback altered!\n");

    mac_frame_extract_address(fcf, NULL, &src);

    ptr = mac_frame_get_data_ptr(fcf, &phy_len);
    WS_DEBUG("found data in frame: %r\n", ptr, phy_len);

    rx_cb(ptr, phy_len, &src);
    WS_DEBUG("dest\n");
    ws_pktbuf_destroy(pkt);
}


static void
enc_done(ws_pktbuf_t *pkt,
         mac_security_status_t status)
{
    mac_fcf_t *fcf;

    WS_DEBUG("encryption complete for packet (ptr=%p)\n", pkt);

    /* Encryption is done, so let's dispatch the packet */
    if (status != MAC_SECURITY_STATUS_SUCCESS)
    {
        WS_ERROR("security failed with status (%u)\n", status);
        if (confirm_cb != NULL)
        {
            fcf = (mac_fcf_t *)ws_pktbuf_get_data(pkt);
            /* TODO: Translate security error into confirm error */
            confirm_cb(fcf->data[0], WS_MAC_MCPS_UNSUPPORTED_SECURITY);
        }
        WS_DEBUG("dest\n");
        ws_pktbuf_destroy(pkt);
    }
    else
    {
        dispatch_packet(pkt);
    }
}


static void
dec_done(ws_pktbuf_t *pkt,
         mac_security_status_t status)
{
    if (status == MAC_SECURITY_STATUS_SUCCESS)
    {
        WS_DEBUG("successfully decrypted packet\n");
        pass_up_packet(pkt);
    }
    else
    {
        WS_ERROR("security failed with status (%u)\n", status);
        WS_DEBUG("dest\n");
        ws_pktbuf_destroy(pkt);
    }
}


static ws_pktbuf_t *
build_packet(const uint8_t *data, uint8_t len, ws_mac_addr_t *dest_addr,
             uint8_t sqn, bool secure)
{
    mac_fcf_t *fcf;
    uint8_t *ptr;
    ws_pktbuf_t *pkt;

    mac_security_status_t ret;

    ws_mac_addr_t src;

    pkt = ws_pktbuf_create(WS_RADIO_MAX_PACKET_LEN);
    if (pkt == NULL)
    {
        WS_ERROR("Failed to allocated pktbuf for packet\n");
        return NULL;
    }

    ptr = ws_pktbuf_get_data(pkt);
    memset(ptr, 0, ws_pktbuf_get_len(pkt));

    /* FCF */
    fcf = (mac_fcf_t *)ws_pktbuf_get_data(pkt);
    ptr = fcf->data;

    fcf->frame_type = MAC_FRAME_TYPE_DATA;
    fcf->security_enabled = secure;
    fcf->frame_pending = 0; /* TODO */
    fcf->ack_req = 1;
    fcf->frame_version = WS_MAC_MAX_FRAME_VERSION;

    /* Sequence number */
    *ptr++ = sqn;

    /* Add addressing information */
    WS_DEBUG("appending address\n");
    ws_mac_mlme_get_address(&src);
    ptr += mac_frame_append_address(fcf, dest_addr, &src);

    WS_DEBUG("sending message to: ");
    mac_frame_print_address(dest_addr);
    PRINTF("\n");

    /* The security supplicant will handle adding the data if we want
     * to create a secure frame. Otherwise we can add the data ourself */
    if (secure)
    {
        ret = mac_security_encrypt_frame(pkt, data, len, enc_done);
        if (ret != MAC_SECURITY_STATUS_IN_PROGRESS)
        {
            enc_done(pkt, ret);
        }
    }
    else
    {
        /* Data */
        memcpy(ptr, data, len);
        ptr += len;

        ws_pktbuf_increment_end(pkt, (uint32_t)(ptr - (uint8_t *)fcf));
    }

    WS_DEBUG("packet: %r\n",
             (uint8_t *)fcf, (ptr - (uint8_t *)fcf));

    return pkt;
}


void
mac_mcps_init(void)
{
    rx_cb = NULL;
    confirm_cb = NULL;
}


void
mac_mcps_handle_packet(ws_pktbuf_t *pkt)
{
    mac_fcf_t *fcf = (mac_fcf_t *)ws_pktbuf_get_data(pkt);

    mac_security_status_t ret;

    switch (fcf->frame_type)
    {
    case MAC_FRAME_TYPE_DATA:
        if (rx_cb == NULL)
        {
            WS_WARN("no receive callback for DATA packet\n");
            return;
        }

        if (fcf->security_enabled)
        {
            ret = mac_security_decrypt_frame(pkt, dec_done);
            if (ret != MAC_SECURITY_STATUS_IN_PROGRESS)
            {
                dec_done(pkt, ret);
            }
        }
        else
        {
            pass_up_packet(pkt);
        }
        break;
    }
}


void
mac_mcps_handle_status(uint8_t sqn, mac_tx_status_t status)
{
    if (confirm_cb == NULL)
        return;

    switch (status)
    {
    case MAC_TX_STATUS_SUCCESS:
        confirm_cb(sqn, WS_MAC_MCPS_SUCCESS);
        break;

    case MAC_TX_STATUS_NO_ACK:
        confirm_cb(sqn, WS_MAC_MCPS_NO_ACK);
        break;

    case MAC_TX_STATUS_NOT_SENT:
        confirm_cb(sqn, WS_MAC_MCPS_CHANNEL_ACCESS_FAILURE);
        break;

    default:
        WS_WARN("unhandled mac_tx_status (status=%u,sqn=%u)\n", status, sqn);
        break;
    }
}


void
ws_mac_mcps_register_rx_callback(ws_mac_mcps_rx_callback_t cb)
{
    rx_cb = cb;
}


void
ws_mac_mcps_register_confirm_callback(ws_mac_mcps_confirm_callback_t cb)
{
    confirm_cb = cb;
}


uint8_t
ws_mac_mcps_send_data(const uint8_t *data, uint8_t len,
                      ws_mac_addr_t *dest_addr,
                      bool secure)
{
    uint8_t handle;
    ws_pktbuf_t *pkt;

    if (mac.state != MAC_STATE_COORDINATING &&
        mac.state != MAC_STATE_ASSOCIATED)
    {
        WS_WARN("ignoring MCPS-DATA.request in state (%u)\n", mac.state);
        if (confirm_cb == NULL)
            confirm_cb(0, WS_MAC_MCPS_NOT_ALLOWED);
        return 0;
    }

    handle = mac_mlme_get_sqn();
    pkt = build_packet(data, len, dest_addr, handle, secure);

    /* If security is not enabled, then we don't need to wait for the
     * encryption to complete before dispatching the packet */
    if (!secure)
    {
        dispatch_packet(pkt);
    }

    /* TODO: We have no queuing in here. We should be able to call this
     * function until the queue is full, but currently only one message
     * can be passed to the supplicant at a time. Either we add a plaintext
     * queue in here, or we let the packet scheduler manage encryption - it
     * makes more sense than having these handlers for each upper MAC layer
     * anyway. */

    return handle;
}
