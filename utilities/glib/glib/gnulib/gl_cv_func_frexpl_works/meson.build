# Copyright (C) 2007-2018 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

frexpl_test = '''
#include <float.h>
#include <math.h>
/* Override the values of <float.h>, like done in float.in.h.  */
#if defined __i386__ && (defined __BEOS__ || defined __OpenBSD__)
# undef LDBL_MIN_EXP
# define LDBL_MIN_EXP    (-16381)
#endif
#if defined __i386__ && (defined __FreeBSD__ || defined __DragonFly__)
# undef LDBL_MIN_EXP
# define LDBL_MIN_EXP    (-16381)
#endif
#if (defined _ARCH_PPC || defined _POWER) && defined _AIX && (LDBL_MANT_DIG == 106) && defined __GNUC__
# undef LDBL_MIN_EXP
# define LDBL_MIN_EXP DBL_MIN_EXP
#endif
#if defined __sgi && (LDBL_MANT_DIG >= 106)
# if defined __GNUC__
#  undef LDBL_MIN_EXP
#  define LDBL_MIN_EXP DBL_MIN_EXP
# endif
#endif
extern
#ifdef __cplusplus
"C"
#endif

#if !defined (_MSC_VER) || defined (TEST_FREXPL_DECL)
long double frexpl (long double, int *);
#endif

int main()
{
  int result = 0;
  volatile long double x;
  /* Test on finite numbers that fails on AIX 5.1.  */
  x = 16.0L;
  {
    int exp = -9999;
    frexpl (x, &exp);
    if (exp != 5)
      result |= 1;
  }
  /* Test on finite numbers that fails on Mac OS X 10.4, because its frexpl
     function returns an invalid (incorrectly normalized) value: it returns
               y = { 0x3fe028f5, 0xc28f5c28, 0x3c9eb851, 0xeb851eb8 }
     but the correct result is
          0.505L = { 0x3fe028f5, 0xc28f5c29, 0xbc547ae1, 0x47ae1480 }  */
  x = 1.01L;
  {
    int exp = -9999;
    long double y = frexpl (x, &exp);
    if (!(exp == 1 && y == 0.505L))
      result |= 2;
  }
  /* Test on large finite numbers.  This fails on BeOS at i = 16322, while
     LDBL_MAX_EXP = 16384.
     In the loop end test, we test x against Infinity, rather than comparing
     i with LDBL_MAX_EXP, because BeOS <float.h> has a wrong LDBL_MAX_EXP.  */
  {
    int i;
    for (i = 1, x = 1.0L; x != x + x; i++, x *= 2.0L)
      {
        int exp = -9999;
        frexpl (x, &exp);
        if (exp != i)
          {
            result |= 4;
            break;
          }
      }
  }
  /* Test on denormalized numbers.  */
  {
    int i;
    for (i = 1, x = 1.0L; i >= LDBL_MIN_EXP; i--, x *= 0.5L)
      ;
    if (x > 0.0L)
      {
        int exp;
        long double y = frexpl (x, &exp);
        /* On machines with IEEE854 arithmetic: x = 1.68105e-4932,
           exp = -16382, y = 0.5.  On Mac OS X 10.5: exp = -16384, y = 0.5.  */
        (void) y;
        if (exp != LDBL_MIN_EXP - 1)
          result |= 8;
      }
  }
  /* Test on infinite numbers.  */
  /* The dance around 0.0L is an attempt to prevent MSVC from erroring out */
  x = 0.0L;
  x = 1.0L / x;
  {
    int exp;
    long double y = frexpl (x, &exp);
    if (y != x)
      result |= 16;
  }
  return result;
}
'''

if meson.can_run_host_binaries()
  run_result = cc.run(frexpl_test,
      name : 'frexpl works',
      dependencies : [libm])
  rc = run_result.returncode()
  gl_cv_func_frexpl_works = run_result.compiled() and rc == 0
  gl_cv_func_frexpl_broken_beyond_repair = not gl_cv_func_frexpl_works
# bit 1 is not set
  if (rc == 16)
    gl_cv_func_frexpl_broken_beyond_repair = false
  else
    gl_cv_func_frexpl_broken_beyond_repair = true
  endif
else
  if (host_system.startswith ('aix') or
      host_system.startswith ('beos') or
      host_system.startswith ('irix'))
    gl_cv_func_frexpl_works = false
    gl_cv_func_frexpl_broken_beyond_repair = true
  elif (host_system == 'windows')
    gl_cv_func_frexpl_works = false
    gl_cv_func_frexpl_broken_beyond_repair = false
  else
    gl_cv_func_frexpl_works = true
    gl_cv_func_frexpl_broken_beyond_repair = false
  endif
endif

frexpl_test_decl = '''
#define TEST_FREXPL_DECL 1
''' + frexpl_test

build_result = cc.compiles(frexpl_test_decl,
    name : 'frexpl prototype can be re-listed')
gl_cv_func_frexpl_decl = build_result
