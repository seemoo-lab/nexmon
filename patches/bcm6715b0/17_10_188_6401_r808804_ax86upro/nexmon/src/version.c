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
 * Copyright (c) 2023 NexMon Team                                          *
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
#include <patcher.h>            // macros used to create patches such as BLPatch, BPatch, ...

char date[] = DATE;
__attribute__((at(DATE_PTR, "", CHIP_VER_BCM6715b0, FW_VER_17_10_188_6401_r808804_ax86upro)))
GenericPatch4(date_patch, date);

char time[] = TIME;
__attribute__((at(TIME_PTR, "", CHIP_VER_BCM6715b0, FW_VER_17_10_188_6401_r808804_ax86upro)))
GenericPatch4(time_patch, time);

char version[] = "17.10.188.6401 (r808804 nexmon.org: " GIT_VERSION "-" BUILD_NUMBER ")";
__attribute__((at(VERSION_PTR_1, "", CHIP_VER_BCM6715b0, FW_VER_17_10_188_6401_r808804_ax86upro)))
GenericPatch4(version_patch_1, version);
__attribute__((at(VERSION_PTR_2, "", CHIP_VER_BCM6715b0, FW_VER_17_10_188_6401_r808804_ax86upro)))
GenericPatch4(version_patch_2, version);
__attribute__((at(VERSION_PTR_3, "", CHIP_VER_BCM6715b0, FW_VER_17_10_188_6401_r808804_ax86upro)))
GenericPatch4(version_patch_3, version);
__attribute__((at(VERSION_PTR_4, "", CHIP_VER_BCM6715b0, FW_VER_17_10_188_6401_r808804_ax86upro)))
GenericPatch4(version_patch_4, version);
