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
#include <wrapper.h>	// wrapper definitions for functions that already exist in the firmware
#include <structs.h>	// structures that are used by the code in the firmware
#include <patcher.h>
#include <helper.h>
#include <ieee80211_radiotap.h>
#include <sendframe.h>
#include "d11.h"
#include "brcm.h"
#include <nexioctls.h>
#include <debug.h>

__attribute__((naked))
uint32
get_DBGDRAR(void)
{
    asm(
        "MRC p14, 0, r0, c1, c0, 0\n"
        "bx lr\n"
    );
}

__attribute__((naked))
uint32
get_DBGDIDR(void)
{
    asm(
        "MRC p14, 0, r0, c0, c0, 0\n"
        "bx lr\n"
    );
}

__attribute__((naked))
uint32
get_DBGDSAR(void)
{
    asm(
        "MRC p14, 0, r0, c2, c0, 0\n"
        "bx lr\n"
    );
}

__attribute__((naked))
uint32
get_DBGAUTHSTATUS(void)
{
    asm(
        "MRC p14, 0, r0, c7, c14, 6\n"
        "bx lr\n"
    );
}

void
si_attach_hook(uint32 r0, uint32 r1)
{
	printf("%s %08x %08x\n", __FUNCTION__, r0, r1);
}

__attribute__((at(0x20B85C, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
HookPatch4(si_attach_hook, si_attach_hook, "push {r0-r10,lr}");


void
si_lowpwr_opt_hook(uint32 r0, uint32 r1)
{
	printf("%s\n", __FUNCTION__);
}

__attribute__((at(0x16630, "flashpatch", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
HookPatch4(si_lowpwr_opt_hook, si_lowpwr_opt_hook, "push {r0-r2,r4-r7,lr}\nmov r4, r0");

/*
void
si_pmu_chipcontrol_hook(uint32 r0, uint32 r1, uint32 r2, uint32 r3)
{
	printf("pmu %08x %08x %08x %08x\n", r0, r1, r2, r3);
}

__attribute__((at(0x1A2738, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
HookPatch4(si_pmu_chipcontrol_hook, si_pmu_chipcontrol_hook, "push {r0-r2,r4-r7,lr}\nmov r6, r3");
*/

void
after_si_kattach_hook(void *r0, uint32 r1)
{
	printf("%s %08x %08x\n", __FUNCTION__, r0, r1);
	printf("dbg %08x %08x\n", (get_DBGDRAR() & 0xFFFFF000) + (get_DBGDSAR() & 0xFFFFF000), get_DBGDIDR());
//	printf("DBGAUTHSTATUS %08x\n", get_DBGAUTHSTATUS());
	si_swdenable((void *) 0x20326c, 1);
//	dbg_unlock_debug_registers();
//    dbg_disable_breakpoint(0);
//    dbg_disable_breakpoint(1);
//    dbg_disable_breakpoint(2);
//    dbg_disable_breakpoint(3);
//    dbg_enable_monitor_mode_debugging();
//	printf("dbg1 %08x %08x\n", *(uint32 *) (get_DBGDRAR() & 0xFFFFF000) + (get_DBGDSAR() & 0xFFFFF000), get_DBGDIDR());
//	printf("dbg1 %08x %08x\n", *(uint32 *) ((get_DBGDRAR() & 0xFFFFF000) + (get_DBGDSAR() & 0xFFFFF000)), get_DBGDIDR());
//	printf("dbg2 %08x %08x\n", *(uint32 *) 0x18007000, get_DBGDIDR());
}

__attribute__((at(0x203CB4, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
HookPatch4(after_si_kattach_hook, after_si_kattach_hook, "push {r0,r1,r4,lr}\nmov r1,0");

void
dma_attach_hook(uint32 r0, uint32 r1)
{
	printf("%s %08x %08x\n", __FUNCTION__, r0, r1);
	// si_pmu_chipcontrol(sih, PMU_CHIPCTL5, (1 << ARMCR4_DBG_CLK_BIT), (1 << ARMCR4_DBG_CLK_BIT));
//	si_pmu_chipcontrol((void *) 0x20326c, 5, 16, 16);
	// si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), CCS_FORCEHT, CCS_FORCEHT);
//    si_corereg((void *) 0x20326c, 0, 480, 2, 2);
    // si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, jtagctrl), JCTRL_TAPSEL_BIT, JCTRL_TAPSEL_BIT);
//    si_corereg((void *) 0x20326c, 0, 60, 8, 8);
    //si_corereg((void *) 0x20326c, 0, 60, 8 | 4 | 2 | 1, 8 | 4 | 2 | 1);
//	printf("dbg1 %08x %08x\n", *(uint32 *) ((get_DBGDRAR() & 0xFFFFF000) + (get_DBGDSAR() & 0xFFFFF000)), get_DBGDIDR());
}

__attribute__((at(0x20997C, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
HookPatch4(dma_attach_hook, dma_attach_hook, "push {r0-r2,r4-r11,lr}");

/*
void
si_swdenable_hook(uint32 r0, uint32 r1)
{
	printf("%s %08x %08x\n", __FUNCTION__, r0, r1);
}

__attribute__((at(0x20B492, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
HookPatch4(si_swdenable_hook, si_swdenable_hook, "push {r0,r1,r4,lr}\nmov r4, r0");
*/

/* activate swdenable
__attribute__((at(0x20B790, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
__attribute__((naked))
uint32
si_swdenable_on(void)
{
    asm(
        "nop\n"
        "nop\n"
        "nop\n"
        "nop\n"
        "nop\n"
        "nop\n"
        "nop\n"
        "nop\n"
        "mov r1,1\n"
    );
}
*/

void
si_kattach_hook(uint32 r0, uint32 r1)
{
	printf("%s %08x %08x\n", __FUNCTION__, r0, r1);
}

__attribute__((at(0x20B7F8, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
HookPatch4(si_kattach_hook, si_kattach_hook, "push {r4,r5,lr}");

void
stop(uint32 r0, uint32 r1, uint32 r2, uint32 r3)
{
	printf("%s: %08x\n", __FUNCTION__, r3);
    while(1);
}

//__attribute__((at(0x199E10, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
//GenericPatch4(stop, stop + 1);

//__attribute__((at(0x19CA26, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))

/*
__attribute__((at(0x19C90C, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
BPatch(stop, stop);

__attribute__((at(0x19C908, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
GenericPatch4(stop, 0x000068DB);
*/

void
test1(uint32 r0, uint32 r1)
{
	printf("test1 %08x %08x\n", r0, r1);
}

/*
__attribute__((at(0x19C858, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
HookPatch4(test1, test1, "push {r0-r2,r4-r11,lr}");
*/

/*
__attribute__((at(0x205262, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
HookPatch4(test1, test1, "push {r4-r11,lr}");
*/

void
test2(uint32 r0, uint32 r1)
{
	printf("test2 %08x %08x %08x\n", (*(uint32 *)(r0+60)), (*(uint32 *)((*(uint32 *)(r0+60))+16)), (*(uint32 *)((*(uint32 *)(r0+60))+16))+12);
}

/*
__attribute__((at(0x19C730, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
HookPatch4(test2, test2, "subs r2, r1, 0\n mov r3, r0");
*/

/*
__attribute__((at(0x203BE4, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
HookPatch4(test2, test2, "cmp r1, 0x700");
*/

int
adddevice(void)
{
	printf("adddevice\n");
	return 0;
}

#if 1
__attribute__((at(0x205418, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
BLPatch(adddevice, adddevice);

//__attribute__((at(0x205430, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
//GenericPatch2(binddev, 0);
#endif

int
wl_ioctl_hook(void *dev, uint32 cmd, void *buf, int len, int *used, int *needed, int set)
{
//	printf("%s %08x %d %d\n", __FUNCTION__, cmd, cmd, len);

	if (cmd == WLC_GET_VAR) {
//		printf("%s\n", buf);
		if (!strncmp(buf, "cur_etheraddr", 13)) {
//			printf("cur_etheraddr found\n");
			memcpy(buf, "NEXMON", 6);
			return 0;
		}
		if (!strncmp(buf, "clmload_status", 14)) {
//			printf("clmload_status found\n");
			return 0;
		}
		if (!strncmp(buf, "event_msgs", 14)) {
//			printf("event_msgs found\n");
			return 0;
		}
		if (!strncmp(buf, "chanspecs", 9)) {
//			printf("chanspecs found\n");
			((int *) buf)[0] = 0;
			((int *) buf)[1] = 0;
			return 0;
		}
		if (!strncmp(buf, "ver", 3)) {
//			printf("ver found\n");
			memcpy(buf, "wl0: nexmon nexmon version 1.1.1 FWID 01-00000000\n\0", 60);
			return 0;
		}
		return -23;
	}

	if (cmd == WLC_SET_VAR) {
//		printf("%s\n", buf);
		if (!strncmp(buf, "clmload", 7)) {
//			printf("clmload found\n");
			return 0;
		}
		if (!strncmp(buf, "mpc", 7)) {
//			printf("mpc found\n");
			return 0;
		}
		if (!strncmp(buf, "event_msgs", 14)) {
//			printf("event_msgs found\n");
			return 0;
		}
		if (!strncmp(buf, "bcn_timeout", 11)) {
//			printf("bcn_timeout found\n");
			return 0;
		}
		if (!strncmp(buf, "roam_off", 8)) {
//			printf("roam_off found\n");
			return 0;
		}
		return -23;
	}

	if (cmd == 0xb9) {
		return 0;
	}

	if (cmd == 0xbb) {
		return 0;
	}

	if (cmd == 0x1) {
//		printf("cmd 1\n");
//		printf("dbg1 %08x %08x\n", *(uint32 *) ((get_DBGDRAR() & 0xFFFFF000) + (get_DBGDSAR() & 0xFFFFF000)), get_DBGDIDR());
		return 0;
	}

	if (cmd == WLC_SET_PM) {
		return 0;
	}

	if (cmd == WLC_SET_ROAM_TRIGGER) {
		return 0;
	}

	if (cmd == WLC_SET_ROAM_DELTA) {
		return 0;
	}

	if (cmd == WLC_SET_INFRA) {
		return 0;
	}

	if (cmd == 0x8c) {
		int *bufi = (int *) buf;
		bufi[0] = 1;
		bufi[1] = 1;
		bufi[2] = 0;
		return 0;
	}

	return -23;
}

__attribute__((at(0x202248, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
GenericPatch4(wl_ioctl_hook, wl_ioctl_hook + 1);

int
wl_open_hook(void *dev)
{
	printf("%s: enter\n", __FUNCTION__);
	return -23;
}

__attribute__((at(0x202238, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
GenericPatch4(wl_open_hook, wl_open_hook + 1);

int
wl_send_hook(void *src, void *dev, void *lb)
{
	printf("%s: enter\n", __FUNCTION__);
	return 0;
}

__attribute__((at(0x202240, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
GenericPatch4(wl_send_hook, wl_send_hook + 1);
