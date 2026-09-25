# stpncpy.m4
# serial 23
dnl Copyright (C) 2002-2007, 2009-2026 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl This file is offered as-is, without any warranty.

AC_DEFUN([gl_FUNC_STPNCPY],
[
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles

  dnl Persuade glibc <string.h> to declare stpncpy().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  dnl The stpncpy() declaration in lib/string.in.h uses 'restrict'.
  AC_REQUIRE([AC_C_RESTRICT])

  AC_REQUIRE([gl_STRING_H_DEFAULTS])

  dnl Check for prerequisites for memory fence checks.
  gl_FUNC_MMAP_ANON
  AC_CHECK_HEADERS_ONCE([sys/mman.h])
  AC_CHECK_FUNCS_ONCE([mprotect])

  dnl Both glibc and AIX (4.3.3, 5.1) have an stpncpy() function
  dnl declared in <string.h>. Its side effects are the same as those
  dnl of strncpy():
  dnl      stpncpy (dest, src, n)
  dnl overwrites dest[0..n-1], min(strlen(src),n) bytes coming from src,
  dnl and the remaining bytes being NULs.  However, the return value is
  dnl   in glibc:   dest + min(strlen(src),n)
  dnl   in AIX:     dest + max(0,n-1)
  dnl Only the glibc return value is useful in practice.
  dnl
  dnl Also detect bug in FreeBSD 15.0 on x86_64:
  dnl stpncpy should not dereference more than n bytes, but always dereferences
  dnl n+1 bytes if the first n bytes don't contain a NUL byte.
  dnl Assume that stpncpy works on platforms that lack mprotect.

  AC_CHECK_DECLS_ONCE([stpncpy])
  gl_CHECK_FUNCS_ANDROID([stpncpy], [[#include <string.h>]])
  if test $ac_cv_func_stpncpy = yes; then
    AC_CACHE_CHECK([for working stpncpy], [gl_cv_func_stpncpy], [
      AC_RUN_IFELSE(
        [AC_LANG_SOURCE([[
#include <stdlib.h>
#include <string.h> /* for strcpy */
/* The stpncpy prototype is missing in <string.h> on AIX 4.  */
#if !HAVE_DECL_STPNCPY
extern
# ifdef __cplusplus
"C"
# endif
char *stpncpy (char *dest, const char *src, size_t n);
#endif
#if HAVE_SYS_MMAN_H
# include <fcntl.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/mman.h>
#endif
int main ()
{
  int result = 0;
  const char *src = "Hello";
  char dest[10];
  /* AIX 4.3.3 and AIX 5.1 stpncpy() returns dest+1 here.  */
  {
    strcpy (dest, "\377\377\377\377\377\377");
    if (stpncpy (dest, src, 2) != dest + 2)
      result |= 1;
  }
  /* AIX 4.3.3 and AIX 5.1 stpncpy() returns dest+4 here.  */
  {
    strcpy (dest, "\377\377\377\377\377\377");
    if (stpncpy (dest, src, 5) != dest + 5)
      result |= 2;
  }
  /* AIX 4.3.3 and AIX 5.1 stpncpy() returns dest+6 here.  */
  {
    strcpy (dest, "\377\377\377\377\377\377");
    if (stpncpy (dest, src, 7) != dest + 5)
      result |= 4;
  }
  /* FreeBSD 15.0/x86_64 crashes here.  */
  {
    char *fence = NULL;
#if HAVE_SYS_MMAN_H && HAVE_MPROTECT
    {
      long int pagesize = sysconf (_SC_PAGESIZE);
      char *two_pages =
        (char *) mmap (NULL, 2 * pagesize, PROT_READ | PROT_WRITE,
                       MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
      if (two_pages != (char *)(-1)
          && mprotect (two_pages + pagesize, pagesize, PROT_NONE) == 0)
        fence = two_pages + pagesize;
    }
#endif
    if (fence)
      {
        char dest[8];

        dest[0] = 'a';
        dest[1] = 'b';
        dest[2] = 'c';
        dest[3] = 'd';
        dest[4] = 'e';
        dest[5] = 'f';
        dest[6] = 'g';

        *(fence - 3) = '7';
        *(fence - 2) = '2';
        *(fence - 1) = '9';

        if (stpncpy (dest + 1, fence - 3, 3) != dest + 4)
          result |= 8;
        if (dest[0] != 'a')
          result |= 16;
        if (dest[1] != '7' || dest[2] != '2' || dest[3] != '9')
          result |= 32;
        if (dest[4] != 'e')
          result |= 16;
      }
  }
  return result;
}
]])],
        [gl_cv_func_stpncpy=yes],
        [gl_cv_func_stpncpy=no],
        [dnl Guess yes on glibc systems and musl systems, no on FreeBSD.
         AC_EGREP_CPP([Thanks for using GNU], [
#include <features.h>
#ifdef __GNU_LIBRARY__
  Thanks for using GNU
#endif
],         [gl_cv_func_stpncpy="guessing yes"],
           [case "$host_os" in
              *-musl* | midipix*)    gl_cv_func_stpncpy="guessing yes" ;;
              freebsd* | dragonfly*) gl_cv_func_stpncpy="guessing no" ;;
              *)                     gl_cv_func_stpncpy="$gl_cross_guess_normal" ;;
            esac
           ])
        ])
    ])
    case "$gl_cv_func_stpncpy" in
      *yes)
        AC_DEFINE([HAVE_STPNCPY], [1],
          [Define if you have the stpncpy() function and it works.])
        ;;
      *)
        REPLACE_STPNCPY=1
        ;;
    esac
  else
    HAVE_STPNCPY=0
    case "$gl_cv_onwards_func_stpncpy" in
      future*) REPLACE_STPNCPY=1 ;;
    esac
  fi
])

# Prerequisites of lib/stpncpy.c.
AC_DEFUN([gl_PREREQ_STPNCPY], [
  :
])
