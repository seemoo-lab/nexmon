/*
   libc of DJGPP 2.03 does not offer function unsetenv,
   so this version from DJGPP 2.04 CVS tree is supplied.
   This file will become superflous and will be removed
   from the  distribution as soon as DJGPP 2.04 has been
   released.
*/

/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */

#include <libc/stubs.h>
#include <libc/unconst.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

extern char **environ;

int
unsetenv(const char *name)
{
  /* No environment == success */
  if (environ == 0)
    return 0;

  /* Check for the failure conditions */
  if (name == NULL || *name == '\0' || strchr (name, '=') != NULL)
  {
    errno = EINVAL;
    return -1;
  }

  /* Let putenv() do the work
   * Note that we can just pass name directly as our putenv() treats
   * this the same as passing 'name='. */
  /* The cast is needed because POSIX specifies putenv() to take a non-const
   * parameter.  Our putenv is well-behaved, so this is OK.  */
  putenv (unconst (name, char*));

  return 0;
}
