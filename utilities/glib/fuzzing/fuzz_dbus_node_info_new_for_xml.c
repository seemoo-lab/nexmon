/*
 * Copyright 2026 Philip Withnall
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

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  char *nul_terminated_data = NULL;
  GDBusNodeInfo *node = NULL;
  GError *local_error = NULL;

  fuzz_set_logging_func ();

  /* ignore @size (g_dbus_node_info_new_for_xml() doesn’t support it); ensure @data is nul-terminated */
  nul_terminated_data = g_strndup ((const gchar *) data, size);
  node = g_dbus_node_info_new_for_xml (nul_terminated_data, &local_error);
  g_free (nul_terminated_data);

  g_assert ((node == NULL) == (local_error != NULL));

  g_clear_pointer (&node, g_dbus_node_info_unref);
  g_clear_error (&local_error);

  return 0;
}
