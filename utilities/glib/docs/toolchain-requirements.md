Toolchain/Compiler requirements
===

GLib requires a toolchain that supports C11.

GLib contains some fall back code that allows supporting toolchains that are not
fully C11-compatible.

GLib makes some assumptions about features of the C library and C preprocessor,
compiler and linker that may go beyond what C11 mandates.  We will use features
beyond C11 if they are substantially useful and if they are supported in a wide
range of compilers.

In general, we are primarily interested in supporting these four compilers:

 * GCC on *nix
 * Clang (LLVM)
 * MSVC
 * mingw32-w64

This is in keeping with our goal of primarily targeting GNU/Linux, Windows and
Mac OS, along with Free Software POSIX-compliant operating systems.  See
[Supported platforms](./supported-platforms.md) for a bit more information and
rationale about that.

In particular, we are interested in MSVC because, although there are other
compilers which target Windows, they do not output debugging information that is
compatible with MSVC.  In interest of usability, we want users of GLib to be
able to debug GLib along with their own code while using MSVC as their
development environment.

At any given time, GLib may work with mingw32 (from mingw.org) but it is not
specifically supported.  Politics aside, it seems that mingw.org is mostly
dormant and, at this point, all of the big distributions have switched over to
mingw32-w64.  In several cases, mingw.org has been missing APIs that we’ve
wanted to use and upstream has not been responsive about adding them.

GLib will attempt to remain compatible with other compilers, but some ‘extra
features’ are assumed.  Those are detailed below.

GLib additionally requires Python 3 to build.

C99 Varargs macros
---

_Hard requirement._

GLib requires C99 ``__VA_ARG__`` support for both C and C++ compilers.

Symbol visibility control
---

_Not a hard requirement._

When available, GLib uses `__attribute__((visibility("hidden")))` and the
`-fvisibility=hidden` compiler option to control symbol visibility, and the
`-Bsymbolic-functions` linker flag.

Builtin atomic operations
---

_Not a hard requirement._

GLib will fall back to using a mutex-based implementation if atomic builtins are
not available.

C99 printf and positional parameters
---

_Not a hard requirement._

GLib can be built with an included printf implementation (from GNUlib) if the
system printf is deficient.

Constructors and destructors
---

_Hard requirement._

GLib can work with pragma-based, as well as with attribute-based constructor
support. There is a fallback for MSVC using a `DllMain()` instead.

`va_list` pass-by-reference
---

_Hard requirement._

GLib depends on the ability to pass-by-reference a `va_list`, as mandated in
C99  § 7.15: “It is permitted to create a pointer to a `va_list` and pass that
pointer to another function, in which case the original function may make
further use of the original list after the other function returns.”

Support for `static inline`
---

_Hard requirement._

GLib depends on implementation of the `inline` keyword as described by
C99 § 6.7.4.

GLib further assumes that functions appearing in header files and marked
`static inline`, but not used in a particular compilation unit will:

 * not generate warnings about being unused
 * not be emitted in the compiler’s output

It is possible that a compiler adheres to C99 § 6.7.4 but not to GLib’s further
assumptions.  Such compilers may produce large numbers of warnings or large
executables when compiling GLib or programs based on GLib.

Support for `alloca()`
---

_Hard requirement._

Your compiler must support `alloca()`, defined in `<alloca.h>` (or `<malloc.h>`
on Windows) and it must accept a non-constant argument.

C11 support for type redefinition
---

_Hard requirement._

Your compiler must allow “a typedef name [to] be redefined to denote the same
type as it currently does”, as per C11 §6.7, item 3.

‘Big’ enums
---

_Hard requirement._

Some of our enum types use `1<<31` as a value. We also use negative values in
enums. We rely on the compiler to choose a suitable storage size for the enum
that can accommodate this.

Selected C99 features
---

_Hard requirement._

Starting with GLib 2.52 and GTK 3.90, we will be using the following C99
features where appropriate:

 * Compound literals
 * Designated initializers
 * Mixed declarations

Function pointer conversions
---

_Hard requirement._

GLib heavily relies on the ability to convert a function pointer to a `void*`
and back again losslessly. Any platform or compiler which doesn’t support this
cannot be used to compile GLib or code which uses GLib. This precludes use of
the `-pedantic` GCC flag with GLib.

Calling functions through differently typed function pointers
---
_Hard requirement_

GLib heavily relies on the ability to cast a function to a differently-typed
function pointer and call it.  Specifically, all of the following must work:

- Calling a function that takes one type of pointer through a function pointer
  that takes a different type of pointer:

  ```c
  typedef void (*callback_type) (void *);
  void func (char *x);
  void *p = NULL;
  ((callback_type) func) (p);
  ```

- Calling a function with additional arguments:

  ```c
  typedef void (*callback_type) (void *, void *);
  void func (GtkWidget *);
  ((callback_type) func) (some_widget, some_pointer_that_will_be_ignored);
  ```

- Calling a function with too few arguments, provided that the function
  only uses arguments that are actually provided:

  ```c
  typedef void (*callback_type) (void *);
  void
  func (void *a, void *b)
  {
    /* b is unused */
    do_something (a);
  }
  ((callback_type) func) (some_pointer);
  ```

Most native toolchains meet this requirement. When using Emscripten,
`-sEMULATE_FUNCTION_POINTER_CASTS` is required.  This causes a performance
penalty.

NULL defined as void pointer, or same size and representation as void pointer
---

_Hard requirement._

GLib assumes that it is safe to pass `NULL` to a variadic function without
explicitly casting it to a char * pointer, e.g.:

```
g_object_new (G_TYPE_FOO,
              "bar", bar,
              NULL);
```

This pattern is pervasive throughout GLib and applications that use GLib, but it
is also nonportable. If `NULL` is defined as an integer type, `0`, rather than a
pointer type, `(void *) 0`, then using an integer where a pointer is expected
results in stack corruption if the size of a pointer is different from the size
of `NULL`. Portable code would use `(char *) NULL`, `(void *) NULL` or `nullptr`
(in C23 or C++11 or later) instead.

This requirement technically only applies to C, since GLib is written in C and
since C++ does not permit `NULL` to be defined as a void pointer. If you are
writing C++ code that uses GLib, use `nullptr` to avoid this problem.

GLib further assumes that the representations of pointers of different types are
identical, which is not guaranteed.

`stdint.h`
---

_Hard requirement since GLib 2.67.x._

GLib [requires a C99 `stdint.h`](https://gitlab.gnome.org/GNOME/glib/-/merge_requests/1675)
with all the usual sized integer types (`int8_t`, `uint64_t` and so on),
believed to be supported by all relevant Unix platforms/compilers, as well as
Microsoft compilers since MSVC 2013.
