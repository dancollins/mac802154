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
#include <stdint.h>

#include "tomcrypt.h"

/* Message inputs */
static uint8_t key[16] = "abcdefghijklmnop";
static uint8_t msg[16];
static uint8_t addr[8] = {0x04, 0x13, 0x34, 0xd2, 0x00, 0x12, 0x4b, 0x00};
static uint32_t frame_ctr = 0;
static uint8_t sec_level = 0x5;

static uint8_t hdr[] = {0x69, 0x98, 0x07, 0x00, 0xdc, 0xbb, 0xaa, 0x01,
                        0x00, 0x05, 0x00, 0x00, 0x00, 0x00};


static uint8_t ct[16];
static uint8_t tag[16];
static unsigned long tag_len = 4;


static void
print_buffer(uint8_t *buf, uint16_t len)
{
    uint16_t i = 0;

    for (i = 0; i < len; i++)
    {
        printf("%02x ", buf[i]);
    }
}


static void
swap(uint8_t *buf, uint16_t buf_len)
{
    uint16_t i, j;
    uint8_t tmp;

    for (i = 0; i < buf_len; i += 4)
    {
        for (j = 0; j < 2; j++)
        {
            tmp = buf[i+j];
            buf[i+j] = buf[i+(3-j)];
            buf[i+(3-j)] = tmp;
        }
    }
}


int
main(int argc, char **argv)
{
    symmetric_key skey;
    int err;

    /* Build the nonse */
    uint8_t nonse[13];
    uint8_t *ptr = nonse;
    memcpy(ptr, addr, 8);
    ptr += 8;
    memcpy(ptr, (uint8_t *)&frame_ctr, 4);
    ptr += 4;
    memcpy(ptr, &sec_level, 1);

    /* build the message */
    memset(msg, 0, sizeof(msg));
    msg[0] = 0xaa;

    printf("VARIABLES\n");
    printf("key: ");
    print_buffer(key, sizeof(key));
    printf("\n");

    printf("nonse: ");
    print_buffer(nonse, sizeof(nonse));
    printf("\n");

    printf("a: ");
    print_buffer(hdr, sizeof(hdr));
    printf("\n");
    printf("\n");

    register_cipher(&aes_desc);

    err = aes_setup(key, 16, 0, &skey);
    if (err != CRYPT_OK)
    {
        printf("setup failed with error %s\n", error_to_string(err));
        return -1;
    }

    err = ccm_memory(find_cipher("aes"),
                     NULL, 0,
                     &skey,
                     nonse, 13,
                     hdr, sizeof(hdr),
                     msg, 2,
                     ct,
                     tag, &tag_len,
                     CCM_ENCRYPT);
    if (err != CRYPT_OK)
    {
        printf("encryption failed with error %s\n", error_to_string(err));
        return -1;
    }

    printf("ENCRYPTED\n");
    printf("input: ");
    print_buffer(msg, sizeof(msg));
    printf("\n");

    printf("output: ");
    print_buffer(ct, sizeof(ct));
    printf("\n");

    printf("tag: ");
    print_buffer(tag, tag_len);
    printf("\n");
    printf("\n");

    memset(msg, 0xff, sizeof(msg));
    memset(tag, 0xff, sizeof(tag));
    memset(ct+2, 0x00, sizeof(ct)-2);

    err = ccm_memory(find_cipher("aes"),
                     NULL, 0,
                     &skey,
                     nonse, 13,
                     hdr, sizeof(hdr),
                     msg, 2,
                     ct,
                     tag, &tag_len,
                     CCM_DECRYPT);
    if (err != CRYPT_OK)
    {
        printf("encryption failed with error %s\n", error_to_string(err));
        return -1;
    }

    printf("DECRYPTED\n");
    printf("input: ");
    print_buffer(ct, sizeof(ct));
    printf("\n");

    printf("output: ");
    print_buffer(msg, sizeof(msg));
    printf("\n");

    printf("tag: ");
    print_buffer(tag, tag_len);
    printf("\n");

    aes_done(&skey);

    return 0;
}
