# Copyright (C) 2002-2004, 2006-2018 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Test whether the *printf family of functions recovers gracefully in case
# of an out-of-memory condition, or whether it crashes the entire program.
# Result is gl_cv_func_printf_enomem.

printf_enomem_test = '''
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
int main()
{
  struct rlimit limit;
  int ret;
  nocrash_init ();
  /* Some printf implementations allocate temporary space with malloc.  */
  /* On BSD systems, malloc() is limited by RLIMIT_DATA.  */
#ifdef RLIMIT_DATA
  if (getrlimit (RLIMIT_DATA, &limit) < 0)
    return 77;
  if (limit.rlim_max == RLIM_INFINITY || limit.rlim_max > 5000000)
    limit.rlim_max = 5000000;
  limit.rlim_cur = limit.rlim_max;
  if (setrlimit (RLIMIT_DATA, &limit) < 0)
    return 77;
#endif
  /* On Linux systems, malloc() is limited by RLIMIT_AS.  */
#ifdef RLIMIT_AS
  if (getrlimit (RLIMIT_AS, &limit) < 0)
    return 77;
  if (limit.rlim_max == RLIM_INFINITY || limit.rlim_max > 5000000)
    limit.rlim_max = 5000000;
  limit.rlim_cur = limit.rlim_max;
  if (setrlimit (RLIMIT_AS, &limit) < 0)
    return 77;
#endif
  /* Some printf implementations allocate temporary space on the stack.  */
#ifdef RLIMIT_STACK
  if (getrlimit (RLIMIT_STACK, &limit) < 0)
    return 77;
  if (limit.rlim_max == RLIM_INFINITY || limit.rlim_max > 5000000)
    limit.rlim_max = 5000000;
  limit.rlim_cur = limit.rlim_max;
  if (setrlimit (RLIMIT_STACK, &limit) < 0)
    return 77;
#endif
  ret = printf ("%.5000000f", 1.0);
  return !(ret == 5000002 || (ret < 0 && errno == ENOMEM));
}
'''

if meson.can_run_host_binaries()
  run_result = cc.run(printf_enomem_test,
      name : 'printf survives out-of-memory conditions')
  gl_cv_func_printf_enomem = run_result.compiled() and run_result.returncode() == 0
else
  # If we don't know, assume the worst.
  gl_cv_func_printf_enomem = false
  if (host_system == 'linux' or
      host_system == 'solaris' or
      host_system == 'sunos' or
      host_system == 'aix' or 
      host_system == 'irix' or 
      host_system == 'beos' or 
      host_system == 'haiku')
    gl_cv_func_printf_enomem = true
  endif
endif
