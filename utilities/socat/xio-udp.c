/* source: xio-udp.c */
/* Copyright Gerhard Rieger */
/* Published under the GNU General Public License V.2, see file COPYING */

/* this file contains the source for handling UDP addresses */

#include "xiosysincludes.h"

#if WITH_UDP && (WITH_IP4 || WITH_IP6)

#include "xioopen.h"
#include "xio-socket.h"
#include "xio-ip4.h"
#include "xio-ip6.h"
#include "xio-ip.h"
#include "xio-ipapp.h"
#include "xio-tcpwrap.h"

#include "xio-udp.h"


static
int xioopen_udp_sendto(int argc, const char *argv[], struct opt *opts,
		     int xioflags, xiofile_t *xfd, unsigned groups,
		     int pf, int socktype, int ipproto);
static
int xioopen_udp_datagram(int argc, const char *argv[], struct opt *opts,
		     int xioflags, xiofile_t *xfd, unsigned groups,
		     int pf, int socktype, int ipproto);
static
int xioopen_udp_recvfrom(int argc, const char *argv[], struct opt *opts,
		       int xioflags, xiofile_t *xfd, unsigned groups,
		       int pf, int socktype, int ipproto);
static
int xioopen_udp_recv(int argc, const char *argv[], struct opt *opts,
		     int xioflags, xiofile_t *xfd, unsigned groups,
		     int pf, int socktype, int ipproto);
static
int _xioopen_udp_sendto(const char *hostname, const char *servname,
			struct opt *opts,
			int xioflags, xiofile_t *xxfd, unsigned groups,
			int pf, int socktype, int ipproto);

static const struct xioaddr_endpoint_desc xioaddr_udp_connect2  = { XIOADDR_SYS, "udp-connect",  2, XIOBIT_ALL,                GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP4|GROUP_SOCK_IP6|GROUP_IP_UDP, XIOSHUT_DOWN, XIOCLOSE_CLOSE, xioopen_ipapp_connect, SOCK_DGRAM, IPPROTO_UDP, PF_UNSPEC HELP(":<host>:<port>") };
const union xioaddr_desc *xioaddrs_udp_connect[]   = { (union xioaddr_desc *)&xioaddr_udp_connect2, NULL };
#if WITH_LISTEN
static const struct xioaddr_endpoint_desc xioaddr_udp_listen1   = { XIOADDR_SYS, "udp-listen",   1, XIOBIT_RDONLY|XIOBIT_RDWR,    GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP4|GROUP_SOCK_IP6|GROUP_IP_UDP|GROUP_LISTEN|GROUP_CHILD|GROUP_RANGE, XIOSHUT_DOWN, XIOCLOSE_CLOSE, xioopen_ipdgram_listen, PF_UNSPEC, IPPROTO_UDP, PF_UNSPEC HELP(":<port>") };
const union xioaddr_desc *xioaddrs_udp_listen[]    = { (union xioaddr_desc *)&xioaddr_udp_listen1, NULL };
#endif /* WITH_LISTEN */
static const struct xioaddr_endpoint_desc xioaddr_udp_sendto2   = { XIOADDR_SYS, "udp-sendto",   2, XIOBIT_WRONLY|XIOBIT_RDWR, GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP4|GROUP_SOCK_IP6|GROUP_IP_UDP, XIOSHUT_DOWN, XIOCLOSE_CLOSE, xioopen_udp_sendto, PF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP HELP(":<host>:<port>") };
const union xioaddr_desc *xioaddrs_udp_sendto[]    = { (union xioaddr_desc *)&xioaddr_udp_sendto2, NULL };
static const struct xioaddr_endpoint_desc xioaddr_udp_recvfrom1 = { XIOADDR_SYS, "udp-recvfrom", 1, XIOBIT_RDONLY|XIOBIT_RDWR, GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP4|GROUP_SOCK_IP6|GROUP_IP_UDP|GROUP_CHILD|GROUP_RANGE, XIOSHUT_NONE, XIOCLOSE_NONE, xioopen_udp_recvfrom,   PF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP HELP(":<port>") };
const union xioaddr_desc *xioaddrs_udp_recvfrom[]  = { (union xioaddr_desc *)&xioaddr_udp_recvfrom1, NULL };
static const struct xioaddr_endpoint_desc xioaddr_udp_recv1     = { XIOADDR_SYS, "udp-recv",     1, XIOBIT_RDONLY,             GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP4|GROUP_SOCK_IP6|GROUP_IP_UDP|GROUP_RANGE,     XIOSHUT_DOWN, XIOCLOSE_CLOSE, xioopen_udp_recv,     PF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP  HELP(":<port>") };
const union xioaddr_desc *xioaddrs_udp_recv[]      = { (union xioaddr_desc *)&xioaddr_udp_recv1, NULL };
static const struct xioaddr_endpoint_desc xioaddr_udp_datagram2 = { XIOADDR_SYS, "udp-datagram", 2, XIOBIT_ALL,                GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP4|GROUP_SOCK_IP6|GROUP_IP_UDP|GROUP_RANGE,     XIOSHUT_DOWN, XIOCLOSE_CLOSE, xioopen_udp_datagram, PF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP HELP(":<host>:<port>") };
const union xioaddr_desc *xioaddrs_udp_datagram[]  = { (union xioaddr_desc *)&xioaddr_udp_datagram2, NULL };

#if WITH_IP4
static const struct xioaddr_endpoint_desc xioaddr_udp4_connect2 = { XIOADDR_SYS, "udp4-connect", 2, XIOBIT_ALL,                GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP4|GROUP_IP_UDP, XIOSHUT_DOWN, XIOCLOSE_CLOSE, xioopen_ipapp_connect, SOCK_DGRAM, IPPROTO_UDP, PF_INET HELP(":<host>:<port>") };
const union xioaddr_desc *xioaddrs_udp4_connect[]  = { (union xioaddr_desc *)&xioaddr_udp4_connect2, NULL };
#if WITH_LISTEN
static const struct xioaddr_endpoint_desc xioaddr_udp4_listen1  = { XIOADDR_SYS, "udp4-listen",  1, XIOBIT_RDONLY|XIOBIT_RDWR, GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP4|GROUP_IP_UDP|GROUP_LISTEN|GROUP_CHILD|GROUP_RANGE, XIOSHUT_DOWN, XIOCLOSE_CLOSE, xioopen_ipdgram_listen, PF_INET, IPPROTO_UDP, PF_INET HELP(":<port>") };
const union xioaddr_desc *xioaddrs_udp4_listen[]   = { (union xioaddr_desc *)&xioaddr_udp4_listen1, NULL };
#endif /* WITH_LISTEN */
static const struct xioaddr_endpoint_desc xioaddr_udp4_sendto2  = { XIOADDR_SYS, "udp4-sendto",  2, XIOBIT_WRONLY|XIOBIT_RDWR, GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP4|GROUP_IP_UDP, XIOSHUT_DOWN, XIOCLOSE_CLOSE, xioopen_udp_sendto,   PF_INET, SOCK_DGRAM, IPPROTO_UDP  HELP(":<host>:<port>") };
const union xioaddr_desc *xioaddrs_udp4_sendto[]   = { (union xioaddr_desc *)&xioaddr_udp4_sendto2, NULL };
static const struct xioaddr_endpoint_desc xioaddr_udp4_datagram2= { XIOADDR_SYS, "udp4-datagram",2, XIOBIT_ALL,                GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP4|GROUP_IP_UDP, XIOSHUT_DOWN, XIOCLOSE_CLOSE, xioopen_udp_datagram,   PF_INET, SOCK_DGRAM, IPPROTO_UDP  HELP(":<host>:<port>") };
const union xioaddr_desc *xioaddrs_udp4_datagram[] = { (union xioaddr_desc *)&xioaddr_udp4_datagram2, NULL };
static const struct xioaddr_endpoint_desc xioaddr_udp4_recvfrom1= { XIOADDR_SYS, "udp4-recvfrom",1, XIOBIT_RDONLY|XIOBIT_RDWR, GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP4|GROUP_IP_UDP|GROUP_CHILD|GROUP_RANGE, XIOSHUT_NONE, XIOCLOSE_NONE, xioopen_udp_recvfrom, PF_INET, SOCK_DGRAM, IPPROTO_UDP  HELP(":<host>:<port>") };
const union xioaddr_desc *xioaddrs_udp4_recvfrom[] = { (union xioaddr_desc *)&xioaddr_udp4_recvfrom1, NULL };
static const struct xioaddr_endpoint_desc xioaddr_udp4_recv1    = { XIOADDR_SYS, "udp4-recv",    1, XIOBIT_RDONLY,             GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP4|GROUP_IP_UDP|GROUP_RANGE,             XIOSHUT_DOWN, XIOCLOSE_CLOSE, xioopen_udp_recv,     PF_INET, SOCK_DGRAM, IPPROTO_UDP  HELP(":<port>") };
const union xioaddr_desc *xioaddrs_udp4_recv[]     = { (union xioaddr_desc *)&xioaddr_udp4_recv1, NULL };
#endif /* WITH_IP4 */

#if WITH_IP6
static const struct xioaddr_endpoint_desc xioaddr_udp6_connect2 = { XIOADDR_SYS, "udp6-connect", 2, XIOBIT_ALL,                GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP6|GROUP_IP_UDP, XIOSHUT_DOWN, XIOCLOSE_CLOSE, xioopen_ipapp_connect, SOCK_DGRAM, IPPROTO_UDP, PF_INET6 HELP(":<host>:<port>") };
const union xioaddr_desc *xioaddrs_udp6_connect[]  = { (union xioaddr_desc *)&xioaddr_udp6_connect2, NULL };
#if WITH_LISTEN
static const struct xioaddr_endpoint_desc xioaddr_udp6_listen1  = { XIOADDR_SYS, "udp6-listen",  1, XIOBIT_RDONLY|XIOBIT_RDWR, GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP6|GROUP_IP_UDP|GROUP_LISTEN|GROUP_CHILD|GROUP_RANGE, XIOSHUT_DOWN, XIOCLOSE_CLOSE, xioopen_ipdgram_listen, PF_INET6, IPPROTO_UDP, 0 HELP(":<port>") };
const union xioaddr_desc *xioaddrs_udp6_listen[]   = { (union xioaddr_desc *)&xioaddr_udp6_listen1, NULL };
#endif /* WITH_LISTEN */
static const struct xioaddr_endpoint_desc xioaddr_udp6_sendto2  = { XIOADDR_SYS, "udp6-sendto",  2, XIOBIT_WRONLY|XIOBIT_RDWR, GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP6|GROUP_IP_UDP, XIOSHUT_DOWN, XIOCLOSE_CLOSE, xioopen_udp_sendto, PF_INET6, SOCK_DGRAM, IPPROTO_UDP HELP(":<host>:<port>") };
const union xioaddr_desc *xioaddrs_udp6_sendto[]   = { (union xioaddr_desc *)&xioaddr_udp6_sendto2, NULL };
static const struct xioaddr_endpoint_desc xioaddr_udp6_datagram2= { XIOADDR_SYS, "udp6-datagram",2, XIOBIT_ALL,                GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP6|GROUP_IP_UDP, XIOSHUT_DOWN, XIOCLOSE_CLOSE, xioopen_udp_datagram, PF_INET6, SOCK_DGRAM, IPPROTO_UDP HELP(":<host>:<port>") };
const union xioaddr_desc *xioaddrs_udp6_datagram[] = { (union xioaddr_desc *)&xioaddr_udp6_datagram2, NULL };
static const struct xioaddr_endpoint_desc xioaddr_udp6_recvfrom1= { XIOADDR_SYS, "udp6-recvfrom",1, XIOBIT_RDONLY|XIOBIT_RDWR, GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP6|GROUP_IP_UDP|GROUP_CHILD|GROUP_RANGE, XIOSHUT_NONE, XIOCLOSE_NONE, xioopen_udp_recvfrom, PF_INET6, SOCK_DGRAM, IPPROTO_UDP  HELP(":<port>") };
const union xioaddr_desc *xioaddrs_udp6_recvfrom[] = { (union xioaddr_desc *)&xioaddr_udp6_recvfrom1, NULL };
static const struct xioaddr_endpoint_desc xioaddr_udp6_recv1    = { XIOADDR_SYS, "udp6-recv",    1, XIOBIT_RDONLY,             GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP6|GROUP_IP_UDP|GROUP_RANGE,             XIOSHUT_DOWN, XIOCLOSE_CLOSE, xioopen_udp_recv,     PF_INET6, SOCK_DGRAM, IPPROTO_UDP  HELP(":<port>") };
const union xioaddr_desc *xioaddrs_udp6_recv[]     = { (union xioaddr_desc *)&xioaddr_udp6_recv1, NULL };
#endif /* WITH_IP6 */


/* we expect the form: port */
int xioopen_ipdgram_listen(int argc, const char *argv[], struct opt *opts,
			   int xioflags, xiofile_t *fd,
			  unsigned groups, int pf, int ipproto,
			  int protname) {
   int rw = (xioflags&XIO_ACCMODE);
   const char *portname = argv[1];
   union sockaddr_union us;
   union sockaddr_union themunion;
   union sockaddr_union *them = &themunion;
   int socktype = SOCK_DGRAM;
   struct pollfd readfd;
   bool dofork = false;
   pid_t pid;
   char *rangename;
   char infobuff[256];
   unsigned char buff1[1];
   socklen_t uslen;
   socklen_t themlen;
   int result;

   if (argc != 2) {
      Error2("%s: wrong number of parameters (%d instead of 1)", argv[0], argc-1);
   }

   if (pf == PF_UNSPEC) {
#if WITH_IP4 && WITH_IP6
      pf = xioopts.default_ip=='6'?PF_INET6:PF_INET;
#elif WITH_IP6
      pf = PF_INET6;
#else
      pf = PF_INET;
#endif
   }

   retropt_socket_pf(opts, &pf);

   if (applyopts_single(&fd->stream, opts, PH_INIT) < 0)  return -1;
   applyopts(-1, opts, PH_INIT);

   uslen = socket_init(pf, &us);
   retropt_bind(opts, pf, socktype, IPPROTO_UDP,
		(struct sockaddr *)&us, &uslen, 1,
		fd->stream.para.socket.ip.res_opts[1],
		fd->stream.para.socket.ip.res_opts[0]);

   if (false) {
      ;
#if WITH_IP4
   } else if (pf == PF_INET) {
      us.ip4.sin_port = parseport(portname, ipproto);
#endif
#if WITH_IP6
   } else if (pf == PF_INET6) {
      us.ip6.sin6_port = parseport(portname, ipproto);
#endif
   } else {
      Error1("xioopen_ipdgram_listen(): unknown address family %d", pf);
   }

   retropt_bool(opts, OPT_FORK, &dofork);

   if (dofork) {
      if (!(xioflags & XIO_MAYFORK)) {
	 Error("option fork not allowed here");
	 return STAT_NORETRY;
      }
   }

#if WITH_IP4 /*|| WITH_IP6*/
   if (retropt_string(opts, OPT_RANGE, &rangename) >= 0) {
      if (xioparserange(rangename, pf, &fd->stream.para.socket.range) < 0) {
	 free(rangename);
	 return STAT_NORETRY;
      }
      free(rangename);
      fd->stream.para.socket.dorange = true;
   }
#endif

#if WITH_LIBWRAP
   xio_retropt_tcpwrap(&fd->stream, opts);
#endif /* WITH_LIBWRAP */

   if (retropt_ushort(opts, OPT_SOURCEPORT, &fd->stream.para.socket.ip.sourceport)
       >= 0) {
      fd->stream.para.socket.ip.dosourceport = true;
   }
   retropt_bool(opts, OPT_LOWPORT, &fd->stream.para.socket.ip.lowport);

   if (dofork) {
      xiosetchilddied();	/* set SIGCHLD handler */
   }

   while (true) {	/* we loop with fork or prohibited packets */
      /* now wait for some packet on this datagram socket, get its sender
	 address, connect there, and return */
      int reuseaddr = dofork;
      int doreuseaddr = (dofork != 0);
      char infobuff[256];
      union sockaddr_union _sockname;
      union sockaddr_union *la = &_sockname;	/* local address */

      if ((fd->stream.rfd = xiosocket(opts, pf, socktype, ipproto, E_ERROR)) < 0) {
	 return STAT_RETRYLATER;
      }
      /*0 Info4("socket(%d, %d, %d) -> %d", pf, socktype, ipproto, fd->stream.fd);*/
      doreuseaddr |= (retropt_int(opts, OPT_SO_REUSEADDR, &reuseaddr) >= 0);
      applyopts(fd->stream.rfd, opts, PH_PASTSOCKET);
      if (doreuseaddr) {
	 if (Setsockopt(fd->stream.rfd, opt_so_reuseaddr.major,
			opt_so_reuseaddr.minor, &reuseaddr, sizeof(reuseaddr))
	     < 0) {
	    Warn6("setsockopt(%d, %d, %d, {%d}, "F_Zd"): %s",
		  fd->stream.rfd, opt_so_reuseaddr.major,
		  opt_so_reuseaddr.minor, reuseaddr, sizeof(reuseaddr),
		  strerror(errno));
	 }
      }
      applyopts_cloexec(fd->stream.rfd, opts);
      applyopts(fd->stream.rfd, opts, PH_PREBIND);
      applyopts(fd->stream.rfd, opts, PH_BIND);
      if (Bind(fd->stream.rfd, &us.soa, uslen) < 0) {
	 Error4("bind(%d, {%s}, "F_socklen"): %s", fd->stream.rfd,
		sockaddr_info(&us.soa, uslen, infobuff, sizeof(infobuff)),
		uslen, strerror(errno));
	 return STAT_RETRYLATER;
      }
      /* under some circumstances bind() fills sockaddr with interesting info. */
      if (Getsockname(fd->stream.rfd, &us.soa, &uslen) < 0) {
	 Error4("getsockname(%d, %p, {%d}): %s",
		fd->stream.rfd, &us.soa, uslen, strerror(errno));
      }
      applyopts(fd->stream.rfd, opts, PH_PASTBIND);

      Notice1("listening on UDP %s",
	      sockaddr_info(&us.soa, uslen, infobuff, sizeof(infobuff)));
      readfd.fd = fd->stream.rfd;
      readfd.events = POLLIN|POLLERR;
      while (xiopoll(&readfd, 1, NULL) < 0) {
	 if (errno != EINTR)  break;
      }

      themlen = socket_init(pf, them);
      do {
	 result = Recvfrom(fd->stream.rfd, buff1, 1, MSG_PEEK,
			     &them->soa, &themlen);
      } while (result < 0 && errno == EINTR);
      if (result < 0) {
	 Error5("recvfrom(%d, %p, 1, MSG_PEEK, {%s}, {"F_socklen"}): %s",
		fd->stream.rfd, buff1,
		sockaddr_info(&them->soa, themlen, infobuff, sizeof(infobuff)),
		themlen, strerror(errno));
	 return STAT_RETRYLATER;
      }
      Notice1("accepting UDP connection from %s",
	      sockaddr_info(&them->soa, themlen, infobuff, sizeof(infobuff)));

      if (xiocheckpeer(&fd->stream, them, la) < 0) {
	 /* drop packet */
	 char buff[512];
	 Recv(fd->stream.rfd, buff, sizeof(buff), 0);	/* drop packet */
	 Close(fd->stream.rfd);
	 continue;
      }
      Info1("permitting UDP connection from %s",
	    sockaddr_info(&them->soa, themlen, infobuff, sizeof(infobuff)));

      if (dofork) {
	 pid = xio_fork(false, E_ERROR);
	 if (pid < 0) {
	    return STAT_RETRYLATER;
	 }

	 if (pid == 0) {	/* child */
	    break;
	 }

	 /* server: continue loop with socket()+recvfrom() */
	 /* when we dont close this we get awkward behaviour on Linux 2.4:
	    recvfrom gives 0 bytes with invalid socket address */
	 if (Close(fd->stream.rfd) < 0) {
	    Info2("close(%d): %s", fd->stream.rfd, strerror(errno));
	 }

	 continue;
      }
      break;
   }

   applyopts(fd->stream.rfd, opts, PH_CONNECT);
   if ((result = Connect(fd->stream.rfd, &them->soa, themlen)) < 0) {
      Error4("connect(%d, {%s}, "F_socklen"): %s",
	     fd->stream.rfd,
	     sockaddr_info(&them->soa, themlen, infobuff, sizeof(infobuff)),
	     themlen, strerror(errno));
      return STAT_RETRYLATER;
   }

   /* set the env vars describing the local and remote sockets */
   if (Getsockname(fd->stream.rfd, &us.soa, &uslen) < 0) {
      Warn4("getsockname(%d, %p, {%d}): %s",
	    fd->stream.rfd, &us.soa, uslen, strerror(errno));
   }
   xiosetsockaddrenv("SOCK", &us,  uslen,   IPPROTO_UDP);
   xiosetsockaddrenv("PEER", them, themlen, IPPROTO_UDP);

   applyopts_fchown(fd->stream.rfd, opts);
   applyopts(fd->stream.rfd, opts, PH_LATE);

   if ((result = _xio_openlate(&fd->stream, opts)) < 0)
      return result;

   if (XIOWITHWR(rw))   fd->stream.wfd = fd->stream.rfd;
   if (!XIOWITHRD(rw))  fd->stream.rfd = -1;

   return 0;
}


static
int xioopen_udp_sendto(int argc, const char *argv[], struct opt *opts,
		     int xioflags, xiofile_t *xxfd, unsigned groups,
		     int pf, int socktype, int ipproto) {
   int result;

   if (argc != 3) {
      Error2("%s: wrong number of parameters (%d instead of 2)",
	     argv[0], argc-1);
      return STAT_NORETRY;
   }

   retropt_socket_pf(opts, &pf);
   if ((result = _xioopen_udp_sendto(argv[1], argv[2], opts, xioflags, xxfd,
				     groups, pf, socktype, ipproto))
       != STAT_OK) {
      return result;
   }
   _xio_openlate(&xxfd->stream, opts);
   return STAT_OK;
}

/*
   applies and consumes the following option:
   PH_INIT, PH_PASTSOCKET, PH_FD, PH_PREBIND, PH_BIND, PH_PASTBIND, PH_CONNECTED, PH_LATE
   OFUNC_OFFSET
   OPT_BIND, OPT_SOURCEPORT, OPT_LOWPORT, OPT_SO_TYPE, OPT_SO_PROTOTYPE, OPT_USER, OPT_GROUP, OPT_CLOEXEC
 */
static
int _xioopen_udp_sendto(const char *hostname, const char *servname,
			struct opt *opts,
		     int xioflags, xiofile_t *xxfd, unsigned groups,
		     int pf, int socktype, int ipproto) {
   xiosingle_t *xfd = &xxfd->stream;
   int rw = (xioflags&XIO_ACCMODE);
   union sockaddr_union us;
   socklen_t uslen;
   int feats = 3;	/* option bind supports address and port */
   bool needbind = false;
   int result;

   retropt_int(opts, OPT_SO_TYPE, &socktype);

   /* ...res_opts[] */
   if (applyopts_single(xfd, opts, PH_INIT) < 0)  return -1;
   applyopts(-1, opts, PH_INIT);

   xfd->salen = sizeof(xfd->peersa);
   if ((result =
	xiogetaddrinfo(hostname, servname, pf, socktype, ipproto,
		       &xfd->peersa, &xfd->salen,
		       xfd->para.socket.ip.res_opts[0],
		       xfd->para.socket.ip.res_opts[1]))
       != STAT_OK) {
      return result;
   }
   if (pf == PF_UNSPEC) {
      pf = xfd->peersa.soa.sa_family;
   }
   uslen = socket_init(pf, &us);
   if (retropt_bind(opts, pf, socktype, ipproto, &us.soa, &uslen, feats,
		    xfd->para.socket.ip.res_opts[0],
		    xfd->para.socket.ip.res_opts[1])
       != STAT_NOACTION) {
      needbind = true;
   }

   if (retropt_ushort(opts, OPT_SOURCEPORT,
		      &xfd->para.socket.ip.sourceport) >= 0) {
      switch (pf) {
#if WITH_IP4
      case PF_INET:
	 us.ip4.sin_port = htons(xfd->para.socket.ip.sourceport);
	 break;
#endif
#if WITH_IP6
      case PF_INET6:
	 us.ip6.sin6_port = htons(xfd->para.socket.ip.sourceport);
	 break;
#endif
      }
      needbind = true;
   }

   retropt_bool(opts, OPT_LOWPORT, &xfd->para.socket.ip.lowport);
   if (xfd->para.socket.ip.lowport) {
      switch (pf) {
#if WITH_IP4
      case PF_INET:
	 /*!!! this is buggy */
	 us.ip4.sin_port = htons(xfd->para.socket.ip.lowport); break;
#endif
#if WITH_IP6
      case PF_INET6: 
	 /*!!! this is buggy */
	 us.ip6.sin6_port = htons(xfd->para.socket.ip.lowport); break;
#endif
      }
      needbind = true;
   }

   xfd->dtype = XIODATA_RECVFROM;
   if ((result =
	_xioopen_dgram_sendto(needbind?&us:NULL, uslen,
			      opts, xioflags, xfd, groups,
			      pf, socktype, ipproto)) != STAT_OK) {
      return result;
   }
   if (XIOWITHWR(rw))   xfd->wfd = xfd->rfd;
   if (!XIOWITHRD(rw))  xfd->rfd = -1;
   return STAT_OK;
}


static
int xioopen_udp_datagram(int argc, const char *argv[], struct opt *opts,
		     int xioflags, xiofile_t *xxfd, unsigned groups,
		     int pf, int socktype, int ipproto) {
   xiosingle_t *xfd = &xxfd->stream;
   char *rangename;
   char *hostname;
   int result;

   if (argc != 3) {
      Error2("%s: wrong number of parameters (%d instead of 2)",
	     argv[0], argc-1);
      return STAT_NORETRY;
   }

   if ((hostname = strdup(argv[1])) == NULL) {
      Error1("strdup(\"%s\"): out of memory", argv[1]);
      return STAT_RETRYLATER;
   }

   retropt_socket_pf(opts, &pf);
   result =
      _xioopen_udp_sendto(hostname, argv[2], opts, xioflags, xxfd, groups,
			 pf, socktype, ipproto);
   free(hostname);
   if (result != STAT_OK) {
      return result;
   }

   xfd->dtype = XIOREAD_RECV|XIOWRITE_SENDTO;

   xfd->para.socket.la.soa.sa_family = xfd->peersa.soa.sa_family;

   /* only accept packets with correct remote ports */
   xfd->para.socket.ip.sourceport = ntohs(xfd->peersa.ip4.sin_port);
   xfd->para.socket.ip.dosourceport = true;

   /* which reply packets will be accepted - determine by range option */
   if (retropt_string(opts, OPT_RANGE, &rangename)
       >= 0) {
      if (xioparserange(rangename, pf, &xfd->para.socket.range) < 0) {
	 free(rangename);
	 return STAT_NORETRY;
      }
      xfd->para.socket.dorange = true;
      xfd->dtype |= XIOREAD_RECV_CHECKRANGE;
      free(rangename);
   }

#if WITH_LIBWRAP
   xio_retropt_tcpwrap(xfd, opts);
#endif /* WITH_LIBWRAP */

   _xio_openlate(xfd, opts);
   return STAT_OK;
}


static
int xioopen_udp_recvfrom(int argc, const char *argv[], struct opt *opts,
		     int xioflags, xiofile_t *xfd, unsigned groups,
		     int pf, int socktype, int ipproto) {
   union sockaddr_union us;
   socklen_t uslen = sizeof(us);
   int result;

   if (argc != 2) {
      Error2("%s: wrong number of parameters (%d instead of 1)",
	     argv[0], argc-1);
      return STAT_NORETRY;
   }

   retropt_socket_pf(opts, &pf);
   if (pf == PF_UNSPEC) {
#if WITH_IP4 && WITH_IP6
      pf = xioopts.default_ip=='6'?PF_INET6:PF_INET;
#elif WITH_IP6
      pf = PF_INET6;
#else
      pf = PF_INET;
#endif
   }

   if ((result =
	xiogetaddrinfo(NULL, argv[1], pf, socktype, ipproto,
		       &us, &uslen, xfd->stream.para.socket.ip.res_opts[0],
		       xfd->stream.para.socket.ip.res_opts[1]))
       != STAT_OK) {
      return result;
   }
   if (pf == PF_UNSPEC) {
      pf = us.soa.sa_family;
   }

   {
      union sockaddr_union la;
      socklen_t lalen = sizeof(la);
     
      if (retropt_bind(opts, pf, socktype, ipproto, &la.soa, &lalen, 1,
		       xfd->stream.para.socket.ip.res_opts[0],
		       xfd->stream.para.socket.ip.res_opts[1])
	  != STAT_NOACTION) {
	 switch (pf) {
#if WITH_IP4
	 case PF_INET:  us.ip4.sin_addr  = la.ip4.sin_addr;  break;
#endif
#if WITH_IP6
	 case PF_INET6: us.ip6.sin6_addr = la.ip6.sin6_addr; break;
#endif
	 }
      }
   }

   if (retropt_ushort(opts, OPT_SOURCEPORT, &xfd->stream.para.socket.ip.sourceport) >= 0) {
      xfd->stream.para.socket.ip.dosourceport = true;
   }
   retropt_bool(opts, OPT_LOWPORT, &xfd->stream.para.socket.ip.lowport);

   xfd->stream.dtype = XIODATA_RECVFROM_ONE;
   if ((result =
	_xioopen_dgram_recvfrom(&xfd->stream, xioflags, &us.soa, uslen,
				opts, pf, socktype, ipproto, E_ERROR))
       != STAT_OK) {
      return result;
   }
   _xio_openlate(&xfd->stream, opts);
   return STAT_OK;
}


static
int xioopen_udp_recv(int argc, const char *argv[], struct opt *opts,
		     int xioflags, xiofile_t *xfd, unsigned groups,
		     int pf, int socktype, int ipproto) {
   union sockaddr_union us;
   socklen_t uslen = sizeof(us);
   char *rangename;
   int result;

   if (argc != 2) {
      Error2("%s: wrong number of parameters (%d instead of 1)",
	     argv[0], argc-1);
      return STAT_NORETRY;
   }

   retropt_socket_pf(opts, &pf);
   if (pf == PF_UNSPEC) {
#if WITH_IP4 && WITH_IP6
      pf = xioopts.default_ip=='6'?PF_INET6:PF_INET;
#elif WITH_IP6
      pf = PF_INET6;
#else
      pf = PF_INET;
#endif
   }

   if ((result =
	xiogetaddrinfo(NULL, argv[1], pf, socktype, ipproto,
		       &us, &uslen, xfd->stream.para.socket.ip.res_opts[0],
		       xfd->stream.para.socket.ip.res_opts[1]))
       != STAT_OK) {
      return result;
   }
   if (pf == PF_UNSPEC) {
      pf = us.soa.sa_family;
   }

#if 1
   {
      union sockaddr_union la;
      socklen_t lalen = sizeof(la);
     
      if (retropt_bind(opts, pf, socktype, ipproto,
		       &xfd->stream.para.socket.la.soa, &lalen, 1,
		       xfd->stream.para.socket.ip.res_opts[0],
		       xfd->stream.para.socket.ip.res_opts[1])
	  != STAT_NOACTION) {
	 switch (pf) {
#if WITH_IP4
	 case PF_INET:
	    us.ip4.sin_addr  = xfd->stream.para.socket.la.ip4.sin_addr;  break;
#endif
#if WITH_IP6
	 case PF_INET6:
	    us.ip6.sin6_addr = xfd->stream.para.socket.la.ip6.sin6_addr; break;
#endif
	 }
      } else {
	 xfd->stream.para.socket.la.soa.sa_family = pf;
      }
   }
#endif

#if WITH_IP4 /*|| WITH_IP6*/
   if (retropt_string(opts, OPT_RANGE, &rangename) >= 0) {
      if (xioparserange(rangename, pf, &xfd->stream.para.socket.range) < 0) {
	 return STAT_NORETRY;
      }
      xfd->stream.para.socket.dorange = true;
   }
#endif

#if WITH_LIBWRAP
   xio_retropt_tcpwrap(&xfd->stream, opts);
#endif /* WITH_LIBWRAP */

   if (retropt_ushort(opts, OPT_SOURCEPORT,
		      &xfd->stream.para.socket.ip.sourceport)
       >= 0) {
      xfd->stream.para.socket.ip.dosourceport = true;
   }
   retropt_bool(opts, OPT_LOWPORT, &xfd->stream.para.socket.ip.lowport);

   xfd->stream.dtype = XIODATA_RECV;
   if ((result = _xioopen_dgram_recv(&xfd->stream, xioflags, &us.soa, uslen,
				     opts, pf, socktype, ipproto, E_ERROR))
       != STAT_OK) {
      return result;
   }
   _xio_openlate(&xfd->stream, opts);
   return result;
}

#endif /* WITH_UDP && (WITH_IP4 || WITH_IP6) */
