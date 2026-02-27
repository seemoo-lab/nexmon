/* source: socat.c */
/* Copyright Gerhard Rieger */
/* Published under the GNU General Public License V.2, see file COPYING */

/* this is the main source, including command line option parsing and general
   control */

#include "config.h"
#include "xioconfig.h"	/* what features are enabled */

#include "sysincludes.h"

#include "mytypes.h"
#include "compat.h"
#include "error.h"

#include "sycls.h"
#include "sysutils.h"
#include "dalan.h"
#include "filan.h"
#include "xio.h"
#include "xioopts.h"
#include "xiosigchld.h"
#include "xiolockfile.h"

#include "xioopen.h"

/* command line options */
struct {
   bool strictopts;	/* stop on errors in address options */
   xiolock_t lock;	/* a lock file */
} socat_opts = {
   0,		/* strictopts */
   { NULL, 0 },	/* lock */
};

void socat_usage(FILE *fd);
void socat_version(FILE *fd);
int socat(int argc, const char *address1, const char *address2);
int _socat(xiofile_t *xfd1, xiofile_t *xfd2);
void socat_signal(int sig);

void lftocrlf(char **in, ssize_t *len, size_t bufsiz);
void crlftolf(char **in, ssize_t *len, size_t bufsiz);

static int socat_lock(void);
static void socat_unlock(void);
static int socat_newchild(void);

static const char socatversion[] =
#include "./VERSION"
      ;
static const char timestamp[] = BUILD_DATE;

const char copyright_socat[] = "socat by Gerhard Rieger - see www.dest-unreach.org";
#if WITH_OPENSSL
const char copyright_openssl[] = "This product includes software developed by the OpenSSL Project for use in the OpenSSL Toolkit. (http://www.openssl.org/)";
const char copyright_ssleay[] = "This product includes software written by Tim Hudson (tjh@cryptsoft.com)";
#endif

bool havelock;


int main(int argc, const char *argv[]) {
   const char **arg1, *a;
   char *mainwaitstring;
   char buff[10];
   double rto;
   int i, argc0, result;
   struct utsname ubuf;
   int lockrc;

   if (mainwaitstring = getenv("SOCAT_MAIN_WAIT")) {
       sleep(atoi(mainwaitstring));
   }
   diag_set('p', strchr(argv[0], '/') ? strrchr(argv[0], '/')+1 : argv[0]);

   /* we must init before applying options because env settings have lower
      priority and are to be overridden by options */
   if (xioinitialize(XIO_MAYALL) != 0) {
      Exit(1);
   }

   xiosetopt('p', "%");
   xiosetopt('o', ":");

   argc0 = argc;	/* save for later use */
   arg1 = argv+1;  --argc;
   while (arg1[0] && (arg1[0][0] == '-')) {
      switch (arg1[0][1]) {
      case 'V': socat_version(stdout); Exit(0);
#if WITH_HELP
      case '?':
      case 'h':
	 socat_usage(stdout);
	 xioopenhelp(stdout, (arg1[0][2]=='?'||arg1[0][2]=='h') ? (arg1[0][3]=='?'||arg1[0][3]=='h') ? 2 : 1 : 0);
	 Exit(0);
#endif /* WITH_HELP */
      case 'd': diag_set('d', NULL); break;
#if WITH_FILAN
      case 'D': xioparams->debug = true; break;
#endif
      case 'l':
	 switch (arg1[0][2]) {
	 case 'm': /* mixed mode: stderr, then switch to syslog; + facility */
	    diag_set('s', NULL);
	    xiosetopt('l', "m");
	    xioparams->logopt = arg1[0][2];
	    xiosetopt('y', &arg1[0][3]);
	    break;
	 case 'y': /* syslog + facility */
	    diag_set(arg1[0][2], &arg1[0][3]);
	    break;
	 case 'f': /* to file, +filename */
	 case 'p': /* artificial program name */
	    if (arg1[0][3]) {
	       diag_set(arg1[0][2], &arg1[0][3]);
	    } else if (arg1[1]) {
	       diag_set(arg1[0][2], arg1[1]);
	       ++arg1, --argc;
	    } else {
	       Error1("option -l%c requires an argument; use option \"-h\" for help", arg1[0][2]);
	    }
	    break;
	 case 's': /* stderr */
	    diag_set(arg1[0][2], NULL);
	    break;
	 case 'u':
	    diag_set('u', NULL);
	    break;
	 case 'h':
	    diag_set_int('h', true);
	    break;
	 default:
	    Error1("unknown log option \"%s\"; use option \"-h\" for help", arg1[0]);
	    break;
	 }
	 break;
      case 'v': xioparams->verbose = true; break;
      case 'x': xioparams->verbhex = true; break;
      case 'b': if (arg1[0][2]) {
	    a = *arg1+2;
	 } else {
	    ++arg1, --argc;
	    if ((a = *arg1) == NULL) {
	       Error("option -b requires an argument; use option \"-h\" for help");
	       Exit(1);
	    }
	 }
	 xioparams->bufsiz = strtoul(a, (char **)&a, 0);
	 break;
      case 's':
	 diag_set_int('e', E_FATAL); break;
      case 't': if (arg1[0][2]) {
	    a = *arg1+2;
	 } else {
	    ++arg1, --argc;
	    if ((a = *arg1) == NULL) {
	       Error("option -t requires an argument; use option \"-h\" for help");
	       Exit(1);
	    }
	 }
	 rto = strtod(a, (char **)&a);
	 xioparams->closwait.tv_sec = rto;
	 xioparams->closwait.tv_usec =
	    (rto-xioparams->closwait.tv_sec) * 1000000; 
	 break;
      case 'T':  if (arg1[0][2]) {
	    a = *arg1+2;
	 } else {
	    ++arg1, --argc;
	    if ((a = *arg1) == NULL) {
	       Error("option -T requires an argument; use option \"-h\" for help");
	       Exit(1);
	    }
	 }
	 rto = strtod(a, (char **)&a);
	 xioparams->total_timeout.tv_sec = rto;
	 xioparams->total_timeout.tv_usec =
	    (rto-xioparams->total_timeout.tv_sec) * 1000000; 
	 break;
      case 'u': xioparams->lefttoright = true; break;
      case 'U': xioparams->righttoleft = true; break;
      case 'g': xioopts_ignoregroups = true; break;
      case 'L': if (socat_opts.lock.lockfile)
	     Error("only one -L and -W option allowed");
	 if (arg1[0][2]) {
	    socat_opts.lock.lockfile = *arg1+2;
	 } else {
	    ++arg1, --argc;
	    if ((socat_opts.lock.lockfile = *arg1) == NULL) {
	       Error("option -L requires an argument; use option \"-h\" for help");
	       Exit(1);
	    }
	 }
	 break;
      case 'W': if (socat_opts.lock.lockfile)
	    Error("only one -L and -W option allowed");
	 if (arg1[0][2]) {
	    socat_opts.lock.lockfile = *arg1+2;
	 } else {
	    ++arg1, --argc;
	    if ((socat_opts.lock.lockfile = *arg1) == NULL) {
	       Error("option -W requires an argument; use option \"-h\" for help");
	       Exit(1);
	    }
	 }
	 socat_opts.lock.waitlock = true;
	 socat_opts.lock.intervall.tv_sec  = 1;
	 socat_opts.lock.intervall.tv_nsec = 0;
	 break;
#if WITH_IP4 || WITH_IP6
#if WITH_IP4
      case '4':
#endif
#if WITH_IP6
      case '6':
#endif
	 xioopts.default_ip = arg1[0][1];
	 xioopts.preferred_ip = arg1[0][1];
	 break;
#endif /* WITH_IP4 || WITH_IP6 */
      case 'c':
	 switch (arg1[0][2]) {
	 case 'S': xioparams->pipetype = XIOCOMM_SOCKETPAIRS; break;
	 case 'P':
	 case 'p': xioparams->pipetype = XIOCOMM_PIPES;       break;
	 case 's': xioparams->pipetype = XIOCOMM_SOCKETPAIR;  break;
	 case 'Y': xioparams->pipetype = XIOCOMM_PTYS;        break;
	 case 'y': xioparams->pipetype = XIOCOMM_PTY;         break;
	 case 't': xioparams->pipetype = XIOCOMM_TCP;         break;
	 case '0': case '1': case '2': case '3': case '4':
	 case '5': case '6': case '7': case '8': case '9':
	    xioparams->pipetype = atoi(&arg1[0][2]); break;
	 default: Error1("bad chain communication type \"%s\"", &arg1[0][2]);
	 }
	 break;
      case '\0':
      case '-':	/*! this is hardcoded "--" */
      case ',':
      case ':': break;	/* this "-" is a variation of STDIO or -- */
      default:
	 xioinqopt('p', buff, sizeof(buff));
	 if (arg1[0][1] == buff[0]) {
	    break;
	 }
	 Error1("unknown option \"%s\"; use option \"-h\" for help", arg1[0]);
	 Exit(1);
      }
      /* the leading "-" might be a form of the first address */
      xioinqopt('p', buff, sizeof(buff));
      if (arg1[0][0] == '-' &&
	  (arg1[0][1] == '\0' || arg1[0][1] == ':' ||
	   arg1[0][1] == ',' || arg1[0][1] == '-'/*!*/ ||
	   arg1[0][1] == buff[0]))
	 break;
      ++arg1; --argc;
   }
#if 0
   Info1("%d address arguments", argc);
#else
   if (argc != 2) {
      Error1("exactly 2 addresses required (there are %d); use option \"-h\" for help", argc);
      Exit(1);
   }
#endif
   if (xioparams->lefttoright && xioparams->righttoleft) {
      Error("-U and -u must not be combined");
   }

   xioinitialize2();
   Info(copyright_socat);
#if WITH_OPENSSL
   Info(copyright_openssl);
   Info(copyright_ssleay);
#endif
   Debug2("socat version %s on %s", socatversion, timestamp);
   xiosetenv("VERSION", socatversion, 1, NULL);	/* SOCAT_VERSION */
   uname(&ubuf);	/* ! here we circumvent internal tracing (Uname) */
   Debug4("running on %s version %s, release %s, machine %s\n",
	   ubuf.sysname, ubuf.version, ubuf.release, ubuf.machine);

#if WITH_MSGLEVEL <= E_DEBUG
   for (i = 0; i < argc0; ++i) {
      Debug2("argv[%d]: \"%s\"", i, argv[i]);
   }
#endif /* WITH_MSGLEVEL <= E_DEBUG */

   {
      struct sigaction act;
      sigfillset(&act.sa_mask);
      act.sa_flags = 0;
      act.sa_handler = socat_signal;
      /* not sure which signals should be cauhgt and print a message */
      Sigaction(SIGHUP,  &act, NULL);
      Sigaction(SIGINT,  &act, NULL);
      Sigaction(SIGQUIT, &act, NULL);
      Sigaction(SIGILL,  &act, NULL);
      Sigaction(SIGABRT, &act, NULL);
      Sigaction(SIGBUS,  &act, NULL);
      Sigaction(SIGFPE,  &act, NULL);
      Sigaction(SIGSEGV, &act, NULL);
      Sigaction(SIGTERM, &act, NULL);
   }
   Signal(SIGPIPE, SIG_IGN);
#if HAVE_SIGACTION
   {
      struct sigaction act;
      memset(&act, 0, sizeof(struct sigaction));
      act.sa_flags   = SA_NOCLDSTOP|SA_RESTART|SA_SIGINFO
#ifdef SA_NOMASK
	 |SA_NOMASK
#endif
	 ;
      act.sa_sigaction = xiosigaction_subaddr_ok;
      if (Sigaction(SIGUSR1, &act, NULL) < 0) {
	 /*! Linux man does not explicitely say that errno is defined */
	 Warn1("sigaction(SIGUSR1, {&xiosigaction_subaddr_ok}, NULL): %s", strerror(errno));
      }
   }
#else /* !HAVE_SIGACTION */
   if (Signal(SIGUSR1, xiosigaction_subaddr_ok) == SIG_ERR) {
      Warn1("signal(SIGCHLD, xiosigaction_subaddr_ok): %s", strerror(errno));
   }
#endif /* !HAVE_SIGACTION */

   /* set xio hooks */
   xiohook_newchild = &socat_newchild;

   if (lockrc = socat_lock()) {
      /* =0: goon; >0: locked; <0: error, printed in sub */
      if (lockrc > 0)
	 Error1("could not obtain lock \"%s\"", socat_opts.lock.lockfile);
      Exit(1);
   }

   Atexit(socat_unlock);

   result = socat(argc, arg1[0], arg1[1]);
   Notice1("exiting with status %d", result);
   Exit(result);
   return 0;	/* not reached, just for gcc -Wall */
}


void socat_usage(FILE *fd) {
   fputs(copyright_socat, fd); fputc('\n', fd);
   fputs("Usage:\n", fd);
   fputs("socat [options] <bi-address> <bi-address>\n", fd);
   fputs("   options:\n", fd);
   fputs("      -V     print version and feature information to stdout, and exit\n", fd);
#if WITH_HELP
   fputs("      -h|-?  print a help text describing command line options and addresses\n", fd);
   fputs("      -hh    like -h, plus a list of all common address option names\n", fd);
   fputs("      -hhh   like -hh, plus a list of all available address option names\n", fd);
#endif /* WITH_HELP */
   fputs("      -d     increase verbosity (use up to 4 times; 2 are recommended)\n", fd);
#if WITH_FILAN
   fputs("      -D     analyze file descriptors before loop\n", fd);
#endif
   fputs("      -ly[facility]  log to syslog, using facility (default is daemon)\n", fd);
   fputs("      -lf<logfile>   log to file\n", fd);
   fputs("      -ls            log to stderr (default if no other log)\n", fd);
   fputs("      -lm[facility]  mixed log mode (stderr during initialization, then syslog)\n", fd);
   fputs("      -lp<progname>  set the program name used for logging\n", fd);
   fputs("      -lu            use microseconds for logging timestamps\n", fd);
   fputs("      -lh            add hostname to log messages\n", fd);
   fputs("      -v     verbose data traffic, text\n", fd);
   fputs("      -x     verbose data traffic, hexadecimal\n", fd);
   fputs("      -b<size_t>     set data buffer size (8192)\n", fd);
   fputs("      -s     sloppy (continue on error)\n", fd);
   fputs("      -t<timeout>    wait seconds before closing second channel\n", fd);
   fputs("      -T<timeout>    total inactivity timeout in seconds\n", fd);
   fputs("      -u     unidirectional mode (left to right)\n", fd);
   fputs("      -U     unidirectional mode (right to left)\n", fd);
   fputs("      -g     do not check option groups\n", fd);
   fputs("      -L <lockfile>  try to obtain lock, or fail\n", fd);
   fputs("      -W <lockfile>  try to obtain lock, or wait\n", fd);
#if WITH_IP4
   fputs("      -4     prefer IPv4 if version is not explicitly specified\n", fd);
#endif
#if WITH_IP6
   fputs("      -6     prefer IPv6 if version is not explicitly specified\n", fd);
#endif
}


void socat_version(FILE *fd) {
   struct utsname ubuf;

   fputs(copyright_socat, fd); fputc('\n', fd);
   fprintf(fd, "socat version %s on %s\n", socatversion, timestamp);
   Uname(&ubuf);
   fprintf(fd, "   running on %s version %s, release %s, machine %s\n",
	   ubuf.sysname, ubuf.version, ubuf.release, ubuf.machine);
   fputs("features:\n", fd);
#ifdef WITH_STDIO
   fprintf(fd, "  #define WITH_STDIO %d\n", WITH_STDIO);
#else
   fputs("  #undef WITH_STDIO\n", fd);
#endif
#ifdef WITH_FDNUM
   fprintf(fd, "  #define WITH_FDNUM %d\n", WITH_FDNUM);
#else
   fputs("  #undef WITH_FDNUM\n", fd);
#endif
#ifdef WITH_FILE
   fprintf(fd, "  #define WITH_FILE %d\n", WITH_FILE);
#else
   fputs("  #undef WITH_FILE\n", fd);
#endif
#ifdef WITH_CREAT
   fprintf(fd, "  #define WITH_CREAT %d\n", WITH_CREAT);
#else
   fputs("  #undef WITH_CREAT\n", fd);
#endif
#ifdef WITH_GOPEN
   fprintf(fd, "  #define WITH_GOPEN %d\n", WITH_GOPEN);
#else
   fputs("  #undef WITH_GOPEN\n", fd);
#endif
#ifdef WITH_TERMIOS
   fprintf(fd, "  #define WITH_TERMIOS %d\n", WITH_TERMIOS);
#else
   fputs("  #undef WITH_TERMIOS\n", fd);
#endif
#ifdef WITH_PIPE
   fprintf(fd, "  #define WITH_PIPE %d\n", WITH_PIPE);
#else
   fputs("  #undef WITH_PIPE\n", fd);
#endif
#ifdef WITH_UNIX
   fprintf(fd, "  #define WITH_UNIX %d\n", WITH_UNIX);
#else
   fputs("  #undef WITH_UNIX\n", fd);
#endif /* WITH_UNIX */
#ifdef WITH_ABSTRACT_UNIXSOCKET
   fprintf(fd, "  #define WITH_ABSTRACT_UNIXSOCKET %d\n", WITH_ABSTRACT_UNIXSOCKET);
#else
   fputs("  #undef WITH_ABSTRACT_UNIXSOCKET\n", fd);
#endif /* WITH_ABSTRACT_UNIXSOCKET */
#ifdef WITH_IP4
   fprintf(fd, "  #define WITH_IP4 %d\n", WITH_IP4);
#else
   fputs("  #undef WITH_IP4\n", fd);
#endif
#ifdef WITH_IP6
   fprintf(fd, "  #define WITH_IP6 %d\n", WITH_IP6);
#else
   fputs("  #undef WITH_IP6\n", fd);
#endif
#ifdef WITH_RAWIP
   fprintf(fd, "  #define WITH_RAWIP %d\n", WITH_RAWIP);
#else
   fputs("  #undef WITH_RAWIP\n", fd);
#endif
#ifdef WITH_GENERICSOCKET
   fprintf(fd, "  #define WITH_GENERICSOCKET %d\n", WITH_GENERICSOCKET);
#else
   fputs("  #undef WITH_GENERICSOCKET\n", fd);
#endif
#ifdef WITH_INTERFACE
   fprintf(fd, "  #define WITH_INTERFACE %d\n", WITH_INTERFACE);
#else
   fputs("  #undef WITH_INTERFACE\n", fd);
#endif
#ifdef WITH_TCP
   fprintf(fd, "  #define WITH_TCP %d\n", WITH_TCP);
#else
   fputs("  #undef WITH_TCP\n", fd);
#endif
#ifdef WITH_UDP
   fprintf(fd, "  #define WITH_UDP %d\n", WITH_UDP);
#else
   fputs("  #undef WITH_UDP\n", fd);
#endif
#ifdef WITH_SCTP
   fprintf(fd, "  #define WITH_SCTP %d\n", WITH_SCTP);
#else
   fputs("  #undef WITH_SCTP\n", fd);
#endif
#ifdef WITH_LISTEN
   fprintf(fd, "  #define WITH_LISTEN %d\n", WITH_LISTEN);
#else
   fputs("  #undef WITH_LISTEN\n", fd);
#endif
#ifdef WITH_SOCKS4
   fprintf(fd, "  #define WITH_SOCKS4 %d\n", WITH_SOCKS4);
#else
   fputs("  #undef WITH_SOCKS4\n", fd);
#endif
#ifdef WITH_SOCKS4A
   fprintf(fd, "  #define WITH_SOCKS4A %d\n", WITH_SOCKS4A);
#else
   fputs("  #undef WITH_SOCKS4A\n", fd);
#endif
#ifdef WITH_PROXY
   fprintf(fd, "  #define WITH_PROXY %d\n", WITH_PROXY);
#else
   fputs("  #undef WITH_PROXY\n", fd);
#endif
#ifdef WITH_SYSTEM
   fprintf(fd, "  #define WITH_SYSTEM %d\n", WITH_SYSTEM);
#else
   fputs("  #undef WITH_SYSTEM\n", fd);
#endif
#ifdef WITH_EXEC
   fprintf(fd, "  #define WITH_EXEC %d\n", WITH_EXEC);
#else
   fputs("  #undef WITH_EXEC\n", fd);
#endif
#ifdef WITH_READLINE
   fprintf(fd, "  #define WITH_READLINE %d\n", WITH_READLINE);
#else
   fputs("  #undef WITH_READLINE\n", fd);
#endif
#ifdef WITH_TUN
   fprintf(fd, "  #define WITH_TUN %d\n", WITH_TUN);
#else
   fputs("  #undef WITH_TUN\n", fd);
#endif
#ifdef WITH_PTY
   fprintf(fd, "  #define WITH_PTY %d\n", WITH_PTY);
#else
   fputs("  #undef WITH_PTY\n", fd);
#endif
#ifdef WITH_OPENSSL
   fprintf(fd, "  #define WITH_OPENSSL %d\n", WITH_OPENSSL);
#else
   fputs("  #undef WITH_OPENSSL\n", fd);
#endif
#ifdef WITH_FIPS
   fprintf(fd, "  #define WITH_FIPS %d\n", WITH_FIPS);
#else
   fputs("  #undef WITH_FIPS\n", fd);
#endif
#ifdef WITH_LIBWRAP
   fprintf(fd, "  #define WITH_LIBWRAP %d\n", WITH_LIBWRAP);
#else
   fputs("  #undef WITH_LIBWRAP\n", fd);
#endif
#ifdef WITH_SYCLS
   fprintf(fd, "  #define WITH_SYCLS %d\n", WITH_SYCLS);
#else
   fputs("  #undef WITH_SYCLS\n", fd);
#endif
#ifdef WITH_FILAN
   fprintf(fd, "  #define WITH_FILAN %d\n", WITH_FILAN);
#else
   fputs("  #undef WITH_FILAN\n", fd);
#endif
#ifdef WITH_RETRY
   fprintf(fd, "  #define WITH_RETRY %d\n", WITH_RETRY);
#else
   fputs("  #undef WITH_RETRY\n", fd);
#endif
#ifdef WITH_MSGLEVEL
   fprintf(fd, "  #define WITH_MSGLEVEL %d /*%s*/\n", WITH_MSGLEVEL,
	   &"debug\0\0\0info\0\0\0\0notice\0\0warn\0\0\0\0error\0\0\0fatal\0\0\0"[WITH_MSGLEVEL<<3]);
#else
   fputs("  #undef WITH_MSGLEVEL\n", fd);
#endif
}


/* call this function when the common command line options are parsed, and the
   addresses are extracted (but not resolved). */
int socat(int argc, const char *address1, const char *address2) {
   xiofile_t *xfd1, *xfd2;

   xioinitialize(XIO_MAYALL);

   /* open the first (left most) address */
   if (xioparams->lefttoright) {
      if ((xfd1 = socat_open(address1, XIO_RDONLY, XIO_MAYFORK|XIO_MAYCHILD|XIO_MAYCONVERT)) == NULL) {
	 return -1;
      }
   } else if (xioparams->righttoleft) {
      if ((xfd1 = socat_open(address1, XIO_WRONLY, XIO_MAYFORK|XIO_MAYCHILD|XIO_MAYCONVERT)) == NULL) {
	 return -1;
      }
   } else {
      if ((xfd1 = socat_open(address1, XIO_RDWR, XIO_MAYFORK|XIO_MAYCHILD|XIO_MAYCONVERT)) == NULL) {
	 return -1;
      }
   }
   xiosetsigchild(xfd1, socat_sigchild);
#if 1	/*! */
   if (XIO_RDSTREAM(xfd1)->subaddrstat < 0) {
      Info1("child "F_pid" has already died",
	    XIO_RDSTREAM(xfd1)->child.pid);
      XIO_RDSTREAM(xfd1)->child.pid = 0;
      /* return STAT_RETRYLATER; */
   }
#endif

   /* second (right) addresses chain */
   if (XIO_WRITABLE(xfd1)) {
      if (XIO_READABLE(xfd1)) {
	 if ((xfd2 = socat_open(address2, XIO_RDWR, XIO_MAYFORK|XIO_MAYCHILD|XIO_MAYEXEC|XIO_MAYCONVERT)) == NULL) {
	    return -1;
	 }
      } else {
	 if ((xfd2 = socat_open(address2, XIO_RDONLY, XIO_MAYFORK|XIO_MAYCHILD|XIO_MAYEXEC|XIO_MAYCONVERT)) == NULL) {
	    return -1;
	 }
      }
   } else {	/* assuming sock1 is readable */
      if ((xfd2 = socat_open(address2, XIO_WRONLY, XIO_MAYFORK|XIO_MAYCHILD|XIO_MAYEXEC|XIO_MAYCONVERT)) == NULL) {
	 return -1;
      }
   }
   xiosetsigchild(xfd2, socat_sigchild);
#if 1	/*! */
   if (XIO_RDSTREAM(xfd2)->subaddrstat < 0) {
      Info1("child "F_pid" has already died",
	    XIO_RDSTREAM(xfd2)->child.pid);
      XIO_RDSTREAM(xfd2)->child.pid = 0;
      /* return STAT_RETRYLATER; */
   }
#endif

   Info("resolved and opened all sock addresses");
   return 
      _socat(xfd1, xfd2);
}


/* childleftdata() has been moved to xioengine.c */


/* _socat() has been moved to xioengine.c */


/* gettimestamp() has been moved to xiotransfer.c */


/* xioprintblockheader has been moved to xiotransfer.c */


/* xiotransfer has been moved to xiotransfer.c */


/* cv_newline has been moved to xiotransfer.c */


void socat_signal(int signum) {
   int _errno;
   _errno = errno;
   diag_in_handler = 1;
   Notice1("socat_signal(): handling signal %d", signum);
   switch (signum) {
   case SIGQUIT:
   case SIGILL:
   case SIGABRT:
   case SIGBUS:
   case SIGFPE:
   case SIGSEGV:
   case SIGPIPE:
      diag_set_int('x', 128+signum);	/* in case Error exits for us */
      Error1("exiting on signal %d", signum);
      diag_set_int('x', 0);	/* in case Error did not exit */
      break;
   case SIGTERM:
      Warn1("exiting on signal %d", signum); break;
   case SIGHUP:  
   case SIGINT:
      Notice1("exiting on signal %d", signum); break;
   }
   //Exit(128+signum);	/* internal cleanup + _exit() */
   Notice1("socat_signal(): finishing signal %d", signum);
   diag_exit(128+signum);    /*!!! internal cleanup + _exit() */
   diag_in_handler = 0;
   errno = _errno;
}

#if 0
/* this is the callback when the child of an address died */
static int socat_sigchild(struct single *file) {
   if (file->ignoreeof && !closing) {
      ;
   } else {
      file->eof = MAX(file->eof, 1);
      closing = 1;
   }
   return 0;
}
#endif

static int socat_lock(void) {
   int lockrc;

#if 1
   if ((lockrc = xiolock(&socat_opts.lock)) < 0) {
      return -1;
   }
   if (lockrc == 0) {
      havelock = true;
   }
   return lockrc;
#else
   if (socat_opts.lock.lockfile) {
      if ((lockrc = xiolock(socat_opts.lock.lockfile)) < 0) {
	 /*Error1("error with lockfile \"%s\"", socat_opts.lock.lockfile);*/
	 return -1;
      }
      if (lockrc) {
	 return 1;
      }
      havelock = true;
      /*0 Info1("obtained lock \"%s\"", socat_opts.lock.lockfile);*/
   }

   if (socat_opts.lock.waitlock) {
      if (xiowaitlock(socat_opts.lock.waitlock, socat_opts.lock.intervall)) {
	 /*Error1("error with lockfile \"%s\"", socat_opts.lock.lockfile);*/
	 return -1;
      } else {
	 havelock = true;
	 /*0 Info1("obtained lock \"%s\"", socat_opts.lock.waitlock);*/
      }
   }
   return 0;
#endif
}

static void socat_unlock(void) {
   if (!havelock)  return;
   if (socat_opts.lock.lockfile) {
      if (Unlink(socat_opts.lock.lockfile) < 0) {
	 if (!diag_in_handler) {
	    Warn2("unlink(\"%s\"): %s",
		  socat_opts.lock.lockfile, strerror(errno));
	 } else {
	    Warn1("unlink(\"%s\"): "F_strerror,
		  socat_opts.lock.lockfile);
	 }
      } else {
	 Info1("released lock \"%s\"", socat_opts.lock.lockfile);
      }
   }
}

/* this is a callback function that may be called by the newchild hook of xio
 */
static int socat_newchild(void) {
   havelock = false;
   return 0;
}
