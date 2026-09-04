/* t-mldsa.c - Check the Crystals Dilithium computation by Known Answers
 * Copyright (C) 2025 g10 Code GmbH
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: LGPL-2.1+
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "stopwatch.h"

#define PGM "t-mldsa"

#define NEED_SHOW_NOTE
#define NEED_PREPEND_SRCDIR
#define NEED_READ_TEXTLINE
#define NEED_COPY_DATA
#define NEED_HEX2BUFFER
#include "t-common.h"

#define N_TESTS 9

static int custom_data_file;

/* Constants taken from cipher/pubkey-dilithium.c */
enum gcry_mldsa_algos
  {                             /* See FIPS 204, Table 1 */
    GCRY_MLDSA44,               /* Category 2 */
    GCRY_MLDSA65,               /* Category 3 */
    GCRY_MLDSA87                /* Category 5 */
  };
#define MAX_PUBKEY_LEN 2592
#define MAX_SECKEY_LEN 4896
#define MAX_SIG_LEN 4627
struct mldsa_info
{
  const char *name;            /* Name of the algo.  */
  unsigned int namelen;        /* Only here to avoid strlen calls.  */
  int algo;                    /* ML-DSA algo number.   */
  unsigned int nbits;  /* Number of bits (pubkey size in bits).  */
  unsigned int fips:1; /* True if this is a FIPS140-4??? approved.  */
  int pubkey_len;      /* Length of the public key.  */
  int seckey_len;      /* Length of the secret key.  */
  int sig_len;         /* Length of the signature.  */
};

/* Information about the ML-DSA algoithms for use by the
 * s-expression interface.  */
static const struct mldsa_info mldsa_infos[] =
  {
    { "dilithium2", 10, GCRY_MLDSA44, 1312*8, 1, 1312, 2560, 2420 },
    { "dilithium3", 10, GCRY_MLDSA65, 1952*8, 1, 1952, 4032, 3309 },
    { "dilithium5", 10, GCRY_MLDSA87, 2592*8, 1, 2592, 4896, 4627 },
    { NULL }
  };

static const struct mldsa_info *
mldsa_get_info_by_algo (int algo)
{
  const char *name;
  int i;

  for (i=0; (name=mldsa_infos[i].name); i++)
    if (mldsa_infos[i].algo == algo)
      break;
  if (!name)
    return NULL;

  return &mldsa_infos[i];
}

/*
 * The input line is like:
 *
 *      [Dilithium2]
 *      [Dilithium3]
 *      [Dilithium5]
 *
 */
static int
parse_annotation (const char *line, int lineno)
{
  const char *s;

  s = strchr (line, 'm');
  if (!s)
    {
      fail ("syntax error at input line %d", lineno);
      return 0;
    }

  switch (atoi (s+1))
    {
    case 2:
      return GCRY_MLDSA44;
      break;
    case 3:
    default:
      return GCRY_MLDSA65;
      break;
    case 5:
      return GCRY_MLDSA87;
      break;
    }
}

static void
one_genkey_test (int testno, int algo, const char *seed_str,
                 const char *sk_str, const char *pk_str)
{
  gpg_error_t err;
  const struct mldsa_info *mldsa_info;
  unsigned char *seed;
  size_t seed_len;
  const unsigned char *sk_computed;
  const unsigned char *pk_computed;
  unsigned char *pk, *sk;
  size_t pk_len, sk_len;
  gcry_sexp_t keyparm = NULL;
  gcry_sexp_t key     = NULL;
  gcry_sexp_t skey    = NULL;
  gcry_sexp_t pkey    = NULL;
  gcry_mpi_t pk_mpi = NULL;
  gcry_mpi_t sk_mpi = NULL;
  unsigned int n;

  pk = sk = seed = NULL;

  if (verbose > 1)
    info ("Running test %d\n", testno);

  mldsa_info = mldsa_get_info_by_algo (algo);
  if (!mldsa_info)
    {
      fail ("error to determine the algorithm for test %d, %d: %s",
            testno, algo, "invalid algo");
      goto leave;
    }

  if (!(seed = hex2buffer (seed_str, &seed_len)))
    {
      fail ("error preparing input for test %d, %s: %s",
            testno, "seed", "invalid hex string");
      goto leave;
    }
  if (!(pk = hex2buffer (pk_str, &pk_len)))
    {
      fail ("error preparing input for test %d, %s: %s",
            testno, "pk", "invalid hex string");
      goto leave;
    }
  if (mldsa_info->pubkey_len != pk_len)
    {
      fail ("error preparing input for test %d, %s: %s",
            testno, "pk", "length error");
      goto leave;
    }
  if (!(sk = hex2buffer (sk_str, &sk_len)))
    {
      fail ("error preparing input for test %d, %s: %s",
            testno, "sk", "invalid hex string");
      goto leave;
    }
  if (mldsa_info->seckey_len != sk_len)
    {
      fail ("error preparing input for test %d, %s: %s",
            testno, "sk", "length error");
      goto leave;
    }

  err = gcry_sexp_build (&keyparm, NULL, "(genkey(%s(S%b)))",
                         mldsa_info->name, (int)seed_len, seed, NULL);
  if (err)
    {
      fail ("error building s-exp for test %d, %s: %s",
            testno, "keyparm", gpg_strerror (err));
      goto leave;
    }

  err = gcry_pk_genkey (&key, keyparm);
  if (err)
    {
      fail ("gcry_pk_genkey failed for test %d: %s",
            testno, gpg_strerror (err));
      goto leave;
    }

  pkey = gcry_sexp_find_token (key, "public-key", 0);
  if (!pkey)
    {
      fail ("gcry_pk_genkey returns no pubkey for test %d", testno);
      goto leave;
    }

  skey = gcry_sexp_find_token (key, "private-key", 0);
  if (!skey)
    {
      fail ("gcry_pk_genkey returns no seckey for test %d", testno);
      goto leave;
    }

  /*
   * Extract the public key.
   */
  err = gcry_sexp_extract_param (pkey, NULL, "/p", &pk_mpi, NULL);
  if (err)
    goto leave;
  pk_computed = gcry_mpi_get_opaque (pk_mpi, &n);
  if (!pk_computed || mldsa_info->pubkey_len != (n + 7) / 8)
    {
      fail ("gcry_pk_genkey returns bad pubkey for test %d", testno);
      goto leave;
    }

  /*
   * Extract the secret key.
   */
  err = gcry_sexp_extract_param (skey, NULL, "/s", &sk_mpi, NULL);
  if (err)
    goto leave;
  sk_computed = gcry_mpi_get_opaque (sk_mpi, &n);
  if (!sk_computed || mldsa_info->seckey_len != (n + 7) / 8)
    {
      fail ("gcry_pk_genkey returns bad seckey for test %d", testno);
      goto leave;
    }

  if (memcmp (pk_computed, pk, pk_len) != 0)
    fail ("test %d failed: pk mismatch\n", testno);

  if (memcmp (sk_computed, sk, sk_len) != 0)
    fail ("test %d failed: sk mismatch\n", testno);

 leave:
  gcry_mpi_release (sk_mpi);
  gcry_mpi_release (pk_mpi);
  gcry_sexp_release (skey);
  gcry_sexp_release (pkey);
  gcry_sexp_release (key);
  gcry_sexp_release (keyparm);
  xfree (seed);
  xfree (pk);
  xfree (sk);
}


static void
one_siggen_test (int testno, int algo, const char *rnd_str,
                 const char *sk_str, const char *msg_str, const char *sig_str)
{
  gpg_error_t err;
  const struct mldsa_info *mldsa_info;
  unsigned char *rnd;
  size_t rnd_len;
  unsigned char *sig;
  size_t sig_len;
  unsigned char *msg;
  size_t msg_len;
  unsigned char *sk;
  size_t sk_len;
  gcry_sexp_t s_sig = NULL;
  gcry_sexp_t s_data = NULL;
  gcry_sexp_t s_skey = NULL;
  gcry_mpi_t sig_mpi = NULL;
  unsigned int n;
  const unsigned char *sig_computed;

  rnd = sig = msg = sk = NULL;

  if (verbose > 1)
    info ("Running test %d\n", testno);

  mldsa_info = mldsa_get_info_by_algo (algo);
  if (!mldsa_info)
    {
      fail ("error to determine the algorithm for test %d, %d: %s",
            testno, algo, "invalid algo");
      goto leave;
    }

  if (!(rnd = hex2buffer (rnd_str, &rnd_len)))
    {
      fail ("error preparing input for test %d, %s: %s",
            testno, "rnd", "invalid hex string");
      goto leave;
    }
  if (32 != rnd_len)
    {
      fail ("error preparing input for test %d, %s: %s",
            testno, "rnd", "length error");
      goto leave;
    }
  if (!(sk = hex2buffer (sk_str, &sk_len)))
    {
      fail ("error preparing input for test %d, %s: %s",
            testno, "sk", "invalid hex string");
      goto leave;
    }
  if (mldsa_info->seckey_len != sk_len)
    {
      fail ("error preparing input for test %d, %s: %s",
            testno, "sk", "length error");
      goto leave;
    }
  if (!(msg = hex2buffer (msg_str, &msg_len)))
    {
      fail ("error preparing input for test %d, %s: %s",
            testno, "msg", "invalid hex string");
      goto leave;
    }
  if (!(sig = hex2buffer (sig_str, &sig_len)))
    {
      fail ("error preparing input for test %d, %s: %s",
            testno, "sig", "invalid hex string");
      goto leave;
    }
  if (mldsa_info->sig_len != sig_len)
    {
      fail ("error preparing input for test %d, %s: %s",
            testno, "sig", "length error");
      goto leave;
    }

  err = gcry_sexp_build (&s_skey, NULL,
                         "(private-key(%s(s%b)))",
                         mldsa_info->name,
                         (int)sk_len, sk,
                         NULL);
  if (err)
    {
      fail ("error building s-exp for test %d, %s: %s",
            testno, "s_skey", gpg_strerror (err));
      goto leave;
    }

  err = gcry_sexp_build (&s_data, NULL,
                         "(data(raw)"
                         " (flags no-prefix)(value%b)(random-override%b))",
                         (int)msg_len, msg,
                         (int)rnd_len, rnd,
                         NULL);
  if (err)
    {
      fail ("error building s-exp for test %d, %s: %s",
            testno, "s_data", gpg_strerror (err));
      goto leave;
    }

  err = gcry_pk_sign (&s_sig, s_data, s_skey);
  if (err)
    {
      fail ("gcry_pk_sign failed for test %d: %s",
            testno, gpg_strerror (err));
      goto leave;
    }

  /*
   * Extract the signature.
   */
  err = gcry_sexp_extract_param (s_sig, NULL, "/s", &sig_mpi, NULL);
  if (err)
    goto leave;
  sig_computed = gcry_mpi_get_opaque (sig_mpi, &n);
  if (!sig_computed || mldsa_info->sig_len != (n + 7) / 8)
    {
      fail ("gcry_pk_sign returns bad signature for test %d", testno);
      goto leave;
    }

  if (memcmp (sig_computed, sig, sig_len) != 0)
    fail ("test %d failed: sig mismatch\n", testno);

 leave:
  gcry_mpi_release (sig_mpi);
  gcry_sexp_release (s_sig);
  gcry_sexp_release (s_skey);
  gcry_sexp_release (s_data);
  xfree (rnd);
  xfree (msg);
  xfree (sk);
  xfree (sig);
}


static void
one_sigver_test (int testno, int algo, const char *pk_str,
                 const char *msg_str, const char *ctx_str, const char *sig_str)
{
  gpg_error_t err;
  const struct mldsa_info *mldsa_info;
  unsigned char *ctx;
  size_t ctx_len;
  unsigned char *sig;
  size_t sig_len;
  unsigned char *msg;
  size_t msg_len;
  unsigned char *pk;
  size_t pk_len;
  gcry_sexp_t s_sig = NULL;
  gcry_sexp_t s_data = NULL;
  gcry_sexp_t s_pkey = NULL;

  sig = msg = pk = ctx = NULL;

  if (verbose > 1)
    info ("Running test %d\n", testno);

  mldsa_info = mldsa_get_info_by_algo (algo);
  if (!mldsa_info)
    {
      fail ("error to determine the algorithm for test %d, %d: %s",
            testno, algo, "invalid algo");
      goto leave;
    }

  if (!(ctx = hex2buffer (ctx_str, &ctx_len)))
    {
      fail ("error preparing input for test %d, %s: %s",
            testno, "ctx", "invalid hex string");
      goto leave;
    }
  if (255 < ctx_len)
    {
      fail ("error preparing input for test %d, %s: %s",
            testno, "ctx", "length error");
      goto leave;
    }
  if (!(pk = hex2buffer (pk_str, &pk_len)))
    {
      fail ("error preparing input for test %d, %s: %s",
            testno, "pk", "invalid hex string");
      goto leave;
    }
  if (mldsa_info->pubkey_len != pk_len)
    {
      fail ("error preparing input for test %d, %s: %s",
            testno, "pk", "length error");
      goto leave;
    }
  if (!(msg = hex2buffer (msg_str, &msg_len)))
    {
      fail ("error preparing input for test %d, %s: %s",
            testno, "msg", "invalid hex string");
      goto leave;
    }
  if (!(sig = hex2buffer (sig_str, &sig_len)))
    {
      fail ("error preparing input for test %d, %s: %s",
            testno, "sig", "invalid hex string");
      goto leave;
    }
  if (mldsa_info->sig_len != sig_len)
    {
      fail ("error preparing input for test %d, %s: %s",
            testno, "sig", "length error");
      goto leave;
    }

  err = gcry_sexp_build (&s_pkey, NULL,
                         "(public-key(%s(p%b)))",
                         mldsa_info->name,
                         (int)pk_len, pk,
                         NULL);
  if (err)
    {
      fail ("error building s-exp for test %d, %s: %s",
            testno, "s_pkey", gpg_strerror (err));
      goto leave;
    }

  err = gcry_sexp_build (&s_data, NULL,
                         "(data(raw)(value%b)(label%b))",
                         (int)msg_len, msg,
                         (int)ctx_len, ctx,
                         NULL);
  if (err)
    {
      fail ("error building s-exp for test %d, %s: %s",
            testno, "s_data", gpg_strerror (err));
      goto leave;
    }

  err = gcry_sexp_build (&s_sig, NULL,
                         "(sig-val(%s(s%b)))",
                         mldsa_info->name, (int)sig_len, sig,
                         NULL);
  if (err)
    {
      fail ("error building s-exp for test %d, %s: %s",
            testno, "s_sig", gpg_strerror (err));
      goto leave;
    }

  err = gcry_pk_verify (s_sig, s_data, s_pkey);
  if (err)
    fail ("gcry_pk_verify failed for test %d: %s", testno, gpg_strerror (err));

 leave:
  gcry_sexp_release (s_sig);
  gcry_sexp_release (s_data);
  gcry_sexp_release (s_pkey);
  xfree (ctx);
  xfree (msg);
  xfree (pk);
  xfree (sig);
}


static void
check_mldsa_kat (const char *fname)
{
  FILE *fp;
  int lineno, ntests;
  char *line;
  int testno;
  char *sk_str, *pk_str, *seed_str;
  char *rnd_str, *msg_str, *sig_str;
  char *ctx_str;
  int algo = 0;

  info ("Checking ML-DSA.\n");

  fp = fopen (fname, "r");
  if (!fp)
    die ("error opening '%s': %s\n", fname, strerror (errno));

  testno = 0;
  sk_str = pk_str = seed_str = NULL;
  rnd_str = msg_str = sig_str = NULL;
  ctx_str = NULL;
  lineno = ntests = 0;
  while ((line = read_textline (fp, &lineno)))
    {
      if (!strncmp (line, "[", 1))
        algo = parse_annotation (line, lineno);
      else if (!strncmp (line, "sk:", 3))
        copy_data (&sk_str, line, lineno);
      else if (!strncmp (line, "pk:", 3))
        copy_data (&pk_str, line, lineno);
      else if (!strncmp (line, "rnd:", 4))
        copy_data (&rnd_str, line, lineno);
      else if (!strncmp (line, "seed:", 5))
        copy_data (&seed_str, line, lineno);
      else if (!strncmp (line, "context:", 8))
        copy_data (&ctx_str, line, lineno);
      else if (!strncmp (line, "message:", 8))
        copy_data (&msg_str, line, lineno);
      else if (!strncmp (line, "signature:", 10))
        copy_data (&sig_str, line, lineno);
      else
        fail ("unknown tag at input line %d", lineno);

      xfree (line);

      if (sk_str && pk_str && seed_str)
        {
          testno++;
          one_genkey_test (testno, algo, seed_str, sk_str, pk_str);
          ntests++;
          if (!(ntests % 256))
            show_note ("%d of %d tests done\n", ntests, N_TESTS);
          xfree (sk_str);   sk_str = NULL;
          xfree (pk_str);   pk_str = NULL;
          xfree (seed_str); seed_str = NULL;
        }
      else if (rnd_str && sk_str && msg_str && sig_str)
        {
          testno++;
          one_siggen_test (testno, algo, rnd_str, sk_str, msg_str, sig_str);
          ntests++;
          if (!(ntests % 256))
            show_note ("%d of %d tests done\n", ntests, N_TESTS);
          xfree (rnd_str); rnd_str = NULL;
          xfree (sk_str);  sk_str = NULL;
          xfree (msg_str); msg_str = NULL;
          xfree (sig_str); sig_str = NULL;
        }
      else if (pk_str && msg_str && ctx_str && sig_str)
        {
          testno++;
          one_sigver_test (testno, algo, pk_str, msg_str, ctx_str, sig_str);
          ntests++;
          if (!(ntests % 256))
            show_note ("%d of %d tests done\n", ntests, N_TESTS);
          xfree (pk_str); pk_str = NULL;
          xfree (msg_str); msg_str = NULL;
          xfree (ctx_str); ctx_str = NULL;
          xfree (sig_str); sig_str = NULL;
        }
    }
  xfree (sk_str);
  xfree (pk_str);
  xfree (seed_str);
  xfree (rnd_str);
  xfree (msg_str);
  xfree (ctx_str);
  xfree (sig_str);

  if (ntests != N_TESTS && !custom_data_file)
    fail ("did %d tests but expected %d", ntests, N_TESTS);
  else if ((ntests % 256))
    show_note ("%d tests done\n", ntests);

  fclose (fp);
}


int
main (int argc, char **argv)
{
  int last_argc = -1;
  char *fname   = NULL;

  if (argc)
    { argc--; argv++; }

  while (argc && last_argc != argc)
    {
      last_argc = argc;
      if (!strcmp (*argv, "--"))
        {
          argc--; argv++;
          break;
        }
      else if (!strcmp (*argv, "--help"))
        {
          fputs ("usage: " PGM " [options]\n"
                 "Options:\n"
                 "  --verbose       print timings etc.\n"
                 "  --debug         flyswatter\n"
                 "  --data FNAME    take test data from file FNAME\n",
                 stdout);
          exit (0);
        }
      else if (!strcmp (*argv, "--verbose"))
        {
          verbose++;
          argc--; argv++;
        }
      else if (!strcmp (*argv, "--debug"))
        {
          verbose += 2;
          debug++;
          argc--; argv++;
        }
      else if (!strcmp (*argv, "--data"))
        {
          argc--; argv++;
          if (argc)
            {
              xfree (fname);
              fname = xstrdup (*argv);
              argc--; argv++;
            }
        }
      else if (!strncmp (*argv, "--", 2))
        die ("unknown option '%s'", *argv);
    }

  if (!fname)
    fname = prepend_srcdir ("t-mldsa.inp");
  else
    custom_data_file = 1;

  xgcry_control ((GCRYCTL_DISABLE_SECMEM, 0));
  if (!gcry_check_version (GCRYPT_VERSION))
    die ("version mismatch\n");
  if (debug)
    xgcry_control ((GCRYCTL_SET_DEBUG_FLAGS, 1u , 0));
  xgcry_control ((GCRYCTL_ENABLE_QUICK_RANDOM, 0));
  xgcry_control ((GCRYCTL_INITIALIZATION_FINISHED, 0));

  start_timer ();
  check_mldsa_kat (fname);
  stop_timer ();

  xfree (fname);

  info ("All tests completed in %s.  Errors: %d\n",
        elapsed_time (1), error_count);
  return !!error_count;
}
