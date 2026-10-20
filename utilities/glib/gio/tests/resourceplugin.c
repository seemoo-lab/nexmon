/*
 * Ensure the g_io_module_*() symbols are exported
 * on all supported compilers without using config.h.
 * This must be done before including any GLib headers,
 * since GLIB_AVAILABLE_IN_ALL, which is used to mark the
 * g_io_module*() symbols, is defined to be _GLIB_EXTERN,
 * which must be overridden to export the symbols.
 */

#include <gio/gio.h>

void
g_io_module_load (GIOModule *module)
{
}

void
g_io_module_unload (GIOModule   *module)
{
}

char **
g_io_module_query (void)
{
  return NULL;
}
