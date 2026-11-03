AC_DEFUN([ISL_DETECT_DIFF_OPTIONS], [
	AC_SUBST([DIFF_OPTIONS])
	AC_PATH_PROG([DIFF], [diff])

	AC_MSG_CHECKING([for $DIFF context option])
	for opt in -u -c; do
		if $DIFF $opt /dev/null /dev/null > /dev/null 2>&1; then
			DIFF_OPTIONS=$opt
			break
		fi
	done
	AC_MSG_RESULT([$DIFF_OPTIONS])
])
