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
#include <debug.h>              // contains macros to access the debug hardware
#include <wrapper.h>            // wrapper definitions for functions that already exist in the firmware
#include <structs.h>            // structures that are used by the code in the firmware
#include <helper.h>             // useful helper functions
#include <patcher.h>            // macros used to craete patches such as BLPatch, BPatch, ...
#include <rates.h>              // rates used to build the ratespec for frame injection
#include <nexioctls.h>          // ioctls added in the nexmon patch
#include <version.h>            // version information
#include <argprintf.h>          // allows to execute argprintf to print into the arg buffer
#include <channels.h>

#define PHYREF_SamplePlayStartPtr           u.d11acregs.SamplePlayStartPtr
#define PHYREF_SamplePlayStopPtr            u.d11acregs.SamplePlayStopPtr
#define PHYREF_SampleCollectPlayCtrl        u.d11acregs.SampleCollectPlayCtrl

#define wreg32(r, v)        (*(volatile uint32*)(r) = (uint32)(v))
#define rreg32(r)       (*(volatile uint32*)(r))
#define wreg16(r, v)        (*(volatile uint16*)(r) = (uint16)(v))
#define rreg16(r)       (*(volatile uint16*)(r))
#define wreg8(r, v)     (*(volatile uint8*)(r) = (uint8)(v))
#define rreg8(r)        (*(volatile uint8*)(r))

#define BCM_REFERENCE(data) ((void)(data))

#define W_REG(osh, r, v) do { \
    BCM_REFERENCE(osh); \
    switch (sizeof(*(r))) { \
    case sizeof(uint8): wreg8((void *)(r), (v)); break; \
    case sizeof(uint16):    wreg16((void *)(r), (v)); break; \
    case sizeof(uint32):    wreg32((void *)(r), (v)); break; \
    } \
} while (0)

#define R_REG(osh, r) ({ \
    __typeof(*(r)) __osl_v; \
    BCM_REFERENCE(osh); \
    switch (sizeof(*(r))) { \
    case sizeof(uint8): __osl_v = rreg8((void *)(r)); break; \
    case sizeof(uint16):    __osl_v = rreg16((void *)(r)); break; \
    case sizeof(uint32):    __osl_v = rreg32((void *)(r)); break; \
    } \
    __osl_v; \
})

static void
exp_set_gains_by_index(struct phy_info *pi, int8 index)
{
    ac_txgain_setting_t gains = { 0 };
    wlc_phy_txpwrctrl_enable_acphy(pi, 0);
    wlc_phy_get_txgain_settings_by_index_acphy(pi, &gains, index);
    wlc_phy_txcal_txgain_cleanup_acphy(pi, &gains);
}

int 
wlc_ioctl_hook(struct wlc_info *wlc, int cmd, char *arg, int len, void *wlc_if)
{
    int ret = IOCTL_ERROR;
    argprintf_init(arg, len);

    switch(cmd) {

    case NEX_WRITE_TEMPLATE_RAM: // write template ram
        {
            struct params {
                int32 offset;
                int32 length;
                int8 data[];
            };

            struct params *params = (struct params *) arg;

            set_scansuppress(wlc, 1);
            set_mpc(wlc, 0);

            set_chanspec(wlc, CH20MHZ_CHSPEC(1));
            struct phy_info *pi = wlc->band->pi;

            if (len >= 8) {
                int32 *arg32 = (int32 *) arg; // 0: offset, 1: length, 2: data
                printf("offset=%d length=%d data[0]=%08x\n", arg32[0], arg32[1], arg32[3]);
                if (len >= 8 + arg32[1]) {
                    wlc_bmac_write_template_ram(wlc->hw, arg32[0], arg32[1], &arg32[2]);
                    ret = IOCTL_SUCCESS;
                }
            }
/*
            if (len >= 8) {
                if (len >= 8 + params->length) {
                    printf("%d %d %08x\n", params->offset, params->length, params->data);
                    wlc_bmac_write_template_ram(wlc->hw, params->offset, params->length, params->data);
                    ret = IOCTL_SUCCESS;
                }
            }
*/
        }
        break;

    case NEX_SDR_START_TRANSMISSION:
        {
            struct params {
                int32 num_samps;
                int32 start_offset;
                int32 chanspec;
                int32 power_index;
                int32 endless;
            };

            struct params *params = (struct params *) arg;

            if (len < 20) {
                break;
            }

            set_scansuppress(wlc, 1);
            set_mpc(wlc, 0);

            set_chanspec(wlc, params->chanspec);
            struct phy_info *pi = wlc->band->pi;

            wlc_phy_stay_in_carriersearch_acphy(pi, 1);

            exp_set_gains_by_index(pi, params->power_index);

            W_REG(pi->sh->osh, &pi->regs->PHYREF_SamplePlayStartPtr, (params->start_offset & 0xFFFF));
            W_REG(pi->sh->osh, &pi->regs->PHYREF_SamplePlayStopPtr, ((params->start_offset + params->num_samps) & 0xFFFF));

            W_REG(pi->sh->osh, &pi->regs->psm_phy_hdr_param, 0);

            wlc_phy_runsamples_acphy(pi, params->num_samps, 1, 0, 0, 1);

            if (params->endless) {
                printf("aaaaaaaaaaaaaaaa\n");
#if (NEXMON_CHIP == CHIP_VER_BCM4339)
                W_REG(pi->sh->osh, &pi->regs->psm_phy_hdr_param, (1 << 11) | (1 << 1));
#elif (NEXMON_CHIP == CHIP_VER_BCM43455c0)
                W_REG(pi->sh->osh, &pi->regs->PHYREF_SampleCollectPlayCtrl, (1 << 9));
#endif
            } else {
                printf("bbbbbbbbbbbbbbbb\n");
#if (NEXMON_CHIP == CHIP_VER_BCM4339)
                W_REG(pi->sh->osh, &pi->regs->psm_phy_hdr_param, (1 << 12) | (1 << 11) | (1 << 1));
#elif (NEXMON_CHIP == CHIP_VER_BCM43455c0)
                W_REG(pi->sh->osh, &pi->regs->PHYREF_SampleCollectPlayCtrl, (1 << 9)); // need to find out which additional bit needs to be set to only transmit once
#endif
                if (CHSPEC_IS80(pi->radio_chanspec))
                    udelay(params->num_samps/160);
                else if (CHSPEC_IS40(pi->radio_chanspec))
                    udelay(params->num_samps/80);
                else
                    udelay(params->num_samps/40);
#if (NEXMON_CHIP == CHIP_VER_BCM4339)
                W_REG(pi->sh->osh, &pi->regs->psm_phy_hdr_param, 0);
#elif (NEXMON_CHIP == CHIP_VER_BCM43455c0)
                W_REG(pi->sh->osh, &pi->regs->PHYREF_SampleCollectPlayCtrl, 0);
#endif
            }

            ret = IOCTL_SUCCESS;
        }
        break;

    case NEX_SDR_STOP_TRANSMISSION:
        {
            struct phy_info *pi = wlc->band->pi;
            wlc_phy_stopplayback_acphy(pi);
            ret = IOCTL_SUCCESS;
        }
        break;

    case 777:
        {
            uint32 tplramdump[16] = { 0xffffffff };
            uint32 oldvalue = 0;
            struct phy_info *pi = wlc->band->pi;
            
            set_scansuppress(wlc, 1);
            set_mpc(wlc, 0);

            wlc_phyreg_enter(pi);

            int i = 0;

            W_REG(pi->sh->osh, &pi->regs->tplatewrptr, 256*1024-5*4);
            W_REG(pi->sh->osh, &pi->regs->tplatewrdata, 0xaabbeeff);
            W_REG(pi->sh->osh, &pi->regs->tplatewrptr, 256*1024-1*4);
            W_REG(pi->sh->osh, &pi->regs->tplatewrdata, 0xaabbeedd);
            W_REG(pi->sh->osh, &pi->regs->tplatewrptr, 256*1024);
            W_REG(pi->sh->osh, &pi->regs->tplatewrdata, 0xaabbeecc);

            W_REG(pi->sh->osh, &pi->regs->tplatewrptr, 0);
            for (i = 0; i < sizeof(tplramdump)/sizeof(tplramdump[0]); i++) {
                tplramdump[i] = R_REG(pi->sh->osh, &pi->regs->tplatewrdata);
            }
            hexdump("xxx", tplramdump, sizeof(tplramdump));

            W_REG(pi->sh->osh, &pi->regs->tplatewrptr, 256*1024-8*4);
            for (i = 0; i < sizeof(tplramdump)/sizeof(tplramdump[0]); i++) {
                tplramdump[i] = R_REG(pi->sh->osh, &pi->regs->tplatewrdata);
            }
            hexdump("xxx2", tplramdump, sizeof(tplramdump));
            printf("\n");

            wlc_phyreg_exit(pi);

            ret = IOCTL_SUCCESS;
        }
        break;

    default:
        {
            ret = wlc_ioctl(wlc, cmd, arg, len, wlc_if);
        }
    }

    return ret;
}

__attribute__((at(0x1F3488, "", CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704)))
__attribute__((at(0x208F20, "", CHIP_VER_BCM43455c0, FW_VER_7_45_154)))
GenericPatch4(wlc_ioctl_hook, wlc_ioctl_hook + 1);
