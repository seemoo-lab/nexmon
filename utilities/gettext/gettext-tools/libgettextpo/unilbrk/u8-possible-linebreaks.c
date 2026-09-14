/* Line breaking of UTF-8 strings.
   Copyright (C) 2001-2003, 2006-2026 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2001.

   This file is free software.
   It is dual-licensed under "the GNU LGPLv3+ or the GNU GPLv2+".
   You can redistribute it and/or modify it under either
     - the terms of the GNU Lesser General Public License as published
       by the Free Software Foundation, either version 3, or (at your
       option) any later version, or
     - the terms of the GNU General Public License as published by the
       Free Software Foundation; either version 2, or (at your option)
       any later version, or
     - the same dual license "the GNU LGPLv3+ or the GNU GPLv2+".

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License and the GNU General Public License
   for more details.

   You should have received a copy of the GNU Lesser General Public
   License and of the GNU General Public License along with this
   program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include "unilbrk.h"
#include "unilbrk/internal.h"

#include <stdlib.h>
#include <string.h>

#include "unilbrk/lbrktables.h"
#include "uniwidth/cjk.h"
#include "unistr.h"

/* This file implements
   Unicode Standard Annex #14 <https://www.unicode.org/reports/tr14/>.  */

void
u8_possible_linebreaks_loop (const uint8_t *s, size_t n, const char *encoding,
                             int cr, char *p)
{
  if (n > 0)
    {
      int LBP_AI_REPLACEMENT = (is_cjk_encoding (encoding) ? LBP_ID : LBP_AL1);

      /* Don't break inside multibyte characters.  */
      memset (p, UC_BREAK_PROHIBITED, n);

      const uint8_t *s_end = s + n;

      /* We need 2 characters of lookahead:
           - 1 character of lookahead for (LB15c,LB19a,LB28a),
           - 2 characters of lookahead for (LB25).  */
      const uint8_t *lookahead1_end;
      ucs4_t lookahead1_uc;
      int lookahead1_prop_ea;
      const uint8_t *lookahead2_end;
      ucs4_t lookahead2_uc;
      int lookahead2_prop_ea;
      /* Get the first lookahead character.  */
      lookahead1_end = s;
      lookahead1_end += u8_mbtouc_unsafe (&lookahead1_uc, lookahead1_end, s_end - lookahead1_end);
      lookahead1_prop_ea = unilbrkprop_lookup (lookahead1_uc);
      /* Get the second lookahead character.  */
      lookahead2_end = lookahead1_end;
      if (lookahead2_end < s_end)
        {
          lookahead2_end += u8_mbtouc_unsafe (&lookahead2_uc, lookahead2_end, s_end - lookahead2_end);
          lookahead2_prop_ea = unilbrkprop_lookup (lookahead2_uc);
        }
      else
        {
          lookahead2_uc = 0xFFFD;
          lookahead2_prop_ea = PROP_EA (LBP_BK, 0);
        }

      int preceding_prop = LBP_BK; /* line break property of preceding character */
      int prev_prop = LBP_BK; /* line break property of previous character
                                 (= last character, ignoring intervening characters of class CM or ZWJ) */
      int prev_ea = 0;        /* EastAsian property of previous character
                                 (= last character, ignoring intervening characters of class CM or ZWJ) */
      int prev2_ea = 0;       /* EastAsian property of character before the previous character */
      bool prev_initial_hyphen = false; /* the previous character was a
                                           word-initial hyphen or unambiguous hyphen */
      bool prev_nus = false; /* before the previous character, there was a character
                                with line break property LBP_NU and since then
                                only characters with line break property LBP_SY
                                or LBP_IS */
      int last_prop = LBP_BK; /* line break property of last non-space character
                                 (= last character, ignoring intervening characters of class SP or CM or ZWJ) */
      char *seen_space = NULL; /* Was a space seen after the last non-space character? */

      /* Number of consecutive regional indicator (RI) characters seen
         immediately before the current point.  */
      size_t ri_count = 0;

      do
        {
          /* Read the next character.  */
          size_t count = lookahead1_end - s;
          s = lookahead1_end;
          ucs4_t uc = lookahead1_uc;
          int prop_ea = lookahead1_prop_ea; /* = unilbrkprop_lookup (uc); */
          int prop = PROP (prop_ea); /* line break property of uc */
          int ea = EA (prop_ea);     /* EastAsian property of uc */
          /*  Refill the pipeline of 2 lookahead characters.  */
          lookahead1_end = lookahead2_end;
          lookahead1_uc = lookahead2_uc;
          lookahead1_prop_ea = lookahead2_prop_ea;
          if (lookahead2_end < s_end)
            {
              lookahead2_end += u8_mbtouc_unsafe (&lookahead2_uc, lookahead2_end, s_end - lookahead2_end);
              lookahead2_prop_ea = unilbrkprop_lookup (lookahead2_uc);
            }
          else
            {
              lookahead2_uc = 0xFFFD;
              lookahead2_prop_ea = PROP_EA (LBP_BK, 0);
            }

          bool nus = /* ending at the previous character, there was a character
                        with line break property LBP_NU and since then only
                        characters with line break property LBP_SY or LBP_IS */
            (prev_prop == LBP_NU
             || (prev_nus && (prev_prop == LBP_SY || prev_prop == LBP_IS)));

          if (prop == LBP_BK || prop == LBP_LF || prop == LBP_CR)
            {
              /* (LB4,LB5,LB6) Mandatory break.  */
              *p = UC_BREAK_MANDATORY;
              /* cr is either LBP_CR or -1.  In the first case, recognize
                 a CR-LF sequence.  */
              if (prev_prop == cr && prop == LBP_LF)
                p[-1] = UC_BREAK_CR_BEFORE_LF;
              last_prop = LBP_BK;
              seen_space = NULL;
            }
          else
            {
              /* Resolve property values whose behaviour is not fixed.  */
              switch (prop)
                {
                case LBP_AI:
                  /* Resolve ambiguous.  */
                  prop = LBP_AI_REPLACEMENT;
                  break;
                case LBP_CB:
                  /* This is arbitrary.  */
                  prop = LBP_ID;
                  break;
                case LBP_SA1:
                  /* We don't handle complex scripts yet.
                     Treat LBP_SA1 like LBP_XX.  */
                case LBP_XX:
                  /* This is arbitrary.  */
                  prop = LBP_AL1;
                  break;
                }

              /* Deal with spaces and combining characters.  */
              if (prop == LBP_SP)
                {
                  /* (LB7) Don't break just before a space.  */
                  *p = UC_BREAK_PROHIBITED;
                  seen_space = p;
                }
              else if (prop == LBP_ZW)
                {
                  /* (LB7) Don't break just before a zero-width space.  */
                  *p = UC_BREAK_PROHIBITED;
                  last_prop = LBP_ZW;
                  seen_space = NULL;
                }
              else if (prop == LBP_CM || prop == LBP_SA2 || prop == LBP_ZWJ)
                {
                  /* (LB9) Don't break just before a combining character or
                     zero-width joiner, except immediately after a mandatory
                     break character, space, or zero-width space.  */
                  if (last_prop == LBP_BK)
                    {
                      /* (LB4,LB5,LB6) Don't break at the beginning of a line.  */
                      *p = UC_BREAK_PROHIBITED;
                      /* (LB10) Treat CM or ZWJ as AL.  */
                      last_prop = LBP_AL1;
                      seen_space = NULL;
                    }
                  else if (last_prop == LBP_ZW
                           || (seen_space != NULL
                               /* (LB14) has higher priority than (LB18).  */
                               && !(last_prop == LBP_OP1 || last_prop == LBP_OP2)
                               /* (LB15a) has higher priority than (LB18).  */
                               && !(last_prop == LBP_QU2)))
                    {
                      /* (LB8) Break after zero-width space.  */
                      /* (LB18) Break after spaces.
                         We do *not* implement the "legacy support for space
                         character as base for combining marks" because now the
                         NBSP CM sequence is recommended instead of SP CM.  */
                      *p = UC_BREAK_POSSIBLE;
                      /* (LB10) Treat CM or ZWJ as AL.  */
                      last_prop = LBP_AL1;
                      seen_space = NULL;
                    }
                  else
                    {
                      /* Treat X CM as if it were X.  */
                      *p = UC_BREAK_PROHIBITED;
                    }
                }
              else
                {
                  /* prop must be usable as an index for table 7.3 of UTR #14.  */
                  if (!(prop >= 0 && prop < sizeof (unilbrk_table) / sizeof (unilbrk_table[0])))
                    abort ();

                  if (last_prop == LBP_BK)
                    {
                      /* (LB4,LB5,LB6) Don't break at the beginning of a line.  */
                      *p = UC_BREAK_PROHIBITED;
                    }
                  else if (last_prop == LBP_ZW)
                    {
                      /* (LB8) Break after zero-width space.  */
                      *p = UC_BREAK_POSSIBLE;
                    }
                  else if (preceding_prop == LBP_ZWJ)
                    {
                      /* (LB8a) Don't break right after a zero-width joiner.  */
                      *p = UC_BREAK_PROHIBITED;
                    }
                  else if (prop == LBP_IS && prev_prop == LBP_SP
                           && PROP (lookahead1_prop_ea) == LBP_NU)
                    {
                      /* (LB15c) Break before a decimal mark that follows a space.  */
                      *p = UC_BREAK_POSSIBLE;
                    }
                  else if (((prop == LBP_QU1 || prop == LBP_QU2 || prop == LBP_QU3)
                            && (! prev_ea || ! EA (lookahead1_prop_ea))
                            /* (LB18) has higher priority than (LB19a).  */
                            && prev_prop != LBP_SP)
                           || ((prev_prop == LBP_QU1 || prev_prop == LBP_QU2 || prev_prop == LBP_QU3)
                               && (! prev2_ea || ! ea)))
                    {
                      /* (LB19a) Don't break on either side of ambiguous
                         quotation marks, except next to an EastAsian character.  */
                      *p = UC_BREAK_PROHIBITED;
                    }
                  else if (prev_initial_hyphen
                           && (prop == LBP_AL1 || prop == LBP_AL2 || prop == LBP_HL))
                    {
                      /* (LB20a) Don't break after a word-initial hyphen.  */
                      *p = UC_BREAK_PROHIBITED;
                    }
                  else if (prev_prop == LBP_HL_HY && prop != LBP_HL)
                    {
                      /* (LB21a) Don't break after Hebrew + Hyphen/Unambiguous hyphen,
                         before non-Hebrew.  */
                      *p = UC_BREAK_PROHIBITED;
                    }
                  else if ((prev_nus
                            && (prev_prop == LBP_CL
                                || prev_prop == LBP_CP1 || prev_prop == LBP_CP2)
                            && (prop == LBP_PO || prop == LBP_PR))
                           || (nus && (prop == LBP_PO || prop == LBP_PR
                                       || prop == LBP_NU)))
                    {
                      /* (LB25) Don't break numbers.  */
                      *p = UC_BREAK_PROHIBITED;
                    }
                  else if ((prev_prop == LBP_PO || prev_prop == LBP_PR)
                           && (prop == LBP_OP1 || prop == LBP_OP2)
                           && (PROP (lookahead1_prop_ea) == LBP_NU
                               || (PROP (lookahead1_prop_ea) == LBP_IS
                                   && PROP (lookahead2_prop_ea) == LBP_NU)))
                    {
                      /* (LB25) Don't break numbers.  */
                      *p = UC_BREAK_PROHIBITED;
                    }
                  else if (prev_prop == LBP_AKLS_VI
                           && (prop == LBP_AK || prop == LBP_AL2))
                    {
                      /* (LB28a) Don't break inside orthographic syllables of
                         Brahmic scripts, line 3.  */
                      *p = UC_BREAK_PROHIBITED;
                    }
                  else if (PROP (lookahead1_prop_ea) == LBP_VF
                           && (prop == LBP_AK || prop == LBP_AL2 || prop == LBP_AS)
                           && (prev_prop == LBP_AK || prev_prop == LBP_AL2 || prev_prop == LBP_AS))
                    {
                      /* (LB28a) Don't break inside orthographic syllables of
                         Brahmic scripts, line 4.  */
                      *p = UC_BREAK_PROHIBITED;
                    }
                  else if (last_prop == LBP_IS && uc == 0x003C)
                    {
                      /* Partially disable (LB29) Do not break between numeric
                         punctuation and alphabetics ("e.g.").  We find it
                         desirable to break before the HTML tag "</P>" in
                         strings like "<P>Some sentence.</P>".  */
                      *p = UC_BREAK_POSSIBLE;
                    }
                  else if (last_prop == LBP_RI && prop == LBP_RI)
                    {
                      /* (LB30a) Break between two regional indicator symbols
                         if and only if there are an even number of regional
                         indicators preceding the position of the break.  */
                      *p = (seen_space != NULL || (ri_count % 2) == 0
                            ? UC_BREAK_POSSIBLE
                            : UC_BREAK_PROHIBITED);
                    }
                  else
                    {
                      int this_prop = prop;
                      if (prop == LBP_QU3)
                        {
                          /* For (LB15b): Replace LBP_QU3 with LBP_QU1 if the
                             next character's line break property is not one of
                             BK, CR, LF, SP, GL, WJ, CL, QU, CP, EX, IS, SY, ZW.  */
                          switch (PROP (lookahead1_prop_ea))
                            {
                            case LBP_BK:
                            case LBP_CR:
                            case LBP_LF:
                            case LBP_SP:
                            case LBP_GL:
                            case LBP_WJ:
                            case LBP_CL:
                            case LBP_QU1: case LBP_QU2: case LBP_QU3:
                            case LBP_CP1: case LBP_CP2:
                            case LBP_EX:
                            case LBP_IS:
                            case LBP_SY:
                            case LBP_ZW:
                              break;
                            default:
                              this_prop = LBP_QU1;
                              break;
                            }
                        }

                      switch (unilbrk_table [last_prop] [this_prop])
                        {
                        case D:
                          *p = UC_BREAK_POSSIBLE;
                          break;
                        case I:
                          *p = (seen_space != NULL ? UC_BREAK_POSSIBLE : UC_BREAK_PROHIBITED);
                          break;
                        case P:
                          *p = UC_BREAK_PROHIBITED;
                          break;
                        default:
                          abort ();
                        }
                    }

                  if (prop == LBP_QU2)
                    {
                      /* For (LB15a): Replace LBP_QU2 with LBP_QU1 if the
                         previous character's line break property was not one of
                         BK, CR, LF, OP, QU, GL, SP, ZW.  */
                      switch (prev_prop)
                        {
                        case LBP_BK:
                        case LBP_CR:
                        case LBP_LF:
                        case LBP_OP1: case LBP_OP2:
                        case LBP_QU1: case LBP_QU2: case LBP_QU3:
                        case LBP_GL:
                        case LBP_SP:
                        case LBP_ZW:
                          break;
                        default:
                          prop = LBP_QU1;
                          break;
                        }
                    }

                  last_prop = prop;
                  seen_space = NULL;
                }
            }

          /* (LB9) Treat X (CM | ZWJ)* as if it were X, where X is any line
             break class except BK, CR, LF, NL, SP, or ZW.  */
          if (!((prop == LBP_CM || prop == LBP_ZWJ)
                && !(prev_prop == LBP_BK || prev_prop == LBP_LF || prev_prop == LBP_CR
                     || prev_prop == LBP_SP || prev_prop == LBP_ZW)))
            {
              prev_initial_hyphen =
                (prop == LBP_HY || prop == LBP_HH)
                && (prev_prop == LBP_BK || prev_prop == LBP_CR || prev_prop == LBP_LF
                    || prev_prop == LBP_SP || prev_prop == LBP_ZW
                    || prev_prop == LBP_CB || prev_prop == LBP_GL);
              prev_prop = (prop == LBP_VI && (prev_prop == LBP_AK
                                              || prev_prop == LBP_AL2
                                              || prev_prop == LBP_AS)
                           ? LBP_AKLS_VI :
                           prev_prop == LBP_HL && (prop == LBP_HY || prop == LBP_HH)
                           ? LBP_HL_HY :
                           prop);
              prev2_ea = prev_ea;
              prev_ea = ea;
              prev_nus = nus;
            }

          preceding_prop = prop;

          if (prop == LBP_RI)
            ri_count++;
          else
            ri_count = 0;

          p += count;
        }
      while (s < s_end);
    }
}

#if defined IN_LIBUNISTRING
/* For backward compatibility with older versions of libunistring.  */

# undef u8_possible_linebreaks

void
u8_possible_linebreaks (const uint8_t *s, size_t n, const char *encoding,
                        char *p)
{
  u8_possible_linebreaks_loop (s, n, encoding, -1, p);
}

#endif

void
u8_possible_linebreaks_v2 (const uint8_t *s, size_t n, const char *encoding,
                           char *p)
{
  u8_possible_linebreaks_loop (s, n, encoding, LBP_CR, p);
}


#ifdef TEST

#include <stdio.h>
#include <string.h>

/* Read the contents of an input stream, and return it, terminated with a NUL
   byte. */
char *
read_file (FILE *stream)
{
#define BUFSIZE 4096
  char *buf = NULL;
  int alloc = 0;
  int size = 0;

  while (! feof (stream))
    {
      if (size + BUFSIZE > alloc)
        {
          alloc = alloc + alloc / 2;
          if (alloc < size + BUFSIZE)
            alloc = size + BUFSIZE;
          buf = realloc (buf, alloc);
          if (buf == NULL)
            {
              fprintf (stderr, "out of memory\n");
              exit (1);
            }
        }
      int count = fread (buf + size, 1, BUFSIZE, stream);
      if (count == 0)
        {
          if (ferror (stream))
            {
              perror ("fread");
              exit (1);
            }
        }
      else
        size += count;
    }
  buf = realloc (buf, size + 1);
  if (buf == NULL)
    {
      fprintf (stderr, "out of memory\n");
      exit (1);
    }
  buf[size] = '\0';
  return buf;
#undef BUFSIZE
}

int
main (int argc, char * argv[])
{
  if (argc == 1)
    {
      /* Display all the break opportunities in the input string.  */
      char *input = read_file (stdin);
      int length = strlen (input);
      char *breaks = malloc (length);

      u8_possible_linebreaks_v2 ((uint8_t *) input, length, "UTF-8", breaks);

      for (int i = 0; i < length; i++)
        {
          switch (breaks[i])
            {
            case UC_BREAK_POSSIBLE:
              /* U+2027 in UTF-8 encoding */
              putc (0xe2, stdout); putc (0x80, stdout); putc (0xa7, stdout);
              break;
            case UC_BREAK_MANDATORY:
              /* U+21B2 (or U+21B5) in UTF-8 encoding */
              putc (0xe2, stdout); putc (0x86, stdout); putc (0xb2, stdout);
              break;
            case UC_BREAK_CR_BEFORE_LF:
              /* U+21E4 in UTF-8 encoding */
              putc (0xe2, stdout); putc (0x87, stdout); putc (0xa4, stdout);
              break;
            case UC_BREAK_PROHIBITED:
              break;
            default:
              abort ();
            }
          putc (input[i], stdout);
        }

      free (breaks);

      return 0;
    }
  else
    return 1;
}

#endif /* TEST */
