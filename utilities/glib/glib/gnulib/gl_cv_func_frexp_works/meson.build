# Copyright (C) 2002-2004, 2006-2018 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

frexp_test = '''
#define HAVE_DECL_ALARM 0
#include <float.h>
#include <math.h>
#include <string.h>
#if HAVE_DECL_ALARM
# include <signal.h>
# include <unistd.h>
#endif
/* HP cc on HP-UX 10.20 has a bug with the constant expression -0.0.
   ICC 10.0 has a bug when optimizing the expression -zero.
   The expression -DBL_MIN * DBL_MIN does not work when cross-compiling
   to PowerPC on Mac OS X 10.5.  */
#if defined __hpux || defined __sgi || defined __ICC
static double
compute_minus_zero (void)
{
  return -DBL_MIN * DBL_MIN;
}
# define minus_zero compute_minus_zero ()
#else
double minus_zero = -0.0;
#endif
int main()
{
  int result = 0;
  int i;
  volatile double x;
  double zero = 0.0;
#if HAVE_DECL_ALARM
  /* NeXTstep 3.3 frexp() runs into an endless loop when called on an infinite
     number.  Let the test fail in this case.  */
  signal (SIGALRM, SIG_DFL);
  alarm (5);
#endif
  /* Test on denormalized numbers.  */
  for (i = 1, x = 1.0; i >= DBL_MIN_EXP; i--, x *= 0.5)
    ;
  if (x > 0.0)
    {
      int exp;
      double y = frexp (x, &exp);
      /* On machines with IEEE754 arithmetic: x = 1.11254e-308, exp = -1022.
         On NetBSD: y = 0.75. Correct: y = 0.5.  */
      if (y != 0.5)
        result |= 1;
    }
  /* Test on infinite numbers.  */
  x = 1.0 / zero;
  {
    int exp;
    double y = frexp (x, &exp);
    if (y != x)
      result |= 2;
  }
  /* Test on negative zero.  */
  x = minus_zero;
  {
    int exp;
    double y = frexp (x, &exp);
    if (memcmp (&y, &x, sizeof x))
      result |= 4;
  }
  x = 0.0;
  {
    int exp;
    double y = frexp (x, &exp);
    if (memcmp (&y, &x, sizeof x))
      result |= 8;
  }
  return result;
}'''

if meson.can_run_host_binaries()
  run_result = cc.run(frexp_test,
      name : 'frexp works',
      dependencies : [libm])
  rc = run_result.returncode()
  gl_cv_func_frexp_works = run_result.compiled() and rc == 0
  gl_cv_func_frexp_broken_beyond_repair = not gl_cv_func_frexp_works
# bit 1 is not set
  if (rc == 1 or rc == 3 or rc == 5 or rc == 9 or
      rc == 7 or rc == 11 or rc == 13)
    gl_cv_func_frexp_broken_beyond_repair = true
  else
    gl_cv_func_frexp_broken_beyond_repair = false
  endif
else
  if (host_system.startswith ('netbsd') or
      host_system.startswith ('irix'))
    gl_cv_func_frexp_works = false
    gl_cv_func_frexp_broken_beyond_repair = true
  elif host_system == 'windows'
    if cc.get_id () == 'msvc' or cc.get_id() == 'clang-cl'
      gl_cv_func_frexp_works = true
      gl_cv_func_frexp_broken_beyond_repair = false
    else
      gl_cv_func_frexp_works = false
      gl_cv_func_frexp_broken_beyond_repair = false
    endif
  else
    gl_cv_func_frexp_works = true
    gl_cv_func_frexp_broken_beyond_repair = false
  endif
endif
