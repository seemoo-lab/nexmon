/* Test of command line argument processing.
   Copyright (C) 2009-2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2009.  */

#include <config.h>

/* Specification.  */
#include "options.h"

#include <string.h>

#include "macros.h"

static int a_seen;
static int b_seen;
static int q_seen;

/* Reduce casting, so we can use string literals elsewhere.
   getopt_long takes an array of char*, but luckily does not modify
   those elements, so we can pass const char*.  */

static void
options_loop (enum non_option_handling nonopt_handling,
              unsigned int error_handling,
              const char **p_value, const char **q_value,
              int *non_options_count, const char **non_options,
              int *unrecognized)
{
  int c;

  q_seen = 0;
  while ((c = get_next_option ()) != -1)
    {
      switch (c)
        {
        case 0:
          /* An option with key == 0 was processed.  */
          if (q_seen)
            *q_value = optarg;
          break;
        case 'a':
          a_seen++;
          break;
        case 'b':
          b_seen = 1;
          break;
        case 'p':
          *p_value = optarg;
          break;
        case 'q':
          *q_value = optarg;
          break;
        case '\1':
          ASSERT (nonopt_handling == PROCESS_NON_OPTIONS);
          non_options[(*non_options_count)++] = optarg;
          break;
        case ':':
          ASSERT (error_handling & OPTIONS_MISSING_IS_COLON);
          FALLTHROUGH;
        case '?':
          *unrecognized = optopt;
          break;
        default:
          *unrecognized = c;
          break;
        }
    }
}

static void
test_getopt_long (void)
{
  /* Test disambiguation of options.  */
  {
    static const struct program_option options[] =
      {
        { "alpha",     'a',  no_argument        },
        { "beta",      0,    no_argument,       &b_seen, 1 },
        { "prune",     1000, required_argument  },
        { "quetsche",  0,    required_argument, &q_seen, 1 },
        { "xtremely-", 1003, no_argument        },
        { "xtra",      1001, no_argument        },
        { "xtreme",    1002, no_argument        },
        { "xtremely",  1003, no_argument        },
        { NULL,        'b',  no_argument        },
      };

    {
      int argc = 0;
      const char *argv[10];
      int c;

      argv[argc++] = "program";
      argv[argc++] = "--x";
      argv[argc] = NULL;
      optind = 1;
      start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
      c = get_next_option ();
      ASSERT (c == '?');
      ASSERT (optopt == 0);
    }
    {
      int argc = 0;
      const char *argv[10];
      int c;

      argv[argc++] = "program";
      argv[argc++] = "--xt";
      argv[argc] = NULL;
      optind = 1;
      start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
      c = get_next_option ();
      ASSERT (c == '?');
      ASSERT (optopt == 0);
    }
    {
      int argc = 0;
      const char *argv[10];
      int c;

      argv[argc++] = "program";
      argv[argc++] = "--xtr";
      argv[argc] = NULL;
      optind = 1;
      start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
      c = get_next_option ();
      ASSERT (c == '?');
      ASSERT (optopt == 0);
    }
    {
      int argc = 0;
      const char *argv[10];
      int c;

      argv[argc++] = "program";
      argv[argc++] = "--xtra";
      argv[argc] = NULL;
      optind = 1;
      start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
      c = get_next_option ();
      ASSERT (c == 1001);
    }
    {
      int argc = 0;
      const char *argv[10];
      int c;

      argv[argc++] = "program";
      argv[argc++] = "--xtre";
      argv[argc] = NULL;
      optind = 1;
      start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
      c = get_next_option ();
      ASSERT (c == '?');
      ASSERT (optopt == 0);
    }
    {
      int argc = 0;
      const char *argv[10];
      int c;

      argv[argc++] = "program";
      argv[argc++] = "--xtrem";
      argv[argc] = NULL;
      optind = 1;
      start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
      c = get_next_option ();
      ASSERT (c == '?');
      ASSERT (optopt == 0);
    }
    {
      int argc = 0;
      const char *argv[10];
      int c;

      argv[argc++] = "program";
      argv[argc++] = "--xtreme";
      argv[argc] = NULL;
      optind = 1;
      start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
      c = get_next_option ();
      ASSERT (c == 1002);
    }
    {
      int argc = 0;
      const char *argv[10];
      int c;

      argv[argc++] = "program";
      argv[argc++] = "--xtremel";
      argv[argc] = NULL;
      optind = 1;
      start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
      c = get_next_option ();
      ASSERT (c == 1003);
    }
    {
      int argc = 0;
      const char *argv[10];
      int c;

      argv[argc++] = "program";
      argv[argc++] = "--xtremely";
      argv[argc] = NULL;
      optind = 1;
      start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
      c = get_next_option ();
      ASSERT (c == 1003);
    }
  }

  {
    static const struct program_option options[] =
      {
        { "alpha",     'a',  no_argument        },
        { "beta",      0,    no_argument,       &b_seen, 1 },
        { "prune",     1000, required_argument  },
        { "quetsche",  0,    required_argument, &q_seen, 1 },
        { "xtremely-", 1003, no_argument        },
        { "xtra",      1001, no_argument        },
        { "xtreme",    1002, no_argument        },
        { "xtremely",  1003, no_argument        },
        { NULL,        'b',  no_argument        },
      };

    /* Test processing of boolean short options.  */
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "-a";
        argv[argc++] = "foo";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 1);
        ASSERT (b_seen == 0);
        ASSERT (p_value == NULL);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 2);
      }
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "-b";
        argv[argc++] = "-a";
        argv[argc++] = "foo";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 1);
        ASSERT (b_seen == 1);
        ASSERT (p_value == NULL);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 3);
      }
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "-ba";
        argv[argc++] = "foo";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 1);
        ASSERT (b_seen == 1);
        ASSERT (p_value == NULL);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 2);
      }
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "-ab";
        argv[argc++] = "-a";
        argv[argc++] = "foo";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 2);
        ASSERT (b_seen == 1);
        ASSERT (p_value == NULL);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 3);
      }

    /* Test processing of boolean long options.  */
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "--alpha";
        argv[argc++] = "foo";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 1);
        ASSERT (b_seen == 0);
        ASSERT (p_value == NULL);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 2);
      }
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "--beta";
        argv[argc++] = "--alpha";
        argv[argc++] = "foo";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 1);
        ASSERT (b_seen == 1);
        ASSERT (p_value == NULL);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 3);
      }
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "--alpha";
        argv[argc++] = "--beta";
        argv[argc++] = "--alpha";
        argv[argc++] = "--beta";
        argv[argc++] = "foo";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 2);
        ASSERT (b_seen == 1);
        ASSERT (p_value == NULL);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 5);
      }
  }

  {
    static const struct program_option options[] =
      {
        { "alpha",     0,    no_argument        },
        { "beta",      0,    no_argument,       &b_seen, 1 },
        { "prune",     'p',  required_argument  },
        { "quetsche",  0,    required_argument, &q_seen, 1 },
        { "xtremely-", 1003, no_argument        },
        { "xtra",      1001, no_argument        },
        { "xtreme",    1002, no_argument        },
        { "xtremely",  1003, no_argument        },
        { NULL,        'q',  required_argument  },
      };
    static const struct program_option options_with_ab[] =
      {
        { "alpha",     'a',  no_argument        },
        { "beta",      0,    no_argument,       &b_seen, 1 },
        { "prune",     'p',  required_argument  },
        { "quetsche",  0,    required_argument, &q_seen, 1 },
        { "xtremely-", 1003, no_argument        },
        { "xtra",      1001, no_argument        },
        { "xtreme",    1002, no_argument        },
        { "xtremely",  1003, no_argument        },
        { NULL,        'q',  required_argument  },
        { NULL,        'b',  no_argument        },
      };

    /* Test processing of short options with arguments.  */
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "-pfoo";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 0);
        ASSERT (b_seen == 0);
        ASSERT (p_value != NULL && strcmp (p_value, "foo") == 0);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 2);
      }
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "-p";
        argv[argc++] = "foo";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 0);
        ASSERT (b_seen == 0);
        ASSERT (p_value != NULL && strcmp (p_value, "foo") == 0);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 3);
      }
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "-ab";
        argv[argc++] = "-q";
        argv[argc++] = "baz";
        argv[argc++] = "-pfoo";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options_with_ab, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 1);
        ASSERT (b_seen == 1);
        ASSERT (p_value != NULL && strcmp (p_value, "foo") == 0);
        ASSERT (q_value != NULL && strcmp (q_value, "baz") == 0);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 5);
      }

    /* Test processing of long options with arguments.  */
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "--p=foo";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 0);
        ASSERT (b_seen == 0);
        ASSERT (p_value != NULL && strcmp (p_value, "foo") == 0);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 2);
      }
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "--p";
        argv[argc++] = "foo";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 0);
        ASSERT (b_seen == 0);
        ASSERT (p_value != NULL && strcmp (p_value, "foo") == 0);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 3);
      }
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "-ab";
        argv[argc++] = "--q";
        argv[argc++] = "baz";
        argv[argc++] = "--p=foo";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options_with_ab, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 1);
        ASSERT (b_seen == 1);
        ASSERT (p_value != NULL && strcmp (p_value, "foo") == 0);
        ASSERT (q_value != NULL && strcmp (q_value, "baz") == 0);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 5);
      }
  }

  {
    static const struct program_option options[] =
      {
        { "alpha",     0,    no_argument        },
        { "beta",      0,    no_argument,       &b_seen, 1 },
        { "prune",     'p',  optional_argument  },
        { "quetsche",  0,    optional_argument, &q_seen, 1 },
        { NULL,        'q',  optional_argument  },
      };
    static const struct program_option options_with_ab[] =
      {
        { "alpha",     'a',  no_argument        },
        { "beta",      0,    no_argument,       &b_seen, 1 },
        { "prune",     'p',  optional_argument  },
        { "quetsche",  0,    optional_argument, &q_seen, 1 },
        { NULL,        'q',  optional_argument  },
        { NULL,        'b',  no_argument        },
      };

    /* Test processing of short options with optional arguments.  */
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "-pfoo";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 0);
        ASSERT (b_seen == 0);
        ASSERT (p_value != NULL && strcmp (p_value, "foo") == 0);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 2);
      }
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "-p";
        argv[argc++] = "foo";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 0);
        ASSERT (b_seen == 0);
        ASSERT (p_value == NULL);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 2);
      }
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "-p";
        argv[argc++] = "-a";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options_with_ab, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 1);
        ASSERT (b_seen == 0);
        ASSERT (p_value == NULL);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 3);
      }

    /* Test processing of long options with optional arguments.  */
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "--p=foo";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 0);
        ASSERT (b_seen == 0);
        ASSERT (p_value != NULL && strcmp (p_value, "foo") == 0);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 2);
      }
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "--p";
        argv[argc++] = "foo";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 0);
        ASSERT (b_seen == 0);
        ASSERT (p_value == NULL);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 2);
      }
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "--p=";
        argv[argc++] = "foo";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 0);
        ASSERT (b_seen == 0);
        ASSERT (p_value != NULL && *p_value == '\0');
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 2);
      }
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "--p";
        argv[argc++] = "-a";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options_with_ab, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 1);
        ASSERT (b_seen == 0);
        ASSERT (p_value == NULL);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 3);
      }
  }

  {
    static const struct program_option options[] =
      {
        { "alpha",     'a',  no_argument        },
        { "beta",      0,    no_argument,       &b_seen, 1 },
        { "prune",     'p',  required_argument  },
        { "quetsche",  0,    required_argument, &q_seen, 1 },
        { "xtremely-", 1003, no_argument        },
        { "xtra",      1001, no_argument        },
        { "xtreme",    1002, no_argument        },
        { "xtremely",  1003, no_argument        },
        { NULL,        'q',  required_argument  },
        { NULL,        'b',  no_argument        },
      };

    /* Check that invalid options are recognized.  */
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "-p";
        argv[argc++] = "foo";
        argv[argc++] = "-x";
        argv[argc++] = "-a";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 1);
        ASSERT (b_seen == 0);
        ASSERT (p_value != NULL && strcmp (p_value, "foo") == 0);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 'x');
        ASSERT (optind == 5);
      }
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "-p";
        argv[argc++] = "foo";
        argv[argc++] = "-:";
        argv[argc++] = "-a";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 1);
        ASSERT (b_seen == 0);
        ASSERT (p_value != NULL && strcmp (p_value, "foo") == 0);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == ':');
        ASSERT (optind == 5);
      }

    /* Check that unexpected arguments are recognized.  */
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "-p";
        argv[argc++] = "foo";
        argv[argc++] = "--a=";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 0);
        ASSERT (b_seen == 0);
        ASSERT (p_value != NULL && strcmp (p_value, "foo") == 0);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 'a');
        ASSERT (optind == 4);
      }
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "-p";
        argv[argc++] = "foo";
        argv[argc++] = "--b=";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 0);
        ASSERT (b_seen == 0);
        ASSERT (p_value != NULL && strcmp (p_value, "foo") == 0);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        /* When flag is non-zero, glibc sets optopt anyway, but BSD
           leaves optopt unchanged.  */
        ASSERT (unrecognized == 1 || unrecognized == 0);
        ASSERT (optind == 4);
      }

    /* Check that by default, non-options arguments are moved to the end.  */
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "donald";
        argv[argc++] = "-p";
        argv[argc++] = "billy";
        argv[argc++] = "duck";
        argv[argc++] = "-a";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (strcmp (argv[0], "program") == 0);
        ASSERT (strcmp (argv[1], "-p") == 0);
        ASSERT (strcmp (argv[2], "billy") == 0);
        ASSERT (strcmp (argv[3], "-a") == 0);
        ASSERT (strcmp (argv[4], "donald") == 0);
        ASSERT (strcmp (argv[5], "duck") == 0);
        ASSERT (strcmp (argv[6], "bar") == 0);
        ASSERT (argv[7] == NULL);
        ASSERT (a_seen == 1);
        ASSERT (b_seen == 0);
        ASSERT (p_value != NULL && strcmp (p_value, "billy") == 0);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 4);
      }

    /* Check that '--' ends the argument processing.  */
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[20];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "donald";
        argv[argc++] = "-p";
        argv[argc++] = "billy";
        argv[argc++] = "duck";
        argv[argc++] = "-a";
        argv[argc++] = "--";
        argv[argc++] = "-b";
        argv[argc++] = "foo";
        argv[argc++] = "-q";
        argv[argc++] = "johnny";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (strcmp (argv[0], "program") == 0);
        ASSERT (strcmp (argv[1], "-p") == 0);
        ASSERT (strcmp (argv[2], "billy") == 0);
        ASSERT (strcmp (argv[3], "-a") == 0);
        ASSERT (strcmp (argv[4], "--") == 0);
        ASSERT (strcmp (argv[5], "donald") == 0);
        ASSERT (strcmp (argv[6], "duck") == 0);
        ASSERT (strcmp (argv[7], "-b") == 0);
        ASSERT (strcmp (argv[8], "foo") == 0);
        ASSERT (strcmp (argv[9], "-q") == 0);
        ASSERT (strcmp (argv[10], "johnny") == 0);
        ASSERT (strcmp (argv[11], "bar") == 0);
        ASSERT (argv[12] == NULL);
        ASSERT (a_seen == 1);
        ASSERT (b_seen == 0);
        ASSERT (p_value != NULL && strcmp (p_value, "billy") == 0);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 5);
      }

    /* Check that the PROCESS_NON_OPTIONS flag causes non-options to be
       returned in order.  */
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "donald";
        argv[argc++] = "-p";
        argv[argc++] = "billy";
        argv[argc++] = "duck";
        argv[argc++] = "-a";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, PROCESS_NON_OPTIONS, OPTIONS_ERRORS_SILENT);
        options_loop (PROCESS_NON_OPTIONS, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (strcmp (argv[0], "program") == 0);
        ASSERT (strcmp (argv[1], "donald") == 0);
        ASSERT (strcmp (argv[2], "-p") == 0);
        ASSERT (strcmp (argv[3], "billy") == 0);
        ASSERT (strcmp (argv[4], "duck") == 0);
        ASSERT (strcmp (argv[5], "-a") == 0);
        ASSERT (strcmp (argv[6], "bar") == 0);
        ASSERT (argv[7] == NULL);
        ASSERT (a_seen == 1);
        ASSERT (b_seen == 0);
        ASSERT (p_value != NULL && strcmp (p_value, "billy") == 0);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 3);
        ASSERT (strcmp (non_options[0], "donald") == 0);
        ASSERT (strcmp (non_options[1], "duck") == 0);
        ASSERT (strcmp (non_options[2], "bar") == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 7);
      }

    /* Check that '--' ends the argument processing.  */
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[20];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "donald";
        argv[argc++] = "-p";
        argv[argc++] = "billy";
        argv[argc++] = "duck";
        argv[argc++] = "-a";
        argv[argc++] = "--";
        argv[argc++] = "-b";
        argv[argc++] = "foo";
        argv[argc++] = "-q";
        argv[argc++] = "johnny";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, PROCESS_NON_OPTIONS, OPTIONS_ERRORS_SILENT);
        options_loop (PROCESS_NON_OPTIONS, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (strcmp (argv[0], "program") == 0);
        ASSERT (strcmp (argv[1], "donald") == 0);
        ASSERT (strcmp (argv[2], "-p") == 0);
        ASSERT (strcmp (argv[3], "billy") == 0);
        ASSERT (strcmp (argv[4], "duck") == 0);
        ASSERT (strcmp (argv[5], "-a") == 0);
        ASSERT (strcmp (argv[6], "--") == 0);
        ASSERT (strcmp (argv[7], "-b") == 0);
        ASSERT (strcmp (argv[8], "foo") == 0);
        ASSERT (strcmp (argv[9], "-q") == 0);
        ASSERT (strcmp (argv[10], "johnny") == 0);
        ASSERT (strcmp (argv[11], "bar") == 0);
        ASSERT (argv[12] == NULL);
        ASSERT (a_seen == 1);
        ASSERT (b_seen == 0);
        ASSERT (p_value != NULL && strcmp (p_value, "billy") == 0);
        ASSERT (q_value == NULL);
        if (non_options_count == 2)
        {
          /* glibc behaviour.  */
          ASSERT (non_options_count == 2);
          ASSERT (strcmp (non_options[0], "donald") == 0);
          ASSERT (strcmp (non_options[1], "duck") == 0);
          ASSERT (unrecognized == 0);
          ASSERT (optind == 7);
        }
        else
        {
          /* Another valid behaviour.  */
          ASSERT (non_options_count == 7);
          ASSERT (strcmp (non_options[0], "donald") == 0);
          ASSERT (strcmp (non_options[1], "duck") == 0);
          ASSERT (strcmp (non_options[2], "-b") == 0);
          ASSERT (strcmp (non_options[3], "foo") == 0);
          ASSERT (strcmp (non_options[4], "-q") == 0);
          ASSERT (strcmp (non_options[5], "johnny") == 0);
          ASSERT (strcmp (non_options[6], "bar") == 0);
          ASSERT (unrecognized == 0);
          ASSERT (optind == 12);
        }
      }

    /* Check that the NON_OPTION_TERMINATES_OPTIONS flag causes the first
       non-option to terminate the loop.  */
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "donald";
        argv[argc++] = "-p";
        argv[argc++] = "billy";
        argv[argc++] = "duck";
        argv[argc++] = "-a";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, NON_OPTION_TERMINATES_OPTIONS, OPTIONS_ERRORS_SILENT);
        options_loop (NON_OPTION_TERMINATES_OPTIONS, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (strcmp (argv[0], "program") == 0);
        ASSERT (strcmp (argv[1], "donald") == 0);
        ASSERT (strcmp (argv[2], "-p") == 0);
        ASSERT (strcmp (argv[3], "billy") == 0);
        ASSERT (strcmp (argv[4], "duck") == 0);
        ASSERT (strcmp (argv[5], "-a") == 0);
        ASSERT (strcmp (argv[6], "bar") == 0);
        ASSERT (argv[7] == NULL);
        ASSERT (a_seen == 0);
        ASSERT (b_seen == 0);
        ASSERT (p_value == NULL);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 1);
      }
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "-+";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, NON_OPTION_TERMINATES_OPTIONS, OPTIONS_ERRORS_SILENT);
        options_loop (NON_OPTION_TERMINATES_OPTIONS, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 0);
        ASSERT (b_seen == 0);
        ASSERT (p_value == NULL);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == '+');
        ASSERT (optind == 2);
      }

    /* Check that '--' ends the argument processing.  */
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[20];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "donald";
        argv[argc++] = "-p";
        argv[argc++] = "billy";
        argv[argc++] = "duck";
        argv[argc++] = "-a";
        argv[argc++] = "--";
        argv[argc++] = "-b";
        argv[argc++] = "foo";
        argv[argc++] = "-q";
        argv[argc++] = "johnny";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, NON_OPTION_TERMINATES_OPTIONS, OPTIONS_ERRORS_SILENT);
        options_loop (NON_OPTION_TERMINATES_OPTIONS, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (strcmp (argv[0], "program") == 0);
        ASSERT (strcmp (argv[1], "donald") == 0);
        ASSERT (strcmp (argv[2], "-p") == 0);
        ASSERT (strcmp (argv[3], "billy") == 0);
        ASSERT (strcmp (argv[4], "duck") == 0);
        ASSERT (strcmp (argv[5], "-a") == 0);
        ASSERT (strcmp (argv[6], "--") == 0);
        ASSERT (strcmp (argv[7], "-b") == 0);
        ASSERT (strcmp (argv[8], "foo") == 0);
        ASSERT (strcmp (argv[9], "-q") == 0);
        ASSERT (strcmp (argv[10], "johnny") == 0);
        ASSERT (strcmp (argv[11], "bar") == 0);
        ASSERT (argv[12] == NULL);
        ASSERT (a_seen == 0);
        ASSERT (b_seen == 0);
        ASSERT (p_value == NULL);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 1);
      }
  }
}

/* Test behavior of getopt_long when POSIXLY_CORRECT is set in the
   environment.  Options with optional arguments should not change
   behavior just because of an environment variable.
   https://lists.gnu.org/r/bug-m4/2006-09/msg00028.html  */
static void
test_getopt_long_posix (void)
{
  {
    static const struct program_option options[] =
      {
        { "alpha",     'a',  no_argument        },
        { "beta",      0,    no_argument,       &b_seen, 1 },
        { "prune",     'p',  required_argument  },
        { "quetsche",  0,    required_argument, &q_seen, 1 },
        { "xtremely-", 1003, no_argument        },
        { "xtra",      1001, no_argument        },
        { "xtreme",    1002, no_argument        },
        { "xtremely",  1003, no_argument        },
        { NULL,        'q',  required_argument  },
        { NULL,        'b',  no_argument        },
      };

    /* Check that POSIXLY_CORRECT stops parsing the same as leading '+'.  */
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "donald";
        argv[argc++] = "-p";
        argv[argc++] = "billy";
        argv[argc++] = "duck";
        argv[argc++] = "-a";
        argv[argc++] = "bar";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (strcmp (argv[0], "program") == 0);
        ASSERT (strcmp (argv[1], "donald") == 0);
        ASSERT (strcmp (argv[2], "-p") == 0);
        ASSERT (strcmp (argv[3], "billy") == 0);
        ASSERT (strcmp (argv[4], "duck") == 0);
        ASSERT (strcmp (argv[5], "-a") == 0);
        ASSERT (strcmp (argv[6], "bar") == 0);
        ASSERT (argv[7] == NULL);
        ASSERT (a_seen == 0);
        ASSERT (b_seen == 0);
        ASSERT (p_value == NULL);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 1);
      }
  }

  {
    static const struct program_option options[] =
      {
        { "alpha",     0,    no_argument        },
        { "beta",      0,    no_argument,       &b_seen, 1 },
        { "prune",     0,    required_argument  },
        { "quetsche",  0,    required_argument, &q_seen, 1 },
        { "xtremely-", 1003, no_argument        },
        { "xtra",      1001, no_argument        },
        { "xtreme",    1002, no_argument        },
        { "xtremely",  1003, no_argument        },
        { NULL,        'p',  optional_argument  },
      };

    /* Check that POSIXLY_CORRECT doesn't change optional arguments.  */
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "-p";
        argv[argc++] = "billy";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT);
        options_loop (MOVE_OPTIONS_FIRST, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 0);
        ASSERT (b_seen == 0);
        ASSERT (p_value == NULL);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 2);
      }
  }

  {
    static const struct program_option options[] =
      {
        { "alpha",     'a',  no_argument        },
        { "beta",      0,    no_argument,       &b_seen, 1 },
        { "prune",     0,    required_argument  },
        { "quetsche",  0,    required_argument, &q_seen, 1 },
        { "xtremely-", 1003, no_argument        },
        { "xtra",      1001, no_argument        },
        { "xtreme",    1002, no_argument        },
        { "xtremely",  1003, no_argument        },
        { NULL,        'b',  no_argument        },
      };

    /* Check that leading - still sees options after non-options.  */
    for (int start = 0; start <= 1; start++)
      {
        const char *p_value = NULL;
        const char *q_value = NULL;
        int non_options_count = 0;
        const char *non_options[10];
        int unrecognized = 0;
        int argc = 0;
        const char *argv[10];
        a_seen = 0;
        b_seen = 0;

        argv[argc++] = "program";
        argv[argc++] = "-a";
        argv[argc++] = "billy";
        argv[argc++] = "-b";
        argv[argc] = NULL;
        optind = start;
        start_options (argc, (char **) argv, options, PROCESS_NON_OPTIONS, OPTIONS_ERRORS_SILENT);
        options_loop (PROCESS_NON_OPTIONS, OPTIONS_ERRORS_SILENT,
                      &p_value, &q_value,
                      &non_options_count, non_options, &unrecognized);
        ASSERT (a_seen == 1);
        ASSERT (b_seen == 1);
        ASSERT (p_value == NULL);
        ASSERT (q_value == NULL);
        ASSERT (non_options_count == 1);
        ASSERT (strcmp (non_options[0], "billy") == 0);
        ASSERT (unrecognized == 0);
        ASSERT (optind == 4);
      }
  }
}

int
main ()
{
  setenv ("POSIXLY_CORRECT", "1", 1);

  test_getopt_long_posix ();

  unsetenv ("POSIXLY_CORRECT");

  test_getopt_long ();

  return test_exit_status;
}
