/*

  Broadcom Sonics Silicon Backplane bus SPROM data modification tool

  Copyright (c) 2006-2008 Michael Buesch <m@bues.ch>
  Copyright (c) 2008 Larry Finger <Larry.Finger@lwfinger.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; see the file COPYING.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.

*/

#include "ssb_sprom.h"
#include "utils.h"

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>


struct cmdline_args cmdargs;
static uint8_t sprom_rev;
static uint16_t sprom_size;

/* SPROM layouts are described by the following table. The entries are as follows:
 *
 * uint16_t rev_mask	A bit mask of the sprom revisions that contain this data
 * enum valuetype type	The type of datum represented by this table entry
 * uint16_t length	The length of this datum in bits. A value of 34 means a MAC address.
 *			A value of 33 means a 2 character country code.
 * uint16_t offset	The offset (in bytes) from the start of the sprom.
 * uint16_t mask	The mask needed to extract this datum from the 16-bit word.
 * uint16_t shift	The shift needed to right align this datum.
 * char *desc		The short character string used to describe this datum.
 * char *label		The long character string that tells the function of this datum.
 *
 * The table is ended with a rev_mask of zero.
 */

static const struct var_entry sprom_table[] = {
	{ MASK_1_8, VAL_SUBP,   16, 0x04, 0xFFFF, 0x00, "subp",    "Subsystem Product ID" },
	{ MASK_1_8, VAL_SUBV,   16, 0x06, 0xFFFF, 0x00, "subv",    "Subsystem Vendor ID " },
	{ MASK_1_8, VAL_PPID,   16, 0x08, 0xFFFF, 0x00, "ppid",    "PCI Product ID      " },
	{ MASK_2_3, VAL_BFLHI,  16, 0x38, 0xFFFF, 0x00, "bflhi",   "High 16 bits of boardflags" },
	{ MASK_4,   VAL_BFLHI,  16, 0x46, 0xFFFF, 0x00, "bflhi",   "High 16 bits of boardflags" },
	{ MASK_5,   VAL_BFLHI,  16, 0x4C, 0xFFFF, 0x00, "bflhi",   "High 16 bits of boardflags" },
	{ MASK_8,   VAL_BFLHI,  16, 0x86, 0xFFFF, 0x00, "bflhi",   "High 16 bits of boardflags" },
	{ MASK_1_3, VAL_BFL,    16, 0x72, 0xFFFF, 0x00, "bfl",     "Low 16 bits of boardflags " },
	{ MASK_4,   VAL_BFL,    16, 0x44, 0xFFFF, 0x00, "bfl",     "Low 16 bits of boardflags " },
	{ MASK_5,   VAL_BFL,    16, 0x4A, 0xFFFF, 0x00, "bfl",     "Low 16 bits of boardflags " },
	{ MASK_8,   VAL_BFL,    16, 0x84, 0xFFFF, 0x00, "bfl",     "Low 16 bits of boardflags " },
	{ MASK_1_2, VAL_BGMAC,  34, 0x48, 0xFFFF, 0x00, "bgmac",   "MAC Address for 802.11b/g" },
	{ MASK_3,   VAL_BGMAC,  34, 0x4A, 0xFFFF, 0x00, "bgmac",   "MAC Address for 802.11b/g" },
	{ MASK_4,   VAL_BGMAC,  34, 0x4C, 0xFFFF, 0x00, "macadr",  "MAC Address" },
	{ MASK_5,   VAL_BGMAC,  34, 0x52, 0xFFFF, 0x00, "macadr",  "MAC Address" },
	{ MASK_8,   VAL_BGMAC,  34, 0x8C, 0xFFFF, 0x00, "macadr",  "MAC Address" },
	{ MASK_1_2, VAL_ETMAC,  34, 0x4E, 0xFFFF, 0x00, "etmac",   "MAC Address for ethernet " },
	{ MASK_1_2, VAL_AMAC,   34, 0x54, 0xFFFF, 0x00, "amac",    "MAC Address for 802.11a  " },
	{ MASK_1_3, VAL_ET0PHY,  5, 0x5A, 0x001F, 0x00, "et0phy",  "Ethernet phy settings(0)" },
	{ MASK_1_3, VAL_ET1PHY,  5, 0x5A, 0x03E0, 0x05, "et1phy",  "Ethernet phy settings(1)" },
	{ MASK_1_3, VAL_ET0MDC,  1, 0x5A, 0x4000, 0x0E, "et0mdc",  "MDIO for ethernet 0" },
	{ MASK_1_3, VAL_ET1MDC,  1, 0x5A, 0x8000, 0x0F, "et1mdc",  "MDIO for ethernet 1" },
	{ MASK_1_3, VAL_BREV,    8, 0x5C, 0x00FF, 0x00, "brev",    "Board revision" },
	{ MASK_4_5, VAL_BREV,    8, 0x42, 0x00FF, 0x00, "brev",    "Board revision" },
	{ MASK_8,   VAL_BREV,    8, 0x82, 0x00FF, 0x00, "brev",    "Board revision" },
	{ MASK_1_3, VAL_LOC,     4, 0x5C, 0x0300, 0x08, "loc",     "Locale / Country Code" },
	{ MASK_4,   VAL_LOC,    33, 0x52, 0xFFFF, 0x00, "ccode",   "Country Code" },
	{ MASK_5,   VAL_LOC,    33, 0x44, 0xFFFF, 0x00, "ccode",   "Country Code" },
	{ MASK_8,   VAL_LOC,    33, 0x92, 0xFFFF, 0x00, "ccode",   "Country Code" },
	{ MASK_4_5, VAL_REGREV, 16, 0x54, 0xFFFF, 0x00, "regrev",  "Regulatory revision" },
	{ MASK_8,   VAL_REGREV, 16, 0x94, 0xFFFF, 0x00, "regrev",  "Regulatory revision" },
	{ MASK_1_3, VAL_ANTBG0,  1, 0x5C, 0x1000, 0x0C, "antbg0",  "Antenna 0 available for B/G PHY" },
	{ MASK_1_3, VAL_ANTBG1,  1, 0x5C, 0x2000, 0x0D, "antbg1",  "Antenna 1 available for B/G PHY" },
	{ MASK_1_3, VAL_ANTA0,   1, 0x5C, 0x4000, 0x0E, "anta0",   "Antenna 0 available for A PHY" },
	{ MASK_1_3, VAL_ANTA1,   1, 0x5C, 0x8000, 0x0F, "anta1",   "Antenna 1 available for A PHY" },
	{ MASK_4_5, VAL_ANTBG0,  8, 0x5C, 0x00FF, 0x00, "antbg0",  "Available antenna bitmask for 2 GHz" },
	{ MASK_8,   VAL_ANTBG0,  8, 0x9C, 0x00FF, 0x00, "antbg0",  "Available antenna bitmask for 2 GHz" },
	{ MASK_4_5, VAL_ANTA0,   8, 0x5C, 0xFF00, 0x08, "anta0",   "Available antenna bitmask for 5 GHz" },
	{ MASK_8,   VAL_ANTA0,   8, 0x9C, 0xFF00, 0x08, "anta0",   "Available antenna bitmask for 5 GHz" },
	{ MASK_1_3, VAL_ANTGA,   8, 0x74, 0xFF00, 0x08, "antga" ,  "Antenna gain (5 GHz)" },
	{ MASK_1_3, VAL_ANTGBG,  8, 0x74, 0x00FF, 0x00, "antgbg",  "Antenna gain (2 GHz)" },
	{ MASK_4_5, VAL_ANTG0,   8, 0x5E, 0x00FF, 0x00, "antg0",   "Antenna 0 gain" },
	{ MASK_4_5, VAL_ANTG1,   8, 0x5E, 0xFF00, 0x08, "antg1",   "Antenna 1 gain" },
	{ MASK_4_5, VAL_ANTG2,   8, 0x60, 0x00FF, 0x00, "antg2",   "Antenna 2 gain" },
	{ MASK_4_5, VAL_ANTG3,   8, 0x60, 0xFF00, 0x08, "antg3",   "Antenna 3 gain" },
	{ MASK_8,   VAL_ANTG0,   8, 0x9E, 0x00FF, 0x00, "antg0",   "Antenna 0 gain" },
	{ MASK_8,   VAL_ANTG1,   8, 0x9E, 0xFF00, 0x08, "antg1",   "Antenna 1 gain" },
	{ MASK_8,   VAL_ANTG2,   8, 0xA0, 0x00FF, 0x00, "antg2",   "Antenna 2 gain" },
	{ MASK_8,   VAL_ANTG3,   8, 0xA0, 0xFF00, 0x08, "antg3",   "Antenna 3 gain" },
	{ MASK_1_3, VAL_PA0B0,  16, 0x5E, 0xFFFF, 0x00, "pa0b0",   "Power Amplifier W0 PAB0" },
	{ MASK_1_3, VAL_PA0B1,  16, 0x60, 0xFFFF, 0x00, "pa0b1",   "Power Amplifier W0 PAB1" },
	{ MASK_1_3, VAL_PA0B2,  16, 0x62, 0xFFFF, 0x00, "pa0b2",   "Power Amplifier W0 PAB2" },
	{ MASK_1_3, VAL_PA1B0,  16, 0x6A, 0xFFFF, 0x00, "pa1b0",   "Power Amplifier W1 PAB0" },
	{ MASK_1_3, VAL_PA1B1,  16, 0x6C, 0xFFFF, 0x00, "pa1b1",   "Power Amplifier W1 PAB1" },
	{ MASK_1_3, VAL_PA1B2,  16, 0x6E, 0xFFFF, 0x00, "pa1b2",   "Power Amplifier W1 PAB2" },
	{ MASK_1_3, VAL_LED0,    8, 0x64, 0x00FF, 0x00, "led0",    "LED 0 behavior" },
	{ MASK_1_3, VAL_LED1,    8, 0x64, 0xFF00, 0x08, "led1",    "LED 1 behavior" },
	{ MASK_1_3, VAL_LED2,    8, 0x66, 0x00FF, 0x00, "led2",    "LED 2 behavior" },
	{ MASK_1_3, VAL_LED3,    8, 0x66, 0xFF00, 0x08, "led3",    "LED 3 behavior" },
	{ MASK_4,   VAL_LED0,    8, 0x56, 0x00FF, 0x00, "led0",    "LED 0 behavior" },
	{ MASK_4,   VAL_LED1,    8, 0x56, 0xFF00, 0x08, "led1",    "LED 1 behavior" },
	{ MASK_4,   VAL_LED2,    8, 0x58, 0x00FF, 0x00, "led2",    "LED 2 behavior" },
	{ MASK_4,   VAL_LED3,    8, 0x58, 0xFF00, 0x08, "led3",    "LED 3 behavior" },
	{ MASK_5,   VAL_LED0,    8, 0x76, 0x00FF, 0x00, "led0",    "LED 0 behavior" },
	{ MASK_5,   VAL_LED1,    8, 0x76, 0xFF00, 0x08, "led1",    "LED 1 behavior" },
	{ MASK_5,   VAL_LED2,    8, 0x78, 0x00FF, 0x00, "led2",    "LED 2 behavior" },
	{ MASK_5,   VAL_LED3,    8, 0x78, 0xFF00, 0x08, "led3",    "LED 3 behavior" },
	{ MASK_1_3, VAL_MAXPBG,  8, 0x68, 0x00FF, 0x00, "maxpbg",  "B/G PHY max power out" },
	{ MASK_4_5, VAL_MAXPBG,  8, 0x80, 0x00FF, 0x00, "maxpbg",  "Max power 2GHz - Path 1" },
	{ MASK_8,   VAL_MAXPBG,  8, 0xC0, 0x00FF, 0x00, "maxpbg",  "Max power 2GHz - Path 1" },
	{ MASK_1_3, VAL_MAXPA,   8, 0x68, 0xFF00, 0x08, "maxpa",   "A PHY max power out  " },
	{ MASK_4_5, VAL_MAXPA,   8, 0x8A, 0x00FF, 0x00, "maxpa",   "Max power 5GHz - Path 1" },
	{ MASK_8,   VAL_MAXPA,   8, 0xCA, 0xFF00, 0x08, "maxpa",   "Max power 5GHz - Path 1" },
	{ MASK_1_3, VAL_ITSSIBG, 8, 0x70, 0x00FF, 0x00, "itssibg", "Idle TSSI target 2 GHz" },
	{ MASK_1_3, VAL_ITSSIA,  8, 0x70, 0xFF00, 0x08, "itssia",  "Idle TSSI target 5 GHz" },
	{ MASK_4_5, VAL_ITSSIBG, 8, 0x80, 0xFF00, 0x08, "itssibg", "Idle TSSI target 2 GHz - Path 1" },
	{ MASK_4_5, VAL_ITSSIA,  8, 0x8A, 0xFF00, 0x08, "itssia",  "Idle TSSI target 5 GHz - Path 1" },
	{ MASK_8,   VAL_ITSSIBG, 8, 0xC0, 0xFF00, 0x08, "itssibg", "Idle TSSI target 2 GHz - Path 1" },
	{ MASK_8,   VAL_ITSSIA,  8, 0xCA, 0xFF00, 0x08, "itssia",  "Idle TSSI target 5 GHz - Path 1" },
	{ MASK_8,   VAL_TPI2G0, 16, 0x62, 0xFFFF, 0x00, "tpi2g0",  "TX Power Index 2GHz" },
	{ MASK_8,   VAL_TPI2G1, 16, 0x64, 0xFFFF, 0x00, "tpi2g1",  "TX Power Index 2GHz" },
	{ MASK_8,   VAL_TPI5GM0,16, 0x66, 0xFFFF, 0x00, "tpi5gm0", "TX Power Index 5GHz middle subband" },
	{ MASK_8,   VAL_TPI5GM1,16, 0x68, 0xFFFF, 0x00, "tpi5gm1", "TX Power Index 5GHz middle subband" },
	{ MASK_8,   VAL_TPI5GL0,16, 0x6A, 0xFFFF, 0x00, "tpi5gl0", "TX Power Index 5GHz low subband   " },
	{ MASK_8,   VAL_TPI5GL1,16, 0x6C, 0xFFFF, 0x00, "tpi5gl1", "TX Power Index 5GHz low subband   " },
	{ MASK_8,   VAL_TPI5GH0,16, 0x6E, 0xFFFF, 0x00, "tpi5gh0", "TX Power Index 5GHz high subband  " },
	{ MASK_8,   VAL_TPI5GH1,16, 0x70, 0xFFFF, 0x00, "tpi5gh1", "TX Power Index 5GHz high subband  " },
	{ MASK_8,   VAL_2CCKPO, 16, 0x140,0xFFFF, 0x00, "cckpo2g", "2 GHz CCK power offset " },
	{ MASK_8,   VAL_2OFDMPO,32, 0x142,0xFFFF, 0x00, "ofdm2g",  "2 GHz OFDM power offset" },
	{ MASK_8,   VAL_5MPO,   32, 0x146,0xFFFF, 0x00, "ofdm5gm", "5 GHz OFDM middle subband power offset" },
	{ MASK_8,   VAL_5LPO,   32, 0x14A,0xFFFF, 0x00, "ofdm5gl", "5 GHz OFDM low subband power offset   " },
	{ MASK_8,   VAL_5HPO,   32, 0x14E,0xFFFF, 0x00, "ofdm5gh", "5 GHz OFDM high subband power offset  " },
	{ MASK_8,   VAL_2MCSPO, 16, 0x152,0xFFFF, 0x00, "mcspo2",  "2 GHz MCS power offset" },
	{ MASK_8,   VAL_5MMCSPO,16, 0x162,0xFFFF, 0x00, "mcspo5m", "5 GHz middle subband MCS power offset" },
	{ MASK_8,   VAL_5LMCSPO,16, 0x172,0xFFFF, 0x00, "mcspo5l", "5 GHz low subband MCS power offset   " },
	{ MASK_8,   VAL_5HMCSPO,16, 0x182,0xFFFF, 0x00, "mcspo5h", "5 GHz high subband MCS power offset  " },
	{ MASK_8,   VAL_CCDPO,  16, 0x192,0xFFFF, 0x00, "ccdpo",   "CCD power offset  " },
	{ MASK_8,   VAL_STBCPO, 16, 0x194,0xFFFF, 0x00, "stbcpo",  "STBC power offset " },
	{ MASK_8,   VAL_BW40PO, 16, 0x196,0xFFFF, 0x00, "bw40po",  "BW40 power offset " },
	{ MASK_8,   VAL_BWDUPPO,16, 0x198,0xFFFF, 0x00, "bwduppo", "BWDUP power offset" },
	{ MASK_4_5, VAL_TPI2G0, 16, 0x62, 0xFFFF, 0x00, "tpi2g0",  "TX Power Index 2GHz" },
	{ MASK_4_5, VAL_TPI2G1, 16, 0x64, 0xFFFF, 0x00, "tpi2g1",  "TX Power Index 2GHz" },
	{ MASK_4_5, VAL_TPI5GM0,16, 0x66, 0xFFFF, 0x00, "tpi5gm0", "TX Power Index 5GHz middle subband" },
	{ MASK_4_5, VAL_TPI5GM1,16, 0x68, 0xFFFF, 0x00, "tpi5gm1", "TX Power Index 5GHz middle subband" },
	{ MASK_4_5, VAL_TPI5GL0,16, 0x6A, 0xFFFF, 0x00, "tpi5gl0", "TX Power Index 5GHz low subband   " },
	{ MASK_4_5, VAL_TPI5GL1,16, 0x6C, 0xFFFF, 0x00, "tpi5gl1", "TX Power Index 5GHz low subband   " },
	{ MASK_4_5, VAL_TPI5GH0,16, 0x6E, 0xFFFF, 0x00, "tpi5gh0", "TX Power Index 5GHz high subband  " },
	{ MASK_4_5, VAL_TPI5GH1,16, 0x70, 0xFFFF, 0x00, "tpi5gh1", "TX Power Index 5GHz high subband  " },
	{ MASK_4_5, VAL_2CCKPO, 16, 0x138,0xFFFF, 0x00, "cckpo2g", "2 GHz CCK power offset " },
	{ MASK_4_5, VAL_2OFDMPO,32, 0x13A,0xFFFF, 0x00, "ofdm2g",  "2 GHz OFDM power offset" },
	{ MASK_4_5, VAL_5MPO,   32, 0x13E,0xFFFF, 0x00, "ofdm5gm", "5 GHz OFDM middle subband power offset" },
	{ MASK_4_5, VAL_5LPO,   32, 0x142,0xFFFF, 0x00, "ofdm5gl", "5 GHz OFDM low subband power offset   " },
	{ MASK_4_5, VAL_5HPO,   32, 0x146,0xFFFF, 0x00, "ofdm5gh", "5 GHz OFDM high subband power offset  " },
	{ MASK_4_5, VAL_2MCSPO, 16, 0x14A,0xFFFF, 0x00, "mcspo2",  "2 GHz MCS power offset" },
	{ MASK_4_5, VAL_5MMCSPO,16, 0x15A,0xFFFF, 0x00, "mcspo5m", "5 GHz middle subband MCS power offset" },
	{ MASK_4_5, VAL_5LMCSPO,16, 0x16A,0xFFFF, 0x00, "mcspo5l", "5 GHz low subband MCS power offset   " },
	{ MASK_4_5, VAL_5HMCSPO,16, 0x17A,0xFFFF, 0x00, "mcspo5h", "5 GHz high subband MCS power offset  " },
	{ MASK_4_5, VAL_CCDPO,  16, 0x18A,0xFFFF, 0x00, "ccdpo",   "CCD power offset  " },
	{ MASK_4_5, VAL_STBCPO, 16, 0x18C,0xFFFF, 0x00, "stbcpo",  "STBC power offset " },
	{ MASK_4_5, VAL_BW40PO, 16, 0x18E,0xFFFF, 0x00, "bw40po",  "BW40 power offset " },
	{ MASK_4_5, VAL_BWDUPPO,16, 0x190,0xFFFF, 0x00, "bwduppo", "BWDUP power offset" },
	/* per path variables are below here - only path 1 decoded for now */
	{ MASK_4_5, VAL_PA0B0,  16, 0xC2, 0xFFFF, 0x00, "pa0b0",   "Path 1: Power Amplifier W0 PAB0" },
	{ MASK_4_5, VAL_PA0B1,  16, 0xC4, 0xFFFF, 0x00, "pa0b1",   "Path 1: Power Amplifier W0 PAB1" },
	{ MASK_4_5, VAL_PA0B2,  16, 0xC6, 0xFFFF, 0x00, "pa0b2",   "Path 1: Power Amplifier W0 PAB2" },
	{ MASK_4_5, VAL_PA0B3,  16, 0xC8, 0xFFFF, 0x00, "pa0b3",   "Path 1: Power Amplifier W0 PAB3" },
	{ MASK_4_5, VAL_PA1B0,   8, 0xCC, 0x00FF, 0x00, "pam5h",   "Path 1: 5 GHz high subband PAM " },
	{ MASK_4_5, VAL_PA1B0,   8, 0xCC, 0xFF00, 0x08, "pam5l",   "Path 1: 5 GHz low subband PAM  " },
	{ MASK_4_5, VAL_5MPA0,  16, 0xCE, 0xFFFF, 0x00, "pa5m0",   "Path 1: 5 GHz Power Amplifier middle 0" },
	{ MASK_4_5, VAL_5MPA1,  16, 0xD0, 0xFFFF, 0x00, "pa5m1",   "Path 1: 5 GHz Power Amplifier middle 1" },
	{ MASK_4_5, VAL_5MPA2,  16, 0xD2, 0xFFFF, 0x00, "pa5m2",   "Path 1: 5 GHz Power Amplifier middle 2" },
	{ MASK_4_5, VAL_5MPA3,  16, 0xD4, 0xFFFF, 0x00, "pa5m3",   "Path 1: 5 GHz Power Amplifier middle 3" },
	{ MASK_4_5, VAL_5LPA0,  16, 0xD6, 0xFFFF, 0x00, "pa5l0",   "Path 1: 5 GHz Power Amplifier low 0   " },
	{ MASK_4_5, VAL_5LPA1,  16, 0xD8, 0xFFFF, 0x00, "pa5l1",   "Path 1: 5 GHz Power Amplifier low 1   " },
	{ MASK_4_5, VAL_5LPA2,  16, 0xDA, 0xFFFF, 0x00, "pa5l2",   "Path 1: 5 GHz Power Amplifier low 2   " },
	{ MASK_4_5, VAL_5LPA3,  16, 0xDC, 0xFFFF, 0x00, "pa5l3",   "Path 1: 5 GHz Power Amplifier low 3   " },
	{ MASK_4_5, VAL_5HPA0,  16, 0xDE, 0xFFFF, 0x00, "pa5h0",   "Path 1: 5 GHz Power Amplifier high 0  " },
	{ MASK_4_5, VAL_5HPA1,  16, 0xE0, 0xFFFF, 0x00, "pa5h1",   "Path 1: 5 GHz Power Amplifier high 1  " },
	{ MASK_4_5, VAL_5HPA2,  16, 0xE2, 0xFFFF, 0x00, "pa5h2",   "Path 1: 5 GHz Power Amplifier high 2  " },
	{ MASK_4_5, VAL_5HPA3,  16, 0xE4, 0xFFFF, 0x00, "pa5h3",   "Path 1: 5 GHz Power Amplifier high 3  " },
	{ MASK_8,   VAL_PA0B0,  16, 0xC2, 0xFFFF, 0x00, "pa0b0",   "SISO (Path 1) Power Amplifier W0 PAB0" },
	{ MASK_8,   VAL_PA0B1,  16, 0xC4, 0xFFFF, 0x00, "pa0b1",   "SISO (Path 1) Power Amplifier W0 PAB1" },
	{ MASK_8,   VAL_PA0B2,  16, 0xC6, 0xFFFF, 0x00, "pa0b2",   "SISO (Path 1) Power Amplifier W0 PAB2" },
	{ MASK_8,   VAL_PA1B0,  16, 0xCC, 0xFFFF, 0x00, "pa5m0",   "SISO (Path 1) 5 GHz Power Amplifier middle 0" },
	{ MASK_8,   VAL_PA1B1,  16, 0xCE, 0xFFFF, 0x00, "pa5m1",   "SISO (Path 1) 5 GHz Power Amplifier middle 1" },
	{ MASK_8,   VAL_PA1B2,  16, 0xD0, 0xFFFF, 0x00, "pa5m2",   "SISO (Path 1) 5 GHz Power Amplifier middle 2" },
	{ MASK_8,   VAL_5MPA0,  16, 0xD2, 0xFFFF, 0x00, "pa5l0",   "SISO (Path 1) 5 GHz Power Amplifier low 0   " },
	{ MASK_8,   VAL_5MPA1,  16, 0xD4, 0xFFFF, 0x00, "pa5l1",   "SISO (Path 1) 5 GHz Power Amplifier low 1   " },
	{ MASK_8,   VAL_5MPA2,  16, 0xD6, 0xFFFF, 0x00, "pa5l2",   "SISO (Path 1) 5 GHz Power Amplifier low 2   " },
	{ MASK_8,   VAL_5LPA0,  16, 0xD8, 0xFFFF, 0x00, "pa5h0",   "SISO (Path 1) 5 GHz Power Amplifier high 0  " },
	{ MASK_8,   VAL_5LPA1,  16, 0xDA, 0xFFFF, 0x00, "pa5h1",   "SISO (Path 1) 5 GHz Power Amplifier high 1  " },
	{ MASK_8,   VAL_5LPA2,  16, 0xDC, 0xFFFF, 0x00, "pa5h2",   "SISO (Path 1) 5 GHz Power Amplifier high 2  " },

	{ 0, },
};

/* find an item in the table by sprom revision and short description
 * returns length and type. The function value is -1 if the item is not
 * found, otherwise 0.
 */

static int locate_item_by_desc(int rev, enum valuetype *type, uint16_t *length, char *desc)
{
	int i;

	for (i = 0; ; i++) {
		if (sprom_table[i].rev_mask == 0)
			return -1;	/* end of table */
		if ((sprom_table[i].rev_mask & rev) &&
		     (!strcmp(sprom_table[i].desc, desc))) {
		/* this is the record we want */
			*length = sprom_table[i].length;
			*type = sprom_table[i].type;
			return 0;
		}
	}
	return -1; /* flow cannot reach here, but this statement makes gcc happy */
}

/* find an item in the table by sprom revision and type
 * return length, offset, mask, shift, desc, and label
 * The function returns -1 if no item matches the request.
 */

static int locate_item_rev(int rev, enum valuetype type, uint16_t *length, uint16_t *offset,
			   uint16_t *mask, uint16_t *shift, char *desc, char *label)
{
	int i;

	for (i = 0; ; i++) {
		if (sprom_table[i].rev_mask == 0)
			return -1;	/* end of table */
		if ((sprom_table[i].rev_mask & rev) &&
		     (sprom_table[i].type == type)) {
		/* this is the record we want */
			*length = sprom_table[i].length;
			*offset = sprom_table[i].offset;
			*mask = sprom_table[i].mask;
			*shift = sprom_table[i].shift;
			strcpy(desc, sprom_table[i].desc);
			strcpy(label, sprom_table[i].label);
			return 0;
		}
	}
	return -1; /* flow cannot reach here, but this statement makes gcc happy */
}

static int check_rev(uint16_t rev)
{
	if ((rev < 0) || (rev > 8) || (rev == 6) || (rev == 7)) {
		prerror("\nIllegal value for sprom_rev\n");
		return -1;
	}
	return 0;
}

static int hexdump_sprom(const uint8_t *sprom, char *buffer, size_t bsize)
{
	int i, pos = 0;

	for (i = 0; i < sprom_size; i++) {
		pos += snprintf(buffer + pos, bsize - pos - 1,
				"%02X", sprom[i] & 0xFF);
	}

	return pos + 1;
}

static uint8_t sprom_crc(const uint8_t *sprom)
{
	int i;
	uint8_t crc = 0xFF;

	for (i = 0; i < sprom_size - 1; i++)
		crc = crc8(crc, sprom[i]);
	crc ^= 0xFF;

	return crc;
}

static int write_output_binary(int fd, const uint8_t *sprom)
{
	ssize_t w;

	w = write(fd, sprom, sprom_size);
	if (w < 0)
		return -1;

	return 0;
}

static int write_output_hex(int fd, const uint8_t *sprom)
{
	ssize_t w;
	char tmp[SPROM4_SIZE * 2 + 10] = { 0 };

	hexdump_sprom(sprom, tmp, sizeof(tmp));
	prinfo("Raw output:  %s\n", tmp);
	w = write(fd, tmp, sprom_size * 2);
	if (w < 0)
		return -1;

	return 0;
}

static int write_output(int fd, const uint8_t *sprom)
{
	int err;

	if (cmdargs.outfile) {
		err = ftruncate(fd, 0);
		if (err) {
			prerror("Could not truncate --outfile %s\n",
				cmdargs.outfile);
			return -1;
		}
	}

	if (cmdargs.bin_mode)
		err = write_output_binary(fd, sprom);
	else
		err = write_output_hex(fd, sprom);
	if (err)
		prerror("Could not write output data.\n");

	return err;
}

static int modify_value(uint8_t *sprom,
			struct cmdline_vparm *vparm)
{
	const uint32_t v = vparm->u.value;
	uint16_t tmp = 0;
	uint16_t offset;
	char desc[100];
	char label[200];
	uint16_t length;
	uint16_t mask;
	uint16_t shift;
	uint16_t old_value;
	uint32_t value = 0;

	int rev_bit = BIT(sprom_rev);


	if (vparm->type == VAL_RAW) {
		sprom[vparm->u.raw.offset] = vparm->u.raw.value;
		return 0;
	}
	if (locate_item_rev(rev_bit, vparm->type, &length, &offset, &mask,
			    &shift, desc, label))
		return -1;

	if (length < 32) {
		old_value = sprom[offset + 0];
		old_value |= sprom[offset + 1] << 8;
		if (length < 16) {
			tmp = v << shift;
			value = (old_value & ~mask) | tmp;
		} else
			value = v;
		sprom[offset + 0] = (value & 0x00FF);
		sprom[offset + 1] = (value & 0xFF00) >> 8;
	} else if (length == 32) {
		value = v;
		sprom[offset + 0] = (value & 0x00FF);
		sprom[offset + 1] = (value >> 8) & 0xFF;
		sprom[offset + 2] = (value >> 16) & 0xFF;
		sprom[offset + 3] = (value >> 24) & 0xFF;
	} else if (length == 34) { /* MAC address */
		sprom[offset + 1] = vparm->u.mac[0];
		sprom[offset + 0] = vparm->u.mac[1];
		sprom[offset + 3] = vparm->u.mac[2];
		sprom[offset + 2] = vparm->u.mac[3];
		sprom[offset + 5] = vparm->u.mac[4];
		sprom[offset + 4] = vparm->u.mac[5];
	} else if (length == 33) { /* country code */
		sprom[offset + 1] = vparm->u.ccode[0];
		sprom[offset + 0] = vparm->u.ccode[1];
	} else {
		prerror("Incorrect value for length (%d)\n", length);
		exit(1);
	}

	return 0;
}

static int modify_sprom(uint8_t *sprom)
{
	struct cmdline_vparm *vparm;
	int i;
	int modified = 0;
	uint8_t crc;

	for (i = 0; i < cmdargs.nr_vparm; i++) {
		vparm = &(cmdargs.vparm[i]);
		if (!vparm->set)
			continue;
		modify_value(sprom, vparm);
		modified = 1;
	}
	if (modified) {
		/* Recalculate the CRC. */
		crc = sprom_crc(sprom);
		sprom[sprom_size - 1] = crc;
	}

	return modified;
}

static void display_value(const uint8_t *sprom,
			  struct cmdline_vparm *vparm)
{
	char desc[100];
	char label[200];
	char buffer[50];
	char tbuf[2];
	uint16_t offset;
	uint16_t length;
	uint16_t mask;
	uint16_t shift;
	uint32_t value = 0;
	int rev_bit = BIT(sprom_rev);
	const uint8_t *p;
	int i;

	if (locate_item_rev(rev_bit, vparm->type, &length, &offset, &mask,
			    &shift, desc, label))
		return;
	if (length < 32) {
		value = sprom[offset + 0];
		value |= sprom[offset + 1] << 8;
		value = (value & mask) >> shift;
	} else if (length == 32) {
		value = sprom[offset + 0];
		value |= sprom[offset + 1] << 8;
		value |= sprom[offset + 2] << 16;
		value |= sprom[offset + 3] << 24;
	}
	sprintf(buffer, "SPROM(0x%03X), %s,        ", offset, desc);
	buffer[25] = '\0';
	p = &(sprom[offset]);

	switch (length) {
	case 1:
		prdata("%s%s = %s\n", buffer, label, value ? "ON" : "OFF");
		break;
	case 4:
		prdata("%s%s = 0x%01X\n", buffer, label, (value & 0xF));
		break;
	case 5:
		prdata("%s%s = 0x%02X\n", buffer, label, (value & 0x1F));
		break;
	case 8:
		prdata("%s%s = 0x%02X\n", buffer, label, (value & 0xFF));
		break;
	case 16:
		prdata("%s%s = 0x%04X\n", buffer, label, value);
		break;
	case 32:
		prdata("%s%s = 0x%08X\n", buffer, label, value);
		break;
	case 33: /* alphabetic country code */
		for (i = 0; i < 2; i++) {
			tbuf[i] = p[i];
			if (!tbuf[i])	/* if not encoded, the value is zero */
				tbuf[i] = ' ';
		}
		prdata("%s%s = \"%c%c\"\n", buffer, label, tbuf[1], tbuf[0]);
		break;
	case 34:
		/* MAC address. */
		prdata("%s%s = %02x:%02x:%02x:%02x:%02x:%02x\n",
		       buffer, label, p[1], p[0], p[3], p[2], p[5], p[4]);
		break;
	default:
		prerror("vparm->bits internal error (%d)\n",
			vparm->bits);
		exit(1);
	}
}

static int display_sprom(const uint8_t *sprom)
{
	struct cmdline_vparm *vparm;
	int i;

	for (i = 0; i < cmdargs.nr_vparm; i++) {
		vparm = &(cmdargs.vparm[i]);
		if (vparm->set)
			continue;
		display_value(sprom, vparm);
	}

	return 0;
}

static int validate_input(const uint8_t *sprom)
{
	uint8_t crc, expected_crc;

	crc = sprom_crc(sprom);
	expected_crc = sprom[sprom_size - 1];

	if (crc != expected_crc) {
		prerror("Corrupt input data (crc: 0x%02X, expected: 0x%02X)\n",
			crc, expected_crc);
		if (!cmdargs.force)
			return 1;
	}

	return 0;
}

static int parse_input(uint8_t *sprom, char *buffer, size_t bsize)
{
	char *input;
	size_t inlen;
	size_t cnt;
	unsigned long parsed;
	char tmp[SPROM4_SIZE * 2 + 10] = { 0 };

	if (cmdargs.bin_mode) {
		/* The input buffer already contains
		 * the binary sprom data.
		 */
		internal_error_on(bsize != SPROM_SIZE && bsize != SPROM4_SIZE);
		memcpy(sprom, buffer, bsize);
		return 0;
	}

	inlen = bsize;
	input = strchr(buffer, ':');
	if (input) {
		input++;
		inlen -= input - buffer;
	} else
		input = buffer;

	if (inlen < SPROM_SIZE * 2) {
		prerror("Input data too short\n");
		return -1;
	}
	for (cnt = 0; cnt < inlen / 2; cnt++) {
		memcpy(tmp, input + cnt * 2, 2);
		parsed = strtoul(tmp, NULL, 16);
		sprom[cnt] = parsed & 0xFF;
	}
	/* check for 440 byte versions (V4 and higher) */
	if (inlen > 300) {
		sprom_rev = sprom[SPROM4_SIZE - 2];
		sprom_size = SPROM4_SIZE;
	} else {
		sprom_rev = sprom[SPROM_SIZE - 2];
		sprom_size = SPROM_SIZE;
	}
	if (check_rev(sprom_rev))
		exit(1);
	if (cmdargs.verbose) {
		hexdump_sprom(sprom, tmp, sizeof(tmp));
		prinfo("Raw input:  %s\n", tmp);
	}

	return 0;
}

static int read_infile(int fd, char **buffer, size_t *bsize)
{
	struct stat s;
	int err;
	ssize_t r;

	err = fstat(fd, &s);
	if (err) {
		prerror("Could not stat input file.\n");
		return err;
	}
	if (s.st_size == 0) {
		prerror("No input data\n");
		return -1;
	}
	if (cmdargs.bin_mode) {
		if (s.st_size != SPROM_SIZE && s.st_size != SPROM4_SIZE) {
			prerror("The input data is not SPROM Binary data. "
				"The size must be exactly %d (V1-3) "
				"or %d (V4-8) bytes, "
				"but it is %u bytes\n",
				SPROM_SIZE, SPROM4_SIZE,
				(unsigned int)(s.st_size));
			return -1;
		}
	} else {
		if (s.st_size > 1024 * 1024) {
			prerror("The input data does not look "
				"like SPROM HEX data (too long).\n");
			return -1;
		}
	}

	*bsize = s.st_size;
	if (!cmdargs.bin_mode)
		(*bsize)++;
	*buffer = malloce(*bsize);
	r = read(fd, *buffer, s.st_size);
	if (r != s.st_size) {
		prerror("Could not read input data.\n");
		return -1;
	}
	if (!cmdargs.bin_mode)
		(*buffer)[r] = '\0';

	return 0;
}

static void close_infile(int fd)
{
	if (cmdargs.infile)
		close(fd);
}

static void close_outfile(int fd)
{
	if (cmdargs.outfile)
		close(fd);
}

static int open_infile(int *fd)
{
	*fd = STDIN_FILENO;
	if (!cmdargs.infile)
		return 0;
	*fd = open(cmdargs.infile, O_RDONLY);
	if (*fd < 0) {
		prerror("Could not open --infile %s\n",
			cmdargs.infile);
		return -1;
	}

	return 0;
}

static int open_outfile(int *fd)
{
	*fd = STDOUT_FILENO;
	if (!cmdargs.outfile)
		return 0;
	*fd = open(cmdargs.outfile, O_RDWR | O_CREAT, 0644);
	if (*fd < 0) {
		prerror("Could not open --outfile %s\n",
			cmdargs.outfile);
		return -1;
	}

	return 0;
}

static void print_banner(int forceprint)
{
	const char *str = "Broadcom-SSB SPROM data modification tool.\n"
			  "\n"
			  "Copyright (C) Michael Buesch\n"
			  "Licensed under the GNU/GPL version 2 or later\n"
			  "\n"
			  "Be exceedingly careful with this tool. Improper"
			  " usage WILL BRICK YOUR DEVICE.\n";
	if (forceprint)
		prdata(str);
	else
		prinfo(str);
}

static void print_usage(int argc, char *argv[])
{
	enum valuetype loop;
	char desc[100];
	char label[200];
	char buffer[200];
	uint16_t offset;
	uint16_t length;
	uint16_t mask;
	uint16_t shift;
	int rev_bit;

	print_banner(1);
	prdata("\nUsage: %s [OPTION]\n", argv[0]);
	prdata("  -i|--input FILE           Input file\n");
	prdata("  -o|--output FILE          Output file\n");
	prdata("  -b|--binmode              The Input data is plain binary data and Output will be binary\n");
	prdata("  -V|--verbose              Be verbose\n");
	prdata("  -f|--force                Override error checks\n");
	prdata("  -v|--version              Print version\n");
	prdata("  -h|--help                 Print this help\n");
	prdata("\nValue Parameters:\n");
	prdata("\n");
	prdata("  -s|--rawset OFF,VAL       Set a VALue at a byte-OFFset\n");
	prdata("  -g|--rawget OFF           Get a value at a byte-OFFset\n");
	prdata("\n");

	for (sprom_rev = 1; sprom_rev < 9; sprom_rev++) {
	    if (sprom_rev == 6 || sprom_rev == 7)
		sprom_rev = 8;

	    rev_bit = BIT(sprom_rev);
	    prdata("\n================================================================\n"
	           "Rev. %d: Predefined values (for displaying (GET) or modification)\n"
	           "================================================================\n", sprom_rev);

	    for (loop = 0; loop <= VAL_LAST; loop++) {
		if (locate_item_rev(rev_bit, loop, &length, &offset, &mask,
			    &shift, desc, label))
			continue;

		switch (length) {
		case 34:
			sprintf(buffer, "  --%s [MAC-ADDR]%30s", desc, " ");
			break;
		case 33:
			sprintf(buffer, "  --%s [2 Char String]%30s", desc, " ");
			break;
		case 32:
			sprintf(buffer, "  --%s [0xFFFFFFFF]%30s", desc, " ");
			break;
		case 16:
			sprintf(buffer, "  --%s [0xFFFF]%30s", desc, " ");
			break;
		case 8:
			sprintf(buffer, "  --%s [0xFF]%30s", desc, " ");
			break;
		case 5:
			sprintf(buffer, "  --%s [0x1F]%30s", desc, " ");
			break;
		case 4:
			sprintf(buffer, "  --%s [0xF]%30s", desc, " ");
			break;
		case 1:
			sprintf(buffer, "  --%s [BOOL]%30s", desc, " ");
			break;
		default:
			prerror("Program error: Incorrect value of item length (%d)\n", length);
			exit(1);
		}
		buffer[28] = '\0';
		prdata("%s%s\n", buffer, label);
	    }
	}

	prdata("\n");
	prdata("  -P|--print-all            Display all values\n");
	prdata("\n");
	prdata(" BOOL      is a boolean value. Either 0 or 1\n");
	prdata(" 0xF..     is a hexadecimal value\n");
	prdata(" MAC-ADDR  is a MAC address in the format 00:00:00:00:00:00\n");
	prdata(" If the value parameter is \"GET\", the value will be printed;\n");
	prdata(" otherwise it is modified.\n");
	prdata("\nBe exceedingly careful with this tool. Improper"
	       " usage WILL BRICK YOUR DEVICE.\n");
}

#define ARG_MATCH		0
#define ARG_NOMATCH		1
#define ARG_ERROR		-1

static int do_cmp_arg(char **argv, int *pos,
		      const char *template,
		      int allow_merged,
		      char **param)
{
	char *arg;
	char *next_arg;
	size_t arg_len, template_len;

	arg = argv[*pos];
	next_arg = argv[*pos + 1];
	arg_len = strlen(arg);
	template_len = strlen(template);

	if (param) {
		/* Maybe we have a merged parameter here.
		 * A merged parameter is "-pfoobar" for example.
		 */
		if (allow_merged && arg_len > template_len) {
			if (memcmp(arg, template, template_len) == 0) {
				*param = arg + template_len;
				return ARG_MATCH;
			}
			return ARG_NOMATCH;
		} else if (arg_len != template_len)
			return ARG_NOMATCH;
		*param = next_arg;
	}
	if (strcmp(arg, template) == 0) {
		if (param) {
			if (*param == NULL) {
				prerror("%s needs a parameter\n", arg);
				return ARG_ERROR;
			}
			/* Skip the parameter on the next iteration. */
			(*pos)++;
		}
		return ARG_MATCH;
	}

	return ARG_NOMATCH;
}

/* Simple and lean command line argument parsing. */
static int cmp_arg(char **argv, int *pos,
		   const char *long_template,
		   const char *short_template,
		   char **param)
{
	int err;

	if (long_template) {
		err = do_cmp_arg(argv, pos, long_template, 0, param);
		if (err == ARG_MATCH || err == ARG_ERROR)
			return err;
	}
	err = ARG_NOMATCH;
	if (short_template)
		err = do_cmp_arg(argv, pos, short_template, 1, param);
	return err;
}

static int parse_err;

static int arg_match(char **argv, int *i,
		     const char *long_template,
		     const char *short_template,
		     char **param)
{
	int res;

	res = cmp_arg(argv, i, long_template,
		      short_template, param);
	if (res == ARG_ERROR) {
		parse_err = 1;
		return 0;
	}
	return (res == ARG_MATCH);
}

static int parse_value(const char *str,
		       struct cmdline_vparm *vparm,
		       const char *param)
{
	unsigned long v;
	int i;

	vparm->set = 1;
	if (strcmp(str, "GET") == 0 || strcmp(str, "get") == 0) {
		vparm->set = 0;
		return 0;
	}
	if (vparm->bits > 32)
		return 0;
	if (vparm->bits == 1) {
		/* This is a boolean value. */
		if (strcmp(str, "0") == 0)
			vparm->u.value = 0;
		else if (strcmp(str, "1") == 0)
			vparm->u.value = 1;
		else
			goto error_bool;
		return 1;
	}

	if (strncmp(str, "0x", 2) != 0)
		goto error;
	str += 2;
	/* The following logic presents a problem because the offsets
	 * for V4 SPROMs can be greater than 0xFF; however, the arguments
	 * are parsed before the SPROM revision is known. To fix this
	 * problem, if an input is expecting 0xFF-type input, then input
	 * of 0xFFF will be permitted */
	for (i = 0; i < vparm->bits / 4; i++) {
		if (str[i] == '\0')
			goto error;
	}
	if (str[i] != '\0') {
		if (i == 2)
			i++;		/* add an extra character */
		if (str[i] != '\0')
			goto error;
	}
	errno = 0;
	v = strtoul(str, NULL, 16);
	if (errno)
		goto error;
	vparm->u.value = v;

	return 1;
error:
	if (param) {
		prerror("%s value parsing error. Format: 0x", param);
		for (i = 0; i < vparm->bits / 4; i++)
			prerror("F");
		prerror("\n");
	}
	return -1;

error_bool:
	if (param)
		prerror("%s value parsing error. Format: 0 or 1 (boolean)\n", param);
	return -1;
}

static int parse_ccode(const char *str,
		       struct cmdline_vparm *vparm,
		       const char *param)
{
	const char *in = str;
	char *out = vparm->u.ccode;

	vparm->bits = 33;
	vparm->set = 1;
	if (strcmp(str, "GET") == 0 || strcmp(str, "get") == 0) {
		vparm->set = 0;
		return 0;
	}

	memcpy(out, in, 2);
	return 1;
}

static int parse_mac(const char *str,
		     struct cmdline_vparm *vparm,
		     const char *param)
{
	int i;
	char *delim;
	const char *in = str;
	uint8_t *out = vparm->u.mac;

	vparm->bits = 34;
	vparm->set = 1;
	if (strcmp(str, "GET") == 0 || strcmp(str, "get") == 0) {
		vparm->set = 0;
		return 0;
	}

	for (i = 0; ; i++) {
		errno = 0;
		out[i] = strtoul(in, NULL, 16);
		if (errno)
			goto error;
		if (i == 5) {
			if (in[1] != '\0' && in[2] != '\0')
				goto error;
			break;
		}
		delim = strchr(in, ':');
		if (!delim)
			goto error;
		in = delim + 1;
	}

	return 1;
error:
	prerror("%s MAC parsing error. Format: 00:00:00:00:00:00\n", param);
	return -1;
}

static int parse_rawset(const char *str,
			struct cmdline_vparm *vparm)
{
	char *delim;
	uint8_t value;
	uint16_t offset;
	int err;

	vparm->type = VAL_RAW;

	delim = strchr(str, ',');
	if (!delim)
		goto error;
	*delim = '\0';
	err = parse_value(str, vparm, NULL);
	if (err != 1)
		goto error;
	offset = vparm->u.value;
	if (offset >= SPROM4_SIZE) {
		prerror("--rawset offset too big (>= 0x%02X)\n",
			SPROM4_SIZE);
		return -1;
	}
	err = parse_value(delim + 1, vparm, NULL);
	if (err != 1)
		goto error;
	value = vparm->u.value;

	vparm->u.raw.value = value;
	vparm->u.raw.offset = offset;
	vparm->set = 1;

	return 0;
error:
	prerror("--rawset value parsing error. Format: 0xFF,0xFF "
		"(first Offset, second Value)\n");
	return -1;
}

static int parse_rawget(const char *str,
			struct cmdline_vparm *vparm)
{
	int err;
	uint16_t offset;

	vparm->type = VAL_RAW;

	err = parse_value(str, vparm, "--rawget");
	if (err != 1)
		return -1;
	offset = vparm->u.value;
	if (offset >= SPROM4_SIZE) {
		prerror("--rawget offset too big (>= 0x%02X)\n",
			SPROM4_SIZE);
		return -1;
	}

	vparm->u.raw.offset = offset;
	vparm->type = VAL_RAW;
	vparm->set = 0;

	return 0;
}

static int generate_printall(void)
{
	enum valuetype vt = 0;
	int j;

	for (vt = 0; vt <= VAL_LAST; vt++) {
		if (cmdargs.nr_vparm == MAX_VPARM) {
			prerror("Too many value parameters.\n");
			return -1;
		}
		for (j = 0; ; j++) {
			enum valuetype type = sprom_table[j].type;
			short mask = sprom_table[j].rev_mask;

			if (mask == 0)
				break;
			if ((mask & BIT(sprom_rev)) && (type == vt)) {
				cmdargs.vparm[cmdargs.nr_vparm].type = vt;
				cmdargs.vparm[cmdargs.nr_vparm].set = 0;
				cmdargs.vparm[cmdargs.nr_vparm++].bits = sprom_table[j].length;
			}
		}
	}
	return 0;
}

static int parse_args(int argc, char *argv[], int pass)
{
	struct cmdline_vparm *vparm;
	int i, err;
	char *param;
	char *arg;
	uint16_t length;
	enum valuetype type;

	parse_err = 0;
	for (i = 1; i < argc; i++) {
		if (cmdargs.nr_vparm == MAX_VPARM) {
			prerror("Too many value parameters.\n");
			return -1;
		}

		if (arg_match(argv, &i, "--version", "-v", NULL)) {
			print_banner(1);
			return 1;
		} else if (arg_match(argv, &i, "--help", "-h", NULL)) {
			goto out_usage;
		} else if (arg_match(argv, &i, "--input", "-i", &param)) {
			cmdargs.infile = param;
		} else if (arg_match(argv, &i, "--output", "-o", &param)) {
			cmdargs.outfile = param;
		} else if (arg_match(argv, &i, "--verbose", "-V", NULL)) {
			cmdargs.verbose = 1;
		} else if (arg_match(argv, &i, "--force", "-n", NULL)) {
			cmdargs.force = 1;
		} else if (arg_match(argv, &i, "--binmode", "-b", NULL)) {
			cmdargs.bin_mode = 1;
		} else if (pass == 2 && arg_match(argv, &i, "--rawset", "-s", &param)) {
			vparm = &(cmdargs.vparm[cmdargs.nr_vparm++]);
			err = parse_rawset(param, vparm);
			if (err < 0)
				goto error;
		} else if (pass == 2 && arg_match(argv, &i, "--rawget", "-g", &param)) {
			vparm = &(cmdargs.vparm[cmdargs.nr_vparm++]);
			err = parse_rawget(param, vparm);
			if (err < 0)
				goto error;

		} else if (pass == 2 && arg_match(argv, &i, "--print-all", "-P", NULL)) {
			err = generate_printall();
			if (err)
				goto error;

		} else if (pass == 2) {
			arg = argv[i];
			if (arg[0] != '-' || arg[1] != '-')
				goto out_usage;		/* all must start with "--" */
			if (locate_item_by_desc(BIT(sprom_rev), &type, &length, arg + 2))
				goto out_usage;
			arg_match(argv, &i, arg, NULL, &param);
			vparm = &(cmdargs.vparm[cmdargs.nr_vparm++]);
			vparm->type = type;
			vparm->bits = length;
			err = parse_value(param, vparm, arg);
			if (err < 0)
				goto error;
			if (length == 34) {
				err = parse_mac(param, vparm, arg);
				if (err < 0)
					goto error;
			}
			if (length == 33) {
				err = parse_ccode(param, vparm, arg);
				if (err < 0)
					goto error;
			}
		}
		if (parse_err)
			goto out_usage;
	}
	if (pass == 2 && cmdargs.nr_vparm == 0) {
		prerror("No Value parameter given. See --help.\n");
		return -1;
	}
	return 0;

out_usage:
	print_usage(argc, argv);
error:
	return -1;	
}


int main(int argc, char **argv)
{
	int err;
	int fd;
	uint8_t sprom[SPROM4_SIZE + 10];
	char *buffer = NULL;
	size_t buffer_size = 0;

	/* Some arguments require that the revision of the sprom be known,
	 * but that is not known until the sprom data are read. This difficulty
	 * is handled by making two passes through the argument list. The first
	 * only process those arguments that do not depend on sprom revision.
	 *
	 * Do the first pass through arguments
	 */
	err = parse_args(argc, argv, 1);
	if (err == 1)
		return 0;
	else if (err != 0)
		goto out;

	print_banner(0);
	prinfo("\nReading input from \"%s\"...\n",
	       cmdargs.infile ? cmdargs.infile : "stdin");

	err = open_infile(&fd);
	if (err)
		goto out;
	err = read_infile(fd, &buffer, &buffer_size);
	close_infile(fd);
	if (err)
		goto out;
	err = parse_input(sprom, buffer, buffer_size);
	free(buffer);
	if (err)
		goto out;
	err = validate_input(sprom);
	if (err)
		goto out;

	/* do second pass through argument list */
	err = parse_args(argc, argv, 2);
	if (err == 1)
		return 0;
	else if (err != 0)
		goto out;

	err = display_sprom(sprom);
	if (err)
		goto out;
	err = modify_sprom(sprom);
	if (err < 0)
		goto out;
	if (err) {
		err = open_outfile(&fd);
		if (err)
			goto out;
		err = write_output(fd, sprom);
		close_outfile(fd);
		if (err)
			goto out;
		prinfo("SPROM modified.\n");
	}
	prdata("The input file is data from a revision %d SPROM.\n", sprom_rev);
out:
	return err;
}
