/* source: xioclose.c */
/* Copyright Gerhard Rieger 2001-2008 */
/* Published under the GNU General Public License V.2, see file COPYING */

/* this is the source of the extended close function */


#include "xiosysincludes.h"
#include "xioopen.h"
#include "xiolockfile.h"

#include "xio-termios.h"


/* close the xio fd; must be valid and "simple" (not dual) */
int xioclose1(struct single *pipe) {

   if (pipe->tag == XIO_TAG_INVALID) {
      Notice("xioclose1(): invalid file descriptor");
      errno = EINVAL;
      return -1;
   }

   switch (pipe->howtoclose) {

#if WITH_READLINE
   case XIOCLOSE_READLINE:
      Write_history(pipe->para.readline.history_file);
      /*xiotermios_setflag(pipe->fd, 3, ECHO|ICANON);*/	/* error when pty closed */
      break;
#endif /* WITH_READLINE */

#if WITH_OPENSSL
   case XIOCLOSE_OPENSSL:
      if (pipe->para.openssl.ssl) {
	 /* e.g. on TCP connection refused, we do not yet have this set */
	 sycSSL_shutdown(pipe->para.openssl.ssl);
	 sycSSL_free(pipe->para.openssl.ssl);
	 pipe->para.openssl.ssl = NULL;
      }
      Close(pipe->rfd);  pipe->rfd = -1;
      Close(pipe->wfd);  pipe->wfd = -1;
      if (pipe->para.openssl.ctx) {
	 sycSSL_CTX_free(pipe->para.openssl.ctx);
	 pipe->para.openssl.ctx = NULL;
      }
      break;
#endif /* WITH_OPENSSL */

   case XIOCLOSE_SIGTERM:
      if (pipe->child.pid > 0) {
	 if (Kill(pipe->child.pid, SIGTERM) < 0) {
	    Msg2(errno==ESRCH?E_INFO:E_WARN, "kill(%d, SIGTERM): %s",
		 pipe->child.pid, strerror(errno));
	 }
      }
      break;
   case XIOCLOSE_CLOSE_SIGTERM:
      if (pipe->child.pid > 0) {
	    if (Kill(pipe->child.pid, SIGTERM) < 0) {
	       Msg2(errno==ESRCH?E_INFO:E_WARN, "kill(%d, SIGTERM): %s",
		    pipe->child.pid, strerror(errno));
	    }
      }
      /*PASSTHROUGH*/
   case XIOCLOSE_CLOSE:
      if (XIOWITHRD(pipe->flags) && pipe->rfd >= 0) {
	 if (Close(pipe->rfd) < 0) {
	    Info2("close(%d): %s", pipe->rfd, strerror(errno));
	 }
      }
      if (XIOWITHWR(pipe->flags) && pipe->wfd >= 0) {
	 if (Close(pipe->wfd) < 0) {
	    Info2("close(%d): %s", pipe->wfd, strerror(errno));
	 }
      }
      break;

   case XIOCLOSE_SLEEP_SIGTERM:
      Usleep(250000);
      if (pipe->child.pid > 0) {
	    if (Kill(pipe->child.pid, SIGTERM) < 0) {
	       Msg2(errno==ESRCH?E_INFO:E_WARN, "kill(%d, SIGTERM): %s",
		    pipe->child.pid, strerror(errno));
	    }
      }
      break;

   case XIOCLOSE_NONE:
      break;

   case XIOCLOSE_UNSPEC:
      Warn1("xioclose(): no close action specified on 0x%x", pipe);
      break;

   default:
      Error2("xioclose(): bad close action 0x%x on 0x%x", pipe->howtoclose, pipe);
      break;
   }

#if WITH_TERMIOS
   if (pipe->ttyvalid) {
      if (Tcsetattr(pipe->rfd, 0, &pipe->savetty) < 0) {
	 Warn2("cannot restore terminal settings on fd %d: %s",
	       pipe->rfd, strerror(errno));
      }
   }
#endif /* WITH_TERMIOS */

   /* unlock */
   if (pipe->havelock) {
      xiounlock(pipe->lock.lockfile);
      pipe->havelock = false;
   }      
   if (pipe->opt_unlink_close && pipe->unlink_close) {
      if (Unlink(pipe->unlink_close) < 0) {
	 Info2("unlink(\"%s\"): %s", pipe->unlink_close, strerror(errno));
      }
      free(pipe->unlink_close);
   }

   pipe->tag = XIO_TAG_INVALID;
   return 0;	/*! */
}


/* close the xio fd */
int xioclose(xiofile_t *file) {
   xiofile_t *xfd = file;
   int result;

   if (file->tag == XIO_TAG_INVALID) {
      Error("xioclose(): invalid file descriptor");
      errno = EINVAL;
      return -1;
   }

   if (file->tag == XIO_TAG_DUAL) {
      result  = xioclose1(file->dual.stream[0]);
      result |= xioclose1(file->dual.stream[1]);
      file->tag = XIO_TAG_INVALID;
   } else {
      result = xioclose1(&file->stream);
   }
   if (xfd->tag != XIO_TAG_INVALID && xfd->stream.subthread != 0) {
      Pthread_join(xfd->stream.subthread, NULL);
   }
   return result;
}

