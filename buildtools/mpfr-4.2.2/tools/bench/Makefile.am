# Copyright 2000-2025 Free Software Foundation, Inc.
# Contributed by the Pascaline and Caramba projects, INRIA.

# This file is part of the GNU MPFR Library.

# This Makefile.am is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.


# The ".POSIX" line is needed in particular for GNU "make", so that
# recipes are invoked as if the shell had been passed the -e flag.
# But note that since GNU Automake adds non-comment lines before it
# in its generated Makefile, this does not ensure POSIX behavior with
# other "make" implementations.
.POSIX:

AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src
AM_DEFAULT_SOURCE_EXT = .c

LDADD = $(top_builddir)/src/libmpfr.la

EXTRA_PROGRAMS = mpfrbench

EXTRA_DIST = README

noinst_HEADERS = benchtime.h

bench : mpfrbench
	./mpfrbench
