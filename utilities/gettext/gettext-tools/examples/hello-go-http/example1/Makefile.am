# Example for use of GNU gettext.
# This file is in the public domain.
#
# Makefile configuration - processed by automake.

# The list of programs that are built.
bin_PROGRAMS = hello1ml

# The source files of the 'hello1ml' program.
nodist_hello1ml_SOURCES = hello1ml.go

EXTRA_DIST = go.mod.in

CLEANFILES = go.sum

# Rules for compiling the programs.

go.sum:
	$(GO) mod download github.com/leonelquinteros/gotext

hello1ml$(EXEEXT): $(nodist_hello1ml_SOURCES) go.sum
	$(SHELL) ../gocomp.sh $(nodist_hello1ml_SOURCES)
