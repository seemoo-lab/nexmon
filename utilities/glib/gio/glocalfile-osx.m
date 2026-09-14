/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2026 Byoungchan Lee <byoungchan.lee@gmx.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "glocalfile.h"

#include <Foundation/Foundation.h>
#include <gio/gcancellable.h>
#include <gio/gioerror.h>

static GIOErrorEnum
ns_error_to_gio_error (NSError *error)
{
  switch (error.code)
    {
    case NSFileNoSuchFileError:
      return G_IO_ERROR_NOT_FOUND;

    case NSFileReadNoPermissionError:
    case NSFileWriteNoPermissionError:
    case NSFileWriteVolumeReadOnlyError:
      return G_IO_ERROR_PERMISSION_DENIED;

    case NSUserCancelledError:
      return G_IO_ERROR_CANCELLED;

    default:
      return G_IO_ERROR_FAILED;
    }
}

gboolean
_g_local_file_trash_macos (const char    *path,
                           GCancellable  *cancellable,
                           GError       **error)
{
  g_return_val_if_fail (path != NULL, FALSE);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  @autoreleasepool
  {
    NSURL *url;
    NSError *ns_error = nil;
    NSFileManager *file_manager;

    url = [NSURL fileURLWithPath:@(path)];
    file_manager = [NSFileManager defaultManager];

    if (![file_manager trashItemAtURL:url resultingItemURL:NULL error:&ns_error])
      {
        if (g_cancellable_set_error_if_cancelled (cancellable, error))
          return FALSE;

        g_set_error_literal (error,
                             G_IO_ERROR,
                             ns_error_to_gio_error (ns_error),
                             ns_error.localizedDescription.UTF8String);
        return FALSE;
      }
  }

  return TRUE;
}
