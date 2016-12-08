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

#pragma once

struct d11rxhdr {
	unsigned short RxFrameSize;			/* Actual byte length of the frame data received */
	unsigned short PAD;
	unsigned short PhyRxStatus_0;		/* PhyRxStatus 15:0 */
	unsigned short PhyRxStatus_1;		/* PhyRxStatus 31:16 */
	unsigned short PhyRxStatus_2;		/* PhyRxStatus 47:32 */
	unsigned short PhyRxStatus_3;		/* PhyRxStatus 63:48 */
	unsigned short PhyRxStatus_4;		/* PhyRxStatus 79:64 */
	unsigned short PhyRxStatus_5;		/* PhyRxStatus 95:80 */
	unsigned short RxStatus1;			/* MAC Rx status */
	unsigned short RxStatus2;			/* extended MAC Rx status */
	unsigned short RxTSFTime;			/* RxTSFTime time of first MAC symbol + M_PHY_PLCPRX_DLY */
	unsigned short RxChan;				/* gain code, channel radio code, and phy type -> looks like chanspec */
} __attribute__((packed));

 /* ucode RxStatus1: */
#define RXS_BCNSENT             0x8000
#define RXS_SECKINDX_MASK       0x07e0
#define RXS_SECKINDX_SHIFT      5
#define RXS_DECERR              (1 << 4)
#define RXS_DECATMPT            (1 << 3)
/* PAD bytes to make IP data 4 bytes aligned */
#define RXS_PBPRES              (1 << 2)
#define RXS_RESPFRAMETX         (1 << 1)
#define RXS_FCSERR              (1 << 0)

/* ucode RxStatus2: */
#define RXS_AMSDU_MASK          1
#define RXS_AGGTYPE_MASK        0x6
#define RXS_AGGTYPE_SHIFT       1
#define RXS_PHYRXST_VALID       (1 << 8)
#define RXS_RXANT_MASK          0x3
#define RXS_RXANT_SHIFT         12

/* RxChan */
#define RXS_CHAN_40             0x1000
#define RXS_CHAN_5G             0x0800
#define RXS_CHAN_ID_MASK        0x07f8
#define RXS_CHAN_ID_SHIFT       3
#define RXS_CHAN_PHYTYPE_MASK   0x0007
#define RXS_CHAN_PHYTYPE_SHIFT  0

struct wlc_d11rxhdr {
	struct d11rxhdr rxhdr;
	unsigned int tsf_l;
	char rssi;							/* computed instanteneous RSSI in BMAC */
	char rxpwr0;
	char rxpwr1;
	char do_rssi_ma;					/* do per-pkt sampling for per-antenna ma in HIGH */
	char rxpwr[4];						/* rssi for supported antennas */
} __attribute__((packed));

