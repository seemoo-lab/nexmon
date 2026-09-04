/* stringutils.c - String helper functions.
 * Copyright (C) 1997, 2014 Werner Koch
 * Copyright (C) 2020 g10 Code GmbH
 *
 * This file is part of libgpg-error.
 *
 * libgpg-error is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * libgpg-error is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <config.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>
#ifdef HAVE_STAT
# include <sys/stat.h>
#endif
#include <sys/types.h>

#include "gpgrt-int.h"


char *
_gpgrt_trim_spaces (char *str)
{
  char *string, *p, *mark;

  string = str;
  /* Find first non space character. */
  for (p=string; *p && isspace (*(unsigned char*)p) ; p++)
    ;
  /* Move characters. */
  for ((mark = NULL); (*string = *p); string++, p++)
    if (isspace (*(unsigned char*)p))
      {
        if (!mark)
          mark = string;
      }
    else
      mark = NULL;
  if (mark)
    *mark = '\0' ;  /* Remove trailing spaces. */

  return str ;
}


/* Helper for _gpgrt_fnameconcat.  FLAGS controls special features of
 * the function.  See GPGRT_FCONCAT_ constants for details. */
char *
_gpgrt_vfnameconcat (unsigned int flags,
                     const char *first_part, va_list arg_ptr)
{
  const char *argv[32];
  int argc;
  size_t n, n0;
  int skip = 1;  /* Characters to skip from FIRST_PART.  Only used if
                  * HOME is NULL */
  char *home_buffer = NULL;
  char *name, *p;
  const char *home;
  const char *extradelim = "";

  /* Put all args into an array because we need to scan them twice.  */
  n = strlen (first_part) + 1;
  argc = 0;
  while ((argv[argc] = va_arg (arg_ptr, const char *)))
    {
      n += strlen (argv[argc]) + 1;
      if (argc >= DIM (argv)-1)
        {
          _gpg_err_set_errno (EINVAL);
          return NULL;
        }
      argc++;
    }
  n++;

  home = NULL;
  if ((flags & GPGRT_FCONCAT_SYSCONF))
    {
#ifdef HAVE_W32_SYSTEM
      home = _gpgrt_w32_get_sysconfdir ();
      if (!home)
        return NULL;
#else
      home = SYSCONFDIR;
#endif
      skip = 0;
      n0 = strlen (home);
      n += n0;
      if (n0 && home[n0-1] != '/' && *first_part != '/')
        {
          extradelim = "/";
          n++;
        }
    }
  else if (*first_part == '~' && (flags & GPGRT_FCONCAT_TILDE))
    {
      if (first_part[1] == '/' || !first_part[1])
        {
          /* This is the "~/" or "~" case.  */
#ifdef HAVE_W32_SYSTEM
          home = _gpgrt_w32_get_profile ();
          /* Note that we fallback to $HOME if the above failed.  */
#endif
          if (!home)
            {
              home_buffer = _gpgrt_getenv ("HOME");
              if (!home_buffer)
                home_buffer = _gpgrt_getpwdir (NULL);
              home = home_buffer;
            }
          if (home && *home)
            n += strlen (home);
        }
      else
        {
          /* This is the "~username/" or "~username" case.  */
          char *user;

          user = _gpgrt_strdup (first_part+1);
          if (!user)
            return NULL;

          p = strchr (user, '/');
          if (p)
            *p = 0;
          skip = 1 + strlen (user);

          home = home_buffer = _gpgrt_getpwdir (user);
          xfree (user);
          if (home)
            n += strlen (home);
          else
            skip = 1;
        }
    }

  name = xtrymalloc (n);
  if (!name)
    {
      _gpgrt_free (home_buffer);
      return NULL;
    }

  if (home)
    p = stpcpy (stpcpy (stpcpy (name, home), extradelim), first_part + skip);
  else
    p = stpcpy (name, first_part);

  xfree (home_buffer);
  home_buffer = NULL;

  for (argc=0; argv[argc]; argc++)
    {
      /* Avoid a leading double slash if the first part was "/".  */
      if (!argc && name[0] == '/' && !name[1])
        p = stpcpy (p, argv[argc]);
      else
        p = stpcpy (stpcpy (p, "/"), argv[argc]);
    }

  if ((flags & GPGRT_FCONCAT_ABS))
    {
#ifdef HAVE_W32_SYSTEM
      p = strchr (name, ':');
      if (p)
        p++;
      else
        p = name;
#else
      p = name;
#endif
      if (*p != '/'
#ifdef HAVE_W32_SYSTEM
          && *p != '\\'
#endif
          )
        {
          char *mycwd = _gpgrt_getcwd ();
          if (!mycwd)
            {
              xfree (name);
              return NULL;
            }

          n = strlen (mycwd) + 1 + strlen (name) + 1;
          home_buffer = xtrymalloc (n);
          if (!home_buffer)
            {
              xfree (mycwd);
              xfree (name);
              return NULL;
            }

          if (p == name)
            p = home_buffer;
          else /* Windows case.  */
            {
              memcpy (home_buffer, p, p - name + 1);
              p = home_buffer + (p - name + 1);
            }

          /* Avoid a leading double slash if the cwd is "/".  */
          if (mycwd[0] == '/' && !mycwd[1])
            strcpy (stpcpy (p, "/"), name);
          else
            strcpy (stpcpy (stpcpy (p, mycwd), "/"), name);

          xfree (mycwd);
          xfree (name);
          name = home_buffer;
          /* Let's do a simple compression to catch the common case of
           * a trailing "/.".  */
          n = strlen (name);
          if (n > 2 && name[n-2] == '/' && name[n-1] == '.')
            name[n-2] = 0;
        }
#ifdef HAVE_W32_SYSTEM
      else if (name[0] && name[1] != ':'
               && !(name[0] == '/' && name[1] == '/')
               && !(name[0] == '\\' && name[1] == '\\'))
        {
          /* Absolute name but no drive letter but also no UNC.  */
          char *mycwd = _gpgrt_getcwd ();
          if (!mycwd)
            {
              xfree (name);
              return NULL;
            }
          if (mycwd[0] && mycwd[1] == ':')
            {
              home_buffer = xtrymalloc (2 + strlen (name) + 1);
              if (!home_buffer)
                {
                  xfree (mycwd);
                  xfree (name);
                  return NULL;
                }
              home_buffer[0] = mycwd[0];
              home_buffer[1] = mycwd[1];
              strcpy (home_buffer + 2, name);
              xfree (name);
              name = home_buffer;
            }
          xfree (mycwd);
        }
#endif /* W32 */

#ifdef HAVE_W32_SYSTEM
      /* Fix "c://foo" to c:/foo".  */
      n = strlen (name);
      if (name[0] && name[1] == ':' && name[2] == '/' && name[3] == '/')
        memmove (name+3, name+4, strlen (name+4)+1);
#endif
    } /* end GPGRT_FCONCAT_ABS */

#ifdef HAVE_W32_SYSTEM
  for (p=name; *p; p++)
    if (*p == '\\')
      *p = '/';
#endif
  return name;
}


char *
_gpgrt_fconcat (unsigned int flags, const char *first_part, ... )
{
  va_list arg_ptr;
  char *result;

  va_start (arg_ptr, first_part);
  result = _gpgrt_vfnameconcat (flags, first_part, arg_ptr);
  va_end (arg_ptr);
  return result;
}


/* Construct a filename from the NULL terminated list of parts.  Tilde
 * expansion is done for the first argument.  The caller must release
 * the result using gpgrt_free; on error ERRNO is set and NULL
 * returned.  */
char *
_gpgrt_fnameconcat (const char *first_part, ... )
{
  va_list arg_ptr;
  char *result;

  va_start (arg_ptr, first_part);
  result = _gpgrt_vfnameconcat (GPGRT_FCONCAT_TILDE, first_part, arg_ptr);
  va_end (arg_ptr);
  return result;
}


/* Construct a filename from the NULL terminated list of parts.  Tilde
 * expansion is done for the first argument.  The caller must release
 * the result using gpgrt_free; on error ERRNO is set and NULL
 * returned.  This version returns an absolute filename. */
char *
_gpgrt_absfnameconcat (const char *first_part, ... )
{
  va_list arg_ptr;
  char *result;

  va_start (arg_ptr, first_part);
  result = _gpgrt_vfnameconcat (GPGRT_FCONCAT_TILDE| GPGRT_FCONCAT_ABS,
                                first_part, arg_ptr);
  va_end (arg_ptr);
  return result;
}
