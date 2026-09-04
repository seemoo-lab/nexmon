/* sha256-riscv-zvknha-zvkb.c - RISC-V vector crypto implementation of SHA-256
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

#if defined (__riscv) && \
    defined(HAVE_COMPATIBLE_CC_RISCV_VECTOR_INTRINSICS) && \
    defined(HAVE_COMPATIBLE_CC_RISCV_VECTOR_CRYPTO_INTRINSICS)

#include "g10lib.h"
#include "simd-common-riscv.h"
#include <riscv_vector.h>


#define ALWAYS_INLINE inline __attribute__((always_inline))
#define NO_INLINE __attribute__((noinline))
#define NO_INSTRUMENT_FUNCTION __attribute__((no_instrument_function))

#define ASM_FUNC_ATTR          NO_INSTRUMENT_FUNCTION
#define ASM_FUNC_ATTR_INLINE   ASM_FUNC_ATTR ALWAYS_INLINE
#define ASM_FUNC_ATTR_NOINLINE ASM_FUNC_ATTR NO_INLINE


static ASM_FUNC_ATTR_INLINE vuint32m1_t
working_vsha2cl_vv_u32m1(vuint32m1_t hgcd, vuint32m1_t feba,
			 vuint32m1_t kw, size_t vl)
{
#ifdef HAVE_BROKEN_VSHA2CL_INTRINSIC
  asm (
    "vsetvli zero,%3,e32,m1,ta,ma;\n\t"
    "vsha2cl.vv %0,%1,%2;\n\t"
    : "+vr" (hgcd)
    : "vr" (feba), "vr" (kw), "r" (vl)
    : "vl", "vtype"
  );
  return hgcd;
#else
  return __riscv_vsha2cl_vv_u32m1(hgcd, feba, kw, vl);
#endif
}


/* Quad-round with message expansion (rounds 0-47) */
#define QUAD_ROUND_W_SCHED(w0, w1, w2, w3) \
    v_k = __riscv_vle32_v_u32m1(k, vl); \
    k += 4; \
    v_kw = __riscv_vadd_vv_u32m1(v_k, w0, vl); \
    v_hgcd_work = working_vsha2cl_vv_u32m1(v_hgcd_work, v_feba_work, v_kw, vl); \
    v_feba_work = __riscv_vsha2ch_vv_u32m1(v_feba_work, v_hgcd_work, v_kw, vl); \
    v_w_merged = __riscv_vmerge_vvm_u32m1(w2, w1, merge_mask, vl); \
    w0 = __riscv_vsha2ms_vv_u32m1(w0, v_w_merged, w3, vl);

/* Quad-round without message expansion (rounds 48-63) */
#define QUAD_ROUND_NO_SCHED(w0) \
    v_k = __riscv_vle32_v_u32m1(k, vl); \
    k += 4; \
    v_kw = __riscv_vadd_vv_u32m1(v_k, w0, vl); \
    v_hgcd_work = working_vsha2cl_vv_u32m1(v_hgcd_work, v_feba_work, v_kw, vl); \
    v_feba_work = __riscv_vsha2ch_vv_u32m1(v_feba_work, v_hgcd_work, v_kw, vl);


static ASM_FUNC_ATTR_INLINE vuint32m1_t
load_and_swap (const byte * p, size_t vl)
{
#ifdef RVV_UNALIGNED_NOT_ALLOWED
  vuint8m1_t temp = __riscv_vle8_v_u8m1(p, vl * 4);
  return __riscv_vrev8_v_u32m1(__riscv_vreinterpret_v_u8m1_u32m1(temp), vl);
#else
  return __riscv_vrev8_v_u32m1(__riscv_vle32_v_u32m1((const void *)p, vl), vl);
#endif
}


static ASM_FUNC_ATTR_INLINE void
sha256_transform_zvknha_zvkb (u32 state[8], const uint8_t * data,
			      size_t nblocks)
{
  static const u32 k_const[64] =
  {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
  };
  static const u32 feba_hgcd_indices[4] = { 20, 16, 4, 0 };
  static const int feba_offset = 0;
  static const int hgcd_offset = 8 / sizeof(u32);
  size_t vl;
  vuint32m1_t idx;
  vuint32m1_t v_feba_work, v_feba;
  vuint32m1_t v_hgcd_work, v_hgcd;
  vuint32m1_t w0, w1, w2, w3;
  vuint32m1_t v_k, v_kw, v_w_merged;
  vbool32_t merge_mask;
  vuint32m1_t v_feba_hgcd_idx;

  vl = 4;
  idx = __riscv_vid_v_u32m1(vl);
  merge_mask = __riscv_vmseq_vx_u32m1_b32(idx, 0, vl);

  v_feba_hgcd_idx = __riscv_vle32_v_u32m1(feba_hgcd_indices, vl);

  v_feba = __riscv_vluxei32_v_u32m1(state + feba_offset, v_feba_hgcd_idx, vl);
  v_hgcd = __riscv_vluxei32_v_u32m1(state + hgcd_offset, v_feba_hgcd_idx, vl);

  while (nblocks > 0)
    {
      const u32 *k = k_const;

      v_feba_work = v_feba;
      v_hgcd_work = v_hgcd;

      w0 = load_and_swap(data + 0, vl);
      w1 = load_and_swap(data + 16, vl);
      w2 = load_and_swap(data + 32, vl);
      w3 = load_and_swap(data + 48, vl);

      QUAD_ROUND_W_SCHED(w0, w1, w2, w3);
      QUAD_ROUND_W_SCHED(w1, w2, w3, w0);
      QUAD_ROUND_W_SCHED(w2, w3, w0, w1);
      QUAD_ROUND_W_SCHED(w3, w0, w1, w2);
      QUAD_ROUND_W_SCHED(w0, w1, w2, w3);
      QUAD_ROUND_W_SCHED(w1, w2, w3, w0);
      QUAD_ROUND_W_SCHED(w2, w3, w0, w1);
      QUAD_ROUND_W_SCHED(w3, w0, w1, w2);
      QUAD_ROUND_W_SCHED(w0, w1, w2, w3);
      QUAD_ROUND_W_SCHED(w1, w2, w3, w0);
      QUAD_ROUND_W_SCHED(w2, w3, w0, w1);
      QUAD_ROUND_W_SCHED(w3, w0, w1, w2);
      QUAD_ROUND_NO_SCHED(w0);
      QUAD_ROUND_NO_SCHED(w1);
      QUAD_ROUND_NO_SCHED(w2);
      QUAD_ROUND_NO_SCHED(w3);

      v_feba = __riscv_vadd_vv_u32m1(v_feba, v_feba_work, vl);
      v_hgcd = __riscv_vadd_vv_u32m1(v_hgcd, v_hgcd_work, vl);

      data += 64;
      nblocks--;
    }

  __riscv_vsuxei32_v_u32m1(state + feba_offset, v_feba_hgcd_idx, v_feba, vl);
  __riscv_vsuxei32_v_u32m1(state + hgcd_offset, v_feba_hgcd_idx, v_hgcd, vl);

  clear_vec_regs();
}


#ifdef HAVE_GCC_ATTRIBUTE_OPTIMIZE
# define FUNC_ATTR_OPT_O2 __attribute__((optimize("-O2")))
#else
# define FUNC_ATTR_OPT_O2
#endif

unsigned int ASM_FUNC_ATTR FUNC_ATTR_OPT_O2
_gcry_sha256_transform_riscv_zvknha_zvkb(u32 state[8],
					 const unsigned char *input_data,
					 size_t num_blks)
{
  sha256_transform_zvknha_zvkb(state, input_data, num_blks);
  return 0;
}

unsigned int ASM_FUNC_ATTR FUNC_ATTR_OPT_O2
_gcry_sha256_riscv_v_check_hw(void)
{
  return (__riscv_vsetvl_e32m1(4) == 4);
}

#endif /* HAVE_COMPATIBLE_CC_RISCV_VECTOR_INTRINSICS */
