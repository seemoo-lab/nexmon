Title: Type System Concepts

# Type System Concepts

## Introduction

Most modern programming languages come with their own native object systems
and additional fundamental algorithmic language constructs. Just as GLib
serves as an implementation of such fundamental types and algorithms (linked
lists, hash tables and so forth), the GLib Object System provides the
required implementations of a flexible, extensible, and intentionally easy
to map (into other languages) object-oriented framework for C. The
substantial elements that are provided can be summarized as:

- A generic type system to register arbitrary single-inherited flat and deep
  derived types as well as interfaces for structured types. It takes care of
  creation, initialization and memory management of the assorted object and
  class structures, maintains parent/child relationships and deals with
  dynamic implementations of such types. That is, their type specific
  implementations are relocatable/unloadable during runtime.
- A collection of fundamental type implementations, such as integers,
  doubles, enums and structured types, to name a few.
- A sample fundamental type implementation to base object hierarchies upon -
  the GObject fundamental type.
- A signal system that allows very flexible user customization of
  virtual/overridable object methods and can serve as a powerful
  notification mechanism.
- An extensible parameter/value system, supporting all the provided
  fundamental types that can be used to generically handle object properties
  or otherwise parameterized types.

## Background

GObject, and its lower-level type system, GType, are used by GTK and most
GNOME libraries to provide:

- object-oriented C-based APIs and
- automatic transparent API bindings to other compiled or interpreted
  languages.

A lot of programmers are used to working with compiled-only or dynamically
interpreted-only languages and do not understand the challenges associated
with cross-language interoperability. This introduction tries to provide an
insight into these challenges and briefly describes the solution chosen by
GLib.

The following chapters go into greater detail into how GType and GObject
work and how you can use them as a C programmer. It is useful to keep in
mind that allowing access to C objects from other interpreted languages was
one of the major design goals: this can often explain the sometimes rather
convoluted APIs and features present in this library.

### Data types and programming

One could say that a programming language is merely a way to create data
types and manipulate them. Most languages provide a number of
language-native types and a few primitives to create more complex types
based on these primitive types.

In C, the language provides types such as char, long, pointer. During
compilation of C code, the compiler maps these language types to the
compiler's target architecture machine types. If you are using a C
interpreter (assuming one exists), the interpreter (the program which
interprets the source code and executes it) maps the language types to the
machine types of the target machine at runtime, during the program execution
(or just before execution if it uses a Just In Time compiler engine).

Perl and Python are interpreted languages which do not really provide type
definitions similar to those used by C. Perl and Python programmers
manipulate variables and the type of the variables is decided only upon the
first assignment or upon the first use which forces a type on the variable.
The interpreter also often provides a lot of automatic conversions from one
type to the other. For example, in Perl, a variable which holds an integer
can be automatically converted to a string given the required context:

```perl
my $tmp = 10;
print "this is an integer converted to a string:" . $tmp . "\n";
```

Of course, it is also often possible to explicitly specify conversions when
the default conversions provided by the language are not intuitive.

### Exporting a C API

C APIs are defined by a set of functions and global variables which are
usually exported from a binary. C functions have an arbitrary number of
arguments and one return value. Each function is thus uniquely identified by
the function name and the set of C types which describe the function
arguments and return value. The global variables exported by the API are
similarly identified by their name and their type.

A C API is thus merely defined by a set of names to which a set of types are
associated. If you know the function calling convention and the mapping of
the C types to the machine types used by the platform you are on, you can
resolve the name of each function to find where the code associated to this
function is located in memory, and then construct a valid argument list for
the function. Finally, all you have to do is trigger a call to the target C
function with the argument list.

For the sake of discussion, here is a sample C function and the associated
32 bit x86 assembly code generated by GCC on a Linux computer:

```c
static void
function_foo (int foo)
{
}

int
main (int   argc,
      char *argv[])
{
	function_foo (10);

	return 0;
}
```

```asm
push   $0xa
call   0x80482f4 <function_foo>
```

The assembly code shown above is pretty straightforward: the first
instruction pushes the hexadecimal value 0xa (decimal value 10) as a 32-bit
integer on the stack and calls `function_foo`. As you can see, C function
calls are implemented by GCC as native function calls (this is probably the
fastest implementation possible).

Now, let's say we want to call the C function `function_foo` from a Python
program. To do this, the Python interpreter needs to:

- Find where the function is located. This probably means finding the binary
  generated by the C compiler which exports this function.
- Load the code of the function in executable memory.
- Convert the Python parameters to C-compatible parameters before calling
  the function.
- Call the function with the right calling convention.
- Convert the return values of the C function to Python-compatible variables
  to return them to the Python code.

The process described above is pretty complex and there are a lot of ways to
make it entirely automatic and transparent to C and Python programmers:

- The first solution is to write by hand a lot of glue code, once for each
  function exported or imported, which does the Python-to-C parameter
  conversion and the C-to-Python return value conversion. This glue code is
  then linked with the interpreter which allows Python programs to call
  Python functions which delegate work to C functions.
- Another, nicer solution is to automatically generate the glue code, once
  for each function exported or imported, with a special compiler which
  reads the original function signature.

The solution used by GLib is to use the GType library which holds at runtime
a description of all the objects manipulated by the programmer. This
so-called dynamic type library is then used by special generic glue code to
automatically convert function parameters and function calling conventions
between different runtime domains.

The greatest advantage of the solution implemented by GType is that the glue
code sitting at the runtime domain boundaries is written once: the figure
below states this more clearly.

![](./glue.png)

Currently, there exist multiple generic glue code which makes it possible to
use C objects written with GType directly in a variety of languages, with a
minimum amount of work: there is no need to generate huge amounts of glue
code either automatically or by hand.

Although that goal was arguably laudable, its pursuit has had a major
influence on the whole GType/GObject library. C programmers are likely to be
puzzled at the complexity of the features exposed in the following chapters
if they forget that the GType/GObject library was not only designed to offer
OO-like features to C programmers but also transparent cross-language
interoperability.

## The GLib Dynamic Type System

A type, as manipulated by the GLib type system, is much more generic than what is usually understood as an Object type. It is best explained by looking at the structure and the functions used to register new types in the type system.

```c
typedef struct _GTypeInfo               GTypeInfo;
struct _GTypeInfo
{
  /* interface types, classed types, instantiated types */
  guint16                class_size;

  GBaseInitFunc          base_init;
  GBaseFinalizeFunc      base_finalize;

  /* classed types, instantiated types */
  GClassInitFunc         class_init;
  GClassFinalizeFunc     class_finalize;
  gconstpointer          class_data;

  /* instantiated types */
  guint16                instance_size;
  guint16                n_preallocs;
  GInstanceInitFunc      instance_init;

  /* value handling */
  const GTypeValueTable *value_table;
};

GType
g_type_register_static (GType            parent_type,
                        const gchar     *type_name,
                        const GTypeInfo *info,
                        GTypeFlags       flags);

GType
g_type_register_fundamental (GType                       type_id,
                             const gchar                *type_name,
                             const GTypeInfo            *info,
                             const GTypeFundamentalInfo *finfo,
                             GTypeFlags                  flags);
```

`g_type_register_static()`, `g_type_register_dynamic()` and
`g_type_register_fundamental()` are the C functions, defined in `gtype.h`
and implemented in `gtype.c` which you should use to register a new GType in
the program's type system. It is not likely you will ever need to use
`g_type_register_fundamental()` but in case you want to, the last chapter
explains how to create new fundamental types.

Fundamental types are top-level types which do not derive from any other
type while other non-fundamental types derive from other types. Upon
initialization, the type system not only initializes its internal data
structures but it also registers a number of core types: some of these are
fundamental types. Others are types derived from these fundamental types.

Fundamental and non-fundamental types are defined by:

- class size: the `class_size` field in `GTypeInfo`.
- class initialization functions (C++ constructor): the `base_init` and
  `class_init` fields in `GTypeInfo`.
- class destruction functions (C++ destructor): the `base_finalize` and
  `class_finalize` fields in `GTypeInfo`.
- instance size (C++ parameter to new): the `instance_size` field in
  `GTypeInfo`.
- instantiation policy (C++ type of new operator): the `n_preallocs` field
  in `GTypeInfo`.
- copy functions (C++ copy operators): the `value_table` field in
  `GTypeInfo`.
- type characteristic flags: `GTypeFlags`.

Fundamental types are also defined by a set of `GTypeFundamentalFlags` which
are stored in a `GTypeFundamentalInfo`. Non-fundamental types are
furthermore defined by the type of their parent which is passed as the
`parent_type` parameter to `g_type_register_static()` and
`g_type_register_dynamic()`.

## Copy functions

The major common point between all GLib types (fundamental and
non-fundamental, classed and non-classed, instantiatable and
non-instantiatable) is that they can all be manipulated through a single API
to copy/assign them.

The `GValue` structure is used as an abstract container for all of these
types. Its simplistic API (defined in `gobject/gvalue.h`) can be used to
invoke the `value_table` functions registered during type registration: for
example `g_value_copy()` copies the content of a `GValue` to another
`GValue`. This is similar to a C++ assignment which invokes the C++ copy
operator to modify the default bit-by-bit copy semantics of C++/C
structures/classes.

The following code shows how you can copy around a 64 bit integer, as well
as a `GObject` instance pointer:

```c
static void test_int (void)
{
  GValue a_value = G_VALUE_INIT;
  GValue b_value = G_VALUE_INIT;
  guint64 a, b;

  a = 0xdeadbeef;

  g_value_init (&a_value, G_TYPE_UINT64);
  g_value_set_uint64 (&a_value, a);

  g_value_init (&b_value, G_TYPE_UINT64);
  g_value_copy (&a_value, &b_value);

  b = g_value_get_uint64 (&b_value);

  if (a == b) {
    g_print ("Yay !! 10 lines of code to copy around a uint64.\n");
  } else {
    g_print ("Are you sure this is not a Z80 ?\n");
  }
}

static void test_object (void)
{
  GObject *obj;
  GValue obj_vala = G_VALUE_INIT;
  GValue obj_valb = G_VALUE_INIT;
  obj = g_object_new (VIEWER_TYPE_FILE, NULL);

  g_value_init (&obj_vala, VIEWER_TYPE_FILE);
  g_value_set_object (&obj_vala, obj);

  g_value_init (&obj_valb, G_TYPE_OBJECT);

  /* g_value_copy's semantics for G_TYPE_OBJECT types is to copy the reference.
   * This function thus calls g_object_ref.
   * It is interesting to note that the assignment works here because
   * VIEWER_TYPE_FILE is a G_TYPE_OBJECT.
   */
  g_value_copy (&obj_vala, &obj_valb);

  g_object_unref (G_OBJECT (obj));
  g_object_unref (G_OBJECT (obj));
}
```

The important point about the above code is that the exact semantics of the
copy calls is undefined since they depend on the implementation of the copy
function. Certain copy functions might decide to allocate a new chunk of
memory and then to copy the data from the source to the destination. Others
might want to simply increment the reference count of the instance and copy
the reference to the new `GValue`.

The value table used to specify these assignment functions is documented in
`GTypeValueTable`.

Interestingly, it is also very unlikely you will ever need to specify a
`value_table` during type registration because these `value_tables` are
inherited from the parent types for non-fundamental types.

## Conventions

There are a number of conventions users are expected to follow when creating
new types which are to be exported in a header file:

- Type names (including object names) must be at least three characters long
  and start with a–z, A–Z or ‘\_’.
- Use the `object_method` pattern for function names: to invoke the method
  named ‘save’ on an instance of object type ‘file’, call `file_save`.
- Use prefixing to avoid namespace conflicts with other projects. If your
  library (or application) is named `Viewer`, prefix all your function names
  with `viewer_`. For example: `viewer_file_save`.
- The prefix should be a single term, i.e. should not contain any capital
  letters after the first letter. For example, `Exampleprefix` rather than
  `ExamplePrefix`.
  - This allows the lowercase-with-underscores versions of names
    to be automatically and unambiguously generated from the camel-case
    versions. See [Name mangling](#name-mangling).
  - Multiple-term prefixes can be supported if additional arguments are passed
    to type and introspection tooling, but it’s best to avoid the need for this.
- Object/Class names (such as `File` in the examples here) may contain multiple
  terms. For example, `LocalFile`.
- Create a macro named `PREFIX_TYPE_OBJECT` which always returns the `GType`
  for the associated object type. For an object of type `File` in the
  `Viewer` namespace, use: `VIEWER_TYPE_FILE`. This macro is implemented
  using a function named `prefix_object_get_type`; for example,
  `viewer_file_get_type`.
- Use `G_DECLARE_FINAL_TYPE` or `G_DECLARE_DERIVABLE_TYPE` to define various
  other conventional macros for your object:
  - `PREFIX_OBJECT (obj)`, which returns a pointer of type `PrefixObject`.
    This macro is used to enforce static type safety by doing explicit casts
    wherever needed. It also enforces dynamic type safety by doing runtime
    checks. It is possible to disable the dynamic type checks in production
    builds (see [Building GLib](https://docs.gtk.org/glib/building.html)). For
    example, we would create `VIEWER_FILE (obj)` to keep the previous
    example.
  - `PREFIX_OBJECT_CLASS (klass)`, which is strictly equivalent to the
    previous casting macro: it does static casting with dynamic type
    checking of class structures. It is expected to return a pointer to a
    class structure of type `PrefixObjectClass`. An example is:
    `VIEWER_FILE_CLASS`.
  - `PREFIX_IS_OBJECT (obj)`, which returns a boolean which indicates
    whether the input object instance pointer is non-`NULL` and of type
    `OBJECT`. For example, `VIEWER_IS_FILE`.
  - `PREFIX_IS_OBJECT_CLASS (klass)`, which returns a boolean if the input
    class pointer is a pointer to a class of type `OBJECT`. For example,
    `VIEWER_IS_FILE_CLASS`.
  - `PREFIX_OBJECT_GET_CLASS (obj)`, which returns the class pointer
    associated to an instance of a given type. This macro is used for static
    and dynamic type safety purposes (just like the previous casting
    macros). For example, `VIEWER_FILE_GET_CLASS`.

The implementation of these macros is pretty straightforward: a number of
simple-to-use macros are provided in `gtype.h`. For the example we used above,
we would write the following trivial code to declare the macros:

```c
#define VIEWER_TYPE_FILE viewer_file_get_type()
G_DECLARE_FINAL_TYPE (ViewerFile, viewer_file, VIEWER, FILE, GObject)
```

Unless your code has special requirements, you can use the `G_DEFINE_TYPE`
macro to define a class:

```c
G_DEFINE_TYPE (ViewerFile, viewer_file, G_TYPE_OBJECT)
```

Otherwise, the `viewer_file_get_type` function must be implemented manually:

```c
GType viewer_file_get_type (void)
{
  static GType type = 0;
  if (type == 0) {
    const GTypeInfo info = {
      /* You fill this structure. */
    };
    type = g_type_register_static (G_TYPE_OBJECT,
                                   "ViewerFile",
                                   &info, 0);
  }
  return type;
}
```

## Name mangling

GObject tooling, in particular introspection, relies on being able to
unambiguously convert between type names (in camel-case, such as
`GNetworkMonitor` or `MyViewerFile`) and function prefixes (in
lowercase-with-underscores, such as `g_network_monitor` or `my_viewer_file`).
The latter can then be used to prefix methods such as
`g_network_monitor_can_reach()` or `my_viewer_file_get_type()`.

The algorithm for converting from camel-case to lowercase-with-underscores is:
<!-- This algorithm is here:
     https://gitlab.gnome.org/GNOME/gtk/-/blob/a118cbb3ff552d4258d433e67bdb94e8aef0f439/gtk/gtkbuilderscope.c#L195-229
     We’ve ignored the case of (split_first_cap == TRUE) to simplify things, and
     because that functionality is discouraged. -->

 1. The output is a lower-case version of the input, with zero or more ‘splits’.
 2. Wherever the input is ‘split’, insert an underscore in the output.
 3. Split the input on:
    - Each character (after index zero) which is uppercase and the previous
      character is not uppercase
    - The second character (index one) if it is uppercase and the first character
      (index zero) is uppercase
    - Each character (after index two) which is uppercase and the previous two
      characters are also uppercase

## Non-instantiatable non-classed fundamental types

A lot of types are not instantiatable by the type system and do not have a class. Most of these types are fundamental trivial types such as `gchar`, and are already registered by GLib.

In the rare case of needing to register such a type in the type system, fill a `GTypeInfo` structure with zeros since these types are also most of the time fundamental:

```c
GTypeInfo info = {
  .class_size = 0,

  .base_init = NULL,
  .base_finalize = NULL,

  .class_init = NULL,
  .class_finalize = NULL,
  .class_data = NULL,

  .instance_size = 0,
  .n_preallocs = 0,
  .instance_init = NULL,

  .value_table = NULL,
};

static const GTypeValueTable value_table = {
  .value_init = value_init_long0,
  .value_free = NULL,
  .value_copy = value_copy_long0,
  .value_peek_pointer = NULL,

  .collect_format = "i",
  .collect_value = value_collect_int,
  .lcopy_format = "p",
  .lcopy_value = value_lcopy_char,
};

info.value_table = &value_table;

type = g_type_register_fundamental (G_TYPE_CHAR, "gchar", &info, &finfo, 0);
```

Having non-instantiatable types might seem a bit useless: what good is a
type if you cannot instantiate an instance of that type? Most of these types
are used in conjunction with `GValue`s: a `GValue` is initialized with an
integer or a string and it is passed around by using the registered type's
`value_table`. `GValue`s (and by extension these trivial fundamental types)
are most useful when used in conjunction with object properties and signals.

## Instantiatable classed types: objects

Types which are registered with a class and are declared instantiatable are
what most closely resembles an object. Although `GObject`s are the most well
known type of instantiatable classed types, other kinds of similar objects
used as the base of an inheritance hierarchy have been externally developed
and they are all built on the fundamental features described below.

For example, the code below shows how you could register such a fundamental
object type in the type system (using none of the GObject convenience API):

```c
typedef struct {
  GObject parent_instance;

  /* instance members */
  char *filename;
} ViewerFile;

typedef struct {
  GObjectClass parent_class;

  /* class members */

  /* the first is public, pure and virtual */
  void (*open)  (ViewerFile  *self,
                 GError     **error);

  /* the second is public and virtual */
  void (*close) (ViewerFile  *self,
                 GError     **error);
} ViewerFileClass;

#define VIEWER_TYPE_FILE (viewer_file_get_type ())

GType
viewer_file_get_type (void)
{
  static GType type = 0;
  if (type == 0) {
    const GTypeInfo info = {
      .class_size = sizeof (ViewerFileClass),
      .base_init = NULL,
      .base_finalize = NULL,
      .class_init = (GClassInitFunc) viewer_file_class_init,
      .class_finalize = NULL,
      .class_data = NULL,
      .instance_size = sizeof (ViewerFile),
      .n_preallocs = 0,
      .instance_init = (GInstanceInitFunc) viewer_file_init,
    };
    type = g_type_register_static (G_TYPE_OBJECT,
                                   "ViewerFile",
                                   &info, 0);
  }
  return type;
}
```

Upon the first call to `viewer_file_get_type`, the type named `ViewerFile` will be registered in the type system as inheriting from the type `G_TYPE_OBJECT`.

Every object must define two structures: its class structure and its instance structure. All class structures must contain as first member a `GTypeClass` structure. All instance structures must contain as first member a `GTypeInstance` structure. The declaration of these C types, coming from `gtype.h` is shown below:

```c
struct _GTypeClass
{
  GType g_type;
};

struct _GTypeInstance
{
  GTypeClass *g_class;
};
```

These constraints allow the type system to make sure that every object instance (identified by a pointer to the object's instance structure) contains in its first bytes a pointer to the object's class structure.

This relationship is best explained by an example: let's take object B which inherits from object A:

```c
/* A definitions */
typedef struct {
  GTypeInstance parent;
  int field_a;
  int field_b;
} A;

typedef struct {
  GTypeClass parent_class;
  void (*method_a) (void);
  void (*method_b) (void);
} AClass;

/* B definitions. */
typedef struct {
  A parent;
  int field_c;
  int field_d;
} B;

typedef struct {
  AClass parent_class;
  void (*method_c) (void);
  void (*method_d) (void);
} BClass;
```

The C standard mandates that the first field of a C structure is stored starting in the first byte of the buffer used to hold the structure's fields in memory. This means that the first field of an instance of an object B is A's first field which in turn is `GTypeInstance`'s first field which in turn is `g_class`, a pointer to B's class structure.

Thanks to these simple conditions, it is possible to detect the type of every object instance by doing:

```c
B *b;
b->parent.parent.g_class->g_type
```

or, more compactly:

```c
B *b;
((GTypeInstance *) b)->g_class->g_type
```

### Initialization and destruction

Instantiation of these types can be done with `g_type_create_instance()`, which
will look up the type information structure associated with the type
requested. Then, the instance size and instantiation policy (if the
`n_preallocs` field is set to a non-zero value, the type system allocates the
object's instance structures in chunks rather than mallocing for every
instance) declared by the user are used to get a buffer to hold the object's
instance structure.

If this is the first instance of the object ever created, the type system
must create a class structure. It allocates a buffer to hold the object's
class structure and initializes it. The first part of the class structure
(ie: the embedded parent class structure) is initialized by copying the
contents from the class structure of the parent class. The rest of class
structure is initialized to zero. If there is no parent, the entire class
structure is initialized to zero. The type system then invokes the
`base_init` functions `(GBaseInitFunc)` from topmost fundamental
object to bottom-most most derived object. The object's `class_init`
`(GClassInitFunc)` function is invoked afterwards to complete initialization
of the class structure. Finally, the object's interfaces are initialized (we
will discuss interface initialization in more detail later).

Once the type system has a pointer to an initialized class structure, it
sets the object's instance class pointer to the object's class structure and
invokes the object's `instance_init` `(GInstanceInitFunc)` functions, from
top-most fundamental type to bottom-most most-derived type.

Object instance destruction through `g_type_free_instance()` is very simple:
the instance structure is returned to the instance pool if there is one and
if this was the last living instance of the object, the class is destroyed.

Class destruction (the concept of destruction is sometimes partly referred
to as finalization in GType) is the symmetric process of the initialization:
interfaces are destroyed first. Then, the most derived `class_finalize`
`(GClassFinalizeFunc)` function is invoked. Finally, the `base_finalize`
`(GBaseFinalizeFunc)` functions are invoked from bottom-most most-derived type
to top-most fundamental type and the class structure is freed.

The base initialization/finalization process is very similar to the C++
constructor/destructor paradigm. The practical details are different though
and it is important not to get confused by superficial similarities. GTypes
have no instance destruction mechanism. It is the user's responsibility to
implement correct destruction semantics on top of the existing GType code.
(This is what `GObject` does) Furthermore, C++
code equivalent to the `base_init` and `class_init` callbacks of GType is
usually not needed because C++ cannot really create object types at runtime.

The instantiation/finalization process can be summarized as follows:

<!-- Markdown's tables cannot deal with multiple row spans -->
<table border="1" class="table" summary="GType Instantiation/Finalization">
  <colgroup>
  <col align="left"></col>
  <col align="left"></col>
  <col align="left"></col>
  </colgroup>
  <thead><tr>
  <th align="left">Invocation time</th>
  <th align="left">Function invoked</th>
  <th align="left">Function's parameters</th>
  </tr></thead>
  <tbody>
  <tr>
  <td align="left" rowspan="3">First call to <code class="function">g_type_create_instance()</code> for target type</td>
  <td align="left">type's <code class="function">base_init</code> function</td>
  <td align="left">On the inheritance tree of classes from fundamental type to target type. <code class="function">base_init</code> is invoked once for each class structure.</td>
  </tr>
  <tr>
  <td align="left">target type's <code class="function">class_init</code> function</td>
  <td align="left">On target type's class structure</td>
  </tr>
  <tr>
  <td align="left">interface initialization, see the section called “Interface Initialization”</td>
  <td align="left"></td>
  </tr>
  <tr>
  <td align="left">Each call to <code class="function">g_type_create_instance()</code> for target type</td>
  <td align="left">target type's <code class="function">instance_init</code> function</td>
  <td align="left">On object's instance</td>
  </tr>
  <tr>
  <td align="left" rowspan="3">Last call to <code class="function">g_type_free_instance()</code> for target type</td>
  <td align="left">interface destruction, see the section called “Interface Destruction”</td>
  <td align="left"></td>
  </tr>
  <tr>
  <td align="left">target type's <code class="function">class_finalize</code> function</td>
  <td align="left">On target type's class structure</td>
  </tr>
  <tr>
  <td align="left">type's <code class="function">base_finalize</code> function</td>
  <td align="left">On the inheritance tree of classes from fundamental type to target type. <code class="function">base_finalize</code> is invoked once for each class structure.</td>
  </tr>
  </tbody>
</table>

## Non-instantiatable classed types: interfaces

GType's interfaces are very similar to Java's interfaces. They allow to
describe a common API that several classes will adhere to. Imagine the play,
pause and stop buttons on hi-fi equipment—those can be seen as a playback
interface. Once you know what they do, you can control your CD player, MP3
player or anything that uses these symbols.

To declare an interface you have to register a non-instantiatable classed
type which derives from `GTypeInterface`. The following piece of code declares
such an interface:

```c
#define VIEWER_TYPE_EDITABLE viewer_editable_get_type ()
G_DECLARE_INTERFACE (ViewerEditable, viewer_editable, VIEWER, EDITABLE, GObject)

struct _ViewerEditableInterface {
  GTypeInterface parent;

  void (*save) (ViewerEditable  *self,
                GError         **error);
};

void viewer_editable_save (ViewerEditable  *self,
                           GError         **error);
```

The interface function, `viewer_editable_save` is implemented in a pretty
simple way:

```c
void
viewer_editable_save (ViewerEditable  *self,
                      GError         **error)
{
  ViewerEditableinterface *iface;

  g_return_if_fail (VIEWER_IS_EDITABLE (self));
  g_return_if_fail (error == NULL || *error == NULL);

  iface = VIEWER_EDITABLE_GET_IFACE (self);
  g_return_if_fail (iface->save != NULL);
  iface->save (self);
}
```

`viewer_editable_get_type` registers a type named `ViewerEditable` which
inherits from `G_TYPE_INTERFACE`. All interfaces must be children of
`G_TYPE_INTERFACE` in the inheritance tree.

An interface is defined by only one structure which must contain as first
member a `GTypeInterface` structure. The interface structure is expected to
contain the function pointers of the interface methods. It is good style to
define helper functions for each of the interface methods which simply call
the interface's method directly: `viewer_editable_save` is one of these.

If you have no special requirements you can use the `G_IMPLEMENT_INTERFACE`
macro to implement an interface:

```c
static void
viewer_file_save (ViewerEditable *self)
{
  g_print ("File implementation of editable interface save method.\n");
}

static void
viewer_file_editable_interface_init (ViewerEditableInterface *iface)
{
  iface->save = viewer_file_save;
}

G_DEFINE_TYPE_WITH_CODE (ViewerFile, viewer_file, VIEWER_TYPE_FILE,
                         G_IMPLEMENT_INTERFACE (VIEWER_TYPE_EDITABLE,
                                                viewer_file_editable_interface_init))
```

If your code does have special requirements, you must write a custom
`get_type` function to register your GType which inherits from some GObject
and which implements the interface `ViewerEditable`. For example, this code
registers a new `ViewerFile` class which implements `ViewerEditable`:

```c
static void
viewer_file_save (ViewerEditable *editable)
{
  g_print ("File implementation of editable interface save method.\n");
}

static void
viewer_file_editable_interface_init (gpointer g_iface,
                                     gpointer iface_data)
{
  ViewerEditableInterface *iface = g_iface;

  iface->save = viewer_file_save;
}

GType
viewer_file_get_type (void)
{
  static GType type = 0;
  if (type == 0) {
    const GTypeInfo info = {
      .class_size = sizeof (ViewerFileClass),
      .base_init = NULL,
      .base_finalize = NULL,
      .class_init = (GClassInitFunc) viewer_file_class_init,
      .class_finalize = NULL,
      .class_data = NULL,
      .instance_size = sizeof (ViewerFile),
      .n_preallocs = 0,
      .instance_init = (GInstanceInitFunc) viewer_file_init
    };

    const GInterfaceInfo editable_info = {
      .interface_init = (GInterfaceInitFunc) viewer_file_editable_interface_init,
      .interface_finalize = NULL,
      .interface_data = NULL,
    };

    type = g_type_register_static (VIEWER_TYPE_FILE,
                                   "ViewerFile",
                                   &info, 0);

    g_type_add_interface_static (type,
                                 VIEWER_TYPE_EDITABLE,
                                 &editable_info);
  }
  return type;
}
```

`g_type_add_interface_static()` records in the type system that the given
`ViewerFile` type implements also `ViewerEditable`
(`viewer_editable_get_type()` returns the type of `ViewerEditable`). The
`GInterfaceInfo` structure holds information about the implementation of the
interface:

```c
struct _GInterfaceInfo
{
  GInterfaceInitFunc     interface_init;
  GInterfaceFinalizeFunc interface_finalize;
  gpointer               interface_data;
};
```

### Interface initialization

When an instantiatable classed type which implements an interface (either
directly or by inheriting an implementation from a superclass) is created
for the first time, its class structure is initialized following the process
described in the section called "Instantiatable classed types: objects".
After that, the interface implementations associated with the type are
initialized.

First a memory buffer is allocated to hold the interface structure. The
parent's interface structure is then copied over to the new interface
structure (the parent interface is already initialized at that point). If
there is no parent interface, the interface structure is initialized with
zeros. The `g_type` and the `g_instance_type` fields are then initialized:
`g_type` is set to the type of the most-derived interface and `g_instance_type`
is set to the type of the most derived type which implements this interface.

The interface's `base_init` function is called, and then the interface's
`default_init` is invoked. Finally if the type has registered an
implementation of the interface, the implementation's `interface_init`
function is invoked. If there are multiple implementations of an interface
the `base_init` and `interface_init` functions will be invoked once for each
implementation initialized.

It is thus recommended to use a `default_init` function to initialize an
interface. This function is called only once for the interface no matter how
many implementations there are. The `default_init` function is declared by
`G_DEFINE_INTERFACE` which can be used to define the interface:

```c
G_DEFINE_INTERFACE (ViewerEditable, viewer_editable, G_TYPE_OBJECT)

static void
viewer_editable_default_init (ViewerEditableInterface *iface)
{
  /* add properties and signals here, will only be called once */
}
```

Or you can do that yourself in a GType function for your interface:

```c
GType
viewer_editable_get_type (void)
{
  static gsize type_id = 0;
  if (g_once_init_enter (&type_id)) {
    const GTypeInfo info = {
      sizeof (ViewerEditableInterface),
      NULL,   /* base_init */
      NULL,   /* base_finalize */
      viewer_editable_default_init, /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      0,      /* instance_size */
      0,      /* n_preallocs */
      NULL    /* instance_init */
    };
    GType type = g_type_register_static (G_TYPE_INTERFACE,
                                         "ViewerEditable",
                                         &info, 0);
    g_once_init_leave (&type_id, type);
  }
  return type_id;
}

static void
viewer_editable_default_init (ViewerEditableInterface *iface)
{
  /* add properties and signals here, will only called once */
}
```

In summary, interface initialization uses the following functions:

<table border="1" class="table" summary="Interface Initialization">
  <colgroup>
  <col align="left"></col>
  <col align="left"></col>
  <col align="left"></col>
  </colgroup>
  <thead><tr>
  <th align="left">Invocation time</th>
  <th align="left">Function Invoked</th>
  <th align="left">Function's parameters</th>
  <th>Remark</th>
  </tr></thead>
  <tbody>
  <tr>
  <td align="left">First call to <code class="function">g_type_create_instance()</code> for <span class="emphasis"><em>any</em></span> type implementing interface</td>
  <td align="left">interface's <code class="function">base_init</code> function</td>
  <td align="left">On interface's vtable</td>
  <td>Rarely necessary to use this. Called once per instantiated classed type implementing the interface.</td>
  </tr>
  <tr>
  <td align="left">First call to <code class="function">g_type_create_instance()</code> for <span class="emphasis"><em>each</em></span> type implementing interface</td>
  <td align="left">interface's <code class="function">default_init</code> function</td>
  <td align="left">On interface's vtable</td>
  <td>Register interface's signals, properties, etc. here. Will be called once.</td>
  </tr>
  <tr>
  <td align="left">First call to <code class="function">g_type_create_instance()</code> for <span class="emphasis"><em>any</em></span> type implementing interface</td>
  <td align="left">implementation's <code class="function">interface_init</code> function</td>
  <td align="left">On interface's vtable</td>
  <td>Initialize interface implementation. Called for each class that that implements the interface. Initialize the interface method pointers in the interface structure to the implementing class's implementation.</td>
  </tr>
  </tbody>
</table>

### Interface Destruction

When the last instance of an instantiatable type which registered an
interface implementation is destroyed, the interface's implementations
associated to the type are destroyed.

To destroy an interface implementation, GType first calls the
implementation's `interface_finalize` function and then the interface's
most-derived `base_finalize` function.

Again, it is important to understand, as in the section called "Interface
Initialization", that both `interface_finalize` and `base_finalize` are
invoked exactly once for the destruction of each implementation of an
interface. Thus, if you were to use one of these functions, you would need
to use a static integer variable which would hold the number of instances of
implementations of an interface such that the interface's class is destroyed
only once (when the integer variable reaches zero).

The above process can be summarized as follows: 

<table border="1" class="table" summary="Interface Finalization">
  <colgroup>
  <col align="left"></col>
  <col align="left"></col>
  <col align="left"></col>
  </colgroup>
  <thead><tr>
  <th align="left">Invocation time</th>
  <th align="left">Function Invoked</th>
  <th align="left">Function's parameters</th>
  </tr></thead>
  <tbody>
  <tr>
  <td align="left" rowspan="2">Last call to <code class="function">g_type_free_instance()</code> for type implementing interface</td>
  <td align="left">interface's <code class="function">interface_finalize</code> function</td>
  <td align="left">On interface's vtable</td>
  </tr>
  <tr>
  <td align="left">interface's <code class="function">base_finalize</code> function</td>
  <td align="left">On interface's vtable</td>
  </tr>
  </tbody>
</table>

## The GObject base class

The previous chapter discussed the details of GLib's Dynamic Type System.
The GObject library also contains an implementation for a base fundamental
type named `GObject`.

`GObject` is a fundamental classed instantiatable type. It implements:

- memory management with reference counting
- construction/Destruction of instances
- generic per-object properties with set/get function pairs
- easy use of signals

All the GNOME libraries which use the GLib type system (like GTK and
GStreamer) inherit from `GObject` which is why it is important to understand
the details of how it works.

### Object instantiation

The `g_object_new()` family of functions can be used to instantiate any
GType which inherits from the GObject base type. All these functions make
sure the class and instance structures have been correctly initialized by
GLib's type system and then invoke at one point or another the constructor
class method which is used to:

- allocate and clear memory through `g_type_create_instance()`
- initialize the object's instance with the construction properties.

GObject explicitly guarantees that all class and instance members (except
the fields pointing to the parents) to be set to zero.

Once all construction operations have been completed and constructor
properties set, the constructed class method is called.

Objects which inherit from `GObject` are allowed to override this
constructed class method. The example below shows how `ViewerFile` overrides
the parent's construction process:

```c
#define VIEWER_TYPE_FILE viewer_file_get_type ()
G_DECLARE_FINAL_TYPE (ViewerFile, viewer_file, VIEWER, FILE, GObject)

struct _ViewerFile
{
  GObject parent_instance;

  /* instance members */
  char *filename;
  guint zoom_level;
};

/* will create viewer_file_get_type and set viewer_file_parent_class */
G_DEFINE_TYPE (ViewerFile, viewer_file, G_TYPE_OBJECT)

static void
viewer_file_constructed (GObject *obj)
{
  /* update the object state depending on constructor properties */

  /* Always chain up to the parent constructed function to complete object
   * initialization. */
  G_OBJECT_CLASS (viewer_file_parent_class)->constructed (obj);
}

static void
viewer_file_finalize (GObject *obj)
{
  ViewerFile *self = VIEWER_FILE (obj);

  g_free (self->filename);

  /* Always chain up to the parent finalize function to complete object
   * destruction. */
  G_OBJECT_CLASS (viewer_file_parent_class)->finalize (obj);
}

static void
viewer_file_class_init (ViewerFileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = viewer_file_constructed;
  object_class->finalize = viewer_file_finalize;
}

static void
viewer_file_init (ViewerFile *self)
{
  /* initialize the object */
}
```

If the user instantiates an object `ViewerFile` with:

```c
ViewerFile *file = g_object_new (VIEWER_TYPE_FILE, NULL);
```

If this is the first instantiation of such an object, the
`viewer_file_class_init` function will be invoked after any
`viewer_file_base_class_init` function. This will make sure the class
structure of this new object is correctly initialized. Here,
`viewer_file_class_init` is expected to override the object's class methods
and setup the class' own methods. In the example above, the constructed
method is the only overridden method: it is set to
`viewer_file_constructed`.

Once `g_object_new()` has obtained a reference to an initialized class
structure, it invokes its constructor method to create an instance of the
new object, if the constructor has been overridden in
`viewer_file_class_init`. Overridden constructors must chain up to their
parent’s constructor. In order to find the parent class and chain up to the
parent class constructor, we can use the `viewer_file_parent_class` pointer
that has been set up for us by the `G_DEFINE_TYPE` macro.

Finally, at one point or another, `g_object_constructor` is invoked by the
last constructor in the chain. This function allocates the object's instance
buffer through `g_type_create_instance()` which means that the
`instance_init` function is invoked at this point if one was registered.
After `instance_init` returns, the object is fully initialized and should be
ready to have its methods called by the user. When
`g_type_create_instance()` returns, `g_object_constructor` sets the
construction properties (i.e. the properties which were given to
`g_object_new()`) and returns to the user's constructor.

The process described above might seem a bit complicated, but it can be
summarized easily by the table below which lists the functions invoked by
`g_object_new()` and their order of invocation:

<!-- Markdown's tables do not allow multiple row spans -->
<table border="1" class="table" summary="g_object_new">
  <colgroup>
  <col align="left"></col>
  <col align="left"></col>
  <col align="left"></col>
  </colgroup>
  <thead><tr>
  <th align="left">Invocation time</th>
  <th align="left">Function invoked</th>
  <th align="left">Function's parameters</th>
  <th>Remark</th>
  </tr></thead>
  <tbody>
  <tr>
  <td align="left" rowspan="4">First call to <code class="function">g_object_new()</code> for target type</td>
  <td align="left">target type's <code class="function">base_init</code> function</td>
  <td align="left">On the inheritance tree of classes from fundamental type to target type. <code class="function">base_init</code> is invoked once for each class structure.</td>
  <td>Never used in practice. Unlikely you will need it.</td>
  </tr>
  <tr>
  <td align="left">target type's <code class="function">class_init</code> function</td>
  <td align="left">On target type's class structure</td>
  <td>Here, you should make sure to initialize or override class methods (that is, assign to each class' method its function pointer) and create the signals and the properties associated to your object.</td>
  </tr>
  <tr>
  <td align="left">interface's <code class="function">base_init</code> function</td>
  <td align="left">On interface's vtable</td>
  <td></td>
  </tr>
  <tr>
  <td align="left">interface's <code class="function">interface_init</code> function</td>
  <td align="left">On interface's vtable</td>
  <td></td>
  </tr>
  <tr>
  <td align="left" rowspan="3">Each call to <code class="function">g_object_new()</code> for target type</td>
  <td align="left">target type's class <code class="function">constructor</code> method: <code class="function">GObjectClass-&gt;constructor</code>
  </td>
  <td align="left">On object's instance</td>
  <td>If you need to handle construct properties in a custom way, or implement a singleton class, override the constructor method and make sure to chain up to the object's parent class before doing your own initialization. In doubt, do not override the constructor method.</td>
  </tr>
  <tr>
  <td align="left">type's <code class="function">instance_init</code> function</td>
  <td align="left">On the inheritance tree of classes from fundamental type to target type. The <code class="function">instance_init</code> provided for each type is invoked once for each instance structure.</td>
  <td>Provide an <code class="function">instance_init</code> function to initialize your object before its construction properties are set. This is the preferred way to initialize a GObject instance. This function is equivalent to C++ constructors.</td>
  </tr>
  <tr>
  <td align="left">target type's class <code class="function">constructed</code> method: <code class="function">GObjectClass-&gt;constructed</code></td>
  <td align="left">On object's instance</td>
  <td>If you need to perform object initialization steps after all construct properties have been set. This is the final step in the object initialization process, and is only called if the <code class="function">constructor</code> method returned a new object instance (rather than, for example, an existing singleton).</td>
  </tr>
  </tbody>
</table>

Readers should feel concerned about one little twist in the order in which
functions are invoked: while, technically, the class' constructor method is
called before the GType's `instance_init` function (since
`g_type_create_instance()` which calls `instance_init` is called by
`g_object_constructor` which is the top-level class constructor method and
to which users are expected to chain to), the user's code which runs in a
user-provided constructor will always run after GType's `instance_init`
function since the user-provided constructor must (you've been warned) chain
up before doing anything useful.

### Object memory management

The memory-management API for GObjects is a bit complicated but the idea
behind it is pretty simple: the goal is to provide a flexible model based on
reference counting which can be integrated in applications which use or
require different memory management models (such as garbage collection). The
methods which are used to manipulate this reference count are described
below.

#### Reference count

The functions `g_object_ref()` and `g_object_unref()` increase and decrease
the reference count, respectively. These functions are thread-safe.
`g_clear_object()` is a convenience wrapper around `g_object_unref()` which
also clears the pointer passed to it.

The reference count is initialized to one by `g_object_new()` which means
that the caller is currently the sole owner of the newly-created reference.
(If the object is derived from `GInitiallyUnowned`, this reference is
"floating", and must be "sunk", i.e. transformed into a real reference.)
When the reference count reaches zero, that is, when `g_object_unref()` is
called by the last owner of a reference to the object, the `dispose()` and
the `finalize()` class methods are invoked.

Finally, after `finalize()` is invoked, `g_type_free_instance()` is called
to free the object instance. Depending on the memory allocation policy
decided when the type was registered (through one of the `g_type_register_*`
functions), the object's instance memory will be freed or returned to the
object pool for this type. Once the object has been freed, if it was the
last instance of the type, the type's class will be destroyed as described
in the section called "Instantiatable classed types: objects" and the
section called "Non-instantiatable classed types: interfaces".

The table below summarizes the destruction process of a `GObject`:

<table border="1" class="table" summary="g_object_unref">
  <colgroup>
  <col align="left"></col>
  <col align="left"></col>
  <col align="left"></col>
  </colgroup>
  <thead><tr>
  <th align="left">Invocation time</th>
  <th align="left">Function invoked</th>
  <th align="left">Function's parameters</th>
  <th>Remark</th>
  </tr></thead>
  <tbody>
  <tr>
  <td align="left" rowspan="2">Last call to <code class="function">g_object_unref()</code> for an instance of target type</td>
  <td align="left">target type's dispose class function</td>
  <td align="left">GObject instance</td>
  <td>When dispose ends, the object should not hold any reference to any other member object. The object is also expected to be able to answer client method invocations (with possibly an error code but no memory violation) until finalize is executed. dispose can be executed more than once. dispose should chain up to its parent implementation just before returning to the caller.</td>
  </tr>
  <tr>
  <td align="left">target type's finalize class function</td>
  <td align="left">GObject instance</td>
  <td>Finalize is expected to complete the destruction process initiated by dispose. It should complete the object's destruction. finalize will be executed only once. finalize should chain up to its parent implementation just before returning to the caller. See the section on "Reference counts and cycles" for more information.</td>
  </tr>
  <tr>
  <td align="left" rowspan="4">Last call to <code class="function">g_object_unref()</code> for the last instance of target type</td>
  <td align="left">interface's <code class="function">interface_finalize</code> function</td>
  <td align="left">On interface's vtable</td>
  <td>Never used in practice. Unlikely you will need it.</td>
  </tr>
  <tr>
  <td align="left">interface's <code class="function">base_finalize</code> function</td>
  <td align="left">On interface's vtable</td>
  <td>Never used in practice. Unlikely you will need it.</td>
  </tr>
  <tr>
  <td align="left">target type's <code class="function">class_finalize</code> function</td>
  <td align="left">On target type's class structure</td>
  <td>Never used in practice. Unlikely you will need it.</td>
  </tr>
  <tr>
  <td align="left">type's <code class="function">base_finalize</code> function</td>
  <td align="left">On the inheritance tree of classes from fundamental type to target type. <code class="function">base_init</code> is invoked once for each class structure.</td>
  <td>Never used in practice. Unlikely you will need it.</td>
  </tr>
  </tbody>
</table>

#### Weak References

Weak references are used to monitor object finalization:
`g_object_weak_ref()` adds a monitoring callback which does not hold a
reference to the object but which is invoked when the object runs its
dispose method. Weak references on the object are automatically dropped when
the instance is disposed, so there is no need to invoke `g_object_weak_unref()`
from the `GWeakNotify` callback. Remember that the object instance is not
passed to the `GWeakNotify` callback because the object has already been
disposed. Instead, the callback receives a pointer to where the object
previously was.

Weak references are also used to implement `g_object_add_weak_pointer()` and
`g_object_remove_weak_pointer()`. These functions add a weak reference to
the object they are applied to which makes sure to nullify the pointer given
by the user when object is finalized.

Similarly, `GWeakRef` can be used to implement weak references if thread
safety is required.

#### Reference counts and cycles

GObject's memory management model was designed to be easily integrated in
existing code using garbage collection. This is why the destruction process
is split in two phases: the first phase, executed in the `dispose()` handler is
supposed to release all references to other member objects. The second
phase, executed by the `finalize()` handler is supposed to complete the object's
destruction process. Object methods should be able to run without program
error in-between the two phases.

This two-step destruction process is very useful to break reference counting
cycles. While the detection of the cycles is up to the external code, once
the cycles have been detected, the external code can invoke
`g_object_run_dispose()` which will indeed break any existing cycles since
it will run the dispose handler associated to the object and thus release
all references to other objects.

This explains one of the rules about the `dispose()` handler stated earlier:
the `dispose()` handler can be invoked multiple times. Let's say we have a
reference count cycle: object A references B which itself references object
A. Let's say we have detected the cycle and we want to destroy the two
objects. One way to do this would be to invoke `g_object_run_dispose()` on one
of the objects.

If object A releases all its references to all objects, this means it
releases its reference to object B. If object B was not owned by anyone
else, this is its last reference count which means this last unref runs B's
dispose handler which, in turn, releases B's reference on object A. If this
is A's last reference count, this last unref runs A's dispose handler which
is running for the second time before A's finalize handler is invoked!

The above example, which might seem a bit contrived, can really happen if
GObjects are being handled by language bindings—hence the rules for object
destruction should be closely followed.

### Object properties

One of GObject's nice features is its generic get/set mechanism for object
properties. When an object is instantiated, the object's `class_init`
handler should be used to register the object's properties with
`g_object_class_install_properties()`.

The best way to understand how object properties work is by looking at a
real example of how it is used:

```c
// Implementation

typedef enum
{
  PROP_FILENAME = 1,
  PROP_ZOOM_LEVEL,
  N_PROPERTIES
} ViewerFileProperty;

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
viewer_file_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  ViewerFile *self = VIEWER_FILE (object);

  switch ((ViewerFileProperty) property_id)
    {
    case PROP_FILENAME:
      g_free (self->filename);
      self->filename = g_value_dup_string (value);
      g_print ("filename: %s\n", self->filename);
      break;

    case PROP_ZOOM_LEVEL:
      self->zoom_level = g_value_get_uint (value);
      g_print ("zoom level: %u\n", self->zoom_level);
      break;

    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
viewer_file_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  ViewerFile *self = VIEWER_FILE (object);

  switch ((ViewerFileProperty) property_id)
    {
    case PROP_FILENAME:
      g_value_set_string (value, self->filename);
      break;

    case PROP_ZOOM_LEVEL:
      g_value_set_uint (value, self->zoom_level);
      break;

    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
viewer_file_class_init (ViewerFileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = viewer_file_set_property;
  object_class->get_property = viewer_file_get_property;

  obj_properties[PROP_FILENAME] =
    g_param_spec_string ("filename",
                         "Filename",
                         "Name of the file to load and display from.",
                         NULL  /* default value */,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  obj_properties[PROP_ZOOM_LEVEL] =
    g_param_spec_uint ("zoom-level",
                       "Zoom level",
                       "Zoom level to view the file at.",
                       0  /* minimum value */,
                       10 /* maximum value */,
                       2  /* default value */,
                       G_PARAM_READWRITE);

  g_object_class_install_properties (object_class,
                                     N_PROPERTIES,
                                     obj_properties);
}
```

```c
// Use

ViewerFile *file;
GValue val = G_VALUE_INIT;

file = g_object_new (VIEWER_TYPE_FILE, NULL);

g_value_init (&val, G_TYPE_UINT);
g_value_set_char (&val, 11);

g_object_set_property (G_OBJECT (file), "zoom-level", &val);

g_value_unset (&val);
```

The client code above looks simple but a lot of things happen under the hood:

`g_object_set_property()` first ensures a property with this name was
registered in file's `class_init` handler. If so it walks the class
hierarchy, from bottom-most most-derived type, to top-most fundamental type
to find the class which registered that property. It then tries to convert
the user-provided GValue into a GValue whose type is that of the associated
property.

If the user provides a signed char GValue, as is shown here, and if the
object's property was registered as an `unsigned int`, `g_value_transform()`
will try to transform the input signed char into an unsigned int. Of course,
the success of the transformation depends on the availability of the
required transform function. In practice, there will almost always be a
transformation which matches and conversion will be carried out if needed.

After transformation, the GValue is validated by `g_param_value_validate()`
which makes sure the user's data stored in the GValue matches the
characteristics specified by the property's GParamSpec. Here, the GParamSpec
we provided in `class_init` has a validation function which makes sure that
the GValue contains a value which respects the minimum and maximum bounds of
the GParamSpec. In the example above, the client's GValue does not respect
these constraints (it is set to 11, while the maximum is 10). As such, the
`g_object_set_property()` function will return with an error.

If the user's GValue had been set to a valid value,
`g_object_set_property()` would have proceeded with calling the object's
`set_property` class method. Here, since our implementation of ViewerFile
did override this method, execution would jump to `viewer_file_set_property`
after having retrieved from the GParamSpec the `param_id` which had been
stored by `g_object_class_install_property()`.

Once the property has been set by the object's `set_property` class method,
execution returns to `g_object_set_property()` which makes sure that the
"notify" signal is emitted on the object's instance with the changed
property as parameter unless notifications were frozen by
`g_object_freeze_notify()`.

`g_object_thaw_notify()` can be used to re-enable notification of property
modifications through the “notify” signal. It is important to remember that
even if properties are changed while property change notification is frozen,
the "notify" signal will be emitted once for each of these changed
properties as soon as the property change notification is thawed: no
property change is lost for the "notify" signal, although multiple
notifications for a single property are compressed. Signals can only be
delayed by the notification freezing mechanism.

It sounds like a tedious task to set up GValues every time when one wants to
modify a property. In practice one will rarely do this. The functions
`g_object_set_property()` and `g_object_get_property()` are meant to be used
by language bindings. For application there is an easier way and that is
described next.

### Accessing multiple properties at once

It is interesting to note that the `g_object_set()` and
`g_object_set_valist()` (variadic version) functions can be used to set
multiple properties at once. The client code shown above can then be
re-written as:

```c
ViewerFile *file;
file = /* */;
g_object_set (G_OBJECT (file),
              "zoom-level", 6, 
              "filename", "~/some-file.txt", 
              NULL);
```

This saves us from managing the GValues that we were needing to handle when
using `g_object_set_property()`. The code above will trigger one notify
signal emission for each property modified.

Equivalent `_get` versions are also available: `g_object_get()` and
`g_object_get_valist()` (variadic version) can be used to get numerous
properties at once.

These high level functions have one drawback — they don't provide a return
value. One should pay attention to the argument types and ranges when using
them. A known source of errors is to pass a different type from what the
property expects; for instance, passing an integer when the property expects
a floating point value and thus shifting all subsequent parameters by some
number of bytes. Also forgetting the terminating NULL will lead to undefined
behaviour.

This explains how `g_object_new()`, `g_object_newv()` and
`g_object_new_valist()` work: they parse the user-provided variable number
of parameters and invoke `g_object_set()` on the parameters only after the
object has been successfully constructed. The "notify" signal will be
emitted for each property set.

## The GObject messaging system

### Closures

Closures are central to the concept of asynchronous signal delivery which is
widely used throughout GTK and GNOME applications. A closure is an
abstraction, a generic representation of a callback. It is a small structure
which contains three objects:

- a function pointer (the callback itself) whose prototype looks like:
  `return_type function_callback (... , gpointer user_data);`
- the `user_data` pointer which is passed to the callback upon invocation of
  the closure
- a function pointer which represents the destructor of the closure:
  whenever the closure's refcount reaches zero, this function will be called
  before the closure structure is freed

The `GClosure` structure represents the common functionality of all closure
implementations: there exists a different closure implementation for each
separate runtime which wants to use the GObject type system. The GObject
library provides a simple GCClosure type which is a specific implementation
of closures to be used with C/C++ callbacks.

A `GClosure` provides simple services:

- invocation (`g_closure_invoke()`): this is what closures were created for;
  they hide the details of callback invocation from the callback invoker.
- notification: the closure notifies listeners of certain events such as
  closure invocation, closure invalidation and closure finalization.
  Listeners can be registered with `g_closure_add_finalize_notifier()`
  (finalization notification), `g_closure_add_invalidate_notifier()`
  (invalidation notification) and `g_closure_add_marshal_guards()`
  (invocation notification). There exist symmetric deregistration functions
  for finalization and invalidation events
  (`g_closure_remove_finalize_notifier()` and
  `g_closure_remove_invalidate_notifier()`) but not for the invocation
  process

#### C Closures

If you are using C or C++ to connect a callback to a given event, you will
either use simple `GCClosures` which have a pretty minimal API or the even
simpler `g_signal_connect()` functions (which will be presented a bit later).

`g_cclosure_new()` will create a new closure which can invoke the
user-provided `callback_func` with the user-provided `user_data` as its last
parameter. When the closure is finalized (second stage of the destruction
process), it will invoke the `destroy_data` function if the user has
supplied one.

`g_cclosure_new_swap()` will create a new closure which can invoke the
user-provided `callback_func` with the user-provided `user_data` as its
first parameter (instead of being the last parameter as with
`g_cclosure_new()`). When the closure is finalized (second stage of the
destruction process), it will invoke the `destroy_data` function if the user
has supplied one.

#### Non-C closures (for the fearless)

As was explained above, closures hide the details of callback invocation. In
C, callback invocation is just like function invocation: it is a matter of
creating the correct stack frame for the called function and executing a
call assembly instruction.

C closure marshallers transform the array of GValues which represent the
parameters to the target function into a C-style function parameter list,
invoke the user-supplied C function with this new parameter list, get the
return value of the function, transform it into a GValue and return this
GValue to the marshaller caller.

A generic C closure marshaller is available as
`g_cclosure_marshal_generic()` which implements marshalling for all function
types using libffi. Custom marshallers for different types are not needed
apart from performance critical code where the libffi-based marshaller may
be too slow.

An example of a custom marshaller is given below, illustrating how GValues
can be converted to a C function call. The marshaller is for a C function
which takes an integer as its first parameter and returns `void`.

```c
g_cclosure_marshal_VOID__INT (GClosure     *closure,
                              GValue       *return_value,
                              guint         n_param_values,
                              const GValue *param_values,
                              gpointer      invocation_hint,
                              gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__INT) (gpointer     data1,
                                          gint         arg_1,
                                          gpointer     data2);
  register GMarshalFunc_VOID__INT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 2);

  data1 = g_value_peek_pointer (param_values + 0);
  data2 = closure->data;

  callback = (GMarshalFunc_VOID__INT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_int (param_values + 1),
            data2);
}
```

There exist other kinds of marshallers, for example there is a generic
Python marshaller which is used by all Python closures (a Python closure is
used to invoke a callback written in Python). This Python marshaller
transforms the input GValue list representing the function parameters into a
Python tuple which is the equivalent structure in Python.

### Signals

GObject's signals have nothing to do with standard UNIX signals: they
connect arbitrary application-specific events with any number of listeners.
For example, in GTK, every user event (keystroke or mouse move) is received
from the windowing system and generates a GTK event in the form of a signal
emission on the widget object instance.

Each signal is registered in the type system together with the type on which
it can be emitted: users of the type are said to connect to the signal on a
given type instance when they register a closure to be invoked upon the
signal emission. Users can also emit the signal by themselves or stop the
emission of the signal from within one of the closures connected to the
signal.

When a signal is emitted on a given type instance, all the closures
connected to this signal on this type instance will be invoked. All the
closures connected to such a signal represent callbacks whose signature
looks like:

```c
return_type
function_callback (gpointer instance,
                   ...,
                   gpointer user_data);
```

#### Signal registration

To register a new signal on an existing type, we can use any of
`g_signal_newv()`, `g_signal_new_valist()` or `g_signal_new()` functions:

```c
guint
g_signal_newv (const gchar        *signal_name,
               GType               itype,
               GSignalFlags        signal_flags,
               GClosure           *class_closure,
               GSignalAccumulator  accumulator,
               gpointer            accu_data,
               GSignalCMarshaller  c_marshaller,
               GType               return_type,
               guint               n_params,
               GType              *param_types);
```

The number of parameters to these functions is a bit intimidating but they are relatively simple:

- `signal_name`: is a string which can be used to uniquely identify a given
  signal
- `itype`: is the instance type on which this signal can be emitted
- `signal_flags`: partly defines the order in which closures which were
  connected to the signal are invoked
- `class_closure`: this is the default closure for the signal: if it is not
  `NULL` upon the signal emission, it will be invoked upon this emission of
  the signal. The moment where this closure is invoked compared to other
  closures connected to that signal depends partly on the `signal_flags`
- `accumulator`: this is a function pointer which is invoked after each
  closure has been invoked. If it returns `FALSE`, signal emission is stopped.
  If it returns `TRUE`, signal emission proceeds normally. It is also used to
  compute the return value of the signal based on the return value of all
  the invoked closures. For example, an accumulator could ignore `NULL`
  returns from closures; or it could build a list of the values returned by
  the closures
- `accu_data`: this pointer will be passed down to each invocation of the
  accumulator during emission
- `c_marshaller`: this is the default C marshaller for any closure which is
  connected to this signal
- `return_type`: this is the type of the return value of the signal
- `n_params`: this is the number of parameters this signal takes
- `param_types`: this is an array of GTypes which indicate the type of each
  parameter of the signal. The length of this array is indicated by
  `n_params`.

As you can see from the above definition, a signal is basically a
description of the closures which can be connected to this signal and a
description of the order in which the closures connected to this signal will
be invoked.

#### Signal connection

If you want to connect to a signal with a closure, you have three possibilities:

- you can register a class closure at signal registration: this is a
  system-wide operation. i.e.: the class closure will be invoked during each
  emission of a given signal on any of the instances of the type which
  supports that signal
- you can use `g_signal_override_class_closure()` which overrides the class
  closure of a given type. It is possible to call this function only on a
  derived type of the type on which the signal was registered. This function
  is of use only to language bindings
- you can register a closure with the `g_signal_connect()` family of
  functions. This is an instance-specific operation: the closure will be
  invoked only during emission of a given signal on a given instance

It is also possible to connect a different kind of callback on a given
signal: emission hooks are invoked whenever a given signal is emitted
whatever the instance on which it is emitted. Emission hooks are connected
with `g_signal_add_emission_hook()` and removed with
`g_signal_remove_emission_hook()`.

#### Signal emission

Signal emission is done through the use of the `g_signal_emit()` family of functions.

```c
void
g_signal_emitv (const GValue  instance_and_params[],
                guint         signal_id,
                GQuark        detail,
                GValue       *return_value);
```

- the `instance_and_params` array of GValues contains the list of input
  parameters to the signal. The first element of the array is the instance
  pointer on which to invoke the signal. The following elements of the array
  contain the list of parameters to the signal
- `signal_id` identifies the signal to invoke
- `detail` identifies the specific detail of the signal to invoke. A detail
  is a kind of magic token/argument which is passed around during signal
  emission and which is used by closures connected to the signal to filter
  out unwanted signal emissions. In most cases, you can safely set this
  value to zero. See the section called "The detail argument" for more
  information about this parameter
- `return_value` holds the return value of the last closure invoked during
  emission if no accumulator was specified. If an accumulator was specified
  during signal creation, this accumulator is used to calculate the return
  value as a function of the return values of all the closures invoked
  during emission. If no closure is invoked during emission, the
  `return_value` is nonetheless initialized to zero/`NULL`

Signal emission can be decomposed in 6 steps:

1. `RUN_FIRST`: if the `G_SIGNAL_RUN_FIRST` flag was used during signal
   registration and if there exists a class closure for this signal, the
   class closure is invoked.
2. `EMISSION_HOOK`: if any emission hook was added to the signal, they are
   invoked from first to last added. Accumulate return values.
3. `HANDLER_RUN_FIRST`: if any closure were connected with the
   `g_signal_connect()` family of functions, and if they are not blocked
   (with the `g_signal_handler_block()` family of functions) they are run
   here, from first to last connected.
4. `RUN_LAST`: if the `G_SIGNAL_RUN_LAST` flag was set during registration
   and if a class closure was set, it is invoked here.
5. `HANDLER_RUN_LAST`: if any closure were connected with the
   `g_signal_connect_after()` family of functions, if they were not invoked
   during `HANDLER_RUN_FIRST` and if they are not blocked, they are run
   here, from first to last connected.
6. `RUN_CLEANUP`: if the `G_SIGNAL_RUN_CLEANUP` flag was set during
   registration and if a class closure was set, it is invoked here. Signal
   emission is completed here.

If, at any point during the emission (except in the `RUN_CLEANUP` or
`EMISSION_HOOK` states), one of the closures stops the signal emission with
`g_signal_stop_emission()`, the emission jumps to the `RUN_CLEANUP` state.

If, at any point during emission, one of the closures or emission hook emits
the same signal on the same instance, emission is restarted from the
`RUN_FIRST` state.

The accumulator function is invoked in all states, after invocation of each
closure (except in `RUN_EMISSION_HOOK` and `RUN_CLEANUP`). It accumulates
the closure return value into the signal return value and returns `TRUE` or
`FALSE`.  If, at any point, it does not return `TRUE`, emission jumps to
`RUN_CLEANUP` state.

If no accumulator function was provided, the value returned by the last
handler run will be returned by `g_signal_emit()`.

#### The detail argument

All the functions related to signal emission or signal connection have a
parameter named the detail. Sometimes, this parameter is hidden by the API
but it is always there, in one form or another.

Of the three main connection functions, only one has an explicit detail
parameter as a GQuark: `g_signal_connect_closure_by_id()`.

The two other functions, `g_signal_connect_closure()` and
`g_signal_connect_data()` hide the detail parameter in the signal name
identification. Their `detailed_signal` parameter is a string which
identifies the name of the signal to connect to. The format of this string
should match `signal_name::detail_name`. For example, connecting to the
signal named `notify::cursor_position` will actually connect to the signal
named `notify` with the `cursor_position` detail. Internally, the detail
string is transformed to a GQuark if it is present.

Of the four main signal emission functions, one hides it in its signal name
parameter: `g_signal_emit_by_name()`. The other three have an explicit
detail parameter as a GQuark again: `g_signal_emit()`, `g_signal_emitv()`
and `g_signal_emit_valist()`.

If a detail is provided by the user to the emission function, it is used
during emission to match against the closures which also provide a detail.
If a closure's detail does not match the detail provided by the user, it
will not be invoked (even though it is connected to a signal which is being
emitted).

This completely optional filtering mechanism is mainly used as an
optimization for signals which are often emitted for many different reasons:
the clients can filter out which events they are interested in before the
closure's marshalling code runs. For example, this is used extensively by
the notify signal of GObject: whenever a property is modified on a GObject,
instead of just emitting the notify signal, GObject associates as a detail
to this signal emission the name of the property modified. This allows
clients who wish to be notified of changes to only one property to filter
most events before receiving them.

As a simple rule, users can and should set the detail parameter to zero:
this will disable completely this optional filtering for that signal.
