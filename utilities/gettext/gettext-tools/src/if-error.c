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

#include <config.h>

/* Specification.  */
#include "if-error.h"

#include <stdarg.h>
#include <stdlib.h>

#include <error.h>
#include "xerror.h"
#include "error-progname.h"
#include "xalloc.h"
#include "xvasprintf.h"
#include "gettext.h"

#define _(str) gettext (str)


void
if_verror (int severity,
           const char *filename, size_t lineno, size_t column,
           bool multiline, const char *format, va_list args)
{
  const char *prefix_tail =
    (severity == IF_SEVERITY_WARNING ? _("warning: ") : _("error: "));

  char *message_text = xvasprintf (format, args);
  if (message_text == NULL)
    message_text = xstrdup (severity == IF_SEVERITY_WARNING
                            ? _("<unformattable warning message>")
                            : _("<unformattable error message>"));

  error_with_progname = false;
  if (multiline)
    {
      char *prefix;
      if (filename != NULL)
        {
          if (lineno != (size_t)(-1))
            {
              if (column != (size_t)(-1))
                prefix = xasprintf ("%s:%ld:%ld: %s",
                                    filename, (long) lineno, (long) column,
                                    prefix_tail);
              else
                prefix = xasprintf ("%s:%ld: %s", filename, (long) lineno,
                                    prefix_tail);
            }
          else
            prefix = xasprintf ("%s: %s", filename, prefix_tail);
        }
      else
        prefix = xasprintf ("%s", prefix_tail);

      if (severity == IF_SEVERITY_WARNING)
        multiline_warning (prefix, message_text);
      else
        multiline_error (prefix, message_text);
    }
  else
    {
      if (filename != NULL)
        {
          if (lineno != (size_t)(-1))
            {
              if (column != (size_t)(-1))
                error (0, 0, "%s:%ld:%ld: %s%s",
                       filename, (long) lineno, (long) column, prefix_tail,
                       message_text);
              else
                error (0, 0, "%s:%ld: %s%s",
                       filename, (long) lineno, prefix_tail, message_text);
            }
          else
            error (0, 0, "%s: %s%s", filename, prefix_tail, message_text);
        }
      else
        error (0, 0, "%s%s", prefix_tail, message_text);
      if (severity == IF_SEVERITY_WARNING)
        --error_message_count;
      free (message_text);
    }
  error_with_progname = true;

  if (severity == IF_SEVERITY_FATAL_ERROR)
    exit (EXIT_FAILURE);
}

void
if_error (int severity,
          const char *filename, size_t lineno, size_t column,
          bool multiline, const char *format, ...)
{
  va_list args;
  va_start (args, format);
  if_verror (severity, filename, lineno, column, multiline, format, args);
  va_end (args);
}
