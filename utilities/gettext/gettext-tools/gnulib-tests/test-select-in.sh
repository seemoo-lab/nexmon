#!/bin/sh
# Test select() on file descriptors opened for reading.

# This test is known to fail on Solaris 2.6 and older, due to its handling
# of /dev/null.

tmpfiles=""
trap 'rm -fr $tmpfiles' HUP INT QUIT TERM

tmpfiles="$tmpfiles t-select-in.tmp"

# Regular files.

rm -f t-select-in.tmp
${CHECKER} ./test-select-fd${EXEEXT} r 0 t-select-in.tmp < ./test-select-fd${EXEEXT}
test `cat t-select-in.tmp` = "1" || exit 1

# Pipes.

rm -f t-select-in.tmp
{ sleep 1; echo abc; } | \
  { ${CHECKER} ./test-select-fd${EXEEXT} r 0 t-select-in.tmp; cat > /dev/null; }
test `cat t-select-in.tmp` = "0" || exit 1

rm -f t-select-in.tmp
echo abc | { sleep 1; ${CHECKER} ./test-select-fd${EXEEXT} r 0 t-select-in.tmp; }
test `cat t-select-in.tmp` = "1" || exit 1

# Special files.
# This part of the test is known to fail on Solaris 2.6 and older.
# Also it fails in Cygwin 3.6.1, due to a Cygwin regression.
# <https://sourceware.org/pipermail/cygwin/2025-April/257940.html>
# <https://sourceware.org/pipermail/cygwin/2025-April/257952.html>
case `uname -s` in
  CYGWIN*)
    case `uname -r` in
      3.6.*)
        echo "Skipping test: known Cygwin 3.6.x bug"
        exit 77;;
    esac
    ;;
esac
rm -f t-select-in.tmp
${CHECKER} ./test-select-fd${EXEEXT} r 0 t-select-in.tmp < /dev/null
test `cat t-select-in.tmp` = "1" || exit 1

rm -fr $tmpfiles

exit 0
