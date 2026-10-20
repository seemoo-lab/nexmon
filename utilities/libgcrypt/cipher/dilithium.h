/* dilithium.h - the Dilithium (header)
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
/* Standalone use is possible either with DILITHIUM_MODE defined with
 * the value (2, 3, or 5), or not defined.  For the latter, routines
 * for three variants are available.
 */
#ifndef DILITHIUM_H
#define DILITHIUM_H

#define SEEDBYTES 32
#define RNDBYTES 32

#ifdef _GCRYPT_IN_LIBGCRYPT
/**** Start of the glue code to libgcrypt ****/
#define dilithium_keypair   _gcry_mldsa_keypair
#define dilithium_sign      _gcry_mldsa_sign
#define dilithium_verify    _gcry_mldsa_verify
/**** End of the glue code ****/

#define DILITHIUM_KEYPAIR_STACK_BURN (128 * 1024)
#define DILITHIUM_SIGN_STACK_BURN (161 * 1024)
#define DILITHIUM_VERIFY_STACK_BURN (122 * 1024)

gpg_err_code_t dilithium_keypair (int algo, uint8_t *pk, uint8_t *sk,
                                  const uint8_t seed[SEEDBYTES]);
gpg_err_code_t dilithium_sign (int algo, uint8_t *sig, size_t siglen,
                               const uint8_t *m, size_t mlen,
                               const uint8_t *ctx, size_t ctxlen,
                               const uint8_t *sk, const uint8_t rnd[RNDBYTES]);
gpg_err_code_t dilithium_verify (int algo, const uint8_t *sig, size_t siglen,
                                 const uint8_t *m, size_t mlen,
                                 const uint8_t *ctx, size_t ctxlen,
                                 const uint8_t *pk);
#endif

#if defined(DILITHIUM_MODE)
#ifndef DILITHIUM_INTERNAL_API_ONLY
int crypto_sign_keypair (uint8_t *pk, uint8_t *sk);
int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen,
                          const uint8_t *ctx, size_t ctxlen,
                          const uint8_t *sk);
int crypto_sign (uint8_t *sm, size_t *smlen,
                 const uint8_t *m, size_t mlen,
                 const uint8_t *ctx, size_t ctxlen,
                 const uint8_t *sk);
int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen,
                       const uint8_t *ctx, size_t ctxlen,
                       const uint8_t *pk);
int crypto_sign_open (uint8_t *m, size_t *mlen,
                      const uint8_t *sm, size_t smlen,
                      const uint8_t *ctx, size_t ctxlen,
                      const uint8_t *pk);
#endif
int crypto_sign_keypair_internal (uint8_t *pk, uint8_t *sk,
                                  const uint8_t seed[SEEDBYTES]);
int crypto_sign_signature_internal (uint8_t *sig, size_t *siglen,
                                    const uint8_t *m, size_t mlen,
                                    const uint8_t *pre, size_t prelen,
                                    const uint8_t rnd[RNDBYTES],
                                    const uint8_t *sk);
int crypto_sign_verify_internal (const uint8_t *sig, size_t siglen,
                                 const uint8_t *m, size_t mlen,
                                 const uint8_t *pre, size_t prelen,
                                 const uint8_t *pk);

# if DILITHIUM_MODE == 2
# define CRYPTO_PUBLICKEYBYTES (SEEDBYTES + 4*320)
# define CRYPTO_SECRETKEYBYTES (2*SEEDBYTES \
                                + 64 \
                                + 4*96 \
                                + 4*96 \
                                + 4*416)
# define CRYPTO_BYTES (32 + 4*576 + 80 + 4)
# elif DILITHIUM_MODE == 3
# define CRYPTO_PUBLICKEYBYTES (SEEDBYTES + 6*320)
# define CRYPTO_SECRETKEYBYTES (2*SEEDBYTES \
                                + 64 \
                                + 5*128 \
                                + 6*128 \
                                + 6*416)
# define CRYPTO_BYTES (48 + 5*640 + 55 + 6)
# elif DILITHIUM_MODE == 5
# define CRYPTO_PUBLICKEYBYTES (SEEDBYTES + 8*320)
# define CRYPTO_SECRETKEYBYTES (2*SEEDBYTES \
                                + 64 \
                                + 7*96 \
                                + 8*96 \
                                + 8*416)
# define CRYPTO_BYTES (64 + 7*640 + 75 + 8)
# else
# error "DILITHIUM_MODE should be either 2, 3 or 5"
# endif
#else
# ifndef DILITHIUM_INTERNAL_API_ONLY
int crypto_sign_keypair_2 (uint8_t *pk, uint8_t *sk);
int crypto_sign_keypair_3 (uint8_t *pk, uint8_t *sk);
int crypto_sign_keypair_5 (uint8_t *pk, uint8_t *sk);
int crypto_sign_2 (uint8_t *sm, size_t *smlen,
                   const uint8_t *m, size_t mlen,
                   const uint8_t *ctx, size_t ctxlen,
                   const uint8_t *sk);
int crypto_sign_3 (uint8_t *sm, size_t *smlen,
                   const uint8_t *m, size_t mlen,
                   const uint8_t *ctx, size_t ctxlen,
                   const uint8_t *sk);
int crypto_sign_5 (uint8_t *sm, size_t *smlen,
                   const uint8_t *m, size_t mlen,
                   const uint8_t *ctx, size_t ctxlen,
                   const uint8_t *sk);
int crypto_sign_open_2 (uint8_t *m, size_t *mlen,
                        const uint8_t *sm, size_t smlen,
                        const uint8_t *ctx, size_t ctxlen,
                        const uint8_t *pk);
int crypto_sign_open_3 (uint8_t *m, size_t *mlen,
                        const uint8_t *sm, size_t smlen,
                        const uint8_t *ctx, size_t ctxlen,
                        const uint8_t *pk);
int crypto_sign_open_5 (uint8_t *m, size_t *mlen,
                        const uint8_t *sm, size_t smlen,
                        const uint8_t *ctx, size_t ctxlen,
                        const uint8_t *pk);
# endif

# define CRYPTO_PUBLICKEYBYTES_2 (SEEDBYTES + 4*320)
# define CRYPTO_SECRETKEYBYTES_2 (2*SEEDBYTES \
                                  + 64 \
                                  + 4*96 \
                                  + 4*96 \
                                  + 4*416)
# define CRYPTO_BYTES_2 (32 + 4*576 + 80 + 4)

# define CRYPTO_PUBLICKEYBYTES_3 (SEEDBYTES + 6*320)
# define CRYPTO_SECRETKEYBYTES_3 (2*SEEDBYTES \
                                  + 64 \
                                  + 5*128 \
                                  + 6*128 \
                                  + 6*416)
# define CRYPTO_BYTES_3 (48 + 5*640 + 55 + 6)

# define CRYPTO_PUBLICKEYBYTES_5 (SEEDBYTES + 8*320)
# define CRYPTO_SECRETKEYBYTES_5 (2*SEEDBYTES \
                                  + 64 \
                                  + 7*96 \
                                  + 8*96 \
                                  + 8*416)
# define CRYPTO_BYTES_5 (64 + 7*640 + 75 + 8)
#endif

#endif
