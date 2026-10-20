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
nodist_hello_SOURCES = hello.d

# Compile time dependencies.
dincludedir = $(D_PACKAGES_PREFIX)/include/d
dlibdir = $(D_PACKAGES_EXECPREFIX)/lib

# Link time dependencies.
LDADD = @LIBINTL@

hello$(EXEEXT): hello.d $(dlibdir)/libintl_d.a
	$(SHELL) ./dcomp.sh -I$(dincludedir) -o hello$(EXEEXT) hello.d $(dlibdir)/libintl_d.a $(LDADD)

# Compile dependency module.
# For simplicity, compile the entire module to a single object file.
$(dlibdir)/libintl_d.a:
	mkdir -p $(dlibdir)
	$(SHELL) ./dcomp.sh -I$(dincludedir) -c -o $(dlibdir)/libintl_d.a `find $(dincludedir)/gnu/libintl -name '*.d' -print`

DISTCLEANFILES = dcomp.sh

distclean-local: distclean-generic
	@if test '$(D_PACKAGES_EXECPREFIX)' = './depends'; then \
	  rm -f $(dlibdir)/libintl_d.a; \
	  rmdir $(dlibdir); \
	  rmdir depends 2>/dev/null || true; \
	fi

# Additional files to be distributed.
EXTRA_DIST = \
  autogen.sh autoclean.sh \
  depends/include/d/gnu/libintl/package.d \
  depends/include/d/gnu/libintl/libintl.d \
  depends/include/d/gnu/libintl/internal/low.d
