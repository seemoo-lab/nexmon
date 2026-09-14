#ifndef BCM43xx_ASM_MAIN_H_
#define BCM43xx_ASM_MAIN_H_

#include <stdint.h>

#include "list.h"
#include "util.h"


/* The header that fwcutter also puts in to every .fw file */
struct fw_header {
	/* Type of the firmware data */
	uint8_t type;
	/* Version number of the firmware data format */
	uint8_t ver;
	uint8_t __padding[2];
	/* Size of the data. For ucode and PCM this is in bytes.
	 * For IV this is in number-of-ivs. (big-endian!) */
	be32_t size;
} __attribute__ ((__packed__));

/* struct fw_header -> type */
#define FW_TYPE_UCODE	'u'
#define FW_TYPE_PCM	'p'
#define FW_TYPE_IV	'i'
/* struct fw_header -> ver */
#define FW_HDR_VER	0x01


/* Maximum number of allowed instructions in the code output.
 * This is what device memory can hold at maximum.
 */
#define NUM_INSN_LIMIT_R5	4096


struct lineinfo {
	char file[64];
	char linecopy[128];
	unsigned int lineno;
	unsigned int column;
};
extern struct lineinfo cur_lineinfo;


struct immediate {
	unsigned int imm;
};

struct address {
	unsigned int addr;
};

struct registr {
	int type; /* SPR, GPR or OFFR */
	unsigned int nr;
};

struct memory {
	enum {
		MEM_DIRECT,
		MEM_INDIRECT,
	} type;
	/* Offset immediate */
	unsigned int offset;
	/* Offset Register number (only MEM_INDIRECT) */
	unsigned int offr_nr;
};

struct label {
	const char *name;

	/* direction is only used for label references. */
	enum {
		LABELREF_ABSOLUTE,
		LABELREF_RELATIVE_BACK,
		LABELREF_RELATIVE_FORWARD,
	} direction;
};

struct operand {
	enum {
		OPER_IMM,
		OPER_REG,
		OPER_MEM,
		OPER_LABEL,
		OPER_ADDR,
		OPER_RAW,
	} type;
	union {
		struct immediate *imm;
		struct registr *reg;
		struct memory *mem;
		struct label *label;
		struct address *addr;
		unsigned int raw;
	} u;
};

struct operand_shift_mask {
	struct operand *op;
	unsigned int mask;
	unsigned int shift;
};

struct operlist {
	struct operand *oper[7];
};

struct instruction {
	int op;
	struct operlist *operands;
	unsigned int opcode; /* only for RAW */
};

struct asmdir {
	enum {
		ADIR_ARCH,
		ADIR_START,
	} type;
	union {
		unsigned int arch;
		struct label *start;
	} u;
};

struct statement {
	enum {
		STMT_INSN,
		STMT_LABEL,
		STMT_ASMDIR,
	} type;
	union {
		struct instruction *insn;
		struct label *label;
		struct asmdir *asmdir;
	} u;
	struct lineinfo info;

	struct list_head list;
};


struct file {
	/* The (microcode) statement list */
	struct list_head sl;
	/* The initvals sections list */
	struct list_head ivals;
	/* The file descriptor */
	int fd;
};


extern struct file infile;
extern const char *infile_name;
extern const char *outfile_name;

#endif /* BCM43xx_ASM_MAIN_H_ */
