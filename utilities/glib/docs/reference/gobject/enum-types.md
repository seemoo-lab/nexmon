Title: Enumeration Types

# Enumeration Types

The GLib type system provides fundamental types for enumeration and flags
types. Enumerations types are collection of identifiers associated with a
numeric value; flags types are like enumerations, but allow their values to
be combined by bitwise or.

A registered enumeration or flags type associates a name and a nickname with
each allowed value, and the methods [`func@GObject.enum_get_value_by_name`],
[`func@GObject.enum_get_value_by_nick`], [`func@GObject.flags_get_value_by_name`] and
[`func@GObject.flags_get_value_by_nick`] can look up values by their name or nickname.

When an enumeration or flags type is registered with the GLib type system,
it can be used as value type for object properties, using
[`func@GObject.param_spec_enum`] or [`func@GObject.param_spec_flags`].

GObject ships with a utility called `glib-mkenums`, that can construct
suitable type registration functions from C enumeration definitions.

Example of how to get a string representation of an enum value:

```c
GEnumClass *enum_class;
GEnumValue *enum_value;

enum_class = g_type_class_ref (EXAMPLE_TYPE_ENUM);
enum_value = g_enum_get_value (enum_class, EXAMPLE_ENUM_FOO);

g_print ("Name: %s\n", enum_value->value_name);

g_type_class_unref (enum_class);
```

Alternatively, you can use [`func@GObject.enum_to_string`].
