/* Test of buffer that accumulates a string by piecewise concatenation.
   Copyright (C) 2021-2026 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2021.  */

#include <config.h>

#include "string-buffer-reversed.h"

#include <errno.h>
#include <string.h>
#include <wchar.h>

#include "macros.h"

static int
my_prependf (struct string_buffer_reversed *buffer, const char *formatstring,
             ...)
{
  va_list args;

  va_start (args, formatstring);
  int ret = sbr_prependvf (buffer, formatstring, args);
  va_end (args);

  return ret;
}

char invalid_format_string_1[] = "%&";
char invalid_format_string_2[] = "%^";

int
main ()
{
  /* Test accumulation.  */
  {
    struct string_buffer_reversed buffer;

    sbr_init (&buffer);
    rw_string_desc_t contents = sbr_dupfree (&buffer);
    ASSERT (sd_is_empty (contents));
    /* Here it is important to distinguish (0, NULL), which stands for an error,
       from (0, non-NULL), which is a successful result.  */
    ASSERT (sd_data (contents) != NULL);
    sd_free (contents);
  }
  {
    struct string_buffer_reversed buffer;

    sbr_init (&buffer);
    sbr_prepend1 (&buffer, '\377');
    sbr_prepend1 (&buffer, 'x');
    char *s = sbr_dupfree_c (&buffer);
    ASSERT (s != NULL && strcmp (s, "x\377") == 0);
    free (s);
  }
  {
    struct string_buffer_reversed buffer;

    sbr_init (&buffer);
    sbr_prepend1 (&buffer, '\377');
    sbr_prepend1 (&buffer, 'x');
    {
      string_desc_t sd = sbr_contents (&buffer);
      ASSERT (sd_length (sd) == 2);
      ASSERT (sd_char_at (sd, 0) == 'x');
      ASSERT (sd_char_at (sd, 1) == '\377');
    }
    sbr_prepend1 (&buffer, '\0');
    sbr_prepend1 (&buffer, 'z');
    {
      string_desc_t sd = sbr_contents (&buffer);
      ASSERT (sd_length (sd) == 4);
      ASSERT (sd_char_at (sd, 0) == 'z');
      ASSERT (sd_char_at (sd, 1) == '\0');
      ASSERT (sd_char_at (sd, 2) == 'x');
      ASSERT (sd_char_at (sd, 3) == '\377');
    }
    char *s = sbr_dupfree_c (&buffer);
    ASSERT (s != NULL && memcmp (s, "z\0x\377\0", 5) == 0);
    free (s);
  }

  /* Test simple string concatenation.  */
  {
    struct string_buffer_reversed buffer;

    sbr_init (&buffer);
    char *s = sbr_dupfree_c (&buffer);
    ASSERT (s != NULL && strcmp (s, "") == 0);
    free (s);
  }

  {
    struct string_buffer_reversed buffer;

    sbr_init (&buffer);
    sbr_prepend_c (&buffer, "efg");
    sbr_prepend_c (&buffer, "");
    sbr_prepend_c (&buffer, "abcd");
    char *s = sbr_dupfree_c (&buffer);
    ASSERT (s != NULL && strcmp (s, "abcdefg") == 0);
    free (s);
  }

  {
    struct string_buffer_reversed buffer;

    sbr_init (&buffer);
    sbr_prepend_c (&buffer, "hij");
    sbr_prepend_desc (&buffer, sd_new_addr (5, "de\0fg"));
    sbr_prepend_c (&buffer, "abc");
    char *s = sbr_dupfree_c (&buffer);
    ASSERT (s != NULL && memcmp (s, "abcde\0fghij", 12) == 0);
    free (s);
  }

  /* Test printf-like formatting.  */
  {
    struct string_buffer_reversed buffer;

    sbr_init (&buffer);
    sbr_prepend_c (&buffer, ">");
    sbr_prependf (&buffer, "%x", 3735928559U);
    sbr_prepend_c (&buffer, "<");
    char *s = sbr_dupfree_c (&buffer);
    ASSERT (s != NULL && strcmp (s, "<deadbeef>") == 0);
    free (s);
  }

  /* Test vprintf-like formatting.  */
  {
    struct string_buffer_reversed buffer;

    sbr_init (&buffer);
    sbr_prepend_c (&buffer, ">");
    my_prependf (&buffer, "%x", 3735928559U);
    sbr_prepend_c (&buffer, "<");
    char *s = sbr_dupfree_c (&buffer);
    ASSERT (s != NULL && strcmp (s, "<deadbeef>") == 0);
    free (s);
  }

  /* Test printf-like formatting failure.
     On all systems except AIX, trying to convert the wide-character 0x76543210
     to a multibyte string (in the "C" locale) fails.
     On all systems, invalid format directives make the vsnzprintf() call
     fail.  */
  {
    struct string_buffer_reversed buffer;
    int ret;

    sbr_init (&buffer);
    sbr_prepend_c (&buffer, ">");

    ret = sbr_prependf (&buffer, "%lc", (wint_t) 0x76543210);
    #if !(defined _AIX || (defined _WIN32 && !defined __CYGWIN__))
    ASSERT (ret < 0);
    ASSERT (errno == EILSEQ);
    #endif

    sbr_prepend_c (&buffer, "|");

    ret = sbr_prependf (&buffer, invalid_format_string_1, 1);
    ASSERT (ret < 0);
    ASSERT (errno == EINVAL);

    sbr_prepend_c (&buffer, "|");

    ret = sbr_prependf (&buffer, invalid_format_string_2, 2);
    ASSERT (ret < 0);
    ASSERT (errno == EINVAL);

    sbr_prepend_c (&buffer, "<");
    char *s = sbr_dupfree_c (&buffer);
    ASSERT (s == NULL);
  }

  return test_exit_status;
}
