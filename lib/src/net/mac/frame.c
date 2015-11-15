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
#define NO_DEBUG (1)
#endif

uint8_t
mac_frame_append_address(mac_fcf_t *fcf, ws_mac_addr_t *dest,
                         ws_mac_addr_t *src)
{
    uint8_t *ptr;

    ASSERT(fcf != NULL, "NULL FCF\n");

    /* FCF, SQN, Addressing */
    ptr = fcf->data;
    ptr++;

    if (dest != NULL)
    {
        /* Add the destination address */
        fcf->dest_addr_mode = dest->type;
        switch (fcf->dest_addr_mode)
        {
        case WS_MAC_ADDR_TYPE_NONE:
            break;

        case WS_MAC_ADDR_TYPE_SHORT:
            memcpy(ptr, &dest->pan_id, 2);
            ptr += 2;
            memcpy(ptr, &dest->short_addr, 2);
            ptr += 2;
            break;

        case WS_MAC_ADDR_TYPE_EXTENDED:
            memcpy(ptr, &dest->pan_id, 2);
            ptr += 2;
            memcpy(ptr, dest->extended_addr, WS_MAC_ADDR_TYPE_EXTENDED_LEN);
            ptr += WS_MAC_ADDR_TYPE_EXTENDED_LEN;
            break;

        default:
            WS_WARN("Unknown destination address\n");
            break;
        }
    }
    else
    {
        fcf->dest_addr_mode = WS_MAC_ADDR_TYPE_NONE;
    }

    if (src != NULL)
    {
        /* Add the source PAN ID if we need to */
        if (dest->type == WS_MAC_ADDR_TYPE_NONE ||
            dest->pan_id != src->pan_id)
        {
            fcf->pan_id_compression = 0;
            memcpy(ptr, &src->pan_id, 2);
            ptr += 2;
        }
        else
        {
            fcf->pan_id_compression = 1;
        }

        /* Add the destination address */
        fcf->src_addr_mode = src->type;
        switch (fcf->src_addr_mode)
        {
        case WS_MAC_ADDR_TYPE_NONE:
            break;

        case WS_MAC_ADDR_TYPE_SHORT:
            memcpy(ptr, &src->short_addr, 2);
            ptr += 2;
            break;

        case WS_MAC_ADDR_TYPE_EXTENDED:
            memcpy(ptr, src->extended_addr, WS_MAC_ADDR_TYPE_EXTENDED_LEN);
            ptr += WS_MAC_ADDR_TYPE_EXTENDED_LEN;
            break;

        default:
            WS_WARN("Unknown destination address\n");
            break;
        }
    }
    else
    {
        fcf->src_addr_mode = WS_MAC_ADDR_TYPE_NONE;
    }

    /* Return the number of bytes written */
    return (ptr - fcf->data - 1);
}


uint8_t *
mac_frame_extract_address(mac_fcf_t *fcf, ws_mac_addr_t *dest,
                          ws_mac_addr_t *src)
{
    uint8_t *ptr;
    uint16_t dest_pan;

    ASSERT(fcf != NULL, "NULL FCF\n");

    /* FCF, SQN, Addressing */
    ptr = fcf->data;
    ptr++;

    /* Extract destination PAN ID and address */
    switch (fcf->dest_addr_mode)
    {
    case WS_MAC_ADDR_TYPE_NONE:
        break;

    case WS_MAC_ADDR_TYPE_SHORT:
        memcpy(&dest_pan, ptr, 2);
        if (dest != NULL)
        {
            dest->type = fcf->dest_addr_mode;
            dest->pan_id = dest_pan;
            memcpy(&dest->short_addr, ptr+2, 2);
        }
        ptr += 4;
        break;

    case WS_MAC_ADDR_TYPE_EXTENDED:
        memcpy(&dest_pan, ptr, 2);
        if (dest != NULL)
        {
            dest->type = fcf->dest_addr_mode;
            dest->pan_id = dest_pan;
            memcpy(&dest->extended_addr, ptr+2, WS_MAC_ADDR_TYPE_EXTENDED_LEN);
        }
        ptr += 2 + WS_MAC_ADDR_TYPE_EXTENDED_LEN;
        break;

    default:
        WS_WARN("Unknown destination address (%u)\n", fcf->dest_addr_mode);
        break;
    }

    if (src != NULL)
    {
        /* Extract source PAN ID */
        if (!fcf->pan_id_compression)
        {
            memcpy(&src->pan_id, ptr, 2);
            ptr += 2;
        }
        else
        {
            src->pan_id = dest_pan;
        }
    }

    /* Extract source address */
    src->type = fcf->src_addr_mode;
    switch (fcf->src_addr_mode)
    {
    case WS_MAC_ADDR_TYPE_NONE:
        break;

    case WS_MAC_ADDR_TYPE_SHORT:
        if (src != NULL)
            memcpy(&src->short_addr, ptr, 2);
        ptr += 2;
        break;

    case WS_MAC_ADDR_TYPE_EXTENDED:
        if (src != NULL)
            memcpy(src->extended_addr, ptr, WS_MAC_ADDR_TYPE_EXTENDED_LEN);
        ptr += WS_MAC_ADDR_TYPE_EXTENDED_LEN;
        break;

    default:
        WS_WARN("Unknown source address (%u)\n", fcf->src_addr_mode);
        break;
    }

    /* Pointer to the data following the addresses */
    return ptr;
}


void
mac_frame_print_address(ws_mac_addr_t *addr)
{
    int i;

#ifndef NO_DEBUG
    switch (addr->type)
    {
    case WS_MAC_ADDR_TYPE_NONE:
        PRINTF("(none)");
        break;

    case WS_MAC_ADDR_TYPE_SHORT:
        PRINTF("(pan=%04x, addr=%04x)", addr->pan_id, addr->short_addr);
        break;

    case WS_MAC_ADDR_TYPE_EXTENDED:
        PRINTF("(pan=%04x, addr=", addr->pan_id);
        for (i = 0; i < WS_MAC_ADDR_TYPE_EXTENDED_LEN-1; i++)
            PRINTF("%02x:", addr->extended_addr[i]);
        PRINTF("%02x)", addr->extended_addr[i]);
        break;

    default:
        PRINTF("(unknown type %u)", addr->type);
        break;
    }
#endif
}


uint8_t *
mac_frame_get_data_ptr(mac_fcf_t *fcf, uint8_t *data_len)
{
    uint8_t *ptr;
    mac_security_control_t *sec_ctrl;

    WS_DEBUG("finding data pointer within: %r\n",
             (uint8_t *)fcf, *data_len);

    /* Skip over the FCF, SQN and addressing information */
    ptr = mac_frame_extract_address(fcf, NULL, NULL);

    if (fcf->security_enabled)
    {
        WS_DEBUG("message has security enabled\n");

        sec_ctrl = (mac_security_control_t *)ptr;
        ptr = sec_ctrl->data;

        if (sec_ctrl->key_id_mode != MAC_KEY_ID_MODE_IMPLICIT)
        {
            /* TODO: skip over the key source info */
            WS_ERROR("unable to parse key_id_mode: %u\n",
                     sec_ctrl->key_id_mode);
            return NULL;
        }

        ptr += 4; /* Frame Counter */

        *data_len -= (ptr - (uint8_t *)fcf);

        /* If this packet actually has data in it, then we can chop
         * off the MIC. */
        if (*data_len > 0)
        {
            /* TODO: Support other MIC lengths... */
            *data_len -=4;
        }
    }
    else
    {
        *data_len -= (ptr - (uint8_t *)fcf);
    }

    WS_DEBUG("found message (len=%u): %r\n", *data_len, ptr, *data_len);

    return ptr;
}
