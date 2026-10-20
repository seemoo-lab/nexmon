/* GLIB - Library of useful routines for C programming
 *
 * Copyright (C) 2020 Red Hat, Inc.
 *
 * Author: Jakub Jelen <jjelen@redhat.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <dlfcn.h>
#include <glib.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include "config.h"

#ifndef __APPLE__

#define GET_REAL(func) \
  (real_##func = dlsym (RTLD_NEXT, #func))

#define DEFINE_WRAPPER(return_type, func, argument_list) \
  static return_type (* real_##func) argument_list;      \
  return_type func argument_list

#else

#define GET_REAL(func) \
  func

/* https://opensource.apple.com/source/dyld/dyld-852.2/include/mach-o/dyld-interposing.h */
#define DYLD_INTERPOSE(_replacement,_replacee) \
   __attribute__((used)) static struct{ const void* replacement; const void* replacee; } _interpose_##_replacee \
            __attribute__ ((section ("__DATA,__interpose"))) = { (const void*)(unsigned long)&_replacement, (const void*)(unsigned long)&_replacee };

#define DEFINE_WRAPPER(return_type, func, argument_list) \
  static return_type wrap_##func argument_list;          \
  DYLD_INTERPOSE(wrap_##func, func)                      \
  static return_type wrap_##func argument_list

#endif /* __APPLE__ */

/* This is used in gutils.c used to make sure utility functions
 * handling user information do not crash on bad data (for example
 * caused by getpwuid returning some NULL elements.
 */

#undef getpwuid

/* Only modify the result if running inside a GLib test. Otherwise, we risk
 * breaking test harness code, or test wrapper processes (as this binary is
 * loaded via LD_PRELOAD, it will affect all wrapper processes). */
static inline int
should_modify_result (void)
{
  const char *path = g_test_get_path ();
  return (path != NULL && *path != '\0');
}

static struct passwd my_pw;

DEFINE_WRAPPER (struct passwd *, getpwuid, (uid_t uid))
{
  struct passwd *pw = GET_REAL (getpwuid) (uid);
  my_pw = *pw;
  if (should_modify_result ())
    my_pw.pw_name = NULL;
  return &my_pw;
}

#ifdef HAVE_GETPWNAM_R
DEFINE_WRAPPER (int, getpwnam_r, (const char     *name,
                                  struct passwd  *pwd,
                                  char            buf[],
                                  size_t          buflen,
                                  struct passwd **result))
{
  int code = GET_REAL (getpwnam_r) (name, pwd, buf, buflen, result);
  if (should_modify_result ())
    pwd->pw_name = NULL;
  return code;
}
#endif

#ifdef HAVE_GETPWUID_R
DEFINE_WRAPPER (int, getpwuid_r, (uid_t           uid,
                                  struct passwd  *restrict pwd,
                                  char            buf[],
                                  size_t          buflen,
                                  struct passwd **result))
{
  int code = GET_REAL (getpwuid_r) (uid, pwd, buf, buflen, result);
  if (should_modify_result ())
    pwd->pw_name = NULL;
  return code;
}
#endif
