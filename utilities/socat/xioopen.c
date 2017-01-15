/* source: xioopen.c */
/* Copyright Gerhard Rieger */
/* Published under the GNU General Public License V.2, see file COPYING */

/* this is the source file of the extended open function */

#include "xiosysincludes.h"

#include "xioopen.h"
#include "xiomodes.h"
#include "nestlex.h"

#include "xiosigchld.h"

void *xioopenleftthenengine(void *thread_void);
static xiosingle_t *xioparse_single(const char **addr);

static int 
   xioopen_inter_single(xiofile_t *xfd, int xioflags);
static int 
   xioopen_endpoint_single(xiofile_t *xfd, int xioflags);
static int
   xioopen_unoverload(xiosingle_t *sfd,
		      int mayleft,	/* what may be on left side: or'd
					   XIOBIT_RDWR, XIOBIT_RDONLY,
					   XIOBIT_WRONLY */
		      int *isleft,	/* what the selected address desc
					   provides on left side: XIO_RDWR,
					   XIO_RDONLY, or XIO_WRONLY */
		      int mayright,	/* what may be on right side: or'd
					   XIOBIT_RDWR, XIOBIT_RDONLY,
					   XIOBIT_WRONLY */
		      int *isright);	/* what the selected address desc
					   provides on right side: XIO_RDWR,
					   XIO_RDONLY, or XIO_WRONLY */


const struct xioaddrname address_names[] = {
#if 1
#if WITH_STDIO
   { "-",		xioaddrs_stdio },
#endif
#if defined(WITH_UNIX) && defined(WITH_ABSTRACT_UNIXSOCKET)
   { "abstract",		xioaddrs_abstract_client },
   { "abstract-client",		xioaddrs_abstract_client },
   { "abstract-connect",	xioaddrs_abstract_connect },
#if WITH_LISTEN
   { "abstract-listen",		xioaddrs_abstract_listen },
#endif
   { "abstract-recv",		xioaddrs_abstract_recv },
   { "abstract-recvfrom",	xioaddrs_abstract_recvfrom },
   { "abstract-sendto",		xioaddrs_abstract_sendto },
#endif /* defined(WITH_UNIX) && defined(WITH_ABSTRACT_UNIXSOCKET) */
#if WITH_CREAT
   { "creat",		xioaddrs_creat },
   { "create",		xioaddrs_creat },
#endif
#if WITH_GENERICSOCKET
   { "datagram",		xioaddrs_socket_datagram },
   { "dgram",			xioaddrs_socket_datagram },
#endif
#if WITH_PIPE
   { "echo",		xioaddrs_pipe },
#endif
#if WITH_EXEC
   { "exec",		xioaddrs_exec },
   { "exec1",		xioaddrs_exec1 },
   { "exec2",		xioaddrs_exec },
#endif
#if WITH_FDNUM
   { "fd",		xioaddrs_fdnum },
#endif
#if WITH_PIPE
   { "fifo",		xioaddrs_pipe },
#endif
#if WITH_FILE
   { "file",		xioaddrs_open },
#endif
#if WITH_GOPEN
   { "gopen",		xioaddrs_gopen },
#endif
#if WITH_INTERFACE
   { "if",		xioaddrs_interface },
#endif
#if (WITH_IP4 || WITH_IP6) && WITH_TCP
   { "inet",		xioaddrs_tcp_connect },
#endif
#if (WITH_IP4 || WITH_IP6) && WITH_TCP && WITH_LISTEN
   { "inet-l",		xioaddrs_tcp_listen },
   { "inet-listen",	xioaddrs_tcp_listen },
#endif
#if WITH_IP4 && WITH_TCP
   { "inet4",		xioaddrs_tcp4_connect },
#endif
#if WITH_IP4 && WITH_TCP && WITH_LISTEN
   { "inet4-l",		xioaddrs_tcp4_listen },
   { "inet4-listen",	xioaddrs_tcp4_listen },
#endif
#if WITH_IP6 && WITH_TCP
   { "inet6",		xioaddrs_tcp6_connect },
#endif
#if WITH_IP6 && WITH_TCP && WITH_LISTEN
   { "inet6-l",		xioaddrs_tcp6_listen },
   { "inet6-listen",	xioaddrs_tcp6_listen },
#endif
#if WITH_INTERFACE
   { "interface",	xioaddrs_interface },
#endif
#if WITH_RAWIP
#if (WITH_IP4 || WITH_IP6)
   { "ip",		xioaddrs_rawip_sendto },
   { "ip-datagram",	xioaddrs_rawip_datagram },
   { "ip-dgram",	xioaddrs_rawip_datagram },
   { "ip-recv",		xioaddrs_rawip_recv },
   { "ip-recvfrom",	xioaddrs_rawip_recvfrom },
   { "ip-send",		xioaddrs_rawip_sendto },
   { "ip-sendto",	xioaddrs_rawip_sendto },
#endif
#if WITH_IP4
   { "ip4",		xioaddrs_rawip4_sendto },
   { "ip4-datagram",	xioaddrs_rawip4_datagram },
   { "ip4-dgram",	xioaddrs_rawip4_datagram },
   { "ip4-recv",	xioaddrs_rawip4_recv },
   { "ip4-recvfrom",	xioaddrs_rawip4_recvfrom },
   { "ip4-send",	xioaddrs_rawip4_sendto },
   { "ip4-sendto",	xioaddrs_rawip4_sendto },
#endif
#if WITH_IP6
   { "ip6",		xioaddrs_rawip6_sendto },
   { "ip6-datagram",	xioaddrs_rawip6_datagram },
   { "ip6-dgram",	xioaddrs_rawip6_datagram },
   { "ip6-recv",	xioaddrs_rawip6_recv },
   { "ip6-recvfrom",	xioaddrs_rawip6_recvfrom },
   { "ip6-send",	xioaddrs_rawip6_sendto },
   { "ip6-sendto",	xioaddrs_rawip6_sendto },
#endif
#endif /* WITH_RAWIP */
#if WITH_UNIX
   { "local",		xioaddrs_unix_connect },
#endif
#if WITH_NOP
   { "nop",		xioaddrs_nop },
#endif
#if WITH_FILE
   { "open",		xioaddrs_open },
#endif
#if WITH_OPENSSL
   { "openssl",		xioaddrs_openssl_connect },
   { "openssl-client",	xioaddrs_openssl_connect },
   { "openssl-connect",	xioaddrs_openssl_connect },
#if WITH_LISTEN
   { "openssl-listen",	xioaddrs_openssl_listen },
   { "openssl-server",	xioaddrs_openssl_listen },
#endif
#endif
#if WITH_PIPE
   { "pipe",		xioaddrs_pipe },
#endif
#if WITH_PROXY
   { "proxy",		xioaddrs_proxy_connect },
   { "proxy-connect",	xioaddrs_proxy_connect },
#endif
#if WITH_PTY
   { "pty",		xioaddrs_pty },
#endif
#if WITH_READLINE
   { "readline",	xioaddrs_readline },
#endif
#if (WITH_IP4 || WITH_IP6) && WITH_SCTP
   { "sctp",		xioaddrs_sctp_connect },
   { "sctp-connect",	xioaddrs_sctp_connect },
#if WITH_LISTEN
   { "sctp-l",		xioaddrs_sctp_listen },
   { "sctp-listen",	xioaddrs_sctp_listen },
#endif
#if WITH_IP4
   { "sctp4",		xioaddrs_sctp4_connect },
   { "sctp4-connect",	xioaddrs_sctp4_connect },
#if WITH_LISTEN
   { "sctp4-l",		xioaddrs_sctp4_listen },
   { "sctp4-listen",	xioaddrs_sctp4_listen },
#endif
#endif /* WITH_IP4 */
#if WITH_IP6
   { "sctp6",		xioaddrs_sctp6_connect },
   { "sctp6-connect",	xioaddrs_sctp6_connect },
#if WITH_LISTEN
   { "sctp6-l",		xioaddrs_sctp6_listen },
   { "sctp6-listen",	xioaddrs_sctp6_listen },
#endif
#endif /* WITH_IP6 */
#endif /* (WITH_IP4 || WITH_IP6) && WITH_SCTP */
#if WITH_GENERICSOCKET
   { "sendto",			xioaddrs_socket_sendto },
#endif
#if WITH_GENERICSOCKET
   { "socket-connect",		xioaddrs_socket_connect },
   { "socket-datagram",		xioaddrs_socket_datagram },
#if WITH_LISTEN
   { "socket-listen",		xioaddrs_socket_listen },
#endif /* WITH_LISTEN */
   { "socket-recv",		xioaddrs_socket_recv },
   { "socket-recvfrom",		xioaddrs_socket_recvfrom },
   { "socket-sendto",		xioaddrs_socket_sendto },
#endif
#if WITH_SOCKS4
   { "socks",		xioaddrs_socks4_connect },
   { "socks-client",	xioaddrs_socks4_connect },
   { "socks4",		xioaddrs_socks4_connect },
   { "socks4-client",	xioaddrs_socks4_connect },
#endif
#if WITH_SOCKS4A
   { "socks4a",		xioaddrs_socks4a_connect },
   { "socks4a-client",	xioaddrs_socks4a_connect },
#endif
#if WITH_SOCKS5
   { "socks5",		xioaddrs_socks5_client },
   { "socks5-client",	xioaddrs_socks5_client },
#endif
#if WITH_OPENSSL
   { "ssl",		xioaddrs_openssl_connect },
#if WITH_LISTEN
   { "ssl-l",		xioaddrs_openssl_listen },
   { "ssl-s",		xioaddrs_openssl_listen },
#endif
#endif
#if WITH_STDIO
   { "stderr",		xioaddrs_stderr },
   { "stdin",		xioaddrs_stdin },
   { "stdio",		xioaddrs_stdio },
   { "stdout",		xioaddrs_stdout },
#endif
#if WITH_SYSTEM
   { "system",		xioaddrs_system },
   { "system1",		xioaddrs_system1 },
   { "system2",		xioaddrs_system },
#endif
#if (WITH_IP4 || WITH_IP6) && WITH_TCP
   { "tcp",		xioaddrs_tcp_connect },
   { "tcp-connect",	xioaddrs_tcp_connect },
#endif
#if (WITH_IP4 || WITH_IP6) && WITH_TCP && WITH_LISTEN
   { "tcp-l",		xioaddrs_tcp_listen },
   { "tcp-listen",	xioaddrs_tcp_listen },
#endif
#if WITH_IP4 && WITH_TCP
   { "tcp4",		xioaddrs_tcp4_connect },
   { "tcp4-connect",	xioaddrs_tcp4_connect },
#endif
#if WITH_IP4 && WITH_TCP && WITH_LISTEN
   { "tcp4-l",		xioaddrs_tcp4_listen },
   { "tcp4-listen",	xioaddrs_tcp4_listen },
#endif
#if WITH_IP6 && WITH_TCP
   { "tcp6",		xioaddrs_tcp6_connect },
   { "tcp6-connect",	xioaddrs_tcp6_connect },
#endif
#if WITH_IP6 && WITH_TCP && WITH_LISTEN
   { "tcp6-l",		xioaddrs_tcp6_listen },
   { "tcp6-listen",	xioaddrs_tcp6_listen },
#endif
#if WITH_TEST
   { "test",		xioaddrs_test },
   { "testrev",		xioaddrs_testrev },
   { "testuni",		xioaddrs_testuni },
#endif
#if WITH_TUN
   { "tun",		xioaddrs_tun },
#endif
#if (WITH_IP4 || WITH_IP6) && WITH_UDP
   { "udp",		xioaddrs_udp_connect },
   { "udp-connect",	xioaddrs_udp_connect },
   { "udp-datagram",	xioaddrs_udp_datagram },
   { "udp-dgram",	xioaddrs_udp_datagram },
#endif
#if (WITH_IP4 || WITH_IP6) && WITH_UDP && WITH_LISTEN
   { "udp-l",	xioaddrs_udp_listen },
   { "udp-listen",	xioaddrs_udp_listen },
#endif
#if (WITH_IP4 || WITH_IP6) && WITH_UDP
   { "udp-recv",	xioaddrs_udp_recv },
   { "udp-recvfrom",	xioaddrs_udp_recvfrom },
   { "udp-send",	xioaddrs_udp_sendto },
   { "udp-sendto",	xioaddrs_udp_sendto },
#endif
#if WITH_IP4 && WITH_UDP
   { "udp4",		xioaddrs_udp4_connect },
   { "udp4-connect",	xioaddrs_udp4_connect },
   { "udp4-datagram",	xioaddrs_udp4_datagram },
   { "udp4-dgram",	xioaddrs_udp4_datagram },
#endif
#if WITH_IP4 && WITH_UDP && WITH_LISTEN
   { "udp4-l",		xioaddrs_udp4_listen },
   { "udp4-listen",	xioaddrs_udp4_listen },
#endif
#if WITH_IP4 && WITH_UDP
   { "udp4-recv",	xioaddrs_udp4_recv },
   { "udp4-recvfrom",	xioaddrs_udp4_recvfrom },
   { "udp4-send",	xioaddrs_udp4_sendto },
   { "udp4-sendto",	xioaddrs_udp4_sendto },
#endif
#if WITH_IP6 && WITH_UDP
   { "udp6",		xioaddrs_udp6_connect },
   { "udp6-connect",	xioaddrs_udp6_connect },
   { "udp6-datagram",	xioaddrs_udp6_datagram },
   { "udp6-dgram",	xioaddrs_udp6_datagram },
#endif
#if WITH_IP6 && WITH_UDP && WITH_LISTEN
   { "udp6-l",		xioaddrs_udp6_listen },
   { "udp6-listen",	xioaddrs_udp6_listen },
#endif
#if WITH_IP6 && WITH_UDP
   { "udp6-recv",	xioaddrs_udp6_recv },
   { "udp6-recvfrom",	xioaddrs_udp6_recvfrom },
   { "udp6-send",	xioaddrs_udp6_sendto },
   { "udp6-sendto",	xioaddrs_udp6_sendto },
#endif
#if WITH_UNIX
   { "unix",		xioaddrs_unix_client },
   { "unix-client",	xioaddrs_unix_client },
   { "unix-connect",	xioaddrs_unix_connect },
#endif
#if WITH_UNIX && WITH_LISTEN
   { "unix-l",		xioaddrs_unix_listen },
   { "unix-listen",	xioaddrs_unix_listen },
#endif
#if WITH_UNIX
   { "unix-recv",	xioaddrs_unix_recv },
   { "unix-recvfrom",	xioaddrs_unix_recvfrom },
   { "unix-send",	xioaddrs_unix_sendto },
   { "unix-sendto",	xioaddrs_unix_sendto },
#endif
#else /* !0 */
#  if WITH_INTEGRATE
#    include "xiointegrate.c"
#  else
#    include "xioaddrtab.c"
#  endif
#endif /* !0 */
   { NULL }	/* end marker */
} ;


/* prepares a xiofile_t record for dual address type:
   sets the tag and allocates memory for the substreams.
   returns 0 on success, or <0 if an error occurred.
*/
int xioopen_makedual(xiofile_t *file) {
   file->tag = XIO_TAG_DUAL;
   file->common.flags = XIO_RDWR;
   if ((file->dual.stream[0] = (xiosingle_t *)xioallocfd()) == NULL)
      return -1;
   file->dual.stream[0]->flags = XIO_RDONLY;
   if ((file->dual.stream[1] = (xiosingle_t *)xioallocfd()) == NULL)
      return -1;
   file->dual.stream[1]->flags = XIO_WRONLY;
   return 0;
}

/* returns NULL if an error occurred */
xiofile_t *xioallocfd(void) {
   xiofile_t *fd;

   if ((fd = Calloc(1, sizeof(xiofile_t))) == NULL) {
      return NULL;
   }
   /* some default values; 0's and NULL's need not be applied (calloc'ed) */
   fd->common.tag       = XIO_TAG_INVALID;
/* fd->common.addr      = NULL; */
   fd->common.flags     = XIO_RDWR;

#if WITH_RETRY
/* fd->stream.retry	= 0; */
/* fd->stream.forever	= false; */
   fd->stream.intervall.tv_sec  = 1;
/* fd->stream.intervall.tv_nsec = 0; */
#endif /* WITH_RETRY */
/* fd->common.ignoreeof = false; */
/* fd->common.eof       = 0; */

   fd->stream.rfd       = -1;
   fd->stream.wfd       = -1;
   fd->stream.dtype     = XIODATA_STREAM;
#if _WITH_SOCKET
/* fd->stream.salen     = 0; */
#endif /* _WITH_SOCKET */
/* fd->stream.howtoshut = XIOSHUT_UNSPEC;*/
/* fd->stream.howtoclose  = XIOCLOSE_UNSPEC;*/
/* fd->stream.name      = NULL; */
   fd->stream.escape	= -1;
/* fd->stream.para.exec.pid = 0; */
   fd->stream.lineterm  = LINETERM_RAW;

   /*!! support n socks */
   if (!sock[0]) {
      sock[0] = fd;
   } else {
      sock[1] = fd;
   }
   return fd;
}

void xiofreefd(xiofile_t *xfd) {
   if (xfd->stream.opts != NULL) {
      dropopts(xfd->stream.opts, PH_ALL);
   }
   free(xfd);
}


/* handle one chain of addresses
   rw is one of XIO_RDWR, XIO_RDONLY, or XIO_WRONLY
   when finished with this and the following sub addresses we return an xfd
   that can be used by the _socat() loop
*/
xiofile_t *socat_open(const char *addrs0, int rw, int flags) {
   const char *addrs;
   xiosingle_t *sfdA;	/* what we just parse(d) */
   xiosingle_t *sfdB;	/* what we just parse(d) - second part of dual */
   int newpipesep = 1;	/* dual address: 1%0 instead of 0!!1 */
   int reverseA, reverseB=0;
   bool currentisendpoint = false;
   int mayleftA, mayrightA,  mayleftB, mayrightB;
   int isleftA,  isrightA,   isleftB,  isrightB;
   int srchleftA, srchrightA, srchleftB=0, srchrightB=0;
   xiofile_t *xfd0;	/* what we return */
   xiofile_t *xfd1;	/* left hand of engine */
   xiofile_t *xfd2;	/* returned by sub address */
   int rw0, rw1, rw2;	/* the data directions for respective xfd
				 directions are sepcified as seen by transfer
				 engine */
   xiofd_t left, right;
   struct threadarg_struct *thread_arg;
   /*0 pthread_t thread = 0;*/
   /*pthread_attr_t attr;*/
   int _errno = 0;

   Info3("opening address \"%s\", rw=%d, flags=0x%x", addrs0, rw, flags);

   /* loop over retries, contains nearly the complete function */
   while (true) {

      addrs = addrs0;
      skipsp(&addrs);
      rw0 = rw;

      /* here we do not know much: will the next sub address be inter or
	 endpoint, single or dual, reverse? */

      /* check the logical direction of the current subaddress */
      reverseA = !(strncmp(addrs, xioparams->reversechar,
		       strlen(xioparams->reversechar)));
      if (reverseA) {
	 addrs += strlen(xioparams->reversechar);  /* consume "reverse" */
	 skipsp(&addrs);
      }

      if ((sfdA = xioparse_single(&addrs)) == NULL) {
	 Error1("syntax error in \"%s\"", addrs);
	 return NULL;
      }
      skipsp(&addrs);

      /* is it a dual sub address? */
      if (!strncmp(addrs, xioparams->pipesep, strlen(xioparams->pipesep))) {
	 /* yes, found dual-address operator */
	 if (rw != XIO_RDWR) {
	    Error("dual address cannot handle single direction data stream");
	 }
	 skipsp(&addrs);
	 addrs += strlen(xioparams->pipesep);  /* consume "%" */
	 /* check the logical direction of the current subaddress */
	 skipsp(&addrs);
	 reverseB = !(strncmp(addrs, xioparams->reversechar,
			      strlen(xioparams->reversechar)));
	 if (reverseB) {
	    addrs += strlen(xioparams->reversechar);  /* consume "reverse" */
	    skipsp(&addrs);
	 }

	 if ((sfdB = xioparse_single(&addrs)) == NULL) {
	    Error1("syntax error in \"%s\"", addrs);
	    return NULL;
	 }
	 skipsp(&addrs);
      } else {
	 sfdB = NULL;
      }

      /* is it the final sub address? */
      if (*addrs == '\0') {
	 currentisendpoint = true;
      } else if (!strncmp(addrs, xioparams->chainsep,
			  strlen(xioparams->chainsep))) {
	 addrs += strlen(xioparams->chainsep);
	 skipsp(&addrs);
	 currentisendpoint = false;
      } else {
	 Error2("syntax error on \"%s\": expected eol or \"%s\"",
		addrs, xioparams->chainsep);
	 xiofreefd((xiofile_t *)sfdA);
	 if (sfdB != NULL)  xiofreefd((xiofile_t *)sfdB);
	 return NULL;
      }

      /* now we know the context of the current sub address:
	 currentisendpoint...it is an endpoint, not an inter address
	 sfdB.......if not null, we have a dual type address
	 reverseA...sfdA is reverse
	 reverseB...if dual address then sfdB is reverse
	 rw0......the data direction of xfd0 */
      /* note: with dual inter, sfdB is implicitely reverse */

      /* calculate context parameters that are easier to handle */
      if (sfdB == NULL) {
	 srchleftA  = mayleftA  = (1 << rw0);
	 srchrightA = mayrightA = (currentisendpoint ? 0 : XIOBIT_ALL);
	 if (reverseA) {
	    srchrightA = srchleftA;
	    /*srchrightA = XIOBIT_REVERSE(srchleftA); no, see what right means*/
	    srchleftA  = XIOBIT_ALL;
	 }
      } else {	/* A is only part of dual */
	 srchleftA  = mayleftA  = XIOBIT_WRONLY;
	 srchrightA = mayrightA = (currentisendpoint ? 0 : XIOBIT_RDONLY);
	 if (reverseA) {
	    srchleftA = XIOBIT_RDONLY;
	    srchrightA  = XIOBIT_WRONLY;
	 }
	 srchleftB  = mayleftB  = (currentisendpoint ? XIOBIT_RDONLY : XIOBIT_WRONLY);
	 srchrightB = mayrightB = (currentisendpoint ? 0 : XIOBIT_RDONLY);
	 if (reverseB) {
	    srchleftB  = XIOBIT_RDONLY;
	    srchrightB = XIOBIT_WRONLY;
	 }
      }

      if ((true /*0 || ((rw0+1) & (XIO_WRONLY+1))*/) || currentisendpoint) {
	 if (xioopen_unoverload(sfdA, srchleftA, &isleftA, srchrightA, &isrightA)
	     < 0) {
	    Error1("address \"%s\" can not be used in this context",
		   sfdA->addrdescs[0]->inter_desc.defname);
	 }
      } else {
	 if (xioopen_unoverload(sfdA, srchrightA, &isrightA, srchleftA, &isleftA)
	     < 0) {
	    Error1("address \"%s\" can not be used in this context",
		   sfdA->addrdescs[0]->inter_desc.defname);
	 }
      }
      if (reverseA)  {
	 int tmp;
	 tmp = isleftA;  isrightA = isleftA;  isleftA = tmp;
      }

      if (sfdB != NULL) {
	 if (xioopen_unoverload(sfdB, srchleftB, &isleftB, srchrightB, &isrightB)
	     < 0) {
	    Error1("address \"%s\" can not be used in this context",
		   sfdB->addrdescs[0]->inter_desc.defname);
	 }
	 if (reverseB)  { isleftB = XIOBIT_REVERSE(srchrightB); }
	 if (!currentisendpoint && ((isrightA+1) & (isleftB+1))) {
	    /* conflict in directions on right side (xfd1) */
	    Error("conflict in data directions");/*!!*/
	 }
	 rw1 = ((isrightA+1) | (isleftB+1)) - 1;
      } else {
	 rw1 = isrightA;
      }
      rw2 = (rw1==XIO_RDWR) ? XIO_RDWR : (rw1==XIO_RDONLY) ? XIO_WRONLY :
	 XIO_RDONLY;

      /* now we know exactly what to do with the current sub address */

      /* we need the values for retry, forever, and intervall */
      applyopts_offset(sfdA, sfdA->opts);
      if (sfdB != NULL) {
	 applyopts_offset(sfdB, sfdB->opts);
      }

      /* if we found the endpoint address we are almost finished here */

      if (currentisendpoint) {
	 if (sfdB != NULL) {
	    if ((xfd0 = xioallocfd()) == NULL) {
	       xiofreefd((xiofile_t *)sfdA);  xiofreefd((xiofile_t *)sfdB);
	       return NULL;
	    }
	    xioopen_makedual(xfd0);
	    xfd0->dual.stream[0] = sfdB;
	    xfd0->dual.stream[1] = sfdA;
	 } else {
	    xfd0 = (xiofile_t *)sfdA;
	 }
	 /* open it and be ready in this thread */
	 if (xioopen_endpoint_dual(xfd0, rw0|flags) < 0) {
	    xiofreefd(xfd0);
	    return NULL;
	 }
	 return xfd0;
      }

      /* the current addr is not the final sub address */

      /* recursively open the following addresses of chain */
      /* loop over retries if appropriate */
      do {
	 xfd2 = socat_open(addrs, rw2, flags);
	 if (xfd2 != NULL) {
	    break;	/* succeeded */
	 }
	 if (sfdA->retry == 0 && !sfdA->forever) {
	    xiofreefd((xiofile_t *)sfdA);
	    if (sfdB != NULL)  xiofreefd((xiofile_t *)sfdB);
	    /*! close()? */
	    return NULL;
	 }
	 Nanosleep(&sfdA->intervall, NULL);
	 if (sfdA->retry)  --sfdA->retry;
      } while (true);

      /* only xfd2 is valid here, contains a handle for the rest of the chain
	 */
      /* yupp, and the single addresses sfdA and ev.sfdB are valid too, but
	 not yet opened */

      /* what are xfd0, xfd1, and xfd2?
	 consider chain: addr1|addr2
	 with no reverse address, this will run like:
	 _socat(<upstream>,addr1) --- _socat(-,   addr2)
	 _socat(???,       xfd0)  --- _socat(xfd1,xfd2)
	 xfd0 will be opened in this routine
	 xfd1 will be assembled now, just using FDs
	 xfd2 comes from recursive open call
      */
      /* but, with reverse, it looks so:
	 consider chain: ^addr1|addr2
	 _socat(<upstream>,-)      --- _socat(addr1,addr2)
	 _socat(???       ,xfd0)   --- _socat(xfd1,  xfd2)
	 xfd0 will be assembled now, just using FDs
	 xfd1 was just initialized in this routine
	 xfd2 comes from recursive open call
      */
      /* even worse, with mixed forward/reverse dual address:
	 consider chain: addr1a%^addr1b|addr2
	 _socat(<upstream, addr1a%nop) --- _socat(nop%addr1b, addr2)
	 _socat(???,       xfd0)        --- _socat(xfd1,        xfd2)
      */
				
      /* prepare FD based communication of current addr with its right neighbor
	 (xfd0-xfd1) */
      {
	 switch (xioopts.pipetype) {
	 case XIOCOMM_SOCKETPAIR:
	 case XIOCOMM_SOCKETPAIRS:
	    if (xiocommpair(xioopts.pipetype,
			    (rw2+1)&(XIO_WRONLY+1), (rw1+1)&(XIO_WRONLY+1),
			    sfdB!=0, &left, &right,
			    PF_UNIX, SOCK_STREAM, 0) != 0) {
	       return NULL;
	    }
	    break;
	 case XIOCOMM_PTY:
	 case XIOCOMM_PTYS:
	    if (xiocommpair(xioopts.pipetype,
			    (rw2+1)&(XIO_WRONLY+1), (rw1+1)&(XIO_WRONLY+1),
			    sfdB!=0, &left, &right,
			    1 /* useptmx */) != 0) {
	       return NULL;
	    }
	    break;
	 default:
	    if (xiocommpair(xioopts.pipetype,
			    (rw2+1)&(XIO_WRONLY+1), (rw1+1)&(XIO_WRONLY+1),
			    sfdB!=0, &left, &right) != 0) {
	       return NULL;
	    }
	    break;
	 }
      }

      /* now assemble xfd0 and xfd1 */

      if (sfdB != NULL && reverseA == reverseB) {
	 /* dual address, differing orientation (B is impl.reverse) */
	 /* dual implies (rw0==rw1==XIO_RDWR) */
	 if (!reverseA) {
	    /* A is not reverse, but B */
#	    define MAXADDR 45
	    char addr[MAXADDR];

	    xfd0 = xioallocfd();
	    xioopen_makedual(xfd0);
	    xfd0->dual.stream[1] = sfdA;
	    snprintf(addr, MAXADDR, "FD:%u", /*0 righttoleft[0]*/left.rfd);
	    if ((xfd0->dual.stream[0] =
		 (xiosingle_t *)socat_open(addr, XIO_WRONLY, 0))
		== NULL) {
	       xiofreefd(xfd0);  xiofreefd(xfd2);  return NULL;
	    }
	    /* address type FD keeps the FDs open per default, but ... */

	    xfd1 = xioallocfd();
	    xioopen_makedual(xfd1);
	    xfd1->dual.stream[1] = sfdB;
	    snprintf(addr, MAXADDR, "FD:%u", /*0 lefttoright[0]*/right.rfd);
	    if ((xfd1->dual.stream[0] =
		 (xiosingle_t *)socat_open(addr, XIO_RDONLY, 0))
		== NULL) {
	       xiofreefd(xfd0);  xiofreefd(xfd1);  xiofreefd(xfd2);
	       return NULL;
	    }
	 } else {
	    /* A is reverse, but B is not */
	    char addr[MAXADDR];

	    xfd0 = xioallocfd();
	    xioopen_makedual(xfd0);
	    xfd0->dual.stream[0] = sfdB;
	    snprintf(addr, MAXADDR, "FD:%u", /*0 lefttoright[1]*/left.wfd);
	    if ((xfd0->dual.stream[1] =
		 (xiosingle_t *)socat_open(addr, XIO_RDONLY, 0))
		== NULL) {
	       xiofreefd(xfd0);  xiofreefd(xfd2);	 return NULL;
	    }

	    xfd1 = xioallocfd();
	    xioopen_makedual(xfd1);
	    xfd1->dual.stream[0] = sfdA;
	    snprintf(addr, MAXADDR, "FD:%u", /*0 righttoleft[1]*/right.wfd);
	    if ((xfd1->dual.stream[1] =
		 (xiosingle_t *)socat_open(addr, XIO_RDONLY, 0))
		== NULL) {
	       xiofreefd(xfd0);  xiofreefd(xfd1);  xiofreefd(xfd2);
	       return NULL;
	    }
	 }
	 xfd0->dual.stream[0]->rfd = left.rfd;
	 xfd0->dual.stream[1]->wfd = left.wfd;
	 xfd1->dual.stream[0]->rfd = right.rfd;
	 xfd1->dual.stream[1]->wfd = right.wfd;

      } else {
	 /* either dual with equal directions, or non-dual */
	 xiofile_t *tfd;	/* temp xfd */
	 char addr[MAXADDR];
	 if (sfdB != NULL) {
	    /* dual, none or both are reverse */
	    tfd = xioallocfd();
	    xioopen_makedual(tfd);
	    tfd->dual.stream[1] = sfdA;
	    tfd->dual.stream[0] = sfdB;
	 } else {
	    /* non-dual */
	    tfd = (xiofile_t *)sfdA;
	 }

	 /* now take care of orientation */
	 if (!reverseA) {
	    /* forward */
	    xfd0 = tfd;
	    if (rw1 == XIO_RDWR) {
	       snprintf(addr, MAXADDR, "FD:%u:%u", /*0 righttoleft[1]*/right.wfd, /*0 lefttoright[0]*/right.rfd);
	    } else if (rw1 == XIO_RDONLY) {
	       snprintf(addr, MAXADDR, "FD:%u", /*0 lefttoright[0]*/right.rfd);
	    } else {
	       snprintf(addr, MAXADDR, "FD:%u", /*0 righttoleft[1]*/right.wfd);
	    }
	    if ((xfd1 = socat_open(addr, rw1, 0)) == NULL) {
	       xiofreefd(xfd0);  xiofreefd(xfd2);
	       return NULL;
	    }
	 } else {
	    /* reverse */
	    xfd1 = tfd;
	    if (rw0 == XIO_RDWR) {
	       snprintf(addr, MAXADDR, "FD:%u:%u", /*0 lefttoright[1]*/left.wfd, /*0 righttoleft[0]*/left.rfd);
	    } else if (rw0 == XIO_RDONLY) {
	       snprintf(addr, MAXADDR, "FD:%u", /*0 righttoleft[0]*/left.rfd);
	    } else {
	       snprintf(addr, MAXADDR, "FD:%u", /*0 lefttoright[1]*/left.wfd);
	    }
	    if ((xfd0 = socat_open(addr, rw0/*0 XIO_RDWR*/, 0)) == NULL) {
	       xiofreefd(xfd1);  xiofreefd(xfd2);
	       return NULL;
	    }
	    /* address type FD keeps the FDs open per default, but ... */
	 }
	 if (xfd0->tag == XIO_TAG_DUAL) {
	    xfd0->dual.stream[0]->rfd = /*0 righttoleft[0]*/left.rfd;
	    xfd0->dual.stream[1]->wfd = /*0 lefttoright[1]*/left.wfd;
	 } else {
	    xfd0->stream.rfd = /*0 righttoleft[0]*/left.rfd;
	    xfd0->stream.wfd = /*0 lefttoright[1]*/left.wfd;
	 }
	 if (xfd1->tag == XIO_TAG_DUAL) {
	    xfd1->dual.stream[0]->rfd = /*0 righttoleft[0]*/left.rfd;
	    xfd1->dual.stream[1]->rfd = /*0 lefttoright[1]*/left.wfd;
	 } else {
	    xfd1->stream.rfd = /*0 lefttoright[0]*/right.rfd;
	    xfd1->stream.wfd = /*0 righttoleft[1]*/right.wfd;
	 }
      }

      /* address type FD keeps the FDs open per default, but ... */
      if (xfd0->tag == XIO_TAG_DUAL) {
	 xfd0->dual.stream[0]->howtoshut = left.howtoshut;
	 xfd0->dual.stream[0]->howtoclose = left.howtoclose;
	 xfd0->dual.stream[0]->dtype      = left.dtype;
	 xfd0->dual.stream[1]->howtoshut = right.howtoshut;
	 xfd0->dual.stream[1]->howtoclose = right.howtoclose;
	 xfd0->dual.stream[1]->dtype      = right.dtype;
      } else {
	 xfd0->stream.howtoshut = left.howtoshut;
	 xfd0->stream.howtoclose = left.howtoclose;
	 xfd0->stream.dtype      = left.dtype;
      }
      if (xfd1->tag == XIO_TAG_DUAL) {
	 xfd1->dual.stream[0]->howtoshut = left.howtoshut;
	 xfd1->dual.stream[0]->howtoclose = left.howtoclose;
	 xfd1->dual.stream[0]->dtype      = left.dtype;
	 xfd1->dual.stream[1]->howtoshut = right.howtoshut;
	 xfd1->dual.stream[1]->howtoclose = right.howtoclose;
	 xfd1->dual.stream[1]->dtype      = right.dtype;
      } else {
	 xfd1->stream.howtoshut = right.howtoshut;
	 xfd1->stream.howtoclose = right.howtoclose;
	 xfd1->stream.dtype      = right.dtype;
      }

      /* here xfd2 is valid and ready for transfer;
	 and xfd0 and xfd1 are valid and ready for opening */

      /* create a new thread that do the xioopen() of xfd1 and xfd2, and then
	 drive the transfer engine between them */
      if ((thread_arg = Malloc(sizeof(struct threadarg_struct))) == NULL) {
	 /*! free something */
	 xiofreefd(xfd0); xiofreefd(xfd1); xiofreefd(xfd2);
	 return NULL;
      }
      thread_arg->rw = (reverseA ? rw1 : rw0);
      thread_arg->xfd1 = xfd1;
      thread_arg->xfd2 = xfd2;
      Notice5("starting thread: dir=%d, reverseA=%d, reverseB=%d, xfd1->tag=%d, xfd2->tag=%d",
	      rw0, reverseA, reverseB, xfd1->tag, xfd2->tag);
      if (xfd1->tag==XIO_TAG_DUAL) {
	 Notice4("xfd1: [%s, wfd=%d] %% [%s, rfd=%d]",
		 xfd1->dual.stream[1]->addrdesc->common_desc.defname,
		 xfd1->dual.stream[1]->wfd,
		 xfd1->dual.stream[0]->addrdesc->common_desc.defname,
		 xfd1->dual.stream[0]->rfd);
      } else {
	 Notice3("xfd1: %s, wfd=%d, rfd=%d",
		 xfd1->stream.addrdesc->common_desc.defname,
		 xfd1->stream.wfd, xfd1->stream.rfd);
      }
      if (xfd2->tag==XIO_TAG_DUAL) {
	 Notice4("xfd2: [%s, wfd=%d] %% [%s, rfd=%d]",
		 xfd2->dual.stream[1]->addrdesc->common_desc.defname,
		 xfd2->dual.stream[1]->wfd,
		 xfd2->dual.stream[0]->addrdesc->common_desc.defname,
		 xfd2->dual.stream[0]->rfd);
      } else {
	 Notice3("xfd2: %s, wfd=%d, rfd=%d",
		 xfd2->stream.addrdesc->common_desc.defname,
		 xfd2->stream.wfd, xfd2->stream.rfd);
      }
      Info5("pthread_create(%p, NULL, %s, {%d,%p,%p})",
	    &xfd0->stream.subthread,
	    (reverseA||(sfdB!=NULL)&&!reverseB)?"xioopenleftthenengine":"xioengine",
	    thread_arg->rw, thread_arg->xfd1, thread_arg->xfd2);
      if ((_errno =
	   Pthread_create(&xfd0->stream.subthread, NULL,
			  (reverseA||(sfdB!=NULL)&&!reverseB)?xioopenleftthenengine:xioengine,
			   thread_arg))
	  != 0) {
	 Error3("pthread_create(%p, {}, xioengine, %p): %s",
		&xfd0->stream.subthread, thread_arg,
		strerror(_errno));
	 xiofreefd(xfd0); xiofreefd(xfd1); xiofreefd(xfd2);
	 free(thread_arg); return NULL;
      }
      Info1("started thread "F_thread, xfd0->stream.subthread);
      xfd1 = NULL;
      xfd2 = NULL;

      /* open protocol part */
      if (xioopen_inter_dual(xfd0, rw|flags)
	  < 0) {
	 /*! close sub chain */
	 if (xfd0->stream.retry == 0 && !xfd0->stream.forever) {
	    xiofreefd(xfd0); /*! close()? */ return NULL;
	 }
	 Nanosleep(&xfd0->stream.intervall, NULL);
	 if (xfd0->stream.retry)  --xfd0->stream.retry;
	 continue;
      }
      break;
   }
   /*!!!?*/
#if 0
   xfd0->stream.howtoshut = XIOSHUT_CLOSE;
#endif
   return xfd0;
}

void *xioopenleftthenengine(void *thread_void) {
   struct threadarg_struct *thread_arg = thread_void;
   int rw = thread_arg->rw;
   xiofile_t *xfd1 = thread_arg->xfd1;
   xiofile_t *xfd2 = thread_arg->xfd2;

   /*! design a function with better interface */
   if (xioopen_inter_dual(xfd1, rw|XIO_MAYCONVERT|XIO_MAYCHILD) < 0) {
      xioclose(xfd2);
      xiofreefd(xfd1);
      xiofreefd(xfd2);
      return NULL;
   }
   xioengine(thread_void);
   return NULL;
}


xiofile_t *xioparse_dual(const char **addr) {
   xiofile_t *xfd;
   xiosingle_t *sfd1;
   int reverse;

   /* check the logical direction of the current subaddress */
   reverse = !(strncmp(*addr, xioparams->reversechar,
		       strlen(xioparams->reversechar)));
   if (reverse) {
      /* skip "reverse" token */
      *addr += strlen(xioparams->reversechar);
   }

   /* we parse a single address */
   if ((sfd1 = xioparse_single(addr)) == NULL) {
      return NULL;
   }
   sfd1->reverse = reverse;

   /* and now we see if we reached a dual-address separator */
   if (strncmp(*addr, xioopts.pipesep, strlen(xioopts.pipesep))) {
      /* no, finish */
      return (xiofile_t *)sfd1;
   }

   /* we found the dual-address operator, so we parse the second single address
      */
   *addr += strlen(xioparams->pipesep);

   if ((xfd = xioallocfd()) == NULL) {
      xiofreefd((xiofile_t *)sfd1);
      return NULL;
   }
   xfd->tag = XIO_TAG_DUAL;
   xfd->dual.stream[0] = sfd1;
   if ((xfd->dual.stream[1] = xioparse_single(addr)) == NULL) {
      xiofreefd(xfd); /*! and maybe have free some if its contents */
      return NULL;
   }
   
   return xfd;
}

int xioopen_inter_dual(xiofile_t *xfd, int xioflags) {

   if (xfd->tag == XIO_TAG_DUAL) {
      /* a really dual address */
      if ((xioflags&XIO_ACCMODE) != XIO_RDWR) {
	 Warn("unidirectional open of dual address");
      }

      /* a "usual" bidirectional stream specification, one address */
      if (((xioflags&XIO_ACCMODE)+1) & (XIO_RDONLY+1)) {
	 if (xioopen_inter_single((xiofile_t *)xfd->dual.stream[0], XIO_RDONLY|(xioflags&~XIO_ACCMODE&~XIO_MAYEXEC))
	     < 0) {
	    return -1;
	 }
      }
      /*! should come before xioopensingle? */
      if (((xioflags&XIO_ACCMODE)+1) & (XIO_WRONLY+1)) {
	 if (xioopen_inter_single((xiofile_t *)xfd->dual.stream[1], XIO_WRONLY|(xioflags&~XIO_ACCMODE&~XIO_MAYEXEC))
	     < 0) {
	    xioclose((xiofile_t *)xfd->dual.stream[0]);
	    return -1;
	 }
      }
      return 0;
   }

   return xioopen_inter_single(xfd, xioflags);
}

int xioopen_endpoint_dual(xiofile_t *xfd, int xioflags) {

   if (xfd->tag == XIO_TAG_DUAL) {
      /* a really dual address */
      if ((xioflags&XIO_ACCMODE) != XIO_RDWR) {
	 Warn("unidirectional open of dual address");
      }

      /* a "usual" bidirectional stream specification, one address */
      if (((xioflags&XIO_ACCMODE)+1) & (XIO_WRONLY+1)) {
	 if (xioopen_endpoint_single((xiofile_t *)xfd->dual.stream[1], XIO_WRONLY|(xioflags&~XIO_ACCMODE&~XIO_MAYEXEC))
	     < 0) {
	    return -1;
	 }
      }
      /*! should come before xioopensingle? */
      if (((xioflags&XIO_ACCMODE)+1) & (XIO_RDONLY+1)) {
	 if (xioopen_endpoint_single((xiofile_t *)xfd->dual.stream[0], XIO_RDONLY|(xioflags&~XIO_ACCMODE&~XIO_MAYEXEC))
	     < 0) {
	    xioclose((xiofile_t *)xfd->dual.stream[1]);
	    return -1;
	 }
      }
      return 0;
   }

   return xioopen_endpoint_single(xfd, xioflags);
}


/* parses the parameters and options of a single (sub)address.
   returns 0 on success or -1 if an error occurred. */
static xiosingle_t *
   xioparse_single(const char **addr	/* input string; afterwards points to
					   first char not belonging to this
					   sub address */
		   ) {
   char addr0[20];
   xiofile_t *xfd;
   xiosingle_t *sfd;
   struct xioaddrname *ae;
   /*int maxparams;*/	/* max number of parameters */
   const char *ends[4+1];
   const char *hquotes[] = {
      "'",
      NULL
   } ;
   const char *squotes[] = {
      "\"",
      NULL
   } ;
   const char *nests[] = {
      "'", "'",
      "(", ")",
      "[", "]",
      "{", "}",
      NULL
   } ;
   char token[512], *tokp;
   size_t len;
   int i;

   /* init */
   i = 0;
   ends[i++] = xioopts.chainsep;	/* default: "|" */
   ends[i++] = xioopts.pipesep;		/* default: "%" */
   ends[i++] = ","/*xioopts.comma*/;		/* default: "," */
   ends[i++] = ":"/*xioopts.colon*/;		/* default: ":" */
   ends[i++] = NULL;

   if ((xfd = xioallocfd()) == NULL) {
      return NULL;
   }
   sfd = &xfd->stream;
   sfd->argc = 0;

   /* for error messages */
   addr0[0] = '\0'; strncat(addr0, *addr, sizeof(addr0)-1);

   len = sizeof(token); tokp = token;
   if (nestlex(addr, &tokp, &len, ends, hquotes, squotes, nests,
	       true, true, true, false) < 0) {
      Error2("keyword too long, in address \"%s%s\"", token, *addr);
   }
   *tokp = '\0';  /*! len? */
   ae = (struct xioaddrname *)
      keyw((struct wordent *)&address_names, token,
	   sizeof(address_names)/sizeof(struct xioaddrname)-1);
   if (ae != NULL) {
      /* found keyword */
      sfd->addrdescs = ae->desc;
      if ((sfd->argv[sfd->argc++] = strdup(token)) == NULL) {
	 Error1("strdup(\"%s\"): out of memory", token);
      }
   } else {
      if (false) {	/* for canonical reasons */
	 ;
#if WITH_FDNUM
      } else if (isdigit(token[0]&0xff) && token[1] == '\0') {
	 Info1("interpreting address \"%s\" as file descriptor", token);
	 sfd->addrdescs = xioaddrs_fdnum;
	 if ((sfd->argv[sfd->argc++] = strdup("FD")) == NULL) {
	    Error("strdup(\"FD\"): out of memory");
	 }
	 if ((sfd->argv[sfd->argc++] = strdup(token)) == NULL) {
	    Error1("strdup(\"%s\"): out of memory", token);
	 }
	 /*! check argc overflow */
#endif /* WITH_FDNUM */
#if WITH_GOPEN
      } else if (strchr(token, '/')) {
	 Info1("interpreting address \"%s\" as file name", token);
	 sfd->addrdescs = xioaddrs_gopen;
	 if ((sfd->argv[sfd->argc++] = strdup("GOPEN")) == NULL) {
	    Error("strdup(\"GOPEN\"): out of memory");
	 }
	 if ((sfd->argv[sfd->argc++] = strdup(token)) == NULL) {
	    Error1("strdup(\"%s\"): out of memory", token);
	 }
	 /*! check argc overflow */
#endif /* WITH_GOPEN */
      } else {
	 Error1("unknown device/address \"%s\"", token);
	 xiofreefd(xfd);
	 return NULL;
      }
   }

   while (!strncmp(*addr, xioopts.paramsep, strlen(xioopts.paramsep))) {
      if (sfd->argc >= MAXARGV) {
	 Error1("address \"%s\": succeeds max number of parameters",
		sfd->argv[0]);
      }
      *addr += strlen(xioopts.paramsep);
      len = sizeof(token);  tokp = token;
      if (nestlex(addr, &tokp, &len, ends, hquotes, squotes, nests,
		  true, true, true, false) != 0) {
	 Error2("syntax error in address \"%s%s\"", token, *addr);
      }
      *tokp = '\0';
      if ((sfd->argv[sfd->argc++] = strdup(token)) == NULL) {
	 Error1("strdup(\"%s\"): out of memory", token);
      }
   }

   if (parseopts(addr, &sfd->opts) < 0) {
      xiofreefd(xfd);
      return NULL;
   }

   return sfd;
}

/* during parsing the sub address, we did not know if it is used as inter
   address or endpoint; therefore we could not select the appropriate address
   descriptor. Here we already know the context and thus select the address
   descriptor and check the option groups.
   returns 0 on success or -1 if an error occurred */
static int
   xioopen_unoverload(xiosingle_t *sfd,
		      int mayleft,	/* what may be on left side: or'd
					   XIOBIT_RDWR, XIOBIT_RDONLY,
					   XIOBIT_WRONLY */
		      int *isleft,	/* what the selected address desc
					   provides on left side: XIO_RDWR,
					   XIO_RDONLY, or XIO_WRONLY */
		      int mayright,	/* what may be on right side: or'd
					   XIOBIT_RDWR, XIOBIT_RDONLY,
					   XIOBIT_WRONLY */
		      int *isright)	/* what the selected address desc
					   provides on right side: XIO_RDWR,
					   XIO_RDONLY, or XIO_WRONLY */
{
   const union xioaddr_desc **addrdescs;
   int tag;
   int i;

   addrdescs = sfd->addrdescs;
   tag = (mayright ? XIOADDR_INTER : XIOADDR_ENDPOINT);

   /* look for a matching entry in the list of address descriptions */
   Debug5("searching record for \"%s\" with tag=%d, numparams=%d, leftdir %d, rightdir %d",
	  addrdescs[0]->common_desc.defname,
	  tag, sfd->argc-1, mayleft, mayright);
   while ((*addrdescs) != NULL) {
      if ((*addrdescs)->tag == tag &&
	  addrdescs[0]->common_desc.numparams == sfd->argc-1 &&
	  (addrdescs[0]->common_desc.leftdirs & mayleft) != 0 &&
	  (mayright ? (addrdescs[0]->inter_desc.rightdirs & mayright) : 1)) {
	 break;
      }
      ++addrdescs;
   }

   if (addrdescs[0] == NULL) {
      if (tag == XIOADDR_ENDPOINT) {
	 Error3("address \"%s...\" in endpoint context, leftdirs=%d, with %d parameter(s) is not available",
		sfd->argv[0], mayleft, sfd->argc-1);
      } else {
	 Error4("address \"%s...\" in intermediate context, leftdirs=%d, rightdirs=%d, with %d parameter(s) is not available",
		sfd->argv[0], mayleft, mayright, sfd->argc-1);
      }
      xiofreefd((xiofile_t *)sfd);  return -1;
   }

   i = (addrdescs[0]->common_desc.leftdirs & mayleft);
   *isleft = 0;
   while (i>>=1) {
      ++*isleft;
   }
   if (true /*0 mayright*/) {
      i = (addrdescs[0]->inter_desc.rightdirs & mayright);
      *isright = 0;
      while (i>>=1) {
	 ++*isright;
      }
   }
   sfd->tag  = (*isleft + 1);
   sfd->addrdesc = addrdescs[0];
   if (addrdescs[0]->common_desc.howtoshut != XIOSHUT_UNSPEC)
      sfd->howtoshut = addrdescs[0]->common_desc.howtoshut;
   if (addrdescs[0]->common_desc.howtoclose != XIOCLOSE_UNSPEC)
      sfd->howtoclose  = addrdescs[0]->common_desc.howtoclose;

   if (tag == XIOADDR_ENDPOINT) {
      Debug1("selected record with leftdirs %d",
	     addrdescs[0]->common_desc.leftdirs);
   } else {
      Debug2("selected record with leftdirs %d, rightdirs %d",
	     addrdescs[0]->common_desc.leftdirs,
	     addrdescs[0]->inter_desc.rightdirs);
   }
   return 0;
}


static int
   xioopen_inter_single(xiofile_t *xfd, int xioflags) {
   const struct xioaddr_inter_desc *addrdesc;
   int result;

   addrdesc = &xfd->stream.addrdesc->inter_desc;

   if ((xioflags&XIO_ACCMODE) == XIO_RDONLY) {
      xfd->tag = XIO_TAG_RDONLY;
   } else if ((xioflags&XIO_ACCMODE) == XIO_WRONLY) {
      xfd->tag = XIO_TAG_WRONLY;
   } else if ((xioflags&XIO_ACCMODE) == XIO_RDWR) {
      xfd->tag = XIO_TAG_RDWR;
   } else {
      Error1("invalid mode for address \"%s\"", xfd->stream.argv[0]);
   }
   xfd->stream.flags     &= (~XIO_ACCMODE);
   xfd->stream.flags     |= (xioflags & XIO_ACCMODE);
   result = (*addrdesc->func)(xfd->stream.argc, xfd->stream.argv,
			      xfd->stream.opts, xioflags, xfd, 
			      addrdesc->groups, addrdesc->arg1,
			      addrdesc->arg2, addrdesc->arg3);
   return result;
}

static int
   xioopen_endpoint_single(xiofile_t *xfd, int xioflags) {
   const struct xioaddr_endpoint_desc *addrdesc;
   int result;

   addrdesc = &xfd->stream.addrdesc->endpoint_desc;
   if ((xioflags&XIO_ACCMODE) == XIO_RDONLY) {
      xfd->tag = XIO_TAG_RDONLY;
   } else if ((xioflags&XIO_ACCMODE) == XIO_WRONLY) {
      xfd->tag = XIO_TAG_WRONLY;
   } else if ((xioflags&XIO_ACCMODE) == XIO_RDWR) {
      xfd->tag = XIO_TAG_RDWR;
   } else {
      Error1("invalid mode for address \"%s\"", xfd->stream.argv[0]);
   }
   xfd->stream.flags     &= (~XIO_ACCMODE);
   xfd->stream.flags     |= (xioflags & XIO_ACCMODE);
   result = (*addrdesc->func)(xfd->stream.argc, xfd->stream.argv,
			      xfd->stream.opts, xioflags, xfd, 
			      addrdesc->groups, addrdesc->arg1,
			      addrdesc->arg2, addrdesc->arg3);
   return result;
}
