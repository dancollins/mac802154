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

#ifndef _MAC_PRIVATE_H
#define _MAC_PRIVATE_H


#include "wsn.h"


/* See IEE 802.15.4-2011 5.1.1.4
 * This value is the contention window length used for CSMA-CA */
#define MAC_CW_0 (2)


/* See IEEE 802.15.4-2011 6.4.2 Table 51 */
#define UNIT_BACKOFF_PERIOD (20)


/* See IEEE 802.15.4-2011 5.2.1.1 */
typedef struct __attribute__((packed))
{
    unsigned frame_type:3;          /* Frame type as per standard */
    unsigned security_enabled:1;    /* True is security is used */
    unsigned frame_pending:1;       /* True if sender has more data to send */
    unsigned ack_req:1;             /* Should this be ACK'd? */
    unsigned pan_id_compression:1;  /* Are the DST and SRC PAN IDs equal? */
    unsigned RESERVED:3;            /* Unused. Set to zero */
    unsigned dest_addr_mode:2;      /* DST address type */
    unsigned frame_version:2;       /* Standard version implemented */
    unsigned src_addr_mode:2;       /* SRC address type */
    uint8_t data[0];
} mac_fcf_t;


/* See IEEE 802.15.4-2011 5.2.1.1.1 */
typedef enum
{
    MAC_FRAME_TYPE_BEACON  = 0x00,
    MAC_FRAME_TYPE_DATA    = 0x01,
    MAC_FRAME_TYPE_ACK     = 0x02,
    MAC_FRAME_TYPE_MAC     = 0x03,
} mac_frame_type_t;


/* See IEEE 802.15.4-2011 5.2.2.1.2 */
typedef struct __attribute__((packed))
{
    unsigned beacon_order:4;        /* See 5.1.1.1  */
    unsigned superframe_order:4;    /* See 5.1.1.1  */
    unsigned final_cap_slot:4;      /* The last slot for the CAP */
    unsigned ble:1;                 /* See 5.2.2.1.2 */
    unsigned RESERVED:1;            /* Unused. Set to zero */
    unsigned pan_coordinator:1;     /* Set if beacon originated from the PAN
                                     * coordinator */
    unsigned association_permit:1;  /* Set if association is allowed */
    uint8_t data[0];
} mac_superframe_spec_t;


/* See IEEE 802.15.4-2011 5.2.2.1.3 */
typedef struct __attribute__((packed))
{
    unsigned descriptor_count:3;    /* Number of GTS list fields */
    unsigned RESERVED:4;            /* Unused. Set to zero */
    unsigned gts_permit:1;          /* True if devices can request GTS slots */
    uint8_t data[0];
} mac_gts_spec_t;


/* See IEEE 802.15.4-2011 5.2.2.1.6 */
typedef struct __attribute__((packed))
{
    unsigned short_addr_count:3;    /* Number of short addresses following */
    unsigned RESERVED0:1;           /* Unused. Set to zero */
    unsigned extended_addr_count:3; /* Number of extended addresses following */
    unsigned RESERVED1:1;           /* Unused. Set to zero */
    uint8_t data[0];
} mac_pending_addr_t;


/* See IEEE 802.15.4-2011 5.3 */
typedef enum
{
    MAC_COMMAND_ASSOCIATION_REQUEST   = 0x01,
    MAC_COMMAND_ASSOCIATION_RESPONSE  = 0x02,
    MAC_COMMAND_DISASSOC_REQUEST      = 0x03,
    MAC_COMMAND_DATA_REQUEST          = 0x04,
    MAC_COMMAND_PAN_ID_COLLECT_INFO   = 0x05,
    MAC_COMMAND_ORPHAN_NOTIFICATION   = 0x06,
    MAC_COMMAND_BEACON_REQUEST        = 0x07,
    MAC_COMMAND_COORDINATOR_REALIGN   = 0x08,
    MAC_COMMAND_GTS_REQUEST           = 0x09,
} mac_command_t;


/* See IEEE 802.15.4-2011 5.3.1.2 */
typedef struct __attribute__((packed))
{
    unsigned RESERVED0:1;           /* Unused. Set to zero */
    unsigned device_type:1;         /* Set for FFD */
    unsigned power_source:1;        /* Clear if battery powered */
    unsigned rx_when_idle:1;        /* Set if receiver is on while idle */
    unsigned RESERVED1:2;           /* Unused. Set to zero */
    unsigned security_capable:1;    /* Set if device supports security */
    unsigned allocate_addr:1;       /* Set to request a short address */
    uint8_t data[0];
} mac_capability_info_t;


/* See IEEE 802.15.4-2011 5.3.1.2 */
typedef enum
{
    MAC_DEVICE_TYPE_RFD  = 0x00,
    MAC_DEVICE_TYPE_FFD  = 0x01,
} mac_device_type_t;


/* See IEEE 802.15.4-2011 7.4.1 */
typedef struct __attribute__((packed))
{
    unsigned security_level:3; /* See 7.4.1.1 */
    unsigned key_id_mode:2; /* See 7.4.1.2 */
    unsigned RESERVED:3; /* Unused. Set to zero */
    uint8_t data[0];
} mac_security_control_t;


/* See IEEE 802.15.4-2011 7.4.1.1 Table 58 */
typedef enum
{
    MAC_SECURITY_LEVEL_NONE         = 0x00,
    MAC_SECURITY_LEVEL_MIC_32       = 0x01,
    MAC_SECURITY_LEVEL_MIC_64       = 0x02,
    MAC_SECURITY_LEVEL_MIC_128      = 0x03,
    MAC_SECURITY_LEVEL_ENC          = 0x04,
    MAC_SECURITY_LEVEL_ENC_MIC_32   = 0x05,
    MAC_SECURITY_LEVEL_ENC_MIC_64   = 0x06,
    MAC_SECURITY_LEVEL_ENC_MIC_128  = 0x07,
} mac_security_level_t;


/* See IEEE 802.15.4-2011 7.4.1.2 Table 59 */
typedef enum
{
    MAC_KEY_ID_MODE_IMPLICIT      = 0x00, /* Key source = dst/src addrs */
    MAC_KEY_ID_MODE_DEFAULT       = 0x01, /* Key source = macDefaultKeySource */
    MAC_KEY_ID_MODE_KEY_SOURCE_4  = 0x02, /* Key source = pan_id | short_addr */
    MAC_KEY_ID_MODE_KEY_SOURCE_8  = 0x03, /* Key source = ext_addr */
} mac_key_id_mode_t;


typedef struct
{
    ws_list_t list;
    ws_mac_addr_t addr;
    uint16_t pan_id;
    uint8_t last_sqn;
    uint32_t last_seen;
    uint32_t last_frame_ctr;
    bool sec_min_exempt;
    ws_list_t key_list;

    /* Data specific to coordinator operation. Non-coordinators should
     * ignore this. */
    uint8_t coord_data[0];
} mac_device_t;


typedef struct
{
    ws_list_t list;
    uint8_t index;
    uint8_t key[WS_MAC_KEY_LEN];
} mac_key_t;


typedef enum
{
    MAC_SECURITY_STATUS_SUCCESS,
    MAC_SECURITY_STATUS_IN_PROGRESS,
    MAC_SECURITY_STATUS_NO_KEY,
    MAC_SECURITY_STATUS_AES_ERROR,
    MAC_SECURITY_STATUS_BUSY,
    MAC_SECURITY_STATUS_ERROR,
} mac_security_status_t;



/*
 * MAC state
 */
typedef enum
{
    MAC_STATE_IDLE,
    MAC_STATE_SCANNING,
    MAC_STATE_ASSOCIATING,
    MAC_STATE_ASSOCIATED,
    MAC_STATE_COORDINATING,
} mac_state_t;

typedef struct
{
    mac_state_t state;
    bool is_pan_coordinator;

    /* If this is a device, then it will support having a hard coded AES
     * key (derived from MAC and PSK) */
    /* XXX: This is possibly an ugly hack! */
    uint8_t own_key[WS_MAC_KEY_LEN];

    /* MAC PIB */
    uint8_t extended_address[WS_MAC_ADDR_TYPE_EXTENDED_LEN];
    uint16_t short_address;
    uint16_t pan_id;
    uint8_t beacon_order;
    uint8_t superframe_order;
    uint32_t responseWaitTime;
    uint8_t coord_extended_address[WS_MAC_ADDR_TYPE_EXTENDED_LEN];
    uint16_t coord_short_address;
    bool batt_life_extension;
    uint8_t batt_life_extension_periods;
    uint8_t min_backoff_exponent;
    uint8_t max_backoff_exponent;
    uint8_t max_csma_backoffs;
    uint8_t sqn;
    ws_list_t device_list;
    uint8_t max_frame_retries;
    uint32_t frame_counter;

    /* PHY PIB */
    uint8_t current_channel;
} mac_t;

extern mac_t mac;


/* TODO: Add CSMA failure to this */
typedef enum
{
    /* Packet was sent and ACK was received */
    MAC_TX_STATUS_SUCCESS,
    /* Packet was sent, but acknowledge was not received */
    MAC_TX_STATUS_NO_ACK,
    /* Packet did not leave the radio */
    MAC_TX_STATUS_NOT_SENT,
} mac_tx_status_t;


/*
 * MCPS
 */
extern void
mac_mcps_init(void);


extern void
mac_mcps_handle_packet(ws_pktbuf_t *pkt);


extern void
mac_mcps_handle_status(uint8_t sqn, mac_tx_status_t status);


/*
 * MLME
 */
extern void
mac_mlme_init(void);


extern uint8_t
mac_mlme_get_sqn(void);


extern void
mac_mlme_send_association_request(ws_mac_addr_t *dest);


extern void
mac_mlme_send_beacon_request(void);


extern void
mac_mlme_send_data_request(ws_mac_addr_t *dest);


extern void
mac_mlme_handle_packet(ws_pktbuf_t *pkt);


extern void
mac_mlme_handle_status(uint8_t sqn, mac_tx_status_t status);


extern void
mac_mlme_scan_handle_packet(ws_pktbuf_t *pkt);


extern void
mac_mlme_association_handle_packet(ws_pktbuf_t *pkt);


extern void
mac_mlme_association_handle_status(uint8_t sqn, mac_tx_status_t status);


/*
 * Coordinator
 */
extern void
mac_coordinator_init(void);


extern void
mac_coordinator_handle_packet(ws_pktbuf_t *pkt);


extern void
mac_coordinator_handle_status(uint8_t sqn, mac_tx_status_t status);


extern void
mac_coordinator_send_data(ws_pktbuf_t *pkt);


extern ws_pktbuf_t *
mac_coordinator_request_beacon(void);


extern mac_device_t *
mac_coordinator_create_device(ws_mac_addr_t *ext_addr);

extern void
mac_coordinator_register_associate_callback(ws_mac_coordinator_associate_callback_t cb);


/*
 * Packet Scheduler
 */
extern void
mac_packet_scheduler_init(void);


extern void
mac_packet_scheduler_clear_receiver(void);


extern void
mac_packet_scheduler_sync(void);


extern void
mac_packet_scheduler_send_data(ws_pktbuf_t *pkt);


/*
 * Framing
 */
/**
 * Append the destination and source addresses to the frame. The function
 * has the logic to detect PAN ID compression.
 * \param fcf MAC frame header, along with room following for the addresses
 * \param dest destination address to append
 * \param src source address to append
 * \returns the number of octets appended
 */
extern uint8_t
mac_frame_append_address(mac_fcf_t *fcf, ws_mac_addr_t *dest,
                         ws_mac_addr_t *src);


/**
 * Extract addresses contained in the frame. Contains logic to detect
 * PAN ID compression
 * \param fcf MAC frame header
 * \param dest location to store the destination address
 * \param src location to store the source address
 * \returns pointer to the data following the addresses
 */
extern uint8_t *
mac_frame_extract_address(mac_fcf_t *fcf, ws_mac_addr_t *dest,
                          ws_mac_addr_t *src);


extern void
mac_frame_print_address(ws_mac_addr_t *addr);


extern uint8_t *
mac_frame_get_data_ptr(mac_fcf_t *fcf, uint8_t *data_len);


/*
 * Device
 */
extern mac_device_t *
mac_device_get_by_short(uint16_t short_addr);


extern mac_device_t *
mac_device_get_by_extended(uint8_t *extended_addr);


extern mac_device_t *
mac_device_get_by_addr(ws_mac_addr_t *addr);


extern mac_key_t *
mac_device_get_key(mac_device_t *dev, uint8_t index);


extern void
mac_device_set_key(mac_device_t *dev, uint8_t index, uint8_t *key);


extern void
mac_device_remove_key(mac_device_t *dev, uint8_t index);


/*
 * Security
 */
typedef void (*mac_security_status_callback_t)
(ws_pktbuf_t *pkt,
 mac_security_status_t status);


extern mac_security_status_t
mac_security_encrypt_frame(ws_pktbuf_t *frame,
                           const uint8_t *data, uint8_t data_len,
                           mac_security_status_callback_t cb);


extern mac_security_status_t
mac_security_decrypt_frame(ws_pktbuf_t *frame,
                           mac_security_status_callback_t cb);


#endif /* _MAC_PRIVATE_H */
