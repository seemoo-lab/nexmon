/* sysutils.c - Platform specific helper functions
 * Copyright (C) 2017 g10 Code GmbH
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include <config.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#ifdef HAVE_W32_SYSTEM
# include <windows.h>
# include <wchar.h>
#endif
#ifdef HAVE_STAT
# include <sys/stat.h>
#endif
#include <sys/types.h>
#include <fcntl.h>
#ifdef HAVE_PWD_H
# include <pwd.h>
#endif

#include "gpgrt-int.h"


/* Return true if FD is valid.  */
int
_gpgrt_fd_valid_p (int fd)
{
  int d = dup (fd);
  if (d < 0)
    return 0;
  close (d);
  return 1;
}


/* Our variant of getenv.  The returned string must be freed.  If the
 * environment variable does not exists NULL is returned and ERRNO set
 * to 0.  */
char *
_gpgrt_getenv (const char *name)
{
  if (!name || !*name || strchr (name, '='))
    {
      _gpg_err_set_errno (EINVAL);
      return NULL;
    }

#ifdef HAVE_W32_SYSTEM
  {
    int len, size;
    char *result;

    len = GetEnvironmentVariable (name, NULL, 0);
    if (!len && GetLastError () == ERROR_ENVVAR_NOT_FOUND)
      {
        _gpg_err_set_errno (0);
        return NULL;
      }
  again:
    size = len;
    result = _gpgrt_malloc (size);
    if (!result)
      return NULL;
    len = GetEnvironmentVariable (name, result, size);
    if (len >= size)
      {
        /* Changed in the meantime - retry.  */
        _gpgrt_free (result);
        goto again;
      }
    if (!len && GetLastError () == ERROR_ENVVAR_NOT_FOUND)
      {
        /* Deleted in the meantime.  */
        _gpgrt_free (result);
        _gpg_err_set_errno (0);
        return NULL;
      }
    if (!len)
      {
        /* Other error.  FIXME: We need mapping fucntion. */
        _gpgrt_free (result);
        _gpg_err_set_errno (EIO);
        return NULL;
      }

    return result;
  }
#else /*!HAVE_W32_SYSTEM*/
  {
    const char *s = getenv (name);
    if (!s)
      {
        _gpg_err_set_errno (0);
        return NULL;
      }
    return _gpgrt_strdup (s);
  }
#endif /*!HAVE_W32_SYSTEM*/
}


/* Wrapper around setenv so that we can have the same function in
 * Windows and Unix.  In contrast to the standard setenv passing a
 * VALUE as NULL and setting OVERWRITE will remove the envvar.  */
gpg_err_code_t
_gpgrt_setenv (const char *name, const char *value, int overwrite)
{
  if (!name || !*name || strchr (name, '='))
    return GPG_ERR_EINVAL;

#ifdef HAVE_W32_SYSTEM
  /* Windows maintains (at least) two sets of environment variables.
   * One set can be accessed by GetEnvironmentVariable and
   * SetEnvironmentVariable.  This set is inherited by the children.
   * The other set is maintained in the C runtime, and is accessed
   * using getenv and putenv.  We try to keep them in sync by
   * modifying both sets.  Note that gpgrt_getenv ignores the libc
   * values - however, too much existing code still uses getenv.  */
  {
    int exists;
    char tmpbuf[10];

    if (!value && overwrite)
      {
        if (!SetEnvironmentVariable (name, NULL))
          return GPG_ERR_EINVAL;
        if (getenv (name))
#if defined(_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES)
          {
            int e = _putenv_s (name, "");

            if (e)
              {
                _gpg_err_set_errno (e);
                return -1;
              }
          }
#else
        /* Deleting an envvar.  Although the Microsoft specs for
         * putenv tell us that one may use "NAME=" to unset an envvar,
         * this seems not to be correct.  Thus we do the same as what
         * we do in Unix (or well in GNU libc) and use just "NAME". */
          {
            char *buf;
            /* Ugly: Leaking memory.  */
            buf = _gpgrt_strdup (name);
            if (!buf)
              return _gpg_err_code_from_syserror ();
            if (putenv (buf))
              return _gpg_err_code_from_syserror ();
          }
#endif
        return 0;
      }

    exists = GetEnvironmentVariable (name, tmpbuf, sizeof tmpbuf);
    if ((! exists || overwrite) && !SetEnvironmentVariable (name, value))
      return GPG_ERR_EINVAL; /* (Might also be ENOMEM.) */
    if (overwrite || !getenv (name))
#if defined(_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES)
      {
        int e = _putenv_s (name, value);

        if (e)
          {
            _gpg_err_set_errno (e);
            return -1;
          }
      }
#else
      {
        /* Ugly: Leaking memory.  */
        buf = _gpgrt_strconcat (name, "=", value, NULL);
        if (!buf)
          return _gpg_err_code_from_syserror ();
        if (putenv (buf))
          return _gpg_err_code_from_syserror ();
      }
#endif
    return 0;
  }

#else /*!HAVE_W32_SYSTEM*/

# ifdef HAVE_SETENV

  {
    if (!value && overwrite)
      {
        if (unsetenv (name))
          return _gpg_err_code_from_syserror ();
      }
    else
      {
        if (setenv (name, value ? value : "", overwrite))
          return _gpg_err_code_from_syserror ();
      }

    return 0;
  }

# else /*!HAVE_SETENV*/

# if __GNUC__
#   warning no setenv - using putenv but leaking memory.
# endif
  {
    char *buf;

    if (!value && overwrite)
      {
        if (getenv (name))
          {
            buf = _gpgrt_strdup (name);
            if (!buf)
              return _gpg_err_code_from_syserror ();
            if (putenv (buf))
              return -1;
          }
      }
    else if (overwrite || !getenv (name))
      {
        buf = _gpgrt_strconcat (name, "=", value, NULL);
        if (!buf)
          return _gpg_err_code_from_syserror ();
        if (putenv (buf))
          return _gpg_err_code_from_syserror ();
      }

    return 0;
  }
# endif /*!HAVE_SETENV*/
#endif /*!HAVE_W32_SYSTEM*/
}


#ifdef HAVE_W32_SYSTEM
/* Checks whether fname has a path separator, that is a slash or
 * backslash somewhere.  However, it skips leading slashes with drive
 * letters (backslashes and slashes are considered the same):
 *  x:\foo     -> false
 *  x:foo      -> false
 *  x:         -> false
 *  x:\        -> false
 *  foo        -> false
 *  foo\bar    -> true
 *  x:\foo\bar -> true
 *  x:foo\bar  -> true
 *  \\server   -> true
 */
static int
has_path_separator (const char *fname)
{
  if (fname[0] == '/' || fname[0] == '\\')
    return 1;
  else if (fname[0] && fname[1] == ':')
    return (fname[2] && strpbrk (fname+3, "/\\"));
  else
    return !!strpbrk (fname, "/\\");
}

/* Convert an UTF-8 encode file name to wchar.  If the file name is
 * close to the limit of MAXPATH the API functions will fail.  The
 * method to overcome this API limitation is to use a prefix which
 * bypasses the checking by CreateFile.  This also requires to first
 * convert the name to an absolute file name.  To avoid looking up the
 * full name in all cases we do the \\?\ prefixing only if FNAME is
 * longer than 60 byes or if it has any path separator.  */
wchar_t *
_gpgrt_fname_to_wchar (const char *fname)
{
  static int no_extlenpath;
  wchar_t *wname, *w;
  wchar_t *wfullpath = NULL;
  int success = 0;

  if (!no_extlenpath)
    {
      /* Note that a value of -2 enables debugging of extlenpath.  */
      const char *s = getenv ("GPGRT_DISABLE_EXTLENPATH");
      int i = atoi (s? s:"0");

      if (i > 0)
        no_extlenpath = 1;
      else
        no_extlenpath = (i == -2)? -2 : -1;
    }

  wname = _gpgrt_utf8_to_wchar (fname);
  if (!wname)
    return NULL;

  /* Under Wine, keep the slash as is.  */
  if (!_gpgrt_windows_feature (GPGRT_WINDOWS_UNDER_WINE))
    {
      /* We use only backslashes in the wchar version of fname.  */
      for (w = wname; *w; w++)
        if (*w == L'/')
          *w = L'\\';
    }

  if (no_extlenpath > 0)
    success = 1; /* Extended length path support has been disabled.  */
  else if (!strncmp (fname, "\\\\?\\", 4) || !strncmp (fname, "//?/", 4))
    success = 1; /* Already translated.  */
  else if (has_path_separator (fname) || wcslen (wname) > 60)
    {
      int wlen = 1024;
      DWORD res;

    try_again:
      /* We need some extra characters (backslashes are actually used):
       * Either "//?/" = 4 or "//?/UNC" = 7 and to be safe one more
       * for the end of string */
      wfullpath = xtrymalloc ((wlen+8) * sizeof *wfullpath);
      if (!wfullpath)
        goto leave;

      if (no_extlenpath == -2)
        {
          char *tmpn = _gpgrt_wchar_to_utf8 (wname, -1);
          _gpgrt_log_debug ("%s:%d: checking '%s'\n",
                            __func__, __LINE__, tmpn);
          xfree (tmpn);
        }
      /* Note that GFPN is basically a string function without a
       * MAX_PATH limitation.  cf https://googleprojectzero.blogspot.com
       * /2016/02/the-definitive-guide-on-win32-to-nt.html */
      res = GetFullPathNameW (wname, wlen, wfullpath, NULL);
      if (!res)
        {
          _gpgrt_w32_set_errno (-1);
          goto leave;
        }
      else if (res >= wlen)
        {
          /* Truncated - increase to the desired length.  */
          if (wlen > 1024)
            {
              /* We should never get to here.  */
              errno = ENAMETOOLONG;
              goto leave;
            }
          /* GetFullPathNameW indicated the required buffer length.  */
          _gpgrt_free_wchar (wfullpath);
          wfullpath = NULL;
          wlen = res;
          goto try_again;
        }
      _gpgrt_free_wchar (wname);
      wname = wfullpath;
      wfullpath = NULL;
      if (no_extlenpath == -2)
        {
          char *tmpn = _gpgrt_wchar_to_utf8 (wname, -1);
          _gpgrt_log_debug ("%s:%d: absfname '%s' (len=%d)\n",
                            __func__, __LINE__, tmpn, (int)res);
          xfree (tmpn);
        }

      if (res < 250)
        {
          /* No need for extended length path.  For unknown reasons
           * this does not work reliable with 255 or even with the
           * real value of MAX_PATH (260).  A value of 250 seems to
           * work though. */
        }
      else if (*wname == L'\\' && wname[1] == L'\\' && wname[2])
        {
          /* For an UNC extended length path only one backslash is
           * expected after the prefix.  This we overwrite the first
           * slash with the 'C'.  Thus a shift by 6 and not 7.  */
          wmemmove (wname+6, wname, res+1);
          wname[0] = L'\\';
          wname[1] = L'\\';
          wname[2] = L'?';
          wname[3] = L'\\';
          wname[4] = L'U';
          wname[5] = L'N';
          wname[6] = L'C'; /* (Overwrites the first slash.)  */
        }
      else
        {
          wmemmove (wname+4, wname, res+1);
          wname[0] = L'\\';
          wname[1] = L'\\';
          wname[2] = L'?';
          wname[3] = L'\\';
        }
      success = 2;
    }
  else
    success = 1;

 leave:
  _gpgrt_free_wchar (wfullpath);
  if (!success)
    {
      _gpgrt_free_wchar (wname);
      wname = NULL;
    }
  if (wname && success == 2 && no_extlenpath == -2)
    {
      char *tmpn = _gpgrt_wchar_to_utf8 (wname, -1);
      _gpgrt_log_debug ("%s:%d:    using '%s'\n", __func__, __LINE__, tmpn);
      xfree (tmpn);
    }
  return wname;
}

#endif /*HAVE_W32_SYSTEM*/



#ifndef HAVE_W32_SYSTEM
static mode_t
modestr_to_mode (const char *modestr)
{
  mode_t mode = 0;

  if (modestr && *modestr)
    {
      modestr++;
      if (*modestr && *modestr++ == 'r')
        mode |= S_IRUSR;
      if (*modestr && *modestr++ == 'w')
        mode |= S_IWUSR;
      if (*modestr && *modestr++ == 'x')
        mode |= S_IXUSR;
      if (*modestr && *modestr++ == 'r')
        mode |= S_IRGRP;
      if (*modestr && *modestr++ == 'w')
        mode |= S_IWGRP;
      if (*modestr && *modestr++ == 'x')
        mode |= S_IXGRP;
      if (*modestr && *modestr++ == 'r')
        mode |= S_IROTH;
      if (*modestr && *modestr++ == 'w')
        mode |= S_IWOTH;
      if (*modestr && *modestr++ == 'x')
        mode |= S_IXOTH;
    }

  return mode;
}
#endif


/* A wrapper around mkdir which takes a string for the mode argument.
 * This makes it easier to handle the mode argument which is not
 * defined on all systems.  The format of the modestring is
 *
 *    "-rwxrwxrwx"
 *
 * '-' is a don't care or not set.  'r', 'w', 'x' are read allowed,
 * write allowed, execution allowed with the first group for the user,
 * the second for the group and the third for all others.  If the
 * string is shorter than above the missing mode characters are meant
 * to be not set.
 *
 * Note that in addition to returning an gpg-error error code ERRNO is
 * also set by this function.
 */
gpg_err_code_t
_gpgrt_mkdir (const char *name, const char *modestr)
{
#ifdef HAVE_W32_SYSTEM
  wchar_t *wname;
  gpg_err_code_t ec;
  (void)modestr;

  /* Note: Fixme: We should set appropriate permissions.  */
  wname = _gpgrt_fname_to_wchar (name);
  if (!wname)
    return _gpg_err_code_from_syserror ();

  if (!CreateDirectoryW (wname, NULL))
    {
      _gpgrt_w32_set_errno (-1);
      ec = _gpg_err_code_from_syserror ();
    }
  else
    ec = 0;

  _gpgrt_free_wchar (wname);
  return ec;

#elif MKDIR_TAKES_ONE_ARG
  (void)modestr;
  if (mkdir (name))
    return _gpg_err_code_from_syserror ();
  return 0;
#else
  if (mkdir (name, modestr_to_mode (modestr)))
    return _gpg_err_code_from_syserror ();
  return 0;
#endif
}


/* A simple wrapper around chdir.  NAME is expected to be utf8
 * encoded.  Note that in addition to returning an gpg-error error
 * code ERRNO is also set by this function.  */
gpg_err_code_t
_gpgrt_chdir (const char *name)
{
#ifdef HAVE_W32_SYSTEM
  wchar_t *wname;
  gpg_err_code_t ec;

  /* Note that the \\?\ trick does not work with SetCurrentDirectoryW
   * Thus we use the plain conversion function.  */
  wname = _gpgrt_utf8_to_wchar (name);
  if (!wname)
    return _gpg_err_code_from_syserror ();
  if (!SetCurrentDirectoryW (wname))
    {
      _gpgrt_w32_set_errno (-1);
      ec = _gpg_err_code_from_syserror ();
    }
  else
    ec = 0;
  _gpgrt_free_wchar (wname);
  return ec;

#else /*!HAVE_W32_SYSTEM*/
  if (chdir (name))
    return _gpg_err_code_from_syserror ();
  return 0;
#endif /*!HAVE_W32_SYSTEM*/
}


/* Return the current working directory as a malloced string.  Return
 * NULL and sets ERRNO on error.  */
char *
_gpgrt_getcwd (void)
{
#if defined(HAVE_W32_SYSTEM)
  wchar_t wbuffer[MAX_PATH + sizeof(wchar_t)];
  DWORD wlen;
  char *buf, *p;

  wlen = GetCurrentDirectoryW (MAX_PATH, wbuffer);
  if (!wlen)
    {
      _gpgrt_w32_set_errno (-1);
      return NULL;

    }
  else if (wlen > MAX_PATH)
    {
      /* FWIW: I tried to use GetFullPathNameW (L".") but found no way
       * to execute a test program at a too long cwd.  */
      _gpg_err_set_errno (ENAMETOOLONG);
      return NULL;
    }
  buf = _gpgrt_wchar_to_utf8 (wbuffer, wlen);
  if (buf)
    {
      for (p=buf; *p; p++)
        if (*p == '\\')
          *p = '/';
    }
  return buf;

#else /*Unix*/
  char *buffer;
  size_t size = 100;

  for (;;)
    {
      buffer = xtrymalloc (size+1);
      if (!buffer)
        return NULL;
      if (getcwd (buffer, size) == buffer)
        return buffer;
      xfree (buffer);
      if (errno != ERANGE)
        return NULL;
      size *= 2;
    }
#endif /*Unix*/
}


/* Wrapper around access to handle file name encoding under Windows.
 * Returns 0 if FNAME can be accessed in MODE or an error code.  ERRNO
 * is also set on error. */
gpg_err_code_t
_gpgrt_access (const char *fname, int mode)
{
  gpg_err_code_t ec;

#ifdef HAVE_W32_SYSTEM
  wchar_t *wfname;
  DWORD attribs;

  wfname = _gpgrt_fname_to_wchar (fname);
  if (!wfname)
    return _gpg_err_code_from_syserror ();

  attribs = GetFileAttributesW (wfname);
  if (attribs == (DWORD)(-1))
    ec = _gpgrt_w32_get_last_err_code ();
  else
    {
      if ((mode & W_OK) && (attribs & FILE_ATTRIBUTE_READONLY))
        {
          _gpg_err_set_errno (EACCES);
          ec = _gpg_err_code_from_syserror ();
        }
      else
        ec = 0;
    }
  _gpgrt_free_wchar (wfname);
#else /* Unix */
  ec = access (fname, mode)? _gpg_err_code_from_syserror () : 0;
#endif /* Unix */

  return ec;
}


/* Get the standard home directory for user NAME. If NAME is NULL the
 * directory for the current user is returned.  Caller must release
 * the returned string.  */
char *
_gpgrt_getpwdir (const char *name)
{
  char *result = NULL;
#ifdef HAVE_PWD_H
  struct passwd *pwd = NULL;

  if (name)
    {
#ifdef HAVE_GETPWNAM
      /* Fixme: We should use getpwnam_r if available.  */
      pwd = getpwnam (name);
#endif
    }
  else
    {
#ifdef HAVE_GETPWUID
      /* Fixme: We should use getpwuid_r if available.  */
      pwd = getpwuid (getuid());
#endif
    }
  if (pwd)
    {
      result = _gpgrt_strdup (pwd->pw_dir);
    }
#else /*!HAVE_PWD_H*/
  /* No support at all.  */
  (void)name;
#endif /*HAVE_PWD_H*/
  return result;
}


/* Return a malloced copy of the current user's account name; this may
 * return NULL on memory failure.  */
char *
_gpgrt_getusername (void)
{
  char *result = NULL;

#ifdef HAVE_W32_SYSTEM
  wchar_t wtmp[1];
  wchar_t *wbuf;
  DWORD wsize = 1;
  char *buf;

  GetUserNameW (wtmp, &wsize);
  wbuf = _gpgrt_malloc (wsize * sizeof *wbuf);
  if (!wbuf)
    {
      _gpgrt_w32_set_errno (-1);
      return NULL;
    }
  if (!GetUserNameW (wbuf, &wsize))
    {
      _gpgrt_w32_set_errno (-1);
      xfree (wbuf);
      return NULL;
    }
  buf = _gpgrt_wchar_to_utf8 (wbuf, wsize);
  xfree (wbuf);
  return buf;

#else /* !HAVE_W32_SYSTEM */

# if defined(HAVE_PWD_H) && defined(HAVE_GETPWUID)
  struct passwd *pwd;

  pwd = getpwuid (getuid());
  if (pwd)
    {
      result = _gpgrt_strdup (pwd->pw_name);
    }

# endif /*HAVE_PWD_H*/

#endif /* !HAVE_W32_SYSTEM */

  return result;
}
