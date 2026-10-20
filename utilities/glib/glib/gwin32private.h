/*
 * Copyright © 2025 Luca Bacci
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Luca Bacci <luca.bacci@outlook.com>
 */

#ifndef __G_WIN32PRIVATE_H__
#define __G_WIN32PRIVATE_H__

#ifdef G_PLATFORM_WIN32

#include "config.h"

#include <windows.h>

#include <stdio.h>
#include <stdbool.h>

bool
g_win32_file_stream_is_console_output (FILE *stream);

bool
g_win32_handle_is_console_output (HANDLE handle);

void
g_win32_api_failed_with_code (const char *where,
                              const char *api,
                              DWORD       code);

void
g_win32_api_failed (const char *where,
                    const char *api);

#define WIN32_API_FAILED_WITH_CODE(api, code) do { g_win32_api_failed_with_code (G_STRLOC, api, code); } while (0)
#define WIN32_API_FAILED(api) do { g_win32_api_failed (G_STRLOC, api); } while (0)

bool
g_win32_error_message_in_place   (DWORD          code,
                                  wchar_t       *buffer,
                                  size_t         wchars_count);

bool
g_win32_substitute_pid_and_event (wchar_t       *local_debugger,
                                  gsize          debugger_size,
                                  const wchar_t *cmdline,
                                  DWORD          pid,
                                  guintptr       event);

#endif /* G_PLATFORM_WIN32 */

#endif /* __G_WIN32PRIVATE_H__ */
