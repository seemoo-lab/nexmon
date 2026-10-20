# Example for use of GNU gettext.
# This file is in the public domain.
#
# Makefile configuration - processed by automake.

# The list of programs that are built.
bin_PROGRAMS = hello3ml

# The source files of the 'hello3ml' program.
nodist_hello3ml_SOURCES = hello3ml.go

EXTRA_DIST = go.mod.in

CLEANFILES = go.sum

# Rules for compiling the programs.

go.sum:
	$(GO) mod download github.com/snapcore/go-gettext

hello3ml$(EXEEXT): $(nodist_hello3ml_SOURCES) go.sum
	$(SHELL) ../gocomp.sh $(nodist_hello3ml_SOURCES)
