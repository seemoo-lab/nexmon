/* Test of canonical normalization of Unicode strings.
   Copyright (C) 2009-2026 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2009.  */

#include <config.h>

#include "uninorm.h"

#include "macros.h"

#if !(((defined _WIN32 || defined __CYGWIN__) && (HAVE_LIBUNISTRING || WOE32DLL)) || defined __ANDROID__)
/* Check that UNINORM_NFC is defined and links.  */
uninorm_t n = UNINORM_NFC;
#endif

extern void test_u8_nfc (void);
extern void test_u16_nfc (void);
extern void test_u32_nfc (void);

int
main ()
{
  /* Check that UNINORM_NFC is defined and links.  */
  volatile uninorm_t nf = UNINORM_NFC;
  (void) nf;

  test_u32_nfc ();
  test_u16_nfc ();
  test_u8_nfc ();

  return test_exit_status;
}
