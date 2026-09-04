/* t-fips-service-ind.c - FIPS service indicator regression tests
 * Copyright (C) 2024 g10 Code GmbH
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
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define PGM "t-fips-service-ind"

#define NEED_HEX2BUFFER
#include "t-common.h"
static int in_fips_mode;
#define MAX_DATA_LEN 1040

/* Mingw requires us to include windows.h after winsock2.h which is
   included by gcrypt.h.  */
#ifdef _WIN32
# include <windows.h>
#endif

/* Check gcry_pk_genkey, gcry_pk_testkey, gcry_pk_get_nbits, gcry_pk_get_curve API.  */
static void
check_pk_g_t_n_c (int reject)
{
  static struct {
    const char *keyparms;
    int expect_failure;
  } tv[] = {
    {
      "(genkey (ecc (curve nistp256)))",
      0
    },
    {                           /* non-compliant curve */
      "(genkey (ecc (curve secp256k1)))",
      1
    }
  };
  int tvidx;
  gpg_error_t err;
  gpg_err_code_t ec;

  for (tvidx=0; tvidx < DIM(tv); tvidx++)
    {
      gcry_sexp_t s_kp = NULL;
      gcry_sexp_t s_sk = NULL;
      int nbits;
      const char *name;

      if (verbose)
        info ("checking gcry_pk_{genkey,testkey,get_nbits,get_curve} test %d\n", tvidx);

      err = gcry_sexp_build (&s_kp, NULL, tv[tvidx].keyparms);
      if (err)
        {
          fail ("error building SEXP for test, %s: %s",
                "keyparms", gpg_strerror (err));
          goto next;
        }

      err = gcry_pk_genkey (&s_sk, s_kp);
      if (err)
        {
          if (in_fips_mode && reject && tv[tvidx].expect_failure)
            /* Here, an error is expected */
            ;
          else
            fail ("gcry_pk_genkey failed: %s", gpg_strerror (err));
          goto next;
        }
      else
        {
          if (in_fips_mode && reject && tv[tvidx].expect_failure)
            {
              fail ("gcry_pk_genkey test %d unexpectedly succeeded", tvidx);
              goto next;
            }
        }

      ec = gcry_get_fips_service_indicator ();
      if (ec == GPG_ERR_INV_OP)
        {
          /* libgcrypt is old, no support of the FIPS service indicator.  */
          fail ("gcry_pk_genkey test %d unexpectedly failed to check the FIPS service indicator.\n",
                tvidx);
          goto next;
        }

      if (in_fips_mode && !tv[tvidx].expect_failure && ec)
        {
          /* Success with the FIPS service indicator == 0 expected, but != 0.  */
          fail ("gcry_pk_genkey test %d unexpectedly set the indicator in FIPS mode.\n",
                tvidx);
          goto next;
        }
      else if (in_fips_mode && tv[tvidx].expect_failure && !ec)
        {
          /* Success with the FIPS service indicator != 0 expected, but == 0.  */
          fail ("gcry_pk_genkey test %d unexpectedly cleared the indicator in FIPS mode.\n",
                tvidx);
          goto next;
        }

      err = gcry_pk_testkey (s_sk);
      if (err)
        {
          fail ("gcry_pk_testkey failed for test: %s", gpg_strerror (err));
          goto next;
        }

      ec = gcry_get_fips_service_indicator ();
      if (ec == GPG_ERR_INV_OP)
        {
          /* libgcrypt is old, no support of the FIPS service indicator.  */
          fail ("gcry_pk_testkey test %d unexpectedly failed to check the FIPS service indicator.\n",
                tvidx);
          goto next;
        }

      if (in_fips_mode && !tv[tvidx].expect_failure && ec)
        {
          /* Success with the FIPS service indicator == 0 expected, but != 0.  */
          fail ("gcry_pk_testkey test %d unexpectedly set the indicator in FIPS mode.\n",
                tvidx);
          goto next;
        }
      else if (in_fips_mode && tv[tvidx].expect_failure && !ec)
        {
          /* Success with the FIPS service indicator != 0 expected, but == 0.  */
          fail ("gcry_pk_testkey test %d unexpectedly cleared the indicator in FIPS mode.\n",
                tvidx);
          goto next;
        }

      nbits = gcry_pk_get_nbits (s_sk);
      if (!nbits)
        {
          fail ("gcry_pk_get_nbits failed for test");
          goto next;
        }

      ec = gcry_get_fips_service_indicator ();
      if (ec == GPG_ERR_INV_OP)
        {
          /* libgcrypt is old, no support of the FIPS service indicator.  */
          fail ("gcry_pk_get_nbits test %d unexpectedly failed to check the FIPS service indicator.\n",
                tvidx);
          goto next;
        }

      if (in_fips_mode && !tv[tvidx].expect_failure && ec)
        {
          /* Success with the FIPS service indicator == 0 expected, but != 0.  */
          fail ("gcry_pk_get_nbits test %d unexpectedly set the indicator in FIPS mode.\n",
                tvidx);
          goto next;
        }
      else if (in_fips_mode && tv[tvidx].expect_failure && !ec)
        {
          /* Success with the FIPS service indicator != 0 expected, but == 0.  */
          fail ("gcry_pk_get_nbits test %d unexpectedly cleared the indicator in FIPS mode.\n",
                tvidx);
          goto next;
        }

      name = gcry_pk_get_curve (s_sk, 0, NULL);
      if (!name)
        {
          fail ("gcry_pk_get_curve failed for test: %s", gpg_strerror (err));
          goto next;
        }

      ec = gcry_get_fips_service_indicator ();
      if (ec == GPG_ERR_INV_OP)
        {
          /* libgcrypt is old, no support of the FIPS service indicator.  */
          fail ("gcry_pk_get_curve test %d unexpectedly failed to check the FIPS service indicator.\n",
                tvidx);
          goto next;
        }

      if (in_fips_mode && !tv[tvidx].expect_failure && ec)
        {
          /* Success with the FIPS service indicator == 0 expected, but != 0.  */
          fail ("gcry_pk_get_curve test %d unexpectedly set the indicator in FIPS mode.\n",
                tvidx);
          goto next;
        }
      else if (in_fips_mode && tv[tvidx].expect_failure && !ec)
        {
          /* Success with the FIPS service indicator != 0 expected, but == 0.  */
          fail ("gcry_pk_get_curve test %d unexpectedly cleared the indicator in FIPS mode.\n",
                tvidx);
          goto next;
        }

    next:
      gcry_sexp_release (s_kp);
      gcry_sexp_release (s_sk);
    }
}

/* Check gcry_pk_sign, gcry_verify API.  */
static void
check_pk_s_v (int reject)
{
  static struct {
    const char *prvkey;
    const char *pubkey;
    const char *data;
    int expect_failure;
  } tv[] = {
    {                           /* Hashing is done externally, and feeded
                                   to gcry_pk_sign, specifing the hash used */
      "(private-key (ecc (curve nistp256)"
      " (d #519b423d715f8b581f4fa8ee59f4771a5b44c8130b4e3eacca54a56dda72b464#)))",
      "(public-key (ecc (curve nistp256)"
      " (q #041ccbe91c075fc7f4f033bfa248db8fccd3565de94bbfb12f3c59ff46c271bf83"
      "ce4014c68811f9a21a1fdb2c0e6113e06db7ca93b7404e78dc7ccd5ca89a4ca9#)))",
      "(data (flags raw)(hash sha256 "
      "#00112233445566778899AABBCCDDEEFF000102030405060708090A0B0C0D0E0F#))",
      0
    },
    {                           /* non-compliant curve */
      "(private-key (ecc (curve secp256k1)"
      " (d #c2cdf0a8b0a83b35ace53f097b5e6e6a0a1f2d40535eff1cf434f52a43d59d8f#)))",
      "(public-key (ecc (curve secp256k1)"
      " (q #046fcc37ea5e9e09fec6c83e5fbd7a745e3eee81d16ebd861c9e66f55518c19798"
      "4e9f113c07f875691df8afc1029496fc4cb9509b39dcd38f251a83359cc8b4f7#)))",
      "(data (flags raw)(hash sha256 "
      "#00112233445566778899AABBCCDDEEFF000102030405060708090A0B0C0D0E0F#))",
      1
    },
    {                           /* non-compliant hash */
      "(private-key (ecc (curve nistp256)"
      " (d #519b423d715f8b581f4fa8ee59f4771a5b44c8130b4e3eacca54a56dda72b464#)))",
      "(public-key (ecc (curve nistp256)"
      " (q #041ccbe91c075fc7f4f033bfa248db8fccd3565de94bbfb12f3c59ff46c271bf83"
      "ce4014c68811f9a21a1fdb2c0e6113e06db7ca93b7404e78dc7ccd5ca89a4ca9#)))",
      "(data (flags raw)(hash ripemd160 "
      "#00112233445566778899AABBCCDDEEFF00010203#))",
      1
    },
    {                           /* non-compliant hash for signing */
      "(private-key (ecc (curve nistp256)"
      " (d #519b423d715f8b581f4fa8ee59f4771a5b44c8130b4e3eacca54a56dda72b464#)))",
      "(public-key (ecc (curve nistp256)"
      " (q #041ccbe91c075fc7f4f033bfa248db8fccd3565de94bbfb12f3c59ff46c271bf83"
      "ce4014c68811f9a21a1fdb2c0e6113e06db7ca93b7404e78dc7ccd5ca89a4ca9#)))",
      "(data (flags raw)(hash sha1 "
      "#00112233445566778899AABBCCDDEEFF00010203#))",
      1
    },
    {                           /* Hashing is done internally in
                                   gcry_pk_sign with the hash-algo specified.  */
      "(private-key\n"
      " (ecc\n"
      "  (curve Ed25519)(flags eddsa)\n"
      "  (q #4014DB483F15527253B25B4C72BEA8BB70255029636BD71DBBCCD5D8BF48A35F17#)"
      "  (d #09A0C38E0F1699073541447C19DA12E3A07A7BFDB0C186E4AC5BCE6F23D55252#)"
      "))",
      "(public-key\n"
      " (ecc\n"
      "  (curve Ed25519)(flags eddsa)\n"
      "  (q #4014DB483F15527253B25B4C72BEA8BB70255029636BD71DBBCCD5D8BF48A35F17#)"
      "))",
      "(data(flags eddsa)(hash-algo sha512)(value "
      "#00112233445566778899AABBCCDDEEFF000102030405060708090A0B0C0D0E0F"
      " 00112233445566778899AABBCCDDEEFF000102030405060708090A0B0C0D0E0F"
      " 00112233445566778899AABBCCDDEEFF000102030405060708090A0B0C0D0E0F#))",
      0
    }
#if USE_RSA
    ,
    {                           /* RSA with compliant hash for signing */
      "(private-key"
      " (rsa"
      "  (n #009F56231A3D82E3E7D613D59D53E9AB921BEF9F08A782AED0B6E46ADBC853EC"
      "      7C71C422435A3CD8FA0DB9EFD55CD3295BADC4E8E2E2B94E15AE82866AB8ADE8"
      "      7E469FAE76DC3577DE87F1F419C4EB41123DFAF8D16922D5EDBAD6E9076D5A1C"
      "      958106F0AE5E2E9193C6B49124C64C2A241C4075D4AF16299EB87A6585BAE917"
      "      DEF27FCDD165764D069BC18D16527B29DAAB549F7BBED4A7C6A842D203ED6613"
      "      6E2411744E432CD26D940132F25874483DCAEECDFD95744819CBCF1EA810681C"
      "      42907EBCB1C7EAFBE75C87EC32C5413EA10476545D3FC7B2ADB1B66B7F200918"
      "      664B0E5261C2895AA28B0DE321E921B3F877172CCCAB81F43EF98002916156F6"
      "      CB#)\n"
      "   (e #010001#)\n"
      "   (d #07EF82500C403899934FE993AC5A36F14FF2DF38CF1EF315F205EE4C83EDAA19"
      "       8890FC23DE9AA933CAFB37B6A8A8DBA675411958337287310D3FF2F1DDC0CB93"
      "       7E70F57F75F833C021852B631D2B9A520E4431A03C5C3FCB5742DCD841D9FB12"
      "       771AA1620DCEC3F1583426066ED9DC3F7028C5B59202C88FDF20396E2FA0EC4F"
      "       5A22D9008F3043673931BC14A5046D6327398327900867E39CC61B2D1AFE2F48"
      "       EC8E1E3861C68D257D7425F4E6F99ABD77D61F10CA100EFC14389071831B33DD"
      "       69CC8EABEF860D1DC2AAA84ABEAE5DFC91BC124DAF0F4C8EF5BBEA436751DE84"
      "       3A8063E827A024466F44C28614F93B0732A100D4A0D86D532FE1E22C7725E401"
      "       #)\n"
      "   (p #00C29D438F115825779631CD665A5739367F3E128ADC29766483A46CA80897E0"
      "       79B32881860B8F9A6A04C2614A904F6F2578DAE13EA67CD60AE3D0AA00A1FF9B"
      "       441485E44B2DC3D0B60260FBFE073B5AC72FAF67964DE15C8212C389D20DB9CF"
      "       54AF6AEF5C4196EAA56495DD30CF709F499D5AB30CA35E086C2A1589D6283F17"
      "       83#)\n"
      "   (q #00D1984135231CB243FE959C0CBEF551EDD986AD7BEDF71EDF447BE3DA27AF46"
      "       79C974A6FA69E4D52FE796650623DE70622862713932AA2FD9F2EC856EAEAA77"
      "       88B4EA6084DC81C902F014829B18EA8B2666EC41586818E0589E18876065F97E"
      "       8D22CE2DA53A05951EC132DCEF41E70A9C35F4ACC268FFAC2ADF54FA1DA110B9"
      "       19#)\n"
      "   (u #67CF0FD7635205DD80FA814EE9E9C267C17376BF3209FB5D1BC42890D2822A04"
      "       479DAF4D5B6ED69D0F8D1AF94164D07F8CD52ECEFE880641FA0F41DDAB1785E4"
      "       A37A32F997A516480B4CD4F6482B9466A1765093ED95023CA32D5EDC1E34CEE9"
      "       AF595BC51FE43C4BF810FA225AF697FB473B83815966188A4312C048B885E3F7"
      "       #)))\n",
      "(public-key\n"
      " (rsa\n"
      "  (n #009F56231A3D82E3E7D613D59D53E9AB921BEF9F08A782AED0B6E46ADBC853EC"
      "      7C71C422435A3CD8FA0DB9EFD55CD3295BADC4E8E2E2B94E15AE82866AB8ADE8"
      "      7E469FAE76DC3577DE87F1F419C4EB41123DFAF8D16922D5EDBAD6E9076D5A1C"
      "      958106F0AE5E2E9193C6B49124C64C2A241C4075D4AF16299EB87A6585BAE917"
      "      DEF27FCDD165764D069BC18D16527B29DAAB549F7BBED4A7C6A842D203ED6613"
      "      6E2411744E432CD26D940132F25874483DCAEECDFD95744819CBCF1EA810681C"
      "      42907EBCB1C7EAFBE75C87EC32C5413EA10476545D3FC7B2ADB1B66B7F200918"
      "      664B0E5261C2895AA28B0DE321E921B3F877172CCCAB81F43EF98002916156F6"
      "      CB#)\n"
      "   (e #010001#)))\n",
      "(data\n (flags pkcs1)\n"
      " (hash sha256 "
      "#00112233445566778899AABBCCDDEEFF000102030405060708090A0B0C0D0E0F#))\n",
      0
    },
    {                           /* RSA with non-compliant hash for signing */
      "(private-key"
      " (rsa"
      "  (n #009F56231A3D82E3E7D613D59D53E9AB921BEF9F08A782AED0B6E46ADBC853EC"
      "      7C71C422435A3CD8FA0DB9EFD55CD3295BADC4E8E2E2B94E15AE82866AB8ADE8"
      "      7E469FAE76DC3577DE87F1F419C4EB41123DFAF8D16922D5EDBAD6E9076D5A1C"
      "      958106F0AE5E2E9193C6B49124C64C2A241C4075D4AF16299EB87A6585BAE917"
      "      DEF27FCDD165764D069BC18D16527B29DAAB549F7BBED4A7C6A842D203ED6613"
      "      6E2411744E432CD26D940132F25874483DCAEECDFD95744819CBCF1EA810681C"
      "      42907EBCB1C7EAFBE75C87EC32C5413EA10476545D3FC7B2ADB1B66B7F200918"
      "      664B0E5261C2895AA28B0DE321E921B3F877172CCCAB81F43EF98002916156F6"
      "      CB#)\n"
      "   (e #010001#)\n"
      "   (d #07EF82500C403899934FE993AC5A36F14FF2DF38CF1EF315F205EE4C83EDAA19"
      "       8890FC23DE9AA933CAFB37B6A8A8DBA675411958337287310D3FF2F1DDC0CB93"
      "       7E70F57F75F833C021852B631D2B9A520E4431A03C5C3FCB5742DCD841D9FB12"
      "       771AA1620DCEC3F1583426066ED9DC3F7028C5B59202C88FDF20396E2FA0EC4F"
      "       5A22D9008F3043673931BC14A5046D6327398327900867E39CC61B2D1AFE2F48"
      "       EC8E1E3861C68D257D7425F4E6F99ABD77D61F10CA100EFC14389071831B33DD"
      "       69CC8EABEF860D1DC2AAA84ABEAE5DFC91BC124DAF0F4C8EF5BBEA436751DE84"
      "       3A8063E827A024466F44C28614F93B0732A100D4A0D86D532FE1E22C7725E401"
      "       #)\n"
      "   (p #00C29D438F115825779631CD665A5739367F3E128ADC29766483A46CA80897E0"
      "       79B32881860B8F9A6A04C2614A904F6F2578DAE13EA67CD60AE3D0AA00A1FF9B"
      "       441485E44B2DC3D0B60260FBFE073B5AC72FAF67964DE15C8212C389D20DB9CF"
      "       54AF6AEF5C4196EAA56495DD30CF709F499D5AB30CA35E086C2A1589D6283F17"
      "       83#)\n"
      "   (q #00D1984135231CB243FE959C0CBEF551EDD986AD7BEDF71EDF447BE3DA27AF46"
      "       79C974A6FA69E4D52FE796650623DE70622862713932AA2FD9F2EC856EAEAA77"
      "       88B4EA6084DC81C902F014829B18EA8B2666EC41586818E0589E18876065F97E"
      "       8D22CE2DA53A05951EC132DCEF41E70A9C35F4ACC268FFAC2ADF54FA1DA110B9"
      "       19#)\n"
      "   (u #67CF0FD7635205DD80FA814EE9E9C267C17376BF3209FB5D1BC42890D2822A04"
      "       479DAF4D5B6ED69D0F8D1AF94164D07F8CD52ECEFE880641FA0F41DDAB1785E4"
      "       A37A32F997A516480B4CD4F6482B9466A1765093ED95023CA32D5EDC1E34CEE9"
      "       AF595BC51FE43C4BF810FA225AF697FB473B83815966188A4312C048B885E3F7"
      "       #)))\n",
      "(public-key\n"
      " (rsa\n"
      "  (n #009F56231A3D82E3E7D613D59D53E9AB921BEF9F08A782AED0B6E46ADBC853EC"
      "      7C71C422435A3CD8FA0DB9EFD55CD3295BADC4E8E2E2B94E15AE82866AB8ADE8"
      "      7E469FAE76DC3577DE87F1F419C4EB41123DFAF8D16922D5EDBAD6E9076D5A1C"
      "      958106F0AE5E2E9193C6B49124C64C2A241C4075D4AF16299EB87A6585BAE917"
      "      DEF27FCDD165764D069BC18D16527B29DAAB549F7BBED4A7C6A842D203ED6613"
      "      6E2411744E432CD26D940132F25874483DCAEECDFD95744819CBCF1EA810681C"
      "      42907EBCB1C7EAFBE75C87EC32C5413EA10476545D3FC7B2ADB1B66B7F200918"
      "      664B0E5261C2895AA28B0DE321E921B3F877172CCCAB81F43EF98002916156F6"
      "      CB#)\n"
      "   (e #010001#)))\n",
      "(data\n (flags pkcs1)\n"
      " (hash sha1 #11223344556677889900AABBCCDDEEFF10203040#))\n",
      1
    },
    {                           /* RSA with unknown hash for signing */
      "(private-key"
      " (rsa"
      "  (n #009F56231A3D82E3E7D613D59D53E9AB921BEF9F08A782AED0B6E46ADBC853EC"
      "      7C71C422435A3CD8FA0DB9EFD55CD3295BADC4E8E2E2B94E15AE82866AB8ADE8"
      "      7E469FAE76DC3577DE87F1F419C4EB41123DFAF8D16922D5EDBAD6E9076D5A1C"
      "      958106F0AE5E2E9193C6B49124C64C2A241C4075D4AF16299EB87A6585BAE917"
      "      DEF27FCDD165764D069BC18D16527B29DAAB549F7BBED4A7C6A842D203ED6613"
      "      6E2411744E432CD26D940132F25874483DCAEECDFD95744819CBCF1EA810681C"
      "      42907EBCB1C7EAFBE75C87EC32C5413EA10476545D3FC7B2ADB1B66B7F200918"
      "      664B0E5261C2895AA28B0DE321E921B3F877172CCCAB81F43EF98002916156F6"
      "      CB#)\n"
      "   (e #010001#)\n"
      "   (d #07EF82500C403899934FE993AC5A36F14FF2DF38CF1EF315F205EE4C83EDAA19"
      "       8890FC23DE9AA933CAFB37B6A8A8DBA675411958337287310D3FF2F1DDC0CB93"
      "       7E70F57F75F833C021852B631D2B9A520E4431A03C5C3FCB5742DCD841D9FB12"
      "       771AA1620DCEC3F1583426066ED9DC3F7028C5B59202C88FDF20396E2FA0EC4F"
      "       5A22D9008F3043673931BC14A5046D6327398327900867E39CC61B2D1AFE2F48"
      "       EC8E1E3861C68D257D7425F4E6F99ABD77D61F10CA100EFC14389071831B33DD"
      "       69CC8EABEF860D1DC2AAA84ABEAE5DFC91BC124DAF0F4C8EF5BBEA436751DE84"
      "       3A8063E827A024466F44C28614F93B0732A100D4A0D86D532FE1E22C7725E401"
      "       #)\n"
      "   (p #00C29D438F115825779631CD665A5739367F3E128ADC29766483A46CA80897E0"
      "       79B32881860B8F9A6A04C2614A904F6F2578DAE13EA67CD60AE3D0AA00A1FF9B"
      "       441485E44B2DC3D0B60260FBFE073B5AC72FAF67964DE15C8212C389D20DB9CF"
      "       54AF6AEF5C4196EAA56495DD30CF709F499D5AB30CA35E086C2A1589D6283F17"
      "       83#)\n"
      "   (q #00D1984135231CB243FE959C0CBEF551EDD986AD7BEDF71EDF447BE3DA27AF46"
      "       79C974A6FA69E4D52FE796650623DE70622862713932AA2FD9F2EC856EAEAA77"
      "       88B4EA6084DC81C902F014829B18EA8B2666EC41586818E0589E18876065F97E"
      "       8D22CE2DA53A05951EC132DCEF41E70A9C35F4ACC268FFAC2ADF54FA1DA110B9"
      "       19#)\n"
      "   (u #67CF0FD7635205DD80FA814EE9E9C267C17376BF3209FB5D1BC42890D2822A04"
      "       479DAF4D5B6ED69D0F8D1AF94164D07F8CD52ECEFE880641FA0F41DDAB1785E4"
      "       A37A32F997A516480B4CD4F6482B9466A1765093ED95023CA32D5EDC1E34CEE9"
      "       AF595BC51FE43C4BF810FA225AF697FB473B83815966188A4312C048B885E3F7"
      "       #)))\n",
      "(public-key\n"
      " (rsa\n"
      "  (n #009F56231A3D82E3E7D613D59D53E9AB921BEF9F08A782AED0B6E46ADBC853EC"
      "      7C71C422435A3CD8FA0DB9EFD55CD3295BADC4E8E2E2B94E15AE82866AB8ADE8"
      "      7E469FAE76DC3577DE87F1F419C4EB41123DFAF8D16922D5EDBAD6E9076D5A1C"
      "      958106F0AE5E2E9193C6B49124C64C2A241C4075D4AF16299EB87A6585BAE917"
      "      DEF27FCDD165764D069BC18D16527B29DAAB549F7BBED4A7C6A842D203ED6613"
      "      6E2411744E432CD26D940132F25874483DCAEECDFD95744819CBCF1EA810681C"
      "      42907EBCB1C7EAFBE75C87EC32C5413EA10476545D3FC7B2ADB1B66B7F200918"
      "      664B0E5261C2895AA28B0DE321E921B3F877172CCCAB81F43EF98002916156F6"
      "      CB#)\n"
      "   (e #010001#)))\n",
      "(data\n (flags pkcs1-raw)\n"
      " (value "
      "#00112233445566778899AABBCCDDEEFF000102030405060708090A0B0C0D0E0F#))\n",
      1
    },
    {                           /* RSA with compliant hash for signing */
      "(private-key"
      " (rsa"
      "  (n #009F56231A3D82E3E7D613D59D53E9AB921BEF9F08A782AED0B6E46ADBC853EC"
      "      7C71C422435A3CD8FA0DB9EFD55CD3295BADC4E8E2E2B94E15AE82866AB8ADE8"
      "      7E469FAE76DC3577DE87F1F419C4EB41123DFAF8D16922D5EDBAD6E9076D5A1C"
      "      958106F0AE5E2E9193C6B49124C64C2A241C4075D4AF16299EB87A6585BAE917"
      "      DEF27FCDD165764D069BC18D16527B29DAAB549F7BBED4A7C6A842D203ED6613"
      "      6E2411744E432CD26D940132F25874483DCAEECDFD95744819CBCF1EA810681C"
      "      42907EBCB1C7EAFBE75C87EC32C5413EA10476545D3FC7B2ADB1B66B7F200918"
      "      664B0E5261C2895AA28B0DE321E921B3F877172CCCAB81F43EF98002916156F6"
      "      CB#)\n"
      "   (e #010001#)\n"
      "   (d #07EF82500C403899934FE993AC5A36F14FF2DF38CF1EF315F205EE4C83EDAA19"
      "       8890FC23DE9AA933CAFB37B6A8A8DBA675411958337287310D3FF2F1DDC0CB93"
      "       7E70F57F75F833C021852B631D2B9A520E4431A03C5C3FCB5742DCD841D9FB12"
      "       771AA1620DCEC3F1583426066ED9DC3F7028C5B59202C88FDF20396E2FA0EC4F"
      "       5A22D9008F3043673931BC14A5046D6327398327900867E39CC61B2D1AFE2F48"
      "       EC8E1E3861C68D257D7425F4E6F99ABD77D61F10CA100EFC14389071831B33DD"
      "       69CC8EABEF860D1DC2AAA84ABEAE5DFC91BC124DAF0F4C8EF5BBEA436751DE84"
      "       3A8063E827A024466F44C28614F93B0732A100D4A0D86D532FE1E22C7725E401"
      "       #)\n"
      "   (p #00C29D438F115825779631CD665A5739367F3E128ADC29766483A46CA80897E0"
      "       79B32881860B8F9A6A04C2614A904F6F2578DAE13EA67CD60AE3D0AA00A1FF9B"
      "       441485E44B2DC3D0B60260FBFE073B5AC72FAF67964DE15C8212C389D20DB9CF"
      "       54AF6AEF5C4196EAA56495DD30CF709F499D5AB30CA35E086C2A1589D6283F17"
      "       83#)\n"
      "   (q #00D1984135231CB243FE959C0CBEF551EDD986AD7BEDF71EDF447BE3DA27AF46"
      "       79C974A6FA69E4D52FE796650623DE70622862713932AA2FD9F2EC856EAEAA77"
      "       88B4EA6084DC81C902F014829B18EA8B2666EC41586818E0589E18876065F97E"
      "       8D22CE2DA53A05951EC132DCEF41E70A9C35F4ACC268FFAC2ADF54FA1DA110B9"
      "       19#)\n"
      "   (u #67CF0FD7635205DD80FA814EE9E9C267C17376BF3209FB5D1BC42890D2822A04"
      "       479DAF4D5B6ED69D0F8D1AF94164D07F8CD52ECEFE880641FA0F41DDAB1785E4"
      "       A37A32F997A516480B4CD4F6482B9466A1765093ED95023CA32D5EDC1E34CEE9"
      "       AF595BC51FE43C4BF810FA225AF697FB473B83815966188A4312C048B885E3F7"
      "       #)))\n",
      "(public-key\n"
      " (rsa\n"
      "  (n #009F56231A3D82E3E7D613D59D53E9AB921BEF9F08A782AED0B6E46ADBC853EC"
      "      7C71C422435A3CD8FA0DB9EFD55CD3295BADC4E8E2E2B94E15AE82866AB8ADE8"
      "      7E469FAE76DC3577DE87F1F419C4EB41123DFAF8D16922D5EDBAD6E9076D5A1C"
      "      958106F0AE5E2E9193C6B49124C64C2A241C4075D4AF16299EB87A6585BAE917"
      "      DEF27FCDD165764D069BC18D16527B29DAAB549F7BBED4A7C6A842D203ED6613"
      "      6E2411744E432CD26D940132F25874483DCAEECDFD95744819CBCF1EA810681C"
      "      42907EBCB1C7EAFBE75C87EC32C5413EA10476545D3FC7B2ADB1B66B7F200918"
      "      664B0E5261C2895AA28B0DE321E921B3F877172CCCAB81F43EF98002916156F6"
      "      CB#)\n"
      "   (e #010001#)))\n",
      "(data\n (flags pss)\n"
      " (hash sha256 "
      "#00112233445566778899AABBCCDDEEFF000102030405060708090A0B0C0D0E0F#))\n",
      0
    },
    {                           /* RSA with non-compliant hash for signing */
      "(private-key"
      " (rsa"
      "  (n #009F56231A3D82E3E7D613D59D53E9AB921BEF9F08A782AED0B6E46ADBC853EC"
      "      7C71C422435A3CD8FA0DB9EFD55CD3295BADC4E8E2E2B94E15AE82866AB8ADE8"
      "      7E469FAE76DC3577DE87F1F419C4EB41123DFAF8D16922D5EDBAD6E9076D5A1C"
      "      958106F0AE5E2E9193C6B49124C64C2A241C4075D4AF16299EB87A6585BAE917"
      "      DEF27FCDD165764D069BC18D16527B29DAAB549F7BBED4A7C6A842D203ED6613"
      "      6E2411744E432CD26D940132F25874483DCAEECDFD95744819CBCF1EA810681C"
      "      42907EBCB1C7EAFBE75C87EC32C5413EA10476545D3FC7B2ADB1B66B7F200918"
      "      664B0E5261C2895AA28B0DE321E921B3F877172CCCAB81F43EF98002916156F6"
      "      CB#)\n"
      "   (e #010001#)\n"
      "   (d #07EF82500C403899934FE993AC5A36F14FF2DF38CF1EF315F205EE4C83EDAA19"
      "       8890FC23DE9AA933CAFB37B6A8A8DBA675411958337287310D3FF2F1DDC0CB93"
      "       7E70F57F75F833C021852B631D2B9A520E4431A03C5C3FCB5742DCD841D9FB12"
      "       771AA1620DCEC3F1583426066ED9DC3F7028C5B59202C88FDF20396E2FA0EC4F"
      "       5A22D9008F3043673931BC14A5046D6327398327900867E39CC61B2D1AFE2F48"
      "       EC8E1E3861C68D257D7425F4E6F99ABD77D61F10CA100EFC14389071831B33DD"
      "       69CC8EABEF860D1DC2AAA84ABEAE5DFC91BC124DAF0F4C8EF5BBEA436751DE84"
      "       3A8063E827A024466F44C28614F93B0732A100D4A0D86D532FE1E22C7725E401"
      "       #)\n"
      "   (p #00C29D438F115825779631CD665A5739367F3E128ADC29766483A46CA80897E0"
      "       79B32881860B8F9A6A04C2614A904F6F2578DAE13EA67CD60AE3D0AA00A1FF9B"
      "       441485E44B2DC3D0B60260FBFE073B5AC72FAF67964DE15C8212C389D20DB9CF"
      "       54AF6AEF5C4196EAA56495DD30CF709F499D5AB30CA35E086C2A1589D6283F17"
      "       83#)\n"
      "   (q #00D1984135231CB243FE959C0CBEF551EDD986AD7BEDF71EDF447BE3DA27AF46"
      "       79C974A6FA69E4D52FE796650623DE70622862713932AA2FD9F2EC856EAEAA77"
      "       88B4EA6084DC81C902F014829B18EA8B2666EC41586818E0589E18876065F97E"
      "       8D22CE2DA53A05951EC132DCEF41E70A9C35F4ACC268FFAC2ADF54FA1DA110B9"
      "       19#)\n"
      "   (u #67CF0FD7635205DD80FA814EE9E9C267C17376BF3209FB5D1BC42890D2822A04"
      "       479DAF4D5B6ED69D0F8D1AF94164D07F8CD52ECEFE880641FA0F41DDAB1785E4"
      "       A37A32F997A516480B4CD4F6482B9466A1765093ED95023CA32D5EDC1E34CEE9"
      "       AF595BC51FE43C4BF810FA225AF697FB473B83815966188A4312C048B885E3F7"
      "       #)))\n",
      "(public-key\n"
      " (rsa\n"
      "  (n #009F56231A3D82E3E7D613D59D53E9AB921BEF9F08A782AED0B6E46ADBC853EC"
      "      7C71C422435A3CD8FA0DB9EFD55CD3295BADC4E8E2E2B94E15AE82866AB8ADE8"
      "      7E469FAE76DC3577DE87F1F419C4EB41123DFAF8D16922D5EDBAD6E9076D5A1C"
      "      958106F0AE5E2E9193C6B49124C64C2A241C4075D4AF16299EB87A6585BAE917"
      "      DEF27FCDD165764D069BC18D16527B29DAAB549F7BBED4A7C6A842D203ED6613"
      "      6E2411744E432CD26D940132F25874483DCAEECDFD95744819CBCF1EA810681C"
      "      42907EBCB1C7EAFBE75C87EC32C5413EA10476545D3FC7B2ADB1B66B7F200918"
      "      664B0E5261C2895AA28B0DE321E921B3F877172CCCAB81F43EF98002916156F6"
      "      CB#)\n"
      "   (e #010001#)))\n",
      "(data\n (flags pss)\n"
      " (hash sha1 #11223344556677889900AABBCCDDEEFF10203040#))\n",
      1
    }
#endif /* USE_RSA */
  };
  int tvidx;
  gpg_error_t err;
  gpg_err_code_t ec;

  for (tvidx=0; tvidx < DIM(tv); tvidx++)
    {
      gcry_sexp_t s_pk = NULL;
      gcry_sexp_t s_sk = NULL;
      gcry_sexp_t s_data = NULL;
      gcry_sexp_t s_sig= NULL;

      if (verbose)
        info ("checking gcry_pk_{sign,verify} test %d\n", tvidx);

      err = gcry_sexp_build (&s_sk, NULL, tv[tvidx].prvkey);
      if (err)
        {
          fail ("error building SEXP for test, %s: %s",
                "sk", gpg_strerror (err));
          goto next;
        }

      err = gcry_sexp_build (&s_pk, NULL, tv[tvidx].pubkey);
      if (err)
        {
          fail ("error building SEXP for test, %s: %s",
                "pk", gpg_strerror (err));
          goto next;
        }

      err = gcry_sexp_build (&s_data, NULL, tv[tvidx].data);
      if (err)
        {
          fail ("error building SEXP for test, %s: %s",
                "data", gpg_strerror (err));
          goto next;
        }

      err = gcry_pk_sign (&s_sig, s_data, s_sk);
      if (err)
        {
          if (in_fips_mode && reject && tv[tvidx].expect_failure)
            /* Here, an error is expected */
            ;
          else
            fail ("gcry_pk_sign failed: %s", gpg_strerror (err));
          goto next;
        }
      else
        {
          if (in_fips_mode && reject && tv[tvidx].expect_failure)
            {
              fail ("gcry_pk_sign test %d unexpectedly succeeded", tvidx);
              goto next;
            }
        }

      ec = gcry_get_fips_service_indicator ();
      if (ec == GPG_ERR_INV_OP)
        {
          /* libgcrypt is old, no support of the FIPS service indicator.  */
          fail ("gcry_pk_sign test %d unexpectedly failed to check the FIPS service indicator.\n",
                tvidx);
          goto next;
        }

      if (in_fips_mode && !tv[tvidx].expect_failure && ec)
        {
          /* Success with the FIPS service indicator == 0 expected, but != 0.  */
          fail ("gcry_pk_sign test %d unexpectedly set the indicator in FIPS mode.\n",
                tvidx);
          goto next;
        }
      else if (in_fips_mode && tv[tvidx].expect_failure && !ec)
        {
          /* Success with the FIPS service indicator != 0 expected, but == 0.  */
          fail ("gcry_pk_sign test %d unexpectedly cleared the indicator in FIPS mode.\n",
                tvidx);
          goto next;
        }

      err = gcry_pk_verify (s_sig, s_data, s_pk);
      if (err)
        {
          fail ("gcry_pk_verify failed for test: %s", gpg_strerror (err));
          goto next;
        }

      ec = gcry_get_fips_service_indicator ();
      if (ec == GPG_ERR_INV_OP)
        {
          /* libgcrypt is old, no support of the FIPS service indicator.  */
          fail ("gcry_pk_verify test %d unexpectedly failed to check the FIPS service indicator.\n",
                tvidx);
          goto next;
        }

      if (in_fips_mode && !tv[tvidx].expect_failure && ec)
        {
          /* Success with the FIPS service indicator == 0 expected, but != 0.  */
          fail ("gcry_pk_verify test %d unexpectedly set the indicator in FIPS mode.\n",
                tvidx);
          goto next;
        }
      else if (in_fips_mode && tv[tvidx].expect_failure && !ec)
        {
          /* Success with the FIPS service indicator != 0 expected, but == 0.  */
          fail ("gcry_pk_verify test %d unexpectedly cleared the indicator in FIPS mode.\n",
                tvidx);
          goto next;
        }

    next:
      gcry_sexp_release (s_sig);
      gcry_sexp_release (s_data);
      gcry_sexp_release (s_pk);
      gcry_sexp_release (s_sk);
    }
}

/* Check gcry_pk_hash_sign, gcry_pk_hash_verify API.  */
static void
check_pk_hash_sign_verify (void)
{
  static struct {
    int md_algo;
    const char *prvkey;
    const char *pubkey;
    const char *data_tmpl;
    const char *k;
    int expect_failure;
    int expect_failure_hash;
  } tv[] = {
    {                           /* non-compliant hash */
      GCRY_MD_BLAKE2B_512,
      "(private-key (ecc (curve nistp256)"
      " (d #519b423d715f8b581f4fa8ee59f4771a5b44c8130b4e3eacca54a56dda72b464#)))",
      "(public-key (ecc (curve nistp256)"
      " (q #041ccbe91c075fc7f4f033bfa248db8fccd3565de94bbfb12f3c59ff46c271bf83"
      "ce4014c68811f9a21a1fdb2c0e6113e06db7ca93b7404e78dc7ccd5ca89a4ca9#)))",
      "(data(flags raw)(hash %s %b)(label %b))",
      "94a1bbb14b906a61a280f245f9e93c7f3b4a6247824f5d33b9670787642a68de",
      1, 1
    },
    {                           /* non-compliant curve */
      GCRY_MD_SHA256,
      "(private-key (ecc (curve secp256k1)"
      " (d #c2cdf0a8b0a83b35ace53f097b5e6e6a0a1f2d40535eff1cf434f52a43d59d8f#)))",

      "(public-key (ecc (curve secp256k1)"
      " (q #046fcc37ea5e9e09fec6c83e5fbd7a745e3eee81d16ebd861c9e66f55518c19798"
      "4e9f113c07f875691df8afc1029496fc4cb9509b39dcd38f251a83359cc8b4f7#)))",
      "(data(flags raw)(hash %s %b)(label %b))",
      "94a1bbb14b906a61a280f245f9e93c7f3b4a6247824f5d33b9670787642a68de",
      1, 0
    },
    {
      GCRY_MD_SHA256,
      "(private-key (ecc (curve nistp256)"
      " (d #519b423d715f8b581f4fa8ee59f4771a5b44c8130b4e3eacca54a56dda72b464#)))",
      "(public-key (ecc (curve nistp256)"
      " (q #041ccbe91c075fc7f4f033bfa248db8fccd3565de94bbfb12f3c59ff46c271bf83"
      "ce4014c68811f9a21a1fdb2c0e6113e06db7ca93b7404e78dc7ccd5ca89a4ca9#)))",
      "(data(flags raw)(hash %s %b)(label %b))",
      "94a1bbb14b906a61a280f245f9e93c7f3b4a6247824f5d33b9670787642a68de",
      1, 0,
    }
  };
  int tvidx;
  gpg_error_t err;
  gpg_err_code_t ec;
  const char *msg = "Takerufuji Mikiya, who won the championship in March 2024";
  int msglen;

  msglen = strlen (msg);
  for (tvidx=0; tvidx < DIM(tv); tvidx++)
    {
      gcry_md_hd_t hd = NULL;
      gcry_sexp_t s_sk = NULL;
      gcry_sexp_t s_pk = NULL;
      void *buffer = NULL;
      size_t buflen;
      gcry_ctx_t ctx = NULL;
      gcry_sexp_t s_sig= NULL;

      if (verbose)
        info ("checking gcry_pk_hash_ test %d\n", tvidx);

      err = gcry_md_open (&hd, tv[tvidx].md_algo, 0);
      if (err)
        {
          fail ("algo %d, gcry_md_open failed: %s\n", tv[tvidx].md_algo,
                gpg_strerror (err));
          goto next;
        }

      ec = gcry_get_fips_service_indicator ();
      if (ec == GPG_ERR_INV_OP)
        {
          /* libgcrypt is old, no support of the FIPS service indicator.  */
          fail ("gcry_pk_hash test %d unexpectedly failed to check the FIPS service indicator.\n",
                tvidx);
          goto next;
        }

      if (in_fips_mode && !tv[tvidx].expect_failure_hash && ec)
        {
          /* Success with the FIPS service indicator == 0 expected, but != 0.  */
          fail ("gcry_pk_hash test %d unexpectedly set the indicator in FIPS mode.\n",
                tvidx);
          goto next;
        }
      else if (in_fips_mode && tv[tvidx].expect_failure_hash && !ec)
        {
          /* Success with the FIPS service indicator != 0 expected, but == 0.  */
          fail ("gcry_pk_hash test %d unexpectedly cleared the indicator in FIPS mode.\n",
                tvidx);
          goto next;
        }

      err = gcry_sexp_build (&s_sk, NULL, tv[tvidx].prvkey);
      if (err)
        {
          fail ("error building SEXP for test, %s: %s",
                "sk", gpg_strerror (err));
          goto next;
        }

      err = gcry_sexp_build (&s_pk, NULL, tv[tvidx].pubkey);
      if (err)
        {
          fail ("error building SEXP for test, %s: %s",
                "pk", gpg_strerror (err));
          goto next;
        }

      if (!(buffer = hex2buffer (tv[tvidx].k, &buflen)))
        {
          fail ("error parsing for test, %s: %s",
                "msg", "invalid hex string");
          goto next;
        }

      err = gcry_pk_random_override_new (&ctx, buffer, buflen);
      if (err)
        {
          fail ("error setting 'k' for test: %s",
                gpg_strerror (err));
          goto next;
        }

      gcry_md_write (hd, msg, msglen);

      err = gcry_pk_hash_sign (&s_sig, tv[tvidx].data_tmpl, s_sk, hd, ctx);
      if (err)
        {
          fail ("gcry_pk_hash_sign failed: %s", gpg_strerror (err));
          goto next;
        }

      ec = gcry_get_fips_service_indicator ();
      if (ec == GPG_ERR_INV_OP)
        {
          /* libgcrypt is old, no support of the FIPS service indicator.  */
          fail ("gcry_pk_hash_sign test %d unexpectedly failed to check the FIPS service indicator.\n",
                tvidx);
          goto next;
        }

      if (in_fips_mode && !tv[tvidx].expect_failure && ec)
        {
          /* Success with the FIPS service indicator == 0 expected, but != 0.  */
          fail ("gcry_pk_hash_sign test %d unexpectedly set the indicator in FIPS mode.\n",
                tvidx);
          goto next;
        }
      else if (in_fips_mode && tv[tvidx].expect_failure && !ec)
        {
          /* Success with the FIPS service indicator != 0 expected, but == 0.  */
          fail ("gcry_pk_hash_sign test %d unexpectedly cleared the indicator in FIPS mode.\n",
                tvidx);
          goto next;
        }

      err = gcry_pk_hash_verify (s_sig, tv[tvidx].data_tmpl, s_pk, hd, ctx);
      if (err)
        {
          fail ("gcry_pk_hash_verify failed for test: %s",
                gpg_strerror (err));
          goto next;
        }

      ec = gcry_get_fips_service_indicator ();
      if (ec == GPG_ERR_INV_OP)
        {
          /* libgcrypt is old, no support of the FIPS service indicator.  */
          fail ("gcry_pk_hash_verify test %d unexpectedly failed to check the FIPS service indicator.\n",
                tvidx);
          goto next;
        }

      if (in_fips_mode && !tv[tvidx].expect_failure && ec)
        {
          /* Success with the FIPS service indicator == 0 expected, but != 0.  */
          fail ("gcry_pk_hash_verify test %d unexpectedly set the indicator in FIPS mode.\n",
                tvidx);
          goto next;
        }
      else if (in_fips_mode && tv[tvidx].expect_failure && !ec)
        {
          /* Success with the FIPS service indicator != 0 expected, but == 0.  */
          fail ("gcry_pk_hash_verify test %d unexpectedly cleared the indicator in FIPS mode.\n",
                tvidx);
          goto next;
        }

    next:
      gcry_sexp_release (s_sig);
      xfree (buffer);
      gcry_ctx_release (ctx);
      gcry_sexp_release (s_pk);
      gcry_sexp_release (s_sk);
      if (hd)
        gcry_md_close (hd);
    }
}

/* Check gcry_cipher_open, gcry_cipher_setkey, gcry_cipher_encrypt,
   gcry_cipher_decrypt, gcry_cipher_close API.  */
static void
check_cipher_o_s_e_d_c (int reject)
{
  static struct {
    int algo;
    int mode;
    const char *key;
    int keylen;
    const char *tag;
    int taglen;
    const char *expect;
    int expect_failure;
  } tv[] = {
#if USE_DES
   { GCRY_CIPHER_3DES, GCRY_CIPHER_MODE_ECB,
	 "\xe3\x34\x7a\x6b\x0b\xc1\x15\x2c\x64\x2a\x25\xcb\xd3\xbc\x31\xab"
	 "\xfb\xa1\x62\xa8\x1f\x19\x7c\x15", 24,
     "", -1,
     "\x3f\x1a\xb8\x83\x18\x8b\xb5\x97", 1 },
#endif
   { GCRY_CIPHER_AES, GCRY_CIPHER_MODE_ECB,
	 "\x2b\x7e\x15\x16\x28\xae\xd2\xa6\xab\xf7\x15\x88\x09\xcf\x4f\x3c", 16,
     "", -1,
     "\x5c\x71\xd8\x5d\x26\x5e\xcd\xb5\x95\x40\x41\xab\xff\x25\x6f\xd1" },
   { GCRY_CIPHER_AES128, GCRY_CIPHER_MODE_SIV,
	 "\xff\xfe\xfd\xfc\xfb\xfa\xf9\xf8\xf7\xf6\xf5\xf4\xf3\xf2\xf1\xf0"
	 "\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff", 32,
     "\x51\x66\x54\xc4\xe1\xb5\xd9\x37\x31\x52\xdb\xea\x35\x10\x8b\x7b", 16,
     "\x83\x69\xf6\xf3\x20\xff\xa2\x72\x31\x67\x15\xcf\xf4\x75\x01\x9a", 1 }
  };

  const char *pt = "Shohei Ohtani 2024: 54 HR, 59 SB";
  int ptlen;
  int tvidx;
  unsigned char out[MAX_DATA_LEN];
  gpg_error_t err;

  unsigned char tag[16];
  size_t taglen = 0;

  ptlen = strlen (pt);
  assert (ptlen == 32);
  for (tvidx=0; tvidx < DIM(tv); tvidx++)
    {
      gpg_err_code_t ec;
      gcry_cipher_hd_t h;
      size_t blklen;

      if (verbose)
        fprintf (stderr, "checking gcry_cipher_open test %d\n",
                 tvidx);

      blklen = gcry_cipher_get_algo_blklen (tv[tvidx].algo);

      assert (blklen != 0);
      assert (blklen <= ptlen);
      assert (blklen <= DIM (out));
      assert (tv[tvidx].taglen <= 16);
      err = gcry_cipher_open (&h, tv[tvidx].algo, tv[tvidx].mode, 0);
      if (err)
        {
          if (in_fips_mode && reject && tv[tvidx].expect_failure)
            /* Here, an error is expected */
            ;
          else
            fail ("gcry_cipher_open test %d unexpectedly failed: %s\n",
                  tvidx, gpg_strerror (err));
          continue;
        }
      else
        {
          if (in_fips_mode && reject && tv[tvidx].expect_failure)
            /* This case, an error is expected, but we observed success */
            fail ("gcry_cipher_open test %d unexpectedly succeeded\n", tvidx);
        }

      ec = gcry_get_fips_service_indicator ();
      if (ec == GPG_ERR_INV_OP)
        {
          /* libgcrypt is old, no support of the FIPS service indicator.  */
          fail ("gcry_cipher_open test %d unexpectedly failed to check the FIPS service indicator.\n",
                tvidx);
          continue;
        }

      if (in_fips_mode && !tv[tvidx].expect_failure && ec)
        {
          /* Success with the FIPS service indicator == 0 expected, but != 0.  */
          fail ("gcry_cipher_open test %d unexpectedly set the indicator in FIPS mode.\n",
                tvidx);
          continue;
        }
      else if (in_fips_mode && tv[tvidx].expect_failure && !ec)
        {
          /* Success with the FIPS service indicator != 0 expected, but == 0.  */
          fail ("gcry_cipher_open test %d unexpectedly cleared the indicator in FIPS mode.\n",
                tvidx);
          continue;
        }

      err = gcry_cipher_setkey (h, tv[tvidx].key, tv[tvidx].keylen);
      if (err)
        {
          fail ("gcry_cipher_setkey %d failed: %s\n", tvidx,
                gpg_strerror (err));
          gcry_cipher_close (h);
          continue;
        }

      if (tv[tvidx].taglen >= 0)
        {
          err = gcry_cipher_info (h, GCRYCTL_GET_TAGLEN, NULL, &taglen);
          if (err)
              fail ("gcry_cipher_info %d failed: %s\n", tvidx,
                    gpg_strerror (err));

          if (taglen != tv[tvidx].taglen)
              fail ("gcry_cipher_info %d failed: taglen mismatch %d != %zu\n", tvidx,
                    tv[tvidx].taglen, taglen);
        }

      err = gcry_cipher_encrypt (h, out, MAX_DATA_LEN, pt, blklen);
      if (err)
        {
          fail ("gcry_cipher_encrypt %d failed: %s\n", tvidx,
                gpg_strerror (err));
          gcry_cipher_close (h);
          continue;
        }

      if (memcmp (out, tv[tvidx].expect, blklen))
        {
          int i;

          fail ("gcry_cipher_open test %d failed: encryption mismatch\n", tvidx);
          fputs ("got:", stderr);
          for (i=0; i < blklen; i++)
            fprintf (stderr, " %02x", out[i]);
          putc ('\n', stderr);
        }

      if (tv[tvidx].taglen >= 0)
        {
           err = gcry_cipher_gettag (h, tag, tv[tvidx].taglen);
           if (err)
              fail ("gcry_cipher_gettag %d failed: %s", tvidx,
                     gpg_strerror(err));

          if (memcmp (tv[tvidx].tag, tag, tv[tvidx].taglen))
            {
              int i;

              fail ("gcry_cipher_gettag %d: tag mismatch\n", tvidx);
              fputs ("got:", stderr);
              for (i=0; i < 16 ; i++)
                fprintf (stderr, " %02x", tag[i]);
              putc ('\n', stderr);
            }

          err = gcry_cipher_reset (h);
          if (err)
            fail("gcry_cipher_reset %d failed: %s", tvidx,
                  gpg_strerror(err));

          err = gcry_cipher_set_decryption_tag (h, tag, 16);
          if (err)
            fail ("gcry_cipher_set_decryption_tag %d failed: %s\n", tvidx,
                   gpg_strerror (err));
      }

      err = gcry_cipher_decrypt (h, out, blklen, NULL, 0);
      if (err)
        {
          fail ("gcry_cipher_decrypt %d failed: %s\n", tvidx,
                gpg_strerror (err));
          gcry_cipher_close (h);
          continue;
        }

      if (memcmp (out, pt, blklen))
        {
          int i;

          fail ("gcry_cipher_open test %d failed: decryption mismatch\n", tvidx);
          fputs ("got:", stderr);
          for (i=0; i < blklen; i++)
            fprintf (stderr, " %02x", out[i]);
          putc ('\n', stderr);
        }

      gcry_cipher_close (h);
    }
}

/* Check gcry_mac_open, gcry_mac_write, gcry_mac_write, gcry_mac_read,
   gcry_mac_close API.  */
static void
check_mac_o_w_r_c (int reject)
{
  static struct {
    int algo;
    const char *data;
    int datalen;
    const char *key;
    int keylen;
    const char *expect;
    int expect_failure;
  } tv[] = {
#if USE_MD5
    { GCRY_MAC_HMAC_MD5, "hmac input abc", 14, "hmac key input", 14,
      "\x0d\x72\xd0\x60\xaf\x34\xf2\xca\x33\x58\xa9\xcc\xd3\x5a\xac\xb5", 1 },
#endif
#if USE_SHA1
    { GCRY_MAC_HMAC_SHA1, "hmac input abc", 14, "hmac key input", 14,
      "\xc9\x62\x9d\x16\x0f\xc2\xc4\xcd\x38\xac\x3a\x00\xdc\x29\x61\x03"
      "\x69\x50\xd7\x3a", 1 },
#endif
    { GCRY_MAC_HMAC_SHA256, "hmac input abc", 14, "hmac key input", 14,
      "\x6a\xda\x4d\xd5\xf3\xa7\x32\x9d\xd2\x55\xc0\x7f\xe6\x0a\x93\xb8"
      "\x7a\x6e\x76\x68\x46\x34\x67\xf9\xc2\x29\xb8\x24\x2e\xc8\xe3\xb4" },
    { GCRY_MAC_HMAC_SHA384, "hmac input abc", 14, "hmac key input", 14,
      "\xc6\x59\x14\x4a\xac\x4d\xd5\x62\x09\x2c\xbd\x5e\xbf\x41\x94\xf9"
      "\xa4\x78\x18\x46\xfa\xd6\xd1\x12\x90\x4f\x65\xd4\xe8\x44\xcc\xcc"
      "\x3d\xcc\xf3\xe4\x27\xd8\xf0\xff\x01\xe8\x70\xcd\xfb\xfa\x24\x45" },
    { GCRY_MAC_HMAC_SHA512, "hmac input abc", 14, "hmac key input", 14,
      "\xfa\x77\x49\x49\x24\x3d\x7e\x03\x1b\x0e\xd1\xfc\x20\x81\xcf\x95"
      "\x81\x21\xa4\x4f\x3b\xe5\x69\x9a\xe6\x67\x27\x10\xbc\x62\xc7\xb3"
      "\xb3\xcf\x2b\x1e\xda\x20\x48\x25\xc5\x6a\x52\xc7\xc9\xd9\x77\xf6"
      "\xf6\x49\x9d\x70\xe6\x04\x33\xab\x6a\xdf\x7e\x9f\xf4\xd1\x59\x6e" },
    { GCRY_MAC_HMAC_SHA3_256, "hmac input abc", 14, "hmac key input", 14,
      "\x2b\xe9\x02\x92\xc2\x37\xbe\x91\x06\xbf\x9c\x8e\x7b\xa3\xf2\xfc"
      "\x68\x10\x8a\x71\xd5\xc7\x84\x3c\x0b\xdd\x7d\x1e\xdf\xa5\xf6\xa7" },
    { GCRY_MAC_HMAC_SHA3_384, "hmac input abc", 14, "hmac key input", 14,
      "\x9f\x6b\x9f\x49\x95\x57\xed\x33\xb1\xe7\x22\x2f\xda\x40\x68\xb0"
      "\x28\xd2\xdb\x6f\x73\x3c\x2e\x2b\x29\x51\x64\x53\xc4\xc5\x63\x8a"
      "\x98\xca\x78\x1a\xe7\x1b\x7d\xf6\xbf\xf3\x6a\xf3\x2a\x0e\xa0\x5b" },
    { GCRY_MAC_HMAC_SHA3_512, "hmac input abc", 14, "hmac key input", 14,
      "\xf3\x19\x70\x54\x25\xdf\x0f\xde\x09\xe9\xea\x3b\x34\x67\x14\x32"
      "\xe6\xe2\x58\x9d\x76\x38\xa4\xbd\x90\x35\x4c\x07\x7c\xa3\xdb\x23"
      "\x3c\x78\x0c\x45\xee\x8e\x39\xd5\x81\xd8\x5c\x13\x20\x40\xba\x34"
      "\xd0\x0b\x75\x31\x38\x4b\xe7\x74\x87\xa9\xc5\x68\x7f\xbc\x19\xa1" }
#if USE_RMD160
    ,
    { GCRY_MAC_HMAC_RMD160, "hmac input abc", 14, "hmac key input", 14,
      "\xf2\x45\x5c\x7e\x48\x1a\xbb\xe5\xe8\xec\x40\xa4\x1b\x89\x26\x2b"
      "\xdc\xa1\x79\x59", 1 }
#endif
  };
  int tvidx;
  unsigned char mac[64];
  int expectlen;
  gpg_error_t err;
  size_t buflen;

  for (tvidx=0; tvidx < DIM(tv); tvidx++)
    {
      gpg_err_code_t ec;
      gcry_mac_hd_t h;

      if (verbose)
        fprintf (stderr, "checking gcry_mac_open test %d\n",
                 tvidx);

      expectlen = gcry_mac_get_algo_maclen (tv[tvidx].algo);
      assert (expectlen != 0);
      assert (expectlen <= DIM (mac));
      err = gcry_mac_open (&h, tv[tvidx].algo, 0, NULL);
      if (err)
        {
          if (in_fips_mode && reject && tv[tvidx].expect_failure)
            /* Here, an error is expected */
            ;
          else
            fail ("gcry_mac_open test %d unexpectedly failed: %s\n",
                  tvidx, gpg_strerror (err));
          continue;
        }
      else
        {
          if (in_fips_mode && reject && tv[tvidx].expect_failure)
            /* This case, an error is expected, but we observed success */
            fail ("gcry_mac_open test %d unexpectedly succeeded\n", tvidx);
        }


      ec = gcry_get_fips_service_indicator ();
      if (ec == GPG_ERR_INV_OP)
        {
          /* libgcrypt is old, no support of the FIPS service indicator.  */
          fail ("gcry_mac_open test %d unexpectedly failed to check the FIPS service indicator.\n",
                tvidx);
          continue;
        }

      if (in_fips_mode && !tv[tvidx].expect_failure && ec)
        {
          /* Success with the FIPS service indicator == 0 expected, but != 0.  */
          fail ("gcry_mac_open test %d unexpectedly set the indicator in FIPS mode.\n",
                tvidx);
          continue;
        }
      else if (in_fips_mode && tv[tvidx].expect_failure && !ec)
        {
          /* Success with the FIPS service indicator != 0 expected, but == 0.  */
          fail ("gcry_mac_open test %d unexpectedly cleared the indicator in FIPS mode.\n",
                tvidx);
          continue;
        }

      err = gcry_mac_setkey (h, tv[tvidx].key, tv[tvidx].keylen);
      if (err)
        {
          fail ("gcry_mac_setkey test %d unexpectedly failed: %s\n",
                tvidx, gpg_strerror (err));
          gcry_mac_close (h);
          continue;
        }

      err = gcry_mac_write (h, tv[tvidx].data, tv[tvidx].datalen);
      if (err)
        {
          fail ("gcry_mac_write test %d unexpectedly failed: %s\n",
                tvidx, gpg_strerror (err));
          gcry_mac_close (h);
          continue;
        }

      buflen = expectlen;
      err = gcry_mac_read (h, mac, &buflen);
      if (err || buflen != expectlen)
        {
          fail ("gcry_mac_read test %d unexpectedly failed: %s\n",
                tvidx, gpg_strerror (err));
          gcry_mac_close (h);
          continue;
        }

      if (memcmp (mac, tv[tvidx].expect, expectlen))
        {
          int i;

          fail ("gcry_mac_open test %d failed: mismatch\n", tvidx);
          fputs ("got:", stderr);
          for (i=0; i < expectlen; i++)
            fprintf (stderr, " %02x", mac[i]);
          putc ('\n', stderr);
        }

      gcry_mac_close (h);
    }
}


/* Check gcry_md_open, gcry_md_write, gcry_md_write, gcry_md_read,
   gcry_md_close API.  */
static void
check_md_o_w_r_c (int reject)
{
  static struct {
    int algo;
    const char *data;
    int datalen;
    const char *expect;
    int expect_failure;
  } tv[] = {
#if USE_MD5
    { GCRY_MD_MD5, "abc", 3,
      "\x90\x01\x50\x98\x3C\xD2\x4F\xB0\xD6\x96\x3F\x7D\x28\xE1\x7F\x72", 1 },
#endif
#if USE_SHA1
    { GCRY_MD_SHA1, "abc", 3,
      "\xA9\x99\x3E\x36\x47\x06\x81\x6A\xBA\x3E"
      "\x25\x71\x78\x50\xC2\x6C\x9C\xD0\xD8\x9D", 1 },
#endif
    { GCRY_MD_SHA256, "abc", 3,
      "\xba\x78\x16\xbf\x8f\x01\xcf\xea\x41\x41\x40\xde\x5d\xae\x22\x23"
      "\xb0\x03\x61\xa3\x96\x17\x7a\x9c\xb4\x10\xff\x61\xf2\x00\x15\xad" },
    { GCRY_MD_SHA384, "abc", 3,
      "\xcb\x00\x75\x3f\x45\xa3\x5e\x8b\xb5\xa0\x3d\x69\x9a\xc6\x50\x07"
      "\x27\x2c\x32\xab\x0e\xde\xd1\x63\x1a\x8b\x60\x5a\x43\xff\x5b\xed"
      "\x80\x86\x07\x2b\xa1\xe7\xcc\x23\x58\xba\xec\xa1\x34\xc8\x25\xa7" },
    { GCRY_MD_SHA512, "abc", 3,
      "\xDD\xAF\x35\xA1\x93\x61\x7A\xBA\xCC\x41\x73\x49\xAE\x20\x41\x31"
      "\x12\xE6\xFA\x4E\x89\xA9\x7E\xA2\x0A\x9E\xEE\xE6\x4B\x55\xD3\x9A"
      "\x21\x92\x99\x2A\x27\x4F\xC1\xA8\x36\xBA\x3C\x23\xA3\xFE\xEB\xBD"
      "\x45\x4D\x44\x23\x64\x3C\xE8\x0E\x2A\x9A\xC9\x4F\xA5\x4C\xA4\x9F" },
    { GCRY_MD_SHA3_256, "abc", 3,
      "\x3a\x98\x5d\xa7\x4f\xe2\x25\xb2\x04\x5c\x17\x2d\x6b\xd3\x90\xbd"
      "\x85\x5f\x08\x6e\x3e\x9d\x52\x5b\x46\xbf\xe2\x45\x11\x43\x15\x32" },
    { GCRY_MD_SHA3_384, "abc", 3,
      "\xec\x01\x49\x82\x88\x51\x6f\xc9\x26\x45\x9f\x58\xe2\xc6\xad\x8d"
      "\xf9\xb4\x73\xcb\x0f\xc0\x8c\x25\x96\xda\x7c\xf0\xe4\x9b\xe4\xb2"
      "\x98\xd8\x8c\xea\x92\x7a\xc7\xf5\x39\xf1\xed\xf2\x28\x37\x6d\x25" },
    { GCRY_MD_SHA3_512, "abc", 3,
      "\xb7\x51\x85\x0b\x1a\x57\x16\x8a\x56\x93\xcd\x92\x4b\x6b\x09\x6e"
      "\x08\xf6\x21\x82\x74\x44\xf7\x0d\x88\x4f\x5d\x02\x40\xd2\x71\x2e"
      "\x10\xe1\x16\xe9\x19\x2a\xf3\xc9\x1a\x7e\xc5\x76\x47\xe3\x93\x40"
      "\x57\x34\x0b\x4c\xf4\x08\xd5\xa5\x65\x92\xf8\x27\x4e\xec\x53\xf0" }
#if USE_RMD160
    ,
    { GCRY_MD_RMD160, "abc", 3,
      "\x8e\xb2\x08\xf7\xe0\x5d\x98\x7a\x9b\x04"
      "\x4a\x8e\x98\xc6\xb0\x87\xf1\x5a\x0b\xfc", 1 }
#endif
  };
  int tvidx;
  unsigned char *hash;
  int expectlen;
  gpg_error_t err;

  for (tvidx=0; tvidx < DIM(tv); tvidx++)
    {
      gpg_err_code_t ec;
      gcry_md_hd_t h;

      if (verbose)
        fprintf (stderr, "checking gcry_md_open test %d\n",
                 tvidx);

      expectlen = gcry_md_get_algo_dlen (tv[tvidx].algo);
      assert (expectlen != 0);
      err = gcry_md_open (&h, tv[tvidx].algo, 0);
      if (err)
        {
          if (in_fips_mode && reject && tv[tvidx].expect_failure)
            /* Here, an error is expected */
            ;
          else
            fail ("gcry_md_open test %d unexpectedly failed: %s\n",
                  tvidx, gpg_strerror (err));
          continue;
        }
      else
        {
          if (in_fips_mode && reject && tv[tvidx].expect_failure)
            /* This case, an error is expected, but we observed success */
            fail ("gcry_md_open test %d unexpectedly succeeded\n", tvidx);
        }


      ec = gcry_get_fips_service_indicator ();
      if (ec == GPG_ERR_INV_OP)
        {
          /* libgcrypt is old, no support of the FIPS service indicator.  */
          fail ("gcry_md_open test %d unexpectedly failed to check the FIPS service indicator.\n",
                tvidx);
          continue;
        }

      if (in_fips_mode && !tv[tvidx].expect_failure && ec)
        {
          /* Success with the FIPS service indicator == 0 expected, but != 0.  */
          fail ("gcry_md_open test %d unexpectedly set the indicator in FIPS mode.\n",
                tvidx);
          continue;
        }
      else if (in_fips_mode && tv[tvidx].expect_failure && !ec)
        {
          /* Success with the FIPS service indicator != 0 expected, but == 0.  */
          fail ("gcry_md_open test %d unexpectedly cleared the indicator in FIPS mode.\n",
                tvidx);
          continue;
        }

      gcry_md_write (h, tv[tvidx].data, tv[tvidx].datalen);
      hash = gcry_md_read (h, tv[tvidx].algo);
      if (memcmp (hash, tv[tvidx].expect, expectlen))
        {
          int i;

          fail ("gcry_md_open test %d failed: mismatch\n", tvidx);
          fputs ("got:", stderr);
          for (i=0; i < expectlen; i++)
            fprintf (stderr, " %02x", hash[i]);
          putc ('\n', stderr);
        }

      gcry_md_close (h);
    }
}

static void
check_hash_buffer (void)
{
  static struct {
    int algo;
    const char *data;
    int datalen;
    const char *expect;
    int expect_failure;
  } tv[] = {
#if USE_MD5
    { GCRY_MD_MD5, "abc", 3,
      "\x90\x01\x50\x98\x3C\xD2\x4F\xB0\xD6\x96\x3F\x7D\x28\xE1\x7F\x72", 1 },
#endif
#if USE_SHA1
    { GCRY_MD_SHA1, "abc", 3,
      "\xA9\x99\x3E\x36\x47\x06\x81\x6A\xBA\x3E"
      "\x25\x71\x78\x50\xC2\x6C\x9C\xD0\xD8\x9D", 1 },
#endif
    { GCRY_MD_SHA256, "abc", 3,
      "\xba\x78\x16\xbf\x8f\x01\xcf\xea\x41\x41\x40\xde\x5d\xae\x22\x23"
      "\xb0\x03\x61\xa3\x96\x17\x7a\x9c\xb4\x10\xff\x61\xf2\x00\x15\xad" },
    { GCRY_MD_SHA384, "abc", 3,
      "\xcb\x00\x75\x3f\x45\xa3\x5e\x8b\xb5\xa0\x3d\x69\x9a\xc6\x50\x07"
      "\x27\x2c\x32\xab\x0e\xde\xd1\x63\x1a\x8b\x60\x5a\x43\xff\x5b\xed"
      "\x80\x86\x07\x2b\xa1\xe7\xcc\x23\x58\xba\xec\xa1\x34\xc8\x25\xa7" },
    { GCRY_MD_SHA512, "abc", 3,
      "\xDD\xAF\x35\xA1\x93\x61\x7A\xBA\xCC\x41\x73\x49\xAE\x20\x41\x31"
      "\x12\xE6\xFA\x4E\x89\xA9\x7E\xA2\x0A\x9E\xEE\xE6\x4B\x55\xD3\x9A"
      "\x21\x92\x99\x2A\x27\x4F\xC1\xA8\x36\xBA\x3C\x23\xA3\xFE\xEB\xBD"
      "\x45\x4D\x44\x23\x64\x3C\xE8\x0E\x2A\x9A\xC9\x4F\xA5\x4C\xA4\x9F" },
    { GCRY_MD_SHA3_256, "abc", 3,
      "\x3a\x98\x5d\xa7\x4f\xe2\x25\xb2\x04\x5c\x17\x2d\x6b\xd3\x90\xbd"
      "\x85\x5f\x08\x6e\x3e\x9d\x52\x5b\x46\xbf\xe2\x45\x11\x43\x15\x32" },
    { GCRY_MD_SHA3_384, "abc", 3,
      "\xec\x01\x49\x82\x88\x51\x6f\xc9\x26\x45\x9f\x58\xe2\xc6\xad\x8d"
      "\xf9\xb4\x73\xcb\x0f\xc0\x8c\x25\x96\xda\x7c\xf0\xe4\x9b\xe4\xb2"
      "\x98\xd8\x8c\xea\x92\x7a\xc7\xf5\x39\xf1\xed\xf2\x28\x37\x6d\x25" },
    { GCRY_MD_SHA3_512, "abc", 3,
      "\xb7\x51\x85\x0b\x1a\x57\x16\x8a\x56\x93\xcd\x92\x4b\x6b\x09\x6e"
      "\x08\xf6\x21\x82\x74\x44\xf7\x0d\x88\x4f\x5d\x02\x40\xd2\x71\x2e"
      "\x10\xe1\x16\xe9\x19\x2a\xf3\xc9\x1a\x7e\xc5\x76\x47\xe3\x93\x40"
      "\x57\x34\x0b\x4c\xf4\x08\xd5\xa5\x65\x92\xf8\x27\x4e\xec\x53\xf0" }
#if USE_RMD160
    ,
    { GCRY_MD_RMD160, "abc", 3,
      "\x8e\xb2\x08\xf7\xe0\x5d\x98\x7a\x9b\x04"
      "\x4a\x8e\x98\xc6\xb0\x87\xf1\x5a\x0b\xfc", 1 }
#endif
  };
  int tvidx;
  unsigned char hash[64];
  int expectlen;

  for (tvidx=0; tvidx < DIM(tv); tvidx++)
    {
      gpg_err_code_t ec;

      if (verbose)
        fprintf (stderr, "checking gcry_md_hash_buffer test %d\n",
                 tvidx);

      expectlen = gcry_md_get_algo_dlen (tv[tvidx].algo);
      assert (expectlen != 0);
      assert (expectlen <= sizeof hash);
      gcry_md_hash_buffer (tv[tvidx].algo, hash,
                           tv[tvidx].data, tv[tvidx].datalen);

      ec = gcry_get_fips_service_indicator ();
      if (ec == GPG_ERR_INV_OP)
        {
          /* libgcrypt is old, no support of the FIPS service indicator.  */
          fail ("gcry_md_hash_buffer test %d unexpectedly failed to check the FIPS service indicator.\n",
                tvidx);
          continue;
        }

      if (in_fips_mode && !tv[tvidx].expect_failure && ec)
        {
          /* Success with the FIPS service indicator == 0 expected, but != 0.  */
          fail ("gcry_md_hash_buffer test %d unexpectedly set the indicator in FIPS mode.\n",
                tvidx);
          continue;
        }
      else if (in_fips_mode && tv[tvidx].expect_failure && !ec)
        {
          /* Success with the FIPS service indicator != 0 expected, but == 0.  */
          fail ("gcry_md_hash_buffer test %d unexpectedly cleared the indicator in FIPS mode.\n",
                tvidx);
          continue;
        }

      if (memcmp (hash, tv[tvidx].expect, expectlen))
        {
          int i;

          fail ("gcry_md_hash_buffer test %d failed: mismatch\n", tvidx);
          fputs ("got:", stderr);
          for (i=0; i < expectlen; i++)
            fprintf (stderr, " %02x", hash[i]);
          putc ('\n', stderr);
        }
    }
}

static void
check_hash_buffers (void)
{
  static struct {
    int algo;
    const char *data;
    int datalen;
    const char *key;
    int keylen;
    const char *expect;
    int expect_failure;
  } tv[] = {
#if USE_MD5
    { GCRY_MD_MD5, "abc", 3,
      "key", 3,
      "\xd2\xfe\x98\x06\x3f\x87\x6b\x03\x19\x3a\xfb\x49\xb4\x97\x95\x91", 1 },
#endif
#if USE_SHA1
    { GCRY_MD_SHA1, "abc", 3,
      "key", 3,
      "\x4f\xd0\xb2\x15\x27\x6e\xf1\x2f\x2b\x3e"
      "\x4c\x8e\xca\xc2\x81\x14\x98\xb6\x56\xfc", 1 },
#endif
    { GCRY_MD_SHA256, "abc", 3,
      "key", 3,
      "\x9c\x19\x6e\x32\xdc\x01\x75\xf8\x6f\x4b\x1c\xb8\x92\x89\xd6\x61"
      "\x9d\xe6\xbe\xe6\x99\xe4\xc3\x78\xe6\x83\x09\xed\x97\xa1\xa6\xab" },
    { GCRY_MD_SHA384, "abc", 3,
      "key", 3,
      "\x30\xdd\xb9\xc8\xf3\x47\xcf\xfb\xfb\x44\xe5\x19\xd8\x14\xf0\x74"
      "\xcf\x40\x47\xa5\x5d\x6f\x56\x33\x24\xf1\xc6\xa3\x39\x20\xe5\xed"
      "\xfb\x2a\x34\xba\xc6\x0b\xdc\x96\xcd\x33\xa9\x56\x23\xd7\xd6\x38" },
    { GCRY_MD_SHA512, "abc", 3,
      "key", 3,
      "\x39\x26\xa2\x07\xc8\xc4\x2b\x0c\x41\x79\x2c\xbd\x3e\x1a\x1a\xaa"
      "\xf5\xf7\xa2\x57\x04\xf6\x2d\xfc\x93\x9c\x49\x87\xdd\x7c\xe0\x60"
      "\x00\x9c\x5b\xb1\xc2\x44\x73\x55\xb3\x21\x6f\x10\xb5\x37\xe9\xaf"
      "\xa7\xb6\x4a\x4e\x53\x91\xb0\xd6\x31\x17\x2d\x07\x93\x9e\x08\x7a" },
    { GCRY_MD_SHA3_256, "abc", 3,
      "key", 3,
      "\x09\xb6\xdb\xab\x8d\x11\x79\x5c\xa7\xc8\xd8\x2f\x1c\xf9\x16\x82"
      "\x01\x3c\x7c\xb9\x80\xab\xbb\x25\x47\x3b\xe4\xae\x7f\x7b\x56\x83" },
    { GCRY_MD_SHA3_384, "abc", 3,
      "key", 3,
      "\x94\xf2\xaa\x7a\xe7\xc4\xb7\xb8\xfa\x4c\x61\x2f\xdb\x42\x2b\x33"
      "\x43\x81\x1b\x13\xc8\x88\x82\x57\x90\x4f\x54\x39\x95\xcd\xbc\xba"
      "\x5e\x49\xf1\x0f\x8e\xd6\xf7\xb9\xdd\xc1\xb3\x0b\x38\x28\x81\x5c" },
    { GCRY_MD_SHA3_512, "abc", 3,
      "key", 3,
      "\x08\x5e\x4e\x83\x50\x3f\x40\xb8\x2f\xef\x38\x43\x8b\xc4\x90\x5a"
      "\x55\xdb\xaa\x8c\x88\x78\x09\x7a\x89\x9d\xb0\xb5\x7c\xe7\xda\x57"
      "\xa3\x68\x25\x1c\x34\x47\x4f\x60\xb3\xeb\xac\xb3\x9b\x2e\xda\xca"
      "\x4b\x29\x04\x56\x41\x1c\x76\xec\x7a\xb6\x19\x44\xcf\xe2\x28\x8e" }
#if USE_RMD160
    ,
    { GCRY_MD_RMD160, "abc", 3,
      "key", 3,
      "\x67\xfd\xce\x73\x8e\xbf\xc7\x37\x2b\xcd"
      "\x38\xf0\x3c\x02\x3b\x57\x46\x72\x4d\x18", 1 }
#endif
  };
  int tvidx;
  unsigned char hash[64];
  int expectlen;
  gcry_buffer_t iov[2];
  gpg_error_t err;

  for (tvidx=0; tvidx < DIM(tv); tvidx++)
    {
      gpg_err_code_t ec;

      if (verbose)
        fprintf (stderr, "checking gcry_md_hash_buffers test %d\n",
                 tvidx);

      expectlen = gcry_md_get_algo_dlen (tv[tvidx].algo);
      assert (expectlen != 0);
      assert (expectlen <= sizeof hash);
      memset (iov, 0, sizeof iov);
      iov[0].data = (void *)tv[tvidx].key;
      iov[0].len = tv[tvidx].keylen;
      iov[1].data = (void *)tv[tvidx].data;
      iov[1].len = tv[tvidx].datalen;
      err = gcry_md_hash_buffers (tv[tvidx].algo, GCRY_MD_FLAG_HMAC, hash,
                                  iov, 2);
      if (err)
        {
          fail ("gcry_md_hash_buffers test %d unexpectedly failed\n", tvidx);
          continue;
        }

      ec = gcry_get_fips_service_indicator ();
      if (ec == GPG_ERR_INV_OP)
        {
          /* libgcrypt is old, no support of the FIPS service indicator.  */
          fail ("gcry_md_hash_buffers test %d unexpectedly failed to check the FIPS service indicator.\n",
                tvidx);
          continue;
        }

      if (in_fips_mode && !tv[tvidx].expect_failure && ec)
        {
          /* Success with the FIPS service indicator == 0 expected, but != 0.  */
          fail ("gcry_md_hash_buffers test %d unexpectedly set the indicator in FIPS mode.\n",
                tvidx);
          continue;
        }
      else if (in_fips_mode && tv[tvidx].expect_failure && !ec)
        {
          /* Success with the FIPS service indicator != 0 expected, but == 0.  */
          fail ("gcry_md_hash_buffers test %d unexpectedly cleared the indicator in FIPS mode.\n",
                tvidx);
          continue;
        }

      if (memcmp (hash, tv[tvidx].expect, expectlen))
        {
          int i;

          fail ("gcry_md_hash_buffers test %d failed: mismatch\n", tvidx);
          fputs ("got:", stderr);
          for (i=0; i < expectlen; i++)
            fprintf (stderr, " %02x", hash[i]);
          putc ('\n', stderr);
        }
    }
}


static void
check_kdf_derive (void)
{
  static struct {
    const char *p;   /* Passphrase.  */
    size_t plen;     /* Length of P. */
    int algo;
    int subalgo;
    const char *salt;
    size_t saltlen;
    unsigned long iterations;
    int dklen;       /* Requested key length.  */
    const char *dk;  /* Derived key.  */
    int expect_failure;
  } tv[] = {
    {
      "passwordPASSWORDpassword", 24,
      GCRY_KDF_PBKDF2, GCRY_MD_SHA256,
      "saltSALTsaltSALTsaltSALTsaltSALTsalt", 36,
      4096,
      25,
      "\x34\x8c\x89\xdb\xcb\xd3\x2b\x2f\x32\xd8"
      "\x14\xb8\x11\x6e\x84\xcf\x2b\x17\x34\x7e"
      "\xbc\x18\x00\x18\x1c",
      0
    },
    {
      "pleaseletmein", 13,
      GCRY_KDF_SCRYPT, 16384,
      "SodiumChloride", 14,
      1,
      64,
      "\x70\x23\xbd\xcb\x3a\xfd\x73\x48\x46\x1c\x06\xcd\x81\xfd\x38\xeb"
      "\xfd\xa8\xfb\xba\x90\x4f\x8e\x3e\xa9\xb5\x43\xf6\x54\x5d\xa1\xf2"
      "\xd5\x43\x29\x55\x61\x3f\x0f\xcf\x62\xd4\x97\x05\x24\x2a\x9a\xf9"
      "\xe6\x1e\x85\xdc\x0d\x65\x1e\x40\xdf\xcf\x01\x7b\x45\x57\x58\x87",
      1 /* not-compliant because unallowed algo */
    },
    {
      "passwor", 7,
      GCRY_KDF_PBKDF2, GCRY_MD_SHA256,
      "saltSALTsaltSALTsaltSALTsaltSALTsalt", 36,
      4096,
      25,
      "\x2d\x72\xa9\xe5\x4e\x2f\x37\x6e\xe5\xe4"
      "\xf5\x55\x76\xb5\xaa\x49\x73\x01\x97\x1c"
      "\xad\x3a\x7c\xc4\xde",
      1 /* not-compliant because passphrase len is too small */
    },
    {
      "passwordPASSWORDpassword", 24,
      GCRY_KDF_PBKDF2, GCRY_MD_SHA256,
      "saltSALTsaltSAL", 15,
      4096,
      25,
      "\xf7\x55\xdd\x3c\x5e\xfb\x23\x06\xa7\x85"
      "\x94\xa7\x31\x12\x45\xcf\x5a\x4b\xdc\x09"
      "\xee\x65\x4b\x50\x3f",
      1 /* not-compliant because salt len is too small */
    },
    {
      "passwordPASSWORDpassword", 24,
      GCRY_KDF_PBKDF2, GCRY_MD_SHA256,
      "saltSALTsaltSALTsaltSALTsaltSALTsalt", 36,
      999,
      25,
      "\x09\x3e\x1a\xd8\x63\x30\x71\x9c\x17\xcf"
      "\xb0\x53\x3e\x1f\xc8\x51\x29\x71\x54\x28"
      "\x5d\xf7\x8e\x41\xaa",
      1 /* not-compliant because too few iterations */
    },
    {
      "passwordPASSWORDpassword", 24,
      GCRY_KDF_PBKDF2, GCRY_MD_SHA256,
      "saltSALTsaltSALTsaltSALTsaltSALTsalt", 36,
      4096,
      13,
      "\x34\x8c\x89\xdb\xcb\xd3\x2b\x2f\x32\xd8"
      "\x14\xb8\x11",
      1 /* not-compliant because key size too small */
    },
    {
      "passwordPASSWORDpassword", 24,
      GCRY_KDF_PBKDF2, GCRY_MD_BLAKE2B_512,
      "saltSALTsaltSALTsaltSALTsaltSALTsalt", 36,
      4096,
      60,
      "\xa4\x6b\x53\x35\xdb\xdd\xa3\xd2\x5d\x19\xbb\x11\xfe\xdd\xd9\x9e"
      "\x45\x2a\x7c\x34\x47\x41\x98\xca\x31\x74\xb6\x34\x22\xac\x83\xb0"
      "\x38\x6e\xf5\x93\x0f\xf5\x16\x46\x0b\x97\xdc\x6c\x27\x5b\xe7\x25"
      "\xc2\xcb\xec\x50\x02\xc6\x52\x8b\x34\x68\x53\x65",
      1 /* not-compliant because subalgo is not the one of approved */
    }
  };

  int tvidx;
  gpg_error_t err;
  unsigned char outbuf[100];
  int i;

  for (tvidx=0; tvidx < DIM(tv); tvidx++)
    {
      if (verbose)
        fprintf (stderr, "checking gcry_kdf_derive test vector %d algo %d for FIPS\n",
                 tvidx, tv[tvidx].algo);
      assert (tv[tvidx].dklen <= sizeof outbuf);
      err = gcry_kdf_derive (tv[tvidx].p, tv[tvidx].plen,
                             tv[tvidx].algo, tv[tvidx].subalgo,
                             tv[tvidx].salt, tv[tvidx].saltlen,
                             tv[tvidx].iterations, tv[tvidx].dklen, outbuf);

      if (err)
        {
          fail ("gcry_kdf_derive test %d unexpectedly returned an error in FIPS mode: %s\n",
                tvidx, gpg_strerror (err));
        }
      else
        {
          gpg_err_code_t ec;

          ec = gcry_get_fips_service_indicator ();
          if (ec == GPG_ERR_INV_OP)
            {
              /* libgcrypt is old, no support of the FIPS service indicator.  */
              fail ("gcry_kdf_derive test %d unexpectedly failed to check the FIPS service indicator.\n",
                    tvidx);
              continue;
            }

          if (!tv[tvidx].expect_failure && ec)
            {
              /* Success with the FIPS service indicator == 0 expected, but != 0.  */
              fail ("gcry_kdf_derive test %d unexpectedly set the indicator in FIPS mode.\n",
                    tvidx);
              continue;
            }
          else if (tv[tvidx].expect_failure && !ec && in_fips_mode)
            {
              /* Success with the FIPS service indicator != 0 expected, but == 0.  */
              fail ("gcry_kdf_derive test %d unexpectedly cleared the indicator in FIPS mode.\n",
                    tvidx);
              continue;
            }

          if (memcmp (outbuf, tv[tvidx].dk, tv[tvidx].dklen))
            {
              fail ("gcry_kdf_derive test %d failed: mismatch\n", tvidx);
              fputs ("got:", stderr);
              for (i=0; i < tv[tvidx].dklen; i++)
                fprintf (stderr, " %02x", outbuf[i]);
              putc ('\n', stderr);
            }
        }
    }
}


int
main (int argc, char **argv)
{
  int last_argc = -1;

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
                 "  --debug         flyswatter\n",
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
      else if (!strncmp (*argv, "--", 2))
        die ("unknown option '%s'", *argv);
    }

  if (!gcry_check_version (GCRYPT_VERSION))
    die ("version mismatch\n");

  if (gcry_fips_mode_active ())
    in_fips_mode = 1;

  if (!in_fips_mode)
    xgcry_control ((GCRYCTL_DISABLE_SECMEM, 0));

  xgcry_control ((GCRYCTL_INITIALIZATION_FINISHED, 0));
  if (debug)
    xgcry_control ((GCRYCTL_SET_DEBUG_FLAGS, 1u , 0));

  xgcry_control ((GCRYCTL_FIPS_REJECT_NON_FIPS, 0));

  check_kdf_derive ();
  check_hash_buffer ();
  check_hash_buffers ();
  check_md_o_w_r_c (0);
  check_mac_o_w_r_c (0);
  check_cipher_o_s_e_d_c (0);
  check_pk_hash_sign_verify ();
  check_pk_s_v (0);
  check_pk_g_t_n_c (0);

  xgcry_control ((GCRYCTL_FIPS_REJECT_NON_FIPS,
                  (GCRY_FIPS_FLAG_REJECT_MD_MD5
                   | GCRY_FIPS_FLAG_REJECT_CIPHER_MODE
                   | GCRY_FIPS_FLAG_REJECT_PK_MD
                   | GCRY_FIPS_FLAG_REJECT_PK_GOST_SM2
                   | GCRY_FIPS_FLAG_REJECT_MD_SHA1
                   | GCRY_FIPS_FLAG_REJECT_PK_ECC_K
                   | GCRY_FIPS_FLAG_REJECT_PK_FLAGS
                   | GCRY_FIPS_FLAG_REJECT_COMPAT110)));

  check_md_o_w_r_c (1);
  check_mac_o_w_r_c (1);
  check_cipher_o_s_e_d_c (1);
  check_pk_s_v (1);
  check_pk_g_t_n_c (1);

  return !!error_count;
}
