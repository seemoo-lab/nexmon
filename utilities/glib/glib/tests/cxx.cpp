/*
 * Copyright 2020 Xavier Claessens
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

#include <glib.h>

#ifdef G_OS_WIN32
#include <wincodec.h>
#endif

#if !defined (G_CXX_STD_VERSION) || !defined (G_CXX_STD_CHECK_VERSION)
#error G_CXX_STD_VERSION is not defined
#endif

#ifdef G_C_STD_VERSION
#error G_C_STD_VERSION should be undefined in C programs
#endif

G_STATIC_ASSERT (G_CXX_STD_VERSION);
G_STATIC_ASSERT (!G_C_STD_CHECK_VERSION (99));

#if G_CXX_STD_VERSION >= 199711L
  G_STATIC_ASSERT (G_CXX_STD_CHECK_VERSION (98));
  G_STATIC_ASSERT (G_CXX_STD_CHECK_VERSION (199711L));
  G_STATIC_ASSERT (G_CXX_STD_CHECK_VERSION (03));
#endif

#if G_CXX_STD_VERSION == 199711L
  G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (11));
  G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (201103L));
  G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (14));
  G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (201402L));
  G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (17));
  G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (201703L));
  G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (20));
  G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (202002L));
#endif

#if G_CXX_STD_VERSION >= 201103L
  G_STATIC_ASSERT (G_CXX_STD_CHECK_VERSION (98));
  G_STATIC_ASSERT (G_CXX_STD_CHECK_VERSION (199711L));
  G_STATIC_ASSERT (G_CXX_STD_CHECK_VERSION (03));
  G_STATIC_ASSERT (G_CXX_STD_CHECK_VERSION (11));
  G_STATIC_ASSERT (G_CXX_STD_CHECK_VERSION (201103L));
#endif

#if G_CXX_STD_VERSION == 201103L
  G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (14));
  G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (201402L));
  G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (17));
  G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (201703L));
  G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (20));
  G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (202002L));
#endif

#if G_CXX_STD_VERSION >= 201402L
  G_STATIC_ASSERT (G_CXX_STD_CHECK_VERSION (14));
  G_STATIC_ASSERT (G_CXX_STD_CHECK_VERSION (201402L));
#endif

#if G_CXX_STD_VERSION == 201402L
  G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (17));
  G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (201703L));
  G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (20));
  G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (202002L));
#endif

#if G_CXX_STD_VERSION >= 201703L
  G_STATIC_ASSERT (G_CXX_STD_CHECK_VERSION (17));
  G_STATIC_ASSERT (G_CXX_STD_CHECK_VERSION (201703L));
#endif

#if G_CXX_STD_VERSION == 201703L
  G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (20));
  G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (202002L));
#endif

#if G_CXX_STD_VERSION >= 202002L
  G_STATIC_ASSERT (G_CXX_STD_CHECK_VERSION (20));
  G_STATIC_ASSERT (G_CXX_STD_CHECK_VERSION (202002L));
#endif

#if G_CXX_STD_VERSION == 202002L
  G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (23));
  G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (202302L));
#endif

#if G_CXX_STD_VERSION >= 202302L
  G_STATIC_ASSERT (G_CXX_STD_CHECK_VERSION (23));
  G_STATIC_ASSERT (G_CXX_STD_CHECK_VERSION (202302L));
#endif

#if G_CXX_STD_VERSION == 202302L
  G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (26));
  G_STATIC_ASSERT (!G_CXX_STD_CHECK_VERSION (202600L));
#endif

#ifdef _G_EXPECTED_CXX_STANDARD
static void
test_cpp_standard (void)
{
  guint64 std_version = 0;

  if (!g_ascii_string_to_unsigned (_G_EXPECTED_CXX_STANDARD, 10, 0, G_MAXUINT64,
                                   &std_version, NULL))
    {
      g_test_skip ("Expected standard value is non-numeric: "
                   _G_EXPECTED_CXX_STANDARD);
      return;
    }

#if !G_GNUC_CHECK_VERSION (11, 0)
  if (std_version >= 20)
    {
      // See: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=93821
      g_test_skip ("Expected standard version is not properly supported by compiler");
      return;
    }
#endif

#if !G_GNUC_CHECK_VERSION(14, 0)
  if (std_version >= 23)
    {
      // See: https://gcc.gnu.org/git/?p=gcc.git;a=commit;h=5388a43f6a3f348929292998bd6d0c1da6f006de
      g_test_skip ("Expected standard version is not properly supported by compiler");
      return;
    }
#endif

  g_test_message ("Checking if '" G_STRINGIFY (G_CXX_STD_VERSION) "' respects "
                  "the expected  standard version '" _G_EXPECTED_CXX_STANDARD "'");
  g_assert_true (G_CXX_STD_CHECK_VERSION (std_version));

  if (std_version < 10 || std_version > 90)
    std_version = 97;

  if (std_version >= 90)
    g_assert_cmpuint (G_CXX_STD_VERSION, >=, (std_version + 1900) * 100);
  else
    g_assert_cmpuint (G_CXX_STD_VERSION, >=, (std_version + 2000) * 100);
}
#endif

typedef struct
{
  int dummy;
} MyObject;

static void
test_typeof (void)
{
  MyObject *obj = g_rc_box_new0 (MyObject);
  MyObject *obj2 = g_rc_box_acquire (obj);
  g_assert_true (obj2 == obj);

  G_STATIC_ASSERT (sizeof (glib_typeof (*obj)) == sizeof (glib_typeof (*obj2)));

  MyObject *obj3 = g_atomic_pointer_get (&obj2);
  g_assert_true (obj3 == obj);

  MyObject *obj4 = NULL;
  g_atomic_pointer_set (&obj4, obj3);
  g_assert_true (obj4 == obj);

  MyObject *obj5 = NULL;
  g_atomic_pointer_compare_and_exchange (&obj5, NULL, obj4);
  g_assert_true (obj5 == obj);

  MyObject *obj6 = g_steal_pointer (&obj5);
  g_assert_true (obj6 == obj);

  g_clear_pointer (&obj6, g_rc_box_release);
  g_rc_box_release (obj);
}

static void
test_atomic_pointer_compare_and_exchange (void)
{
  const gchar *str1 = "str1";
  const gchar *str2 = "str2";
  const gchar *atomic_string = str1;

  g_test_message ("Test that g_atomic_pointer_compare_and_exchange() with a "
                  "non-void* pointer doesn’t have any compiler warnings in C++ mode");

  g_assert_true (g_atomic_pointer_compare_and_exchange (&atomic_string, str1, str2));
  g_assert_true (atomic_string == str2);
}

static void
test_atomic_pointer_compare_and_exchange_full (void)
{
  const gchar *str1 = "str1";
  const gchar *str2 = "str2";
  const gchar *atomic_string = str1;
  const gchar *old;

  g_test_message ("Test that g_atomic_pointer_compare_and_exchange_full() with a "
                  "non-void* pointer doesn’t have any compiler warnings in C++ mode");

  g_assert_true (g_atomic_pointer_compare_and_exchange_full (&atomic_string, str1, str2, &old));
  g_assert_true (atomic_string == str2);
  g_assert_true (old == str1);
}

static void
test_atomic_int_compare_and_exchange (void)
{
  gint atomic_int = 5;

  g_test_message ("Test that g_atomic_int_compare_and_exchange() doesn’t have "
                  "any compiler warnings in C++ mode");

  g_assert_true (g_atomic_int_compare_and_exchange (&atomic_int, 5, 50));
  g_assert_cmpint (atomic_int, ==, 50);
}

static void
test_atomic_int_compare_and_exchange_full (void)
{
  gint atomic_int = 5;
  gint old_value;

  g_test_message ("Test that g_atomic_int_compare_and_exchange_full() doesn’t have "
                  "any compiler warnings in C++ mode");

  g_assert_true (g_atomic_int_compare_and_exchange_full (&atomic_int, 5, 50, &old_value));
  g_assert_cmpint (atomic_int, ==, 50);
  g_assert_cmpint (old_value, ==, 5);
}

static void
test_atomic_pointer_exchange (void)
{
  const gchar *str1 = "str1";
  const gchar *str2 = "str2";
  const gchar *atomic_string = str1;

  g_test_message ("Test that g_atomic_pointer_exchange() with a "
                  "non-void* pointer doesn’t have any compiler warnings in C++ mode");

  g_assert_true (g_atomic_pointer_exchange (&atomic_string, str2) == str1);
  g_assert_true (atomic_string == str2);
}

static void
test_atomic_int_exchange (void)
{
  gint atomic_int = 5;

  g_test_message ("Test that g_atomic_int_compare_and_exchange() doesn’t have "
                  "any compiler warnings in C++ mode");

  g_assert_cmpint (g_atomic_int_exchange (&atomic_int, 50), ==, 5);
}

G_NO_INLINE
static gboolean
do_not_inline_this (void)
{
  return FALSE;
}

G_ALWAYS_INLINE
static inline gboolean
do_inline_this (void)
{
  return TRUE;
}

static void
test_inline_no_inline_macros (void)
{
  g_test_message ("Test that G_NO_INLINE and G_ALWAYS_INLINE functions "
                  "can be compiled with C++ compiler");

  g_assert_false (do_not_inline_this ());
  g_assert_true (do_inline_this ());
}

static void
clear_boolean_ptr (gboolean *val)
{
  *val = TRUE;
}

static void
test_clear_pointer (void)
{
  gboolean value = FALSE;
  gboolean *ptr = &value;

  g_assert_true (ptr == &value);
  g_assert_false (value);
  g_clear_pointer (&ptr, clear_boolean_ptr);
  g_assert_null (ptr);
  g_assert_true (value);
}

static void
test_steal_pointer (void)
{
  gpointer ptr = &ptr;

  g_assert_true (ptr == &ptr);
  g_assert_true (g_steal_pointer (&ptr) == &ptr);
  g_assert_null (ptr);
}

static void
test_str_equal (void)
{
  const char *str_a = "a";
  char *str_b = g_strdup ("b");
  char *str_null = g_strdup (NULL);
  gconstpointer str_a_ptr = str_a, str_b_ptr = str_b;
  const unsigned char *str_c = (const unsigned char *) "c";

  g_test_summary ("Test typechecking and casting of arguments to g_str_equal() macro in C++");
  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/2820");

  /* We don’t actually care what the comparisons do at runtime. What we’re
   * checking here is that the types don’t emit warnings at compile time. */
  g_assert_true (g_str_equal ("a", str_a));
  g_assert_false (g_str_equal ("a", str_b));
  g_assert_true (g_str_equal (str_a, str_a_ptr));
  g_assert_false (g_str_equal (str_a_ptr, str_b_ptr));
  g_assert_false (g_str_equal (str_c, str_b));
  g_assert_cmpstr (str_b, !=, str_null);

  g_free (str_b);
}

static void
test_strdup (void)
{
  gchar *str;

  g_assert_null ((g_strdup) (NULL));

  str = (g_strdup) ("C++ is cool too!");
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, "C++ is cool too!");
  g_free (str);
}

static void
test_strdup_macro (void)
{
  gchar *str;

  g_assert_null (g_strdup (NULL));

  str = g_strdup ("C++ is cool too!");
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, "C++ is cool too!");
  g_free (str);
}

static void
test_strdup_macro_qualified (void)
{
  gchar *str;

  g_assert_null (::g_strdup (NULL));

  str = ::g_strdup ("C++ is cool too!");
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, "C++ is cool too!");
  g_free (str);
}

static void
test_strdup_macro_nested_initializer (void)
{
  struct
  {
    char *p, *q;
  } strings = {
    g_strdup (NULL),
    g_strdup ("C++ is cool too!"),
  };

  g_assert_null (strings.p);
  g_assert_nonnull (strings.q);
  g_assert_cmpstr (strings.q, ==, "C++ is cool too!");
  g_free (strings.q);
}

static void
test_str_has_prefix (void)
{
  g_assert_true ((g_str_has_prefix) ("C++ is cool!", "C++"));
}

static void
test_str_has_prefix_macro (void)
{
  g_assert_true (g_str_has_prefix ("C++ is cool!", "C++"));
}

static void
test_str_has_suffix (void)
{
  g_assert_true ((g_str_has_suffix) ("C++ is cool!", "cool!"));
}

static void
test_str_has_suffix_macro (void)
{
  g_assert_true (g_str_has_suffix ("C++ is cool!", "cool!"));
}

static void
test_string_append (void)
{
  GString *string;
  char *tmp;
  int i;

  tmp = g_strdup ("more");

  /* append */
  string = g_string_new ("firsthalf");
  g_string_append (string, "last");
  (g_string_append) (string, "half");

  g_assert_cmpstr (string->str, ==, "firsthalflasthalf");

  i = 0;
  g_string_append (string, &tmp[i++]);
  (g_string_append) (string, &tmp[i++]);
  g_assert_true (i == 2);
  g_assert_cmpstr (string->str, ==, "firsthalflasthalfmoreore");

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*string != NULL*failed*");
  g_assert_null (g_string_append (NULL, NULL));
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*string != NULL*failed*");
  g_assert_null ((g_string_append) (NULL, NULL));
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*val != NULL*failed*");
  g_assert_true (g_string_append (string, NULL) == string);
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*val != NULL*failed*");
  g_assert_true ((g_string_append) (string, NULL) == string);
  g_test_assert_expected_messages ();

  g_string_free (string, TRUE);

  /* append_len */
  string = g_string_new ("firsthalf");
  g_string_append_len (string, "lasthalfjunkjunk", strlen ("last"));
  (g_string_append_len) (string, "halfjunkjunk", strlen ("half"));
  g_string_append_len (string, "more", -1);
  (g_string_append_len) (string, "ore", -1);

  g_assert_true (g_string_append_len (string, NULL, 0) == string);
  g_assert_true ((g_string_append_len) (string, NULL, 0) == string);

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*string != NULL*failed*");
  g_assert_null (g_string_append_len (NULL, NULL, -1));
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*string != NULL*failed*");
  g_assert_null ((g_string_append_len) (NULL, NULL, -1));
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*val != NULL*failed*");
  g_assert_true (g_string_append_len (string, NULL, -1) == string);
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*val != NULL*failed*");
  g_assert_true ((g_string_append_len) (string, NULL, -1) == string);
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*val != NULL*failed*");
  g_assert_true (g_string_append_len (string, NULL, 1) == string);
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*val != NULL*failed*");
  g_assert_true ((g_string_append_len) (string, NULL, 1) == string);
  g_test_assert_expected_messages ();

  g_assert_cmpstr (string->str, ==, "firsthalflasthalfmoreore");

  char c = 'A';
  g_string_append_c (string, c++);
  (g_string_append_c) (string, c++);
  g_assert_cmpstr (string->str, ==, "firsthalflasthalfmoreoreAB");

  i = string->len;
  g_string_truncate (string, --i);
  (g_string_truncate) (string, --i);
  g_assert_cmpstr (string->str, ==, "firsthalflasthalfmoreore");

  g_string_free (string, TRUE);
  g_free (tmp);
}

static void
test_string_free (void)
{
  GString *str;
  gchar *data;

  g_test_message ("Test that g_string_free() macro compiles and doesn’t "
                  "cause any compiler warnings in C++ mode");

  /* Test that g_string_free (_, TRUE) does not cause a warning if
   * its return value is unused. */
  str = g_string_new ("test");
  g_string_free (str, TRUE);

  /* Test that g_string_free (_, FALSE) does not emit a warning if
   * its return value is used. */
  str = g_string_new ("test");
  data = g_string_free (str, FALSE);
  g_free (data);

  /* Test that g_string_free () with an expression that is always FALSE
   * at runtime, but the compiler can't know it, does not cause any
   * warnings if its return value is unused. */
  str = g_string_new ("test");
  data = str->str;
  g_string_free (str, g_test_get_path ()[0] == 0);
  g_free (data);
}

#if G_CXX_STD_CHECK_VERSION(14)
#if !defined (_MSC_VER) || _MSC_VER >= 1910 /* VS2017 */
/* N3652 Extended Constexpr is not supported in VS2015
 * https://learn.microsoft.com/en-us/cpp/overview/visual-cpp-language-conformance
 */
static constexpr gboolean
g_likely_test_expr (void)
{
  if G_LIKELY (1 == 1)
    {
      return TRUE;
    }
  return FALSE;
}

static void
test_constexpr_var_init (void)
{
  g_test_message ("Test that G_LIKELY macro creates an initialized variable "
                  "for compatibility with constexpr");
  g_assert_true (g_likely_test_expr ());
}
#endif
#endif

#ifdef G_OS_WIN32
static void
test_clear_com (void)
{
  IWICImagingFactory *tmp;
  IWICImagingFactory *o =  NULL;

  CoInitialize (NULL);
  g_win32_clear_com (&o);
  g_assert_null (o);
  g_assert_true (SUCCEEDED (CoCreateInstance (CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (void **)&tmp)));
  g_assert_nonnull (tmp);
  tmp->QueryInterface (IID_IWICImagingFactory, (void **)&o); /* IWICImagingFactory::QueryInterface increments tmp's refcount */
  g_assert_nonnull (o);
  g_assert_cmpint (tmp->AddRef (), ==, 3); /* tmp's refcount incremented again */
  g_win32_clear_com (&o);  /* tmp's refcount gets decremented */
  g_assert_null (o);
  g_assert_cmpint (tmp->Release (), ==, 1);  /* tmp's refcount gets decremented, again */
  g_win32_clear_com (&tmp);
  g_assert_null (tmp);

  CoUninitialize ();
}
#endif

int
main (int argc, char *argv[])
{
#if G_CXX_STD_CHECK_VERSION (11)
  g_test_init (&argc, &argv, nullptr);
#else
  g_test_init (&argc, &argv, static_cast<void *>(NULL));
#endif

#ifdef _G_EXPECTED_CXX_STANDARD
  g_test_add_func ("/C++/check-standard-" _G_EXPECTED_CXX_STANDARD, test_cpp_standard);
#endif
  g_test_add_func ("/C++/typeof", test_typeof);
  g_test_add_func ("/C++/atomic-pointer-compare-and-exchange", test_atomic_pointer_compare_and_exchange);
  g_test_add_func ("/C++/atomic-pointer-compare-and-exchange-full", test_atomic_pointer_compare_and_exchange_full);
  g_test_add_func ("/C++/atomic-int-compare-and-exchange", test_atomic_int_compare_and_exchange);
  g_test_add_func ("/C++/atomic-int-compare-and-exchange-full", test_atomic_int_compare_and_exchange_full);
  g_test_add_func ("/C++/atomic-pointer-exchange", test_atomic_pointer_exchange);
  g_test_add_func ("/C++/atomic-int-exchange", test_atomic_int_exchange);
  g_test_add_func ("/C++/inlined-not-inlined-functions", test_inline_no_inline_macros);
  g_test_add_func ("/C++/clear-pointer", test_clear_pointer);
  g_test_add_func ("/C++/steal-pointer", test_steal_pointer);
  g_test_add_func ("/C++/str-equal", test_str_equal);
  g_test_add_func ("/C++/strdup", test_strdup);
  g_test_add_func ("/C++/strdup/macro", test_strdup_macro);
  g_test_add_func ("/C++/strdup/macro/qualified", test_strdup_macro_qualified);
  g_test_add_func ("/C++/strdup/macro/nested-initializer", test_strdup_macro_nested_initializer);
  g_test_add_func ("/C++/str-has-prefix", test_str_has_prefix);
  g_test_add_func ("/C++/str-has-prefix/macro", test_str_has_prefix_macro);
  g_test_add_func ("/C++/str-has-suffix", test_str_has_suffix);
  g_test_add_func ("/C++/str-has-suffix/macro", test_str_has_suffix_macro);
  g_test_add_func ("/C++/string-append", test_string_append);
  g_test_add_func ("/C++/string-free", test_string_free);

#if G_CXX_STD_CHECK_VERSION(14)
#if !defined (_MSC_VER) || _MSC_VER >= 1910 /* VS2017 */
  g_test_add_func ("/C++/constexpr-var-init", test_constexpr_var_init);
#endif
#endif

#ifdef G_OS_WIN32
  g_test_add_func ("/C++/test_clear_com", test_clear_com);
#endif
  return g_test_run ();
}
