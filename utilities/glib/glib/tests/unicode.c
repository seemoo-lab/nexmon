/* Unit tests for utilities
 * Copyright (C) 2010 Red Hat, Inc.
 * Copyright (C) 2011 Google, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-old-glib-tests
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 * Author: Matthias Clasen, Behdad Esfahbod
 */

/* We are testing some deprecated APIs here */
#ifndef GLIB_DISABLE_DEPRECATION_WARNINGS
#define GLIB_DISABLE_DEPRECATION_WARNINGS
#endif

#include <locale.h>
#include <stdio.h>

#include "glib.h"
#include "glib/gstdio.h"

#include "glib/gunidecomp.h"

#ifdef G_OS_WIN32
#include <windows.h>
#endif

static void
save_and_clear_env (const char  *name,
                    char       **save)
{
  *save = g_strdup (g_getenv (name));
  g_unsetenv (name);
}

/* Test that g_unichar_validate() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_unichar_validate (void)
{
  g_assert_true (g_unichar_validate ('j'));
  g_assert_true (g_unichar_validate (8356));
  g_assert_true (g_unichar_validate (8356));
  g_assert_true (g_unichar_validate (0xFDD1));
  g_assert_true (g_unichar_validate (917760));
  g_assert_false (g_unichar_validate (0x110000));
}

/* Test that g_unichar_type() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_unichar_character_type (void)
{
  guint i;
  struct {
    GUnicodeType type;
    gunichar     c;
  } examples[] = {
    { G_UNICODE_CONTROL,              0x000D },
    { G_UNICODE_FORMAT,               0x200E },
     /* G_UNICODE_UNASSIGNED */
    { G_UNICODE_PRIVATE_USE,          0xE000 },
    { G_UNICODE_SURROGATE,            0xD800 },
    { G_UNICODE_LOWERCASE_LETTER,     0x0061 },
    { G_UNICODE_MODIFIER_LETTER,      0x02B0 },
    { G_UNICODE_OTHER_LETTER,         0x3400 },
    { G_UNICODE_TITLECASE_LETTER,     0x01C5 },
    { G_UNICODE_UPPERCASE_LETTER,     0xFF21 },
    { G_UNICODE_SPACING_MARK,         0x0903 },
    { G_UNICODE_ENCLOSING_MARK,       0x20DD },
    { G_UNICODE_NON_SPACING_MARK,     0xA806 },
    { G_UNICODE_DECIMAL_NUMBER,       0xFF10 },
    { G_UNICODE_LETTER_NUMBER,        0x16EE },
    { G_UNICODE_OTHER_NUMBER,         0x17F0 },
    { G_UNICODE_CONNECT_PUNCTUATION,  0x005F },
    { G_UNICODE_DASH_PUNCTUATION,     0x058A },
    { G_UNICODE_CLOSE_PUNCTUATION,    0x0F3B },
    { G_UNICODE_FINAL_PUNCTUATION,    0x2019 },
    { G_UNICODE_INITIAL_PUNCTUATION,  0x2018 },
    { G_UNICODE_OTHER_PUNCTUATION,    0x2016 },
    { G_UNICODE_OPEN_PUNCTUATION,     0x0F3A },
    { G_UNICODE_CURRENCY_SYMBOL,      0x20A0 },
    { G_UNICODE_MODIFIER_SYMBOL,      0x309B },
    { G_UNICODE_MATH_SYMBOL,          0xFB29 },
    { G_UNICODE_OTHER_SYMBOL,         0x00A6 },
    { G_UNICODE_LINE_SEPARATOR,       0x2028 },
    { G_UNICODE_PARAGRAPH_SEPARATOR,  0x2029 },
    { G_UNICODE_SPACE_SEPARATOR,      0x202F },
  };

  for (i = 0; i < G_N_ELEMENTS (examples); i++)
    {
      g_assert_cmpint (g_unichar_type (examples[i].c), ==, examples[i].type);
    }

  /*** Testing TYPE() border cases ***/
  g_assert_cmpint (g_unichar_type (0x3FF5), ==, 0x07);
  /* U+FFEFF Plane 15 Private Use */
  g_assert_cmpint (g_unichar_type (0xFFEFF), ==, 0x03);
  /* U+E0001 Language Tag */
  g_assert_cmpint (g_unichar_type (0xE0001), ==, 0x01);
  g_assert_cmpint (g_unichar_type (G_UNICODE_LAST_CHAR), ==, 0x02);
  g_assert_cmpint (g_unichar_type (G_UNICODE_LAST_CHAR + 1), ==, 0x02);
  g_assert_cmpint (g_unichar_type (G_UNICODE_LAST_CHAR_PART1), ==, 0x02);
  g_assert_cmpint (g_unichar_type (G_UNICODE_LAST_CHAR_PART1 + 1), ==, 0x02);
}

/* Test that g_unichar_break_type() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_unichar_break_type (void)
{
  guint i;
  struct {
    GUnicodeBreakType type;
    gunichar          c;
  } examples[] = {
    { G_UNICODE_BREAK_MANDATORY,           0x2028 },
    { G_UNICODE_BREAK_CARRIAGE_RETURN,     0x000D },
    { G_UNICODE_BREAK_LINE_FEED,           0x000A },
    { G_UNICODE_BREAK_COMBINING_MARK,      0x0300 },
    { G_UNICODE_BREAK_SURROGATE,           0xD800 },
    { G_UNICODE_BREAK_ZERO_WIDTH_SPACE,    0x200B },
    { G_UNICODE_BREAK_INSEPARABLE,         0x2024 },
    { G_UNICODE_BREAK_NON_BREAKING_GLUE,   0x00A0 },
    { G_UNICODE_BREAK_CONTINGENT,          0xFFFC },
    { G_UNICODE_BREAK_SPACE,               0x0020 },
    { G_UNICODE_BREAK_AFTER,               0x1680 },
    { G_UNICODE_BREAK_BEFORE,              0x02C8 },
    { G_UNICODE_BREAK_BEFORE_AND_AFTER,    0x2014 },
    { G_UNICODE_BREAK_HYPHEN,              0x002D },
    { G_UNICODE_BREAK_NON_STARTER,         0x17D6 },
    { G_UNICODE_BREAK_OPEN_PUNCTUATION,    0x0028 },
    { G_UNICODE_BREAK_CLOSE_PARENTHESIS,   0x0029 },
    { G_UNICODE_BREAK_CLOSE_PUNCTUATION,   0x007D },
    { G_UNICODE_BREAK_QUOTATION,           0x0022 },
    { G_UNICODE_BREAK_EXCLAMATION,         0x0021 },
    { G_UNICODE_BREAK_IDEOGRAPHIC,         0x2E80 },
    { G_UNICODE_BREAK_NUMERIC,             0x0030 },
    { G_UNICODE_BREAK_INFIX_SEPARATOR,     0x002C },
    { G_UNICODE_BREAK_SYMBOL,              0x002F },
    { G_UNICODE_BREAK_ALPHABETIC,          0x0023 },
    { G_UNICODE_BREAK_PREFIX,              0x0024 },
    { G_UNICODE_BREAK_POSTFIX,             0x0025 },
    { G_UNICODE_BREAK_COMPLEX_CONTEXT,     0x0E01 },
    { G_UNICODE_BREAK_AMBIGUOUS,           0x00F7 },
    { G_UNICODE_BREAK_UNKNOWN,             0xE000 },
    { G_UNICODE_BREAK_NEXT_LINE,           0x0085 },
    { G_UNICODE_BREAK_WORD_JOINER,         0x2060 },
    { G_UNICODE_BREAK_HANGUL_L_JAMO,       0x1100 },
    { G_UNICODE_BREAK_HANGUL_V_JAMO,       0x1160 },
    { G_UNICODE_BREAK_HANGUL_T_JAMO,       0x11A8 },
    { G_UNICODE_BREAK_HANGUL_LV_SYLLABLE,  0xAC00 },
    { G_UNICODE_BREAK_HANGUL_LVT_SYLLABLE, 0xAC01 },
    { G_UNICODE_BREAK_CONDITIONAL_JAPANESE_STARTER, 0x3041 },
    { G_UNICODE_BREAK_HEBREW_LETTER,                0x05D0 },
    { G_UNICODE_BREAK_REGIONAL_INDICATOR,           0x1F1F6 },
    { G_UNICODE_BREAK_EMOJI_BASE,          0x1F466 },
    { G_UNICODE_BREAK_EMOJI_MODIFIER,      0x1F3FB },
    { G_UNICODE_BREAK_ZERO_WIDTH_JOINER,   0x200D },
    { G_UNICODE_BREAK_AKSARA,              0x1B45 },
    { G_UNICODE_BREAK_AKSARA_PRE_BASE,     0x1193F },
    { G_UNICODE_BREAK_AKSARA_START,        0x11F50 },
    { G_UNICODE_BREAK_VIRAMA_FINAL,        0x1BF3 },
    { G_UNICODE_BREAK_VIRAMA,              0xA9C0 },
    { G_UNICODE_BREAK_UNAMBIGUOUS_HYPHEN,  0x05BE },
  };

  for (i = 0; i < G_N_ELEMENTS (examples); i++)
    {
      g_assert_cmpint (g_unichar_break_type (examples[i].c), ==, examples[i].type);
    }
}

/* Test that g_unichar_get_script() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_unichar_script (void)
{
  guint i;
  struct {
    GUnicodeScript script;
    gunichar          c;
  } examples[] = {
    { G_UNICODE_SCRIPT_COMMON,                  0x002A },
    { G_UNICODE_SCRIPT_INHERITED,               0x1CED },
    { G_UNICODE_SCRIPT_INHERITED,               0x0670 },
    { G_UNICODE_SCRIPT_ARABIC,                  0x060D },
    { G_UNICODE_SCRIPT_ARMENIAN,                0x0559 },
    { G_UNICODE_SCRIPT_BENGALI,                 0x09CD },
    { G_UNICODE_SCRIPT_BOPOMOFO,                0x31B6 },
    { G_UNICODE_SCRIPT_CHEROKEE,                0x13A2 },
    { G_UNICODE_SCRIPT_COPTIC,                  0x2CFD },
    { G_UNICODE_SCRIPT_CYRILLIC,                0x0482 },
    { G_UNICODE_SCRIPT_DESERET,                0x10401 },
    { G_UNICODE_SCRIPT_DEVANAGARI,              0x094D },
    { G_UNICODE_SCRIPT_ETHIOPIC,                0x1258 },
    { G_UNICODE_SCRIPT_GEORGIAN,                0x10FC },
    { G_UNICODE_SCRIPT_GOTHIC,                 0x10341 },
    { G_UNICODE_SCRIPT_GREEK,                   0x0375 },
    { G_UNICODE_SCRIPT_GUJARATI,                0x0A83 },
    { G_UNICODE_SCRIPT_GURMUKHI,                0x0A3C },
    { G_UNICODE_SCRIPT_HAN,                     0x3005 },
    { G_UNICODE_SCRIPT_HANGUL,                  0x1100 },
    { G_UNICODE_SCRIPT_HEBREW,                  0x05BF },
    { G_UNICODE_SCRIPT_HIRAGANA,                0x309F },
    { G_UNICODE_SCRIPT_KANNADA,                 0x0CBC },
    { G_UNICODE_SCRIPT_KATAKANA,                0x30FF },
    { G_UNICODE_SCRIPT_KHMER,                   0x17DD },
    { G_UNICODE_SCRIPT_LAO,                     0x0EDD },
    { G_UNICODE_SCRIPT_LATIN,                   0x0061 },
    { G_UNICODE_SCRIPT_MALAYALAM,               0x0D3D },
    { G_UNICODE_SCRIPT_MONGOLIAN,               0x1843 },
    { G_UNICODE_SCRIPT_MYANMAR,                 0x1031 },
    { G_UNICODE_SCRIPT_OGHAM,                   0x169C },
    { G_UNICODE_SCRIPT_OLD_ITALIC,             0x10322 },
    { G_UNICODE_SCRIPT_ORIYA,                   0x0B3C },
    { G_UNICODE_SCRIPT_RUNIC,                   0x16EF },
    { G_UNICODE_SCRIPT_SINHALA,                 0x0DBD },
    { G_UNICODE_SCRIPT_SYRIAC,                  0x0711 },
    { G_UNICODE_SCRIPT_TAMIL,                   0x0B82 },
    { G_UNICODE_SCRIPT_TELUGU,                  0x0C03 },
    { G_UNICODE_SCRIPT_THAANA,                  0x07B1 },
    { G_UNICODE_SCRIPT_THAI,                    0x0E31 },
    { G_UNICODE_SCRIPT_TIBETAN,                 0x0FD4 },
    { G_UNICODE_SCRIPT_CANADIAN_ABORIGINAL,     0x1400 },
    { G_UNICODE_SCRIPT_CANADIAN_ABORIGINAL,     0x1401 },
    { G_UNICODE_SCRIPT_YI,                      0xA015 },
    { G_UNICODE_SCRIPT_TAGALOG,                 0x1700 },
    { G_UNICODE_SCRIPT_HANUNOO,                 0x1720 },
    { G_UNICODE_SCRIPT_BUHID,                   0x1740 },
    { G_UNICODE_SCRIPT_TAGBANWA,                0x1760 },
    { G_UNICODE_SCRIPT_BRAILLE,                 0x2800 },
    { G_UNICODE_SCRIPT_CYPRIOT,                0x10808 },
    { G_UNICODE_SCRIPT_LIMBU,                   0x1932 },
    { G_UNICODE_SCRIPT_OSMANYA,                0x10480 },
    { G_UNICODE_SCRIPT_SHAVIAN,                0x10450 },
    { G_UNICODE_SCRIPT_LINEAR_B,               0x10000 },
    { G_UNICODE_SCRIPT_TAI_LE,                  0x1950 },
    { G_UNICODE_SCRIPT_UGARITIC,               0x1039F },
    { G_UNICODE_SCRIPT_NEW_TAI_LUE,             0x1980 },
    { G_UNICODE_SCRIPT_BUGINESE,                0x1A1F },
    { G_UNICODE_SCRIPT_GLAGOLITIC,              0x2C00 },
    { G_UNICODE_SCRIPT_TIFINAGH,                0x2D6F },
    { G_UNICODE_SCRIPT_SYLOTI_NAGRI,            0xA800 },
    { G_UNICODE_SCRIPT_OLD_PERSIAN,            0x103D0 },
    { G_UNICODE_SCRIPT_KHAROSHTHI,             0x10A3F },
    { G_UNICODE_SCRIPT_UNKNOWN,              0x1111111 },
    { G_UNICODE_SCRIPT_BALINESE,                0x1B04 },
    { G_UNICODE_SCRIPT_CUNEIFORM,              0x12000 },
    { G_UNICODE_SCRIPT_PHOENICIAN,             0x10900 },
    { G_UNICODE_SCRIPT_PHAGS_PA,                0xA840 },
    { G_UNICODE_SCRIPT_NKO,                     0x07C0 },
    { G_UNICODE_SCRIPT_KAYAH_LI,                0xA900 },
    { G_UNICODE_SCRIPT_LEPCHA,                  0x1C00 },
    { G_UNICODE_SCRIPT_REJANG,                  0xA930 },
    { G_UNICODE_SCRIPT_SUNDANESE,               0x1B80 },
    { G_UNICODE_SCRIPT_SAURASHTRA,              0xA880 },
    { G_UNICODE_SCRIPT_CHAM,                    0xAA00 },
    { G_UNICODE_SCRIPT_OL_CHIKI,                0x1C50 },
    { G_UNICODE_SCRIPT_VAI,                     0xA500 },
    { G_UNICODE_SCRIPT_CARIAN,                 0x102A0 },
    { G_UNICODE_SCRIPT_LYCIAN,                 0x10280 },
    { G_UNICODE_SCRIPT_LYDIAN,                 0x1093F },
    { G_UNICODE_SCRIPT_AVESTAN,                0x10B00 },
    { G_UNICODE_SCRIPT_BAMUM,                   0xA6A0 },
    { G_UNICODE_SCRIPT_EGYPTIAN_HIEROGLYPHS,   0x13000 },
    { G_UNICODE_SCRIPT_IMPERIAL_ARAMAIC,       0x10840 },
    { G_UNICODE_SCRIPT_INSCRIPTIONAL_PAHLAVI,  0x10B60 },
    { G_UNICODE_SCRIPT_INSCRIPTIONAL_PARTHIAN, 0x10B40 },
    { G_UNICODE_SCRIPT_JAVANESE,                0xA980 },
    { G_UNICODE_SCRIPT_KAITHI,                 0x11082 },
    { G_UNICODE_SCRIPT_LISU,                    0xA4D0 },
    { G_UNICODE_SCRIPT_MEETEI_MAYEK,            0xABE5 },
    { G_UNICODE_SCRIPT_OLD_SOUTH_ARABIAN,      0x10A60 },
    { G_UNICODE_SCRIPT_OLD_TURKIC,             0x10C00 },
    { G_UNICODE_SCRIPT_SAMARITAN,               0x0800 },
    { G_UNICODE_SCRIPT_TAI_THAM,                0x1A20 },
    { G_UNICODE_SCRIPT_TAI_VIET,                0xAA80 },
    { G_UNICODE_SCRIPT_BATAK,                   0x1BC0 },
    { G_UNICODE_SCRIPT_BRAHMI,                 0x11000 },
    { G_UNICODE_SCRIPT_MANDAIC,                 0x0840 },
    { G_UNICODE_SCRIPT_CHAKMA,                 0x11100 },
    { G_UNICODE_SCRIPT_MEROITIC_CURSIVE,       0x109A0 },
    { G_UNICODE_SCRIPT_MEROITIC_HIEROGLYPHS,   0x10980 },
    { G_UNICODE_SCRIPT_MIAO,                   0x16F00 },
    { G_UNICODE_SCRIPT_SHARADA,                0x11180 },
    { G_UNICODE_SCRIPT_SORA_SOMPENG,           0x110D0 },
    { G_UNICODE_SCRIPT_TAKRI,                  0x11680 },
    { G_UNICODE_SCRIPT_BASSA_VAH,              0x16AD0 },
    { G_UNICODE_SCRIPT_CAUCASIAN_ALBANIAN,     0x10530 },
    { G_UNICODE_SCRIPT_DUPLOYAN,               0x1BC00 },
    { G_UNICODE_SCRIPT_ELBASAN,                0x10500 },
    { G_UNICODE_SCRIPT_GRANTHA,                0x11301 },
    { G_UNICODE_SCRIPT_KHOJKI,                 0x11200 },
    { G_UNICODE_SCRIPT_KHUDAWADI,              0x112B0 },
    { G_UNICODE_SCRIPT_LINEAR_A,               0x10600 },
    { G_UNICODE_SCRIPT_MAHAJANI,               0x11150 },
    { G_UNICODE_SCRIPT_MANICHAEAN,             0x10AC0 },
    { G_UNICODE_SCRIPT_MENDE_KIKAKUI,          0x1E800 },
    { G_UNICODE_SCRIPT_MODI,                   0x11600 },
    { G_UNICODE_SCRIPT_MRO,                    0x16A40 },
    { G_UNICODE_SCRIPT_NABATAEAN,              0x10880 },
    { G_UNICODE_SCRIPT_OLD_NORTH_ARABIAN,      0x10A80 },
    { G_UNICODE_SCRIPT_OLD_PERMIC,             0x10350 },
    { G_UNICODE_SCRIPT_PAHAWH_HMONG,           0x16B00 },
    { G_UNICODE_SCRIPT_PALMYRENE,              0x10860 },
    { G_UNICODE_SCRIPT_PAU_CIN_HAU,            0x11AC0 },
    { G_UNICODE_SCRIPT_PSALTER_PAHLAVI,        0x10B80 },
    { G_UNICODE_SCRIPT_SIDDHAM,                0x11580 },
    { G_UNICODE_SCRIPT_TIRHUTA,                0x11480 },
    { G_UNICODE_SCRIPT_WARANG_CITI,            0x118A0 },
    { G_UNICODE_SCRIPT_CHEROKEE,               0x0AB71 },
    { G_UNICODE_SCRIPT_HATRAN,                 0x108E0 },
    { G_UNICODE_SCRIPT_OLD_HUNGARIAN,          0x10C80 },
    { G_UNICODE_SCRIPT_MULTANI,                0x11280 },
    { G_UNICODE_SCRIPT_AHOM,                   0x11700 },
    { G_UNICODE_SCRIPT_CUNEIFORM,              0x12480 },
    { G_UNICODE_SCRIPT_ANATOLIAN_HIEROGLYPHS,  0x14400 },
    { G_UNICODE_SCRIPT_SIGNWRITING,            0x1D800 },
    { G_UNICODE_SCRIPT_ADLAM,                  0x1E900 },
    { G_UNICODE_SCRIPT_BHAIKSUKI,              0x11C00 },
    { G_UNICODE_SCRIPT_MARCHEN,                0x11C70 },
    { G_UNICODE_SCRIPT_NEWA,                   0x11400 },
    { G_UNICODE_SCRIPT_OSAGE,                  0x104B0 },
    { G_UNICODE_SCRIPT_TANGUT,                 0x16FE0 },
    { G_UNICODE_SCRIPT_MASARAM_GONDI,          0x11D00 },
    { G_UNICODE_SCRIPT_NUSHU,                  0x1B170 },
    { G_UNICODE_SCRIPT_SOYOMBO,                0x11A50 },
    { G_UNICODE_SCRIPT_ZANABAZAR_SQUARE,       0x11A00 },
    { G_UNICODE_SCRIPT_DOGRA,                  0x11800 },
    { G_UNICODE_SCRIPT_GUNJALA_GONDI,          0x11D60 },
    { G_UNICODE_SCRIPT_HANIFI_ROHINGYA,        0x10D00 },
    { G_UNICODE_SCRIPT_MAKASAR,                0x11EE0 },
    { G_UNICODE_SCRIPT_MEDEFAIDRIN,            0x16E40 },
    { G_UNICODE_SCRIPT_OLD_SOGDIAN,            0x10F00 },
    { G_UNICODE_SCRIPT_SOGDIAN,                0x10F30 },
    { G_UNICODE_SCRIPT_ELYMAIC,                0x10FE0 },
    { G_UNICODE_SCRIPT_NANDINAGARI,            0x119A0 },
    { G_UNICODE_SCRIPT_NYIAKENG_PUACHUE_HMONG, 0x1E100 },
    { G_UNICODE_SCRIPT_WANCHO,                 0x1E2C0 },
    { G_UNICODE_SCRIPT_CHORASMIAN,             0x10FB0 },
    { G_UNICODE_SCRIPT_DIVES_AKURU,            0x11900 },
    { G_UNICODE_SCRIPT_KHITAN_SMALL_SCRIPT,    0x18B00 },
    { G_UNICODE_SCRIPT_YEZIDI,                 0x10E80 },
    { G_UNICODE_SCRIPT_CYPRO_MINOAN,           0x12F90 },
    { G_UNICODE_SCRIPT_OLD_UYGHUR,             0x10F70 },
    { G_UNICODE_SCRIPT_TANGSA,                 0x16A70 },
    { G_UNICODE_SCRIPT_TOTO,                   0x1E290 },
    { G_UNICODE_SCRIPT_VITHKUQI,               0x10570 },
    { G_UNICODE_SCRIPT_KAWI,                   0x11F00 },
    { G_UNICODE_SCRIPT_NAG_MUNDARI,            0x1E4D0 },
    { G_UNICODE_SCRIPT_TODHRI,                 0x105C8 },
    { G_UNICODE_SCRIPT_GARAY,                  0x10D40 },
    { G_UNICODE_SCRIPT_TULU_TIGALARI,          0x11387 },
    { G_UNICODE_SCRIPT_SUNUWAR,                0x11BC0 },
    { G_UNICODE_SCRIPT_GURUNG_KHEMA,           0x16139 },
    { G_UNICODE_SCRIPT_KIRAT_RAI,              0x16D40 },
    { G_UNICODE_SCRIPT_OL_ONAL,                0x1E5D0 },
  };
  for (i = 0; i < G_N_ELEMENTS (examples); i++)
    g_assert_cmpint (g_unichar_get_script (examples[i].c), ==, examples[i].script);
}

/* Test that g_unichar_combining_class() returns the correct value for
 * various ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_combining_class (void)
{
  guint i;
  struct {
    gint class;
    gunichar          c;
  } examples[] = {
    {   0, 0x0020 },
    {   1, 0x0334 },
    {   7, 0x093C },
    {   8, 0x3099 },
    {   9, 0x094D },
    {  10, 0x05B0 },
    {  11, 0x05B1 },
    {  12, 0x05B2 },
    {  13, 0x05B3 },
    {  14, 0x05B4 },
    {  15, 0x05B5 },
    {  16, 0x05B6 },
    {  17, 0x05B7 },
    {  18, 0x05B8 },
    {  19, 0x05B9 },
    {  20, 0x05BB },
    {  21, 0x05BC },
    {  22, 0x05BD },
    {  23, 0x05BF },
    {  24, 0x05C1 },
    {  25, 0x05C2 },
    {  26, 0xFB1E },
    {  27, 0x064B },
    {  28, 0x064C },
    {  29, 0x064D },
    /* ... */
    { 228, 0x05AE },
    { 230, 0x0300 },
    { 232, 0x302C },
    { 233, 0x0362 },
    { 234, 0x0360 },
    { 234, 0x1DCD },
    { 240, 0x0345 },
    /* These are all (currently) unassigned, but exercise various branches in
     * the combining class lookup tables: */
    { 0, 0x323FF },
    { 0, 0x32400 },
    { 0, 0xDFFFF },
    { 0, 0xE0000 },
    { 0, G_UNICODE_LAST_CHAR },
    { 0, G_UNICODE_LAST_CHAR + 1 },
  };
  for (i = 0; i < G_N_ELEMENTS (examples); i++)
    {
      g_assert_cmpint (g_unichar_combining_class (examples[i].c), ==, examples[i].class);
    }
}

/* Test that g_unichar_get_mirror() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_mirror (void)
{
  gunichar mirror;

  g_assert_true (g_unichar_get_mirror_char ('(', &mirror));
  g_assert_cmpint (mirror, ==, ')');
  g_assert_true (g_unichar_get_mirror_char (')', &mirror));
  g_assert_cmpint (mirror, ==, '(');
  g_assert_true (g_unichar_get_mirror_char ('{', &mirror));
  g_assert_cmpint (mirror, ==, '}');
  g_assert_true (g_unichar_get_mirror_char ('}', &mirror));
  g_assert_cmpint (mirror, ==, '{');
  g_assert_true (g_unichar_get_mirror_char (0x208D, &mirror));
  g_assert_cmpint (mirror, ==, 0x208E);
  g_assert_true (g_unichar_get_mirror_char (0x208E, &mirror));
  g_assert_cmpint (mirror, ==, 0x208D);
  g_assert_false (g_unichar_get_mirror_char ('a', &mirror));
}

/* Test that g_utf8_strup() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_strup (void)
{
  char *str_up = NULL;
  const char *str = "AaZz09x;\x03\x45"
    "\xEF\xBD\x81"  /* Unichar 'A' (U+FF21) */
    "\xEF\xBC\xA1"; /* Unichar 'a' (U+FF41) */

  /* Testing degenerated cases */
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      str_up = g_utf8_strup (NULL, 0);
      g_test_assert_expected_messages ();
    }

  str_up = g_utf8_strup (str, strlen (str));
  /* Tricky, comparing two unicode strings with an ASCII function */
  g_assert_cmpstr (str_up, ==, "AAZZ09X;\003E\357\274\241\357\274\241");
  g_free (str_up);

  str_up = g_utf8_strup ("", 0);
  g_assert_cmpstr (str_up, ==, "");
  g_free (str_up);
}

/* Test that g_utf8_strdown() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_strdown (void)
{
  char *str_down = NULL;
  const char *str = "AaZz09x;\x03\x07"
    "\xEF\xBD\x81"  /* Unichar 'A' (U+FF21) */
    "\xEF\xBC\xA1"; /* Unichar 'a' (U+FF41) */

  /* Testing degenerated cases */
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      str_down = g_utf8_strdown (NULL, 0);
      g_test_assert_expected_messages ();
    }

  str_down = g_utf8_strdown (str, strlen (str));
  /* Tricky, comparing two unicode strings with an ASCII function */
  g_assert_cmpstr (str_down, ==, "aazz09x;\003\007\357\275\201\357\275\201");
  g_free (str_down);

  str_down = g_utf8_strdown ("", 0);
  g_assert_cmpstr (str_down, ==, "");
  g_free (str_down);
}

/* Test that g_utf8_strup() and g_utf8_strdown() return the correct
 * value for Turkish 'i' with and without dot above. */
static void
test_turkish_strupdown (void)
{
  char *str_up = NULL;
  char *str_down = NULL;
  const char *str = "iII"
                    "\xcc\x87"  /* COMBINING DOT ABOVE (U+307) */
                    "\xc4\xb1"  /* LATIN SMALL LETTER DOTLESS I (U+131) */
                    "\xc4\xb0"; /* LATIN CAPITAL LETTER I WITH DOT ABOVE (U+130) */
  char *oldlocale;
  char *old_lc_all, *old_lc_messages, *old_lang;
#ifdef G_OS_WIN32
  LCID old_lcid;
#endif

  /* interferes with g_win32_getlocale() */
  save_and_clear_env ("LC_ALL", &old_lc_all);
  save_and_clear_env ("LC_MESSAGES", &old_lc_messages);
  save_and_clear_env ("LANG", &old_lang);

  oldlocale = g_strdup (setlocale (LC_ALL, "tr_TR"));
  if (oldlocale == NULL)
    {
      g_test_skip ("locale tr_TR not available");
      goto out;
    }

#ifdef G_OS_WIN32
  old_lcid = GetThreadLocale ();
  SetThreadLocale (MAKELCID (MAKELANGID (LANG_TURKISH, SUBLANG_TURKISH_TURKEY), SORT_DEFAULT));
#endif

  str_up = g_utf8_strup (str, strlen (str));
  str_down = g_utf8_strdown (str, strlen (str));
  /* i => LATIN CAPITAL LETTER I WITH DOT ABOVE,
   * I => I,
   * I + COMBINING DOT ABOVE => I + COMBINING DOT ABOVE,
   * LATIN SMALL LETTER DOTLESS I => I,
   * LATIN CAPITAL LETTER I WITH DOT ABOVE => LATIN CAPITAL LETTER I WITH DOT ABOVE */
  g_assert_cmpstr (str_up, ==, "\xc4\xb0II\xcc\x87I\xc4\xb0");
  /* i => i,
   * I => LATIN SMALL LETTER DOTLESS I,
   * I + COMBINING DOT ABOVE => i,
   * LATIN SMALL LETTER DOTLESS I => LATIN SMALL LETTER DOTLESS I,
   * LATIN CAPITAL LETTER I WITH DOT ABOVE => i */
  g_assert_cmpstr (str_down, ==, "i\xc4\xb1i\xc4\xb1i");
  g_free (str_up);
  g_free (str_down);

out:
  if (oldlocale != NULL)
    setlocale (LC_ALL, oldlocale);
#ifdef G_OS_WIN32
  if (oldlocale != NULL)
    SetThreadLocale (old_lcid);
#endif
  g_free (oldlocale);
  if (old_lc_all)
    g_setenv ("LC_ALL", old_lc_all, TRUE);
  if (old_lc_messages)
    g_setenv ("LC_MESSAGES", old_lc_messages, TRUE);
  if (old_lang)
    g_setenv ("LANG", old_lang, TRUE);
  g_free (old_lc_all);
  g_free (old_lc_messages);
  g_free (old_lang);
}

/* Test that g_utf8_casefold() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_casefold (void)
{
  char *str_casefold = NULL;
  const char *str = "AaZz09x;"
    "\xEF\xBD\x81"  /* Unichar 'A' (U+FF21) */
    "\xEF\xBC\xA1"; /* Unichar 'a' (U+FF41) */

  /* Testing degenerated cases */
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      str_casefold = g_utf8_casefold (NULL, 0);
      g_test_assert_expected_messages ();
    }

  str_casefold = g_utf8_casefold (str, strlen (str));
  /* Tricky, comparing two unicode strings with an ASCII function */
  g_assert_cmpstr (str_casefold, ==, "aazz09x;\357\275\201\357\275\201");
  g_free (str_casefold);

  str_casefold = g_utf8_casefold ("", 0);
  g_assert_cmpstr (str_casefold, ==, "");
  g_free (str_casefold);
}

static void
test_casemap_and_casefold (void)
{
  FILE *infile;
  char buffer[1024];
  char **strings;
  char *filename;
  const char *locale;
  const char *test;
  const char *expected;
  size_t line = 0;
  char *convert;
  char *current_locale = setlocale (LC_CTYPE, NULL);
  char *old_lc_all, *old_lc_messages, *old_lang;
#ifdef G_OS_WIN32
  LCID old_lcid;

  old_lcid = GetThreadLocale ();
#endif

  /* interferes with g_win32_getlocale() */
  save_and_clear_env ("LC_ALL", &old_lc_all);
  save_and_clear_env ("LC_MESSAGES", &old_lc_messages);
  save_and_clear_env ("LANG", &old_lang);

  filename = g_test_build_filename (G_TEST_DIST, "casemap.txt", NULL);
  infile = g_fopen (filename, "re");
  g_assert (infile != NULL);

  while (fgets (buffer, sizeof (buffer), infile))
    {
      line++;
      if (buffer[0] == '#')
        continue;

      strings = g_strsplit (buffer, "\t", -1);
      locale = strings[0];
      if (!locale[0])
        locale = "C";

      if (strcmp (locale, current_locale) != 0)
        {
          setlocale (LC_CTYPE, locale);
          current_locale = setlocale (LC_CTYPE, NULL);

          if (strncmp (current_locale, locale, 2) != 0)
            {
              g_test_message ("Cannot set locale to %s, skipping", locale);
              goto next;
            }
        }

#ifdef G_OS_WIN32
      if (strstr (locale, "lt_LT"))
        SetThreadLocale (MAKELCID (MAKELANGID (LANG_LITHUANIAN, SUBLANG_LITHUANIAN), SORT_DEFAULT));
      else if (strstr (locale, "tr_TR"))
        SetThreadLocale (MAKELCID (MAKELANGID (LANG_TURKISH, SUBLANG_TURKISH_TURKEY), SORT_DEFAULT));
      else if (strstr (locale, "az_AZ"))
        SetThreadLocale (MAKELCID (MAKELANGID (LANG_AZERBAIJANI, SUBLANG_AZERBAIJANI_AZERBAIJAN_LATIN), SORT_DEFAULT));
      else
        SetThreadLocale (old_lcid);
#endif

      test = strings[1];

      /* gen-casemap-txt.py uses an empty string when a single
       * character doesn't have an equivalent in a particular case;
       * since that behavior is nonsense for multicharacter strings,
       * it would make more sense to put the expected result ... the
       * original character unchanged. But for now, we just work
       * around it here and take the empty string to mean "same as
       * original"
       */

      convert = g_utf8_strup (test, -1);
      expected = strings[4][0] ? strings[4] : test;
      g_test_message ("Converting '%s' => '%s' [expected '%s'] "
                      "(line %" G_GSIZE_FORMAT ")",
                      test, convert, expected, line);

      g_assert_cmpstr (convert, ==, expected);
      g_free (convert);

      convert = g_utf8_strdown (test, -1);
      expected = strings[2][0] ? strings[2] : test;
      g_assert_cmpstr (convert, ==, expected);
      g_free (convert);

    next:
      g_strfreev (strings);
    }

  fclose (infile);

  g_free (filename);
  filename = g_test_build_filename (G_TEST_DIST, "casefold.txt", NULL);

  infile = g_fopen (filename, "re");
  g_assert (infile != NULL);
  line = 0;

  while (fgets (buffer, sizeof (buffer), infile))
    {
      line++;
      if (buffer[0] == '#')
        continue;

      buffer[strlen (buffer) - 1] = '\0';
      strings = g_strsplit (buffer, "\t", -1);

      test = strings[0];

      convert = g_utf8_casefold (test, -1);
      g_test_message ("Converting '%s' => '%s' [expected '%s'] "
                      "(line %" G_GSIZE_FORMAT ")",
                      test, convert, strings[1], line);

      g_assert_cmpstr (convert, ==, strings[1]);
      g_free (convert);

      g_strfreev (strings);
    }

  fclose (infile);
  g_free (filename);

  if (old_lc_all)
    g_setenv ("LC_ALL", old_lc_all, TRUE);
  if (old_lc_messages)
    g_setenv ("LC_MESSAGES", old_lc_messages, TRUE);
  if (old_lang)
    g_setenv ("LANG", old_lang, TRUE);
  g_free (old_lc_all);
  g_free (old_lc_messages);
  g_free (old_lang);
#ifdef G_OS_WIN32
  SetThreadLocale (old_lcid);
#endif
}

/* Test that g_unichar_ismark() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_mark (void)
{
  g_assert_true (g_unichar_ismark (0x0903));
  g_assert_true (g_unichar_ismark (0x20DD));
  g_assert_true (g_unichar_ismark (0xA806));
  g_assert_false (g_unichar_ismark ('a'));

  /*** Testing TYPE() border cases ***/
  g_assert_false (g_unichar_ismark (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > G_UNICODE_MAX_TABLE_INDEX) */
  g_assert_false (g_unichar_ismark (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (g_unichar_ismark (0xE0001));
  g_assert_false (g_unichar_ismark (G_UNICODE_LAST_CHAR));
  g_assert_false (g_unichar_ismark (G_UNICODE_LAST_CHAR + 1));
  g_assert_false (g_unichar_ismark (G_UNICODE_LAST_CHAR_PART1));
  g_assert_false (g_unichar_ismark (G_UNICODE_LAST_CHAR_PART1 + 1));
}

/* Test that g_unichar_isspace() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_space (void)
{
  g_assert_false (g_unichar_isspace ('a'));
  g_assert_true (g_unichar_isspace (' '));
  g_assert_true (g_unichar_isspace ('\t'));
  g_assert_true (g_unichar_isspace ('\n'));
  g_assert_true (g_unichar_isspace ('\r'));
  g_assert_true (g_unichar_isspace ('\f'));
  g_assert_false (g_unichar_isspace (0xff41)); /* Unicode fullwidth 'a' */
  g_assert_true (g_unichar_isspace (0x202F)); /* Unicode space separator */
  g_assert_true (g_unichar_isspace (0x2028)); /* Unicode line separator */
  g_assert_true (g_unichar_isspace (0x2029)); /* Unicode paragraph separator */

  /*** Testing TYPE() border cases ***/
  g_assert_false (g_unichar_isspace (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > G_UNICODE_MAX_TABLE_INDEX) */
  g_assert_false (g_unichar_isspace (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (g_unichar_isspace (0xE0001));
  g_assert_false (g_unichar_isspace (G_UNICODE_LAST_CHAR));
  g_assert_false (g_unichar_isspace (G_UNICODE_LAST_CHAR + 1));
  g_assert_false (g_unichar_isspace (G_UNICODE_LAST_CHAR_PART1));
  g_assert_false (g_unichar_isspace (G_UNICODE_LAST_CHAR_PART1 + 1));
}

/* Test that g_unichar_isalnum() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_alnum (void)
{
  g_assert_false (g_unichar_isalnum (' '));
  g_assert_true (g_unichar_isalnum ('a'));
  g_assert_true (g_unichar_isalnum ('z'));
  g_assert_true (g_unichar_isalnum ('0'));
  g_assert_true (g_unichar_isalnum ('9'));
  g_assert_true (g_unichar_isalnum ('A'));
  g_assert_true (g_unichar_isalnum ('Z'));
  g_assert_false (g_unichar_isalnum ('-'));
  g_assert_false (g_unichar_isalnum ('*'));
  g_assert_true (g_unichar_isalnum (0xFF21));  /* Unichar fullwidth 'A' */
  g_assert_true (g_unichar_isalnum (0xFF3A));  /* Unichar fullwidth 'Z' */
  g_assert_true (g_unichar_isalnum (0xFF41));  /* Unichar fullwidth 'a' */
  g_assert_true (g_unichar_isalnum (0xFF5A));  /* Unichar fullwidth 'z' */
  g_assert_true (g_unichar_isalnum (0xFF10));  /* Unichar fullwidth '0' */
  g_assert_true (g_unichar_isalnum (0xFF19));  /* Unichar fullwidth '9' */
  g_assert_false (g_unichar_isalnum (0xFF0A)); /* Unichar fullwidth '*' */

  /*** Testing TYPE() border cases ***/
  g_assert_true (g_unichar_isalnum (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > G_UNICODE_MAX_TABLE_INDEX) */
  g_assert_false (g_unichar_isalnum (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (g_unichar_isalnum (0xE0001));
  g_assert_false (g_unichar_isalnum (G_UNICODE_LAST_CHAR));
  g_assert_false (g_unichar_isalnum (G_UNICODE_LAST_CHAR + 1));
  g_assert_false (g_unichar_isalnum (G_UNICODE_LAST_CHAR_PART1));
  g_assert_false (g_unichar_isalnum (G_UNICODE_LAST_CHAR_PART1 + 1));
}

/* Test that g_unichar_isalpha() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_alpha (void)
{
  g_assert_false (g_unichar_isalpha (' '));
  g_assert_true (g_unichar_isalpha ('a'));
  g_assert_true (g_unichar_isalpha ('z'));
  g_assert_false (g_unichar_isalpha ('0'));
  g_assert_false (g_unichar_isalpha ('9'));
  g_assert_true (g_unichar_isalpha ('A'));
  g_assert_true (g_unichar_isalpha ('Z'));
  g_assert_false (g_unichar_isalpha ('-'));
  g_assert_false (g_unichar_isalpha ('*'));
  g_assert_true (g_unichar_isalpha (0xFF21));  /* Unichar fullwidth 'A' */
  g_assert_true (g_unichar_isalpha (0xFF3A));  /* Unichar fullwidth 'Z' */
  g_assert_true (g_unichar_isalpha (0xFF41));  /* Unichar fullwidth 'a' */
  g_assert_true (g_unichar_isalpha (0xFF5A));  /* Unichar fullwidth 'z' */
  g_assert_false (g_unichar_isalpha (0xFF10)); /* Unichar fullwidth '0' */
  g_assert_false (g_unichar_isalpha (0xFF19)); /* Unichar fullwidth '9' */
  g_assert_false (g_unichar_isalpha (0xFF0A)); /* Unichar fullwidth '*' */

  /*** Testing TYPE() border cases ***/
  g_assert_true (g_unichar_isalpha (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > G_UNICODE_MAX_TABLE_INDEX) */
  g_assert_false (g_unichar_isalpha (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (g_unichar_isalpha (0xE0001));
  g_assert_false (g_unichar_isalpha (G_UNICODE_LAST_CHAR));
  g_assert_false (g_unichar_isalpha (G_UNICODE_LAST_CHAR + 1));
  g_assert_false (g_unichar_isalpha (G_UNICODE_LAST_CHAR_PART1));
  g_assert_false (g_unichar_isalpha (G_UNICODE_LAST_CHAR_PART1 + 1));
}

/* Test that g_unichar_isdigit() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_digit (void)
{
  g_assert_false (g_unichar_isdigit (' '));
  g_assert_false (g_unichar_isdigit ('a'));
  g_assert_true (g_unichar_isdigit ('0'));
  g_assert_true (g_unichar_isdigit ('9'));
  g_assert_false (g_unichar_isdigit ('A'));
  g_assert_false (g_unichar_isdigit ('-'));
  g_assert_false (g_unichar_isdigit ('*'));
  g_assert_false (g_unichar_isdigit (0xFF21)); /* Unichar fullwidth 'A' */
  g_assert_false (g_unichar_isdigit (0xFF3A)); /* Unichar fullwidth 'Z' */
  g_assert_false (g_unichar_isdigit (0xFF41)); /* Unichar fullwidth 'a' */
  g_assert_false (g_unichar_isdigit (0xFF5A)); /* Unichar fullwidth 'z' */
  g_assert_true (g_unichar_isdigit (0xFF10));  /* Unichar fullwidth '0' */
  g_assert_true (g_unichar_isdigit (0xFF19));  /* Unichar fullwidth '9' */
  g_assert_false (g_unichar_isdigit (0xFF0A)); /* Unichar fullwidth '*' */

  /*** Testing TYPE() border cases ***/
  g_assert_false (g_unichar_isdigit (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > G_UNICODE_MAX_TABLE_INDEX) */
  g_assert_false (g_unichar_isdigit (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (g_unichar_isdigit (0xE0001));
  g_assert_false (g_unichar_isdigit (G_UNICODE_LAST_CHAR));
  g_assert_false (g_unichar_isdigit (G_UNICODE_LAST_CHAR + 1));
  g_assert_false (g_unichar_isdigit (G_UNICODE_LAST_CHAR_PART1));
  g_assert_false (g_unichar_isdigit (G_UNICODE_LAST_CHAR_PART1 + 1));
}

/* Test that g_unichar_digit_value() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_digit_value (void)
{
  g_assert_cmpint (g_unichar_digit_value (' '), ==, -1);
  g_assert_cmpint (g_unichar_digit_value ('a'), ==, -1);
  g_assert_cmpint (g_unichar_digit_value ('0'), ==, 0);
  g_assert_cmpint (g_unichar_digit_value ('9'), ==, 9);
  g_assert_cmpint (g_unichar_digit_value ('A'), ==, -1);
  g_assert_cmpint (g_unichar_digit_value ('-'), ==, -1);
  g_assert_cmpint (g_unichar_digit_value (0xFF21), ==, -1); /* Unichar 'A' */
  g_assert_cmpint (g_unichar_digit_value (0xFF3A), ==, -1); /* Unichar 'Z' */
  g_assert_cmpint (g_unichar_digit_value (0xFF41), ==, -1); /* Unichar 'a' */
  g_assert_cmpint (g_unichar_digit_value (0xFF5A), ==, -1); /* Unichar 'z' */
  g_assert_cmpint (g_unichar_digit_value (0xFF10), ==, 0);  /* Unichar '0' */
  g_assert_cmpint (g_unichar_digit_value (0xFF19), ==, 9);  /* Unichar '9' */
  g_assert_cmpint (g_unichar_digit_value (0xFF0A), ==, -1); /* Unichar '*' */

  /*** Testing TYPE() border cases ***/
  g_assert_cmpint (g_unichar_digit_value (0x3FF5), ==, -1);
   /* U+FFEFF Plane 15 Private Use (needed to be > G_UNICODE_MAX_TABLE_INDEX) */
  g_assert_cmpint (g_unichar_digit_value (0xFFEFF), ==, -1);
  /* U+E0001 Language Tag */
  g_assert_cmpint (g_unichar_digit_value (0xE0001), ==, -1);
  g_assert_cmpint (g_unichar_digit_value (G_UNICODE_LAST_CHAR), ==, -1);
  g_assert_cmpint (g_unichar_digit_value (G_UNICODE_LAST_CHAR + 1), ==, -1);
  g_assert_cmpint (g_unichar_digit_value (G_UNICODE_LAST_CHAR_PART1), ==, -1);
  g_assert_cmpint (g_unichar_digit_value (G_UNICODE_LAST_CHAR_PART1 + 1), ==, -1);
}

/* Test that g_unichar_isxdigit() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_xdigit (void)
{
  g_assert_false (g_unichar_isxdigit (' '));
  g_assert_true (g_unichar_isxdigit ('a'));
  g_assert_true (g_unichar_isxdigit ('f'));
  g_assert_false (g_unichar_isxdigit ('g'));
  g_assert_false (g_unichar_isxdigit ('z'));
  g_assert_true (g_unichar_isxdigit ('0'));
  g_assert_true (g_unichar_isxdigit ('9'));
  g_assert_true (g_unichar_isxdigit ('A'));
  g_assert_true (g_unichar_isxdigit ('F'));
  g_assert_false (g_unichar_isxdigit ('G'));
  g_assert_false (g_unichar_isxdigit ('Z'));
  g_assert_false (g_unichar_isxdigit ('-'));
  g_assert_false (g_unichar_isxdigit ('*'));
  g_assert_true (g_unichar_isxdigit (0xFF21));  /* Unichar fullwidth 'A' */
  g_assert_true (g_unichar_isxdigit (0xFF26));  /* Unichar fullwidth 'F' */
  g_assert_false (g_unichar_isxdigit (0xFF27)); /* Unichar fullwidth 'G' */
  g_assert_false (g_unichar_isxdigit (0xFF3A)); /* Unichar fullwidth 'Z' */
  g_assert_true (g_unichar_isxdigit (0xFF41));  /* Unichar fullwidth 'a' */
  g_assert_true (g_unichar_isxdigit (0xFF46));  /* Unichar fullwidth 'f' */
  g_assert_false (g_unichar_isxdigit (0xFF47)); /* Unichar fullwidth 'g' */
  g_assert_false (g_unichar_isxdigit (0xFF5A)); /* Unichar fullwidth 'z' */
  g_assert_true (g_unichar_isxdigit (0xFF10));  /* Unichar fullwidth '0' */
  g_assert_true (g_unichar_isxdigit (0xFF19));  /* Unichar fullwidth '9' */
  g_assert_false (g_unichar_isxdigit (0xFF0A)); /* Unichar fullwidth '*' */

  /*** Testing TYPE() border cases ***/
  g_assert_false (g_unichar_isxdigit (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > G_UNICODE_MAX_TABLE_INDEX) */
  g_assert_false (g_unichar_isxdigit (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (g_unichar_isxdigit (0xE0001));
  g_assert_false (g_unichar_isxdigit (G_UNICODE_LAST_CHAR));
  g_assert_false (g_unichar_isxdigit (G_UNICODE_LAST_CHAR + 1));
  g_assert_false (g_unichar_isxdigit (G_UNICODE_LAST_CHAR_PART1));
  g_assert_false (g_unichar_isxdigit (G_UNICODE_LAST_CHAR_PART1 + 1));
}

/* Test that g_unichar_xdigit_value() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_xdigit_value (void)
{
  g_assert_cmpint (g_unichar_xdigit_value (' '), ==, -1);
  g_assert_cmpint (g_unichar_xdigit_value ('a'), ==, 10);
  g_assert_cmpint (g_unichar_xdigit_value ('f'), ==, 15);
  g_assert_cmpint (g_unichar_xdigit_value ('g'), ==, -1);
  g_assert_cmpint (g_unichar_xdigit_value ('0'), ==, 0);
  g_assert_cmpint (g_unichar_xdigit_value ('9'), ==, 9);
  g_assert_cmpint (g_unichar_xdigit_value ('A'), ==, 10);
  g_assert_cmpint (g_unichar_xdigit_value ('F'), ==, 15);
  g_assert_cmpint (g_unichar_xdigit_value ('G'), ==, -1);
  g_assert_cmpint (g_unichar_xdigit_value ('-'), ==, -1);
  g_assert_cmpint (g_unichar_xdigit_value (0xFF21), ==, 10); /* Unichar 'A' */
  g_assert_cmpint (g_unichar_xdigit_value (0xFF26), ==, 15); /* Unichar 'F' */
  g_assert_cmpint (g_unichar_xdigit_value (0xFF27), ==, -1); /* Unichar 'G' */
  g_assert_cmpint (g_unichar_xdigit_value (0xFF3A), ==, -1); /* Unichar 'Z' */
  g_assert_cmpint (g_unichar_xdigit_value (0xFF41), ==, 10); /* Unichar 'a' */
  g_assert_cmpint (g_unichar_xdigit_value (0xFF46), ==, 15); /* Unichar 'f' */
  g_assert_cmpint (g_unichar_xdigit_value (0xFF47), ==, -1); /* Unichar 'g' */
  g_assert_cmpint (g_unichar_xdigit_value (0xFF5A), ==, -1); /* Unichar 'z' */
  g_assert_cmpint (g_unichar_xdigit_value (0xFF10), ==, 0);  /* Unichar '0' */
  g_assert_cmpint (g_unichar_xdigit_value (0xFF19), ==, 9);  /* Unichar '9' */
  g_assert_cmpint (g_unichar_xdigit_value (0xFF0A), ==, -1); /* Unichar '*' */

  /*** Testing TYPE() border cases ***/
  g_assert_cmpint (g_unichar_xdigit_value (0x3FF5), ==, -1);
   /* U+FFEFF Plane 15 Private Use (needed to be > G_UNICODE_MAX_TABLE_INDEX) */
  g_assert_cmpint (g_unichar_xdigit_value (0xFFEFF), ==, -1);
  /* U+E0001 Language Tag */
  g_assert_cmpint (g_unichar_xdigit_value (0xE0001), ==, -1);
  g_assert_cmpint (g_unichar_xdigit_value (G_UNICODE_LAST_CHAR), ==, -1);
  g_assert_cmpint (g_unichar_xdigit_value (G_UNICODE_LAST_CHAR + 1), ==, -1);
  g_assert_cmpint (g_unichar_xdigit_value (G_UNICODE_LAST_CHAR_PART1), ==, -1);
  g_assert_cmpint (g_unichar_xdigit_value (G_UNICODE_LAST_CHAR_PART1 + 1), ==, -1);
}

/* Test that g_unichar_ispunct() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_punctuation (void)
{
  g_assert_false (g_unichar_ispunct (' '));
  g_assert_false (g_unichar_ispunct ('a'));
  g_assert_true (g_unichar_ispunct ('.'));
  g_assert_true (g_unichar_ispunct (','));
  g_assert_true (g_unichar_ispunct (';'));
  g_assert_true (g_unichar_ispunct (':'));
  g_assert_true (g_unichar_ispunct ('-'));

  g_assert_false (g_unichar_ispunct (0xFF21)); /* Unichar fullwidth 'A' */
  g_assert_true (g_unichar_ispunct (0x005F));  /* Unichar fullwidth '.' */
  g_assert_true (g_unichar_ispunct (0x058A));  /* Unichar fullwidth '-' */

  /*** Testing TYPE() border cases ***/
  g_assert_false (g_unichar_ispunct (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > G_UNICODE_MAX_TABLE_INDEX) */
  g_assert_false (g_unichar_ispunct (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (g_unichar_ispunct (0xE0001));
  g_assert_false (g_unichar_ispunct (G_UNICODE_LAST_CHAR));
  g_assert_false (g_unichar_ispunct (G_UNICODE_LAST_CHAR + 1));
  g_assert_false (g_unichar_ispunct (G_UNICODE_LAST_CHAR_PART1));
  g_assert_false (g_unichar_ispunct (G_UNICODE_LAST_CHAR_PART1 + 1));
}

/* Test that g_unichar_iscntrl() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_cntrl (void)
{
  g_assert_true (g_unichar_iscntrl (0x08));
  g_assert_false (g_unichar_iscntrl ('a'));
  g_assert_true (g_unichar_iscntrl (0x007F)); /* Unichar fullwidth <del> */
  g_assert_true (g_unichar_iscntrl (0x009F)); /* Unichar fullwidth control */

  /*** Testing TYPE() border cases ***/
  g_assert_false (g_unichar_iscntrl (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > G_UNICODE_MAX_TABLE_INDEX) */
  g_assert_false (g_unichar_iscntrl (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (g_unichar_iscntrl (0xE0001));
  g_assert_false (g_unichar_iscntrl (G_UNICODE_LAST_CHAR));
  g_assert_false (g_unichar_iscntrl (G_UNICODE_LAST_CHAR + 1));
  g_assert_false (g_unichar_iscntrl (G_UNICODE_LAST_CHAR_PART1));
  g_assert_false (g_unichar_iscntrl (G_UNICODE_LAST_CHAR_PART1 + 1));
}

/* Test that g_unichar_isgraph() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_graph (void)
{
  g_assert_false (g_unichar_isgraph (0x08));
  g_assert_false (g_unichar_isgraph (' '));
  g_assert_true (g_unichar_isgraph ('a'));
  g_assert_true (g_unichar_isgraph ('0'));
  g_assert_true (g_unichar_isgraph ('9'));
  g_assert_true (g_unichar_isgraph ('A'));
  g_assert_true (g_unichar_isgraph ('-'));
  g_assert_true (g_unichar_isgraph ('*'));
  g_assert_true (g_unichar_isgraph (0xFF21));  /* Unichar fullwidth 'A' */
  g_assert_true (g_unichar_isgraph (0xFF3A));  /* Unichar fullwidth 'Z' */
  g_assert_true (g_unichar_isgraph (0xFF41));  /* Unichar fullwidth 'a' */
  g_assert_true (g_unichar_isgraph (0xFF5A));  /* Unichar fullwidth 'z' */
  g_assert_true (g_unichar_isgraph (0xFF10));  /* Unichar fullwidth '0' */
  g_assert_true (g_unichar_isgraph (0xFF19));  /* Unichar fullwidth '9' */
  g_assert_true (g_unichar_isgraph (0xFF0A));  /* Unichar fullwidth '*' */
  g_assert_false (g_unichar_isgraph (0x007F)); /* Unichar fullwidth <del> */
  g_assert_false (g_unichar_isgraph (0x009F)); /* Unichar fullwidth control */

  /*** Testing TYPE() border cases ***/
  g_assert_true (g_unichar_isgraph (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > G_UNICODE_MAX_TABLE_INDEX) */
  g_assert_true (g_unichar_isgraph (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (g_unichar_isgraph (0xE0001));
  g_assert_false (g_unichar_isgraph (G_UNICODE_LAST_CHAR));
  g_assert_false (g_unichar_isgraph (G_UNICODE_LAST_CHAR + 1));
  g_assert_false (g_unichar_isgraph (G_UNICODE_LAST_CHAR_PART1));
  g_assert_false (g_unichar_isgraph (G_UNICODE_LAST_CHAR_PART1 + 1));
}

/* Test that g_unichar_iszerowidth() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_zerowidth (void)
{
  g_assert_false (g_unichar_iszerowidth (0x00AD));
  g_assert_false (g_unichar_iszerowidth (0x115F));
  g_assert_true (g_unichar_iszerowidth (0x1160));
  g_assert_true (g_unichar_iszerowidth (0x11AA));
  g_assert_true (g_unichar_iszerowidth (0x11FF));
  g_assert_false (g_unichar_iszerowidth (0x1200));
  g_assert_false (g_unichar_iszerowidth (0x200A));
  g_assert_true (g_unichar_iszerowidth (0x200B));
  g_assert_true (g_unichar_iszerowidth (0x200C));
  g_assert_true (g_unichar_iszerowidth (0x591));

  /*** Testing TYPE() border cases ***/
  g_assert_false (g_unichar_iszerowidth (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > G_UNICODE_MAX_TABLE_INDEX) */
  g_assert_false (g_unichar_iszerowidth (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_true (g_unichar_iszerowidth (0xE0001));
  g_assert_false (g_unichar_iszerowidth (G_UNICODE_LAST_CHAR));
  g_assert_false (g_unichar_iszerowidth (G_UNICODE_LAST_CHAR + 1));
  g_assert_false (g_unichar_iszerowidth (G_UNICODE_LAST_CHAR_PART1));
  g_assert_false (g_unichar_iszerowidth (G_UNICODE_LAST_CHAR_PART1 + 1));

  /* Hangul Jamo Extended-B block, containing jungseong and jongseong for
   * Old Korean */
  g_assert_true (g_unichar_iszerowidth (0xD7B0));
  g_assert_true (g_unichar_iszerowidth (0xD7FB));
}

/* Test that g_unichar_istitle() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_title (void)
{
  g_assert_true (g_unichar_istitle (0x01c5));
  g_assert_true (g_unichar_istitle (0x1f88));
  g_assert_true (g_unichar_istitle (0x1fcc));
  g_assert_false (g_unichar_istitle ('a'));
  g_assert_false (g_unichar_istitle ('A'));
  g_assert_false (g_unichar_istitle (';'));

  /*** Testing TYPE() border cases ***/
  g_assert_false (g_unichar_istitle (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > G_UNICODE_MAX_TABLE_INDEX) */
  g_assert_false (g_unichar_istitle (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (g_unichar_istitle (0xE0001));
  g_assert_false (g_unichar_istitle (G_UNICODE_LAST_CHAR));
  g_assert_false (g_unichar_istitle (G_UNICODE_LAST_CHAR + 1));
  g_assert_false (g_unichar_istitle (G_UNICODE_LAST_CHAR_PART1));
  g_assert_false (g_unichar_istitle (G_UNICODE_LAST_CHAR_PART1 + 1));

  g_assert_cmphex (g_unichar_totitle (0x0000), ==, 0x0000);
  g_assert_cmphex (g_unichar_totitle (0x01c6), ==, 0x01c5);
  g_assert_cmphex (g_unichar_totitle (0x01c4), ==, 0x01c5);
  g_assert_cmphex (g_unichar_totitle (0x01c5), ==, 0x01c5);
  g_assert_cmphex (g_unichar_totitle (0x1f80), ==, 0x1f88);
  g_assert_cmphex (g_unichar_totitle (0x1f88), ==, 0x1f88);
  g_assert_cmphex (g_unichar_totitle ('a'), ==, 'A');
  g_assert_cmphex (g_unichar_totitle ('A'), ==, 'A');

  /*** Testing TYPE() border cases ***/
  g_assert_cmphex (g_unichar_totitle (0x3FF5), ==, 0x3FF5);
  /* U+FFEFF Plane 15 Private Use (needed to be > G_UNICODE_MAX_TABLE_INDEX) */
  g_assert_cmphex (g_unichar_totitle (0xFFEFF), ==, 0xFFEFF);
  g_assert_cmphex (g_unichar_totitle (0xDFFFF), ==, 0xDFFFF);
  /* U+E0001 Language Tag */
  g_assert_cmphex (g_unichar_totitle (0xE0001), ==, 0xE0001);
  g_assert_cmphex (g_unichar_totitle (G_UNICODE_LAST_CHAR), ==,
                   G_UNICODE_LAST_CHAR);
  g_assert_cmphex (g_unichar_totitle (G_UNICODE_LAST_CHAR + 1), ==,
                   (G_UNICODE_LAST_CHAR + 1));
  g_assert_cmphex (g_unichar_totitle (G_UNICODE_LAST_CHAR_PART1), ==,
                   (G_UNICODE_LAST_CHAR_PART1));
  g_assert_cmphex (g_unichar_totitle (G_UNICODE_LAST_CHAR_PART1 + 1), ==,
                   (G_UNICODE_LAST_CHAR_PART1 + 1));
}

/* Test that g_unichar_isupper() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_upper (void)
{
  g_assert_false (g_unichar_isupper (' '));
  g_assert_false (g_unichar_isupper ('0'));
  g_assert_false (g_unichar_isupper ('a'));
  g_assert_true (g_unichar_isupper ('A'));
  g_assert_false (g_unichar_isupper (0xff41)); /* Unicode fullwidth 'a' */
  g_assert_true (g_unichar_isupper (0xff21)); /* Unicode fullwidth 'A' */

  /*** Testing TYPE() border cases ***/
  g_assert_false (g_unichar_isupper (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > G_UNICODE_MAX_TABLE_INDEX) */
  g_assert_false (g_unichar_isupper (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (g_unichar_isupper (0xE0001));
  g_assert_false (g_unichar_isupper (G_UNICODE_LAST_CHAR));
  g_assert_false (g_unichar_isupper (G_UNICODE_LAST_CHAR + 1));
  g_assert_false (g_unichar_isupper (G_UNICODE_LAST_CHAR_PART1));
  g_assert_false (g_unichar_isupper (G_UNICODE_LAST_CHAR_PART1 + 1));
}

/* Test that g_unichar_islower() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_lower (void)
{
  g_assert_false (g_unichar_islower (' '));
  g_assert_false (g_unichar_islower ('0'));
  g_assert_true (g_unichar_islower ('a'));
  g_assert_false (g_unichar_islower ('A'));
  g_assert_true (g_unichar_islower (0xff41)); /* Unicode fullwidth 'a' */
  g_assert_false (g_unichar_islower (0xff21)); /* Unicode fullwidth 'A' */

  /*** Testing TYPE() border cases ***/
  g_assert_false (g_unichar_islower (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > G_UNICODE_MAX_TABLE_INDEX) */
  g_assert_false (g_unichar_islower (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (g_unichar_islower (0xE0001));
  g_assert_false (g_unichar_islower (G_UNICODE_LAST_CHAR));
  g_assert_false (g_unichar_islower (G_UNICODE_LAST_CHAR + 1));
  g_assert_false (g_unichar_islower (G_UNICODE_LAST_CHAR_PART1));
  g_assert_false (g_unichar_islower (G_UNICODE_LAST_CHAR_PART1 + 1));
}

/* Test that g_unichar_isprint() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_print (void)
{
  g_assert_true (g_unichar_isprint (' '));
  g_assert_true (g_unichar_isprint ('0'));
  g_assert_true (g_unichar_isprint ('a'));
  g_assert_true (g_unichar_isprint ('A'));
  g_assert_true (g_unichar_isprint (0xff41)); /* Unicode fullwidth 'a' */
  g_assert_true (g_unichar_isprint (0xff21)); /* Unicode fullwidth 'A' */

  /*** Testing TYPE() border cases ***/
  g_assert_true (g_unichar_isprint (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > G_UNICODE_MAX_TABLE_INDEX) */
  g_assert_true (g_unichar_isprint (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (g_unichar_isprint (0xE0001));
  g_assert_false (g_unichar_isprint (G_UNICODE_LAST_CHAR));
  g_assert_false (g_unichar_isprint (G_UNICODE_LAST_CHAR + 1));
  g_assert_false (g_unichar_isprint (G_UNICODE_LAST_CHAR_PART1));
  g_assert_false (g_unichar_isprint (G_UNICODE_LAST_CHAR_PART1 + 1));
}

/* Test that g_unichar_toupper() and g_unichar_tolower() return the
 * correct values for various ASCII and Unicode alphabetic, numeric,
 * and other, codepoints. */
static void
test_cases (void)
{
  g_assert_cmphex (g_unichar_toupper (0x0), ==, 0x0);
  g_assert_cmphex (g_unichar_tolower (0x0), ==, 0x0);
  g_assert_cmphex (g_unichar_toupper ('a'), ==, 'A');
  g_assert_cmphex (g_unichar_toupper ('A'), ==, 'A');
  /* Unicode fullwidth 'a' == 'A' */
  g_assert_cmphex (g_unichar_toupper (0xff41), ==, 0xff21);
  /* Unicode fullwidth 'A' == 'A' */
  g_assert_cmphex (g_unichar_toupper (0xff21), ==, 0xff21);
  g_assert_cmphex (g_unichar_toupper (0x01C5), ==, 0x01C4);
  g_assert_cmphex (g_unichar_toupper (0x01C6), ==, 0x01C4);
  g_assert_cmphex (g_unichar_tolower ('A'), ==, 'a');
  g_assert_cmphex (g_unichar_tolower ('a'), ==, 'a');
  /* Unicode fullwidth 'A' == 'a' */
  g_assert_cmphex (g_unichar_tolower (0xff21), ==, 0xff41);
  /* Unicode fullwidth 'a' == 'a' */
  g_assert_cmphex (g_unichar_tolower (0xff41), ==, 0xff41);
  g_assert_cmphex (g_unichar_tolower (0x01C4), ==, 0x01C6);
  g_assert_cmphex (g_unichar_tolower (0x01C5), ==, 0x01C6);
  g_assert_cmphex (g_unichar_tolower (0x1F8A), ==, 0x1F82);
  g_assert_cmphex (g_unichar_totitle (0x1F8A), ==, 0x1F8A);
  g_assert_cmphex (g_unichar_toupper (0x1F8A), ==, 0x1F8A);
  g_assert_cmphex (g_unichar_tolower (0x1FB2), ==, 0x1FB2);
  g_assert_cmphex (g_unichar_toupper (0x1FB2), ==, 0x1FB2);

  /* U+130 is a special case, it's a 'I' with a dot on top */
  g_assert_cmphex (g_unichar_tolower (0x130), ==, 0x69);

  /* Testing ATTTABLE() border cases */
  g_assert_cmphex (g_unichar_toupper (0x1D6FE), ==, 0x1D6FE);

  /*** Testing TYPE() border cases ***/
  g_assert_cmphex (g_unichar_toupper (0x3FF5), ==, 0x3FF5);
  /* U+FFEFF Plane 15 Private Use (needed to be > G_UNICODE_MAX_TABLE_INDEX) */
  g_assert_cmphex (g_unichar_toupper (0xFFEFF), ==, 0xFFEFF);
  g_assert_cmphex (g_unichar_toupper (0xDFFFF), ==, 0xDFFFF);
  /* U+E0001 Language Tag */
  g_assert_cmphex (g_unichar_toupper (0xE0001), ==, 0xE0001);
  g_assert_cmphex (g_unichar_toupper (G_UNICODE_LAST_CHAR), ==,
                   G_UNICODE_LAST_CHAR);
  g_assert_cmphex (g_unichar_toupper (G_UNICODE_LAST_CHAR + 1), ==,
                   (G_UNICODE_LAST_CHAR + 1));
  g_assert_cmphex (g_unichar_toupper (G_UNICODE_LAST_CHAR_PART1), ==,
                   (G_UNICODE_LAST_CHAR_PART1));
  g_assert_cmphex (g_unichar_toupper (G_UNICODE_LAST_CHAR_PART1 + 1), ==,
                   (G_UNICODE_LAST_CHAR_PART1 + 1));

  /* Testing ATTTABLE() border cases */
  g_assert_cmphex (g_unichar_tolower (0x1D6FA), ==, 0x1D6FA);

  /*** Testing TYPE() border cases ***/
  g_assert_cmphex (g_unichar_tolower (0x3FF5), ==, 0x3FF5);
  /* U+FFEFF Plane 15 Private Use (needed to be > G_UNICODE_MAX_TABLE_INDEX) */
  g_assert_cmphex (g_unichar_tolower (0xFFEFF), ==, 0xFFEFF);
  g_assert_cmphex (g_unichar_tolower (0xDFFFF), ==, 0xDFFFF);
  /* U+E0001 Language Tag */
  g_assert_cmphex (g_unichar_tolower (0xE0001), ==, 0xE0001);
  g_assert_cmphex (g_unichar_tolower (G_UNICODE_LAST_CHAR), ==,
                   G_UNICODE_LAST_CHAR);
  g_assert_cmphex (g_unichar_tolower (G_UNICODE_LAST_CHAR + 1), ==,
                   (G_UNICODE_LAST_CHAR + 1));
  g_assert_cmphex (g_unichar_tolower (G_UNICODE_LAST_CHAR_PART1), ==,
                   G_UNICODE_LAST_CHAR_PART1);
  g_assert_cmphex (g_unichar_tolower (G_UNICODE_LAST_CHAR_PART1 + 1), ==,
                   (G_UNICODE_LAST_CHAR_PART1 + 1));
}

/* Test that g_unichar_isdefined() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_defined (void)
{
  g_assert_true (g_unichar_isdefined (0x0903));
  g_assert_true (g_unichar_isdefined (0x20DD));
  g_assert_true (g_unichar_isdefined (0x20BA));
  g_assert_true (g_unichar_isdefined (0xA806));
  g_assert_true (g_unichar_isdefined ('a'));
  g_assert_false (g_unichar_isdefined (0x10C49));
  g_assert_false (g_unichar_isdefined (0x169D));

  /*** Testing TYPE() border cases ***/
  g_assert_true (g_unichar_isdefined (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > G_UNICODE_MAX_TABLE_INDEX) */
  g_assert_true (g_unichar_isdefined (0xFFEFF));
  g_assert_false (g_unichar_isdefined (0xDFFFF));
  /* U+E0001 Language Tag */
  g_assert_true (g_unichar_isdefined (0xE0001));
  g_assert_false (g_unichar_isdefined (G_UNICODE_LAST_CHAR));
  g_assert_false (g_unichar_isdefined (G_UNICODE_LAST_CHAR + 1));
  g_assert_false (g_unichar_isdefined (G_UNICODE_LAST_CHAR_PART1));
  g_assert_false (g_unichar_isdefined (G_UNICODE_LAST_CHAR_PART1 + 1));
}

/* Test that g_unichar_iswide() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_wide (void)
{
  guint i;
  struct {
    gunichar c;
    enum {
      NOT_WIDE,
      WIDE_CJK,
      WIDE
    } wide;
  } examples[] = {
    /* Neutral */
    {   0x0000, NOT_WIDE },
    {   0x0483, NOT_WIDE },
    {   0x0641, NOT_WIDE },
    {   0xFFFC, NOT_WIDE },
    {  0x10000, NOT_WIDE },
    {  0xE0001, NOT_WIDE },
    {  0x2FFFE, NOT_WIDE },
    {  0x3FFFE, NOT_WIDE },

    /* Narrow */
    {   0x0020, NOT_WIDE },
    {   0x0041, NOT_WIDE },
    {   0x27E6, NOT_WIDE },

    /* Halfwidth */
    {   0x20A9, NOT_WIDE },
    {   0xFF61, NOT_WIDE },
    {   0xFF69, NOT_WIDE },
    {   0xFFEE, NOT_WIDE },

    /* Ambiguous */
    {   0x00A1, WIDE_CJK },
    {   0x00BE, WIDE_CJK },
    {   0x02DD, WIDE_CJK },
    {   0x2020, WIDE_CJK },
    {   0xFFFD, WIDE_CJK },
    {   0x00A1, WIDE_CJK },
    {  0x1F100, WIDE_CJK },
    {  0xE0100, WIDE_CJK },
    { 0x100000, WIDE_CJK },
    { 0x10FFFD, WIDE_CJK },

    /* Fullwidth */
    {   0x3000, WIDE },
    {   0xFF60, WIDE },

    /* Wide */
    {   0x2329, WIDE },
    {   0x3001, WIDE },
    {   0xFE69, WIDE },
    {  0x30000, WIDE },
    {  0x3FFFD, WIDE },

    /* Default Wide blocks */
    {   0x4DBF, WIDE },
    {   0x9FFF, WIDE },
    {   0xFAFF, WIDE },
    {  0x2A6DF, WIDE },
    {  0x2B73F, WIDE },
    {  0x2B81F, WIDE },
    {  0x2FA1F, WIDE },

    /* Uniode-5.2 character additions */
    /* Wide */
    {   0x115F, WIDE },

    /* Uniode-6.0 character additions */
    /* Wide */
    {  0x2B740, WIDE },
    {  0x1B000, WIDE },

    { 0x111111, NOT_WIDE }
  };

  for (i = 0; i < G_N_ELEMENTS (examples); i++)
    {
      g_assert_cmpint (g_unichar_iswide (examples[i].c), ==,
                       (examples[i].wide == WIDE));
      g_assert_cmpint (g_unichar_iswide_cjk (examples[i].c), ==,
                       (examples[i].wide != NOT_WIDE));
    }
};

/* Test g_unichar_to_utf8(). */
static void
test_unichar_to_utf8 (void)
{
  const struct
    {
      gunichar unichar;
      int expected_length;
      const char *expected_output;  /* must be of length `expected_length` */
    }
  vectors[] =
    {
      { 0x21, 1, "!" },
      { 0x00A1, 2, "\xc2\xa1" },
      { 0x0800, 3, "\xe0\xa0\x80" },
      { 0x10000, 4, "\xf0\x90\x80\x80" },
      { 0x200000, 5, "\xf8\x88\x80\x80\x80" },
      { 0x4000000, 6, "\xfc\x84\x80\x80\x80\x80" },
    };

  for (size_t i = 0; i < G_N_ELEMENTS (vectors); i++)
    {
      int length;
      char output[6];

      length = g_unichar_to_utf8 (vectors[i].unichar, NULL);
      g_assert_cmpint (length, ==, vectors[i].expected_length);

      length = g_unichar_to_utf8 (vectors[i].unichar, output);
      g_assert_cmpint (length, ==, vectors[i].expected_length);
      g_assert_cmpmem (output, length, vectors[i].expected_output, vectors[i].expected_length);
    }
}

/* Test that g_unichar_compose() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_compose (void)
{
  const struct
    {
      gunichar a;
      gunichar b;
      gunichar expected_result;  /* 0 for failure */
    }
  vectors[] =
    {
      /* Not composable */
      { 0x0041, 0x0042, 0 },
      { 0x0041, 0x0000, 0 },
      { 0x0066, 0x0069, 0 },

      /* Tricky non-composable */
      { 0x0308, 0x0301, 0 }, /* !0x0344 */
      { 0x0F71, 0x0F72, 0 }, /* !0x0F73 */

      /* Singletons should not compose */
      { 0x212B, 0x0000, 0 },
      { 0x00C5, 0x0000, 0 },
      { 0x2126, 0x0000, 0 },
      { 0x03A9, 0x0000, 0 },

      /* Pairs */
      { 0x0041, 0x030A, 0x00C5 },
      { 0x006F, 0x0302, 0x00F4 },
      { 0x1E63, 0x0307, 0x1E69 },
      { 0x0073, 0x0323, 0x1E63 },
      { 0x0064, 0x0307, 0x1E0B },
      { 0x0064, 0x0323, 0x1E0D },

       /* Hangul */
      { 0xD4CC, 0x11B6, 0xD4DB },
      { 0x1111, 0x1171, 0xD4CC },
      { 0xCE20, 0x11B8, 0xCE31 },
      { 0x110E, 0x1173, 0xCE20 },

      /* Hangul non-compositions (testing various exit conditions in combine_hangul()) */
      { 0x1100, 0x1160, 0 },
      { 0x1100, 0x1177, 0 },
      { 0xABFF, 0x11B6, 0 },
      { 0xD7A5, 0x11B6, 0 },
      { 0xAC01, 0x11B6, 0 },
      { 0xD4CC, 0x11A6, 0 },
      { 0xD4CC, 0x11C4, 0 },

      /* Primary composite above U+FFFF (a significant boundary value in our implementation) */
      { 0x1611E, 0x1611E, 0x16121 },  /* first and second char equal */
      { 0x1611E, 0x1611F, 0x16123 },

      /* First singletons */
      { 0x00F6, 0x0304, 0x022B },

      /* Second singletons */
      { 0x0B47, 0x0B57, 0x0B4C },
      { 0x00A0, 0x0B57, 0 },

      /* Very high values (exercising some branches in COMPOSE_INDEX) */
      { 0x16E00, 0x030A, 0 },
      { 0x212B, 0x16E00, 0 },

      /* Exercise some failure paths in the lookup tables */
      { 0x1E63, 0x0306, 0 },
      { 0x1E63, 0x0304, 0 },
      { 0x1E63, 0x0B57, 0 },
      { 0x1E63, 0x0000, 0 },
      { 0x1E63, 0x113C2, 0 },
      { 0x1F01, 0x113C2, 0 },
      { 0x006E, 0x0302, 0 },
      { 0x1E63, 0x1611F, 0 },
      { 0x1138E, 0x113B8, 0 },
      { 0x1611E, 0x0000, 0 },
      { 0x0000, 0x1611F, 0 },
      { 0x11390, 0x113C2, 0 },
    };

  for (size_t i = 0; i < G_N_ELEMENTS (vectors); i++)
    {
      gunichar ch;
      gboolean result;

      g_test_message ("Composing U+%06x and U+%06x; expecting U+%06x",
                      vectors[i].a, vectors[i].b, vectors[i].expected_result);

      result = g_unichar_compose (vectors[i].a, vectors[i].b, &ch);
      if (vectors[i].expected_result != 0)
        {
          g_assert_cmpuint (ch, ==, vectors[i].expected_result);
          g_assert_true (result);
        }
      else
        {
          g_assert_cmpuint (ch, ==, 0);
          g_assert_false (result);
        }
    }
}

/* Test that g_unichar_decompose() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_decompose (void)
{
  gunichar a, b;

  /* Not decomposable */
  g_assert_false (g_unichar_decompose (0x0041, &a, &b) && a == 0x0041 && b == 0);
  g_assert_false (g_unichar_decompose (0xFB01, &a, &b) && a == 0xFB01 && b == 0);

  /* Singletons */
  g_assert_true (g_unichar_decompose (0x212B, &a, &b) && a == 0x00C5 && b == 0);
  g_assert_true (g_unichar_decompose (0x2126, &a, &b) && a == 0x03A9 && b == 0);

  /* Tricky pairs */
  g_assert_true (g_unichar_decompose (0x0344, &a, &b) && a == 0x0308 && b == 0x0301);
  g_assert_true (g_unichar_decompose (0x0F73, &a, &b) && a == 0x0F71 && b == 0x0F72);

  /* Pairs */
  g_assert_true (g_unichar_decompose (0x00C5, &a, &b) && a == 0x0041 && b == 0x030A);
  g_assert_true (g_unichar_decompose (0x00F4, &a, &b) && a == 0x006F && b == 0x0302);
  g_assert_true (g_unichar_decompose (0x1E69, &a, &b) && a == 0x1E63 && b == 0x0307);
  g_assert_true (g_unichar_decompose (0x1E63, &a, &b) && a == 0x0073 && b == 0x0323);
  g_assert_true (g_unichar_decompose (0x1E0B, &a, &b) && a == 0x0064 && b == 0x0307);
  g_assert_true (g_unichar_decompose (0x1E0D, &a, &b) && a == 0x0064 && b == 0x0323);

  /* Hangul */
  g_assert_true (g_unichar_decompose (0xD4DB, &a, &b) && a == 0xD4CC && b == 0x11B6);
  g_assert_true (g_unichar_decompose (0xD4CC, &a, &b) && a == 0x1111 && b == 0x1171);
  g_assert_true (g_unichar_decompose (0xCE31, &a, &b) && a == 0xCE20 && b == 0x11B8);
  g_assert_true (g_unichar_decompose (0xCE20, &a, &b) && a == 0x110E && b == 0x1173);

  /* Primary composite above U+FFFF (a significant boundary value in our implementation) */
  g_assert_true (g_unichar_decompose (0x16121, &a, &b) && a == 0x1611E && b == 0x1611E);  /* first and second char equal */
  g_assert_true (g_unichar_decompose (0x16123, &a, &b) && a == 0x1611E && b == 0x1611F);
}

/* Test that g_unichar_fully_decompose() returns the correct value for
 * various ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_fully_decompose_canonical (void)
{
  const struct
    {
      gunichar input;
      size_t expected_len;
      gunichar expected_decomposition[4];
    }
  vectors[] =
    {
#define TEST0(ch)		{ ch, 1, { ch, 0, 0, 0 }}
#define TEST1(ch, a)		{ ch, 1, { a, 0, 0, 0 }}
#define TEST2(ch, a, b)		{ ch, 2, { a, b, 0, 0 }}
#define TEST3(ch, a, b, c)	{ ch, 3, { a, b, c, 0 }}
#define TEST4(ch, a, b, c, d)	{ ch, 4, { a, b, c, d }}

      /* Not decomposable */
      TEST0 (0x0041),
      TEST0 (0xFB01),

      /* Singletons */
      TEST2 (0x212B, 0x0041, 0x030A),
      TEST1 (0x2126, 0x03A9),

      /* Tricky pairs */
      TEST2 (0x0344, 0x0308, 0x0301),
      TEST2 (0x0F73, 0x0F71, 0x0F72),

      /* General */
      TEST2 (0x00C5, 0x0041, 0x030A),
      TEST2 (0x00F4, 0x006F, 0x0302),
      TEST3 (0x1E69, 0x0073, 0x0323, 0x0307),
      TEST2 (0x1E63, 0x0073, 0x0323),
      TEST2 (0x1E0B, 0x0064, 0x0307),
      TEST2 (0x1E0D, 0x0064, 0x0323),

      /* Hangul */
      TEST3 (0xD4DB, 0x1111, 0x1171, 0x11B6),
      TEST2 (0xD4CC, 0x1111, 0x1171),
      TEST3 (0xCE31, 0x110E, 0x1173, 0x11B8),
      TEST2 (0xCE20, 0x110E, 0x1173),

#undef TEST4
#undef TEST3
#undef TEST2
#undef TEST1
#undef TEST0
    };

  for (size_t i = 0; i < G_N_ELEMENTS (vectors); i++)
    {
      gunichar decomp[5];
      size_t len;

      g_test_message ("Fully decomposing U+%06x; expecting %" G_GSIZE_FORMAT " codepoints",
                      vectors[i].input, vectors[i].expected_len);

      /* Test with all possible output array sizes, to check that the function
       * can write partial results OK. */
      for (size_t j = 0; j <= G_N_ELEMENTS (decomp); j++)
        {
          len = g_unichar_fully_decompose (vectors[i].input, FALSE, decomp, G_N_ELEMENTS (decomp) - j);
          g_assert_cmpuint (len, ==, vectors[i].expected_len);
          if (len >= j)
            g_assert_cmpmem (decomp, (len - j) * sizeof (*decomp),
                             vectors[i].expected_decomposition, (vectors[i].expected_len - j) * sizeof (*vectors[i].expected_decomposition));
        }

      /* And again with no result array at all, just to get the length. */
      len = g_unichar_fully_decompose (vectors[i].input, FALSE, NULL, 0);
      g_assert_cmpuint (len, ==, vectors[i].expected_len);
    }
}

/* Test that g_unicode_canonical_decomposition() returns the correct
 * value for various ASCII and Unicode alphabetic, numeric, and other,
 * codepoints. */
static void
test_canonical_decomposition (void)
{
  gunichar *decomp;
  gsize len;

#define TEST_DECOMP(ch, expected_len, a, b, c, d) \
  decomp = g_unicode_canonical_decomposition (ch, &len); \
  g_assert_cmpint (expected_len, ==, len); \
  if (expected_len >= 1) g_assert_cmphex (decomp[0], ==, a); \
  if (expected_len >= 2) g_assert_cmphex (decomp[1], ==, b); \
  if (expected_len >= 3) g_assert_cmphex (decomp[2], ==, c); \
  if (expected_len >= 4) g_assert_cmphex (decomp[3], ==, d); \
  g_free (decomp);

#define TEST0(ch)		TEST_DECOMP (ch, 1, ch, 0, 0, 0)
#define TEST1(ch, a)		TEST_DECOMP (ch, 1, a, 0, 0, 0)
#define TEST2(ch, a, b)		TEST_DECOMP (ch, 2, a, b, 0, 0)
#define TEST3(ch, a, b, c)	TEST_DECOMP (ch, 3, a, b, c, 0)
#define TEST4(ch, a, b, c, d)	TEST_DECOMP (ch, 4, a, b, c, d)

  /* Not decomposable */
  TEST0 (0x0041);
  TEST0 (0xFB01);

  /* Singletons */
  TEST2 (0x212B, 0x0041, 0x030A);
  TEST1 (0x2126, 0x03A9);

  /* Tricky pairs */
  TEST2 (0x0344, 0x0308, 0x0301);
  TEST2 (0x0F73, 0x0F71, 0x0F72);

  /* General */
  TEST2 (0x00C5, 0x0041, 0x030A);
  TEST2 (0x00F4, 0x006F, 0x0302);
  TEST3 (0x1E69, 0x0073, 0x0323, 0x0307);
  TEST2 (0x1E63, 0x0073, 0x0323);
  TEST2 (0x1E0B, 0x0064, 0x0307);
  TEST2 (0x1E0D, 0x0064, 0x0323);

  /* Hangul */
  TEST3 (0xD4DB, 0x1111, 0x1171, 0x11B6);
  TEST2 (0xD4CC, 0x1111, 0x1171);
  TEST3 (0xCE31, 0x110E, 0x1173, 0x11B8);
  TEST2 (0xCE20, 0x110E, 0x1173);

#undef TEST_DECOMP
}

/* Test that g_unichar_decompose() whenever encountering a char ch
 * decomposes into a and b, b itself won't decompose any further. */
static void
test_decompose_tail (void)
{
  gunichar ch, a, b, c, d;

  /* Test that whenever a char ch decomposes into a and b, b itself
   * won't decompose any further. */

  for (ch = 0; ch < 0x110000; ch++)
    if (g_unichar_decompose (ch, &a, &b))
      g_assert_false (g_unichar_decompose (b, &c, &d));
    else
      {
        g_assert_cmpuint (a, ==, ch);
        g_assert_cmpuint (b, ==, 0);
      }
}

/* Test that all canonical decompositions of g_unichar_fully_decompose()
 * are at most 4 in length, and compatibility decompositions are
 * at most 18 in length. */
static void
test_fully_decompose_len (void)
{
  gunichar ch;

  /* Test that all canonical decompositions are at most 4 in length,
   * and compatibility decompositions are at most 18 in length.
   */

  for (ch = 0; ch < 0x110000; ch++) {
    g_assert_cmpint (g_unichar_fully_decompose (ch, FALSE, NULL, 0), <=, 4);
    g_assert_cmpint (g_unichar_fully_decompose (ch, TRUE,  NULL, 0), <=, 18);
  }
}

/* Check various examples from Unicode Annex #15 for NFD and NFC
 * normalization.
 */
static void
test_normalization (void)
{
  const struct {
    const char *source;
    const char *nfd;
    const char *nfc;
  } tests[] = {
    // Singletons
    { "\xe2\x84\xab", "A\xcc\x8a", "Å" }, // U+212B ANGSTROM SIGN
    { "\xe2\x84\xa6", "Ω", "Ω" }, // U+2126 OHM SIGN
    // Canonical Composites
    { "Å", "A\xcc\x8a", "Å" }, // U+00C5 LATIN CAPITAL LETTER A WITH RING ABOVE
    { "ô", "o\xcc\x82", "ô" }, // U+00F4 LATIN SMALL LETTER O WITH CIRCUMFLEX
    // Multiple Combining Marks
    { "\xe1\xb9\xa9", "s\xcc\xa3\xcc\x87", "ṩ" }, // U+1E69 LATIN SMALL LETTER S WITH DOT BELOW AND DOT ABOVE
    { "\xe1\xb8\x8b\xcc\xa3", "d\xcc\xa3\xcc\x87", "ḍ̇" },
    { "q\xcc\x87\xcc\xa3", "q\xcc\xa3\xcc\x87", "q̣̇" },
    // Compatibility Composites
    { "ﬁ", "ﬁ", "ﬁ" }, // U+FB01 LATIN SMALL LIGATURE FI
    { "2\xe2\x81\xb5", "2\xe2\x81\xb5", "2⁵" },
    { "\xe1\xba\x9b\xcc\xa3", "\xc5\xbf\xcc\xa3\xcc\x87", "ẛ̣" },

    // Tests for behavior with reordered marks
    { "s\xcc\x87\xcc\xa3", "s\xcc\xa3\xcc\x87", "ṩ" },
    { "α\xcc\x94\xcd\x82", "α\xcc\x94\xcd\x82", "ἇ" },
    { "α\xcd\x82\xcc\x94", "α\xcd\x82\xcc\x94", "ᾶ\xcc\x94" },
  };
  gsize i;

  for (i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      char *nfd, *nfc;

      nfd = g_utf8_normalize (tests[i].source, -1, G_NORMALIZE_NFD);
      g_assert_cmpstr (nfd, ==, tests[i].nfd);

      nfc = g_utf8_normalize (tests[i].nfd, -1, G_NORMALIZE_NFC);
      g_assert_cmpstr (nfc, ==, tests[i].nfc);

      g_free (nfd);
      g_free (nfc);
    }
}

static void
test_iso15924 (void)
{
  const struct {
    GUnicodeScript script;
    char four_letter_code[5];
  } data[] = {
    { G_UNICODE_SCRIPT_COMMON,             "Zyyy" },
    { G_UNICODE_SCRIPT_INHERITED,          "Zinh" },
    { G_UNICODE_SCRIPT_MATH,               "Zmth" },
    { G_UNICODE_SCRIPT_ARABIC,             "Arab" },
    { G_UNICODE_SCRIPT_ARMENIAN,           "Armn" },
    { G_UNICODE_SCRIPT_BENGALI,            "Beng" },
    { G_UNICODE_SCRIPT_BOPOMOFO,           "Bopo" },
    { G_UNICODE_SCRIPT_CHEROKEE,           "Cher" },
    { G_UNICODE_SCRIPT_COPTIC,             "Copt" },
    { G_UNICODE_SCRIPT_CYRILLIC,           "Cyrl" },
    { G_UNICODE_SCRIPT_DESERET,            "Dsrt" },
    { G_UNICODE_SCRIPT_DEVANAGARI,         "Deva" },
    { G_UNICODE_SCRIPT_ETHIOPIC,           "Ethi" },
    { G_UNICODE_SCRIPT_GEORGIAN,           "Geor" },
    { G_UNICODE_SCRIPT_GOTHIC,             "Goth" },
    { G_UNICODE_SCRIPT_GREEK,              "Grek" },
    { G_UNICODE_SCRIPT_GUJARATI,           "Gujr" },
    { G_UNICODE_SCRIPT_GURMUKHI,           "Guru" },
    { G_UNICODE_SCRIPT_HAN,                "Hani" },
    { G_UNICODE_SCRIPT_HANGUL,             "Hang" },
    { G_UNICODE_SCRIPT_HEBREW,             "Hebr" },
    { G_UNICODE_SCRIPT_HIRAGANA,           "Hira" },
    { G_UNICODE_SCRIPT_KANNADA,            "Knda" },
    { G_UNICODE_SCRIPT_KATAKANA,           "Kana" },
    { G_UNICODE_SCRIPT_KHMER,              "Khmr" },
    { G_UNICODE_SCRIPT_LAO,                "Laoo" },
    { G_UNICODE_SCRIPT_LATIN,              "Latn" },
    { G_UNICODE_SCRIPT_MALAYALAM,          "Mlym" },
    { G_UNICODE_SCRIPT_MONGOLIAN,          "Mong" },
    { G_UNICODE_SCRIPT_MYANMAR,            "Mymr" },
    { G_UNICODE_SCRIPT_OGHAM,              "Ogam" },
    { G_UNICODE_SCRIPT_OLD_ITALIC,         "Ital" },
    { G_UNICODE_SCRIPT_ORIYA,              "Orya" },
    { G_UNICODE_SCRIPT_RUNIC,              "Runr" },
    { G_UNICODE_SCRIPT_SINHALA,            "Sinh" },
    { G_UNICODE_SCRIPT_SYRIAC,             "Syrc" },
    { G_UNICODE_SCRIPT_TAMIL,              "Taml" },
    { G_UNICODE_SCRIPT_TELUGU,             "Telu" },
    { G_UNICODE_SCRIPT_THAANA,             "Thaa" },
    { G_UNICODE_SCRIPT_THAI,               "Thai" },
    { G_UNICODE_SCRIPT_TIBETAN,            "Tibt" },
    { G_UNICODE_SCRIPT_CANADIAN_ABORIGINAL, "Cans" },
    { G_UNICODE_SCRIPT_YI,                 "Yiii" },
    { G_UNICODE_SCRIPT_TAGALOG,            "Tglg" },
    { G_UNICODE_SCRIPT_HANUNOO,            "Hano" },
    { G_UNICODE_SCRIPT_BUHID,              "Buhd" },
    { G_UNICODE_SCRIPT_TAGBANWA,           "Tagb" },

    /* Unicode-4.0 additions */
    { G_UNICODE_SCRIPT_BRAILLE,            "Brai" },
    { G_UNICODE_SCRIPT_CYPRIOT,            "Cprt" },
    { G_UNICODE_SCRIPT_LIMBU,              "Limb" },
    { G_UNICODE_SCRIPT_OSMANYA,            "Osma" },
    { G_UNICODE_SCRIPT_SHAVIAN,            "Shaw" },
    { G_UNICODE_SCRIPT_LINEAR_B,           "Linb" },
    { G_UNICODE_SCRIPT_TAI_LE,             "Tale" },
    { G_UNICODE_SCRIPT_UGARITIC,           "Ugar" },

    /* Unicode-4.1 additions */
    { G_UNICODE_SCRIPT_NEW_TAI_LUE,        "Talu" },
    { G_UNICODE_SCRIPT_BUGINESE,           "Bugi" },
    { G_UNICODE_SCRIPT_GLAGOLITIC,         "Glag" },
    { G_UNICODE_SCRIPT_TIFINAGH,           "Tfng" },
    { G_UNICODE_SCRIPT_SYLOTI_NAGRI,       "Sylo" },
    { G_UNICODE_SCRIPT_OLD_PERSIAN,        "Xpeo" },
    { G_UNICODE_SCRIPT_KHAROSHTHI,         "Khar" },

    /* Unicode-5.0 additions */
    { G_UNICODE_SCRIPT_UNKNOWN,            "Zzzz" },
    { G_UNICODE_SCRIPT_BALINESE,           "Bali" },
    { G_UNICODE_SCRIPT_CUNEIFORM,          "Xsux" },
    { G_UNICODE_SCRIPT_PHOENICIAN,         "Phnx" },
    { G_UNICODE_SCRIPT_PHAGS_PA,           "Phag" },
    { G_UNICODE_SCRIPT_NKO,                "Nkoo" },

    /* Unicode-5.1 additions */
    { G_UNICODE_SCRIPT_KAYAH_LI,           "Kali" },
    { G_UNICODE_SCRIPT_LEPCHA,             "Lepc" },
    { G_UNICODE_SCRIPT_REJANG,             "Rjng" },
    { G_UNICODE_SCRIPT_SUNDANESE,          "Sund" },
    { G_UNICODE_SCRIPT_SAURASHTRA,         "Saur" },
    { G_UNICODE_SCRIPT_CHAM,               "Cham" },
    { G_UNICODE_SCRIPT_OL_CHIKI,           "Olck" },
    { G_UNICODE_SCRIPT_VAI,                "Vaii" },
    { G_UNICODE_SCRIPT_CARIAN,             "Cari" },
    { G_UNICODE_SCRIPT_LYCIAN,             "Lyci" },
    { G_UNICODE_SCRIPT_LYDIAN,             "Lydi" },

    /* Unicode-5.2 additions */
    { G_UNICODE_SCRIPT_AVESTAN,                "Avst" },
    { G_UNICODE_SCRIPT_BAMUM,                  "Bamu" },
    { G_UNICODE_SCRIPT_EGYPTIAN_HIEROGLYPHS,   "Egyp" },
    { G_UNICODE_SCRIPT_IMPERIAL_ARAMAIC,       "Armi" },
    { G_UNICODE_SCRIPT_INSCRIPTIONAL_PAHLAVI,  "Phli" },
    { G_UNICODE_SCRIPT_INSCRIPTIONAL_PARTHIAN, "Prti" },
    { G_UNICODE_SCRIPT_JAVANESE,               "Java" },
    { G_UNICODE_SCRIPT_KAITHI,                 "Kthi" },
    { G_UNICODE_SCRIPT_LISU,                   "Lisu" },
    { G_UNICODE_SCRIPT_MEETEI_MAYEK,           "Mtei" },
    { G_UNICODE_SCRIPT_OLD_SOUTH_ARABIAN,      "Sarb" },
    { G_UNICODE_SCRIPT_OLD_TURKIC,             "Orkh" },
    { G_UNICODE_SCRIPT_SAMARITAN,              "Samr" },
    { G_UNICODE_SCRIPT_TAI_THAM,               "Lana" },
    { G_UNICODE_SCRIPT_TAI_VIET,               "Tavt" },

    /* Unicode-6.0 additions */
    { G_UNICODE_SCRIPT_BATAK,                  "Batk" },
    { G_UNICODE_SCRIPT_BRAHMI,                 "Brah" },
    { G_UNICODE_SCRIPT_MANDAIC,                "Mand" },

    /* Unicode-6.1 additions */
    { G_UNICODE_SCRIPT_CHAKMA,                 "Cakm" },
    { G_UNICODE_SCRIPT_MEROITIC_CURSIVE,       "Merc" },
    { G_UNICODE_SCRIPT_MEROITIC_HIEROGLYPHS,   "Mero" },
    { G_UNICODE_SCRIPT_MIAO,                   "Plrd" },
    { G_UNICODE_SCRIPT_SHARADA,                "Shrd" },
    { G_UNICODE_SCRIPT_SORA_SOMPENG,           "Sora" },
    { G_UNICODE_SCRIPT_TAKRI,                  "Takr" },

    /* Unicode 7.0 additions */
    { G_UNICODE_SCRIPT_BASSA_VAH,              "Bass" },
    { G_UNICODE_SCRIPT_CAUCASIAN_ALBANIAN,     "Aghb" },
    { G_UNICODE_SCRIPT_DUPLOYAN,               "Dupl" },
    { G_UNICODE_SCRIPT_ELBASAN,                "Elba" },
    { G_UNICODE_SCRIPT_GRANTHA,                "Gran" },
    { G_UNICODE_SCRIPT_KHOJKI,                 "Khoj" },
    { G_UNICODE_SCRIPT_KHUDAWADI,              "Sind" },
    { G_UNICODE_SCRIPT_LINEAR_A,               "Lina" },
    { G_UNICODE_SCRIPT_MAHAJANI,               "Mahj" },
    { G_UNICODE_SCRIPT_MANICHAEAN,             "Mani" },
    { G_UNICODE_SCRIPT_MENDE_KIKAKUI,          "Mend" },
    { G_UNICODE_SCRIPT_MODI,                   "Modi" },
    { G_UNICODE_SCRIPT_MRO,                    "Mroo" },
    { G_UNICODE_SCRIPT_NABATAEAN,              "Nbat" },
    { G_UNICODE_SCRIPT_OLD_NORTH_ARABIAN,      "Narb" },
    { G_UNICODE_SCRIPT_OLD_PERMIC,             "Perm" },
    { G_UNICODE_SCRIPT_PAHAWH_HMONG,           "Hmng" },
    { G_UNICODE_SCRIPT_PALMYRENE,              "Palm" },
    { G_UNICODE_SCRIPT_PAU_CIN_HAU,            "Pauc" },
    { G_UNICODE_SCRIPT_PSALTER_PAHLAVI,        "Phlp" },
    { G_UNICODE_SCRIPT_SIDDHAM,                "Sidd" },
    { G_UNICODE_SCRIPT_TIRHUTA,                "Tirh" },
    { G_UNICODE_SCRIPT_WARANG_CITI,            "Wara" },

    /* Unicode 8.0 additions */
    { G_UNICODE_SCRIPT_AHOM,                   "Ahom" },
    { G_UNICODE_SCRIPT_ANATOLIAN_HIEROGLYPHS,  "Hluw" },
    { G_UNICODE_SCRIPT_HATRAN,                 "Hatr" },
    { G_UNICODE_SCRIPT_MULTANI,                "Mult" },
    { G_UNICODE_SCRIPT_OLD_HUNGARIAN,          "Hung" },
    { G_UNICODE_SCRIPT_SIGNWRITING,            "Sgnw" },

    /* Unicode 9.0 additions */
    { G_UNICODE_SCRIPT_ADLAM,                  "Adlm" },
    { G_UNICODE_SCRIPT_BHAIKSUKI,              "Bhks" },
    { G_UNICODE_SCRIPT_MARCHEN,                "Marc" },
    { G_UNICODE_SCRIPT_NEWA,                   "Newa" },
    { G_UNICODE_SCRIPT_OSAGE,                  "Osge" },
    { G_UNICODE_SCRIPT_TANGUT,                 "Tang" },

    /* Unicode 10.0 additions */
    { G_UNICODE_SCRIPT_MASARAM_GONDI,          "Gonm" },
    { G_UNICODE_SCRIPT_NUSHU,                  "Nshu" },
    { G_UNICODE_SCRIPT_SOYOMBO,                "Soyo" },
    { G_UNICODE_SCRIPT_ZANABAZAR_SQUARE,       "Zanb" },

    /* Unicode 11.0 additions */
    { G_UNICODE_SCRIPT_DOGRA,                  "Dogr" },
    { G_UNICODE_SCRIPT_GUNJALA_GONDI,          "Gong" },
    { G_UNICODE_SCRIPT_HANIFI_ROHINGYA,        "Rohg" },
    { G_UNICODE_SCRIPT_MAKASAR,                "Maka" },
    { G_UNICODE_SCRIPT_MEDEFAIDRIN,            "Medf" },
    { G_UNICODE_SCRIPT_OLD_SOGDIAN,            "Sogo" },
    { G_UNICODE_SCRIPT_SOGDIAN,                "Sogd" },

    /* Unicode 12.0 additions */
    { G_UNICODE_SCRIPT_ELYMAIC,                "Elym" },
    { G_UNICODE_SCRIPT_NANDINAGARI,            "Nand" },
    { G_UNICODE_SCRIPT_NYIAKENG_PUACHUE_HMONG, "Hmnp" },
    { G_UNICODE_SCRIPT_WANCHO,                 "Wcho" },

    /* Unicode 13.0 additions */
    { G_UNICODE_SCRIPT_CHORASMIAN,             "Chrs" },
    { G_UNICODE_SCRIPT_DIVES_AKURU,            "Diak" },
    { G_UNICODE_SCRIPT_KHITAN_SMALL_SCRIPT,    "Kits" },
    { G_UNICODE_SCRIPT_YEZIDI,                 "Yezi" },

    /* Unicode 14.0 additions */
    { G_UNICODE_SCRIPT_CYPRO_MINOAN,           "Cpmn" },
    { G_UNICODE_SCRIPT_OLD_UYGHUR,             "Ougr" },
    { G_UNICODE_SCRIPT_TANGSA,                 "Tnsa" },
    { G_UNICODE_SCRIPT_TOTO,                   "Toto" },
    { G_UNICODE_SCRIPT_VITHKUQI,               "Vith" },

    /* Unicode 15.0 additions */
    { G_UNICODE_SCRIPT_KAWI,                   "Kawi" },
    { G_UNICODE_SCRIPT_NAG_MUNDARI,            "Nagm" },

    /* Unicode 16.0 additions */
    { G_UNICODE_SCRIPT_TODHRI,                 "Todr" },
    { G_UNICODE_SCRIPT_GARAY,                  "Gara" },
    { G_UNICODE_SCRIPT_TULU_TIGALARI,          "Tutg" },
    { G_UNICODE_SCRIPT_SUNUWAR,                "Sunu" },
    { G_UNICODE_SCRIPT_GURUNG_KHEMA,           "Gukh" },
    { G_UNICODE_SCRIPT_KIRAT_RAI,              "Krai" },
    { G_UNICODE_SCRIPT_OL_ONAL,                "Onao" },
  };
  guint i;

  g_assert_cmphex (0, ==,
                   g_unicode_script_to_iso15924 (G_UNICODE_SCRIPT_INVALID_CODE));
  g_assert_cmphex (0x5A7A7A7A, ==, g_unicode_script_to_iso15924 (1000));
  g_assert_cmphex (0x41726162, ==,
                   g_unicode_script_to_iso15924 (G_UNICODE_SCRIPT_ARABIC));

  g_assert_cmphex (G_UNICODE_SCRIPT_INVALID_CODE, ==,
                   g_unicode_script_from_iso15924 (0));
  g_assert_cmphex (G_UNICODE_SCRIPT_UNKNOWN, ==,
                   g_unicode_script_from_iso15924 (0x12345678));

#define PACK(a,b,c,d) \
  ((guint32)((((guint8)(a))<<24)|(((guint8)(b))<<16)|(((guint8)(c))<<8)|((guint8)(d))))

  for (i = 0; i < G_N_ELEMENTS (data); i++)
    {
      guint32 code = PACK (data[i].four_letter_code[0],
                           data[i].four_letter_code[1],
                           data[i].four_letter_code[2],
                           data[i].four_letter_code[3]);

      g_test_message ("Testing script %s (code %u)", data[i].four_letter_code, code);
      g_assert_cmphex (g_unicode_script_to_iso15924 (data[i].script), ==, code);
      g_assert_cmpint (g_unicode_script_from_iso15924 (code), ==, data[i].script);
    }

#undef PACK
}

static void
test_normalize (void)
{
  guint i;
  typedef struct
  {
    const gchar *str;
    const gchar *nfd;
    const gchar *nfc;
    const gchar *nfkd;
    const gchar *nfkc;
  } Test;
  Test tests[] = {
    { "Äffin", "A\u0308ffin", "Äffin", "A\u0308ffin", "Äffin" },
    { "Ä\uFB03n", "A\u0308\uFB03n", "Ä\uFB03n", "A\u0308ffin", "Äffin" },
    { "Henry IV", "Henry IV", "Henry IV", "Henry IV", "Henry IV" },
    { "Henry \u2163", "Henry \u2163", "Henry \u2163", "Henry IV", "Henry IV" },
    { "non-utf\x88", NULL, NULL, NULL, NULL },
    { "", "", "", "", "" },
  };

#define TEST(str, mode, expected)                         \
  {                                                       \
    gchar *normalized = g_utf8_normalize (str, -1, mode); \
    g_assert_cmpstr (normalized, ==, expected);           \
    g_free (normalized);                                  \
  }

  for (i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      TEST (tests[i].str, G_NORMALIZE_NFD, tests[i].nfd);
      TEST (tests[i].str, G_NORMALIZE_NFC, tests[i].nfc);
      TEST (tests[i].str, G_NORMALIZE_NFKD, tests[i].nfkd);
      TEST (tests[i].str, G_NORMALIZE_NFKC, tests[i].nfkc);
    }

#undef TEST
}

static void
test_unknown_scripts (void)
{
  gunichar ch;
  GUnicodeScript max_script;

  max_script = G_UNICODE_SCRIPT_INVALID_CODE;
  for (ch = 0; ch <= 0x10FFFF; ch++)
    max_script = MAX (max_script, g_unichar_get_script (ch));

#define PACK(a, b, c, d) \
  ((guint32) ((((guint8) (a)) << 24) | (((guint8) (b)) << 16) | (((guint8) (c)) << 8) | ((guint8) (d))))

  for (GUnicodeScript i = 0; i <= max_script; i++)
    {
      g_test_message ("Testing script %d", i);

      guint32 tag = g_unicode_script_to_iso15924 (i);
      if (i == G_UNICODE_SCRIPT_UNKNOWN)
        g_assert_cmphex (tag, ==, PACK ('Z', 'z', 'z', 'z'));
      else
        g_assert_cmphex (tag, !=, PACK ('Z', 'z', 'z', 'z'));

      GUnicodeScript script = g_unicode_script_from_iso15924 (tag);
      g_assert_cmpint (script, ==, i);
    }

#undef PACK
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/unicode/alnum", test_alnum);
  g_test_add_func ("/unicode/alpha", test_alpha);
  g_test_add_func ("/unicode/break-type", test_unichar_break_type);
  g_test_add_func ("/unicode/canonical-decomposition", test_canonical_decomposition);
  g_test_add_func ("/unicode/casefold", test_casefold);
  g_test_add_func ("/unicode/casemap_and_casefold", test_casemap_and_casefold);
  g_test_add_func ("/unicode/cases", test_cases);
  g_test_add_func ("/unicode/character-type", test_unichar_character_type);
  g_test_add_func ("/unicode/cntrl", test_cntrl);
  g_test_add_func ("/unicode/combining-class", test_combining_class);
  g_test_add_func ("/unicode/compose", test_compose);
  g_test_add_func ("/unicode/decompose", test_decompose);
  g_test_add_func ("/unicode/decompose-tail", test_decompose_tail);
  g_test_add_func ("/unicode/defined", test_defined);
  g_test_add_func ("/unicode/digit", test_digit);
  g_test_add_func ("/unicode/digit-value", test_digit_value);
  g_test_add_func ("/unicode/fully-decompose-canonical", test_fully_decompose_canonical);
  g_test_add_func ("/unicode/fully-decompose-len", test_fully_decompose_len);
  g_test_add_func ("/unicode/normalization", test_normalization);
  g_test_add_func ("/unicode/graph", test_graph);
  g_test_add_func ("/unicode/iso15924", test_iso15924);
  g_test_add_func ("/unicode/lower", test_lower);
  g_test_add_func ("/unicode/mark", test_mark);
  g_test_add_func ("/unicode/mirror", test_mirror);
  g_test_add_func ("/unicode/print", test_print);
  g_test_add_func ("/unicode/punctuation", test_punctuation);
  g_test_add_func ("/unicode/script", test_unichar_script);
  g_test_add_func ("/unicode/space", test_space);
  g_test_add_func ("/unicode/strdown", test_strdown);
  g_test_add_func ("/unicode/strup", test_strup);
  g_test_add_func ("/unicode/turkish-strupdown", test_turkish_strupdown);
  g_test_add_func ("/unicode/title", test_title);
  g_test_add_func ("/unicode/upper", test_upper);
  g_test_add_func ("/unicode/validate", test_unichar_validate);
  g_test_add_func ("/unicode/wide", test_wide);
  g_test_add_func ("/unicode/unichar-to-utf8", test_unichar_to_utf8);
  g_test_add_func ("/unicode/xdigit", test_xdigit);
  g_test_add_func ("/unicode/xdigit-value", test_xdigit_value);
  g_test_add_func ("/unicode/zero-width", test_zerowidth);
  g_test_add_func ("/unicode/normalize", test_normalize);
  g_test_add_func ("/unicode/unknown-scripts", test_unknown_scripts);

  return g_test_run();
}
