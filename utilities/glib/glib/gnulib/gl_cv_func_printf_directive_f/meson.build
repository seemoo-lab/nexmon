# Copyright (C) 2002-2004, 2006-2018 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Test whether the *printf family of functions supports the %F format
# directive. (ISO C99, POSIX:2001)
# Result is gl_cv_func_printf_directive_f.

printf_directive_f_test = '''
#include <stdio.h>
#include <string.h>
static char buf[100];
static double zero = 0.0;
int main ()
{
  int result = 0;
  if (sprintf (buf, "%F %d", 1234567.0, 33) < 0
      || strcmp (buf, "1234567.000000 33") != 0)
    result |= 1;
  if (sprintf (buf, "%F", 1.0 / zero) < 0
      || (strcmp (buf, "INF") != 0 && strcmp (buf, "INFINITY") != 0))
    result |= 2;
  /* This catches a Cygwin 1.5.x bug.  */
  if (sprintf (buf, "%.F", 1234.0) < 0
      || strcmp (buf, "1234") != 0)
    result |= 4;
  return result;
}
'''

if meson.can_run_host_binaries()
  run_result = cc.run(printf_directive_f_test,
      name : 'printf supports the \'F\' directive')
  gl_cv_func_printf_directive_f = run_result.compiled() and run_result.returncode() == 0
else
  if host_system in ['linux', 'android']
    gl_cv_func_printf_directive_f = true
  elif (host_system.startswith ('freebsd1') or
        host_system.startswith ('freebsd2') or
        host_system.startswith ('freebsd3') or
        host_system.startswith ('freebsd4') or
        host_system.startswith ('freebsd5'))
    gl_cv_func_printf_directive_f = false
  elif (host_system.startswith ('freebsd') or
        host_system.startswith ('kfreebsd'))
    gl_cv_func_printf_directive_f = true
  elif (host_system.startswith ('darwin1') or
        host_system.startswith ('darwin2') or
        host_system.startswith ('darwin3') or
        host_system.startswith ('darwin4') or
        host_system.startswith ('darwin5') or
        host_system.startswith ('darwin6'))
    gl_cv_func_printf_directive_f = false
  elif host_system.startswith ('darwin')
    gl_cv_func_printf_directive_f = true
# Split the check from the main if statement, ensure that
# some meson versions (old ones, presumable) won't try
# to evaluate host_system[9] when it's shorter than that
  elif host_system.startswith ('solaris2.')
    if (host_system[9] == '1' and
        '0123456789'.contains (host_system[10])) or
       ('23456789'.contains (host_system[9]) == '1' and
        '0123456789'.contains (host_system[10]))
      gl_cv_func_printf_directive_f = true
    elif host_system.startswith ('solaris')
      gl_cv_func_printf_directive_f = false
    endif
  elif host_system.startswith ('solaris')
    gl_cv_func_printf_directive_f = false
  elif host_system == 'windows'
    # Guess yes on MSVC, no on mingw.
    if cc.get_id() == 'msvc' or cc.get_id() == 'clang-cl'
      gl_cv_func_printf_directive_f = true
    else
      gl_cv_func_printf_directive_f = false
    endif
  else
    gl_cv_func_printf_directive_f = false
  endif
endif
