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

#include <firmware_version.h>
#include <channels.h>
#include <patcher.h>
#include <wrapper.h>
#include <channels.h>

// Nop the following call to keep user tx power targets
//    Choose least of user and now combined regulatory/hw targets
//    ppr_compare_min(tx_pwr_target, srom_max_txpwr);
__attribute__((at(0x1C50B8, "", CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704)))
GenericPatch4(nop_ppr_compare_min, 0);

// This allows 80 MHz channels in the 2 GHz band
int
wf_chspec_malformed_hook(unsigned short chanspec)
{
    return 0;
}

__attribute__((at(0x13778, "flashpatch", CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704)))
BPatch(wf_chspec_malformed_hook, wf_chspec_malformed_hook);

unsigned short additional_valid_chanspecs[] = {
  CH20MHZ_CHSPEC(1),
  CH20MHZ_CHSPEC(2),
  CH20MHZ_CHSPEC(3),
  CH20MHZ_CHSPEC(4),
  CH20MHZ_CHSPEC(5),
  CH20MHZ_CHSPEC(6),
  CH20MHZ_CHSPEC(7),
  CH20MHZ_CHSPEC(8),
  CH20MHZ_CHSPEC(9),
  CH20MHZ_CHSPEC(10),
  CH20MHZ_CHSPEC(11),
  CH20MHZ_CHSPEC(12),
  CH20MHZ_CHSPEC(13),
  CH20MHZ_CHSPEC(14),
  CH80MHZ_CHSPEC(6, WL_CHANSPEC_CTL_SB_L),
  CH20MHZ_CHSPEC(7),
  CH40MHZ_CHSPEC(7, WL_CHANSPEC_CTL_SB_L),
  CH80MHZ_CHSPEC(7, WL_CHANSPEC_CTL_SB_L),
  CH20MHZ_CHSPEC(106),
  CH40MHZ_CHSPEC(106, WL_CHANSPEC_CTL_SB_L),
  CH80MHZ_CHSPEC(106, WL_CHANSPEC_CTL_SB_L),
  CH20MHZ_CHSPEC(120),
  CH20MHZ_CHSPEC(140),
};

int
wlc_valid_chanspec_ext_hook(void *wlc_cm, unsigned short chanspec, int dualband)
{
  int valid = wlc_valid_chanspec_ext(wlc_cm, chanspec, dualband);
  int i;

  if (!valid && dualband == 1)
    for (i = 0; i < sizeof(additional_valid_chanspecs)/sizeof(additional_valid_chanspecs[0]); i++)
      valid |= additional_valid_chanspecs[i] == chanspec;
    
    return valid;
}

__attribute__((at(0x5BA28, "flashpatch", CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704)))
BPatch(wlc_valid_chanspec_ext, wlc_valid_chanspec_ext_hook)
