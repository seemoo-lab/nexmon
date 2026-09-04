/*
 * coWPAtty - Brute-force dictionary attack against WPA-PSK.
 *
 * Copyright (c) 2004-2018, Joshua Wright <jwright@hasborg.com>
 *
 * This software may be modified and distributed under the terms
 * of the BSD-3-clause license. See the LICENSE file for details.
 */

/*
 * Significant code is graciously taken from the following:
 * wpa_supplicant by Jouni Malinen.  This tool would have been MUCH more
 * difficult for me if not for this code.  Thanks Jouni.
 */

#ifndef SHA1_H
#define SHA1_H

#ifdef OPENSSL

#include <openssl/sha.h>

#define SHA1_CTX SHA_CTX
#define SHA1Init SHA1_Init
#define SHA1Update SHA1_Update
#define SHA1Final SHA1_Final
#define SHA1_MAC_LEN SHA_DIGEST_LENGTH

#else				/* OPENSSL */
#error "OpenSSL is required for SHA1 support."
#endif				/* OPENSSL */

#define USECACHED 1
#define NOCACHED 0

typedef struct {
	SHA1_CTX k_ipad;
	SHA1_CTX k_opad;
    /*
	unsigned char k_ipad[65];
	unsigned char k_opad[65];
    */
	unsigned char k_ipad_set;
	unsigned char k_opad_set;
} SHA1_CACHE;

void sha1_mac(unsigned char *key, unsigned int key_len,
	      unsigned char *data, unsigned int data_len, unsigned char *mac);
void hmac_sha1_vector(unsigned char *key, unsigned int key_len,
		      size_t num_elem, unsigned char *addr[],
		      unsigned int *len, unsigned char *mac, int usecached);
void hmac_sha1(unsigned char *key, unsigned int key_len,
	       unsigned char *data, unsigned int data_len,
	       unsigned char *mac, int usecached);
void sha1_prf(unsigned char *key, unsigned int key_len,
	      char *label, unsigned char *data, unsigned int data_len,
	      unsigned char *buf, size_t buf_len);
void pbkdf2_sha1(char *passphrase, char *ssid, size_t ssid_len, int iterations,
		 unsigned char *buf, size_t buflen, int usecached);
void sha1_transform(u8 * state, u8 data[64]);

#endif				/* SHA1_H */
