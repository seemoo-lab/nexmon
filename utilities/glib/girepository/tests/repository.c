/*
 * Copyright 2008-2011 Colin Walters <walters@verbum.org>
 * Copyright 2011 Laszlo Pandy <lpandy@src.gnome.org>
 * Copyright 2011 Torsten Schönfeld <kaffeetisch@gmx.de>
 * Copyright 2011, 2012 Pavel Holejsovsky <pavel.holejsovsky@gmail.com>
 * Copyright 2013 Martin Pitt <martinpitt@gnome.org>
 * Copyright 2014 Giovanni Campagna <gcampagna@src.gnome.org>
 * Copyright 2018 Christoph Reiter
 * Copyright 2019, 2024 Philip Chimento <philip.chimento@gmail.com>
 * Copyright 2022 Emmanuele Bassi <ebassi@gnome.org>
 * Copyright 2023 GNOME Foundation, Inc.
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
 *
 * Author: Philip Withnall <pwithnall@gnome.org>
 */

#include "config.h"

#include "gio.h"
#include "girepository.h"
#include "glib.h"
#include "test-common.h"

#if defined(G_OS_UNIX) && !defined(__APPLE__)
#include "gio/gdesktopappinfo.h"
#elif defined(G_OS_WIN32)
#include "gio/gwin32inputstream.h"
#endif

static void
test_repository_basic (RepositoryFixture *fx,
                       const void *unused)
{
  const char * const * search_paths;
  char **namespaces = NULL;
  size_t n_namespaces;
#if defined(G_OS_UNIX)
  const char *expected_namespaces[] = { "GLib", "GLibUnix", NULL };
#elif defined(G_OS_WIN32)
  const char *expected_namespaces[] = { "GLib", "GLibWin32", NULL };
#else
  const char *expected_namespaces[] = { "GLib", NULL };
#endif
  char **versions;
  size_t n_versions;
  const char *prefix = NULL;

  g_test_summary ("Test basic opening of a repository and requiring a typelib");

  versions = gi_repository_enumerate_versions (fx->repository, "SomeInvalidNamespace", &n_versions);
  g_assert_nonnull (versions);
  g_assert_cmpstrv (versions, ((char *[]){NULL}));
  g_assert_cmpuint (n_versions, ==, 0);
  g_clear_pointer (&versions, g_strfreev);

  versions = gi_repository_enumerate_versions (fx->repository, "GLib", NULL);
  g_assert_nonnull (versions);
  g_assert_cmpstrv (versions, ((char *[]){"2.0", NULL}));
  g_clear_pointer (&versions, g_strfreev);

  search_paths = gi_repository_get_search_path (fx->repository, NULL);
  g_assert_nonnull (search_paths);
  g_assert_cmpuint (g_strv_length ((char **) search_paths), >, 0);
  g_assert_cmpstr (search_paths[0], ==, fx->gobject_typelib_dir);

  namespaces = gi_repository_get_loaded_namespaces (fx->repository, &n_namespaces);
  g_assert_cmpstrv (namespaces, expected_namespaces);
  g_assert_cmpuint (n_namespaces, ==, g_strv_length ((char **) expected_namespaces));
  g_strfreev (namespaces);

  prefix = gi_repository_get_c_prefix (fx->repository, "GLib");
  g_assert_nonnull (prefix);
  g_assert_cmpstr (prefix, ==, "G");
}

static void
test_repository_info (RepositoryFixture *fx,
                      const void *unused)
{
  GIBaseInfo *not_found_info = NULL;
  GIObjectInfo *object_info = NULL, *object_info_by_gtype = NULL;
  GISignalInfo *signal_info = NULL;
  GIFunctionInfo *method_info = NULL;
  GType gtype;

  g_test_summary ("Test retrieving some basic info blobs from a typelib");

  not_found_info = gi_repository_find_by_name (fx->repository, "GObject", "ThisDoesNotExist");
  g_assert_null (not_found_info);

  object_info = GI_OBJECT_INFO (gi_repository_find_by_name (fx->repository, "GObject", "Object"));
  g_assert_nonnull (object_info);
  g_assert_true (GI_IS_OBJECT_INFO (object_info));
  g_assert_true (GI_IS_REGISTERED_TYPE_INFO (object_info));
  g_assert_true (GI_IS_BASE_INFO (object_info));

  g_assert_cmpstr (gi_base_info_get_name (GI_BASE_INFO (object_info)), ==, "Object");
  g_assert_cmpstr (gi_base_info_get_namespace (GI_BASE_INFO (object_info)), ==, "GObject");

  gtype = gi_registered_type_info_get_g_type (GI_REGISTERED_TYPE_INFO (object_info));
  g_assert_true (g_type_is_a (gtype, G_TYPE_OBJECT));

  object_info_by_gtype = GI_OBJECT_INFO (gi_repository_find_by_gtype (fx->repository, G_TYPE_OBJECT));
  g_assert_nonnull (object_info);

  signal_info = gi_object_info_find_signal (object_info, "does-not-exist");
  g_assert_null (signal_info);

  signal_info = gi_object_info_find_signal (object_info, "notify");
  g_assert_nonnull (signal_info);
  g_assert_true (GI_IS_SIGNAL_INFO (signal_info));
  g_assert_true (GI_IS_CALLABLE_INFO (signal_info));
  g_assert_true (GI_IS_BASE_INFO (signal_info));

  g_assert_cmpint (gi_signal_info_get_flags (signal_info), ==,
                   G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE | G_SIGNAL_DETAILED | G_SIGNAL_NO_HOOKS | G_SIGNAL_ACTION);

  g_assert_cmpuint (gi_object_info_get_n_methods (object_info), >, 2);

  method_info = gi_object_info_find_method (object_info, "get_property");
  g_assert_nonnull (method_info);
  g_assert_true (GI_IS_FUNCTION_INFO (method_info));
  g_assert_true (GI_IS_CALLABLE_INFO (method_info));
  g_assert_true (GI_IS_BASE_INFO (method_info));

  g_assert_true (gi_callable_info_is_method (GI_CALLABLE_INFO (method_info)));
  g_assert_cmpuint (gi_callable_info_get_n_args (GI_CALLABLE_INFO (method_info)), ==, 2);
  g_clear_pointer (&method_info, gi_base_info_unref);

  method_info = gi_object_info_get_method (object_info,
                                           gi_object_info_get_n_methods (object_info) - 1);
  g_assert_true (gi_callable_info_is_method (GI_CALLABLE_INFO (method_info)));
  g_assert_cmpuint (gi_callable_info_get_n_args (GI_CALLABLE_INFO (method_info)), >, 0);
  g_clear_pointer (&method_info, gi_base_info_unref);

  gi_base_info_unref (signal_info);
  gi_base_info_unref (object_info);
  gi_base_info_unref (object_info_by_gtype);
}

static void
test_repository_dependencies (RepositoryFixture *fx,
                              const void *unused)
{
  GError *error = NULL;
  char **dependencies;
  size_t n_dependencies;

  g_test_summary ("Test ensures namespace dependencies are correctly exposed");

  dependencies = gi_repository_get_dependencies (fx->repository, "GObject", &n_dependencies);
  g_assert_cmpuint (g_strv_length (dependencies), ==, 1);
  g_assert_cmpuint (n_dependencies, ==, 1);
  g_assert_true (g_strv_contains ((const char **) dependencies, "GLib-2.0"));

  g_clear_error (&error);
  g_clear_pointer (&dependencies, g_strfreev);
}

static void
test_repository_base_info_clear (RepositoryFixture *fx,
                                 const void        *unused)
{
  GITypeInfo zeroed_type_info = { 0, };
  GITypeInfo idempotent_type_info;
  GIObjectInfo *object_info = NULL;
  GIFunctionInfo *method_info = NULL;
  GIArgInfo *arg_info = NULL;

  g_test_summary ("Test calling gi_base_info_clear() on a zero-filled struct");

  /* Load a valid #GITypeInfo onto the stack and clear it multiple times to
   * check gi_base_info_clear() is idempotent after the first call. */
  object_info = GI_OBJECT_INFO (gi_repository_find_by_name (fx->repository, "GObject", "Object"));
  g_assert_nonnull (object_info);
  method_info = gi_object_info_find_method (object_info, "get_property");
  g_assert_nonnull (method_info);
  arg_info = gi_callable_info_get_arg (GI_CALLABLE_INFO (method_info), 0);
  g_assert_nonnull (arg_info);
  gi_arg_info_load_type_info (arg_info, &idempotent_type_info);

  gi_base_info_clear (&idempotent_type_info);
  gi_base_info_clear (&idempotent_type_info);
  gi_base_info_clear (&idempotent_type_info);

  g_clear_pointer (&arg_info, gi_base_info_unref);
  g_clear_pointer (&method_info, gi_base_info_unref);
  g_clear_pointer (&object_info, gi_base_info_unref);

  /* Try clearing a #GITypeInfo which has always been zero-filled on the stack. */
  gi_base_info_clear (&zeroed_type_info);
  gi_base_info_clear (&zeroed_type_info);
  gi_base_info_clear (&zeroed_type_info);
}

static void
test_repository_arg_info (RepositoryFixture *fx,
                          const void *unused)
{
  GIObjectInfo *object_info = NULL;
  GIStructInfo *struct_info = NULL;
  GIFunctionInfo *method_info = NULL;
  GIArgInfo *arg_info = NULL;
  GITypeInfo *type_info = NULL;
  GITypeInfo type_info_stack;
  unsigned int idx;

  g_test_summary ("Test retrieving GIArgInfos from a typelib");

  /* Test all the methods of GIArgInfo. Here we’re looking at the
   * `const char *property_name` argument of g_object_get_property(). (The
   * ‘self’ argument is not exposed through gi_callable_info_get_arg().) */
  object_info = GI_OBJECT_INFO (gi_repository_find_by_name (fx->repository, "GObject", "Object"));
  g_assert_nonnull (object_info);

  method_info = gi_object_info_find_method (object_info, "get_property");
  g_assert_nonnull (method_info);

  arg_info = gi_callable_info_get_arg (GI_CALLABLE_INFO (method_info), 0);
  g_assert_nonnull (arg_info);

  g_assert_cmpint (gi_arg_info_get_direction (arg_info), ==, GI_DIRECTION_IN);
  g_assert_false (gi_arg_info_is_return_value (arg_info));
  g_assert_false (gi_arg_info_is_optional (arg_info));
  g_assert_false (gi_arg_info_is_caller_allocates (arg_info));
  g_assert_false (gi_arg_info_may_be_null (arg_info));
  g_assert_false (gi_arg_info_is_skip (arg_info));
  g_assert_cmpint (gi_arg_info_get_ownership_transfer (arg_info), ==, GI_TRANSFER_NOTHING);
  g_assert_cmpint (gi_arg_info_get_scope (arg_info), ==, GI_SCOPE_TYPE_INVALID);
  g_assert_false (gi_arg_info_get_closure_index (arg_info, NULL));
  g_assert_false (gi_arg_info_get_closure_index (arg_info, &idx));
  g_assert_cmpuint (idx, ==, 0);
  g_assert_false (gi_arg_info_get_destroy_index (arg_info, NULL));
  g_assert_false (gi_arg_info_get_destroy_index (arg_info, &idx));
  g_assert_cmpuint (idx, ==, 0);

  type_info = gi_arg_info_get_type_info (arg_info);
  g_assert_nonnull (type_info);
  g_assert_true (gi_type_info_is_pointer (type_info));
  g_assert_cmpint (gi_type_info_get_tag (type_info), ==, GI_TYPE_TAG_UTF8);

  gi_arg_info_load_type_info (arg_info, &type_info_stack);
  g_assert_true (gi_type_info_is_pointer (&type_info_stack) == gi_type_info_is_pointer (type_info));
  g_assert_cmpint (gi_type_info_get_tag (&type_info_stack), ==, gi_type_info_get_tag (type_info));

  gi_base_info_clear (&type_info_stack);
  g_clear_pointer (&type_info, gi_base_info_unref);

  g_clear_pointer (&arg_info, gi_base_info_unref);
  g_clear_pointer (&method_info, gi_base_info_unref);
  g_clear_pointer (&object_info, gi_base_info_unref);

  /* Test an (out) argument. Here it’s the `guint *n_properties` from
   * g_object_class_list_properties(). */
  struct_info = GI_STRUCT_INFO (gi_repository_find_by_name (fx->repository, "GObject", "ObjectClass"));
  g_assert_nonnull (struct_info);

  method_info = gi_struct_info_find_method (struct_info, "list_properties");
  g_assert_nonnull (method_info);

  arg_info = gi_callable_info_get_arg (GI_CALLABLE_INFO (method_info), 0);
  g_assert_nonnull (arg_info);

  g_assert_cmpint (gi_arg_info_get_direction (arg_info), ==, GI_DIRECTION_OUT);
  g_assert_false (gi_arg_info_is_optional (arg_info));
  g_assert_false (gi_arg_info_is_caller_allocates (arg_info));
  g_assert_cmpint (gi_arg_info_get_ownership_transfer (arg_info), ==, GI_TRANSFER_EVERYTHING);

  g_clear_pointer (&arg_info, gi_base_info_unref);
  g_clear_pointer (&method_info, gi_base_info_unref);
  g_clear_pointer (&struct_info, gi_base_info_unref);
}

static void
test_repository_callable_info (RepositoryFixture *fx,
                               const void *unused)
{
  GIObjectInfo *object_info = NULL;
  GIFunctionInfo *method_info = NULL;
  GICallableInfo *callable_info;
  GITypeInfo *type_info = NULL;
  GITypeInfo type_info_stack;
  GIAttributeIter iter = GI_ATTRIBUTE_ITER_INIT;
  const char *name, *value;
  GIArgInfo *arg_info = NULL;
  GIArgInfo arg_info_stack;

  g_test_summary ("Test retrieving GICallableInfos from a typelib");

  /* Test all the methods of GICallableInfo. Here we’re looking at
   * g_object_get_qdata(). */
  object_info = GI_OBJECT_INFO (gi_repository_find_by_name (fx->repository, "GObject", "Object"));
  g_assert_nonnull (object_info);

  method_info = gi_object_info_find_method (object_info, "get_qdata");
  g_assert_nonnull (method_info);

  callable_info = GI_CALLABLE_INFO (method_info);

  g_assert_true (gi_callable_info_is_method (callable_info));
  g_assert_false (gi_callable_info_can_throw_gerror (callable_info));

  type_info = gi_callable_info_get_return_type (callable_info);
  g_assert_nonnull (type_info);
  g_assert_true (gi_type_info_is_pointer (type_info));
  g_assert_cmpint (gi_type_info_get_tag (type_info), ==, GI_TYPE_TAG_VOID);

  gi_callable_info_load_return_type (callable_info, &type_info_stack);
  g_assert_true (gi_type_info_is_pointer (&type_info_stack) == gi_type_info_is_pointer (type_info));
  g_assert_cmpint (gi_type_info_get_tag (&type_info_stack), ==, gi_type_info_get_tag (type_info));

  gi_base_info_clear (&type_info_stack);
  g_clear_pointer (&type_info, gi_base_info_unref);

  /* This method has no attributes */
  g_assert_false (gi_callable_info_iterate_return_attributes (callable_info, &iter, &name, &value));

  g_assert_null (gi_callable_info_get_return_attribute (callable_info, "doesnt-exist"));

  g_assert_false (gi_callable_info_get_caller_owns (callable_info));
  g_assert_true (gi_callable_info_may_return_null (callable_info));
  g_assert_false (gi_callable_info_skip_return (callable_info));

  g_assert_cmpuint (gi_callable_info_get_n_args (callable_info), ==, 1);

  arg_info = gi_callable_info_get_arg (callable_info, 0);
  g_assert_nonnull (arg_info);

  gi_callable_info_load_arg (callable_info, 0, &arg_info_stack);
  g_assert_cmpint (gi_arg_info_get_direction (&arg_info_stack), ==, gi_arg_info_get_direction (arg_info));
  g_assert_true (gi_arg_info_may_be_null (&arg_info_stack) == gi_arg_info_may_be_null (arg_info));

  gi_base_info_clear (&arg_info_stack);
  g_clear_pointer (&arg_info, gi_base_info_unref);

  g_assert_cmpint (gi_callable_info_get_instance_ownership_transfer (callable_info), ==, GI_TRANSFER_NOTHING);

  g_clear_pointer (&method_info, gi_base_info_unref);
  g_clear_pointer (&object_info, gi_base_info_unref);
}

static void
test_repository_callback_info (RepositoryFixture *fx,
                               const void *unused)
{
  GICallbackInfo *callback_info = NULL;

  g_test_summary ("Test retrieving GICallbackInfos from a typelib");

  /* Test all the methods of GICallbackInfo. This is simple, because there are none. */
  callback_info = GI_CALLBACK_INFO (gi_repository_find_by_name (fx->repository, "GObject", "ObjectFinalizeFunc"));
  g_assert_nonnull (callback_info);

  g_clear_pointer (&callback_info, gi_base_info_unref);
}

static void
test_repository_char_types (RepositoryFixture *fx,
                            const void *unused)
{
  GIStructInfo *gvalue_info;
  GIFunctionInfo *method_info;
  GITypeInfo *type_info;

  g_test_summary ("Test that signed and unsigned char GITypeInfo have GITypeTag of INT8 and UINT8 respectively");

  gvalue_info = GI_STRUCT_INFO (gi_repository_find_by_name (fx->repository, "GObject", "Value"));
  g_assert_nonnull (gvalue_info);

  /* unsigned char */
  method_info = gi_struct_info_find_method (gvalue_info, "get_uchar");
  g_assert_nonnull (method_info);

  type_info = gi_callable_info_get_return_type (GI_CALLABLE_INFO (method_info));
  g_assert_nonnull (type_info);
  g_assert_cmpuint (gi_type_info_get_tag (type_info), ==, GI_TYPE_TAG_UINT8);

  g_clear_pointer (&type_info, gi_base_info_unref);
  g_clear_pointer (&method_info, gi_base_info_unref);

  /* signed char */
  method_info = gi_struct_info_find_method (gvalue_info, "get_schar");
  g_assert_nonnull (method_info);

  type_info = gi_callable_info_get_return_type (GI_CALLABLE_INFO (method_info));
  g_assert_nonnull (type_info);
  g_assert_cmpuint (gi_type_info_get_tag (type_info), ==, GI_TYPE_TAG_INT8);

  g_clear_pointer (&type_info, gi_base_info_unref);
  g_clear_pointer (&method_info, gi_base_info_unref);
  g_clear_pointer (&gvalue_info, gi_base_info_unref);
}

static void
test_repository_constructor_return_type (RepositoryFixture *fx,
                                         const void *unused)
{
  GIObjectInfo *object_info = NULL;
  GIFunctionInfo *constructor = NULL;
  GITypeInfo *return_type = NULL;
  GIBaseInfo *return_info = NULL;
  const char *class_name = NULL;
  const char *return_name = NULL;

  g_test_summary ("Test the return type of a constructor, g_object_newv()");

  object_info = GI_OBJECT_INFO (gi_repository_find_by_name (fx->repository, "GObject", "Object"));
  g_assert_nonnull (object_info);

  class_name = gi_registered_type_info_get_type_name (GI_REGISTERED_TYPE_INFO (object_info));
  g_assert_nonnull (class_name);

  constructor = gi_object_info_find_method (object_info, "newv");
  g_assert_nonnull (constructor);

  return_type = gi_callable_info_get_return_type (GI_CALLABLE_INFO (constructor));
  g_assert_nonnull (return_type);
  g_assert_cmpuint (gi_type_info_get_tag (return_type), ==, GI_TYPE_TAG_INTERFACE);

  return_info = gi_type_info_get_interface (return_type);
  g_assert_nonnull (return_info);

  return_name = gi_registered_type_info_get_type_name (GI_REGISTERED_TYPE_INFO (return_info));
  g_assert_nonnull (return_name);
  g_assert_cmpstr (class_name, ==, return_name);

  g_clear_pointer (&return_info, gi_base_info_unref);
  g_clear_pointer (&return_type, gi_base_info_unref);
  g_clear_pointer (&constructor, gi_base_info_unref);
  g_clear_pointer (&object_info, gi_base_info_unref);
}

static void
test_repository_enum_info_c_identifier (RepositoryFixture *fx,
                                        const void *unused)
{
  GIBaseInfo *info = NULL;
  GIValueInfo *value_info = NULL;
  unsigned n_infos, n_values, ix, jx;
  const char *c_identifier = NULL;

  g_test_summary ("Test that every enum member has a C identifier");

  n_infos = gi_repository_get_n_infos (fx->repository, "GLib");

  for (ix = 0; ix < n_infos; ix++)
    {
      info = gi_repository_get_info (fx->repository, "GLib", ix);

      if (GI_IS_ENUM_INFO (info))
        {
          n_values = gi_enum_info_get_n_values (GI_ENUM_INFO (info));
          for (jx = 0; jx < n_values; jx++)
            {
              value_info = gi_enum_info_get_value (GI_ENUM_INFO (info), jx);
              c_identifier = gi_base_info_get_attribute (GI_BASE_INFO (value_info), "c:identifier");
              g_assert_nonnull (c_identifier);

              g_clear_pointer (&value_info, gi_base_info_unref);
            }
        }

      g_clear_pointer (&info, gi_base_info_unref);
    }
}

static void
test_repository_enum_info_static_methods (RepositoryFixture *fx,
                                          const void *unused)
{
  GIEnumInfo *enum_info = NULL;
  unsigned n_methods, ix;
  GIFunctionInfo *function_info = NULL;
  GIFunctionInfoFlags flags;
  const char *symbol = NULL;

  g_test_summary ("Test an enum with methods");

  enum_info = GI_ENUM_INFO (gi_repository_find_by_name (fx->repository, "GLib", "UnicodeScript"));
  g_assert_nonnull (enum_info);

  n_methods = gi_enum_info_get_n_methods (enum_info);
  g_assert_cmpuint (n_methods, >, 0);

  for (ix = 0; ix < n_methods; ix++)
    {
      function_info = gi_enum_info_get_method (enum_info, ix);
      g_assert_nonnull (function_info);

      flags = gi_function_info_get_flags (function_info);
      g_assert_false (flags & GI_FUNCTION_IS_METHOD); /* must be static */

      symbol = gi_function_info_get_symbol (function_info);
      g_assert_nonnull (symbol);
      g_assert_true (g_str_has_prefix (symbol, "g_unicode_script_"));

      g_clear_pointer (&function_info, gi_base_info_unref);
    }

  g_clear_pointer (&enum_info, gi_base_info_unref);
}

static void
test_repository_error_quark (RepositoryFixture *fx,
                             const void *unused)
{
  GIEnumInfo *error_info = NULL;

  g_test_summary ("Test finding an error quark by error domain");

  /* Find a simple error domain. */
  error_info = gi_repository_find_by_error_domain (fx->repository, G_RESOLVER_ERROR);
  g_assert_nonnull (error_info);
  g_assert_true (GI_IS_ENUM_INFO (error_info));
  g_assert_cmpstr (gi_base_info_get_name (GI_BASE_INFO (error_info)), ==, "ResolverError");

  g_clear_pointer (&error_info, gi_base_info_unref);

  /* Find again to check the caching. */
  error_info = gi_repository_find_by_error_domain (fx->repository, G_RESOLVER_ERROR);
  g_assert_nonnull (error_info);
  g_assert_true (GI_IS_ENUM_INFO (error_info));
  g_assert_cmpstr (gi_base_info_get_name (GI_BASE_INFO (error_info)), ==, "ResolverError");

  g_clear_pointer (&error_info, gi_base_info_unref);

  /* Try and find an unknown error domain. */
  g_assert_null (gi_repository_find_by_error_domain (fx->repository, GI_REPOSITORY_ERROR));

  /* And check caching for unknown error domains. */
  g_assert_null (gi_repository_find_by_error_domain (fx->repository, GI_REPOSITORY_ERROR));

  /* It would be good to try and find one which will resolve in both Gio and
   * GioUnix/GioWin32, but neither of the platform-specific GIRs actually define
   * any error domains at the moment. */
}

static void
test_repository_flags_info_c_identifier (RepositoryFixture *fx,
                                         const void *unused)
{
  GIBaseInfo *info = NULL;
  GIValueInfo *value_info = NULL;
  unsigned n_infos, n_values, ix, jx;
  const char *c_identifier = NULL;

  g_test_summary ("Test that every flags member has a C identifier");

  n_infos = gi_repository_get_n_infos (fx->repository, "GLib");

  for (ix = 0; ix < n_infos; ix++)
    {
      info = gi_repository_get_info (fx->repository, "GLib", ix);

      if (GI_IS_FLAGS_INFO (info))
        {
          n_values = gi_enum_info_get_n_values (GI_ENUM_INFO (info));
          for (jx = 0; jx < n_values; jx++)
            {
              value_info = gi_enum_info_get_value (GI_ENUM_INFO (info), jx);
              c_identifier = gi_base_info_get_attribute (GI_BASE_INFO (value_info), "c:identifier");
              g_assert_nonnull (c_identifier);

              g_clear_pointer (&value_info, gi_base_info_unref);
            }
        }

      g_clear_pointer (&info, gi_base_info_unref);
    }
}

static void
test_repository_fundamental_ref_func (RepositoryFixture *fx,
                                      const void *unused)
{
  GIObjectInfo *info;

  g_test_summary ("Test getting the ref func of a fundamental type");

  info = GI_OBJECT_INFO (gi_repository_find_by_name (fx->repository, "GObject", "ParamSpec"));
  g_assert_nonnull (info);

  g_assert_nonnull (gi_object_info_get_ref_function_pointer (info));

  g_clear_pointer (&info, gi_base_info_unref);
}

static void
test_repository_instance_method_ownership_transfer (RepositoryFixture *fx,
                                                    const void *unused)
{
  GIObjectInfo *class_info = NULL;
  GIFunctionInfo *func_info = NULL;
  GITransfer transfer;

  g_test_summary ("Test two methods of the same object having opposite ownership transfer of the instance parameter");

  class_info = GI_OBJECT_INFO (gi_repository_find_by_name (fx->repository, "Gio", "DBusMethodInvocation"));
  g_assert_nonnull (class_info);

  func_info = gi_object_info_find_method (class_info, "get_connection");
  g_assert_nonnull (func_info);
  transfer = gi_callable_info_get_instance_ownership_transfer (GI_CALLABLE_INFO (func_info));
  g_assert_cmpint (GI_TRANSFER_NOTHING, ==, transfer);

  g_clear_pointer (&func_info, gi_base_info_unref);

  func_info = gi_object_info_find_method (class_info, "return_error_literal");
  g_assert_nonnull (func_info);
  transfer = gi_callable_info_get_instance_ownership_transfer (GI_CALLABLE_INFO (func_info));
  g_assert_cmpint (GI_TRANSFER_EVERYTHING, ==, transfer);

  g_clear_pointer (&func_info, gi_base_info_unref);
  g_clear_pointer (&class_info, gi_base_info_unref);
}

static void
test_repository_object_gtype_interfaces (RepositoryFixture *fx,
                                         const void *unused)
{
  GIInterfaceInfo **interfaces;
  size_t n_interfaces, ix;
  const char *name;
  gboolean found_initable = FALSE, found_async_initable = FALSE;

  g_test_summary ("Test gi_repository_get_object_gtype_interfaces()");

  gi_repository_get_object_gtype_interfaces (fx->repository, G_TYPE_DBUS_CONNECTION, &n_interfaces, &interfaces);

  g_assert_cmpuint (n_interfaces, ==, 2);

  for (ix = 0; ix < n_interfaces; ix++)
    {
      name = gi_base_info_get_name (GI_BASE_INFO (*(interfaces + ix)));
      if (strcmp (name, "Initable") == 0)
        found_initable = TRUE;
      else if (strcmp (name, "AsyncInitable") == 0)
        found_async_initable = TRUE;
    }

  g_assert_true (found_initable);
  g_assert_true (found_async_initable);
}

static void
test_repository_signal_info_with_array_length_arg (RepositoryFixture *fx,
                                                   const void *unused)
{
  GIObjectInfo *gsettings_info = NULL;
  GISignalInfo *sig_info = NULL;
  GIArgInfo *arg_info = NULL;
  GITypeInfo *type_info = NULL;
  unsigned length_ix;

  g_test_summary ("Test finding the associated array length argument of an array parameter of a signal");

  gsettings_info = GI_OBJECT_INFO (gi_repository_find_by_name (fx->repository, "Gio", "Settings"));
  g_assert_nonnull (gsettings_info);

  sig_info = gi_object_info_find_signal (gsettings_info, "change-event");
  g_assert_nonnull (sig_info);

  g_assert_cmpuint (gi_callable_info_get_n_args (GI_CALLABLE_INFO (sig_info)), ==, 2);

  /* verify array argument */
  arg_info = gi_callable_info_get_arg (GI_CALLABLE_INFO (sig_info), 0);
  g_assert_nonnull (arg_info);
  g_assert_cmpstr (gi_base_info_get_name (GI_BASE_INFO (arg_info)), ==, "keys");

  type_info = gi_arg_info_get_type_info (arg_info);
  g_assert_nonnull (type_info);
  g_assert_cmpint (gi_type_info_get_tag (type_info), ==, GI_TYPE_TAG_ARRAY);
  g_assert_cmpint (gi_type_info_get_array_type (type_info), ==, GI_ARRAY_TYPE_C);
  g_assert_false (gi_type_info_is_zero_terminated (type_info));
  gboolean ok = gi_type_info_get_array_length_index (type_info, &length_ix);
  g_assert_true (ok);
  g_assert_cmpuint (length_ix, ==, 1);

  g_clear_pointer (&arg_info, gi_base_info_unref);
  g_clear_pointer (&type_info, gi_base_info_unref);

  /* verify array length argument */
  arg_info = gi_callable_info_get_arg (GI_CALLABLE_INFO (sig_info), 1);
  g_assert_nonnull (arg_info);
  g_assert_cmpstr (gi_base_info_get_name (GI_BASE_INFO (arg_info)), ==, "n_keys");

  g_clear_pointer (&arg_info, gi_base_info_unref);
  g_clear_pointer (&type_info, gi_base_info_unref);
  g_clear_pointer (&sig_info, gi_base_info_unref);
  g_clear_pointer (&gsettings_info, gi_base_info_unref);
}

static void
test_repository_type_info_name (RepositoryFixture *fx,
                                const void *unused)
{
  GIInterfaceInfo *interface_info = NULL;
  GIVFuncInfo *vfunc;
  GITypeInfo *typeinfo;

  g_test_summary ("Test that gi_base_info_get_name() returns null for GITypeInfo");
  g_test_bug ("https://gitlab.gnome.org/GNOME/gobject-introspection/issues/96");

  interface_info = GI_INTERFACE_INFO (gi_repository_find_by_name (fx->repository, "Gio", "File"));
  g_assert_nonnull (interface_info);
  vfunc = gi_interface_info_find_vfunc (interface_info, "read_async");
  g_assert_nonnull (vfunc);

  typeinfo = gi_callable_info_get_return_type (GI_CALLABLE_INFO (vfunc));
  g_assert_nonnull (typeinfo);

  g_assert_null (gi_base_info_get_name (GI_BASE_INFO (typeinfo)));

  g_clear_pointer (&interface_info, gi_base_info_unref);
  g_clear_pointer (&vfunc, gi_base_info_unref);
  g_clear_pointer (&typeinfo, gi_base_info_unref);
}

static void
test_repository_vfunc_info_with_no_invoker (RepositoryFixture *fx,
                                            const void *unused)
{
  GIObjectInfo *object_info = NULL;
  GIVFuncInfo *vfunc_info = NULL;
  GIFunctionInfo *invoker_info = NULL;

  g_test_summary ("Test vfunc with no known invoker on object, such as GObject.dispose");

  object_info = GI_OBJECT_INFO (gi_repository_find_by_name (fx->repository, "GObject", "Object"));
  g_assert_nonnull (object_info);

  vfunc_info = gi_object_info_find_vfunc (object_info, "dispose");
  g_assert_nonnull (vfunc_info);

  invoker_info = gi_vfunc_info_get_invoker (vfunc_info);
  g_assert_null (invoker_info);

  g_clear_pointer (&object_info, gi_base_info_unref);
  g_clear_pointer (&vfunc_info, gi_base_info_unref);
}

static void
test_repository_vfunc_info_with_invoker_on_interface (RepositoryFixture *fx,
                                                      const void *unused)
{
  GIInterfaceInfo *interface_info = NULL;
  GIVFuncInfo *vfunc_info = NULL;
  GIFunctionInfo *invoker_info = NULL;

  g_test_summary ("Test vfunc with invoker on interface, such as GFile.read_async");

  interface_info = GI_INTERFACE_INFO (gi_repository_find_by_name (fx->repository, "Gio", "File"));
  g_assert_nonnull (interface_info);

  vfunc_info = gi_interface_info_find_vfunc (interface_info, "read_async");
  g_assert_nonnull (vfunc_info);

  invoker_info = gi_vfunc_info_get_invoker (vfunc_info);
  g_assert_nonnull (invoker_info);

  g_assert_cmpstr (gi_base_info_get_name (GI_BASE_INFO (invoker_info)), ==, "read_async");

  g_clear_pointer (&interface_info, gi_base_info_unref);
  g_clear_pointer (&vfunc_info, gi_base_info_unref);
  g_clear_pointer (&invoker_info, gi_base_info_unref);
}

static void
test_repository_vfunc_info_with_invoker_on_object (RepositoryFixture *fx,
                                                   const void *unused)
{
  GIObjectInfo *object_info = NULL;
  GIVFuncInfo *vfunc_info = NULL;
  GIFunctionInfo *invoker_info = NULL;

  g_test_summary ("Test vfunc with invoker on object, such as GAppLaunchContext.get_display");

  object_info = GI_OBJECT_INFO (gi_repository_find_by_name (fx->repository, "Gio", "AppLaunchContext"));
  g_assert_nonnull (object_info);

  vfunc_info = gi_object_info_find_vfunc (object_info, "get_display");
  g_assert_nonnull (vfunc_info);

  invoker_info = gi_vfunc_info_get_invoker (vfunc_info);
  g_assert_nonnull (invoker_info);
  g_assert_cmpstr (gi_base_info_get_name (GI_BASE_INFO (invoker_info)), ==, "get_display");

  /* And let's be sure we can find the method directly */
  g_clear_pointer (&invoker_info, gi_base_info_unref);

  invoker_info = gi_object_info_find_method (object_info, "get_display");
  g_assert_nonnull (invoker_info);
  g_assert_cmpstr (gi_base_info_get_name (GI_BASE_INFO (invoker_info)), ==, "get_display");

  g_clear_pointer (&object_info, gi_base_info_unref);
  g_clear_pointer (&vfunc_info, gi_base_info_unref);
  g_clear_pointer (&invoker_info, gi_base_info_unref);
}

static void
test_repository_find_by_gtype (RepositoryFixture *fx,
                               const void        *unused)
{
  GIObjectInfo *object_info = NULL;

  g_test_summary ("Test finding a GType");

  object_info = (GIObjectInfo *) gi_repository_find_by_gtype (fx->repository, G_TYPE_OBJECT);
  g_assert_nonnull (object_info);
  g_assert_true (GI_IS_OBJECT_INFO (object_info));
  g_assert_cmpstr (gi_base_info_get_name (GI_BASE_INFO (object_info)), ==, "Object");

  g_clear_pointer (&object_info, gi_base_info_unref);

  /* Find it again; this time it should hit the cache. */
  object_info = (GIObjectInfo *) gi_repository_find_by_gtype (fx->repository, G_TYPE_OBJECT);
  g_assert_nonnull (object_info);
  g_assert_true (GI_IS_OBJECT_INFO (object_info));
  g_assert_cmpstr (gi_base_info_get_name (GI_BASE_INFO (object_info)), ==, "Object");

  g_clear_pointer (&object_info, gi_base_info_unref);

  /* Try and find an unknown GType. */
  g_assert_null (gi_repository_find_by_gtype (fx->repository, GI_TYPE_BASE_INFO));

  /* And check caching for unknown GTypes. */
  g_assert_null (gi_repository_find_by_gtype (fx->repository, GI_TYPE_BASE_INFO));

  /* Now try and find one which will resolve in both Gio and GioUnix/GioWin32.
   * The longest-named typelib should be returned. */
  {
    GType platform_specific_type;
    const char *expected_name, *expected_namespace;

#if defined(G_OS_UNIX) && !(__APPLE__)
    platform_specific_type = G_TYPE_DESKTOP_APP_INFO;
    expected_name = "DesktopAppInfo";
    expected_namespace = "GioUnix";
#elif defined(G_OS_WIN32)
    platform_specific_type = G_TYPE_WIN32_INPUT_STREAM;
    expected_name = "InputStream";
    expected_namespace = "GioWin32";
#else
    platform_specific_type = G_TYPE_INVALID;
    expected_name = NULL;
    expected_namespace = NULL;
#endif

    if (expected_name != NULL)
      {
        object_info = (GIObjectInfo *) gi_repository_find_by_gtype (fx->repository, platform_specific_type);
        g_assert_nonnull (object_info);
        g_assert_true (GI_IS_OBJECT_INFO (object_info));
        g_assert_cmpstr (gi_base_info_get_name (GI_BASE_INFO (object_info)), ==, expected_name);
        g_assert_cmpstr (gi_base_info_get_namespace (GI_BASE_INFO (object_info)), ==, expected_namespace);
        g_clear_pointer (&object_info, gi_base_info_unref);
      }
  }
}

static void
test_repository_loaded_namespaces (RepositoryFixture *fx,
                                   const void        *unused)
{
  char **namespaces;
  size_t n_namespaces;

  /* These should be in alphabetical order */
#if defined(G_OS_UNIX)
  const char *expected_namespaces[] = { "GLib", "GModule", "GObject", "Gio", "GioUnix", NULL };
#elif defined(G_OS_WIN32)
  const char *expected_namespaces[] = { "GLib", "GModule", "GObject", "Gio", "GioWin32", NULL };
#else
  const char *expected_namespaces[] = { "GLib", "GModule", "GObject", "Gio", NULL };
#endif

  g_test_summary ("Test listing loaded namespaces");

  namespaces = gi_repository_get_loaded_namespaces (fx->repository, &n_namespaces);
  g_assert_cmpstrv (namespaces, expected_namespaces);
  g_assert_cmpuint (n_namespaces, ==, g_strv_length ((char **) expected_namespaces));
  g_strfreev (namespaces);

  /* Test again but without passing `n_namespaces`. */
  namespaces = gi_repository_get_loaded_namespaces (fx->repository, NULL);
  g_assert_cmpstrv (namespaces, expected_namespaces);
  g_strfreev (namespaces);
}

static void
test_repository_dup_default (void)
{
  GIRepository *repository1 = gi_repository_dup_default ();
  GIRepository *repository2 = gi_repository_dup_default ();

  g_assert_nonnull (repository1);
  g_assert_nonnull (repository2);
  g_assert_true (repository1 == repository2);

  g_clear_object (&repository1);
  g_clear_object (&repository2);
}

int
main (int   argc,
      char *argv[])
{
  repository_init (&argc, &argv);

  ADD_REPOSITORY_TEST ("/repository/basic", test_repository_basic, &typelib_load_spec_glib);
  ADD_REPOSITORY_TEST ("/repository/info", test_repository_info, &typelib_load_spec_gobject);
  ADD_REPOSITORY_TEST ("/repository/dependencies", test_repository_dependencies, &typelib_load_spec_gobject);
  ADD_REPOSITORY_TEST ("/repository/base-info/clear", test_repository_base_info_clear, &typelib_load_spec_gobject);
  ADD_REPOSITORY_TEST ("/repository/arg-info", test_repository_arg_info, &typelib_load_spec_gobject);
  ADD_REPOSITORY_TEST ("/repository/callable-info", test_repository_callable_info, &typelib_load_spec_gobject);
  ADD_REPOSITORY_TEST ("/repository/callback-info", test_repository_callback_info, &typelib_load_spec_gobject);
  ADD_REPOSITORY_TEST ("/repository/char-types", test_repository_char_types, &typelib_load_spec_gobject);
  ADD_REPOSITORY_TEST ("/repository/constructor-return-type", test_repository_constructor_return_type, &typelib_load_spec_gobject);
  ADD_REPOSITORY_TEST ("/repository/enum-info-c-identifier", test_repository_enum_info_c_identifier, &typelib_load_spec_glib);
  ADD_REPOSITORY_TEST ("/repository/enum-info-static-methods", test_repository_enum_info_static_methods, &typelib_load_spec_glib);
  ADD_REPOSITORY_TEST ("/repository/error-quark", test_repository_error_quark, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/repository/flags-info-c-identifier", test_repository_flags_info_c_identifier, &typelib_load_spec_gobject);
  ADD_REPOSITORY_TEST ("/repository/fundamental-ref-func", test_repository_fundamental_ref_func, &typelib_load_spec_gobject);
  ADD_REPOSITORY_TEST ("/repository/instance-method-ownership-transfer", test_repository_instance_method_ownership_transfer, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/repository/object-gtype-interfaces", test_repository_object_gtype_interfaces, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/repository/signal-info-with-array-length-arg", test_repository_signal_info_with_array_length_arg, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/repository/type-info-name", test_repository_type_info_name, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/repository/vfunc-info-with-no-invoker", test_repository_vfunc_info_with_no_invoker, &typelib_load_spec_gobject);
  ADD_REPOSITORY_TEST ("/repository/vfunc-info-with-invoker-on-interface", test_repository_vfunc_info_with_invoker_on_interface, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/repository/vfunc-info-with-invoker-on-object", test_repository_vfunc_info_with_invoker_on_object, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/repository/find-by-gtype", test_repository_find_by_gtype, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/repository/loaded-namespaces", test_repository_loaded_namespaces, &typelib_load_spec_gio);
  g_test_add_func ("/repository/dup_default", test_repository_dup_default);

  return g_test_run ();
}
