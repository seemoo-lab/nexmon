/* source: xio-test.c */
/* Copyright Gerhard Rieger 2007-2009 */
/* Published under the GNU General Public License V.2, see file COPYING */

/* this file contains the source for an intermediate test address that appends
   '>' to every block transferred from right to left, and '<' in the other
   direction */

#include "xiosysincludes.h"
#include "xioopen.h"

#include "xio-test.h"


#if WITH_TEST

static int xioopen_test(int argc, const char *argv[], struct opt *opts,
				  int xioflags, xiofile_t *xxfd,
				  unsigned groups, int dummy1, int dummy2,
				  int dummy3);
static int xioopen_testuni(int argc, const char *argv[], struct opt *opts,
				  int xioflags, xiofile_t *xxfd,
				  unsigned groups, int dummy1, int dummy2,
				  int dummy3);
static int xioopen_testrev(int argc, const char *argv[], struct opt *opts,
				  int xioflags, xiofile_t *xxfd,
				  unsigned groups, int dummy1, int dummy2,
				  int dummy3);

static const struct xioaddr_inter_desc xiointer_test0ro = { XIOADDR_PROT, "test", 0, XIOBIT_RDONLY, 0/*groups*/, XIOSHUT_UNSPEC, XIOCLOSE_UNSPEC, xioopen_test, 0, 0, 0, XIOBIT_WRONLY HELP("") };
static const struct xioaddr_inter_desc xiointer_test0wo = { XIOADDR_PROT, "test", 0, XIOBIT_WRONLY, 0/*groups*/, XIOSHUT_UNSPEC, XIOCLOSE_UNSPEC, xioopen_test, 0, 0, 0, XIOBIT_RDONLY HELP("") };
static const struct xioaddr_inter_desc xiointer_test0rw = { XIOADDR_PROT, "test", 0, XIOBIT_RDWR,   0/*groups*/, XIOSHUT_UNSPEC, XIOCLOSE_UNSPEC, xioopen_test, 0, 0, 0, XIOBIT_RDWR   HELP("") };

const union xioaddr_desc *xioaddrs_test[] = {
   (union xioaddr_desc *)&xiointer_test0ro,
   (union xioaddr_desc *)&xiointer_test0wo,
   (union xioaddr_desc *)&xiointer_test0rw,
   NULL };


static const struct xioaddr_inter_desc xiointer_testuni = { XIOADDR_PROT, "testuni", 0, XIOBIT_WRONLY, 0/*groups*/, XIOSHUT_CLOSE, XIOCLOSE_NONE, xioopen_testuni, 0, 0, 0, XIOBIT_RDONLY HELP("") };

const union xioaddr_desc *xioaddrs_testuni[] = {
   (union xioaddr_desc *)&xiointer_testuni,
   NULL };


static const struct xioaddr_inter_desc xiointer_testrev = { XIOADDR_PROT, "testrev", 0, XIOBIT_WRONLY, 0/*groups*/, XIOSHUT_CLOSE, XIOCLOSE_NONE, xioopen_testrev, 0, 0, 0, XIOBIT_RDONLY HELP("") };

const union xioaddr_desc *xioaddrs_testrev[] = {
   (union xioaddr_desc *)&xiointer_testrev,
   NULL };

static int xioopen_test(int argc, const char *argv[], struct opt *opts,
				  int xioflags, xiofile_t *xxfd,
				  unsigned groups, int dummy, int dummy2,
				  int dummy3) {
   struct single *xfd = &xxfd->stream;
   int result;

   assert(argc == 1);
   assert(!(xfd->rfd < 0 && xfd->wfd < 0));	/*!!!*/

   applyopts(-1, opts, PH_INIT);
   if (applyopts_single(xfd, opts, PH_INIT) < 0)  return -1;

   Notice("opening TEST");
   xfd->dtype = XIODATA_TEST;
   applyopts(xfd->rfd, opts, PH_ALL);
   if ((result = _xio_openlate(xfd, opts)) < 0)
      return result;
   return 0;
}

static int xioopen_testuni(int argc, const char *argv[], struct opt *opts,
				  int xioflags, xiofile_t *xxfd,
				  unsigned groups, int dummy, int dummy2,
				  int dummy3) {
   struct single *xfd = &xxfd->stream;
   int result;

   assert(argc == 1);
   assert(!(xfd->rfd < 0 && xfd->wfd < 0));	/*!!!*/

   applyopts(-1, opts, PH_INIT);
   if (applyopts_single(xfd, opts, PH_INIT) < 0)  return -1;

   Notice("opening TESTUNI");
   xfd->dtype = XIODATA_TESTUNI;
   applyopts(xfd->rfd, opts, PH_ALL);
   if ((result = _xio_openlate(xfd, opts)) < 0)
      return result;
   return 0;
}

static int xioopen_testrev(int argc, const char *argv[], struct opt *opts,
				  int xioflags, xiofile_t *xxfd,
				  unsigned groups, int dummy, int dummy2,
				  int dummy3) {
   struct single *xfd = &xxfd->stream;
   int result;

   assert(argc == 1);
   assert(!(xfd->rfd < 0 && xfd->wfd < 0));	/*!!!*/

   applyopts(-1, opts, PH_INIT);
   if (applyopts_single(xfd, opts, PH_INIT) < 0)  return -1;

   Notice("opening TESTREV");
   xfd->dtype = XIODATA_TESTREV;
   applyopts(xfd->rfd, opts, PH_ALL);
   if ((result = _xio_openlate(xfd, opts)) < 0)
      return result;
   return 0;
}

size_t xioread_test(struct single *sfd, void *buff, size_t bufsiz) {
   int fd = sfd->rfd;
   ssize_t bytes;
   int _errno;

   do {
      bytes = Read(fd, buff, bufsiz-1);
   } while (bytes < 0 && errno == EINTR);
   if (bytes < 0) {
      _errno = errno;
      switch (_errno) {
      case EPIPE: case ECONNRESET:
	 Warn4("read(%d, %p, "F_Zu"): %s",
	       fd, buff, bufsiz-1, strerror(_errno));
	 break;
      default:
	 Error4("read(%d, %p, "F_Zu"): %s",
		fd, buff, bufsiz-1, strerror(_errno));
      }
      return -1;
   }
   if (bytes == 0) {
      return 0;
   }
   ((char *)buff)[bytes] = '<';
   return bytes+1;
}

size_t xiowrite_test(struct single *sfd, const void *buff, size_t bytes) {
   int fd = sfd->wfd;
   void *buff1;
   ssize_t writt;
   int _errno;

   if ((buff1 = Malloc(bytes+1)) == NULL) {
      return -1;
   }
   memcpy(buff1, buff, bytes);
   ((char *)buff1)[bytes] = '>';
   do {
      writt = Write(fd, buff1, bytes+1);
   } while (writt < 0 && errno == EINTR);
   if (writt < 0) {
      _errno = errno;
      switch (_errno) {
      case EPIPE:
      case ECONNRESET:
	 if (sfd->cool_write) {
	    Notice4("write(%d, %p, "F_Zu"): %s",
		    fd, buff1, bytes+1, strerror(_errno));
	    break;
	 }
	 /*PASSTHROUGH*/
      default:
	 Error4("write(%d, %p, "F_Zu"): %s",
		fd, buff1, bytes+1, strerror(_errno));
      }
      errno = _errno;
      free(buff1);
      return -1;
   }
   if ((size_t)writt < bytes) {
      Warn2("write() only wrote "F_Zu" of "F_Zu" bytes",
	    writt, bytes+1);
   }
   free(buff1);
   return writt;
}

size_t xiowrite_testrev(struct single *sfd, const void *buff, size_t bytes) {
   int fd = sfd->wfd;
   void *buff1;
   ssize_t writt;
   int _errno;

   if ((buff1 = Malloc(bytes+1)) == NULL) {
      return -1;
   }
   memcpy(buff1, buff, bytes);
   ((char *)buff1)[bytes] = '<';
   do {
      writt = Write(fd, buff1, bytes+1);
   } while (writt < 0 && errno == EINTR);
   if (writt < 0) {
      _errno = errno;
      switch (_errno) {
      case EPIPE:
      case ECONNRESET:
	 if (sfd->cool_write) {
	    Notice4("write(%d, %p, "F_Zu"): %s",
		    fd, buff1, bytes+1, strerror(_errno));
	    break;
	 }
	 /*PASSTHROUGH*/
      default:
	 Error4("write(%d, %p, "F_Zu"): %s",
		fd, buff1, bytes+1, strerror(_errno));
      }
      errno = _errno;
      free(buff1);
      return -1;
   }
   if ((size_t)writt < bytes) {
      Warn2("write() only wrote "F_Zu" of "F_Zu" bytes",
	    writt, bytes+1);
   }
   free(buff1);
   return writt;
}

#endif /* WITH_TEST */

