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

#ifndef _WS_MAC_H
#define _WS_MAC_H


#include "wsn.h"


#define WS_MAC_ADDR_TYPE_EXTENDED_LEN (8)
#define WS_MAC_MAX_FRAME_VERSION (0x01)

/* This is the same for the PAN ID and Short Address. */
#define WS_MAC_BROADCAST_ADDR (0xffff)


/* See IEEE 802.15.4-2011 7.5 Table 61 */
#define WS_MAC_KEY_LEN (16) /* encryption key length in octets */


typedef enum
{
    WS_MAC_ADDR_TYPE_NONE = 0x00,
    WS_MAC_ADDR_TYPE_SHORT = 0x02,
    WS_MAC_ADDR_TYPE_EXTENDED = 0x03,
} ws_mac_addr_type_t;


typedef struct
{
    ws_mac_addr_type_t type;
    uint16_t pan_id;
    uint16_t short_addr;
    uint8_t extended_addr[8];
} ws_mac_addr_t;


typedef struct
{
    uint8_t beacon_order;
    uint8_t superframe_order;
    uint8_t final_cap_slot;
    bool batt_life_ext;
    bool pan_coordinator;
    bool association_permit;
} ws_mac_beacon_superframe_spec_t;


typedef struct
{
    ws_mac_addr_t addr;
    uint8_t channel;
    ws_mac_beacon_superframe_spec_t superframe_spec;
    bool gts_permit;
    uint8_t link_quality;
    uint32_t timestamp;
} ws_mac_pan_descriptor_t;


typedef enum
{
    WS_MAC_START_SUCCESS,
    WS_MAC_START_NO_SHORT_ADDRESS,
    WS_MAC_START_SUPERFRAME_OVERLAP,
    WS_MAC_START_TRACKING_OFF,
    WS_MAC_START_INVALID_PARAMETER,
    WS_MAC_START_COUNTER_ERROR,
    WS_MAC_START_FRAME_TOO_LONG,
    WS_MAC_START_UNAVAILABLE_KEY,
    WS_MAC_START_UNSUPPORTED_SECURITY,
    WS_MAC_START_CHANNEL_ACCESS_FAILURE,
} ws_mac_start_status_t;


typedef enum
{
    WS_MAC_SCAN_TYPE_ED,
    WS_MAC_SCAN_TYPE_ACTIVE,
    WS_MAC_SCAN_TYPE_PASSIVE,
    WS_MAC_SCAN_TYPE_ORPHAN,
} ws_mac_scan_type_t;


typedef enum
{
    WS_MAC_SCAN_SUCCESS,
    WS_MAC_SCAN_LIMIT_REACHED,
    WS_MAC_SCAN_NO_BEACON,
    WS_MAC_SCAN_IN_PROGRESS,
    WS_MAC_SCAN_COUNTER_ERROR,
    WS_MAC_SCAN_FRAME_TOO_LONG,
    WS_MAC_SCAN_UNAVAILABLE_KEY,
    WS_MAC_SCAN_UNSUPPORTED_SECURITY,
    WS_MAC_SCAN_INVALID_PARAMETER,
} ws_mac_scan_status_t;


typedef enum
{
    WS_MAC_ASSOCIATION_SUCCESS = 0x00,
    WS_MAC_ASSOCIATION_PAN_FULL,
    WS_MAC_ASSOCIATION_ACCESS_DENIED,
    WS_MAC_ASSOCIATION_CHANNEL_ACCESS_FAILURE = 0x80,
    WS_MAC_ASSOCIATION_NO_ACK,
    WS_MAC_ASSOCIATION_NO_DATA,
    WS_MAC_ASSOCIATION_COUNTER_ERROR,
    WS_MAC_ASSOCIATION_FRAME_TOO_LONG,
    WS_MAC_ASSOCIATION_IMPROPER_KEY_TYPE,
    WS_MAC_ASSOCIATION_IMPROPER_SECURITY_LEVEL,
    WS_MAC_ASSOCIATION_SECURITY_ERROR,
    WS_MAC_ASSOCIATION_UNAVAILABLE_KEY,
    WS_MAC_ASSOCIATION_UNSUPPORTED_LEGACY,
    WS_MAC_ASSOCIATION_UNSUPPORTED_SECURITY,
    WS_MAC_ASSOCIATION_INVALID_PARAMETER,
} ws_mac_association_status_t;


typedef enum
{
    WS_MAC_MCPS_SUCCESS,
    WS_MAC_MCPS_TRANSACTION_EXPIRED,
    WS_MAC_MCPS_TRANSACTION_OVERFLOW,
    WS_MAC_MCPS_CHANNEL_ACCESS_FAILURE,
    WS_MAC_MCPS_INVALID_ADDRESS,
    WS_MAC_MCPS_INVALID_GTS,
    WS_MAC_MCPS_NO_ACK,
    WS_MAC_MCPS_COUNTER_ERROR,
    WS_MAC_MCPS_FRAME_TOO_LONG,
    WS_MAC_MCPS_UNAVAILABLE_KEY,
    WS_MAC_MCPS_UNSUPPORTED_SECURITY,
    WS_MAC_MCPS_INVALID_PARAMETER,
    WS_MAC_MCPS_NOT_ALLOWED,
} ws_mac_mcps_status_t;


typedef struct
{
    ws_list_t list;

    /* Only one of these will be filled and depends on the scan type */
    uint8_t energy;
    ws_mac_pan_descriptor_t pan_desc;
} ws_mac_scan_result_t;



typedef void (*ws_mac_mcps_rx_callback_t)(const uint8_t *data, uint8_t len,
                                          ws_mac_addr_t *src_addr);


typedef void (*ws_mac_mcps_confirm_callback_t)(uint8_t handle,
                                               ws_mac_mcps_status_t status);


typedef void (*ws_mac_beacon_rx_callback_t)(uint8_t seq_num,
                                            ws_mac_pan_descriptor_t *pan_desc,
                                            bool data_pending,
                                            const uint8_t *beacon_data,
                                            uint8_t len);


typedef void (*ws_mac_coordinator_associate_callback_t)(ws_mac_addr_t *addr);


typedef void (*ws_mac_scan_callback_t)(ws_mac_scan_status_t status,
                                       ws_mac_scan_type_t type,
                                       ws_list_t *scan_results);


typedef void
(*ws_mac_association_callback_t)(ws_mac_association_status_t status,
                                 uint16_t short_addr);


extern void
ws_mac_init(uint8_t *extended_address);


/*
 * MCPS
 */
extern void
ws_mac_mcps_register_rx_callback(ws_mac_mcps_rx_callback_t cb);


extern void
ws_mac_mcps_register_confirm_callback(ws_mac_mcps_confirm_callback_t cb);


extern uint8_t
ws_mac_mcps_send_data(const uint8_t *data, uint8_t len,
                      ws_mac_addr_t *dest_addr,
                      bool secure);


/*
 * MLME
 */
extern void
ws_mac_mlme_scan(ws_mac_scan_type_t type, uint16_t channels,
                 uint8_t duration, ws_mac_scan_callback_t cb);


extern ws_mac_start_status_t
ws_mac_mlme_start(uint16_t pan_id, uint8_t channel,
                  uint8_t beacon_order, uint8_t superframe_order,
                  bool pan_coordinator,
                  ws_mac_coordinator_associate_callback_t associate_callback);


extern void
ws_mac_mlme_associate(ws_mac_pan_descriptor_t *pan,
                      ws_mac_association_callback_t cb);


extern uint16_t
ws_mac_mlme_get_short_address(void);


extern void
ws_mac_mlme_set_short_address(uint16_t addr);


extern uint8_t *
ws_mac_mlme_get_extended_address(void);


extern uint16_t
ws_mac_mlme_get_pan_id(void);


extern void
ws_mac_mlme_get_address(ws_mac_addr_t *addr);


/*
 * Coordinator
 */
extern void
ws_mac_coordinator_register_callback(ws_mac_beacon_rx_callback_t cb);


extern void
ws_mac_coordinator_add_data(const uint8_t *data, uint8_t len);


/*
 * Security Supplicant
 */
extern void
ws_mac_security_add_own_key(uint8_t *psk, uint16_t psk_len);

extern void
ws_mac_security_add_device_key(ws_mac_addr_t *addr,
                               uint8_t *psk, uint16_t psk_len);


#endif /*_WS_MAC_H */
