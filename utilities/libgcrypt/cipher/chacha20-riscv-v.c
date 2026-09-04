/* chacha20-riscv-v.c - RISC-V vector implementation of ChaCha20
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
    defined(USE_CHACHA20)

#include "simd-common-riscv.h"
#include <riscv_vector.h>
#include "bufhelp.h"


#define ALWAYS_INLINE inline __attribute__((always_inline))
#define NO_INLINE __attribute__((noinline))
#define NO_INSTRUMENT_FUNCTION __attribute__((no_instrument_function))

#define ASM_FUNC_ATTR          NO_INSTRUMENT_FUNCTION
#define ASM_FUNC_ATTR_INLINE   ASM_FUNC_ATTR ALWAYS_INLINE
#define ASM_FUNC_ATTR_NOINLINE ASM_FUNC_ATTR NO_INLINE


/**********************************************************************
  RISC-V vector extension chacha20
 **********************************************************************/

#define ROTATE16(v)	__riscv_vreinterpret_v_u16m1_u32m1( \
				__riscv_vrgather_vv_u16m1( \
					__riscv_vreinterpret_v_u32m1_u16m1(v), \
					rot16, vl * 2))
#define ROTATE8(v)	__riscv_vreinterpret_v_u8m1_u32m1( \
				__riscv_vrgather_vv_u8m1( \
					__riscv_vreinterpret_v_u32m1_u8m1(v), \
					rot8, vl * 4))
#define ROTATE(v, c)	__riscv_vadd_vv_u32m1( \
				__riscv_vsll_vx_u32m1((v), (c), vl), \
				__riscv_vsrl_vx_u32m1((v), 32 - (c), vl), vl)
#define XOR(v, w)	__riscv_vxor_vv_u32m1((v), (w), vl)
#define PLUS(v, w)	__riscv_vadd_vv_u32m1((v), (w), vl)
#define WORD_ROL(v, c)	__riscv_vrgather_vv_u32m1((v), (rol##c), vl)

#define QUARTERROUND_4(a0, b0, c0, d0, a1, b1, c1, d1, \
		       a2, b2, c2, d2, a3, b3, c3, d3) \
  a0 = PLUS(a0, b0); a1 = PLUS(a1, b1); \
  a2 = PLUS(a2, b2); a3 = PLUS(a3, b3); \
    d0 = XOR(d0, a0); d1 = XOR(d1, a1); \
    d2 = XOR(d2, a2); d3 = XOR(d3, a3); \
      d0 = ROTATE16(d0); d1 = ROTATE16(d1); \
      d2 = ROTATE16(d2); d3 = ROTATE16(d3); \
  c0 = PLUS(c0, d0); c1 = PLUS(c1, d1); \
  c2 = PLUS(c2, d2); c3 = PLUS(c3, d3); \
    b0 = XOR(b0, c0); b1 = XOR(b1, c1); \
    b2 = XOR(b2, c2); b3 = XOR(b3, c3); \
      b0 = ROTATE(b0, 12); b1 = ROTATE(b1, 12); \
      b2 = ROTATE(b2, 12); b3 = ROTATE(b3, 12); \
  a0 = PLUS(a0, b0); a1 = PLUS(a1, b1); \
  a2 = PLUS(a2, b2); a3 = PLUS(a3, b3); \
    d0 = XOR(d0, a0); d1 = XOR(d1, a1); \
    d2 = XOR(d2, a2); d3 = XOR(d3, a3); \
      d0 = ROTATE8(d0); d1 = ROTATE8(d1); \
      d2 = ROTATE8(d2); d3 = ROTATE8(d3); \
  c0 = PLUS(c0, d0); c1 = PLUS(c1, d1); \
  c2 = PLUS(c2, d2); c3 = PLUS(c3, d3); \
    b0 = XOR(b0, c0); b1 = XOR(b1, c1); \
    b2 = XOR(b2, c2); b3 = XOR(b3, c3); \
      b0 = ROTATE(b0, 7); b1 = ROTATE(b1, 7); \
      b2 = ROTATE(b2, 7); b3 = ROTATE(b3, 7);

#define QUARTERROUND4_2(x0, x1, x2, x3, y0, y1, y2, y3, rol_x1, rol_x2, rol_x3) \
  x0 = PLUS(x0, x1); y0 = PLUS(y0, y1); \
    x3 = XOR(x3, x0); y3 = XOR(y3, y0); \
      x3 = ROTATE16(x3); y3 = ROTATE16(y3); \
  x2 = PLUS(x2, x3); y2 = PLUS(y2, y3); \
    x1 = XOR(x1, x2); y1 = XOR(y1, y2); \
      x1 = ROTATE(x1, 12); y1 = ROTATE(y1, 12); \
  x0 = PLUS(x0, x1); y0 = PLUS(y0, y1); \
    x3 = XOR(x3, x0); y3 = XOR(y3, y0); \
      x3 = ROTATE8(x3); y3 = ROTATE8(y3); \
  x2 = PLUS(x2, x3); y2 = PLUS(y2, y3); \
    x3 = WORD_ROL(x3, rol_x3); y3 = WORD_ROL(y3, rol_x3);\
      x1 = XOR(x1, x2); y1 = XOR(y1, y2); \
	x2 = WORD_ROL(x2, rol_x2); y2 = WORD_ROL(y2, rol_x2); \
	  x1 = ROTATE(x1, 7); y1 = ROTATE(y1, 7); \
	    x1 = WORD_ROL(x1, rol_x1); y1 = WORD_ROL(y1, rol_x1);

#define QUARTERROUND4(x0, x1, x2, x3, rol_x1, rol_x2, rol_x3) \
  x0 = PLUS(x0, x1); x3 = XOR(x3, x0); x3 = ROTATE16(x3); \
  x2 = PLUS(x2, x3); x1 = XOR(x1, x2); x1 = ROTATE(x1, 12); \
  x0 = PLUS(x0, x1); x3 = XOR(x3, x0); x3 = ROTATE8(x3); \
  x2 = PLUS(x2, x3); \
    x3 = WORD_ROL(x3, rol_x3); \
		     x1 = XOR(x1, x2); \
    x2 = WORD_ROL(x2, rol_x2); \
				       x1= ROTATE(x1, 7); \
    x1 = WORD_ROL(x1, rol_x1);

#define ADD_U64(a, b) __riscv_vreinterpret_v_u64m1_u32m1( \
			__riscv_vadd_vv_u64m1( \
			  __riscv_vreinterpret_v_u32m1_u64m1(a), \
			  __riscv_vreinterpret_v_u32m1_u64m1(b), vl / 2))

#define vxor_v_u32m1_u32m1x8(data, idx, vs, vl) \
      __riscv_vset_v_u32m1_u32m1x8((data), (idx), \
	  __riscv_vxor_vv_u32m1( \
	      __riscv_vget_v_u32m1x8_u32m1((data), (idx)), (vs), (vl)))

static ASM_FUNC_ATTR_INLINE vuint16m1_t
gen_rot16(size_t vl)
{
  return __riscv_vxor_vx_u16m1(__riscv_vid_v_u16m1(vl * 2), 1, vl * 2);
}

static ASM_FUNC_ATTR_INLINE vuint8m1_t
gen_rot8(size_t vl)
{
  vuint8m1_t rot8, rot8_hi;

  rot8 = __riscv_vid_v_u8m1(vl * 4);
  rot8_hi = __riscv_vand_vx_u8m1(rot8, ~3, vl * 4);
  rot8 = __riscv_vadd_vx_u8m1(rot8, 3, vl * 4);
  rot8 = __riscv_vand_vx_u8m1(rot8, 3, vl * 4);
  rot8 = __riscv_vadd_vv_u8m1(rot8, rot8_hi, vl * 4);

  return rot8;
}

static ASM_FUNC_ATTR_INLINE vuint16m2_t
gen_indexes(size_t vl, size_t stride)
{
  vuint16m2_t idx = __riscv_vid_v_u16m2(vl * 4);
  vuint16m2_t idx_lo = __riscv_vand_vx_u16m2(idx, 3, vl * 4);
  vuint16m2_t idx_hi = __riscv_vsrl_vx_u16m2(idx, 2, vl * 4);
  idx_hi = __riscv_vmul_vx_u16m2(idx_hi, stride, vl * 4);
  return __riscv_vadd_vv_u16m2(idx_hi, idx_lo, vl * 4);
}

static ASM_FUNC_ATTR_INLINE vuint32m1_t
unaligned_load_u32m1(const void *src, size_t vl)
{
#ifdef RVV_UNALIGNED_NOT_ALLOWED
  return __riscv_vreinterpret_v_u8m1_u32m1(__riscv_vle8_v_u8m1(src, vl * 4));
#else
  return __riscv_vle32_v_u32m1(src, vl);
#endif
}

static ASM_FUNC_ATTR_INLINE void
unaligned_store_u32m1(void *dst, vuint32m1_t vec, size_t vl)
{
#ifdef RVV_UNALIGNED_NOT_ALLOWED
  __riscv_vse8_v_u8m1(dst, __riscv_vreinterpret_v_u32m1_u8m1(vec), vl * 4);
#else
  __riscv_vse32_v_u32m1(dst, vec, vl);
#endif
}

static ASM_FUNC_ATTR_INLINE vuint32m1x8_t
unaligned_vlsseg8e32_v_u32m1x8(const void *src, size_t vl)
{
#ifdef RVV_UNALIGNED_NOT_ALLOWED
  if (UNLIKELY(((uintptr_t)src & 3) != 0))
    {
      const byte *bsrc = src;
      vuint16m2_t indexes;
      vuint8m1_t b0, b1, b2, b3, b4, b5, b6, b7;
      vuint32m1x8_t data;

      indexes = gen_indexes(4 * vl, 64);

      b0 = __riscv_vluxei16_v_u8m1(bsrc + 0 * 4, indexes, vl * 4);
      b1 = __riscv_vluxei16_v_u8m1(bsrc + 1 * 4, indexes, vl * 4);
      b2 = __riscv_vluxei16_v_u8m1(bsrc + 2 * 4, indexes, vl * 4);
      b3 = __riscv_vluxei16_v_u8m1(bsrc + 3 * 4, indexes, vl * 4);
      b4 = __riscv_vluxei16_v_u8m1(bsrc + 4 * 4, indexes, vl * 4);
      b5 = __riscv_vluxei16_v_u8m1(bsrc + 5 * 4, indexes, vl * 4);
      b6 = __riscv_vluxei16_v_u8m1(bsrc + 6 * 4, indexes, vl * 4);
      b7 = __riscv_vluxei16_v_u8m1(bsrc + 7 * 4, indexes, vl * 4);

      data = __riscv_vundefined_u32m1x8();
      data = __riscv_vset_v_u32m1_u32m1x8(
		data, 0, __riscv_vreinterpret_v_u8m1_u32m1(b0));
      data = __riscv_vset_v_u32m1_u32m1x8(
		data, 1, __riscv_vreinterpret_v_u8m1_u32m1(b1));
      data = __riscv_vset_v_u32m1_u32m1x8(
		data, 2, __riscv_vreinterpret_v_u8m1_u32m1(b2));
      data = __riscv_vset_v_u32m1_u32m1x8(
		data, 3, __riscv_vreinterpret_v_u8m1_u32m1(b3));
      data = __riscv_vset_v_u32m1_u32m1x8(
		data, 4, __riscv_vreinterpret_v_u8m1_u32m1(b4));
      data = __riscv_vset_v_u32m1_u32m1x8(
		data, 5, __riscv_vreinterpret_v_u8m1_u32m1(b5));
      data = __riscv_vset_v_u32m1_u32m1x8(
		data, 6, __riscv_vreinterpret_v_u8m1_u32m1(b6));
      data = __riscv_vset_v_u32m1_u32m1x8(
		data, 7, __riscv_vreinterpret_v_u8m1_u32m1(b7));

      return data;
    }
  else
#endif
    {
      /* Fast path for 32-bit aligned loads. */
      return __riscv_vlsseg8e32_v_u32m1x8(src, 64, vl);
    }
}

static ASM_FUNC_ATTR_INLINE void
unaligned_vssseg8e32_v_u32m1x8(void *dst, vuint32m1x8_t data, size_t vl)
{
#ifdef RVV_UNALIGNED_NOT_ALLOWED
  if (UNLIKELY(((uintptr_t)dst & 3) != 0))
    {
      byte *bdst = dst;
      vuint16m2_t indexes;
      vuint8m1_t b0, b1, b2, b3, b4, b5, b6, b7;

      indexes = gen_indexes(4 * vl, 64);

      b0 = __riscv_vreinterpret_v_u32m1_u8m1(__riscv_vget_v_u32m1x8_u32m1(data, 0));
      b1 = __riscv_vreinterpret_v_u32m1_u8m1(__riscv_vget_v_u32m1x8_u32m1(data, 1));
      b2 = __riscv_vreinterpret_v_u32m1_u8m1(__riscv_vget_v_u32m1x8_u32m1(data, 2));
      b3 = __riscv_vreinterpret_v_u32m1_u8m1(__riscv_vget_v_u32m1x8_u32m1(data, 3));
      b4 = __riscv_vreinterpret_v_u32m1_u8m1(__riscv_vget_v_u32m1x8_u32m1(data, 4));
      b5 = __riscv_vreinterpret_v_u32m1_u8m1(__riscv_vget_v_u32m1x8_u32m1(data, 5));
      b6 = __riscv_vreinterpret_v_u32m1_u8m1(__riscv_vget_v_u32m1x8_u32m1(data, 6));
      b7 = __riscv_vreinterpret_v_u32m1_u8m1(__riscv_vget_v_u32m1x8_u32m1(data, 7));

      __riscv_vsuxei16_v_u8m1(bdst + 0 * 4, indexes, b0, vl * 4);
      __riscv_vsuxei16_v_u8m1(bdst + 1 * 4, indexes, b1, vl * 4);
      __riscv_vsuxei16_v_u8m1(bdst + 2 * 4, indexes, b2, vl * 4);
      __riscv_vsuxei16_v_u8m1(bdst + 3 * 4, indexes, b3, vl * 4);
      __riscv_vsuxei16_v_u8m1(bdst + 4 * 4, indexes, b4, vl * 4);
      __riscv_vsuxei16_v_u8m1(bdst + 5 * 4, indexes, b5, vl * 4);
      __riscv_vsuxei16_v_u8m1(bdst + 6 * 4, indexes, b6, vl * 4);
      __riscv_vsuxei16_v_u8m1(bdst + 7 * 4, indexes, b7, vl * 4);
    }
  else
#endif
    {
      /* Fast path for 32-bit aligned stores. */
      __riscv_vssseg8e32_v_u32m1x8(dst, 64, data, vl);
      return;
    }
}

static ASM_FUNC_ATTR_INLINE unsigned int
chacha20_rvv_blocks(u32 *input, byte *dst, const byte *src, size_t nblks)
{
  unsigned int i;

  if (nblks == 0)
    return 0;

  /* Try use vector implementation when there is 4 or more blocks. */
  if (nblks >= 4)
    {
      size_t vl = __riscv_vsetvl_e32m1(nblks) < 4
		    ? __riscv_vsetvl_e32m1(4) : __riscv_vsetvl_e32m1(nblks);
      vuint32m1_t x0, x1, x2, x3, x4, x5, x6, x7;
      vuint32m1_t x8, x9, x10, x11, x12, x13, x14, x15;
      u32 s0, s1, s2, s3, s4, s5, s6, s7;
      u32 s8, s9, s10, s11, s12, s13, s14, s15;
      vuint16m1_t rot16 = gen_rot16(vl);
      vuint8m1_t rot8 = gen_rot8(vl);

      s0 = input[0];
      s1 = input[1];
      s2 = input[2];
      s3 = input[3];
      s4 = input[4];
      s5 = input[5];
      s6 = input[6];
      s7 = input[7];
      s8 = input[8];
      s9 = input[9];
      s10 = input[10];
      s11 = input[11];
      s12 = input[12];
      s13 = input[13];
      s14 = input[14];
      s15 = input[15];

      while (nblks >= 4)
	{
	  vuint32m1_t ctr;
	  vbool32_t carry;
	  vuint32m1x8_t data;

	  if (vl < 4)
	    break;

	  x0 = __riscv_vmv_v_x_u32m1(s0, vl);
	  x1 = __riscv_vmv_v_x_u32m1(s1, vl);
	  x2 = __riscv_vmv_v_x_u32m1(s2, vl);
	  x3 = __riscv_vmv_v_x_u32m1(s3, vl);
	  x4 = __riscv_vmv_v_x_u32m1(s4, vl);
	  x5 = __riscv_vmv_v_x_u32m1(s5, vl);
	  x6 = __riscv_vmv_v_x_u32m1(s6, vl);
	  x7 = __riscv_vmv_v_x_u32m1(s7, vl);
	  x8 = __riscv_vmv_v_x_u32m1(s8, vl);
	  x9 = __riscv_vmv_v_x_u32m1(s9, vl);
	  x10 = __riscv_vmv_v_x_u32m1(s10, vl);
	  x11 = __riscv_vmv_v_x_u32m1(s11, vl);
	  x13 = __riscv_vmv_v_x_u32m1(s13, vl);
	  x14 = __riscv_vmv_v_x_u32m1(s14, vl);
	  x15 = __riscv_vmv_v_x_u32m1(s15, vl);

	  ctr = __riscv_vid_v_u32m1(vl);
	  carry = __riscv_vmadc_vx_u32m1_b32(ctr, s12, vl);
	  ctr = __riscv_vadd_vx_u32m1(ctr, s12, vl);
	  x12 = ctr;
	  x13 = __riscv_vadc_vxm_u32m1(x13, 0, carry, vl);

	  for (i = 20; i > 0; i -= 2)
	    {
	      QUARTERROUND_4(x0, x4,  x8, x12,
			     x1, x5,  x9, x13,
			     x2, x6, x10, x14,
			     x3, x7, x11, x15);
	      QUARTERROUND_4(x0, x5, x10, x15,
			     x1, x6, x11, x12,
		             x2, x7,  x8, x13,
		             x3, x4,  x9, x14);
	    }

	  x0 = __riscv_vadd_vx_u32m1(x0, s0, vl);
	  x1 = __riscv_vadd_vx_u32m1(x1, s1, vl);
	  x2 = __riscv_vadd_vx_u32m1(x2, s2, vl);
	  x3 = __riscv_vadd_vx_u32m1(x3, s3, vl);
	  x4 = __riscv_vadd_vx_u32m1(x4, s4, vl);
	  x5 = __riscv_vadd_vx_u32m1(x5, s5, vl);
	  x6 = __riscv_vadd_vx_u32m1(x6, s6, vl);
	  x7 = __riscv_vadd_vx_u32m1(x7, s7, vl);
	  x8 = __riscv_vadd_vx_u32m1(x8, s8, vl);
	  x9 = __riscv_vadd_vx_u32m1(x9, s9, vl);
	  x10 = __riscv_vadd_vx_u32m1(x10, s10, vl);
	  x11 = __riscv_vadd_vx_u32m1(x11, s11, vl);
	  x12 = __riscv_vadd_vv_u32m1(x12, ctr, vl);
	  x13 = __riscv_vadc_vxm_u32m1(x13, s13, carry, vl);
	  x14 = __riscv_vadd_vx_u32m1(x14, s14, vl);
	  x15 = __riscv_vadd_vx_u32m1(x15, s15, vl);

	  s12 += vl;
	  s13 += s12 < vl;

	  data = unaligned_vlsseg8e32_v_u32m1x8((const void *)src, vl);

	  data = vxor_v_u32m1_u32m1x8(data, 0, x0, vl);
	  data = vxor_v_u32m1_u32m1x8(data, 1, x1, vl);
	  data = vxor_v_u32m1_u32m1x8(data, 2, x2, vl);
	  data = vxor_v_u32m1_u32m1x8(data, 3, x3, vl);
	  data = vxor_v_u32m1_u32m1x8(data, 4, x4, vl);
	  data = vxor_v_u32m1_u32m1x8(data, 5, x5, vl);
	  data = vxor_v_u32m1_u32m1x8(data, 6, x6, vl);
	  data = vxor_v_u32m1_u32m1x8(data, 7, x7, vl);

	  unaligned_vssseg8e32_v_u32m1x8((void *)dst, data, vl);

	  data = unaligned_vlsseg8e32_v_u32m1x8((const void *)(src + 32), vl);

	  data = vxor_v_u32m1_u32m1x8(data, 0, x8, vl);
	  data = vxor_v_u32m1_u32m1x8(data, 1, x9, vl);
	  data = vxor_v_u32m1_u32m1x8(data, 2, x10, vl);
	  data = vxor_v_u32m1_u32m1x8(data, 3, x11, vl);
	  data = vxor_v_u32m1_u32m1x8(data, 4, x12, vl);
	  data = vxor_v_u32m1_u32m1x8(data, 5, x13, vl);
	  data = vxor_v_u32m1_u32m1x8(data, 6, x14, vl);
	  data = vxor_v_u32m1_u32m1x8(data, 7, x15, vl);

	  unaligned_vssseg8e32_v_u32m1x8((void *)(dst + 32), data, vl);

	  src += vl * 64;
	  dst += vl * 64;
	  nblks -= vl;
	  vl = __riscv_vsetvl_e32m1(nblks) < 4
		    ? __riscv_vsetvl_e32m1(4) : __riscv_vsetvl_e32m1(nblks);
	}

      input[12] = s12;
      input[13] = s13;
    }

  /* Use SIMD implementation for remaining blocks. */
  if (nblks > 0)
    {
      static const u32 rol_const[3][4] =
	{
	  { 1, 2, 3, 0 },
	  { 2, 3, 0, 1 },
	  { 3, 0, 1, 2 }
	};
      static const u32 one_u64_const[4] = { 1, 0, 0, 0 };
      size_t vl = 4;
      vuint32m1_t rol1, rol2, rol3;
      vuint32m1_t one_u64;
      vuint32m1_t v0, v1, v2, v3;
      vuint32m1_t v4, v5, v6, v7;
      vuint32m1_t state0, state1, state2, state3;
      vuint32m1_t i0, i1, i2, i3;
      vuint32m1_t i4, i5, i6, i7;
      vuint16m1_t rot16 = gen_rot16(vl);
      vuint8m1_t rot8 = gen_rot8(vl);

      rol1 = __riscv_vle32_v_u32m1(rol_const[0], vl);
      rol2 = __riscv_vle32_v_u32m1(rol_const[1], vl);
      rol3 = __riscv_vle32_v_u32m1(rol_const[2], vl);
      one_u64 = __riscv_vle32_v_u32m1(one_u64_const, vl);

      state0 = __riscv_vle32_v_u32m1(&input[0], vl);
      state1 = __riscv_vle32_v_u32m1(&input[4], vl);
      state2 = __riscv_vle32_v_u32m1(&input[8], vl);
      state3 = __riscv_vle32_v_u32m1(&input[12], vl);

      input[12] += nblks;
      input[13] += input[12] < nblks;

      /* SIMD 2x block implementation */
      while (nblks >= 2)
	{
	  v0 = state0;
	  v1 = state1;
	  v2 = state2;
	  v3 = state3;

	  v4 = state0;
	  v5 = state1;
	  v6 = state2;
	  v7 = state3;
	  v7 = ADD_U64(v7, one_u64);

	  i0 = unaligned_load_u32m1(src + 0 * 16, vl);
	  i1 = unaligned_load_u32m1(src + 1 * 16, vl);
	  i2 = unaligned_load_u32m1(src + 2 * 16, vl);
	  i3 = unaligned_load_u32m1(src + 3 * 16, vl);

	  for (i = 20; i > 0; i -= 2)
	    {
	      QUARTERROUND4_2(v0, v1, v2, v3, v4, v5, v6, v7, 1, 2, 3);
	      QUARTERROUND4_2(v0, v1, v2, v3, v4, v5, v6, v7, 3, 2, 1);
	    }

	  v0 = __riscv_vadd_vv_u32m1(v0, state0, vl);
	  v1 = __riscv_vadd_vv_u32m1(v1, state1, vl);
	  v2 = __riscv_vadd_vv_u32m1(v2, state2, vl);
	  v3 = __riscv_vadd_vv_u32m1(v3, state3, vl);
	  state3 = ADD_U64(state3, one_u64);

	  v0 = __riscv_vxor_vv_u32m1(i0, v0, vl);
	  v1 = __riscv_vxor_vv_u32m1(i1, v1, vl);
	  v2 = __riscv_vxor_vv_u32m1(i2, v2, vl);
	  v3 = __riscv_vxor_vv_u32m1(i3, v3, vl);

	  v4 = __riscv_vadd_vv_u32m1(v4, state0, vl);
	  v5 = __riscv_vadd_vv_u32m1(v5, state1, vl);
	  v6 = __riscv_vadd_vv_u32m1(v6, state2, vl);
	  v7 = __riscv_vadd_vv_u32m1(v7, state3, vl);
	  state3 = ADD_U64(state3, one_u64);

	  i4 = unaligned_load_u32m1(src + 4 * 16, vl);
	  i5 = unaligned_load_u32m1(src + 5 * 16, vl);
	  i6 = unaligned_load_u32m1(src + 6 * 16, vl);
	  i7 = unaligned_load_u32m1(src + 7 * 16, vl);

	  unaligned_store_u32m1(dst + 0 * 16, v0, vl);
	  unaligned_store_u32m1(dst + 1 * 16, v1, vl);
	  unaligned_store_u32m1(dst + 2 * 16, v2, vl);
	  unaligned_store_u32m1(dst + 3 * 16, v3, vl);

	  v4 = __riscv_vxor_vv_u32m1(i4, v4, vl);
	  v5 = __riscv_vxor_vv_u32m1(i5, v5, vl);
	  v6 = __riscv_vxor_vv_u32m1(i6, v6, vl);
	  v7 = __riscv_vxor_vv_u32m1(i7, v7, vl);

	  unaligned_store_u32m1(dst + 4 * 16, v4, vl);
	  unaligned_store_u32m1(dst + 5 * 16, v5, vl);
	  unaligned_store_u32m1(dst + 6 * 16, v6, vl);
	  unaligned_store_u32m1(dst + 7 * 16, v7, vl);

	  src += 2 * 64;
	  dst += 2 * 64;

	  nblks -= 2;
	}

      /* 1x block implementation */
      while (nblks)
	{
	  v0 = state0;
	  v1 = state1;
	  v2 = state2;
	  v3 = state3;

	  i0 = unaligned_load_u32m1(src + 0 * 16, vl);
	  i1 = unaligned_load_u32m1(src + 1 * 16, vl);
	  i2 = unaligned_load_u32m1(src + 2 * 16, vl);
	  i3 = unaligned_load_u32m1(src + 3 * 16, vl);

	  for (i = 20; i > 0; i -= 2)
	    {
	      QUARTERROUND4(v0, v1, v2, v3, 1, 2, 3);
	      QUARTERROUND4(v0, v1, v2, v3, 3, 2, 1);
	    }

	  v0 = __riscv_vadd_vv_u32m1(v0, state0, vl);
	  v1 = __riscv_vadd_vv_u32m1(v1, state1, vl);
	  v2 = __riscv_vadd_vv_u32m1(v2, state2, vl);
	  v3 = __riscv_vadd_vv_u32m1(v3, state3, vl);

	  state3 = ADD_U64(state3, one_u64);

	  v0 = __riscv_vxor_vv_u32m1(i0, v0, vl);
	  v1 = __riscv_vxor_vv_u32m1(i1, v1, vl);
	  v2 = __riscv_vxor_vv_u32m1(i2, v2, vl);
	  v3 = __riscv_vxor_vv_u32m1(i3, v3, vl);
	  unaligned_store_u32m1(dst + 0 * 16, v0, vl);
	  unaligned_store_u32m1(dst + 1 * 16, v1, vl);
	  unaligned_store_u32m1(dst + 2 * 16, v2, vl);
	  unaligned_store_u32m1(dst + 3 * 16, v3, vl);
	  src += 64;
	  dst += 64;

	  nblks--;
	}
    }

  clear_vec_regs();

  return 0;
}


#ifdef HAVE_GCC_ATTRIBUTE_OPTIMIZE
# define FUNC_ATTR_OPT_O2 __attribute__((optimize("-O2")))
#else
# define FUNC_ATTR_OPT_O2
#endif


unsigned int ASM_FUNC_ATTR FUNC_ATTR_OPT_O2
_gcry_chacha20_riscv_v_blocks(u32 *state, byte *dst, const byte *src,
			      size_t nblks)
{
  return chacha20_rvv_blocks(state, dst, src, nblks);
}

unsigned int ASM_FUNC_ATTR FUNC_ATTR_OPT_O2
_gcry_chacha20_riscv_v_check_hw(void)
{
  return (__riscv_vsetvl_e8m1(16) == 16);
}

#endif /* HAVE_COMPATIBLE_CC_RISCV_VECTOR_INTRINSICS */
