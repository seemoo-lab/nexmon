# exponentl.m4
# serial 8
dnl Copyright (C) 2007-2026 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl This file is offered as-is, without any warranty.
AC_DEFUN_ONCE([gl_LONG_DOUBLE_EXPONENT_LOCATION],
[
  AC_REQUIRE([gl_BIGENDIAN])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([where to find the exponent in a 'long double'],
    [gl_cv_cc_long_double_expbit0],
    [
      AC_RUN_IFELSE(
        [AC_LANG_SOURCE([[
#include <float.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#define NWORDS \
  ((sizeof (long double) + sizeof (unsigned int) - 1) / sizeof (unsigned int))
typedef union { long double value; unsigned int word[NWORDS]; }
        memory_long_double;
static unsigned int ored_words[NWORDS];
static unsigned int anded_words[NWORDS];
static void add_to_ored_words (long double *x)
{
  memory_long_double m;
  size_t i;
  /* Clear it first, in case
     sizeof (long double) < sizeof (memory_long_double).  */
  memset (&m, 0, sizeof (memory_long_double));
  m.value = *x;
  for (i = 0; i < NWORDS; i++)
    {
      ored_words[i] |= m.word[i];
      anded_words[i] &= m.word[i];
    }
}
int main ()
{
  static long double samples[5] = { 0.25L, 0.5L, 1.0L, 2.0L, 4.0L };
  size_t j;
  FILE *fp = fopen ("conftest.out", "w");
  if (fp == NULL)
    return 1;
  for (j = 0; j < NWORDS; j++)
    anded_words[j] = ~ (unsigned int) 0;
  for (j = 0; j < 5; j++)
    add_to_ored_words (&samples[j]);
  /* Remove bits that are common (e.g. if representation of the first mantissa
     bit is explicit).  */
  for (j = 0; j < NWORDS; j++)
    ored_words[j] &= ~anded_words[j];
  /* Now find the nonzero word.  */
  for (j = 0; j < NWORDS; j++)
    if (ored_words[j] != 0)
      break;
  if (j < NWORDS)
    {
      size_t i;
      for (i = j + 1; i < NWORDS; i++)
        if (ored_words[i] != 0)
          {
            fprintf (fp, "unknown");
            return (fclose (fp) != 0);
          }
      for (i = 0; ; i++)
        if ((ored_words[j] >> i) & 1)
          {
            fprintf (fp, "word %d bit %d", (int) j, (int) i);
            return (fclose (fp) != 0);
          }
    }
  fprintf (fp, "unknown");
  return (fclose (fp) != 0);
}
        ]])],
        [gl_cv_cc_long_double_expbit0=`cat conftest.out`],
        [gl_cv_cc_long_double_expbit0="unknown"],
        [
          dnl When cross-compiling, in general we don't know. It depends on the
          dnl ABI and compiler version. But we know the results for specific
          dnl CPUs.
          AC_REQUIRE([gl_LONG_DOUBLE_VS_DOUBLE])
          if test $HAVE_SAME_LONG_DOUBLE_AS_DOUBLE = 1; then
            gl_DOUBLE_EXPONENT_LOCATION
            gl_cv_cc_long_double_expbit0="$gl_cv_cc_double_expbit0"
            if test "$gl_cv_cc_double_expbit0" = unknown; then
              case "$host_cpu" in
                arm*)
                  # See the comments in exponentd.m4.
                  ;;
                aarch64 | sh4)
                  # little-endian IEEE 754 double-precision
                  gl_cv_cc_long_double_expbit0='word 1 bit 20'
                  ;;
                hppa*)
                  # big-endian IEEE 754 double-precision
                  gl_cv_cc_long_double_expbit0='word 0 bit 20'
                  ;;
                mips*)
                  AC_COMPILE_IFELSE(
                    [AC_LANG_PROGRAM([[
                       #if defined _MIPSEB /* equivalent: __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ */
                       int big;
                       #else
                       #error little
                       #endif
                       ]], [[]])
                    ],
                    [# big-endian IEEE 754 double-precision
                     gl_cv_cc_long_double_expbit0='word 0 bit 20'
                    ],
                    [# little-endian IEEE 754 double-precision
                     gl_cv_cc_long_double_expbit0='word 1 bit 20'
                    ])
                  ;;
              esac
            fi
          else
            case "$host_cpu" in
changequote(,)dnl
              i[34567]86 | x86_64 | ia64*)
changequote([,])dnl
                # 80-bits "extended precision"
                gl_cv_cc_long_double_expbit0='word 2 bit 0'
                ;;
              m68k*)
                # big-endian, 80-bits padded to 96 bits, non-IEEE exponent
                gl_cv_cc_long_double_expbit0='word 0 bit 16'
                ;;
              alpha* | aarch64 | loongarch64 | riscv32 | riscv64 | sh4)
                # little-endian IEEE 754 quadruple-precision
                gl_cv_cc_long_double_expbit0='word 3 bit 16'
                ;;
              s390* | sparc | sparc64)
                # big-endian IEEE 754 quadruple-precision
                gl_cv_cc_long_double_expbit0='word 0 bit 16'
                ;;
              arm*)
                AC_COMPILE_IFELSE(
                  [AC_LANG_PROGRAM([[
                     #if defined _ARMEL
                     int little;
                     #else
                     #error big
                     #endif
                     ]], [[]])
                  ],
                  [# little-endian IEEE 754 quadruple-precision
                   gl_cv_cc_long_double_expbit0='word 3 bit 16'
                  ],
                  [# big-endian IEEE 754 quadruple-precision
                   gl_cv_cc_long_double_expbit0='word 0 bit 16'
                  ])
                ;;
              mips*)
                AC_COMPILE_IFELSE(
                  [AC_LANG_PROGRAM([[
                     #if defined _MIPSEB /* equivalent: __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ */
                     int big;
                     #else
                     #error little
                     #endif
                     ]], [[]])
                  ],
                  [# big-endian IEEE 754 quadruple-precision
                   gl_cv_cc_long_double_expbit0='word 0 bit 16'
                  ],
                  [# little-endian IEEE 754 quadruple-precision
                   gl_cv_cc_long_double_expbit0='word 3 bit 16'
                  ])
                ;;
              powerpc64le)
                # little-endian double-double
                gl_cv_cc_long_double_expbit0='word 1 bit 20'
                ;;
              powerpc* | rs6000)
                # big-endian double-double
                gl_cv_cc_long_double_expbit0='word 0 bit 20'
                ;;
              *)
                gl_cv_cc_long_double_expbit0="unknown"
                ;;
            esac
          fi
        ])
      rm -f conftest.out
    ])
  case "$gl_cv_cc_long_double_expbit0" in
    word*bit*)
      word=`echo "$gl_cv_cc_long_double_expbit0" | sed -e 's/word //' -e 's/ bit.*//'`
      bit=`echo "$gl_cv_cc_long_double_expbit0" | sed -e 's/word.*bit //'`
      AC_DEFINE_UNQUOTED([LDBL_EXPBIT0_WORD], [$word],
        [Define as the word index where to find the exponent of 'long double'.])
      AC_DEFINE_UNQUOTED([LDBL_EXPBIT0_BIT], [$bit],
        [Define as the bit index in the word where to find bit 0 of the exponent of 'long double'.])
      ;;
  esac
])
