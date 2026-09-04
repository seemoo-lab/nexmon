/* GIO - GLib Input, Output and Streaming Library
 * 
 * Copyright (C) 2006-2007 Red Hat, Inc.
 * Copyright (C) 2022 Canonical Ltd.
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
 * Author: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "config.h"
#include <errno.h>
#include "gioerror.h"

#ifdef G_OS_WIN32
#include <winsock2.h>
#endif

/**
 * g_io_error_quark:
 *
 * Gets the GIO Error Quark.
 *
 * Returns: a #GQuark.
 **/
G_DEFINE_QUARK (g-io-error-quark, g_io_error)

/**
 * g_io_error_from_errno:
 * @err_no: Error number as defined in errno.h.
 *
 * Converts `errno.h` error codes into GIO error codes.
 *
 * The fallback value %G_IO_ERROR_FAILED is returned for error codes not
 * currently handled (but note that future GLib releases may return a more
 * specific value instead).
 *
 * As `errno` is global and may be modified by intermediate function
 * calls, you should save its value immediately after the call returns,
 * and use the saved value instead of `errno`:
 * 
 *
 * |[<!-- language="C" -->
 *   int saved_errno;
 *
 *   ret = read (blah);
 *   saved_errno = errno;
 *
 *   g_io_error_from_errno (saved_errno);
 * ]|
 *
 * Returns: #GIOErrorEnum value for the given `errno.h` error number
 */
GIOErrorEnum
g_io_error_from_errno (gint err_no)
{
  GFileError file_error;
  GIOErrorEnum io_error;

  file_error = g_file_error_from_errno (err_no);
  io_error = g_io_error_from_file_error (file_error);

  if (io_error != G_IO_ERROR_FAILED)
    return io_error;

  switch (err_no)
    {
#ifdef EMLINK
    case EMLINK:
      return G_IO_ERROR_TOO_MANY_LINKS;
      break;
#endif

#ifdef ENOMSG
    case ENOMSG:
      return G_IO_ERROR_INVALID_DATA;
      break;
#endif

#ifdef ENODATA
    case ENODATA:
      return G_IO_ERROR_INVALID_DATA;
      break;
#endif

#ifdef EBADMSG
    case EBADMSG:
      return G_IO_ERROR_INVALID_DATA;
      break;
#endif

#ifdef ECANCELED
    case ECANCELED:
      return G_IO_ERROR_CANCELLED;
      break;
#endif

    /* ENOTEMPTY == EEXIST on AIX for backward compatibility reasons */
#if defined (ENOTEMPTY) && (!defined (EEXIST) || (ENOTEMPTY != EEXIST))
    case ENOTEMPTY:
      return G_IO_ERROR_NOT_EMPTY;
      break;
#endif

#ifdef ENOTSUP
    case ENOTSUP:
      return G_IO_ERROR_NOT_SUPPORTED;
      break;
#endif

    /* EOPNOTSUPP == ENOTSUP on Linux, but POSIX considers them distinct */
#if defined (EOPNOTSUPP) && (!defined (ENOTSUP) || (EOPNOTSUPP != ENOTSUP))
    case EOPNOTSUPP:
      return G_IO_ERROR_NOT_SUPPORTED;
      break;
#endif

#ifdef EPROTONOSUPPORT
    case EPROTONOSUPPORT:
      return G_IO_ERROR_NOT_SUPPORTED;
      break;
#endif

#ifdef ESOCKTNOSUPPORT
    case ESOCKTNOSUPPORT:
      return G_IO_ERROR_NOT_SUPPORTED;
      break;
#endif

#ifdef EPFNOSUPPORT
    case EPFNOSUPPORT:
      return G_IO_ERROR_NOT_SUPPORTED;
      break;
#endif

#ifdef EAFNOSUPPORT
    case EAFNOSUPPORT:
      return G_IO_ERROR_NOT_SUPPORTED;
      break;
#endif

#ifdef ETIMEDOUT
    case ETIMEDOUT:
      return G_IO_ERROR_TIMED_OUT;
      break;
#endif

#ifdef EBUSY
    case EBUSY:
      return G_IO_ERROR_BUSY;
      break;
#endif

#ifdef EWOULDBLOCK
    case EWOULDBLOCK:
      return G_IO_ERROR_WOULD_BLOCK;
      break;
#endif

    /* EWOULDBLOCK == EAGAIN on most systems, but POSIX considers them distinct */
#if defined (EAGAIN) && (!defined (EWOULDBLOCK) || (EWOULDBLOCK != EAGAIN))
    case EAGAIN:
      return G_IO_ERROR_WOULD_BLOCK;
      break;
#endif

#ifdef EADDRINUSE
    case EADDRINUSE:
      return G_IO_ERROR_ADDRESS_IN_USE;
      break;
#endif

#ifdef EHOSTUNREACH
    case EHOSTUNREACH:
      return G_IO_ERROR_HOST_UNREACHABLE;
      break;
#endif

#ifdef ENETUNREACH
    case ENETUNREACH:
      return G_IO_ERROR_NETWORK_UNREACHABLE;
      break;
#endif

#ifdef ENETDOWN
    case ENETDOWN:
      return G_IO_ERROR_NETWORK_UNREACHABLE;
      break;
#endif

#ifdef ECONNREFUSED
    case ECONNREFUSED:
      return G_IO_ERROR_CONNECTION_REFUSED;
      break;
#endif

#ifdef EADDRNOTAVAIL
    case EADDRNOTAVAIL:
      return G_IO_ERROR_CONNECTION_REFUSED;
      break;
#endif

#ifdef ECONNRESET
    case ECONNRESET:
      return G_IO_ERROR_CONNECTION_CLOSED;
      break;
#endif

#ifdef ENOTCONN
    case ENOTCONN:
      return G_IO_ERROR_NOT_CONNECTED;
      break;
#endif

#ifdef EDESTADDRREQ
    case EDESTADDRREQ:
      return G_IO_ERROR_DESTINATION_UNSET;
      break;
#endif

#ifdef EMSGSIZE
    case EMSGSIZE:
      return G_IO_ERROR_MESSAGE_TOO_LARGE;
      break;
#endif

#ifdef ENOTSOCK
    case ENOTSOCK:
      return G_IO_ERROR_INVALID_ARGUMENT;
      break;
#endif

    default:
      return G_IO_ERROR_FAILED;
      break;
    }
}

/**
 * g_io_error_from_file_error:
 * @file_error: a #GFileError.
 *
 * Converts #GFileError error codes into GIO error codes.
 *
 * Returns: #GIOErrorEnum value for the given #GFileError error value.
 *
 * Since: 2.74
 **/
GIOErrorEnum
g_io_error_from_file_error (GFileError file_error)
{
  switch (file_error)
  {
    case G_FILE_ERROR_EXIST:
      return G_IO_ERROR_EXISTS;
    case G_FILE_ERROR_ISDIR:
      return G_IO_ERROR_IS_DIRECTORY;
    case G_FILE_ERROR_ACCES:
      return G_IO_ERROR_PERMISSION_DENIED;
    case G_FILE_ERROR_NAMETOOLONG:
      return G_IO_ERROR_FILENAME_TOO_LONG;
    case G_FILE_ERROR_NOENT:
      return G_IO_ERROR_NOT_FOUND;
    case G_FILE_ERROR_NOTDIR:
      return G_IO_ERROR_NOT_DIRECTORY;
    case G_FILE_ERROR_NXIO:
      return G_IO_ERROR_NOT_REGULAR_FILE;
    case G_FILE_ERROR_NODEV:
      return G_IO_ERROR_NO_SUCH_DEVICE;
    case G_FILE_ERROR_ROFS:
      return G_IO_ERROR_READ_ONLY;
    case G_FILE_ERROR_TXTBSY:
      return G_IO_ERROR_BUSY;
    case G_FILE_ERROR_LOOP:
      return G_IO_ERROR_TOO_MANY_LINKS;
    case G_FILE_ERROR_NOSPC:
    case G_FILE_ERROR_NOMEM:
      return G_IO_ERROR_NO_SPACE;
    case G_FILE_ERROR_MFILE:
    case G_FILE_ERROR_NFILE:
      return G_IO_ERROR_TOO_MANY_OPEN_FILES;
    case G_FILE_ERROR_INVAL:
      return G_IO_ERROR_INVALID_ARGUMENT;
    case G_FILE_ERROR_PIPE:
      return G_IO_ERROR_BROKEN_PIPE;
    case G_FILE_ERROR_AGAIN:
      return G_IO_ERROR_WOULD_BLOCK;
    case G_FILE_ERROR_PERM:
      return G_IO_ERROR_PERMISSION_DENIED;
    case G_FILE_ERROR_NOSYS:
      return G_IO_ERROR_NOT_SUPPORTED;
    case G_FILE_ERROR_BADF:
    case G_FILE_ERROR_FAILED:
    case G_FILE_ERROR_FAULT:
    case G_FILE_ERROR_INTR:
    case G_FILE_ERROR_IO:
      return G_IO_ERROR_FAILED;
    default:
      g_return_val_if_reached (G_IO_ERROR_FAILED);
  }
}

#ifdef G_OS_WIN32

/**
 * g_io_error_from_win32_error:
 * @error_code: Windows error number.
 *
 * Converts some common error codes (as returned from GetLastError()
 * or WSAGetLastError()) into GIO error codes. The fallback value
 * %G_IO_ERROR_FAILED is returned for error codes not currently
 * handled (but note that future GLib releases may return a more
 * specific value instead).
 *
 * You can use g_win32_error_message() to get a localized string
 * corresponding to @error_code. (But note that unlike g_strerror(),
 * g_win32_error_message() returns a string that must be freed.)
 *
 * Returns: #GIOErrorEnum value for the given error number.
 *
 * Since: 2.26
 **/
GIOErrorEnum
g_io_error_from_win32_error (gint error_code)
{
  /* Note: Winsock errors are a subset of Win32 error codes as a
   * whole. (The fact that the Winsock API makes them look like they
   * aren't is just because the API predates Win32.)
   */

  switch (error_code)
    {
    case WSAEADDRINUSE:
      return G_IO_ERROR_ADDRESS_IN_USE;

    case WSAEWOULDBLOCK:
      return G_IO_ERROR_WOULD_BLOCK;

    case WSAEACCES:
      return G_IO_ERROR_PERMISSION_DENIED;

    case WSA_INVALID_HANDLE:
    case WSA_INVALID_PARAMETER:
    case WSAEINVAL:
    case WSAEBADF:
    case WSAENOTSOCK:
      return G_IO_ERROR_INVALID_ARGUMENT;

    case WSAEPROTONOSUPPORT:
      return G_IO_ERROR_NOT_SUPPORTED;

    case WSAECANCELLED:
      return G_IO_ERROR_CANCELLED;

    case WSAESOCKTNOSUPPORT:
    case WSAEOPNOTSUPP:
    case WSAEPFNOSUPPORT:
    case WSAEAFNOSUPPORT:
      return G_IO_ERROR_NOT_SUPPORTED;

    case WSAECONNRESET:
    case WSAENETRESET:
    case WSAESHUTDOWN:
      return G_IO_ERROR_CONNECTION_CLOSED;

    case WSAEHOSTUNREACH:
      return G_IO_ERROR_HOST_UNREACHABLE;

    case WSAENETUNREACH:
      return G_IO_ERROR_NETWORK_UNREACHABLE;

    case WSAECONNREFUSED:
      return G_IO_ERROR_CONNECTION_REFUSED;

    case WSAETIMEDOUT:
      return G_IO_ERROR_TIMED_OUT;

    case WSAENOTCONN:
    case ERROR_PIPE_LISTENING:
      return G_IO_ERROR_NOT_CONNECTED;

    case WSAEMSGSIZE:
      return G_IO_ERROR_MESSAGE_TOO_LARGE;

    default:
      return G_IO_ERROR_FAILED;
    }
}

#endif
