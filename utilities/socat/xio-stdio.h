/* source: xio-stdio.h */
/* Copyright Gerhard Rieger 2001-2007 */
/* Published under the GNU General Public License V.2, see file COPYING */

#ifndef __xio_stdio_h_included
#define __xio_stdio_h_included 1


extern int xioopen_stdio_bi(xiofile_t *sock);

extern const union xioaddr_desc *xioaddrs_stdio[];
extern const union xioaddr_desc *xioaddrs_stdin[];
extern const union xioaddr_desc *xioaddrs_stdout[];
extern const union xioaddr_desc *xioaddrs_stderr[];

#endif /* !defined(__xio_stdio_h_included) */
