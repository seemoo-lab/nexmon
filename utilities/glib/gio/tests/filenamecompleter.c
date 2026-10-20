/*
 * Copyright (C) 2025 Khalid Abu Shawarib.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Author: Khalid Abu Shawarib <kas@gnome.org>
 */

#include <gio/gio.h>

static void
create_files (const GStrv hier)
{
  g_mkdir_with_parents (g_get_home_dir (), 0700);

  for (guint i = 0; hier[i] != NULL; i++)
    {
      GFile *file = g_file_new_build_filename (g_get_home_dir (), hier[i], NULL);
      gboolean is_directory = g_str_has_suffix (hier[i], G_DIR_SEPARATOR_S);

      if (is_directory)
        g_file_make_directory (file, NULL, NULL);
      else
        {
          GFileOutputStream *stream = g_file_create (file, G_FILE_CREATE_NONE, NULL, NULL);

          g_object_unref (stream);
        }

      g_assert_true (g_file_query_exists (file, NULL));

      g_object_unref (file);
    }
}

static GStrv
prefix_filenames (const gchar *const strings[],
                  const gchar *prefix)
{
  GStrvBuilder *builder = g_strv_builder_new ();

  for (guint i = 0; strings[i] != NULL; i++)
    g_strv_builder_take (builder, g_build_filename (prefix, strings[i], NULL));

  return g_strv_builder_unref_to_strv (builder);
}

static int
strcmp_data (const void *a,
             const void *b,
             void       *user_data)
{
  return strcmp (*((const char * const *) a), *((const char * const *) b));
}

#define assert_cmpstrv_unsorted(a1, a2) \
  G_STMT_START { \
    const char **_a1 = (const char **) (a1); \
    const char **_a2 = (const char **) (a2); \
    g_sort_array (_a1, g_strv_length ((char **) _a1), sizeof (*_a1), strcmp_data, NULL); \
    g_sort_array (_a2, g_strv_length ((char **) _a2), sizeof (*_a2), strcmp_data, NULL); \
    g_assert_cmpstrv (_a1, _a2); \
  } G_STMT_END

typedef struct
{
  const gchar *string;
  const gchar *all_completion_suffix;
  const gchar *dirs_completion_suffix;
  const gchar *const all_completions[5];
  const gchar *const dirs_completions[5];
} FilenameCompleterTestCase;

static void
run_test_cases (const FilenameCompleterTestCase test_cases[],
                guint len)
{
  const gchar *base_path = g_get_home_dir ();

  for (guint i = 0; i < len; i++)
    {
      GFilenameCompleter *all_completer = g_filename_completer_new ();
      GFilenameCompleter *dirs_completer = g_filename_completer_new ();
      gchar *completion_suffix;
      GStrv all_results_completions, all_expected_completions;
      GStrv dirs_results_completions, dirs_expected_completions;
      gchar *path_to_complete = g_build_filename (base_path, test_cases[i].string, NULL);
      GMainLoop *loop = g_main_loop_new (NULL, FALSE);

      /* dirs_only = FALSE */
      g_signal_connect_swapped (all_completer, "got-completion-data",
                                G_CALLBACK (g_main_loop_quit), loop);
      g_filename_completer_set_dirs_only (all_completer, FALSE);
      g_strfreev (g_filename_completer_get_completions (all_completer, path_to_complete));

      g_main_loop_run (loop);

      all_expected_completions = prefix_filenames (test_cases[i].all_completions, base_path);
      all_results_completions = g_filename_completer_get_completions (all_completer, path_to_complete);
      assert_cmpstrv_unsorted (all_expected_completions, all_results_completions);
      g_strfreev (all_expected_completions);
      g_strfreev (all_results_completions);

      completion_suffix = g_filename_completer_get_completion_suffix (all_completer,
                                                                      path_to_complete);
      g_assert_cmpstr (test_cases[i].all_completion_suffix, ==, completion_suffix);
      g_free (completion_suffix);

      /* dirs_only = TRUE */
      g_signal_connect_swapped (dirs_completer, "got-completion-data",
                                G_CALLBACK (g_main_loop_quit), loop);
      g_filename_completer_set_dirs_only (dirs_completer, TRUE);
      g_strfreev (g_filename_completer_get_completions (dirs_completer, path_to_complete));

      g_main_loop_run (loop);

      dirs_expected_completions = prefix_filenames (test_cases[i].dirs_completions, base_path);
      dirs_results_completions = g_filename_completer_get_completions (dirs_completer, path_to_complete);
      assert_cmpstrv_unsorted (dirs_expected_completions, dirs_results_completions);

      g_strfreev (dirs_expected_completions);
      g_strfreev (dirs_results_completions);

      completion_suffix = g_filename_completer_get_completion_suffix (dirs_completer,
                                                                      path_to_complete);
      g_assert_cmpstr (test_cases[i].dirs_completion_suffix, ==, completion_suffix);
      g_free (completion_suffix);

      g_object_unref (all_completer);
      g_object_unref (dirs_completer);
      g_free (path_to_complete);
      g_main_loop_unref (loop);
    }
}

static void
test_completions (void)
{
  const GStrv files_hier = (char *[]){
    "file_1",
    "file_2",
    "folder_1" G_DIR_SEPARATOR_S,
    "folder_2" G_DIR_SEPARATOR_S,
    NULL
  };
  const FilenameCompleterTestCase test_cases[] = {
    {
        "f",
        "",
        "older_",
        { "file_1",
          "file_2",
          "folder_1" G_DIR_SEPARATOR_S,
          "folder_2" G_DIR_SEPARATOR_S,
          NULL },
        { "folder_1" G_DIR_SEPARATOR_S,
          "folder_2" G_DIR_SEPARATOR_S,
          NULL },
    },

    {
        "fi",
        "le_",
        NULL,
        { "file_1",
          "file_2",
          NULL },
        { NULL },
    },

    {
        "fo",
        "lder_",
        "lder_",
        { "folder_1" G_DIR_SEPARATOR_S,
          "folder_2" G_DIR_SEPARATOR_S,
          NULL },
        { "folder_1" G_DIR_SEPARATOR_S,
          "folder_2" G_DIR_SEPARATOR_S,
          NULL },
    },
  };

  create_files (files_hier);

  run_test_cases (test_cases, G_N_ELEMENTS (test_cases));
}

int
main (int argc, char *argv[])
{
  g_setenv ("LC_ALL", "C", TRUE);

  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add_func ("/filenamecompleter/basic", test_completions);

  return g_test_run ();
}
