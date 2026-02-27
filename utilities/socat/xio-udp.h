/* source: xio-udp.h */
/* Copyright Gerhard Rieger 2001-2007 */
/* Published under the GNU General Public License V.2, see file COPYING */

#ifndef __xio_udp_h_included
#define __xio_udp_h_included 1

extern const union xioaddr_desc *xioaddrs_udp_connect[];
extern const union xioaddr_desc *xioaddrs_udp_listen[];
extern const union xioaddr_desc *xioaddrs_udp_sendto[];
extern const union xioaddr_desc *xioaddrs_udp_datagram[];
extern const union xioaddr_desc *xioaddrs_udp_recvfrom[];
extern const union xioaddr_desc *xioaddrs_udp_recv[];
extern const union xioaddr_desc *xioaddrs_udp4_connect[];
extern const union xioaddr_desc *xioaddrs_udp4_listen[];
extern const union xioaddr_desc *xioaddrs_udp4_sendto[];
extern const union xioaddr_desc *xioaddrs_udp4_datagram[];
extern const union xioaddr_desc *xioaddrs_udp4_recvfrom[];
extern const union xioaddr_desc *xioaddrs_udp4_recv[];
extern const union xioaddr_desc *xioaddrs_udp6_connect[];
extern const union xioaddr_desc *xioaddrs_udp6_listen[];
extern const union xioaddr_desc *xioaddrs_udp6_sendto[];
extern const union xioaddr_desc *xioaddrs_udp6_datagram[];
extern const union xioaddr_desc *xioaddrs_udp6_recvfrom[];
extern const union xioaddr_desc *xioaddrs_udp6_recv[];

extern int xioopen_ipdgram_listen(int argc, const char *argv[], struct opt *opts,
				  int rw, xiofile_t *fd,
			  unsigned groups, int af, int ipproto,
			  int protname);

#endif /* !defined(__xio_udp_h_included) */
