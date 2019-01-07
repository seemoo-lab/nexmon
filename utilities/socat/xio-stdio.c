/* source: xio-stdio.c */
/* Copyright Gerhard Rieger 2001-2009 */
/* Published under the GNU General Public License V.2, see file COPYING */

/* this file contains the source for opening addresses stdio type */

#include "xiosysincludes.h"
#include "xioopen.h"

#include "xio-fdnum.h"
#include "xio-stdio.h"


#if WITH_STDIO

static int xioopen_stdio(int argc, const char *argv[], struct opt *opts, int xioflags, xiofile_t *fd, unsigned groups, int dummy1, int dummy2, int dummy3);
static int xioopen_stdfd(int argc, const char *argv[], struct opt *opts, int xioflags, xiofile_t *xfd, unsigned groups, int fd, int dummy2, int dummy3);


/* we specify all option groups that we can imagine for a FD, becasue the
   changed parsing mechanism does not allow us to check the type of FD before
   applying the options */
static const struct xioaddr_endpoint_desc xioaddr_stdio0  = { XIOADDR_SYS, "stdio",  0, XIOBIT_ALL,   GROUP_FD|GROUP_FIFO|GROUP_CHR|GROUP_BLK|GROUP_FILE|GROUP_SOCKET|GROUP_TERMIOS|GROUP_SOCK_UNIX|GROUP_SOCK_IP|GROUP_IPAPP, XIOSHUT_UNSPEC, XIOCLOSE_NONE, xioopen_stdio, 0, 0, 0 HELP(NULL) };
static const struct xioaddr_endpoint_desc xioaddr_stdin0  = { XIOADDR_SYS, "stdin",  0, XIOBIT_RDONLY, GROUP_FD|GROUP_FIFO|GROUP_CHR|GROUP_BLK|GROUP_FILE|GROUP_SOCKET|GROUP_TERMIOS|GROUP_SOCK_UNIX|GROUP_SOCK_IP|GROUP_IPAPP, XIOSHUT_UNSPEC, XIOCLOSE_NONE, xioopen_stdfd, 0, 0, 0 HELP(NULL) };
static const struct xioaddr_endpoint_desc xioaddr_stdout0 = { XIOADDR_SYS, "stdout", 0, XIOBIT_WRONLY, GROUP_FD|GROUP_FIFO|GROUP_CHR|GROUP_BLK|GROUP_FILE|GROUP_SOCKET|GROUP_TERMIOS|GROUP_SOCK_UNIX|GROUP_SOCK_IP|GROUP_IPAPP, XIOSHUT_UNSPEC, XIOCLOSE_NONE, xioopen_stdfd, 1, 0, 0 HELP(NULL) };
static const struct xioaddr_endpoint_desc xioaddr_stderr0 = { XIOADDR_SYS, "stderr", 0, XIOBIT_WRONLY, GROUP_FD|GROUP_FIFO|GROUP_CHR|GROUP_BLK|GROUP_FILE|GROUP_SOCKET|GROUP_TERMIOS|GROUP_SOCK_UNIX|GROUP_SOCK_IP|GROUP_IPAPP, XIOSHUT_UNSPEC, XIOCLOSE_NONE, xioopen_stdfd, 2, 0, 0 HELP(NULL) };

const union xioaddr_desc *xioaddrs_stdio[] = {
   (union xioaddr_desc *)&xioaddr_stdio0, NULL };
const union xioaddr_desc *xioaddrs_stdin[] = {
   (union xioaddr_desc *)&xioaddr_stdin0, NULL };
const union xioaddr_desc *xioaddrs_stdout[] = {
   (union xioaddr_desc *)&xioaddr_stdout0, NULL };
const union xioaddr_desc *xioaddrs_stderr[] = {
   (union xioaddr_desc *)&xioaddr_stderr0, NULL };

/* process a bidirectional "stdio" or "-" argument with options. */
int xioopen_stdio_bi(xiofile_t *sock) {
   struct opt *opts1, *opts2, *optspr;
   unsigned int groups1 = xioaddr_stdio0.groups, groups2 = xioaddr_stdio0.groups;
   int result;

   sock->stream.rfd = 0 /*stdin*/;
   sock->stream.wfd = 1 /*stdout*/;

#if WITH_TERMIOS
   if (Isatty(sock->stream.rfd)) {
      if (Tcgetattr(sock->stream.rfd,
		    &sock->stream.savetty)
	  < 0) {
	 Warn2("cannot query current terminal settings on fd %d: %s",
	       sock->stream.rfd, strerror(errno));
      } else {
	 sock->stream.ttyvalid = true;
      }
   }
   if (Isatty(sock->stream.wfd) && (sock->stream.wfd != sock->stream.rfd)) {
      if (Tcgetattr(sock->stream.wfd,
		    &sock->stream.savetty)
	  < 0) {
	 Warn2("cannot query current terminal settings on fd %d: %s",
	       sock->stream.wfd, strerror(errno));
      } else {
	 sock->stream.ttyvalid = true;
      }
   }
#endif /* WITH_TERMIOS */
   if (applyopts_single(&sock->stream, sock->stream.opts, PH_INIT) < 0)
      return -1;
   applyopts(-1, sock->stream.opts, PH_INIT);
   if (sock->stream.howtoshut == XIOSHUT_UNSPEC)
      sock->stream.howtoshut = XIOSHUT_NONE;

   /* options here are one-time and one-direction, no second use */
   retropt_bool(sock->stream.opts, OPT_IGNOREEOF, &sock->stream.ignoreeof);

   /* extract opts that should be applied only once */
   if ((optspr = copyopts(sock->stream.opts, GROUP_PROCESS)) == NULL) {
      return -1;
   }
   if ((result = applyopts(-1, optspr, PH_EARLY)) < 0)
      return result;
   if ((result = applyopts(-1, optspr, PH_PREOPEN)) < 0)
      return result;

   /* here we copy opts, because most have to be applied twice! */
   if ((opts1 = copyopts(sock->stream.opts, GROUP_FD|GROUP_APPL|(groups1&~GROUP_PROCESS))) == NULL) {
      return -1;
   }

   /* apply options to first FD */
   if ((result = applyopts(sock->stream.rfd, opts1, PH_ALL)) < 0) {
      return result;
   }
   if ((result = _xio_openlate(&sock->stream, opts1)) < 0) {
      return result;
   }

   if ((opts2 = copyopts(sock->stream.opts, GROUP_FD|GROUP_APPL|(groups2&~GROUP_PROCESS))) == NULL) {
      return -1;
   }
   /* apply options to second FD */
   if ((result = applyopts(sock->stream.wfd, opts2, PH_ALL)) < 0) {
      return result;
   }
   if ((result = _xio_openlate(&sock->stream, opts2)) < 0) {
      return result;
   }

   if ((result = _xio_openlate(&sock->stream, optspr)) < 0) {
      return result;
   }

   Notice("reading from and writing to stdio");
   return 0;
}


/* wrap around unidirectional xioopensingle and xioopen_fd to automatically determine stdin or stdout fd depending on rw.
   Do not set FD_CLOEXEC flag. */
static int xioopen_stdio(int argc, const char *argv[], struct opt *opts, int xioflags, xiofile_t *xfd, unsigned groups, int dummy1, int dummy2, int dummy3) {
   int rw = (xioflags&XIO_ACCMODE);

   if (argc != 1) {
      Error2("%s: wrong number of parameters (%d instead of 0)", argv[0], argc-1);
   }

   if (rw == XIO_RDWR) {
      return xioopen_stdio_bi(xfd);
   }

   Notice2("using %s for %s",
	   &("stdin\0\0\0stdout"[rw<<3]),
	   ddirection[rw]);
   return xioopen_fd(opts, rw, xfd,
		     XIOWITHRD(rw)?0:-1,
		     XIOWITHWR(rw)?1:-1,
		     dummy2, dummy3);
}

/* wrap around unidirectional xioopensingle and xioopen_fd to automatically determine stdin or stdout fd depending on rw.
   Do not set FD_CLOEXEC flag. */
static int xioopen_stdfd(int argc, const char *argv[], struct opt *opts, int xioflags, xiofile_t *xfd, unsigned groups, int fd, int dummy2, int dummy3) {
   int rw = (xioflags&XIO_ACCMODE);

   if (argc != 1) {
      Error2("%s: wrong number of parameters (%d instead of 0)", argv[0], argc-1);
   }
   Notice2("using %s for %s",
	   &("stdin\0\0\0stdout\0\0stderr"[fd<<3]),
	   ddirection[rw]);
   return xioopen_fd(opts, rw, xfd,
		     XIOWITHRD(rw)?fd:-1,
		     XIOWITHWR(rw)?fd:-1,
		     dummy2, dummy3);
}
#endif /* WITH_STDIO */
