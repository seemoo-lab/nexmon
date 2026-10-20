/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Typelib creation
 *
 * Copyright (C) 2005 Matthias Clasen
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

#include "config.h"

#include "girmodule-private.h"

#include "girnode-private.h"
#include "gitypelib-internal.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define ALIGN_VALUE(this, boundary) \
  (( ((unsigned long)(this)) + (((unsigned long)(boundary)) -1)) & (~(((unsigned long)(boundary))-1)))

#define NUM_SECTIONS 2

/*< private >
 * gi_ir_module_new:
 * @name:
 * @version:
 * @shared_library: (nullable):
 * @c_prefix:
 *
 * Since: 2.80
 */
GIIrModule *
gi_ir_module_new (const char *name,
                  const char *version,
                  const char *shared_library,
                  const char *c_prefix)
{
  GIIrModule *module;

  module = g_slice_new0 (GIIrModule);

  module->name = g_strdup (name);
  module->version = g_strdup (version);
  module->shared_library = g_strdup (shared_library);
  module->c_prefix = g_strdup (c_prefix);
  module->dependencies = NULL;
  module->entries = NULL;

  module->include_modules = NULL;
  module->aliases = NULL;

  return module;
}

void
gi_ir_module_free (GIIrModule *module)
{
  GList *e;

  g_free (module->name);
  g_free (module->version);
  g_free (module->shared_library);
  g_free (module->c_prefix);

  for (e = module->entries; e; e = e->next)
    gi_ir_node_free ((GIIrNode *)e->data);

  g_list_free (module->entries);
  g_clear_pointer (&module->dependencies, g_ptr_array_unref);

  g_list_free (module->include_modules);

  g_hash_table_destroy (module->aliases);
  g_hash_table_destroy (module->pointer_structures);
  g_hash_table_destroy (module->disguised_structures);

  g_slice_free (GIIrModule, module);
}

/**
 * gi_ir_module_fatal:
 * @build: Current build
 * @line: Origin line number, or 0 if unknown
 * @msg: printf-format string
 * @args: Remaining arguments
 *
 * Report a fatal error, then exit.
 *
 * Since: 2.80
 */
void
gi_ir_module_fatal (GIIrTypelibBuild *build,
                    unsigned int      line,
                    const char       *msg,
                    ...)
{
  GString *context;
  char *formatted;
  GList *link;

  va_list args;

  va_start (args, msg);

  formatted = g_strdup_vprintf (msg, args);

  context = g_string_new ("");
  if (line > 0)
    g_string_append_printf (context, "%u: ", line);
  if (build->stack)
    g_string_append (context, "In ");
  for (link = g_list_last (build->stack); link; link = link->prev)
    {
      GIIrNode *node = link->data;
      const char *name = node->name;
      if (name)
        g_string_append (context, name);
      if (link->prev)
        g_string_append (context, ".");
    }
  if (build->stack)
    g_string_append (context, ": ");

  g_printerr ("%s-%s.gir:%serror: %s\n", build->module->name, 
              build->module->version,
              context->str, formatted);
  g_string_free (context, TRUE);

  exit (1);

  va_end (args);
}

static void
add_alias_foreach (gpointer key,
                   gpointer value,
                   gpointer data)
{
  GIIrModule *module = data;

  g_hash_table_replace (module->aliases, g_strdup (key), g_strdup (value));
}

static void
add_pointer_structure_foreach (gpointer key,
                               gpointer value,
                               gpointer data)
{
  GIIrModule *module = data;

  g_hash_table_replace (module->pointer_structures, g_strdup (key), value);
}

static void
add_disguised_structure_foreach (gpointer key,
                                 gpointer value,
                                 gpointer data)
{
  GIIrModule *module = data;

  g_hash_table_replace (module->disguised_structures, g_strdup (key), value);
}

void
gi_ir_module_add_include_module (GIIrModule *module,
                                 GIIrModule *include_module)
{
  module->include_modules = g_list_prepend (module->include_modules,
                                            include_module);

  g_hash_table_foreach (include_module->aliases,
                        add_alias_foreach,
                        module);

  g_hash_table_foreach (include_module->pointer_structures,
                        add_pointer_structure_foreach,
                        module);
  g_hash_table_foreach (include_module->disguised_structures,
                        add_disguised_structure_foreach,
                        module);
}

struct AttributeWriteData
{
  unsigned int count;
  uint8_t *databuf;
  GIIrNode *node;
  GHashTable *strings;
  uint32_t *offset;
  uint32_t *offset2;
};

static void
write_attribute (gpointer key, gpointer value, gpointer datap)
{
  struct AttributeWriteData *data = datap;
  uint32_t old_offset = *(data->offset);
  AttributeBlob *blob = (AttributeBlob*)&(data->databuf[old_offset]);

  *(data->offset) += sizeof (AttributeBlob);

  blob->offset = data->node->offset;
  blob->name = gi_ir_write_string ((const char*) key, data->strings, data->databuf, data->offset2);
  blob->value = gi_ir_write_string ((const char*) value, data->strings, data->databuf, data->offset2);

  data->count++;
}

static unsigned
write_attributes (GIIrModule *module,
                  GIIrNode   *node,
                  GHashTable *strings,
                  uint8_t    *data,
                  uint32_t   *offset,
                  uint32_t   *offset2)
{
  struct AttributeWriteData wdata;
  wdata.count = 0;
  wdata.databuf = data;
  wdata.node = node;
  wdata.offset = offset;
  wdata.offset2 = offset2;
  wdata.strings = strings;

  g_hash_table_foreach (node->attributes, write_attribute, &wdata);

  return wdata.count;
}

static int
node_cmp_offset_func (const void *a,
                      const void *b)
{
  const GIIrNode *na = a;
  const GIIrNode *nb = b;
  return (int) na->offset - (int) nb->offset;
}

static void
alloc_section (uint8_t *data, SectionType section_id, uint32_t offset)
{
  int i;
  Header *header = (Header*)data;
  Section *section_data = (Section*)&data[header->sections];

  g_assert (section_id != GI_SECTION_END);

  for (i = 0; i < NUM_SECTIONS; i++)
    {
      if (section_data->id == GI_SECTION_END)
        {
          section_data->id = section_id;
          section_data->offset = offset;
          return;
        }
      section_data++;
    }
  g_assert_not_reached ();
}

static uint8_t *
add_directory_index_section (uint8_t *data, GIIrModule *module, uint32_t *offset2)
{
  DirEntry *entry;
  Header *header = (Header*)data;
  GITypelibHashBuilder *dirindex_builder;
  uint16_t n_interfaces;
  uint16_t required_size;
  uint32_t new_offset;

  dirindex_builder = gi_typelib_hash_builder_new ();

  n_interfaces = ((Header *)data)->n_local_entries;

  for (uint16_t i = 0; i < n_interfaces; i++)
    {
      const char *str;
      entry = (DirEntry *)&data[header->directory + (i * header->entry_blob_size)];
      str = (const char *) (&data[entry->name]);
      gi_typelib_hash_builder_add_string (dirindex_builder, str, i);
    }

  if (!gi_typelib_hash_builder_prepare (dirindex_builder))
    {
      /* This happens if CMPH couldn't create a perfect hash.  So
       * we just punt and leave no directory index section.
       */
      gi_typelib_hash_builder_destroy (dirindex_builder);
      return data;
    }

  alloc_section (data, GI_SECTION_DIRECTORY_INDEX, *offset2);

  required_size = gi_typelib_hash_builder_get_buffer_size (dirindex_builder);
  required_size = ALIGN_VALUE (required_size, 4);

  new_offset = *offset2 + required_size;

  data = g_realloc (data, new_offset);

  gi_typelib_hash_builder_pack (dirindex_builder, ((uint8_t*)data) + *offset2, required_size);

  *offset2 = new_offset;

  gi_typelib_hash_builder_destroy (dirindex_builder);
  return data;
}

GITypelib *
gi_ir_module_build_typelib (GIIrModule *module)
{
  GError *error = NULL;
  GBytes *bytes = NULL;
  GITypelib *typelib;
  size_t length;
  size_t i;
  GList *e;
  Header *header;
  DirEntry *entry;
  uint32_t header_size;
  uint32_t dir_size;
  uint32_t n_entries;
  uint32_t n_local_entries;
  uint32_t size, offset, offset2, old_offset;
  GHashTable *strings;
  GHashTable *types;
  GList *nodes_with_attributes;
  char *dependencies;
  uint8_t *data;
  Section *section;

  header_size = ALIGN_VALUE (sizeof (Header), 4);
  n_local_entries = g_list_length (module->entries);

  /* Serialize dependencies into one string; this is convenient
   * and not a major change to the typelib format. */
  if (module->dependencies->len)
    {
      GString *dependencies_str = g_string_new (NULL);
      for (guint i = module->dependencies->len; i > 0; --i)
        {
          const char *dependency = g_ptr_array_index (module->dependencies, i-1);
          if (!strcmp (dependency, module->name))
            continue;
          g_string_append (dependencies_str, dependency);
          if (i > 1)
            g_string_append_c (dependencies_str, '|');
        }
      dependencies = g_string_free (dependencies_str, FALSE);
      if (dependencies && !dependencies[0])
        {
          g_free (dependencies);
          dependencies = NULL;
        }
    }
  else
    {
      dependencies = NULL;
    }

 restart:
  gi_ir_node_init_stats ();
  strings = g_hash_table_new (g_str_hash, g_str_equal);
  types = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  nodes_with_attributes = NULL;
  n_entries = g_list_length (module->entries);

  g_message ("%d entries (%d local), %u dependencies", n_entries, n_local_entries,
             module->dependencies ? module->dependencies->len : 0);

  dir_size = n_entries * sizeof (DirEntry);
  size = header_size + dir_size;

  size += ALIGN_VALUE (strlen (module->name) + 1, 4);

  for (e = module->entries; e; e = e->next)
    {
      GIIrNode *node = e->data;

      size += gi_ir_node_get_full_size (node);

      /* Also reset the offset here */
      node->offset = 0;
    }

  /* Adjust size for strings allocated in header below specially */
  size += ALIGN_VALUE (strlen (module->name) + 1, 4);
  if (module->shared_library)
    size += ALIGN_VALUE (strlen (module->shared_library) + 1, 4);
  if (dependencies != NULL)
    size += ALIGN_VALUE (strlen (dependencies) + 1, 4);
  if (module->c_prefix != NULL)
    size += ALIGN_VALUE (strlen (module->c_prefix) + 1, 4);

  size += sizeof (Section) * NUM_SECTIONS;

  g_message ("allocating %d bytes (%d header, %d directory, %d entries)",
          size, header_size, dir_size, size - header_size - dir_size);

  data = g_malloc0 (size);

  /* fill in header */
  header = (Header *)data;
  memcpy (header, GI_IR_MAGIC, 16);
  header->major_version = 4;
  header->minor_version = 0;
  header->reserved = 0;
  header->n_entries = n_entries;
  header->n_local_entries = n_local_entries;
  header->n_attributes = 0;
  header->attributes = 0; /* filled in later */
  /* NOTE: When writing strings to the typelib here, you should also update
   * the size calculations above.
   */
  if (dependencies != NULL)
    header->dependencies = gi_ir_write_string (dependencies, strings, data, &header_size);
  else
    header->dependencies = 0;
  header->size = 0; /* filled in later */
  header->namespace = gi_ir_write_string (module->name, strings, data, &header_size);
  header->nsversion = gi_ir_write_string (module->version, strings, data, &header_size);
  header->shared_library = (module->shared_library?
                             gi_ir_write_string (module->shared_library, strings, data, &header_size)
                             : 0);
  if (module->c_prefix != NULL)
    header->c_prefix = gi_ir_write_string (module->c_prefix, strings, data, &header_size);
  else
    header->c_prefix = 0;
  header->entry_blob_size = sizeof (DirEntry);
  header->function_blob_size = sizeof (FunctionBlob);
  header->callback_blob_size = sizeof (CallbackBlob);
  header->signal_blob_size = sizeof (SignalBlob);
  header->vfunc_blob_size = sizeof (VFuncBlob);
  header->arg_blob_size = sizeof (ArgBlob);
  header->property_blob_size = sizeof (PropertyBlob);
  header->field_blob_size = sizeof (FieldBlob);
  header->value_blob_size = sizeof (ValueBlob);
  header->constant_blob_size = sizeof (ConstantBlob);
  header->error_domain_blob_size = 16; /* No longer used */
  header->attribute_blob_size = sizeof (AttributeBlob);
  header->signature_blob_size = sizeof (SignatureBlob);
  header->enum_blob_size = sizeof (EnumBlob);
  header->struct_blob_size = sizeof (StructBlob);
  header->object_blob_size = sizeof(ObjectBlob);
  header->interface_blob_size = sizeof (InterfaceBlob);
  header->union_blob_size = sizeof (UnionBlob);

  offset2 = ALIGN_VALUE (header_size, 4);
  header->sections = offset2;

  /* Initialize all the sections to _END/0; we fill them in later using
   * alloc_section().  (Right now there's just the directory index
   * though, note)
   */
  for (i = 0; i < NUM_SECTIONS; i++)
    {
      section = (Section*) &data[offset2];
      section->id = GI_SECTION_END;
      section->offset = 0;
      offset2 += sizeof(Section);
    }
  header->directory = offset2;

  /* fill in directory and content */
  entry = (DirEntry *)&data[header->directory];

  offset2 += dir_size;

  for (e = module->entries, i = 0; e; e = e->next, i++)
    {
      GIIrNode *node = e->data;

      if (strchr (node->name, '.'))
        {
          g_error ("Names may not contain '.'");
        }

      /* we picked up implicit xref nodes, start over */
      if (i == n_entries)
        {
          GList *link;
          g_message ("Found implicit cross references, starting over");

          g_hash_table_destroy (strings);
          g_hash_table_destroy (types);

          /* Reset the cached offsets */
          for (link = nodes_with_attributes; link; link = link->next)
            ((GIIrNode *) link->data)->offset = 0;

          g_list_free (nodes_with_attributes);
          strings = NULL;

          g_free (data);
          data = NULL;

          goto restart;
        }

      offset = offset2;

      if (node->type == GI_IR_NODE_XREF)
        {
          const char *namespace = ((GIIrNodeXRef*)node)->namespace;

          entry->blob_type = 0;
          entry->local = FALSE;
          entry->offset = gi_ir_write_string (namespace, strings, data, &offset2);
          entry->name = gi_ir_write_string (node->name, strings, data, &offset2);
        }
      else
        {
          GIIrTypelibBuild build = {0};
          old_offset = offset;
          offset2 = offset + gi_ir_node_get_size (node);

          entry->blob_type = node->type;
          entry->local = TRUE;
          entry->offset = offset;
          entry->name = gi_ir_write_string (node->name, strings, data, &offset2);

          build.module = module;
          build.strings = strings;
          build.types = types;
          build.nodes_with_attributes = nodes_with_attributes;
          build.n_attributes = header->n_attributes;
          build.data = data;
          gi_ir_node_build_typelib (node, NULL, &build, &offset, &offset2, NULL);
          g_clear_list (&build.stack, NULL);

          nodes_with_attributes = build.nodes_with_attributes;
          header->n_attributes = build.n_attributes;

          if (offset2 > old_offset + gi_ir_node_get_full_size (node))
            g_error ("left a hole of %d bytes", offset2 - old_offset - gi_ir_node_get_full_size (node));
        }

      entry++;
    }

  /* GIBaseInfo expects the AttributeBlob array to be sorted on the field (offset) */
  nodes_with_attributes = g_list_sort (nodes_with_attributes, node_cmp_offset_func);

  g_message ("header: %d entries, %d attributes", header->n_entries, header->n_attributes);

  gi_ir_node_dump_stats ();

  /* Write attributes after the blobs */
  offset = offset2;
  header->attributes = offset;
  offset2 = offset + header->n_attributes * header->attribute_blob_size;

  for (e = nodes_with_attributes; e; e = e->next)
    {
      GIIrNode *node = e->data;
      write_attributes (module, node, strings, data, &offset, &offset2);
    }

  g_message ("reallocating to %d bytes", offset2);

  data = g_realloc (data, offset2);
  header = (Header*) data;

  data = add_directory_index_section (data, module, &offset2);
  header = (Header *)data;

  length = header->size = offset2;

  bytes = g_bytes_new_take (g_steal_pointer (&data), length);
  typelib = gi_typelib_new_from_bytes (bytes, &error);
  g_bytes_unref (bytes);

  if (!typelib)
    {
      g_error ("error building typelib: %s",
               error->message);
    }

  g_hash_table_destroy (strings);
  g_hash_table_destroy (types);
  g_list_free (nodes_with_attributes);
  g_free (dependencies);

  return typelib;
}

