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
#include <patcher.h>            // macros used to craete patches such as BLPatch, BPatch, ...

char version[] = "7.45.189 (nexmon.org: " GIT_VERSION "-" BUILD_NUMBER ")";
char date[] = __DATE__;
char time[] = __TIME__;

__attribute__((at(0x1a2048, "", CHIP_VER_BCM43455c0, FW_VER_7_45_154)))
__attribute__((at(0x1A6DCC, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
GenericPatch4(version_patch, version);

__attribute__((at(0x1a2054, "", CHIP_VER_BCM43455c0, FW_VER_7_45_154)))
__attribute__((at(0x1A6DD8, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
GenericPatch4(date_patch, date);

__attribute__((at(0x1a2044, "", CHIP_VER_BCM43455c0, FW_VER_7_45_154)))
__attribute__((at(0x1A6DC8, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
GenericPatch4(time_patch, time);
