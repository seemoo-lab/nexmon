/* Casefolding mapping for Unicode strings (locale dependent).
   Copyright (C) 2009-2026 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2009.

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

UNIT *
FUNC (const UNIT *s, size_t n, const char *iso639_language,
      uninorm_t nf,
      UNIT *resultbuf, size_t *lengthp)
{
  return U_CT_CASEFOLD (s, n,
                        unicase_empty_prefix_context, unicase_empty_suffix_context,
                        iso639_language,
                        nf,
                        resultbuf, lengthp);
}
