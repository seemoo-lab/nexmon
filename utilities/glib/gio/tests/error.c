/* Unit tests for gioerror
 * GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2022 Marco Trevisan
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
 * Author: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "config.h"
#include <errno.h>

#include <gio/gio.h>

#ifdef G_OS_WIN32
#include <winsock2.h>
#endif

/* We are testing some deprecated APIs here */
#ifndef GLIB_DISABLE_DEPRECATION_WARNINGS
#define GLIB_DISABLE_DEPRECATION_WARNINGS
#endif

static void
test_error_from_errno (void)
{
  g_assert_cmpint (g_io_error_from_errno (-1), ==, G_IO_ERROR_FAILED);

#ifdef EEXIST
  g_assert_cmpint (g_io_error_from_errno (EEXIST), ==,
                   G_IO_ERROR_EXISTS);
#endif

#ifdef EISDIR
  g_assert_cmpint (g_io_error_from_errno (EISDIR), ==,
                   G_IO_ERROR_IS_DIRECTORY);
#endif

#ifdef EACCES
  g_assert_cmpint (g_io_error_from_errno (EACCES), ==,
                   G_IO_ERROR_PERMISSION_DENIED);
#endif

#ifdef ENAMETOOLONG
  g_assert_cmpint (g_io_error_from_errno (ENAMETOOLONG), ==,
                   G_IO_ERROR_FILENAME_TOO_LONG);
#endif

#ifdef ENOENT
  g_assert_cmpint (g_io_error_from_errno (ENOENT), ==,
                   G_IO_ERROR_NOT_FOUND);
#endif

#ifdef ENOTDIR
  g_assert_cmpint (g_io_error_from_errno (ENOTDIR), ==,
                   G_IO_ERROR_NOT_DIRECTORY);
#endif

#ifdef ENXIO
  g_assert_cmpint (g_io_error_from_errno (ENXIO), ==,
                   G_IO_ERROR_NOT_REGULAR_FILE);
#endif

#ifdef EROFS
  g_assert_cmpint (g_io_error_from_errno (EROFS), ==,
                   G_IO_ERROR_READ_ONLY);
#endif

#ifdef ELOOP
  g_assert_cmpint (g_io_error_from_errno (ELOOP), ==,
                   G_IO_ERROR_TOO_MANY_LINKS);
#endif

#ifdef EMLINK
  g_assert_cmpint (g_io_error_from_errno (EMLINK), ==,
                   G_IO_ERROR_TOO_MANY_LINKS);
#endif

#ifdef ENOSPC
  g_assert_cmpint (g_io_error_from_errno (ENOSPC), ==,
                   G_IO_ERROR_NO_SPACE);
#endif

#ifdef ENOMEM
  g_assert_cmpint (g_io_error_from_errno (ENOMEM), ==,
                   G_IO_ERROR_NO_SPACE);
#endif

#ifdef EINVAL
  g_assert_cmpint (g_io_error_from_errno (EINVAL), ==,
                   G_IO_ERROR_INVALID_ARGUMENT);
#endif

#ifdef EPERM
  g_assert_cmpint (g_io_error_from_errno (EPERM), ==,
                   G_IO_ERROR_PERMISSION_DENIED);
#endif

#ifdef ECANCELED
  g_assert_cmpint (g_io_error_from_errno (ECANCELED), ==,
                   G_IO_ERROR_CANCELLED);
#endif

#ifdef ENOTEMPTY
  g_assert_cmpint (g_io_error_from_errno (ENOTEMPTY), ==,
                   G_IO_ERROR_NOT_EMPTY);
#endif

#ifdef ENOTSUP
  g_assert_cmpint (g_io_error_from_errno (ENOTSUP), ==,
                   G_IO_ERROR_NOT_SUPPORTED);
#endif

#ifdef EOPNOTSUPP
  g_assert_cmpint (g_io_error_from_errno (EOPNOTSUPP), ==,
                   G_IO_ERROR_NOT_SUPPORTED);
#endif

#ifdef EPROTONOSUPPORT
  g_assert_cmpint (g_io_error_from_errno (EPROTONOSUPPORT), ==,
                   G_IO_ERROR_NOT_SUPPORTED);
#endif

#ifdef ESOCKTNOSUPPORT
  g_assert_cmpint (g_io_error_from_errno (ESOCKTNOSUPPORT), ==,
                   G_IO_ERROR_NOT_SUPPORTED);
#endif

#ifdef EPFNOSUPPORT
  g_assert_cmpint (g_io_error_from_errno (EPFNOSUPPORT), ==,
                   G_IO_ERROR_NOT_SUPPORTED);
#endif

#ifdef EAFNOSUPPORT
  g_assert_cmpint (g_io_error_from_errno (EAFNOSUPPORT), ==,
                   G_IO_ERROR_NOT_SUPPORTED);
#endif

#ifdef ETIMEDOUT
  g_assert_cmpint (g_io_error_from_errno (ETIMEDOUT), ==,
                   G_IO_ERROR_TIMED_OUT);
#endif

#ifdef EBUSY
  g_assert_cmpint (g_io_error_from_errno (EBUSY), ==,
                   G_IO_ERROR_BUSY);
#endif

#ifdef EWOULDBLOCK
  g_assert_cmpint (g_io_error_from_errno (EWOULDBLOCK), ==,
                   G_IO_ERROR_WOULD_BLOCK);
#endif

#ifdef EAGAIN
  g_assert_cmpint (g_io_error_from_errno (EAGAIN), ==,
                   G_IO_ERROR_WOULD_BLOCK);
#endif

#ifdef EMFILE
  g_assert_cmpint (g_io_error_from_errno (EMFILE), ==,
                   G_IO_ERROR_TOO_MANY_OPEN_FILES);
#endif

#ifdef EADDRINUSE
  g_assert_cmpint (g_io_error_from_errno (EADDRINUSE), ==,
                   G_IO_ERROR_ADDRESS_IN_USE);
#endif

#ifdef EHOSTUNREACH
  g_assert_cmpint (g_io_error_from_errno (EHOSTUNREACH), ==,
                   G_IO_ERROR_HOST_UNREACHABLE);
#endif

#ifdef ENETUNREACH
  g_assert_cmpint (g_io_error_from_errno (ENETUNREACH), ==,
                   G_IO_ERROR_NETWORK_UNREACHABLE);
#endif

#ifdef ECONNREFUSED
  g_assert_cmpint (g_io_error_from_errno (ECONNREFUSED), ==,
                   G_IO_ERROR_CONNECTION_REFUSED);
#endif

#ifdef EPIPE
  g_assert_cmpint (g_io_error_from_errno (EPIPE), ==,
                   G_IO_ERROR_BROKEN_PIPE);
#endif

#ifdef ECONNRESET
  g_assert_cmpint (g_io_error_from_errno (ECONNRESET), ==,
                   G_IO_ERROR_CONNECTION_CLOSED);
#endif

#ifdef ENOTCONN
  g_assert_cmpint (g_io_error_from_errno (ENOTCONN), ==,
                   G_IO_ERROR_NOT_CONNECTED);
#endif

#ifdef EMSGSIZE
  g_assert_cmpint (g_io_error_from_errno (EMSGSIZE), ==,
                   G_IO_ERROR_MESSAGE_TOO_LARGE);
#endif

#ifdef ENOTSOCK
  g_assert_cmpint (g_io_error_from_errno (ENOTSOCK), ==,
                   G_IO_ERROR_INVALID_ARGUMENT);
#endif

#ifdef ESRCH
  g_assert_cmpint (g_io_error_from_errno (ESRCH), ==,
                   G_IO_ERROR_FAILED);
#endif

#ifdef EINTR
  g_assert_cmpint (g_io_error_from_errno (EINTR), ==,
                   G_IO_ERROR_FAILED);
#endif

#ifdef EIO
  g_assert_cmpint (g_io_error_from_errno (EIO), ==,
                   G_IO_ERROR_FAILED);
#endif

#ifdef E2BIG
  g_assert_cmpint (g_io_error_from_errno (E2BIG), ==,
                   G_IO_ERROR_FAILED);
#endif

#ifdef ENOEXEC
  g_assert_cmpint (g_io_error_from_errno (ENOEXEC), ==,
                   G_IO_ERROR_FAILED);
#endif

#ifdef EBADF
  g_assert_cmpint (g_io_error_from_errno (EBADF), ==,
                   G_IO_ERROR_FAILED);
#endif

#ifdef ECHILD
  g_assert_cmpint (g_io_error_from_errno (ECHILD), ==,
                   G_IO_ERROR_FAILED);
#endif

#ifdef EFAULT
  g_assert_cmpint (g_io_error_from_errno (EFAULT), ==,
                   G_IO_ERROR_FAILED);
#endif

#ifdef ENOTBLK
  g_assert_cmpint (g_io_error_from_errno (ENOTBLK), ==,
                   G_IO_ERROR_FAILED);
#endif

#ifdef EXDEV
  g_assert_cmpint (g_io_error_from_errno (EXDEV), ==,
                   G_IO_ERROR_FAILED);
#endif

#ifdef ENODEV
  g_assert_cmpint (g_io_error_from_errno (ENODEV), ==,
                   G_IO_ERROR_NO_SUCH_DEVICE);
#endif

#ifdef ENFILE
  g_assert_cmpint (g_io_error_from_errno (ENFILE), ==,
                   G_IO_ERROR_TOO_MANY_OPEN_FILES);
#endif

#ifdef ENOTTY
  g_assert_cmpint (g_io_error_from_errno (ENOTTY), ==,
                   G_IO_ERROR_FAILED);
#endif

#ifdef ETXTBSY
  g_assert_cmpint (g_io_error_from_errno (ETXTBSY), ==,
                   G_IO_ERROR_BUSY);
#endif

#ifdef EFBIG
  g_assert_cmpint (g_io_error_from_errno (EFBIG), ==,
                   G_IO_ERROR_FAILED);
#endif

#ifdef ESPIPE
  g_assert_cmpint (g_io_error_from_errno (ESPIPE), ==,
                   G_IO_ERROR_FAILED);
#endif

#ifdef EDOM
  g_assert_cmpint (g_io_error_from_errno (EDOM), ==,
                   G_IO_ERROR_FAILED);
#endif

#ifdef ERANGE
  g_assert_cmpint (g_io_error_from_errno (ERANGE), ==,
                   G_IO_ERROR_FAILED);
#endif

#ifdef EDEADLK
  g_assert_cmpuint (g_io_error_from_errno (EDEADLK), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ENOLCK
  g_assert_cmpuint (g_io_error_from_errno (ENOLCK), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ENOSYS
  g_assert_cmpuint (g_io_error_from_errno (ENOSYS), ==,
                    G_IO_ERROR_NOT_SUPPORTED);
#endif

#ifdef ENOMSG
  g_assert_cmpuint (g_io_error_from_errno (ENOMSG), ==,
                    G_IO_ERROR_INVALID_DATA);
#endif

#ifdef EIDRM
  g_assert_cmpuint (g_io_error_from_errno (EIDRM), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ECHRNG
  g_assert_cmpuint (g_io_error_from_errno (ECHRNG), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EL2NSYNC
  g_assert_cmpuint (g_io_error_from_errno (EL2NSYNC), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EL3HLT
  g_assert_cmpuint (g_io_error_from_errno (EL3HLT), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EL3RST
  g_assert_cmpuint (g_io_error_from_errno (EL3RST), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ELNRNG
  g_assert_cmpuint (g_io_error_from_errno (ELNRNG), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EUNATCH
  g_assert_cmpuint (g_io_error_from_errno (EUNATCH), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ENOCSI
  g_assert_cmpuint (g_io_error_from_errno (ENOCSI), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EL2HLT
  g_assert_cmpuint (g_io_error_from_errno (EL2HLT), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EBADE
  g_assert_cmpuint (g_io_error_from_errno (EBADE), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EBADR
  g_assert_cmpuint (g_io_error_from_errno (EBADR), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EXFULL
  g_assert_cmpuint (g_io_error_from_errno (EXFULL), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ENOANO
  g_assert_cmpuint (g_io_error_from_errno (ENOANO), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EBADRQC
  g_assert_cmpuint (g_io_error_from_errno (EBADRQC), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EBADSLT
  g_assert_cmpuint (g_io_error_from_errno (EBADSLT), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EDEADLOCK
  g_assert_cmpuint (g_io_error_from_errno (EDEADLOCK), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EBFONT
  g_assert_cmpuint (g_io_error_from_errno (EBFONT), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ENOSTR
  g_assert_cmpuint (g_io_error_from_errno (ENOSTR), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ENODATA
  g_assert_cmpuint (g_io_error_from_errno (ENODATA), ==,
                    G_IO_ERROR_INVALID_DATA);
#endif

#ifdef ETIME
  g_assert_cmpuint (g_io_error_from_errno (ETIME), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ENOSR
  g_assert_cmpuint (g_io_error_from_errno (ENOSR), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ENONET
  g_assert_cmpuint (g_io_error_from_errno (ENONET), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ENOPKG
  g_assert_cmpuint (g_io_error_from_errno (ENOPKG), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EREMOTE
  g_assert_cmpuint (g_io_error_from_errno (EREMOTE), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ENOLINK
  g_assert_cmpuint (g_io_error_from_errno (ENOLINK), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EADV
  g_assert_cmpuint (g_io_error_from_errno (EADV), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ESRMNT
  g_assert_cmpuint (g_io_error_from_errno (ESRMNT), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ECOMM
  g_assert_cmpuint (g_io_error_from_errno (ECOMM), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EPROTO
  g_assert_cmpuint (g_io_error_from_errno (EPROTO), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EMULTIHOP
  g_assert_cmpuint (g_io_error_from_errno (EMULTIHOP), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EDOTDOT
  g_assert_cmpuint (g_io_error_from_errno (EDOTDOT), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EBADMSG
  g_assert_cmpuint (g_io_error_from_errno (EBADMSG), ==,
                    G_IO_ERROR_INVALID_DATA);
#endif

#ifdef EOVERFLOW
  g_assert_cmpuint (g_io_error_from_errno (EOVERFLOW), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ENOTUNIQ
  g_assert_cmpuint (g_io_error_from_errno (ENOTUNIQ), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EBADFD
  g_assert_cmpuint (g_io_error_from_errno (EBADFD), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EREMCHG
  g_assert_cmpuint (g_io_error_from_errno (EREMCHG), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ELIBACC
  g_assert_cmpuint (g_io_error_from_errno (ELIBACC), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ELIBBAD
  g_assert_cmpuint (g_io_error_from_errno (ELIBBAD), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ELIBSCN
  g_assert_cmpuint (g_io_error_from_errno (ELIBSCN), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ELIBMAX
  g_assert_cmpuint (g_io_error_from_errno (ELIBMAX), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ELIBEXEC
  g_assert_cmpuint (g_io_error_from_errno (ELIBEXEC), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EILSEQ
  g_assert_cmpuint (g_io_error_from_errno (EILSEQ), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ERESTART
  g_assert_cmpuint (g_io_error_from_errno (ERESTART), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ESTRPIPE
  g_assert_cmpuint (g_io_error_from_errno (ESTRPIPE), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EUSERS
  g_assert_cmpuint (g_io_error_from_errno (EUSERS), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EDESTADDRREQ
  g_assert_cmpuint (g_io_error_from_errno (EDESTADDRREQ), ==,
                    G_IO_ERROR_DESTINATION_UNSET);
#endif

#ifdef EPROTOTYPE
  g_assert_cmpuint (g_io_error_from_errno (EPROTOTYPE), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ENOPROTOOPT
  g_assert_cmpuint (g_io_error_from_errno (ENOPROTOOPT), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EADDRNOTAVAIL
  g_assert_cmpuint (g_io_error_from_errno (EADDRNOTAVAIL), ==,
                    G_IO_ERROR_CONNECTION_REFUSED);
#endif

#ifdef ENETDOWN
  g_assert_cmpuint (g_io_error_from_errno (ENETDOWN), ==,
                    G_IO_ERROR_NETWORK_UNREACHABLE);
#endif

#ifdef ECONNABORTED
  g_assert_cmpuint (g_io_error_from_errno (ECONNABORTED), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ENOBUFS
  g_assert_cmpuint (g_io_error_from_errno (ENOBUFS), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EISCONN
  g_assert_cmpuint (g_io_error_from_errno (EISCONN), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ESHUTDOWN
  g_assert_cmpuint (g_io_error_from_errno (ESHUTDOWN), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ETOOMANYREFS
  g_assert_cmpuint (g_io_error_from_errno (ETOOMANYREFS), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EHOSTDOWN
  g_assert_cmpuint (g_io_error_from_errno (EHOSTDOWN), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EALREADY
  g_assert_cmpuint (g_io_error_from_errno (EALREADY), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EINPROGRESS
  g_assert_cmpuint (g_io_error_from_errno (EINPROGRESS), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ESTALE
  g_assert_cmpuint (g_io_error_from_errno (ESTALE), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EUCLEAN
  g_assert_cmpuint (g_io_error_from_errno (EUCLEAN), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ENOTNAM
  g_assert_cmpuint (g_io_error_from_errno (ENOTNAM), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ENAVAIL
  g_assert_cmpuint (g_io_error_from_errno (ENAVAIL), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EISNAM
  g_assert_cmpuint (g_io_error_from_errno (EISNAM), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EREMOTEIO
  g_assert_cmpuint (g_io_error_from_errno (EREMOTEIO), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EDQUOT
  g_assert_cmpuint (g_io_error_from_errno (EDQUOT), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ENOMEDIUM
  g_assert_cmpuint (g_io_error_from_errno (ENOMEDIUM), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EMEDIUMTYPE
  g_assert_cmpuint (g_io_error_from_errno (EMEDIUMTYPE), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ENOKEY
  g_assert_cmpuint (g_io_error_from_errno (ENOKEY), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EKEYEXPIRED
  g_assert_cmpuint (g_io_error_from_errno (EKEYEXPIRED), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EKEYREVOKED
  g_assert_cmpuint (g_io_error_from_errno (EKEYREVOKED), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EKEYREJECTED
  g_assert_cmpuint (g_io_error_from_errno (EKEYREJECTED), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EOWNERDEAD
  g_assert_cmpuint (g_io_error_from_errno (EOWNERDEAD), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ENOTRECOVERABLE
  g_assert_cmpuint (g_io_error_from_errno (ENOTRECOVERABLE), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef ERFKILL
  g_assert_cmpuint (g_io_error_from_errno (ERFKILL), ==,
                    G_IO_ERROR_FAILED);
#endif

#ifdef EHWPOISON
  g_assert_cmpuint (g_io_error_from_errno (EHWPOISON), ==,
                    G_IO_ERROR_FAILED);
#endif
}

static void
test_error_from_file_error (void)
{
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*should not be reached*");
  g_assert_cmpuint (g_io_error_from_file_error (-1), ==,
                    G_IO_ERROR_FAILED);
  g_test_assert_expected_messages ();

  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_EXIST), ==,
                    G_IO_ERROR_EXISTS);
  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_ISDIR), ==,
                    G_IO_ERROR_IS_DIRECTORY);
  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_ACCES), ==,
                    G_IO_ERROR_PERMISSION_DENIED);
  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_NAMETOOLONG), ==,
                    G_IO_ERROR_FILENAME_TOO_LONG);
  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_NOENT), ==,
                    G_IO_ERROR_NOT_FOUND);
  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_NOTDIR), ==,
                    G_IO_ERROR_NOT_DIRECTORY);
  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_NXIO), ==,
                    G_IO_ERROR_NOT_REGULAR_FILE);
  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_NODEV), ==,
                    G_IO_ERROR_NO_SUCH_DEVICE);
  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_ROFS), ==,
                    G_IO_ERROR_READ_ONLY);
  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_TXTBSY), ==,
                    G_IO_ERROR_BUSY);
  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_LOOP), ==,
                    G_IO_ERROR_TOO_MANY_LINKS);
  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_NOSPC), ==,
                    G_IO_ERROR_NO_SPACE);
  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_NOMEM), ==,
                    G_IO_ERROR_NO_SPACE);
  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_MFILE), ==,
                    G_IO_ERROR_TOO_MANY_OPEN_FILES);
  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_NFILE), ==,
                    G_IO_ERROR_TOO_MANY_OPEN_FILES);
  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_INVAL), ==,
                    G_IO_ERROR_INVALID_ARGUMENT);
  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_PIPE), ==,
                    G_IO_ERROR_BROKEN_PIPE);
  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_AGAIN), ==,
                    G_IO_ERROR_WOULD_BLOCK);
  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_PERM), ==,
                    G_IO_ERROR_PERMISSION_DENIED);
  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_NOSYS), ==,
                    G_IO_ERROR_NOT_SUPPORTED);

  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_BADF), ==,
                    G_IO_ERROR_FAILED);
  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_FAILED), ==,
                    G_IO_ERROR_FAILED);
  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_FAULT), ==,
                    G_IO_ERROR_FAILED);
  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_INTR), ==,
                    G_IO_ERROR_FAILED);
  g_assert_cmpuint (g_io_error_from_file_error (G_FILE_ERROR_IO), ==,
                    G_IO_ERROR_FAILED);
}

static void
test_error_from_win32_error (void)
{
#ifdef G_OS_WIN32
  g_assert_cmpint (g_io_error_from_win32_error (-1), ==, G_IO_ERROR_FAILED);

  g_assert_cmpint (g_io_error_from_win32_error (WSAEADDRINUSE), ==,
                   G_IO_ERROR_ADDRESS_IN_USE);

  g_assert_cmpint (g_io_error_from_win32_error (WSAEWOULDBLOCK), ==,
                   G_IO_ERROR_WOULD_BLOCK);

  g_assert_cmpint (g_io_error_from_win32_error (WSAEACCES), ==,
                   G_IO_ERROR_PERMISSION_DENIED);

  g_assert_cmpint (g_io_error_from_win32_error (WSA_INVALID_HANDLE), ==,
                   G_IO_ERROR_INVALID_ARGUMENT);
  g_assert_cmpint (g_io_error_from_win32_error (WSA_INVALID_PARAMETER), ==,
                   G_IO_ERROR_INVALID_ARGUMENT);
  g_assert_cmpint (g_io_error_from_win32_error (WSAEINVAL), ==,
                   G_IO_ERROR_INVALID_ARGUMENT);
  g_assert_cmpint (g_io_error_from_win32_error (WSAEBADF), ==,
                   G_IO_ERROR_INVALID_ARGUMENT);
  g_assert_cmpint (g_io_error_from_win32_error (WSAENOTSOCK), ==,
                   G_IO_ERROR_INVALID_ARGUMENT);

  g_assert_cmpint (g_io_error_from_win32_error (WSAEPROTONOSUPPORT), ==,
                   G_IO_ERROR_NOT_SUPPORTED);

  g_assert_cmpint (g_io_error_from_win32_error (WSAECANCELLED), ==,
                   G_IO_ERROR_CANCELLED);

  g_assert_cmpint (g_io_error_from_win32_error (WSAESOCKTNOSUPPORT), ==,
                   G_IO_ERROR_NOT_SUPPORTED);
  g_assert_cmpint (g_io_error_from_win32_error (WSAEOPNOTSUPP), ==,
                   G_IO_ERROR_NOT_SUPPORTED);
  g_assert_cmpint (g_io_error_from_win32_error (WSAEPFNOSUPPORT), ==,
                   G_IO_ERROR_NOT_SUPPORTED);
  g_assert_cmpint (g_io_error_from_win32_error (WSAEAFNOSUPPORT), ==,
                   G_IO_ERROR_NOT_SUPPORTED);

  g_assert_cmpint (g_io_error_from_win32_error (WSAECONNRESET), ==,
                   G_IO_ERROR_CONNECTION_CLOSED);
  g_assert_cmpint (g_io_error_from_win32_error (WSAENETRESET), ==,
                   G_IO_ERROR_CONNECTION_CLOSED);
  g_assert_cmpint (g_io_error_from_win32_error (WSAESHUTDOWN), ==,
                   G_IO_ERROR_CONNECTION_CLOSED);

  g_assert_cmpint (g_io_error_from_win32_error (WSAEHOSTUNREACH), ==,
                   G_IO_ERROR_HOST_UNREACHABLE);

  g_assert_cmpint (g_io_error_from_win32_error (WSAENETUNREACH), ==,
                   G_IO_ERROR_NETWORK_UNREACHABLE);

  g_assert_cmpint (g_io_error_from_win32_error (WSAECONNREFUSED), ==,
                   G_IO_ERROR_CONNECTION_REFUSED);

  g_assert_cmpint (g_io_error_from_win32_error (WSAETIMEDOUT), ==,
                   G_IO_ERROR_TIMED_OUT);

  g_assert_cmpint (g_io_error_from_win32_error (WSAENOTCONN), ==,
                   G_IO_ERROR_NOT_CONNECTED);
  g_assert_cmpint (g_io_error_from_win32_error (ERROR_PIPE_LISTENING), ==,
                   G_IO_ERROR_NOT_CONNECTED);

  g_assert_cmpint (g_io_error_from_win32_error (WSAEMSGSIZE), ==,
                   G_IO_ERROR_MESSAGE_TOO_LARGE);
#else
  g_test_skip ("Windows error codes can only be checked on Windows");
#endif /* G_OS_WIN32 */
}


int
main (int   argc,
      char *argv[])
{
  g_setenv ("LC_ALL", "C", TRUE);
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add_func ("/error/from-errno", test_error_from_errno);
  g_test_add_func ("/error/from-file-error", test_error_from_file_error);
  g_test_add_func ("/error/from-win32-error", test_error_from_win32_error);

  return g_test_run ();
}
