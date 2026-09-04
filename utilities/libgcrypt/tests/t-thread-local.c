/* t-thread-local.c - Check the thread local storage
 * Copyright (C) 2024 g10 Code GmbH
 *
 * This file is part of Libgcrypt.
 *
 * Libgcrypt is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Libgcrypt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: LGPL-2.1+
 */

/* For now, this program simply test __thread storage class specifier.
 * After we implement thread local context for libgcrypt, we will
 * modity to test the feature.  */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#if HAVE_PTHREAD
# include <pthread.h>
#endif

#define PGM "t-thread-local"

#include "t-common.h"
#include "../src/gcrypt-testapi.h"

/* Mingw requires us to include windows.h after winsock2.h which is
   included by gcrypt.h.  */
#ifdef _WIN32
# include <windows.h>
#endif

#ifdef _WIN32
# define THREAD_RET_TYPE  DWORD WINAPI
# define THREAD_RET_VALUE 0
#else
# define THREAD_RET_TYPE  void *
# define THREAD_RET_VALUE NULL
#endif

#define N_TESTS 1

#define N_THREADS 19

static __thread unsigned long t;

struct thread_arg_s
{
  int no;
};

#if defined(HAVE_PTHREAD) || defined(_WIN32)
/* Checking the local storage thread.  */
static THREAD_RET_TYPE
check_ls_thread (void *argarg)
{
  struct thread_arg_s *arg = argarg;

  t = arg->no;
  info ("a thread update the local storage: %lu", t);

  gcry_free (arg);
  return THREAD_RET_VALUE;
}
#endif

static void
check_thread_local (void)
{
  struct thread_arg_s *arg;

#ifdef _WIN32
  HANDLE threads[N_THREADS];
  int i;
  int rc;

  t = N_THREADS;
  for (i=0; i < N_THREADS; i++)
    {
      arg = gcry_xmalloc (sizeof *arg);
      arg->no = i;
      threads[i] = CreateThread (NULL, 0, check_ls_thread, arg, 0, NULL);
      if (!threads[i])
        die ("error creating a thread %d: rc=%d",
             i, (int)GetLastError ());
    }

  for (i=0; i < N_THREADS; i++)
    {
      rc = WaitForSingleObject (threads[i], INFINITE);
      if (rc == WAIT_OBJECT_0)
        info ("a thread %d has terminated", i);
      else
        fail ("waiting for a thread %d failed: %d",
              i, (int)GetLastError ());
      CloseHandle (threads[i]);
    }

#elif HAVE_PTHREAD
  pthread_t threads[N_THREADS];
  int rc, i;

  t = N_THREADS;
  for (i=0; i < N_THREADS; i++)
    {
      arg = gcry_xmalloc (sizeof *arg);
      arg->no = i;
      pthread_create (&threads[i], NULL, check_ls_thread, arg);
    }

  for (i=0; i < N_THREADS; i++)
    {
      rc = pthread_join (threads[i], NULL);
      if (rc)
        fail ("pthread_join failed for a thread %d: %s",
              i, strerror (errno));
      else
        info ("a thread %d has terminated", i);
    }
#else
  (void)arg;
#endif /*!_WIN32*/
  if (t != N_THREADS)
    fail ("failed t=%lu\n", t);
  else
    info ("success");
}

int
main (int argc, char **argv)
{
  int last_argc = -1;

  if (argc)
    { argc--; argv++; }

  while (argc && last_argc != argc)
    {
      last_argc = argc;
      if (!strcmp (*argv, "--"))
        {
          argc--; argv++;
          break;
        }
      else if (!strcmp (*argv, "--help"))
        {
          fputs ("usage: " PGM " [options]\n"
                 "Options:\n"
                 "  --verbose       print timings etc.\n"
                 "  --debug         flyswatter\n",
                 stdout);
          exit (0);
        }
      else if (!strcmp (*argv, "--verbose"))
        {
          verbose++;
          argc--; argv++;
        }
      else if (!strcmp (*argv, "--debug"))
        {
          verbose += 2;
          debug++;
          argc--; argv++;
        }
      else if (!strncmp (*argv, "--", 2))
        die ("unknown option '%s'", *argv);
    }

  xgcry_control ((GCRYCTL_DISABLE_SECMEM, 0));
  if (!gcry_check_version (GCRYPT_VERSION))
    die ("version mismatch\n");
  if (debug)
    xgcry_control ((GCRYCTL_SET_DEBUG_FLAGS, 1u , 0));
  xgcry_control ((GCRYCTL_ENABLE_QUICK_RANDOM, 0));
  xgcry_control ((GCRYCTL_INITIALIZATION_FINISHED, 0));

  check_thread_local ();

  return !!error_count;
}
