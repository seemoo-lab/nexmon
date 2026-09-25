/* Error handling during reading of input files.
   Copyright (C) 2023-2026 Free Software Foundation, Inc.

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

#ifndef _IF_ERROR_H
#define _IF_ERROR_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


/* A higher-level error printing facility than the one in error.h / xerror.h
   and error-progname.h.  */

#define IF_SEVERITY_WARNING     0 /* just a warning, tell the user */
#define IF_SEVERITY_ERROR       1 /* an error, the operation cannot complete */
#define IF_SEVERITY_FATAL_ERROR 2 /* an error, the operation must be aborted */

/* Signal a problem of the given severity.
   FILENAME + LINENO indicate where the problem occurred.
   If FILENAME is NULL, FILENAME and LINENO and COLUMN are ignored.
   If LINENO is (size_t)(-1), LINENO and COLUMN are ignored.
   If COLUMN is (size_t)(-1), it is ignored.
   FORMAT and the following format string arguments are the problem description
   (if MULTILINE is true, multiple lines of text, each terminated with a
   newline, otherwise usually a single line).
   Does not return if SEVERITY is IF_SEVERITY_FATAL_ERROR.  */
extern void if_error (int severity,
                      const char *filename, size_t lineno, size_t column,
                      bool multiline, const char *format, ...)
#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)) || defined __clang__
     __attribute__ ((__format__ (__printf__, 6, 7)))
#endif
     ;
extern void if_verror (int severity,
                       const char *filename, size_t lineno, size_t column,
                       bool multiline, const char *format, va_list args)
#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)) || defined __clang__
     __attribute__ ((__format__ (__printf__, 6, 0)))
#endif
     ;


#ifdef __cplusplus
}
#endif

#endif /* _IF_ERROR_H */
