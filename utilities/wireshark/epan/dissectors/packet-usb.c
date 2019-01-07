/* packet-usb.c
 *
 * USB basic dissector
 * By Paolo Abeni <paolo.abeni@email.it>
 * Ronnie Sahlberg 2006
 *
 * http://www.usb.org/developers/docs/usb_20_122909-2.zip
 *
 * https://github.com/torvalds/linux/blob/master/Documentation/usb/usbmon.txt
 *
 * http://desowin.org/usbpcap/captureformat.html
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
#include <epan/exceptions.h>
#include <epan/addr_resolv.h>
#include <epan/address_types.h>
#include <epan/conversation_table.h>
#include <epan/expert.h>
#include <epan/prefs.h>
#include <epan/decode_as.h>
#include <epan/proto_data.h>
#include <wsutil/pint.h>

#include "packet-usb.h"
#include "packet-mausb.h"
#include "packet-usbip.h"

/* protocols and header fields */
static int proto_usb = -1;

/* USB pseudoheader fields, both FreeBSD and Linux */
static int hf_usb_totlen = -1;
static int hf_usb_busunit = -1;
static int hf_usb_address = -1;
static int hf_usb_mode = -1;
static int hf_usb_freebsd_urb_type = -1;
static int hf_usb_freebsd_transfer_type = -1;
static int hf_usb_xferflags = -1;
static int hf_usb_xferflags_force_short_xfer = -1;
static int hf_usb_xferflags_short_xfer_ok = -1;
static int hf_usb_xferflags_short_frames_ok = -1;
static int hf_usb_xferflags_pipe_bof = -1;
static int hf_usb_xferflags_proxy_buffer = -1;
static int hf_usb_xferflags_ext_buffer = -1;
static int hf_usb_xferflags_manual_status = -1;
static int hf_usb_xferflags_no_pipe_ok = -1;
static int hf_usb_xferflags_stall_pipe = -1;
static int hf_usb_xferstatus = -1;
static int hf_usb_xferstatus_open = -1;
static int hf_usb_xferstatus_transferring = -1;
static int hf_usb_xferstatus_did_dma_delay = -1;
static int hf_usb_xferstatus_did_close = -1;
static int hf_usb_xferstatus_draining = -1;
static int hf_usb_xferstatus_started = -1;
static int hf_usb_xferstatus_bw_reclaimed = -1;
static int hf_usb_xferstatus_control_xfr = -1;
static int hf_usb_xferstatus_control_hdr = -1;
static int hf_usb_xferstatus_control_act = -1;
static int hf_usb_xferstatus_control_stall = -1;
static int hf_usb_xferstatus_short_frames_ok = -1;
static int hf_usb_xferstatus_short_xfer_ok = -1;
static int hf_usb_xferstatus_bdma_enable = -1;
static int hf_usb_xferstatus_bdma_no_post_sync = -1;
static int hf_usb_xferstatus_bdma_setup = -1;
static int hf_usb_xferstatus_isochronous_xfr = -1;
static int hf_usb_xferstatus_curr_dma_set = -1;
static int hf_usb_xferstatus_can_cancel_immed = -1;
static int hf_usb_xferstatus_doing_callback = -1;
static int hf_usb_error = -1;
static int hf_usb_interval = -1;
static int hf_usb_nframes = -1;
static int hf_usb_packet_size = -1;
static int hf_usb_packet_count = -1;
static int hf_usb_speed = -1;
static int hf_usb_frame_length = -1;
static int hf_usb_frame_flags = -1;
static int hf_usb_frame_flags_read = -1;
static int hf_usb_frame_flags_data_follows = -1;
static int hf_usb_frame_data = -1;
static int hf_usb_urb_id = -1;
static int hf_usb_linux_urb_type = -1;
static int hf_usb_linux_transfer_type = -1;
static int hf_usb_endpoint_number = -1;
static int hf_usb_endpoint_direction = -1;
static int hf_usb_endpoint_number_value = -1;
static int hf_usb_device_address = -1;
static int hf_usb_bus_id = -1;
static int hf_usb_setup_flag = -1;
static int hf_usb_data_flag = -1;
static int hf_usb_urb_ts_sec = -1;
static int hf_usb_urb_ts_usec = -1;
static int hf_usb_urb_status = -1;
static int hf_usb_urb_len = -1;
static int hf_usb_urb_data_len = -1;
static int hf_usb_urb_unused_setup_header = -1;
static int hf_usb_urb_interval = -1;
static int hf_usb_urb_start_frame = -1;
static int hf_usb_urb_copy_of_transfer_flags = -1;

/* Win32 USBPcap pseudoheader fields */
static int hf_usb_win32_header_len = -1;
static int hf_usb_irp_id = -1;
static int hf_usb_usbd_status = -1;
static int hf_usb_function = -1;
static int hf_usb_info = -1;
static int hf_usb_usbpcap_info_reserved = -1;
static int hf_usb_usbpcap_info_direction = -1;
static int hf_usb_win32_device_address = -1;
/* hf_usb_bus_id, hf_usb_endpoint_number, hf_usb_endpoint_direction,
 * hf_usb_endpoint_number_value, hf_usb_transfer_type are common with
 * Linux pseudoheader */
static int hf_usb_win32_data_len = -1;
static int hf_usb_control_stage = -1;
static int hf_usb_win32_iso_start_frame = -1;
static int hf_usb_win32_iso_num_packets = -1;
static int hf_usb_win32_iso_error_count = -1;
static int hf_usb_win32_iso_offset = -1;
static int hf_usb_win32_iso_length = -1;
static int hf_usb_win32_iso_status = -1;

static int hf_usb_request = -1;
static int hf_usb_request_unknown_class = -1;
static int hf_usb_value = -1;
static int hf_usb_index = -1;
static int hf_usb_length = -1;
/* static int hf_usb_data_len = -1; */
static int hf_usb_capdata = -1;
static int hf_usb_device_wFeatureSelector = -1;
static int hf_usb_interface_wFeatureSelector = -1;
static int hf_usb_endpoint_wFeatureSelector = -1;
static int hf_usb_wInterface = -1;
static int hf_usb_wEndpoint = -1;
static int hf_usb_wStatus = -1;
static int hf_usb_wFrameNumber = -1;

static int hf_usb_iso_error_count = -1;
static int hf_usb_iso_numdesc = -1;
static int hf_usb_iso_status = -1;
static int hf_usb_iso_off = -1;
static int hf_usb_iso_len = -1;
static int hf_usb_iso_actual_len = -1;
static int hf_usb_iso_pad = -1;
static int hf_usb_iso_data = -1;

static int hf_usb_bmRequestType = -1;
static int hf_usb_control_response_generic = -1;
static int hf_usb_bmRequestType_direction = -1;
static int hf_usb_bmRequestType_type = -1;
static int hf_usb_bmRequestType_recipient = -1;
static int hf_usb_bDescriptorType = -1;
static int hf_usb_get_descriptor_resp_generic = -1;
static int hf_usb_descriptor_index = -1;
static int hf_usb_language_id = -1;
static int hf_usb_bLength = -1;
static int hf_usb_bcdUSB = -1;
static int hf_usb_bDeviceClass = -1;
static int hf_usb_bDeviceSubClass = -1;
static int hf_usb_bDeviceProtocol = -1;
static int hf_usb_bMaxPacketSize0 = -1;
static int hf_usb_idVendor = -1;
static int hf_usb_idProduct = -1;
static int hf_usb_bcdDevice = -1;
static int hf_usb_iManufacturer = -1;
static int hf_usb_iProduct = -1;
static int hf_usb_iSerialNumber = -1;
static int hf_usb_bNumConfigurations = -1;
static int hf_usb_wLANGID = -1;
static int hf_usb_bString = -1;
static int hf_usb_bInterfaceNumber = -1;
static int hf_usb_bAlternateSetting = -1;
static int hf_usb_bNumEndpoints = -1;
static int hf_usb_bInterfaceClass = -1;
static int hf_usb_bInterfaceSubClass = -1;
static int hf_usb_bInterfaceSubClass_cdc = -1;
static int hf_usb_bInterfaceSubClass_hid = -1;
static int hf_usb_bInterfaceSubClass_misc = -1;
static int hf_usb_bInterfaceSubClass_app = -1;
static int hf_usb_bInterfaceProtocol = -1;
static int hf_usb_bInterfaceProtocol_cdc = -1;
static int hf_usb_bInterfaceProtocol_cdc_data = -1;
static int hf_usb_bInterfaceProtocol_hid_boot = -1;
static int hf_usb_bInterfaceProtocol_app_dfu = -1;
static int hf_usb_bInterfaceProtocol_app_irda = -1;
static int hf_usb_bInterfaceProtocol_app_usb_test_and_measurement = -1;
static int hf_usb_iInterface = -1;
static int hf_usb_bEndpointAddress = -1;
static int hf_usb_bmAttributes = -1;
static int hf_usb_bEndpointAttributeTransfer = -1;
static int hf_usb_bEndpointAttributeSynchonisation = -1;
static int hf_usb_bEndpointAttributeBehaviour = -1;
static int hf_usb_wMaxPacketSize = -1;
static int hf_usb_wMaxPacketSize_size = -1;
static int hf_usb_wMaxPacketSize_slots = -1;
static int hf_usb_bInterval = -1;
static int hf_usb_wTotalLength = -1;
static int hf_usb_bNumInterfaces = -1;
static int hf_usb_bConfigurationValue = -1;
static int hf_usb_iConfiguration = -1;
static int hf_usb_bMaxPower = -1;
static int hf_usb_configuration_bmAttributes = -1;
static int hf_usb_configuration_legacy10buspowered = -1;
static int hf_usb_configuration_selfpowered = -1;
static int hf_usb_configuration_remotewakeup = -1;
static int hf_usb_bEndpointAddress_direction = -1;
static int hf_usb_bEndpointAddress_number = -1;
static int hf_usb_response_in = -1;
static int hf_usb_time = -1;
static int hf_usb_request_in = -1;
static int hf_usb_bFirstInterface = -1;
static int hf_usb_bInterfaceCount = -1;
static int hf_usb_bFunctionClass = -1;
static int hf_usb_bFunctionSubClass = -1;
static int hf_usb_bFunctionProtocol = -1;
static int hf_usb_iFunction = -1;
static int hf_usb_data_fragment = -1;
static int hf_usb_src = -1;
static int hf_usb_dst = -1;
static int hf_usb_addr = -1;

static gint ett_usb_hdr = -1;
static gint ett_usb_setup_hdr = -1;
static gint ett_usb_isodesc = -1;
static gint ett_usb_win32_iso_packet = -1;
static gint ett_usb_endpoint = -1;
static gint ett_usb_setup_bmrequesttype = -1;
static gint ett_usb_usbpcap_info = -1;
static gint ett_descriptor_device = -1;
static gint ett_configuration_bmAttributes = -1;
static gint ett_configuration_bEndpointAddress = -1;
static gint ett_endpoint_bmAttributes = -1;
static gint ett_endpoint_wMaxPacketSize = -1;
static gint ett_usb_xferflags = -1;
static gint ett_usb_xferstatus = -1;
static gint ett_usb_frame = -1;
static gint ett_usb_frame_flags = -1;

static expert_field ei_usb_bLength_even = EI_INIT;
static expert_field ei_usb_bLength_too_short = EI_INIT;
static expert_field ei_usb_desc_length_invalid = EI_INIT;
static expert_field ei_usb_invalid_setup = EI_INIT;

static int usb_address_type = -1;

static const int *usb_endpoint_fields[] = {
    &hf_usb_endpoint_direction,
    &hf_usb_endpoint_number_value,
    NULL
};

static const int *usb_usbpcap_info_fields[] = {
    &hf_usb_usbpcap_info_reserved,
    &hf_usb_usbpcap_info_direction,
    NULL
};

static int usb_tap = -1;
static gboolean try_heuristics = TRUE;

static dissector_table_t usb_bulk_dissector_table;
static dissector_table_t usb_control_dissector_table;
static dissector_table_t usb_interrupt_dissector_table;
static dissector_table_t usb_descriptor_dissector_table;

static heur_dissector_list_t heur_bulk_subdissector_list;
static heur_dissector_list_t heur_control_subdissector_list;
static heur_dissector_list_t heur_interrupt_subdissector_list;

static wmem_tree_t *device_to_protocol_table = NULL;
static wmem_tree_t *device_to_product_table  = NULL;

static dissector_table_t device_to_dissector;
static dissector_table_t protocol_to_dissector;
static dissector_table_t product_to_dissector;

typedef struct _device_product_data_t {
    guint16  vendor;
    guint16  product;
    guint  bus_id;
    guint  device_address;
} device_product_data_t;

typedef struct _device_protocol_data_t {
    guint32  protocol;
    guint  bus_id;
    guint  device_address;
} device_protocol_data_t;

typedef struct _usb_alt_setting_t {
    guint8 altSetting;
    guint8 interfaceClass;
    guint8 interfaceSubclass;
    guint8 interfaceProtocol;
} usb_alt_setting_t;

/* http://www.usb.org/developers/docs/USB_LANGIDs.pdf */
static const value_string usb_langid_vals[] = {
    {0x0000, "no language specified"},
    {0x0401, "Arabic (Saudi Arabia)"},
    {0x0402, "Bulgarian"},
    {0x0403, "Catalan"},
    {0x0404, "Chinese (Taiwan)"},
    {0x0405, "Czech"},
    {0x0406, "Danish"},
    {0x0407, "German (Standard)"},
    {0x0408, "Greek"},
    {0x0409, "English (United States)"},
    {0x040a, "Spanish (Traditional Sort)"},
    {0x040b, "Finnish"},
    {0x040c, "French (Standard)"},
    {0x040d, "Hebrew"},
    {0x040e, "Hungarian"},
    {0x040f, "Icelandic"},
    {0x0410, "Italian (Standard)"},
    {0x0411, "Japanese"},
    {0x0412, "Korean"},
    {0x0413, "Dutch (Netherlands)"},
    {0x0414, "Norwegian (Bokmal)"},
    {0x0415, "Polish"},
    {0x0416, "Portuguese (Brazil)"},
    {0x0418, "Romanian"},
    {0x0419, "Russian"},
    {0x041a, "Croatian"},
    {0x041b, "Slovak"},
    {0x041c, "Albanian"},
    {0x041d, "Swedish"},
    {0x041e, "Thai"},
    {0x041f, "Turkish"},
    {0x0420, "Urdu (Pakistan)"},
    {0x0421, "Indonesian"},
    {0x0422, "Ukrainian"},
    {0x0423, "Belarussian"},
    {0x0424, "Slovenian"},
    {0x0425, "Estonian"},
    {0x0426, "Latvian"},
    {0x0427, "Lithuanian"},
    {0x0429, "Farsi"},
    {0x042a, "Vietnamese"},
    {0x042b, "Armenian"},
    {0x042c, "Azeri (Latin)"},
    {0x042d, "Basque"},
    {0x042f, "Macedonian"},
    {0x0430, "Sutu"},
    {0x0436, "Afrikaans"},
    {0x0437, "Georgian"},
    {0x0438, "Faeroese"},
    {0x0439, "Hindi"},
    {0x043e, "Malay (Malaysian)"},
    {0x043f, "Kazakh"},
    {0x0441, "Swahili (Kenya)"},
    {0x0443, "Uzbek (Latin)"},
    {0x0444, "Tatar (Tatarstan)"},
    {0x0445, "Bengali"},
    {0x0446, "Punjabi"},
    {0x0447, "Gujarati"},
    {0x0448, "Oriya"},
    {0x0449, "Tamil"},
    {0x044a, "Telugu"},
    {0x044b, "Kannada"},
    {0x044c, "Malayalam"},
    {0x044d, "Assamese"},
    {0x044e, "Marathi"},
    {0x044f, "Sanskrit"},
    {0x0455, "Burmese"},
    {0x0457, "Konkani"},
    {0x0458, "Manipuri"},
    {0x0459, "Sindhi"},
    {0x04ff, "HID (Usage Data Descriptor)"},
    {0x0801, "Arabic (Iraq)"},
    {0x0804, "Chinese (PRC)"},
    {0x0807, "German (Switzerland)"},
    {0x0809, "English (United Kingdom)"},
    {0x080a, "Spanish (Mexican)"},
    {0x080c, "French (Belgian)"},
    {0x0810, "Italian (Switzerland)"},
    {0x0812, "Korean (Johab)"},
    {0x0813, "Dutch (Belgium)"},
    {0x0814, "Norwegian (Nynorsk)"},
    {0x0816, "Portuguese (Standard)"},
    {0x081a, "Serbian (Latin)"},
    {0x081d, "Swedish (Finland)"},
    {0x0820, "Urdu (India)"},
    {0x0827, "Lithuanian (Classic)"},
    {0x082c, "Azeri (Cyrillic)"},
    {0x083e, "Malay (Brunei Darussalam)"},
    {0x0843, "Uzbek (Cyrillic)"},
    {0x0860, "Kashmiri (India)"},
    {0x0861, "Nepali (India)"},
    {0x0c01, "Arabic (Egypt)"},
    {0x0c04, "Chinese (Hong Kong SAR, PRC)"},
    {0x0c07, "German (Austria)"},
    {0x0c09, "English (Australian)"},
    {0x0c0a, "Spanish (Modern Sort)"},
    {0x0c0c, "French (Canadian)"},
    {0x0c1a, "Serbian (Cyrillic)"},
    {0x1001, "Arabic (Libya)"},
    {0x1004, "Chinese (Singapore)"},
    {0x1007, "German (Luxembourg)"},
    {0x1009, "English (Canadian)"},
    {0x100a, "Spanish (Guatemala)"},
    {0x100c, "French (Switzerland)"},
    {0x1401, "Arabic (Algeria)"},
    {0x1404, "Chinese (Macau SAR)"},
    {0x1407, "German (Liechtenstein)"},
    {0x1409, "English (New Zealand)"},
    {0x140a, "Spanish (Costa Rica)"},
    {0x140c, "French (Luxembourg)"},
    {0x1801, "Arabic (Morocco)"},
    {0x1809, "English (Ireland)"},
    {0x180a, "Spanish (Panama)"},
    {0x180c, "French (Monaco)"},
    {0x1c01, "Arabic (Tunisia)"},
    {0x1c09, "English (South Africa)"},
    {0x1c0a, "Spanish (Dominican Republic)"},
    {0x2001, "Arabic (Oman)"},
    {0x2009, "English (Jamaica)"},
    {0x200a, "Spanish (Venezuela)"},
    {0x2401, "Arabic (Yemen)"},
    {0x2409, "English (Caribbean)"},
    {0x240a, "Spanish (Colombia)"},
    {0x2801, "Arabic (Syria)"},
    {0x2809, "English (Belize)"},
    {0x280a, "Spanish (Peru)"},
    {0x2c01, "Arabic (Jordan)"},
    {0x2c09, "English (Trinidad)"},
    {0x2c0a, "Spanish (Argentina)"},
    {0x3001, "Arabic (Lebanon)"},
    {0x3009, "English (Zimbabwe)"},
    {0x300a, "Spanish (Ecuador)"},
    {0x3401, "Arabic (Kuwait)"},
    {0x3409, "English (Philippines)"},
    {0x340a, "Spanish (Chile)"},
    {0x3801, "Arabic (U.A.E.)"},
    {0x380a, "Spanish (Uruguay)"},
    {0x3c01, "Arabic (Bahrain)"},
    {0x3c0a, "Spanish (Paraguay)"},
    {0x4001, "Arabic (Qatar)"},
    {0x400a, "Spanish (Bolivia)"},
    {0x440a, "Spanish (El Salvador)"},
    {0x480a, "Spanish (Honduras)"},
    {0x4c0a, "Spanish (Nicaragua)"},
    {0x500a, "Spanish (Puerto Rico)"},
    {0xf0ff, "HID (Vendor Defined 1)"},
    {0xf4ff, "HID (Vendor Defined 2)"},
    {0xf8ff, "HID (Vendor Defined 3)"},
    {0xfcff, "HID (Vendor Defined 4)"},
    {0, NULL}
};
value_string_ext usb_langid_vals_ext = VALUE_STRING_EXT_INIT(usb_langid_vals);

static const value_string usb_class_vals[] = {
    {IF_CLASS_DEVICE,                   "Device"},
    {IF_CLASS_AUDIO,                    "Audio"},
    {IF_CLASS_COMMUNICATIONS,           "Communications and CDC Control"},
    {IF_CLASS_HID,                      "HID"},
    {IF_CLASS_PHYSICAL,                 "Physical"},
    {IF_CLASS_IMAGE,                    "Imaging"},
    {IF_CLASS_PRINTER,                  "Printer"},
    {IF_CLASS_MASS_STORAGE,             "Mass Storage"},
    {IF_CLASS_HUB,                      "Hub"},
    {IF_CLASS_CDC_DATA,                 "CDC-Data"},
    {IF_CLASS_SMART_CARD,               "Smart Card"},
    {IF_CLASS_CONTENT_SECURITY,         "Content Security"},
    {IF_CLASS_VIDEO,                    "Video"},
    {IF_CLASS_PERSONAL_HEALTHCARE,      "Personal Healthcare"},
    {IF_CLASS_AUDIO_VIDEO,              "Audio/Video Devices"},
    {IF_CLASS_DIAGNOSTIC_DEVICE,        "Diagnostic Device"},
    {IF_CLASS_WIRELESS_CONTROLLER,      "Wireless Controller"},
    {IF_CLASS_MISCELLANEOUS,            "Miscellaneous"},
    {IF_CLASS_APPLICATION_SPECIFIC,     "Application Specific"},
    {IF_CLASS_VENDOR_SPECIFIC,          "Vendor Specific"},
    {0, NULL}
};
value_string_ext usb_class_vals_ext = VALUE_STRING_EXT_INIT(usb_class_vals);

/* use usb class, subclass and protocol id together
  http://www.usb.org/developers/defined_class
  USB Class Definitions for Communications Devices, Revision 1.2 December 6, 2012
*/
static const value_string usb_protocols[] = {
    {0x000000,    "Use class code info from Interface Descriptors"},
    {0x060101,    "Still Imaging"},
    {0x090000,    "Full speed Hub"},
    {0x090001,    "Hi-speed hub with single TT"},
    {0x090002,    "Hi-speed hub with multiple TTs"},
    {0x0D0000,    "Content Security"},
    {0x100100,    "AVControl Interface"},
    {0x100200,    "AVData Video Streaming Interface"},
    {0x100300,    "AVData Audio Streaming Interface"},
    {0xDC0101,    "USB2 Compliance Device"},
    {0xE00101,    "Bluetooth Programming Interface"},
    {0xE00102,    "UWB Radio Control Interface"},
    {0xE00103,    "Remote NDIS"},
    {0xE00104,    "Bluetooth AMP Controller"},
    {0xE00201,    "Host Wire Adapter Control/Data interface"},
    {0xE00202,    "Device Wire Adapter Control/Data interface"},
    {0xE00203,    "Device Wire Adapter Isochronous interface"},
    {0xEF0101,    "Active Sync device"},
    {0xEF0102,    "Palm Sync"},
    {0xEF0201,    "Interface Association Descriptor"},
    {0xEF0202,    "Wire Adapter Multifunction Peripheral programming interface"},
    {0xEF0301,    "Cable Based Association Framework"},
    {0xFE0101,    "Device Firmware Upgrade"},
    {0xFE0200,    "IRDA Bridge device"},
    {0xFE0300,    "USB Test and Measurement Device"},
    {0xFE0301,    "USB Test and Measurement Device conforming to the USBTMC USB488"},
    {0, NULL}
};
static value_string_ext usb_protocols_ext = VALUE_STRING_EXT_INIT(usb_protocols);

/* FreeBSD header */

/* Transfer mode */
#define FREEBSD_MODE_HOST       0
#define FREEBSD_MODE_DEVICE     1
static const value_string usb_freebsd_transfer_mode_vals[] = {
    {FREEBSD_MODE_HOST,   "Host"},
    {FREEBSD_MODE_DEVICE, "Device"},
    {0, NULL}
};

/* Type */
#define FREEBSD_URB_SUBMIT   0
#define FREEBSD_URB_COMPLETE 1
static const value_string usb_freebsd_urb_type_vals[] = {
    {FREEBSD_URB_SUBMIT,   "URB_SUBMIT"},
    {FREEBSD_URB_COMPLETE, "URB_COMPLETE"},
    {0, NULL}
};

/* Transfer type */
#define FREEBSD_URB_CONTROL     0
#define FREEBSD_URB_ISOCHRONOUS 1
#define FREEBSD_URB_BULK        2
#define FREEBSD_URB_INTERRUPT   3

static const value_string usb_freebsd_transfer_type_vals[] = {
    {FREEBSD_URB_CONTROL,     "URB_CONTROL"},
    {FREEBSD_URB_ISOCHRONOUS, "URB_ISOCHRONOUS"},
    {FREEBSD_URB_BULK,        "URB_BULK"},
    {FREEBSD_URB_INTERRUPT,   "URB_INTERRUPT"},
    {0, NULL}
};

/* Transfer flags */
#define FREEBSD_FLAG_FORCE_SHORT_XFER 0x00000001
#define FREEBSD_FLAG_SHORT_XFER_OK    0x00000002
#define FREEBSD_FLAG_SHORT_FRAMES_OK  0x00000004
#define FREEBSD_FLAG_PIPE_BOF         0x00000008
#define FREEBSD_FLAG_PROXY_BUFFER     0x00000010
#define FREEBSD_FLAG_EXT_BUFFER       0x00000020
#define FREEBSD_FLAG_MANUAL_STATUS    0x00000040
#define FREEBSD_FLAG_NO_PIPE_OK       0x00000080
#define FREEBSD_FLAG_STALL_PIPE       0x00000100

static const int *usb_xferflags_fields[] = {
    &hf_usb_xferflags_force_short_xfer,
    &hf_usb_xferflags_short_xfer_ok,
    &hf_usb_xferflags_short_frames_ok,
    &hf_usb_xferflags_pipe_bof,
    &hf_usb_xferflags_proxy_buffer,
    &hf_usb_xferflags_ext_buffer,
    &hf_usb_xferflags_manual_status,
    &hf_usb_xferflags_no_pipe_ok,
    &hf_usb_xferflags_stall_pipe,
    NULL
};

/* Transfer status */
#define FREEBSD_STATUS_OPEN              0x00000001
#define FREEBSD_STATUS_TRANSFERRING      0x00000002
#define FREEBSD_STATUS_DID_DMA_DELAY     0x00000004
#define FREEBSD_STATUS_DID_CLOSE         0x00000008
#define FREEBSD_STATUS_DRAINING          0x00000010
#define FREEBSD_STATUS_STARTED           0x00000020
#define FREEBSD_STATUS_BW_RECLAIMED      0x00000040
#define FREEBSD_STATUS_CONTROL_XFR       0x00000080
#define FREEBSD_STATUS_CONTROL_HDR       0x00000100
#define FREEBSD_STATUS_CONTROL_ACT       0x00000200
#define FREEBSD_STATUS_CONTROL_STALL     0x00000400
#define FREEBSD_STATUS_SHORT_FRAMES_OK   0x00000800
#define FREEBSD_STATUS_SHORT_XFER_OK     0x00001000
#define FREEBSD_STATUS_BDMA_ENABLE       0x00002000
#define FREEBSD_STATUS_BDMA_NO_POST_SYNC 0x00004000
#define FREEBSD_STATUS_BDMA_SETUP        0x00008000
#define FREEBSD_STATUS_ISOCHRONOUS_XFR   0x00010000
#define FREEBSD_STATUS_CURR_DMA_SET      0x00020000
#define FREEBSD_STATUS_CAN_CANCEL_IMMED  0x00040000
#define FREEBSD_STATUS_DOING_CALLBACK    0x00080000

static const int *usb_xferstatus_fields[] = {
    &hf_usb_xferstatus_open,
    &hf_usb_xferstatus_transferring,
    &hf_usb_xferstatus_did_dma_delay,
    &hf_usb_xferstatus_did_close,
    &hf_usb_xferstatus_draining,
    &hf_usb_xferstatus_started,
    &hf_usb_xferstatus_bw_reclaimed,
    &hf_usb_xferstatus_control_xfr,
    &hf_usb_xferstatus_control_hdr,
    &hf_usb_xferstatus_control_act,
    &hf_usb_xferstatus_control_stall,
    &hf_usb_xferstatus_short_frames_ok,
    &hf_usb_xferstatus_short_xfer_ok,
    &hf_usb_xferstatus_bdma_enable,
    &hf_usb_xferstatus_bdma_no_post_sync,
    &hf_usb_xferstatus_bdma_setup,
    &hf_usb_xferstatus_isochronous_xfr,
    &hf_usb_xferstatus_curr_dma_set,
    &hf_usb_xferstatus_can_cancel_immed,
    &hf_usb_xferstatus_doing_callback,
    NULL
};

/* USB errors */
#define FREEBSD_ERR_NORMAL_COMPLETION 0
#define FREEBSD_ERR_PENDING_REQUESTS  1
#define FREEBSD_ERR_NOT_STARTED       2
#define FREEBSD_ERR_INVAL             3
#define FREEBSD_ERR_NOMEM             4
#define FREEBSD_ERR_CANCELLED         5
#define FREEBSD_ERR_BAD_ADDRESS       6
#define FREEBSD_ERR_BAD_BUFSIZE       7
#define FREEBSD_ERR_BAD_FLAG          8
#define FREEBSD_ERR_NO_CALLBACK       9
#define FREEBSD_ERR_IN_USE            10
#define FREEBSD_ERR_NO_ADDR           11
#define FREEBSD_ERR_NO_PIPE           12
#define FREEBSD_ERR_ZERO_NFRAMES      13
#define FREEBSD_ERR_ZERO_MAXP         14
#define FREEBSD_ERR_SET_ADDR_FAILED   15
#define FREEBSD_ERR_NO_POWER          16
#define FREEBSD_ERR_TOO_DEEP          17
#define FREEBSD_ERR_IOERROR           18
#define FREEBSD_ERR_NOT_CONFIGURED    19
#define FREEBSD_ERR_TIMEOUT           20
#define FREEBSD_ERR_SHORT_XFER        21
#define FREEBSD_ERR_STALLED           22
#define FREEBSD_ERR_INTERRUPTED       23
#define FREEBSD_ERR_DMA_LOAD_FAILED   24
#define FREEBSD_ERR_BAD_CONTEXT       25
#define FREEBSD_ERR_NO_ROOT_HUB       26
#define FREEBSD_ERR_NO_INTR_THREAD    27
#define FREEBSD_ERR_NOT_LOCKED        28

static const value_string usb_freebsd_err_vals[] = {
    {FREEBSD_ERR_NORMAL_COMPLETION, "Normal completion"},
    {FREEBSD_ERR_PENDING_REQUESTS,  "Pending requests"},
    {FREEBSD_ERR_NOT_STARTED,       "Not started"},
    {FREEBSD_ERR_INVAL,             "Invalid"},
    {FREEBSD_ERR_NOMEM,             "No memory"},
    {FREEBSD_ERR_CANCELLED,         "Cancelled"},
    {FREEBSD_ERR_BAD_ADDRESS,       "Bad address"},
    {FREEBSD_ERR_BAD_BUFSIZE,       "Bad buffer size"},
    {FREEBSD_ERR_BAD_FLAG,          "Bad flag"},
    {FREEBSD_ERR_NO_CALLBACK,       "No callback"},
    {FREEBSD_ERR_IN_USE,            "In use"},
    {FREEBSD_ERR_NO_ADDR,           "No address"},
    {FREEBSD_ERR_NO_PIPE,           "No pipe"},
    {FREEBSD_ERR_ZERO_NFRAMES,      "Number of frames is zero"},
    {FREEBSD_ERR_ZERO_MAXP,         "MAXP is zero"},
    {FREEBSD_ERR_SET_ADDR_FAILED,   "Set address failed"},
    {FREEBSD_ERR_NO_POWER,          "No power"},
    {FREEBSD_ERR_TOO_DEEP,          "Too deep"},
    {FREEBSD_ERR_IOERROR,           "I/O error"},
    {FREEBSD_ERR_NOT_CONFIGURED,    "Not configured"},
    {FREEBSD_ERR_TIMEOUT,           "Timeout"},
    {FREEBSD_ERR_SHORT_XFER,        "Short transfer"},
    {FREEBSD_ERR_STALLED,           "Stalled"},
    {FREEBSD_ERR_INTERRUPTED,       "Interrupted"},
    {FREEBSD_ERR_DMA_LOAD_FAILED,   "DMA load failed"},
    {FREEBSD_ERR_BAD_CONTEXT,       "Bad context"},
    {FREEBSD_ERR_NO_ROOT_HUB,       "No root hub"},
    {FREEBSD_ERR_NO_INTR_THREAD,    "No interrupt thread"},
    {FREEBSD_ERR_NOT_LOCKED,        "Not locked"},
    {0, NULL}
};

/* USB speeds */
#define FREEBSD_SPEED_VARIABLE 0
#define FREEBSD_SPEED_LOW      1
#define FREEBSD_SPEED_FULL     2
#define FREEBSD_SPEED_HIGH     3
#define FREEBSD_SPEED_SUPER    4

static const value_string usb_freebsd_speed_vals[] = {
    {FREEBSD_SPEED_VARIABLE, "Variable"},
    {FREEBSD_SPEED_LOW,      "Low"},
    {FREEBSD_SPEED_FULL,     "Full"},
    {FREEBSD_SPEED_HIGH,     "High"},
    {FREEBSD_SPEED_SUPER,    "Super"},
    {0, NULL}
};

/* Frame flags */
#define FREEBSD_FRAMEFLAG_READ         0x00000001
#define FREEBSD_FRAMEFLAG_DATA_FOLLOWS 0x00000002

static const int *usb_frame_flags_fields[] = {
    &hf_usb_frame_flags_read,
    &hf_usb_frame_flags_data_follows,
    NULL
};

static const value_string usb_linux_urb_type_vals[] = {
    {URB_SUBMIT,   "URB_SUBMIT"},
    {URB_COMPLETE, "URB_COMPLETE"},
    {URB_ERROR,    "URB_ERROR"},
    {0, NULL}
};

static const value_string usb_linux_transfer_type_vals[] = {
    {URB_CONTROL,                       "URB_CONTROL"},
    {URB_ISOCHRONOUS,                   "URB_ISOCHRONOUS"},
    {URB_INTERRUPT,                     "URB_INTERRUPT"},
    {URB_BULK,                          "URB_BULK"},
    {0, NULL}
};

static const value_string usb_transfer_type_and_direction_vals[] = {
    {URB_CONTROL,                       "URB_CONTROL out"},
    {URB_ISOCHRONOUS,                   "URB_ISOCHRONOUS out"},
    {URB_INTERRUPT,                     "URB_INTERRUPT out"},
    {URB_BULK,                          "URB_BULK out"},
    {URB_CONTROL | URB_TRANSFER_IN,     "URB_CONTROL in"},
    {URB_ISOCHRONOUS | URB_TRANSFER_IN, "URB_ISOCHRONOUS in"},
    {URB_INTERRUPT | URB_TRANSFER_IN,   "URB_INTERRUPT in"},
    {URB_BULK | URB_TRANSFER_IN,        "URB_BULK in"},
    {0, NULL}
};

static const value_string usb_endpoint_direction_vals[] = {
    {0, "OUT"},
    {1, "IN"},
    {0, NULL}
};

extern value_string_ext ext_usb_vendors_vals;
extern value_string_ext ext_usb_products_vals;
extern value_string_ext ext_usb_com_subclass_vals;

/*
 * Standard descriptor types.
 *
 * all class specific descriptor types were removed from this list
 * a descriptor type is not globally unique
 * dissectors for the USB classes should provide their own value string
 *  and pass it to dissect_usb_descriptor_header()
 *
 */
#define USB_DT_DEVICE                          1
#define USB_DT_CONFIG                          2
#define USB_DT_STRING                          3
#define USB_DT_INTERFACE                       4
#define USB_DT_ENDPOINT                        5
#define USB_DT_DEVICE_QUALIFIER                6
#define USB_DT_OTHER_SPEED_CONFIG              7
#define USB_DT_INTERFACE_POWER                 8
/* these are from a minor usb 2.0 revision (ECN) */
#define USB_DT_OTG                             9
#define USB_DT_DEBUG                          10
#define USB_DT_INTERFACE_ASSOCIATION          11

/* There are only Standard Descriptor Types, Class-specific types are
   provided by "usb.descriptor" descriptors table*/
static const value_string std_descriptor_type_vals[] = {
    {USB_DT_DEVICE,                         "DEVICE"},
    {USB_DT_CONFIG,                         "CONFIGURATION"},
    {USB_DT_STRING,                         "STRING"},
    {USB_DT_INTERFACE,                      "INTERFACE"},
    {USB_DT_ENDPOINT,                       "ENDPOINT"},
    {USB_DT_DEVICE_QUALIFIER,               "DEVICE QUALIFIER"},
    {USB_DT_OTHER_SPEED_CONFIG,             "OTHER SPEED CONFIG"},
    {USB_DT_INTERFACE_POWER,                "INTERFACE POWER"},
    {USB_DT_OTG,                            "OTG"},
    {USB_DT_DEBUG,                          "DEBUG"},
    {USB_DT_INTERFACE_ASSOCIATION,          "INTERFACE ASSOCIATION"},
    { 0x0F,                                 "BOS"},
    { 0x10,                                 "DEVICE CAPABILITY"},
    { 0x30,                                 "SUPERSPEED USB ENDPOINT COMPANION"},
    { 0x31,                                 "SUPERSPEED PLUS ISOCHRONOUS ENDPOINT COMPANION"},
    {0,NULL}
};
static value_string_ext std_descriptor_type_vals_ext =
               VALUE_STRING_EXT_INIT(std_descriptor_type_vals);

/*
 * Feature selectors.
 * Per USB 3.1 spec, Table 9-7
 */
#define USB_FS_ENDPOINT_HALT            0
#define USB_FS_FUNCTION_SUSPEND         0 /* same as ENDPOINT_HALT */
#define USB_FS_DEVICE_REMOTE_WAKEUP     1
#define USB_FS_TEST_MODE                2
#define USB_FS_B_HNP_ENABLE             3
#define USB_FS_A_HNP_SUPPORT            4
#define USB_FS_A_ALT_HNP_SUPPORT        5
#define USB_FS_WUSB_DEVICE              6
#define USB_FS_U1_ENABLE                48
#define USB_FS_U2_ENABLE                49
#define USB_FS_LTM_ENABLE               50
#define USB_FS_B3_NTF_HOST_REL          51
#define USB_FS_B3_RSP_ENABLE            52
#define USB_FS_LDM_ENABLE               53

static const value_string usb_endpoint_feature_selector_vals[] = {
    {USB_FS_ENDPOINT_HALT,              "ENDPOINT HALT"},
    {0, NULL}
};

static const value_string usb_interface_feature_selector_vals[] = {
    {USB_FS_FUNCTION_SUSPEND,           "FUNCTION SUSPEND"},
    {0, NULL}
};

static const value_string usb_device_feature_selector_vals[] = {
    {USB_FS_DEVICE_REMOTE_WAKEUP,       "DEVICE REMOTE WAKEUP"},
    {USB_FS_TEST_MODE,                  "TEST MODE"},
    {USB_FS_B_HNP_ENABLE,               "B HNP ENABLE"},
    {USB_FS_A_HNP_SUPPORT,              "A HNP SUPPORT"},
    {USB_FS_A_ALT_HNP_SUPPORT,          "A ALT HNP SUPPORT"},
    {USB_FS_WUSB_DEVICE,                "WUSB DEVICE"},
    {USB_FS_U1_ENABLE,                  "U1 ENABLE"},
    {USB_FS_U2_ENABLE,                  "U2 ENABLE"},
    {USB_FS_LTM_ENABLE,                 "LTM ENABLE"},
    {USB_FS_B3_NTF_HOST_REL,            "B3 NTF HOST REL"},
    {USB_FS_B3_RSP_ENABLE,              "B3 RSP ENABLE"},
    {USB_FS_LDM_ENABLE,                 "LDM ENABLE"},
    {0, NULL}
};


/* the transfer type in the endpoint descriptor, i.e. the type of the endpoint
   (this is not the same as the URB transfer type) */
#define USB_EP_CONTROL     0x00
#define USB_EP_ISOCHRONOUS 0x01
#define USB_EP_BULK        0x02
#define USB_EP_INTERRUPT   0x03

static const value_string usb_bmAttributes_transfer_vals[] = {
    {USB_EP_CONTROL,     "Control-Transfer"},
    {USB_EP_ISOCHRONOUS, "Isochronous-Transfer"},
    {USB_EP_BULK,        "Bulk-Transfer"},
    {USB_EP_INTERRUPT,   "Interrupt-Transfer"},
    {0, NULL}
};

static const value_string usb_bmAttributes_sync_vals[] = {
    {0x00,      "No Sync"},
    {0x01,      "Asynchronous"},
    {0x02,      "Adaptive"},
    {0x03,      "Synchronous"},
    {0, NULL}
};

static const value_string usb_bmAttributes_behaviour_vals[] = {
    {0x00,      "Data-Endpoint"},
    {0x01,      "Explicit Feedback-Endpoint"},
    {0x02,      "Implicit Feedback-Data-Endpoint"},
    {0x03,      "Reserved"},
    {0, NULL}
};

static const value_string usb_wMaxPacketSize_slots_vals[]  = {
    {0x00,      "1"},
    {0x01,      "2"},
    {0x02,      "3"},
    {0x03,      "Reserved"},
    {0, NULL}
};

/* Note: sorted in (unsigned) ascending order */
static const value_string usb_urb_status_vals[] = {
    /* from linux/include/asm-generic/errno.h*/
    { -131, "State not recoverable (-ENOTRECOVERABLE)" },
    { -130, "Owner died (-EOWNERDEAD)" },
    { -129, "Key was rejected by service (-EKEYREJECTED)" },
    { -128, "Key has been revoked (-EKEYREVOKED)" },
    { -127, "Key has expired (-EKEYEXPIRED)" },
    { -126, "Required key not available (-ENOKEY)" },
    { -125, "Operation Canceled (-ECANCELED)" },
    { -124, "Wrong medium type (-EMEDIUMTYPE)" },
    { -123, "No medium found (-ENOMEDIUM)" },
    { -122, "Quota exceeded (-EDQUOT)" },
    { -121, "Remote I/O error (-EREMOTEIO)" },
    { -120, "Is a named type file (-EISNAM)" },
    { -119, "No XENIX semaphores available (-ENAVAIL)" },
    { -118, "Not a XENIX named type file (-ENOTNAM)" },
    { -117, "Structure needs cleaning (-EUCLEAN)" },
    { -116, "Stale NFS file handle (-ESTALE)" },
    { -115, "Operation now in progress (-EINPROGRESS)" },
    { -114, "Operation already in progress (-EALREADY)" },
    { -113, "No route to host (-EHOSTUNREACH)" },
    { -112, "Host is down (-EHOSTDOWN)" },
    { -111, "Connection refused (-ECONNREFUSED)" },
    { -110, "Connection timed out (-ETIMEDOUT)" },
    { -109, "Too many references: cannot splice (-ETOOMANYREFS)" },
    { -108, "Cannot send after transport endpoint shutdown (-ESHUTDOWN)" },
    { -107, "Transport endpoint is not connected (-ENOTCONN)" },
    { -106, "Transport endpoint is already connected (-EISCONN)" },
    { -105, "No buffer space available (-ENOBUFS)" },
    { -104, "Connection reset by peer (-ECONNRESET)" },
    { -103, "Software caused connection abort (-ECONNABORTED)" },
    { -102, "Network dropped connection because of reset (-ENETRESET)" },
    { -101, "Network is unreachable (-ENETUNREACH)" },
    { -100, "Network is down (-ENETDOWN)" },
    { -99,  "Cannot assign requested address (-EADDRNOTAVAIL)" },
    { -98,  "Address already in use (-EADDRINUSE)" },
    { -97,  "Address family not supported by protocol (-EAFNOSUPPORT)" },
    { -96,  "Protocol family not supported (-EPFNOSUPPORT)" },
    { -95,  "Operation not supported on transport endpoint (-EOPNOTSUPP)" },
    { -94,  "Socket type not supported (-ESOCKTNOSUPPORT)" },
    { -93,  "Protocol not supported (-EPROTONOSUPPORT)" },
    { -92,  "Protocol not available (-ENOPROTOOPT)" },
    { -91,  "Protocol wrong type for socket (-EPROTOTYPE)" },
    { -90,  "Message too long (-EMSGSIZE)" },
    { -89,  "Destination address required (-EDESTADDRREQ)" },
    { -88,  "Socket operation on non-socket (-ENOTSOCK)" },
    { -87,  "Too many users (-EUSERS)" },
    { -86,  "Streams pipe error (-ESTRPIPE)" },
    { -85,  "Interrupted system call should be restarted (-ERESTART)" },
    { -84,  "Illegal byte sequence (-EILSEQ)" },
    { -83,  "Cannot exec a shared library directly (-ELIBEXEC)" },
    { -82,  "Attempting to link in too many shared libraries (-ELIBMAX)" },
    { -81,  ".lib section in a.out corrupted (-ELIBSCN)" },
    { -80,  "Accessing a corrupted shared library (-ELIBBAD)" },
    { -79,  "Can not access a needed shared library (-ELIBACC)" },
    { -78,  "Remote address changed (-EREMCHG)" },
    { -77,  "File descriptor in bad state (-EBADFD)" },
    { -76,  "Name not unique on network (-ENOTUNIQ)" },
    { -75,  "Value too large for defined data type (-EOVERFLOW)" },
    { -74,  "Not a data message (-EBADMSG)" },
    { -73,  "RFS specific error (-EDOTDOT)" },
    { -72,  "Multihop attempted (-EMULTIHOP)" },
    { -71,  "Protocol error (-EPROTO)" },
    { -70,  "Communication error on send (-ECOMM)" },
    { -69,  "Srmount error (-ESRMNT)" },
    { -68,  "Advertise error (-EADV)" },
    { -67,  "Link has been severed (-ENOLINK)" },
    { -66,  "Object is remote (-EREMOTE)" },
    { -65,  "Package not installed (-ENOPKG)" },
    { -64,  "Machine is not on the network (-ENONET)" },
    { -63,  "Out of streams resources (-ENOSR)" },
    { -62,  "Timer expired (-ETIME)" },
    { -61,  "No data available (-ENODATA)" },
    { -60,  "Device not a stream (-ENOSTR)" },
    { -59,  "Bad font file format (-EBFONT)" },
    { -58,  "(-58 \?\?\?)" },   /* dummy so that there are no "gaps" */
    { -57,  "Invalid slot (-EBADSLT)" },
    { -56,  "Invalid request code (-EBADRQC)" },
    { -55,  "No anode (-ENOANO)" },
    { -54,  "Exchange full (-EXFULL)" },
    { -53,  "Invalid request descriptor (-EBADR)" },
    { -52,  "Invalid exchange (-EBADE)" },
    { -51,  "Level 2 halted (-EL2HLT)" },
    { -50,  "No CSI structure available (-ENOCSI)" },
    { -49,  "Protocol driver not attached (-EUNATCH)" },
    { -48,  "Link number out of range (-ELNRNG)" },
    { -47,  "Level 3 reset (-EL3RST)" },
    { -46,  "Level 3 halted (-EL3HLT)" },
    { -45,  "Level 2 not synchronized (-EL2NSYNC)" },
    { -44,  "Channel number out of range (-ECHRNG)" },
    { -43,  "Identifier removed (-EIDRM)" },
    { -42,  "No message of desired type (-ENOMSG)" },
    { -41,  "(-41 \?\?\?)" },   /* dummy so that there are no "gaps" */
    { -40,  "Too many symbolic links encountered (-ELOOP)" },
    { -39,  "Directory not empty (-ENOTEMPTY)" },
    { -38,  "Function not implemented (-ENOSYS)" },
    { -37,  "No record locks available (-ENOLCK)" },
    { -36,  "File name too long (-ENAMETOOLONG)" },
    { -35,  "Resource deadlock would occur (-EDEADLK)" },
    /* from linux/include/asm-generic/errno.h */
    { -34,  "Math result not representable (-ERANGE)" },
    { -33,  "Math argument out of domain of func (-EDOM)" },
    { -32,  "Broken pipe (-EPIPE)" },
    { -31,  "Too many links (-EMLINK)" },
    { -30,  "Read-only file system (-EROFS)" },
    { -29,  "Illegal seek (-ESPIPE)" },
    { -28,  "No space left on device (-ENOSPC)" },
    { -27,  "File too large (-EFBIG)" },
    { -26,  "Text file busy (-ETXTBSY)" },
    { -25,  "Not a typewriter (-ENOTTY)" },
    { -24,  "Too many open files (-EMFILE)" },
    { -23,  "File table overflow (-ENFILE)" },
    { -22,  "Invalid argument (-EINVAL)" },
    { -21,  "Is a directory (-EISDIR)" },
    { -20,  "Not a directory (-ENOTDIR)" },
    { -19,  "No such device (-ENODEV)" },
    { -18,  "Cross-device link (-EXDEV)" },
    { -17,  "File exists (-EEXIST)" },
    { -16,  "Device or resource busy (-EBUSY)" },
    { -15,  "Block device required (-ENOTBLK)" },
    { -14,  "Bad address (-EFAULT)" },
    { -13,  "Permission denied (-EACCES)" },
    { -12,  "Out of memory (-ENOMEM)" },
    { -11,  "Try again (-EAGAIN)" },
    { -10,  "No child processes (-ECHILD)" },
    { -9,   "Bad file number (-EBADF)" },
    { -8,   "Exec format error (-ENOEXEC)" },
    { -7,   "Argument list too long (-E2BIG)" },
    { -6,   "No such device or address (-ENXIO)" },
    { -5,   "I/O error (-EIO)" },
    { -4,   "Interrupted system call (-EINTR)" },
    { -3,   "No such process (-ESRCH)" },
    { -2,   "No such file or directory (-ENOENT)" },
    { -1,   "Operation not permitted (-EPERM)" },
    { 0,    "Success"},
    { 0, NULL }
};
value_string_ext usb_urb_status_vals_ext = VALUE_STRING_EXT_INIT(usb_urb_status_vals);

#define USB_CONTROL_STAGE_SETUP  0x00
#define USB_CONTROL_STAGE_DATA   0x01
#define USB_CONTROL_STAGE_STATUS 0x02

static const value_string usb_control_stage_vals[] = {
    {USB_CONTROL_STAGE_SETUP,  "Setup"},
    {USB_CONTROL_STAGE_DATA,   "Data"},
    {USB_CONTROL_STAGE_STATUS, "Status"},
    {0, NULL}
};

static const value_string win32_urb_function_vals[] = {
    {0x0000, "URB_FUNCTION_SELECT_CONFIGURATION"},
    {0x0001, "URB_FUNCTION_SELECT_INTERFACE"},
    {0x0002, "URB_FUNCTION_ABORT_PIPE"},
    {0x0003, "URB_FUNCTION_TAKE_FRAME_LENGTH_CONTROL"},
    {0x0004, "URB_FUNCTION_RELEASE_FRAME_LENGTH_CONTROL"},
    {0x0005, "URB_FUNCTION_GET_FRAME_LENGTH"},
    {0x0006, "URB_FUNCTION_SET_FRAME_LENGTH"},
    {0x0007, "URB_FUNCTION_GET_CURRENT_FRAME_NUMBER"},
    {0x0008, "URB_FUNCTION_CONTROL_TRANSFER"},
    {0x0009, "URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER"},
    {0x000A, "URB_FUNCTION_ISOCH_TRANSFER"},
    {0x000B, "URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE"},
    {0x000C, "URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE"},
    {0x000D, "URB_FUNCTION_SET_FEATURE_TO_DEVICE"},
    {0x000E, "URB_FUNCTION_SET_FEATURE_TO_INTERFACE"},
    {0x000F, "URB_FUNCTION_SET_FEATURE_TO_ENDPOINT"},
    {0x0010, "URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE"},
    {0x0011, "URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE"},
    {0x0012, "URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT"},
    {0x0013, "URB_FUNCTION_GET_STATUS_FROM_DEVICE"},
    {0x0014, "URB_FUNCTION_GET_STATUS_FROM_INTERFACE"},
    {0x0015, "URB_FUNCTION_GET_STATUS_FROM_ENDPOINT"},
    {0x0016, "URB_FUNCTION_RESERVED_0X0016"},
    {0x0017, "URB_FUNCTION_VENDOR_DEVICE"},
    {0x0018, "URB_FUNCTION_VENDOR_INTERFACE"},
    {0x0019, "URB_FUNCTION_VENDOR_ENDPOINT"},
    {0x001A, "URB_FUNCTION_CLASS_DEVICE"},
    {0x001B, "URB_FUNCTION_CLASS_INTERFACE"},
    {0x001C, "URB_FUNCTION_CLASS_ENDPOINT"},
    {0x001D, "URB_FUNCTION_RESERVE_0X001D"},
    {0x001E, "URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL"},
    {0x001F, "URB_FUNCTION_CLASS_OTHER"},
    {0x0020, "URB_FUNCTION_VENDOR_OTHER"},
    {0x0021, "URB_FUNCTION_GET_STATUS_FROM_OTHER"},
    {0x0022, "URB_FUNCTION_CLEAR_FEATURE_TO_OTHER"},
    {0x0023, "URB_FUNCTION_SET_FEATURE_TO_OTHER"},
    {0x0024, "URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT"},
    {0x0025, "URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT"},
    {0x0026, "URB_FUNCTION_GET_CONFIGURATION"},
    {0x0027, "URB_FUNCTION_GET_INTERFACE"},
    {0x0028, "URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE"},
    {0x0029, "URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE"},
    {0x002A, "URB_FUNCTION_GET_MS_FEATURE_DESCRIPTOR"},
    {0x002B, "URB_FUNCTION_RESERVE_0X002B"},
    {0x002C, "URB_FUNCTION_RESERVE_0X002C"},
    {0x002D, "URB_FUNCTION_RESERVE_0X002D"},
    {0x002E, "URB_FUNCTION_RESERVE_0X002E"},
    {0x002F, "URB_FUNCTION_RESERVE_0X002F"},
    {0x0030, "URB_FUNCTION_SYNC_RESET_PIPE"},
    {0x0031, "URB_FUNCTION_SYNC_CLEAR_STALL"},
    {0x0032, "URB_FUNCTION_CONTROL_TRANSFER_EX"},
    {0x0033, "URB_FUNCTION_RESERVE_0X0033"},
    {0x0034, "URB_FUNCTION_RESERVE_0X0034"},
    {0, NULL}
};
value_string_ext win32_urb_function_vals_ext = VALUE_STRING_EXT_INIT(win32_urb_function_vals);

static const value_string win32_usbd_status_vals[] = {
    {0x00000000, "USBD_STATUS_SUCCESS"},
    {0x40000000, "USBD_STATUS_PENDING"},

    {0x80000200, "USBD_STATUS_INVALID_URB_FUNCTION"},
    {0x80000300, "USBD_STATUS_INVALID_PARAMETER"},
    {0x80000400, "USBD_STATUS_ERROR_BUSY"},
    {0x80000600, "USBD_STATUS_INVALID_PIPE_HANDLE"},
    {0x80000700, "USBD_STATUS_NO_BANDWIDTH"},
    {0x80000800, "USBD_STATUS_INTERNAL_HC_ERROR"},
    {0x80000900, "USBD_STATUS_ERROR_SHORT_TRANSFER"},

    {0xC0000001, "USBD_STATUS_CRC"},
    {0xC0000002, "USBD_STATUS_BTSTUFF"},
    {0xC0000003, "USBD_STATUS_DATA_TOGGLE_MISMATCH"},
    {0xC0000004, "USBD_STATUS_STALL_PID"},
    {0xC0000005, "USBD_STATUS_DEV_NOT_RESPONDING"},
    {0xC0000006, "USBD_STATUS_PID_CHECK_FAILURE"},
    {0xC0000007, "USBD_STATUS_UNEXPECTED_PID"},
    {0xC0000008, "USBD_STATUS_DATA_OVERRUN"},
    {0xC0000009, "USBD_STATUS_DATA_UNDERRUN"},
    {0xC000000A, "USBD_STATUS_RESERVED1"},
    {0xC000000B, "USBD_STATUS_RESERVED2"},
    {0xC000000C, "USBD_STATUS_BUFFER_OVERRUN"},
    {0xC000000D, "USBD_STATUS_BUFFER_UNDERRUN"},
    {0xC000000F, "USBD_STATUS_NOT_ACCESSED"},
    {0xC0000010, "USBD_STATUS_FIFO"},
    {0xC0000011, "USBD_STATUS_XACT_ERROR"},
    {0xC0000012, "USBD_STATUS_BABBLE_DETECTED"},
    {0xC0000013, "USBD_STATUS_DATA_BUFFER_ERROR"},
    {0xC0000030, "USBD_STATUS_ENDPOINT_HALTED"},

    {0xC0000A00, "USBD_STATUS_BAD_START_FRAME"},
    {0xC0000B00, "USBD_STATUS_ISOCH_REQUEST_FAILED"},
    {0xC0000C00, "USBD_STATUS_FRAME_CONTROL_OWNED"},
    {0xC0000D00, "USBD_STATUS_FRAME_CONTROL_NOT_OWNED"},
    {0xC0000E00, "USBD_STATUS_NOT_SUPPORTED"},
    {0xC0000F00, "USBD_STATUS_INVALID_CONFIGURATION_DESCRIPTOR"},
    {0xC0001000, "USBD_STATUS_INSUFFICIENT_RESOURCES"},
    {0xC0002000, "USBD_STATUS_SET_CONFIG_FAILED"},
    {0xC0003000, "USBD_STATUS_BUFFER_TOO_SMALL"},
    {0xC0004000, "USBD_STATUS_INTERFACE_NOT_FOUND"},
    {0xC0005000, "USBD_STATUS_INVALID_PIPE_FLAGS"},
    {0xC0006000, "USBD_STATUS_TIMEOUT"},
    {0xC0007000, "USBD_STATUS_DEVICE_GONE"},
    {0xC0008000, "USBD_STATUS_STATUS_NOT_MAPPED"},
    {0xC0009000, "USBD_STATUS_HUB_INTERNAL_ERROR"},
    {0xC0010000, "USBD_STATUS_CANCELED"},
    {0xC0020000, "USBD_STATUS_ISO_NOT_ACCESSED_BY_HW"},
    {0xC0030000, "USBD_STATUS_ISO_TD_ERROR"},
    {0xC0040000, "USBD_STATUS_ISO_NA_LATE_USBPORT"},
    {0xC0050000, "USBD_STATUS_ISO_NOT_ACCESSED_LATE"},
    {0xC0100000, "USBD_STATUS_BAD_DESCRIPTOR"},
    {0xC0100001, "USBD_STATUS_BAD_DESCRIPTOR_BLEN"},
    {0xC0100002, "USBD_STATUS_BAD_DESCRIPTOR_TYPE"},
    {0xC0100003, "USBD_STATUS_BAD_INTERFACE_DESCRIPTOR"},
    {0xC0100004, "USBD_STATUS_BAD_ENDPOINT_DESCRIPTOR"},
    {0xC0100005, "USBD_STATUS_BAD_INTERFACE_ASSOC_DESCRIPTOR"},
    {0xC0100006, "USBD_STATUS_BAD_CONFIG_DESC_LENGTH"},
    {0xC0100007, "USBD_STATUS_BAD_NUMBER_OF_INTERFACES"},
    {0xC0100008, "USBD_STATUS_BAD_NUMBER_OF_ENDPOINTS"},
    {0xC0100009, "USBD_STATUS_BAD_ENDPOINT_ADDRESS"},
    {0, NULL}
};
static value_string_ext win32_usbd_status_vals_ext = VALUE_STRING_EXT_INIT(win32_usbd_status_vals);

static const value_string win32_usb_info_direction_vals[] = {
    {0, "FDO -> PDO"},
    {1, "PDO -> FDO"},
    {0, NULL}
};

static const value_string usb_cdc_protocol_vals[] = {
    {0x00, "No class specific protocol required"},
    {0x01, "AT Commands: V.250 etc"},
    {0x02, "AT Commands defined by PCCA-101"},
    {0x03, "AT Commands defined by PCCA-101 & Annex O"},
    {0x04, "AT Commands defined by GSM 07.07"},
    {0x05, "AT Commands defined by 3GPP 27.007"},
    {0x06, "AT Commands defined by TIA for CDMA"},
    {0x07, "Ethernet Emulation Model"},
    {0xFE, "External Protocol: Commands defined by Command Set Functional Descriptor"},
    {0xFF, "Vendor-specific"},
    {0, NULL}
};
static value_string_ext usb_cdc_protocol_vals_ext = VALUE_STRING_EXT_INIT(usb_cdc_protocol_vals);

static const value_string usb_cdc_data_protocol_vals[] = {
    {0x00, "No class specific protocol required"},
    {0x01, "Network Transfer Block"},
    {0x02, "Network Transfer Block (IP + DSS)"},
    {0x30, "Physical interface protocol for ISDN BRI"},
    {0x31, "HDLC"},
    {0x32, "Transparent"},
    {0x50, "Management protocol for Q.921 data link protocol"},
    {0x51, "Data link protocol for Q.931"},
    {0x52, "TEI-multiplexor for Q.921 data link protocol"},
    {0x90, "Data compression procedures"},
    {0x91, "Euro-ISDN protocol control"},
    {0x92, "V.24 rate adaptation to ISDN"},
    {0x93, "CAPI Commands"},
    {0xFE, "The protocol(s) are described using a Protocol Unit Functional Descriptors on Communications Class Interface"},
    {0xFF, "Vendor-specific"},
    {0, NULL}
};
static value_string_ext usb_cdc_data_protocol_vals_ext = VALUE_STRING_EXT_INIT(usb_cdc_data_protocol_vals);

static const value_string usb_hid_subclass_vals[] = {
    {0, "No Subclass"},
    {1, "Boot Interface"},
    {0, NULL}
};
static value_string_ext usb_hid_subclass_vals_ext = VALUE_STRING_EXT_INIT(usb_hid_subclass_vals);

static const value_string usb_hid_boot_protocol_vals[] = {
    {0, "None"},
    {1, "Keyboard"},
    {2, "Mouse"},
    {0, NULL}
};
static value_string_ext usb_hid_boot_protocol_vals_ext = VALUE_STRING_EXT_INIT(usb_hid_boot_protocol_vals);

static const value_string usb_misc_subclass_vals[] = {
    {0x03, "Cable Based Association Framework"},
    {0x04, "RNDIS"},
    {IF_SUBCLASS_MISC_U3V, "USB3 Vision"},
    {0x06, "Stream Transport Efficient Protocol"},
    {0, NULL}
};
static value_string_ext usb_misc_subclass_vals_ext = VALUE_STRING_EXT_INIT(usb_misc_subclass_vals);


static const value_string usb_app_subclass_vals[] = {
    {0x01, "Device Firmware Upgrade"},
    {0x02, "IRDA Bridge"},
    {0x03, "USB Test and Measurement Device"},
    {0, NULL}
};
static value_string_ext usb_app_subclass_vals_ext = VALUE_STRING_EXT_INIT(usb_app_subclass_vals);


static const value_string usb_app_dfu_protocol_vals[] = {
    {0x01, "Runtime protocol"},
    {0x02, "DFU mode protocol"},
    {0, NULL}
};
static value_string_ext usb_app_dfu_protocol_vals_ext = VALUE_STRING_EXT_INIT(usb_app_dfu_protocol_vals);

static const value_string usb_app_irda_protocol_vals[] = {
    {0x00, "IRDA Bridge device"},
    {0, NULL}
};
static value_string_ext usb_app_irda_protocol_vals_ext = VALUE_STRING_EXT_INIT(usb_app_irda_protocol_vals);

static const value_string usb_app_usb_test_and_measurement_protocol_vals[] = {
    {0x00, "USB Test and Measurement Device"},
    {0x01, "USB Test and Measurement Device conforming to the USBTMC USB488 Subclass Specification"},
    {0, NULL}
};
static value_string_ext usb_app_usb_test_and_measurement_protocol_vals_ext = VALUE_STRING_EXT_INIT(usb_app_usb_test_and_measurement_protocol_vals);

void proto_register_usb(void);
void proto_reg_handoff_usb(void);

/* USB address handling */
static int usb_addr_to_str(const address* addr, gchar *buf, int buf_len _U_)
{
    const guint8 *addrp = (const guint8 *)addr->data;

    if(pletoh32(&addrp[0])==0xffffffff){
        g_strlcpy(buf, "host", buf_len);
    } else {
        g_snprintf(buf, buf_len, "%d.%d.%d", pletoh16(&addrp[8]),
                        pletoh32(&addrp[0]), pletoh32(&addrp[4]));
    }

    return (int)(strlen(buf)+1);
}

static int usb_addr_str_len(const address* addr _U_)
{
    return 50;
}


/* This keys provide information for DecodeBy and other dissector via
   per packet data: p_get_proto_data()/p_add_proto_data() */
#define USB_BUS_ID           0
#define USB_DEVICE_ADDRESS   1
#define USB_VENDOR_ID        2
#define USB_PRODUCT_ID       3
#define USB_DEVICE_CLASS     4
#define USB_DEVICE_SUBCLASS  5
#define USB_DEVICE_PROTOCOL  6

static void
usb_device_prompt(packet_info *pinfo, gchar* result)
{
    g_snprintf(result, MAX_DECODE_AS_PROMPT_LEN, "Bus ID %u \nDevice Address %u\nas ",
            GPOINTER_TO_UINT(p_get_proto_data(pinfo->pool, pinfo, proto_usb, USB_BUS_ID)),
            GPOINTER_TO_UINT(p_get_proto_data(pinfo->pool, pinfo, proto_usb, USB_DEVICE_ADDRESS)));
}

static gpointer
usb_device_value(packet_info *pinfo)
{
    guint32 value = GPOINTER_TO_UINT(p_get_proto_data(pinfo->pool, pinfo, proto_usb, USB_BUS_ID)) << 16;
    value |= GPOINTER_TO_UINT(p_get_proto_data(pinfo->pool, pinfo, proto_usb, USB_DEVICE_ADDRESS));
    return GUINT_TO_POINTER(value);
}

static void
usb_product_prompt(packet_info *pinfo, gchar* result)
{
    g_snprintf(result, MAX_DECODE_AS_PROMPT_LEN, "Vendor ID 0x%04x \nProduct ID 0x%04x\nas ",
            GPOINTER_TO_UINT(p_get_proto_data(pinfo->pool, pinfo, proto_usb, USB_VENDOR_ID)),
            GPOINTER_TO_UINT(p_get_proto_data(pinfo->pool, pinfo, proto_usb, USB_PRODUCT_ID)));
}

static gpointer
usb_product_value(packet_info *pinfo)
{
    guint32 value = GPOINTER_TO_UINT(p_get_proto_data(pinfo->pool, pinfo, proto_usb, USB_VENDOR_ID)) << 16;
    value |= GPOINTER_TO_UINT(p_get_proto_data(pinfo->pool, pinfo, proto_usb, USB_PRODUCT_ID));
    return GUINT_TO_POINTER(value);
}

static void
usb_protocol_prompt(packet_info *pinfo, gchar* result)
{
    g_snprintf(result, MAX_DECODE_AS_PROMPT_LEN, "Class ID 0x%04x \nSubclass ID 0x%04x\nProtocol 0x%04x\nas ",
            GPOINTER_TO_UINT(p_get_proto_data(pinfo->pool, pinfo, proto_usb, USB_DEVICE_CLASS)),
            GPOINTER_TO_UINT(p_get_proto_data(pinfo->pool, pinfo, proto_usb, USB_DEVICE_SUBCLASS)),
            GPOINTER_TO_UINT(p_get_proto_data(pinfo->pool, pinfo, proto_usb, USB_DEVICE_PROTOCOL)));
}

static gpointer
usb_protocol_value(packet_info *pinfo)
{
    guint32 value = GPOINTER_TO_UINT(p_get_proto_data(pinfo->pool, pinfo, proto_usb, USB_DEVICE_CLASS)) << 16;
    value |= GPOINTER_TO_UINT(p_get_proto_data(pinfo->pool, pinfo, proto_usb, USB_DEVICE_SUBCLASS)) << 8;
    value |= GPOINTER_TO_UINT(p_get_proto_data(pinfo->pool, pinfo, proto_usb, USB_DEVICE_PROTOCOL));
    return GUINT_TO_POINTER(value);
}

static build_valid_func   usb_product_da_build_value[1] = {usb_product_value};
static decode_as_value_t  usb_product_da_values         = {usb_product_prompt, 1, usb_product_da_build_value};
static decode_as_t        usb_product_da = {
        "usb", "USB Product", "usb.product",
        1, 0, &usb_product_da_values, NULL, NULL,
        decode_as_default_populate_list, decode_as_default_reset,
        decode_as_default_change, NULL};

static build_valid_func   usb_device_da_build_value[1] = {usb_device_value};
static decode_as_value_t  usb_device_da_values         = {usb_device_prompt, 1, usb_device_da_build_value};
static decode_as_t        usb_device_da = {
        "usb", "USB Device", "usb.device",
        1, 0, &usb_device_da_values, NULL, NULL,
        decode_as_default_populate_list, decode_as_default_reset,
        decode_as_default_change, NULL};

static build_valid_func   usb_protocol_da_build_value[1] = {usb_protocol_value};
static decode_as_value_t  usb_protocol_da_values         = {usb_protocol_prompt, 1, usb_protocol_da_build_value};
static decode_as_t        usb_protocol_da = {
        "usb", "USB Device Protocol", "usb.protocol",
        1, 0, &usb_protocol_da_values, NULL, NULL,
        decode_as_default_populate_list, decode_as_default_reset,
        decode_as_default_change, NULL};


static usb_conv_info_t *
get_usb_conv_info(conversation_t *conversation)
{
    usb_conv_info_t *usb_conv_info;

    /* do we have conversation specific data ? */
    usb_conv_info = (usb_conv_info_t *)conversation_get_proto_data(conversation, proto_usb);
    if (!usb_conv_info) {
        /* no not yet so create some */
        usb_conv_info = wmem_new0(wmem_file_scope(), usb_conv_info_t);
        usb_conv_info->interfaceClass    = IF_CLASS_UNKNOWN;
        usb_conv_info->interfaceSubclass = IF_SUBCLASS_UNKNOWN;
        usb_conv_info->interfaceProtocol = IF_PROTOCOL_UNKNOWN;
        usb_conv_info->deviceVendor      = DEV_VENDOR_UNKNOWN;
        usb_conv_info->deviceProduct     = DEV_PRODUCT_UNKNOWN;
        usb_conv_info->alt_settings      = wmem_array_new(wmem_file_scope(), sizeof(usb_alt_setting_t));
        usb_conv_info->transactions      = wmem_tree_new(wmem_file_scope());

        conversation_add_proto_data(conversation, proto_usb, usb_conv_info);
    }

    return usb_conv_info;
}


/* usb_conv_info_t contains some components that are valid only for one specific packet
   clear_usb_conv_tmp_data() clears these components, it should be called
   before we dissect a new packet */
static void clear_usb_conv_tmp_data(usb_conv_info_t *usb_conv_info)
{
    /* caller must have checked that usb_conv_info!= NULL */

    usb_conv_info->direction = P2P_DIR_UNKNOWN;
    usb_conv_info->transfer_type = URB_UNKNOWN;
    usb_conv_info->is_request = FALSE;
    usb_conv_info->is_setup = FALSE;
    usb_conv_info->setup_requesttype = 0;

    /* when we parse the configuration, interface and endpoint
       descriptors, we store the current interface class in endpoint 0's
       conversation

       this must be cleared since endpoint 0 does not belong to any
       interface class

       we used to clear these info in dissect_usb_configuration_descriptor()
       this doesn't work when the descriptor parsing throws an exception */

    if (usb_conv_info->endpoint==0) {
        usb_conv_info->interfaceClass    = IF_CLASS_UNKNOWN;
        usb_conv_info->interfaceSubclass = IF_SUBCLASS_UNKNOWN;
        usb_conv_info->interfaceProtocol = IF_PROTOCOL_UNKNOWN;
    }
}

static conversation_t *
get_usb_conversation(packet_info *pinfo,
                     address *src_addr, address *dst_addr,
                     guint32 src_endpoint, guint32 dst_endpoint)
{
    conversation_t *conversation;

    /*
     * Do we have a conversation for this connection?
     */
    conversation = find_conversation(pinfo->num,
                               src_addr, dst_addr,
                               pinfo->ptype,
                               src_endpoint, dst_endpoint, 0);
    if (conversation) {
        return conversation;
    }

    /* We don't yet have a conversation, so create one. */
    conversation = conversation_new(pinfo->num,
                           src_addr, dst_addr,
                           pinfo->ptype,
                           src_endpoint, dst_endpoint, 0);
    return conversation;
}

/* Fetch or create usb_conv_info for a specified interface. */
usb_conv_info_t *
get_usb_iface_conv_info(packet_info *pinfo, guint8 interface_num)
{
    conversation_t *conversation;
    guint32 if_port;

    if_port = GUINT32_TO_LE(INTERFACE_PORT | interface_num);

    if (pinfo->srcport == NO_ENDPOINT) {
        conversation = get_usb_conversation(pinfo, &pinfo->src, &pinfo->dst, pinfo->srcport, if_port);
    } else {
        conversation = get_usb_conversation(pinfo, &pinfo->src, &pinfo->dst, if_port, pinfo->destport);
    }

    return get_usb_conv_info(conversation);
}

static const char* usb_conv_get_filter_type(conv_item_t* conv, conv_filter_type_e filter)
{
    if ((filter == CONV_FT_SRC_ADDRESS) && (conv->src_address.type == usb_address_type))
        return "usb.src";

    if ((filter == CONV_FT_DST_ADDRESS) && (conv->dst_address.type == usb_address_type))
        return "usb.dst";

    if ((filter == CONV_FT_ANY_ADDRESS) && (conv->src_address.type == usb_address_type))
        return "usb.addr";

    return CONV_FILTER_INVALID;
}

static ct_dissector_info_t usb_ct_dissector_info = {&usb_conv_get_filter_type};

static int
usb_conversation_packet(void *pct, packet_info *pinfo, epan_dissect_t *edt _U_, const void *vip _U_)
{
    conv_hash_t *hash = (conv_hash_t*) pct;
    add_conversation_table_data(hash, &pinfo->src, &pinfo->dst, 0, 0, 1, pinfo->fd->pkt_len, &pinfo->rel_ts, &pinfo->abs_ts, &usb_ct_dissector_info, PT_NONE);

    return 1;
}

static const char* usb_host_get_filter_type(hostlist_talker_t* host, conv_filter_type_e filter)
{
    if ((filter == CONV_FT_ANY_ADDRESS) && (host->myaddress.type == usb_address_type))
        return "usb.addr";

    return CONV_FILTER_INVALID;
}

static hostlist_dissector_info_t usb_host_dissector_info = {&usb_host_get_filter_type};

static int
usb_hostlist_packet(void *pit, packet_info *pinfo, epan_dissect_t *edt _U_, const void *vip _U_)
{
    conv_hash_t *hash = (conv_hash_t*) pit;

    /* Take two "add" passes per packet, adding for each direction, ensures that all
       packets are counted properly (even if address is sending to itself)
       XXX - this could probably be done more efficiently inside hostlist_table */
    add_hostlist_table_data(hash, &pinfo->src, 0, TRUE, 1, pinfo->fd->pkt_len, &usb_host_dissector_info, PT_NONE);
    add_hostlist_table_data(hash, &pinfo->dst, 0, FALSE, 1, pinfo->fd->pkt_len, &usb_host_dissector_info, PT_NONE);

    return 1;
}

/* SETUP dissectors */


/*
 * These dissectors are used to dissect the setup part and the data
 * for URB_CONTROL_INPUT / CLEAR FEATURE
 */


/* 9.4.1 */
static int
dissect_usb_setup_clear_feature_request(packet_info *pinfo _U_, proto_tree *tree,
                                        tvbuff_t *tvb, int offset,
                                        usb_conv_info_t  *usb_conv_info)
{
    guint8 recip;

    if (usb_conv_info) {
        recip = USB_RECIPIENT(usb_conv_info->usb_trans_info->setup.requesttype);

        /* feature selector, zero/interface/endpoint */
        switch (recip) {
        case RQT_SETUP_RECIPIENT_DEVICE:
            proto_tree_add_item(tree, hf_usb_device_wFeatureSelector, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(tree, hf_usb_index, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            break;

        case RQT_SETUP_RECIPIENT_INTERFACE:
            proto_tree_add_item(tree, hf_usb_interface_wFeatureSelector, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(tree, hf_usb_wInterface, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            break;

        case RQT_SETUP_RECIPIENT_ENDPOINT:
            proto_tree_add_item(tree, hf_usb_endpoint_wFeatureSelector, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(tree, hf_usb_wEndpoint, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            break;

        case RQT_SETUP_RECIPIENT_OTHER:
        default:
            proto_tree_add_item(tree, hf_usb_value, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(tree, hf_usb_index, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            break;
        }
    } else {
        /* No conversation information, so recipient type is unknown */
        proto_tree_add_item(tree, hf_usb_value, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;
        proto_tree_add_item(tree, hf_usb_index, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    }
    offset += 2;

    /* length */
    proto_tree_add_item(tree, hf_usb_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    return offset;
}

static int
dissect_usb_setup_clear_feature_response(packet_info *pinfo _U_, proto_tree *tree _U_,
                                         tvbuff_t *tvb _U_, int offset,
                                         usb_conv_info_t  *usb_conv_info _U_)
{
    return offset;
}


/*
 * These dissectors are used to dissect the setup part and the data
 * for URB_CONTROL_INPUT / GET CONFIGURATION
 */


/* 9.4.2 */
static int
dissect_usb_setup_get_configuration_response(packet_info *pinfo _U_, proto_tree *tree _U_,
                                             tvbuff_t *tvb _U_, int offset,
                                             usb_conv_info_t  *usb_conv_info _U_)
{
    proto_tree_add_item(tree, hf_usb_bConfigurationValue, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    return offset;
}


/*
 * These dissectors are used to dissect the setup part and the data
 * for URB_CONTROL_INPUT / GET DESCRIPTOR
 */

proto_item * dissect_usb_descriptor_header(proto_tree *tree,
                                           tvbuff_t *tvb, int offset,
                                           value_string_ext *type_val_str)
{
    guint8      desc_type;
    proto_item *length_item;
    proto_item *type_item;


    length_item = proto_tree_add_item(tree, hf_usb_bLength,
          tvb, offset, 1,  ENC_LITTLE_ENDIAN);
    offset++;

    desc_type = tvb_get_guint8(tvb, offset);
    type_item = proto_tree_add_item(tree, hf_usb_bDescriptorType,
          tvb, offset, 1, ENC_LITTLE_ENDIAN);
    /* if the caller provided no class specific value string, we're
     * using the standard descriptor types */
    if (!type_val_str)
          type_val_str = &std_descriptor_type_vals_ext;
    proto_item_append_text(type_item, " (%s)",
             val_to_str_ext(desc_type, type_val_str, "unknown"));

    return length_item;
}

/* 9.6.2 */
static int
dissect_usb_device_qualifier_descriptor(packet_info *pinfo _U_, proto_tree *parent_tree,
                                        tvbuff_t *tvb, int offset,
                                        usb_conv_info_t  *usb_conv_info)
{
    proto_item *item;
    proto_tree *tree;
    proto_item *nitem;
    int         old_offset = offset;
    guint32     protocol;
    const gchar *description;

    tree = proto_tree_add_subtree(parent_tree, tvb, offset, -1, ett_descriptor_device, &item, "DEVICE QUALIFIER DESCRIPTOR");

    dissect_usb_descriptor_header(tree, tvb, offset, NULL);
    offset += 2;

    /* bcdUSB */
    proto_tree_add_item(tree, hf_usb_bcdUSB, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    protocol = tvb_get_ntoh24(tvb, offset);
    description = val_to_str_ext_const(protocol, &usb_protocols_ext, "");

    /* bDeviceClass */
    proto_tree_add_item(tree, hf_usb_bDeviceClass, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    /* bDeviceSubClass */
    proto_tree_add_item(tree, hf_usb_bDeviceSubClass, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    /* bDeviceProtocol */
    nitem = proto_tree_add_item(tree, hf_usb_bDeviceProtocol, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    if (*description)
        proto_item_append_text(nitem, " (%s)", description);
    offset += 1;

    if (!pinfo->fd->flags.visited) {
        guint                   k_bus_id;
        guint                   k_device_address;
        guint                   k_frame_number;
        wmem_tree_key_t         key[4];
        device_protocol_data_t  *device_protocol_data;

        k_frame_number = pinfo->num;
        k_device_address = usb_conv_info->device_address;
        k_bus_id = usb_conv_info->bus_id;

        key[0].length = 1;
        key[0].key    = &k_device_address;
        key[1].length = 1;
        key[1].key    = &k_bus_id;
        key[2].length = 1;
        key[2].key    = &k_frame_number;
        key[3].length = 0;
        key[3].key    = NULL;

        device_protocol_data = wmem_new(wmem_file_scope(), device_protocol_data_t);
        device_protocol_data->protocol = protocol;
        device_protocol_data->bus_id = usb_conv_info->bus_id;
        device_protocol_data->device_address = usb_conv_info->device_address;
        wmem_tree_insert32_array(device_to_protocol_table, key, device_protocol_data);
    }

    /* bMaxPacketSize0 */
    proto_tree_add_item(tree, hf_usb_bMaxPacketSize0, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    /* bNumConfigurations */
    proto_tree_add_item(tree, hf_usb_bNumConfigurations, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    /* one reserved byte */
    offset += 1;

    proto_item_set_len(item, offset-old_offset);

    return offset;
}

/* 9.6.1 */
static int
dissect_usb_device_descriptor(packet_info *pinfo, proto_tree *parent_tree,
                              tvbuff_t *tvb, int offset,
                              usb_conv_info_t *usb_conv_info)
{
    proto_item        *item;
    proto_tree        *tree;
    proto_item        *nitem;
    int                old_offset = offset;
    guint32            protocol;
    const gchar       *description;
    guint32            vendor_id;
    guint32            product;
    guint16            product_id;

    tree = proto_tree_add_subtree(parent_tree, tvb, offset, -1, ett_descriptor_device, &item, "DEVICE DESCRIPTOR");

    dissect_usb_descriptor_header(tree, tvb, offset, NULL);
    offset += 2;

    /* bcdUSB */
    proto_tree_add_item(tree, hf_usb_bcdUSB, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    protocol = tvb_get_ntoh24(tvb, offset);
    description = val_to_str_ext_const(protocol, &usb_protocols_ext, "");

    /* bDeviceClass */
    proto_tree_add_item(tree, hf_usb_bDeviceClass, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    /* bDeviceSubClass */
    proto_tree_add_item(tree, hf_usb_bDeviceSubClass, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    /* bDeviceProtocol */
    nitem = proto_tree_add_item(tree, hf_usb_bDeviceProtocol, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    if (*description)
        proto_item_append_text(nitem, " (%s)", description);
    offset += 1;

    /* bMaxPacketSize0 */
    proto_tree_add_item(tree, hf_usb_bMaxPacketSize0, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    /* if request was only for the first 8 bytes */
    /* per 5.5.3 of USB2.0 Spec */
    if (8 == usb_conv_info->usb_trans_info->setup.wLength) {
        proto_item_set_len(item, offset-old_offset);
        return offset;
    }

    /* idVendor */
    proto_tree_add_item_ret_uint(tree, hf_usb_idVendor, tvb, offset, 2, ENC_LITTLE_ENDIAN, &vendor_id);
    usb_conv_info->deviceVendor = (guint16)vendor_id;
    offset += 2;

    /* idProduct */
    product_id = tvb_get_letohs(tvb, offset);
    usb_conv_info->deviceProduct = product_id;
    product = (guint16)vendor_id << 16 | product_id;

    proto_tree_add_uint_format_value(tree, hf_usb_idProduct, tvb, offset, 2, product_id, "%s (0x%04x)",
                                     val_to_str_ext_const(product, &ext_usb_products_vals, "Unknown"),
                                     product_id);
    offset += 2;

    if (!pinfo->fd->flags.visited) {
        guint                   k_bus_id;
        guint                   k_device_address;
        guint                   k_frame_number;
        wmem_tree_key_t         key[4];
        device_product_data_t   *device_product_data;
        device_protocol_data_t  *device_protocol_data;

        k_frame_number = pinfo->num;
        k_device_address = usb_conv_info->device_address;
        k_bus_id = usb_conv_info->bus_id;

        key[0].length = 1;
        key[0].key    = &k_device_address;
        key[1].length = 1;
        key[1].key    = &k_bus_id;
        key[2].length = 1;
        key[2].key    = &k_frame_number;
        key[3].length = 0;
        key[3].key    = NULL;

        device_product_data = wmem_new(wmem_file_scope(), device_product_data_t);
        device_product_data->vendor = vendor_id;
        device_product_data->product = product_id;
        device_product_data->bus_id = usb_conv_info->bus_id;
        device_product_data->device_address = usb_conv_info->device_address;
        wmem_tree_insert32_array(device_to_product_table, key, device_product_data);

        device_protocol_data = wmem_new(wmem_file_scope(), device_protocol_data_t);
        device_protocol_data->protocol = protocol;
        device_protocol_data->bus_id = usb_conv_info->bus_id;
        device_protocol_data->device_address = usb_conv_info->device_address;

        wmem_tree_insert32_array(device_to_protocol_table, key, device_protocol_data);
    }

    /* bcdDevice */
    proto_tree_add_item(tree, hf_usb_bcdDevice, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    /* iManufacturer */
    proto_tree_add_item(tree, hf_usb_iManufacturer, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    /* iProduct */
    proto_tree_add_item(tree, hf_usb_iProduct, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    /* iSerialNumber */
    proto_tree_add_item(tree, hf_usb_iSerialNumber, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    /* bNumConfigurations */
    proto_tree_add_item(tree, hf_usb_bNumConfigurations, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    proto_item_set_len(item, offset-old_offset);

    return offset;
}

/* 9.6.7 */
static int
dissect_usb_string_descriptor(packet_info *pinfo _U_, proto_tree *parent_tree,
                              tvbuff_t *tvb, int offset,
                              usb_conv_info_t  *usb_conv_info)
{
    proto_item *item;
    proto_tree *tree;
    int         old_offset = offset;
    guint8      len;
    proto_item *len_item;
    usb_trans_info_t *usb_trans_info;

    usb_trans_info = usb_conv_info->usb_trans_info;

    tree = proto_tree_add_subtree(parent_tree, tvb, offset, -1, ett_descriptor_device, &item, "STRING DESCRIPTOR");

    len = tvb_get_guint8(tvb, offset);
    /* The USB spec says that the languages / the string are UTF16 and not
       0-terminated, i.e. the length field must contain an even number */
    if (len & 0x1) {
        /* bLength */
        len_item = proto_tree_add_item(tree, hf_usb_bLength, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        expert_add_info(pinfo, len_item, &ei_usb_bLength_even);

        /* bDescriptorType */
        proto_tree_add_item(tree, hf_usb_bDescriptorType, tvb, offset+1, 1, ENC_LITTLE_ENDIAN);
    }
    else
       len_item = dissect_usb_descriptor_header(tree, tvb, offset, NULL);
    offset += 2;

    /* Report an error, and give up, if the length is < 2 */
    if (len < 2) {
        expert_add_info(pinfo, len_item, &ei_usb_bLength_too_short);
        return offset;
    }

    if (!usb_trans_info->u.get_descriptor.usb_index) {
        /* list of languanges */
        while (offset >= old_offset && len > (offset - old_offset)) {
            /* wLANGID */
            proto_tree_add_item(tree, hf_usb_wLANGID, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset+=2;
        }
    } else {
        /* UTF-16 string */
        /* handle case of host requesting only substring */
        guint8 len_str = MIN(len-2, usb_trans_info->setup.wLength -2);
        proto_tree_add_item(tree, hf_usb_bString, tvb, offset, len_str, ENC_UTF_16 | ENC_LITTLE_ENDIAN);
        offset += len_str;
    }

    proto_item_set_len(item, offset-old_offset);

    return offset;
}



/* 9.6.5 */
static int
dissect_usb_interface_descriptor(packet_info *pinfo, proto_tree *parent_tree,
                                 tvbuff_t *tvb, int offset,
                                 usb_conv_info_t  *usb_conv_info)
{
    proto_item       *item;
    proto_tree       *tree;
    const char       *class_str  = NULL;
    int               old_offset = offset;
    guint8            len;
    guint8            interface_num;
    guint8            alt_setting;
    usb_trans_info_t *usb_trans_info;

    usb_trans_info = usb_conv_info->usb_trans_info;

    tree = proto_tree_add_subtree(parent_tree, tvb, offset, -1, ett_descriptor_device, &item, "INTERFACE DESCRIPTOR");

    len = tvb_get_guint8(tvb, offset);
    dissect_usb_descriptor_header(tree, tvb, offset, NULL);
    offset += 2;

    /* bInterfaceNumber */
    interface_num = tvb_get_guint8(tvb, offset);
    proto_tree_add_item(tree, hf_usb_bInterfaceNumber, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    usb_conv_info->interfaceNum = interface_num;
    offset += 1;

    /* bAlternateSetting */
    alt_setting = tvb_get_guint8(tvb, offset);
    proto_tree_add_item(tree, hf_usb_bAlternateSetting, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    /* bNumEndpoints */
    proto_tree_add_item(tree, hf_usb_bNumEndpoints, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    /* bInterfaceClass */
    proto_tree_add_item(tree, hf_usb_bInterfaceClass, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    /* save the class so we can access it later in the endpoint descriptor */
    usb_conv_info->interfaceClass = tvb_get_guint8(tvb, offset);

    class_str = val_to_str_ext(usb_conv_info->interfaceClass, &usb_class_vals_ext, "unknown (0x%X)");
    proto_item_append_text(item, " (%u.%u): class %s", interface_num, alt_setting, class_str);

    if (!pinfo->fd->flags.visited) {
        usb_alt_setting_t alternate_setting;

        /* Register conversation for this interface in case CONTROL messages are sent to it */
        usb_trans_info->interface_info = get_usb_iface_conv_info(pinfo, interface_num);

        alternate_setting.altSetting = alt_setting;
        alternate_setting.interfaceClass = tvb_get_guint8(tvb, offset);
        alternate_setting.interfaceSubclass = tvb_get_guint8(tvb, offset+1);
        alternate_setting.interfaceProtocol = tvb_get_guint8(tvb, offset+2);
        wmem_array_append_one(usb_trans_info->interface_info->alt_settings, alternate_setting);

        if (alt_setting == 0) {
            /* By default let's assume alternate setting 0 will be used */

            /* in interface conversations, endpoint has no meaning */
            usb_trans_info->interface_info->endpoint = NO_ENDPOINT8;

            usb_trans_info->interface_info->interfaceClass = alternate_setting.interfaceClass;
            usb_trans_info->interface_info->interfaceSubclass = alternate_setting.interfaceSubclass;
            usb_trans_info->interface_info->interfaceProtocol = alternate_setting.interfaceProtocol;
            usb_trans_info->interface_info->deviceVendor      = usb_conv_info->deviceVendor;
            usb_trans_info->interface_info->deviceProduct     = usb_conv_info->deviceProduct;
        }
    }
    offset += 1;

    /* bInterfaceSubClass */
    switch (usb_conv_info->interfaceClass) {
    case IF_CLASS_COMMUNICATIONS:
        proto_tree_add_item(tree, hf_usb_bInterfaceSubClass_cdc, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        break;
    case IF_CLASS_HID:
        proto_tree_add_item(tree, hf_usb_bInterfaceSubClass_hid, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        break;
    case IF_CLASS_MISCELLANEOUS:
        proto_tree_add_item(tree, hf_usb_bInterfaceSubClass_misc, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        break;
    case IF_CLASS_APPLICATION_SPECIFIC:
        proto_tree_add_item(tree, hf_usb_bInterfaceSubClass_app, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        break;
    default:
        proto_tree_add_item(tree, hf_usb_bInterfaceSubClass, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    }

    /* save the subclass so we can access it later in class-specific descriptors */
    usb_conv_info->interfaceSubclass = tvb_get_guint8(tvb, offset);
    offset += 1;

    /* bInterfaceProtocol */
    switch (usb_conv_info->interfaceClass) {
    case IF_CLASS_COMMUNICATIONS:
        proto_tree_add_item(tree, hf_usb_bInterfaceProtocol_cdc, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        break;
    case IF_CLASS_CDC_DATA:
        proto_tree_add_item(tree, hf_usb_bInterfaceProtocol_cdc_data, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        break;
    case IF_CLASS_APPLICATION_SPECIFIC:
        switch (usb_conv_info->interfaceSubclass) {
        case 0x01:
            proto_tree_add_item(tree, hf_usb_bInterfaceProtocol_app_dfu, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            break;
        case 0x02:
            proto_tree_add_item(tree, hf_usb_bInterfaceProtocol_app_irda, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            break;
        case 0x03:
            proto_tree_add_item(tree, hf_usb_bInterfaceProtocol_app_usb_test_and_measurement, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            break;
        default:
            proto_tree_add_item(tree, hf_usb_bInterfaceProtocol, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        }
        break;
    case IF_CLASS_HID:
        if (usb_conv_info->interfaceSubclass == 1) {
            proto_tree_add_item(tree, hf_usb_bInterfaceProtocol_hid_boot, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            break;
        }

        proto_tree_add_item(tree, hf_usb_bInterfaceProtocol, tvb, offset, 1, ENC_LITTLE_ENDIAN);

        break;
    default:
        proto_tree_add_item(tree, hf_usb_bInterfaceProtocol, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    }

    usb_conv_info->interfaceProtocol = tvb_get_guint8(tvb, offset);
    offset += 1;

    /* iInterface */
    proto_tree_add_item(tree, hf_usb_iInterface, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    proto_item_set_len(item, len);

    if (offset < old_offset+len) {
        /* skip unknown records */
        offset = old_offset + len;
    }

    return offset;
}

/* 9.6.6 */
const true_false_string tfs_endpoint_direction = {
    "IN Endpoint",
    "OUT Endpoint"
};

void dissect_usb_endpoint_address(proto_tree *tree, tvbuff_t *tvb, int offset)
{
    proto_item *endpoint_item;
    proto_tree *endpoint_tree;
    guint8      endpoint;

    endpoint_item = proto_tree_add_item(tree, hf_usb_bEndpointAddress, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    endpoint_tree = proto_item_add_subtree(endpoint_item, ett_configuration_bEndpointAddress);

    endpoint = tvb_get_guint8(tvb, offset)&0x0f;
    proto_tree_add_item(endpoint_tree, hf_usb_bEndpointAddress_direction, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    proto_item_append_text(endpoint_item, "  %s", (tvb_get_guint8(tvb, offset)&0x80)?"IN":"OUT");
    proto_tree_add_item(endpoint_tree, hf_usb_bEndpointAddress_number, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    proto_item_append_text(endpoint_item, "  Endpoint:%d", endpoint);
}

int
dissect_usb_endpoint_descriptor(packet_info *pinfo, proto_tree *parent_tree,
                                tvbuff_t *tvb, int offset,
                                usb_conv_info_t  *usb_conv_info)
{
    proto_item       *item;
    proto_tree       *tree;
    proto_item       *ep_attrib_item;
    proto_tree       *ep_attrib_tree;
    proto_item       *ep_pktsize_item;
    proto_tree       *ep_pktsize_tree;
    int               old_offset     = offset;
    guint8            endpoint;
    guint8            ep_type;
    guint8            len;
    usb_trans_info_t *usb_trans_info = NULL;

    if (usb_conv_info)
        usb_trans_info = usb_conv_info->usb_trans_info;

    tree = proto_tree_add_subtree(parent_tree, tvb, offset, -1, ett_descriptor_device, &item, "ENDPOINT DESCRIPTOR");

    len = tvb_get_guint8(tvb, offset);
    dissect_usb_descriptor_header(tree, tvb, offset, NULL);
    offset += 2;

    endpoint = tvb_get_guint8(tvb, offset)&0x0f;
    dissect_usb_endpoint_address(tree, tvb, offset);
    offset += 1;

    /* Together with class from the interface descriptor we know what kind
     * of class the device at endpoint is.
     * Make sure a conversation exists for this endpoint and attach a
     * usb_conv_into_t structure to it.
     *
     * All endpoints for the same interface descriptor share the same
     * usb_conv_info structure.
     */
    if ((!pinfo->fd->flags.visited) && usb_trans_info && usb_trans_info->interface_info) {
        conversation_t *conversation = NULL;

        if (pinfo->destport == NO_ENDPOINT) {
            address tmp_addr;
            usb_address_t *usb_addr = wmem_new0(wmem_packet_scope(), usb_address_t);

            /* packet is sent from a USB device's endpoint 0 to the host
             * replace endpoint 0 with the endpoint of this descriptor
             * and find the corresponding conversation
             */
            usb_addr->bus_id = ((const usb_address_t *)(pinfo->src.data))->bus_id;
            usb_addr->device = ((const usb_address_t *)(pinfo->src.data))->device;
            usb_addr->endpoint = GUINT32_TO_LE(endpoint);
            set_address(&tmp_addr, usb_address_type, USB_ADDR_LEN, (char *)usb_addr);
            conversation = get_usb_conversation(pinfo, &tmp_addr, &pinfo->dst, usb_addr->endpoint, pinfo->destport);
        }

        if (conversation) {
            usb_trans_info->interface_info->endpoint = endpoint;
            conversation_add_proto_data(conversation, proto_usb, usb_trans_info->interface_info);
        }
    }

    /* bmAttributes */
    ep_type = ENDPOINT_TYPE(tvb_get_guint8(tvb, offset));

    ep_attrib_item = proto_tree_add_item(tree, hf_usb_bmAttributes, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    ep_attrib_tree = proto_item_add_subtree(ep_attrib_item, ett_endpoint_bmAttributes);

    proto_tree_add_item(ep_attrib_tree, hf_usb_bEndpointAttributeTransfer, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    if (ep_type==USB_EP_ISOCHRONOUS) {
        proto_tree_add_item(ep_attrib_tree, hf_usb_bEndpointAttributeSynchonisation, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        proto_tree_add_item(ep_attrib_tree, hf_usb_bEndpointAttributeBehaviour, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    }
    offset += 1;

    /* wMaxPacketSize */
    ep_pktsize_item = proto_tree_add_item(tree, hf_usb_wMaxPacketSize, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    ep_pktsize_tree = proto_item_add_subtree(ep_pktsize_item, ett_endpoint_wMaxPacketSize);
    if ((ep_type == ENDPOINT_TYPE_INTERRUPT) || (ep_type == ENDPOINT_TYPE_ISOCHRONOUS)) {
        proto_tree_add_item(ep_pktsize_tree, hf_usb_wMaxPacketSize_slots, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    }
    proto_tree_add_item(ep_pktsize_tree, hf_usb_wMaxPacketSize_size, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset+=2;

    /* bInterval */
    proto_tree_add_item(tree, hf_usb_bInterval, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    proto_item_set_len(item, len);

    if (offset < old_offset+len) {
        /* skip unknown records */
        offset = old_offset + len;
    }

    return offset;
}

/* ECN */
static int
dissect_usb_interface_assn_descriptor(packet_info *pinfo _U_, proto_tree *parent_tree,
                                      tvbuff_t *tvb, int offset,
                                      usb_conv_info_t  *usb_conv_info _U_)
{
    proto_item *item;
    proto_tree *tree;
    int         old_offset = offset;

    tree = proto_tree_add_subtree(parent_tree, tvb, offset, -1, ett_descriptor_device, &item, "INTERFACE ASSOCIATION DESCRIPTOR");

    dissect_usb_descriptor_header(tree, tvb, offset, NULL);
    offset += 2;

    /* bFirstInterface */
    proto_tree_add_item(tree, hf_usb_bFirstInterface, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    /* bInterfaceCount */
    proto_tree_add_item(tree, hf_usb_bInterfaceCount, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    /* bFunctionClass */
    proto_tree_add_item(tree, hf_usb_bFunctionClass, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    /* bFunctionSubclass */
    proto_tree_add_item(tree, hf_usb_bFunctionSubClass, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    /* bFunctionProtocol */
    proto_tree_add_item(tree, hf_usb_bFunctionProtocol, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    /* iFunction */
    proto_tree_add_item(tree, hf_usb_iFunction, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    proto_item_set_len(item, offset-old_offset);

    return offset;
}

int
dissect_usb_unknown_descriptor(packet_info *pinfo _U_, proto_tree *parent_tree,
                               tvbuff_t *tvb, int offset,
                               usb_conv_info_t  *usb_conv_info _U_)
{
    proto_item *item;
    proto_tree *tree;
    guint8      bLength;

    tree = proto_tree_add_subtree(parent_tree, tvb, offset, -1, ett_descriptor_device, &item, "UNKNOWN DESCRIPTOR");

    bLength = tvb_get_guint8(tvb, offset);
    dissect_usb_descriptor_header(tree, tvb, offset, NULL);
    offset += bLength;

    proto_item_set_len(item, bLength);

    return offset;
}

/* 9.6.3 */
static const true_false_string tfs_mustbeone = {
    "Must be 1 for USB 1.1 and higher",
    "FIXME: Is this a USB 1.0 device"
};
static const true_false_string tfs_selfpowered = {
    "This device is SELF-POWERED",
    "This device is powered from the USB bus"
};
static const true_false_string tfs_remotewakeup = {
    "This device supports REMOTE WAKEUP",
    "This device does NOT support remote wakeup"
};
static int
dissect_usb_configuration_descriptor(packet_info *pinfo _U_, proto_tree *parent_tree,
                                     tvbuff_t *tvb, int offset,
                                     usb_conv_info_t  *usb_conv_info)
{
    proto_item *item;
    proto_tree *tree;
    int         old_offset = offset;
    guint16     len;
    proto_item *flags_item;
    proto_tree *flags_tree;
    guint8      flags;
    proto_item *power_item;
    guint8      power;
    gboolean    truncation_expected;
    usb_trans_info_t *usb_trans_info;

    usb_trans_info = usb_conv_info->usb_trans_info;

    usb_conv_info->interfaceClass    = IF_CLASS_UNKNOWN;
    usb_conv_info->interfaceSubclass = IF_SUBCLASS_UNKNOWN;
    usb_conv_info->interfaceProtocol = IF_PROTOCOL_UNKNOWN;

    tree = proto_tree_add_subtree(parent_tree, tvb, offset, -1, ett_descriptor_device, &item, "CONFIGURATION DESCRIPTOR");

    dissect_usb_descriptor_header(tree, tvb, offset, NULL);
    offset += 2;

    /* wTotalLength */
    proto_tree_add_item(tree, hf_usb_wTotalLength, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    len = tvb_get_letohs(tvb, offset);
    offset+=2;

    /* bNumInterfaces */
    proto_tree_add_item(tree, hf_usb_bNumInterfaces, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    /* bConfigurationValue */
    proto_tree_add_item(tree, hf_usb_bConfigurationValue, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    /* iConfiguration */
    proto_tree_add_item(tree, hf_usb_iConfiguration, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    /* bmAttributes */
    flags_item = proto_tree_add_item(tree, hf_usb_configuration_bmAttributes, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    flags_tree = proto_item_add_subtree(flags_item, ett_configuration_bmAttributes);

    flags = tvb_get_guint8(tvb, offset);
    proto_tree_add_item(flags_tree, hf_usb_configuration_legacy10buspowered, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    proto_tree_add_item(flags_tree, hf_usb_configuration_selfpowered, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    proto_item_append_text(flags_item, "  %sSELF-POWERED", (flags&0x40)?"":"NOT ");
    proto_tree_add_item(flags_tree, hf_usb_configuration_remotewakeup, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    proto_item_append_text(flags_item, "  %sREMOTE-WAKEUP", (flags&0x20)?"":"NO ");
    offset += 1;

    /* bMaxPower */
    power_item = proto_tree_add_item(tree, hf_usb_bMaxPower, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    power = tvb_get_guint8(tvb, offset);
    proto_item_append_text(power_item, "  (%dmA)", power*2);
    offset += 1;

    /* initialize interface_info to NULL */
    usb_trans_info->interface_info = NULL;

    truncation_expected = (usb_trans_info->setup.wLength < len);

    /* decode any additional interface and endpoint descriptors */
    while(len>(offset-old_offset)) {
        guint8 next_type;
        guint8 next_len = 0;
        gint remaining_tvb, remaining_len;
        tvbuff_t *next_tvb = NULL;

        /* Handle truncated descriptors appropriately */
        remaining_tvb = tvb_reported_length_remaining(tvb, offset);
        if (remaining_tvb > 0) {
            next_len  = tvb_get_guint8(tvb, offset);
            remaining_len = len - (offset - old_offset);
            if ((next_len < 3) || (next_len > remaining_len)) {
                proto_tree_add_expert_format(parent_tree, pinfo, &ei_usb_desc_length_invalid,
                    tvb, offset, 1, "Invalid descriptor length: %u",  next_len);
                item = NULL;
                break;
            }
        }

        if ((remaining_tvb == 0) || (next_len > remaining_tvb)) {
            if (truncation_expected)
                break;
        }

        next_type = tvb_get_guint8(tvb, offset+1);
        switch(next_type) {
        case USB_DT_INTERFACE:
            offset = dissect_usb_interface_descriptor(pinfo, parent_tree, tvb, offset, usb_conv_info);
            break;
        case USB_DT_ENDPOINT:
            offset = dissect_usb_endpoint_descriptor(pinfo, parent_tree, tvb, offset, usb_conv_info);
            break;
        case USB_DT_INTERFACE_ASSOCIATION:
            offset = dissect_usb_interface_assn_descriptor(pinfo, parent_tree, tvb, offset, usb_conv_info);
            break;
        default:
            next_tvb = tvb_new_subset_length(tvb, offset, next_len);
            if (dissector_try_uint_new(usb_descriptor_dissector_table, usb_conv_info->interfaceClass, next_tvb, pinfo, parent_tree, TRUE, usb_conv_info)) {
                offset += next_len;
            } else {
                offset = dissect_usb_unknown_descriptor(pinfo, parent_tree, tvb, offset, usb_conv_info);
            }
            break;
            /* was: return offset; */
        }
    }

    proto_item_set_len(item, offset-old_offset);

    return offset;
}

/* 9.4.3 */
static int
dissect_usb_setup_get_descriptor_request(packet_info *pinfo, proto_tree *tree,
                                         tvbuff_t *tvb, int offset,
                                         usb_conv_info_t  *usb_conv_info)
{
    usb_trans_info_t *usb_trans_info, trans_info;

    if (usb_conv_info)
        usb_trans_info = usb_conv_info->usb_trans_info;
    else
        usb_trans_info = &trans_info;

    /* descriptor index */
    proto_tree_add_item(tree, hf_usb_descriptor_index, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    usb_trans_info->u.get_descriptor.usb_index = tvb_get_guint8(tvb, offset);
    offset += 1;

    /* descriptor type */
    proto_tree_add_item(tree, hf_usb_bDescriptorType, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    usb_trans_info->u.get_descriptor.type = tvb_get_guint8(tvb, offset);
    offset += 1;
    col_append_fstr(pinfo->cinfo, COL_INFO, " %s",
        val_to_str_ext(usb_trans_info->u.get_descriptor.type, &std_descriptor_type_vals_ext, "Unknown type %u"));

    /* language id */
    proto_tree_add_item(tree, hf_usb_language_id, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset+=2;

    /* length */
    proto_tree_add_item(tree, hf_usb_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    return offset;
}

static int
dissect_usb_setup_get_descriptor_response(packet_info *pinfo, proto_tree *tree,
                                          tvbuff_t *tvb, int offset,
                                          usb_conv_info_t  *usb_conv_info)
{
    usb_trans_info_t *usb_trans_info;

    usb_trans_info = usb_conv_info->usb_trans_info;

    col_append_fstr(pinfo->cinfo, COL_INFO, " %s",
        val_to_str_ext(usb_trans_info->u.get_descriptor.type, &std_descriptor_type_vals_ext, "Unknown type %u"));

    switch(usb_trans_info->u.get_descriptor.type) {
        case USB_DT_INTERFACE:
        case USB_DT_ENDPOINT:
            /* an interface or an endpoint descriptor can only be accessed
               as part of a configuration descriptor */
            break;
        case USB_DT_DEVICE:
            offset = dissect_usb_device_descriptor(pinfo, tree, tvb, offset, usb_conv_info);
            break;
        case USB_DT_CONFIG:
            offset = dissect_usb_configuration_descriptor(pinfo, tree, tvb, offset, usb_conv_info);
            break;
        case USB_DT_STRING:
            offset = dissect_usb_string_descriptor(pinfo, tree, tvb, offset, usb_conv_info);
            break;
        case USB_DT_DEVICE_QUALIFIER:
            offset = dissect_usb_device_qualifier_descriptor(pinfo, tree, tvb, offset, usb_conv_info);
            break;
        default:
            /* XXX dissect the descriptor coming back from the device */
            {
                guint len = tvb_reported_length_remaining(tvb, offset);
                proto_tree_add_bytes_format(tree, hf_usb_get_descriptor_resp_generic, tvb, offset, len, NULL,
                                            "GET DESCRIPTOR Response data (unknown descriptor type %u): %s",
                                            usb_trans_info->u.get_descriptor.type,
                                            tvb_bytes_to_str(wmem_packet_scope(), tvb, offset, len));
                offset = offset + len;
            }
            break;
    }

    return offset;
}


/*
 * These dissectors are used to dissect the setup part and the data
 * for URB_CONTROL_INPUT / GET INTERFACE
 */


/* 9.4.4 */
static int
dissect_usb_setup_get_interface_request(packet_info *pinfo _U_, proto_tree *tree,
                                        tvbuff_t *tvb, int offset,
                                        usb_conv_info_t  *usb_conv_info _U_)
{
    /* zero */
    proto_tree_add_item(tree, hf_usb_value, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    /* interface */
    proto_tree_add_item(tree, hf_usb_wInterface, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    /* length */
    proto_tree_add_item(tree, hf_usb_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    return offset;
}

static int
dissect_usb_setup_get_interface_response(packet_info *pinfo _U_, proto_tree *tree,
                                         tvbuff_t *tvb, int offset,
                                         usb_conv_info_t  *usb_conv_info _U_)
{
    /* alternate setting */
    proto_tree_add_item(tree, hf_usb_bAlternateSetting, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    return offset;
}


/*
 * These dissectors are used to dissect the setup part and the data
 * for URB_CONTROL_INPUT / GET STATUS
 */


/* 9.4.5 */
static int
dissect_usb_setup_get_status_request(packet_info *pinfo _U_, proto_tree *tree,
                                     tvbuff_t *tvb, int offset,
                                     usb_conv_info_t  *usb_conv_info _U_)
{
    /* zero */
    proto_tree_add_item(tree, hf_usb_value, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    /* zero/interface/endpoint */
    if (usb_conv_info) {
        guint8 recip;

        recip = USB_RECIPIENT(usb_conv_info->usb_trans_info->setup.requesttype);

        switch (recip) {
        case RQT_SETUP_RECIPIENT_INTERFACE:
            proto_tree_add_item(tree, hf_usb_wInterface, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            break;

        case RQT_SETUP_RECIPIENT_ENDPOINT:
            proto_tree_add_item(tree, hf_usb_wEndpoint, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            break;

        case RQT_SETUP_RECIPIENT_DEVICE:
        case RQT_SETUP_RECIPIENT_OTHER:
        default:
            proto_tree_add_item(tree, hf_usb_index, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            break;
        }
    } else {
        proto_tree_add_item(tree, hf_usb_index, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    }
    offset += 2;

    /* length */
    proto_tree_add_item(tree, hf_usb_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    return offset;
}

static int
dissect_usb_setup_get_status_response(packet_info *pinfo _U_, proto_tree *tree,
                                      tvbuff_t *tvb, int offset,
                                      usb_conv_info_t  *usb_conv_info _U_)
{
    /* status */
    /* XXX - show bits */
    proto_tree_add_item(tree, hf_usb_wStatus, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    return offset;
}


/*
 * These dissectors are used to dissect the setup part and the data
 * for URB_CONTROL_INPUT / SET ADDRESS
 */


/* 9.4.6 */
static int
dissect_usb_setup_set_address_request(packet_info *pinfo _U_, proto_tree *tree,
                                      tvbuff_t *tvb, int offset,
                                      usb_conv_info_t  *usb_conv_info _U_)
{
    /* device address */
    proto_tree_add_item(tree, hf_usb_device_address, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    /* zero */
    proto_tree_add_item(tree, hf_usb_index, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    /* zero */
    proto_tree_add_item(tree, hf_usb_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    return offset;
}

static int
dissect_usb_setup_set_address_response(packet_info *pinfo _U_, proto_tree *tree _U_,
                                       tvbuff_t *tvb _U_, int offset,
                                       usb_conv_info_t  *usb_conv_info _U_)
{
    return offset;
}


/*
 * These dissectors are used to dissect the setup part and the data
 * for URB_CONTROL_INPUT / SET CONFIGURATION
 */


/* 9.4.7 */
static int
dissect_usb_setup_set_configuration_request(packet_info *pinfo _U_, proto_tree *tree,
                                            tvbuff_t *tvb, int offset,
                                            usb_conv_info_t  *usb_conv_info _U_)
{
    /* configuration value */
    proto_tree_add_item(tree, hf_usb_bConfigurationValue, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 2;

    /* zero */
    proto_tree_add_item(tree, hf_usb_index, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    /* zero */
    proto_tree_add_item(tree, hf_usb_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    return offset;
}

static int
dissect_usb_setup_set_configuration_response(packet_info *pinfo _U_, proto_tree *tree _U_,
                                             tvbuff_t *tvb _U_, int offset,
                                             usb_conv_info_t  *usb_conv_info _U_)
{
    return offset;
}


/*
 * These dissectors are used to dissect the setup part and the data
 * for URB_CONTROL_INPUT / SET FEATURE
 */


/* 9.4.9 */
static int
dissect_usb_setup_set_feature_request(packet_info *pinfo _U_, proto_tree *tree,
                                      tvbuff_t *tvb, int offset,
                                      usb_conv_info_t  *usb_conv_info)
{
    guint8 recip;

    if (usb_conv_info) {
        recip = USB_RECIPIENT(usb_conv_info->usb_trans_info->setup.requesttype);

        /* feature selector, zero/interface/endpoint */
        switch (recip) {
        case RQT_SETUP_RECIPIENT_DEVICE:
            proto_tree_add_item(tree, hf_usb_device_wFeatureSelector, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(tree, hf_usb_index, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            break;

        case RQT_SETUP_RECIPIENT_INTERFACE:
            proto_tree_add_item(tree, hf_usb_interface_wFeatureSelector, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(tree, hf_usb_wInterface, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            break;

        case RQT_SETUP_RECIPIENT_ENDPOINT:
            proto_tree_add_item(tree, hf_usb_endpoint_wFeatureSelector, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(tree, hf_usb_wEndpoint, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            break;

        case RQT_SETUP_RECIPIENT_OTHER:
        default:
            proto_tree_add_item(tree, hf_usb_value, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(tree, hf_usb_index, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            break;
        }
    } else {
        /* No conversation information, so recipient type is unknown */
        proto_tree_add_item(tree, hf_usb_value, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;
        proto_tree_add_item(tree, hf_usb_index, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    }
    offset += 2;

    /* zero */
    proto_tree_add_item(tree, hf_usb_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    return offset;
}

static int
dissect_usb_setup_set_feature_response(packet_info *pinfo _U_, proto_tree *tree _U_,
                                       tvbuff_t *tvb _U_, int offset,
                                       usb_conv_info_t  *usb_conv_info _U_)
{
    return offset;
}


/*
 * These dissectors are used to dissect the setup part and the data
 * for URB_CONTROL_INPUT / SET INTERFACE
 */


/* 9.4.10 */
static int
dissect_usb_setup_set_interface_request(packet_info *pinfo, proto_tree *tree,
                                        tvbuff_t *tvb, int offset,
                                        usb_conv_info_t  *usb_conv_info _U_)
{
    guint8 alt_setting, interface_num;

    /* alternate setting */
    alt_setting = tvb_get_guint8(tvb, offset);
    proto_tree_add_uint(tree, hf_usb_bAlternateSetting, tvb, offset, 2, alt_setting);
    offset += 2;

    /* interface */
    interface_num = tvb_get_guint8(tvb, offset);
    proto_tree_add_uint(tree, hf_usb_wInterface, tvb, offset, 2, interface_num);
    offset += 2;

    /* zero */
    proto_tree_add_item(tree, hf_usb_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    if (!PINFO_FD_VISITED(pinfo)) {
        guint i, count;
        usb_conv_info_t *iface_conv_info = get_usb_iface_conv_info(pinfo, interface_num);

        /* update the conversation info with the selected alternate setting */
        count = wmem_array_get_count(iface_conv_info->alt_settings);
        for (i = 0; i < count; i++) {
            usb_alt_setting_t *alternate_setting = (usb_alt_setting_t *)wmem_array_index(iface_conv_info->alt_settings, i);

            if (alternate_setting->altSetting == alt_setting) {
                iface_conv_info->interfaceClass = alternate_setting->interfaceClass;
                iface_conv_info->interfaceSubclass = alternate_setting->interfaceSubclass;
                iface_conv_info->interfaceProtocol = alternate_setting->interfaceProtocol;
                break;
            }
        }
    }

    return offset;
}

static int
dissect_usb_setup_set_interface_response(packet_info *pinfo _U_, proto_tree *tree _U_,
                                         tvbuff_t *tvb _U_, int offset,
                                         usb_conv_info_t  *usb_conv_info _U_)
{
    return offset;
}


/*
 * These dissectors are used to dissect the setup part and the data
 * for URB_CONTROL_INPUT / SYNCH FRAME
 */


/* 9.4.11 */
static int
dissect_usb_setup_synch_frame_request(packet_info *pinfo _U_, proto_tree *tree,
                                      tvbuff_t *tvb, int offset,
                                      usb_conv_info_t  *usb_conv_info _U_)
{
    /* zero */
    proto_tree_add_item(tree, hf_usb_value, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    /* endpoint */
    proto_tree_add_item(tree, hf_usb_wEndpoint, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    /* two */
    proto_tree_add_item(tree, hf_usb_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    return offset;
}

static int
dissect_usb_setup_synch_frame_response(packet_info *pinfo _U_, proto_tree *tree _U_,
                                       tvbuff_t *tvb _U_, int offset,
                                       usb_conv_info_t  *usb_conv_info _U_)
{
    /* frame number */
    proto_tree_add_item(tree, hf_usb_wFrameNumber, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    return offset;
}

/* Dissector used for unknown USB setup request/responses */
static int
dissect_usb_setup_generic(packet_info *pinfo _U_, proto_tree *tree ,
                                       tvbuff_t *tvb, int offset,
                                       usb_conv_info_t  *usb_conv_info _U_)
{

    proto_tree_add_item(tree, hf_usb_value, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;
    proto_tree_add_item(tree, hf_usb_index, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;
    proto_tree_add_item(tree, hf_usb_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    return offset;
}



typedef int (*usb_setup_dissector)(packet_info *pinfo, proto_tree *tree,
                                   tvbuff_t *tvb, int offset,
                                   usb_conv_info_t  *usb_conv_info);

typedef struct _usb_setup_dissector_table_t {
    guint8 request;
    usb_setup_dissector dissector;

} usb_setup_dissector_table_t;
static const usb_setup_dissector_table_t setup_request_dissectors[] = {
    {USB_SETUP_GET_STATUS,        dissect_usb_setup_get_status_request},
    {USB_SETUP_CLEAR_FEATURE,     dissect_usb_setup_clear_feature_request},
    {USB_SETUP_SET_FEATURE,       dissect_usb_setup_set_feature_request},
    {USB_SETUP_SET_ADDRESS,       dissect_usb_setup_set_address_request},
    {USB_SETUP_GET_DESCRIPTOR,    dissect_usb_setup_get_descriptor_request},
    {USB_SETUP_SET_CONFIGURATION, dissect_usb_setup_set_configuration_request},
    {USB_SETUP_GET_INTERFACE,     dissect_usb_setup_get_interface_request},
    {USB_SETUP_SET_INTERFACE,     dissect_usb_setup_set_interface_request},
    {USB_SETUP_SYNCH_FRAME,       dissect_usb_setup_synch_frame_request},
    {0, NULL}
};

static const usb_setup_dissector_table_t setup_response_dissectors[] = {
    {USB_SETUP_GET_STATUS,        dissect_usb_setup_get_status_response},
    {USB_SETUP_CLEAR_FEATURE,     dissect_usb_setup_clear_feature_response},
    {USB_SETUP_SET_FEATURE,       dissect_usb_setup_set_feature_response},
    {USB_SETUP_SET_ADDRESS,       dissect_usb_setup_set_address_response},
    {USB_SETUP_GET_DESCRIPTOR,    dissect_usb_setup_get_descriptor_response},
    {USB_SETUP_GET_CONFIGURATION, dissect_usb_setup_get_configuration_response},
    {USB_SETUP_SET_CONFIGURATION, dissect_usb_setup_set_configuration_response},
    {USB_SETUP_GET_INTERFACE,     dissect_usb_setup_get_interface_response},
    {USB_SETUP_SET_INTERFACE,     dissect_usb_setup_set_interface_response},
    {USB_SETUP_SYNCH_FRAME,       dissect_usb_setup_synch_frame_response},
    {0, NULL}
};

static const value_string setup_request_names_vals[] = {
    {USB_SETUP_GET_STATUS,              "GET STATUS"},
    {USB_SETUP_CLEAR_FEATURE,           "CLEAR FEATURE"},
    {USB_SETUP_SET_FEATURE,             "SET FEATURE"},
    {USB_SETUP_SET_ADDRESS,             "SET ADDRESS"},
    {USB_SETUP_GET_DESCRIPTOR,          "GET DESCRIPTOR"},
    {USB_SETUP_SET_DESCRIPTOR,          "SET DESCRIPTOR"},
    {USB_SETUP_GET_CONFIGURATION,       "GET CONFIGURATION"},
    {USB_SETUP_SET_CONFIGURATION,       "SET CONFIGURATION"},
    {USB_SETUP_GET_INTERFACE,           "GET INTERFACE"},
    {USB_SETUP_SET_INTERFACE,           "SET INTERFACE"},
    {USB_SETUP_SYNCH_FRAME,             "SYNCH FRAME"},
    {USB_SETUP_SET_SEL,                 "SET SEL"},
    {USB_SETUP_SET_ISOCH_DELAY,         "SET ISOCH DELAY"},
    {0, NULL}
};
static value_string_ext setup_request_names_vals_ext = VALUE_STRING_EXT_INIT(setup_request_names_vals);


static const true_false_string tfs_bmrequesttype_direction = {
    "Device-to-host",
    "Host-to-device"
};

static const value_string bmrequesttype_type_vals[] = {
    {RQT_SETUP_TYPE_STANDARD, "Standard"},
    {RQT_SETUP_TYPE_CLASS,    "Class"},
    {RQT_SETUP_TYPE_VENDOR,   "Vendor"},
    {0, NULL}
};

static const value_string bmrequesttype_recipient_vals[] = {
    {RQT_SETUP_RECIPIENT_DEVICE,    "Device" },
    {RQT_SETUP_RECIPIENT_INTERFACE, "Interface" },
    {RQT_SETUP_RECIPIENT_ENDPOINT,  "Endpoint" },
    {RQT_SETUP_RECIPIENT_OTHER,     "Other" },
    {0, NULL }
};

/* Dissector used for standard usb setup requests */
static int
dissect_usb_standard_setup_request(packet_info *pinfo, proto_tree *tree ,
                                   tvbuff_t *tvb, usb_conv_info_t  *usb_conv_info,
                                   usb_trans_info_t *usb_trans_info)
{
    gint offset = 0;
    const usb_setup_dissector_table_t *tmp;
    usb_setup_dissector dissector;

    proto_tree_add_item(tree, hf_usb_request, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    col_add_fstr(pinfo->cinfo, COL_INFO, "%s Request",
            val_to_str_ext(usb_trans_info->setup.request, &setup_request_names_vals_ext, "Unknown type %x"));

    dissector = NULL;
    for(tmp = setup_request_dissectors;tmp->dissector;tmp++) {
        if (tmp->request == usb_trans_info->setup.request) {
            dissector = tmp->dissector;
            break;
        }
    }

    if (!dissector) {
            dissector = &dissect_usb_setup_generic;
    }

    offset = dissector(pinfo, tree, tvb, offset, usb_conv_info);

    return offset;

}

/* Dissector used for standard usb setup responses */
static int
dissect_usb_standard_setup_response(packet_info *pinfo, proto_tree *tree,
                                    tvbuff_t *tvb, int offset,
                                    usb_conv_info_t  *usb_conv_info)
{
    const usb_setup_dissector_table_t *tmp;
    usb_setup_dissector dissector;
    gint length_remaining;


    col_add_fstr(pinfo->cinfo, COL_INFO, "%s Response",
        val_to_str_ext(usb_conv_info->usb_trans_info->setup.request,
            &setup_request_names_vals_ext, "Unknown type %x"));

    dissector = NULL;
    for(tmp = setup_response_dissectors;tmp->dissector;tmp++) {
        if (tmp->request == usb_conv_info->usb_trans_info->setup.request) {
            dissector = tmp->dissector;
            break;
        }
    }

    length_remaining = tvb_reported_length_remaining(tvb, offset);

    if (length_remaining <= 0)
        return offset;

    if (dissector) {
        offset = dissector(pinfo, tree, tvb, offset, usb_conv_info);
    } else {
        proto_tree_add_item(tree, hf_usb_control_response_generic,
                            tvb, offset, length_remaining, ENC_NA);
        offset += length_remaining;
    }

    return offset;
}


static void
usb_tap_queue_packet(packet_info *pinfo, guint8 urb_type,
                     usb_conv_info_t *usb_conv_info)
{
    usb_tap_data_t *tap_data;

    tap_data                = wmem_new(wmem_packet_scope(), usb_tap_data_t);
    tap_data->urb_type      = urb_type;
    tap_data->transfer_type = (guint8)(usb_conv_info->transfer_type);
    tap_data->conv_info     = usb_conv_info;
    tap_data->trans_info    = usb_conv_info->usb_trans_info;

    tap_queue_packet(usb_tap, pinfo, tap_data);
}


static gboolean
is_usb_standard_setup_request(usb_trans_info_t *usb_trans_info)
{
    guint8 type, recip;

    type = USB_TYPE(usb_trans_info->setup.requesttype);
    recip = USB_RECIPIENT(usb_trans_info->setup.requesttype);

    if (type != RQT_SETUP_TYPE_STANDARD)
        return FALSE;

    /* the USB standards defines the GET_DESCRIPTOR request only as a
       request to a device
       if it's not aimed at a device, it's a non-standard request that
       should be handled by a class-specific dissector */
    if (usb_trans_info->setup.request == USB_SETUP_GET_DESCRIPTOR &&
            recip != RQT_SETUP_RECIPIENT_DEVICE) {
        return FALSE;
    }

    return TRUE;
}


static gint
try_dissect_next_protocol(proto_tree *tree, tvbuff_t *next_tvb, packet_info *pinfo,
        usb_conv_info_t *usb_conv_info, guint8 urb_type, proto_tree *urb_tree)
{
    int                      ret;
    wmem_tree_key_t          key[4];
    guint32                  k_frame_number;
    guint32                  k_device_address;
    guint32                  k_bus_id;
    usb_trans_info_t        *usb_trans_info;
    heur_dtbl_entry_t       *hdtbl_entry;
    heur_dissector_list_t    heur_subdissector_list = NULL;
    dissector_table_t        usb_dissector_table = NULL;
    proto_item              *sub_item;
    device_product_data_t   *device_product_data;
    device_protocol_data_t  *device_protocol_data;
    guint8                   ctrl_recip;
    /* if we select the next dissector based on a class,
       this is the (device or interface) class we're using */
    guint32                  usb_class;

    if (!usb_conv_info) {
        /*
         * Not enough information to choose the next protocol.
         * XXX - is there something we can still do here?
         */
        if (tvb_reported_length(next_tvb) > 0)
            call_data_dissector(next_tvb, pinfo, tree);

        return tvb_captured_length(next_tvb);
    }

    /* try dissect by "usb.device" */
    ret = dissector_try_uint_new(device_to_dissector,
            (guint32)(usb_conv_info->bus_id<<16 | usb_conv_info->device_address),
            next_tvb, pinfo, tree, TRUE, usb_conv_info);
    if (ret)
        return tvb_captured_length(next_tvb);

    k_frame_number = pinfo->num;
    k_device_address = usb_conv_info->device_address;
    k_bus_id = usb_conv_info->bus_id;

    key[0].length = 1;
    key[0].key    = &k_device_address;
    key[1].length = 1;
    key[1].key    = &k_bus_id;
    key[2].length = 1;
    key[2].key    = &k_frame_number;
    key[3].length = 0;
    key[3].key    = NULL;

    /* try dissect by "usb.protocol" */
    device_protocol_data = (device_protocol_data_t *)wmem_tree_lookup32_array_le(device_to_protocol_table, key);

    if (device_protocol_data &&
            device_protocol_data->bus_id == usb_conv_info->bus_id &&
            device_protocol_data->device_address == usb_conv_info->device_address) {
                ret = dissector_try_uint_new(protocol_to_dissector,
                        (guint32)device_protocol_data->protocol,
                        next_tvb, pinfo, tree, TRUE, usb_conv_info);
                if (ret)
                    return tvb_captured_length(next_tvb);
    }

    device_product_data = (device_product_data_t *)wmem_tree_lookup32_array_le(device_to_product_table, key);

    if (device_product_data && device_product_data->bus_id == usb_conv_info->bus_id &&
            device_product_data->device_address == usb_conv_info->device_address) {
                ret = dissector_try_uint_new(product_to_dissector,
                        (guint32)(device_product_data->vendor<<16 | device_product_data->product),
                        next_tvb, pinfo, tree, TRUE, usb_conv_info);
                if (ret)
                    return tvb_captured_length(next_tvb);
    }

    switch(usb_conv_info->transfer_type) {
        case URB_BULK:
            heur_subdissector_list = heur_bulk_subdissector_list;
            usb_dissector_table = usb_bulk_dissector_table;
            break;

        case URB_INTERRUPT:
            heur_subdissector_list = heur_interrupt_subdissector_list;
            usb_dissector_table = usb_interrupt_dissector_table;
            break;

        case URB_CONTROL:
            usb_trans_info = usb_conv_info->usb_trans_info;
            if (!usb_trans_info)
                break;

            /* for standard control requests and responses, there's no
               need to query dissector tables */
            if (is_usb_standard_setup_request(usb_trans_info))
                break;

            ctrl_recip = USB_RECIPIENT(usb_trans_info->setup.requesttype);

            if (ctrl_recip == RQT_SETUP_RECIPIENT_DEVICE) {
                /* a non-standard control message addressed to a device
                   this is not supported (I don't think it's used
                   anywhere) */
                break;
            }
            else if (ctrl_recip == RQT_SETUP_RECIPIENT_INTERFACE) {
                guint8 interface_num = usb_trans_info->setup.wIndex & 0xff;

                heur_subdissector_list = heur_control_subdissector_list;
                usb_dissector_table = usb_control_dissector_table;

                usb_conv_info = get_usb_iface_conv_info(pinfo, interface_num);
                usb_conv_info->usb_trans_info = usb_trans_info;
            }
            else if (ctrl_recip == RQT_SETUP_RECIPIENT_ENDPOINT) {
                address               endpoint_addr;
                gint                  endpoint;
                guint32               src_endpoint, dst_endpoint;
                conversation_t       *conversation;

                heur_subdissector_list = heur_control_subdissector_list;
                usb_dissector_table = usb_control_dissector_table;

                endpoint = usb_trans_info->setup.wIndex & 0x0f;

                if (usb_conv_info->is_request) {
                    usb_address_t *dst_addr = wmem_new0(wmem_packet_scope(), usb_address_t);
                    dst_addr->bus_id = usb_conv_info->bus_id;
                    dst_addr->device = usb_conv_info->device_address;
                    dst_addr->endpoint = dst_endpoint = GUINT32_TO_LE(endpoint);
                    set_address(&endpoint_addr, usb_address_type, USB_ADDR_LEN, (char *)dst_addr);

                    conversation = get_usb_conversation(pinfo, &pinfo->src, &endpoint_addr, pinfo->srcport, dst_endpoint);
                }
                else {
                    usb_address_t *src_addr = wmem_new0(wmem_packet_scope(), usb_address_t);
                    src_addr->bus_id = usb_conv_info->bus_id;
                    src_addr->device = usb_conv_info->device_address;
                    src_addr->endpoint = src_endpoint = GUINT32_TO_LE(endpoint);
                    set_address(&endpoint_addr, usb_address_type, USB_ADDR_LEN, (char *)src_addr);

                    conversation  = get_usb_conversation(pinfo, &endpoint_addr, &pinfo->dst, src_endpoint, pinfo->destport);
                }

                usb_conv_info = get_usb_conv_info(conversation);
                usb_conv_info->usb_trans_info = usb_trans_info;
            }
            else {
                /* the recipient is "other" or "reserved"
                   there's no way for us to determine the interfaceClass
                   we set the usb_dissector_table anyhow as some
                   dissectors register for control messages to
                   IF_CLASS_UNKNOWN (this should be fixed) */
                heur_subdissector_list = heur_control_subdissector_list;
                usb_dissector_table = usb_control_dissector_table;
            }

            usb_tap_queue_packet(pinfo, urb_type, usb_conv_info);
            sub_item = proto_tree_add_uint(urb_tree, hf_usb_bInterfaceClass, next_tvb, 0, 0, usb_conv_info->interfaceClass);
            PROTO_ITEM_SET_GENERATED(sub_item);
            break;

        default:
            break;
    }

    if (try_heuristics && heur_subdissector_list) {
        ret = dissector_try_heuristic(heur_subdissector_list,
                next_tvb, pinfo, tree, &hdtbl_entry, usb_conv_info);
        if (ret)
            return tvb_captured_length(next_tvb);
    }

    if (usb_dissector_table) {
        /* we prefer the interface class unless it says we should refer
           to the device class
           XXX - use the device class if the interface class is unknown */
        if (usb_conv_info->interfaceClass == IF_CLASS_DEVICE) {
            usb_class = (usb_conv_info->device_protocol>>16) & 0xFF;
        }
        else {
            usb_class = usb_conv_info->interfaceClass;
        }

        ret = dissector_try_uint_new(usb_dissector_table, usb_class,
                next_tvb, pinfo, tree, TRUE, usb_conv_info);
        if (ret)
            return tvb_captured_length(next_tvb);
    }

    return 0;
}


static int
dissect_usb_setup_response(packet_info *pinfo, proto_tree *tree,
                           tvbuff_t *tvb, int offset,
                           guint8 urb_type, usb_conv_info_t *usb_conv_info)
{
    proto_tree *parent;
    tvbuff_t   *next_tvb = NULL;
    gint        length_remaining;

    parent = proto_tree_get_parent_tree(tree);

    if (usb_conv_info) {
        if (usb_conv_info->usb_trans_info && is_usb_standard_setup_request(usb_conv_info->usb_trans_info)) {
            offset = dissect_usb_standard_setup_response(pinfo, parent, tvb, offset, usb_conv_info);
        }
        else {
            next_tvb = tvb_new_subset_remaining(tvb, offset);
            offset += try_dissect_next_protocol(parent, next_tvb, pinfo, usb_conv_info, urb_type, tree);

            length_remaining = tvb_reported_length_remaining(tvb, offset);
            if (length_remaining > 0) {
                proto_tree_add_item(parent, hf_usb_control_response_generic,
                        tvb, offset, length_remaining, ENC_NA);
                offset += length_remaining;
            }
        }
    }
    else {
        /* no matching request available */
        length_remaining = tvb_reported_length_remaining(tvb, offset);
        if (length_remaining > 0) {
            proto_tree_add_item(parent, hf_usb_control_response_generic, tvb,
                    offset, length_remaining, ENC_NA);
            offset += length_remaining;
        }
    }

    return offset;
}


static int
dissect_usb_bmrequesttype(proto_tree *parent_tree, tvbuff_t *tvb, int offset)
{
    proto_item *item;
    proto_tree *tree;

    item = proto_tree_add_item(parent_tree, hf_usb_bmRequestType, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    tree = proto_item_add_subtree(item, ett_usb_setup_bmrequesttype);

    proto_tree_add_item(tree, hf_usb_bmRequestType_direction, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    proto_tree_add_item(tree, hf_usb_bmRequestType_type,      tvb, offset, 1, ENC_LITTLE_ENDIAN);
    proto_tree_add_item(tree, hf_usb_bmRequestType_recipient, tvb, offset, 1, ENC_LITTLE_ENDIAN);

    return ++offset;
}


static int
dissect_linux_usb_pseudo_header_ext(tvbuff_t *tvb, int offset,
                                    packet_info *pinfo _U_,
                                    proto_tree *tree)
{
    proto_tree_add_item(tree, hf_usb_urb_interval, tvb, offset, 4, ENC_HOST_ENDIAN);
    offset += 4;
    proto_tree_add_item(tree, hf_usb_urb_start_frame, tvb, offset, 4, ENC_HOST_ENDIAN);
    offset += 4;
    proto_tree_add_item(tree, hf_usb_urb_copy_of_transfer_flags, tvb, offset, 4, ENC_HOST_ENDIAN);
    offset += 4;
    proto_tree_add_item(tree, hf_usb_iso_numdesc, tvb, offset, 4, ENC_HOST_ENDIAN);
    offset += 4;

    return offset;
}


/* Dissector used for usb setup requests */
static int
dissect_usb_setup_request(packet_info *pinfo, proto_tree *tree,
                          tvbuff_t *tvb, int offset,
                          guint8 urb_type, usb_conv_info_t *usb_conv_info,
                          usb_header_t header_type)
{
    gint              setup_offset;
    gint              req_type;
    gint              ret;
    proto_tree       *parent, *setup_tree;
    usb_trans_info_t *usb_trans_info, trans_info;
    tvbuff_t         *next_tvb, *data_tvb = NULL;

    /* we should do the NULL check in all non-static functions */
    if (usb_conv_info)
        usb_trans_info = usb_conv_info->usb_trans_info;
    else
        usb_trans_info = &trans_info;

    parent = proto_tree_get_parent_tree(tree);

    setup_tree = proto_tree_add_subtree(parent, tvb, offset, 8, ett_usb_setup_hdr, NULL, "URB setup");

    req_type = USB_TYPE(tvb_get_guint8(tvb, offset));
    usb_trans_info->setup.requesttype = tvb_get_guint8(tvb, offset);
    if (usb_conv_info) {
        usb_conv_info->setup_requesttype = tvb_get_guint8(tvb, offset);
        if (req_type != RQT_SETUP_TYPE_CLASS)
            usb_tap_queue_packet(pinfo, urb_type, usb_conv_info);
    }

    offset = dissect_usb_bmrequesttype(setup_tree, tvb, offset);

    /* as we're going through the data, we build a next_tvb that
       contains the the setup packet without the request type
       and request-specific data
       all subsequent dissection routines work on this tvb */

    setup_offset = offset;
    usb_trans_info->setup.request = tvb_get_guint8(tvb, offset);
    offset++;
    usb_trans_info->setup.wValue  = tvb_get_letohs(tvb, offset);
    offset += 2;
    usb_trans_info->setup.wIndex  = tvb_get_letohs(tvb, offset);
    offset += 2;
    usb_trans_info->setup.wLength = tvb_get_letohs(tvb, offset);
    offset += 2;

    if (header_type == USB_HEADER_LINUX_64_BYTES) {
        offset = dissect_linux_usb_pseudo_header_ext(tvb, offset, pinfo, tree);
    }


    if (tvb_captured_length_remaining(tvb, offset) > 0) {
        next_tvb = tvb_new_composite();
        tvb_composite_append(next_tvb, tvb_new_subset(tvb, setup_offset, 7, 7));

        data_tvb = tvb_new_subset_remaining(tvb, offset);
        tvb_composite_append(next_tvb, data_tvb);
        offset += tvb_captured_length(data_tvb);
        tvb_composite_finalize(next_tvb);
        next_tvb = tvb_new_child_real_data(tvb,
                (const guint8 *) tvb_memdup(pinfo->pool, next_tvb, 0, tvb_captured_length(next_tvb)),
                tvb_captured_length(next_tvb),
                tvb_captured_length(next_tvb));
        add_new_data_source(pinfo, next_tvb, "USB Control");
    } else {
        next_tvb = tvb_new_subset(tvb, setup_offset, 7, 7);
    }

    /* at this point, offset contains the number of bytes that we
       dissected */

    if (is_usb_standard_setup_request(usb_trans_info)) {
        /* there's no point in checking the return value as there's no
           fallback for standard setup requests */
        dissect_usb_standard_setup_request(pinfo, setup_tree,
                next_tvb, usb_conv_info, usb_trans_info);
    }
    else {
        /* no standard request - pass it on to class-specific dissectors */
        ret = try_dissect_next_protocol(
                parent, next_tvb, pinfo, usb_conv_info, urb_type, tree);
        if (ret <= 0) {
            /* no class-specific dissector could handle it,
               dissect it as generic setup request */
            proto_tree_add_item(setup_tree, hf_usb_request_unknown_class,
                    next_tvb, 0, 1, ENC_LITTLE_ENDIAN);
            dissect_usb_setup_generic(pinfo, setup_tree,
                    next_tvb, 1, usb_conv_info);
        } else if (data_tvb) {
            proto_tree_add_item(setup_tree, hf_usb_request_unknown_class,
                    tvb, setup_offset, 1, ENC_LITTLE_ENDIAN);
            dissect_usb_setup_generic(pinfo, setup_tree,
                    tvb, setup_offset+1, usb_conv_info);
        }
    }

    if (data_tvb)
        proto_tree_add_item(setup_tree, hf_usb_data_fragment, data_tvb, 0, -1, ENC_NA);

    return offset;
}


/* dissect the linux-specific USB pseudo header and fill the conversation struct
   return the number of dissected bytes */
static gint
dissect_linux_usb_pseudo_header(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
        usb_conv_info_t *usb_conv_info, guint64 *urb_id)
{
    guint8  transfer_type;
    guint8  endpoint_byte;
    guint8  transfer_type_and_direction;
    guint8  urb_type;
    guint8  flag[2];

    *urb_id = tvb_get_guint64(tvb, 0, ENC_HOST_ENDIAN);
    proto_tree_add_uint64(tree, hf_usb_urb_id, tvb, 0, 8, *urb_id);

    /* show the urb type of this URB as string and as a character */
    urb_type = tvb_get_guint8(tvb, 8);
    usb_conv_info->is_request = (urb_type==URB_SUBMIT);
    proto_tree_add_uint_format_value(tree, hf_usb_linux_urb_type, tvb, 8, 1,
        urb_type, "%s ('%c')", val_to_str(urb_type, usb_linux_urb_type_vals, "Unknown %d"),
        g_ascii_isprint(urb_type) ? urb_type : '.');
    proto_tree_add_item(tree, hf_usb_linux_transfer_type, tvb, 9, 1, ENC_LITTLE_ENDIAN);

    transfer_type = tvb_get_guint8(tvb, 9);
    usb_conv_info->transfer_type = transfer_type;

    endpoint_byte = tvb_get_guint8(tvb, 10);   /* direction bit | endpoint */
    usb_conv_info->endpoint = endpoint_byte & 0x7F;
    if (endpoint_byte & URB_TRANSFER_IN)
        usb_conv_info->direction = P2P_DIR_RECV;
    else
        usb_conv_info->direction = P2P_DIR_SENT;

    transfer_type_and_direction = (transfer_type & 0x7F) | (endpoint_byte & 0x80);
    col_append_str(pinfo->cinfo, COL_INFO,
                    val_to_str(transfer_type_and_direction, usb_transfer_type_and_direction_vals, "Unknown type %x"));

    proto_tree_add_bitmask(tree, tvb, 10, hf_usb_endpoint_number, ett_usb_endpoint, usb_endpoint_fields, ENC_NA);
    proto_tree_add_item(tree, hf_usb_device_address, tvb, 11, 1, ENC_LITTLE_ENDIAN);
    usb_conv_info->device_address = (guint16)tvb_get_guint8(tvb, 11);

    proto_tree_add_item(tree, hf_usb_bus_id, tvb, 12, 2, ENC_HOST_ENDIAN);
    tvb_memcpy(tvb, &usb_conv_info->bus_id, 12, 2);

    /* Right after the pseudo header we always have
     * sizeof(struct usb_device_setup_hdr) bytes. The content of these
     * bytes only have meaning in case setup_flag == 0.
     */
    flag[0] = tvb_get_guint8(tvb, 14);
    flag[1] = '\0';
    if (flag[0] == 0) {
        usb_conv_info->is_setup = TRUE;
        proto_tree_add_string(tree, hf_usb_setup_flag, tvb, 14, 1, "relevant (0)");
        if (usb_conv_info->transfer_type!=URB_CONTROL)
            proto_tree_add_expert(tree, pinfo, &ei_usb_invalid_setup, tvb, 14, 1);
    } else {
        usb_conv_info->is_setup = FALSE;
        proto_tree_add_string_format_value(tree, hf_usb_setup_flag, tvb,
            14, 1, flag, "not relevant ('%c')", g_ascii_isprint(flag[0]) ? flag[0]: '.');
    }

    flag[0] = tvb_get_guint8(tvb, 15);
    flag[1] = '\0';
    if (flag[0] == 0) {
        proto_tree_add_string(tree, hf_usb_data_flag, tvb, 15, 1, "present (0)");
    } else {
        proto_tree_add_string_format_value(tree, hf_usb_data_flag, tvb,
            15, 1, flag, "not present ('%c')", g_ascii_isprint(flag[0]) ? flag[0] : '.');
    }

    proto_tree_add_item(tree, hf_usb_urb_ts_sec, tvb, 16, 8, ENC_HOST_ENDIAN);
    proto_tree_add_item(tree, hf_usb_urb_ts_usec, tvb, 24, 4, ENC_HOST_ENDIAN);
    proto_tree_add_item(tree, hf_usb_urb_status, tvb, 28, 4, ENC_HOST_ENDIAN);
    proto_tree_add_item(tree, hf_usb_urb_len, tvb, 32, 4, ENC_HOST_ENDIAN);
    proto_tree_add_item(tree, hf_usb_urb_data_len, tvb, 36, 4, ENC_HOST_ENDIAN);

    return 40;
}

/* dissect the usbpcap_buffer_packet_header and fill the conversation struct
   this function does not handle the transfer-specific headers
   return the number of bytes processed */
static gint
dissect_usbpcap_buffer_packet_header(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
        usb_conv_info_t *usb_conv_info, guint32 *win32_data_len, guint64 *irp_id)
{
    guint8   transfer_type;
    guint8   endpoint_byte;
    guint8   transfer_type_and_direction;
    guint8   tmp_val8;

    proto_tree_add_item(tree, hf_usb_win32_header_len, tvb, 0, 2, ENC_LITTLE_ENDIAN);
    *irp_id = tvb_get_guint64(tvb, 2, ENC_LITTLE_ENDIAN);
    proto_tree_add_uint64(tree, hf_usb_irp_id, tvb, 2, 8, *irp_id);
    proto_tree_add_item(tree, hf_usb_usbd_status, tvb, 10, 4, ENC_LITTLE_ENDIAN);
    proto_tree_add_item(tree, hf_usb_function, tvb, 14, 2, ENC_LITTLE_ENDIAN);

    proto_tree_add_bitmask(tree, tvb, 16, hf_usb_info, ett_usb_usbpcap_info, usb_usbpcap_info_fields, ENC_LITTLE_ENDIAN);
    tmp_val8 = tvb_get_guint8(tvb, 16);
    /* TODO: Handle errors */
    if (tmp_val8 & 0x01) {
        usb_conv_info->is_request = FALSE;
    } else {
        usb_conv_info->is_request = TRUE;
    }

    proto_tree_add_item(tree, hf_usb_bus_id, tvb, 17, 2, ENC_LITTLE_ENDIAN);
    usb_conv_info->bus_id = tvb_get_letohs(tvb, 17);

    proto_tree_add_item(tree, hf_usb_win32_device_address, tvb, 19, 2, ENC_LITTLE_ENDIAN);
    usb_conv_info->device_address = tvb_get_letohs(tvb, 19);

    endpoint_byte = tvb_get_guint8(tvb, 21);
    usb_conv_info->direction = endpoint_byte&URB_TRANSFER_IN ?  P2P_DIR_RECV : P2P_DIR_SENT;
    usb_conv_info->endpoint = endpoint_byte&0x7F;
    proto_tree_add_bitmask(tree, tvb, 21, hf_usb_endpoint_number, ett_usb_endpoint, usb_endpoint_fields, ENC_LITTLE_ENDIAN);

    transfer_type = tvb_get_guint8(tvb, 22);
    usb_conv_info->transfer_type = transfer_type;
    proto_tree_add_item(tree, hf_usb_linux_transfer_type, tvb, 22, 1, ENC_LITTLE_ENDIAN);

    transfer_type_and_direction = (transfer_type & 0x7F) | (endpoint_byte & 0x80);
    col_append_str(pinfo->cinfo, COL_INFO,
                   val_to_str(transfer_type_and_direction, usb_transfer_type_and_direction_vals, "Unknown type %x"));

    *win32_data_len = tvb_get_letohl(tvb, 23);
    proto_tree_add_item(tree, hf_usb_win32_data_len, tvb, 23, 4, ENC_LITTLE_ENDIAN);

    /* by default, we assume it's no setup packet
       the correct values will be set when we parse the control header */
    usb_conv_info->is_setup = FALSE;
    usb_conv_info->setup_requesttype = 0;

    /* we don't handle the transfer-specific headers here */
    return 27;
}

/* Set the usb_address_t fields based on the direction of the urb */
static void
usb_set_addr(proto_tree *tree, tvbuff_t *tvb, packet_info *pinfo, guint16 bus_id, guint16 device_address,
             int endpoint, gboolean req)
{
    proto_item     *sub_item;
    usb_address_t  *src_addr = wmem_new0(pinfo->pool, usb_address_t),
                   *dst_addr = wmem_new0(pinfo->pool, usb_address_t);
    guint8         *str_src_addr;
    guint8         *str_dst_addr;

    if (req) {
        /* request */
        src_addr->device   = 0xffffffff;
        src_addr->endpoint = NO_ENDPOINT;
        dst_addr->device   = GUINT16_TO_LE(device_address);
        dst_addr->endpoint = GUINT32_TO_LE(endpoint);
    } else {
        /* response */
        src_addr->device   = GUINT16_TO_LE(device_address);
        src_addr->endpoint = GUINT32_TO_LE(endpoint);
        dst_addr->device   = 0xffffffff;
        dst_addr->endpoint = NO_ENDPOINT;
    }
    src_addr->bus_id = GUINT16_TO_LE(bus_id);
    dst_addr->bus_id = GUINT16_TO_LE(bus_id);

    set_address(&pinfo->net_src, usb_address_type, USB_ADDR_LEN, (char *)src_addr);
    copy_address_shallow(&pinfo->src, &pinfo->net_src);
    set_address(&pinfo->net_dst, usb_address_type, USB_ADDR_LEN, (char *)dst_addr);
    copy_address_shallow(&pinfo->dst, &pinfo->net_dst);

    pinfo->ptype = PT_USB;
    pinfo->srcport = src_addr->endpoint;
    pinfo->destport = dst_addr->endpoint;
    /* sent/received is from the perspective of the USB host */
    pinfo->p2p_dir = req ? P2P_DIR_SENT : P2P_DIR_RECV;

    str_src_addr = address_to_str(wmem_packet_scope(), &pinfo->src);
    str_dst_addr = address_to_str(wmem_packet_scope(), &pinfo->dst);

    sub_item = proto_tree_add_string(tree, hf_usb_src, tvb, 0, 0, str_src_addr);
    PROTO_ITEM_SET_GENERATED(sub_item);

    sub_item = proto_tree_add_string(tree, hf_usb_addr, tvb, 0, 0, str_src_addr);
    PROTO_ITEM_SET_HIDDEN(sub_item);

    sub_item = proto_tree_add_string(tree, hf_usb_dst, tvb, 0, 0, str_dst_addr);
    PROTO_ITEM_SET_GENERATED(sub_item);

    sub_item = proto_tree_add_string(tree, hf_usb_addr, tvb, 0, 0, str_dst_addr);
    PROTO_ITEM_SET_HIDDEN(sub_item);
}


/* Gets the transfer info for a given packet
 * Generates transfer info if none exists yet
 * Also adds request/response info to the tree for the given packet */
static usb_trans_info_t
*usb_get_trans_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                    usb_header_t header_type, usb_conv_info_t *usb_conv_info, guint64 usb_id)
{
    usb_trans_info_t *usb_trans_info;
    proto_item       *ti;
    nstime_t          t, deltat;
    wmem_tree_key_t   key[3];

    /* request/response matching so we can keep track of transaction specific
     * data.
     */
    key[0].length = 2;
    key[0].key = (guint32 *)&usb_id;
    key[1].length = 1;
    key[1].key = &pinfo->num;
    key[2].length = 0;
    key[2].key = NULL;

    if (usb_conv_info->is_request) {
        /* this is a request */
        usb_trans_info = (usb_trans_info_t *)wmem_tree_lookup32_array(usb_conv_info->transactions, key);
        if (!usb_trans_info) {
            usb_trans_info              = wmem_new0(wmem_file_scope(), usb_trans_info_t);
            usb_trans_info->request_in  = pinfo->num;
            usb_trans_info->req_time    = pinfo->abs_ts;
            usb_trans_info->header_type = header_type;
            usb_trans_info->usb_id      = usb_id;

            wmem_tree_insert32_array(usb_conv_info->transactions, key, usb_trans_info);
        }

        if (usb_trans_info->response_in) {
            ti = proto_tree_add_uint(tree, hf_usb_response_in, tvb, 0, 0, usb_trans_info->response_in);
            PROTO_ITEM_SET_GENERATED(ti);
        }

    } else {
        /* this is a response */
        if (pinfo->fd->flags.visited) {
            usb_trans_info = (usb_trans_info_t *)wmem_tree_lookup32_array(usb_conv_info->transactions, key);

        } else {
            usb_trans_info = (usb_trans_info_t *)wmem_tree_lookup32_array_le(usb_conv_info->transactions, key);
            if (usb_trans_info) {
                if (usb_trans_info->usb_id == usb_id) {
                    if (usb_trans_info->response_in == 0) {
                        /* USBPcap generates 2 frames for response; store the first one */
                        usb_trans_info->response_in = pinfo->num;
                    }
                    wmem_tree_insert32_array(usb_conv_info->transactions, key, usb_trans_info);
                } else {
                    usb_trans_info = NULL;
                }
            }
        }

        if (usb_trans_info && usb_trans_info->request_in) {

            ti = proto_tree_add_uint(tree, hf_usb_request_in, tvb, 0, 0, usb_trans_info->request_in);
            PROTO_ITEM_SET_GENERATED(ti);

            t = pinfo->abs_ts;
            nstime_delta(&deltat, &t, &usb_trans_info->req_time);
            ti = proto_tree_add_time(tree, hf_usb_time, tvb, 0, 0, &deltat);
            PROTO_ITEM_SET_GENERATED(ti);
        }
    }

    return usb_trans_info;
}


/* dissect a group of isochronous packets inside an usb packet in
   usbpcap format */
static gint
dissect_usbpcap_iso_packets(packet_info *pinfo _U_, proto_tree *urb_tree, guint8 urb_type,
        tvbuff_t *tvb, gint offset, guint32 win32_data_len, usb_conv_info_t *usb_conv_info)
{
    guint32     i;
    guint32     num_packets;
    guint32     data_start_offset;
    proto_item *urb_tree_ti;

    proto_tree_add_item(urb_tree, hf_usb_win32_iso_start_frame, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;

    num_packets = tvb_get_letohl(tvb, offset);
    proto_tree_add_item(urb_tree, hf_usb_win32_iso_num_packets, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;

    proto_tree_add_item(urb_tree, hf_usb_win32_iso_error_count, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;

    data_start_offset = offset + 12 * num_packets;
    urb_tree_ti = proto_tree_get_parent(urb_tree);
    proto_item_set_len(urb_tree_ti, data_start_offset);

    for (i = 0; i < num_packets; i++) {
        guint32 this_offset;
        guint32 next_offset;
        guint32 iso_len;
        proto_item *iso_packet_ti, *ti;
        proto_tree *iso_packet_tree;

        iso_packet_ti = proto_tree_add_protocol_format(
                proto_tree_get_root(urb_tree), proto_usb,
                tvb, offset, 12, "USB isochronous packet");
        iso_packet_tree = proto_item_add_subtree(iso_packet_ti, ett_usb_win32_iso_packet);

        this_offset = tvb_get_letohl(tvb, offset);
        if (num_packets - i == 1) {
            /* this is the last packet */
            next_offset = win32_data_len;
        } else {
            /* there is next packet */
            next_offset = tvb_get_letohl(tvb, offset + 12);
        }

        if (next_offset > this_offset) {
            iso_len = next_offset - this_offset;
        } else {
            iso_len = 0;
        }

        /* If this packet does not contain isochrounous data, do not try to display it */
        if (!((usb_conv_info->is_request && usb_conv_info->direction==P2P_DIR_SENT) ||
                    (!usb_conv_info->is_request && usb_conv_info->direction==P2P_DIR_RECV))) {
            iso_len = 0;
        }

        proto_tree_add_item(iso_packet_tree, hf_usb_win32_iso_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;

        ti = proto_tree_add_item(iso_packet_tree, hf_usb_win32_iso_length, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        if (usb_conv_info->direction==P2P_DIR_SENT) {
            /* Isochronous OUT transfer */
            proto_item_append_text(ti, " (not used)");
        } else {
            /* Isochronous IN transfer.
             * Length field is being set by host controller.
             */
            if (usb_conv_info->is_request) {
                /* Length was not yet set */
                proto_item_append_text(ti, " (irrelevant)");
            } else {
                /* Length was set and (should be) valid */
                proto_item_append_text(ti, " (relevant)");
                iso_len = tvb_get_letohl(tvb, offset);
            }
        }
        offset += 4;

        ti = proto_tree_add_item(iso_packet_tree, hf_usb_win32_iso_status, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        if (urb_type == URB_SUBMIT) {
            proto_item_append_text(ti, " (irrelevant)");
        } else {
            proto_item_append_text(ti, " (relevant)");
        }
        offset += 4;

        if (iso_len && data_start_offset + this_offset + iso_len <= tvb_captured_length(tvb)) {
            proto_tree_add_item(iso_packet_tree, hf_usb_iso_data, tvb, (gint)(data_start_offset + this_offset), (gint)iso_len, ENC_NA);
            proto_tree_set_appendix(iso_packet_tree, tvb, (gint)(data_start_offset + this_offset), (gint)iso_len);
        }
    }

    if ((usb_conv_info->is_request && usb_conv_info->direction==P2P_DIR_SENT) ||
            (!usb_conv_info->is_request && usb_conv_info->direction==P2P_DIR_RECV)) {
        /* We have dissected all the isochronous data */
        offset += win32_data_len;
    }

    return offset;
}


static gint
dissect_linux_usb_iso_transfer(packet_info *pinfo _U_, proto_tree *urb_tree,
        usb_header_t header_type, tvbuff_t *tvb, gint offset,
        usb_conv_info_t *usb_conv_info)
{
    guint32     iso_numdesc = 0;
    proto_item *tii;
    guint32     val32;
    guint32     i;
    guint       data_base;
    guint32     iso_status;
    guint32     iso_off = 0;
    guint32     iso_len = 0;

    tii = proto_tree_add_uint(urb_tree, hf_usb_bInterfaceClass, tvb, offset, 0, usb_conv_info->interfaceClass);
    PROTO_ITEM_SET_GENERATED(tii);

    /* All fields which belong to Linux usbmon headers are in host-endian
     * byte order. The fields coming from the USB communication are in little
     * endian format (see usb_20.pdf, chapter 8.1 Byte/Bit ordering).
     *
     * When a capture file is transferred to a host with different endianness
     * than packet was captured then the necessary swapping happens in
     * wiretap/pcap-common.c, pcap_byteswap_linux_usb_pseudoheader().
     */

    /* iso urbs on linux can't possibly contain a setup packet
       see mon_bin_event() in the linux kernel */

    /* Process ISO related fields (usbmon_packet.iso). The fields are
     * in host endian byte order so use tvb_memcopy() and
     * proto_tree_add_uint() pair.
     */

    tvb_memcpy(tvb, (guint8 *)&val32, offset, 4);
    proto_tree_add_uint(urb_tree, hf_usb_iso_error_count, tvb, offset, 4, val32);
    offset += 4;

    tvb_memcpy(tvb, (guint8 *)&iso_numdesc, offset, 4);
    proto_tree_add_uint(urb_tree, hf_usb_iso_numdesc, tvb, offset, 4, iso_numdesc);
    offset += 4;

    if (header_type == USB_HEADER_LINUX_64_BYTES) {
        offset = dissect_linux_usb_pseudo_header_ext(tvb, offset, pinfo, urb_tree);
    }

    data_base = offset + iso_numdesc*16;
    for (i = 0; i<iso_numdesc; i++) {
        proto_item   *iso_desc_ti;
        proto_tree   *iso_desc_tree;
        guint32       iso_pad;

        /* Fetch ISO descriptor fields stored in host endian byte order. */
        tvb_memcpy(tvb, (guint8 *)&iso_status, offset, 4);
        tvb_memcpy(tvb, (guint8 *)&iso_off, offset+4,  4);
        tvb_memcpy(tvb, (guint8 *)&iso_len, offset+8,  4);

        iso_desc_ti = proto_tree_add_protocol_format(urb_tree, proto_usb, tvb, offset,
                16, "USB isodesc %u [%s]", i, val_to_str_ext(iso_status, &usb_urb_status_vals_ext, "Error %d"));
        if (iso_len > 0)
            proto_item_append_text(iso_desc_ti, " (%u bytes)", iso_len);
        iso_desc_tree = proto_item_add_subtree(iso_desc_ti, ett_usb_isodesc);

        proto_tree_add_int(iso_desc_tree, hf_usb_iso_status, tvb, offset, 4, iso_status);
        offset += 4;

        proto_tree_add_uint(iso_desc_tree, hf_usb_iso_off, tvb, offset, 4, iso_off);
        offset += 4;

        proto_tree_add_uint(iso_desc_tree, hf_usb_iso_len, tvb, offset, 4, iso_len);
        offset += 4;

        /* Show the ISO data if we captured them and either the status
           is OK or the packet is sent from host to device.
           The Linux kernel sets the status field in outgoing isochronous
           URBs to -EXDEV and fills the data part with valid data.
         */
        if ((pinfo->p2p_dir==P2P_DIR_SENT || !iso_status) &&
                iso_len && data_base + iso_off + iso_len <= tvb_captured_length(tvb)) {
            proto_tree_add_item(iso_desc_tree, hf_usb_iso_data, tvb, data_base + iso_off, iso_len, ENC_NA);
            proto_tree_set_appendix(iso_desc_tree, tvb, (gint)(data_base+iso_off), (gint)iso_len);
        }

        tvb_memcpy(tvb, (guint8 *)&iso_pad, offset, 4);
        proto_tree_add_uint(iso_desc_tree, hf_usb_iso_pad, tvb, offset, 4, iso_pad);
        offset += 4;
    }

    /* we jump to the end of the last iso data chunk
       this assumes that the iso data starts immediately after the
       iso descriptors
       we have to use the offsets from the last iso descriptor, we can't keep
       track of the offset ourselves as there may be gaps
       between data packets in the transfer buffer */
    return data_base+iso_off+iso_len;
}

static gint
dissect_usbip_iso_transfer(packet_info *pinfo _U_, proto_tree *urb_tree,
        tvbuff_t *tvb, gint offset, guint32 iso_numdesc, guint32 desc_offset,
        usb_conv_info_t *usb_conv_info)
{
    proto_item *tii;
    guint32     i;
    guint       data_base;
    guint32     iso_off = 0;
    guint32     iso_len = 0;

    tii = proto_tree_add_uint(urb_tree, hf_usb_bInterfaceClass, tvb, offset, 0, usb_conv_info->interfaceClass);
    PROTO_ITEM_SET_GENERATED(tii);

    /* All fields which belong to usbip are in big-endian byte order.
     * unlike the linux kernel, the usb isoc descriptor is appended at
     * the end of the isoc data. We have to reassemble the pdus and jump
     * to the end (actual_length) and the remaining data is the isoc
     * descriptor.
     */

    data_base = offset;
    for (i = 0; i<iso_numdesc; i++) {
        proto_item   *iso_desc_ti;
        proto_tree   *iso_desc_tree;
        guint32       iso_status;

        iso_status = tvb_get_ntohl(tvb, desc_offset + 12);
        iso_desc_ti = proto_tree_add_protocol_format(urb_tree, proto_usb, tvb, desc_offset,
                16, "USB isodesc %u [%s]", i, val_to_str_ext(iso_status, &usb_urb_status_vals_ext, "Error %d"));
        iso_desc_tree = proto_item_add_subtree(iso_desc_ti, ett_usb_isodesc);

        proto_tree_add_item_ret_uint(iso_desc_tree, hf_usb_iso_off, tvb, desc_offset, 4, ENC_BIG_ENDIAN, &iso_off);
        desc_offset += 4;

        proto_tree_add_item(iso_desc_tree, hf_usb_iso_len, tvb, desc_offset, 4, ENC_BIG_ENDIAN);
        desc_offset += 4;

        proto_tree_add_item_ret_uint(iso_desc_tree, hf_usb_iso_actual_len, tvb, desc_offset, 4, ENC_BIG_ENDIAN, &iso_len);
        desc_offset += 4;
        if (iso_len > 0)
            proto_item_append_text(iso_desc_ti, " (%u bytes)", iso_len);

        proto_tree_add_uint(iso_desc_tree, hf_usb_iso_status, tvb, desc_offset, 4, iso_status);
        desc_offset += 4;

        /* Show the ISO data if we captured them and either the status
           is OK or the packet is sent from host to device.
           The Linux kernel sets the status field in outgoing isochronous
           URBs to -EXDEV and fills the data part with valid data.
         */
        if ((pinfo->p2p_dir==P2P_DIR_SENT || !iso_status) &&
                iso_len && data_base + iso_off + iso_len <= tvb_reported_length(tvb)) {
            proto_tree_add_item(iso_desc_tree, hf_usb_iso_data, tvb, (guint) data_base + iso_off, iso_len, ENC_NA);
            proto_tree_set_appendix(iso_desc_tree, tvb, (guint) data_base + iso_off, (gint)iso_len);
        }
    }
    return desc_offset;
}

static gint
dissect_usb_payload(tvbuff_t *tvb, packet_info *pinfo,
                    proto_tree *parent, proto_tree *tree,
                    usb_conv_info_t *usb_conv_info, guint8 urb_type,
                    gint offset, guint16 device_address)
{
    wmem_tree_key_t          key[4];
    guint32                  k_frame_number;
    guint32                  k_device_address;
    guint32                  k_bus_id;
    device_product_data_t   *device_product_data = NULL;
    device_protocol_data_t  *device_protocol_data = NULL;
    tvbuff_t                *next_tvb = NULL;

    k_frame_number = pinfo->num;
    k_device_address = device_address;
    k_bus_id = usb_conv_info->bus_id;

    key[0].length = 1;
    key[0].key    = &k_device_address;
    key[1].length = 1;
    key[1].key    = &k_bus_id;
    key[2].length = 1;
    key[2].key    = &k_frame_number;
    key[3].length = 0;
    key[3].key    = NULL;

    device_product_data = (device_product_data_t *) wmem_tree_lookup32_array_le(device_to_product_table, key);
    if (device_product_data && device_product_data->bus_id == usb_conv_info->bus_id &&
            device_product_data->device_address == device_address) {
        p_add_proto_data(pinfo->pool, pinfo, proto_usb, USB_VENDOR_ID, GUINT_TO_POINTER((guint)device_product_data->vendor));
        p_add_proto_data(pinfo->pool, pinfo, proto_usb, USB_PRODUCT_ID, GUINT_TO_POINTER((guint)device_product_data->product));
    }

    device_protocol_data = (device_protocol_data_t *) wmem_tree_lookup32_array_le(device_to_protocol_table, key);
    if (device_protocol_data && device_protocol_data->bus_id == usb_conv_info->bus_id &&
            device_protocol_data->device_address == device_address) {
        p_add_proto_data(pinfo->pool, pinfo, proto_usb, USB_DEVICE_CLASS, GUINT_TO_POINTER(device_protocol_data->protocol >> 16));
        p_add_proto_data(pinfo->pool, pinfo, proto_usb, USB_DEVICE_SUBCLASS, GUINT_TO_POINTER((device_protocol_data->protocol >> 8) & 0xFF));
        p_add_proto_data(pinfo->pool, pinfo, proto_usb, USB_DEVICE_PROTOCOL, GUINT_TO_POINTER(device_protocol_data->protocol & 0xFF));
        usb_conv_info->device_protocol = device_protocol_data->protocol;
    }

    p_add_proto_data(pinfo->pool, pinfo, proto_usb, USB_BUS_ID, GUINT_TO_POINTER((guint)usb_conv_info->bus_id));
    p_add_proto_data(pinfo->pool, pinfo, proto_usb, USB_DEVICE_ADDRESS, GUINT_TO_POINTER((guint)device_address));

    if (tvb_captured_length_remaining(tvb, offset) > 0) {
        next_tvb = tvb_new_subset_remaining(tvb, offset);
        offset += try_dissect_next_protocol(parent, next_tvb, pinfo, usb_conv_info, urb_type, tree);
    }

    if (tvb_captured_length_remaining(tvb, offset) > 0) {
        /* There is still leftover capture data to add (padding?) */
        proto_tree_add_item(parent, hf_usb_capdata, tvb, offset, -1, ENC_NA);
    }

    return offset;
}

static int
dissect_freebsd_usb(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent, void *data _U_)
{
    int offset = 0;
    proto_item *ti;
    proto_tree *tree = NULL, *frame_tree = NULL;
    guint32 nframes;
    guint32 i;

    col_set_str(pinfo->cinfo, COL_PROTOCOL, "USB");

    /* add usb hdr*/
    if (parent) {
      ti = proto_tree_add_protocol_format(parent, proto_usb, tvb, 0, 128,
                                          "USB URB");
      tree = proto_item_add_subtree(ti, ett_usb_hdr);
    }

    proto_tree_add_item(tree, hf_usb_totlen, tvb, 0, 4, ENC_LITTLE_ENDIAN);
    proto_tree_add_item(tree, hf_usb_busunit, tvb, 4, 4, ENC_LITTLE_ENDIAN);
    proto_tree_add_item(tree, hf_usb_address, tvb, 8, 1, ENC_LITTLE_ENDIAN);
    proto_tree_add_item(tree, hf_usb_mode, tvb, 9, 1, ENC_LITTLE_ENDIAN);
    proto_tree_add_item(tree, hf_usb_freebsd_urb_type, tvb, 10, 1, ENC_LITTLE_ENDIAN);
    proto_tree_add_item(tree, hf_usb_freebsd_transfer_type, tvb, 11, 1, ENC_LITTLE_ENDIAN);
    proto_tree_add_bitmask(tree, tvb, 12, hf_usb_xferflags, ett_usb_xferflags,
                           usb_xferflags_fields, ENC_LITTLE_ENDIAN);
    proto_tree_add_bitmask(tree, tvb, 16, hf_usb_xferstatus, ett_usb_xferstatus,
                           usb_xferstatus_fields, ENC_LITTLE_ENDIAN);
    proto_tree_add_item(tree, hf_usb_error, tvb, 20, 4, ENC_LITTLE_ENDIAN);
    proto_tree_add_item(tree, hf_usb_interval, tvb, 24, 4, ENC_LITTLE_ENDIAN);
    proto_tree_add_item_ret_uint(tree, hf_usb_nframes, tvb, 28, 4, ENC_LITTLE_ENDIAN, &nframes);
    proto_tree_add_item(tree, hf_usb_packet_size, tvb, 32, 4, ENC_LITTLE_ENDIAN);
    proto_tree_add_item(tree, hf_usb_packet_count, tvb, 36, 4, ENC_LITTLE_ENDIAN);
    proto_tree_add_item(tree, hf_usb_endpoint_number, tvb, 40, 4, ENC_LITTLE_ENDIAN);
    proto_tree_add_item(tree, hf_usb_speed, tvb, 44, 1, ENC_LITTLE_ENDIAN);

    offset += 128;
    for (i = 0; i < nframes; i++) {
        guint32 framelen;
        guint64 frameflags;

        frame_tree = proto_tree_add_subtree_format(tree, tvb, offset, -1,
                                                   ett_usb_frame, &ti,
                                                   "Frame %u", i);
        proto_tree_add_item_ret_uint(frame_tree, hf_usb_frame_length,
                                     tvb, offset, 4, ENC_LITTLE_ENDIAN,
                                     &framelen);
        offset += 4;
        proto_tree_add_bitmask_ret_uint64(frame_tree, tvb, offset,
                                          hf_usb_frame_flags,
                                          ett_usb_frame_flags,
                                          usb_frame_flags_fields,
                                          ENC_LITTLE_ENDIAN, &frameflags);
        offset += 4;
        if (frameflags & FREEBSD_FRAMEFLAG_DATA_FOLLOWS) {
            /*
             * XXX - ultimately, we should dissect this data.
             */
            proto_tree_add_item(frame_tree, hf_usb_frame_data, tvb, offset,
                                framelen, ENC_NA);
            offset += (framelen + 3) & ~3;
        }
        proto_item_set_end(ti, tvb, offset);
    }

    return tvb_captured_length(tvb);
}

void
dissect_usb_common(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent,
                   usb_header_t header_type, void *extra_data)
{
    gint                  offset = 0;
    int                   endpoint;
    guint8                urb_type;
    guint32               win32_data_len = 0;
    guint32               iso_numdesc = 0;
    guint32               desc_offset = 0;
    proto_item           *urb_tree_ti;
    proto_tree           *tree;
    proto_item           *item;
    usb_conv_info_t      *usb_conv_info;
    conversation_t       *conversation;
    guint16              device_address;
    guint16              bus_id;
    guint8                   usbpcap_control_stage = 0;
    guint64                  usb_id;
    struct mausb_header  *ma_header = NULL;
    struct usbip_header  *ip_header = NULL;

    /* the goal is to get the conversation struct as early as possible
       and store all status values in this struct
       at first, we read the fields required to create/identify
       the right conversation struct */
    switch (header_type) {

    case USB_HEADER_LINUX_48_BYTES:
    case USB_HEADER_LINUX_64_BYTES:
        urb_type = tvb_get_guint8(tvb, 8);
        endpoint = tvb_get_guint8(tvb, 10) & 0x7F;
        device_address = (guint16)tvb_get_guint8(tvb, 11);
        bus_id = tvb_get_letohs(tvb, 12);
        break;

    case USB_HEADER_USBPCAP:
        urb_type = tvb_get_guint8(tvb, 16) & 0x01 ? URB_COMPLETE : URB_SUBMIT;
        device_address = tvb_get_letohs(tvb, 19);
        endpoint = tvb_get_guint8(tvb, 21) & 0x7F;
        bus_id = tvb_get_letohs(tvb, 17);
        break;

    case USB_HEADER_MAUSB:
        ma_header = (struct mausb_header *) extra_data;
        urb_type = mausb_is_from_host(ma_header) ? URB_SUBMIT : URB_COMPLETE;
        device_address = mausb_ep_handle_dev_addr(ma_header->handle);
        endpoint = mausb_ep_handle_ep_num(ma_header->handle);
        bus_id = mausb_ep_handle_bus_num(ma_header->handle);
        break;

    case USB_HEADER_USBIP:
        ip_header = (struct usbip_header *) extra_data;
        urb_type = tvb_get_ntohl(tvb, 0) == 1 ? URB_SUBMIT : URB_COMPLETE;
        device_address = ip_header->devid;
        bus_id = ip_header->busid;
        endpoint = ip_header->ep;
        break;

    default:
        return; /* invalid USB pseudo header */
    }

    col_set_str(pinfo->cinfo, COL_PROTOCOL, "USB");
    urb_tree_ti = proto_tree_add_protocol_format(parent, proto_usb, tvb, 0, -1, "USB URB");
    tree = proto_item_add_subtree(urb_tree_ti, ett_usb_hdr);

    usb_set_addr(tree, tvb, pinfo, bus_id, device_address, endpoint,
                 (urb_type == URB_SUBMIT));

    conversation = get_usb_conversation(pinfo, &pinfo->src, &pinfo->dst, pinfo->srcport, pinfo->destport);
    usb_conv_info = get_usb_conv_info(conversation);
    clear_usb_conv_tmp_data(usb_conv_info);


    switch (header_type) {

    case USB_HEADER_LINUX_48_BYTES:
    case USB_HEADER_LINUX_64_BYTES:
        proto_item_set_len(urb_tree_ti, (header_type == USB_HEADER_LINUX_64_BYTES) ? 64 : 48);
        offset = dissect_linux_usb_pseudo_header(tvb, pinfo, tree, usb_conv_info, &usb_id);
        break;

    case USB_HEADER_USBPCAP:
        offset = dissect_usbpcap_buffer_packet_header(tvb, pinfo, tree, usb_conv_info, &win32_data_len, &usb_id);
        /* the length that we're setting here might have to be corrected
           if there's a transfer-specific pseudo-header following */
        proto_item_set_len(urb_tree_ti, offset);
        break;

    case USB_HEADER_MAUSB:
        /* MA USB header gets dissected earlier, just set conversation variables */
        offset = MAUSB_DPH_LENGTH;
        mausb_set_usb_conv_info(usb_conv_info, ma_header);
        usb_id = 0;
        break;

    case USB_HEADER_USBIP:
        iso_numdesc = tvb_get_ntohl(tvb, 0x20);
        usb_conv_info->transfer_type = endpoint == 0 ? URB_CONTROL : (iso_numdesc > 0 ? URB_ISOCHRONOUS : URB_UNKNOWN);
        usb_conv_info->direction = ip_header->dir == USBIP_DIR_OUT ? P2P_DIR_SENT : P2P_DIR_RECV;
        usb_conv_info->is_setup = endpoint == 0 ? (tvb_get_ntoh64(tvb, 0x28) != G_GUINT64_CONSTANT(0)) : FALSE;
        usb_conv_info->is_request = (urb_type==URB_SUBMIT);
        offset = usb_conv_info->is_setup ? USBIP_HEADER_WITH_SETUP_LEN : USBIP_HEADER_LEN;

        /* The ISOC descriptor is located at the end of the isoc frame behind the isoc data. */
        if ((usb_conv_info->is_request && usb_conv_info->direction == USBIP_DIR_OUT) ||
            (!usb_conv_info->is_request && usb_conv_info->direction == USBIP_DIR_IN)) {
            desc_offset += tvb_get_ntohl(tvb, 0x18);
        }

        desc_offset += offset;
        usb_id = 0;
        break;

    default:
        usb_id = 0;
        break;
    }

    usb_conv_info->usb_trans_info = usb_get_trans_info(tvb, pinfo, tree, header_type, usb_conv_info, usb_id);

    if (usb_conv_info->transfer_type != URB_CONTROL) {
        usb_tap_queue_packet(pinfo, urb_type, usb_conv_info);
    }

    switch(usb_conv_info->transfer_type) {
    case URB_BULK:
    case URB_INTERRUPT:
        item = proto_tree_add_uint(tree, hf_usb_bInterfaceClass, tvb, 0, 0, usb_conv_info->interfaceClass);
        PROTO_ITEM_SET_GENERATED(item);

        switch (header_type) {

        case USB_HEADER_LINUX_48_BYTES:
        case USB_HEADER_LINUX_64_BYTES:
            /* bulk and interrupt transfers never contain a setup packet */
            proto_tree_add_item(tree, hf_usb_urb_unused_setup_header, tvb, offset, 8, ENC_NA);
            offset += 8;
            if (header_type == USB_HEADER_LINUX_64_BYTES) {
                offset = dissect_linux_usb_pseudo_header_ext(tvb, offset, pinfo, tree);
            }
            break;

        case USB_HEADER_USBPCAP:
            break;

        case USB_HEADER_MAUSB:
            break;

        case USB_HEADER_USBIP:
            break;
        }
        break;

    case URB_CONTROL:
        if (header_type == USB_HEADER_USBPCAP) {
            proto_tree_add_item(tree, hf_usb_control_stage, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            usbpcap_control_stage = tvb_get_guint8(tvb, offset);
            if (usbpcap_control_stage == USB_CONTROL_STAGE_SETUP)
                usb_conv_info->is_setup = TRUE;
            offset++;
            proto_item_set_len(urb_tree_ti, offset);
        }

        if (usb_conv_info->is_request) {
            if (usb_conv_info->is_setup) {
                offset = dissect_usb_setup_request(pinfo, tree, tvb, offset,
                                                   urb_type, usb_conv_info, header_type);

            } else {
                switch (header_type) {

                case USB_HEADER_LINUX_48_BYTES:
                case USB_HEADER_LINUX_64_BYTES:
                    proto_tree_add_item(tree, hf_usb_urb_unused_setup_header, tvb, offset, 8, ENC_NA);
                    offset += 8;
                    if (header_type == USB_HEADER_LINUX_64_BYTES) {
                        offset = dissect_linux_usb_pseudo_header_ext(tvb, offset, pinfo, tree);
                    }
                    break;

                case USB_HEADER_USBPCAP:
                    break;

                case USB_HEADER_MAUSB:
                    break;

                case USB_HEADER_USBIP:
                    break;
                }
            }
        } else {
            /* this is a response */

            switch (header_type) {

            case USB_HEADER_LINUX_48_BYTES:
            case USB_HEADER_LINUX_64_BYTES:
                /* Skip setup header - it's never applicable for responses */
                proto_tree_add_item(tree, hf_usb_urb_unused_setup_header, tvb, offset, 8, ENC_NA);
                offset += 8;
                if (header_type == USB_HEADER_LINUX_64_BYTES) {
                    offset = dissect_linux_usb_pseudo_header_ext(tvb, offset, pinfo, tree);
                }
                break;

            case USB_HEADER_USBPCAP:
                /* Check if this is status stage */
                if ((usb_conv_info->usb_trans_info) &&
                    (usbpcap_control_stage == USB_CONTROL_STAGE_STATUS)) {
                    col_add_fstr(pinfo->cinfo, COL_INFO, "%s Status",
                        val_to_str_ext(usb_conv_info->usb_trans_info->setup.request,
                            &setup_request_names_vals_ext, "Unknown type %x"));
                    /* There is no data to dissect */
                    return;
                }
                break;

            case USB_HEADER_MAUSB:
                break;

            case USB_HEADER_USBIP:
                break;
            }

            offset = dissect_usb_setup_response(pinfo, tree, tvb, offset,
                                                urb_type, usb_conv_info);
        }
        break;
    case URB_ISOCHRONOUS:
        switch (header_type) {

        case USB_HEADER_LINUX_48_BYTES:
        case USB_HEADER_LINUX_64_BYTES:
            offset = dissect_linux_usb_iso_transfer(pinfo, tree, header_type,
                    tvb, offset, usb_conv_info);
            break;

        case USB_HEADER_USBPCAP:
            offset = dissect_usbpcap_iso_packets(pinfo, tree,
                    urb_type, tvb, offset, win32_data_len, usb_conv_info);
            break;

        case USB_HEADER_MAUSB:
            break;

        case USB_HEADER_USBIP:
            offset = dissect_usbip_iso_transfer(pinfo, tree,
                    tvb, offset, iso_numdesc, desc_offset, usb_conv_info);
            break;
        }
        break;

    default:
        /* unknown transfer type */
        switch (header_type) {
        case USB_HEADER_LINUX_48_BYTES:
        case USB_HEADER_LINUX_64_BYTES:
            proto_tree_add_item(tree, hf_usb_urb_unused_setup_header, tvb, offset, 8, ENC_NA);
            offset += 8;
            if (header_type == USB_HEADER_LINUX_64_BYTES) {
                offset = dissect_linux_usb_pseudo_header_ext(tvb, offset, pinfo, tree);
            }
            break;

        case USB_HEADER_USBPCAP:
            break;

        case USB_HEADER_MAUSB:
            break;

        case USB_HEADER_USBIP:
            break;
        }
        break;
    }

    dissect_usb_payload(tvb, pinfo, parent, tree, usb_conv_info, urb_type,
                        offset, device_address);
}

static int
dissect_linux_usb(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent, void* data _U_)
{
    dissect_usb_common(tvb, pinfo, parent, USB_HEADER_LINUX_48_BYTES, NULL);
    return tvb_captured_length(tvb);
}

static int
dissect_linux_usb_mmapped(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent, void* data _U_)
{
    dissect_usb_common(tvb, pinfo, parent, USB_HEADER_LINUX_64_BYTES, NULL);
    return tvb_captured_length(tvb);
}


static int
dissect_win32_usb(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent, void* data _U_)
{
    dissect_usb_common(tvb, pinfo, parent, USB_HEADER_USBPCAP, NULL);
    return tvb_captured_length(tvb);
}

void
proto_register_usb(void)
{
    module_t *usb_module;
    static hf_register_info hf[] = {

    /* USB packet pseudoheader members */

        { &hf_usb_totlen,
          { "Total length", "usb.totlen",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_busunit,
          { "Host controller unit number", "usb.busunit",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_address,
          { "USB device index", "usb.address",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_mode,
          { "Mode of transfer", "usb.address",
            FT_UINT8, BASE_DEC, VALS(usb_freebsd_transfer_mode_vals), 0x0,
            NULL, HFILL }},

        { &hf_usb_freebsd_urb_type,
          { "URB type", "usb.freebsd_type",
            FT_UINT8, BASE_DEC, VALS(usb_freebsd_urb_type_vals), 0x0,
            NULL, HFILL }},

        { &hf_usb_freebsd_transfer_type,
          { "URB transfer type", "usb.freebsd_transfer_type",
            FT_UINT8, BASE_HEX, VALS(usb_freebsd_transfer_type_vals), 0x0,
            NULL, HFILL }},

        { &hf_usb_xferflags,
          { "Transfer flags", "usb.xferflags",
            FT_UINT32, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_xferflags_force_short_xfer,
          { "Force short transfer", "usb.xferflags.force_short_xfer",
            FT_BOOLEAN, 32, NULL, FREEBSD_FLAG_FORCE_SHORT_XFER,
            NULL, HFILL }},

        { &hf_usb_xferflags_short_xfer_ok,
          { "Short transfer OK", "usb.xferflags.short_xfer_ok",
            FT_BOOLEAN, 32, NULL, FREEBSD_FLAG_SHORT_XFER_OK,
            NULL, HFILL }},

        { &hf_usb_xferflags_short_frames_ok,
          { "Short frames OK", "usb.xferflags.short_frames_ok",
            FT_BOOLEAN, 32, NULL, FREEBSD_FLAG_SHORT_FRAMES_OK,
            NULL, HFILL }},

        { &hf_usb_xferflags_pipe_bof,
          { "Pipe BOF", "usb.xferflags.pipe_bof",
            FT_BOOLEAN, 32, NULL, FREEBSD_FLAG_PIPE_BOF,
            NULL, HFILL }},

        { &hf_usb_xferflags_proxy_buffer,
          { "Proxy buffer", "usb.xferflags.proxy_buffer",
            FT_BOOLEAN, 32, NULL, FREEBSD_FLAG_PROXY_BUFFER,
            NULL, HFILL }},

        { &hf_usb_xferflags_ext_buffer,
          { "External buffer", "usb.xferflags.ext_buffer",
            FT_BOOLEAN, 32, NULL, FREEBSD_FLAG_EXT_BUFFER,
            NULL, HFILL }},

        { &hf_usb_xferflags_manual_status,
          { "Manual status", "usb.xferflags.manual_status",
            FT_BOOLEAN, 32, NULL, FREEBSD_FLAG_MANUAL_STATUS,
            NULL, HFILL }},

        { &hf_usb_xferflags_no_pipe_ok,
          { "No pipe OK", "usb.xferflags.no_pipe_ok",
            FT_BOOLEAN, 32, NULL, FREEBSD_FLAG_NO_PIPE_OK,
            NULL, HFILL }},

        { &hf_usb_xferflags_stall_pipe,
          { "Stall pipe", "usb.xferflags.stall_pipe",
            FT_BOOLEAN, 32, NULL, FREEBSD_FLAG_STALL_PIPE,
            NULL, HFILL }},

        { &hf_usb_xferstatus,
          { "Transfer status", "usb.xferstatus",
            FT_UINT32, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_xferstatus_open,
          { "Pipe has been opened", "usb.xferstatus.open",
            FT_BOOLEAN, 32, NULL, FREEBSD_STATUS_OPEN,
            NULL, HFILL }},

        { &hf_usb_xferstatus_transferring,
          { "Transfer in progress", "usb.xferstatus.transferring",
            FT_BOOLEAN, 32, NULL, FREEBSD_STATUS_TRANSFERRING,
            NULL, HFILL }},

        { &hf_usb_xferstatus_did_dma_delay,
          { "Waited for hardware DMA", "usb.xferstatus.did_dma_delay",
            FT_BOOLEAN, 32, NULL, FREEBSD_STATUS_DID_DMA_DELAY,
            NULL, HFILL }},

        { &hf_usb_xferstatus_did_close,
          { "Transfer closed", "usb.xferstatus.did_close",
            FT_BOOLEAN, 32, NULL, FREEBSD_STATUS_DID_CLOSE,
            NULL, HFILL }},

        { &hf_usb_xferstatus_draining,
          { "Draining transfer", "usb.xferstatus.draining",
            FT_BOOLEAN, 32, NULL, FREEBSD_STATUS_DRAINING,
            NULL, HFILL }},

        { &hf_usb_xferstatus_started,
          { "Transfer started", "usb.xferstatus.started",
            FT_BOOLEAN, 32, NULL, FREEBSD_STATUS_STARTED,
            "Whether the transfer is started or stopped", HFILL }},

        { &hf_usb_xferstatus_bw_reclaimed,
          { "Bandwidth reclaimed", "usb.xferstatus.bw_reclaimed",
            FT_BOOLEAN, 32, NULL, FREEBSD_STATUS_BW_RECLAIMED,
            NULL, HFILL }},

        { &hf_usb_xferstatus_control_xfr,
          { "Control transfer", "usb.xferstatus.control_xfr",
            FT_BOOLEAN, 32, NULL, FREEBSD_STATUS_CONTROL_XFR,
            NULL, HFILL }},

        { &hf_usb_xferstatus_control_hdr,
          { "Control header being sent", "usb.xferstatus.control_hdr",
            FT_BOOLEAN, 32, NULL, FREEBSD_STATUS_CONTROL_HDR,
            NULL, HFILL }},

        { &hf_usb_xferstatus_control_act,
          { "Control transfer active", "usb.xferstatus.control_act",
            FT_BOOLEAN, 32, NULL, FREEBSD_STATUS_CONTROL_ACT,
            NULL, HFILL }},

        { &hf_usb_xferstatus_control_stall,
          { "Control transfer should be stalled", "usb.xferstatus.control_stall",
            FT_BOOLEAN, 32, NULL, FREEBSD_STATUS_CONTROL_STALL,
            NULL, HFILL }},

        { &hf_usb_xferstatus_short_frames_ok,
          { "Short frames OK", "usb.xferstatus.short_frames_ok",
            FT_BOOLEAN, 32, NULL, FREEBSD_STATUS_SHORT_FRAMES_OK,
            NULL, HFILL }},

        { &hf_usb_xferstatus_short_xfer_ok,
          { "Short transfer OK", "usb.xferstatus.short_xfer_ok",
            FT_BOOLEAN, 32, NULL, FREEBSD_STATUS_SHORT_XFER_OK,
            NULL, HFILL }},

        { &hf_usb_xferstatus_bdma_enable,
          { "BUS-DMA enabled", "usb.xferstatus.bdma_enable",
            FT_BOOLEAN, 32, NULL, FREEBSD_STATUS_BDMA_ENABLE,
            NULL, HFILL }},

        { &hf_usb_xferstatus_bdma_no_post_sync,
          { "BUS-DMA post sync op not done", "usb.xferstatus.bdma_no_post_sync",
            FT_BOOLEAN, 32, NULL, FREEBSD_STATUS_BDMA_NO_POST_SYNC,
            NULL, HFILL }},

        { &hf_usb_xferstatus_bdma_setup,
          { "BUS-DMA set up", "usb.xferstatus.bdma_setup",
            FT_BOOLEAN, 32, NULL, FREEBSD_STATUS_BDMA_SETUP,
            NULL, HFILL }},

        { &hf_usb_xferstatus_isochronous_xfr,
          { "Isochronous transfer", "usb.xferstatus.isochronous_xfr",
            FT_BOOLEAN, 32, NULL, FREEBSD_STATUS_ISOCHRONOUS_XFR,
            NULL, HFILL }},

        { &hf_usb_xferstatus_curr_dma_set,
          { "Current DMA set", "usb.xferstatus.curr_dma_set",
            FT_UINT32, BASE_DEC, NULL, FREEBSD_STATUS_CURR_DMA_SET,
            NULL, HFILL }},

        { &hf_usb_xferstatus_can_cancel_immed,
          { "Transfer can be cancelled immediately", "usb.xferstatus.can_cancel_immed",
            FT_BOOLEAN, 32, NULL, FREEBSD_STATUS_CAN_CANCEL_IMMED,
            NULL, HFILL }},

        { &hf_usb_xferstatus_doing_callback,
          { "Executing the callback", "usb.xferstatus.doing_callback",
            FT_BOOLEAN, 32, NULL, FREEBSD_STATUS_DOING_CALLBACK,
            NULL, HFILL }},

        { &hf_usb_error,
          { "Error", "usb.error",
            FT_UINT32, BASE_DEC, VALS(usb_freebsd_err_vals), 0x0,
            NULL, HFILL }},

        { &hf_usb_interval,
          { "Interval", "usb.interval",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            "Interval (ms)", HFILL }},

        { &hf_usb_nframes,
          { "Number of following frames", "usb.nframes",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_packet_size,
          { "Packet size used", "usb.packet_size",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_packet_count,
          { "Packet count used", "usb.packet_count",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_speed,
          { "Speed", "usb.speed",
            FT_UINT8, BASE_DEC, VALS(usb_freebsd_speed_vals), 0x0,
            NULL, HFILL }},

        { &hf_usb_frame_length,
          { "Frame length", "usb.frame.length",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_frame_flags,
          { "Frame flags", "usb.frame.flags",
            FT_UINT32, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_frame_flags_read,
          { "Data direction is read", "usb.frame.read",
            FT_BOOLEAN, 32, NULL, FREEBSD_FRAMEFLAG_READ,
            NULL, HFILL }},

        { &hf_usb_frame_flags_data_follows,
          { "Frame contains data", "usb.frame.data_follows",
            FT_BOOLEAN, 32, NULL, FREEBSD_FRAMEFLAG_DATA_FOLLOWS,
            NULL, HFILL }},

        { &hf_usb_frame_data,
          { "Frame data", "usb.frame.data",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_urb_id,
          { "URB id", "usb.urb_id",
            FT_UINT64, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_linux_urb_type,
          { "URB type", "usb.urb_type",
            FT_UINT8, BASE_DEC, VALS(usb_linux_urb_type_vals), 0x0,
            NULL, HFILL }},

        { &hf_usb_linux_transfer_type,
          { "URB transfer type", "usb.transfer_type",
            FT_UINT8, BASE_HEX, VALS(usb_linux_transfer_type_vals), 0x0,
            NULL, HFILL }},

        { &hf_usb_endpoint_number,
          { "Endpoint", "usb.endpoint_number",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            "USB endpoint number", HFILL }},

        { &hf_usb_endpoint_direction,
          { "Direction", "usb.endpoint_number.direction",
            FT_UINT8, BASE_DEC, VALS(usb_endpoint_direction_vals), 0x80,
            "USB endpoint direction", HFILL }},

        { &hf_usb_endpoint_number_value,
          { "Endpoint value", "usb.endpoint_number.endpoint",
            FT_UINT8, BASE_DEC, NULL, 0x7F,
            "USB endpoint value", HFILL }},

        { &hf_usb_device_address,
          { "Device", "usb.device_address",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            "USB device address", HFILL }},

        { &hf_usb_bus_id,
          { "URB bus id", "usb.bus_id",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_setup_flag,
          { "Device setup request", "usb.setup_flag",
            FT_STRING, BASE_NONE, NULL, 0x0,
            "USB device setup request is relevant (0) or not", HFILL }},

        { &hf_usb_data_flag,
          { "Data", "usb.data_flag",
            FT_STRING, BASE_NONE, NULL, 0x0,
            "USB data is present (0) or not", HFILL }},

        { &hf_usb_urb_ts_sec,
          { "URB sec", "usb.urb_ts_sec",
            FT_UINT64, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_urb_ts_usec,
          { "URB usec", "usb.urb_ts_usec",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_urb_status,
          { "URB status", "usb.urb_status",
            FT_INT32, BASE_DEC|BASE_EXT_STRING, &usb_urb_status_vals_ext, 0x0,
            NULL, HFILL }},

        { &hf_usb_urb_len,
          { "URB length [bytes]", "usb.urb_len",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            "URB length in bytes", HFILL }},

        { &hf_usb_urb_data_len,
          { "Data length [bytes]", "usb.data_len",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            "URB data length in bytes", HFILL }},

        { &hf_usb_urb_unused_setup_header,
          { "Unused Setup Header",
            "usb.unused_setup_header", FT_NONE, BASE_NONE,
            NULL, 0x0, NULL, HFILL }},

        { &hf_usb_urb_interval,
          { "Interval",
            "usb.interval", FT_UINT32, BASE_DEC,
            NULL, 0x0, NULL, HFILL }},

        { &hf_usb_urb_start_frame,
          { "Start frame",
            "usb.start_frame", FT_UINT32, BASE_DEC,
            NULL, 0x0, NULL, HFILL }},

        { &hf_usb_urb_copy_of_transfer_flags,
          { "Copy of Transfer Flags",
            "usb.copy_of_transfer_flags", FT_UINT32, BASE_HEX,
            NULL, 0x0, NULL, HFILL }},

        /* Win32 USBPcap pseudoheader */
        { &hf_usb_win32_header_len,
          { "USBPcap pseudoheader length", "usb.usbpcap_header_len",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_irp_id,
          { "IRP ID", "usb.irp_id",
            FT_UINT64, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_usbd_status,
          { "IRP USBD_STATUS", "usb.usbd_status",
            FT_UINT32, BASE_HEX | BASE_EXT_STRING, &win32_usbd_status_vals_ext, 0x0,
            "USB request status value", HFILL }},

        { &hf_usb_function,
          { "URB Function", "usb.function",
            FT_UINT16, BASE_HEX|BASE_EXT_STRING, &win32_urb_function_vals_ext, 0x0,
            NULL, HFILL }},

        { &hf_usb_info,
          { "IRP information", "usb.irp_info",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_usbpcap_info_reserved,
          { "Reserved", "usb.irp_info.reserved",
            FT_UINT8, BASE_HEX, NULL, 0xFE,
            NULL, HFILL }},

        { &hf_usb_usbpcap_info_direction,
          { "Direction", "usb.irp_info.direction",
            FT_UINT8, BASE_HEX, VALS(win32_usb_info_direction_vals), 0x01,
            NULL, HFILL }},

        { &hf_usb_win32_device_address,
          { "Device address", "usb.device_address",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            "Windows USB device address", HFILL }},

        { &hf_usb_win32_data_len,
          { "Packet Data Length", "usb.data_len",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_control_stage,
          { "Control transfer stage", "usb.control_stage",
            FT_UINT8, BASE_DEC, VALS(usb_control_stage_vals), 0x0,
            NULL, HFILL }},

        { &hf_usb_win32_iso_start_frame,
          { "Isochronous transfer start frame", "usb.win32.iso_frame",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_win32_iso_num_packets,
          { "Isochronous transfer number of packets", "usb.win32.iso_num_packets",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_win32_iso_error_count,
          { "Isochronous transfer error count", "usb.win32.iso_error_count",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_win32_iso_offset,
          { "ISO Data offset", "usb.win32.iso_offset",
            FT_UINT32, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_win32_iso_length,
          { "ISO Data length", "usb.win32.iso_data_len",
            FT_UINT32, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_win32_iso_status,
          { "ISO USBD status", "usb.win32.iso_status",
            FT_UINT32, BASE_HEX | BASE_EXT_STRING, &win32_usbd_status_vals_ext, 0x0,
            NULL, HFILL }},


        { &hf_usb_bmRequestType,
          { "bmRequestType", "usb.bmRequestType",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

        /* Only used when response type cannot be determined */
        { &hf_usb_control_response_generic,
          { "CONTROL response data", "usb.control.Response",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_request,
          { "bRequest", "usb.setup.bRequest",
            FT_UINT8, BASE_DEC | BASE_EXT_STRING, &setup_request_names_vals_ext, 0x0,
            NULL, HFILL }},

        /* Same as hf_usb_request but no descriptive text */
        { &hf_usb_request_unknown_class,
          { "bRequest", "usb.setup.bRequest",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_value,
          { "wValue", "usb.setup.wValue",
            FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_index,
          { "wIndex", "usb.setup.wIndex",
            FT_UINT16, BASE_DEC_HEX, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_length,
          { "wLength", "usb.setup.wLength",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_device_wFeatureSelector,
          { "wFeatureSelector", "usb.setup.wFeatureSelector",
            FT_UINT16, BASE_DEC, VALS(usb_device_feature_selector_vals), 0x0,
            NULL, HFILL }},

        { &hf_usb_interface_wFeatureSelector,
          { "wFeatureSelector", "usb.setup.wFeatureSelector",
            FT_UINT16, BASE_DEC, VALS(usb_interface_feature_selector_vals), 0x0,
            NULL, HFILL }},

        { &hf_usb_endpoint_wFeatureSelector,
          { "wFeatureSelector", "usb.setup.wFeatureSelector",
            FT_UINT16, BASE_DEC, VALS(usb_endpoint_feature_selector_vals), 0x0,
            NULL, HFILL }},

        { &hf_usb_wInterface,
          { "wInterface", "usb.setup.wInterface",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_wEndpoint,
          { "wEndpoint", "usb.setup.wEndpoint",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_wStatus,
          { "wStatus", "usb.setup.wStatus",
            FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_wFrameNumber,
          { "wFrameNumber", "usb.setup.wFrameNumber",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

    /* --------------------------------- */
        { &hf_usb_iso_error_count,                /* host endian byte order */
          { "ISO error count", "usb.iso.error_count",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_iso_numdesc,                    /* host endian byte order */
          { "Number of ISO descriptors", "usb.iso.numdesc",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        /* fields of struct mon_bin_isodesc from linux/drivers/usb/mon/mon_bin.c */
        { &hf_usb_iso_status,                     /* host endian byte order */
          { "Status", "usb.iso.iso_status",
            FT_INT32, BASE_DEC|BASE_EXT_STRING, &usb_urb_status_vals_ext, 0x0,
            "ISO descriptor status", HFILL }},

        { &hf_usb_iso_off,                        /* host endian byte order */
          { "Offset [bytes]", "usb.iso.iso_off",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            "ISO data offset in bytes starting from the end of the last ISO descriptor", HFILL }},

        { &hf_usb_iso_len,                        /* host endian byte order */
          { "Length [bytes]", "usb.iso.iso_len",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            "ISO data length in bytes", HFILL }},

        { &hf_usb_iso_actual_len,
          { "Actual Length [bytes]", "usb.iso.iso_actual_len",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            "ISO data actual length in bytes", HFILL }},

        { &hf_usb_iso_pad,                        /* host endian byte order */
          { "Padding", "usb.iso.pad",
            FT_UINT32, BASE_HEX, NULL, 0x0,
            "Padding field of ISO descriptor structure", HFILL }},

        { &hf_usb_iso_data,
          {"ISO Data", "usb.iso.data",
           FT_BYTES, BASE_NONE, NULL, 0x0,
           NULL, HFILL }},
    /* --------------------------------- */
#if 0
        { &hf_usb_data_len,
          {"Application Data Length", "usb.data.length",
           FT_UINT32, BASE_DEC, NULL, 0x0,
           NULL, HFILL }},
#endif

        { &hf_usb_capdata,
          {"Leftover Capture Data", "usb.capdata",
           FT_BYTES, BASE_NONE, NULL, 0x0,
           "Padding added by the USB capture system", HFILL }},

        { &hf_usb_bmRequestType_direction,
          { "Direction", "usb.bmRequestType.direction",
            FT_BOOLEAN, 8, TFS(&tfs_bmrequesttype_direction), USB_DIR_IN,
            NULL, HFILL }},

        { &hf_usb_bmRequestType_type,
          { "Type", "usb.bmRequestType.type",
            FT_UINT8, BASE_HEX, VALS(bmrequesttype_type_vals), USB_TYPE_MASK,
            NULL, HFILL }},

        { &hf_usb_bmRequestType_recipient,
          { "Recipient", "usb.bmRequestType.recipient",
            FT_UINT8, BASE_HEX, VALS(bmrequesttype_recipient_vals), 0x1f,
            NULL, HFILL }},

        { &hf_usb_bDescriptorType,
          { "bDescriptorType", "usb.bDescriptorType",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

        /* Only used when descriptor type cannot be determined */
        { &hf_usb_get_descriptor_resp_generic,
          { "GET DESCRIPTOR Response data", "usb.getDescriptor.Response",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_descriptor_index,
          { "Descriptor Index", "usb.DescriptorIndex",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_language_id,
          { "Language Id", "usb.LanguageId",
            FT_UINT16, BASE_HEX|BASE_EXT_STRING,&usb_langid_vals_ext, 0x0,
            NULL, HFILL }},

        { &hf_usb_bLength,
          { "bLength", "usb.bLength",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_bcdUSB,
          { "bcdUSB", "usb.bcdUSB",
            FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_bDeviceClass,
          { "bDeviceClass", "usb.bDeviceClass",
            FT_UINT8, BASE_HEX|BASE_EXT_STRING, &usb_class_vals_ext, 0x0,
            NULL, HFILL }},

        { &hf_usb_bDeviceSubClass,
          { "bDeviceSubClass", "usb.bDeviceSubClass",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_bDeviceProtocol,
          { "bDeviceProtocol", "usb.bDeviceProtocol",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_bMaxPacketSize0,
          { "bMaxPacketSize0", "usb.bMaxPacketSize0",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_idVendor,
          { "idVendor", "usb.idVendor",
            FT_UINT16, BASE_HEX | BASE_EXT_STRING, &ext_usb_vendors_vals, 0x0,
            NULL, HFILL }},

        { &hf_usb_idProduct,
          { "idProduct", "usb.idProduct",
            FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_bcdDevice,
          { "bcdDevice", "usb.bcdDevice",
            FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_iManufacturer,
          { "iManufacturer", "usb.iManufacturer",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_iProduct,
          { "iProduct", "usb.iProduct",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_iSerialNumber,
          { "iSerialNumber", "usb.iSerialNumber",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_bNumConfigurations,
          { "bNumConfigurations", "usb.bNumConfigurations",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_wLANGID,
          { "wLANGID", "usb.wLANGID",
            FT_UINT16, BASE_HEX|BASE_EXT_STRING,&usb_langid_vals_ext, 0x0,
            NULL, HFILL }},

        { &hf_usb_bString,
          { "bString", "usb.bString",
            FT_STRING, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_bInterfaceNumber,
          { "bInterfaceNumber", "usb.bInterfaceNumber",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_bAlternateSetting,
          { "bAlternateSetting", "usb.bAlternateSetting",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_bNumEndpoints,
          { "bNumEndpoints", "usb.bNumEndpoints",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_bInterfaceClass,
          { "bInterfaceClass", "usb.bInterfaceClass",
            FT_UINT8, BASE_HEX|BASE_EXT_STRING, &usb_class_vals_ext, 0x0,
            NULL, HFILL }},

        { &hf_usb_bInterfaceSubClass,
          { "bInterfaceSubClass", "usb.bInterfaceSubClass",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_bInterfaceSubClass_cdc,
          { "bInterfaceSubClass", "usb.bInterfaceSubClass",
            FT_UINT8, BASE_HEX | BASE_EXT_STRING, &ext_usb_com_subclass_vals, 0x0,
            NULL, HFILL }},

        { &hf_usb_bInterfaceSubClass_hid,
          { "bInterfaceSubClass", "usb.bInterfaceSubClass",
            FT_UINT8, BASE_HEX | BASE_EXT_STRING, &usb_hid_subclass_vals_ext, 0x0,
            NULL, HFILL }},

        { &hf_usb_bInterfaceSubClass_misc,
          { "bInterfaceSubClass", "usb.bInterfaceSubClass",
            FT_UINT8, BASE_HEX | BASE_EXT_STRING, &usb_misc_subclass_vals_ext, 0x0,
            NULL, HFILL }},

        { &hf_usb_bInterfaceSubClass_app,
          { "bInterfaceSubClass", "usb.bInterfaceSubClass",
            FT_UINT8, BASE_HEX | BASE_EXT_STRING, &usb_app_subclass_vals_ext, 0x0,
            NULL, HFILL }},

        { &hf_usb_bInterfaceProtocol,
          { "bInterfaceProtocol", "usb.bInterfaceProtocol",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_bInterfaceProtocol_cdc,
          { "bInterfaceProtocol", "usb.bInterfaceProtocol",
            FT_UINT8, BASE_HEX | BASE_EXT_STRING, &usb_cdc_protocol_vals_ext, 0x0,
            NULL, HFILL }},

        { &hf_usb_bInterfaceProtocol_cdc_data,
          { "bInterfaceProtocol", "usb.bInterfaceProtocol",
            FT_UINT8, BASE_HEX | BASE_EXT_STRING, &usb_cdc_data_protocol_vals_ext, 0x0,
            NULL, HFILL }},

        { &hf_usb_bInterfaceProtocol_hid_boot,
          { "bInterfaceProtocol", "usb.bInterfaceProtocol",
            FT_UINT8, BASE_HEX | BASE_EXT_STRING, &usb_hid_boot_protocol_vals_ext, 0x0,
            NULL, HFILL }},

        { &hf_usb_bInterfaceProtocol_app_dfu,
          { "bInterfaceProtocol", "usb.bInterfaceProtocol",
            FT_UINT8, BASE_HEX | BASE_EXT_STRING, &usb_app_dfu_protocol_vals_ext, 0x0,
            NULL, HFILL }},

        { &hf_usb_bInterfaceProtocol_app_irda,
          { "bInterfaceProtocol", "usb.bInterfaceProtocol",
            FT_UINT8, BASE_HEX | BASE_EXT_STRING, &usb_app_irda_protocol_vals_ext, 0x0,
            NULL, HFILL }},

        { &hf_usb_bInterfaceProtocol_app_usb_test_and_measurement,
          { "bInterfaceProtocol", "usb.bInterfaceProtocol",
            FT_UINT8, BASE_HEX | BASE_EXT_STRING, &usb_app_usb_test_and_measurement_protocol_vals_ext, 0x0,
            NULL, HFILL }},


        { &hf_usb_iInterface,
          { "iInterface", "usb.iInterface",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_bEndpointAddress,
          { "bEndpointAddress", "usb.bEndpointAddress",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_configuration_bmAttributes,
          { "Configuration bmAttributes", "usb.configuration.bmAttributes",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_bmAttributes,
          { "bmAttributes", "usb.bmAttributes",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_bEndpointAttributeTransfer,
          { "Transfertype", "usb.bmAttributes.transfer",
            FT_UINT8, BASE_HEX, VALS(usb_bmAttributes_transfer_vals), 0x03,
            NULL, HFILL }},

        { &hf_usb_bEndpointAttributeSynchonisation,
          { "Synchronisationtype", "usb.bmAttributes.sync",
            FT_UINT8, BASE_HEX, VALS(usb_bmAttributes_sync_vals), 0x0c,
            NULL, HFILL }},

        { &hf_usb_bEndpointAttributeBehaviour,
          { "Behaviourtype", "usb.bmAttributes.behaviour",
            FT_UINT8, BASE_HEX, VALS(usb_bmAttributes_behaviour_vals), 0x30,
            NULL, HFILL }},

        { &hf_usb_wMaxPacketSize,
          { "wMaxPacketSize", "usb.wMaxPacketSize",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_wMaxPacketSize_size,
          { "Maximum Packet Size", "usb.wMaxPacketSize.size",
            FT_UINT16, BASE_DEC, NULL, 0x3FF,
            NULL, HFILL }},

        { &hf_usb_wMaxPacketSize_slots,
          { "Transactions per microframe", "usb.wMaxPacketSize.slots",
            FT_UINT16, BASE_DEC, VALS(usb_wMaxPacketSize_slots_vals), (3<<11),
            NULL, HFILL }},

        { &hf_usb_bInterval,
          { "bInterval", "usb.bInterval",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_wTotalLength,
          { "wTotalLength", "usb.wTotalLength",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_bNumInterfaces,
          { "bNumInterfaces", "usb.bNumInterfaces",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_bConfigurationValue,
          { "bConfigurationValue", "usb.bConfigurationValue",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_iConfiguration,
          { "iConfiguration", "usb.iConfiguration",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_bMaxPower,
          { "bMaxPower", "usb.bMaxPower",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_usb_configuration_legacy10buspowered,
          { "Must be 1", "usb.configuration.legacy10buspowered",
            FT_BOOLEAN, 8, TFS(&tfs_mustbeone), 0x80,
            "Legacy USB 1.0 bus powered", HFILL }},

        { &hf_usb_configuration_selfpowered,
          { "Self-Powered", "usb.configuration.selfpowered",
            FT_BOOLEAN, 8, TFS(&tfs_selfpowered), 0x40,
            NULL, HFILL }},

        { &hf_usb_configuration_remotewakeup,
          { "Remote Wakeup", "usb.configuration.remotewakeup",
            FT_BOOLEAN, 8, TFS(&tfs_remotewakeup), 0x20,
            NULL, HFILL }},

        { &hf_usb_bEndpointAddress_number,
          { "Endpoint Number", "usb.bEndpointAddress.number",
            FT_UINT8, BASE_HEX, NULL, 0x0f,
            NULL, HFILL }},

        { &hf_usb_bEndpointAddress_direction,
          { "Direction", "usb.bEndpointAddress.direction",
            FT_BOOLEAN, 8, TFS(&tfs_endpoint_direction), 0x80,
            NULL, HFILL }},

        { &hf_usb_request_in,
          { "Request in", "usb.request_in",
            FT_FRAMENUM, BASE_NONE,  NULL, 0,
            "The request to this packet is in this packet", HFILL }},

        { &hf_usb_time,
          { "Time from request", "usb.time",
            FT_RELATIVE_TIME, BASE_NONE,  NULL, 0,
            "Time between Request and Response for USB cmds", HFILL }},

        { &hf_usb_response_in,
          { "Response in", "usb.response_in",
            FT_FRAMENUM, BASE_NONE,  NULL, 0,
            "The response to this packet is in this packet", HFILL }},

        { &hf_usb_bFirstInterface,
          { "bFirstInterface", "usb.bFirstInterface",
            FT_UINT8, BASE_DEC,  NULL, 0x0, NULL, HFILL }},

        { &hf_usb_bInterfaceCount,
          { "bInterfaceCount",
            "usb.bInterfaceCount", FT_UINT8, BASE_DEC,
            NULL, 0x0, NULL, HFILL }},

        { &hf_usb_bFunctionClass,
          { "bFunctionClass", "usb.bFunctionClass",
            FT_UINT8, BASE_HEX|BASE_EXT_STRING,  &usb_class_vals_ext, 0x0, NULL, HFILL }},

        { &hf_usb_bFunctionSubClass,
          { "bFunctionSubClass",
            "usb.bFunctionSubClass", FT_UINT8, BASE_HEX,
            NULL, 0x0, NULL, HFILL }},

        { &hf_usb_bFunctionProtocol,
          { "bFunctionProtocol", "usb.bFunctionProtocol",
            FT_UINT8, BASE_HEX,  NULL, 0x0, NULL, HFILL }},

        { &hf_usb_iFunction,
          { "iFunction",
            "usb.iFunction", FT_UINT8, BASE_DEC,
            NULL, 0x0, NULL, HFILL }},

        { &hf_usb_data_fragment,
          { "Data Fragment",
            "usb.data_fragment", FT_BYTES, BASE_NONE,
            NULL, 0x0, NULL, HFILL }},
        { &hf_usb_src,
            { "Source",                              "usb.src",
            FT_STRING, STR_ASCII, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_usb_dst,
            { "Destination",                         "usb.dst",
            FT_STRING, STR_ASCII, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_usb_addr,
            { "Source or Destination",               "usb.addr",
            FT_STRING, STR_ASCII, NULL, 0x0,
            NULL, HFILL }
        }
   };

    static gint *usb_subtrees[] = {
        &ett_usb_hdr,
        &ett_usb_setup_hdr,
        &ett_usb_isodesc,
        &ett_usb_win32_iso_packet,
        &ett_usb_endpoint,
        &ett_usb_xferflags,
        &ett_usb_xferstatus,
        &ett_usb_frame,
        &ett_usb_frame_flags,
        &ett_usb_setup_bmrequesttype,
        &ett_usb_usbpcap_info,
        &ett_descriptor_device,
        &ett_configuration_bmAttributes,
        &ett_configuration_bEndpointAddress,
        &ett_endpoint_bmAttributes,
        &ett_endpoint_wMaxPacketSize
    };

    static ei_register_info ei[] = {
        { &ei_usb_bLength_even, { "usb.bLength.even", PI_PROTOCOL, PI_WARN, "Invalid STRING DESCRIPTOR Length (must be even)", EXPFILL }},
        { &ei_usb_bLength_too_short, { "usb.bLength.too_short", PI_MALFORMED, PI_ERROR, "Invalid STRING DESCRIPTOR Length (must be 2 or larger)", EXPFILL }},
        { &ei_usb_desc_length_invalid, { "usb.desc_length.invalid", PI_MALFORMED, PI_ERROR, "Invalid descriptor length", EXPFILL }},
        { &ei_usb_invalid_setup, { "usb.setup.invalid", PI_MALFORMED, PI_ERROR, "Only control URBs may contain a setup packet", EXPFILL }}
    };

    expert_module_t* expert_usb;

    proto_usb = proto_register_protocol("USB", "USB", "usb");
    proto_register_field_array(proto_usb, hf, array_length(hf));
    proto_register_subtree_array(usb_subtrees, array_length(usb_subtrees));

    expert_usb = expert_register_protocol(proto_usb);
    expert_register_field_array(expert_usb, ei, array_length(ei));

    device_to_product_table = wmem_tree_new_autoreset(wmem_epan_scope(), wmem_file_scope());
    device_to_protocol_table = wmem_tree_new_autoreset(wmem_epan_scope(), wmem_file_scope());
    device_to_dissector = register_dissector_table("usb.device",     "USB device",   proto_usb, FT_UINT32, BASE_HEX);
    protocol_to_dissector = register_dissector_table("usb.protocol", "USB protocol", proto_usb, FT_UINT32, BASE_HEX);
    product_to_dissector = register_dissector_table("usb.product",   "USB product",  proto_usb, FT_UINT32, BASE_HEX);

    usb_bulk_dissector_table = register_dissector_table("usb.bulk",
        "USB bulk endpoint", proto_usb, FT_UINT8, BASE_DEC);
    heur_bulk_subdissector_list = register_heur_dissector_list("usb.bulk", proto_usb);
    usb_control_dissector_table = register_dissector_table("usb.control",
        "USB control endpoint", proto_usb, FT_UINT8, BASE_DEC);
    heur_control_subdissector_list = register_heur_dissector_list("usb.control", proto_usb);
    usb_interrupt_dissector_table = register_dissector_table("usb.interrupt",
        "USB interrupt endpoint", proto_usb, FT_UINT8, BASE_DEC);
    heur_interrupt_subdissector_list = register_heur_dissector_list("usb.interrupt", proto_usb);
    usb_descriptor_dissector_table = register_dissector_table("usb.descriptor",
        "USB descriptor", proto_usb, FT_UINT8, BASE_DEC);

    usb_module = prefs_register_protocol(proto_usb, NULL);
    prefs_register_bool_preference(usb_module, "try_heuristics",
        "Try heuristic sub-dissectors",
        "Try to decode a packet using a heuristic sub-dissector before "
         "attempting to dissect the packet using the \"usb.bulk\", \"usb.interrupt\" or "
         "\"usb.control\" dissector tables.", &try_heuristics);

    usb_tap = register_tap("usb");

    register_decode_as(&usb_protocol_da);
    register_decode_as(&usb_product_da);
    register_decode_as(&usb_device_da);

    usb_address_type = address_type_dissector_register("AT_USB", "USB Address", usb_addr_to_str, usb_addr_str_len, NULL, NULL, NULL, NULL, NULL);

    register_conversation_table(proto_usb, TRUE, usb_conversation_packet, usb_hostlist_packet);
}

void
proto_reg_handoff_usb(void)
{
    dissector_handle_t  linux_usb_handle;
    dissector_handle_t  linux_usb_mmapped_handle;
    dissector_handle_t  win32_usb_handle;
    dissector_handle_t  freebsd_usb_handle;

    linux_usb_handle = create_dissector_handle(dissect_linux_usb, proto_usb);
    linux_usb_mmapped_handle = create_dissector_handle(dissect_linux_usb_mmapped,
                                                       proto_usb);
    win32_usb_handle = create_dissector_handle(dissect_win32_usb, proto_usb);
    freebsd_usb_handle = create_dissector_handle(dissect_freebsd_usb, proto_usb);

    dissector_add_uint("wtap_encap", WTAP_ENCAP_USB_LINUX, linux_usb_handle);
    dissector_add_uint("wtap_encap", WTAP_ENCAP_USB_LINUX_MMAPPED, linux_usb_mmapped_handle);
    dissector_add_uint("wtap_encap", WTAP_ENCAP_USBPCAP, win32_usb_handle);
    dissector_add_uint("wtap_encap", WTAP_ENCAP_USB_FREEBSD, freebsd_usb_handle);
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
