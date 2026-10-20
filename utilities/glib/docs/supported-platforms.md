Supported platforms
===

GLib’s approach to portability is that we only support systems that we can test.
That means that either a large number of GLib developers are regularly using
GLib on a particular system, or we have regular builds of GLib on that system.

Minimum versions
---

This list is authoritative, and documents what this version of GLib targets to
support. The list will be periodically updated for the development branch,
with versions typically being updated as they lapse from receiving support from
their vendor.

 * macOS: minimum version macOS 11.
   * [No support for universal binaries ](https://bugzilla.gnome.org/show_bug.cgi?id=780238);
   * macOS 10.13 support is maintained on a best-effort basis.
 * Windows: [minimum version is Windows 10](https://gitlab.gnome.org/GNOME/glib/-/work_items/3889),
   minimum build chain is Visual Studio 2019 16.8.x
   * Visual Studio 2015/2017 support is maintained on a best-effort basis; not recommended
   for introspection.
 * Android: [minimum NDK version 15](https://gitlab.gnome.org/GNOME/glib/issues/1113)
 * Linux: glibc newer than 2.5 (if using glibc; other forms of libc are supported)

Tested platforms
---

GLib is regularly built on at least the following systems:

 * GNOME OS Nightly: https://os.gnome.org/
 * Fedora: http://koji.fedoraproject.org/koji/packageinfo?packageID=382
 * Ubuntu: http://packages.ubuntu.com/source/glib2.0
 * Debian: https://packages.debian.org/experimental/libglib2.0-0
 * FreeBSD: https://cgit.freebsd.org/ports/tree/devel/glib20
 * openSUSE: https://build.opensuse.org/package/show/GNOME:Factory/glib2
 * CI runners, https://gitlab.gnome.org/GNOME/glib/blob/main/.gitlab-ci.yml:
   - Fedora (41, https://gitlab.gnome.org/GNOME/glib/-/blob/main/.gitlab-ci/fedora.Dockerfile)
   - Debian (Bookworm, https://gitlab.gnome.org/GNOME/glib/-/blob/main/.gitlab-ci/debian-stable.Dockerfile)
   - Alpine Linux (3.19 using muslc, https://gitlab.gnome.org/GNOME/glib/-/blob/main/.gitlab-ci/alpine.Dockerfile)
   - Windows (MinGW64)
   - Windows (msys2-clang64 and msys2-mingw32; msys2 is a rolling release distribution)
   - Windows (Visual Studio 2019 x64, a static linking version on x64, and an x86 and an ARM64 version)
   - Android (NDK r23b, API 31, arm64, https://gitlab.gnome.org/GNOME/glib/-/blob/main/.gitlab-ci/android-ndk.sh)
   - FreeBSD (13)
   - macOS (arm64, SDK 11.3)

If other platforms are to be supported, we need to set up regular CI testing for
them. Please contact us if you want to help.

Policy and rationale
---

Due to their position in the market, we consider supporting GNU/Linux, Windows
and macOS to be the highest priorities and we will go out of our way to
accommodate these systems, even in places that they are contravening standards.

In general, we are open to the idea of supporting any Free Software UNIX-like
system with good POSIX compliance.  We are always interested in receiving
patches that improve our POSIX compliance — if there is a good POSIX equivalent
for a platform-specific API that we’re using, then all other things equal, we
prefer the POSIX one.

We may use a non-POSIX API available on one or more of our supported systems in
the case that it provides some advantage over the POSIX equivalent (such as the
case with `pipe2()` solving the `O_CLOEXEC` race).  In these cases, we will try
to provide a fallback to the pure POSIX approach.  If we’ve used a
system-specific API without providing a fallback to a largely-equivalent POSIX
API then it is likely a mistake, and we’re happy to receive a patch to fix it.

We are not interested in supporting other systems if it involves adding code
paths that we cannot test.  Specifically, this means that we will reject patches
that introduce platform-specific `#ifdef` sections in the code unless we are
actively doing builds of GLib on this platform (ie: see the lists above).  We’ve
historically accepted such patches from users of these systems on an ad hoc
basis, but it created an unsustainable situation.  Patches that fix
platform-specific build issues in such a way that the code is improved in the
general case are of course welcome.

Although we aim to support all systems with good POSIX compliance, we are not
interested in adhering to “pure POSIX and nothing else”.  If we need to add a
feature and we can provide good support on all of the platforms that we support
(above), we will not hold back for other systems.  We will always try to provide
a fallback to a POSIX-specified approach, if possible, or to simply replace a
given functionality with a no-op, but even this may not be possible in cases of
critical functionality.

32-bit support
---

GLib will support 32-bit systems as long as they are supported by any of the
tested platforms above. This means we are guaranteed to be able to test our
32-bit build in CI.

In particular, Debian
[continues to support 32-bit installations on x86](https://lists.debian.org/debian-devel-announce/2023/12/msg00003.html)
as containers or chroots, though it doesn’t support running on 32-bit physical
x86 hardware. It also supports a 32-bit armhf kernel and userspace.

As of 2024, other known significant consumers of 32-bit GLib are:
 * [GStreamer on 32-bit Windows](https://gitlab.gnome.org/GNOME/glib/-/issues/3477#note_2235483)
 * [GStreamer on 32-bit armv7](https://gitlab.gnome.org/GNOME/glib/-/issues/3477#note_2236548)
 * [Wine](https://gitlab.gnome.org/GNOME/glib/-/issues/3477#note_2235484)
 * [Steam](https://gitlab.gnome.org/GNOME/glib/-/issues/3477#note_2238744)

Notes on specific features
---

Note that we currently depend on a number of features specified in POSIX, but
listed as optional:

 * [`CLOCK_MONOTONIC`](http://pubs.opengroup.org/onlinepubs/009695399/functions/clock_gettime.html)
   is expected to be present and working
 * [`pthread_condattr_setclock()`](http://pubs.opengroup.org/onlinepubs/7999959899/functions/pthread_condattr_setclock.html)
   is expected to be present and working

Also see [toolchain requirements](./toolchain-requirements.md).
