/* source: xio-openssl.c */
/* Copyright Gerhard Rieger */
/* Published under the GNU General Public License V.2, see file COPYING */

/* this file contains the implementation of the openssl addresses */

#include "xiosysincludes.h"
#if WITH_OPENSSL	/* make this address configure dependend */
#include <openssl/conf.h>
#include <openssl/x509v3.h>

#include "xioopen.h"

#include "xio-fd.h"
#include "xio-socket.h"	/* _xioopen_connect() */
#include "xio-listen.h"
#include "xio-ipapp.h"
#include "xio-openssl.h"

/* the openssl library requires a file descriptor for external communications.
   so our best effort is to provide any possible kind of un*x file descriptor 
   (not only tcp, but also pipes, stdin, files...)
   for tcp we want to provide support for socks and proxy.
   read and write functions must use the openssl crypt versions.
   but currently only plain tcp4 is implemented.
*/

/* Linux: "man 3 ssl" */

/* generate a simple openssl server for testing:
   1) generate a private key
   openssl genrsa -out server.key 1024
   2) generate a self signed cert
   openssl req -new -key server.key -x509 -days 3653 -out server.crt
      enter fields...
   3) generate the pem file
   cat server.key server.crt >server.pem
   openssl s_server  (listens on 4433/tcp)
 */

/* static declaration of ssl's open function */
static int xioopen_openssl_connect(int argc, const char *argv[], struct opt *opts,
				   int xioflags, xiofile_t *fd, unsigned groups,
			   int dummy1, int dummy2, int dummy3);

/* static declaration of ssl's open function */
static int xioopen_openssl_listen(int argc, const char *argv[], struct opt *opts,
				  int xioflags, xiofile_t *fd, unsigned groups,
			   int dummy1, int dummy2, int dummy3);
static int openssl_SSL_ERROR_SSL(int level, const char *funcname);
static int openssl_handle_peer_certificate(struct single *xfd,
					   const char *peername,
					   bool opt_ver,
					   int level);
static int xioSSL_set_fd(struct single *xfd, int level);
static int xioSSL_connect(struct single *xfd, const char *opt_commonname, bool opt_ver, int level);
static int openssl_delete_cert_info(void);
 

/* description record for inter-address ssl connect with 0 parameters */
static const struct xioaddr_inter_desc xiointer_openssl_connect0 = {
   XIOADDR_INTER,	/* this is an embedded address (inter module) */
   "openssl-client",	/* keyword for selecting this address type in xioopen calls
		   (canonical or main name) */
   0,		/* number of required parameters */
   XIOBIT_ALL,	/* data flow directions this address supports on API layer:
		   XIOBIT_RDONLY|XIOBIT_WRONLY|XIOBIT_RDWR */
   GROUP_CHILD|GROUP_OPENSSL|GROUP_RETRY,	/* bitwise OR of address groups this address belongs to.
		   You might have to specify a new group in xioopts.h */
   XIOSHUT_OPENSSL,
   XIOCLOSE_CLOSE,
   xioopen_openssl_connect,	/* a function pointer used to "open" these addresses.*/
   0,		/* an integer passed to xioopen_openssl; makes it possible to
		   use the same xioopen_openssl function for slightly different
		   address types. */
   0,		/* like previous argument */
   0,		/* like previous arguments, but pointer type.
		   No trailing comma or semicolon! */
   XIOBIT_RDWR	/* SSL is a bidirectional protocol */
   HELP("")	/* a text displayed from xio help function.
			   No trailing comma or semicolon!
			   only generates this text if WITH_HELP is != 0 */
} ;

/* description record for endpoint-address ssl connect with 2 parameters */
static const struct xioaddr_endpoint_desc xioendpoint_openssl_connect2 = {
   XIOADDR_ENDPOINT,	/* this is not an embedded address but a sys address */
   "openssl-client",	/* keyword for selecting this address type in xioopen calls
		   (canonical or main name) */
   2,		/* number of required parameters */
   XIOBIT_ALL,	/* data flow directions this address supports on API layer:
		   XIOBIT_RDONLY|XIOBIT_WRONLY|XIOBIT_RDWR */
   GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP4|GROUP_SOCK_IP6|GROUP_IP_TCP|GROUP_CHILD|GROUP_OPENSSL|GROUP_RETRY,	/* bitwise OR of address groups this address belongs to.
		   You might have to specify a new group in xioopts.h */
   XIOSHUT_OPENSSL,
   XIOCLOSE_CLOSE,
   xioopen_openssl_connect,	/* a function pointer used to "open" these addresses.*/
   0,		/* an integer passed to xioopen_openssl; makes it possible to
		   use the same xioopen_openssl function for slightly different
		   address types. */
   0,		/* like previous argument */
   0		/* like previous arguments, but pointer type.
		   No trailing comma or semicolon! */
   HELP(":<host>:<port>")	/* a text displayed from xio help function.
			   No trailing comma or semicolon!
			   only generates this text if WITH_HELP is != 0 */
} ;

/* array containing ssl connect description records */
const union xioaddr_desc *xioaddrs_openssl_connect[] = {
   (union xioaddr_desc *)&xiointer_openssl_connect0,
   (union xioaddr_desc *)&xioendpoint_openssl_connect2,
   NULL
 };


#if WITH_LISTEN
/* description record for inter-address ssl listen */
static const struct xioaddr_inter_desc xiointer_openssl_listen0 = {
   XIOADDR_INTER,	/* this is an embedded address (inter module) */
   "openssl-server",	/* keyword for selecting this address type in xioopen calls
		   (canonical or main name) */
   0,		/* number of required parameters */
   XIOBIT_ALL,	/* data flow directions this address supports on API layer:
		   XIOBIT_RDONLY|XIOBIT_WRONLY|XIOBIT_RDWR */
   GROUP_CHILD|GROUP_OPENSSL|GROUP_RETRY,	/* bitwise OR of address groups this address belongs to.
		   You might have to specify a new group in xioopts.h */
   XIOSHUT_OPENSSL,
   XIOCLOSE_CLOSE,
   xioopen_openssl_listen,	/* a function pointer used to "open" these addresses.*/
   0,		/* an integer passed to xioopen_openssl_listen; makes it possible to
		   use the same xioopen_openssl_listen function for slightly different
		   address types. */
   0,		/* like previous argument */
   0,		/* like previous arguments, but pointer type.
		   No trailing comma or semicolon! */
   XIOBIT_RDWR	/* SSL is a bidirectional protocol */
   HELP("")	/* a text displayed from xio help function.
			   No trailing comma or semicolon!
			   only generates this text if WITH_HELP is != 0 */
} ;

/* description record for enpoint-address ssl listen */
static const struct xioaddr_endpoint_desc xioendpoint_openssl_listen1 = {
   XIOADDR_ENDPOINT,	/* this is not an embedded module but a sys module */
   "openssl-server",	/* keyword for selecting this address type in xioopen calls
		   (canonical or main name) */
   1,		/* number of required parameters */
   XIOBIT_ALL,	/* data flow directions this address supports on API layer:
		   XIOBIT_RDONLY|XIOBIT_WRONLY|XIOBIT_RDWR */
   GROUP_FD|GROUP_SOCKET|GROUP_SOCK_IP4|GROUP_SOCK_IP6|GROUP_IP_TCP|GROUP_LISTEN|GROUP_CHILD|GROUP_RANGE|GROUP_OPENSSL|GROUP_RETRY,	/* bitwise OR of address groups this address belongs to.
		   You might have to specify a new group in xioopts.h */
   XIOSHUT_OPENSSL,
   XIOCLOSE_CLOSE,
   xioopen_openssl_listen,	/* a function pointer used to "open" these addresses.*/
   0,		/* an integer passed to xioopen_openssl_listen; makes it possible to
		   use the same xioopen_openssl_listen function for slightly different
		   address types. */
   0,		/* like previous argument */
   0		/* like previous arguments, but pointer type.
		   No trailing comma or semicolon! */
   HELP(":<port>")	/* a text displayed from xio help function.
			   No trailing comma or semicolon!
			   only generates this text if WITH_HELP is != 0 */
} ;

/* array containing ssl listen description records */
const union xioaddr_desc *xioaddrs_openssl_listen[] = {
   (union xioaddr_desc *)&xiointer_openssl_listen0,
   (union xioaddr_desc *)&xioendpoint_openssl_listen1,
   NULL
 };
#endif /* WITH_LISTEN */

/* both client and server */
const struct optdesc opt_openssl_cipherlist = { "openssl-cipherlist", "ciphers", OPT_OPENSSL_CIPHERLIST, GROUP_OPENSSL, PH_SPEC, TYPE_STRING, OFUNC_SPEC };
const struct optdesc opt_openssl_method     = { "openssl-method",     "method",  OPT_OPENSSL_METHOD,     GROUP_OPENSSL, PH_SPEC, TYPE_STRING, OFUNC_SPEC };
const struct optdesc opt_openssl_verify     = { "openssl-verify",     "verify",  OPT_OPENSSL_VERIFY,     GROUP_OPENSSL, PH_SPEC, TYPE_BOOL,   OFUNC_SPEC };
const struct optdesc opt_openssl_certificate = { "openssl-certificate", "cert",  OPT_OPENSSL_CERTIFICATE, GROUP_OPENSSL, PH_SPEC, TYPE_FILENAME, OFUNC_SPEC };
const struct optdesc opt_openssl_key         = { "openssl-key",         "key",   OPT_OPENSSL_KEY,         GROUP_OPENSSL, PH_SPEC, TYPE_FILENAME, OFUNC_SPEC };
const struct optdesc opt_openssl_dhparam     = { "openssl-dhparam",     "dh",    OPT_OPENSSL_DHPARAM,     GROUP_OPENSSL, PH_SPEC, TYPE_FILENAME, OFUNC_SPEC };
const struct optdesc opt_openssl_cafile      = { "openssl-cafile",     "cafile", OPT_OPENSSL_CAFILE,      GROUP_OPENSSL, PH_SPEC, TYPE_FILENAME, OFUNC_SPEC };
const struct optdesc opt_openssl_capath      = { "openssl-capath",     "capath", OPT_OPENSSL_CAPATH,      GROUP_OPENSSL, PH_SPEC, TYPE_FILENAME, OFUNC_SPEC };
const struct optdesc opt_openssl_egd         = { "openssl-egd",        "egd",    OPT_OPENSSL_EGD,         GROUP_OPENSSL, PH_SPEC, TYPE_FILENAME, OFUNC_SPEC };
const struct optdesc opt_openssl_pseudo      = { "openssl-pseudo",     "pseudo", OPT_OPENSSL_PSEUDO,      GROUP_OPENSSL, PH_SPEC, TYPE_BOOL,     OFUNC_SPEC };
#if OPENSSL_VERSION_NUMBER >= 0x00908000L && !defined(OPENSSL_NO_COMP)
const struct optdesc opt_openssl_compress    = { "openssl-compress",   "compress", OPT_OPENSSL_COMPRESS,  GROUP_OPENSSL, PH_SPEC, TYPE_STRING,   OFUNC_SPEC };
#endif
#if WITH_FIPS
const struct optdesc opt_openssl_fips        = { "openssl-fips",       "fips",   OPT_OPENSSL_FIPS,        GROUP_OPENSSL, PH_SPEC, TYPE_BOOL,     OFUNC_SPEC };
#endif
const struct optdesc opt_openssl_commonname  = { "openssl-commonname", "cn",     OPT_OPENSSL_COMMONNAME,  GROUP_OPENSSL, PH_SPEC, TYPE_STRING,   OFUNC_SPEC };


/* If FIPS is compiled in, we need to track if the user asked for FIPS mode.
 * On forks, the FIPS mode must be reset by a disable, then enable since
 * FIPS tracks the process ID that initializes things.
 * If FIPS is not compiled in, no tracking variable is needed
 * and we make the reset code compile out.  This keeps the
 * rest of the code below free of FIPS related #ifs
 */
#if WITH_FIPS
static bool xio_openssl_fips = false;
int xio_reset_fips_mode(void) {
   if (xio_openssl_fips) {
      if(!sycFIPS_mode_set(0) || !sycFIPS_mode_set(1)) {
	 ERR_load_crypto_strings();
	 ERR_print_errors(BIO_new_fp(stderr,BIO_NOCLOSE));
	 Error("Failed to reset OpenSSL FIPS mode");
	 xio_openssl_fips = false;
         return -1;
      }
   }
   return 0;
}
#else
#define xio_reset_fips_mode() 0
#endif

static void openssl_conn_loginfo(SSL *ssl) {
   Notice1("SSL connection using %s", SSL_get_cipher(ssl));

#if OPENSSL_VERSION_NUMBER >= 0x00908000L && !defined(OPENSSL_NO_COMP)
   {
      const COMP_METHOD *comp, *expansion;

      comp = sycSSL_get_current_compression(ssl);
      expansion = sycSSL_get_current_expansion(ssl);

      Notice1("SSL connection compression \"%s\"",
	      comp?sycSSL_COMP_get_name(comp):"none");
      Notice1("SSL connection expansion \"%s\"",
	      expansion?sycSSL_COMP_get_name(expansion):"none");
   }
#endif
}

/* the open function for OpenSSL client */
static int
   xioopen_openssl_connect(int argc,
		   const char *argv[],	/* the arguments in the address string */
		   struct opt *opts,
		   int xioflags,	/* is the open meant for reading (0),
				   writing (1), or both (2) ? */
		   xiofile_t *xxfd,	/* a xio file descriptor structure,
				   already allocated */
		   unsigned groups,	/* the matching address groups... */
		   int dummy1,	/* first transparent integer value from
				   addr_openssl */
		   int dummy2,	/* second transparent integer value from
				   addr_openssl */
		   int dummy3)	/* transparent pointer value from
					   addr_openssl */
{
   struct single *xfd = &xxfd->stream;
   int rw = (xioflags&XIO_ACCMODE);
   struct opt *opts0 = NULL;
   const char *hostname, *portname = NULL;
   int pf = PF_UNSPEC;
   int ipproto = IPPROTO_TCP;
   int socktype = SOCK_STREAM;
   bool dofork = false;
   union sockaddr_union us_sa,  *us = &us_sa;
   union sockaddr_union them_sa, *them = &them_sa;
   socklen_t uslen = sizeof(us_sa);
   socklen_t themlen = sizeof(them_sa);
   bool needbind = false;
   bool lowport = false;
   int level;
   /*0 SSL_CTX* ctx;*/
   bool opt_ver = true;	/* verify peer certificate */
   char *opt_cert = NULL;	/* file name of client certificate */
   const char *opt_commonname = NULL;	/* for checking peer certificate */
   int result;

   if (!(xioflags & XIO_MAYCONVERT)) {
      Error("address with data processing not allowed here");
      return STAT_NORETRY;
   }
   xfd->flags |= XIO_DOESCONVERT;

   xfd->howtoshut  = XIOSHUT_OPENSSL;
   xfd->howtoclose = XIOCLOSE_CLOSE;

   /* we support two forms of openssl-connect */
   if (argc == 3) {
      hostname = argv[1];
      portname = argv[2];
      if (hostname[0] == '\0') {
	 /* we catch this explicitely because empty commonname (peername) disables
	 commonName check of peer certificate */
	 Error1("%s: empty host name", argv[0]);
	 return STAT_NORETRY;
      }

      /* a "terminal" form where we build a tcp connection to given host and
	 port */

      applyopts_single(xfd, opts, PH_INIT);
      applyopts(-1, opts, PH_INIT);

      retropt_bool(opts, OPT_FORK, &dofork);

      retropt_string(opts, OPT_OPENSSL_CERTIFICATE, &opt_cert);
      retropt_string(opts, OPT_OPENSSL_COMMONNAME, (char **)&opt_commonname);
   
      if (opt_commonname == NULL) {
	 opt_commonname = hostname;
      }

      result =
	 _xioopen_openssl_prepare(opts, xfd, false, &opt_ver, opt_cert,
				  &xfd->para.openssl.ctx);
      if (result != STAT_OK)  return STAT_NORETRY;

      result =
	 _xioopen_ipapp_prepare(opts, &opts0, hostname, portname, &pf, ipproto,
				xfd->para.socket.ip.res_opts[1],
				xfd->para.socket.ip.res_opts[0],
				them, &themlen, us, &uslen,
				&needbind, &lowport, socktype);
      if (result != STAT_OK)  return STAT_NORETRY;
   } else if (argc == 1) {

      /* or a "non terminal" address without required parameters */
      if (xfd->wfd < 0) {
	 Error("openssl-connect without hostname and port must be an embedded address");
	 return STAT_NORETRY;
      }
   
      if (applyopts_single(xfd, opts, PH_INIT) < 0)  return -1;
      applyopts(-1, opts, PH_INIT);

      retropt_bool(opts, OPT_FORK, &dofork);
      retropt_string(opts, OPT_OPENSSL_CERTIFICATE, &opt_cert);
      retropt_string(opts, OPT_OPENSSL_COMMONNAME, (char **)&opt_commonname);

      result =
	 _xioopen_openssl_prepare(opts, xfd, false, &opt_ver, opt_cert,
				  &xfd->para.openssl.ctx);
      if (result != STAT_OK)  return STAT_NORETRY;

   retropt_bool(opts, OPT_FORK, &dofork);

   } else {
      Error1("%s: 0 or 2 parameters required", argv[0]);
      return STAT_NORETRY;
   }

   if (xioopts.logopt == 'm') {
      Info("starting connect loop, switching to syslog");
      diag_set('y', xioopts.syslogfac);  xioopts.logopt = 'y';
   } else {
      Info("starting connect loop");
   }

   do {	/* loop over failed connect and SSL handshake attempts */

#if WITH_RETRY
      if (xfd->forever || xfd->retry) {
	 level = E_INFO;
      } else
#endif /* WITH_RETRY */
	 level = E_ERROR;

      /*!!! this belongs only to "old" openssl-connect form */
     if (portname) {
      /* this cannot fork because we retrieved fork option above */
      result =
	 _xioopen_connect(xfd,
			  needbind?(struct sockaddr *)us:NULL, uslen,
			  (struct sockaddr *)them, themlen,
			  opts, pf, socktype, ipproto, lowport, level);
      switch (result) {
      case STAT_OK: break;
#if WITH_RETRY
      case STAT_RETRYLATER:
      case STAT_RETRYNOW:
	 if (xfd->forever || xfd->retry) {
	    dropopts(opts, PH_ALL); opts = copyopts(opts0, GROUP_ALL);
	    if (result == STAT_RETRYLATER) {
	       Nanosleep(&xfd->intervall, NULL);
	    }
	    --xfd->retry;
	    continue;
	 }
	 return STAT_NORETRY;
#endif /* WITH_RETRY */
      default:
	 return result;
	 }
      xfd->wfd = xfd->rfd;
       }

       /*! isn't this too early? */
       if ((result = _xio_openlate(xfd, opts)) < 0) {
	  return result;
       }

       result =
	  _xioopen_openssl_connect(xfd, opt_ver, opt_commonname, xfd->para.openssl.ctx, level);
       switch (result) {
       case STAT_OK: break;
 #if WITH_RETRY
       case STAT_RETRYLATER:
       case STAT_RETRYNOW:
	  if (xfd->forever || xfd->retry) {
	     Close(xfd->rfd);
	     Close(xfd->wfd);
	     dropopts(opts, PH_ALL); opts = copyopts(opts0, GROUP_ALL);
	     if (result == STAT_RETRYLATER) {
		Nanosleep(&xfd->intervall, NULL);
	     }
	     --xfd->retry;
	     continue;
	  }
 #endif /* WITH_RETRY */
       default: return STAT_NORETRY;
       }

       if (dofork) {
	  xiosetchilddied();	/* set SIGCHLD handler */
       }

 #if WITH_RETRY
       if (dofork) {
	  pid_t pid;
	  int level = E_ERROR;
	  if (xfd->forever || xfd->retry) {
	     level = E_WARN;
	  }
	  while ((pid = xio_fork(false, level)) < 0) {
	     if (xfd->forever || --xfd->retry) {
		Nanosleep(&xfd->intervall, NULL); continue;
	     }
	     return STAT_RETRYLATER;
	  }

	  if (pid == 0) {	/* child process */
	     xfd->forever = false;  xfd->retry = 0;
	     break;
	  }

	  /* parent process */
	  Notice1("forked off child process "F_pid, pid);
	  Close(xfd->rfd);
	  Close(xfd->wfd);
	  sycSSL_free(xfd->para.openssl.ssl);
	  xfd->para.openssl.ssl = NULL;
	  /* with and without retry */
	  Nanosleep(&xfd->intervall, NULL);
	  dropopts(opts, PH_ALL); opts = copyopts(opts0, GROUP_ALL);
	  continue;	/* with next socket() bind() connect() */
       }
 #endif /* WITH_RETRY */
       break;
    } while (true);	/* drop out on success */

    openssl_conn_loginfo(xfd->para.openssl.ssl);

    /* fill in the fd structure */
    return STAT_OK;
 }


 /* this function is typically called within the OpenSSL client fork/retry loop.
    xfd must be of type DATA_OPENSSL, and its fd must be set with a valid file
    descriptor. this function then performs all SSL related step to make a valid
    SSL connection from an FD and a CTX. */
 int _xioopen_openssl_connect(struct single *xfd,
			     bool opt_ver,
			      const char *opt_commonname,
			     SSL_CTX *ctx,
			     int level) {
   SSL *ssl;
   unsigned long err;
   int result;

   /* create a SSL object */
   if ((ssl = sycSSL_new(ctx)) == NULL) {
      if (ERR_peek_error() == 0)  Msg(level, "SSL_new() failed");
      while (err = ERR_get_error()) {
	 Msg1(level, "SSL_new(): %s", ERR_error_string(err, NULL));
      }
      /*Error("SSL_new()");*/
      return STAT_RETRYLATER;
   }
   xfd->para.openssl.ssl = ssl;

   result = xioSSL_set_fd(xfd, level);
   if (result != STAT_OK) {
      sycSSL_free(xfd->para.openssl.ssl);
      xfd->para.openssl.ssl = NULL;
      return result;
   }

   result = xioSSL_connect(xfd, opt_commonname, opt_ver, level);
   if (result != STAT_OK) {
      sycSSL_free(xfd->para.openssl.ssl);
      xfd->para.openssl.ssl = NULL;
      return result;
   }

   result = openssl_handle_peer_certificate(xfd, opt_commonname,
					    opt_ver, level);
   if (result != STAT_OK) {
      sycSSL_free(xfd->para.openssl.ssl);
      xfd->para.openssl.ssl = NULL;
      return result;
   }

   return STAT_OK;
}


#if WITH_LISTEN

static int
   xioopen_openssl_listen(int argc,
		   const char *argv[],	/* the arguments in the address string */
		   struct opt *opts,
		   int xioflags,	/* is the open meant for reading (0),
				   writing (1), or both (2) ? */
		   xiofile_t *xxfd,	/* a xio file descriptor structure,
				   already allocated */
		   unsigned groups,	/* the matching address groups... */
		   int dummy1,	/* first transparent integer value from
				   addr_openssl */
		   int dummy2,	/* second transparent integer value from
				   addr_openssl */
		   int dummy3)	/* transparent pointer value from
					   addr_openssl */
{
   struct single *xfd = &xxfd->stream;
   const char *portname = NULL;
   struct opt *opts0 = NULL;
   union sockaddr_union us_sa, *us = &us_sa;
   socklen_t uslen = sizeof(us_sa);
   int pf;
   int socktype = SOCK_STREAM;
   int ipproto = IPPROTO_TCP;
   /*! lowport? */
   int level;
   /*0 SSL_CTX* ctx;*/
   bool opt_ver = true;	/* verify peer certificate - changed with 1.6.0 */
   char *opt_cert = NULL;	/* file name of server certificate */
   const char *opt_commonname = NULL;	/* for checking peer certificate */
   int result;

   if (!(xioflags & XIO_MAYCONVERT)) {
      Error("address with data processing not allowed here");
      return STAT_NORETRY;
   }
   xfd->flags |= XIO_DOESCONVERT;

#if WITH_IP4 && WITH_IP6
   pf = xioopts.default_ip=='6'?PF_INET6:PF_INET;
#elif WITH_IP6
   pf = PF_INET6;
#else
   pf = PF_INET;
#endif

   if (argc == 2) {

      portname = argv[1];
      if (applyopts_single(xfd, opts, PH_INIT) < 0)  return -1;
      applyopts(-1, opts, PH_INIT);

      retropt_string(opts, OPT_OPENSSL_CERTIFICATE, &opt_cert);
      if (opt_cert == NULL) {
	 Warn("no certificate given; consider option \"cert\"");
      }

      retropt_string(opts, OPT_OPENSSL_COMMONNAME, (char **)&opt_commonname);

      result =
	 _xioopen_openssl_prepare(opts, xfd, true, &opt_ver, opt_cert,
				  &xfd->para.openssl.ctx);
      if (result != STAT_OK)  return STAT_NORETRY;

      if (_xioopen_ipapp_listen_prepare(opts, &opts0, portname, &pf, ipproto,
					xfd->para.socket.ip.res_opts[1],
					xfd->para.socket.ip.res_opts[0],
					us, &uslen, socktype)
       != STAT_OK) {
	 return STAT_NORETRY;
      }

   } else if (argc == 1) {
      if (xfd->rfd < 0) {
	 Error("openssl-listen without port must be an embedded address");
	 return STAT_NORETRY;
      }

      if (applyopts_single(xfd, opts, PH_INIT) < 0)  return -1;
      applyopts(-1, opts, PH_INIT);

      retropt_string(opts, OPT_OPENSSL_CERTIFICATE, &opt_cert);
      if (opt_cert == NULL) {
	 Warn("no certificate given; consider option \"cert\"");
      }

      retropt_string(opts, OPT_OPENSSL_COMMONNAME, (char **)&opt_commonname);

      applyopts(-1, opts, PH_EARLY);

      result =
	 _xioopen_openssl_prepare(opts, xfd, true, &opt_ver, opt_cert,
				  &xfd->para.openssl.ctx);
      if (result != STAT_OK)  return STAT_NORETRY;

   } else {
      Error1("%s: 1 parameter required", argv[0]);
      return -1;
   }

   xfd->dtype = XIODATA_OPENSSL;

   while (true) {	/* loop over failed attempts */

#if WITH_RETRY
      if (xfd->forever || xfd->retry) {
	 level = E_INFO;
      } else
#endif /* WITH_RETRY */
	 level = E_ERROR;

      if (portname) {
       /* tcp listen; this can fork() for us; it only returns on error or on
	  successful establishment of tcp connection */
	 result = _xioopen_listen(xfd, (xioflags&~XIO_ACCMODE)|XIO_RDWR,
				  (struct sockaddr *)us, uslen,
				  opts, pf, socktype, IPPROTO_TCP,
#if WITH_RETRY
				  (xfd->retry||xfd->forever)?E_INFO:E_ERROR
#else
				  E_ERROR
#endif /* WITH_RETRY */
				  );
	 /*! not sure if we should try again on retry/forever */
      switch (result) {
      case STAT_OK: break;
#if WITH_RETRY
      case STAT_RETRYLATER:
      case STAT_RETRYNOW:
	 if (xfd->forever || xfd->retry) {
	    dropopts(opts, PH_ALL); opts = copyopts(opts0, GROUP_ALL);
	    if (result == STAT_RETRYLATER) {
	       Nanosleep(&xfd->intervall, NULL);
	    }
	    dropopts(opts, PH_ALL); opts = copyopts(opts0, GROUP_ALL);
	    --xfd->retry;
	    continue;
	 }
	 return STAT_NORETRY;
#endif /* WITH_RETRY */
      default:
	 return result;
      }
      xfd->wfd = xfd->rfd;
      }
      result =
	 _xioopen_openssl_listen(xfd, opt_ver, opt_commonname, xfd->para.openssl.ctx, level);
      switch (result) {
      case STAT_OK: break;
#if WITH_RETRY
      case STAT_RETRYLATER:
      case STAT_RETRYNOW:
	 if (xfd->forever || xfd->retry) {
	    dropopts(opts, PH_ALL); opts = copyopts(opts0, GROUP_ALL);
	    if (result == STAT_RETRYLATER) {
	       Nanosleep(&xfd->intervall, NULL);
	    }
	    dropopts(opts, PH_ALL); opts = copyopts(opts0, GROUP_ALL);
	    --xfd->retry;
	    continue;
	 }
	 return STAT_NORETRY;
#endif /* WITH_RETRY */
      default:
	 return result;
      }

      openssl_conn_loginfo(xfd->para.openssl.ssl);
      break;

   }	/* drop out on success */

   /* fill in the fd structure */

   return STAT_OK;
}


int _xioopen_openssl_listen(struct single *xfd,
			     bool opt_ver,
			    const char *opt_commonname,
			     SSL_CTX *ctx,
			     int level) {
   char error_string[120];
   unsigned long err;
   int errint, ret;

   /* create an SSL object */
   if ((xfd->para.openssl.ssl = sycSSL_new(ctx)) == NULL) {
      if (ERR_peek_error() == 0)  Msg(level, "SSL_new() failed");
      while (err = ERR_get_error()) {
	 Msg1(level, "SSL_new(): %s", ERR_error_string(err, NULL));
      }
      /*Error("SSL_new()");*/
      return STAT_NORETRY;
   }

   /* assign the network connection to the SSL object */
   ret = xioSSL_set_fd(xfd, level);
   if (ret != STAT_OK) {
      sycSSL_free(xfd->para.openssl.ssl);
      xfd->para.openssl.ssl = NULL;
      return ret;
   }

#if WITH_DEBUG
   {
      int i = 0;
      const char *ciphers = NULL;
      Debug("available ciphers:");
      do {
	 ciphers = SSL_get_cipher_list(xfd->para.openssl.ssl, i);
	 if (ciphers == NULL)  break;
	 Debug2("CIPHERS pri=%d: %s", i, ciphers);
	 ++i;
      } while (1);
   }
#endif /* WITH_DEBUG */

   /* connect via SSL by performing handshake */
   if ((ret = sycSSL_accept(xfd->para.openssl.ssl)) <= 0) {
      /*if (ERR_peek_error() == 0) Msg(level, "SSL_accept() failed");*/
      errint = SSL_get_error(xfd->para.openssl.ssl, ret);
      switch (errint) {
      case SSL_ERROR_NONE:
	 Msg(level, "ok"); break;
      case SSL_ERROR_ZERO_RETURN:
	 Msg(level, "connection closed (wrong version number?)"); break;
      case SSL_ERROR_WANT_READ: case SSL_ERROR_WANT_WRITE:
      case SSL_ERROR_WANT_CONNECT:
      case SSL_ERROR_WANT_X509_LOOKUP:
	 Msg(level, "nonblocking operation did not complete"); break;	/*!*/
      case SSL_ERROR_SYSCALL:
	 if (ERR_peek_error() == 0) {
	    if (ret == 0) {
	       Msg(level, "SSL_accept(): socket closed by peer");
	    } else if (ret == -1) {
	       Msg1(level, "SSL_accept(): %s", strerror(errno));
	    }
	 } else {
	    Msg(level, "I/O error");	/*!*/
	    while (err = ERR_get_error()) {
	       ERR_error_string_n(err, error_string, sizeof(error_string));
	       Msg4(level, "SSL_accept(): %s / %s / %s / %s", error_string,
		    ERR_lib_error_string(err), ERR_func_error_string(err),
		    ERR_reason_error_string(err));
	    }
	    /* Msg1(level, "SSL_accept(): %s", ERR_error_string(e, buf));*/
	 }
	 break;
      case SSL_ERROR_SSL:
	 /*ERR_print_errors_fp(stderr);*/
	 openssl_SSL_ERROR_SSL(level, "SSL_accept");
	 break;
      default:
	 Msg(level, "unknown error");
      }

      return STAT_RETRYLATER;
   }

   if (openssl_handle_peer_certificate(xfd, opt_commonname, opt_ver, E_ERROR/*!*/) < 0) {
      return STAT_NORETRY;
   }

   return STAT_OK;
}

#endif /* WITH_LISTEN */


#if OPENSSL_VERSION_NUMBER >= 0x00908000L && !defined(OPENSSL_NO_COMP)
/* In OpenSSL 0.9.7 compression methods could be added using
 * SSL_COMP_add_compression_method(3), but the implemntation is not compatible
 * with the standard (RFC3749).
 */
static int openssl_setup_compression(SSL_CTX *ctx, char *method)
{
   STACK_OF(SSL_COMP)* comp_methods;

   assert(method);

   /* Getting the stack of compression methods has the intended side-effect of
    * initializing the SSL library's compression part.
    */
   comp_methods = SSL_COMP_get_compression_methods();
   if (!comp_methods) {
      Info("OpenSSL built without compression support");
      return STAT_OK;
   }

   if (strcasecmp(method, "auto") == 0) {
      Info("Using default OpenSSL compression");
      return STAT_OK;
   }

   if (strcasecmp(method, "none") == 0) {
      /* Disable compression */
#ifdef SSL_OP_NO_COMPRESSION
      Info("Disabling OpenSSL compression");
      SSL_CTX_set_options(ctx, SSL_OP_NO_COMPRESSION);
#else
      /* SSL_OP_NO_COMPRESSION was only introduced in OpenSSL 0.9.9 (released
       * as 1.0.0). Removing all compression methods is a work-around for
       * earlier versions of OpenSSL, but it affects all SSL connections.
       */
      Info("Disabling OpenSSL compression globally");
      sk_SSL_COMP_zero(comp_methods);
#endif
      return STAT_OK;
   }

   /* zlib compression in OpenSSL before version 0.9.8e-beta1 uses the libc's
    * default malloc/free instead of the ones passed to OpenSSL. Should socat
    * ever use custom malloc/free functions for OpenSSL, this must be taken
    * into consideration. See OpenSSL bug #1468.
    */

   Error1("openssl-compress=\"%s\": unknown compression method", method);
   return STAT_NORETRY;
}
#endif


int
   _xioopen_openssl_prepare(struct opt *opts,
			    struct single *xfd,/* a xio file descriptor
						  structure, already allocated
					       */
			    bool server,	/* SSL client: false */
			    bool *opt_ver,
			    const char *opt_cert,
			    SSL_CTX **ctx)
{
   bool opt_fips = false;
   const SSL_METHOD *method = NULL;
   char *me_str = NULL;	/* method string */
   char *ci_str = "HIGH:-NULL:-PSK:-aNULL";	/* cipher string */
   char *opt_key  = NULL;	/* file name of client private key */
   char *opt_dhparam = NULL;	/* file name of DH params */
   char *opt_cafile = NULL;	/* certificate authority file */
   char *opt_capath = NULL;	/* certificate authority directory */
   char *opt_egd = NULL;	/* entropy gathering daemon socket path */
#if OPENSSL_VERSION_NUMBER >= 0x00908000L && !defined(OPENSSL_NO_COMP)
   char *opt_compress = NULL;  /* compression method */
#endif
   bool opt_pseudo = false;	/* use pseudo entropy if nothing else */
   unsigned long err;
   int result;

   xfd->dtype = XIODATA_OPENSSL;

   retropt_bool(opts, OPT_OPENSSL_FIPS, &opt_fips);
   retropt_string(opts, OPT_OPENSSL_METHOD, &me_str);
   retropt_string(opts, OPT_OPENSSL_CIPHERLIST, &ci_str);
   retropt_bool(opts, OPT_OPENSSL_VERIFY, opt_ver);
   retropt_string(opts, OPT_OPENSSL_CAFILE, &opt_cafile);
   retropt_string(opts, OPT_OPENSSL_CAPATH, &opt_capath);
   retropt_string(opts, OPT_OPENSSL_KEY, &opt_key);
   retropt_string(opts, OPT_OPENSSL_DHPARAM, &opt_dhparam);
   retropt_string(opts, OPT_OPENSSL_EGD, &opt_egd);
   retropt_bool(opts,OPT_OPENSSL_PSEUDO, &opt_pseudo);
#if OPENSSL_VERSION_NUMBER >= 0x00908000L && !defined(OPENSSL_NO_COMP)
   retropt_string(opts, OPT_OPENSSL_COMPRESS, &opt_compress);
#endif
#if WITH_FIPS
   if (opt_fips) {
      if (!sycFIPS_mode_set(1)) {
	 ERR_load_crypto_strings();
	 ERR_print_errors(BIO_new_fp(stderr,BIO_NOCLOSE));
	 Error("Failed to set FIPS mode");
      } else {
	 xio_openssl_fips = true;
      }
   }
#endif

   openssl_delete_cert_info();

   OpenSSL_add_all_algorithms();
   OpenSSL_add_all_ciphers();
   OpenSSL_add_all_digests();
   sycSSL_load_error_strings();

   /* OpenSSL preparation */
   sycSSL_library_init();
   
   /*! actions_to_seed_PRNG();*/

   if (!server) {
      if (me_str != NULL) {
	 if (false) {
	    ;       /* for canonical reasons */
#if HAVE_SSLv2_client_method
	 } else if (!strcasecmp(me_str, "SSL2")) {
	    method = sycSSLv2_client_method();
#endif
#if HAVE_SSLv3_client_method
	 } else if (!strcasecmp(me_str, "SSL3")) {
	    method = sycSSLv3_client_method();
#endif
#if HAVE_SSLv23_client_method
	 } else if (!strcasecmp(me_str, "SSL23")) {
	    method = sycSSLv23_client_method();
#endif
#if HAVE_TLSv1_client_method
	 } else if (!strcasecmp(me_str, "TLS1") || !strcasecmp(me_str, "TLS1.0")) {
	    method = sycTLSv1_client_method();
#endif
#if HAVE_TLSv1_1_client_method
	 } else if (!strcasecmp(me_str, "TLS1.1")) {
	    method = sycTLSv1_1_client_method();
#endif
#if HAVE_TLSv1_2_client_method
	 } else if (!strcasecmp(me_str, "TLS1.2")) {
	    method = sycTLSv1_2_client_method();
#endif
#if HAVE_DTLSv1_client_method
	 } else if (!strcasecmp(me_str, "DTLS") || !strcasecmp(me_str, "DTLS1")) {
	    method = sycDTLSv1_client_method();
#endif
 	 } else {
	    Error1("openssl-method=\"%s\": method unknown or not provided by library", me_str);
	 }
      } else {
#if   HAVE_SSLv23_client_method
	 method = sycSSLv23_client_method();
#elif HAVE_TLSv1_2_client_method
	 method = sycTLSv1_2_client_method();
#elif HAVE_TLSv1_1_client_method
	 method = sycTLSv1_1_client_method();
#elif HAVE_TLSv1_client_method
	 method = sycTLSv1_client_method();
#elif HAVE_SSLv3_client_method
	 method = sycSSLv3_client_method();
#elif HAVE_SSLv2_client_method
	 method = sycSSLv2_client_method();
#else
#        error "OpenSSL does not seem to provide client methods"
#endif
      }
   } else /* server */ {
      if (me_str != NULL) {
	 if (false) {
	    ;       /* for canonical reasons */
#if HAVE_SSLv2_server_method
	 } else if (!strcasecmp(me_str, "SSL2")) {
 	    method = sycSSLv2_server_method();
#endif
#if HAVE_SSLv3_server_method
	 } else if (!strcasecmp(me_str, "SSL3")) {
	    method = sycSSLv3_server_method();
#endif
#if HAVE_SSLv23_server_method
	 } else if (!strcasecmp(me_str, "SSL23")) {
	    method = sycSSLv23_server_method();
#endif
#if HAVE_TLSv1_server_method
	 } else if (!strcasecmp(me_str, "TLS1") || !strcasecmp(me_str, "TLS1.0")) {
	    method = sycTLSv1_server_method();
#endif
#if HAVE_TLSv1_1_server_method
	 } else if (!strcasecmp(me_str, "TLS1.1")) {
	    method = sycTLSv1_1_server_method();
#endif
#if HAVE_TLSv1_2_server_method
	 } else if (!strcasecmp(me_str, "TLS1.2")) {
	    method = sycTLSv1_2_server_method();
#endif
#if HAVE_DTLSv1_server_method
	 } else if (!strcasecmp(me_str, "DTLS") || !strcasecmp(me_str, "DTLS1")) {
	    method = sycDTLSv1_server_method();
#endif
	 } else {
	    Error1("openssl-method=\"%s\": method unknown or not provided by library", me_str);
	 }
      } else {
#if   HAVE_SSLv23_server_method
	 method = sycSSLv23_server_method();
#elif HAVE_TLSv1_2_server_method
	 method = sycTLSv1_2_server_method();
#elif HAVE_TLSv1_1_server_method
	 method = sycTLSv1_1_server_method();
#elif HAVE_TLSv1_server_method
	 method = sycTLSv1_server_method();
#elif HAVE_SSLv3_server_method
	 method = sycSSLv3_server_method();
#elif HAVE_SSLv2_server_method
	 method = sycSSLv2_server_method();
#else
#        error "OpenSSL does not seem to provide client methods"
#endif
      }
   }

   if (opt_egd) {
      sycRAND_egd(opt_egd);
   }

   if (opt_pseudo) {
      long int randdata;
      /* initialize libc random from actual microseconds */
      struct timeval tv;
      struct timezone tz;
      tz.tz_minuteswest = 0;
      tz.tz_dsttime = 0;
      if ((result = Gettimeofday(&tv, &tz)) < 0) {
	 Warn2("gettimeofday(%p, {0,0}): %s", &tv, strerror(errno));
      }
      srandom(tv.tv_sec*1000000+tv.tv_usec);

      while (!RAND_status()) {
	 randdata = random();
	 Debug2("RAND_seed(0x{%lx}, "F_Zu")",
		randdata, sizeof(randdata));
	 RAND_seed(&randdata, sizeof(randdata));
      }
   }

   if ((*ctx = sycSSL_CTX_new(method)) == NULL) {
      if (ERR_peek_error() == 0) Error("SSL_CTX_new()");
      while (err = ERR_get_error()) {
	 Error1("SSL_CTX_new(): %s", ERR_error_string(err, NULL));
      }

      /*ERR_clear_error;*/
      return STAT_RETRYLATER;
   }
 
   {
      static unsigned char dh1024_p[] = {
	 0xCC,0x17,0xF2,0xDC,0x96,0xDF,0x59,0xA4,0x46,0xC5,0x3E,0x0E,
	 0xB8,0x26,0x55,0x0C,0xE3,0x88,0xC1,0xCE,0xA7,0xBC,0xB3,0xBF,
	 0x16,0x94,0xD8,0xA9,0x45,0xA2,0xCE,0xA9,0x5B,0x22,0x25,0x5F,
	 0x92,0x59,0x94,0x1C,0x22,0xBF,0xCB,0xC8,0xC8,0x57,0xCB,0xBF,
	 0xBC,0x0E,0xE8,0x40,0xF9,0x87,0x03,0xBF,0x60,0x9B,0x08,0xC6,
	 0x8E,0x99,0xC6,0x05,0xFC,0x00,0xD6,0x6D,0x90,0xA8,0xF5,0xF8,
	 0xD3,0x8D,0x43,0xC8,0x8F,0x7A,0xBD,0xBB,0x28,0xAC,0x04,0x69,
	 0x4A,0x0B,0x86,0x73,0x37,0xF0,0x6D,0x4F,0x04,0xF6,0xF5,0xAF,
	 0xBF,0xAB,0x8E,0xCE,0x75,0x53,0x4D,0x7F,0x7D,0x17,0x78,0x0E,
	 0x12,0x46,0x4A,0xAF,0x95,0x99,0xEF,0xBC,0xA6,0xC5,0x41,0x77,
	 0x43,0x7A,0xB9,0xEC,0x8E,0x07,0x3C,0x6D,
      };
      static unsigned char dh1024_g[] = {
	 0x02,
      };
      DH *dh;
      unsigned long err;

      if ((dh = DH_new()) == NULL) {
	 while (err = ERR_get_error()) {
	    Warn1("DH_new(): %s",
		  ERR_error_string(err, NULL));
	 }
	 Error("DH_new() failed");
      } else {
	 dh->p = BN_bin2bn(dh1024_p, sizeof(dh1024_p), NULL);
	 dh->g = BN_bin2bn(dh1024_g, sizeof(dh1024_g), NULL);
	 if ((dh->p == NULL) || (dh->g == NULL)) {
	    while (err = ERR_get_error()) {
	       Warn1("BN_bin2bn(): %s",
		     ERR_error_string(err, NULL));
	    }
	    Error("BN_bin2bn() failed");
	 } else {
	    if (sycSSL_CTX_set_tmp_dh(*ctx, dh) <= 0) {
	       while (err = ERR_get_error()) {
		  Warn3("SSL_CTX_set_tmp_dh(%p, %p): %s", *ctx, dh,
			ERR_error_string(err, NULL));
	       }
	       Error2("SSL_CTX_set_tmp_dh(%p, %p) failed", *ctx, dh);
	    }
	    /*! OPENSSL_free(dh->p,g)? doc does not tell so */
	 }
	 DH_free(dh);
      }
   }

#if defined(EC_KEY)  /* not on Openindiana 5.11 */
   {
      /* see http://openssl.6102.n7.nabble.com/Problem-with-cipher-suite-ECDHE-ECDSA-AES256-SHA384-td42229.html */
      int     nid;
      EC_KEY *ecdh;

#if 0
      nid = OBJ_sn2nid(ECDHE_CURVE);
      if (nid == NID_undef) {
	 Error("openssl: failed to set ECDHE parameters");
	 return -1;
      }
#endif
      nid = NID_X9_62_prime256v1;
      ecdh = EC_KEY_new_by_curve_name(nid);
      if (NULL == ecdh) {
	 Error("openssl: failed to set ECDHE parameters");
	 return -1;
      }

      SSL_CTX_set_tmp_ecdh(*ctx, ecdh);
   }
#endif /* !defined(EC_KEY) */

#if OPENSSL_VERSION_NUMBER >= 0x00908000L && !defined(OPENSSL_NO_COMP)
   if (opt_compress) {
      int result;
      result = openssl_setup_compression(*ctx, opt_compress);
      if (result != STAT_OK) {
       return result;
      }
   }
#endif

   if (opt_cafile != NULL || opt_capath != NULL) {
      if (sycSSL_CTX_load_verify_locations(*ctx, opt_cafile, opt_capath) != 1) {
	 int result;

	 if ((result =
	      openssl_SSL_ERROR_SSL(E_ERROR, "SSL_CTX_load_verify_locations"))
	     != STAT_OK) {
	    SSL_CTX_free(*ctx); *ctx = NULL;
	    return STAT_RETRYLATER;
	 }
      }
#ifdef HAVE_SSL_CTX_set_default_verify_paths
   } else {
      SSL_CTX_set_default_verify_paths(*ctx);
#endif
   }

   if (opt_cert) {
      BIO *bio;
      DH *dh;

      if (sycSSL_CTX_use_certificate_chain_file(*ctx, opt_cert) <= 0) {
	 /*! trace functions */
	 /*0 ERR_print_errors_fp(stderr);*/
	 if (ERR_peek_error() == 0)
	    Error2("SSL_CTX_use_certificate_file(%p, \"%s\", SSL_FILETYPE_PEM) failed",
		 *ctx, opt_cert);
	 while (err = ERR_get_error()) {
	    Error1("SSL_CTX_use_certificate_file(): %s",
		   ERR_error_string(err, NULL));
	 }
	 return STAT_RETRYLATER;
      }

      if (sycSSL_CTX_use_PrivateKey_file(*ctx, opt_key?opt_key:opt_cert, SSL_FILETYPE_PEM) <= 0) {
	 /*ERR_print_errors_fp(stderr);*/
	 openssl_SSL_ERROR_SSL(E_ERROR/*!*/, "SSL_CTX_use_PrivateKey_file");
	 return STAT_RETRYLATER;
      }

      if (opt_dhparam == NULL) {
	 opt_dhparam = (char *)opt_cert;
      }
      if ((bio = sycBIO_new_file(opt_dhparam, "r")) == NULL) {
	 Warn2("BIO_new_file(\"%s\", \"r\"): %s",
	       opt_dhparam, strerror(errno));
      } else {
	 if ((dh = sycPEM_read_bio_DHparams(bio, NULL, NULL, NULL)) == NULL) {
	    Info1("PEM_read_bio_DHparams(%p, NULL, NULL, NULL): error", bio);
	 } else {
	    BIO_free(bio);
	    if (sycSSL_CTX_set_tmp_dh(*ctx, dh) <= 0) {
	       while (err = ERR_get_error()) {
		  Warn3("SSL_CTX_set_tmp_dh(%p, %p): %s", *ctx, dh,
			ERR_error_string(err, NULL));
	       }
	       Error2("SSL_CTX_set_tmp_dh(%p, %p): error", *ctx, dh);
	    }
	 }
      }
   }

   /* set pre ssl-connect options */
   /* SSL_CIPHERS */
   if (ci_str != NULL) {
      if (sycSSL_CTX_set_cipher_list(*ctx, ci_str) <= 0) {
	 if (ERR_peek_error() == 0)
	    Error1("SSL_set_cipher_list(, \"%s\") failed", ci_str);
	 while (err = ERR_get_error()) {
	    Error2("SSL_set_cipher_list(, \"%s\"): %s",
		   ci_str, ERR_error_string(err, NULL));
	 }
	 /*Error("SSL_new()");*/
	 return STAT_RETRYLATER;
      }
   }

   if (*opt_ver) {
      sycSSL_CTX_set_verify(*ctx,
			    SSL_VERIFY_PEER| SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
			    NULL);
   } else {
      sycSSL_CTX_set_verify(*ctx,
			    SSL_VERIFY_NONE,
			    NULL);
   }

   return STAT_OK;
}


/* analyses an OpenSSL error condition, prints the appropriate messages with
   severity 'level' and returns one of STAT_OK, STAT_RETRYLATER, or
   STAT_NORETRY */
static int openssl_SSL_ERROR_SSL(int level, const char *funcname) {
   unsigned long e;
   char buf[120];	/* this value demanded by "man ERR_error_string" */
   int stat = STAT_OK;

   while (e = ERR_get_error()) {
      Debug1("ERR_get_error(): %lx", e);
      if (e == ((ERR_LIB_RAND<<24)|
		(RAND_F_SSLEAY_RAND_BYTES<<12)|
		(RAND_R_PRNG_NOT_SEEDED)) /*0x24064064*/) {
	 Error("too few entropy; use options \"egd\" or \"pseudo\"");
	 stat = STAT_NORETRY;
      } else {
	 Msg2(level, "%s(): %s", funcname, ERR_error_string(e, buf));
	 stat =  level==E_ERROR ? STAT_NORETRY : STAT_RETRYLATER;
      }
   }
   return stat;
}

static const char *openssl_verify_messages[] = {
   /*  0 */ "ok",
   /*  1 */ NULL,
   /*  2 */ "unable to get issuer certificate",
   /*  3 */ "unable to get certificate CRL",
   /*  4 */ "unable to decrypt certificate's signature",
   /*  5 */ "unable to decrypt CRL's signature",
   /*  6 */ "unable to decode issuer public key",
   /*  7 */ "certificate signature failure",
   /*  8 */ "CRL signature failure",
   /*  9 */ "certificate is not yet valid",
   /* 10 */ "certificate has expired",
   /* 11 */ "CRL is not yet valid",
   /* 12 */ "CRL has expired",
   /* 13 */ "format error in certificate's notBefore field",
   /* 14 */ "format error in certificate's notAfter field",
   /* 15 */ "format error in CRL's lastUpdate field",
   /* 16 */ "format error in CRL's nextUpdate field",
   /* 17 */ "out of memory",
   /* 18 */ "self signed certificate",
   /* 19 */ "self signed certificate in certificate chain",
   /* 20 */ "unable to get local issuer certificate",
   /* 21 */ "unable to verify the first certificate",
   /* 22 */ "certificate chain too long",
   /* 23 */ "certificate revoked",
   /* 24 */ "invalid CA certificate",
   /* 25 */ "path length constraint exceeded",
   /* 26 */ "unsupported certificate purpose",
   /* 27 */ "certificate not trusted",
   /* 28 */ "certificate rejected",
   /* 29 */ "subject issuer mismatch",
   /* 30 */ "authority and subject key identifier mismatch",
   /* 31 */ "authority and issuer serial number mismatch",
   /* 32 */ "key usage does not include certificate signing",
   /* 33 */ NULL,
   /* 34 */ NULL,
   /* 35 */ NULL,
   /* 36 */ NULL,
   /* 37 */ NULL,
   /* 38 */ NULL,
   /* 39 */ NULL,
   /* 40 */ NULL,
   /* 41 */ NULL,
   /* 42 */ NULL,
   /* 43 */ NULL,
   /* 44 */ NULL,
   /* 45 */ NULL,
   /* 46 */ NULL,
   /* 47 */ NULL,
   /* 48 */ NULL,
   /* 49 */ NULL,
   /* 50 */ "application verification failure",
} ;


/* delete all environment variables whose name begins with SOCAT_OPENSSL_
   resp. <progname>_OPENSSL_ */
static int openssl_delete_cert_info(void) {
#  define XIO_ENVNAMELEN 256
   const char *progname;
   char envprefix[XIO_ENVNAMELEN];
   char envname[XIO_ENVNAMELEN];
   size_t i, l;
   const char **entry;

   progname = diag_get_string('p');
   envprefix[0] = '\0'; strncat(envprefix, progname, XIO_ENVNAMELEN-1);
   l = strlen(envprefix);
   for (i = 0; i < l; ++i)  envprefix[i] = toupper(envprefix[i]);
   strncat(envprefix+l, "_OPENSSL_", XIO_ENVNAMELEN-l-1);

#if HAVE_VAR_ENVIRON
   entry = (const char **)environ;
   while (*entry != NULL) {
      if (!strncmp(*entry, envprefix, strlen(envprefix))) {
	 const char *eq = strchr(*entry, '=');
	 if (eq == NULL)  eq = *entry + strlen(*entry);
	 envname[0] = '\0'; strncat(envname, *entry, eq-*entry);
	 Unsetenv(envname);
      } else {
	 ++entry;
      }
   }
#endif /* HAVE_VAR_ENVIRON */
   return 0;
}

/* compares the peername used/provided by the client to cn as extracted from
   the peer certificate.
   supports wildcard cn like *.domain which matches domain and
   host.domain
   returns true on match */
static bool openssl_check_name(const char *cn, const char *peername) {
   const char *dotp;
   if (peername == NULL) {
      Info1("commonName \"%s\": no peername", cn);
      return false;
   } else if (peername[0] == '\0') {
      Info1("commonName \"%s\": matched by empty peername", cn);
      return true;
   }
   if (! (cn[0] == '*' && cn[1] == '.')) {
      /* normal server name - this is simple */
      Debug1("commonName \"%s\" has no wildcard", cn);
      if (strcmp(cn, peername) == 0) {
	 Debug2("commonName \"%s\" matches peername \"%s\"", cn, peername);
	 return true;
      } else {
	 Info2("commonName \"%s\" does not match peername \"%s\"", cn, peername);
	 return false;
      }
   }
   /* wildcard cert */
   Debug1("commonName \"%s\" is a wildcard name", cn);
   /* case: just the base domain */
   if (strcmp(cn+2, peername) == 0) {
      Debug2("wildcard commonName \"%s\" matches base domain \"%s\"", cn, peername);
      return true;
   }
   /* case: subdomain; only one level! */
   dotp = strchr(peername, '.');
   if (dotp == NULL) {
      Info2("peername \"%s\" is not a subdomain, thus is not matched by wildcard commonName \"%s\"",
	    peername, cn);
      return false;
   }
   if (strcmp(cn+1, dotp) != 0) {
      Info2("commonName \"%s\" does not match subdomain peername \"%s\"", cn, peername);
      return false;
   }
   Debug2("commonName \"%s\" matches subdomain peername \"%s\"", cn, peername);
   return true;
}

/* retrieves the commonName field and compares it to the peername
   returns true on match, false otherwise */
static bool openssl_check_peername(X509_NAME *name, const char *peername) {
   int ind = -1;
   X509_NAME_ENTRY *entry;
   ASN1_STRING *data;
   unsigned char *text;
   ind = X509_NAME_get_index_by_NID(name, NID_commonName, -1);
   if (ind < 0) {
      Info("no COMMONNAME field in peer certificate"); 
      return false;
   }
   entry = X509_NAME_get_entry(name, ind);
   data = X509_NAME_ENTRY_get_data(entry);
   text = ASN1_STRING_data(data);
   return openssl_check_name((const char *)text, peername);
}

/* retrieves certificate provided by peer, sets env vars containing
   certificates field values, and checks peername if provided by
   calling function */
/* parts of this code were copied from Gene Spaffords C/C++ Secure Programming at Etutorials.org:
   http://etutorials.org/Programming/secure+programming/Chapter+10.+Public+Key+Infrastructure/10.8+Adding+Hostname+Checking+to+Certificate+Verification/
   The code examples in this tutorial do not seem to have explicit license restrictions.
*/

/* read in the "name" information (from field "issuer" or "subject") and
   create environment variable with complete info, eg:
   SOCAT_OPENSSL_X509_SUBJECT */
static int openssl_setenv_cert_name(const char *field, X509_NAME *name) {
   BIO *bio = BIO_new(BIO_s_mem());
   char *buf = NULL, *str;
   size_t len;
   X509_NAME_print_ex(bio, name, 0, XN_FLAG_ONELINE&~ASN1_STRFLGS_ESC_MSB);	/* rc not documented */
   len = BIO_get_mem_data (bio, &buf);
   if ((str = Malloc(len+1)) == NULL) {
      BIO_free(bio);
      return -1;
   }
   memcpy(str, buf, len);
   str[len] = '\0';
   Info2("SSL peer cert %s: \"%s\"", field, buf);
   xiosetenv2("OPENSSL_X509", field, str, 1, NULL);
   free(str);
   BIO_free(bio);
   return 0;
}

/* read in the "name" information (from field "issuer" or "subject") and
   create environment variables with the fields, eg:
   SOCAT_OPENSSL_X509_COMMONNAME
*/
static int openssl_setenv_cert_fields(const char *field, X509_NAME *name) {
   int n, i;
   n = X509_NAME_entry_count(name);
   /* extract fields of cert name */
   for (i = 0; i < n; ++i) {
      X509_NAME_ENTRY *entry;
      ASN1_OBJECT *obj;
      ASN1_STRING *data;
      unsigned char *text;
      int nid;
      entry = X509_NAME_get_entry(name, i);
      obj  = X509_NAME_ENTRY_get_object(entry);
      data = X509_NAME_ENTRY_get_data(entry);
      nid  = OBJ_obj2nid(obj);
      text = ASN1_STRING_data(data);
      Debug3("SSL peer cert %s entry: %s=\"%s\"", (field[0]?field:"subject"), OBJ_nid2ln(nid), text);
      if (field != NULL && field[0] != '\0') {
	 xiosetenv3("OPENSSL_X509", field, OBJ_nid2ln(nid), (const char *)text, 2, " // ");
      } else {
	 xiosetenv2("OPENSSL_X509", OBJ_nid2ln(nid), (const char *)text, 2, " // ");
      }
   }
   return 0;
}

static int openssl_handle_peer_certificate(struct single *xfd,
					   const char *peername,
					   bool opt_ver, int level) {
   X509 *peer_cert;
   X509_NAME *subjectname, *issuername;
   /*ASN1_TIME not_before, not_after;*/
   int extcount, i, ok = 0;
   int status;

   if ((peer_cert = SSL_get_peer_certificate(xfd->para.openssl.ssl)) == NULL) {
      if (opt_ver) {
	 Msg(level, "no peer certificate");
	 status = STAT_RETRYLATER;
      } else {
	 Notice("no peer certificate and no check");
	 status = STAT_OK;
      }
      return status;
   }

   /* verify peer certificate (trust, signature, validity dates) */
   if (opt_ver) {
      long verify_result;
      if ((verify_result = sycSSL_get_verify_result(xfd->para.openssl.ssl)) != X509_V_OK) {
	 const char *message = NULL;
	 if (verify_result >= 0 &&
	     (size_t)verify_result <
	     sizeof(openssl_verify_messages)/sizeof(char*)) {
	    message = openssl_verify_messages[verify_result];
	 }
	 if (message) {
	    Msg1(level, "%s", message);
	 } else {
	    Msg1(level, "rejected peer certificate with error %ld", verify_result);
	 }
	 status = STAT_RETRYLATER;
	 X509_free(peer_cert);
	 return STAT_RETRYLATER;
      }
      Info("peer certificate is trusted");
   }

   /* set env vars from cert's subject and issuer values */
   if ((subjectname = X509_get_subject_name(peer_cert)) != NULL) {
      openssl_setenv_cert_name("subject", subjectname);
      openssl_setenv_cert_fields("", subjectname);
      /*! I'd like to provide dates too; see
	http://markmail.org/message/yi4vspp7aeu3xwtu#query:+page:1+mid:jhnl4wklif3pgzqf+state:results */
   }
   if ((issuername = X509_get_issuer_name(peer_cert)) != NULL) {
      openssl_setenv_cert_name("issuer", issuername);
   }

   /* check peername against cert's subjectAltName DNS entries */
   /* this code is based on example from Gerhard Gappmeier in
      http://openssl.6102.n7.nabble.com/How-to-extract-subjectAltName-td17236.html
   */
   if ((extcount = X509_get_ext_count(peer_cert)) > 0) {
      for (i = 0;  !ok && i < extcount;  ++i) {
	 const char            *extstr;
	 X509_EXTENSION        *ext;
	 const X509V3_EXT_METHOD     *meth;
	 ext = X509_get_ext(peer_cert, i);
	 extstr = OBJ_nid2sn(OBJ_obj2nid(X509_EXTENSION_get_object(ext)));
	 if (!strcasecmp(extstr, "subjectAltName")) {
	    void *names;
	    if (!(meth = X509V3_EXT_get(ext))) break;   
	    names = X509_get_ext_d2i(peer_cert, NID_subject_alt_name, NULL, NULL);
	    if (names) {
	       int numalts;
	       int i;

	       /* get amount of alternatives, RFC2459 claims there MUST be at least one, but we don't depend on it... */
	       numalts = sk_GENERAL_NAME_num ( names );
	       /* loop through all alternatives */
	       for ( i=0; ( i<numalts ); i++ ) {
		  /* get a handle to alternative name number i */
		  const GENERAL_NAME *pName = sk_GENERAL_NAME_value (names, i );
		  unsigned char *pBuffer;
		  switch ( pName->type ) {

		  case GEN_DNS:
		     ASN1_STRING_to_UTF8(&pBuffer, pName->d.ia5);
		     xiosetenv("OPENSSL_X509V3_SUBJECTALTNAME_DNS", (char *)pBuffer, 2, " // ");
		     if (peername != NULL &&
			 openssl_check_name((char *)pBuffer, /*const char*/peername)) {
			ok = 1;
		     }
		     OPENSSL_free(pBuffer);
		     break;

		  default: continue;
		  }
	       }
	    }
	 }
      }
   }

   if (!opt_ver) {
      Notice("option openssl-verify disabled, no check of certificate");
      X509_free(peer_cert);
      return STAT_OK;
   }
   if (peername == NULL || peername[0] == '\0') {
      Notice("trusting certificate, no check of commonName");
      X509_free(peer_cert);
      return STAT_OK;
   }
   if (ok) {
      Notice("trusting certificate, commonName matches");
      X509_free(peer_cert);
      return STAT_OK;
   }

   /* here: all envs set; opt_ver, cert verified, no subjAltName match -> check subject CN */
   if (!openssl_check_peername(/*X509_NAME*/subjectname, /*const char*/peername)) {
      Error("certificate is valid but its commonName does not match hostname");
      status = STAT_NORETRY;
   } else {
      Notice("trusting certificate, commonName matches");
      status = STAT_OK;
   }
   X509_free(peer_cert);
   return status;
}

static int xioSSL_set_fd(struct single *xfd, int level) {
   unsigned long err;

   /* assign a network connection to the SSL object */
  if (xfd->rfd == xfd->wfd) {
   if (sycSSL_set_fd(xfd->para.openssl.ssl, xfd->rfd) <= 0) {
      Msg(level, "SSL_set_fd() failed");
      while (err = ERR_get_error()) {
	 Msg2(level, "SSL_set_fd(, %d): %s",
	      xfd->wfd, ERR_error_string(err, NULL));
      }
      return STAT_RETRYLATER;
   }
  } else {
      if (xfd->rfd >= 0) {
	 if (sycSSL_set_rfd(xfd->para.openssl.ssl, xfd->rfd) <= 0) {
	    Msg(level, "SSL_set_rfd() failed");
	    while (err = ERR_get_error()) {
	       Msg2(level, "SSL_set_rfd(, %d): %s",
		    xfd->rfd, ERR_error_string(err, NULL));
	    }
	    return STAT_RETRYLATER;
	 }
      }
      if (xfd->wfd >= 0) {
	 if (sycSSL_set_wfd(xfd->para.openssl.ssl, xfd->wfd) <= 0) {
	    Msg(level, "SSL_set_wfd() failed");
	    while (err = ERR_get_error()) {
	       Msg2(level, "SSL_set_wfd(, %d): %s",
		    xfd->wfd, ERR_error_string(err, NULL));
	    }
	    return STAT_RETRYLATER;
	 }
      }
   }  
   return STAT_OK;
}


/* ...
   in case of an error condition, this function check forever and retry
   options and ev. sleeps an interval. It returns NORETRY when the caller
   should not retry for any reason. */
static int xioSSL_connect(struct single *xfd, const char *opt_commonname,
			  bool opt_ver, int level) {
   char error_string[120];
   int errint, status, ret;
   unsigned long err;

   /* connect via SSL by performing handshake */
   if ((ret = sycSSL_connect(xfd->para.openssl.ssl)) <= 0) {
      /*if (ERR_peek_error() == 0) Msg(level, "SSL_connect() failed");*/
      errint = SSL_get_error(xfd->para.openssl.ssl, ret);
      switch (errint) {
      case SSL_ERROR_NONE:
	 /* this is not an error, but I dare not continue for security reasons*/
	 Msg(level, "ok");
	 status = STAT_RETRYLATER;
      case SSL_ERROR_ZERO_RETURN:
	 Msg(level, "connection closed (wrong version number?)");
	 status = STAT_RETRYLATER;
	 break;
      case SSL_ERROR_WANT_READ:
      case SSL_ERROR_WANT_WRITE:
      case SSL_ERROR_WANT_CONNECT:
      case SSL_ERROR_WANT_X509_LOOKUP:
	 Msg(level, "nonblocking operation did not complete");
	 status = STAT_RETRYLATER;
	 break;	/*!*/
      case SSL_ERROR_SYSCALL:
	 if (ERR_peek_error() == 0) {
	    if (ret == 0) {
	       Msg(level, "SSL_connect(): socket closed by peer");
	    } else if (ret == -1) {
	       Msg1(level, "SSL_connect(): %s", strerror(errno));
	    }
	 } else {
	    Msg(level, "I/O error");	/*!*/
	    while (err = ERR_get_error()) {
	       ERR_error_string_n(err, error_string, sizeof(error_string));
	       Msg4(level, "SSL_connect(): %s / %s / %s / %s", error_string,
		    ERR_lib_error_string(err), ERR_func_error_string(err),
		    ERR_reason_error_string(err));
	    }
	 }
	 status = STAT_RETRYLATER;
	 break;
      case SSL_ERROR_SSL:
	 status = openssl_SSL_ERROR_SSL(level, "SSL_connect");
	 if (openssl_handle_peer_certificate(xfd, opt_commonname, opt_ver, level/*!*/) < 0) {
	    return STAT_RETRYLATER;
	 }
	 break;
      default:
	 Msg(level, "unknown error");
	 status = STAT_RETRYLATER;
	 break;
      }
      return status;
   }
   return STAT_OK;
}

/* on result < 0: errno is set (at least to EIO) */
ssize_t xioread_openssl(struct single *pipe, void *buff, size_t bufsiz) {
   unsigned long err;
   char error_string[120];
   int _errno = EIO;	/* if we have no better idea about nature of error */
   int errint, ret;

   ret = sycSSL_read(pipe->para.openssl.ssl, buff, bufsiz);
   if (ret < 0) {
      errint = SSL_get_error(pipe->para.openssl.ssl, ret);
      switch (errint) {
      case SSL_ERROR_NONE:
	 /* this is not an error, but I dare not continue for security reasons*/
	 Error("ok");
	 break;
      case SSL_ERROR_ZERO_RETURN:
	 Error("connection closed by peer");
	 break;
      case SSL_ERROR_WANT_READ:
      case SSL_ERROR_WANT_WRITE:
      case SSL_ERROR_WANT_CONNECT:
      case SSL_ERROR_WANT_X509_LOOKUP:
	 Info("nonblocking operation did not complete");
	 errno = EAGAIN;
	 return -1;
      case SSL_ERROR_SYSCALL:
	 if (ERR_peek_error() == 0) {
	    if (ret == 0) {
	       Error("SSL_read(): socket closed by peer");
	    } else if (ret == -1) {
	       _errno = errno;
	       Error1("SSL_read(): %s", strerror(errno));
	    }
	 } else {
	    Error("I/O error");	/*!*/
	    while (err = ERR_get_error()) {
	       ERR_error_string_n(err, error_string, sizeof(error_string));
	       Error4("SSL_read(): %s / %s / %s / %s", error_string,
		      ERR_lib_error_string(err), ERR_func_error_string(err),
		      ERR_reason_error_string(err));
	    }
	 }
	 break;
      case SSL_ERROR_SSL:
	 openssl_SSL_ERROR_SSL(E_ERROR, "SSL_connect");
	 break;
      default:
	 Error("unknown error");
	 break;
      }
      errno = _errno;
      return -1;
   }
   return ret;
}

ssize_t xiopending_openssl(struct single *pipe) {
   int bytes = sycSSL_pending(pipe->para.openssl.ssl);
   return bytes;
}

/* on result < 0: errno is set (at least to EIO) */
ssize_t xiowrite_openssl(struct single *pipe, const void *buff, size_t bufsiz) {
   unsigned long err;
   char error_string[120];
   int _errno = EIO;	/* if we have no better idea about nature of error */
   int errint, ret;

   ret = sycSSL_write(pipe->para.openssl.ssl, buff, bufsiz);
   if (ret < 0) {
      errint = SSL_get_error(pipe->para.openssl.ssl, ret);
      switch (errint) {
      case SSL_ERROR_NONE:
	 /* this is not an error, but I dare not continue for security reasons*/
	 Error("ok");
      case SSL_ERROR_ZERO_RETURN:
	 Error("connection closed by peer");
	 break;
      case SSL_ERROR_WANT_READ:
      case SSL_ERROR_WANT_WRITE:
      case SSL_ERROR_WANT_CONNECT:
      case SSL_ERROR_WANT_X509_LOOKUP:
	 Error("nonblocking operation did not complete");
	 break;	/*!*/
      case SSL_ERROR_SYSCALL:
	 if (ERR_peek_error() == 0) {
	    if (ret == 0) {
	       Error("SSL_write(): socket closed by peer");
	    } else if (ret == -1) {
	       _errno = errno;
	       Error1("SSL_write(): %s", strerror(errno));
	    }
	 } else {
	    Error("I/O error");	/*!*/
	    while (err = ERR_get_error()) {
	       ERR_error_string_n(err, error_string, sizeof(error_string));
	       Error4("SSL_write(): %s / %s / %s / %s", error_string,
		      ERR_lib_error_string(err), ERR_func_error_string(err),
		      ERR_reason_error_string(err));
	    }
	 }
	 break;
      case SSL_ERROR_SSL:
	 openssl_SSL_ERROR_SSL(E_ERROR, "SSL_connect");
	 break;
      default:
	 Error("unknown error");
	 break;
      }
      errno = _errno;
      return -1;
   }
   return ret;
}


#endif /* WITH_OPENSSL */
