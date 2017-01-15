/* source: xio-sctp.h */
/* Copyright Gerhard Rieger 2008 */
/* Published under the GNU General Public License V.2, see file COPYING */

#ifndef __xio_sctp_h_included
#define __xio_sctp_h_included 1

extern const union xioaddr_desc *xioaddrs_sctp_connect[];
extern const union xioaddr_desc *xioaddrs_sctp_listen[];
extern const union xioaddr_desc *xioaddrs_sctp4_connect[];
extern const union xioaddr_desc *xioaddrs_sctp4_listen[];
extern const union xioaddr_desc *xioaddrs_sctp6_connect[];
extern const union xioaddr_desc *xioaddrs_sctp6_listen[];

extern const struct optdesc opt_sctp_nodelay;
extern const struct optdesc opt_sctp_maxseg;
extern const struct optdesc opt_sctp_maxseg_late;

#endif /* !defined(__xio_sctp_h_included) */
