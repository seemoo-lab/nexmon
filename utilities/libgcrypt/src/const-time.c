/* const-time.c  -  Constant-time functions
 *      Copyright (C) 2023  g10 Code GmbH
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
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include "g10lib.h"
#include "const-time.h"
#include "../cipher/bufhelp.h"


#ifndef HAVE_GCC_ASM_VOLATILE_MEMORY
/* These variables are used to generate masks from conditional operation
 * flag parameters.  Use of volatile prevents compiler optimizations from
 * converting AND-masking to conditional branches.  */
volatile unsigned int _gcry_ct_vzero = 0;
volatile unsigned int _gcry_ct_vone = 1;
#endif


/*
 * Compare byte arrays of length LEN, return 1 if it's not same,
 * 0, otherwise.
 */
unsigned int
_gcry_ct_not_memequal (const void *b1, const void *b2, size_t len)
{
  const byte *a = b1;
  const byte *b = b2;
  u32 ab = 0;
  u32 ba = 0;

  /* Constant-time compare. */

  if (len >= sizeof(u64))
    {
      u64 ab8 = 0;
      u64 ba8 = 0;

      while (len >= sizeof(u64))
	{
	  u64 a8 = buf_get_he64 (a);
	  u64 b8 = buf_get_he64 (b);

	  /* If a8 != b8, either ab8 or ba8 will have high bit set. */
	  ab8 |= a8 - b8;
	  ba8 |= b8 - a8;

	  a += sizeof(u64);
	  b += sizeof(u64);
	  len -= sizeof(u64);
	}

      ab = ct_u64_gen_mask ((ab8 >> (sizeof(u64) * 8 - 1)) & 1);
      ba = ct_u64_gen_mask ((ba8 >> (sizeof(u64) * 8 - 1)) & 1);
    }

  while (len > 0)
    {
      /* If *a != *b, either ab or ba will have high bit set. */
      ab |= *a - *b;
      ba |= *b - *a;
      a++;
      b++;
      len--;
    }

  /* 'ab | ba' has high bit set when buffers are not equal.  */
  return ((ab | ba) >> (sizeof(u32) * 8 - 1)) & 1;
}

/*
 * Compare byte arrays of length LEN, return 0 if it's not same,
 * 1, otherwise.
 */
unsigned int
_gcry_ct_memequal (const void *b1, const void *b2, size_t len)
{
  return _gcry_ct_not_memequal (b1, b2, len) ^ 1;
}

/*
 * Copy LEN bytes from memory area SRC to memory area DST, when
 * OP_ENABLED=1.  When DST <= SRC, the memory areas may overlap.  When
 * DST > SRC, the memory areas must not overlap.
 */
void
_gcry_ct_memmov_cond (void *dst, const void *src, size_t len,
		      unsigned long op_enable)
{
  /* Note: dual mask with AND/OR used for EM leakage mitigation */
  u64 mask1 = ct_u64_gen_mask (op_enable);
  u64 mask2 = ct_u64_gen_inv_mask (op_enable);
  unsigned char *b_dst = dst;
  const unsigned char *b_src = src;

  while (len >= sizeof(u64))
    {
      u64 dst8 = buf_get_he64 (b_dst);
      u64 src8 = buf_get_he64 (b_src);

      buf_put_he64 (b_dst, (dst8 & mask2) | (src8 & mask1));

      b_dst += sizeof(u64);
      b_src += sizeof(u64);
      len -= sizeof(u64);
    }

  while (len > 0)
    {
      *b_dst = (*b_dst & mask2) | (*b_src & mask1);
      b_dst++;
      b_src++;
      len--;
    }
}
