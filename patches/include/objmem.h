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

#include <structs.h>            // structures that are used by the code in the firmware

void wlc_bmac_read_objmem32_objaddr(struct wlc_hw_info *wlc_hw, unsigned int objaddr, unsigned int *val);
void wlc_bmac_read_objmem32(struct wlc_hw_info *wlc_hw, unsigned int offset, unsigned int *val, int sel);
void wlc_bmac_read_objmem64_objaddr(struct wlc_hw_info *wlc_hw, unsigned int objaddr, unsigned int *val_low, unsigned int *val_high);
void wlc_bmac_read_objmem64(struct wlc_hw_info *wlc_hw, unsigned int offset, unsigned int *val_low, unsigned int *val_high, int sel);
void wlc_bmac_write_objmem64_objaddr(struct wlc_hw_info *wlc_hw, unsigned int objaddr, unsigned int val_low, unsigned int val_high);
void wlc_bmac_write_objmem64(struct wlc_hw_info *wlc_hw, unsigned int offset, unsigned int val_low, unsigned int val_high, int sel);
void wlc_bmac_write_objmem32_objaddr(struct wlc_hw_info *wlc_hw, unsigned int objaddr, unsigned int value);
void wlc_bmac_write_objmem32(struct wlc_hw_info *wlc_hw, unsigned int offset, unsigned int value, int sel);
void wlc_bmac_write_objmem_byte(struct wlc_hw_info *wlc_hw, unsigned int offset, unsigned char value, int sel);
unsigned char wlc_bmac_read_objmem_byte(struct wlc_hw_info *wlc_hw, unsigned int offset, int sel);
