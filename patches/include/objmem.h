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
