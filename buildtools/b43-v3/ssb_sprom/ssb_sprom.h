/*

  Broadcom Sonics Silicon Backplane bus SPROM data modification tool

  Copyright (c) 2006-2007 Michael Buesch <m@bues.ch>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; see the file COPYING.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.

*/

#ifndef SSB_SPROMTOOL_H_
#define SSB_SPROMTOOL_H_

#include "utils.h"


#define SPROM_SIZE		128 /* bytes */
#define SPROM4_SIZE		440

enum valuetype {
	VAL_RAW,

	VAL_ET0PHY,
	VAL_ET1PHY,
	VAL_ET0MDC,
	VAL_ET1MDC,
	VAL_BREV,
	VAL_ANTBG0,
	VAL_ANTBG1,
	VAL_ANTBG2,
	VAL_ANTBG3,
	VAL_ANTA0,
	VAL_ANTA1,
	VAL_ANTA2,
	VAL_ANTA3,
	VAL_ANTGBG,
	VAL_ANTGA,
	VAL_ANTG0,
	VAL_ANTG1,
	VAL_ANTG2,
	VAL_ANTG3,
	VAL_TPI2G0,
	VAL_TPI2G1,
	VAL_TPI5GM0,
	VAL_TPI5GM1,
	VAL_TPI5GL0,
	VAL_TPI5GL1,
	VAL_TPI5GH0,
	VAL_TPI5GH1,
	VAL_2CCKPO,
	VAL_2OFDMPO,
	VAL_5MPO,
	VAL_5LPO,
	VAL_5HPO,
	VAL_2MCSPO,
	VAL_5MMCSPO,
	VAL_5LMCSPO,
	VAL_5HMCSPO,
	VAL_CCDPO,
	VAL_STBCPO,
	VAL_BW40PO,
	VAL_BWDUPPO,
	VAL_5HPAM,
	VAL_5LPAM,
	VAL_PA0B0,
	VAL_PA0B1,
	VAL_PA0B2,
	VAL_PA0B3,
	VAL_PA1B0,
	VAL_PA1B1,
	VAL_PA1B2,
	VAL_PA1B3,
	VAL_5MPA0,
	VAL_5MPA1,
	VAL_5MPA2,
	VAL_5MPA3,
	VAL_5LPA0,
	VAL_5LPA1,
	VAL_5LPA2,
	VAL_5LPA3,
	VAL_5HPA0,
	VAL_5HPA1,
	VAL_5HPA2,
	VAL_5HPA3,
	VAL_LED0,
	VAL_LED1,
	VAL_LED2,
	VAL_LED3,
	VAL_MAXPBG,
	VAL_MAXPA,
	VAL_ITSSIBG,
	VAL_ITSSIA,
	VAL_BGMAC,
	VAL_ETMAC,
	VAL_AMAC,
	VAL_SUBP,
	VAL_SUBV,
	VAL_PPID,
	VAL_BFLHI,
	VAL_BFL,
	VAL_REGREV,
	VAL_LOC,
 VAL_LAST = VAL_LOC,
};

#define BIT(i)  (1U << (i))

#define MASK_1  BIT(1)
#define MASK_2  BIT(2)
#define MASK_3  BIT(3)
#define MASK_4  BIT(4)
#define MASK_5  BIT(5)
#define MASK_8  BIT(8)

#define MASK_1_2  MASK_1 | MASK_2	/* Revs 1 - 2 */
#define MASK_1_3  MASK_1_2 | MASK_3	/* Revs 1 - 3 */
#define MASK_2_3  MASK_2 | MASK_3	/* Revs 2 - 3 */
#define MASK_4_5  MASK_4 | MASK_5	/* Revs 4 - 5 */
#define MASK_1_5  MASK_1_3 | MASK_4_5	/* Revs 1 - 5 */
#define MASK_1_8  MASK_1_5 | MASK_8	/* Revs 1 - 5, 8 */

struct cmdline_vparm {
	enum valuetype type;
	int set;
	int bits;
	union {
		uint32_t value;
		uint8_t mac[6];
		char ccode[2];
		struct {
			uint16_t value;
			uint16_t offset;
		} raw;
	} u;
};

struct cmdline_args {
	const char *infile;
	const char *outfile;
	int verbose;
	int force;
	int bin_mode;

#define MAX_VPARM	512
	struct cmdline_vparm vparm[MAX_VPARM];
	int nr_vparm;
};

struct var_entry {
	uint16_t rev_mask;
	enum valuetype type;
	uint16_t length;
	uint16_t offset;
	uint16_t mask;
	uint16_t shift;
	const char *desc;
	const char *label;
};

extern struct cmdline_args cmdargs;

#endif /* SSB_SPROMTOOL_H_ */
