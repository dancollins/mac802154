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

#include "ws_aes.h"

#include "aes.h"
#include "ccm.h"

#include "sys_ctrl.h"
#include "interrupt.h"

#if 1
#undef WS_DEBUG
#define WS_DEBUG(...)
#endif


typedef enum
{
    AES_STATE_IDLE,
    AES_STATE_ENCRYPTING,
    AES_STATE_DECRYPTING,
} aes_state_t;


typedef struct
{
    aes_state_t state;
    ws_aes_callback_t status_cb;
    uint8_t M;
    uint16_t m_len;
    uint8_t *c;
    uint16_t c_len;
    uint8_t tag[16];
} aes_t;

static aes_t aes;


WS_TIMER_DECLARE(aes_timer);


static void
aes_interrupt_handler(void)
{
    /* Clear the interrupt and set a timer to process the data */
    HWREG(AES_CTRL_INT_CLR) = AES_CTRL_INT_CLR_DMA_IN_DONE |
        AES_CTRL_INT_CLR_RESULT_AV;
    WS_TIMER_SET_NOW(aes_timer);
}


static void
aes_timer(void)
{
    WS_DEBUG("current state: (%u)\n", aes.state);

    switch (aes.state)
    {
    case AES_STATE_IDLE:
        break;

    case AES_STATE_ENCRYPTING:
        aes.state = AES_STATE_IDLE;

        if (aes.status_cb == NULL)
        {
            WS_WARN("nowhere to put the encrypted data!\n");
            return;
        }

        aes.c_len = aes.m_len + aes.M;

        if (CCMAuthEncryptGetResult(aes.M, aes.m_len, aes.tag) !=
            AES_SUCCESS)
        {
            aes.status_cb(WS_AES_STATUS_ENCRYPT_ERROR, NULL);
        }
        else
        {
            aes.status_cb(WS_AES_STATUS_SUCCESS, aes.tag);
        }
        break;

    case AES_STATE_DECRYPTING:
        aes.state = AES_STATE_IDLE;

        if (aes.status_cb == NULL)
        {
            WS_WARN("nowhere to put the encrypted data!\n");
            return;
        }

        if (CCMInvAuthDecryptGetResult(aes.M, aes.c, aes.c_len, aes.tag) !=
            AES_SUCCESS)
        {
            WS_ERROR("authentication failed.\n");
            WS_DEBUG("calculated tag: %r\n", aes.tag, aes.M);
            WS_DEBUG("c || T: %r\n", aes.c, aes.c_len);
            aes.status_cb(WS_AES_STATUS_ENCRYPT_ERROR, NULL);
        }
        else
        {
            WS_DEBUG("calculated tag: %r\n", aes.tag, aes.M);
            aes.status_cb(WS_AES_STATUS_SUCCESS, aes.tag);
        }
        break;

    default:
        WS_WARN("Unhandled state (%u)\n", aes.state);
        break;
    }
}


void
ws_aes_init(void)
{
    uint32_t version;

    WS_DEBUG("init\n");

    /* Enable AES clock */
    SysCtrlPeripheralReset(SYS_CTRL_PERIPH_AES);
    SysCtrlPeripheralEnable(SYS_CTRL_PERIPH_AES);

    /* Reset interrupts */
    IntDisable(INT_AES);
    HWREG(AES_CTRL_INT_EN) |= (AES_CTRL_INT_CLR_DMA_IN_DONE |
                               AES_CTRL_INT_CLR_RESULT_AV);

    /* Register interrupt handler */
    IntRegister(INT_AES, aes_interrupt_handler);

    /* Check AES core version numbers match what we expect */
    version = HWREG(AES_CTRL_VERSION);
    WS_DEBUG("version: %x.%x\n",
             (version & AES_CTRL_VERSION_MAJOR_VERSION_M) >>
             AES_CTRL_VERSION_MAJOR_VERSION_S,
             (version & AES_CTRL_VERSION_MINOR_VERSION_M) >>
             AES_CTRL_VERSION_MINOR_VERSION_S);
    WS_DEBUG("patch level: %x\n",
             (version & AES_CTRL_VERSION_PATCH_LEVEL_M) >>
             AES_CTRL_VERSION_PATCH_LEVEL_S);
    WS_DEBUG("EIP number: %02x, %02x\n",
             (version & AES_CTRL_VERSION_EIP_NUMBER_M) >>
             AES_CTRL_VERSION_EIP_NUMBER_S,
             (version & AES_CTRL_VERSION_EIP_NUMBER_COMPL_M) >>
             AES_CTRL_VERSION_EIP_NUMBER_COMPL_S);
    ASSERT(version == 0x91108778, "AES core version does not match!\n");
}


void
ws_aes_ccm_encrypt(bool encrypt,
                   uint8_t M, uint8_t L,
                   uint8_t *N,
                   uint8_t *m, uint8_t m_len,
                   uint8_t *a, uint8_t a_len,
                   uint8_t *key,
                   ws_aes_callback_t cb)
{
    uint8_t ret;

    /* TODO: Real return value */
    if (aes.state != AES_STATE_IDLE)
    {
        WS_ERROR("AES in use!\n");
        aes.status_cb(WS_AES_STATUS_KEY_WRITE_ERROR, NULL);
        aes.state = AES_STATE_IDLE;
        return;
    }

    memset(&aes, 0, sizeof(aes));
    aes.status_cb = cb;
    aes.M = M;
    aes.m_len = m_len;
    aes.state = AES_STATE_ENCRYPTING;

    /* Load the key into the key store */
    ret = AESLoadKey(key, KEY_AREA_0);
    if (ret != AES_SUCCESS)
    {
        if (aes.status_cb != NULL)
        {
            aes.status_cb(WS_AES_STATUS_KEY_WRITE_ERROR, NULL);
            aes.state = AES_STATE_IDLE;
            return;
        }
    }

    /* Perform CCM (we'll get an interrupt when the encryption is done,
     * and the authentication will be done when this function returns */
    ret = CCMAuthEncryptStart(encrypt,
                              M,
                              N,
                              m, (uint16_t)m_len,
                              a, (uint16_t)a_len,
                              KEY_AREA_0,
                              NULL,
                              L,
                              encrypt);
    if (ret != AES_SUCCESS)
    {
        if (aes.status_cb != NULL)
        {
            aes.status_cb(WS_AES_STATUS_ENCRYPT_ERROR, NULL);
            aes.state = AES_STATE_IDLE;
            return;
        }
    }
}


void
ws_aes_ccm_decrypt(bool decrypt,
                   uint8_t M, uint8_t L,
                   uint8_t *N,
                   uint8_t *c, uint8_t c_len,
                   uint8_t *a, uint8_t a_len,
                   uint8_t *key,
                   ws_aes_callback_t cb)
{
    uint8_t ret;

    /* TODO: Real return value */
    if (aes.state != AES_STATE_IDLE)
    {
        WS_ERROR("AES in use!\n");
        aes.status_cb(WS_AES_STATUS_KEY_WRITE_ERROR, NULL);
        aes.state = AES_STATE_IDLE;
        return;
    }

    memset(&aes, 0, sizeof(aes));
    aes.status_cb = cb;
    aes.M = M;
    aes.c = c;
    aes.c_len = c_len;
    aes.state = AES_STATE_DECRYPTING;

    /* Load the key into the key store */
    ret = AESLoadKey(key, KEY_AREA_0);
    if (ret != AES_SUCCESS)
    {
        if (aes.status_cb != NULL)
        {
            aes.status_cb(WS_AES_STATUS_KEY_WRITE_ERROR, NULL);
            aes.state = AES_STATE_IDLE;
            return;
        }
    }

    /* Perform Inv CCM (we'll get an interrupt when the decryption is done,
     * and the authentication will be done when this function returns */
    WS_DEBUG("starting decryption\n");
    ret = CCMInvAuthDecryptStart(decrypt,
                                 M,
                                 N,
                                 c, (uint16_t)c_len,
                                 a, (uint16_t)a_len,
                                 KEY_AREA_0,
                                 aes.tag,
                                 L,
                                 decrypt);
    if (ret != AES_SUCCESS)
    {
        if (aes.status_cb != NULL)
        {
            aes.status_cb(WS_AES_STATUS_ENCRYPT_ERROR, NULL);
            aes.state = AES_STATE_IDLE;
            return;
        }
    }
}
