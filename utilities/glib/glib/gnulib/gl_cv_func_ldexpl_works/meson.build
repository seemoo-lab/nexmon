# Copyright (C) 2002-2004, 2006-2018 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

ldexpl_test = '''
#include <math.h>
extern
#ifdef __cplusplus
"C"
#endif

#if !defined (_MSC_VER) || defined (TEST_LDEXPL_DECL)
long double ldexpl (long double, int);
#endif

int main()
{
  int result = 0;
  {
    volatile long double x = 1.0;
    volatile long double y = ldexpl (x, -1);
    if (y != 0.5L)
      result |= 1;
  }
  {
    volatile long double x = 1.73205L;
    volatile long double y = ldexpl (x, 0);
    if (y != x)
      result |= 2;
  }
  return result;
}
'''

if meson.can_run_host_binaries()
  run_result = cc.run(ldexpl_test,
      name : 'ldexpl works',
      dependencies : [libm])
  gl_cv_func_ldexpl_works = run_result.compiled() and run_result.returncode() == 0
else
  if host_system.startswith ('aix')
    gl_cv_func_ldexpl_works = false
  elif host_system == 'windows'
    gl_cv_func_ldexpl_works = true
  else
    gl_cv_func_ldexpl_works = true
  endif
endif

ldexpl_test_decl = '''
#define TEST_LDEXPL_DECL 1
''' + ldexpl_test

build_result = cc.compiles(ldexpl_test_decl,
    name : 'ldexpl prototype can be re-listed')
gl_cv_func_ldexpl_decl = build_result
