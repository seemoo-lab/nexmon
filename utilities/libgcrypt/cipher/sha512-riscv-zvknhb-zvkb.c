/* sha512-riscv-zvknhb-zvkb.c - RISC-V vector crypto implementation of SHA-512
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
    defined(HAVE_COMPATIBLE_CC_RISCV_VECTOR_CRYPTO_INTRINSICS) && \
    defined(USE_SHA512)

#include "g10lib.h"
#include "simd-common-riscv.h"
#include <riscv_vector.h>


#define ALWAYS_INLINE inline __attribute__((always_inline))
#define NO_INLINE __attribute__((noinline))
#define NO_INSTRUMENT_FUNCTION __attribute__((no_instrument_function))

#define ASM_FUNC_ATTR          NO_INSTRUMENT_FUNCTION
#define ASM_FUNC_ATTR_INLINE   ASM_FUNC_ATTR ALWAYS_INLINE
#define ASM_FUNC_ATTR_NOINLINE ASM_FUNC_ATTR NO_INLINE


static ASM_FUNC_ATTR_INLINE vuint64m2_t
working_vsha2cl_vv_u64m2(vuint64m2_t hgcd, vuint64m2_t feba,
			 vuint64m2_t kw, size_t vl)
{
#ifdef HAVE_BROKEN_VSHA2CL_INTRINSIC
  asm (
    "vsetvli zero,%3,e64,m2,ta,ma;\n\t"
    "vsha2cl.vv %0,%1,%2;\n\t"
    : "+vr" (hgcd)
    : "vr" (feba), "vr" (kw), "r" (vl)
    : "vl", "vtype"
  );
  return hgcd;
#else
  return __riscv_vsha2cl_vv_u64m2(hgcd, feba, kw, vl);
#endif
}


/* Quad-round with message expansion (rounds 0-63) */
#define QUAD_ROUND_W_SCHED(w0, w1, w2, w3) \
  k_tmp = k; \
  asm ("" : "+r" (k_tmp) :: "memory"); \
  v_k = __riscv_vle64_v_u64m2(k_tmp, vl); \
  k += 4; \
  v_kw = __riscv_vadd_vv_u64m2(v_k, w0, vl); \
  v_hgcd_work = working_vsha2cl_vv_u64m2(v_hgcd_work, v_feba_work, v_kw, vl); \
  v_feba_work = __riscv_vsha2ch_vv_u64m2(v_feba_work, v_hgcd_work, v_kw, vl); \
  v_w_merged = __riscv_vmerge_vvm_u64m2(w2, w1, merge_mask, vl); \
  w0 = __riscv_vsha2ms_vv_u64m2(w0, v_w_merged, w3, vl);

/* Quad-round without message expansion (rounds 64-79) */
#define QUAD_ROUND_NO_SCHED(w0) \
  k_tmp = k; \
  asm ("" : "+r" (k_tmp) :: "memory"); \
  v_k = __riscv_vle64_v_u64m2(k_tmp, vl); \
  k += 4; \
  v_kw = __riscv_vadd_vv_u64m2(v_k, w0, vl); \
  v_hgcd_work = working_vsha2cl_vv_u64m2(v_hgcd_work, v_feba_work, v_kw, vl); \
  v_feba_work = __riscv_vsha2ch_vv_u64m2(v_feba_work, v_hgcd_work, v_kw, vl);


static ASM_FUNC_ATTR_INLINE vuint64m2_t
load_and_swap(const byte *p, size_t vl)
{
#ifdef RVV_UNALIGNED_NOT_ALLOWED
  vuint8m2_t temp = __riscv_vle8_v_u8m2(p, vl * 8);
  return __riscv_vrev8_v_u64m2(__riscv_vreinterpret_v_u8m2_u64m2(temp), vl);
#else
  return __riscv_vrev8_v_u64m2(__riscv_vle64_v_u64m2((const void *)p, vl), vl);
#endif
}


static ASM_FUNC_ATTR_INLINE void
sha512_transform_zvknhb_zvkb (u64 state[8], const byte *data,
			      size_t nblocks, const u64 k_const[80])
{
  static const u64 feba_hgcd_indices[4] = { 40, 32, 8, 0 };
  static const int feba_offset = 0;
  static const int hgcd_offset = 16 / sizeof(u64);
  size_t vl;
  vuint64m2_t idx;
  vuint64m2_t v_feba_work, v_feba;
  vuint64m2_t v_hgcd_work, v_hgcd;
  vuint64m2_t w0, w1, w2, w3;
  vuint64m2_t v_k, v_kw, v_w_merged;
  vbool32_t merge_mask;
  vuint64m2_t v_feba_hgcd_idx;

  vl = 4;
  idx = __riscv_vid_v_u64m2(vl);
  merge_mask = __riscv_vmseq_vx_u64m2_b32(idx, 0, vl);

  v_feba_hgcd_idx = __riscv_vle64_v_u64m2(feba_hgcd_indices, vl);

  v_feba = __riscv_vluxei64_v_u64m2(state + feba_offset, v_feba_hgcd_idx, vl);
  v_hgcd = __riscv_vluxei64_v_u64m2(state + hgcd_offset, v_feba_hgcd_idx, vl);

  while (nblocks > 0)
    {
      const u64 *k = k_const;
      const u64 *k_tmp;

      v_feba_work = v_feba;
      v_hgcd_work = v_hgcd;

      w0 = load_and_swap(data + 0, vl);
      w1 = load_and_swap(data + 32, vl);
      w2 = load_and_swap(data + 64, vl);
      w3 = load_and_swap(data + 96, vl);

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
      QUAD_ROUND_W_SCHED(w0, w1, w2, w3);
      QUAD_ROUND_W_SCHED(w1, w2, w3, w0);
      QUAD_ROUND_W_SCHED(w2, w3, w0, w1);
      QUAD_ROUND_W_SCHED(w3, w0, w1, w2);

      QUAD_ROUND_NO_SCHED(w0);
      QUAD_ROUND_NO_SCHED(w1);
      QUAD_ROUND_NO_SCHED(w2);
      QUAD_ROUND_NO_SCHED(w3);

      v_feba = __riscv_vadd_vv_u64m2(v_feba, v_feba_work, vl);
      v_hgcd = __riscv_vadd_vv_u64m2(v_hgcd, v_hgcd_work, vl);

      data += 128;
      nblocks--;
    }

  __riscv_vsuxei64_v_u64m2(state + feba_offset, v_feba_hgcd_idx, v_feba, vl);
  __riscv_vsuxei64_v_u64m2(state + hgcd_offset, v_feba_hgcd_idx, v_hgcd, vl);

  clear_vec_regs();
}


#ifdef HAVE_GCC_ATTRIBUTE_OPTIMIZE
# define FUNC_ATTR_OPT_O2 __attribute__((optimize("-O2")))
#else
# define FUNC_ATTR_OPT_O2
#endif

unsigned int ASM_FUNC_ATTR FUNC_ATTR_OPT_O2
_gcry_sha512_transform_riscv_zvknhb_zvkb(u64 state[8],
					 const unsigned char *input_data,
					 size_t num_blks,
					 const u64 k[80])
{
  sha512_transform_zvknhb_zvkb(state, input_data, num_blks, k);
  return 0;
}

unsigned int ASM_FUNC_ATTR FUNC_ATTR_OPT_O2
_gcry_sha512_riscv_v_check_hw(void)
{
  return (__riscv_vsetvl_e64m2(4) == 4);
}

#endif /* HAVE_COMPATIBLE_CC_RISCV_VECTOR_INTRINSICS */
