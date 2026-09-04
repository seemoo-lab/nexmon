/*
 * Copyright 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright 2023 Collabora Ltd.
 *
 * SPDX-License-Identifier: LicenseRef-old-glib-tests
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

#include "config.h"

#include <errno.h>

#include "glib-private.h"
#include "gstdio.h"

static inline void
assert_fd_was_closed (int fd)
{
  /* We can't tell a fd was really closed without behaving as though it
   * was still valid */
  if (g_test_undefined ())
    {
      int result, errsv;
      GWin32InvalidParameterHandler handler;

      GLIB_PRIVATE_CALL (g_win32_push_empty_invalid_parameter_handler) (&handler);
      result = g_fsync (fd);
      errsv = errno;
      GLIB_PRIVATE_CALL (g_win32_pop_invalid_parameter_handler) (&handler);

      g_assert_cmpint (result, !=, 0);
      g_assert_cmpint (errsv, ==, EBADF);
    }
}
