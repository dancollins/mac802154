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


/* These AES parameters are defined in IEEE 802.15.4-2011 and have an effect
 * on the variable sizes used throughout this supplicant. Making the functions
 * less generic (setting these values at compile time) reduces the memory
 * footprint and complexity of this supplicant */
#define AES_L (2)
#define MAC_NONSE_LEN (13)

#define SECURITY_BUF_LEN (256)


#if 1
#undef WS_DEBUG
#define WS_DEBUG(...)
#endif


typedef enum
{
    SECURITY_STATE_IDLE,
    SECURITY_STATE_ENCRYPTING,
    SECURITY_STATE_DECRYPTING,
} security_state_t;


typedef struct
{
    security_state_t state;

    mac_security_status_callback_t cb;

    ws_pktbuf_t *pkt;
    mac_device_t *dev;
    mac_key_t *key;

    uint8_t nonse[MAC_NONSE_LEN];
    uint8_t buf[SECURITY_BUF_LEN];
    uint8_t buf_len;
} security_t;

static security_t sec;


static void
prepare_nonse(uint32_t frame_counter, uint8_t *ext_src_addr,
              mac_security_level_t sec_level)
{
    /* N = src_addr_ext || frame_ctr || sec_level */
    uint8_t offset = 0;

    int i;

    /* Swap the endianness of the address */
    for (i = 0; i < WS_MAC_ADDR_TYPE_EXTENDED_LEN; i++)
    {
        sec.nonse[i] =
            ext_src_addr[WS_MAC_ADDR_TYPE_EXTENDED_LEN-1-i];
    }

    offset += WS_MAC_ADDR_TYPE_EXTENDED_LEN;

    memcpy(&sec.nonse[offset], &frame_counter, 4);
    offset += 4;

    memcpy(&sec.nonse[offset], &sec_level, 1);
}


static void
aes_cb(ws_aes_status_t status, uint8_t *MIC)
{
    mac_fcf_t *fcf = (mac_fcf_t *)ws_pktbuf_get_data(sec.pkt);
    uint8_t len = ws_pktbuf_get_len(sec.pkt);
    uint8_t *ptr;

    WS_DEBUG("AES status (%u)\n", status);

    ASSERT(sec.cb != NULL, "unable to pass status back to caller!\n");

    if (status != WS_AES_STATUS_SUCCESS)
    {
        WS_ERROR("AES error.\n");
        sec.cb(sec.pkt, MAC_SECURITY_STATUS_AES_ERROR);
    }
    else
    {
        if (sec.state == SECURITY_STATE_ENCRYPTING)
        {
            WS_DEBUG("getting data ptr from pkt of len %u\n", len);
            ptr = mac_frame_get_data_ptr(fcf, &len);
            WS_DEBUG("ciphertext: %r\n", sec.buf, sec.buf_len);
            WS_DEBUG("tag: %r\n", MIC, 4);
            memcpy(ptr, sec.buf, sec.buf_len);
            ptr += sec.buf_len;
            memcpy(ptr, MIC, 4);
            ws_pktbuf_increment_end(sec.pkt, sec.buf_len + 4);
            sec.cb(sec.pkt, MAC_SECURITY_STATUS_SUCCESS);
        }
        else if (sec.state == SECURITY_STATE_DECRYPTING)
        {
            /* XXX: The TI library does the comparison of the tag to the
             * message for us. */
            WS_DEBUG("calculated MIC: %r\n", MIC, 4);

            ptr = mac_frame_get_data_ptr(fcf, &len);

            /* XXX: This packet wont change length, as l(m) == l(c).
             * We'll keep the tag on the end. */
            memcpy(ptr, sec.buf, sec.buf_len);
            sec.cb(sec.pkt, MAC_SECURITY_STATUS_SUCCESS);
        }
        else
        {
            WS_ERROR("invalid security state in AES callback!\n");
            sec.cb(sec.pkt, MAC_SECURITY_STATUS_ERROR);
        }
    }

    sec.state = SECURITY_STATE_IDLE;
    sec.pkt = NULL;
}


static mac_security_status_t
prepare_state(ws_pktbuf_t *pkt,
              mac_security_status_callback_t cb)
{
    mac_fcf_t *fcf = (mac_fcf_t *)ws_pktbuf_get_data(pkt);

    if (sec.state != SECURITY_STATE_IDLE)
    {
        WS_WARN("supplicant busy!\n");
        return MAC_SECURITY_STATUS_BUSY;
    }

    /* We should have tidied this up earlier, so we either got into a funny
     * state, or we didn't tidy it up */
    ASSERT(sec.pkt == NULL, "unclean supplicant state\n");

    /* Security supplicant doesn't do anything useful without the callback */
    ASSERT(cb != NULL, "callback is required\n");

    /* Ensure the security bit is set in the FCF */
    if (!fcf->security_enabled)
    {
        /* This check should be done by caller */
        WS_WARN("security not enabled for frame\n");
        return MAC_SECURITY_STATUS_ERROR;
    }

    /* Set up the supplicant state */
    memset(&sec, 0, sizeof(sec));
    sec.pkt = pkt;
    sec.cb = cb;

    return MAC_SECURITY_STATUS_SUCCESS;
}


static mac_security_status_t
prepare_key(ws_mac_addr_t *other_device)
{
    /* Find the device */
    sec.dev = mac_device_get_by_addr(other_device);
    if (sec.dev == NULL)
    {
        WS_ERROR("unknown device\n");
        return MAC_SECURITY_STATUS_NO_KEY;
    }

    /* Load the device key */
    sec.key = mac_device_get_key(sec.dev, 0);
    if (sec.key == NULL)
    {
        WS_ERROR("no key for device\n");
        return MAC_SECURITY_STATUS_NO_KEY;
    }

    return MAC_SECURITY_STATUS_SUCCESS;
}


static void
derive_key_from_psk(uint8_t *key, const uint8_t *psk, uint16_t psk_len)
{
    /* TODO: This is clearly a hack, and should not be used for real security!
     * I have proposed a better model in my thesis, however time constraints
     * mean this wasn't implemented. */
    uint16_t len = psk_len < WS_MAC_KEY_LEN ? psk_len : WS_MAC_KEY_LEN;
    memset(key, 0, WS_MAC_KEY_LEN);
    memcpy(key, psk, len);
}


mac_security_status_t
mac_security_encrypt_frame(ws_pktbuf_t *frame,
                           const uint8_t *data, uint8_t data_len,
                           mac_security_status_callback_t cb)
{
    mac_fcf_t *fcf = (mac_fcf_t *)ws_pktbuf_get_data(frame);
    uint8_t *ptr;
    ws_mac_addr_t dst;

    mac_security_status_t ret;

    mac_security_control_t *sec_ctrl;

    uint8_t *a;
    uint8_t a_len;

    ret = prepare_state(frame, cb);
    if (ret != MAC_SECURITY_STATUS_SUCCESS)
    {
        WS_ERROR("failed to prepare supplicant state\n");
        return ret;
    }

    sec.state = SECURITY_STATE_ENCRYPTING;

    ptr = mac_frame_extract_address(fcf, &dst, NULL);
    WS_DEBUG("encrypting data for ");
    mac_frame_print_address(&dst);
    PRINTF("\n");

    ret = prepare_key(&dst);
    if (ret != MAC_SECURITY_STATUS_SUCCESS)
    {
        WS_ERROR("failed to load device key\n");
        sec.state = SECURITY_STATE_IDLE;
        sec.pkt = NULL;
        return ret;
    }

    /* Create the auxillary security header */
    sec_ctrl = (mac_security_control_t *)ptr;
    sec_ctrl->security_level = MAC_SECURITY_LEVEL_ENC_MIC_32;
    sec_ctrl->key_id_mode = MAC_KEY_ID_MODE_IMPLICIT;
    ptr += sizeof(mac_security_control_t);

    /* Copy the frame counter in the correct endianness */
    ptr[0] = ((uint8_t *)(&mac.frame_counter))[3];
    ptr[1] = ((uint8_t *)(&mac.frame_counter))[2];
    ptr[2] = ((uint8_t *)(&mac.frame_counter))[1];
    ptr[3] = ((uint8_t *)(&mac.frame_counter))[0];
    ptr += 4;

    /* The pktbuf should now hold the MAC header including the security
     * header, but no data yet */
    ws_pktbuf_increment_end(sec.pkt, (uint32_t)(ptr - (uint8_t *)fcf));

    /* Create the nonse */
    prepare_nonse(mac.frame_counter, mac.extended_address,
                  MAC_SECURITY_LEVEL_ENC_MIC_32);

    /* Important that the frame counter is the same for the MHR and the
     * nonse, so we increment after everything has used the frame counter
     */
    mac.frame_counter++;

    /* Point to the auth data */
    a = (uint8_t *)fcf;
    a_len = (uint8_t)(ptr - a);


    /* Copy the message into our data buffer */
    /* TODO: We should use the pktbuf, rather than a seperate buffer... */
    memset(sec.buf, 0, 256);
    memcpy(sec.buf, data, data_len);
    sec.buf_len = data_len;

    /* Debug info. This is really useful for comparing the encryption
     * output against a reference implementation */
    WS_DEBUG("plaintext: %r\n", sec.buf, sec.buf_len);
    WS_DEBUG("message len %u\n", sec.buf_len);
    WS_DEBUG("a: %r\n", a, a_len);
    WS_DEBUG("a len %u\n", a_len);
    WS_DEBUG("nonse: %r\n", sec.nonse, MAC_NONSE_LEN);
    WS_DEBUG("M: %u\n", 4);
    WS_DEBUG("L: %u\n", AES_L);
    WS_DEBUG("key: %r\n", sec.key->key, WS_MAC_KEY_LEN);
    WS_DEBUG("extended addr: %r\n", mac.extended_address,
             WS_MAC_ADDR_TYPE_EXTENDED_LEN);

    /* Perform the encryption */
    ws_aes_ccm_encrypt(true, 4, AES_L, sec.nonse,
                       sec.buf, sec.buf_len,
                       a, a_len,
                       sec.key->key,
                       aes_cb);

    return MAC_SECURITY_STATUS_IN_PROGRESS;
}

mac_security_status_t
mac_security_decrypt_frame(ws_pktbuf_t *frame,
                           mac_security_status_callback_t cb)
{
    mac_fcf_t *fcf = (mac_fcf_t *)ws_pktbuf_get_data(frame);
    uint8_t *ptr;
    ws_mac_addr_t src;

    mac_security_status_t ret;

    mac_security_control_t *sec_ctrl;
    uint32_t frame_counter;

    uint8_t *a;
    uint8_t a_len;

    ret = prepare_state(frame, cb);
    if (ret != MAC_SECURITY_STATUS_SUCCESS)
    {
        WS_ERROR("failed to prepare supplicant state\n");
        return ret;
    }

    sec.state = SECURITY_STATE_DECRYPTING;

    ptr = mac_frame_extract_address(fcf, NULL, &src);
    WS_DEBUG("decrypting data from ");
    mac_frame_print_address(&src);
    PRINTF("\n");

    ret = prepare_key(&src);
    if (ret != MAC_SECURITY_STATUS_SUCCESS)
    {
        WS_ERROR("failed to load device key\n");
        sec.state = SECURITY_STATE_IDLE;
        sec.pkt = NULL;
        return ret;
    }

    /* Parse the security header */
    /* TODO: Support more of the options... */
    sec_ctrl = (mac_security_control_t *)ptr;
    if (sec_ctrl->security_level != MAC_SECURITY_LEVEL_ENC_MIC_32)
    {
        /* TODO... */
        WS_ERROR("Unsupported security level (%u)\n",
                 sec_ctrl->security_level);
        sec.state = SECURITY_STATE_IDLE;
        return MAC_SECURITY_STATUS_ERROR;
    }
    if (sec_ctrl->key_id_mode != MAC_KEY_ID_MODE_IMPLICIT)
    {
        /* TODO... */
        WS_ERROR("Unsupported key ID mode (%u)\n",
                 sec_ctrl->key_id_mode);
        sec.state = SECURITY_STATE_IDLE;
        return MAC_SECURITY_STATUS_ERROR;
    }
    ptr += sizeof(mac_security_control_t);

    /* Copy the frame counter in the correct endianness */
    ((uint8_t *)&frame_counter)[0] = ptr[3];
    ((uint8_t *)&frame_counter)[1] = ptr[2];
    ((uint8_t *)&frame_counter)[2] = ptr[1];
    ((uint8_t *)&frame_counter)[3] = ptr[0];
    ptr += 4;

    /* Create the nonse */
    prepare_nonse(frame_counter, sec.dev->addr.extended_addr,
                  MAC_SECURITY_LEVEL_ENC_MIC_32);

    /* Point to the authentication data */
    a = (uint8_t *)fcf;
    a_len =  ptr - ((uint8_t *)fcf);

    /* Copy the ciphertext from the message into the buffer */
    sec.buf_len = ws_pktbuf_get_len(sec.pkt) - a_len;
    memset(sec.buf, 0, 256);
    memcpy(sec.buf, ptr, sec.buf_len);

    /* Debug info */
    WS_DEBUG("whole message: %r\n", a, ws_pktbuf_get_len(sec.pkt));
    WS_DEBUG("message len: %u\n", ws_pktbuf_get_len(sec.pkt));
    WS_DEBUG("ciphertext: %r\n", sec.buf, sec.buf_len);
    WS_DEBUG("message len %u\n", sec.buf_len);
    WS_DEBUG("a: %r\n", a, a_len);
    WS_DEBUG("a len %u\n", a_len);
    WS_DEBUG("nonse: %r\n", sec.nonse, MAC_NONSE_LEN);
    WS_DEBUG("M: %u\n", 4);
    WS_DEBUG("L: %u\n", AES_L);
    WS_DEBUG("key: %r\n", sec.key->key, WS_MAC_KEY_LEN);
    WS_DEBUG("extended addr: %r\n", sec.dev->addr.extended_addr,
             WS_MAC_ADDR_TYPE_EXTENDED_LEN);

    /* Perform the decryption */
    ws_aes_ccm_decrypt(true, 4, AES_L, sec.nonse,
                       sec.buf, sec.buf_len,
                       a, a_len,
                       sec.key->key,
                       aes_cb);

    return MAC_SECURITY_STATUS_IN_PROGRESS;
}


void
ws_mac_security_add_own_key(uint8_t *psk, uint16_t psk_len)
{
    WS_DEBUG("Installing own key");

    /* Derive the key from the PSK */
    derive_key_from_psk(mac.own_key, psk, psk_len);

    WS_DEBUG("Key installed\n");
}


void
ws_mac_security_add_device_key(ws_mac_addr_t *addr,
                               uint8_t *psk, uint16_t psk_len)
{
    mac_device_t *dev;

    uint8_t key[WS_MAC_KEY_LEN];

    WS_DEBUG("Installing key for: ");
    mac_frame_print_address(addr);
    PRINTF("\n");

    /* Derive the key from the PSK */
    derive_key_from_psk(key, psk, psk_len);

    /* Gain reference to a device */
    dev = mac_device_get_by_extended(addr->extended_addr);
    if (dev == NULL)
    {
        /* Unable to find the device, so we'll need to create one! */
        dev = mac_coordinator_create_device(addr);
        if (dev == NULL)
        {
            WS_ERROR("Failed to create device\n");
            return;
        }
    }

    WS_DEBUG("Got device, installing key\n");

    /* Add the key to the device */
    /* TODO: Should we support other key indicies? */
    mac_device_set_key(dev, 0, key);

    WS_DEBUG("Key installed\n");
}
