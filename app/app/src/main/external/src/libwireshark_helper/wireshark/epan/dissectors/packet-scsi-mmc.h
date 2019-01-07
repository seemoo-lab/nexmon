/* packet-scsi-mmc.h
 * Ronnie sahlberg 2006
 *
 * $Id: packet-scsi-mmc.h 29512 2009-08-23 06:57:16Z jmayer $
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 2002 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __PACKET_SCSI_MMC_H_
#define __PACKET_SCSI_MMC_H_

extern int hf_scsi_mmc_opcode;
extern scsi_cdb_table_t scsi_mmc_table[256];

#endif
