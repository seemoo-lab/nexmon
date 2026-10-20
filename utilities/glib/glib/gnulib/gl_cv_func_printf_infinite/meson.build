# Copyright (C) 2002-2004, 2006-2018 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Test whether the *printf family of functions supports infinite and NaN
# 'double' arguments and negative zero arguments in the %f, %e, %g
# directives. (ISO C99, POSIX:2001)
# Result is gl_cv_func_printf_infinite.

printf_infinite_double_test = '''
#include <stdio.h>
#include <string.h>
static int
strisnan (const char *string, size_t start_index, size_t end_index)
{
  if (start_index < end_index)
    {
      if (string[start_index] == '-')
        start_index++;
      if (start_index + 3 <= end_index
          && memcmp (string + start_index, "nan", 3) == 0)
        {
          start_index += 3;
          if (start_index == end_index
              || (string[start_index] == '(' && string[end_index - 1] == ')'))
            return 1;
        }
    }
  return 0;
}
static int
have_minus_zero ()
{
  static double plus_zero = 0.0;
  double minus_zero = - plus_zero;
  return memcmp (&plus_zero, &minus_zero, sizeof (double)) != 0;
}
static char buf[10000];
static double zero = 0.0;
int main ()
{
  int result = 0;
  if (sprintf (buf, "%f", 1.0 / zero) < 0
      || (strcmp (buf, "inf") != 0 && strcmp (buf, "infinity") != 0))
    result |= 1;
  if (sprintf (buf, "%f", -1.0 / zero) < 0
      || (strcmp (buf, "-inf") != 0 && strcmp (buf, "-infinity") != 0))
    result |= 1;
  if (sprintf (buf, "%f", zero / zero) < 0
      || !strisnan (buf, 0, strlen (buf)))
    result |= 2;
  if (sprintf (buf, "%e", 1.0 / zero) < 0
      || (strcmp (buf, "inf") != 0 && strcmp (buf, "infinity") != 0))
    result |= 4;
  if (sprintf (buf, "%e", -1.0 / zero) < 0
      || (strcmp (buf, "-inf") != 0 && strcmp (buf, "-infinity") != 0))
    result |= 4;
  if (sprintf (buf, "%e", zero / zero) < 0
      || !strisnan (buf, 0, strlen (buf)))
    result |= 8;
  if (sprintf (buf, "%g", 1.0 / zero) < 0
      || (strcmp (buf, "inf") != 0 && strcmp (buf, "infinity") != 0))
    result |= 16;
  if (sprintf (buf, "%g", -1.0 / zero) < 0
      || (strcmp (buf, "-inf") != 0 && strcmp (buf, "-infinity") != 0))
    result |= 16;
  if (sprintf (buf, "%g", zero / zero) < 0
      || !strisnan (buf, 0, strlen (buf)))
    result |= 32;
  /* This test fails on HP-UX 10.20.  */
  if (have_minus_zero ())
    if (sprintf (buf, "%g", - zero) < 0
        || strcmp (buf, "-0") != 0)
    result |= 64;
  return result;
}
'''

if meson.can_run_host_binaries()
  run_result = cc.run(printf_infinite_double_test,
      name : 'printf supports infinite \'double\' arguments')
  gl_cv_func_printf_infinite = run_result.compiled() and run_result.returncode() == 0
else
  if host_system in ['linux', 'android']
    gl_cv_func_printf_infinite = true
  elif (host_system.startswith ('freebsd1') or
        host_system.startswith ('freebsd2') or
        host_system.startswith ('freebsd3') or
        host_system.startswith ('freebsd4') or
        host_system.startswith ('freebsd5'))
    gl_cv_func_printf_infinite = false
  elif (host_system.startswith ('freebsd') or
        host_system.startswith ('kfreebsd'))
    gl_cv_func_printf_infinite = true
  elif (host_system.startswith ('darwin1') or
        host_system.startswith ('darwin2') or
        host_system.startswith ('darwin3') or
        host_system.startswith ('darwin4') or
        host_system.startswith ('darwin5'))
    gl_cv_func_printf_infinite = false
  elif host_system.startswith ('darwin')
    gl_cv_func_printf_infinite = true
  elif (host_system.startswith ('hpux7') or
        host_system.startswith ('hpux8') or
        host_system.startswith ('hpux9') or
        host_system.startswith ('hpux10'))
    gl_cv_func_printf_infinite = false
  elif host_system.startswith ('hpux')
    gl_cv_func_printf_infinite = true
  elif (host_system.startswith ('netbsd1') or
        host_system.startswith ('netbsd2') or
        host_system.startswith ('netbsdelf1') or
        host_system.startswith ('netbsdelf2') or
        host_system.startswith ('netbsdaout1') or
        host_system.startswith ('netbsdaout2') or
        host_system.startswith ('netbsdcoff1') or
        host_system.startswith ('netbsdcoff2'))
    gl_cv_func_printf_infinite = false
  elif host_system.startswith ('netbsd')
    gl_cv_func_printf_infinite = true
  elif host_system.startswith ('beos')
    gl_cv_func_printf_infinite = true
  elif host_system.startswith ('windows')
    # Guess yes on MSVC, no on mingw.
    if cc.get_id() == 'msvc' or cc.get_id() == 'clang-cl'
      gl_cv_func_printf_infinite = true
    else
      gl_cv_func_printf_infinite = false
    endif
  else
    # If we don't know, assume the worst.
    gl_cv_func_printf_infinite = false
  endif
endif
