/*
 * string.h
 *
 * Definitions for memory and string functions.
 */

#ifndef _STRING_H_
#define	_STRING_H_

#include "_ansi.h"
#include <sys/reent.h>
#include <sys/cdefs.h>
#include <sys/features.h>

#define __need_size_t
#define __need_NULL
#include <stddef.h>

#if __POSIX_VISIBLE >= 200809
#include <sys/_locale.h>
#endif

_BEGIN_STD_C

_PTR 	 _EXFUN(memchr,(const _PTR, int, size_t));
int 	 _EXFUN(memcmp,(const _PTR, const _PTR, size_t));
_PTR 	 _EXFUN(memcpy,(_PTR __restrict, const _PTR __restrict, size_t));
_PTR	 _EXFUN(memmove,(_PTR, const _PTR, size_t));
_PTR	 _EXFUN(memset,(_PTR, int, size_t));
char 	*_EXFUN(strcat,(char *__restrict, const char *__restrict));
char 	*_EXFUN(strchr,(const char *, int));
int	 _EXFUN(strcmp,(const char *, const char *));
int	 _EXFUN(strcoll,(const char *, const char *));
char 	*_EXFUN(strcpy,(char *__restrict, const char *__restrict));
size_t	 _EXFUN(strcspn,(const char *, const char *));
char 	*_EXFUN(strerror,(int));
size_t	 _EXFUN(strlen,(const char *));
char 	*_EXFUN(strncat,(char *__restrict, const char *__restrict, size_t));
int	 _EXFUN(strncmp,(const char *, const char *, size_t));
char 	*_EXFUN(strncpy,(char *__restrict, const char *__restrict, size_t));
char 	*_EXFUN(strpbrk,(const char *, const char *));
char 	*_EXFUN(strrchr,(const char *, int));
size_t	 _EXFUN(strspn,(const char *, const char *));
char 	*_EXFUN(strstr,(const char *, const char *));
#ifndef _REENT_ONLY
char 	*_EXFUN(strtok,(char *__restrict, const char *__restrict));
#endif
size_t	 _EXFUN(strxfrm,(char *__restrict, const char *__restrict, size_t));

#if __POSIX_VISIBLE >= 200809
int	 strcoll_l (const char *, const char *, locale_t);
char	*strerror_l (int, locale_t);
size_t	 strxfrm_l (char *__restrict, const char *__restrict, size_t, locale_t);
#endif
#if __GNU_VISIBLE
int	 strcasecmp_l (const char *, const char *, locale_t);
int	 strncasecmp_l (const char *, const char *, size_t, locale_t);
#endif
#if __MISC_VISIBLE || __POSIX_VISIBLE
char 	*_EXFUN(strtok_r,(char *__restrict, const char *__restrict, char **__restrict));
#endif
#if __BSD_VISIBLE /* POSIX declaration is in <strings.h> */
int	 _EXFUN(bcmp,(const void *, const void *, size_t));
void	 _EXFUN(bcopy,(const void *, void *, size_t));
void	 _EXFUN(bzero,(void *, size_t));
#endif
#if __BSD_VISIBLE
void	 _EXFUN(explicit_bzero,(void *, size_t));
int	 _EXFUN(timingsafe_bcmp,(const void *, const void *, size_t));
int	 _EXFUN(timingsafe_memcmp,(const void *, const void *, size_t));
#endif
#if __BSD_VISIBLE /* POSIX declaration is in <strings.h> */
int	 _EXFUN(ffs,(int));
char 	*_EXFUN(index,(const char *, int));
#endif
#if __MISC_VISIBLE || __POSIX_VISIBLE
_PTR	 _EXFUN(memccpy,(_PTR __restrict, const _PTR __restrict, int, size_t));
#endif
#if __GNU_VISIBLE
_PTR	 _EXFUN(mempcpy,(_PTR, const _PTR, size_t));
_PTR	 _EXFUN(memmem, (const _PTR, size_t, const _PTR, size_t));
_PTR 	 _EXFUN(memrchr,(const _PTR, int, size_t));
_PTR 	 _EXFUN(rawmemchr,(const _PTR, int));
#endif
#if __BSD_VISIBLE /* POSIX declaration is in <strings.h> */
char 	*_EXFUN(rindex,(const char *, int));
#endif
#if __POSIX_VISIBLE >= 200809
char 	*_EXFUN(stpcpy,(char *__restrict, const char *__restrict));
char 	*_EXFUN(stpncpy,(char *__restrict, const char *__restrict, size_t));
#endif
#if __BSD_VISIBLE /* POSIX declaration is in <strings.h> */
int	 _EXFUN(strcasecmp,(const char *, const char *));
#endif
#if __GNU_VISIBLE
char	*_EXFUN(strcasestr,(const char *, const char *));
char 	*_EXFUN(strchrnul,(const char *, int));
#endif
#if __MISC_VISIBLE || __POSIX_VISIBLE >= 200809 || __XSI_VISIBLE >= 4
char 	*_EXFUN(strdup,(const char *));
#endif
char 	*_EXFUN(_strdup_r,(struct _reent *, const char *));
#if __POSIX_VISIBLE >= 200809
char 	*_EXFUN(strndup,(const char *, size_t));
#endif
char 	*_EXFUN(_strndup_r,(struct _reent *, const char *, size_t));

#if __GNU_VISIBLE
int	 _EXFUN(ffsl,(long));
int	 _EXFUN(ffsll, (long long));
#endif

/* There are two common strerror_r variants.  If you request
   _GNU_SOURCE, you get the GNU version; otherwise you get the POSIX
   version.  POSIX requires that #undef strerror_r will still let you
   invoke the underlying function, but that requires gcc support.  */
#if __GNU_VISIBLE
char	*_EXFUN(strerror_r,(int, char *, size_t));
#elif __POSIX_VISIBLE >= 200112
# ifdef __GNUC__
int	_EXFUN(strerror_r,(int, char *, size_t))
#ifdef __ASMNAME
             __asm__ (__ASMNAME ("__xpg_strerror_r"))
#endif
  ;
# else
int	_EXFUN(__xpg_strerror_r,(int, char *, size_t));
#  define strerror_r __xpg_strerror_r
# endif
#endif

/* Reentrant version of strerror.  */
char *	_EXFUN(_strerror_r, (struct _reent *, int, int, int *));

#if __BSD_VISIBLE
size_t	_EXFUN(strlcat,(char *, const char *, size_t));
size_t	_EXFUN(strlcpy,(char *, const char *, size_t));
#endif
#if __BSD_VISIBLE /* POSIX declaration is in <strings.h> */
int	_EXFUN(strncasecmp,(const char *, const char *, size_t));
#endif
#if __POSIX_VISIBLE >= 200809
size_t	 _EXFUN(strnlen,(const char *, size_t));
#endif
#if __BSD_VISIBLE
char 	*_EXFUN(strsep,(char **, const char *));
#endif

#if __MISC_VISIBLE
char	*_EXFUN(strlwr,(char *));
char	*_EXFUN(strupr,(char *));
#endif

#ifndef DEFS_H	/* Kludge to work around problem compiling in gdb */
char	*_EXFUN(strsignal, (int __signo));
#endif

#ifdef __CYGWIN__
int	_EXFUN(strtosigno, (const char *__name));
#endif

#if __GNU_VISIBLE && defined(__GNUC__)
#define strdupa(__s) \
	(__extension__ ({const char *__in = (__s); \
			 size_t __len = strlen (__in) + 1; \
			 char * __out = (char *) __builtin_alloca (__len); \
			 (char *) memcpy (__out, __in, __len);}))
#define strndupa(__s, __n) \
	(__extension__ ({const char *__in = (__s); \
			 size_t __len = strnlen (__in, (__n)) + 1; \
			 char *__out = (char *) __builtin_alloca (__len); \
			 __out[__len-1] = '\0'; \
			 (char *) memcpy (__out, __in, __len-1);}))
#endif /* __GNU_VISIBLE && __GNUC__ */

/* There are two common basename variants.  If you do NOT #include <libgen.h>
   and you do

     #define _GNU_SOURCE
     #include <string.h>

   you get the GNU version.  Otherwise you get the POSIX versionfor which you
   should #include <libgen.h>i for the function prototype.  POSIX requires that
   #undef basename will still let you invoke the underlying function.  However,
   this also implies that the POSIX version is used in this case.  That's made
   sure here. */
#if __GNU_VISIBLE && !defined(basename)
# define basename basename
char	*_EXFUN(__nonnull (1) basename,(const char *)) __asm__(__ASMNAME("__gnu_basename"));
#endif

#include <sys/string.h>

_END_STD_C

#endif /* _STRING_H_ */
