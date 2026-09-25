#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <locale.h>
#include <stdlib.h>

static void
print (const gchar *str)
{
  g_print ("%s\n", str ? str : "nil");
}

static void
print_app_list (GList *list)
{
  while (list)
    {
      GAppInfo *info = list->data;
      print (g_app_info_get_id (info));
      list = g_list_delete_link (list, list);
      g_object_unref (info);
    }
}

static void
quit (gpointer user_data)
{
  g_print ("appinfo database changed.\n");
  exit (0);
}

int
main (int argc, char **argv)
{
  setlocale (LC_ALL, "");

  if (argv[1] == NULL || g_str_equal (argv[1], "--help"))
    g_print ("Usage:\n"
      "  apps --help\n"
      "  apps COMMAND [COMMAND_OPTIONS]\n"
      "\n"
      "COMMANDS:\n"
      "  list\n"
      "  search [--should-show-only] TEXT_TO_SEARCH\n"
      "  implementations INTERFACE_NAME\n"
      "  show-info DESKTOP_FILE\n"
      "  default-for-type MIME_TYPE\n"
      "  recommended-for-type MIME_TYPE\n"
      "  all-for-type MIME_TYPE\n"
      "  fallback-for-type MIME_TYPE\n"
      "  should-show DESKTOP_FILE\n"
      "  monitor\n"
      "\n"
      "Examples:\n"
      "  apps search --should-show-only ter\n"
      "  apps show-info org.gnome.Nautilus.desktop\n"
      "  apps default-for-type image/png\n"
      "\n");
  else if (g_str_equal (argv[1], "list"))
    {
      GList *all, *i;

      all = g_app_info_get_all ();
      for (i = all; i; i = i->next)
        g_print ("%s%s", g_app_info_get_id (i->data), i->next ? " " : "\n");
      g_list_free_full (all, g_object_unref);
    }
  else if (g_str_equal (argv[1], "search"))
    {
      gchar ***results;
      gboolean should_show_only;
      gint i, j;

      should_show_only = argc > 3 && g_str_equal (argv[2], "--should-show-only");
      results = g_desktop_app_info_search (argv[ should_show_only ? 3 : 2 ]);
      for (i = 0; results[i]; i++)
        {
          for (j = 0; results[i][j]; j++)
            {
              if (should_show_only)
                {
                  GDesktopAppInfo *info;
                  gboolean should_show;

                  info = g_desktop_app_info_new (results[i][j]);
                  if (info)
                    {
                      should_show = g_app_info_should_show (G_APP_INFO (info));
                      g_object_unref (info);
                      if (!should_show)
                        continue;
                    }
                }
              g_print ("%s%s", j ? " " : "", results[i][j]);
            }
          g_print ("\n");
          g_strfreev (results[i]);
        }
      g_free (results);
    }
  else if (g_str_equal (argv[1], "implementations"))
    {
      GList *results;

      results = g_desktop_app_info_get_implementations (argv[2]);
      print_app_list (results);
    }
  else if (g_str_equal (argv[1], "show-info"))
    {
      GAppInfo *info;

      info = (GAppInfo *) g_desktop_app_info_new (argv[2]);
      if (info)
        {
          print (g_app_info_get_id (info));
          print (g_app_info_get_name (info));
          print (g_app_info_get_display_name (info));
          print (g_app_info_get_description (info));
          g_object_unref (info);
        }
    }
  else if (g_str_equal (argv[1], "default-for-type"))
    {
      GAppInfo *info;

      info = g_app_info_get_default_for_type (argv[2], FALSE);

      if (info)
        {
          print (g_app_info_get_id (info));
          g_object_unref (info);
        }
    }
  else if (g_str_equal (argv[1], "recommended-for-type"))
    {
      GList *list;

      list = g_app_info_get_recommended_for_type (argv[2]);
      print_app_list (list);
    }
  else if (g_str_equal (argv[1], "all-for-type"))
    {
      GList *list;

      list = g_app_info_get_all_for_type (argv[2]);
      print_app_list (list);
    }

  else if (g_str_equal (argv[1], "fallback-for-type"))
    {
      GList *list;

      list = g_app_info_get_fallback_for_type (argv[2]);
      print_app_list (list);
    }

  else if (g_str_equal (argv[1], "should-show"))
    {
      GAppInfo *info;

      info = (GAppInfo *) g_desktop_app_info_new (argv[2]);
      if (info)
        {
          g_print ("%s\n", g_app_info_should_show (info) ? "true" : "false");
          g_object_unref (info);
        }
    }

  else if (g_str_equal (argv[1], "monitor"))
    {
      GAppInfoMonitor *monitor;
      GAppInfo *info;

      monitor = g_app_info_monitor_get ();

      info = (GAppInfo *) g_desktop_app_info_new ("this-desktop-file-does-not-exist");
      g_assert (!info);

      g_signal_connect (monitor, "changed", G_CALLBACK (quit), NULL);

      while (1)
        g_main_context_iteration (NULL, TRUE);
    }

  return 0;
}
