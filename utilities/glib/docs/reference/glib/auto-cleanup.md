Title: Automatic Cleanup
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2023 Matthias Clasen

# Automatic Cleanup

GLib provides a set of macros that wrap the GCC extension for automatic
cleanup of variables when they go out of scope.

These macros can only be used with GCC and GCC-compatible C compilers.

## Variable declaration

`g_auto(TypeName)`

:   Helper to declare a variable with automatic cleanup.

    The variable is cleaned up in a way appropriate to its type when the
    variable goes out of scope. The `TypeName` of the variable must support
    this.

    The way to clean up the type must have been defined using one of the macros
    `G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC()` or `G_DEFINE_AUTO_CLEANUP_FREE_FUNC()`.

    This feature is only supported on GCC and clang.  This macro is not
    defined on other compilers and should not be used in programs that
    are intended to be portable to those compilers.

    This macro meant to be used with stack-allocated structures and
    non-pointer types.  For the (more commonly used) pointer version, see
    `g_autoptr()`.

    This macro can be used to avoid having to do explicit cleanups of
    local variables when exiting functions.  It often vastly simplifies
    handling of error conditions, removing the need for various tricks
    such as `goto out` or repeating of cleanup code.  It is also helpful
    for non-error cases.

    Consider the following example:

        GVariant *
        my_func(void)
        {
          g_auto(GQueue) queue = G_QUEUE_INIT;
          g_auto(GVariantBuilder) builder;
          g_auto(GStrv) strv;

          g_variant_builder_init (&builder, G_VARIANT_TYPE_VARDICT);
          strv = g_strsplit("a:b:c", ":", -1);

          // ...

          if (error_condition)
            return NULL;

          // ...

          return g_variant_builder_end (&builder);
        }

    You must initialize the variable in some way — either by use of an
    initialiser or by ensuring that an `_init` function will be called on
    it unconditionally before it goes out of scope.

    Since: 2.44


`g_autoptr(TypeName)`
:   Helper to declare a pointer variable with automatic cleanup.

    The variable is cleaned up in a way appropriate to its type when the
    variable goes out of scope. The `TypeName` of the variable must support this.
    The way to clean up the type must have been defined using the macro
    `G_DEFINE_AUTOPTR_CLEANUP_FUNC()`.

    This feature is only supported on GCC and clang.  This macro is not
    defined on other compilers and should not be used in programs that
    are intended to be portable to those compilers.

    This is meant to be used to declare pointers to types with cleanup
    functions.  The type of the variable is a pointer to `TypeName`.  You
    must not add your own `*`.

    This macro can be used to avoid having to do explicit cleanups of
    local variables when exiting functions.  It often vastly simplifies
    handling of error conditions, removing the need for various tricks
    such as `goto out` or repeating of cleanup code.  It is also helpful
    for non-error cases.

    Consider the following example:

        gboolean
        check_exists(GVariant *dict)
        {
          g_autoptr(GVariant) dirname, basename = NULL;
          g_autofree gchar *path = NULL;

          dirname = g_variant_lookup_value (dict, "dirname", G_VARIANT_TYPE_STRING);
          if (dirname == NULL)
            return FALSE;

          basename = g_variant_lookup_value (dict, "basename", G_VARIANT_TYPE_STRING);
          if (basename == NULL)
            return FALSE;

          path = g_build_filename (g_variant_get_string (dirname, NULL),
                                   g_variant_get_string (basename, NULL),
                                   NULL);

          return g_access (path, R_OK) == 0;
        }

    You must initialise the variable in some way — either by use of an
    initialiser or by ensuring that it is assigned to unconditionally
    before it goes out of scope.

    See also: `g_auto()`, `g_autofree()` and `g_steal_pointer()`.

    Since: 2.44


`g_autofree`

:   Macro to add an attribute to pointer variable to ensure automatic
    cleanup using `g_free()`.

    This macro differs from `g_autoptr()` in that it is an attribute supplied
    before the type name, rather than wrapping the type definition.  Instead
    of using a type-specific lookup, this macro always calls `g_free()` directly.

    This means it's useful for any type that is returned from `g_malloc()`.

    Otherwise, this macro has similar constraints as `g_autoptr()`: only
    supported on GCC and clang, the variable must be initialized, etc.

        gboolean
        operate_on_malloc_buf (void)
        {
          g_autofree guint8* membuf = NULL;

          membuf = g_malloc (8192);

          // Some computation on membuf

          // membuf will be automatically freed here
          return TRUE;
        }

    Since: 2.44


`g_autolist(TypeName)`

:   Helper to declare a list variable with automatic deep cleanup.

    The list is deeply freed, in a way appropriate to the specified type, when the
    variable goes out of scope.  The type must support this.

    This feature is only supported on GCC and clang.  This macro is not
    defined on other compilers and should not be used in programs that
    are intended to be portable to those compilers.

    This is meant to be used to declare lists of a type with a cleanup
    function.  The type of the variable is a `GList *`.  You
    must not add your own `*`.

    This macro can be used to avoid having to do explicit cleanups of
    local variables when exiting functions.  It often vastly simplifies
    handling of error conditions, removing the need for various tricks
    such as `goto out` or repeating of cleanup code.  It is also helpful
    for non-error cases.

    See also: `g_autoslist()`, `g_autoptr()` and `g_steal_pointer()`.

    Since: 2.56


`g_autoslist(TypeName)`

:   Helper to declare a singly linked list variable with automatic deep cleanup.

    The list is deeply freed, in a way appropriate to the specified type, when the
    variable goes out of scope.  The type must support this.

    This feature is only supported on GCC and clang.  This macro is not
    defined on other compilers and should not be used in programs that
    are intended to be portable to those compilers.

    This is meant to be used to declare lists of a type with a cleanup
    function.  The type of the variable is a `GSList *`.  You
    must not add your own `*`.

    This macro can be used to avoid having to do explicit cleanups of
    local variables when exiting functions.  It often vastly simplifies
    handling of error conditions, removing the need for various tricks
    such as `goto out` or repeating of cleanup code.  It is also helpful
    for non-error cases.

    See also: `g_autolist()`, `g_autoptr()` and `g_steal_pointer()`.

    Since: 2.56


`g_autoqueue(TypeName)`

:   Helper to declare a double-ended queue variable with automatic deep cleanup.

    The queue is deeply freed, in a way appropriate to the specified type, when the
    variable goes out of scope.  The type must support this.

    This feature is only supported on GCC and clang.  This macro is not
    defined on other compilers and should not be used in programs that
    are intended to be portable to those compilers.

    This is meant to be used to declare queues of a type with a cleanup
    function.  The type of the variable is a `GQueue *`.  You
    must not add your own `*`.

    This macro can be used to avoid having to do explicit cleanups of
    local variables when exiting functions.  It often vastly simplifies
    handling of error conditions, removing the need for various tricks
    such as `goto out` or repeating of cleanup code.  It is also helpful
    for non-error cases.

    See also: `g_autolist()`, `g_autoptr()` and `g_steal_pointer()`.

    Since: 2.62


## Type definition

`G_DEFINE_AUTOPTR_CLEANUP_FUNC(TypeName, func)`

:   Defines the appropriate cleanup function for a pointer type.

    The function will not be called if the variable to be cleaned up
    contains `NULL`.

    This will typically be the `_free()` or `_unref()` function for the given
    type.

    With this definition, it will be possible to use `g_autoptr()` with
    the given `TypeName`.

        G_DEFINE_AUTOPTR_CLEANUP_FUNC(GObject, g_object_unref)

    This macro should be used unconditionally; it is a no-op on compilers
    where cleanup is not supported.

    Since: 2.44


`G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(TypeName, func)`

:   Defines the appropriate cleanup function for a type.

    This will typically be the `_clear()` function for the given type.

    With this definition, it will be possible to use `g_auto()` with
    the given `TypeName`.

        G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(GQueue, g_queue_clear)

    This macro should be used unconditionally; it is a no-op on compilers
    where cleanup is not supported.

    Since: 2.44


`G_DEFINE_AUTO_CLEANUP_FREE_FUNC(TypeName, func, none_value)`

:   Defines the appropriate cleanup function for a type.

    With this definition, it will be possible to use `g_auto()` with the
    given `TypeName`.

    This function will be rarely used.  It is used with pointer-based
    typedefs and non-pointer types where the value of the variable
    represents a resource that must be freed.  Two examples are `GStrv`
    and file descriptors.

    `none_value` specifies the "none" value for the type in question. It
    is probably something like `NULL` or `-1`.If the variable is found to
    contain this value then the free function will not be called.

        G_DEFINE_AUTO_CLEANUP_FREE_FUNC(GStrv, g_strfreev, NULL)

    This macro should be used unconditionally; it is a no-op on compilers
    where cleanup is not supported.

    Since: 2.44
