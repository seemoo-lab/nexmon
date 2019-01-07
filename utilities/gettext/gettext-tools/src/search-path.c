/* Routines for locating data files
   Copyright (C) 2016 Free Software Foundation, Inc.

   This file was written by Daiki Ueno <ueno@gnu.org>, 2016.

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
# include <config.h>
#endif

/* Specification.  */
#include "search-path.h"

#include <stdlib.h>
#include <string.h>

#include "concat-filename.h"
#include "relocatable.h"
#include "xalloc.h"
#include "xmemdup0.h"
#include "xvasprintf.h"

typedef void (* foreach_function_ty) (const char *dir, size_t len, void *data);

struct path_array_ty {
  char **ptr;
  size_t len;
  const char *sub;
};

static void
foreach_elements (const char *dirs, foreach_function_ty function, void *data)
{
  const char *start = dirs;

  /* Count the number of valid elements in GETTEXTDATADIRS.  */
  while (*start != '\0')
    {
      char *end = strchrnul (start, ':');

      /* Skip empty element.  */
      if (start != end)
        function (start, end - start, data);

      if (*end == '\0')
        break;

      start = end + 1;
    }
}

static void
increment (const char *dir, size_t len, void *data)
{
  size_t *count = data;
  (*count)++;
}

static void
fill (const char *dir, size_t len, void *data)
{
  struct path_array_ty *array = data;
  char *base, *name;

  base = xmemdup0 (dir, len);
  if (array->sub == NULL)
    name = base;
  else
    {
      name = xconcatenated_filename (base, array->sub, NULL);
      free (base);
    }

  array->ptr[array->len++] = name;
}

/* Find the standard search path for data files.  Returns a NULL
   terminated list of strings.  The order in the path is as follows:

   1. $GETTEXTDATADIR or GETTEXTDATADIR
   2. $GETTEXTDATADIRS
   3. $XDG_DATA_DIRS, where each element is suffixed with "gettext"
   4. $GETTEXTDATADIR or GETTEXTDATADIR, suffixed with PACKAGE_SUFFIX  */
char **
get_search_path (const char *sub)
{
  const char *gettextdatadir;
  const char *gettextdatadirs;
  struct path_array_ty array;
  char *base, *name;
  size_t count = 2;

  gettextdatadirs = getenv ("GETTEXTDATADIRS");
  if (gettextdatadirs != NULL)
    foreach_elements (gettextdatadirs, increment, &count);

  gettextdatadirs = getenv ("XDG_DATA_DIRS");
  if (gettextdatadirs != NULL)
    foreach_elements (gettextdatadirs, increment, &count);

  array.ptr = XCALLOC (count + 1, char *);
  array.len = 0;

  gettextdatadir = getenv ("GETTEXTDATADIR");
  if (gettextdatadir == NULL || gettextdatadir[0] == '\0')
    /* Make it possible to override the locator file location.  This
       is necessary for running the testsuite before "make
       install".  */
    gettextdatadir = relocate (GETTEXTDATADIR);

  /* Append element from GETTEXTDATADIR.  */
  if (sub == NULL)
    name = xstrdup (gettextdatadir);
  else
    name = xconcatenated_filename (gettextdatadir, sub, NULL);
  array.ptr[array.len++] = name;

  /* Append elements from GETTEXTDATADIRS.  */
  array.sub = sub;
  gettextdatadirs = getenv ("GETTEXTDATADIRS");
  if (gettextdatadirs != NULL)
    foreach_elements (gettextdatadirs, fill, &array);

  /* Append elements from XDG_DATA_DIRS.  Note that each element needs
     to have "gettext" suffix.  */
  if (sub == NULL)
    array.sub = xstrdup ("gettext");
  else
    array.sub = xconcatenated_filename ("gettext", sub, NULL);
  gettextdatadirs = getenv ("XDG_DATA_DIRS");
  if (gettextdatadirs != NULL)
    foreach_elements (gettextdatadirs, fill, &array);
  free (array.sub);

  /* Append version specific directory.  */
  base = xasprintf ("%s%s", gettextdatadir, PACKAGE_SUFFIX);
  if (sub == NULL)
    name = base;
  else
    {
      name = xconcatenated_filename (base, sub, NULL);
      free (base);
    }
  array.ptr[array.len++] = name;

  return array.ptr;
}
