#ifndef _MSC_VER
#error "This implementation is currently supported for Visual Studio only!"
#endif

#include "config.h"
#include <gnulib_math.h>
#include <float.h>
#include <math.h>

int
gl_isinff (float x)
{
#if defined (_WIN64) && (defined (_M_X64) || defined (_M_AMD64))
  return !_finitef (x);
#else
  return !_finite (x);
#endif
}

int
gl_isinfd (double x)
{
  return !_finite (x);
}

int
gl_isinfl (long double x)
{
  return gl_isinfd (x);
}
