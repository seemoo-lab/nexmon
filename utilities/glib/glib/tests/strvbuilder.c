/*
 * Copyright © 2020 Canonical Ltd.
 * Copyright © 2021 Alexandros Theodotou
 *
 * SPDX-License-Identifier: LicenseRef-old-glib-tests
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

#include "glib.h"

static void
test_strvbuilder_empty (void)
{
  GStrvBuilder *builder;
  GStrv result;

  builder = g_strv_builder_new ();
  result = g_strv_builder_end (builder);
  g_assert_nonnull (result);
  g_assert_cmpint (g_strv_length (result), ==, 0);

  g_strfreev (result);
  g_strv_builder_unref (builder);
}

static void
test_strvbuilder_add (void)
{
  GStrvBuilder *builder;
  GStrv result;
  const gchar *expected[] = { "one", "two", "three", NULL };

  builder = g_strv_builder_new ();
  g_strv_builder_add (builder, "one");
  g_strv_builder_add (builder, "two");
  g_strv_builder_add (builder, "three");
  result = g_strv_builder_end (builder);
  g_assert_nonnull (result);
  g_assert_true (g_strv_equal ((const gchar *const *) result, expected));

  g_strfreev (result);
  g_strv_builder_unref (builder);
}

static void
test_strvbuilder_addv (void)
{
  GStrvBuilder *builder;
  GStrv result;
  const gchar *expected[] = { "one", "two", "three", NULL };

  builder = g_strv_builder_new ();
  g_strv_builder_addv (builder, expected);
  result = g_strv_builder_end (builder);
  g_assert_nonnull (result);
  g_assert_cmpstrv ((const gchar *const *) result, expected);

  g_strfreev (result);
  g_strv_builder_unref (builder);
}

static void
test_strvbuilder_add_many (void)
{
  GStrvBuilder *builder;
  GStrv result;
  const gchar *expected[] = { "one", "two", "three", NULL };

  builder = g_strv_builder_new ();
  g_strv_builder_add_many (builder, "one", "two", "three", NULL);
  result = g_strv_builder_end (builder);
  g_assert_nonnull (result);
  g_assert_cmpstrv ((const gchar *const *) result, expected);

  g_strfreev (result);
  g_strv_builder_unref (builder);
}

static void
test_strvbuilder_take (void)
{
  GStrvBuilder *builder;
  GStrv result;
  const gchar *expected[] = { "one", "two", "three", NULL };

  builder = g_strv_builder_new ();
  g_strv_builder_take (builder, g_strdup ("one"));
  g_strv_builder_add (builder, "two");
  g_strv_builder_take (builder, g_strdup ("three"));
  result = g_strv_builder_end (builder);
  g_assert_nonnull (result);
  g_assert_true (g_strv_equal ((const gchar *const *) result, expected));

  g_strfreev (result);
  g_strv_builder_unref (builder);
}

static void
test_strvbuilder_ref (void)
{
  GStrvBuilder *builder;

  builder = g_strv_builder_new ();
  g_strv_builder_ref (builder);
  g_strv_builder_unref (builder);
  g_strv_builder_unref (builder);
}

static void
test_strvbuilder_unref_to_strv (void)
{
  GStrvBuilder *builder = g_strv_builder_new ();
  GStrv result;

  g_strv_builder_add_many (builder, "hello", "world", NULL);
  result = g_strv_builder_unref_to_strv (builder);

  g_assert_true (g_strv_equal ((const char * const *) result,
                               (const char *[]) {
                                 "hello",
                                 "world",
                                 NULL,
                               }));

  g_strfreev (result);

  builder = g_strv_builder_new ();
  result = g_strv_builder_unref_to_strv (builder);
  g_assert_null (result[0]);
  g_strfreev (result);
}

int
main (int argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/strvbuilder/empty", test_strvbuilder_empty);
  g_test_add_func ("/strvbuilder/add", test_strvbuilder_add);
  g_test_add_func ("/strvbuilder/addv", test_strvbuilder_addv);
  g_test_add_func ("/strvbuilder/add_many", test_strvbuilder_add_many);
  g_test_add_func ("/strvbuilder/take", test_strvbuilder_take);
  g_test_add_func ("/strvbuilder/ref", test_strvbuilder_ref);
  g_test_add_func ("/strvbuilder/unref_to_strv", test_strvbuilder_unref_to_strv);

  return g_test_run ();
}
