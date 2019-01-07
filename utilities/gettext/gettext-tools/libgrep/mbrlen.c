/* Recognize multibyte character.
   Copyright (C) 1999-2000, 2008-2016 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2008.

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

#include <config.h>

/* Specification.  */
#include <wchar.h>


static mbstate_t internal_state;

size_t
mbrlen (const char *s, size_t n, mbstate_t *ps)
{
  if (ps == NULL)
    ps = &internal_state;
  return mbrtowc (NULL, s, n, ps);
}
