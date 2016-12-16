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

#ifndef WRAPPER_C
#define WRAPPER_C

#include <firmware_version.h>
#include <structs.h>

#ifndef WRAPPER_H
    // if this file is not included in the wrapper.h file, create dummy functions
    #define VOID_DUMMY { ; }
    #define RETURN_DUMMY { ; return 0; }

    #define AT(CHIPVER, FWVER, ADDR) __attribute__((at(ADDR, "dummy", CHIPVER, FWVER)))
#else
    // if this file is included in the wrapper.h file, create prototypes
    #define VOID_DUMMY ;
    #define RETURN_DUMMY ;
    #define AT(CHIPVER, FWVER, ADDR)
#endif

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0xFE44)
int 
ai_setcoreidx(void *sii, unsigned int coreidx)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x1B8C4)
int 
bus_binddev_rom(void *sdiodev, void *d11dev)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x16284)
int 
dma64_txunframed(void *di, void *data, unsigned int len, char commit)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x15694)
void *
dma_attach(void *osh, char *name, void* sih, unsigned int dmaregstx, unsigned int dmaregsrx, unsigned int ntxd, unsigned int nrxd, unsigned int rxbufsize, int rxextheadroom, unsigned int nrxpost, unsigned int rxoffset, void *msg_level)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8c69c)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x4E44)
void *
dma_rx(void *di)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8c6cc)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x5070)
void *
dma_rxfill(void *di)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8C49C)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x3520)
AT(CHIP_VER_BCM4330, FW_VER_ALL, 0x80B9C8)
void *
dngl_sendpkt(void *sdio, void *p, int chan)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x166B4)
void 
enable_interrupts_and_wait(void)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x16620)
void 
free(void *p)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x136FC)
int 
fw_wf_chspec_ctlchan(unsigned short chanspec) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x2BDBC)
int 
handle_ioctl_cmd(void *wlc, int cmd, void *buf, int len)
RETURN_DUMMY

// should be renamed to dngl_sendup
AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8C51C) 
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x4264)
void *
handle_sdio_xmit_request(void *sdio_hw, void *p) 
RETURN_DUMMY

// should be renamed to dngl_sendup_ram
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x183798) 
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x183798)
void *
handle_sdio_xmit_request_ram(void *sdio_hw, void *p) 
RETURN_DUMMY

AT(CHIP_VER_BCM4330, FW_VER_ALL, 0x80BA48)
void *
dngl_sendup(void *sdio_hw, void *p)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x1654C)
AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x807B24)
AT(CHIP_VER_BCM4330, FW_VER_5_90_100_41, 0x80D800)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x8290)
void *
hndrte_add_timer(void *t, int ms, int periodic)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x16604)
int
hndrte_del_timer(void *t)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x16690)
void
hndrte_free_timer(void *t)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x166cc)
AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x807CB4)
AT(CHIP_VER_BCM4330, FW_VER_5_90_100_41, 0x80D9A0)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x84A4)
void *
hndrte_init_timer(void *context, void *data, void *mainfn, void *auxfn)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x168AC)
int
hndrte_schedule_work(void *context, void *data, void *taskfn, int delay)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x1814f4)
void *
malloc(unsigned int size, char alignment)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8C3DC)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x35F8)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x2360)
AT(CHIP_VER_BCM4330, FW_VER_ALL, 0x809344)
int 
memcpy(void *dst, void *src, int len) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x1269C)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x3700)
AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x803B14)
AT(CHIP_VER_BCM4330, FW_VER_ALL, 0x80940C)
void *
memset(void *dst, int value, int len) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x1673C) 
void *
osl_malloc(void *osh, unsigned int size) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C48DE)
void
phy_reg_mod(void *pi, unsigned short addr, unsigned short mask, unsigned short val)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1C3D32) 
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C3F7E) 
int 
phy_reg_read(void *pi, int add)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1C3D48)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C3F94)
void *
phy_reg_write(void *pi, int addr, int val)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8C70C) 
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x8FD2C)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x625C)
AT(CHIP_VER_BCM4330, FW_VER_ALL, 0x80E358)
void *
pkt_buf_get_skb(void *osh, unsigned int len) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8C71C)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x8FD1C)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x62A0)
AT(CHIP_VER_BCM4330, FW_VER_ALL, 0x80E300)
void *
pkt_buf_free_skb(void *osh, void *p, int send) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x126f0)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x374C)
AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x803B60)
AT(CHIP_VER_BCM4330, FW_VER_ALL, 0x809454)
int 
printf(const char *format, ...) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x1d474) 
unsigned int 
si_getcuridx(void *sii) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x1DCBC) 
void 
si_setcore(void *sii, int coreid, int coreunit) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x12794) 
int 
sprintf(char *buf, const char *format, ...) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x1283C) 
int 
strncmp(char *str1, char *str2, unsigned int num) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x166b4) 
void 
sub_166b4(void) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x16D8C) 
void 
sub_16D8C(int a1, int a2, void *a3) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x1360C) 
int 
sum_buff_lengths(void *osh, void *p) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x39DCC) 
void *
sub_39DCC(void *wlc, int chanspec) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x4F080)
AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x8457F4)
void 
wlc_bmac_mctrl(void *wlc_hw, int mask, int val) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x4DB68) 
short 
wlc_bmac_read_objmem(void *wlc_hw, unsigned int offset, int sel) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x4F79C) 
int 
wlc_bmac_read_shm(void *wlc_hw, unsigned int offset) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x3B0BC)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x3085C)
void 
wlc_suspend_mac_and_wait(void *wlc) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x4FC88)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x449FC)
void 
wlc_bmac_suspend_mac_and_wait(void *wlc_hw) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8CB9C)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x1BEAC)
void 
wlc_bmac_read_tsf(void *wlc_hw, unsigned int *tsf_l_ptr, unsigned int *tsf_h_ptr) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x4DCC8) 
void *
wlc_bmac_write_objmem(void *wlc_hw, unsigned int offset, short v, int sel) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x504B0) 
void 
wlc_bmac_write_template_ram(void *wlc_hw, int offset, int len, void *buf) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8CBFC)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x1CF3E)
void *
wlc_bsscfg_find_by_wlcif(void *wlc, int wlcif) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x32A90) 
void *
wlc_compute_plcp(void *wlc, unsigned int rspec, short length, short type_subtype_frame_ctl_field, char *plcp) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x32C74) 
void
wlc_custom_scan_complete(void *wlc, int status, void *cfg)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8C97C)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x9F38)
AT(CHIP_VER_BCM4330, FW_VER_ALL, 0x81C51C)
int 
wlc_d11hdrs(void *wlc, void *p, void *scb, int short_preamble, unsigned int frag, unsigned int nfrag, unsigned int queue, int next_frag_len, int key, int rspec_override) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x3352C) 
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x29F50)
void *
wlc_enable_mac(void *wlc) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x4DF60) 
void 
wlc_mctrl_write(void *wlc_hw) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x35aa0) 
int 
wlc_prec_enq_head(void *wlc, void *q, void *pkt, int prec, char head) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8CD4C) 
void *
__wlc_scb_lookup(void *wlc, void *bsscfg, char *ea, int bandunit) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x76900)
AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x86AC74)
void *
wlc_scb_set_bsscfg(void *scb, void *bsscfg) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x38BB0)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0xdc84)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x2E4B0)
AT(CHIP_VER_BCM4330, FW_VER_ALL, 0x832A48)
int 
wlc_sendctl(void *wlc, void *p, void *qi, void *scb, unsigned int fifo, unsigned int rate_override, char enq_only)
RETURN_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x9DEA)
void *
wlc_get_txh_info(void *wlc, void *p, void *txh)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8c79c)
//AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x90AEC)
AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x1997A6)
AT(CHIP_VER_BCM4330, FW_VER_ALL, 0x8182F8)
int
wl_sendup(void *wl, void *wlif, void *p)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x18628)
AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x819510)
AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x270F0)
AT(CHIP_VER_BCM4330, FW_VER_ALL, 0x817ACC)
void
wl_monitor(void *wl, void *sts, void *p)
VOID_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x1210C)
int
wlc_recvdata(void *wlc, void *osh, void *rxh, void *p)
RETURN_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x12A0C)
void *
wlc_recv(void *wlc, void *p)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x399FC) 
void 
wlc_set_chanspec(void *wlc, unsigned short chanspec) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x4E0C8)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x42B94)
void 
wlc_ucode_write(void *wlc_hw, const int ucode[], const unsigned int nbytes) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x5D3F4) 
int 
wlc_valid_chanspec_db(void *wlc_cm, unsigned short chanspec) 
RETURN_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x84FF24)
int
wlc_valid_chanspec_dualband(void *wlc_cm, unsigned short chanspec)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x26F50) 
void *
wl_add_if(void *wl, int wlcif, int unit, int wds_or_bss) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x271B0) 
void *
wl_alloc_if(void *wl, int iftype, int unit, int wlc_if) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x2716C) 
int 
wl_init(void *wl) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x27138) 
int 
wl_reset(void *wl) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x183886)
void 
before_before_initialize_memory(void)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1837F8) 
int 
bus_binddev(void *sdio_hw, void *sdiodev, void *d11dev) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1844B2) 
void *
dma_txfast(void *di, void *p, int commit) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1844B6) 
void *
dma_txfast_plus_4(void *di, void *p, int commit) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x182750) 
void *
dngl_sendpkt_ram(void *sdio, void *p, int chan) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x181E48) 
void *
dump_stack_print_dbg_stuff_intr_handler(void) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x19B25C) 
int 
function_with_huge_jump_table(void *wlc, int a2, int cmd, int a4, int a5, unsigned int a6, int a7, int a8, int a9, int a10) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x08C3CC)
int
memcmp(void *s1, void *s2, int n)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x181418) 
int 
memcpy_ram(void *dst, void *src, int len) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1FD78C) 
int 
path_to_load_ucode(int devid, void *osh, void *regs, int bustype, void *sdh) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x184F14) 
void *
pkt_buf_get_skb_ram(void *osh, unsigned int len) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x184F64) 
void *
pkt_buf_free_skb_ram(void *osh, void *p, int send) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1C3CE8) 
int 
read_radio_reg(void *pi, short addr) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x182A64) 
void *
sdio_header_parsing_from_sk_buff(void *sdio, void *p) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x181128) 
void 
setup(void) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1F319C) 
void *
setup_some_stuff(void *wl, int vendor, int a3, int a4, char a5, void *osh, int a7, int a8, int a9, int a10) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x184878) 
void 
si_update_chipcontrol_shm(void *sii, int addr, int mask, int data) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1831A0) 
void *
sub_1831A0(void *osh, void *a2, int a3, void *sdiodev) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1ECAB0) 
void 
sub_1ECAB0(int a1) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1ed41c) 
void 
sub_1ed41c(void) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1810a8) 
void 
sub_1810a8(void) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1ec7c8) 
void 
sub_1ec7c8(void) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1ed584) 
void 
sub_1ed584(void) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1ecab0) 
void 
sub_1ecab0(void) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1ec6fc) 
void 
sub_1ec6fc(void) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1816e4) 
void 
sub_1816e4(void) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1CECBC) 
void *
sub_1CECBC(void *a, void *bsscfg, void *p, void *data) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x18256C) 
int 
towards_dma_txfast(void *sdio, void *p, int chan) 
RETURN_DUMMY

//AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x80506C)
//int
//wf_chspec_malformed(unsigned short chanpsec)
//RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1AB840)
void 
wlc_bmac_init(void *wlc_hw, unsigned int chanspec, unsigned int mute) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1AAD84) 
void 
wlc_bmac_read_tsf_ram(void *wlc_hw, unsigned int *tsf_l_ptr, unsigned int *tsf_h_ptr) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1AAD98) 
int 
wlc_bmac_recv(void *wlc_hw, unsigned int fifo, int bound, int *processed_frame_cnt) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1AAD9C) 
int 
wlc_bmac_recv_plus4(void *wlc_hw, unsigned int fifo, int bound, int *processed_frame_cnt) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1AC166) 
void *
wlc_bsscfg_find_by_wlcif_ram(void *wlc, int wlcif) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1AE2BC) 
void *
wlc_channel_set_chanspec(void *wlc_cm, unsigned short chanspec, int local_constraint_qdbm) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1AE2C0) 
void *
wlc_channel_set_chanspec_plus4(void *wlc_cm, unsigned short chanspec, int local_constraint_qdbm) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1AB66C) 
void 
wlc_coreinit(void *wlc_hw) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x191438)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x191528)
int
wlc_custom_scan(void *wlc, void *arg, int arg_len, void *chanspec_start, int macreq, void *bsscfg)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x18C4C8) 
int 
wlc_d11hdrs_RAM(void *wlc, void *p, void *scb, int short_preamble, unsigned int frag, unsigned int nfrag, unsigned int queue, int next_frag_len, int key, int rspec_override) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x199874) 
int 
wlc_init(void *wlc) 
RETURN_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x8081F0)
unsigned int
udelay(int a1)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x19551C) 
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x19560C)
AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x81A2D4)
AT(CHIP_VER_BCM4330, FW_VER_5_90_100_41, 0x68DC)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x19734)
int 
wlc_ioctl(void *wlc, int cmd, void *arg, int len, void *wlc_if) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x19B25C) 
int 
wlc_iovar_change_handler(void *wlc, int a2, int cmd, char *a4, unsigned int a6, int a7, int a8, int a9, int wlcif) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x19B260) 
int 
wlc_iovar_change_handler_plus4(void *wlc, int a2, int cmd, char *a4, unsigned int a6, int a7, int a8, int a9, int wlcif) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x18BB6C)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x18BC5C)
AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x82ABEC)
int 
wlc_iovar_op(void *wlc, char *varname, void *params, int p_len, void *arg, int len, char set, void *wlcif) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x18D648)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x18D738)
AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x1ECC4)
void *
wlc_monitor(void *wlc, void * wrxh, void *p, int wlc_if) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1C456E)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C47BA)
AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x1D15B2)
void 
wlc_phyreg_enter(void *pi) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1C4588)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C47D4)
AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x1D15CC)
void 
wlc_phyreg_exit(void *pi) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1C4B40)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C4D8C)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x23278)
AT(CHIP_VER_BCM4330, FW_VER_ALL, 0x826F3C)
int 
wlc_phy_channel2freq(unsigned int channel) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1C4B36) 
int 
wlc_phy_chanspec_get(void *ppi) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1B39F8)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1b3c44)
AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x1BB96A)
int 
wlc_phy_chan2freq_acphy(void *pi, int chanspec, int *freq, void **chan_info_ptr) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1B39FE) 
int 
wlc_phy_chan2freq_nphy_plus6(void *pi, int a2, int a3, int a4) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1C553C) 
void 
wlc_phy_rssi_compute(void *pih, void *ctx) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1BE636) 
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1BE882)
AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x1C68D8)
void 
wlc_phy_stay_in_carriersearch_phy(void *pi, int enable) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1BAF88)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1BB1D4)
AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x1C2EC8)
void 
wlc_phy_table_read_phy(void *pi, unsigned int id, unsigned int len, unsigned int offset, unsigned int width, void *data) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1B8CD2)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1B8F1E)
void 
wlc_phy_table_write_phy(void *pi, unsigned int id, unsigned int len, unsigned int offset, unsigned int width, const void *data) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1BE876) 
int 
wlc_phy_tx_tone_phy(void *pi, int f_kHz, int max_val, char iqmode, char dac_test_mode, char modify_bbmult) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x191654) 
int 
wlc_prep_pdu(void *wlc, void *p, int *fifo) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x191B00) 
void 
wlc_prep_sdu(void *wlc, void *p, int *counter, int *fifo) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x19123C) 
void 
wlc_radio_upd(void *wlc) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1C9DBE) 
void *
wlc_scbfindband(void *wlc, void *bsscfg, char *ea, int bandunit) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1CA4CE) 
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x301F8)
void *
wlc_scb_lookup(void *wlc, void *bsscfg, char *ea) 
RETURN_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x81F80C)
int
wlc_pdu_txhdr(void *wlc, void *p, void *scb)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1CA496) 
void *
__wlc_scb_lookup_ram(void *wlc, void *bsscfg, char *ea, int bandunit) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x197A18) 
int 
wlc_sendpkt(void *wlc, void *p, int wlcif) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1926B8) 
void 
wlc_send_q(void *wlc, void *qi) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x193348) 
void *
wlc_sendnulldata(void *wlc, void *bsscfg, unsigned int *ea, int datarate_maybe, int p_field_26, int prio) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x193744) 
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x193834) 
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0xF4A8)
void 
wlc_txfifo(void *wlc, int fifo, void *p, void *txh, unsigned char commit, char txpktpend) 
VOID_DUMMY

AT(CHIP_VER_BCM4330, FW_VER_ALL, 0x8353B8)
void
wlc_txfifo_wotxh(void *wlc, int fifo, void *p, unsigned char commit, char txpktpend)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x18BB56) 
void *
wlc_txc_upd(void *wlc) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1F4EF8)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x44EC0)
void 
wlc_ucode_download(void *wlc_hw) 
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1ADA64) 
int 
wlc_valid_chanspec_ext(void *wlc_cm, unsigned short chanspec, int dualband) 
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1ADA68) 
int 
wlc_valid_chanspec_ext_plus4(void *wlc_cm, unsigned short chanspec, int dualband) 
RETURN_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x84EEA0)
int
wlc_valid_chanspec(void *wlc_cm, unsigned short chanspec, int dualband)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1995C8) 
void *
wlc_wlc_txq_enq(void *wlc, void *scb, void *p, int prec) 
RETURN_DUMMY

#undef VOID_DUMMY
#undef RETURN_DUMMY
#undef AT

#endif /*WRAPPER_C*/
