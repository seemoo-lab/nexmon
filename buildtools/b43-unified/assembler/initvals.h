#ifndef BCM43xx_ASM_INITVALS_H_
#define BCM43xx_ASM_INITVALS_H_

#include "main.h"


struct initval_op {
	enum {
		IVAL_W_MMIO16,
		IVAL_W_MMIO32,
		IVAL_W_PHY,
		IVAL_W_RADIO,
		IVAL_W_SHM16,
		IVAL_W_SHM32,
		IVAL_W_TRAM,
	} type;
	unsigned int args[3];

	struct lineinfo info;

	struct list_head list;
};

struct initvals_sect {
	/* The name string for this initvals section */
	const char *name;
	/* List of initval operations */
	struct list_head ops;

	struct list_head list;
};


extern void assemble_initvals(void);

#endif /* BCM43xx_ASM_INITVALS_H_ */
