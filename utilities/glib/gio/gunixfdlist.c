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
 * See the included COPYING file for more information.
 *
 * Authors: Ryan Lortie <desrt@desrt.ca>
 */

/**
 * GUnixFDList:
 *
 * A `GUnixFDList` contains a list of file descriptors.  It owns the file
 * descriptors that it contains, closing them when finalized.
 *
 * It may be wrapped in a
 * [`GUnixFDMessage`](../gio-unix/class.UnixFDMessage.html) and sent over a
 * [class@Gio.Socket] in the `G_SOCKET_FAMILY_UNIX` family by using
 * [method@Gio.Socket.send_message] and received using
 * [method@Gio.Socket.receive_message].
 *
 * Before 2.74, `<gio/gunixfdlist.h>` belonged to the UNIX-specific GIO
 * interfaces, thus you had to use the `gio-unix-2.0.pc` pkg-config file when
 * using it.
 *
 * Since 2.74, the API is available for Windows.
 */

#include "config.h"

#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "gunixfdlist.h"
#include "gnetworking.h"
#include "gioerror.h"
#include "glib/glib-private.h"
#include "glib/gstdio.h"

#ifdef G_OS_WIN32
#include <io.h>
#endif

struct _GUnixFDListPrivate
{
  int *fds;
  size_t nfd;
};

G_DEFINE_TYPE_WITH_PRIVATE (GUnixFDList, g_unix_fd_list, G_TYPE_OBJECT)

static void
g_unix_fd_list_init (GUnixFDList *list)
{
  list->priv = g_unix_fd_list_get_instance_private (list);
}

static void
g_unix_fd_list_finalize (GObject *object)
{
  GUnixFDList *list = G_UNIX_FD_LIST (object);

  for (size_t i = 0; i < list->priv->nfd; i++)
    g_close (list->priv->fds[i], NULL);
  g_free (list->priv->fds);

  G_OBJECT_CLASS (g_unix_fd_list_parent_class)->finalize (object);
}

static void
g_unix_fd_list_class_init (GUnixFDListClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = g_unix_fd_list_finalize;
}

static int
dup_close_on_exec_fd (int      fd,
                      GError **error)
{
  int new_fd;
#ifndef G_OS_WIN32
  int s;
#endif

#ifdef F_DUPFD_CLOEXEC
  do
    new_fd = fcntl (fd, F_DUPFD_CLOEXEC, 0l);
  while (new_fd < 0 && (errno == EINTR));

  if (new_fd >= 0)
    return new_fd;

  /* if that didn't work (new libc/old kernel?), try it the other way. */
#endif

  do
    new_fd = dup (fd);
  while (new_fd < 0 && (errno == EINTR));

  if (new_fd < 0)
    {
      int saved_errno = errno;

      g_set_error (error, G_IO_ERROR,
                   g_io_error_from_errno (saved_errno),
                   "dup: %s", g_strerror (saved_errno));

      return -1;
    }

#ifdef G_OS_WIN32
  new_fd = GLIB_PRIVATE_CALL (g_win32_reopen_noninherited) (new_fd, 0, error);
#else
  do
    {
      s = fcntl (new_fd, F_GETFD);

      if (s >= 0)
        s = fcntl (new_fd, F_SETFD, (long) (s | FD_CLOEXEC));
    }
  while (s < 0 && (errno == EINTR));

  if (s < 0)
    {
      int saved_errno = errno;

      g_set_error (error, G_IO_ERROR,
                   g_io_error_from_errno (saved_errno),
                   "fcntl: %s", g_strerror (saved_errno));
      g_close (new_fd, NULL);

      return -1;
    }
#endif

  return new_fd;
}

/**
 * g_unix_fd_list_new:
 *
 * Creates a new [class@Gio.UnixFDList] containing no file descriptors.
 *
 * Returns: a new [class@Gio.UnixFDList]
 *
 * Since: 2.24
 **/
GUnixFDList *
g_unix_fd_list_new (void)
{
  return g_object_new (G_TYPE_UNIX_FD_LIST, NULL);
}

/**
 * g_unix_fd_list_new_from_array:
 * @fds: (array length=n_fds): the initial list of file descriptors
 * @n_fds: the length of @fds, or `-1`
 *
 * Creates a new [class@Gio.UnixFDList] containing the file descriptors given
 * in @fds. The file descriptors become the property of the new list and may no
 * longer be used by the caller. The array itself is owned by the caller.
 *
 * Each file descriptor in the array should be set to close-on-exec.
 *
 * If @n_fds is -1 then @fds must be terminated with -1.
 *
 * Returns: a new [class@Gio.UnixFDList]
 *
 * Since: 2.24
 **/
GUnixFDList *
g_unix_fd_list_new_from_array (const int *fds,
                               int        n_fds)
{
  GUnixFDList *list;
  size_t n_fds_unsigned;

  g_return_val_if_fail (fds != NULL || n_fds == 0, NULL);
  g_return_val_if_fail (n_fds >= -1, NULL);

  if (n_fds >= 0)
    n_fds_unsigned = n_fds;
  else
    for (n_fds_unsigned = 0; fds[n_fds_unsigned] != -1; n_fds_unsigned++);

  g_assert (n_fds_unsigned < G_MAXSIZE);

  list = g_object_new (G_TYPE_UNIX_FD_LIST, NULL);
  list->priv->fds = g_new (int, n_fds_unsigned + 1);
  list->priv->nfd = n_fds_unsigned;

  if (n_fds_unsigned > 0)
    memcpy (list->priv->fds, fds, sizeof (int) * n_fds_unsigned);
  list->priv->fds[n_fds_unsigned] = -1;

  return list;
}

/**
 * g_unix_fd_list_steal_fds:
 * @list: a [class@Gio.UnixFDList]
 * @length: (out) (optional): pointer to the length of the returned
 *     array, or `NULL`
 *
 * Returns the array of file descriptors that is contained in this
 * object.
 *
 * After this call, the descriptors are no longer contained in
 * @list. Further calls will return an empty list (unless more
 * descriptors have been added).
 *
 * The return result of this function must be freed with `g_free()`.
 * The caller is also responsible for closing all of the file
 * descriptors.  The file descriptors in the array are set to
 * close-on-exec.
 *
 * If @length is non-`NULL` then it is set to the number of file
 * descriptors in the returned array. The returned array is also
 * terminated with `-1`.
 *
 * This function never returns `NULL`. In case there are no file
 * descriptors contained in @list, an empty array is returned.
 *
 * Returns: (array length=length) (transfer full): an array of file
 *     descriptors
 *
 * Since: 2.24
 */
int *
g_unix_fd_list_steal_fds (GUnixFDList *list,
                          int         *length)
{
  int *result;

  g_return_val_if_fail (G_IS_UNIX_FD_LIST (list), NULL);
  g_return_val_if_fail (list->priv->nfd <= G_MAXINT, NULL);

  /* will be true for fresh object or if we were just called */
  if (list->priv->fds == NULL)
    {
      list->priv->fds = g_new (int, 1);
      list->priv->fds[0] = -1;
      list->priv->nfd = 0;
    }

  if (length)
    *length = list->priv->nfd;
  result = list->priv->fds;

  list->priv->fds = NULL;
  list->priv->nfd = 0;

  return result;
}

/**
 * g_unix_fd_list_peek_fds:
 * @list: a [class@Gio.UnixFDList]
 * @length: (out) (optional): pointer to the length of the returned
 *     array, or `NULL`
 *
 * Returns the array of file descriptors that is contained in this
 * object.
 *
 * After this call, the descriptors remain the property of @list.  The
 * caller must not close them and must not free the array.  The array is
 * valid only until @list is changed in any way.
 *
 * If @length is non-`NULL` then it is set to the number of file
 * descriptors in the returned array. The returned array is also
 * terminated with `-1`.
 *
 * This function never returns `NULL`. In case there are no file
 * descriptors contained in @list, an empty array is returned.
 *
 * Returns: (array length=length) (transfer none): an array of file
 *     descriptors
 *
 * Since: 2.24
 */
const int *
g_unix_fd_list_peek_fds (GUnixFDList *list,
                         int         *length)
{
  g_return_val_if_fail (G_IS_UNIX_FD_LIST (list), NULL);
  g_return_val_if_fail (list->priv->nfd <= G_MAXINT, NULL);

  /* will be true for fresh object or if steal() was just called */
  if (list->priv->fds == NULL)
    {
      list->priv->fds = g_new (int, 1);
      list->priv->fds[0] = -1;
      list->priv->nfd = 0;
    }

  if (length)
    *length = list->priv->nfd;

  return list->priv->fds;
}

/**
 * g_unix_fd_list_append:
 * @list: a [class@Gio.UnixFDList]
 * @fd: a valid open file descriptor
 * @error: a [type@GLib.Error]
 *
 * Adds a file descriptor to @list.
 *
 * The file descriptor is duplicated using `dup()`. You keep your copy
 * of the descriptor and the copy contained in @list will be closed
 * when @list is finalized.
 *
 * A possible cause of failure is exceeding the per-process or
 * system-wide file descriptor limit.
 *
 * The index of the file descriptor in the list is returned.  If you use
 * this index with [method@Gio.UnixFDList.get] then you will receive back a
 * duplicated copy of the same file descriptor.
 *
 * Returns: the index of the appended fd in case of success, else `-1`
 *          (and @error is set)
 *
 * Since: 2.24
 */
int
g_unix_fd_list_append (GUnixFDList  *list,
                       int           fd,
                       GError      **error)
{
  int new_fd;
  size_t index_;

  g_return_val_if_fail (G_IS_UNIX_FD_LIST (list), -1);
  g_return_val_if_fail (fd >= 0, -1);
  g_return_val_if_fail (error == NULL || *error == NULL, -1);

  index_ = list->priv->nfd;
  if (index_ > G_MAXINT)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Too many file descriptors");
      return -1;
    }

  if ((new_fd = dup_close_on_exec_fd (fd, error)) < 0)
    return -1;

  /* we allocate nfd + 2 elements (fd itself and -1 terminator) */
  g_assert (list->priv->nfd <= G_MAXSIZE - 2);

  list->priv->nfd++;
  list->priv->fds = g_realloc_n (list->priv->fds,
                                 list->priv->nfd + 1,
                                 sizeof (int));
  list->priv->fds[index_] = new_fd;
  list->priv->fds[index_ + 1] = -1;

  return index_;
}

/**
 * g_unix_fd_list_append_take:
 * @list: a [class@Gio.UnixFDList]
 * @fd: a valid open file descriptor
 *
 * Adds a file descriptor to @list.
 *
 * After this call, @fd belongs to the @list and may no longer be closed by the
 * caller.
 *
 * The file descriptor @fd should be set to close-on-exec.
 *
 * The index of the file descriptor in the list is returned. If you use this
 * index with [method@Gio.UnixFDList.get] then you will receive back a
 * duplicated copy of the same file descriptor.
 *
 * Returns: the index of the appended @fd
 *
 * Since: 2.90
 */
size_t
g_unix_fd_list_append_take (GUnixFDList *list,
                            int          fd)
{
  size_t index_;

  g_return_val_if_fail (G_IS_UNIX_FD_LIST (list), 0);
  g_return_val_if_fail (fd >= 0, 0);

  index_ = list->priv->nfd;

  /* we allocate nfd + 2 elements (fd itself and -1 terminator) */
  g_assert (list->priv->nfd <= G_MAXSIZE - 2);

  list->priv->nfd++;
  list->priv->fds = g_realloc_n (list->priv->fds,
                                 list->priv->nfd + 1,
                                 sizeof (int));
  list->priv->fds[index_] = g_steal_fd (&fd);
  list->priv->fds[index_ + 1] = -1;

  return index_;
}

/**
 * g_unix_fd_list_get:
 * @list: a [method@Gio.UnixFDList.get]
 * @index_: the index into the list
 * @error: a [type@GLib.Error]
 *
 * Gets a file descriptor out of @list.
 *
 * @index_ specifies the index of the file descriptor to get.  It is a
 * programmer error for @index_ to be out of range. Either use
 * [method@Gio.UnixFDList.lookup] to do a checked lookup, or check the index
 * against the list length using [method@Gio.UnixFDList.get_length].
 *
 * The file descriptor is duplicated using `dup()` and set as
 * close-on-exec before being returned.  You must call `close()` on it
 * when you are done.
 *
 * A possible cause of failure is exceeding the per-process or
 * system-wide file descriptor limit.
 *
 * Returns: the file descriptor, or `-1` in case of error
 *
 * Since: 2.24
 **/
int
g_unix_fd_list_get (GUnixFDList  *list,
                    int           index_,
                    GError      **error)
{
  g_return_val_if_fail (G_IS_UNIX_FD_LIST (list), -1);
  g_return_val_if_fail (index_ >= 0 && (size_t) index_ < list->priv->nfd, -1);
  g_return_val_if_fail (error == NULL || *error == NULL, -1);

  return dup_close_on_exec_fd (list->priv->fds[index_], error);
}

/**
 * g_unix_fd_list_peek:
 * @list: a [class@Gio.UnixFDList]
 * @index_: the index into the list
 *
 * Gets a file descriptor out of @list.
 *
 * @index_ specifies the index of the file descriptor to get. It is a programmer
 * error for @index_ to be out of range; see [method@Gio.UnixFDList.get_length].
 *
 * This will always return a valid (non-negative) file descriptor.
 *
 * After this call, the descriptor remains the property of @list. The caller
 * must not close it. The descriptor is valid only until @list is changed in any
 * way.
 *
 * Returns: the file descriptor
 *
 * Since: 2.90
 **/
int
g_unix_fd_list_peek (GUnixFDList *list,
                     size_t       index_)
{
  g_return_val_if_fail (G_IS_UNIX_FD_LIST (list), -1);
  g_return_val_if_fail (index_ < list->priv->nfd, -1);

  return list->priv->fds[index_];
}

/**
 * g_unix_fd_list_lookup:
 * @list: a [class@Gio.UnixFDList]
 * @index_: the file descriptor index
 *
 * Looks up a file descriptor in @list at position @index_.
 *
 * @index_ specifies the index of the file descriptor to get. If no file
 * descriptor exists at this index, `-1` is returned.
 *
 * After this call, the descriptor remains the property of @list. The caller
 * must not close it. The descriptor is valid only until @list is changed in any
 * way.
 *
 * Returns: the file descriptor, or `-1` if not found
 *
 * Since: 2.90
 **/
int
g_unix_fd_list_lookup (GUnixFDList *list,
                       size_t       index_)
{
  g_return_val_if_fail (G_IS_UNIX_FD_LIST (list), -1);

  if (index_ >= list->priv->nfd)
    return -1;

  return list->priv->fds[index_];
}

/**
 * g_unix_fd_list_get_length:
 * @list: a [class@Gio.UnixFDList]
 *
 * Gets the length of @list (ie: the number of file descriptors
 * contained within).
 *
 * Returns: the length of @list
 *
 * Since: 2.24
 **/
int
g_unix_fd_list_get_length (GUnixFDList *list)
{
  g_return_val_if_fail (G_IS_UNIX_FD_LIST (list), 0);
  g_return_val_if_fail (list->priv->nfd <= G_MAXINT, 0);

  return list->priv->nfd;
}
