/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2013 Canonical Limited
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
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#ifndef __G_APP_INFO_PRIVATE_H__
#define __G_APP_INFO_PRIVATE_H__

void g_app_info_monitor_fire (void);

GAppInfo *g_app_info_create_from_commandline_impl (const char           *commandline,
                                                   const char           *application_name,
                                                   GAppInfoCreateFlags   flags,
                                                   GError              **error);
GList *g_app_info_get_recommended_for_type_impl (const gchar *content_type);
GList *g_app_info_get_fallback_for_type_impl (const gchar *content_type);
GList *g_app_info_get_all_for_type_impl (const char *content_type);
void g_app_info_reset_type_associations_impl (const char *content_type);
GAppInfo *g_app_info_get_default_for_type_impl (const char *content_type,
                                                gboolean    must_support_uris);
GAppInfo *g_app_info_get_default_for_uri_scheme_impl (const char *uri_scheme);
GList *g_app_info_get_all_impl (void);

#endif /* __G_APP_INFO_PRIVATE_H__ */
