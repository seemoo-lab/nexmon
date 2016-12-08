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
#include <objmem.h>             // Functions to access object memory

void
wlc_bmac_read_objmem32_objaddr(struct wlc_hw_info *wlc_hw, unsigned int objaddr, unsigned int *val)
{
  volatile struct d11regs *regs;

  regs = wlc_hw->regs;
  regs->objaddr = objaddr;
  regs->objaddr;

  *val = regs->objdata;  
}

void
wlc_bmac_read_objmem32(struct wlc_hw_info *wlc_hw, unsigned int offset, unsigned int *val, int sel)
{
  wlc_bmac_read_objmem32_objaddr(wlc_hw, sel | ((offset & 0xfffffffc) >> 2), val);
}

void
wlc_bmac_read_objmem64_objaddr(struct wlc_hw_info *wlc_hw, unsigned int objaddr, unsigned int *val_low, unsigned int *val_high)
{
  volatile struct d11regs *regs;

  regs = wlc_hw->regs;
  regs->objaddr = objaddr;
  regs->objaddr;

  *val_low = regs->objdata;

  regs->objaddr = objaddr + 1;
  regs->objaddr;

  *val_high = regs->objdata;
}

void
wlc_bmac_read_objmem64(struct wlc_hw_info *wlc_hw, unsigned int offset, unsigned int *val_low, unsigned int *val_high, int sel)
{
  wlc_bmac_read_objmem64_objaddr(wlc_hw, sel | ((offset & 0xfffffff8) >> 2), val_low, val_high);
}

void
wlc_bmac_write_objmem64_objaddr(struct wlc_hw_info *wlc_hw, unsigned int objaddr, unsigned int val_low, unsigned int val_high)
{
  volatile struct d11regs *regs;

  regs = wlc_hw->regs;
  regs->objaddr = objaddr;
  regs->objaddr;

  regs->objdata = val_low;

  regs->objaddr = objaddr + 1;
  regs->objaddr;

  regs->objdata = val_high;
}

void
wlc_bmac_write_objmem64(struct wlc_hw_info *wlc_hw, unsigned int offset, unsigned int val_low, unsigned int val_high, int sel)
{
  wlc_bmac_write_objmem64_objaddr(wlc_hw, sel | ((offset & 0xfffffff8) >> 2), val_low, val_high);
}

void
wlc_bmac_write_objmem32_objaddr(struct wlc_hw_info *wlc_hw, unsigned int objaddr, unsigned int value)
{
  unsigned int low;
  unsigned int high;

    // first we read in one QWORD of existing bytes stored in the d11 object memory
  wlc_bmac_read_objmem64_objaddr(wlc_hw, objaddr, &low, &high);
    
    // then we replace the DWORD that should be written in this QWORD
  if (objaddr & 1) {
    high = value;
  } else {
    low = value;
  }

    // then we write back the changed QWORD into the object memory. We always access
    // a whole QWORD to be able to read back the written value at the next call to the
    // wlc_bmac_read_objmem64 function. Writing less than 64 bits (one QWORD) does not
    // deliver the new but the old value on the next read.
  wlc_bmac_write_objmem64_objaddr(wlc_hw, objaddr, low, high);
}

void
wlc_bmac_write_objmem32(struct wlc_hw_info *wlc_hw, unsigned int offset, unsigned int value, int sel)
{
  wlc_bmac_write_objmem32_objaddr(wlc_hw, sel | ((offset & 0xfffffff8) >> 2), value);
}

void
wlc_bmac_write_objmem_byte(struct wlc_hw_info *wlc_hw, unsigned int offset, unsigned char value, int sel)
{
  unsigned int low;
  unsigned int high;

    // first we read in one QWORD of existing bytes stored in the d11 object memory
  wlc_bmac_read_objmem64(wlc_hw, offset, &low, &high, sel);
    
    // then we replace the byte that should be written in this QWORD
  if (offset & 4) {
    ((unsigned char *) &high)[offset & 3] = value;
  } else {
    ((unsigned char *) &low)[offset & 3] = value;
  }

    // then we write back the changed QWORD into the object memory. We always access
    // a whole QWORD to be able to read back the written value at the next call to the
    // wlc_bmac_read_objmem64 function. Writing less than 64 bits (one QWORD) does not
    // deliver the new but the old value on the next read.
  wlc_bmac_write_objmem64(wlc_hw, offset, low, high, sel);
}

unsigned char
wlc_bmac_read_objmem_byte(struct wlc_hw_info *wlc_hw, unsigned int offset, int sel)
{
  unsigned int val;

  wlc_bmac_read_objmem32(wlc_hw, offset, &val, sel);
    
    return ((unsigned char *) &val)[offset & 0x3];
}
