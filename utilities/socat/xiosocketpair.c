/* xiosocketpair.c */
/* Copyright Gerhard Rieger */
/* Published under the GNU General Public License V.2, see file COPYING */

/* this is the source of the internal xiosocketpair function */

#include "xiosysincludes.h"
#include "sycls.h"
#include "compat.h"
#include "error.h"
#include "xio.h"


#if defined(HAVE_DEV_PTMX)
#  define PTMX "/dev/ptmx"	/* Linux */
#elif HAVE_DEV_PTC
#  define PTMX "/dev/ptc"	/* AIX */
#endif

#define MAXPTYNAMELEN 64

int xiopty(int useptmx, int *ttyfdp, int *ptyfdp) {
   int ttyfd, ptyfd = -1;
   char ptyname[MAXPTYNAMELEN];
   struct termios termarg;

   if (useptmx) {
      if ((ptyfd = Open(PTMX, O_RDWR|O_NOCTTY, 0620)) < 0) {
	 Warn1("open(\""PTMX"\", O_RDWR|O_NOCTTY, 0620): %s",
	       strerror(errno));
	 /*!*/
      } else {
	 ;/*0 Info1("open(\""PTMX"\", O_RDWR|O_NOCTTY, 0620) -> %d", ptyfd);*/
      }
      if (ptyfd >= 0) {
	 char *tn = NULL;

	 /* we used PTMX before forking */
	 /*0 extern char *ptsname(int);*/
#if HAVE_GRANTPT	/* AIX, not Linux */
	 if (Grantpt(ptyfd)/*!*/ < 0) {
	    Warn2("grantpt(%d): %s", ptyfd, strerror(errno));
	 }
#endif /* HAVE_GRANTPT */
#if HAVE_UNLOCKPT
	 if (Unlockpt(ptyfd)/*!*/ < 0) {
	    Warn2("unlockpt(%d): %s", ptyfd, strerror(errno));
	 }
#endif /* HAVE_UNLOCKPT */
#if HAVE_PROTOTYPE_LIB_ptsname	/* AIX, not Linux */
	 if ((tn = Ptsname(ptyfd)) == NULL) {
	    Warn2("ptsname(%d): %s", ptyfd, strerror(errno));
	 } else {
	    Notice1("PTY is %s", tn);
	 }
#endif /* HAVE_PROTOTYPE_LIB_ptsname */
#if 0
	 if (tn == NULL) {
	    /*! ttyname_r() */
	    if ((tn = Ttyname(ptyfd)) == NULL) {
	       Warn2("ttyname(%d): %s", ptyfd, strerror(errno));
	    }
	 }
	 ptyname[0] = '\0'; strncat(ptyname, tn, MAXPTYNAMELEN-1);
#endif
	 if ((ttyfd = Open(tn, O_RDWR|O_NOCTTY, 0620)) < 0) {
	    Warn2("open(\"%s\", O_RDWR|O_NOCTTY, 0620): %s", tn, strerror(errno));
	 } else {
	    /*0 Info2("open(\"%s\", O_RDWR|O_NOCTTY, 0620) -> %d", tn, ttyfd);*/
	 }

#ifdef I_PUSH
	 /* Linux: I_PUSH def'd; pty: ioctl(, I_FIND, ...) -> -1 EINVAL */
	 /* AIX:   I_PUSH def'd; pty: ioctl(, I_FIND, ...) -> 1 */
	 /* SunOS: I_PUSH def'd; pty: ioctl(, I_FIND, ...) -> 0 */
	 /* HP-UX: I_PUSH def'd; pty: ioctl(, I_FIND, ...) -> 0 */
	 if (Ioctl(ttyfd, I_FIND, "ldterm") == 0) {
	    Ioctl(ttyfd, I_PUSH, "ptem");		/* 0 */
	    Ioctl(ttyfd, I_PUSH, "ldterm");		/* 0 */
	    Ioctl(ttyfd, I_PUSH, "ttcompat");	/* HP-UX: -1 */
	 }
#endif
      }
   }
#if HAVE_OPENPTY
   if (ptyfd < 0) {
      int result;
      if ((result = Openpty(&ptyfd, &ttyfd, ptyname, NULL, NULL)) < 0) {
	 Error4("openpty(%p, %p, %p, NULL, NULL): %s",
		&ptyfd, &ttyfd, ptyname, strerror(errno));
	 return -1;
      }
      Notice1("PTY is %s", ptyname);
   }
#endif /* HAVE_OPENPTY */

   if (Tcgetattr(ttyfd, &termarg) < 0) {
      Error3("tcgetattr(%d, %p): %s",
	     ttyfd, &termarg, strerror(errno));
   }
#if 0
   cfmakeraw(&termarg);
#else
   /*!!! share code with xioopts.c raw,echo=0 */
   termarg.c_iflag &=
      ~(IGNBRK|BRKINT|IGNPAR|PARMRK|INPCK|ISTRIP|INLCR|IGNCR|ICRNL|IXON|IXOFF
#ifdef IUCLC
	|IUCLC
#endif
	|IXANY|IMAXBEL);
   termarg.c_iflag |= (0);
   termarg.c_oflag &= ~(OPOST);
   termarg.c_oflag |= (0);
   termarg.c_cflag &= ~(0);
   termarg.c_cflag |= (0);
   termarg.c_lflag &= ~(ECHO|ECHONL|ISIG|ICANON
#ifdef XCASE
			|XCASE
#endif
			);
   termarg.c_lflag |= (0);
   termarg.c_cc[VMIN] = 1;
   termarg.c_cc[VTIME] = 0;
#endif
   if (Tcsetattr(ttyfd, TCSADRAIN, &termarg) < 0) {
      Error3("tcsetattr(%d, TCSADRAIN, %p): %s",
	     ttyfd, &termarg, strerror(errno));
   }

   *ttyfdp = ttyfd;
   *ptyfdp = ptyfd;
   return 0;
}

/* generates a socket pair; supports not only PF_UNIX but also PF_INET */
int xiosocketpair2(int pf, int socktype, int protocol, int sv[2]) {
   int result;

   switch (pf) {
      struct sockaddr_in ssin, csin, xsin;	/* server, client, compare */
      socklen_t cslen, xslen;
      int sconn, slist, sserv;	/* socket FDs */

   case PF_UNIX:
      result = Socketpair(pf, socktype, protocol, sv);
      if (result < 0) {
	 Error5("socketpair(%d, %d, %d, %p): %s",
		pf, socktype, protocol, sv, strerror(errno));
	 return -1;
      }
      break;
#if LATER
   case PF_INET:
#if 1 /*!!! Linux */
      ssin.sin_family = pf;
      ssin.sin_port = htons(1024+random()%(65536-1024));
      ssin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
#endif /*  */
      if ((s = Socket(pf, socktype, protocol)) < 0) {
	 Error4("socket(%d, %d, %d): %s",
		pf, socktype, protocol, strerror(errno));
      }
      if (Bind(s, (struct sockaddr *)&ssin, sizeof(ssin)) < 0) {
	 Error6("bind(%d, {%u, 0x%x:%u}, "F_Zu"): %s",
		s, ssin.sin_family, ssin.sin_addr.s_addr, ssin.sin_port,
		sizeof(ssin), strerror(errno));
      }
      if (Connect(s, (struct sockaddr *)&ssin, sizeof(ssin)) < 0) {
	 Error6("connect(%d, {%u, 0x%x:%u}, "F_Zu"): %s",
		s, ssin.sin_family, ssin.sin_addr.s_addr, ssin.sin_port,
		sizeof(ssin), strerror(errno));
	 return -1;
      }
      break;
#endif /* LATER */
   case PF_INET:
      ssin.sin_family = pf;
      ssin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      if ((slist = Socket(pf, socktype, protocol)) < 0) {
	 Error4("socket(%d, %d, %d): %s",
		pf, socktype, protocol, strerror(errno));
      }
      while (true) {	/* find a port we can bind to */
	 ssin.sin_port = htons(1024+random()%(65536-1024));
	 if (Bind(slist, (struct sockaddr *)&ssin, sizeof(ssin)) == 0)  break;
	 if (errno == EADDRINUSE) {
	    Info6("bind(%d, {%u, 0x%x:%u}, "F_Zu"): %s",
		  slist, ssin.sin_family, ssin.sin_addr.s_addr,
		  ntohs(ssin.sin_port), sizeof(ssin), strerror(errno));
	    continue;
	 }
	 Error6("bind(%d, {%u, 0x%x:%u}, "F_Zu"): %s",
		slist, ssin.sin_family, ssin.sin_addr.s_addr, ssin.sin_port,
		sizeof(ssin), strerror(errno));
	 Close(slist);
	 return -1;
      }
      Listen(slist, 0);
      if ((sconn = Socket(pf, socktype, protocol)) < 0) {
	 Error4("socket(%d, %d, %d): %s",
		pf, socktype, protocol, strerror(errno));
	 Close(slist); return -1;
      }
      /* for testing race condition: Sleep(30); */
      if (Connect(sconn, (struct sockaddr *)&ssin, sizeof(ssin)) < 0) {
	 Error6("connect(%d, {%u, 0x%x:%u}, "F_Zu"): %s",
		sconn, ssin.sin_family, ssin.sin_addr.s_addr,
		ntohs(ssin.sin_port),
		sizeof(ssin), strerror(errno));
	 Close(slist); Close(sconn); return -1;
      }
      cslen = sizeof(csin);
      if (Getsockname(sconn, (struct sockaddr *)&csin, &cslen) < 0) {
	 Error4("getsockname(%d, %p, %p): %s",
		sconn, &csin, &cslen, strerror(errno));
	 Close(slist); Close(sconn); return -1;
      }
      do {
	 xslen = sizeof(xsin);
	 if ((sserv = Accept(slist, (struct sockaddr *)&xsin, &xslen)) < 0) {
	    Error4("accept(%d, %p, {"F_Zu"}): %s",
		   slist, &csin, sizeof(xslen), strerror(errno));
	    Close(slist); Close(sconn); return -1;
	 }
	 if (!memcmp(&csin, &xsin, cslen)) {
	    break;	/* expected connection */
	 }
	 Warn4("unexpected connection to 0x%lx:%hu from 0x%lx:%hu",
	       ntohl(ssin.sin_addr.s_addr), ntohs(ssin.sin_port),
	       ntohl(xsin.sin_addr.s_addr), ntohs(xsin.sin_port));
      } while (true);
      Close(slist);
      sv[0] = sconn;
      sv[1] = sserv;
      break;
   default:
      Error1("xiosocketpair2(): pf=%u not implemented", pf);
      return -1;
   }
   return 0;
}

/*
   dual should only be != 0 when both directions are used
   returns 0 on success
 */
int xiocommpair(int commtype, bool lefttoright, bool righttoleft,
		int dual, xiofd_t *left, xiofd_t *right, ...) {
   va_list ap;
   int domain = -1, socktype = -1, protocol = -1;
   int useptmx = 0;
   /* arrays can be used with pipe(2) and socketpair(2): */
   int svlr[2] = {-1, -1};	/* left to right: rfd, wfd */
   int svrl[2] = {-1, -1};	/* right to left: rfd, wfd */

   /* get related parameters from parameter list */
   switch (commtype) {
   case XIOCOMM_SOCKETPAIR:
   case XIOCOMM_SOCKETPAIRS:
      va_start(ap, right);
      domain   = va_arg(ap, int);
      socktype = va_arg(ap, int);
      protocol = va_arg(ap, int);
      va_end(ap);
      break;
   case XIOCOMM_PTY:
   case XIOCOMM_PTYS:
      va_start(ap, right);
      useptmx = va_arg(ap, int);
      va_end(ap);
      break;
   default:
      break;
   }

   switch (commtype) {
   default: /* unspec */
      Warn1("internal: undefined communication type %d, defaulting to 0",
	    commtype);
      commtype = 0;
      /*PASSTHROUGH*/
   case XIOCOMM_SOCKETPAIRS: /* two socketpairs - the default */
      if (lefttoright) {
	 if (Socketpair(domain, socktype, protocol, svlr) < 0) {
	    Error5("socketpair(%d, %d, %d, %p): %s",
		   domain, socktype, protocol, svlr, strerror(errno));
	 }
	 Shutdown(svlr[0], SHUT_WR);
      }	 
      if (righttoleft) {
	 if (Socketpair(domain, socktype, protocol, svrl) < 0) {
	    Error5("socketpair(%d, %d, %d, %p): %s",
		   domain, socktype, protocol, svrl, strerror(errno));
	 }
	 Shutdown(svrl[0], SHUT_WR);
      }
      left->single     = right->single     = false;
      left->dtype      = right->dtype      = XIODATA_STREAM;
      left->howtoshut  = right->howtoshut  = XIOSHUT_DOWN;
      left->howtoclose = right->howtoclose = XIOCLOSE_CLOSE;
      break;

   case XIOCOMM_PTYS: /* two ptys in raw mode, EOF in canonical mode */
      if (lefttoright) {
	 if (xiopty(useptmx, &svlr[0], &svlr[1]) < 0)  return -1;
	 /* pty is write side, interpretes ^D in canonical mode */
      }
      if (righttoleft) {
	 if (xiopty(useptmx, &svrl[0], &svrl[1]) < 0)  return -1;
      }
      left->single     = right->single     = false;
      left->dtype      = right->dtype      = XIODATA_PTY;
      left->howtoshut  = right->howtoshut  = XIOSHUT_PTYEOF;
      left->howtoclose = right->howtoclose = XIOCLOSE_CLOSE;
      break;

   case XIOCOMM_SOCKETPAIR: /* one socketpair */
      if (Socketpair(domain, socktype, protocol, svlr) < 0) {
	 Error5("socketpair(%d %d %d, %p): %s",
		domain, socktype, protocol, svlr, strerror(errno));
	 return -1;
      }
      left->single     = right->single     = true;
      left->dtype      = right->dtype      = XIODATA_STREAM;
      left->howtoshut  = right->howtoshut  = XIOSHUT_DOWN;
      left->howtoclose = right->howtoclose = XIOCLOSE_CLOSE;
      break;

   case XIOCOMM_PTY: /* one pty in raw mode, EOF in canonical mode */
      if (xiopty(useptmx, &svlr[0], &svlr[1]) < 0)  return -1;
      left->single     = right->single     = true;
      left->dtype      = right->dtype      = XIODATA_PTY;
      left->howtoshut  = XIOSHUT_PTYEOF;
      right->howtoshut = XIOSHUT_CLOSE;
      left->howtoclose = right->howtoclose = XIOCLOSE_CLOSE;
      break;

   case XIOCOMM_PIPES: /* two pipes */
      if (lefttoright) {
	 if (Pipe(svlr) < 0) {
	    Error2("pipe(%p): %s", svlr, strerror(errno));
	    return -1;
	 }
      }
      if (righttoleft) {
	 if (Pipe(svrl) < 0) {
	    Error2("pipe(%p): %s", svrl, strerror(errno));
	    return -1;
	 }
      }
      left->single     = right->single     = false;
      left->dtype      = right->dtype      = XIODATA_STREAM;
      left->howtoshut  = right->howtoshut  = XIOSHUT_CLOSE;
      left->howtoclose = right->howtoclose = XIOCLOSE_CLOSE;
      break;

   case XIOCOMM_TCP:
   case XIOCOMM_TCP4: /* one TCP/IPv4 socket pair */
      if (xiosocketpair2(PF_INET, SOCK_STREAM, 0, svlr) < 0) {
	 Error2("socketpair(PF_UNIX, PF_STREAM, 0, %p): %s",
		svlr, strerror(errno));
	 return -1;
      }
      left->single     = right->single     = true;
      left->dtype      = right->dtype      = XIODATA_STREAM;
      left->howtoshut  = right->howtoshut  = XIOSHUT_DOWN;
      left->howtoclose = right->howtoclose = XIOCLOSE_CLOSE;
      break;
   }

   if (dual && left->single) {
      /* one pair */
      /* dual; we use different FDs for the channels to avoid conflicts
	 (happened in dual exec) */
      if ((svrl[1] = Dup(svlr[0])) < 0) {
	 Error2("dup(%d): %s", svrl[0], strerror(errno));
	 return -1;
      }
      if ((svrl[0] = Dup(svlr[1])) < 0) {
	 Error2("dup(%d): %s", svlr[1], strerror(errno));
	 return -1;
      }
   } else if (left->single) {
      svrl[1] = svlr[0];
      svrl[0] = svlr[1];
   }

   /* usually they are not to be passed to exec'd child processes */
   if (lefttoright) {
      Fcntl_l(svlr[0], F_SETFD, 1);
      Fcntl_l(svlr[1], F_SETFD, 1);
   }
   if (righttoleft && (!left->single || dual)) {
      Fcntl_l(svrl[0], F_SETFD, 1);
      Fcntl_l(svrl[1], F_SETFD, 1);
   }

   left->rfd = svrl[0];
   left->wfd = svlr[1];
   right->rfd = svlr[0];
   right->wfd = svrl[1];
   Notice4("xiocommpair() -> [%d:%d], [%d:%d]",
	   left->rfd, left->wfd, right->rfd, right->wfd);
   return 0;
}

