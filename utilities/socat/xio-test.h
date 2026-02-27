/* $Id$ */
/* Copyright Gerhard Rieger 2007 */
/* Published under the GNU General Public License V.2, see file COPYING */

#ifndef __xio_test_h_included
#define __xio_test_h_included 1

extern const union xioaddr_desc *xioaddrs_test[];
extern const union xioaddr_desc *xioaddrs_testuni[];
extern const union xioaddr_desc *xioaddrs_testrev[];

extern size_t xioread_test(struct single *sfd, void *buff, size_t bufsiz);
extern size_t xiowrite_test(struct single *sfd, const void *buff, size_t bytes);
extern size_t xiowrite_testrev(struct single *sfd, const void *buff, size_t bytes);

#endif /* !defined(__xio_test_h_included) */
