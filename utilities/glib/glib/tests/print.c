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

#include "glib.h"
#include "gprintf.h"

#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

static void
test_print_console (void)
{
#ifdef _WIN32
  UINT previous_codepage = CP_UTF8;
  const DWORD access_rights = GENERIC_READ | GENERIC_WRITE;
  const DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
  const DWORD flags = CONSOLE_TEXTMODE_BUFFER;
  HANDLE handle;
  int fd;
  FILE *stream;
  const char *utf8;
  wchar_t *utf16;
  DWORD length_cells;
  wchar_t *aux;
  DWORD aux_read;
  BOOL ret;

  /* A process can be attached to at most one console. If the calling process is
   * already attached to a console, the error code returned is ERROR_ACCESS_DENIED
   */
  g_assert_true (AllocConsole () || GetLastError () == ERROR_ACCESS_DENIED);

  /* Set a limited codepage */
  previous_codepage = GetConsoleOutputCP ();
  g_assert_true (IsValidCodePage (1252));
  SetConsoleOutputCP (1252);

  handle = CreateConsoleScreenBuffer (access_rights, share_mode, NULL, flags, NULL);
  g_assert_true (handle != INVALID_HANDLE_VALUE);

  fd = _open_osfhandle ((intptr_t)handle, _O_BINARY);
  g_assert_cmpint (fd, >=, 0);

  stream = _fdopen (fd, "w");
  g_assert_nonnull (stream);

  /* Use only characters in the Basic Multilingual Plane (BMP).
   * We can write any UTF-8 string, but the read-back API used
   * in this test only supports UCS2 */
  utf8 = "\u00E0"    /* Latin Small Letter A with Grave */
         "\u03B1"    /* Greek Small Letter Alpha */
         "\u3041";   /* Hiragana Letter Small A */
  g_fprintf (stream, "%s", utf8);

  fflush (stream);
  g_assert_false (ferror (stream));

  utf16 = g_utf8_to_utf16 (utf8, -1, NULL, NULL, NULL);
  g_assert_nonnull (utf16);

  /* CJK characters usually are "wide", they take the space of
   * two cells (see wcwidth()). Probably newer terminals use
   * one cell per grapheme cluster, but let's just multiply
   * by two to account for wide characters */
  length_cells = (DWORD)wcslen (utf16) * 2 + 1;
  aux = g_new (wchar_t, length_cells);

  ret = ReadConsoleOutputCharacter (handle,
                                    aux,
                                    length_cells,
                                    (COORD){0, 0},
                                    &aux_read);
  g_assert_true (ret);

  aux[aux_read] = L'\0';

  if (wcsncmp (aux, utf16, wcslen (utf16)) != 0)
    {
      char *aux_utf8 = g_utf16_to_utf8 (aux, aux_read, NULL, NULL, NULL);

      g_test_fail_printf ("string `%s' was written as `%s'", utf8, aux_utf8);
      g_free (aux_utf8);
    }

  g_clear_pointer (&aux, g_free);
  g_clear_pointer (&utf16, g_free);

  /* Closes all of stream, fd, and handle */
  g_assert_cmpint (fclose (stream), !=, EOF);

  SetConsoleOutputCP (previous_codepage);
#else
  g_test_skip ("Testing console output only supported on Windows");
#endif
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/print-console", test_print_console);

  return g_test_run ();
}
