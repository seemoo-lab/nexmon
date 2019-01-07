/* packet-actrace.h
 * Routines for AudioCodes Trunk traces packet disassembly
 *
 * $Id: packet-actrace.h 18196 2006-05-21 04:49:01Z sahlberg $
 *
 * Copyright (c) 2005 by Alejandro Vaquero <alejandro.vaquero@verso.com>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1999 Gerald Combs
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
 
/* Container for tapping relevant data */
typedef struct _actrace_info_t
{
	int type; /* ACTRACE_CAS=1   ACTRACE_ISDN=2 */
	int direction;  /* direction BLADE_TO_PSTN=0 PSTN_TO_BLADE=1 */
	int trunk;
	gint32 cas_bchannel;
	gchar *cas_frame_label;
} actrace_info_t;

