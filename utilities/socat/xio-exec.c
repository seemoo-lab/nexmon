/* source: xio-exec.c */
/* Copyright Gerhard Rieger */
/* Published under the GNU General Public License V.2, see file COPYING */

/* this file contains the source for opening addresses of exec type */
/* this file contains the source for an inter address that just invokes a
   program. The program should provide its data side on FDs 0 and 1, and its
   protocol side on FDs 3 and 4. */

#include "xiosysincludes.h"
#include "xioopen.h"
#include "nestlex.h"

#include "xio-progcall.h"
#include "xio-exec.h"

#if WITH_EXEC

static int xioopen_exec1(int argc, const char *argv[], struct opt *opts,
		int xioflags,	/* XIO_RDONLY etc. */
		xiofile_t *fd,
		unsigned groups,
		int inter, int form, int dummy3
		);

/* the endpoint variant: get stdin and/or stdout on "left" side. socat does not
   provide a "right" side for script */
static const struct xioaddr_endpoint_desc xioendpoint_exec1 = { XIOADDR_ENDPOINT, "exec",  1, XIOBIT_ALL, GROUP_FD|GROUP_FORK|GROUP_EXEC|GROUP_SOCKET|GROUP_SOCK_UNIX|GROUP_TERMIOS|GROUP_FIFO|GROUP_PTY|GROUP_PARENT, XIOSHUT_UNSPEC, XIOCLOSE_UNSPEC, xioopen_exec1,   false, 0, 0 HELP(":<command-line>") };
/* the inter address variant: the bidirectional form has stdin and stdout on
   its "left" side, and FDs 3 (for reading) and FD 4 (for writing) on its right
   side. */
static const struct xioaddr_inter_desc    xiointer_exec1_2rw    = { XIOADDR_INTER,    "exec", 1, XIOBIT_RDWR, GROUP_FD|GROUP_FORK|GROUP_EXEC|GROUP_SOCKET|GROUP_SOCK_UNIX|GROUP_TERMIOS|GROUP_FIFO|GROUP_PTY|GROUP_PARENT, XIOSHUT_UNSPEC, XIOCLOSE_UNSPEC, xioopen_exec1, true, 2, 0, XIOBIT_RDWR HELP(":<command-line>") };
static const struct xioaddr_inter_desc    xiointer_exec1_2ro    = { XIOADDR_INTER,    "exec", 1, XIOBIT_RDONLY, GROUP_FD|GROUP_FORK|GROUP_EXEC|GROUP_SOCKET|GROUP_SOCK_UNIX|GROUP_TERMIOS|GROUP_FIFO|GROUP_PTY|GROUP_PARENT, XIOSHUT_UNSPEC, XIOCLOSE_UNSPEC, xioopen_exec1, true, 2, 0, XIOBIT_WRONLY HELP(":<command-line>") };
static const struct xioaddr_inter_desc    xiointer_exec1_2wo    = { XIOADDR_INTER,    "exec", 1, XIOBIT_WRONLY, GROUP_FD|GROUP_FORK|GROUP_EXEC|GROUP_SOCKET|GROUP_SOCK_UNIX|GROUP_TERMIOS|GROUP_FIFO|GROUP_PTY|GROUP_PARENT, XIOSHUT_UNSPEC, XIOCLOSE_UNSPEC, xioopen_exec1, true, 2, 0, XIOBIT_RDONLY HELP(":<command-line>") };
/* the unidirectional inter address variant: the "left" side reads from stdin,
   and the right side reads from stdout. */
static const struct xioaddr_inter_desc    xiointer_exec1_1wo    = { XIOADDR_INTER,    "exec1",  1, XIOBIT_WRONLY, GROUP_FD|GROUP_FORK|GROUP_EXEC|GROUP_SOCKET|GROUP_SOCK_UNIX|GROUP_TERMIOS|GROUP_FIFO|GROUP_PTY|GROUP_PARENT, XIOSHUT_UNSPEC, XIOCLOSE_UNSPEC, xioopen_exec1, true, 1, 0, XIOBIT_RDONLY HELP(":<command-line>") };

/* the general forms, designed for bidirectional transfer (stdio -- 3,4) */
const union xioaddr_desc *xioaddrs_exec[] = {
   (union xioaddr_desc *)&xioendpoint_exec1,
   (union xioaddr_desc *)&xiointer_exec1_2ro,
   (union xioaddr_desc *)&xiointer_exec1_2wo,
   (union xioaddr_desc *)&xiointer_exec1_2rw,
   NULL };

/* unidirectional inter address (stdin -- stdout) */
const union xioaddr_desc *xioaddrs_exec1[] = {
   (union xioaddr_desc *)&xiointer_exec1_1wo,
   NULL };


const struct optdesc opt_dash = { "dash", "login", OPT_DASH, GROUP_EXEC, PH_PREEXEC, TYPE_BOOL, OFUNC_SPEC };


/* the "1" in the function name means that it takes one parameter */
static int xioopen_exec1(int argc, const char *argv[], struct opt *opts,
		int xioflags,	/* XIO_RDONLY, XIO_MAYCHILD etc. */
		xiofile_t *fd,
		unsigned groups,
		int inter, int form, int dummy3
		) {
   int status;
   bool dash = false;
   int duptostderr;

   if (argc != 2) {
      Error3("\"%s:%s\": wrong number of parameters (%d instead of 1)", argv[0], argv[1], argc-1);
   }
      
   retropt_bool(opts, OPT_DASH, &dash);

   status = _xioopen_progcall(xioflags, &fd->stream, groups, &opts, &duptostderr, inter, form);
   if (status < 0)  return status;
   if (status == 0) {	/* child */
      const char *ends[] = { " ", NULL };
      const char *hquotes[] = { "'", NULL };
      const char *squotes[] = { "\"", NULL };
      const char *nests[] = {
	 "'", "'",
	 "(", ")",
	 "[", "]",
	 "{", "}",
	 NULL
      } ;
      char **pargv = NULL;
      int pargc;
      size_t len;
      const char *strp;
      char *token; /*! */
      char *tokp;
      char *path = NULL;
      char *tmp;
      int numleft;

      /*! Close(something) */
      /* parse command line */
      Debug1("child: args = \"%s\"", argv[1]);
      pargv = Malloc(8*sizeof(char *));
      if (pargv == NULL)  return STAT_RETRYLATER;
      len = strlen(argv[1])+1;
      strp = argv[1];
      token = Malloc(len); /*! */
      tokp = token;
      if (nestlex(&strp, &tokp, &len, ends, hquotes, squotes, nests,
		  false, true, true, false) < 0) {
	 Error("internal: miscalculated string lengths");
      }
      *tokp++ = '\0';
      pargv[0] = strrchr(tokp-1, '/');
      if (pargv[0] == NULL)  pargv[0] = token;  else  ++pargv[0];
      pargc = 1;
      while (*strp == ' ') {
	 while (*++strp == ' ')  ;
	 if ((pargc & 0x07) == 0) {
	    pargv = Realloc(pargv, (pargc+8)*sizeof(char *));
	    if (pargv == NULL)  return STAT_RETRYLATER;
	 }
	 pargv[pargc++] = tokp;
	 if (nestlex(&strp, &tokp, &len, ends, hquotes, squotes, nests,
		     false, true, true, false) < 0) {
	    Error("internal: miscalculated string lengths");
	 }
	 *tokp++ = '\0';
      }
      pargv[pargc] = NULL;

      if ((tmp = Malloc(strlen(pargv[0])+2)) == NULL) {
	 return STAT_RETRYLATER;
      }
      if (dash) {
	 tmp[0] = '-';
	 strcpy(tmp+1, pargv[0]);
      } else {
	 strcpy(tmp, pargv[0]);
      }
      pargv[0] = tmp;

      if (setopt_path(opts, &path) < 0) {
	 /* this could be dangerous, so let us abort this child... */
	 Exit(1);
      }

      if ((numleft = leftopts(opts)) > 0) {
	 Error1("%d option(s) could not be used", numleft);
	 showleft(opts);
	 return STAT_NORETRY;
      }

      /* only now redirect stderr */
      if (duptostderr >= 0) {
	 diag_dup();
	 Dup2(duptostderr, 2);
      }
      Notice1("execvp'ing \"%s\"", token);
      Execvp(token, pargv);
      /* here we come only if execvp() failed */
      switch (pargc) {
      case 1: Error3("execvp(\"%s\", \"%s\"): %s", token, pargv[0], strerror(errno)); break; 
      case 2: Error4("execvp(\"%s\", \"%s\", \"%s\"): %s", token, pargv[0], pargv[1], strerror(errno)); break; 
      case 3:
      default:
	 Error5("execvp(\"%s\", \"%s\", \"%s\", \"%s\", ...): %s", token, pargv[0], pargv[1], pargv[2], strerror(errno)); break; 
      }
      Exit(1);	/* this child process */
   }

   /* parent */
   return 0;
}

#endif /* WITH_EXEC */
