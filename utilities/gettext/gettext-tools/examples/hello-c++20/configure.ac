dnl Example for use of GNU gettext.
dnl This file is in the public domain.
dnl
dnl Configuration file - processed by autoconf.

AC_INIT([hello-c++20], [0], , [hello-c++20])
AC_CONFIG_SRCDIR([hello.cc])
AM_INIT_AUTOMAKE([1.11])

AC_PROG_CXX

# Try to find compiler option for ISO C++ 20 support.
cat <<\EOF > conftest.cc
#include <format>
#if __cpp_lib_format <= 202106L
using std::vformat;
using std::make_format_args;
#else
using std::format;
using std::runtime_format;
#endif
EOF
if ${CXX} ${CXXFLAGS} ${CPPFLAGS} -c conftest.cc 2>/dev/null; then
  :
elif ${CXX} ${CXXFLAGS} -std=gnu++20 ${CPPFLAGS} -c conftest.cc 2>/dev/null; then
  CXXFLAGS="${CXXFLAGS} -std=gnu++20"
elif ${CXX} ${CXXFLAGS} -std=gnu++26 ${CPPFLAGS} -c conftest.cc 2>/dev/null; then
  CXXFLAGS="${CXXFLAGS} -std=gnu++26"
fi

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([1.0])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([m4/Makefile])
AC_CONFIG_FILES([po/Makefile.in])
AC_OUTPUT
