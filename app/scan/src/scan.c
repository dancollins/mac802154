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

#include "bsp.h"
#include "bsp_led.h"


#define TI_EUI_64_ADDR (0x00280028)
static uint8_t extended_address[8];


WS_TIMER_DECLARE(start_scan_timer);


static void
scan_callback(ws_mac_scan_status_t status,
              ws_mac_scan_type_t type,
              ws_list_t *scan_results)
{
    ws_list_t *ptr;
    ws_mac_scan_result_t *res;

    if (ws_list_count(scan_results) > 0)
    {
        ptr = scan_results->next;
        while (ptr != scan_results)
        {
            res = ws_list_get_data(ptr, ws_mac_scan_result_t, list);
            ptr = ptr->next;

            PRINTF("found beacon on channel %u with PAN ID %04x\n",
                   res->pan_desc.channel, res->pan_desc.addr.pan_id);
        }
    }
    else
    {
        PRINTF("nothing found during the scan :(\n");
    }

    WS_TIMER_SET_NOW(start_scan_timer);
}


static void
start_scan_timer(void)
{
    ws_mac_mlme_scan(WS_MAC_SCAN_TYPE_PASSIVE, 1, 4, scan_callback);
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

    PRINTF("\n\nScan Demo\n");

    read_eui_addr(extended_address);
    ws_mac_init(extended_address);

    WS_TIMER_SET_NOW(start_scan_timer);

    /* TODO: Start the OS main loop */
    return 0;
}
