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

#include <firmware_version.h>   // definition of firmware version macros
#include <debug.h>              // contains macros to access the debug hardware
#include <wrapper.h>            // wrapper definitions for functions that already exist in the firmware
#include <structs.h>            // structures that are used by the code in the firmware
#include <helper.h>             // useful helper functions
#include <patcher.h>            // macros used to craete patches such as BLPatch, BPatch, ...
#include <rates.h>              // rates used to build the ratespec for frame injection
#include <nexioctls.h>          // ioctls added in the nexmon patch
#include <capabilities.h>       // capabilities included in a nexmon patch
#include <sendframe.h>          // sendframe functionality
#include <version.h>            // version information
#include <bcmpcie.h>
#include <argprintf.h>          // allows to execute argprintf to print into the arg buffer

int 
wlc_ioctl_hook(struct wlc_info *wlc, int cmd, char *arg, int len, void *wlc_if)
{
    int ret = IOCTL_ERROR;
    argprintf_init(arg, len);

    switch (cmd) {
        case NEX_GET_CAPABILITIES:
            if (len == 4) {
                memcpy(arg, &capabilities, 4);
                ret = IOCTL_SUCCESS;
            }
            break;

        case NEX_WRITE_TO_CONSOLE:
            if (len > 0) {
                arg[len-1] = 0;
                printf("ioctl: %s\n", arg);
                ret = IOCTL_SUCCESS; 
            }
            break;

        case NEX_GET_VERSION_STRING:
            {
                int strlen = 0;
                for ( strlen = 0; version[strlen]; ++strlen );
                if (len >= strlen) {
                    memcpy(arg, version, strlen);
                    ret = IOCTL_SUCCESS;
                }
            }
            break;

        case 0x700: // reading the address of the shared structure (pciedev_shared_t)
            {
                pciedev_shared_t *pcie = *(pciedev_shared_t **) 0x23FFFC;
                
                argprintf("flags: %08x\n", pcie->flags);
                argprintf("trap_addr: %08x\n", pcie->trap_addr);
                argprintf("assert_exp_addr: %08x\n", pcie->assert_exp_addr);
                argprintf("assert_file_addr: %08x\n", pcie->assert_file_addr);
                argprintf("assert_line: %08x\n", pcie->assert_line);
                argprintf("console_addr: %08x\n", pcie->console_addr);
                argprintf("msgtrace_addr: %08x\n", pcie->msgtrace_addr);
                argprintf("fwid: %08x\n", pcie->fwid);
                argprintf("total_lfrag_pkt_cnt: %04x\n", pcie->total_lfrag_pkt_cnt);
                argprintf("max_host_rxbufs: %04x\n", pcie->max_host_rxbufs);
                argprintf("dma_rxoffset: %08x\n", pcie->dma_rxoffset);
                argprintf("h2d_mb_data_ptr: %08x\n", pcie->h2d_mb_data_ptr);
                argprintf("d2h_mb_data_ptr: %08x\n", pcie->d2h_mb_data_ptr);
                argprintf("rings_info_ptr: %08x\n", pcie->rings_info_ptr);
                argprintf("host_dma_scratch_buffer_len: %08x\n", pcie->host_dma_scratch_buffer_len);
                argprintf("host_dma_scratch_buffer: %08x %08x\n", pcie->host_dma_scratch_buffer.high_addr, pcie->host_dma_scratch_buffer.low_addr);
                argprintf("device_rings_stsblk_len: %08x\n", pcie->device_rings_stsblk_len);
                argprintf("device_rings_stsblk: %08x %08x\n", pcie->device_rings_stsblk.high_addr, pcie->device_rings_stsblk.low_addr);
                
                ret = IOCTL_SUCCESS;
            }
            break;

        case 0x701: // dumping the ring buffer physical addresses (ring_info_t)
            {
                pciedev_shared_t *pcie = *(pciedev_shared_t **) 0x23FFFC;
                ring_info_t *ring = pcie->rings_info_ptr;

                argprintf("ringmem_ptr: %08x\n", ring->ringmem_ptr);
                argprintf("h2d_w_idx_ptr: %08x\n", ring->h2d_w_idx_ptr);
                argprintf("h2d_r_idx_ptr: %08x\n", ring->h2d_r_idx_ptr);
                argprintf("d2h_w_idx_ptr: %08x\n", ring->d2h_w_idx_ptr);
                argprintf("d2h_r_idx_ptr: %08x\n", ring->d2h_r_idx_ptr);

                argprintf("h2d_w_idx_hostaddr: %08x %08x\n", ring->h2d_w_idx_hostaddr.high_addr, ring->h2d_w_idx_hostaddr.low_addr);
                argprintf("h2d_r_idx_hostaddr: %08x %08x\n", ring->h2d_r_idx_hostaddr.high_addr, ring->h2d_r_idx_hostaddr.low_addr);
                argprintf("d2h_w_idx_hostaddr: %08x %08x\n", ring->d2h_w_idx_hostaddr.high_addr, ring->d2h_w_idx_hostaddr.low_addr);
                argprintf("d2h_r_idx_hostaddr: %08x %08x\n", ring->d2h_r_idx_hostaddr.high_addr, ring->d2h_r_idx_hostaddr.low_addr);

                argprintf("max_sub_queues: %04x\n", ring->max_sub_queues);
                argprintf("rsvd: %04x\n", ring->rsvd);
                
                ret = IOCTL_SUCCESS;
            }
            break;

        case 0x702: // Dumping each of the rings metadata (ring_mem_t)
            {
                pciedev_shared_t *pcie = *(pciedev_shared_t **) 0x23FFFC;
                ring_info_t *ring = pcie->rings_info_ptr;
                ring_mem_t *mem = ring->ringmem_ptr;

                int i = 0;
                for (i = 0; i < BCMPCIE_COMMON_MSGRING_MAX_ID; i++) {
                    argprintf("ring: %d\n", i);
                    argprintf("idx: %04x\n", mem[i].idx);
                    argprintf("type: %02x\n", mem[i].type);
                    argprintf("max_item: %d\n", mem[i].max_item);
                    argprintf("len_items: %d\n", mem[i].len_items);
                    argprintf("base_addr: %08x %08x\n", mem[i].base_addr.high_addr, mem[i].base_addr.low_addr);
                }

                ret = IOCTL_SUCCESS;
            }
            break;

        default:
            ret = wlc_ioctl(wlc, cmd, arg, len, wlc_if);
    }

    return ret;
}

__attribute__((at(0x1F1DE8, "", CHIP_VER_BCM4358, FW_VER_7_112_200_17)))
__attribute__((at(0x1F1EE8, "", CHIP_VER_BCM4358, FW_VER_7_112_201_3)))
GenericPatch4(wlc_ioctl_hook, wlc_ioctl_hook + 1);
