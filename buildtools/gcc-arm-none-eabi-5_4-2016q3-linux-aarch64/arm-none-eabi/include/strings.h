/*
 * strings.h
 *
 * Definitions for string operations.
 */

#ifndef _STRINGS_H_
#define _STRINGS_H_

/* These functions are already declared in <string.h> with __BSD_VISIBLE */
#if !(defined(_STRING_H_) && __BSD_VISIBLE)

#include "_ansi.h"
#include <sys/reent.h>
#include <sys/cdefs.h>
#include <sys/types.h> /* for size_t */

#if __POSIX_VISIBLE >= 200809
#include <sys/_locale.h>
#endif

_BEGIN_STD_C

#if __BSD_VISIBLE || (__POSIX_VISIBLE && __POSIX_VISIBLE < 200809)
/* 
 * Marked LEGACY in Open Group Base Specifications Issue 6/IEEE Std 1003.1-2004
 * Removed from Open Group Base Specifications Issue 7/IEEE Std 1003.1-2008
 */
int	 _EXFUN(bcmp,(const void *, const void *, size_t));
void	 _EXFUN(bcopy,(const void *, void *, size_t));
void	 _EXFUN(bzero,(void *, size_t));
char 	*_EXFUN(index,(const char *, int));
char 	*_EXFUN(rindex,(const char *, int));
#endif /* __BSD_VISIBLE || (__POSIX_VISIBLE && __POSIX_VISIBLE < 200809) */

int	 _EXFUN(ffs,(int));
int	 _EXFUN(strcasecmp,(const char *, const char *));
int	 _EXFUN(strncasecmp,(const char *, const char *, size_t));

#if __POSIX_VISIBLE >= 200809
int	 strcasecmp_l (const char *, const char *, locale_t);
int	 strncasecmp_l (const char *, const char *, size_t, locale_t);
#endif /* __POSIX_VISIBLE >= 200809 */

_END_STD_C

#endif /* !(_STRING_H_ && __BSD_VISIBLE) */

#endif /* _STRINGS_H_ */
