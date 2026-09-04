/* constructor-helpers.c - Helper library for the constructor test
 *
 * Copyright © 2023 Luca Bacci
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

/* This helper library manages a set of strings. Strings can be added,
 * removed and checked for presence. This library does not call into
 * libc, so can be used from module constructors in a safe manner on
 * a wide range of operating systems.
 *
 * GLib is used only for assertions or hard errors; in such cases we
 * don't really care about supported libc calls, as the test ought to
 * fail anyway.
 */

#include <stddef.h> /* for size_t */

#include <glib.h>

#if defined (_MSC_VER)
#  pragma optimize ("", off)
#else
#  if defined (__clang__)
#    pragma clang optimize off
#  elif defined (__GNUC__)
#    pragma GCC optimize ("O0")
#  endif
#endif

#if defined(_WIN32)
  #define MODULE_EXPORT \
    __declspec (dllexport)
#elif defined (__GNUC__)
  #define MODULE_EXPORT \
    __attribute__((visibility("default")))
#else
  #define MODULE_EXPORT
#endif

char buffer[500];
size_t position;

MODULE_EXPORT
void string_add (const char *string);

MODULE_EXPORT
void string_add_exclusive (const char *string);

MODULE_EXPORT
int string_remove (const char *string);

MODULE_EXPORT
int string_find (const char *string);

MODULE_EXPORT
void string_check (const char *string);

static size_t
strlen_ (const char *string)
{
  size_t i = 0;

  g_assert_nonnull (string);

  while (string[i] != 0)
    i++;

  return i;
}

static void
memmove_ (char *buf,
          size_t from,
          size_t size,
          size_t to)
{
  g_assert_true (to <= from);

  for (size_t i = 0; i < size; i++)
    buffer[to + i] = buffer[from + i];
}

static void
memcpy_ (char *dst,
         const char *src,
         size_t size)
{
  for (size_t i = 0; i < size; i++)
    dst[i] = src[i];
}

static void
memset_ (char   *dst,
         char    val,
         size_t  size)
{
  for (size_t i = 0; i < size; i++)
    dst[i] = val;
}

static size_t
string_find_index_ (const char *string)
{
  size_t string_len;
  size_t i = 0;

  g_assert_nonnull (string);
  g_assert_true ((string_len = strlen_ (string)) > 0);

  for (i = 0; (i < sizeof (buffer) - position) && (buffer[i] != 0);)
    {
      const char *iter = &buffer[i];
      size_t len = strlen_ (iter);

      if (len == string_len && strcmp (iter, string) == 0)
        return i;

      i += len + 1;
    }

  return sizeof (buffer);
}

/**< private >
 * string_add:
 *
 * @string: NULL-terminated string. Must not be empty
 *
 * Adds @string to the set
 */
MODULE_EXPORT
void
string_add (const char *string)
{
  size_t len, size;

  g_assert_nonnull (string);
  g_assert_true ((len = strlen_ (string)) > 0);

  size = len + 1;

  if (size > sizeof (buffer) - position)
    g_error ("Not enough space in the buffer");

  memcpy_ (buffer + position, string, size);

  position += size;
}

/**< private >
 * string_add_exclusive:
 *
 * @string: NULL-terminated string. Must not be empty
 *
 * Adds @string to the set, asserting that it's not already present.
 */
MODULE_EXPORT
void
string_add_exclusive (const char *string)
{
  if (string_find_index_ (string) < sizeof (buffer))
    g_error ("string %s already set", string);

  string_add (string);
}

/**< private >
 * string_remove:
 *
 * @string: NULL-terminated string. Must not be empty
 *
 * Removes the first occurrence of @string from the set.
 *
 * Returns: 1 if the string was removed, 0 otherwise.
 */
MODULE_EXPORT
int
string_remove (const char *string)
{
  size_t index = string_find_index_ (string);
  size_t len, size;

  if (index >= sizeof (buffer))
    return 0;

  g_assert_true ((len = strlen_ (string)) > 0);
  size = len + 1;

  memmove_ (buffer, index + size, index, position - (index + size));

  position -= size;

  memset_ (buffer + position, 0, size);

  return 1;
}

/**< private >
 * string_find:
 *
 * @string: NULL-terminated string. Must not be empty
 *
 * Returns 1 if the string is present, 0 otherwise
 */
MODULE_EXPORT
int string_find (const char *string)
{
  return string_find_index_ (string) < sizeof (buffer);
}

/**< private >
 * string_check:
 *
 * @string: NULL-terminated string. Must not be empty
 *
 * Asserts that @string is present in the set
 */
MODULE_EXPORT
void
string_check (const char *string)
{
  if (string_find_index_ (string) >= sizeof (buffer))
    g_error ("String %s not present", string);
}
