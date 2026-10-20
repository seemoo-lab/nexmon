/* t-strlist.c - Check strlist functions
 * Copyright (C) 2015, 2025 g10 Code GmbH
 *
 * This file is part of Libgpg-error.
 *
 * Libgpg-error is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Libgpg-error is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#ifdef HAVE_STAT
# include <sys/stat.h>
#endif
#include <sys/types.h>
#ifdef HAVE_PWD_H
# include <pwd.h>
#endif
#include <errno.h>
#ifdef HAVE_W32_SYSTEM
# include <windows.h>
#endif

#define PGM "t-strutils"
#include "t-common.h"


static const char *
my_strusage (int level)
{
  const char *p;

  switch (level)
    {
    case 9: p = "LGPL-2.1-or-later"; break;
    case 11: p = PGM; break;
    default: p = NULL;
    }
  return p;
}


static void
check_strlist_rev (void)
{
  gpgrt_strlist_t s = NULL;

  enter_test_function ();

  /* Reversing an empty list should yield the empty list.  */
  if (! (gpgrt_strlist_rev (&s) == NULL))
    fail ("reversing an empty list does not yield an empty list");

  gpgrt_strlist_add (&s, "1", 0);
  gpgrt_strlist_add (&s, "2", 0);
  gpgrt_strlist_add (&s, "3", 0);

  if (strcmp (s->d, "3") != 0)
    fail ("failed at %d", __LINE__);
  if (strcmp (s->next->d, "2") != 0)
    fail ("failed at %d", __LINE__);
  if (strcmp (s->next->next->d, "1") != 0)
    fail ("failed at %d", __LINE__);
  if (s->next->next->next)
    fail ("failed at %d", __LINE__);

  gpgrt_strlist_rev (&s);

  if (strcmp (s->d, "1") != 0)
    fail ("failed at %d", __LINE__);
  if (strcmp (s->next->d, "2") != 0)
    fail ("failed at %d", __LINE__);
  if (strcmp (s->next->next->d, "3") != 0)
    fail ("failed at %d", __LINE__);
  if (s->next->next->next)
    fail ("failed at %d", __LINE__);

  gpgrt_strlist_free (s);
  leave_test_function ();
}


static void
check_tokenize_to_strlist (void)
{
  struct {
    const char *s;
    const char *delim;
    int error_expected;
    const char *items_expected[10];
  } tv[] = {
    {
      "", ":",
      1, { NULL }
    },
    {
      "a", ":",
      0, { "a", NULL }
    },
    {
      ":", ":",
      1, { NULL }
    },
    {
      "::", ":",
      1, { NULL }
    },
    {
      "a:b:c", ":",
      0, { "a", "b", "c", NULL }
    },
    {
      "a:b:", ":",
      0, { "a", "b", NULL }
    },
    {
      "a:b", ":",
      0, { "a", "b", NULL }
    },
    {
      "aa:b:cd", ":",
      0, { "aa", "b", "cd", NULL }
    },
    {
      "aa::b:cd", ":",
      0, { "aa", "b", "cd", NULL }
    },
    {
      "::b:cd", ":",
      0, { "b", "cd", NULL }
    },
    {
      "aa:   : b:cd ", ":",
      0, { "aa", "b", "cd", NULL }
    },
    {
      "  aa:   : b:  cd ", ":",
      0, { "aa", "b", "cd", NULL }
    },
    {
      "  :", ":",
      1, { NULL }
    },
    {
      "  : ", ":",
      1, { NULL }
    },
    {
      ": ", ":",
      1, { NULL }
    },
    {
      ": x ", ":",
      0, { "x", NULL }
    },
    {
      "a:bc:cde:fghi:jklmn::foo:", ":",
      0, { "a", "bc", "cde", "fghi", "jklmn", "foo", NULL }
    },
    {
      ",a,bc,,def,", ",",
      0, { "a", "bc", "def", NULL }
    },
    {
      " a ", " ",
      0, { "a", NULL }
    },
    {
      " ", " ",
      1, { NULL }
    },
    {
      "a:bc:c de:fg   hi:jklmn::foo :", ":",
      0, { "a", "bc", "c de", "fg   hi", "jklmn", "foo", NULL }
    },
    {
      "", " ",
      1, { NULL }
    }
  };
  const char *prefixes[3] = { "abc", "bcd", "efg" };
  int tidx;
  int nprefixes; /* Number of items in already in the list.  */
  gpgrt_strlist_t list = NULL;

  enter_test_function ();

  for (nprefixes = 0; nprefixes < DIM (prefixes); nprefixes++)
    for (tidx = 0; tidx < DIM(tv); tidx++)
      {
        int item_count_expected;
        int i;
        gpgrt_strlist_t sl, newitems;

        for (item_count_expected = 0;
             tv[tidx].items_expected[item_count_expected];
             item_count_expected++)
          ;

        /* printf  ("np=%d testing %d \"%s\" delim=\"%s\"\n", */
        /*          nprefixes, tidx, tv[tidx].s, tv[tidx].delim); */
        for (i=0; i < nprefixes; i++)
          gpgrt_strlist_add (&list, prefixes[i], GPGRT_STRLIST_APPEND);

        newitems = gpgrt_strlist_tokenize (&list, tv[tidx].s, tv[tidx].delim,
                                           0);
        if (!newitems)
          {
            if (gpg_err_code_from_syserror () == GPG_ERR_ENOENT
                && tv[tidx].error_expected)
              {
                /* Good.  But need to check the prefixes.  */
                for (sl=list, i=0; i < nprefixes; i++, sl=sl->next)
                  {
                    if (!sl || strcmp (prefixes[i], sl->d))
                      {
                        show ("for item %d prefix item %d, expected '%s'\n",
                              tidx, i, prefixes[i]);
                        fail ("error at index %d, line %d",
                              tidx * 1000 + 40 + i + 1, __LINE__);
                      }
                  }
              }
            else
              fail ("error at index %d, line %d", tidx * 1000, __LINE__);
          }
        else if (tv[tidx].error_expected)
          {
            show ("got items:");
            for (sl = list; sl; sl = sl->next)
              show (" \"%s\"", sl->d);
            fail ("error at index %d, line %d", tidx * 1000, __LINE__);
          }
        else
          {
            if (gpgrt_strlist_count (list) != nprefixes + item_count_expected)
              fail ("error at index %d, line %d", tidx * 1000, __LINE__);
            else
              {
                for (sl=list, i=0; i < nprefixes; i++, sl=sl->next)
                  {
                    if (!sl || strcmp (prefixes[i], sl->d))
                      {
                        show ("for item %d prefix item %d, expected '%s'\n",
                              tidx, i, prefixes[i]);
                        fail ("error at index %d, line %d",
                              tidx * 1000 + 50 + i + 1, __LINE__);
                      }
                  }
                for (i=0; i < item_count_expected; i++, sl=sl->next)
                  {
                    if (!sl)
                      {
                        show ("no item at item index %d\n", i);
                        fail ("error at index %d, line %d",
                              tidx * 1000 + i + 0, __LINE__);
                        break;
                      }
                    if (strcmp (tv[tidx].items_expected[i], sl->d))
                      {
                        show ("for item %d, expected '%s', but got '%s'\n",
                                i, tv[tidx].items_expected[i], sl->d);
                        fail ("error at index %d, line %d",
                              tidx * 1000 + 10 + i + 1, __LINE__);
                      }
                  }
              }
          }

        gpgrt_strlist_free (list);
        list = NULL;
      }

  leave_test_function ();
}


int
main (int argc, char **argv)
{
  gpgrt_opt_t opts[] = {
    ARGPARSE_x  ('v', "verbose", NONE, 0, "Print more diagnostics"),
    ARGPARSE_s_n('d', "debug", "Flyswatter"),
    ARGPARSE_end()
  };
  gpgrt_argparse_t pargs = { &argc, &argv, 0 };

  gpgrt_set_strusage (my_strusage);
  gpgrt_log_set_prefix (gpgrt_strusage (11), GPGRT_LOG_WITH_PREFIX);

  while (gpgrt_argparse  (NULL, &pargs, opts))
    {
      switch (pargs.r_opt)
        {
        case 'v': verbose++; break;
        case 'd': debug++; break;
        default : pargs.err = ARGPARSE_PRINT_ERROR; break;
	}
    }
  gpgrt_argparse (NULL, &pargs, NULL);

  show ("testing strlist functions\n");

  check_strlist_rev ();
  check_tokenize_to_strlist ();

  show ("testing strlist functions finished\n");
  return !!errorcount;
}
