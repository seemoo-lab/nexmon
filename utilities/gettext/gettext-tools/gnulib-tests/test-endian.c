/* Test of <endian.h> substitute.
   Copyright (C) 2024-2026 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published
   by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Collin Funk <collin.funk1@gmail.com>, 2024.  */

#include <config.h>

/* Specification.  */
#include <endian.h>

/* Check for uint16_t and uint32_t.  */
uint16_t t1;
uint32_t t2;

/* The next POSIX revision requires 64-bit types. Gnulib doesn't.  */
#if 0
uint64_t t3;
#endif

/* "These macros shall be suitable for use in #if preprocessing directives."  */
#if BYTE_ORDER == LITTLE_ENDIAN
int a = 17;
#endif
#if BYTE_ORDER == BIG_ENDIAN
int a = 19;
#endif

/* "The macros BIG_ENDIAN and LITTLE_ENDIAN shall have distinct values."  */
static_assert (LITTLE_ENDIAN != BIG_ENDIAN);

static_assert (BYTE_ORDER == LITTLE_ENDIAN || BYTE_ORDER == BIG_ENDIAN);

#include <stdint.h>

#include "macros.h"

/* Test byte order conversion functions with constant values.  */
static void
test_convert_constant (void)
{
#if BYTE_ORDER == BIG_ENDIAN
  /* 16-bit.  */
  ASSERT (be16toh (UINT16_C (0x1234)) == UINT16_C (0x1234));
  ASSERT (htobe16 (UINT16_C (0x1234)) == UINT16_C (0x1234));
  ASSERT (le16toh (UINT16_C (0x1234)) == UINT16_C (0x3412));
  ASSERT (htole16 (UINT16_C (0x1234)) == UINT16_C (0x3412));

  /* 32-bit.  */
  ASSERT (be32toh (UINT32_C (0x12345678)) == UINT32_C (0x12345678));
  ASSERT (htobe32 (UINT32_C (0x12345678)) == UINT32_C (0x12345678));
  ASSERT (le32toh (UINT32_C (0x12345678)) == UINT32_C (0x78563412));
  ASSERT (htole32 (UINT32_C (0x12345678)) == UINT32_C (0x78563412));

  /* 64-bit.  */
# ifdef UINT64_MAX
  ASSERT (be64toh (UINT64_C (0x1234567890ABCDEF))
          == UINT64_C (0x1234567890ABCDEF));
  ASSERT (htobe64 (UINT64_C (0x1234567890ABCDEF))
          == UINT64_C (0x1234567890ABCDEF));
  ASSERT (le64toh (UINT64_C (0x1234567890ABCDEF))
          == UINT64_C (0xEFCDAB9078563412));
  ASSERT (htole64 (UINT64_C (0x1234567890ABCDEF))
          == UINT64_C (0xEFCDAB9078563412));
# endif
#else
  /* 16-bit.  */
  ASSERT (be16toh (UINT16_C (0x1234)) == UINT16_C (0x3412));
  ASSERT (htobe16 (UINT16_C (0x1234)) == UINT16_C (0x3412));
  ASSERT (le16toh (UINT16_C (0x1234)) == UINT16_C (0x1234));
  ASSERT (htole16 (UINT16_C (0x1234)) == UINT16_C (0x1234));

  /* 32-bit.  */
  ASSERT (be32toh (UINT32_C (0x12345678)) == UINT32_C (0x78563412));
  ASSERT (htobe32 (UINT32_C (0x12345678)) == UINT32_C (0x78563412));
  ASSERT (le32toh (UINT32_C (0x12345678)) == UINT32_C (0x12345678));
  ASSERT (htole32 (UINT32_C (0x12345678)) == UINT32_C (0x12345678));

  /* 64-bit.  */
# ifdef UINT64_MAX
  ASSERT (be64toh (UINT64_C (0x1234567890ABCDEF))
          == UINT64_C (0xEFCDAB9078563412));
  ASSERT (htobe64 (UINT64_C (0x1234567890ABCDEF))
          == UINT64_C (0xEFCDAB9078563412));
  ASSERT (le64toh (UINT64_C (0x1234567890ABCDEF))
          == UINT64_C (0x1234567890ABCDEF));
  ASSERT (htole64 (UINT64_C (0x1234567890ABCDEF))
          == UINT64_C (0x1234567890ABCDEF));
# endif
#endif
}

/* Test that the byte order conversion functions evaluate their
   arguments once.  */
static void
test_convert_eval_once (void)
{
  /* 16-bit.  */
  {
    uint16_t value = 0;
    ASSERT (be16toh (value++) == 0);
    ASSERT (value == 1);
  }
  {
    uint16_t value = 0;
    ASSERT (htobe16 (value++) == 0);
    ASSERT (value == 1);
  }
  {
    uint16_t value = 0;
    ASSERT (le16toh (value++) == 0);
    ASSERT (value == 1);
  }
  {
    uint16_t value = 0;
    ASSERT (htole16 (value++) == 0);
    ASSERT (value == 1);
  }

  /* 32-bit.  */
  {
    uint32_t value = 0;
    ASSERT (be32toh (value++) == 0);
    ASSERT (value == 1);
  }
  {
    uint32_t value = 0;
    ASSERT (htobe32 (value++) == 0);
    ASSERT (value == 1);
  }
  {
    uint32_t value = 0;
    ASSERT (le32toh (value++) == 0);
    ASSERT (value == 1);
  }
  {
    uint32_t value = 0;
    ASSERT (htole32 (value++) == 0);
    ASSERT (value == 1);
  }

  /* 64-bit.  */
#ifdef UINT64_MAX
  {
    uint64_t value = 0;
    ASSERT (be64toh (value++) == 0);
    ASSERT (value == 1);
  }
  {
    uint64_t value = 0;
    ASSERT (htobe64 (value++) == 0);
    ASSERT (value == 1);
  }
  {
    uint64_t value = 0;
    ASSERT (le64toh (value++) == 0);
    ASSERT (value == 1);
  }
  {
    uint64_t value = 0;
    ASSERT (htole64 (value++) == 0);
    ASSERT (value == 1);
  }
#endif
}

/* Test that the byte order conversion functions accept floating-point
   arguments.  */
static void
test_convert_double (void)
{
  /* 16-bit.  */
  ASSERT (be16toh (0.0) == 0);
  ASSERT (htobe16 (0.0) == 0);
  ASSERT (le16toh (0.0) == 0);
  ASSERT (htole16 (0.0) == 0);

  /* 32-bit.  */
  ASSERT (be32toh (0.0) == 0);
  ASSERT (htobe32 (0.0) == 0);
  ASSERT (le32toh (0.0) == 0);
  ASSERT (htole32 (0.0) == 0);

  /* 64-bit.  */
#ifdef UINT64_MAX
  ASSERT (be64toh (0.0) == 0);
  ASSERT (htobe64 (0.0) == 0);
  ASSERT (le64toh (0.0) == 0);
  ASSERT (htole64 (0.0) == 0);
#endif
}

int
main (void)
{
  test_convert_constant ();
  test_convert_eval_once ();
  test_convert_double ();

  return test_exit_status;
}
