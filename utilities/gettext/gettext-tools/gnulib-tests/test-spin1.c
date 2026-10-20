/* Test of spin locks in multithreaded situations.
   Copyright (C) 2005, 2008-2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2020.  */

#include <config.h>

/* Specification.  */
#include "glthread/spin.h"

gl_spinlock_define_initialized (, global_spin_lock)

int
main (void)
{
  /* Check a spin-lock initialized through the constant initializer.  */
  {
    gl_spinlock_lock (global_spin_lock);
    gl_spinlock_unlock (global_spin_lock);
  }

  /* Check a spin-lock initialized through gl_spinlock_init.  */
  {
    gl_spinlock_define (, local_spin_lock)

    gl_spinlock_init (local_spin_lock);

    for (int i = 0; i < 10; i++)
      {
        gl_spinlock_lock (local_spin_lock);
        gl_spinlock_unlock (local_spin_lock);
      }

    gl_spinlock_destroy (local_spin_lock);
  }

  return 0;
}
