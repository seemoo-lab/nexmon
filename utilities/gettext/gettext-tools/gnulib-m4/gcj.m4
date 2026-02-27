# gcj.m4 serial 2 (gettext-0.17)
dnl Copyright (C) 2002, 2006, 2015-2016 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# Check for a Java compiler that creates executables.
# Assigns the variables GCJ and GCJFLAGS, and set HAVE_GCJ to nonempty,
# if found. Otherwise sets HAVE_GCJ to empty.

AC_DEFUN([gt_GCJ],
[
  AC_ARG_VAR([GCJ], [Java native code compiler command])
  AC_ARG_VAR([GCJFLAGS], [Java native code compiler flags])

  AC_MSG_CHECKING([for Java to native code compiler])
  # Search for the gcj command or use the one provided by the user.
  if test -z "$GCJ"; then
    pushdef([AC_MSG_CHECKING],[:])dnl
    pushdef([AC_CHECKING],[:])dnl
    pushdef([AC_MSG_RESULT],[:])dnl
    AC_CHECK_TOOL([GCJ], [gcj], [none])
    popdef([AC_MSG_RESULT])dnl
    popdef([AC_CHECKING])dnl
    popdef([AC_MSG_CHECKING])dnl
  fi
  # Choose GCJFLAGS or use the one provided by the user.
  if test "$GCJ" != none; then
    test "${GCJFLAGS+set}" != set || GCJFLAGS="-O2 -g"
  fi
  # Check whether the version is ok and it can create executables.
  ac_gcj_link="$GCJ $GCJFLAGS conftest.java --main=conftest -o conftest$ac_exeext"
changequote(,)dnl
  if test "$GCJ" != none \
     && $GCJ --version 2>/dev/null | sed -e 's,^[^0-9]*,,' -e 1q | grep '^[3-9]' >/dev/null \
     && (
      # See if libgcj.so is well installed and if exception handling works.
      cat > conftest.java <<EOF
public class conftest {
  public static void main (String[] args) {
    try {
      java.util.ResourceBundle.getBundle("foobar");
    } catch (Exception e) {
    }
    System.exit(0);
  }
}
EOF
changequote([,])dnl
      AC_TRY_EVAL([ac_gcj_link])
      error=$?
      if test $error = 0 && test "$cross_compiling" != yes; then
        # Run conftest and catch its exit status, but silently.
        error=`./conftest >/dev/null 2>&1; echo $?`
        test $error = 0 || error=1
        rm -f core conftest.core
      fi
      rm -f conftest.java conftest$ac_exeext
      exit $error
     ); then
    :
  else
    GCJ=none
  fi
  AC_MSG_RESULT($GCJ)
  if test "$GCJ" != none; then
    HAVE_GCJ=1
  else
    HAVE_GCJ=
  fi
  AC_SUBST(GCJ)
  AC_SUBST(GCJFLAGS)
  AC_SUBST(HAVE_GCJ)
])
