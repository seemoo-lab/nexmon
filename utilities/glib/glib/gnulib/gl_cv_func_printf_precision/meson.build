# Copyright (C) 2002-2004, 2006-2018 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Test whether the *printf family of functions supports large precisions.
# On mingw, precisions larger than 512 are treated like 512, in integer,
# floating-point or pointer output. On Solaris 10/x86, precisions larger
# than 510 in floating-point output crash the program. On Solaris 10/SPARC,
# precisions larger than 510 in floating-point output yield wrong results.
# On AIX 7.1, precisions larger than 998 in floating-point output yield
# wrong results. On BeOS, precisions larger than 1044 crash the program.
# Result is gl_cv_func_printf_precision.

printf_precision_test = '''
#include <stdio.h>
#include <string.h>
static char buf[5000];
int main ()
{
  int result = 0;
#ifdef __BEOS__
  /* On BeOS, this would crash and show a dialog box.  Avoid the crash.  */
  return 1;
#endif
  if (sprintf (buf, "%.4000d %d", 1, 33) < 4000 + 3)
    result |= 1;
  if (sprintf (buf, "%.4000f %d", 1.0, 33) < 4000 + 5)
    result |= 2;
  if (sprintf (buf, "%.511f %d", 1.0, 33) < 511 + 5
      || buf[0] != '1')
    result |= 4;
  if (sprintf (buf, "%.999f %d", 1.0, 33) < 999 + 5
      || buf[0] != '1')
    result |= 4;
  return result;
}
'''

if meson.can_run_host_binaries()
  run_result = cc.run(printf_precision_test,
      name : 'printf supports large precisions')
  gl_cv_func_printf_precision = run_result.compiled() and run_result.returncode() == 0
else
  gl_cv_func_printf_precision = true
  # Guess no only on Solaris, native Windows, and BeOS systems.
  if (host_system == 'windows' or
      host_system == 'beos' or
      host_system == 'haiku' or
      host_system == 'sunos' or 
      host_system == 'solaris')
    gl_cv_func_printf_precision = false
  endif
endif
