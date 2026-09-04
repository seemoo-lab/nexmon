#include "config.h"

/* 
 * This is the only way to disable deprecation warnings for macros, and we need
 * to continue using G_MODULE_SUFFIX in the implementation of
 * g_module_build_path() which is also deprecated API.
 */
#ifndef GLIB_DISABLE_DEPRECATION_WARNINGS
#define GLIB_DISABLE_DEPRECATION_WARNINGS
#endif

#include <glib.h>

#include <stdbool.h>

#if (G_MODULE_IMPL == G_MODULE_IMPL_AR) || (G_MODULE_IMPL == G_MODULE_IMPL_DL)
G_GNUC_INTERNAL gchar*    _g_module_build_path (const gchar *directory,
                                                const gchar *module_name);

#ifndef __CYGWIN__

gchar*
_g_module_build_path (const gchar *directory,
		      const gchar *module_name)
{
  if (directory && *directory) {
    if (strncmp (module_name, "lib", 3) == 0)
      return g_strconcat (directory, "/", module_name, NULL);
    else
      return g_strconcat (directory, "/lib", module_name, "." G_MODULE_SUFFIX, NULL);
  } else if (strncmp (module_name, "lib", 3) == 0)
    return g_strdup (module_name);
  else
    return g_strconcat ("lib", module_name, "." G_MODULE_SUFFIX, NULL);
}

#else /* __CYGWIN__ */

#ifdef __MSYS__
#define CYG_MODULE_PREFIX "msys-"
#else
#define CYG_MODULE_PREFIX "cyg"
#endif

gchar*
_g_module_build_path (const gchar *directory,
                      const gchar *module_name)
{
  bool module_name_has_prefix = g_str_has_prefix (module_name, CYG_MODULE_PREFIX) ||
                                g_str_has_prefix (module_name, "lib");

  if (directory && *directory)
    return module_name_has_prefix ?
           g_strconcat (directory, "/", module_name, NULL) :
           g_strconcat (directory, "/" CYG_MODULE_PREFIX, module_name, "." G_MODULE_SUFFIX, NULL);
  else
    return module_name_has_prefix ?
           g_strdup (module_name) :
           g_strconcat (CYG_MODULE_PREFIX, module_name, "." G_MODULE_SUFFIX, NULL);
}

#endif  /* __CYGWIN__ */

#endif
