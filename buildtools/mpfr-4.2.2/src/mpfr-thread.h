/* MPFR internal header related to thread-local variables.

Copyright 2005-2025 Free Software Foundation, Inc.
Contributed by the Pascaline and Caramba projects, INRIA.

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
along with the GNU MPFR Library; see the file COPYING.LESSER.
If not, see <https://www.gnu.org/licenses/>. */

#ifndef __MPFR_THREAD_H__
#define __MPFR_THREAD_H__

/* Note: MPFR_NEED_THREAD_LOCK shall be defined before including this
   header */

/* Note: Let's define MPFR_THREAD_ATTR even after a #error to make the
   error message more visible (e.g. gcc doesn't immediately stop after
   the #error line and outputs many error messages if MPFR_THREAD_ATTR
   is not defined). But some compilers will just output a message and
   may build MPFR "successfully" (without thread support). */
#ifndef MPFR_THREAD_ATTR
# ifdef MPFR_USE_THREAD_SAFE
#  if defined(_MSC_VER)
#   define MPFR_THREAD_ATTR __declspec( thread )
#  elif defined(MPFR_USE_C11_THREAD_SAFE)
#   define MPFR_THREAD_ATTR _Thread_local
#  else
#   define MPFR_THREAD_ATTR __thread
#  endif
# else
#  define MPFR_THREAD_ATTR
# endif
#endif

/* If MPFR needs a lock mechanism for thread synchro */
#ifdef MPFR_NEED_THREAD_LOCK

/**************************************************************************/
/**************************************************************************/
/*                           ISO C11 version                              */
/**************************************************************************/
/**************************************************************************/

#if defined (MPFR_HAVE_C11_LOCK)
/* NOTE: This version has not been completely tested */

#define MPFR_THREAD_LOCK_METHOD "C11"

#include <threads.h>

#define MPFR_LOCK_DECL(_lock)                   \
  mtx_t _lock;

#define MPFR_LOCK_C(E)                                  \
  do {                                                  \
    if ((E) != thrd_success)                            \
      {                                                 \
        fprintf (stderr, "MPFR lock failure\n");        \
        abort ();                                       \
      }                                                 \
  } while (0)

#define MPFR_LOCK_INIT(_lock)    MPFR_LOCK_C(mtx_init(&(_lock), mtx_plain))
#define MPFR_LOCK_CLEAR(_lock)   do { mtx_destroy(&(_lock)); } while (0)
#define MPFR_LOCK_READ(_lock)    MPFR_LOCK_C(mtx_lock(&(_lock)))
#define MPFR_UNLOCK_READ(_lock)  MPFR_LOCK_C(mtx_unlock(&(_lock)))
#define MPFR_LOCK_WRITE(_lock)   MPFR_LOCK_C(mtx_lock(&(_lock)))
#define MPFR_UNLOCK_WRITE(_lock) MPFR_LOCK_C(mtx_unlock(&(_lock)))

#define MPFR_LOCK_READ2WRITE(_lock) do {} while (0)
#define MPFR_LOCK_WRITE2READ(_lock) do {} while (0)

#define MPFR_ONCE_DECL(_once)                   \
  once_flag _once;

#define MPFR_ONCE_INIT_VALUE ONCE_FLAG_INIT

#define MPFR_ONCE_CALL(_once, _func) do {               \
    call_once(&(_once), (_func));                       \
  } while (0)

#define MPFR_NEED_DEFERRED_INIT 1


/**************************************************************************/
/**************************************************************************/
/*                           POSIX   version                              */
/**************************************************************************/
/**************************************************************************/

#elif defined (HAVE_PTHREAD)

#define MPFR_THREAD_LOCK_METHOD "pthread"

#include <pthread.h>

#define MPFR_LOCK_DECL(_lock)                   \
  pthread_rwlock_t _lock;

#define MPFR_LOCK_C(E)                                  \
  do {                                                  \
    if ((E) != 0)                                       \
      {                                                 \
        fprintf (stderr, "MPFR lock failure\n");        \
        abort ();                                       \
      }                                                 \
  } while (0)

#define MPFR_LOCK_INIT(_lock) MPFR_LOCK_C(pthread_rwlock_init(&(_lock), NULL))

#define MPFR_LOCK_CLEAR(_lock) do {                     \
    pthread_rwlock_destroy(&(_lock));                   \
  } while (0)

#define MPFR_LOCK_READ(_lock)    MPFR_LOCK_C(pthread_rwlock_rdlock(&(_lock)))
#define MPFR_UNLOCK_READ(_lock)  MPFR_LOCK_C(pthread_rwlock_unlock(&(_lock)))
#define MPFR_LOCK_WRITE(_lock)   MPFR_LOCK_C(pthread_rwlock_wrlock(&(_lock)))
#define MPFR_UNLOCK_WRITE(_lock) MPFR_LOCK_C(pthread_rwlock_unlock(&(_lock)))

#define MPFR_LOCK_READ2WRITE(_lock) do {                        \
    MPFR_UNLOCK_READ(_lock);                                    \
    MPFR_LOCK_WRITE(_lock);                                     \
  } while (0)

#define MPFR_LOCK_WRITE2READ(_lock) do {                         \
  MPFR_UNLOCK_WRITE(_lock);                                      \
  MPFR_LOCK_READ(_lock);                                         \
  } while (0)

#define MPFR_ONCE_DECL(_once)                   \
  pthread_once_t _once ;

#define MPFR_ONCE_INIT_VALUE PTHREAD_ONCE_INIT

#define MPFR_ONCE_CALL(_once, _func) \
  MPFR_LOCK_C(pthread_once (&(_once), (_func)))

#define MPFR_NEED_DEFERRED_INIT 1

#else

/* TODO: Win32 */
# error "No thread lock / unsupported OS."

#endif

#else

/**************************************************************************/
/**************************************************************************/
/*                       No lock thread version                           */
/**************************************************************************/
/**************************************************************************/

#define MPFR_LOCK_DECL(_lock)
#define MPFR_LOCK_INIT(_lock)       do {} while (0)
#define MPFR_LOCK_CLEAR(_lock)      do {} while (0)
#define MPFR_LOCK_READ(_lock)       do {} while (0)
#define MPFR_UNLOCK_READ(_lock)     do {} while (0)
#define MPFR_LOCK_WRITE(_lock)      do {} while (0)
#define MPFR_UNLOCK_WRITE(_lock)    do {} while (0)
#define MPFR_LOCK_READ2WRITE(_lock) do {} while (0)
#define MPFR_LOCK_WRITE2READ(_lock) do {} while (0)
#define MPFR_ONCE_INIT_VALUE
#define MPFR_ONCE_DECL(_once)
#define MPFR_ONCE_CALL(_once,_func) do {} while (0)

#endif



/**************************************************************************/
/**************************************************************************/

/**************************************************************************/
/**************************************************************************/

/* If MPFR Needs a way to init data before using them */
#if defined(MPFR_NEED_DEFERRED_INIT)

#if defined(MPFR_HAVE_CONSTRUCTOR_ATTR)

/*********************** Use constructor extension ************************/
#define MPFR_DEFERRED_INIT_MASTER_DECL(_id, _init, _clear)              \
  __attribute__ ((constructor)) static void                             \
  mpfr_init_cache_ ## _id (void)        {                               \
    _init ;                                                             \
  }                                                                     \
  __attribute__ ((destructor)) static void                              \
  mpfr_clean_cache_ ## _id (void)       {                               \
    _clear;                                                             \
  }

#define MPFR_DEFERRED_INIT_CALL(_master) do {} while (0)
#define MPFR_DEFERRED_INIT_SLAVE_DECL()
#define MPFR_DEFERRED_INIT_SLAVE_VALUE(_id)

#else

/**************************** Use once semantic ***************************/
#define MPFR_DEFERRED_INIT_MASTER_DECL(_id, _init, _clear)       \
  static void mpfr_once_ ## _id ## _clear_func (void) {          \
    _clear ;                                                     \
  }                                                              \
  static void mpfr_once_ ## _id ## _init_func (void) {           \
    _init;                                                       \
    atexit(mpfr_once_ ## _id ## _clear_func);                    \
  }

#define MPFR_DEFERRED_INIT_CALL(_master)                \
  MPFR_ONCE_CALL((_master)->once, (_master)->init_once)

#define MPFR_DEFERRED_INIT_SLAVE_DECL()                         \
  MPFR_ONCE_DECL(once)                                          \
  void (*init_once)(void);

#define MPFR_DEFERRED_INIT_SLAVE_VALUE(_id)                     \
  , MPFR_ONCE_INIT_VALUE, mpfr_once_ ## _id ## _init_func

#endif

#else

/* No need */
#define MPFR_DEFERRED_INIT_MASTER_DECL(_id_lock, _init, _clear)
#define MPFR_DEFERRED_INIT_CALL(_master) do {} while (0)
#define MPFR_DEFERRED_INIT_SLAVE_DECL()
#define MPFR_DEFERRED_INIT_SLAVE_VALUE(_id)

#endif

/**************************************************************************/

#endif
