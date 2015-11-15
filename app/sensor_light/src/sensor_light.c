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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "wsn.h"

#include "hw_memmap.h"
#include "sys_ctrl.h"
#include "interrupt.h"
#include "io_pin_int.h"

#include "bsp.h"
#include "bsp_led.h"
#include "bsp_key.h"
#include "als_sfh5711.h"


/**
 * We average a number of samples over a period to low pass filter
 */
#define LIGHT_SAMPLES (8)
static int16_t light_samples[LIGHT_SAMPLES];
uint8_t light_sample_index = 0;


/**
 * The message structure for passing sensor information. This should REALLY
 * be defined externally (app/common).
 */
#define MSG_PREAMBLE (0xaa)

typedef enum
{
    MSG_ID_TEST_ENCRYPTION = 0,
    MSG_ID_LIGHT_SENSOR = 1,
    MSG_ID_KNOCK_SENSOR = 2,
} msg_id_t;


typedef struct __attribute__((packed))
{
    unsigned preamble:8;
    unsigned id:8;
    unsigned value:16;
} msg_t;


#define TI_EUI_64_ADDR (0x00280028)
static uint8_t extended_address[8];


#define STATE_LIST \
    X(APP_STATE_INIT),                      \
    X(APP_STATE_SCANNING),                  \
    X(APP_STATE_ASSOCIATING),               \
    X(APP_STATE_AUTHENTICATING),            \
    X(APP_STATE_CONNECTED),                 \
    X(APP_STATE_SENDING),

#define X(name) name
typedef enum
{
    STATE_LIST
} app_state_t;
#undef X


#define X(name) #name
static const char *app_state_names[] = {
    STATE_LIST
};
#undef X

/** Keeps track of the current app state */
static app_state_t app_state;

WS_TIMER_DECLARE(connected_led_timer);
static bool connected_led = false;

/** Forward declaration of our change state function */
static void
set_state(app_state_t new_state);


/** Message handle of our last sent message */
static uint8_t msg_handle = 0;


/** Our PAN coordinator details */
static ws_mac_pan_descriptor_t pan;


/* ---------------------------------------------------------------------
 *   WSN
 * --------------------------------------------------------------------- */
WS_TIMER_DECLARE(association_timer);
WS_TIMER_DECLARE(test_encryption_timer);

static void
receive_handler(const uint8_t *data, uint8_t len,
                ws_mac_addr_t *src_addr)
{
    msg_t *msg;

    if (len != sizeof(msg_t))
    {
        PRINTF("received message of invalid length: %u\n", len);
        return;
    }

    msg = (msg_t *)data;
    if (msg->preamble != MSG_PREAMBLE)
    {
        PRINTF("invalid preamble: %u\n", msg->preamble);
        return;
    }

    switch (app_state)
    {
    case APP_STATE_AUTHENTICATING:
        if (msg->id == MSG_ID_TEST_ENCRYPTION)
        {
            PRINTF("successfully authenticated!\n");
            connected_led = true;
            WS_TIMER_SET(connected_led_timer, 5000);
            set_state(APP_STATE_CONNECTED);
        }
        else
        {
            PRINTF("invalid message ID in authentication state: %u\n",
                   msg->id);
        }
        break;

    default:
        PRINTF("ignoring message in state: %u\n", app_state);
        break;
    }
}


static void
confirm_handler(uint8_t handle, ws_mac_mcps_status_t status)
{
    if (handle != msg_handle)
        return;

    PRINTF("received confirm\n");

    switch (app_state)
    {
    case APP_STATE_AUTHENTICATING:
        if (status == WS_MAC_MCPS_SUCCESS)
        {
            /* Timeout after 5 seconds */
            WS_TIMER_SET(test_encryption_timer, 5000);
        }
        else
        {
            /* Sending failed, so let's just try again */
            WS_TIMER_SET_NOW(test_encryption_timer);
        }
        break;

    case APP_STATE_SENDING:
        if (status == WS_MAC_MCPS_SUCCESS)
            PRINTF("message sent!\n");
        else
            PRINTF("message failed with status: %u\n", status);

        set_state(APP_STATE_CONNECTED);
        break;

    default:
        PRINTF("received confirm in invalid state: %u\n",
               app_state);
        break;
    }
}


static void
scan_callback(ws_mac_scan_status_t status,
              ws_mac_scan_type_t type,
              ws_list_t *scan_results)
{
    ws_list_t *ptr;
    ws_mac_scan_result_t *res;

    if (ws_list_count(scan_results) == 0)
    {
        PRINTF("nothing found during the scan :(\n");
        return;
    }

    ptr = scan_results->next;
    while (ptr != scan_results)
    {
        res = ws_list_get_data(ptr, ws_mac_scan_result_t, list);
        ptr = ptr->next;

        PRINTF("found beacon on channel %u with PAN ID %04x\n",
               res->pan_desc.channel, res->pan_desc.addr.pan_id);
    }

    /* Try to associate with the first one we find */
    res = ws_list_get_data(scan_results->next, ws_mac_scan_result_t, list);
    memcpy(&pan, &res->pan_desc, sizeof(pan));
    WS_TIMER_SET_NOW(association_timer);

    set_state(APP_STATE_ASSOCIATING);
}


static void
associate_callback(ws_mac_association_status_t status,
                   uint16_t short_addr)
{
    switch (status)
    {
    case WS_MAC_ASSOCIATION_SUCCESS:
        PRINTF("associated to PAN %04x. Address is %04x\n",
               pan.addr.pan_id, short_addr);
        WS_TIMER_SET_NOW(test_encryption_timer);

        set_state(APP_STATE_AUTHENTICATING);
        break;

    case WS_MAC_ASSOCIATION_NO_ACK:
    case WS_MAC_ASSOCIATION_NO_DATA:
        PRINTF("coorindator stopped responding. Retrying.\n");
        WS_TIMER_SET_NOW(association_timer);
        break;

    default:
        PRINTF("association failed with status %u\n", status);
    }
}


static void
association_timer(void)
{
    if (app_state != APP_STATE_ASSOCIATING)
    {
        PRINTF("attempting to associated in invalid state: %u\n",
               app_state);
        return;
    }

    PRINTF("associating with %04x\n", pan.addr.pan_id);

    ws_mac_mlme_associate(&pan, associate_callback);
}


static void
test_encryption_timer(void)
{
    msg_t msg;

    if (app_state != APP_STATE_AUTHENTICATING)
    {
        PRINTF("testing encryption in invalid state!\n");
        return;
    }

    PRINTF("testing link encryption\n");

    msg.preamble = MSG_PREAMBLE;
    msg.id = MSG_ID_TEST_ENCRYPTION;
    msg.value = 0;

    msg_handle = ws_mac_mcps_send_data((uint8_t *)&msg, sizeof(msg),
                                       &pan.addr, true);
}


/* ---------------------------------------------------------------------
 *   Application
 * --------------------------------------------------------------------- */
WS_TIMER_DECLARE(measure_light_timer);
WS_TIMER_DECLARE(send_light_message_timer);
WS_TIMER_DECLARE(state_led_timer);

static void
measure_light_timer(void)
{
    msg_t msg;

    /* Take a reading */
    light_samples[light_sample_index++] = alsRead();
    if (light_sample_index >= LIGHT_SAMPLES)
    {
        light_sample_index = 0;
        WS_TIMER_SET_NOW(send_light_message_timer);
    }

    /* Ensure we take enough samples to average every second */
    WS_TIMER_SET(measure_light_timer, 1000/LIGHT_SAMPLES);
}


static void
send_light_message_timer(void)
{
    msg_t msg;
    int16_t avg_light_level = 0;

    /* Average the light samples */
    for (int i = 0; i < LIGHT_SAMPLES; i++)
    {
        /* Max light value seems to be around 2048 so we just need to
         * ensure LIGHT_SAMPLES < ~15 */
        avg_light_level += light_samples[i];
    }
    avg_light_level /= LIGHT_SAMPLES;
    PRINTF("average light level: %d\n", avg_light_level);

    if (app_state == APP_STATE_CONNECTED)
    {
        PRINTF("sending light level\n");

        msg.preamble = MSG_PREAMBLE;
        msg.id = MSG_ID_LIGHT_SENSOR;
        msg.value = (uint16_t)avg_light_level;

        /* Send the message to the coordinator */
        PRINTF("sending: % r\n", (uint8_t *)&msg, sizeof(msg));
        msg_handle = ws_mac_mcps_send_data((uint8_t *)&msg, sizeof(msg),
                                           &pan.addr, true);
    }
}


static void
connected_led_timer(void)
{
    connected_led = false;
    bspLedClear(BSP_LED_3);
}


static void
state_led_timer(void)
{
    switch(app_state)
    {
    case APP_STATE_INIT:
        bspLedSet(BSP_LED_1);
        break;

    case APP_STATE_SCANNING:
        bspLedToggle(BSP_LED_1);
        WS_TIMER_SET(state_led_timer, 200);
        break;

    case APP_STATE_ASSOCIATING:
    case APP_STATE_AUTHENTICATING:
        bspLedClear(BSP_LED_1);
        bspLedToggle(BSP_LED_2);
        WS_TIMER_SET(state_led_timer, 200);
        break;

    case APP_STATE_CONNECTED:
        bspLedClear(BSP_LED_1);
        bspLedClear(BSP_LED_2);
        if (connected_led)
            bspLedSet(BSP_LED_3);
        else
            bspLedClear(BSP_LED_3);
        break;

    default:
        bspLedClear(BSP_LED_1);
        bspLedClear(BSP_LED_2);
        break;
    }
}


static void
set_state(app_state_t new)
{
    PRINTF("new app state: %s\n", app_state_names[new]);
    app_state = new;

    WS_TIMER_SET_NOW(state_led_timer);
}


/* ---------------------------------------------------------------------
 *   Initialisation
 * --------------------------------------------------------------------- */
static void
board_init(void)
{
    uint8_t val;

    /* Initialise the board */
    bspInit(BSP_SYS_CLK_SPD);

    alsInit();

    IntMasterEnable();
}


static void
read_eui_addr(uint8_t *buf)
{
    /* TODO: This works on the boards I have, but some TI parts have different
     * byte ordering. Contiki has a better way (based on finding the TI OUI)
     * that could be copied */
    memcpy(buf, (uint32_t *)TI_EUI_64_ADDR, 8);
}


int
main(void)
{
    app_state = APP_STATE_INIT;
    memset(&light_samples, 0, sizeof(light_samples));

    board_init();

    /* TODO: Initialise the OS */

    PRINTF("\n\nSensor demo: light\n");

    read_eui_addr(extended_address);
    ws_mac_init(extended_address);

    ws_mac_security_add_own_key((uint8_t *)"cheddar", strlen("cheddar"));

    ws_aes_init();

    ws_mac_mcps_register_rx_callback(receive_handler);
    ws_mac_mcps_register_confirm_callback(confirm_handler);

    set_state(APP_STATE_SCANNING);
    ws_mac_mlme_scan(WS_MAC_SCAN_TYPE_PASSIVE, 1, 4, scan_callback);

    WS_TIMER_SET_NOW(measure_light_timer);

    /* TODO: Start the OS main loop */
    return 0;
}
