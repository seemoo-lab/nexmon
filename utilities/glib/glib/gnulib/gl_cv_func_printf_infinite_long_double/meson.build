# Copyright (C) 2002-2004, 2006-2018 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Test whether the *printf family of functions supports infinite and NaN
# 'long double' arguments in the %f, %e, %g directives. (ISO C99, POSIX:2001)
# Result is gl_cv_func_printf_infinite_long_double.

# The user can set or unset the variable gl_printf_safe to indicate
# that he wishes a safe handling of non-IEEE-754 'long double' values.
check_print_safe = ''
if get_variable ('gl_printf_safe', false)
  check_print_safe = '''
#if ((defined __ia64 && LDBL_MANT_DIG == 64) || (defined __x86_64__ || defined __amd64__) || (defined __i386 || defined __i386__ || defined _I386 || defined _M_IX86 || defined _X86_)) && !HAVE_SAME_LONG_DOUBLE_AS_DOUBLE
/* Representation of an 80-bit 'long double' as an initializer for a sequence
   of 'unsigned int' words.  */
# ifdef WORDS_BIGENDIAN
#  define LDBL80_WORDS(exponent,manthi,mantlo) \
     { ((unsigned int) (exponent) << 16) | ((unsigned int) (manthi) >> 16), \
       ((unsigned int) (manthi) << 16) | ((unsigned int) (mantlo) >> 16),   \
       (unsigned int) (mantlo) << 16                                        \
     }
# else
#  define LDBL80_WORDS(exponent,manthi,mantlo) \
     { mantlo, manthi, exponent }
# endif
  { /* Quiet NaN.  */
    static union { unsigned int word[4]; long double value; } x =
      { LDBL80_WORDS (0xFFFF, 0xC3333333, 0x00000000) };
    if (sprintf (buf, "%Lf", x.value) < 0
        || !strisnan (buf, 0, strlen (buf)))
      result |= 2;
    if (sprintf (buf, "%Le", x.value) < 0
        || !strisnan (buf, 0, strlen (buf)))
      result |= 2;
    if (sprintf (buf, "%Lg", x.value) < 0
        || !strisnan (buf, 0, strlen (buf)))
      result |= 2;
  }
  {
    /* Signalling NaN.  */
    static union { unsigned int word[4]; long double value; } x =
      { LDBL80_WORDS (0xFFFF, 0x83333333, 0x00000000) };
    if (sprintf (buf, "%Lf", x.value) < 0
        || !strisnan (buf, 0, strlen (buf)))
      result |= 2;
    if (sprintf (buf, "%Le", x.value) < 0
        || !strisnan (buf, 0, strlen (buf)))
      result |= 2;
    if (sprintf (buf, "%Lg", x.value) < 0
        || !strisnan (buf, 0, strlen (buf)))
      result |= 2;
  }
  { /* Pseudo-NaN.  */
    static union { unsigned int word[4]; long double value; } x =
      { LDBL80_WORDS (0xFFFF, 0x40000001, 0x00000000) };
    if (sprintf (buf, "%Lf", x.value) <= 0)
      result |= 4;
    if (sprintf (buf, "%Le", x.value) <= 0)
      result |= 4;
    if (sprintf (buf, "%Lg", x.value) <= 0)
      result |= 4;
  }
  { /* Pseudo-Infinity.  */
    static union { unsigned int word[4]; long double value; } x =
      { LDBL80_WORDS (0xFFFF, 0x00000000, 0x00000000) };
    if (sprintf (buf, "%Lf", x.value) <= 0)
      result |= 8;
    if (sprintf (buf, "%Le", x.value) <= 0)
      result |= 8;
    if (sprintf (buf, "%Lg", x.value) <= 0)
      result |= 8;
  }
  { /* Pseudo-Zero.  */
    static union { unsigned int word[4]; long double value; } x =
      { LDBL80_WORDS (0x4004, 0x00000000, 0x00000000) };
    if (sprintf (buf, "%Lf", x.value) <= 0)
      result |= 16;
    if (sprintf (buf, "%Le", x.value) <= 0)
      result |= 16;
    if (sprintf (buf, "%Lg", x.value) <= 0)
      result |= 16;
  }
  { /* Unnormalized number.  */
    static union { unsigned int word[4]; long double value; } x =
      { LDBL80_WORDS (0x4000, 0x63333333, 0x00000000) };
    if (sprintf (buf, "%Lf", x.value) <= 0)
      result |= 32;
    if (sprintf (buf, "%Le", x.value) <= 0)
      result |= 32;
    if (sprintf (buf, "%Lg", x.value) <= 0)
      result |= 32;
  }
  { /* Pseudo-Denormal.  */
    static union { unsigned int word[4]; long double value; } x =
      { LDBL80_WORDS (0x0000, 0x83333333, 0x00000000) };
    if (sprintf (buf, "%Lf", x.value) <= 0)
      result |= 64;
    if (sprintf (buf, "%Le", x.value) <= 0)
      result |= 64;
    if (sprintf (buf, "%Lg", x.value) <= 0)
      result |= 64;
  }
#endif
  '''
endif

printf_infinite_long_double_test = '''
#include <float.h>
#include <stdio.h>
#include <string.h>
static int
strisnan (const char *string, size_t start_index, size_t end_index)
{
  if (start_index < end_index)
    {
      if (string[start_index] == '-')
        start_index++;
      if (start_index + 3 <= end_index
          && memcmp (string + start_index, "nan", 3) == 0)
        {
          start_index += 3;
          if (start_index == end_index
              || (string[start_index] == '(' && string[end_index - 1] == ')'))
            return 1;
        }
    }
  return 0;
}
static char buf[10000];
static long double zeroL = 0.0L;
int main ()
{
  int result = 0;
  nocrash_init();
  if (sprintf (buf, "%Lf", 1.0L / zeroL) < 0
      || (strcmp (buf, "inf") != 0 && strcmp (buf, "infinity") != 0))
    result |= 1;
  if (sprintf (buf, "%Lf", -1.0L / zeroL) < 0
      || (strcmp (buf, "-inf") != 0 && strcmp (buf, "-infinity") != 0))
    result |= 1;
  if (sprintf (buf, "%Lf", zeroL / zeroL) < 0
      || !strisnan (buf, 0, strlen (buf)))
    result |= 1;
  if (sprintf (buf, "%Le", 1.0L / zeroL) < 0
      || (strcmp (buf, "inf") != 0 && strcmp (buf, "infinity") != 0))
    result |= 1;
  if (sprintf (buf, "%Le", -1.0L / zeroL) < 0
      || (strcmp (buf, "-inf") != 0 && strcmp (buf, "-infinity") != 0))
    result |= 1;
  if (sprintf (buf, "%Le", zeroL / zeroL) < 0
      || !strisnan (buf, 0, strlen (buf)))
    result |= 1;
  if (sprintf (buf, "%Lg", 1.0L / zeroL) < 0
      || (strcmp (buf, "inf") != 0 && strcmp (buf, "infinity") != 0))
    result |= 1;
  if (sprintf (buf, "%Lg", -1.0L / zeroL) < 0
      || (strcmp (buf, "-inf") != 0 && strcmp (buf, "-infinity") != 0))
    result |= 1;
  if (sprintf (buf, "%Lg", zeroL / zeroL) < 0
      || !strisnan (buf, 0, strlen (buf)))
    result |= 1;
''' + check_print_safe + '''
  return result;
}
'''

if gl_cv_func_printf_long_double
  if meson.can_run_host_binaries()
    run_result = cc.run(printf_infinite_long_double_test,
        name : 'printf supports infinite \'long double\' arguments')
    gl_cv_func_printf_infinite_long_double = (run_result.compiled() and run_result.returncode() == 0)
  else
    if host_machine.cpu_family() == 'x86_64'
      gl_cv_func_printf_infinite_long_double = false
    else
      if host_system in ['linux', 'android']
        gl_cv_func_printf_infinite_long_double = true
      elif (host_system.startswith ('freebsd1') or
            host_system.startswith ('freebsd2') or
            host_system.startswith ('freebsd3') or
            host_system.startswith ('freebsd4') or
            host_system.startswith ('freebsd5'))
        gl_cv_func_printf_infinite_long_double = false
      elif (host_system.startswith ('freebsd') or
            host_system.startswith ('kfreebsd'))
        gl_cv_func_printf_infinite_long_double = true
      elif (host_system.startswith ('hpux7') or
            host_system.startswith ('hpux8') or
            host_system.startswith ('hpux9') or
            host_system.startswith ('hpux10'))
        gl_cv_func_printf_infinite = false
      elif host_system.startswith ('hpux')
        gl_cv_func_printf_infinite_long_double = true
      elif host_system == 'windows'
        # Guess yes on MSVC, no on mingw.
        if cc.get_id() == 'msvc' or cc.get_id() == 'clang-cl'
          gl_cv_func_printf_infinite_long_double = true
        else
          gl_cv_func_printf_infinite_long_double = false
        endif
      else
        gl_cv_func_printf_infinite_long_double = false
      endif
    endif
  endif
endif
