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

#ifndef _WS_AES_H
#define _WS_AES_H


#include "wsn.h"


typedef enum
{
    WS_AES_STATUS_SUCCESS,
    WS_AES_STATUS_KEY_WRITE_ERROR,
    WS_AES_STATUS_KEY_READ_ERROR,
    WS_AES_STATUS_ENCRYPT_ERROR,
} ws_aes_status_t;


/**
 * On completion, this callback will be called to pass the AES status
 * and the authentication tag.
 * @param status the status of the AES process
 * @param tag the authentication tag
 */
typedef void (*ws_aes_callback_t)(ws_aes_status_t status,
                                  uint8_t *tag);

/**
 * Prepare the AES library.
 */
extern void
ws_aes_init(void);


/**
 * Start CCM* encryption on the given data.
 * @param encrypt   true to encrypt the data (false to just perform
 *                  authentication)
 * @param M         authentication tag length (in octets)
 * @param L         CCM* L parameter
 * @param N         pointer to a nonse of length 15 - L octets
 * @param m         pointer to the message to be encrypted. The encryption
                    is performed in-place, so the resulting ciphertext will
                    be written here.
 * @param m_len     length of message in octets
 * @param a         pointer to the data used for authentication
 * @param a_len     length of authentication data in octets
 * @param key       pointer to the 128 AES key to use
 * @param cb        callback function to run on completetion
 */
extern void
ws_aes_ccm_encrypt(bool encrypt,
                   uint8_t M, uint8_t L,
                   uint8_t *N,
                   uint8_t *m, uint8_t m_len,
                   uint8_t *a, uint8_t a_len,
                   uint8_t *key,
                   ws_aes_callback_t cb);


/**
 * Start CCM* decryption on the given data.
 * @param decrypt   true to decrypt the data (false to just perform
 *                  authentication)
 * @param M         authentication tag length (in octets)
 * @param L         CCM* L parameter
 * @param N         pointer to a nonse of length 15 - L octets
 * @param c         pointer to the message to be decrypted. The decryption
                    is performed in-place, so the resulting plaintext will
                    be written here.
 * @param c_len     length of message in octets
 * @param a         pointer to the data used for authentication
 * @param a_len     length of authentication data in octets
 * @param key       pointer to the 128 AES key to use
 * @param cb        callback function to run on completetion
 */
extern void
ws_aes_ccm_decrypt(bool decrypt,
                   uint8_t M, uint8_t L,
                   uint8_t *N,
                   uint8_t *c, uint8_t c_len,
                   uint8_t *a, uint8_t a_len,
                   uint8_t *key,
                   ws_aes_callback_t cb);



#endif /* _WS_AES_H */
