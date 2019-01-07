/* Enable a variable CVSUSER for cvs.  */
/* See cvs/subr.c: getcaller().  */

#include <stdlib.h>
#include <string.h>
#include <pwd.h>

int getuid (void)
{
  return 0;
}

char * getlogin (void)
{
  char *s;

  s = getenv ("CVSUSER");
  if (s && *s)
    return s;
  s = getenv ("USER");
  if (s && *s)
    return s;
  return NULL;
}

struct passwd * getpwnam (const char *name)
{
  static struct passwd pw;
  static char namebuf[100];

  pw.pw_name = strcpy (namebuf, name);
  pw.pw_passwd = "*";
  pw.pw_uid = 100;
  pw.pw_gid = 100;
  pw.pw_gecos = "";
  pw.pw_dir = "/";
  pw.pw_shell = "/bin/sh";

  return &pw;
}
