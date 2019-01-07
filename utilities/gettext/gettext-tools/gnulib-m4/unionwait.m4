# unionwait.m4 serial 1 (gettext-0.11)
dnl Copyright (C) 1993-2002, 2015-2016 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Taken from GNU make 3.79.1.

AC_DEFUN([gt_UNION_WAIT],
[
AC_CHECK_FUNCS(waitpid)
AC_MSG_CHECKING(for union wait)
AC_CACHE_VAL(gt_cv_union_wait, [dnl
AC_TRY_LINK([#include <sys/types.h>
#include <sys/wait.h>],
            [union wait status; int pid; pid = wait (&status);
#ifdef WEXITSTATUS
/* Some POSIXoid systems have both the new-style macros and the old
   union wait type, and they do not work together.  If union wait
   conflicts with WEXITSTATUS et al, we don't want to use it at all.  */
if (WEXITSTATUS (status) != 0) pid = -1;
#ifdef WTERMSIG
/* If we have WEXITSTATUS and WTERMSIG, just use them on ints.  */
-- blow chunks here --
#endif
#endif
#ifdef HAVE_WAITPID
/* Make sure union wait works with waitpid.  */
pid = waitpid (-1, &status, 0);
#endif
],
            [gt_cv_union_wait=yes], [gt_cv_union_wait=no])])
if test "$gt_cv_union_wait" = yes; then
  AC_DEFINE(HAVE_UNION_WAIT, 1,
            [Define if <sys/wait.h> defines the 'union wait' type.])
fi
AC_MSG_RESULT($gt_cv_union_wait)
])
