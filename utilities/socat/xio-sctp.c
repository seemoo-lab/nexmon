/* source: xio-sctp.c */
/* Copyright Gerhard Rieger 2008 */
/* Published under the GNU General Public License V.2, see file COPYING */

/* this file contains the source for SCTP related functions and options */

#include "xiosysincludes.h"

#if WITH_SCTP

#include "xioopen.h"
#include "xio-listen.h"
#include "xio-ip4.h"
#include "xio-ipapp.h"
#include "xio-sctp.h"

/****** SCTP addresses ******/

#if WITH_IP4 || WITH_IP6
const struct xioaddr_endpoint_desc xioaddr_sctp_connect2  = { XIOADDR_SYS, "sctp-connect",  2, XIOBIT_ALL, GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP4|GROUP_SOCK_IP6|GROUP_IP_SCTP|GROUP_CHILD|GROUP_RETRY, XIOSHUT_DOWN, XIOCLOSE_CLOSE, xioopen_ipapp_connect, SOCK_STREAM, IPPROTO_SCTP, PF_UNSPEC HELP(":<host>:<port>") };
const union xioaddr_desc *xioaddrs_sctp_connect[] = {
   (union xioaddr_desc *)&xioaddr_sctp_connect2,
   NULL
};

#if WITH_LISTEN
const struct xioaddr_endpoint_desc xioaddr_sctp_listen1   = { XIOADDR_SYS, "sctp-listen",   1, XIOBIT_ALL, GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP4|GROUP_SOCK_IP6|GROUP_IP_SCTP|GROUP_LISTEN|GROUP_CHILD|GROUP_RANGE|GROUP_RETRY, XIOSHUT_DOWN, XIOCLOSE_CLOSE, xioopen_ipapp_listen, SOCK_STREAM, IPPROTO_SCTP, PF_UNSPEC HELP(":<port>") };
const union xioaddr_desc *xioaddrs_sctp_listen[] = {
   (union xioaddr_desc *)&xioaddr_sctp_listen1,
   NULL
};
#endif /* WITH_LISTEN */
#endif /* WITH_IP4 || WITH_IP6 */

#if WITH_IP4
const struct xioaddr_endpoint_desc xioaddr_sctp4_connect2 = { XIOADDR_SYS, "sctp4-connect", 2, XIOBIT_ALL, GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP4|GROUP_IP_SCTP|GROUP_CHILD|GROUP_RETRY, XIOSHUT_DOWN, XIOCLOSE_CLOSE, xioopen_ipapp_connect, SOCK_STREAM, IPPROTO_SCTP, PF_INET HELP(":<host>:<port>") };
const union xioaddr_desc *xioaddrs_sctp4_connect[] = {
   (union xioaddr_desc *)&xioaddr_sctp4_connect2,
   NULL
};

#if WITH_LISTEN
const struct xioaddr_endpoint_desc xioaddr_sctp4_listen1  = { XIOADDR_SYS, "sctp4-listen",  1, XIOBIT_ALL, GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP4|GROUP_IP_SCTP|GROUP_LISTEN|GROUP_CHILD|GROUP_RANGE|GROUP_RETRY, XIOSHUT_DOWN, XIOCLOSE_CLOSE, xioopen_ipapp_listen, SOCK_STREAM, IPPROTO_SCTP, PF_INET HELP(":<port>") };
const union xioaddr_desc *xioaddrs_sctp4_listen[] = {
   (union xioaddr_desc *)&xioaddr_sctp4_listen1,
   NULL
};
#endif /* WITH_LISTEN */
#endif /* WITH_IP4 */

#if WITH_IP6
const struct xioaddr_endpoint_desc xioaddr_sctp6_connect2 = { XIOADDR_SYS, "sctp6-connect", 2, XIOBIT_ALL, GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP6|GROUP_IP_SCTP|GROUP_CHILD|GROUP_RETRY, XIOSHUT_DOWN, XIOCLOSE_CLOSE, xioopen_ipapp_connect, SOCK_STREAM, IPPROTO_SCTP, PF_INET6 HELP(":<host>:<port>") };
const union xioaddr_desc *xioaddrs_sctp6_connect[] = {
   (union xioaddr_desc *)&xioaddr_sctp6_connect2,
   NULL
};

#if WITH_LISTEN
const struct xioaddr_endpoint_desc xioaddr_sctp6_listen1  = { XIOADDR_SYS, "sctp6-listen",  1, XIOBIT_ALL, GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP6|GROUP_IP_SCTP|GROUP_LISTEN|GROUP_CHILD|GROUP_RANGE|GROUP_RETRY, XIOSHUT_DOWN, XIOCLOSE_CLOSE, xioopen_ipapp_listen, SOCK_STREAM, IPPROTO_SCTP, PF_INET6 HELP(":<port>") };
const union xioaddr_desc *xioaddrs_sctp6_listen[] = {
   (union xioaddr_desc *)&xioaddr_sctp6_listen1,
   NULL
};
#endif /* WITH_LISTEN */
#endif /* WITH_IP6 */

/****** SCTP address options ******/

#ifdef SCTP_NODELAY
const struct optdesc opt_sctp_nodelay = { "sctp-nodelay",   "nodelay", OPT_SCTP_NODELAY, GROUP_IP_SCTP, PH_PASTSOCKET, TYPE_INT,	OFUNC_SOCKOPT, SOL_SCTP, SCTP_NODELAY };
#endif
#ifdef SCTP_MAXSEG
const struct optdesc opt_sctp_maxseg  = { "sctp-maxseg",    "mss",  OPT_SCTP_MAXSEG,  GROUP_IP_SCTP, PH_PASTSOCKET,TYPE_INT, OFUNC_SOCKOPT, SOL_SCTP, SCTP_MAXSEG };
const struct optdesc opt_sctp_maxseg_late={"sctp-maxseg-late","mss-late",OPT_SCTP_MAXSEG_LATE,GROUP_IP_SCTP,PH_CONNECTED,TYPE_INT,OFUNC_SOCKOPT, SOL_SCTP, SCTP_MAXSEG};
#endif

#endif /* WITH_SCTP */
