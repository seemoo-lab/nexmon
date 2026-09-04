/* Charset handling while reading PO files.
   Copyright (C) 2001-2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible.  */


#include <config.h>
#include <alloca.h>

/* Specification.  */
#include "po-charset.h"

#include <string.h>

#include "c-strcase.h"
#include "gettext.h"

#define _(str) gettext (str)

#define SIZEOF(a) (sizeof(a) / sizeof(a[0]))

static const char ascii[] = "ASCII";

/* The canonicalized encoding name for ASCII.  */
const char *po_charset_ascii = ascii;

static const char utf8[] = "UTF-8";

/* The canonicalized encoding name for UTF-8.  */
const char *po_charset_utf8 = utf8;

/* Canonicalize an encoding name.  */
const char *
po_charset_canonicalize (const char *charset)
{
  /* The list of charsets supported by glibc's iconv() and by the portable
     iconv() across platforms.  Taken from intl/localcharset.h.  */
  static const char *standard_charsets[] =
  {
    ascii, "ANSI_X3.4-1968", "US-ASCII",        /* i = 0..2 */
    "ISO-8859-1", "ISO_8859-1",                 /* i = 3, 4 */
    "ISO-8859-2", "ISO_8859-2",
    "ISO-8859-3", "ISO_8859-3",
    "ISO-8859-4", "ISO_8859-4",
    "ISO-8859-5", "ISO_8859-5",
    "ISO-8859-6", "ISO_8859-6",
    "ISO-8859-7", "ISO_8859-7",
    "ISO-8859-8", "ISO_8859-8",
    "ISO-8859-9", "ISO_8859-9",
    "ISO-8859-13", "ISO_8859-13",
    "ISO-8859-14", "ISO_8859-14",
    "ISO-8859-15", "ISO_8859-15",               /* i = 25, 26 */
    "KOI8-R",
    "KOI8-U",
    "KOI8-T",
    "CP850",
    "CP866",
    "CP874",
    "CP932",
    "CP949",
    "CP950",
    "CP1250",
    "CP1251",
    "CP1252",
    "CP1253",
    "CP1254",
    "CP1255",
    "CP1256",
    "CP1257",
    "GB2312",
    "EUC-JP",
    "EUC-KR",
    "EUC-TW",
    "BIG5",
    "BIG5-HKSCS",
    "GBK",
    "GB18030",
    "SHIFT_JIS",
    "JOHAB",
    "TIS-620",
    "VISCII",
    "GEORGIAN-PS",
    utf8
  };

  for (size_t i = 0; i < SIZEOF (standard_charsets); i++)
    if (c_strcasecmp (charset, standard_charsets[i]) == 0)
      return standard_charsets[i < 3 ? 0 : i < 27 ? ((i - 3) & ~1) + 3 : i];
  return NULL;
}

/* Test for ASCII compatibility.  */
bool
po_charset_ascii_compatible (const char *canon_charset)
{
  /* There are only a few exceptions to ASCII compatibility.  */
  if (strcmp (canon_charset, "SHIFT_JIS") == 0
      || strcmp (canon_charset, "JOHAB") == 0
      || strcmp (canon_charset, "VISCII") == 0)
    return false;
  else
    return true;
}

/* Test for a weird encoding, i.e. an encoding which has double-byte
   characters ending in 0x5C.  */
bool po_is_charset_weird (const char *canon_charset)
{
  static const char *weird_charsets[] =
  {
    "BIG5",
    "BIG5-HKSCS",
    "GBK",
    "GB18030",
    "SHIFT_JIS",
    "JOHAB"
  };

  for (size_t i = 0; i < SIZEOF (weird_charsets); i++)
    if (strcmp (canon_charset, weird_charsets[i]) == 0)
      return true;
  return false;
}

/* Test for a weird CJK encoding, i.e. a weird encoding with CJK structure.
   An encoding has CJK structure if every valid character stream is composed
   of single bytes in the range 0x{00..7F} and of byte pairs in the range
   0x{80..FF}{30..FF}.  */
bool po_is_charset_weird_cjk (const char *canon_charset)
{
  static const char *weird_cjk_charsets[] =
  {                     /* single bytes   double bytes       */
    "BIG5",             /* 0x{00..7F},    0x{A1..F9}{40..FE} */
    "BIG5-HKSCS",       /* 0x{00..7F},    0x{88..FE}{40..FE} */
    "GBK",              /* 0x{00..7F},    0x{81..FE}{40..FE} */
    "GB18030",          /* 0x{00..7F},    0x{81..FE}{30..FE} */
    "SHIFT_JIS",        /* 0x{00..7F},    0x{81..F9}{40..FC} */
    "JOHAB"             /* 0x{00..7F},    0x{84..F9}{31..FE} */
  };

  for (size_t i = 0; i < SIZEOF (weird_cjk_charsets); i++)
    if (strcmp (canon_charset, weird_cjk_charsets[i]) == 0)
      return true;
  return false;
}

/* Hardcoded iterator functions for all kinds of encodings.
   We could also implement a general iterator function with iconv(),
   but we need a fast one.  */

/* Character iterator for 8-bit encodings.  */
static size_t
char_iterator (const char *s)
{
  return 1;
}

/* Character iterator for GB2312.  See libiconv/lib/euc_cn.h.  */
/* Character iterator for EUC-KR.  See libiconv/lib/euc_kr.h.  */
static size_t
euc_character_iterator (const char *s)
{
  unsigned char c = *s;
  if (c >= 0xa1 && c < 0xff)
    {
      unsigned char c2 = s[1];
      if (c2 >= 0xa1 && c2 < 0xff)
        return 2;
    }
  return 1;
}

/* Character iterator for EUC-JP.  See libiconv/lib/euc_jp.h.  */
static size_t
euc_jp_character_iterator (const char *s)
{
  unsigned char c = *s;
  if (c >= 0xa1 && c < 0xff)
    {
      unsigned char c2 = s[1];
      if (c2 >= 0xa1 && c2 < 0xff)
        return 2;
    }
  else if (c == 0x8e)
    {
      unsigned char c2 = s[1];
      if (c2 >= 0xa1 && c2 < 0xe0)
        return 2;
    }
  else if (c == 0x8f)
    {
      unsigned char c2 = s[1];
      if (c2 >= 0xa1 && c2 < 0xff)
        {
          unsigned char c3 = s[2];
          if (c3 >= 0xa1 && c3 < 0xff)
            return 3;
        }
    }
  return 1;
}

/* Character iterator for EUC-TW.  See libiconv/lib/euc_tw.h.  */
static size_t
euc_tw_character_iterator (const char *s)
{
  unsigned char c = *s;
  if (c >= 0xa1 && c < 0xff)
    {
      unsigned char c2 = s[1];
      if (c2 >= 0xa1 && c2 < 0xff)
        return 2;
    }
  else if (c == 0x8e)
    {
      unsigned char c2 = s[1];
      if (c2 >= 0xa1 && c2 <= 0xb0)
        {
          unsigned char c3 = s[2];
          if (c3 >= 0xa1 && c3 < 0xff)
            {
              unsigned char c4 = s[3];
              if (c4 >= 0xa1 && c4 < 0xff)
                return 4;
            }
        }
    }
  return 1;
}

/* Character iterator for BIG5.  See libiconv/lib/ces_big5.h.  */
static size_t
big5_character_iterator (const char *s)
{
  unsigned char c = *s;
  if (c >= 0xa1 && c < 0xff)
    {
      unsigned char c2 = s[1];
      if ((c2 >= 0x40 && c2 < 0x7f) || (c2 >= 0xa1 && c2 < 0xff))
        return 2;
    }
  return 1;
}

/* Character iterator for BIG5-HKSCS.  See libiconv/lib/big5hkscs.h.  */
static size_t
big5hkscs_character_iterator (const char *s)
{
  unsigned char c = *s;
  if (c >= 0x88 && c < 0xff)
    {
      unsigned char c2 = s[1];
      if ((c2 >= 0x40 && c2 < 0x7f) || (c2 >= 0xa1 && c2 < 0xff))
        return 2;
    }
  return 1;
}

/* Character iterator for GBK.  See libiconv/lib/ces_gbk.h and
   libiconv/lib/gbk.h.  */
static size_t
gbk_character_iterator (const char *s)
{
  unsigned char c = *s;
  if (c >= 0x81 && c < 0xff)
    {
      unsigned char c2 = s[1];
      if ((c2 >= 0x40 && c2 < 0x7f) || (c2 >= 0x80 && c2 < 0xff))
        return 2;
    }
  return 1;
}

/* Character iterator for GB18030.  See libiconv/lib/gb18030.h.  */
static size_t
gb18030_character_iterator (const char *s)
{
  unsigned char c = *s;
  if (c >= 0x81 && c < 0xff)
    {
      unsigned char c2 = s[1];
      if ((c2 >= 0x40 && c2 < 0x7f) || (c2 >= 0x80 && c2 < 0xff))
        return 2;
    }
  if (c >= 0x81 && c <= 0x84)
    {
      unsigned char c2 = s[1];
      if (c2 >= 0x30 && c2 <= 0x39)
        {
          unsigned char c3 = s[2];
          if (c3 >= 0x81 && c3 < 0xff)
            {
              unsigned char c4 = s[3];
              if (c4 >= 0x30 && c4 <= 0x39)
                return 4;
            }
        }
    }
  return 1;
}

/* Character iterator for SHIFT_JIS.  See libiconv/lib/sjis.h.  */
static size_t
shift_jis_character_iterator (const char *s)
{
  unsigned char c = *s;
  if ((c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xf9))
    {
      unsigned char c2 = s[1];
      if ((c2 >= 0x40 && c2 <= 0x7e) || (c2 >= 0x80 && c2 <= 0xfc))
        return 2;
    }
  return 1;
}

/* Character iterator for JOHAB.  See libiconv/lib/johab.h and
   libiconv/lib/johab_hangul.h.  */
static size_t
johab_character_iterator (const char *s)
{
  unsigned char c = *s;
  if (c >= 0x84 && c <= 0xd3)
    {
      unsigned char c2 = s[1];
      if ((c2 >= 0x41 && c2 < 0x7f) || (c2 >= 0x81 && c2 < 0xff))
        return 2;
    }
  else if (c >= 0xd9 && c <= 0xf9)
    {
      unsigned char c2 = s[1];
      if ((c2 >= 0x31 && c2 <= 0x7e) || (c2 >= 0x91 && c2 <= 0xfe))
        return 2;
    }
  return 1;
}

/* Character iterator for UTF-8.  See libiconv/lib/utf8.h.  */
static size_t
utf8_character_iterator (const char *s)
{
  unsigned char c = *s;
  if (c >= 0xc2)
    {
      if (c < 0xe0)
        {
          unsigned char c2 = s[1];
          if (c2 >= 0x80 && c2 < 0xc0)
            return 2;
        }
      else if (c < 0xf0)
        {
          unsigned char c2 = s[1];
          if (c2 >= 0x80 && c2 < 0xc0)
            {
              unsigned char c3 = s[2];
              if (c3 >= 0x80 && c3 < 0xc0)
                return 3;
            }
        }
      else if (c < 0xf8)
        {
          unsigned char c2 = s[1];
          if (c2 >= 0x80 && c2 < 0xc0)
            {
              unsigned char c3 = s[2];
              if (c3 >= 0x80 && c3 < 0xc0)
                {
                  unsigned char c4 = s[3];
                  if (c4 >= 0x80 && c4 < 0xc0)
                    return 4;
                }
            }
        }
    }
  return 1;
}

/* Returns a character iterator for a given encoding.
   Given a pointer into a string, it returns the number occupied by the next
   single character.  If the piece of string is not valid or if the *s == '\0',
   it returns 1.  */
character_iterator_t
po_charset_character_iterator (const char *canon_charset)
{
  if (canon_charset == utf8)
    return utf8_character_iterator;
  if (strcmp (canon_charset, "GB2312") == 0
      || strcmp (canon_charset, "EUC-KR") == 0)
    return euc_character_iterator;
  if (strcmp (canon_charset, "EUC-JP") == 0)
    return euc_jp_character_iterator;
  if (strcmp (canon_charset, "EUC-TW") == 0)
    return euc_tw_character_iterator;
  if (strcmp (canon_charset, "BIG5") == 0)
    return big5_character_iterator;
  if (strcmp (canon_charset, "BIG5-HKSCS") == 0)
    return big5hkscs_character_iterator;
  if (strcmp (canon_charset, "GBK") == 0)
    return gbk_character_iterator;
  if (strcmp (canon_charset, "GB18030") == 0)
    return gb18030_character_iterator;
  if (strcmp (canon_charset, "SHIFT_JIS") == 0)
    return shift_jis_character_iterator;
  if (strcmp (canon_charset, "JOHAB") == 0)
    return johab_character_iterator;
  return char_iterator;
}
