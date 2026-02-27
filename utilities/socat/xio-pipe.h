/* source: xio-pipe.h */
/* Copyright Gerhard Rieger 2001-2007 */
/* Published under the GNU General Public License V.2, see file COPYING */

#ifndef __xio_pipe_h_included
#define __xio_pipe_h_included 1

extern const union xioaddr_desc *xioaddrs_pipe[];

extern int xioopen_fifo_unnamed(char *arg, xiofile_t *sock);

#endif /* !defined(__xio_pipe_h_included) */
