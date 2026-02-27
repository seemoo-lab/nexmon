#!/bin/sh

# Test copy-file on the file system of /var/tmp, which usually is a local
# file system.

if test -d /var/tmp; then
  TMPDIR=/var/tmp
else
  TMPDIR=/tmp
fi
export TMPDIR

"${srcdir}/test-copy-file.sh"
ret1=$?
NO_STDERR_OUTPUT=1 "${srcdir}/test-copy-file.sh"
ret2=$?
case $ret1 in
  77 ) exit $ret2 ;;
  * ) exit $ret1 ;;
esac
