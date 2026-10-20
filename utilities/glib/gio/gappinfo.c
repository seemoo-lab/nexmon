/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#include "config.h"

#include "gappinfo.h"
#include "gappinfoprivate.h"
#include "gcontextspecificgroup.h"
#include "gdesktopappinfo.h"
#include "gtask.h"
#include "gcancellable.h"

#include "glibintl.h"
#include "gmarshal-internal.h"
#include <gioerror.h>
#include <gfile.h>

#ifdef G_OS_UNIX
#include "gdbusconnection.h"
#include "gdbusmessage.h"
#include "gportalsupport.h"
#include "gunixfdlist.h"
#include "gopenuriportal.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

/**
 * GAppInfo:
 *
 * Information about an installed application and methods to launch
 * it (with file arguments).

 * `GAppInfo` and `GAppLaunchContext` are used for describing and launching
 * applications installed on the system.
 *
 * As of GLib 2.20, URIs will always be converted to POSIX paths
 * (using [method@Gio.File.get_path]) when using [method@Gio.AppInfo.launch]
 * even if the application requested an URI and not a POSIX path. For example
 * for a desktop-file based application with the following Exec key:
 *
 * ```
 * Exec=totem %U
 * ```
 *
 * and a single URI, `sftp://foo/file.avi`, then
 * `/home/user/.gvfs/sftp on foo/file.avi` will be passed. This will only work
 * if a set of suitable GIO extensions (such as GVfs 2.26 compiled with FUSE
 * support), is available and operational; if this is not the case, the URI
 * will be passed unmodified to the application. Some URIs, such as `mailto:`,
 * of course cannot be mapped to a POSIX path (in GVfs there’s no FUSE mount
 * for it); such URIs will be passed unmodified to the application.
 *
 * Specifically for GVfs 2.26 and later, the POSIX URI will be mapped
 * back to the GIO URI in the [iface@Gio.File] constructors (since GVfs
 * implements the GVfs extension point). As such, if the application
 * needs to examine the URI, it needs to use [method@Gio.File.get_uri]
 * or similar on [iface@Gio.File]. In other words, an application cannot
 * assume that the URI passed to e.g. [func@Gio.File.new_for_commandline_arg]
 * is equal to the result of [method@Gio.File.get_uri]. The following snippet
 * illustrates this:
 *
 * ```c
 * GFile *f;
 * char *uri;
 *
 * file = g_file_new_for_commandline_arg (uri_from_commandline);
 *
 * uri = g_file_get_uri (file);
 * strcmp (uri, uri_from_commandline) == 0;
 * g_free (uri);
 *
 * if (g_file_has_uri_scheme (file, "cdda"))
 *   {
 *     // do something special with uri
 *   }
 * g_object_unref (file);
 * ```
 *
 * This code will work when both `cdda://sr0/Track 1.wav` and
 * `/home/user/.gvfs/cdda on sr0/Track 1.wav` is passed to the
 * application. It should be noted that it’s generally not safe
 * for applications to rely on the format of a particular URIs.
 * Different launcher applications (e.g. file managers) may have
 * different ideas of what a given URI means.
 */

struct _GAppLaunchContextPrivate {
  char **envp;
};

typedef GAppInfoIface GAppInfoInterface;
G_DEFINE_INTERFACE (GAppInfo, g_app_info, G_TYPE_OBJECT)

static void
g_app_info_default_init (GAppInfoInterface *iface)
{
}

/**
 * g_app_info_create_from_commandline:
 * @commandline: (type filename): the command line to use
 * @application_name: (nullable): the application name, or `NULL` to use @commandline
 * @flags: flags that can specify details of the created [iface@Gio.AppInfo]
 * @error: a [type@GLib.Error] location to store the error occurring,
 *   `NULL` to ignore.
 *
 * Creates a new [iface@Gio.AppInfo] from the given information.
 *
 * When constructing @commandline, quote any filenames or potentially-
 * untrusted input using [func@GLib.shell_quote], and note that the
 * quoting rules of the `Exec` key of the
 * [freedesktop.org Desktop Entry Specification](http://freedesktop.org/Standards/desktop-entry-spec)
 * are applied. For example, if the @commandline contains
 * percent-encoded URIs, the percent-character must be doubled in order to prevent it from
 * being swallowed by `Exec` key unquoting. See
 * [the specification](https://specifications.freedesktop.org/desktop-entry-spec/latest/ar01s07.html)
 * for exact quoting rules.
 *
 * Returns: (transfer full): new [iface@Gio.AppInfo] for given command.
 **/
GAppInfo *
g_app_info_create_from_commandline (const char           *commandline,
                                    const char           *application_name,
                                    GAppInfoCreateFlags   flags,
                                    GError              **error)
{
  g_return_val_if_fail (commandline, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  return g_app_info_create_from_commandline_impl (commandline, application_name,
                                                  flags, error);
}

/**
 * g_app_info_dup:
 * @appinfo: the app info
 * 
 * Creates a duplicate of a [iface@Gio.AppInfo].
 *
 * Returns: (transfer full): a duplicate of @appinfo.
 **/
GAppInfo *
g_app_info_dup (GAppInfo *appinfo)
{
  GAppInfoIface *iface;

  g_return_val_if_fail (G_IS_APP_INFO (appinfo), NULL);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  return (* iface->dup) (appinfo);
}

/**
 * g_app_info_equal:
 * @appinfo1: the first [iface@Gio.AppInfo].
 * @appinfo2: the second [iface@Gio.AppInfo].
 *
 * Checks if two [iface@Gio.AppInfo]s are equal.
 *
 * Note that the check *may not* compare each individual field, and only does
 * an identity check. In case detecting changes in the contents is needed,
 * program code must additionally compare relevant fields.
 *
 * Returns: `TRUE` if @appinfo1 is equal to @appinfo2. `FALSE` otherwise.
 **/
gboolean
g_app_info_equal (GAppInfo *appinfo1,
		  GAppInfo *appinfo2)
{
  GAppInfoIface *iface;

  g_return_val_if_fail (G_IS_APP_INFO (appinfo1), FALSE);
  g_return_val_if_fail (G_IS_APP_INFO (appinfo2), FALSE);

  if (G_TYPE_FROM_INSTANCE (appinfo1) != G_TYPE_FROM_INSTANCE (appinfo2))
    return FALSE;
  
  iface = G_APP_INFO_GET_IFACE (appinfo1);

  return (* iface->equal) (appinfo1, appinfo2);
}

/**
 * g_app_info_get_id:
 * @appinfo: the app info
 * 
 * Gets the ID of an application. An id is a string that identifies the
 * application. The exact format of the id is platform dependent. For instance,
 * on Unix this is the desktop file id from the xdg menu specification.
 *
 * Note that the returned ID may be `NULL`, depending on how the @appinfo has
 * been constructed.
 *
 * Returns: (nullable): a string containing the application’s ID.
 **/
const char *
g_app_info_get_id (GAppInfo *appinfo)
{
  GAppInfoIface *iface;
  
  g_return_val_if_fail (G_IS_APP_INFO (appinfo), NULL);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  return (* iface->get_id) (appinfo);
}

/**
 * g_app_info_get_name:
 * @appinfo: the app info
 * 
 * Gets the installed name of the application. 
 *
 * Returns: the name of the application for @appinfo.
 **/
const char *
g_app_info_get_name (GAppInfo *appinfo)
{
  GAppInfoIface *iface;
  
  g_return_val_if_fail (G_IS_APP_INFO (appinfo), NULL);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  return (* iface->get_name) (appinfo);
}

/**
 * g_app_info_get_display_name:
 * @appinfo: the app info
 *
 * Gets the display name of the application. The display name is often more
 * descriptive to the user than the name itself.
 *
 * Returns: the display name of the application for @appinfo, or the name if
 * no display name is available.
 *
 * Since: 2.24
 **/
const char *
g_app_info_get_display_name (GAppInfo *appinfo)
{
  GAppInfoIface *iface;

  g_return_val_if_fail (G_IS_APP_INFO (appinfo), NULL);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  if (iface->get_display_name == NULL)
    return (* iface->get_name) (appinfo);

  return (* iface->get_display_name) (appinfo);
}

/**
 * g_app_info_get_description:
 * @appinfo: the app info
 * 
 * Gets a human-readable description of an installed application.
 *
 * Returns: (nullable): a string containing a description of the 
 * application @appinfo, or `NULL` if none.
 **/
const char *
g_app_info_get_description (GAppInfo *appinfo)
{
  GAppInfoIface *iface;
  
  g_return_val_if_fail (G_IS_APP_INFO (appinfo), NULL);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  return (* iface->get_description) (appinfo);
}

/**
 * g_app_info_get_executable: (virtual get_executable)
 * @appinfo: the app info
 * 
 * Gets the executable’s name for the installed application.
 *
 * This is intended to be used for debugging or labelling what program is going
 * to be run. To launch the executable, use [method@Gio.AppInfo.launch] and related
 * functions, rather than spawning the return value from this function.
 *
 * Returns: (type filename): a string containing the @appinfo’s application
 * binaries name
 **/
const char *
g_app_info_get_executable (GAppInfo *appinfo)
{
  GAppInfoIface *iface;
  
  g_return_val_if_fail (G_IS_APP_INFO (appinfo), NULL);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  return (* iface->get_executable) (appinfo);
}


/**
 * g_app_info_get_commandline: (virtual get_commandline)
 * @appinfo: the app info
 * 
 * Gets the commandline with which the application will be
 * started.  
 *
 * Returns: (nullable) (type filename): a string containing the @appinfo’s
 *   commandline, or `NULL` if this information is not available
 *
 * Since: 2.20
 **/
const char *
g_app_info_get_commandline (GAppInfo *appinfo)
{
  GAppInfoIface *iface;
  
  g_return_val_if_fail (G_IS_APP_INFO (appinfo), NULL);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  if (iface->get_commandline)
    return (* iface->get_commandline) (appinfo);
 
  return NULL;
}

/**
 * g_app_info_set_as_default_for_type:
 * @appinfo: the app info
 * @content_type: the content type.
 * 
 * Sets the application as the default handler for a given type.
 *
 * Returns: `TRUE` on success, `FALSE` on error.
 **/
gboolean
g_app_info_set_as_default_for_type (GAppInfo    *appinfo,
				    const char  *content_type,
				    GError     **error)
{
  GAppInfoIface *iface;
  
  g_return_val_if_fail (G_IS_APP_INFO (appinfo), FALSE);
  g_return_val_if_fail (content_type != NULL, FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  if (iface->set_as_default_for_type)
    return (* iface->set_as_default_for_type) (appinfo, content_type, error);

  g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                       _("Setting default applications not supported yet"));
  return FALSE;
}

/**
 * g_app_info_set_as_last_used_for_type:
 * @appinfo: the app info
 * @content_type: the content type.
 *
 * Sets the application as the last used application for a given type. This
 * will make the application appear as first in the list returned by
 * [func@Gio.AppInfo.get_recommended_for_type], regardless of the default
 * application for that content type.
 *
 * Returns: `TRUE` on success, `FALSE` on error.
 **/
gboolean
g_app_info_set_as_last_used_for_type (GAppInfo    *appinfo,
                                      const char  *content_type,
                                      GError     **error)
{
  GAppInfoIface *iface;
  
  g_return_val_if_fail (G_IS_APP_INFO (appinfo), FALSE);
  g_return_val_if_fail (content_type != NULL, FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  if (iface->set_as_last_used_for_type)
    return (* iface->set_as_last_used_for_type) (appinfo, content_type, error);

  g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                       _("Setting application as last used for type not supported yet"));
  return FALSE;
}

/**
 * g_app_info_get_all:
 *
 * Gets a list of all of the applications currently registered
 * on this system.
 *
 * For desktop files, this includes applications that have
 * [`NoDisplay=true`](https://specifications.freedesktop.org/desktop-entry-spec/latest/ar01s06.html#key-nodisplay)
 * set or are excluded from display by means of
 * [`OnlyShowIn`](https://specifications.freedesktop.org/desktop-entry-spec/latest/ar01s06.html#key-onlyshowin)
 * or [`NotShowIn`](https://specifications.freedesktop.org/desktop-entry-spec/latest/ar01s06.html#key-notshowin).
 * See [method@Gio.AppInfo.should_show].
 *
 * The returned list does not include applications which have the
 * [`Hidden` key](https://specifications.freedesktop.org/desktop-entry-spec/latest/ar01s06.html#key-hidden)
 * set.
 *
 * Returns: (element-type GAppInfo) (transfer full): a newly allocated
 *   list of references to [iface@Gio.AppInfo]s.
 **/
GList *
g_app_info_get_all (void)
{
  return g_app_info_get_all_impl ();
}

/**
 * g_app_info_get_recommended_for_type:
 * @content_type: the content type to find a [iface@Gio.AppInfo] for
 *
 * Gets a list of recommended [iface@Gio.AppInfo]s for a given content type,
 * i.e. those applications which claim to support the given content type
 * exactly, and not by MIME type subclassing.
 *
 * Note that the first application of the list is the last used one, i.e.
 * the last one for which [method@Gio.AppInfo.set_as_last_used_for_type] has
 * been called.
 *
 * Returns: (element-type GAppInfo) (transfer full): list of
 *   [iface@Gio.AppInfo]s for given @content_type or `NULL` on error.
 *
 * Since: 2.28
 **/
GList *
g_app_info_get_recommended_for_type (const gchar *content_type)
{
  g_return_val_if_fail (content_type != NULL, NULL);

  return g_app_info_get_recommended_for_type_impl (content_type);
}

/**
 * g_app_info_get_fallback_for_type:
 * @content_type: the content type to find a [iface@Gio.AppInfo] for
 *
 * Gets a list of fallback [iface@Gio.AppInfo]s for a given content type, i.e.
 * those applications which claim to support the given content type by MIME
 * type subclassing and not directly.
 *
 * Returns: (element-type GAppInfo) (transfer full): list of [iface@Gio.AppInfo]s
 *     for given @content_type or `NULL` on error.
 *
 * Since: 2.28
 **/
GList *
g_app_info_get_fallback_for_type (const gchar *content_type)
{
  g_return_val_if_fail (content_type != NULL, NULL);

  return g_app_info_get_fallback_for_type_impl (content_type);
}

/**
 * g_app_info_get_all_for_type:
 * @content_type: the content type to find a [iface@Gio.AppInfo] for
 *
 * Gets a list of all [iface@Gio.AppInfo]s for a given content type,
 * including the recommended and fallback [iface@Gio.AppInfo]s. See
 * [func@Gio.AppInfo.get_recommended_for_type] and
 * [func@Gio.AppInfo.get_fallback_for_type].
 *
 * Returns: (element-type GAppInfo) (transfer full): list of
 *   [iface@Gio.AppInfo]s for given @content_type.
 **/
GList *
g_app_info_get_all_for_type (const char *content_type)
{
  g_return_val_if_fail (content_type != NULL, NULL);

  return g_app_info_get_all_for_type_impl (content_type);
}

/**
 * g_app_info_reset_type_associations:
 * @content_type: a content type
 *
 * Removes all changes to the type associations done by
 * [method@Gio.AppInfo.set_as_default_for_type],
 * [method@Gio.AppInfo.set_as_default_for_extension],
 * [method@Gio.AppInfo.add_supports_type] or
 * [method@Gio.AppInfo.remove_supports_type].
 *
 * Since: 2.20
 */
void
g_app_info_reset_type_associations (const char *content_type)
{
  g_app_info_reset_type_associations_impl (content_type);
}

/**
 * g_app_info_get_default_for_type:
 * @content_type: the content type to find a [iface@Gio.AppInfo] for
 * @must_support_uris: if `TRUE`, the [iface@Gio.AppInfo] is expected to
 *   support URIs
 *
 * Gets the default [iface@Gio.AppInfo] for a given content type.
 *
 * Returns: (transfer full) (nullable): [iface@Gio.AppInfo] for given
 *   @content_type or `NULL` on error.
 */
GAppInfo *
g_app_info_get_default_for_type (const char *content_type,
                                 gboolean    must_support_uris)
{
  g_return_val_if_fail (content_type != NULL, NULL);

  return g_app_info_get_default_for_type_impl (content_type, must_support_uris);
}

/**
 * g_app_info_get_default_for_uri_scheme:
 * @uri_scheme: a string containing a URI scheme.
 *
 * Gets the default application for handling URIs with the given URI scheme.
 *
 * A URI scheme is the initial part of the URI, up to but not including the `:`.
 * For example, `http`, `ftp` or `sip`.
 *
 * Returns: (transfer full) (nullable): [iface@Gio.AppInfo] for given
 *   @uri_scheme or `NULL` on error.
 */
GAppInfo *
g_app_info_get_default_for_uri_scheme (const char *uri_scheme)
{
  g_return_val_if_fail (uri_scheme != NULL && *uri_scheme != '\0', NULL);

  return g_app_info_get_default_for_uri_scheme_impl (uri_scheme);
}

static gboolean
is_valid_extension (const char *extension)
{
  return (*extension != '\0' &&
          strpbrk (extension, "/\\") == NULL);
}

/**
 * g_app_info_set_as_default_for_extension:
 * @appinfo: the app info
 * @extension: (type filename): a string containing the file extension (without
 *   the dot).
 * 
 * Sets the application as the default handler for the given file extension.
 *
 * Returns: `TRUE` on success, `FALSE` on error.
 **/
gboolean
g_app_info_set_as_default_for_extension (GAppInfo    *appinfo,
					 const char  *extension,
					 GError     **error)
{
  GAppInfoIface *iface;
  
  g_return_val_if_fail (G_IS_APP_INFO (appinfo), FALSE);
  g_return_val_if_fail (extension != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  if (!is_valid_extension (extension))
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                   _("Invalid file extension ‘%s’"), extension);
      return FALSE;
    }

  if (iface->set_as_default_for_extension)
    return (* iface->set_as_default_for_extension) (appinfo, extension, error);

  g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                       "g_app_info_set_as_default_for_extension not supported yet");
  return FALSE;
}


/**
 * g_app_info_add_supports_type:
 * @appinfo: the app info
 * @content_type: a string.
 * 
 * Adds a content type to the application information to indicate the 
 * application is capable of opening files with the given content type.
 *
 * Returns: `TRUE` on success, `FALSE` on error.
 **/
gboolean
g_app_info_add_supports_type (GAppInfo    *appinfo,
			      const char  *content_type,
			      GError     **error)
{
  GAppInfoIface *iface;
  
  g_return_val_if_fail (G_IS_APP_INFO (appinfo), FALSE);
  g_return_val_if_fail (content_type != NULL, FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  if (iface->add_supports_type)
    return (* iface->add_supports_type) (appinfo, content_type, error);

  g_set_error_literal (error, G_IO_ERROR, 
                       G_IO_ERROR_NOT_SUPPORTED, 
                       "g_app_info_add_supports_type not supported yet");

  return FALSE;
}


/**
 * g_app_info_can_remove_supports_type:
 * @appinfo: the app info
 * 
 * Checks if a supported content type can be removed from an application.
 *
 * Returns: `TRUE` if it is possible to remove supported content types from a
 *   given @appinfo, `FALSE` if not.
 **/
gboolean
g_app_info_can_remove_supports_type (GAppInfo *appinfo)
{
  GAppInfoIface *iface;
  
  g_return_val_if_fail (G_IS_APP_INFO (appinfo), FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  if (iface->can_remove_supports_type)
    return (* iface->can_remove_supports_type) (appinfo);

  return FALSE;
}


/**
 * g_app_info_remove_supports_type:
 * @appinfo: the app info
 * @content_type: a string.
 *
 * Removes a supported type from an application, if possible.
 * 
 * Returns: `TRUE` on success, `FALSE` on error.
 **/
gboolean
g_app_info_remove_supports_type (GAppInfo    *appinfo,
				 const char  *content_type,
				 GError     **error)
{
  GAppInfoIface *iface;
  
  g_return_val_if_fail (G_IS_APP_INFO (appinfo), FALSE);
  g_return_val_if_fail (content_type != NULL, FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  if (iface->remove_supports_type)
    return (* iface->remove_supports_type) (appinfo, content_type, error);

  g_set_error_literal (error, G_IO_ERROR, 
                       G_IO_ERROR_NOT_SUPPORTED, 
                       "g_app_info_remove_supports_type not supported yet");

  return FALSE;
}

/**
 * g_app_info_get_supported_types:
 * @appinfo: an app info that can handle files
 *
 * Retrieves the list of content types that @app_info claims to support.
 * If this information is not provided by the environment, this function
 * will return `NULL`.
 *
 * This function does not take in consideration associations added with
 * [method@Gio.AppInfo.add_supports_type], but only those exported directly by
 * the application.
 *
 * Returns: (transfer none) (nullable) (array zero-terminated=1) (element-type utf8):
 *   a list of content types.
 *
 * Since: 2.34
 */
const char **
g_app_info_get_supported_types (GAppInfo *appinfo)
{
  GAppInfoIface *iface;

  g_return_val_if_fail (G_IS_APP_INFO (appinfo), NULL);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  if (iface->get_supported_types)
    return iface->get_supported_types (appinfo);
  else
    return NULL;
}


/**
 * g_app_info_get_icon:
 * @appinfo: the app info
 * 
 * Gets the icon for the application.
 *
 * Returns: (nullable) (transfer none): the default [iface@Gio.Icon] for
 *   @appinfo or `NULL` if there is no default icon.
 **/
GIcon *
g_app_info_get_icon (GAppInfo *appinfo)
{
  GAppInfoIface *iface;
  
  g_return_val_if_fail (G_IS_APP_INFO (appinfo), NULL);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  return (* iface->get_icon) (appinfo);
}


/**
 * g_app_info_launch:
 * @appinfo: the app info
 * @files: (nullable) (element-type GFile): a list of [iface@Gio.File] objects
 * @context: (nullable): the launch context
 * 
 * Launches the application. Passes @files to the launched application
 * as arguments, using the optional @context to get information
 * about the details of the launcher (like what screen it is on).
 * On error, @error will be set accordingly.
 *
 * To launch the application without arguments pass a `NULL` @files list.
 *
 * Note that even if the launch is successful the application launched
 * can fail to start if it runs into problems during startup. There is
 * no way to detect this.
 *
 * Some URIs can be changed when passed through a GFile (for instance
 * unsupported URIs with strange formats like mailto:), so if you have
 * a textual URI you want to pass in as argument, consider using
 * [method@Gio.AppInfo.launch_uris] instead.
 *
 * The launched application inherits the environment of the launching
 * process, but it can be modified with [method@Gio.AppLaunchContext.setenv]
 * and [method@Gio.AppLaunchContext.unsetenv].
 *
 * On UNIX, this function sets the `GIO_LAUNCHED_DESKTOP_FILE`
 * environment variable with the path of the launched desktop file and
 * `GIO_LAUNCHED_DESKTOP_FILE_PID` to the process id of the launched
 * process. This can be used to ignore `GIO_LAUNCHED_DESKTOP_FILE`,
 * should it be inherited by further processes. The `DISPLAY`,
 * `XDG_ACTIVATION_TOKEN` and `DESKTOP_STARTUP_ID` environment
 * variables are also set, based on information provided in @context.
 *
 * Returns: `TRUE` on successful launch, `FALSE` otherwise.
 **/
gboolean
g_app_info_launch (GAppInfo           *appinfo,
		   GList              *files,
		   GAppLaunchContext  *launch_context,
		   GError            **error)
{
  GAppInfoIface *iface;
  
  g_return_val_if_fail (G_IS_APP_INFO (appinfo), FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  return (* iface->launch) (appinfo, files, launch_context, error);
}


/**
 * g_app_info_supports_uris:
 * @appinfo: the app info
 * 
 * Checks if the application supports reading files and directories from URIs.
 *
 * Returns: `TRUE` if the @appinfo supports URIs.
 **/
gboolean
g_app_info_supports_uris (GAppInfo *appinfo)
{
  GAppInfoIface *iface;
  
  g_return_val_if_fail (G_IS_APP_INFO (appinfo), FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  return (* iface->supports_uris) (appinfo);
}


/**
 * g_app_info_supports_files:
 * @appinfo: the app info
 * 
 * Checks if the application accepts files as arguments.
 *
 * Returns: `TRUE` if the @appinfo supports files.
 **/
gboolean
g_app_info_supports_files (GAppInfo *appinfo)
{
  GAppInfoIface *iface;
  
  g_return_val_if_fail (G_IS_APP_INFO (appinfo), FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  return (* iface->supports_files) (appinfo);
}


/**
 * g_app_info_launch_uris:
 * @appinfo: the app info
 * @uris: (nullable) (element-type utf8): a list of URIs to launch.
 * @context: (nullable): the launch context
 * 
 * Launches the application. This passes the @uris to the launched application
 * as arguments, using the optional @context to get information
 * about the details of the launcher (like what screen it is on).
 * On error, @error will be set accordingly. If the application only supports
 * one URI per invocation as part of their command-line, multiple instances
 * of the application will be spawned.
 *
 * To launch the application without arguments pass a `NULL` @uris list.
 *
 * Note that even if the launch is successful the application launched
 * can fail to start if it runs into problems during startup. There is
 * no way to detect this.
 *
 * Returns: `TRUE` on successful launch, `FALSE` otherwise.
 **/
gboolean
g_app_info_launch_uris (GAppInfo           *appinfo,
			GList              *uris,
			GAppLaunchContext  *launch_context,
			GError            **error)
{
  GAppInfoIface *iface;
  
  g_return_val_if_fail (G_IS_APP_INFO (appinfo), FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  return (* iface->launch_uris) (appinfo, uris, launch_context, error);
}

/**
 * g_app_info_launch_uris_async:
 * @appinfo: the app info
 * @uris: (nullable) (element-type utf8): a list of URIs to launch.
 * @context: (nullable): the launch context
 * @cancellable: (nullable): a [class@Gio.Cancellable]
 * @callback: (scope async) (nullable): a [type@Gio.AsyncReadyCallback] to call
 *   when the request is done
 * @user_data: (nullable): data to pass to @callback
 *
 * Async version of [method@Gio.AppInfo.launch_uris].
 *
 * The @callback is invoked immediately after the application launch, but it
 * waits for activation in case of D-Bus–activated applications and also provides
 * extended error information for sandboxed applications, see notes for
 * [func@Gio.AppInfo.launch_default_for_uri_async].
 *
 * Since: 2.60
 **/
void
g_app_info_launch_uris_async (GAppInfo           *appinfo,
                              GList              *uris,
                              GAppLaunchContext  *context,
                              GCancellable       *cancellable,
                              GAsyncReadyCallback callback,
                              gpointer            user_data)
{
  GAppInfoIface *iface;

  g_return_if_fail (G_IS_APP_INFO (appinfo));
  g_return_if_fail (context == NULL || G_IS_APP_LAUNCH_CONTEXT (context));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

  iface = G_APP_INFO_GET_IFACE (appinfo);
  if (iface->launch_uris_async == NULL)
    {
      GTask *task;

      task = g_task_new (appinfo, cancellable, callback, user_data);
      g_task_set_source_tag (task, g_app_info_launch_uris_async);
      g_task_return_new_error_literal (task, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                                       "Operation not supported for the current backend.");
      g_object_unref (task);

      return;
    }

  (* iface->launch_uris_async) (appinfo, uris, context, cancellable, callback, user_data);
}

/**
 * g_app_info_launch_uris_finish:
 * @appinfo: the app info
 * @result: the async result
 *
 * Finishes a [method@Gio.AppInfo.launch_uris_async] operation.
 *
 * Returns: `TRUE` on successful launch, `FALSE` otherwise.
 *
 * Since: 2.60
 */
gboolean
g_app_info_launch_uris_finish (GAppInfo     *appinfo,
                               GAsyncResult *result,
                               GError      **error)
{
  GAppInfoIface *iface;

  g_return_val_if_fail (G_IS_APP_INFO (appinfo), FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);
  if (iface->launch_uris_finish == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                           "Operation not supported for the current backend.");
      return FALSE;
    }

  return (* iface->launch_uris_finish) (appinfo, result, error);
}

/**
 * g_app_info_should_show:
 * @appinfo: the app info
 *
 * Checks if the application info should be shown in menus that 
 * list available applications.
 * 
 * Returns: `TRUE` if the @appinfo should be shown, `FALSE` otherwise.
 **/
gboolean
g_app_info_should_show (GAppInfo *appinfo)
{
  GAppInfoIface *iface;
  
  g_return_val_if_fail (G_IS_APP_INFO (appinfo), FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  return (* iface->should_show) (appinfo);
}

typedef struct {
  char *content_type;
  gboolean must_support_uris;
} DefaultForTypeData;

static void
default_for_type_data_free (DefaultForTypeData *data)
{
  g_free (data->content_type);
  g_free (data);
}

static void
get_default_for_type_thread (GTask         *task,
                             gpointer       object,
                             gpointer       task_data,
                             GCancellable  *cancellable)
{
  DefaultForTypeData *data = task_data;
  GAppInfo *info;

  info = g_app_info_get_default_for_type (data->content_type,
                                          data->must_support_uris);

  if (!info)
    {
      g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
                               _("Failed to find default application for "
                                 "content type ‘%s’"), data->content_type);
      return;
    }

  g_task_return_pointer (task, g_steal_pointer (&info), g_object_unref);
}

/**
 * g_app_info_get_default_for_type_async:
 * @content_type: the content type to find a [iface@Gio.AppInfo] for
 * @must_support_uris: if `TRUE`, the [iface@Gio.AppInfo] is expected to
 *   support URIs
 * @cancellable: (nullable): a [class@Gio.Cancellable]
 * @callback: (scope async) (nullable): a [type@Gio.AsyncReadyCallback] to call
 *   when the request is done
 * @user_data: (nullable): data to pass to @callback
 *
 * Asynchronously gets the default [iface@Gio.AppInfo] for a given content
 * type.
 *
 * Since: 2.74
 */
void
g_app_info_get_default_for_type_async  (const char          *content_type,
                                        gboolean             must_support_uris,
                                        GCancellable        *cancellable,
                                        GAsyncReadyCallback  callback,
                                        gpointer             user_data)
{
  GTask *task;
  DefaultForTypeData *data;

  g_return_if_fail (content_type != NULL && *content_type != '\0');
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

  data = g_new0 (DefaultForTypeData, 1);
  data->content_type = g_strdup (content_type);
  data->must_support_uris = must_support_uris;

  task = g_task_new (NULL, cancellable, callback, user_data);
  g_task_set_source_tag (task, g_app_info_get_default_for_type_async);
  g_task_set_task_data (task, data, (GDestroyNotify) default_for_type_data_free);
  g_task_set_check_cancellable (task, TRUE);
  g_task_run_in_thread (task, get_default_for_type_thread);
  g_object_unref (task);
}

static void
get_default_for_scheme_thread (GTask         *task,
                               gpointer       object,
                               gpointer       task_data,
                               GCancellable  *cancellable)
{
  const char *uri_scheme = task_data;
  GAppInfo *info;

  info = g_app_info_get_default_for_uri_scheme (uri_scheme);

  if (!info)
    {
      g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
                               _("Failed to find default application for "
                                 "URI Scheme ‘%s’"), uri_scheme);
      return;
    }

  g_task_return_pointer (task, g_steal_pointer (&info), g_object_unref);
}

/**
 * g_app_info_get_default_for_uri_scheme_async:
 * @uri_scheme: a string containing a URI scheme.
 * @cancellable: (nullable): a [class@Gio.Cancellable]
 * @callback: (scope async) (nullable): a [type@Gio.AsyncReadyCallback] to call
 *   when the request is done
 * @user_data: (nullable): data to pass to @callback
 *
 * Asynchronously gets the default application for handling URIs with
 * the given URI scheme. A URI scheme is the initial part
 * of the URI, up to but not including the `:`, e.g. `http`,
 * `ftp` or `sip`.
 *
 * Since: 2.74
 */
void
g_app_info_get_default_for_uri_scheme_async (const char          *uri_scheme,
                                             GCancellable        *cancellable,
                                             GAsyncReadyCallback  callback,
                                             gpointer             user_data)
{
  GTask *task;

  g_return_if_fail (uri_scheme != NULL && *uri_scheme != '\0');
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (NULL, cancellable, callback, user_data);
  g_task_set_source_tag (task, g_app_info_get_default_for_uri_scheme_async);
  g_task_set_task_data (task, g_strdup (uri_scheme), g_free);
  g_task_set_check_cancellable (task, TRUE);
  g_task_run_in_thread (task, get_default_for_scheme_thread);
  g_object_unref (task);
}

/**
 * g_app_info_get_default_for_uri_scheme_finish:
 * @result: the async result
 *
 * Finishes a default [iface@Gio.AppInfo] lookup started by
 * [func@Gio.AppInfo.get_default_for_uri_scheme_async].
 *
 * If no [iface@Gio.AppInfo] is found, then @error will be set to
 * [error@Gio.IOErrorEnum.NOT_FOUND].
 *
 * Returns: (transfer full): [iface@Gio.AppInfo] for given @uri_scheme or
 *   `NULL` on error.
 *
 * Since: 2.74
 */
GAppInfo *
g_app_info_get_default_for_uri_scheme_finish (GAsyncResult  *result,
                                              GError       **error)
{
  g_return_val_if_fail (g_task_is_valid (result, NULL), NULL);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) ==
                        g_app_info_get_default_for_uri_scheme_async, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}

/**
 * g_app_info_get_default_for_type_finish:
 * @result: the async result
 *
 * Finishes a default [iface@Gio.AppInfo] lookup started by
 * [func@Gio.AppInfo.get_default_for_type_async].
 *
 * If no #[iface@Gio.AppInfo] is found, then @error will be set to
 * [error@Gio.IOErrorEnum.NOT_FOUND].
 *
 * Returns: (transfer full): [iface@Gio.AppInfo] for given @content_type or
 *   `NULL` on error.
 *
 * Since: 2.74
 */
GAppInfo *
g_app_info_get_default_for_type_finish (GAsyncResult  *result,
                                        GError       **error)
{
  g_return_val_if_fail (g_task_is_valid (result, NULL), NULL);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) ==
                        g_app_info_get_default_for_type_async, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}

/**
 * g_app_info_launch_default_for_uri:
 * @uri: the uri to show
 * @context: (nullable): optional launch context
 *
 * Utility function that launches the default application registered to handle
 * the specified uri. Synchronous I/O is done on the uri to detect the type of
 * the file if required.
 *
 * The D-Bus–activated applications don’t have to be started if your application
 * terminates too soon after this function. To prevent this, use
 * [func@Gio.AppInfo.launch_default_for_uri_async] instead.
 *
 * Returns: `TRUE` on success, `FALSE` on error.
 **/
gboolean
g_app_info_launch_default_for_uri (const char         *uri,
                                   GAppLaunchContext  *launch_context,
                                   GError            **error)
{
  char *uri_scheme;
  GAppInfo *app_info = NULL;
  gboolean res = FALSE;

  /* g_file_query_default_handler() calls
   * g_app_info_get_default_for_uri_scheme() too, but we have to do it
   * here anyway in case GFile can't parse @uri correctly.
   */
  uri_scheme = g_uri_parse_scheme (uri);
  if (uri_scheme && uri_scheme[0] != '\0')
    app_info = g_app_info_get_default_for_uri_scheme (uri_scheme);
  g_free (uri_scheme);

  if (!app_info)
    {
      GFile *file;

      file = g_file_new_for_uri (uri);
      app_info = g_file_query_default_handler (file, NULL, error);
      g_object_unref (file);
    }

  if (app_info)
    {
      GList l;

      l.data = (char *)uri;
      l.next = l.prev = NULL;
      res = g_app_info_launch_uris (app_info, &l, launch_context, error);
      g_object_unref (app_info);
    }

#ifdef G_OS_UNIX
  if (!res && glib_should_use_portal ())
    {
      GFile *file = NULL;
      const char *parent_window = NULL;
      char *startup_id = NULL;

      /* Reset any error previously set by launch_default_for_uri */
      g_clear_error (error);

      file = g_file_new_for_uri (uri);

      if (launch_context)
        {
          GList *file_list;

          if (launch_context->priv->envp)
            parent_window = g_environ_getenv (launch_context->priv->envp, "PARENT_WINDOW_ID");

          file_list = g_list_prepend (NULL, file);

          startup_id = g_app_launch_context_get_startup_notify_id (launch_context,
                                                                   NULL,
                                                                   file_list);
          g_list_free (file_list);
        }

      res = g_openuri_portal_open_file (file, parent_window, startup_id, error);

      g_object_unref (file);
      g_free (startup_id);
    }
#endif

  return res;
}

typedef struct
{
  gchar *uri;
  GAppLaunchContext *context;
} LaunchDefaultForUriData;

static void
launch_default_for_uri_data_free (LaunchDefaultForUriData *data)
{
  g_free (data->uri);
  g_clear_object (&data->context);
  g_free (data);
}

#ifdef G_OS_UNIX
static void
launch_default_for_uri_portal_open_uri_cb (GObject      *object,
                                           GAsyncResult *result,
                                           gpointer      user_data)
{
  GTask *task = G_TASK (user_data);
  GError *error = NULL;

  if (g_openuri_portal_open_file_finish (result, &error))
    g_task_return_boolean (task, TRUE);
  else
    g_task_return_error (task, g_steal_pointer (&error));
  g_object_unref (task);
}
#endif

static void
launch_default_for_uri_portal_open_uri (GTask *task, GError *error)
{
#ifdef G_OS_UNIX
  LaunchDefaultForUriData *data = g_task_get_task_data (task);
  GCancellable *cancellable = g_task_get_cancellable (task);

  if (glib_should_use_portal ())
    {
      GFile *file;
      const char *parent_window = NULL;
      char *startup_id = NULL;

      /* Reset any error previously set by launch_default_for_uri */
      g_error_free (error);

      file = g_file_new_for_uri (data->uri);

      if (data->context)
        {
          GList *file_list;

          if (data->context->priv->envp)
            parent_window = g_environ_getenv (data->context->priv->envp,
                                              "PARENT_WINDOW_ID");

          file_list = g_list_prepend (NULL, file);

          startup_id = g_app_launch_context_get_startup_notify_id (data->context,
                                                                   NULL,
                                                                   file_list);
          g_list_free (file_list);
        }

      g_openuri_portal_open_file_async (file,
                                        parent_window,
                                        startup_id,
                                        cancellable,
                                        launch_default_for_uri_portal_open_uri_cb,
                                        g_steal_pointer (&task));
      g_object_unref (file);
      g_free (startup_id);

      return;
    }
#endif

  g_task_return_error (task, g_steal_pointer (&error));
  g_object_unref (task);
}

static void
launch_default_for_uri_launch_uris_cb (GObject      *object,
                                       GAsyncResult *result,
                                       gpointer      user_data)
{
  GAppInfo *app_info = G_APP_INFO (object);
  GTask *task = G_TASK (user_data);
  GError *error = NULL;

  if (g_app_info_launch_uris_finish (app_info, result, &error))
    {
      g_task_return_boolean (task, TRUE);
      g_object_unref (task);
    }
  else
    launch_default_for_uri_portal_open_uri (g_steal_pointer (&task), g_steal_pointer (&error));
}

static void
launch_default_for_uri_launch_uris (GTask *task,
                                    GAppInfo *app_info)
{
  GCancellable *cancellable = g_task_get_cancellable (task);
  GList l;
  LaunchDefaultForUriData *data = g_task_get_task_data (task);

  l.data = (char *)data->uri;
  l.next = l.prev = NULL;
  g_app_info_launch_uris_async (app_info,
                                &l,
                                data->context,
                                cancellable,
                                launch_default_for_uri_launch_uris_cb,
                                g_steal_pointer (&task));
  g_object_unref (app_info);
}

static void
launch_default_for_uri_default_handler_cb (GObject      *object,
                                           GAsyncResult *result,
                                           gpointer      user_data)
{
  GFile *file = G_FILE (object);
  GTask *task = G_TASK (user_data);
  GAppInfo *app_info = NULL;
  GError *error = NULL;

  app_info = g_file_query_default_handler_finish (file, result, &error);
  if (app_info)
    launch_default_for_uri_launch_uris (g_steal_pointer (&task), g_steal_pointer (&app_info));
  else
    launch_default_for_uri_portal_open_uri (g_steal_pointer (&task), g_steal_pointer (&error));
}

static void
launch_default_app_for_default_handler (GTask *task)
{
  GFile *file;
  GCancellable *cancellable;
  LaunchDefaultForUriData *data;

  data = g_task_get_task_data (task);
  cancellable = g_task_get_cancellable (task);
  file = g_file_new_for_uri (data->uri);

  g_file_query_default_handler_async (file,
                                      G_PRIORITY_DEFAULT,
                                      cancellable,
                                      launch_default_for_uri_default_handler_cb,
                                      g_steal_pointer (&task));
  g_object_unref (file);
}

static void
launch_default_app_for_uri_cb (GObject      *object,
                               GAsyncResult *result,
                               gpointer      user_data)
{
  GTask *task = G_TASK (user_data);
  GAppInfo *app_info;

  app_info = g_app_info_get_default_for_uri_scheme_finish (result, NULL);

  if (!app_info)
    {
      launch_default_app_for_default_handler (g_steal_pointer (&task));
    }
  else
    {
      launch_default_for_uri_launch_uris (g_steal_pointer (&task),
                                          g_steal_pointer (&app_info));
    }
}

/**
 * g_app_info_launch_default_for_uri_async:
 * @uri: the uri to show
 * @context: (nullable): optional launch context
 * @cancellable: (nullable): a [class@Gio.Cancellable]
 * @callback: (scope async) (nullable): a [type@Gio.AsyncReadyCallback] to call
 *   when the request is done
 * @user_data: (nullable): data to pass to @callback
 *
 * Async version of [func@Gio.AppInfo.launch_default_for_uri].
 *
 * This version is useful if you are interested in receiving error information
 * in the case where the application is sandboxed and the portal may present an
 * application chooser dialog to the user.
 *
 * This is also useful if you want to be sure that the D-Bus–activated
 * applications are really started before termination and if you are interested
 * in receiving error information from their activation.
 *
 * Since: 2.50
 */
void
g_app_info_launch_default_for_uri_async (const char          *uri,
                                         GAppLaunchContext   *context,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             user_data)
{
  GTask *task;
  char *uri_scheme;
  LaunchDefaultForUriData *data;

  g_return_if_fail (uri != NULL);

  task = g_task_new (NULL, cancellable, callback, user_data);
  g_task_set_source_tag (task, g_app_info_launch_default_for_uri_async);

  data = g_new (LaunchDefaultForUriData, 1);
  data->uri = g_strdup (uri);
  data->context = (context != NULL) ? g_object_ref (context) : NULL;
  g_task_set_task_data (task, g_steal_pointer (&data), (GDestroyNotify) launch_default_for_uri_data_free);

  /* g_file_query_default_handler_async() calls
   * g_app_info_get_default_for_uri_scheme() too, but we have to do it
   * here anyway in case GFile can't parse @uri correctly.
   */
  uri_scheme = g_uri_parse_scheme (uri);
  if (uri_scheme && uri_scheme[0] != '\0')
    {
      g_app_info_get_default_for_uri_scheme_async (uri_scheme,
                                                   cancellable,
                                                   launch_default_app_for_uri_cb,
                                                   g_steal_pointer (&task));
    }
  else
    {
      launch_default_app_for_default_handler (g_steal_pointer (&task));
    }

  g_free (uri_scheme);
}

/**
 * g_app_info_launch_default_for_uri_finish:
 * @result: the async result
 *
 * Finishes an asynchronous launch-default-for-uri operation.
 *
 * Returns: `TRUE` if the launch was successful, `FALSE` if @error is set
 *
 * Since: 2.50
 */
gboolean
g_app_info_launch_default_for_uri_finish (GAsyncResult  *result,
                                          GError       **error)
{
  g_return_val_if_fail (g_task_is_valid (result, NULL), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

/**
 * g_app_info_can_delete:
 * @appinfo: the app info
 *
 * Obtains the information whether the [iface@Gio.AppInfo] can be deleted.
 * See [method@Gio.AppInfo.delete].
 *
 * Returns: `TRUE` if @appinfo can be deleted
 *
 * Since: 2.20
 */
gboolean
g_app_info_can_delete (GAppInfo *appinfo)
{
  GAppInfoIface *iface;
  
  g_return_val_if_fail (G_IS_APP_INFO (appinfo), FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  if (iface->can_delete)
    return (* iface->can_delete) (appinfo);
 
  return FALSE; 
}


/**
 * g_app_info_delete: (virtual do_delete)
 * @appinfo: the app info
 *
 * Tries to delete a [iface@Gio.AppInfo].
 *
 * On some platforms, there may be a difference between user-defined
 * [iface@Gio.AppInfo]s which can be deleted, and system-wide ones which cannot.
 * See [method@Gio.AppInfo.can_delete].
 *
 * Returns: `TRUE` if @appinfo has been deleted
 *
 * Since: 2.20
 */
gboolean
g_app_info_delete (GAppInfo *appinfo)
{
  GAppInfoIface *iface;
  
  g_return_val_if_fail (G_IS_APP_INFO (appinfo), FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  if (iface->do_delete)
    return (* iface->do_delete) (appinfo);
 
  return FALSE; 
}


enum {
  LAUNCH_FAILED,
  LAUNCH_STARTED,
  LAUNCHED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (GAppLaunchContext, g_app_launch_context, G_TYPE_OBJECT)

/**
 * g_app_launch_context_new:
 * 
 * Creates a new application launch context. This is not normally used,
 * instead you instantiate a subclass of this, such as
 * [`GdkAppLaunchContext`](https://docs.gtk.org/gdk4/class.AppLaunchContext.html).
 *
 * Returns: a launch context.
 **/
GAppLaunchContext *
g_app_launch_context_new (void)
{
  return g_object_new (G_TYPE_APP_LAUNCH_CONTEXT, NULL);
}

static void
g_app_launch_context_finalize (GObject *object)
{
  GAppLaunchContext *context = G_APP_LAUNCH_CONTEXT (object);

  g_strfreev (context->priv->envp);

  G_OBJECT_CLASS (g_app_launch_context_parent_class)->finalize (object);
}

static void
g_app_launch_context_class_init (GAppLaunchContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = g_app_launch_context_finalize;

  /**
   * GAppLaunchContext::launch-failed:
   * @context: the object emitting the signal
   * @startup_notify_id: the startup notification id for the failed launch
   *
   * The [signal@Gio.AppLaunchContext::launch-failed] signal is emitted when a
   * [iface@Gio.AppInfo] launch fails. The startup notification id is provided,
   * so that the launcher can cancel the startup notification.
   *
   * Because a launch operation may involve spawning multiple instances of the
   * target application, you should expect this signal to be emitted multiple
   * times, one for each spawned instance.
   *
   * Since: 2.36
   */
  signals[LAUNCH_FAILED] = g_signal_new (I_("launch-failed"),
                                         G_OBJECT_CLASS_TYPE (object_class),
                                         G_SIGNAL_RUN_LAST,
                                         G_STRUCT_OFFSET (GAppLaunchContextClass, launch_failed),
                                         NULL, NULL, NULL,
                                         G_TYPE_NONE, 1, G_TYPE_STRING);

  /**
   * GAppLaunchContext::launch-started:
   * @context: the object emitting the signal
   * @info: the [iface@Gio.AppInfo] that is about to be launched
   * @platform_data: (nullable): additional platform-specific data for this launch
   *
   * The [signal@Gio.AppLaunchContext::launch-started] signal is emitted when a
   * [iface@Gio.AppInfo] is about to be launched. If non-null the
   * @platform_data is an GVariant dictionary mapping strings to variants
   * (ie `a{sv}`), which contains additional, platform-specific data about this
   * launch. On UNIX, at least the `startup-notification-id` keys will be
   * present.
   *
   * The value of the `startup-notification-id` key (type `s`) is a startup
   * notification ID corresponding to the format from the [startup-notification
   * specification](https://specifications.freedesktop.org/startup-notification-spec/startup-notification-0.1.txt).
   * It allows tracking the progress of the launchee through startup.
   *
   * It is guaranteed that this signal is followed by either a
   * [signal@Gio.AppLaunchContext::launched] or
   * [signal@Gio.AppLaunchContext::launch-failed] signal.
   *
   * Because a launch operation may involve spawning multiple instances of the
   * target application, you should expect this signal to be emitted multiple
   * times, one for each spawned instance.
   *
   * Since: 2.72
   */
  signals[LAUNCH_STARTED] = g_signal_new (I_("launch-started"),
                                          G_OBJECT_CLASS_TYPE (object_class),
                                          G_SIGNAL_RUN_LAST,
                                          G_STRUCT_OFFSET (GAppLaunchContextClass, launch_started),
                                          NULL, NULL,
                                          _g_cclosure_marshal_VOID__OBJECT_VARIANT,
                                          G_TYPE_NONE, 2,
                                          G_TYPE_APP_INFO, G_TYPE_VARIANT);
  g_signal_set_va_marshaller (signals[LAUNCH_STARTED],
                              G_TYPE_FROM_CLASS (klass),
                              _g_cclosure_marshal_VOID__OBJECT_VARIANTv);

  /**
   * GAppLaunchContext::launched:
   * @context: the object emitting the signal
   * @info: the [iface@Gio.AppInfo] that was just launched
   * @platform_data: additional platform-specific data for this launch
   *
   * The [signal@Gio.AppLaunchContext::launched] signal is emitted when a
   * [iface@Gio.AppInfo] is successfully launched.
   *
   * Because a launch operation may involve spawning multiple instances of the
   * target application, you should expect this signal to be emitted multiple
   * times, one time for each spawned instance.
   *
   * The @platform_data is an GVariant dictionary mapping
   * strings to variants (ie `a{sv}`), which contains additional,
   * platform-specific data about this launch. On UNIX, at least the
   * `pid` and `startup-notification-id` keys will be present.
   *
   * Since 2.72 the `pid` may be 0 if the process id wasn’t known (for
   * example if the process was launched via D-Bus). The `pid` may not be
   * set at all in subsequent releases.
   *
   * On Windows, `pid` is guaranteed to be valid only for the duration of the
   * [signal@Gio.AppLaunchContext::launched] signal emission; after the signal
   * is emitted, GLib will call [func@GLib.spawn_close_pid]. If you need to
   * keep the [alias@GLib.Pid] after the signal has been emitted, then you can
   * duplicate `pid` using `DuplicateHandle()`.
   *
   * Since: 2.36
   */
  signals[LAUNCHED] = g_signal_new (I_("launched"),
                                    G_OBJECT_CLASS_TYPE (object_class),
                                    G_SIGNAL_RUN_LAST,
                                    G_STRUCT_OFFSET (GAppLaunchContextClass, launched),
                                    NULL, NULL,
                                    _g_cclosure_marshal_VOID__OBJECT_VARIANT,
                                    G_TYPE_NONE, 2,
                                    G_TYPE_APP_INFO, G_TYPE_VARIANT);
  g_signal_set_va_marshaller (signals[LAUNCHED],
                              G_TYPE_FROM_CLASS (klass),
                              _g_cclosure_marshal_VOID__OBJECT_VARIANTv);
}

static void
g_app_launch_context_init (GAppLaunchContext *context)
{
  context->priv = g_app_launch_context_get_instance_private (context);
}

/**
 * g_app_launch_context_setenv:
 * @context: the launch context
 * @variable: (type filename): the environment variable to set
 * @value: (type filename): the value for to set the variable to.
 *
 * Arranges for @variable to be set to @value in the child’s environment when
 * @context is used to launch an application.
 *
 * Since: 2.32
 */
void
g_app_launch_context_setenv (GAppLaunchContext *context,
                             const char        *variable,
                             const char        *value)
{
  g_return_if_fail (G_IS_APP_LAUNCH_CONTEXT (context));
  g_return_if_fail (variable != NULL);
  g_return_if_fail (value != NULL);

  if (!context->priv->envp)
    context->priv->envp = g_get_environ ();

  context->priv->envp =
    g_environ_setenv (context->priv->envp, variable, value, TRUE);
}

/**
 * g_app_launch_context_unsetenv:
 * @context: the launch context
 * @variable: (type filename): the environment variable to remove
 *
 * Arranges for @variable to be unset in the child’s environment when @context
 * is used to launch an application.
 *
 * Since: 2.32
 */
void
g_app_launch_context_unsetenv (GAppLaunchContext *context,
                               const char        *variable)
{
  g_return_if_fail (G_IS_APP_LAUNCH_CONTEXT (context));
  g_return_if_fail (variable != NULL);

  if (!context->priv->envp)
    context->priv->envp = g_get_environ ();

  context->priv->envp =
    g_environ_unsetenv (context->priv->envp, variable);
}

/**
 * g_app_launch_context_get_environment:
 * @context: the launch context
 *
 * Gets the complete environment variable list to be passed to
 * the child process when @context is used to launch an application.
 * This is a `NULL`-terminated array of strings, where each string has
 * the form `KEY=VALUE`.
 *
 * Returns: (array zero-terminated=1) (element-type filename) (transfer full):
 *   the child’s environment
 *
 * Since: 2.32
 */
char **
g_app_launch_context_get_environment (GAppLaunchContext *context)
{
  g_return_val_if_fail (G_IS_APP_LAUNCH_CONTEXT (context), NULL);

  if (!context->priv->envp)
    context->priv->envp = g_get_environ ();

  return g_strdupv (context->priv->envp);
}

/**
 * g_app_launch_context_get_display:
 * @context: the launch context
 * @info: the app info
 * @files: (element-type GFile): a list of [iface@Gio.File] objects
 *
 * Gets the display string for the @context. This is used to ensure new
 * applications are started on the same display as the launching
 * application, by setting the `DISPLAY` environment variable.
 *
 * Returns: (nullable): a display string for the display.
 */
char *
g_app_launch_context_get_display (GAppLaunchContext *context,
				  GAppInfo          *info,
				  GList             *files)
{
  GAppLaunchContextClass *class;

  g_return_val_if_fail (G_IS_APP_LAUNCH_CONTEXT (context), NULL);
  g_return_val_if_fail (G_IS_APP_INFO (info), NULL);

  class = G_APP_LAUNCH_CONTEXT_GET_CLASS (context);

  if (class->get_display == NULL)
    return NULL;

  return class->get_display (context, info, files);
}

/**
 * g_app_launch_context_get_startup_notify_id:
 * @context: the launch context
 * @info: (nullable): the app info
 * @files: (nullable) (element-type GFile): a list of [iface@Gio.File] objects
 * 
 * Initiates startup notification for the application and returns the
 * `XDG_ACTIVATION_TOKEN` or `DESKTOP_STARTUP_ID` for the launched operation,
 * if supported.
 *
 * The returned token may be referred to equivalently as an ‘activation token’
 * (using Wayland terminology) or a ‘startup sequence ID’ (using X11 terminology).
 * The two [are interoperable](https://gitlab.freedesktop.org/wayland/wayland-protocols/-/blob/main/staging/xdg-activation/x11-interoperation.rst).
 *
 * Activation tokens are defined in the [XDG Activation Protocol](https://wayland.app/protocols/xdg-activation-v1),
 * and startup notification IDs are defined in the 
 * [freedesktop.org Startup Notification Protocol](http://standards.freedesktop.org/startup-notification-spec/startup-notification-latest.txt).
 *
 * Support for the XDG Activation Protocol was added in GLib 2.76.
 * Since GLib 2.82 @info and @files can be `NULL`. If that’s not supported by the backend,
 * the returned token will be `NULL`.
 *
 * Returns: (nullable): a startup notification ID for the application, or `NULL` if
 *   not supported.
 **/
char *
g_app_launch_context_get_startup_notify_id (GAppLaunchContext *context,
					    GAppInfo          *info,
					    GList             *files)
{
  GAppLaunchContextClass *class;

  g_return_val_if_fail (G_IS_APP_LAUNCH_CONTEXT (context), NULL);
  g_return_val_if_fail (info == NULL || G_IS_APP_INFO (info), NULL);

  class = G_APP_LAUNCH_CONTEXT_GET_CLASS (context);

  if (class->get_startup_notify_id == NULL)
    return NULL;

  return class->get_startup_notify_id (context, info, files);
}


/**
 * g_app_launch_context_launch_failed:
 * @context: the launch context
 * @startup_notify_id: the startup notification id that was returned by
 *   [method@Gio.AppLaunchContext.get_startup_notify_id].
 *
 * Called when an application has failed to launch, so that it can cancel
 * the application startup notification started in
 * [method@Gio.AppLaunchContext.get_startup_notify_id].
 * 
 **/
void
g_app_launch_context_launch_failed (GAppLaunchContext *context,
				    const char        *startup_notify_id)
{
  g_return_if_fail (G_IS_APP_LAUNCH_CONTEXT (context));
  g_return_if_fail (startup_notify_id != NULL);

  g_signal_emit (context, signals[LAUNCH_FAILED], 0, startup_notify_id);
}


/**
 * GAppInfoMonitor:
 *
 * `GAppInfoMonitor` monitors application information for changes.
 *
 * `GAppInfoMonitor` is a very simple object used for monitoring the app
 * info database for changes (newly installed or removed applications).
 *
 * Call [func@Gio.AppInfoMonitor.get] to get a `GAppInfoMonitor` and connect
 * to the [signal@Gio.AppInfoMonitor::changed] signal. The signal will be emitted once when
 * the app info database changes, and will not be emitted again until after the
 * next call to [func@Gio.AppInfo.get_all] or another `g_app_info_*()` function.
 * This is because monitoring the app info database for changes is expensive.
 *
 * The following functions will re-arm the [signal@Gio.AppInfoMonitor::changed]
 * signal so it can be emitted again:
 *
 *  - [func@Gio.AppInfo.get_all]
 *  - [func@Gio.AppInfo.get_all_for_type]
 *  - [func@Gio.AppInfo.get_default_for_type]
 *  - [func@Gio.AppInfo.get_fallback_for_type]
 *  - [func@Gio.AppInfo.get_recommended_for_type]
 *  - [`g_desktop_app_info_get_implementations()`](../gio-unix/type_func.DesktopAppInfo.get_implementation.html)
 *  - [`g_desktop_app_info_new()`](../gio-unix/ctor.DesktopAppInfo.new.html)
 *  - [`g_desktop_app_info_new_from_filename()`](../gio-unix/ctor.DesktopAppInfo.new_from_filename.html)
 *  - [`g_desktop_app_info_new_from_keyfile()`](../gio-unix/ctor.DesktopAppInfo.new_from_keyfile.html)
 *  - [`g_desktop_app_info_search()`](../gio-unix/type_func.DesktopAppInfo.search.html)
 *
 * The latter functions are available if using
 * [`GDesktopAppInfo`](../gio-unix/class.DesktopAppInfo.html) from
 * `gio-unix-2.0.pc` (GIR namespace `GioUnix-2.0`).
 *
 * In the usual case, applications should try to make note of the change
 * (doing things like invalidating caches) but not act on it. In
 * particular, applications should avoid making calls to `GAppInfo` APIs
 * in response to the change signal, deferring these until the time that
 * the updated data is actually required. The exception to this case is when
 * application information is actually being displayed on the screen
 * (for example, during a search or when the list of all applications is shown).
 * The reason for this is that changes to the list of installed applications
 * often come in groups (like during system updates) and rescanning the list
 * on every change is pointless and expensive.
 *
 * Since: 2.40
 */

typedef struct _GAppInfoMonitorClass GAppInfoMonitorClass;

struct _GAppInfoMonitor
{
  GObject parent_instance;
  GMainContext *context;
};

struct _GAppInfoMonitorClass
{
  GObjectClass parent_class;
};

static GContextSpecificGroup g_app_info_monitor_group;
static guint                 g_app_info_monitor_changed_signal;

G_DEFINE_TYPE (GAppInfoMonitor, g_app_info_monitor, G_TYPE_OBJECT)

static void
g_app_info_monitor_finalize (GObject *object)
{
  GAppInfoMonitor *monitor = G_APP_INFO_MONITOR (object);

  g_context_specific_group_remove (&g_app_info_monitor_group, monitor->context, monitor, NULL);

  G_OBJECT_CLASS (g_app_info_monitor_parent_class)->finalize (object);
}

static void
g_app_info_monitor_init (GAppInfoMonitor *monitor)
{
}

static void
g_app_info_monitor_class_init (GAppInfoMonitorClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  /**
   * GAppInfoMonitor::changed:
   *
   * Signal emitted when the app info database changes, when applications are
   * installed or removed.
   *
   * Since: 2.40
   **/
  g_app_info_monitor_changed_signal = g_signal_new (I_("changed"), G_TYPE_APP_INFO_MONITOR, G_SIGNAL_RUN_FIRST,
                                                    0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  object_class->finalize = g_app_info_monitor_finalize;
}

/**
 * g_app_info_monitor_get:
 *
 * Gets the #GAppInfoMonitor for the current thread-default main
 * context.
 *
 * The #GAppInfoMonitor will emit a “changed” signal in the
 * thread-default main context whenever the list of installed
 * applications (as reported by g_app_info_get_all()) may have changed.
 *
 * The #GAppInfoMonitor::changed signal will only be emitted once until
 * g_app_info_get_all() (or another `g_app_info_*()` function) is called. Doing
 * so will re-arm the signal ready to notify about the next change.
 *
 * You must only call g_object_unref() on the return value from under
 * the same main context as you created it.
 *
 * Returns: (transfer full): a reference to a #GAppInfoMonitor
 *
 * Since: 2.40
 **/
GAppInfoMonitor *
g_app_info_monitor_get (void)
{
  return g_context_specific_group_get (&g_app_info_monitor_group,
                                       G_TYPE_APP_INFO_MONITOR,
                                       G_STRUCT_OFFSET (GAppInfoMonitor, context),
                                       NULL);
}

void
g_app_info_monitor_fire (void)
{
  g_context_specific_group_emit (&g_app_info_monitor_group, g_app_info_monitor_changed_signal);
}
