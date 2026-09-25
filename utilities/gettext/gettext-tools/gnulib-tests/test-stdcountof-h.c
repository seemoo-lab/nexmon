/* Test <stdcountof.h>.
   Copyright 2025-2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

#include <stdcountof.h>

#include "macros.h"

/* Whether the compiler supports _Generic.
   Test program:
     int f (int x) { return _Generic (x, char *: 2, int: 3); }
 */
#if (defined __GNUC__ && __GNUC__ + (__GNUC_MINOR__ >= 9) > 4) \
    || (defined __clang__ && __clang_major__ >= 3) \
    || (defined __SUNPRO_C && __SUNPRO_C >= 0x5150)
# define HAVE__GENERIC 1
#endif

extern int integer;
extern int unbounded[];
extern int bounded[10];
extern int multidimensional[10][20];
extern int a, b, c;

static void
test_func (int parameter[3])
{
  int local_bounded[20];

  (void) local_bounded;

#ifdef _gl_verify_is_array
  (void) _gl_verify_is_array (unbounded);
  (void) _gl_verify_is_array (bounded);
  (void) _gl_verify_is_array (multidimensional);
  (void) _gl_verify_is_array ("string");
  (void) _gl_verify_is_array (L"wide string");
# if ((__STDC_VERSION__ >= 201112L || __cplusplus >= 201103L) \
      && !defined __SUNPRO_C)
  (void) _gl_verify_is_array (u8"UTF-8 string");
  (void) _gl_verify_is_array (u"UTF-16 string");
  (void) _gl_verify_is_array (U"UTF-32 string");
# endif
#endif

  ASSERT (countof (bounded) == 10);
  ASSERT (countof (multidimensional) == 10);
  ASSERT (countof (local_bounded) == 20);
  ASSERT (countof ("string") == 6 + 1);
  ASSERT (countof (L"wide string") == 11 + 1);
# if ((__STDC_VERSION__ >= 201112L || __cplusplus >= 201103L) \
      && !defined __SUNPRO_C)
  ASSERT (countof (u8"UTF-8 string") == 12 + 1);
  ASSERT (countof (u"UTF-16 string") == 13 + 1);
  ASSERT (countof (U"UTF-32 string") == 13 + 1);
#endif

#if 0 /* These produce compilation errors.  */
# ifdef _gl_verify_is_array
  (void) _gl_verify_is_array (integer);
  (void) _gl_verify_is_array (parameter);
# endif

  ASSERT (countof (integer) >= 0);
  ASSERT (countof (unbounded) >= 0);
#endif

  /* Avoid MSVC C++ error C4576 "a parenthesized type followed by an
     initializer list is a non-standard explicit type conversion syntax".  */
#if !defined __cplusplus
  ASSERT (countof ((int[]) { a, b, c }) == 3);
  ASSERT (countof (((int[]) { a, b, c })) == 3);
#endif

  /* Check that countof(...) is an expression of type size_t.  */
#if !defined __cplusplus && HAVE__GENERIC
  ASSERT (_Generic (countof (bounded),          size_t: 1, default: 0));
  ASSERT (_Generic (countof (multidimensional), size_t: 1, default: 0));
  ASSERT (_Generic (countof (local_bounded),    size_t: 1, default: 0));
  ASSERT (_Generic (countof ("string"),         size_t: 1, default: 0));
  ASSERT (_Generic (countof (L"wide string"),   size_t: 1, default: 0));
# if __STDC_VERSION__ >= 201112L && !defined __SUNPRO_C
  ASSERT (_Generic (countof (u8"UTF-8 string"), size_t: 1, default: 0));
  ASSERT (_Generic (countof (u"UTF-16 string"), size_t: 1, default: 0));
  ASSERT (_Generic (countof (U"UTF-32 string"), size_t: 1, default: 0));
# endif
#endif
}

int
main ()
{
  int x[3];

  test_func (x);

  return test_exit_status;
}

/* A definition of the variables a, b, c is not required by ISO C, because
   these identifiers are only used as part of 'sizeof' expressions whose
   results are integer expressions.  See ISO C 23 § 6.9.1.(5).  However,
   MSVC 14 and Oracle Developer Studio 12.6 generate actual references
   to these variables.  We thus need to define these variables;
   otherwise we get link errors.  */
#if (defined _MSC_VER && !defined __clang__) || defined __SUNPRO_C
int a, b, c;
#endif
