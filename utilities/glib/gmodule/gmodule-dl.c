/* GMODULE - GLIB wrapper code for dynamic module loading
 * Copyright (C) 1998, 2000 Tim Janik
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/. 
 */

/* 
 * MT safe
 */
#include "config.h"

#include <dlfcn.h>
#include <glib.h>

#ifdef __CYGWIN__
#define CYGWIN_WORKAROUND
#endif

#ifdef CYGWIN_WORKAROUND
#include <sys/cygwin.h>
#include <windows.h>
#include <tlhelp32.h>

#include <errno.h>
#endif

/* Perl includes <nlist.h> and <link.h> instead of <dlfcn.h> on some systems? */


/* dlerror() is not implemented on all systems
 */
#ifndef	G_MODULE_HAVE_DLERROR
#  ifdef __NetBSD__
#    define dlerror()	g_strerror (errno)
#  else /* !__NetBSD__ */
/* could we rely on errno's state here? */
#    define dlerror()	"unknown dl-error"
#  endif /* !__NetBSD__ */
#endif	/* G_MODULE_HAVE_DLERROR */

/* some flags are missing on some systems, so we provide
 * harmless defaults.
 * The Perl sources say, RTLD_LAZY needs to be defined as (1),
 * at least for Solaris 1.
 *
 * Mandatory:
 * RTLD_LAZY   - resolve undefined symbols as code from the dynamic library
 *		 is executed.
 * RTLD_NOW    - resolve all undefined symbols before dlopen returns, and fail
 *		 if this cannot be done.
 * Optionally:
 * RTLD_GLOBAL - the external symbols defined in the library will be made
 *		 available to subsequently loaded libraries.
 */
#ifndef	HAVE_RTLD_LAZY
#define	RTLD_LAZY	1
#endif	/* RTLD_LAZY */
#ifndef	HAVE_RTLD_NOW
#define	RTLD_NOW	0
#endif	/* RTLD_NOW */
/* some systems (OSF1 V5.0) have broken RTLD_GLOBAL linkage */
#ifdef G_MODULE_BROKEN_RTLD_GLOBAL
#undef	RTLD_GLOBAL
#undef	HAVE_RTLD_GLOBAL
#endif /* G_MODULE_BROKEN_RTLD_GLOBAL */
#ifndef	HAVE_RTLD_GLOBAL
#define	RTLD_GLOBAL	0
#endif	/* RTLD_GLOBAL */


/* According to POSIX.1-2001, dlerror() is not necessarily thread-safe
 * (see https://pubs.opengroup.org/onlinepubs/009695399/), and so must be
 * called within the same locked section as the dlopen()/dlsym() call which
 * may have caused an error.
 *
 * However, some libc implementations, such as glibc, implement dlerror() using
 * thread-local storage, so are thread-safe. As of early 2021:
 *  - glibc is thread-safe: https://github.com/bminor/glibc/blob/HEAD/dlfcn/libc_dlerror_result.c
 *  - musl-libc is thread-safe since version 1.1.9 -- released in 2015: https://wiki.musl-libc.org/functional-differences-from-glibc.html#Thread-safety-of-%3Ccode%3Edlerror%3C/code%3E
 *  - uclibc-ng is not thread-safe: https://cgit.uclibc-ng.org/cgi/cgit/uclibc-ng.git/tree/ldso/libdl/libdl.c?id=132decd2a043d0ccf799f42bf89f3ae0c11e95d5#n1075
 *  - Other libc implementations have not been checked, and no problems have
 *    been reported with them in 10 years, so default to assuming that they
 *    don’t need additional thread-safety from GLib
 */
#if defined(__UCLIBC__)
G_LOCK_DEFINE_STATIC (errors);
#else
#define DLERROR_IS_THREADSAFE 1
#endif

static void
lock_dlerror (void)
{
#ifndef DLERROR_IS_THREADSAFE
  G_LOCK (errors);
#endif
}

static void
unlock_dlerror (void)
{
#ifndef DLERROR_IS_THREADSAFE
  G_UNLOCK (errors);
#endif
}

/* This should be called with lock_dlerror() held */
static const gchar *
fetch_dlerror (gboolean replace_null)
{
  const gchar *msg = dlerror ();

  /* make sure we always return an error message != NULL, if
   * expected to do so. */

  if (!msg && replace_null)
    return "unknown dl-error";

  return msg;
}

static gpointer
_g_module_open (const gchar *file_name,
		gboolean     bind_lazy,
		gboolean     bind_local,
                GError     **error)
{
  gpointer handle;
  
  lock_dlerror ();
  handle = dlopen (file_name,
                   (bind_local ? RTLD_LOCAL : RTLD_GLOBAL) | (bind_lazy ? RTLD_LAZY : RTLD_NOW));
  if (!handle)
    {
      const gchar *message = fetch_dlerror (TRUE);

      g_module_set_error (message);
      g_set_error_literal (error, G_MODULE_ERROR, G_MODULE_ERROR_FAILED, message);
    }

  unlock_dlerror ();
  
  return handle;
}

static gpointer
_g_module_self (void)
{
  gpointer handle;
  
  /* to query symbols from the program itself, special link options
   * are required on some systems.
   */

  /* On Android 32 bit (i.e. not __LP64__), dlopen(NULL)
   * does not work reliable and generally no symbols are found
   * at all. RTLD_DEFAULT works though.
   * On Android 64 bit, dlopen(NULL) seems to work but dlsym(handle)
   * always returns 'undefined symbol'. Only if RTLD_DEFAULT or 
   * NULL is given, dlsym returns an appropriate pointer.
   */
  lock_dlerror ();
#if defined(__ANDROID__) || defined(__NetBSD__)
  handle = RTLD_DEFAULT;
#else
  handle = dlopen (NULL, RTLD_GLOBAL | RTLD_LAZY);
#endif
  if (!handle)
    g_module_set_error (fetch_dlerror (TRUE));
  unlock_dlerror ();
  
  return handle;
}

static void
_g_module_close (gpointer handle)
{
#if defined(__ANDROID__) || defined(__NetBSD__)
  if (handle != RTLD_DEFAULT)
#endif
    {
      lock_dlerror ();
      if (dlclose (handle) != 0)
        g_module_set_error (fetch_dlerror (TRUE));
      unlock_dlerror ();
    }
}

#ifdef CYGWIN_WORKAROUND

static gpointer
find_in_any_module_cygwin (const char *symbol_name)
{
  HANDLE snapshot;
  MODULEENTRY32 entry = {
    .dwSize = sizeof (entry),
  };
  gpointer p = NULL;

retry:
  snapshot = CreateToolhelp32Snapshot (TH32CS_SNAPMODULE, 0);
  if (snapshot == INVALID_HANDLE_VALUE)
    {
      DWORD code = GetLastError ();
      if (code == ERROR_BAD_LENGTH)
        {
          /* Probably only happens when inspecting other processes */
          g_thread_yield ();
          goto retry;
        }
      else
        {
          g_warning ("%s failed with error code %u",
                     "CreateToolhelp32Snapshot",
                     (unsigned int) code);
          return NULL;
        }
    }

  if (Module32First (snapshot, &entry))
    {
      do
        {
          if ((p = GetProcAddress (entry.hModule, symbol_name)) != NULL)
            {
              ssize_t size = 0;
              char *posix_path;

              G_STATIC_ASSERT (G_N_ELEMENTS (entry.szExePath) <= MAX_PATH);

              do
                {
                  posix_path = (size > 0) ? g_alloca ((long unsigned int) size) : NULL;
                  size = cygwin_conv_path (CCP_WIN_W_TO_POSIX, entry.szExePath, posix_path, (size_t) size);
                }
              while (size > 0);

              if (size < 0)
                {
                  g_warning ("%s failed with errno %d", "cygwin_conv_path", errno);
                  break;
                }

              if (g_str_has_prefix (posix_path, "/usr/lib") ||
                  g_str_has_prefix (posix_path, "/usr/local/lib"))
                {
                  p = NULL;
                }
              else
                break;
            }
        }
      while (Module32Next (snapshot, &entry));
    }

  CloseHandle (snapshot);

  return p;
}

#endif

static gpointer
_g_module_symbol (gpointer     handle,
		  const gchar *symbol_name)
{
  gpointer p;
  const gchar *msg;

  lock_dlerror ();
  fetch_dlerror (FALSE);
  p = dlsym (handle, symbol_name);
  msg = fetch_dlerror (FALSE);
#ifndef CYGWIN_WORKAROUND
  if (msg)
    g_module_set_error (msg);
  unlock_dlerror ();
#else
  if (msg)
    {
      char *msg_copy = g_strdup (msg);
      unlock_dlerror ();
      p = find_in_any_module_cygwin (symbol_name);
      if (!p)
        g_module_set_error_unduped (msg_copy);
      else
        g_free (msg_copy);
    }
#endif

  return p;
}
