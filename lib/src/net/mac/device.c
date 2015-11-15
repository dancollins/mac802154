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


mac_device_t *
mac_device_get_by_short(uint16_t short_addr)
{
    ws_list_t *ptr;
    mac_device_t *dev;

    WS_DEBUG("searching for %u\n", short_addr);

    for (ptr = mac.device_list.next;
         ptr != &mac.device_list;
         ptr = ptr->next)
    {
        dev = ws_list_get_data(ptr, mac_device_t, list);
        WS_DEBUG("comparing to: %u\n", dev->addr.short_addr);
        if (dev->addr.short_addr == short_addr)
            return dev;
    }

    return NULL;
}


mac_device_t *
mac_device_get_by_extended(uint8_t *extended_addr)
{
    ws_list_t *ptr;
    mac_device_t *dev;

    WS_DEBUG("Searching for %r\n", extended_addr,
             WS_MAC_ADDR_TYPE_EXTENDED_LEN);

    for (ptr = mac.device_list.next;
         ptr != &mac.device_list;
         ptr = ptr->next)
    {
        dev = ws_list_get_data(ptr, mac_device_t, list);
        WS_DEBUG("comparing to: %r\n", dev->addr.extended_addr,
                 WS_MAC_ADDR_TYPE_EXTENDED_LEN);
        if (memcmp(dev->addr.extended_addr, extended_addr,
                   WS_MAC_ADDR_TYPE_EXTENDED_LEN) == 0)
            return dev;
    }

    return NULL;
}


mac_device_t *
mac_device_get_by_addr(ws_mac_addr_t *addr)
{
    WS_DEBUG("searching by address type: %u\n", addr->type);
    switch (addr->type)
    {
    case WS_MAC_ADDR_TYPE_SHORT:
        return mac_device_get_by_short(addr->short_addr);

    case WS_MAC_ADDR_TYPE_EXTENDED:
        return mac_device_get_by_extended(addr->extended_addr);

    default:
        WS_WARN("unknown address type\n");
        return NULL;
    }
}


mac_key_t *
mac_device_get_key(mac_device_t *dev, uint8_t index)
{
    ws_list_t *ptr;
    mac_key_t *key;

    for (ptr = dev->key_list.next;
         ptr != &dev->key_list;
         ptr = ptr->next)
    {
        key = ws_list_get_data(ptr, mac_key_t, list);
        if (key->index == index)
            return key;
    }

    return NULL;
}


void
mac_device_set_key(mac_device_t *dev, uint8_t index, uint8_t *key)
{
    mac_key_t *k = mac_device_get_key(dev, index);
    if (k != NULL)
    {
        WS_DEBUG("updating existing key (%u)\n", index);
        memcpy(k->key, key, WS_MAC_KEY_LEN);
        k->index = index;
    }
    else
    {
        k = (mac_key_t *)MALLOC(sizeof(mac_key_t));
        if (k == NULL)
        {
            WS_ERROR("failed to allocate memory for new key\n");
            return;
        }

        WS_DEBUG("adding new key (%u)\n", index);

        memcpy(k->key, key, WS_MAC_KEY_LEN);
        k->index = index;
        ws_list_add_after(&dev->key_list, &k->list);
    }
}


void
mac_device_remove_key(mac_device_t *dev, uint8_t index)
{
    mac_key_t *k = mac_device_get_key(dev, index);
    if (k != NULL)
    {
        WS_DEBUG("removing key (%u)\n", index);
        ws_list_remove(&k->list);
        FREE(k);
    }
    else
    {
        WS_WARN("no key (%u) to remove\n", index);
    }
}
