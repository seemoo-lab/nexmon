Title: Migrating from girepository-1.0 to girepository-2.0
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2014 GNOME Foundation

# Migrating from girepository-1.0 to girepository-2.0

libgirepository was originally part of
[gobject-introspection](https://gitlab.gnome.org/GNOME/gobject-introspection/),
where it was prototyped for a number of years.

Now that it’s considered stable (for a long time), it’s been moved to
[glib](https://gitlab.gnome.org/GNOME/glib/) in order to simplify the build
process of the two modules.

As part of this move, the API version has been bumped from 1.0 to 2.0, and some
fairly straightforward API changes have been made. Note that the GIR version
was previously 2.0, and is now 3.0 — so the new version of libgirepository uses
`girepository-2.0.pc` and `GIRepository-3.0.typelib`.

The main change between the two versions of libgirepository is that it now uses
[type@GObject.TypeInstance] as the basis of its type system, rather than simple
C struct aliasing. This means that [type@GIRepository.BaseInfo] instances are
now reference counted using [method@GIRepository.BaseInfo.ref] and
[method@GIRepository.BaseInfo.unref].

It also means that runtime cast macros such as [func@GIRepository.CALLABLE_INFO]
are now available. Using these instead of simple C casts can improve the runtime
type safety of your code.

Stack allocation of some [type@GIRepository.BaseInfo] subtypes is still
possible, but they must now be cleared using
[method@GIRepository.BaseInfo.clear] once finished with. Previously this wasn’t
necessary.

As part of moving the code over, the symbol prefix has changed from `g_` to
`gi_` — this has affected every API in the library, but trivially.

The types of various function arguments have changed — for example from
`guint32` to `size_t` for most offsets. This will require minor adjustments in
your code if integer type warnings are enabled.

## API replacements from version 1.0 to 2.0

| girepository-1.0 | girepository-2.0 |
|------------------|------------------|
| `GI_CHECK_VERSION` | [func@GLib.CHECK_VERSION] |
| `g_arg_info_get_closure` | [method@GIRepository.ArgInfo.get_closure_index] |
| `g_arg_info_get_destroy` | [method@GIRepository.ArgInfo.get_destroy_index] |
| `g_arg_info_get_type` | [method@GIRepository.ArgInfo.get_type_info] |
| `g_arg_info_load_type` | [method@GIRepository.ArgInfo.load_type_info] |
| - | [method@GIRepository.BaseInfo.ref] and [method@GIRepository.BaseInfo.unref] |
| `g_base_info_get_type` | Use type checking macros like [func@GIRepository.IS_OBJECT_INFO], or raw [type@GObject.Type]s with [func@GObject.TYPE_FROM_INSTANCE] |
| `g_info_new` | Removed with no replacement, use [method@GIRepository.find_by_name] and related APIs |
| `g_callable_info_invoke` arguments | `is_method` and `throws` dropped in [method@GIRepository.CallableInfo.invoke] |
| `g_constant_info_get_type` | [method@GIRepository.ConstantInfo.get_type_info] |
| `g_field_info_get_type` | [method@GIRepository.FieldInfo.get_type_info] |
| `g_object_info_find_method_using_interfaces` and `g_object_info_find_vfunc_using_interfaces` | The `implementor` out argument has been renamed to `declarer` and is now of type [type@GIRepository.BaseInfo] |
| `g_object_info_get_type_init` | [method@GIRepository.ObjectInfo.get_type_init_function_name] |
| `g_object_info_get_ref_function` | [method@GIRepository.ObjectInfo.get_ref_function_name] |
| `g_object_info_get_unref_function` | [method@GIRepository.ObjectInfo.get_unref_function_name] |
| `g_object_info_get_set_value_function` | [method@GIRepository.ObjectInfo.get_set_value_function_name] |
| `g_object_info_get_get_value_function` | [method@GIRepository.ObjectInfo.get_get_value_function_name] |
| `g_property_info_get_type` | [method@GIRepository.PropertyInfo.get_type_info] |
| `g_registered_type_info_get_type_init` | [method@GIRepository.RegisteredTypeInfo.get_type_init_function_name] |
| `g_irepository_*` | `gi_repository_*` |
| `g_irepository_get_default` | Singleton object removed; create separate [class@GIRepository.Repository] instances instead |
| `g_irepository_get_search_path` and `g_irepository_get_library_path` | Now return arrays rather than linked lists |
| `g_irepository_enumerate_versions` | Now returns an array rather than a linked list |
| `g_irepository_get_immediate_dependencies`, `g_irepository_get_dependencies` and `g_irepository_get_loaded_namespaces` | Now additionally return a length argument |
| `g_irepository_get_shared_library` | [method@GIRepository.get_shared_libraries] |
| `g_irepository_dump` | Takes structured `input_filename` and `output_filename` arguments rather than a single formatted string |
| `g_function_invoker_destroy` | `gi_function_invoker_clear()` |
| `g_struct_info_get_copy_function` | [method@GIRepository.StructInfo.get_copy_function_name] |
| `g_struct_info_get_free_function` | [method@GIRepository.StructInfo.get_free_function_name] |
| `g_type_info_get_array_length` and `g_type_info_get_array_fixed_size` | Split success and failure return values out into a new out-argument and return value |
| `g_type_info_get_array_length` | [method@GIRepository.TypeInfo.get_array_length_index] |
| `g_typelib_new_from_*` | All replaced with `gi_typelib_new_from_bytes()` |
| `g_typelib_free` | [type@GIRepository.Typelib] is now a refcounted and boxed type, so use [method@GIRepository.Typelib.unref] |
| `GI_FUNCTION_THROWS` and `GI_VFUNC_THROWS` | [method@GIRepository.CallableInfo.can_throw_gerror] |
| `g_union_info_get_discriminator_offset` | Split success and failure return values out into a new out-argument and return value |
| `g_union_info_get_copy_function` | [method@GIRepository.UnionInfo.get_copy_function_name] |
| `g_union_info_get_free_function` | [method@GIRepository.UnionInfo.get_free_function_name] |
| `GIInfoType` | Use [type@GObject.Type] directly |
| `GI_INFO_TYPE_BOXED` | Dropped in favour of [method@GIRepository.RegisteredTypeInfo.is_boxed] |

## Utility program renames from version 1.0 to 2.0

| girepository-1.0 | girepository-2.0        |
|------------------|-------------------------|
| `g-ir-compiler`  | `gi-compile-repository` |
| `g-ir-generate`  | `gi-decompile-typelib`  |
| `g-ir-inspect`   | `gi-inspect-typelib`    |

In addition, some command-line options have been changed.

The `--version` option for `g-ir-inspect` has been renamed to
`--typelib-version` in `gi-inspect-typelib`.

The `--includedir` option to `gi-decompile-typelib` treats the
given directories as most-important-first, consistent with
`gi-compile-repository --includedir` and `gcc -I`.
`g-ir-generate` treated `--includedir` options as least-important-first.

The unimplemented `g-ir-compiler --module` option has been removed in
`gi-compile-repository`.

The unimplemented `g-ir-generate --shlib` option has been removed in
`gi-decompile-typelib`.

`gi-inspect-typelib` only accepts one namespace parameter. `g-ir-inspect`
accepted multiple namespaces, but would only inspect the first one,
with the others being ignored.
