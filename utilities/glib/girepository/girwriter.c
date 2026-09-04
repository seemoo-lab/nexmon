/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: IDL generator
 *
 * Copyright (C) 2005 Matthias Clasen
 * Copyright (C) 2008,2009 Red Hat, Inc.
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

#include "girwriter-private.h"

#include "gibaseinfo-private.h"
#include "girepository.h"
#include "girepository-private.h"
#include "gitypelib-internal.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gstdio.h>

typedef struct {
  FILE *file;
  GSList *stack;
  gboolean show_all;
} Xml;

typedef struct {
  char *name;
  unsigned has_children : 1;
} XmlElement;

static XmlElement *
xml_element_new (const char *name)
{
  XmlElement *elem;

  elem = g_slice_new (XmlElement);
  elem->name = g_strdup (name);
  elem->has_children = FALSE;
  return elem;
}

static void
xml_element_free (XmlElement *elem)
{
  g_free (elem->name);
  g_slice_free (XmlElement, elem);
}

static void
xml_printf (Xml *xml, const char *fmt, ...) G_GNUC_PRINTF (2, 3);

static void
xml_printf (Xml *xml, const char *fmt, ...)
{
  va_list ap;
  char *s;

  va_start (ap, fmt);
  s = g_markup_vprintf_escaped (fmt, ap);
  fputs (s, xml->file);
  g_free (s);
  va_end (ap);
}

static void
xml_start_element (Xml *xml, const char *element_name)
{
  XmlElement *parent = NULL;

  if (xml->stack)
    {
      parent = xml->stack->data;

      if (!parent->has_children)
        xml_printf (xml, ">\n");

      parent->has_children = TRUE;
    }

  xml_printf (xml, "%*s<%s", g_slist_length(xml->stack)*2, "", element_name);

  xml->stack = g_slist_prepend (xml->stack, xml_element_new (element_name));
}

static void
xml_end_element (Xml *xml, const char *name)
{
  XmlElement *elem;

  g_assert (xml->stack != NULL);

  elem = xml->stack->data;
  xml->stack = g_slist_delete_link (xml->stack, xml->stack);

  if (name != NULL)
    g_assert_cmpstr (name, ==, elem->name);

  if (elem->has_children)
    xml_printf (xml, "%*s</%s>\n", g_slist_length (xml->stack)*2, "", elem->name);
  else
    xml_printf (xml, "/>\n");

  xml_element_free (elem);
}

static void
xml_end_element_unchecked (Xml *xml)
{
  xml_end_element (xml, NULL);
}

static Xml *
xml_open (FILE *file)
{
  Xml *xml;

  xml = g_slice_new (Xml);
  xml->file = file;
  xml->stack = NULL;

  return xml;
}

static void
xml_close (Xml *xml)
{
  g_assert (xml->stack == NULL);
  if (xml->file != NULL)
    {
      fflush (xml->file);
      if (xml->file != stdout)
        fclose (xml->file);
      xml->file = NULL;
    }
}

static void
xml_free (Xml *xml)
{
  xml_close (xml);
  g_slice_free (Xml, xml);
}


static void
check_unresolved (GIBaseInfo *info)
{
  if (!GI_IS_UNRESOLVED_INFO (info))
    return;

  g_critical ("Found unresolved type '%s' '%s'",
              gi_base_info_get_name (info), gi_base_info_get_namespace (info));
}

static void
write_type_name (const char *ns,
                 GIBaseInfo  *info,
                 Xml         *file)
{
  if (strcmp (ns, gi_base_info_get_namespace (info)) != 0)
    xml_printf (file, "%s.", gi_base_info_get_namespace (info));

  xml_printf (file, "%s", gi_base_info_get_name (info));
}

static void
write_type_name_attribute (const char *ns,
                           GIBaseInfo  *info,
                           const char  *attr_name,
                           Xml         *file)
{
  xml_printf (file, " %s=\"", attr_name);
  write_type_name (ns, info, file);
  xml_printf (file, "\"");
}

 static void
write_ownership_transfer (GITransfer transfer,
                          Xml       *file)
{
  switch (transfer)
    {
    case GI_TRANSFER_NOTHING:
      xml_printf (file, " transfer-ownership=\"none\"");
      break;
    case GI_TRANSFER_CONTAINER:
      xml_printf (file, " transfer-ownership=\"container\"");
      break;
    case GI_TRANSFER_EVERYTHING:
      xml_printf (file, " transfer-ownership=\"full\"");
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
write_type_info (const char *ns,
                 GITypeInfo  *info,
                 Xml         *file)
{
  GITypeTag tag;
  GITypeInfo *type;
  gboolean is_pointer;

  check_unresolved ((GIBaseInfo*)info);

  tag = gi_type_info_get_tag (info);
  is_pointer = gi_type_info_is_pointer (info);

  if (tag == GI_TYPE_TAG_VOID)
    {
      xml_start_element (file, "type");

      xml_printf (file, " name=\"%s\"", is_pointer ? "any" : "none");

      xml_end_element (file, "type");
    }
  else if (GI_TYPE_TAG_IS_BASIC (tag))
    {
      xml_start_element (file, "type");
      xml_printf (file, " name=\"%s\"", gi_type_tag_to_string (tag));
      xml_end_element (file, "type");
    }
  else if (tag == GI_TYPE_TAG_ARRAY)
    {
      unsigned int length_index;
      size_t size;
      const char *name = NULL;

      xml_start_element (file, "array");

      switch (gi_type_info_get_array_type (info)) {
        case GI_ARRAY_TYPE_C:
            break;
        case GI_ARRAY_TYPE_ARRAY:
            name = "GLib.Array";
            break;
        case GI_ARRAY_TYPE_PTR_ARRAY:
            name = "GLib.PtrArray";
            break;
        case GI_ARRAY_TYPE_BYTE_ARRAY:
            name = "GLib.ByteArray";
            break;
        default:
            break;
      }

      if (name)
        xml_printf (file, " name=\"%s\"", name);

      type = gi_type_info_get_param_type (info, 0);

      if (gi_type_info_get_array_length_index (info, &length_index))
        xml_printf (file, " length=\"%u\"", length_index);

      if (gi_type_info_get_array_fixed_size (info, &size))
        xml_printf (file, " fixed-size=\"%zu\"", size);

      if (gi_type_info_is_zero_terminated (info))
        xml_printf (file, " zero-terminated=\"1\"");

      write_type_info (ns, type, file);

      gi_base_info_unref ((GIBaseInfo *)type);

      xml_end_element (file, "array");
    }
  else if (tag == GI_TYPE_TAG_INTERFACE)
    {
      GIBaseInfo *iface = gi_type_info_get_interface (info);
      xml_start_element (file, "type");
      write_type_name_attribute (ns, iface, "name", file);
      xml_end_element (file, "type");
      gi_base_info_unref (iface);
    }
  else if (tag == GI_TYPE_TAG_GLIST)
    {
      xml_start_element (file, "type");
      xml_printf (file, " name=\"GLib.List\"");
      type = gi_type_info_get_param_type (info, 0);
      if (type)
        {
          write_type_info (ns, type, file);
          gi_base_info_unref ((GIBaseInfo *)type);
        }
      xml_end_element (file, "type");
    }
  else if (tag == GI_TYPE_TAG_GSLIST)
    {
      xml_start_element (file, "type");
      xml_printf (file, " name=\"GLib.SList\"");
      type = gi_type_info_get_param_type (info, 0);
      if (type)
        {
          write_type_info (ns, type, file);
          gi_base_info_unref ((GIBaseInfo *)type);
        }
      xml_end_element (file, "type");
    }
  else if (tag == GI_TYPE_TAG_GHASH)
    {
      xml_start_element (file, "type");
      xml_printf (file, " name=\"GLib.HashTable\"");
      type = gi_type_info_get_param_type (info, 0);
      if (type)
        {
          write_type_info (ns, type, file);
          gi_base_info_unref ((GIBaseInfo *)type);
          type = gi_type_info_get_param_type (info, 1);
          write_type_info (ns, type, file);
          gi_base_info_unref ((GIBaseInfo *)type);
        }
      xml_end_element (file, "type");
    }
  else if (tag == GI_TYPE_TAG_ERROR)
    {
      xml_start_element (file, "type");
      xml_printf (file, " name=\"GLib.Error\"");
      xml_end_element (file, "type");
    }
  else
    {
      g_printerr ("Unhandled type tag %d\n", tag);
      g_assert_not_reached ();
    }
}

static void
write_attributes (Xml *file,
                  GIBaseInfo *info)
{
  GIAttributeIter iter = GI_ATTRIBUTE_ITER_INIT;
  const char *name, *value;

  while (gi_base_info_iterate_attributes (info, &iter, &name, &value))
    {
      xml_start_element (file, "attribute");
      xml_printf (file, " name=\"%s\" value=\"%s\"", name, value);
      xml_end_element (file, "attribute");
    }
}

static void
write_return_value_attributes (Xml *file,
                               GICallableInfo *info)
{
  GIAttributeIter iter = GI_ATTRIBUTE_ITER_INIT;
  const char *name, *value;

  while (gi_callable_info_iterate_return_attributes (info, &iter, &name, &value))
    {
      xml_start_element (file, "attribute");
      xml_printf (file, " name=\"%s\" value=\"%s\"", name, value);
      xml_end_element (file, "attribute");
    }
}

static void
write_constant_value (const char *ns,
                      GITypeInfo *info,
                      GIArgument *argument,
                      Xml *file);

static void
write_callback_info (const char     *ns,
                     GICallbackInfo *info,
                     Xml            *file);

static void
write_field_info (const char *ns,
                  GIFieldInfo *info,
                  GIConstantInfo *branch,
                  Xml         *file)
{
  const char *name;
  GIFieldInfoFlags flags;
  size_t size;
  size_t offset;
  GITypeInfo *type;
  GIBaseInfo *interface;
  GIArgument value;

  name = gi_base_info_get_name ((GIBaseInfo *)info);
  flags = gi_field_info_get_flags (info);
  size = gi_field_info_get_size (info);
  offset = gi_field_info_get_offset (info);

  xml_start_element (file, "field");
  xml_printf (file, " name=\"%s\"", name);

  /* Fields are assumed to be read-only
   * (see also girwriter.py and girparser.c)
   */
  if (!(flags & GI_FIELD_IS_READABLE))
    xml_printf (file, " readable=\"0\"");
  if (flags & GI_FIELD_IS_WRITABLE)
    xml_printf (file, " writable=\"1\"");

  if (size)
    xml_printf (file, " bits=\"%zu\"", size);

  write_attributes (file, (GIBaseInfo*) info);

  type = gi_field_info_get_type_info (info);

  if (branch)
    {
      xml_printf (file, " branch=\"");
      gi_base_info_unref ((GIBaseInfo *)type);
      type = gi_constant_info_get_type_info (branch);
      gi_constant_info_get_value (branch, &value);
      write_constant_value (ns, type, &value, file);
      xml_printf (file, "\"");
    }

  if (file->show_all)
    {
      xml_printf (file, "offset=\"%zu\"", offset);
    }

  interface = gi_type_info_get_interface (type);
  if (interface != NULL && GI_IS_CALLBACK_INFO (interface))
    write_callback_info (ns, (GICallbackInfo *)interface, file);
  else
    write_type_info (ns, type, file);

  if (interface)
    gi_base_info_unref (interface);

  gi_base_info_unref ((GIBaseInfo *)type);

  xml_end_element (file, "field");
}

static void
write_callable_info (const char     *ns,
                     GICallableInfo *info,
                     Xml            *file)
{
  GITypeInfo *type;
  GICallableInfo *async_func;
  GICallableInfo *sync_func;
  GICallableInfo *finish_func;

  if (gi_callable_info_can_throw_gerror (info))
    xml_printf (file, " throws=\"1\"");

  write_attributes (file, (GIBaseInfo*) info);

  type = gi_callable_info_get_return_type (info);
  async_func = gi_callable_info_get_async_function (info);
  sync_func = gi_callable_info_get_sync_function (info);
  finish_func = gi_callable_info_get_finish_function (info);

  if (sync_func)
    xml_printf (file, " glib:sync-func=\"%s\"", gi_base_info_get_name ((GIBaseInfo*) sync_func));

  if (finish_func)
    xml_printf (file, " glib:finish-func=\"%s\"", gi_base_info_get_name ((GIBaseInfo*) finish_func));

  if (async_func)
    xml_printf (file, " glib:async-func=\"%s\"", gi_base_info_get_name ((GIBaseInfo*) async_func));

  xml_start_element (file, "return-value");

  write_ownership_transfer (gi_callable_info_get_caller_owns (info), file);

  if (gi_callable_info_may_return_null (info))
    xml_printf (file, " allow-none=\"1\"");

  if (gi_callable_info_skip_return (info))
    xml_printf (file, " skip=\"1\"");

  write_return_value_attributes (file, info);

  write_type_info (ns, type, file);

  xml_end_element (file, "return-value");

  if (gi_callable_info_get_n_args (info) <= 0)
    return;

  xml_start_element (file, "parameters");
  for (unsigned int i = 0; i < gi_callable_info_get_n_args (info); i++)
    {
      GIArgInfo *arg = gi_callable_info_get_arg (info, i);
      unsigned int closure_index, destroy_index;

      xml_start_element (file, "parameter");
      xml_printf (file, " name=\"%s\"",
                  gi_base_info_get_name ((GIBaseInfo *) arg));

      write_ownership_transfer (gi_arg_info_get_ownership_transfer (arg), file);

      switch (gi_arg_info_get_direction (arg))
        {
        case GI_DIRECTION_IN:
          break;
        case GI_DIRECTION_OUT:
          xml_printf (file, " direction=\"out\" caller-allocates=\"%s\"",
                      gi_arg_info_is_caller_allocates (arg) ? "1" : "0");
          break;
        case GI_DIRECTION_INOUT:
          xml_printf (file, " direction=\"inout\"");
          break;
        default:
          g_assert_not_reached ();
        }

      if (gi_arg_info_may_be_null (arg))
        xml_printf (file, " allow-none=\"1\"");

      if (gi_arg_info_is_return_value (arg))
        xml_printf (file, " retval=\"1\"");

      if (gi_arg_info_is_optional (arg))
        xml_printf (file, " optional=\"1\"");

      switch (gi_arg_info_get_scope (arg))
        {
        case GI_SCOPE_TYPE_INVALID:
          break;
        case GI_SCOPE_TYPE_CALL:
          xml_printf (file, " scope=\"call\"");
          break;
        case GI_SCOPE_TYPE_ASYNC:
          xml_printf (file, " scope=\"async\"");
          break;
        case GI_SCOPE_TYPE_NOTIFIED:
          xml_printf (file, " scope=\"notified\"");
          break;
        case GI_SCOPE_TYPE_FOREVER:
          xml_printf (file, " scope=\"forever\"");
          break;
        default:
          g_assert_not_reached ();
        }

      if (gi_arg_info_get_closure_index (arg, &closure_index))
        xml_printf (file, " closure=\"%u\"", closure_index);

      if (gi_arg_info_get_destroy_index (arg, &destroy_index))
        xml_printf (file, " destroy=\"%u\"", destroy_index);

      if (gi_arg_info_is_skip (arg))
        xml_printf (file, " skip=\"1\"");

      write_attributes (file, (GIBaseInfo*) arg);

      type = gi_arg_info_get_type_info (arg);
      write_type_info (ns, type, file);

      xml_end_element (file, "parameter");

      gi_base_info_unref ((GIBaseInfo *)arg);
    }

  xml_end_element (file, "parameters");
  gi_base_info_unref ((GIBaseInfo *)type);
}

static void
write_function_info (const char    *ns,
                     GIFunctionInfo *info,
                     Xml            *file)
{
  GIFunctionInfoFlags flags;
  const char *tag;
  const char *name;
  const char *symbol;
  gboolean deprecated;

  flags = gi_function_info_get_flags (info);
  name = gi_base_info_get_name ((GIBaseInfo *)info);
  symbol = gi_function_info_get_symbol (info);
  deprecated = gi_base_info_is_deprecated ((GIBaseInfo *)info);

  if (flags & GI_FUNCTION_IS_CONSTRUCTOR)
    tag = "constructor";
  else if (flags & GI_FUNCTION_IS_METHOD)
    tag = "method";
  else
    tag = "function";

  xml_start_element (file, tag);
  xml_printf (file, " name=\"%s\" c:identifier=\"%s\"",
              name, symbol);

  if ((flags & GI_FUNCTION_IS_SETTER) || (flags & GI_FUNCTION_IS_GETTER))
    {
      GIPropertyInfo *property = gi_function_info_get_property (info);

      if (property != NULL)
        {
          const char *property_name = gi_base_info_get_name ((GIBaseInfo *)property);

          if (flags & GI_FUNCTION_IS_SETTER)
            xml_printf (file, " glib:set-property=\"%s\"", property_name);
          else if (flags & GI_FUNCTION_IS_GETTER)
            xml_printf (file, " glib:get-property=\"%s\"", property_name);

          gi_base_info_unref ((GIBaseInfo *) property);
        }
    }

  if (deprecated)
    xml_printf (file, " deprecated=\"1\"");

  write_callable_info (ns, (GICallableInfo*)info, file);
  xml_end_element (file, tag);
}

static void
write_callback_info (const char     *ns,
                     GICallbackInfo *info,
                     Xml            *file)
{
  const char *name;
  gboolean deprecated;

  name = gi_base_info_get_name ((GIBaseInfo *)info);
  deprecated = gi_base_info_is_deprecated ((GIBaseInfo *)info);

  xml_start_element (file, "callback");
  xml_printf (file, " name=\"%s\"", name);

  if (deprecated)
    xml_printf (file, " deprecated=\"1\"");

  write_callable_info (ns, (GICallableInfo*)info, file);
  xml_end_element (file, "callback");
}

static void
write_struct_info (const char   *ns,
                   GIStructInfo *info,
                   Xml          *file)
{
  const char *name;
  const char *type_name;
  const char *type_init;
  const char *func;
  gboolean deprecated;
  gboolean is_gtype_struct;
  gboolean foreign;
  size_t size;
  unsigned int n_elts;

  name = gi_base_info_get_name ((GIBaseInfo *)info);
  deprecated = gi_base_info_is_deprecated ((GIBaseInfo *)info);

  type_name = gi_registered_type_info_get_type_name ((GIRegisteredTypeInfo*)info);
  type_init = gi_registered_type_info_get_type_init_function_name ((GIRegisteredTypeInfo*)info);

  if (gi_registered_type_info_is_boxed (GI_REGISTERED_TYPE_INFO (info)))
    {
      xml_start_element (file, "glib:boxed");
      xml_printf (file, " glib:name=\"%s\"", name);
    }
  else
    {
      xml_start_element (file, "record");
      xml_printf (file, " name=\"%s\"", name);
    }

  if (type_name != NULL)
    xml_printf (file, " glib:type-name=\"%s\" glib:get-type=\"%s\"", type_name, type_init);

  if (deprecated)
    xml_printf (file, " deprecated=\"1\"");

  is_gtype_struct = gi_struct_info_is_gtype_struct (info);
  if (is_gtype_struct)
    xml_printf (file, " glib:is-gtype-struct=\"1\"");

  func = gi_struct_info_get_copy_function_name (info);
  if (func)
    xml_printf (file, " copy-function=\"%s\"", func);

  func = gi_struct_info_get_free_function_name (info);
  if (func)
    xml_printf (file, " free-function=\"%s\"", func);

  write_attributes (file, (GIBaseInfo*) info);

  size = gi_struct_info_get_size (info);
  if (file->show_all)
    xml_printf (file, " size=\"%zu\"", size);

  foreign = gi_struct_info_is_foreign (info);
  if (foreign)
    xml_printf (file, " foreign=\"1\"");

  n_elts = gi_struct_info_get_n_fields (info) + gi_struct_info_get_n_methods (info);
  if (n_elts > 0)
    {
      for (unsigned int i = 0; i < gi_struct_info_get_n_fields (info); i++)
        {
          GIFieldInfo *field = gi_struct_info_get_field (info, i);
          write_field_info (ns, field, NULL, file);
          gi_base_info_unref ((GIBaseInfo *)field);
        }

      for (unsigned int i = 0; i < gi_struct_info_get_n_methods (info); i++)
        {
          GIFunctionInfo *function = gi_struct_info_get_method (info, i);
          write_function_info (ns, function, file);
          gi_base_info_unref ((GIBaseInfo *)function);
        }

    }

  xml_end_element_unchecked (file);
}

static void
write_value_info (const char *ns,
                  GIValueInfo *info,
                  Xml         *file)
{
  const char *name;
  int64_t value;
  char *value_str;
  gboolean deprecated;

  name = gi_base_info_get_name ((GIBaseInfo *)info);
  value = gi_value_info_get_value (info);
  deprecated = gi_base_info_is_deprecated ((GIBaseInfo *)info);

  xml_start_element (file, "member");
  value_str = g_strdup_printf ("%" G_GINT64_FORMAT, value);
  xml_printf (file, " name=\"%s\" value=\"%s\"", name, value_str);
  g_free (value_str);

  if (deprecated)
    xml_printf (file, " deprecated=\"1\"");

  write_attributes (file, (GIBaseInfo*) info);

  xml_end_element (file, "member");
}

static void
write_constant_value (const char *ns,
                      GITypeInfo *type,
                      GIArgument  *value,
                      Xml        *file)
{
  switch (gi_type_info_get_tag (type))
    {
    case GI_TYPE_TAG_BOOLEAN:
      xml_printf (file, "%d", value->v_boolean);
      break;
    case GI_TYPE_TAG_INT8:
      xml_printf (file, "%d", value->v_int8);
      break;
    case GI_TYPE_TAG_UINT8:
      xml_printf (file, "%d", value->v_uint8);
      break;
    case GI_TYPE_TAG_INT16:
      xml_printf (file, "%" G_GINT16_FORMAT, value->v_int16);
      break;
    case GI_TYPE_TAG_UINT16:
      xml_printf (file, "%" G_GUINT16_FORMAT, value->v_uint16);
      break;
    case GI_TYPE_TAG_INT32:
      xml_printf (file, "%" G_GINT32_FORMAT, value->v_int32);
      break;
    case GI_TYPE_TAG_UINT32:
      xml_printf (file, "%" G_GUINT32_FORMAT, value->v_uint32);
      break;
    case GI_TYPE_TAG_INT64:
      xml_printf (file, "%" G_GINT64_FORMAT, value->v_int64);
      break;
    case GI_TYPE_TAG_UINT64:
      xml_printf (file, "%" G_GUINT64_FORMAT, value->v_uint64);
      break;
    case GI_TYPE_TAG_FLOAT:
      xml_printf (file, "%f", (double)value->v_float);
      break;
    case GI_TYPE_TAG_DOUBLE:
      xml_printf (file, "%f", value->v_double);
      break;
    case GI_TYPE_TAG_UTF8:
    case GI_TYPE_TAG_FILENAME:
      xml_printf (file, "%s", value->v_string);
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
write_constant_info (const char     *ns,
                     GIConstantInfo *info,
                     Xml            *file)
{
  GITypeInfo *type;
  const char *name;
  GIArgument value;

  name = gi_base_info_get_name ((GIBaseInfo *)info);

  xml_start_element (file, "constant");
  xml_printf (file, " name=\"%s\"", name);

  type = gi_constant_info_get_type_info (info);
  xml_printf (file, " value=\"");

  gi_constant_info_get_value (info, &value);
  write_constant_value (ns, type, &value, file);
  xml_printf (file, "\"");

  write_type_info (ns, type, file);

  write_attributes (file, (GIBaseInfo*) info);

  xml_end_element (file, "constant");

  gi_base_info_unref ((GIBaseInfo *)type);
}


static void
write_enum_info (const char *ns,
                 GIEnumInfo *info,
                 Xml         *file)
{
  const char *name;
  const char *type_name;
  const char *type_init;
  const char *error_domain;
  gboolean deprecated;

  name = gi_base_info_get_name ((GIBaseInfo *)info);
  deprecated = gi_base_info_is_deprecated ((GIBaseInfo *)info);

  type_name = gi_registered_type_info_get_type_name ((GIRegisteredTypeInfo*)info);
  type_init = gi_registered_type_info_get_type_init_function_name ((GIRegisteredTypeInfo*)info);
  error_domain = gi_enum_info_get_error_domain (info);

  if (GI_IS_ENUM_INFO (info))
    xml_start_element (file, "enumeration");
  else
    xml_start_element (file, "bitfield");
  xml_printf (file, " name=\"%s\"", name);

  if (type_init)
    xml_printf (file, " glib:type-name=\"%s\" glib:get-type=\"%s\"", type_name, type_init);
  if (error_domain)
    xml_printf (file, " glib:error-domain=\"%s\"", error_domain);

  if (deprecated)
    xml_printf (file, " deprecated=\"1\"");

  write_attributes (file, (GIBaseInfo*) info);

  for (unsigned int i = 0; i < gi_enum_info_get_n_values (info); i++)
    {
      GIValueInfo *value = gi_enum_info_get_value (info, i);
      write_value_info (ns, value, file);
      gi_base_info_unref ((GIBaseInfo *)value);
    }

  xml_end_element_unchecked (file);
}

static void
write_signal_info (const char   *ns,
                   GISignalInfo *info,
                   Xml          *file)
{
  GSignalFlags flags;
  const char *name;
  gboolean deprecated;

  name = gi_base_info_get_name ((GIBaseInfo *)info);
  flags = gi_signal_info_get_flags (info);
  deprecated = gi_base_info_is_deprecated ((GIBaseInfo *)info);

  xml_start_element (file, "glib:signal");
  xml_printf (file, " name=\"%s\"", name);

  if (deprecated)
    xml_printf (file, " deprecated=\"1\"");

  if (flags & G_SIGNAL_RUN_FIRST)
    xml_printf (file, " when=\"FIRST\"");
  else if (flags & G_SIGNAL_RUN_LAST)
    xml_printf (file, " when=\"LAST\"");
  else if (flags & G_SIGNAL_RUN_CLEANUP)
    xml_printf (file, " when=\"CLEANUP\"");

  if (flags & G_SIGNAL_NO_RECURSE)
    xml_printf (file, " no-recurse=\"1\"");

  if (flags & G_SIGNAL_DETAILED)
    xml_printf (file, " detailed=\"1\"");

  if (flags & G_SIGNAL_ACTION)
    xml_printf (file, " action=\"1\"");

  if (flags & G_SIGNAL_NO_HOOKS)
    xml_printf (file, " no-hooks=\"1\"");

  write_callable_info (ns, (GICallableInfo*)info, file);

  xml_end_element (file, "glib:signal");
}

static void
write_vfunc_info (const char  *ns,
                  GIVFuncInfo *info,
                  Xml         *file)
{
  GIVFuncInfoFlags flags;
  const char *name;
  GIFunctionInfo *invoker;
  gboolean deprecated;
  size_t offset;

  name = gi_base_info_get_name ((GIBaseInfo *)info);
  flags = gi_vfunc_info_get_flags (info);
  deprecated = gi_base_info_is_deprecated ((GIBaseInfo *)info);
  offset = gi_vfunc_info_get_offset (info);
  invoker = gi_vfunc_info_get_invoker (info);

  xml_start_element (file, "virtual-method");
  xml_printf (file, " name=\"%s\"", name);

  if (deprecated)
    xml_printf (file, " deprecated=\"1\"");

  if (flags & GI_VFUNC_MUST_CHAIN_UP)
    xml_printf (file, " must-chain-up=\"1\"");

  if (flags & GI_VFUNC_MUST_OVERRIDE)
    xml_printf (file, " override=\"always\"");
  else if (flags & GI_VFUNC_MUST_NOT_OVERRIDE)
    xml_printf (file, " override=\"never\"");

  xml_printf (file, " offset=\"%zu\"", offset);

  if (invoker)
    {
      xml_printf (file, " invoker=\"%s\"", gi_base_info_get_name ((GIBaseInfo*)invoker));
      gi_base_info_unref ((GIBaseInfo *)invoker);
    }

  write_callable_info (ns, (GICallableInfo*)info, file);

  xml_end_element (file, "virtual-method");
}

static void
write_property_info (const char     *ns,
                     GIPropertyInfo *info,
                     Xml            *file)
{
  GParamFlags flags;
  const char *name;
  gboolean deprecated;
  GITypeInfo *type;

  name = gi_base_info_get_name ((GIBaseInfo *)info);
  flags = gi_property_info_get_flags (info);
  deprecated = gi_base_info_is_deprecated ((GIBaseInfo *)info);

  xml_start_element (file, "property");
  xml_printf (file, " name=\"%s\"", name);

  if (deprecated)
    xml_printf (file, " deprecated=\"1\"");

  /* Properties are assumed to be read-only (see also girwriter.py) */
  if (!(flags & G_PARAM_READABLE))
    xml_printf (file, " readable=\"0\"");
  if (flags & G_PARAM_WRITABLE)
    xml_printf (file, " writable=\"1\"");

  if (flags & G_PARAM_CONSTRUCT)
    xml_printf (file, " construct=\"1\"");

  if (flags & G_PARAM_CONSTRUCT_ONLY)
    xml_printf (file, " construct-only=\"1\"");

  if (flags & G_PARAM_READABLE)
    {
      GIFunctionInfo *getter = gi_property_info_get_getter (info);

      if (getter != NULL)
        {
          xml_printf (file, " getter=\"%s\"", gi_base_info_get_name ((GIBaseInfo *) getter));
          gi_base_info_unref ((GIBaseInfo *) getter);
        }
    }

  if (flags & G_PARAM_WRITABLE)
    {
      GIFunctionInfo *setter = gi_property_info_get_setter (info);

      if (setter != NULL)
        {
          xml_printf (file, " setter=\"%s\"", gi_base_info_get_name ((GIBaseInfo *) setter));
          gi_base_info_unref ((GIBaseInfo *) setter);
        }
    }

  write_ownership_transfer (gi_property_info_get_ownership_transfer (info), file);

  write_attributes (file, (GIBaseInfo*) info);

  type = gi_property_info_get_type_info (info);

  write_type_info (ns, type, file);

  xml_end_element (file, "property");
}

static void
write_object_info (const char   *ns,
                   GIObjectInfo *info,
                   Xml          *file)
{
  const char *name;
  const char *type_name;
  const char *type_init;
  const char *func;
  gboolean deprecated;
  gboolean is_abstract;
  gboolean is_fundamental;
  gboolean is_final;
  GIObjectInfo *pnode;
  GIStructInfo *class_struct;

  name = gi_base_info_get_name ((GIBaseInfo *)info);
  deprecated = gi_base_info_is_deprecated ((GIBaseInfo *)info);
  is_abstract = gi_object_info_get_abstract (info);
  is_fundamental = gi_object_info_get_fundamental (info);
  is_final = gi_object_info_get_final (info);

  type_name = gi_registered_type_info_get_type_name ((GIRegisteredTypeInfo*)info);
  type_init = gi_registered_type_info_get_type_init_function_name ((GIRegisteredTypeInfo*)info);
  xml_start_element (file, "class");
  xml_printf (file, " name=\"%s\"", name);

  pnode = gi_object_info_get_parent (info);
  if (pnode)
    {
      write_type_name_attribute (ns, (GIBaseInfo *)pnode, "parent", file);
      gi_base_info_unref ((GIBaseInfo *)pnode);
    }

  class_struct = gi_object_info_get_class_struct (info);
  if (class_struct)
    {
      write_type_name_attribute (ns, (GIBaseInfo*) class_struct, "glib:type-struct", file);
      gi_base_info_unref ((GIBaseInfo*)class_struct);
    }

  if (is_abstract)
    xml_printf (file, " abstract=\"1\"");

  if (is_final)
    xml_printf (file, " final=\"1\"");

  xml_printf (file, " glib:type-name=\"%s\" glib:get-type=\"%s\"", type_name, type_init);

  if (is_fundamental)
    xml_printf (file, " glib:fundamental=\"1\"");

  func = gi_object_info_get_unref_function_name (info);
  if (func)
    xml_printf (file, " glib:unref-function=\"%s\"", func);

  func = gi_object_info_get_ref_function_name (info);
  if (func)
    xml_printf (file, " glib:ref-function=\"%s\"", func);

  func = gi_object_info_get_set_value_function_name (info);
  if (func)
    xml_printf (file, " glib:set-value-function=\"%s\"", func);

  func = gi_object_info_get_get_value_function_name (info);
  if (func)
    xml_printf (file, " glib:get-value-function=\"%s\"", func);

  if (deprecated)
    xml_printf (file, " deprecated=\"1\"");

  write_attributes (file, (GIBaseInfo*) info);

  if (gi_object_info_get_n_interfaces (info) > 0)
    {
      for (unsigned int i = 0; i < gi_object_info_get_n_interfaces (info); i++)
        {
          GIInterfaceInfo *imp = gi_object_info_get_interface (info, i);
          xml_start_element (file, "implements");
          write_type_name_attribute (ns, (GIBaseInfo *)imp, "name", file);
          xml_end_element (file, "implements");
          gi_base_info_unref ((GIBaseInfo*)imp);
        }
    }

  for (unsigned int i = 0; i < gi_object_info_get_n_fields (info); i++)
    {
      GIFieldInfo *field = gi_object_info_get_field (info, i);
      write_field_info (ns, field, NULL, file);
      gi_base_info_unref ((GIBaseInfo *)field);
    }

  for (unsigned int i = 0; i < gi_object_info_get_n_methods (info); i++)
    {
      GIFunctionInfo *function = gi_object_info_get_method (info, i);
      write_function_info (ns, function, file);
      gi_base_info_unref ((GIBaseInfo *)function);
    }

  for (unsigned int i = 0; i < gi_object_info_get_n_properties (info); i++)
    {
      GIPropertyInfo *prop = gi_object_info_get_property (info, i);
      write_property_info (ns, prop, file);
      gi_base_info_unref ((GIBaseInfo *)prop);
    }

  for (unsigned int i = 0; i < gi_object_info_get_n_signals (info); i++)
    {
      GISignalInfo *signal = gi_object_info_get_signal (info, i);
      write_signal_info (ns, signal, file);
      gi_base_info_unref ((GIBaseInfo *)signal);
    }

  for (unsigned int i = 0; i < gi_object_info_get_n_vfuncs (info); i++)
    {
      GIVFuncInfo *vfunc = gi_object_info_get_vfunc (info, i);
      write_vfunc_info (ns, vfunc, file);
      gi_base_info_unref ((GIBaseInfo *)vfunc);
    }

  for (unsigned int i = 0; i < gi_object_info_get_n_constants (info); i++)
    {
      GIConstantInfo *constant = gi_object_info_get_constant (info, i);
      write_constant_info (ns, constant, file);
      gi_base_info_unref ((GIBaseInfo *)constant);
    }

  xml_end_element (file, "class");
}

static void
write_interface_info (const char      *ns,
                      GIInterfaceInfo *info,
                      Xml             *file)
{
  const char *name;
  const char *type_name;
  const char *type_init;
  GIStructInfo *class_struct;
  gboolean deprecated;

  name = gi_base_info_get_name ((GIBaseInfo *)info);
  deprecated = gi_base_info_is_deprecated ((GIBaseInfo *)info);

  type_name = gi_registered_type_info_get_type_name ((GIRegisteredTypeInfo*)info);
  type_init = gi_registered_type_info_get_type_init_function_name ((GIRegisteredTypeInfo*)info);
  xml_start_element (file, "interface");
  xml_printf (file, " name=\"%s\" glib:type-name=\"%s\" glib:get-type=\"%s\"",
             name, type_name, type_init);

  class_struct = gi_interface_info_get_iface_struct (info);
  if (class_struct)
    {
      write_type_name_attribute (ns, (GIBaseInfo*) class_struct, "glib:type-struct", file);
      gi_base_info_unref ((GIBaseInfo*)class_struct);
    }

  if (deprecated)
    xml_printf (file, " deprecated=\"1\"");

  write_attributes (file, (GIBaseInfo*) info);

  if (gi_interface_info_get_n_prerequisites (info) > 0)
    {
      for (unsigned int i = 0; i < gi_interface_info_get_n_prerequisites (info); i++)
        {
          GIBaseInfo *req = gi_interface_info_get_prerequisite (info, i);

          xml_start_element (file, "prerequisite");
          write_type_name_attribute (ns, req, "name", file);

          xml_end_element_unchecked (file);
          gi_base_info_unref (req);
        }
    }

  for (unsigned int i = 0; i < gi_interface_info_get_n_methods (info); i++)
    {
      GIFunctionInfo *function = gi_interface_info_get_method (info, i);
      write_function_info (ns, function, file);
      gi_base_info_unref ((GIBaseInfo *)function);
    }

  for (unsigned int i = 0; i < gi_interface_info_get_n_properties (info); i++)
    {
      GIPropertyInfo *prop = gi_interface_info_get_property (info, i);
      write_property_info (ns, prop, file);
      gi_base_info_unref ((GIBaseInfo *)prop);
    }

  for (unsigned int i = 0; i < gi_interface_info_get_n_signals (info); i++)
    {
      GISignalInfo *signal = gi_interface_info_get_signal (info, i);
      write_signal_info (ns, signal, file);
      gi_base_info_unref ((GIBaseInfo *)signal);
    }

  for (unsigned int i = 0; i < gi_interface_info_get_n_vfuncs (info); i++)
    {
      GIVFuncInfo *vfunc = gi_interface_info_get_vfunc (info, i);
      write_vfunc_info (ns, vfunc, file);
      gi_base_info_unref ((GIBaseInfo *)vfunc);
    }

  for (unsigned int i = 0; i < gi_interface_info_get_n_constants (info); i++)
    {
      GIConstantInfo *constant = gi_interface_info_get_constant (info, i);
      write_constant_info (ns, constant, file);
      gi_base_info_unref ((GIBaseInfo *)constant);
    }

  xml_end_element (file, "interface");
}

static void
write_union_info (const char *ns,
                  GIUnionInfo *info,
                  Xml         *file)
{
  const char *name;
  const char *type_name;
  const char *type_init;
  const char *func;
  gboolean deprecated;
  size_t size;

  name = gi_base_info_get_name ((GIBaseInfo *)info);
  deprecated = gi_base_info_is_deprecated ((GIBaseInfo *)info);

  type_name = gi_registered_type_info_get_type_name ((GIRegisteredTypeInfo*)info);
  type_init = gi_registered_type_info_get_type_init_function_name ((GIRegisteredTypeInfo*)info);

  /* FIXME: Add support for boxed unions */
  xml_start_element (file, "union");
  xml_printf (file, " name=\"%s\"", name);

  if (type_name)
    xml_printf (file, " type-name=\"%s\" get-type=\"%s\"", type_name, type_init);

  if (deprecated)
    xml_printf (file, " deprecated=\"1\"");

  size = gi_union_info_get_size (info);
  if (file->show_all)
    xml_printf (file, " size=\"%" G_GSIZE_FORMAT "\"", size);

  func = gi_union_info_get_copy_function_name (info);
  if (func)
    xml_printf (file, " copy-function=\"%s\"", func);

  func = gi_union_info_get_free_function_name (info);
  if (func)
    xml_printf (file, " free-function=\"%s\"", func);

  write_attributes (file, (GIBaseInfo*) info);

  if (gi_union_info_is_discriminated (info))
    {
      size_t offset;
      GITypeInfo *type;

      gi_union_info_get_discriminator_offset (info, &offset);
      type = gi_union_info_get_discriminator_type (info);

      xml_start_element (file, "discriminator");
      xml_printf (file, " offset=\"%zu\" type=\"", offset);
      write_type_info (ns, type, file);
      xml_end_element (file, "discriminator");
      gi_base_info_unref ((GIBaseInfo *)type);
    }

  for (unsigned int i = 0; i < gi_union_info_get_n_fields (info); i++)
    {
      GIFieldInfo *field = gi_union_info_get_field (info, i);
      GIConstantInfo *constant = gi_union_info_get_discriminator (info, i);
      write_field_info (ns, field, constant, file);
      gi_base_info_unref ((GIBaseInfo *)field);
      if (constant)
        gi_base_info_unref ((GIBaseInfo *)constant);
    }

  for (unsigned int i = 0; i < gi_union_info_get_n_methods (info); i++)
    {
      GIFunctionInfo *function = gi_union_info_get_method (info, i);
      write_function_info (ns, function, file);
      gi_base_info_unref ((GIBaseInfo *)function);
    }

  xml_end_element (file, "union");
}


/**
 * gi_ir_writer_write:
 * @repository: repository containing @ns
 * @filename: (type filename): filename to write to
 * @ns: GIR namespace to write
 * @needs_prefix: if the filename needs prefixing
 * @show_all: if field size calculations should be included
 *
 * Writes the output of a typelib represented by @ns
 * into a GIR xml file named @filename.
 *
 * Since: 2.80
 */
void
gi_ir_writer_write (GIRepository *repository,
                    const char *filename,
                    const char *ns,
                    gboolean    needs_prefix,
                    gboolean    show_all)
{
  FILE *ofile;
  size_t i;
  unsigned int j;
  char **dependencies;
  Xml *xml;

  if (filename == NULL)
    ofile = stdout;
  else
    {
      char *full_filename;

      if (needs_prefix)
        full_filename = g_strdup_printf ("%s-%s", ns, filename);
      else
        full_filename = g_strdup (filename);
      ofile = g_fopen (filename, "we");

      if (ofile == NULL)
        {
          g_fprintf (stderr, "failed to open '%s': %s\n",
                     full_filename, g_strerror (errno));
          g_free (full_filename);

          return;
        }

      g_free (full_filename);
    }

  xml = xml_open (ofile);
  xml->show_all = show_all;
  xml_printf (xml, "<?xml version=\"1.0\"?>\n");
  xml_start_element (xml, "repository");
  xml_printf (xml, " version=\"1.0\"\n"
              "            xmlns=\"http://www.gtk.org/introspection/core/1.0\"\n"
              "            xmlns:c=\"http://www.gtk.org/introspection/c/1.0\"\n"
              "            xmlns:glib=\"http://www.gtk.org/introspection/glib/1.0\"");

  dependencies = gi_repository_get_immediate_dependencies (repository, ns, NULL);
  if (dependencies != NULL)
    {
      for (i = 0; dependencies[i]; i++)
        {
          char **parts = g_strsplit (dependencies[i], "-", 2);
          xml_start_element (xml, "include");
          xml_printf (xml, " name=\"%s\" version=\"%s\"", parts[0], parts[1]);
          xml_end_element (xml, "include");
          g_strfreev (parts);
        }
    }

  if (TRUE)
    {
      const char * const *shared_libraries;
      const char *c_prefix;
      const char *cur_ns = ns;
      const char *cur_version;
      unsigned int n_infos;

      cur_version = gi_repository_get_version (repository, cur_ns);

      shared_libraries = gi_repository_get_shared_libraries (repository, cur_ns, NULL);
      c_prefix = gi_repository_get_c_prefix (repository, cur_ns);
      xml_start_element (xml, "namespace");
      xml_printf (xml, " name=\"%s\" version=\"%s\"", cur_ns, cur_version);
      if (shared_libraries != NULL)
        {
          char *shared_libraries_str = g_strjoinv (",", (char **) shared_libraries);
          xml_printf (xml, " shared-library=\"%s\"", shared_libraries_str);
          g_free (shared_libraries_str);
        }
      if (c_prefix)
        xml_printf (xml, " c:prefix=\"%s\"", c_prefix);

      n_infos = gi_repository_get_n_infos (repository, cur_ns);
      for (j = 0; j < n_infos; j++)
        {
          GIBaseInfo *info = gi_repository_get_info (repository, cur_ns, j);

          if (GI_IS_FUNCTION_INFO (info))
            write_function_info (ns, (GIFunctionInfo *)info, xml);
          else if (GI_IS_CALLBACK_INFO (info))
            write_callback_info (ns, (GICallbackInfo *)info, xml);
          else if (GI_IS_STRUCT_INFO (info))
            write_struct_info (ns, (GIStructInfo *)info, xml);
          else if (GI_IS_UNION_INFO (info))
            write_union_info (ns, (GIUnionInfo *)info, xml);
          else if (GI_IS_ENUM_INFO (info) ||
                   GI_IS_FLAGS_INFO (info))
            write_enum_info (ns, (GIEnumInfo *)info, xml);
          else if (GI_IS_CONSTANT_INFO (info))
            write_constant_info (ns, (GIConstantInfo *)info, xml);
          else if (GI_IS_OBJECT_INFO (info))
            write_object_info (ns, (GIObjectInfo *)info, xml);
          else if (GI_IS_INTERFACE_INFO (info))
            write_interface_info (ns, (GIInterfaceInfo *)info, xml);
          else
            g_error ("unknown info type %s", g_type_name (G_TYPE_FROM_INSTANCE (info)));

          gi_base_info_unref (info);
        }

      xml_end_element (xml, "namespace");
    }

  xml_end_element (xml, "repository");

  xml_free (xml);
}
