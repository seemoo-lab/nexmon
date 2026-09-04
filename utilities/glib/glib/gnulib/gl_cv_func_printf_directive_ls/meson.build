# Copyright (C) 2002-2004, 2006-2018 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Test whether the *printf family of functions supports the %ls format
# directive and in particular, when a precision is specified, whether
# the functions stop converting the wide string argument when the number
# of bytes that have been produced by this conversion equals or exceeds
# the precision.
# Result is gl_cv_func_printf_directive_ls.

printf_directive_ls_test = '''
/* Tru64 with Desktop Toolkit C has a bug: <stdio.h> must be included before
   <wchar.h>.
   BSD/OS 4.0.1 has a bug: <stddef.h>, <stdio.h> and <time.h> must be
   included before <wchar.h>.  */
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <wchar.h>
#include <string.h>
int main ()
{
  int result = 0;
  char buf[100];
  /* Test whether %ls works at all.
     This test fails on OpenBSD 4.0, IRIX 6.5, Solaris 2.6, Haiku, but not on
     Cygwin 1.5.  */
  {
    static const wchar_t wstring[] = { 'a', 'b', 'c', 0 };
    buf[0] = '\0';
    if (sprintf (buf, "%ls", wstring) < 0
        || strcmp (buf, "abc") != 0)
      result |= 1;
  }
  /* This test fails on IRIX 6.5, Solaris 2.6, Cygwin 1.5, Haiku (with an
     assertion failure inside libc), but not on OpenBSD 4.0.  */
  {
    static const wchar_t wstring[] = { 'a', 0 };
    buf[0] = '\0';
    if (sprintf (buf, "%ls", wstring) < 0
        || strcmp (buf, "a") != 0)
      result |= 2;
  }
  /* Test whether precisions in %ls are supported as specified in ISO C 99
     section 7.19.6.1:
       "If a precision is specified, no more than that many bytes are written
        (including shift sequences, if any), and the array shall contain a
        null wide character if, to equal the multibyte character sequence
        length given by the precision, the function would need to access a
        wide character one past the end of the array."
     This test fails on Solaris 10.  */
  {
    static const wchar_t wstring[] = { 'a', 'b', (wchar_t) 0xfdfdfdfd, 0 };
    buf[0] = '\0';
    if (sprintf (buf, "%.2ls", wstring) < 0
        || strcmp (buf, "ab") != 0)
      result |= 8;
  }
  return result;
}
'''

if meson.can_run_host_binaries()
  run_result = cc.run(printf_directive_ls_test,
      name : 'printf supports the \'ls\' directive')
  gl_cv_func_printf_directive_ls = run_result.compiled() and run_result.returncode() == 0
else
  if (host_system.startswith ('openbsd') or
        host_system.startswith ('irix') or
        host_system.startswith ('solaris') or
        host_system.startswith ('cygwin') or
        host_system.startswith ('beos') or
        host_system.startswith ('haiku'))
    gl_cv_func_printf_directive_ls = false
  elif host_system == 'windows'
    gl_cv_func_printf_directive_ls = true
  else
    gl_cv_func_printf_directive_ls = true
  endif
endif
