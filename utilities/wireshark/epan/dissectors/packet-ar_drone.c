/* packet-ar_drone.c
 * Routines for AR ar_drone protocol packet disassembly
 * By Paul Hoisington <hoisingtonp@bit-sys.com>,
 * Tom Hildesheim <hildesheimt@bit-sys.com>,
 * and Claire Brantley <brantleyc@bit-sys.com>
 * Copyright 2012 BIT Systems
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

#include "config.h"

#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/expert.h>

void proto_register_ar_drone(void);
void proto_reg_handoff_ar_drone(void);

static guint ar_drone_port = 0;

/* ************************************************ */
/* Begin static variable declaration/initialization */
/* ************************************************ */

/* ar_drone Protocol */
static int proto_ar_drone = -1;

/* Headers */
static int hf_command = -1;
static int hf_PCMD_id = -1;
static int hf_PCMD_flag = -1;
static int hf_PCMD_roll = -1;
static int hf_PCMD_pitch = -1;
static int hf_PCMD_gaz = -1;
static int hf_PCMD_yaw = -1;
static int hf_REF_id = -1;
static int hf_REF_ctrl = -1;
static int hf_FTRIM_seq = -1;
static int hf_CONFIG_seq = -1;
static int hf_CONFIG_name = -1;
static int hf_CONFIG_val = -1;
static int hf_CONFIG_ID_seq = -1;
static int hf_CONFIG_ID_session = -1;
static int hf_CONFIG_ID_user = -1;
static int hf_CONFIG_ID_app = -1;
static int hf_COMWDG = -1;
static int hf_LED_seq = -1;
static int hf_LED_anim = -1;
static int hf_LED_freq = -1;
static int hf_LED_sec = -1;
static int hf_ANIM_seq = -1;
static int hf_ANIM_anim = -1;
static int hf_ANIM_sec = -1;
static int hf_CTRL_seq = -1;
static int hf_CTRL_mode = -1;
static int hf_CTRL_fsize = -1;

/**Subtrees */
static gint ett_FTRIM = -1;
static gint ett_ar_drone = -1;
static gint ett_PCMD = -1;
static gint ett_REF = -1;
static gint ett_CONFIG = -1;
static gint ett_CONFIG_ID = -1;
static gint ett_COMWDG = -1;
static gint ett_LED = -1;
static gint ett_ANIM = -1;
static gint ett_CTRL = -1;

static expert_field ei_NO_COMMA = EI_INIT;
static expert_field ei_NO_CR = EI_INIT;

/* Value String */
#if 0 /* TODO: Delete these?  Or make use of them? */
static const value_string REF_types_vs[] = {
    { 0x38323038, "FLYING MODE" },
    { 0x37393532, "EMERGENCY LANDING" },
    { 0x37363936, "LANDING MODE" },
    { 0, NULL }
};
static const value_string PCMD_flag_vs[] = {
    { 0x30 , "DO NOT ALLOW ROLL/PITCH" },
    { 0x31 , "ALLOW ROLL/PITCH" },
    { 0 , NULL }
};
#endif

static const string_string CTRL_mode_vs[] = {
    { "4" , " (CFG_GET_CONTROL_MODE)" },
    { "5" , " (ACK_CONTROL_MODE)" },
    { "6" , " (CUSTOM_CFG_GET_CONTROL_MODE)" },
    { 0, NULL }
};

/* ********************************************** */
/* End static variable declaration/initialization */
/* ********************************************** */
static int
dissect_ar_drone(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    gint        offset, length;
    gint        master_offset = 0;
    proto_item *ti, *sub_item;
    proto_tree *ar_tree, *sub_tree;
    char       *command;
    guint32     dword;

    if (tvb_captured_length(tvb) < 4)
        return 0;

    /* Make sure the packet we're dissecting is a ar_drone packet
     *  Cheap string check for 'AT*'
     */
    dword = tvb_get_ntoh24(tvb, 0);
    if (dword != 0x41542a)
        return 0;

    col_set_str(pinfo->cinfo, COL_PROTOCOL, "ar_drone");
    col_set_str(pinfo->cinfo, COL_INFO, "AR Drone Packet");

    /* Initialize ar_drone Packet tree with subtrees */
    ti = proto_tree_add_item(tree, proto_ar_drone, tvb, 0, -1, ENC_NA);
    ar_tree = proto_item_add_subtree(ti, ett_ar_drone);

    while (tvb_reported_length_remaining(tvb, master_offset) > 3)
    {
        /* Get a string to compare our command strings (aka "AT*PCMD", etc.) to */
        offset = tvb_find_guint8(tvb, master_offset, -1, '=');
        if (offset < master_offset)
            return master_offset;

        command = tvb_get_string_enc(wmem_packet_scope(), tvb, master_offset, offset-master_offset, ENC_ASCII|ENC_NA);
        sub_item = proto_tree_add_string(ar_tree, hf_command, tvb, master_offset, -1,
            tvb_get_string_enc(wmem_packet_scope(), tvb, master_offset+3, offset-master_offset-3, ENC_ASCII|ENC_NA));

        if (!strncmp(command, "AT*PCMD", 7))
        {
            /** Parse according the PCMD layout: */
            guint8      PCMD_byte;
            const char *PCMD_str;

            sub_tree = proto_item_add_subtree(sub_item, ett_PCMD);

            offset = master_offset + 8;

            /* Add PCMD ID */
            length = tvb_find_guint8(tvb, offset, -1, ',') - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_COMMA);
                return offset;
            }
            proto_tree_add_item(sub_tree, hf_PCMD_id, tvb, offset, length, ENC_ASCII|ENC_NA);
            offset += (length + 1);

            /* Add PCMD Flag */
            length = tvb_find_guint8(tvb, offset, -1, ',') - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_COMMA);
                return offset;
            }
            proto_tree_add_item(sub_tree, hf_PCMD_flag, tvb, offset, length, ENC_ASCII|ENC_NA);
            offset += (length + 1);

            /* Add PCMD Roll */
            length = tvb_find_guint8(tvb, offset, -1, ',') - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_COMMA);
                return offset;
            }
            ti = proto_tree_add_item(sub_tree, hf_PCMD_roll, tvb, offset, length, ENC_ASCII|ENC_NA);

            PCMD_byte = tvb_get_guint8(tvb, offset);
            if (PCMD_byte == 0x30)
            {
                PCMD_str = " (NO CHANGE)";
            }
            else if (PCMD_byte == 0x2d)
            {
                PCMD_byte = tvb_get_guint8(tvb, offset + 1);
                if (PCMD_byte == 0x30)
                {
                    PCMD_str = " (NO CHANGE)";
                }
                else
                {
                    PCMD_str = " (ROLL LEFT)";
                }
            }
            else
            {
                PCMD_str = " (ROLL RIGHT)";
            }
            proto_item_append_text(ti, "%s", PCMD_str);
            offset += (length + 1);

            /* Add PCMD Pitch */
            length = tvb_find_guint8(tvb, offset, -1, ',') - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_COMMA);
                return offset;
            }
            ti = proto_tree_add_item(sub_tree, hf_PCMD_pitch, tvb, offset, length, ENC_ASCII|ENC_NA);

            PCMD_byte = tvb_get_guint8(tvb, offset);
            if (PCMD_byte == 0x30)
            {
                PCMD_str = " (NO CHANGE)";
            }
            else if (PCMD_byte == 0x2d)
            {
                PCMD_byte = tvb_get_guint8(tvb, offset + 1);
                if (PCMD_byte == 0x30)
                {
                    PCMD_str = " (NO CHANGE)";
                }
                else
                {
                    PCMD_str = " (PITCH FORWARD)";
                }
            }
            else
            {
                PCMD_str = " (PITCH BACKWARD)";
            }
            proto_item_append_text(ti, "%s", PCMD_str);
            offset += (length + 1);

            /* Add PCMD Gaz */
            length = tvb_find_guint8(tvb, offset, -1, ',') - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_COMMA);
                return offset;
            }
            ti = proto_tree_add_item(sub_tree, hf_PCMD_gaz, tvb, offset, length, ENC_ASCII|ENC_NA);

            PCMD_byte = tvb_get_guint8(tvb, offset);
            if (PCMD_byte == 0x30)
            {
                PCMD_str = " (NO CHANGE)";
            }
            else if (PCMD_byte == 0x2d)
            {
                PCMD_byte = tvb_get_guint8(tvb, offset + 1);
                if (PCMD_byte == 0x30)
                {
                    PCMD_str = " (NO CHANGE)";
                }
                else
                {
                    PCMD_str = " (DECREASE VERT SPEED)";
                }
            }
            else
            {
                PCMD_str = " (INCREASE VERT SPEED)";
            }
            proto_item_append_text(ti, "%s", PCMD_str);
            offset += (length + 1);

            /* Add PCMD Yaw */
            length = tvb_find_guint8(tvb, offset, -1, 0x0d) - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_CR);
                return offset;
            }
            ti = proto_tree_add_item(sub_tree, hf_PCMD_yaw, tvb, offset, length, ENC_ASCII|ENC_NA);

            PCMD_byte = tvb_get_guint8(tvb, offset);
            if (PCMD_byte == 0x30)
            {
                PCMD_str = " (NO CHANGE)";
            }
            else if (PCMD_byte == 0x2d)
            {
                PCMD_byte = tvb_get_guint8(tvb, offset + 1);
                if (PCMD_byte == 0x30)
                {
                    PCMD_str = " (NO CHANGE)";
                }
                else
                {
                    PCMD_str = " (ROTATE LEFT)";
                }
            }
            else
            {
                PCMD_str = " (ROTATE RIGHT)";
            }
            proto_item_append_text(ti, "%s", PCMD_str);
            offset += (length + 1);
        }
        else if (!strncmp(command, "AT*REF", 6))
        {
            /** Parse according to the REF layout: */
            sub_tree = proto_item_add_subtree(sub_item, ett_REF);

            offset = master_offset + 7;

            /* Add REF ID */
            length = tvb_find_guint8(tvb, offset, -1, ',') - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_COMMA);
                return offset;
            }
            proto_tree_add_item(sub_tree, hf_REF_id, tvb, offset, length, ENC_ASCII|ENC_NA);
            offset += (length + 1);

            /* Add REF ctrl */
            length = tvb_find_guint8(tvb, offset, -1, 0x0d) - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_CR);
                return offset;
            }
            proto_tree_add_item(sub_tree, hf_REF_ctrl, tvb, offset, length, ENC_ASCII|ENC_NA);
            offset += (length + 1);

        } else if (!strncmp(command, "AT*CONFIG_IDS", 13))
        {
            /** Parse according to the CONFIG_ID layout:  */
            sub_tree = proto_item_add_subtree(sub_item, ett_CONFIG_ID);

            offset = master_offset + 14;

            /* Add Sequence Number */
            length = tvb_find_guint8(tvb, offset, -1, ',') - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_COMMA);
                return offset;
            }
            proto_tree_add_item(sub_tree, hf_CONFIG_ID_seq, tvb, offset, length, ENC_ASCII|ENC_NA);
            offset += (length + 1);

            /* Add Session ID */
            length = tvb_find_guint8(tvb, offset, -1, ',') - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_COMMA);
                return offset;
            }
            proto_tree_add_item(sub_tree, hf_CONFIG_ID_session, tvb, offset, length, ENC_ASCII|ENC_NA);
            offset += (length + 1);

            /* Add User ID */
            length = tvb_find_guint8(tvb, offset, -1, ',') - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_COMMA);
                return offset;
            }
            proto_tree_add_item(sub_tree, hf_CONFIG_ID_user, tvb, offset, length, ENC_ASCII|ENC_NA);
            offset += (length + 1);

            /* Add Application ID */
            length = tvb_find_guint8(tvb, offset, -1, 0x0d) - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_CR);
                return offset;
            }
            proto_tree_add_item(sub_tree, hf_CONFIG_ID_app, tvb, offset, length, ENC_ASCII|ENC_NA);
            offset += (length + 1);

        } else if (!strncmp(command, "AT*ANIM", 7))
        {
            /** Parse according to the ANIM layout: */
            sub_tree = proto_item_add_subtree(sub_item, ett_ANIM);

            offset = master_offset + 8;

            /* Add sequence */
            length = tvb_find_guint8(tvb, offset, -1, ',') - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_COMMA);
                return offset;
            }
            proto_tree_add_item(sub_tree, hf_ANIM_seq, tvb, offset, length, ENC_ASCII|ENC_NA);
            offset += (length + 1);

            /* Add Animation */
            length = tvb_find_guint8(tvb, offset, -1, ',') - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_COMMA);
                return offset;
            }
            proto_tree_add_item(sub_tree, hf_ANIM_anim, tvb, offset, length, ENC_ASCII|ENC_NA);
            offset += (length + 1);

            /* Add animation time(sec) */
            length = tvb_find_guint8(tvb, offset, -1, 0x0d) - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_CR);
                return offset;
            }
            proto_tree_add_item(sub_tree, hf_ANIM_sec, tvb, offset, length, ENC_ASCII|ENC_NA);
            offset += (length + 1);

        } else if (!strncmp(command, "AT*FTRIM", 8))
        {
            /** Parse according to the FTRIM layout: */
            sub_tree = proto_item_add_subtree(sub_item, ett_FTRIM);

            offset = master_offset + 9;

            /* Add sequence number */
            length = tvb_find_guint8(tvb, offset, -1, 0x0d) - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_CR);
                return offset;
            }
            proto_item_append_text(sub_item, " (Sets the reference for the horizontal plane)");
            proto_tree_add_item(sub_tree, hf_FTRIM_seq, tvb, offset, length, ENC_ASCII|ENC_NA);
            offset += (length + 1);
        } else if (!strncmp(command, "AT*CONFIG", 9))
        {
            /** Parse according to the CONFIG layout: */
            sub_tree = proto_item_add_subtree(sub_item, ett_CONFIG);

            offset = master_offset + 10;

            /* Add sequence */
            length = tvb_find_guint8(tvb, offset, -1, ',') - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_COMMA);
                return offset;
            }
            proto_tree_add_item(sub_tree, hf_CONFIG_seq, tvb, offset, length, ENC_ASCII|ENC_NA);
            offset += (length + 1);

            /* Add Name */
            length = tvb_find_guint8(tvb, offset, -1, ',') - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_COMMA);
                return offset;
            }
            proto_tree_add_item(sub_tree, hf_CONFIG_name, tvb, offset, length, ENC_ASCII|ENC_NA);
            offset += (length + 1);

            /* Add Value */
            length = tvb_find_guint8(tvb, offset, -1, 0x0d) - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_CR);
                return offset;
            }
            proto_tree_add_item(sub_tree, hf_CONFIG_val, tvb, offset, length, ENC_ASCII|ENC_NA);
            offset += (length + 1);

        } else if (!strncmp(command, "AT*LED", 6))
        {
            /** Parse according to the LED layout: */
            sub_tree = proto_item_add_subtree(sub_item, ett_LED);

            offset = master_offset + 7;

            /* Add sequence */
            length = tvb_find_guint8(tvb, offset, -1, ',') - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_COMMA);
                return offset;
            }
            proto_tree_add_item(sub_tree, hf_LED_seq, tvb, offset, length, ENC_ASCII|ENC_NA);
            offset += (length + 1);

            /* Add animation to play */
            length = tvb_find_guint8(tvb, offset, -1, ',') - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_COMMA);
                return offset;
            }
            proto_tree_add_item(sub_tree, hf_LED_anim, tvb, offset, length, ENC_ASCII|ENC_NA);
            offset += (length + 1);

            /* Add frequency */
            length = tvb_find_guint8(tvb, offset, -1, ',') - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_COMMA);
                return offset;
            }
            proto_tree_add_item(sub_tree, hf_LED_freq, tvb, offset, length, ENC_ASCII|ENC_NA);
            offset += (length + 1);

            /* Add Time to play in sec  */
            length = tvb_find_guint8(tvb, offset, -1, 0x0d) - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_CR);
                return offset;
            }
            proto_tree_add_item(sub_tree, hf_LED_sec, tvb, offset, length, ENC_ASCII|ENC_NA);
            offset += (length + 1);

        } else if (!strncmp(command, "AT*COMWDG", 9))
        {
            /** Parse according to the COMWDG layout: */
            sub_tree = proto_item_add_subtree(sub_item, ett_COMWDG);

            offset = master_offset + 10;

            /* Add sequence number */
            length = tvb_find_guint8(tvb, offset, -1, 0x0d) - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_CR);
                return offset;
            }
            proto_tree_add_item(sub_tree, hf_COMWDG, tvb, offset, length, ENC_ASCII|ENC_NA);
            offset += (length + 1);

        }else if (!strncmp(command, "AT*CTRL", 7))
        {
            const guint8* CTRL_mode_str;

            /** Parse according to the CTRL layout: */
            sub_tree = proto_item_add_subtree(sub_item, ett_CTRL);

            offset = master_offset + 8;

            /* Add sequence */
            length = tvb_find_guint8(tvb, offset, -1, ',') - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_COMMA);
                return offset;
            }
            proto_tree_add_item(sub_tree, hf_CTRL_seq, tvb, offset, length, ENC_ASCII|ENC_NA);
            offset += (length + 1);

            /* Add Mode */
            length = tvb_find_guint8(tvb, offset, -1, ',') - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_COMMA);
                return offset;
            }
            ti = proto_tree_add_item_ret_string(sub_tree, hf_CTRL_mode, tvb, offset, length, ENC_ASCII|ENC_NA, wmem_packet_scope(), &CTRL_mode_str);
            proto_item_append_text(ti, "%s", str_to_str(CTRL_mode_str, CTRL_mode_vs, " (Unknown Mode)"));
            offset += (length + 1);

            /* Add File Size */
            length = tvb_find_guint8(tvb, offset, -1, 0x0d) - offset;
            if (length < 0) {
                expert_add_info(pinfo, sub_item, &ei_NO_CR);
                return offset;
            }
            proto_tree_add_item(sub_tree, hf_CTRL_fsize, tvb, offset, length, ENC_ASCII|ENC_NA);
            offset += (length + 1);
        }
        else
        {
            /* Unknown command, just abort */
            return master_offset;
        }

        proto_item_set_len(sub_item, offset-master_offset);
        master_offset = offset;
    }

    return master_offset;
}

void
proto_register_ar_drone(void)
{
    /* Setup protocol header array */
    static hf_register_info hf[] = {
    { &hf_command,
        { "Command", "ar_drone.command",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        NULL, HFILL }
    },
    { &hf_PCMD_id,
        { "Sequence Number", "ar_drone.pcmd.id",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        "Progressive Command ID", HFILL }
    },
    { &hf_PCMD_flag,
        { "Flag", "ar_drone.pcmd.flag",
        FT_STRING, BASE_NONE,
        NULL/*VALS(PCMD_flag_vs)*/, 0x0,
        "Progressive Command Flag", HFILL }
    },
    { &hf_PCMD_roll,
        { "Roll", "ar_drone.pcmd.roll",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        "Progressive Command Roll", HFILL }
    },
    { &hf_PCMD_pitch,
        { "Pitch", "ar_drone.pcmd.pitch",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        "Progressive Command Pitch", HFILL }
    },
    { &hf_PCMD_gaz,
        { "Gaz", "ar_drone.pcmd.gaz",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        "Progressive Command Gaz", HFILL }
        },
    { &hf_PCMD_yaw,
        { "Yaw", "ar_drone.pcmd.yaw",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        "Progressive Command Yaw", HFILL }
        },
    { &hf_REF_id,
        { "Sequence Number", "ar_drone.ref.id",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        "Reference ID", HFILL }
    },
    { &hf_REF_ctrl,
        { "Control Command", "ar_drone.ref.ctrl",
        FT_STRING, BASE_NONE,
        NULL/*VALS(REF_types_vs)*/, 0x0,
        NULL, HFILL }
    },
    { &hf_FTRIM_seq,
        { "Sequence Number", "ar_drone.ftrim.seq",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        "Flap Trim / Horizontal Plane Reference", HFILL }
    },
    { &hf_CONFIG_ID_seq,
        { "Sequence Number", "ar_drone.configids.seq",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        "Configuration ID sequence number", HFILL }
    },
    { &hf_CONFIG_ID_session,
        { "Current Session ID", "ar_drone.configids.session",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        "Configuration ID current session ID", HFILL }
    },
    { &hf_CONFIG_ID_user,
        { "Current User ID", "ar_drone.configids.user",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        "Configuration ID current user ID", HFILL }
    },
    { &hf_CONFIG_ID_app,
        { "Current Application ID", "ar_drone.configids.app",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        "Configuration ID current application ID", HFILL }
    },
    { &hf_COMWDG,
        { "Command WatchDog Request", "ar_drone.comwdg",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        "Command WatchDog Reset request", HFILL }
    },
    { &hf_CONFIG_seq,
        { "Sequence Number", "ar_drone.config.seq",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        "Configuration Seq Num", HFILL }
    },
    { &hf_CONFIG_name,
        { "Option Name", "ar_drone.config.name",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        NULL, HFILL }
    },
    { &hf_CONFIG_val,
        { "Option Parameter", "ar_drone.config.val",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        NULL, HFILL }
    },
    { &hf_LED_seq,
        { "Sequence Number", "ar_drone.led.seq",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        "LED Sequence Number", HFILL }
    },
    { &hf_LED_anim,
        { "Selected Animation", "ar_drone.led.anim",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        "Selected LED Animation", HFILL }
    },
    { &hf_LED_freq,
        { "Animation Frequency", "ar_drone.led.freq",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        "LED Animation Frequency", HFILL }
    },
    { &hf_LED_sec,
        { "LED Animation Length (Seconds)", "ar_drone.led.sec",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        "LED Anim Length", HFILL }
    },
    { &hf_ANIM_seq,
        { "Animation Sequence Number", "ar_drone.anim.seq",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        "Movment(Animation) Sequence #", HFILL }
    },
    { &hf_ANIM_anim,
        { "Selected Animation Number", "ar_drone.anim.num",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        "Movment(Animation) to Play", HFILL }
    },
    { &hf_ANIM_sec,
        { "Animation Duration (seconds)", "ar_drone.anim.sec",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        "Movment(Animation) Time in Seconds", HFILL }
    },
    { &hf_CTRL_seq,
        { "Sequence Number", "ar_drone.ctrl.seq",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        NULL, HFILL }
    },
    { &hf_CTRL_mode,
        { "Control Mode", "ar_drone.ctrl.mode",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        NULL, HFILL }
    },
    { &hf_CTRL_fsize,
        { "Firmware Update File Size (0 for no update)", "ar_drone.ctrl.filesize",
        FT_STRING, BASE_NONE,
        NULL, 0x0,
        NULL, HFILL }
    }
    };

    /* Setup protocol subtree array */
    static gint *ett[] = {
    &ett_ar_drone,
    &ett_PCMD,
    &ett_REF,
    &ett_FTRIM,
    &ett_CONFIG,
    &ett_CONFIG_ID,
    &ett_COMWDG,
    &ett_LED,
    &ett_ANIM,
    &ett_CTRL
    };

    static ei_register_info ei[] = {
        { &ei_NO_COMMA, { "ar_drone.no_comma", PI_MALFORMED, PI_ERROR, "Comma delimiter not found", EXPFILL }},
        { &ei_NO_CR,    { "ar_drone.no_cr",    PI_MALFORMED, PI_ERROR, "Carriage return delimiter (0x0d) not found", EXPFILL }},
    };

    module_t         *drone_module;
    expert_module_t*  expert_drone;

    /* Setup protocol info */
    proto_ar_drone = proto_register_protocol (
        "AR Drone Packet", /* name       */
        "AR Drone",        /* short name */
        "ar_drone"         /* abbrev     */
    );

    proto_register_field_array(proto_ar_drone, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

    expert_drone = expert_register_protocol(proto_ar_drone);
    expert_register_field_array(expert_drone, ei, array_length(ei));

    drone_module = prefs_register_protocol(proto_ar_drone, NULL);

    prefs_register_uint_preference(drone_module, "udp.port",
                                   "AR Drone UDP Port",
                                   "AR Drone UDP port",
                                   10,
                                   &ar_drone_port);
}

void
proto_reg_handoff_ar_drone(void)
{
    static dissector_handle_t ar_drone_handle;
    static guint old_port = 0;
    static gboolean initialized = FALSE;

    if (initialized == FALSE)
    {
        ar_drone_handle = create_dissector_handle(dissect_ar_drone, proto_ar_drone);

        heur_dissector_add("udp", dissect_ar_drone, "AR Drone over UDP", "ar_drone_udp", proto_ar_drone, HEURISTIC_ENABLE);

        initialized = TRUE;
    }

    /* Register UDP port for dissection */
    if (old_port != 0 && old_port != ar_drone_port)
    {
        dissector_delete_uint("udp.port", old_port, ar_drone_handle);
    }

    if (ar_drone_port != 0 && old_port != ar_drone_port)
    {
        dissector_add_uint("udp.port", ar_drone_port, ar_drone_handle);
    }

    old_port = ar_drone_port;
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
