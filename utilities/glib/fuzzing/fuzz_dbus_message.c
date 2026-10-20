/*
 * Copyright 2018 pdknsk
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "fuzz.h"

static const GDBusCapabilityFlags flags = G_DBUS_CAPABILITY_FLAGS_UNIX_FD_PASSING;

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  gssize bytes;
  GDBusMessage *msg = NULL;
  guchar *blob = NULL;
  gsize msg_size;

  fuzz_set_logging_func ();

  bytes = g_dbus_message_bytes_needed ((guchar*) data, size, NULL);
  if (bytes <= 0)
    return 0;

  msg = g_dbus_message_new_from_blob ((guchar*) data, size, flags, NULL);
  if (msg == NULL)
    return 0;

  blob = g_dbus_message_to_blob (msg, &msg_size, flags, NULL);

  g_free (blob);
  g_object_unref (msg);
  return 0;
}
