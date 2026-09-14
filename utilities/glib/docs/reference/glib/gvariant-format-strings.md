Title: GVariant Format Strings

# GVariant Format Strings

## Variable Argument Conversions

This page attempts to document how to perform variable argument conversions
with [struct@GLib.Variant].

Conversions occur according to format strings. A format string is a two-way
mapping between a single `GVariant` value and one or more C values.

A conversion from C values into a `GVariant` value is made using the
[ctor@GLib.Variant.new] function. A conversion from a `GVariant` into C
values is made using the [method@GLib.Variant.get] function.

## Syntax

This section exhaustively describes all possibilities for GVariant format
strings. There are no valid forms of format strings other than those
described here. Please note that the format string syntax is likely to
expand in the future.

Valid format strings have one of the following forms:

- any type string
- a type string prefixed with a `@`
- `&s`, `&o`, `&g`, `^as`, `^a&s`, `^ao`, `^a&o`, `^ay`, `^&ay`, `^aay` or
  `^a&ay`.
- any format string, prefixed with an `m`
- a sequence of zero or more format strings, concatenated and enclosed in
  parentheses
- an opening brace, followed by two format strings, followed by a closing
  brace (subject to the constraint that the first format string correspond
  to a type valid for use as the key type of a dictionary)

## Symbols

The following table describes the rough meaning of symbols that may appear inside a GVariant format string. Each symbol is described in detail in its own section, including usage examples.

| Symbol | Meaning |
|--------|---------|
| `b`, `y`, `n`, `q`, `i`, `u`, `x`, `t`, `h`, `d` | Used for building or deconstructing boolean, byte and numeric types. See [Numeric Types](#numeric-types) below. |
| `s`, `o`, `g` | Used for building or deconstructing string types. See [Strings](#strings) below. |
| `v` | Used for building or deconstructing variant types. See [Variants](#variants) below. |
| `a` | Used for building or deconstructing arrays. See [Arrays](#arrays) below. |
| `m` | Used for building or deconstructing maybe types. See [Maybe Types](#maybe-types) below. |
| `()` | Used for building or deconstructing tuples. See [Tuples](#tuples) below. |
| `{}` | Used for building or deconstructing dictionary entries. See [Dictionaries](#dictionaries) below. |
| `@` | Used as a prefix for a `GVariant` type string (not a prefix for a format string, so `@as` is a valid format string but `@^as` is not). Denotes that a pointer to a GVariant should be used in place of the normal C type or types. For `g_variant_new()` this means that you must pass a non-`NULL` `(GVariant *)`; if it is a floating reference, ownership will be taken, as if by using `g_variant_ref_sink()`. For `g_variant_get()` this means that you must pass a pointer to a `(GVariant *)` for the value to be returned by reference or `NULL` to ignore the value. See [`GVariant *`](#gvariant) below. |
| `*`, `?`, `r` | Exactly equivalent to `@*`, `@?` and `@r`. Provided only for completeness so that all `GVariant` type strings can be used also as format strings. See [`GVariant *`](#gvariant) below. |
| `&` | Used as a prefix for a `GVariant` type string (not a prefix for a format string, so `&s` is a valid format string but `&@s` is not). Denotes that a C pointer to serialised data should be used in place of the normal C type. See [Pointers](#pointers) below. |
| `^` | Used as a prefix on some specific types of format strings. See [Convenience Conversions](#convenience-conversions) below. |

## Numeric Types

Characters: `b`, `y`, `n`, `q`, `i`, `u`, `x`, `t`, `h`, `d`

Variable argument conversions from numeric types work in the most obvious
way possible. Upon encountering one of these characters, `g_variant_new()`
takes the equivalent C type as an argument. `g_variant_get()` takes a pointer
to the equivalent C type (or `NULL` to ignore the value).

The equivalent C types are as follows:

| Character | Equivalent C type |
|-----------|-------------------|
| `b`       | `gboolean`        |
| `y`       | `guchar`          |
| `n`       | `gint16`          |
| `q`       | `guint16`         |
| `i`       | `gint32`          |
| `u`       | `guint32`         |
| `x`       | `gint64`          |
| `t`       | `guint64`         |
| `h`       | `gint32` (handle) |
| `d`       | `gdouble`         |

<a id="varargs"></a>
Note that in C, small integer types in variable argument lists are promoted
up to `int` or `unsigned int` as appropriate, and read back accordingly. `int`
is 32 bits on every platform on which GLib is currently supported. This
means that you can use C expressions of type `int` with `g_variant_new()` and
format characters `b`, `y`, `n`, `q`, `i`, `u` and `h`. Specifically, you
can use integer literals with these characters.

When using the `x` and `t` characters, you must ensure that the value that
you provide is 64 bit. This means that you should use a cast or make use of
the `G_GINT64_CONSTANT` or `G_GUINT64_CONSTANT` macros.

No type promotion occurs when using `g_variant_get()` since it operates with
pointers. The pointers must always point to a memory region of exactly the
correct size.

### Examples

```c
GVariant *value1, *value2, *value3, *value4;

value1 = g_variant_new ("y", 200);
value2 = g_variant_new ("b", TRUE);
value3 = g_variant_new ("d", 37.5):
value4 = g_variant_new ("x", G_GINT64_CONSTANT (998877665544332211));

{
  gdouble floating;
  gboolean truth;
  gint64 bignum;


  g_variant_get (value1, "y", NULL);      /* ignore the value. */
  g_variant_get (value2, "b", &truth);
  g_variant_get (value3, "d", &floating);
  g_variant_get (value4, "x", &bignum);
}
```

## Strings

Characters: `s`, `o`, `g`

String conversions occur to and from standard nul-terminated C strings. Upon
encountering an `s`, `o` or `g` in a format string, `g_variant_new()` takes a
`(const gchar *)` and makes a copy of it. `NULL` is not a valid string; use
maybe types to encode that. If the `o` or `g` characters are used, care must
be taken to ensure that the passed string is a valid D-Bus object path or
D-Bus type signature, respectively.

Upon encountering `s`, `o` or `g`, `g_variant_get()` takes a pointer to a
`(gchar *)` (ie: `(gchar **)`) and sets it to a newly-allocated copy of the
string. It is appropriate to free this copy using `g_free()`. `NULL` may
also be passed to indicate that the value of the string should be ignored
(in which case no copy is made).

### Examples

```c
GVariant *value1, *value2, *value3;

value1 = g_variant_new ("s", "hello world!");
value2 = g_variant_new ("o", "/must/be/a/valid/path");
value3 = g_variant_new ("g", "iias");

#if 0
  g_variant_new ("s", NULL);      /* not valid: NULL is not a string. */
#endif

{
  gchar *result;

  g_variant_get (value1, "s", &result);
  g_print ("It was '%s'\n", result);
  g_free (result);
}
```

## Variants

Characters: `v`

Upon encountering a `v`, `g_variant_new()` takes a `(GVariant *)`. The value
of the `GVariant` is used as the contents of the variant value.

Upon encountering a `v`, `g_variant_get()` takes a pointer to a `(GVariant
*)` (ie: `(GVariant **)`). It is set to a new reference to a `GVariant`
instance containing the contents of the variant value. It is appropriate to
free this reference using `g_variant_unref()`. `NULL` may also be passed to
indicate that the value should be ignored (in which case no new reference is
created).

### Examples

```c
GVariant *x, *y;

/* the following two lines are equivalent: */
x = g_variant_new ("v", y);
x = g_variant_new_variant (y);

/* as are these: */
g_variant_get (x, "v", &y);
y = g_variant_get_variant (x);
```

## Arrays

Characters: `a`

Upon encountering an `a` character followed by a type string,
`g_variant_new()` will take a `(GVariantBuilder *)` that has been created as
an array builder for an array of the type given in the type string. The
builder will have `g_variant_builder_end()` called on it and the result will
be used as the value. As a special exception, if the given type string is a
definite type, then `NULL` may be given to mean an empty array of that type.

Upon encountering an `a` character followed by a type string,
`g_variant_get()` will take a pointer to a `(GVariantIter *)` (ie:
`(GVariantIter **)`). A new heap-allocated iterator is created and returned,
initialised for iterating over the elements of the array. This iterator
should be freed when you are done with it, using `g_variant_iter_free()`.
`NULL` may also be given to indicate that the value of the array should be
ignored.

### Examples

```c
GVariantBuilder *builder;
GVariant *value;

builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));
g_variant_builder_add (builder, "s", "when");
g_variant_builder_add (builder, "s", "in");
g_variant_builder_add (builder, "s", "the");
g_variant_builder_add (builder, "s", "course");
value = g_variant_new ("as", builder);
g_variant_builder_unref (builder);

{
  GVariantIter *iter;
  gchar *str;

  g_variant_get (value, "as", &iter);
  while (g_variant_iter_loop (iter, "s", &str))
    g_print ("%s\n", str);
  g_variant_iter_free (iter);
}

g_variant_unref (value);
```

## Maybe Types

Characters: `m`

Maybe types are handled in two separate ways depending on the format string
that follows the `m`. The method that is used currently depends entirely on
the character immediately following the `m`.

The first way is used with format strings starting with `a`, `s`, `o`, `g`,
`v`, `@`, `*`, `?`, `r`, `&`, or `^`. In all of these cases, for non-maybe
types, `g_variant_new()` takes a pointer to a non-`NULL` value and
`g_variant_get()` returns (by reference) a non-`NULL` pointer. When any of
these format strings are prefixed with an `m`, the type of arguments that
are collected does not change in any way, but `NULL` becomes a permissible
value, to indicate the Nothing case.

Note that the "special exception" introduced in the array section for
constructing empty arrays is ignored here. Using a `NULL` pointer with the
format string `mas` constructs the Nothing value -- not an empty array.

The second way is used with all other format strings. For `g_variant_new()`
an additional gboolean argument is collected and for `g_variant_get()` an
additional `(gboolean *)`. Following this argument, the arguments that are
normally collected for the equivalent non-maybe type will be collected.

If `FALSE` is given to `g_variant_new()` then the Nothing value is
constructed and the collected arguments are ignored. Otherwise (if `TRUE`
was given), the arguments are used in the normal way to create the Just
value.

If `NULL` is given to `g_variant_get()` then the value is ignored. If a
non-`NULL` pointer is given then it is used to return by reference whether
the value was Just. In the case that the value was Just, the `gboolean` will
be set to `TRUE` and the value will be stored in the arguments in the usual
way. In the case that the value was Nothing, the `gboolean` will be set to
`FALSE` and the arguments will be collected in the normal way but have their
values set to binary zero.

### Examples

```c
GVariant *value1, *value2, *value3, *value4, *value5, *value6;
value1 = g_variant_new ("ms", "Hello world");
value2 = g_variant_new ("ms", NULL);
value3 = g_variant_new ("(m(ii)s)", TRUE, 123, 456, "Done");
value4 = g_variant_new ("(m(ii)s)", FALSE, -1, -1, "Done");          /* both '-1' are ignored. */
value5 = g_variant_new ("(m@(ii)s)", NULL, "Done");

{
  GVariant *contents;
  const gchar *cstr;
  gboolean just;
  gint32 x, y;
  gchar *str;

  g_variant_get (value1, "ms", &str);
  if (str != NULL)
    g_print ("str: %s\n", str);
  else
    g_print ("it was null\n");
  g_free (str);


  g_variant_get (value2, "m&s", &cstr);
  if (cstr != NULL)
    g_print ("str: %s\n", cstr);
  else
    g_print ("it was null\n");
  /* don't free 'cstr' */


  /* NULL passed for the gboolean *, but two 'gint32 *' still collected */
  g_variant_get (value3, "(m(ii)s)", NULL, NULL, NULL, &str);
  g_print ("string is %s\n", str);
  g_free (str);

  /* note: &s used, so g_free() not needed */
  g_variant_get (value4, "(m(ii)&s)", &just, &x, &y, &cstr);
  if (just)
    g_print ("it was (%d, %d)\n", x, y);
  else
    g_print ("it was null\n");
  g_print ("string is %s\n", cstr);
  /* don't free 'cstr' */


  g_variant_get (value5, "(m*s)", &contents, NULL); /* ignore the string. */
  if (contents != NULL)
    {
      g_variant_get (contents, "(ii)", &x, &y);
      g_print ("it was (%d, %d)\n", x, y);
      g_variant_unref (contents);
    }
  else
    g_print ("it was null\n");
}
```

## Tuples

Characters: `()`

Tuples are handled by handling each item in the tuple, in sequence. Each
item is handled in the usual way.

### Examples

```c
GVariant *value1, *value2;

value1 = g_variant_new ("(s(ii))", "Hello", 55, 77);
value2 = g_variant_new ("()");

{
  gchar *string;
  gint x, y;

  g_variant_get (value1, "(s(ii))", &string, &x, &y);
  g_print ("%s, %d, %d\n", string, x, y);
  g_free (string);

  g_variant_get (value2, "()");   /* do nothing... */
}
```

## `GVariant *`

Characters: `@`, `*`, `?`, `r`

Upon encountering a `@` in front of a type string, `g_variant_new()` takes a
non-`NULL` pointer to a `GVariant` and uses its value directly instead of
collecting arguments to create the value. The provided `GVariant` must have
a type that matches the type string following the `@`. `*` is the same as
`@*` (ie: take a `GVariant` of any type). `?` is the same as `@?` (ie: take
a `GVariant` of any basic type). `r` is the same as `@r` (ie: take a
`GVariant` of any tuple type).

Upon encountering a `@` in front of a type string, `g_variant_get()` takes a
pointer to a `(GVariant *)` (ie: a `(GVariant **)`) and sets it to a new
reference to a `GVariant` containing the value (instead of deconstructing
the value into C types in the usual way). `NULL` can be given to ignore the
value. `*`, `?` and `r` are handled in a way analogous to what is stated
above.

You can always use `*` as an alternative to `?`, `r` or any use of `@`.
Using the other characters where possible is recommended, however, due to
the improvements in type safety and code self-documentation.

### Examples

```c
GVariant *value1, *value2;

value1 = g_variant_new ("(i@ii)", 44, g_variant_new_int32 (55), 66);

/* note: consumes floating reference count on 'value1' */
value2 = g_variant_new ("(@(iii)*)", value1, g_variant_new_string ("foo"));

{
  const gchar *string;
  GVariant *tmp;
  gsize length;
  gint x, y, z;

  g_variant_get (value2, "((iii)*)", &x, &y, &z, &tmp);
  string = g_variant_get_string (tmp, &length);
  g_print ("it is %d %d %d %s (length=%d)\n", x, y, z, string, (int) length);
  g_variant_unref (tmp);

  /* quick way to skip all the values in a tuple */
  g_variant_get (value2, "(rs)", NULL, &string); /* or "(@(iii)s)" */
  g_print ("i only got the string: %s\n", string);
  g_free (string);
}
```

## Dictionaries

Characters: `{}`

Dictionary entries are handled by handling first the key, then the value. Each is handled in the usual way.

### Examples

```c
GVariantBuilder *b;
GVariant *dict;

b = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));
g_variant_builder_add (b, "{sv}", "name", g_variant_new_string ("foo"));
g_variant_builder_add (b, "{sv}", "timeout", g_variant_new_int32 (10));
dict = g_variant_builder_end (b);
```

To extract data from nested dictionaries you can go through a vardict.

### Examples

```c
GVariant *data;
gint value = 1;
gint max = 3;

/* type (oa{sa{sv}) */
data = g_variant_new_parsed ("(%o, {'brightness': {'value': <%i>, 'max': <%i>}})",
                             "/object/path", value, max);
{
  GVariant *params;
  GVariant *p_brightness;
  gchar *obj
  gint p_max;

  g_variant_get (data, "(o@a{?*})", &obj, &params);
  g_print ("object_path: %s\n", obj);

  p_brightness = g_variant_lookup_value (params, "brightness", G_VARIANT_TYPE_VARDICT);
  g_variant_lookup (p_brightness, "max", "i", &p_max);
  g_print ("max: %d\n", p_max);
}
```

## Pointers

Characters: `&`

The `&` character is used to indicate that serialised data should be
directly exchanged via a pointer.

Currently, the only use for this character is when it is applied to a string
(ie: `&s`, `&o` or `&g`). For `g_variant_new()` this has absolutely no
effect. The string is collected and duplicated normally. For
`g_variant_get()` it means that instead of creating a newly allocated copy
of the string, a pointer to the serialised data is returned. This pointer
should not be freed. Validity checks are performed to ensure that the string
data will always be properly nul-terminated.

### Examples

```c
{
  const gchar *str;
  GVariant *value;

  value = g_variant_new ("&s", "hello world");
  g_variant_get (value, "&s", &str);
  g_print ("string is: %s\n", str);
  /* no need to free str */
}
```

## Convenience Conversions

Characters: `^`

The `^` character currently supports conversion to and from bytestrings or
to and from arrays of strings or bytestrings. It does not support byte
arrays. It has a number of forms.

In all forms, when used with `g_variant_new()` one pointer value is
collected from the variable arguments and passed to a function (as given in
the table below). The result of that function is used as the value for this
position. When used with `g_variant_get()` one pointer value is produced by
using the function (given in the table) and returned by reference.

| Conversion | Used with `g_variant_new()`                              | Used with `g_variant_get()`                                |
|------------|----------------------------------------------------------|------------------------------------------------------------|
| `^as`      | equivalent to [`ctor@GLib.Variant.new_strv`]             | equivalent to [`method@GLib.Variant.dup_strv`]             |
| `^a&s`     |                                                          | equivalent to [`method@GLib.Variant.get_strv`]             |
| `^ao`      | equivalent to [`ctor@GLib.Variant.new_objv`]             | equivalent to [`method@GLib.Variant.dup_objv`]             |
| `^a&o`     |                                                          | equivalent to [`method@GLib.Variant.get_objv`]             |
| `^ay`      | equivalent to [`ctor@GLib.Variant.new_bytestring`]       | equivalent to [`method@GLib.Variant.dup_bytestring`]       |
| `^&ay`     |                                                          | equivalent to [`method@GLib.Variant.get_bytestring`]       |
| `^aay`     | equivalent to [`ctor@GLib.Variant.new_bytestring_array`] | equivalent to [`method@GLib.Variant.dup_bytestring_array`] |
| `^a&ay`    |                                                          | equivalent to [`method@GLib.Variant.get_bytestring_array`] |
