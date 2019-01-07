/* source: xiohelp.c */
/* Copyright Gerhard Rieger 2001-2009 */
/* Published under the GNU General Public License V.2, see file COPYING */

/* this file contains the source for the help function */

#include "xiosysincludes.h"
#include "xioopen.h"

#include "xiohelp.h"

#if WITH_HELP

/* keep consistent with xioopts.h:enum e_types ! */
static const char *optiontypenames[] = {
	"CONST",	"BIN",		"BOOL",		"BYTE",
	"INT",		"LONG",		"STRING",	"PTRDIFF",
	"SHORT",	"SIZE_T",	"SOCKADDR",	"UNSIGNED-INT",
	"UNSIGNED-LONG","UNSIGNED-SHORT","MODE_T",	"GID_T",
	"UID_T",	"INT[3]",	"STRUCT-TIMEVAL", "STRUCT-TIMESPEC",
	"DOUBLE",	"STRING-NULL",	"LONG:LONG",	"OFF_T",
	"OFF64_T",	"INT:INT",	"INT:INTP",	"INT:BIN",
	"INT:STRING",	"INT:INT:INT",	"INT:INT:BIN",	"INT:INT:STRING",
	"IP4NAME",	"STRING:STRING",

#if HAVE_STRUCT_LINGER
	"STRUCT-LINGER",
#endif
#if HAVE_STRUCT_IP_MREQN
			"STRUCT-IP_MREQN",
#elif HAVE_STRUCT_IP_MREQ
			"STRUCT-IP_MREQ",
#endif
} ;


/* keep consistent with xioopts.h:#define GROUP_* ! */
static const char *addressgroupnames[] = {
	"FD",		"FIFO",		"SOCKS5",		"BLK",
	"REG",		"SOCKET",	"READLINE",	"undef",
	"NAMED",	"OPEN",		"EXEC",		"FORK",
	"LISTEN",	"DEVICE",	"CHILD",	"RETRY",
	"TERMIOS",	"RANGE",	"PTY",		"PARENT",
	"UNIX",		"IP4",		"IP6",		"INTERFACE",
	"UDP",		"TCP",		"SOCKS4",	"OPENSSL",
	"PROCESS",	"APPL",		"HTTP",		"SCTP"
} ;


/* keep consistent with xioopts.h:enum ephase ! */
static char *optionphasenames[] = {
	"ALL",		"INIT",		"EARLY",
	"PREOPEN",	"OPEN",		"PASTOPEN",
	"PRESOCKET",	"SOCKET",	"PASTSOCKET",
	"PREBIGEN",	"BIGEN",	"PASTBIGEN",
	"FD",
	"PREBIND",	"BIND",		"PASTBIND",	
	"PRELISTEN",	"LISTEN",	"PASTLISTEN",
	"PRECONNECT",	"CONNECT",	"PASTCONNECT",
	"PREACCEPT",	"ACCEPT",	"PASTACCEPT",
	"CONNECTED",
	"PREFORK",	"FORK",		"PASTFORK",
	"LATE",		"LATE2",
	"PREEXEC",	"EXEC",		"SPECIFIC",
	NULL
} ;


/* print a line about a single option */
static int xiohelp_option(FILE *of, const struct optname *on, const char *name) {
   int j;
   unsigned int groups;
   bool occurred;

   fprintf(of, "      %s\tgroups=", name);
   groups = on->desc->group;  occurred = false;
   for (j = 0; j < 32; ++j) {
      if (groups & 1) {
	 if (occurred)  { fputc(',', of); }
	 fprintf(of, "%s", addressgroupnames[j]);
	 occurred = true;
      }
      groups >>= 1;
   }
   fprintf(of, "\tphase=%s", optionphasenames[on->desc->phase]);
   fprintf(of, "\ttype=%s", optiontypenames[on->desc->type]);
   fputc('\n', of);
   return 0;
}

int xioopenhelp(FILE *of,
	       int level	/* 0..only addresses, 1..and options */
	       ) {
   const struct xioaddrname *an;
   const struct optname *on;
   int i, j;
   unsigned int groups;
   bool occurred;

   fputs("   bi-address:\n", of);
   fputs("      pipe[,<opts>]\tgroups=FD,FIFO\n", of);
   if (level == 2) {
      fputs("      echo is an alias for pipe\n", of);
      fputs("      fifo is an alias for pipe\n", of);
   }
   fputs("      <single-address>%<single-address>\n", of);
   fputs("      <single-address>\n", of);
   fputs("   single-address:\n", of);
   fputs("      <address-head>[,<opts>]\n", of);
   fputs("   address-head:\n", of);
   an = &address_names[0];
   i = 0;
   while (address_names[i].name) {
      if (!strcmp(an->name, (an->desc)[0]->common_desc.defname)) {
         /* it is a canonical address name */
	const union xioaddr_desc **ad;
	ad = an->desc;
	while (*ad != NULL) {
	   const char *syntax;
	   int pos = 0;
	   fprintf(of, "      %s", an->name);  pos += strlen(an->name);
	 if ((*ad)->tag == XIOADDR_ENDPOINT) {
	    syntax = (*ad)->endpoint_desc.syntax;
	 } else if ((*ad)->tag == XIOADDR_INTER) {
	    syntax = (*ad)->inter_desc.syntax;
	 } else {
	    syntax = NULL;
	 }
	 if (syntax) {
	    fputs(syntax, of);  pos += strlen(syntax); }
	 while (pos < 36) { putc(' ', of); ++pos; }
	 if ((*ad)->common_desc.leftdirs & XIOBIT_RDONLY)  {
	    putc('r', of); }  else { putc(' ', of); }
	 if ((*ad)->common_desc.leftdirs & XIOBIT_WRONLY)  {
	    putc('w', of); }  else { putc(' ', of); }
	 if ((*ad)->common_desc.leftdirs & XIOBIT_RDWR)  {
	    putc('b', of); }  else { putc(' ', of); }
	 putc(' ', of);
	 if ((*ad)->tag == 1) {
	    while (pos < 36) { putc(' ', of); ++pos; }
	    if ((*ad)->inter_desc.rightdirs & XIOBIT_RDONLY)  {
	       putc('r', of); }  else { putc(' ', of); }
	    if ((*ad)->inter_desc.rightdirs & XIOBIT_WRONLY)  {
	       putc('w', of); }  else { putc(' ', of); }
	    if ((*ad)->inter_desc.rightdirs & XIOBIT_RDWR)  {
	       putc('b', of); }  else { putc(' ', of); }
	 } else {
	    fputs("   ", of);
	 }

	 pos += 7;
	 fputs(" groups=", of);
	 groups = (*ad)->common_desc.groups;  occurred = false;
	 for (j = 0; j < 32; ++j) {
	    if (groups & 1) {
	       if (occurred) { fputc(',', of); }
	       fprintf(of, "%s", addressgroupnames[j]);
	       pos += strlen(addressgroupnames[j]);
	       occurred = true;
	    }
	    groups >>= 1;
	 }
	 fputc('\n', of);
	 ++ad;
	}
      } else if (level == 2) {
         fprintf(of, "         %s is an alias name for %s\n", an->name, (an->desc)[0]->common_desc.defname);
      }
      ++an; ++i;
   }
   if (level == 2) {
      fputs("         <num> is a short form for fd:<num>\n", of);
      fputs("         <filename> is a short form for gopen:<filename>\n", of);
   }

   if (level <= 0)  return 0;

   fputs("   opts:\n", of);
   fputs("      <opt>{,<opts>}:\n", of);
   fputs("   opt:\n", of);
   on = optionnames;
   while (on->name != NULL) {
      if (on->desc->nickname!= NULL
	  && !strcmp(on->name, on->desc->nickname)) {
	 if (level == 2) {
	    fprintf(of, "      %s is an alias for %s\n", on->name, on->desc->defname);
	 } else {
	    xiohelp_option(of, on, on->name);
	 }
      } else if (on->desc->nickname == NULL &&
		 !strcmp(on->name, on->desc->defname)) {
	 xiohelp_option(of, on, on->name);
      } else if (level == 2) {
	 if (!strcmp(on->name, on->desc->defname)) {
	    xiohelp_option(of, on, on->name);
	 } else {
	    fprintf(of, "      %s is an alias for %s\n", on->name, on->desc->defname);
	 }
      }
      ++on;
   }
   fflush(of);
   return 0;
}

#endif /* WITH_HELP */
