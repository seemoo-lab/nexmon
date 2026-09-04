/* Querying a Large Language Model.
   Copyright (C) 2025-2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible.  */

/*
 * This program passes an input to an ollama instance and prints the response.
 */

#include <config.h>

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>

/* We use JSON-C.
   It is multithread-safe, as long as we don't use any of the json_global_*
   API functions, nor the json_util_get_last_err function.
   Documentation:
   <http://json-c.github.io/json-c/json-c-0.18/doc/html/index.html#using>  */
#include <json.h>

/* We use libcurl.
   Documentation:
   <https://curl.se/libcurl/c/>  */
#include <curl/curl.h>

#include <error.h>
#include "options.h"
#include "closeout.h"
#include "progname.h"
#include "relocatable.h"
#include "basename-lgpl.h"
#include "xalloc.h"
#include "xvasprintf.h"
#include "read-file.h"
#include "full-write.h"
#include "spawn-pipe.h"
#include "wait-process.h"
#include "lang-table.h"
#include "country-table.h"
#include "propername.h"
#include "gettext.h"

#define _(str) gettext (str)


/* Returns the English name of a language (lowercase ISO 639 code),
   or NULL if unknown.  */
static const char *
englishname_of_language (const char *language)
{
  for (size_t i = 0; i < language_table_size; i++)
    if (strcmp (language_table[i].code, language) == 0)
      return language_table[i].english;

  return NULL;
}

/* Returns the English name of a country (uppercase ISO 3166 code),
   or NULL if unknown.  */
static const char *
englishname_of_country (const char *country)
{
  for (size_t i = 0; i < country_table_size; i++)
    if (strcmp (country_table[i].code, country) == 0)
      return country_table[i].english;

  return NULL;
}

/* Returns a name or description of a catalog name.  */
static const char *
language_in_english (const char *catalogname)
{
  const char *underscore = strchr (catalogname, '_');
  if (underscore != NULL)
    {
      /* Treat a few cases specially.  */
      for (size_t i = 0; i < language_variant_table_size; i++)
        if (strcmp (language_variant_table[i].code, catalogname) == 0)
          return language_variant_table[i].english;

      /* Decompose "ll_CC" into "ll" and "CC".  */
      char *language = xstrdup (catalogname);
      language[underscore - catalogname] = '\0';

      const char *country = underscore + 1;

      const char *english_language = englishname_of_language (language);
      if (english_language != NULL)
        {
          const char *english_country = englishname_of_country (country);
          if (english_country != NULL)
            return xasprintf ("%s (as spoken in %s)", english_language, english_country);
          else
            return english_language;
        }
      else
        return catalogname;
    }
  else
    {
      /* It's a simple language name.  */
      const char *english_language = englishname_of_language (catalogname);
      if (english_language != NULL)
        return english_language;
      else
        return catalogname;
    }
}

static void
curl_die ()
{
  error (EXIT_FAILURE, 0, "%s", _("curl error"));
}

static void
process_response_line (const char *line, int out_fd)
{
  /* Note: As of json-c version 0.15, the jerrno value is unreliable.
     See <https://github.com/json-c/json-c/issues/853>
     and <https://github.com/json-c/json-c/issues/854>.
     json-c version 0.17 introduces json_tokener_error_memory, but its value
     changes in version 0.18.  */
  struct json_object *j;
#if JSON_C_MAJOR_VERSION > 0 || (JSON_C_MAJOR_VERSION == 0 && JSON_C_MINOR_VERSION >= 18)
  enum json_tokener_error jerrno = json_tokener_error_memory;
  j = json_tokener_parse_verbose (line, &jerrno);
  if (j == NULL && jerrno == json_tokener_error_memory)
    xalloc_die ();
#else
  j = json_tokener_parse (line);
#endif
  /* Ignore an empty line.  */
  if (j != NULL)
    {
      /* We expect a JSON object.  */
      if (json_object_is_type (j, json_type_object))
        {
          /* Output its "response" property.  */
          const char *prop =
            json_object_get_string(json_object_object_get (j, "response"));
          if (prop != NULL)
            {
              size_t prop_length = strlen (prop);
              if (full_write (out_fd, prop, prop_length) < prop_length)
                if (errno != EPIPE)
                  error (EXIT_FAILURE, errno, _("write to subprocess failed"));
            }
        }
    }
}

/* A libcurl header callback that determines, during curl_easy_perform,
   whether the HTTP request returned an error.  */
static size_t
my_header_callback (char *buffer, size_t one, size_t n, void *userdata)
{
  bool *is_error_p = (bool *) userdata;
#if DEBUG
  fprintf (stderr, "in my_header_callback: buffer = %.*s\n", (int) n, buffer);
#endif
  if (n >= 5 && memcmp (buffer, "HTTP/", 5) == 0)
    {
      /* buffer contains a line of the form "HTTP/1.1 code description".
         Extract the code.  */
      char old = buffer[n - 1];
      buffer[n - 1] = '\0';
      int code;
      if (sscanf (buffer, "%*s %d\n", &code) == 1)
        {
          if (code >= 400)
            *is_error_p = true;
        }
      buffer[n - 1] = old;
    }
  return n;
}

struct my_write_locals
{
  int out_fd;
  bool is_error;
  char *body;
  size_t body_allocated;
  size_t body_start;
  size_t body_end;
};

/* Makes room for n more bytes in l->body.  */
static void
my_write_grow (struct my_write_locals *l, size_t n)
{
  if ((l->body_end - l->body_start) + n > l->body_allocated)
    {
      /* Grow the buffer.  */
      size_t new_allocated = (l->body_end - l->body_start) + n;
      if (new_allocated < 2 * l->body_allocated)
        new_allocated = 2 * l->body_allocated;
      if (new_allocated < 1024)
        new_allocated = 1024;
      char *new_body;
      if (l->body_start == 0 && l->body_end > 0)
        {
          new_body = (char *) realloc (l->body, new_allocated);
          if (new_body == NULL)
            xalloc_die ();
        }
      else
        {
          new_body = (char *) malloc (new_allocated);
          if (new_body == NULL)
            xalloc_die ();
          memcpy (new_body, l->body + l->body_start, l->body_end - l->body_start);
          free (l->body);
          l->body_end = l->body_end - l->body_start;
          l->body_start = 0;
        }
      l->body = new_body;
      l->body_allocated = new_allocated;
    }
  else
    {
      /* We can keep the buffer, but may need to move the contents to the
         front.  */
      if (l->body_end + n > l->body_allocated)
        {
          memmove (l->body, l->body + l->body_start, l->body_end - l->body_start);
          l->body_end = l->body_end - l->body_start;
          l->body_start = 0;
        }
    }
  /* Here l->body_end + n <= l->body_allocated.  */
}

/* A libcurl write callback that processes a piece of response body,
   depending on whether the HTTP request returned an error.  */
static size_t
my_write_callback (char *buffer, size_t one, size_t n, void *userdata)
{
  struct my_write_locals *l = (struct my_write_locals *) userdata;
#if DEBUG
  fprintf (stderr, "in my_write_callback: buffer = %.*s\n", (int) n, buffer);
#endif

  /* Append the buffer's contents to the body.  */
  my_write_grow (l, n);
  memcpy (l->body + l->body_end, buffer, n);
  l->body_end += n;

  if (!l->is_error)
    {
      /* Process entire lines that are in the buffer.  */
      char *newline = (char *) memchr (l->body + l->body_end - n, '\n', n);
      while (newline != NULL)
        {
          /* We have an entire line.  */
          *newline = '\0';
          process_response_line (l->body + l->body_start, l->out_fd);
          l->body_start = (newline + 1) - l->body;

          newline = (char *) memchr (l->body + l->body_start, '\n',
                                     l->body_end - l->body_start);
        }
    }

  return n;
}

/* Make the HTTP POST request to the given URL, sending its output
   to the file descriptor FD.  */
static void
do_request (const char *url, const char *payload_as_string, int fd)
{
  if (curl_global_init (CURL_GLOBAL_DEFAULT))
    curl_die ();
  CURL *curl = curl_easy_init ();
  if (!curl)
    curl_die ();
  /* Documentation: <https://curl.se/libcurl/c/CURLOPT_URL.html>  */
  curl_easy_setopt (curl, CURLOPT_URL, url);

  /* Documentation: <https://curl.se/libcurl/c/CURLOPT_POST.html>  */
  curl_easy_setopt (curl, CURLOPT_POST, 1L);

  {
    struct curl_slist *headers = NULL;
    /* Override the Content-Type header set by CURLOPT_POST.  */
    headers = curl_slist_append(headers, "Content-Type: " "application/json");
    /* Documentation: <https://curl.se/libcurl/c/CURLOPT_HTTPHEADER.html>  */
    curl_easy_setopt (curl, CURLOPT_HTTPHEADER, headers);
  }

  /* Set the payload.
     Documentation: <https://curl.se/libcurl/c/CURLOPT_POSTFIELDS.html>  */
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_as_string);

  /* Documentation: <https://curl.se/libcurl/c/CURLOPT_NOPROGRESS.html>  */
  curl_easy_setopt (curl, CURLOPT_NOPROGRESS, 1L);

#if DEBUG > 1
  /* For debugging:  */
  /* Documentation: <https://curl.se/libcurl/c/CURLOPT_VERBOSE.html>  */
  curl_easy_setopt (curl, CURLOPT_VERBOSE, 1L);
#endif

#if 0
  /* Not reliable, see <https://curl.se/libcurl/c/CURLOPT_FAILONERROR.html>.  */
  curl_easy_setopt (curl, CURLOPT_FAILONERROR, 1L);
#endif

  struct my_write_locals locals;
  locals.out_fd = fd;
  locals.is_error = false;
  locals.body = NULL;
  locals.body_allocated = 0;
  locals.body_start = 0;
  locals.body_end = 0;

  /* Documentation:
     <https://curl.se/libcurl/c/CURLOPT_HEADERFUNCTION.html>
     <https://curl.se/libcurl/c/CURLOPT_HEADERDATA.html>  */
  curl_easy_setopt (curl, CURLOPT_HEADERFUNCTION, my_header_callback);
  curl_easy_setopt (curl, CURLOPT_HEADERDATA, &locals.is_error);

  /* Documentation:
     <https://curl.se/libcurl/c/CURLOPT_WRITEFUNCTION.html>
     <https://curl.se/libcurl/c/CURLOPT_WRITEDATA.html>  */
  curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, my_write_callback);
  curl_easy_setopt (curl, CURLOPT_WRITEDATA, &locals);

  /* Documentation: <https://curl.se/libcurl/c/curl_easy_perform.html>  */
  CURLcode ret = curl_easy_perform (curl);
  if (ret != CURLE_OK)
    error (EXIT_FAILURE, 0, _("curl error %u: %s"), ret,
           /* Documentation: <https://curl.se/libcurl/c/curl_easy_strerror.html>  */
           curl_easy_strerror (ret));

  /* Documentation: <https://curl.se/libcurl/c/CURLINFO_RESPONSE_CODE.html>  */
  long status_code;
  curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &status_code);
  if (status_code != 200)
    fprintf (stderr, "Status: %ld\n", status_code);
  if (locals.is_error != (status_code >= 400))
    /* The my_header_callback did not work right.  */
    abort ();
  if (status_code >= 400)
    {
      /* In this case, print the response body to stderr, not to fd.  */
      fprintf (stderr, "Body: ");
      fwrite (locals.body + locals.body_start,
              1, locals.body_end - locals.body_start,
              stderr);
      fprintf (stderr, "\n");
      exit (EXIT_FAILURE);
    }

  /* Most lines have already been processed through my_write_callback.
     Now process the last line (without terminating newline).  */
  if (locals.body_end > locals.body_start)
    {
      my_write_grow (&locals, 1);
      *(locals.body + locals.body_end) = '\0';
      process_response_line (locals.body + locals.body_start, locals.out_fd);
    }
}

/* Display usage information and exit.  */
static void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try '%s --help' for more information.\n"),
             program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION...]\n"),
              program_name);
      printf ("\n");
      printf (_("\
Passes standard input to a Large Language Model (LLM) instance and prints\n\
the response.\n\
With the %s option, it translates standard input to the specified language\n\
through a Large Language Model (LLM) and prints the translation.\n"),
              "--to");
      printf ("\n");
      printf (_("\
Warning: The output might not be what you expect.\n\
It might be of the wrong form, be of poor quality, or reflect some biases.\n"));
      printf ("\n");
      printf (_("\
Options:\n"));
      printf (_("\
      --species=TYPE          Specifies the type of LLM.  The default and only\n\
                              valid value is '%s'.\n"),
              "ollama");
      printf (_("\
      --url=URL               Specifies the URL of the server that runs the LLM.\n"));
      printf (_("\
  -m, --model=MODEL           Specifies the model to use.\n"));
      printf (_("\
      --to=LANGUAGE           Specifies the target language.\n"));
      printf (_("\
      --prompt=TEXT           Specifies the prompt to use before standard input.\n\
                              This option overrides the --to option.\n"));
      printf (_("\
      --postprocess=COMMAND   Specifies a command to post-process the output.\n"));
      printf ("\n");
      printf (_("\
Informative output:\n"));
      printf ("\n");
      printf (_("\
  -h, --help                  Display this help and exit.\n"));
      printf (_("\
  -V, --version               Output version information and exit.\n"));
      printf ("\n");
      /* TRANSLATORS: The first placeholder is the web address of the Savannah
         project of this package.  The second placeholder is the bug-reporting
         email address for this package.  Please add _another line_ saying
         "Report translation bugs to <...>\n" with the address for translation
         bugs (typically your translation team's web or email address).  */
      printf (_("\
Report bugs in the bug tracker at <%s>\n\
or by email to <%s>.\n"),
             "https://savannah.gnu.org/projects/gettext",
             "bug-gettext@gnu.org");
    }

  exit (status);
}

int
main (int argc, char **argv)
{
  /* Set program name for messages.  */
  set_program_name (argv[0]);

  /* Set locale via LC_ALL.  */
  setlocale (LC_ALL, "");

  /* Set the text message domain.  */
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  bindtextdomain ("gnulib", relocate (GNULIB_LOCALEDIR));
  textdomain (PACKAGE);

  /* Ensure that write errors on stdout are detected.  */
  atexit (close_stdout);

  /* Default values for command line options.  */
  bool do_help = false;
  bool do_version = false;
  const char *species = "ollama";
  const char *url = "http://localhost:11434";
  const char *model = NULL;
  const char *to_language = NULL;
  const char *prompt = NULL;
  const char *postprocess = NULL;

  /* Parse command line options.  */
  BEGIN_ALLOW_OMITTING_FIELD_INITIALIZERS
  static const struct program_option options[] =
  {
    { "help",        'h',          no_argument       },
    { "model",       'm',          required_argument },
    { "postprocess", CHAR_MAX + 5, required_argument },
    { "prompt",      CHAR_MAX + 4, required_argument },
    { "species",     CHAR_MAX + 1, required_argument },
    { "to",          CHAR_MAX + 3, required_argument },
    { "url",         CHAR_MAX + 2, required_argument },
    { "version",     'V',          no_argument       },
  };
  END_ALLOW_OMITTING_FIELD_INITIALIZERS
  start_options (argc, argv, options, MOVE_OPTIONS_FIRST, 0);
  {
    int opt;
    while ((opt = get_next_option ()) != -1)
      switch (opt)
        {
        case '\0':                /* Long option with key == 0.  */
          break;

        case 'h': /* --help */
          do_help = true;
          break;

        case 'V': /* --version */
          do_version = true;
          break;

        case CHAR_MAX + 1: /* --species */
          species = optarg;
          break;

        case CHAR_MAX + 2: /* --url */
          url = optarg;
          break;

        case 'm': /* --model */
          model = optarg;
          break;

        case CHAR_MAX + 3: /* --to */
          to_language = optarg;
          break;

        case CHAR_MAX + 4: /* --prompt */
          prompt = optarg;
          break;

        case CHAR_MAX + 5: /* --postprocess */
          postprocess = optarg;
          break;

        default:
          usage (EXIT_FAILURE);
          break;
        }
  }

  /* Version information is requested.  */
  if (do_version)
    {
      printf ("%s (GNU %s) %s\n", last_component (program_name),
              PACKAGE, VERSION);
      /* xgettext: no-wrap */
      printf (_("Copyright (C) %s Free Software Foundation, Inc.\n\
License GPLv3+: GNU GPL version 3 or later <%s>\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
"),
              "2025-2026", "https://gnu.org/licenses/gpl.html");
      printf (_("Written by %s.\n"), proper_name ("Bruno Haible"));
      exit (EXIT_SUCCESS);
    }

  /* Help is requested.  */
  if (do_help)
    usage (EXIT_SUCCESS);

  /* Test for extraneous arguments.  */
  if (optind != argc)
    error (EXIT_FAILURE, 0, _("too many arguments"));

  /* Check --species option.  */
  if (strcmp (species, "ollama") != 0)
    error (EXIT_FAILURE, 0, _("invalid value for %s option: %s"),
           "--species", species);

  /* Check --model option.  */
  if (model == NULL)
    error (EXIT_FAILURE, 0, _("missing %s option"),
           "--model");

  /* Sanitize URL.  */
  if (!(strlen (url) > 0 && url[strlen (url) - 1] == '/'))
    url = xasprintf ("%s/", url);

  /* Read the contents of standard input.  */
  errno = 0;
  size_t input_length;
  char *input = fread_file (stdin, 0, &input_length);
  if (input == NULL)
    error (EXIT_FAILURE, errno, _("error reading standard input"));

  /* Compute a default prompt.  */
  if (prompt == NULL && to_language != NULL)
    prompt = xasprintf ("Translate into %s:", language_in_english (to_language));

  /* Prepend the prompt.  */
  if (prompt != NULL)
    input = xasprintf ("%s\n%s", prompt, input);

  /* Documentation of the ollama API:
     <https://docs.ollama.com/api/generate>  */

  url = xasprintf ("%sapi/generate", url);

  /* Compose the payload.  */
  struct json_object *payload = json_object_new_object ();
  if (payload == NULL)
    xalloc_die ();
  {
    struct json_object *value = json_object_new_string (model);
    if (value == NULL)
      xalloc_die ();
    if (json_object_object_add (payload, "model", value))
      xalloc_die ();
  }
  {
    struct json_object *value = json_object_new_string (input);
    if (value == NULL)
      xalloc_die ();
    if (json_object_object_add (payload, "prompt", value))
      xalloc_die ();
  }
  const char *payload_as_string =
    json_object_to_json_string_ext (payload, JSON_C_TO_STRING_PLAIN
                                             | JSON_C_TO_STRING_NOSLASHESCAPE);
  if (payload_as_string == NULL)
    xalloc_die ();

  /* Make the request to the ollama server.  */
  if (postprocess != NULL)
    {
      /* Open a pipe to a subprocess.  */
      const char *sub_argv[4];
      sub_argv[0] = BOURNE_SHELL;
      sub_argv[1] = "-c";
      sub_argv[2] = postprocess;
      sub_argv[3] = NULL;
      int fd[1];
      pid_t child = create_pipe_out (BOURNE_SHELL, BOURNE_SHELL, sub_argv, NULL,
                                     NULL, NULL, false, true, true, fd);

      /* Ignore SIGPIPE here.  We don't care if the subprocesses terminates
         successfully without having read all of the input that we feed it.  */
      void (*orig_sigpipe_handler)(int);
      orig_sigpipe_handler = signal (SIGPIPE, SIG_IGN);

      do_request (url, payload_as_string, fd[0]);

      close (fd[0]);

      signal (SIGPIPE, orig_sigpipe_handler);

      /* Remove zombie process from process list, and retrieve exit status.  */
      int exitstatus =
        wait_subprocess (child, BOURNE_SHELL, true, false, true, true, NULL);

      return exitstatus;
    }
  else
    {
      do_request (url, payload_as_string, STDOUT_FILENO);

      return EXIT_SUCCESS;
    }
}

/*
 * Local Variables:
 * run-command: "echo 'Translate into German: "Welcome to the GNU project!"' | ./spit --model=ministral-3:14b"
 * End:
 */
