/* Spin locks in multithreaded situations.
   Copyright (C) 2005-2026 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2025.  */

/* This file contains short-duration locking primitives for use with a given
   thread library.

   Spin locks:
     Type:                gl_spinlock_t
     Declaration:         gl_spinlock_define(extern, name)
     Initializer:         gl_spinlock_define_initialized(, name)
     Initialization:      gl_spinlock_init (name);
     Taking the lock:     gl_spinlock_lock (name);
     Releasing the lock:  gl_spinlock_unlock (name);
     De-initialization:   gl_spinlock_destroy (name);
   Equivalent functions with control of error handling:
     Initialization:      glthread_spinlock_init (&name);
     Taking the lock:     glthread_spinlock_lock (&name);
     Releasing the lock:  err = glthread_spinlock_unlock (&name);
     De-initialization:   glthread_spinlock_destroy (&name);
*/


#ifndef _SPINLOCK_H
#define _SPINLOCK_H

#if defined _WIN32 && !defined __CYGWIN__
# include "windows-spin.h"
typedef glwthread_spinlock_t gl_spinlock_t;
# define gl_spinlock_initializer GLWTHREAD_SPIN_INIT
#else
typedef unsigned int gl_spinlock_t;
# define gl_spinlock_initializer 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#define gl_spinlock_define(STORAGECLASS, NAME) \
  STORAGECLASS gl_spinlock_t NAME;
#define gl_spinlock_define_initialized(STORAGECLASS, NAME) \
  STORAGECLASS gl_spinlock_t NAME = gl_spinlock_initializer;
#define gl_spinlock_init(NAME) \
  glthread_spinlock_init (&NAME)
#define gl_spinlock_lock(NAME) \
  glthread_spinlock_lock (&NAME)
#define gl_spinlock_unlock(NAME) \
   do                                        \
     {                                       \
       if (glthread_spinlock_unlock (&NAME)) \
         abort ();                           \
     }                                       \
    while (0)
#define gl_spinlock_destroy(NAME) \
  glthread_spinlock_destroy (&NAME)

#if defined _WIN32 && !defined __CYGWIN__
# define glthread_spinlock_init(lock) \
    glwthread_spin_init (lock)
# define glthread_spinlock_lock(lock) \
    ((void) glwthread_spin_lock (lock))
# define glthread_spinlock_unlock(lock) \
    glwthread_spin_unlock (lock)
# define glthread_spinlock_destroy(lock) \
    ((void) glwthread_spin_destroy (lock))
#else
extern void glthread_spinlock_init (gl_spinlock_t *lock);
extern void glthread_spinlock_lock (gl_spinlock_t *lock);
extern int glthread_spinlock_unlock (gl_spinlock_t *lock);
extern void glthread_spinlock_destroy (gl_spinlock_t *lock);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _SPINLOCK_H */
