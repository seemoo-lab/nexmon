#include <glib.h>

#include "glib-private.h"

static void
test_overwrite (void)
{
  GError *error, *dest, *src;

  if (!g_test_undefined ())
    return;

  error = g_error_new_literal (G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY, "bla");

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                         "*set over the top*");
  g_set_error_literal (&error, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE, "bla");
  g_test_assert_expected_messages ();

  g_assert_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY);
  g_error_free (error);


  error = g_error_new_literal (G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY, "bla");

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                         "*set over the top*");
  g_set_error (&error, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE, "bla");
  g_test_assert_expected_messages ();

  g_assert_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY);
  g_error_free (error);


  dest = g_error_new_literal (G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY, "bla");
  src = g_error_new_literal (G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE, "bla");

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                         "*set over the top*");
  g_propagate_error (&dest, src);
  g_test_assert_expected_messages ();

  g_assert_error (dest, G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY);
  g_error_free (dest);
}

static void
test_prefix (void)
{
  GError *error;
  GError *dest, *src;

  error = NULL;
  g_prefix_error (&error, "foo %d %s: ", 1, "two");
  g_assert (error == NULL);

  error = g_error_new_literal (G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY, "bla");
  g_prefix_error (&error, "foo %d %s: ", 1, "two");
  g_assert_cmpstr (error->message, ==, "foo 1 two: bla");
  g_error_free (error);

  dest = NULL;
  src = g_error_new_literal (G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY, "bla");
  g_propagate_prefixed_error (&dest, src, "foo %d %s: ", 1, "two");
  g_assert_cmpstr (dest->message, ==, "foo 1 two: bla");
  g_error_free (dest);

  src = g_error_new_literal (G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY, "bla");
  g_propagate_prefixed_error (NULL, src, "foo %d %s: ", 1, "two");
}

static void
test_prefix_literal (void)
{
  GError *error = NULL;

  g_prefix_error_literal (NULL, "foo: ");

  g_prefix_error_literal (&error, "foo: ");
  g_assert_null (error);

  error = NULL;
  g_prefix_error_literal (&error, "foo: ");
  g_assert_null (error);

  error = g_error_new_literal (G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY, "bla");
  g_assert_nonnull (error);
  g_prefix_error_literal (&error, "foo: ");
  g_assert_cmpstr (error->message, ==, "foo: bla");
  g_error_free (error);
}

static void
test_literal (void)
{
  GError *error;

  error = NULL;
  g_set_error_literal (&error, G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY, "%s %d %x");
  g_assert_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY);
  g_assert_cmpstr (error->message, ==, "%s %d %x");
  g_error_free (error);
}

static void
test_copy (void)
{
  GError *error;
  GError *copy;

  error = NULL;
  g_set_error_literal (&error, G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY, "%s %d %x");
  copy = g_error_copy (error);

  g_assert_error (copy, G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY);
  g_assert_cmpstr (copy->message, ==, "%s %d %x");

  g_error_free (error);
  g_error_free (copy);
}

static void
test_new_valist_invalid_va (gpointer dummy,
                         ...)
{
#if defined(__linux__) && defined(__GLIBC__)
  /* Only worth testing this on Linux with glibc; if other platforms regress on
   * this legacy behaviour, we don’t care. In particular, calling
   * g_error_new_valist() with a %NULL format will crash on FreeBSD as its
   * implementation of vasprintf() is less forgiving than Linux’s. That’s
   * fine: it’s a programmer error in either case. */

  g_test_summary ("Test that g_error_new_valist() rejects invalid input");

  if (!g_test_undefined ())
    {
      g_test_skip ("Not testing response to programmer error");
      return;
    }

    {
      GError *error = NULL;
      va_list ap;

      va_start (ap, dummy);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

      g_test_expect_message (G_LOG_DOMAIN,
                             G_LOG_LEVEL_CRITICAL,
                             "*g_error_new_valist: assertion 'format != NULL' failed*");
      error = g_error_new_valist (G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY, NULL, ap);
      g_test_assert_expected_messages ();
      g_assert_null (error);

#pragma GCC diagnostic pop

      va_end (ap);
    }

    {
      GError *error = NULL, *error_copy = NULL;
      va_list ap;

      va_start (ap, dummy);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"

      g_test_expect_message (G_LOG_DOMAIN,
                             G_LOG_LEVEL_WARNING,
                             "*g_error_new_valist: runtime check failed*");
      error = g_error_new_valist (0, G_MARKUP_ERROR_EMPTY, "Message", ap);
      g_test_assert_expected_messages ();
      g_assert_nonnull (error);

#pragma GCC diagnostic pop

      g_test_expect_message (G_LOG_DOMAIN,
                             G_LOG_LEVEL_WARNING,
                             "*g_error_copy: runtime check failed*");
      error_copy = g_error_copy (error);
      g_test_assert_expected_messages ();
      g_assert_nonnull (error_copy);

      g_clear_error (&error);
      g_clear_error (&error_copy);

      va_end (ap);
    }
#else  /* if !__linux__ || !__GLIBC__ */
  g_test_skip ("g_error_new_valist() programmer error handling is only relevant on Linux with glibc");
#endif /* !__linux__ || ! __GLIBC__ */
}

static void
test_new_valist_invalid (void)
{
  /* We need a wrapper function so we can build a va_list */
  test_new_valist_invalid_va (NULL);
}

static void
test_matches (void)
{
  GError *error = NULL;

  g_test_summary ("Test g_error_matches()");

  error = g_error_new (G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY, "Oh no!");

  g_assert_true (g_error_matches (error, G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY));
  g_assert_false (g_error_matches (NULL, G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY));
  g_assert_false (g_error_matches (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE));  /* same numeric value as G_MARKUP_ERROR_EMPTY */
  g_assert_false (g_error_matches (error, G_OPTION_ERROR, G_OPTION_ERROR_FAILED));  /* different numeric value from G_MARKUP_ERROR_EMPTY */
  g_assert_false (g_error_matches (error, G_MARKUP_ERROR, G_MARKUP_ERROR_BAD_UTF8));

  g_error_free (error);
}

static void
test_clear (void)
{
  GError *error = NULL;

  g_test_summary ("Test g_error_clear()");

  g_clear_error (&error);
  g_assert_null (error);

  g_clear_error (NULL);

  error = g_error_new (G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY, "Oh no!");
  g_clear_error (&error);
  g_assert_null (error);
}

typedef struct
{
  int init_called;
  int copy_called;
  int free_called;
} TestErrorCheck;

static TestErrorCheck *init_check;

GQuark test_error_quark (void);
#define TEST_ERROR (test_error_quark ())

typedef struct
{
  int foo;
  TestErrorCheck *check;
} TestErrorPrivate;

static void
test_error_private_init (TestErrorPrivate *priv)
{
  g_assert_nonnull (priv);

  priv->foo = 13;
  /* If that triggers, it's test bug.
   */
  g_assert_nonnull (init_check);
  /* Using global init_check, because error->check is still nil at
   * this point.
   */
  init_check->init_called++;
}

static void
test_error_private_copy (const TestErrorPrivate *src_priv,
                         TestErrorPrivate       *dest_priv)
{
  g_assert_nonnull (src_priv);
  g_assert_nonnull (dest_priv);

  dest_priv->foo = src_priv->foo;
  dest_priv->check = src_priv->check;

  g_assert_nonnull (dest_priv->check);
  dest_priv->check->copy_called++;
}

static void
test_error_private_clear (TestErrorPrivate *priv)
{
  g_assert_nonnull (priv->check);
  priv->check->free_called++;
}

G_DEFINE_EXTENDED_ERROR (TestError, test_error)

static TestErrorPrivate *
fill_test_error (GError *error, TestErrorCheck *check)
{
  TestErrorPrivate *test_error = test_error_get_private (error);

  g_assert_nonnull (test_error);
  test_error->check = check;

  return test_error;
}

static void
test_extended (void)
{
  TestErrorCheck check = { 0, 0, 0 };
  GError *error;
  TestErrorPrivate *test_priv;
  GError *copy_error;
  TestErrorPrivate *copy_test_priv;

  init_check = &check;
  error = g_error_new_literal (TEST_ERROR, 0, "foo");
  test_priv = fill_test_error (error, &check);

  g_assert_cmpint (check.init_called, ==, 1);
  g_assert_cmpint (check.copy_called, ==, 0);
  g_assert_cmpint (check.free_called, ==, 0);

  g_assert_cmpuint (error->domain, ==, TEST_ERROR);
  g_assert_cmpint (test_priv->foo, ==, 13);

  copy_error = g_error_copy (error);
  g_assert_cmpint (check.init_called, ==, 2);
  g_assert_cmpint (check.copy_called, ==, 1);
  g_assert_cmpint (check.free_called, ==, 0);

  g_assert_cmpuint (error->domain, ==, copy_error->domain);
  g_assert_cmpint (error->code, ==, copy_error->code);
  g_assert_cmpstr (error->message, ==, copy_error->message);

  copy_test_priv = test_error_get_private (copy_error);
  g_assert_nonnull (copy_test_priv);
  g_assert_cmpint (test_priv->foo, ==, copy_test_priv->foo);

  g_error_free (error);
  g_error_free (copy_error);

  g_assert_cmpint (check.init_called, ==, 2);
  g_assert_cmpint (check.copy_called, ==, 1);
  g_assert_cmpint (check.free_called, ==, 2);
}

static void
test_extended_duplicate (void)
{
  g_test_summary ("Test that registering a duplicate extended error domain doesn’t work");

  if (!g_test_subprocess ())
    {
      /* Spawn a subprocess and expect it to fail. */
      g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
      g_test_trap_assert_failed ();
      g_test_trap_assert_stderr ("*CRITICAL*Attempted to register an extended error domain for TestError more than once*");
    }
  else
    {
      GQuark q;
      guint i;

      for (i = 0; i < 2; i++)
        {
          q = g_error_domain_register_static ("TestError",
                                              sizeof (TestErrorPrivate),
                                              g_error_with_test_error_private_init,
                                              g_error_with_test_error_private_copy,
                                              g_error_with_test_error_private_clear);
          g_assert_cmpstr (g_quark_to_string (q), ==, "TestError");
        }
    }
}

typedef struct
{
  int dummy;
} TestErrorNonStaticPrivate;

static void test_error_non_static_private_init (GError *error) {}
static void test_error_non_static_private_copy (const GError *src_error, GError *dest_error) {}
static void test_error_non_static_private_clear (GError *error) {}

static void
test_extended_non_static (void)
{
  gchar *domain_name = g_strdup ("TestErrorNonStatic");
  GQuark q;
  GError *error = NULL;

  g_test_summary ("Test registering an extended error domain with a non-static name");

  q = g_error_domain_register (domain_name,
                               sizeof (TestErrorNonStaticPrivate),
                               test_error_non_static_private_init,
                               test_error_non_static_private_copy,
                               test_error_non_static_private_clear);
  g_free (domain_name);

  error = g_error_new (q, 0, "Test error: %s", "hello");
  g_assert_true (g_error_matches (error, q, 0));
  g_assert_cmpstr (g_quark_to_string (q), ==, "TestErrorNonStatic");
  g_error_free (error);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/error/overwrite", test_overwrite);
  g_test_add_func ("/error/prefix", test_prefix);
  g_test_add_func ("/error/prefix-literal", test_prefix_literal);
  g_test_add_func ("/error/literal", test_literal);
  g_test_add_func ("/error/copy", test_copy);
  g_test_add_func ("/error/matches", test_matches);
  g_test_add_func ("/error/clear", test_clear);
  g_test_add_func ("/error/new-valist/invalid", test_new_valist_invalid);
  g_test_add_func ("/error/extended", test_extended);
  g_test_add_func ("/error/extended/duplicate", test_extended_duplicate);
  g_test_add_func ("/error/extended/non-static", test_extended_non_static);

  return g_test_run ();
}
