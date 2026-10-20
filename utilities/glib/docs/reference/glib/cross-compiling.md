Title: Cross-compiling the GLib package

# Cross-compiling the GLib Package

## Building the Library for a different architecture

Cross-compilation is the process of compiling a program or library on a
different architecture or operating system then it will be run upon. GLib is
slightly more difficult to cross-compile than many packages because much of
GLib is about hiding differences between different systems.

These notes cover things specific to cross-compiling GLib; for general
information about cross-compilation, see the [Meson
documentation](http://mesonbuild.com/Cross-compilation.html).

GLib tries to detect as much information as possible about the target system
by compiling and linking programs without actually running anything;
however, some information GLib needs is not available this way. This
information needs to be provided to meson via a ‘cross file’.

As an example of using a cross file, to cross compile for the ‘MingW32’
Win64 runtime environment on a Linux system, create a file `cross_file.txt`
with the following contents:

```
[host_machine]
system = 'windows'
cpu_family = 'x86_64'
cpu = 'x86_64'
endian = 'little'

[properties]
c_args = []
c_link_args = []

[binaries]
c = 'x86_64-w64-mingw32-gcc'
cpp = 'x86_64-w64-mingw32-g++'
ar = 'x86_64-w64-mingw32-ar'
ld = 'x86_64-w64-mingw32-ld'
objcopy = 'x86_64-w64-mingw32-objcopy'
strip = 'x86_64-w64-mingw32-strip'
pkgconfig = 'x86_64-w64-mingw32-pkg-config'
windres = 'x86_64-w64-mingw32-windres'
```

Then execute the following commands:

    meson setup --cross-file cross_file.txt builddir

The complete list of cross properties follows. Most of these won't need to
be set in most cases.

## Cross properties

`have_[function]`
: When meson checks if a function is supported, the test can be overridden by
  setting the `have_function` property to `true` or `false`. For example:

    Checking for function "fsync" : YES

  can be overridden by setting

    have_fsync = false

`growing_stack=[true/false]`
: Whether the stack grows up or down. Most places will want `false`. A few
  architectures, such as PA-RISC need `true`.

`have_strlcpy=[true/false]`
: Whether you have `strlcpy()` that matches OpenBSD. Defaults to `false`,
  which is safe, since GLib uses a built-in version in that case.

`va_val_copy=[true/false]`
: Whether `va_list` can be copied as a pointer. If set to `false`, then
  `memcopy()` will be used. Only matters if you don't have `va_copy()` or
  `__va_copy()`. (So, doesn't matter for GCC.) Defaults to `true` which is
  slightly more common than `false`.

`have_c99_vsnprintf=[true/false]`
: Whether you have a `vsnprintf()` with C99 semantics. (C99 semantics means
  returning the number of bytes that would have been written had the output
  buffer had enough space.) Defaults to `false`.

`have_c99_snprintf=[true/false]`
: Whether you have a `snprintf()` with C99 semantics. (C99 semantics means
  returning the number of bytes that would have been written had the output
  buffer had enough space.) Defaults to `false`.
