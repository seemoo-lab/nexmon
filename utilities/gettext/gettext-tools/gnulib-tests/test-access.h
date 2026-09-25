/* Tests of access and euidaccess.
   Copyright (C) 2019-2026 Free Software Foundation, Inc.

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

/* mingw and MSVC 9 lack geteuid, so setup a dummy value.  */
#if !HAVE_GETEUID
# define geteuid() ROOT_UID
#endif

#if defined __CYGWIN__
/* Cygwin uses the security model from Windows.  */
# include <grp.h>
# include <string.h>
# include <unistd.h>
/* Test whether the current user is in the 'Administrators' group.
   We cannot use the group name "Administrators", due to internationalization.
   Therefore test for the SID "S-1-5-32-544" (cf.
   <https://learn.microsoft.com/en-us/windows/win32/secauthz/well-known-sids>).
 */
static int
is_administrator (void)
{
  gid_t list[100];
  int count = getgroups (sizeof (list) / sizeof (list[0]), list);
  if (count < 0)
    return 0;
  for (int i = 0; i < count; i++)
    {
      gid_t id = list[i];
      struct group *details = getgrgid (id);
      if (details != NULL && strcmp (details->gr_passwd, "S-1-5-32-544") == 0)
        return 1;
    }
  return 0;
}
# define is_root() is_administrator ()
#else
# define is_root() (geteuid () == ROOT_UID)
#endif

static void
test_access (int (*func) (const char * /*file*/, int /*mode*/))
{
  /* Remove anything from prior partial run.  */
  unlink (BASE "f");
  unlink (BASE "f1");
  chmod (BASE "f2", 0600);
  unlink (BASE "f2");
  unlink (BASE "sl");

  {
    errno = 0;
    ASSERT (func (BASE "f", R_OK) == -1);
    ASSERT (errno == ENOENT);

    errno = 0;
    ASSERT (func (BASE "f", W_OK) == -1);
    ASSERT (errno == ENOENT);

    errno = 0;
    ASSERT (func (BASE "f", X_OK) == -1);
    ASSERT (errno == ENOENT);
  }
  {
    ASSERT (close (creat (BASE "f1", 0700)) == 0);

    ASSERT (func (BASE "f1", F_OK) == 0);
    ASSERT (func (BASE "f1", R_OK) == 0);
    ASSERT (func (BASE "f1", W_OK) == 0);
    ASSERT (func (BASE "f1", X_OK) == 0);

    ASSERT (func (BASE "f1/", F_OK) == -1);
    ASSERT (errno == ENOTDIR);
    ASSERT (func (BASE "f1/", R_OK) == -1);
    ASSERT (errno == ENOTDIR);
    ASSERT (func (BASE "f1/", W_OK) == -1);
    ASSERT (errno == ENOTDIR);
    ASSERT (func (BASE "f1/", X_OK) == -1);
    ASSERT (errno == ENOTDIR);

    if (symlink (BASE "f1", BASE "sl") == 0)
      {
        ASSERT (func (BASE "sl/", F_OK) == -1);
        ASSERT (errno == ENOTDIR);
        ASSERT (func (BASE "sl/", R_OK) == -1);
        ASSERT (errno == ENOTDIR);
        ASSERT (func (BASE "sl/", W_OK) == -1);
        ASSERT (errno == ENOTDIR);
        ASSERT (func (BASE "sl/", X_OK) == -1);
        ASSERT (errno == ENOTDIR);
      }
  }
  {
    ASSERT (close (creat (BASE "f2", 0600)) == 0);
    ASSERT (chmod (BASE "f2", 0400) == 0);

    ASSERT (func (BASE "f2", R_OK) == 0);

    /* On Cygwin, for users that are in the 'Administrators' group,
       W_OK is allowed.  */
    if (! is_root ())
      {
        errno = 0;
        ASSERT (func (BASE "f2", W_OK) == -1);
        ASSERT (errno == EACCES);
      }

#if defined _WIN32 && !defined __CYGWIN__
    /* X_OK works like R_OK.  */
    ASSERT (func (BASE "f2", X_OK) == 0);
#else
    /* On Solaris, for the root user, X_OK is allowed.
       On Cygwin 3.5.5, for users that are in the 'Administrators' group,
       X_OK is allowed.  */
# if defined __sun || defined __CYGWIN__
    if (! is_root ())
# endif
      {
        errno = 0;
        ASSERT (func (BASE "f2", X_OK) == -1);
        ASSERT (errno == EACCES);
      }
#endif
  }

  /* Cleanup.  */
  ASSERT (unlink (BASE "f1") == 0);
  ASSERT (chmod (BASE "f2", 0600) == 0);
  ASSERT (unlink (BASE "f2") == 0);
  unlink (BASE "sl");
}
