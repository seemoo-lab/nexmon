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

__attribute__((at(0xd7a00, "", CHIP_VER_BCM4335C0_BT, FW_VER_1_BT)))
__attribute__((optimize("O0")))
void
test_func(void) {
	int *ram_dest = (int *) 0xd7b00;
	int *rom_src = (int *) 0;

    while(rom_src <= (int *) 512) {
        *ram_dest++ = *rom_src++;
    }
}

//// insert as vendor specific hci cmd: 0xfc59
//__attribute__((at(0x262204, "", CHIP_VER_BCM4335C0_BT, FW_VER_1_BT)))
//GenericPatch4(test_func, test_func + 1);

// patch code of hci handler function (hci cmd 0xfc61)
__attribute__((at(0x1E5C4, "", CHIP_VER_BCM4335C0_BT, FW_VER_1_BT)))
BPatch(test_func, test_func + 1);
