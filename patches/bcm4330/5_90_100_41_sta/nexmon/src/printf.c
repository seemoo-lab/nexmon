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
 * Warning:                                                                *
 *                                                                         *
 * Our software may damage your hardware and may void your hardware’s      *
 * warranty! You use our tools at your own risk and responsibility!        *
 *                                                                         *
 * License:                                                                *
 * Copyright (c) 2015 NexMon Team                                          *
 *                                                                         *
 * Permission is hereby granted, free of charge, to any person obtaining   *
 * a copy of this software and associated documentation files (the         *
 * "Software"), to deal in the Software without restriction, including     *
 * without limitation the rights to use, copy, modify, merge, publish,     *
 * distribute copies of the Software, and to permit persons to whom the    *
 * Software is furnished to do so, subject to the following conditions:    *
 *                                                                         *
 * The above copyright notice and this permission notice shall be included *
 * in all copies or substantial portions of the Software.                  *
 *                                                                         *
 * Any use of the Software which results in an academic publication or     *
 * other publication which includes a bibliography must include a citation *
 * to the author's publication "M. Schulz, D. Wegemer and M. Hollick.      *
 * NexMon: A Cookbook for Firmware Modifications on Smartphones to Enable  *
 * Monitor Mode.".                                                         *
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

#define htons(A) ((((uint16)(A) & 0xff00) >> 8) | (((uint16)(A) & 0x00ff) << 8))

struct ethernet_ipv6_udp_header {
//	struct bdc_header bdc;
    struct ethernet_header ethernet;
    struct ipv6_header ipv6;
    struct udp_header udp;
} __attribute__((packed));

struct ethernet_ipv6_udp_header frm = {
//	.bdc = {
//		.flags = 0x20,
//		.priority = 0x00,
//		.flags2 = 0x00,
//		.dataOffset = 0x00
//	},
	.ethernet = {
		.dst = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
		.src = { 'N', 'E', 'X', 'M', 'O', 'N' },
		.type = 0xdd86
	},
	.ipv6 = {
		.version_traffic_class_flow_label = 0x60,
		.payload_length = htons(8),
		.next_header = 0x88,
		.hop_limit = 1,
		.src_ip = { 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 },
		.dst_ip = { 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }
	},
	.udp = {
		.src_port = htons(55000),
		.dst_port = htons(55000),
		.len_chk_cov.checksum_coverage = htons(8),
		.checksum = htons(0x535f)
	}
};

int protected = 1;

void
send_console_over_udp(void *buf, int len)
{
	struct wl_info *wl = (struct wl_info *) 0x439d8;
	struct sk_buff *p = pkt_buf_get_skb(wl->wlc->osh, sizeof(frm) + len);
//	void *sdio = (void *) 0x43aa8;
	frm.ipv6.payload_length = htons(8 + len);
	memcpy(p->data, &frm, sizeof(frm));
	memcpy(((char *) p->data) + sizeof(frm), buf, len);
//	dngl_sendpkt(sdio, p, 2);
	wl_sendup(wl, 0, p);
}

__attribute__((naked))
int
called_by_printf(void *buf, int a1, int a2, int a3)
{
	asm("b 0x809680");
}

int
called_by_printf_hook(void *buf, int a1, int a2, int a3)
{
	int len = called_by_printf(buf, a1, a2, a3);
	
	if (protected == 0) {
		protected = 1;
		send_console_over_udp(buf, len);
		protected = 0;
	}

	return len;
}

// reduce the amount of ucode memory freed to become part of the heap
__attribute__((at(0x80946C, "flashpatch", CHIP_VER_BCM4330, FW_VER_ALL)))
BLPatch(called_by_printf_hook, called_by_printf_hook);

__attribute__((naked))
void
sub_B3E8(void)
{
	asm("b 0xb3e8");
}

void
sub_B3E8_hook(void)
{
	protected = 0;
	sub_B3E8();
}

__attribute__((at(0x1DC70, "", CHIP_VER_BCM4330, FW_VER_5_90_100_41)))
BLPatch(sub_B3E8_hook, sub_B3E8_hook);


void *
dngl_sendpkt_hook(void *sdio, void *p, int chan)
{
	printf("sdio: %08x %08x %08x\n", sdio, p, chan);
	return dngl_sendpkt(sdio, p, chan);
}

__attribute__((at(0x458, "", CHIP_VER_BCM4330, FW_VER_5_90_100_41)))
GenericPatch4(dngl_sendpkt_hook, dngl_sendpkt_hook + 1);

