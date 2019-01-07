/* packet-btavrcp.c
 * Routines for Bluetooth AVRCP dissection
 *
 * Copyright 2012, Michal Labedzki for Tieto Corporation
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
#include <epan/oui.h>
#include <epan/iana_charsets.h>
#include "packet-btl2cap.h"
#include "packet-btsdp.h"
#include "packet-btavctp.h"
#include "packet-btavrcp.h"

static int proto_btavrcp                                                   = -1;

static int hf_btavrcp_rfa                                                  = -1;
static int hf_btavrcp_ctype                                                = -1;
static int hf_btavrcp_subunit_type                                         = -1;
static int hf_btavrcp_subunit_id                                           = -1;
static int hf_btavrcp_opcode                                               = -1;
static int hf_btavrcp_company_id                                           = -1;
static int hf_btavrcp_length                                               = -1;
static int hf_btavrcp_pdu_id                                               = -1;
static int hf_btavrcp_bt_pdu_id                                            = -1;
static int hf_btavrcp_bt_continuing_pdu_id                                 = -1;
static int hf_btavrcp_browsing_pdu_id                                      = -1;
static int hf_btavrcp_reserved                                             = -1;
static int hf_btavrcp_packet_type                                          = -1;
static int hf_btavrcp_passthrough_state                                    = -1;
static int hf_btavrcp_passthrough_operation                                = -1;
static int hf_btavrcp_passthrough_data_length                              = -1;
static int hf_btavrcp_passthrough_company_id                               = -1;
static int hf_btavrcp_passthrough_vendor_unique_id                         = -1;
static int hf_btavrcp_unit_unknown                                         = -1;
static int hf_btavrcp_unit_type                                            = -1;
static int hf_btavrcp_unit_id                                              = -1;
static int hf_btavrcp_subunit_page                                         = -1;
static int hf_btavrcp_subunit_extension_code                               = -1;
static int hf_btavrcp_item                                                 = -1;
static int hf_btavrcp_folder                                               = -1;
static int hf_btavrcp_player_id                                            = -1;
static int hf_btavrcp_status                                               = -1;
static int hf_btavrcp_uid_counter                                          = -1;
static int hf_btavrcp_number_of_items                                      = -1;
static int hf_btavrcp_number_of_items16                                    = -1;
static int hf_btavrcp_character_set                                        = -1;
static int hf_btavrcp_folder_depth                                         = -1;
static int hf_btavrcp_folder_name_length                                   = -1;
static int hf_btavrcp_folder_name                                          = -1;
static int hf_btavrcp_search                                               = -1;
static int hf_btavrcp_search_length                                        = -1;
static int hf_btavrcp_number_of_attributes                                 = -1;
static int hf_btavrcp_uid                                                  = -1;
static int hf_btavrcp_scope                                                = -1;
static int hf_btavrcp_start_item                                           = -1;
static int hf_btavrcp_end_item                                             = -1;
static int hf_btavrcp_direction                                            = -1;
static int hf_btavrcp_identifier                                           = -1;
static int hf_btavrcp_song_length                                          = -1;
static int hf_btavrcp_song_position                                        = -1;
static int hf_btavrcp_play_status                                          = -1;
static int hf_btavrcp_notification_interval                                = -1;
static int hf_btavrcp_event_id                                             = -1;
static int hf_btavrcp_battery_status                                       = -1;
static int hf_btavrcp_number_of_character_set                              = -1;
static int hf_btavrcp_absolute_volume_rfa                                  = -1;
static int hf_btavrcp_absolute_volume                                      = -1;
static int hf_btavrcp_capability                                           = -1;
static int hf_btavrcp_capability_count                                     = -1;
static int hf_btavrcp_item_type                                            = -1;
static int hf_btavrcp_item_length                                          = -1;
static int hf_btavrcp_system_status                                        = -1;
static int hf_btavrcp_number_of_settings                                   = -1;
static int hf_btavrcp_settings_attribute                                   = -1;
static int hf_btavrcp_settings_value                                       = -1;
static int hf_btavrcp_displayable_name                                     = -1;
static int hf_btavrcp_displayable_name_length                              = -1;
static int hf_btavrcp_media_type                                           = -1;
static int hf_btavrcp_folder_type                                          = -1;
static int hf_btavrcp_folder_playable                                      = -1;
static int hf_btavrcp_major_player_type                                    = -1;
static int hf_btavrcp_player_subtype                                       = -1;
static int hf_btavrcp_player_item                                          = -1;
static int hf_btavrcp_attribute                                            = -1;
static int hf_btavrcp_attribute_count                                      = -1;
static int hf_btavrcp_attribute_value                                      = -1;
static int hf_btavrcp_attribute_value_length                               = -1;
static int hf_btavrcp_attribute_item                                       = -1;
static int hf_btavrcp_attribute_list                                       = -1;
static int hf_btavrcp_attribute_entries                                    = -1;
static int hf_btavrcp_attribute_name_length                                = -1;
static int hf_btavrcp_attribute_name                                       = -1;
static int hf_btavrcp_setting_value_length                                 = -1;
static int hf_btavrcp_setting_value                                        = -1;
static int hf_btavrcp_features                                             = -1;
static int hf_btavrcp_not_used_features                                    = -1;
static int hf_btavrcp_feature_reserved_0                                   = -1;
static int hf_btavrcp_feature_reserved_1                                   = -1;
static int hf_btavrcp_feature_reserved_2                                   = -1;
static int hf_btavrcp_feature_reserved_3                                   = -1;
static int hf_btavrcp_feature_reserved_4                                   = -1;
static int hf_btavrcp_feature_reserved_5                                   = -1;
static int hf_btavrcp_feature_reserved_6                                   = -1;
static int hf_btavrcp_feature_reserved_7                                   = -1;
static int hf_btavrcp_feature_passthrough_select                           = -1;
static int hf_btavrcp_feature_passthrough_up                               = -1;
static int hf_btavrcp_feature_passthrough_down                             = -1;
static int hf_btavrcp_feature_passthrough_left                             = -1;
static int hf_btavrcp_feature_passthrough_right                            = -1;
static int hf_btavrcp_feature_passthrough_right_up                         = -1;
static int hf_btavrcp_feature_passthrough_right_down                       = -1;
static int hf_btavrcp_feature_passthrough_left_up                          = -1;
static int hf_btavrcp_feature_passthrough_left_down                        = -1;
static int hf_btavrcp_feature_passthrough_root_menu                        = -1;
static int hf_btavrcp_feature_passthrough_setup_menu                       = -1;
static int hf_btavrcp_feature_passthrough_contents_menu                    = -1;
static int hf_btavrcp_feature_passthrough_favorite_menu                    = -1;
static int hf_btavrcp_feature_passthrough_exit                             = -1;
static int hf_btavrcp_feature_passthrough_0                                = -1;
static int hf_btavrcp_feature_passthrough_1                                = -1;
static int hf_btavrcp_feature_passthrough_2                                = -1;
static int hf_btavrcp_feature_passthrough_3                                = -1;
static int hf_btavrcp_feature_passthrough_4                                = -1;
static int hf_btavrcp_feature_passthrough_5                                = -1;
static int hf_btavrcp_feature_passthrough_6                                = -1;
static int hf_btavrcp_feature_passthrough_7                                = -1;
static int hf_btavrcp_feature_passthrough_8                                = -1;
static int hf_btavrcp_feature_passthrough_9                                = -1;
static int hf_btavrcp_feature_passthrough_dot                              = -1;
static int hf_btavrcp_feature_passthrough_enter                            = -1;
static int hf_btavrcp_feature_passthrough_clear                            = -1;
static int hf_btavrcp_feature_passthrough_channel_up                       = -1;
static int hf_btavrcp_feature_passthrough_channel_down                     = -1;
static int hf_btavrcp_feature_passthrough_previous_channel                 = -1;
static int hf_btavrcp_feature_passthrough_sound_select                     = -1;
static int hf_btavrcp_feature_passthrough_input_select                     = -1;
static int hf_btavrcp_feature_passthrough_display_information              = -1;
static int hf_btavrcp_feature_passthrough_help                             = -1;
static int hf_btavrcp_feature_passthrough_page_up                          = -1;
static int hf_btavrcp_feature_passthrough_page_down                        = -1;
static int hf_btavrcp_feature_passthrough_power                            = -1;
static int hf_btavrcp_feature_passthrough_volume_up                        = -1;
static int hf_btavrcp_feature_passthrough_volume_down                      = -1;
static int hf_btavrcp_feature_passthrough_mute                             = -1;
static int hf_btavrcp_feature_passthrough_play                             = -1;
static int hf_btavrcp_feature_passthrough_stop                             = -1;
static int hf_btavrcp_feature_passthrough_pause                            = -1;
static int hf_btavrcp_feature_passthrough_record                           = -1;
static int hf_btavrcp_feature_passthrough_rewind                           = -1;
static int hf_btavrcp_feature_passthrough_fast_forward                     = -1;
static int hf_btavrcp_feature_passthrough_eject                            = -1;
static int hf_btavrcp_feature_passthrough_forward                          = -1;
static int hf_btavrcp_feature_passthrough_backward                         = -1;
static int hf_btavrcp_feature_passthrough_angle                            = -1;
static int hf_btavrcp_feature_passthrough_subpicture                       = -1;
static int hf_btavrcp_feature_passthrough_f1                               = -1;
static int hf_btavrcp_feature_passthrough_f2                               = -1;
static int hf_btavrcp_feature_passthrough_f3                               = -1;
static int hf_btavrcp_feature_passthrough_f4                               = -1;
static int hf_btavrcp_feature_passthrough_f5                               = -1;
static int hf_btavrcp_feature_vendor_unique                                = -1;
static int hf_btavrcp_feature_basic_group_navigation                       = -1;
static int hf_btavrcp_feature_advanced_control_player                      = -1;
static int hf_btavrcp_feature_browsing                                     = -1;
static int hf_btavrcp_feature_searching                                    = -1;
static int hf_btavrcp_feature_addtonowplayer                               = -1;
static int hf_btavrcp_feature_uid_unique                                   = -1;
static int hf_btavrcp_feature_only_browsable_when_addressed                = -1;
static int hf_btavrcp_feature_only_searchable_when_addressed               = -1;
static int hf_btavrcp_feature_nowplaying                                   = -1;
static int hf_btavrcp_feature_uid_persistency                              = -1;
static int hf_btavrcp_reassembled                                          = -1;
static int hf_btavrcp_currect_path                                         = -1;
static int hf_btavrcp_response_time                                        = -1;
static int hf_btavrcp_command_in_frame                                     = -1;
static int hf_btavrcp_response_in_frame                                    = -1;
static int hf_btavrcp_data                                                 = -1;

static gint ett_btavrcp                                                    = -1;
static gint ett_btavrcp_attribute_list                                     = -1;
static gint ett_btavrcp_attribute_entry                                    = -1;
static gint ett_btavrcp_attribute_entries                                  = -1;
static gint ett_btavrcp_element                                            = -1;
static gint ett_btavrcp_folder                                             = -1;
static gint ett_btavrcp_player                                             = -1;
static gint ett_btavrcp_features                                           = -1;
static gint ett_btavrcp_features_not_used                                  = -1;
static gint ett_btavrcp_path                                               = -1;

static expert_field ei_btavrcp_no_response = EI_INIT;
static expert_field ei_btavrcp_item_length_bad = EI_INIT;
static expert_field ei_btavrcp_unexpected_data = EI_INIT;

static dissector_handle_t btavrcp_handle;

#define OPCODE_VENDOR_DEPENDANT 0x00
#define OPCODE_UNIT             0x30
#define OPCODE_SUBUNIT          0x31
#define OPCODE_PASSTHROUGH      0x7C

#define ITEM_MEDIAPLAYER        0x01
#define ITEM_FOLDER             0x02
#define ITEM_MEDIA_ELEMENT      0x03

#define PACKET_TYPE_SINGLE      0x00
#define PACKET_TYPE_START       0x01
#define PACKET_TYPE_CONTINUE    0x02
#define PACKET_TYPE_END         0x03

#define EVENT_PLAYBACK_STATUS_CHANGED              0x01
#define EVENT_TRACK_CHANGED                        0x02
#define EVENT_TRACK_REACHED_END                    0x03
#define EVENT_TRACK_REACHED_START                  0x04
#define EVENT_PLAYBACK_POSITION_CHANGED            0x05
#define EVENT_BATTERY_STATUS_CHANGED               0x06
#define EVENT_SYSTEM_STATUS_CHANGED                0x07
#define EVENT_PLAYER_APPLICATION_SETTING_CHANGED   0x08
#define EVENT_NOWPLAYING_CONTENT_CHANGED           0x09
#define EVENT_AVAILABLE_PLAYERS_CHANGED            0x0A
#define EVENT_ADDRESSEDPLAYER_CHANGED              0x0B
#define EVENT_UIDS_CHANGED                         0x0C
#define EVENT_VOLUME_CHANGED                       0x0D

#define PDU_GET_CAPABILITIES                                 0x10
#define PDU_LIST_PLAYER_APPLICATION_SETTING_ATTRIBUTES       0x11
#define PDU_LIST_PLAYER_APPLICATION_SETTING_VALUE            0x12
#define PDU_GET_CURRENT_PLAYER_APPLICATION_SETTING_VALUE     0x13
#define PDU_SET_PLAYER_APPLICATION_SETTING_VALUE             0x14
#define PDU_GET_PLAYER_APPLICATION_SETTING_ATTRIBUTE_TEXT    0x15
#define PDU_GET_PLAYER_APPLICATION_SETTING_VALUE_TEXT        0x16
#define PDU_INFORM_DISPLAYABLE_CHARACTER_SET                 0x17
#define PDU_INFORM_BATTERY_STATUS_OF_CT                      0x18
#define PDU_GET_ELEMENT_ATTRIBUTES                           0x20
#define PDU_GET_PLAY_STATUS                                  0x30
#define PDU_REGISTER_NOTIFICATION                            0x31
#define PDU_REQUEST_CONTINUING_RESPONSE                      0x40
#define PDU_ABORT_CONTINUING_RESPONSE                        0x41
#define PDU_SET_ABSOLUTE_VOLUME                              0x50
#define PDU_SET_ADDRESSED_PLAYER                             0x60
#define PDU_SET_BROWSED_PLAYER                               0x70
#define PDU_GET_FOLDER_ITEMS                                 0x71
#define PDU_CHANGE_PATH                                      0x72
#define PDU_GET_ITEM_ATTRIBUTES                              0x73
#define PDU_PLAY_ITEM                                        0x74
#define PDU_SEARCH                                           0x80
#define PDU_ADD_TO_NOW_PLAYING                               0x90
#define PDU_GENERAL_REJECT                                   0xA0

#define STATUS_OK  0x04

static wmem_tree_t *reassembling  = NULL;
static wmem_tree_t *timing        = NULL;
       wmem_tree_t *btavrcp_song_positions = NULL;

typedef struct _avrcp_proto_data_t {
    guint32  interface_id;
    guint32  adapter_id;
    guint32  chandle;
    guint32  channel;
} avrcp_proto_data_t;

typedef struct _fragment {
    guint        start_frame_number;
    guint        end_frame_number;
    guint32      interface_id;
    guint32      adapter_id;
    guint32      chandle;
    guint32      psm;
    guint32      op;
    guint        state;
    guint32      count;
    wmem_tree_t  *fragments;
    } fragment_t;

typedef struct _data_fragment_t {
    guint32   length;
    guint8    *data;
} data_fragment_t;

typedef struct _timing_info {
    guint     command_frame_number;
    nstime_t  command_timestamp;
    guint     response_frame_number;
    nstime_t  response_timestamp;
    guint     max_response_time;
    guint     used;
    guint32   interface_id;
    guint32   adapter_id;
    guint32   chandle;
    guint32   psm;
    guint32   opcode;
    guint32   op;
    guint32   op_arg;
} timing_info_t;

static const value_string packet_type_vals[] = {
    { PACKET_TYPE_SINGLE,     "Single" },
    { PACKET_TYPE_START,      "Start" },
    { PACKET_TYPE_CONTINUE,   "Continue" },
    { PACKET_TYPE_END,        "End" },
    { 0, NULL }
};

static const value_string ctype_vals[] = {
    { 0x00,   "Control" },
    { 0x01,   "Status" },
    { 0x02,   "Specific Inquiry" },
    { 0x03,   "Notify" },
    { 0x04,   "General Inquiry" },
    { 0x05,   "reserved" },
    { 0x06,   "reserved" },
    { 0x07,   "reserved" },
    { 0x08,   "Not Implemented" },
    { 0x09,   "Accepted" },
    { 0x0A,   "Rejected" },
    { 0x0B,   "In Transition" },
    { 0x0C,   "Stable" },
    { 0x0D,   "Changed" },
    { 0x0E,   "reserved" },
    { 0x0F,   "Interim" },
    { 0, NULL }
};

static const value_string opcode_vals[] = {
    { OPCODE_VENDOR_DEPENDANT,   "Vendor dependent" },
    { OPCODE_UNIT,               "Unit Info" },
    { OPCODE_SUBUNIT,            "Subunit Info" },
    { OPCODE_PASSTHROUGH,        "Pass Through" },
    { 0, NULL }
};

static const value_string subunit_type_vals[] = {
    { 0x00,   "Monitor" },
    { 0x01,   "Audio" },
    { 0x02,   "Printer" },
    { 0x03,   "Disc" },
    { 0x04,   "Tape Recorder/Player" },
    { 0x05,   "Tuner" },
    { 0x06,   "CA" },
    { 0x07,   "Camera" },

    { 0x08,   "reserved" },

    { 0x09,   "Panel" },
    { 0x0A,   "Bulletin Board" },
    { 0x0B,   "Camera Storage" },

    { 0x1C,   "Vendor Unique" },
    { 0x1D,   "All subunit types" },
    { 0x1E,   "Subunit_type extended to next byte" },
    { 0x1F,   "Unit" },
    { 0, NULL }
};

static const value_string passthrough_state_vals[] = {
    { 0x00,   "Pushed" },
    { 0x01,   "Released" },
    { 0, NULL }
};

static const value_string passthrough_operation_vals[] = {
    { 0x41,   "VOLUME UP" },
    { 0x42,   "VOLUME DOWN" },
    { 0x43,   "MUTE" },
    { 0x44,   "PLAY" },
    { 0x45,   "STOP" },
    { 0x46,   "PAUSE" },
    { 0x47,   "RECORD" },
    { 0x48,   "REWIND" },
    { 0x49,   "FAST FORWARD" },
    { 0x4A,   "EJECT" },
    { 0x4B,   "FORWARD" },
    { 0x4C,   "BACKWARD" },
    { 0, NULL }
};

static const value_string pdu_id_vals[] = {
    { PDU_GET_CAPABILITIES,                               "GetCapabilities" },
    { PDU_LIST_PLAYER_APPLICATION_SETTING_ATTRIBUTES,     "ListPlayerApplicationSettingAttributes" },
    { PDU_LIST_PLAYER_APPLICATION_SETTING_VALUE,          "ListPlayerApplicationSettingValue" },
    { PDU_GET_CURRENT_PLAYER_APPLICATION_SETTING_VALUE,   "GetCurrentPlayerApplicationSettingValue" },
    { PDU_SET_PLAYER_APPLICATION_SETTING_VALUE,           "SetPlayerApplicationSettingValue" },
    { PDU_GET_PLAYER_APPLICATION_SETTING_ATTRIBUTE_TEXT,  "GetPlayerApplicationSettingAttributeText" },
    { PDU_GET_PLAYER_APPLICATION_SETTING_VALUE_TEXT,      "GetPlayerApplicationSettingValueText" },
    { PDU_INFORM_DISPLAYABLE_CHARACTER_SET,               "InformDisplayableCharacterSet" },
    { PDU_INFORM_BATTERY_STATUS_OF_CT,                    "InformBatteryStatusOfCT" },
    { PDU_GET_ELEMENT_ATTRIBUTES,                         "GetElementAttributes" },
    { PDU_GET_PLAY_STATUS,                                "GetPlayStatus" },
    { PDU_REGISTER_NOTIFICATION,                          "RegisterNotification" },
    { PDU_REQUEST_CONTINUING_RESPONSE,                    "RequestContinuingResponse" },
    { PDU_ABORT_CONTINUING_RESPONSE,                      "AbortContinuingResponse" },
    { PDU_SET_ABSOLUTE_VOLUME,                            "SetAbsoluteVolume" },
    { PDU_SET_ADDRESSED_PLAYER,                           "SetAddressedPlayer" },
    { PDU_PLAY_ITEM,                                      "PlayItem" },
    { PDU_ADD_TO_NOW_PLAYING,                             "AddToNowPlaying" },
    { 0, NULL }
};

static const value_string browsing_pdu_id_vals[] = {
    { PDU_SET_BROWSED_PLAYER,   "SetBrowsedPlayer" },
    { PDU_GET_FOLDER_ITEMS,     "GetFolderItems" },
    { PDU_CHANGE_PATH,          "ChangePath" },
    { PDU_GET_ITEM_ATTRIBUTES,  "GetItemAttributes" },
    { PDU_SEARCH,               "Search" },
    { PDU_GENERAL_REJECT,       "GeneralReject" },
    { 0, NULL }
};

static const value_string status_vals[] = {
    { 0x00,   "Invalid Command" },
    { 0x01,   "Invalid Parameter" },
    { 0x02,   "Parameter Not Found" },
    { 0x03,   "Internal Error" },
    { 0x04,   "OK" },
    { 0x05,   "UID Changed" },
    { 0x06,   "Reserved" },
    { 0x07,   "Invalid Direction" },
    { 0x08,   "Not a Directory" },
    { 0x09,   "Does Not Exist" },
    { 0x0A,   "Invalid Scope" },
    { 0x0B,   "Range Out of Bounds" },
    { 0x0C,   "UID is a Directory" },
    { 0x0D,   "Media In Use" },
    { 0x0E,   "Now Player List Full" },
    { 0x0F,   "Search Not Supported" },
    { 0x10,   "Search in Progress" },
    { 0x11,   "Invalid Player Id" },
    { 0x12,   "Player Not Browsable" },
    { 0x13,   "Player Not Addressed" },
    { 0x14,   "No Valid Search Results" },
    { 0x15,   "No Available Players" },
    { 0x16,   "Addressed Player Changed" },
    { 0, NULL }
};

static const value_string vendor_unique_id_vals[] = {
    { 0x0000,   "Next Group" },
    { 0x0001,   "Previous Group" },
    { 0, NULL }
};

static const value_string direction_vals[] = {
    { 0x00,   "Folder Up" },
    { 0x01,   "Folder Down" },
    { 0, NULL }
};

static const value_string attribute_count_vals[] = {
    { 0x00,   "All attributes are requested" },
    { 0xFF,   "No attributes are requested" },
    { 0, NULL }
};

static const value_string scope_vals[] = {
    { 0x00,   "MediaPlayerList" },
    { 0x01,   "VFS" },
    { 0x02,   "Search" },
    { 0x03,   "NowPlaying" },
    { 0, NULL }
};

static const value_string attribute_id_vals[] = {
    { 0x00,   "Invalid" },
    { 0x01,   "Title" },
    { 0x02,   "Artist" },
    { 0x03,   "Album" },
    { 0x04,   "Media Number" },
    { 0x05,   "Total Number of Media" },
    { 0x06,   "Genre" },
    { 0x07,   "Playing Time" },
    { 0, NULL }
};

static const value_string settings_vals[] = {
    { 0x00,   "Illegal" },
    { 0x01,   "Equalizer" },
    { 0x02,   "Repeat mode" },
    { 0x03,   "Shuffle" },
    { 0x04,   "Scan" },
    { 0, NULL }
};

static const value_string notification_vals[] = {
    { EVENT_PLAYBACK_STATUS_CHANGED,              "PlaybackStatusChanged" },
    { EVENT_TRACK_CHANGED,                        "TrackChanged" },
    { EVENT_TRACK_REACHED_END,                    "TrackReachedEnd" },
    { EVENT_TRACK_REACHED_START,                  "TrackReachedStart" },
    { EVENT_PLAYBACK_POSITION_CHANGED,            "PlaybackPositionChanged" },
    { EVENT_BATTERY_STATUS_CHANGED,               "BatteryStatusChanged" },
    { EVENT_SYSTEM_STATUS_CHANGED,                "SystemStatusChanged" },
    { EVENT_PLAYER_APPLICATION_SETTING_CHANGED,   "PlayerApplicationSettingChanged" },
    { EVENT_NOWPLAYING_CONTENT_CHANGED,           "NowPlayingContentChanged" },
    { EVENT_AVAILABLE_PLAYERS_CHANGED,            "AvailablePlayersChanged" },
    { EVENT_ADDRESSEDPLAYER_CHANGED,              "AddressedPlayerChanged" },
    { EVENT_UIDS_CHANGED,                         "UIDsChanged" },
    { EVENT_VOLUME_CHANGED,                       "VolumeChanged" },
    { 0, NULL }
};

static const value_string play_status_vals[] = {
    { 0x00,   "Stopped" },
    { 0x01,   "Playing" },
    { 0x02,   "Paused" },
    { 0x03,   "Forward Seek" },
    { 0x04,   "Revrse Seek" },
    { 0xFF,   "Error" },
    { 0, NULL }
};

static const value_string battery_status_vals[] = {
    { 0x00,   "Normal" },
    { 0x01,   "Warning" },
    { 0x02,   "Critical" },
    { 0x03,   "External" },
    { 0x04,   "Full Charge" },
    { 0, NULL }
};

static const value_string capability_vals[] = {
    { 0x02,   "Company ID" },
    { 0x03,   "Events Supported" },
    { 0, NULL }
};

static const value_string item_type_vals[] = {
    { 0x01,   "Media Player Item" },
    { 0x02,   "Folder Item" },
    { 0x03,   "Media Element Item" },
    { 0, NULL }
};

static const value_string system_status_vals[] = {
    { 0x00,   "Power On" },
    { 0x01,   "Power Off" },
    { 0x02,   "Unplugged" },
    { 0, NULL }
};

static const value_string media_type_vals[] = {
    { 0x00,   "Audio" },
    { 0x01,   "Video" },
    { 0, NULL }
};

static const value_string folder_type_vals[] = {
    { 0x00,   "Mixed" },
    { 0x01,   "Titles" },
    { 0x02,   "Albums" },
    { 0x03,   "Artists" },
    { 0x04,   "Genres" },
    { 0x05,   "Playlists" },
    { 0x06,   "Years" },
    { 0, NULL }
};

static const value_string folder_playable_vals[] = {
    { 0x00,   "Not Playable" },
    { 0x01,   "Playable" },
    { 0, NULL }
};

static const value_string major_player_type_vals[] = {
    { 0x00,   "Audio" },
    { 0x02,   "Video" },
    { 0x04,   "Broadcasting Audio" },
    { 0x08,   "Broadcasting Video" },
    { 0, NULL }
};

static const value_string player_subtype_vals[] = {
    { 0x00,   "Audio Book" },
    { 0x02,   "Podcast" },
    { 0, NULL }
};

void proto_register_btavrcp(void);
void proto_reg_handoff_btavrcp(void);

static  gint
dissect_attribute_id_list(tvbuff_t *tvb, proto_tree *tree, gint offset,
                          guint count)
{
    guint       i_attribute;
    proto_item *pitem;
    proto_tree *ptree;

    pitem = proto_tree_add_item(tree, hf_btavrcp_attribute_list, tvb, offset, count * 4, ENC_NA);
    ptree = proto_item_add_subtree(pitem, ett_btavrcp_attribute_list);

    for (i_attribute = 0; i_attribute < count; ++i_attribute) {
        proto_tree_add_item(ptree, hf_btavrcp_attribute, tvb, offset, 4, ENC_BIG_ENDIAN);
        offset += 4;
    }

    return offset;
}


static gint
dissect_attribute_entries(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                          gint offset, guint count)
{
    guint       i_entry;
    guint       attribute_id;
    guint       value_length;
    guint       length;
    guint8     *value;
    proto_item *pitem      = NULL;
    proto_tree *ptree      = NULL;
    proto_item *entry_item = NULL;
    proto_tree *entry_tree = NULL;

    length = 0;
    for (i_entry = 0; i_entry < count; ++i_entry) {
        length += 4 + 2 + 2 + tvb_get_ntohs(tvb, offset + length + 4 + 2);
    }

    pitem = proto_tree_add_item(tree, hf_btavrcp_attribute_entries, tvb, offset, length, ENC_NA);
    ptree = proto_item_add_subtree(pitem, ett_btavrcp_attribute_entries);

    for (i_entry = 0; i_entry < count; ++i_entry) {
        attribute_id = tvb_get_ntohl(tvb, offset);
        value_length = tvb_get_ntohs(tvb, offset + 4 + 2);
        value = tvb_get_string_enc(wmem_packet_scope(), tvb, offset + 4 + 2 + 2, value_length, ENC_ASCII);

        if (attribute_id == 0x01) col_append_fstr(pinfo->cinfo, COL_INFO, " - Title: \"%s\"", value);

        entry_item = proto_tree_add_none_format(ptree, hf_btavrcp_attribute_item, tvb, offset, 4 + 2 + 2 + value_length, "Attribute [%21s]: %s", val_to_str_const(attribute_id, attribute_id_vals, "Unknown"), value);
        entry_tree = proto_item_add_subtree(entry_item, ett_btavrcp_attribute_entry);

        proto_tree_add_item(entry_tree, hf_btavrcp_attribute, tvb, offset, 4, ENC_BIG_ENDIAN);
        offset += 4;
        proto_tree_add_item(entry_tree, hf_btavrcp_character_set, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
        proto_tree_add_item(entry_tree, hf_btavrcp_setting_value_length, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
        proto_tree_add_item(entry_tree, hf_btavrcp_setting_value, tvb, offset, value_length, ENC_UTF_8 | ENC_NA);
        offset += value_length;
    }

    return offset;
}


static gint
dissect_item_mediaplayer(tvbuff_t *tvb, proto_tree *tree, gint offset)
{
    guint       displayable_name_length;
    guint       item_length;
    guint       feature_octet;
    guint       i_feature;
    guint8     *displayable_name;
    proto_item *pitem;
    proto_tree *ptree;
    proto_item *features_item;
    proto_tree *features_tree;
    proto_item *features_not_set_item;
    proto_tree *features_not_set_tree;

    item_length = tvb_get_ntohs(tvb, offset + 1);
    displayable_name_length = tvb_get_ntohs(tvb, offset + 1 + 2 + 1 + 1 + 4 + 16 + 1 + 2);
    displayable_name = tvb_get_string_enc(wmem_packet_scope(), tvb, offset + 1 + 2 + 1 + 1 + 4 + 16 + 1 + 2 + 2, displayable_name_length, ENC_ASCII);

    pitem = proto_tree_add_none_format(tree, hf_btavrcp_player_item, tvb, offset, 1 + 2 + item_length, "Player: %s", displayable_name);
    ptree = proto_item_add_subtree(pitem, ett_btavrcp_player);

    proto_tree_add_item(ptree, hf_btavrcp_item_type, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;
    proto_tree_add_item(ptree, hf_btavrcp_item_length, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset += 2;

    proto_tree_add_item(ptree, hf_btavrcp_player_id, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;

    proto_tree_add_item(ptree, hf_btavrcp_major_player_type, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;

    proto_tree_add_item(ptree, hf_btavrcp_player_subtype, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset += 4;

    /* feature bit mask */
    features_item = proto_tree_add_item(ptree, hf_btavrcp_features, tvb, offset, 16, ENC_NA);
    features_tree = proto_item_add_subtree(features_item, ett_btavrcp_features);

    features_not_set_item = proto_tree_add_item(features_tree, hf_btavrcp_not_used_features, tvb, offset, 16, ENC_NA);
    features_not_set_tree = proto_item_add_subtree(features_not_set_item, ett_btavrcp_features_not_used);

    feature_octet = tvb_get_guint8(tvb, offset + 0);
    proto_tree_add_item((feature_octet & (1 << 0)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_select, tvb, offset + 0, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 1)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_up, tvb, offset + 0, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 2)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_down, tvb, offset + 0, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 3)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_left, tvb, offset + 0, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 4)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_right, tvb, offset + 0, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 5)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_right_up, tvb, offset + 0, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 6)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_right_down, tvb, offset + 0, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 7)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_left_up, tvb, offset + 0, 1, ENC_BIG_ENDIAN);

    feature_octet = tvb_get_guint8(tvb, offset + 1);
    proto_tree_add_item((feature_octet & (1 << 0)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_left_down , tvb, offset + 1, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 1)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_root_menu , tvb, offset + 1, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 2)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_setup_menu, tvb, offset + 1, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 3)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_contents_menu, tvb, offset + 1, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 4)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_favorite_menu, tvb, offset + 1, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 5)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_exit, tvb, offset + 1, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 6)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_0, tvb, offset + 1, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 7)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_1, tvb, offset + 1, 1, ENC_BIG_ENDIAN);

    feature_octet = tvb_get_guint8(tvb, offset + 2);
    proto_tree_add_item((feature_octet & (1 << 0)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_2, tvb, offset + 2, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 1)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_3, tvb, offset + 2, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 2)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_4, tvb, offset + 2, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 3)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_5, tvb, offset + 2, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 4)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_6, tvb, offset + 2, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 5)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_7, tvb, offset + 2, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 6)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_8, tvb, offset + 2, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 7)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_9, tvb, offset + 2, 1, ENC_BIG_ENDIAN);

    feature_octet = tvb_get_guint8(tvb, offset + 3);
    proto_tree_add_item((feature_octet & (1 << 0)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_dot, tvb, offset + 3, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 1)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_enter, tvb, offset + 3, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 2)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_clear, tvb, offset + 3, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 3)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_channel_up, tvb, offset + 3, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 4)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_channel_down, tvb, offset + 3, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 5)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_previous_channel, tvb, offset + 3, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 6)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_sound_select, tvb, offset + 3, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 7)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_input_select, tvb, offset + 3, 1, ENC_BIG_ENDIAN);

    feature_octet = tvb_get_guint8(tvb, offset + 4);
    proto_tree_add_item((feature_octet & (1 << 0)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_display_information, tvb, offset + 4, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 1)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_help, tvb, offset + 4, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 2)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_page_up, tvb, offset + 4, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 3)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_page_down, tvb, offset + 4, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 4)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_power, tvb, offset + 4, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 5)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_volume_up, tvb, offset + 4, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 6)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_volume_down, tvb, offset + 4, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 7)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_mute, tvb, offset + 4, 1, ENC_BIG_ENDIAN);

    feature_octet = tvb_get_guint8(tvb, offset + 5);
    proto_tree_add_item((feature_octet & (1 << 0)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_play, tvb, offset + 5, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 1)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_stop, tvb, offset + 5, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 2)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_pause, tvb, offset + 5, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 3)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_record, tvb, offset + 5, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 4)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_rewind, tvb, offset + 5, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 5)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_fast_forward, tvb, offset + 5, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 6)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_eject, tvb, offset + 5, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 7)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_forward, tvb, offset + 5, 1, ENC_BIG_ENDIAN);

    feature_octet = tvb_get_guint8(tvb, offset + 6);
    proto_tree_add_item((feature_octet & (1 << 0)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_backward, tvb, offset + 6, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 1)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_angle, tvb, offset + 6, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 2)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_subpicture, tvb, offset + 6, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 3)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_f1, tvb, offset + 6, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 4)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_f2, tvb, offset + 6, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 5)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_f3, tvb, offset + 6, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 6)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_f4, tvb, offset + 6, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 7)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_passthrough_f5, tvb, offset + 6, 1, ENC_BIG_ENDIAN);

    feature_octet = tvb_get_guint8(tvb, offset + 7);
    proto_tree_add_item((feature_octet & (1 << 0)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_vendor_unique, tvb, offset + 7, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 1)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_basic_group_navigation, tvb, offset + 7, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 2)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_advanced_control_player, tvb, offset + 7, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 3)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_browsing, tvb, offset + 7, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 4)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_searching, tvb, offset + 7, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 5)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_addtonowplayer, tvb, offset + 7, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 6)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_uid_unique, tvb, offset + 7, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 7)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_only_browsable_when_addressed, tvb, offset + 7, 1, ENC_BIG_ENDIAN);

    feature_octet = tvb_get_guint8(tvb, offset + 8);
    proto_tree_add_item((feature_octet & (1 << 0)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_only_searchable_when_addressed, tvb, offset + 8, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 1)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_nowplaying, tvb, offset + 8, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 2)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_uid_persistency, tvb, offset + 8, 1, ENC_BIG_ENDIAN);

    /* reserved */
    proto_tree_add_item((feature_octet & (1 << 3)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_reserved_3, tvb, offset + 8, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 4)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_reserved_4, tvb, offset + 8, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 5)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_reserved_5, tvb, offset + 8, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 6)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_reserved_6, tvb, offset + 8, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item((feature_octet & (1 << 7)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_reserved_7, tvb, offset + 8, 1, ENC_BIG_ENDIAN);

    for (i_feature = 9; i_feature <= 16; ++i_feature) {
    feature_octet = tvb_get_guint8(tvb, offset + i_feature);
        proto_tree_add_item((feature_octet & (1 << 0)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_reserved_0, tvb, offset + i_feature, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item((feature_octet & (1 << 1)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_reserved_1, tvb, offset + i_feature, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item((feature_octet & (1 << 2)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_reserved_2, tvb, offset + i_feature, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item((feature_octet & (1 << 3)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_reserved_3, tvb, offset + i_feature, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item((feature_octet & (1 << 4)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_reserved_4, tvb, offset + i_feature, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item((feature_octet & (1 << 5)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_reserved_5, tvb, offset + i_feature, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item((feature_octet & (1 << 6)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_reserved_6, tvb, offset + i_feature, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item((feature_octet & (1 << 7)) ? features_tree : features_not_set_tree, hf_btavrcp_feature_reserved_7, tvb, offset + i_feature, 1, ENC_BIG_ENDIAN);
    }
    offset += 16;

    proto_tree_add_item(ptree, hf_btavrcp_play_status, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;

    proto_tree_add_item(ptree, hf_btavrcp_character_set, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset += 2;

    proto_tree_add_item(ptree, hf_btavrcp_displayable_name_length, tvb, offset, 2, ENC_BIG_ENDIAN);
    displayable_name_length = tvb_get_ntohs(tvb, offset);
    offset += 2;

    proto_tree_add_item(ptree, hf_btavrcp_displayable_name, tvb, offset, displayable_name_length, ENC_UTF_8 | ENC_NA);
    offset += displayable_name_length;

    return offset;
}


static gint
dissect_item_media_element(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                           gint offset)
{
    guint       number_of_attributes;
    guint       displayable_name_length;
    guint       item_length;
    guint8     *displayable_name;
    proto_item *pitem;
    proto_tree *ptree;
    gint        offset_in;

    item_length = tvb_get_ntohs(tvb, offset + 1);
    displayable_name_length = tvb_get_ntohs(tvb, offset + 1 + 2 + 8 + 1 + 2);
    displayable_name = tvb_get_string_enc(wmem_packet_scope(), tvb, offset + 1 + 2 + 8 + 1 + 2 + 2, displayable_name_length, ENC_ASCII);

    pitem = proto_tree_add_none_format(tree, hf_btavrcp_item , tvb, offset, 1 + 2 + item_length, "Element: %s", displayable_name);
    ptree = proto_item_add_subtree(pitem, ett_btavrcp_element);

    proto_tree_add_item(ptree, hf_btavrcp_item_type, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;
    proto_tree_add_item(ptree, hf_btavrcp_item_length, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset += 2;
    offset_in = offset;

    proto_tree_add_item(ptree, hf_btavrcp_uid, tvb, offset, 8, ENC_BIG_ENDIAN);
    offset += 8;

    proto_tree_add_item(ptree, hf_btavrcp_media_type, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;

    proto_tree_add_item(ptree, hf_btavrcp_character_set, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset += 2;

    proto_tree_add_item(ptree, hf_btavrcp_displayable_name_length, tvb, offset, 2, ENC_BIG_ENDIAN);
    displayable_name_length = tvb_get_ntohs(tvb, offset);
    offset += 2;

    proto_tree_add_item(ptree, hf_btavrcp_displayable_name, tvb, offset, displayable_name_length, ENC_UTF_8 | ENC_NA);
    offset += displayable_name_length;

    proto_tree_add_item(ptree, hf_btavrcp_number_of_attributes, tvb, offset, 1, ENC_BIG_ENDIAN);
    number_of_attributes = tvb_get_guint8(tvb, offset);
    offset += 1;

    offset = dissect_attribute_entries(tvb, pinfo, ptree, offset, number_of_attributes);

    if ( item_length != (guint) offset - offset_in) {
        expert_add_info(pinfo, pitem, &ei_btavrcp_item_length_bad);
    }

    return offset;
}


static gint
dissect_item_folder(tvbuff_t *tvb, proto_tree *tree, gint offset)
{
    guint       displayable_name_length;
    guint       item_length;
    guint8     *displayable_name;
    proto_item *pitem;
    proto_tree *ptree;

    item_length = tvb_get_ntohs(tvb, offset + 1);
    displayable_name_length = tvb_get_ntohs(tvb, offset + 1 + 2 + 8 + 1 + 1 + 2);
    displayable_name = tvb_get_string_enc(wmem_packet_scope(), tvb, offset + 1 + 2 + 8 + 1 + 1 + 2 + 2, displayable_name_length, ENC_ASCII);

    pitem = proto_tree_add_none_format(tree, hf_btavrcp_folder, tvb, offset, 1 + 2 + item_length, "Folder : %s", displayable_name);
    ptree = proto_item_add_subtree(pitem, ett_btavrcp_folder);

    proto_tree_add_item(ptree, hf_btavrcp_item_type, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;
    proto_tree_add_item(ptree, hf_btavrcp_item_length, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset += 2;

    proto_tree_add_item(ptree, hf_btavrcp_uid, tvb, offset, 8, ENC_BIG_ENDIAN);
    offset += 8;

    proto_tree_add_item(ptree, hf_btavrcp_folder_type, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;

    proto_tree_add_item(ptree, hf_btavrcp_folder_playable, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;

    proto_tree_add_item(ptree, hf_btavrcp_character_set, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset += 2;

    proto_tree_add_item(ptree, hf_btavrcp_displayable_name_length, tvb, offset, 2, ENC_BIG_ENDIAN);
    displayable_name_length = tvb_get_ntohs(tvb, offset);
    offset += 2;

    proto_tree_add_item(ptree, hf_btavrcp_displayable_name, tvb, offset, displayable_name_length, ENC_UTF_8 | ENC_NA);
    offset += displayable_name_length;

    return offset;
}


static gint
dissect_passthrough(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                    gint offset, gboolean is_command, guint32 *op)
{
    guint operation;
    guint state;

    proto_tree_add_item(tree, hf_btavrcp_passthrough_state, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_btavrcp_passthrough_operation, tvb, offset, 1, ENC_BIG_ENDIAN);
    *op = tvb_get_guint8(tvb, offset);
    operation = tvb_get_guint8(tvb, offset) & 0x7F;
    state = (tvb_get_guint8(tvb, offset) & 0x80) >> 7;
    offset += 1;
    proto_tree_add_item(tree, hf_btavrcp_passthrough_data_length, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;

    if (!is_command) {
        if (operation == 0x7E) {
            proto_tree_add_item(tree, hf_btavrcp_passthrough_company_id, tvb, offset, 3, ENC_BIG_ENDIAN);
            offset += 3;
            proto_tree_add_item(tree, hf_btavrcp_passthrough_vendor_unique_id, tvb, offset, 2, ENC_BIG_ENDIAN);
            offset += 2;
        }
    }

    col_append_fstr(pinfo->cinfo, COL_INFO, " - %s (%s)",
                    val_to_str_const(operation, passthrough_operation_vals, "Unknown opcode"),
                    val_to_str_const(state, passthrough_state_vals, "unknown"));
    return offset;
}


static gint
dissect_unit(tvbuff_t *tvb, proto_tree *tree, gint offset, gboolean is_command)
{
    if (is_command) {
        proto_tree_add_item(tree, hf_btavrcp_data, tvb, offset, 5, ENC_NA);
        offset += 5;
    } else {
        proto_tree_add_item(tree, hf_btavrcp_unit_unknown, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset += 1;
        proto_tree_add_item(tree, hf_btavrcp_unit_type, tvb, offset, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(tree, hf_btavrcp_unit_id, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset += 1;
        proto_tree_add_item(tree, hf_btavrcp_passthrough_company_id, tvb, offset, 3, ENC_BIG_ENDIAN);
        offset += 3;
    }

    return offset;
}


static gint
dissect_subunit(tvbuff_t *tvb, proto_tree *tree, gint offset, gboolean is_command)
{
    proto_tree_add_item(tree, hf_btavrcp_subunit_page, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_btavrcp_subunit_extension_code, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;

    if (!is_command) {
        proto_tree_add_item(tree, hf_btavrcp_subunit_type, tvb, offset, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(tree, hf_btavrcp_subunit_id, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset += 1;
        proto_tree_add_item(tree, hf_btavrcp_data, tvb, offset, 3, ENC_NA);
        offset += 3;
    } else {
        proto_tree_add_item(tree, hf_btavrcp_data, tvb, offset, 4, ENC_NA);
        offset += 4;
    }

    return offset;
}


static gint
dissect_vendor_dependant(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                         gint offset, guint ctype, guint32 *op, guint32 *op_arg,
                         gboolean is_command, avrcp_proto_data_t *avrcp_proto_data)
{
    proto_item      *pitem;
    guint            pdu_id;
    guint            company_id;
    guint            event_id;
    guint            packet_type;
    guint            parameter_length;
    gint             length;
    wmem_tree_key_t  key[7];
    guint32          k_op;
    guint32          frame_number;
    guint            volume;
    guint            volume_percent;
    fragment_t       *fragment;
    data_fragment_t  *data_fragment;

    *op_arg = 0;

    proto_tree_add_item(tree, hf_btavrcp_company_id, tvb, offset, 3, ENC_BIG_ENDIAN);
    company_id = tvb_get_ntoh24(tvb, offset);
    offset += 3;

    if (company_id == OUI_BLUETOOTH) {
        proto_tree_add_item(tree, hf_btavrcp_bt_pdu_id, tvb, offset, 1, ENC_BIG_ENDIAN);
    } else {

        if (tvb_reported_length_remaining(tvb, offset) == 0) {
            col_append_str(pinfo->cinfo, COL_INFO, " - No PDU ID");
            return offset;
        }

        proto_tree_add_item(tree, hf_btavrcp_pdu_id, tvb, offset, 1, ENC_BIG_ENDIAN);
    }
    pdu_id = tvb_get_guint8(tvb, offset);
    *op = pdu_id | (company_id << 8);
    offset += 1;

    if (company_id != OUI_BLUETOOTH) {
        col_append_fstr(pinfo->cinfo, COL_INFO, " - %s",
                val_to_str_const(pdu_id, NULL, "Unknown PDU ID"));
    }

    proto_tree_add_item(tree, hf_btavrcp_rfa, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_btavrcp_packet_type, tvb, offset, 1, ENC_BIG_ENDIAN);
    packet_type = tvb_get_guint8(tvb, offset) & 0x03;
    offset += 1;

    proto_tree_add_item(tree, hf_btavrcp_length, tvb, offset, 2, ENC_BIG_ENDIAN);
    parameter_length = tvb_get_ntohs(tvb, offset);
    offset += 2;

    if (company_id != OUI_BLUETOOTH)
        return offset;

    col_append_fstr(pinfo->cinfo, COL_INFO, " - %s",
            val_to_str_const(pdu_id, pdu_id_vals, "Unknown PDU ID"));

    if (parameter_length == 0) return offset;

    length = tvb_reported_length_remaining(tvb, offset);
    if (packet_type == PACKET_TYPE_START) {
        if (pinfo->fd->flags.visited == 0 && tvb_captured_length_remaining(tvb, offset) == length) {
            k_op = pdu_id | (company_id << 8);
            frame_number = pinfo->num;

            fragment = wmem_new(wmem_file_scope(), fragment_t);
            fragment->start_frame_number = pinfo->num;
            fragment->end_frame_number = 0;
            fragment->state = 0;

            fragment->count = 1;
            fragment->fragments = wmem_tree_new(wmem_file_scope());

            data_fragment = wmem_new(wmem_file_scope(), data_fragment_t);
            data_fragment->length = length;
            data_fragment->data = (guint8 *) wmem_alloc(wmem_file_scope(), data_fragment->length);
            tvb_memcpy(tvb, data_fragment->data, offset, data_fragment->length);

            wmem_tree_insert32(fragment->fragments, fragment->count, data_fragment);

            key[0].length = 1;
            key[0].key = &avrcp_proto_data->interface_id;
            key[1].length = 1;
            key[1].key = &avrcp_proto_data->adapter_id;
            key[2].length = 1;
            key[2].key = &avrcp_proto_data->chandle;
            key[3].length = 1;
            key[3].key = &avrcp_proto_data->channel;
            key[4].length = 1;
            key[4].key = &k_op;
            key[5].length = 1;
            key[5].key = &frame_number;
            key[6].length = 0;
            key[6].key = NULL;

            fragment->interface_id = avrcp_proto_data->interface_id;
            fragment->adapter_id   = avrcp_proto_data->adapter_id;
            fragment->chandle      = avrcp_proto_data->chandle;
            fragment->psm          = avrcp_proto_data->channel;
            fragment->op           = pdu_id | (company_id << 8);

            wmem_tree_insert32_array(reassembling, key, fragment);
        }

        col_append_str(pinfo->cinfo, COL_INFO, " [start]");
        return offset;
    } else if (packet_type == PACKET_TYPE_CONTINUE) {
        if (pinfo->fd->flags.visited == 0 && tvb_captured_length_remaining(tvb, offset) == length) {
            k_op = pdu_id | (company_id << 8);
            frame_number = pinfo->num;

            key[0].length = 1;
            key[0].key = &avrcp_proto_data->interface_id;
            key[1].length = 1;
            key[1].key = &avrcp_proto_data->adapter_id;
            key[2].length = 1;
            key[2].key = &avrcp_proto_data->chandle;
            key[3].length = 1;
            key[3].key = &avrcp_proto_data->channel;
            key[4].length = 1;
            key[4].key = &k_op;
            key[5].length = 1;
            key[5].key = &frame_number;
            key[6].length = 0;
            key[6].key = NULL;

            fragment = (fragment_t *)wmem_tree_lookup32_array_le(reassembling, key);
            if (fragment && fragment->interface_id == avrcp_proto_data->interface_id &&
                    fragment->adapter_id == avrcp_proto_data->adapter_id &&
                    fragment->chandle == avrcp_proto_data->chandle &&
                    fragment->psm == avrcp_proto_data->channel &&
                    fragment->op == (pdu_id | (company_id << 8)) &&
                    fragment->state == 1) {
                fragment->count += 1;
                fragment->state = 0;

                data_fragment = wmem_new(wmem_file_scope(), data_fragment_t);
                data_fragment->length = length;
                data_fragment->data = (guint8 *) wmem_alloc(wmem_file_scope(), data_fragment->length);
                tvb_memcpy(tvb, data_fragment->data, offset, data_fragment->length);
                wmem_tree_insert32(fragment->fragments, fragment->count, data_fragment);
            }
        }

        col_append_str(pinfo->cinfo, COL_INFO, " [continue]");
        return offset;
    } else if (packet_type == PACKET_TYPE_END) {
        guint            i_frame;
        tvbuff_t         *next_tvb;

        col_append_str(pinfo->cinfo, COL_INFO, " [end]");

        k_op = pdu_id | (company_id << 8);
        frame_number = pinfo->num;

        key[0].length = 1;
        key[0].key = &avrcp_proto_data->interface_id;
        key[1].length = 1;
        key[1].key = &avrcp_proto_data->adapter_id;
        key[2].length = 1;
        key[2].key = &avrcp_proto_data->chandle;
        key[3].length = 1;
        key[3].key = &avrcp_proto_data->channel;
        key[4].length = 1;
        key[4].key = &k_op;
        key[5].length = 1;
        key[5].key = &frame_number;
        key[6].length = 0;
        key[6].key = NULL;

        fragment = (fragment_t *)wmem_tree_lookup32_array_le(reassembling, key);
        if (fragment && fragment->interface_id == avrcp_proto_data->interface_id &&
                    fragment->adapter_id == avrcp_proto_data->adapter_id &&
                    fragment->chandle == avrcp_proto_data->chandle &&
                    fragment->psm == avrcp_proto_data->channel &&
                    fragment->op == (pdu_id | (company_id << 8))) {


            if (fragment->state == 1 && pinfo->fd->flags.visited == 0) {
                fragment->end_frame_number = pinfo->num;
                fragment->count += 1;
                fragment->state = 2;

                data_fragment = wmem_new(wmem_file_scope(), data_fragment_t);
                data_fragment->length = length;
                data_fragment->data = (guint8 *) wmem_alloc(wmem_file_scope(), data_fragment->length);
                tvb_memcpy(tvb, data_fragment->data, offset, data_fragment->length);
                wmem_tree_insert32(fragment->fragments, fragment->count, data_fragment);
            }
            /* reassembling*/
            length = 0;
            if  (fragment->state == 2) {
                guint       i_length = 0;
                guint8     *reassembled;

                for (i_frame = 1; i_frame <= fragment->count; ++i_frame) {
                    data_fragment = (data_fragment_t *)wmem_tree_lookup32_le(fragment->fragments, i_frame);
                    length += data_fragment->length;
                }

                reassembled = (guint8 *) wmem_alloc(pinfo->pool, length);

                for (i_frame = 1; i_frame <= fragment->count; ++i_frame) {
                    data_fragment = (data_fragment_t *)wmem_tree_lookup32_le(fragment->fragments, i_frame);
                    memcpy(reassembled + i_length,
                            data_fragment->data,
                            data_fragment->length);
                    i_length += data_fragment->length;
                }

                next_tvb = tvb_new_child_real_data(tvb, reassembled, length, length);
                add_new_data_source(pinfo, next_tvb, "Reassembled AVRCP");

                tvb = next_tvb;
                offset = 0;

                pitem = proto_tree_add_item(tree, hf_btavrcp_reassembled, tvb, offset, 0, ENC_NA);
                PROTO_ITEM_SET_GENERATED(pitem);
            }
        }
    }

    if (ctype == 0x0a) { /* REJECT */
        guint status;
        proto_tree_add_item(tree, hf_btavrcp_status, tvb, offset, 1, ENC_BIG_ENDIAN);
        status = tvb_get_guint8(tvb, offset);
        offset += 1;

        col_append_fstr(pinfo->cinfo, COL_INFO, " - Status: %s",
                val_to_str_const(status, status_vals, "Unknown status"));
    } else switch(pdu_id) {
        case PDU_GET_CAPABILITIES:
            if (is_command)  {
                guint capability;

                proto_tree_add_item(tree, hf_btavrcp_capability, tvb, offset, 1, ENC_BIG_ENDIAN);
                capability = tvb_get_guint8(tvb, offset);
                *op_arg = capability;
                col_append_fstr(pinfo->cinfo, COL_INFO, "(%s)",
                        val_to_str_const(capability, capability_vals, "unknown"));
                offset += 1;
            } else {
                guint capability;
                guint capability_count;
                guint i_capability;

                proto_tree_add_item(tree, hf_btavrcp_capability, tvb, offset, 1, ENC_BIG_ENDIAN);
                capability = tvb_get_guint8(tvb, offset);
                *op_arg = capability;
                offset += 1;
                proto_tree_add_item(tree, hf_btavrcp_capability_count, tvb, offset, 1, ENC_BIG_ENDIAN);
                capability_count = tvb_get_guint8(tvb, offset);
                offset += 1;

                for (i_capability = 0; i_capability < capability_count; ++i_capability) {
                    if (capability == 0x02) {
                        proto_tree_add_item(tree, hf_btavrcp_company_id, tvb, offset, 3, ENC_BIG_ENDIAN);
                        offset += 3;
                    } else if (capability == 0x03) {
                        proto_tree_add_item(tree, hf_btavrcp_event_id, tvb, offset, 1, ENC_BIG_ENDIAN);
                        offset += 1;
                    }
                }

                col_append_fstr(pinfo->cinfo, COL_INFO, "(%s) - Count: %u",
                        val_to_str_const(capability, capability_vals, "unknown"), capability_count);
            }
            break;
        case PDU_LIST_PLAYER_APPLICATION_SETTING_ATTRIBUTES:
            if (is_command)  {
                /* non */
            } else {
                guint number_of_attributes;
                guint i_attribute;

                proto_tree_add_item(tree, hf_btavrcp_number_of_settings, tvb, offset, 1, ENC_BIG_ENDIAN);
                number_of_attributes = tvb_get_guint8(tvb, offset);
                offset += 1;

                for (i_attribute = 0; i_attribute < number_of_attributes; ++i_attribute) {
                    proto_tree_add_item(tree, hf_btavrcp_settings_attribute, tvb, offset, 1, ENC_BIG_ENDIAN);
                    offset += 1;
                }
            }
            break;
        case PDU_LIST_PLAYER_APPLICATION_SETTING_VALUE:
            if (is_command)  {
                proto_tree_add_item(tree, hf_btavrcp_settings_attribute, tvb, offset, 1, ENC_BIG_ENDIAN);
                offset += 1;
            } else {
                guint number_of_values;
                guint i_value;

                proto_tree_add_item(tree, hf_btavrcp_number_of_settings, tvb, offset, 1, ENC_BIG_ENDIAN);
                number_of_values = tvb_get_guint8(tvb, offset);
                offset += 1;

                for (i_value = 0; i_value < number_of_values; ++i_value) {
                    proto_tree_add_item(tree, hf_btavrcp_settings_value, tvb, offset, 1, ENC_BIG_ENDIAN);
                    offset += 1;
                }
            }
            break;
        case PDU_GET_CURRENT_PLAYER_APPLICATION_SETTING_VALUE:
            if (is_command)  {
                guint number_of_attributes;
                guint i_attribute;

                proto_tree_add_item(tree, hf_btavrcp_number_of_settings, tvb, offset, 1, ENC_BIG_ENDIAN);
                number_of_attributes = tvb_get_guint8(tvb, offset);
                offset += 1;

                for (i_attribute = 0; i_attribute < number_of_attributes; ++i_attribute) {
                    proto_tree_add_item(tree, hf_btavrcp_settings_attribute, tvb, offset, 1, ENC_BIG_ENDIAN);
                    offset += 1;
                }
            } else {
                guint number_of_settings;
                guint i_setting;

                proto_tree_add_item(tree, hf_btavrcp_number_of_settings, tvb, offset, 1, ENC_BIG_ENDIAN);
                number_of_settings = tvb_get_guint8(tvb, offset);
                offset += 1;

                for (i_setting = 0; i_setting < number_of_settings; ++i_setting) {
                    proto_tree_add_item(tree, hf_btavrcp_settings_attribute, tvb, offset, 1, ENC_BIG_ENDIAN);
                    offset += 1;
                    proto_tree_add_item(tree, hf_btavrcp_settings_value, tvb, offset, 1, ENC_BIG_ENDIAN);
                    offset += 1;
                }
            }
            break;
        case PDU_SET_PLAYER_APPLICATION_SETTING_VALUE:
            if (is_command)  {
                guint number_of_settings;
                guint i_setting;

                proto_tree_add_item(tree, hf_btavrcp_number_of_settings, tvb, offset, 1, ENC_BIG_ENDIAN);
                number_of_settings = tvb_get_guint8(tvb, offset);
                offset += 1;

                for (i_setting = 0; i_setting < number_of_settings; ++i_setting) {
                    proto_tree_add_item(tree, hf_btavrcp_settings_attribute, tvb, offset, 1, ENC_BIG_ENDIAN);
                    offset += 1;
                    proto_tree_add_item(tree, hf_btavrcp_settings_value, tvb, offset, 1, ENC_BIG_ENDIAN);
                    offset += 1;
                }
            } else {
                /* non */
            }
            break;
        case PDU_GET_PLAYER_APPLICATION_SETTING_ATTRIBUTE_TEXT:
            if (is_command)  {
                guint number_of_attributes;
                guint i_attribute;

                proto_tree_add_item(tree, hf_btavrcp_number_of_settings, tvb, offset, 1, ENC_BIG_ENDIAN);
                number_of_attributes = tvb_get_guint8(tvb, offset);
                offset += 1;

                for (i_attribute = 0; i_attribute < number_of_attributes; ++i_attribute) {
                    proto_tree_add_item(tree, hf_btavrcp_settings_attribute, tvb, offset, 1, ENC_BIG_ENDIAN);
                    offset += 1;
                }
            } else {
                guint number_of_attributes;
                guint i_attribute;
                guint attribute_name_length;

                proto_tree_add_item(tree, hf_btavrcp_number_of_settings, tvb, offset, 1, ENC_BIG_ENDIAN);
                number_of_attributes = tvb_get_guint8(tvb, offset);
                offset += 1;

                for (i_attribute = 0; i_attribute < number_of_attributes; ++i_attribute) {
                    proto_tree_add_item(tree, hf_btavrcp_settings_attribute, tvb, offset, 1, ENC_BIG_ENDIAN);
                    offset += 1;

                    proto_tree_add_item(tree, hf_btavrcp_character_set, tvb, offset, 1, ENC_BIG_ENDIAN);
                    offset += 2;

                    proto_tree_add_item(tree, hf_btavrcp_attribute_name_length, tvb, offset, 1, ENC_BIG_ENDIAN);
                    attribute_name_length = tvb_get_ntohs(tvb, offset);
                    offset += 1;

                    proto_tree_add_item(tree, hf_btavrcp_attribute_name, tvb, offset, attribute_name_length, ENC_UTF_8 | ENC_NA);
                    offset += attribute_name_length;
                }
            }
            break;
        case PDU_GET_PLAYER_APPLICATION_SETTING_VALUE_TEXT:
            if (is_command)  {
                guint number_of_values;
                guint i_value;

                proto_tree_add_item(tree, hf_btavrcp_settings_attribute, tvb, offset, 1, ENC_BIG_ENDIAN);
                offset += 1;
                proto_tree_add_item(tree, hf_btavrcp_number_of_settings, tvb, offset, 1, ENC_BIG_ENDIAN);
                number_of_values = tvb_get_guint8(tvb, offset);
                offset += 1;

                for (i_value = 0; i_value < number_of_values; ++i_value) {
                    proto_tree_add_item(tree, hf_btavrcp_settings_value, tvb, offset, 1, ENC_BIG_ENDIAN);
                    offset += 1;
                }
            } else {
                guint number_of_values;
                guint i_attribute;
                guint attribute_value_length;

                proto_tree_add_item(tree, hf_btavrcp_number_of_settings, tvb, offset, 1, ENC_BIG_ENDIAN);
                number_of_values = tvb_get_guint8(tvb, offset);
                offset += 1;

                for (i_attribute = 0; i_attribute < number_of_values; ++i_attribute) {
                    proto_tree_add_item(tree, hf_btavrcp_settings_attribute, tvb, offset, 1, ENC_BIG_ENDIAN);
                    offset += 1;

                    proto_tree_add_item(tree, hf_btavrcp_character_set, tvb, offset, 1, ENC_BIG_ENDIAN);
                    offset += 2;

                    proto_tree_add_item(tree, hf_btavrcp_attribute_value_length, tvb, offset, 1, ENC_BIG_ENDIAN);
                    attribute_value_length = tvb_get_ntohs(tvb, offset);
                    offset += 1;

                    proto_tree_add_item(tree, hf_btavrcp_attribute_value, tvb, offset, attribute_value_length, ENC_UTF_8 | ENC_NA);
                    offset += attribute_value_length;
                }
            }
            break;
        case PDU_INFORM_DISPLAYABLE_CHARACTER_SET:
            if (is_command)  {
                guint number_of_character_set;
                guint i_character_set;

                proto_tree_add_item(tree, hf_btavrcp_number_of_character_set, tvb, offset, 1, ENC_BIG_ENDIAN);
                number_of_character_set = tvb_get_guint8(tvb, offset);
                offset += 1;

                for (i_character_set = 0; i_character_set < number_of_character_set; ++i_character_set) {
                    proto_tree_add_item(tree, hf_btavrcp_character_set, tvb, offset, 2, ENC_BIG_ENDIAN);
                    offset += 2;
                }
            } else {
                /* non */
            }
            break;
        case PDU_INFORM_BATTERY_STATUS_OF_CT:
            if (is_command)  {
                guint battery_status;

                proto_tree_add_item(tree, hf_btavrcp_battery_status, tvb, offset, 1, ENC_BIG_ENDIAN);
                battery_status = tvb_get_guint8(tvb, offset);
                offset += 1;
                col_append_fstr(pinfo->cinfo, COL_INFO, " - Battery: %s", val_to_str_const(battery_status, battery_status_vals, "unknown"));
            } else {
                /* non */
            }
            break;
        case PDU_GET_ELEMENT_ATTRIBUTES:
            if (is_command)  {
                guint  number_of_attributes;
                guint64       identifier;

                proto_tree_add_item(tree, hf_btavrcp_identifier, tvb, offset, 8, ENC_BIG_ENDIAN);
                identifier = tvb_get_ntoh64(tvb, offset);
                offset += 8;

                col_append_fstr(pinfo->cinfo, COL_INFO, " - 0x%08X%08X", (guint) (identifier >> 32), (guint) (identifier & 0xFFFFFFFF));
                if (identifier == 0x00) col_append_str(pinfo->cinfo, COL_INFO, " (PLAYING)");

                pitem = proto_tree_add_item(tree, hf_btavrcp_number_of_attributes, tvb, offset, 1, ENC_BIG_ENDIAN);
                number_of_attributes = tvb_get_guint8(tvb, offset);
                if (number_of_attributes == 0) proto_item_append_text(pitem, " (All Supported Attributes)");
                offset += 1;
                offset = dissect_attribute_id_list(tvb, tree, offset, number_of_attributes);
            } else {
                guint number_of_attributes;

                proto_tree_add_item(tree, hf_btavrcp_number_of_attributes, tvb, offset, 1, ENC_BIG_ENDIAN);
                number_of_attributes = tvb_get_guint8(tvb, offset);
                offset += 1;
                offset = dissect_attribute_entries(tvb, pinfo, tree, offset, number_of_attributes);
            }
            break;
        case PDU_GET_PLAY_STATUS:
            if (is_command)  {
                /* non */
            } else {
                guint  song_length;
                guint  song_position;
                guint  play_status;

                proto_tree_add_item(tree, hf_btavrcp_song_length, tvb, offset, 4, ENC_BIG_ENDIAN);
                song_length = tvb_get_ntohl(tvb, offset);
                offset += 4;
                proto_tree_add_item(tree, hf_btavrcp_song_position, tvb, offset, 4, ENC_BIG_ENDIAN);
                song_position = tvb_get_ntohl(tvb, offset);
                offset += 4;
                proto_tree_add_item(tree, hf_btavrcp_play_status, tvb, offset, 1, ENC_BIG_ENDIAN);
                play_status = tvb_get_guint8(tvb, offset);
                offset += 1;

                col_append_fstr(pinfo->cinfo, COL_INFO, " PlayStatus: %s, SongPosition: %ums, SongLength: %ums",
                        val_to_str_const(play_status, play_status_vals, "unknown"), song_position, song_length);
            }
            break;
        case PDU_REGISTER_NOTIFICATION:
            event_id = tvb_get_guint8(tvb, offset);
            *op_arg = event_id;
            col_append_fstr(pinfo->cinfo, COL_INFO, " - %s",
                    val_to_str_const(event_id, notification_vals, "Unknown Event ID"));

            if (is_command)  {
                proto_tree_add_item(tree, hf_btavrcp_event_id, tvb, offset, 1, ENC_BIG_ENDIAN);
                offset += 1;
                proto_tree_add_item(tree, hf_btavrcp_notification_interval, tvb, offset, 4, ENC_BIG_ENDIAN);
                offset += 4;
            } else {
                guint       number_of_settings;
                guint       i_setting;
                guint64     identifier;
                guint       play_status;
                guint       song_position;
                guint       battery_status;
                guint       uid_counter;
                guint       player_id;
                guint       system_status;

                proto_tree_add_item(tree, hf_btavrcp_event_id, tvb, offset, 1, ENC_BIG_ENDIAN);
                offset += 1;

                if (ctype == 0x0D || ctype == 0x0F) switch(event_id) {
                    case EVENT_PLAYBACK_STATUS_CHANGED:
                        proto_tree_add_item(tree, hf_btavrcp_play_status, tvb, offset, 1, ENC_BIG_ENDIAN);
                        play_status = tvb_get_guint8(tvb, offset);
                        offset += 1;
                        col_append_fstr(pinfo->cinfo, COL_INFO, " - PlayStatus: %s", val_to_str_const(play_status, play_status_vals, "unknown"));
                        break;
                    case EVENT_TRACK_CHANGED:
                        pitem = proto_tree_add_item(tree, hf_btavrcp_identifier, tvb, offset, 8, ENC_BIG_ENDIAN);
                        identifier = tvb_get_ntoh64(tvb, offset);
                        offset += 8;

                        col_append_fstr(pinfo->cinfo, COL_INFO, " - 0x%08X%08X", (guint) (identifier >> 32), (guint) (identifier & 0xFFFFFFFF));
                        if (identifier == G_GUINT64_CONSTANT(0x0000000000000000)) {
                            col_append_str(pinfo->cinfo, COL_INFO, " (SELECTED)");
                            proto_item_append_text(pitem, " (SELECTED)");
                        } else if (identifier == G_GUINT64_CONSTANT(0xFFFFFFFFFFFFFFFF)) {
                            col_append_str(pinfo->cinfo, COL_INFO, " (NOT SELECTED)");
                            proto_item_append_text(pitem, " (NOT SELECTED)");
                        }

                        break;
                    case EVENT_TRACK_REACHED_END:
                        /* non */
                        break;
                    case EVENT_TRACK_REACHED_START:
                        /* non */
                        break;
                    case EVENT_PLAYBACK_POSITION_CHANGED:
                        pitem = proto_tree_add_item(tree, hf_btavrcp_song_position, tvb, offset, 4, ENC_BIG_ENDIAN);
                        song_position = tvb_get_ntohl(tvb, offset);
                        offset += 4;
                        col_append_fstr(pinfo->cinfo, COL_INFO, " - SongPosition: %ums", song_position);
                        if (song_position == 0xFFFFFFFF) {
                            proto_item_append_text(pitem, " (NOT SELECTED)");
                            col_append_str(pinfo->cinfo, COL_INFO, " (NOT SELECTED)");
                        } else if (!pinfo->fd->flags.visited) {
                            btavrcp_song_position_data_t  *song_position_data;

                            frame_number = pinfo->num;

                            key[0].length = 1;
                            key[0].key = &avrcp_proto_data->interface_id;
                            key[1].length = 1;
                            key[1].key = &avrcp_proto_data->adapter_id;
                            key[2].length = 1;
                            key[2].key = &frame_number;
                            key[3].length = 0;
                            key[3].key = NULL;

                            song_position_data = wmem_new(wmem_file_scope(), btavrcp_song_position_data_t);
                            song_position_data->song_position = song_position;
                            song_position_data->used_in_frame = 0;

                            wmem_tree_insert32_array(btavrcp_song_positions, key, song_position_data);
                        }
                        break;
                    case EVENT_BATTERY_STATUS_CHANGED:
                        proto_tree_add_item(tree, hf_btavrcp_battery_status, tvb, offset, 1, ENC_BIG_ENDIAN);
                        battery_status = tvb_get_guint8(tvb, offset);
                        offset += 1;
                        col_append_fstr(pinfo->cinfo, COL_INFO, " - Battery: %s", val_to_str_const(battery_status, battery_status_vals, "unknown"));
                        break;
                    case EVENT_SYSTEM_STATUS_CHANGED:
                        proto_tree_add_item(tree, hf_btavrcp_system_status, tvb, offset, 1, ENC_BIG_ENDIAN);
                        system_status = tvb_get_guint8(tvb, offset);
                        offset += 1;
                        col_append_fstr(pinfo->cinfo, COL_INFO, " - SystemStatus: %s", val_to_str_const(system_status, system_status_vals, "unknown"));
                        break;
                    case EVENT_PLAYER_APPLICATION_SETTING_CHANGED:
                        proto_tree_add_item(tree, hf_btavrcp_number_of_settings, tvb, offset, 1, ENC_BIG_ENDIAN);
                        number_of_settings = tvb_get_guint8(tvb, offset);
                        offset += 1;

                        for (i_setting = 0; i_setting < number_of_settings; ++i_setting) {
                            proto_tree_add_item(tree, hf_btavrcp_settings_attribute, tvb, offset, 1, ENC_BIG_ENDIAN);
                            offset += 1;
                            proto_tree_add_item(tree, hf_btavrcp_settings_value, tvb, offset, 1, ENC_BIG_ENDIAN);
                            offset += 1;
                        }
                        break;
                    case EVENT_NOWPLAYING_CONTENT_CHANGED:
                        /* non */
                        break;
                    case EVENT_AVAILABLE_PLAYERS_CHANGED:
                        /* non */
                        break;
                    case EVENT_ADDRESSEDPLAYER_CHANGED:
                        proto_tree_add_item(tree, hf_btavrcp_player_id, tvb, offset, 2, ENC_BIG_ENDIAN);
                        player_id = tvb_get_ntohs(tvb, offset);
                        offset += 2;
                        proto_tree_add_item(tree, hf_btavrcp_uid_counter, tvb, offset, 2, ENC_BIG_ENDIAN);
                        uid_counter = tvb_get_ntohs(tvb, offset);
                        offset += 2;
                        col_append_fstr(pinfo->cinfo, COL_INFO, " - PlayerID: %u, UidCounter: 0x%04x", player_id, uid_counter);
                        break;
                    case EVENT_UIDS_CHANGED:
                        proto_tree_add_item(tree, hf_btavrcp_uid_counter, tvb, offset, 2, ENC_BIG_ENDIAN);
                        uid_counter = tvb_get_ntohs(tvb, offset);
                        offset += 2;
                        col_append_fstr(pinfo->cinfo, COL_INFO, " - UidCounter: 0x%04x", uid_counter);
                        break;
                    case EVENT_VOLUME_CHANGED:
                        proto_tree_add_item(tree, hf_btavrcp_absolute_volume_rfa, tvb, offset, 1, ENC_BIG_ENDIAN);
                        pitem = proto_tree_add_item(tree, hf_btavrcp_absolute_volume, tvb, offset, 1, ENC_BIG_ENDIAN);

                        volume = tvb_get_guint8(tvb, offset) & 0x7F;
                        volume_percent = (guint) ((double) volume * 100 / (double) 0x7F);
                        offset += 1;

                        proto_item_append_text(pitem, " (%u%%)", volume_percent);
                        col_append_fstr(pinfo->cinfo, COL_INFO, " - Volume: %u%%", volume_percent);
                        break;
                    default:
                        proto_tree_add_item(tree, hf_btavrcp_data, tvb, offset, tvb_reported_length_remaining(tvb, offset), ENC_NA);
                        offset = tvb_reported_length(tvb);
                        break;
                }
            }

            break;
        case PDU_REQUEST_CONTINUING_RESPONSE:
            if (is_command)  {
                guint continuing_op;

                proto_tree_add_item(tree, hf_btavrcp_bt_continuing_pdu_id, tvb, offset, 1, ENC_BIG_ENDIAN);
                continuing_op = tvb_get_guint8(tvb, offset) | (company_id << 8);
                col_append_fstr(pinfo->cinfo, COL_INFO, " - %s",
                        val_to_str_const(tvb_get_guint8(tvb, offset), pdu_id_vals, "Unknown opcode"));
                offset += 1;

                if (pinfo->fd->flags.visited == 0) {
                    k_op           = continuing_op;
                    frame_number = pinfo->num;

                    key[0].length = 1;
                    key[0].key = &avrcp_proto_data->interface_id;
                    key[1].length = 1;
                    key[1].key = &avrcp_proto_data->adapter_id;
                    key[2].length = 1;
                    key[2].key = &avrcp_proto_data->chandle;
                    key[3].length = 1;
                    key[3].key = &avrcp_proto_data->channel;
                    key[4].length = 1;
                    key[4].key = &k_op;
                    key[5].length = 1;
                    key[5].key = &frame_number;
                    key[6].length = 0;
                    key[6].key = NULL;

                    fragment = (fragment_t *)wmem_tree_lookup32_array_le(reassembling, key);
                    if (fragment && fragment->interface_id == avrcp_proto_data->interface_id &&
                            fragment->adapter_id == avrcp_proto_data->adapter_id &&
                            fragment->chandle == avrcp_proto_data->chandle &&
                            fragment->psm == avrcp_proto_data->channel &&
                            fragment->op == continuing_op &&
                            fragment->state == 0) {
                        fragment->state = 1;
                    }
                }

                *op = continuing_op;
            } else {
                /* non */
            }
            break;
        case PDU_ABORT_CONTINUING_RESPONSE:
            if (is_command)  {
                guint continuing_op;

                proto_tree_add_item(tree, hf_btavrcp_bt_continuing_pdu_id, tvb, offset, 1, ENC_BIG_ENDIAN);
                continuing_op = tvb_get_guint8(tvb, offset) | (company_id << 8);
                col_append_fstr(pinfo->cinfo, COL_INFO, " - %s",
                        val_to_str_const(tvb_get_guint8(tvb, offset), pdu_id_vals, "Unknown opcode"));
                offset += 1;

                if (pinfo->fd->flags.visited == 0) {
                    k_op           = continuing_op;
                    frame_number = pinfo->num;

                    key[0].length = 1;
                    key[0].key = &avrcp_proto_data->interface_id;
                    key[1].length = 1;
                    key[1].key = &avrcp_proto_data->adapter_id;
                    key[2].length = 1;
                    key[2].key = &avrcp_proto_data->chandle;
                    key[3].length = 1;
                    key[3].key = &avrcp_proto_data->channel;
                    key[4].length = 1;
                    key[4].key = &k_op;
                    key[5].length = 1;
                    key[5].key = &frame_number;
                    key[6].length = 0;
                    key[6].key = NULL;

                    fragment = (fragment_t *)wmem_tree_lookup32_array_le(reassembling, key);
                    if (fragment && fragment->interface_id == avrcp_proto_data->interface_id &&
                            fragment->adapter_id == avrcp_proto_data->adapter_id &&
                            fragment->chandle == avrcp_proto_data->chandle &&
                            fragment->psm == avrcp_proto_data->channel &&
                            fragment->op == continuing_op &&
                            fragment->state == 0) {
                        fragment->state = 3;
                    }
                }
            } else {
                /* non */
            }
            break;
        case PDU_SET_ABSOLUTE_VOLUME:
            proto_tree_add_item(tree, hf_btavrcp_absolute_volume_rfa, tvb, offset, 1, ENC_BIG_ENDIAN);
            pitem = proto_tree_add_item(tree, hf_btavrcp_absolute_volume, tvb, offset, 1, ENC_BIG_ENDIAN);
            volume = tvb_get_guint8(tvb, offset) & 0x7F;
            volume_percent = (guint) ((double) volume * 100 / (double) 0x7F);
            offset += 1;

            proto_item_append_text(pitem, " (%u%%)", volume_percent);
            col_append_fstr(pinfo->cinfo, COL_INFO, " - Volume: %u%%", volume_percent);
            break;
        case PDU_SET_ADDRESSED_PLAYER:
            if (is_command)  {
                guint player_id;

                proto_tree_add_item(tree, hf_btavrcp_player_id, tvb, offset, 2, ENC_BIG_ENDIAN);
                player_id = tvb_get_ntohs(tvb, offset);
                offset += 2;

                col_append_fstr(pinfo->cinfo, COL_INFO, " - Player ID: %u", player_id);
            } else {
                guint status;

                proto_tree_add_item(tree, hf_btavrcp_status, tvb, offset, 1, ENC_BIG_ENDIAN);
                status = tvb_get_guint8(tvb, offset);
                offset += 1;

                col_append_fstr(pinfo->cinfo, COL_INFO, " - Status: %s",
                        val_to_str_const(status, status_vals, "Unknown status"));
            }
            break;
        case PDU_PLAY_ITEM:
            if (is_command)  {
                guint scope;
                guint64 uid;
                guint uid_counter;

                proto_tree_add_item(tree, hf_btavrcp_scope, tvb, offset, 1, ENC_BIG_ENDIAN);
                scope = tvb_get_guint8(tvb, offset);
                offset += 1;
                proto_tree_add_item(tree, hf_btavrcp_uid, tvb, offset, 8, ENC_BIG_ENDIAN);
                uid = tvb_get_ntoh64(tvb, offset);
                offset += 8;
                proto_tree_add_item(tree, hf_btavrcp_uid_counter, tvb, offset, 2, ENC_BIG_ENDIAN);
                uid_counter = tvb_get_ntohs(tvb, offset);
                offset += 2;

                col_append_fstr(pinfo->cinfo, COL_INFO, " - Scope: %s, Uid: 0x%016" G_GINT64_MODIFIER "x, UidCounter: 0x%04x",
                        val_to_str_const(scope, scope_vals, "unknown"), uid, uid_counter);
            } else {
                guint status;

                proto_tree_add_item(tree, hf_btavrcp_status, tvb, offset, 1, ENC_BIG_ENDIAN);
                status = tvb_get_guint8(tvb, offset);
                offset += 1;
                col_append_fstr(pinfo->cinfo, COL_INFO, " - Status: %s",
                        val_to_str_const(status, status_vals, "Unknown status"));
            }
            break;
        case PDU_ADD_TO_NOW_PLAYING:
            if (is_command)  {
                guint scope;
                guint64 uid;
                guint uid_counter;

                proto_tree_add_item(tree, hf_btavrcp_scope, tvb, offset, 1, ENC_BIG_ENDIAN);
                scope = tvb_get_guint8(tvb, offset);
                offset += 1;
                proto_tree_add_item(tree, hf_btavrcp_uid, tvb, offset, 8, ENC_BIG_ENDIAN);
                uid = tvb_get_ntoh64(tvb, offset);
                offset += 8;
                proto_tree_add_item(tree, hf_btavrcp_uid_counter, tvb, offset, 2, ENC_BIG_ENDIAN);
                uid_counter = tvb_get_ntohs(tvb, offset);
                offset += 2;

                col_append_fstr(pinfo->cinfo, COL_INFO, " - Scope: %s, Uid: 0x%016" G_GINT64_MODIFIER "x, UidCounter: 0x%04x",
                        val_to_str_const(scope, scope_vals, "unknown"), uid, uid_counter);
            } else {
                guint status;

                proto_tree_add_item(tree, hf_btavrcp_status, tvb, offset, 1, ENC_BIG_ENDIAN);
                status = tvb_get_guint8(tvb, offset);
                offset += 1;
                col_append_fstr(pinfo->cinfo, COL_INFO, " - Status: %s",
                        val_to_str_const(status, status_vals, "Unknown status"));
            }
            break;
    };

    return offset;
}


static gint
dissect_browsing(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                 gint offset, gboolean is_command)
{
    guint pdu_id;
    guint status = 0x00;

    proto_tree_add_item(tree, hf_btavrcp_browsing_pdu_id, tvb, offset, 1, ENC_BIG_ENDIAN);
    pdu_id = tvb_get_guint8(tvb, offset);
    offset += 1;

    proto_tree_add_item(tree, hf_btavrcp_length, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset += 2;

    if (is_command) {
        col_append_str(pinfo->cinfo, COL_INFO, ": Command");
    } else {
        /* common code "status" in response */
        proto_tree_add_item(tree, hf_btavrcp_status, tvb, offset, 1, ENC_BIG_ENDIAN);
        status = tvb_get_guint8(tvb, offset);
        offset += 1;

        col_append_fstr(pinfo->cinfo, COL_INFO, ": %s",
                val_to_str_const(status, status_vals, "Unknown status"));
    }

    col_append_fstr(pinfo->cinfo, COL_INFO, " - %s",
                    val_to_str_const(pdu_id, browsing_pdu_id_vals, "Unknown opcode"));

    if (is_command || status == STATUS_OK) switch(pdu_id) {
        case PDU_SET_BROWSED_PLAYER:
            if (is_command)  {
                guint player_id;

                proto_tree_add_item(tree, hf_btavrcp_player_id, tvb, offset, 2, ENC_BIG_ENDIAN);
                player_id = tvb_get_ntohs(tvb, offset);
                offset += 2;

                col_append_fstr(pinfo->cinfo, COL_INFO, " - Player ID: %u", player_id);
            } else {
                guint         i_folder;
                guint         folder_depth;
                guint         folder_name_length;
                proto_item    *pitem;
                proto_tree    *ptree;
                const guint8  *folder_name;

                proto_tree_add_item(tree, hf_btavrcp_uid_counter, tvb, offset, 2, ENC_BIG_ENDIAN);
                offset += 2;
                proto_tree_add_item(tree, hf_btavrcp_number_of_items, tvb, offset, 4, ENC_BIG_ENDIAN);
                offset += 4;
                proto_tree_add_item(tree, hf_btavrcp_character_set, tvb, offset, 2, ENC_BIG_ENDIAN);
                offset += 2;
                proto_tree_add_item(tree, hf_btavrcp_folder_depth, tvb, offset, 1, ENC_BIG_ENDIAN);
                folder_depth = tvb_get_guint8(tvb, offset);
                offset += 1;

                pitem = proto_tree_add_none_format(tree, hf_btavrcp_currect_path, tvb, offset, tvb_reported_length_remaining(tvb, offset), "Current Path: /");
                col_append_str(pinfo->cinfo, COL_INFO, "Current Path: /");
                ptree = proto_item_add_subtree(pitem, ett_btavrcp_path);

                for (i_folder = 0; i_folder < folder_depth; ++i_folder) {
                    proto_tree_add_item(ptree, hf_btavrcp_folder_name_length, tvb, offset, 2, ENC_BIG_ENDIAN);
                    folder_name_length = tvb_get_ntohs(tvb, offset);
                    offset += 2;
                    proto_tree_add_item_ret_string(ptree, hf_btavrcp_folder_name, tvb, offset, folder_name_length, ENC_ASCII|ENC_NA, wmem_packet_scope(), &folder_name);
                    offset += folder_name_length;
                    proto_item_append_text(pitem, "%s/", folder_name);
                    col_append_fstr(pinfo->cinfo, COL_INFO, "%s/", folder_name);
                }
            }
            break;
        case PDU_GET_FOLDER_ITEMS:
            if (is_command)  {
                guint attribute_count;
                guint scope;
                guint start_item;
                guint end_item;

                proto_tree_add_item(tree, hf_btavrcp_scope, tvb, offset, 1, ENC_BIG_ENDIAN);
                scope = tvb_get_guint8(tvb, offset);
                offset += 1;
                proto_tree_add_item(tree, hf_btavrcp_start_item, tvb, offset, 4, ENC_BIG_ENDIAN);
                start_item = tvb_get_ntohl(tvb, offset);
                offset += 4;
                proto_tree_add_item(tree, hf_btavrcp_end_item, tvb, offset, 4, ENC_BIG_ENDIAN);
                end_item = tvb_get_ntohl(tvb, offset);
                offset += 4;
                proto_tree_add_item(tree, hf_btavrcp_attribute_count, tvb, offset, 1, ENC_BIG_ENDIAN);
                attribute_count = tvb_get_guint8(tvb, offset);
                offset += 1;
                offset = dissect_attribute_id_list(tvb, tree, offset, attribute_count);

                col_append_fstr(pinfo->cinfo, COL_INFO, " - Scope: %s, StartItem: 0x%04x, EndItem: 0x%04x",
                        val_to_str_const(scope, scope_vals, "unknown"), start_item, end_item);
            } else {
                guint number_of_items;
                guint uid_counter;
                guint i_item;
                guint item_type;
                guint item_length;

                proto_tree_add_item(tree, hf_btavrcp_uid_counter, tvb, offset, 2, ENC_BIG_ENDIAN);
                uid_counter = tvb_get_ntohs(tvb, offset);
                offset += 2;
                proto_tree_add_item(tree, hf_btavrcp_number_of_items16, tvb, offset, 2, ENC_BIG_ENDIAN);
                number_of_items = tvb_get_ntohs(tvb, offset);
                offset += 2;

                col_append_fstr(pinfo->cinfo, COL_INFO, " - UidCounter: 0x%04x, NumberOfItems: %u",
                        uid_counter, number_of_items);

                for (i_item = 0; i_item < number_of_items; ++i_item) {
                    item_type = tvb_get_guint8(tvb, offset);
                    item_length = tvb_get_ntohs(tvb, offset + 1);

                    if (item_type == ITEM_MEDIAPLAYER) {
                        dissect_item_mediaplayer(tvb, tree, offset);
                        offset += 1 + 2 + item_length;
                    } else if (item_type == ITEM_MEDIA_ELEMENT) {
                        dissect_item_media_element(tvb, pinfo, tree, offset);
                        offset += 1 + 2 + item_length;
                    } else if (item_type == ITEM_FOLDER) {
                        dissect_item_folder(tvb, tree, offset);
                        offset += 1 + 2 + item_length;
                    } else {
                        proto_tree_add_item(tree, hf_btavrcp_data, tvb, offset, item_length, ENC_NA);
                        offset += 1 + 2 + item_length;
                    }
                }
            }
            break;
        case PDU_CHANGE_PATH:
            if (is_command)  {
                guint64 uid;
                guint uid_counter;
                guint direction;

                proto_tree_add_item(tree, hf_btavrcp_uid_counter, tvb, offset, 2, ENC_BIG_ENDIAN);
                uid_counter = tvb_get_ntohs(tvb, offset);
                offset += 2;
                proto_tree_add_item(tree, hf_btavrcp_direction, tvb, offset, 1, ENC_BIG_ENDIAN);
                direction = tvb_get_guint8(tvb, offset);
                offset += 1;
                proto_tree_add_item(tree, hf_btavrcp_uid, tvb, offset, 8, ENC_BIG_ENDIAN);
                uid = tvb_get_ntoh64(tvb, offset);
                offset += 8;

                col_append_fstr(pinfo->cinfo, COL_INFO, " - Direction: %s, Uid: 0x%016" G_GINT64_MODIFIER "x, UidCounter: 0x%04x",
                        val_to_str_const(direction, direction_vals, "unknown"), uid, uid_counter);
            } else {
                guint number_of_items;

                proto_tree_add_item(tree, hf_btavrcp_number_of_items, tvb, offset, 4, ENC_BIG_ENDIAN);
                number_of_items = tvb_get_ntohl(tvb, offset);
                offset += 4;
                col_append_fstr(pinfo->cinfo, COL_INFO, " - NumberOfItems: %u", number_of_items);
            }
            break;
        case PDU_GET_ITEM_ATTRIBUTES:
            if (is_command)  {
                guint       number_of_attributes;
                guint64     uid;
                guint       uid_counter;
                guint       scope;
                proto_item  *pitem = NULL;

                proto_tree_add_item(tree, hf_btavrcp_scope, tvb, offset, 1, ENC_BIG_ENDIAN);
                scope = tvb_get_guint8(tvb, offset);
                offset += 1;
                proto_tree_add_item(tree, hf_btavrcp_uid, tvb, offset, 8, ENC_BIG_ENDIAN);
                uid = tvb_get_ntoh64(tvb, offset);
                offset += 8;
                proto_tree_add_item(tree, hf_btavrcp_uid_counter, tvb, offset, 2, ENC_BIG_ENDIAN);
                uid_counter = tvb_get_ntohs(tvb, offset);
                offset += 2;
                pitem = proto_tree_add_item(tree, hf_btavrcp_number_of_attributes, tvb, offset, 1, ENC_BIG_ENDIAN);
                number_of_attributes = tvb_get_guint8(tvb, offset);

                col_append_fstr(pinfo->cinfo, COL_INFO, " - Scope: %s, Uid: 0x%016" G_GINT64_MODIFIER "x, UidCounter: 0x%04x",
                        val_to_str_const(scope, scope_vals, "unknown"), uid, uid_counter);

                if (number_of_attributes == 0) proto_item_append_text(pitem, " (All Supported Attributes)");
                offset += 1;
                offset = dissect_attribute_id_list(tvb, tree, offset, number_of_attributes);
            } else {
                guint number_of_attributes;

                proto_tree_add_item(tree, hf_btavrcp_number_of_attributes, tvb, offset, 1, ENC_BIG_ENDIAN);
                number_of_attributes = tvb_get_guint8(tvb, offset);
                offset += 1;
                offset = dissect_attribute_entries(tvb, pinfo, tree, offset, number_of_attributes);
            }
            break;
        case PDU_SEARCH:
            if (is_command)  {
                guint search_length;

                proto_tree_add_item(tree, hf_btavrcp_character_set, tvb, offset, 2, ENC_BIG_ENDIAN);
                offset += 2;
                proto_tree_add_item(tree, hf_btavrcp_search_length, tvb, offset, 2, ENC_BIG_ENDIAN);
                search_length = tvb_get_ntohs(tvb, offset);
                offset += 2;
                proto_tree_add_item(tree, hf_btavrcp_search, tvb, offset, search_length, ENC_NA);
                offset += search_length;
            } else {
                proto_tree_add_item(tree, hf_btavrcp_uid_counter, tvb, offset, 2, ENC_BIG_ENDIAN);
                offset += 2;
                proto_tree_add_item(tree, hf_btavrcp_number_of_items, tvb, offset, 4, ENC_BIG_ENDIAN);
                offset += 4;
            }
            break;
        case PDU_GENERAL_REJECT:
            /* implemented in common code before switch() */
            break;

    };

    return offset;
}

static gint
dissect_btavrcp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data)
{
    proto_item      *ti;
    proto_tree      *btavrcp_tree;
    proto_item      *pitem  = NULL;
    gint             offset = 0;
    guint32          opcode;
    guint32          op     = 0;
    guint32          op_arg = 0;
    guint            ctype;
    guint            response_time;
    guint            max_response_time;
    guint            is_command;
    timing_info_t   *timing_info;
    wmem_tree_key_t  key[9];
    guint32          k_op_arg;
    guint32          frame_number;
    gint             previous_proto;
    avrcp_proto_data_t  avrcp_proto_data;

    previous_proto = (GPOINTER_TO_INT(wmem_list_frame_data(wmem_list_frame_prev(wmem_list_tail(pinfo->layers)))));
    if (previous_proto == proto_btavctp) {
        btavctp_data_t  *avctp_data;

        avctp_data = (btavctp_data_t *) data;

        avrcp_proto_data.interface_id = avctp_data->interface_id;
        avrcp_proto_data.adapter_id   = avctp_data->adapter_id;
        avrcp_proto_data.chandle      = avctp_data->chandle;
        avrcp_proto_data.channel      = avctp_data->psm;

        is_command = !avctp_data->cr;
    } else {
        avrcp_proto_data.interface_id = HCI_INTERFACE_DEFAULT;
        avrcp_proto_data.adapter_id   = HCI_ADAPTER_DEFAULT;
        avrcp_proto_data.chandle      = 0;
        avrcp_proto_data.channel      = 0;

/* NOTE: There is need to allow user specify that */
        is_command = (pinfo->p2p_dir == P2P_DIR_SENT);
    }

    ti = proto_tree_add_item(tree, proto_btavrcp, tvb, offset, tvb_captured_length_remaining(tvb, offset), ENC_NA);
    btavrcp_tree = proto_item_add_subtree(ti, ett_btavrcp);

    col_set_str(pinfo->cinfo, COL_PROTOCOL, "AVRCP");

    switch (pinfo->p2p_dir) {
        case P2P_DIR_SENT:
            col_set_str(pinfo->cinfo, COL_INFO, "Sent ");
            break;
        case P2P_DIR_RECV:
            col_set_str(pinfo->cinfo, COL_INFO, "Rcvd ");
            break;
        default:
            col_set_str(pinfo->cinfo, COL_INFO, "UnknownDirection ");
            break;
    }

    if (avrcp_proto_data.channel == BTL2CAP_PSM_AVCTP_BRWS) {
        col_append_str(pinfo->cinfo, COL_INFO, "Browsing");
        offset = dissect_browsing(tvb, pinfo, btavrcp_tree, offset, is_command);
    } else {
        proto_tree_add_item(btavrcp_tree, hf_btavrcp_reserved, tvb, offset, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(btavrcp_tree, hf_btavrcp_ctype, tvb, offset, 1, ENC_BIG_ENDIAN);
        ctype = tvb_get_guint8(tvb, offset) & 0x0F;
        offset += 1;
        proto_tree_add_item(btavrcp_tree, hf_btavrcp_subunit_type, tvb, offset, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(btavrcp_tree, hf_btavrcp_subunit_id, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset += 1;
        proto_tree_add_item(btavrcp_tree, hf_btavrcp_opcode, tvb, offset, 1, ENC_BIG_ENDIAN);
        opcode = tvb_get_guint8(tvb, offset);
        offset += 1;

        col_append_fstr(pinfo->cinfo, COL_INFO, "%s: %s",
                        val_to_str_const(opcode, opcode_vals, "Unknown opcode"),
                        val_to_str_const(ctype, ctype_vals, "Unknown ctype"));

        switch(opcode) {
            case OPCODE_PASSTHROUGH:
                offset = dissect_passthrough(tvb, pinfo, btavrcp_tree, offset,
                        is_command, &op);
                break;
            case OPCODE_UNIT:
                offset = dissect_unit(tvb, btavrcp_tree, offset, is_command);
                break;
            case OPCODE_SUBUNIT:
                offset = dissect_subunit(tvb, btavrcp_tree, offset, is_command);
                break;
            case OPCODE_VENDOR_DEPENDANT:
                offset = dissect_vendor_dependant(tvb, pinfo, btavrcp_tree,
                        offset, ctype, &op, &op_arg, is_command, &avrcp_proto_data);
                break;
        };

        k_op_arg       = (ctype == 0x0a) ? G_MAXUINT32 : op_arg;
        frame_number = pinfo->num;

        key[0].length = 1;
        key[0].key = &avrcp_proto_data.interface_id;
        key[1].length = 1;
        key[1].key = &avrcp_proto_data.adapter_id;
        key[2].length = 1;
        key[2].key = &avrcp_proto_data.chandle;
        key[3].length = 1;
        key[3].key = &avrcp_proto_data.channel;
        key[4].length = 1;
        key[4].key = &opcode;
        key[5].length = 1;
        key[5].key = &op;
        key[6].length = 1;
        key[6].key = &k_op_arg;
        key[7].length = 1;
        key[7].key = &frame_number;
        key[8].length = 0;
        key[8].key = NULL;

        if (pinfo->fd->flags.visited == 0) {
            if (is_command) {
                if (ctype == 0x00) { /*  MTC is for CONTROL */
                    max_response_time = 200;
                } else if (ctype == 0x01 || ctype == 0x03) {
                                     /* MTP is for STATUS, NOTIFY */
                    max_response_time = 1000;
                } else {             /* RCP */
                    max_response_time = 100;
                }

                timing_info = wmem_new(wmem_file_scope(), timing_info_t);
                timing_info->command_frame_number = pinfo->num;
                timing_info->command_timestamp = pinfo->abs_ts;
                timing_info->response_frame_number = 0;
                timing_info->response_timestamp.secs = 0;
                timing_info->response_timestamp.nsecs = 0;
                timing_info->max_response_time = max_response_time;

                timing_info->interface_id = avrcp_proto_data.interface_id;
                timing_info->adapter_id   = avrcp_proto_data.adapter_id;
                timing_info->chandle      = avrcp_proto_data.chandle;
                timing_info->psm          = avrcp_proto_data.channel;
                timing_info->opcode       = opcode;
                timing_info->op           = op;
                timing_info->op_arg       = op_arg;
                timing_info->used         = 0;

                wmem_tree_insert32_array(timing, key, timing_info);
            } else {
                timing_info = (timing_info_t *)wmem_tree_lookup32_array_le(timing, key);
                if (timing_info && timing_info->interface_id == avrcp_proto_data.interface_id &&
                        timing_info->adapter_id == avrcp_proto_data.adapter_id &&
                        timing_info->chandle == avrcp_proto_data.chandle &&
                        timing_info->psm == avrcp_proto_data.channel &&
                        timing_info->opcode == opcode &&
                        timing_info->op == op &&
                        ((ctype == 0x0a) ? 1 : (timing_info->op_arg == op_arg)) &&
                        timing_info->used == 0) {
                    timing_info->response_frame_number = pinfo->num;
                    timing_info->response_timestamp = pinfo->abs_ts;
                    timing_info->used = 1;
                }
            }

            k_op_arg       = (ctype == 0x0a) ? G_MAXUINT32 : op_arg;
            frame_number = pinfo->num;

            key[0].length = 1;
            key[0].key = &avrcp_proto_data.interface_id;
            key[1].length = 1;
            key[1].key = &avrcp_proto_data.adapter_id;
            key[2].length = 1;
            key[2].key = &avrcp_proto_data.chandle;
            key[3].length = 1;
            key[3].key = &avrcp_proto_data.channel;
            key[4].length = 1;
            key[4].key = &opcode;
            key[5].length = 1;
            key[5].key = &op;
            key[6].length = 1;
            key[6].key = &k_op_arg;
            key[7].length = 1;
            key[7].key = &frame_number;
            key[8].length = 0;
            key[8].key = NULL;

        }

        timing_info = (timing_info_t *)wmem_tree_lookup32_array_le(timing, key);
        if (timing_info && timing_info->interface_id == avrcp_proto_data.interface_id &&
                timing_info->adapter_id == avrcp_proto_data.adapter_id &&
                timing_info->chandle == avrcp_proto_data.chandle &&
                timing_info->psm == avrcp_proto_data.channel &&
                timing_info->opcode == opcode &&
                timing_info->op == op &&
                ((ctype == 0x0a) ? 1 : (timing_info->op_arg == op_arg))) {

            if (timing_info->command_timestamp.nsecs > timing_info->response_timestamp.nsecs) {
                response_time = timing_info->response_timestamp.nsecs + (1000000000 - timing_info->command_timestamp.nsecs);
                response_time /= 1000000;
                response_time += ((guint)timing_info->response_timestamp.secs - 1 - (guint)timing_info->command_timestamp.secs) / 1000;
            } else {
                response_time = timing_info->response_timestamp.nsecs - timing_info->command_timestamp.nsecs;
                response_time /= 1000000;
                response_time += ((guint)timing_info->response_timestamp.secs - (guint)timing_info->command_timestamp.secs) / 1000;
            }

            if (timing_info->response_frame_number == 0) {
                response_time = UINT_MAX;
            }

            pitem = proto_tree_add_uint(btavrcp_tree, hf_btavrcp_response_time, tvb, 0, 0, response_time);
            proto_item_append_text(pitem, "/%ums", timing_info->max_response_time);
            if (response_time > timing_info->max_response_time) {
                proto_item_append_text(pitem, "; TIME EXCEEDED");
            }
            PROTO_ITEM_SET_GENERATED(pitem);

            if (timing_info->response_frame_number == 0) {
                pitem = proto_tree_add_expert(btavrcp_tree, pinfo, &ei_btavrcp_no_response, tvb, 0, 0);
                PROTO_ITEM_SET_GENERATED(pitem);
            }  else {
                if (is_command)  {
                    pitem = proto_tree_add_uint(btavrcp_tree, hf_btavrcp_response_in_frame, tvb, 0, 0, timing_info->response_frame_number);
                    PROTO_ITEM_SET_GENERATED(pitem);
                } else {
                    pitem = proto_tree_add_uint(btavrcp_tree, hf_btavrcp_command_in_frame, tvb, 0, 0, timing_info->command_frame_number);
                    PROTO_ITEM_SET_GENERATED(pitem);
                }
            }

        }

    }

    if (tvb_reported_length_remaining(tvb, offset) > 0) {
        pitem = proto_tree_add_item(btavrcp_tree, hf_btavrcp_data, tvb, offset, tvb_reported_length_remaining(tvb, offset), ENC_NA);
        expert_add_info(pinfo, pitem, &ei_btavrcp_unexpected_data);
    }

    return offset;
}


void
proto_register_btavrcp(void)
{
    module_t *module;
    expert_module_t* expert_btavrcp;

    static hf_register_info hf[] = {
        { &hf_btavrcp_reserved,
            { "Reserved",                        "btavrcp.reserved",
            FT_UINT8, BASE_HEX, NULL, 0xF0,
            NULL, HFILL }
        },
        { &hf_btavrcp_ctype,
            { "Ctype",                           "btavrcp.ctype",
            FT_UINT8, BASE_HEX, VALS(ctype_vals), 0x0F,
            NULL, HFILL }
        },
        { &hf_btavrcp_subunit_type,
            { "Subunit Type",                    "btavrcp.subunit_type",
            FT_UINT8, BASE_HEX, VALS(subunit_type_vals), 0xF8,
            NULL, HFILL }
        },
        { &hf_btavrcp_subunit_id,
            { "Subunit ID",                      "btavrcp.subunit_id",
            FT_UINT8, BASE_HEX, NULL, 0x07,
            NULL, HFILL }
        },
        { &hf_btavrcp_opcode,
            { "Opcode",                          "btavrcp.opcode",
            FT_UINT8, BASE_HEX, VALS(opcode_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_company_id,
            { "Company ID",                      "btavrcp.company_id",
            FT_UINT24, BASE_HEX, VALS(oui_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_passthrough_state,
            { "State",                           "btavrcp.passthrough.state",
            FT_UINT8, BASE_HEX, VALS(passthrough_state_vals), 0x80,
            NULL, HFILL }
        },
        { &hf_btavrcp_passthrough_operation,
            { "Operation ID",                    "btavrcp.passthrough.operation",
            FT_UINT8, BASE_HEX, VALS(passthrough_operation_vals), 0x7F,
            NULL, HFILL }
        },
        { &hf_btavrcp_passthrough_data_length,
            { "Data Length",                     "btavrcp.passthrough.length",
            FT_UINT8, BASE_HEX, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_passthrough_vendor_unique_id,
            { "Vendor Unique ID",                "btavrcp.passthrough.vendor_unique_id",
            FT_UINT16, BASE_HEX, VALS(vendor_unique_id_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_passthrough_company_id,
            { "Company ID",                      "btavrcp.passthrough.company_id",
            FT_UINT24, BASE_HEX, VALS(oui_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_unit_unknown,
            { "Unknown",                         "btavrcp.unit.unknown",
            FT_UINT8, BASE_HEX, NULL, 0xFF,
            NULL, HFILL }
        },
        { &hf_btavrcp_unit_type,
            { "Unit Type",                       "btavrcp.unit.type",
            FT_UINT8, BASE_HEX, VALS(subunit_type_vals), 0xF8,
            NULL, HFILL }
        },
        { &hf_btavrcp_unit_id,
            { "Subunit ID",                      "btavrcp.unit.id",
            FT_UINT8, BASE_HEX, NULL, 0x07,
            NULL, HFILL }
        },
        { &hf_btavrcp_subunit_page,
            { "Page",                            "btavrcp.subunit.page",
            FT_UINT8, BASE_HEX, VALS(subunit_type_vals), 0xF8,
            NULL, HFILL }
        },
        { &hf_btavrcp_subunit_extension_code,
            { "Extension Code",                  "btavrcp.subunit.extension_code",
            FT_UINT8, BASE_HEX, NULL, 0x07,
            NULL, HFILL }
        },
        { &hf_btavrcp_bt_pdu_id,
            { "PDU ID",                          "btavrcp.pdu_id",
            FT_UINT8, BASE_HEX, VALS(pdu_id_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_pdu_id,
            { "PDU ID",                          "btavrcp.pdu_id",
            FT_UINT8, BASE_HEX, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_bt_continuing_pdu_id,
            { "Continuing PDU ID",               "btavrcp.pdu_id",
            FT_UINT8, BASE_HEX, VALS(pdu_id_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_browsing_pdu_id,
            { "PDU ID",                          "btavrcp.pdu_id",
            FT_UINT8, BASE_HEX, VALS(browsing_pdu_id_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_rfa,
            { "RFA",                             "btavrcp.rfa",
            FT_UINT8, BASE_HEX, NULL, 0xFC,
            NULL, HFILL }
        },
        { &hf_btavrcp_packet_type,
            { "Packet Type",                     "btavrcp.packet_type",
            FT_UINT8, BASE_HEX, VALS(packet_type_vals), 0x03,
            NULL, HFILL }
        },
        { &hf_btavrcp_length,
            { "Parameter Length",                "btavrcp.length",
            FT_UINT16, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_player_id,
            { "Player ID",                       "btavrcp.player_id",
            FT_UINT16, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_status,
            { "Status",                          "btavrcp.status",
            FT_UINT8, BASE_HEX, VALS(status_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_uid_counter,
            { "UID Counter",                     "btavrcp.uid_counter",
            FT_UINT16, BASE_HEX, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_character_set,
            { "Character Set",                   "btavrcp.character_set",
            FT_UINT16, BASE_DEC|BASE_EXT_STRING, &mibenum_vals_character_sets_ext, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_number_of_items,
            { "Number Of Items",                 "btavrcp.number_of_items",
            FT_UINT32, BASE_DEC, NULL, 0x00,
            "32bit number of items", HFILL }
        },
        { &hf_btavrcp_number_of_items16,
            { "Number Of Items",                 "btavrcp.number_of_items",
            FT_UINT16, BASE_DEC, NULL, 0x00,
            "16bit number of items", HFILL }
        },
        { &hf_btavrcp_folder_depth,
            { "Folder Depth",                    "btavrcp.folder_depth",
            FT_UINT8, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_folder_name_length,
            { "Folder Name Length",              "btavrcp.folder_name_length",
            FT_UINT16, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_folder_name,
            { "Folder Name",                    "btavrcp.folder_name",
            FT_NONE, BASE_NONE, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_search_length,
            { "Search String Length",            "btavrcp.search_length",
            FT_UINT16, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_search,
            { "Search String",                   "btavrcp.search",
            FT_NONE, BASE_NONE, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_number_of_attributes,
            { "Number of Attributes",            "btavrcp.number_of_attributes",
            FT_UINT8, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_attribute_count,
            { "Attribute Count",                 "btavrcp.attribute_count",
            FT_UINT8, BASE_DEC, VALS(attribute_count_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_direction,
            { "Direction",                       "btavrcp.direction",
            FT_UINT8, BASE_HEX, VALS(direction_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_scope,
            { "Scope",                           "btavrcp.scope",
            FT_UINT8, BASE_HEX, VALS(scope_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_start_item,
            { "StartItem",                       "btavrcp.start_item",
            FT_UINT32, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_end_item,
            { "EndItem",                         "btavrcp.end_item",
            FT_UINT32, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_uid,
            { "UID",                             "btavrcp.uid",
            FT_UINT64, BASE_HEX, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_identifier,
            { "Identifier",                      "btavrcp.identifier",
            FT_UINT64, BASE_HEX, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_play_status,
            { "Play Status",                     "btavrcp.play_status",
            FT_UINT8, BASE_HEX, VALS(play_status_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_song_length,
            { "Song Length",                     "btavrcp.song_length",
            FT_UINT32, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_song_position,
            { "Song Position",                   "btavrcp.song_position",
            FT_UINT32, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_notification_interval,
            { "Interval",                        "btavrcp.notification.interval",
            FT_UINT32, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_event_id,
            { "Event ID",                        "btavrcp.notification.event_id",
            FT_UINT8, BASE_HEX, VALS(notification_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_battery_status,
            { "Battery Status",                  "btavrcp.battery_status",
            FT_UINT8, BASE_HEX, VALS(battery_status_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_number_of_character_set,
            { "Number of Character Set",         "btavrcp.number_of_character_set",
            FT_UINT8, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_absolute_volume_rfa,
            { "RFA",                             "btavrcp.absoluter_volume_rfa",
            FT_UINT8, BASE_HEX, NULL, 0x80,
            NULL, HFILL }
        },
        { &hf_btavrcp_absolute_volume,
            { "Volume",                          "btavrcp.volume",
            FT_UINT8, BASE_HEX, NULL, 0x7F,
            NULL, HFILL }
        },
        { &hf_btavrcp_capability,
            { "Capability",                      "btavrcp.capability",
            FT_UINT8, BASE_HEX, VALS(capability_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_capability_count,
            { "Capability Count",                "btavrcp.capability.count",
            FT_UINT8, BASE_HEX, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_item_type,
            { "Item Type",                       "btavrcp.item.type",
            FT_UINT8, BASE_HEX, VALS(item_type_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_item_length,
            { "Item Length",                     "btavrcp.item.length",
            FT_UINT16, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },

        { &hf_btavrcp_number_of_settings,
            { "Number of Settings",              "btavrcp.number_of_settings",
            FT_UINT8, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_settings_attribute,
            { "Setting Attribute",               "btavrcp.settings.attribute",
            FT_UINT8, BASE_HEX, VALS(settings_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_settings_value,
            { "Setting Value",                   "btavrcp.settings.value",
            FT_UINT8, BASE_HEX, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_system_status,
            { "System Status",                   "btavrcp.system_status",
            FT_UINT8, BASE_HEX, VALS(system_status_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_player_item,
            { "Player",                          "btavrcp.player_item",
            FT_NONE, BASE_NONE, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_folder,
            { "Folder",                          "btavrcp.folder",
            FT_NONE, BASE_NONE, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_item,
            { "Item",                            "btavrcp.item",
            FT_NONE, BASE_NONE, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_features,
            { "Features",                        "btavrcp.features",
            FT_NONE, BASE_NONE, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_not_used_features,
            { "Not Used Features",               "btavrcp.not_used_features",
            FT_NONE, BASE_NONE, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_attribute_list,
            { "Attribute List",                       "btavrcp.attribute_list",
            FT_NONE, BASE_NONE, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_attribute_item,
            { "Attribute",                       "btavrcp.attribute_item",
            FT_NONE, BASE_NONE, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_attribute_entries,
            { "Attribute Entries",               "btavrcp.attribute_entries",
            FT_NONE, BASE_NONE, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_attribute,
            { "Attribute ID",                    "btavrcp.attribute",
            FT_UINT32, BASE_HEX, VALS(attribute_id_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_attribute_value_length,
            { "Value Length",                    "btavrcp.value.length",
            FT_UINT16, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_attribute_value,
            { "Value",                           "btavrcp.value",
            FT_STRING, BASE_NONE, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_displayable_name_length,
            { "Displayable Name Length",         "btavrcp.displayable_name_length",
            FT_UINT16, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_displayable_name,
            { "Displayable Name",                "btavrcp.displayable_name",
            FT_STRING, BASE_NONE, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_media_type,
            { "Media Type",                      "btavrcp.media_type",
            FT_UINT8, BASE_HEX, VALS(media_type_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_folder_type,
            { "Folder Type",                     "btavrcp.folder_type",
            FT_UINT8, BASE_HEX, VALS(folder_type_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_folder_playable,
            { "Folder Playable",                 "btavrcp.folder_playable",
            FT_UINT8, BASE_HEX, VALS(folder_playable_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_major_player_type,
            { "Major Player Type",               "btavrcp.major_player_type",
            FT_UINT8, BASE_HEX, VALS(major_player_type_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_player_subtype,
            { "Player SubType",                  "btavrcp.player_subtype",
            FT_UINT8, BASE_HEX, VALS(player_subtype_vals), 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_setting_value_length,
            { "Value Length",                    "btavrcp.setting_value.length",
            FT_UINT8, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_setting_value,
            { "Value",                           "btavrcp.setting_value",
            FT_STRING, BASE_NONE, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_attribute_name_length,
            { "Value Length",                    "btavrcp.attribute_name.length",
            FT_UINT8, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_attribute_name,
            { "Value",                           "btavrcp.attribute_name",
            FT_STRING, BASE_NONE, NULL, 0x00,
            NULL, HFILL }
        },
        /* features */
        { &hf_btavrcp_feature_reserved_0,
            { "Feature Reserved",                "btavrcp.feature.reserved",
            FT_UINT8, BASE_HEX, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_reserved_1,
            { "Feature Reserved",                "btavrcp.feature.reserved",
            FT_UINT8, BASE_HEX, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_reserved_2,
            { "Feature Reserved",                "btavrcp.feature.reserved",
            FT_UINT8, BASE_HEX, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_reserved_3,
            { "Feature Reserved",                "btavrcp.feature.reserved",
            FT_UINT8, BASE_HEX, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_reserved_4,
            { "Feature Reserved",                "btavrcp.feature.reserved",
            FT_UINT8, BASE_HEX, NULL, 0x10,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_reserved_5,
            { "Feature Reserved",                "btavrcp.feature.reserved",
            FT_UINT8, BASE_HEX, NULL, 0x20,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_reserved_6,
            { "Feature Reserved",                "btavrcp.feature.reserved",
            FT_UINT8, BASE_HEX, NULL, 0x40,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_reserved_7,
            { "Feature Reserved",                "btavrcp.feature.reserved",
            FT_UINT8, BASE_HEX, NULL, 0x80,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_select,
            { "PASSTHROUGH Select",              "btavrcp.feature.passthrough.select",
            FT_UINT8, BASE_HEX, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_up,
            { "PASSTHROUGH Up",                  "btavrcp.feature.passthrough.up",
            FT_UINT8, BASE_HEX, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_down,
            { "PASSTHROUGH Down",                "btavrcp.feature.passthrough.down",
            FT_UINT8, BASE_HEX, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_left,
            { "PASSTHROUGH Left",                "btavrcp.feature.passthrough.left",
            FT_UINT8, BASE_HEX, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_right,
            { "PASSTHROUGH Right",               "btavrcp.feature.passthrough.right",
            FT_UINT8, BASE_HEX, NULL, 0x10,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_right_up,
            { "PASSTHROUGH Right Up",            "btavrcp.feature.passthrough.right_up",
            FT_UINT8, BASE_HEX, NULL, 0x20,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_right_down,
            { "PASSTHROUGH Right Down",          "btavrcp.feature.passthrough.right_down",
            FT_UINT8, BASE_HEX, NULL, 0x40,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_left_up,
            { "PASSTHROUGH Left Up",             "btavrcp.feature.passthrough.left_up",
            FT_UINT8, BASE_HEX, NULL, 0x80,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_left_down,
            { "PASSTHROUGH Left Down",           "btavrcp.feature.passthrough.left_down",
            FT_UINT8, BASE_HEX, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_root_menu,
            { "PASSTHROUGH Root Menu",           "btavrcp.feature.passthrough.root_menu",
            FT_UINT8, BASE_HEX, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_setup_menu,
            { "PASSTHROUGH Setup Menu",          "btavrcp.feature.passthrough.setup_menu",
            FT_UINT8, BASE_HEX, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_contents_menu,
            { "PASSTHROUGH Contents Menu",       "btavrcp.feature.passthrough.contents_menu",
            FT_UINT8, BASE_HEX, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_favorite_menu,
            { "PASSTHROUGH Favorite Menu",       "btavrcp.feature.passthrough.favorite_menu",
            FT_UINT8, BASE_HEX, NULL, 0x10,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_exit,
            { "PASSTHROUGH Exit",                "btavrcp.feature.passthrough.exit",
            FT_UINT8, BASE_HEX, NULL, 0x20,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_0,
            { "PASSTHROUGH 0",                   "btavrcp.feature.passthrough.0",
            FT_UINT8, BASE_HEX, NULL, 0x40,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_1,
            { "PASSTHROUGH 1",                   "btavrcp.feature.passthrough.1",
            FT_UINT8, BASE_HEX, NULL, 0x80,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_2,
            { "PASSTHROUGH 2",                   "btavrcp.feature.passthrough.2",
            FT_UINT8, BASE_HEX, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_3,
            { "PASSTHROUGH 3",                   "btavrcp.feature.passthrough.3",
            FT_UINT8, BASE_HEX, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_4,
            { "PASSTHROUGH 4",                   "btavrcp.feature.passthrough.4",
            FT_UINT8, BASE_HEX, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_5,
            { "PASSTHROUGH 5",                   "btavrcp.feature.passthrough.5",
            FT_UINT8, BASE_HEX, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_6,
            { "PASSTHROUGH 6",                   "btavrcp.feature.passthrough.6",
            FT_UINT8, BASE_HEX, NULL, 0x10,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_7,
            { "PASSTHROUGH 7",                   "btavrcp.feature.passthrough.7",
            FT_UINT8, BASE_HEX, NULL, 0x20,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_8,
            { "PASSTHROUGH 8",                   "btavrcp.feature.passthrough.8",
            FT_UINT8, BASE_HEX, NULL, 0x40,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_9,
            { "PASSTHROUGH 9",                   "btavrcp.feature.passthrough.9",
            FT_UINT8, BASE_HEX, NULL, 0x80,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_dot,
            { "PASSTHROUGH Dot",                 "btavrcp.feature.passthrough.dot",
            FT_UINT8, BASE_HEX, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_enter,
            { "PASSTHROUGH Enter",               "btavrcp.feature.passthrough.enter",
            FT_UINT8, BASE_HEX, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_clear,
            { "PASSTHROUGH Clear",               "btavrcp.feature.passthrough.clear",
            FT_UINT8, BASE_HEX, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_channel_up,
            { "PASSTHROUGH Channel Up",          "btavrcp.feature.passthrough.channel_up",
            FT_UINT8, BASE_HEX, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_channel_down,
            { "PASSTHROUGH Channel Down",        "btavrcp.feature.passthrough.channel_down",
            FT_UINT8, BASE_HEX, NULL, 0x10,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_previous_channel,
            { "PASSTHROUGH Previous Channel",    "btavrcp.feature.passthrough.previous_channel",
            FT_UINT8, BASE_HEX, NULL, 0x20,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_sound_select,
            { "PASSTHROUGH Sound Select",        "btavrcp.feature.passthrough.sound_select",
            FT_UINT8, BASE_HEX, NULL, 0x40,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_input_select,
            { "PASSTHROUGH Input Select",        "btavrcp.feature.passthrough.input_select",
            FT_UINT8, BASE_HEX, NULL, 0x80,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_display_information,
            { "PASSTHROUGH Display Information", "btavrcp.feature.passthrough.display_information",
            FT_UINT8, BASE_HEX, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_help,
            { "PASSTHROUGH Help",                "btavrcp.feature.passthrough.help",
            FT_UINT8, BASE_HEX, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_page_up,
            { "PASSTHROUGH Page Up",             "btavrcp.feature.passthrough.page_up",
            FT_UINT8, BASE_HEX, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_page_down,
            { "PASSTHROUGH Page Down",           "btavrcp.feature.passthrough.page_down",
            FT_UINT8, BASE_HEX, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_power,
            { "PASSTHROUGH Power",               "btavrcp.feature.passthrough.power",
            FT_UINT8, BASE_HEX, NULL, 0x10,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_volume_up,
            { "PASSTHROUGH Volume Up",           "btavrcp.feature.passthrough.volume_up",
            FT_UINT8, BASE_HEX, NULL, 0x20,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_volume_down,
            { "PASSTHROUGH Volume Down",         "btavrcp.feature.passthrough.volume_down",
            FT_UINT8, BASE_HEX, NULL, 0x40,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_mute,
            { "PASSTHROUGH Mute",                "btavrcp.feature.passthrough.mute",
            FT_UINT8, BASE_HEX, NULL, 0x80,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_play,
            { "PASSTHROUGH Play",                "btavrcp.feature.passthrough.play",
            FT_UINT8, BASE_HEX, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_stop,
            { "PASSTHROUGH Stop",                "btavrcp.feature.passthrough.stop",
            FT_UINT8, BASE_HEX, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_pause,
            { "PASSTHROUGH Pause",               "btavrcp.feature.passthrough.pause",
            FT_UINT8, BASE_HEX, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_record,
            { "PASSTHROUGH Record",              "btavrcp.feature.passthrough.record",
            FT_UINT8, BASE_HEX, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_rewind,
            { "PASSTHROUGH Rewind",              "btavrcp.feature.passthrough.rewind",
            FT_UINT8, BASE_HEX, NULL, 0x10,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_fast_forward,
            { "PASSTHROUGH FastForward",         "btavrcp.feature.passthrough.fast_forward",
            FT_UINT8, BASE_HEX, NULL, 0x20,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_eject,
            { "PASSTHROUGH Eject",               "btavrcp.feature.passthrough.eject",
            FT_UINT8, BASE_HEX, NULL, 0x40,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_forward,
            { "PASSTHROUGH Forward",             "btavrcp.feature.passthrough.forward",
            FT_UINT8, BASE_HEX, NULL, 0x80,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_backward,
            { "PASSTHROUGH Backward",            "btavrcp.feature.passthrough.backward",
            FT_UINT8, BASE_HEX, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_angle,
            { "PASSTHROUGH Angle",               "btavrcp.feature.passthrough.angle",
            FT_UINT8, BASE_HEX, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_subpicture,
            { "PASSTHROUGH SubPicture",          "btavrcp.feature.passthrough.subpicture",
            FT_UINT8, BASE_HEX, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_f1,
            { "PASSTHROUGH F1",                  "btavrcp.feature.passthrough.f1",
            FT_UINT8, BASE_HEX, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_f2,
            { "PASSTHROUGH F2",                  "btavrcp.feature.passthrough.f2",
            FT_UINT8, BASE_HEX, NULL, 0x10,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_f3,
            { "PASSTHROUGH F3",                  "btavrcp.feature.passthrough.f3",
            FT_UINT8, BASE_HEX, NULL, 0x20,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_f4,
            { "PASSTHROUGH F4",                  "btavrcp.feature.passthrough.f4",
            FT_UINT8, BASE_HEX, NULL, 0x40,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_passthrough_f5,
            { "PASSTHROUGH F5",                  "btavrcp.feature.passthrough.f5",
            FT_UINT8, BASE_HEX, NULL, 0x80,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_vendor_unique,
            { "Vendor Unique",                   "btavrcp.feature.vendor_unique",
            FT_UINT8, BASE_HEX, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_basic_group_navigation,
            { "Basic Group Navigation",          "btavrcp.feature.basic_group_navigation",
            FT_UINT8, BASE_HEX, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_advanced_control_player,
            { "Advanced Control Player",         "btavrcp.feature.advanced_control_player",
            FT_UINT8, BASE_HEX, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_browsing,
            { "Browsing",                        "btavrcp.feature.browsing",
            FT_UINT8, BASE_HEX, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_searching,
            { "Searching",                       "btavrcp.feature.searching",
            FT_UINT8, BASE_HEX, NULL, 0x10,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_addtonowplayer,
            { "AddToNowPlaying",                 "btavrcp.feature.addtonowplaying",
            FT_UINT8, BASE_HEX, NULL, 0x20,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_uid_unique,
            { "UID Unique",                      "btavrcp.feature.uid_unique",
            FT_UINT8, BASE_HEX, NULL, 0x40,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_only_browsable_when_addressed,
            { "Only Browsable When Addressed",   "btavrcp.feature.only_browsable_when_addressed",
            FT_UINT8, BASE_HEX, NULL, 0x80,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_only_searchable_when_addressed,
            { "Only Searchable When Addressed",  "btavrcp.feature.only_searchable_when_addressed",
            FT_UINT8, BASE_HEX, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_nowplaying,
            { "Nowplaying",                      "btavrcp.feature.nowplaying",
            FT_UINT8, BASE_HEX, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_btavrcp_feature_uid_persistency,
            { "UID Persistency",                 "btavrcp.feature.uid_persistency",
            FT_UINT8, BASE_HEX, NULL, 0x04,
            NULL, HFILL }
        },
        /* end of features */
        { &hf_btavrcp_currect_path,
            { "Currect Path",                     "btavrcp.currect_path",
            FT_NONE, BASE_NONE, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_reassembled,
            { "Reassembled",                     "btavrcp.reassembled",
            FT_NONE, BASE_NONE, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_response_time,
            { "Response Time",                   "btavrcp.response_time",
            FT_UINT32, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_command_in_frame,
            { "Command in frame",                "btavrcp.command_in_frame",
            FT_FRAMENUM, BASE_NONE, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_response_in_frame,
            { "Response in frame",               "btavrcp.response_in_frame",
            FT_FRAMENUM, BASE_NONE, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_btavrcp_data,
            { "Data",                            "btavrcp.data",
            FT_NONE, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        }
    };

    static gint *ett[] = {
        &ett_btavrcp,
        &ett_btavrcp_attribute_list,
        &ett_btavrcp_attribute_entry,
        &ett_btavrcp_attribute_entries,
        &ett_btavrcp_element,
        &ett_btavrcp_player,
        &ett_btavrcp_folder,
        &ett_btavrcp_path,
    };

    static ei_register_info ei[] = {
        { &ei_btavrcp_item_length_bad, { "btavrcp.item.length.bad", PI_PROTOCOL, PI_WARN, "Item length does not correspond to sum of length of attributes", EXPFILL }},
        { &ei_btavrcp_unexpected_data, { "btavrcp.unexpected_data", PI_PROTOCOL, PI_WARN, "Unexpected data", EXPFILL }},
        { &ei_btavrcp_no_response,     { "btavrcp.no_response",     PI_PROTOCOL, PI_WARN, "No response", EXPFILL }},
    };

    reassembling   = wmem_tree_new_autoreset(wmem_epan_scope(), wmem_file_scope());
    timing         = wmem_tree_new_autoreset(wmem_epan_scope(), wmem_file_scope());
    btavrcp_song_positions = wmem_tree_new_autoreset(wmem_epan_scope(), wmem_file_scope());

    proto_btavrcp = proto_register_protocol("Bluetooth AVRCP Profile", "BT AVRCP", "btavrcp");
    btavrcp_handle = register_dissector("btavrcp", dissect_btavrcp, proto_btavrcp);

    proto_register_field_array(proto_btavrcp, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
    expert_btavrcp = expert_register_protocol(proto_btavrcp);
    expert_register_field_array(expert_btavrcp, ei, array_length(ei));

    module = prefs_register_protocol(proto_btavrcp, NULL);
    prefs_register_static_text_preference(module, "avrcp.version",
            "Bluetooth Profile AVRCP version: 1.5",
            "Version of profile supported by this dissector.");
}

void
proto_reg_handoff_btavrcp(void)
{
    dissector_add_string("bluetooth.uuid", "110c", btavrcp_handle);
    dissector_add_string("bluetooth.uuid", "110e", btavrcp_handle);
    dissector_add_string("bluetooth.uuid", "110f", btavrcp_handle);
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
