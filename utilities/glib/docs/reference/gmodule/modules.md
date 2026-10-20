Title: Dynamic Loading of Modules

## Dynamic Loading of Modules

These functions provide a portable way to dynamically load object files
(commonly known as 'plug-ins'). The current implementation supports all
systems that provide an implementation of `dlopen()` (e.g. Linux/Sun), as
well as Windows platforms via DLLs.

Do not call these functions from within global constructors (for example, those
using GCC’s `__attribute__((constructor))` attribute) as this may lead to deadlock.

A program which wants to use these functions must be linked to the libraries
output by the command:

    pkg-config --libs gmodule-2.0

To use them you must first determine whether dynamic loading is supported on
the platform by calling [`func@GModule.Module.supported`]. If it is, you can
open a module with [`func@GModule.Module.open`], find the module's symbols
(e.g. function names) with [`method@GModule.Module.symbol`], and later close
the module with [`method@GModule.Module.close`].
[`method@GModule.Module.name`] will return the file name of a currently
opened module.

If any of the above functions fail, the error status can be found with
[`func@GModule.Module.error`].

The `GModule` implementation features reference counting for opened modules,
and supports hook functions within a module which are called when the module
is loaded and unloaded (see [callback@GModule.ModuleCheckInit] and
[callback@GModule.ModuleUnload]).

If your module introduces static data to common subsystems in the running
program, e.g. through calling API like:

```c
static GQuark my_module_quark =
  g_quark_from_static_string ("my-module-stuff");
```

it must ensure that it is never unloaded, by calling
[`method@GModule.Module.make_resident`].

### Calling a function defined in a GModule

```c
// the function signature for 'say_hello'
typedef void (* SayHelloFunc) (const char *message);

gboolean
just_say_hello (const char *filename, GError **error)
{
  SayHelloFunc say_hello;
  GModule *module;

  module = g_module_open (filename, G_MODULE_BIND_LAZY);
  if (module == NULL)
    {
      g_set_error (error, FOO_ERROR, FOO_ERROR_BLAH,
                   "%s", g_module_error ());
      return FALSE;
    }

  if (!g_module_symbol (module, "say_hello", (gpointer *)&say_hello))
    {
      g_set_error (error, SAY_ERROR, SAY_ERROR_OPEN,
                   "%s: %s", filename, g_module_error ());

      if (!g_module_close (module))
        g_warning ("%s: %s", filename, g_module_error ());

      return FALSE;
    }

  if (say_hello == NULL)
    {
      g_set_error (error, SAY_ERROR, SAY_ERROR_OPEN,
                   "symbol say_hello is NULL");

      if (!g_module_close (module))
        g_warning ("%s: %s", filename, g_module_error ());

      return FALSE;
    }

  // call our function in the module
  say_hello ("Hello world!");

  if (!g_module_close (module))
    g_warning ("%s: %s", filename, g_module_error ());

  return TRUE;
 }
```
