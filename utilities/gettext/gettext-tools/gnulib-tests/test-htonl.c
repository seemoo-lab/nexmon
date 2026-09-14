/* Test of host and network byte order conversion functions.
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
#include <arpa/inet.h>

#include <endian.h>
#include <stdint.h>

#include "macros.h"

/* Test byte order conversion functions with constant values.  */
static void
test_convert_constant (void)
{
#if BYTE_ORDER == BIG_ENDIAN
  /* 16-bit.  */
  ASSERT (htons (UINT16_C (0x1234)) == UINT16_C (0x1234));
  ASSERT (ntohs (UINT16_C (0x1234)) == UINT16_C (0x1234));

  /* 32-bit.  */
  ASSERT (htonl (UINT32_C (0x12345678)) == UINT32_C (0x12345678));
  ASSERT (ntohl (UINT32_C (0x12345678)) == UINT32_C (0x12345678));
#else
  /* 16-bit.  */
  ASSERT (htons (UINT16_C (0x1234)) == UINT16_C (0x3412));
  ASSERT (ntohs (UINT16_C (0x1234)) == UINT16_C (0x3412));

  /* 32-bit.  */
  ASSERT (htonl (UINT32_C (0x12345678)) == UINT32_C (0x78563412));
  ASSERT (ntohl (UINT32_C (0x12345678)) == UINT32_C (0x78563412));
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
    ASSERT (htons (value++) == 0);
    ASSERT (value == 1);
  }
  {
    uint16_t value = 0;
    ASSERT (ntohs (value++) == 0);
    ASSERT (value == 1);
  }

  /* 32-bit.  */
  {
    uint32_t value = 0;
    ASSERT (htonl (value++) == 0);
    ASSERT (value == 1);
  }
  {
    uint32_t value = 0;
    ASSERT (ntohl (value++) == 0);
    ASSERT (value == 1);
  }
}

/* Test that the byte order conversion functions accept floating-point
   arguments.  */
static void
test_convert_double (void)
{
  /* 16-bit.  */
  ASSERT (htons (0.0) == 0);
  ASSERT (ntohs (0.0) == 0);

  /* 32-bit.  */
  ASSERT (htonl (0.0) == 0);
  ASSERT (ntohs (0.0) == 0);
}

int
main (void)
{
  test_convert_constant ();
  test_convert_eval_once ();
  test_convert_double ();

  return test_exit_status;
}
