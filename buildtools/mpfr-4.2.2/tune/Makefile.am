# Copyright 2010-2025 Free Software Foundation, Inc.
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

EXTRA_PROGRAMS = tuneup speed bidimensional_sample

tuneup_SOURCES = tuneup.c
tuneup_LDADD = -lspeed $(top_builddir)/src/libmpfr.la $(TUNE_LIBS)
tuneup_LDFLAGS = -static

speed_SOURCES = speed.c
speed_LDADD = -lspeed $(top_builddir)/src/libmpfr.la $(TUNE_LIBS)
speed_LDFLAGS = -static

bidimensional_sample_SOURCES = bidimensional_sample.c
bidimensional_sample_LDADD = -lspeed $(top_builddir)/src/libmpfr.la $(TUNE_LIBS)
bidimensional_sample_LDFLAGS = -static

AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src

tune:
	$(MAKE) $(AM_MAKEFLAGS) tuneup$(EXEEXT)
	./tuneup$(EXEEXT) -v
	mv mparam.h $(top_builddir)/src/
	cd $(top_builddir) && $(MAKE) $(AM_MAKEFLAGS) clean
	cd $(top_builddir)/src && $(MAKE) $(AM_MAKEFLAGS) libmpfr.la

$(top_builddir)/src/libmpfr.la:
	cd $(top_builddir)/src && $(MAKE) $(AM_MAKEFLAGS) libmpfr.la

CLEANFILES = $(EXTRA_PROGRAMS) mparam.h
