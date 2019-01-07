/*
 * coWPAtty - Brute-force dictionary attack against WPA-PSK.
 *
 * Copyright (c) 2004-2005, Joshua Wright <jwright@hasborg.com>
 *
 * $Id: common.h,v 4.1 2008-03-20 16:49:38 jwright Exp $
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

#ifndef COMMON_H
#define COMMON_H

#if defined(__FreeBSD__)
# include <sys/endian.h>
# define bswap_16 bswap16
# define bswap_32 bswap32
# define bswap_64 bswap64
#elif defined(__APPLE__)
# include <machine/endian.h>
# include <libkern/OSByteOrder.h>
# define bswap_16 OSSwapBigToHostInt16
# define bswap_32 OSSwapBigToHostInt32
# define bswap_64 OSSwapBigToHostInt64
#else
# include <endian.h>
# include <byteswap.h>
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define le_to_host16(n) (n)
#define host_to_le16(n) (n)
#define be_to_host16(n) bswap_16(n)
#define host_to_be16(n) bswap_16(n)
#else
#define le_to_host16(n) bswap_16(n)
#define host_to_le16(n) bswap_16(n)
#define be_to_host16(n) (n)
#define host_to_be16(n) (n)
#endif

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

#include <stdint.h>
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

#define GENPMKMAGIC 0x43575041

#endif				/* COMMON_H */
