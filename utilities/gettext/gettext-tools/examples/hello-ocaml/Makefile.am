# Example for use of GNU gettext.
# This file is in the public domain.
#
# Makefile configuration - processed by automake.

# General automake options.
AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

# The list of subdirectories containing Makefiles.
SUBDIRS = m4 po

# The list of OCaml programs that are built.
bin_OCAMLPROGRAMS = hello

# The source files of the 'hello' program.
hello_SOURCES = hello.ml

DISTCLEANFILES =

# Additional files to be distributed.
EXTRA_DIST = autogen.sh autoclean.sh


# ----------------- General rules for compiling OCaml programs -----------------

# Support for VPATH builds.
DUNE_BUILD = @DUNE@ build --root=.

EXTRA_DIST += dune.in dune-project.in


# Rules for compiling OCaml programs.

all-local: _build/default/hello.exe

_build/default/hello.exe: dune dune-project $(hello_SOURCES)
	$(DUNE_BUILD) hello.exe

mostlyclean-local:
	rm -rf _build

install-exec-local: all-local
	$(MKDIR_P) $(DESTDIR)$(bindir)
	$(INSTALL_PROGRAM) _build/default/hello.exe $(DESTDIR)$(bindir)/hello

installdirs-local:
	$(MKDIR_P) $(DESTDIR)$(bindir)

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/hello
