#ifndef B43_DASM_MAIN_H_
#define B43_DASM_MAIN_H_

#include <stdio.h>

#include "util.h"

/* supported archs */
typedef enum {
	ARCH5 = 5,	// 4318
	ARCH15 = 15,	// 43224, 4339 (Nexus5), 4360, 43455c0 (Raspberry PI)
	ARCH65 = 65,	// 4365, 4366 (Asus rt-ac86u)
	ARCH129 = 129,	// 43684 (Asus rt-ax86u)
	ARCH132 = 132,  // 6715b0 (Asus rt-ax86u-pro)
	ARCH134 = 134,	// Asus rt-be96u and includes Google Pixel 8 as subarch
	ARCHINVALID = 135,
} arch_t;

typedef enum {
	NOSUBARCH = 0,
	SUBARCH1 = 1,
	SUBARCHINVALID = 2,
} subarch_t;

typedef struct subarch_valid {
	arch_t arch;
	subarch_t subarch;
} subarch_valid_t;

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


extern FILE *infile;
extern FILE *outfile;
extern const char *infile_name;
extern const char *outfile_name;

#endif /* B43_DASM_MAIN_H_ */
