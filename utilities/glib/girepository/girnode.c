/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Typelib creation
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

#include "girnode-private.h"
#include "girepository-private.h"
#include "gitypelib-internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#define strtoll _strtoi64
#define strtoull _strtoui64
#endif

static gulong string_count = 0;
static gulong unique_string_count = 0;
static gulong string_size = 0;
static gulong unique_string_size = 0;
static gulong types_count = 0;
static gulong unique_types_count = 0;

void
gi_ir_node_init_stats (void)
{
  string_count = 0;
  unique_string_count = 0;
  string_size = 0;
  unique_string_size = 0;
  types_count = 0;
  unique_types_count = 0;
}

void
gi_ir_node_dump_stats (void)
{
  g_message ("%lu strings (%lu before sharing), %lu bytes (%lu before sharing)",
             unique_string_count, string_count, unique_string_size, string_size);
  g_message ("%lu types (%lu before sharing)", unique_types_count, types_count);
}

#define DO_ALIGNED_COPY(dest_addr, value, type) \
do {                                            \
        type tmp_var;                                \
        tmp_var = value;                        \
        memcpy(dest_addr, &tmp_var, sizeof(type));        \
} while(0)

#define ALIGN_VALUE(this, boundary) \
  (( ((unsigned long)(this)) + (((unsigned long)(boundary)) -1)) & (~(((unsigned long)(boundary))-1)))


const char *
gi_ir_node_type_to_string (GIIrNodeTypeId type)
{
  switch (type)
    {
    case GI_IR_NODE_FUNCTION:
      return "function";
    case GI_IR_NODE_CALLBACK:
      return "callback";
    case GI_IR_NODE_PARAM:
      return "param";
    case GI_IR_NODE_TYPE:
      return "type";
    case GI_IR_NODE_OBJECT:
      return "object";
    case GI_IR_NODE_INTERFACE:
      return "interface";
    case GI_IR_NODE_SIGNAL:
      return "signal";
    case GI_IR_NODE_PROPERTY:
      return "property";
    case GI_IR_NODE_VFUNC:
      return "vfunc";
    case GI_IR_NODE_FIELD:
      return "field";
    case GI_IR_NODE_ENUM:
      return "enum";
    case GI_IR_NODE_FLAGS:
      return "flags";
    case GI_IR_NODE_BOXED:
      return "boxed";
    case GI_IR_NODE_STRUCT:
      return "struct";
    case GI_IR_NODE_VALUE:
      return "value";
    case GI_IR_NODE_CONSTANT:
      return "constant";
    case GI_IR_NODE_XREF:
      return "xref";
    case GI_IR_NODE_UNION:
      return "union";
    default:
      return "unknown";
    }
}

GIIrNode *
gi_ir_node_new (GIIrNodeTypeId  type,
                GIIrModule     *module)
{
  GIIrNode *node = NULL;

  switch (type)
    {
   case GI_IR_NODE_FUNCTION:
   case GI_IR_NODE_CALLBACK:
      node = g_malloc0 (sizeof (GIIrNodeFunction));
      break;

   case GI_IR_NODE_PARAM:
      node = g_malloc0 (sizeof (GIIrNodeParam));
      break;

   case GI_IR_NODE_TYPE:
      node = g_malloc0 (sizeof (GIIrNodeType));
      break;

    case GI_IR_NODE_OBJECT:
    case GI_IR_NODE_INTERFACE:
      node = g_malloc0 (sizeof (GIIrNodeInterface));
      break;

    case GI_IR_NODE_SIGNAL:
      node = g_malloc0 (sizeof (GIIrNodeSignal));
      break;

    case GI_IR_NODE_PROPERTY:
      node = g_malloc0 (sizeof (GIIrNodeProperty));
      break;

    case GI_IR_NODE_VFUNC:
      node = g_malloc0 (sizeof (GIIrNodeFunction));
      break;

    case GI_IR_NODE_FIELD:
      node = g_malloc0 (sizeof (GIIrNodeField));
      break;

    case GI_IR_NODE_ENUM:
    case GI_IR_NODE_FLAGS:
      node = g_malloc0 (sizeof (GIIrNodeEnum));
      break;

    case GI_IR_NODE_BOXED:
      node = g_malloc0 (sizeof (GIIrNodeBoxed));
      break;

    case GI_IR_NODE_STRUCT:
      node = g_malloc0 (sizeof (GIIrNodeStruct));
      break;

    case GI_IR_NODE_VALUE:
      node = g_malloc0 (sizeof (GIIrNodeValue));
      break;

    case GI_IR_NODE_CONSTANT:
      node = g_malloc0 (sizeof (GIIrNodeConstant));
      break;

    case GI_IR_NODE_XREF:
      node = g_malloc0 (sizeof (GIIrNodeXRef));
      break;

    case GI_IR_NODE_UNION:
      node = g_malloc0 (sizeof (GIIrNodeUnion));
      break;

    default:
      g_error ("Unhandled node type %d", type);
      break;
    }

  node->type = type;
  node->module = module;
  node->offset = 0;
  node->attributes = g_hash_table_new_full (g_str_hash, g_str_equal,
                                            g_free, g_free);

  return node;
}

void
gi_ir_node_free (GIIrNode *node)
{
  GList *l;

  if (node == NULL)
    return;

  switch (node->type)
    {
    case GI_IR_NODE_FUNCTION:
    case GI_IR_NODE_CALLBACK:
      {
        GIIrNodeFunction *function = (GIIrNodeFunction *)node;

        g_free (node->name);
        g_free (function->symbol);
        g_free (function->property);
        g_free (function->async_func);
        g_free (function->sync_func);
        g_free (function->finish_func);
        gi_ir_node_free ((GIIrNode *)function->result);
        for (l = function->parameters; l; l = l->next)
          gi_ir_node_free ((GIIrNode *)l->data);
        g_list_free (function->parameters);
      }
      break;

    case GI_IR_NODE_TYPE:
      {
        GIIrNodeType *type = (GIIrNodeType *)node;

        g_free (node->name);
        gi_ir_node_free ((GIIrNode *)type->parameter_type1);
        gi_ir_node_free ((GIIrNode *)type->parameter_type2);

        g_free (type->giinterface);
        g_strfreev (type->errors);
        g_free (type->unparsed);
      }
      break;

    case GI_IR_NODE_PARAM:
      {
        GIIrNodeParam *param = (GIIrNodeParam *)node;

        g_free (node->name);
        gi_ir_node_free ((GIIrNode *)param->type);
      }
      break;

    case GI_IR_NODE_PROPERTY:
      {
        GIIrNodeProperty *property = (GIIrNodeProperty *)node;

        g_free (node->name);
        g_free (property->setter);
        g_free (property->getter);
        gi_ir_node_free ((GIIrNode *)property->type);
      }
      break;

    case GI_IR_NODE_SIGNAL:
      {
        GIIrNodeSignal *signal = (GIIrNodeSignal *)node;

        g_free (node->name);
        for (l = signal->parameters; l; l = l->next)
          gi_ir_node_free ((GIIrNode *)l->data);
        g_list_free (signal->parameters);
        gi_ir_node_free ((GIIrNode *)signal->result);
      }
      break;

    case GI_IR_NODE_VFUNC:
      {
        GIIrNodeVFunc *vfunc = (GIIrNodeVFunc *)node;

        g_free (node->name);
        g_free (vfunc->async_func);
        g_free (vfunc->sync_func);
        g_free (vfunc->finish_func);
        g_free (vfunc->invoker);
        for (l = vfunc->parameters; l; l = l->next)
          gi_ir_node_free ((GIIrNode *)l->data);
        g_list_free (vfunc->parameters);
        gi_ir_node_free ((GIIrNode *)vfunc->result);
      }
      break;

    case GI_IR_NODE_FIELD:
      {
        GIIrNodeField *field = (GIIrNodeField *)node;

        g_free (node->name);
        gi_ir_node_free ((GIIrNode *)field->type);
        gi_ir_node_free ((GIIrNode *)field->callback);
      }
      break;

    case GI_IR_NODE_OBJECT:
    case GI_IR_NODE_INTERFACE:
      {
        GIIrNodeInterface *iface = (GIIrNodeInterface *)node;

        g_free (node->name);
        g_free (iface->gtype_name);
        g_free (iface->gtype_init);
        g_free (iface->ref_func);
        g_free (iface->unref_func);
        g_free (iface->set_value_func);
        g_free (iface->get_value_func);


        g_free (iface->glib_type_struct);
        g_free (iface->parent);

        for (l = iface->interfaces; l; l = l->next)
          g_free ((GIIrNode *)l->data);
        g_list_free (iface->interfaces);

        g_list_free_full (iface->prerequisites, g_free);

        for (l = iface->members; l; l = l->next)
          gi_ir_node_free ((GIIrNode *)l->data);
        g_list_free (iface->members);

      }
      break;

    case GI_IR_NODE_VALUE:
      {
        g_free (node->name);
      }
      break;

    case GI_IR_NODE_ENUM:
    case GI_IR_NODE_FLAGS:
      {
        GIIrNodeEnum *enum_ = (GIIrNodeEnum *)node;

        g_free (node->name);
        g_free (enum_->gtype_name);
        g_free (enum_->gtype_init);
        g_free (enum_->error_domain);

        for (l = enum_->values; l; l = l->next)
          gi_ir_node_free ((GIIrNode *)l->data);
        g_list_free (enum_->values);

        for (l = enum_->methods; l; l = l->next)
          gi_ir_node_free ((GIIrNode *)l->data);
        g_list_free (enum_->methods);
      }
      break;

    case GI_IR_NODE_BOXED:
      {
        GIIrNodeBoxed *boxed = (GIIrNodeBoxed *)node;

        g_free (node->name);
        g_free (boxed->gtype_name);
        g_free (boxed->gtype_init);

        for (l = boxed->members; l; l = l->next)
          gi_ir_node_free ((GIIrNode *)l->data);
        g_list_free (boxed->members);
      }
      break;

    case GI_IR_NODE_STRUCT:
      {
        GIIrNodeStruct *struct_ = (GIIrNodeStruct *)node;

        g_free (node->name);
        g_free (struct_->gtype_name);
        g_free (struct_->gtype_init);
        g_free (struct_->copy_func);
        g_free (struct_->free_func);

        for (l = struct_->members; l; l = l->next)
          gi_ir_node_free ((GIIrNode *)l->data);
        g_list_free (struct_->members);
      }
      break;

    case GI_IR_NODE_CONSTANT:
      {
        GIIrNodeConstant *constant = (GIIrNodeConstant *)node;

        g_free (node->name);
        g_free (constant->value);
        gi_ir_node_free ((GIIrNode *)constant->type);
      }
      break;

    case GI_IR_NODE_XREF:
      {
        GIIrNodeXRef *xref = (GIIrNodeXRef *)node;

        g_free (node->name);
        g_free (xref->namespace);
      }
      break;

    case GI_IR_NODE_UNION:
      {
        GIIrNodeUnion *union_ = (GIIrNodeUnion *)node;

        g_free (node->name);
        g_free (union_->gtype_name);
        g_free (union_->gtype_init);
        g_free (union_->copy_func);
        g_free (union_->free_func);

        gi_ir_node_free ((GIIrNode *)union_->discriminator_type);
        g_clear_list (&union_->members, (GDestroyNotify) gi_ir_node_free);
        g_clear_list (&union_->discriminators, (GDestroyNotify) gi_ir_node_free);
      }
      break;

    default:
      g_error ("Unhandled node type %d", node->type);
      break;
    }

  g_hash_table_destroy (node->attributes);

  g_free (node);
}

/* returns the fixed size of the blob */
uint32_t
gi_ir_node_get_size (GIIrNode *node)
{
  GList *l;
  size_t size, n;

  switch (node->type)
    {
    case GI_IR_NODE_CALLBACK:
      size = sizeof (CallbackBlob);
      break;

    case GI_IR_NODE_FUNCTION:
      size = sizeof (FunctionBlob);
      break;

    case GI_IR_NODE_PARAM:
      /* See the comment in the GI_IR_NODE_PARAM/ArgBlob writing below */
      size = sizeof (ArgBlob) - sizeof (SimpleTypeBlob);
      break;

    case GI_IR_NODE_TYPE:
      size = sizeof (SimpleTypeBlob);
      break;

    case GI_IR_NODE_OBJECT:
      {
        GIIrNodeInterface *iface = (GIIrNodeInterface *)node;

        n = g_list_length (iface->interfaces);
        size = sizeof (ObjectBlob) + 2 * (n + (n % 2));

        for (l = iface->members; l; l = l->next)
          size += gi_ir_node_get_size ((GIIrNode *)l->data);
      }
      break;

    case GI_IR_NODE_INTERFACE:
      {
        GIIrNodeInterface *iface = (GIIrNodeInterface *)node;

        n = g_list_length (iface->prerequisites);
        size = sizeof (InterfaceBlob) + 2 * (n + (n % 2));

        for (l = iface->members; l; l = l->next)
          size += gi_ir_node_get_size ((GIIrNode *)l->data);
      }
      break;

    case GI_IR_NODE_ENUM:
    case GI_IR_NODE_FLAGS:
      {
        GIIrNodeEnum *enum_ = (GIIrNodeEnum *)node;

        size = sizeof (EnumBlob);
        for (l = enum_->values; l; l = l->next)
          size += gi_ir_node_get_size ((GIIrNode *)l->data);
        for (l = enum_->methods; l; l = l->next)
          size += gi_ir_node_get_size ((GIIrNode *)l->data);
      }
      break;

    case GI_IR_NODE_VALUE:
      size = sizeof (ValueBlob);
      break;

    case GI_IR_NODE_STRUCT:
      {
        GIIrNodeStruct *struct_ = (GIIrNodeStruct *)node;

        size = sizeof (StructBlob);
        for (l = struct_->members; l; l = l->next)
          size += gi_ir_node_get_size ((GIIrNode *)l->data);
      }
      break;

    case GI_IR_NODE_BOXED:
      {
        GIIrNodeBoxed *boxed = (GIIrNodeBoxed *)node;

        size = sizeof (StructBlob);
        for (l = boxed->members; l; l = l->next)
          size += gi_ir_node_get_size ((GIIrNode *)l->data);
      }
      break;

    case GI_IR_NODE_PROPERTY:
      size = sizeof (PropertyBlob);
      break;

    case GI_IR_NODE_SIGNAL:
      size = sizeof (SignalBlob);
      break;

    case GI_IR_NODE_VFUNC:
      size = sizeof (VFuncBlob);
      break;

    case GI_IR_NODE_FIELD:
      {
        GIIrNodeField *field = (GIIrNodeField *)node;

        size = sizeof (FieldBlob);
        if (field->callback)
          size += gi_ir_node_get_size ((GIIrNode *)field->callback);
      }
      break;

    case GI_IR_NODE_CONSTANT:
      size = sizeof (ConstantBlob);
      break;

    case GI_IR_NODE_XREF:
      size = 0;
      break;

    case GI_IR_NODE_UNION:
      {
        GIIrNodeUnion *union_ = (GIIrNodeUnion *)node;

        size = sizeof (UnionBlob);
        for (l = union_->members; l; l = l->next)
          size += gi_ir_node_get_size ((GIIrNode *)l->data);
        for (l = union_->discriminators; l; l = l->next)
          size += gi_ir_node_get_size ((GIIrNode *)l->data);
      }
      break;

    default:
      g_error ("Unhandled node type '%s'", gi_ir_node_type_to_string (node->type));
      size = 0;
    }

  g_debug ("node %p type '%s' size %zu", node,
           gi_ir_node_type_to_string (node->type), size);

  g_assert (size <= G_MAXUINT32);

  return (guint32) size;
}

static void
add_attribute_size (gpointer key, gpointer value, gpointer data)
{
  const char *key_str = key;
  const char *value_str = value;
  size_t *size_p = data;

  *size_p += sizeof (AttributeBlob);
  *size_p += ALIGN_VALUE (strlen (key_str) + 1, 4);
  *size_p += ALIGN_VALUE (strlen (value_str) + 1, 4);
}

/* returns the full size of the blob including variable-size parts (including attributes) */
static uint32_t
gi_ir_node_get_full_size_internal (GIIrNode *parent,
                                   GIIrNode *node)
{
  GList *l;
  size_t size, n;

  g_assert (node != NULL);

  g_debug ("node %p type '%s'", node,
           gi_ir_node_type_to_string (node->type));

  switch (node->type)
    {
    case GI_IR_NODE_CALLBACK:
      {
        GIIrNodeFunction *function = (GIIrNodeFunction *)node;
        size = sizeof (CallbackBlob);
        size += ALIGN_VALUE (strlen (node->name) + 1, 4);
        for (l = function->parameters; l; l = l->next)
          {
            size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)l->data);
          }
        size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)function->result);
      }
      break;

    case GI_IR_NODE_FUNCTION:
      {
        GIIrNodeFunction *function = (GIIrNodeFunction *)node;
        size = sizeof (FunctionBlob);
        size += ALIGN_VALUE (strlen (node->name) + 1, 4);
        size += ALIGN_VALUE (strlen (function->symbol) + 1, 4);
        for (l = function->parameters; l; l = l->next)
          size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)l->data);
        size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)function->result);
      }
      break;

    case GI_IR_NODE_PARAM:
      {
        GIIrNodeParam *param = (GIIrNodeParam *)node;

        /* See the comment in the GI_IR_NODE_PARAM/ArgBlob writing below */
        size = sizeof (ArgBlob) - sizeof (SimpleTypeBlob);
        if (node->name)
          size += ALIGN_VALUE (strlen (node->name) + 1, 4);
        size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)param->type);
      }
      break;

    case GI_IR_NODE_TYPE:
      {
        GIIrNodeType *type = (GIIrNodeType *)node;
        size = sizeof (SimpleTypeBlob);
        if (!GI_TYPE_TAG_IS_BASIC (type->tag))
          {
            g_debug ("node %p type tag '%s'", node,
                     gi_type_tag_to_string (type->tag));

            switch (type->tag)
              {
              case GI_TYPE_TAG_ARRAY:
                size = sizeof (ArrayTypeBlob);
                if (type->parameter_type1)
                  size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)type->parameter_type1);
                break;
              case GI_TYPE_TAG_INTERFACE:
                size += sizeof (InterfaceTypeBlob);
                break;
              case GI_TYPE_TAG_GLIST:
              case GI_TYPE_TAG_GSLIST:
                size += sizeof (ParamTypeBlob);
                if (type->parameter_type1)
                  size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)type->parameter_type1);
                break;
              case GI_TYPE_TAG_GHASH:
                size += sizeof (ParamTypeBlob) * 2;
                if (type->parameter_type1)
                  size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)type->parameter_type1);
                if (type->parameter_type2)
                  size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)type->parameter_type2);
                break;
              case GI_TYPE_TAG_ERROR:
                size += sizeof (ErrorTypeBlob);
                break;
              default:
                g_error ("Unknown type tag %d", type->tag);
                break;
              }
          }
      }
      break;

    case GI_IR_NODE_OBJECT:
      {
        GIIrNodeInterface *iface = (GIIrNodeInterface *)node;

        n = g_list_length (iface->interfaces);
        size = sizeof(ObjectBlob);
        if (iface->parent)
          size += ALIGN_VALUE (strlen (iface->parent) + 1, 4);
        if (iface->glib_type_struct)
          size += ALIGN_VALUE (strlen (iface->glib_type_struct) + 1, 4);
        size += ALIGN_VALUE (strlen (node->name) + 1, 4);
        size += ALIGN_VALUE (strlen (iface->gtype_name) + 1, 4);
        if (iface->gtype_init)
          size += ALIGN_VALUE (strlen (iface->gtype_init) + 1, 4);
        if (iface->ref_func)
          size += ALIGN_VALUE (strlen (iface->ref_func) + 1, 4);
        if (iface->unref_func)
          size += ALIGN_VALUE (strlen (iface->unref_func) + 1, 4);
        if (iface->set_value_func)
          size += ALIGN_VALUE (strlen (iface->set_value_func) + 1, 4);
        if (iface->get_value_func)
          size += ALIGN_VALUE (strlen (iface->get_value_func) + 1, 4);
        size += 2 * (n + (n % 2));

        for (l = iface->members; l; l = l->next)
          size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)l->data);
      }
      break;

    case GI_IR_NODE_INTERFACE:
      {
        GIIrNodeInterface *iface = (GIIrNodeInterface *)node;

        n = g_list_length (iface->prerequisites);
        size = sizeof (InterfaceBlob);
        size += ALIGN_VALUE (strlen (node->name) + 1, 4);
        size += ALIGN_VALUE (strlen (iface->gtype_name) + 1, 4);
        size += ALIGN_VALUE (strlen (iface->gtype_init) + 1, 4);
        size += 2 * (n + (n % 2));

        for (l = iface->members; l; l = l->next)
          size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)l->data);
      }
      break;

    case GI_IR_NODE_ENUM:
    case GI_IR_NODE_FLAGS:
      {
        GIIrNodeEnum *enum_ = (GIIrNodeEnum *)node;

        size = sizeof (EnumBlob);
        size += ALIGN_VALUE (strlen (node->name) + 1, 4);
        if (enum_->gtype_name)
          {
            size += ALIGN_VALUE (strlen (enum_->gtype_name) + 1, 4);
            size += ALIGN_VALUE (strlen (enum_->gtype_init) + 1, 4);
          }
        if (enum_->error_domain)
          size += ALIGN_VALUE (strlen (enum_->error_domain) + 1, 4);

        for (l = enum_->values; l; l = l->next)
          size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)l->data);
        for (l = enum_->methods; l; l = l->next)
          size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)l->data);
      }
      break;

    case GI_IR_NODE_VALUE:
      {
        size = sizeof (ValueBlob);
        size += ALIGN_VALUE (strlen (node->name) + 1, 4);
      }
      break;

    case GI_IR_NODE_STRUCT:
      {
        GIIrNodeStruct *struct_ = (GIIrNodeStruct *)node;

        size = sizeof (StructBlob);
        size += ALIGN_VALUE (strlen (node->name) + 1, 4);
        if (struct_->gtype_name)
          size += ALIGN_VALUE (strlen (struct_->gtype_name) + 1, 4);
        if (struct_->gtype_init)
          size += ALIGN_VALUE (strlen (struct_->gtype_init) + 1, 4);
        if (struct_->copy_func)
          size += ALIGN_VALUE (strlen (struct_->copy_func) + 1, 4);
        if (struct_->free_func)
          size += ALIGN_VALUE (strlen (struct_->free_func) + 1, 4);
        for (l = struct_->members; l; l = l->next)
          size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)l->data);
      }
      break;

    case GI_IR_NODE_BOXED:
      {
        GIIrNodeBoxed *boxed = (GIIrNodeBoxed *)node;

        size = sizeof (StructBlob);
        size += ALIGN_VALUE (strlen (node->name) + 1, 4);
        if (boxed->gtype_name)
          {
            size += ALIGN_VALUE (strlen (boxed->gtype_name) + 1, 4);
            size += ALIGN_VALUE (strlen (boxed->gtype_init) + 1, 4);
          }
        for (l = boxed->members; l; l = l->next)
          size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)l->data);
      }
      break;

    case GI_IR_NODE_PROPERTY:
      {
        GIIrNodeProperty *prop = (GIIrNodeProperty *)node;

        size = sizeof (PropertyBlob);
        size += ALIGN_VALUE (strlen (node->name) + 1, 4);
        size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)prop->type);
      }
      break;

    case GI_IR_NODE_SIGNAL:
      {
        GIIrNodeSignal *signal = (GIIrNodeSignal *)node;

        size = sizeof (SignalBlob);
        size += ALIGN_VALUE (strlen (node->name) + 1, 4);
        for (l = signal->parameters; l; l = l->next)
          size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)l->data);
        size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)signal->result);
      }
      break;

    case GI_IR_NODE_VFUNC:
      {
        GIIrNodeVFunc *vfunc = (GIIrNodeVFunc *)node;

        size = sizeof (VFuncBlob);
        size += ALIGN_VALUE (strlen (node->name) + 1, 4);
        for (l = vfunc->parameters; l; l = l->next)
          size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)l->data);
        size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)vfunc->result);
      }
      break;

    case GI_IR_NODE_FIELD:
      {
        GIIrNodeField *field = (GIIrNodeField *)node;

        size = sizeof (FieldBlob);
        size += ALIGN_VALUE (strlen (node->name) + 1, 4);
        if (field->callback)
          size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)field->callback);
        else
          size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)field->type);
      }
      break;

    case GI_IR_NODE_CONSTANT:
      {
        GIIrNodeConstant *constant = (GIIrNodeConstant *)node;

        size = sizeof (ConstantBlob);
        size += ALIGN_VALUE (strlen (node->name) + 1, 4);
        /* FIXME non-string values */
        size += ALIGN_VALUE (strlen (constant->value) + 1, 4);
        size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)constant->type);
      }
      break;

    case GI_IR_NODE_XREF:
      {
        GIIrNodeXRef *xref = (GIIrNodeXRef *)node;

        size = 0;
        size += ALIGN_VALUE (strlen (node->name) + 1, 4);
        size += ALIGN_VALUE (strlen (xref->namespace) + 1, 4);
      }
      break;

    case GI_IR_NODE_UNION:
      {
        GIIrNodeUnion *union_ = (GIIrNodeUnion *)node;

        size = sizeof (UnionBlob);
        size += ALIGN_VALUE (strlen (node->name) + 1, 4);
        if (union_->gtype_name)
          size += ALIGN_VALUE (strlen (union_->gtype_name) + 1, 4);
        if (union_->gtype_init)
          size += ALIGN_VALUE (strlen (union_->gtype_init) + 1, 4);
        if (union_->copy_func)
          size += ALIGN_VALUE (strlen (union_->copy_func) + 1, 4);
        if (union_->free_func)
          size += ALIGN_VALUE (strlen (union_->free_func) + 1, 4);
        for (l = union_->members; l; l = l->next)
          size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)l->data);
        for (l = union_->discriminators; l; l = l->next)
          size += gi_ir_node_get_full_size_internal (node, (GIIrNode *)l->data);
      }
      break;

    default:
      g_error ("Unknown type tag %d", node->type);
      size = 0;
    }

  g_debug ("node %s%s%s%p type '%s' full size %zu",
           node->name ? "'" : "",
           node->name ? node->name : "",
           node->name ? "' " : "",
           node, gi_ir_node_type_to_string (node->type), size);

  g_hash_table_foreach (node->attributes, add_attribute_size, &size);

  g_assert (size <= G_MAXUINT32);

  return size;
}

uint32_t
gi_ir_node_get_full_size (GIIrNode *node)
{
  return gi_ir_node_get_full_size_internal (NULL, node);
}

int
gi_ir_node_cmp (GIIrNode *node,
                GIIrNode *other)
{
  if (node->type < other->type)
    return -1;
  else if (node->type > other->type)
    return 1;
  else
    return strcmp (node->name, other->name);
}

gboolean
gi_ir_node_can_have_member (GIIrNode *node)
{
  switch (node->type)
    {
    case GI_IR_NODE_OBJECT:
    case GI_IR_NODE_INTERFACE:
    case GI_IR_NODE_BOXED:
    case GI_IR_NODE_STRUCT:
    case GI_IR_NODE_UNION:
      return TRUE;
    /* list others individually rather than with default: so that compiler
     * warns if new node types are added without adding them to the switch
     */
    case GI_IR_NODE_INVALID:
    case GI_IR_NODE_FUNCTION:
    case GI_IR_NODE_CALLBACK:
    case GI_IR_NODE_ENUM:
    case GI_IR_NODE_FLAGS:
    case GI_IR_NODE_CONSTANT:
    case GI_IR_NODE_INVALID_0:
    case GI_IR_NODE_PARAM:
    case GI_IR_NODE_TYPE:
    case GI_IR_NODE_PROPERTY:
    case GI_IR_NODE_SIGNAL:
    case GI_IR_NODE_VALUE:
    case GI_IR_NODE_VFUNC:
    case GI_IR_NODE_FIELD:
    case GI_IR_NODE_XREF:
      return FALSE;
    default:
      g_assert_not_reached ();
    };
  return FALSE;
}

void
gi_ir_node_add_member (GIIrNode         *node,
                       GIIrNodeFunction *member)
{
  g_return_if_fail (node != NULL);
  g_return_if_fail (member != NULL);

  switch (node->type)
    {
    case GI_IR_NODE_OBJECT:
    case GI_IR_NODE_INTERFACE:
      {
        GIIrNodeInterface *iface = (GIIrNodeInterface *)node;
        iface->members =
          g_list_insert_sorted (iface->members, member,
                                (GCompareFunc) gi_ir_node_cmp);
        break;
      }
    case GI_IR_NODE_BOXED:
      {
        GIIrNodeBoxed *boxed = (GIIrNodeBoxed *)node;
        boxed->members =
          g_list_insert_sorted (boxed->members, member,
                                (GCompareFunc) gi_ir_node_cmp);
        break;
      }
    case GI_IR_NODE_STRUCT:
      {
        GIIrNodeStruct *struct_ = (GIIrNodeStruct *)node;
        struct_->members =
          g_list_insert_sorted (struct_->members, member,
                                (GCompareFunc) gi_ir_node_cmp);
        break;
      }
    case GI_IR_NODE_UNION:
      {
        GIIrNodeUnion *union_ = (GIIrNodeUnion *)node;
        union_->members =
          g_list_insert_sorted (union_->members, member,
                                (GCompareFunc) gi_ir_node_cmp);
        break;
      }
    default:
      g_error ("Cannot add a member to unknown type tag type %d",
               node->type);
      break;
    }
}

const char *
gi_ir_node_param_direction_string (GIIrNodeParam * node)
{
  if (node->out)
    {
      if (node->in)
        return "in-out";
      else
        return "out";
    }
  return "in";
}

static int64_t
parse_int_value (const char *str)
{
  return g_ascii_strtoll (str, NULL, 0);
}

static uint64_t
parse_uint_value (const char *str)
{
  return g_ascii_strtoull (str, NULL, 0);
}

static double
parse_float_value (const char *str)
{
  return g_ascii_strtod (str, NULL);
}

static gboolean
parse_boolean_value (const char *str)
{
  if (g_ascii_strcasecmp (str, "TRUE") == 0)
    return TRUE;

  if (g_ascii_strcasecmp (str, "FALSE") == 0)
    return FALSE;

  return parse_int_value (str) ? TRUE : FALSE;
}

static GIIrNode *
find_entry_node (GIIrTypelibBuild *build,
                 const char       *name,
                 uint16_t         *idx)

{
  GIIrModule *module = build->module;
  GList *l;
  size_t i;
  unsigned int n_names;
  char **names;
  GIIrNode *result = NULL;

  g_assert (name != NULL);
  g_assert (strlen (name) > 0);

  names = g_strsplit (name, ".", 0);
  n_names = g_strv_length (names);
  if (n_names > 2)
    g_error ("Too many name parts");

  for (l = module->entries, i = 1; l; l = l->next, i++)
    {
      GIIrNode *node = (GIIrNode *)l->data;

      if (n_names > 1)
        {
          if (node->type != GI_IR_NODE_XREF)
            continue;

          if (((GIIrNodeXRef *)node)->namespace == NULL ||
              strcmp (((GIIrNodeXRef *)node)->namespace, names[0]) != 0)
            continue;
        }

      if (strcmp (node->name, names[n_names - 1]) == 0)
        {
          if (idx)
            *idx = i;

          result = node;
          goto out;
        }
    }

  if (n_names > 1)
    {
      GIIrNode *node = gi_ir_node_new (GI_IR_NODE_XREF, module);

      ((GIIrNodeXRef *)node)->namespace = g_strdup (names[0]);
      node->name = g_strdup (names[1]);

      module->entries = g_list_append (module->entries, node);

      if (idx)
        *idx = g_list_length (module->entries);

      result = node;

      g_debug ("Creating XREF: %s %s", names[0], names[1]);

      goto out;
    }

  
  gi_ir_module_fatal (build, 0, "type reference '%s' not found", name);
 out:

  g_strfreev (names);

  return result;
}

static uint16_t
find_entry (GIIrTypelibBuild *build,
            const char       *name)
{
  uint16_t idx = 0;

  find_entry_node (build, name, &idx);

  return idx;
}

static GIIrModule *
find_namespace (GIIrModule *module,
                const char *name)
{
  GIIrModule *target;
  GList *l;
  
  if (strcmp (module->name, name) == 0)
    return module;

  for (l = module->include_modules; l; l = l->next)
    {
      GIIrModule *submodule = l->data;

     if (strcmp (submodule->name, name) == 0)
       return submodule;

      target = find_namespace (submodule, name);
      if (target)
       return target;
    }
  return NULL;
}

GIIrNode *
gi_ir_find_node (GIIrTypelibBuild *build,
                 GIIrModule       *src_module,
                 const char       *name)
{
  GList *l;
  GIIrNode *return_node = NULL;
  char **names = g_strsplit (name, ".", 0);
  unsigned n_names = g_strv_length (names);
  const char *target_name;
  GIIrModule *target_module;

  if (n_names == 1)
    {
      target_module = src_module;
      target_name = name;
    }
  else
    {
      target_module = find_namespace (build->module, names[0]);
      target_name = names[1];
    }

  /* find_namespace() may return NULL. */
  if (target_module == NULL)
      goto done;

  for (l = target_module->entries; l; l = l->next)
    {
      GIIrNode *node = (GIIrNode *)l->data;

      if (strcmp (node->name, target_name) == 0)
        {
          return_node = node;
          break;
        }
    }

done:
  g_strfreev (names);

  return return_node;
}

static int
get_index_of_member_type (GIIrNodeInterface *node,
                          GIIrNodeTypeId     type,
                          const char        *name)
{
  int index = -1;
  GList *l;

  for (l = node->members; l; l = l->next)
    {
      GIIrNode *member_node = l->data;

      if (member_node->type != type)
        continue;

      index++;

      if (strcmp (member_node->name, name) == 0)
        break;
    }

  return index;
}

static int
get_index_for_function (GIIrTypelibBuild *build,
                        GIIrNode         *parent,
                        const char        *name)
{
  if (parent == NULL)
    {
      uint16_t index = find_entry (build, name);
      if (index == 0)
        return -1;

      return index;      
    }

  int index = get_index_of_member_type ((GIIrNodeInterface *) parent, GI_IR_NODE_FUNCTION, name);
  if (index != -1)
    return index;

  return -1;
}

static void
serialize_type (GIIrTypelibBuild *build,
                GIIrNodeType     *node,
                GString          *str)
{
  size_t i;

  if (GI_TYPE_TAG_IS_BASIC (node->tag))
    {
      g_string_append_printf (str, "%s%s", gi_type_tag_to_string (node->tag),
                              node->is_pointer ? "*" : "");
    }
  else if (node->tag == GI_TYPE_TAG_ARRAY)
    {
      if (node->array_type == GI_ARRAY_TYPE_C)
        {
          serialize_type (build, node->parameter_type1, str);
          g_string_append (str, "[");

          if (node->has_length)
            g_string_append_printf (str, "length=%d", node->length);
          else if (node->has_size)
            g_string_append_printf (str, "fixed-size=%" G_GSIZE_FORMAT, node->size);

          if (node->zero_terminated)
            g_string_append_printf (str, "%szero-terminated=1",
                                    node->has_length ? "," : "");

          g_string_append (str, "]");
          if (node->is_pointer)
            g_string_append (str, "*");
        }
      else if (node->array_type == GI_ARRAY_TYPE_BYTE_ARRAY)
        {
          /* We on purpose skip serializing parameter_type1, which should
             always be void*
          */
          g_string_append (str, "GByteArray");
        }
      else
        {
          if (node->array_type == GI_ARRAY_TYPE_ARRAY)
            g_string_append (str, "GArray");
          else
            g_string_append (str, "GPtrArray");
          if (node->parameter_type1)
            {
              g_string_append (str, "<");
              serialize_type (build, node->parameter_type1, str);
              g_string_append (str, ">");
            }
        }
    }
  else if (node->tag == GI_TYPE_TAG_INTERFACE)
    {
      GIIrNode *iface;
      char *name;

      iface = find_entry_node (build, node->giinterface, NULL);
      if (iface)
        {
          if (iface->type == GI_IR_NODE_XREF)
            g_string_append_printf (str, "%s.", ((GIIrNodeXRef *)iface)->namespace);
          name = iface->name;
        }
      else
        {
          g_warning ("Interface for type reference %s not found", node->giinterface);
          name = node->giinterface;
        }

      g_string_append_printf (str, "%s%s", name,
                              node->is_pointer ? "*" : "");
    }
  else if (node->tag == GI_TYPE_TAG_GLIST)
    {
      g_string_append (str, "GList");
      if (node->parameter_type1)
        {
          g_string_append (str, "<");
          serialize_type (build, node->parameter_type1, str);
          g_string_append (str, ">");
        }
    }
  else if (node->tag == GI_TYPE_TAG_GSLIST)
    {
      g_string_append (str, "GSList");
      if (node->parameter_type1)
        {
          g_string_append (str, "<");
          serialize_type (build, node->parameter_type1, str);
          g_string_append (str, ">");
        }
    }
  else if (node->tag == GI_TYPE_TAG_GHASH)
    {
      g_string_append (str, "GHashTable");
      if (node->parameter_type1)
        {
          g_string_append (str, "<");
          serialize_type (build, node->parameter_type1, str);
          g_string_append (str, ",");
          serialize_type (build, node->parameter_type2, str);
          g_string_append (str, ">");
        }
    }
  else if (node->tag == GI_TYPE_TAG_ERROR)
    {
      g_string_append (str, "GError");
      if (node->errors)
        {
          g_string_append (str, "<");
          for (i = 0; node->errors[i]; i++)
            {
              if (i > 0)
                g_string_append (str, ",");
              g_string_append (str, node->errors[i]);
            }
          g_string_append (str, ">");
        }
    }
}

static void
gi_ir_node_build_members (GList            **members,
                          GIIrNodeTypeId     type,
                          uint16_t          *count,
                          GIIrNode          *parent,
                          GIIrTypelibBuild  *build,
                          uint32_t           *offset,
                          uint32_t           *offset2,
                          uint16_t          *count2)
{
  GList *l = *members;

  while (l)
    {
      GIIrNode *member = (GIIrNode *)l->data;
      GList *next = l->next;

      if (member->type == type)
        {
          (*count)++;
          gi_ir_node_build_typelib (member, parent, build, offset, offset2, count2);
          *members = g_list_delete_link (*members, l);
        }
      l = next;
    }
}

static void
gi_ir_node_check_unhandled_members (GList          **members,
                                    GIIrNodeTypeId   container_type)
{
#if 0
  if (*members)
    {
      GList *l;

      for (l = *members; l; l = l->next)
        {
          GIIrNode *member = (GIIrNode *)l->data;
          g_printerr ("Unhandled '%s' member '%s' type '%s'\n",
                      gi_ir_node_type_to_string (container_type),
                      member->name,
                      gi_ir_node_type_to_string (member->type));
        }

      g_list_free (*members);
      *members = NULL;

      g_error ("Unhandled members. Aborting.");
    }
#else
  g_list_free (*members);
  *members = NULL;
#endif
}

void
gi_ir_node_build_typelib (GIIrNode         *node,
                          GIIrNode         *parent,
                          GIIrTypelibBuild *build,
                          uint32_t         *offset,
                          uint32_t         *offset2,
                          uint16_t         *count2)
{
  gboolean appended_stack;
  GHashTable *strings = build->strings;
  GHashTable *types = build->types;
  uint8_t *data = build->data;
  GList *l;
  uint32_t old_offset = *offset;
  uint32_t old_offset2 = *offset2;

  g_assert (node != NULL);

  g_debug ("build_typelib: %s%s(%s)",
           node->name ? node->name : "",
           node->name ? " " : "",
           gi_ir_node_type_to_string (node->type));

  if (build->stack)
    appended_stack = node != (GIIrNode*)build->stack->data;
  else
    appended_stack = TRUE;
  if (appended_stack)
    build->stack = g_list_prepend (build->stack, node);
  
  gi_ir_node_compute_offsets (build, node);

  /* We should only be building each node once.  If we do a typelib expansion, we also
   * reset the offset in girmodule.c.
   */
  g_assert (node->offset == 0);
  node->offset = *offset;
  build->nodes_with_attributes = g_list_prepend (build->nodes_with_attributes, node);

  build->n_attributes += g_hash_table_size (node->attributes);

  switch (node->type)
    {
    case GI_IR_NODE_TYPE:
      {
        GIIrNodeType *type = (GIIrNodeType *)node;
        SimpleTypeBlob *blob = (SimpleTypeBlob *)&data[*offset];

        *offset += sizeof (SimpleTypeBlob);

        if (GI_TYPE_TAG_IS_BASIC (type->tag))
          {
            blob->flags.reserved = 0;
            blob->flags.reserved2 = 0;
            blob->flags.pointer = type->is_pointer;
            blob->flags.reserved3 = 0;
            blob->flags.tag = type->tag;
          }
        else
          {
            GString *str;
            gpointer value;

            str = g_string_new (0);
            serialize_type (build, type, str);

            types_count += 1;
            value = g_hash_table_lookup (types, str->str);
            if (value)
              {
                blob->offset = GPOINTER_TO_UINT (value);
                g_string_free (g_steal_pointer (&str), TRUE);
              }
            else
              {
                unique_types_count += 1;
                g_hash_table_insert (types, g_string_free_and_steal (g_steal_pointer (&str)),
                                     GUINT_TO_POINTER(*offset2));

                blob->offset = *offset2;
                switch (type->tag)
                  {
                  case GI_TYPE_TAG_ARRAY:
                    {
                      ArrayTypeBlob *array = (ArrayTypeBlob *)&data[*offset2];
                      uint32_t pos;

                      array->pointer = type->is_pointer;
                      array->reserved = 0;
                      array->tag = type->tag;
                      array->zero_terminated = type->zero_terminated;
                      array->has_length = type->has_length;
                      array->has_size = type->has_size;
                      array->array_type = type->array_type;
                      array->reserved2 = 0;
                      if (array->has_length)
                        array->dimensions.length = type->length;
                      else if (array->has_size)
                        array->dimensions.size  = type->size;

                      pos = *offset2 + G_STRUCT_OFFSET (ArrayTypeBlob, type);
                      *offset2 += sizeof (ArrayTypeBlob);

                      gi_ir_node_build_typelib ((GIIrNode *)type->parameter_type1,
                                                node, build, &pos, offset2, NULL);
                    }
                    break;

                  case GI_TYPE_TAG_INTERFACE:
                    {
                      InterfaceTypeBlob *iface = (InterfaceTypeBlob *)&data[*offset2];
                      *offset2 += sizeof (InterfaceTypeBlob);

                      iface->pointer = type->is_pointer;
                      iface->reserved = 0;
                      iface->tag = type->tag;
                      iface->reserved2 = 0;
                      iface->interface = find_entry (build, type->giinterface);

                    }
                    break;

                  case GI_TYPE_TAG_GLIST:
                  case GI_TYPE_TAG_GSLIST:
                    {
                      ParamTypeBlob *param = (ParamTypeBlob *)&data[*offset2];
                      uint32_t pos;

                      param->pointer = 1;
                      param->reserved = 0;
                      param->tag = type->tag;
                      param->reserved2 = 0;
                      param->n_types = 1;

                      pos = *offset2 + G_STRUCT_OFFSET (ParamTypeBlob, type);
                      *offset2 += sizeof (ParamTypeBlob) + sizeof (SimpleTypeBlob);

                      gi_ir_node_build_typelib ((GIIrNode *)type->parameter_type1,
                                                node, build, &pos, offset2, NULL);
                    }
                    break;

                  case GI_TYPE_TAG_GHASH:
                    {
                      ParamTypeBlob *param = (ParamTypeBlob *)&data[*offset2];
                      uint32_t pos;

                      param->pointer = 1;
                      param->reserved = 0;
                      param->tag = type->tag;
                      param->reserved2 = 0;
                      param->n_types = 2;

                      pos = *offset2 + G_STRUCT_OFFSET (ParamTypeBlob, type);
                      *offset2 += sizeof (ParamTypeBlob) + sizeof (SimpleTypeBlob)*2;

                      gi_ir_node_build_typelib ((GIIrNode *)type->parameter_type1,
                                                node, build, &pos, offset2, NULL);
                      gi_ir_node_build_typelib ((GIIrNode *)type->parameter_type2,
                                                node, build, &pos, offset2, NULL);
                    }
                    break;

                  case GI_TYPE_TAG_ERROR:
                    {
                      ErrorTypeBlob *error_blob = (ErrorTypeBlob *)&data[*offset2];

                      error_blob->pointer = 1;
                      error_blob->reserved = 0;
                      error_blob->tag = type->tag;
                      error_blob->reserved2 = 0;
                      error_blob->n_domains = 0;

                      *offset2 += sizeof (ErrorTypeBlob);
                    }
                    break;

                  default:
                    g_error ("Unknown type tag %d", type->tag);
                    break;
                  }
              }
          }
      }
      break;

    case GI_IR_NODE_FIELD:
      {
        GIIrNodeField *field = (GIIrNodeField *)node;
        FieldBlob *blob;

        blob = (FieldBlob *)&data[*offset];

        blob->name = gi_ir_write_string (node->name, strings, data, offset2);
        blob->readable = field->readable;
        blob->writable = field->writable;
        blob->reserved = 0;
        blob->bits = 0;
        if (field->offset_state == GI_IR_OFFSETS_COMPUTED)
          blob->struct_offset = field->offset;
        else
          blob->struct_offset = 0xFFFF; /* mark as unknown */

        if (field->callback)
          {
            blob->has_embedded_type = TRUE;
            blob->type.offset = GI_INFO_TYPE_CALLBACK;
            *offset += sizeof (FieldBlob);
            gi_ir_node_build_typelib ((GIIrNode *)field->callback,
                                      node, build, offset, offset2, NULL);
            /* Fields with callbacks are bigger than normal, update count2
             * as an extra hint which represents the number of fields which are
             * callbacks. This allows us to gain constant time performance in the
             * repository for skipping over the fields section.
             */
            if (count2)
              (*count2)++;
          }
        else
          {
            blob->has_embedded_type = FALSE;
            /* We handle the size member specially below, so subtract it */
            *offset += sizeof (FieldBlob) - sizeof (SimpleTypeBlob);
            gi_ir_node_build_typelib ((GIIrNode *)field->type,
                                      node, build, offset, offset2, NULL);
          }
      }
      break;

    case GI_IR_NODE_PROPERTY:
      {
        GIIrNodeProperty *prop = (GIIrNodeProperty *)node;
        PropertyBlob *blob = (PropertyBlob *)&data[*offset];
        /* We handle the size member specially below, so subtract it */
        *offset += sizeof (PropertyBlob) - sizeof (SimpleTypeBlob);

        blob->name = gi_ir_write_string (node->name, strings, data, offset2);
        blob->deprecated = prop->deprecated;
        blob->readable = prop->readable;
        blob->writable = prop->writable;
        blob->construct = prop->construct;
        blob->construct_only = prop->construct_only;
        blob->transfer_ownership = prop->transfer;
        blob->transfer_container_ownership = prop->shallow_transfer;
        blob->reserved = 0;

        if (prop->setter != NULL)
          {
            int index = get_index_of_member_type ((GIIrNodeInterface*)parent,
                                                  GI_IR_NODE_FUNCTION,
                                                  prop->setter);
            if (index == -1)
              {
                g_error ("Unknown setter %s for property %s:%s", prop->setter, parent->name, node->name);
              }

            blob->setter = (uint16_t) index;
          }
        else
          blob->setter = ACCESSOR_SENTINEL;

        if (prop->getter != NULL)
          {
            int index = get_index_of_member_type ((GIIrNodeInterface*)parent,
                                                  GI_IR_NODE_FUNCTION,
                                                  prop->getter);
            if (index == -1)
              {
                g_error ("Unknown getter %s for property %s:%s", prop->getter, parent->name, node->name);
              }

            blob->getter = (uint16_t) index;
          }
        else
          blob->getter = ACCESSOR_SENTINEL;

        gi_ir_node_build_typelib ((GIIrNode *)prop->type,
                                  node, build, offset, offset2, NULL);
      }
      break;

    case GI_IR_NODE_FUNCTION:
      {
        FunctionBlob *blob = (FunctionBlob *)&data[*offset];
        SignatureBlob *blob2 = (SignatureBlob *)&data[*offset2];
        GIIrNodeFunction *function = (GIIrNodeFunction *)node;
        uint32_t signature;
        unsigned int n;

        signature = *offset2;
        n = g_list_length (function->parameters);

        *offset += sizeof (FunctionBlob);
        *offset2 += sizeof (SignatureBlob) + n * sizeof (ArgBlob);

        blob->blob_type = BLOB_TYPE_FUNCTION;
        blob->deprecated = function->deprecated;
        blob->is_static = !function->is_method;
        blob->setter = FALSE;
        blob->getter = FALSE;
        blob->constructor = function->is_constructor;
        blob->wraps_vfunc = function->wraps_vfunc;
        blob->throws = function->throws; /* Deprecated. Also stored in SignatureBlob. */
        blob->index = 0;
        blob->name = gi_ir_write_string (node->name, strings, data, offset2);
        blob->symbol = gi_ir_write_string (function->symbol, strings, data, offset2);
        blob->signature = signature;
        blob->finish = ASYNC_SENTINEL;
        blob->sync_or_async = ASYNC_SENTINEL;
        blob->is_async = function->is_async;

        if (function->is_async)
          {
            if (function->sync_func != NULL)
              {
                int sync_index =
                    get_index_for_function (build,
                                            parent,
                                            function->sync_func);

                if (sync_index == -1)
                  {
                    g_error ("Unknown sync function %s:%s",
                             parent->name, function->sync_func);
                  }

                blob->sync_or_async = (uint16_t) sync_index;
              }

            if (function->finish_func != NULL)
              {
                int finish_index =
                    get_index_for_function (build,
                                            parent,
                                            function->finish_func);

                if (finish_index == -1)
                  {
                    g_error ("Unknown finish function %s:%s", parent->name, function->finish_func);
                  }

                blob->finish = (uint16_t) finish_index;
              }
          }
        else
          {
            if (function->async_func != NULL)
              {
                int async_index =
                    get_index_for_function (build,
                                            parent,
                                            function->async_func);

                if (async_index == -1)
                  {
                    g_error ("Unknown async function %s:%s", parent->name, function->async_func);
                  }

                blob->sync_or_async = (uint16_t) async_index;
              }
          }


        if (function->is_setter || function->is_getter)
          {
            int index = get_index_of_member_type ((GIIrNodeInterface*)parent,
                                                  GI_IR_NODE_PROPERTY,
                                                  function->property);
            if (index == -1)
              {
                g_error ("Unknown property %s:%s for accessor %s", parent->name, function->property, function->symbol);
              }

            blob->setter = function->is_setter;
            blob->getter = function->is_getter;
            blob->index = (uint16_t) index;
          }

        /* function->result is special since it doesn't appear in the serialized format but
         * we do want the attributes for it to appear
         */
        build->nodes_with_attributes = g_list_prepend (build->nodes_with_attributes, function->result);
        build->n_attributes += g_hash_table_size (((GIIrNode *) function->result)->attributes);
        g_assert (((GIIrNode *) function->result)->offset == 0);
        ((GIIrNode *) function->result)->offset = signature;

        g_debug ("building function '%s'", function->symbol);

        gi_ir_node_build_typelib ((GIIrNode *)function->result->type,
                                  node, build, &signature, offset2, NULL);

        blob2->may_return_null = function->result->nullable;
        blob2->caller_owns_return_value = function->result->transfer;
        blob2->caller_owns_return_container = function->result->shallow_transfer;
        blob2->skip_return = function->result->skip;
        blob2->instance_transfer_ownership = function->instance_transfer_full;
        blob2->reserved = 0;
        blob2->n_arguments = n;
        blob2->throws = function->throws;

        signature += 4;

        for (l = function->parameters; l; l = l->next)
          {
            GIIrNode *param = (GIIrNode *)l->data;

            gi_ir_node_build_typelib (param, node, build, &signature, offset2, NULL);
          }

      }
      break;

    case GI_IR_NODE_CALLBACK:
      {
        CallbackBlob *blob = (CallbackBlob *)&data[*offset];
        SignatureBlob *blob2 = (SignatureBlob *)&data[*offset2];
        GIIrNodeFunction *function = (GIIrNodeFunction *)node;
        uint32_t signature;
        unsigned int n;

        signature = *offset2;
        n = g_list_length (function->parameters);

        *offset += sizeof (CallbackBlob);
        *offset2 += sizeof (SignatureBlob) + n * sizeof (ArgBlob);

        blob->blob_type = BLOB_TYPE_CALLBACK;
        blob->deprecated = function->deprecated;
        blob->reserved = 0;
        blob->name = gi_ir_write_string (node->name, strings, data, offset2);
        blob->signature = signature;

        gi_ir_node_build_typelib ((GIIrNode *)function->result->type,
                                  node, build, &signature, offset2, NULL);

        blob2->may_return_null = function->result->nullable;
        blob2->caller_owns_return_value = function->result->transfer;
        blob2->caller_owns_return_container = function->result->shallow_transfer;
        blob2->reserved = 0;
        blob2->n_arguments = n;
        blob2->throws = function->throws;

        signature += 4;

        for (l = function->parameters; l; l = l->next)
          {
            GIIrNode *param = (GIIrNode *)l->data;

            gi_ir_node_build_typelib (param, node, build, &signature, offset2, NULL);
          }
      }
      break;

    case GI_IR_NODE_SIGNAL:
      {
        SignalBlob *blob = (SignalBlob *)&data[*offset];
        SignatureBlob *blob2 = (SignatureBlob *)&data[*offset2];
        GIIrNodeSignal *signal = (GIIrNodeSignal *)node;
        uint32_t signature;
        unsigned int n;

        signature = *offset2;
        n = g_list_length (signal->parameters);

        *offset += sizeof (SignalBlob);
        *offset2 += sizeof (SignatureBlob) + n * sizeof (ArgBlob);

        blob->deprecated = signal->deprecated;
        blob->run_first = signal->run_first;
        blob->run_last = signal->run_last;
        blob->run_cleanup = signal->run_cleanup;
        blob->no_recurse = signal->no_recurse;
        blob->detailed = signal->detailed;
        blob->action = signal->action;
        blob->no_hooks = signal->no_hooks;
        blob->has_class_closure = 0; /* FIXME */
        blob->true_stops_emit = 0; /* FIXME */
        blob->reserved = 0;
        blob->class_closure = 0; /* FIXME */
        blob->name = gi_ir_write_string (node->name, strings, data, offset2);
        blob->signature = signature;

        /* signal->result is special since it doesn't appear in the serialized format but
         * we do want the attributes for it to appear
         */
        build->nodes_with_attributes = g_list_prepend (build->nodes_with_attributes, signal->result);
        build->n_attributes += g_hash_table_size (((GIIrNode *) signal->result)->attributes);
        g_assert (((GIIrNode *) signal->result)->offset == 0);
        ((GIIrNode *) signal->result)->offset = signature;

        gi_ir_node_build_typelib ((GIIrNode *)signal->result->type,
                                  node, build, &signature, offset2, NULL);

        blob2->may_return_null = signal->result->nullable;
        blob2->caller_owns_return_value = signal->result->transfer;
        blob2->caller_owns_return_container = signal->result->shallow_transfer;
        blob2->instance_transfer_ownership = signal->instance_transfer_full;
        blob2->reserved = 0;
        blob2->n_arguments = n;

        signature += 4;

        for (l = signal->parameters; l; l = l->next)
          {
            GIIrNode *param = (GIIrNode *)l->data;

            gi_ir_node_build_typelib (param, node, build, &signature, offset2, NULL);
          }
      }
      break;

    case GI_IR_NODE_VFUNC:
      {
        VFuncBlob *blob = (VFuncBlob *)&data[*offset];
        SignatureBlob *blob2 = (SignatureBlob *)&data[*offset2];
        GIIrNodeVFunc *vfunc = (GIIrNodeVFunc *)node;
        uint32_t signature;
        unsigned int n;

        signature = *offset2;
        n = g_list_length (vfunc->parameters);

        *offset += sizeof (VFuncBlob);
        *offset2 += sizeof (SignatureBlob) + n * sizeof (ArgBlob);

        blob->name = gi_ir_write_string (node->name, strings, data, offset2);
        blob->must_chain_up = 0; /* FIXME */
        blob->must_be_implemented = 0; /* FIXME */
        blob->must_not_be_implemented = 0; /* FIXME */
        blob->class_closure = 0; /* FIXME */
        blob->throws = vfunc->throws; /* Deprecated. Also stored in SignatureBlob. */
        blob->reserved = 0;
        blob->is_async = vfunc->is_async;
        blob->finish = ASYNC_SENTINEL;
        blob->sync_or_async = ASYNC_SENTINEL;

        if (vfunc->invoker)
          {
            int index = get_index_of_member_type ((GIIrNodeInterface*)parent, GI_IR_NODE_FUNCTION, vfunc->invoker);
            if (index == -1)
              {
                g_error ("Unknown member function %s for vfunc %s", vfunc->invoker, node->name);
              }
            blob->invoker = (uint16_t) index;
          }
        else
          blob->invoker = 0x3ff; /* max of 10 bits */

        if (vfunc->is_async)
          {
            if (vfunc->sync_func != NULL)
              {
                int sync_index =
                    get_index_of_member_type ((GIIrNodeInterface *) parent,
                                              GI_IR_NODE_VFUNC,
                                              vfunc->sync_func);

                if (sync_index == -1)
                  {
                    g_error ("Unknown sync vfunc %s:%s for accessor %s",
                             parent->name, vfunc->sync_func, vfunc->invoker);
                  }

                blob->sync_or_async = (uint16_t) sync_index;
              }

            if (vfunc->finish_func != NULL)
              {
                int finish_index =
                    get_index_of_member_type ((GIIrNodeInterface *) parent,
                                              GI_IR_NODE_VFUNC,
                                              vfunc->finish_func);

                if (finish_index == -1)
                  {
                    g_error ("Unknown finish vfunc %s:%s for function %s",
                             parent->name, vfunc->finish_func, vfunc->invoker);
                  }

                blob->finish = (uint16_t) finish_index;
              }
          }
        else
          {
            if (vfunc->async_func != NULL)
              {
                int async_index =
                    get_index_of_member_type ((GIIrNodeInterface *) parent,
                                              GI_IR_NODE_VFUNC,
                                              vfunc->async_func);
                if (async_index == -1)
                  {
                    g_error ("Unknown async vfunc %s:%s for accessor %s",
                             parent->name, vfunc->async_func, vfunc->invoker);
                  }

                blob->sync_or_async = (uint16_t) async_index;
              }
          }

        blob->struct_offset = vfunc->offset;
        blob->reserved2 = 0;
        blob->signature = signature;
        blob->is_static = vfunc->is_static;

        gi_ir_node_build_typelib ((GIIrNode *)vfunc->result->type,
                                  node, build, &signature, offset2, NULL);

        blob2->may_return_null = vfunc->result->nullable;
        blob2->caller_owns_return_value = vfunc->result->transfer;
        blob2->caller_owns_return_container = vfunc->result->shallow_transfer;
        blob2->instance_transfer_ownership = vfunc->instance_transfer_full;
        blob2->reserved = 0;
        blob2->n_arguments = n;
        blob2->throws = vfunc->throws;

        signature += 4;

        for (l = vfunc->parameters; l; l = l->next)
          {
            GIIrNode *param = (GIIrNode *)l->data;

            gi_ir_node_build_typelib (param, node, build, &signature, offset2, NULL);
          }
      }
      break;

    case GI_IR_NODE_PARAM:
      {
        ArgBlob *blob = (ArgBlob *)&data[*offset];
        GIIrNodeParam *param = (GIIrNodeParam *)node;

        /* The offset for this one is smaller than the struct because
         * we recursively build the simple type inline here below.
         */
        *offset += sizeof (ArgBlob) - sizeof (SimpleTypeBlob);

        blob->name = gi_ir_write_string (node->name, strings, data, offset2);
        blob->in = param->in;
        blob->out = param->out;
        blob->caller_allocates = param->caller_allocates;
        blob->nullable = param->nullable;
        blob->skip = param->skip;
        blob->optional = param->optional;
        blob->transfer_ownership = param->transfer;
        blob->transfer_container_ownership = param->shallow_transfer;
        blob->return_value = param->retval;
        blob->scope = param->scope;
        blob->reserved = 0;
        blob->closure = param->closure;
        blob->destroy = param->destroy;

        gi_ir_node_build_typelib ((GIIrNode *)param->type, node, build, offset, offset2, NULL);
      }
      break;

    case GI_IR_NODE_STRUCT:
      {
        StructBlob *blob = (StructBlob *)&data[*offset];
        GIIrNodeStruct *struct_ = (GIIrNodeStruct *)node;
        GList *members;

        blob->blob_type = BLOB_TYPE_STRUCT;
        blob->foreign = struct_->foreign;
        blob->deprecated = struct_->deprecated;
        blob->is_gtype_struct = struct_->is_gtype_struct;
        blob->reserved = 0;
        blob->name = gi_ir_write_string (node->name, strings, data, offset2);
        blob->alignment = struct_->alignment;
        blob->size = struct_->size;

        if (struct_->gtype_name)
          {
            blob->unregistered = FALSE;
            blob->gtype_name = gi_ir_write_string (struct_->gtype_name, strings, data, offset2);
            blob->gtype_init = gi_ir_write_string (struct_->gtype_init, strings, data, offset2);
          }
        else
          {
            blob->unregistered = TRUE;
            blob->gtype_name = 0;
            blob->gtype_init = 0;
          }

        if (struct_->copy_func)
          blob->copy_func = gi_ir_write_string (struct_->copy_func, strings, data, offset2);
        if (struct_->free_func)
          blob->free_func = gi_ir_write_string (struct_->free_func, strings, data, offset2);

        blob->n_fields = 0;
        blob->n_methods = 0;

        *offset += sizeof (StructBlob);

        members = g_list_copy (struct_->members);

        gi_ir_node_build_members (&members, GI_IR_NODE_FIELD, &blob->n_fields,
                                  node, build, offset, offset2, NULL);

        gi_ir_node_build_members (&members, GI_IR_NODE_FUNCTION, &blob->n_methods,
                                  node, build, offset, offset2, NULL);

        gi_ir_node_check_unhandled_members (&members, node->type);

        g_assert (members == NULL);
      }
      break;

    case GI_IR_NODE_BOXED:
      {
        StructBlob *blob = (StructBlob *)&data[*offset];
        GIIrNodeBoxed *boxed = (GIIrNodeBoxed *)node;
        GList *members;

        blob->blob_type = BLOB_TYPE_BOXED;
        blob->deprecated = boxed->deprecated;
        blob->unregistered = FALSE;
        blob->reserved = 0;
        blob->name = gi_ir_write_string (node->name, strings, data, offset2);
        blob->gtype_name = gi_ir_write_string (boxed->gtype_name, strings, data, offset2);
        blob->gtype_init = gi_ir_write_string (boxed->gtype_init, strings, data, offset2);
        blob->alignment = boxed->alignment;
        blob->size = boxed->size;

        blob->n_fields = 0;
        blob->n_methods = 0;

        *offset += sizeof (StructBlob);

        members = g_list_copy (boxed->members);

        gi_ir_node_build_members (&members, GI_IR_NODE_FIELD, &blob->n_fields,
                                  node, build, offset, offset2, NULL);

        gi_ir_node_build_members (&members, GI_IR_NODE_FUNCTION, &blob->n_methods,
                                  node, build, offset, offset2, NULL);

        gi_ir_node_check_unhandled_members (&members, node->type);

        g_assert (members == NULL);
      }
      break;

    case GI_IR_NODE_UNION:
      {
        UnionBlob *blob = (UnionBlob *)&data[*offset];
        GIIrNodeUnion *union_ = (GIIrNodeUnion *)node;
        GList *members;

        blob->blob_type = BLOB_TYPE_UNION;
        blob->deprecated = union_->deprecated;
        blob->reserved = 0;
        blob->name = gi_ir_write_string (node->name, strings, data, offset2);
        blob->alignment = union_->alignment;
        blob->size = union_->size;
        if (union_->gtype_name)
          {
            blob->unregistered = FALSE;
            blob->gtype_name = gi_ir_write_string (union_->gtype_name, strings, data, offset2);
            blob->gtype_init = gi_ir_write_string (union_->gtype_init, strings, data, offset2);
          }
        else
          {
            blob->unregistered = TRUE;
            blob->gtype_name = 0;
            blob->gtype_init = 0;
          }

        blob->n_fields = 0;
        blob->n_functions = 0;

        g_assert (union_->discriminator_offset <= G_MAXINT32);
        blob->discriminator_offset = (int32_t) union_->discriminator_offset;

        if (union_->copy_func)
          blob->copy_func = gi_ir_write_string (union_->copy_func, strings, data, offset2);
        if (union_->free_func)
          blob->free_func = gi_ir_write_string (union_->free_func, strings, data, offset2);

        /* We don't support Union discriminators right now. */
        /*
        if (union_->discriminator_type)
          {
            *offset += 28;
            blob->discriminated = TRUE;
            gi_ir_node_build_typelib ((GIIrNode *)union_->discriminator_type,
                                      build, offset, offset2, NULL);
          }
        else
          {
        */
        *offset += sizeof (UnionBlob);
        blob->discriminated = FALSE;
        blob->discriminator_type.offset = 0;

        members = g_list_copy (union_->members);

        gi_ir_node_build_members (&members, GI_IR_NODE_FIELD, &blob->n_fields,
                                  node, build, offset, offset2, NULL);

        gi_ir_node_build_members (&members, GI_IR_NODE_FUNCTION, &blob->n_functions,
                                  node, build, offset, offset2, NULL);

        gi_ir_node_check_unhandled_members (&members, node->type);

        g_assert (members == NULL);

        if (union_->discriminator_type)
          {
            for (l = union_->discriminators; l; l = l->next)
              {
                GIIrNode *member = (GIIrNode *)l->data;

                gi_ir_node_build_typelib (member, node, build, offset, offset2, NULL);
              }
          }
      }
      break;

    case GI_IR_NODE_ENUM:
    case GI_IR_NODE_FLAGS:
      {
        EnumBlob *blob = (EnumBlob *)&data[*offset];
        GIIrNodeEnum *enum_ = (GIIrNodeEnum *)node;

        *offset += sizeof (EnumBlob);

        if (node->type == GI_IR_NODE_ENUM)
          blob->blob_type = BLOB_TYPE_ENUM;
        else
          blob->blob_type = BLOB_TYPE_FLAGS;

        blob->deprecated = enum_->deprecated;
        blob->reserved = 0;
        blob->storage_type = enum_->storage_type;
        blob->name = gi_ir_write_string (node->name, strings, data, offset2);
        if (enum_->gtype_name)
          {
            blob->unregistered = FALSE;
            blob->gtype_name = gi_ir_write_string (enum_->gtype_name, strings, data, offset2);
            blob->gtype_init = gi_ir_write_string (enum_->gtype_init, strings, data, offset2);
          }
        else
          {
            blob->unregistered = TRUE;
            blob->gtype_name = 0;
            blob->gtype_init = 0;
          }
        if (enum_->error_domain)
          blob->error_domain = gi_ir_write_string (enum_->error_domain, strings, data, offset2);
        else
          blob->error_domain = 0;

        blob->n_values = 0;
        blob->n_methods = 0;

        for (l = enum_->values; l; l = l->next)
          {
            GIIrNode *value = (GIIrNode *)l->data;

            blob->n_values++;
            gi_ir_node_build_typelib (value, node, build, offset, offset2, NULL);
          }

        for (l = enum_->methods; l; l = l->next)
          {
            GIIrNode *method = (GIIrNode *)l->data;

            blob->n_methods++;
            gi_ir_node_build_typelib (method, node, build, offset, offset2, NULL);
          }
      }
      break;

    case GI_IR_NODE_OBJECT:
      {
        ObjectBlob *blob = (ObjectBlob *)&data[*offset];
        GIIrNodeInterface *object = (GIIrNodeInterface *)node;
        GList *members;

        blob->blob_type = BLOB_TYPE_OBJECT;
        blob->abstract = object->abstract;
        blob->fundamental = object->fundamental;
        blob->final_ = object->final_;
        blob->deprecated = object->deprecated;
        blob->reserved = 0;
        blob->name = gi_ir_write_string (node->name, strings, data, offset2);
        blob->gtype_name = gi_ir_write_string (object->gtype_name, strings, data, offset2);
        blob->gtype_init = gi_ir_write_string (object->gtype_init, strings, data, offset2);
        if (object->ref_func)
          blob->ref_func = gi_ir_write_string (object->ref_func, strings, data, offset2);
        if (object->unref_func)
          blob->unref_func = gi_ir_write_string (object->unref_func, strings, data, offset2);
        if (object->set_value_func)
          blob->set_value_func = gi_ir_write_string (object->set_value_func, strings, data, offset2);
        if (object->get_value_func)
          blob->get_value_func = gi_ir_write_string (object->get_value_func, strings, data, offset2);
        if (object->parent)
          blob->parent = find_entry (build, object->parent);
        else
          blob->parent = 0;
        if (object->glib_type_struct)
          blob->gtype_struct = find_entry (build, object->glib_type_struct);
        else
          blob->gtype_struct = 0;

        blob->n_interfaces = 0;
        blob->n_fields = 0;
        blob->n_properties = 0;
        blob->n_methods = 0;
        blob->n_signals = 0;
        blob->n_vfuncs = 0;
        blob->n_constants = 0;
        blob->n_field_callbacks = 0;

        *offset += sizeof(ObjectBlob);
        for (l = object->interfaces; l; l = l->next)
          {
            blob->n_interfaces++;
            *(uint16_t *)&data[*offset] = find_entry (build, (char *)l->data);
            *offset += 2;
          }

        members = g_list_copy (object->members);

        *offset = ALIGN_VALUE (*offset, 4);
        gi_ir_node_build_members (&members, GI_IR_NODE_FIELD, &blob->n_fields,
                                  node, build, offset, offset2, &blob->n_field_callbacks);

        *offset = ALIGN_VALUE (*offset, 4);
        gi_ir_node_build_members (&members, GI_IR_NODE_PROPERTY, &blob->n_properties,
                                  node, build, offset, offset2, NULL);

        *offset = ALIGN_VALUE (*offset, 4);
        gi_ir_node_build_members (&members, GI_IR_NODE_FUNCTION, &blob->n_methods,
                                  node, build, offset, offset2, NULL);

        *offset = ALIGN_VALUE (*offset, 4);
        gi_ir_node_build_members (&members, GI_IR_NODE_SIGNAL, &blob->n_signals,
                                  node, build, offset, offset2, NULL);

        *offset = ALIGN_VALUE (*offset, 4);
        gi_ir_node_build_members (&members, GI_IR_NODE_VFUNC, &blob->n_vfuncs,
                                  node, build, offset, offset2, NULL);

        *offset = ALIGN_VALUE (*offset, 4);
        gi_ir_node_build_members (&members, GI_IR_NODE_CONSTANT, &blob->n_constants,
                                  node, build, offset, offset2, NULL);

        gi_ir_node_check_unhandled_members (&members, node->type);

        g_assert (members == NULL);
      }
      break;

    case GI_IR_NODE_INTERFACE:
      {
        InterfaceBlob *blob = (InterfaceBlob *)&data[*offset];
        GIIrNodeInterface *iface = (GIIrNodeInterface *)node;
        GList *members;

        blob->blob_type = BLOB_TYPE_INTERFACE;
        blob->deprecated = iface->deprecated;
        blob->reserved = 0;
        blob->name = gi_ir_write_string (node->name, strings, data, offset2);
        blob->gtype_name = gi_ir_write_string (iface->gtype_name, strings, data, offset2);
        blob->gtype_init = gi_ir_write_string (iface->gtype_init, strings, data, offset2);
        if (iface->glib_type_struct)
          blob->gtype_struct = find_entry (build, iface->glib_type_struct);
        else
          blob->gtype_struct = 0;
        blob->n_prerequisites = 0;
        blob->n_properties = 0;
        blob->n_methods = 0;
        blob->n_signals = 0;
        blob->n_vfuncs = 0;
        blob->n_constants = 0;

        *offset += sizeof (InterfaceBlob);
        for (l = iface->prerequisites; l; l = l->next)
          {
            blob->n_prerequisites++;
            *(uint16_t *)&data[*offset] = find_entry (build, (char *)l->data);
            *offset += 2;
          }

        members = g_list_copy (iface->members);

        *offset = ALIGN_VALUE (*offset, 4);
        gi_ir_node_build_members (&members, GI_IR_NODE_PROPERTY, &blob->n_properties,
                                  node, build, offset, offset2, NULL);

        *offset = ALIGN_VALUE (*offset, 4);
        gi_ir_node_build_members (&members, GI_IR_NODE_FUNCTION, &blob->n_methods,
                                  node, build, offset, offset2, NULL);

        *offset = ALIGN_VALUE (*offset, 4);
        gi_ir_node_build_members (&members, GI_IR_NODE_SIGNAL, &blob->n_signals,
                                  node, build, offset, offset2, NULL);

        *offset = ALIGN_VALUE (*offset, 4);
        gi_ir_node_build_members (&members, GI_IR_NODE_VFUNC, &blob->n_vfuncs,
                                  node, build, offset, offset2, NULL);

        *offset = ALIGN_VALUE (*offset, 4);
        gi_ir_node_build_members (&members, GI_IR_NODE_CONSTANT, &blob->n_constants,
                                  node, build, offset, offset2, NULL);

        gi_ir_node_check_unhandled_members (&members, node->type);

        g_assert (members == NULL);
      }
      break;


    case GI_IR_NODE_VALUE:
      {
        GIIrNodeValue *value = (GIIrNodeValue *)node;
        ValueBlob *blob = (ValueBlob *)&data[*offset];
        *offset += sizeof (ValueBlob);

        blob->deprecated = value->deprecated;
        blob->reserved = 0;
        blob->unsigned_value = value->value >= 0 ? 1 : 0;
        blob->name = gi_ir_write_string (node->name, strings, data, offset2);
        blob->value = (int32_t) value->value;
      }
      break;

    case GI_IR_NODE_CONSTANT:
      {
        GIIrNodeConstant *constant = (GIIrNodeConstant *)node;
        ConstantBlob *blob = (ConstantBlob *)&data[*offset];
        uint32_t pos;

        pos = *offset + G_STRUCT_OFFSET (ConstantBlob, type);
        *offset += sizeof (ConstantBlob);

        blob->blob_type = BLOB_TYPE_CONSTANT;
        blob->deprecated = constant->deprecated;
        blob->reserved = 0;
        blob->name = gi_ir_write_string (node->name, strings, data, offset2);

        blob->offset = *offset2;
        switch (constant->type->tag)
          {
          case GI_TYPE_TAG_BOOLEAN:
            blob->size = 4;
            *(gboolean*)&data[blob->offset] = parse_boolean_value (constant->value);
            break;
            case GI_TYPE_TAG_INT8:
            blob->size = 1;
            *(int8_t *)&data[blob->offset] = (int8_t) parse_int_value (constant->value);
            break;
          case GI_TYPE_TAG_UINT8:
            blob->size = 1;
            *(uint8_t *)&data[blob->offset] = (uint8_t) parse_uint_value (constant->value);
            break;
          case GI_TYPE_TAG_INT16:
            blob->size = 2;
            *(int16_t *)&data[blob->offset] = (int16_t) parse_int_value (constant->value);
            break;
          case GI_TYPE_TAG_UINT16:
            blob->size = 2;
            *(uint16_t *)&data[blob->offset] = (uint16_t) parse_uint_value (constant->value);
            break;
          case GI_TYPE_TAG_INT32:
            blob->size = 4;
            *(int32_t *)&data[blob->offset] = (int32_t) parse_int_value (constant->value);
            break;
          case GI_TYPE_TAG_UINT32:
            blob->size = 4;
            *(uint32_t*)&data[blob->offset] = (uint32_t) parse_uint_value (constant->value);
            break;
          case GI_TYPE_TAG_INT64:
            blob->size = 8;
            DO_ALIGNED_COPY (&data[blob->offset], parse_int_value (constant->value), int64_t);
            break;
          case GI_TYPE_TAG_UINT64:
            blob->size = 8;
            DO_ALIGNED_COPY (&data[blob->offset], parse_uint_value (constant->value), uint64_t);
            break;
          case GI_TYPE_TAG_FLOAT:
            blob->size = sizeof (float);
            DO_ALIGNED_COPY (&data[blob->offset], (float) parse_float_value (constant->value), float);
            break;
          case GI_TYPE_TAG_DOUBLE:
            blob->size = sizeof (double);
            DO_ALIGNED_COPY (&data[blob->offset], parse_float_value (constant->value), double);
            break;
          case GI_TYPE_TAG_UTF8:
          case GI_TYPE_TAG_FILENAME:
            blob->size = strlen (constant->value) + 1;
            memcpy (&data[blob->offset], constant->value, blob->size);
            break;
          default:
            break;
          }
        *offset2 += ALIGN_VALUE (blob->size, 4);

        gi_ir_node_build_typelib ((GIIrNode *)constant->type, node, build, &pos, offset2, NULL);
      }
      break;
    default:
      g_assert_not_reached ();
    }

  g_debug ("node %s%s%s%p type '%s', offset %d -> %d, offset2 %d -> %d",
           node->name ? "'" : "",
           node->name ? node->name : "",
           node->name ? "' " : "",
           node, gi_ir_node_type_to_string (node->type),
           old_offset, *offset, old_offset2, *offset2);

  if (*offset2 - old_offset2 + *offset - old_offset > gi_ir_node_get_full_size (node))
    g_error ("exceeding space reservation; offset: %d (prev %d) offset2: %d (prev %d) nodesize: %d",
             *offset, old_offset, *offset2, old_offset2, gi_ir_node_get_full_size (node));

  if (appended_stack)
    build->stack = g_list_delete_link (build->stack, build->stack);
}

/* if str is already in the pool, return previous location, otherwise write str
 * to the typelib at offset, put it in the pool and update offset. If the
 * typelib is not large enough to hold the string, reallocate it.
 */
uint32_t
gi_ir_write_string (const char  *str,
                    GHashTable  *strings,
                    uint8_t     *data,
                    uint32_t    *offset)
{
  uint32_t start;
  void *value;

  string_count += 1;
  string_size += strlen (str);

  value = g_hash_table_lookup (strings, str);

  if (value)
    return GPOINTER_TO_UINT (value);

  unique_string_count += 1;
  unique_string_size += strlen (str);

  g_hash_table_insert (strings, (void *)str, GUINT_TO_POINTER (*offset));

  start = *offset;
  *offset = ALIGN_VALUE (start + strlen (str) + 1, 4);

  strcpy ((char *)&data[start], str);

  return start;
}

