# Copyright (C) 2002-2004, 2006-2018 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Test whether the *printf family of functions supports padding of non-finite
# values with the 0 flag correctly. (ISO C99 + TC1 + TC2.) See
# <https://lists.gnu.org/r/bug-gnulib/2007-04/msg00107.html>
# Result is gl_cv_func_printf_flag_zero.

printf_flag_zero_test = '''
#include <stdio.h>
#include <string.h>
static char buf[100];
static double zero = 0.0;
int main ()
{
  if (sprintf (buf, "%010f", 1.0 / zero) < 0
      || (strcmp (buf, "       inf") != 0
          && strcmp (buf, "  infinity") != 0))
    return 1;
  return 0;
}
'''

if meson.can_run_host_binaries()
  run_result = cc.run(printf_flag_zero_test,
      name : 'printf supports the zero flag correctly')
  gl_cv_func_printf_flag_zero = run_result.compiled() and run_result.returncode() == 0
else
  if host_system in ['linux', 'android']
    gl_cv_func_printf_flag_zero = true
  elif host_system.startswith ('beos')
    gl_cv_func_printf_flag_zero = true
  elif host_system == 'windows'
    gl_cv_func_printf_flag_zero = false
  else
    gl_cv_func_printf_flag_zero = false
  endif
endif
