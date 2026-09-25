/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Test typelib hashing
 *
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdint.h>
#include <glib-object.h>
#include "gitypelib-internal.h"

static void
test_build_retrieve (void)
{
  GITypelibHashBuilder *builder;
  uint32_t bufsize;
  uint8_t* buf;

  builder = gi_typelib_hash_builder_new ();

  gi_typelib_hash_builder_add_string (builder, "Action", 0);
  gi_typelib_hash_builder_add_string (builder, "ZLibDecompressor", 42);
  gi_typelib_hash_builder_add_string (builder, "VolumeMonitor", 9);
  gi_typelib_hash_builder_add_string (builder, "FileMonitorFlags", 31);

  g_assert_true (gi_typelib_hash_builder_prepare (builder));

  bufsize = gi_typelib_hash_builder_get_buffer_size (builder);

  buf = g_malloc (bufsize);

  gi_typelib_hash_builder_pack (builder, buf, bufsize);

  gi_typelib_hash_builder_destroy (builder);

  g_assert_cmpuint (gi_typelib_hash_search (buf, "Action", 4), ==, 0);
  g_assert_cmpuint (gi_typelib_hash_search (buf, "ZLibDecompressor", 4), ==, 42);
  g_assert_cmpuint (gi_typelib_hash_search (buf, "VolumeMonitor", 4), ==, 9);
  g_assert_cmpuint (gi_typelib_hash_search (buf, "FileMonitorFlags", 4), ==, 31);

  g_free (buf);
}

int
main(int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/gthash/build-retrieve", test_build_retrieve);

  return g_test_run ();
}

