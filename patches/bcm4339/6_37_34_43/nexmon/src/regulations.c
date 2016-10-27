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

#include <firmware_version.h>

__attribute__((at(0x1fa034, "", CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243)))
__attribute__((at(0x1fa040, "", CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704)))
__attribute__((aligned(1)))
unsigned char _locale_channels[] = {
  0x00, 
  0x01, 0xFF, 
  0x01, 0xFF, 
  0x02, 0xFF, 0xFF, 
  0x04, 0xFF, 0xFF, 0xFF, 0xFF, 
  0x02, 0xFF, 0xFF, 
  0x02, 0xFF, 0xFF, 
  0x01, 0xFF, 
  0x04, 0xFF, 0xFF, 0xFF, 0xFF, 
  0x03, 0xFF, 0xFF, 0xFF, 
  0x04, 0xFF, 0xFF, 0xFF, 0xFF, 
  0x04, 0xFF, 0xFF, 0xFF, 0xFF, 
  0x03, 0xFF, 0xFF, 0xFF,
  0x03, 0xFF, 0xFF, 0xFF,
  0x03, 0xFF, 0xFF, 0xFF,
  0x02, 0xFF, 0xFF, 
  0x02, 0xFF, 0xFF,
  0x02, 0xFF, 0xFF,
  0x01, 0xFF, 
  0x02, 0xFF, 0xFF,
  0x02, 0xFF, 0xFF,
  0x02, 0xFF, 0xFF,
  0x02, 0xFF, 0xFF,
  0x02, 0xFF, 0xFF,
  0x02, 0xFF, 0xFF,
  0x02, 0xFF, 0xFF,
  0x01, 0xFF, 
  0x02, 0xFF, 0xFF,
  0x02, 0xFF, 0xFF,
  0x01, 0xFF, 
  0x01, 0xFF,
  0x02, 0xFF, 0xFF,
  0x02, 0xFF, 0xFF,
  0x01, 0xFF, 
  0x01, 0xFF, 
  0x01, 0xFF
};

__attribute__((at(0x1fa480, "", CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243)))
__attribute__((at(0x1fa48c, "", CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704)))
__attribute__((aligned(1)))
unsigned char _valid_channel_2g_20m[] = { 0x01, 0x0f, 0x01, 0x00 };

__attribute__((at(0x1fb082, "", CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243)))
__attribute__((at(0x1fb08e, "", CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704)))
__attribute__((aligned(1)))
unsigned char _valid_channel_5g_20m[] = { 0x22, 0x2e, 0x04, 0x24 };
//unsigned char _valid_channel_5g_20m[] = { 0xB8, 0xB8, 0x01, 0x24 };

/*
unsigned char _locale_channels[] = {
  0x00, 
  0x01, 0x02, 
  0x01, 0x03, 
  0x02, 0x0b, 0x0c, 
  0x04, 0x0b, 0x0c, 0x18, 0x37, 
  0x02, 0x0b, 0x0d, 
  0x02, 0x0b, 0x10, 
  0x01, 0x0c, 
  0x04, 0x0c, 0x16, 0x2f, 0x38, 
  0x03, 0x0c, 0x16, 0x38, 
  0x04, 0x0c, 0x17, 0x2f, 0x38, 
  0x04, 0x0c, 0x17, 0x30, 0x38, 
  0x03, 0x0c, 0x18, 0x37, 
  0x03, 0x0c, 0x19, 0x38,
  0x03, 0x0c, 0x1a, 0x38, 
  0x02, 0x0c, 0x1b, 
  0x02, 0x0c, 0x37, 
  0x02, 0x0c, 0x38, 
  0x01, 0x0d, 
  0x02, 0x0d, 0x24, 
  0x02, 0x0d, 0x26, 
  0x02, 0x0d, 0x2f,
  0x02, 0x0d, 0x37, 
  0x02, 0x0d, 0x38, 
  0x02, 0x0e, 0x31, 
  0x02, 0x0f, 0x38,
  0x01, 0x10, 
  0x02, 0x10, 0x38, 
  0x02, 0x11, 0x38, 
  0x01, 0x12, 
  0x01, 0x13,
  0x02, 0x16, 0x38, 
  0x02, 0x1d, 0x26, 
  0x01, 0x24, 
  0x01, 0x37, 
  0x01, 0x38
};
*/