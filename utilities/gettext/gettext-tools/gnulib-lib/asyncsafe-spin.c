/* Spin locks for communication between threads and signal handlers.
   Copyright (C) 2020-2026 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2020.  */

#include <config.h>

/* Specification.  */
#include "asyncsafe-spin.h"

#include <stdlib.h>

void
asyncsafe_spin_init (asyncsafe_spinlock_t *lock)
{
  glthread_spinlock_init (lock);
}

void
asyncsafe_spin_lock (asyncsafe_spinlock_t *lock,
                     bool from_signal_handler,
                     const sigset_t *mask, sigset_t *saved_mask)
{
  /* On all platforms, when not running in a signal handler, we need to block
     the signals until the corresponding asyncsafe_spin_unlock() invocation.
     This is needed because if, during that period, a signal occurred and it
     happened to run in the current thread and it were to wait on this spin
     lock, it would hang.
     On platforms other than native Windows, it is useful to do the same
     thing also within a signal handler, since signals may remain enabled
     while a signal handler runs.  It is possible to do this because
     sigprocmask() is safe to call from within a signal handler, see
     POSIX section "Signal Actions"
     <https://pubs.opengroup.org/onlinepubs/9799919799/functions/V2_chap02.html#tag_16_04_03>.
     (In other words, sigprocmask() is atomic, because it is implemented as a
     system call.)
     Whereas on native Windows, sigprocmask() is not atomic, because it
     manipulates global variables.  Therefore in this case, we are *not*
     allowed to call it from within a signal handler.  */
#if defined _WIN32 && !defined __CYGWIN__
  if (!from_signal_handler)
#endif
    sigprocmask (SIG_BLOCK, mask, saved_mask); /* equivalent to pthread_sigmask */

  glthread_spinlock_lock (lock);
}

void
asyncsafe_spin_unlock (asyncsafe_spinlock_t *lock,
                       bool from_signal_handler,
                       const sigset_t *saved_mask)
{
  if (glthread_spinlock_unlock (lock))
    abort ();

#if defined _WIN32 && !defined __CYGWIN__
  if (!from_signal_handler)
#endif
    sigprocmask (SIG_SETMASK, saved_mask, NULL); /* equivalent to pthread_sigmask */
}

void
asyncsafe_spin_destroy (asyncsafe_spinlock_t *lock)
{
  glthread_spinlock_destroy (lock);
}
