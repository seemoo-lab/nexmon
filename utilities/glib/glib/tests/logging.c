#include "config.h"

#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#define G_LOG_USE_STRUCTURED 1
#include <glib.h>

#ifdef G_OS_WIN32
#define LINE_END "\r\n"
#else
#define LINE_END "\n"
#endif

/* Test g_warn macros */
static void
test_warnings (void)
{
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                         "*test_warnings*should not be reached*");
  g_warn_if_reached ();
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                         "*test_warnings*runtime check failed*");
  g_warn_if_fail (FALSE);
  g_test_assert_expected_messages ();
}

static guint log_count = 0;

static void
log_handler (const gchar    *log_domain,
             GLogLevelFlags  log_level,
             const gchar    *message,
             gpointer        user_data)
{
  g_assert_cmpstr (log_domain, ==, "bu");
  g_assert_cmpint (log_level, ==, G_LOG_LEVEL_INFO);

  log_count++;
}

/* test that custom log handlers only get called for
 * their domain and level
 */
static void
test_set_handler (void)
{
  guint id;

  id = g_log_set_handler ("bu", G_LOG_LEVEL_INFO, log_handler, NULL);

  g_log ("bu", G_LOG_LEVEL_DEBUG, "message");
  g_log ("ba", G_LOG_LEVEL_DEBUG, "message");
  g_log ("bu", G_LOG_LEVEL_INFO, "message");
  g_log ("ba", G_LOG_LEVEL_INFO, "message");

  g_assert_cmpint (log_count, ==, 1);

  g_log_remove_handler ("bu", id);
}

static void
test_default_handler_error (void)
{
  g_log_set_default_handler (g_log_default_handler, NULL);
  g_error ("message1");
  exit (0);
}

static void
test_default_handler_error_stderr (void)
{
  g_log_writer_default_set_use_stderr (FALSE);
  g_log_set_default_handler (g_log_default_handler, NULL);
  g_error ("message1");
  exit (0);
}

static void
test_default_handler_critical_stderr (void)
{
  g_log_writer_default_set_use_stderr (TRUE);
  g_log_set_default_handler (g_log_default_handler, NULL);
  g_critical ("message2");
  exit (0);
}

static void
test_default_handler_critical (void)
{
  g_log_writer_default_set_use_stderr (FALSE);
  g_log_set_default_handler (g_log_default_handler, NULL);
  g_critical ("message2");
  exit (0);
}

static void
test_default_handler_warning_stderr (void)
{
  g_log_writer_default_set_use_stderr (TRUE);
  g_log_set_default_handler (g_log_default_handler, NULL);
  g_warning ("message3");
  exit (0);
}

static void
test_default_handler_warning (void)
{
  g_log_writer_default_set_use_stderr (FALSE);
  g_log_set_default_handler (g_log_default_handler, NULL);
  g_warning ("message3");
  exit (0);
}

static void
test_default_handler_message (void)
{
  g_log_writer_default_set_use_stderr (FALSE);
  g_log_set_default_handler (g_log_default_handler, NULL);
  g_message ("message4");
  exit (0);
}

static void
test_default_handler_message_stderr (void)
{
  g_log_writer_default_set_use_stderr (TRUE);
  g_log_set_default_handler (g_log_default_handler, NULL);
  g_message ("message4");
  exit (0);
}

static void
test_default_handler_info (void)
{
  g_log_writer_default_set_use_stderr (FALSE);
  g_log_set_default_handler (g_log_default_handler, NULL);
  g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "message5");
  exit (0);
}

static void
test_default_handler_info_stderr (void)
{
  g_log_writer_default_set_use_stderr (TRUE);
  g_log_set_default_handler (g_log_default_handler, NULL);
  g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "message5");
  exit (0);
}

static void
test_default_handler_bar_info (void)
{
  g_log_writer_default_set_use_stderr (FALSE);
  g_log_set_default_handler (g_log_default_handler, NULL);

  g_assert_cmpstr (g_getenv ("G_MESSAGES_DEBUG"), ==, "foo bar baz");

  g_log ("bar", G_LOG_LEVEL_INFO, "message5");
  exit (0);
}

static void
test_default_handler_baz_debug (void)
{
  g_log_writer_default_set_use_stderr (FALSE);
  g_log_set_default_handler (g_log_default_handler, NULL);

  g_assert_cmpstr (g_getenv ("G_MESSAGES_DEBUG"), ==, "foo bar baz");

  g_log ("baz", G_LOG_LEVEL_DEBUG, "message6");
  exit (0);
}

static void
test_default_handler_debug (void)
{
  g_log_writer_default_set_use_stderr (FALSE);
  g_log_set_default_handler (g_log_default_handler, NULL);

  g_assert_cmpstr (g_getenv ("G_MESSAGES_DEBUG"), ==, "all");

  g_log ("foo", G_LOG_LEVEL_DEBUG, "6");
  g_log ("bar", G_LOG_LEVEL_DEBUG, "6");
  g_log ("baz", G_LOG_LEVEL_DEBUG, "6");

  exit (0);
}

static void
test_default_handler_debug_stderr (void)
{
  g_log_writer_default_set_use_stderr (TRUE);
  g_log_set_default_handler (g_log_default_handler, NULL);

  g_assert_cmpstr (g_getenv ("G_MESSAGES_DEBUG"), ==, "all");

  g_log ("foo", G_LOG_LEVEL_DEBUG, "6");
  g_log ("bar", G_LOG_LEVEL_DEBUG, "6");
  g_log ("baz", G_LOG_LEVEL_DEBUG, "6");

  exit (0);
}

static void
test_default_handler_would_drop_env_systemd (void)
{
  g_assert_cmpstr (g_getenv ("DEBUG_INVOCATION"), ==, "1");

  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "bar"));
}

static void
test_default_handler_would_drop_env5 (void)
{
  g_assert_cmpstr (g_getenv ("G_MESSAGES_DEBUG"), ==, "foobar");

  g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "foo"));
  g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "bar"));
}

static void
test_default_handler_would_drop_env4 (void)
{
  g_assert_cmpstr (g_getenv ("G_MESSAGES_DEBUG"), ==, "all");

  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_ERROR, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_CRITICAL, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_WARNING, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_MESSAGE, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_INFO, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "foo"));
  g_assert_false (g_log_writer_default_would_drop (1<<G_LOG_LEVEL_USER_SHIFT, "foo"));
}

static void
test_default_handler_would_drop_env3 (void)
{
  g_assert_cmpstr (g_getenv ("G_MESSAGES_DEBUG"), ==, "foo bar");

  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_ERROR, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_CRITICAL, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_WARNING, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_MESSAGE, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_INFO, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "foo"));
  g_assert_false (g_log_writer_default_would_drop (1<<G_LOG_LEVEL_USER_SHIFT, "foo"));
}

static void
test_default_handler_would_drop_env2 (void)
{
  g_assert_cmpstr (g_getenv ("G_MESSAGES_DEBUG"), ==, "  bar    baz ");

  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_ERROR, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_CRITICAL, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_WARNING, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_MESSAGE, "foo"));
  g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_INFO, "foo"));
  g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "foo"));
  g_assert_false (g_log_writer_default_would_drop (1<<G_LOG_LEVEL_USER_SHIFT, "foo"));
}

static void
test_default_handler_would_drop_env1 (void)
{
  g_assert_null (g_getenv ("G_MESSAGES_DEBUG"));

  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_ERROR, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_CRITICAL, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_WARNING, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_MESSAGE, "foo"));
  g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_INFO, "foo"));
  g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "foo"));
  g_assert_false (g_log_writer_default_would_drop (1<<G_LOG_LEVEL_USER_SHIFT, "foo"));
}

static void
test_default_handler_would_drop (void)
{
  g_assert_null (g_getenv ("G_MESSAGES_DEBUG"));

  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_ERROR, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_CRITICAL, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_WARNING, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_MESSAGE, "foo"));
  g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_INFO, "foo"));
  g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "foo"));
  g_assert_false (g_log_writer_default_would_drop (1<<G_LOG_LEVEL_USER_SHIFT, "foo"));

  /* Expected to have no effect */
  g_setenv ("G_MESSAGES_DEBUG", "all", TRUE);

  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_ERROR, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_CRITICAL, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_WARNING, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_MESSAGE, "foo"));
  g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_INFO, "foo"));
  g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "foo"));
  g_assert_false (g_log_writer_default_would_drop (1<<G_LOG_LEVEL_USER_SHIFT, "foo"));

  {
    const gchar *domains[] = { "all", NULL };
    g_log_writer_default_set_debug_domains (domains);

    g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_ERROR, "foo"));
    g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_CRITICAL, "foo"));
    g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_WARNING, "foo"));
    g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_MESSAGE, "foo"));
    g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_INFO, "foo"));
    g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "foo"));
    g_assert_false (g_log_writer_default_would_drop (1<<G_LOG_LEVEL_USER_SHIFT, "foo"));
  }

  {
    const gchar *domains[] = { "foobar", NULL };
    g_log_writer_default_set_debug_domains (domains);

    g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "foo"));
    g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "bar"));
  }

  {
    const gchar *domains[] = { "foobar", "bar", NULL };
    g_log_writer_default_set_debug_domains (domains);

    g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "foo"));
    g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "bar"));
  }

  {
    const gchar *domains[] = { "foobar", "bar", "barfoo", NULL };
    g_log_writer_default_set_debug_domains (domains);

    g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "foo"));
    g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "bar"));
  }

  {
    const gchar *domains[] = { "", NULL };
    g_log_writer_default_set_debug_domains (domains);

    g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "foo"));
    g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "bar"));
  }

  {
    const gchar *domains[] = { "foobar", "bar", "foo", "barfoo", NULL };
    g_log_writer_default_set_debug_domains (domains);

    g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "foo"));
    g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "bar"));
    g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "baz"));
  }

  {
    const gchar *domains[] = { "foo", "bar", "baz", NULL };
    g_log_writer_default_set_debug_domains (domains);

    g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "foo"));
    g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "bar"));
    g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "baz"));
  }


  {
    const gchar *domains[] = { "foo", NULL };
    g_log_writer_default_set_debug_domains (domains);

    g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "foo"));
    g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "bar"));
    g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "foobarbaz"));
    g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "barfoobaz"));
    g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "barbazfoo"));
  }

  {
    const gchar *domains[] = {NULL};
    g_log_writer_default_set_debug_domains (domains);
  
    g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "foo"));
    g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "bar"));
  }

  g_log_writer_default_set_debug_domains (NULL);
  g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "foo"));
  g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "bar"));

  exit (0);
}

static const gchar *
test_would_drop_robustness_random_domain (void)
{
  static const gchar *domains[] = { "foo", "bar", "baz", NULL };
  return domains[g_random_int_range (0, G_N_ELEMENTS (domains))];
}

static gboolean test_would_drop_robustness_stopping;

static gpointer
test_would_drop_robustness_thread (gpointer data)
{
  while (!g_atomic_int_get (&test_would_drop_robustness_stopping))
    {
      gsize i;
      const gchar *domains[4] = { 0 };

      for (i = 0; i < G_N_ELEMENTS (domains) - 1; i++)
        domains[i] = test_would_drop_robustness_random_domain ();

      domains[G_N_ELEMENTS (domains) - 1] = 0;
    
      g_log_writer_default_set_debug_domains (domains);
    }
  return NULL;
}

static void
test_default_handler_would_drop_robustness (void)
{
  GThread *threads[2];
  gsize i;
  guint counter = 1024 * 128;
  g_log_writer_default_set_debug_domains (NULL);

  for (i = 0; i < G_N_ELEMENTS (threads); i++)
    threads[i] = g_thread_new (NULL, test_would_drop_robustness_thread, NULL);

  while (counter-- > 0)
    g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, test_would_drop_robustness_random_domain ());

  g_atomic_int_set (&test_would_drop_robustness_stopping, TRUE);
  for (i = 0; i < G_N_ELEMENTS (threads); i++)
    g_thread_join (threads[i]);
}

static void
test_default_handler_0x400 (void)
{
  g_log_writer_default_set_use_stderr (FALSE);
  g_log_set_default_handler (g_log_default_handler, NULL);
  g_log (G_LOG_DOMAIN, 1<<10, "message7");
  exit (0);
}

static void
test_default_handler_structured_logging_non_nul_terminated_strings (void)
{
  g_log_writer_default_set_use_stderr (FALSE);
  g_log_set_default_handler (g_log_default_handler, NULL);
  g_assert_cmpstr (g_getenv ("G_MESSAGES_DEBUG"), ==, "foo");

  const gchar domain_1[] = {'f', 'o', 'o' };
  const gchar domain_2[] = { 'b', 'a', 'r' };
  const gchar message_1[] = { 'b', 'a', 'z' };
  const gchar message_2[] = { 'b', 'l', 'a' };
  const GLogField fields[] = {
    { "GLIB_DOMAIN", domain_1, sizeof (domain_1) },
    { "MESSAGE", message_1, sizeof (message_1) },
  };
  const GLogField other_fields[] = {
    { "GLIB_DOMAIN", domain_2, sizeof (domain_2) },
    { "MESSAGE", message_2, sizeof (message_2) },
  };

  g_log_structured_array (G_LOG_LEVEL_DEBUG, fields, G_N_ELEMENTS (fields));
  g_log_structured_array (G_LOG_LEVEL_DEBUG, other_fields, G_N_ELEMENTS (other_fields));

  exit (0);
}

/* Helper wrapper around g_test_trap_subprocess_with_envp() which sets the
 * logging-related environment variables. `NULL` will unset a variable. */
static void
test_trap_subprocess_with_logging_envp (const char *test_path,
                                        const char *g_messages_debug,
                                        const char *debug_invocation)
{
  char **envp = g_get_environ ();

  if (g_messages_debug != NULL)
    envp = g_environ_setenv (g_steal_pointer (&envp), "G_MESSAGES_DEBUG", g_messages_debug, TRUE);
  else
    envp = g_environ_unsetenv (g_steal_pointer (&envp), "G_MESSAGES_DEBUG");

  if (debug_invocation != NULL)
    envp = g_environ_setenv (g_steal_pointer (&envp), "DEBUG_INVOCATION", debug_invocation, TRUE);
  else
    envp = g_environ_unsetenv (g_steal_pointer (&envp), "DEBUG_INVOCATION");

  g_test_trap_subprocess_with_envp (test_path, (const char * const *) envp, 0, G_TEST_SUBPROCESS_DEFAULT);

  g_strfreev (envp);
}

static void
test_default_handler (void)
{
  test_trap_subprocess_with_logging_envp ("/logging/default-handler/subprocess/error",
                                          NULL, NULL);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*ERROR*message1*");

  test_trap_subprocess_with_logging_envp ("/logging/default-handler/subprocess/error-stderr",
                                          NULL, NULL);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*ERROR*message1*");

  test_trap_subprocess_with_logging_envp ("/logging/default-handler/subprocess/critical",
                                          NULL, NULL);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*CRITICAL*message2*");

  test_trap_subprocess_with_logging_envp ("/logging/default-handler/subprocess/critical-stderr",
                                          NULL, NULL);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*CRITICAL*message2*");

  test_trap_subprocess_with_logging_envp ("/logging/default-handler/subprocess/warning",
                                          NULL, NULL);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*WARNING*message3*");

  test_trap_subprocess_with_logging_envp ("/logging/default-handler/subprocess/warning-stderr",
                                          NULL, NULL);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*WARNING*message3*");

  test_trap_subprocess_with_logging_envp ("/logging/default-handler/subprocess/message",
                                          NULL, NULL);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stderr ("*Message*message4*");

  test_trap_subprocess_with_logging_envp ("/logging/default-handler/subprocess/message-stderr",
                                          NULL, NULL);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stderr ("*Message*message4*");

  test_trap_subprocess_with_logging_envp ("/logging/default-handler/subprocess/info",
                                          NULL, NULL);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stdout_unmatched ("*INFO*message5*");

  test_trap_subprocess_with_logging_envp ("/logging/default-handler/subprocess/info-stderr",
                                          NULL, NULL);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stderr_unmatched ("*INFO*message5*");

  test_trap_subprocess_with_logging_envp ("/logging/default-handler/subprocess/bar-info",
                                          "foo bar baz", NULL);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stdout ("*INFO*message5*");

  test_trap_subprocess_with_logging_envp ("/logging/default-handler/subprocess/baz-debug",
                                          "foo bar baz", NULL);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stdout ("*DEBUG*message6*");

  test_trap_subprocess_with_logging_envp ("/logging/default-handler/subprocess/debug",
                                          "all", NULL);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stdout ("*DEBUG*6*6*6*");

  test_trap_subprocess_with_logging_envp ("/logging/default-handler/subprocess/debug-stderr",
                                          "all", NULL);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stdout_unmatched ("DEBUG");
  g_test_trap_assert_stderr ("*DEBUG*6*6*6*");

  test_trap_subprocess_with_logging_envp ("/logging/default-handler/subprocess/0x400",
                                          NULL, NULL);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stdout ("*LOG-0x400*message7*");

  test_trap_subprocess_with_logging_envp ("/logging/default-handler/subprocess/would-drop",
                                          NULL, NULL);
  g_test_trap_assert_passed ();

  test_trap_subprocess_with_logging_envp ("/logging/default-handler/subprocess/would-drop-env1",
                                          NULL, NULL);
  g_test_trap_assert_passed ();

  test_trap_subprocess_with_logging_envp ("/logging/default-handler/subprocess/would-drop-env2",
                                          "  bar    baz ", NULL);
  g_test_trap_assert_passed ();

  test_trap_subprocess_with_logging_envp ("/logging/default-handler/subprocess/would-drop-env3",
                                          "foo bar", NULL);
  g_test_trap_assert_passed ();

  test_trap_subprocess_with_logging_envp ("/logging/default-handler/subprocess/would-drop-env4",
                                          "all", NULL);
  g_test_trap_assert_passed ();

  test_trap_subprocess_with_logging_envp ("/logging/default-handler/subprocess/would-drop-env5",
                                          "foobar", NULL);
  g_test_trap_assert_passed ();

  test_trap_subprocess_with_logging_envp ("/logging/default-handler/subprocess/would-drop-env-systemd",
                                          NULL, "1");
  g_test_trap_assert_passed ();

  test_trap_subprocess_with_logging_envp ("/logging/default-handler/subprocess/would-drop-robustness",
                                          NULL, NULL);
  g_test_trap_assert_passed ();

  test_trap_subprocess_with_logging_envp ("/logging/default-handler/subprocess/structured-logging-non-null-terminated-strings",
                                          "foo", NULL);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stdout_unmatched ("*bar*");
  g_test_trap_assert_stdout_unmatched ("*bla*");
  g_test_trap_assert_stdout ("*foo-DEBUG*baz*");
}

static void
test_journald_handler (void)
{
  /* We can’t require that the journal exists on the test system. But if it
   * does, we can check that the writer doesn’t crash. */
  const GLogField fields[] = {
    { "MESSAGE", "This is a test message.", -1 },
    { "MESSAGE_ID", "7187d27ad7f84351b76b7612eef52cd6", -1 },
    { "MY_APPLICATION_CUSTOM_FIELD", "some debug string", -1 },
  };

  g_log_writer_journald (G_LOG_LEVEL_DEBUG, fields, G_N_ELEMENTS (fields), NULL);
}

static void
test_fatal_log_mask (void)
{
  if (g_test_subprocess ())
    {
      g_log_set_fatal_mask ("bu", G_LOG_LEVEL_INFO);
      g_assert_null (g_getenv ("G_MESSAGES_DEBUG"));
      g_log ("bu", G_LOG_LEVEL_INFO, "fatal");
      return;
    }
  test_trap_subprocess_with_logging_envp (NULL, NULL, NULL);
  g_test_trap_assert_failed ();
  /* G_LOG_LEVEL_INFO isn't printed by default */
  g_test_trap_assert_stdout_unmatched ("*fatal*");
}

static void
test_always_fatal (void)
{
  GLogLevelFlags log_level;

  log_level = G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING;
  g_log_set_always_fatal (log_level);

  g_assert_cmpint (g_log_get_always_fatal (), ==, log_level | G_LOG_LEVEL_ERROR);
}

static gint my_print_count = 0;
static void
my_print_handler (const gchar *text)
{
  my_print_count++;
}

static void
test_print_handler (void)
{
  GPrintFunc old_print_handler;

  old_print_handler = g_set_print_handler (my_print_handler);
  g_assert_nonnull (old_print_handler);

  my_print_count = 0;
  g_print ("bu ba");
  g_assert_cmpint (my_print_count, ==, 1);

  if (g_test_subprocess ())
    {
      g_set_print_handler (NULL);
      old_print_handler ("default handler\n");
      g_print ("bu ba\n");
      return;
    }

  g_set_print_handler (old_print_handler);
  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_stdout ("*default handler" LINE_END "*");
  g_test_trap_assert_stdout ("*bu ba" LINE_END "*");
  g_test_trap_assert_stdout_unmatched ("*# default handler" LINE_END "*");
  g_test_trap_assert_stdout_unmatched ("*# bu ba" LINE_END "*");
  g_test_trap_has_passed ();
}

static void
test_printerr_handler (void)
{
  GPrintFunc old_printerr_handler;

  old_printerr_handler = g_set_printerr_handler (my_print_handler);
  g_assert_nonnull (old_printerr_handler);

  my_print_count = 0;
  g_printerr ("bu ba");
  g_assert_cmpint (my_print_count, ==, 1);

  if (g_test_subprocess ())
    {
      g_set_printerr_handler (NULL);
      old_printerr_handler ("default handler\n");
      g_printerr ("bu ba\n");
      return;
    }

  g_set_printerr_handler (old_printerr_handler);
  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_stderr ("*default handler" LINE_END "*");
  g_test_trap_assert_stderr ("*bu ba" LINE_END "*");
  g_test_trap_has_passed ();
}

static char *fail_str = "foo";
static char *log_str = "bar";

static gboolean
good_failure_handler (const gchar    *log_domain,
                      GLogLevelFlags  log_level,
                      const gchar    *msg,
                      gpointer        user_data)
{
  g_test_message ("The Good Fail Message Handler\n");
  g_assert ((char *)user_data != log_str);
  g_assert ((char *)user_data == fail_str);

  return FALSE;
}

static gboolean
bad_failure_handler (const gchar    *log_domain,
                     GLogLevelFlags  log_level,
                     const gchar    *msg,
                     gpointer        user_data)
{
  g_test_message ("The Bad Fail Message Handler\n");
  g_assert ((char *)user_data == log_str);
  g_assert ((char *)user_data != fail_str);

  return FALSE;
}

static void
test_handler (const gchar    *log_domain,
              GLogLevelFlags  log_level,
              const gchar    *msg,
              gpointer        user_data)
{
  g_test_message ("The Log Message Handler\n");
  g_assert ((char *)user_data != fail_str);
  g_assert ((char *)user_data == log_str);
}

static void
bug653052 (void)
{
  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=653052");

  g_test_log_set_fatal_handler (good_failure_handler, fail_str);
  g_log_set_default_handler (test_handler, log_str);

  g_return_if_fail (0);

  g_test_log_set_fatal_handler (bad_failure_handler, fail_str);
  g_log_set_default_handler (test_handler, log_str);

  g_return_if_fail (0);
}

static void
test_gibberish (void)
{
  if (g_test_subprocess ())
    {
      g_warning ("bla bla \236\237\190");
      return;
    }
  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*bla bla \\x9e\\x9f\\u000190*");
}

static GLogWriterOutput
null_log_writer (GLogLevelFlags   log_level,
                 const GLogField *fields,
                 gsize            n_fields,
                 gpointer         user_data)
{
  log_count++;
  return G_LOG_WRITER_HANDLED;
}

typedef struct {
  const GLogField *fields;
  gsize n_fields;
} ExpectedMessage;

static gboolean
compare_field (const GLogField *f1, const GLogField *f2)
{
  if (strcmp (f1->key, f2->key) != 0)
    return FALSE;
  if (f1->length != f2->length)
    return FALSE;

  if (f1->length == -1)
    return strcmp (f1->value, f2->value) == 0;
  else
    return memcmp (f1->value, f2->value, f1->length) == 0;
}

static gboolean
compare_fields (const GLogField *f1, gsize n1, const GLogField *f2, gsize n2)
{
  gsize i, j;

  for (i = 0; i < n1; i++)
    {
      for (j = 0; j < n2; j++)
        {
          if (compare_field (&f1[i], &f2[j]))
            break;
        }
      if (j == n2)
        return FALSE;
    }

  return TRUE;
}

static GSList *expected_messages = NULL;
static const guchar binary_field[] = {1, 2, 3, 4, 5};


static GLogWriterOutput
expect_log_writer (GLogLevelFlags   log_level,
                   const GLogField *fields,
                   gsize            n_fields,
                   gpointer         user_data)
{
  ExpectedMessage *expected = expected_messages->data;

  if (compare_fields (fields, n_fields, expected->fields, expected->n_fields))
    {
      expected_messages = g_slist_delete_link (expected_messages, expected_messages);
    }
  else if ((log_level & G_LOG_LEVEL_DEBUG) != G_LOG_LEVEL_DEBUG)
    {
      char *str;

      str = g_log_writer_format_fields (log_level, fields, n_fields, FALSE);
      g_test_fail_printf ("Unexpected message: %s", str);
      g_free (str);
    }

  return G_LOG_WRITER_HANDLED;
}

static void
test_structured_logging_no_state (void)
{
  /* Test has to run in a subprocess as it calls g_log_set_writer_func(), which
   * can only be called once per process. */
  if (g_test_subprocess ())
    {
      gpointer some_pointer = GUINT_TO_POINTER (0x100);
      guint some_integer = 123;

      log_count = 0;
      g_log_set_writer_func (null_log_writer, NULL, NULL);

      g_log_structured ("some-domain", G_LOG_LEVEL_MESSAGE,
                        "MESSAGE_ID", "06d4df59e6c24647bfe69d2c27ef0b4e",
                        "MY_APPLICATION_CUSTOM_FIELD", "some debug string",
                        "MESSAGE", "This is a debug message about pointer %p and integer %u.",
                        some_pointer, some_integer);

      g_assert_cmpint (log_count, ==, 1);
    }
  else
    {
      g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
      g_test_trap_assert_passed ();
    }
}

static void
test_structured_logging_some_state (void)
{
  /* Test has to run in a subprocess as it calls g_log_set_writer_func(), which
   * can only be called once per process. */
  if (g_test_subprocess ())
    {
      gpointer state_object = NULL;  /* this must not be dereferenced */
      const GLogField fields[] = {
        { "MESSAGE", "This is a debug message.", -1 },
        { "MESSAGE_ID", "fcfb2e1e65c3494386b74878f1abf893", -1 },
        { "MY_APPLICATION_CUSTOM_FIELD", "some debug string", -1 },
        { "MY_APPLICATION_STATE", state_object, 0 },
      };

      log_count = 0;
      g_log_set_writer_func (null_log_writer, NULL, NULL);

      g_log_structured_array (G_LOG_LEVEL_DEBUG, fields, G_N_ELEMENTS (fields));

      g_assert_cmpint (log_count, ==, 1);
    }
  else
    {
      g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
      g_test_trap_assert_passed ();
    }
}

static void
test_structured_logging_robustness (void)
{
  /* Test has to run in a subprocess as it calls g_log_set_writer_func(), which
   * can only be called once per process. */
  if (g_test_subprocess ())
    {
      log_count = 0;
      g_log_set_writer_func (null_log_writer, NULL, NULL);

      /* NULL log_domain shouldn't crash */
      g_log (NULL, G_LOG_LEVEL_MESSAGE, "Test");
      g_log_structured (NULL, G_LOG_LEVEL_MESSAGE, "MESSAGE", "Test");

      g_assert_cmpint (log_count, ==, 2);
    }
  else
    {
      g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
      g_test_trap_assert_passed ();
    }
}

static void
test_structured_logging_roundtrip1 (void)
{
  /* Test has to run in a subprocess as it calls g_log_set_writer_func(), which
   * can only be called once per process. */
  if (g_test_subprocess ())
    {
      gpointer some_pointer = GUINT_TO_POINTER (0x100);
      gint some_integer = 123;
      gchar message[200];
      GLogField fields[] = {
        { "GLIB_DOMAIN", "some-domain", -1 },
        { "PRIORITY", "5", -1 },
        { "MESSAGE", "String assigned using g_snprintf() below", -1 },
        { "MESSAGE_ID", "fcfb2e1e65c3494386b74878f1abf893", -1 },
        { "MY_APPLICATION_CUSTOM_FIELD", "some debug string", -1 }
      };
      ExpectedMessage expected = { fields, 5 };

      /* %p format is implementation defined and depends on the platform */
      g_snprintf (message, sizeof (message),
                  "This is a debug message about pointer %p and integer %u.",
                  some_pointer, some_integer);
      fields[2].value = message;

      expected_messages = g_slist_append (NULL, &expected);
      g_log_set_writer_func (expect_log_writer, NULL, NULL);

      g_log_structured ("some-domain", G_LOG_LEVEL_MESSAGE,
                        "MESSAGE_ID", "fcfb2e1e65c3494386b74878f1abf893",
                        "MY_APPLICATION_CUSTOM_FIELD", "some debug string",
                        "MESSAGE", "This is a debug message about pointer %p and integer %u.",
                        some_pointer, some_integer);

      if (expected_messages != NULL)
        {
          char *str;
          ExpectedMessage *msg = expected_messages->data;

          str = g_log_writer_format_fields (0, msg->fields, msg->n_fields, FALSE);
          g_test_fail_printf ("Unexpected message: %s", str);
          g_free (str);
        }
    }
  else
    {
      g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
      g_test_trap_assert_passed ();
    }
}

static void
test_structured_logging_roundtrip2 (void)
{
  /* Test has to run in a subprocess as it calls g_log_set_writer_func(), which
   * can only be called once per process. */
  if (g_test_subprocess ())
    {
      const gchar *some_string = "abc";
      const GLogField fields[] = {
        { "GLIB_DOMAIN", "some-domain", -1 },
        { "PRIORITY", "5", -1 },
        { "MESSAGE", "This is a debug message about string 'abc'.", -1 },
        { "MESSAGE_ID", "fcfb2e1e65c3494386b74878f1abf893", -1 },
        { "MY_APPLICATION_CUSTOM_FIELD", "some debug string", -1 }
      };
      ExpectedMessage expected = { fields, 5 };

      expected_messages = g_slist_append (NULL, &expected);
      g_log_set_writer_func (expect_log_writer, NULL, NULL);

      g_log_structured ("some-domain", G_LOG_LEVEL_MESSAGE,
                        "MESSAGE_ID", "fcfb2e1e65c3494386b74878f1abf893",
                        "MY_APPLICATION_CUSTOM_FIELD", "some debug string",
                        "MESSAGE", "This is a debug message about string '%s'.",
                        some_string);

      g_assert (expected_messages == NULL);
    }
  else
    {
      g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
      g_test_trap_assert_passed ();
    }
}

static void
test_structured_logging_roundtrip3 (void)
{
  /* Test has to run in a subprocess as it calls g_log_set_writer_func(), which
   * can only be called once per process. */
  if (g_test_subprocess ())
    {
      const GLogField fields[] = {
        { "GLIB_DOMAIN", "some-domain", -1 },
        { "PRIORITY", "4", -1 },
        { "MESSAGE", "Test test test.", -1 }
      };
      ExpectedMessage expected = { fields, 3 };

      expected_messages = g_slist_append (NULL, &expected);
      g_log_set_writer_func (expect_log_writer, NULL, NULL);

      g_log_structured ("some-domain", G_LOG_LEVEL_WARNING,
                        "MESSAGE", "Test test test.");

      g_assert (expected_messages == NULL);
    }
  else
    {
      g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
      g_test_trap_assert_passed ();
    }
}

static GVariant *
create_variant_fields (void)
{
  GVariant *binary;
  GVariantBuilder builder;

  binary = g_variant_new_fixed_array (G_VARIANT_TYPE_BYTE, binary_field, G_N_ELEMENTS (binary_field), sizeof (binary_field[0]));

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{sv}"));
  g_variant_builder_add (&builder, "{sv}", "MESSAGE_ID", g_variant_new_string ("06d4df59e6c24647bfe69d2c27ef0b4e"));
  g_variant_builder_add (&builder, "{sv}", "MESSAGE", g_variant_new_string ("This is a debug message"));
  g_variant_builder_add (&builder, "{sv}", "MY_APPLICATION_CUSTOM_FIELD", g_variant_new_string ("some debug string"));
  g_variant_builder_add (&builder, "{sv}", "MY_APPLICATION_CUSTOM_FIELD_BINARY", binary);

  return g_variant_builder_end (&builder);
}

static void
test_structured_logging_variant1 (void)
{
  /* Test has to run in a subprocess as it calls g_log_set_writer_func(), which
   * can only be called once per process. */
  if (g_test_subprocess ())
    {
      GVariant *v = create_variant_fields ();

      log_count = 0;
      g_log_set_writer_func (null_log_writer, NULL, NULL);

      g_log_variant ("some-domain", G_LOG_LEVEL_MESSAGE, v);
      g_variant_unref (v);
      g_assert_cmpint (log_count, ==, 1);
    }
  else
    {
      g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
      g_test_trap_assert_passed ();
    }
}

static void
test_structured_logging_variant2 (void)
{
  /* Test has to run in a subprocess as it calls g_log_set_writer_func(), which
   * can only be called once per process. */
  if (g_test_subprocess ())
    {
      const GLogField fields[] = {
        { "GLIB_DOMAIN", "some-domain", -1 },
        { "PRIORITY", "5", -1 },
        { "MESSAGE", "This is a debug message", -1 },
        { "MESSAGE_ID", "06d4df59e6c24647bfe69d2c27ef0b4e", -1 },
        { "MY_APPLICATION_CUSTOM_FIELD", "some debug string", -1 },
        { "MY_APPLICATION_CUSTOM_FIELD_BINARY", binary_field, sizeof (binary_field) }
      };
      ExpectedMessage expected = { fields, 6 };
      GVariant *v = create_variant_fields ();

      expected_messages = g_slist_append (NULL, &expected);
      g_log_set_writer_func (expect_log_writer, NULL, NULL);

      g_log_variant ("some-domain", G_LOG_LEVEL_MESSAGE, v);
      g_variant_unref (v);
      g_assert (expected_messages == NULL);
    }
  else
    {
      g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
      g_test_trap_assert_passed ();
    }
}

static void
test_structured_logging_set_writer_func_twice (void)
{
  /* Test has to run in a subprocess as it calls g_log_set_writer_func() and
   * causes an error. */
  if (g_test_subprocess ())
    {
      g_log_set_writer_func (null_log_writer, NULL, NULL);
      g_log_set_writer_func (expect_log_writer, NULL, NULL);
    }
  else
    {
      g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
      g_test_trap_assert_failed ();
    }
}

static GLogWriterOutput
n_fields_log_writer (GLogLevelFlags log_level, const GLogField *fields, gsize n_fields, gpointer user_data)
{
  size_t expected_n_fields = *(size_t *) user_data;

  /* Let process exit successfully for g_test_trap_assert_passed(). */
  if (n_fields == expected_n_fields)
    _exit (0);

  return G_LOG_WRITER_HANDLED;
}

static void
test_structured_logging_recursion_overflow (void)
{
  g_test_summary ("Test that g_log_structured always sets n_fields correctly.");
  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/3760");

  if (g_test_subprocess ())
    {
      size_t expected_n_fields = 16;
      const int log_level = G_LOG_LEVEL_MESSAGE | G_LOG_FLAG_RECURSION;

      g_log_set_writer_func (n_fields_log_writer, &expected_n_fields, NULL);
      g_log_structured (G_LOG_DOMAIN, log_level,
                        "key01", "value",
                        "key02", "value",
                        "key03", "value",
                        "key04", "value",
                        "key05", "value",
                        "key06", "value",
                        "key07", "value",
                        "key08", "value",
                        "key09", "value",
                        "key10", "value",
                        "key11", "value",
                        "key12", "value",
                        "key13", "value",
                        "key14", "value",
                        "key15", "value",
                        "key16", "value",
                        "key17", "value",
                        "key18", "value",
                        "MESSAGE", "Triggered segmentation fault in the past");
    }
  else
    {
      g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
      g_test_trap_assert_passed ();
    }
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/logging/default-handler", test_default_handler);
  g_test_add_func ("/logging/default-handler/subprocess/error", test_default_handler_error);
  g_test_add_func ("/logging/default-handler/subprocess/error-stderr", test_default_handler_error_stderr);
  g_test_add_func ("/logging/default-handler/subprocess/critical", test_default_handler_critical);
  g_test_add_func ("/logging/default-handler/subprocess/critical-stderr", test_default_handler_critical_stderr);
  g_test_add_func ("/logging/default-handler/subprocess/warning", test_default_handler_warning);
  g_test_add_func ("/logging/default-handler/subprocess/warning-stderr", test_default_handler_warning_stderr);
  g_test_add_func ("/logging/default-handler/subprocess/message", test_default_handler_message);
  g_test_add_func ("/logging/default-handler/subprocess/message-stderr", test_default_handler_message_stderr);
  g_test_add_func ("/logging/default-handler/subprocess/info", test_default_handler_info);
  g_test_add_func ("/logging/default-handler/subprocess/info-stderr", test_default_handler_info_stderr);
  g_test_add_func ("/logging/default-handler/subprocess/bar-info", test_default_handler_bar_info);
  g_test_add_func ("/logging/default-handler/subprocess/baz-debug", test_default_handler_baz_debug);
  g_test_add_func ("/logging/default-handler/subprocess/debug", test_default_handler_debug);
  g_test_add_func ("/logging/default-handler/subprocess/debug-stderr", test_default_handler_debug_stderr);
  g_test_add_func ("/logging/default-handler/subprocess/0x400", test_default_handler_0x400);
  g_test_add_func ("/logging/default-handler/subprocess/would-drop", test_default_handler_would_drop);
  g_test_add_func ("/logging/default-handler/subprocess/would-drop-env1", test_default_handler_would_drop_env1);
  g_test_add_func ("/logging/default-handler/subprocess/would-drop-env2", test_default_handler_would_drop_env2);
  g_test_add_func ("/logging/default-handler/subprocess/would-drop-env3", test_default_handler_would_drop_env3);
  g_test_add_func ("/logging/default-handler/subprocess/would-drop-env4", test_default_handler_would_drop_env4);
  g_test_add_func ("/logging/default-handler/subprocess/would-drop-env5", test_default_handler_would_drop_env5);
  g_test_add_func ("/logging/default-handler/subprocess/would-drop-env-systemd", test_default_handler_would_drop_env_systemd);
  g_test_add_func ("/logging/default-handler/subprocess/would-drop-robustness", test_default_handler_would_drop_robustness);
  g_test_add_func ("/logging/default-handler/subprocess/structured-logging-non-null-terminated-strings", test_default_handler_structured_logging_non_nul_terminated_strings);
  g_test_add_func ("/logging/journald-handler", test_journald_handler);
  g_test_add_func ("/logging/warnings", test_warnings);
  g_test_add_func ("/logging/fatal-log-mask", test_fatal_log_mask);
  g_test_add_func ("/logging/always-fatal", test_always_fatal);
  g_test_add_func ("/logging/set-handler", test_set_handler);
  g_test_add_func ("/logging/print-handler", test_print_handler);
  g_test_add_func ("/logging/printerr-handler", test_printerr_handler);
  g_test_add_func ("/logging/653052", bug653052);
  g_test_add_func ("/logging/gibberish", test_gibberish);
  g_test_add_func ("/structured-logging/no-state", test_structured_logging_no_state);
  g_test_add_func ("/structured-logging/some-state", test_structured_logging_some_state);
  g_test_add_func ("/structured-logging/recursion-overflow", test_structured_logging_recursion_overflow);
  g_test_add_func ("/structured-logging/robustness", test_structured_logging_robustness);
  g_test_add_func ("/structured-logging/roundtrip1", test_structured_logging_roundtrip1);
  g_test_add_func ("/structured-logging/roundtrip2", test_structured_logging_roundtrip2);
  g_test_add_func ("/structured-logging/roundtrip3", test_structured_logging_roundtrip3);
  g_test_add_func ("/structured-logging/variant1", test_structured_logging_variant1);
  g_test_add_func ("/structured-logging/variant2", test_structured_logging_variant2);
  g_test_add_func ("/structured-logging/set-writer-func-twice", test_structured_logging_set_writer_func_twice);

  return g_test_run ();
}
