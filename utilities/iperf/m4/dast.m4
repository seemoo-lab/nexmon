
AH_TEMPLATE([HAVE_POSIX_THREAD], [])
AH_TEMPLATE([_REENTRANT], [])
AH_TEMPLATE([ssize_t], [Define to "int" if <sys/types.h> does not define.])

dnl DAST_CHECK_ARG
dnl Check for the 3rd arguement to accept

AC_DEFUN([DAST_ACCEPT_ARG], [
  if test -z "$ac_cv_accept_arg" ; then
    AC_LANG_PUSH([C++])

    AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
      [[$ac_includes_default
       #include <sys/socket.h>]],
      [[$1 length;
       accept( 0, 0, &length );]])],
    ac_cv_accept_arg=$1)

    AC_LANG_POP([C++])
  fi
])




AC_DEFUN([DAST_SOCKLEN_T], [
  AC_CACHE_CHECK(3rd argument of accept, ac_cv_accept_arg, [
    dnl Try socklen_t (POSIX)
    DAST_ACCEPT_ARG(socklen_t)

    dnl Try int (original BSD)
    DAST_ACCEPT_ARG(int)

    dnl Try size_t (older standard; AIX)
    DAST_ACCEPT_ARG(size_t)

    dnl Try short (shouldn't be)
    DAST_ACCEPT_ARG(short)

    dnl Try long (shouldn't be)
    DAST_ACCEPT_ARG(long)
  ])

  if test -z "$ac_cv_accept_arg" ; then
    ac_cv_accept_arg=int
  fi

  AC_DEFINE_UNQUOTED([Socklen_t], $ac_cv_accept_arg, [Define 3rd arg of accept])
])
