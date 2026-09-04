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

#ifndef MD5_H
#define MD5_H

void md5_mac(u8 * key, size_t key_len, u8 * data, size_t data_len, u8 * mac);
void hmac_md5_vector(u8 * key, size_t key_len, size_t num_elem,
			 u8 * addr[], size_t * len, u8 * mac);
void hmac_md5(u8 * key, size_t key_len, u8 * data, size_t data_len, u8 * mac);

#ifdef OPENSSL
#include <openssl/md5.h>
#else				/* OPENSSL */
#error "OpenSSL is required for WPA/MD5 support."
#endif				/* OPENSSL */

#endif				/* MD5_H */
