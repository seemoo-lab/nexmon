/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Typelib hashing
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

#include <glib.h>
#include <glib-object.h>
#include <string.h>

#include "cmph/cmph.h"
#include "gitypelib-internal.h"

#define ALIGN_VALUE(this, boundary) \
  (( ((unsigned long)(this)) + (((unsigned long)(boundary)) -1)) & (~(((unsigned long)(boundary))-1)))

/*
 * String hashing in the typelib.  We have a set of static (fixed) strings,
 * and given one, we need to find its index number.  This problem is perfect
 * hashing: http://en.wikipedia.org/wiki/Perfect_hashing
 *
 * I chose CMPH (http://cmph.sourceforge.net/) as it seemed high
 * quality, well documented, and easy to embed.
 *
 * CMPH provides a number of algorithms; I chose BDZ, because while CHD
 * appears to be the "best", the simplicity of BDZ appealed, and really,
 * we're only talking about thousands of strings here, not millions, so
 * a few microseconds is no big deal.
 *
 * In memory, the format is:
 * INT32 mph_size
 * MPH (mph_size bytes)
 * (padding for alignment to uint32 if necessary)
 * INDEX (array of uint16_t)
 *
 * Because BDZ is not order preserving, we need a lookaside table which
 * maps the hash value into the directory index.
 */

struct _GITypelibHashBuilder {
  gboolean prepared;
  gboolean buildable;
  cmph_t *c;
  GHashTable *strings;
  uint32_t dirmap_offset;
  uint32_t packed_size;
};

GITypelibHashBuilder *
gi_typelib_hash_builder_new (void)
{
  GITypelibHashBuilder *builder = g_slice_new0 (GITypelibHashBuilder);
  builder->c = NULL;
  builder->strings = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  return builder;
}

void
gi_typelib_hash_builder_add_string (GITypelibHashBuilder *builder,
                                    const char           *str,
                                    uint16_t              value)
{
  g_return_if_fail (builder->c == NULL);
  g_hash_table_insert (builder->strings, g_strdup (str), GUINT_TO_POINTER (value));
}

gboolean
gi_typelib_hash_builder_prepare (GITypelibHashBuilder *builder)
{
  char **strs;
  GHashTableIter hashiter;
  void *key, *value;
  cmph_io_adapter_t *io;
  cmph_config_t *config;
  uint32_t num_elts;
  uint32_t offset;
  unsigned i;

  if (builder->prepared)
    return builder->buildable;
  g_assert (builder->c == NULL);

  num_elts = g_hash_table_size (builder->strings);
  g_assert (num_elts <= 65536);

  strs = (char**) g_new (char *, num_elts + 1);

  i = 0;
  g_hash_table_iter_init (&hashiter, builder->strings);
  while (g_hash_table_iter_next (&hashiter, &key, &value))
    {
      const char *str = key;

      strs[i++] = g_strdup (str);
    }
  strs[i++] = NULL;

  io = cmph_io_vector_adapter (strs, num_elts);
  config = cmph_config_new (io);
  cmph_config_set_algo (config, CMPH_BDZ);

  builder->c = cmph_new (config);
  builder->prepared = TRUE;
  if (!builder->c)
    {
      builder->buildable = FALSE;
      goto out;
    }
  builder->buildable = TRUE;
  g_assert (cmph_size (builder->c) == num_elts);

  /* Pack a size counter at front */
  offset = sizeof (uint32_t) + cmph_packed_size (builder->c);
  builder->dirmap_offset = ALIGN_VALUE (offset, 4);
  builder->packed_size = builder->dirmap_offset + (num_elts * sizeof (uint16_t));
 out:
  g_strfreev (strs);
  cmph_config_destroy (config);
  cmph_io_vector_adapter_destroy (io);
  return builder->buildable;
}

uint32_t
gi_typelib_hash_builder_get_buffer_size (GITypelibHashBuilder *builder)
{
  g_return_val_if_fail (builder != NULL, 0);
  g_return_val_if_fail (builder->prepared, 0);
  g_return_val_if_fail (builder->buildable, 0 );

  return builder->packed_size;
}

void
gi_typelib_hash_builder_pack (GITypelibHashBuilder *builder, uint8_t* mem, uint32_t len)
{
  uint16_t *table;
  GHashTableIter hashiter;
  void *key, *value;
#ifndef G_DISABLE_ASSERT
  uint32_t num_elts;
#endif
  uint8_t *packed_mem;

  g_return_if_fail (builder != NULL);
  g_return_if_fail (builder->prepared);
  g_return_if_fail (builder->buildable);

  g_assert (len >= builder->packed_size);
  g_assert ((((size_t)mem) & 0x3) == 0);

  memset (mem, 0, len);

  *((uint32_t*) mem) = builder->dirmap_offset;
  packed_mem = (uint8_t*)(mem + sizeof (uint32_t));
  cmph_pack (builder->c, packed_mem);

  table = (uint16_t*) (mem + builder->dirmap_offset);

#ifndef G_DISABLE_ASSERT
  num_elts = g_hash_table_size (builder->strings);
#endif
  g_hash_table_iter_init (&hashiter, builder->strings);
  while (g_hash_table_iter_next (&hashiter, &key, &value))
    {
      const char *str = key;
      uint16_t strval = (uint16_t)GPOINTER_TO_UINT(value);
      uint32_t hashv;

      hashv = cmph_search_packed (packed_mem, str, strlen (str));
      g_assert (hashv < num_elts);
      table[hashv] = strval;
    }
}

void
gi_typelib_hash_builder_destroy (GITypelibHashBuilder *builder)
{
  if (builder->c)
    {
      cmph_destroy (builder->c);
      builder->c = NULL;
    }
  g_hash_table_destroy (builder->strings);
  g_slice_free (GITypelibHashBuilder, builder);
}

uint16_t
gi_typelib_hash_search (uint8_t* memory, const char *str, uint32_t n_entries)
{
  uint32_t *mph;
  uint16_t *table;
  uint32_t dirmap_offset;
  uint32_t offset;

  g_assert ((((size_t)memory) & 0x3) == 0);
  mph = ((uint32_t*)memory)+1;

  offset = cmph_search_packed (mph, str, strlen (str));

  /* Make sure that offset always lies in the entries array.  cmph
     cometimes generates offset larger than number of entries (for
     'str' argument which is not in the hashed list). In this case,
     fake the correct result and depend on caller's final check that
     the entry is really the one that the caller wanted. */
  if (offset >= n_entries)
    offset = 0;

  dirmap_offset = *((uint32_t*)memory);
  table = (uint16_t*) (memory + dirmap_offset);

  return table[offset];
}

