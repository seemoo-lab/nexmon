/*
   libc of DJGPP 2.03 does not offer a pw_gecos entry,
   so this version from DJGPP 2.04 CVS tree is supplied.
   This file will become superflous and will be removed
   from the  distribution as soon as DJGPP 2.04 has been
   released.
*/

/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include "djpwd.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static char passwd[] = "";
static char slash [] = "/";
static char shell [] = "sh";

struct passwd *
getpwnam(const char *name)
{
  static struct passwd rv;
  rv.pw_name = getlogin();
  if (strcmp(rv.pw_name, name) != 0)
    return 0;
  rv.pw_uid = getuid();
  rv.pw_gid = getgid();
  rv.pw_dir = getenv("HOME");
  if (rv.pw_dir == 0)
    rv.pw_dir = slash;
  rv.pw_shell = getenv("SHELL");
  if (rv.pw_shell == 0)
    rv.pw_shell = getenv("COMSPEC");
  if (rv.pw_shell == 0)
    rv.pw_shell = shell;
  rv.pw_gecos = getlogin();
  rv.pw_passwd = passwd;
  return &rv;
}
