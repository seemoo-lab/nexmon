/* grefstring.c: Reference counted strings
 *
 * Copyright 2018  Emmanuele Bassi
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

#include "grefstring.h"

#include "ghash.h"
#include "gmessages.h"
#include "gtestutils.h"
#include "gthread.h"

#include <string.h>

/* A global table of refcounted strings; the hash table does not own
 * the strings, just a pointer to them. Strings are interned as long
 * as they are alive; once their reference count drops to zero, they
 * are removed from the table
 */
G_LOCK_DEFINE_STATIC (interned_ref_strings);
static GHashTable *interned_ref_strings;

#if G_GNUC_CHECK_VERSION(4,8) || defined(__clang__)
# define _attribute_aligned(n) __attribute__((aligned(n)))
#elif defined(_MSC_VER)
# define _attribute_aligned(n) __declspec(align(n))
#else
# define _attribute_aligned(n)
#endif

typedef struct {
  /* Length of the string without NUL-terminator */
  size_t len;
  /* Atomic reference count placed here to reduce struct padding */
  int ref_count;
  /* TRUE if interned, FALSE otherwise; immutable after construction */
  guint8 interned;

  /* First character of the actual string
   * Make sure it is at least 2 * sizeof (size_t) aligned to allow for SIMD
   * optimizations in operations on the string.
   * Because MSVC sucks we need to handle both cases explicitly. */
#if GLIB_SIZEOF_SIZE_T == 4
  _attribute_aligned (8) char s[];
#elif GLIB_SIZEOF_SIZE_T == 8
  _attribute_aligned (16) char s[];
#else
#error "Only 32 bit and 64 bit size_t supported currently"
#endif
} GRefStringImpl;

/* Assert that the start of the actual string is at least 2 * alignof (size_t) aligned */
G_STATIC_ASSERT (offsetof (GRefStringImpl, s) % (2 * G_ALIGNOF (size_t)) == 0);

/* Gets a pointer to the GRefStringImpl from its string pointer */
#define G_REF_STRING_IMPL_FROM_STR(str) ((GRefStringImpl *) ((guint8 *) str - offsetof (GRefStringImpl, s)))
/* Gets a pointer to the string pointer from the GRefStringImpl */
#define G_REF_STRING_IMPL_TO_STR(str) (str->s)

/**
 * g_ref_string_new:
 * @str: (not nullable): a NUL-terminated string
 *
 * Creates a new reference counted string and copies the contents of @str
 * into it.
 *
 * Returns: (transfer full) (not nullable): the newly created reference counted string
 *
 * Since: 2.58
 */
char *
g_ref_string_new (const char *str)
{
  GRefStringImpl *impl;
  gsize len;

  g_return_val_if_fail (str != NULL, NULL);

  len = strlen (str);

  if (sizeof (char) * len > G_MAXSIZE - sizeof (GRefStringImpl) - 1)
    g_error ("GRefString allocation would overflow");

  impl = g_malloc (sizeof (GRefStringImpl) + sizeof (char) * len + 1);
  impl->len = len;
  impl->ref_count = 1;
  impl->interned = FALSE;
  memcpy (G_REF_STRING_IMPL_TO_STR (impl), str, len + 1);

  return G_REF_STRING_IMPL_TO_STR (impl);
}

/**
 * g_ref_string_new_len:
 * @str: (not nullable): a string
 * @len: length of @str to use, or -1 if @str is nul-terminated
 *
 * Creates a new reference counted string and copies the contents of @str
 * into it, up to @len bytes.
 *
 * Since this function does not stop at nul bytes, it is the caller's
 * responsibility to ensure that @str has at least @len addressable bytes.
 *
 * Returns: (transfer full) (not nullable): the newly created reference counted string
 *
 * Since: 2.58
 */
char *
g_ref_string_new_len (const char *str, gssize len)
{
  GRefStringImpl *impl;

  g_return_val_if_fail (str != NULL, NULL);

  if (len < 0)
    return g_ref_string_new (str);

  /* allocate then copy as str[len] may not be readable */
  if (sizeof (char) * len > G_MAXSIZE - sizeof (GRefStringImpl) - 1)
    g_error ("GRefString allocation would overflow");

  impl = g_malloc (sizeof (GRefStringImpl) + sizeof (char) * len + 1);
  impl->len = len;
  impl->ref_count = 1;
  impl->interned = FALSE;
  memcpy (G_REF_STRING_IMPL_TO_STR (impl), str, len);
  G_REF_STRING_IMPL_TO_STR (impl)[len] = '\0';

  return G_REF_STRING_IMPL_TO_STR (impl);
}

/* interned_str_equal: variant of g_str_equal() that compares
 * pointers as well as contents; this avoids running strcmp()
 * on arbitrarily long strings, as it's more likely to have
 * g_ref_string_new_intern() being called on the same refcounted
 * string instance, than on a different string with the same
 * contents
 */
static gboolean
interned_str_equal (gconstpointer v1,
                    gconstpointer v2)
{
  const char *str1 = v1;
  const char *str2 = v2;

  if (v1 == v2)
    return TRUE;

  return strcmp (str1, str2) == 0;
}

/**
 * g_ref_string_new_intern:
 * @str: (not nullable): a NUL-terminated string
 *
 * Creates a new reference counted string and copies the content of @str
 * into it.
 *
 * If you call this function multiple times with the same @str, or with
 * the same contents of @str, it will return a new reference, instead of
 * creating a new string.
 *
 * Returns: (transfer full) (not nullable): the newly created reference
 *   counted string, or a new reference to an existing string
 *
 * Since: 2.58
 */
char *
g_ref_string_new_intern (const char *str)
{
  char *res;

  g_return_val_if_fail (str != NULL, NULL);

  G_LOCK (interned_ref_strings);

  if (G_UNLIKELY (interned_ref_strings == NULL))
    interned_ref_strings = g_hash_table_new (g_str_hash, interned_str_equal);

  res = g_hash_table_lookup (interned_ref_strings, str);
  if (res != NULL)
    {
      GRefStringImpl *impl = G_REF_STRING_IMPL_FROM_STR (res);
      g_atomic_int_inc (&impl->ref_count);
      G_UNLOCK (interned_ref_strings);
      return res;
    }

  res = g_ref_string_new (str);
  G_REF_STRING_IMPL_FROM_STR (res)->interned = TRUE;
  g_hash_table_add (interned_ref_strings, res);
  G_UNLOCK (interned_ref_strings);

  return res;
}

/**
 * g_ref_string_acquire:
 * @str: a reference counted string
 *
 * Acquires a reference on a string.
 *
 * Returns: the given string, with its reference count increased
 *
 * Since: 2.58
 */
char *
g_ref_string_acquire (char *str)
{
  g_return_val_if_fail (str != NULL, NULL);

  g_atomic_int_inc (&G_REF_STRING_IMPL_FROM_STR (str)->ref_count);

  return str;
}

/**
 * g_ref_string_release:
 * @str: a reference counted string
 *
 * Releases a reference on a string; if it was the last reference, the
 * resources allocated by the string are freed as well.
 *
 * Since: 2.58
 */
void
g_ref_string_release (char *str)
{
  GRefStringImpl *impl;
  int old_ref_count;

  g_return_if_fail (str != NULL);

  impl = G_REF_STRING_IMPL_FROM_STR (str);

  /* Non-interned strings are easy so let's get that out of the way here first */
  if (!impl->interned)
    {
      if (g_atomic_int_dec_and_test (&impl->ref_count))
        g_free (impl);
      return;
    }

  old_ref_count = g_atomic_int_get (&impl->ref_count);
  g_assert (old_ref_count >= 1);
retry:
  /* Fast path: multiple references, we can just try decrementing and be done with it */
  if (old_ref_count > 1)
    {
      /* If the reference count stayed the same we're done, otherwise retry */
      if (!g_atomic_int_compare_and_exchange_full (&impl->ref_count, old_ref_count, old_ref_count - 1, &old_ref_count))
        goto retry;

      return;
    }

  /* This is the last reference *currently* and would potentially free the string.
   * To avoid races between freeing it and returning it from g_ref_string_new_intern()
   * we must take the lock here before decrementing the reference count!
   */
  G_LOCK (interned_ref_strings);
  /* If the string was not given out again in the meantime we're done */
  if (g_atomic_int_dec_and_test (&impl->ref_count))
    {
      gboolean removed G_GNUC_UNUSED  /* when compiling with G_DISABLE_ASSERT */;

      removed = g_hash_table_remove (interned_ref_strings, str);
      g_assert (removed);

      if (g_hash_table_size (interned_ref_strings) == 0)
        g_clear_pointer (&interned_ref_strings, g_hash_table_destroy);

      g_free (impl);
    }
  G_UNLOCK (interned_ref_strings);
}

/**
 * g_ref_string_length:
 * @str: a reference counted string
 *
 * Retrieves the length of @str.
 *
 * Returns: the length of the given string, in bytes
 *
 * Since: 2.58
 */
gsize
g_ref_string_length (char *str)
{
  g_return_val_if_fail (str != NULL, 0);

  return G_REF_STRING_IMPL_FROM_STR (str)->len;
}

/**
 * g_ref_string_equal:
 * @str1: a reference counted string
 * @str2: a reference counted string
 *
 * Compares two ref-counted strings for byte-by-byte equality.
 *
 * It can be passed to [func@GLib.HashTable.new] as the key equality function,
 * and behaves exactly the same as [func@GLib.str_equal] (or `strcmp()`), but
 * can return slightly faster as it can check the string lengths before checking
 * all the bytes.
 *
 * Returns: `TRUE` if the strings are equal, otherwise `FALSE`
 *
 * Since: 2.84
 */
gboolean
g_ref_string_equal (const char *str1,
                    const char *str2)
{
  if (G_REF_STRING_IMPL_FROM_STR ((char *) str1)->len != G_REF_STRING_IMPL_FROM_STR ((char *) str2)->len)
    return FALSE;

  return strcmp (str1, str2) == 0;
}
