/* packet-cwids.c
 * Routines for dissecting wireless ids packets sent from a Cisco
 * access point to the WLSE (or whatever)
 *
 * Copyright 2006 Joerg Mayer (see AUTHORS file)
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

/* With current IOS, you can use Cisco wireless Bridges/APs as
 * wireless sniffers and configure them to send the data to some
 * central IDS:
 * interface dot11Radio 0
 *   station-role scanner
 *   monitor frames endpoint ip address 172.22.1.1 port 8999 truncate 2312
 * These frames are raw, i.e. they don't have a pcap header.
 * Running wireshark at the receiving end will provide those.
 */

/* 2do:
 *	- Find out more about the contents of the capture header
 *	- Protect the address fields etc (all columns?)
 *	- Create subelements and put each header and packet into it
 *	- fuzz-test the dissector
 *	- Find some heuristic to detect the packet automagically and
 *	  convert dissector into a heuristic dissector
 *	- Is the TRY/CATCH stuff OK?
 */

#include "config.h"

#include <wiretap/wtap.h>

#include <epan/packet.h>
#include <epan/exceptions.h>
#include <epan/expert.h>
#include <epan/prefs.h>
#include <epan/show_exception.h>

static guint global_udp_port = 0;

static int proto_cwids = -1;
static int hf_cwids_version = -1;
static int hf_cwids_timestamp = -1;
static int hf_cwids_unknown1 = -1;
static int hf_cwids_channel = -1;
static int hf_cwids_unknown2 = -1;
static int hf_cwids_reallength = -1;
static int hf_cwids_capturelen = -1;
static int hf_cwids_unknown3 = -1;

static gint ett_cwids = -1;

static expert_field ie_ieee80211_subpacket = EI_INIT;

static dissector_handle_t ieee80211_radio_handle;

static int
dissect_cwids(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
	tvbuff_t *wlan_tvb;
	proto_tree *ti, *cwids_tree;
	volatile int offset = 0;
	guint16 capturelen;

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "CWIDS");
	col_set_str(pinfo->cinfo, COL_INFO, "Cwids: ");
	/* FIXME: col_set_fence(pinfo->cinfo, all-cols, only addr-cols?); */

	cwids_tree = NULL;

	while(tvb_reported_length_remaining(tvb, offset) > 0) {
		struct ieee_802_11_phdr phdr;

		ti = proto_tree_add_item(tree, proto_cwids, tvb, offset, 28, ENC_NA);
		cwids_tree = proto_item_add_subtree(ti, ett_cwids);

		memset(&phdr, 0, sizeof(phdr));
		phdr.fcs_len = 0;	/* no FCS */
		phdr.decrypted = FALSE;
		phdr.datapad = FALSE;
		phdr.phy = PHDR_802_11_PHY_UNKNOWN;
		proto_tree_add_item(cwids_tree, hf_cwids_version, tvb, offset, 2, ENC_BIG_ENDIAN);
		offset += 2;
		proto_tree_add_item(cwids_tree, hf_cwids_timestamp, tvb, offset, 6, ENC_NA);
		offset += 6;
		proto_tree_add_item(cwids_tree, hf_cwids_unknown1, tvb, offset, 1, ENC_NA);
		offset += 1;
		phdr.has_channel = TRUE;
		phdr.channel = tvb_get_guint8(tvb, offset);
		proto_tree_add_item(cwids_tree, hf_cwids_channel, tvb, offset, 1, ENC_BIG_ENDIAN);
		offset += 1;
		proto_tree_add_item(cwids_tree, hf_cwids_unknown2, tvb, offset, 6, ENC_NA);
		offset += 6;
		proto_tree_add_item(cwids_tree, hf_cwids_reallength, tvb, offset, 2, ENC_BIG_ENDIAN);
		offset += 2;
		capturelen = tvb_get_ntohs(tvb, offset);
		proto_tree_add_item(cwids_tree, hf_cwids_capturelen, tvb, offset, 2, ENC_BIG_ENDIAN);
		offset += 2;
		proto_tree_add_item(cwids_tree, hf_cwids_unknown3, tvb, offset, 8, ENC_NA);
		offset += 8;

		wlan_tvb = tvb_new_subset_length(tvb, offset, capturelen);
		/* Continue after ieee80211 dissection errors */
		TRY {
			call_dissector_with_data(ieee80211_radio_handle, wlan_tvb, pinfo, tree, &phdr);
		} CATCH_BOUNDS_ERRORS {
			show_exception(wlan_tvb, pinfo, tree, EXCEPT_CODE, GET_MESSAGE);

			expert_add_info(pinfo, ti, &ie_ieee80211_subpacket);
		} ENDTRY;

		offset += capturelen;
	}
	return tvb_captured_length(tvb);
}

void proto_register_cwids(void);
void proto_reg_handoff_cwids(void);

void
proto_register_cwids(void)
{
	static hf_register_info hf[] = {
		{ &hf_cwids_version,
		{ "Capture Version", "cwids.version", FT_UINT16, BASE_DEC, NULL,
			0x0, "Version or format of record", HFILL }},

		{ &hf_cwids_timestamp,
		{ "Timestamp [us]", "cwids.timestamp", FT_BYTES, BASE_NONE, NULL,
			0x0, NULL, HFILL }},

		{ &hf_cwids_unknown1,
		{ "Unknown1", "cwids.unknown1", FT_BYTES, BASE_NONE, NULL,
			0x0, "1st Unknown block", HFILL }},

		{ &hf_cwids_channel,
		{ "Channel", "cwids.channel", FT_UINT8, BASE_DEC, NULL,
			0x0, "Channel for this capture", HFILL }},

		{ &hf_cwids_unknown2,
		{ "Unknown2", "cwids.unknown2", FT_BYTES, BASE_NONE, NULL,
			0x0, "2nd Unknown block", HFILL }},

		{ &hf_cwids_reallength,
		{ "Original length", "cwids.reallen", FT_UINT16, BASE_DEC, NULL,
			0x0, "Original num bytes in frame", HFILL }},

		{ &hf_cwids_capturelen,
		{ "Capture length", "cwids.caplen", FT_UINT16, BASE_DEC, NULL,
			0x0, "Captured bytes in record", HFILL }},

		{ &hf_cwids_unknown3,
		{ "Unknown3", "cwids.unknown3", FT_BYTES, BASE_NONE, NULL,
			0x0, "3rd Unknown block", HFILL }},

	};
	static gint *ett[] = {
		&ett_cwids,
	};

	static ei_register_info ei[] = {
		{ &ie_ieee80211_subpacket, { "cwids.ieee80211_malformed", PI_MALFORMED, PI_ERROR, "Malformed or short IEEE80211 subpacket", EXPFILL }},
	};

	module_t *cwids_module;
	expert_module_t* expert_cwids;

	proto_cwids = proto_register_protocol("Cisco Wireless IDS Captures", "CWIDS", "cwids");
	proto_register_field_array(proto_cwids, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
	expert_cwids = expert_register_protocol(proto_cwids);
	expert_register_field_array(expert_cwids, ei, array_length(ei));

	cwids_module = prefs_register_protocol(proto_cwids, proto_reg_handoff_cwids);
	prefs_register_uint_preference(cwids_module, "udp.port",
		"CWIDS port",
		"Set the destination UDP port Cisco wireless IDS messages",
		10, &global_udp_port);

}

void
proto_reg_handoff_cwids(void)
{
	static dissector_handle_t cwids_handle;
	static guint saved_udp_port;
	static gboolean initialized = FALSE;

	if (!initialized) {
		cwids_handle = create_dissector_handle(dissect_cwids, proto_cwids);
		dissector_add_for_decode_as("udp.port", cwids_handle);
		ieee80211_radio_handle = find_dissector_add_dependency("wlan_noqos_radio", proto_cwids);
		initialized = TRUE;
	} else {
		if (saved_udp_port != 0) {
			dissector_delete_uint("udp.port", saved_udp_port, cwids_handle);
		}
	}
	if (global_udp_port != 0) {
		dissector_add_uint("udp.port", global_udp_port, cwids_handle);
	}
	saved_udp_port = global_udp_port;
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 noexpandtab:
 * :indentSize=8:tabSize=8:noTabs=false:
 */
