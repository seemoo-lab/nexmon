# Example for use of GNU gettext.
# This file is in the public domain.
#
# Makefile configuration - processed by automake.

# General automake options.
AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

# The list of subdirectories containing Makefiles.
SUBDIRS = m4 po

# The list of programs that are built.
bin_PROGRAMS = hello

# The source files of the 'hello' program.
nodist_hello_SOURCES = hello.mod

# Compile time dependencies.
if HAVE_M2_LIBRARIES_PREFIX
# Use the prefix specified as configure option.
m2includedir = $(M2_LIBRARIES_PREFIX)/include/m2
m2libdir = $(M2_LIBRARIES_EXECPREFIX)/lib
else
# Use the prefix determined by autogen.sh.
include $(srcdir)/m2dirs.mk
endif

# Link time dependencies.
LDADD = -lintl_m2 @LIBINTL@

hello$(EXEEXT): hello.mod
	$(M2C) -I$(m2includedir) -L$(m2libdir) -Wl,-rpath,$(m2libdir) $(M2FLAGS) \
	       -o hello$(EXEEXT) hello.mod $(LDADD)

# Additional files to be distributed.
EXTRA_DIST = autogen.sh autoclean.sh
