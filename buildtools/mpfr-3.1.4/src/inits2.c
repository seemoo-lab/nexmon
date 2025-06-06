/* mpfr_inits2 -- initialize several floating-point numbers with given
   precision

Copyright 2003-2004, 2006-2016 Free Software Foundation, Inc.
Contributed by the AriC and Caramba projects, INRIA.

This file is part of the GNU MPFR Library.

The GNU MPFR Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

The GNU MPFR Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the GNU MPFR Library; see the file COPYING.LESSER.  If not, see
http://www.gnu.org/licenses/ or write to the Free Software Foundation, Inc.,
51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA. */

#ifdef HAVE_CONFIG_H
#undef HAVE_STDARG
#include "config.h"     /* for a build within gmp */
#endif

#if HAVE_STDARG
# include <stdarg.h>
#else
# include <varargs.h>
#endif

#include "mpfr-impl.h"

/*
 * Contrary to mpfr_init2, mpfr_prec_t p is the first argument
 */

/* Explicit support for K&R compiler */
void
#if HAVE_STDARG
mpfr_inits2 (mpfr_prec_t p, mpfr_ptr x, ...)
#else
mpfr_inits2 (va_alist)
 va_dcl
#endif
{
  va_list arg;
#if HAVE_STDARG
  va_start (arg, x);
#else
  mpfr_prec_t p;
  mpfr_ptr x;
  va_start(arg);
  p =  va_arg (arg, mpfr_prec_t);
  x =  va_arg (arg, mpfr_ptr);
#endif
  while (x != 0)
    {
      mpfr_init2 (x, p);
      x = (mpfr_ptr) va_arg (arg, mpfr_ptr);
    }
  va_end (arg);
}
