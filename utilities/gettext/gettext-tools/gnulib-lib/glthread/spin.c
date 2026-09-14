/* Spin locks in multithreaded situations.
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
#include "glthread/spin.h"

#include <errno.h>
#if defined _AIX
# include <sys/atomic_op.h>
#endif
#if 0x590 <= __SUNPRO_C && __STDC__
# define asm __asm
#endif

#if defined _WIN32 && !defined __CYGWIN__
/* Use Windows threads.  */

/* All definitions are inline in glthread/spin.h.  */

#else

/* We don't use semaphores (although sem_post() is allowed in signal handlers),
   because it would require to link with -lrt on HP-UX 11, Solaris 10, and also
   because on macOS only named semaphores work.

   We don't use the C11 <stdatomic.h> (available in GCC >= 4.9) because it would
   require to link with -latomic.  */

# if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7) \
      || __clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 1)) \
     && !defined __ibmxl__
/* Use GCC built-ins (available in GCC >= 4.7 and clang >= 3.1) that operate on
   the first byte of the lock.
   Documentation:
   <https://gcc.gnu.org/onlinedocs/gcc-4.7.0/gcc/_005f_005fatomic-Builtins.html>
 */

#  if 1
/* An implementation that verifies the unlocks.  */

void
glthread_spinlock_init (gl_spinlock_t *lock)
{
  __atomic_store_n (lock, 0, __ATOMIC_SEQ_CST);
}

void
glthread_spinlock_lock (gl_spinlock_t *lock)
{
  /* Wait until *lock becomes 0, then replace it with 1.  */
  gl_spinlock_t zero;
  while (!(zero = 0,
           __atomic_compare_exchange_n (lock, &zero, 1, false,
                                        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)))
    ;
}

int
glthread_spinlock_unlock (gl_spinlock_t *lock)
{
  /* If *lock is 1, then replace it with 0.  */
  gl_spinlock_t one = 1;
  if (!__atomic_compare_exchange_n (lock, &one, 0, false,
                                    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
    return EINVAL;
  return 0;
}

#  else
/* An implementation that is a little bit more optimized, but does not verify
   the unlocks.  */

void
glthread_spinlock_init (gl_spinlock_t *lock)
{
  __atomic_clear (lock, __ATOMIC_SEQ_CST);
}

void
glthread_spinlock_lock (gl_spinlock_t *lock)
{
  while (__atomic_test_and_set (lock, __ATOMIC_SEQ_CST))
    ;
}

int
glthread_spinlock_unlock (gl_spinlock_t *lock)
{
  __atomic_clear (lock, __ATOMIC_SEQ_CST);
  return 0;
}

#  endif

# elif (((__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)) \
         || __clang_major__ >= 3) \
        && HAVE_ATOMIC_COMPARE_AND_SWAP_GCC41)
/* Use GCC built-ins (available on many platforms with GCC >= 4.1 or
   clang >= 3.0).
   Documentation:
   <https://gcc.gnu.org/onlinedocs/gcc-4.1.2/gcc/Atomic-Builtins.html>  */

void
glthread_spinlock_init (gl_spinlock_t *lock)
{
  volatile unsigned int *vp = lock;
  *vp = 0;
  __sync_synchronize ();
}

void
glthread_spinlock_lock (gl_spinlock_t *lock)
{
  /* Wait until *lock becomes 0, then replace it with 1.  */
  while (__sync_val_compare_and_swap (lock, 0, 1) != 0)
    ;
}

int
glthread_spinlock_unlock (gl_spinlock_t *lock)
{
  /* If *lock is 1, then replace it with 0.  */
  if (__sync_val_compare_and_swap (lock, 1, 0) != 1)
    return EINVAL;
  return 0;
}

# elif defined _AIX
/* AIX */

void
glthread_spinlock_init (gl_spinlock_t *lock)
{
  atomic_p vp = (int *) lock;
  _clear_lock (vp, 0);
}

void
glthread_spinlock_lock (gl_spinlock_t *lock)
{
  atomic_p vp = (int *) lock;
  while (_check_lock (vp, 0, 1))
    ;
}

int
glthread_spinlock_unlock (gl_spinlock_t *lock)
{
  atomic_p vp = (int *) lock;
  if (_check_lock (vp, 1, 0))
    return EINVAL;
  return 0;
}

# elif ((defined __GNUC__ || defined __clang__ || defined __SUNPRO_C) && (defined __sparc || defined __i386 || defined __x86_64__)) || (defined __TINYC__ && (defined __i386 || defined __x86_64__))
/* For older versions of GCC or clang, use inline assembly.
   GCC, clang, and the Oracle Studio C 12 compiler understand GCC's extended
   asm syntax, but the plain Oracle Studio C 11 compiler understands only
   simple asm.  */
/* An implementation that verifies the unlocks.  */

static void
memory_barrier (void)
{
#  if defined __GNUC__ || defined __clang__ || __SUNPRO_C >= 0x590 || defined __TINYC__
#   if defined __i386 || defined __x86_64__
#    if defined __TINYC__ && defined __i386
  /* Cannot use the SSE instruction "mfence" with this compiler.  */
  asm volatile ("lock orl $0,(%esp)");
#    else
  asm volatile ("mfence");
#    endif
#   endif
#   if defined __sparc
  asm volatile ("membar 2");
#   endif
#  else
#   if defined __i386 || defined __x86_64__
  asm ("mfence");
#   endif
#   if defined __sparc
  asm ("membar 2");
#   endif
#  endif
}

/* Store NEWVAL in *VP if the old value *VP is == CMP.
   Return the old value.  */
static unsigned int
atomic_compare_and_swap (volatile unsigned int *vp, unsigned int cmp,
                         unsigned int newval)
{
#  if defined __GNUC__ || defined __clang__ || __SUNPRO_C >= 0x590 || defined __TINYC__
  unsigned int oldval;
#   if defined __i386 || defined __x86_64__
  asm volatile (" lock\n cmpxchgl %3,(%1)"
                : "=a" (oldval) : "r" (vp), "a" (cmp), "r" (newval) : "memory");
#   endif
#   if defined __sparc
  asm volatile (" cas [%1],%2,%3\n"
                " mov %3,%0"
                : "=r" (oldval) : "r" (vp), "r" (cmp), "r" (newval) : "memory");
#   endif
  return oldval;
#  else /* __SUNPRO_C */
#   if defined __x86_64__
  asm (" movl %esi,%eax\n"
       " lock\n cmpxchgl %edx,(%rdi)");
#   elif defined __i386
  asm (" movl 16(%ebp),%ecx\n"
       " movl 12(%ebp),%eax\n"
       " movl 8(%ebp),%edx\n"
       " lock\n cmpxchgl %ecx,(%edx)");
#   endif
#   if defined __sparc
  asm (" cas [%i0],%i1,%i2\n"
       " mov %i2,%i0");
#   endif
#  endif
}

void
glthread_spinlock_init (gl_spinlock_t *lock)
{
  volatile unsigned int *vp = lock;
  *vp = 0;
  memory_barrier ();
}

void
glthread_spinlock_lock (gl_spinlock_t *lock)
{
  volatile unsigned int *vp = lock;
  while (atomic_compare_and_swap (vp, 0, 1) != 0)
    ;
}

int
glthread_spinlock_unlock (gl_spinlock_t *lock)
{
  volatile unsigned int *vp = lock;
  if (atomic_compare_and_swap (vp, 1, 0) != 1)
    return EINVAL;
  return 0;
}

# else
/* Fallback code.  It has some race conditions.  */

void
glthread_spinlock_init (gl_spinlock_t *lock)
{
  volatile unsigned int *vp = lock;
  *vp = 0;
}

void
glthread_spinlock_lock (gl_spinlock_t *lock)
{
  volatile unsigned int *vp = lock;
  while (*vp)
    ;
  *vp = 1;
}

int
glthread_spinlock_unlock (gl_spinlock_t *lock)
{
  volatile unsigned int *vp = lock;
  *vp = 0;
  return 0;
}

# endif

void
glthread_spinlock_destroy (gl_spinlock_t *lock)
{
}

#endif
