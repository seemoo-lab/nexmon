/** @file
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __WSUTIL_BITS_CTZ_H__
#define __WSUTIL_BITS_CTZ_H__

#include <inttypes.h>

/* ws_ctz == trailing zeros == position of lowest set bit [0..63] */
/* ws_ilog2 == position of highest set bit == 63 - leading zeros [0..63] */

/* The return value of both ws_ctz and ws_ilog2 is undefined for x == 0 */

#if defined(__GNUC__) && ((__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))

static inline int
ws_ctz(uint64_t x)
{
	return __builtin_ctzll(x);
}

static inline int
ws_ilog2(uint64_t x)
{
	return 63 - __builtin_clzll(x);
}

#else

/**
 * @brief Calculate the number of trailing zeros in a 32-bit unsigned integer.
 *
 * This function uses a lookup table to efficiently determine the number of trailing zeros in a given 32-bit unsigned integer.
 *
 * @param x The 32-bit unsigned integer to analyze.
 * @return The number of trailing zeros in the input value.
 */
static inline int
__ws_ctz32(uint32_t x)
{
	/* From http://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightMultLookup */
	static const uint8_t table[32] = {
		0,   1, 28,  2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17,  4, 8,
		31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18,  6, 11,  5, 10, 9
	};

	return table[((uint32_t)((x & -(int32_t)x) * 0x077CB531U)) >> 27];
}

/**
 * @brief Counts the number of trailing zeros in a 64-bit unsigned integer.
 *
 * This function calculates the number of trailing zeros in the binary representation
 * of the given 64-bit unsigned integer. It first checks if the lower 32 bits are zero,
 * and if so, it counts the trailing zeros in the upper 32 bits and adds 32 to the result.
 * Otherwise, it counts the trailing zeros in the lower 32 bits.
 *
 * @param x The 64-bit unsigned integer whose trailing zeros are to be counted.
 * @return The number of trailing zeros in the binary representation of x.
 */
static inline int
ws_ctz(uint64_t x)
{
	uint32_t hi = x >> 32;
	uint32_t lo = (uint32_t) x;

	if (lo == 0)
		return 32 + __ws_ctz32(hi);
	else
		return __ws_ctz32(lo);
}

/**
 * @brief Calculate the integer base-2 logarithm of a 32-bit unsigned integer.
 *
 * This function uses the De Bruijn sequence method to efficiently compute the
 * integer base-2 logarithm of a given 32-bit unsigned integer.
 *
 * @param x The input 32-bit unsigned integer.
 * @return The integer base-2 logarithm of x.
 */
static inline int
__ws_ilog2_32(uint32_t x)
{
	/* From http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogDeBruijn */
	static const uint8_t table[32] =	{
		0,  9,  1, 10, 13, 21,  2, 29, 11, 14, 16, 18, 22, 25,  3, 30,
		8, 12, 20, 28, 15, 17, 24,  7, 19, 27, 23,  6, 26,  5,  4, 31
	};

	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;

	return table[((uint32_t)(x * 0x07C4ACDDU)) >> 27];
}

/**
 * @brief Calculate the integer base-2 logarithm of a 64-bit unsigned integer.
 *
 * This function computes the integer base-2 logarithm of a given 64-bit unsigned integer.
 * If the high 32 bits are zero, it calculates the log for the low 32 bits; otherwise,
 * it adds 32 to the log of the high 32 bits.
 *
 * @param x The 64-bit unsigned integer whose base-2 logarithm is to be calculated.
 * @return The integer base-2 logarithm of the input value.
 */
static inline int
ws_ilog2(uint64_t x)
{
	uint32_t hi = x >> 32;
	uint32_t lo = (uint32_t) x;

	if (hi == 0)
		return __ws_ilog2_32(lo);
	else
		return 32 + __ws_ilog2_32(hi);
}

#endif

#endif /* __WSUTIL_BITS_CTZ_H__ */
