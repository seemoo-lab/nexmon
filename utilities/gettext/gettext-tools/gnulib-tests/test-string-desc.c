/* Test of string descriptors.
   Copyright (C) 2023-2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2023.  */

#include <config.h>

#include "string-desc.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "macros.h"

int
main (int argc, char *argv[])
{
  ASSERT (argc > 1);
  int fd3 = open (argv[1], O_RDWR | O_TRUNC | O_CREAT, 0600);
  ASSERT (fd3 >= 0);

  string_desc_t s0 = sd_new_empty ();
  string_desc_t s1 = sd_from_c ("Hello world!");
  string_desc_t s2 = sd_new_addr (21, "The\0quick\0brown\0\0fox");

  /* Test sd_length.  */
  ASSERT (sd_length (s0) == 0);
  ASSERT (sd_length (s1) == 12);
  ASSERT (sd_length (s2) == 21);

  /* Test sd_char_at.  */
  ASSERT (sd_char_at (s1, 0) == 'H');
  ASSERT (sd_char_at (s1, 11) == '!');
  ASSERT (sd_char_at (s2, 0) == 'T');
  ASSERT (sd_char_at (s2, 1) == 'h');
  ASSERT (sd_char_at (s2, 2) == 'e');
  ASSERT (sd_char_at (s2, 3) == '\0');
  ASSERT (sd_char_at (s2, 4) == 'q');
  ASSERT (sd_char_at (s2, 15) == '\0');
  ASSERT (sd_char_at (s2, 16) == '\0');

  /* Test sd_data.  */
  (void) sd_data (s0);
  ASSERT (memcmp (sd_data (s1), "Hello world!", 12) == 0);
  ASSERT (memcmp (sd_data (s2), "The\0quick\0brown\0\0fox", 21) == 0);

  /* Test sd_is_empty.  */
  ASSERT (sd_is_empty (s0));
  ASSERT (!sd_is_empty (s1));
  ASSERT (!sd_is_empty (s2));

  /* Test sd_startswith.  */
  ASSERT (sd_startswith (s1, s0));
  ASSERT (!sd_startswith (s0, s1));
  ASSERT (!sd_startswith (s1, s2));
  ASSERT (!sd_startswith (s2, s1));
  ASSERT (sd_startswith (s2, sd_from_c ("The")));
  ASSERT (sd_startswith (s2, sd_new_addr (9, "The\0quick")));
  ASSERT (!sd_startswith (s2, sd_new_addr (9, "The\0quirk")));

  /* Test sd_endswith.  */
  ASSERT (sd_endswith (s1, s0));
  ASSERT (!sd_endswith (s0, s1));
  ASSERT (!sd_endswith (s1, s2));
  ASSERT (!sd_endswith (s2, s1));
  ASSERT (!sd_endswith (s2, sd_from_c ("fox")));
  ASSERT (sd_endswith (s2, sd_new_addr (4, "fox")));
  ASSERT (sd_endswith (s2, sd_new_addr (6, "\0\0fox")));
  ASSERT (!sd_endswith (s2, sd_new_addr (5, "\0\0ox")));

  /* Test sd_cmp.  */
  ASSERT (sd_cmp (s0, s0) == 0);
  ASSERT (sd_cmp (s0, s1) < 0);
  ASSERT (sd_cmp (s0, s2) < 0);
  ASSERT (sd_cmp (s1, s0) > 0);
  ASSERT (sd_cmp (s1, s1) == 0);
  ASSERT (sd_cmp (s1, s2) < 0);
  ASSERT (sd_cmp (s2, s0) > 0);
  ASSERT (sd_cmp (s2, s1) > 0);
  ASSERT (sd_cmp (s2, s2) == 0);

  /* Test sd_c_casecmp.  */
  ASSERT (sd_c_casecmp (s0, s0) == 0);
  ASSERT (sd_c_casecmp (s0, s1) < 0);
  ASSERT (sd_c_casecmp (s0, s2) < 0);
  ASSERT (sd_c_casecmp (s1, s0) > 0);
  ASSERT (sd_c_casecmp (s1, s1) == 0);
  ASSERT (sd_c_casecmp (s1, s2) < 0);
  ASSERT (sd_c_casecmp (s2, s0) > 0);
  ASSERT (sd_c_casecmp (s2, s1) > 0);
  ASSERT (sd_c_casecmp (s2, s2) == 0);
  ASSERT (sd_c_casecmp (sd_from_c ("acab"), sd_from_c ("AcAB")) == 0);
  ASSERT (sd_c_casecmp (sd_from_c ("AcAB"), sd_from_c ("acab")) == 0);
  ASSERT (sd_c_casecmp (sd_from_c ("aca"), sd_from_c ("AcAB")) < 0);
  ASSERT (sd_c_casecmp (sd_from_c ("AcAB"), sd_from_c ("aca")) > 0);
  ASSERT (sd_c_casecmp (sd_from_c ("aca"), sd_from_c ("Aca\377")) < 0);
  ASSERT (sd_c_casecmp (sd_from_c ("Aca\377"), sd_from_c ("aca")) > 0);

  /* Test sd_index.  */
  ASSERT (sd_index (s0, 'o') == -1);
  ASSERT (sd_index (s2, 'o') == 12);

  /* Test sd_last_index.  */
  ASSERT (sd_last_index (s0, 'o') == -1);
  ASSERT (sd_last_index (s2, 'o') == 18);

  /* Test sd_contains.  */
  ASSERT (sd_contains (s0, sd_from_c ("ll")) == -1);
  ASSERT (sd_contains (s1, sd_from_c ("ll")) == 2);
  ASSERT (sd_contains (s1, sd_new_addr (1, "")) == -1);
  ASSERT (sd_contains (s2, sd_new_addr (1, "")) == 3);
  ASSERT (sd_contains (s1, sd_new_addr (2, "\0")) == -1);
  ASSERT (sd_contains (s2, sd_new_addr (2, "\0")) == 15);

  /* Test sd_substring.  */
  ASSERT (sd_cmp (sd_substring (s1, 2, 5),
                  sd_from_c ("llo")) == 0);

  /* Test sd_write.  */
  ASSERT (sd_write (fd3, s0) == 0);
  ASSERT (sd_write (fd3, s1) == 0);
  ASSERT (sd_write (fd3, s2) == 0);

  /* Test sd_fwrite.  */
  ASSERT (sd_fwrite (stdout, s0) == 0);
  ASSERT (sd_fwrite (stdout, s1) == 0);
  ASSERT (sd_fwrite (stdout, s2) == 0);

  /* Test sd_new, sd_set_char_at, sd_fill.  */
  rw_string_desc_t s4;
  ASSERT (sd_new (&s4, 5) == 0);
  sd_set_char_at (s4, 0, 'H');
  sd_set_char_at (s4, 4, 'o');
  sd_set_char_at (s4, 1, 'e');
  sd_fill (s4, 2, 4, 'l');
  ASSERT (sd_length (s4) == 5);
  ASSERT (sd_startswith (s1, s4));

  /* Test sd_new_filled, sd_set_char_at.  */
  rw_string_desc_t s5;
  ASSERT (sd_new_filled (&s5, 5, 'l') == 0);
  sd_set_char_at (s5, 0, 'H');
  sd_set_char_at (s5, 4, 'o');
  sd_set_char_at (s5, 1, 'e');
  ASSERT (sd_length (s5) == 5);
  ASSERT (sd_startswith (s1, s5));

  /* Test sd_equals.  */
  ASSERT (!sd_equals (s1, s5));
  ASSERT (sd_equals (s4, s5));

  /* Test sd_copy, sd_free.  */
  {
    rw_string_desc_t s6;
    ASSERT (sd_copy (&s6, s0) == 0);
    ASSERT (sd_is_empty (s6));
    sd_free (s6);
  }
  {
    rw_string_desc_t s6;
    ASSERT (sd_copy (&s6, s2) == 0);
    ASSERT (sd_equals (s6, s2));
    sd_free (s6);
  }

  /* Test sd_overwrite.  */
  {
    rw_string_desc_t s7;
    ASSERT (sd_copy (&s7, s2) == 0);
    sd_overwrite (s7, 4, s1);
    ASSERT (sd_equals (s7, sd_new_addr (21, "The\0Hello world!\0fox")));
  }

  /* Test sd_concat.  */
  {
    rw_string_desc_t s8;
    ASSERT (sd_concat (&s8, 3, sd_new_addr (10, "The\0quick"),
                               sd_new_addr (7, "brown\0"),
                               sd_new_addr (4, "fox"),
                               sd_new_addr (7, "unused")) == 0);
    ASSERT (sd_equals (s8, s2));
    sd_free (s8);
  }

  /* Test sd_c.  */
  {
    char *ptr = sd_c (s2);
    ASSERT (ptr != NULL);
    ASSERT (memcmp (ptr, "The\0quick\0brown\0\0fox\0", 22) == 0);
    free (ptr);
  }

  close (fd3);

  return test_exit_status;
}
