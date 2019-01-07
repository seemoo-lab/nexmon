/* pcap-encap.h
 * Declarations for routines to handle libpcap/pcap-NG linktype values
 *
 * Wiretap Library
 * Copyright (c) 1998 by Gilbert Ramirez <gram@alumni.rice.edu>
 *
 * File format support for pcap-ng file format
 * Copyright (c) 2007 by Ulf Lamping <ulf.lamping@web.de>
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

#ifndef __W_PCAP_ENCAP_H__
#define __W_PCAP_ENCAP_H__

#include <glib.h>
#include <wiretap/wtap.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

WS_DLL_PUBLIC int wtap_pcap_encap_to_wtap_encap(int encap);
WS_DLL_PUBLIC int wtap_wtap_encap_to_pcap_encap(int encap);
WS_DLL_PUBLIC gboolean wtap_encap_requires_phdr(int encap);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
