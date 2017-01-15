/* source: xio-interface.c */
/* Copyright Gerhard Rieger 2008-2012 */
/* Published under the GNU General Public License V.2, see file COPYING */

/* this file contains the source for opening addresses of raw socket type */

#include "xiosysincludes.h"

#if WITH_INTERFACE

#include "xioopen.h"
#include "xio-socket.h"

#include "xio-interface.h"


static
int xioopen_interface(int argc, const char *argv[], struct opt *opts,
		      int xioflags, xiofile_t *xfd, unsigned groups, int dummy,
		      int dummy2, int dummy3);

const struct xioaddr_endpoint_desc xioaddr_interface1 = { XIOADDR_SYS, "interface", 1, XIOBIT_ALL, GROUP_FD|GROUP_SOCKET, XIOSHUT_DOWN, XIOCLOSE_CLOSE, xioopen_interface, 0, 0, 0 HELP(":<interface>") };
const union xioaddr_desc *xioaddrs_interface[] = {
   (union xioaddr_desc *)&xioaddr_interface1,
   NULL
};

static
int _xioopen_interface(const char *ifname,
		       struct opt *opts, int xioflags, xiofile_t *xxfd,
		       unsigned groups) {
   xiosingle_t *xfd = &xxfd->stream;
   int rw = (xioflags&XIO_ACCMODE);
   int pf = PF_PACKET;
   union sockaddr_union us = {{0}};
   socklen_t uslen;
   int socktype = SOCK_RAW;
   unsigned int ifidx;
   bool needbind = false;
   char *bindstring = NULL;
   struct sockaddr_ll sall = { 0 };
   int result;

   if (ifindex(ifname, &ifidx, -1) < 0) {
      Error1("unknown interface \"%s\"", ifname);
      ifidx = 0;	/* desparate attempt to continue */
   }

   retropt_int(opts, OPT_SO_TYPE, &socktype);

   retropt_socket_pf(opts, &pf);

   /* ...res_opts[] */
   if (applyopts_single(xfd, opts, PH_INIT) < 0)  return -1;
   applyopts(-1, opts, PH_INIT);

   xfd->salen = sizeof(xfd->peersa);
   if (pf == PF_UNSPEC) {
      pf = xfd->peersa.soa.sa_family;
   }

   xfd->dtype = XIODATA_RECVFROM_SKIPIP;

   if (retropt_string(opts, OPT_BIND, &bindstring)) {
      needbind = true;
   }
   /*!!! parse by ':' */
   us.ll.sll_family = pf;
   us.ll.sll_protocol = htons(ETH_P_ALL);
   us.ll.sll_ifindex = ifidx;
   uslen = sizeof(sall);
   needbind = true;
   xfd->peersa = (union sockaddr_union)us;

   if ((result =
	_xioopen_dgram_sendto(needbind?&us:NULL, uslen,
			      opts, xioflags, xfd, groups, pf, socktype, 0))
       != STAT_OK) {
      return result;
   }
   if (XIOWITHWR(rw))   xfd->wfd = xfd->rfd;
   if (!XIOWITHRD(rw))  xfd->rfd = -1;
   return result;
}

static
int xioopen_interface(int argc, const char *argv[], struct opt *opts,
		      int xioflags, xiofile_t *xxfd, unsigned groups,
		      int summy, int dummy2, int dummy3) {
   xiosingle_t *xfd = &xxfd->stream;
   int result;

   if (argc != 2) {
      Error2("%s: wrong number of parameters (%d instead of 1)",
	     argv[0], argc-1);
      return STAT_NORETRY;
   }

   if ((result =
	_xioopen_interface(argv[1], opts, xioflags, xxfd, groups))
       != STAT_OK) {
      return result;
   }

   xfd->dtype = XIOREAD_RECV|XIOWRITE_SENDTO;

   xfd->para.socket.la.soa.sa_family = xfd->peersa.soa.sa_family;

   _xio_openlate(xfd, opts);
   return STAT_OK;
}

#endif /* WITH_INTERFACE */
