/* Full and short program names for argp module
   Copyright (C) 2005, 2009-2026 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stddef.h>

#ifndef HAVE_PROGRAM_INVOCATION_SHORT_NAME
char *program_invocation_short_name = NULL;
#endif
#ifndef HAVE_PROGRAM_INVOCATION_NAME
char *program_invocation_name = NULL;
#endif

#if (defined HAVE_PROGRAM_INVOCATION_SHORT_NAME \
     && defined HAVE_PROGRAM_INVOCATION_NAME)
/* This declaration is solely to ensure that after preprocessing
   this file is never empty.  */
typedef int dummy;
#endif
