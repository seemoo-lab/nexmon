/* SIMD128 intrinsics implementation vector permutation AES for Libgcrypt
 * Copyright (C) 2024-2025 Jussi Kivilinna <jussi.kivilinna@iki.fi>
 *
 * This file is part of Libgcrypt.
 *
 * Libgcrypt is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Libgcrypt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 *
 * The code is based on the public domain library libvpaes version 0.5
 * available at http://crypto.stanford.edu/vpaes/ and which carries
 * this notice:
 *
 *     libvpaes: constant-time SSSE3 AES encryption and decryption.
 *     version 0.5
 *
 *     By Mike Hamburg, Stanford University, 2009.  Public domain.
 *     I wrote essentially all of this code.  I did not write the test
 *     vectors; they are the NIST known answer tests.  I hereby release all
 *     the code and documentation here that I wrote into the public domain.
 *
 *     This is an implementation of AES following my paper,
 *       "Accelerating AES with Vector Permute Instructions"
 *       CHES 2009; http://shiftleft.org/papers/vector_aes/
 */

#include <config.h>
#include "types.h"
#include "bufhelp.h"

#define ALWAYS_INLINE inline __attribute__((always_inline))
#define NO_INLINE __attribute__((noinline))
#define NO_INSTRUMENT_FUNCTION __attribute__((no_instrument_function))

#define ASM_FUNC_ATTR          NO_INSTRUMENT_FUNCTION
#define ASM_FUNC_ATTR_INLINE   ASM_FUNC_ATTR ALWAYS_INLINE
#define ASM_FUNC_ATTR_NOINLINE ASM_FUNC_ATTR NO_INLINE SIMD128_OPT_ATTR

/**********************************************************************
  helper macros
 **********************************************************************/

#define SWAP_LE64(x) (x)

#define M128I_BYTE(a0, a1, a2, a3, a4, a5, a6, a7, b0, b1, b2, b3, b4, b5, b6, b7) \
	{ \
	  SWAP_LE64((((a0) & 0xffULL) << 0) | \
		    (((a1) & 0xffULL) << 8) | \
		    (((a2) & 0xffULL) << 16) | \
		    (((a3) & 0xffULL) << 24) | \
		    (((a4) & 0xffULL) << 32) | \
		    (((a5) & 0xffULL) << 40) | \
		    (((a6) & 0xffULL) << 48) | \
		    (((a7) & 0xffULL) << 56)), \
	  SWAP_LE64((((b0) & 0xffULL) << 0) | \
		    (((b1) & 0xffULL) << 8) | \
		    (((b2) & 0xffULL) << 16) | \
		    (((b3) & 0xffULL) << 24) | \
		    (((b4) & 0xffULL) << 32) | \
		    (((b5) & 0xffULL) << 40) | \
		    (((b6) & 0xffULL) << 48) | \
		    (((b7) & 0xffULL) << 56)) \
	}

#define PSHUFD_MASK_TO_PSHUFB_MASK(m32) \
	M128I_BYTE(((((m32) >> 0) & 0x03) * 4) + 0, \
		   ((((m32) >> 0) & 0x03) * 4) + 1, \
		   ((((m32) >> 0) & 0x03) * 4) + 2, \
		   ((((m32) >> 0) & 0x03) * 4) + 3, \
		   ((((m32) >> 2) & 0x03) * 4) + 0, \
		   ((((m32) >> 2) & 0x03) * 4) + 1, \
		   ((((m32) >> 2) & 0x03) * 4) + 2, \
		   ((((m32) >> 2) & 0x03) * 4) + 3, \
		   ((((m32) >> 4) & 0x03) * 4) + 0, \
		   ((((m32) >> 4) & 0x03) * 4) + 1, \
		   ((((m32) >> 4) & 0x03) * 4) + 2, \
		   ((((m32) >> 4) & 0x03) * 4) + 3, \
		   ((((m32) >> 6) & 0x03) * 4) + 0, \
		   ((((m32) >> 6) & 0x03) * 4) + 1, \
		   ((((m32) >> 6) & 0x03) * 4) + 2, \
		   ((((m32) >> 6) & 0x03) * 4) + 3)

#define M128I_U64(a0, a1) { a0, a1 }

typedef u64 __m128i_const[2]  __attribute__ ((aligned (16)));

#if defined(__x86_64__) || defined(__i386__)

/**********************************************************************
  AT&T x86 asm to intrinsics conversion macros
 **********************************************************************/

#include <x86intrin.h>

#define pand128(a, o)           (o = _mm_and_si128(o, a))
#define pandn128(a, o)          (o = _mm_andnot_si128(o, a))
#define pxor128(a, o)           (o = _mm_xor_si128(o, a))
#define paddq128(a, o)          (o = _mm_add_epi64(o, a))

#define psrld128(s, o)          (o = _mm_srli_epi32(o, s))
#define psraq128(s, o)          (o = _mm_srai_epi64(o, s))
#define psrldq128(s, o)         (o = _mm_srli_si128(o, s))
#define pslldq128(s, o)         (o = _mm_slli_si128(o, s))
#define psrl_byte_128(s, o)     psrld128(o, s)

#define pshufb128(m8, o)        (o = _mm_shuffle_epi8(o, m8))
#define pshufd128(m32, a, o)    (o = _mm_shuffle_epi32(a, m32))
#define pshufd128_0x93(a, o)    pshufd128(0x93, a, o)
#define pshufd128_0xFF(a, o)    pshufd128(0xFF, a, o)
#define pshufd128_0xFE(a, o)    pshufd128(0xFE, a, o)
#define pshufd128_0x4E(a, o)    pshufd128(0x4E, a, o)

#define palignr128(s, a, o)     (o = _mm_alignr_epi8(o, a, s))

#define movdqa128(a, o)         (o = a)

#define movdqa128_memld(a, o)   (o = (__m128i)_mm_load_si128((const void *)(a)))

#define pand128_amemld(m, o)    pand128((__m128i)_mm_load_si128((const void *)(m)), o)
#define pxor128_amemld(m, o)    pxor128((__m128i)_mm_load_si128((const void *)(m)), o)
#define paddq128_amemld(m, o)   paddq128((__m128i)_mm_load_si128((const void *)(m)), o)
#define paddd128_amemld(m, o)   paddd128((__m128i)_mm_load_si128((const void *)(m)), o)
#define pshufb128_amemld(m, o)  pshufb128((__m128i)_mm_load_si128((const void *)(m)), o)

/* Following operations may have unaligned memory input */
#define movdqu128_memld(a, o)   (o = _mm_loadu_si128((const __m128i *)(a)))

/* Following operations may have unaligned memory output */
#define movdqu128_memst(a, o)   _mm_storeu_si128((__m128i *)(o), a)

#define memory_barrier_with_vec(a) __asm__("" : "+x"(a) :: "memory")

#ifdef __WIN64__
#define clear_vec_regs() __asm__ volatile("pxor %%xmm0, %%xmm0\n" \
					  "pxor %%xmm1, %%xmm1\n" \
					  "pxor %%xmm2, %%xmm2\n" \
					  "pxor %%xmm3, %%xmm3\n" \
					  "pxor %%xmm4, %%xmm4\n" \
					  "pxor %%xmm5, %%xmm5\n" \
					  /* xmm6-xmm15 are ABI callee \
					   * saved and get cleared by \
					   * function epilog when used. */ \
					  ::: "memory", "xmm0", "xmm1", \
					      "xmm2", "xmm3", "xmm4", "xmm5")
#else
#define clear_vec_regs() __asm__ volatile("pxor %%xmm0, %%xmm0\n" \
					  "pxor %%xmm1, %%xmm1\n" \
					  "pxor %%xmm2, %%xmm2\n" \
					  "pxor %%xmm3, %%xmm3\n" \
					  "pxor %%xmm4, %%xmm4\n" \
					  "pxor %%xmm5, %%xmm5\n" \
					  "pxor %%xmm6, %%xmm6\n" \
					  "pxor %%xmm7, %%xmm7\n" \
					  "pxor %%xmm8, %%xmm8\n" \
					  "pxor %%xmm9, %%xmm9\n" \
					  "pxor %%xmm10, %%xmm10\n" \
					  "pxor %%xmm11, %%xmm11\n" \
					  "pxor %%xmm12, %%xmm12\n" \
					  "pxor %%xmm13, %%xmm13\n" \
					  "pxor %%xmm14, %%xmm14\n" \
					  "pxor %%xmm15, %%xmm15\n" \
					  ::: "memory", "xmm0", "xmm1", \
					      "xmm2", "xmm3", "xmm4", "xmm5", \
					      "xmm6", "xmm7", "xmm8", "xmm9", \
					      "xmm10", "xmm11", "xmm12", \
					      "xmm13", "xmm14", "xmm15")
#endif

#endif /* x86 */

/**********************************************************************
  constant vectors
 **********************************************************************/

static const __m128i_const k_s0F =
	M128I_U64(
		0x0F0F0F0F0F0F0F0F,
		0x0F0F0F0F0F0F0F0F
	);

static const __m128i_const k_iptlo =
	M128I_U64(
		0xC2B2E8985A2A7000,
		0xCABAE09052227808
	);

static const __m128i_const k_ipthi =
	M128I_U64(
		0x4C01307D317C4D00,
		0xCD80B1FCB0FDCC81
	);

static const __m128i_const k_inv =
	M128I_U64(
		0x0E05060F0D080180,
		0x040703090A0B0C02
	);

static const __m128i_const k_inva =
	M128I_U64(
		0x01040A060F0B0780,
		0x030D0E0C02050809
	);

static const __m128i_const k_sb1u =
	M128I_U64(
		0xB19BE18FCB503E00,
		0xA5DF7A6E142AF544
	);

static const __m128i_const k_sb1t =
	M128I_U64(
		0x3618D415FAE22300,
		0x3BF7CCC10D2ED9EF
	);

static const __m128i_const k_sb2u =
	M128I_U64(
		0xE27A93C60B712400,
		0x5EB7E955BC982FCD
	);

static const __m128i_const k_sb2t =
	M128I_U64(
		0x69EB88400AE12900,
		0xC2A163C8AB82234A
	);

static const __m128i_const k_sbou =
	M128I_U64(
		0xD0D26D176FBDC700,
		0x15AABF7AC502A878
	);

static const __m128i_const k_sbot =
	M128I_U64(
		0xCFE474A55FBB6A00,
		0x8E1E90D1412B35FA
	);

static const __m128i_const k_mc_forward[4] =
{
	M128I_U64(
		0x0407060500030201,
		0x0C0F0E0D080B0A09
	),
	M128I_U64(
		0x080B0A0904070605,
		0x000302010C0F0E0D
	),
	M128I_U64(
		0x0C0F0E0D080B0A09,
		0x0407060500030201
	),
	M128I_U64(
		0x000302010C0F0E0D,
		0x080B0A0904070605
	)
};

static const __m128i_const k_mc_backward[4] =
{
	M128I_U64(
		0x0605040702010003,
		0x0E0D0C0F0A09080B
	),
	M128I_U64(
		0x020100030E0D0C0F,
		0x0A09080B06050407
	),
	M128I_U64(
		0x0E0D0C0F0A09080B,
		0x0605040702010003
	),
	M128I_U64(
		0x0A09080B06050407,
		0x020100030E0D0C0F
	)
};

static const __m128i_const k_sr[4] =
{
	M128I_U64(
		0x0706050403020100,
		0x0F0E0D0C0B0A0908
	),
	M128I_U64(
		0x030E09040F0A0500,
		0x0B06010C07020D08
	),
	M128I_U64(
		0x0F060D040B020900,
		0x070E050C030A0108
	),
	M128I_U64(
		0x0B0E0104070A0D00,
		0x0306090C0F020508
	)
};

static const __m128i_const k_rcon =
	M128I_U64(
		0x1F8391B9AF9DEEB6,
		0x702A98084D7C7D81
	);

static const __m128i_const k_s63 =
	M128I_U64(
		0x5B5B5B5B5B5B5B5B,
		0x5B5B5B5B5B5B5B5B
	);

static const __m128i_const k_opt[2] =
{
	M128I_U64(
		0xFF9F4929D6B66000,
		0xF7974121DEBE6808
	),
	M128I_U64(
		0x01EDBD5150BCEC00,
		0xE10D5DB1B05C0CE0
	)
};

static const __m128i_const k_deskew[2] =
{
	M128I_U64(
		0x07E4A34047A4E300,
		0x1DFEB95A5DBEF91A
	),
	M128I_U64(
		0x5F36B5DC83EA6900,
		0x2841C2ABF49D1E77
	)
};

static const __m128i_const k_dks_1[2] =
{
	M128I_U64(
		0xB6116FC87ED9A700,
		0x4AED933482255BFC
	),
	M128I_U64(
		0x4576516227143300,
		0x8BB89FACE9DAFDCE
	)
};

static const __m128i_const k_dks_2[2] =
{
	M128I_U64(
		0x27438FEBCCA86400,
		0x4622EE8AADC90561
	),
	M128I_U64(
		0x815C13CE4F92DD00,
		0x73AEE13CBD602FF2
	)
};

static const __m128i_const k_dks_3[2] =
{
	M128I_U64(
		0x03C4C50201C6C700,
		0xF83F3EF9FA3D3CFB
	),
	M128I_U64(
		0xEE1921D638CFF700,
		0xA5526A9D7384BC4B
	)
};

static const __m128i_const k_dks_4[2] =
{
	M128I_U64(
		0xE3C390B053732000,
		0xA080D3F310306343
	),
	M128I_U64(
		0xA0CA214B036982E8,
		0x2F45AEC48CE60D67
	)
};

static const __m128i_const k_dipt[2] =
{
	M128I_U64(
		0x0F505B040B545F00,
		0x154A411E114E451A
	),
	M128I_U64(
		0x86E383E660056500,
		0x12771772F491F194
	)
};

static const __m128i_const k_dsb9[2] =
{
	M128I_U64(
		0x851C03539A86D600,
		0xCAD51F504F994CC9
	),
	M128I_U64(
		0xC03B1789ECD74900,
		0x725E2C9EB2FBA565
	)
};

static const __m128i_const k_dsbd[2] =
{
	M128I_U64(
		0x7D57CCDFE6B1A200,
		0xF56E9B13882A4439
	),
	M128I_U64(
		0x3CE2FAF724C6CB00,
		0x2931180D15DEEFD3
	)
};

static const __m128i_const k_dsbb[2] =
{
	M128I_U64(
		0xD022649296B44200,
		0x602646F6B0F2D404
	),
	M128I_U64(
		0xC19498A6CD596700,
		0xF3FF0C3E3255AA6B
	)
};

static const __m128i_const k_dsbe[2] =
{
	M128I_U64(
		0x46F2929626D4D000,
		0x2242600464B4F6B0
	),
	M128I_U64(
		0x0C55A6CDFFAAC100,
		0x9467F36B98593E32
	)
};

static const __m128i_const k_dsbo[2] =
{
	M128I_U64(
		0x1387EA537EF94000,
		0xC7AA6DB9D4943E2D
	),
	M128I_U64(
		0x12D7560F93441D00,
		0xCA4B8159D8C58E9C
	)
};

/**********************************************************************
  vector permutate AES
 **********************************************************************/

struct vp_aes_config_s
{
  union
  {
    const byte *sched_keys;
    byte *keysched;
  };
  unsigned int nround;
};

static ASM_FUNC_ATTR_INLINE void
aes_schedule_round(__m128i *pxmm0, __m128i *pxmm7, __m128i *pxmm8,
		   __m128i xmm9, __m128i xmm10, __m128i xmm11,
		   int low_round_only)
{
  /* aes_schedule_round
   *
   * Runs one main round of the key schedule on %xmm0, %xmm7
   *
   * Specifically, runs subbytes on the high dword of %xmm0
   * then rotates it by one byte and xors into the low dword of
   * %xmm7.
   *
   * Adds rcon from low byte of %xmm8, then rotates %xmm8 for
   * next rcon.
   *
   * Smears the dwords of %xmm7 by xoring the low into the
   * second low, result into third, result into highest.
   *
   * Returns results in %xmm7 = %xmm0.
   */

  __m128i xmm1, xmm2, xmm3, xmm4;
  __m128i xmm0 = *pxmm0;
  __m128i xmm7 = *pxmm7;
  __m128i xmm8 = *pxmm8;

  if (!low_round_only)
    {
      /* extract rcon from xmm8 */
      static const __m128i_const zero = { 0 };
      movdqa128_memld(&zero, xmm1);
      palignr128(15, xmm8, xmm1);
      palignr128(15, xmm8, xmm8);
      pxor128(xmm1, xmm7);

      /* rotate */
      pshufd128_0xFF(xmm0, xmm0);
      palignr128(1, xmm0, xmm0);
    }

  /* smear xmm7 */
  movdqa128(xmm7, xmm1);
  pslldq128(4, xmm7);
  pxor128(xmm1, xmm7);
  movdqa128(xmm7, xmm1);
  pslldq128(8, xmm7);
  pxor128(xmm1, xmm7);
  pxor128_amemld(&k_s63, xmm7);

  /* subbytes */
  movdqa128(xmm9, xmm1);
  pandn128(xmm0, xmm1);
  psrl_byte_128(4, xmm1);       /* 1 = i */
  pand128(xmm9, xmm0);          /* 0 = k */
  movdqa128(xmm11, xmm2);       /* 2 : a/k */
  pshufb128(xmm0, xmm2);        /* 2 = a/k */
  pxor128(xmm1, xmm0);          /* 0 = j */
  movdqa128(xmm10, xmm3);       /* 3 : 1/i */
  pshufb128(xmm1, xmm3);        /* 3 = 1/i */
  pxor128(xmm2, xmm3);          /* 3 = iak = 1/i + a/k */
  movdqa128(xmm10, xmm4);       /* 4 : 1/j */
  pshufb128(xmm0, xmm4);        /* 4 = 1/j */
  pxor128(xmm2, xmm4);          /* 4 = jak = 1/j + a/k */
  movdqa128(xmm10, xmm2);       /* 2 : 1/iak */
  pshufb128(xmm3, xmm2);        /* 2 = 1/iak */
  pxor128(xmm0, xmm2);          /* 2 = io */
  movdqa128(xmm10, xmm3);       /* 3 : 1/jak */
  pshufb128(xmm4, xmm3);        /* 3 = 1/jak */
  pxor128(xmm1, xmm3);          /* 3 = jo */
  movdqa128_memld(&k_sb1u, xmm4);      /* 4 : sbou */
  pshufb128(xmm2, xmm4);        /* 4 = sbou */
  movdqa128_memld(&k_sb1t, xmm0);      /* 0 : sbot */
  pshufb128(xmm3, xmm0);        /* 0 = sb1t */
  pxor128(xmm4, xmm0);          /* 0 = sbox output */

  /* add in smeared stuff */
  pxor128(xmm7, xmm0);
  movdqa128(xmm0, xmm7);

  *pxmm0 = xmm0;
  *pxmm7 = xmm7;
  *pxmm8 = xmm8;
}

static ASM_FUNC_ATTR_INLINE __m128i
aes_schedule_transform(__m128i xmm0, const __m128i xmm9,
		       const __m128i_const *tablelo,
		       const __m128i_const *tablehi)
{
  /* aes_schedule_transform
   *
   * Linear-transform %xmm0 according to tablelo:tablehi
   *
   * Requires that %xmm9 = 0x0F0F... as in preheat
   * Output in %xmm0
   */

  __m128i xmm1, xmm2;

  movdqa128(xmm9, xmm1);
  pandn128(xmm0, xmm1);
  psrl_byte_128(4, xmm1);
  pand128(xmm9, xmm0);
  movdqa128_memld(tablelo, xmm2);
  pshufb128(xmm0, xmm2);
  movdqa128_memld(tablehi, xmm0);
  pshufb128(xmm1, xmm0);
  pxor128(xmm2, xmm0);

  return xmm0;
}

static ASM_FUNC_ATTR_INLINE void
aes_schedule_mangle(__m128i xmm0, struct vp_aes_config_s *pconfig, int decrypt,
		    unsigned int *protoffs, __m128i xmm9)
{
  /* aes_schedule_mangle
   *
   * Mangle xmm0 from (basis-transformed) standard version
   * to our version.
   *
   * On encrypt,
   *     xor with 0x63
   *     multiply by circulant 0,1,1,1
   *     apply shiftrows transform
   *
   * On decrypt,
   *    xor with 0x63
   *    multiply by 'inverse mixcolumns' circulant E,B,D,9
   *    deskew
   *    apply shiftrows transform
   *
   * Writes out to (keysched), and increments or decrements it
   * Keeps track of round number mod 4 in (rotoffs)
   */
  __m128i xmm3, xmm4, xmm5;
  struct vp_aes_config_s config = *pconfig;
  byte *keysched = config.keysched;
  unsigned int rotoffs = *protoffs;

  movdqa128(xmm0, xmm4);
  movdqa128_memld(&k_mc_forward[0], xmm5);

  if (!decrypt)
    {
      keysched += 16;
      pxor128_amemld(&k_s63, xmm4);
      pshufb128(xmm5, xmm4);
      movdqa128(xmm4, xmm3);
      pshufb128(xmm5, xmm4);
      pxor128(xmm4, xmm3);
      pshufb128(xmm5, xmm4);
      pxor128(xmm4, xmm3);
    }
  else
    {
      /* first table: *9 */
      xmm0 = aes_schedule_transform(xmm0, xmm9, &k_dks_1[0], &k_dks_1[1]);
      movdqa128(xmm0, xmm3);
      pshufb128(xmm5, xmm3);

      /* next table:  *B */
      xmm0 = aes_schedule_transform(xmm0, xmm9, &k_dks_2[0], &k_dks_2[1]);
      pxor128(xmm0, xmm3);
      pshufb128(xmm5, xmm3);

      /* next table:  *D */
      xmm0 = aes_schedule_transform(xmm0, xmm9, &k_dks_3[0], &k_dks_3[1]);
      pxor128(xmm0, xmm3);
      pshufb128(xmm5, xmm3);

      /* next table:  *E */
      xmm0 = aes_schedule_transform(xmm0, xmm9, &k_dks_4[0], &k_dks_4[1]);
      pxor128(xmm0, xmm3);
      pshufb128(xmm5, xmm3);

      keysched -= 16;
    }

  pshufb128_amemld(&k_sr[rotoffs], xmm3);
  rotoffs -= 16 / 16;
  rotoffs &= 48 / 16;
  movdqu128_memst(xmm3, keysched);

  config.keysched = keysched;
  *pconfig = config;
  *protoffs = rotoffs;
}

static ASM_FUNC_ATTR_INLINE void
aes_schedule_mangle_last(__m128i xmm0, struct vp_aes_config_s config,
			 int decrypt, unsigned int rotoffs, __m128i xmm9)
{
  /* aes_schedule_mangle_last
   *
   * Mangler for last round of key schedule
   *
   * Mangles %xmm0
   *     when encrypting, outputs out(%xmm0) ^ 63
   *     when decrypting, outputs unskew(%xmm0)
   */

  if (!decrypt)
    {
      pshufb128_amemld(&k_sr[rotoffs], xmm0); /* output permute */
      config.keysched += 16;
      pxor128_amemld(&k_s63, xmm0);
      xmm0 = aes_schedule_transform(xmm0, xmm9, &k_opt[0], &k_opt[1]);
    }
  else
    {
      config.keysched -= 16;
      pxor128_amemld(&k_s63, xmm0);
      xmm0 = aes_schedule_transform(xmm0, xmm9, &k_deskew[0], &k_deskew[1]);
    }

  movdqu128_memst(xmm0, config.keysched); /* save last key */
}

static ASM_FUNC_ATTR_INLINE void
aes_schedule_128(struct vp_aes_config_s config, int decrypt,
		 unsigned int rotoffs, __m128i xmm0, __m128i xmm7,
		 __m128i xmm8, __m128i xmm9, __m128i xmm10, __m128i xmm11)
{
  /* aes_schedule_128
   *
   * 128-bit specific part of key schedule.
   *
   * This schedule is really simple, because all its parts
   * are accomplished by the subroutines.
   */

  int r = 10;

  while (1)
    {
      aes_schedule_round(&xmm0, &xmm7, &xmm8, xmm9, xmm10, xmm11, 0);

      if (--r == 0)
	break;

      aes_schedule_mangle(xmm0, &config, decrypt, &rotoffs, xmm9);
    }

  aes_schedule_mangle_last(xmm0, config, decrypt, rotoffs, xmm9);
}

static ASM_FUNC_ATTR_INLINE void
aes_schedule_192_smear(__m128i *pxmm0, __m128i *pxmm6, __m128i xmm7)
{
  /*
   * aes_schedule_192_smear
   *
   * Smear the short, low side in the 192-bit key schedule.
   *
   * Inputs:
   *    %xmm7: high side, b  a  x  y
   *    %xmm6:  low side, d  c  0  0
   *
   * Outputs:
   *    %xmm6: b+c+d  b+c  0  0
   *    %xmm0: b+c+d  b+c  b  a
   */

  __m128i xmm0 = *pxmm0;
  __m128i xmm6 = *pxmm6;

  movdqa128(xmm6, xmm0);
  pslldq128(4, xmm0);           /* d c 0 0 -> c 0 0 0 */
  pxor128(xmm0, xmm6);          /* -> c+d c 0 0 */
  pshufd128_0xFE(xmm7, xmm0);   /* b a _ _ -> b b b a */
  pxor128(xmm6, xmm0);          /* -> b+c+d b+c b a */
  movdqa128(xmm0, xmm6);
  psrldq128(8, xmm6);
  pslldq128(8, xmm6);           /* clobber low side with zeros */

  *pxmm0 = xmm0;
  *pxmm6 = xmm6;
}

static ASM_FUNC_ATTR_INLINE void
aes_schedule_192(const byte *key, struct vp_aes_config_s config, int decrypt,
		 unsigned int rotoffs, __m128i xmm0, __m128i xmm7,
		 __m128i xmm8, __m128i xmm9, __m128i xmm10, __m128i xmm11)
{
  /* aes_schedule_192
   *
   * 192-bit specific part of key schedule.
   *
   * The main body of this schedule is the same as the 128-bit
   * schedule, but with more smearing.  The long, high side is
   * stored in %xmm7 as before, and the short, low side is in
   * the high bits of %xmm6.
   *
   * This schedule is somewhat nastier, however, because each
   * round produces 192 bits of key material, or 1.5 round keys.
   * Therefore, on each cycle we do 2 rounds and produce 3 round
   * keys.
   */

  __m128i xmm6;
  int r = 4;

  movdqu128_memld(key + 8, xmm0); /* load key part 2 (very unaligned) */
  xmm0 = aes_schedule_transform(xmm0, xmm9, &k_iptlo, &k_ipthi); /* input transform */
  movdqa128(xmm0, xmm6);
  psrldq128(8, xmm6);
  pslldq128(8, xmm6); /* clobber low side with zeros */

  while (1)
    {
      aes_schedule_round(&xmm0, &xmm7, &xmm8, xmm9, xmm10, xmm11, 0);
      palignr128(8, xmm6, xmm0);
      aes_schedule_mangle(xmm0, &config, decrypt, &rotoffs, xmm9); /* save key n */
      aes_schedule_192_smear(&xmm0, &xmm6, xmm7);
      aes_schedule_mangle(xmm0, &config, decrypt, &rotoffs, xmm9); /* save key n+1 */
      aes_schedule_round(&xmm0, &xmm7, &xmm8, xmm9, xmm10, xmm11, 0);
      if (--r == 0)
	break;
      aes_schedule_mangle(xmm0, &config, decrypt, &rotoffs, xmm9); /* save key n+2 */
      aes_schedule_192_smear(&xmm0, &xmm6, xmm7);
    }

  aes_schedule_mangle_last(xmm0, config, decrypt, rotoffs, xmm9);
}

static ASM_FUNC_ATTR_INLINE void
aes_schedule_256(const byte *key, struct vp_aes_config_s config, int decrypt,
		 unsigned int rotoffs, __m128i xmm0, __m128i xmm7,
		 __m128i xmm8, __m128i xmm9, __m128i xmm10, __m128i xmm11)
{
  /* aes_schedule_256
   *
   * 256-bit specific part of key schedule.
   *
   * The structure here is very similar to the 128-bit
   * schedule, but with an additional 'low side' in
   * %xmm6.  The low side's rounds are the same as the
   * high side's, except no rcon and no rotation.
   */

  __m128i xmm5, xmm6;

  int r = 7;

  movdqu128_memld(key + 16, xmm0); /* load key part 2 (unaligned) */
  xmm0 = aes_schedule_transform(xmm0, xmm9, &k_iptlo, &k_ipthi); /* input transform */

  while (1)
    {
      aes_schedule_mangle(xmm0, &config, decrypt, &rotoffs, xmm9); /* output low result */
      movdqa128(xmm0, xmm6); /* save cur_lo in xmm6 */

      /* high round */
      aes_schedule_round(&xmm0, &xmm7, &xmm8, xmm9, xmm10, xmm11, 0);

      if (--r == 0)
	break;

      aes_schedule_mangle(xmm0, &config, decrypt, &rotoffs, xmm9);

      /* low round. swap xmm7 and xmm6 */
      pshufd128_0xFF(xmm0, xmm0);
      movdqa128(xmm7, xmm5);
      movdqa128(xmm6, xmm7);
      aes_schedule_round(&xmm0, &xmm7, &xmm8, xmm9, xmm10, xmm11, 1);
      movdqa128(xmm5, xmm7);
    }

  aes_schedule_mangle_last(xmm0, config, decrypt, rotoffs, xmm9);
}

static ASM_FUNC_ATTR_INLINE void
aes_schedule_core(const byte *key, struct vp_aes_config_s config,
		  int decrypt, unsigned int rotoffs)
{
  unsigned int keybits = (config.nround - 10) * 32 + 128;
  __m128i xmm0, xmm3, xmm7, xmm8, xmm9, xmm10, xmm11;

  movdqa128_memld(&k_s0F, xmm9);
  movdqa128_memld(&k_inv, xmm10);
  movdqa128_memld(&k_inva, xmm11);
  movdqa128_memld(&k_rcon, xmm8);

  movdqu128_memld(key, xmm0);

  /* input transform */
  movdqa128(xmm0, xmm3);
  xmm0 = aes_schedule_transform(xmm0, xmm9, &k_iptlo, &k_ipthi);
  movdqa128(xmm0, xmm7);

  if (!decrypt)
    {
      /* encrypting, output zeroth round key after transform */
      movdqu128_memst(xmm0, config.keysched);
    }
  else
    {
      /* decrypting, output zeroth round key after shiftrows */
      pshufb128_amemld(&k_sr[rotoffs], xmm3);
      movdqu128_memst(xmm3, config.keysched);
      rotoffs ^= 48 / 16;
    }

  if (keybits < 192)
    {
      aes_schedule_128(config, decrypt, rotoffs, xmm0, xmm7, xmm8, xmm9,
		       xmm10, xmm11);
    }
  else if (keybits == 192)
    {
      aes_schedule_192(key, config, decrypt, rotoffs, xmm0, xmm7, xmm8, xmm9,
		       xmm10, xmm11);
    }
  else
    {
      aes_schedule_256(key, config, decrypt, rotoffs, xmm0, xmm7, xmm8, xmm9,
		       xmm10, xmm11);
    }
}

ASM_FUNC_ATTR_NOINLINE void
FUNC_SETKEY (RIJNDAEL_context *ctx, const byte *key)
{
  unsigned int keybits = (ctx->rounds - 10) * 32 + 128;
  struct vp_aes_config_s config;
  __m128i xmm0, xmm1;

  config.nround = ctx->rounds;
  config.keysched = (byte *)&ctx->keyschenc32[0][0];

  aes_schedule_core(key, config, 0, 48 / 16);

  /* Save key for setting up decryption. */
  switch (keybits)
    {
    default:
    case 128:
      movdqu128_memld(key, xmm0);
      movdqu128_memst(xmm0, ((byte *)&ctx->keyschdec32[0][0]));
      break;

    case 192:
      movdqu128_memld(key, xmm0);
      movdqu128_memld(key + 8, xmm1);
      movdqu128_memst(xmm0, ((byte *)&ctx->keyschdec32[0][0]));
      movdqu128_memst(xmm1, ((byte *)&ctx->keyschdec32[0][0]) + 8);
      break;

    case 256:
      movdqu128_memld(key, xmm0);
      movdqu128_memld(key + 16, xmm1);
      movdqu128_memst(xmm0, ((byte *)&ctx->keyschdec32[0][0]));
      movdqu128_memst(xmm1, ((byte *)&ctx->keyschdec32[0][0]) + 16);
      break;
    }

  clear_vec_regs();
}


ASM_FUNC_ATTR_NOINLINE void
FUNC_PREPARE_DEC (RIJNDAEL_context *ctx)
{
  unsigned int keybits = (ctx->rounds - 10) * 32 + 128;
  struct vp_aes_config_s config;

  config.nround = ctx->rounds;
  config.keysched = (byte *)&ctx->keyschdec32[ctx->rounds][0];

  aes_schedule_core((byte *)&ctx->keyschdec32[0][0], config, 1,
		    ((keybits == 192) ? 0 : 32) / 16);

  clear_vec_regs();
}

#define enc_preload(xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15) \
	movdqa128_memld(&k_s0F, xmm9); \
	movdqa128_memld(&k_inv, xmm10); \
	movdqa128_memld(&k_inva, xmm11); \
	movdqa128_memld(&k_sb1u, xmm13); \
	movdqa128_memld(&k_sb1t, xmm12); \
	movdqa128_memld(&k_sb2u, xmm15); \
	movdqa128_memld(&k_sb2t, xmm14);

#define dec_preload(xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15, xmm8) \
	movdqa128_memld(&k_s0F, xmm9); \
	movdqa128_memld(&k_inv, xmm10); \
	movdqa128_memld(&k_inva, xmm11); \
	movdqa128_memld(&k_dsb9[0], xmm13); \
	movdqa128_memld(&k_dsb9[1], xmm12); \
	movdqa128_memld(&k_dsbd[0], xmm15); \
	movdqa128_memld(&k_dsbb[0], xmm14); \
	movdqa128_memld(&k_dsbe[0], xmm8);

static ASM_FUNC_ATTR_INLINE __m128i
aes_encrypt_core(__m128i xmm0, struct vp_aes_config_s config,
		 __m128i xmm9, __m128i xmm10, __m128i xmm11, __m128i xmm12,
		 __m128i xmm13, __m128i xmm14, __m128i xmm15)
{
  __m128i xmm1, xmm2, xmm3, xmm4;
  const byte *end_keys = config.sched_keys + 16 * config.nround;
  unsigned int mc_pos = 1;

  movdqa128_memld(&k_iptlo, xmm2);
  movdqa128(xmm9, xmm1);
  pandn128(xmm0, xmm1);
  psrl_byte_128(4, xmm1);
  pand128(xmm9, xmm0);
  pshufb128(xmm0, xmm2);
  movdqa128_memld(&k_ipthi, xmm0);

  pshufb128(xmm1, xmm0);
  pxor128_amemld(config.sched_keys, xmm2);
  pxor128(xmm2, xmm0);

  config.sched_keys += 16;

  while (1)
    {
      /* top of round */
      movdqa128(xmm9, xmm1);                  /* 1 : i */
      pandn128(xmm0, xmm1);                   /* 1 = i<<4 */
      psrl_byte_128(4, xmm1);                 /* 1 = i */
      pand128(xmm9, xmm0);                    /* 0 = k */
      movdqa128(xmm11, xmm2);                 /* 2 : a/k */
      pshufb128(xmm0, xmm2);                  /* 2 = a/k */
      pxor128(xmm1, xmm0);                    /* 0 = j */
      movdqa128(xmm10, xmm3);                 /* 3 : 1/i */
      pshufb128(xmm1, xmm3);                  /* 3 = 1/i */
      pxor128(xmm2, xmm3);                    /* 3 = iak = 1/i + a/k */
      movdqa128(xmm10, xmm4);                 /* 4 : 1/j */
      pshufb128(xmm0,  xmm4);                 /* 4 = 1/j */
      pxor128(xmm2, xmm4);                    /* 4 = jak = 1/j + a/k */
      movdqa128(xmm10, xmm2);                 /* 2 : 1/iak */
      pshufb128(xmm3, xmm2);                  /* 2 = 1/iak */
      pxor128(xmm0, xmm2);                    /* 2 = io */
      movdqa128(xmm10, xmm3);                 /* 3 : 1/jak */
      pshufb128(xmm4, xmm3);                  /* 3 = 1/jak */
      pxor128(xmm1, xmm3);                    /* 3 = jo */

      if (config.sched_keys == end_keys)
	break;

      /* middle of middle round */
      movdqa128(xmm13, xmm4);                 /* 4 : sb1u */
      pshufb128(xmm2, xmm4);                  /* 4 = sb1u */
      pxor128_amemld(config.sched_keys, xmm4); /* 4 = sb1u + k */
      movdqa128(xmm12, xmm0);                 /* 0 : sb1t */
      pshufb128(xmm3, xmm0);                  /* 0 = sb1t */
      pxor128(xmm4, xmm0);                    /* 0 = A */
      movdqa128(xmm15, xmm4);                 /* 4 : sb2u */
      pshufb128(xmm2, xmm4);                  /* 4 = sb2u */
      movdqa128_memld(&k_mc_forward[mc_pos], xmm1);
      movdqa128(xmm14, xmm2);                 /* 2 : sb2t */
      pshufb128(xmm3, xmm2);                  /* 2 = sb2t */
      pxor128(xmm4, xmm2);                    /* 2 = 2A */
      movdqa128(xmm0, xmm3);                  /* 3 = A */
      pshufb128(xmm1, xmm0);                  /* 0 = B */
      pxor128(xmm2, xmm0);                    /* 0 = 2A+B */
      pshufb128_amemld(&k_mc_backward[mc_pos], xmm3); /* 3 = D */
      pxor128(xmm0, xmm3);                    /* 3 = 2A+B+D */
      pshufb128(xmm1, xmm0);                  /* 0 = 2B+C */
      pxor128(xmm3, xmm0);                    /* 0 = 2A+3B+C+D */

      config.sched_keys += 16;
      mc_pos = (mc_pos + 1) % 4; /* next mc mod 4 */
    }

  /* middle of last round */
  movdqa128_memld(&k_sbou, xmm4);   /* 3 : sbou */
  pshufb128(xmm2, xmm4);            /* 4 = sbou */
  pxor128_amemld(config.sched_keys, xmm4); /* 4 = sb1u + k */
  movdqa128_memld(&k_sbot, xmm0);   /* 0 : sbot */
  pshufb128(xmm3, xmm0);            /* 0 = sb1t */
  pxor128(xmm4, xmm0);              /* 0 = A */
  pshufb128_amemld(&k_sr[mc_pos], xmm0);

  return xmm0;
}

static ASM_FUNC_ATTR_INLINE void
aes_encrypt_core_2blks(__m128i *pxmm0_a, __m128i *pxmm0_b,
		       struct vp_aes_config_s config,
		       __m128i xmm9, __m128i xmm10, __m128i xmm11,
		       __m128i xmm12, __m128i xmm13, __m128i xmm14,
		       __m128i xmm15)
{
  __m128i xmm0_a, xmm0_b;
  __m128i xmm1_a, xmm2_a, xmm3_a, xmm4_a;
  __m128i xmm1_b, xmm2_b, xmm3_b, xmm4_b;
  __m128i xmm5, xmm6;
  const byte *end_keys = config.sched_keys + 16 * config.nround;
  unsigned int mc_pos = 1;

  xmm0_a = *pxmm0_a;
  xmm0_b = *pxmm0_b;

  movdqa128_memld(&k_iptlo, xmm2_a); movdqa128(xmm2_a, xmm2_b);
  movdqa128(xmm9, xmm1_a);	movdqa128(xmm9, xmm1_b);
  pandn128(xmm0_a, xmm1_a);	pandn128(xmm0_b, xmm1_b);
  psrl_byte_128(4, xmm1_a);	psrl_byte_128(4, xmm1_b);
  pand128(xmm9, xmm0_a);	pand128(xmm9, xmm0_b);
  pshufb128(xmm0_a, xmm2_a);	pshufb128(xmm0_b, xmm2_b);
  movdqa128_memld(&k_ipthi, xmm0_a); movdqa128(xmm0_a, xmm0_b);

  pshufb128(xmm1_a, xmm0_a);	pshufb128(xmm1_b, xmm0_b);
  movdqu128_memld(config.sched_keys, xmm5);
  pxor128(xmm5, xmm2_a);	pxor128(xmm5, xmm2_b);
  pxor128(xmm2_a, xmm0_a);	pxor128(xmm2_b, xmm0_b);

  config.sched_keys += 16;

  while (1)
    {
      /* top of round */
      movdqa128(xmm9, xmm1_a);		movdqa128(xmm9, xmm1_b);
      pandn128(xmm0_a, xmm1_a);		pandn128(xmm0_b, xmm1_b);
      psrl_byte_128(4, xmm1_a);		psrl_byte_128(4, xmm1_b);
      pand128(xmm9, xmm0_a);		pand128(xmm9, xmm0_b);
      movdqa128(xmm11, xmm2_a);		movdqa128(xmm11, xmm2_b);
      pshufb128(xmm0_a, xmm2_a);	pshufb128(xmm0_b, xmm2_b);
      pxor128(xmm1_a, xmm0_a);		pxor128(xmm1_b, xmm0_b);
      movdqa128(xmm10, xmm3_a);		movdqa128(xmm10, xmm3_b);
      pshufb128(xmm1_a, xmm3_a);	pshufb128(xmm1_b, xmm3_b);
      pxor128(xmm2_a, xmm3_a);		pxor128(xmm2_b, xmm3_b);
      movdqa128(xmm10, xmm4_a);		movdqa128(xmm10, xmm4_b);
      pshufb128(xmm0_a,  xmm4_a);	pshufb128(xmm0_b,  xmm4_b);
      pxor128(xmm2_a, xmm4_a);		pxor128(xmm2_b, xmm4_b);
      movdqa128(xmm10, xmm2_a);		movdqa128(xmm10, xmm2_b);
      pshufb128(xmm3_a, xmm2_a);	pshufb128(xmm3_b, xmm2_b);
      pxor128(xmm0_a, xmm2_a);		pxor128(xmm0_b, xmm2_b);
      movdqa128(xmm10, xmm3_a);		movdqa128(xmm10, xmm3_b);
      pshufb128(xmm4_a, xmm3_a);	pshufb128(xmm4_b, xmm3_b);
      pxor128(xmm1_a, xmm3_a);		pxor128(xmm1_b, xmm3_b);

      if (config.sched_keys == end_keys)
	break;

      /* middle of middle round */
      movdqa128(xmm13, xmm4_a);		movdqa128(xmm13, xmm4_b);
      pshufb128(xmm2_a, xmm4_a);	pshufb128(xmm2_b, xmm4_b);
      movdqu128_memld(config.sched_keys, xmm5);
      pxor128(xmm5, xmm4_a);		pxor128(xmm5, xmm4_b);
      movdqa128(xmm12, xmm0_a);		movdqa128(xmm12, xmm0_b);
      pshufb128(xmm3_a, xmm0_a);	pshufb128(xmm3_b, xmm0_b);
      pxor128(xmm4_a, xmm0_a);		pxor128(xmm4_b, xmm0_b);
      movdqa128(xmm15, xmm4_a);		movdqa128(xmm15, xmm4_b);
      pshufb128(xmm2_a, xmm4_a);	pshufb128(xmm2_b, xmm4_b);
      movdqa128_memld(&k_mc_forward[mc_pos], xmm6);
      movdqa128(xmm14, xmm2_a);		movdqa128(xmm14, xmm2_b);
      pshufb128(xmm3_a, xmm2_a);	pshufb128(xmm3_b, xmm2_b);
      pxor128(xmm4_a, xmm2_a);		pxor128(xmm4_b, xmm2_b);
      movdqa128(xmm0_a, xmm3_a);	movdqa128(xmm0_b, xmm3_b);
      pshufb128(xmm6, xmm0_a);		pshufb128(xmm6, xmm0_b);
      pxor128(xmm2_a, xmm0_a);		pxor128(xmm2_b, xmm0_b);
      movdqa128_memld(&k_mc_backward[mc_pos], xmm5);
      pshufb128(xmm5, xmm3_a);		pshufb128(xmm5, xmm3_b);
      pxor128(xmm0_a, xmm3_a);		pxor128(xmm0_b, xmm3_b);
      pshufb128(xmm6, xmm0_a);		pshufb128(xmm6, xmm0_b);
      pxor128(xmm3_a, xmm0_a);		pxor128(xmm3_b, xmm0_b);

      config.sched_keys += 16;
      mc_pos = (mc_pos + 1) % 4; /* next mc mod 4 */
    }

  /* middle of last round */
  movdqa128_memld(&k_sbou, xmm4_a); movdqa128_memld(&k_sbou, xmm4_b);
  pshufb128(xmm2_a, xmm4_a);	pshufb128(xmm2_b, xmm4_b);
  movdqu128_memld(config.sched_keys, xmm5);
  pxor128(xmm5, xmm4_a);	pxor128(xmm5, xmm4_b);
  movdqa128_memld(&k_sbot, xmm0_a); movdqa128_memld(&k_sbot, xmm0_b);
  pshufb128(xmm3_a, xmm0_a);	pshufb128(xmm3_b, xmm0_b);
  pxor128(xmm4_a, xmm0_a);	pxor128(xmm4_b, xmm0_b);
  movdqa128_memld(&k_sr[mc_pos], xmm5);
  pshufb128(xmm5, xmm0_a);	pshufb128(xmm5, xmm0_b);

  *pxmm0_a = xmm0_a;
  *pxmm0_b = xmm0_b;
}

#ifdef HAVE_SIMD256

static ASM_FUNC_ATTR_INLINE void
aes_encrypt_core_4blks_simd256(__m256i *pymm0_a, __m256i *pymm0_b,
			       struct vp_aes_config_s config,
			       __m128i xmm9, __m128i xmm10, __m128i xmm11,
			       __m128i xmm12, __m128i xmm13, __m128i xmm14,
			       __m128i xmm15)
{
  __m256i ymm9, ymm10, ymm11, ymm12, ymm13, ymm14, ymm15;
  __m256i ymm0_a, ymm0_b;
  __m256i ymm1_a, ymm2_a, ymm3_a, ymm4_a;
  __m256i ymm1_b, ymm2_b, ymm3_b, ymm4_b;
  __m256i ymm5, ymm6;
  const byte *end_keys = config.sched_keys + 16 * config.nround;
  unsigned int mc_pos = 1;

  broadcast128_256(xmm9, ymm9);
  movdqa128_256(xmm10, ymm10);
  movdqa128_256(xmm11, ymm11);
  movdqa128_256(xmm12, ymm12);
  movdqa128_256(xmm13, ymm13);
  movdqa128_256(xmm14, ymm14);
  movdqa128_256(xmm15, ymm15);

  ymm0_a = *pymm0_a;
  ymm0_b = *pymm0_b;

  load_tab16_table(&k_iptlo, ymm2_a); 	movdqa256(ymm2_a, ymm2_b);
  movdqa256(ymm9, ymm1_a);		movdqa256(ymm9, ymm1_b);
  pandn256(ymm0_a, ymm1_a);		pandn256(ymm0_b, ymm1_b);
  psrl_byte_256(4, ymm1_a);		psrl_byte_256(4, ymm1_b);
  pand256(ymm9, ymm0_a);		pand256(ymm9, ymm0_b);
  pshufb256_tab16(ymm0_a, ymm2_a);	pshufb256_tab16(ymm0_b, ymm2_b);
  load_tab16_table(&k_ipthi, ymm0_a); 	movdqa256(ymm0_a, ymm0_b);

  pshufb256_tab16(ymm1_a, ymm0_a);	pshufb256_tab16(ymm1_b, ymm0_b);
  broadcast128_256_amemld(config.sched_keys, ymm5);
  pxor256(ymm5, ymm2_a);		pxor256(ymm5, ymm2_b);
  pxor256(ymm2_a, ymm0_a);		pxor256(ymm2_b, ymm0_b);

  config.sched_keys += 16;

  while (1)
    {
      /* top of round */
      movdqa256(ymm9, ymm1_a);		movdqa256(ymm9, ymm1_b);
      pandn256(ymm0_a, ymm1_a);		pandn256(ymm0_b, ymm1_b);
      psrl_byte_256(4, ymm1_a);		psrl_byte_256(4, ymm1_b);
      pand256(ymm9, ymm0_a);		pand256(ymm9, ymm0_b);
      movdqa256(ymm11, ymm2_a);		movdqa256(ymm11, ymm2_b);
      pshufb256_tab16(ymm0_a, ymm2_a);	pshufb256_tab16(ymm0_b, ymm2_b);
      pxor256(ymm1_a, ymm0_a);		pxor256(ymm1_b, ymm0_b);
      movdqa256(ymm10, ymm3_a);		movdqa256(ymm10, ymm3_b);
      pshufb256_tab16(ymm1_a, ymm3_a);	pshufb256_tab16(ymm1_b, ymm3_b);
      pxor256(ymm2_a, ymm3_a);		pxor256(ymm2_b, ymm3_b);
      movdqa256(ymm10, ymm4_a);		movdqa256(ymm10, ymm4_b);
      pshufb256_tab16(ymm0_a,  ymm4_a);	pshufb256_tab16(ymm0_b,  ymm4_b);
      pxor256(ymm2_a, ymm4_a);		pxor256(ymm2_b, ymm4_b);
      movdqa256(ymm10, ymm2_a);		movdqa256(ymm10, ymm2_b);
      pshufb256_tab16(ymm3_a, ymm2_a);	pshufb256_tab16(ymm3_b, ymm2_b);
      pxor256(ymm0_a, ymm2_a);		pxor256(ymm0_b, ymm2_b);
      movdqa256(ymm10, ymm3_a);		movdqa256(ymm10, ymm3_b);
      pshufb256_tab16(ymm4_a, ymm3_a);	pshufb256_tab16(ymm4_b, ymm3_b);
      pxor256(ymm1_a, ymm3_a);		pxor256(ymm1_b, ymm3_b);

      if (config.sched_keys == end_keys)
	break;

      /* middle of middle round */
      movdqa256(ymm13, ymm4_a);		movdqa256(ymm13, ymm4_b);
      pshufb256_tab16(ymm2_a, ymm4_a);	pshufb256_tab16(ymm2_b, ymm4_b);
      broadcast128_256_amemld(config.sched_keys, ymm5);
      pxor256(ymm5, ymm4_a);		pxor256(ymm5, ymm4_b);
      movdqa256(ymm12, ymm0_a);		movdqa256(ymm12, ymm0_b);
      pshufb256_tab16(ymm3_a, ymm0_a);	pshufb256_tab16(ymm3_b, ymm0_b);
      pxor256(ymm4_a, ymm0_a);		pxor256(ymm4_b, ymm0_b);
      movdqa256(ymm15, ymm4_a);		movdqa256(ymm15, ymm4_b);
      pshufb256_tab16(ymm2_a, ymm4_a);	pshufb256_tab16(ymm2_b, ymm4_b);
      load_tab32_mask(&k_mc_forward[mc_pos], ymm6);
      movdqa256(ymm14, ymm2_a);		movdqa256(ymm14, ymm2_b);
      pshufb256_tab16(ymm3_a, ymm2_a);	pshufb256_tab16(ymm3_b, ymm2_b);
      pxor256(ymm4_a, ymm2_a);		pxor256(ymm4_b, ymm2_b);
      movdqa256(ymm0_a, ymm3_a);	movdqa256(ymm0_b, ymm3_b);
      pshufb256_tab32(ymm6, ymm0_a);	pshufb256_tab32(ymm6, ymm0_b);
      pxor256(ymm2_a, ymm0_a);		pxor256(ymm2_b, ymm0_b);
      load_tab32_mask(&k_mc_backward[mc_pos], ymm5);
      pshufb256_tab32(ymm5, ymm3_a);	pshufb256_tab32(ymm5, ymm3_b);
      pxor256(ymm0_a, ymm3_a);		pxor256(ymm0_b, ymm3_b);
      pshufb256_tab32(ymm6, ymm0_a);	pshufb256_tab32(ymm6, ymm0_b);
      pxor256(ymm3_a, ymm0_a);		pxor256(ymm3_b, ymm0_b);

      config.sched_keys += 16;
      mc_pos = (mc_pos + 1) % 4; /* next mc mod 4 */
    }

  /* middle of last round */
  movdqa256_memld(&k_sbou, ymm4_a); 	movdqa256_memld(&k_sbou, ymm4_b);
  pshufb256_tab16(ymm2_a, ymm4_a);	pshufb256_tab16(ymm2_b, ymm4_b);
  broadcast128_256_amemld(config.sched_keys, ymm5);
  pxor256(ymm5, ymm4_a);		pxor256(ymm5, ymm4_b);
  movdqa256_memld(&k_sbot, ymm0_a); 	movdqa256_memld(&k_sbot, ymm0_b);
  pshufb256_tab16(ymm3_a, ymm0_a);	pshufb256_tab16(ymm3_b, ymm0_b);
  pxor256(ymm4_a, ymm0_a);		pxor256(ymm4_b, ymm0_b);
  load_tab32_mask(&k_sr[mc_pos], ymm5);
  pshufb256_tab32(ymm5, ymm0_a);	pshufb256_tab32(ymm5, ymm0_b);

  *pymm0_a = ymm0_a;
  *pymm0_b = ymm0_b;
}

#endif /* HAVE_SIMD256 */

static ASM_FUNC_ATTR_INLINE __m128i
aes_decrypt_core(__m128i xmm0, struct vp_aes_config_s config,
		 __m128i xmm9, __m128i xmm10, __m128i xmm11, __m128i xmm12,
		 __m128i xmm13, __m128i xmm14, __m128i xmm15, __m128i xmm8)
{
  __m128i xmm1, xmm2, xmm3, xmm4, xmm5;
  const byte *end_keys = config.sched_keys + 16 * config.nround;
  unsigned int mc_pos = config.nround % 4;

  movdqa128_memld(&k_dipt[0], xmm2);
  movdqa128(xmm9, xmm1);
  pandn128(xmm0, xmm1);
  psrl_byte_128(4, xmm1);
  pand128(xmm9, xmm0);
  pshufb128(xmm0, xmm2);
  movdqa128_memld(&k_dipt[1], xmm0);
  pshufb128(xmm1, xmm0);
  pxor128_amemld(config.sched_keys, xmm2);
  pxor128(xmm2, xmm0);
  movdqa128_memld(&k_mc_forward[3], xmm5);

  config.sched_keys += 16;

  while (1)
    {
      /* top of round */
      movdqa128(xmm9, xmm1);                  /* 1 : i */
      pandn128(xmm0, xmm1);                   /* 1 = i<<4 */
      psrl_byte_128(4, xmm1);                 /* 1 = i */
      pand128(xmm9, xmm0);                    /* 0 = k */
      movdqa128(xmm11, xmm2);                 /* 2 : a/k */
      pshufb128(xmm0, xmm2);                  /* 2 = a/k */
      pxor128(xmm1, xmm0);                    /* 0 = j */
      movdqa128(xmm10, xmm3);                 /* 3 : 1/i */
      pshufb128(xmm1, xmm3);                  /* 3 = 1/i */
      pxor128(xmm2, xmm3);                    /* 3 = iak = 1/i + a/k */
      movdqa128(xmm10, xmm4);                 /* 4 : 1/j */
      pshufb128(xmm0,  xmm4);                 /* 4 = 1/j */
      pxor128(xmm2, xmm4);                    /* 4 = jak = 1/j + a/k */
      movdqa128(xmm10, xmm2);                 /* 2 : 1/iak */
      pshufb128(xmm3, xmm2);                  /* 2 = 1/iak */
      pxor128(xmm0, xmm2);                    /* 2 = io */
      movdqa128(xmm10, xmm3);                 /* 3 : 1/jak */
      pshufb128(xmm4, xmm3);                  /* 3 = 1/jak */
      pxor128(xmm1, xmm3);                    /* 3 = jo */

      if (config.sched_keys == end_keys)
	break;

      /* Inverse mix columns */
      movdqa128(xmm13, xmm4);                 /* 4 : sb9u */
      pshufb128(xmm2, xmm4);                  /* 4 = sb9u */
      pxor128_amemld(config.sched_keys, xmm4);
      movdqa128(xmm12, xmm0);                 /* 0 : sb9t */
      pshufb128(xmm3, xmm0);                  /* 0 = sb9t */
      movdqa128_memld(&k_dsbd[1], xmm1);      /* 1 : sbdt */
      pxor128(xmm4, xmm0);                    /* 0 = ch */

      pshufb128(xmm5, xmm0);                  /* MC ch */
      movdqa128(xmm15, xmm4);                 /* 4 : sbdu */
      pshufb128(xmm2, xmm4);                  /* 4 = sbdu */
      pxor128(xmm0, xmm4);                    /* 4 = ch */
      pshufb128(xmm3, xmm1);                  /* 1 = sbdt */
      pxor128(xmm4, xmm1);                    /* 1 = ch */

      pshufb128(xmm5, xmm1);                  /* MC ch */
      movdqa128(xmm14, xmm4);                 /* 4 : sbbu */
      pshufb128(xmm2, xmm4);                  /* 4 = sbbu */
      pxor128(xmm1, xmm4);                    /* 4 = ch */
      movdqa128_memld(&k_dsbb[1], xmm0);      /* 0 : sbbt */
      pshufb128(xmm3, xmm0);                  /* 0 = sbbt */
      pxor128(xmm4, xmm0);                    /* 0 = ch */

      pshufb128(xmm5, xmm0);                  /* MC ch */
      movdqa128(xmm8, xmm4);                  /* 4 : sbeu */
      pshufb128(xmm2, xmm4);                  /* 4 = sbeu */
      pshufd128_0x93(xmm5, xmm5);
      pxor128(xmm0, xmm4);                    /* 4 = ch */
      movdqa128_memld(&k_dsbe[1], xmm0);      /* 0 : sbet */
      pshufb128(xmm3, xmm0);                  /* 0 = sbet */
      pxor128(xmm4, xmm0);                    /* 0 = ch */

      config.sched_keys += 16;
    }

  /* middle of last round */
  movdqa128_memld(&k_dsbo[0], xmm4);/* 3 : sbou */
  pshufb128(xmm2, xmm4);            /* 4 = sbou */
  pxor128_amemld(config.sched_keys, xmm4); /* 4 = sb1u + k */
  movdqa128_memld(&k_dsbo[1], xmm0);/* 0 : sbot */
  pshufb128(xmm3, xmm0);            /* 0 = sb1t */
  pxor128(xmm4, xmm0);              /* 0 = A */
  pshufb128_amemld(&k_sr[mc_pos], xmm0);

  return xmm0;
}

static ASM_FUNC_ATTR_INLINE void
aes_decrypt_core_2blks(__m128i *pxmm0_a, __m128i *pxmm0_b,
		       struct vp_aes_config_s config,
		       __m128i xmm9, __m128i xmm10, __m128i xmm11,
		       __m128i xmm12, __m128i xmm13, __m128i xmm14,
		       __m128i xmm15, __m128i xmm8)
{
  __m128i xmm0_a, xmm0_b;
  __m128i xmm1_a, xmm2_a, xmm3_a, xmm4_a;
  __m128i xmm1_b, xmm2_b, xmm3_b, xmm4_b;
  __m128i xmm5, xmm6;
  const byte *end_keys = config.sched_keys + 16 * config.nround;
  unsigned int mc_pos = config.nround % 4;

  xmm0_a = *pxmm0_a;
  xmm0_b = *pxmm0_b;

  movdqa128_memld(&k_dipt[0], xmm2_a); movdqa128(xmm2_a, xmm2_b);
  movdqa128(xmm9, xmm1_a);	movdqa128(xmm9, xmm1_b);
  pandn128(xmm0_a, xmm1_a);	pandn128(xmm0_b, xmm1_b);
  psrl_byte_128(4, xmm1_a);	psrl_byte_128(4, xmm1_b);
  pand128(xmm9, xmm0_a);	pand128(xmm9, xmm0_b);
  pshufb128(xmm0_a, xmm2_a);	pshufb128(xmm0_b, xmm2_b);
  movdqa128_memld(&k_dipt[1], xmm0_a); movdqa128(xmm0_a, xmm0_b);
  pshufb128(xmm1_a, xmm0_a);	pshufb128(xmm1_b, xmm0_b);
  movdqu128_memld(config.sched_keys, xmm6);
  pxor128(xmm6, xmm2_a);	pxor128(xmm6, xmm2_b);
  pxor128(xmm2_a, xmm0_a);	pxor128(xmm2_b, xmm0_b);
  movdqa128_memld(&k_mc_forward[3], xmm5);

  config.sched_keys += 16;

  while (1)
    {
      /* top of round */
      movdqa128(xmm9, xmm1_a);		movdqa128(xmm9, xmm1_b);
      pandn128(xmm0_a, xmm1_a);		pandn128(xmm0_b, xmm1_b);
      psrl_byte_128(4, xmm1_a);		psrl_byte_128(4, xmm1_b);
      pand128(xmm9, xmm0_a);		pand128(xmm9, xmm0_b);
      movdqa128(xmm11, xmm2_a);		movdqa128(xmm11, xmm2_b);
      pshufb128(xmm0_a, xmm2_a);	pshufb128(xmm0_b, xmm2_b);
      pxor128(xmm1_a, xmm0_a);		pxor128(xmm1_b, xmm0_b);
      movdqa128(xmm10, xmm3_a);		movdqa128(xmm10, xmm3_b);
      pshufb128(xmm1_a, xmm3_a);	pshufb128(xmm1_b, xmm3_b);
      pxor128(xmm2_a, xmm3_a);		pxor128(xmm2_b, xmm3_b);
      movdqa128(xmm10, xmm4_a);		movdqa128(xmm10, xmm4_b);
      pshufb128(xmm0_a, xmm4_a);	pshufb128(xmm0_b, xmm4_b);
      pxor128(xmm2_a, xmm4_a);		pxor128(xmm2_b, xmm4_b);
      movdqa128(xmm10, xmm2_a);		movdqa128(xmm10, xmm2_b);
      pshufb128(xmm3_a, xmm2_a);	pshufb128(xmm3_b, xmm2_b);
      pxor128(xmm0_a, xmm2_a);		pxor128(xmm0_b, xmm2_b);
      movdqa128(xmm10, xmm3_a);		movdqa128(xmm10, xmm3_b);
      pshufb128(xmm4_a, xmm3_a);	pshufb128(xmm4_b, xmm3_b);
      pxor128(xmm1_a, xmm3_a);		pxor128(xmm1_b, xmm3_b);

      if (config.sched_keys == end_keys)
	break;

      /* Inverse mix columns */
      movdqa128(xmm13, xmm4_a);		movdqa128(xmm13, xmm4_b);
      pshufb128(xmm2_a, xmm4_a);	pshufb128(xmm2_b, xmm4_b);
      movdqu128_memld(config.sched_keys, xmm6);
      pxor128(xmm6, xmm4_a);		pxor128(xmm6, xmm4_b);
      movdqa128(xmm12, xmm0_a);		movdqa128(xmm12, xmm0_b);
      pshufb128(xmm3_a, xmm0_a);	pshufb128(xmm3_b, xmm0_b);
      movdqa128_memld(&k_dsbd[1], xmm1_a); movdqa128(xmm1_a, xmm1_b);
      pxor128(xmm4_a, xmm0_a);		pxor128(xmm4_b, xmm0_b);

      pshufb128(xmm5, xmm0_a);		pshufb128(xmm5, xmm0_b);
      movdqa128(xmm15, xmm4_a);		movdqa128(xmm15, xmm4_b);
      pshufb128(xmm2_a, xmm4_a);	pshufb128(xmm2_b, xmm4_b);
      pxor128(xmm0_a, xmm4_a);		pxor128(xmm0_b, xmm4_b);
      pshufb128(xmm3_a, xmm1_a);	pshufb128(xmm3_b, xmm1_b);
      pxor128(xmm4_a, xmm1_a);		pxor128(xmm4_b, xmm1_b);

      pshufb128(xmm5, xmm1_a);		pshufb128(xmm5, xmm1_b);
      movdqa128(xmm14, xmm4_a);		movdqa128(xmm14, xmm4_b);
      pshufb128(xmm2_a, xmm4_a);	pshufb128(xmm2_b, xmm4_b);
      pxor128(xmm1_a, xmm4_a);		pxor128(xmm1_b, xmm4_b);
      movdqa128_memld(&k_dsbb[1], xmm0_a); movdqa128(xmm0_a, xmm0_b);
      pshufb128(xmm3_a, xmm0_a);	pshufb128(xmm3_b, xmm0_b);
      pxor128(xmm4_a, xmm0_a);		pxor128(xmm4_b, xmm0_b);

      pshufb128(xmm5, xmm0_a);		pshufb128(xmm5, xmm0_b);
      movdqa128(xmm8, xmm4_a);		movdqa128(xmm8, xmm4_b);
      pshufb128(xmm2_a, xmm4_a);	pshufb128(xmm2_b, xmm4_b);
      pshufd128_0x93(xmm5, xmm5);
      pxor128(xmm0_a, xmm4_a);		pxor128(xmm0_b, xmm4_b);
      movdqa128_memld(&k_dsbe[1], xmm0_a); movdqa128(xmm0_a, xmm0_b);
      pshufb128(xmm3_a, xmm0_a);	pshufb128(xmm3_b, xmm0_b);
      pxor128(xmm4_a, xmm0_a);		pxor128(xmm4_b, xmm0_b);

      config.sched_keys += 16;
    }

  /* middle of last round */
  movdqa128_memld(&k_dsbo[0], xmm4_a); movdqa128(xmm4_a, xmm4_b);
  pshufb128(xmm2_a, xmm4_a);	pshufb128(xmm2_b, xmm4_b);
  movdqu128_memld(config.sched_keys, xmm6);
  pxor128(xmm6, xmm4_a);	pxor128(xmm6, xmm4_b);
  movdqa128_memld(&k_dsbo[1], xmm0_a); movdqa128(xmm0_a, xmm0_b);
  pshufb128(xmm3_a, xmm0_a);	pshufb128(xmm3_b, xmm0_b);
  pxor128(xmm4_a, xmm0_a);	pxor128(xmm4_b, xmm0_b);
  movdqa128_memld(&k_sr[mc_pos], xmm5);
  pshufb128(xmm5, xmm0_a);	pshufb128(xmm5, xmm0_b);

  *pxmm0_a = xmm0_a;
  *pxmm0_b = xmm0_b;
}

#ifdef HAVE_SIMD256

static ASM_FUNC_ATTR_INLINE void
aes_decrypt_core_4blks_simd256(__m256i *pymm0_a, __m256i *pymm0_b,
			       struct vp_aes_config_s config,
			       __m128i xmm9, __m128i xmm10, __m128i xmm11,
			       __m128i xmm12, __m128i xmm13, __m128i xmm14,
			       __m128i xmm15, __m128i xmm8)
{
  __m256i ymm9, ymm10, ymm11, ymm12, ymm13, ymm14, ymm15, ymm8;
  __m256i ymm0_a, ymm0_b;
  __m256i ymm1_a, ymm2_a, ymm3_a, ymm4_a;
  __m256i ymm1_b, ymm2_b, ymm3_b, ymm4_b;
  __m256i ymm5, ymm6;
  const byte *end_keys = config.sched_keys + 16 * config.nround;
  unsigned int mc_pos = config.nround % 4;

  broadcast128_256(xmm9, ymm9);
  movdqa128_256(xmm10, ymm10);
  movdqa128_256(xmm11, ymm11);
  movdqa128_256(xmm12, ymm12);
  movdqa128_256(xmm13, ymm13);
  movdqa128_256(xmm14, ymm14);
  movdqa128_256(xmm15, ymm15);
  movdqa128_256(xmm8, ymm8);

  ymm0_a = *pymm0_a;
  ymm0_b = *pymm0_b;

  load_tab16_table(&k_dipt[0], ymm2_a); movdqa256(ymm2_a, ymm2_b);
  movdqa256(ymm9, ymm1_a);		movdqa256(ymm9, ymm1_b);
  pandn256(ymm0_a, ymm1_a);		pandn256(ymm0_b, ymm1_b);
  psrl_byte_256(4, ymm1_a);		psrl_byte_256(4, ymm1_b);
  pand256(ymm9, ymm0_a);		pand256(ymm9, ymm0_b);
  pshufb256_tab16(ymm0_a, ymm2_a);	pshufb256_tab16(ymm0_b, ymm2_b);
  load_tab16_table(&k_dipt[1], ymm0_a); movdqa256(ymm0_a, ymm0_b);
  pshufb256_tab16(ymm1_a, ymm0_a);	pshufb256_tab16(ymm1_b, ymm0_b);
  broadcast128_256_amemld(config.sched_keys, ymm6);
  pxor256(ymm6, ymm2_a);		pxor256(ymm6, ymm2_b);
  pxor256(ymm2_a, ymm0_a);		pxor256(ymm2_b, ymm0_b);
  load_tab32_mask(&k_mc_forward[3], ymm5);

  config.sched_keys += 16;

  while (1)
    {
      /* top of round */
      movdqa256(ymm9, ymm1_a);		movdqa256(ymm9, ymm1_b);
      pandn256(ymm0_a, ymm1_a);		pandn256(ymm0_b, ymm1_b);
      psrl_byte_256(4, ymm1_a);		psrl_byte_256(4, ymm1_b);
      pand256(ymm9, ymm0_a);		pand256(ymm9, ymm0_b);
      movdqa256(ymm11, ymm2_a);		movdqa256(ymm11, ymm2_b);
      pshufb256_tab16(ymm0_a, ymm2_a);	pshufb256_tab16(ymm0_b, ymm2_b);
      pxor256(ymm1_a, ymm0_a);		pxor256(ymm1_b, ymm0_b);
      movdqa256(ymm10, ymm3_a);		movdqa256(ymm10, ymm3_b);
      pshufb256_tab16(ymm1_a, ymm3_a);	pshufb256_tab16(ymm1_b, ymm3_b);
      pxor256(ymm2_a, ymm3_a);		pxor256(ymm2_b, ymm3_b);
      movdqa256(ymm10, ymm4_a);		movdqa256(ymm10, ymm4_b);
      pshufb256_tab16(ymm0_a, ymm4_a);	pshufb256_tab16(ymm0_b, ymm4_b);
      pxor256(ymm2_a, ymm4_a);		pxor256(ymm2_b, ymm4_b);
      movdqa256(ymm10, ymm2_a);		movdqa256(ymm10, ymm2_b);
      pshufb256_tab16(ymm3_a, ymm2_a);	pshufb256_tab16(ymm3_b, ymm2_b);
      pxor256(ymm0_a, ymm2_a);		pxor256(ymm0_b, ymm2_b);
      movdqa256(ymm10, ymm3_a);		movdqa256(ymm10, ymm3_b);
      pshufb256_tab16(ymm4_a, ymm3_a);	pshufb256_tab16(ymm4_b, ymm3_b);
      pxor256(ymm1_a, ymm3_a);		pxor256(ymm1_b, ymm3_b);

      if (config.sched_keys == end_keys)
	break;

      /* Inverse mix columns */
      movdqa256(ymm13, ymm4_a);		movdqa256(ymm13, ymm4_b);
      pshufb256_tab16(ymm2_a, ymm4_a);	pshufb256_tab16(ymm2_b, ymm4_b);
      broadcast128_256_amemld(config.sched_keys, ymm6);
      pxor256(ymm6, ymm4_a);		pxor256(ymm6, ymm4_b);
      movdqa256(ymm12, ymm0_a);		movdqa256(ymm12, ymm0_b);
      pshufb256_tab16(ymm3_a, ymm0_a);	pshufb256_tab16(ymm3_b, ymm0_b);
      load_tab16_table(&k_dsbd[1], ymm1_a); movdqa256(ymm1_a, ymm1_b);
      pxor256(ymm4_a, ymm0_a);		pxor256(ymm4_b, ymm0_b);

      pshufb256_tab32(ymm5, ymm0_a);	pshufb256_tab32(ymm5, ymm0_b);
      movdqa256(ymm15, ymm4_a);		movdqa256(ymm15, ymm4_b);
      pshufb256_tab16(ymm2_a, ymm4_a);	pshufb256_tab16(ymm2_b, ymm4_b);
      pxor256(ymm0_a, ymm4_a);		pxor256(ymm0_b, ymm4_b);
      pshufb256_tab16(ymm3_a, ymm1_a);	pshufb256_tab16(ymm3_b, ymm1_b);
      pxor256(ymm4_a, ymm1_a);		pxor256(ymm4_b, ymm1_b);

      pshufb256_tab32(ymm5, ymm1_a);	pshufb256_tab32(ymm5, ymm1_b);
      movdqa256(ymm14, ymm4_a);		movdqa256(ymm14, ymm4_b);
      pshufb256_tab16(ymm2_a, ymm4_a);	pshufb256_tab16(ymm2_b, ymm4_b);
      pxor256(ymm1_a, ymm4_a);		pxor256(ymm1_b, ymm4_b);
      load_tab16_table(&k_dsbb[1], ymm0_a); movdqa256(ymm0_a, ymm0_b);
      pshufb256_tab16(ymm3_a, ymm0_a);	pshufb256_tab16(ymm3_b, ymm0_b);
      pxor256(ymm4_a, ymm0_a);		pxor256(ymm4_b, ymm0_b);

      pshufb256_tab32(ymm5, ymm0_a);	pshufb256_tab32(ymm5, ymm0_b);
      movdqa256(ymm8, ymm4_a);		movdqa256(ymm8, ymm4_b);
      pshufb256_tab16(ymm2_a, ymm4_a);	pshufb256_tab16(ymm2_b, ymm4_b);
      pshufd256_0x93(ymm5, ymm5);
      pxor256(ymm0_a, ymm4_a);		pxor256(ymm0_b, ymm4_b);
      load_tab16_table(&k_dsbe[1], ymm0_a); movdqa256(ymm0_a, ymm0_b);
      pshufb256_tab16(ymm3_a, ymm0_a);	pshufb256_tab16(ymm3_b, ymm0_b);
      pxor256(ymm4_a, ymm0_a);		pxor256(ymm4_b, ymm0_b);

      config.sched_keys += 16;
    }

  /* middle of last round */
  load_tab16_table(&k_dsbo[0], ymm4_a); movdqa256(ymm4_a, ymm4_b);
  pshufb256_tab16(ymm2_a, ymm4_a);	pshufb256_tab16(ymm2_b, ymm4_b);
  broadcast128_256_amemld(config.sched_keys, ymm6);
  pxor256(ymm6, ymm4_a);		pxor256(ymm6, ymm4_b);
  load_tab16_table(&k_dsbo[1], ymm0_a); movdqa256(ymm0_a, ymm0_b);
  pshufb256_tab16(ymm3_a, ymm0_a);	pshufb256_tab16(ymm3_b, ymm0_b);
  pxor256(ymm4_a, ymm0_a);		pxor256(ymm4_b, ymm0_b);
  load_tab32_mask(&k_sr[mc_pos], ymm5);
  pshufb256_tab16(ymm5, ymm0_a);	pshufb256_tab16(ymm5, ymm0_b);

  *pymm0_a = ymm0_a;
  *pymm0_b = ymm0_b;
}

#endif /* HAVE_SIMD256 */

ASM_FUNC_ATTR_NOINLINE unsigned int
FUNC_ENCRYPT (const RIJNDAEL_context *ctx, unsigned char *dst,
              const unsigned char *src)
{
  __m128i xmm0, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15;
  struct vp_aes_config_s config;

  config.nround = ctx->rounds;
  config.sched_keys = ctx->keyschenc[0][0];

  enc_preload(xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15);

  movdqu128_memld(src, xmm0);

  xmm0 = aes_encrypt_core(xmm0, config,
			  xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15);

  movdqu128_memst(xmm0, dst);

  clear_vec_regs();

  return 0;
}

ASM_FUNC_ATTR_NOINLINE unsigned int
FUNC_DECRYPT (const RIJNDAEL_context *ctx, unsigned char *dst,
              const unsigned char *src)
{
  __m128i xmm0, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15, xmm8;
  struct vp_aes_config_s config;

  config.nround = ctx->rounds;
  config.sched_keys = ctx->keyschdec[0][0];

  dec_preload(xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15, xmm8);

  movdqu128_memld(src, xmm0);

  xmm0 = aes_decrypt_core(xmm0, config,
			  xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15, xmm8);

  movdqu128_memst(xmm0, dst);

  clear_vec_regs();

  return 0;
}

ASM_FUNC_ATTR_NOINLINE void
FUNC_CFB_ENC (RIJNDAEL_context *ctx, unsigned char *iv,
	      unsigned char *outbuf, const unsigned char *inbuf,
	      size_t nblocks)
{
  __m128i xmm0, xmm1, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15;
  struct vp_aes_config_s config;

  config.nround = ctx->rounds;
  config.sched_keys = ctx->keyschenc[0][0];

  enc_preload(xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15);

  movdqu128_memld(iv, xmm0);

  for (; nblocks; nblocks--)
    {
      xmm0 = aes_encrypt_core(xmm0, config,
			      xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15);

      movdqu128_memld(inbuf, xmm1);
      pxor128(xmm1, xmm0);
      movdqu128_memst(xmm0, outbuf);

      outbuf += BLOCKSIZE;
      inbuf  += BLOCKSIZE;
    }

  movdqu128_memst(xmm0, iv);

  clear_vec_regs();
}

ASM_FUNC_ATTR_NOINLINE void
FUNC_CBC_ENC (RIJNDAEL_context *ctx, unsigned char *iv,
	      unsigned char *outbuf, const unsigned char *inbuf,
	      size_t nblocks, int cbc_mac)
{
  __m128i xmm0, xmm7, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15;
  struct vp_aes_config_s config;
  size_t outbuf_add = (!cbc_mac) * BLOCKSIZE;

  config.nround = ctx->rounds;
  config.sched_keys = ctx->keyschenc[0][0];

  enc_preload(xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15);

  movdqu128_memld(iv, xmm7);

  for (; nblocks; nblocks--)
    {
      movdqu128_memld(inbuf, xmm0);
      pxor128(xmm7, xmm0);

      xmm0 = aes_encrypt_core(xmm0, config,
			      xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15);

      movdqa128(xmm0, xmm7);
      movdqu128_memst(xmm0, outbuf);

      inbuf += BLOCKSIZE;
      outbuf += outbuf_add;
    }

  movdqu128_memst(xmm7, iv);

  clear_vec_regs();
}

ASM_FUNC_ATTR_NOINLINE void
FUNC_CTR_ENC (RIJNDAEL_context *ctx, unsigned char *ctr,
	      unsigned char *outbuf, const unsigned char *inbuf,
	      size_t nblocks)
{
  __m128i xmm0, xmm1, xmm2, xmm3, xmm6, xmm7, xmm8;
  __m128i xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15;
  static const __m128i_const be_mask =
    M128I_BYTE(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
  static const __m128i_const bigendian_add =
    M128I_BYTE(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);
  static const __m128i_const littleendian_add = M128I_U64(1, 0);
  u64 ctrlow = buf_get_be64(ctr + 8);
  struct vp_aes_config_s config;

  config.nround = ctx->rounds;
  config.sched_keys = ctx->keyschenc[0][0];

  enc_preload(xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15);

  movdqa128_memld(&bigendian_add, xmm8); /* Preload byte add */
  movdqu128_memld(ctr, xmm7); /* Preload CTR */
  movdqa128_memld(&be_mask, xmm6); /* Preload mask */

#ifdef HAVE_SIMD256
  if (check_simd256_support())
    {
      __m256i ymm0, ymm1, ymm2, ymm3;

      for (; nblocks >= 4; nblocks -= 4)
	{
	  movdqa128_256(xmm7, ymm0);

	  /* detect if 8-bit carry handling is needed */
	  if (UNLIKELY(((ctrlow += 4) & 0xff) <= 3))
	    {
	      pshufb128(xmm6, xmm7);
	      paddd128_amemld(&littleendian_add, xmm7);
	      movdqa128(xmm7, xmm2);
	      pshufb128(xmm6, xmm2);
	      insert256_hi128(xmm2, ymm0);
	      paddd128_amemld(&littleendian_add, xmm7);
	      movdqa128(xmm7, xmm2);
	      pshufb128(xmm6, xmm2);
	      movdqa128_256(xmm2, ymm1);
	      paddd128_amemld(&littleendian_add, xmm7);
	      movdqa128(xmm7, xmm2);
	      pshufb128(xmm6, xmm2);
	      insert256_hi128(xmm2, ymm1);
	      paddd128_amemld(&littleendian_add, xmm7);
	      pshufb128(xmm6, xmm7);
	    }
	  else
	    {
	      paddb128(xmm8, xmm7);
	      insert256_hi128(xmm7, ymm0);
	      paddb128(xmm8, xmm7);
	      movdqa128_256(xmm7, ymm1);
	      paddb128(xmm8, xmm7);
	      insert256_hi128(xmm7, ymm1);
	      paddb128(xmm8, xmm7);
	    }

	  aes_encrypt_core_4blks_simd256(&ymm0, &ymm1, config,
					xmm9, xmm10, xmm11, xmm12, xmm13, xmm14,
					xmm15);

	  movdqu256_memld(inbuf + 0 * BLOCKSIZE, ymm2);
	  movdqu256_memld(inbuf + 2 * BLOCKSIZE, ymm3);
	  pxor256(ymm2, ymm0);
	  pxor256(ymm3, ymm1);
	  movdqu256_memst(ymm0, outbuf + 0 * BLOCKSIZE);
	  movdqu256_memst(ymm1, outbuf + 2 * BLOCKSIZE);

	  outbuf += 4 * BLOCKSIZE;
	  inbuf  += 4 * BLOCKSIZE;
	}
    }
#endif /* HAVE_SIMD256 */

  for (; nblocks >= 2; nblocks -= 2)
    {
      movdqa128(xmm7, xmm0);

      /* detect if 8-bit carry handling is needed */
      if (UNLIKELY(((ctrlow += 2) & 0xff) <= 1))
	{
	  pshufb128(xmm6, xmm7);
	  paddd128_amemld(&littleendian_add, xmm7);
	  movdqa128(xmm7, xmm1);
	  paddq128_amemld(&littleendian_add, xmm7);
	  pshufb128(xmm6, xmm1);
	  pshufb128(xmm6, xmm7);
	}
      else
	{
	  paddb128(xmm8, xmm7);
	  movdqa128(xmm7, xmm1);
	  paddb128(xmm8, xmm7);
	}

      aes_encrypt_core_2blks(&xmm0, &xmm1, config,
			      xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15);

      movdqu128_memld(inbuf, xmm2);
      movdqu128_memld(inbuf + BLOCKSIZE, xmm3);
      pxor128(xmm2, xmm0);
      pxor128(xmm3, xmm1);
      movdqu128_memst(xmm0, outbuf);
      movdqu128_memst(xmm1, outbuf + BLOCKSIZE);

      outbuf += 2 * BLOCKSIZE;
      inbuf  += 2 * BLOCKSIZE;
    }

  for (; nblocks; nblocks--)
    {
      movdqa128(xmm7, xmm0);

      /* detect if 8-bit carry handling is needed */
      if (UNLIKELY((++ctrlow & 0xff) == 0))
	{
	  pshufb128(xmm6, xmm7);
	  paddd128_amemld(&littleendian_add, xmm7);
	  pshufb128(xmm6, xmm7);
	}
      else
	{
	  paddb128(xmm8, xmm7);
	}

      xmm0 = aes_encrypt_core(xmm0, config,
			      xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15);

      movdqu128_memld(inbuf, xmm1);
      pxor128(xmm1, xmm0);
      movdqu128_memst(xmm0, outbuf);

      outbuf += BLOCKSIZE;
      inbuf  += BLOCKSIZE;
    }

  movdqu128_memst(xmm7, ctr);

  clear_vec_regs();
}

ASM_FUNC_ATTR_NOINLINE void
FUNC_CTR32LE_ENC (RIJNDAEL_context *ctx, unsigned char *ctr,
		  unsigned char *outbuf, const unsigned char *inbuf,
		  size_t nblocks)
{
  __m128i xmm0, xmm1, xmm2, xmm3, xmm7, xmm8;
  __m128i xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15;
  static const __m128i_const add_one = M128I_U64(1, 0);
  static const __m128i_const add_two = M128I_U64(2, 0);
  struct vp_aes_config_s config;

  config.nround = ctx->rounds;
  config.sched_keys = ctx->keyschenc[0][0];

  enc_preload(xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15);

  movdqa128_memld(&add_one, xmm8); /* Preload byte add */
  movdqu128_memld(ctr, xmm7); /* Preload CTR */

#ifdef HAVE_SIMD256
  if (check_simd256_support())
    {
      __m256i ymm0, ymm1, ymm2, ymm3;

      for (; nblocks >= 4; nblocks -= 4)
	{
	  movdqa128(xmm7, xmm0);
	  movdqa128(xmm7, xmm1);
	  paddd128(xmm8, xmm1);
	  paddd128_amemld(&add_two, xmm7);
	  movdqa128_256(xmm0, ymm0);
	  insert256_hi128(xmm1, ymm0);

	  movdqa128(xmm7, xmm1);
	  movdqa128(xmm7, xmm2);
	  paddd128(xmm8, xmm2);
	  paddd128_amemld(&add_two, xmm7);
	  movdqa128_256(xmm1, ymm1);
	  insert256_hi128(xmm2, ymm1);

	  aes_encrypt_core_4blks_simd256(&ymm0, &ymm1, config,
					 xmm9, xmm10, xmm11, xmm12, xmm13,
					 xmm14, xmm15);

	  movdqu256_memld(inbuf + 0 * BLOCKSIZE, ymm2);
	  movdqu256_memld(inbuf + 2 * BLOCKSIZE, ymm3);
	  pxor256(ymm2, ymm0);
	  pxor256(ymm3, ymm1);
	  movdqu256_memst(ymm0, outbuf + 0 * BLOCKSIZE);
	  movdqu256_memst(ymm1, outbuf + 2 * BLOCKSIZE);

	  outbuf += 4 * BLOCKSIZE;
	  inbuf  += 4 * BLOCKSIZE;
	}
    }
#endif /* HAVE_SIMD256 */

  for (; nblocks >= 2; nblocks -= 2)
    {
      movdqa128(xmm7, xmm0);
      movdqa128(xmm7, xmm1);
      paddd128(xmm8, xmm1);
      paddd128_amemld(&add_two, xmm7);

      aes_encrypt_core_2blks(&xmm0, &xmm1, config,
			      xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15);

      movdqu128_memld(inbuf, xmm2);
      movdqu128_memld(inbuf + BLOCKSIZE, xmm3);
      pxor128(xmm2, xmm0);
      pxor128(xmm3, xmm1);
      movdqu128_memst(xmm0, outbuf);
      movdqu128_memst(xmm1, outbuf + BLOCKSIZE);

      outbuf += 2 * BLOCKSIZE;
      inbuf  += 2 * BLOCKSIZE;
    }

  for (; nblocks; nblocks--)
    {
      movdqa128(xmm7, xmm0);
      paddd128(xmm8, xmm7);

      xmm0 = aes_encrypt_core(xmm0, config,
			      xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15);

      movdqu128_memld(inbuf, xmm1);
      pxor128(xmm1, xmm0);
      movdqu128_memst(xmm0, outbuf);

      outbuf += BLOCKSIZE;
      inbuf  += BLOCKSIZE;
    }

  movdqu128_memst(xmm7, ctr);

  clear_vec_regs();
}

ASM_FUNC_ATTR_NOINLINE void
FUNC_CFB_DEC (RIJNDAEL_context *ctx, unsigned char *iv,
	      unsigned char *outbuf, const unsigned char *inbuf,
	      size_t nblocks)
{
  __m128i xmm0, xmm1, xmm2, xmm6, xmm9;
  __m128i xmm10, xmm11, xmm12, xmm13, xmm14, xmm15;
  struct vp_aes_config_s config;

  config.nround = ctx->rounds;
  config.sched_keys = ctx->keyschenc[0][0];

  enc_preload(xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15);

  movdqu128_memld(iv, xmm0);

#ifdef HAVE_SIMD256
  if (check_simd256_support())
    {
      __m256i ymm6, ymm1, ymm2, ymm3;

      for (; nblocks >= 4; nblocks -= 4)
	{
	  movdqa128_256(xmm0, ymm6);
	  movdqu256_memld(inbuf + 0 * BLOCKSIZE, ymm2);
	  movdqa256_128(ymm2, xmm2);
	  insert256_hi128(xmm2, ymm6);
	  movdqu256_memld(inbuf + 1 * BLOCKSIZE, ymm1);
	  movdqu256_memld(inbuf + 2 * BLOCKSIZE, ymm3);
	  extract256_hi128(ymm3, xmm0);

	  aes_encrypt_core_4blks_simd256(&ymm6, &ymm1, config,
					 xmm9, xmm10, xmm11, xmm12, xmm13,
					 xmm14, xmm15);

	  pxor256(ymm2, ymm6);
	  pxor256(ymm3, ymm1);
	  movdqu256_memst(ymm6, outbuf + 0 * BLOCKSIZE);
	  movdqu256_memst(ymm1, outbuf + 2 * BLOCKSIZE);

	  outbuf += 4 * BLOCKSIZE;
	  inbuf  += 4 * BLOCKSIZE;
	}
    }
#endif /* HAVE_SIMD256 */

  for (; nblocks >= 2; nblocks -= 2)
    {
      movdqa128(xmm0, xmm1);
      movdqu128_memld(inbuf, xmm2);
      movdqu128_memld(inbuf + BLOCKSIZE, xmm0);
      movdqa128(xmm2, xmm6);

      aes_encrypt_core_2blks(&xmm1, &xmm2, config,
			      xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15);

      pxor128(xmm6, xmm1);
      pxor128(xmm0, xmm2);
      movdqu128_memst(xmm1, outbuf);
      movdqu128_memst(xmm2, outbuf + BLOCKSIZE);

      outbuf += 2 * BLOCKSIZE;
      inbuf  += 2 * BLOCKSIZE;
    }

  for (; nblocks; nblocks--)
    {
      xmm0 = aes_encrypt_core(xmm0, config,
			      xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15);

      movdqa128(xmm0, xmm6);
      movdqu128_memld(inbuf, xmm0);
      pxor128(xmm0, xmm6);
      movdqu128_memst(xmm6, outbuf);

      outbuf += BLOCKSIZE;
      inbuf  += BLOCKSIZE;
    }

  movdqu128_memst(xmm0, iv);

  clear_vec_regs();
}

ASM_FUNC_ATTR_NOINLINE void
FUNC_CBC_DEC (RIJNDAEL_context *ctx, unsigned char *iv,
	      unsigned char *outbuf, const unsigned char *inbuf,
	      size_t nblocks)
{
  __m128i xmm0, xmm1, xmm5, xmm6, xmm7;
  __m128i xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15, xmm8;
  struct vp_aes_config_s config;

  if (!ctx->decryption_prepared)
    {
      FUNC_PREPARE_DEC (ctx);
      ctx->decryption_prepared = 1;
    }

  config.nround = ctx->rounds;
  config.sched_keys = ctx->keyschdec[0][0];

  dec_preload(xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15, xmm8);

  movdqu128_memld(iv, xmm7);

#ifdef HAVE_SIMD256
  if (check_simd256_support())
    {
      __m256i ymm0, ymm1, ymm2, ymm3;

      for (; nblocks >= 4; nblocks -= 4)
	{
	  movdqu256_memld(inbuf + 0 * BLOCKSIZE, ymm0);
	  movdqu256_memld(inbuf + 2 * BLOCKSIZE, ymm1);
	  movdqa256_128(ymm0, xmm0);
	  movdqa128_256(xmm7, ymm2);
	  insert256_hi128(xmm0, ymm2);
	  movdqu256_memld(inbuf + 1 * BLOCKSIZE, ymm3);
	  extract256_hi128(ymm1, xmm7);

	  aes_decrypt_core_4blks_simd256(&ymm0, &ymm1, config,
					 xmm9, xmm10, xmm11, xmm12, xmm13,
					 xmm14, xmm15, xmm8);

	  pxor256(ymm2, ymm0);
	  pxor256(ymm3, ymm1);
	  movdqu256_memst(ymm0, outbuf + 0 * BLOCKSIZE);
	  movdqu256_memst(ymm1, outbuf + 2 * BLOCKSIZE);

	  outbuf += 4 * BLOCKSIZE;
	  inbuf  += 4 * BLOCKSIZE;
	}
    }
#endif /* HAVE_SIMD256 */

  for (; nblocks >= 2; nblocks -= 2)
    {
      movdqu128_memld(inbuf, xmm0);
      movdqu128_memld(inbuf + BLOCKSIZE, xmm1);
      movdqa128(xmm0, xmm5);
      movdqa128(xmm1, xmm6);

      aes_decrypt_core_2blks(&xmm0, &xmm1, config,
			     xmm9, xmm10, xmm11, xmm12, xmm13, xmm14,
			     xmm15, xmm8);

      pxor128(xmm7, xmm0);
      pxor128(xmm5, xmm1);
      movdqu128_memst(xmm0, outbuf);
      movdqu128_memst(xmm1, outbuf + BLOCKSIZE);
      movdqa128(xmm6, xmm7);

      outbuf += 2 * BLOCKSIZE;
      inbuf  += 2 * BLOCKSIZE;
    }

  for (; nblocks; nblocks--)
    {
      movdqu128_memld(inbuf, xmm0);
      movdqa128(xmm0, xmm6);

      xmm0 = aes_decrypt_core(xmm0, config,
			      xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15,
			      xmm8);

      pxor128(xmm7, xmm0);
      movdqu128_memst(xmm0, outbuf);
      movdqa128(xmm6, xmm7);

      outbuf += BLOCKSIZE;
      inbuf  += BLOCKSIZE;
    }

  movdqu128_memst(xmm7, iv);

  clear_vec_regs();
}

static ASM_FUNC_ATTR_NOINLINE size_t
aes_simd128_ocb_enc (gcry_cipher_hd_t c, void *outbuf_arg,
		     const void *inbuf_arg, size_t nblocks)
{
  __m128i xmm0, xmm1, xmm2, xmm3, xmm6, xmm7;
  __m128i xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15;
  RIJNDAEL_context *ctx = (void *)&c->context.c;
  unsigned char *outbuf = outbuf_arg;
  const unsigned char *inbuf = inbuf_arg;
  u64 n = c->u_mode.ocb.data_nblocks;
  struct vp_aes_config_s config;

  config.nround = ctx->rounds;
  config.sched_keys = ctx->keyschenc[0][0];

  enc_preload(xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15);

  /* Preload Offset and Checksum */
  movdqu128_memld(c->u_iv.iv, xmm7);
  movdqu128_memld(c->u_ctr.ctr, xmm6);

#ifdef HAVE_SIMD256
  if (check_simd256_support() && nblocks >= 4)
    {
      __m256i ymm0, ymm1, ymm3, ymm6, ymm8;

      movdqa128_256(xmm6, ymm6);

      for (; nblocks >= 4; nblocks -= 4)
	{
	  const unsigned char *l;

	  movdqu256_memld(inbuf + 0 * BLOCKSIZE, ymm0);
	  movdqu256_memld(inbuf + 2 * BLOCKSIZE, ymm1);

	  /* Offset_i = Offset_{i-1} xor L_{ntz(i)} */
	  /* C_i = Offset_i xor ENCIPHER(K, P_i xor Offset_i)  */
	  l = ocb_get_l(c, ++n);
	  movdqu128_memld(l, xmm2);
	  pxor128(xmm2, xmm7);
	  movdqa128_256(xmm7, ymm3);

	  l = ocb_get_l(c, ++n);
	  movdqu128_memld(l, xmm2);
	  pxor128(xmm2, xmm7);
	  insert256_hi128(xmm7, ymm3);

	  l = ocb_get_l(c, ++n);
	  movdqu128_memld(l, xmm2);
	  pxor128(xmm2, xmm7);
	  movdqa128_256(xmm7, ymm8);

	  l = ocb_get_l(c, ++n);
	  movdqu128_memld(l, xmm2);
	  pxor128(xmm2, xmm7);
	  insert256_hi128(xmm7, ymm8);

	  /* Checksum_i = Checksum_{i-1} xor P_i  */
	  pxor256(ymm0, ymm6);
	  pxor256(ymm1, ymm6);

	  pxor256(ymm3, ymm0);
	  pxor256(ymm8, ymm1);

	  aes_encrypt_core_4blks_simd256(&ymm0, &ymm1, config,
					 xmm9, xmm10, xmm11, xmm12, xmm13,
					 xmm14, xmm15);

	  pxor256(ymm3, ymm0);
	  pxor256(ymm8, ymm1);
	  movdqu256_memst(ymm0, outbuf + 0 * BLOCKSIZE);
	  movdqu256_memst(ymm1, outbuf + 2 * BLOCKSIZE);

	  inbuf += 4 * BLOCKSIZE;
	  outbuf += 4 * BLOCKSIZE;
	}

      extract256_hi128(ymm6, xmm0);
      movdqa256_128(ymm6, xmm6);
      pxor128(xmm0, xmm6);
    }
#endif /* HAVE_SIMD256 */

  for (; nblocks >= 2; nblocks -= 2)
    {
      const unsigned char *l;

      /* Offset_i = Offset_{i-1} xor L_{ntz(i)} */
      /* Checksum_i = Checksum_{i-1} xor P_i  */
      /* C_i = Offset_i xor ENCIPHER(K, P_i xor Offset_i)  */
      l = ocb_get_l(c, ++n);
      movdqu128_memld(l, xmm2);
      movdqu128_memld(inbuf, xmm0);
      movdqu128_memld(inbuf + BLOCKSIZE, xmm1);
      movdqa128(xmm7, xmm3);
      pxor128(xmm2, xmm3);
      pxor128(xmm0, xmm6);
      pxor128(xmm3, xmm0);

      /* Offset_i = Offset_{i-1} xor L_{ntz(i)} */
      /* Checksum_i = Checksum_{i-1} xor P_i  */
      /* C_i = Offset_i xor ENCIPHER(K, P_i xor Offset_i)  */
      l = ocb_get_l(c, ++n);
      movdqu128_memld(l, xmm2);
      movdqa128(xmm3, xmm7);
      pxor128(xmm2, xmm7);
      pxor128(xmm1, xmm6);
      pxor128(xmm7, xmm1);

      aes_encrypt_core_2blks(&xmm0, &xmm1, config,
			      xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15);

      pxor128(xmm3, xmm0);
      pxor128(xmm7, xmm1);
      movdqu128_memst(xmm0, outbuf);
      movdqu128_memst(xmm1, outbuf + BLOCKSIZE);

      inbuf += 2 * BLOCKSIZE;
      outbuf += 2 * BLOCKSIZE;
    }

  for (; nblocks; nblocks--)
    {
      const unsigned char *l;

      l = ocb_get_l(c, ++n);

      /* Offset_i = Offset_{i-1} xor L_{ntz(i)} */
      /* Checksum_i = Checksum_{i-1} xor P_i  */
      /* C_i = Offset_i xor ENCIPHER(K, P_i xor Offset_i)  */
      movdqu128_memld(l, xmm1);
      movdqu128_memld(inbuf, xmm0);
      pxor128(xmm1, xmm7);
      pxor128(xmm0, xmm6);
      pxor128(xmm7, xmm0);

      xmm0 = aes_encrypt_core(xmm0, config,
			      xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15);

      pxor128(xmm7, xmm0);
      movdqu128_memst(xmm0, outbuf);

      inbuf += BLOCKSIZE;
      outbuf += BLOCKSIZE;
    }

  c->u_mode.ocb.data_nblocks = n;
  movdqu128_memst(xmm7, c->u_iv.iv);
  movdqu128_memst(xmm6, c->u_ctr.ctr);

  clear_vec_regs();

  return 0;
}

static ASM_FUNC_ATTR_NOINLINE size_t
aes_simd128_ocb_dec (gcry_cipher_hd_t c, void *outbuf_arg,
		     const void *inbuf_arg, size_t nblocks)
{
  __m128i xmm0, xmm1, xmm2, xmm3, xmm6, xmm7;
  __m128i xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15, xmm8;
  RIJNDAEL_context *ctx = (void *)&c->context.c;
  unsigned char *outbuf = outbuf_arg;
  const unsigned char *inbuf = inbuf_arg;
  u64 n = c->u_mode.ocb.data_nblocks;
  struct vp_aes_config_s config;

  if (!ctx->decryption_prepared)
    {
      FUNC_PREPARE_DEC (ctx);
      ctx->decryption_prepared = 1;
    }

  config.nround = ctx->rounds;
  config.sched_keys = ctx->keyschdec[0][0];

  dec_preload(xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15, xmm8);

  /* Preload Offset and Checksum */
  movdqu128_memld(c->u_iv.iv, xmm7);
  movdqu128_memld(c->u_ctr.ctr, xmm6);

#ifdef HAVE_SIMD256
  if (check_simd256_support() && nblocks >= 4)
    {
      __m256i ymm0, ymm1, ymm3, ymm6, ymm8;

      movdqa128_256(xmm6, ymm6);

      for (; nblocks >= 4; nblocks -= 4)
	{
	  const unsigned char *l;

	  movdqu256_memld(inbuf + 0 * BLOCKSIZE, ymm0);
	  movdqu256_memld(inbuf + 2 * BLOCKSIZE, ymm1);

	  /* Offset_i = Offset_{i-1} xor L_{ntz(i)} */
	  /* C_i = Offset_i xor ENCIPHER(K, P_i xor Offset_i)  */
	  l = ocb_get_l(c, ++n);
	  movdqu128_memld(l, xmm2);
	  pxor128(xmm2, xmm7);
	  movdqa128_256(xmm7, ymm3);

	  l = ocb_get_l(c, ++n);
	  movdqu128_memld(l, xmm2);
	  pxor128(xmm2, xmm7);
	  insert256_hi128(xmm7, ymm3);

	  l = ocb_get_l(c, ++n);
	  movdqu128_memld(l, xmm2);
	  pxor128(xmm2, xmm7);
	  movdqa128_256(xmm7, ymm8);

	  l = ocb_get_l(c, ++n);
	  movdqu128_memld(l, xmm2);
	  pxor128(xmm2, xmm7);
	  insert256_hi128(xmm7, ymm8);

	  pxor256(ymm3, ymm0);
	  pxor256(ymm8, ymm1);

	  aes_decrypt_core_4blks_simd256(&ymm0, &ymm1, config,
					 xmm9, xmm10, xmm11, xmm12, xmm13,
					 xmm14, xmm15, xmm8);

	  pxor256(ymm3, ymm0);
	  pxor256(ymm8, ymm1);

	  /* Checksum_i = Checksum_{i-1} xor P_i  */
	  pxor256(ymm0, ymm6);
	  pxor256(ymm1, ymm6);

	  movdqu256_memst(ymm0, outbuf + 0 * BLOCKSIZE);
	  movdqu256_memst(ymm1, outbuf + 2 * BLOCKSIZE);

	  inbuf += 4 * BLOCKSIZE;
	  outbuf += 4 * BLOCKSIZE;
	}

      extract256_hi128(ymm6, xmm0);
      movdqa256_128(ymm6, xmm6);
      pxor128(xmm0, xmm6);
    }
#endif /* HAVE_SIMD256 */

  for (; nblocks >= 2; nblocks -= 2)
    {
      const unsigned char *l;

      /* Offset_i = Offset_{i-1} xor L_{ntz(i)} */
      /* P_i = Offset_i xor DECIPHER(K, C_i xor Offset_i)  */
      /* Checksum_i = Checksum_{i-1} xor P_i  */
      l = ocb_get_l(c, ++n);
      movdqu128_memld(l, xmm2);
      movdqu128_memld(inbuf, xmm0);
      movdqu128_memld(inbuf + BLOCKSIZE, xmm1);
      movdqa128(xmm7, xmm3);
      pxor128(xmm2, xmm3);
      pxor128(xmm3, xmm0);

      /* Offset_i = Offset_{i-1} xor L_{ntz(i)} */
      /* P_i = Offset_i xor DECIPHER(K, C_i xor Offset_i)  */
      /* Checksum_i = Checksum_{i-1} xor P_i  */
      l = ocb_get_l(c, ++n);
      movdqu128_memld(l, xmm2);
      movdqa128(xmm3, xmm7);
      pxor128(xmm2, xmm7);
      pxor128(xmm7, xmm1);

      aes_decrypt_core_2blks(&xmm0, &xmm1, config,
			     xmm9, xmm10, xmm11, xmm12, xmm13, xmm14,
			     xmm15, xmm8);

      pxor128(xmm3, xmm0);
      pxor128(xmm7, xmm1);
      pxor128(xmm0, xmm6);
      pxor128(xmm1, xmm6);
      movdqu128_memst(xmm0, outbuf);
      movdqu128_memst(xmm1, outbuf + BLOCKSIZE);

      inbuf += 2 * BLOCKSIZE;
      outbuf += 2 * BLOCKSIZE;
    }

  for (; nblocks; nblocks--)
    {
      const unsigned char *l;

      /* Offset_i = Offset_{i-1} xor L_{ntz(i)} */
      /* P_i = Offset_i xor DECIPHER(K, C_i xor Offset_i)  */
      /* Checksum_i = Checksum_{i-1} xor P_i  */
      l = ocb_get_l(c, ++n);
      movdqu128_memld(l, xmm1);
      movdqu128_memld(inbuf, xmm0);
      pxor128(xmm1, xmm7);
      pxor128(xmm7, xmm0);

      xmm0 = aes_decrypt_core(xmm0, config,
			      xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15,
			      xmm8);

      pxor128(xmm7, xmm0);
      pxor128(xmm0, xmm6);
      movdqu128_memst(xmm0, outbuf);

      inbuf += BLOCKSIZE;
      outbuf += BLOCKSIZE;
    }

  c->u_mode.ocb.data_nblocks = n;
  movdqu128_memst(xmm7, c->u_iv.iv);
  movdqu128_memst(xmm6, c->u_ctr.ctr);

  clear_vec_regs();

  return 0;
}

ASM_FUNC_ATTR_NOINLINE size_t
FUNC_OCB_CRYPT(gcry_cipher_hd_t c, void *outbuf_arg,
	       const void *inbuf_arg, size_t nblocks, int encrypt)
{
  if (encrypt)
    return aes_simd128_ocb_enc(c, outbuf_arg, inbuf_arg, nblocks);
  else
    return aes_simd128_ocb_dec(c, outbuf_arg, inbuf_arg, nblocks);
}

ASM_FUNC_ATTR_NOINLINE size_t
FUNC_OCB_AUTH(gcry_cipher_hd_t c, const void *abuf_arg, size_t nblocks)
{
  __m128i xmm0, xmm1, xmm2, xmm6, xmm7;
  __m128i xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15;
  RIJNDAEL_context *ctx = (void *)&c->context.c;
  const unsigned char *abuf = abuf_arg;
  u64 n = c->u_mode.ocb.aad_nblocks;
  struct vp_aes_config_s config;

  config.nround = ctx->rounds;
  config.sched_keys = ctx->keyschenc[0][0];

  enc_preload(xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15);

  /* Preload Offset and Sum */
  movdqu128_memld(c->u_mode.ocb.aad_offset, xmm7);
  movdqu128_memld(c->u_mode.ocb.aad_sum, xmm6);

#ifdef HAVE_SIMD256
  if (check_simd256_support() && nblocks >= 4)
    {
      __m256i ymm0, ymm1, ymm3, ymm6, ymm8;

      movdqa128_256(xmm6, ymm6);

      for (; nblocks >= 4; nblocks -= 4)
	{
	  const unsigned char *l;

	  movdqu256_memld(abuf + 0 * BLOCKSIZE, ymm0);
	  movdqu256_memld(abuf + 2 * BLOCKSIZE, ymm1);

	  /* Offset_i = Offset_{i-1} xor L_{ntz(i)} */
	  /* Sum_i = Sum_{i-1} xor ENCIPHER(K, A_i xor Offset_i)  */
	  l = ocb_get_l(c, ++n);
	  movdqu128_memld(l, xmm2);
	  pxor128(xmm2, xmm7);
	  movdqa128_256(xmm7, ymm3);

	  l = ocb_get_l(c, ++n);
	  movdqu128_memld(l, xmm2);
	  pxor128(xmm2, xmm7);
	  insert256_hi128(xmm7, ymm3);

	  l = ocb_get_l(c, ++n);
	  movdqu128_memld(l, xmm2);
	  pxor128(xmm2, xmm7);
	  movdqa128_256(xmm7, ymm8);

	  l = ocb_get_l(c, ++n);
	  movdqu128_memld(l, xmm2);
	  pxor128(xmm2, xmm7);
	  insert256_hi128(xmm7, ymm8);

	  pxor256(ymm3, ymm0);
	  pxor256(ymm8, ymm1);

	  aes_encrypt_core_4blks_simd256(&ymm0, &ymm1, config,
					 xmm9, xmm10, xmm11, xmm12, xmm13,
					 xmm14, xmm15);

	  pxor256(ymm0, ymm6);
	  pxor256(ymm1, ymm6);

	  abuf += 4 * BLOCKSIZE;
	}

      extract256_hi128(ymm6, xmm0);
      movdqa256_128(ymm6, xmm6);
      pxor128(xmm0, xmm6);
    }
#endif /* HAVE_SIMD256 */

  for (; nblocks >= 2; nblocks -= 2)
    {
      const unsigned char *l;

      /* Offset_i = Offset_{i-1} xor L_{ntz(i)} */
      /* Sum_i = Sum_{i-1} xor ENCIPHER(K, A_i xor Offset_i)  */
      l = ocb_get_l(c, ++n);
      movdqu128_memld(l, xmm2);
      movdqu128_memld(abuf, xmm0);
      movdqu128_memld(abuf + BLOCKSIZE, xmm1);
      pxor128(xmm2, xmm7);
      pxor128(xmm7, xmm0);

      /* Offset_i = Offset_{i-1} xor L_{ntz(i)} */
      /* Sum_i = Sum_{i-1} xor ENCIPHER(K, A_i xor Offset_i)  */
      l = ocb_get_l(c, ++n);
      movdqu128_memld(l, xmm2);
      pxor128(xmm2, xmm7);
      pxor128(xmm7, xmm1);

      aes_encrypt_core_2blks(&xmm0, &xmm1, config,
			      xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15);

      pxor128(xmm0, xmm6);
      pxor128(xmm1, xmm6);

      abuf += 2 * BLOCKSIZE;
    }

  for (; nblocks; nblocks--)
    {
      const unsigned char *l;

      /* Offset_i = Offset_{i-1} xor L_{ntz(i)} */
      /* Sum_i = Sum_{i-1} xor ENCIPHER(K, A_i xor Offset_i)  */
      l = ocb_get_l(c, ++n);
      movdqu128_memld(l, xmm1);
      movdqu128_memld(abuf, xmm0);
      pxor128(xmm1, xmm7);
      pxor128(xmm7, xmm0);

      xmm0 = aes_encrypt_core(xmm0, config,
			      xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15);

      pxor128(xmm0, xmm6);

      abuf += BLOCKSIZE;
    }

  c->u_mode.ocb.aad_nblocks = n;
  movdqu128_memst(xmm7, c->u_mode.ocb.aad_offset);
  movdqu128_memst(xmm6, c->u_mode.ocb.aad_sum);

  clear_vec_regs();

  return 0;
}

ASM_FUNC_ATTR_NOINLINE void
aes_simd128_ecb_enc (void *context, void *outbuf_arg, const void *inbuf_arg,
		     size_t nblocks)
{
  __m128i xmm0, xmm1, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15;
  RIJNDAEL_context *ctx = context;
  unsigned char *outbuf = outbuf_arg;
  const unsigned char *inbuf = inbuf_arg;
  struct vp_aes_config_s config;

  config.nround = ctx->rounds;
  config.sched_keys = ctx->keyschenc[0][0];

  enc_preload(xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15);

#ifdef HAVE_SIMD256
  if (check_simd256_support())
    {
      __m256i ymm0, ymm1;

      for (; nblocks >= 4; nblocks -= 4)
	{
	  movdqu256_memld(inbuf + 0 * BLOCKSIZE, ymm0);
	  movdqu256_memld(inbuf + 2 * BLOCKSIZE, ymm1);

	  aes_encrypt_core_4blks_simd256(&ymm0, &ymm1, config,
					 xmm9, xmm10, xmm11, xmm12, xmm13,
					 xmm14, xmm15);

	  movdqu256_memst(ymm0, outbuf + 0 * BLOCKSIZE);
	  movdqu256_memst(ymm1, outbuf + 2 * BLOCKSIZE);

	  inbuf += 4 * BLOCKSIZE;
	  outbuf += 4 * BLOCKSIZE;
	}
    }
#endif /* HAVE_SIMD256 */

  for (; nblocks >= 2; nblocks -= 2)
    {
      movdqu128_memld(inbuf + 0 * BLOCKSIZE, xmm0);
      movdqu128_memld(inbuf + 1 * BLOCKSIZE, xmm1);

      aes_encrypt_core_2blks(&xmm0, &xmm1, config,
			      xmm9, xmm10, xmm11, xmm12, xmm13, xmm14,
			      xmm15);

      movdqu128_memst(xmm0, outbuf + 0 * BLOCKSIZE);
      movdqu128_memst(xmm1, outbuf + 1 * BLOCKSIZE);

      inbuf += 2 * BLOCKSIZE;
      outbuf += 2 * BLOCKSIZE;
    }

  for (; nblocks; nblocks--)
    {
      movdqu128_memld(inbuf, xmm0);

      xmm0 = aes_encrypt_core(xmm0, config,
			      xmm9, xmm10, xmm11, xmm12, xmm13, xmm14,
			      xmm15);

      movdqu128_memst(xmm0, outbuf);

      inbuf += BLOCKSIZE;
      outbuf += BLOCKSIZE;
    }

  clear_vec_regs();
}

ASM_FUNC_ATTR_NOINLINE void
aes_simd128_ecb_dec (void *context, void *outbuf_arg, const void *inbuf_arg,
		     size_t nblocks)
{
  __m128i xmm0, xmm1, xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15;
  RIJNDAEL_context *ctx = context;
  unsigned char *outbuf = outbuf_arg;
  const unsigned char *inbuf = inbuf_arg;
  struct vp_aes_config_s config;

  if (!ctx->decryption_prepared)
    {
      FUNC_PREPARE_DEC (ctx);
      ctx->decryption_prepared = 1;
    }

  config.nround = ctx->rounds;
  config.sched_keys = ctx->keyschdec[0][0];

  dec_preload(xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15, xmm8);

#ifdef HAVE_SIMD256
  if (check_simd256_support())
    {
      __m256i ymm0, ymm1;

      for (; nblocks >= 4; nblocks -= 4)
	{
	  movdqu256_memld(inbuf + 0 * BLOCKSIZE, ymm0);
	  movdqu256_memld(inbuf + 2 * BLOCKSIZE, ymm1);

	  aes_decrypt_core_4blks_simd256(&ymm0, &ymm1, config,
					 xmm9, xmm10, xmm11, xmm12, xmm13,
					 xmm14, xmm15, xmm8);

	  movdqu256_memst(ymm0, outbuf + 0 * BLOCKSIZE);
	  movdqu256_memst(ymm1, outbuf + 2 * BLOCKSIZE);

	  inbuf += 4 * BLOCKSIZE;
	  outbuf += 4 * BLOCKSIZE;
	}
    }
#endif /* HAVE_SIMD256 */

  for (; nblocks >= 2; nblocks -= 2)
    {
      movdqu128_memld(inbuf + 0 * BLOCKSIZE, xmm0);
      movdqu128_memld(inbuf + 1 * BLOCKSIZE, xmm1);

      aes_decrypt_core_2blks(&xmm0, &xmm1, config,
			      xmm9, xmm10, xmm11, xmm12, xmm13, xmm14,
			      xmm15, xmm8);

      movdqu128_memst(xmm0, outbuf + 0 * BLOCKSIZE);
      movdqu128_memst(xmm1, outbuf + 1 * BLOCKSIZE);

      inbuf += 2 * BLOCKSIZE;
      outbuf += 2 * BLOCKSIZE;
    }

  for (; nblocks; nblocks--)
    {
      movdqu128_memld(inbuf, xmm0);

      xmm0 = aes_decrypt_core(xmm0, config,
			      xmm9, xmm10, xmm11, xmm12, xmm13, xmm14,
			      xmm15, xmm8);

      movdqu128_memst(xmm0, outbuf);

      inbuf += BLOCKSIZE;
      outbuf += BLOCKSIZE;
    }

  clear_vec_regs();
}

ASM_FUNC_ATTR_NOINLINE void
FUNC_ECB_CRYPT (void *context, void *outbuf_arg, const void *inbuf_arg,
		size_t nblocks, int encrypt)
{
  if (encrypt)
    aes_simd128_ecb_enc(context, outbuf_arg, inbuf_arg, nblocks);
  else
    aes_simd128_ecb_dec(context, outbuf_arg, inbuf_arg, nblocks);
}

static ASM_FUNC_ATTR_INLINE __m128i xts_gfmul_byA (__m128i xmm5)
{
  static const __m128i_const xts_gfmul_const = M128I_U64(0x87, 0x01);
  __m128i xmm1;

  pshufd128_0x4E(xmm5, xmm1);
  psraq128(63, xmm1);
  paddq128(xmm5, xmm5);
  pand128_amemld(&xts_gfmul_const, xmm1);
  pxor128(xmm1, xmm5);

  return xmm5;
}

static ASM_FUNC_ATTR_NOINLINE void
aes_simd128_xts_enc (void *context, unsigned char *tweak, void *outbuf_arg,
		     const void *inbuf_arg, size_t nblocks)
{
  __m128i xmm0, xmm1, xmm2, xmm3, xmm7;
  __m128i xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15;
  RIJNDAEL_context *ctx = context;
  unsigned char *outbuf = outbuf_arg;
  const unsigned char *inbuf = inbuf_arg;
  struct vp_aes_config_s config;

  config.nround = ctx->rounds;
  config.sched_keys = ctx->keyschenc[0][0];

  enc_preload(xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15);

  movdqu128_memld(tweak, xmm7); /* Preload tweak */

#ifdef HAVE_SIMD256
  if (check_simd256_support())
    {
      __m256i ymm0, ymm1, ymm2, ymm3;

      for (; nblocks >= 4; nblocks -= 4)
	{
	  movdqu256_memld(inbuf + 0 * BLOCKSIZE, ymm0);
	  movdqu256_memld(inbuf + 2 * BLOCKSIZE, ymm1);

	  movdqa128_256(xmm7, ymm2);
	  xmm7 = xts_gfmul_byA(xmm7);
	  insert256_hi128(xmm7, ymm2);
	  xmm7 = xts_gfmul_byA(xmm7);
	  movdqa128_256(xmm7, ymm3);
	  xmm7 = xts_gfmul_byA(xmm7);
	  insert256_hi128(xmm7, ymm3);
	  xmm7 = xts_gfmul_byA(xmm7);

	  pxor256(ymm2, ymm0);
	  pxor256(ymm3, ymm1);

	  aes_encrypt_core_4blks_simd256(&ymm0, &ymm1, config,
					 xmm9, xmm10, xmm11, xmm12, xmm13,
					 xmm14, xmm15);

	  pxor256(ymm2, ymm0);
	  pxor256(ymm3, ymm1);
	  movdqu256_memst(ymm0, outbuf + 0 * BLOCKSIZE);
	  movdqu256_memst(ymm1, outbuf + 2 * BLOCKSIZE);

	  outbuf += 4 * BLOCKSIZE;
	  inbuf  += 4 * BLOCKSIZE;
	}
    }
#endif /* HAVE_SIMD256 */

  for (; nblocks >= 2; nblocks -= 2)
    {
      movdqu128_memld(inbuf, xmm0);
      movdqu128_memld(inbuf + BLOCKSIZE, xmm1);
      pxor128(xmm7, xmm0);
      movdqa128(xmm7, xmm2);
      xmm3 = xts_gfmul_byA(xmm7);
      pxor128(xmm3, xmm1);
      xmm7 = xts_gfmul_byA(xmm3);

      aes_encrypt_core_2blks(&xmm0, &xmm1, config,
			      xmm9, xmm10, xmm11, xmm12, xmm13, xmm14,
			      xmm15);

      pxor128(xmm2, xmm0);
      pxor128(xmm3, xmm1);
      movdqu128_memst(xmm0, outbuf);
      movdqu128_memst(xmm1, outbuf + BLOCKSIZE);

      outbuf += 2 * BLOCKSIZE;
      inbuf  += 2 * BLOCKSIZE;
    }

  for (; nblocks; nblocks--)
    {
      movdqu128_memld(inbuf, xmm0);
      pxor128(xmm7, xmm0);
      movdqa128(xmm7, xmm2);
      xmm7 = xts_gfmul_byA(xmm7);

      xmm0 = aes_encrypt_core(xmm0, config,
			      xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15);

      pxor128(xmm2, xmm0);
      movdqu128_memst(xmm0, outbuf);

      outbuf += BLOCKSIZE;
      inbuf  += BLOCKSIZE;
    }

  movdqu128_memst(xmm7, tweak);

  clear_vec_regs();
}

static ASM_FUNC_ATTR_NOINLINE void
aes_simd128_xts_dec (void *context, unsigned char *tweak, void *outbuf_arg,
		     const void *inbuf_arg, size_t nblocks)
{
  __m128i xmm0, xmm1, xmm2, xmm3, xmm7, xmm8;
  __m128i xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15;
  RIJNDAEL_context *ctx = context;
  unsigned char *outbuf = outbuf_arg;
  const unsigned char *inbuf = inbuf_arg;
  struct vp_aes_config_s config;

  if (!ctx->decryption_prepared)
    {
      FUNC_PREPARE_DEC (ctx);
      ctx->decryption_prepared = 1;
    }

  config.nround = ctx->rounds;
  config.sched_keys = ctx->keyschdec[0][0];

  dec_preload(xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15, xmm8);

  movdqu128_memld(tweak, xmm7); /* Preload tweak */

#ifdef HAVE_SIMD256
  if (check_simd256_support())
    {
      __m256i ymm0, ymm1, ymm2, ymm3;

      for (; nblocks >= 4; nblocks -= 4)
	{
	  movdqu256_memld(inbuf + 0 * BLOCKSIZE, ymm0);
	  movdqu256_memld(inbuf + 2 * BLOCKSIZE, ymm1);

	  movdqa128_256(xmm7, ymm2);
	  xmm7 = xts_gfmul_byA(xmm7);
	  insert256_hi128(xmm7, ymm2);
	  xmm7 = xts_gfmul_byA(xmm7);
	  movdqa128_256(xmm7, ymm3);
	  xmm7 = xts_gfmul_byA(xmm7);
	  insert256_hi128(xmm7, ymm3);
	  xmm7 = xts_gfmul_byA(xmm7);

	  pxor256(ymm2, ymm0);
	  pxor256(ymm3, ymm1);

	  aes_decrypt_core_4blks_simd256(&ymm0, &ymm1, config,
					 xmm9, xmm10, xmm11, xmm12, xmm13,
					 xmm14, xmm15, xmm8);

	  pxor256(ymm2, ymm0);
	  pxor256(ymm3, ymm1);
	  movdqu256_memst(ymm0, outbuf + 0 * BLOCKSIZE);
	  movdqu256_memst(ymm1, outbuf + 2 * BLOCKSIZE);

	  outbuf += 4 * BLOCKSIZE;
	  inbuf  += 4 * BLOCKSIZE;
	}
    }
#endif /* HAVE_SIMD256 */

  for (; nblocks >= 2; nblocks -= 2)
    {
      movdqu128_memld(inbuf, xmm0);
      movdqu128_memld(inbuf + BLOCKSIZE, xmm1);
      pxor128(xmm7, xmm0);
      movdqa128(xmm7, xmm2);
      xmm3 = xts_gfmul_byA(xmm7);
      pxor128(xmm3, xmm1);
      xmm7 = xts_gfmul_byA(xmm3);

      aes_decrypt_core_2blks(&xmm0, &xmm1, config,
			      xmm9, xmm10, xmm11, xmm12, xmm13, xmm14,
			      xmm15, xmm8);

      pxor128(xmm2, xmm0);
      pxor128(xmm3, xmm1);
      movdqu128_memst(xmm0, outbuf);
      movdqu128_memst(xmm1, outbuf + BLOCKSIZE);

      outbuf += 2 * BLOCKSIZE;
      inbuf  += 2 * BLOCKSIZE;
    }

  for (; nblocks; nblocks--)
    {
      movdqu128_memld(inbuf, xmm0);
      pxor128(xmm7, xmm0);
      movdqa128(xmm7, xmm2);
      xmm7 = xts_gfmul_byA(xmm7);

      xmm0 = aes_decrypt_core(xmm0, config,
			      xmm9, xmm10, xmm11, xmm12, xmm13, xmm14,
			      xmm15, xmm8);

      pxor128(xmm2, xmm0);
      movdqu128_memst(xmm0, outbuf);

      outbuf += BLOCKSIZE;
      inbuf  += BLOCKSIZE;
    }

  movdqu128_memst(xmm7, tweak);

  clear_vec_regs();
}

ASM_FUNC_ATTR_NOINLINE void
FUNC_XTS_CRYPT (void *context, unsigned char *tweak, void *outbuf_arg,
		const void *inbuf_arg, size_t nblocks, int encrypt)
{
  if (encrypt)
    aes_simd128_xts_enc(context, tweak, outbuf_arg, inbuf_arg, nblocks);
  else
    aes_simd128_xts_dec(context, tweak, outbuf_arg, inbuf_arg, nblocks);
}
