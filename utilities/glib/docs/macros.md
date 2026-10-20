GLib's configure options and corresponding macros
=================================================

The following Meson configure options will result in certain macros or options
being defined at build time:

`--buildtype={plain,release,minsize,custom}`
 : No special macros or options

`--buildtype={debug,debugoptimized}` (`debugoptimized` is the default)
 : `-DG_ENABLE_DEBUG -g`

`-Dglib_debug=disabled`
 : Omits `G_ENABLE_DEBUG` when implied by `--buildtype`/`-Ddebug`

`-Dglib_debug=enabled`
 : Defines `G_ENABLE_DEBUG` regardless of `--buildtype`/`-Ddebug`

`-Dglib_asserts=false`
 : `-DG_DISABLE_ASSERT`

`-Dglib_checks=false`
 : `-DG_DISABLE_CHECKS`

Besides these, there are some local feature specific options, but the main
focus here is to concentrate on macros that affect overall GLib behaviour
and/or third party code.


GLib's internal and global macros
=================================

`G_DISABLE_ASSERT`
---

The `g_assert()` and `g_assert_not_reached()` macros become non-functional
with this define. The motivation is to speed up end-user apps by
avoiding expensive checks.

This macro can affect third-party code. Defining it when building GLib
will only disable the assertion macros for GLib itself, but third-party code
that passes `-DG_DISABLE_ASSERT` to the compiler in its own build
will end up with the non-functional variants after including `glib.h`
as well.

Note: Code inside the assertion macros should not have side effects
that affect the operation of the program, as they may get compiled out.

`G_DISABLE_CHECKS`
---

This macro is similar to `G_DISABLE_ASSERT`, it affects third-party
code as mentioned above and the note about `G_DISABLE_ASSERT` applies
too.

The macros that become non-functional here are `g_return_if_fail()`,
`g_return_val_if_fail()`, `g_return_if_reached()` and
`g_return_val_if_reached()`.

This macro also switches off certain checks in the GSignal code.

`G_ENABLE_DEBUG`
---

Quite a bit of additional debugging code is compiled into GLib when this
macro is defined, and since it is a globally visible define, third-party code
may be affected by it similarly to `G_DISABLE_ASSERT`.

Some of these checks can be relatively expensive at runtime, as they affect
every GObject type cast. Distributions are recommended to disable
`G_ENABLE_DEBUG` in stable release builds.

The additional code executed/compiled for this macro currently includes the
following, but this is not an exhaustive list:
 - extra validity checks for `GDate`
 - breakpoint abortion for fatal log levels in `gmessages.c` instead of
   plain `abort()` to allow debuggers trapping and overriding them
 - added verbosity of `gscanner.c` to catch deprecated code paths
 - added verbosity of `gutils.c` to catch deprecated code paths
 - object and type bookkeeping in `gobject.c`
 - extra validity checks in `gsignal.c`
 - support for tracking still-alive `GTask`s
