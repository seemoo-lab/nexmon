/*
 * packet-radiotap.h
 *	Declarations for packet-radiotap.c
 *
 * $Id: packet-radiotap.h 26861 2008-11-27 08:34:41Z sfisher $
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * Copied from README.developer
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

void capture_radiotap(const guchar *pd, int offset, int len, packet_counts *ld);
void proto_register_radiotap(void);
void proto_reg_handoff_radiotap(void);

struct _radiotap_info {
  guint radiotap_length;
  guint32 rate;
  gint8 dbm_antsignal;
  gint8 dbm_antnoise;
  guint32 freq;
  guint32 flags;
  guint64 tsft;
};
