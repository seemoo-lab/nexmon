/* source: xio-fdnum.c */
/* Copyright Gerhard Rieger 2001-2007 */
/* Published under the GNU General Public License V.2, see file COPYING */

/* this file contains the source for opening addresses of fdnum type */

#include "xiosysincludes.h"
#include "xioopen.h"

#include "xio-fdnum.h"


#if WITH_FDNUM

static int xioopen_fdnum(int argc, const char *argv[], struct opt *opts, int rw, xiofile_t *xfd, unsigned groups, int dummy1, int dummy2, int dummy3);


const struct xioaddr_endpoint_desc xioaddr_fdnum1  = { XIOADDR_ENDPOINT, "fd", 1, XIOBIT_ALL,  GROUP_FD|GROUP_FIFO|GROUP_CHR|GROUP_BLK|GROUP_FILE|GROUP_SOCKET|GROUP_TERMIOS|GROUP_SOCK_UNIX|GROUP_SOCK_IP|GROUP_IPAPP, XIOSHUT_UNSPEC, XIOCLOSE_CLOSE, xioopen_fdnum, 0, 0, 0 HELP(":<num>") };
const struct xioaddr_endpoint_desc xioaddr_fdnum2  = { XIOADDR_ENDPOINT, "fd", 2, XIOBIT_RDWR, GROUP_FD|GROUP_FIFO|GROUP_CHR|GROUP_BLK|GROUP_FILE|GROUP_SOCKET|GROUP_TERMIOS|GROUP_SOCK_UNIX|GROUP_SOCK_IP|GROUP_IPAPP, XIOSHUT_UNSPEC, XIOCLOSE_CLOSE, xioopen_fdnum, 0, 0, 0 HELP(":<numout>:<numin>") };

const union xioaddr_desc *xioaddrs_fdnum[] = {
   (union xioaddr_desc *)&xioaddr_fdnum1,
   (union xioaddr_desc *)&xioaddr_fdnum2,
   NULL };

/* use some file descriptor and apply the options. Set the FD_CLOEXEC flag. */
static int xioopen_fdnum(int argc, const char *argv[], struct opt *opts,
			 int xioflags, xiofile_t *xfd, unsigned groups,
			 int dummy1, int dummy2, int dummy3) {
   char *a1;
   int rw = (xioflags&XIO_ACCMODE);
   int numfd1, numfd2 = -1;
   int numrfd, numwfd;
   int result;

   if (argc < 2 || argc > 3) {
      Error3("%s:%s: wrong number of parameters (%d instead of 1 or 2)", argv[0], argv[1], argc-1);
   }

   numfd1 = strtoul(argv[1], &a1, 0);
   if (*a1 != '\0') {
      Error1("error in FD number \"%s\"", argv[1]);
   }
   /* we dont want to see these fds in child processes */
   if (Fcntl_l(numfd1, F_SETFD, FD_CLOEXEC) < 0) {
      Warn2("fcntl(%d, F_SETFD, FD_CLOEXEC): %s", numfd1, strerror(errno));
   }

   if (argv[2]) {
      if (rw != XIO_RDWR) {
	 Warn("two file descriptors given for unidirectional transfer");
      }
      numwfd = numfd1;
      numrfd = strtoul(argv[2], &a1, 0);
      if (*a1 != '\0') {
	 Error1("error in FD number \"%s\"", argv[2]);
      }
      /* we dont want to see these fds in child processes */
      if (Fcntl_l(numrfd, F_SETFD, FD_CLOEXEC) < 0) {
	 Warn2("fcntl(%d, F_SETFD, FD_CLOEXEC): %s", numrfd, strerror(errno));
      }
   } else {
      if (XIOWITHWR(rw)) {
	 numrfd = numfd1;
	 numwfd = numfd1;
      } else {
	 numrfd = numfd1;
	 numwfd = -1;
      }
   }

   if (argv[2] == NULL) {
      Notice2("using file descriptor %d for %s",
	      numrfd>=0?numrfd:numwfd, ddirection[rw]);
   } else {
      Notice4("using file descriptors %d for %s and %d for %s", numrfd, ddirection[((rw+1)&1)-1], numwfd, ddirection[((rw+1)&2)-1]);
   }
   if ((result = xioopen_fd(opts, rw, xfd, numrfd, numwfd, dummy2, dummy3)) < 0) {
      return result;
   }
   return 0;
}

#endif /* WITH_FDNUM */

#if WITH_FD

/* retrieve and apply options to a standard file descriptor.
   Do not set FD_CLOEXEC flag. */
int xioopen_fd(struct opt *opts, int rw, xiofile_t *xfd, int numrfd, int numwfd, int dummy2, int dummy3) {
   int fd;
   struct stat buf;

   if (numwfd >= 0) {
      if (Fstat(numwfd, &buf) < 0) {
	 Warn2("fstat(%d, ): %s", numwfd, strerror(errno));
      }
      if ((buf.st_mode&S_IFMT) == S_IFSOCK &&
	  xfd->stream.howtoshut == XIOSHUT_UNSPEC) {
	 xfd->stream.howtoshut = XIOSHUT_DOWN;
      }
   }
   if (xfd->stream.howtoshut == XIOSHUT_UNSPEC)
      xfd->stream.howtoshut = XIOSHUT_CLOSE;
   
   xfd->stream.rfd = numrfd;
   xfd->stream.wfd = numwfd;
   if (numrfd >= 0) {
      fd = numrfd;
   } else {
      fd = numwfd;
   }

#if WITH_TERMIOS
   if (Isatty(fd)) {
      if (Tcgetattr(fd, &xfd->stream.savetty) < 0) {
	 Warn2("cannot query current terminal settings on fd %d: %s",
	       fd, strerror(errno));
      } else {
	 xfd->stream.ttyvalid = true;
      }
   }
#endif /* WITH_TERMIOS */
   if (applyopts_single(&xfd->stream, opts, PH_INIT) < 0)  return -1;
   applyopts(-1, opts, PH_INIT);

   applyopts2(fd, opts, PH_INIT, PH_FD);

   return _xio_openlate(&xfd->stream, opts);
}

#endif /* WITH_FD */
