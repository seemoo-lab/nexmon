/* packet-usb-hid.c
 *
 * USB HID dissector
 * By Adam Nielsen <a.nielsen@shikadi.net> 2009
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
#include "packet-usb.h"
#include "packet-usb-hid.h"
#include "packet-btsdp.h"


void proto_register_usb_hid(void);
void proto_reg_handoff_usb_hid(void);

/* protocols and header fields */
static int proto_usb_hid = -1;
static int hf_usb_hid_item_bSize = -1;
static int hf_usb_hid_item_bType = -1;
static int hf_usb_hid_mainitem_bTag = -1;
static int hf_usb_hid_globalitem_bTag = -1;
static int hf_usb_hid_localitem_bTag = -1;
static int hf_usb_hid_longitem_bTag = -1;
static int hf_usb_hid_item_bDataSize = -1;
static int hf_usb_hid_item_bLongItemTag = -1;
static int hf_usb_hid_item_unk_data = -1;

static int hf_usb_hid_mainitem_bit0 = -1;
static int hf_usb_hid_mainitem_bit1 = -1;
static int hf_usb_hid_mainitem_bit2 = -1;
static int hf_usb_hid_mainitem_bit3 = -1;
static int hf_usb_hid_mainitem_bit4 = -1;
static int hf_usb_hid_mainitem_bit5 = -1;
static int hf_usb_hid_mainitem_bit6 = -1;
static int hf_usb_hid_mainitem_bit7 = -1;
static int hf_usb_hid_mainitem_bit7_input = -1;
static int hf_usb_hid_mainitem_bit8 = -1;
static int hf_usb_hid_mainitem_colltype = -1;

static int hf_usb_hid_globalitem_usage = -1;
static int hf_usb_hid_globalitem_log_min = -1;
static int hf_usb_hid_globalitem_log_max = -1;
static int hf_usb_hid_globalitem_phy_min = -1;
static int hf_usb_hid_globalitem_phy_max = -1;
static int hf_usb_hid_globalitem_unit_exp = -1;
static int hf_usb_hid_globalitem_unit_sys = -1;
static int hf_usb_hid_globalitem_unit_len = -1;
static int hf_usb_hid_globalitem_unit_mass = -1;
static int hf_usb_hid_globalitem_unit_time = -1;
static int hf_usb_hid_globalitem_unit_temp = -1;
static int hf_usb_hid_globalitem_unit_current = -1;
static int hf_usb_hid_globalitem_unit_brightness = -1;
static int hf_usb_hid_globalitem_report_size = -1;
static int hf_usb_hid_globalitem_report_id = -1;
static int hf_usb_hid_globalitem_report_count = -1;
static int hf_usb_hid_globalitem_push = -1;
static int hf_usb_hid_globalitem_pop = -1;

static int hf_usb_hid_localitem_usage = -1;
static int hf_usb_hid_localitem_usage_min = -1;
/* static int hf_usb_hid_localitem_usage_max = -1; */
static int hf_usb_hid_localitem_desig_index = -1;
static int hf_usb_hid_localitem_desig_min = -1;
static int hf_usb_hid_localitem_desig_max = -1;
static int hf_usb_hid_localitem_string_index = -1;
static int hf_usb_hid_localitem_string_min = -1;
static int hf_usb_hid_localitem_string_max = -1;
static int hf_usb_hid_localitem_delimiter = -1;

static gint ett_usb_hid_report = -1;
static gint ett_usb_hid_item_header = -1;
static gint ett_usb_hid_wValue = -1;
static gint ett_usb_hid_descriptor = -1;

static int hf_usb_hid_request = -1;
static int hf_usb_hid_value = -1;
static int hf_usb_hid_index = -1;
static int hf_usb_hid_length = -1;
static int hf_usb_hid_report_type = -1;
static int hf_usb_hid_report_id = -1;
static int hf_usb_hid_duration = -1;
static int hf_usb_hid_zero = -1;

static int hf_usb_hid_bcdHID = -1;
static int hf_usb_hid_bCountryCode = -1;
static int hf_usb_hid_bNumDescriptors = -1;
static int hf_usb_hid_bDescriptorIndex = -1;
static int hf_usb_hid_bDescriptorType = -1;
static int hf_usb_hid_wInterfaceNumber = -1;
static int hf_usb_hid_wDescriptorLength = -1;

static int hf_usbhid_boot_report_keyboard_modifier_right_gui = -1;
static int hf_usbhid_boot_report_keyboard_modifier_right_alt = -1;
static int hf_usbhid_boot_report_keyboard_modifier_right_shift = -1;
static int hf_usbhid_boot_report_keyboard_modifier_right_ctrl = -1;
static int hf_usbhid_boot_report_keyboard_modifier_left_gui = -1;
static int hf_usbhid_boot_report_keyboard_modifier_left_alt = -1;
static int hf_usbhid_boot_report_keyboard_modifier_left_shift = -1;
static int hf_usbhid_boot_report_keyboard_modifier_left_ctrl = -1;
static int hf_usbhid_boot_report_keyboard_reserved = -1;
static int hf_usbhid_boot_report_keyboard_keycode_1 = -1;
static int hf_usbhid_boot_report_keyboard_keycode_2 = -1;
static int hf_usbhid_boot_report_keyboard_keycode_3 = -1;
static int hf_usbhid_boot_report_keyboard_keycode_4 = -1;
static int hf_usbhid_boot_report_keyboard_keycode_5 = -1;
static int hf_usbhid_boot_report_keyboard_keycode_6 = -1;
static int hf_usbhid_boot_report_keyboard_leds_constants = -1;
static int hf_usbhid_boot_report_keyboard_leds_kana = -1;
static int hf_usbhid_boot_report_keyboard_leds_compose = -1;
static int hf_usbhid_boot_report_keyboard_leds_scroll_lock = -1;
static int hf_usbhid_boot_report_keyboard_leds_caps_lock = -1;
static int hf_usbhid_boot_report_keyboard_leds_num_lock = -1;
static int hf_usbhid_boot_report_mouse_button_8 = -1;
static int hf_usbhid_boot_report_mouse_button_7 = -1;
static int hf_usbhid_boot_report_mouse_button_6 = -1;
static int hf_usbhid_boot_report_mouse_button_5 = -1;
static int hf_usbhid_boot_report_mouse_button_4 = -1;
static int hf_usbhid_boot_report_mouse_button_middle = -1;
static int hf_usbhid_boot_report_mouse_button_right = -1;
static int hf_usbhid_boot_report_mouse_button_left = -1;
static int hf_usbhid_boot_report_mouse_x_displacement = -1;
static int hf_usbhid_boot_report_mouse_y_displacement = -1;
static int hf_usbhid_boot_report_mouse_horizontal_scroll_wheel = -1;
static int hf_usbhid_boot_report_mouse_vertical_scroll_wheel = -1;
static int hf_usbhid_data = -1;

static const true_false_string tfs_mainitem_bit0 = {"Constant", "Data"};
static const true_false_string tfs_mainitem_bit1 = {"Variable", "Array"};
static const true_false_string tfs_mainitem_bit2 = {"Relative", "Absolute"};
static const true_false_string tfs_mainitem_bit3 = {"Wrap", "No Wrap"};
static const true_false_string tfs_mainitem_bit4 = {"Non Linear", "Linear"};
static const true_false_string tfs_mainitem_bit5 = {"No Preferred", "Preferred State"};
static const true_false_string tfs_mainitem_bit6 = {"Null state", "No Null position"};
static const true_false_string tfs_mainitem_bit7 = {"Volatile", "Non Volatile"};
static const true_false_string tfs_mainitem_bit8 = {"Buffered Bytes", "Bit Field"};


struct usb_hid_global_state {
    unsigned int usage_page;
};


/* HID class specific descriptor types */
#define USB_DT_HID        0x21
#define USB_DT_HID_REPORT 0x22
static const value_string hid_descriptor_type_vals[] = {
    {USB_DT_HID, "HID"},
    {USB_DT_HID_REPORT, "HID Report"},
    {0,NULL}
};
static value_string_ext hid_descriptor_type_vals_ext =
               VALUE_STRING_EXT_INIT(hid_descriptor_type_vals);


#define USBHID_SIZE_MASK  0x03
#define USBHID_TYPE_MASK  0x0C
#define USBHID_TAG_MASK   0xF0

static const value_string usb_hid_item_bSize_vals[] = {
    {0, "0 bytes"},
    {1, "1 byte"},
    {2, "2 bytes"},
    {3, "4 bytes"},
    {0, NULL}
};

#define USBHID_ITEMTYPE_MAIN    0
#define USBHID_ITEMTYPE_GLOBAL  1
#define USBHID_ITEMTYPE_LOCAL   2
#define USBHID_ITEMTYPE_LONG    3
static const value_string usb_hid_item_bType_vals[] = {
    {USBHID_ITEMTYPE_MAIN,   "Main"},
    {USBHID_ITEMTYPE_GLOBAL, "Global"},
    {USBHID_ITEMTYPE_LOCAL,  "Local"},
    {USBHID_ITEMTYPE_LONG,   "Long item"},
    {0, NULL}
};

#define USBHID_MAINITEM_TAG_INPUT           8
#define USBHID_MAINITEM_TAG_OUTPUT          9
#define USBHID_MAINITEM_TAG_FEATURE        11
#define USBHID_MAINITEM_TAG_COLLECTION     10
#define USBHID_MAINITEM_TAG_ENDCOLLECTION  12
static const value_string usb_hid_mainitem_bTag_vals[] = {
    {USBHID_MAINITEM_TAG_INPUT,         "Input"},
    {USBHID_MAINITEM_TAG_OUTPUT,        "Output"},
    {USBHID_MAINITEM_TAG_FEATURE,       "Feature"},
    {USBHID_MAINITEM_TAG_COLLECTION,    "Collection"},
    {USBHID_MAINITEM_TAG_ENDCOLLECTION, "End collection"},
    {0, NULL}
};
#define USBHID_GLOBALITEM_TAG_USAGE_PAGE    0
#define USBHID_GLOBALITEM_TAG_LOG_MIN       1
#define USBHID_GLOBALITEM_TAG_LOG_MAX       2
#define USBHID_GLOBALITEM_TAG_PHY_MIN       3
#define USBHID_GLOBALITEM_TAG_PHY_MAX       4
#define USBHID_GLOBALITEM_TAG_UNIT_EXP      5
#define USBHID_GLOBALITEM_TAG_UNIT          6
#define USBHID_GLOBALITEM_TAG_REPORT_SIZE   7
#define USBHID_GLOBALITEM_TAG_REPORT_ID     8
#define USBHID_GLOBALITEM_TAG_REPORT_COUNT  9
#define USBHID_GLOBALITEM_TAG_PUSH         10
#define USBHID_GLOBALITEM_TAG_POP          11
static const value_string usb_hid_globalitem_bTag_vals[] = {
    {USBHID_GLOBALITEM_TAG_USAGE_PAGE,   "Usage"},
    {USBHID_GLOBALITEM_TAG_LOG_MIN,      "Logical minimum"},
    {USBHID_GLOBALITEM_TAG_LOG_MAX,      "Logical maximum"},
    {USBHID_GLOBALITEM_TAG_PHY_MIN,      "Physical minimum"},
    {USBHID_GLOBALITEM_TAG_PHY_MAX,      "Physical maximum"},
    {USBHID_GLOBALITEM_TAG_UNIT_EXP,     "Unit exponent"},
    {USBHID_GLOBALITEM_TAG_UNIT,         "Units"},
    {USBHID_GLOBALITEM_TAG_REPORT_SIZE,  "Report size"},
    {USBHID_GLOBALITEM_TAG_REPORT_ID,    "Report ID"},
    {USBHID_GLOBALITEM_TAG_REPORT_COUNT, "Report count"},
    {USBHID_GLOBALITEM_TAG_PUSH,         "Push"},
    {USBHID_GLOBALITEM_TAG_POP,          "Pop"},
    {12, "[Reserved]"},
    {13, "[Reserved]"},
    {14, "[Reserved]"},
    {15, "[Reserved]"},
    {0, NULL}
};
#define USBHID_LOCALITEM_TAG_USAGE_PAGE     0
#define USBHID_LOCALITEM_TAG_USAGE_MIN      1
#define USBHID_LOCALITEM_TAG_USAGE_MAX      2
#define USBHID_LOCALITEM_TAG_DESIG_INDEX    3
#define USBHID_LOCALITEM_TAG_DESIG_MIN      4
#define USBHID_LOCALITEM_TAG_DESIG_MAX      5
/* No 6 in spec */
#define USBHID_LOCALITEM_TAG_STRING_INDEX   7
#define USBHID_LOCALITEM_TAG_STRING_MIN     8
#define USBHID_LOCALITEM_TAG_STRING_MAX     9
#define USBHID_LOCALITEM_TAG_DELIMITER     10 /* Also listed as reserved in spec! */
static const value_string usb_hid_localitem_bTag_vals[] = {
    {USBHID_LOCALITEM_TAG_USAGE_PAGE,   "Usage"},
    {USBHID_LOCALITEM_TAG_USAGE_MIN,    "Usage minimum"},
    {USBHID_LOCALITEM_TAG_USAGE_MAX,    "Usage maximum"},
    {USBHID_LOCALITEM_TAG_DESIG_INDEX,  "Designator index"},
    {USBHID_LOCALITEM_TAG_DESIG_MIN,    "Designator minimum"},
    {USBHID_LOCALITEM_TAG_DESIG_MAX,    "Designator maximum"},
    {USBHID_LOCALITEM_TAG_STRING_INDEX, "String index"},
    {USBHID_LOCALITEM_TAG_STRING_MIN,   "String minimum"},
    {USBHID_LOCALITEM_TAG_STRING_MAX,   "String maximum"},
    {USBHID_LOCALITEM_TAG_DELIMITER,    "Delimiter"},
    {11, "[Reserved]"},
    {12, "[Reserved]"},
    {13, "[Reserved]"},
    {14, "[Reserved]"},
    {15, "[Reserved]"},
    {0, NULL}
};
static const value_string usb_hid_longitem_bTag_vals[] = {
    {15, "Long item"},
    {0, NULL}
};

static const range_string usb_hid_mainitem_colltype_vals[] = {
    {0x00, 0x00, "Physical"},
    {0x01, 0x01, "Application"},
    {0x02, 0x02, "Logical"},
    {0x03, 0x03, "Report"},
    {0x04, 0x04, "Named array"},
    {0x05, 0x05, "Usage switch"},
    {0x06, 0x06, "Usage modifier"},
    {0x07, 0x7F, "[Reserved]"},
    {0x80, 0xFF, "[Vendor-defined]"},
    {0, 0, NULL}
};

static const value_string usb_hid_globalitem_unit_exp_vals[] = {
    {0x0, "n^0"},
    {0x1, "n^1"},
    {0x2, "n^2"},
    {0x3, "n^3"},
    {0x4, "n^4"},
    {0x5, "n^5"},
    {0x6, "n^6"},
    {0x7, "n^7"},
    {0x8, "n^-8"},
    {0x9, "n^-7"},
    {0xA, "n^-6"},
    {0xB, "n^-5"},
    {0xC, "n^-4"},
    {0xD, "n^-3"},
    {0xE, "n^-2"},
    {0xF, "n^-1"},
    {0, NULL}
};
static const range_string usb_hid_item_usage_vals[] = {
    {0x00, 0x00, "Undefined"},
    {0x01, 0x01, "Generic desktop controls"},
    {0x02, 0x02, "Simulation controls"},
    {0x03, 0x03, "VR controls"},
    {0x04, 0x04, "Sport controls"},
    {0x05, 0x05, "Game controls"},
    {0x06, 0x06, "Generic device controls"},
    {0x07, 0x07, "Keyboard/keypad"},
    {0x08, 0x08, "LEDs"},
    {0x09, 0x09, "Button"},
    {0x0A, 0x0A, "Ordinal"},
    {0x0B, 0x0B, "Telephony"},
    {0x0C, 0x0C, "Consumer"},
    {0x0D, 0x0D, "Digitizer"},
    {0x0E, 0x0E, "[Reserved]"},
    {0x0F, 0x0F, "Physical Interface Device (PID) page"},
    {0x10, 0x10, "Unicode"},
    {0x11, 0x13, "[Reserved]"},
    {0x14, 0x14, "Alphanumeric display"},
    {0x15, 0x3F, "[Reserved]"},
    {0x40, 0x40, "Medical instruments"},
    {0x41, 0x7F, "[Reserved]"},
    {0x80, 0x83, "Monitor page"},
    {0x84, 0x87, "Power page"},
    {0x88, 0x8B, "[Reserved]"},
    {0x8C, 0x8C, "Bar code scanner page"},
    {0x8D, 0x8D, "Scale page"},
    {0x8E, 0x8E, "Magnetic Stripe Reading (MSR) devices"},
    {0x8F, 0x8F, "[Reserved Point of Sale page]"},
    {0x90, 0x90, "Camera control page"},
    {0x91, 0x91, "Arcade page"},
    {0x92, 0xFEFF, "[Reserved]"},
    {0xFF00, 0xFFFF, "[Vendor-defined]"},
    {0, 0, NULL}
};

static const value_string keycode_vals[] = {
    { 0x00,   "<ACTION KEY UP>" },
    { 0x01,   "ErrorRollOver" },
    { 0x02,   "POSTFail" },
    { 0x03,   "ErrorUndefined" },

    { 0x04,   "a" },
    { 0x05,   "b" },
    { 0x06,   "c" },
    { 0x07,   "d" },
    { 0x08,   "e" },
    { 0x09,   "f" },
    { 0x0A,   "g" },
    { 0x0B,   "h" },
    { 0x0C,   "i" },
    { 0x0D,   "j" },
    { 0x0E,   "k" },
    { 0x0F,   "l" },
    { 0x10,   "m" },
    { 0x11,   "n" },
    { 0x12,   "o" },
    { 0x13,   "p" },
    { 0x14,   "q" },
    { 0x15,   "r" },
    { 0x16,   "s" },
    { 0x17,   "t" },
    { 0x18,   "u" },
    { 0x19,   "v" },
    { 0x1A,   "w" },
    { 0x1B,   "x" },
    { 0x1C,   "y" },
    { 0x1D,   "z" },

    { 0x1E,   "1" },
    { 0x1F,   "2" },
    { 0x20,   "3" },
    { 0x21,   "4" },
    { 0x22,   "5" },
    { 0x23,   "6" },
    { 0x24,   "7" },
    { 0x25,   "8" },
    { 0x26,   "9" },
    { 0x27,   "0" },

    { 0x28,   "ENTER" },
    { 0x29,   "Escape" },
    { 0x2A,   "Backspace" },
    { 0x2B,   "Tab" },
    { 0x2C,   "Spacebar" },

    { 0x2D,   "-" },
    { 0x2E,   "=" },
    { 0x2F,   "[" },
    { 0x30,   "]" },
    { 0x31,   "\\" },
    { 0x32,   "NonUS #/~" },
    { 0x33,   ";" },
    { 0x34,   "'" },
    { 0x35,   "`" },
    { 0x36,   "," },
    { 0x37,   "." },
    { 0x38,   "/" },
    { 0x39,   "CapsLock" },
    { 0x3A,   "F1" },
    { 0x3B,   "F2" },
    { 0x3C,   "F3" },
    { 0x3D,   "F4" },
    { 0x3E,   "F5" },
    { 0x3F,   "F6" },
    { 0x40,   "F7" },
    { 0x41,   "F8" },
    { 0x42,   "F9" },
    { 0x43,   "F10" },
    { 0x44,   "F11" },
    { 0x45,   "F12" },
    { 0x46,   "PrintScreen" },
    { 0x47,   "ScrollLock" },
    { 0x48,   "Pause" },
    { 0x49,   "Insert" },
    { 0x4A,   "Home" },
    { 0x4B,   "PageUp" },
    { 0x4C,   "DeleteForward" },
    { 0x4D,   "End" },
    { 0x4E,   "PageDown" },
    { 0x4F,   "RightArrow" },
    { 0x50,   "LeftArrow" },
    { 0x51,   "DownArrow" },
    { 0x52,   "UpArrow" },
    { 0x53,   "NumLock" },

    /* Keypad */
    { 0x54,   "Keypad /" },
    { 0x55,   "Keypad *" },
    { 0x56,   "Keypad -" },
    { 0x57,   "Keypad +" },
    { 0x58,   "Keypad ENTER" },
    { 0x59,   "Keypad 1" },
    { 0x5A,   "Keypad 2" },
    { 0x5B,   "Keypad 3" },
    { 0x5C,   "Keypad 4" },
    { 0x5D,   "Keypad 5" },
    { 0x5E,   "Keypad 6" },
    { 0x5F,   "Keypad 7" },
    { 0x60,   "Keypad 8" },
    { 0x61,   "Keypad 9" },
    { 0x62,   "Keypad 0" },
    { 0x63,   "Keypad ." },

    /* non PC AT */
    { 0x64,   "NonUS \\/|" },
    { 0x65,   "Application" },
    { 0x66,   "Power" },
    { 0x67,   "Keypad =" },
    { 0x68,   "F13" },
    { 0x69,   "F14" },
    { 0x6A,   "F15" },
    { 0x6B,   "F16" },
    { 0x6C,   "F17" },
    { 0x6D,   "F18" },
    { 0x6E,   "F19" },
    { 0x6F,   "F20" },

    { 0x70,   "F21" },
    { 0x71,   "F22" },
    { 0x72,   "F23" },
    { 0x73,   "F24" },
    { 0x74,   "Execute" },
    { 0x75,   "Help" },
    { 0x76,   "Menu" },
    { 0x77,   "Select" },
    { 0x78,   "Stop" },
    { 0x79,   "Again" },
    { 0x7A,   "Undo" },
    { 0x7B,   "Cut" },
    { 0x7C,   "Copy" },
    { 0x7D,   "Paste" },
    { 0x7E,   "Find" },
    { 0x7F,   "Mute" },

    { 0x80,   "VolumeUp" },
    { 0x81,   "VolumeDown" },
    { 0x82,   "Locking CapsLock" },
    { 0x83,   "Locking NumLock" },
    { 0x84,   "Locking ScrollLock" },
    { 0x85,   "Keypad Comma" },
    { 0x86,   "Keypad EqualSign" },
    { 0x87,   "International1" },
    { 0x88,   "International2" },
    { 0x89,   "International3" },
    { 0x8A,   "International4" },
    { 0x8B,   "International5" },
    { 0x8C,   "International6" },
    { 0x8D,   "International7" },
    { 0x8E,   "International8" },
    { 0x8F,   "International9" },

    { 0x90,   "LANG1" },
    { 0x91,   "LANG2" },
    { 0x92,   "LANG3" },
    { 0x93,   "LANG4" },
    { 0x94,   "LANG5" },
    { 0x95,   "LANG6" },
    { 0x96,   "LANG7" },
    { 0x97,   "LANG8" },
    { 0x98,   "LANG9" },
    { 0x99,   "AlternateErase" },
    { 0x9A,   "SysReq/Attention" },
    { 0x9B,   "Cancel" },
    { 0x9C,   "Clear" },
    { 0x9D,   "Prior" },
    { 0x9E,   "Return" },
    { 0x9F,   "Separator" },

    { 0xA0,   "Out" },
    { 0xA1,   "Oper" },
    { 0xA2,   "Clear/Again" },
    { 0xA3,   "CrSel/Props" },
    { 0xA4,   "ExSel" },
    /* 0xA5..0xAF - reserved */
    { 0xB0,   "Keypad 00" },
    { 0xB1,   "Keypad 000" },
    { 0xB2,   "ThousandsSeparator" },
    { 0xB3,   "DecimalSeparator" },
    { 0xB4,   "CurrencyUnit" },
    { 0xB5,   "CurrencySubunit" },
    { 0xB6,   "Keypad (" },
    { 0xB7,   "Keypad )" },
    { 0xB8,   "Keypad {" },
    { 0xB9,   "Keypad }" },
    { 0xBA,   "Keypad Tab" },
    { 0xBB,   "Keypad Backspace" },
    { 0xBC,   "Keypad A" },
    { 0xBD,   "Keypad B" },
    { 0xBE,   "Keypad C" },
    { 0xBF,   "Keypad D" },

    { 0xC0,   "Keypad E" },
    { 0xC1,   "Keypad F" },
    { 0xC2,   "Keypad XOR" },
    { 0xC3,   "Keypad ^" },
    { 0xC4,   "Keypad %" },
    { 0xC5,   "Keypad <" },
    { 0xC6,   "Keypad >" },
    { 0xC7,   "Keypad &" },
    { 0xC8,   "Keypad &&" },
    { 0xC9,   "Keypad |" },
    { 0xCA,   "Keypad ||" },
    { 0xCB,   "Keypad :" },
    { 0xCC,   "Keypad #" },
    { 0xCD,   "Keypad Space" },
    { 0xCE,   "Keypad @" },
    { 0xCF,   "Keypad !" },

    { 0xD0,   "Keypad Memory Store" },
    { 0xD1,   "Keypad Memory Recall" },
    { 0xD2,   "Keypad Memory Clear" },
    { 0xD3,   "Keypad Memory Add" },
    { 0xD4,   "Keypad Memory Subtract" },
    { 0xD5,   "Keypad Memory Multiply" },
    { 0xD6,   "Keypad Memory Divide" },
    { 0xD7,   "Keypad +/-" },
    { 0xD8,   "Keypad Clear" },
    { 0xD9,   "Keypad Clear Entry" },
    { 0xDA,   "Keypad Binary" },
    { 0xDB,   "Keypad Octal" },
    { 0xDC,   "Keypad Decimal" },
    { 0xDD,   "Keypad Hexadecimal" },
    /* 0xDE..0xDF - reserved,  */
    { 0xE0,   "LeftControl" },
    { 0xE1,   "LeftShift" },
    { 0xE2,   "LeftAlt" },
    { 0xE3,   "LeftGUI" },
    { 0xE4,   "RightControl" },
    { 0xE5,   "RightShift" },
    { 0xE6,   "RightAlt" },
    { 0xE7,   "RightGUI" },

    { 0, NULL }
};
value_string_ext keycode_vals_ext = VALUE_STRING_EXT_INIT(keycode_vals);

/* Dissector for the data in a HID main report. */
static int
dissect_usb_hid_report_mainitem_data(packet_info *pinfo _U_, proto_tree *tree, tvbuff_t *tvb, int offset, unsigned int bSize, unsigned int bTag)
{
    switch (bTag) {
        case USBHID_MAINITEM_TAG_INPUT:
        case USBHID_MAINITEM_TAG_OUTPUT:
        case USBHID_MAINITEM_TAG_FEATURE:
            proto_tree_add_item(tree, hf_usb_hid_mainitem_bit0, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            proto_tree_add_item(tree, hf_usb_hid_mainitem_bit1, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            proto_tree_add_item(tree, hf_usb_hid_mainitem_bit2, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            proto_tree_add_item(tree, hf_usb_hid_mainitem_bit3, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            proto_tree_add_item(tree, hf_usb_hid_mainitem_bit4, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            proto_tree_add_item(tree, hf_usb_hid_mainitem_bit5, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            proto_tree_add_item(tree, hf_usb_hid_mainitem_bit6, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            if (bTag == USBHID_MAINITEM_TAG_INPUT) {
                proto_tree_add_item(tree, hf_usb_hid_mainitem_bit7_input, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            } else {
                proto_tree_add_item(tree, hf_usb_hid_mainitem_bit7, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            }
            if (bSize > 1) {
                proto_tree_add_item(tree, hf_usb_hid_mainitem_bit8, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            } else {
                proto_tree_add_boolean_format_value(tree, hf_usb_hid_mainitem_bit8, tvb, offset, 0, FALSE, "Buffered bytes (default, no second byte present)");
            }
            break;
        case USBHID_MAINITEM_TAG_COLLECTION:
            proto_tree_add_item(tree, hf_usb_hid_mainitem_colltype, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            break;
        case USBHID_MAINITEM_TAG_ENDCOLLECTION:
            /* No item data */
            break;
        default:
            proto_tree_add_item(tree, hf_usb_hid_item_unk_data, tvb, offset, bSize, ENC_NA);
            break;
    }
    offset += bSize;
    return offset;
}

/* Dissector for the data in a HID main report. */
static int
dissect_usb_hid_report_globalitem_data(packet_info *pinfo _U_, proto_tree *tree, tvbuff_t *tvb, int offset, unsigned int bSize, unsigned int bTag, struct usb_hid_global_state *global)
{
    switch (bTag) {
        case USBHID_GLOBALITEM_TAG_USAGE_PAGE:
            switch (bSize) {
                case 1: global->usage_page = tvb_get_guint8(tvb, offset); break;
                case 2: global->usage_page = tvb_get_letohs(tvb, offset); break;
                case 3: global->usage_page = tvb_get_letoh24(tvb, offset); break;
                case 4: global->usage_page = tvb_get_letohl(tvb, offset); break;
                default: global->usage_page = 0; break;
            }
            proto_tree_add_item(tree, hf_usb_hid_globalitem_usage, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            break;
        case USBHID_GLOBALITEM_TAG_LOG_MIN:
            proto_tree_add_item(tree, hf_usb_hid_globalitem_log_min, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            break;
        case USBHID_GLOBALITEM_TAG_LOG_MAX:
            proto_tree_add_item(tree, hf_usb_hid_globalitem_log_max, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            break;
        case USBHID_GLOBALITEM_TAG_PHY_MIN:
            proto_tree_add_item(tree, hf_usb_hid_globalitem_phy_min, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            break;
        case USBHID_GLOBALITEM_TAG_PHY_MAX:
            proto_tree_add_item(tree, hf_usb_hid_globalitem_phy_max, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            break;
        case USBHID_GLOBALITEM_TAG_UNIT_EXP:
            proto_tree_add_item(tree, hf_usb_hid_globalitem_unit_exp, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            break;
        case USBHID_GLOBALITEM_TAG_UNIT:
            proto_tree_add_item(tree, hf_usb_hid_globalitem_unit_sys, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            proto_tree_add_item(tree, hf_usb_hid_globalitem_unit_len, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            proto_tree_add_item(tree, hf_usb_hid_globalitem_unit_mass, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            proto_tree_add_item(tree, hf_usb_hid_globalitem_unit_time, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            proto_tree_add_item(tree, hf_usb_hid_globalitem_unit_temp, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            proto_tree_add_item(tree, hf_usb_hid_globalitem_unit_current, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            proto_tree_add_item(tree, hf_usb_hid_globalitem_unit_brightness, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            break;
        case USBHID_GLOBALITEM_TAG_REPORT_SIZE:
            proto_tree_add_item(tree, hf_usb_hid_globalitem_report_size, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            break;
        case USBHID_GLOBALITEM_TAG_REPORT_ID:
            proto_tree_add_item(tree, hf_usb_hid_globalitem_report_id, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            break;
        case USBHID_GLOBALITEM_TAG_REPORT_COUNT:
            proto_tree_add_item(tree, hf_usb_hid_globalitem_report_count, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            break;
        case USBHID_GLOBALITEM_TAG_PUSH:
            proto_tree_add_item(tree, hf_usb_hid_globalitem_push, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            break;
        case USBHID_GLOBALITEM_TAG_POP:
            proto_tree_add_item(tree, hf_usb_hid_globalitem_pop, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            break;
        default:
            proto_tree_add_item(tree, hf_usb_hid_item_unk_data, tvb, offset, bSize, ENC_NA);
            break;
    }
    offset += bSize;
    return offset;
}

/* Dissector for the data in a HID main report. */
static int
dissect_usb_hid_report_localitem_data(packet_info *pinfo _U_, proto_tree *tree, tvbuff_t *tvb, int offset, unsigned int bSize, unsigned int bTag, struct usb_hid_global_state *global)
{
    unsigned int usage_page = 0xffffffff; /* in case bSize == 0 */

    switch (bTag) {
        case USBHID_LOCALITEM_TAG_USAGE_PAGE:
            if (bSize > 2) {
                /* Full page ID */
                proto_tree_add_item(tree, hf_usb_hid_localitem_usage, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            } else {
                /* Only lower few bits given, need to combine with last global ID */
                if (bSize == 1)
                    usage_page = (global->usage_page & 0xFFFFFF00) | tvb_get_guint8(tvb, offset);
                else if (bSize == 2)
                    usage_page = (global->usage_page & 0xFFFF0000) | tvb_get_ntohs(tvb, offset);
                proto_tree_add_uint(tree, hf_usb_hid_localitem_usage, tvb, offset, bSize, usage_page);
            }
            break;
        case USBHID_LOCALITEM_TAG_USAGE_MIN:
            proto_tree_add_item(tree, hf_usb_hid_localitem_usage_min, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            break;
        case USBHID_LOCALITEM_TAG_USAGE_MAX:
            proto_tree_add_item(tree, hf_usb_hid_localitem_usage, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            break;
        case USBHID_LOCALITEM_TAG_DESIG_INDEX:
            proto_tree_add_item(tree, hf_usb_hid_localitem_desig_index, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            break;
        case USBHID_LOCALITEM_TAG_DESIG_MIN:
            proto_tree_add_item(tree, hf_usb_hid_localitem_desig_min, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            break;
        case USBHID_LOCALITEM_TAG_DESIG_MAX:
            proto_tree_add_item(tree, hf_usb_hid_localitem_desig_max, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            break;
        case USBHID_LOCALITEM_TAG_STRING_INDEX:
            proto_tree_add_item(tree, hf_usb_hid_localitem_string_index, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            break;
        case USBHID_LOCALITEM_TAG_STRING_MIN:
            proto_tree_add_item(tree, hf_usb_hid_localitem_string_min, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            break;
        case USBHID_LOCALITEM_TAG_STRING_MAX:
            proto_tree_add_item(tree, hf_usb_hid_localitem_string_max, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            break;
        case USBHID_LOCALITEM_TAG_DELIMITER:
            proto_tree_add_item(tree, hf_usb_hid_localitem_delimiter, tvb, offset, bSize, ENC_LITTLE_ENDIAN);
            break;
        default:
            proto_tree_add_item(tree, hf_usb_hid_item_unk_data, tvb, offset, bSize, ENC_NA);
            break;
    }
    offset += bSize;
    return offset;
}

/* Dissector for individual HID report items.  Recursive. */
static int
dissect_usb_hid_report_item(packet_info *pinfo _U_, proto_tree *parent_tree, tvbuff_t *tvb, int offset, usb_conv_info_t *usb_conv_info _U_, const struct usb_hid_global_state *global)
{
    proto_item *subitem;
    proto_tree *tree, *subtree;
    int old_offset;
    unsigned int tmp;
    unsigned int bSize, bType, bTag;
    const value_string *usb_hid_cur_bTag_vals;
    int hf_usb_hid_curitem_bTag;
    struct usb_hid_global_state cur_global;
    memcpy(&cur_global, global, sizeof(struct usb_hid_global_state));

    while (tvb_reported_length_remaining(tvb, offset) > 0)
    {
        old_offset=offset;

        tmp = tvb_get_guint8(tvb, offset);
        bSize = tmp & USBHID_SIZE_MASK;
        if (bSize == 3) bSize++; /* 3 == four bytes */
        bType = (tmp & USBHID_TYPE_MASK) >> 2;
        bTag = (tmp & USBHID_TAG_MASK) >> 4;

        switch (bType) {
            case USBHID_ITEMTYPE_MAIN:
                hf_usb_hid_curitem_bTag = hf_usb_hid_mainitem_bTag;
                usb_hid_cur_bTag_vals = usb_hid_mainitem_bTag_vals;
                break;
            case USBHID_ITEMTYPE_GLOBAL:
                hf_usb_hid_curitem_bTag = hf_usb_hid_globalitem_bTag;
                usb_hid_cur_bTag_vals = usb_hid_globalitem_bTag_vals;
                break;
            case USBHID_ITEMTYPE_LOCAL:
                hf_usb_hid_curitem_bTag = hf_usb_hid_localitem_bTag;
                usb_hid_cur_bTag_vals = usb_hid_localitem_bTag_vals;
                break;
            default: /* Only USBHID_ITEMTYPE_LONG, but keep compiler happy */
                hf_usb_hid_curitem_bTag = hf_usb_hid_longitem_bTag;
                usb_hid_cur_bTag_vals = usb_hid_longitem_bTag_vals;
                break;
        }

        subtree = proto_tree_add_subtree_format(parent_tree, tvb, offset, bSize + 1,
            ett_usb_hid_item_header, &subitem, "%s item (%s)",
            val_to_str(bType, usb_hid_item_bType_vals, "Unknown/%u"),
            val_to_str(bTag, usb_hid_cur_bTag_vals, "Unknown/%u tag")
        );

        tree = proto_tree_add_subtree(subtree, tvb, offset, 1, ett_usb_hid_item_header, NULL, "Header");
        proto_tree_add_item(tree, hf_usb_hid_item_bSize, tvb, offset,   1, ENC_LITTLE_ENDIAN);
        proto_tree_add_item(tree, hf_usb_hid_item_bType, tvb, offset,   1, ENC_LITTLE_ENDIAN);
        proto_tree_add_item(tree, hf_usb_hid_curitem_bTag, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        offset++;
        if ((bType == 3) && (bTag == 16)) {
            /* Long item */
            bSize = tvb_get_guint8(tvb, offset);
            proto_tree_add_item(subtree, hf_usb_hid_item_bDataSize, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset++;
            proto_tree_add_item(subtree, hf_usb_hid_item_bLongItemTag, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset++;
            proto_tree_add_item(subtree, hf_usb_hid_item_unk_data, tvb, offset, bSize, ENC_NA);
            offset += bSize;
        } else {
            /* Short item */
            switch (bType) {
                case USBHID_ITEMTYPE_MAIN:
                    offset = dissect_usb_hid_report_mainitem_data(pinfo, subtree, tvb, offset, bSize, bTag);
                    break;
                case USBHID_ITEMTYPE_GLOBAL:
                    offset = dissect_usb_hid_report_globalitem_data(pinfo, subtree, tvb, offset, bSize, bTag, &cur_global);
                    break;
                case USBHID_ITEMTYPE_LOCAL:
                    offset = dissect_usb_hid_report_localitem_data(pinfo, subtree, tvb, offset, bSize, bTag, &cur_global);
                    break;
                default: /* Only USBHID_ITEMTYPE_LONG, but keep compiler happy */
                    proto_tree_add_item(subtree, hf_usb_hid_item_unk_data, tvb, offset, bSize, ENC_NA);
                    offset += bSize;
                    break;
            }
        }

        if (bType == USBHID_ITEMTYPE_MAIN) {
            if (bTag == USBHID_MAINITEM_TAG_COLLECTION) {
                /* Begin collection, nest following elements under us */
                offset = dissect_usb_hid_report_item(pinfo, subtree, tvb, offset, usb_conv_info, &cur_global);
                proto_item_set_len(subitem, offset-old_offset);
            } else if (bTag == USBHID_MAINITEM_TAG_ENDCOLLECTION) {
                /* End collection, break out to parent tree item */
                break;
            }
        }
    }
    return offset;
}

/* Dissector for HID "GET DESCRIPTOR" subtype. */
int
dissect_usb_hid_get_report_descriptor(packet_info *pinfo _U_, proto_tree *parent_tree, tvbuff_t *tvb, int offset, usb_conv_info_t *usb_conv_info)
{
    proto_item *item;
    proto_tree *tree;
    int old_offset=offset;
    struct usb_hid_global_state initial_global;

    memset(&initial_global, 0, sizeof(struct usb_hid_global_state));

    item = proto_tree_add_protocol_format(parent_tree, proto_usb_hid, tvb, offset,
                                          -1, "HID Report");
    tree = proto_item_add_subtree(item, ett_usb_hid_report);
    offset = dissect_usb_hid_report_item(pinfo, tree, tvb, offset, usb_conv_info, &initial_global);

    proto_item_set_len(item, offset-old_offset);

    return offset;
}

/* Dissector for HID GET_REPORT request. See USBHID 1.11, Chapter 7.2.1 Get_Report Request */
static void
dissect_usb_hid_get_report(packet_info *pinfo _U_, proto_tree *tree, tvbuff_t *tvb, int offset, gboolean is_request, usb_conv_info_t *usb_conv_info _U_)
{
    proto_item *item;
    proto_tree *subtree;

    if (!is_request)
        return;

    item = proto_tree_add_item(tree, hf_usb_hid_value, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    subtree = proto_item_add_subtree(item, ett_usb_hid_wValue);

    /* Report Type in the high byte, Report ID in the low byte */
    proto_tree_add_item(subtree, hf_usb_hid_report_id, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset++;
    proto_tree_add_item(subtree, hf_usb_hid_report_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset++;

    proto_tree_add_item(tree, hf_usb_hid_index, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    proto_tree_add_item(tree, hf_usb_hid_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    /*offset += 2;*/
}

/* Dissector for HID SET_REPORT request. See USBHID 1.11, Chapter 7.2.2 Set_Report Request */
static void
dissect_usb_hid_set_report(packet_info *pinfo _U_, proto_tree *tree, tvbuff_t *tvb, int offset, gboolean is_request, usb_conv_info_t *usb_conv_info _U_)
{
    proto_item *item;
    proto_tree *subtree;

    if (!is_request)
        return;

    item = proto_tree_add_item(tree, hf_usb_hid_value, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    subtree = proto_item_add_subtree(item, ett_usb_hid_wValue);

    proto_tree_add_item(subtree, hf_usb_hid_report_id, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset++;
    proto_tree_add_item(subtree, hf_usb_hid_report_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset++;

    proto_tree_add_item(tree, hf_usb_hid_index, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    proto_tree_add_item(tree, hf_usb_hid_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    /*offset += 2;*/
}


/* Dissector for HID GET_IDLE request. See USBHID 1.11, Chapter 7.2.3 Get_Idle Request */
static void
dissect_usb_hid_get_idle(packet_info *pinfo _U_, proto_tree *tree, tvbuff_t *tvb, int offset, gboolean is_request, usb_conv_info_t *usb_conv_info _U_)
{
    proto_item *item;
    proto_tree *subtree;

    if (!is_request)
        return;

    item = proto_tree_add_item(tree, hf_usb_hid_value, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    subtree = proto_item_add_subtree(item, ett_usb_hid_wValue);

    proto_tree_add_item(subtree, hf_usb_hid_report_id, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset++;
    proto_tree_add_item(subtree, hf_usb_hid_zero, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset++;

    proto_tree_add_item(tree, hf_usb_hid_index, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    proto_tree_add_item(tree, hf_usb_hid_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    /*offset += 2;*/
}

/* Dissector for HID SET_IDLE request. See USBHID 1.11, Chapter 7.2.4 Set_Idle Request */
static void
dissect_usb_hid_set_idle(packet_info *pinfo _U_, proto_tree *tree, tvbuff_t *tvb, int offset, gboolean is_request, usb_conv_info_t *usb_conv_info _U_)
{
    proto_item *item;
    proto_tree *subtree;

    if (!is_request)
        return;

    item = proto_tree_add_item(tree, hf_usb_hid_value, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    subtree = proto_item_add_subtree(item, ett_usb_hid_wValue);

    /* Duration in the high byte, Report ID in the low byte */
    proto_tree_add_item(subtree, hf_usb_hid_report_id, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset++;
    proto_tree_add_item(subtree, hf_usb_hid_duration, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset++;

    proto_tree_add_item(tree, hf_usb_hid_index, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    proto_tree_add_item(tree, hf_usb_hid_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    /*offset += 2;*/
}

/* Dissector for HID GET_PROTOCOL request. See USBHID 1.11, Chapter 7.2.5 Get_Protocol Request */
static void
dissect_usb_hid_get_protocol(packet_info *pinfo _U_, proto_tree *tree, tvbuff_t *tvb, int offset, gboolean is_request, usb_conv_info_t *usb_conv_info _U_)
{
    if (!is_request)
        return;

    proto_tree_add_item(tree, hf_usb_hid_value, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    proto_tree_add_item(tree, hf_usb_hid_index, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    proto_tree_add_item(tree, hf_usb_hid_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    /*offset += 2;*/
}

/* Dissector for HID SET_PROTOCOL request. See USBHID 1.11, Chapter 7.2.6 Set_Protocol Request */
static void
dissect_usb_hid_set_protocol(packet_info *pinfo _U_, proto_tree *tree, tvbuff_t *tvb, int offset, gboolean is_request, usb_conv_info_t *usb_conv_info _U_)
{
    if (!is_request)
        return;

    proto_tree_add_item(tree, hf_usb_hid_value, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    proto_tree_add_item(tree, hf_usb_hid_index, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    proto_tree_add_item(tree, hf_usb_hid_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    /*offset += 2;*/
}


typedef void (*usb_setup_dissector)(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset, gboolean is_request, usb_conv_info_t *usb_conv_info);

typedef struct _usb_setup_dissector_table_t {
    guint8 request;
    usb_setup_dissector dissector;
} usb_setup_dissector_table_t;


/* USBHID 1.11, Chapter 7.2 Class-Specific Requests */
#define USB_HID_SETUP_GET_REPORT      0x01
#define USB_HID_SETUP_GET_IDLE        0x02
#define USB_HID_SETUP_GET_PROTOCOL    0x03
/* 0x04..0x08: Reserved */
#define USB_HID_SETUP_SET_REPORT      0x09
#define USB_HID_SETUP_SET_IDLE        0x0A
#define USB_HID_SETUP_SET_PROTOCOL    0x0B

static const usb_setup_dissector_table_t setup_dissectors[] = {
    { USB_HID_SETUP_GET_REPORT,   dissect_usb_hid_get_report },
    { USB_HID_SETUP_GET_IDLE,     dissect_usb_hid_get_idle },
    { USB_HID_SETUP_GET_PROTOCOL, dissect_usb_hid_get_protocol },
    { USB_HID_SETUP_SET_REPORT,   dissect_usb_hid_set_report },
    { USB_HID_SETUP_SET_IDLE,     dissect_usb_hid_set_idle },
    { USB_HID_SETUP_SET_PROTOCOL, dissect_usb_hid_set_protocol },
    { 0, NULL }
};

static const value_string setup_request_names_vals[] = {
    { USB_HID_SETUP_GET_REPORT,   "GET_REPORT" },
    { USB_HID_SETUP_GET_IDLE,     "GET_IDLE" },
    { USB_HID_SETUP_GET_PROTOCOL, "GET_PROTOCOL" },
    { USB_HID_SETUP_SET_REPORT,   "SET_REPORT" },
    { USB_HID_SETUP_SET_IDLE,     "SET_IDLE" },
    { USB_HID_SETUP_SET_PROTOCOL, "SET_PROTOCOL" },
    { 0, NULL }
};

static const value_string usb_hid_report_type_vals[] = {
    { 1, "Input" },
    { 2, "Output" },
    { 3, "Feature" },
    { 0, NULL }
};

static gint
dissect_usb_hid_boot_keyboard_input_report(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    gint      offset = 0;
    gboolean  shortcut_helper = FALSE;
    guint     modifier;
    guint     keycode;

    proto_tree_add_item(tree, hf_usbhid_boot_report_keyboard_modifier_right_gui, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_usbhid_boot_report_keyboard_modifier_right_alt, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_usbhid_boot_report_keyboard_modifier_right_shift, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_usbhid_boot_report_keyboard_modifier_right_ctrl, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_usbhid_boot_report_keyboard_modifier_left_gui, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_usbhid_boot_report_keyboard_modifier_left_alt, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_usbhid_boot_report_keyboard_modifier_left_shift, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_usbhid_boot_report_keyboard_modifier_left_ctrl, tvb, offset, 1, ENC_BIG_ENDIAN);
    modifier = tvb_get_guint8(tvb, offset);

    col_append_str(pinfo->cinfo, COL_INFO, " - ");
    if (modifier & 0x80) {
        col_append_str(pinfo->cinfo, COL_INFO, "RIGHT GUI");
        shortcut_helper = TRUE;
    }
    if (modifier & 0x40) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, " + ");
        col_append_str(pinfo->cinfo, COL_INFO, "RIGHT ALT");
        shortcut_helper = TRUE;
    }
    if (modifier & 0x20) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, " + ");
        col_append_str(pinfo->cinfo, COL_INFO, "RIGHT SHIFT");
        shortcut_helper = TRUE;
    }
    if (modifier & 0x10) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, " + ");
        col_append_str(pinfo->cinfo, COL_INFO, "RIGHT CTRL");
        shortcut_helper = TRUE;
    }
    if (modifier & 0x08) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, " + ");
        col_append_str(pinfo->cinfo, COL_INFO, "LEFT GUI");
        shortcut_helper = TRUE;
    }
    if (modifier & 0x04) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, " + ");
        col_append_str(pinfo->cinfo, COL_INFO, "LEFT ALT");
        shortcut_helper = TRUE;
    }
    if (modifier & 0x02) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, " + ");
        col_append_str(pinfo->cinfo, COL_INFO, "LEFT SHIFT");
        shortcut_helper = TRUE;
    }
    if (modifier & 0x01) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, " + ");
        col_append_str(pinfo->cinfo, COL_INFO, "LEFT CTRL");
        shortcut_helper = TRUE;
    }
    offset += 1;

    proto_tree_add_item(tree, hf_usbhid_boot_report_keyboard_reserved, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;

    proto_tree_add_item(tree, hf_usbhid_boot_report_keyboard_keycode_1, tvb, offset, 1, ENC_BIG_ENDIAN);
    keycode = tvb_get_guint8(tvb, offset);
    offset += 1;

    if (keycode) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, " + ");
        col_append_fstr(pinfo->cinfo, COL_INFO, "%s", val_to_str_ext(keycode, &keycode_vals_ext, "Unknown"));
        shortcut_helper = TRUE;
    }

    proto_tree_add_item(tree, hf_usbhid_boot_report_keyboard_keycode_2, tvb, offset, 1, ENC_BIG_ENDIAN);
    keycode = tvb_get_guint8(tvb, offset);
    offset += 1;

    if (keycode) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, " + ");
        col_append_fstr(pinfo->cinfo, COL_INFO, "%s", val_to_str_ext(keycode, &keycode_vals_ext, "Unknown"));
        shortcut_helper = TRUE;
    }

    proto_tree_add_item(tree, hf_usbhid_boot_report_keyboard_keycode_3, tvb, offset, 1, ENC_BIG_ENDIAN);
    keycode = tvb_get_guint8(tvb, offset);
    offset += 1;

    if (keycode) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, " + ");
        col_append_fstr(pinfo->cinfo, COL_INFO, "%s", val_to_str_ext(keycode, &keycode_vals_ext, "Unknown"));
        shortcut_helper = TRUE;
    }

    proto_tree_add_item(tree, hf_usbhid_boot_report_keyboard_keycode_4, tvb, offset, 1, ENC_BIG_ENDIAN);
    keycode = tvb_get_guint8(tvb, offset);
    offset += 1;

    if (keycode) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, " + ");
        col_append_fstr(pinfo->cinfo, COL_INFO, "%s", val_to_str_ext(keycode, &keycode_vals_ext, "Unknown"));
        shortcut_helper = TRUE;
    }

    proto_tree_add_item(tree, hf_usbhid_boot_report_keyboard_keycode_5, tvb, offset, 1, ENC_BIG_ENDIAN);
    keycode = tvb_get_guint8(tvb, offset);
    offset += 1;

    if (keycode) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, " + ");
        col_append_fstr(pinfo->cinfo, COL_INFO, "%s", val_to_str_ext(keycode, &keycode_vals_ext, "Unknown"));
        shortcut_helper = TRUE;
    }

    proto_tree_add_item(tree, hf_usbhid_boot_report_keyboard_keycode_6, tvb, offset, 1, ENC_BIG_ENDIAN);
    keycode = tvb_get_guint8(tvb, offset);
    offset += 1;

    if (keycode) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, " + ");
        col_append_fstr(pinfo->cinfo, COL_INFO, "%s", val_to_str_ext(keycode, &keycode_vals_ext, "Unknown"));
        shortcut_helper = TRUE;
    }

    if (shortcut_helper == FALSE) {
        col_append_str(pinfo->cinfo, COL_INFO, "<action key up>");
    }

    return offset;
}

static gint
dissect_usb_hid_boot_keyboard_output_report(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    gint      offset = 0;
    gboolean  shortcut_helper = FALSE;
    guint     leds;

    proto_tree_add_item(tree, hf_usbhid_boot_report_keyboard_leds_constants, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_usbhid_boot_report_keyboard_leds_kana, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_usbhid_boot_report_keyboard_leds_compose, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_usbhid_boot_report_keyboard_leds_scroll_lock, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_usbhid_boot_report_keyboard_leds_caps_lock, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_usbhid_boot_report_keyboard_leds_num_lock, tvb, offset, 1, ENC_BIG_ENDIAN);
    leds = tvb_get_guint8(tvb, offset);

    col_append_str(pinfo->cinfo, COL_INFO, " - LEDs: ");
    if (leds & 0x01) {
        col_append_str(pinfo->cinfo, COL_INFO, "NumLock");
        shortcut_helper = TRUE;
    }
    if (leds & 0x02) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, ", ");
        col_append_str(pinfo->cinfo, COL_INFO, "CapsLock");
        shortcut_helper = TRUE;
    }
    if (leds & 0x04) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, ", ");
        col_append_str(pinfo->cinfo, COL_INFO, "ScrollLock");
        shortcut_helper = TRUE;
    }
    if (leds & 0x08) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, ", ");
        col_append_str(pinfo->cinfo, COL_INFO, "Compose");
        shortcut_helper = TRUE;
    }
    if (leds & 0x10) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, ", ");
        col_append_str(pinfo->cinfo, COL_INFO, "Kana");
        shortcut_helper = TRUE;
    }
    if (leds & 0x20) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, ", ");
        col_append_str(pinfo->cinfo, COL_INFO, "Constant1");
        shortcut_helper = TRUE;
    }
    if (leds & 0x40) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, ", ");
        col_append_str(pinfo->cinfo, COL_INFO, "Constant2");
        shortcut_helper = TRUE;
    }
    if (leds & 0x80) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, ", ");
        col_append_str(pinfo->cinfo, COL_INFO, "Constant3");
        /*shortcut_helper = TRUE;*/
    }
    if (!leds) {
        col_append_str(pinfo->cinfo, COL_INFO, "none");
    }

    offset += 1;

    return offset;
}

static gint
dissect_usb_hid_boot_mouse_input_report(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    gint      offset = 0;
    gboolean  shortcut_helper = FALSE;
    guint     buttons;

    proto_tree_add_item(tree, hf_usbhid_boot_report_mouse_button_8, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_usbhid_boot_report_mouse_button_7, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_usbhid_boot_report_mouse_button_6, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_usbhid_boot_report_mouse_button_5, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_usbhid_boot_report_mouse_button_4, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_usbhid_boot_report_mouse_button_middle, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_usbhid_boot_report_mouse_button_right, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_usbhid_boot_report_mouse_button_left, tvb, offset, 1, ENC_BIG_ENDIAN);
    buttons = tvb_get_guint8(tvb, offset);
    offset += 1;

    if (buttons) col_append_str(pinfo->cinfo, COL_INFO, " - ");
    if (buttons & 0x01) {
        col_append_str(pinfo->cinfo, COL_INFO, "Button LEFT");
        shortcut_helper = TRUE;
    }
    if (buttons & 0x02) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, " + ");
        col_append_str(pinfo->cinfo, COL_INFO, "Button RIGHT");
        shortcut_helper = TRUE;
    }
    if (buttons & 0x04) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, " + ");
        col_append_str(pinfo->cinfo, COL_INFO, "Button MIDDLE");
    }
    if (buttons & 0x08) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, " + ");
        col_append_str(pinfo->cinfo, COL_INFO, "Button 4");
        shortcut_helper = TRUE;
    }
    if (buttons & 0x10) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, " + ");
        col_append_str(pinfo->cinfo, COL_INFO, "Button 5");
        shortcut_helper = TRUE;
    }
    if (buttons & 0x20) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, " + ");
        col_append_str(pinfo->cinfo, COL_INFO, "Button 6");
        shortcut_helper = TRUE;
    }
    if (buttons & 0x40) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, " + ");
        col_append_str(pinfo->cinfo, COL_INFO, "Button 7");
        shortcut_helper = TRUE;
    }
    if (buttons & 0x80) {
        if (shortcut_helper) col_append_str(pinfo->cinfo, COL_INFO, " + ");
        col_append_str(pinfo->cinfo, COL_INFO, "Button 8");
        /* Not necessary, this is the last case where it is used
         * shortcut_helper = TRUE;
         */
    }

    proto_tree_add_item(tree, hf_usbhid_boot_report_mouse_x_displacement, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    proto_tree_add_item(tree, hf_usbhid_boot_report_mouse_y_displacement, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;

    /* not really in HID Specification */
    if (tvb_reported_length_remaining(tvb, offset)) {
        proto_tree_add_item(tree, hf_usbhid_boot_report_mouse_horizontal_scroll_wheel, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset += 1;
    }

    /* not really in HID Specification */
    if (tvb_reported_length_remaining(tvb, offset)) {
        proto_tree_add_item(tree, hf_usbhid_boot_report_mouse_vertical_scroll_wheel, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset += 1;
    }

    if (tvb_reported_length_remaining(tvb, offset)) {
        proto_tree_add_item(tree, hf_usbhid_data, tvb, offset, -1, ENC_NA);
        offset += tvb_captured_length_remaining(tvb, offset);
    }

    return offset;
}


/* dissect a "standard" control message that's sent to an interface */
static gint
dissect_usb_hid_control_std_intf(tvbuff_t *tvb, packet_info *pinfo,
        proto_tree *tree, usb_conv_info_t *usb_conv_info)
{
    gint              offset = 0;
    usb_trans_info_t *usb_trans_info;
    guint8            req;

    usb_trans_info = usb_conv_info->usb_trans_info;

    /* XXX - can we do some plausibility checks here? */

    col_set_str(pinfo->cinfo, COL_PROTOCOL, "USBHID");

    /* we can't use usb_conv_info->is_request since usb_conv_info
       was replaced with the interface conversation */
    if (usb_trans_info->request_in == pinfo->num) {
        /* the tvb that we see here is the setup packet
           without the request type byte */

        req = tvb_get_guint8(tvb, offset);
        if (req != USB_SETUP_GET_DESCRIPTOR)
            return offset;
        col_clear(pinfo->cinfo, COL_INFO);
        col_append_fstr(pinfo->cinfo, COL_INFO, "GET DESCRIPTOR Request");
        offset += 1;

        proto_tree_add_item(tree, hf_usb_hid_bDescriptorIndex, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        usb_trans_info->u.get_descriptor.usb_index = tvb_get_guint8(tvb, offset);
        offset += 1;

        proto_tree_add_item(tree, hf_usb_hid_bDescriptorType, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        usb_trans_info->u.get_descriptor.type = tvb_get_guint8(tvb, offset);
        col_append_fstr(pinfo->cinfo, COL_INFO, " %s",
                val_to_str_ext(usb_trans_info->u.get_descriptor.type,
                    &hid_descriptor_type_vals_ext, "Unknown type %u"));
        offset += 1;

        proto_tree_add_item(tree, hf_usb_hid_wInterfaceNumber, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;

        proto_tree_add_item(tree, hf_usb_hid_wDescriptorLength, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;
    }
    else {
        col_clear(pinfo->cinfo, COL_INFO);
        col_append_fstr(pinfo->cinfo, COL_INFO, "GET DESCRIPTOR Response");
        col_append_fstr(pinfo->cinfo, COL_INFO, " %s",
                val_to_str_ext(usb_trans_info->u.get_descriptor.type,
                    &hid_descriptor_type_vals_ext, "Unknown type %u"));
        if (usb_trans_info->u.get_descriptor.type == USB_DT_HID_REPORT) {
            offset = dissect_usb_hid_get_report_descriptor(
                    pinfo, tree, tvb, offset, usb_conv_info);
        }
    }

    return offset;
}

/* dissect a class-specific control message that's sent to an interface */
static gint
dissect_usb_hid_control_class_intf(tvbuff_t *tvb, packet_info *pinfo,
        proto_tree *tree, usb_conv_info_t *usb_conv_info)
{
    usb_trans_info_t *usb_trans_info;
    gboolean is_request;
    int offset = 0;
    usb_setup_dissector dissector = NULL;
    const usb_setup_dissector_table_t *tmp;

    usb_trans_info = usb_conv_info->usb_trans_info;

    is_request = (pinfo->srcport==NO_ENDPOINT);

    /* Check valid values for bmRequestType. See Chapter 7.2 in USBHID 1.11 */
    for (tmp = setup_dissectors; tmp->dissector; tmp++) {
        if (tmp->request == usb_trans_info->setup.request) {
            dissector = tmp->dissector;
            break;
        }
    }
    /* No, we could not find any class specific dissector for this request
     * return 0 and let USB try any of the standard requests.
     */
    if (!dissector) {
        return 0;
    }

    col_set_str(pinfo->cinfo, COL_PROTOCOL, "USBHID");

    col_add_fstr(pinfo->cinfo, COL_INFO, "%s %s",
    val_to_str(usb_trans_info->setup.request, setup_request_names_vals, "Unknown type %x"),
        is_request ? "Request" : "Response");

    if (is_request) {
        proto_tree_add_item(tree, hf_usb_hid_request, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        offset += 1;
    }

    dissector(pinfo, tree, tvb, offset, is_request, usb_conv_info);
    return tvb_captured_length(tvb);
}

/* Dissector for HID class-specific control request as defined in
 * USBHID 1.11, Chapter 7.2.
 * returns the number of bytes consumed */
static gint
dissect_usb_hid_control(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data)
{
    usb_conv_info_t *usb_conv_info;
    usb_trans_info_t *usb_trans_info;
    guint8 type, recip;

    usb_conv_info = (usb_conv_info_t *)data;
    if (!usb_conv_info)
        return 0;
    usb_trans_info = usb_conv_info->usb_trans_info;
    if (!usb_trans_info)
        return 0;

    type = USB_TYPE(usb_trans_info->setup.requesttype);
    recip = USB_RECIPIENT(usb_trans_info->setup.requesttype);

    if (recip == RQT_SETUP_RECIPIENT_INTERFACE) {
        if (type == RQT_SETUP_TYPE_STANDARD) {
            return dissect_usb_hid_control_std_intf(
                    tvb, pinfo, tree, usb_conv_info);
        }
        else if (type == RQT_SETUP_TYPE_CLASS) {
            return dissect_usb_hid_control_class_intf(
                    tvb, pinfo, tree, usb_conv_info);
        }
    }

    return 0;
}

/* dissect a descriptor that is specific to the HID class */
static gint
dissect_usb_hid_class_descriptors(tvbuff_t *tvb, packet_info *pinfo _U_,
        proto_tree *tree, void *data _U_)
{
    guint8      type;
    gint        offset = 0;
    proto_item *ti;
    proto_tree *desc_tree;
    guint8      num_desc;
    guint       i;

    type = tvb_get_guint8(tvb, 1);

    /* for now, we only handle the HID descriptor here */
    if (type != USB_DT_HID)
        return 0;

    desc_tree = proto_tree_add_subtree(tree, tvb, offset, -1, ett_usb_hid_descriptor, &ti, "HID DESCRIPTOR");

    dissect_usb_descriptor_header(desc_tree, tvb, offset,
            &hid_descriptor_type_vals_ext);
    offset += 2;
    proto_tree_add_item(desc_tree, hf_usb_hid_bcdHID,
            tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;
    proto_tree_add_item(desc_tree, hf_usb_hid_bCountryCode,
            tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset++;
    num_desc = tvb_get_guint8(tvb, offset);
    proto_tree_add_item(desc_tree, hf_usb_hid_bNumDescriptors,
            tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset++;
    for (i=0;i<num_desc;i++) {
        proto_tree_add_item(desc_tree, hf_usb_hid_bDescriptorType,
                tvb, offset, 1, ENC_LITTLE_ENDIAN);
        offset++;
        proto_tree_add_item(desc_tree, hf_usb_hid_wDescriptorLength,
                tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;
    }

    proto_item_set_len(ti, offset);
    return offset;
}


void
proto_register_usb_hid(void)
{
    static hf_register_info hf[] = {
        { &hf_usb_hid_item_bSize,
            { "bSize", "usbhid.item.bSize", FT_UINT8, BASE_DEC,
                VALS(usb_hid_item_bSize_vals), USBHID_SIZE_MASK, NULL, HFILL }},

        { &hf_usb_hid_item_bType,
            { "bType", "usbhid.item.bType", FT_UINT8, BASE_DEC,
                VALS(usb_hid_item_bType_vals), USBHID_TYPE_MASK, NULL, HFILL }},

        { &hf_usb_hid_mainitem_bTag,
            { "bTag", "usbhid.item.bTag", FT_UINT8, BASE_HEX,
                VALS(usb_hid_mainitem_bTag_vals), USBHID_TAG_MASK, NULL, HFILL }},

        { &hf_usb_hid_globalitem_bTag,
            { "bTag", "usbhid.item.bTag", FT_UINT8, BASE_HEX,
                VALS(usb_hid_globalitem_bTag_vals), USBHID_TAG_MASK, NULL, HFILL }},

        { &hf_usb_hid_localitem_bTag,
            { "bTag", "usbhid.item.bTag", FT_UINT8, BASE_HEX,
                VALS(usb_hid_localitem_bTag_vals), USBHID_TAG_MASK, NULL, HFILL }},

        { &hf_usb_hid_longitem_bTag,
            { "bTag", "usbhid.item.bTag", FT_UINT8, BASE_HEX,
                VALS(usb_hid_longitem_bTag_vals), USBHID_TAG_MASK, NULL, HFILL }},

        { &hf_usb_hid_item_bDataSize,
            { "bDataSize", "usbhid.item.bDataSize", FT_UINT8, BASE_DEC,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_item_bLongItemTag,
            { "bTag", "usbhid.item.bLongItemTag", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},

        /* Main-report item data */

        { &hf_usb_hid_mainitem_bit0,
            { "Data/constant", "usbhid.item.main.readonly", FT_BOOLEAN, 9,
                TFS(&tfs_mainitem_bit0), 1<<0, NULL, HFILL }},

        { &hf_usb_hid_mainitem_bit1,
            { "Data type", "usbhid.item.main.variable", FT_BOOLEAN, 9,
                TFS(&tfs_mainitem_bit1), 1<<1, NULL, HFILL }},

        { &hf_usb_hid_mainitem_bit2,
            { "Coordinates", "usbhid.item.main.relative", FT_BOOLEAN, 9,
                TFS(&tfs_mainitem_bit2), 1<<2, NULL, HFILL }},

        { &hf_usb_hid_mainitem_bit3,
            { "Min/max wraparound", "usbhid.item.main.wrap", FT_BOOLEAN, 9,
                TFS(&tfs_mainitem_bit3), 1<<3, NULL, HFILL }},

        { &hf_usb_hid_mainitem_bit4,
            { "Physical relationship to data", "usbhid.item.main.nonlinear", FT_BOOLEAN, 9,
                TFS(&tfs_mainitem_bit4), 1<<4, NULL, HFILL }},

        { &hf_usb_hid_mainitem_bit5,
            { "Preferred state", "usbhid.item.main.no_preferred_state", FT_BOOLEAN, 9,
                TFS(&tfs_mainitem_bit5), 1<<5, NULL, HFILL }},

        { &hf_usb_hid_mainitem_bit6,
            { "Has null position", "usbhid.item.main.nullstate", FT_BOOLEAN, 9,
                TFS(&tfs_mainitem_bit6), 1<<6, NULL, HFILL }},

        { &hf_usb_hid_mainitem_bit7,
            { "(Non)-volatile", "usbhid.item.main.volatile", FT_BOOLEAN, 9,
                TFS(&tfs_mainitem_bit7), 1<<7, NULL, HFILL }},

        { &hf_usb_hid_mainitem_bit7_input,
            { "[Reserved]", "usbhid.item.main.volatile", FT_BOOLEAN, 9,
                NULL, 1<<7, NULL, HFILL }},

        { &hf_usb_hid_mainitem_bit8,
            { "Bits or bytes", "usbhid.item.main.buffered_bytes", FT_BOOLEAN, 9,
                TFS(&tfs_mainitem_bit8), 1<<8, NULL, HFILL }},

        { &hf_usb_hid_mainitem_colltype,
            { "Collection type", "usbhid.item.main.colltype", FT_UINT8, BASE_RANGE_STRING|BASE_HEX,
                RVALS(usb_hid_mainitem_colltype_vals), 0, NULL, HFILL }},

        /* Global-report item data */

        { &hf_usb_hid_globalitem_usage,
            { "Usage page", "usbhid.item.global.usage", FT_UINT8, BASE_RANGE_STRING|BASE_HEX,
                RVALS(usb_hid_item_usage_vals), 0, NULL, HFILL }},

        { &hf_usb_hid_globalitem_log_min,
            { "Logical minimum", "usbhid.item.global.log_min", FT_UINT8, BASE_DEC,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_globalitem_log_max,
            { "Logical maximum", "usbhid.item.global.log_max", FT_UINT8, BASE_DEC,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_globalitem_phy_min,
            { "Physical minimum", "usbhid.item.global.phy_min", FT_UINT8, BASE_DEC,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_globalitem_phy_max,
            { "Physical maximum", "usbhid.item.global.phy_max", FT_UINT8, BASE_DEC,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_globalitem_unit_exp,
            { "Unit exponent", "usbhid.item.global.unit_exp", FT_UINT8, BASE_DEC,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_globalitem_unit_sys,
            { "System", "usbhid.item.global.unit.system", FT_UINT32, BASE_HEX,
                VALS(usb_hid_globalitem_unit_exp_vals), 0x0000000F, NULL, HFILL }},

        { &hf_usb_hid_globalitem_unit_len,
            { "Length", "usbhid.item.global.unit.length", FT_UINT32, BASE_HEX,
                VALS(usb_hid_globalitem_unit_exp_vals), 0x000000F0, NULL, HFILL }},

        { &hf_usb_hid_globalitem_unit_mass,
            { "Mass", "usbhid.item.global.unit.mass", FT_UINT32, BASE_HEX,
                VALS(usb_hid_globalitem_unit_exp_vals), 0x00000F00, NULL, HFILL }},

        { &hf_usb_hid_globalitem_unit_time,
            { "Time", "usbhid.item.global.unit.time", FT_UINT32, BASE_HEX,
                VALS(usb_hid_globalitem_unit_exp_vals), 0x0000F000, NULL, HFILL }},

        { &hf_usb_hid_globalitem_unit_temp,
            { "Temperature", "usbhid.item.global.unit.temperature", FT_UINT32, BASE_HEX,
                VALS(usb_hid_globalitem_unit_exp_vals), 0x000F0000, NULL, HFILL }},

        { &hf_usb_hid_globalitem_unit_current,
            { "Current", "usbhid.item.global.unit.current", FT_UINT32, BASE_HEX,
                VALS(usb_hid_globalitem_unit_exp_vals), 0x00F00000, NULL, HFILL }},

        { &hf_usb_hid_globalitem_unit_brightness,
            { "Luminous intensity", "usbhid.item.global.unit.brightness", FT_UINT32, BASE_HEX,
                VALS(usb_hid_globalitem_unit_exp_vals), 0x0F000000, NULL, HFILL }},

        { &hf_usb_hid_globalitem_report_size,
            { "Report size", "usbhid.item.global.report_size", FT_UINT8, BASE_DEC,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_globalitem_report_id,
            { "Report ID", "usbhid.item.global.report_id", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_globalitem_report_count,
            { "Report count", "usbhid.item.global.report_count", FT_UINT8, BASE_DEC,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_globalitem_push,
            { "Push", "usbhid.item.global.push", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_globalitem_pop,
            { "Pop", "usbhid.item.global.pop", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},

        /* Local-report item data */

        { &hf_usb_hid_localitem_usage,
            { "Usage", "usbhid.item.local.usage", FT_UINT8, BASE_RANGE_STRING|BASE_HEX,
                RVALS(usb_hid_item_usage_vals), 0, NULL, HFILL }},

        { &hf_usb_hid_localitem_usage_min,
            { "Usage minimum", "usbhid.item.local.usage_min", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},

#if 0
        { &hf_usb_hid_localitem_usage_max,
            { "Usage maximum", "usbhid.item.local.usage_max", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},
#endif

        { &hf_usb_hid_localitem_desig_index,
            { "Designator index", "usbhid.item.local.desig_index", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_localitem_desig_min,
            { "Designator minimum", "usbhid.item.local.desig_min", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_localitem_desig_max,
            { "Designator maximum", "usbhid.item.local.desig_max", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_localitem_string_index,
            { "String index", "usbhid.item.local.string_index", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_localitem_string_min,
            { "String minimum", "usbhid.item.local.string_min", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_localitem_string_max,
            { "String maximum", "usbhid.item.local.string_max", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_localitem_delimiter,
            { "Delimiter", "usbhid.item.local.delimiter", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},


        { &hf_usb_hid_item_unk_data,
            { "Item data", "usbhid.item.data", FT_BYTES, BASE_NONE,
                NULL, 0, NULL, HFILL }},

        /* USB HID specific requests */
        { &hf_usb_hid_request,
        { "bRequest", "usbhid.setup.bRequest", FT_UINT8, BASE_HEX, VALS(setup_request_names_vals), 0x0,
          NULL, HFILL }},

        { &hf_usb_hid_value,
        { "wValue", "usbhid.setup.wValue", FT_UINT16, BASE_HEX, NULL, 0x0,
          NULL, HFILL }},

        { &hf_usb_hid_index,
        { "wIndex", "usbhid.setup.wIndex", FT_UINT16, BASE_DEC, NULL, 0x0,
          NULL, HFILL }},

        { &hf_usb_hid_length,
        { "wLength", "usbhid.setup.wLength", FT_UINT16, BASE_DEC, NULL, 0x0,
          NULL, HFILL }},

        { &hf_usb_hid_report_type,
        { "ReportType", "usbhid.setup.ReportType", FT_UINT8, BASE_DEC,
          VALS(usb_hid_report_type_vals), 0x0,
          NULL, HFILL }},

        { &hf_usb_hid_report_id,
        { "ReportID", "usbhid.setup.ReportID", FT_UINT8, BASE_DEC, NULL, 0x0,
          NULL, HFILL }},

        { &hf_usb_hid_duration,
        { "Duration", "usbhid.setup.Duration", FT_UINT8, BASE_DEC, NULL, 0x0,
          NULL, HFILL }},

        { &hf_usb_hid_zero,
        { "(zero)", "usbhid.setup.zero", FT_UINT8, BASE_DEC, NULL, 0x0,
          NULL, HFILL }},

        /* components of the HID descriptor */
        { &hf_usb_hid_bcdHID,
        { "bcdHID", "usbhid.descriptor.hid.bcdHID", FT_UINT16, BASE_HEX, NULL, 0x0,
          NULL, HFILL }},

        { &hf_usb_hid_bCountryCode,
        { "bCountryCode", "usbhid.descriptor.hid.bCountryCode", FT_UINT8,
            BASE_HEX, VALS(hid_country_code_vals), 0x0, NULL, HFILL }},

        { &hf_usb_hid_bNumDescriptors,
        { "bNumDescriptors", "usbhid.descriptor.hid.bNumDescriptors", FT_UINT8,
            BASE_DEC, NULL, 0x0, NULL, HFILL }},

        { &hf_usb_hid_bDescriptorIndex,
        { "bDescriptorIndex", "usbhid.descriptor.hid.bDescriptorIndex", FT_UINT8,
            BASE_HEX, NULL, 0x0, NULL, HFILL }},

        { &hf_usb_hid_bDescriptorType,
        { "bDescriptorType", "usbhid.descriptor.hid.bDescriptorType", FT_UINT8,
            BASE_HEX|BASE_EXT_STRING, &hid_descriptor_type_vals_ext,
            0x00, NULL, HFILL }},

        { &hf_usb_hid_wInterfaceNumber,
        { "wInterfaceNumber", "usbhid.descriptor.hid.wInterfaceNumber", FT_UINT16,
            BASE_DEC, NULL, 0x0, NULL, HFILL }},

        { &hf_usb_hid_wDescriptorLength,
        { "wDescriptorLength", "usbhid.descriptor.hid.wDescriptorLength", FT_UINT16,
            BASE_DEC, NULL, 0x0, NULL, HFILL }},

        { &hf_usbhid_boot_report_keyboard_reserved,
            { "Reserved",                        "usbhid.boot_report.keyboard.reserved",
            FT_UINT8, BASE_HEX, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_keyboard_keycode_1,
            { "Keycode 1",                       "usbhid.boot_report.keyboard.keycode_1",
            FT_UINT8, BASE_HEX|BASE_EXT_STRING, &keycode_vals_ext, 0x00,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_keyboard_keycode_2,
            { "Keycode 2",                       "usbhid.boot_report.keyboard.keycode_2",
            FT_UINT8, BASE_HEX|BASE_EXT_STRING, &keycode_vals_ext, 0x00,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_keyboard_keycode_3,
            { "Keycode 3",                       "usbhid.boot_report.keyboard.keycode_3",
            FT_UINT8, BASE_HEX|BASE_EXT_STRING, &keycode_vals_ext, 0x00,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_keyboard_keycode_4,
            { "Keycode 4",                       "usbhid.boot_report.keyboard.keycode_4",
            FT_UINT8, BASE_HEX|BASE_EXT_STRING, &keycode_vals_ext, 0x00,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_keyboard_keycode_5,
            { "Keycode 5",                       "usbhid.boot_report.keyboard.keycode_5",
            FT_UINT8, BASE_HEX|BASE_EXT_STRING, &keycode_vals_ext, 0x00,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_keyboard_keycode_6,
            { "Keycode 6",                       "usbhid.boot_report.keyboard.keycode_6",
            FT_UINT8, BASE_HEX|BASE_EXT_STRING, &keycode_vals_ext, 0x00,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_keyboard_modifier_right_gui,
            { "Modifier: RIGHT GUI",             "usbhid.boot_report.keyboard.modifier.right_gui",
            FT_BOOLEAN, 8, NULL, 0x80,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_keyboard_modifier_right_alt,
            { "Modifier: RIGHT ALT",             "usbhid.boot_report.keyboard.modifier.right_alt",
            FT_BOOLEAN, 8, NULL, 0x40,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_keyboard_modifier_right_shift,
            { "Modifier: RIGHT SHIFT",           "usbhid.boot_report.keyboard.modifier.right_shift",
            FT_BOOLEAN, 8, NULL, 0x20,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_keyboard_modifier_right_ctrl,
            { "Modifier: RIGHT CTRL",            "usbhid.boot_report.keyboard.modifier.right_ctrl",
            FT_BOOLEAN, 8, NULL, 0x10,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_keyboard_modifier_left_gui,
            { "Modifier: LEFT GUI",              "usbhid.boot_report.keyboard.modifier.left_gui",
            FT_BOOLEAN, 8, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_keyboard_modifier_left_alt,
            { "Modifier: LEFT ALT",              "usbhid.boot_report.keyboard.modifier.left_alt",
            FT_BOOLEAN, 8, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_keyboard_modifier_left_shift,
            { "Modifier: LEFT SHIFT",            "usbhid.boot_report.keyboard.modifier.left_shift",
            FT_BOOLEAN, 8, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_keyboard_modifier_left_ctrl,
            { "Modifier: LEFT CTRL",             "usbhid.boot_report.keyboard.modifier.left_ctrl",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_keyboard_leds_constants,
            { "Constants",                       "usbhid.boot_report.keyboard.leds.constants",
            FT_UINT8, BASE_HEX, NULL, 0xE0,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_keyboard_leds_kana,
            { "KANA",                            "usbhid.boot_report.keyboard.leds.kana",
            FT_BOOLEAN, 8, NULL, 0x10,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_keyboard_leds_compose,
            { "COMPOSE",                         "usbhid.boot_report.keyboard.leds.compose",
            FT_BOOLEAN, 8, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_keyboard_leds_scroll_lock,
            { "SCROLL LOCK",                     "usbhid.boot_report.keyboard.leds.scroll_lock",
            FT_BOOLEAN, 8, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_keyboard_leds_caps_lock,
            { "CAPS LOCK",                       "usbhid.boot_report.keyboard.leds.caps_lock",
            FT_BOOLEAN, 8, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_keyboard_leds_num_lock,
            { "NUM LOCK",                        "usbhid.boot_report.keyboard.leds.num_lock",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_mouse_button_8,
            { "Button 8",                        "usbhid.boot_report.mouse.button.8",
            FT_BOOLEAN, 8, NULL, 0x80,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_mouse_button_7,
            { "Button 7",                        "usbhid.boot_report.mouse.button.7",
            FT_BOOLEAN, 8, NULL, 0x40,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_mouse_button_6,
            { "Button 6",                        "usbhid.boot_report.mouse.button.6",
            FT_BOOLEAN, 8, NULL, 0x20,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_mouse_button_5,
            { "Button 5",                        "usbhid.boot_report.mouse.button.5",
            FT_BOOLEAN, 8, NULL, 0x10,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_mouse_button_4,
            { "Button 4",                        "usbhid.boot_report.mouse.button.4",
            FT_BOOLEAN, 8, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_mouse_button_middle,
            { "Button Middle",                   "usbhid.boot_report.mouse.button.middle",
            FT_BOOLEAN, 8, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_mouse_button_right,
            { "Button Right",                    "usbhid.boot_report.mouse.button.right",
            FT_BOOLEAN, 8, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_mouse_button_left,
            { "Button Left",                     "usbhid.boot_report.mouse.button.left",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_mouse_x_displacement,
            { "X Displacement",                  "usbhid.boot_report.mouse.x_displacement",
            FT_INT8, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_mouse_y_displacement,
            { "Y Displacement",                  "usbhid.boot_report.mouse.y_displacement",
            FT_INT8, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_mouse_horizontal_scroll_wheel,
            { "Horizontal Scroll Wheel",         "usbhid.boot_report.mouse.scroll_wheel.horizontal",
            FT_INT8, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_usbhid_boot_report_mouse_vertical_scroll_wheel,
            { "Vertical Scroll Wheel",           "usbhid.boot_report.mouse.scroll_wheel.vertical",
            FT_INT8, BASE_DEC, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_usbhid_data,
            { "Data",                            "usbhid.data",
            FT_NONE, BASE_NONE, NULL, 0x00,
            NULL, HFILL }
        },
    };

    static gint *usb_hid_subtrees[] = {
        &ett_usb_hid_report,
        &ett_usb_hid_item_header,
        &ett_usb_hid_wValue,
        &ett_usb_hid_descriptor
    };

    proto_usb_hid = proto_register_protocol("USB HID", "USBHID", "usbhid");
    proto_register_field_array(proto_usb_hid, hf, array_length(hf));
    proto_register_subtree_array(usb_hid_subtrees, array_length(usb_hid_subtrees));

    /*usb_hid_boot_keyboard_input_report_handle  =*/ register_dissector("usbhid.boot_report.keyboard.input",  dissect_usb_hid_boot_keyboard_input_report,  proto_usb_hid);
    /*usb_hid_boot_keyboard_output_report_handle =*/ register_dissector("usbhid.boot_report.keyboard.output", dissect_usb_hid_boot_keyboard_output_report, proto_usb_hid);
    /*usb_hid_boot_mouse_input_report_handle     =*/ register_dissector("usbhid.boot_report.mouse.input",     dissect_usb_hid_boot_mouse_input_report,     proto_usb_hid);

}

void
proto_reg_handoff_usb_hid(void)
{
    dissector_handle_t usb_hid_control_handle, usb_hid_descr_handle;

    usb_hid_control_handle = create_dissector_handle(
                        dissect_usb_hid_control, proto_usb_hid);
    dissector_add_uint("usb.control", IF_CLASS_HID, usb_hid_control_handle);

    dissector_add_for_decode_as("usb.device", usb_hid_control_handle);

    usb_hid_descr_handle = create_dissector_handle(
                        dissect_usb_hid_class_descriptors, proto_usb_hid);
    dissector_add_uint("usb.descriptor", IF_CLASS_HID, usb_hid_descr_handle);
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
