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
