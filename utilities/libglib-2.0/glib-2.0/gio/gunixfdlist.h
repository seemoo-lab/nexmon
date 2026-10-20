/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2009 Codethink Limited
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
 * Authors: Ryan Lortie <desrt@desrt.ca>
 */

#ifndef __G_UNIX_FD_LIST_H__
#define __G_UNIX_FD_LIST_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define G_TYPE_UNIX_FD_LIST                                 (g_unix_fd_list_get_type ())
#define G_UNIX_FD_LIST(inst)                                (G_TYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             G_TYPE_UNIX_FD_LIST, GUnixFDList))
#define G_UNIX_FD_LIST_CLASS(class)                         (G_TYPE_CHECK_CLASS_CAST ((class),                       \
                                                             G_TYPE_UNIX_FD_LIST, GUnixFDListClass))
#define G_IS_UNIX_FD_LIST(inst)                             (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             G_TYPE_UNIX_FD_LIST))
#define G_IS_UNIX_FD_LIST_CLASS(class)                      (G_TYPE_CHECK_CLASS_TYPE ((class),                       \
                                                             G_TYPE_UNIX_FD_LIST))
#define G_UNIX_FD_LIST_GET_CLASS(inst)                      (G_TYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                             G_TYPE_UNIX_FD_LIST, GUnixFDListClass))
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GUnixFDList, g_object_unref)

typedef struct _GUnixFDListPrivate                       GUnixFDListPrivate;
typedef struct _GUnixFDListClass                         GUnixFDListClass;

struct _GUnixFDListClass
{
  GObjectClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
};

struct _GUnixFDList
{
  GObject parent_instance;
  GUnixFDListPrivate *priv;
};

GIO_AVAILABLE_IN_ALL
GType                   g_unix_fd_list_get_type                         (void);
GIO_AVAILABLE_IN_ALL
GUnixFDList *           g_unix_fd_list_new                              (void);
GIO_AVAILABLE_IN_ALL
GUnixFDList *           g_unix_fd_list_new_from_array                   (const int    *fds,
                                                                         int           n_fds);

GIO_AVAILABLE_IN_ALL
int                     g_unix_fd_list_append                           (GUnixFDList  *list,
                                                                         int           fd,
                                                                         GError      **error);

GIO_AVAILABLE_IN_2_90
size_t                  g_unix_fd_list_append_take                      (GUnixFDList  *list,
                                                                         int           fd);

GIO_AVAILABLE_IN_ALL
int                     g_unix_fd_list_get_length                       (GUnixFDList  *list);

GIO_AVAILABLE_IN_ALL
int                     g_unix_fd_list_get                              (GUnixFDList  *list,
                                                                         int           index_,
                                                                         GError      **error);

GIO_AVAILABLE_IN_2_90
int                     g_unix_fd_list_peek                             (GUnixFDList  *list,
                                                                         size_t        index_);

GIO_AVAILABLE_IN_2_90
int                     g_unix_fd_list_lookup                           (GUnixFDList  *list,
                                                                         size_t        index_);

GIO_AVAILABLE_IN_ALL
const int *             g_unix_fd_list_peek_fds                         (GUnixFDList  *list,
                                                                         int          *length);

GIO_AVAILABLE_IN_ALL
int *                   g_unix_fd_list_steal_fds                        (GUnixFDList *list,
                                                                         int         *length);

G_END_DECLS

#endif /* __G_UNIX_FD_LIST_H__ */
