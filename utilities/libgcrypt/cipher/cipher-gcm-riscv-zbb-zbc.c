/* cipher-gcm-riscv-zbb-zbc.c - RISC-V Zbb+Zbc accelerated GHASH
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
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include "types.h"
#include "g10lib.h"
#include "cipher.h"
#include "bufhelp.h"
#include "cipher-internal.h"

#ifdef GCM_USE_RISCV_ZBB_ZBC

#define ALWAYS_INLINE inline __attribute__((always_inline))
#define NO_INLINE __attribute__((noinline))
#define NO_INSTRUMENT_FUNCTION __attribute__((no_instrument_function))

#define ASM_FUNC_ATTR          NO_INSTRUMENT_FUNCTION
#define ASM_FUNC_ATTR_INLINE   ASM_FUNC_ATTR ALWAYS_INLINE
#define ASM_FUNC_ATTR_NOINLINE ASM_FUNC_ATTR NO_INLINE

typedef struct { u64 val[2]; } u64x2;
typedef struct { u64x2 val[2]; } u64x2x2;

static ASM_FUNC_ATTR_INLINE u64x2
load_aligned_u64x2(const void *ptr)
{
  u64x2 vec;

  asm ("ld %0, %1"
       : "=r" (vec.val[0])
       : "m" (((const bufhelp_u64_t *)ptr)[0]));
  asm ("ld %0, %1"
       : "=r" (vec.val[1])
       : "m" (((const bufhelp_u64_t *)ptr)[1]));

  return vec;
}

static ASM_FUNC_ATTR_INLINE u64x2
load_unaligned_u64x2(const void *ptr)
{
#if !(defined(__riscv_zicclsm) && (__riscv_zicclsm >= 1000000))
  if (UNLIKELY(((uintptr_t)ptr & 7) != 0))
    {
      /* unaligned load */
      const bufhelp_u64_t *ptr_u64 = ptr;
      u64x2 vec;
      vec.val[0] = ptr_u64[0].a;
      vec.val[1] = ptr_u64[1].a;
      return vec;
    }
#endif

  /* aligned load */
  return load_aligned_u64x2(ptr);
}

static ASM_FUNC_ATTR_INLINE void
store_aligned_u64x2(void *ptr, u64x2 vec)
{
  asm ("sd %1, %0"
       : "=m" (((bufhelp_u64_t *)ptr)[0])
       : "r" (vec.val[0]));
  asm ("sd %1, %0"
       : "=m" (((bufhelp_u64_t *)ptr)[1])
       : "r" (vec.val[1]));
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

static ASM_FUNC_ATTR_INLINE u64x2
byteswap_u64x2(u64x2 vec)
{
  u64 tmp = byteswap_u64(vec.val[0]);
  vec.val[0] = byteswap_u64(vec.val[1]);
  vec.val[1] = tmp;
  return vec;
}

static ASM_FUNC_ATTR_INLINE u64x2
veor_u64x2(u64x2 va, u64x2 vb)
{
  va.val[0] ^= vb.val[0];
  va.val[1] ^= vb.val[1];
  return va;
}

/* 64x64 => 128 carry-less multiplication */
static ASM_FUNC_ATTR_INLINE u64x2
clmul_u64x2(u64 a, u64 b)
{
  u64x2 vec;
  asm (".option push;\n\t"
       ".option arch, +zbc;\n\t"
       "clmul %0, %1, %2;\n\t"
       ".option pop;\n\t"
       : "=r" (vec.val[0])
       : "r" (a), "r" (b));
  asm (".option push;\n\t"
       ".option arch, +zbc;\n\t"
       "clmulh %0, %1, %2;\n\t"
       ".option pop;\n\t"
       : "=r" (vec.val[1])
       : "r" (a), "r" (b));
  return vec;
}

/* GHASH functions.
 *
 * See "Gouvêa, C. P. L. & López, J. Implementing GCM on ARMv8. Topics in
 * Cryptology — CT-RSA 2015" for details.
 */
static ASM_FUNC_ATTR_INLINE u64x2x2
pmul_128x128(u64x2 a, u64x2 b)
{
  u64 a_l = a.val[0];
  u64 a_h = a.val[1];
  u64 b_l = b.val[0];
  u64 b_h = b.val[1];
  u64 t1_h = b_l ^ b_h;
  u64 t1_l = a_l ^ a_h;
  u64x2 r0 = clmul_u64x2(a_l, b_l);
  u64x2 r1 = clmul_u64x2(a_h, b_h);
  u64x2 t2 = clmul_u64x2(t1_h, t1_l);
  u64 t2_l, t2_h;
  u64 r0_l, r0_h;
  u64 r1_l, r1_h;

  t2 = veor_u64x2(t2, r0);
  t2 = veor_u64x2(t2, r1);

  r0_l = r0.val[0];
  r0_h = r0.val[1];
  r1_l = r1.val[0];
  r1_h = r1.val[1];
  t2_l = t2.val[0];
  t2_h = t2.val[1];

  r0_h = r0_h ^ t2_l;
  r1_l = r1_l ^ t2_h;

  r0 = (const u64x2){ .val = { r0_l, r0_h } };
  r1 = (const u64x2){ .val = { r1_l, r1_h } };

  return (const u64x2x2){ .val = { r0, r1 } };
}

static ASM_FUNC_ATTR_INLINE u64x2
reduction(u64x2x2 r0r1)
{
  static const u64 rconst = { U64_C(0xc200000000000000) };
  u64x2 r0 = r0r1.val[0];
  u64x2 r1 = r0r1.val[1];
  u64x2 t = clmul_u64x2(r0.val[0], rconst);
  r0.val[1] ^= t.val[0];
  r1.val[0] ^= t.val[1];
  t = clmul_u64x2(r0.val[1], rconst);
  r1 = veor_u64x2(r1, t);
  return veor_u64x2(r0, r1);
}

static ASM_FUNC_ATTR_INLINE unsigned int
ghash_polyval_riscv_zbb_zbc(gcry_cipher_hd_t c, byte *result, const byte *buf,
			    size_t nblocks, int is_polyval)
{
  u64x2 rhash;
  u64x2 rh1;
  u64x2 rbuf;
  u64x2x2 rr0rr1;

  if (nblocks == 0)
    return 0;

  rhash = load_aligned_u64x2(result);
  rh1 = load_aligned_u64x2(c->u_mode.gcm.u_ghash_key.key);

  rhash = byteswap_u64x2(rhash);

  rbuf = load_unaligned_u64x2(buf);
  buf += 16;
  nblocks--;

  rbuf = is_polyval ? rbuf : byteswap_u64x2(rbuf);

  rhash = veor_u64x2(rhash, rbuf);

  while (nblocks)
    {
      rbuf = load_unaligned_u64x2(buf);
      buf += 16;
      nblocks--;

      rr0rr1 = pmul_128x128(rhash, rh1);

      rbuf = is_polyval ? rbuf : byteswap_u64x2(rbuf);

      rhash = reduction(rr0rr1);

      rhash = veor_u64x2(rhash, rbuf);
    }

  rr0rr1 = pmul_128x128(rhash, rh1);
  rhash = reduction(rr0rr1);

  rhash = byteswap_u64x2(rhash);

  store_aligned_u64x2(result, rhash);

  return 0;
}

ASM_FUNC_ATTR_NOINLINE unsigned int
_gcry_ghash_riscv_zbb_zbc(gcry_cipher_hd_t c, byte *result, const byte *buf,
			  size_t nblocks)
{
  return ghash_polyval_riscv_zbb_zbc(c, result, buf, nblocks, 0);
}

ASM_FUNC_ATTR_NOINLINE unsigned int
_gcry_polyval_riscv_zbb_zbc(gcry_cipher_hd_t c, byte *result, const byte *buf,
			    size_t nblocks)
{
  return ghash_polyval_riscv_zbb_zbc(c, result, buf, nblocks, 1);
}

static ASM_FUNC_ATTR_INLINE void
gcm_lsh_1(void *r_out, u64x2 i)
{
  static const u64 rconst = { U64_C(0xc200000000000000) };
  u64 ia = i.val[0];
  u64 ib = i.val[1];
  u64 oa, ob, ma;
  u64x2 oa_ob;

  ma = (u64)-(ib >> 63);
  oa = ib >> 63;
  ob = ia >> 63;
  ma = ma & rconst;
  ib = ib << 1;
  ia = ia << 1;
  ob = ob | ib;
  oa = oa | ia;
  ob = ob ^ ma;
  oa_ob = (const u64x2){ .val = { oa, ob } };
  store_aligned_u64x2(r_out, oa_ob);
}

ASM_FUNC_ATTR_NOINLINE void
_gcry_ghash_setup_riscv_zbb_zbc(gcry_cipher_hd_t c)
{
  u64x2 rhash = load_aligned_u64x2(c->u_mode.gcm.u_ghash_key.key);

  rhash = byteswap_u64x2(rhash);

  gcm_lsh_1(c->u_mode.gcm.u_ghash_key.key, rhash);
}

#endif /* GCM_USE_RISCV_ZBB_ZBC */
