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

#include "xstring-desc.h"

#include <stdlib.h>
#include <string.h>

#include "macros.h"

int
main (void)
{
  string_desc_t s0 = sd_new_empty ();
  string_desc_t s1 = sd_from_c ("Hello world!");
  string_desc_t s2 = sd_new_addr (21, "The\0quick\0brown\0\0fox");

  /* Test xsd_new.  */
  rw_string_desc_t s4 = xsd_new (5);
  sd_set_char_at (s4, 0, 'H');
  sd_set_char_at (s4, 4, 'o');
  sd_set_char_at (s4, 1, 'e');
  sd_fill (s4, 2, 4, 'l');
  ASSERT (sd_length (s4) == 5);
  ASSERT (sd_startswith (s1, s4));

  /* Test xsd_new_filled.  */
  rw_string_desc_t s5 = xsd_new_filled (5, 'l');
  sd_set_char_at (s5, 0, 'H');
  sd_set_char_at (s5, 4, 'o');
  sd_set_char_at (s5, 1, 'e');
  ASSERT (sd_length (s5) == 5);
  ASSERT (sd_startswith (s1, s5));

  /* Test xsd_copy.  */
  {
    rw_string_desc_t s6 = xsd_copy (s0);
    ASSERT (sd_is_empty (s6));
    sd_free (s6);
  }
  {
    rw_string_desc_t s6 = xsd_copy (s2);
    ASSERT (sd_equals (s6, s2));
    sd_free (s6);
  }

  /* Test xsd_concat.  */
  {
    rw_string_desc_t s8 =
      xsd_concat (3, sd_new_addr (10, "The\0quick"),
                     sd_new_addr (7, "brown\0"),
                     sd_new_addr (4, "fox"),
                     sd_new_addr (7, "unused"));
    ASSERT (sd_equals (s8, s2));
    sd_free (s8);
  }

  /* Test xsd_c.  */
  {
    char *ptr = xsd_c (s2);
    ASSERT (ptr != NULL);
    ASSERT (memcmp (ptr, "The\0quick\0brown\0\0fox\0", 22) == 0);
    free (ptr);
  }

  return test_exit_status;
}
