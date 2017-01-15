/* source: xioengine.c */
/* Copyright Gerhard Rieger */
/* Published under the GNU General Public License V.2, see file COPYING */

/* this is the source file of the socat transfer loop/engine */

#include "xiosysincludes.h"

#include "xioopen.h"
#include "xiosigchld.h"


/* checks if this is a connection to a child process, and if so, sees if the
   child already died, leaving some data for us.
   returns <0 if an error occurred;
   returns 0 if no child or not yet died or died without data (sets eof);
   returns >0 if child died and left data
*/
int childleftdata(xiofile_t *xfd) {
   struct pollfd in;
   int retval;

   /* have to check if a child process died before, but left read data */
   if (XIO_READABLE(xfd) &&
       (XIO_RDSTREAM(xfd)->howtoclose == XIOCLOSE_SIGTERM ||
	XIO_RDSTREAM(xfd)->howtoclose == XIOCLOSE_SIGKILL ||
	XIO_RDSTREAM(xfd)->howtoclose == XIOCLOSE_CLOSE_SIGTERM ||
	XIO_RDSTREAM(xfd)->howtoclose == XIOCLOSE_CLOSE_SIGKILL) &&
       XIO_RDSTREAM(xfd)->child.pid == 0) {
      struct timeval timeout = { 0,0 };

      if (XIO_READABLE(xfd) && !(XIO_RDSTREAM(xfd)->eof >= 2 && !XIO_RDSTREAM(xfd)->ignoreeof)) {
	 in.fd = XIO_GETRDFD(xfd);
	 in.events = POLLIN/*|POLLRDBAND*/;
	 in.revents = 0;
      }
      do {
	 int _errno;
	 retval = xiopoll(&in, 1, &timeout);
	 _errno = errno; diag_flush(); errno = _errno;	/* just in case it's not debug level and Msg() not been called */
      } while (retval < 0 && errno == EINTR);

      if (retval < 0) {
         Error5("xiopoll({%d,%0o}, 1, {"F_tv_sec"."F_tv_usec"}): %s",
                in.fd, in.events, timeout.tv_sec, timeout.tv_usec,
                strerror(errno));
         return -1;
      }
      if (retval == 0) {
         Info("terminated child did not leave data for us");
         XIO_RDSTREAM(xfd)->eof = 2;
         xfd->stream.eof = 2;
         xfd->stream.closing = MAX(xfd->stream.closing, 1);
      }
   }
   return 0;
}

void *xioengine(void *thread_arg) {
   struct threadarg_struct *engine_arg = thread_arg;

   _socat(engine_arg->xfd1, engine_arg->xfd2);
   free(engine_arg);
   return NULL/*!*/;
}

/* here we come when the sockets are opened (in the meaning of C language),
   and their options are set/applied
   returns -1 on error or 0 on success */
int _socat(xiofile_t *xfd1, xiofile_t *xfd2) {
   xiofile_t *sock1, *sock2;
   struct pollfd fds[4],
       *fd1in  = &fds[0],
       *fd1out = &fds[1],
       *fd2in  = &fds[2],
       *fd2out = &fds[3];
   int retval;
   unsigned char *buff;
   ssize_t bytes1, bytes2;
   int polling = 0;	/* handling ignoreeof */
   int wasaction = 1;	/* last select was active, do NOT sleep before next */
   struct timeval total_timeout;	/* the actual total timeout timer */
   bool mayrd1 = false;	/* sock1 has read data or eof, according to select() */
   bool mayrd2 = false;	/* sock2 has read data or eof, according to select() */
   bool maywr1 = false;	/* sock1 can be written to, according to select() */
   bool maywr2 = false;	/* sock2 can be written to, according to select() */

   sock1 = xfd1;
   sock2 = xfd2;

#if WITH_FILAN
   if (xioparams->debug) {
      int fdi, fdo;
      int msglevel, exitlevel;

      msglevel = diag_get_int('D');	/* save current message level */
      diag_set_int('D', E_ERROR);	/* only print errors and fatals in filan */
      exitlevel = diag_get_int('e');	/* save current exit level */
      diag_set_int('e', E_FATAL);	/* only exit on fatals */

      fdi = XIO_GETRDFD(sock1);
      fdo = XIO_GETWRFD(sock1);
      filan_fd(fdi, stderr);
      if (fdo != fdi) {
	 filan_fd(fdo, stderr);
      }

      fdi = XIO_GETRDFD(sock2);
      fdo = XIO_GETWRFD(sock2);
      filan_fd(fdi, stderr);
      if (fdo != fdi) {
	 filan_fd(fdo, stderr);
      }

      diag_set_int('e', exitlevel);	/* restore old exit level */
      diag_set_int('D', msglevel);	/* restore old message level */
   }
#endif /* WITH_FILAN */

   /* when converting nl to crnl, size might double */
   buff = Malloc(2*xioparams->bufsiz+1);
   if (buff == NULL)  return -1;

   if (xioparams->logopt == 'm' && xioinqopt('l', NULL, 0) == 'm') {
      Info("switching to syslog");
      diag_set('y', xioopts.syslogfac);
      xiosetopt('l', "\0");
   }
   total_timeout = xioparams->total_timeout;

   Notice4("starting data transfer loop with FDs [%d,%d] and [%d,%d]",
	   XIO_READABLE(sock1)?XIO_GETRDFD(sock1):-1,
	   XIO_WRITABLE(sock1)?XIO_GETWRFD(sock1):-1,
	   XIO_READABLE(sock2)?XIO_GETRDFD(sock2):-1,
	   XIO_WRITABLE(sock2)?XIO_GETWRFD(sock2):-1);
   while (XIO_RDSTREAM(sock1)->eof <= 1 ||
	  XIO_RDSTREAM(sock2)->eof <= 1) {
      struct timeval timeout, *to = NULL;

      Debug7("data loop: sock1->eof=%d, sock2->eof=%d, 1->closing=%d, 2->closing=%d, wasaction=%d, total_to={"F_tv_sec"."F_tv_usec"}",
	     XIO_RDSTREAM(sock1)->eof, XIO_RDSTREAM(sock2)->eof,
	     XIO_RDSTREAM(sock1)->closing, XIO_RDSTREAM(sock2)->closing,
	     wasaction, total_timeout.tv_sec, total_timeout.tv_usec);

      /* for ignoreeof */
      if (polling) {
	 if (!wasaction) {
	    /* yes we could do it with select but I like readable trace output */
	    if (xioparams->total_timeout.tv_sec != 0 ||
		xioparams->total_timeout.tv_usec != 0) {
	       if (total_timeout.tv_usec < xioparams->pollintv.tv_usec) {
		  total_timeout.tv_usec += 1000000;
		  total_timeout.tv_sec  -= 1;
	       }
	       total_timeout.tv_sec  -= xioparams->pollintv.tv_sec;
	       total_timeout.tv_usec -= xioparams->pollintv.tv_usec;
	       if (total_timeout.tv_sec < 0 ||
		   total_timeout.tv_sec == 0 && total_timeout.tv_usec < 0) {
		  Notice("inactivity timeout triggered");
		  xioclose(sock1);
		  xioclose(sock2);
		  free(buff);
		  return 0;
	       }
	    }

	 } else {
	    wasaction = 0;
	 }
      }

      if (polling) {
	 /* there is a ignoreeof poll timeout, use it */
	 timeout = xioparams->pollintv;
	 to = &timeout;
     } else if (xioparams->total_timeout.tv_sec != 0 ||
		 xioparams->total_timeout.tv_usec != 0) {
	 /* there might occur a total inactivity timeout */
	 timeout = xioparams->total_timeout;
	 to = &timeout;
      } else {
	 to = NULL;
      }

#if 1
      if (XIO_RDSTREAM(sock1)->closing>=1 || XIO_RDSTREAM(sock2)->closing>=1) {
	 /* first eof already occurred, start end timer */
	 timeout = xioparams->closwait;
	 to = &timeout;
	 /*0 closing = 2;*/
      }
#endif

      /* frame 1: set the poll parameters and loop over poll() EINTR) */
      do {
	 int _errno;

	 childleftdata(sock1);
	 childleftdata(sock2);

#if 0
	 if (closing>=1) {
	    /* first eof already occurred, start end timer */
	    timeout = xioparams->closwait;
	    to = &timeout;
	    closing = 2;
	 }
#else
	 if (XIO_RDSTREAM(sock1)->closing>=1 || XIO_RDSTREAM(sock2)->closing>=1) {
	    /* first eof already occurred, start end timer */
	    timeout = xioparams->closwait;
	    to = &timeout;
	    if (XIO_RDSTREAM(sock1)->closing==1) {
	       XIO_RDSTREAM(sock1)->closing = 2;
	    }
	    if (XIO_RDSTREAM(sock2)->closing==1) {
	       XIO_RDSTREAM(sock2)->closing = 2;
	    }
	 }
#endif

	 /* use the ignoreeof timeout if appropriate */
	 if (polling) {
	    if ((XIO_RDSTREAM(sock1)->closing == 0 && XIO_RDSTREAM(sock2)->closing == 0) ||
		(xioparams->pollintv.tv_sec < timeout.tv_sec) ||
		((xioparams->pollintv.tv_sec == timeout.tv_sec) &&
		 xioparams->pollintv.tv_usec < timeout.tv_usec)) {
	       timeout = xioparams->pollintv;
	    }
	 }

	 /* now the fds will be assigned */
	 if (XIO_READABLE(sock1) &&
	     !(XIO_RDSTREAM(sock1)->eof > 1 && !XIO_RDSTREAM(sock1)->ignoreeof)
	     /*0 && !xioparams->righttoleft*/) {
	    Debug3("*** sock1: %p [%d,%d]", sock1, XIO_GETRDFD(sock1), XIO_GETWRFD(sock1));
	    if (!mayrd1 && !(XIO_RDSTREAM(sock1)->eof > 1)) {
	       fd1in->fd = XIO_GETRDFD(sock1);
	       fd1in->events = POLLIN;
	    } else {
	       fd1in->fd = -1;
	    }
	    if (!maywr2) {
	       fd2out->fd = XIO_GETWRFD(sock2);
	       fd2out->events = POLLOUT;
	    } else {
	       fd2out->fd = -1;
	    }
	 } else {
	    fd1in->fd = -1;
	    fd2out->fd = -1;
         }
	 if (XIO_READABLE(sock2) &&
	     !(XIO_RDSTREAM(sock2)->eof > 1 && !XIO_RDSTREAM(sock2)->ignoreeof)
	     /*0 && !xioparams->lefttoright*/) {
	    Debug3("*** sock2: %p [%d,%d]", sock2, XIO_GETRDFD(sock2), XIO_GETWRFD(sock2));
	    if (!mayrd2 && !(XIO_RDSTREAM(sock2)->eof > 1)) {
	       fd2in->fd = XIO_GETRDFD(sock2);
	       fd2in->events = POLLIN;
	    } else {
	       fd2in->fd = -1;
	    }
	    if (!maywr1) {
	       fd1out->fd = XIO_GETWRFD(sock1);
	       fd1out->events = POLLOUT;
	    } else {
	       fd1out->fd = -1;
	    }
	 } else {
	    fd1out->fd = -1;
	    fd2in->fd = -1;
	 }
         /* frame 0: innermost part of the transfer loop: check FD status */
	 retval = xiopoll(fds, 4, to);
	 _errno = errno; diag_flush();	/* just in case it's not debug level and Msg() not been called */
	 if (retval >= 0 || _errno != EINTR) {
	    break;
	 }
	 Info1("xiopoll(): %s", strerror(errno));
	 errno = _errno;
      } while (true);

      /* attention:
	 when an exec'd process sends data and terminates, it is unpredictable
	 whether the data or the sigchild arrives first.
	 */

      if (retval < 0) {
	 Error11("xiopoll({%d,%0o}{%d,%0o}{%d,%0o}{%d,%0o}, 4, {"F_tv_sec"."F_tv_usec"}): %s",
		 fds[0].fd, fds[0].events, fds[1].fd, fds[1].events,
		 fds[2].fd, fds[2].events, fds[3].fd, fds[3].events,
		 timeout.tv_sec, timeout.tv_usec, strerror(errno));
	 free(buff);
	 return -1;
      } else if (retval == 0) {
	 Info2("poll timed out (no data within %ld.%06ld seconds)",
	       (XIO_RDSTREAM(sock1)->closing>=1||XIO_RDSTREAM(sock2)->closing>=1)?
	       xioparams->closwait.tv_sec:xioparams->total_timeout.tv_sec,
	       (XIO_RDSTREAM(sock1)->closing>=1||XIO_RDSTREAM(sock2)->closing>=1)?
	       xioparams->closwait.tv_usec:xioparams->total_timeout.tv_usec);
	 if (polling && !wasaction) {
	    /* there was a ignoreeof poll timeout, use it */
	    polling = 0;        /*%%%*/
	    if (XIO_RDSTREAM(sock1)->ignoreeof) {
	       mayrd1 = 0;
	    }
	    if (XIO_RDSTREAM(sock2)->ignoreeof) {
	       mayrd2 = 0;
	    }
         } else if (polling && wasaction) {
            wasaction = 0;

	 } else if (xioparams->total_timeout.tv_sec != 0 ||
		    xioparams->total_timeout.tv_usec != 0) {
	    /* there was a total inactivity timeout */
	    Notice("inactivity timeout triggered");
	    xioclose(sock1);
	    xioclose(sock2);
	    free(buff);
	    return 0;
	 }

	 if (XIO_RDSTREAM(sock1)->closing || XIO_RDSTREAM(sock2)->closing) {
	    break;
	 }
	 /* one possibility to come here is ignoreeof on some fd, but no EOF 
	    and no data on any descriptor - this is no indication for end! */
	 continue;
      }

      /*0 Debug1("XIO_READABLE(sock1) = %d", XIO_READABLE(sock1));*/
      /*0 Debug1("XIO_GETRDFD(sock1) = %d", XIO_GETRDFD(sock1));*/
      if (XIO_READABLE(sock1) && XIO_GETRDFD(sock1) >= 0 &&
	  (fd1in->revents /*&(POLLIN|POLLHUP|POLLERR)*/)) {
         if (fd1in->revents & POLLNVAL) {
            /* this is what we find on Mac OS X when poll()'ing on a device or
               named pipe. a read() might imm. return with 0 bytes, resulting
               in a loop? */ 
            Error1("poll(...[%d]: invalid request", fd1in->fd);
	    free(buff);
            return -1;
         }
 	 mayrd1 = true;
      }
      /*0 Debug1("XIO_READABLE(sock2) = %d", XIO_READABLE(sock2));*/
      /*0 Debug1("XIO_GETRDFD(sock2) = %d", XIO_GETRDFD(sock2));*/
      /*0 Debug1("FD_ISSET(XIO_GETRDFD(sock2), &in) = %d", FD_ISSET(XIO_GETRDFD(sock2), &in));*/
      if (XIO_READABLE(sock2) && XIO_GETRDFD(sock2) >= 0 &&
	  (fd2in->revents)) {
	 if (fd2in->revents & POLLNVAL) {
	    Error1("poll(...[%d]: invalid request", fd2in->fd);
	    free(buff);
	    return -1;
	 }
	 mayrd2 = true;
      }
      /*0 Debug2("mayrd2 = %d, maywr1 = %d", mayrd2, maywr1);*/
      if (XIO_GETWRFD(sock1) >= 0 && fd1out->fd >= 0 && fd1out->revents) {
	 if (fd1out->revents & POLLNVAL) {
	    Error1("poll(...[%d]: invalid request", fd1out->fd);
	    free(buff);
	    return -1;
	 }
	 maywr1 = true;
      }
      if (XIO_GETWRFD(sock2) >= 0 && fd2out->fd >= 0 && fd2out->revents) {
	 if (fd2out->revents & POLLNVAL) {
	    Error1("poll(...[%d]: invalid request", fd2out->fd);
	    free(buff);
	    return -1;
	 }
	 maywr2 = true;
      }

      if (mayrd1 && maywr2) {
	 mayrd1 = false;
	 if ((bytes1 = xiotransfer(sock1, sock2, &buff, xioparams->bufsiz, false))
	     < 0) {
	    if (errno != EAGAIN) {
	       /*XIO_RDSTREAM(sock2)->closing = MAX(XIO_RDSTREAM(socks2)->closing, 1);*/
	       Notice("socket 1 to socket 2 is in error");
	       if (/*0 xioparams->lefttoright*/ !XIO_READABLE(sock2)) {
		  break;
	       }
	    }
	 } else if (bytes1 > 0) {
	    maywr2 = false;
	    total_timeout = xioparams->total_timeout;
	    wasaction = 1;
	    /* is more data available that has already passed select()? */
	    mayrd1 = (xiopending(sock1) > 0);
	    if (XIO_RDSTREAM(sock1)->readbytes != 0 &&
		XIO_RDSTREAM(sock1)->actbytes == 0) {
	       /* avoid idle when all readbytes already there */
	       mayrd1 = true;
	    }          
	    /* escape char occurred? */
	    if (XIO_RDSTREAM(sock1)->actescape) {
	       bytes1 = 0;      /* indicate EOF */
	    }
	 }
	 if (bytes1 == 0) {
	    if (XIO_RDSTREAM(sock1)->ignoreeof && !XIO_RDSTREAM(sock1)->closing) {
	       ;
	    } else {
	       XIO_RDSTREAM(sock1)->eof = 2;
	    }
	    /* (bytes1 == 0)  handled later */
	 }
      } else {
	 bytes1 = -1;
      }

      if (mayrd2 && maywr1) {
	 mayrd2 = false;
	 if ((bytes2 = xiotransfer(sock2, sock1, &buff, xioparams->bufsiz, true))
	     < 0) {
	    if (errno != EAGAIN) {
	       /*XIO_RDSTREAM(sock1)->closing = MAX(XIO_RDSTREAM(sock1)->closing, 1);*/
	       Notice("socket 2 to socket 1 is in error");
	       if (/*0 xioparams->righttoleft*/ !XIO_READABLE(sock1)) {
		  break;
	       }
	    }
	 } else if (bytes2 > 0) {
	    maywr1 = false;
	    total_timeout = xioparams->total_timeout;
	    wasaction = 1;
	    /* is more data available that has already passed select()? */
	    mayrd2 = (xiopending(sock2) > 0);
	    if (XIO_RDSTREAM(sock2)->readbytes != 0 &&
		XIO_RDSTREAM(sock2)->actbytes == 0) {
	       /* avoid idle when all readbytes already there */
	       mayrd2 = true;
	    }          
	    /* escape char occurred? */
	    if (XIO_RDSTREAM(sock2)->actescape) {
	       bytes2 = 0;      /* indicate EOF */
	    }
	 }
	 if (bytes2 == 0) {
	    if (XIO_RDSTREAM(sock2)->ignoreeof && !XIO_RDSTREAM(sock2)->closing) {
	       ;
	    } else {
	       XIO_RDSTREAM(sock2)->eof = 2;
	    }
	    /* (bytes2 == 0)  handled later */
	 }
      } else {
	 bytes2 = -1;
      }

      /* NOW handle EOFs */

      if (bytes1 == 0 || XIO_RDSTREAM(sock1)->eof >= 2) {
	 if (XIO_RDSTREAM(sock1)->ignoreeof &&
	     !XIO_RDSTREAM(sock1)->actescape && !XIO_RDSTREAM(sock1)->closing) {
	    Debug1("socket 1 (fd %d) is at EOF, ignoring",
		   XIO_RDSTREAM(sock1)->rfd);	/*! */
            mayrd1 = true;
	    polling = 1;       /* do not hook this eof fd to poll for pollintv*/
	 } else {
	    Notice1("socket 1 (fd %d) is at EOF", XIO_GETRDFD(sock1));
	    xioshutdown(sock2, SHUT_WR);
	    XIO_RDSTREAM(sock1)->eof = 2;
	    XIO_RDSTREAM(sock1)->ignoreeof = false;
	 }
      } else if (polling && XIO_RDSTREAM(sock1)->ignoreeof) {
         polling = 0;
      }
      if (XIO_RDSTREAM(sock1)->eof >= 2) {
	 XIO_RDSTREAM(sock2)->closing = MAX(XIO_RDSTREAM(sock2)->closing, 1);
	 if (!XIO_READABLE(sock2)) {
	    break;
	 }
      }

      if (bytes2 == 0 || XIO_RDSTREAM(sock2)->eof >= 2) {
	 if (XIO_RDSTREAM(sock2)->ignoreeof &&
	     !XIO_RDSTREAM(sock2)->actescape && !XIO_RDSTREAM(sock2)->closing) {
	    Debug1("socket 2 (fd %d) is at EOF, ignoring",
		   XIO_RDSTREAM(sock2)->rfd);
	    mayrd2 = true;
	    polling = 1;       /* do not hook this eof fd to poll for pollintv*/
	 } else {
	    Notice1("socket 2 (fd %d) is at EOF", XIO_GETRDFD(sock2));
	    xioshutdown(sock1, SHUT_WR);
	    XIO_RDSTREAM(sock2)->eof = 2;
	    XIO_RDSTREAM(sock2)->ignoreeof = false;
	 }
      } else if (polling && XIO_RDSTREAM(sock2)->ignoreeof) {
         polling = 0;
      }
      if (XIO_RDSTREAM(sock2)->eof >= 2) {
	 XIO_RDSTREAM(sock1)->closing = MAX(XIO_RDSTREAM(sock1)->closing, 1);
	 if (!XIO_READABLE(sock1)) {
	    break;
	 }
      }
   }

   /* close everything that's still open */
   xioclose(sock1);
   xioclose(sock2);

   free(buff);
   return 0;
}


/* this is the callback when the child of an address died */
int socat_sigchild(struct single *file) {
   Debug3("socat_sigchild().1: file->ignoreeof=%d, file->closing=%d, file->eof=%d",
	  file->ignoreeof, file->closing, file->eof);
   if (file->ignoreeof && !file->closing) {
      ;
   } else {
      file->eof = MAX(file->eof, 1);
      file->closing = 3;
   }
   Debug3("socat_sigchild().9: file->ignoreeof=%d, file->closing=%d, file->eof=%d",
	  file->ignoreeof, file->closing, file->eof);
   return 0;
}
