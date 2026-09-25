/*
 * Copyright 2016 Red Hat, Inc.
 * Copyright 2016-2022 Collabora Ltd.
 * Copyright 2017-2022 Endless OS Foundation, LLC
 * Copyright 2018 Will Thompson
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "gjournal-private.h"

#if defined(__linux__) && !defined(__ANDROID__)
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

/*
 * Reimplementation of g_str_has_prefix(), necessary when compiled into
 * gio-launch-desktop.
 */
static int
str_has_prefix (const char *str,
                const char *prefix)
{
  return strncmp (str, prefix, strlen (prefix)) == 0;
}

/*
 * _g_fd_is_journal:
 * @output_fd: output file descriptor to check
 *
 * Same as g_log_writer_is_journald(), but with no GLib dependencies.
 *
 * Returns: 1 if @output_fd points to the journal, 0 otherwise
 */
int
_g_fd_is_journal (int output_fd)
{
  /* FIXME: Use the new journal API for detecting whether we’re writing to the
   * journal. See: https://github.com/systemd/systemd/issues/2473
   */
  union {
    struct sockaddr_storage storage;
    struct sockaddr sa;
    struct sockaddr_un un;
  } addr;
  socklen_t addr_len;
  int err;

  if (output_fd < 0)
    return 0;

  /* Namespaced journals start with `/run/systemd/journal.${name}/` (see
   * `RuntimeDirectory=systemd/journal.%i` in `systemd-journald@.service`. The
   * default journal starts with `/run/systemd/journal/`. */
  memset (&addr, 0, sizeof (addr));
  addr_len = sizeof(addr);
  err = getpeername (output_fd, &addr.sa, &addr_len);
  if (err == 0 && addr.storage.ss_family == AF_UNIX)
    return (str_has_prefix (addr.un.sun_path, "/run/systemd/journal/") ||
            str_has_prefix (addr.un.sun_path, "/run/systemd/journal."));

  return 0;
}
#endif
