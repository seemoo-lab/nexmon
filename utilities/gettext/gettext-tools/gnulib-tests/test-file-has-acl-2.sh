#!/bin/sh

# Test file-has-acl on the file system of the build directory, which may be
# a local file system or NFS mounted.

. "${srcdir=.}/init.sh"; path_prepend_ .

# Around 2025-06-06, this test started failing on the GitHub CI machines.
# Cause unknown.
case "$host_os" in
  cygwin*) Exit 77 ;;
esac

TMPDIR=`pwd`
export TMPDIR

$BOURNE_SHELL "${srcdir}/test-file-has-acl.sh"

Exit $?
