/* $Id$ */
/* Copyright Gerhard Rieger 2004-2007 */
/* Published under the GNU General Public License V.2, see file COPYING */

#ifndef __xio_socks5_h_included
#define __xio_socks5_h_included 1

#define SOCKS5_VERSION 5

#define SOCKS5_METHOD_NOAUTH    0x00
#define SOCKS5_METHOD_GSSAPI    0x01
#define SOCKS5_METHOD_USERPASS  0x02
#define SOCKS5_METHOD_AVENTAIL  0x86	/*!*/
#define SOCKS5_METHOD_NONE      0xff

#define SOCKS5_COMMAND_CONNECT  0x01
#define SOCKS5_COMMAND_BIND     0x02
#define SOCKS5_COMMAND_UDPASSOC 0x03

#define SOCKS5_ADDRTYPE_IPV4    0x01
#define SOCKS5_ADDRTYPE_NAME    0x03
#define SOCKS5_ADDRTYPE_IPV6    0x04

#define SOCKS5_REPLY_SUCCESS    0x00
#define SOCKS5_REPLY_FAILURE    0x01
#define SOCKS5_REPLY_DENIED     0x02
#define SOCKS5_REPLY_NETUNREACH 0x03
#define SOCKS5_REPLY_HOSTUNREACH 0x04
#define SOCKS5_REPLY_REFUSED    0x05
#define SOCKS5_REPLY_TTLEXPIRED 0x06
#define SOCKS5_REPLY_CMDUNSUPP  0x07
#define SOCKS5_REPLY_ADDRUNSUPP 0x08

#define SOCKS5_USERPASS_VERSION 0x01

/* just the first byte of server replies */
struct socks5_version {
   uint8_t  version;
} ;

/* the version/method selection message of the client */
struct socks5_method {
   uint8_t  version;
   uint8_t  nmethods;
   uint8_t  methods[1];		/* has variable length, see nmethods */
} ;

/* the selection message of the server */
struct socks5_select {
   uint8_t  version;
   uint8_t  method;
} ;
#define SOCKS5_SELECT_LENGTH sizeof(struct socks5_select)

/* the request message */
struct socks5_request {
   uint8_t  version;
   uint8_t  command;
   uint8_t  reserved;
   uint8_t  addrtype;
   uint8_t  destaddr[1];	/* has variable length */
   /*uint16_t destport;*/	/* position depends on length of destaddr */
} ;

/* the request message */
struct socks5_reply {
   uint8_t  version;
   uint8_t  reply;
   uint8_t  reserved;
   uint8_t  addrtype;
   uint8_t  destaddr[1];	/* has variable length */
   /*uint16_t destport;*/	/* position depends on length of destaddr */
} ;
#define SOCKS5_REPLY_LENGTH1 4	/*!*/ /* the fixed size fields */

/* the username/password authentication reply */
struct socks5_userpass_reply {
   uint8_t  version;	/* authentication version, not socks version */
   uint8_t  status;
} ;

#if 0
extern const struct optdesc opt_socksport;
extern const struct optdesc opt_socksuser;
#endif
extern const struct optdesc opt_socks5_username;
extern const struct optdesc opt_socks5_password;

extern const union xioaddr_desc *xioaddrs_socks5_client[];

extern int
   _xioopen_socks5_connect0(struct single *xfd,
			    const char *targetname, const char *targetservice,
			    int level);
extern int _xioopen_socks5_connect(struct single *xfd,
				   const char *targetname,
				   const char *targetservice,
				   struct opt *opts,
				   int level);
extern int xio_socks5_dialog(int level, struct single *xfd,
			     unsigned char *sendbuff, size_t sendlen,
			     unsigned char *recvbuff, size_t recvlen,
			     const char *descr);
extern int xio_socks5_username_password(int level, struct opt *opts,
					struct single *xfd);

#endif /* !defined(__xio_socks5_h_included) */
