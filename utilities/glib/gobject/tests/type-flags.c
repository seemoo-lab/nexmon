// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2021  Emmanuele Bassi

#include <glib-object.h>

typedef struct
{
  GTypeInterface g_iface;
} TestInterfaceInterface;

GType test_interface_get_type (void);
#define TEST_TYPE_INTERFACE test_interface_get_type ()
G_DEFINE_INTERFACE (TestInterface, test_interface, G_TYPE_INVALID)

static void
test_interface_default_init (TestInterfaceInterface *iface)
{
}

static void
test_type_flags_interface (void)
{
  g_assert_false (G_TYPE_IS_ABSTRACT (TEST_TYPE_INTERFACE));
  g_assert_false (g_type_test_flags (TEST_TYPE_INTERFACE, G_TYPE_FLAG_ABSTRACT));

  g_assert_false (G_TYPE_IS_CLASSED (TEST_TYPE_INTERFACE));
  g_assert_false (g_type_test_flags (TEST_TYPE_INTERFACE, G_TYPE_FLAG_CLASSED));

  g_assert_false (G_TYPE_IS_DEEP_DERIVABLE (TEST_TYPE_INTERFACE));
  g_assert_false (g_type_test_flags (TEST_TYPE_INTERFACE, G_TYPE_FLAG_DEEP_DERIVABLE));

  g_assert_true (G_TYPE_IS_DERIVABLE (TEST_TYPE_INTERFACE));
  g_assert_true (g_type_test_flags (TEST_TYPE_INTERFACE, G_TYPE_FLAG_DERIVABLE));

  g_assert_false (G_TYPE_IS_FINAL (TEST_TYPE_INTERFACE));
  g_assert_false (g_type_test_flags (TEST_TYPE_INTERFACE, G_TYPE_FLAG_FINAL));

  g_assert_false (G_TYPE_IS_INSTANTIATABLE (TEST_TYPE_INTERFACE));
  g_assert_false (g_type_test_flags (TEST_TYPE_INTERFACE, G_TYPE_FLAG_INSTANTIATABLE));
}

#define TEST_TYPE_FINAL (test_final_get_type())
G_DECLARE_FINAL_TYPE (TestFinal, test_final, TEST, FINAL, GObject)

struct _TestFinal
{
  GObject parent_instance;
};

struct _TestFinalClass
{
  GObjectClass parent_class;
};

G_DEFINE_FINAL_TYPE (TestFinal, test_final, G_TYPE_OBJECT)

static void
test_final_class_init (TestFinalClass *klass)
{
}

static void
test_final_init (TestFinal *self)
{
}

#define TEST_TYPE_FINAL2 (test_final2_get_type())
G_DECLARE_FINAL_TYPE (TestFinal2, test_final2, TEST, FINAL2, TestFinal)

struct _TestFinal2
{
  TestFinal parent_instance;
};

struct _TestFinal2Class
{
  TestFinalClass parent_class;
};

G_DEFINE_TYPE (TestFinal2, test_final2, TEST_TYPE_FINAL)

static void
test_final2_class_init (TestFinal2Class *klass)
{
}

static void
test_final2_init (TestFinal2 *self)
{
}

/* test_type_flags_final: Check that trying to derive from a final class
 * will result in a warning from the type system
 */
static void
test_type_flags_final (void)
{
  GType final2_type;

  g_assert_true (G_TYPE_IS_FINAL (TEST_TYPE_FINAL));
  g_assert_true (g_type_test_flags (TEST_TYPE_FINAL, G_TYPE_FLAG_FINAL));
  g_assert_true (G_TYPE_IS_CLASSED (TEST_TYPE_FINAL));
  g_assert_true (g_type_test_flags (TEST_TYPE_FINAL, G_TYPE_FLAG_CLASSED));
  g_assert_true (G_TYPE_IS_INSTANTIATABLE (TEST_TYPE_FINAL));
  g_assert_true (g_type_test_flags (TEST_TYPE_FINAL, G_TYPE_FLAG_INSTANTIATABLE));
  g_assert_true (g_type_test_flags (TEST_TYPE_FINAL,
                                    G_TYPE_FLAG_FINAL |
                                    G_TYPE_FLAG_CLASSED |
                                    G_TYPE_FLAG_INSTANTIATABLE));
  g_assert_false (g_type_test_flags (TEST_TYPE_FINAL,
                                     G_TYPE_FLAG_FINAL |
                                     G_TYPE_FLAG_CLASSED |
                                     G_TYPE_FLAG_DEPRECATED |
                                     G_TYPE_FLAG_INSTANTIATABLE));

  /* This is the message we print out when registering the type */
  g_test_expect_message ("GLib-GObject", G_LOG_LEVEL_CRITICAL,
                         "*cannot derive*");

  /* This is the message when we fail to return from the GOnce init
   * block within the test_final2_get_type() function
   */
  g_test_expect_message ("GLib", G_LOG_LEVEL_CRITICAL,
                         "*g_once_init_leave_pointer: assertion*");

  final2_type = TEST_TYPE_FINAL2;
  g_assert_true (final2_type == G_TYPE_INVALID);

  g_test_assert_expected_messages ();
}

#define TEST_TYPE_DEPRECATED (test_deprecated_get_type())
G_DECLARE_FINAL_TYPE (TestDeprecated, test_deprecated, TEST, DEPRECATED, GObject)

struct _TestDeprecated
{
  GObject parent_instance;
};

struct _TestDeprecatedClass
{
  GObjectClass parent_class;
};

G_DEFINE_TYPE_EXTENDED (TestDeprecated, test_deprecated, G_TYPE_OBJECT, G_TYPE_FLAG_FINAL | G_TYPE_FLAG_DEPRECATED, {})

static void
test_deprecated_class_init (TestDeprecatedClass *klass)
{
}

static void
test_deprecated_init (TestDeprecated *self)
{
}

static void
test_type_flags_final_instance_check (void)
{
  TestFinal *final;

  final = g_object_new (TEST_TYPE_FINAL, NULL);
  g_assert_true (g_type_check_instance_is_a ((GTypeInstance *) final,
                                              TEST_TYPE_FINAL));
  g_assert_false (g_type_check_instance_is_a ((GTypeInstance *) final,
                                              TEST_TYPE_DEPRECATED));
  g_assert_true (g_type_check_instance_is_a ((GTypeInstance *) final,
                                              G_TYPE_OBJECT));
  g_assert_false (g_type_check_instance_is_a ((GTypeInstance *) final,
                                              G_TYPE_INVALID));

  g_clear_object (&final);
}

static void
test_type_flags_deprecated (void)
{
  GType deprecated_type;
  GObject *deprecated_object = NULL;

  g_test_summary ("Test that trying to instantiate a deprecated type results in a warning.");

  /* This is the message we print out when registering the type */
  g_test_expect_message ("GLib-GObject", G_LOG_LEVEL_WARNING,
                         "*The type TestDeprecated is deprecated and shouldn’t be used any more*");

  /* The type itself should not be considered invalid. */
  deprecated_type = TEST_TYPE_DEPRECATED;
  g_assert_false (deprecated_type == G_TYPE_INVALID);
  g_assert_true (G_TYPE_IS_DEPRECATED (deprecated_type));

  g_assert_true (G_TYPE_IS_FINAL (deprecated_type));
  g_assert_true (g_type_test_flags (deprecated_type, G_TYPE_FLAG_FINAL));

  g_assert_true (g_type_test_flags (deprecated_type,
                                    G_TYPE_FLAG_DEPRECATED |
                                    G_TYPE_FLAG_CLASSED |
                                    G_TYPE_FLAG_FINAL |
                                    G_TYPE_FLAG_INSTANTIATABLE));
  g_assert_false (g_type_test_flags (deprecated_type,
                                     G_TYPE_FLAG_DEPRECATED |
                                     G_TYPE_FLAG_CLASSED |
                                     G_TYPE_FLAG_FINAL |
                                     G_TYPE_FLAG_ABSTRACT |
                                     G_TYPE_FLAG_INSTANTIATABLE));

  /* Instantiating it should work, but emit a warning. */
  deprecated_object = g_object_new (deprecated_type, NULL);
  g_assert_nonnull (deprecated_object);

  g_test_assert_expected_messages ();

  g_object_unref (deprecated_object);

  /* Instantiating it again should not emit a second warning. */
  deprecated_object = g_object_new (deprecated_type, NULL);
  g_assert_nonnull (deprecated_object);

  g_assert_true (g_type_check_instance_is_a ((GTypeInstance *) deprecated_object,
                                              TEST_TYPE_DEPRECATED));
  g_assert_true (g_type_check_instance_is_a ((GTypeInstance *) deprecated_object,
                                              G_TYPE_OBJECT));
  g_assert_false (g_type_check_instance_is_a ((GTypeInstance *) deprecated_object,
                                              TEST_TYPE_FINAL));
  g_assert_false (g_type_check_instance_is_a ((GTypeInstance *) deprecated_object,
                                              G_TYPE_INVALID));

  g_test_assert_expected_messages ();

  g_object_unref (deprecated_object);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_setenv ("G_ENABLE_DIAGNOSTIC", "1", TRUE);

  g_test_add_func ("/type/flags/interface", test_type_flags_interface);
  g_test_add_func ("/type/flags/final", test_type_flags_final);
  g_test_add_func ("/type/flags/final/instance-check", test_type_flags_final_instance_check);
  g_test_add_func ("/type/flags/deprecated", test_type_flags_deprecated);

  return g_test_run ();
}
