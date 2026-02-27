/* source: xiowrite.c */
/* Copyright Gerhard Rieger 2001-2012 */
/* Published under the GNU General Public License V.2, see file COPYING */

/* this is the source of the extended write function */


#include "xiosysincludes.h"
#include "xioopen.h"

#include "xio-test.h"
#include "xio-readline.h"
#include "xio-openssl.h"


/* ...
   note that the write() call can block even if the select()/poll() call
   reported the FD writeable: in case the FD is not nonblocking and a lock
   defers the operation.
   on return value < 0: errno reflects the value from write() */
ssize_t xiowrite(xiofile_t *file, const void *buff, size_t bytes) {
   ssize_t writt;
   struct single *pipe;
   int fd;
   int _errno;

   if (file->tag == XIO_TAG_INVALID) {
      Error1("xiowrite(): invalid xiofile descriptor %p", file);
      errno = EINVAL;
      return -1;
   }

   if (file->tag == XIO_TAG_DUAL) {
      pipe = file->dual.stream[1];
      if (pipe->tag == XIO_TAG_INVALID) {
	 Error1("xiowrite(): invalid xiofile sub descriptor %p[1]", file);
	 errno = EINVAL;
	 return -1;
      }
   } else {
      pipe = &file->stream;
   }

#if WITH_READLINE
   /* try to extract a prompt from the write data */
   if ((pipe->dtype & XIODATA_READMASK) == XIOREAD_READLINE) {
      xioscan_readline(pipe, buff, bytes);
   }
#endif /* WITH_READLINE */

   fd = XIO_GETWRFD(file);

   switch (pipe->dtype & XIODATA_WRITEMASK) {

   case XIOWRITE_STREAM:
      writt = writefull(pipe->wfd, buff, bytes);
      if (writt < 0) {
	 _errno = errno;
	 switch (_errno) {
	 case EPIPE:
	 case ECONNRESET:
	    if (pipe->cool_write) {
	       Notice4("write(%d, %p, "F_Zu"): %s",
		       fd, buff, bytes, strerror(_errno));
	       break;
	    }
	    /*PASSTRHOUGH*/
	 default:
	    Error4("write(%d, %p, "F_Zu"): %s",
		   fd, buff, bytes, strerror(_errno));
	 }
	 errno = _errno;
	 return -1;
      }
      break;

#if _WITH_SOCKET
   case XIOWRITE_SENDTO:
      /*union {
	 char space[sizeof(struct sockaddr_un)];
	 struct sockaddr sa;
	 } from;*/
      /*socklen_t fromlen;*/

      do {
	 writt = Sendto(fd, buff, bytes, 0,
			&pipe->peersa.soa, pipe->salen);
      } while (writt < 0 && errno == EINTR);
      if (writt < 0) {
	 char infobuff[256];
	 _errno = errno;
	 Error6("sendto(%d, %p, "F_Zu", 0, %s, "F_socklen"): %s",
		fd, buff, bytes, 
		sockaddr_info(&pipe->peersa.soa, pipe->salen,
			      infobuff, sizeof(infobuff)),
		pipe->salen, strerror(_errno));
	 errno = _errno;
	 return -1;
      }
      if ((size_t)writt < bytes) {
	 char infobuff[256];
	 Warn7("sendto(%d, %p, "F_Zu", 0, %s, "F_socklen") only wrote "F_Zu" of "F_Zu" bytes",
	       fd, buff, bytes, 
	       sockaddr_info(&pipe->peersa.soa, pipe->salen,
			     infobuff, sizeof(infobuff)),
	       pipe->salen, writt, bytes);
      } else {
      }
      {
	 char infobuff[256];
	 union sockaddr_union us;
	 socklen_t uslen = sizeof(us);
	 Getsockname(fd, &us.soa, &uslen);
	 Notice1("local address: %s",
		 sockaddr_info(&us.soa, uslen, infobuff, sizeof(infobuff)));
      }
      break;
#endif /* _WITH_SOCKET */

   case XIOWRITE_PIPE:
   case XIOWRITE_2PIPE:
      writt = Write(pipe->wfd, buff, bytes);
      _errno = errno;
      if (writt < 0) {
	 Error4("write(%d, %p, "F_Zu"): %s",
		fd, buff, bytes, strerror(_errno));
	 errno = _errno;
	 return -1;
      }
      break;

#if WITH_TEST
   case XIOWRITE_TEST:
      /* this function prints its own error messages */
      return xiowrite_test(pipe, buff, bytes);
   case XIOWRITE_TESTREV:
      /* this function prints its own error messages */
      return xiowrite_testrev(pipe, buff, bytes);
#endif /* WITH_TEST */

#if WITH_OPENSSL
   case XIOWRITE_OPENSSL:
      /* this function prints its own error messages */
      return xiowrite_openssl(pipe, buff, bytes);
#endif /* WITH_OPENSSL */

   default:
      Error1("xiowrite(): bad data type specification %d", pipe->dtype);
      errno = EINVAL;
      return -1;
   }
   return writt;
}
