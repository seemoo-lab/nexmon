/* t-sntrup761.c - SNTRUP761 internal arithmetic regression tests
 * Copyright (C) 2026 Jussi Kivilinna <jussi.kivilinna@iki.fi>
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

/*
 * Reference functions derived from public domain source, written
 * by (in alphabetical order):
 * - Daniel J. Bernstein
 * - Chitchanok Chuengsatiansup
 * - Tanja Lange
 * - Christine van Vredendaal
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Stub out external dependencies so that including implementation does
   not require linking them.  */
#define _gcry_md_hash_buffer   t_sntrup761_md_hash_buffer
#define _gcry_ct_not_memequal  t_sntrup761_ct_not_memequal
#define _gcry_ct_memmov_cond   t_sntrup761_ct_memmov_cond

static void
t_sntrup761_md_hash_buffer (int algo, void *digest, const void *buf,
			    size_t len);

static unsigned int
t_sntrup761_ct_not_memequal (const void *b1, const void *b2, size_t len);

static void
t_sntrup761_ct_memmov_cond (void *dst, const void *src, size_t len,
			    unsigned long op_enable);

/* Include after implementation, as 't-common.h' pulls in public gcrypt.h
   which 'sntrup761.h' refuses to see.  */
#include "../cipher/sntrup761.c"

#define PGM "t-sntrup761"
#include "t-common.h"

static void
t_sntrup761_md_hash_buffer (int algo, void *digest, const void *buf,
			    size_t len)
{
  (void)buf;
  (void)len;

  if (algo == GCRY_MD_SHA512)
    memset(digest, 0, 64);
}

static unsigned int
t_sntrup761_ct_not_memequal (const void *b1, const void *b2, size_t len)
{
  (void)b1;
  (void)b2;
  (void)len;
  return 0;
}

static void
t_sntrup761_ct_memmov_cond (void *dst, const void *src, size_t len,
			    unsigned long op_enable)
{
  (void)dst;
  (void)src;
  (void)len;
  (void)op_enable;
}

static uint32_t rng_counter;

static uint32_t
rng (void)
{
  unsigned char ctr[4], dig[20];

  ctr[0] = rng_counter;
  ctr[1] = rng_counter >> 8;
  ctr[2] = rng_counter >> 16;
  ctr[3] = rng_counter >> 24;
  rng_counter++;

  gcry_md_hash_buffer (GCRY_MD_SHA1, dig, ctr, sizeof (ctr));

  return ((uint32_t)dig[0] | ((uint32_t)dig[1] << 8)
	  | ((uint32_t)dig[2] << 16) | ((uint32_t)dig[3] << 24));
}

/* Canonical representative of X modulo M, in [0,M).  */
static long
ref_mod (long long x, long m)
{
  long long r = x % m;

  if (r < 0)
    r += m;
  return (long)r;
}

static int
ref_F3_freeze (long long x)
{
  return ref_mod (x + 1, 3) - 1;
}

static int
ref_Fq_freeze (long long x)
{
  return ref_mod (x + q12, q) - q12;
}

/* Multiplication in Z[x]/(q, x^p - x - 1).  */
static void
ref_Rq_mult_small (Fq *h, const Fq *f, const small *g)
{
  static long long fg[p + p - 1];
  int i, j;

  for (i = 0; i < p + p - 1; i++)
    fg[i] = 0;
  for (i = 0; i < p; i++)
    for (j = 0; j < p; j++)
      fg[i + j] += (long long)f[i] * g[j];
  for (i = p + p - 2; i >= p; i--)
    {
      fg[i - p] += fg[i];
      fg[i - p + 1] += fg[i];
    }
  for (i = 0; i < p; i++)
    h[i] = ref_Fq_freeze (fg[i]);
}

static void
ref_R3_mult (small *h, const small *f, const small *g)
{
  static long long fg[p + p - 1];
  int i, j;

  for (i = 0; i < p + p - 1; i++)
    fg[i] = 0;
  for (i = 0; i < p; i++)
    for (j = 0; j < p; j++)
      fg[i + j] += (long long)f[i] * g[j];
  for (i = p + p - 2; i >= p; i--)
    {
      fg[i - p] += fg[i];
      fg[i - p + 1] += fg[i];
    }
  for (i = 0; i < p; i++)
    h[i] = ref_F3_freeze (fg[i]);
}

static void
test_freeze_helpers (void)
{
  /* Coprime to q, so that sweep hits different residue every step.  */
  static const long sweep_step = 9973;
  static const long fq_extremes[] =
    {
      -2 * (long)q12 * q12, 2 * (long)q12 * q12,
      -2 * (long)q12 * q12 + 1, 2 * (long)q12 * q12 - 1,
      -(long)q12 * q12, (long)q12 * q12,
      -(long)p * q12, (long)p * q12
    };
  long x;
  unsigned int i;

  if (verbose)
    fprintf (stderr, PGM ": checking F3_freeze over all int16_t\n");
  for (x = -32768; x <= 32767; x++)
    {
      int got = F3_freeze ((int16_t)x);
      int want = ref_F3_freeze (x);

      if (got != want)
	fail ("F3_freeze(%ld): got %d, want %d", x, got, want);
    }

  if (verbose)
    fprintf (stderr, PGM ": checking Fq_freeze over reachable range\n");
  for (x = -(2 * (long)q12 + 8); x <= 2 * (long)q12 + 8; x++)
    {
      int got = Fq_freeze ((int32_t)x);
      int want = ref_Fq_freeze (x);

      if (got != want)
	fail ("Fq_freeze(%ld): got %d, want %d", x, got, want);
    }

  /* Widest values callers produce, from Rq_recip3.  */
  for (x = -2 * (long)q12 * q12; x <= 2 * (long)q12 * q12; x += sweep_step)
    {
      int got = Fq_freeze ((int32_t)x);
      int want = ref_Fq_freeze (x);

      if (got != want)
	fail ("Fq_freeze(%ld): got %d, want %d", x, got, want);
    }

  for (i = 0; i < DIM (fq_extremes); i++)
    {
      int got = Fq_freeze ((int32_t)fq_extremes[i]);
      int want = ref_Fq_freeze (fq_extremes[i]);

      if (got != want)
	fail ("Fq_freeze(%ld): got %d, want %d", fq_extremes[i], got, want);
    }
}

/* Input patterns maximize intermediate accumulators.  */
static void
make_inputs (int pattern, Fq *f, small *g)
{
  int i;

  switch (pattern)
    {
    case 0:
      for (i = 0; i < p; i++)
	{
	  f[i] = q12;
	  g[i] = 1;
	}
      break;
    case 1:
      for (i = 0; i < p; i++)
	{
	  f[i] = -q12;
	  g[i] = -1;
	}
      break;
    case 2:
      for (i = 0; i < p; i++)
	{
	  f[i] = q12;
	  g[i] = -1;
	}
      break;
    case 3:
      for (i = 0; i < p; i++)
	{
	  f[i] = q12;
	  g[i] = (i & 1) ? -1 : 1;
	}
      break;
    case 4:
      for (i = 0; i < p; i++)
	{
	  f[i] = (i & 1) ? -q12 : q12;
	  g[i] = 1;
	}
      break;
    case 5:
      for (i = 0; i < p; i++)
	{
	  f[i] = (rng () & 1) ? q12 : -q12;
	  g[i] = 1;
	}
      break;
    case 6:
      for (i = 0; i < p; i++)
	{
	  f[i] = 0;
	  g[i] = 0;
	}
      break;
    case 7:
      for (i = 0; i < p; i++)
	{
	  f[i] = 0;
	  g[i] = 0;
	}
      f[p - 1] = q12;
      g[p - 1] = -1;
      break;
    case 8:
      for (i = 0; i < p; i++)
	{
	  f[i] = q12;
	  g[i] = 0;
	}
      g[0] = 1;
      g[p - 1] = 1;
      break;
    case 9:
      for (i = 0; i < p; i++)
	{
	  f[i] = (int)(rng () % (2 * q12 + 1)) - q12;
	  g[i] = i < w ? ((rng () & 1) ? 1 : -1) : 0;
	}
      break;
    default:
      for (i = 0; i < p; i++)
	{
	  f[i] = (int)(rng () % (2 * q12 + 1)) - q12;
	  g[i] = (int)(rng () % 3) - 1;
	}
      break;
    }
}

static void
test_mult (void)
{
  static Fq f[p], h[p], h_ref[p];
  static small g[p], fs[p], h3[p], h3_ref[p];
  int pattern, i;

  for (pattern = 0; pattern < 32; pattern++)
    {
      rng_counter = pattern << 24;

      make_inputs (pattern, f, g);
      for (i = 0; i < p; i++)
	fs[i] = g[i];

      Rq_mult_small (h, f, g);
      ref_Rq_mult_small (h_ref, f, g);
      if (memcmp (h, h_ref, sizeof (h)))
	{
	  for (i = 0; i < p; i++)
	    if (h[i] != h_ref[i])
	      {
		fail ("Rq_mult_small pattern %d coeff %d: got %d, want %d",
		      pattern, i, (int)h[i], (int)h_ref[i]);
		break;
	      }
	}

      R3_mult (h3, fs, g);
      ref_R3_mult (h3_ref, fs, g);
      if (memcmp (h3, h3_ref, sizeof (h3)))
	{
	  for (i = 0; i < p; i++)
	    if (h3[i] != h3_ref[i])
	      {
		fail ("R3_mult pattern %d coeff %d: got %d, want %d",
		      pattern, i, (int)h3[i], (int)h3_ref[i]);
		break;
	      }
	}
    }
}

int
main (int argc, char **argv)
{
  int last_argc = -1;

  if (argc)
    {
      argc--;
      argv++;
    }

  while (argc && last_argc != argc)
    {
      last_argc = argc;
      if (!strcmp (*argv, "--"))
	{
	  argc--;
	  argv++;
	  break;
	}
      else if (!strcmp (*argv, "--help"))
	{
	  fputs ("usage: " PGM " [--verbose]\n", stdout);
	  exit (0);
	}
      else if (!strcmp (*argv, "--verbose"))
	{
	  verbose++;
	  argc--;
	  argv++;
	}
      else if (!strncmp (*argv, "--", 2))
	{
	  fprintf (stderr, PGM ": unknown option '%s'\n", *argv);
	  exit (1);
	}
    }

  xgcry_control ((GCRYCTL_SET_VERBOSITY, (int) verbose));

  if (!gcry_check_version (GCRYPT_VERSION))
    die ("version mismatch\n");

  xgcry_control ((GCRYCTL_DISABLE_SECMEM, 0));
  xgcry_control ((GCRYCTL_INITIALIZATION_FINISHED, 0));
  xgcry_control ((GCRYCTL_ENABLE_QUICK_RANDOM, 0));

  test_freeze_helpers ();
  test_mult ();

  if (verbose)
    fprintf (stderr, PGM ": %d error(s)\n", error_count);
  return !!error_count;
}
