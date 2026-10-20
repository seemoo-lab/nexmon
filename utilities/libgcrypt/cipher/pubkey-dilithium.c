/* pubkey-dilithium.c - the Dilithium for libgcrypt
 * Copyright (C) 2025 g10 Code GmbH
 *
 * This file was modified for use by Libgcrypt.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
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

#include "g10lib.h"
#include "mpi.h"
#include "cipher.h"
#include "pubkey-internal.h"
#include "dilithium.h"

static const char *mldsa_names[] =
  {
    "dilithium2",
    "dilithium3",
    "dilithium5",
    NULL,
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
mldsa_get_info (gcry_sexp_t keyparam)
{
  const char *algo;
  size_t algolen;
  const char *name;
  int i;

  algo = sexp_nth_data (keyparam, 0, &algolen);
  if (!algo || !algolen)
    return NULL;
  for (i=0; (name=mldsa_infos[i].name); i++)
    if (mldsa_infos[i].namelen == algolen && !memcmp (name, algo, algolen))
      break;
  if (!name)
    return NULL;

  return &mldsa_infos[i];
}

static unsigned int
mldsa_get_nbits (gcry_sexp_t keyparam)
{
  const struct mldsa_info *info = mldsa_get_info (keyparam);
  if (!info)
    return 0;  /* GPG_ERR_PUBKEY_ALGO */

  return info->nbits;
}

static gpg_err_code_t
mldsa_compute_keygrip (gcry_md_hd_t md, gcry_sexp_t keyparam)
{
  gcry_sexp_t l1;
  const char *data;
  size_t datalen;

  const struct mldsa_info *info = mldsa_get_info (keyparam);
  if (!info)
    return GPG_ERR_WRONG_PUBKEY_ALGO;

  _gcry_md_write (md, info->name, info->namelen+1); /* (also hash the nul) */

  l1 = sexp_find_token (keyparam, "p", 1);
  if (!l1)
    return GPG_ERR_NO_OBJ;

  data = sexp_nth_data (l1, 1, &datalen);
  if (!data)
    {
      sexp_release (l1);
      return GPG_ERR_NO_OBJ;
    }

  _gcry_md_write (md, data, datalen);
  sexp_release (l1);

  return 0;
}


static void
randombytes (unsigned char *out, size_t outlen,
             enum gcry_random_level level)
{
  _gcry_randomize (out, outlen, level);
}

static gcry_err_code_t
mldsa_generate (const gcry_sexp_t genparms, gcry_sexp_t *r_skey)
{
  gpg_err_code_t rc = 0;
  gcry_mpi_t seed_mpi = NULL;
  unsigned char seed[SEEDBYTES];
  unsigned char pk[MAX_PUBKEY_LEN];
  unsigned char sk[MAX_SECKEY_LEN];
  const struct mldsa_info *info = mldsa_get_info (genparms);

  if (!info)
    return GPG_ERR_PUBKEY_ALGO;

  if (info->pubkey_len > MAX_PUBKEY_LEN)
    return GPG_ERR_INTERNAL;

  if (info->seckey_len > MAX_SECKEY_LEN)
    return GPG_ERR_INTERNAL;

  /*
   * Extract the seed (if any).
   */
  rc = sexp_extract_param (genparms, NULL, "/S", &seed_mpi, NULL);
  if (rc == GPG_ERR_NOT_FOUND)
    {
      /* Long term key material. */
      randombytes (seed, SEEDBYTES, GCRY_VERY_STRONG_RANDOM);
      rc = 0;
    }
  else if (rc)
    goto leave;
  else
    {
      const unsigned char *seed_supplied;
      unsigned int n;

      seed_supplied = mpi_get_opaque (seed_mpi, &n);
      if (SEEDBYTES != (n + 7) / 8)
        {
          rc = GPG_ERR_INV_DATA;
          goto leave;
        }
      memcpy (seed, seed_supplied, SEEDBYTES);
    }

  rc = dilithium_keypair (info->algo, pk, sk, seed);
  _gcry_burn_stack (DILITHIUM_KEYPAIR_STACK_BURN);

  if (!rc)
    rc = sexp_build (r_skey,
                     NULL,
                     "(key-data"
                     " (public-key(%s(p%b)))"
                     " (private-key(%s(p%b)(s%b)(S%b))))",
                     info->name, info->pubkey_len, pk,
                     info->name, info->pubkey_len, pk, info->seckey_len, sk,
                     SEEDBYTES, seed,
                     NULL);

 leave:
  _gcry_mpi_release (seed_mpi);
  wipememory (seed, SEEDBYTES);
  wipememory (sk, info->seckey_len);
  return rc;
}


static gcry_err_code_t
mldsa_sign (gcry_sexp_t *r_sig, gcry_sexp_t s_data, gcry_sexp_t keyparms)
{
  gpg_err_code_t rc = 0;
  unsigned int n;
  struct pk_encoding_ctx ctx;
  gcry_mpi_t sk_mpi = NULL;
  gcry_mpi_t data_mpi = NULL;
  unsigned char sig[MAX_SIG_LEN];
  unsigned char rnd[RNDBYTES];
  const unsigned char *data;
  size_t data_len;
  const unsigned char *sk;
  const struct mldsa_info *info = mldsa_get_info (keyparms);

  if (!info)
    return GPG_ERR_PUBKEY_ALGO;

  if (info->sig_len > MAX_SIG_LEN)
    return GPG_ERR_INTERNAL;

  _gcry_pk_util_init_encoding_ctx (&ctx, PUBKEY_OP_SIGN, 0);

  /* Dilithium requires the byte string for its DATA.  */
  ctx.flags |= PUBKEY_FLAG_BYTE_STRING;

  /*
   * Extract the secret key.
   */
  rc = sexp_extract_param (keyparms, NULL, "/s", &sk_mpi, NULL);
  if (rc)
    goto leave;
  sk = mpi_get_opaque (sk_mpi, &n);
  if (!sk || info->seckey_len != (n + 7) / 8)
    {
      rc = GPG_ERR_BAD_SECKEY;
      goto leave;
    }

  /* Extract the data.  */
  rc = _gcry_pk_util_data_to_mpi (s_data, &data_mpi, &ctx);
  if (rc)
    goto leave;
  if (DBG_CIPHER)
    log_mpidump ("mldsa_sign    data", data_mpi);
  if (!mpi_is_opaque (data_mpi))
    {
      rc = GPG_ERR_INV_DATA;
      goto leave;
    }
  data = mpi_get_opaque (data_mpi, &n);
  data_len = (n + 7) / 8;

  if (ctx.rnd)
    {
      if (ctx.rndlen != RNDBYTES)
        {
          rc = GPG_ERR_INV_DATA;
          goto leave;
        }
      memcpy (rnd, ctx.rnd, RNDBYTES);
    }
  else
    {
      /* Per signature value, same level as ECDSA nonce. */
      randombytes (rnd, RNDBYTES, GCRY_STRONG_RANDOM);
    }
  if (ctx.flags & PUBKEY_FLAG_NO_PREFIX)
    rc = dilithium_sign (info->algo, sig, info->sig_len, data, data_len,
                         NULL, -1, sk, rnd);
  else
    rc = dilithium_sign (info->algo, sig, info->sig_len, data, data_len,
                         ctx.label, ctx.labellen, sk, rnd);
  _gcry_burn_stack (DILITHIUM_SIGN_STACK_BURN);
  if (rc)
    goto leave;

  rc = sexp_build (r_sig, NULL, "(sig-val(%s(s%b)))", info->name,
                   info->sig_len, sig);
  if (rc)
    goto leave;

leave:
  _gcry_pk_util_free_encoding_ctx (&ctx);
  _gcry_mpi_release (sk_mpi);
  _gcry_mpi_release (data_mpi);
  wipememory (rnd, RNDBYTES);
  if (DBG_CIPHER)
    log_debug ("mldsa_sign    => %s\n", gpg_strerror (rc));
  return rc;
}


static gcry_err_code_t
mldsa_verify (gcry_sexp_t s_sig, gcry_sexp_t s_data, gcry_sexp_t keyparms)
{
  gpg_err_code_t rc = 0;
  unsigned int n;
  struct pk_encoding_ctx ctx;
  gcry_mpi_t sig_mpi = NULL;
  gcry_mpi_t data_mpi = NULL;
  gcry_mpi_t pk_mpi = NULL;
  const unsigned char *sig;
  const unsigned char *data;
  size_t data_len;
  const unsigned char *pk;
  const struct mldsa_info *info = mldsa_get_info (keyparms);

  if (!info)
    return GPG_ERR_PUBKEY_ALGO;

  _gcry_pk_util_init_encoding_ctx (&ctx, PUBKEY_OP_VERIFY, 0);

  /* Dilithium requires the byte string for its DATA.  */
  ctx.flags |= PUBKEY_FLAG_BYTE_STRING;

  /*
   * Extract the public key.
   */
  rc = sexp_extract_param (keyparms, NULL, "/p", &pk_mpi, NULL);
  if (rc)
    goto leave;
  pk = mpi_get_opaque (pk_mpi, &n);
  if (!pk || info->pubkey_len != (n + 7) / 8)
    {
      rc = GPG_ERR_BAD_PUBKEY;
      goto leave;
    }

  rc = _gcry_pk_util_data_to_mpi (s_data, &data_mpi, &ctx);
  if (rc)
    goto leave;
  if (DBG_CIPHER)
    log_mpidump ("mldsa_verify  data", data_mpi);
  if (!mpi_is_opaque (data_mpi))
    {
      rc = GPG_ERR_INV_DATA;
      goto leave;
    }
  data = mpi_get_opaque (data_mpi, &n);
  data_len = (n + 7) / 8;

  /* Extract the signature.  */
  rc = sexp_extract_param (s_sig, NULL, "/s", &sig_mpi, NULL);
  if (rc)
    goto leave;
  if (DBG_CIPHER)
    log_printmpi ("mldsa_verify  sig", sig_mpi);
  sig = mpi_get_opaque (sig_mpi, &n);
  if (!sig || info->sig_len != (n + 7) / 8)
    {
      rc = GPG_ERR_BAD_SIGNATURE;
      goto leave;
    }

  if (ctx.flags & PUBKEY_FLAG_NO_PREFIX)
    rc = dilithium_verify (info->algo, sig, info->sig_len, data, data_len,
                           NULL, -1, pk);
  else
    rc = dilithium_verify (info->algo, sig, info->sig_len, data, data_len,
                           ctx.label, ctx.labellen, pk);
  _gcry_burn_stack (DILITHIUM_VERIFY_STACK_BURN);
  if (rc)
    goto leave;

leave:
  _gcry_pk_util_free_encoding_ctx (&ctx);
  _gcry_mpi_release (pk_mpi);
  _gcry_mpi_release (data_mpi);
  _gcry_mpi_release (sig_mpi);
  if (DBG_CIPHER)
    log_debug ("mldsa_verify  => %s\n", gpg_strerror (rc));
  return rc;
}

gcry_pk_spec_t _gcry_pubkey_spec_mldsa =
  {
    GCRY_PK_MLDSA, {0, 1},
    GCRY_PK_USAGE_SIGN,
    "ML-DSA", mldsa_names,
    "p",                        /* p: public */
    "sS",                       /* s: secret, S: seed */
    "",
    "s",                        /* s: signature */
    "p",                        /* p: public */
    mldsa_generate,
    NULL /* mldsa_check_secret_key */,
    NULL,
    NULL,
    mldsa_sign,
    mldsa_verify,
    mldsa_get_nbits,
    NULL, /*run_selftests*/
    mldsa_compute_keygrip
  };
