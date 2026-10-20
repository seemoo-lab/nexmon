# Copyright (C) 2002-2004, 2006-2018 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# gl_LONG_DOUBLE_VS_DOUBLE
# determines whether 'long double' and 'double' have the same representation.
# The currently known platforms where this is the case are:
# Linux/HPPA, Minix 3.1.8, AIX 5, AIX 6 and 7 with xlc, MSVC 9.

long_double_eq_double_test = '''
#include <float.h>
int main ()
{
  typedef int check[sizeof (long double) == sizeof (double)
                    && LDBL_MANT_DIG == DBL_MANT_DIG
                    && LDBL_MAX_EXP == DBL_MAX_EXP
                    && LDBL_MIN_EXP == DBL_MIN_EXP
                    ? 1 : -1];
  return check;
}
'''

gl_cv_long_double_equals_double = cc.compiles(long_double_eq_double_test)
