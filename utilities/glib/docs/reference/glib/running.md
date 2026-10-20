Title: Running GLib Applications

# Running GLib Applications

## Environment variables

The runtime behaviour of GLib applications can be influenced by a number of
environment variables.

Standard variables
:  GLib reads standard environment variables like `LANG`, `PATH`, `HOME`,
   `TMPDIR`, `TZ` and `LOGNAME`.

XDG directories
:  GLib consults the environment variables `XDG_DATA_HOME`,
   `XDG_DATA_DIRS`, `XDG_CONFIG_HOME`, `XDG_CONFIG_DIRS`, `XDG_CACHE_HOME` and
   `XDG_RUNTIME_DIR` for the various XDG directories. For more information, see
   the [XDG basedir specification](https://specifications.freedesktop.org/basedir-spec/latest/).

`G_FILENAME_ENCODING`
:  This environment variable can be set to a comma-separated list of character
   set names. GLib assumes that filenames are encoded in the first character
   set from that list rather than in UTF-8. The special token "@locale" can be
   used to specify the character set for the current locale.

`G_BROKEN_FILENAMES`
:  If this environment variable is set, GLib assumes that filenames are in the
   locale encoding rather than in UTF-8. `G_FILENAME_ENCODING` takes priority
   over `G_BROKEN_FILENAMES`.

`G_MESSAGES_PREFIXED`
:  A list of log levels for which messages should be prefixed by the program
   name and PID of the application. The default is to prefix everything except
   `G_LOG_LEVEL_MESSAGE` and `G_LOG_LEVEL_INFO`. The possible values are error,
   warning, critical, message, info and debug. You can also use the special
   values all and help. This environment variable only affects the default log
   handler, `g_log_default_handler()`.

`G_MESSAGES_DEBUG`
:  A space-separated list of log domains for which informational and debug
   messages should be printed. By default, these messages are not printed. You
   can also use the special value all. This environment variable only affects
   the default log handler, `g_log_default_handler()`.

`G_DEBUG`
:  This environment variable can be set to a list of debug options, which cause
   GLib to print out different types of debugging information.

   - `fatal-warnings`: Causes GLib to abort the program at the first call to
     `g_warning()` or `g_critical()`. Use of this flag is not recommended
     except when debugging.
   - `fatal-criticals`: Causes GLib to abort the program at the first call
     to `g_critical()`. This flag can be useful during debugging and
     testing.
   - `gc-friendly`: Newly allocated memory that isn't directly initialized,
     as well as memory being freed will be reset to 0. The point here is to
     allow memory checkers and similar programs that use Boehm GC alike
     algorithms to produce more accurate results.
   - `resident-modules`: All modules loaded by GModule will be made
     resident. This can be useful for tracking memory leaks in modules which
     are later unloaded; but it can also hide bugs where code is accessed
     after the module would have normally been unloaded.
   - `bind-now-modules`: All modules loaded by GModule will bind their
     symbols at load time, even when the code uses `G_MODULE_BIND_LAZY`.

   The special value `all` can be used to turn on all debug options. The special
   value `help` can be used to print all available options.

`G_SLICE`
:  This environment variable allowed reconfiguration of the GSlice memory
   allocator. Since GLib 2.76, GSlice uses the system `malloc()` implementation
   internally, so this variable is ignored.

`G_RANDOM_VERSION`
:  If this environment variable is set to '2.0', the outdated pseudo-random
   number seeding and generation algorithms from GLib 2.0 are used instead of
   the newer, better ones. You should only set this variable if you have
   sequences of numbers that were generated with Glib 2.0 that you need to
   reproduce exactly.

`LIBCHARSET_ALIAS_DIR`
:  Allows to specify a nonstandard location for the `charset.aliases` file
   that is used by the character set conversion routines. The default
   location is the `libdir` specified at compilation time.

`TZDIR`
:  Allows to specify a nonstandard location for the timezone data files that
   are used by the `GDateTime` API. The default location is under
   `/usr/share/zoneinfo`. For more information, also look at the `tzset` manual
   page.

`G_ENABLE_DIAGNOSTIC`
:  If set to a non-zero value, this environment variable enables diagnostic
   messages, like deprecation messages for GObject properties and signals.

`G_DEBUGGER`
:  When running on Windows, if set to a non-empty string, GLib will try to
   interpret the contents of this environment variable as a command line to a
   debugger, and run it if the process crashes. The debugger command line
   should contain `%p` and `%e` substitution tokens, which GLib will replace
   with the process ID of the crashing process and a handle to an event that
   the debugger should signal to let GLib know that the debugger successfully
   attached to the process. If `%e` is absent, or if the debugger is not able
   to signal events, GLib will resume execution after 60 seconds. If `%p` is
   absent, the debugger won't know which process to attach to, and GLib will
   also resume execution after 60 seconds. Additionally, even if `G_DEBUGGER`
   is not set, GLib would still try to print basic exception information (code
   and address) into `stderr`. By default the debugger gets a new console
   allocated for it. Set the `G_DEBUGGER_OLD_CONSOLE` environment variable to
   any non-empty string to make the debugger inherit the console of the
   crashing process. Normally this is only used by the GLib testsuite. The
   exception handler is written with the aim of making it as simple as
   possible, to minimize the risk of it invoking buggy functions or running
   buggy code, which would result in exceptions being raised recursively.
   Because of that it lacks most of the amenities that one would expect of
   GLib. Namely, it does not support Unicode, so it is highly advisable to
   only use ASCII characters in `G_DEBUGGER`. See also `G_VEH_CATCH`.

`G_VEH_CATCH`
:  Catching some exceptions can break the program, since Windows will
   sometimes use exceptions for execution flow control and other purposes
   other than signalling a crash. The `G_VEH_CATCH` environment variable
   augments Vectored Exception Handling on Windows (see `G_DEBUGGER`),
   allowing GLib to catch more exceptions. Set this variable to a
   comma-separated list of hexadecimal exception codes that should
   additionally be caught. By default GLib will only catch Access Violation,
   Stack Overflow and Illegal Instruction exceptions.

## Locale

A number of interfaces in GLib depend on the current locale in which an
application is running. Therefore, most GLib-using applications should call
`setlocale (LC_ALL, "")` to set up the current locale.

On Windows, in a C program there are several locale concepts that not
necessarily are synchronized. On one hand, there is the system default ANSI
code-page, which determines what encoding is used for file names handled by
the C library's functions and the Win32 API. (We are talking about the
"narrow" functions here that take character pointers, not the "wide" ones.)

On the other hand, there is the C library's current locale. The character
set (code-page) used by that is not necessarily the same as the system
default ANSI code-page. Strings in this character set are returned by
functions like `strftime()`.

## Debugging with GDB

GLib ships with a set of Python macros for the GDB debugger. These
macros make it easier to debug applications written using GLib.

To use this you need to install GLib in the same prefix as GDB so that
the Python GDB autoloaded files get installed in the right place for
GDB to pick up.

You can check if gdb has picked up the GLib gdb scripts correctly by
running the following command in your gdb session.

```
(gdb) info auto-load python-scripts
Loaded  Script
Yes     /usr/share/gdb/auto-load/usr/lib/x86_64-linux-gnu/libglib-2.0.so.0.7800.0-gdb.py
Yes     /usr/share/gdb/auto-load/usr/lib/x86_64-linux-gnu/libgobject-2.0.so.0.7800.0-gdb.py
```

The version numbers and paths might differ based on the OS
distribution, but if you see the entries listed for libglib and
libgobject, then the gdb scripts should work.

GLib Python macros provide the following benefits while debugging
applications written using GLib.

1. Pretty printing GLib types
2. Iterating lists using `gforeach` command
3. Backtrace decorations

### 1. Pretty printing GLib types:

General pretty printing GLib types should work without having to do
anything special.

For example, printing a hash table with string keys will display the hash
contents in the following format.

```
(gdb) print my_string_hash_variable
$1 = 0x5555556d9660 = {
  [0x7ffff76f6592 "GdkWaylandPopup"] = 0x555555768a50,
  [0x7ffff76e67e8 "GtkStyleProvider"] = 0x5555557a19c0,
  [0x7ffff7db7151 "GHttpProxy"] = 0x5555556f5bc0
}
```

Printing a hash table with pointer keys will display the hash contents
in the following format.

```
(gdb) print my_pointer_hash_variable
$2 = 0x5555556dcb50 = {
  [0x555555b59ec0] = 0x555555b59770,
  [0x555555c79f80] = 0x555555c86b00,
  [0x555555c68f40] = 0x555555c69630
}
```

Printing a list will display the list contents in the following
format.

```
(gdb) p my_appname_list
$3 = 0x55c4582deef0 = {0x7f16ab85dc90, 0x7f16abb02700, 0x7f16a80066e0}
```

It's not possible for us to decode and print string entries even if
the list has string entries, as GLib stores list entries as
`gpointer`s internally. But, that can be achieved using the `gforeach`
command.

### 2. Iterating lists using "gforeach" command:

The `gforeach` command can be used to apply a command on each item in
a list (`GList`/`GSList`). We can also type cast the `gpointer`s in the
list to desired type.

For example, to print all strings in a string list named `my_appname_list`,
you can do the following.

```
(gdb) gforeach appname in my_appname_list: print (gchar*) $appname
$4 = (gchar *) 0x7f16ab85dc90 "Boxes"
$5 = (gchar *) 0x7f16abb02700 "GNOME Application Platform version 45"
$6 = (gchar *) 0x7f16a80066e0 "Firefox ESR"
```

You can also call functions on each entry in the list, as below:

```
(gdb) gforeach appname in my_appname_list: call (int) strlen($appname)
$7 = 5
$8 = 37
$9 = 11
```

### 3. Backtrace decorations:

Backtraces are decorated with GLib type names and signal names.

E.g. From the below backtrace, we can say that
`gs_updates_page_button_refresh_cb ()` callback function was called
when a button (via the `GtkButton` type decoration) was clicked (via
the `clicked` signal decoration)

```
#0  gs_updates_page_button_refresh_cb (widget=0x5602f986b670 [GtkButton], self=0x5602f93aeb80 [GsUpdatesPage]) at ../src/gs-updates-page.c:826
#1  0x00007f09da7651de in <emit signal 'clicked' on instance 0x5602f986b670 [GtkButton]> (instance=0x5602f986b670, signal_id=158, detail=0) at ../gobject/gsignal.c:3675
#2  0x00007f09da747bad in g_cclosure_marshal_VOID__VOIDv (closure=0x5602f986c6a0, return_value=0x0, instance=0x5602f986b670, ...)
    at ../gobject/gmarshal.c:165
…
#20 0x00007f09da52c250 in g_application_run (application=0x5602f9068ae0 [GsApplication], argc=1, argv=0x7ffcfb3bb468) at ../gio/gapplication.c:2577
#21 0x00005602f8b53ccd in main (argc=1, argv=0x7ffcfb3bb468) at ../src/gs-main.c:49
```

Following is the same backtrace without any GLib decorations, which is
not very useful.

```
#0  gs_updates_page_button_refresh_cb (widget=0x5602f986b670, self=0x5602f93aeb80) at ../src/gs-updates-page.c:826
#1  0x00007f09da747bad in g_cclosure_marshal_VOID__VOIDv (closure=0x5602f986c6a0, return_value=0x0, instance=0x5602f986b670, ...)
    at ../gobject/gmarshal.c:165
…
#20 0x00007f09da52c250 in g_application_run (application=0x5602f9068ae0, argc=1, argv=0x7ffcfb3bb468) at ../gio/gapplication.c:2577
#21 0x00005602f8b53ccd in main (argc=1, argv=0x7ffcfb3bb468) at ../src/gs-main.c:49
```

## SystemTap

SystemTap is a dynamic whole-system analysis toolkit. GLib ships with a file
`libglib-2.0.so.*.stp` which defines a set of probe points, which you can hook
into with custom SystemTap scripts. See the files `libglib-2.0.so.*.stp`,
`libgobject-2.0.so.*.stp` and `libgio-2.0.so.*.stp` which are in your shared
SystemTap scripts directory.

## Memory statistics

`g_mem_profile()` will output a summary `g_malloc()` memory usage, if memory
profiling has been enabled by calling:

```
g_mem_set_vtable (glib_mem_profiler_table);
```

upon startup.

If GLib has been configured with full debugging support, then
`g_slice_debug_tree_statistics()` can be called in a debugger to output details
about the memory usage of the slice allocator.
