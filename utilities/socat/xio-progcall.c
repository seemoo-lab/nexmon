/* source: xio-progcall.c */
/* Copyright Gerhard Rieger */
/* Published under the GNU General Public License V.2, see file COPYING */

/* this file contains common code dealing with program calls (exec, system) */

#include "xiosysincludes.h"
#include "xioopen.h"
#include "xiosigchld.h"

#include "xio-process.h"
#include "xio-progcall.h"

#include "xio-socket.h"


static int reassignfds(int oldfd1, int oldfd2, int newfd1, int newfd2);


/* these options are used by address pty too */
#if HAVE_OPENPTY
const struct optdesc opt_openpty = { "openpty",   NULL, OPT_OPENPTY,     GROUP_PTY,   PH_BIGEN, TYPE_BOOL, 	OFUNC_SPEC };
#endif /* HAVE_OPENPTY */
#if HAVE_DEV_PTMX || HAVE_DEV_PTC
const struct optdesc opt_ptmx    = { "ptmx",      NULL, OPT_PTMX,        GROUP_PTY,   PH_BIGEN, TYPE_BOOL, 	OFUNC_SPEC };
#endif

#if WITH_EXEC || WITH_SYSTEM

#define MAXPTYNAMELEN 64

const struct optdesc opt_leftfd     = { "leftfd",     "left",     OPT_LEFTFD,     GROUP_FORK,   PH_PASTBIGEN,   TYPE_INT,	OFUNC_SPEC };
const struct optdesc opt_leftinfd   = { "leftin",     "fdin",     OPT_LEFTINFD,   GROUP_FORK,   PH_PASTBIGEN,   TYPE_INT,	OFUNC_SPEC };
const struct optdesc opt_leftoutfd  = { "leftout",    "fdout",    OPT_LEFTOUTFD,  GROUP_FORK,   PH_PASTBIGEN,   TYPE_INT,	OFUNC_SPEC };
const struct optdesc opt_rightfd    = { "rightfd",    "right",    OPT_RIGHTFD,    GROUP_FORK,   PH_PASTBIGEN,   TYPE_INT,	OFUNC_SPEC };
const struct optdesc opt_rightinfd  = { "rightinfd",  "rightin",  OPT_RIGHTINFD,  GROUP_FORK,   PH_PASTBIGEN,   TYPE_INT,	OFUNC_SPEC };
const struct optdesc opt_rightoutfd = { "rightoutfd", "rightout", OPT_RIGHTOUTFD, GROUP_FORK,   PH_PASTBIGEN,   TYPE_INT,	OFUNC_SPEC };
const struct optdesc opt_path    = { "path",      NULL, OPT_PATH,        GROUP_EXEC,   PH_PREEXEC,     TYPE_STRING,	OFUNC_SPEC };
const struct optdesc opt_pipes   = { "pipes",     NULL, OPT_PIPES,       GROUP_FORK,   PH_BIGEN,       TYPE_BOOL, 	OFUNC_SPEC };
#if HAVE_PTY
const struct optdesc opt_pty     = { "pty",       NULL, OPT_PTY,         GROUP_FORK,   PH_BIGEN, TYPE_BOOL, 	OFUNC_SPEC };
#endif
const struct optdesc opt_commtype= { "commtype",  NULL,  OPT_COMMTYPE,    GROUP_FORK,   PH_BIGEN,       TYPE_STRING,     OFUNC_SPEC };
const struct optdesc opt_stderr  = { "stderr",    NULL, OPT_STDERR,      GROUP_FORK,   PH_PASTFORK,        TYPE_BOOL,	OFUNC_SPEC };
const struct optdesc opt_nofork  = { "nofork",    NULL, OPT_NOFORK,      GROUP_FORK,   PH_BIGEN,       TYPE_BOOL,       OFUNC_SPEC };
const struct optdesc opt_sighup  = { "sighup",    NULL, OPT_SIGHUP,      GROUP_PARENT, PH_LATE,        TYPE_CONST,      OFUNC_SIGNAL, SIGHUP };
const struct optdesc opt_sigint  = { "sigint",    NULL, OPT_SIGINT,      GROUP_PARENT, PH_LATE,        TYPE_CONST,      OFUNC_SIGNAL, SIGINT };
const struct optdesc opt_sigquit = { "sigquit",   NULL, OPT_SIGQUIT,     GROUP_PARENT, PH_LATE,        TYPE_CONST,      OFUNC_SIGNAL, SIGQUIT };

int getcommtype(const char *commname) {
   struct wordent commnames[] = {
      { "pipes",       (void *)XIOCOMM_PIPES },
      { "pty",         (void *)XIOCOMM_PTY },
      { "ptys",        (void *)XIOCOMM_PTYS },
      { "socketpair",  (void *)XIOCOMM_SOCKETPAIR },
      { "socketpairs", (void *)XIOCOMM_SOCKETPAIRS },
      { "tcp",         (void *)XIOCOMM_TCP },
      { "tcp4",        (void *)XIOCOMM_TCP4 },
      { "tcp4listen",  (void *)XIOCOMM_TCP4_LISTEN },
      { NULL }
   } ;
   const struct wordent *comm_ent;
   comm_ent = keyw((struct wordent *)commnames, commname,
		   sizeof(commnames)/sizeof(struct wordent));
   if (!comm_ent) {
      return -1;
   }
   return (int)comm_ent->desc;
}

/* fork for exec/system, but return before exec'ing.
   return=0: is child process
   return>0: is parent process
   return<0: error occurred, assume parent process and no child exists !!!
   function formerly known as _xioopen_foxec()
 */
int _xioopen_progcall(int xioflags,	/* XIO_RDONLY etc. */
		       struct single *xfd,
		       unsigned groups,
		       struct opt **copts, /* in: opts; out: opts for child */
		       int *duptostderr,
		       bool inter,	/* is interaddr, not endpoint */
		       int form		/* with interaddr: =2: FDs 1,0--4,3
					   =1: FDs 1--0 */
		) {
   struct single *fd = xfd;
   struct opt *popts;	/* parent process options */
   int numleft;
   int sv[2], rdpip[2], wrpip[2];
   int saverfd = -1, savewfd = -1;	/* with inter addr, save assigned right fds */
   int rw = (xioflags & XIO_ACCMODE);
   char *commname;
   int commtype = XIOCOMM_SOCKETPAIRS;
   bool usepipes = false;
#if HAVE_PTY
   int ptyfd = -1, ttyfd = -1;
   bool usebestpty = false;	/* use the best available way to open pty */
#if defined(HAVE_DEV_PTMX) || defined(HAVE_DEV_PTC)
   bool useptmx = false;	/* use /dev/ptmx or equivalent */
#endif
#if HAVE_OPENPTY
   bool useopenpty = false;	/* try only openpty */
#endif	/* HAVE_OPENPTY */
   bool usepty = false;		/* any of the pty options is selected */
   char ptyname[MAXPTYNAMELEN];
#endif /* HAVE_PTY */
   pid_t pid = 0;	/* mostly int */
   int leftfd[2] = { 0, 1 };
#  define fdi (leftfd[0])
#  define fdo (leftfd[1])
   int rightfd[2] = { 3, 4 };
#  define rightin (rightfd[0])
#  define rightout (rightfd[1])
   short result;
   bool withstderr = false;
   bool nofork = false;
   bool withfork;

   popts = moveopts(*copts, GROUP_ALL);
   if (applyopts_single(fd, popts, PH_INIT) < 0)  return -1;
   applyopts2(-1, popts, PH_INIT, PH_EARLY);

   retropt_bool(popts, OPT_NOFORK, &nofork);
   withfork = !nofork;

   if ((retropt_string(popts, OPT_COMMTYPE, &commname)) >= 0) {
      if ((commtype = getcommtype(commname)) < 0) {
	 Error1("bad communication type \"%s\"", commname);
	 commtype = XIOCOMM_SOCKETPAIRS;
      }
   }

   retropt_bool(popts, OPT_PIPES, &usepipes);
#if HAVE_PTY
   retropt_bool(popts, OPT_PTY, &usebestpty);
#if HAVE_OPENPTY
   retropt_bool(popts, OPT_OPENPTY, &useopenpty);
#endif
#if defined(HAVE_DEV_PTMX) || defined(HAVE_DEV_PTC)
   retropt_bool(popts, OPT_PTMX, &useptmx);
#endif
   usepty = (usebestpty
#if HAVE_OPENPTY
	     || useopenpty
#endif
#if defined(HAVE_DEV_PTMX) || defined(HAVE_DEV_PTC)
	     || useptmx
#endif
	     );
   if (usepipes && usepty) {
      Warn("_xioopen_progcall(): options \"pipes\" and \"pty\" must not be specified together; ignoring \"pipes\"");
      usepipes = false;
   }
#endif /* HAVE_PTY */

   if (usepty) {
      commtype = XIOCOMM_PTY;
   } else if (usepipes) {
      commtype = XIOCOMM_PIPES;
   }

   /*------------------------------------------------------------------------*/
   /* retrieve options regarding file descriptors */
   if (!retropt_int(popts, OPT_LEFTFD,  &fdi)) {
      fdo = fdi;
   }

   if (retropt_int(popts, OPT_LEFTINFD,  (unsigned short *)&fdi) >= 0) {
      if ((xioflags&XIO_ACCMODE) == XIO_RDONLY) {
	 Error("_xioopen_progcall(): option fdin is useless in read-only mode");
      }
   }
   if (retropt_int(popts, OPT_LEFTOUTFD, (unsigned short *)&fdo) >= 0) {
      if ((xioflags&XIO_ACCMODE) == XIO_WRONLY) {
	 Error("_xioopen_progcall(): option fdout is useless in write-only mode");
      }
   }

   if (!retropt_int(popts, OPT_RIGHTFD,  &rightin)) {
      rightout = rightin;
   }
   retropt_int(popts, OPT_RIGHTINFD,  &rightin);
   retropt_int(popts, OPT_RIGHTOUTFD, &rightout);
   /* when the superordinate communication type provides two distinct fds we
      cannot pass just one fd to the program */
   if (rw == XIO_RDWR && rightin==rightout) {
      struct stat rstat, wstat;
      if (Fstat(xfd->rfd, &rstat) < 0)
	 Error2("fstat(%d, ...): %s", xfd->rfd, strerror(errno));
      if (Fstat(xfd->wfd, &wstat) < 0)
	 Error2("fstat(%d, ...): %s", xfd->wfd, strerror(errno));
      if (memcmp(&rstat, &wstat, sizeof(rstat))) {
	 Error("exec/system: your rightfd options require the same FD for both directions but the communication environment provides two different FDs");
      }
   }

   /*------------------------------------------------------------------------*/
   if (rw == XIO_WRONLY) {
      if (fd->howtoclose == XIOCLOSE_UNSPEC) {
	 fd->howtoclose = XIOCLOSE_SLEEP_SIGTERM;
      }
   }
   if (withfork) {
      const char *typename;
      if (!(xioflags&XIO_MAYCHILD)) {
	 Error("fork for exec not allowed in this context");
	 /*!! free something */
	 return -1;
      }
      fd->flags |= XIO_DOESCHILD;

      switch (commtype) {
      case XIOCOMM_PIPES:       typename = "pipes"; break;
#if HAVE_PTY
      case XIOCOMM_PTY:         typename = "pty"; break;
      case XIOCOMM_PTYS:        typename = "two pty's"; break;
#endif /* HAVE_PTY */
      case XIOCOMM_SOCKETPAIR:  typename = "socketpair"; break;
      case XIOCOMM_SOCKETPAIRS: typename = "two socketpairs"; break;
#if _WITH_TCP
      case XIOCOMM_TCP:         typename = "TCP socket pair"; break;
      case XIOCOMM_TCP4:        typename = "TCP4 socket pair"; break;
      case XIOCOMM_TCP4_LISTEN: typename = "TCP4 listen socket pair"; break;
#endif
      default:			typename = NULL; break;
      }
      Notice2("forking off child, using %s for %s",
	      typename, ddirection[rw]);
   }
   applyopts(-1, popts, PH_PREBIGEN);

   if (inter) {
      saverfd = xfd->rfd;
      savewfd = xfd->wfd;
      xfd->howtoshut = XIOSHUT_UNSPEC;
      xfd->howtoclose = XIOCLOSE_UNSPEC;
   }

   if (!withfork) {
      /*0 struct single *stream1, *stream2;*/

      free(*copts);
      *copts = moveopts(popts, GROUP_ALL);
      /* what if WE are sock1 ? */
#if 1
      if (!(xioflags & XIO_MAYEXEC /* means exec+nofork */)) {
	 Error("nofork option is not allowed here");
	 /*!! free something */
	 return -1;
      }
      fd->flags |= XIO_DOESEXEC;
#else      /*!! */
      if (sock1 == NULL) {
	 Fatal("nofork option must no be applied to first socat address");
      }
#endif
      if (fd->howtoclose == XIOCLOSE_UNSPEC) {
	 fd->howtoclose = XIOCLOSE_CLOSE;
      }

#if 0 /*!! */
      if (sock1->tag == XIO_TAG_DUAL) {
	 stream1 = &sock1->dual.stream[0]->stream;
	 stream2 = &sock1->dual.stream[1]->stream;
      } else {
	 stream1 = &sock1->stream;
	 stream2 = &sock1->stream;
      }
      if (stream1->dtype == DATA_READLINE || stream2->dtype == DATA_READLINE ||
	  stream1->dtype == DATA_OPENSSL  || stream2->dtype == DATA_OPENSSL
	  ) {
	 Error("with option nofork, openssl and readline in address1 do not work");
      }
      if (stream1->lineterm != LINETERM_RAW ||
	  stream2->lineterm != LINETERM_RAW ||
	  stream1->ignoreeof || stream2->ignoreeof) {
	 Warn("due to option nofork, address1 options for lineterm and igoreeof do not apply");
      }
#endif

      /*! problem: when fdi==WRFD(sock[0]) or fdo==RDFD(sock[0]) */
      if (rw != XIO_WRONLY) {
	 if (XIO_GETRDFD(sock[0]/*!!!*/) == fdi) {
	    if (Fcntl_l(fdi, F_SETFD, 0) < 0) {
	       Warn2("fcntl(%d, F_SETFD, 0): %s", fdi, strerror(errno));
	    }
	    if (Dup2(XIO_GETRDFD(sock[0]), fdi) < 0) {
	       Error3("dup2(%d, %d): %s",
		      XIO_GETRDFD(sock[0]), fdi, strerror(errno));
	    }
	    /*0 Info2("dup2(%d, %d)", XIO_GETRDFD(sock[0]), fdi);*/
	 } else {
	    if (Dup2(XIO_GETRDFD(sock[0]), fdi) < 0) {
	       Error3("dup2(%d, %d): %s",
		      XIO_GETRDFD(sock[0]), fdi, strerror(errno));
	    }
	    /*0 Info2("dup2(%d, %d)", XIO_GETRDFD(sock[0]), fdi);*/
	 }
      }
      if (rw != XIO_RDONLY) {
	 if (XIO_GETWRFD(sock[0]) == fdo) {
	    if (Fcntl_l(fdo, F_SETFD, 0) < 0) {
	       Warn2("fcntl(%d, F_SETFD, 0): %s", fdo, strerror(errno));
	    }
	    if (Dup2(XIO_GETWRFD(sock[0]), fdo) < 0) {
	       Error3("dup2(%d, %d): %s)",
		      XIO_GETWRFD(sock[0]), fdo, strerror(errno));
	    }
	    /*0 Info2("dup2(%d, %d)", XIO_GETWRFD(sock[0]), fdo);*/
	 } else {
	    if (Dup2(XIO_GETWRFD(sock[0]), fdo) < 0) {
	       Error3("dup2(%d, %d): %s)",
		      XIO_GETWRFD(sock[0]), fdo, strerror(errno));
	    }
	    /*0 Info2("dup2(%d, %d)", XIO_GETWRFD(sock[0]), fdo);*/
	 }
      }
   } else /* withfork */
      /* create fd pair(s), set related xfd parameters, and apply options */
      switch (commtype) {

#if HAVE_PTY
      case XIOCOMM_PTY:
	 /*!indent*/
#if defined(HAVE_DEV_PTMX)
#  define PTMX "/dev/ptmx"	/* Linux */
#elif HAVE_DEV_PTC
#  define PTMX "/dev/ptc"	/* AIX 4.3.3 */
#endif
      fd->dtype = XIODATA_PTY;
#if 0
      if (fd->howtoshut == XIOSHUT_UNSPEC) {
	 fd->howtoshut = XIOSHUTRD_SIGTERM|XIOSHUTWR_SIGHUP;
      }
      if (fd->howtoclose == XIOCLOSE_UNSPEC) {
	 fd->howtoclose = XIOCLOSE_CLOSE_SIGTERM;
      }
#endif

      if (xiopty(usebestpty||useptmx, &ttyfd, &ptyfd) < 0) {
	 return -1;
      }

      free(*copts);
      if ((*copts = moveopts(popts, GROUP_TERMIOS|GROUP_FORK|GROUP_EXEC|GROUP_PROCESS)) == NULL) {
	 return -1;
      }
      applyopts_cloexec(ptyfd, popts);/*!*/

      /* exec:...,pty did not kill child process under some circumstances */
      if (fd->howtoshut == XIOSHUT_UNSPEC) {
	 fd->howtoshut = XIOSHUTRD_SIGTERM|XIOSHUTWR_SIGHUP;
      }
      if (fd->howtoclose == XIOCLOSE_UNSPEC) {
	 fd->howtoclose = XIOCLOSE_CLOSE_SIGTERM;
      }

      /* this for parent, was after fork */
      applyopts(ptyfd, popts, PH_FD);
      applyopts(ptyfd, popts, PH_LATE);
      if (applyopts_single(fd, popts, PH_LATE) < 0)  return -1;

      if (XIOWITHRD(rw))  fd->rfd = ptyfd;
      if (XIOWITHWR(rw))  fd->wfd = ptyfd;

      /* this for child, was after fork */
      applyopts(ttyfd, *copts, PH_FD);

      break;
#endif /* HAVE_PTY */

      case XIOCOMM_PIPES: {
	 /*!indent*/
      struct opt *popts2, *copts2;

      if (rw == XIO_RDWR) {
	 fd->dtype = XIODATA_2PIPE;
      }
      if (fd->howtoshut == XIOSHUT_UNSPEC || fd->howtoshut == XIOSHUT_DOWN) {
	 fd->howtoshut = XIOSHUT_CLOSE;
      }
      if (fd->howtoclose == XIOCLOSE_UNSPEC) {
	 fd->howtoclose = XIOCLOSE_CLOSE;
      }

      if (rw != XIO_WRONLY) {
	 if (Pipe(rdpip) < 0) {
	    Error2("pipe(%p): %s", rdpip, strerror(errno));
	    return -1;
	 }
      }
      /*0 Info2("pipe({%d,%d})", rdpip[0], rdpip[1]);*/
      /* rdpip[0]: read by socat; rdpip[1]: write by child */
      free(*copts);
      if ((*copts = moveopts(popts, GROUP_FORK|GROUP_EXEC|GROUP_PROCESS))
	  == NULL) {
	 return -1;
      }

      popts2 = copyopts(popts, GROUP_ALL);
      copts2 = copyopts(*copts, GROUP_ALL);

      if (rw != XIO_WRONLY) {
	 applyopts_cloexec(rdpip[0], popts);
	 applyopts(rdpip[0], popts, PH_FD);
	 applyopts(rdpip[1], *copts, PH_FD);
      }

      if (rw != XIO_RDONLY) {
	 if (Pipe(wrpip) < 0) {
	    Error2("pipe(%p): %s", wrpip, strerror(errno));
	    return -1;
	 }
      }
      /*0 Info2("pipe({%d,%d})", wrpip[0], wrpip[1]);*/

      /* wrpip[1]: write by socat; wrpip[0]: read by child */
      if (rw != XIO_RDONLY) {
	 applyopts_cloexec(wrpip[1], popts2);
	 applyopts(wrpip[1], popts2, PH_FD);
	 applyopts(wrpip[0], copts2, PH_FD);
      }

      /* this for parent, was after fork */
      if (XIOWITHRD(rw))  fd->rfd = rdpip[0];
      if (XIOWITHWR(rw))  fd->wfd = wrpip[1];
      applyopts(fd->rfd, popts, PH_FD);
      applyopts(fd->rfd, popts, PH_LATE);
      if (applyopts_single(fd, popts, PH_LATE) < 0)  return -1;
      break;
      }

      case XIOCOMM_SOCKETPAIR: {
	 /*!indent*/
      int pf = AF_UNIX;
      retropt_int(popts, OPT_PROTOCOL_FAMILY, &pf);
      result = xiosocketpair(popts, pf, SOCK_STREAM, 0, sv);
      if (result < 0) {
	 return -1;
      }

      if (xfd->howtoshut == XIOSHUT_UNSPEC) {
	 xfd->howtoshut = XIOSHUT_DOWN;
      }
      if (xfd->howtoclose == XIOCLOSE_UNSPEC) {
	 xfd->howtoclose = XIOCLOSE_CLOSE;
      }

      /*0 Info5("socketpair(%d, %d, %d, {%d,%d})",
	d, type, protocol, sv[0], sv[1]);*/
      free(*copts);
      if ((*copts = moveopts(popts, GROUP_FORK|GROUP_EXEC|GROUP_PROCESS)) == NULL) {
	 return -1;
      }
      applyopts(sv[0], *copts, PH_PASTSOCKET);
      applyopts(sv[1], popts, PH_PASTSOCKET);

      applyopts_cloexec(sv[0], *copts);
      applyopts(sv[0], *copts, PH_FD);
      applyopts(sv[1], popts, PH_FD);

      applyopts(sv[0], *copts, PH_PREBIND);
      applyopts(sv[0], *copts, PH_BIND);
      applyopts(sv[0], *copts, PH_PASTBIND);
      applyopts(sv[1], popts, PH_PREBIND);
      applyopts(sv[1], popts, PH_BIND);
      applyopts(sv[1], popts, PH_PASTBIND);

Warn1("xio-progcall.c: fd->howtoshut == %d", fd->howtoshut);
      if (inter || fd->howtoshut == XIOSHUT_UNSPEC) {
	 fd->howtoshut = XIOSHUT_DOWN;
      }
      if (fd->howtoclose == XIOCLOSE_UNSPEC) {
	 fd->howtoclose = XIOCLOSE_SIGTERM;
      }

      /* this for parent, was after fork */
   /*!!!*/ Warn2("2: fd->rfd==%d, fd->wfd==%d", fd->rfd, fd->wfd);
      if (XIOWITHRD(rw))  fd->rfd = sv[0];
      if (XIOWITHWR(rw))  fd->wfd = sv[0];
   /*!!!*/ Warn2("3: fd->rfd==%d, fd->wfd==%d", fd->rfd, fd->wfd);
      applyopts(fd->rfd, popts, PH_FD);
      applyopts(fd->rfd, popts, PH_LATE);
      if (applyopts_single(fd, popts, PH_LATE) < 0)  return -1;
      }
	 break;

      case XIOCOMM_TCP:
      case XIOCOMM_TCP4: {
	 /*!indent*/
	 int pf = AF_INET;
	 xiofd_t socatfd, execfd;
	 retropt_int(popts, OPT_PROTOCOL_FAMILY, &pf);
	 if (xiocommpair(commtype, XIOWITHWR(rw), XIOWITHRD(rw),
			 0, &socatfd, &execfd) < 0) {
	    return -1;
	 }
      free(*copts);
      if ((*copts = moveopts(popts, GROUP_FORK|GROUP_EXEC|GROUP_PROCESS)) == NULL) {
	 return -1;
      }
      sv[0] = socatfd.rfd;	/*!!! r/w */
      sv[1] = execfd.wfd;
      applyopts(socatfd.rfd, *copts, PH_PASTSOCKET);
      applyopts(execfd.rfd, popts, PH_PASTSOCKET);

      applyopts_cloexec(sv[0], *copts);
      applyopts(sv[0], *copts, PH_FD);
      applyopts(sv[1], popts, PH_FD);

      applyopts(sv[0], *copts, PH_PREBIND);
      applyopts(sv[0], *copts, PH_BIND);
      applyopts(sv[0], *copts, PH_PASTBIND);
      applyopts(sv[1], popts, PH_PREBIND);
      applyopts(sv[1], popts, PH_BIND);
      applyopts(sv[1], popts, PH_PASTBIND);

Warn1("xio-progcall.c: fd->howtoshut == %d", fd->howtoshut);
      if (inter || fd->howtoshut == XIOSHUT_UNSPEC) {
	 fd->howtoshut = XIOSHUT_DOWN;
      }
      if (fd->howtoclose == XIOCLOSE_UNSPEC) {
	 fd->howtoclose = XIOCLOSE_SIGTERM;
      }

      /* this for parent, was after fork */
      if (XIOWITHRD(rw))  fd->rfd = sv[0];
      if (XIOWITHWR(rw))  fd->wfd = sv[0];
      applyopts(fd->rfd, popts, PH_FD);
      applyopts(fd->rfd, popts, PH_LATE);
      if (applyopts_single(fd, popts, PH_LATE) < 0)  return -1;
      }
	 break;

#if LATER
      case XIOCOMM_TCP4_LISTEN: {
	 /*!indent*/
	 int pf = AF_INET;
	 xiofd_t socatfd, execfd;
	 retropt_int(popts, OPT_PROTOCOL_FAMILY, &pf);
	 if (xiocommpair(commtype, XIOWITHWR(rw), XIOWITHRD(rw),
			 0, &socatfd, &execfd) < 0) {
	    return -1;
	 }

	 free(*copts);
	 if ((*copts = moveopts(popts, GROUP_TERMIOS|GROUP_FORK|GROUP_EXEC|GROUP_PROCESS)) == NULL) {
	    return -1;
	 }
	 applyopts_cloexec(ptyfd, popts);/*!*/
      }
	 break;
#endif /* LATER */

      case XIOCOMM_SOCKETPAIRS:
      case XIOCOMM_PTYS: {
	 xiofd_t socatfd, execfd;
	 struct termios andmask, ormask;
	 switch (commtype) {
	 case XIOCOMM_SOCKETPAIRS:
	    if (xiocommpair(commtype, XIOWITHWR(rw), XIOWITHRD(rw),
			    0, &socatfd, &execfd, PF_UNIX, SOCK_STREAM, 0) < 0)
	       return -1;
	    break;
	 case XIOCOMM_PTYS:
	    if (xiocommpair(commtype, XIOWITHWR(rw), XIOWITHRD(rw),
			    0, &socatfd, &execfd, &andmask, &ormask) < 0)
	       return -1;
	    break;
	 }

	 free(*copts);
	 if ((*copts = copyopts(popts, GROUP_TERMIOS|GROUP_FORK)) == NULL) {
	    return -1;
	 }
	 if (socatfd.rfd >= 0) {
	    applyopts_cloexec(socatfd.rfd, *copts);/*!*/
	    applyopts(socatfd.rfd, *copts, PH_FD);
	    applyopts(socatfd.rfd, *copts, PH_LATE);
	 }
	 if (applyopts_single(xfd, *copts, PH_LATE) < 0)  return -1;

	 free(*copts);
	 if ((*copts = moveopts(popts, GROUP_TERMIOS|GROUP_FORK|GROUP_EXEC|GROUP_PROCESS)) == NULL) {
	    return -1;
	 }
	 if (socatfd.wfd >= 0) {
	    applyopts_cloexec(socatfd.wfd, *copts);/*!*/
	    applyopts(socatfd.wfd, *copts, PH_FD);
	    applyopts(socatfd.wfd, *copts, PH_LATE);
	 }
	 if (applyopts_single(xfd, *copts, PH_LATE) < 0)  return -1;

	 if (XIOWITHRD(rw))  xfd->rfd = socatfd.rfd;
	 if (XIOWITHWR(rw))  xfd->wfd = socatfd.wfd;
	 xfd->dtype      = socatfd.dtype;
	 if (xfd->howtoshut == XIOSHUT_UNSPEC)
	    xfd->howtoshut  = socatfd.howtoshut;
	 if (fd->howtoclose == XIOCLOSE_UNSPEC) {
	    fd->howtoclose = XIOWITHRD(rw)?XIOCLOSE_CLOSE_SIGTERM:XIOCLOSE_SLEEP_SIGTERM;
	 }
	 wrpip[0] = execfd.rfd;
	 rdpip[1] = execfd.wfd;
	 rdpip[0] = socatfd.rfd;
	 wrpip[1] = socatfd.wfd;
      }
	 break;

	 default:
	    Error1("_xioopen_progcall() internal: commtype %d not handled",
		   commtype);
	    break;
      }

   /*0   if ((optpr = copyopts(*copts, GROUP_PROCESS)) == NULL)
     return -1;*/
   retropt_bool(*copts, OPT_STDERR, &withstderr);

   xiosetchilddied();	/* set SIGCHLD handler */

   xiosetchilddied();	/* set SIGCHLD handler */

   if (withfork) {
      sigset_t set, oldset;

      sigemptyset(&set);
      sigaddset(&set, SIGCHLD);
      Sigprocmask(SIG_BLOCK, &set, &oldset);	/* disable SIGCHLD */
      pid = xio_fork(true, E_ERROR);
      if (pid < 0) {
	 Sigprocmask(SIG_SETMASK, &oldset, NULL);
	 Error1("fork(): %s", strerror(errno));
	 return -1;
      }

      if (pid > 0) {
	 /* for parent (this is our socat process) */
	 xiosigchld_register(pid, xiosigaction_child, fd);
	 Sigprocmask(SIG_SETMASK, &oldset, NULL);	/* enable SIGCHLD */
      }

      if (pid == 0) {	/* child */
	 /* drop parents locks, reset FIPS... */
	 if (xio_forked_inchild() != 0) {
	    Exit(1);
	 }
	 Sigprocmask(SIG_SETMASK, &oldset, NULL);	/* enable SIGCHLD */
      }
   }
   if (!withfork || pid == 0) {	/* child */
      uid_t user;
      gid_t group;

      if (withfork) {
	 /* The child should have default handling for SIGCHLD. */
	 /* In particular, it's not defined whether ignoring SIGCHLD is inheritable. */
	 if (Signal(SIGCHLD, SIG_DFL) == SIG_ERR) {
	    Warn1("signal(SIGCHLD, SIG_DFL): %s", strerror(errno));
	 }

	 /* dup2() the fds to desired values, close old fds, and apply late 
	    options */
	 switch (commtype) {
#if HAVE_PTY
	 case XIOCOMM_PTY:
	    if (rw != XIO_RDONLY && fdi != ttyfd) {
	       if (Dup2(ttyfd, fdi) < 0) {
		  Error3("dup2(%d, %d): %s", ttyfd, fdi, strerror(errno));
		  return -1; }
	       /*0 Info2("dup2(%d, %d)", ttyfd, fdi);*/
	    }
	    if (rw != XIO_WRONLY && fdo != ttyfd) {
	       if (Dup2(ttyfd, fdo) < 0) {
		  Error3("dup2(%d, %d): %s", ttyfd, fdo, strerror(errno));
		  return -1; }
	       /*0 Info2("dup2(%d, %d)", ttyfd, fdo);*/
	    }
	    if ((rw == XIO_RDONLY || fdi != ttyfd) &&
		(rw == XIO_WRONLY || fdo != ttyfd)) {
	       applyopts_cloexec(ttyfd, *copts);
	    }

	    applyopts(ttyfd, *copts, PH_LATE);
	    applyopts(ttyfd, *copts, PH_LATE2);
	    break;
#endif /* HAVE_PTY */

	 case XIOCOMM_PIPES:
	 case XIOCOMM_SOCKETPAIRS:
	 case XIOCOMM_PTYS:
	    {
	    /*!indent*/
	       /* we might have a temporary conflict between what FDs are
		  currently allocated, and which are to be used. We try to find
		  a graceful solution via temporary descriptors */
	       int tmpi, tmpo;

	       /* needed with system() (not with exec()) */
	       if (XIOWITHRD(rw))  Close(rdpip[0]);
	       if (XIOWITHWR(rw))  Close(wrpip[1]);
#if 0
	       /*! might not be needed */
	       if (XIOWITHRD(rw))  Close(rdpip[0]);
	       if (XIOWITHWR(rw))  Close(wrpip[1]);

	       if (fdi == rdpip[1]) {	/* a conflict here */
		  if ((tmpi = Dup(wrpip[0])) < 0) {
		     Error2("dup(%d): %s", wrpip[0], strerror(errno));
		     return -1;
		  }
		  /*0 Info2("dup(%d) -> %d", wrpip[0], tmpi);*/
		  rdpip[1] = tmpi;
	       }
	       if (fdo == wrpip[0]) {	/* a conflict here */
		  if ((tmpo = Dup(rdpip[1])) < 0) {
		     Error2("dup(%d): %s", rdpip[1], strerror(errno));
		     return -1;
		  }
		  /*0 Info2("dup(%d) -> %d", rdpip[1], tmpo);*/
		  wrpip[0] = tmpo;
	       }
	       
	       if (rw != XIO_WRONLY && rdpip[1] != fdo) {
		  if (Dup2(rdpip[1], fdo) < 0) {
		     Error3("dup2(%d, %d): %s", rdpip[1], fdo, strerror(errno));
		     return -1;
		  }
		  Close(rdpip[1]);
		  /*0 Info2("dup2(%d, %d)", rdpip[1], fdo);*/
		  /*0 applyopts_cloexec(fdo, *copts);*/
	       }
	       if (rw != XIO_RDONLY && wrpip[0] != fdi) {
		  if (Dup2(wrpip[0], fdi) < 0) {
		     Error3("dup2(%d, %d): %s", wrpip[0], fdi, strerror(errno));
		     return -1;
		  }
		  Close(wrpip[0]);
		  /*0 Info2("dup2(%d, %d)", wrpip[0], fdi);*/
		  /*0 applyopts_cloexec(wrpip[0], *copts);*/	/* option is already consumed! */
		  /* applyopts_cloexec(fdi, *copts);*/	/* option is already consumed! */
	       }
#else
	       result = reassignfds(XIOWITHWR(rw)?wrpip[0]:-1,
				    XIOWITHRD(rw)?rdpip[1]:-1,
				    fdi, fdo);
	       if (result < 0)  return result;
#endif
	       applyopts(fdi, *copts, PH_LATE);
	       applyopts(fdo, *copts, PH_LATE);
	       applyopts(fdi, *copts, PH_LATE2);
	       applyopts(fdo, *copts, PH_LATE2);
	       break;
	 }
	 case XIOCOMM_SOCKETPAIR:
	 case XIOCOMM_TCP:
	 case XIOCOMM_TCP4:
	 case XIOCOMM_TCP4_LISTEN:
	    /*!indent*/
	       if (rw != XIO_RDONLY && fdi != sv[1]) {
		  if (Dup2(sv[1], fdi) < 0) {
		     Error3("dup2(%d, %d): %s", sv[1], fdi, strerror(errno));
		     return -1; }
		  /*0 Info2("dup2(%d, %d)", sv[1], fdi);*/
	       }
	       if (rw != XIO_WRONLY && fdo != sv[1]) {
		  if (Dup2(sv[1], fdo) < 0) {
		     Error3("dup2(%d, %d): %s", sv[1], fdo, strerror(errno));
		     return -1; }
		  /*0 Info2("dup2(%d, %d)", sv[1], fdo);*/
	       }
	       if (fdi != sv[1] && fdo != sv[1]) {
		  applyopts_cloexec(sv[1], *copts);
		  Close(sv[1]);
	       }

	       applyopts(fdi, *copts, PH_LATE);
	       applyopts(fdi, *copts, PH_LATE2);
	       Close(sv[1]);
	       break;

	 default:
	    Error1("_xioopen_progcall() internal: commtype %d not handled",
		   commtype);
	    break;

	 }

	 /* in case of an inter address, assign the right side FDs (e.g. 3 and 4) */
	 if (inter) {
	    Info2("preparing the right side FDs %d and %d for exec process",
		  rightin, rightout);
	    result = reassignfds(XIOWITHRD(rw)?saverfd:-1,
				 XIOWITHWR(rw)?savewfd:-1,
				 rightin, form==2?rightout:STDOUT_FILENO);
	    if (result < 0)  return result;
	    if (form == 2) {
	       Fcntl_l(rightin, F_SETFD, 0);
	       Fcntl_l(rightout, F_SETFD, 0);
	    }
	 }

      } /* withfork */
      else /* !withfork */ {
	 applyopts(-1, *copts, PH_LATE);
	 applyopts(-1, *copts, PH_LATE2);
      }
      _xioopen_setdelayeduser();
      /* set group before user - maybe you are not permitted afterwards */
      if (retropt_gidt(*copts, OPT_SETGID, &group) >= 0) {
	 Setgid(group);
      }
      if (retropt_uidt(*copts, OPT_SETUID, &user) >= 0) {
	 Setuid(user);
      }
      if (withstderr) {
	 *duptostderr = fdo;
      } else {
	 *duptostderr = -1;
      }

      return 0;	/* indicate child process */
   }

   /* for parent (this is our socat process) */
   Notice1("forked off child process "F_pid, pid);

#if 0
   if ((popts = copyopts(*copts,
			 GROUP_FD|GROUP_TERMIOS|GROUP_FORK|GROUP_SOCKET|GROUP_SOCK_UNIX|GROUP_FIFO)) == NULL)
      return STAT_RETRYLATER;
#endif

   /* in parent: close fds that are only needed in child */
   switch (commtype) {
#if HAVE_PTY
   case XIOCOMM_PTY:
      if (Close(ttyfd) < 0) {
	 Info2("close(%d): %s", ttyfd, strerror(errno));
      }
      break;
#endif /* HAVE_PTY */
   case XIOCOMM_SOCKETPAIR:
   case XIOCOMM_TCP:
   case XIOCOMM_TCP4:
   case XIOCOMM_TCP4_LISTEN:
      Close(sv[1]);
      break;
   case XIOCOMM_PIPES:
   default:
      if (XIOWITHWR(rw))  Close(wrpip[0]);
      if (XIOWITHRD(rw))  Close(rdpip[1]);
      break;
   }

   fd->child.pid = pid;

   if (applyopts_single(fd, popts, PH_LATE) < 0)  return -1;
   applyopts_signal(fd, popts);
   if ((numleft = leftopts(popts)) > 0) {
      Error1("%d option(s) could not be used", numleft);
      showleft(popts);
      return STAT_NORETRY;
   }

   if (inter) {
      if (XIOWITHRD(rw))  Close(saverfd);
      if (XIOWITHWR(rw))  Close(savewfd);
   }

   return pid;	/* indicate parent (main) process */
}

#endif /* WITH_EXEC || WITH_SYSTEM */


int setopt_path(struct opt *opts, char **path) {
   if (retropt_string(opts, OPT_PATH, path) >= 0) {
      if (setenv("PATH", *path, 1) < 0) {
	 Error1("setenv(\"PATH\", \"%s\", 1): insufficient space", *path);
	 return -1;
      }
   }
   return 0;
}


/* like dup(), but prints error on failure */
static int xiodup(int oldfd) {
   int result;
   if ((result = Dup(oldfd)) >= 0)  return result;
   Error2("dup2(%d): %s", oldfd, strerror(errno));
   return result;
}

/* like dup2(), but prints error on failure and returns 0 on success */
static int xiodup2(int oldfd, int newfd) {
   int result;
   if ((result = Dup2(oldfd, newfd)) >= 0)  return 0;
   Error3("dup2(%d, %d): %s", oldfd, newfd, strerror(errno));
   return result;
}

/* move the filedescriptors from the old handles to the new handles.
   old -1 handles are ignored, new -1 handles are not closed.
   returns 0 on success, -1 if a dup error occurred, or 1 on a close error
*/
static int reassignfds(int oldfd1, int oldfd2, int newfd1, int newfd2) {
   int tmpfd;
   int result;

   Debug4("reassignfds(%d, %d, %d, %d)", oldfd1, oldfd2, newfd1, newfd2);
   if (oldfd1 == newfd1) {
      Fcntl_l(newfd1, F_SETFD, 0);
      oldfd1 = -1;
   }
   if (oldfd2 == newfd2) {
      Fcntl_l(newfd2, F_SETFD, 0);
      oldfd2 = -1;
   }

   if (oldfd1 < 0 && oldfd2 < 0)  return 0;

   if (oldfd2 < 0) {
      if ((result = xiodup2(oldfd1, newfd1)) < 0)  return result;
      if (newfd2 != oldfd1)  if (Close(oldfd1) < 0)  return 1;
      return 0;
   }

   if (oldfd1 < 0) {
      if ((result = xiodup2(oldfd2, newfd2)) < 0)  return result;
      if (oldfd2 >= 0)  if (Close(oldfd2) < 0)  return 1;
      return 0;
   }

   if (oldfd2 == newfd1) {
      if (oldfd1 == newfd2) {
	 /* exchange them */
	 if ((tmpfd  = xiodup(oldfd2)) < 0)  return tmpfd;
	 if ((result = xiodup2(oldfd1, newfd1)) < 0)  return result;
	 if ((result = xiodup2(tmpfd,  newfd2)) < 0)  return result;
	 if (Close(tmpfd) < 0)  return 1;
      } else {
	 if ((result = xiodup2(oldfd2, newfd2)) < 0)  return result;
	 if ((result = xiodup2(oldfd1, newfd1)) < 0)  return result;
	 if (Close(oldfd1) < 0)  return 1;
      }
   } else {
      if (oldfd1 == newfd2) {
	 if ((result = xiodup2(oldfd1, newfd1)) < 0)  return result;
	 if ((result = xiodup2(oldfd2, newfd2)) < 0)  return result;
	 if (Close(oldfd2) < 0)  return 1;
      } else {
	 if ((result = xiodup2(oldfd1, newfd1)) < 0)  return result;
	 if ((result = xiodup2(oldfd2, newfd2)) < 0)  return result;
	 if (Close(oldfd1) < 0 || Close(oldfd2) < 0)  return 1;
      }
   }
   return 0;
}
