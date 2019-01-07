/* Real definitions for extern inline functions in argp.h
   Copyright (C) 1997-1998, 2004, 2009-2016 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Written by Miles Bader <miles@gnu.ai.mit.edu>.

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

#if defined _LIBC || defined HAVE_FEATURES_H
# include <features.h>
#endif

#ifndef __USE_EXTERN_INLINES
# define __USE_EXTERN_INLINES   1
#endif
#ifdef _LIBC
# define ARGP_EI
#else
# define ARGP_EI _GL_EXTERN_INLINE
#endif
#undef __OPTIMIZE__
#define __OPTIMIZE__ 1
#include "argp.h"

/* Add weak aliases.  */
#if _LIBC - 0 && defined (weak_alias)

weak_alias (__argp_usage, argp_usage)
weak_alias (__option_is_short, _option_is_short)
weak_alias (__option_is_end, _option_is_end)

#endif
