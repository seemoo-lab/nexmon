/* cipher-gcm-aarch64-simd.c - ARM/NEON accelerated GHASH
 * Copyright (C) 2019-2024 Jussi Kivilinna <jussi.kivilinna@iki.fi>
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
#include "./cipher-internal.h"

#ifdef GCM_USE_AARCH64

#include "simd-common-aarch64.h"
#include <arm_neon.h>

#define ALWAYS_INLINE inline __attribute__((always_inline))
#define NO_INLINE __attribute__((noinline))
#define NO_INSTRUMENT_FUNCTION __attribute__((no_instrument_function))

#define ASM_FUNC_ATTR          NO_INSTRUMENT_FUNCTION
#define ASM_FUNC_ATTR_INLINE   ASM_FUNC_ATTR ALWAYS_INLINE
#define ASM_FUNC_ATTR_NOINLINE ASM_FUNC_ATTR NO_INLINE

static ASM_FUNC_ATTR_INLINE uint64x2_t
byteswap_u64x2(uint64x2_t vec)
{
  vec = (uint64x2_t)vrev64q_u8((uint8x16_t)vec);
  vec = (uint64x2_t)vextq_u8((uint8x16_t)vec, (uint8x16_t)vec, 8);
  return vec;
}

static ASM_FUNC_ATTR_INLINE uint64x2_t
veor_u64x2(uint64x2_t va, uint64x2_t vb)
{
  return (uint64x2_t)veorq_u8((uint8x16_t)va, (uint8x16_t)vb);
}

static ASM_FUNC_ATTR_INLINE uint64x1_t
veor_u64x1(uint64x1_t va, uint64x1_t vb)
{
  return (uint64x1_t)veor_u8((uint8x8_t)va, (uint8x8_t)vb);
}

static ASM_FUNC_ATTR_INLINE uint64x1_t
vand_u64x1(uint64x1_t va, uint64x1_t vb)
{
  return (uint64x1_t)vand_u8((uint8x8_t)va, (uint8x8_t)vb);
}

static ASM_FUNC_ATTR_INLINE uint64x1_t
vorr_u64x1(uint64x1_t va, uint64x1_t vb)
{
  return (uint64x1_t)vorr_u8((uint8x8_t)va, (uint8x8_t)vb);
}

/* 64x64=>128 carry-less multiplication using vmull.p8 instruction.
 *
 * From "Câmara, D.; Gouvêa, C. P. L.; López, J. & Dahab, R. Fast Software
 * Polynomial Multiplication on ARM Processors using the NEON Engine. The
 * Second International Workshop on Modern Cryptography and Security
 * Engineering — MoCrySEn, 2013". */
static ASM_FUNC_ATTR_INLINE uint64x2_t
emulate_vmull_p64(uint64x1_t ad, uint64x1_t bd)
{
  static const uint64x1_t k0 = { 0 };
  static const uint64x1_t k16 = { U64_C(0xffff) };
  static const uint64x1_t k32 = { U64_C(0xffffffff) };
  static const uint64x1_t k48 = { U64_C(0xffffffffffff) };
  uint64x1_t rl;
  uint64x2_t rq;
  uint64x1_t t0l;
  uint64x1_t t0h;
  uint64x2_t t0q;
  uint64x1_t t1l;
  uint64x1_t t1h;
  uint64x2_t t1q;
  uint64x1_t t2l;
  uint64x1_t t2h;
  uint64x2_t t2q;
  uint64x1_t t3l;
  uint64x1_t t3h;
  uint64x2_t t3q;

  t0l = (uint64x1_t)vext_u8((uint8x8_t)ad, (uint8x8_t)ad, 1);
  t0q = (uint64x2_t)vmull_p8((poly8x8_t)t0l, (poly8x8_t)bd);

  rl = (uint64x1_t)vext_u8((uint8x8_t)bd, (uint8x8_t)bd, 1);
  rq = (uint64x2_t)vmull_p8((poly8x8_t)ad, (poly8x8_t)rl);

  t1l = (uint64x1_t)vext_u8((uint8x8_t)ad, (uint8x8_t)ad, 2);
  t1q = (uint64x2_t)vmull_p8((poly8x8_t)t1l, (poly8x8_t)bd);

  t3l = (uint64x1_t)vext_u8((uint8x8_t)bd, (uint8x8_t)bd, 2);
  t3q = (uint64x2_t)vmull_p8((poly8x8_t)ad, (poly8x8_t)t3l);

  t2l = (uint64x1_t)vext_u8((uint8x8_t)ad, (uint8x8_t)ad, 3);
  t2q = (uint64x2_t)vmull_p8((poly8x8_t)t2l, (poly8x8_t)bd);

  t0q = veor_u64x2(t0q, rq);
  t0l = vget_low_u64(t0q);
  t0h = vget_high_u64(t0q);

  rl = (uint64x1_t)vext_u8((uint8x8_t)bd, (uint8x8_t)bd, 3);
  rq = (uint64x2_t)vmull_p8((poly8x8_t)ad, (poly8x8_t)rl);

  t1q = veor_u64x2(t1q, t3q);
  t1l = vget_low_u64(t1q);
  t1h = vget_high_u64(t1q);

  t3l = (uint64x1_t)vext_u8((uint8x8_t)bd, (uint8x8_t)bd, 4);
  t3q = (uint64x2_t)vmull_p8((poly8x8_t)ad, (poly8x8_t)t3l);
  t3l = vget_low_u64(t3q);
  t3h = vget_high_u64(t3q);

  t0l = veor_u64x1(t0l, t0h);
  t0h = vand_u64x1(t0h, k48);
  t1l = veor_u64x1(t1l, t1h);
  t1h = vand_u64x1(t1h, k32);
  t2q = veor_u64x2(t2q, rq);
  t2l = vget_low_u64(t2q);
  t2h = vget_high_u64(t2q);
  t0l = veor_u64x1(t0l, t0h);
  t1l = veor_u64x1(t1l, t1h);
  t2l = veor_u64x1(t2l, t2h);
  t2h = vand_u64x1(t2h, k16);
  t3l = veor_u64x1(t3l, t3h);
  t3h = k0;
  t0q = vcombine_u64(t0l, t0h);
  t0q = (uint64x2_t)vextq_u8((uint8x16_t)t0q, (uint8x16_t)t0q, 15);
  t2l = veor_u64x1(t2l, t2h);
  t1q = vcombine_u64(t1l, t1h);
  t1q = (uint64x2_t)vextq_u8((uint8x16_t)t1q, (uint8x16_t)t1q, 14);
  rq = (uint64x2_t)vmull_p8((poly8x8_t)ad, (poly8x8_t)bd);
  t2q = vcombine_u64(t2l, t2h);
  t2q = (uint64x2_t)vextq_u8((uint8x16_t)t2q, (uint8x16_t)t2q, 13);
  t3q = vcombine_u64(t3l, t3h);
  t3q = (uint64x2_t)vextq_u8((uint8x16_t)t3q, (uint8x16_t)t3q, 12);
  t0q = veor_u64x2(t0q, t1q);
  t2q = veor_u64x2(t2q, t3q);
  rq = veor_u64x2(rq, t0q);
  rq = veor_u64x2(rq, t2q);
  return rq;
}

/* GHASH functions.
 *
 * See "Gouvêa, C. P. L. & López, J. Implementing GCM on ARMv8. Topics in
 * Cryptology — CT-RSA 2015" for details.
 */
static ASM_FUNC_ATTR_INLINE uint64x2x2_t
pmul_128x128(uint64x2_t a, uint64x2_t b)
{
  uint64x1_t a_l = vget_low_u64(a);
  uint64x1_t a_h = vget_high_u64(a);
  uint64x1_t b_l = vget_low_u64(b);
  uint64x1_t b_h = vget_high_u64(b);
  uint64x1_t t1_h = veor_u64x1(b_l, b_h);
  uint64x1_t t1_l = veor_u64x1(a_l, a_h);
  uint64x2_t r0 = emulate_vmull_p64(a_l, b_l);
  uint64x2_t r1 = emulate_vmull_p64(a_h, b_h);
  uint64x2_t t2 = emulate_vmull_p64(t1_h, t1_l);
  uint64x1_t t2_l, t2_h;
  uint64x1_t r0_l, r0_h;
  uint64x1_t r1_l, r1_h;

  t2 = veor_u64x2(t2, r0);
  t2 = veor_u64x2(t2, r1);

  r0_l = vget_low_u64(r0);
  r0_h = vget_high_u64(r0);
  r1_l = vget_low_u64(r1);
  r1_h = vget_high_u64(r1);
  t2_l = vget_low_u64(t2);
  t2_h = vget_high_u64(t2);

  r0_h = veor_u64x1(r0_h, t2_l);
  r1_l = veor_u64x1(r1_l, t2_h);

  r0 = vcombine_u64(r0_l, r0_h);
  r1 = vcombine_u64(r1_l, r1_h);

  return (const uint64x2x2_t){ .val = { r0, r1 } };
}

/* Reduction using Xor and Shift.
 *
 * See "Shay Gueron, Michael E. Kounavis. Intel Carry-Less Multiplication
 * Instruction and its Usage for Computing the GCM Mode" for details.
 */
static ASM_FUNC_ATTR_INLINE uint64x2_t
reduction(uint64x2x2_t r0r1)
{
  static const uint64x2_t k0 = { U64_C(0), U64_C(0) };
  uint64x2_t r0 = r0r1.val[0];
  uint64x2_t r1 = r0r1.val[1];
  uint64x2_t t0q;
  uint64x2_t t1q;
  uint64x2_t t2q;
  uint64x2_t t;

  t0q = (uint64x2_t)vshlq_n_u32((uint32x4_t)r0, 31);
  t1q = (uint64x2_t)vshlq_n_u32((uint32x4_t)r0, 30);
  t2q = (uint64x2_t)vshlq_n_u32((uint32x4_t)r0, 25);
  t0q = veor_u64x2(t0q, t1q);
  t0q = veor_u64x2(t0q, t2q);
  t = (uint64x2_t)vextq_u8((uint8x16_t)t0q, (uint8x16_t)k0, 4);
  t0q = (uint64x2_t)vextq_u8((uint8x16_t)k0, (uint8x16_t)t0q, 16 - 12);
  r0 = veor_u64x2(r0, t0q);
  t0q = (uint64x2_t)vshrq_n_u32((uint32x4_t)r0, 1);
  t1q = (uint64x2_t)vshrq_n_u32((uint32x4_t)r0, 2);
  t2q = (uint64x2_t)vshrq_n_u32((uint32x4_t)r0, 7);
  t0q = veor_u64x2(t0q, t1q);
  t0q = veor_u64x2(t0q, t2q);
  t0q = veor_u64x2(t0q, t);
  r0 = veor_u64x2(r0, t0q);
  return veor_u64x2(r0, r1);
}

ASM_FUNC_ATTR_NOINLINE unsigned int
_gcry_ghash_aarch64_simd(gcry_cipher_hd_t c, byte *result, const byte *buf,
			 size_t nblocks)
{
  uint64x2_t rhash;
  uint64x2_t rh1;
  uint64x2_t rbuf;
  uint64x2x2_t rr0rr1;

  if (nblocks == 0)
    return 0;

  rhash = vld1q_u64((const void *)result);
  rh1 = vld1q_u64((const void *)c->u_mode.gcm.u_ghash_key.key);

  rhash = byteswap_u64x2(rhash);

  rbuf = vld1q_u64((const void *)buf);
  buf += 16;
  nblocks--;

  rbuf = byteswap_u64x2(rbuf);

  rhash = veor_u64x2(rhash, rbuf);

  while (nblocks)
    {
      rbuf = vld1q_u64((const void *)buf);
      buf += 16;
      nblocks--;

      rr0rr1 = pmul_128x128(rhash, rh1);

      rbuf = byteswap_u64x2(rbuf);

      rhash = reduction(rr0rr1);

      rhash = veor_u64x2(rhash, rbuf);
    }

  rr0rr1 = pmul_128x128(rhash, rh1);
  rhash = reduction(rr0rr1);

  rhash = byteswap_u64x2(rhash);

  vst1q_u64((void *)result, rhash);

  clear_vec_regs();

  return 0;
}

static ASM_FUNC_ATTR_INLINE void
gcm_lsh_1(void *r_out, uint64x2_t i)
{
  static const uint64x1_t const_d = { U64_C(0xc200000000000000) };
  uint64x1_t ia = vget_low_u64(i);
  uint64x1_t ib = vget_high_u64(i);
  uint64x1_t oa, ob, ma;
  uint64x1x2_t oa_ob;

  ma = (uint64x1_t)vshr_n_s64((int64x1_t)ib, 63);
  oa = vshr_n_u64(ib, 63);
  ob = vshr_n_u64(ia, 63);
  ma = vand_u64x1(ma, const_d);
  ib = vshl_n_u64(ib, 1);
  ia = vshl_n_u64(ia, 1);
  ob = vorr_u64x1(ob, ib);
  oa = vorr_u64x1(oa, ia);
  ob = veor_u64x1(ob, ma);
  oa_ob = (const uint64x1x2_t){ .val = { oa, ob } };
  vst2_u64(r_out, oa_ob);
}

ASM_FUNC_ATTR_NOINLINE void
_gcry_ghash_setup_aarch64_simd(gcry_cipher_hd_t c)
{
  uint64x2_t rhash = vld1q_u64((const void *)c->u_mode.gcm.u_ghash_key.key);

  rhash = byteswap_u64x2(rhash);

  gcm_lsh_1(c->u_mode.gcm.u_ghash_key.key, rhash);

  clear_vec_regs();
}

#endif /* GCM_USE_AARCH64 */
