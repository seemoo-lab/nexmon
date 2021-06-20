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
#include <stdarg.h>

#ifndef WRAPPER_H
    // if this file is not included in the wrapper.h file, create dummy functions
    #define VOID_DUMMY { ; }
    #define RETURN_DUMMY { ; return 0; }

    #define AT(CHIPVER, FWVER, ADDR) __attribute__((weak, at(ADDR, "dummy", CHIPVER, FWVER)))
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

AT(CHIP_VER_BCM43455c0, FW_VER_ALL, 0x3D20)
void
bcm_binit(void *b, char *buf, uint size)
VOID_DUMMY

AT(CHIP_VER_BCM43455c0, FW_VER_ALL, 0x3D4C)
int
bcm_bprintf(void *b, const char *fmt, ...)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x1B8C4)
int
bus_binddev_rom(void *sdiodev, void *d11dev)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x16284)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x7E00)
int
dma64_txunframed(void *di, void *data, unsigned int len, char commit)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x15694)
void *
dma_attach(void *osh, char *name, void* sih, unsigned int dmaregstx, unsigned int dmaregsrx, unsigned int ntxd, unsigned int nrxd, unsigned int rxbufsize, int rxextheadroom, unsigned int nrxpost, unsigned int rxoffset, void *msg_level)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8c69c)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x4E44)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x4E44)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x4F30)
void *
dma_rx(void *di)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8c6cc)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x5070)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x5070)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x515C)
void *
dma_rxfill(void *di)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8C49C)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x3520)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x3520)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x3550)
AT(CHIP_VER_BCM4330, FW_VER_ALL, 0x80B9C8)
void *
dngl_sendpkt(void *sdio, void *p, int chan)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x166B4)
AT(CHIP_VER_BCM43451b1, FW_VER_ALL, 0x6140)
void
enable_interrupts_and_wait(int a1)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x16620)
AT(CHIP_VER_BCM4335b0, FW_VER_ALL, 0x17F98)
AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x880B90)
AT(CHIP_VER_BCM43430a1, FW_VER_ALL, 0x880B90)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x18234c)
AT(CHIP_VER_BCM43455c0, FW_VER_ALL, 0x9BE4C)
void
free(void *p)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x136FC)
int
fw_wf_chspec_ctlchan(unsigned short chanspec)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x181C4C)
void *
get_printf_config_location(void)
RETURN_DUMMY


AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x2BDBC)
int
handle_ioctl_cmd(void *wlc, int cmd, void *buf, int len)
RETURN_DUMMY

// should be renamed to dngl_sendup
AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8C51C)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x4264)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x4264)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x4294)
void *
handle_sdio_xmit_request(void *sdio_hw, void *p)
RETURN_DUMMY

// should be renamed to dngl_sendup_ram
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x183798)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x183798)
AT(CHIP_VER_BCM4335b0, FW_VER_6_30_171_1_sta, 0x186EE8)
void *
handle_sdio_xmit_request_ram(void *sdio_hw, void *p)
RETURN_DUMMY

AT(CHIP_VER_BCM4330, FW_VER_ALL, 0x80BA48)
void *
dngl_sendup(void *sdio_hw, void *p)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x1654C)
AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x807B24)
AT(CHIP_VER_BCM43430a1, FW_VER_ALL, 0x807B24)
AT(CHIP_VER_BCM4330, FW_VER_5_90_100_41, 0x80D800)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x8290)
AT(CHIP_VER_BCM4356, FW_VER_ALL, 0x80A4)
AT(CHIP_VER_BCM4335b0, FW_VER_ALL, 0x17EA8)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_154, 0x19A4B4)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x19A480)
void *
hndrte_add_timer(void *t, int ms, int periodic)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x16604)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x2754)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x2754)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x2784)
AT(CHIP_VER_BCM4335b0, FW_VER_ALL, 0x17F78)
int
hndrte_del_timer(void *t)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x16690)
AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x807C24)
AT(CHIP_VER_BCM43430a1, FW_VER_ALL, 0x807C24)
AT(CHIP_VER_BCM4335b0, FW_VER_ALL, 0x18010)
AT(CHIP_VER_BCM43455c0, FW_VER_ALL, 0x8170)
void
hndrte_free_timer(void *t)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x166cc)
AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x807CB4)
AT(CHIP_VER_BCM43430a1, FW_VER_ALL, 0x807CB4)
AT(CHIP_VER_BCM4330, FW_VER_5_90_100_41, 0x80D9A0)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x84A4)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x8240)
AT(CHIP_VER_BCM4335b0, FW_VER_ALL, 0x18060)
AT(CHIP_VER_BCM43455c0, FW_VER_ALL, 0x8228)
void *
hndrte_init_timer(void *context, void *data, void *mainfn, void *auxfn)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x168AC)
AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x807E60)
AT(CHIP_VER_BCM43430a1, FW_VER_ALL, 0x807E60)
AT(CHIP_VER_BCM4335b0, FW_VER_ALL, 0x1826C)
AT(CHIP_VER_BCM43455c0, FW_VER_ALL, 0x8524)
int
hndrte_schedule_work(void *context, void *data, void *taskfn, int delay)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x168F4)
AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x807F24)
AT(CHIP_VER_BCM43430a1, FW_VER_ALL, 0x807F24)
AT(CHIP_VER_BCM43455c0, FW_VER_ALL, 0x8448)
unsigned int
hndrte_time_ms()
RETURN_DUMMY

AT(CHIP_VER_BCM43455c0, FW_VER_ALL, 0x9BE5E)
void *
lb_alloc(unsigned int size, unsigned int lbuf_type)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x1814f4)
AT(CHIP_VER_BCM4335b0, FW_VER_ALL, 0x180E0)
AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x880B80)
AT(CHIP_VER_BCM43430a1, FW_VER_ALL, 0x880B80)
AT(CHIP_VER_BCM43451b1, FW_VER_ALL, 0x1be7fe)
AT(CHIP_VER_BCM43455, FW_VER_7_45_77_0, 0x19ED78)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x182238)
AT(CHIP_VER_BCM43455, FW_VER_7_45_77_0_23_8_2017, 0x19ED88)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_154, 0x19A17C)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x19A0FC)
AT(CHIP_VER_BCM43455, FW_VER_7_46_77_11, 0x19F660)
AT(CHIP_VER_BCM43455, FW_VER_7_45_59_16, 0x19F3B0)
void *
malloc(unsigned int size, char alignment)
RETURN_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x880E20)
void *
calls_malloc_2nd_arg_zero(unsigned int len)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8C3DC)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x35F8)
AT(CHIP_VER_BCM4356, FW_VER_ALL, 0x2E5C)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x2360)
AT(CHIP_VER_BCM43430a1, FW_VER_ALL, 0x880B80)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x2390)
AT(CHIP_VER_BCM4330, FW_VER_ALL, 0x809344)
AT(CHIP_VER_BCM4335b0, FW_VER_ALL, 0x12D20)
AT(CHIP_VER_BCM43596a0, FW_VER_ALL, 0x1d2c)
AT(CHIP_VER_BCM43451b1, FW_VER_ALL, 0x64c38)
AT(CHIP_VER_BCM43455, FW_VER_7_45_77_0, 0x19A0E8)
AT(CHIP_VER_BCM43455, FW_VER_7_120_5_1_sta_C0, 0x19A0F8)
AT(CHIP_VER_BCM43455, FW_VER_7_45_77_0_23_8_2017, 0x19A0F8)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_154, 0x19a098)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x19A018)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_206, 0x19A0F8)
AT(CHIP_VER_BCM43909b0, FW_VER_ALL, 0x64588)
AT(CHIP_VER_BCM4361b0, FW_VER_ALL, 0x116D6C)
AT(CHIP_VER_BCM4366c0, FW_VER_10_10_122_20, 0x45D8)
AT(CHIP_VER_BCM43455, FW_VER_7_46_77_11, 0x19A538)
AT(CHIP_VER_BCM43455, FW_VER_7_45_59_16, 0x19A0C8)
int
memcpy(void *dst, void *src, int len)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x1269C)
AT(CHIP_VER_BCM4356, FW_VER_ALL, 0x2F64)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x3700)
AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x803B14)
AT(CHIP_VER_BCM43430a1, FW_VER_ALL, 0x803B14)
AT(CHIP_VER_BCM4330, FW_VER_ALL, 0x80940C)
AT(CHIP_VER_BCM4335b0, FW_VER_ALL, 0x12E38)
AT(CHIP_VER_BCM43596a0, FW_VER_ALL, 0x1e34)
AT(CHIP_VER_BCM43451b1, FW_VER_ALL, 0x24b8)
AT(CHIP_VER_BCM43455, FW_VER_ALL, 0x37E8)
AT(CHIP_VER_BCM43455c0, FW_VER_ALL, 0x37e8)
void *
memset(void *dst, int value, int len)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x1673C)
void *
osl_malloc(void *osh, unsigned int size)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C48A8)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x1d2912)
void
phy_reg_and(void *pi, uint16 addr, uint16 val)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C48C2)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x1d292c)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x22fda)
void
phy_reg_or(void *pi, uint16 addr, uint16 val)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C48DE)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x22BCA)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x22BCA)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x22ff6)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x1d2948)
void
phy_reg_mod(void *pi, unsigned short addr, unsigned short mask, unsigned short val)
VOID_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x225AE)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x225AE)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x229da)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1C3D32)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C3F7E)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x1d184a)
int
phy_reg_read(void *pi, int add)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1C3D48)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C3F94)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x1d1860)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_154, 0x1D9C62)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x1DF5BE)
void *
phy_reg_write(void *pi, int addr, int val)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8C70C)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x8FD2C)
AT(CHIP_VER_BCM4356, FW_VER_ALL, 0x89A3C)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x625C)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x625C)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x6348)
AT(CHIP_VER_BCM4330, FW_VER_ALL, 0x80E358)
AT(CHIP_VER_BCM4335b0, FW_VER_ALL, 0x18B04)
AT(CHIP_VER_BCM43596a0, FW_VER_ALL, 0x6054)
AT(CHIP_VER_BCM43451b1, FW_VER_ALL, 0x64d88)
AT(CHIP_VER_BCM43455, FW_VER_ALL, 0x9C04C)
AT(CHIP_VER_BCM43455c0, FW_VER_ALL, 0x9C04C)
AT(CHIP_VER_BCM43909b0, FW_VER_ALL, 0x646B8)
AT(CHIP_VER_BCM4361b0, FW_VER_13_38_55_1_sta, 0x177954)
AT(CHIP_VER_BCM4366c0, FW_VER_10_10_122_20, 0x205B50)
void *
pkt_buf_get_skb(void *osh, unsigned int len)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8C71C)
AT(CHIP_VER_BCM4356, FW_VER_ALL, 0x89A1C)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x8FD1C)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x62A0)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x62A0)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x638C)
AT(CHIP_VER_BCM4330, FW_VER_ALL, 0x80E300)
AT(CHIP_VER_BCM4335b0, FW_VER_ALL, 0x18A98)
AT(CHIP_VER_BCM43596a0, FW_VER_ALL, 0x5FE8)
AT(CHIP_VER_BCM43455, FW_VER_ALL, 0x9C05C)
AT(CHIP_VER_BCM43455c0, FW_VER_ALL, 0x9C05C)
AT(CHIP_VER_BCM4361b0, FW_VER_13_38_55_1_sta, 0x177994)
AT(CHIP_VER_BCM4366c0, FW_VER_10_10_122_20, 0x7930)
void *
pkt_buf_free_skb(void *osh, void *p, int send)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8C72C)
void *
pkt_buf_dup_skb(void *osh, void *p)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x126f0)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x374C)
AT(CHIP_VER_BCM4356, FW_VER_ALL, 0x2FB0)
AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x803B60)
AT(CHIP_VER_BCM43430a1, FW_VER_ALL, 0x803B60)
AT(CHIP_VER_BCM4330, FW_VER_ALL, 0x809454)
AT(CHIP_VER_BCM4335b0, FW_VER_ALL, 0x12E94)
// we use RAM locations as this function is overwritten by flashpatches
AT(CHIP_VER_BCM43596a0, FW_VER_9_75_155_45_sta_c0, 0x162858)
AT(CHIP_VER_BCM43596a0, FW_VER_9_96_4_sta_c0, 0x162BB8)
AT(CHIP_VER_BCM43451b1, FW_VER_ALL, 0x2504)
AT(CHIP_VER_BCM43455, FW_VER_ALL, 0x3834)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_154, 0x3834)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x3834)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_206, 0x3834)
AT(CHIP_VER_BCM4366c0, FW_VER_10_10_122_20, 0x46C0)
int
printf(const char *format, ...)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x1d474)
unsigned int
si_getcuridx(void *sih)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8BBAC)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x18c4c6)
void
si_pmu_pllcontrol(void *sih, int addr, int mask, int data)
VOID_DUMMY

AT(CHIP_VER_BCM43455c0, FW_VER_ALL, 0x15E18)
unsigned int
si_corereg(void *sih, unsigned int coreidx, unsigned int regoff, unsigned int mask, unsigned int val)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x184AEC)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x18c532)
void
si_pmu_pllupd(void *sih)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x184968)
// AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x1A2738)
unsigned int
si_pmu_chipcontrol(void *sih, unsigned int reg, unsigned int mask, unsigned int val)
RETURN_DUMMY

AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x20B492)
void
si_swdenable(void *sih, unsigned int swdflag)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x1DCBC)
void
si_setcore(void *sih, int coreid, int coreunit)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x12778)
int
snprintf(char *buf, unsigned int n, const char *format, ...)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x12794)
AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x803BF8)
AT(CHIP_VER_BCM43430a1, FW_VER_ALL, 0x803BF8)
AT(CHIP_VER_BCM43596a0, FW_VER_ALL, 0x1ed0)
AT(CHIP_VER_BCM43451b1, FW_VER_ALL, 0x259c)
AT(CHIP_VER_BCM43455c0, FW_VER_ALL, 0x38cc)
int
sprintf(char *buf, const char *format, ...)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x12824)
AT(CHIP_VER_BCM43455c0, FW_VER_ALL, 0x3950)
int
strlen(char *str)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x1283C)
AT(CHIP_VER_BCM43909b0, FW_VER_ALL, 0x279C)
AT(CHIP_VER_BCM43455c0, FW_VER_ALL, 0x3960)
int
strncmp(char *str1, char *str2, unsigned int num)
RETURN_DUMMY

AT(CHIP_VER_BCM43451b1, FW_VER_ALL, 0x265C)
AT(CHIP_VER_BCM43430a1, FW_VER_ALL, 0x803cd4)
char *
strncpy(char *dst, char *src, unsigned int num)
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

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x12978)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x39AC)
AT(CHIP_VER_BCM4356, FW_VER_ALL, 0x3210)
AT(CHIP_VER_BCM43451b1, FW_VER_ALL, 0x2748)
AT(CHIP_VER_BCM43430a1, FW_VER_ALL, 0x803dc0)
AT(CHIP_VER_BCM43455c0, FW_VER_ALL, 0x3A94)
int
vsnprintf(char *buf, unsigned int n, const char *format, va_list ap)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1c6e7e)
void
wlapi_bmac_write_shm(void *physhim, unsigned int offset, unsigned short v)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1c6ea2)
void
wlapi_enable_mac(void *physhim)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1c6e96)
void
wlapi_suspend_mac_and_wait(void *physhim)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1A0E7E)
int
wlc_ampdu_tx_set(void *ampdu_tx, bool on)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x34D68)
AT(CHIP_VER_BCM4335b0, FW_VER_ALL, 0x3F468)
AT(CHIP_VER_BCM43455, FW_VER_ALL, 0x2FC50)
void
wlc_mctrl(void *wlc, uint32 mask, uint32 val)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1c53d6)
bool
wlc_phy_get_rxgainerr_phy(void *pi, int16 *gainerr)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x4fad8)
void
wlc_bmac_set_clk(void *wlc_hw, bool on)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1c6962)
int
wlc_phy_iovar_dispatch(void *pi, uint32 actionid, uint16 type, void *p, uint plen, void *a, int alen, int vsize)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x35094)
int
wlc_module_register(void *pub, const void *iovars, const char *name, void *hdl, void *iovar_fn, void *watchdog_fn, void *up_fn, void *down_fn)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C8620)
bool
wlc_valid_vht_mcs(uint8 mcs, uint8 nss, uint8 bw, uint8 ratemask, bool ldpc, uint8 mcscode)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x3E4F8)
void
wlc_antsel_antcfg_get(void *asi, bool usedef, bool sel, uint8 antselid, uint8 fbantselid, uint8 *antcfg, uint8 *fbantcfg)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x774A4)
void
wlc_scb_ratesel_gettxrate(void *wrsi, void *scb, uint16 *frameid, void *cur_rate, uint16 *flags)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x4F080)
AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x8457F4)
AT(CHIP_VER_BCM43430a1, FW_VER_ALL, 0x8457F4)
void
wlc_bmac_mctrl(void *wlc_hw, int mask, int val)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x4DB68)
short
wlc_bmac_read_objmem(void *wlc_hw, unsigned int offset, int sel)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x4F79C)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x1C1CCC)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x44420)
AT(CHIP_VER_BCM4366c0, FW_VER_10_10_122_20, 0x435E0)
int
wlc_bmac_read_shm(void *wlc_hw, unsigned int offset)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1ABA2C)
void
wlc_bmac_set_chanspec(void *wlc_hw, int chanspec, int mute, void *txpwr)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C6666)
void
wlc_phy_btcx_override_disable(void *pi)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1c1dba)
void
wlc_phy_lpf_hpc_override_acphy(void *pi, bool setup_not_cleanup)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C65AA)
void
wlc_btcx_override_enable(void *pi)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x37070)
unsigned short
wlc_read_shm(void *wlc, unsigned int offset)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x3B0BC)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x3085C)
AT(CHIP_VER_BCM4356, FW_VER_ALL, 0x2C000)
void
wlc_suspend_mac_and_wait(void *wlc)
VOID_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x1BE5E)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x1BE5E)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x1C0AA)
AT(CHIP_VER_BCM4356, FW_VER_ALL, 0x44DAC)
void
wlc_bmac_enable_mac(void *wlc_hw)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x4FC88)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x449FC)
AT(CHIP_VER_BCM4356, FW_VER_ALL, 0x45CA0)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x1C1C8)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x1C1C8)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x1C414)
void
wlc_bmac_suspend_mac_and_wait(void *wlc_hw)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8CB9C)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x1BEAC)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x1BEAC)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x1C0F8)
void
wlc_bmac_read_tsf(void *wlc_hw, unsigned int *tsf_l_ptr, unsigned int *tsf_h_ptr)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x4DCC8)
void *
wlc_bmac_write_objmem(void *wlc_hw, unsigned int offset, short v, int sel)
RETURN_DUMMY

// found by searching for 07 f4 80 37
AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x504B0)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x45200)
AT(CHIP_VER_BCM43455c0, FW_VER_ALL, 0x4A8CC)
void
wlc_bmac_write_template_ram(void *wlc_hw, int offset, int len, void *buf)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8CBFC)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x1CF3E)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x1CF3E)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x1D21A)
void *
wlc_bsscfg_find_by_wlcif(void *wlc, int wlcif)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x046E88)
int
wlc_bsscfg_tdls_init(void *wlc, void *bsscfg_t, uint8 initiator)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1AE014)
void
wlc_channel_reg_limits(void *wlc_cm, int chanspec, void *txpwr)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x5AF20)
void
wlc_channel_srom_limits(void *wlc_cm, int chanspec, void *srommin, void *srommax)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x32A90)
void *
wlc_compute_plcp(void *wlc, unsigned int rspec, short length, short type_subtype_frame_ctl_field, char *plcp)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x32C74)
void
wlc_custom_scan_complete(void *wlc, int status, void *cfg)
VOID_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x8517F4)
void *
wlc_event_alloc(void *eq)
RETURN_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x829D4C)
void
wlc_event_if(void *wlc, void *bsscfg, void *event, void *ether_addr)
VOID_DUMMY

/* supposed to be wlc_d11ac_hdrs */
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x18C5B8)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x18C4C8)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x9F38)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x9F38)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0xA024)
AT(CHIP_VER_BCM4330, FW_VER_ALL, 0x81C51C)
int
wlc_d11hdrs(void *wlc, void *p, void *scb, int short_preamble, unsigned int frag, unsigned int nfrag, unsigned int queue, int next_frag_len, int key, int rspec_override)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8C97C)
AT(CHIP_VER_BCM4335b0, FW_VER_6_30_171_1_sta, 0x191C0C)
int
wlc_d11hdrs_ext(void *wlc, void *p, void *scb, int short_preamble, unsigned int frag, unsigned int nfrag, unsigned int queue, int next_frag_len, int key, int rspec_override, short *txh_off)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x3352C)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x29F50)
AT(CHIP_VER_BCM4356, FW_VER_ALL, 0x2586C)
void *
wlc_enable_mac(void *wlc)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x4DF60)
void
wlc_mctrl_write(void *wlc_hw)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x2C760)
void
wlc_pdu_push_txparams(void *wlc, void *p, unsigned int flags, void *key, unsigned int rate_override, unsigned int fifo)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x35aa0)
int
wlc_prec_enq_head(void *wlc, void *q, void *pkt, int prec, char head)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8C8EC)
AT(CHIP_VER_BCM4335b0, FW_VER_ALL, 0x402E0)
int
wlc_prec_enq(void *wlc, void *q, void *p, int preq)
RETURN_DUMMY

// found after seraching for scan_assoc_time
AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x75790)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x70754)
AT(CHIP_VER_BCM43455c0, FW_VER_ALL, 0x7D46C)
AT(CHIP_VER_BCM4366c0, FW_VER_10_10_122_20, 0x7B0B8)
int
wlc_scan_ioctl(void *wlc_scan_info, int cmd, void *arg, int len, void *wlcif)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8CD4C)
void *
__wlc_scb_lookup(void *wlc, void *bsscfg, char *ea, int bandunit)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x76900)
AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x86AC74)
AT(CHIP_VER_BCM43430a1, FW_VER_ALL, 0x86AC74)
void *
wlc_scb_set_bsscfg(void *scb, void *bsscfg)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x38BB0)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0xdc84)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0xdc84)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0xDDB8)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x2E4B0)
AT(CHIP_VER_BCM4356, FW_VER_7_35_101_5_sta, 0x1A1658)
AT(CHIP_VER_BCM4330, FW_VER_ALL, 0x832A48)
AT(CHIP_VER_BCM4335b0, FW_VER_ALL, 0x38BB0)
AT(CHIP_VER_BCM43596a0, FW_VER_ALL, 0x470cc)
AT(CHIP_VER_BCM43455, FW_VER_ALL, 0x31CE8)
AT(CHIP_VER_BCM43455c0, FW_VER_ALL, 0x31CE8)
int
wlc_sendctl(void *wlc, void *p, void *qi, void *scb, unsigned int fifo, unsigned int rate_override, char enq_only)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x18AEC4)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x18AFB4)
//AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x3A95C)
void
wlc_statsupd(void *wlc)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x4DE60)
char *
wlc_get_macaddr(void *wlc_hw)
RETURN_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x9DEA)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x9DEA)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x9ED6)
void *
wlc_get_txh_info(void *wlc, void *p, void *txh)
RETURN_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x7ef8)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x7ef8)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x7fe4)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x199b64)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_154, 0x1A1D00)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x1A6A84)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_206, 0x1A1EEC)
int
wl_send(void *src, void *dev, void *p)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8c79c)
//AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x90AEC)
AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x1997A6)
AT(CHIP_VER_BCM4358, FW_VER_7_112_201_3, 0x199866)
//AT(CHIP_VER_BCM4356, FW_VER_7_35_101_5_sta, 0x198FCA)
AT(CHIP_VER_BCM4356, FW_VER_ALL, 0x8AABC)
AT(CHIP_VER_BCM4330, FW_VER_ALL, 0x8182F8)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_154, 0x1a2438)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x1A71BC)
AT(CHIP_VER_BCM4361b0, FW_VER_13_38_55_1_sta, 0x177994)
int
wl_sendup(void *wl, void *wlif, void *p)
RETURN_DUMMY

AT(CHIP_VER_BCM43596a0, FW_VER_9_96_4_sta_c0, 0x1624AC)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_154, 0x1a2438)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x1A71BC)
void
wl_sendup_newdrv(void *wl, void *wlif, void *p, int numpkt)
VOID_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x18628)
AT(CHIP_VER_BCM4356, FW_VER_7_35_101_5_sta, 0x199112)
AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x819510)
AT(CHIP_VER_BCM43430a1, FW_VER_ALL, 0x819510)
AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x270F0)
AT(CHIP_VER_BCM4330, FW_VER_ALL, 0x817ACC)
AT(CHIP_VER_BCM4335b0, FW_VER_ALL, 0x29BB8)
AT(CHIP_VER_BCM43596a0, FW_VER_ALL, 0x369B8)
AT(CHIP_VER_BCM43455, FW_VER_ALL, 0x1ED5C)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_154, 0x1A270C)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x1A7490)
void
wl_monitor(void *wl, void *sts, void *p)
VOID_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x1210C)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x1210C)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x12320)
int
wlc_recvdata(void *wlc, void *osh, void *rxh, void *p)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x19B138)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x12A0C)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x12A0C)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x12C20)
AT(CHIP_VER_BCM4361b0, FW_VER_13_38_55_1_sta, 0x1D96B8)
AT(CHIP_VER_BCM43455c0, FW_VER_ALL, 0x9C59C)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x1A6C84)
AT(CHIP_VER_BCM4366c0, FW_VER_10_10_122_20, 0x26CC08)
void *
wlc_recv(void *wlc, void *p)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C8540)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x1E8A76)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x678DC)
unsigned int
wlc_recv_compute_rspec(void *wrxh, void *plcp)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x399FC)
void
wlc_set_chanspec(void *wlc, unsigned short chanspec)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x18E490)
int
wlc_set_ratespec_override(void *wlc, int band_id, unsigned int rspec, bool mcast)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x4E0C8)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x42B94)
AT(CHIP_VER_BCM4356, FW_VER_ALL, 0x43E44)
void
wlc_ucode_write(void *wlc_hw, const int ucode[], const unsigned int nbytes)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x5D3F4)
int
wlc_valid_chanspec_db(void *wlc_cm, unsigned short chanspec)
RETURN_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x84FF24)
AT(CHIP_VER_BCM43430a1, FW_VER_ALL, 0x84FF24)
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

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x185A2C)
int
wl_arp_recv_proc(void *arpi, void *sdu)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x2716C)
int
wl_init(void *wl)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x27138)
int
wl_reset(void *wl)
RETURN_DUMMY

// is the c_main function
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x183886)
void
before_before_initialize_memory(void)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x183886)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x19CA0E)
void
c_main(void)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1837F8)
int
bus_binddev(void *sdio_hw, void *sdiodev, void *d11dev)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8c6bc)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x7770)
int
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
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x181E48)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x19ABA8)
void *
dump_stack_print_dbg_stuff_intr_handler(void)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x19B25C)
int
function_with_huge_jump_table(void *wlc, int a2, int cmd, int a4, int a5, unsigned int a6, int a7, int a8, int a9, int a10)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x08C3CC)
AT(CHIP_VER_BCM4335b0, FW_VER_ALL, 0x12CEC)
AT(CHIP_VER_BCM43455, FW_VER_7_45_77_0, 0x19A0C2)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x35d0)
AT(CHIP_VER_BCM43455, FW_VER_7_45_77_0_23_8_2017, 0x19A0D2)
AT(CHIP_VER_BCM43455, FW_VER_7_46_77_11, 0x19A512)
AT(CHIP_VER_BCM43455, FW_VER_7_45_59_16, 0x19A0A2)
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

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x504A0)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x1C1D60)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x451F8)
AT(CHIP_VER_BCM4366c0, FW_VER_10_10_122_20, 0x44080)
void
wlc_bmac_write_shm(void *wlc_hw, unsigned int offset, unsigned short v)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1AC166)
void *
wlc_bsscfg_find_by_wlcif_ram(void *wlc, int wlcif)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1AE2BC)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1AE40C)
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

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x32ffc)
unsigned int
wlc_down(void *wlc)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x199874)
int
wlc_init(void *wlc)
RETURN_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x8081F0)
AT(CHIP_VER_BCM43430a1, FW_VER_ALL, 0x8081F0)
AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x16A40)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x8b58)
AT(CHIP_VER_BCM43455c0, FW_VER_ALL, 0x891C)
unsigned int
udelay(int a1)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x19551C)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x19560C)
AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x81A2D4)
AT(CHIP_VER_BCM43430a1, FW_VER_ALL, 0x81A2D4)
AT(CHIP_VER_BCM4330, FW_VER_5_90_100_41, 0x68DC)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x19734)
AT(CHIP_VER_BCM4356, FW_VER_ALL, 0x14008)
AT(CHIP_VER_BCM4335b0, FW_VER_ALL, 0x2B2DC)
AT(CHIP_VER_BCM43451b1, FW_VER_ALL, 0xf7cc)
AT(CHIP_VER_BCM43455, FW_VER_ALL, 0x203B8)
AT(CHIP_VER_BCM43596a0, FW_VER_ALL, 0x38E3C)
AT(CHIP_VER_BCM43455c0, FW_VER_ALL, 0x203B8)
AT(CHIP_VER_BCM43909b0, FW_VER_ALL, 0xe4c4)
AT(CHIP_VER_BCM4361b0, FW_VER_ALL, 0x4AE30)
AT(CHIP_VER_BCM4366c0, FW_VER_10_10_122_20, 0x1E4D8)
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
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x9264)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x9264)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x9350)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x2b428)
AT(CHIP_VER_BCM43455c0, FW_VER_ALL, 0x9C23C)
AT(CHIP_VER_BCM43909b0, FW_VER_ALL, 0x648A8)
AT(CHIP_VER_BCM4366c0, FW_VER_10_10_122_20, 0x295C4)
int
wlc_iovar_op(void *wlc, char *varname, void *params, int p_len, void *arg, int len, char set, void *wlcif)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x058B44)
int
wlc_l2_filter_tdls(void *wlc, void *sdu)
RETURN_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x2A0A2)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x2A0A2)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x2A4DA)
void
wlc_lcn40phy_deaf_mode(void *pi, unsigned char mode)
VOID_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x2561E)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x2561E)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x25a4a)
void
wlc_lcn40phy_force_pwr_index(void *pi, int indx)
VOID_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x24818)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x24818)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x24c44)
unsigned short
wlc_lcn40phy_num_samples(void *pi, int f_Hz, unsigned int phy_bw)
RETURN_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x26384)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x26384)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x267b0)
void
wlc_lcn40phy_run_samples(void *pi, unsigned short num_samps, unsigned short num_loops, unsigned short wait, unsigned char iqcalmode, unsigned char tx_pu_param)
VOID_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x254D8)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x254D8)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x25904)
void
wlc_lcn40phy_set_bbmult(void *pi, uint8 m0)
VOID_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x24A08)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x24A08)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x24e34)
void
wlc_lcn40phy_set_pa_gain(void *pi, uint16 gain)
VOID_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x24F22)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x24F22)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x2534e)
void
wlc_lcn40phy_set_tx_gain(void *pi,  void *target_gains)
VOID_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x29442)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x29442)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x2987a)
void
wlc_lcn40phy_set_tx_pwr_by_index(void *pi, int indx)
VOID_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x25A82)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x25A82)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x25eae)
void
wlc_lcn40phy_set_tx_pwr_ctrl(void *pi, uint16 mode)
VOID_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x2A174)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x2A174)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x2a5ac)
void
wlc_lcn40phy_start_tx_tone(void *pi, signed int f_Hz, int max_val, int iqcalmode, int deaf_set_to_1, int tx_pu_param)
VOID_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x2A0EA)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x2A0EA)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x2a522)
void
wlc_lcn40phy_stop_tx_tone(void *pi, int unused0, int unused1, int  phy_tx_tone_freq_set_to_0)
VOID_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x261AC)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x261AC)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x265d8)
void
wlc_lcn40phy_tx_tone_samples(void *pi, signed int f_Hz, int max_val, int *data_buf, int phy_bw, unsigned short num_samps)
VOID_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x253B0)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x253B0)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x257dc)
void
wlc_lcn40phy_write_table(void *pi, const void *pti)
VOID_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x82C684)
void
wlc_process_event(void *wlc, void *e)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x191388)
AT(CHIP_VER_BCM43451b1, FW_VER_ALL, 0x1de6c)
void
wlc_radio_mpc_upd(void *wlc)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x18D648)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x18D738)
AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x1ECC4)
AT(CHIP_VER_BCM4356, FW_VER_ALL, 0x19D80)
void *
wlc_monitor(void *wlc, void * wrxh, void *p, int wlc_if)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1C456E)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C47BA)
AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x1D15B2)
AT(CHIP_VER_BCM4358, FW_VER_7_112_201_3, 0x1D169E)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x1D2742)
AT(CHIP_VER_BCM4356, FW_VER_7_35_101_5_sta, 0x1D9D96)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_154, 0x1DBDD6)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x1E1AC6)
void
wlc_phyreg_enter(void *pi)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1C4588)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C47D4)
AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x1D15CC)
AT(CHIP_VER_BCM4358, FW_VER_7_112_201_3, 0x1D16B8)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x1D275C)
AT(CHIP_VER_BCM4356, FW_VER_7_35_101_5_sta, 0x1D9DB0)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_154, 0x1DBE06)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x1E1AF6)
void
wlc_phyreg_exit(void *pi)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1C4B40)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C4D8C)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x23278)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x23278)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x236a4)
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
AT(CHIP_VER_BCM4358, FW_VER_7_112_201_3, 0x1BBA56)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x1bcafa)
AT(CHIP_VER_BCM4356, FW_VER_7_35_101_5_sta, 0x1C469A)
AT(CHIP_VER_BCM43455, FW_VER_7_45_77_0, 0x1D0DFC)
AT(CHIP_VER_BCM43455, FW_VER_7_120_5_1_sta_C0, 0x1CF9A0)
AT(CHIP_VER_BCM43455, FW_VER_7_45_77_0_23_8_2017, 0x1D13C8)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_154, 0x1C9FA4)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x1CF494)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_206, 0x1CCF20)
AT(CHIP_VER_BCM43455, FW_VER_7_46_77_11, 0x1D2E70)
AT(CHIP_VER_BCM43455, FW_VER_7_45_59_16, 0x1D0628)
int
wlc_phy_chan2freq_acphy(void *pi, int chanspec, int *freq, void **chan_info_ptr)
RETURN_DUMMY

AT(CHIP_VER_BCM43596a0, FW_VER_9_75_155_45_sta_c0, 0x1727b4)
AT(CHIP_VER_BCM43596a0, FW_VER_9_96_4_sta_c0, 0x172EAC)
int
wlc_phy_chan2freq_acphy_newdvr(void *pi, int chanspec, void **chan_info_ptr, int *xxx)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1B39FE)
int
wlc_phy_chan2freq_nphy_plus6(void *pi, int a2, int a3, int a4)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1BE12C)
uint16
wlc_phy_classifier_acphy(void *pi, uint16 mask, uint16 val)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1B89DE)
void
wlc_phy_clip_det_acphy(void *pi, uint8 enable)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1C5744)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C5990)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0x23F3C)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x23F3C)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x24368)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x1d3f54)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_154, 0x1DE6B8)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x1E44DC)
void
wlc_phy_cordic(int theta, void *val)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1B9B38)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1B9D84)
unsigned short
wlc_phy_gen_load_samples_acphy(void *pi, int f_kHz, unsigned short max_val)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1B9A3A)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1B9C86)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x1c26d2)
void
wlc_phy_loadsampletable_acphy(void *pi, void *tone_buf, unsigned short num_samps)
VOID_DUMMY

AT(CHIP_VER_BCM43455c0, FW_VER_7_45_154, 0x1CD7F4)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x1D2D2C)
void
wlc_phy_loadsampletable_acphy_new(void *pi, void *tone_buf, uint16 num_samps, uint8 alloc, uint8 conj)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1BE80C)
void
wlc_phy_ofdm_crs_acphy(void *pi, uint8 enable)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1BCA64)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x1c5a54)
void
wlc_phy_resetcca_acphy(void *pi)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C5788)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1C553C)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x1E41E6)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x1D3CB4)
AT(CHIP_VER_BCM4366c0, FW_VER_10_10_122_20, 0x22CF44)
void
wlc_phy_rssi_compute(void *pih, void *ctx)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1BE6BC)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1BE908)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x1c7e68)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_154, 0x1D2C8C)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x1D81C0)
void
wlc_phy_runsamples_acphy(void *pi, unsigned short num_samps, unsigned short loops, unsigned short wait, unsigned char iqmode, unsigned char mac_based)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1BB68C)
void
wlc_phy_get_tx_bbmult_acphy(void *pi, unsigned short *bb_mult, unsigned short core)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1B9904)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_154, 0x1CDEA0)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x1D33D8)
void
wlc_phy_set_tx_bbmult_acphy(void *pi, unsigned short *bb_mult, unsigned short core)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1BB502)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x1c472c)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_154, 0x1CF462)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x1D499A)
void
wlc_phy_get_txgain_settings_by_index_acphy(void *pi, void *txgain_settings, int8 txpwrindex)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1BE636)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1BE882)
AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x1C68D8)
AT(CHIP_VER_BCM4358, FW_VER_7_112_201_3, 0x1C69C4)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x1c7a68)
AT(CHIP_VER_BCM4356, FW_VER_7_35_101_5_sta, 0x1CF5F4)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_154, 0x1D2A04)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x1D7F38)
void
wlc_phy_stay_in_carriersearch_acphy(void *pi, int enable)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1BE562)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1BE7AE)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x1c79a8)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_154, 0x1D2964)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x1D7E98)
void
wlc_phy_stopplayback_acphy(void *pi)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1BAF88)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1BB1D4)
AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x1C2EC8)
AT(CHIP_VER_BCM4358, FW_VER_7_112_201_3, 0x1C2FB4)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x1c4058)
AT(CHIP_VER_BCM4356, FW_VER_7_35_101_5_sta, 0x1CBBCC)
void
wlc_phy_table_read_acphy(void *pi, unsigned int id, unsigned int len, unsigned int offset, unsigned int width, void *data)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1B8CD2)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1B8F1E)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x1c2644)
void
wlc_phy_table_write_acphy(void *pi, unsigned int id, unsigned int len, unsigned int offset, unsigned int width, const void *data)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1BE876)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1BEAC2)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x1c8094)
int
wlc_phy_tx_tone_acphy(void *pi, int f_kHz, int max_val, char iqmode, char dac_test_mode, char modify_bbmult)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1B998C)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x1c3344)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_154, 0x1CDF2C)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x1D3464)
void
wlc_phy_txcal_txgain_cleanup_acphy(void *pi, void *orig_txgain)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1BE156)
void
wlc_phy_txpwr_by_index_acphy(void *pi, uint8 core_mask, int8 txpwrindex)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1B4608)
unsigned char
wlc_phy_txpwrctrl_get_cur_index_acphy(void *pi, unsigned char core)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1B4C1E)
void
wlc_phy_txpwrctrl_set_cur_index_acphy(void *pi, unsigned char idx, unsigned char core)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C0192)
void
wlc_phy_txpwrctrl_set_target_acphy(void *pi, unsigned char pwr_qtrdbm, unsigned char core)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C01B4)
AT(CHIP_VER_BCM4358, FW_VER_7_112_300_14, 0x1cb57c)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_154, 0x1D3AF8)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x1D902C)
void
wlc_phy_txpwrctrl_enable_acphy(void *pi, unsigned char ctrl_type)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1B45F0)
unsigned char
wlc_phy_txpwrctrl_ison_acphy(void *pi)
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
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x301F8)
void *
wlc_scb_lookup(void *wlc, void *bsscfg, char *ea)
RETURN_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x81F80C)
AT(CHIP_VER_BCM43430a1, FW_VER_ALL, 0x81F80C)
AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x2C798)
int
wlc_pdu_txhdr(void *wlc, void *p, void *scb)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1CA496)
void *
__wlc_scb_lookup_ram(void *wlc, void *bsscfg, char *ea, int bandunit)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x197A18)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x197B08)
int
wlc_sendpkt(void *wlc, void *p, int wlcif)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x8CA4C)
AT(CHIP_VER_BCM4335b0, FW_VER_ALL, 0x42C18)
void
wlc_send_q(void *wlc, void *qi)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x193348)
void *
wlc_sendnulldata(void *wlc, void *bsscfg, unsigned int *ea, int datarate_maybe, int p_field_26, int prio)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x07A448)
int
wlc_tdls_bsscfg_create(void *wlc, void *sdu, uint8 initiator)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x07A5E0)
int
wlc_tdls_buffer_rom(void *wlc, void *bsscfg, int a3)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x07A814)
int
wlc_tdls_cal_mic_chk(void *wlc, int a2, void* buf, int buflen, uint8 a5)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x07A9F8)
int
wlc_tdls_cal_teardown_mic_chk(void *wlc, int a2, void* buf, int buflen, int16 a5, uint8 a6)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x1DA82A)
int
wlc_tdls_endpoint_op(void *wlc, uint a2, int a3)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x07B2F4)
int
wlc_tdls_endpoint_op_rom(void *wlc, uint a2, int a3)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x07B700)
int
wlc_tdls_get_pkt(void *wlc, int a2, int a3)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x07B73C)
int
wlc_tdls_join(void *wlc, int a2, int a3, uint a4, uint a5)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x07F8D0)
int
wlc_tdls_port_open(void *wlc, uint8 a2)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x07C1C4)
int
wlc_tdls_process_chsw_req(void *tdls, void *scb, void *wlc_frminfo, int offset)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x07C3B8)
int
wlc_tdls_process_chsw_resp(void *tdls, void *scb, void *wlc_frminfo, int offset)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x1DAFD4)
int
wlc_tdls_process_discovery_req(void *tdls, void *scb, void *wlc_frminfo, int offset)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x07F954)
int
wlc_tdls_process_discovery_req_rom(void *tdls, void *scb, void *wlc_frminfo, int offset)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x07C6EC)
int
wlc_tdls_process_pti(void *tdls, void *scb, void *wlc_frminfo, int offset)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x07C7F8)
int
wlc_tdls_process_pti_resp(void *tdls, void *scb, void *wlc_frminfo, int offset)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x1DA350)
int
wlc_tdls_process_setup_cfm(void *tdls, void *scb, void *wlc_frminfo, int offset)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x07C954)
int
wlc_tdls_process_setup_cfm_rom(void *tdls, void *scb, void *wlc_frminfo, int offset)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x1D9D46)
int
wlc_tdls_process_setup_req(void *tdls, void *scb, void *wlc_frminfo, int offset)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x07CBF0)
int
wlc_tdls_process_setup_req_rom(void *tdls, void *scb, void *wlc_frminfo, int offset)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x1DA032)
int
wlc_tdls_process_setup_resp(void *tdls, void *scb, void *wlc_frminfo, int offset)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x07CEC0)
int
wlc_tdls_process_setup_resp_rom(void *tdls, void *scb, void *wlc_frminfo, int offset)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x1DA608)
int
wlc_tdls_process_setup_teardown(void *tdls, void *scb, void *wlc_frminfo, int offset)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x07D160)
int
wlc_tdls_process_setup_teardown_rom(void *tdls, void *scb, void *wlc_frminfo, int offset)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x1D9908)
int
wlc_tdls_process_vendor_specific(void *tdls, void *scb, void *wlc_frminfo, int offset)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x07D28C)
int
wlc_tdls_process_vendor_specific_rom(void *tdls, void *scb, void *wlc_frminfo, int offset)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x07FB18)
int
wlc_tdls_rcv_action_frame(void *wlc, void *scb, void *wlc_frminfo, int pdata_offset)
RETURN_DUMMY

AT(CHIP_VER_BCM4358, FW_VER_7_112_200_17, 0x07D4F8)
int
wlc_tdls_scb_lookup(void *wlc, int a2, int a3, void *a4, uint8 a5)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x193744)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x193834)
AT(CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327, 0xF4A8)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0xF4A8)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0xf680)
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
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_26_r640327, 0x44EC0)
AT(CHIP_VER_BCM43430a1, FW_VER_7_45_41_46, 0x455f8)
void
wlc_ucode_download(void *wlc_hw)
VOID_DUMMY

//AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x5BA28)
AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x01ADBB4)
AT(CHIP_VER_BCM43909b0, FW_VER_ALL, 0x37A30)
AT(CHIP_VER_BCM43455c0, FW_VER_7_45_189, 0x57770)
int
wlc_valid_chanspec_ext(void *wlc_cm, unsigned short chanspec, int dualband)
RETURN_DUMMY

AT(CHIP_VER_BCM43438, FW_VER_ALL, 0x84EEA0)
AT(CHIP_VER_BCM43430a1, FW_VER_ALL, 0x84EEA0)
int
wlc_valid_chanspec(void *wlc_cm, unsigned short chanspec, int dualband)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243, 0x1995C8)
void *
wlc_wlc_txq_enq(void *wlc, void *scb, void *p, int prec)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_ALL, 0x3BF28)
void
wlc_write_shm(void *wlc, unsigned int offset, unsigned short value)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1B47EC)
void
wlc_phy_tssi_phy_setup_acphy(void *pi, uint8 for_iqcal)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1B6884)
void
wlc_phy_tssi_radio_setup_acphy(void *pi, uint8 core_mask, uint8 for_iqcal)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1BEB3E)
void
wlc_phy_poll_samps_WAR_acphy(void *pi, int16 *samp, bool is_tssi, bool for_idle, void *target_gains, bool for_iqcal, bool init_adc_inside, uint16 core)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C0868)
uint8
wlc_phy_get_chan_freq_range_acphy(void *pi, uint channel)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C091C)
void
wlc_phy_get_paparams_for_band_acphy(void *pi, int16 *a1, int16 *b0, int16 *b1)
VOID_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1B3DB4)
uint8
wlc_phy_tssi2dbm_acphy(void *pi, int32 tssi, int32 a1, int32 b0, int32 b1)
RETURN_DUMMY

AT(CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704, 0x1C10C2)
uint8
wlc_phy_set_txpwr_clamp_acphy(void *pi)
RETURN_DUMMY

#undef VOID_DUMMY
#undef RETURN_DUMMY
#undef AT

#endif /*WRAPPER_C*/
