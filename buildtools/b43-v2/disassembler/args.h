#ifndef B43_DASM_ARGS_H_
#define B43_DASM_ARGS_H_

enum fwformat {
	FMT_RAW_LE32,	/* Raw microcode. No headers. 32bit little endian chunks. */
	FMT_RAW_BE32,	/* Raw microcode. No headers. 32bit big endian chunks. */
	FMT_B43,	/* b43/b43legacy headers. */
};

struct cmdline_args {
	int debug;			/* Debug level. */
	unsigned int arch;		/* The architecture we're disassembling. */
	enum fwformat informat;		/* The input file format. */
	int print_addresses;		/* Print a comment with instruction address. */
	int unknown_decode;		/* Decode operands of unknown instructions. */
};

int parse_args(int argc, char **argv);

int open_input_file(void);
void close_input_file(void);
int open_output_file(void);
void close_output_file(void);

extern struct cmdline_args cmdargs;

#define IS_DEBUG		(cmdargs.debug > 0)
#define IS_VERBOSE_DEBUG	(cmdargs.debug > 1)
#define IS_INSANE_DEBUG		(cmdargs.debug > 2)

#endif /* B43_DASM_ARGS_H_ */
