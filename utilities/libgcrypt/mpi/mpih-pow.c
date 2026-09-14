/* mpih-pow.c  -  MPI helper functions
 * Copyright (C) 2025  g10 Code GmbH
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
#include "mpi-internal.h"
#include "longlong.h"

#if BITS_PER_MPI_LIMB <= 32
#define MAX_SCRATCH_SPACE 256*2
#define MAX_WINDOW 8
#else
#define MAX_SCRATCH_SPACE 256
#define MAX_WINDOW 4
#endif

/*
 * Compute -N^(-1) mod 2^BITS_PER_MPI_LIMB
 *
 *  (1) Compute N^(-1) mod 2^BITS_PER_MPI_LIMB
 *  (2) Then, negate the value of (1)
 *
 * For computing N^(-1) mod (power of two), see:
 *
 *   Jeffrey Hurchalla
 *   An Improved Integer Modular Multiplicative Inverse (modulo 2^w)
 *   https://arxiv.org/abs/2204.04342
 *
 */
static mpi_limb_t
compute_minv (mpi_limb_t n)
{
  mpi_limb_t x, y;

  gcry_assert (n%2 == 1);

  x = (3*n)^2;
  y = 1 - n*x;
  x = x*(1 + y);
  y *= y;
  x = x*(1 + y);
  y *= y;
  x = x*(1 + y);
#if BITS_PER_MPI_LIMB == 32
  return -x;
#elif BITS_PER_MPI_LIMB == 64
  y *= y;
  x = x*(1 + y);
  return -x;
#else
# error "Please implement multiplicative inverse mod power of 2"
#endif
}


/*
 * Compute T * R^(-1) mod M (where R is represented by MINV)
 *
 * Reference:
 *   Handbook of Applied Cryptography
 *       Algorithm 14.82: Montgomery reduction
 */
static void
mont_reduc (mpi_ptr_t rp, mpi_ptr_t tp,
            mpi_ptr_t mp, mpi_size_t n, mpi_limb_t minv)
{
  mpi_size_t i;
  mpi_limb_t cy0;
  mpi_limb_t cy1 = 0;

  for (i = 0; i < n; i++)
    {
      mpi_limb_t ui = tp[i] * minv;

      cy0 = _gcry_mpih_addmul_1 (tp + i, mp, n, ui);
      cy1 += _gcry_mpih_add_1_lli (tp + n + i, n - i, cy0);
    }

  cy0 = _gcry_mpih_sub_n (rp, tp + n, mp, n);
  _gcry_mpih_set_cond (rp, tp + n, n,
                       ct_ulong_gen_mask (mpih_limb_is_not_zero (cy0))
                       & mpih_limb_is_zero (cy1));
}

/*
 * Compute X * Y * R^(-1) mod M (where R is represented by MINV)
 *
 * Reference:
 *   Handbook of Applied Cryptography
 *       Algorithm 14.86: Montgomery multiplication
 *
 * RP should have space of 2*N limbs.
 *
 */
static void
mont_mul (mpi_ptr_t rp, mpi_ptr_t xp, mpi_ptr_t yp, mpi_ptr_t mp,
          mpi_size_t n, mpi_limb_t minv, mpi_ptr_t scratch_2n)
{
  _gcry_mpih_mul_lli (scratch_2n, xp, n, yp, n);
  mont_reduc (rp, scratch_2n, mp, n, minv);
}

/* Determine the window size for computing exponentiation.  */
static int
window_size (mpi_size_t esize)
{
  int W;

#if BITS_PER_MPI_LIMB <= 32
  if (esize > 24)
    W = 5;
  else if (esize > 16)
    W = 4;
  else if (esize > 12)
    W = 3;
  else if (esize > 8)
    W = 2;
  else
    W = 1;
  return W;
#else
  if (esize > 8)
    W = 4;
  else if (esize > 6)
    W = 3;
  else if (esize > 4)
    W = 2;
  else
    W = 1;
  return W;
#endif
  return W;
}

/*
 * Compute X ^ E mod M
 *
 * where X is in BP, M is in MP.
 * The size of limbs for BP and MP are N.
 * E is in EP, size of limbs EN.
 *
 * Result will be in RP with size N.
 *
 * Reference:
 *   Handbook of Applied Cryptography
 *       Algorithm 14.82: Left-to-right k-ary exponentiation
 *       Algorithm 14.94: Montgomery exponentiation
 *
 */
void
_gcry_mpih_powm_lli (mpi_ptr_t rp, mpi_ptr_t bp, mpi_ptr_t mp, mpi_size_t n,
                     mpi_ptr_t ep, mpi_size_t en)
{
  mpi_size_t scratch_size;
#define temp0   (scratch)
#define temp2   (scratch+n*2)
#define a       (scratch+n*4)
#define precomp (scratch+n*5)
  mpi_ptr_t scratch;
  mpi_limb_t minv;
  mpi_size_t i;
  int mod_shift_cnt;
  int windowsize = window_size (en);
  mpi_limb_t wmask = (((mpi_limb_t) 1 << windowsize) - 1);
#define temp1   (precomp+n)
#define x_tilde (precomp+n)

  scratch_size = (5 + (1 << windowsize))*n;
  scratch = _gcry_mpi_alloc_limb_space (scratch_size, 1);

  minv = compute_minv (mp[0]);

  gcry_assert (mp[0]*(-minv) == 1);

  MPN_ZERO (temp0, n);

  /* TEMP0 := R mod m */
  count_leading_zeros (mod_shift_cnt, mp[n-1]);
  if (mod_shift_cnt)
    {
      _gcry_mpih_lshift (temp2, mp, n, mod_shift_cnt);
      temp0[n] = (mpi_limb_t)1 << mod_shift_cnt;
    }
  else
    {
      MPN_COPY (temp2, mp, n);
      temp0[n] = 1;
    }
  _gcry_mpih_divrem (temp1, 0, temp0, n+1, temp2, n);
  if (mod_shift_cnt)
    _gcry_mpih_rshift (temp0, temp0, n, mod_shift_cnt);
  /* PRECOMP[0] := R mod m */
  MPN_COPY (precomp, temp0, n);

  /* TEMP0 := (R mod m)^2 */
  _gcry_mpih_sqr_n_basecase (temp0, precomp, n);

  /* TEMP0 := R^2 mod m */
  if (mod_shift_cnt)
    _gcry_mpih_lshift (temp0, temp0, n*2, mod_shift_cnt);
  _gcry_mpih_divrem (temp1, 0, temp0, n*2, temp2, n);
  if (mod_shift_cnt)
    _gcry_mpih_rshift (temp0, temp0, n, mod_shift_cnt);
  /* x~ := Mont(x, R^2 mod m) */
  mont_mul (x_tilde, bp, temp0, mp, n, minv, temp2);

  /* PRECOMP[i] := x~ ^ i */
  for (i = 0; i < (1 << windowsize) - 2; i += 2)
    {
      _gcry_mpih_sqr_n_basecase (temp0, precomp+n*(i/2+1), n);
      mont_reduc (precomp+n*(i+2), temp0, mp, n, minv);
      mont_mul (precomp+n*(i+3), x_tilde, precomp+n*(i+2), mp, n, minv, temp2);
    }

  MPN_COPY (a, precomp, n);
  i = en * BITS_PER_MPI_LIMB;
  do
    {
      mpi_limb_t e;
      int w;

      if (i < windowsize)
        {
          e = ep[0] & (((mpi_limb_t) 1 << i) - 1);
          w = i;
          i = 0;
        }
      else
        {
          mpi_limb_t v;
          mpi_size_t shift;
          mpi_size_t j;
          int nbits_in_v;

          i -= windowsize;
          j = i / BITS_PER_MPI_LIMB;
          shift = i % BITS_PER_MPI_LIMB;
          v = ep[j] >> shift;
          nbits_in_v = BITS_PER_MPI_LIMB - shift;
          if (nbits_in_v < windowsize)
            v += ep[j + 1] << nbits_in_v;
          e = v & wmask;
          w = windowsize;
        }

      do
        {
          _gcry_mpih_sqr_n_basecase (temp0, a, n);
          mont_reduc (a, temp0, mp, n, minv);
        }
      while (--w);

      _gcry_mpih_lookup_lli (temp0, precomp, n, (1 << windowsize), e);
      mont_mul (a, a, temp0, mp, n, minv, temp2);
    }
  while (i);

  MPN_ZERO (temp0, n);
  temp0[0] = 1;
  mont_mul (a, a, temp0, mp, n, minv, temp2);

  MPN_COPY (rp, a, n);
  _gcry_mpi_free_limb_space (scratch, scratch_size);
}
