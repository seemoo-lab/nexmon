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

#ifndef LOCAL_WRAPPER_C
#define LOCAL_WRAPPER_C

#include <firmware_version.h>
#include <structs.h>
#include <stdarg.h>

#ifndef LOCAL_WRAPPER_H
    // if this file is not included in the local_wrapper.h file, create dummy functions
    #define VOID_DUMMY { ; }
    #define RETURN_DUMMY { ; return 0; }

    #define AT(CHIPVER, FWVER, ADDR) __attribute__((weak, at(ADDR, "dummy", CHIPVER, FWVER)))
#else
    // if this file is included in the wrapper.h file, create prototypes
    #define VOID_DUMMY ;
    #define RETURN_DUMMY ;
    #define AT(CHIPVER, FWVER, ADDR)
#endif

AT(CHIP_VER_BCM4375b1, FW_VER_18_41_8_9_sta, 0x1FD9BC)
int
called_by_wlc_ioctl(struct wlc_info *wlc, int cmd, char *arg, int len)
RETURN_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_18_38_18_sta, 0x21803E)
AT(CHIP_VER_BCM4375b1, FW_VER_18_41_8_9_sta, 0x232E0E)
void *
path_to_path_to_indirect_call_of_phy_ac_rssi_compute(void *rssi_related, void *wrxh, void *a3)
RETURN_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_ALL, 0xC4160)
int
wlc_ioctl(void *wlc, int cmd, void *arg, int len, void *wlc_if)
RETURN_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_ALL, 0xC4168)
int
wlc_ioctl_plus8(void *wlc, int cmd, void *arg, int len, void *wlc_if)
RETURN_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_18_38_18_sta, 0x217010)
AT(CHIP_VER_BCM4375b1, FW_VER_18_41_8_9_sta, 0x231DE0)
void *
wlc_recv(void *wlc, void *p)
RETURN_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_18_38_18_sta, 0x183834)
AT(CHIP_VER_BCM4375b1, FW_VER_18_41_8_9_sta, 0x1845A0)
int
memcpy(void *dst, void *src, int len)
RETURN_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_ALL, 0xC48F0)
int
wlc_iovar_op(void *wlc, char *varname, void *params, int p_len, void *arg, int len, char set, void *wlcif)
RETURN_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_18_38_18_sta, 0x1863A0)
AT(CHIP_VER_BCM4375b1, FW_VER_18_41_8_9_sta, 0x1876A8)
void *
pkt_buf_get_skb(void *osh, unsigned int len)
RETURN_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_18_38_18_sta, 0x1863F0)
AT(CHIP_VER_BCM4375b1, FW_VER_18_41_8_9_sta, 0x187718)
void *
pkt_buf_free_skb(void *osh, void *p, int send)
RETURN_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_18_38_18_sta, 0x1C7DF0)
AT(CHIP_VER_BCM4375b1, FW_VER_18_41_8_9_sta, 0x1D5CBC)
void
wl_set_copycount_bytes(void *wl, int copycount, int d11rxoffset)
VOID_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_ALL, 0x8AAC)
int
snprintf(char *buf, unsigned int n, const char *format, ...)
RETURN_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_ALL, 0x8C08)
int
vsnprintf(char *buf, unsigned int n, const char *format, va_list ap)
RETURN_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_ALL, 0x8988)
void *
memset(void *dst, int value, int len)
RETURN_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_18_38_18_sta, 0x255A80)
AT(CHIP_VER_BCM4375b1, FW_VER_18_41_8_9_sta, 0x271C00)
void *
wlc_okc_attach(void *wlc)
RETURN_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_ALL, 0x61304)
void *
osl_mallocz(int size)
RETURN_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_18_38_18_sta, 0x24B894)
AT(CHIP_VER_BCM4375b1, FW_VER_18_41_8_9_sta, 0x267594)
void
wlc_tunables_init(void *tunables, void *pub_cmn, int a3, int a4)
VOID_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_ALL, 0x142D10)
unsigned int
hndrte_time_ms()
RETURN_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_ALL, 0x143AC0)
void
pktfrag_trim_tailbytes(void * osh, void* p, unsigned short len, unsigned char type)
VOID_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_ALL, 0x1428C0)
void
wl_sendup_4param(void *wl, void *wlif, void *p, int numpkt)
VOID_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_ALL, 0x7861C)
void
wlc_recover_tsf64(void *wlc, void *wrxh, unsigned int *tsf_h, unsigned int *tsf_l)
VOID_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_ALL, 0x5B928)
char
phy_noise_avg(void *pih)
RETURN_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_ALL, 0xA068)
unsigned char
wf_chspec_ctlchan(unsigned short chspec)
RETURN_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_ALL, 0x9DA4)
int
wf_channel2mhz(unsigned int ch, unsigned int start_factor)
RETURN_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_ALL, 0x8B6C)
int
strncmp(char *str1, char *str2, unsigned int num)
RETURN_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_ALL, 0xFFBA4)
void *
get_other_wlc(void *wlc)
RETURN_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_ALL, 0x144A60)
void *
find_wlc_for_chanspec(void *wlc, void *bi, unsigned short chanspec, void *skip_wlc, unsigned int match_mask)
RETURN_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_ALL, 0x79EB0)
int
path_to_wl_set_monitor(void *wlc, int value)
RETURN_DUMMY

AT(CHIP_VER_BCM4375b1, FW_VER_18_38_18_sta, 0x1C9B34)
AT(CHIP_VER_BCM4375b1, FW_VER_18_41_8_9_sta, 0x1D7CF8)
int
wlc_sendctl(void *wlc, void *p, void *qi, void *scb, unsigned int fifo, unsigned int rate_override, char enq_only)
RETURN_DUMMY

#undef VOID_DUMMY
#undef RETURN_DUMMY
#undef AT

#endif /*LOCAL_WRAPPER_C*/
