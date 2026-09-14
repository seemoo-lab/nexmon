/* dilithium.c - the Dilithium (main part)
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
 * You can also use this file under the same licence of original code.
 * SPDX-License-Identifier: CC0 OR Apache-2.0
 *
 */
/*
  Original code from:

  Repository: https://github.com/pq-crystals/dilithium.git
  Branch: master
  Commit: 444cdcc84eb36b66fe27b3a2529ee48f6d8150c2

  Licence:
  Public Domain (https://creativecommons.org/share-your-work/public-domain/cc0/);
  or Apache 2.0 License (https://www.apache.org/licenses/LICENSE-2.0.html).

  Authors:
        Léo Ducas
        Eike Kiltz
        Tancrède Lepoint
        Vadim Lyubashevsky
        Gregor Seiler
        Peter Schwabe
        Damien Stehlé

  Dilithium Home: https://github.com/pq-crystals/dilithium.git
 */
/*
 * This implementation consists of four files: dilithium.h (header),
 * dilithium.c (this), dilithium-common.c (common part), and
 * dilithium-dep.c (DILITHIUM_MODE dependent part).
 *
 * It is for inclusion in libgcrypt library.  Also, standalone use of
 * the implementation is possible.  With DILITHIUM_MODE defined, it
 * can offer the variant of that DILITHIUM_MODE specified.  Otherwise,
 * three variants are offered.
 *
 * From original code, following modification was made.
 *
 * - C++ style comments are changed to C-style.
 *
 * - No use of DILITHIUM_NAMESPACE and FIPS202_NAMESPACE.  Don't export
 *   internal symbols.
 *
 * - Different external API for shake128 and shake256, having _close.
 *
 * - Add crypto_sign_keypair_internal function.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef _GCRYPT_IN_LIBGCRYPT
#include <stdarg.h>
#include <gpg-error.h>

#include "types.h"
#include "g10lib.h"
#include "gcrypt-int.h"
#include "const-time.h"

/* With glue code, we only use the "_internal" API of Dilithium.  */
#define DILITHIUM_INTERNAL_API_ONLY 1

#include "dilithium.h"

static int crypto_sign_keypair_internal_2 (uint8_t *pk, uint8_t *sk,
                                           const uint8_t seed[SEEDBYTES]);
static int crypto_sign_keypair_internal_3 (uint8_t *pk, uint8_t *sk,
                                           const uint8_t seed[SEEDBYTES]);
static int crypto_sign_keypair_internal_5 (uint8_t *pk, uint8_t *sk,
                                           const uint8_t seed[SEEDBYTES]);
static int crypto_sign_signature_internal_2 (uint8_t *sig, size_t *siglen,
                                             const uint8_t *m, size_t mlen,
                                             const uint8_t *pre, size_t prelen,
                                             const uint8_t rnd[RNDBYTES],
                                             const uint8_t *sk);
static int crypto_sign_signature_internal_3 (uint8_t *sig, size_t *siglen,
                                             const uint8_t *m, size_t mlen,
                                             const uint8_t *pre, size_t prelen,
                                             const uint8_t rnd[RNDBYTES],
                                             const uint8_t *sk);
static int crypto_sign_signature_internal_5 (uint8_t *sig, size_t *siglen,
                                             const uint8_t *m, size_t mlen,
                                             const uint8_t *pre, size_t prelen,
                                             const uint8_t rnd[RNDBYTES],
                                             const uint8_t *sk);
static int crypto_sign_verify_internal_2 (const uint8_t *sig, size_t siglen,
                                          const uint8_t *m, size_t mlen,
                                          const uint8_t *pre, size_t prelen,
                                          const uint8_t *pk);
static int crypto_sign_verify_internal_3 (const uint8_t *sig, size_t siglen,
                                          const uint8_t *m, size_t mlen,
                                          const uint8_t *pre, size_t prelen,
                                          const uint8_t *pk);
static int crypto_sign_verify_internal_5 (const uint8_t *sig, size_t siglen,
                                          const uint8_t *m, size_t mlen,
                                          const uint8_t *pre, size_t prelen,
                                          const uint8_t *pk);

gpg_err_code_t
dilithium_keypair (int algo, uint8_t *pk, uint8_t *sk,
                   const uint8_t seed[SEEDBYTES])
{
  int r;

  switch (algo)
    {
    case GCRY_MLDSA44:
      r = crypto_sign_keypair_internal_2 (pk, sk, seed);
      break;
    case GCRY_MLDSA65:
    default:
      r = crypto_sign_keypair_internal_3 (pk, sk, seed);
      break;
    case GCRY_MLDSA87:
      r = crypto_sign_keypair_internal_5 (pk, sk, seed);
      break;
    }

  if (r < 0)
    return GPG_ERR_INTERNAL;

  return 0;
}

gpg_err_code_t
dilithium_sign (int algo, uint8_t *sig, size_t siglen,
                const uint8_t *m, size_t mlen,
                const uint8_t *ctx, size_t ctxlen,
                const uint8_t *sk, const uint8_t rnd[RNDBYTES])
{
  size_t i;
  uint8_t pre[257];
  size_t prelen;
  int r;

  if (ctx == NULL)
    {
      if (ctxlen == -1)
        prelen = 0;
      else
        return GPG_ERR_INV_DATA;
    }
  else if (ctxlen > 255)
    return GPG_ERR_INV_DATA;
  else
    {
      /* Prepare pre = (0, ctxlen, ctx) */
      pre[0] = 0;
      pre[1] = ctxlen;
      for(i = 0; i < ctxlen; i++)
        pre[2 + i] = ctx[i];
      prelen = 2 + ctxlen;
    }

  /*
   * Note that the second argument of the upstream routine is the
   * pointer to output length of signature.  It assumes the first
   * argument (pointer to output signature) should have correct (or
   * more) length, beforehand.
   *
   * Before calling the routine, we should check the length.
   */
  switch (algo)
    {
    case GCRY_MLDSA44:
      if (siglen != CRYPTO_BYTES_2)
        return GPG_ERR_INV_DATA;
      r = crypto_sign_signature_internal_2 (sig, &siglen, m, mlen,
                                            pre, prelen, rnd, sk);
      break;
    case GCRY_MLDSA65:
    default:
      if (siglen != CRYPTO_BYTES_3)
        return GPG_ERR_INV_DATA;
      r = crypto_sign_signature_internal_3 (sig, &siglen, m, mlen,
                                            pre, prelen, rnd, sk);
      break;
    case GCRY_MLDSA87:
      if (siglen != CRYPTO_BYTES_5)
        return GPG_ERR_INV_DATA;
      r = crypto_sign_signature_internal_5 (sig, &siglen, m, mlen,
                                            pre, prelen, rnd, sk);
      break;
    }

  if (r < 0)
    return GPG_ERR_INTERNAL;

  return 0;
}

gpg_err_code_t
dilithium_verify (int algo, const uint8_t *sig, size_t siglen,
                  const uint8_t *m, size_t mlen,
                  const uint8_t *ctx, size_t ctxlen,
                  const uint8_t *pk)
{
  size_t i;
  uint8_t pre[257];
  size_t prelen;
  int r;

  if (ctx == NULL)
    {
      if (ctxlen == -1)
        prelen = 0;
      else
        return GPG_ERR_INV_DATA;
    }
  else if (ctxlen > 255)
    return GPG_ERR_INV_DATA;
  else
    {
      /* Prepare pre = (0, ctxlen, ctx) */
      pre[0] = 0;
      pre[1] = ctxlen;
      for(i = 0; i < ctxlen; i++)
        pre[2 + i] = ctx[i];
      prelen = 2 + ctxlen;
    }

  switch (algo)
    {
    case GCRY_MLDSA44:
      r = crypto_sign_verify_internal_2 (sig, siglen, m, mlen,
                                         pre, prelen, pk);
      break;
    case GCRY_MLDSA65:
    default:
      r = crypto_sign_verify_internal_3 (sig, siglen, m, mlen,
                                         pre, prelen, pk);
      break;
    case GCRY_MLDSA87:
      r = crypto_sign_verify_internal_5 (sig, siglen, m, mlen,
                                         pre, prelen, pk);
      break;
    }

  if (r < 0)
    return GPG_ERR_BAD_SIGNATURE;

  return 0;
}

typedef struct {
  gcry_md_hd_t h;
} keccak_state;

static void
shake128_init (keccak_state *state)
{
  gcry_err_code_t ec;

  ec = _gcry_md_open_internal (&state->h, GCRY_MD_SHAKE128, 0, 0);
  if (ec)
    log_fatal ("internal md_open failed: %d\n", ec);
}

static void
shake128_absorb (keccak_state *state, const uint8_t *in, size_t inlen)
{
  _gcry_md_write (state->h, in, inlen);
}

static void
shake128_finalize (keccak_state *state)
{
  (void)state;
}

static void
shake128_squeeze (uint8_t *out, size_t outlen, keccak_state *state)
{
  _gcry_md_extract (state->h, GCRY_MD_SHAKE128, out, outlen);
}

static void
shake128_close (keccak_state *state)
{
  _gcry_md_close (state->h);
}

static void
shake256_init (keccak_state *state)
{
  gcry_err_code_t ec;

  ec = _gcry_md_open_internal (&state->h, GCRY_MD_SHAKE256, 0, 0);
  if (ec)
    log_fatal ("internal md_open failed: %d\n", ec);
}

static void
shake256_absorb (keccak_state *state, const uint8_t *in, size_t inlen)
{
  _gcry_md_write (state->h, in, inlen);
}

static void
shake256_finalize (keccak_state *state)
{
  (void)state;
}

static void
shake256_squeeze (uint8_t *out, size_t outlen, keccak_state *state)
{
  _gcry_md_extract (state->h, GCRY_MD_SHAKE256, out, outlen);
}

static void
shake256_close (keccak_state *state)
{
  _gcry_md_close (state->h);
}

static void
shake256 (uint8_t *out, size_t outlen, const uint8_t *in,
	  size_t inlen)
{
  gcry_buffer_t iov[1];

  iov[0].size = 0;
  iov[0].data = (uint8_t *)in;
  iov[0].off = 0;
  iov[0].len = inlen;

  _gcry_md_hash_buffers_extract (GCRY_MD_SHAKE256, 0, out, outlen,
                                 iov, 1);
}
#else
#include "dilithium.h"

#define DILITHIUM_RANDOMIZED_SIGNING

/*************** dilithium/ref/randombytes.h */
void randombytes (uint8_t *out, size_t outlen);

/*************** dilithium/ref/fips202.h */
typedef struct {
  uint64_t s[25];
  unsigned int pos;
} keccak_state;

void shake128_init(keccak_state *state);
void shake128_absorb(keccak_state *state, const uint8_t *in, size_t inlen);
void shake128_finalize(keccak_state *state);
void shake128_squeeze(uint8_t *out, size_t outlen, keccak_state *state);

void shake256_init(keccak_state *state);
void shake256_absorb(keccak_state *state, const uint8_t *in, size_t inlen);
void shake256_finalize(keccak_state *state);
void shake256_squeeze(uint8_t *out, size_t outlen, keccak_state *state);
void shake256(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen);
#endif /* _GCRYPT_IN_LIBGCRYPT */

/*************** dilithium/ref/fips202.h */
#define SHAKE128_RATE 168
#define SHAKE256_RATE 136
#define SHA3_256_RATE 136

/*************** dilithium/ref/params.h */
#define CRHBYTES 64
#define TRBYTES 64
#define N 256
#define Q 8380417
#define D 13
#define ROOT_OF_UNITY 1753

#define POLYT1_PACKEDBYTES  320
#define POLYT0_PACKEDBYTES  416
#define POLYVECH_PACKEDBYTES (OMEGA + K)

/* DILITHIUM_MODE dependent values */
#define ETA2 2
#define ETA4 4
#define GAMMA1_17 (1 << 17)
#define GAMMA1_19 (1 << 19)
#define GAMMA2_32 ((Q-1)/32)
#define GAMMA2_88 ((Q-1)/88)
#define POLYZ_PACKEDBYTES_17   576
#define POLYZ_PACKEDBYTES_19   640
#define POLYW1_PACKEDBYTES_88  192
#define POLYW1_PACKEDBYTES_32  128
#define POLYETA_PACKEDBYTES_2  96
#define POLYETA_PACKEDBYTES_4 128

/*************** dilithium/ref/poly.h */
typedef struct {
  int32_t coeffs[N];
} poly;

static void poly_reduce(poly *a);
static void poly_caddq(poly *a);

static void poly_add(poly *c, const poly *a, const poly *b);
static void poly_sub(poly *c, const poly *a, const poly *b);
static void poly_shiftl(poly *a);

static void poly_ntt(poly *a);
static void poly_invntt_tomont(poly *a);
static void poly_pointwise_montgomery(poly *c, const poly *a, const poly *b);

static void poly_power2round(poly *a1, poly *a0, const poly *a);

static int poly_chknorm(const poly *a, int32_t B);
static void poly_uniform(poly *a,
                         const uint8_t seed[SEEDBYTES],
                         uint16_t nonce);

static void polyt1_pack(uint8_t *r, const poly *a);
static void polyt1_unpack(poly *r, const uint8_t *a);

static void polyt0_pack(uint8_t *r, const poly *a);
static void polyt0_unpack(poly *r, const uint8_t *a);

/*************** dilithium/ref/reduce.h */
#define MONT -4186625 /* 2^32 % Q */
#define QINV 58728449 /* q^(-1) mod 2^32 */

static int32_t montgomery_reduce(int64_t a);

static int32_t reduce32(int32_t a);

static int32_t caddq(int32_t a);

/*************** dilithium/ref/rounding.h */
static int32_t power2round(int32_t *a0, int32_t a);

/*************** dilithium/ref/symmetric.h */
typedef keccak_state stream128_state;
typedef keccak_state stream256_state;

static void dilithium_shake128_stream_init(keccak_state *state,
                                           const uint8_t seed[SEEDBYTES],
                                           uint16_t nonce);

static void dilithium_shake256_stream_init(keccak_state *state,
                                           const uint8_t seed[CRHBYTES],
                                           uint16_t nonce);

#define STREAM128_BLOCKBYTES SHAKE128_RATE
#define STREAM256_BLOCKBYTES SHAKE256_RATE

#define stream128_init(STATE, SEED, NONCE) \
        dilithium_shake128_stream_init(STATE, SEED, NONCE)
#define stream128_squeezeblocks(OUT, OUTBLOCKS, STATE) \
        shake128_squeezeblocks(OUT, OUTBLOCKS, STATE)
#define stream256_init(STATE, SEED, NONCE) \
        dilithium_shake256_stream_init(STATE, SEED, NONCE)
#define stream256_squeezeblocks(OUT, OUTBLOCKS, STATE) \
        shake256_squeezeblocks(OUT, OUTBLOCKS, STATE)

/*************** dilithium/ref/symmetric-shake.c */

void dilithium_shake128_stream_init(keccak_state *state, const uint8_t seed[SEEDBYTES], uint16_t nonce)
{
  uint8_t t[2];
  t[0] = nonce;
  t[1] = nonce >> 8;

  shake128_init(state);
  shake128_absorb(state, seed, SEEDBYTES);
  shake128_absorb(state, t, 2);
  shake128_finalize(state);
}

void dilithium_shake256_stream_init(keccak_state *state, const uint8_t seed[CRHBYTES], uint16_t nonce)
{
  uint8_t t[2];
  t[0] = nonce;
  t[1] = nonce >> 8;

  shake256_init(state);
  shake256_absorb(state, seed, CRHBYTES);
  shake256_absorb(state, t, 2);
  shake256_finalize(state);
}

/* Glue code */
#define shake128_squeezeblocks(OUT, OUTBLOCKS, STATE) \
	shake128_squeeze(OUT, SHAKE128_RATE*OUTBLOCKS, STATE)
#define stream128_close(STATE) shake128_close(STATE)
#define shake256_squeezeblocks(OUT, OUTBLOCKS, STATE) \
	shake256_squeeze(OUT, SHAKE256_RATE*OUTBLOCKS, STATE)
#define stream256_close(STATE) shake256_close(STATE)
#ifndef _GCRYPT_IN_LIBGCRYPT
static void shake256_close (keccak_state *state) { (void)state; }
static void shake128_close (keccak_state *state) { (void)state; }
#endif

#include "dilithium-common.c"

#ifdef DILITHIUM_MODE

# if DILITHIUM_MODE == 2
# define CRYPTO_ALGNAME "Dilithium2"
# define K 4
# define L 4
# define ETA 2
# define TAU 39
# define BETA 78
# define GAMMA1 (1 << 17)
# define GAMMA2 ((Q-1)/88)
# define OMEGA 80
# define CTILDEBYTES 32

# define poly_decompose poly_decompose_88
# define poly_make_hint poly_make_hint_88
# define poly_use_hint poly_use_hint_88
# define poly_uniform_eta poly_uniform_eta_2
# define poly_uniform_gamma1 poly_uniform_gamma1_17
# define polyz_pack polyz_pack_17
# define polyz_unpack polyz_unpack_17
# define polyeta_pack polyeta_pack_2
# define polyeta_unpack polyeta_unpack_2
# define polyw1_pack polyw1_pack_88

# elif DILITHIUM_MODE == 3
# define CRYPTO_ALGNAME "Dilithium3"
# define K 6
# define L 5
# define ETA 4
# define TAU 49
# define BETA 196
# define GAMMA1 (1 << 19)
# define GAMMA2 ((Q-1)/32)
# define OMEGA 55
# define CTILDEBYTES 48

# define poly_decompose poly_decompose_32
# define poly_make_hint poly_make_hint_32
# define poly_use_hint poly_use_hint_32
# define poly_uniform_eta poly_uniform_eta_4
# define poly_uniform_gamma1 poly_uniform_gamma1_19
# define polyz_pack polyz_pack_19
# define polyz_unpack polyz_unpack_19
# define polyeta_pack polyeta_pack_4
# define polyeta_unpack polyeta_unpack_4
# define polyw1_pack polyw1_pack_32

# elif DILITHIUM_MODE == 5
# define CRYPTO_ALGNAME "Dilithium5"
# define K 8
# define L 7
# define ETA 2
# define TAU 60
# define BETA 120
# define GAMMA1 (1 << 19)
# define GAMMA2 ((Q-1)/32)
# define OMEGA 75
# define CTILDEBYTES 64

# define poly_decompose poly_decompose_32
# define poly_make_hint poly_make_hint_32
# define poly_use_hint poly_use_hint_32
# define poly_uniform_eta poly_uniform_eta_2
# define poly_uniform_gamma1 poly_uniform_gamma1_19
# define polyz_pack polyz_pack_19
# define polyz_unpack polyz_unpack_19
# define polyeta_pack polyeta_pack_2
# define polyeta_unpack polyeta_unpack_2
# define polyw1_pack polyw1_pack_32

# endif

# if !defined(DILITHIUM_MODE) || DILITHIUM_MODE == 2
# define POLYZ_PACKEDBYTES   POLYZ_PACKEDBYTES_17
# endif
# if !defined(DILITHIUM_MODE) || DILITHIUM_MODE == 3 || DILITHIUM_MODE == 5
# define POLYZ_PACKEDBYTES   POLYZ_PACKEDBYTES_19
# endif

# if !defined(DILITHIUM_MODE) || DILITHIUM_MODE == 2
# define POLYW1_PACKEDBYTES  POLYW1_PACKEDBYTES_88
# endif
# if !defined(DILITHIUM_MODE) || DILITHIUM_MODE == 3 || DILITHIUM_MODE == 5
# define POLYW1_PACKEDBYTES  POLYW1_PACKEDBYTES_32
# endif

# if !defined(DILITHIUM_MODE) || DILITHIUM_MODE == 2 || DILITHIUM_MODE == 5
# define POLYETA_PACKEDBYTES POLYETA_PACKEDBYTES_2
# endif
# if !defined(DILITHIUM_MODE) || DILITHIUM_MODE == 3
# define POLYETA_PACKEDBYTES POLYETA_PACKEDBYTES_4
# endif

# include "dilithium-dep.c"
#else

# define CRYPTO_ALGNAME "Dilithium"

# define VARIANT2(name) name ## _2
# define VARIANT3(name) name ## _3
# define VARIANT5(name) name ## _5

# define DILITHIUM_MODE 2

# define CRYPTO_PUBLICKEYBYTES CRYPTO_PUBLICKEYBYTES_2
# define CRYPTO_SECRETKEYBYTES CRYPTO_SECRETKEYBYTES_2
# define CRYPTO_BYTES CRYPTO_BYTES_2
# define POLYZ_PACKEDBYTES POLYZ_PACKEDBYTES_17
# define POLYW1_PACKEDBYTES POLYW1_PACKEDBYTES_88
# define POLYETA_PACKEDBYTES POLYETA_PACKEDBYTES_2

# define K 4
# define L 4
# define ETA 2
# define TAU 39
# define BETA 78
# define GAMMA1 (1 << 17)
# define GAMMA2 ((Q-1)/88)
# define OMEGA 80
# define CTILDEBYTES 32

# define poly_decompose poly_decompose_88
# define poly_make_hint poly_make_hint_88
# define poly_use_hint poly_use_hint_88
# define poly_uniform_eta poly_uniform_eta_2
# define poly_uniform_gamma1 poly_uniform_gamma1_17
# define polyz_pack polyz_pack_17
# define polyz_unpack polyz_unpack_17
# define polyeta_pack polyeta_pack_2
# define polyeta_unpack polyeta_unpack_2
# define polyw1_pack polyw1_pack_88

# define polyvecl VARIANT2(polyvecl)
# define polyveck VARIANT2(polyveck)
# define pack_pk VARIANT2(pack_pk)
# define unpack_pk VARIANT2(unpack_pk)
# define pack_sk VARIANT2(pack_sk)
# define unpack_sk VARIANT2(unpack_sk)
# define pack_sig VARIANT2(pack_sig)
# define unpack_sig VARIANT2(unpack_sig)
# define poly_challenge VARIANT2(poly_challenge)
# define polyvec_matrix_expand VARIANT2(polyvec_matrix_expand)
# define polyvec_matrix_pointwise_montgomery VARIANT2(polyvec_matrix_pointwise_montgomery)
# define polyveck_power2round VARIANT2(polyvec_power2round)
# define polyveck_make_hint VARIANT2(polyvec_make_hint)
# define polyveck_use_hint VARIANT2(polyvec_use_hint)
# define polyvecl_uniform_eta VARIANT2(polyvecl_uniform_eta)
# define polyvecl_uniform_gamma1 VARIANT2(polyvecl_uniform_gamma1)
# define polyvecl_reduce VARIANT2(polyvecl_reduce)
# define polyvecl_add VARIANT2(polyvecl_add)
# define polyvecl_ntt VARIANT2(polyvecl_ntt)
# define polyvecl_invntt_tomont VARIANT2(polyvecl_invntt_tomont)
# define polyvecl_pointwise_poly_montgomery VARIANT2(polyvecl_pointwise_poly_montgomery)
# define polyvecl_pointwise_acc_montgomery VARIANT2(polyvecl_pointwise_acc_montgomery)
# define polyvecl_chknorm VARIANT2(polyvecl_chknorm)
# define polyveck_uniform_eta VARIANT2(polyveck_uniform_eta)
# define polyveck_reduce VARIANT2(polyveck_reduce)
# define polyveck_caddq VARIANT2(polyveck_caddq)
# define polyveck_add VARIANT2(polyveck_add)
# define polyveck_sub VARIANT2(polyveck_sub)
# define polyveck_shiftl VARIANT2(polyveck_shiftl)
# define polyveck_ntt VARIANT2(polyveck_ntt)
# define polyveck_invntt_tomont VARIANT2(polyveck_invntt_tomont)
# define polyveck_pointwise_poly_montgomery VARIANT2(polyveck_pointwise_poly_montgomery)
# define polyveck_chknorm VARIANT2(polyveck_chknorm)
# define polyveck_pack_w1 VARIANT2(polyveck_pack_w1)
# define polyveck_decompose VARIANT2(polyveck_decompose)
# define crypto_sign_keypair VARIANT2(crypto_sign_keypair)
# define crypto_sign_keypair_internal VARIANT2(crypto_sign_keypair_internal)
# define crypto_sign_signature_internal VARIANT2(crypto_sign_signature_internal)
# define crypto_sign_signature VARIANT2(crypto_sign_signature)
# define crypto_sign VARIANT2(crypto_sign)
# define crypto_sign_verify_internal VARIANT2(crypto_sign_verify_internal)
# define crypto_sign_verify VARIANT2(crypto_sign_verify)
# define crypto_sign_open VARIANT2(crypto_sign_open)

# include "dilithium-dep.c"

# define DILITHIUM_MODE 3

# define CRYPTO_PUBLICKEYBYTES CRYPTO_PUBLICKEYBYTES_3
# define CRYPTO_SECRETKEYBYTES CRYPTO_SECRETKEYBYTES_3
# define CRYPTO_BYTES CRYPTO_BYTES_3
# define POLYZ_PACKEDBYTES POLYZ_PACKEDBYTES_19
# define POLYW1_PACKEDBYTES POLYW1_PACKEDBYTES_32
# define POLYETA_PACKEDBYTES POLYETA_PACKEDBYTES_4

# define K 6
# define L 5
# define ETA 4
# define TAU 49
# define BETA 196
# define GAMMA1 (1 << 19)
# define GAMMA2 ((Q-1)/32)
# define OMEGA 55
# define CTILDEBYTES 48

# define poly_decompose poly_decompose_32
# define poly_make_hint poly_make_hint_32
# define poly_use_hint poly_use_hint_32
# define poly_uniform_eta poly_uniform_eta_4
# define poly_uniform_gamma1 poly_uniform_gamma1_19
# define polyz_pack polyz_pack_19
# define polyz_unpack polyz_unpack_19
# define polyeta_pack polyeta_pack_4
# define polyeta_unpack polyeta_unpack_4
# define polyw1_pack polyw1_pack_32

# define polyvecl VARIANT3(polyvecl)
# define polyveck VARIANT3(polyveck)
# define pack_pk VARIANT3(pack_pk)
# define unpack_pk VARIANT3(unpack_pk)
# define pack_sk VARIANT3(pack_sk)
# define unpack_sk VARIANT3(unpack_sk)
# define pack_sig VARIANT3(pack_sig)
# define unpack_sig VARIANT3(unpack_sig)
# define poly_challenge VARIANT3(poly_challenge)
# define polyvec_matrix_expand VARIANT3(polyvec_matrix_expand)
# define polyvec_matrix_pointwise_montgomery VARIANT3(polyvec_matrix_pointwise_montgomery)
# define polyveck_power2round VARIANT3(polyvec_power2round)
# define polyveck_make_hint VARIANT3(polyvec_make_hint)
# define polyveck_use_hint VARIANT3(polyvec_use_hint)
# define polyvecl_uniform_eta VARIANT3(polyvecl_uniform_eta)
# define polyvecl_uniform_gamma1 VARIANT3(polyvecl_uniform_gamma1)
# define polyvecl_reduce VARIANT3(polyvecl_reduce)
# define polyvecl_add VARIANT3(polyvecl_add)
# define polyvecl_ntt VARIANT3(polyvecl_ntt)
# define polyvecl_invntt_tomont VARIANT3(polyvecl_invntt_tomont)
# define polyvecl_pointwise_poly_montgomery VARIANT3(polyvecl_pointwise_poly_montgomery)
# define polyvecl_pointwise_acc_montgomery VARIANT3(polyvecl_pointwise_acc_montgomery)
# define polyvecl_chknorm VARIANT3(polyvecl_chknorm)
# define polyveck_uniform_eta VARIANT3(polyveck_uniform_eta)
# define polyveck_reduce VARIANT3(polyveck_reduce)
# define polyveck_caddq VARIANT3(polyveck_caddq)
# define polyveck_add VARIANT3(polyveck_add)
# define polyveck_sub VARIANT3(polyveck_sub)
# define polyveck_shiftl VARIANT3(polyveck_shiftl)
# define polyveck_ntt VARIANT3(polyveck_ntt)
# define polyveck_invntt_tomont VARIANT3(polyveck_invntt_tomont)
# define polyveck_pointwise_poly_montgomery VARIANT3(polyveck_pointwise_poly_montgomery)
# define polyveck_chknorm VARIANT3(polyveck_chknorm)
# define polyveck_pack_w1 VARIANT3(polyveck_pack_w1)
# define polyveck_decompose VARIANT3(polyveck_decompose)
# define crypto_sign_keypair VARIANT3(crypto_sign_keypair)
# define crypto_sign_keypair_internal VARIANT3(crypto_sign_keypair_internal)
# define crypto_sign_signature_internal VARIANT3(crypto_sign_signature_internal)
# define crypto_sign_signature VARIANT3(crypto_sign_signature)
# define crypto_sign VARIANT3(crypto_sign)
# define crypto_sign_verify_internal VARIANT3(crypto_sign_verify_internal)
# define crypto_sign_verify VARIANT3(crypto_sign_verify)
# define crypto_sign_open VARIANT3(crypto_sign_open)

# include "dilithium-dep.c"

# define DILITHIUM_MODE 5

# define CRYPTO_PUBLICKEYBYTES CRYPTO_PUBLICKEYBYTES_5
# define CRYPTO_SECRETKEYBYTES CRYPTO_SECRETKEYBYTES_5
# define CRYPTO_BYTES CRYPTO_BYTES_5
# define POLYZ_PACKEDBYTES POLYZ_PACKEDBYTES_19
# define POLYW1_PACKEDBYTES POLYW1_PACKEDBYTES_32
# define POLYETA_PACKEDBYTES POLYETA_PACKEDBYTES_2

# define K 8
# define L 7
# define ETA 2
# define TAU 60
# define BETA 120
# define GAMMA1 (1 << 19)
# define GAMMA2 ((Q-1)/32)
# define OMEGA 75
# define CTILDEBYTES 64

# define poly_decompose poly_decompose_32
# define poly_make_hint poly_make_hint_32
# define poly_use_hint poly_use_hint_32
# define poly_uniform_eta poly_uniform_eta_2
# define poly_uniform_gamma1 poly_uniform_gamma1_19
# define polyz_pack polyz_pack_19
# define polyz_unpack polyz_unpack_19
# define polyeta_pack polyeta_pack_2
# define polyeta_unpack polyeta_unpack_2
# define polyw1_pack polyw1_pack_32

# define polyvecl VARIANT5(polyvecl)
# define polyveck VARIANT5(polyveck)
# define pack_pk VARIANT5(pack_pk)
# define unpack_pk VARIANT5(unpack_pk)
# define pack_sk VARIANT5(pack_sk)
# define unpack_sk VARIANT5(unpack_sk)
# define pack_sig VARIANT5(pack_sig)
# define unpack_sig VARIANT5(unpack_sig)
# define poly_challenge VARIANT5(poly_challenge)
# define polyvec_matrix_expand VARIANT5(polyvec_matrix_expand)
# define polyvec_matrix_pointwise_montgomery VARIANT5(polyvec_matrix_pointwise_montgomery)
# define polyveck_power2round VARIANT5(polyvec_power2round)
# define polyveck_make_hint VARIANT5(polyvec_make_hint)
# define polyveck_use_hint VARIANT5(polyvec_use_hint)
# define polyvecl_uniform_eta VARIANT5(polyvecl_uniform_eta)
# define polyvecl_uniform_gamma1 VARIANT5(polyvecl_uniform_gamma1)
# define polyvecl_reduce VARIANT5(polyvecl_reduce)
# define polyvecl_add VARIANT5(polyvecl_add)
# define polyvecl_ntt VARIANT5(polyvecl_ntt)
# define polyvecl_invntt_tomont VARIANT5(polyvecl_invntt_tomont)
# define polyvecl_pointwise_poly_montgomery VARIANT5(polyvecl_pointwise_poly_montgomery)
# define polyvecl_pointwise_acc_montgomery VARIANT5(polyvecl_pointwise_acc_montgomery)
# define polyvecl_chknorm VARIANT5(polyvecl_chknorm)
# define polyveck_uniform_eta VARIANT5(polyveck_uniform_eta)
# define polyveck_reduce VARIANT5(polyveck_reduce)
# define polyveck_caddq VARIANT5(polyveck_caddq)
# define polyveck_add VARIANT5(polyveck_add)
# define polyveck_sub VARIANT5(polyveck_sub)
# define polyveck_shiftl VARIANT5(polyveck_shiftl)
# define polyveck_ntt VARIANT5(polyveck_ntt)
# define polyveck_invntt_tomont VARIANT5(polyveck_invntt_tomont)
# define polyveck_pointwise_poly_montgomery VARIANT5(polyveck_pointwise_poly_montgomery)
# define polyveck_chknorm VARIANT5(polyveck_chknorm)
# define polyveck_pack_w1 VARIANT5(polyveck_pack_w1)
# define polyveck_decompose VARIANT5(polyveck_decompose)
# define crypto_sign_keypair VARIANT5(crypto_sign_keypair)
# define crypto_sign_keypair_internal VARIANT5(crypto_sign_keypair_internal)
# define crypto_sign_signature_internal VARIANT5(crypto_sign_signature_internal)
# define crypto_sign_signature VARIANT5(crypto_sign_signature)
# define crypto_sign VARIANT5(crypto_sign)
# define crypto_sign_verify_internal VARIANT5(crypto_sign_verify_internal)
# define crypto_sign_verify VARIANT5(crypto_sign_verify)
# define crypto_sign_open VARIANT5(crypto_sign_open)

# include "dilithium-dep.c"

#endif
