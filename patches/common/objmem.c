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
