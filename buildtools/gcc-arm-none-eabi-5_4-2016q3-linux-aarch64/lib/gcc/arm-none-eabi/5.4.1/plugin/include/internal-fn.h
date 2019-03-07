/* Internal functions.
   Copyright (C) 2011-2015 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#ifndef GCC_INTERNAL_FN_H
#define GCC_INTERNAL_FN_H

#include "coretypes.h"

/* Initialize internal function tables.  */

extern void init_internal_fns ();

/* Return the name of internal function FN.  The name is only meaningful
   for dumps; it has no linkage.  */

extern const char *const internal_fn_name_array[];

static inline const char *
internal_fn_name (enum internal_fn fn)
{
  return internal_fn_name_array[(int) fn];
}

/* Return the ECF_* flags for function FN.  */

extern const int internal_fn_flags_array[];

static inline int
internal_fn_flags (enum internal_fn fn)
{
  return internal_fn_flags_array[(int) fn];
}

/* Return fnspec for function FN.  */

extern GTY(()) const_tree internal_fn_fnspec_array[IFN_LAST + 1];

static inline const_tree
internal_fn_fnspec (enum internal_fn fn)
{
  return internal_fn_fnspec_array[(int) fn];
}

extern void expand_internal_call (gcall *);

#endif
