#include <config.h>

/* Specification.  */
#include <gnulib_math.h>

#include <float.h>
#include "isnand-nolibm.h"

double rpl_frexp (double x, int *expptr)
{
  if (x == 0.0 || x == -0.0)
  {
    *expptr = x;
    return x;
  }
  else if (isnan (x))
    return x;
  else if (isinf (x))
    return x;
#undef frexp
  return frexp (x, expptr);
}
