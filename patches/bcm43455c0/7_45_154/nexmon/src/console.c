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

#include <firmware_version.h>   // definition of firmware version macros
#include <patcher.h>            // macros used to craete patches such as BLPatch, BPatch, ...

__attribute__((at(0x1E9418, "", CHIP_VER_BCM4358, FW_VER_7_112_200_17)))
__attribute__((at(0x1E9518, "", CHIP_VER_BCM4358, FW_VER_7_112_201_3)))
__attribute__((at(0x1ea848, "", CHIP_VER_BCM4358, FW_VER_7_112_300_14)))
__attribute__((at(0x200170, "", CHIP_VER_BCM43455c0, FW_VER_7_45_154)))
__attribute__((naked))
void
patch_console_size_1(void)
{
	asm("mov r0, 0x800\n");
}

__attribute__((at(0x1E9434, "", CHIP_VER_BCM4358, FW_VER_7_112_200_17)))
__attribute__((at(0x1E9534, "", CHIP_VER_BCM4358, FW_VER_7_112_201_3)))
__attribute__((at(0x1ea864, "", CHIP_VER_BCM4358, FW_VER_7_112_300_14)))
__attribute__((at(0x20018C, "", CHIP_VER_BCM43455c0, FW_VER_7_45_154)))
__attribute__((naked))
void
patch_console_size_2(void)
{
	asm("mov r2, 0x800\n");
}
