/* source: xio-gopen.c */
/* Copyright Gerhard Rieger 2001-2012 */
/* Published under the GNU General Public License V.2, see file COPYING */

/* this file contains the source for opening addresses of generic open type */

#include "xiosysincludes.h"
#include "xioopen.h"

#include "xio-named.h"
#include "xio-unix.h"
#include "xio-gopen.h"


#if WITH_GOPEN

static int xioopen_gopen1(int argc, const char *argv[], struct opt *opts, int xioflags, xiofile_t *xfd, unsigned groups, int dummy1, int dummy2, int dummy3);


const struct xioaddr_endpoint_desc xioaddr_gopen1  = { XIOADDR_SYS, "gopen", 1, XIOBIT_ALL, GROUP_FD|GROUP_FIFO|GROUP_BLK|GROUP_REG|GROUP_NAMED|GROUP_OPEN|GROUP_FILE|GROUP_TERMIOS|GROUP_SOCKET|GROUP_SOCK_UNIX, XIOSHUT_UNSPEC, XIOCLOSE_UNSPEC, xioopen_gopen1, 0, 0, 0 HELP(":<filename>") };

const union xioaddr_desc *xioaddrs_gopen[] = {
   (union xioaddr_desc *)&xioaddr_gopen1,
   NULL
};

static int xioopen_gopen1(int argc, const char *argv[], struct opt *opts, int xioflags, xiofile_t *xfd, unsigned groups, int dummy1, int dummy2, int dummy3) {
   const char *filename = argv[1];
   int fd;
   int rw = (xioflags & XIO_ACCMODE);
   flags_t openflags = (xioflags & XIO_ACCMODE);
   mode_t st_mode;
   bool exists;
   bool opt_unlink_close = false;
   int result;

   if ((result =
	  _xioopen_named_early(argc, argv, xfd, GROUP_NAMED|groups, &exists, opts)) < 0) {
      return result;
   }
   st_mode = result;

   if (exists) {
      /* file (or at least named entry) exists */
      if ((xioflags&XIO_ACCMODE) != XIO_RDONLY) {
	 openflags |= O_APPEND;
      }
   } else {
      openflags |= O_CREAT;
   }

   /* note: when S_ISSOCK was undefined, it always gives 0 */
   if (exists && S_ISSOCK(st_mode)) {
#if WITH_UNIX
      union sockaddr_union us;
      socklen_t uslen = sizeof(us);
      char infobuff[256];

      Info1("\"%s\" is a socket, connecting to it", filename);

      xfd->stream.howtoshut = XIOSHUT_DOWN;
      xfd->stream.howtoclose = XIOCLOSE_CLOSE;
      result =
	 _xioopen_unix_client(&xfd->stream, xioflags, groups, 0, opts, filename);
      if (result < 0) {
	 return result;
      }
      if (xfd->stream.rfd >= 0) {
	 fd = xfd->stream.rfd;
      } else {
	 fd = xfd->stream.wfd;
      }
      
      applyopts_named(filename, opts, PH_PASTOPEN);	/* unlink-late */

      if (Getsockname(fd, (struct sockaddr *)&us, &uslen) < 0) {
	 Warn4("getsockname(%d, %p, {%d}): %s",
	       fd, &us, uslen, strerror(errno));
      } else {
	 Notice1("successfully connected via %s",
		 sockaddr_unix_info(&us.un, uslen,
				    infobuff, sizeof(infobuff)));
      }
#else
      Error("\"%s\" is a socket, but UNIX socket support is not compiled in");
      return -1;
#endif /* WITH_UNIX */

   } else {
      /* a file name */

      Info1("\"%s\" is not a socket, open()'ing it", filename);

      retropt_bool(opts, OPT_UNLINK_CLOSE, &opt_unlink_close);
      if (opt_unlink_close) {
	 if ((xfd->stream.unlink_close = strdup(filename)) == NULL) {
	    Error1("strdup(\"%s\"): out of memory", filename);
	 }
	 xfd->stream.opt_unlink_close = true;
      }
      if (xfd->stream.howtoshut == XIOSHUT_UNSPEC)
	 xfd->stream.howtoshut = XIOSHUT_NONE;
      if (xfd->stream.howtoclose == XIOCLOSE_UNSPEC)
	 xfd->stream.howtoclose = XIOCLOSE_CLOSE;

      Notice3("opening %s \"%s\" for %s",
	      filetypenames[(st_mode&S_IFMT)>>12], filename, ddirection[(xioflags&XIO_ACCMODE)]);
      if ((fd = _xioopen_open(filename, openflags, opts)) < 0)
	 return fd;
#ifdef I_PUSH
      if (S_ISCHR(st_mode)) {
	 Ioctl(result, I_PUSH, "ptem");
	 Ioctl(result, I_PUSH, "ldterm");
	 Ioctl(result, I_PUSH, "ttcompat");
      }
#endif

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
      applyopts_named(filename, opts, PH_FD);
      applyopts(fd, opts, PH_FD);
      applyopts_cloexec(fd, opts);
      if (XIOWITHRD(rw))  xfd->stream.rfd = fd;
      if (XIOWITHWR(rw))  xfd->stream.wfd = fd;
   }

   if ((result = applyopts2(fd, opts, PH_PASTSOCKET, PH_CONNECTED)) < 0) 
      return result;

   if ((result = _xio_openlate(&xfd->stream, opts)) < 0)
      return result;

   return 0;
}

#endif /* WITH_GOPEN */
