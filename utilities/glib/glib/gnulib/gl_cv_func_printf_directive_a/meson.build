# Copyright (C) 2002-2004, 2006-2018 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Test whether the *printf family of functions supports the 'a' and 'A'
# conversion specifier for hexadecimal output of floating-point numbers.
# (ISO C99, POSIX:2001)
# Result is gl_cv_func_printf_directive_a.

printf_directive_a_test = '''
#include <stdio.h>
#include <string.h>
static char buf[100];
static double zero = 0.0;
int main ()
{
  int result = 0;
  if (sprintf (buf, "%a %d", 3.1416015625, 33) < 0
      || (strcmp (buf, "0x1.922p+1 33") != 0
          && strcmp (buf, "0x3.244p+0 33") != 0
          && strcmp (buf, "0x6.488p-1 33") != 0
          && strcmp (buf, "0xc.91p-2 33") != 0))
    result |= 1;
  if (sprintf (buf, "%A %d", -3.1416015625, 33) < 0
      || (strcmp (buf, "-0X1.922P+1 33") != 0
          && strcmp (buf, "-0X3.244P+0 33") != 0
          && strcmp (buf, "-0X6.488P-1 33") != 0
          && strcmp (buf, "-0XC.91P-2 33") != 0))
    result |= 2;
  /* This catches a FreeBSD 6.1 bug: it doesn't round.  */
  if (sprintf (buf, "%.2a %d", 1.51, 33) < 0
      || (strcmp (buf, "0x1.83p+0 33") != 0
          && strcmp (buf, "0x3.05p-1 33") != 0
          && strcmp (buf, "0x6.0ap-2 33") != 0
          && strcmp (buf, "0xc.14p-3 33") != 0))
    result |= 4;
  /* This catches a Mac OS X 10.12.4 (Darwin 16.5) bug: it doesn't round.  */
  if (sprintf (buf, "%.0a %d", 1.51, 33) < 0
      || (strcmp (buf, "0x2p+0 33") != 0
          && strcmp (buf, "0x3p-1 33") != 0
          && strcmp (buf, "0x6p-2 33") != 0
          && strcmp (buf, "0xcp-3 33") != 0))
    result |= 4;
  /* This catches a FreeBSD 6.1 bug.  See
     <https://lists.gnu.org/r/bug-gnulib/2007-04/msg00107.html> */
  if (sprintf (buf, "%010a %d", 1.0 / zero, 33) < 0
      || buf[0] == '0')
    result |= 8;
  /* This catches a Mac OS X 10.3.9 (Darwin 7.9) bug.  */
  if (sprintf (buf, "%.1a", 1.999) < 0
      || (strcmp (buf, "0x1.0p+1") != 0
          && strcmp (buf, "0x2.0p+0") != 0
          && strcmp (buf, "0x4.0p-1") != 0
          && strcmp (buf, "0x8.0p-2") != 0))
    result |= 16;
  /* This catches the same Mac OS X 10.3.9 (Darwin 7.9) bug and also a
     glibc 2.4 bug <https://sourceware.org/bugzilla/show_bug.cgi?id=2908>.  */
  if (sprintf (buf, "%.1La", 1.999L) < 0
      || (strcmp (buf, "0x1.0p+1") != 0
          && strcmp (buf, "0x2.0p+0") != 0
          && strcmp (buf, "0x4.0p-1") != 0
          && strcmp (buf, "0x8.0p-2") != 0))
    result |= 32;
  return result;
}
'''

glibc_printf_directive_a_test = '''
#include <features.h>
#ifdef __GNU_LIBRARY__
 #if ((__GLIBC__ == 2 && __GLIBC_MINOR__ >= 5) || (__GLIBC__ > 2)) && !defined __UCLIBC__
 #else
  #error Too old glibc
 #endif
#else
 #error Not glibc
#endif
int main () { return 0; }
'''

if meson.can_run_host_binaries()
  run_result = cc.run(printf_directive_a_test,
      name : 'printf supports the \'a\' and \'A\' directives')
  gl_cv_func_printf_directive_a = run_result.compiled() and run_result.returncode() == 0
else
  if host_system == 'linux'
    gl_cv_func_printf_directive_a = cc.compiles(glibc_printf_directive_a_test)
  elif host_system == 'windows'
    gl_cv_func_printf_directive_a = false
  else
    gl_cv_func_printf_directive_a = false
  endif
endif
