/* Formatted output to strings.
   Copyright (C) 1999-2026 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include <stdio.h>

#include <stdarg.h>

ptrdiff_t
aszprintf (char **resultp, const char *format, ...)
{
  va_list args;
  va_start (args, format);
  ptrdiff_t result = vaszprintf (resultp, format, args);
  va_end (args);
  return result;
}
