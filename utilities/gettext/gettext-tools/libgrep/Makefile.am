## Makefile for libgrep directory in GNU gettext package.
## Copyright (C) 2005-2025 Free Software Foundation, Inc.
##
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <https://www.gnu.org/licenses/>.

## Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.5 gnits subdir-objects

SUBDIRS = gnulib-lib

EXTRA_DIST =
BUILT_SOURCES =
MOSTLYCLEANFILES = core *.stackdump
MOSTLYCLEANDIRS =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =

noinst_LIBRARIES = libgrep.a

# Note that Automake's $(DEFAULT_INCLUDES) already contains
# -I. -I$(srcdir) -I$(top_builddir).
AM_CPPFLAGS = \
  -Ignulib-lib -I$(srcdir)/gnulib-lib \
  -I../gnulib-lib -I$(top_srcdir)/gnulib-lib \
  -I../../gettext-runtime/intl

# Enable more warning options in this directory.
AM_CFLAGS = @WARN_CFLAGS@

# Sources that are compiled on all platforms.

libgrep_a_SOURCES = \
  libgrep.h \
  m-fgrep.c m-regex.c

libgrep_a_DEPENDENCIES = gnulib-lib/libgnu.a

# Allow users to use "gnulib-tool --update".
EXTRA_DIST += gnulib-m4/gnulib-cache.m4
