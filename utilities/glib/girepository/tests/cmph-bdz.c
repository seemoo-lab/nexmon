/* GObject introspection: Test cmph hashing
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

#include <glib-object.h>
#include "cmph.h"

static cmph_t *
build (void)
{
  cmph_config_t *config;
  cmph_io_adapter_t *io;
  char **strings;
  cmph_t *c;
  uint32_t size;

  strings = g_strsplit ("foo,bar,baz", ",", -1);

  io = cmph_io_vector_adapter (strings, g_strv_length (strings));
  config = cmph_config_new (io);
  cmph_config_set_algo (config, CMPH_BDZ);

  c = cmph_new (config);
  size = cmph_size (c);
  g_assert_cmpuint (size, ==, g_strv_length (strings));

  cmph_config_destroy (config);
  cmph_io_vector_adapter_destroy (io);
  g_strfreev (strings);

  return c;
}

static void
assert_hashes_unique (size_t    n_hashes,
                      uint32_t* hashes)
{
  size_t i;

  for (i = 0; i < n_hashes; i++)
    {
      for (size_t j = 0; j < n_hashes; j++)
	{
	  if (j != i)
	    g_assert_cmpuint (hashes[i], !=, hashes[j]);
	}
    }
}

static void
test_search (void)
{
  cmph_t *c = build();
  size_t i;
  uint32_t hash;
  uint32_t hashes[3];
  uint32_t size;

  size = cmph_size (c);

  i = 0;
  hash = cmph_search (c, "foo", 3);
  g_assert_cmpuint (hash, >=, 0);
  g_assert_cmpuint (hash, <, size);
  hashes[i++] = hash;

  hash = cmph_search (c, "bar", 3);
  g_assert_cmpuint (hash, >=, 0);
  g_assert_cmpuint (hash, <, size);
  hashes[i++] = hash;

  hash = cmph_search (c, "baz", 3);
  g_assert_cmpuint (hash, >=, 0);
  g_assert_cmpuint (hash, <, size);
  hashes[i++] = hash;

  assert_hashes_unique (G_N_ELEMENTS (hashes), &hashes[0]);

  cmph_destroy (c);
}

static void
test_search_packed (void)
{
  cmph_t *c = build();
  size_t i;
  uint32_t bufsize;
  uint32_t hash;
  uint32_t hashes[3];
  uint32_t size;
  uint8_t *buf;

  bufsize = cmph_packed_size (c);
  buf = g_malloc (bufsize);
  cmph_pack (c, buf);

  size = cmph_size (c);

  cmph_destroy (c);
  c = NULL;

  i = 0;
  hash = cmph_search_packed (buf, "foo", 3);
  g_assert_cmpuint (hash, >=, 0);
  g_assert_cmpuint (hash, <, size);
  hashes[i++] = hash;

  hash = cmph_search_packed (buf, "bar", 3);
  g_assert_cmpuint (hash, >=, 0);
  g_assert_cmpuint (hash, <, size);
  hashes[i++] = hash;

  hash = cmph_search_packed (buf, "baz", 3);
  g_assert_cmpuint (hash, >=, 0);
  g_assert_cmpuint (hash, <, size);
  hashes[i++] = hash;

  assert_hashes_unique (G_N_ELEMENTS (hashes), &hashes[0]);

  g_free (buf);
}

int
main(int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/cmph-bdz/search", test_search);
  g_test_add_func ("/cmph-bdz/search-packed", test_search_packed);

  return g_test_run ();
}

