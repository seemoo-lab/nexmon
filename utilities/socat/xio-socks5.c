/* source: xio-socks5.c */
/* Copyright Gerhard Rieger */
/* Published under the GNU General Public License V.2, see file COPYING */

/* this file contains the source for opening addresses of socks5 type */

#include "xiosysincludes.h"
#include "xioopen.h"
#include "xio-socket.h"
#include "xio-ipapp.h"

#include "xio-socks5.h"


#if WITH_SOCKS5

#define SOCKSPORT "1080"
#define SOCKS5_MAXLEN 512

static int 
   xioopen_socks5_client(int argc, const char *argv[],
			  struct opt *opts, int xioflags, xiofile_t *xfd,
			  unsigned groups, int dummy1, int dummy2,
			  int dummy3);

const struct optdesc opt_socks5_username  = { "socks5-username",  "socks5user", OPT_SOCKS5_USERNAME,  GROUP_SOCKS5, PH_LATE, TYPE_STRING,  OFUNC_SPEC };
const struct optdesc opt_socks5_password  = { "socks5-password",  "socks5pass", OPT_SOCKS5_PASSWORD,  GROUP_SOCKS5, PH_LATE, TYPE_STRING,  OFUNC_SPEC };

static const struct xioaddr_inter_desc xiointer_socks5_client = { XIOADDR_PROT, "socks5", 2, XIOBIT_ALL, GROUP_SOCKS5, XIOSHUT_DOWN, XIOCLOSE_CLOSE, xioopen_socks5_client, 0, 0, 0, XIOBIT_RDWR HELP(":<host>:<port>") };
const union xioaddr_desc *xioaddrs_socks5_client[] = {
   (union xioaddr_desc *)&xiointer_socks5_client, NULL };

/* read until buflen bytes received or EOF */
/* returns STAT_OK, STAT_RETRYLATER, or STAT_NOTRETRY */
static int xiosocks5_recvbytes(struct single *xfd,
			       uint8_t *buff, size_t buflen, int level) {
   size_t bytes;
   ssize_t result;

   bytes = 0;
   while (bytes >= 0) {	/* loop over answer chunks until complete or error */
      /* receive socks answer */
      Debug("waiting for data from peer");
      do {
	 result = Read(xfd->rfd, buff+bytes, buflen-bytes);
      } while (result < 0 && errno == EINTR);
      if (result < 0) {
	 Msg4(level, "read(%d, %p, "F_Zu"): %s",
	      xfd->rfd, buff+bytes, buflen-bytes,
	      strerror(errno));
	 if (Close(xfd->rfd) < 0) {
	    Warn2("close(%d): %s", xfd->rfd, strerror(errno));
	 }
	 if (Close(xfd->wfd) < 0) {
	    Warn2("close(%d): %s", xfd->wfd, strerror(errno));
	 }
	 return STAT_RETRYLATER;
      }
      if (result == 0) {
	 Msg(level, "read(): EOF during read of socks reply");
	 if (Close(xfd->rfd) < 0) {
	    Warn2("close(%d): %s", xfd->rfd, strerror(errno));
	 }
	 if (Close(xfd->wfd) < 0) {
	    Warn2("close(%d): %s", xfd->wfd, strerror(errno));
	 }
	 return STAT_RETRYLATER;
      }
      bytes += result;
      if (bytes == buflen) {
	 Debug1("received all "F_Zd" bytes", bytes);
	 break;
      }
      Debug2("received "F_Zd" bytes, waiting for "F_Zu" more bytes",
	     result, buflen-bytes);
   }
   if (result <= 0) {	/* we had a problem while reading socks answer */
      return STAT_RETRYLATER;	/* retry complete open cycle */
   }
   return STAT_OK;
}

static int xioopen_socks5_client(int argc, const char *argv[],
				 struct opt *opts, int xioflags,
				 xiofile_t *xxfd,
				 unsigned groups, int dummy1, int dummy2,
				 int dummy3) {
   /* we expect the form: host:port */
   xiosingle_t *xfd = &xxfd->stream;
   const char *targetname, *targetservice;
   int level;
   int result;

   if (argc != 3) {
      Error("SOCKS5 syntax: socks5-connect:<host>:<port>");	/*! UDP? */
      return STAT_NORETRY;
   }

   targetname = argv[1];
   targetservice = argv[2];

   applyopts(-1, opts, PH_INIT);
   applyopts_single(xfd, opts, PH_INIT);

#if 0
   result = _xioopen_socks5_prepare(a3, opts, &socksport, sockhead, &buflen);
   if (result != STAT_OK)  return result;
#endif

   if (xfd->rfd < 0) {
      Error("socks5 must be used as embedded address");
      return -1;
   }

   Notice2("opening connection to %s:%s using socks5",
	   targetname, targetservice);

   do {	/* loop over failed connect and socks-request attempts */

#if WITH_RETRY
      if (xfd->forever || xfd->retry) {
	 level = E_INFO;
      } else
#endif /* WITH_RETRY */
	 level = E_ERROR;

#if 0
      /* we try to resolve the target address _before_ connecting to the socks
	 server: this avoids unnecessary socks connects and timeouts */
      result =
	 _xioopen_socks5_connect0(xfd, targetname, sockhead, opts,
				  (ssize_t *)&buflen, level);
      switch (result) {
      case STAT_OK: break;
#if WITH_RETRY
      case STAT_RETRYLATER:
      case STAT_RETRYNOW:
	 if (xfd->forever || xfd->retry--) {
	    if (result == STAT_RETRYLATER)  Nanosleep(&xfd->intervall, NULL);
	    continue;
	 }
#endif /* WITH_RETRY */
      default:
	 return result;
      }
#endif

#if 0
     if (xfd->rfd < 0) {
      /* this cannot fork because we retrieved fork option above */
      result =
	 _xioopen_connect (xfd,
			   needbind?(struct sockaddr *)us:NULL, sizeof(*us),
			   (struct sockaddr *)them, sizeof(struct sockaddr_in),
			   opts, PF_INET, socktype, IPPROTO_TCP, lowport, level);
      switch (result) {
      case STAT_OK: break;
#if WITH_RETRY
      case STAT_RETRYLATER:
      case STAT_RETRYNOW:
	 if (xfd->forever || xfd->retry--) {
	    if (result == STAT_RETRYLATER)  Nanosleep(&xfd->intervall, NULL);
	    continue;
	 }
#endif /* WITH_RETRY */
      default:
	 return result;
      }
      xfd->rfd = xfd->wfd = xfd->fd;
     } else
#endif
	xfd->dtype = XIODATA_STREAM;

      result =
	_xioopen_socks5_connect(xfd, targetname, targetservice, opts, level);
      switch (result) {
      case STAT_OK: break;
#if WITH_RETRY
      case STAT_RETRYLATER:
      case STAT_RETRYNOW:
	 if (xfd->forever || xfd->retry--) {
	    if (result == STAT_RETRYLATER)  Nanosleep(&xfd->intervall, NULL);
	    continue;
	 }
#endif /* WITH_RETRY */
      default:
	 return result;
      }

      /*!*/
      applyopts(xfd->rfd, opts, PH_ALL);

      if ((result = _xio_openlate(xfd, opts)) < 0)
	 return result;

      {
	 break;
      }

   } while (true);	/* end of complete open loop - drop out on success */
   return 0;
}



/* perform socks5 client dialog on existing FD.
   Called within fork/retry loop, after connect() */
int _xioopen_socks5_connect(struct single *xfd,
			    const char *targetname, const char *targetservice,
			    struct opt *opts, int level) {
   uint16_t targetport;
   unsigned char sendbuff[SOCKS5_MAXLEN];  size_t sendlen;
   unsigned char recvbuff[SOCKS5_MAXLEN];
   struct socks5_method  *sendmethod;
   struct socks5_select  *recvselect;
   struct socks5_request *sendrequest;
   struct socks5_reply   *recvreply;
   size_t namelen;
   size_t addrlen;
   size_t readpos;
   char *emsg;
   int result;

   /* prepare */
   targetport    = parseport(targetservice, IPPROTO_TCP/*!*/);

   /* just the simplest authentications */
   sendmethod = (struct socks5_method *)sendbuff;
   sendmethod->version = 5;	/* protocol version */
   sendmethod->nmethods = 2;	/* number of proposed authentication types */
   sendmethod->methods[0] = SOCKS5_METHOD_NOAUTH;	/* no auth at all */
   sendmethod->methods[1] = SOCKS5_METHOD_USERPASS;	/* username/password */
   /*sendmethod->methods[2] = SOCKS5_METHOD_AVENTAIL;*/	/* Aventail Connect */
   sendlen = 2+sendmethod->nmethods;

   /* send socks header (target addr+port, +auth) */
   Info("sending socks5 identifier/method selection message");
   do {
      result = Write(xfd->wfd, sendmethod, sendlen);
   } while (result < 0 && errno == EINTR);
   if (result < 0) {
      Msg4(level, "write(%d, %p, "F_Zu"): %s",
	   xfd->wfd, sendmethod, sendlen, strerror(errno));
      if (Close(xfd->wfd) < 0) {
	 Warn2("close(%d): %s", xfd->wfd, strerror(errno));
      }
      if (Close(xfd->rfd) < 0) {
	 Warn2("close(%d): %s", xfd->rfd, strerror(errno));
      }
      return STAT_RETRYLATER;	/* retry complete open cycle */
   }

   Info1("waiting for socks5 select reply ("F_Zu" bytes)", SOCKS5_SELECT_LENGTH);
   if ((result =
	xiosocks5_recvbytes(xfd, recvbuff, SOCKS5_SELECT_LENGTH, level)) !=
       STAT_OK) {
      /* we had a problem while reading socks answer */
      /*! Close(); */
      return result;	/* ev. retry complete open cycle */
   }
   recvselect = (struct socks5_select *)recvbuff;
   Info2("socks5 select: {%u, %u}", recvselect->version, recvselect->method);
   if (recvselect->version != 5) {
      Error1("socks5: server protocol version is %u",
	     recvbuff[0]);
   }
   if (recvselect->method == SOCKS5_METHOD_NONE) {
      Error("socks5: server did not accept our authentication methods");
   }
   /*! check if selected methods is one of our proposals */

   switch (recvselect->method) {
   case SOCKS5_METHOD_NOAUTH:
      break;
   case SOCKS5_METHOD_USERPASS:
      if (xio_socks5_username_password(level, opts, xfd) < 0) {
	 Error("username/password not accepted");
      }
      break;
   default:
      Error("socks5 select: unimplemented authentication method selected");
      break;
   }

   sendrequest = (struct socks5_request *)sendbuff;
   sendrequest->version = SOCKS5_VERSION;
   sendrequest->command = SOCKS5_COMMAND_CONNECT;
   sendrequest->reserved = 0;
   sendrequest->addrtype = SOCKS5_ADDRTYPE_NAME;
   switch (sendrequest->addrtype) {
   case SOCKS5_ADDRTYPE_IPV4:
      break;
   case SOCKS5_ADDRTYPE_NAME:
      if ((namelen = strlen(targetname)) > 255) {
	 Error1("socks5: target name is longer than 255 bytes: \"%s\"",
		targetname);
      }
      sendrequest->destaddr[0] = strlen(targetname);
      *(sendrequest->destaddr+1) = '\0'; strncat(sendrequest->destaddr+1, targetname, sizeof(sendbuff)-5);
      break;
   case SOCKS5_ADDRTYPE_IPV6:
      break;
   default: Fatal("socks5: undefined address type in socks request");
   }
   *((uint16_t *)&sendrequest->destaddr[strlen(targetname)+1]) = targetport;
   sendlen = sizeof(struct socks5_request) + namelen + 2;

   /* send socks request (target addr+port, +auth) */
   Info("sending socks5 request selection");
   do {
      result = Write(xfd->wfd, sendrequest, sendlen);
   } while (result < 0 && errno == EINTR);
   if (result < 0) {
      Msg4(level, "write(%d, %p, "F_Zu"): %s",
	   xfd->wfd, sendmethod, sendlen, strerror(errno));
      if (Close(xfd->wfd) < 0) {
	 Warn2("close(%d): %s", xfd->wfd, strerror(errno));
      }
      if (Close(xfd->rfd) < 0) {
	 Warn2("close(%d): %s", xfd->rfd, strerror(errno));
      }
      return STAT_RETRYLATER;	/* retry complete open cycle */
   }

   Info("waiting for socks5 reply");
   recvreply = (struct socks5_reply *)recvbuff;
   if ((result =
	xiosocks5_recvbytes(xfd, recvbuff, SOCKS5_REPLY_LENGTH1, level))
       != STAT_OK) {
      /*! close... */
      return result;
   }
   if (recvreply->version != SOCKS5_VERSION) {
      Error1("socks5: version of reply is %d", recvreply->version);
   }
   switch (recvreply->reply) {
   case SOCKS5_REPLY_SUCCESS:     emsg = NULL; break;
   case SOCKS5_REPLY_FAILURE:     emsg = "general socks server failure"; break;
   case SOCKS5_REPLY_DENIED:      emsg = "connection not allowed by ruleset"; break;
   case SOCKS5_REPLY_NETUNREACH:  emsg = "network unreachable"; break;
   case SOCKS5_REPLY_HOSTUNREACH: emsg = "host unreachable"; break;
   case SOCKS5_REPLY_REFUSED:     emsg = "connection refused"; break;
   case SOCKS5_REPLY_TTLEXPIRED:  emsg = "TTL expired"; break;
   case SOCKS5_REPLY_CMDUNSUPP:   emsg = "command not supported"; break;
   case SOCKS5_REPLY_ADDRUNSUPP:  emsg = "address type not supported"; break;
   default:                       emsg = "undefined error code"; break;
   }
   if (recvreply->reply != SOCKS5_REPLY_SUCCESS) {
      Error1("socks5 reply: %s", emsg);
   }
   
   switch (recvreply->addrtype) {
   case SOCKS5_ADDRTYPE_IPV4:
      addrlen = 4;	/*!*/
      if ((result =
	   xiosocks5_recvbytes(xfd, recvbuff+SOCKS5_REPLY_LENGTH1, addrlen,
			       level))
	   != STAT_OK) {
	 /*! close... */
	 return result;
      }
      readpos = sizeof(recvreply)-1+4;
      break;
   case SOCKS5_ADDRTYPE_NAME:
      /* read 1 byte containing domain name length */
      if ((result =
	   xiosocks5_recvbytes(xfd, recvbuff+SOCKS5_REPLY_LENGTH1, 1, level))
	  != STAT_OK) {
	 /*! close... */
	 return result;
      }
      /* read domain name */
      if ((result =
	   xiosocks5_recvbytes(xfd, recvbuff+SOCKS5_REPLY_LENGTH1+1,
			       recvreply->destaddr[0], level))
	  != STAT_OK) {
	 /*! close... */
	 return result;
      }
      readpos = sizeof(recvreply)+recvreply->destaddr[0];
      addrlen = recvreply->destaddr[0]+1;
      break;
   case SOCKS5_ADDRTYPE_IPV6:
      addrlen = 16;	/*!*/
      if ((result =
	   xiosocks5_recvbytes(xfd, recvbuff+SOCKS5_REPLY_LENGTH1, addrlen,
			      level))
	  == EOF) {
	 /*! close... */
	 return result;
      }
      readpos = sizeof(recvreply)-1+4;
      break;
   default: Error("socks5: undefined address type in answer");
      addrlen = 0;
      readpos = sizeof(recvreply)-1;
   }
   if ((result =
	xiosocks5_recvbytes(xfd, recvbuff+readpos, 2, level))
        != STAT_OK) {
      /*! close... */
      return result;
   }

   /*! Infos(...); */
   return STAT_OK;
}

int xio_socks5_username_password(int level, struct opt *opts,
				 struct single *xfd) {
   /*!!! */
   unsigned char sendbuff[513];
   unsigned char *pos;
   char *username = NULL;
   char *password = NULL;
   unsigned char recvbuff[2];
   struct socks5_userpass_reply *reply;
   int result;

   retropt_string(opts, OPT_SOCKS5_USERNAME, (char **)&username);
   if (username == NULL) {
      Error("socks5: username required");
      return STAT_NORETRY;
   }
   retropt_string(opts, OPT_SOCKS5_PASSWORD, (char **)&password);
   if (password == NULL) {
      Error("socks5: password required");
      return STAT_NORETRY;
   }

   pos = sendbuff;
   *pos++ = SOCKS5_USERPASS_VERSION;
   *pos++ = strlen(username);
   *pos = '\0'; strncat(pos, username, 255);
   pos += strlen(username);
   *pos++ = strlen(password);
   *pos = '\0'; strncat(pos, password, 255);
   pos += strlen(password);

   result =
     xio_socks5_dialog(level, xfd, sendbuff, pos-sendbuff,
		       recvbuff, 2,
		       "username/password authentication");
   if (result != STAT_OK) {
      Msg(level, "socks5 username/password dialog failed");
      return result;
   }
   Info("socks5 username/password authentication succeeded");
   reply = (struct socks5_userpass_reply *)recvbuff;
   if (reply->version != SOCKS5_USERPASS_VERSION) {
      Msg(level, "socks5 username/password authentication version mismatch");
      return STAT_NORETRY;
   }
   if (reply->status != SOCKS5_REPLY_SUCCESS) {
      Msg(level, "socks5 username/password authentication failure");
      return STAT_RETRYLATER;
   }
   return STAT_OK;
}

int xio_socks5_dialog(int level, struct single *xfd,
		      unsigned char *sendbuff, size_t sendlen,
		      unsigned char *recvbuff, size_t recvlen,
		      const char *descr /* identifier/method selection */) {
   int result;

   /* send socks header (target addr+port, +auth) */
   Info1("sending socks5 %s message", descr);
   do {
      result = Write(xfd->wfd, sendbuff, sendlen);
   } while (result < 0 && errno == EINTR);
   if (result < 0) {
      Msg4(level, "write(%d, %p, "F_Zu"): %s",
	   xfd->wfd, sendbuff, sendlen, strerror(errno));
      if (Close(xfd->wfd) < 0) {
	 Warn2("close(%d): %s", xfd->wfd, strerror(errno));
      }
      if (Close(xfd->rfd) < 0) {
	 Warn2("close(%d): %s", xfd->rfd, strerror(errno));
      }
      return STAT_RETRYLATER;	/* retry complete open cycle */
   }

   Info2("waiting for socks5 %s reply ("F_Zu" bytes)",
	 descr, recvlen);
   if ((result =
	xiosocks5_recvbytes(xfd, recvbuff, recvlen, level))
       == EOF) {
      /* we had a problem while reading socks answer */
      /*! Close(); */
      return result;	/* retry complete open cycle */
   }
#if 0
   replyversion = ((struct socks5_version *)recvbuff)->version;
   Info1("socks5 server reply version: {%u}", replyversion);
   if (replyversion != SOCKS5_VERSION) {
      Error1("socks5: server protocol version is %u",
	     replyversion);
   }
#endif
   return 0;
}

#endif /* WITH_SOCKS5 */

