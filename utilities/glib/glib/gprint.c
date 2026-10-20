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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include "gtypes.h"
#include "gunicodeprivate.h"
#include "gprintprivate.h"
#include "gprintfint.h"

#ifndef _WIN32
#include <unistd.h>
#else
#include "gwin32private.h"
#include <io.h>
#endif

#define CHAR_IS_SAFE(wc) (!((wc < 0x20 && wc != '\t' && wc != '\n' && wc != '\r') || \
                            (wc == 0x7f) || \
                            (wc >= 0x80 && wc < 0xa0)))

char *
g_print_convert (const char *string,
                 const char *charset)
{
  if (!g_utf8_validate (string, -1, NULL))
    {
      GString *gstring = g_string_new ("[Invalid UTF-8] ");
      guchar *p;

      for (p = (guchar *)string; *p; p++)
        {
          if (CHAR_IS_SAFE(*p) &&
              !(*p == '\r' && *(p + 1) != '\n') &&
              *p < 0x80)
            g_string_append_c (gstring, *p);
          else
            g_string_append_printf (gstring, "\\x%02x", (guint)(guchar)*p);
        }

      return g_string_free (gstring, FALSE);
    }
  else
    {
      GError *err = NULL;

      gchar *result = g_convert_with_fallback (string, -1, charset, "UTF-8", "?", NULL, NULL, &err);
      if (result)
        return result;
      else
        {
          /* Not thread-safe, but doesn't matter if we print the warning twice
           */
          static gboolean warned = FALSE;
          if (!warned)
            {
              warned = TRUE;
              _g_fprintf (stderr, "GLib: Cannot convert message: %s\n", err->message);
            }
          g_error_free (err);

          return g_strdup (string);
        }
    }
}

#ifdef _WIN32

static int
print_console_nolock (const char *string,
                      FILE       *stream)
{
  HANDLE handle = (HANDLE) _get_osfhandle (_fileno (stream));
  size_t size = strlen (string);
  DWORD written = 0;

  if (size > INT_MAX)
    return 0;

  /* WriteFile are WriteConsole are limited to DWORD lengths,
   * but int and DWORD should are of the same size, so we don't
   * care.
   */
  G_STATIC_ASSERT (INT_MAX <= MAXDWORD);

  /* We might also check if the source string is ASCII */
  if (GetConsoleOutputCP () == CP_UTF8)
    {
      /* If the output codepage is UTF-8, we can just call WriteFile,
       * avoiding a conversion to UTF-16 (which probably will be done
       * by ConDrv).
       */
      /* Note: we cannot use fputs() here. When outputting to the
       * console, the UCRT converts the passed string to the console
       * charset, which is UTF-8, but interprets the string in the
       * LC_CTYPE charset, which can be anything.
       */

      if (!WriteFile (handle, string, size, &written, NULL))
        WIN32_API_FAILED ("WriteFile");
    }
  else
    {
      /* Convert to UTF-16 and output using WriteConsole */

      /* Note: we can't use fputws() with mode _O_U16TEXT because:
       *
       * - file descriptors cannot be locked, unlike FILE streams, so
       *   we cannot set a custom mode on the file descriptor.
       * - the fputws() implementation is not very good: it outputs codeunit
       *   by codeunit in a loop, so it's slow [1] and breaks UTF-16 surrogate
       *   pairs [2].
       *
       * [1] https://github.com/microsoft/terminal/issues/18124#issuecomment-2451987873
       * [2] https://developercommunity.visualstudio.com/t/wprintf-with-_setmode-_O_U16TEXT-or-_O_U/10447076
       */

      wchar_t buffer[1024];
      wchar_t *utf16 = NULL;
      size_t utf16_len = 0;
      DWORD utf16_written = 0;

      g_utf8_to_utf16_make_valid (string,
                                  buffer, G_N_ELEMENTS (buffer),
                                  &utf16, &utf16_len);

      /* The length of the UTF-16 string (in count of gunichar2) cannot be
       * greater than the length of the UTF-8 string (in count of bytes).
       * So utf16_len <= size <= INT_MAX <= MAXDWORD.
       */
      g_assert (utf16_len <= size);

      if (!WriteConsole (handle, utf16, utf16_len, &utf16_written, NULL))
        WIN32_API_FAILED ("WriteConsole");

      if (utf16_written < utf16_len)
        {
          written = g_utf8_to_utf16_make_valid_backtrack (string, utf16_written);
        }
      else
        {
          written = size;
        }

      if (utf16 != buffer)
        g_free (utf16);
    }

  if (written > INT_MAX)
    written = INT_MAX;

  return (int) written;
}

static int
print_console (const char *string,
               FILE       *stream)
{
  int ret;

  /* Locking the stream is not important, but leads
   * to nicer output in case of concurrent writes.
   */
  _lock_file (stream);

#if defined (_MSC_VER) || defined (_UCRT)
  _fflush_nolock (stream);
#else
  fflush (stream);
#endif

  ret = print_console_nolock (string, stream);

  _unlock_file (stream);

  return ret;
}

#endif /* _WIN32 */

static int
print_string (char const *string,
              FILE       *stream)
{
  size_t written = fwrite (string, 1, strlen (string), stream);

  return MIN (written, INT_MAX);
}

int
g_fputs (char const *string,
         FILE       *stream)
{
  int ret;

#ifdef _WIN32
  if (g_win32_file_stream_is_console_output (stream))
    {
      ret = print_console (string, stream);

      if (string[ret] != '\0')
        ret += print_string (&string[ret], stream);
    }
  else
    {
      ret = print_string (string, stream);
    }
#else
  const char *charset;

  if (isatty (fileno (stream)) &&
      !g_get_charset (&charset))
    {
      char *converted = g_print_convert (string, charset);
      ret = print_string (converted, stream);
      g_free (converted);
    }
  else
    {
      ret = print_string (string, stream);
    }
#endif

  return ret;
}
