/* Copyright (C) 1998, 1999, 2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Zack Weinberg <zack@rabi.phys.columbia.edu>, 1998.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#define _PATH_DEVPTMX "/dev/ptmx"

int openpty (int *amaster, int *aslave, char *name, struct termios *termp,
		struct winsize *winp)
{
	char buf[PATH_MAX];
	int master, slave;

	master = open(_PATH_DEVPTMX, O_RDWR);
	if (master == -1)
		return -1;

	if (grantpt(master))
		goto fail;

	if (unlockpt(master))
		goto fail;

	if (ptsname_r(master, buf, sizeof buf))
		goto fail;

	slave = open(buf, O_RDWR | O_NOCTTY);
	if (slave == -1)
		goto fail;

	/* XXX Should we ignore errors here?  */
	if (termp)
		tcsetattr(slave, TCSAFLUSH, termp);
	if (winp)
		ioctl(slave, TIOCSWINSZ, winp);

	*amaster = master;
	*aslave = slave;
	if (name != NULL)
		strcpy(name, buf);

	return 0;

fail:
	close(master);
	return -1;
}
