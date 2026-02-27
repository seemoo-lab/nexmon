/* source: xio-system.c */
/* Copyright Gerhard Rieger */
/* Published under the GNU General Public License V.2, see file COPYING */

/* this file contains the source for opening addresses of system type */

#include "xiosysincludes.h"
#include "xioopen.h"

#include "xio-progcall.h"
#include "xio-system.h"


#if WITH_SYSTEM

static int xioopen_system(int arg, const char *argv[], struct opt *opts,
		int xioflags,	/* XIO_RDONLY etc. */
		xiofile_t *fd,
		unsigned groups,
		int inter, int form, int dummy3
		);

/* the endpoint variant: get stdin and/or stdout on "left" side. socat does not
   provide a "right" side for script */
static const struct xioaddr_endpoint_desc xioendpoint_system1  = { XIOADDR_SYS,   "system",  1, XIOBIT_ALL,    GROUP_FD|GROUP_FORK|GROUP_EXEC|GROUP_SOCKET|GROUP_SOCK_UNIX|GROUP_TERMIOS|GROUP_FIFO|GROUP_PTY|GROUP_PARENT, XIOSHUT_UNSPEC, XIOCLOSE_UNSPEC, xioopen_system, false, 0, 0 HELP(":<shell-command>") };
/* the inter address variant: the bidirectional form has stdin and stdout on
   its "left" side, and FDs 3 (for reading) and FD 4 (for writing) on its right
   side. */
static const struct xioaddr_inter_desc    xiointer_system1_2rw = { XIOADDR_INTER, "system",  1, XIOBIT_RDWR,   GROUP_FD|GROUP_FORK|GROUP_EXEC|GROUP_SOCKET|GROUP_SOCK_UNIX|GROUP_TERMIOS|GROUP_FIFO|GROUP_PTY|GROUP_PARENT, XIOSHUT_UNSPEC, XIOCLOSE_UNSPEC, xioopen_system, true,  2, 0, XIOBIT_RDWR   HELP(":<shell-command>") };
static const struct xioaddr_inter_desc    xiointer_system1_2ro = { XIOADDR_INTER, "system",  1, XIOBIT_RDONLY, GROUP_FD|GROUP_FORK|GROUP_EXEC|GROUP_SOCKET|GROUP_SOCK_UNIX|GROUP_TERMIOS|GROUP_FIFO|GROUP_PTY|GROUP_PARENT, XIOSHUT_UNSPEC, XIOCLOSE_UNSPEC, xioopen_system, true,  2, 0, XIOBIT_WRONLY HELP(":<shell-command>") };
static const struct xioaddr_inter_desc    xiointer_system1_2wo = { XIOADDR_INTER, "system",  1, XIOBIT_WRONLY, GROUP_FD|GROUP_FORK|GROUP_EXEC|GROUP_SOCKET|GROUP_SOCK_UNIX|GROUP_TERMIOS|GROUP_FIFO|GROUP_PTY|GROUP_PARENT, XIOSHUT_UNSPEC, XIOCLOSE_UNSPEC, xioopen_system, true,  2, 0, XIOBIT_RDONLY HELP(":<shell-command>") };
/* the unidirectional inter address variant: the "left" side reads from stdin,
   and the right side reads from stdout. */
static const struct xioaddr_inter_desc    xiointer_system1_1wo = { XIOADDR_INTER, "system1", 1, XIOBIT_WRONLY, GROUP_FD|GROUP_FORK|GROUP_EXEC|GROUP_SOCKET|GROUP_SOCK_UNIX|GROUP_TERMIOS|GROUP_FIFO|GROUP_PTY|GROUP_PARENT, XIOSHUT_UNSPEC, XIOCLOSE_UNSPEC, xioopen_system, true,  1, 0, XIOBIT_RDONLY HELP(":<shell-command>") };

/* the general forms, designed for bidirectional transfer (stdio -- 3,4) */
const union xioaddr_desc *xioaddrs_system[] = {
   (union xioaddr_desc *)&xioendpoint_system1,
   (union xioaddr_desc *)&xiointer_system1_2rw,
   (union xioaddr_desc *)&xiointer_system1_2ro,
   (union xioaddr_desc *)&xiointer_system1_2wo,
   NULL
};

/* unidirectional inter address (stdin -- stdout) */
const union xioaddr_desc *xioaddrs_system1[] = {
   (union xioaddr_desc *)&xiointer_system1_1wo,
   NULL };

static int xioopen_system(int argc, const char *argv[], struct opt *opts,
		int xioflags,	/* XIO_RDONLY etc. */
		xiofile_t *fd,
		unsigned groups,
		int inter, int form, int dummy3
		) {
   int status;
   char *path = NULL;
   int duptostderr;
   int result;
   const char *string = argv[1];

   status = _xioopen_progcall(xioflags, &fd->stream, groups, &opts, &duptostderr, inter, form);
   if (status < 0)  return status;
   if (status == 0) {	/* child */
      int numleft;

      /* do not shutdown connections that belong our parent */
      sock[0] = NULL;
      sock[1] = NULL;

      if (setopt_path(opts, &path) < 0) {
	 /* this could be dangerous, so let us abort this child... */
	 Exit(1);
      }

      if ((numleft = leftopts(opts)) > 0) {
	 Error1("%d option(s) could not be used", numleft);
	 showleft(opts);
	 return STAT_NORETRY;
      }

      /* only now redirect stderr */
      if (duptostderr >= 0) {
	 diag_dup();
	 Dup2(duptostderr, 2);
      }
      Info1("executing shell command \"%s\"", string);
      result = System(string);
      if (result != 0) {
	 Warn2("system(\"%s\") returned with status %d", string, result);
	 Warn1("system(): %s", strerror(errno));
      }
      Exit(0);	/* this child process */
   }

   /* parent */
   return 0;
}

#endif /* WITH_SYSTEM */
