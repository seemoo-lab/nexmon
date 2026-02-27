/* Version hook for Argp.
   Copyright (C) 2009-2016 Free Software Foundation, Inc.

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
#include <version-etc.h>
#include <argp.h>
#include <argp-version-etc.h>

static const char *program_canonical_name;
static const char * const *program_authors;

static void
version_etc_hook (FILE *stream, struct argp_state *state)
{
  version_etc_ar (stream, program_canonical_name, PACKAGE_NAME, VERSION,
                  program_authors);
}

void
argp_version_setup (const char *name, const char * const *authors)
{
  argp_program_version_hook = version_etc_hook;
  program_canonical_name = name;
  program_authors = authors;
}
