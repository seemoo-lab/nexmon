#include <glib.h>

static void
test_dir_read (void)
{
  GDir *dir;
  GError *error;
  gchar *first;
  const gchar *name;

  error = NULL;
  dir = g_dir_open (".", 0, &error);
  g_assert_no_error (error);

  first = NULL;
  while ((name = g_dir_read_name (dir)) != NULL)
    {
      if (first == NULL)
        first = g_strdup (name);
      g_assert_cmpstr (name, !=, ".");
      g_assert_cmpstr (name, !=, "..");
    }

  g_dir_rewind (dir);
  g_assert_cmpstr (g_dir_read_name (dir), ==, first);

  g_free (first);
  g_dir_close (dir);
}

static void
test_dir_nonexisting (void)
{
  GDir *dir;
  GError *error = NULL;
  char *path = NULL;

  path = g_build_filename (g_get_tmp_dir (), "does-not-exist", NULL);
  dir = g_dir_open (path, 0, &error);
  g_assert_null (dir);
  g_assert_error (error, G_FILE_ERROR, G_FILE_ERROR_NOENT);
  g_error_free (error);
  g_free (path);
}

static void
test_dir_refcounting (void)
{
  GDir *dir;
  GError *local_error = NULL;

  g_test_summary ("Test refcounting interactions with g_dir_close()");

  /* Try keeping the `GDir` struct alive after closing it. */
  dir = g_dir_open (".", 0, &local_error);
  g_assert_no_error (local_error);

  g_dir_ref (dir);
  g_dir_close (dir);
  g_dir_unref (dir);

  /* Test that dropping the last ref closes it. Any leak here should be caught
   * when the test is run under valgrind. */
  dir = g_dir_open (".", 0, &local_error);
  g_assert_no_error (local_error);
  g_dir_unref (dir);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/dir/read", test_dir_read);
  g_test_add_func ("/dir/nonexisting", test_dir_nonexisting);
  g_test_add_func ("/dir/refcounting", test_dir_refcounting);

  return g_test_run ();
}
