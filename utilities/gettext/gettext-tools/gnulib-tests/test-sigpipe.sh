#!/bin/sh

tmpfiles=""
trap 'rm -fr $tmpfiles' 1 2 3 15

# Test signal's default behaviour.
tmpfiles="$tmpfiles t-sigpipeA.tmp"
./test-sigpipe${EXEEXT} A 2> t-sigpipeA.tmp | head -1 > /dev/null
if test -s t-sigpipeA.tmp; then
  LC_ALL=C tr -d '\r' < t-sigpipeA.tmp
  rm -fr $tmpfiles; exit 1
fi

# Test signal's ignored behaviour.
tmpfiles="$tmpfiles t-sigpipeB.tmp"
./test-sigpipe${EXEEXT} B 2> t-sigpipeB.tmp | head -1 > /dev/null
if test -s t-sigpipeB.tmp; then
  LC_ALL=C tr -d '\r' < t-sigpipeB.tmp
  rm -fr $tmpfiles; exit 1
fi

# Test signal's behaviour when a handler is installed.
tmpfiles="$tmpfiles t-sigpipeC.tmp"
./test-sigpipe${EXEEXT} B 2> t-sigpipeC.tmp | head -1 > /dev/null
if test -s t-sigpipeC.tmp; then
  LC_ALL=C tr -d '\r' < t-sigpipeC.tmp
  rm -fr $tmpfiles; exit 1
fi

rm -fr $tmpfiles
exit 0
