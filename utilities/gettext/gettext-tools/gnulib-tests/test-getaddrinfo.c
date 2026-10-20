/* Test the getaddrinfo module.

   Copyright (C) 2006-2026 Free Software Foundation, Inc.

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

/* Written by Simon Josefsson.  */

#include <config.h>

#include <netdb.h>

#include "signature.h"
SIGNATURE_CHECK (gai_strerror, char const *, (int));
/* On native Windows, these two functions may have the __stdcall calling
   convention.  But the SIGNATURE_CHECK works only for functions with __cdecl
   calling convention.  */
#if !(defined _WIN32 && !defined __CYGWIN__)
SIGNATURE_CHECK (freeaddrinfo, void, (struct addrinfo *));
SIGNATURE_CHECK (getaddrinfo, int, (char const *, char const *,
                                    struct addrinfo const *,
                                    struct addrinfo **));
#endif

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

#include "sockets.h"

/* Whether to print debugging messages.  */
#define ENABLE_DEBUGGING 0

#if ENABLE_DEBUGGING
# define dbgprintf printf
#else
# define dbgprintf if (0) printf
#endif

/* BeOS does not have AF_UNSPEC.  */
#ifndef AF_UNSPEC
# define AF_UNSPEC 0
#endif

#ifndef EAI_SERVICE
# define EAI_SERVICE 0
#endif

static int
simple (int pass, char const *host, char const *service)
{
  char buf[BUFSIZ];
  static int skip = 0;
  struct addrinfo hints;
  struct addrinfo *hints_p;
  struct addrinfo *ai0;
  int res;
  int err;

  /* Once we skipped the test, do not try anything else */
  if (skip)
    return 0;

  dbgprintf ("Finding %s service %s...\n", host, service);

  if (pass == 1)
    hints_p = NULL;
  else
    {
      memset (&hints, 0, sizeof (hints));
      hints.ai_flags = AI_CANONNAME
                       | (pass == 3 ? AI_NUMERICHOST : 0)
                       | (pass == 4 ? AI_NUMERICSERV : 0);
      hints.ai_family = AF_UNSPEC;
      hints.ai_socktype = SOCK_STREAM;
      hints_p = &hints;
    }

  res = getaddrinfo (host, service, hints_p, &ai0);
  err = errno;

  dbgprintf ("res %d: %s\n", res, gai_strerror (res));

  if ((pass == 3 && ! isdigit (host[0]))
      || (pass == 4 && ! isdigit (service[0])))
    {
      if (res != EAI_NONAME)
        {
          fprintf (stderr,
                   "Test case pass=%d, host=%s, service=%s failed: "
                   "expected EAI_NONAME, got %d\n",
                   pass, host, service, res);
          return 1;
        }
      return 0;
    }

  if (res != 0)
    {
      /* EAI_AGAIN is returned if no network is available. Don't fail
         the test merely because someone is down the country on their
         in-law's farm. */
      if (res == EAI_AGAIN)
        {
          skip++;
          fprintf (stderr, "skipping getaddrinfo test: no network?\n");
          return 77;
        }
      /* Solaris reports EAI_SERVICE for "http" and "https".  Don't
         fail the test merely because of this.  */
      if (res == EAI_SERVICE)
        return 0;
#ifdef EAI_NODATA
      /* AIX reports EAI_NODATA for "https".  Don't fail the test
         merely because of this.  */
      if (res == EAI_NODATA)
        return 0;
#endif
      /* Provide details if errno was set.  */
      if (res == EAI_SYSTEM)
        fprintf (stderr, "system error: %s\n", strerror (err));

      fprintf (stderr,
               "Test case pass=%d, host=%s, service=%s failed: "
               "expected 0, got %d\n",
               pass, host, service, res);
      return 1;
    }

  for (struct addrinfo *ai = ai0; ai; ai = ai->ai_next)
    {
      void *ai_addr = ai->ai_addr;
      struct sockaddr_in *sock_addr = ai_addr;
      dbgprintf ("\tflags %x\n", ai->ai_flags + 0u);
      dbgprintf ("\tfamily %x\n", ai->ai_family + 0u);
      dbgprintf ("\tsocktype %x\n", ai->ai_socktype + 0u);
      dbgprintf ("\tprotocol %x\n", ai->ai_protocol + 0u);
      dbgprintf ("\taddrlen %lu: ", (unsigned long) ai->ai_addrlen);
      dbgprintf ("\tFound %s\n",
                 inet_ntop (ai->ai_family,
                            &sock_addr->sin_addr,
                            buf, sizeof (buf) - 1));
      if (ai->ai_canonname)
        dbgprintf ("\tFound %s...\n", ai->ai_canonname);

      {
        char ipbuf[BUFSIZ];
        char portbuf[BUFSIZ];

        res = getnameinfo (ai->ai_addr, ai->ai_addrlen,
                           ipbuf, sizeof (ipbuf) - 1,
                           portbuf, sizeof (portbuf) - 1,
                           NI_NUMERICHOST|NI_NUMERICSERV);
        dbgprintf ("\t\tgetnameinfo %d: %s\n", res, gai_strerror (res));
        if (res == 0)
          {
            dbgprintf ("\t\tip %s\n", ipbuf);
            dbgprintf ("\t\tport %s\n", portbuf);
          }
      }

    }

  freeaddrinfo (ai0);

  return 0;
}

#define HOST1 "www.gnu.org"
#define SERV1 "http"
#define HOST2 "www.ibm.com"
#define SERV2 "https"
#define HOST3 "microsoft.com"
#define SERV3 "http"
#define HOST4 "google.org"
#define SERV4 "ldap"
#if HAVE_IPV4
# define NUMERICHOSTV4 "1.2.3.4"
#endif
#if HAVE_IPV6
# define NUMERICHOSTV6 "2001:db8:3333:4444:CCCC:DDDD:EEEE:FFFF"
#endif

int main (void)
{
  (void) gl_sockets_startup (SOCKETS_1_1);

  return (  simple (1, HOST1, SERV1)
          + simple (1, HOST1, "80")
          + simple (1, HOST2, SERV2)
          + simple (1, HOST2, "443")
          + simple (1, HOST3, SERV3)
          + simple (1, HOST3, "80")
          + simple (1, HOST4, SERV4)
          + simple (1, HOST4, "389")
          + simple (2, HOST1, SERV1)
          + simple (2, HOST2, SERV2)
          + simple (2, HOST3, SERV3)
          + simple (2, HOST4, SERV4)
#if HAVE_IPV4
          + simple (3, NUMERICHOSTV4, SERV1)
#endif
#if HAVE_IPV6
          + simple (3, NUMERICHOSTV6, SERV1)
#endif
#if !defined __GLIBC__
          /* avoid glibc bug, possibly
             <https://sourceware.org/PR32465> */
          + simple (3, HOST1, SERV1)
          + simple (3, HOST2, SERV2)
          + simple (3, HOST3, SERV3)
          + simple (3, HOST4, SERV4)
#endif
          + simple (4, HOST1, SERV1)
          + simple (4, HOST1, "80")
          + simple (4, HOST2, SERV2)
          + simple (4, HOST2, "443")
          + simple (4, HOST3, SERV3)
          + simple (4, HOST3, "80")
          + simple (4, HOST4, SERV4)
          + simple (4, HOST4, "389"));
}
