/*
 * Copyright © 2019 Руслан Ижбулатов
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
 */

#include "config.h"

#include <winsock2.h>
#include <windows.h>

#include <glib.h>
#include <glib/gwin32private.h>

static void
test_handle_is_console_output_files (void)
{
  const struct {
    const wchar_t *path;
    DWORD access_rights;
    bool expected_value;
  } tests[] = {
    { L"CONOUT$", GENERIC_WRITE,                true },
    { L"CONIN$",  GENERIC_WRITE | GENERIC_READ, false },
    { L"NUL",     GENERIC_WRITE,                false },
  };
  HANDLE handle;

  /* ERROR_ACCESS_DENIED is returned if this process
   * is already associated with a console. */
  g_assert_true (AllocConsole () || GetLastError () == ERROR_ACCESS_DENIED);

  for (size_t i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      const DWORD share_mode = FILE_SHARE_READ |
                               FILE_SHARE_WRITE |
                               FILE_SHARE_DELETE;

      handle = CreateFile (tests[i].path,
                           tests[i].access_rights,
                           share_mode,
                           NULL,
                           OPEN_EXISTING,
                           0, NULL);
      g_assert_true (handle != INVALID_HANDLE_VALUE);

      if (tests[i].expected_value)
        g_assert_true (g_win32_handle_is_console_output (handle));
      else
        g_assert_false (g_win32_handle_is_console_output (handle));

      CloseHandle (handle);
    }
}

static void
test_handle_is_console_output_screen_buffer (void)
{
  const DWORD access_rights = GENERIC_WRITE;
  const DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
  const DWORD flags = CONSOLE_TEXTMODE_BUFFER;
  HANDLE handle;

  handle = CreateConsoleScreenBuffer (access_rights, share_mode, NULL, flags, NULL);

  g_assert_true (handle != INVALID_HANDLE_VALUE);
  g_assert_true (g_win32_handle_is_console_output (handle));

  CloseHandle (handle);
}

static void
test_handle_is_console_output_socket (void)
{
  const DWORD flags = WSA_FLAG_OVERLAPPED |
                      WSA_FLAG_NO_HANDLE_INHERIT;
  SOCKET sock;

  WSADATA wsa_data;
  int code = WSAStartup (MAKEWORD (2, 2), &wsa_data);
  if (code)
    g_error ("%s failed with error code %d", "WSAStartup", code);

  sock = WSASocket (AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, flags);
  if (sock == INVALID_SOCKET)
    g_error ("%s failed with error code %u", "WSASocket", WSAGetLastError ());

  g_assert_false (g_win32_handle_is_console_output ((HANDLE)sock));

  closesocket (sock);
  WSACleanup ();
}

static void
test_error_message (void)
{
  wchar_t buffer[100];
  size_t length;

  g_win32_error_message_in_place (ERROR_PATH_NOT_FOUND, buffer, G_N_ELEMENTS (buffer));
  length = wcslen (buffer);

  g_assert_cmpint (length, >, 1);
  g_assert_cmpint (length, <, G_N_ELEMENTS (buffer));
  g_assert_cmpint (buffer[length - 1], !=, L'\n');

  /* Test small buffer */
  for (size_t i = 0; i < 10; i++)
    buffer[i] = L'\0';
  g_win32_error_message_in_place (ERROR_PATH_NOT_FOUND, buffer, 10);
  g_assert_cmpint (buffer[10 - 1], ==, L'\0');
}

static void
test_substitute_pid_and_event (void)
{
  const wchar_t not_enough[] = L"too long when %e and %p are substituted";
  wchar_t debugger_3[3];
  wchar_t debugger_not_enough[G_N_ELEMENTS (not_enough)];
  wchar_t debugger_enough[G_N_ELEMENTS (not_enough) + 1];
  char *debugger_enough_utf8;
  wchar_t debugger_big[65535] = {0};
  char *debugger_big_utf8;
  gchar *output;
  guintptr be = (guintptr) 0xFFFFFFFF;
  DWORD bp = MAXDWORD;

  /* %f is not valid */
  g_assert_false (g_win32_substitute_pid_and_event (debugger_3, G_N_ELEMENTS (debugger_3),
                                                    L"%f", 0, 0));

  g_assert_false (g_win32_substitute_pid_and_event (debugger_3, G_N_ELEMENTS (debugger_3),
                                                    L"string longer than 10", 0, 0));
  /* 200 is longer than %e, so the string doesn't fit by 1 byte */
  g_assert_false (g_win32_substitute_pid_and_event (debugger_not_enough, G_N_ELEMENTS (debugger_not_enough),
                                                    not_enough, 10, 200));

  /* This should fit */
  g_assert_true (g_win32_substitute_pid_and_event (debugger_enough, G_N_ELEMENTS (debugger_enough),
                                                   not_enough, 10, 200));
  debugger_enough_utf8 = g_utf16_to_utf8 (debugger_enough, -1, NULL, NULL, NULL);
  g_assert_cmpstr (debugger_enough_utf8, ==, "too long when 200 and 10 are substituted");
  g_free (debugger_enough_utf8);

  g_assert_true (g_win32_substitute_pid_and_event (debugger_big, G_N_ELEMENTS (debugger_big),
                                                   L"multipl%e big %e %entries and %pids are %provided here", bp, be));
  debugger_big_utf8 = g_utf16_to_utf8 (debugger_big, -1, NULL, NULL, NULL);
  output = g_strdup_printf ("multipl%llu big %llu %lluntries and %luids are %lurovided here", (guint64) be, (guint64) be, (guint64) be, bp, bp);
  g_assert_cmpstr (debugger_big_utf8, ==, output);
  g_free (debugger_big_utf8);
  g_free (output);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/win32/handle-is-console-output/files",
                   test_handle_is_console_output_files);
  g_test_add_func ("/win32/handle-is-console-output/screen-buffer",
                   test_handle_is_console_output_screen_buffer);
  g_test_add_func ("/win32/handle-is-console-output/socket",
                   test_handle_is_console_output_socket);

  g_test_add_func ("/win32/error-message", test_error_message);

  g_test_add_func ("/win32/substitute-pid-and-event", test_substitute_pid_and_event);

  return g_test_run();
}
