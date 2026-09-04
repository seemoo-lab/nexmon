Title: Commandline Option Parser

# Commandline Option Parser

The GOption commandline parser is intended to be a simpler replacement
for the popt library. It supports short and long commandline options,
as shown in the following example:

    testtreemodel -r 1 --max-size 20 --rand --display=:1.0 -vb -- file1 file2

The example demonstrates a number of features of the GOption commandline parser:

 - Options can be single letters, prefixed by a single dash.
 - Multiple short options can be grouped behind a single dash.
 - Long options are prefixed by two consecutive dashes.
 - Options can have an extra argument, which can be a number, a string or
   a filename. For long options, the extra argument can be appended with
   an equals sign after the option name, which is useful if the extra
   argument starts with a dash, which would otherwise cause it to be
   interpreted as another option.
 - Non-option arguments are returned to the application as rest arguments.
 - An argument consisting solely of two dashes turns off further parsing,
   any remaining arguments (even those starting with a dash) are returned
   to the application as rest arguments.

Another important feature of GOption is that it can automatically
generate nicely formatted help output. Unless it is explicitly turned
off with [method@GLib.OptionContext.set_help_enabled], GOption will recognize
the `--help`, `-?`, `--help-all` and `--help-groupname` options (where `groupname`
is the name of a [struct@GLib.OptionGroup]) and write a text similar to the one shown
in the following example to stdout.

    Usage:
      testtreemodel [OPTION...] - test tree model performance

    Help Options:
      -h, --help               Show help options
      --help-all               Show all help options
      --help-gtk               Show GTK Options

    Application Options:
      -r, --repeats=N          Average over N repetitions
      -m, --max-size=M         Test up to 2^M items
      --display=DISPLAY        X display to use
      -v, --verbose            Be verbose
     -b, --beep               Beep when done
     --rand                   Randomize the data

GOption groups options in [struct@GLib.OptionGroup]s, which makes it easy to
incorporate options from multiple sources. The intended use for this is
to let applications collect option groups from the libraries it uses,
add them to their #GOptionContext, and parse all options by a single call
to [method@GLib.OptionContext.parse].

If an option is declared to be of type string or filename, GOption takes
care of converting it to the right encoding; strings are returned in UTF-8,
filenames are returned in the GLib filename encoding. Note that this only
works if `setlocale()` has been called before [method@GLib.OptionContext.parse]

Here is a complete example of setting up GOption to parse the example
commandline above and produce the example help output.

```c
static gint repeats = 2;
static gint max_size = 8;
static gboolean verbose = FALSE;
static gboolean beep = FALSE;
static gboolean randomize = FALSE;

static GOptionEntry entries[] =
{
  { "repeats", 'r', 0, G_OPTION_ARG_INT, &repeats, "Average over N repetitions", "N" },
  { "max-size", 'm', 0, G_OPTION_ARG_INT, &max_size, "Test up to 2^M items", "M" },
  { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "Be verbose", NULL },
  { "beep", 'b', 0, G_OPTION_ARG_NONE, &beep, "Beep when done", NULL },
  { "rand", 0, 0, G_OPTION_ARG_NONE, &randomize, "Randomize the data", NULL },
  G_OPTION_ENTRY_NULL
};

int
main (int argc, char *argv[])
{
  GError *error = NULL;
  GOptionContext *context;

  context = g_option_context_new ("- test tree model performance");
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_print ("option parsing failed: %s\n", error->message);
      exit (1);
    }

  ...

}
```

On UNIX systems, the argv that is passed to `main()` has no particular
encoding, even to the extent that different parts of it may have different
encodings. In general, normal arguments and flags will be in the current
locale and filenames should be considered to be opaque byte strings.
Proper use of `G_OPTION_ARG_FILENAME` vs `G_OPTION_ARG_STRING` is
therefore important.

Note that on Windows, filenames do have an encoding, but using
[struct@GLib.OptionContext] with the argv as passed to `main()` will result
in a program that can only accept commandline arguments with characters
from the system codepage.This can cause problems when attempting to
deal with filenames containing Unicode characters that fall outside
of the codepage.

A solution to this is to use `g_win32_get_command_line()` and
[method@GLib.OptionContext.parse_strv] which will properly handle full
Unicode filenames. If you are using #GApplication, this is done
automatically for you.

The following example shows how you can use #GOptionContext directly
in order to correctly deal with Unicode filenames on Windows:

```c
int
main (int argc, char **argv)
{
  GError *error = NULL;
  GOptionContext *context;
  gchar **args;

#ifdef G_OS_WIN32
  args = g_win32_get_command_line ();
#else
  args = g_strdupv (argv);
#endif

  // set up context

  if (!g_option_context_parse_strv (context, &args, &error))
    {
      // error happened
    }

  ...

  g_strfreev (args);

  ...
}
```
