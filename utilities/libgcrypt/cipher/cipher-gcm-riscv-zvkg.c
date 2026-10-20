/* cipher-gcm-riscv-zvkg.c - RISC-V vector cryptography Zvkg accelerated GHASH
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

#include "g10lib.h"
#include "simd-common-riscv.h"
#include "cipher-internal.h"

#ifdef GCM_USE_RISCV_ZVKG

#include <riscv_vector.h>


#define ALWAYS_INLINE inline __attribute__((always_inline))
#define NO_INLINE __attribute__((noinline))
#define NO_INSTRUMENT_FUNCTION __attribute__((no_instrument_function))

#define ASM_FUNC_ATTR          NO_INSTRUMENT_FUNCTION
#define ASM_FUNC_ATTR_INLINE   ASM_FUNC_ATTR ALWAYS_INLINE
#define ASM_FUNC_ATTR_NOINLINE ASM_FUNC_ATTR NO_INLINE


#define cast_u8m1_u32m1(a) __riscv_vreinterpret_v_u8m1_u32m1(a)
#define cast_u32m1_u8m1(a) __riscv_vreinterpret_v_u32m1_u8m1(a)


static ASM_FUNC_ATTR_INLINE vuint32m1_t
unaligned_load_u32m1(const void *ptr, size_t vl_u32)
{
#ifdef RVV_UNALIGNED_NOT_ALLOWED
  size_t vl_bytes = vl_u32 * 4;
  return cast_u8m1_u32m1(__riscv_vle8_v_u8m1(ptr, vl_bytes));
#else
  return __riscv_vle32_v_u32m1(ptr, vl_u32);
#endif
}

static ASM_FUNC_ATTR_INLINE vuint32m1_t
bswap128_u32m1(vuint32m1_t vec, size_t vl_u32)
{
  static const byte bswap128_arr[16] =
    { 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
  size_t vl_bytes = vl_u32 * 4;
  vuint8m1_t bswap128 = __riscv_vle8_v_u8m1(bswap128_arr, vl_bytes);

  return cast_u8m1_u32m1(
	    __riscv_vrgather_vv_u8m1(cast_u32m1_u8m1(vec), bswap128, vl_bytes));
}


ASM_FUNC_ATTR_NOINLINE int
_gcry_ghash_setup_riscv_zvkg(gcry_cipher_hd_t c)
{
  (void)c;

  if (__riscv_vsetvl_e32m1(4) != 4)
    {
      return 0; // VLEN=128 not supported.
    }

  return 1;
}

ASM_FUNC_ATTR_NOINLINE unsigned int
_gcry_ghash_riscv_zvkg(gcry_cipher_hd_t c, byte *result, const byte *buf,
		       size_t nblocks)
{
  u32 *result_u32 = (void *)result;
  const u32 *key_u32 = (void *)c->u_mode.gcm.u_ghash_key.key;
  size_t vl = 4;
  vuint32m1_t rhash = __riscv_vle32_v_u32m1(result_u32, vl);
  vuint32m1_t rh1 = __riscv_vle32_v_u32m1(key_u32, vl);

  while (nblocks)
    {
      vuint32m1_t data = unaligned_load_u32m1(buf, vl);
      buf += 16;
      nblocks--;

      rhash = __riscv_vghsh_vv_u32m1(rhash, rh1, data, vl);
    }

  __riscv_vse32_v_u32m1(result_u32, rhash, vl);

  clear_vec_regs();

  return 0;
}

ASM_FUNC_ATTR_NOINLINE unsigned int
_gcry_polyval_riscv_zvkg(gcry_cipher_hd_t c, byte *result, const byte *buf,
		       size_t nblocks)
{
  u32 *result_u32 = (void *)result;
  const u32 *key_u32 = (void *)c->u_mode.gcm.u_ghash_key.key;
  size_t vl = 4;
  vuint32m1_t rhash = __riscv_vle32_v_u32m1(result_u32, vl);
  vuint32m1_t rh1 = __riscv_vle32_v_u32m1(key_u32, vl);

  while (nblocks)
    {
      vuint32m1_t data = bswap128_u32m1(unaligned_load_u32m1(buf, vl), vl);
      buf += 16;
      nblocks--;

      rhash = __riscv_vghsh_vv_u32m1(rhash, rh1, data, vl);
    }

  __riscv_vse32_v_u32m1(result_u32, rhash, vl);

  clear_vec_regs();

  return 0;
}

#endif /* GCM_USE_RISCV_V_ZVKG */
