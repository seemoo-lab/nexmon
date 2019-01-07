/* sminmpec.h
 * SMI Network Management Private Enterprise Codes for organizations
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 2004 Gerald Combs
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

#ifndef __SMINMPEC_H__
#define __SMINMPEC_H__

#include "ws_symbol_export.h"

/*
 * These are SMI Network Management Private Enterprise Codes for
 * organizations; see
 *
 *  http://www.iana.org/assignments/enterprise-numbers
 *
 * for a list.
 */
#define VENDOR_IETF                     0 /* reserved - used by the IETF in L2TP? */
#define VENDOR_ACC                      5
#define VENDOR_CISCO                    9
#define VENDOR_HEWLETT_PACKARD         11
#define VENDOR_SUN_MICROSYSTEMS        42
#define VENDOR_MERIT                   61
#define VENDOR_AT_AND_T                74
#define VENDOR_MOTOROLA               161
#define VENDOR_SHIVA                  166
#define VENDOR_ERICSSON               193
#define VENDOR_CISCO_VPN5000          255
#define VENDOR_LIVINGSTON             307
#define VENDOR_MICROSOFT              311
#define VENDOR_3COM                   429
#define VENDOR_ASCEND                 529
#define VENDOR_BAY                   1584
#define VENDOR_FOUNDRY               1991
#define VENDOR_VERSANET              2180
#define VENDOR_REDBACK               2352
#define VENDOR_JUNIPER               2636
#define VENDOR_APTIS                 2637
#define VENDOR_DT_AG                 2937
#define VENDOR_IXIA                  3054
#define VENDOR_CISCO_VPN3000         3076
#define VENDOR_COSINE                3085
#define VENDOR_SHASTA                3199
#define VENDOR_NETSCREEN             3224
#define VENDOR_NOMADIX               3309
#define VENDOR_T_MOBILE              3414 /* Former VoiceStream Wireless, Inc. */
#define VENDOR_BROADBAND_FORUM       3561 /* Former ADSL Forum */
#define VENDOR_ZTE                   3902
#define VENDOR_SIEMENS               4329
#define VENDOR_CABLELABS             4491
#define VENDOR_UNISPHERE             4874
#define VENDOR_CISCO_BBSM            5263
#define VENDOR_THE3GPP2              5535
#define VENDOR_SKT_TELECOM           5806
#define VENDOR_IP_UNPLUGGED          5925
#define VENDOR_ISSANNI               5948
#define VENDOR_NETSCALER             5951
#define VENDOR_DE_TE_MOBIL           6490
#define VENDOR_QUINTUM               6618
#define VENDOR_INTERLINK             6728
#define VENDOR_CNCTC                 7951
#define VENDOR_STARENT_NETWORKS      8164
#define VENDOR_COLUBRIS              8744
#define VENDOR_BARRACUDA            10704 /* Former phion Information Technologies */
#define VENDOR_ERICSSON_PKT_CORE    10923
#define VENDOR_COLUMBIA_UNIVERSITY  11862
#define VENDOR_THE3GPP              10415
#define VENDOR_GEMTEK_SYSTEMS       10529
#define VENDOR_FORTINET             12356
#define VENDOR_VERIZON              12951
#define VENDOR_PLIXER               13745
#define VENDOR_WIFI_ALLIANCE        14122
#define VENDOR_T_SYSTEMS_NOVA       16787
#define VENDOR_CHINATELECOM_GUANZHOU 20942
#define VENDOR_CACE                 32622
/* Greater than 32,767 need to be tagged unsigned. */
#define VENDOR_NTOP                 35632u
#define VENDOR_ERICSSON_CANADA_INC  46098u
#define VENDOR_CISCO_WIFI           4232704

WS_DLL_PUBLIC value_string_ext sminmpec_values_ext;

#endif /* __SMINMPEC_H__ */
