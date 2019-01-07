/* Error handling during reading and writing of PO files.
   Copyright (C) 2004, 2015-2016 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2004.

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


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* Specification.  */
#include "po-error.h"

#include "error.h"
#include "xerror.h"


void (*po_error) (int status, int errnum,
                  const char *format, ...)
  = error;

void (*po_error_at_line) (int status, int errnum,
                          const char *filename, unsigned int lineno,
                          const char *format, ...)
  = error_at_line;

void (*po_multiline_warning) (char *prefix, char *message)
  = multiline_warning;
void (*po_multiline_error) (char *prefix, char *message)
  = multiline_error;
