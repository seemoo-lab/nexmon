/* Test for the memory-ostream API.
   Copyright (C) 2019-2026 Free Software Foundation, Inc.

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

#include "memory-ostream.h"

#include <stdlib.h>
#include <string.h>

int
main ()
{
  memory_ostream_t stream = memory_ostream_create ();

  ostream_write_str (stream, "foo");
  ostream_printf (stream, "%d%d", 73, 55);
  ostream_write_str (stream, "\n");

  {
    const void *buf;
    size_t buflen;
    memory_ostream_contents (stream, &buf, &buflen);

    if (!(buflen == 8))
      exit (2);
    if (!(memcmp (buf, "foo7355\n", 8) == 0))
      exit (3);

    ostream_free (stream);
  }

  return 0;
}
