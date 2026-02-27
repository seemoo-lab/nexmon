/* source: xioshutdown.c */
/* Copyright Gerhard Rieger */
/* Published under the GNU General Public License V.2, see file COPYING */

/* this is the source of the extended shutdown function */


#include "xiosysincludes.h"
#include "xioopen.h"


static int xioshut_sleep_kill(pid_t sub, unsigned long usec, int sig);

static pid_t socat_kill_pid;	/* here we pass the pid to be killed in sighandler */

static void signal_kill_pid(int dummy) {
   int _errno;
   _errno = errno;
   diag_in_handler = 1;
   Notice("SIGALRM while waiting for w/o child process to die, killing it now");
   Kill(socat_kill_pid, SIGTERM);
   diag_in_handler = 0;
   errno = _errno;
}

/* how: SHUT_RD, SHUT_WR, or SHUT_RDWR */
int xioshutdown(xiofile_t *sock, int how) {
   int fd;
   int result = 0;

   Debug2("xioshutdown(%p, %d)", sock, how);
   Debug2("xioshutdown(): dtype=0x%x, howtoshut=0x%04x",
	  sock->stream.dtype, sock->stream.howtoshut);

   if (sock->tag == XIO_TAG_INVALID) {
      Error("xioshutdown(): invalid file descriptor");
      errno = EINVAL;
      return -1;
   }

   /*Debug3("xioshutdown: flags=%d, dtype=%d, howtoclose=%d", sock->stream.flags, sock->stream.dtype, sock->stream.howtoclose);*/
   if (sock->tag == XIO_TAG_DUAL) {
      if ((how+1)&1) {
	 result = xioshutdown((xiofile_t *)sock->dual.stream[0], 0);
      }
      if ((how+1)&2) {
	 result |= xioshutdown((xiofile_t *)sock->dual.stream[1], 1);
      }
      return result;
   }

   fd = XIO_GETWRFD(sock);

   /* let us bring how nearer to the resulting action */
   if ((sock->stream.flags&XIO_ACCMODE) == XIO_WRONLY) {
      how = ((how+1) & ~(SHUT_RD+1)) - 1;
   } else if ((sock->stream.flags&XIO_ACCMODE) == XIO_RDONLY) {
      how = ((how+1) & ~(SHUT_WR+1)) - 1;
   }

   switch (sock->stream.howtoshut) {
#if WITH_PTY
   case XIOSHUT_PTYEOF:
      {
	 struct termios termarg;
	 int result;
	 Debug1("tcdrain(%d)", sock->stream.wfd);
	 result = tcdrain(sock->stream.wfd);
	 Debug1("tcdrain() -> %d", result);
	 if (Tcgetattr(sock->stream.wfd, &termarg) < 0) {
	    Error3("tcgetattr(%d, %p): %s",
		   sock->stream.wfd, &termarg, strerror(errno));
	 }
#if 0
	 /* these settings might apply to data still in the buff (despite the
	    TCSADRAIN */
	 termarg.c_iflag |= (IGNBRK | BRKINT | PARMRK | ISTRIP
                           | INLCR | IGNCR | ICRNL | IXON);
	 termarg.c_oflag |= OPOST;
	 termarg.c_lflag |= (/*ECHO | ECHONL |*/ ICANON | ISIG | IEXTEN);
	 //termarg.c_cflag |= (PARENB);
#else
	 termarg.c_lflag |= ICANON;
#endif
	 if (Tcsetattr(sock->stream.wfd, TCSADRAIN, &termarg) < 0) {
	    Error3("tcsetattr(%d, TCSADRAIN, %p): %s",
		   sock->stream.wfd, &termarg, strerror(errno));
	 }
	 if (Write(sock->stream.wfd, &termarg.c_cc[VEOF], 1) < 1) {
	    Warn3("write(%d, 0%o, 1): %s",
		  sock->stream.wfd, termarg.c_cc[VEOF], strerror(errno));
	 }
      }
      return 0;
#endif /* WITH_PTY */
#if WITH_OPENSSL
   case XIOSHUT_OPENSSL:
      sycSSL_shutdown(sock->stream.para.openssl.ssl);
      /*! what about half/full close? */
      return 0;
#endif /* WITH_OPENSSL */
   default:
      break;
   }

   /* here handle special shutdown functions */
   switch (sock->stream.howtoshut & XIOSHUTWR_MASK) {
      char writenull;
   case XIOSHUTWR_NONE:
      return 0;
   case XIOSHUTWR_CLOSE:
      if (Close(fd) < 0) {
	 Info2("close(%d): %s", fd, strerror(errno));
      }
      return 0;
   case XIOSHUTWR_DOWN:
      if ((result = Shutdown(fd, how)) < 0) {
	 Info3("shutdown(%d, %d): %s", fd, how, strerror(errno));
      }
      return 0;
#if _WITH_SOCKET
   case XIOSHUTWR_NULL:
      /* send an empty packet; only useful on datagram sockets? */
      xiowrite(sock, &writenull, 0);
      return 0;
#endif /* _WITH_SOCKET */
   default: break;
   }

#if 0
  if (how == SHUT_RDWR) {
     /* in this branch we handle only shutdown actions where read and write
	shutdown are not independent */

   switch (sock->stream.howtoshut) {
#if _WITH_SOCKET
     case XIOSHUT_DOWN:
      if ((result = Shutdown(fd, how)) < 0) {
	 Info3("shutdown(%d, %d): %s", fd, how, strerror(errno));
      }
      break;
     case XIOSHUT_KILL:
	if ((result = Shutdown(fd, how)) < 0) {
	 Info3("shutdown(%d, %d): %s", fd, how, strerror(errno));
      }
      break;
#endif /* _WITH_SOCKET */
     case XIOSHUT_CLOSE:
	Close(fd);
#if WITH_TERMIOS
	if (sock->stream.ttyvalid) {
	   if (Tcsetattr(fd, 0, &sock->stream.savetty) < 0) {
	      Warn2("cannot restore terminal settings on fd %d: %s",
		    fd, strerror(errno));
	   }
	}
#endif /* WITH_TERMIOS */
	/*PASSTHROUGH*/
     case XIOSHUT_NONE:
	break;
     default:
	Error1("xioshutdown(): bad shutdown action 0x%x", sock->stream.howtoshut);
	return -1;
     }

#if 0 && _WITH_SOCKET
   case XIODATA_RECVFROM:
      if (how >= 1) {
	 if (Close(fd) < 0) {
	    Info2("close(%d): %s",
		  fd, strerror(errno));
	 }
	 sock->stream.eof = 2;
	 sock->stream.rfd = -1;
      }
      break;
#endif /* _WITH_SOCKET */
  }
#endif

   if ((how+1) & 1) {	/* contains SHUT_RD */
      switch (sock->stream.dtype & XIODATA_READMASK) {
	 /* shutdown read channel */

      case XIOREAD_STREAM:
      case XIODATA_2PIPE:
	 if (Close(fd) < 0) {
	    Info2("close(%d): %s", fd, strerror(errno));
	 }
	 break;
      }
   }

   if ((how+1) & 2) {	/* contains SHUT_WR */
      /* shutdown write channel */

      switch (sock->stream.howtoshut & XIOSHUTWR_MASK) {

      case XIOSHUTWR_CLOSE:
	 if (Close(fd) < 0) {
	    Info2("close(%d): %s", fd, strerror(errno));
	 }
	 /*PASSTHROUGH*/
      case XIOSHUTWR_NONE:
	 break;

#if _WITH_SOCKET
      case XIOSHUTWR_DOWN:
	 if (Shutdown(fd, SHUT_WR) < 0) {
	    Info2("shutdown(%d, SHUT_WR): %s", fd, strerror(errno));
	 }
	 break;
#endif /* _WITH_SOCKET */

#if 0
      case XIOSHUTWR_DOWN_KILL:
	 if (Shutdown(fd, SHUT_WR) < 0) {
	    Info2("shutdown(%d, SHUT_WR): %s", fd, strerror(errno));
	 }
	 /*!!!*/
#endif
      case XIOSHUTWR_SIGHUP:
	 /* the child process might want to flush some data before
	    terminating */
	 xioshut_sleep_kill(sock->stream.child.pid, 0, SIGHUP);
	 break;
      case XIOSHUTWR_SIGTERM:
	 /* the child process might want to flush some data before
	    terminating */
	 xioshut_sleep_kill(sock->stream.child.pid, 1000000, SIGTERM);
	 break;
      case XIOSHUTWR_SIGKILL:
	 /* the child process might want to flush some data before
	    terminating */
	 xioshut_sleep_kill(sock->stream.child.pid, 1000000, SIGKILL);
	 break;

      default:
	 Error1("xioshutdown(): unhandled howtoshut=0x%x during SHUT_WR",
		sock->stream.howtoshut&XIOSHUTWR_MASK);
      }
      sock->stream.wfd = -1;
   }

   return result;
}

/* wait some time and then send signal to sub process. This is useful after
   shutting down the connection to give process some time to flush its output
   data */
static int xioshut_sleep_kill(pid_t sub, unsigned long usec, int sig) {
   struct sigaction act;
   int status = 0;

   /* we wait for the child process to die, but to prevent timeout
      we raise an alarm after some time.
      NOTE: the alarm does not terminate waitpid() on Linux/glibc
      (BUG?), 
      therefore we have to do the kill in the signal handler */
	 {
	    struct sigaction act;
	    sigfillset(&act.sa_mask);
	    act.sa_flags = 0;
	    act.sa_handler = signal_kill_pid;
	    Sigaction(SIGALRM, &act, NULL);
	 }

   socat_kill_pid = sub;
#if HAVE_SETITIMER
   /*! with next feature release, we get usec resolution and an option
	      */
#else
   Alarm(1 /*! sock->stream.child.waitdie */);
#endif /* !HAVE_SETITIMER */
   if (Waitpid(sub, &status, 0) < 0) {
      Warn3("waitpid("F_pid", %p, 0): %s",
	    sub, &status, strerror(errno));
   }
   Alarm(0);
   return 0;
}
