/* crc-riscv-zbb-zbc.c - RISC-V Zbb+Zbc accelerated CRC implementation
 * Copyright (C) 2025 Jussi Kivilinna <jussi.kivilinna@iki.fi>
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
 * License along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "g10lib.h"

#include "bufhelp.h"


#if defined (__riscv) && \
    (__riscv_xlen == 64) && \
    defined(HAVE_GCC_INLINE_ASM_RISCV)


#define ALWAYS_INLINE inline __attribute__((always_inline))
#define NO_INLINE __attribute__((noinline))
#define NO_INSTRUMENT_FUNCTION __attribute__((no_instrument_function))

#define ASM_FUNC_ATTR          NO_INSTRUMENT_FUNCTION
#define ASM_FUNC_ATTR_INLINE   ASM_FUNC_ATTR ALWAYS_INLINE
#define ASM_FUNC_ATTR_NOINLINE ASM_FUNC_ATTR NO_INLINE


typedef struct
{
  u64 lo;
  u64 hi;
} u64x2;


/* Constants structure for generic reflected/non-reflected CRC32 CLMUL
 * functions. */
struct crc32_consts_s
{
  /* k: { x^(32*5), x^(32*3), x^(32*2) } mod P(x) */
  u64 k[3];
  /* my_p: { floor(x^64 / P(x)), P(x) } */
  u64 my_p[2];
};


/* CLMUL constants for CRC32 and CRC32RFC1510. */
static const struct crc32_consts_s crc32_consts =
{
  { /* k[3] = reverse_33bits( x^(32*y) mod P(x) ) */
    U64_C(0x1751997d0), U64_C(0x0ccaa009e), /* y = { 5, 3 } */
    U64_C(0x163cd6124)                      /* y = 2 */
  },
  { /* my_p[2] = reverse_33bits ( { floor(x^64 / P(x)), P(x) } ) */
    U64_C(0x1f7011641), U64_C(0x1db710641)
  }
};

/* CLMUL constants for CRC24RFC2440 (polynomial multiplied with x⁸). */
static const struct crc32_consts_s crc24rfc2440_consts =
{
  { /* k[3] = x^(32*y) mod P(x) << 32*/
    U64_C(0xc4b14d00) << 32, U64_C(0xfd7e0c00) << 32, /* y = { 5, 3 } */
    U64_C(0xd9fe8c00) << 32                           /* y = 2 */
  },
  { /* my_p[2] = { floor(x^64 / P(x)), P(x) } */
    U64_C(0x1f845fe24), U64_C(0x1864cfb00)
  }
};


static ASM_FUNC_ATTR_INLINE u64
clmul_low(u64 a, u64 b)
{
  u64 out;
  asm (".option push;\n\t"
       ".option arch, +zbc;\n\t"
       "clmul %0, %1, %2;\n\t"
       ".option pop;\n\t"
       : "=r" (out)
       : "r" (a), "r" (b));
  return out;
}

static ASM_FUNC_ATTR_INLINE u64
clmul_high(u64 a, u64 b)
{
  u64 out;
  asm (".option push;\n\t"
       ".option arch, +zbc;\n\t"
       "clmulh %0, %1, %2;\n\t"
       ".option pop;\n\t"
       : "=r" (out)
       : "r" (a), "r" (b));
  return out;
}

static ASM_FUNC_ATTR_INLINE u64
byteswap_u64(u64 x)
{
  asm (".option push;\n\t"
       ".option arch, +zbb;\n\t"
       "rev8 %0, %1;\n\t"
       ".option pop;\n\t"
       : "=r" (x)
       : "r" (x));
  return x;
}

static ASM_FUNC_ATTR_INLINE u64
and_u64(u64 a, u64 b)
{
  asm ("and %0, %1, %2;\n\t"
       : "=r" (a)
       : "r" (a), "r" (b));
  return a;
}

static ASM_FUNC_ATTR_INLINE u64x2
byteswap_u64x2(u64x2 in)
{
  u64x2 out;
  out.lo = byteswap_u64(in.hi);
  out.hi = byteswap_u64(in.lo);
  return out;
}

static ASM_FUNC_ATTR_INLINE u64
byteswap_u32(u64 x)
{
  return byteswap_u64(x) >> 32;
}

static ASM_FUNC_ATTR_INLINE u64
load_aligned_u32(const void *ptr)
{
  u64 out;
  asm ("lw %0, %1"
       : "=r" (out)
       : "m" (((const bufhelp_u32_t *)ptr)[0]));
  return out;
}

static ASM_FUNC_ATTR_INLINE u64x2
load_aligned_u64x2(const void *ptr)
{
  u64x2 vec;

  asm ("ld %0, %1"
       : "=r" (vec.lo)
       : "m" (((const bufhelp_u64_t *)ptr)[0]));
  asm ("ld %0, %1"
       : "=r" (vec.hi)
       : "m" (((const bufhelp_u64_t *)ptr)[1]));

  return vec;
}

static ASM_FUNC_ATTR_INLINE u64x2
clmul_128(u64 a, u64 b)
{
  u64x2 res;
  res.lo = clmul_low(a, b);
  res.hi = clmul_high(a, b);
  return res;
}

static ASM_FUNC_ATTR_INLINE u64x2
xor_128(u64x2 a, u64x2 b)
{
  u64x2 res;
  res.lo = a.lo ^ b.lo;
  res.hi = a.hi ^ b.hi;
  return res;
}

static ASM_FUNC_ATTR_INLINE u64
crc32r_reduction_4 (u64 data, u64 crc, const struct crc32_consts_s *consts)
{
  u64 step1, step2;

  step1 = clmul_low(data, consts->my_p[0]);
  step1 = and_u64(step1, 0xFFFFFFFFU);
  step2 = clmul_low(step1, consts->my_p[1]);

  return (step2 >> 32) ^ crc;
}

static ASM_FUNC_ATTR_INLINE void
bulk_crc32r (u32 *pcrc, const byte **inbuf, size_t *inlen,
	     const struct crc32_consts_s *consts)
{
  u64 crc = *pcrc;
  u64 k[2] = { consts->k[0], consts->k[1] };
  u64x2 x0, x1, x2;

  x0 = load_aligned_u64x2(*inbuf);
  x0.lo ^= crc;

  *inbuf += 16;
  *inlen -= 16;

  /* Fold by 128 bits */
  while (*inlen >= 16)
    {
      x2 = load_aligned_u64x2(*inbuf);

      x1 = clmul_128(x0.lo, k[0]);
      x0 = clmul_128(x0.hi, k[1]);

      x0 = xor_128(x0, x2);
      x0 = xor_128(x0, x1);

      *inbuf += 16;
      *inlen -= 16;
    }

  /* Reduce 128 bits to 96 bits */
  x1 = clmul_128(x0.lo, k[1]);
  x1.lo ^= x0.hi;

  /* Reduce 96 bits to 64 bits */
  crc = (x1.lo >> 32) ^ (x1.hi << 32);
  crc ^= clmul_low(x1.lo & 0xFFFFFFFFU, consts->k[2]);

  /* Reduce 64 bits to 32 bits */
  crc = crc32r_reduction_4(crc, crc >> 32, consts);

  *pcrc = crc;
}

static ASM_FUNC_ATTR_INLINE u64
tail_crc32r (u64 crc, const byte *inbuf, size_t inlen,
	     const struct crc32_consts_s *consts)
{
  u64 data;

  switch (inlen)
    {
    case 0:
    default:
      break;
    case 1:
      data = inbuf[0];
      data ^= crc;
      data <<= 24;
      crc >>= 8;
      crc = crc32r_reduction_4(data, crc, consts);
      break;
    case 2:
      data = (u32)inbuf[0] ^ ((u32)inbuf[1] << 8);
      data ^= crc;
      data <<= 16;
      crc >>= 16;
      crc = crc32r_reduction_4(data, crc, consts);
      break;
    case 3:
      data = (u32)inbuf[0] ^ ((u32)inbuf[1] << 8) ^ ((u32)inbuf[2] << 16);
      data ^= crc;
      data <<= 8;
      crc >>= 24;
      crc = crc32r_reduction_4(data, crc, consts);
      break;
    }

  return crc;
}

static ASM_FUNC_ATTR_INLINE void
do_crc32r (u32 *pcrc, const byte *inbuf, size_t inlen,
	   const struct crc32_consts_s *consts)
{
  u64 crc = *pcrc;
  u64 data;

  if ((uintptr_t)inbuf & 3)
    {
      /* align input */
      size_t unaligned_len = (-(uintptr_t)inbuf) & 3;

      unaligned_len = unaligned_len < inlen ? unaligned_len : inlen;
      crc = tail_crc32r(crc, inbuf, unaligned_len, consts);

      inbuf += unaligned_len;
      inlen -= unaligned_len;
    }

  while (inlen >= 4)
    {
      data = load_aligned_u32(inbuf);
      data ^= crc;

      inlen -= 4;
      inbuf += 4;

      crc = crc32r_reduction_4(data, 0, consts);
    }

  *pcrc = tail_crc32r(crc, inbuf, inlen, consts);
}

void ASM_FUNC_ATTR
_gcry_crc32_riscv_zbb_zbc (u32 *pcrc, const byte *inbuf, size_t inlen)
{
  const struct crc32_consts_s *consts = &crc32_consts;

  if (!inlen)
    return;

  if (inlen >= 16)
    {
      size_t unaligned_len = (-(uintptr_t)inbuf) & 7;
      if (inlen >= 16 + unaligned_len)
	{
	  if (unaligned_len > 0)
	    {
	      /* align input */
	      do_crc32r (pcrc, inbuf, unaligned_len, consts);
	      inbuf += unaligned_len;
	      inlen -= unaligned_len;
	    }

	  bulk_crc32r (pcrc, &inbuf, &inlen, consts);
	  if (!inlen)
	    return;
	}
    }

  do_crc32r (pcrc, inbuf, inlen, consts);
}

static ASM_FUNC_ATTR_INLINE u64
crc32_reduction_4 (u64 data, u64 crc,
                   const struct crc32_consts_s *consts)
{
  u64 step1, step2;

  step1 = clmul_high((u64)data << 32, consts->my_p[0]);
  step2 = clmul_low(step1, consts->my_p[1]);

  return (byteswap_u64(step2) >> 32) ^ crc;
}

static ASM_FUNC_ATTR_INLINE void
bulk_crc32 (u32 *pcrc, const byte **inbuf, size_t *inlen,
	    const struct crc32_consts_s *consts)
{
  u64 crc = *pcrc;
  u64 k[2] = { consts->k[0], consts->k[1] };
  u64x2 x0, x1, x2;
  u64 temp;

  x0 = load_aligned_u64x2(*inbuf);
  x0.lo ^= crc;
  x0 = byteswap_u64x2(x0);

  *inbuf += 16;
  *inlen -= 16;

  while (*inlen >= 16)
    {
      x2 = load_aligned_u64x2(*inbuf);

      x1 = clmul_128(x0.hi, k[0]);
      x2 = byteswap_u64x2(x2);
      x0 = clmul_128(x0.lo, k[1]);

      x1 = xor_128(x1, x2);
      x0 = xor_128(x0, x1);

      *inbuf += 16;
      *inlen -= 16;
    }

  /* Reduce 128 bits to 96 bits */
  x2 = clmul_128(x0.hi, k[1]);
  x2.hi ^= x0.lo;

  /* Reduce 96 bits to 64 bits */
  crc = (x2.hi << 32) ^ (x2.lo >> 32);
  crc ^= clmul_high(and_u64(x2.hi, ~(u64)0xFFFFFFFFU), consts->k[2]);

  /* Reduce 64 bits to 32 bits */
  temp = clmul_high(and_u64(crc, ~(u64)0xFFFFFFFFU), consts->my_p[0]);
  temp = clmul_low(temp, consts->my_p[1]);
  crc = temp ^ (crc & 0xFFFFFFFFU);

  crc = byteswap_u32(crc);

  *pcrc = crc;
}

static ASM_FUNC_ATTR_INLINE u64
tail_crc32 (u64 crc, const byte *inbuf, size_t inlen,
	     const struct crc32_consts_s *consts)
{
  u64 data;

  switch (inlen)
    {
    case 0:
    default:
      break;
    case 1:
      data = inbuf[0];
      data ^= crc;
      data = data & 0xffU;
      crc = crc >> 8;
      crc = crc32_reduction_4(data, crc, consts);
      break;
    case 2:
      data = (u32)inbuf[0] ^ ((u32)inbuf[1] << 8);
      data ^= crc;
      data = byteswap_u32(data << 16);
      crc = crc >> 16;
      crc = crc32_reduction_4(data, crc, consts);
      break;
    case 3:
      data = (u32)inbuf[0] ^ ((u32)inbuf[1] << 8) ^ ((u32)inbuf[2] << 16);
      data ^= crc;
      data = byteswap_u32(data << 8);
      crc = crc >> 24;
      crc = crc32_reduction_4(data, crc, consts);
      break;
    }

  return crc;
}

static ASM_FUNC_ATTR_INLINE void
do_crc32 (u32 *pcrc, const byte *inbuf, size_t inlen,
          const struct crc32_consts_s *consts)
{
  u64 crc = *pcrc;
  u64 data;

  if ((uintptr_t)inbuf & 3)
    {
      /* align input */
      size_t unaligned_len = (-(uintptr_t)inbuf) & 3;

      unaligned_len = unaligned_len < inlen ? unaligned_len : inlen;
      crc = tail_crc32(crc, inbuf, unaligned_len, consts);

      inbuf += unaligned_len;
      inlen -= unaligned_len;
    }

  while (inlen >= 4)
    {
      data = load_aligned_u32(inbuf);
      data ^= crc;
      data = byteswap_u32(data);

      inlen -= 4;
      inbuf += 4;

      crc = crc32_reduction_4(data, 0, consts);
    }

  *pcrc = tail_crc32(crc, inbuf, inlen, consts);
}

void ASM_FUNC_ATTR
_gcry_crc24rfc2440_riscv_zbb_zbc (u32 *pcrc, const byte *inbuf, size_t inlen)
{
  const struct crc32_consts_s *consts = &crc24rfc2440_consts;

  if (!inlen)
    return;

  if (inlen >= 16)
    {
      size_t unaligned_len = (-(uintptr_t)inbuf) & 7;
      if (inlen >= 16 + unaligned_len)
	{
	  if (unaligned_len > 0)
	    {
	      /* align input */
	      do_crc32 (pcrc, inbuf, unaligned_len, consts);
	      inbuf += unaligned_len;
	      inlen -= unaligned_len;
	    }

	  bulk_crc32 (pcrc, &inbuf, &inlen, consts);
	  if (!inlen)
	    return;
	}
    }

  do_crc32 (pcrc, inbuf, inlen, consts);
}

#endif
