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

#include <firmware_version.h>
#include <wrapper.h>	// wrapper definitions for functions that already exist in the firmware
#include <structs.h>	// structures that are used by the code in the firmware
#include <patcher.h>
#include <helper.h>
#include <capabilities.h>      // capabilities included in a nexmon patch
#include <objmem.h>

unsigned int objaddr = 0x3000000;

void
wlc_ucode_write_compressed(struct wlc_hw_info *wlc_hw, const int ucode[], const unsigned int nbytes);

void
my_wlc_ucode_write_byte(struct wlc_hw_info *wlc_hw, const unsigned char ucode[], const unsigned int nbytes)
{
/*
	volatile struct d11regs *regs = wlc_hw->regs;
	unsigned int i;
	unsigned int ucode_word;

	regs->objaddr = objaddr;
	for (i = 0; i < nbytes; i += 7) {
		ucode_word = ucode[i+3] << 24;
		ucode_word = ucode_word | (ucode[i+4] << 16);
		ucode_word = ucode_word | (ucode[i+5] << 8);
		ucode_word = ucode_word | (ucode[i+6] << 0);
		regs->objdata = ucode_word;

		if (i == 0) printf("0: %08x\n", ucode_word);

		ucode_word = ucode[i+0] << 16;
		ucode_word = ucode_word | (ucode[i+1] << 8);
		ucode_word = ucode_word | (ucode[i+2] << 0);
		regs->objdata = ucode_word;

		if (i == 0) printf("1: %08x\n", ucode_word);
	}
*/
	unsigned int a1 = 0;
	unsigned int a2 = 0;
	printf("%s: Enter\n", __FUNCTION__);
	wlc_ucode_write_compressed(wlc_hw, (int *) ucode, nbytes);
	wlc_bmac_read_objmem64(wlc_hw, 0, &a1, &a2, 0);
	printf("%08x %08x\n", a1, a2);
	printf("%s: Exit\n", __FUNCTION__);
}

//__attribute__((at(0x44ED0, "", CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327)))
//BLPatch(my_wlc_ucode_write_byte, my_wlc_ucode_write_byte);
