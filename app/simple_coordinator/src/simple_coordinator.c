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

#include "wsn.h"

#include "hw_memmap.h"
#include "sys_ctrl.h"
#include "interrupt.h"

#include "bsp.h"
#include "bsp_led.h"


#ifndef PHY_CHANNEL
#define PHY_CHANNEL (11)
#endif


#define TI_EUI_64_ADDR (0x00280028)

static uint8_t extended_address[8];


/* ---------------------------------------------------------------------
 *   WSN
 * --------------------------------------------------------------------- */
static void
receive_handler(const uint8_t *data, uint8_t len,
                ws_mac_addr_t *src_addr)
{
    PRINTF("received message from: %04x\n", src_addr->short_addr);
    PRINTF("  % r\n", data, len);
}

static void
confirm_handler(uint8_t handle, ws_mac_mcps_status_t status)
{
    /* We don't send any data, so wouldn't expect to get this callback */
    PRINTF("Got MCPS-DATA.confirm (status=%u,handle=%u)\n", status, handle);
}


static void
associate_handler(ws_mac_addr_t *addr)
{
    PRINTF("New device associated: %04x\n", addr->short_addr);
}



/* ---------------------------------------------------------------------
 *   Application
 * --------------------------------------------------------------------- */
WS_TIMER_DECLARE(blinky_timer);
static void
blinky_timer(void)
{
    bspLedToggle(BSP_LED_1);
    WS_TIMER_SET(blinky_timer, 200);
}


static void
read_eui_addr(uint8_t *buf)
{
    /* TODO: This works on the boards I have, but some TI parts have different
     * byte ordering. Contiki has a better way (based on finding the TI OUI)
     * that could be copied */
    memcpy(buf, (uint32_t *)TI_EUI_64_ADDR, 8);
}


static void
board_init(void)
{
    /* Initialise the board */
    bspInit(BSP_SYS_CLK_SPD);

    IntMasterEnable();
}


int
main(void)
{
    board_init();

    /* TODO: Initialise the OS */

    PRINTF("\n\nSimple Coordinator\n");

    read_eui_addr(extended_address);
    ws_mac_init(extended_address);

    /* TODO: Do we need AES for this demo? We don't support encrypted
     * data anyway... */
    ws_aes_init();

    ws_mac_mcps_register_rx_callback(receive_handler);
    ws_mac_mcps_register_confirm_callback(confirm_handler);

    ws_mac_mlme_set_short_address(0xaabb);
    ws_mac_mlme_start(0xdc00, PHY_CHANNEL, 5, 4, true, associate_handler);

    WS_TIMER_SET_NOW(blinky_timer);

    /* TODO: Start the OS main loop */
    return 0;
}
