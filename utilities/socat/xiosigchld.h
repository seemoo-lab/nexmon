/* source: xiosigchld.h */
/* Copyright Gerhard Rieger 2012 */
/* Published under the GNU General Public License V.2, see file COPYING */

#ifndef __xiosigchld_h
#define __xiosigchld_h 1

#define NUMUNKNOWN 4
extern pid_t diedunknown[NUMUNKNOWN];	/* child died before it is registered */
#define diedunknown1 (diedunknown[0])
#define diedunknown2 (diedunknown[1])
#define diedunknown3 (diedunknown[2])
#define diedunknown4 (diedunknown[3])

extern int xiosetsigchild(xiofile_t *xfd, int (*callback)(struct single *));
extern void childdied(int signum
#if HAVE_SIGACTION
		      , siginfo_t *siginfo, void *context
#endif /* HAVE_SIGACTION */
		      );

extern int
   xiosigchld_register(pid_t pid,
			    void (*sigaction)(int, siginfo_t *, void *),
			    void *context);
extern int xiosigchld_unregister(pid_t pid);
extern int xiosigchld_clearall(void);

extern void xiosigaction_subaddr_ok(int signum, siginfo_t *siginfo, void *ucontext);
extern void xiosigaction_child(int signum, siginfo_t *siginfo, void *ucontext);

#endif /* !defined(__xiosigchld_h) */
