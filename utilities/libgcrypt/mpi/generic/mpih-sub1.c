/* mpihelp-add_2.c  -  MPI helper functions
 * Copyright (C) 1994, 1996, 1997, 1998, 2001,
 *               2002 Free Software Foundation, Inc.
 *
 * This file is part of Libgcrypt.
 *
 * Libgcrypt is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Libgcrypt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Note: This code is heavily based on the GNU MP Library.
 *	 Actually it's the same code with only minor changes in the
 *	 way the data is stored; this is to support the abstraction
 *	 of an optional secure memory allocation which may be used
 *	 to avoid revealing of sensitive data due to paging etc.
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include "mpi-internal.h"
#include "longlong.h"

mpi_limb_t
_gcry_mpih_sub_n( mpi_ptr_t res_ptr, mpi_ptr_t s1_ptr,
				  mpi_ptr_t s2_ptr, mpi_size_t size)
{
  mpi_limb_t x, y, cy, borrow;
  mpi_size_t j;

  /* The loop counter and index J goes from -SIZE to -1.  This way
     the loop becomes faster.  */
  j = -size;

  /* Offset the base pointers to compensate for the negative indices.  */
  s1_ptr -= j;
  s2_ptr -= j;
  res_ptr -= j;

  cy = 0;
  do
    {
      y = s2_ptr[j];
      x = s1_ptr[j];
      /* Add previous carry to subtrahend and get out carry from
       * that addition.  */
      add_ssaaaa (cy, y, 0, y, 0, cy);
      /* Main subtract and get out carry from the subtract, combine.  */
      sub_ddmmss( borrow, y, 0, x, 0, y );
      cy -= borrow;
      res_ptr[j] = y;
    }
  while( ++j );

  return cy;
}


