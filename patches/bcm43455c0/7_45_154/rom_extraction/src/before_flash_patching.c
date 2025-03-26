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
 * Copyright (c) 2016 NexMon Team                                          *
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
#include <debug.h>              // contains macros to access the debug hardware
#include <wrapper.h>            // wrapper definitions for functions that already exist in the firmware
#include <structs.h>            // structures that are used by the code in the firmware
#include <helper.h>             // useful helper functions
#include <patcher.h>            // macros used to craete patches such as BLPatch, BPatch, ...
#include <rates.h>              // rates used to build the ratespec for frame injection
#include <local_wrapper.h>

unsigned int fp_orig_data[247][2] = { 0 };
unsigned int fp_orig_data_len = 247;

struct fp_config {
	unsigned int *target_addr;
	unsigned int size;
	unsigned int data_ptr;
};

int
fp_apply_patches_hook(void)
{
	struct fp_config *fpc = (struct fp_config *) 0x199000;
	int i;

	for (i = 0; i < fp_orig_data_len; i++) {
		fp_orig_data[i][0] = (unsigned int) (fpc)->target_addr;
		fp_orig_data[i][1] = *((fpc++)->target_addr);
	}

	return fp_apply_patches();
}

// Hook call to fp_apply_patches in c_main
__attribute__((at(0x1bc51a, "", CHIP_VER_BCM43451b1, FW_VER_7_63_43_0)))
__attribute__((at(0x19ca50, "", CHIP_VER_BCM43455c0, FW_VER_7_45_154)))
BLPatch(fp_apply_patches, fp_apply_patches_hook);
