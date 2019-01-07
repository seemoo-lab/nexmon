/* source: xio-readline.c */
/* Copyright Gerhard Rieger */
/* Published under the GNU General Public License V.2, see file COPYING */

/* this file contains the source for opening the readline address */

#include "xiosysincludes.h"
#include "xioopen.h"

#include "xio-termios.h"
#include "xio-readline.h"


#if WITH_READLINE

/*
options: history file
	prompt
	mode=vi?
	inputrc=?

uses stdin!!
*/

/* length of buffer for dynamic prompt */
#define READLINE_MAXPROMPT 512

static int xioopen_readline(int argc, const char *argv[], struct opt *opts,
			    int rw, xiofile_t *xfd, unsigned groups,
			    int dummy1, int dummy2, int dummy3);


static const struct xioaddr_endpoint_desc xioendpoint_readline0 = {
   XIOADDR_SYS, "readline", 0, XIOBIT_RDONLY|XIOBIT_RDWR, GROUP_FD|GROUP_TERMIOS|GROUP_READLINE, XIOSHUT_CLOSE, XIOCLOSE_NONE, xioopen_readline, 0, 0, 0 HELP(NULL) };

const union xioaddr_desc *xioaddrs_readline[] = {
   (union xioaddr_desc *)&xioendpoint_readline0,
   NULL
};

const struct optdesc opt_history_file = { "history-file", "history", OPT_HISTORY_FILE, GROUP_READLINE, PH_LATE, TYPE_STRING, OFUNC_OFFSET, XIO_OFFSETOF(para.readline.history_file) };
const struct optdesc opt_prompt       = { "prompt",       NULL,      OPT_PROMPT,       GROUP_READLINE, PH_LATE, TYPE_STRING, OFUNC_OFFSET, XIO_OFFSETOF(para.readline.prompt) };
const struct optdesc opt_noprompt     = { "noprompt",     NULL,      OPT_NOPROMPT,     GROUP_READLINE, PH_LATE, TYPE_BOOL,   OFUNC_SPEC,   0 };
const struct optdesc opt_noecho       = { "noecho",       NULL,      OPT_NOECHO,       GROUP_READLINE, PH_LATE, TYPE_STRING, OFUNC_SPEC,   0 };

static int xioopen_readline(int argc, const char *argv[], struct opt *opts,
			    int xioflags, xiofile_t *xfd, unsigned groups,
			    int dummy1, int dummy2, int dummy3) {
   int rw = (xioflags & XIO_ACCMODE);
   char msgbuf[256], *cp = msgbuf;
   bool noprompt = false;
   char *noecho = NULL;

   if (argc != 1) {
      Error1("%s: 0 parameters required", argv[0]);
      return STAT_NORETRY;
   }

   if (!(xioflags & XIO_MAYCONVERT)) {
      Error("address with data processing not allowed here");
      return STAT_NORETRY;
   }
   xfd->common.flags |= XIO_DOESCONVERT;

   strcpy(cp, "using "); cp = strchr(cp, '\0');
   if (XIOWITHRD(rw)) {
      strcpy(cp, "readline on stdin for reading"); cp = strchr(cp, '\0');

      if (XIOWITHWR(rw))
      strcpy(cp, " and ");  cp = strchr(cp, '\0');
   }
   if (XIOWITHWR(rw)) {
      strcpy(cp, "stdio for writing"); cp = strchr(cp, '\0');
   }
   Notice(msgbuf);

   xfd->stream.rfd      = 0;	/* stdin */
   if (XIOWITHWR(rw)) {
      xfd->stream.wfd      = 1;	/* stdout */
   }
   xfd->stream.howtoclose = XIOCLOSE_READLINE;
   xfd->stream.dtype    = XIODATA_READLINE;

#if WITH_TERMIOS
   if (Isatty(xfd->stream.rfd)) {
      if (Tcgetattr(xfd->stream.rfd, &xfd->stream.savetty) < 0) {
	 Warn2("cannot query current terminal settings on fd %d. %s",
	       xfd->stream.rfd, strerror(errno));
      } else {
	 xfd->stream.ttyvalid = true;
      }
   }
#endif /* WITH_TERMIOS */

   if (applyopts_single(&xfd->stream, opts, PH_INIT) < 0)  return -1;
   applyopts(-1, opts, PH_INIT);

   applyopts2(xfd->stream.rfd, opts, PH_INIT, PH_FD);

   Using_history();
   applyopts_offset(&xfd->stream, opts);
   retropt_bool(opts, OPT_NOPROMPT, &noprompt);
   if (!noprompt && !xfd->stream.para.readline.prompt) {
      xfd->stream.para.readline.dynbytes = READLINE_MAXPROMPT;
      xfd->stream.para.readline.dynprompt =
	 Malloc(xfd->stream.para.readline.dynbytes+1);
      xfd->stream.para.readline.dynend =
	 xfd->stream.para.readline.dynprompt;
   }

#if HAVE_REGEX_H
   retropt_string(opts, OPT_NOECHO, &noecho);
   if (noecho) {
      int errcode;
      char errbuf[128];
      if ((errcode = regcomp(&xfd->stream.para.readline.noecho, noecho,
			     REG_EXTENDED|REG_NOSUB))
	  != 0) {
	 regerror(errcode, &xfd->stream.para.readline.noecho,
		  errbuf, sizeof(errbuf));
	 Error3("regcomp(%p, \"%s\", REG_EXTENDED|REG_NOSUB): %s",
		&xfd->stream.para.readline.noecho, noecho, errbuf);
	 return -1;
      }
      xfd->stream.para.readline.hasnoecho = true;
   }
#endif /* HAVE_REGEX_H */
   if (xfd->stream.para.readline.history_file) {
      Read_history(xfd->stream.para.readline.history_file);
   }
#if _WITH_TERMIOS
   xiotermios_clrflag(xfd->stream.rfd, 3, ICANON);
   xiotermios_clrflag(xfd->stream.rfd, 3, ECHO);
#endif /* _WITH_TERMIOS */
   return _xio_openlate(&xfd->stream, opts);
}


ssize_t xioread_readline(struct single *pipe, void *buff, size_t bufsiz) {
   /*! indent */
   ssize_t bytes;
   char *line;
   int _errno;

#if HAVE_REGEX_H
      if (pipe->para.readline.dynprompt &&
	  pipe->para.readline.hasnoecho &&
	  !regexec(&pipe->para.readline.noecho,
		   pipe->para.readline.dynprompt, 0, NULL, 0)) {
#if _WITH_TERMIOS
      /* under these conditions, we do not echo input, thus we circumvent
	 readline */
	 struct termios saveterm, setterm;
	 *pipe->para.readline.dynend = '\0';
	 Tcgetattr(pipe->rfd, &saveterm);	/*! error */
	 setterm = saveterm;
	 setterm.c_lflag |= ICANON;
	 Tcsetattr(pipe->rfd, TCSANOW, &setterm);	/*!*/
#endif /* _WITH_TERMIOS */
	 do {
	    bytes = Read(pipe->rfd, buff, bufsiz);
	 } while (bytes < 0 && errno == EINTR);
	 if (bytes < 0) {
	    _errno = errno;
	    Error4("read(%d, %p, "F_Zu"): %s",
		   pipe->rfd, buff, bufsiz, strerror(_errno));
	    errno = _errno;
	    return -1;
	 }
#if _WITH_TERMIOS
	 setterm.c_lflag &= ~ICANON;
	 Tcgetattr(pipe->rfd, &setterm);	/*! error */
	 Tcsetattr(pipe->rfd, TCSANOW, &saveterm);	/*!*/
#endif /* _WITH_TERMIOS */
	 pipe->para.readline.dynend = pipe->para.readline.dynprompt;
	 /*Write(pipe->rfd, "\n", 1);*/	/*!*/
	 return bytes;
      }
#endif /* HAVE_REGEX_H */

#if _WITH_TERMIOS
      xiotermios_setflag(pipe->rfd, 3, ECHO);
#endif /* _WITH_TERMIOS */
      if (pipe->para.readline.prompt || pipe->para.readline.dynprompt) {
	 /* we must carriage return, because readline will first print the
	    prompt */
	 ssize_t writt;
	 writt = writefull(pipe->rfd, "\r", 1);
	 if (writt < 0) {
	    Warn2("write(%d, \"\\r\", 1): %s",
		   pipe->rfd, strerror(errno));
	 } else if (writt < 1) {
	    Warn1("write() only wrote "F_Zu" of 1 byte", writt);
	 }
      }

      if (pipe->para.readline.dynprompt) {
	 *pipe->para.readline.dynend = '\0';
	 line = Readline(pipe->para.readline.dynprompt);
	 pipe->para.readline.dynend = pipe->para.readline.dynprompt;
      } else {
	 line = Readline(pipe->para.readline.prompt);
      }
      /* GNU readline defines no error return */
      if (line == NULL) {
	 return 0;	/* EOF */
      }
#if _WITH_TERMIOS
      xiotermios_clrflag(pipe->rfd, 3, ECHO);
#endif /* _WITH_TERMIOS */
      Add_history(line);
      bytes = strlen(line);
      ((char *)buff)[0] = '\0'; strncat(buff, line, bufsiz-1);
      free(line);
      if ((size_t)bytes < bufsiz) {
	 strcat(buff, "\n");  ++bytes;
      }
      return bytes;
}

void xioscan_readline(struct single *pipe, const void *buff, size_t bytes) {
   if (pipe->dtype == XIODATA_READLINE && pipe->para.readline.dynprompt) {
      /* we save the last part of the output as possible prompt */
      const void *ptr = buff;
      const void *pcr;
      const void *plf;
      size_t len;

      if (bytes > pipe->para.readline.dynbytes) {
	 ptr = (const char *)buff + bytes - pipe->para.readline.dynbytes;
	 len = pipe->para.readline.dynbytes;
      } else {
	 len = bytes;
      }
      pcr = memrchr(ptr, '\r', len);
      plf = memrchr(ptr, '\n', len);
      if (pcr != NULL || plf != NULL) {
	 const void *peol = Max(pcr, plf);
	 /* forget old prompt */
	 pipe->para.readline.dynend = pipe->para.readline.dynprompt;
	 len -= (peol+1 - ptr);
	 /* new prompt starts here */
	 ptr = (const char *)peol+1;
      }
      if (pipe->para.readline.dynend - pipe->para.readline.dynprompt + len >
	  pipe->para.readline.dynbytes) {
	 memmove(pipe->para.readline.dynprompt,
		 pipe->para.readline.dynend -
		    (pipe->para.readline.dynbytes - len),
		 pipe->para.readline.dynbytes - len);
	 pipe->para.readline.dynend =
	    pipe->para.readline.dynprompt + pipe->para.readline.dynbytes - len;
      }
      memcpy(pipe->para.readline.dynend, ptr, len);
      pipe->para.readline.dynend = pipe->para.readline.dynend + len;
   }
   return;
}

#endif /* WITH_READLINE */
