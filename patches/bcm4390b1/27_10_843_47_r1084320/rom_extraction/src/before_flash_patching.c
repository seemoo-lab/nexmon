/***************************************************************************
 *                                                                         *
 *          ###########   ###########   ##########    ##########           *
 *         ############  ############  ############  ############          *
 *         ##            ##            ##   ##   ##  ##        ##          *
 *         ##            ##            ##   ##   ##  ##        ##          *
 *         ###########   ####  ######  ##   ##   ##  ##    ######          *
 *          ###########  ####  #       ##   ##   ##  ##    #    #          *
 *                   ##  ##    ######  ##   ##   ##  ##    #    #          *
 *                   ##  ##    #       ##   ##   ##  ##    #    #          *
 *         ############  ##### ######  ##   ##   ##  ##### ######          *
 *         ###########    ###########  ##   ##   ##   ##########           *
 *                                                                         *
 *            S E C U R E   M O B I L E   N E T W O R K I N G              *
 *                                                                         *
 * This file is part of NexMon.                                            *
 *                                                                         *
 * Copyright (c) 2025 NexMon Team                                          *
 *                                                                         *
 * NexMon is free software: you can redistribute it and/or modify          *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation, either version 3 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * NexMon is distributed in the hope that it will be useful,               *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with NexMon. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 **************************************************************************/

#pragma NEXMON targetregion "patch"

#include <firmware_version.h>   // definition of firmware version macros
#include <wrapper.h>            // wrapper definitions for functions that already exist in the firmware
#include <structs.h>            // structures that are used by the code in the firmware
#include <patcher.h>            // macros used to create patches such as BLPatch, BPatch, ...

struct fp_config {
	unsigned int *target_addr;
	unsigned int data_ptr;
};

unsigned int fp_orig_data[(FP_CONFIG_ORIGEND - FP_CONFIG_ORIGBASE) / sizeof(struct fp_config)][5] = { 0 };
unsigned int fp_orig_data_len = (FP_CONFIG_ORIGEND - FP_CONFIG_ORIGBASE) / sizeof(struct fp_config);

void
hnd_tcam_load_default_hook(unsigned int rambase)
{
	struct fp_config *fpc = (struct fp_config *) FP_CONFIG_ORIGBASE;
	int i;

	for (i = 0; i < fp_orig_data_len; i++) {
		fp_orig_data[i][0] = (unsigned int) (fpc)->target_addr;
		fp_orig_data[i][1] = ((fpc)->target_addr)[0];
		fp_orig_data[i][2] = ((fpc)->target_addr)[1];
		fp_orig_data[i][3] = ((fpc)->target_addr)[2];
		fp_orig_data[i][4] = ((fpc)->target_addr)[3];
		fpc++;
	}

	hnd_tcam_load_default(rambase);
}

__attribute__((at(0x41E4A6, "", CHIP_VER_BCM4390b1, FW_VER_27_10_843_47_r1084320)))
BPatch(hnd_tcam_load_default, hnd_tcam_load_default_hook);
