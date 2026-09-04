/* Test support for locale names in BCP 47 syntax.
   Copyright (C) 2024-2026 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2024.  */

#include <config.h>

#include "bcp47.h"

#include <string.h>

#include "macros.h"

static void
test_correspondence (const char *xpg, const char *bcp47)
{
  /* Test xpg_to_bcp47.  */
  {
    char buf[BCP47_MAX];
    memset (buf, 0x77, BCP47_MAX);

    xpg_to_bcp47 (buf, xpg);
    ASSERT (strcmp (buf, bcp47) == 0);
  }

  /* Test bcp47_to_xpg.  */
  {
    char buf[BCP47_MAX];
    memset (buf, 0x77, BCP47_MAX);

    bcp47_to_xpg (buf, bcp47, NULL);
    ASSERT (strcmp (buf, xpg) == 0);
  }
}

int
main ()
{
  /* Languages with a single script.  */

  test_correspondence ("de", "de");
  test_correspondence ("de_DE", "de-DE");
  test_correspondence ("de_AT", "de-AT");

  /* Languages with a script that depends on the territory.  */

  test_correspondence ("az_AZ", "az-Latn-AZ");
  test_correspondence ("az_AZ@cyrillic", "az-Cyrl-AZ");
  test_correspondence ("az_IR", "az-Arab-IR");

  test_correspondence ("ku_IQ", "ku-Arab-IQ");
  test_correspondence ("ku_IR", "ku-Arab-IR");
  test_correspondence ("ku_SY", "ku-Latn-SY");
  test_correspondence ("ku_TR", "ku-Latn-TR");

  test_correspondence ("pa_PK", "pa-Arab-PK");
  test_correspondence ("pa_IN", "pa-Guru-IN");

  test_correspondence ("zh_CN", "zh-Hans-CN");
  test_correspondence ("zh_HK", "zh-Hant-HK");
  test_correspondence ("zh_MO", "zh-Hant-MO");
  test_correspondence ("zh_SG", "zh-Hans-SG");
  test_correspondence ("zh_TW", "zh-Hant-TW");

  /* Languages with a main script and one or more alternate scripts.  */

  test_correspondence ("be_BY", "be-Cyrl-BY");
  test_correspondence ("be_BY@latin", "be-Latn-BY");

  test_correspondence ("ber@arabic", "ber-Arab");
  test_correspondence ("ber", "ber-Latn");
  test_correspondence ("ber_DZ", "ber-Latn-DZ");
  test_correspondence ("ber_MA", "ber-Latn-MA");

  test_correspondence ("bs_BA", "bs-Latn-BA");
  test_correspondence ("bs_BA@cyrillic", "bs-Cyrl-BA");

  test_correspondence ("ha_NG", "ha-Latn-NG");
  test_correspondence ("ha_NG@arabic", "ha-Arab-NG");

  test_correspondence ("iu_CA", "iu-Cans-CA");
  test_correspondence ("iu_CA@latin", "iu-Latn-CA");

  test_correspondence ("kk_KZ", "kk-Cyrl-KZ");
  test_correspondence ("kk_KZ@latin", "kk-Latn-KZ");

  test_correspondence ("ks_IN", "ks-Arab-IN");
  test_correspondence ("ks_IN@devanagari", "ks-Deva-IN");

  test_correspondence ("mn_MN", "mn-Cyrl-MN");
  test_correspondence ("mn_MN@mongolian", "mn-Mong-MN");

  test_correspondence ("nan_TW", "nan-Hant-TW");
  test_correspondence ("nan_TW@latin", "nan-Latn-TW");

  test_correspondence ("sd_PK", "sd-Arab-PK");
  test_correspondence ("sd_IN", "sd-Arab-IN");
  test_correspondence ("sd_IN@devanagari", "sd-Deva-IN");

  test_correspondence ("sr_BA@latin", "sr-Latn-BA");
  test_correspondence ("sr_BA", "sr-Cyrl-BA");
  test_correspondence ("sr_RS", "sr-Cyrl-RS");
  test_correspondence ("sr_RS@latin", "sr-Latn-RS");

  test_correspondence ("uz_UZ", "uz-Latn-UZ");
  test_correspondence ("uz_UZ@cyrillic", "uz-Cyrl-UZ");

  test_correspondence ("yi_US", "yi-Hebr-US");
  test_correspondence ("yi_US@latin", "yi-Latn-US");

  /* For Quechua, Microsoft uses the ISO 639-3 code "quz" instead of the
     ISO 639-1 code "qu".  */
  {
    char buf[BCP47_MAX];
    memset (buf, 0x77, BCP47_MAX);

    bcp47_to_xpg (buf, "quz-PE", NULL);
    ASSERT (strcmp (buf, "qu_PE") == 0);
  }

  /* For Tamazight, Microsoft uses the ISO 639-3 code "tzm" instead of the
     ISO 639-2 code "ber".  */
  {
    char buf[BCP47_MAX];
    memset (buf, 0x77, BCP47_MAX);

    bcp47_to_xpg (buf, "tzm-MA", NULL);
    ASSERT (strcmp (buf, "ber_MA") == 0);
  }

  /* Languages with a regional variant.  */

  test_correspondence ("ca", "ca");
  test_correspondence ("ca@valencia", "ca-valencia");

  /* Test xpg_to_bcp47 with an encoding.  */
  {
    char buf[BCP47_MAX];
    memset (buf, 0x77, BCP47_MAX);

    xpg_to_bcp47 (buf, "en_US.UTF-8");
    ASSERT (strcmp (buf, "en-US") == 0);
  }
  {
    char buf[BCP47_MAX];
    memset (buf, 0x77, BCP47_MAX);

    xpg_to_bcp47 (buf, "az_AZ.UTF-8@cyrillic");
    ASSERT (strcmp (buf, "az-Cyrl-AZ") == 0);
  }

  /* Test bcp47_to_xpg with an encoding.  */
  {
    char buf[BCP47_MAX];
    memset (buf, 0x77, BCP47_MAX);

    bcp47_to_xpg (buf, "en-US", "UTF-8");
    ASSERT (strcmp (buf, "en_US.UTF-8") == 0);
  }
  {
    char buf[BCP47_MAX];
    memset (buf, 0x77, BCP47_MAX);

    bcp47_to_xpg (buf, "az-Cyrl-AZ", "UTF-8");
    ASSERT (strcmp (buf, "az_AZ.UTF-8@cyrillic") == 0);
  }

  /* Test case mapping done by bcp47_to_xpg.  */
  {
    char buf[BCP47_MAX];
    memset (buf, 0x77, BCP47_MAX);

    bcp47_to_xpg (buf, "EN-US", "UTF-8");
    ASSERT (strcmp (buf, "en_US.UTF-8") == 0);
  }
  {
    char buf[BCP47_MAX];
    memset (buf, 0x77, BCP47_MAX);

    bcp47_to_xpg (buf, "en-us", "UTF-8");
    ASSERT (strcmp (buf, "en_US.UTF-8") == 0);
  }
  {
    char buf[BCP47_MAX];
    memset (buf, 0x77, BCP47_MAX);

    bcp47_to_xpg (buf, "Zh-hANs-cN", "UTF-8");
    ASSERT (strcmp (buf, "zh_CN.UTF-8") == 0);
  }

  return test_exit_status;
}
