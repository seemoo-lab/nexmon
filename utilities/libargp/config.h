/* config.h for argp-standalone 1.5.0 on Android / bionic (NDK, minSdkVersion 21).
 *
 * argp-standalone normally generates this via autoconf/meson. bionic's feature
 * set is fixed and known, so the values are hard-coded here instead of running
 * a configure step inside ndk-build. Keep in sync with Android.mk.
 */

#ifndef NEXMON_LIBARGP_CONFIG_H
#define NEXMON_LIBARGP_CONFIG_H

/* Headers bionic provides. */
#define HAVE_UNISTD_H 1
#define HAVE_ALLOCA_H 1

/* bionic has no <libintl.h>; argp falls back to passing messages through
 * untranslated (HAVE_LIBINTL_H intentionally left undefined). */

/* Functions bionic always provides -> do not build the bundled copies. */
#define HAVE_STRNDUP 1
#define HAVE_STRCASECMP 1

/* Not exposed at API 21 (strchrnul: API 24, mempcpy: API 23) -> the bundled
 * strchrnul.c / mempcpy.c are compiled and declared by argp-namefrob.h. */
#define HAVE_MEMPCPY 0
#define HAVE_STRCHRNUL 0

/* bionic has no program_invocation_name / program_invocation_short_name. */
#define HAVE_DECL_PROGRAM_INVOCATION_NAME 0
#define HAVE_DECL_PROGRAM_INVOCATION_SHORT_NAME 0

/* Route the *_unlocked stdio calls argp-help.c makes to the locked variants;
 * portable across every API level regardless of when bionic added each one. */
#define HAVE_DECL_FPUTS_UNLOCKED 0
#define HAVE_DECL_FWRITE_UNLOCKED 0
#define HAVE_DECL_PUTC_UNLOCKED 0

/* Attribute helpers argp-standalone expects from its generated config.h
 * (argp-parse.c uses UNUSED unguarded; the others have their own fallbacks). */
#define HAVE_GCC_ATTRIBUTE 1
#if defined(__GNUC__)
# define NORETURN __attribute__ ((__noreturn__))
# define PRINTF_STYLE(f, a) __attribute__ ((__format__ (__printf__, f, a)))
# define UNUSED __attribute__ ((__unused__))
#else
# define NORETURN
# define PRINTF_STYLE(f, a)
# define UNUSED
#endif

#endif /* NEXMON_LIBARGP_CONFIG_H */
