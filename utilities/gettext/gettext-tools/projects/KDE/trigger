#!/bin/sh
# Test whether the current package is a KDE package.

# NLS nuisances: Letter ranges are different in the Estonian locale.
LC_ALL=C

while true; do
  configfiles=
  if test -f configure.in; then
    configfiles="$configfiles configure.in"
  fi
  if test -f configure.ac; then
    configfiles="$configfiles configure.ac"
  fi
  if test -n "$configfiles"; then
    if grep '^KDE_' $configfiles >/dev/null 2>&1 || \
       grep '^AC_PATH_KDE' $configfiles >/dev/null 2>&1 || \
       grep '^AM_KDE_WITH_NLS' $configfiles >/dev/null 2>&1 ; then
      exit 0
    fi
    exit 1
  fi
  dir=`basename \`pwd\``
  case "$dir" in
    i18n)
      # This directory name, used in GNU make, is not the top level directory.
      ;;
    *[A-Za-z]*[0-9]*)
      # Reached the top level directory.
      exit 1
  esac
  # Go to parent directory
  last=`/bin/pwd`
  cd ..
  curr=`/bin/pwd`
  if test "$last" = "$curr"; then
    # Oops, didn't find the top level directory.
    exit 1
  fi
done
