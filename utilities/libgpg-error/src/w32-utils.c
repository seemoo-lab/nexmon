/* w32-utils.c - Further Windows utility functions.
 * Copyright (C) 2025 g10 Code GmbH
 *
 * This file is part of Libgpg-error.
 *
 * Libgpg-error is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Libgpg-error is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: LGPL-2.1+
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef HAVE_W32_SYSTEM
# error This module may only be build for Windows.
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <gpg-error.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#ifndef CSIDL_APPDATA
#define CSIDL_APPDATA 0x001a
#endif
#ifndef CSIDL_LOCAL_APPDATA
#define CSIDL_LOCAL_APPDATA 0x001c
#endif
#ifndef CSIDL_COMMON_APPDATA
#define CSIDL_COMMON_APPDATA 0x0023
#endif
#ifndef CSIDL_PROFILE
#define CSIDL_PROFILE 0x0028
#endif
#ifndef CSIDL_FLAG_CREATE
#define CSIDL_FLAG_CREATE 0x8000
#endif

#include "gpgrt-int.h"


/* This is a helper function to load and call and the Windows Shell
 * function SHGetFolderPathW and return the value as UTF-8.  On error
 * NULL is returned and ERRNO is set. */
static char *
w32_shgetfolderpath (HWND a, int b, HANDLE c, DWORD d)
{
  static int initialized;
  static HRESULT (WINAPI * func)(HWND,int,HANDLE,DWORD,LPWSTR);
  wchar_t wfname[MAX_PATH];
  char *buf, *p;

  if (!initialized)
    {
      void *handle;

      handle = LoadLibraryEx ("shell32.dll", NULL, 0);
      if (!handle)
        {
          _gpgrt_w32_set_errno (-1);
          return NULL;
        }
      func = (void*)GetProcAddress (handle, "SHGetFolderPathW");
      if (!func)
        {
          _gpgrt_w32_set_errno (-1);
          CloseHandle (handle);
          handle = NULL;
          return NULL;
        }

      initialized = 1;
    }

  /* We call FUNC only if B != 0 so that it is possible to call this
   * function just for initializing.  */
  if (b && func (a,b,c,d,wfname) >= 0)
    {
      buf = _gpgrt_wchar_to_utf8 (wfname, -1);
      if (buf)
        {
          for (p=buf; *p; p++)
            if (*p == '\\')
              *p = '/';
        }
      return buf;
    }
  else
    return NULL;
}


const char *
_gpgrt_w32_get_sysconfdir (void)
{
  static char *dir;

  if (!dir)
    {
      char *tmp = w32_shgetfolderpath (NULL, CSIDL_COMMON_APPDATA, NULL, 0);
      if (!tmp)
        return NULL;
      dir = _gpgrt_strconcat (tmp, "/GNU/etc", NULL);
      xfree (tmp);
      if (dir)
        gpgrt_annotate_leaked_object (dir);
    }

  return dir;
}


const char *
_gpgrt_w32_get_profile (void)
{
  static char *dir;

  if (!dir)
    {
      dir = w32_shgetfolderpath (NULL, CSIDL_PROFILE, NULL, 0);
      if (dir)
        gpgrt_annotate_leaked_object (dir);
    }

  return dir;
}



/* Constructor for this module.  This can only be used if we are a
 * DLL.  If used as a static lib we can't control the process set; for
 * example it might be used with a main module which is not build with
 * mingw and thus does not know how to call the constructors.  */
#ifdef DLL_EXPORT
static void w32_utils_init (void) _GPG_ERR_CONSTRUCTOR;
#endif

static void
w32_utils_init (void)
{
  w32_shgetfolderpath (NULL, 0, NULL, 0);
}

#if !defined(DLL_EXPORT) || !defined(_GPG_ERR_HAVE_CONSTRUCTOR)
void
_gpgrt_w32__init_utils (void)
{
  w32_utils_init ();
}
#endif
