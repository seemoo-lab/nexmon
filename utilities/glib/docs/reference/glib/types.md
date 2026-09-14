Title: Basic Types
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 1999 Owen Taylor
SPDX-FileCopyrightText: 2000 Red Hat, Inc.
SPDX-FileCopyrightText: 2000 Sebastian Wilhelmi
SPDX-FileCopyrightText: 2008 Matthias Clasen
SPDX-FileCopyrightText: 2008, 2010 Behdad Esfahbod
SPDX-FileCopyrightText: 2009 Christian Persch
SPDX-FileCopyrightText: 2014, 2022 Collabora, Ltd.
SPDX-FileCopyrightText: 2017, 2018 Endless Mobile, Inc.
SPDX-FileCopyrightText: 2018 Christoph Reiter
SPDX-FileCopyrightText: 2019 Alistair Thomas

# Basic Types

GLib defines a number of commonly used types, which can be divided
into several groups:

 - New types which are not part of standard C (but are defined in
   various C standard library header files) — [`gboolean`](#gboolean),
   [`gssize`](#gssize).
 - Integer types which are guaranteed to be the same size across
   all platforms — [`gint8`](#gint8), [`guint8`](#guint8), [`gint16`](#gint16),
   [`guint16`](#guint16), [`gint32`](#gint32), [`guint32`](#guint32),
   [`gint64`](#gint64), [`guint64`](#guint64).
 - Types which are easier to use than their standard C counterparts —
   [`gpointer`](#gpointer), [`gconstpointer`](#gconstpointer),
   [`guchar`](#guchar), [`guint`](#guint), [`gushort`](#gushort),
   [`gulong`](#gulong).
 - Types which correspond exactly to standard C types, but are
   included for completeness — [`gchar`](#gchar), [`gint`](#gint),
   [`gshort`](#gshort), [`glong`](#glong), [`gfloat`](#gfloat),
   [`gdouble`](#gdouble).
 - Types which correspond exactly to standard C99 types, but are available
   to use even if your compiler does not support C99 — [`gsize`](#gsize),
   [`goffset`](#goffset), [`gintptr`](#gintptr), [`guintptr`](#guintptr).

GLib also defines macros for the limits of some of the standard
integer and floating point types, as well as macros for suitable
[`printf()`](man:printf(3)) formats for these types.

Note that depending on the platform and build configuration, the format
macros might not be compatible with the system provided
[`printf()`](man:printf(3)) function, because GLib might use a different
`printf()` implementation internally. The format macros will always work with
GLib API (like [func@GLib.print]), and with any C99 compatible `printf()`
implementation.

## Basic Types

### `gboolean`

A standard boolean type. Variables of this type should only contain the value
`TRUE` or `FALSE`.

Never directly compare the contents of a `gboolean` variable with the values
`TRUE` or `FALSE`. Use `if (condition)` to check a `gboolean` is ‘true’, instead
of `if (condition == TRUE)`. Likewise use `if (!condition)` to check a
`gboolean` is ‘false’.

There is no validation when assigning to a `gboolean` variable and so it could
contain any value represented by a `gint`. This is why the use of `if
(condition)` is recommended. All non-zero values in C evaluate to ‘true’.

### `gpointer`

An untyped pointer, exactly equivalent to `void *`.

The standard C `void *` type should usually be preferred in
new code, but `gpointer` can be used in contexts where a type name
must be a single word, such as in the `GType` name of
`G_TYPE_POINTER` or when generating a family of function names for
multiple types using macros.

### `gconstpointer`

An untyped pointer to constant data, exactly equivalent to `const void *`.

The data pointed to should not be changed.

This is typically used in function prototypes to indicate
that the data pointed to will not be altered by the function.

The standard C `const void *` type should usually be preferred in
new code, but `gconstpointer` can be used in contexts where a type name
must be a single word.

### `gchar`

Equivalent to the standard C `char` type.

This type only exists for symmetry with `guchar`.
The standard C `char` type should be preferred in new code.

### `guchar`

Equivalent to the standard C `unsigned char` type.

The standard C `unsigned char` type should usually be preferred in
new code, but `guchar` can be used in contexts where a type name
must be a single word, such as in the `GType` name of
`G_TYPE_UCHAR` or when generating a family of function names for
multiple types using macros.

## Naturally Sized Integers

### `gint`

Equivalent to the standard C `int` type.

Values of this type can range from `INT_MIN` to `INT_MAX`,
or equivalently from `G_MININT` to `G_MAXINT`.

This type only exists for symmetry with [`guint`](#guint).
The standard C `int` type should be preferred in new code.

`G_MININT`
:   The minimum value which can be held in a `gint`.

    This is the same as standard C `INT_MIN`, which is available since C99
    and should be preferred in new code.

`G_MAXINT`
:   The maximum value which can be held in a `gint`.

    This is the same as standard C `INT_MAX`, which is available since C99
    and should be preferred in new code.

### `guint`

Equivalent to the standard C `unsigned int` type.

Values of this type can range from `0` to `UINT_MAX`,
or equivalently `0` to `G_MAXUINT`.

The standard C `unsigned int` type should usually be preferred in
new code, but `guint` can be used in contexts where a type name
must be a single word, such as in the `GType` name of
`G_TYPE_UINT` or when generating a family of function names for
multiple types using macros.

`G_MAXUINT`
:   The maximum value which can be held in a `guint`.

    This is the same as standard C `UINT_MAX`, which is available since C99
    and should be preferred in new code.

### `gshort`

Equivalent to the standard C `short` type.

Values of this type can range from `SHRT_MIN` to `SHRT_MAX`,
or equivalently `G_MINSHORT` to `G_MAXSHORT`.

This type only exists for symmetry with `gushort`.
The standard C `short` type should be preferred in new code.

`G_MINSHORT`
:   The minimum value which can be held in a `gshort`.

    This is the same as standard C `SHRT_MIN`, which is available since C99
    and should be preferred in new code.

`G_MAXSHORT`
:   The maximum value which can be held in a `gshort`.

    This is the same as standard C `SHRT_MAX`, which is available since C99
    and should be preferred in new code.

### `gushort`

Equivalent to the standard C `unsigned short` type.

Values of this type can range from `0` to `USHRT_MAX`,
or equivalently from `0` to `G_MAXUSHORT`.

The standard C `unsigned short` type should usually be preferred in
new code, but `gushort` can be used in contexts where a type name
must be a single word, such as when generating a family of function
names for multiple types using macros.


`G_MAXUSHORT`
:   The maximum value which can be held in a `gushort`.

    This is the same as standard C `USHRT_MAX`, which is available since C99
    and should be preferred in new code.

### `glong`

Equivalent to the standard C `long` type.

Values of this type can range from `LONG_MIN` to `LONG_MAX`,
or equivalently `G_MINLONG` to `G_MAXLONG`.

This type only exists for symmetry with `gulong`.
The standard C `long` type should be preferred in new code.

`G_MINLONG`
:   The minimum value which can be held in a `glong`.

    This is the same as standard C `LONG_MIN`, which is available since C99
    and should be preferred in new code.

`G_MAXLONG`
:   The maximum value which can be held in a `glong`.

    This is the same as standard C `LONG_MAX`, which is available since C99
    and should be preferred in new code.

### `gulong`

Equivalent to the standard C `unsigned long` type.

Values of this type can range from `0` to `G_MAXULONG`.

The standard C `unsigned long` type should usually be preferred in
new code, but `gulong` can be used in contexts where a type name
must be a single word, such as in the `GType` name of
`G_TYPE_ULONG` or when generating a family of function names for
multiple types using macros.

`G_MAXULONG`
:   The maximum value which can be held in a `gulong`.

    This is the same as standard C `ULONG_MAX`, which is available since C99
    and should be preferred in new code.

## Fixed Width Integers

### `gint8`

A signed integer guaranteed to be 8 bits on all platforms,
similar to the standard C `int8_t`.

The `int8_t` type should be preferred in new code, unless
consistency with pre-existing APIs requires use of `gint8`
(see [`gsize`](#gsize) for more details).

Values of this type can range from `G_MININT8` (= -128) to
`G_MAXINT8` (= 127).

`G_MININT8`
:   The minimum value which can be held in a `gint8`.

    This is the same as standard C `INT8_MIN`, which is available since C99
    and should be preferred in new code.

    Since: 2.4

`G_MAXINT8`
:   The maximum value which can be held in a `gint8`.

    This is the same as standard C `INT8_MAX`, which is available since C99
    and should be preferred in new code.

    Since: 2.4

### `guint8`

An unsigned integer guaranteed to be 8 bits on all platforms,
similar to the standard C `uint8_t`.

The `uint8_t` type should be preferred in new code, unless
consistency with pre-existing APIs requires use of `guint8`
(see [`gsize`](#gsize) for more details).

Values of this type can range from `0` to `G_MAXUINT8` (= 255).

`G_MAXUINT8`
:   The maximum value which can be held in a `guint8`.

    This is the same as standard C `UINT8_MAX`, which is available since C99
    and should be preferred in new code.

    Since: 2.4

### `gint16`

A signed integer guaranteed to be 16 bits on all platforms,
similar to the standard C `int16_t`.

The `int16_t` type should be preferred in new code, unless
consistency with pre-existing APIs requires use of `gint16`
(see [`gsize`](#gsize) for more details).

Values of this type can range from `G_MININT16` (= -32,768) to
`G_MAXINT16` (= 32,767).

To print or scan values of this type, use
`G_GINT16_MODIFIER` and/or `G_GINT16_FORMAT`.

`G_MININT16`
:   The minimum value which can be held in a `gint16`.

    This is the same as standard C `INT16_MIN`, which is available since C99
    and should be preferred in new code.

    Since: 2.4

`G_MAXINT16`
:   The maximum value which can be held in a `gint16`.

    This is the same as standard C `INT16_MAX`, which is available since C99
    and should be preferred in new code.

    Since: 2.4

`G_GINT16_MODIFIER`
:   The platform dependent length modifier for conversion specifiers
    for scanning and printing values of type `gint16` or `guint16`. It
    is a string literal, but doesn’t include the percent-sign, such
    that you can add precision and length modifiers between percent-sign
    and conversion specifier and append a conversion specifier.

    The following example prints `0x7b`;
    ```c
    gint16 value = 123;
    g_print ("%#" G_GINT16_MODIFIER "x", value);
    ```

    This is not necessarily the correct modifier for printing and scanning
    `int16_t` values, even though the in-memory representation is the same.
    Standard C macros like `PRId16` and `SCNd16` should be used for `int16_t`.

    Since: 2.4

`G_GINT16_FORMAT`
:   This is the platform dependent conversion specifier for scanning and
    printing values of type `gint16`. It is a string literal, but doesn’t
    include the percent-sign, such that you can add precision and length
    modifiers between percent-sign and conversion specifier.

    ```c
    gint16 in;
    gint32 out;
    sscanf ("42", "%" G_GINT16_FORMAT, &in)
    out = in * 1000;
    g_print ("%" G_GINT32_FORMAT, out);
    ```

    This is not necessarily the correct format for printing and scanning
    `int16_t` values, even though the in-memory representation is the same.
    Standard C macros like `PRId16` and `SCNd16` should be used for `int16_t`.

### `guint16`

An unsigned integer guaranteed to be 16 bits on all platforms,
similar to the standard C `uint16_t`.

The `uint16_t` type should be preferred in new code, unless
consistency with pre-existing APIs requires use of `guint16`
(see [`gsize`](#gsize) for more details).

Values of this type can range from `0` to `G_MAXUINT16` (= 65,535).

To print or scan values of this type, use
`G_GINT16_MODIFIER` and/or `G_GUINT16_FORMAT`.

`G_MAXUINT16`
:   The maximum value which can be held in a `guint16`.

    This is the same as standard C `UINT16_MAX`, which is available since C99
    and should be preferred in new code.

    Since: 2.4

`G_GUINT16_FORMAT`
:   This is the platform dependent conversion specifier for scanning
    and printing values of type `guint16`. See also `G_GINT16_FORMAT`

    This is not necessarily the correct modifier for printing and scanning
    `uint16_t` values, even though the in-memory representation is the same.
    Standard C macros like `PRIu16` and `SCNu16` should be used for `uint16_t`.

### `gint32`

A signed integer guaranteed to be 32 bits on all platforms.

The `int32_t` type should be preferred in new code, unless
consistency with pre-existing APIs requires use of `gint32`
(see [`gsize`](#gsize) for more details).

Values of this type can range from `G_MININT32` (= -2,147,483,648)
to `G_MAXINT32` (= 2,147,483,647).

To print or scan values of this type, use
`G_GINT32_MODIFIER` and/or `G_GINT32_FORMAT`.

Note that on platforms with more than one 32-bit standard integer type,
`gint32` and `int32_t` are not necessarily implemented by the same
32-bit integer type.
For example, on an ILP32 platform where `int` and `long` are both 32-bit,
it might be the case that one of these types is `int` and the other
is `long`.
See [`gsize`](#gsize) for more details of what this implies.

`G_MININT32`
:   The minimum value which can be held in a `gint32`.

    This is the same as standard C `INT32_MIN`, which is available since C99
    and should be preferred in new code.

    Since: 2.4

`G_MAXINT32`
:   The maximum value which can be held in a `gint32`.

    This is the same as standard C `INT32_MAX`, which is available since C99
    and should be preferred in new code.

    Since: 2.4

`G_GINT32_MODIFIER`
:   The platform dependent length modifier for conversion specifiers
    for scanning and printing values of type `gint32` or `guint32`. It
    is a string literal. See also `G_GINT16_MODIFIER`.

    This is not necessarily the correct modifier for printing and scanning
    `int32_t` values, even though the in-memory representation is the same.
    Standard C macros like `PRId32` and `SCNd32` should be used for `int32_t`.

    Since: 2.4

`G_GINT32_FORMAT`
:   This is the platform dependent conversion specifier for scanning
    and printing values of type `gint32`. See also `G_GINT16_FORMAT`.

    This is not necessarily the correct modifier for printing and scanning
    `int32_t` values, even though the in-memory representation is the same.
    Standard C macros like `PRId32` and `SCNd32` should be used for `int32_t`.

### `guint32`

An unsigned integer guaranteed to be 32 bits on all platforms,
similar to the standard C `uint32_t`.

The `uint32_t` type should be preferred in new code, unless
consistency with pre-existing APIs requires use of `guint32`
(see [`gsize`](#gsize) for more details).

Values of this type can range from `0` to `G_MAXUINT32` (= 4,294,967,295).

To print or scan values of this type, use
`G_GINT32_MODIFIER` and/or `G_GUINT32_FORMAT`.

Note that on platforms with more than one 32-bit standard integer type,
`guint32` and `uint32_t` are not necessarily implemented by the same
32-bit integer type.
See [`gsize`](#gsize) for more details of what this implies.

`G_MAXUINT32`
:   The maximum value which can be held in a `guint32`.

    This is the same as standard C `UINT32_MAX`, which is available since C99
    and should be preferred in new code.

    Since: 2.4

`G_GUINT32_FORMAT`
:   This is the platform dependent conversion specifier for scanning
    and printing values of type `guint32`. See also `G_GINT16_FORMAT`.

    This is not necessarily the correct modifier for printing and scanning
    `uint32_t` values, even though the in-memory representation is the same.
    Standard C macros like `PRIu32` and `SCNu32` should be used for `uint32_t`.

### `gint64`

A signed integer guaranteed to be 64 bits on all platforms,
similar to the standard C `int64_t`.

The `int64_t` type should be preferred in new code, unless
consistency with pre-existing APIs requires use of `gint64`
(see [`gsize`](#gsize) for more details).

Values of this type can range from `G_MININT64`
(= -9,223,372,036,854,775,808) to `G_MAXINT64`
(= 9,223,372,036,854,775,807).

To print or scan values of this type, use
`G_GINT64_MODIFIER` and/or `G_GINT64_FORMAT`.

Note that on platforms with more than one 64-bit standard integer type,
`gint64` and `int64_t` are not necessarily implemented by the same
64-bit integer type.
For example, on a platform where both `long` and `long long` are 64-bit,
it might be the case that one of those types is used for `gint64`
and the other is used for `int64_t`.
See [`gsize`](#gsize) for more details of what this implies.

`G_MININT64`
:   The minimum value which can be held in a `gint64`.

    This is the same as standard C `INT64_MIN`, which is available since C99
    and should be preferred in new code.

`G_MAXINT64`
:   The maximum value which can be held in a `gint64`.

    This is the same as standard C `INT64_MAX`, which is available since C99
    and should be preferred in new code.

`G_GINT64_MODIFIER`
:   The platform dependent length modifier for conversion specifiers
    for scanning and printing values of type `gint64` or `guint64`.
    It is a string literal.

    Some platforms do not support printing 64-bit integers, even
    though the types are supported. On such platforms `G_GINT64_MODIFIER`
    is not defined.

    This is not necessarily the correct modifier for printing and scanning
    `int64_t` values, even though the in-memory representation is the same.
    Standard C macros like `PRId64` and `SCNd64` should be used for `int64_t`.

    Since: 2.4

`G_GINT64_FORMAT`
:   This is the platform dependent conversion specifier for scanning
    and printing values of type `gint64`. See also `G_GINT16_FORMAT`.

    Some platforms do not support scanning and printing 64-bit integers,
    even though the types are supported. On such platforms `G_GINT64_FORMAT`
    is not defined. Note that [`scanf()`](man:scanf(3)) may not support 64-bit
    integers, even if `G_GINT64_FORMAT` is defined. Due to its weak error
    handling, `scanf()` is not recommended for parsing anyway; consider using
    [func@GLib.ascii_strtoll] instead.

    This is not necessarily the correct format for printing and scanning
    `int64_t` values, even though the in-memory representation is the same.
    Standard C macros like `PRId64` and `SCNd64` should be used for `int64_t`.

`G_GINT64_CONSTANT(val)`
:   This macro is used to insert 64-bit integer literals
    into the source code.

    It is similar to the standard C `INT64_C` macro,
    which should be preferred in new code.

### `guint64`

An unsigned integer guaranteed to be 64-bits on all platforms,
similar to the standard C `uint64_t` type.

The `uint64_t` type should be preferred in new code, unless
consistency with pre-existing APIs requires use of `guint64`
(see [`gsize`](#gsize) for more details).

Values of this type can range from `0` to `G_MAXUINT64`
(= 18,446,744,073,709,551,615).

To print or scan values of this type, use
`G_GINT64_MODIFIER` and/or `G_GUINT64_FORMAT`.

Note that on platforms with more than one 64-bit standard integer type,
`guint64` and `uint64_t` are not necessarily implemented by the same
64-bit integer type.
See [`gsize`](#gsize) for more details of what this implies.

`G_MAXUINT64`
:   The maximum value which can be held in a `guint64`.

    This is the same as standard C `UINT64_MAX`, which is available since C99
    and should be preferred in new code.

`G_GUINT64_FORMAT`
:   This is the platform dependent conversion specifier for scanning
    and printing values of type `guint64`. See also `G_GINT16_FORMAT`.

    Some platforms do not support scanning and printing 64-bit integers,
    even though the types are supported. On such platforms `G_GUINT64_FORMAT`
    is not defined.  Note that [`scanf()`](man:scanf(3)) may not support 64-bit
    integers, even if `G_GINT64_FORMAT` is defined. Due to its weak error
    handling, `scanf()` is not recommended for parsing anyway; consider using
    [func@GLib.ascii_strtoull] instead.

    This is not necessarily the correct modifier for printing and scanning
    `uint64_t` values, even though the in-memory representation is the same.
    Standard C macros like `PRIu64` and `SCNu64` should be used for `uint64_t`.

`G_GUINT64_CONSTANT(val)`
:   This macro is used to insert 64-bit unsigned integer
    literals into the source code.

    It is similar to the standard C `UINT64_C` macro,
    which should be preferred in new code.

    Since: 2.10

## Floating Point

### `gfloat`

Equivalent to the standard C `float` type.

Values of this type can range from `-FLT_MAX` to `FLT_MAX`,
or equivalently from `-G_MAXFLOAT` to `G_MAXFLOAT`.

`G_MINFLOAT`
:   The minimum positive value which can be held in a `gfloat`.

    If you are interested in the smallest value which can be held
    in a `gfloat`, use `-G_MAXFLOAT`.

    This is the same as standard C `FLT_MIN`, which is available since C99
    and should be preferred in new code.

`G_MAXFLOAT`
:   The maximum value which can be held in a `gfloat`.

    This is the same as standard C `FLT_MAX`, which is available since C99
    and should be preferred in new code.

### `gdouble`

Equivalent to the standard C `double` type.

Values of this type can range from `-DBL_MAX` to `DBL_MAX`,
or equivalently from `-G_MAXDOUBLE` to `G_MAXDOUBLE`.

`G_MINDOUBLE`
:   The minimum positive value which can be held in a `gdouble`.

    If you are interested in the smallest value which can be held
    in a `gdouble`, use `-G_MAXDOUBLE`.

    This is the same as standard C `DBL_MIN`, which is available since C99
    and should be preferred in new code.

`G_MAXDOUBLE`
:   The maximum value which can be held in a `gdouble`.

    This is the same as standard C `DBL_MAX`, which is available since C99
    and should be preferred in new code.

## Architecture Sized Integers

### `gsize`

An unsigned integer type of the result of the `sizeof` operator,
corresponding to the `size_t` type defined in C99.

The standard `size_t` type should be preferred in new code, unless
consistency with pre-existing APIs requires `gsize`
(see below for more details).

`gsize` is usually 32 bit wide on a 32-bit platform and 64 bit wide
on a 64-bit platform. Values of this type can range from `0` to
`G_MAXSIZE`.

This type is wide enough to hold the size of the largest possible
memory allocation, but is not guaranteed to be wide enough to hold
the numeric value of a pointer: on platforms that use tagged pointers,
such as [CHERI](https://cheri-cpu.org/), pointers can be numerically
larger than the size of the address space.
If the numeric value of a pointer needs to be stored in an integer
without information loss, use the standard C types `intptr_t` or
`uintptr_t`, or the similar GLib types [`gintptr`](#gintptr) or
[`guintptr`](#guintptr).

To print or scan values of this type, use
`G_GSIZE_MODIFIER` and/or `G_GSIZE_FORMAT`.

Note that on platforms where more than one standard integer type is
the same size, `size_t` and `gsize` are always the same size but are
not necessarily implemented by the same standard integer type.
For example, on an ILP32 platform where `int`, `long` and pointers
are all 32-bit, `size_t` might be `unsigned long` while `gsize`
might be `unsigned int`.
This can result in compiler warnings or unexpected C++ name-mangling
if the two types are used inconsistently.

As a result, changing a type from `gsize` to `size_t` in existing APIs
might be an incompatible API or ABI change, especially if C++
is involved. The safe option is to leave existing APIs using the same type
that they have historically used, and only use the standard C types in
new APIs.

Similar considerations apply to all the fixed-size types
([`gint8`](#gint8), [`guint8`](#guint8), [`gint16`](#gint16),
[`guint16`](#guint16), [`gint32`](#gint32), [`guint32`](#guint32),
[`gint64`](#gint64), [`guint64`](#guint64) and [`goffset`](#goffset)), as well
as [`gintptr`](#gintptr) and [`guintptr`](#guintptr).
Types that are 32 bits or larger are particularly likely to be
affected by this.

`G_MAXSIZE`
:   The maximum value which can be held in a `gsize`.

    This is the same as standard C `SIZE_MAX` (available since C99),
    which should be preferred in new code.

    Since: 2.4

`G_GSIZE_MODIFIER`
:   The platform dependent length modifier for conversion specifiers
    for scanning and printing values of type `gsize`. It
    is a string literal.

    Note that this is not necessarily the correct modifier to scan or
    print a `size_t`, even though the in-memory representation is the
    same. The Standard C `"z"` modifier should be used for `size_t`,
    assuming a C99-compliant `printf` implementation is available.

    Since: 2.6

`G_GSIZE_FORMAT`
:   This is the platform dependent conversion specifier for scanning
    and printing values of type `gsize`. See also `G_GINT16_FORMAT`.

    Note that this is not necessarily the correct format to scan or
    print a `size_t`, even though the in-memory representation is the
    same. The standard C `"zu"` format should be used for `size_t`,
    assuming a C99-compliant `printf` implementation is available.

    Since: 2.6

### `gssize`

A signed variant of [`gsize`](#gsize), corresponding to the
`ssize_t` defined in POSIX or the similar `SSIZE_T` in Windows.

In new platform-specific code, consider using `ssize_t` or `SSIZE_T`
directly.

Values of this type can range from `G_MINSSIZE` to `G_MAXSSIZE`.

Note that on platforms where `ssize_t` is implemented, `ssize_t` and
`gssize` might be implemented by different standard integer types
of the same size. Similarly, on Windows, `SSIZE_T` and `gssize`
might be implemented by different standard integer types of the same
size. See [`gsize`](#gsize) for more details.

This type is also not guaranteed to be the same as standard C
`ptrdiff_t`, although they are the same on many platforms.

To print or scan values of this type, use
`G_GSSIZE_MODIFIER` and/or `G_GSSIZE_FORMAT`.

`G_MINSSIZE`
:   The minimum value which can be held in a `gssize`.

    Since: 2.14

`G_MAXSSIZE`
:   The maximum value which can be held in a `gssize`.

    Since: 2.14

`G_GSSIZE_FORMAT`
:   This is the platform dependent conversion specifier for scanning
    and printing values of type `gssize`. See also `G_GINT16_FORMAT`.

    Note that this is not necessarily the correct format to scan or print
    a POSIX `ssize_t` or a Windows `SSIZE_T`, even though the in-memory
    representation is the same.
    On POSIX platforms, the `"zd"` format should be used for `ssize_t`.

    Since: 2.6

`G_GSSIZE_MODIFIER`
:   The platform dependent length modifier for conversion specifiers
    for scanning and printing values of type `gssize`. It
    is a string literal.

    Note that this is not necessarily the correct modifier to scan or print
    a POSIX `ssize_t` or a Windows `SSIZE_T`, even though the in-memory
    representation is the same.
    On POSIX platforms, the `"z"` modifier should be used for `ssize_t`.

    Since: 2.6

### `goffset`

A signed integer type that is used for file offsets,
corresponding to the POSIX type `off_t` as if compiling with
`_FILE_OFFSET_BITS` set to 64. `goffset` is always 64 bits wide, even on
32-bit architectures, and even if `off_t` is only 32 bits.
Values of this type can range from `G_MINOFFSET` to
`G_MAXOFFSET`.

To print or scan values of this type, use
`G_GOFFSET_MODIFIER` and/or `G_GOFFSET_FORMAT`.

On platforms with more than one 64-bit standard integer type,
even if `off_t` is also 64 bits in size, `goffset` and `off_t` are not
necessarily implemented by the same 64-bit integer type.
See [`gsize`](#gsize) for more details of what this implies.

Since: 2.14

`G_MINOFFSET`
:   The minimum value which can be held in a `goffset`.

`G_MAXOFFSET`
:   The maximum value which can be held in a `goffset`.

`G_GOFFSET_MODIFIER`
:   The platform dependent length modifier for conversion specifiers
    for scanning and printing values of type `goffset`. It is a string
    literal. See also `G_GINT64_MODIFIER`.

    This modifier should only be used with `goffset` values, and not
    with `off_t`, which is not necessarily the same type or even the same size.

    Since: 2.20

`G_GOFFSET_FORMAT`
:   This is the platform dependent conversion specifier for scanning
    and printing values of type `goffset`. See also `G_GINT64_FORMAT`.

    This format should only be used with `goffset` values, and not
    with `off_t`, which is not necessarily the same type or even the same size.

    Since: 2.20

`G_GOFFSET_CONSTANT(val)`
:   This macro is used to insert `goffset` 64-bit integer literals
    into the source code.

    See also `G_GINT64_CONSTANT()`.

    Since: 2.20

### `gintptr`

Corresponds to the C99 type `intptr_t`,
a signed integer type that can hold any pointer.

The standard `intptr_t` type should be preferred in new code, unless
consistency with pre-existing APIs requires `gintptr`.
Note that `intptr_t` and `gintptr` might be implemented by different
standard integer types of the same size. See [`gsize`](#gsize) for more details.

`gintptr` is not guaranteed to be the same type or the same size as
[`gssize`](#gssize), even though they are the same on many CPU architectures.

To print or scan values of this type, use
`G_GINTPTR_MODIFIER` and/or `G_GINTPTR_FORMAT`.

Since: 2.18

`G_GINTPTR_MODIFIER`
:   The platform dependent length modifier for conversion specifiers
    for scanning and printing values of type `gintptr` or `guintptr`.
    It is a string literal.

    Note that this is not necessarily the correct modifier to scan or
    print an `intptr_t`, even though the in-memory representation is the
    same.
    Standard C macros like `PRIdPTR` and `SCNdPTR` should be used for
    `intptr_t`.

    Since: 2.22

`G_GINTPTR_FORMAT`
:   This is the platform dependent conversion specifier for scanning
    and printing values of type `gintptr`.

    Note that this is not necessarily the correct format to scan or
    print an `intptr_t`, even though the in-memory representation is the
    same.
    Standard C macros like `PRIdPTR` and `SCNdPTR` should be used for
    `intptr_t`.

    Since: 2.22

### `guintptr`

Corresponds to the C99 type `uintptr_t`,
an unsigned integer type that can hold any pointer.

The standard `uintptr_t` type should be preferred in new code, unless
consistency with pre-existing APIs requires `guintptr`.
Note that `uintptr_t` and `guintptr` might be implemented by different
standard integer types of the same size. See [`gsize`](#gsize) for more details.

`guintptr` is not guaranteed to be the same type or the same size as
[`gsize`](#gsize), even though they are the same on many CPU architectures.

To print or scan values of this type, use
`G_GINTPTR_MODIFIER` and/or `G_GUINTPTR_FORMAT`.

Since: 2.18

`G_GUINTPTR_FORMAT`
:   This is the platform dependent conversion specifier
    for scanning and printing values of type `guintptr`.

    Note that this is not necessarily the correct format to scan or
    print a `uintptr_t`, even though the in-memory representation is the
    same.
    Standard C macros like `PRIuPTR` and `SCNuPTR` should be used for
    `uintptr_t`.

    Since: 2.22
