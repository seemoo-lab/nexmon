/* t-name-value.c - Check name-value functions
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

#define PGM "t-name-value"
#include "t-common.h"


static void test_getting_values (gpgrt_nvc_t cont);
static void test_getting_section_values (gpgrt_nvc_t pk);
static void test_key_extraction (gpgrt_nvc_t pk);
static void test_iteration (gpgrt_nvc_t pk);
static void test_whitespace (gpgrt_nvc_t pk);

static int private_key_mode;
static int section_mode;

static struct
{
  char *value;
  void (*test_func) (gpgrt_nvc_t);
  int only_section_mode;
} tests[] =
  {
    {
      "# This is a comment followed by an empty line\n"
      "\n",
      NULL
    },
    {
      "# This is a comment followed by two empty lines, Windows style\r\n"
      "\r\n"
      "\r\n",
      NULL
    },
    {
      "# Some name,value pairs\n"
      "Comment: Some comment.\n"
      "SomeOtherName: Some value.\n",
      test_getting_values
    },
    {
      "  # Whitespace is preserved as much as possible\r\n"
      "Comment:Some comment.\n"
      "SomeOtherName: Some value.   \n",
      test_getting_values
    },
    {
      "# Values may be continued in the next line as indicated by leading\n"
      "# space\n"
      "Comment: Some rather long\n"
      "  comment that is continued in the next line.\n"
      "\n"
      "  Blank lines with or without whitespace are allowed within\n"
      "  continuations to allow paragraphs.\n"
      "SomeOtherName: Some value.\n",
      test_getting_values
    },
    {
      "# Names may be given multiple times forming an array of values\n"
      "Comment: Some comment, element 0.\n"
      "Comment: Some comment, element 1.\n"
      "Comment: Some comment, element 2.\n"
      "SomeOtherName: Some value.\n",
      test_iteration
    },
    {
      "# One whitespace at the beginning of a continuation is swallowed.\n"
      "One: Without\n"
      " Whitespace\n"
      "Two: With\n"
      "  Whitespace\n"
      "Three: Blank lines in continuations encode newlines.\n"
      "\n"
      "  Next paragraph.\n",
      test_whitespace
    },
    {
      "Description: Key to sign all GnuPG released tarballs.\n"
      "  The key is actually stored on a smart card.\n"
      "Use-for-ssh: yes\n"
      "OpenSSH-cert: long base64 encoded string wrapped so that this\n"
      "  key file can be easily edited with a standard editor.\n"
      "Key: (shadowed-private-key\n"
      "  (rsa\n"
      "  (n #00AA1AD2A55FD8C8FDE9E1941772D9CC903FA43B268CB1B5A1BAFDC900\n"
      "  2961D8AEA153424DC851EF13B83AC64FBE365C59DC1BD3E83017C90D4365B4\n"
      "  83E02859FC13DB5842A00E969480DB96CE6F7D1C03600392B8E08EF0C01FC7\n"
      "  19F9F9086B25AD39B4F1C2A2DF3E2BE317110CFFF21D4A11455508FE407997\n"
      "  601260816C8422297C0637BB291C3A079B9CB38A92CE9E551F80AA0EBF4F0E\n"
      "  72C3F250461E4D31F23A7087857FC8438324A013634563D34EFDDCBF2EA80D\n"
      "  F9662C9CCD4BEF2522D8BDFED24CEF78DC6B309317407EAC576D889F88ADA0\n"
      "  8C4FFB480981FB68C5C6CA27503381D41018E6CDC52AAAE46B166BDC10637A\n"
      "  E186A02BA2497FDC5D1221#)\n"
      "  (e #00010001#)\n"
      "  (shadowed t1-v1\n"
      "   (#D2760001240102000005000011730000# OPENPGP.1)\n"
      "    )))\n",
      test_key_extraction
    },
    {
      "# This is a test for section mode\n"
      "[  HKLM\\Software\\Bla\\Foo ]# the section name\n"
      "Myname: Erika\n"
      "Surname:   Musterfrau\n"
      "\n"
      "[\\Software\\blub]\n"
      "Myname: Otto Blub\n"
      "Surname: Ottoman\n"
      "[HKLM\\Software/Bla\\Foo ] # Add stuff to the first section\n"
      "Url: https://example.org\n"
      "Url: http://example.de\n"
      "Surname: Hacker\n"
      "# last comment\n",
      test_getting_section_values, 1
    }
  };



static void
test_getting_values (gpgrt_nvc_t pk)
{
  gpgrt_nve_t e;

  e = gpgrt_nvc_lookup (pk, "Comment:");
  gpgrt_assert (e);

  /* Names are case-insensitive.  */
  e = gpgrt_nvc_lookup (pk, "comment:");
  gpgrt_assert (e);
  e = gpgrt_nvc_lookup (pk, "COMMENT:");
  gpgrt_assert (e);

  e = gpgrt_nvc_lookup (pk, "SomeOtherName:");
  gpgrt_assert (e);

  /* The colon is optional.  */
  e = gpgrt_nvc_lookup (pk, "comment");
  gpgrt_assert (e);
  e = gpgrt_nvc_lookup (pk, "COMMENT");
  gpgrt_assert (e);

  e = gpgrt_nvc_lookup (pk, "SomeOtherName");
  gpgrt_assert (e);

  /* And check one which does not exist.  */
  e = gpgrt_nvc_lookup (pk, "SomeOtherNam");
  gpgrt_assert (!e);
  e = gpgrt_nvc_lookup (pk, "SomeOtherNam:");
  gpgrt_assert (!e);
  e = gpgrt_nvc_lookup (pk, "SomeOtherNam :");
  gpgrt_assert (!e);

}


static void
test_getting_section_values (gpgrt_nvc_t pk)
{
  gpgrt_nve_t e;
  const char *value;

  enter_test_function ();

  e = gpgrt_nvc_lookup (pk, "MyName");
  if (e)
    fail ("nvc_lookup unexpected succeeded at line %d\n", __LINE__);

  e = gpgrt_nvc_lookup (pk, "HKLM/Software/Bla/Foo:MyName");
  if (!e)
    fail ("nvc_lookup failed at line %d\n", __LINE__);
  else
    {
      value = gpgrt_nve_value (e);
      show ("value for %s is ->%s<-\n", gpgrt_nve_name (e), value);
    }

  e = gpgrt_nvc_lookup (pk, "HKLM/Software/Bla/Foo:SurName");
  if (!e)
    fail ("nvc_lookup failed at line %d\n", __LINE__);
  else
    {
      value = gpgrt_nve_value (e);
      show ("value for %s is ->%s<-\n", gpgrt_nve_name (e), value);
    }
  e = gpgrt_nve_next (e, "HKLM/Software/Bla/Foo:SurName");
  if (!e)
    fail ("nvc_next failed at line %d\n", __LINE__);
  else
    {
      value = gpgrt_nve_value (e);
      show ("value for %s is ->%s<-\n", gpgrt_nve_name (e), value);
    }
  e = gpgrt_nve_next (e, "HKLM/Software/Bla/Foo:surname");
  if (e)
    fail ("nve_next unexpected succeeded at line %d\n", __LINE__);

  e = gpgrt_nvc_lookup (pk, "HKLM/Software/Bla/Foo:Url");
  if (!e)
    fail ("nvc_lookup failed at line %d\n", __LINE__);
  else
    {
      value = gpgrt_nve_value (e);
      show ("value for %s is ->%s<-\n", gpgrt_nve_name (e), value);
    }
  e = gpgrt_nve_next (e, "HKLM/Software/Bla/Foo:Url");
  if (!e)
    fail ("nve_next failed at line %d\n", __LINE__);
  else
    {
      value = gpgrt_nve_value (e);
      show ("value for %s is ->%s<-\n", gpgrt_nve_name (e), value);
    }
  e = gpgrt_nve_next (e, "HKLM/Software/Bla/Foo:Url");
  if (e)
    fail ("nve_next unexpected succeeded at line %d\n", __LINE__);

  leave_test_function ();
}


static void
test_key_extraction (gpgrt_nvc_t pk)
{
  const char *key;

  /* Note that the original test in gnupg used the gnupg specific
   * private key extraction function which works only in
   * private_key_mode.  We can't do this here so we do a mere
   * extraction test.  */
  key = gpgrt_nve_value (gpgrt_nvc_lookup (pk, "Key:"));
  gpgrt_assert (key);

  if (verbose)
    fprintf (stdout, "->%s<-\n", key);
}


static void
test_iteration (gpgrt_nvc_t pk)
{
  int i;
  gpgrt_nve_t e;

  i = 0;
  for (e = gpgrt_nvc_lookup (pk, NULL); e; e = gpgrt_nve_next (e, NULL))
    i++;
  gpgrt_assert (i == 4);

  i = 0;
  for (e = gpgrt_nvc_lookup (pk, "Comment:");
       e;
       e = gpgrt_nve_next (e, "Comment:"))
    i++;
  gpgrt_assert (i == 3);
}


static void
test_whitespace (gpgrt_nvc_t pk)
{
  gpgrt_nve_t e;

  e = gpgrt_nvc_lookup (pk, "One:");
  gpgrt_assert (e);
  gpgrt_assert (strcmp (gpgrt_nve_value (e), "WithoutWhitespace") == 0);

  e = gpgrt_nvc_lookup (pk, "Two:");
  gpgrt_assert (e);
  gpgrt_assert (strcmp (gpgrt_nve_value (e), "With Whitespace") == 0);

  e = gpgrt_nvc_lookup (pk, "Three:");
  gpgrt_assert (e);
  gpgrt_assert (strcmp (gpgrt_nve_value (e),
                  "Blank lines in continuations encode newlines.\n"
                  "Next paragraph.") == 0);
}


/* Convert container PK to a string.  Terminates on error.  */
static char *
nvc_to_string (gpgrt_nvc_t pk)
{
  gpg_error_t err;
  char *buf;
  size_t len;
  estream_t sink;

  sink = es_fopenmem (0, "rw");
  gpgrt_assert (sink);

  err = gpgrt_nvc_write (pk, sink);
  gpgrt_assert (err == 0);

  len = es_ftell (sink);
  buf = xmalloc (len+1);
  gpgrt_assert (buf);

  es_fseek (sink, 0, SEEK_SET);
  es_read (sink, buf, len, NULL);
  buf[len] = 0;

  es_fclose (sink);
  return buf;
}


static void dummy_free (void *p) { (void) p; }
static void *dummy_realloc (void *p, size_t s) { (void) s; return p; }

/*
 * Run the standard test cases.
 */
static void
run_tests (void)
{
  gpg_error_t err;
  gpgrt_nvc_t pk;
  int i;
  int errlno;

  for (i = 0; i < DIM (tests); i++)
    {
      estream_t source;
      char *buf;
      size_t len;

      if (tests[i].only_section_mode && !section_mode)
        continue;

      len = strlen (tests[i].value);
      source = es_mopen (tests[i].value, len, len,
			 0, dummy_realloc, dummy_free, "r");
      gpgrt_assert (source);

      if (private_key_mode)
        err = gpgrt_nvc_parse  (&pk, &errlno, source, GPGRT_NVC_PRIVKEY);
      else if (section_mode)
        err = gpgrt_nvc_parse  (&pk, &errlno, source, GPGRT_NVC_SECTION);
      else
        err = gpgrt_nvc_parse (&pk, &errlno, source, 0);
      if (err)
        show ("parser failed at input line %d: %s\n",
              errlno, gpg_strerror (err));
      gpgrt_assert (err == 0);
      gpgrt_assert (pk);

      if (section_mode)
        buf = NULL;  /* nvc_to_string does not yet work.  */
      else
        {
          if (verbose)
            {
              err = gpgrt_nvc_write (pk, es_stderr);
              gpgrt_assert (err == 0);
            }
          buf = nvc_to_string (pk);
          gpgrt_assert (memcmp (tests[i].value, buf, len) == 0);
        }

      es_fclose (source);
      xfree (buf);

      if (tests[i].test_func)
	tests[i].test_func (pk);

      gpgrt_nvc_release (pk);
    }
}


/*
 * Run tests checking the modification functions.
 */
static void
run_modification_tests (void)
{
  gpg_error_t err;
  gpgrt_nvc_t pk;
  gpgrt_nve_t e;
  char *buf;

  enter_test_function ();

  pk = gpgrt_nvc_new (private_key_mode? GPGRT_NVC_PRIVKEY : 0);
  gpgrt_assert (pk);

  err = gpgrt_nvc_set (pk, "Foo:", "Bar");
  gpgrt_assert (!err);
  buf = nvc_to_string (pk);
  gpgrt_assert (strcmp (buf, "Foo: Bar\n") == 0);
  xfree (buf);

  err = gpgrt_nvc_set (pk, "Foo:", "Baz");
  if (err)
    fail ("gpgrt_nvc_set failed at line %d: %s\n",__LINE__,gpg_strerror (err));
  buf = nvc_to_string (pk);
  gpgrt_assert (strcmp (buf, "Foo: Baz\n") == 0);
  xfree (buf);

  err = gpgrt_nvc_set (pk, "Bar:", "Bazzel");
  if (err)
    fail ("gpgrt_nvc_set failed at line %d: %s\n",__LINE__,gpg_strerror (err));
  buf = nvc_to_string (pk);
  gpgrt_assert (strcmp (buf, "Foo: Baz\nBar: Bazzel\n") == 0);
  xfree (buf);

  err = gpgrt_nvc_add (pk, "Foo:", "Bar");
  if (err)
    fail ("gpgrt_nvc_add failed at line %d: %s\n",__LINE__,gpg_strerror (err));
  buf = nvc_to_string (pk);
  gpgrt_assert (strcmp (buf, "Foo: Baz\nFoo: Bar\nBar: Bazzel\n") == 0);
  xfree (buf);

  err = gpgrt_nvc_add (pk, "DontExistYet:", "Bar");
  if (err)
    fail ("gpgrt_nvc_set failed at line %d: %s\n",__LINE__,gpg_strerror (err));
  buf = nvc_to_string (pk);
  gpgrt_assert (strcmp (buf, ("Foo: Baz\nFoo: Bar\n"
                              "Bar: Bazzel\nDontExistYet: Bar\n")) == 0);
  xfree (buf);

  gpgrt_nvc_delete (pk, gpgrt_nvc_lookup (pk, "DontExistYet:"), NULL);
  buf = nvc_to_string (pk);
  gpgrt_assert (strcmp (buf, "Foo: Baz\nFoo: Bar\nBar: Bazzel\n") == 0);
  xfree (buf);

  gpgrt_nvc_delete (pk, gpgrt_nve_next (gpgrt_nvc_lookup (pk, "Foo:"),
                                        "Foo:"), NULL);
  buf = nvc_to_string (pk);
  gpgrt_assert (strcmp (buf, "Foo: Baz\nBar: Bazzel\n") == 0);
  xfree (buf);

  gpgrt_nvc_delete (pk, gpgrt_nvc_lookup (pk, "Foo:"), NULL);
  buf = nvc_to_string (pk);
  gpgrt_assert (strcmp (buf, "Bar: Bazzel\n") == 0);
  xfree (buf);

  gpgrt_nvc_delete (pk, gpgrt_nvc_lookup (pk, NULL), NULL);
  buf = nvc_to_string (pk);
  gpgrt_assert (strcmp (buf, "") == 0);
  xfree (buf);

  /* Test whether we can delete an entry by name.  */
  err = gpgrt_nvc_add (pk, "Key:", "(3:foo)");
  if (err)
    fail ("gpgrt_nvc_add failed at line %d: %s\n",__LINE__,gpg_strerror (err));
  e = gpgrt_nvc_lookup (pk, "Key:");
  gpgrt_assert (e);
  gpgrt_nvc_delete (pk, NULL, "Kez:");  /* Delete an nonexistent name.  */
  e = gpgrt_nvc_lookup (pk, "Key:");
  gpgrt_assert (e);
  gpgrt_nvc_delete (pk, NULL, "Key:");
  e = gpgrt_nvc_lookup (pk, "Key:");
  gpgrt_assert (!e);

  /* Ditto but now whether it deletes all entries with that name.  We
   * don't use "Key" because that name is special in private key mode.  */
  err = gpgrt_nvc_add (pk, "AKey:", "A-value");
  if (err)
    fail ("gpgrt_nvc_add failed at line %d: %s\n",__LINE__,gpg_strerror (err));

  err = gpgrt_nvc_add (pk, "AKey:", "B-value");
  if (err)
    fail ("gpgrt_nvc_add failed at line %d: %s\n",__LINE__,gpg_strerror (err));

  e = gpgrt_nvc_lookup (pk, "AKey:");
  gpgrt_assert (e);

  gpgrt_nvc_delete (pk, NULL, "AKey:");
  e = gpgrt_nvc_lookup (pk, "AKey:");
  gpgrt_assert (!e);

  err = gpgrt_nvc_set (pk, "Foo:",
                       "A really long value spanning across multiple lines"
                       " that has to be wrapped at a convenient space.");
  if (err)
    fail ("gpgrt_nvc_set failed at line %d: %s\n",__LINE__,gpg_strerror (err));
  buf = nvc_to_string (pk);
  gpgrt_assert (strcmp (buf, "Foo: A really long value spanning across multiple"
		  " lines that has to be\n  wrapped at a convenient space.\n")
	  == 0);
  xfree (buf);

  err = gpgrt_nvc_set (pk, "Foo:",
                       "XA really long value spanning across multiple lines"
                       " that has to be wrapped at a convenient space.");
  if (err)
    fail ("gpgrt_nvc_set failed at line %d: %s\n",__LINE__,gpg_strerror (err));
  buf = nvc_to_string (pk);
  gpgrt_assert (strcmp (buf,
                       "Foo: XA really long value spanning across multiple"
		   " lines that has to\n  be wrapped at a convenient space.\n")
	  == 0);
  xfree (buf);

  err = gpgrt_nvc_set (pk, "Foo:",
                       "XXXXA really long value spanning across multiple lines"
                       " that has to be wrapped at a convenient space.");
  if (err)
    fail ("gpgrt_nvc_set failed at line %d: %s\n",__LINE__,gpg_strerror (err));
  buf = nvc_to_string (pk);
  gpgrt_assert (strcmp (buf,
                  "Foo: XXXXA really long value spanning across multiple"
		  " lines that has\n  to be wrapped at a convenient space.\n")
	  == 0);
  xfree (buf);

  err = gpgrt_nvc_set (pk, "Foo:",
                       "Areallylongvaluespanningacrossmultiplelines"
                       "thathastobewrappedataconvenientspacethatisnotthere.");
  if (err)
    fail ("gpgrt_nvc_set failed at line %d: %s\n",__LINE__,gpg_strerror (err));
  buf = nvc_to_string (pk);
  gpgrt_assert (strcmp (buf,
                  "Foo: Areallylongvaluespanningacrossmultiplelinesthat"
		  "hastobewrappedataco\n nvenientspacethatisnotthere.\n")
	  == 0);
  xfree (buf);
  gpgrt_nvc_release (pk);

  pk = gpgrt_nvc_new (private_key_mode? GPGRT_NVC_PRIVKEY : 0);
  gpgrt_assert (pk);

  err = gpgrt_nvc_set (pk, "Key:", "(hello world)");
  gpgrt_assert (err == 0);
  buf = nvc_to_string (pk);
  gpgrt_assert (strcmp (buf, "Key: (hello world)\n") == 0);
  xfree (buf);
  gpgrt_nvc_release (pk);

  leave_test_function ();
}



static void
parse (const char *fname)
{
  gpg_error_t err;
  estream_t source;
  char *buf;
  gpgrt_nvc_t pk_a, pk_b;
  gpgrt_nve_t e;
  int line;

  source = es_fopen (fname, "rb");
  if (source == NULL)
    {
      perror (fname);
      exit (1);
    }

  if (private_key_mode)
    err = gpgrt_nvc_parse (&pk_a, &line, source, GPGRT_NVC_PRIVKEY);
  else
    err = gpgrt_nvc_parse (&pk_a, &line, source, 0);
  if (err)
    {
      fail ("failed to parse %s line %d: %s\n",
            fname, line, gpg_strerror (err));
      exit (1);
    }

  buf = nvc_to_string (pk_a);
  xfree (buf);

  pk_b = gpgrt_nvc_new (private_key_mode? GPGRT_NVC_PRIVKEY : 0);
  gpgrt_assert (pk_b);

  for (e = gpgrt_nvc_lookup (pk_a, NULL); e; e = gpgrt_nve_next (e, NULL))
    {
      const char *key = NULL;

      if (private_key_mode && !strcasecmp (gpgrt_nve_name (e), "Key:"))
	{
	  key = gpgrt_nve_value (e);
	}

      if (key)
	{
	  err = gpgrt_nve_set (pk_a, e, key);
	  gpgrt_assert (err == 0);
	}
      else
	{
	  err = gpgrt_nvc_add (pk_b, gpgrt_nve_name (e), gpgrt_nve_value (e));
	  gpgrt_assert (err == 0);
	}
    }

  buf = nvc_to_string (pk_b);
  if (verbose)
    fprintf (stdout, "%s", buf);
  xfree (buf);
}



static const char *
my_strusage (int level)
{
  const char *p;

  switch (level)
    {
    case 9: p = "LGPL-2.1-or-later"; break;
    case 11: p = PGM; break;
    case 12: p = PACKAGE_NAME; break;
    case 13: p = PACKAGE_VERSION; break;
    case 14: p = GPGRT_STD_COPYRIGHT_LINE; break;
    case 19: p = "Please report bugs to <https://bugs.gnupg.org>.\n"; break;
    default: p = NULL;
    }
  return p;
}


int
main (int argc, char **argv)
{
  enum { CMD_TEST = 500, CMD_PARSE } command = CMD_TEST;
  gpgrt_opt_t opts[] = {
    ARGPARSE_x  ('v', "verbose", NONE, 0, "Print more diagnostics"),
    ARGPARSE_s_n('d', "debug", "Flyswatter"),
    ARGPARSE_c (CMD_TEST, "test", "Test mode (default)"),
    ARGPARSE_c (CMD_PARSE, "parse", "Parse a file"),
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

        case CMD_TEST:
        case CMD_PARSE:
          command = pargs.r_opt;
          break;

        default : pargs.err = ARGPARSE_PRINT_ERROR; break;
	}
    }
  gpgrt_argparse (NULL, &pargs, NULL);

  if (command == CMD_TEST)
    {
      show ("testing name-value functions\n");

      run_tests ();
      run_modification_tests ();

      show ("again in private key mode\n");
      /* Now again in rivate key mode */
      private_key_mode = 1;
      run_tests ();
      run_modification_tests ();

      show ("again in section mode\n");
      private_key_mode = 0;
      section_mode = 1;
      run_tests ();

      show ("testing name-value functions finished\n");
    }
  else if (command == CMD_PARSE)
    {
      if (argc != 1)
        gpgrt_usage (1);
      parse (*argv);
    }

  return !!errorcount;
}
