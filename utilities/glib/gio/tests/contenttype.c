#include <gio/gio.h>
#include <string.h>

#define g_assert_content_type_equals(s1, s2) 			\
  do { 								\
    const char *__s1 = (s1), *__s2 = (s2); 			\
    if (g_content_type_equals (__s1, __s2)) ;		 	\
    else 							\
      g_assertion_message_cmpstr (G_LOG_DOMAIN, 		\
                                  __FILE__, __LINE__, 		\
                                  G_STRFUNC, 			\
                                  #s1 " == " #s2, 		\
                                  __s1, " == ", __s2); 		\
  } while (0)

static void
test_guess (void)
{
  gchar *res;
  gchar *expected;
  gchar *existing_directory;
  gboolean uncertain;
  guchar data[] =
    "[Desktop Entry]\n"
    "Type=Application\n"
    "Name=appinfo-test\n"
    "Exec=./appinfo-test --option\n";

#ifdef G_OS_WIN32
  existing_directory = (gchar *) g_getenv ("SYSTEMROOT");

  if (existing_directory)
    existing_directory = g_strdup_printf ("%s/", existing_directory);
#else
  existing_directory = g_strdup ("/etc/");
#endif

  res = g_content_type_guess (existing_directory, NULL, 0, &uncertain);
  g_free (existing_directory);
  expected = g_content_type_from_mime_type ("inode/directory");
  g_assert_content_type_equals (expected, res);
  g_assert (uncertain);
  g_free (res);
  g_free (expected);

  res = g_content_type_guess ("foo.txt", NULL, 0, &uncertain);
  expected = g_content_type_from_mime_type ("text/plain");
  g_assert_content_type_equals (expected, res);
  g_free (res);
  g_free (expected);

  res = g_content_type_guess ("foo.desktop", data, sizeof (data) - 1, &uncertain);
  expected = g_content_type_from_mime_type ("application/x-desktop");
  g_assert_content_type_equals (expected, res);
  g_assert (!uncertain);
  g_free (res);
  g_free (expected);

  res = g_content_type_guess ("foo.txt", data, sizeof (data) - 1, &uncertain);
  expected = g_content_type_from_mime_type ("text/plain");
  g_assert_content_type_equals (expected, res);
  g_assert (!uncertain);
  g_free (res);
  g_free (expected);

  res = g_content_type_guess ("foo", data, sizeof (data) - 1, &uncertain);
  expected = g_content_type_from_mime_type ("text/plain");
  g_assert_content_type_equals (expected, res);
  g_assert (!uncertain);
  g_free (res);
  g_free (expected);

  res = g_content_type_guess (NULL, data, sizeof (data) - 1, &uncertain);
  expected = g_content_type_from_mime_type ("application/x-desktop");
  g_assert_content_type_equals (expected, res);
  g_assert (!uncertain);
  g_free (res);
  g_free (expected);

  /* this is potentially ambiguous: it does not match the PO template format,
   * but looks like text so it can't be Powerpoint */
  res = g_content_type_guess ("test.pot", (guchar *)"ABC abc", 7, &uncertain);
  expected = g_content_type_from_mime_type ("text/x-gettext-translation-template");
  g_assert_content_type_equals (expected, res);
  g_assert (!uncertain);
  g_free (res);
  g_free (expected);

  res = g_content_type_guess ("test.pot", (guchar *)"msgid \"", 7, &uncertain);
  expected = g_content_type_from_mime_type ("text/x-gettext-translation-template");
  g_assert_content_type_equals (expected, res);
  g_assert (!uncertain);
  g_free (res);
  g_free (expected);

  res = g_content_type_guess ("test.pot", (guchar *)"\xCF\xD0\xE0\x11", 4, &uncertain);
  expected = g_content_type_from_mime_type ("application/vnd.ms-powerpoint");
  g_assert_content_type_equals (expected, res);
  /* we cannot reliably detect binary powerpoint files as long as there is no
   * defined MIME magic, so do not check uncertain here
   */
  g_free (res);
  g_free (expected);

  res = g_content_type_guess ("test.otf", (guchar *)"OTTO", 4, &uncertain);
  expected = g_content_type_from_mime_type ("application/x-font-otf");
  g_assert_content_type_equals (expected, res);
  g_assert (!uncertain);
  g_free (res);
  g_free (expected);

  res = g_content_type_guess (NULL, (guchar *)"%!PS-Adobe-2.0 EPSF-1.2", 23, &uncertain);
  expected = g_content_type_from_mime_type ("image/x-eps");
  g_assert_content_type_equals (expected, res);
  g_assert (!uncertain);
  g_free (res);
  g_free (expected);
}

static void
test_unknown (void)
{
  gchar *unknown;
  gchar *str;

  unknown = g_content_type_from_mime_type ("application/octet-stream");
  g_assert (g_content_type_is_unknown (unknown));
  str = g_content_type_get_mime_type (unknown);
  g_assert_cmpstr (str, ==, "application/octet-stream");
  g_free (str);
  g_free (unknown);
}

static void
test_subtype (void)
{
  gchar *plain;
  gchar *xml;

  plain = g_content_type_from_mime_type ("text/plain");
  xml = g_content_type_from_mime_type ("application/xml");

  g_assert (g_content_type_is_a (xml, plain));

  g_free (plain);
  g_free (xml);
}

static gint
find_mime (gconstpointer a, gconstpointer b)
{
  if (g_content_type_equals (a, b))
    return 0;
  return 1;
}

static void
test_list (void)
{
  GList *types;
  gchar *plain;
  gchar *xml;

  plain = g_content_type_from_mime_type ("text/plain");
  xml = g_content_type_from_mime_type ("application/xml");

  types = g_content_types_get_registered ();

  g_assert (g_list_length (types) > 1);

  /* just check that some types are in the list */
  g_assert (g_list_find_custom (types, plain, find_mime) != NULL);
  g_assert (g_list_find_custom (types, xml, find_mime) != NULL);

  g_list_free_full (types, g_free);

  g_free (plain);
  g_free (xml);
}

static void
test_executable (void)
{
  gchar *type;

  type = g_content_type_from_mime_type ("application/x-executable");
  g_assert (g_content_type_can_be_executable (type));
  g_free (type);

  type = g_content_type_from_mime_type ("text/plain");
  g_assert (g_content_type_can_be_executable (type));
  g_free (type);

  type = g_content_type_from_mime_type ("image/png");
  g_assert (!g_content_type_can_be_executable (type));
  g_free (type);
}

static void
test_description (void)
{
  gchar *type;
  gchar *desc;

  type = g_content_type_from_mime_type ("text/plain");
  desc = g_content_type_get_description (type);
  g_assert (desc != NULL);

  g_free (desc);
  g_free (type);
}

static void
test_icon (void)
{
  gchar *type;
  GIcon *icon;

  type = g_content_type_from_mime_type ("text/plain");
  icon = g_content_type_get_icon (type);
  g_assert (G_IS_ICON (icon));
  if (G_IS_THEMED_ICON (icon))
    {
      const gchar *const *names;

      names = g_themed_icon_get_names (G_THEMED_ICON (icon));
      g_assert (g_strv_contains (names, "text-plain"));
      g_assert (g_strv_contains (names, "text-x-generic"));
    }
  g_object_unref (icon);
  g_free (type);

  type = g_content_type_from_mime_type ("application/rtf");
  icon = g_content_type_get_icon (type);
  g_assert (G_IS_ICON (icon));
  if (G_IS_THEMED_ICON (icon))
    {
      const gchar *const *names;

      names = g_themed_icon_get_names (G_THEMED_ICON (icon));
      g_assert (g_strv_contains (names, "application-rtf"));
      g_assert (g_strv_contains (names, "x-office-document"));
    }
  g_object_unref (icon);
  g_free (type);
}

static void
test_symbolic_icon (void)
{
#ifndef G_OS_WIN32
  gchar *type;
  GIcon *icon;

  type = g_content_type_from_mime_type ("text/plain");
  icon = g_content_type_get_symbolic_icon (type);
  g_assert (G_IS_ICON (icon));
  if (G_IS_THEMED_ICON (icon))
    {
      const gchar *const *names;

      names = g_themed_icon_get_names (G_THEMED_ICON (icon));
      g_assert (g_strv_contains (names, "text-plain-symbolic"));
      g_assert (g_strv_contains (names, "text-x-generic-symbolic"));
      g_assert (g_strv_contains (names, "text-plain"));
      g_assert (g_strv_contains (names, "text-x-generic"));
    }
  g_object_unref (icon);
  g_free (type);

  type = g_content_type_from_mime_type ("application/rtf");
  icon = g_content_type_get_symbolic_icon (type);
  g_assert (G_IS_ICON (icon));
  if (G_IS_THEMED_ICON (icon))
    {
      const gchar *const *names;

      names = g_themed_icon_get_names (G_THEMED_ICON (icon));
      g_assert (g_strv_contains (names, "application-rtf-symbolic"));
      g_assert (g_strv_contains (names, "x-office-document-symbolic"));
      g_assert (g_strv_contains (names, "application-rtf"));
      g_assert (g_strv_contains (names, "x-office-document"));
    }
  g_object_unref (icon);
  g_free (type);
#endif
}

static void
test_tree (void)
{
  const gchar *tests[] = {
    "x-content/image-dcf",
    "x-content/unix-software",
    "x-content/win32-software"
  };
  const gchar *path;
  GFile *file;
  gchar **types;
  gint i;

  for (i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      path = g_test_get_filename (G_TEST_DIST, tests[i], NULL);
      file = g_file_new_for_path (path);
      types = g_content_type_guess_for_tree (file);
      g_assert_content_type_equals (types[0], tests[i]);
      g_strfreev (types);
      g_object_unref (file);
   }
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/contenttype/guess", test_guess);
  g_test_add_func ("/contenttype/unknown", test_unknown);
  g_test_add_func ("/contenttype/subtype", test_subtype);
  g_test_add_func ("/contenttype/list", test_list);
  g_test_add_func ("/contenttype/executable", test_executable);
  g_test_add_func ("/contenttype/description", test_description);
  g_test_add_func ("/contenttype/icon", test_icon);
  g_test_add_func ("/contenttype/symbolic-icon", test_symbolic_icon);
  g_test_add_func ("/contenttype/tree", test_tree);

  return g_test_run ();
}
