/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Base struct implementation
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

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib-object.h>
#include <gobject/gvaluecollector.h>

#include "gitypelib-internal.h"
#include "girepository-private.h"
#include "gibaseinfo.h"
#include "gibaseinfo-private.h"

#define INVALID_REFCOUNT 0x7FFFFFFF

/* Type registration of BaseInfo. */
#define GI_BASE_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GI_TYPE_BASE_INFO, GIBaseInfoClass))

static void
value_base_info_init (GValue *value)
{
  value->data[0].v_pointer = NULL;
}

static void
value_base_info_free_value (GValue *value)
{
  if (value->data[0].v_pointer != NULL)
    gi_base_info_unref (value->data[0].v_pointer);
}

static void
value_base_info_copy_value (const GValue *src,
                            GValue       *dst)
{
  if (src->data[0].v_pointer != NULL)
    dst->data[0].v_pointer = gi_base_info_ref (src->data[0].v_pointer);
  else
    dst->data[0].v_pointer = NULL;
}

static void *
value_base_info_peek_pointer (const GValue *value)
{
  return value->data[0].v_pointer;
}

static char *
value_base_info_collect_value (GValue      *value,
                               guint        n_collect_values,
                               GTypeCValue *collect_values,
                               guint        collect_flags)
{
  GIBaseInfo *info = collect_values[0].v_pointer;

  if (info == NULL)
    {
      value->data[0].v_pointer = NULL;
      return NULL;
    }

  if (info->parent_instance.g_class == NULL)
    return g_strconcat ("invalid unclassed GIBaseInfo pointer for "
                        "value type '",
                        G_VALUE_TYPE_NAME (value),
                        "'",
                        NULL);

  value->data[0].v_pointer = gi_base_info_ref (info);

  return NULL;
}

static char *
value_base_info_lcopy_value (const GValue *value,
                             guint         n_collect_values,
                             GTypeCValue  *collect_values,
                             guint         collect_flags)
{
  GIBaseInfo **node_p = collect_values[0].v_pointer;

  if (node_p == NULL)
    return g_strconcat ("value location for '",
                        G_VALUE_TYPE_NAME (value),
                        "' passed as NULL",
                        NULL);

  if (value->data[0].v_pointer == NULL)
    *node_p = NULL;
  else if (collect_flags & G_VALUE_NOCOPY_CONTENTS)
    *node_p = value->data[0].v_pointer;
  else
    *node_p = gi_base_info_ref (value->data[0].v_pointer);

  return NULL;
}

static void
gi_base_info_finalize (GIBaseInfo *self)
{
  if (self->ref_count != INVALID_REFCOUNT &&
      self->container && self->container->ref_count != INVALID_REFCOUNT)
    gi_base_info_unref (self->container);
}

static void
gi_base_info_class_init (GIBaseInfoClass *klass)
{
  klass->info_type = GI_INFO_TYPE_INVALID;
  klass->finalize = gi_base_info_finalize;
}

static void
gi_base_info_init (GIBaseInfo *self)
{
  /* Initialise a dynamically allocated #GIBaseInfo’s members.
   *
   * This function *must* be kept in sync with gi_info_init(). */
  g_atomic_ref_count_init (&self->ref_count);
}

GType
gi_base_info_get_type (void)
{
  static GType base_info_type = 0;

  if (g_once_init_enter_pointer (&base_info_type))
    {
      static const GTypeFundamentalInfo finfo = {
        (G_TYPE_FLAG_CLASSED |
         G_TYPE_FLAG_INSTANTIATABLE |
         G_TYPE_FLAG_DERIVABLE |
         G_TYPE_FLAG_DEEP_DERIVABLE),
      };

      static const GTypeValueTable value_table = {
        value_base_info_init,
        value_base_info_free_value,
        value_base_info_copy_value,
        value_base_info_peek_pointer,
        "p",
        value_base_info_collect_value,
        "p",
        value_base_info_lcopy_value,
      };

      const GTypeInfo type_info = {
        /* Class */
        sizeof (GIBaseInfoClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gi_base_info_class_init,
        (GClassFinalizeFunc) NULL,
        NULL,

        /* Instance */
        sizeof (GIBaseInfo),
        0,
        (GInstanceInitFunc) gi_base_info_init,

        /* GValue */
        &value_table,
      };

      GType _base_info_type =
        g_type_register_fundamental (g_type_fundamental_next (),
                                     g_intern_static_string ("GIBaseInfo"),
                                     &type_info, &finfo,
                                     G_TYPE_FLAG_ABSTRACT);

      g_once_init_leave_pointer (&base_info_type, _base_info_type);
    }

  return base_info_type;
}

/*< private >
 * gi_base_info_type_register_static:
 * @type_name: the name of the type
 * @instance_size: size (in bytes) of the type’s instance struct
 * @class_init: class init function for the type
 * @parent_type: [type@GObject.Type] for the parent type; this will typically be
 *   `GI_TYPE_BASE_INFO`
 * @type_flags: flags for the type
 *
 * Registers a new [type@GIRepository.BaseInfo] type for the given @type_name
 * using the type information provided.
 *
 * Returns: the newly registered [type@GObject.Type]
 * Since: 2.80
 */
GType
gi_base_info_type_register_static (const char     *type_name,
                                   size_t          instance_size,
                                   GClassInitFunc  class_init,
                                   GType           parent_type,
                                   GTypeFlags      type_flags)
{
  GTypeInfo info;

  g_assert (instance_size <= G_MAXUINT16);

  info.class_size = sizeof (GIBaseInfoClass);
  info.base_init = NULL;
  info.base_finalize = NULL;
  info.class_init = class_init;
  info.class_finalize = NULL;
  info.class_data = NULL;
  info.instance_size = (guint16) instance_size;
  info.n_preallocs = 0;
  info.instance_init = NULL;
  info.value_table = NULL;

  return g_type_register_static (parent_type, type_name, &info, type_flags);
}

static GType gi_base_info_types[GI_INFO_TYPE_N_TYPES];

#define GI_DEFINE_BASE_INFO_TYPE(type_name, TYPE_ENUM_VALUE) \
GType \
type_name ## _get_type (void) \
{ \
  gi_base_info_init_types (); \
  g_assert (gi_base_info_types[TYPE_ENUM_VALUE] != G_TYPE_INVALID); \
  return gi_base_info_types[TYPE_ENUM_VALUE]; \
}

GI_DEFINE_BASE_INFO_TYPE (gi_callable_info, GI_INFO_TYPE_CALLABLE)
GI_DEFINE_BASE_INFO_TYPE (gi_function_info, GI_INFO_TYPE_FUNCTION)
GI_DEFINE_BASE_INFO_TYPE (gi_callback_info, GI_INFO_TYPE_CALLBACK)
GI_DEFINE_BASE_INFO_TYPE (gi_registered_type_info, GI_INFO_TYPE_REGISTERED_TYPE)
GI_DEFINE_BASE_INFO_TYPE (gi_struct_info, GI_INFO_TYPE_STRUCT)
GI_DEFINE_BASE_INFO_TYPE (gi_union_info, GI_INFO_TYPE_UNION)
GI_DEFINE_BASE_INFO_TYPE (gi_enum_info, GI_INFO_TYPE_ENUM)
GI_DEFINE_BASE_INFO_TYPE (gi_flags_info, GI_INFO_TYPE_FLAGS)
GI_DEFINE_BASE_INFO_TYPE (gi_object_info, GI_INFO_TYPE_OBJECT)
GI_DEFINE_BASE_INFO_TYPE (gi_interface_info, GI_INFO_TYPE_INTERFACE)
GI_DEFINE_BASE_INFO_TYPE (gi_constant_info, GI_INFO_TYPE_CONSTANT)
GI_DEFINE_BASE_INFO_TYPE (gi_value_info, GI_INFO_TYPE_VALUE)
GI_DEFINE_BASE_INFO_TYPE (gi_signal_info, GI_INFO_TYPE_SIGNAL)
GI_DEFINE_BASE_INFO_TYPE (gi_vfunc_info, GI_INFO_TYPE_VFUNC)
GI_DEFINE_BASE_INFO_TYPE (gi_property_info, GI_INFO_TYPE_PROPERTY)
GI_DEFINE_BASE_INFO_TYPE (gi_field_info, GI_INFO_TYPE_FIELD)
GI_DEFINE_BASE_INFO_TYPE (gi_arg_info, GI_INFO_TYPE_ARG)
GI_DEFINE_BASE_INFO_TYPE (gi_type_info, GI_INFO_TYPE_TYPE)
GI_DEFINE_BASE_INFO_TYPE (gi_unresolved_info, GI_INFO_TYPE_UNRESOLVED)

void
gi_base_info_init_types (void)
{
  static size_t register_types_once = 0;

  if (g_once_init_enter (&register_types_once))
    {
      const struct
        {
          GIInfoType info_type;
          const char *type_name;
          size_t instance_size;
          GClassInitFunc class_init;
          GIInfoType parent_info_type;  /* 0 for GIBaseInfo */
          GTypeFlags type_flags;
        }
      types[] =
        {
          { GI_INFO_TYPE_CALLABLE, "GICallableInfo", sizeof (GICallableInfo), gi_callable_info_class_init, 0, G_TYPE_FLAG_ABSTRACT },
          { GI_INFO_TYPE_FUNCTION, "GIFunctionInfo", sizeof (GIFunctionInfo), gi_function_info_class_init, GI_INFO_TYPE_CALLABLE, G_TYPE_FLAG_NONE },
          { GI_INFO_TYPE_CALLBACK, "GICallbackInfo", sizeof (GICallbackInfo), gi_callback_info_class_init, GI_INFO_TYPE_CALLABLE, G_TYPE_FLAG_NONE },
          { GI_INFO_TYPE_REGISTERED_TYPE, "GIRegisteredTypeInfo", sizeof (GIRegisteredTypeInfo), gi_registered_type_info_class_init, 0, G_TYPE_FLAG_ABSTRACT },
          { GI_INFO_TYPE_STRUCT, "GIStructInfo", sizeof (GIStructInfo), gi_struct_info_class_init, GI_INFO_TYPE_REGISTERED_TYPE, G_TYPE_FLAG_NONE },
          { GI_INFO_TYPE_UNION, "GIUnionInfo", sizeof (GIUnionInfo), gi_union_info_class_init, GI_INFO_TYPE_REGISTERED_TYPE, G_TYPE_FLAG_NONE },
          { GI_INFO_TYPE_ENUM, "GIEnumInfo", sizeof (GIEnumInfo), gi_enum_info_class_init, GI_INFO_TYPE_REGISTERED_TYPE, G_TYPE_FLAG_NONE },
          { GI_INFO_TYPE_FLAGS, "GIFlagsInfo", sizeof (GIFlagsInfo), gi_flags_info_class_init, GI_INFO_TYPE_ENUM, G_TYPE_FLAG_NONE },
          { GI_INFO_TYPE_OBJECT, "GIObjectInfo", sizeof (GIObjectInfo), gi_object_info_class_init, GI_INFO_TYPE_REGISTERED_TYPE, G_TYPE_FLAG_NONE },
          { GI_INFO_TYPE_INTERFACE, "GIInterfaceInfo", sizeof (GIInterfaceInfo), gi_interface_info_class_init, GI_INFO_TYPE_REGISTERED_TYPE, G_TYPE_FLAG_NONE },
          { GI_INFO_TYPE_CONSTANT, "GIConstantInfo", sizeof (GIConstantInfo), gi_constant_info_class_init, 0, G_TYPE_FLAG_NONE },
          { GI_INFO_TYPE_VALUE, "GIValueInfo", sizeof (GIValueInfo), gi_value_info_class_init, 0, G_TYPE_FLAG_NONE },
          { GI_INFO_TYPE_SIGNAL, "GISignalInfo", sizeof (GISignalInfo), gi_signal_info_class_init, GI_INFO_TYPE_CALLABLE, G_TYPE_FLAG_NONE },
          { GI_INFO_TYPE_VFUNC, "GIVFuncInfo", sizeof (GIVFuncInfo), gi_vfunc_info_class_init, GI_INFO_TYPE_CALLABLE, G_TYPE_FLAG_NONE },
          { GI_INFO_TYPE_PROPERTY, "GIPropertyInfo", sizeof (GIPropertyInfo), gi_property_info_class_init, 0, G_TYPE_FLAG_NONE },
          { GI_INFO_TYPE_FIELD, "GIFieldInfo", sizeof (GIFieldInfo), gi_field_info_class_init, 0, G_TYPE_FLAG_NONE },
          { GI_INFO_TYPE_ARG, "GIArgInfo", sizeof (GIArgInfo), gi_arg_info_class_init, 0, G_TYPE_FLAG_NONE },
          { GI_INFO_TYPE_TYPE, "GITypeInfo", sizeof (GITypeInfo), gi_type_info_class_init, 0, G_TYPE_FLAG_NONE },
          { GI_INFO_TYPE_UNRESOLVED, "GIUnresolvedInfo", sizeof (GIUnresolvedInfo), gi_unresolved_info_class_init, 0, G_TYPE_FLAG_NONE },
        };

      for (size_t i = 0; i < G_N_ELEMENTS (types); i++)
        {
          GType registered_type, parent_type;

          parent_type = (types[i].parent_info_type == 0) ? GI_TYPE_BASE_INFO : gi_base_info_types[types[i].parent_info_type];
          g_assert (parent_type != G_TYPE_INVALID);

          registered_type = gi_base_info_type_register_static (g_intern_static_string (types[i].type_name),
                                                               types[i].instance_size,
                                                               types[i].class_init,
                                                               parent_type,
                                                               types[i].type_flags);
          gi_base_info_types[types[i].info_type] = registered_type;
        }

      g_once_init_leave (&register_types_once, 1);
    }
}

/* info creation */
GIBaseInfo *
gi_info_new_full (GIInfoType    type,
                  GIRepository *repository,
                  GIBaseInfo   *container,
                  GITypelib    *typelib,
                  uint32_t      offset)
{
  GIRealInfo *info;

  g_return_val_if_fail (container != NULL || repository != NULL, NULL);
  g_return_val_if_fail (GI_IS_REPOSITORY (repository), NULL);
  g_return_val_if_fail (offset <= G_MAXUINT32, NULL);

  gi_base_info_init_types ();
  g_assert (gi_base_info_types[type] != G_TYPE_INVALID);
  info = (GIRealInfo *) g_type_create_instance (gi_base_info_types[type]);

  info->typelib = typelib;
  info->offset = offset;

  if (container)
    info->container = container;
  if (container && container->ref_count != INVALID_REFCOUNT)
    gi_base_info_ref (info->container);

  /* Don’t keep a strong ref, since the repository keeps a cache of #GIBaseInfos
   * and holds refs on them. If we kept a ref here, there’d be a cycle.
   * Don’t keep a weak ref either, as that would make creating/destroying a
   * #GIBaseInfo noticeably more expensive, and infos are performance critical
   * for bindings.
   * As stated in the documentation, the mitigation here is to require the user
   * to keep the #GIRepository alive longer than any of its #GIBaseInfos. */
  info->repository = repository;

  return (GIBaseInfo*)info;
}

/**
 * gi_base_info_new:
 * @type: type of the info to create
 * @container: (nullable): info which contains this one
 * @typelib: typelib containing the info
 * @offset: offset of the info within @typelib, in bytes
 *
 * Create a new #GIBaseInfo representing an object of the given @type from
 * @offset of @typelib.
 *
 * Returns: (transfer full): The new #GIBaseInfo, unref with
 *   [method@GIRepository.BaseInfo.unref]
 * Since: 2.80
 */
GIBaseInfo *
gi_base_info_new (GIInfoType  type,
                  GIBaseInfo *container,
                  GITypelib  *typelib,
                  size_t      offset)
{
  return gi_info_new_full (type, ((GIRealInfo*)container)->repository, container, typelib, offset);
}

/*< private >
 * gi_info_init:
 * @info: (out caller-allocates): caller-allocated #GIRealInfo to populate
 * @type: type of the info to create
 * @repository: repository the info is in
 * @container: (nullable): info which contains this one
 * @typelib: typelib containing the info
 * @offset: offset of the info within @typelib, in bytes
 *
 * Initialise a stack-allocated #GIBaseInfo representing an object of the given
 * @type from @offset of @typelib.
 *
 * Since: 2.80
 */
void
gi_info_init (GIRealInfo   *info,
              GType         type,
              GIRepository *repository,
              GIBaseInfo   *container,
              GITypelib    *typelib,
              uint32_t      offset)
{
  memset (info, 0, sizeof (GIRealInfo));

  /* Evil setup of a stack allocated #GTypeInstance. This is not something it’s
   * really designed to do.
   *
   * This function *must* be kept in sync with gi_base_info_init(), which is
   * the equivalent function for dynamically allocated types. */
  info->parent_instance.g_class = g_type_class_ref (type);

  /* g_type_create_instance() calls the #GInstanceInitFunc for each of the
   * parent types, down to (and including) @type. We don’t need to do that, as
   * #GIBaseInfo is fundamental so doesn’t have a parent type, the instance init
   * function for #GIBaseInfo is gi_base_info_init() (which only sets the
   * refcount, which we already do here), and subtypes of #GIBaseInfo don’t have
   * instance init functions (see gi_base_info_type_register_static()). */

  /* Invalid refcount used to flag stack-allocated infos */
  info->ref_count = INVALID_REFCOUNT;
  info->typelib = typelib;
  info->offset = offset;

  if (container)
    info->container = container;

  g_assert (GI_IS_REPOSITORY (repository));
  info->repository = repository;
}

/**
 * gi_base_info_clear:
 * @info: (type GIRepository.BaseInfo): a #GIBaseInfo
 *
 * Clears memory allocated internally by a stack-allocated
 * [type@GIRepository.BaseInfo].
 *
 * This does not deallocate the [type@GIRepository.BaseInfo] struct itself. It
 * does clear the struct to zero so that calling this function subsequent times
 * on the same struct is a no-op.
 *
 * This must only be called on stack-allocated [type@GIRepository.BaseInfo]s.
 * Use [method@GIRepository.BaseInfo.unref] for heap-allocated ones.
 *
 * Since: 2.80
 */
void
gi_base_info_clear (void *info)
{
  GIBaseInfo *rinfo = (GIBaseInfo *) info;

  /* If @info is zero-filled, do nothing. This allows gi_base_info_clear() to be
   * used with g_auto(). */
  if (rinfo->ref_count == 0)
    return;

  g_return_if_fail (GI_IS_BASE_INFO (rinfo));

  g_assert (rinfo->ref_count == INVALID_REFCOUNT);

  GI_BASE_INFO_GET_CLASS (info)->finalize (rinfo);

  g_type_class_unref (rinfo->parent_instance.g_class);

  memset (rinfo, 0, sizeof (*rinfo));
}

GIBaseInfo *
gi_info_from_entry (GIRepository *repository,
                    GITypelib    *typelib,
                    uint16_t      index)
{
  GIBaseInfo *result;
  DirEntry *entry = gi_typelib_get_dir_entry (typelib, index);

  if (entry->local)
    result = gi_info_new_full (gi_typelib_blob_type_to_info_type (entry->blob_type),
                               repository, NULL, typelib, entry->offset);
  else
    {
      const char *namespace = gi_typelib_get_string (typelib, entry->offset);
      const char *name = gi_typelib_get_string (typelib, entry->name);

      result = gi_repository_find_by_name (repository, namespace, name);
      if (result == NULL)
        {
          GIUnresolvedInfo *unresolved;

          unresolved = (GIUnresolvedInfo *) gi_info_new_full (GI_INFO_TYPE_UNRESOLVED,
                                                              repository,
                                                              NULL,
                                                              typelib,
                                                              entry->offset);

          unresolved->name = name;
          unresolved->namespace = namespace;

          return (GIBaseInfo *)unresolved;
        }
      return (GIBaseInfo *)result;
    }

  return (GIBaseInfo *)result;
}

GITypeInfo *
gi_type_info_new (GIBaseInfo *container,
                  GITypelib  *typelib,
                  uint32_t    offset)
{
  SimpleTypeBlob *type = (SimpleTypeBlob *)&typelib->data[offset];

  return (GITypeInfo *) gi_base_info_new (GI_INFO_TYPE_TYPE, container, typelib,
                                          (type->flags.reserved == 0 && type->flags.reserved2 == 0) ? offset : type->offset);
}

/*< private >
 * gi_type_info_init:
 * @info: (out caller-allocates): caller-allocated #GITypeInfo to populate
 * @container: (nullable): info which contains this one
 * @typelib: typelib containing the info
 * @offset: offset of the info within @typelib, in bytes
 *
 * Initialise a stack-allocated #GITypeInfo representing an object of type
 * [type@GIRepository.TypeInfo] from @offset of @typelib.
 *
 * This is a specialised form of [func@GIRepository.info_init] for type
 * information.
 *
 * Since: 2.80
 */
void
gi_type_info_init (GITypeInfo *info,
                   GIBaseInfo *container,
                   GITypelib  *typelib,
                   uint32_t    offset)
{
  GIRealInfo *rinfo = (GIRealInfo*)container;
  SimpleTypeBlob *type = (SimpleTypeBlob *)&typelib->data[offset];

  gi_info_init ((GIRealInfo*)info, GI_TYPE_TYPE_INFO, rinfo->repository, container, typelib,
                (type->flags.reserved == 0 && type->flags.reserved2 == 0) ? offset : type->offset);
}

/* GIBaseInfo functions */

/**
 * GIBaseInfo:
 *
 * `GIBaseInfo` is the common base struct of all other Info structs
 * accessible through the [class@GIRepository.Repository] API.
 *
 * All info structures can be cast to a `GIBaseInfo`, for instance:
 *
 * ```c
 *    GIFunctionInfo *function_info = …;
 *    GIBaseInfo *info = (GIBaseInfo *) function_info;
 * ```
 *
 * Most [class@GIRepository.Repository] APIs returning a `GIBaseInfo` are
 * actually creating a new struct; in other words,
 * [method@GIRepository.BaseInfo.unref] has to be called when done accessing the
 * data.
 *
 * `GIBaseInfo` structuress are normally accessed by calling either
 * [method@GIRepository.Repository.find_by_name],
 * [method@GIRepository.Repository.find_by_gtype] or
 * [method@GIRepository.get_info].
 *
 * ```c
 * GIBaseInfo *button_info =
 *   gi_repository_find_by_name (NULL, "Gtk", "Button");
 *
 * // use button_info…
 *
 * gi_base_info_unref (button_info);
 * ```
 *
 * Since: 2.80
 */

/**
 * gi_base_info_ref:
 * @info: (type GIRepository.BaseInfo): a #GIBaseInfo
 *
 * Increases the reference count of @info.
 *
 * Returns: (transfer full): the same @info.
 * Since: 2.80
 */
GIBaseInfo *
gi_base_info_ref (void *info)
{
  GIRealInfo *rinfo = (GIRealInfo*)info;

  g_return_val_if_fail (GI_IS_BASE_INFO (info), NULL);

  g_assert (rinfo->ref_count != INVALID_REFCOUNT);
  g_atomic_ref_count_inc (&rinfo->ref_count);

  return info;
}

/**
 * gi_base_info_unref:
 * @info: (type GIRepository.BaseInfo) (transfer full): a #GIBaseInfo
 *
 * Decreases the reference count of @info. When its reference count
 * drops to 0, the info is freed.
 *
 * This must not be called on stack-allocated [type@GIRepository.BaseInfo]s —
 * use [method@GIRepository.BaseInfo.clear] for that.
 *
 * Since: 2.80
 */
void
gi_base_info_unref (void *info)
{
  GIRealInfo *rinfo = (GIRealInfo*)info;

  g_return_if_fail (GI_IS_BASE_INFO (info));

  g_assert (rinfo->ref_count > 0 && rinfo->ref_count != INVALID_REFCOUNT);

  if (g_atomic_ref_count_dec (&rinfo->ref_count))
    {
      GI_BASE_INFO_GET_CLASS (info)->finalize (info);
      g_type_free_instance ((GTypeInstance *) info);
    }
}

/**
 * gi_base_info_get_info_type:
 * @info: a #GIBaseInfo
 *
 * Obtain the info type of the `GIBaseInfo`.
 *
 * Returns: the info type of @info
 * Since: 2.80
 */
GIInfoType
gi_base_info_get_info_type (GIBaseInfo *info)
{
  return GI_BASE_INFO_GET_CLASS (info)->info_type;
}

/**
 * gi_base_info_get_name:
 * @info: a #GIBaseInfo
 *
 * Obtain the name of the @info.
 *
 * What the name represents depends on the type of the
 * @info. For instance for [class@GIRepository.FunctionInfo] it is the name of
 * the function.
 *
 * Returns: (nullable): the name of @info or `NULL` if it lacks a name.
 * Since: 2.80
 */
const char *
gi_base_info_get_name (GIBaseInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo*)info;
  g_assert (rinfo->ref_count > 0);
  switch (gi_base_info_get_info_type ((GIBaseInfo *) info))
    {
    case GI_INFO_TYPE_FUNCTION:
    case GI_INFO_TYPE_CALLBACK:
    case GI_INFO_TYPE_STRUCT:
    case GI_INFO_TYPE_ENUM:
    case GI_INFO_TYPE_FLAGS:
    case GI_INFO_TYPE_OBJECT:
    case GI_INFO_TYPE_INTERFACE:
    case GI_INFO_TYPE_CONSTANT:
    case GI_INFO_TYPE_UNION:
      {
        CommonBlob *blob = (CommonBlob *)&rinfo->typelib->data[rinfo->offset];

        return gi_typelib_get_string (rinfo->typelib, blob->name);
      }
      break;

    case GI_INFO_TYPE_VALUE:
      {
        ValueBlob *blob = (ValueBlob *)&rinfo->typelib->data[rinfo->offset];

        return gi_typelib_get_string (rinfo->typelib, blob->name);
      }
      break;

    case GI_INFO_TYPE_SIGNAL:
      {
        SignalBlob *blob = (SignalBlob *)&rinfo->typelib->data[rinfo->offset];

        return gi_typelib_get_string (rinfo->typelib, blob->name);
      }
      break;

    case GI_INFO_TYPE_PROPERTY:
      {
        PropertyBlob *blob = (PropertyBlob *)&rinfo->typelib->data[rinfo->offset];

        return gi_typelib_get_string (rinfo->typelib, blob->name);
      }
      break;

    case GI_INFO_TYPE_VFUNC:
      {
        VFuncBlob *blob = (VFuncBlob *)&rinfo->typelib->data[rinfo->offset];

        return gi_typelib_get_string (rinfo->typelib, blob->name);
      }
      break;

    case GI_INFO_TYPE_FIELD:
      {
        FieldBlob *blob = (FieldBlob *)&rinfo->typelib->data[rinfo->offset];

        return gi_typelib_get_string (rinfo->typelib, blob->name);
      }
      break;

    case GI_INFO_TYPE_ARG:
      {
        ArgBlob *blob = (ArgBlob *)&rinfo->typelib->data[rinfo->offset];

        return gi_typelib_get_string (rinfo->typelib, blob->name);
      }
      break;
    case GI_INFO_TYPE_UNRESOLVED:
      {
        GIUnresolvedInfo *unresolved = (GIUnresolvedInfo *)info;

        return unresolved->name;
      }
      break;
    case GI_INFO_TYPE_TYPE:
      return NULL;
    default: ;
      g_assert_not_reached ();
      /* unnamed */
    }

  return NULL;
}

/**
 * gi_base_info_get_namespace:
 * @info: a #GIBaseInfo
 *
 * Obtain the namespace of @info.
 *
 * Returns: the namespace
 * Since: 2.80
 */
const char *
gi_base_info_get_namespace (GIBaseInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo*) info;
  Header *header = (Header *)rinfo->typelib->data;

  g_assert (rinfo->ref_count > 0);

  if (gi_base_info_get_info_type (info) == GI_INFO_TYPE_UNRESOLVED)
    {
      GIUnresolvedInfo *unresolved = (GIUnresolvedInfo *)info;

      return unresolved->namespace;
    }

  return gi_typelib_get_string (rinfo->typelib, header->namespace);
}

/**
 * gi_base_info_is_deprecated:
 * @info: a #GIBaseInfo
 *
 * Obtain whether the @info is represents a metadata which is
 * deprecated.
 *
 * Returns: `TRUE` if deprecated
 * Since: 2.80
 */
gboolean
gi_base_info_is_deprecated (GIBaseInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo*) info;
  switch (gi_base_info_get_info_type ((GIBaseInfo *) info))
    {
    case GI_INFO_TYPE_FUNCTION:
    case GI_INFO_TYPE_CALLBACK:
    case GI_INFO_TYPE_STRUCT:
    case GI_INFO_TYPE_ENUM:
    case GI_INFO_TYPE_FLAGS:
    case GI_INFO_TYPE_OBJECT:
    case GI_INFO_TYPE_INTERFACE:
    case GI_INFO_TYPE_CONSTANT:
      {
        CommonBlob *blob = (CommonBlob *)&rinfo->typelib->data[rinfo->offset];

        return blob->deprecated;
      }
      break;

    case GI_INFO_TYPE_VALUE:
      {
        ValueBlob *blob = (ValueBlob *)&rinfo->typelib->data[rinfo->offset];

        return blob->deprecated;
      }
      break;

    case GI_INFO_TYPE_SIGNAL:
      {
        SignalBlob *blob = (SignalBlob *)&rinfo->typelib->data[rinfo->offset];

        return blob->deprecated;
      }
      break;

    case GI_INFO_TYPE_PROPERTY:
      {
        PropertyBlob *blob = (PropertyBlob *)&rinfo->typelib->data[rinfo->offset];

        return blob->deprecated;
      }
      break;

    case GI_INFO_TYPE_VFUNC:
    case GI_INFO_TYPE_FIELD:
    case GI_INFO_TYPE_ARG:
    case GI_INFO_TYPE_TYPE:
    default: ;
      /* no deprecation flag for these */
    }

  return FALSE;
}

/**
 * gi_base_info_get_attribute:
 * @info: a #GIBaseInfo
 * @name: a freeform string naming an attribute
 *
 * Retrieve an arbitrary attribute associated with this node.
 *
 * Returns: (nullable): The value of the attribute, or `NULL` if no such
 *   attribute exists
 * Since: 2.80
 */
const char *
gi_base_info_get_attribute (GIBaseInfo  *info,
                            const char *name)
{
  GIAttributeIter iter = GI_ATTRIBUTE_ITER_INIT;
  const char *curname, *curvalue;
  while (gi_base_info_iterate_attributes (info, &iter, &curname, &curvalue))
    {
      if (strcmp (name, curname) == 0)
        return (const char *) curvalue;
    }

  return NULL;
}

static int
cmp_attribute (const void *av,
               const void *bv)
{
  const AttributeBlob *a = av;
  const AttributeBlob *b = bv;

  if (a->offset < b->offset)
    return -1;
  else if (a->offset == b->offset)
    return 0;
  else
    return 1;
}

/*< private >
 * _attribute_blob_find_first:
 * @GIBaseInfo: A #GIBaseInfo.
 * @blob_offset: The offset for the blob to find the first attribute for.
 *
 * Searches for the first #AttributeBlob for @blob_offset and returns
 * it if found.
 *
 * Returns: (transfer none): A pointer to #AttributeBlob or `NULL` if not found.
 * Since: 2.80
 */
AttributeBlob *
_attribute_blob_find_first (GIBaseInfo *info,
                            uint32_t    blob_offset)
{
  GIRealInfo *rinfo = (GIRealInfo *) info;
  Header *header = (Header *)rinfo->typelib->data;
  AttributeBlob blob, *first, *res, *previous;

  blob.offset = blob_offset;

  first = (AttributeBlob *) &rinfo->typelib->data[header->attributes];

  res = bsearch (&blob, first, header->n_attributes,
                 header->attribute_blob_size, cmp_attribute);

  if (res == NULL)
    return NULL;

  previous = res - 1;
  while (previous >= first && previous->offset == blob_offset)
    {
      res = previous;
      previous = res - 1;
    }

  return res;
}

/**
 * gi_base_info_iterate_attributes:
 * @info: a #GIBaseInfo
 * @iterator: (inout): a [type@GIRepository.AttributeIter] structure, must be
 *   initialized; see below
 * @name: (out) (transfer none): Returned name, must not be freed
 * @value: (out) (transfer none): Returned name, must not be freed
 *
 * Iterate over all attributes associated with this node.
 *
 * The iterator structure is typically stack allocated, and must have its first
 * member initialized to `NULL`.  Attributes are arbitrary namespaced key–value
 * pairs which can be attached to almost any item.  They are intended for use
 * by software higher in the toolchain than bindings, and are distinct from
 * normal GIR annotations.
 *
 * Both the @name and @value should be treated as constants
 * and must not be freed.
 *
 * ```c
 * void
 * print_attributes (GIBaseInfo *info)
 * {
 *   GIAttributeIter iter = GI_ATTRIBUTE_ITER_INIT;
 *   const char *name;
 *   const char *value;
 *   while (gi_base_info_iterate_attributes (info, &iter, &name, &value))
 *     {
 *       g_print ("attribute name: %s value: %s", name, value);
 *     }
 * }
 * ```
 *
 * Returns: `TRUE` if there are more attributes
 * Since: 2.80
 */
gboolean
gi_base_info_iterate_attributes (GIBaseInfo       *info,
                                 GIAttributeIter  *iterator,
                                 const char      **name,
                                 const char      **value)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  Header *header = (Header *)rinfo->typelib->data;
  AttributeBlob *next, *after;

  after = (AttributeBlob *) &rinfo->typelib->data[header->attributes +
                                                  header->n_attributes * header->attribute_blob_size];

  if (iterator->data != NULL)
    next = (AttributeBlob *) iterator->data;
  else
    next = _attribute_blob_find_first (info, rinfo->offset);

  if (next == NULL || next->offset != rinfo->offset || next >= after)
    return FALSE;

  *name = gi_typelib_get_string (rinfo->typelib, next->name);
  *value = gi_typelib_get_string (rinfo->typelib, next->value);
  iterator->data = next + 1;

  return TRUE;
}

/**
 * gi_base_info_get_container:
 * @info: a #GIBaseInfo
 *
 * Obtain the container of the @info.
 *
 * The container is the parent `GIBaseInfo`. For instance, the parent of a
 * [class@GIRepository.FunctionInfo] is an [class@GIRepository.ObjectInfo] or
 * [class@GIRepository.InterfaceInfo].
 *
 * Returns: (transfer none): the container
 * Since: 2.80
 */
GIBaseInfo *
gi_base_info_get_container (GIBaseInfo *info)
{
  return ((GIRealInfo*)info)->container;
}

/**
 * gi_base_info_get_typelib:
 * @info: a #GIBaseInfo
 *
 * Obtain the typelib this @info belongs to
 *
 * Returns: (transfer none): the typelib
 * Since: 2.80
 */
GITypelib *
gi_base_info_get_typelib (GIBaseInfo *info)
{
  return ((GIRealInfo*)info)->typelib;
}

/**
 * gi_base_info_equal:
 * @info1: a #GIBaseInfo
 * @info2: a #GIBaseInfo
 *
 * Compare two `GIBaseInfo`s.
 *
 * Using pointer comparison is not practical since many functions return
 * different instances of `GIBaseInfo` that refers to the same part of the
 * TypeLib; use this function instead to do `GIBaseInfo` comparisons.
 *
 * Returns: `TRUE` if and only if @info1 equals @info2.
 * Since: 2.80
 */
gboolean
gi_base_info_equal (GIBaseInfo *info1, GIBaseInfo *info2)
{
  /* Compare the TypeLib pointers, which are mmapped. */
  GIRealInfo *rinfo1 = (GIRealInfo*)info1;
  GIRealInfo *rinfo2 = (GIRealInfo*)info2;
  return rinfo1->typelib->data + rinfo1->offset == rinfo2->typelib->data + rinfo2->offset;
}
