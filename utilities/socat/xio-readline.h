/* source: xio-readline.h */
/* Copyright Gerhard Rieger 2002-2007 */
/* Published under the GNU General Public License V.2, see file COPYING */

#ifndef __xio_readline_h_included
#define __xio_readline_h_included 1

extern const union xioaddr_desc *xioaddrs_readline[];

extern const struct optdesc opt_history_file;
extern const struct optdesc opt_prompt;
extern const struct optdesc opt_noprompt;
extern const struct optdesc opt_noecho;

extern ssize_t xioread_readline(struct single *pipe, void *buff, size_t bufsiz);extern void xioscan_readline(struct single *pipe, const void *buff, size_t bytes);

#endif /* !defined(__xio_readline_h_included) */
