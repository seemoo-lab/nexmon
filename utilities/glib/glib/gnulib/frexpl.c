#include <config.h>
#include <gnulib_math.h>
#include <float.h>
#include <math.h>

long double rpl_frexpl (long double x, int *expptr)
{
  if (x == 0.0L || x == -0.0L)
  {
    *expptr = x;
    return x;
  }
  else if (isnanl (x))
    return x;
  else if (isinf (x))
    return x;
#ifndef _MSC_VER
#undef frexpl
#endif
  return frexpl (x, expptr);
}
