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

#ifndef _ARGP_VERSION_ETC_H
#define _ARGP_VERSION_ETC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Setup standard display of the version information for the '--version'
   option.  NAME is the canonical program name, and AUTHORS is a NULL-
   terminated array of author names. At least one author name must be
   given.

   If NAME is NULL, the package name (as given by the PACKAGE macro)
   is assumed to be the name of the program.

   This function is intended to be called before argp_parse().
*/
extern void argp_version_setup (const char *name, const char * const *authors);

#ifdef __cplusplus
}
#endif

#endif /* _ARGP_VERSION_ETC_H */
