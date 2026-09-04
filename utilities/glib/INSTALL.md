Simple install procedure
========================

```sh
tar xf glib-*.tar.gz                    # unpack the sources
cd glib-*                               # change to the toplevel directory
meson setup _build                      # configure the build
meson compile -C _build                 # build GLib

# Become root if necessary

meson install -C _build                 # install GLib
```

Requirements
============

GLib requires a [basic C toolchain](./docs/toolchain-requirements.md) and
support for a minimum version of the C standard.

GLib-2.0 requires pkg-config, which is tool for tracking the
compilation flags needed for libraries. (For each library, a small `.pc`
text file is installed in a standard location that contains the
compilation flags needed for that library along with version number
information.) Information about pkg-config can be found at:

  http://www.freedesktop.org/software/pkgconfig/

Meson (http://mesonbuild.com/) is also required. If your distribution does not
package a new enough version of Meson, it can be [installed using
`pip`](https://mesonbuild.com/Getting-meson.html#installing-meson-with-pip).

In order to implement conversions between character sets,
GLib requires an implementation of the standard `iconv()` routine.
Most modern systems will have a suitable implementation, however
many older systems lack an `iconv()` implementation. On such systems,
you must install the libiconv library. This can be found at:

 http://www.gnu.org/software/libiconv/

If your system has an iconv implementation but you want to use
libiconv instead, you can pass the `--with-libiconv` option to
configure. This forces libiconv to be used.

Note that if you have libiconv installed in your default include
search path (for instance, in `/usr/local/`), but don't enable
it, you will get an error while compiling GLib because the
`iconv.h` that libiconv installs hides the system iconv.

If you are using the native iconv implementation on Solaris
instead of libiconv, you'll need to make sure that you have
the converters between locale encodings and UTF-8 installed.
At a minimum you'll need the `SUNWuiu8` package. You probably
should also install the `SUNWciu8`, `SUNWhiu8`, `SUNWjiu8`, and
`SUNWkiu8` packages.

The native iconv on Compaq Tru64 doesn't contain support for
UTF-8, so you'll need to use GNU libiconv instead. (When
using GNU libiconv for GLib, you'll need to use GNU libiconv
for GNU gettext as well.) This probably applies to related
operating systems as well.

Finally, for message catalog handling, GLib requires an implementation
of `gettext()`. If your system doesn't provide this functionality,
you should use the libintl library from the GNU gettext package,
available from:

 http://www.gnu.org/software/gettext/

Support for extended attributes and SELinux in GIO requires
libattr and libselinux.

Some of the mimetype-related functionality in GIO requires the
`update-mime-database` and `update-desktop-database` utilities, which
are part of shared-mime-info and desktop-file-utils, respectively.

GObject uses libffi to implement generic marshalling functionality.

The Nitty-Gritty
================

Complete information about installing GLib can be found
in the file:

 docs/reference/glib/glib-2.0/building.html

Or online at:

 https://docs.gtk.org/glib/building.html


Installation directories
========================

The location of the installed files is determined by the `--prefix`
and `--exec-prefix` options given to configure. There are also more
detailed flags to control individual directories. However, the
use of these flags is not tested.

One particular detail to note, is that the architecture-dependent
include file `glibconfig.h` is installed in `$libdir/glib-2.0/include/`.

`.pc` files for the various libraries are installed in
`$libdir/pkgconfig` to provide information when compiling
other packages that depend on GLib. If you set `PKG_CONFIG_PATH`
so that it points to this directory, then you can get the
correct include flags and library flags for compiling a GLib
application with:

```sh
pkg-config --cflags glib-2.0
pkg-config --libs glib-2.0
```

This is the only supported way of determining the include and library flags
for building against GLib.


Cross-compiling GLib
====================

Information about cross-compilation of GLib can be found
in the file:

 docs/reference/glib/html/glib-cross-compiling.html

Or online at:

 https://docs.gtk.org/glib/cross-compiling.html
