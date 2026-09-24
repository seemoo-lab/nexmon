#ifndef BCM43xx_ASM_ARGS_H_
#define BCM43xx_ASM_ARGS_H_

#include "util.h"


enum fwformat {
	FMT_RAW_LE32,   /* Raw microcode. No headers. 32bit little endian chunks. */
	FMT_RAW_BE32,   /* Raw microcode. No headers. 32bit big endian chunks. */
	FMT_B43,        /* b43/b43legacy headers. */
};

struct cmdline_args {
	int debug;				/* Debug level. */
	bool print_sizes;			/* Print sizes after assembling. */
	const char *initvals_fn_extension;	/* Initvals filename extension. */
	const char *real_infile_name;		/* The real input file name. */
	enum fwformat outformat;		/* The output file format. */
};

int parse_args(int argc, char **argv);
int open_input_file(void);
void close_input_file(void);

extern struct cmdline_args cmdargs;

#define IS_DEBUG		(cmdargs.debug > 0)
#define IS_VERBOSE_DEBUG	(cmdargs.debug > 1)
#define IS_INSANE_DEBUG		(cmdargs.debug > 2)

#endif /* BCM43xx_ASM_ARGS_H_ */
