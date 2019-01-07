/* Pattern Matcher for Fixed String search.
   Copyright (C) 1992, 1998, 2000, 2005-2006, 2010, 2015-2016 Free Software
   Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "libgrep.h"

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#if defined HAVE_WCTYPE_H && defined HAVE_WCHAR_H && defined HAVE_MBRTOWC
/* We can handle multibyte string.  */
# define MBS_SUPPORT
# include <wchar.h>
# include <wctype.h>
#endif

#include "error.h"
#include "exitfail.h"
#include "xalloc.h"
#include "kwset.h"
#include "gettext.h"
#define _(str) gettext (str)

#if defined (STDC_HEADERS) || (!defined (isascii) && !defined (HAVE_ISASCII))
# define IN_CTYPE_DOMAIN(c) 1
#else
# define IN_CTYPE_DOMAIN(c) isascii(c)
#endif
#define ISUPPER(C) (IN_CTYPE_DOMAIN (C) && isupper (C))
#define TOLOWER(C) (ISUPPER(C) ? tolower(C) : (C))
#define ISALNUM(C) (IN_CTYPE_DOMAIN (C) && isalnum (C))
#define IS_WORD_CONSTITUENT(C) (ISALNUM(C) || (C) == '_')

#define NCHAR (UCHAR_MAX + 1)

struct compiled_kwset {
  kwset_t kwset;
  char *trans;
  bool match_words;
  bool match_lines;
  char eolbyte;
};

static void
kwsinit (struct compiled_kwset *ckwset,
         bool match_icase, bool match_words, bool match_lines, char eolbyte)
{
  if (match_icase)
    {
      int i;

      ckwset->trans = XNMALLOC (NCHAR, char);
      for (i = 0; i < NCHAR; i++)
        ckwset->trans[i] = TOLOWER (i);
      ckwset->kwset = kwsalloc (ckwset->trans);
    }
  else
    {
      ckwset->trans = NULL;
      ckwset->kwset = kwsalloc (NULL);
    }
  if (ckwset->kwset == NULL)
    error (exit_failure, 0, _("memory exhausted"));
  ckwset->match_words = match_words;
  ckwset->match_lines = match_lines;
  ckwset->eolbyte = eolbyte;
}

static void *
Fcompile (const char *pattern, size_t pattern_size,
          bool match_icase, bool match_words, bool match_lines,
          char eolbyte)
{
  struct compiled_kwset *ckwset;
  const char *beg;
  const char *err;

  ckwset = XMALLOC (struct compiled_kwset);
  kwsinit (ckwset, match_icase, match_words, match_lines, eolbyte);

  beg = pattern;
  do
    {
      const char *lim;

      for (lim = beg; lim < pattern + pattern_size && *lim != '\n'; ++lim)
        ;
      if ((err = kwsincr (ckwset->kwset, beg, lim - beg)) != NULL)
        error (exit_failure, 0, "%s", err);
      if (lim < pattern + pattern_size)
        ++lim;
      beg = lim;
    }
  while (beg < pattern + pattern_size);

  if ((err = kwsprep (ckwset->kwset)) != NULL)
    error (exit_failure, 0, "%s", err);
  return ckwset;
}

#ifdef MBS_SUPPORT
/* This function allocate the array which correspond to "buf".
   Then this check multibyte string and mark on the positions which
   are not singlebyte character nor the first byte of a multibyte
   character.  Caller must free the array.  */
static char*
check_multibyte_string (const char *buf, size_t buf_size)
{
  char *mb_properties = (char *) malloc (buf_size);
  mbstate_t cur_state;
  int i;

  memset (&cur_state, 0, sizeof (mbstate_t));
  memset (mb_properties, 0, sizeof (char) * buf_size);
  for (i = 0; i < buf_size ;)
    {
      size_t mbclen;
      mbclen = mbrlen (buf + i, buf_size - i, &cur_state);

      if (mbclen == (size_t) -1 || mbclen == (size_t) -2 || mbclen == 0)
        {
          /* An invalid sequence, or a truncated multibyte character.
             We treat it as a singlebyte character.  */
          mbclen = 1;
        }
      mb_properties[i] = mbclen;
      i += mbclen;
    }

  return mb_properties;
}
#endif

static size_t
Fexecute (const void *compiled_pattern, const char *buf, size_t buf_size,
          size_t *match_size, bool exact)
{
  struct compiled_kwset *ckwset = (struct compiled_kwset *) compiled_pattern;
  char eol = ckwset->eolbyte;
  register const char *buflim = buf + buf_size;
  register const char *beg;
  register size_t len;
#ifdef MBS_SUPPORT
  char *mb_properties;
  if (MB_CUR_MAX > 1)
    mb_properties = check_multibyte_string (buf, buf_size);
#endif /* MBS_SUPPORT */

  for (beg = buf; beg <= buflim; ++beg)
    {
      struct kwsmatch kwsmatch;
      size_t offset = kwsexec (ckwset->kwset, beg, buflim - beg, &kwsmatch);
      if (offset == (size_t) -1)
        {
#ifdef MBS_SUPPORT
          if (MB_CUR_MAX > 1)
            free (mb_properties);
#endif /* MBS_SUPPORT */
          return offset;
        }
#ifdef MBS_SUPPORT
      if (MB_CUR_MAX > 1 && mb_properties[offset+beg-buf] == 0)
        continue; /* It is a part of multibyte character.  */
#endif /* MBS_SUPPORT */
      beg += offset;
      len = kwsmatch.size[0];
      if (exact)
        {
          *match_size = len;
#ifdef MBS_SUPPORT
          if (MB_CUR_MAX > 1)
            free (mb_properties);
#endif /* MBS_SUPPORT */
          return beg - buf;
        }
      if (ckwset->match_lines)
        {
          if (beg > buf && beg[-1] != eol)
            continue;
          if (beg + len < buflim && beg[len] != eol)
            continue;
          goto success;
        }
      else if (ckwset->match_words)
        {
          register const char *curr;
          for (curr = beg; len; )
            {
              if (curr > buf && IS_WORD_CONSTITUENT ((unsigned char) curr[-1]))
                break;
              if (curr + len < buflim
                  && IS_WORD_CONSTITUENT ((unsigned char) curr[len]))
                {
                  offset = kwsexec (ckwset->kwset, beg, --len, &kwsmatch);
                  if (offset == (size_t) -1)
                    {
#ifdef MBS_SUPPORT
                      if (MB_CUR_MAX > 1)
                        free (mb_properties);
#endif /* MBS_SUPPORT */
                      return offset;
                    }
                  curr = beg + offset;
                  len = kwsmatch.size[0];
                }
              else
                goto success;
            }
        }
      else
        goto success;
    }

#ifdef MBS_SUPPORT
  if (MB_CUR_MAX > 1)
    free (mb_properties);
#endif /* MBS_SUPPORT */
  return -1;

 success:
  {
    register const char *end;

    end = (const char *) memchr (beg + len, eol, buflim - (beg + len));
    if (end != NULL)
      end++;
    else
      end = buflim;
    while (buf < beg && beg[-1] != eol)
      --beg;
    *match_size = end - beg;
#ifdef MBS_SUPPORT
    if (MB_CUR_MAX > 1)
      free (mb_properties);
#endif /* MBS_SUPPORT */
    return beg - buf;
  }
}

static void
Ffree (void *compiled_pattern)
{
  struct compiled_kwset *ckwset = (struct compiled_kwset *) compiled_pattern;

  free (ckwset->trans);
  free (ckwset);
}

matcher_t matcher_fgrep =
  {
    Fcompile,
    Fexecute,
    Ffree
  };

