Title: Value Collection
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2005 Matthias Clasen

# Value Collection

GLib provides a set of macros for the varargs parsing support needed
in variadic GObject functions such as [ctor@GObject.Object.new] or
[method@GObject.Object.set]

They currently support the collection of integral types, floating point 
types and pointers.

## Macros

`G_VALUE_COLLECT_INIT(value, _value_type, var_args, flags, __error)`

:   Collects a variable argument value from a `va_list`.

    We have to implement the varargs collection as a macro, because on some
    systems `va_list` variables cannot be passed by reference.

    Since: 2.24

`G_VALUE_COLLECT_INIT2(value, g_vci_vtab, _value_type, var_args, flags, __error)`

:   A variant of `G_VALUE_COLLECT_INIT` that provides the [struct@GObject.TypeValueTable]
    to the caller.

    Since: 2.74

`G_VALUE_COLLECT(value, var_args, flags, __error)`

:   Collects a variable argument value from a `va_list`.

    We have to implement the varargs collection as a macro, because on some systems
    `va_list` variables cannot be passed by reference.

    Note: If you are creating the `value argument` just before calling this macro,
    you should use the `G_VALUE_COLLECT_INIT` variant and pass the uninitialized
    `GValue`. That variant is faster than `G_VALUE_COLLECT`.

`G_VALUE_COLLECT_SKIP(_value_type, var_args)`

:   Skip an argument of type `_value_type` from `var_args`.

`G_VALUE_LCOPY(value, var_args, flags, __error)`

:   Stores a value’s value into one or more argument locations from a `va_list`.

    This is the inverse of G_VALUE_COLLECT().

`G_VALUE_COLLECT_FORMAT_MAX_LENGTH`

:   The maximal number of [type@GObject.TypeCValue]s which can be collected for a 
    single `GValue`.
