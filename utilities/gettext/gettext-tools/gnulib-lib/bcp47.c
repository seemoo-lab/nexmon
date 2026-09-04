/* Support for locale names in BCP 47 syntax.
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

/* Specification.  */
#include "bcp47.h"

#include <stdlib.h>
#include <string.h>

#include "c-ctype.h"

/* The set of XPG locale names is historically grown and emphasizes the region
   over the script.  In fact, it uses the script only to disambiguate locale
   with the same region.
   The BCP 47 locale names, on the other hand, emphasize the script over the
   region.

   Therefore we add special treatment of all languages that can be written
   using different scripts:
     - During XPG to BCP 47 conversion, we add the script if not present,
       inferring it from the region.
     - During BCP 47 to XPG conversion, when a region is provided, we remove
       the script if doing so produces a known locale name (i.e. a locale name
       present in glibc, since glibc has the most complete set of locales).

   This affects the following languages:
     - Azerbaijani (az): Latin in Azerbaijan, Arabic in Iran.
       <https://en.wikipedia.org/wiki/Azerbaijani_language>
     - Belarusian (be): Assume Cyrillic by default, but Latin exists as well.
       <https://en.wikipedia.org/wiki/Belarusian_language#Alphabet>
     - Tamazight / Berber (ber): Assume Latin by default, but Arabic exists
       as well.
       <https://en.wikipedia.org/wiki/Berber_languages>
       <https://en.wikipedia.org/wiki/Berber_Latin_alphabet>
       <https://en.wikipedia.org/wiki/Tifinagh>
     - Bosnian (bs): Assume Latin by default, but Cyrillic exists as well.
       <https://en.wikipedia.org/wiki/Bosnian_language>
     - Hausa (ha): Assume Latin by default, but Arabic exists as well.
       <https://en.wikipedia.org/wiki/Hausa_language>
       <https://en.wikipedia.org/wiki/Boko_alphabet>
     - Inuktitut (iu): Assume Inuktitut syllabics by default, but Latin
       exists as well.
       <https://en.wikipedia.org/wiki/Inuktitut#Writing>
       <https://en.wikipedia.org/wiki/Inuktitut_syllabics>
     - Kazakh (kk): Currently (2024) Cyrillic by default, but migrating to
       Latin.
       <https://en.wikipedia.org/wiki/Kazakh_language>
     - Kashmiri (ks): Assume Arabic by default, but Devanagari exists as well.
       <https://en.wikipedia.org/wiki/Kashmiri_language>
     - Kurdish (ku): Latin in Türkiye and Syria, Arabic in Iraq and Iran.
       <https://en.wikipedia.org/wiki/Kurdish_language>
     - Mongolian (mn): Currently (2024) mainly Cyrillic, but the vertically
       written Mongolian script is also in use.
       <https://en.wikipedia.org/wiki/Mongolian_language>
     - Min Nan Chinese (nan): Assume Traditional Chinese by default, but Latin
       exists as well.
       <https://en.wikipedia.org/wiki/Southern_Min>
     - Punjabi (pa): Arabic in Pakistan, Gurmukhi in India.
       <https://en.wikipedia.org/wiki/Punjabi_language>
     - Sindhi (sd): Arabic in Pakistan, assume Arabic in India as well, but
       Devanagari exists in India too.
       <https://en.wikipedia.org/wiki/Sindhi_language#Writing_systems>
     - Serbian (sr): Assume Cyrillic by default, but Latin exists as well.
       <https://en.wikipedia.org/wiki/Serbian_language>
     - Uzbek (uz): Assume Latin by default, but Cyrillic exists as well.
       <https://en.wikipedia.org/wiki/Uzbek_language>
     - Yiddish (yi): Assume Hebrew by default, but Latin exists as well.
       <https://en.wikipedia.org/wiki/Yiddish>
     - Chinese (zh): Simplified Chinese in PRC and Singapore,
       Traditional Chinese elsewhere.
       <https://en.wikipedia.org/wiki/Chinese_language>
 */


struct script
{
  char name[12];                        /* Script name, lowercased, NUL-terminated */
  char code[4] _GL_ATTRIBUTE_NONSTRING; /* Script code, not NUL-terminated */
};

/* Table of script names and four-letter script codes.
   The codes are taken from <https://en.wikipedia.org/wiki/ISO_15924> or
   <https://unicode.org/iso15924/iso15924-codes.html>.  */
static const struct script scripts[] =
{
#define SCRIPT_LATIN      0
  { "latin",      "Latn" },
#define SCRIPT_CYRILLIC   1
  { "cyrillic",   "Cyrl" },
#define SCRIPT_HEBREW     2
  { "hebrew",     "Hebr" },
#define SCRIPT_ARABIC     3
  { "arabic",     "Arab" },
#define SCRIPT_DEVANAGARI 4
  { "devanagari", "Deva" },
#define SCRIPT_GURMUKHI   5
  { "gurmukhi",   "Guru" },
#define SCRIPT_MONGOLIAN  6
  { "mongolian",  "Mong" }
};
#define NUM_SCRIPTS (sizeof (scripts) / sizeof (scripts[0]))


/* For a language that uses a different script depending on the territory,
   other than Chinese, this function returns the default script in the given
   territory, or NULL.  */
static const struct script *
default_script_in_territory (const char language[2], const char territory[2])
{
  if (memeq (language, "az", 2))
    {
      if (memeq (territory, "AZ", 2))
        return &scripts[SCRIPT_LATIN];
      else if (memeq (territory, "IR", 2))
        return &scripts[SCRIPT_ARABIC];
    }
  else if (memeq (language, "ku", 2))
    {
      if (memeq (territory, "IQ", 2)
          || memeq (territory, "IR", 2))
        return &scripts[SCRIPT_ARABIC];
      else if (memeq (territory, "SY", 2)
               || memeq (territory, "TR", 2))
        return &scripts[SCRIPT_LATIN];
    }
  else if (memeq (language, "pa", 2))
    {
      if (memeq (territory, "PK", 2))
        return &scripts[SCRIPT_ARABIC];
      else if (memeq (territory, "IN", 2))
        return &scripts[SCRIPT_GURMUKHI];
    }
  return NULL;
}

/* For a language that can be written using different scripts, independently of
   the territory, other than Inuktitut and Min Nan Chinese, these functions
   return the default (main) script, or NULL.  */
static const struct script *
default_script_for_language2 (const char language[2])
{
  if (memeq (language, "be", 2))
     return &scripts[SCRIPT_CYRILLIC];
   else if (memeq (language, "bs", 2))
     return &scripts[SCRIPT_LATIN];
   else if (memeq (language, "ha", 2))
     return &scripts[SCRIPT_LATIN];
   else if (memeq (language, "kk", 2))
     return &scripts[SCRIPT_CYRILLIC];
   else if (memeq (language, "ks", 2))
     return &scripts[SCRIPT_ARABIC];
   else if (memeq (language, "mn", 2))
     return &scripts[SCRIPT_CYRILLIC];
   else if (memeq (language, "sd", 2))
     return &scripts[SCRIPT_ARABIC];
   else if (memeq (language, "sr", 2))
     return &scripts[SCRIPT_CYRILLIC];
   else if (memeq (language, "uz", 2))
     return &scripts[SCRIPT_LATIN];
   else if (memeq (language, "yi", 2))
     return &scripts[SCRIPT_HEBREW];
   return NULL;
}
static const struct script *
default_script_for_language3 (const char language[3])
{
   if (memeq (language, "ber", 3))
     return &scripts[SCRIPT_LATIN];
   return NULL;
}



void
xpg_to_bcp47 (char *bcp47, const char *xpg)
{
  /* Special cases.  */
  if (streq (xpg, ""))
   fail:
    {
      strcpy (bcp47, "und");
      return;
    }
  if ((xpg[0] == 'C' && (xpg[1] == '\0' || xpg[1] == '.'))
      || streq (xpg, "POSIX"))
    {
      /* The "C" (or "C.UTF-8") and "POSIX" locales most closely resemble the
         "en_US" locale.  */
      strcpy (bcp47, "und");
      return;
    }

  /* Parse XPG as language[_territory][.codeset][@modifier].  */
  const char *language_start = NULL;
  size_t language_len = 0;
  const char *territory_start = NULL;
  size_t territory_len = 0;
  const char *modifier_start = NULL;
  size_t modifier_len = 0;

  {
    const char *p;

    p = xpg;
    language_start = p;
    while (*p != '\0' && *p != '_' && *p != '.' && *p != '@')
      p++;
    language_len = p - language_start;
    if (*p == '_')
      {
        p++;
        territory_start = p;
        while (*p != '\0' && *p != '.' && *p != '@')
          p++;
        territory_len = p - territory_start;
      }
    if (*p == '.')
      {
        p++;
        while (*p != '\0' && *p != '@')
          p++;
      }
    if (*p == '@')
      {
        p++;
        modifier_start = p;
        while (*p != '\0')
          p++;
        modifier_len = p - modifier_start;
      }
  }

  if (language_len == 0)
    /* No language -> fail.  */
    goto fail;

  /* Canonicalize the language.  */
  /* For Quechua, Microsoft uses the ISO 639-3 code "quz" instead of the
     ISO 639-1 code "qu".  */
  if (language_len == 3 && memeq (language_start, "quz", 3))
    {
      language_start = "qu";
      language_len = 2;
    }
  /* For Tamazight, Microsoft uses the ISO 639-3 code "tzm" instead of the
     ISO 639-2 code "ber".  */
  else if (language_len == 3 && memeq (language_start, "tzm", 3))
    {
      language_start = "ber";
      language_len = 3;
    }

  const char *script_subtag = NULL;
  const char *variant_start = NULL;
  size_t variant_len = 0;

  /* Determine script from the modifier.  */
  if (modifier_len > 0)
    {
      for (size_t i = 0; i < NUM_SCRIPTS; i++)
        if (strlen (scripts[i].name) == modifier_len
            && memeq (scripts[i].name, modifier_start, modifier_len))
          script_subtag = scripts[i].code;
      if (script_subtag == NULL)
        {
          /* If the modifier does not designate a script, assume that it
             designates a variant.  */
          variant_start = modifier_start;
          variant_len = modifier_len;
        }
    }

  /* Determine script from the language and possibly the territory.  */
  if (language_len > 0 && script_subtag == NULL)
    {
      /* Languages with a script that depends on the territory.  */
      if (language_len == 2 && territory_len == 2)
        {
          const struct script *sp =
            default_script_in_territory (language_start, territory_start);
          if (sp != NULL)
            script_subtag = sp->code;
          else if (memeq (language_start, "zh", 2))
            {
              if (memeq (territory_start, "CN", 2)
                  || memeq (territory_start, "SG", 2))
                script_subtag = "Hans";
              else
                script_subtag = "Hant";
            }
        }
      /* Languages with a main script and one or more alternate scripts.  */
      if (language_len == 2)
        {
          const struct script *sp =
            default_script_for_language2 (language_start);
          if (sp != NULL)
            script_subtag = sp->code;
          else if (memeq (language_start, "iu", 2))
            script_subtag = "Cans";
        }
      else if (language_len == 3)
        {
          const struct script *sp =
            default_script_for_language3 (language_start);
          if (sp != NULL)
            script_subtag = sp->code;
          else if (memeq (language_start, "nan", 3))
            script_subtag = "Hant";
        }
    }

  /* Construct the result: language[-script][-territory][-variant].  */
  if (language_len
      + (script_subtag != NULL ? 1 + 4 : 0)
      + (territory_len > 0 ? 1 + territory_len : 0)
      + (variant_len > 0 ? 1 + variant_len : 0)
      < BCP47_MAX)
    {
      char *q = bcp47;
      memcpy (q, language_start, language_len);
      q += language_len;
      if (script_subtag != NULL)
        {
          *q++ = '-';
          memcpy (q, script_subtag, 4);
          q += 4;
        }
      if (territory_len > 0)
        {
          *q++ = '-';
          memcpy (q, territory_start, territory_len);
          q += territory_len;
        }
      if (variant_len > 0)
        {
          *q++ = '-';
          memcpy (q, variant_start, variant_len);
          q += variant_len;
        }
      *q = '\0';
      return;
    }
  else
    goto fail;
}

void
bcp47_to_xpg (char *xpg, const char *bcp47, const char *codeset)
{
  /* Special cases.  */
  if (streq (bcp47, ""))
   fail:
    {
      strcpy (xpg, "");
      return;
    }

  /* Parse BCP47 as
     language{-extlang}*[-script][-region]{-variant}*{-extension}*.  */
  const char *language_start = NULL;
  size_t language_len = 0;
  const char *script_start = NULL;
  size_t script_len = 0;
  const char *region_start = NULL;
  size_t region_len = 0;
  const char *variant_start = NULL;
  size_t variant_len = 0;

  {
    bool past_script = false;
    bool past_region = false;
    bool past_variant = false;
    const char *p;

    p = bcp47;
    language_start = p;
    while (*p != '\0' && *p != '-')
      p++;
    language_len = p - language_start;
    while (*p != '\0')
      {
        if (*p == '-')
          {
            p++;
            const char *subtag_start = p;
            while (*p != '\0' && *p != '-')
              p++;
            size_t subtag_len = p - subtag_start;

            if (!past_script && subtag_len == 4)
              {
                /* Parsed -script.  */
                script_start = subtag_start;
                script_len = subtag_len;
                past_script = true;
              }
            else if (!past_region
                     && (subtag_len == 2
                         || (subtag_len == 3
                             && subtag_start[0] >= '0' && subtag_start[0] <= '9'
                             && subtag_start[1] >= '0' && subtag_start[1] <= '9'
                             && subtag_start[2] >= '0' && subtag_start[2] <= '9')))
              {
                /* Parsed -region.  */
                region_start = subtag_start;
                region_len = subtag_len;
                past_region = true;
                past_script = true;
              }
            else
              {
                /* Is it -extlang or -variant or -extension?  */
                if (!past_script && subtag_len == 3)
                  {
                    /* It is -extlang.  */
                  }
                else
                  {
                    /* It must be -variant or -extension.  */
                    past_script = true;
                    past_region = true;
                    if (!past_variant)
                      {
                        variant_start = subtag_start;
                        variant_len = subtag_len;
                        past_variant = true;
                      }
                  }
              }
          }
        else
          abort ();
      }
  }

  if (language_len == 0 || language_len >= BCP47_MAX)
    /* No language or too long -> fail.  */
    goto fail;

  /* Copy the language to the result buffer, converting it to lower case.  */
  for (size_t i = 0; i < language_len; i++)
    xpg[i] = c_tolower (language_start[i]);

  /* Canonicalize the language.  */
  /* For Quechua, Microsoft uses the ISO 639-3 code "quz" instead of the
     ISO 639-1 code "qu".  */
  if (language_len == 3 && memeq (xpg, "quz", 3))
    {
      language_len = 2;
      memcpy (xpg, "qu", language_len);
    }
  /* For Tamazight, Microsoft uses the ISO 639-3 code "tzm" instead of the
     ISO 639-2 code "ber".  */
  else if (language_len == 3 && memeq (xpg, "tzm", 3))
    {
      language_len = 3;
      memcpy (xpg, "ber", language_len);
    }

  /* Copy the region to a temporary buffer, converting it to upper case.  */
  char territory[3];
  size_t territory_len = region_len; /* == 2 or 3 */
  for (size_t i = 0; i < region_len; i++)
    territory[i] = c_toupper (region_start[i]);

  /* Determine script from the script subtag.  */
  const char *script = NULL;

  if (script_len > 0)
    {
      /* Here script_len == 4.  */
      for (size_t i = 0; i < NUM_SCRIPTS; i++)
        if (c_toupper (script_start[0] == scripts[i].code[0])
            && c_tolower (script_start[1] == scripts[i].code[1])
            && c_tolower (script_start[2] == scripts[i].code[2])
            && c_tolower (script_start[3] == scripts[i].code[3]))
          script = scripts[i].name;
    }

  /* Possibly strip away the script, depending on the language and possibly
     the territory.  */
  if (script != NULL)
    {
      /* Languages with a script that depends on the territory.  */
      if (language_len == 2 && territory_len == 2)
        {
          const struct script *sp =
            default_script_in_territory (xpg, territory);
          if (sp != NULL)
            {
              if (streq (script, sp->name))
                script = NULL;
            }
          else if (memeq (xpg, "zh", 2))
            {
              /* "Hans" and "Hant" are not present in the scripts[] table,
                 therefore nothing to do here.  */
            }
        }
      /* Languages with a main script and one or more alternate scripts.  */
      if (language_len == 2)
        {
          const struct script *sp =
            default_script_for_language2 (xpg);
          if (sp != NULL)
            {
              if (streq (script, sp->name))
                script = NULL;
            }
          else if (memeq (xpg, "iu", 2))
            {
              /* "Cans" is not present in the scripts[] table,
                 therefore nothing to do here.  */
            }
        }
      else if (language_len == 3)
        {
          const struct script *sp =
            default_script_for_language3 (xpg);
          if (sp != NULL)
            {
              if (streq (script, sp->name))
                script = NULL;
            }
          else if (memeq (xpg, "nan", 3))
            {
              /* "Hant" is not present in the scripts[] table,
                 therefore nothing to do here.  */
            }
        }
    }

  /* Construct the result: language[_territory][.codeset][@modifier].  */
  size_t codeset_len = (codeset != NULL ? strlen (codeset) : 0);
  /* The modifier is the script or, if the script is absent or already handled,
     the variant.  */
  const char *modifier;
  size_t modifier_len;
  if (script != NULL)
    {
      modifier = script;
      modifier_len = strlen (modifier);
    }
  else if (variant_len > 0)
    {
      modifier = variant_start;
      modifier_len = variant_len;
    }
  else
    {
      modifier = NULL;
      modifier_len = 0;
    }
  if (language_len
      + (territory_len > 0 ? 1 + territory_len : 0)
      + (codeset != NULL ? 1 + codeset_len : 0)
      + (modifier != NULL ? 1 + modifier_len : 0)
      < BCP47_MAX)
    {
      char *q = xpg;
      q += language_len;
      if (territory_len > 0)
        {
          *q++ = '_';
          memcpy (q, territory, territory_len);
          q += territory_len;
        }
      if (codeset != NULL)
        {
          *q++ = '.';
          memcpy (q, codeset, codeset_len);
          q += codeset_len;
        }
      if (modifier != NULL)
        {
          *q++ = '@';
          memcpy (q, modifier, modifier_len);
          q += modifier_len;
        }
      *q = '\0';
      return;
    }
  else
    goto fail;
}
