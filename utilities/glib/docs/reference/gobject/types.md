Title: Types
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2001, 2003 Owen Taylor
SPDX-FileCopyrightText: 2002, 2003, 2004, 2005 Matthias Clasen
SPDX-FileCopyrightText: 2003, 2006 Josh Parsons
SPDX-FileCopyrightText: 2005 Stefan Kost
SPDX-FileCopyrightText: 2015 Collabora, Ltd.
SPDX-FileCopyrightText: 2021 Emmanuele Bassi
SPDX-FileCopyrightText: 2023 Endless OS Foundation, LLC

# Types

The GType API is the foundation of the GObject system. It provides the
facilities for registering and managing all fundamental data types,
user-defined object and interface types.

For type creation and registration purposes, all types fall into one of
two categories: static or dynamic. Static types are never loaded or
unloaded at run-time as dynamic types may be.  Static types are created
with [func@GObject.type_register_static] that gets type specific
information passed in via a `GTypeInfo` structure.

Dynamic types are created with [func@GObject.type_register_dynamic],
which takes a `GTypePlugin` structure instead. The remaining type information
(the `GTypeInfo` structure) is retrieved during runtime through `GTypePlugin`
and the `g_type_plugin_*()` API.

These registration functions are usually called only once from a
function whose only purpose is to return the type identifier for a
specific class. Once the type (or class or interface) is registered,
it may be instantiated, inherited, or implemented depending on exactly
what sort of type it is.

There is also a third registration function for registering fundamental
types called [func@GObject.type_register_fundamental], which requires
both a `GTypeInfo` structure and a `GTypeFundamentalInfo` structure, but it
is rarely used since most fundamental types are predefined rather than user-defined.

Type instance and class structs are limited to a total of 64 KiB,
including all parent types. Similarly, type instances' private data
(as created by `G_ADD_PRIVATE()`) are limited to a total of
64 KiB. If a type instance needs a large static buffer, allocate it
separately (typically by using [`struct@GLib.Array`] or [`struct@GLib.PtrArray`])
and put a pointer to the buffer in the structure.

As mentioned in the [GType conventions](concepts.html#conventions), type names must
be at least three characters long. There is no upper length limit. The first
character must be a letter (a–z or A–Z) or an underscore (‘\_’). Subsequent
characters can be letters, numbers or any of ‘-\_+’.

# Runtime Debugging

When `G_ENABLE_DEBUG` is defined during compilation, the GObject library
supports an environment variable `GOBJECT_DEBUG` that can be set to a
combination of flags to trigger debugging messages about
object bookkeeping and signal emissions during runtime.

The currently supported flags are:

 - `objects`: Tracks all `GObject` instances in a global hash table called
   `debug_objects_ht`, and prints the still-alive objects on exit.
 - `instance-count`: Tracks the number of instances of every `GType` and makes
   it available via the [func@GObject.type_get_instance_count] function.
 - `signals`: Currently unused.
