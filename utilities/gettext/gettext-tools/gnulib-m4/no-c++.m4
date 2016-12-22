# no-c++.m4 serial 1
dnl Copyright (C) 2006, 2009-2016 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# Support for C source files that cannot be compiled by a C++ compiler.
# Set NO_CXX to the C++ compiler flags needed to request C mode instead of
# C++ mode.
# So far only g++ is supported.

AC_DEFUN([gt_NO_CXX],
[
  NO_CXX=
  AC_EGREP_CPP([Is g++], [
#if defined __GNUC__ && defined __cplusplus
  Is g++
#endif
    ],
    [NO_CXX="-x c"])
  AC_SUBST([NO_CXX])
])
