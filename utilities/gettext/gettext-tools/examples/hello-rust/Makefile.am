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
bin_RUSTPROGRAMS = hello

# The source files of the 'hello' program.
hello_SOURCES = src/main.rs

DISTCLEANFILES =

# Additional files to be distributed.
EXTRA_DIST = autogen.sh autoclean.sh


# ----------------- General rules for compiling Rust programs -----------------

# Support for VPATH builds.
DISTCLEANFILES += Cargo.toml
EXTRA_DIST += Cargo.toml.in

DISTCLEANFILES += Cargo.lock


# Rules for compiling Rust programs.

all-local: Cargo.toml $(hello_SOURCES)
	$(CARGO) build

check-local:
	$(CARGO) test

# The target/ directory takes 2.5 to 5 minutes to rebuild. Therefore
# we delete it only at "make maintainer-clean", not at "make distclean".
maintainer-clean-local:
#	$(CARGO) clean
	rm -rf target
# Customize "make distcheck" accordingly.
distcleancheck_listfiles = \
  find . \( -type f \
            -a \! \( -name .nfs* -o -name .smb* -o -name .__afs* \) \
            -a '!' -path './target/*' \) \
         -print

install-exec-local: all-local
	$(MKDIR_P) $(DESTDIR)$(bindir)
	$(INSTALL_PROGRAM) target/debug/hello$(EXEEXT) $(DESTDIR)$(bindir)/hello

installdirs-local:
	$(MKDIR_P) $(DESTDIR)$(bindir)

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/hello
