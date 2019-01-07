/*
 * coWPAtty - Brute-force dictionary attack against WPA-PSK.
 *
 * Copyright (c) 2004-2005, Joshua Wright <jwright@hasborg.com>
 *
 * $Id: md5.h,v 4.1 2008-03-20 16:49:38 jwright Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See COPYING for more
 * details.
 *
 * coWPAtty is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
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
