/* source: xio-rawip.h */
/* Copyright Gerhard Rieger 2001-2007 */
/* Published under the GNU General Public License V.2, see file COPYING */

#ifndef __xio_rawip_h_included
#define __xio_rawip_h_included 1

extern const union xioaddr_desc *xioaddrs_rawip_sendto[];
extern const union xioaddr_desc *xioaddrs_rawip_datagram[];
extern const union xioaddr_desc *xioaddrs_rawip_recvfrom[];
extern const union xioaddr_desc *xioaddrs_rawip_recv[];
extern const union xioaddr_desc *xioaddrs_rawip4_sendto[];
extern const union xioaddr_desc *xioaddrs_rawip4_datagram[];
extern const union xioaddr_desc *xioaddrs_rawip4_recvfrom[];
extern const union xioaddr_desc *xioaddrs_rawip4_recv[];
extern const union xioaddr_desc *xioaddrs_rawip6_sendto[];
extern const union xioaddr_desc *xioaddrs_rawip6_datagram[];
extern const union xioaddr_desc *xioaddrs_rawip6_recvfrom[];
extern const union xioaddr_desc *xioaddrs_rawip6_recv[];

#endif /* !defined(__xio_rawip_h_included) */
