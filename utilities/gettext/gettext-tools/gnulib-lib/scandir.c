/* Copyright (C) 1992-1998, 2000, 2002-2003, 2009-2026 Free Software
   Foundation, Inc.
   This file is part of the GNU C Library.

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

#include <config.h>

#include <dirent.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#if _LIBC
# include <bits/libc-lock.h>
#endif

#undef select

#ifndef _D_EXACT_NAMLEN
# define _D_EXACT_NAMLEN(d) strlen ((d)->d_name)
#endif
#ifndef _D_ALLOC_NAMLEN
# ifndef __KLIBC__
#  define _D_ALLOC_NAMLEN(d) (_D_EXACT_NAMLEN (d) + 1)
# else
/* On OS/2 kLIBC, d_name is not the last field of struct dirent. See
   <https://trac.netlabs.org/libc/browser/branches/libc-0.6/src/emx/include/sys/dirent.h#L68>.  */
#  include <stddef.h>
#  define _D_ALLOC_NAMLEN(d) (sizeof (struct dirent) - \
                              offsetof (struct dirent, d_name))
# endif
#endif

#if _LIBC
# ifndef SCANDIR
#  define SCANDIR scandir
#  define READDIR __readdir
#  define DIRENT_TYPE struct dirent
# endif
#else
# define SCANDIR scandir
# define READDIR readdir
# define DIRENT_TYPE struct dirent
# define __opendir opendir
# define __closedir closedir
# define __set_errno(val) errno = (val)

/* The results of opendir() in this file are not used with dirfd and fchdir,
   and we do not leak fds to any single-threaded code that could use stdio,
   therefore save some unnecessary recursion in fchdir.c and opendir_safer.c.
   FIXME - if the kernel ever adds support for multi-thread safety for
   avoiding standard fds, then we should use opendir_safer.  */
# ifdef GNULIB_defined_DIR
#  undef DIR
#  undef opendir
#  undef closedir
#  undef readdir
# else
#  ifdef GNULIB_defined_opendir
#   undef opendir
#  endif
#  ifdef GNULIB_defined_closedir
#   undef closedir
#  endif
# endif
#endif

#ifndef SCANDIR_CANCEL
# define SCANDIR_CANCEL
struct scandir_cancel_struct
{
  DIR *dp;
  void *v;
  size_t cnt;
};

# if _LIBC
static void
cancel_handler (void *arg)
{
  struct scandir_cancel_struct *cp = arg;
  void **v = cp->v;

  for (size_t i = 0; i < cp->cnt; ++i)
    free (v[i]);
  free (v);
  (void) __closedir (cp->dp);
}
# endif
#endif


int
#ifndef __KLIBC__
SCANDIR (const char *dir,
         DIRENT_TYPE ***namelist,
         int (*select) (const DIRENT_TYPE *),
         int (*cmp) (const DIRENT_TYPE **, const DIRENT_TYPE **))
#else
/* On OS/2 kLIBC, scandir() declaration is different from POSIX. See
   <https://trac.netlabs.org/libc/browser/branches/libc-0.6/src/emx/include/dirent.h#L141>.  */
SCANDIR (const char *dir,
         DIRENT_TYPE ***namelist,
         int (*select) (DIRENT_TYPE *),
         int (*cmp) (const void *, const void *))
#endif
{
  DIR *dp = __opendir (dir);

  if (dp == NULL)
    return -1;

  int saved_errno = errno;
  __set_errno (0);

  struct scandir_cancel_struct c;
  c.dp = dp;
  c.v = NULL;
  c.cnt = 0;
#if _LIBC
  __libc_cleanup_push (cancel_handler, &c);
#endif

  DIRENT_TYPE **v = NULL;
  size_t vsize = 0;

  DIRENT_TYPE *d;
  while ((d = READDIR (dp)) != NULL)
    {
      int use_it = select == NULL;

      if (! use_it)
        {
          use_it = select (d);
          /* The select function might have changed errno.  It was
             zero before and it need to be again to make the latter
             tests work.  */
          __set_errno (0);
        }

      if (use_it)
        {
          /* Ignore errors from select or readdir */
          __set_errno (0);

          if (__builtin_expect (c.cnt == vsize, 0))
            {
              DIRENT_TYPE **new;
              if (vsize == 0)
                vsize = 10;
              else
                vsize *= 2;
              new = (DIRENT_TYPE **) realloc (v, vsize * sizeof (*v));
              if (new == NULL)
                break;
              v = new;
              c.v = (void *) v;
            }

          size_t dsize = &d->d_name[_D_ALLOC_NAMLEN (d)] - (char *) d;
          DIRENT_TYPE *vnew = (DIRENT_TYPE *) malloc (dsize);
          if (vnew == NULL)
            break;

          v[c.cnt++] = (DIRENT_TYPE *) memcpy (vnew, d, dsize);
        }
    }

  if (__builtin_expect (errno, 0) != 0)
    {
      saved_errno = errno;

      while (c.cnt > 0)
        free (v[--c.cnt]);
      free (v);
      c.cnt = -1;
    }
  else
    {
      /* Sort the list if we have a comparison function to sort with.  */
      if (cmp != NULL)
        qsort (v, c.cnt, sizeof (*v), (int (*) (const void *, const void *)) cmp);

      *namelist = v;
    }

#if _LIBC
  __libc_cleanup_pop (0);
#endif

  (void) __closedir (dp);
  __set_errno (saved_errno);

  return c.cnt;
}
