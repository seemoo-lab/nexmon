/* Convert file names between Cygwin syntax and Windows syntax.
   Copyright (C) 2024-2026 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published
   by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2024.  */

#ifndef _WINDOWS_CYGPATH_H
#define _WINDOWS_CYGPATH_H

#ifdef __cplusplus
extern "C" {
#endif

/* On native Windows, this function converts a file name from Cygwin syntax to
   native Windows syntax, like 'cygpath -w' does (if available).
   Returns a freshly allocated file name.
   On the other platforms, it returns a freshly allocated copy of the
   argument file name.  */
extern char *windows_cygpath_w (const char *filename);

#ifdef __cplusplus
}
#endif

#endif /* _WINDOWS_CYGPATH_H */
