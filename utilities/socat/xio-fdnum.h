/* source: xio-fdnum.h */
/* Copyright Gerhard Rieger 2001-2007 */
/* Published under the GNU General Public License V.2, see file COPYING */

#ifndef __xio_fdnum_h_included
#define __xio_fdnum_h_included 1

extern const struct xioaddr_endpoint_desc xioaddr_fdnum1;
extern const union xioaddr_desc *xioaddrs_fdnum[];

extern int xioopen_fd(struct opt *opts, int rw, xiofile_t *xfd, int numfd1, int numfd2, int dummy2, int dummy3);

#endif /* !defined(__xio_fdnum_h_included) */
