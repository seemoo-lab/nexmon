Title: GObject Tutorial

# GObject Tutorial

## How to define and implement a new GObject

This document focuses on the implementation of a subtype of GObject, for
example to create a custom class hierarchy, or to subclass a GTK widget.

Throughout the chapter, a running example of a file viewer program is used,
which has a `ViewerFile` class to represent a single file being viewed, and
various derived classes for different types of files with special
functionality, such as audio files. The example application also supports
editing files (for example, to tweak a photo being viewed), using a
`ViewerEditable` interface.

### Boilerplate header code

The first step before writing the code for your GObject is to write the
type's header which contains the needed type, function and macro
definitions. Each of these elements is nothing but a convention which is
followed by almost all users of GObject, and has been refined over multiple
years of experience developing GObject-based code. If you are writing a
library, it is particularly important for you to adhere closely to these
conventions; users of your library will assume that you have. Even if not
writing a library, it will help other people who want to work on your
project.

Pick a name convention for your headers and source code and stick to it:

- use a dash to separate the prefix from the typename: `viewer-file.h` and
  `viewer-file.c` (this is the convention used by most GNOME libraries and
  applications)
- use an underscore to separate the prefix from the typename:
  `viewer_file.h` and `viewer_file.c`
- do not separate the prefix from the typename: `viewerfile.h` and
  `viewerfile.c` (this is the convention used by GTK)

Some people like the first two solutions better: it makes reading file names
easier for those with poor eyesight.

The basic conventions for any header which exposes a GType are described in
the section of the Type system introduction called
["Conventions"](concepts.html#conventions).

If you want to declare a type named "file" in the namespace "viewer", name
the type instance `ViewerFile` and its class `ViewerFileClass` (names are
case sensitive). The recommended method of declaring a type differs based on
whether the type is final or derivable.

Final types cannot be subclassed further, and should be the default choice
for new types—changing a final type to be derivable is always a change that
will be compatible with existing uses of the code, but the converse will
often cause problems. Final types are declared using the
`G_DECLARE_FINAL_TYPE` macro, and require a structure to hold the instance
data to be declared in the source code (not the header file).

```c
/*
 * Copyright/Licensing information.
 */

/* inclusion guard */
#pragma once

#include <glib-object.h>

/*
 * Potentially, include other headers on which this header depends.
 */

G_BEGIN_DECLS

/*
 * Type declaration.
 */
#define VIEWER_TYPE_FILE viewer_file_get_type()
G_DECLARE_FINAL_TYPE (ViewerFile, viewer_file, VIEWER, FILE, GObject)

/*
 * Method definitions.
 */
ViewerFile *viewer_file_new (void);

G_END_DECLS
```

Derivable types can be subclassed further, and their class and instance
structures form part of the public API which must not be changed if API
stability is cared about. They are declared using the
`G_DECLARE_DERIVABLE_TYPE` macro:

```c
/*
 * Copyright/Licensing information.
 */

/* inclusion guard */
#pragma once

#include <glib-object.h>

/*
 * Potentially, include other headers on which this header depends.
 */

G_BEGIN_DECLS

/*
 * Type declaration.
 */
#define VIEWER_TYPE_FILE viewer_file_get_type()
G_DECLARE_DERIVABLE_TYPE (ViewerFile, viewer_file, VIEWER, FILE, GObject)

struct _ViewerFileClass
{
  GObjectClass parent_class;

  /* Class virtual function fields. */
  void (* open) (ViewerFile  *file,
                 GError     **error);

  /* Padding to allow adding up to 12 new virtual functions without
   * breaking ABI. */
  gpointer padding[12];
};

/*
 * Method definitions.
 */
ViewerFile *viewer_file_new (void);

G_END_DECLS
```

The convention for header includes is to add the minimum number of
`#include` directives to the top of your headers needed to compile that
header. This allows client code to simply `#include "viewer-file.h"`,
without needing to know the prerequisites for `viewer-file.h`.

### Boilerplate code

In your code, the first step is to `#include` the needed headers:

```c
/*
 * Copyright/Licensing information
 */

#include "viewer-file.h"

/* Private structure definition. */
typedef struct {
  char *filename;

  /* other private fields */
} ViewerFilePrivate;

/*
 * forward definitions
 */
```

If the class is being declared as final using `G_DECLARE_FINAL_TYPE`, its instance structure should be defined in the C file:

```c
struct _ViewerFile
{
  GObject parent_instance;

  /* Other members, including private data. */
};
```

Call the `G_DEFINE_TYPE` macro (or `G_DEFINE_TYPE_WITH_PRIVATE` if your
class needs private data—final types do not need private data) using the
name of the type, the prefix of the functions and the parent GType to reduce
the amount of boilerplate needed. This macro will:

- implement the `viewer_file_get_type` function
- define a parent class pointer accessible from the whole `.c` file
- add private instance data to the type (if using `G_DEFINE_TYPE_WITH_PRIVATE`)

If the class has been declared as final using `G_DECLARE_FINAL_TYPE` private
data should be placed in the instance structure, `ViewerFile`, and
`G_DEFINE_TYPE` should be used instead of `G_DEFINE_TYPE_WITH_PRIVATE`. The
instance structure for a final class is not exposed publicly, and is not
embedded in the instance structures of any derived classes (because the
class is final); so its size can vary without causing incompatibilities for
code which uses the class. Conversely, private data for derivable classes
must be included in a private structure, and `G_DEFINE_TYPE_WITH_PRIVATE`
must be used.

```c
G_DEFINE_TYPE (ViewerFile, viewer_file, G_TYPE_OBJECT)
```

or

```c
G_DEFINE_TYPE_WITH_PRIVATE (ViewerFile, viewer_file, G_TYPE_OBJECT)
```

It is also possible to use the `G_DEFINE_TYPE_WITH_CODE` macro to control
the `get_type` function implementation — for instance, to add a call to the
`G_IMPLEMENT_INTERFACE` macro to implement an interface.

### Object construction

People often get confused when trying to construct their GObjects because of
the sheer number of different ways to hook into the objects's construction
process: it is difficult to figure which is the correct, recommended way.

The [documentation on object
instantiation](concepts.html#object-instantiation) shows what user-provided
functions are invoked during object instantiation and in which order they
are invoked. A user looking for the equivalent of the simple C++ constructor
function should use the `instance_init` method. It will be invoked after all
the parents’ `instance_init` functions have been invoked. It cannot take
arbitrary construction parameters (as in C++) but if your object needs
arbitrary parameters to complete initialization, you can use construction
properties.

Construction properties will be set only after all `instance_init` functions have run. No object reference will be returned to the client of `g_object_new()` until all the construction properties have been set.

It is important to note that object construction cannot ever fail. If you
require a fallible GObject construction, you can use the `GInitable` and
`GAsyncInitable` interfaces provided by the GIO library.

You should write the following code first:

```c
G_DEFINE_TYPE_WITH_PRIVATE (ViewerFile, viewer_file, G_TYPE_OBJECT)

static void
viewer_file_class_init (ViewerFileClass *klass)
{
}

static void
viewer_file_init (ViewerFile *self)
{
  ViewerFilePrivate *priv = viewer_file_get_instance_private (self);

  /* initialize all public and private members to reasonable default values.
   * They are all automatically initialized to 0 to begin with. */
}
```

If you need special construction properties (with `G_PARAM_CONSTRUCT_ONLY`
set), install the properties in the `class_init()` function, override the
`set_property()` and `get_property()` methods of the GObject class, and
implement them as described by the section called ["Object
properties"](concepts.html#object-properties).

Property identifiers must start from 1, as 0 is reserved for internal use by GObject.

```c
enum
{
  PROP_FILENAME = 1,
  PROP_ZOOM_LEVEL,
  N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

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

If you need this, make sure you can build and run code similar to the code
shown above. Also, make sure your construct properties can be set without
side effects during construction.

Some people sometimes need to complete the initialization of an instance of
a type only after the properties passed to the constructors have been set.
This is possible through the use of the `constructor()` class method as
described in the section called "Object instantiation" or, more simply,
using the `constructed()` class method. Note that the `constructed()` virtual
function will only be invoked after the properties marked as
`G_PARAM_CONSTRUCT_ONLY` or `G_PARAM_CONSTRUCT` have been consumed, but before
the regular properties passed to `g_object_new()` have been set.

### Object destruction

Again, it is often difficult to figure out which mechanism to use to hook
into the object's destruction process: when the last `g_object_unref()` function
call is made, a lot of things happen as described in [the "Object memory
management" section](concepts.html#object-memory-management) of the
documentation.

The destruction process of your object is in two phases: dispose and
finalize. This split is necessary to handle potential cycles due to the
nature of the reference counting mechanism used by GObject, as well as
dealing with temporary revival of instances in case of signal emission
during the destruction sequence.

```c
struct _ViewerFilePrivate
{
  gchar *filename;
  guint zoom_level;

  GInputStream *input_stream;
};

G_DEFINE_TYPE_WITH_PRIVATE (ViewerFile, viewer_file, G_TYPE_OBJECT)

static void
viewer_file_dispose (GObject *gobject)
{
  ViewerFilePrivate *priv = viewer_file_get_instance_private (VIEWER_FILE (gobject));

  /* In dispose(), you are supposed to free all types referenced from this
   * object which might themselves hold a reference to self. Generally,
   * the most simple solution is to unref all members on which you own a
   * reference.
   */

  /* dispose() might be called multiple times, so we must guard against
   * calling g_object_unref() on an invalid GObject by setting the member
   * NULL; g_clear_object() does this for us.
   */
  g_clear_object (&priv->input_stream);

  /* Always chain up to the parent class; there is no need to check if
   * the parent class implements the dispose() virtual function: it is
   * always guaranteed to do so
   */
  G_OBJECT_CLASS (viewer_file_parent_class)->dispose (gobject);
}

static void
viewer_file_finalize (GObject *gobject)
{
  ViewerFilePrivate *priv = viewer_file_get_instance_private (VIEWER_FILE (gobject));

  g_free (priv->filename);

  /* Always chain up to the parent class; as with dispose(), finalize()
   * is guaranteed to exist on the parent's class virtual function table
   */
  G_OBJECT_CLASS (viewer_file_parent_class)->finalize (gobject);
}

static void
viewer_file_class_init (ViewerFileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = viewer_file_dispose;
  object_class->finalize = viewer_file_finalize;
}

static void
viewer_file_init (ViewerFile *self);
{
  ViewerFilePrivate *priv = viewer_file_get_instance_private (self);

  priv->input_stream = g_object_new (VIEWER_TYPE_INPUT_STREAM, NULL);
  priv->filename = /* would be set as a property */;
}
```

It is possible that object methods might be invoked after dispose is run and
before finalize runs. GObject does not consider this to be a program error:
you must gracefully detect this and neither crash nor warn the user, by
having a disposed instance revert to an inert state.

### Object methods

Just as with C++, there are many different ways to define object methods and
extend them: the following list and sections draw on C++ vocabulary.
(Readers are expected to know basic C++ concepts. Those who have not had to
write C++ code recently can refer to a [C++
tutorial](http://www.cplusplus.com/doc/tutorial/) to refresh their
memories.)

- non-virtual public methods,
- virtual public methods and
- virtual private methods
- non-virtual private methods

#### Non-Virtual Methods

These are the simplest, providing a simple method which acts on the object.
Provide a function prototype in the header and an implementation of that
prototype in the source file.

```c
/* declaration in the header. */
void viewer_file_open (ViewerFile  *self,
                       GError     **error);
```

```c
/* implementation in the source file */
void
viewer_file_open (ViewerFile  *self,
                  GError     **error)
{
  g_return_if_fail (VIEWER_IS_FILE (self));
  g_return_if_fail (error == NULL || *error == NULL);

  /* do stuff here. */
}
```

#### Virtual public methods

This is the preferred way to create GObjects with overridable methods:

- define the common method and its virtual function in the class structure
  in the public header
- define the common method in the header file and implement it in the source
  file
- implement a base version of the virtual function in the source file and
  initialize the virtual function pointer to this implementation in the
  object’s `class_init` function; or leave it as `NULL` for a ‘pure virtual’
  method which must be overridden by derived classes
- re-implement the virtual function in each derived class which needs to
  override it

Note that virtual functions can only be defined if the class is derivable,
declared using `G_DECLARE_DERIVABLE_TYPE` so the class structure can be
defined.

```c
/* declaration in viewer-file.h. */
#define VIEWER_TYPE_FILE viewer_file_get_type ()
G_DECLARE_DERIVABLE_TYPE (ViewerFile, viewer_file, VIEWER, FILE, GObject)

struct _ViewerFileClass
{
  GObjectClass parent_class;

  /* stuff */
  void (*open) (ViewerFile  *self,
                GError     **error);

  /* Padding to allow adding up to 12 new virtual functions without
   * breaking ABI. */
  gpointer padding[12];
};

void viewer_file_open (ViewerFile  *self,
                       GError     **error);
```

```c
/* implementation in viewer-file.c */
void
viewer_file_open (ViewerFile  *self,
                  GError     **error)
{
  ViewerFileClass *klass;

  g_return_if_fail (VIEWER_IS_FILE (self));
  g_return_if_fail (error == NULL || *error == NULL);

  klass = VIEWER_FILE_GET_CLASS (self);
  g_return_if_fail (klass->open != NULL);

  klass->open (self, error);
}
```

The code above simply redirects the open call to the relevant virtual
function.

It is possible to provide a default implementation for this class method in
the object's `class_init` function: initialize the `klass->open` field to a
pointer to the actual implementation. By default, class methods that are not
inherited are initialized to `NULL`, and thus are to be considered "pure
virtual".

```c
static void
viewer_file_real_close (ViewerFile  *self,
                        GError     **error)
{
  /* Default implementation for the virtual method. */
}

static void
viewer_file_class_init (ViewerFileClass *klass)
{
  /* this is not necessary, except for demonstration purposes.
   *
   * pure virtual method: mandates implementation in children.
   */
  klass->open = NULL;

  /* merely virtual method. */
  klass->close = viewer_file_real_close;
}

void
viewer_file_open (ViewerFile  *self,
                  GError     **error)
{
  ViewerFileClass *klass;

  g_return_if_fail (VIEWER_IS_FILE (self));
  g_return_if_fail (error == NULL || *error == NULL);

  klass = VIEWER_FILE_GET_CLASS (self);

  /* if the method is purely virtual, then it is a good idea to
   * check that it has been overridden before calling it, and,
   * depending on the intent of the class, either ignore it silently
   * or warn the user.
   */
  g_return_if_fail (klass->open != NULL);
  klass->open (self, error);
}

void
viewer_file_close (ViewerFile  *self,
                   GError     **error)
{
  ViewerFileClass *klass;

  g_return_if_fail (VIEWER_IS_FILE (self));
  g_return_if_fail (error == NULL || *error == NULL);

  klass = VIEWER_FILE_GET_CLASS (self);
  if (klass->close != NULL)
    klass->close (self, error);
}
```

#### Virtual private Methods

These are very similar to virtual public methods. They just don't have a
public function to call directly. The header file contains only a
declaration of the virtual function:

```c
/* declaration in viewer-file.h. */
struct _ViewerFileClass
{
  GObjectClass parent;

  /* Public virtual method as before. */
  void     (*open)           (ViewerFile  *self,
                              GError     **error);

  /* Private helper function to work out whether the file can be loaded via
   * memory mapped I/O, or whether it has to be read as a stream. */
  gboolean (*can_memory_map) (ViewerFile *self);

  /* Padding to allow adding up to 12 new virtual functions without
   * breaking ABI. */
  gpointer padding[12];
};

void viewer_file_open (ViewerFile *self, GError **error);
```

These virtual functions are often used to delegate part of the job to child classes:

```c
/* this accessor function is static: it is not exported outside of this file. */
static gboolean
viewer_file_can_memory_map (ViewerFile *self)
{
  return VIEWER_FILE_GET_CLASS (self)->can_memory_map (self);
}

void
viewer_file_open (ViewerFile  *self,
                  GError     **error)
{
  g_return_if_fail (VIEWER_IS_FILE (self));
  g_return_if_fail (error == NULL || *error == NULL);

  /*
   * Try to load the file using memory mapped I/O, if the implementation of the
   * class determines that is possible using its private virtual method.
   */
  if (viewer_file_can_memory_map (self))
    {
      /* Load the file using memory mapped I/O. */
    }
  else
    {
      /* Fall back to trying to load the file using streaming I/O… */
    }
}
```

Again, it is possible to provide a default implementation for this private virtual function:

```c
static gboolean
viewer_file_real_can_memory_map (ViewerFile *self)
{
  /* As an example, always return false. Or, potentially return true if the
   * file is local. */
  return FALSE;
}

static void
viewer_file_class_init (ViewerFileClass *klass)
{
  /* non-pure virtual method; does not have to be implemented in children. */
  klass->can_memory_map = viewer_file_real_can_memory_map;
}
```

Derived classes can then override the method with code such as:

```c
static void
viewer_audio_file_class_init (ViewerAudioFileClass *klass)
{
  ViewerFileClass *file_class = VIEWER_FILE_CLASS (klass);

  /* implement pure virtual function. */
  file_class->can_memory_map = viewer_audio_file_can_memory_map;
}
```

### Chaining up

Chaining up is often loosely defined by the following set of conditions:

- parent class A defines a public virtual method named `foo` and provides a
  default implementation
- child class B re-implements method `foo`
- B’s implementation of `foo` calls (‘chains up to’) its parent class A’s
  implementation of `foo`

There are various uses of this idiom:

- you need to extend the behaviour of a class without modifying its code.
  You create a subclass to inherit its implementation, re-implement a public
  virtual method to modify the behaviour and chain up to ensure that the
  previous behaviour is not really modified, just extended
- you need to implement the
  [Chain of Responsibility pattern](https://en.wikipedia.org/wiki/Chain-of-responsibility_pattern):
  each object of the inheritance tree chains up to its parent (typically, at the
  beginning or the end of the method) to ensure that each handler is run in turn

To explicitly chain up to the implementation of the virtual method in the
parent class, you first need a handle to the original parent class
structure. This pointer can then be used to access the original virtual
function pointer and invoke it directly

The "original" adjective used in the sentence above is not innocuous. To
fully understand its meaning, recall how class structures are initialized:
for each object type, the class structure associated with this object is
created by first copying the class structure of its parent type (a simple
memcpy) and then by invoking the `class_init` callback on the resulting class
structure. Since the `class_init` callback is responsible for overwriting the
class structure with the user re-implementations of the class methods, the
modified copy of the parent class structure stored in the derived instance
cannot be used. A copy of the class structure of an instance of the parent
class is needed.

To chain up, you can use the `parent_class` pointer created and initialized
by the `G_DEFINE_TYPE` family of macros, for instance:

```c
static void
b_method_to_call (B *obj, int some_param)
{
  /* do stuff before chain up */

  /* call the method_to_call() virtual function on the
   * parent of BClass, AClass.
   *
   * remember the explicit cast to AClass*
   */
  A_CLASS (b_parent_class)->method_to_call (obj, some_param);

  /* do stuff after chain up */
}
```

## How to define and implement interfaces

### Defining interfaces

The theory behind how GObject interfaces work is given in the section called
["Non-instantiatable classed types:
interfaces"](concepts.html#non-instantiatable-classed-types-interfaces);
this section covers how to define and implement an interface.

The first step is to get the header right. This interface defines three
methods:

```c
/*
 * Copyright/Licensing information.
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define VIEWER_TYPE_EDITABLE viewer_editable_get_type()
G_DECLARE_INTERFACE (ViewerEditable, viewer_editable, VIEWER, EDITABLE, GObject)

struct _ViewerEditableInterface
{
  GTypeInterface parent_iface;

  void (*save) (ViewerEditable  *self,
                GError         **error);
  void (*undo) (ViewerEditable  *self,
                guint            n_steps);
  void (*redo) (ViewerEditable  *self,
                guint            n_steps);
};

void viewer_editable_save (ViewerEditable  *self,
                           GError         **error);
void viewer_editable_undo (ViewerEditable  *self,
                           guint            n_steps);
void viewer_editable_redo (ViewerEditable  *self,
                           guint            n_steps);

G_END_DECLS
```

This code is the same as the code for a normal GType which derives from a
GObject except for a few details:

- the `_GET_CLASS` function is called `_GET_IFACE` (and is defined by `G_DECLARE_INTERFACE`)
- the instance type, `ViewerEditable`, is not fully defined: it is used merely as an abstract type which represents an instance of whatever object which implements the interface
- the parent of the `ViewerEditableInterface` is `GTypeInterface`, not `GObjectClass`

The implementation of the `ViewerEditable` type itself is trivial:

- `G_DEFINE_INTERFACE` creates a `viewer_editable_get_type` function which registers the type in the type system. The third argument is used to define a prerequisite interface (which we'll talk about more later). Just pass 0 for this argument when an interface has no prerequisite
- `viewer_editable_default_init` is expected to register the interface's signals if there are any (we will see a bit later how to use them)
- the interface methods `viewer_editable_save`, `viewer_editable_undo` and `viewer_editable_redo` dereference the interface structure to access its associated interface function and call it

```c
G_DEFINE_INTERFACE (ViewerEditable, viewer_editable, G_TYPE_OBJECT)

static void
viewer_editable_default_init (ViewerEditableInterface *iface)
{
    /* add properties and signals to the interface here */
}

void
viewer_editable_save (ViewerEditable  *self,
                      GError         **error)
{
  ViewerEditableInterface *iface;

  g_return_if_fail (VIEWER_IS_EDITABLE (self));
  g_return_if_fail (error == NULL || *error == NULL);

  iface = VIEWER_EDITABLE_GET_IFACE (self);
  g_return_if_fail (iface->save != NULL);
  iface->save (self, error);
}

void
viewer_editable_undo (ViewerEditable *self,
                      guint           n_steps)
{
  ViewerEditableInterface *iface;

  g_return_if_fail (VIEWER_IS_EDITABLE (self));

  iface = VIEWER_EDITABLE_GET_IFACE (self);
  g_return_if_fail (iface->undo != NULL);
  iface->undo (self, n_steps);
}

void
viewer_editable_redo (ViewerEditable *self,
                      guint           n_steps)
{
  ViewerEditableInterface *iface;

  g_return_if_fail (VIEWER_IS_EDITABLE (self));

  iface = VIEWER_EDITABLE_GET_IFACE (self);
  g_return_if_fail (iface->redo != NULL);
  iface->redo (self, n_steps);
}
```

### Implementing interfaces

Once the interface is defined, implementing it is rather trivial.

The first step is to define a normal final GObject class exactly as usual.

The second step is to implement `ViewerFile` by defining it using
`G_DEFINE_TYPE_WITH_CODE` and `G_IMPLEMENT_INTERFACE` instead of
`G_DEFINE_TYPE`:

```c
static void viewer_file_editable_interface_init (ViewerEditableInterface *iface);

G_DEFINE_TYPE_WITH_CODE (ViewerFile, viewer_file, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (VIEWER_TYPE_EDITABLE,
                                                viewer_file_editable_interface_init))
```

This definition is very much like all the similar functions seen previously.
The only interface-specific code present here is the use of
`G_IMPLEMENT_INTERFACE`.

Classes can implement multiple interfaces by using multiple calls to
`G_IMPLEMENT_INTERFACE` inside the call to `G_DEFINE_TYPE_WITH_CODE`.

`viewer_file_editable_interface_init` is the interface initialization
function: inside it, every virtual method of the interface must be assigned
to its implementation:

```c
static void
viewer_file_editable_save (ViewerFile  *self,
                           GError     **error)
{
  g_print ("File implementation of editable interface save method: %s.\n",
           self->filename);
}

static void
viewer_file_editable_undo (ViewerFile *self,
                           guint       n_steps)
{
  g_print ("File implementation of editable interface undo method: %s.\n",
           self->filename);
}

static void
viewer_file_editable_redo (ViewerFile *self,
                           guint       n_steps)
{
  g_print ("File implementation of editable interface redo method: %s.\n",
           self->filename);
}

static void
viewer_file_editable_interface_init (ViewerEditableInterface *iface)
{
  iface->save = viewer_file_editable_save;
  iface->undo = viewer_file_editable_undo;
  iface->redo = viewer_file_editable_redo;
}

static void
viewer_file_init (ViewerFile *self)
{
  /* Instance variable initialisation code. */
}
```

If the object is not of final type, e.g. was declared using
`G_DECLARE_DERIVABLE_TYPE` then `G_ADD_PRIVATE` macro should be added. The
private structure should be declared exactly as for a normal derivable
object.

```c
G_DEFINE_TYPE_WITH_CODE (ViewerFile, viewer_file, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (ViewerFile)
                         G_IMPLEMENT_INTERFACE (VIEWER_TYPE_EDITABLE,
                                                viewer_file_editable_interface_init))
```

### Interface definition prerequisites

To specify that an interface requires the presence of other interfaces when
implemented, GObject introduces the concept of prerequisites: it is possible
to associate a list of prerequisite types to an interface. For example, if
object A wishes to implement interface I1, and if interface I1 has a
prerequisite on interface I2, A has to implement both I1 and I2.

The mechanism described above is, in practice, very similar to Java's
interface I1 extends interface I2. The example below shows the GObject
equivalent:

```
/* Make the ViewerEditableLossy interface require ViewerEditable interface. */
G_DEFINE_INTERFACE (ViewerEditableLossy, viewer_editable_lossy, VIEWER_TYPE_EDITABLE)
```

In the `G_DEFINE_INTERFACE` call above, the third parameter defines the
prerequisite type. This is the GType of either an interface or a class. In
this case the `ViewerEditable` interface is a prerequisite of
`ViewerEditableLossy`. The code below shows how an implementation can
implement both interfaces and register their implementations:

```c
static void
viewer_file_editable_lossy_compress (ViewerEditableLossy *editable)
{
  ViewerFile *self = VIEWER_FILE (editable);

  g_print ("File implementation of lossy editable interface compress method: %s.\n",
           self->filename);
}

static void
viewer_file_editable_lossy_interface_init (ViewerEditableLossyInterface *iface)
{
  iface->compress = viewer_file_editable_lossy_compress;
}

static void
viewer_file_editable_save (ViewerEditable  *editable,
                           GError         **error)
{
  ViewerFile *self = VIEWER_FILE (editable);

  g_print ("File implementation of editable interface save method: %s.\n",
           self->filename);
}

static void
viewer_file_editable_undo (ViewerEditable *editable,
                           guint           n_steps)
{
  ViewerFile *self = VIEWER_FILE (editable);

  g_print ("File implementation of editable interface undo method: %s.\n",
           self->filename);
}

static void
viewer_file_editable_redo (ViewerEditable *editable,
                           guint           n_steps)
{
  ViewerFile *self = VIEWER_FILE (editable);

  g_print ("File implementation of editable interface redo method: %s.\n",
           self->filename);
}

static void
viewer_file_editable_interface_init (ViewerEditableInterface *iface)
{
  iface->save = viewer_file_editable_save;
  iface->undo = viewer_file_editable_undo;
  iface->redo = viewer_file_editable_redo;
}

static void
viewer_file_class_init (ViewerFileClass *klass)
{
  /* Nothing here. */
}

static void
viewer_file_init (ViewerFile *self)
{
  /* Instance variable initialisation code. */
}

G_DEFINE_TYPE_WITH_CODE (ViewerFile, viewer_file, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (VIEWER_TYPE_EDITABLE,
                                                viewer_file_editable_interface_init)
                         G_IMPLEMENT_INTERFACE (VIEWER_TYPE_EDITABLE_LOSSY,
                                                viewer_file_editable_lossy_interface_init))
```

It is very important to notice that the order in which interface
implementations are added to the main object is not random:
`g_type_add_interface_static()`, which is called by `G_IMPLEMENT_INTERFACE`, must
be invoked first on the interfaces which have no prerequisites and then on
the others.

### Interface properties

GObject interfaces can also have properties. Declaration of the interface
properties is similar to declaring the properties of ordinary GObject types
as explained in the section called ["Object
properties"](concepts.html#object-properties), except that
`g_object_interface_install_property()` is used to declare the properties
instead of `g_object_class_install_property()`.

To include a property named 'autosave-frequency' of type gdouble in the
`ViewerEditable` interface example code above, we only need to add one call in
`viewer_editable_default_init()` as shown below:

```c
static void
viewer_editable_default_init (ViewerEditableInterface *iface)
{
  g_object_interface_install_property (iface,
    g_param_spec_double ("autosave-frequency",
                         "Autosave frequency",
                         "Frequency (in per-seconds) to autosave backups of the editable content at. "
                         "Or zero to disable autosaves.",
                         0.0,  /* minimum */
                         G_MAXDOUBLE,  /* maximum */
                         0.0,  /* default */
                         G_PARAM_READWRITE));
}
```

One point worth noting is that the declared property wasn't assigned an
integer ID. The reason being that integer IDs of properties are used only
inside the `get_property` and `set_property` virtual methods. Since interfaces
declare but do not implement properties, there is no need to assign integer
IDs to them.

An implementation declares and defines its properties in the usual way as
explained in the section called “Object properties”, except for one small
change: it can declare the properties of the interface it implements using
`g_object_class_override_property()` instead of `g_object_class_install_property()`.
The following code snippet shows the modifications needed in the `ViewerFile`
declaration and implementation above:

```c
struct _ViewerFile
{
  GObject parent_instance;

  double autosave_frequency;
};

enum
{
  PROP_AUTOSAVE_FREQUENCY = 1,
  N_PROPERTIES
};

static void
viewer_file_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  ViewerFile *file = VIEWER_FILE (object);

  switch (prop_id)
    {
    case PROP_AUTOSAVE_FREQUENCY:
      file->autosave_frequency = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
viewer_file_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  ViewerFile *file = VIEWER_FILE (object);

  switch (prop_id)
    {
    case PROP_AUTOSAVE_FREQUENCY:
      g_value_set_double (value, file->autosave_frequency);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
viewer_file_class_init (ViewerFileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = viewer_file_set_property;
  object_class->get_property = viewer_file_get_property;

  g_object_class_override_property (object_class, PROP_AUTOSAVE_FREQUENCY, "autosave-frequency");
}
```

### Overriding interface methods

If a base class already implements an interface and a derived class needs to
implement the same interface but needs to override certain methods, you must
reimplement the interface and set only the interface methods which need
overriding.

In this example, `ViewerAudioFile` is derived from `ViewerFile`. Both implement
the `ViewerEditable` interface. `ViewerAudioFile` only implements one method of
the `ViewerEditable` interface and uses the base class implementation of the
other.

```c
static void
viewer_audio_file_editable_save (ViewerEditable  *editable,
                                 GError         **error)
{
  ViewerAudioFile *self = VIEWER_AUDIO_FILE (editable);

  g_print ("Audio file implementation of editable interface save method.\n");
}

static void
viewer_audio_file_editable_interface_init (ViewerEditableInterface *iface)
{
  /* Override the implementation of save(). */
  iface->save = viewer_audio_file_editable_save;

  /*
   * Leave iface->undo and ->redo alone, they are already set to the
   * base class implementation.
   */
}

G_DEFINE_TYPE_WITH_CODE (ViewerAudioFile, viewer_audio_file, VIEWER_TYPE_FILE,
                         G_IMPLEMENT_INTERFACE (VIEWER_TYPE_EDITABLE,
                                                viewer_audio_file_editable_interface_init))

static void
viewer_audio_file_class_init (ViewerAudioFileClass *klass)
{
  /* Nothing here. */
}

static void
viewer_audio_file_init (ViewerAudioFile *self)
{
  /* Nothing here. */
}
```

To access the base class interface implementation use
`g_type_interface_peek_parent()` from within an interface's `default_init`
function.

To call the base class implementation of an interface method from a derived
class where than interface method has been overridden, stash away the
pointer returned from `g_type_interface_peek_parent()` in a global variable.

In this example `ViewerAudioFile` overrides the save interface method. In
its overridden method it calls the base class implementation of the same
interface method.

```c
static ViewerEditableInterface *viewer_editable_parent_interface = NULL;

static void
viewer_audio_file_editable_save (ViewerEditable  *editable,
                                 GError         **error)
{
  ViewerAudioFile *self = VIEWER_AUDIO_FILE (editable);

  g_print ("Audio file implementation of editable interface save method.\n");

  /* Now call the base implementation */
  viewer_editable_parent_interface->save (editable, error);
}

static void
viewer_audio_file_editable_interface_init (ViewerEditableInterface *iface)
{
  viewer_editable_parent_interface = g_type_interface_peek_parent (iface);

  iface->save = viewer_audio_file_editable_save;
}

G_DEFINE_TYPE_WITH_CODE (ViewerAudioFile, viewer_audio_file, VIEWER_TYPE_FILE,
                         G_IMPLEMENT_INTERFACE (VIEWER_TYPE_EDITABLE,
                                                viewer_audio_file_editable_interface_init))

static void
viewer_audio_file_class_init (ViewerAudioFileClass *klass)
{
  /* Nothing here. */
}

static void
viewer_audio_file_init (ViewerAudioFile *self)
{
  /* Nothing here. */
}
```

## How to create and use signals

The signal system in GType is pretty complex and flexible: it is possible
for its users to connect at runtime any number of callbacks (implemented in
any language for which a binding exists) to any signal and to stop the
emission of any signal at any state of the signal emission process. This
flexibility makes it possible to use GSignal for much more than just
emitting signals to multiple clients.

### Simple use of signals

The most basic use of signals is to implement event notification. For
example, given a `ViewerFile` object with a write method, a signal could be
emitted whenever the file is changed using that method. The code below shows
how the user can connect a callback to the "changed" signal.

```c
file = g_object_new (VIEWER_FILE_TYPE, NULL);

g_signal_connect (file, "changed", (GCallback) changed_event, NULL);

viewer_file_write (file, buffer, strlen (buffer));
```

The ViewerFile signal is registered in the `class_init` function:

```c
file_signals[CHANGED] = 
  g_signal_newv ("changed",
                 G_TYPE_FROM_CLASS (object_class),
                 G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                 NULL /* closure */,
                 NULL /* accumulator */,
                 NULL /* accumulator data */,
                 NULL /* C marshaller */,
                 G_TYPE_NONE /* return_type */,
                 0     /* n_params */,
                 NULL  /* param_types */);
```

and the signal is emitted in `viewer_file_write`:

```c
void
viewer_file_write (ViewerFile   *self,
                   const guint8 *buffer,
                   gsize         size)
{
  g_return_if_fail (VIEWER_IS_FILE (self));
  g_return_if_fail (buffer != NULL || size == 0);

  /* First write data. */

  /* Then, notify user of data written. */
  g_signal_emit (self, file_signals[CHANGED], 0 /* details */);
}
```

As shown above, the details parameter can safely be set to zero if no detail
needs to be conveyed. For a discussion of what it can be used for, see the
section called [“The detail argument”](concepts.html#the-detail-argument).

The C signal marshaller should always be `NULL`, in which case the best
marshaller for the given closure type will be chosen by GLib. This may be an
internal marshaller specific to the closure type, or
`g_cclosure_marshal_generic()`, which implements generic conversion of arrays of
parameters to C callback invocations. GLib used to require the user to write
or generate a type-specific marshaller and pass that, but that has been
deprecated in favour of automatic selection of marshallers.

Note that `g_cclosure_marshal_generic()` is slower than non-generic
marshallers, so should be avoided for performance critical code. However,
performance critical code should rarely be using signals anyway, as signals
are synchronous, and the emission blocks until all listeners are invoked,
which has potentially unbounded cost.
