/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2014 Patrick Griffis
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
 */

#include "config.h"

#include "gappinfo.h"
#include "gappinfoprivate.h"
#include "gosxappinfo.h"
#include "gcontenttype.h"
#include "gfile.h"
#include "gfileicon.h"
#include "gioerror.h"
#include "gtask.h"

#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#import <ApplicationServices/ApplicationServices.h>

/**
 * GOsxAppInfo:
 *
 * `GOsxAppInfo` is an implementation of [iface@Gio.AppInfo] based on `NSBundle`
 * information.
 *
 * Note that `<gio/gosxappinfo.h>` is unique to OSX.
 */

static void        g_osx_app_info_iface_init (GAppInfoIface *iface);
static const char *g_osx_app_info_get_id     (GAppInfo      *appinfo);

struct _GOsxAppInfo
{
  GObject parent_instance;

  NSBundle *bundle;

  /* Note that these are all NULL until first call
   * to getter at which point they are cached here
   */
  gchar *id;
  gchar *name;
  gchar *executable;
  gchar *filename;
  GIcon *icon;
};

G_DEFINE_TYPE_WITH_CODE (GOsxAppInfo, g_osx_app_info, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (G_TYPE_APP_INFO, g_osx_app_info_iface_init))

static GOsxAppInfo *
g_osx_app_info_new (NSBundle *bundle)
{
  GOsxAppInfo *info = g_object_new (G_TYPE_OSX_APP_INFO, NULL);

  info->bundle = [bundle retain];

  return info;
}

static void
g_osx_app_info_init (GOsxAppInfo *info)
{
}

static void
g_osx_app_info_finalize (GObject *object)
{
  GOsxAppInfo *info = G_OSX_APP_INFO (object);

  g_free (info->id);
  g_free (info->name);
  g_free (info->executable);
  g_free (info->filename);
  g_clear_object (&info->icon);

  [info->bundle release];

  G_OBJECT_CLASS (g_osx_app_info_parent_class)->finalize (object);
}

static void
g_osx_app_info_class_init (GOsxAppInfoClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = g_osx_app_info_finalize;
}

static GAppInfo *
g_osx_app_info_dup (GAppInfo *appinfo)
{
  GOsxAppInfo *info;
  GOsxAppInfo *new_info;

  g_return_val_if_fail (appinfo != NULL, NULL);

  info = G_OSX_APP_INFO (appinfo);
  new_info = g_osx_app_info_new ([info->bundle retain]);

  return G_APP_INFO (new_info);
}

static gboolean
g_osx_app_info_equal (GAppInfo *appinfo1,
                      GAppInfo *appinfo2)
{
  const gchar *str1, *str2;

  g_return_val_if_fail (appinfo1 != NULL, FALSE);
  g_return_val_if_fail (appinfo2 != NULL, FALSE);

  str1 = g_osx_app_info_get_id (appinfo1);
  str2 = g_osx_app_info_get_id (appinfo2);

  return (g_strcmp0 (str1, str2) == 0);
}

/*< internal >
 * get_bundle_string_value:
 * @bundle: a #NSBundle
 * @key: an #NSString key
 *
 * Returns a value from a bundles info.plist file.
 * It will be utf8 encoded and it must be g_free()'d.
 *
 */
static gchar *
get_bundle_string_value (NSBundle *bundle,
                         NSString *key)
{
  NSString *value;
  const gchar *cvalue;
  gchar *ret;

  g_return_val_if_fail (bundle != NULL, NULL);

  value = (NSString *)[bundle objectForInfoDictionaryKey: key];
  if (!value)
    return NULL;

  cvalue = [value cStringUsingEncoding: NSUTF8StringEncoding];
  ret = g_strdup (cvalue);

  return ret;
}

static CFStringRef
create_cfstring_from_cstr (const gchar *cstr)
{
  g_return_val_if_fail (cstr != NULL, NULL);
  return CFStringCreateWithCString (NULL, cstr, kCFStringEncodingUTF8);
}

#ifdef G_ENABLE_DEBUG
static gchar *
create_cstr_from_cfstring (CFStringRef str)
{
  g_return_val_if_fail (str != NULL, NULL);

  CFIndex length = CFStringGetLength (str);
  CFIndex maxlen = CFStringGetMaximumSizeForEncoding (length, kCFStringEncodingUTF8);
  gchar *buffer = g_malloc (maxlen + 1);
  Boolean success = CFStringGetCString (str, (char *) buffer, maxlen,
                                        kCFStringEncodingUTF8);
  if (success)
    return buffer;
  else
    {
      g_free (buffer);
      return NULL;
    }
}
#endif

static char *
url_escape_hostname (const char *url)
{
  char *host_start, *ret;

  host_start = strstr (url, "://");
  if (host_start != NULL)
    {
      char *host_end, *scheme, *host, *hostname;

      scheme = g_strndup (url, host_start - url);
      host_start += 3;
      host_end = strchr (host_start, '/');

      if (host_end != NULL)
        host = g_strndup (host_start, host_end - host_start);
      else
        host = g_strdup (host_start);

      hostname = g_hostname_to_ascii (host);

      ret = g_strconcat (scheme, "://", hostname, host_end, NULL);

      g_free (scheme);
      g_free (host);
      g_free (hostname);

      return ret;
    }

  return g_strdup (url);
}

static CFURLRef
create_url_from_cstr_or_file (gpointer data,
                              gboolean is_file)
{
  const char *cstr;
  char *cstr_owned = NULL;
  gchar *puny_cstr;
  CFStringRef str;
  CFURLRef url;

  if (is_file)
    cstr = cstr_owned = g_file_get_uri ((GFile *) data);
  else
    cstr = (char *) data;

  puny_cstr = url_escape_hostname (cstr);
  str = CFStringCreateWithCString (NULL, puny_cstr ? puny_cstr : cstr, kCFStringEncodingUTF8);
  url = CFURLCreateWithString (NULL, str, NULL);

  if (!url)
    g_debug ("Creating CFURL from %s %s failed!", cstr, is_file ? "file" : "uri");

  g_free (cstr_owned);
  g_free (puny_cstr);
  CFRelease(str);
  return url;
}

static CFArrayRef
create_url_list_from_glist (GList    *uris,
                            gboolean  are_files)
{
  GList *lst;
  int len = g_list_length (uris);
  CFMutableArrayRef array;

  if (!len)
    return NULL;

  array = CFArrayCreateMutable (NULL, len, &kCFTypeArrayCallBacks);
  if (!array)
    return NULL;

  for (lst = uris; lst != NULL && lst->data; lst = lst->next)
    {
      /* lst->data is either a GFile* or a char* URI, depending on are_files */
      CFURLRef url = create_url_from_cstr_or_file (lst->data, are_files);
      if (url)
        CFArrayAppendValue (array, url);
    }

  return (CFArrayRef)array;
}

static void
fill_urlspec_for_appinfo (LSLaunchURLSpec *urlspec,
                          GOsxAppInfo     *info,
                          GList           *uris,
                          gboolean         are_files)
{
  urlspec->appURL = (CFURLRef) [NSURL fileURLWithPath:[info->bundle bundlePath]];
  urlspec->itemURLs = create_url_list_from_glist (uris, are_files);
  urlspec->launchFlags = kLSLaunchDefaults;
}

static void
clear_urlspec (LSLaunchURLSpec *urlspec)
{
  if (urlspec->itemURLs)
    {
      CFArrayRemoveAllValues ((CFMutableArrayRef) urlspec->itemURLs);
      CFRelease (urlspec->itemURLs);
    }
}

static NSBundle *
get_bundle_for_url (CFURLRef app_url)
{
  NSBundle *bundle = [NSBundle bundleWithURL: (NSURL*)app_url];

  if (!bundle)
    {
      g_debug ("Bundle not found for url.");
      return NULL;
    }

  return bundle;
}

static NSBundle *
get_bundle_for_id (CFStringRef bundle_id)
{
  CFURLRef app_url;
  NSBundle *bundle;

  CFArrayRef urls = LSCopyApplicationURLsForBundleIdentifier (bundle_id, NULL);
  if (urls)
    {
      /* TODO: if there's multiple, we should perhaps prefer one that's in $HOME,
       * instead of just always picking the first.
       */
      app_url = CFArrayGetValueAtIndex (urls, 0);
      CFRetain (app_url);
      CFRelease (urls);
    }
  else
    {
#ifdef G_ENABLE_DEBUG /* This can fail often, no reason to alloc strings */
      gchar *id_str = create_cstr_from_cfstring (bundle_id);
      if (id_str)
        {
          g_debug ("Application not found for id \"%s\".", id_str);
          g_free (id_str);
        }
      else
        g_debug ("Application not found for unconvertible bundle id.");
#endif
      return NULL;
    }

  bundle = get_bundle_for_url (app_url);
  CFRelease (app_url);
  return bundle;
}

static const char *
g_osx_app_info_get_id (GAppInfo *appinfo)
{
  GOsxAppInfo *info = G_OSX_APP_INFO (appinfo);

  if (!info->id)
    info->id = get_bundle_string_value (info->bundle, @"CFBundleIdentifier");

  return info->id;
}

static const char *
g_osx_app_info_get_name (GAppInfo *appinfo)
{
  GOsxAppInfo *info = G_OSX_APP_INFO (appinfo);

  if (!info->name)
    info->name = get_bundle_string_value (info->bundle, @"CFBundleName");

  return info->name;
}

static const char *
g_osx_app_info_get_display_name (GAppInfo *appinfo)
{
  return g_osx_app_info_get_name (appinfo);
}

static const char *
g_osx_app_info_get_description (GAppInfo *appinfo)
{
  /* Bundles do not contain descriptions */
  return NULL;
}

static const char *
g_osx_app_info_get_executable (GAppInfo *appinfo)
{
  GOsxAppInfo *info = G_OSX_APP_INFO (appinfo);

  if (!info->executable)
    info->executable = get_bundle_string_value (info->bundle, @"CFBundleExecutable");

  return info->executable;
}

const char *
g_osx_app_info_get_filename (GOsxAppInfo *info)
{
  g_return_val_if_fail (info != NULL, NULL);

  if (!info->filename)
    {
      info->filename = g_strconcat ("file://", [[info->bundle bundlePath]
                                    cStringUsingEncoding: NSUTF8StringEncoding],
                                    NULL);
    }

  return info->filename;
}

static const char *
g_osx_app_info_get_commandline (GAppInfo *appinfo)
{
  /* There isn't really a command line value */
  return NULL;
}

static GIcon *
g_osx_app_info_get_icon (GAppInfo *appinfo)
{
  GOsxAppInfo *info = G_OSX_APP_INFO (appinfo);

  if (!info->icon)
    {
      const gchar *app_uri;
      gchar *icon_name, *icon_uri;
      GFile *file;

      icon_name = get_bundle_string_value (info->bundle, @"CFBundleIconFile");
      if (!icon_name)
        return NULL;

      app_uri = g_osx_app_info_get_filename (info);
      icon_uri = g_strconcat (app_uri + strlen ("file://"), "/Contents/Resources/", icon_name,
                              g_str_has_suffix (icon_name, ".icns") ? NULL : ".icns", NULL);
      g_free (icon_name);

      file = g_file_new_for_path (icon_uri);
      info->icon = g_file_icon_new (file);
      g_object_unref (file);
      g_free (icon_uri);
    }

  return info->icon;
}

static gboolean
g_osx_app_info_launch_internal (GAppInfo  *appinfo,
                                GList     *uris,
                                gboolean   are_files,
                                GError   **error)
{
  GOsxAppInfo *info = G_OSX_APP_INFO (appinfo);
  LSLaunchURLSpec urlspec = { 0 };
  gint ret, success = TRUE;
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  g_return_val_if_fail (G_IS_OSX_APP_INFO (appinfo), FALSE);

  fill_urlspec_for_appinfo (&urlspec, info, uris, are_files);

  if ((ret = LSOpenFromURLSpec (&urlspec, NULL)))
    {
      /* TODO: Better error codes */
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Opening application failed with code %d", ret);
      success = FALSE;
    }

  clear_urlspec (&urlspec);
  [pool drain];
  return success;
}

static gboolean
g_osx_app_info_supports_uris (GAppInfo *appinfo)
{
  return TRUE;
}

static gboolean
g_osx_app_info_supports_files (GAppInfo *appinfo)
{
  return TRUE;
}

static gboolean
g_osx_app_info_launch (GAppInfo           *appinfo,
                       GList              *files,
                       GAppLaunchContext  *launch_context,
                       GError            **error)
{
  return g_osx_app_info_launch_internal (appinfo, files, TRUE, error);
}

static gboolean
g_osx_app_info_launch_uris (GAppInfo           *appinfo,
                            GList              *uris,
                            GAppLaunchContext  *launch_context,
                            GError            **error)
{
  return g_osx_app_info_launch_internal (appinfo, uris, FALSE, error);
}

typedef struct
{
  GList *uris;  /* (element-type utf8) (owned) (nullable) */
  GAppLaunchContext *context;  /* (owned) (nullable) */
} LaunchUrisData;

static void
launch_uris_data_free (LaunchUrisData *data)
{
  g_clear_object (&data->context);
  g_list_free_full (data->uris, g_free);
  g_free (data);
}

static void
launch_uris_async_thread (GTask         *task,
                          gpointer       source_object,
                          gpointer       task_data,
                          GCancellable  *cancellable)
{
  GAppInfo *appinfo = G_APP_INFO (source_object);
  LaunchUrisData *data = task_data;
  GError *local_error = NULL;
  gboolean succeeded;

  succeeded = g_osx_app_info_launch_internal (appinfo, data->uris, FALSE, &local_error);

  if (local_error != NULL)
    g_task_return_error (task, g_steal_pointer (&local_error));
  else
    g_task_return_boolean (task, succeeded);
}

static void
g_osx_app_info_launch_uris_async (GAppInfo           *appinfo,
                                  GList              *uris,
                                  GAppLaunchContext  *context,
                                  GCancellable       *cancellable,
                                  GAsyncReadyCallback callback,
                                  gpointer            user_data)
{
  GTask *task;
  LaunchUrisData *data;

  task = g_task_new (appinfo, cancellable, callback, user_data);
  g_task_set_source_tag (task, g_osx_app_info_launch_uris_async);

  data = g_new0 (LaunchUrisData, 1);
  data->uris = g_list_copy_deep (uris, (GCopyFunc) g_strdup, NULL);
  g_set_object (&data->context, context);
  g_task_set_task_data (task, g_steal_pointer (&data), (GDestroyNotify) launch_uris_data_free);

  g_task_run_in_thread (task, launch_uris_async_thread);
  g_object_unref (task);
}

static gboolean
g_osx_app_info_launch_uris_finish (GAppInfo     *appinfo,
                                   GAsyncResult *result,
                                   GError      **error)
{
  g_return_val_if_fail (g_task_is_valid (result, appinfo), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

static gboolean
g_osx_app_info_should_show (GAppInfo *appinfo)
{
  /* Bundles don't have hidden attribute */
  return TRUE;
}

static gboolean
g_osx_app_info_set_as_default_for_type (GAppInfo    *appinfo,
                                        const char  *content_type,
                                        GError     **error)
{
  return FALSE;
}

static const char **
g_osx_app_info_get_supported_types (GAppInfo *appinfo)
{
  /* TODO: get CFBundleDocumentTypes */
  return NULL;
}

static gboolean
g_osx_app_info_set_as_last_used_for_type (GAppInfo   *appinfo,
                                          const char  *content_type,
                                          GError     **error)
{
  /* Not supported. */
  return FALSE;
}

static gboolean
g_osx_app_info_can_delete (GAppInfo *appinfo)
{
  return FALSE;
}

static void
g_osx_app_info_iface_init (GAppInfoIface *iface)
{
  iface->dup = g_osx_app_info_dup;
  iface->equal = g_osx_app_info_equal;

  iface->get_id = g_osx_app_info_get_id;
  iface->get_name = g_osx_app_info_get_name;
  iface->get_display_name = g_osx_app_info_get_display_name;
  iface->get_description = g_osx_app_info_get_description;
  iface->get_executable = g_osx_app_info_get_executable;
  iface->get_commandline = g_osx_app_info_get_commandline;
  iface->get_icon = g_osx_app_info_get_icon;
  iface->get_supported_types = g_osx_app_info_get_supported_types;

  iface->set_as_last_used_for_type = g_osx_app_info_set_as_last_used_for_type;
  iface->set_as_default_for_type = g_osx_app_info_set_as_default_for_type;

  iface->launch = g_osx_app_info_launch;
  iface->launch_uris = g_osx_app_info_launch_uris;
  iface->launch_uris_async = g_osx_app_info_launch_uris_async;
  iface->launch_uris_finish = g_osx_app_info_launch_uris_finish;
  iface->supports_uris = g_osx_app_info_supports_uris;
  iface->supports_files = g_osx_app_info_supports_files;
  iface->should_show = g_osx_app_info_should_show;
  iface->can_delete = g_osx_app_info_can_delete;
}

GAppInfo *
g_app_info_create_from_commandline_impl (const char           *commandline,
                                         const char           *application_name,
                                         GAppInfoCreateFlags   flags,
                                         GError              **error)
{
  g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                       "Creating an app info from a command line not currently supported");

  return NULL;
}

GList *
g_osx_app_info_get_all_for_scheme (const char *cscheme)
{
  CFArrayRef bundle_list;
  CFStringRef scheme;
  NSBundle *bundle;
  GList *info_list = NULL;
  gint i;
  
  scheme = create_cfstring_from_cstr (cscheme);
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  /* first deprecated in macOS 10.15 - Use LSCopyApplicationURLsForURL() instead */
  bundle_list = LSCopyAllHandlersForURLScheme (scheme);
  G_GNUC_END_IGNORE_DEPRECATIONS
  CFRelease (scheme);

  if (!bundle_list)
    return NULL;

  for (i = 0; i < CFArrayGetCount (bundle_list); i++)
    {
      CFStringRef bundle_id = CFArrayGetValueAtIndex (bundle_list, i);
      GAppInfo *info;

      bundle = get_bundle_for_id (bundle_id);

      if (!bundle)
        continue;

      info = G_APP_INFO (g_osx_app_info_new (bundle));
      info_list = g_list_append (info_list, info);
    }
  CFRelease (bundle_list);
  return info_list;
}

GList *
g_app_info_get_all_for_type_impl (const char *content_type)
{
  gchar *mime_type;
  CFArrayRef bundle_list;
  CFStringRef type;
  NSBundle *bundle;
  GList *info_list = NULL;
  gint i;

  mime_type = g_content_type_get_mime_type (content_type);
  if (g_str_has_prefix (mime_type, "x-scheme-handler/"))
    {
      gchar *scheme = strchr (mime_type, '/') + 1;
      GList *ret = g_osx_app_info_get_all_for_scheme (scheme);

      g_free (mime_type);
      return ret;
    }
  g_free (mime_type);

  type = create_cfstring_from_cstr (content_type);
  bundle_list = LSCopyAllRoleHandlersForContentType (type, kLSRolesAll);
  CFRelease (type);

  if (!bundle_list)
    return NULL;

  for (i = 0; i < CFArrayGetCount (bundle_list); i++)
    {
      CFStringRef bundle_id = CFArrayGetValueAtIndex (bundle_list, i);
      GAppInfo *info;

      bundle = get_bundle_for_id (bundle_id);

      if (!bundle)
        continue;

      info = G_APP_INFO (g_osx_app_info_new (bundle));
      info_list = g_list_append (info_list, info);
    }
  CFRelease (bundle_list);
  return info_list;
}

GList *
g_app_info_get_recommended_for_type_impl (const char *content_type)
{
  return g_app_info_get_all_for_type (content_type);
}

GList *
g_app_info_get_fallback_for_type_impl (const char *content_type)
{
  return g_app_info_get_all_for_type (content_type);
}

GAppInfo *
g_app_info_get_default_for_type_impl (const char *content_type,
                                      gboolean    must_support_uris)
{
  gchar *mime_type;
  CFStringRef type;
  NSBundle *bundle;
  CFURLRef bundle_id;

  mime_type = g_content_type_get_mime_type (content_type);
  if (g_str_has_prefix (mime_type, "x-scheme-handler/"))
    {
      gchar *scheme = strchr (mime_type, '/') + 1;
      GAppInfo *ret = g_app_info_get_default_for_uri_scheme (scheme);

      g_free (mime_type);
      return ret;
    }
  g_free (mime_type);

  type = create_cfstring_from_cstr (content_type);

  bundle_id = LSCopyDefaultApplicationURLForContentType (type, kLSRolesAll, NULL);
  CFRelease (type);

  if (!bundle_id)
    {
      g_warning ("No default handler found for content type '%s'.", content_type);
      return NULL;
    }

  bundle = get_bundle_for_url (bundle_id);
  CFRelease (bundle_id);

  if (!bundle)
    return NULL;

  return G_APP_INFO (g_osx_app_info_new (bundle));
}

GAppInfo *
g_app_info_get_default_for_uri_scheme_impl (const char *uri_scheme)
{
  CFStringRef scheme, bundle_id;
  NSBundle *bundle;

  scheme = create_cfstring_from_cstr (uri_scheme);
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  /* first deprecated in macOS 10.15 - Use LSCopyDefaultApplicationURLForURL() instead. */
  bundle_id = LSCopyDefaultHandlerForURLScheme (scheme);
  G_GNUC_END_IGNORE_DEPRECATIONS
  CFRelease (scheme);

  if (!bundle_id)
    {
      g_info ("No default handler found for url scheme '%s'.", uri_scheme);
      return NULL;
    }

  bundle = get_bundle_for_id (bundle_id);
  CFRelease (bundle_id);

  if (!bundle)
    return NULL;

  return G_APP_INFO (g_osx_app_info_new (bundle));
}

GList *
g_app_info_get_all_impl (void)
{
  /* There is no API for this afaict
   * could manually do it...
   */
  return NULL;
}

void
g_app_info_reset_type_associations_impl (const char *content_type)
{
}
