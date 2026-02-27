/* source: xio-file.c */
/* Copyright Gerhard Rieger 2001-2007 */
/* Published under the GNU General Public License V.2, see file COPYING */

/* this file contains the source for opening addresses of open type */

#include "xiosysincludes.h"
#include "xioopen.h"

#include "xio-named.h"
#include "xio-file.h"


static int xioopen_open1(int argc, const char *argv[], struct opt *opts, int xioflags, xiofile_t *fd, unsigned groups, int dummy1, int dummy2, int dummy3);


#if WITH_OPEN

/****** OPEN addresses ******/
const struct optdesc opt_o_rdonly    = { "o-rdonly",    "rdonly", OPT_O_RDONLY,    GROUP_OPEN, PH_OPEN, TYPE_BOOL, OFUNC_FLAG_PATTERN, O_RDONLY, O_ACCMODE };
const struct optdesc opt_o_wronly    = { "o-wronly",    "wronly", OPT_O_WRONLY,    GROUP_OPEN, PH_OPEN, TYPE_BOOL, OFUNC_FLAG_PATTERN, O_WRONLY, O_ACCMODE };
const struct optdesc opt_o_rdwr      = { "o-rdwr",      "rdwr",   OPT_O_RDWR,      GROUP_OPEN, PH_OPEN, TYPE_BOOL, OFUNC_FLAG_PATTERN, O_RDWR,   O_ACCMODE };
const struct optdesc opt_o_create    = { "o-create",    "creat",  OPT_O_CREATE,    GROUP_OPEN, PH_OPEN, TYPE_BOOL, OFUNC_FLAG, O_CREAT };
const struct optdesc opt_o_excl      = { "o-excl",      "excl",   OPT_O_EXCL,      GROUP_OPEN, PH_OPEN, TYPE_BOOL, OFUNC_FLAG, O_EXCL };
const struct optdesc opt_o_noctty    = { "o-noctty",    "noctty", OPT_O_NOCTTY,    GROUP_OPEN, PH_OPEN, TYPE_BOOL, OFUNC_FLAG, O_NOCTTY };
#ifdef O_SYNC
const struct optdesc opt_o_sync      = { "o-sync",      "sync",   OPT_O_SYNC,      GROUP_OPEN, PH_OPEN, TYPE_BOOL, OFUNC_FLAG, O_SYNC };
#endif
#ifdef O_NOFOLLOW
const struct optdesc opt_o_nofollow  = { "o-nofollow",  "nofollow",OPT_O_NOFOLLOW, GROUP_OPEN, PH_OPEN, TYPE_BOOL, OFUNC_FLAG, O_NOFOLLOW };
#endif
#ifdef O_DIRECTORY
const struct optdesc opt_o_directory = { "o-directory", "directory",OPT_O_DIRECTORY, GROUP_OPEN, PH_OPEN, TYPE_BOOL, OFUNC_FLAG, O_DIRECTORY };
#endif
#ifdef O_LARGEFILE
const struct optdesc opt_o_largefile = { "o-largefile", "largefile",OPT_O_LARGEFILE, GROUP_OPEN, PH_OPEN, TYPE_BOOL, OFUNC_FLAG, O_LARGEFILE };
#endif
#ifdef O_NSHARE
const struct optdesc opt_o_nshare    = { "o-nshare",    "nshare", OPT_O_NSHARE,    GROUP_OPEN, PH_OPEN, TYPE_BOOL, OFUNC_FLAG, O_NSHARE };
#endif
#ifdef O_RSHARE
const struct optdesc opt_o_rshare    = { "o-rshare",    "rshare", OPT_O_RSHARE,    GROUP_OPEN, PH_OPEN, TYPE_BOOL, OFUNC_FLAG, O_RSHARE };
#endif
#ifdef O_DEFER
const struct optdesc opt_o_defer     = { "o-defer",     "defer",  OPT_O_DEFER,     GROUP_OPEN, PH_OPEN, TYPE_BOOL, OFUNC_FLAG, O_DEFER };
#endif
#ifdef O_DIRECT
const struct optdesc opt_o_direct    = { "o-direct",    "direct", OPT_O_DIRECT,    GROUP_OPEN, PH_OPEN, TYPE_BOOL, OFUNC_FLAG, O_DIRECT };
#endif
#ifdef O_DSYNC
const struct optdesc opt_o_dsync     = { "o-dsync",     "dsync",  OPT_O_DSYNC,     GROUP_OPEN, PH_OPEN, TYPE_BOOL, OFUNC_FLAG, O_DSYNC };
#endif
#ifdef O_RSYNC
const struct optdesc opt_o_rsync     = { "o-rsync",     "rsync",  OPT_O_RSYNC,     GROUP_OPEN, PH_OPEN, TYPE_BOOL, OFUNC_FLAG, O_RSYNC };
#endif
#ifdef O_DELAY
const struct optdesc opt_o_delay     = { "o-delay",     "delay",  OPT_O_DELAY,     GROUP_OPEN, PH_OPEN, TYPE_BOOL, OFUNC_FLAG, O_DELAY };
#endif
#ifdef O_PRIV
const struct optdesc opt_o_priv      = { "o-priv",      "priv",   OPT_O_PRIV,      GROUP_OPEN, PH_OPEN, TYPE_BOOL, OFUNC_FLAG, O_PRIV };
#endif
const struct optdesc opt_o_trunc     = { "o-trunc",     "trunc",  OPT_O_TRUNC,     GROUP_OPEN, PH_LATE, TYPE_BOOL, OFUNC_FLAG, O_TRUNC };

#endif /* WITH_OPEN */


#if _WITH_FILE	/*! inconsistent name FILE vs. OPEN */

static const struct xioaddr_endpoint_desc xioaddr_open1 = { XIOADDR_SYS, "open", 1, XIOBIT_ALL, GROUP_FD|GROUP_FIFO|GROUP_CHR|GROUP_BLK|GROUP_REG|GROUP_NAMED|GROUP_OPEN|GROUP_FILE|GROUP_TERMIOS, XIOSHUT_CLOSE, XIOCLOSE_CLOSE, xioopen_open1, 0, 0, 0 HELP(":<filename>") };
const union xioaddr_desc *xioaddrs_open[] = {
   (union xioaddr_desc *)&xioaddr_open1,
   NULL
};

/* open for writing:
   if the filesystem entry already exists, the data is appended
   if it does not exist, a file is created and the data is appended
*/
static int xioopen_open1(int argc, const char *argv[], struct opt *opts, int xioflags, xiofile_t *xfd, unsigned groups, int dummy1, int dummy2, int dummy3) {
   const char *filename = argv[1];
   int rw = (xioflags & XIO_ACCMODE);
   int fd;
   bool exists;
   bool opt_unlink_close = false;
   int result;

   /* remove old file, or set user/permissions on old file; parse options */
   if ((result = _xioopen_named_early(argc, argv, xfd, groups, &exists, opts)) < 0) {
      return result;
   }

   retropt_bool(opts, OPT_UNLINK_CLOSE, &opt_unlink_close);
   if (opt_unlink_close) {
      if ((xfd->stream.unlink_close = strdup(filename)) == NULL) {
	 Error1("strdup(\"%s\"): out of memory", filename);
      }
      xfd->stream.opt_unlink_close = true;
   }

   Notice3("opening %s \"%s\" for %s",
	   filetypenames[(result&S_IFMT)>>12], filename, ddirection[rw]);
   if ((fd = _xioopen_open(filename, rw, opts)) < 0)
      return fd;
   if (XIOWITHRD(rw))  xfd->stream.rfd = fd;
   if (XIOWITHWR(rw))  xfd->stream.wfd = fd;

#if WITH_TERMIOS
   if (Isatty(fd)) {
      if (Tcgetattr(fd, &xfd->stream.savetty) < 0) {
	 Warn2("cannot query current terminal settings on fd %d: %s",
	       fd, strerror(errno));
      } else {
	 xfd->stream.ttyvalid = true;
      }
   }
#endif /* WITH_TERMIOS */

   applyopts_named(filename, opts, PH_FD);
   applyopts(fd, opts, PH_FD);
   applyopts_cloexec(fd, opts);

   applyopts_fchown(fd, opts);

   if ((result = _xio_openlate(&xfd->stream, opts)) < 0)
      return result;

   return 0;
}

#endif /* _WITH_FILE */
