/* source: xiotransfer.c */
/* Copyright Gerhard Rieger 2007-2012 */
/* Published under the GNU General Public License V.2, see file COPYING */

/* this is the source file of the data transfer function */

#include "xiosysincludes.h"

#include "mytypes.h"
#include "compat.h"
#include "error.h"

#include "sycls.h"
#include "xio.h"

static 
int cv_newline(unsigned char **buff, ssize_t *bytes, int lineterm1, int lineterm2);


#define MAXTIMESTAMPLEN 128
/* prints the timestamp to the buffer and terminates it with '\0'. This buffer
   should be at least MAXTIMESTAMPLEN bytes long.
   returns 0 on success or -1 if an error occurred */
int gettimestamp(char *timestamp) {
   size_t bytes;
#if HAVE_GETTIMEOFDAY || 1
   struct timeval now;
   int result;
   time_t nowt;
#else /* !HAVE_GETTIMEOFDAY */
   time_t now;
#endif /* !HAVE_GETTIMEOFDAY */

#if HAVE_GETTIMEOFDAY || 1
   result = gettimeofday(&now, NULL);
   if (result < 0) {
      return result;
   } else {
      nowt = now.tv_sec;
#if HAVE_STRFTIME
      bytes = strftime(timestamp, 20, "%Y/%m/%d %H:%M:%S", localtime(&nowt));
      bytes += sprintf(timestamp+19, "."F_tv_usec" ", now.tv_usec);
#else
      strcpy(timestamp, ctime(&nowt));
      bytes = strlen(timestamp);
#endif
   }
#else /* !HAVE_GETTIMEOFDAY */
   now = time(NULL);  if (now == (time_t)-1) {
      return -1;
   } else {
#if HAVE_STRFTIME
      bytes = strftime(timestamp, 21, "%Y/%m/%d %H:%M:%S ", localtime(&now));
#else
      strcpy(timestamp, ctime(&now));
      bytes = strlen(timestamp);
#endif
   }
#endif /* !HAVE_GETTIMEOFDAY */
   return 0;
}


static const char *prefixltor = "> ";
static const char *prefixrtol = "< ";
static unsigned long numltor;
static unsigned long numrtol;
/* print block header (during verbose or hex dump)
   returns 0 on success or -1 if an error occurred */
static int
   xioprintblockheader(FILE *file, size_t bytes, bool righttoleft) {
   char timestamp[MAXTIMESTAMPLEN];
   char buff[128+MAXTIMESTAMPLEN];
   if (gettimestamp(timestamp) < 0) {
      return -1;
   }
   if (righttoleft) {
      sprintf(buff, "%s%s length="F_Zu" from=%lu to=%lu\n",
	      prefixrtol, timestamp, bytes, numrtol, numrtol+bytes-1);
      numrtol+=bytes;
   } else {
      sprintf(buff, "%s%s length="F_Zu" from=%lu to=%lu\n",
	      prefixltor, timestamp, bytes, numltor, numltor+bytes-1);
      numltor+=bytes;
   }
   fputs(buff, file);
   return 0;
}


/* inpipe is suspected to have read data available; read at most bufsiz bytes
   and transfer them to outpipe. Perform required data conversions.
   buff must be a malloc()'ed storage and might be realloc()'ed in this
   function if more space is required after conversions. 
   Returns the number of bytes written, or 0 on EOF or <0 if an
   error occurred or when data was read but none written due to conversions
   (with EAGAIN). EAGAIN also occurs when reading from a nonblocking FD where
   the file has a mandatory lock.
   If 0 bytes were read (EOF), it does NOT shutdown or close a channel, and it
   does NOT write a zero bytes block.
   */
/* inpipe, outpipe must be single descriptors (not dual!) */
int xiotransfer(xiofile_t *inpipe, xiofile_t *outpipe,
		unsigned char **buff, size_t bufsiz, bool righttoleft) {
   ssize_t bytes, writt = 0;

	 bytes = xioread(inpipe, *buff, bufsiz);
	 if (bytes < 0) {
	    if (errno != EAGAIN)
	       XIO_RDSTREAM(inpipe)->eof = 2;
	    /*xioshutdown(inpipe, SHUT_RD);*/
	    return -1;
	 }
	 if (bytes == 0 && XIO_RDSTREAM(inpipe)->ignoreeof &&
	     !inpipe->stream.closing) {
	    ;
	 } else if (bytes == 0) {
	    XIO_RDSTREAM(inpipe)->eof = 2;
	    inpipe->stream.closing = MAX(inpipe->stream.closing, 1);
	 }

      if (bytes > 0) {
	 /* handle escape char */
	 if (XIO_RDSTREAM(inpipe)->escape != -1) {
	    /* check input data for escape char */
	    unsigned char *ptr = *buff;
	    size_t ctr = 0;
	    while (ctr < bytes) {
	       if (*ptr == XIO_RDSTREAM(inpipe)->escape) {
		  /* found: set flag, truncate input data */
		  XIO_RDSTREAM(inpipe)->actescape = true;
		  bytes = ctr;
		  Info("escape char found in input");
		  break;
	       }
	       ++ptr; ++ctr;
	    }
	    if (ctr != bytes) {
	       XIO_RDSTREAM(inpipe)->eof = 2;
	    }
	 }
      }

	    if (XIO_RDSTREAM(inpipe)->lineterm !=
		XIO_WRSTREAM(outpipe)->lineterm) {
	       cv_newline(buff, &bytes,
			  XIO_RDSTREAM(inpipe)->lineterm,
			  XIO_WRSTREAM(outpipe)->lineterm);
	    }
	    if (bytes == 0) {
	       /*errno = EAGAIN;  return -1;*/
	       return bytes;
	    }

	    if (xioparams->verbose && xioparams->verbhex) {
	       /* Hack-o-rama */
	       size_t i = 0;
	       size_t j;
	       size_t N = 16;
	       const unsigned char *end, *s, *t;
	       s = *buff;
	       end = (*buff)+bytes;
	       xioprintblockheader(stderr, bytes, righttoleft);
	       while (s < end) {
		  /*! prefix? */
		  j = Min(N, (size_t)(end-s));

		  /* print hex */
		  t = s;
		  i = 0;
		  while (i < j) {
		     int c = *t++;
		     fprintf(stderr, " %02x", c);
		     ++i;
		     if (c == '\n')  break;
		  }

		  /* fill hex column */
		  while (i < N) {
		     fputs("   ", stderr);
		     ++i;
		  }
		  fputs("  ", stderr);

		  /* print acsii */
		  t = s;
		  i = 0;
		  while (i < j) {
		     int c = *t++;
		     if (c == '\n') {
			fputc('.', stderr);
			break;
		     }
		     if (!isprint(c))
			c = '.';
		     fputc(c, stderr);
		     ++i;
		  }

		  fputc('\n', stderr);
		  s = t;
	       }
	       fputs("--\n", stderr);
	    } else if (xioparams->verbose) {
	       size_t i = 0;
	       xioprintblockheader(stderr, bytes, righttoleft);
	       while (i < (size_t)bytes) {
		  int c = (*buff)[i];
		  if (i > 0 && (*buff)[i-1] == '\n')
		     /*! prefix? */;
		  switch (c) {
		  case '\a' : fputs("\\a", stderr); break;
		  case '\b' : fputs("\\b", stderr); break;
		  case '\t' : fputs("\t", stderr); break;
		  case '\n' : fputs("\n", stderr); break;
		  case '\v' : fputs("\\v", stderr); break;
		  case '\f' : fputs("\\f", stderr); break;
		  case '\r' : fputs("\\r", stderr); break;
		  case '\\' : fputs("\\\\", stderr); break;
		  default:
		     if (!isprint(c))
			c = '.';
		     fputc(c, stderr);
		     break;
		  }
		  ++i;
	       }
	    } else if (xioparams->verbhex) {
	       int i;
	       /* print prefix */
	       xioprintblockheader(stderr, bytes, righttoleft);
	       for (i = 0; i < bytes; ++i) {
		  fprintf(stderr, " %02x", (*buff)[i]);
	       }
	       fputc('\n', stderr);
	    }

	    writt = xiowrite(outpipe, *buff, bytes);
	    if (writt < 0) {
	       /* EAGAIN when nonblocking but a mandatory lock is on file.
		  the problem with EAGAIN is that the read cannot be repeated,
		  so we need to buffer the data and try to write it later
		  again. not yet implemented, sorry. */
#if 0
	       if (errno == EPIPE) {
		  return 0;	/* can no longer write; handle like EOF */
	       }
#endif
	       return -1;
	    } else {
	       Info3("transferred "F_Zu" bytes from %d to %d",
		     writt, XIO_GETRDFD(inpipe), XIO_GETWRFD(outpipe));
	    }
   return writt;
}

#define CR '\r'
#define LF '\n'


static int cv_newline(unsigned char **buff, ssize_t *bytes,
	       int lineterm1, int lineterm2) {
   /* must perform newline changes */
   if (lineterm1 <= LINETERM_CR && lineterm2 <= LINETERM_CR) {
      /* no change in data length */
      unsigned char from, to,  *p, *z;
      if (lineterm1 == LINETERM_RAW) {
	 from = '\n'; to = '\r';
      } else {
	 from = '\r'; to = '\n';
      }
      z = *buff + *bytes;
      p = *buff;
      while (p < z) {
	 if (*p == from)  *p = to;
	 ++p;
      }

   } else if (lineterm1 == LINETERM_CRNL) {
      /* buffer becomes shorter */
      unsigned char to,  *s, *t, *z;
      if (lineterm2 == LINETERM_RAW) {
	 to = '\n';
      } else {
	 to = '\r';
      }
      z = *buff + *bytes;
      s = t = *buff;
      while (s < z) {
	 if (*s == '\r') {
	    ++s;
	    continue;
	 }
	 if (*s == '\n') {
	    *t++ = to; ++s;
	 } else {
	    *t++ = *s++;
	 }
      }
      *bytes = t - *buff;
   } else {
      /* buffer becomes longer, must alloc another space */
      unsigned char *buf2;
      unsigned char from;  unsigned char *s, *t, *z;
      if (lineterm1 == LINETERM_RAW) {
	 from = '\n';
      } else {
	 from = '\r';
      }
      if ((buf2 = Malloc(2*xioparams->bufsiz + 1)) == NULL) {
	 return -1;
      }
      s = *buff;  t = buf2;  z = *buff + *bytes;
      while (s < z) {
	 if (*s == from) {
	    *t++ = '\r'; *t++ = '\n';
	    ++s;
	    continue;
	 } else {
	    *t++ = *s++;
	 }
      }
      free(*buff);
      *buff = buf2;
      *bytes = t - buf2;;
   }
   return 0;
}
