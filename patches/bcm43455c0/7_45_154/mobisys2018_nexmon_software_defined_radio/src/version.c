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
 * Copyright (c) 2018 Matthias Schulz                                      *
 *                                                                         *
 * Permission is hereby granted, free of charge, to any person obtaining a *
 * copy of this software and associated documentation files (the           *
 * "Software"), to deal in the Software without restriction, including     *
 * without limitation the rights to use, copy, modify, merge, publish,     *
 * distribute, sublicense, and/or sell copies of the Software, and to      *
 * permit persons to whom the Software is furnished to do so, subject to   *
 * the following conditions:                                               *
 *                                                                         *
 * 1. The above copyright notice and this permission notice shall be       *
 *    include in all copies or substantial portions of the Software.       *
 *                                                                         *
 * 2. Any use of the Software which results in an academic publication or  *
 *    other publication which includes a bibliography must include         *
 *    citations to the nexmon project a) and the paper cited under b) or   *
 *    the thesis cited under c):                                           *
 *                                                                         *
 *    a) "Matthias Schulz, Daniel Wegemer and Matthias Hollick. Nexmon:    *
 *        The C-based Firmware Patching Framework. https://nexmon.org"     *
 *                                                                         *
 *    b) "Matthias Schulz, Jakob Link, Francesco Gringoli, and Matthias    *
 *        Hollick. Shadow Wi-Fi: Teaching Smart- phones to Transmit Raw    *
 *        Signals and to Extract Channel State Information to Implement    *
 *        Practical Covert Channels over Wi-Fi. Accepted to appear in      *
 *        Proceedings of the 16th ACM International Conference on Mobile   *
 *        Systems, Applications, and Services (MobiSys 2018), June 2018."  *
 *                                                                         *
 *    c) "Matthias Schulz. Teaching Your Wireless Card New Tricks:         *
 *        Smartphone Performance and Security Enhancements through Wi-Fi   *
 *        Firmware Modifications. Dr.-Ing. thesis, Technische Universität  *
 *        Darmstadt, Germany, February 2018."                              *
 *                                                                         *
 * 3. The Software is not used by, in cooperation with, or on behalf of    *
 *    any armed forces, intelligence agencies, reconnaissance agencies,    *
 *    defense agencies, offense agencies or any supplier, contractor, or   *
 *    research associated.                                                 *
 *                                                                         *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    *
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    *
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  *
 *                                                                         *
 **************************************************************************/

#pragma NEXMON targetregion "patch"

#include <firmware_version.h>   // definition of firmware version macros
#include <patcher.h>            // macros used to craete patches such as BLPatch, BPatch, ...

#if ((NEXMON_CHIP == CHIP_VER_BCM43455c0) && (NEXMON_FW_VERSION == FW_VER_7_45_154))
char version[] = "7.45.154 (nexmon.org/sdr: " GIT_VERSION "-" BUILD_NUMBER ")";
#else
char version[] = "(nexmon.org/sdr: " GIT_VERSION "-" BUILD_NUMBER ")";
#endif
char date[] = __DATE__;
char time[] = __TIME__;

__attribute__((at(0x1a2048, "", CHIP_VER_BCM43455c0, FW_VER_7_45_154)))
GenericPatch4(version_patch, version);

__attribute__((at(0x1a2054, "", CHIP_VER_BCM43455c0, FW_VER_7_45_154)))
GenericPatch4(date_patch, date);

__attribute__((at(0x1a2044, "", CHIP_VER_BCM43455c0, FW_VER_7_45_154)))
GenericPatch4(time_patch, time);
