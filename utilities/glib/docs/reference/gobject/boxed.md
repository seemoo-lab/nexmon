Title: Boxed Types

# Boxed Types

A "boxed type" is a generic wrapper mechanism for arbitrary C structures.
The only thing the type system needs to know about the structures is how to
copy them (a [`callback@GObject.BoxedCopyFunc`]) and how to free them (a
[`callback@GObject.BoxedFreeFunc`])—beyond that they are treated as opaque
chunks of memory.

Boxed types are useful for simple value-holder structures like rectangles or
points. They can also be used for wrapping structures defined in non-GObject
based libraries. They allow arbitrary structures to be handled in a uniform
way, allowing uniform copying (or referencing) and freeing (or
unreferencing) of them, and uniform representation of the type of the
contained structure. In turn, this allows any type which can be boxed to be
set as the data in a `GValue`, which allows for polymorphic handling of a much
wider range of data types, and hence usage of such types as `GObject` property
values.

All boxed types inherit from the `G_TYPE_BOXED` fundamental type.

It is very important to note that boxed types are **not** deeply
inheritable: you cannot register a boxed type that inherits from another
boxed type. This means you cannot create your own custom, parallel type
hierarchy and map it to GType using boxed types. If you want to have deeply
inheritable types without using GObject, you will need to use
`GTypeInstance`.

## Registering a new boxed type

The recommended way to register a new boxed type is to use the
[`func@GObject.DEFINE_BOXED_TYPE`] macro:

```c
// In the header

#define EXAMPLE_TYPE_RECTANGLE (example_rectangle_get_type())

typedef struct {
  double x, y;
  double width, height;
} ExampleRectangle;

GType
example_rectangle_get_type (void);

ExampleRectangle *
example_rectangle_copy (ExampleRectangle *r);

void
example_rectangle_free (ExampleRectangle *r);

// In the source
G_DEFINE_BOXED_TYPE (ExampleRectangle, example_rectangle,
                     example_rectangle_copy,
                     example_rectangle_free)
```

Just like `G_DEFINE_TYPE` and `G_DEFINE_INTERFACE_TYPE`, the
`G_DEFINE_BOXED_TYPE` macro will provide the definition of the `get_type()`
function, which will call [`func@GObject.boxed_type_register_static`] with
the given type name as well as the `GBoxedCopyFunc` and `GBoxedFreeFunc`
functions.

## Using boxed types

### Object properties

In order to use a boxed type with GObject properties you will need to
register the property using [`func@GObject.param_spec_boxed`], e.g.

```c
obj_props[PROP_BOUNDS] =
  g_param_spec_boxed ("bounds", NULL, NULL,
                      EXAMPLE_TYPE_RECTANGLE,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
```

In the `set_property` implementation you can use `g_value_get_boxed()` to
retrieve a pointer to the boxed type:

```c
switch (prop_id)
  {
  // ...
  case PROP_BOUNDS:
    example_object_set_bounds (self, g_value_get_boxed (value));
    break;
  // ...
  }
```

Similarly, you can use `g_value_set_boxed()` in the implementation of the
`get_property` virtual function:

```c
switch (prop_id)
  {
  // ...
  case PROP_BOUNDS:
    g_value_set_boxed (self, &self->bounds);
    break;
  // ...
  }
```

## Reference counting

Boxed types are designed so that reference counted types can be boxed. Use
the type’s ‘ref’ function as the `GBoxedCopyFunc`, and its ‘unref’ function as
the `GBoxedFreeFunc`. For example, for `GBytes`, the `GBoxedCopyFunc` is
`g_bytes_ref()`, and the GBoxedFreeFunc is `g_bytes_unref()`.
