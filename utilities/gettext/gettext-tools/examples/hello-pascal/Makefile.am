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
bin_PASCALPROGRAMS = hello

# The source files of the 'hello' program.
hello_SOURCES = hello.pas

# Additional files to be distributed.
EXTRA_DIST = autogen.sh autoclean.sh

# ---------------- General rules for compiling Pascal programs ----------------

all-local: hello$(EXEEXT)

# How to build the 'hello' program.
hello$(EXEEXT) hello.rst: $(hello_SOURCES)
	LOCALEDIR='@localedir@' $(PPC) $(hello_SOURCES)

install-exec-local: all-local
	$(MKDIR_P) $(DESTDIR)$(bindir)
	$(INSTALL_PROGRAM) hello$(EXEEXT) $(DESTDIR)$(bindir)/hello$(EXEEXT)

installdirs-local:
	$(MKDIR_P) $(DESTDIR)$(bindir)

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/hello$(EXEEXT)

# Distribute the RST file because it's needed to generate POT files and can
# only be rebuilt on those platforms to which the Pascal compiler is ported.
EXTRA_DIST += hello.rst

# The list of auxiliary files generated during the compilation.
CLEANFILES = hello.o hello$(EXEEXT)
