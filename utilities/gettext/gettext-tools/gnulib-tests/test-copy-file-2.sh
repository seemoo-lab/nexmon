#!/bin/sh

# Test copy-file on the file system of the build directory, which may be
# a local file system or NFS mounted.

TMPDIR=`pwd`
export TMPDIR

"${srcdir}/test-copy-file.sh"
ret1=$?
NO_STDERR_OUTPUT=1 "${srcdir}/test-copy-file.sh"
ret2=$?
case $ret1 in
  77 ) exit $ret2 ;;
  * ) exit $ret1 ;;
esac
