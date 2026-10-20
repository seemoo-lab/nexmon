Title: Building GLib

# Building GLib

GLib uses the [Meson build system](https://mesonbuild.com). The normal
sequence for compiling and installing the GLib library is thus:

    $ meson setup _build
    $ meson compile -C _build
    $ meson install -C _build

On FreeBSD, you will need something more complex:

    $ env CPPFLAGS="-I/usr/local/include" LDFLAGS="-L/usr/local/lib -Wl,--disable-new-dtags" \
    > meson setup \
    > -Dxattr=false \
    > -Dinstalled_tests=true \
    > _build
    $ meson compile -C _build

The standard options provided by Meson may be passed to the `meson` command. Please see the Meson documentation or run:

    meson configure --help

for information about the standard options.

GLib is compiled with
[strict aliasing](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html#index-fstrict-aliasing)
disabled. It is strongly recommended that this is not re-enabled by overriding
the compiler flags, as GLib has not been tested with strict aliasing and cannot
be guaranteed to work.

## Dependencies

Before you can compile the GLib library, you need to have various other
tools and libraries installed on your system. If you are building from a
release archive, you will need a [compliant C
toolchain](https://gitlab.gnome.org/GNOME/glib/-/blob/main/docs/toolchain-requirements.md),
Meson, and pkg-config; the requirements are the same when building from a
Git repository clone of GLib.

- [`pkg-config`](https://www.freedesktop.org/wiki/Software/pkg-config/) is a
  tool for tracking the compilation flags needed for libraries that are used
  by the GLib library. (For each library, a small `.pc` text file is
  installed in a standard location that contains the compilation flags
  needed for that library along with version number information). 

A UNIX build of GLib requires that the system implements at least the
original 1990 version of POSIX. Beyond this, it depends on a number of other
libraries.

- The [GNU libiconv library](http://www.gnu.org/software/libiconv/) is
  needed to build GLib if your system doesn't have the `iconv()` function
  for doing conversion between character encodings. Most modern systems
  should have `iconv()`, however many older systems lack an `iconv()`
  implementation. On such systems, you must install the libiconv library.
  This can be found at: http://www.gnu.org/software/libiconv.

  If your system has an `iconv()` implementation but you want to use libiconv
  instead, make sure it is installed to the default compiler header/library
  search path (for instance, in `/usr/local/`). The `iconv.h` that libiconv
  installs hides the system iconv. Meson then detects this, recognizes that the
  system iconv is unusable and the external one is mandatory, and automatically
  forces it to be used.

  If you are using the native iconv implementation on Solaris instead of
  libiconv, you'll need to make sure that you have the converters between
  locale encodings and UTF-8 installed. At a minimum you'll need the
  SUNWuiu8 package. You probably should also install the SUNWciu8, SUNWhiu8,
  SUNWjiu8, and SUNWkiu8 packages.

  The native iconv on Compaq Tru64 doesn't contain support for UTF-8, so
  you'll need to use GNU libiconv instead. (When using GNU libiconv for
  GLib, you'll need to use GNU libiconv for GNU gettext as well.) This
  probably applies to related operating systems as well.

- Python 3.5 or newer is required. Your system Python must conform to
  [PEP 394](https://www.python.org/dev/peps/pep-0394/) For FreeBSD, this means
  that the `lang/python3` port must be installed.

- The libintl library from the [GNU
  gettext](http://www.gnu.org/software/gettext) package is needed if your
  system doesn't have the `gettext()` functionality for handling message
  translation databases.

- A thread implementation is needed. The thread support in GLib can be based
  upon POSIX threads or win32 threads.

- GRegex uses the [PCRE library](http://www.pcre.org/) for regular
  expression matching. The system version of PCRE is used, unless not available
  (which is the case on Android), in which case a fallback subproject is used.

- The optional extended attribute support in GIO requires the `getxattr()`
  family of functions that may be provided by the C library or by the
  standalone libattr library. To build GLib without extended attribute
  support, use the `-Dxattr=false` option.

- The optional SELinux support in GIO requires libselinux. To build GLib
  without SELinux support, use the `-Dselinux=disabled` option.

- The optional support for DTrace requires the `sys/sdt.h` header, which is
  provided by SystemTap on Linux. To build GLib without DTrace, use the
  `-Ddtrace=false` option.

- The optional support for SystemTap can be disabled with the
  `-Dsystemtap=false` option. Additionally, you can control the location
  where GLib installs the SystemTap probes, using the
  `-Dtapset_install_dir=DIR` option.

- [gobject-introspection](https://gitlab.gnome.org/GNOME/gobject-introspection/)
  is needed to generate introspection data for consumption by other projects,
  and to generate the GLib documentation via
  [gi-docgen](https://gitlab.gnome.org/GNOME/gi-docgen). There is a dependency
  cycle between GLib and gobject-introspection. This can be broken by building
  GLib first with `-Dintrospection=disabled`, then building
  gobject-introspection against this copy of GLib, then re-building GLib against
  the new gobject-introspection with `-Dintrospection=enabled`. The GLib API
  documentation can be built during this second build process if
  `-Ddocumentation=true` is also set.

## Extra Configuration Options

In addition to the normal options, these additional ones are supported when
configuring the GLib library:

`--buildtype`
: This is a standard Meson option which specifies how much debugging and
  optimization to enable. If the build type is `debug`, `G_ENABLE_DEBUG` will be
  defined and GLib will be built with additional debug code enabled. You can
  override this behavior using `-Dglib_debug`.

`-Dforce_posix_threads=true`
: Normally, Meson should be able to work out the correct thread implementation
  to use. This option forces POSIX threads to be used even if the platform
  provides another threading API (for example, on Windows).

`-Dbsymbolic_functions=false` and `-Dbsymbolic_functions=true`
: By default, GLib uses the `-Bsymbolic-functions` linker flag to avoid
  intra-library PLT jumps. A side-effect of this is that it is no longer
  possible to override internal uses of GLib functions with `LD_PRELOAD`.
  Therefore, it may make sense to turn this feature off in some
  situations. The `-Dbsymbolic_functions=false` option allows to do that.

`-Ddocumentation=false` and `-Ddocumentation=true`
: By default, GLib will not build documentation for the library and tools. This
  option can be used to enable building the documentation.

`-Dman-pages=disabled` and `-Dman-pages=enabled`
: By default, GLib will detect whether `rst2man` and the necessary DocBook
  stylesheets are installed. If they are, then it will use them to build
  the included man pages from the reStructuredText sources. These options can be
  used to explicitly control whether man pages should be built and used.

`-Dxattr=false` and `-Dxattr=true`
: By default, GLib will detect whether the `getxattr()` family of functions is
  available. If it is, then extended attribute support will be included in
  GIO. These options can be used to explicitly control whether extended
  attribute support should be included or not. `getxattr()` and friends can be
  provided by glibc or by the standalone libattr library.

`-Dselinux=auto`, `-Dselinux=enabled` or `-Dselinux=disabled`
: By default, GLib will detect if libselinux is available and include SELinux
  support in GIO if it is. These options can be used to explicitly control
  whether SELinux support should be included.

`-Ddtrace=false` and `-Ddtrace=true`
: By default, GLib will detect if DTrace support is available, and use it.
  These options can be used to explicitly control whether DTrace support is
  compiled into GLib.

`-Dsystemtap=false` and `-Dsystemtap=true`
: This option requires DTrace support. If it is available, then GLib will also
  check for the presence of SystemTap.

`-Db_coverage=true` and `-Db_coverage=false`
: Enable the generation of coverage reports for the GLib tests. This requires
  the lcov frontend to gcov from the Linux Test Project. To generate a
  coverage report, use `ninja coverage-html`. The report is placed in the
  `meson-logs` directory.
