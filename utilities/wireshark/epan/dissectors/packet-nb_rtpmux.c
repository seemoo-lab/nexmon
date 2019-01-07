/* packet-nb_rtpmux.c
 * Routines for 3GPP RTP Multiplex dissection, 3GPP TS 29.414
 * Copyright 2009, ip.access ltd <amp@ipaccess.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include <epan/packet.h>
#include <epan/prefs.h>

void proto_register_nb_rtpmux(void);
void proto_reg_handoff_nb_rtpmux(void);

/* Initialize the protocol and registered fields */
static int proto_nb_rtpmux = -1;
static int hf_nb_rtpmux_compressed = -1;
static int hf_nb_rtpmux_dstport    = -1;
static int hf_nb_rtpmux_length     = -1;
static int hf_nb_r_bit             = -1;
static int hf_nb_rtpmux_srcport    = -1;
static int hf_nb_rtpmux_data       = -1;
static int hf_nb_rtpmux_cmp_rtp_sequence_no   = -1;
static int hf_nb_rtpmux_cmp_rtp_timestamp     = -1;
static int hf_nb_rtpmux_cmp_rtp_data          = -1;

/* Initialize the subtree pointers */
static gint ett_nb_rtpmux = -1;
static gint ett_nb_rtpmux_cmp_rtp_hdr = -1;

static dissector_handle_t rtpdissector;

/* There appears not to be a standard port or range of ports that can be used here. */
/* For 3G, could potentially get it from HNB Register Accept... */
#define UDP_PORT_NB_RTPMUX_RANGE    "0"

/* Preference settings */
static range_t *global_nb_rtpmux_port_range;

static int
dissect_nb_rtpmux(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    proto_item *ti;
    proto_tree *nb_rtpmux_tree, *nb_rtpmux_cmp_rtp_tree;
    unsigned int offset = 0;
    gboolean first_rtp_payload_seen = FALSE;

    /*  First, if at all possible, do some heuristics to check if the packet cannot
     *  possibly belong to your protocol.  This is especially important for
     *  protocols directly on top of TCP or UDP where port collisions are
     *  common place (e.g., even though your protocol uses a well known port,
     *  someone else may set up, for example, a web server on that port which,
     *  if someone analyzed that web server's traffic in Wireshark, would result
     *  in Wireshark handing an HTTP packet to your dissector).  For example:
     */

    /*
     * XXX - this is *FAR* too weak a heuristic; it could cause all sorts
     * of stuff to be incorrectly identified as Nb_RTPmux.  Either this
     * needs a stronger heuristic, or it needs to have a preference to
     * set the port on which to dissect it, or it needs to be a non-heuristic
     * dissector and *require* that a user use "Decode As..." to decode
     * traffic as Nb_RTPmux.
     *
     * Look for a payload that looks like an RTP packet, using the
     * same (weakish) heuristics as RTP uses?
     */

    /* Check that there's enough data */
    if (tvb_captured_length(tvb) < 6)
        return 0;

    /* Make entries in Protocol column and Info column on summary display */
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "NB_RTPMUX");

    /* NOTE: The offset and length values in the call to
       "proto_tree_add_item()" define what data bytes to highlight in the hex
       display window when the line in the protocol tree display
       corresponding to that item is selected.

       Supplying a length of -1 is the way to highlight all data from the
       offset to the end of the packet. */

    /* create display subtree for the protocol */
    while (offset < tvb_reported_length(tvb)-5)
    {
        guint16 dstport, srcport;
        unsigned int length;
        gint captured_length;
        tvbuff_t *next_tvb;
        gboolean tbit;

        length = tvb_get_guint8(tvb, offset+2);
        ti = proto_tree_add_item(tree, proto_nb_rtpmux, tvb, offset, length+5, ENC_NA);
        nb_rtpmux_tree = proto_item_add_subtree(ti, ett_nb_rtpmux);

        /* T bit */
        proto_tree_add_item(nb_rtpmux_tree, hf_nb_rtpmux_compressed, tvb, offset, 2, ENC_BIG_ENDIAN);
        tbit = tvb_get_guint8(tvb,offset)>>7;
        if(tbit == 1){
            /* 6.4.2.4 Transport Format for multiplexing with RTP header compression */
            dstport = (tvb_get_ntohs(tvb, offset) & 0x7fff) << 1;
            proto_tree_add_uint(nb_rtpmux_tree, hf_nb_rtpmux_dstport, tvb, offset, 2, dstport );
            proto_tree_add_item(nb_rtpmux_tree, hf_nb_rtpmux_length, tvb, offset+2, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(nb_rtpmux_tree, hf_nb_r_bit, tvb, offset+3, 2, ENC_BIG_ENDIAN);
            srcport = (tvb_get_ntohs(tvb, offset+3) & 0x7fff) << 1;
            proto_tree_add_uint(nb_rtpmux_tree, hf_nb_rtpmux_srcport, tvb, offset+3, 2, srcport );
            nb_rtpmux_cmp_rtp_tree = proto_tree_add_subtree( nb_rtpmux_tree, tvb, offset+5, 3, ett_nb_rtpmux_cmp_rtp_hdr, NULL, "Compressed RTP header" );
            /* Sequence Number (SN) */
            proto_tree_add_item(nb_rtpmux_cmp_rtp_tree, hf_nb_rtpmux_cmp_rtp_sequence_no, tvb, offset+5, 1, ENC_BIG_ENDIAN);
            /* Timestamp (TS) */
            proto_tree_add_item(nb_rtpmux_cmp_rtp_tree, hf_nb_rtpmux_cmp_rtp_timestamp, tvb, offset+6, 2, ENC_BIG_ENDIAN);
            if (length != 0)
                proto_tree_add_item(nb_rtpmux_cmp_rtp_tree, hf_nb_rtpmux_cmp_rtp_data,tvb, offset+8, length-3, ENC_NA);

            /* Not trying to decompress... */

            /* Add summary to protocol root */
            proto_item_append_text(ti, ", Src Port: %u, Dst Port: %u Length: %u", srcport, dstport, length);

        }else{
            /* 6.4.2.3 Transport Format for multiplexing without RTP Header Compression */
            dstport = (tvb_get_ntohs(tvb, offset) & 0x7fff) << 1;
            proto_tree_add_uint(nb_rtpmux_tree, hf_nb_rtpmux_dstport, tvb, offset, 2, dstport );
            proto_tree_add_item(nb_rtpmux_tree,
                                hf_nb_rtpmux_length, tvb, offset+2, 1, ENC_BIG_ENDIAN);
            proto_tree_add_item(nb_rtpmux_tree, hf_nb_r_bit, tvb, offset+3, 1, ENC_BIG_ENDIAN);
            srcport = (tvb_get_ntohs(tvb, offset+3) & 0x7fff) << 1;
            proto_tree_add_uint(nb_rtpmux_tree, hf_nb_rtpmux_srcport, tvb, offset+3, 2, srcport );

            /* Add summary to protocol root */
            proto_item_append_text(ti, ", Src Port: %u, Dst Port: %u Length: %u", srcport, dstport, length);

            if (length != 0)
            {
                /* We have an RTP payload. */
                if (rtpdissector)
                {
                    captured_length = tvb_reported_length_remaining(tvb, offset + 5);
                    if (captured_length > (gint)length)
                        captured_length = length;
                    next_tvb = tvb_new_subset(tvb, offset+5, captured_length,
                                              length);

                    if (first_rtp_payload_seen)
                    {
                        /* Don't want to clear the column, instead show where multiple
                           RTP frames are being carried */
                        col_append_str(pinfo->cinfo, COL_INFO, "  | ");
                        col_set_fence(pinfo->cinfo, COL_INFO);
                    }

                    call_dissector(rtpdissector, next_tvb, pinfo, nb_rtpmux_tree);

                    first_rtp_payload_seen = TRUE;
                }
                else
                {
                    proto_tree_add_item(nb_rtpmux_tree,
                                        hf_nb_rtpmux_data, tvb, offset+5, length, ENC_NA);
                }
            }
        } /* if tbit */
        offset += 5+length;
    }

    /* Return the amount of data this dissector was able to dissect */
    return tvb_reported_length(tvb);
}


/* Register the protocol with Wireshark */

void
proto_register_nb_rtpmux(void)
{

    static hf_register_info hf[] = {
        { &hf_nb_rtpmux_compressed,
          { "Compressed headers(T bit)", "nb_rtpmux.compressed",
            FT_BOOLEAN, 16, NULL, 0x8000,
            NULL, HFILL }
        },
        { &hf_nb_rtpmux_dstport,
          { "Dst port", "nb_rtpmux.dstport",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nb_rtpmux_length,
          { "Length", "nb_rtpmux.length",
            FT_UINT8, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_nb_r_bit,
          { "R bit", "nb_rtpmux.r_bit",
            FT_BOOLEAN, 16, NULL, 0x8000,
            NULL, HFILL }
        },
        { &hf_nb_rtpmux_srcport,
          { "Src port", "nb_rtpmux.srcport",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nb_rtpmux_data,
          { "RTP Packet", "nb_rtpmux.data",
            FT_BYTES, BASE_NONE, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_nb_rtpmux_cmp_rtp_sequence_no,
          { "Sequence Number", "nb_rtpmux.cmp_rtp.sequence_no",
            FT_UINT16, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_nb_rtpmux_cmp_rtp_timestamp,
          { "Timestamp", "nb_rtpmux.cmp_rtp.timestamp",
            FT_UINT16, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_nb_rtpmux_cmp_rtp_data,
          { "RTP Data", "nb_rtpmux.cmp_rtp.data",
            FT_BYTES, BASE_NONE, NULL, 0x00,
            NULL,HFILL }
        }

    };

    /* Setup protocol subtree array */
    static gint *ett[] = {
        &ett_nb_rtpmux,
        &ett_nb_rtpmux_cmp_rtp_hdr
    };

    module_t *nb_rtpmux_module;

    /* Register the protocol name and description */
    proto_nb_rtpmux = proto_register_protocol("3GPP Nb Interface RTP Multiplex",
        "NB_RTPMUX", "nb_rtpmux");

    /* Required function calls to register the header fields and subtrees used */
    proto_register_field_array(proto_nb_rtpmux, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

    /* Set default UDP ports */
    range_convert_str(&global_nb_rtpmux_port_range, UDP_PORT_NB_RTPMUX_RANGE, MAX_UDP_PORT);

    nb_rtpmux_module = prefs_register_protocol(proto_nb_rtpmux, proto_reg_handoff_nb_rtpmux);
    prefs_register_range_preference(nb_rtpmux_module, "udp_ports",
                                    "NB RTPMUX server port numbers",
                                    "UDP port numbers used for NB RTPMUX traffic "
                                    "(default " UDP_PORT_NB_RTPMUX_RANGE ")",
                                    &global_nb_rtpmux_port_range, MAX_UDP_PORT);
}

void
proto_reg_handoff_nb_rtpmux(void)
{
    static range_t *nb_rtpmux_port_range;
    static gboolean nb_rtpmux_initialized = FALSE;
    dissector_handle_t nb_rtpmux_handle;

    /*  Use create_dissector_handle() to indicate that dissect_nb_rtpmux()
     *  returns the number of bytes it dissected (or 0 if it thinks the packet
     *  does not belong to PROTONAME).
     */
    nb_rtpmux_handle = create_dissector_handle(dissect_nb_rtpmux,
                                                   proto_nb_rtpmux);

    if (!nb_rtpmux_initialized)
    {
        nb_rtpmux_initialized = TRUE;
    }
    else {
        /* Remove and delete previous UDP port range */
        dissector_delete_uint_range("udp.port", nb_rtpmux_port_range, nb_rtpmux_handle);
        g_free(nb_rtpmux_port_range);
    }

    /* Set range of ports from preference to use this dissector */
    nb_rtpmux_port_range = range_copy(global_nb_rtpmux_port_range);
    dissector_add_uint_range("udp.port", nb_rtpmux_port_range, nb_rtpmux_handle);

    /* Allow 'decode-as' for UDP ports */
    dissector_add_for_decode_as("udp.port", nb_rtpmux_handle);
    rtpdissector = find_dissector_add_dependency("rtp", proto_nb_rtpmux);
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
