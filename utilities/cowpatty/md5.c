/*
 * MD5 hash implementation and interface functions
 * Copyright (c) 2003-2004, Jouni Malinen <jkmaline@cc.hut.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef OPENSSL
#include <openssl/md5.h>
#endif

#include "common.h"
#include "md5.h"

void md5_mac(u8 * key, size_t key_len, u8 * data, size_t data_len, u8 * mac)
{
	MD5_CTX context;
	MD5_Init(&context);
	MD5_Update(&context, key, key_len);
	MD5_Update(&context, data, data_len);
	MD5_Update(&context, key, key_len);
	MD5_Final(mac, &context);
}

/* HMAC code is based on RFC 2104 */
void hmac_md5_vector(u8 * key, size_t key_len, size_t num_elem,
		     u8 * addr[], size_t * len, u8 * mac)
{
	MD5_CTX context;
	u8 k_ipad[65];		/* inner padding - key XORd with ipad */
	u8 k_opad[65];		/* outer padding - key XORd with opad */
	u8 tk[16];
	int i;

	/* if key is longer than 64 bytes reset it to key = MD5(key) */
	if (key_len > 64) {
		MD5_Init(&context);
		MD5_Update(&context, key, key_len);
		MD5_Final(tk, &context);

		key = tk;
		key_len = 16;
	}

	/* the HMAC_MD5 transform looks like:
	 *
	 * MD5(K XOR opad, MD5(K XOR ipad, text))
	 *
	 * where K is an n byte key
	 * ipad is the byte 0x36 repeated 64 times
	 * opad is the byte 0x5c repeated 64 times
	 * and text is the data being protected */

	/* start out by storing key in pads */
	memset(k_ipad, 0, sizeof(k_ipad));
	memset(k_opad, 0, sizeof(k_opad));
	memcpy(k_ipad, key, key_len);
	memcpy(k_opad, key, key_len);

	/* XOR key with ipad and opad values */
	for (i = 0; i < 64; i++) {
		k_ipad[i] ^= 0x36;
		k_opad[i] ^= 0x5c;
	}

	/* perform inner MD5 */
	MD5_Init(&context);	/* init context for 1st pass */
	MD5_Update(&context, k_ipad, 64);	/* start with inner pad */
	/* then text of datagram; all fragments */
	for (i = 0; i < num_elem; i++) {
		MD5_Update(&context, addr[i], len[i]);
	}
	MD5_Final(mac, &context);	/* finish up 1st pass */

	/* perform outer MD5 */
	MD5_Init(&context);	/* init context for 2nd pass */
	MD5_Update(&context, k_opad, 64);	/* start with outer pad */
	MD5_Update(&context, mac, 16);	/* then results of 1st hash */
	MD5_Final(mac, &context);	/* finish up 2nd pass */
}

void hmac_md5(u8 * key, size_t key_len, u8 * data, size_t data_len, u8 * mac)
{
	hmac_md5_vector(key, key_len, 1, &data, &data_len, mac);
}

#ifndef OPENSSL
#error "OpenSSL is required for WPA/MD5 support."
#endif				/* OPENSSL */
