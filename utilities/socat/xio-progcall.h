/* source: xio-progcall.h */
/* Copyright Gerhard Rieger 2001-2009 */
/* Published under the GNU General Public License V.2, see file COPYING */

#ifndef __xio_progcall_h_included
#define __xio_progcall_h_included 1

extern const struct optdesc opt_leftfd;
extern const struct optdesc opt_leftinfd;
extern const struct optdesc opt_leftoutfd;
extern const struct optdesc opt_rightfd;
extern const struct optdesc opt_rightinfd;
extern const struct optdesc opt_rightoutfd;
extern const struct optdesc opt_path;
extern const struct optdesc opt_pipes;
extern const struct optdesc opt_pty;
extern const struct optdesc opt_openpty;
extern const struct optdesc opt_ptmx;
extern const struct optdesc opt_commtype;
extern const struct optdesc opt_stderr;
extern const struct optdesc opt_nofork;
extern const struct optdesc opt_sighup;
extern const struct optdesc opt_sigint;
extern const struct optdesc opt_sigquit;

extern int 
_xioopen_progcall(int xioflags,	/* XIO_RDONLY etc. */
		   struct single *xfd,
		   unsigned groups,
		   struct opt **opts,
		   int *duptostderr,
		   bool inter,
		   int form
		   );

extern int setopt_path(struct opt *opts, char **path);
extern
int _xioopen_redir_stderr(int fdo);

#endif /* !defined(__xio_progcall_h_included) */
