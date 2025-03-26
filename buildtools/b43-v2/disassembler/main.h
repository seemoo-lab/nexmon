#ifndef B43_DASM_MAIN_H_
#define B43_DASM_MAIN_H_

#include <stdio.h>

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


extern FILE *infile;
extern FILE *outfile;
extern const char *infile_name;
extern const char *outfile_name;

#endif /* B43_DASM_MAIN_H_ */
