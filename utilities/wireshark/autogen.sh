#!/bin/sh
#
# Run this to generate all the initial makefiles.
#
# Copyright 2014 The Wireshark Authors
#
# Wireshark - Network traffic analyzer
# By Gerald Combs <gerald@wireshark.org>
# Copyright 1998 Gerald Combs
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

DIE=true
PROJECT="Wireshark"

# If you are going to use the non-default name for automake becase your OS
# installaion has multiple versions, you need to call both aclocal and automake
# with that version number, as they come from the same package.
#AM_VERSION='-1.8'

ACLOCAL=aclocal$AM_VERSION
AUTOHEADER=autoheader
AUTOMAKE=automake$AM_VERSION
AUTOCONF=autoconf
PKG_CONFIG=pkg-config

# Check for python. Python did not support --version before version 2.5.
# Until we require a version > 2.5, we should use -V.
PYVER=`exec python -V 2>&1 | sed 's/Python *//'`
# If "python" isn't found, try "python3"
if test "$PYVER" = "exec: python: not found"
then
    PYVER=`exec python3 -V 2>&1 | sed 's/Python *//'`
fi
case "$PYVER" in
2*|3*)
  ;;
*)
  cat >&2 <<_EOF_

  	You must have Python in order to compile $PROJECT.
	Download the appropriate package for your distribution/OS,
	or get the source tarball at http://www.python.org/
_EOF_
  DIE="exit 1"
esac

ACVER=`$AUTOCONF --version | grep '^autoconf' | sed 's/.*) *//'`
case "$ACVER" in
'' | 0.* | 1.* | 2.[0-5]* | 2.6[0123]* )
  cat >&2 <<_EOF_

	You must have autoconf 2.64 or later installed to compile $PROJECT.
	Download the appropriate package for your distribution/OS,
	or get the source tarball at ftp://ftp.gnu.org/pub/gnu/autoconf/
_EOF_
  DIE="exit 1"
  ;;
esac


AMVER=`$AUTOMAKE --version | grep '^automake' | sed 's/.*) *//'`
case "$AMVER" in
1.11* | 1.1[2-9]*)
  ;;

*)

  cat >&2 <<_EOF_

	You must have automake 1.11 or later installed to compile $PROJECT.
	Download the appropriate package for your distribution/OS,
	or get the source tarball at ftp://ftp.gnu.org/pub/gnu/automake/
_EOF_
  DIE="exit 1"
  ;;
esac


#
# Apple's Developer Tools have a "libtool" that has nothing to do with
# the GNU libtool; they call the latter "glibtool".  They also call
# libtoolize "glibtoolize".
#
# Check for "glibtool" first.
#
LTVER=`glibtool --version 2>/dev/null | grep ' libtool)' | \
    sed 's/.*libtool) \([0-9][0-9.]*\)[^ ]* .*/\1/'`
if test -z "$LTVER"
then
	LTVER=`libtool --version | grep ' libtool)' | \
	    sed 's/.*) \([0-9][0-9.]*\)[^ ]* .*/\1/' `
	LIBTOOLIZE=libtoolize
else
	LIBTOOLIZE=glibtoolize
fi
case "$LTVER" in
'' | 0.* | 1.* | 2.2 )

  cat >&2 <<_EOF_

	You must have libtool 2.2.2 or later installed to compile $PROJECT.
	Download the appropriate package for your distribution/OS,
	or get the source tarball at ftp://ftp.gnu.org/pub/gnu/libtool/
_EOF_
  DIE="exit 1"
  ;;
esac

#
# XXX - is there some minimum version for which we should be checking?
#
PCVER=`pkg-config --version`
if test -z "$PCVER"; then
  cat >&2 <<_EOF_

	You must have pkg-config installed to compile $PROJECT.
	Download the appropriate package for your distribution/OS,
	or get the source tarball at http://pkgconfig.freedesktop.org/releases/
_EOF_
  DIE="exit 1"
fi

$DIE

aclocal_flags="-I m4"
aclocalinclude="$ACLOCAL_FLAGS $aclocal_flags";
echo $ACLOCAL $aclocalinclude
$ACLOCAL $aclocalinclude || exit 1
LTARGS=" --copy --force"
echo $LIBTOOLIZE $LTARGS
$LIBTOOLIZE $LTARGS || exit 1
echo $AUTOHEADER
$AUTOHEADER || exit 1
echo $AUTOMAKE --add-missing --gnu $am_opt
$AUTOMAKE --add-missing --gnu $am_opt || exit 1
echo $AUTOCONF
$AUTOCONF || exit 1

if [ `uname -s` = Darwin ] ; then
    echo
    echo "To configure Wireshark on OS X, you will need to type:"
    echo "export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/X11/lib/pkgconfig"
    echo "before you can run configure."
fi

echo
echo "Now type \"./configure [options]\" and \"make\" to compile $PROJECT."
