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

#ifndef FIRMWARE_VERSION_H
#define FIRMWARE_VERSION_H

#define CHIP_VER_ALL                        0
#define CHIP_VER_BCM4339                    1
#define CHIP_VER_BCM4330                    2
#define CHIP_VER_BCM4358                    3
#define CHIP_VER_BCM43438                   4
#define CHIP_VER_BCM43430a1                 4
#define CHIP_VER_BCM4356                    5
#define CHIP_VER_BCM4335b0                  6
#define CHIP_VER_BCM43596a0                 7
#define CHIP_VER_BCM43451b1                 8
#define CHIP_VER_BCM43455                   9
#define CHIP_VER_BCM43455c0               101
#define CHIP_VER_BCM43909b0               102
#define CHIP_VER_BCM4366c                 103
#define CHIP_VER_BCM4361b0                104
#define CHIP_VER_BCM4375b1                105
#define CHIP_VER_BCM4366c0                106
#define CHIP_VER_BCM43436b0               107
#define CHIP_VER_BCM4389c1                108
#define CHIP_VER_BCM43439a0               109
#define CHIP_VER_BCM4398d0               1001
#define CHIP_VER_BCM6715b0               1002
#define CHIP_VER_BCM43684b0              1003
#define CHIP_VER_BCM43684c0              1004
#define CHIP_VER_BCM4390b1               1005

#define FW_VER_ALL                          0

// for CHIP_VER_BCM4339
#define FW_VER_6_37_32_RC23_34_40_r581243   10
#define FW_VER_6_37_32_RC23_34_43_r639704   11
#define FW_VER_6_37_32_34_1_mfg             12

// for CHIP_VER_BCM4330
#define FW_VER_5_90_195_114                 20
#define FW_VER_5_90_100_41                  21

// for CHIP_VER_BCM4358
#define FW_VER_7_112_200_17                 30
#define FW_VER_7_112_201_3                  31
#define FW_VER_7_112_300_14                 32

// for CHIP_VER_BCM43438 (wrongly labled) BCM43430a1
#define FW_VER_7_45_41_26_r640327           40
#define FW_VER_7_45_41_46                   41

// for CHIP_VER_BCM4356
#define FW_VER_7_35_101_5_sta               50
#define FW_VER_7_35_101_5_apsta             51

// for CHIP_VER_BCM4335b0
#define FW_VER_6_30_171_1_sta               60

// for CHIP_VER_BCM43596a0
#define FW_VER_9_75_155_45_sta_c0           70
#define FW_VER_9_96_4_sta_c0                71

// for CHIP_VER_BCM43451b1
#define FW_VER_7_63_43_0                    80

// for CHIP_VER_BCM43455
#define FW_VER_7_45_77_0                    90
#define FW_VER_7_120_5_1_sta_C0             91
#define FW_VER_7_120_7_1_sta_C0             92
#define FW_VER_7_45_77_0_23_8_2017          93
#define FW_VER_7_46_77_11                   94
#define FW_VER_7_45_59_16                   95

// for CHIP_VER_BCM43455c0
#define FW_VER_7_45_154                    110
#define FW_VER_7_45_189                    111
#define FW_VER_7_45_206                    112
#define FW_VER_7_45_241                    113
#define FW_VER_7_45_234_4ca95bb_CY         114
#define FW_VER_7_45_265_28bca26_CY         115

// for CHIP_VER_BCM43909b0
#define FW_VER_7_15_168_108                210

// for CHIP_VER_BCM4366c
#define FW_VER_10_10_69_252                310
#define FW_VER_10_10_122_20                311
#define FW_VER_10_28_2                     312

// for CHIP_VER_BCM4361b0
#define FW_VER_13_38_55_1_sta              410
#define FW_VER_13_38_55_1_mfg              411

// for CHIP_VER_BCM4375b1
#define FW_VER_18_38_18_sta                510
#define FW_VER_18_38_24_mon                511
#define FW_VER_18_38_24_mfg                512
#define FW_VER_18_38_18_mon                513
#define FW_VER_18_38_18_mfg                514
#define FW_VER_18_40_42_sta                515
#define FW_VER_18_41_8_9_sta               516
#define FW_VER_18_41_113_sta               517
#define FW_VER_18_41_117_sta               518

// for CHIP_VER_BCM43436b0
#define FW_VER_9_88_4_65                   710

// for CHIP_VER_BCM4389c1
#define FW_VER_20_101_36_2                 810
#define FW_VER_20_82_42_r985250_sta        811
#define FW_VER_20_101_57_r1035009          812
#define FW_VER_20_80_87_3_r1050079_sta     813

// for CHIP_VER_BCM43439a0
#define FW_VER_7_95_49_2271bb6             910

// for CHIP_VER_BCM4398d0
#define FW_VER_24_671_6_9_r1031525        1010
#define FW_VER_24_67101_26_10_r1048778    1011
#define FW_VER_24_67103_33_1_r1056022     1012
#define FW_VER_24_100_671_55_r1085768     1013
#define FW_VER_24_100_671_59_r1091848     1014

// define CHIP_VER_BCM6715b0
#define FW_VER_17_10_188_6401_r808804_ax86upro 1020
#define FW_VER_17_10_188_6401_r808804_ax82uv2  1021

// define CHIP_VER_BCM43684b0
#define FW_VER_17_10_157_2809_r801046_ax86u 1030
#define FW_VER_17_10_121_41_r796515_ax88u   1031

// define CHIP_VER_BCM43684c0
#define FW_VER_17_10_157_2809_r801046_axe11000 1040

// for CHIP_VER_BCM4390b1
#define FW_VER_27_10_843_47_r1084320      1050

#endif /*FIRMWARE_VERSION_H*/
