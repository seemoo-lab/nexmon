/* config.h for gnulib argp (MSYS2 libargp 20260115, gnulib e93789db) on
 * Android / bionic (NDK, minSdkVersion 24).
 *
 * gnulib normally generates this via autoconf. bionic's feature set is
 * fixed and known, so the values are hard-coded here instead of running
 * a configure step inside ndk-build. Keep in sync with Android.mk.
 */

#ifndef NEXMON_LIBARGP_CONFIG_H
#define NEXMON_LIBARGP_CONFIG_H

#include <stddef.h>
#include <stdbool.h>
#include <stdalign.h>
#include <string.h>

#ifndef UCHAR_WIDTH
# define UCHAR_WIDTH 8
#endif
#ifndef UINTPTR_WIDTH
# define UINTPTR_WIDTH (8 * sizeof(uintptr_t))
#endif
#ifndef static_assert
# if defined(__cplusplus)
#  define static_assert(expr) static_assert(expr, #expr)
# else
#  define static_assert(expr) _Static_assert(expr, #expr)
# endif
#endif

#ifndef streq
static inline bool
streq (const char *s1, const char *s2)
{
  return strcmp (s1, s2) == 0;
}
#endif

#define _GL_CONFIG_H_INCLUDED 1
#define HAVE_CONFIG_H 1

#define _GL_INLINE_HEADER_BEGIN
#define _GL_INLINE_HEADER_END
#define _GL_INLINE static inline
#define _GL_EXTERN_INLINE extern inline
#define _GL_UNUSED __attribute__ ((__unused__))
#define _GL_ATTRIBUTE_PURE __attribute__ ((__pure__))
#define _GL_ATTRIBUTE_FORMAT(spec) __attribute__ ((__format__ spec))
#define _GL_ATTRIBUTE_SPEC_PRINTF_STANDARD printf
#define _GL_ATTRIBUTE_SPEC_PRINTF_SYSTEM printf
#define _GL_ATTRIBUTE_DEALLOC(f, n)
#define _GL_ATTRIBUTE_MALLOC
#define _GL_ATTRIBUTE_RETURNS_NONNULL

#define HAVE_UNISTD_H 1
#define HAVE_ALLOCA_H 1
#define HAVE_ALLOCA 1

#define ENABLE_NLS 0
#define GNULIB_TEXT_DOMAIN "argp"

#define HAVE_STRNDUP 1
#define HAVE_STRCASECMP 1
#define HAVE_SLEEP 1
#define HAVE_VSNPRINTF 1

#define HAVE_MEMPCPY 0
#define HAVE_STRCHRNUL 0
#define HAVE_RAWMEMCHR 0

#ifndef __getopt_argv_const
# define __getopt_argv_const const
#endif

#define HAVE_DECL_PROGRAM_INVOCATION_NAME 0
#define HAVE_DECL_PROGRAM_INVOCATION_SHORT_NAME 0

#define HAVE_DECL_FPUTS_UNLOCKED 0
#define HAVE_DECL_FWRITE_UNLOCKED 0
#define HAVE_DECL_PUTC_UNLOCKED 0
#define HAVE_DECL_FPUTC_UNLOCKED 0
#define HAVE_DECL_FGETS_UNLOCKED 0
#define HAVE_DECL_FREAD_UNLOCKED 0
#define HAVE_DECL_FEOF_UNLOCKED 0
#define HAVE_DECL_FERROR_UNLOCKED 0
#define HAVE_DECL_FFLUSH_UNLOCKED 0
#define HAVE_DECL_GETC_UNLOCKED 0
#define HAVE_DECL_GETCHAR_UNLOCKED 0
#define HAVE_DECL_PUTCHAR_UNLOCKED 0
#define HAVE_DECL_CLEARERR_UNLOCKED 0

char *strchrnul (const char *s, int c);
void *mempcpy (void *dest, const void *src, size_t n);
void *rawmemchr (const void *s, int c);

#endif /* NEXMON_LIBARGP_CONFIG_H */
