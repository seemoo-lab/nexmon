/* Example for use of GNU gettext.
   This file is in the public domain.

   Source code of the C program.  */

/* This example implements a simple multithreaded web server.

   In order to get translations via gettext(), a locale must be installed on
   the server system for each language that should be served.  For example,
   in order to get French translations, you need to install the fr_FR.UTF-8
   locale.  You find the list of locales below and the installation instructions
   in the INSTALL file.
   This may seem strange to people who think "why is this necessary? why should
   reading a .mo file need a locale?"  The rationale is that servers who produce
   French text for a web page most often also need French number formatting,
   French sorting (for lists and UI elements), etc. — and these functionalities
   rely on the locale.

   Since the server is multithreaded, different requests may be served in
   different threads.  And different requests can come from different users,
   that have declared different language preferences in their web browser.
   Therefore, while at a certain moment one thread may produce a French
   translation (and thus work with a French locale), another thread may be
   producing a Spanish translation (and thus work with a Spanish locale)
   at the same time.
   Using the global locale (via setlocale()) would require locking, so that
   different threads don't influence each other; but this would severely limit
   the possible throughput of the server (which is the motivation for making
   the server multithreaded in the first place).
   Therefore the server does not use setlocale(), but instead works with
   locale_t objects, that can become the "current locale" of a thread, via
   uselocale().

   While it would be possible to allocate the locale_t objects lazily (upon
   the first request that needs the particular locale), here we allocate
   them all up-front, so that the response time for a given request is fast
   and so that there is no contention between the threads.  */


/* Persuade glibc to declare asprintf().  */
#define _GNU_SOURCE 1

/* Get textdomain(), bindtextdomain(), gettext() declarations.  */
#include <libintl.h>

/* Get locale_t, newlocale(), uselocale() declarations.  */
#include <locale.h>

/* Get pthread_create(), pthread_join() declarations.  */
#include <pthread.h>

/* Get asprintf(), dprintf() declarations.  */
#include <stdio.h>

/* Get abort(), free() declarations.  */
#include <stdlib.h>

/* Get memset(), strchr(), strcasecmp(), strncasecmp() declarations.  */
#include <string.h>

/* Get nanosleep().  */
#include <time.h>

/* Get close() declaration.  */
#include <unistd.h>

/* Get socket(), setsockopt(), bind(), listen(), accept(), recv() declarations.  */
#include <sys/socket.h>

/* Get IPPROTO_TCP, INADDR_ANY, in6addr_any.  */
#include <netinet/in.h>


/* Mapping from language to locale.  */
struct language_support
{
  const char *language;
  const char *locale_name;
  locale_t locale; /* NULL when the locale is not installed on the system */
};
static struct language_support all_languages[] =
{
  /* The locale names here must all be UTF-8 locales.  */
  { "af", "af_ZA.UTF-8" },
  { "ast", "ast_ES.UTF-8" },
  { "bg", "bg_BG.UTF-8" },
  { "ca", "ca_ES.UTF-8" },
  { "cs", "cs_CZ.UTF-8" },
  { "da", "da_DK.UTF-8" },
  { "de", "de_DE.UTF-8" },
  { "el", "el_GR.UTF-8" },
  { "en", "en_US.UTF-8" },
  { "eo", "eo" },
  { "es", "es_ES.UTF-8" },
  { "fi", "fi_FI.UTF-8" },
  { "fr", "fr_FR.UTF-8" },
  { "ga", "ga_IE.UTF-8" },
  { "gl", "gl_ES.UTF-8" },
  { "hr", "hr_HR.UTF-8" },
  { "hu", "hu_HU.UTF-8" },
  { "id", "id_ID.UTF-8" },
  { "it", "it_IT.UTF-8" },
  { "ja", "ja_JP.UTF-8" },
  { "ka", "ka_GE.UTF-8" },
  { "ky", "ky_KG" },
  { "lv", "lv_LV.UTF-8" },
  { "ms", "ms_MY.UTF-8" },
  { "mt", "mt_MT.UTF-8" },
  { "nb", "nb_NO.UTF-8" },
  { "nl", "nl_NL.UTF-8" },
  { "nn", "nn_NO.UTF-8" },
  { "pl", "pl_PL.UTF-8" },
  { "pt", "pt_PT.UTF-8" },
  { "pt_BR", "pt_BR.UTF-8" },
  { "ro", "ro_RO.UTF-8" },
  { "ru", "ru_RU.UTF-8" },
  { "sk", "sk_SK.UTF-8" },
  { "sl", "sl_SI.UTF-8" },
  { "sq", "sq_AL.UTF-8" },
  { "sr", "sr_RS" },
  { "sv", "sv_SE.UTF-8" },
  { "ta", "ta_IN" },
  { "tr", "tr_TR.UTF-8" },
  { "uk", "uk_UA.UTF-8" },
  { "vi", "vi_VN" },
  { "zh_CN", "zh_CN.UTF-8" },
  { "zh_HK", "zh_HK.UTF-8" },
  { "zh_TW", "zh_TW.UTF-8" }
};

/* Get the locale that exactly matches a given language.  */
static locale_t
get_locale_from_language (const char *language)
{
  size_t i;
  for (i = 0; i < sizeof (all_languages) / sizeof (all_languages[0]); i++)
    if (strcasecmp (all_languages[i].language, language) == 0)
      return all_languages[i].locale;
  return NULL;
}

/* Get the locale that can be used for a given language.  */
static locale_t
get_locale_matching_language (char *language)
{
  /* Convert '-' to '_'.  */
  char *dash = strchr (language, '-');
  if (dash != NULL)
    *dash = '_';

  locale_t result = get_locale_from_language (language);
  if (result == NULL && dash != NULL)
    {
      /* Truncate the language at the dash's position.  */
      *dash = '\0';
      result = get_locale_from_language (language);
    }

  /* Restore language.  */
  if (dash != NULL)
    *dash = '-';

  return result;
}

/* Get the locale for an 'Accept-Language' request header field element.  */
static locale_t
get_locale_matching_element (char *element_start, char *element_end)
{
  char *p;

  /* Ignore the element part that starts with a semicolon.  */
  for (p = element_start; p < element_end && *p != ';'; p++)
    ;
  element_end = p;

  /* Trim the element.  */
  while (element_start < element_end && element_end[-1] == ' ')
    element_end--;
  while (element_start < element_end && element_start[0] == ' ')
    element_start++;
  if (element_start == element_end)
    return NULL;

  char saved = *element_end;
  *element_end = '\0';
  locale_t result = get_locale_matching_language (element_start);
  *element_end = saved;

  return result;
}

/* Get the locale for an 'Accept-Language' request header field.  */
static locale_t
get_locale_matching_field (char *field_start, char *field_end)
{
  /* The field's value is a comma-separated list of "lang [; q=...]" elements.
     Each lang is of the form ll-CC, not a BCP 47 string.
     Therefore, for Chinese, expect zh-CN, zh-TW, etc.  See
     <https://stackoverflow.com/questions/69709824/>  */
  char *element_start = field_start;
  for (;;)
    {
      char *p;

      for (p = element_start; p < field_end && *p != ','; p++)
        ;
      char *element_end = p;

      locale_t locale_for_element =
        get_locale_matching_element (element_start, element_end);
      /* If the locale is not supported on this system, skip this element and
         continue with the next one.  */
      if (locale_for_element != NULL)
        return locale_for_element;

      if (element_end == field_end)
        break;
      element_start = element_end + 1;
    }
  return NULL;
}

/* Extract the desired locale from the value of the 'Accept-Language'
   request header field.  */
static locale_t
get_locale_from_header (char *header_start, char *header_end)
{
  char *line_start = header_start;
  while (line_start < header_end)
    {
      char *line_end = strchr (line_start, '\r');
      if (line_end == NULL)
        abort ();
      if (line_end - line_start >= 16
          && strncasecmp (line_start, "Accept-Language:", 16) == 0)
        {
          char *field_start = line_start + 16;
          char *field_end = line_end;
          return get_locale_matching_field (field_start, field_end);
        }
      line_start = line_end + 2;
    }
  return NULL;
}


/* This function defines what each thread does.  */

static void *
server_thread (void *arg)
{
  int server_socket = *(int const *) arg;
  enum { BUFFER_SIZE = 4096 };

  for (;;)
    {
      /* Accept an incoming connection.  */
      struct sockaddr_storage addr;
      socklen_t addrlen = sizeof (addr);
      int connected_socket =
        accept (server_socket, (struct sockaddr *) &addr, &addrlen);
      if (connected_socket >= 0)
        {
          /* Receive the initial part of an HTTP request.  */
          char request[BUFFER_SIZE + 1];
          int req_len = recv (connected_socket, request, BUFFER_SIZE, 0);
          if (req_len >= 0)
            {
              /* Determine the extent of the HTTP request header.
                 We are not interested in the message body.  */
              char *header_start;
              char *header_end;
              {
                request[req_len] = '\0';
                char *line_start = request;
                char *line_end = strchr (line_start, '\r');
                if (line_end != NULL && line_end[1] == '\n')
                  {
                    header_start = line_start = line_end + 2;
                    for (;;)
                      {
                        line_end = strchr (line_start, '\r');
                        if (!(line_end != NULL && line_end[1] == '\n'
                              && line_end > line_start))
                          /* An empty line ends the header and starts the body.  */
                          break;
                        line_start = line_end + 2;
                      }
                    header_end = line_start;
                  }
                else
                  header_start = header_end = request;
              }
              /* Determine the locale.  */
              locale_t locale =
                get_locale_from_header (header_start, header_end);
              /* Set the locale on this thread.
                 If locale == NULL, we use the thread's default locale, which
                 is the global locale, which is "C".  */
              if (locale != NULL)
                uselocale (locale);

              /* Get the localized HTTP response body.  */
              char *response_body;
              /* Some HTML could be added here.  */
              if (asprintf (&response_body, "%s\n", gettext ("Hello, world!")) >= 0)
                {
                  /* Writing to the connected_socket via send() is the same as
                     via write().  So, we can use dprintf().  Alternatively,
                     one could use fdopen() and fprintf().  */
                  dprintf (connected_socket,
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/plain; charset=UTF-8\r\n"
                           "Content-Length: %lu\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "%s",
                           (unsigned long) strlen (response_body),
                           response_body);
                  free (response_body);
                }

              /* Restore the previous locale.  */
              if (locale != NULL)
                uselocale (LC_GLOBAL_LOCALE);
            }
          close (connected_socket);

          /* Enable this to ensure that different threads get actually used.  */
          if (0)
            {
              struct timespec duration = { .tv_sec = 60, .tv_nsec = 0 };
              nanosleep (&duration, NULL);
            }
        }
    }
  return NULL;
}


/* Main program.  */

#define PORT 8080

/* The IPv4 server socket.  */
int server_socket4;
/* The IPv6 server socket.  */
int server_socket6;

/* Number of threads per socket.  */
#define NUM_THREADS 10

int
main ()
{
  textdomain ("hello-c-http");
  bindtextdomain ("hello-c-http", LOCALEDIR);

  /* Initialize all_languages.  */
  unsigned int i;
  for (i = 0; i < sizeof (all_languages) / sizeof (all_languages[0]); i++)
    all_languages[i].locale =
      newlocale (LC_ALL_MASK, all_languages[i].locale_name, NULL);

  /* Initialize an IPv4 server socket.  */
  {
    server_socket4 = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket4 < 0)
      return 1;

    /* Avoid an EADDRINUSE error in the next bind() call.  */
    unsigned int flag = 1;
    setsockopt (server_socket4, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (flag));

    struct sockaddr_in server_addr;
    memset (&server_addr, 0, sizeof (server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons (PORT);
    if (bind (server_socket4, (struct sockaddr *) &server_addr, sizeof (server_addr)) < 0)
      return 2;

    if (listen (server_socket4, 10) < 0)
      return 3;
  }

  /* Initialize an IPv6 server socket.  */
  {
    server_socket6 = socket (PF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket6 >= 0)
      {
        /* Avoid an EADDRINUSE error in the next bind() call.  */
        unsigned int flag = 1;
        setsockopt (server_socket6, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (flag));
        /* We don't want dual-socket support here.  */
        setsockopt (server_socket6, IPPROTO_IPV6, IPV6_V6ONLY, &flag, sizeof (flag));

        struct sockaddr_in6 server_addr;
        memset (&server_addr, 0, sizeof (server_addr));
        server_addr.sin6_family = AF_INET6;
        server_addr.sin6_addr = in6addr_any;
        server_addr.sin6_port = htons (PORT);
        if (bind (server_socket6, (struct sockaddr *) &server_addr, sizeof (server_addr)) < 0)
          return 4;

        if (listen (server_socket6, 10) < 0)
          return 5;
      }
  }

  printf ("Server is listening on port %d...\n", PORT);

  pthread_t thread;
  for (i = 0; i < NUM_THREADS; i++)
    {
      if (pthread_create (&thread, NULL, server_thread, &server_socket4) < 0)
        return 6;
    }
  if (server_socket6 >= 0)
    for (i = 0; i < NUM_THREADS; i++)
      {
        if (pthread_create (&thread, NULL, server_thread, &server_socket6) < 0)
          return 6;
      }

  /* Wait forever.  */
  pthread_join (thread, NULL);
}
