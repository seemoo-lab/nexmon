# Copyright (C) 2002-2004, 2006-2018 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Test whether the *printf family of functions supports 'long double'
# arguments together with the 'L' size specifier. (ISO C99, POSIX:2001)
# Result is gl_cv_func_printf_long_double.

printf_long_double_test = '''
#include <stdio.h>
#include <string.h>
static char buf[10000];
int main ()
{
  int result = 0;
  buf[0] = '\0';
  if (sprintf (buf, "%Lf %d", 1.75L, 33) < 0
      || strcmp (buf, "1.750000 33") != 0)
    result |= 1;
  buf[0] = '\0';
  if (sprintf (buf, "%Le %d", 1.75L, 33) < 0
      || strcmp (buf, "1.750000e+00 33") != 0)
    result |= 2;
  buf[0] = '\0';
  if (sprintf (buf, "%Lg %d", 1.75L, 33) < 0
      || strcmp (buf, "1.75 33") != 0)
    result |= 4;
  return result;
}
'''

if meson.can_run_host_binaries()
  run_result = cc.run(printf_long_double_test,
      name : 'printf supports \'long double\' arguments')
  gl_cv_func_printf_long_double = run_result.compiled() and run_result.returncode() == 0
else
 if host_system.startswith ('beos')
   gl_cv_func_printf_long_double = false
 elif host_system == 'windows'
   # Guess yes on MSVC, no on mingw.
   if cc.get_id() == 'msvc' or cc.get_id() == 'clang-cl'
     gl_cv_func_printf_long_double = true
   else
     gl_cv_func_printf_long_double = false
   endif
 else
   gl_cv_func_printf_long_double = true
 endif
endif
