# Copyright (C) 2002-2004, 2006-2018 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Test whether the *printf family of functions supports the - flag correctly.
# (ISO C99.) See
# <https://lists.gnu.org/r/bug-coreutils/2008-02/msg00035.html>
# Result is gl_cv_func_printf_flag_leftadjust.

printf_flag_grouping_test = '''
#include <stdio.h>
#include <string.h>
static char buf[100];
int main ()
{
  if (sprintf (buf, "%'d %d", 1234567, 99) < 0
      || buf[strlen (buf) - 1] != '9')
    return 1;
  return 0;
}'''

if meson.can_run_host_binaries()
  run_result = cc.run(printf_flag_grouping_test,
      name : 'printf supports the grouping flag')
  gl_cv_func_printf_flag_grouping = run_result.compiled() and run_result.returncode() == 0
else
  if host_system.startswith ('cygwin')
    gl_cv_func_printf_flag_grouping = false
  elif host_system.startswith ('netbsd')
    gl_cv_func_printf_flag_grouping = false
  elif host_system == 'windows'
    gl_cv_func_printf_flag_grouping = false
  else
    gl_cv_func_printf_flag_grouping = true
  endif
endif
