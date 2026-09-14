Title: Compiling GLib Applications

# Compiling GLib Applications

To compile a GLib application, you need to tell the compiler where to find
the GLib header files and libraries. This is done with the
[`pkg-config`](https://www.freedesktop.org/wiki/Software/pkg-config/)
utility.

The following interactive shell session demonstrates how pkg-config is used
(the actual output on your system may be different):

    $ pkg-config --cflags glib-2.0
    -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include
    $ pkg-config --libs glib-2.0
    -L/usr/lib -lm -lglib-2.0

See the `pkg-config` website for more information about `pkg-config`.

If your application uses or GObject features, it must be compiled and linked
with the options returned by the following `pkg-config` invocation:

    $ pkg-config --cflags --libs gobject-2.0

If your application uses modules, it must be compiled and linked with the
options returned by one of the following pkg-config invocations:

    $ pkg-config --cflags --libs gmodule-no-export-2.0
    $ pkg-config --cflags --libs gmodule-2.0

The difference between the two is that `gmodule-2.0` adds `--export-dynamic`
to the linker flags, which is often not needed.

The simplest way to compile a program is to use the "backticks" feature of
the shell. If you enclose a command in backticks (not single quotes), then
its output will be substituted into the command line before execution. So to
compile a GLib Hello, World, you would type the following:

    $ cc `pkg-config --cflags glib-2.0` hello.c -o hello `pkg-config --libs glib-2.0`

Deprecated GLib functions are annotated to make the compiler emit warnings
when they are used (e.g. with GCC, you need to use the
`-Wdeprecated-declarations option`). If these warnings are problematic, they
can be turned off by defining the preprocessor symbol
`GLIB_DISABLE_DEPRECATION_WARNINGS` by using the commandline option
`-DGLIB_DISABLE_DEPRECATION_WARNINGS`

GLib deprecation annotations are versioned; by defining the macros
`GLIB_VERSION_MIN_REQUIRED` and `GLIB_VERSION_MAX_ALLOWED`, you can specify the
range of GLib versions whose API you want to use. APIs that were deprecated
before or introduced after this range will trigger compiler warnings.

Since GLib 2.62, the older deprecation mechanism of hiding deprecated
interfaces entirely from the compiler by using the preprocessor symbol
`G_DISABLE_DEPRECATED` has been removed. All deprecations are now handled
using the above mechanism.

The recommended way of using GLib has always been to only include the
toplevel headers `glib.h`, `glib-object.h`, `gio.h`. Starting with 2.32, GLib
enforces this by generating an error when individual headers are directly
included.

Still, there are some exceptions; these headers have to be included
separately:

- `gmodule.h`
- `glib-unix.h`
- `glib/gi18n-lib.h` or `glib/gi18n.h` (see the section on
  [Internationalization](i18n.html))
- `glib/gprintf.h` and `glib/gstdio.h` (we don't want to pull in all of
  stdio)
