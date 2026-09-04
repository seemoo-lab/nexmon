/* open-po - search for .po file along search path list and open for reading
   Copyright (C) 1995-2026 Free Software Foundation, Inc.

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

/* Written by Ulrich Drepper and Bruno Haible.  */

#include <config.h>

/* Specification.  */
#include "open-catalog.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dir-list.h"
#include "filename.h"
#include "concat-filename.h"
#include "xalloc.h"
#include "xvasprintf.h"
#include "po-xerror.h"
#include "gettext.h"

#define _(str) gettext (str)

/* This macro is used to determine the number of elements in an array.  */
#define SIZEOF(a) (sizeof(a)/sizeof(a[0]))

static FILE *
try_open_catalog_file (const char *input_name, char **real_file_name_p)
{
  if (strcmp (input_name, "-") == 0 || strcmp (input_name, "/dev/stdin") == 0)
    {
      *real_file_name_p = xstrdup (_("<stdin>"));
      return stdin;
    }

  static const char *extension[] = { "", ".po", ".pot", };

  /* We have a real name for the input file.  */
  if (IS_RELATIVE_FILE_NAME (input_name))
    {
      /* For relative file names, look through the directory search list,
         trying the various extensions.  If no directory search list is
         specified, the current directory is used.  */
      const char *dir;
      for (int j = 0; (dir = dir_list_nth (j)) != NULL; ++j)
        for (size_t k = 0; k < SIZEOF (extension); ++k)
          {
            char *file_name =
              xconcatenated_filename (dir, input_name, extension[k]);

            FILE *fp = fopen (file_name, "r");
            if (fp != NULL || errno != ENOENT)
              {
                /* We found the file.  */
                *real_file_name_p = file_name;
                return fp;
              }

            free (file_name);
          }
    }
  else
    {
      /* The name is not relative.  Try the various extensions, but ignore the
         directory search list.  */
      for (size_t k = 0; k < SIZEOF (extension); ++k)
        {
          char *file_name =
            xconcatenated_filename ("", input_name, extension[k]);

          FILE *fp = fopen (file_name, "r");
          if (fp != NULL || errno != ENOENT)
            {
              /* We found the file.  */
              *real_file_name_p = file_name;
              return fp;
            }

          free (file_name);
        }
    }

  /* File does not exist.  */
  *real_file_name_p = xstrdup (input_name);
  errno = ENOENT;
  return NULL;
}

/* Open the input file with the name INPUT_NAME.  The ending .po is added
   if necessary.  If INPUT_NAME is not an absolute file name and the file is
   not found, the list of directories in "dir-list.h" is searched.  The
   file's pathname is returned in *REAL_FILE_NAME_P, for error message
   purposes.  */
FILE *
open_catalog_file (const char *input_name, char **real_file_name_p,
                   bool exit_on_error)
{
  FILE *fp = try_open_catalog_file (input_name, real_file_name_p);

  if (fp == NULL && exit_on_error)
    {
      const char *errno_description = strerror (errno);
      po_xerror (PO_SEVERITY_FATAL_ERROR, NULL, NULL, 0, 0, false,
                 xasprintf ("%s: %s",
                            xasprintf (_("error while opening \"%s\" for reading"),
                                       *real_file_name_p),
                            errno_description));
    }

  return fp;
}
