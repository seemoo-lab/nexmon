/* source: xio-nop.c */
/* Copyright Gerhard Rieger 2006-2007 */
/* Published under the GNU General Public License V.2, see file COPYING */

/* this file contains the source for a degenerated address that just transfers
   data */

#include "xiosysincludes.h"
#include "xioopen.h"

#include "xio-nop.h"


#if WITH_NOP

static int xioopen_nop(int argc, const char *argv[], struct opt *opts,
				  int xioflags, xiofile_t *xxfd,
				  unsigned groups, int dummy1, int dummy2,
				  int dummy3);

static const struct xioaddr_inter_desc xiointer_nop0ro = { XIOADDR_PROT, "nop", 0, XIOBIT_RDONLY, 0/*groups*/, XIOSHUT_CLOSE, XIOCLOSE_NONE, xioopen_nop, 0, 0, 0, XIOBIT_WRONLY HELP("") };
static const struct xioaddr_inter_desc xiointer_nop0wo = { XIOADDR_PROT, "nop", 0, XIOBIT_WRONLY, 0/*groups*/, XIOSHUT_CLOSE, XIOCLOSE_NONE, xioopen_nop, 0, 0, 0, XIOBIT_RDONLY HELP("") };
static const struct xioaddr_inter_desc xiointer_nop0rw = { XIOADDR_PROT, "nop", 0, XIOBIT_RDWR,   0/*groups*/, XIOSHUT_CLOSE, XIOCLOSE_NONE, xioopen_nop, 0, 0, 0, XIOBIT_RDWR   HELP("") };

const union xioaddr_desc *xioaddrs_nop[] = {
   (union xioaddr_desc *)&xiointer_nop0ro,
   (union xioaddr_desc *)&xiointer_nop0wo,
   (union xioaddr_desc *)&xiointer_nop0rw,
   NULL };

static int xioopen_nop(int argc, const char *argv[], struct opt *opts,
				  int xioflags, xiofile_t *xxfd,
				  unsigned groups, int dummy, int dummy2,
				  int dummy3) {
   struct single *xfd = &xxfd->stream;
   int result;

   if (argc != 1) {
      Error("address NOP takes no arguments");
      return STAT_NORETRY;
   }

   if (xfd->rfd < 0 && xfd->wfd < 0) { /*!!!*/
      Error("NOP cannot be endpoint");
      return STAT_NORETRY;
   }

   applyopts(-1, opts, PH_INIT);
   if (applyopts_single(xfd, opts, PH_INIT) < 0)  return -1;

   Notice("opening NOP");

   xfd->dtype = XIODATA_STREAM;
   /*xfd->fdtype = FDTYPE_DOUBLE;*/

   applyopts(xfd->rfd, opts, PH_ALL);

   if ((result = _xio_openlate(xfd, opts)) < 0)
      return result;

   return 0;
}
#endif /* WITH_NOP */

