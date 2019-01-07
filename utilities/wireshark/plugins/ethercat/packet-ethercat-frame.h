/* paket-ethercat-frame.h
 *
 * Copyright (c) 2007 by Beckhoff Automation GmbH
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef _PACKET_ETHERCAT_FRAME_H
#define _PACKET_ETHERCAT_FRAME_H

#include <ws_diag_control.h>

/* structure for decoding the header -----------------------------------------*/
DIAG_OFF(pedantic)
typedef union _EtherCATFrameParser
{
   struct
   {
      guint16 length   : 11;
      guint16 reserved : 1;
      guint16 protocol : 4;
   } v;
   guint16 hdr;
} EtherCATFrameParserHDR;
DIAG_ON(pedantic)
typedef EtherCATFrameParserHDR *PEtherCATFrameParserHDR;

#define EtherCATFrameParserHDR_Len (int)sizeof(EtherCATFrameParserHDR)

#endif /* _PACKET_ETHERCAT_FRAME_H */
