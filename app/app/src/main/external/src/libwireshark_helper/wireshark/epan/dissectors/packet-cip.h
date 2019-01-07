/* packet-cip.h
 * Routines for CIP (Common Industrial Protocol) dissection
 * CIP Home: www.odva.org
 *
 * Copyright 2004
 * Magnus Hansson <mah@hms.se>
 * Joakim Wiberg <jow@hms.se>
 *
 * Added support for Connection Configuration Object
 *   ryan wamsley * Copyright 2007
 *
 * $Id: packet-cip.h 28954 2009-07-06 14:14:13Z stig $
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* CIP Service Codes */
#define SC_GET_ATT_ALL           0x01
#define SC_SET_ATT_ALL           0x02
#define SC_GET_ATT_LIST          0x03
#define SC_SET_ATT_LIST          0x04
#define SC_RESET                 0x05
#define SC_START                 0x06
#define SC_STOP                  0x07
#define SC_CREATE                0x08
#define SC_DELETE                0x09
#define SC_MULT_SERV_PACK        0x0A
#define SC_APPLY_ATTRIBUTES      0x0D
#define SC_GET_ATT_SINGLE        0x0E
#define SC_SET_ATT_SINGLE        0x10
#define SC_FIND_NEXT_OBJ_INST    0x11
#define SC_RESTOR                0x15
#define SC_SAVE                  0x16
#define SC_NO_OP                 0x17
#define SC_GET_MEMBER            0x18
#define SC_SET_MEMBER            0x19
#define SC_INSERT_MEMBER         0x1A
#define SC_REMOVE_MEMBER         0x1B
#define SC_GROUP_SYNC            0x1C
/* Class specific services */
/* Connection Manager */
#define SC_CM_FWD_CLOSE             0x4E
#define SC_CM_UNCON_SEND            0x52
#define SC_CM_FWD_OPEN              0x54
/* Connection Configuration Object services */
#define SC_CCO_KICK_TIMER            0x4B
#define SC_CCO_OPEN_CONN             0x4C
#define SC_CCO_CLOSE_CONN            0x4D
#define SC_CCO_STOP_CONN             0x4E
#define SC_CCO_CHANGE_START          0x4F
#define SC_CCO_GET_STATUS            0x50
#define SC_CCO_CHANGE_COMPLETE       0x51
#define SC_CCO_AUDIT_CHANGE          0x52

/* CIP Genral status codes */
#define CI_GRC_SUCCESS              0x00
#define CI_GRC_FAILURE              0x01
#define CI_GRC_NO_RESOURCE          0x02
#define CI_GRC_BAD_DATA             0x03
#define CI_GRC_BAD_PATH             0x04
#define CI_GRC_BAD_CLASS_INSTANCE   0x05
#define CI_GRC_PARTIAL_DATA         0x06
#define CI_GRC_CONN_LOST            0x07
#define CI_GRC_BAD_SERVICE          0x08
#define CI_GRC_BAD_ATTR_DATA        0x09
#define CI_GRC_ATTR_LIST_ERROR      0x0A
#define CI_GRC_ALREADY_IN_MODE      0x0B
#define CI_GRC_BAD_OBJ_MODE         0x0C
#define CI_GRC_OBJ_ALREADY_EXISTS   0x0D
#define CI_GRC_ATTR_NOT_SETTABLE    0x0E
#define CI_GRC_PERMISSION_DENIED    0x0F
#define CI_GRC_DEV_IN_WRONG_STATE   0x10
#define CI_GRC_REPLY_DATA_TOO_LARGE 0x11
#define CI_GRC_FRAGMENT_PRIMITIVE   0x12
#define CI_GRC_CONFIG_TOO_SMALL     0x13
#define CI_GRC_UNDEFINED_ATTR       0x14
#define CI_GRC_CONFIG_TOO_BIG       0x15
#define CI_GRC_OBJ_DOES_NOT_EXIST   0x16
#define CI_GRC_NO_FRAGMENTATION     0x17
#define CI_GRC_DATA_NOT_SAVED       0x18
#define CI_GRC_DATA_WRITE_FAILURE   0x19
#define CI_GRC_REQUEST_TOO_LARGE    0x1A
#define CI_GRC_RESPONSE_TOO_LARGE   0x1B
#define CI_GRC_MISSING_LIST_DATA    0x1C
#define CI_GRC_INVALID_LIST_STATUS  0x1D
#define CI_GRC_SERVICE_ERROR        0x1E
#define CI_GRC_CONN_RELATED_FAILURE 0x1F
#define CI_GRC_INVALID_PARAMETER    0x20
#define CI_GRC_WRITE_ONCE_FAILURE   0x21
#define CI_GRC_INVALID_REPLY        0x22
#define CI_GRC_BUFFER_OVERFLOW      0x23
#define CI_GRC_MESSAGE_FORMAT       0x24
#define CI_GRC_BAD_KEY_IN_PATH      0x25
#define CI_GRC_BAD_PATH_SIZE        0x26
#define CI_GRC_UNEXPECTED_ATTR      0x27
#define CI_GRC_INVALID_MEMBER       0x28
#define CI_GRC_MEMBER_NOT_SETTABLE  0x29
#define CI_GRC_G2_SERVER_FAILURE    0x2A
#define CI_GRC_UNKNOWN_MB_ERROR     0x2B

#define CI_GRC_STILL_PROCESSING     0xFF


/* IOI Path types */
#define CI_SEGMENT_TYPE_MASK        0xE0

#define CI_PORT_SEGMENT             0x00
#define CI_LOGICAL_SEGMENT          0x20
#define CI_NETWORK_SEGMENT          0x40
#define CI_SYMBOLIC_SEGMENT         0x60
#define CI_DATA_SEGMENT             0x80

#define CI_LOGICAL_SEG_TYPE_MASK    0x1C
#define CI_LOGICAL_SEG_CLASS_ID     0x00
#define CI_LOGICAL_SEG_INST_ID      0x04
#define CI_LOGICAL_SEG_MBR_ID       0x08
#define CI_LOGICAL_SEG_CON_POINT    0x0C
#define CI_LOGICAL_SEG_ATTR_ID      0x10
#define CI_LOGICAL_SEG_SPECIAL      0x14
#define CI_LOGICAL_SEG_SERV_ID      0x18
#define CI_LOGICAL_SEG_RES_1        0x1C

#define CI_LOGICAL_SEG_FORMAT_MASK  0x03
#define CI_LOGICAL_SEG_8_BIT        0x00
#define CI_LOGICAL_SEG_16_BIT       0x01
#define CI_LOGICAL_SEG_32_BIT       0x02
#define CI_LOGICAL_SEG_RES_2        0x03
#define CI_LOGICAL_SEG_E_KEY        0x00

#define CI_E_KEY_FORMAT_VAL         0x04

#define CI_DATA_SEG_SIMPLE          0x80
#define CI_DATA_SEG_SYMBOL          0x91

#define CI_NETWORK_SEG_TYPE_MASK    0x07
#define CI_NETWORK_SEG_SCHEDULE     0x01
#define CI_NETWORK_SEG_FIXED_TAG    0x02
#define CI_NETWORK_SEG_PROD_INHI    0x03

/* Device Profile:s */
#define DP_GEN_DEV                           0x00
#define DP_AC_DRIVE                          0x02
#define DP_MOTOR_OVERLOAD                    0x03
#define DP_LIMIT_SWITCH                      0x04
#define DP_IND_PROX_SWITCH                   0x05
#define DP_PHOTO_SENSOR                      0x06
#define DP_GENP_DISC_IO                      0x07
#define DP_RESOLVER                          0x09
#define DP_COM_ADAPTER                       0x0C
#define DP_POS_CNT                           0x10
#define DP_DC_DRIVE                          0x13
#define DP_CONTACTOR                         0x15
#define DP_MOTOR_STARTER                     0x16
#define DP_SOFT_START                        0x17
#define DP_HMI                               0x18
#define DP_MASS_FLOW_CNT                     0x1A
#define DP_PNEUM_VALVE                       0x1B
#define DP_VACUUM_PRES_GAUGE                 0x1C

/* Define vendor IDs (ControlNet + DeviceNet + EtherNet/IP) */
#define GENERIC_SC_LIST \
   { SC_GET_ATT_ALL,          "Get Attribute All" }, \
   { SC_SET_ATT_ALL,          "Set Attribute All" }, \
   { SC_GET_ATT_LIST,         "Get Attribute List" }, \
   { SC_SET_ATT_LIST,         "Set Attribute List" }, \
   { SC_RESET,                "Reset" }, \
   { SC_START,                "Start" }, \
   { SC_STOP,                 "Stop" }, \
   { SC_CREATE,               "Create" }, \
   { SC_DELETE,               "Delete" }, \
   { SC_MULT_SERV_PACK,       "Multiple Service Packet" }, \
   { SC_APPLY_ATTRIBUTES,     "Apply Attributes" }, \
   { SC_GET_ATT_SINGLE,       "Get Attribute Single" }, \
   { SC_SET_ATT_SINGLE,       "Set Attribute Single" }, \
   { SC_FIND_NEXT_OBJ_INST,   "Find Next Object Instance" }, \
   { SC_RESTOR,               "Restore" }, \
   { SC_SAVE,                 "Save" }, \
   { SC_NO_OP,                "Nop" }, \
   { SC_GET_MEMBER,           "Get Member" }, \
   { SC_SET_MEMBER,           "Set Member" }, \
   { SC_INSERT_MEMBER,        "Insert Member" }, \
   { SC_REMOVE_MEMBER,        "Remove Member" }, \
   { SC_GROUP_SYNC,           "Group Sync" }, \

/* Define vendor IDs (ControlNet + DeviceNet + EtherNet/IP) */
#define VENDOR_ID_LIST \
   {    0,   "Reserved" }, \
   {    1,   "Rockwell Automation/Allen-Bradley" }, \
   {    2,   "Namco Controls Corp." }, \
   {    3,   "Honeywell Inc." }, \
   {    4,   "Parker Hannifin Corp. (Veriflo Division)" }, \
   {    5,   "Rockwell Automation/Reliance Elec." }, \
   {    6,   "Reserved" }, \
   {    7,   "SMC Corporation" }, \
   {    8,   "Molex Incorporated" }, \
   {    9,   "Western Reserve Controls Corp." }, \
   {   10,   "Advanced Micro Controls Inc. (AMCI)" }, \
   {   11,   "ASCO Pneumatic Controls" }, \
   {   12,   "Banner Engineering Corp." }, \
   {   13,   "Belden Wire & Cable Company" }, \
   {   14,   "Cooper Interconnect" }, \
   {   15,   "Reserved" }, \
   {   16,   "Daniel Woodhead Co. (Woodhead Connectivity)" }, \
   {   17,   "Dearborn Group Inc." }, \
   {   18,   "Reserved" }, \
   {   19,   "Helm Instrument Company" }, \
   {   20,   "Huron Net Works" }, \
   {   21,   "Lumberg, Inc." }, \
   {   22,   "Online Development Inc.(Automation Value)" }, \
   {   23,   "Vorne Industries, Inc." }, \
   {   24,   "ODVA Special Reserve" }, \
   {   25,   "Reserved" }, \
   {   26,   "Festo Corporation" }, \
   {   27,   "Reserved" }, \
   {   28,   "Reserved" }, \
   {   29,   "Reserved" }, \
   {   30,   "Unico, Inc." }, \
   {   31,   "Ross Controls" }, \
   {   32,   "Reserved" }, \
   {   33,   "Reserved" }, \
   {   34,   "Hohner Corp." }, \
   {   35,   "Micro Mo Electronics, Inc." }, \
   {   36,   "MKS Instruments, Inc." }, \
   {   37,   "Yaskawa Electric America formerly Magnetek Drives" }, \
   {   38,   "Reserved" }, \
   {   39,   "AVG Automation (Uticor)" }, \
   {   40,   "Wago Corporation" }, \
   {   41,   "Kinetics (Unit Instruments)" }, \
   {   42,   "IMI Norgren Limited" }, \
   {   43,   "BALLUFF, Inc." }, \
   {   44,   "Yaskawa Electric America, Inc." }, \
   {   45,   "Eurotherm Controls Inc" }, \
   {   46,   "ABB Industrial Systems" }, \
   {   47,   "Omron Corporation" }, \
   {   48,   "TURCk, Inc." }, \
   {   49,   "Grayhill Inc." }, \
   {   50,   "Real Time Automation (C&ID)" }, \
   {   51,   "Reserved" }, \
   {   52,   "Numatics, Inc." }, \
   {   53,   "Lutze, Inc." }, \
   {   54,   "Reserved" }, \
   {   55,   "Reserved" }, \
   {   56,   "Softing GmbH" }, \
   {   57,   "Pepperl + Fuchs" }, \
   {   58,   "Spectrum Controls, Inc." }, \
   {   59,   "D.I.P. Inc. MKS Inst." }, \
   {   60,   "Applied Motion Products, Inc." }, \
   {   61,   "Sencon Inc." }, \
   {   62,   "High Country Tek" }, \
   {   63,   "SWAC Automation Consult GmbH" }, \
   {   64,   "Clippard Instrument Laboratory" }, \
   {   65,   "Reserved" }, \
   {   66,   "Reserved" }, \
   {   67,   "Reserved" }, \
   {   68,   "Eaton Electrical" }, \
   {   69,   "Reserved" }, \
   {   70,   "Reserved" }, \
   {   71,   "Toshiba International Corp." }, \
   {   72,   "Control Technology Incorporated" }, \
   {   73,   "TCS (NZ) Ltd." }, \
   {   74,   "Hitachi, Ltd." }, \
   {   75,   "ABB Robotics Products AB" }, \
   {   76,   "NKE Corporation" }, \
   {   77,   "Rockwell Software, Inc." }, \
   {   78,   "Escort Memory Systems (A Datalogic Group Co.)" }, \
   {   79,   "Reserved" }, \
   {   80,   "Industrial Devices Corporation" }, \
   {   81,   "IXXAT Automation GmbH" }, \
   {   82,   "Mitsubishi Electric Automation, Inc." }, \
   {   83,   "OPTO-22" }, \
   {   84,   "Reserved" }, \
   {   85,   "Reserved" }, \
   {   86,   "Horner Electric" }, \
   {   87,   "Burkert Werke GmbH & Co. KG" }, \
   {   88,   "Reserved" }, \
   {   89,   "Industrial Indexing Systems, Inc." }, \
   {   90,   "HMS Industrial Networks AB" }, \
   {   91,   "Robicon" }, \
   {   92,   "Helix Technology (Granville-Phillips)" }, \
   {   93,   "Arlington Laboratory" }, \
   {   94,   "Advantech Co. Ltd." }, \
   {   95,   "Square D Company" }, \
   {   96,   "Digital Electronics Corp." }, \
   {   97,   "Danfoss" }, \
   {   98,   "Reserved" }, \
   {   99,   "Reserved" }, \
   {  100,   "Bosch Rexroth Corporation, Pneumatics" }, \
   {  101,   "Applied Materials, Inc." }, \
   {  102,   "Showa Electric Wire & Cable Co." }, \
   {  103,   "Pacific Scientific (API Controls Inc.)" }, \
   {  104,   "Sharp Manufacturing Systems Corp." }, \
   {  105,   "Olflex Wire & Cable, Inc." }, \
   {  106,   "Reserved" }, \
   {  107,   "Unitrode" }, \
   {  108,   "Beckhoff Automation GmbH" }, \
   {  109,   "National Instruments" }, \
   {  110,   "Mykrolis Corporations (Millipore)" }, \
   {  111,   "International Motion Controls Corp." }, \
   {  112,   "Reserved" }, \
   {  113,   "SEG Kempen GmbH" }, \
   {  114,   "Reserved" }, \
   {  115,   "Reserved" }, \
   {  116,   "MTS Systems Corp." }, \
   {  117,   "Krones, Inc" }, \
   {  118,   "Reserved" }, \
   {  119,   "EXOR Electronic R & D" }, \
   {  120,   "SIEI S.p.A." }, \
   {  121,   "KUKA Roboter GmbH" }, \
   {  122,   "Reserved" }, \
   {  123,   "SEC (Samsung Electronics Co., Ltd)" }, \
   {  124,   "Binary Electronics Ltd" }, \
   {  125,   "Flexible Machine Controls" }, \
   {  126,   "Reserved" }, \
   {  127,   "ABB Inc. (Entrelec)" }, \
   {  128,   "MAC Valves, Inc." }, \
   {  129,   "Auma Actuators Inc" }, \
   {  130,   "Toyoda Machine Works, Ltd" }, \
   {  131,   "Reserved" }, \
   {  132,   "Reserved" }, \
   {  133,   "Balogh T.A.G., Corporation" }, \
   {  134,   "TR Systemtechnik GmbH" }, \
   {  135,   "UNIPULSE Corporation" }, \
   {  136,   "Reserved" }, \
   {  137,   "Reserved" }, \
   {  138,   "Conxall Corporation Inc." }, \
   {  139,   "Reserved" }, \
   {  140,   "Reserved" }, \
   {  141,   "Kuramo Electric Co., Ltd." }, \
   {  142,   "Creative Micro Designs" }, \
   {  143,   "GE Industrial Systems" }, \
   {  144,   "Leybold Vacuum GmbH" }, \
   {  145,   "Siemens Energy & Automation/Drives" }, \
   {  146,   "Kodensha Ltd" }, \
   {  147,   "Motion Engineering, Inc." }, \
   {  148,   "Honda Engineering Co., Ltd" }, \
   {  149,   "EIM Valve Controls" }, \
   {  150,   "Melec Inc." }, \
   {  151,   "Sony Manufacturing Systems Corporation" }, \
   {  152,   "North American Mfg." }, \
   {  153,   "WATLOW" }, \
   {  154,   "Japan Radio Co., Ltd" }, \
   {  155,   "NADEX Co., Ltd" }, \
   {  156,   "Ametek Automation & Process Technologies" }, \
   {  157,   "Reserved" }, \
   {  158,   "KVASER AB" }, \
   {  159,   "IDEC IZUMI Corporation" }, \
   {  160,   "Mitsubishi Heavy Industries Ltd" }, \
   {  161,   "Mitsubishi Electric Corporation" }, \
   {  162,   "Horiba-STEC Inc." }, \
   {  163,   "esd electronic system design gmbh" }, \
   {  164,   "DAIHEN Corporation" }, \
   {  165,   "Tyco Valves & Controls/Keystone" }, \
   {  166,   "EBARA Corporation" }, \
   {  167,   "Reserved" }, \
   {  168,   "Reserved" }, \
   {  169,   "Hokuyo Electric Co. Ltd" }, \
   {  170,   "Pyramid Solutions, Inc." }, \
   {  171,   "Denso Wave Incorporated" }, \
   {  172,   "HLS Hard-Line Solutions Inc" }, \
   {  173,   "Caterpillar, Inc." }, \
   {  174,   "PDL Electronics Ltd." }, \
   {  175,   "Reserved" }, \
   {  176,   "Red Lion Controls" }, \
   {  177,   "ANELVA Corporation" }, \
   {  178,   "Toyo Denki Seizo KK" }, \
   {  179,   "Sanyo Denki Co., Ltd" }, \
   {  180,   "Advanced Energy Japan K.K. (Aera Japan)" }, \
   {  181,   "Pilz GmbH & Co" }, \
   {  182,   "Marsh Bellofram-Bellofram PCD Division" }, \
   {  183,   "Reserved" }, \
   {  184,   "M-SYSTEM Co. Ltd" }, \
   {  185,   "Nissin Electric Co., Ltd" }, \
   {  186,   "Hitachi Metals Ltd." }, \
   {  187,   "Oriental Motor Company" }, \
   {  188,   "A&D Co., Ltd" }, \
   {  189,   "Phasetronics, Inc." }, \
   {  190,   "Cummins Engine Company" }, \
   {  191,   "Deltron Inc." }, \
   {  192,   "Geneer Corporation" }, \
   {  193,   "Anatol Automation, Inc." }, \
   {  194,   "Reserved" }, \
   {  195,   "Reserved" }, \
   {  196,   "Medar, Inc." }, \
   {  197,   "Comdel Inc." }, \
   {  198,   "Advanced Energy Industries, Inc" }, \
   {  199,   "Reserved" }, \
   {  200,   "DAIDEN Co., Ltd" }, \
   {  201,   "CKD Corporation" }, \
   {  202,   "Toyo Electric Corporation" }, \
   {  203,   "Reserved" }, \
   {  204,   "AuCom Electronics Ltd" }, \
   {  205,   "Shinko Electric Co., Ltd" }, \
   {  206,   "Vector Informatik GmbH" }, \
   {  207,   "Reserved" }, \
   {  208,   "Moog Inc." }, \
   {  209,   "Contemporary Controls" }, \
   {  210,   "Tokyo Sokki Kenkyujo Co., Ltd" }, \
   {  211,   "Schenck-AccuRate, Inc." }, \
   {  212,   "The Oilgear Company" }, \
   {  213,   "Reserved" }, \
   {  214,   "ASM Japan K.K." }, \
   {  215,   "HIRATA Corp." }, \
   {  216,   "SUNX Limited" }, \
   {  217,   "Meidensha Corp." }, \
   {  218,   "NIDEC SANKYO CORPORATION (Sankyo Seiki Mfg. Co., Ltd)" }, \
   {  219,   "KAMRO Corp." }, \
   {  220,   "Nippon System Development Co., Ltd" }, \
   {  221,   "EBARA Technologies Inc." }, \
   {  222,   "Reserved" }, \
   {  223,   "Reserved" }, \
   {  224,   "SG Co., Ltd" }, \
   {  225,   "Vaasa Institute of Technology" }, \
   {  226,   "MKS Instruments (ENI Technology)" }, \
   {  227,   "Tateyama System Laboratory Co., Ltd." }, \
   {  228,   "QLOG Corporation" }, \
   {  229,   "Matric Limited Inc." }, \
   {  230,   "NSD Corporation" }, \
   {  231,   "Reserved" }, \
   {  232,   "Sumitomo Wiring Systems, Ltd" }, \
   {  233,   "Group 3 Technology Ltd" }, \
   {  234,   "CTI Cryogenics" }, \
   {  235,   "POLSYS CORP" }, \
   {  236,   "Ampere Inc." }, \
   {  237,   "Reserved" }, \
   {  238,   "Simplatroll Ltd" }, \
   {  239,   "Reserved" }, \
   {  240,   "Reserved" }, \
   {  241,   "Leading Edge Design" }, \
   {  242,   "Humphrey Products" }, \
   {  243,   "Schneider Automation, Inc." }, \
   {  244,   "Westlock Controls Corp." }, \
   {  245,   "Nihon Weidmuller Co., Ltd" }, \
   {  246,   "Brooks Instrument (Div. of Emerson)" }, \
   {  247,   "Reserved" }, \
   {  248,   " Moeller GmbH" }, \
   {  249,   "Varian Vacuum Products" }, \
   {  250,   "Yokogawa Electric Corporation" }, \
   {  251,   "Electrical Design Daiyu Co., Ltd" }, \
   {  252,   "Omron Software Co., Ltd" }, \
   {  253,   "BOC Edwards" }, \
   {  254,   "Control Technology Corporation" }, \
   {  255,   "Bosch Rexroth" }, \
   {  256,   "Turck" }, \
   {  257,   "Control Techniques PLC" }, \
   {  258,   "Hardy Instruments, Inc." }, \
   {  259,   "LS Industrial Systems" }, \
   {  260,   "E.O.A. Systems Inc." }, \
   {  261,   "Reserved" }, \
   {  262,   "New Cosmos Electric Co., Ltd." }, \
   {  263,   "Sense Eletronica LTDA" }, \
   {  264,   "Xycom, Inc." }, \
   {  265,   "Baldor Electric" }, \
   {  266,   "Reserved" }, \
   {  267,   "Patlite Corporation" }, \
   {  268,   "Reserved" }, \
   {  269,   "Mogami Wire & Cable Corporation" }, \
   {  270,   "Welding Technology Corporation (WTC)" }, \
   {  271,   "Reserved" }, \
   {  272,   "Deutschmann Automation GmbH" }, \
   {  273,   "ICP Panel-Tec Inc." }, \
   {  274,   "Bray Controls USA" }, \
   {  275,   "Reserved" }, \
   {  276,   "Status Technologies" }, \
   {  277,   "Trio Motion Technology Ltd" }, \
   {  278,   "Sherrex Systems Ltd" }, \
   {  279,   "Adept Technology, Inc." }, \
   {  280,   "Spang Power Electronics" }, \
   {  281,   "Reserved" }, \
   {  282,   "Acrosser Technology Co., Ltd" }, \
   {  283,   "Hilscher GmbH" }, \
   {  284,   "IMAX Corporation" }, \
   {  285,   "Electronic Innovation, Inc. (Falter Engineering)" }, \
   {  286,   "Netlogic Inc." }, \
   {  287,   "Bosch Rexroth Corporation, Indramat" }, \
   {  288,   "Reserved" }, \
   {  289,   "Reserved" }, \
   {  290,   "Murata  Machinery Ltd." }, \
   {  291,   "MTT Company Ltd." }, \
   {  292,   "Kanematsu Semiconductor Corp." }, \
   {  293,   "Takebishi Electric Sales Co." }, \
   {  294,   "Tokyo Electron Device Ltd" }, \
   {  295,   "PFU Limited" }, \
   {  296,   "Hakko Automation Co., Ltd." }, \
   {  297,   "Advanet Inc." }, \
   {  298,   "Tokyo Electron Software Technologies Ltd." }, \
   {  299,   "Reserved" }, \
   {  300,   "Shinagawa Electric Wire Co., Ltd." }, \
   {  301,   "Yokogawa M&C Corporation" }, \
   {  302,   "KONAN Electric Co., Ltd." }, \
   {  303,   "Binar Elektronik AB" }, \
   {  304,   "Furukawa Electric Co." }, \
   {  305,   "Cooper Energy Services" }, \
   {  306,   "Schleicher GmbH & Co." }, \
   {  307,   "Hirose Electric Co., Ltd" }, \
   {  308,   "Western Servo Design Inc." }, \
   {  309,   "Prosoft Technology" }, \
   {  310,   "Reserved" }, \
   {  311,   "Towa Shoko Co., Ltd" }, \
   {  312,   "Kyopal Co., Ltd" }, \
   {  313,   "Extron Co." }, \
   {  314,   "Wieland Electric GmbH" }, \
   {  315,   "SEW Eurodrive GmbH" }, \
   {  316,   "Aera Corporation" }, \
   {  317,   "STA Reutlingen" }, \
   {  318,   "Reserved" }, \
   {  319,   "Fuji Electric Co., Ltd." }, \
   {  320,   "Reserved" }, \
   {  321,   "Reserved" }, \
   {  322,   "ifm efector, inc." }, \
   {  323,   "Reserved" }, \
   {  324,   "IDEACOD-Hohner Automation S.A." }, \
   {  325,   "CommScope Inc." }, \
   {  326,   "GE Fanuc Automation North America, Inc." }, \
   {  327,   "Matsushita Electric Industrial Co., Ltd" }, \
   {  328,   "Okaya Electronics Corporation" }, \
   {  329,   "KASHIYAMA Industries, Ltd" }, \
   {  330,   "JVC" }, \
   {  331,   "Interface Corporation" }, \
   {  332,   "Grape Systems Inc." }, \
   {  333,   "Reserved" }, \
   {  344,   "Reserved" }, \
   {  335,   "Toshiba IT & Control Systems Corporation" }, \
   {  336,   "Sanyo Machine Works, Ltd." }, \
   {  337,   "Vansco Electronics Ltd." }, \
   {  338,   "Dart Container Corp." }, \
   {  339,   "Livingston & Co., Inc." }, \
   {  340,   "Alfa Laval LKM as" }, \
   {  341,   "BF ENTRON Ltd. (British Federal)" }, \
   {  342,   "Bekaert Engineering NV" }, \
   {  343,   "Ferran  Scientific Inc." }, \
   {  344,   "KEBA AG" }, \
   {  345,   "Endress + Hauser" }, \
   {  346,   "Reserved" }, \
   {  347,   "ABB ALSTOM Power UK Ltd. (EGT)" }, \
   {  348,   "Berger Lahr GmbH" }, \
   {  349,   "Reserved" }, \
   {  350,   "Federal Signal Corp." }, \
   {  351,   "Kawasaki Robotics (USA), Inc." }, \
   {  352,   "Bently Nevada Corporation" }, \
   {  353,   "Reserved" }, \
   {  354,   "FRABA Posital GmbH" }, \
   {  355,   "Elsag Bailey, Inc." }, \
   {  356,   "Fanuc Robotics America" }, \
   {  357,   "Reserved" }, \
   {  358,   "Surface Combustion, Inc." }, \
   {  359,   "Reserved" }, \
   {  360,   "AILES Electronics Ind. Co., Ltd." }, \
   {  361,   "Wonderware Corporation" }, \
   {  362,   "Particle Measuring Systems, Inc." }, \
   {  363,   "Reserved" }, \
   {  364,   "Reserved" }, \
   {  365,   "BITS Co., Ltd" }, \
   {  366,   "Japan Aviation Electronics Industry Ltd" }, \
   {  367,   "Keyence Corporation" }, \
   {  368,   "Kuroda Precision Industries Ltd." }, \
   {  369,   "Mitsubishi Electric Semiconductor Application" }, \
   {  370,   "Nippon Seisen Cable, Ltd." }, \
   {  371,   "Omron ASO Co., Ltd" }, \
   {  372,   "Seiko Seiki Co., Ltd." }, \
   {  373,   "Sumitomo Heavy Industries, Ltd." }, \
   {  374,   "Tango Computer Service Corporation" }, \
   {  375,   "Technology Service, Inc." }, \
   {  376,   "Toshiba Information Systems (Japan) Corporation" }, \
   {  377,   "TOSHIBA Schneider Inverter Corporation" }, \
   {  378,   "Toyooki Kogyo Co., Ltd." }, \
   {  379,   "XEBEC" }, \
   {  380,   "Madison Cable Corporation" }, \
   {  381,   "Hitati Engineering & Services Co., Ltd" }, \
   {  382,   "TEM-TECH Lab Co., Ltd" }, \
   {  383,   "International Laboratory Corporation" }, \
   {  384,   "Dyadic Systems Co., Ltd." }, \
   {  385,   "SETO Electronics Industry Co., Ltd" }, \
   {  386,   "Tokyo Electron Kyushu Limited" }, \
   {  387,   "KEI System Co., Ltd" }, \
   {  388,   "Reserved" }, \
   {  389,   "Asahi Engineering Co., Ltd" }, \
   {  390,   "Contrex Inc." }, \
   {  391,   "Paradigm Controls Ltd." }, \
   {  392,   "Reserved" }, \
   {  393,   "Ohm Electric Co., Ltd." }, \
   {  394,   "RKC Instrument Inc." }, \
   {  395,   "Suzuki Motor Corporation" }, \
   {  396,   "Custom Servo Motors Inc." }, \
   {  397,   "PACE Control Systems" }, \
   {  398,   "Reserved" }, \
   {  399,   "Reserved" }, \
   {  400,   "LINTEC Co., Ltd." }, \
   {  401,   "Hitachi Cable Ltd." }, \
   {  402,   "BUSWARE Direct" }, \
   {  403,   "Eaton Electric B.V. (former Holec Holland N.V.)" }, \
   {  404,   "VAT Vakuumventile AG" }, \
   {  405,   "Scientific Technologies Incorporated" }, \
   {  406,   "Alfa Instrumentos Eletronicos Ltda" }, \
   {  407,   "TWK Elektronik GmbH" }, \
   {  408,   "ABB Welding Systems AB" }, \
   {  409,   "BYSTRONIC Maschinen AG" }, \
   {  410,   "Kimura Electric Co., Ltd" }, \
   {  411,   "Nissei Plastic Industrial Co., Ltd" }, \
   {  412,   "Reserved" }, \
   {  413,   "Kistler-Morse Corporation" }, \
   {  414,   "Proteous Industries Inc." }, \
   {  415,   "IDC Corporation" }, \
   {  416,   "Nordson Corporation" }, \
   {  417,   "Rapistan Systems" }, \
   {  418,   "LP-Elektronik GmbH" }, \
   {  419,   "GERBI & FASE S.p.A.(Fase Saldatura)" }, \
   {  420,   "Phoenix Digital Corporation" }, \
   {  421,   "Z-World Engineering" }, \
   {  422,   "Honda R&D Co., Ltd." }, \
   {  423,   "Bionics Instrument Co., Ltd." }, \
   {  424,   "Teknic, Inc." }, \
   {  425,   "R.Stahl, Inc." }, \
   {  426,   "Reserved" }, \
   {  427,   "Ryco Graphic Manufacturing Inc." }, \
   {  428,   "Giddings & Lewis, Inc." }, \
   {  429,   "Koganei Corporation" }, \
   {  430,   "Reserved" }, \
   {  431,   "Nichigoh Communication Electric Wire Co., Ltd." }, \
   {  432,   "Reserved" }, \
   {  433,   "Fujikura Ltd." }, \
   {  434,   "AD Link Technology Inc." }, \
   {  435,   "StoneL Corporation" }, \
   {  436,   "Computer Optical Products, Inc." }, \
   {  437,   "CONOS Inc." }, \
   {  438,   "Erhardt + Leimer GmbH" }, \
   {  439,   "UNIQUE Co. Ltd" }, \
   {  440,   "Roboticsware, Inc." }, \
   {  441,   "Nachi Fujikoshi Corporation" }, \
   {  442,   "Hengstler GmbH" }, \
   {  443,   "Reserved" }, \
   {  444,   "SUNNY GIKEN Inc." }, \
   {  445,   "Lenze Drive Systems GmbH" }, \
   {  446,   "CD Systems B.V." }, \
   {  447,   "FMT/Aircraft Gate Support Systems AB" }, \
   {  448,   "Axiomatic Technologies Corp" }, \
   {  449,   "Embedded System Products, Inc." }, \
   {  450,   "Reserved" }, \
   {  451,   "Mencom Corporation" }, \
   {  452,   "Reserved" }, \
   {  453,   "Matsushita Welding Systems Co., Ltd." }, \
   {  454,   "Dengensha Mfg. Co. Ltd." }, \
   {  455,   "Quinn Systems Ltd." }, \
   {  456,   "Tellima Technology Ltd" }, \
   {  457,   "MDT, Software" }, \
   {  458,   "Taiwan Keiso Co., Ltd" }, \
   {  459,   "Pinnacle Systems" }, \
   {  460,   "Ascom Hasler Mailing Sys" }, \
   {  461,   "INSTRUMAR Limited" }, \
   {  462,   "Reserved" }, \
   {  463,   "Navistar International Transportation Corp" }, \
   {  464,   "Huettinger Elektronik GmbH + Co. KG" }, \
   {  465,   "OCM Technology Inc." }, \
   {  466,   "Professional Supply Inc." }, \
   {  467,   "Control Solutions" }, \
   {  468,   "Baumer IVO GmbH & Co. KG" }, \
   {  469,   "Worcester Controls Corporation" }, \
   {  470,   "Pyramid Technical Consultants, Inc." }, \
   {  471,   "Reserved" }, \
   {  472,   "Apollo Fire Detectors Limited" }, \
   {  473,   "Avtron Manufacturing, Inc." }, \
   {  474,   "Reserved" }, \
   {  475,   "Tokyo Keiso Co., Ltd." }, \
   {  476,   "Daishowa Swiki Co., Ltd." }, \
   {  477,   "Kojima Instruments Inc." }, \
   {  478,   "Shimadzu Corporation" }, \
   {  479,   "Tatsuta Electric Wire & Cable Co., Ltd." }, \
   {  480,   "MECS Corporation" }, \
   {  481,   "Tahara Electric" }, \
   {  482,   "Koyo Electronics" }, \
   {  483,   "Clever Devices" }, \
   {  484,   "GCD Hardware & Software GmbH" }, \
   {  485,   "Reserved" }, \
   {  486,   "Miller Electric Mfg Co." }, \
   {  487,   "GEA Tuchenhagen GmbH" }, \
   {  488,   "Riken Keiki Co., LTD" }, \
   {  489,   "Keisokugiken Corporation" }, \
   {  490,   "Fuji Machine Mfg. Co., Ltd" }, \
   {  491,   "Reserved" }, \
   {  492,   "Nidec-Shimpo Corp." }, \
   {  493,   "UTEC Corporation" }, \
   {  494,   "Sanyo Electric Co. Ltd." }, \
   {  495,   "Reserved" }, \
   {  496,   "Reserved" }, \
   {  497,   "Okano Electric Wire Co. Ltd" }, \
   {  498,   "Shimaden Co. Ltd." }, \
   {  499,   "Teddington Controls Ltd" }, \
   {  500,   "Reserved" }, \
   {  501,   "VIPA GmbH" }, \
   {  502,   "Warwick Manufacturing Group" }, \
   {  503,   "Danaher Controls" }, \
   {  504,   "Reserved" }, \
   {  505,   "Reserved" }, \
   {  506,   "American Science & Engineering" }, \
   {  507,   "Accutron Controls International Inc." }, \
   {  508,   "Norcott Technologies Ltd" }, \
   {  509,   "TB Woods, Inc" }, \
   {  510,   "Proportion-Air, Inc." }, \
   {  511,   "SICK Stegmann GmbH" }, \
   {  512,   "Reserved" }, \
   {  513,   "Edwards Signaling" }, \
   {  514,   "Sumitomo Metal Industries, Ltd" }, \
   {  515,   "Cosmo Instruments Co., Ltd." }, \
   {  516,   "Denshosha Co., Ltd." }, \
   {  517,   "Kaijo Corp." }, \
   {  518,   "Michiproducts Co., Ltd." }, \
   {  519,   "Miura Corporation" }, \
   {  520,   "TG Information Network Co., Ltd." }, \
   {  521,   "Fujikin , Inc." }, \
   {  522,   "Estic Corp." }, \
   {  523,   "GS Hydraulic Sales" }, \
   {  524,   "Reserved" }, \
   {  525,   "MTE Limited" }, \
   {  526,   "Hyde Park Electronics, Inc." }, \
   {  527,   "Pfeiffer Vacuum GmbH" }, \
   {  528,   "Cyberlogic Technologies" }, \
   {  529,   "OKUMA Corporation FA Systems Division" }, \
   {  530,   "Reserved" }, \
   {  531,   "Hitachi Kokusai Electric Co., Ltd." }, \
   {  532,   "SHINKO TECHNOS Co., Ltd." }, \
   {  533,   "Itoh Electric Co., Ltd." }, \
   {  534,   "Colorado Flow Tech Inc." }, \
   {  535,   "Love Controls Division/Dwyer Inst." }, \
   {  536,   "Alstom Drives and Controls" }, \
   {  537,   "The Foxboro Company" }, \
   {  538,   "Tescom Corporation" }, \
   {  539,   "Reserved" }, \
   {  540,   "Atlas Copco Controls UK" }, \
   {  541,   "Reserved" }, \
   {  542,   "Autojet Technologies" }, \
   {  543,   "Prima Electronics S.p.A." }, \
   {  544,   "PMA GmbH" }, \
   {  545,   "Shimafuji Electric Co., Ltd" }, \
   {  546,   "Oki Electric Industry Co., Ltd" }, \
   {  547,   "Kyushu Matsushita Electric Co., Ltd" }, \
   {  548,   "Nihon Electric Wire & Cable Co., Ltd" }, \
   {  549,   "Tsuken Electric Ind Co., Ltd" }, \
   {  550,   "Tamadic Co." }, \
   {  551,   "MAATEL SA" }, \
   {  552,   "OKUMA America" }, \
   {  553,   "Control Techniques PLC-NA" }, \
   {  554,   "TPC Wire & Cable" }, \
   {  555,   "ATI Industrial Automation" }, \
   {  556,   "Microcontrol (Australia) Pty Ltd" }, \
   {  557,   "Serra Soldadura, S.A." }, \
   {  558,   "Southwest Research Institute" }, \
   {  559,   "Cabinplant International" }, \
   {  560,   "Sartorius Mechatronics T&H GmbH" }, \
   {  561,   "Comau S.p.A. Robotics & Final Assembly Division" }, \
   {  562,   "Phoenix Contact" }, \
   {  563,   "Yokogawa MAT Corporation" }, \
   {  564,   "asahi sangyo co., ltd." }, \
   {  565,   "Reserved" }, \
   {  566,   "Akita Myotoku Ltd." }, \
   {  567,   "OBARA Corp." }, \
   {  568,   "Suetron Electronic GmbH" }, \
   {  569,   "Reserved" }, \
   {  570,   "Serck Controls Limited" }, \
   {  571,   "Fairchild Industrial Products Company" }, \
   {  572,   "ARO S.A." }, \
   {  573,   "M2C GmbH" }, \
   {  574,   "Shin Caterpillar Mitsubishi Ltd." }, \
   {  575,   "Santest Co., Ltd." }, \
   {  576,   "Cosmotechs Co., Ltd." }, \
   {  577,   "Hitachi Electric Systems" }, \
   {  578,   "Smartscan Ltd" }, \
   {  579,   "Woodhead Software & Electronics France" }, \
   {  580,   "Athena Controls, Inc." }, \
   {  581,   "Syron Engineering & Manufacturing, Inc." }, \
   {  582,   "Asahi Optical Co., Ltd." }, \
   {  583,   "Sansha Electric Mfg. Co., Ltd." }, \
   {  584,   "Nikki Denso Co., Ltd." }, \
   {  585,   "Star Micronics, Co., Ltd." }, \
   {  586,   "Ecotecnia Socirtat Corp." }, \
   {  587,   "AC Technology Corp." }, \
   {  588,   "West Instruments Limited" }, \
   {  589,   "NTI Limited" }, \
   {  590,   "Delta Computer Systems, Inc." }, \
   {  591,   "FANUC Ltd." }, \
   {  592,   "Hearn-Gu Lee" }, \
   {  593,   "ABB Automation Products" }, \
   {  594,   "Orion Machinery Co., Ltd." }, \
   {  595,   "Reserved" }, \
   {  596,   "Wire-Pro, Inc." }, \
   {  597,   "Beijing Huakong Technology Co. Ltd." }, \
   {  598,   "Yokoyama Shokai Co., Ltd." }, \
   {  599,   "Toyogiken Co., Ltd." }, \
   {  600,   "Coester Equipamentos Eletronicos Ltda." }, \
   {  601,   "Reserved" }, \
   {  602,   "Electroplating Engineers of Japan Ltd." }, \
   {  603,   "ROBOX S.p.A." }, \
   {  604,   "Spraying Systems Company" }, \
   {  605,   "Benshaw Inc." }, \
   {  606,   "ZPA-DP A.S." }, \
   {  607,   "Wired Rite Systems" }, \
   {  608,   "Tandis Research, Inc." }, \
   {  609,   "SSD Drives GmbH" }, \
   {  610,   "ULVAC Japan Ltd." }, \
   {  611,   "DYNAX Corporation" }, \
   {  612,   "Nor-Cal Products, Inc." }, \
   {  613,   "Aros Electronics AB" }, \
   {  614,   "Jun-Tech Co., Ltd." }, \
   {  615,   "HAN-MI Co. Ltd." }, \
   {  616,   "uniNtech (formerly SungGi Internet)" }, \
   {  617,   "Hae Pyung Electronics Reserch Institute" }, \
   {  618,   "Milwaukee Electronics" }, \
   {  619,   "OBERG Industries" }, \
   {  620,   "Parker Hannifin/Compumotor Division" }, \
   {  621,   "TECHNO DIGITAL CORPORATION" }, \
   {  622,   "Network Supply Co., Ltd." }, \
   {  623,   "Union Electronics Co., Ltd." }, \
   {  624,   "Tritronics Services PM Ltd." }, \
   {  625,   "Rockwell Automation-Sprecher+Schuh" }, \
   {  626,   "Matsushita Electric Industrial Co., Ltd/Motor Co." }, \
   {  627,   "Rolls-Royce Energy Systems, Inc." }, \
   {  628,   "JEONGIL INTERCOM CO., LTD" }, \
   {  629,   "Interroll Corp." }, \
   {  630,   "Hubbell Wiring Device-Kellems (Delaware)" }, \
   {  631,   "Intelligent Motion Systems" }, \
   {  632,   "Reserved" }, \
   {  633,   "INFICON AG" }, \
   {  634,   "Hirschmann, Inc." }, \
   {  635,   "The Siemon Company" }, \
   {  636,   "YAMAHA Motor Co. Ltd." }, \
   {  637,   "aska corporation" }, \
   {  638,   "Woodhead Connectivity" }, \
   {  639,   "Trimble AB" }, \
   {  640,   "Murrelektronik GmbH" }, \
   {  641,   "Creatrix Labs, Inc." }, \
   {  642,   "TopWorx" }, \
   {  643,   "Kumho Industrial Co., Ltd." }, \
   {  644,   "Wind River Systems, Inc." }, \
   {  645,   "Bihl & Wiedemann GmbH" }, \
   {  646,   "Harmonic Drive Systems Inc." }, \
   {  647,   "Rikei Corporation" }, \
   {  648,   "BL Autotec, Ltd." }, \
   {  649,   "Hana Information & Technology Co., Ltd." }, \
   {  650,   "Seoil Electric Co., Ltd." }, \
   {  651,   "Fife Corporation" }, \
   {  652,   "Shanghai Electrical Apparatus Research Institute" }, \
   {  653,   "Reserved" }, \
   {  654,   "Parasense Development Centre" }, \
   {  655,   "Reserved" }, \
   {  656,   "Reserved" }, \
   {  657,   "Six Tau S.p.A." }, \
   {  658,   "Aucos GmbH" }, \
   {  659,   "Rotork Controls" }, \
   {  660,   "Automationdirect.com" }, \
   {  661,   "Thermo BLH" }, \
   {  662,   "System Controls, Ltd." }, \
   {  663,   "Univer S.p.A." }, \
   {  664,   "MKS-Tenta Technology" }, \
   {  665,   "Lika Electronic SNC" }, \
   {  666,   "Mettler-Toledo, Inc." }, \
   {  667,   "DXL USA Inc." }, \
   {  668,   "Rockwell Automation/Entek IRD Intl." }, \
   {  669,   "Nippon Otis Elevator Company" }, \
   {  670,   "Sinano Electric, Co., Ltd." }, \
   {  671,   "Sony Manufacturing Systems" }, \
   {  672,   "Reserved" }, \
   {  673,   "Contec Co., Ltd." }, \
   {  674,   "Automated Solutions" }, \
   {  675,   "Controlweigh" }, \
   {  676,   "Reserved" }, \
   {  677,   "Fincor Electronics" }, \
   {  678,   "Cognex Corporation" }, \
   {  679,   "Qualiflow" }, \
   {  680,   "Weidmuller, Inc." }, \
   {  681,   "Morinaga Milk Industry Co., Ltd." }, \
   {  682,   "Takagi Industrial Co., Ltd." }, \
   {  683,   "Wittenstein AG" }, \
   {  684,   "Sena Technologies, Inc." }, \
   {  685,   "Reserved" }, \
   {  686,   "APV Products Unna" }, \
   {  687,   "Creator Teknisk Utvedkling AB" }, \
   {  688,   "Reserved" }, \
   {  689,   "Mibu Denki Industrial Co., Ltd." }, \
   {  690,   "Takamastsu Machineer Section" }, \
   {  691,   "Startco Engineering Ltd." }, \
   {  692,   "Reserved" }, \
   {  693,   "Holjeron" }, \
   {  694,   "ALCATEL High Vacuum Technology" }, \
   {  695,   "Taesan LCD Co., Ltd." }, \
   {  696,   "POSCON" }, \
   {  697,   "VMIC" }, \
   {  698,   "Matsushita Electric Works, Ltd." }, \
   {  699,   "IAI Corporation" }, \
   {  700,   "Horst GmbH" }, \
   {  701,   "MicroControl GmbH & Co." }, \
   {  702,   "Leine & Linde AB" }, \
   {  703,   "Reserved" }, \
   {  704,   "EC Elettronica Srl" }, \
   {  705,   "VIT Software HB" }, \
   {  706,   "Bronkhorst High-Tech B.V." }, \
   {  707,   "Optex Co., Ltd." }, \
   {  708,   "Yosio Electronic Co." }, \
   {  709,   "Terasaki Electric Co., Ltd." }, \
   {  710,   "Sodick Co., Ltd." }, \
   {  711,   "MTS Systems Corporation-Automation Division" }, \
   {  712,   "Mesa Systemtechnik" }, \
   {  713,   "SHIN HO SYSTEM Co., Ltd." }, \
   {  714,   "Goyo Electronics Co, Ltd." }, \
   {  715,   "Loreme" }, \
   {  716,   "SAB Brockskes GmbH & Co. KG" }, \
   {  717,   "Trumpf Laser GmbH + Co. KG" }, \
   {  718,   "Niigata Electronic Instruments Co., Ltd." }, \
   {  719,   "Yokogawa Digital Computer Corporation" }, \
   {  720,   "O.N. Electronic Co., Ltd." }, \
   {  721,   "Industrial Control  Communication, Inc." }, \
   {  722,   "ABB, Inc." }, \
   {  723,   "ElectroWave USA, Inc." }, \
   {  724,   "Industrial Network Controls, LLC" }, \
   {  725,   "KDT Systems Co., Ltd." }, \
   {  726,   "SEFA Technology Inc." }, \
   {  727,   "Nippon POP Rivets and Fasteners Ltd." }, \
   {  728,   "Yamato Scale Co., Ltd." }, \
   {  729,   "Zener Electric" }, \
   {  730,   "GSE Scale Systems" }, \
   {  731,   "ISAS (Integrated Switchgear & Sys. Pty Ltd)" }, \
   {  732,   "Beta LaserMike Limited" }, \
   {  733,   "TOEI Electric Co., Ltd." }, \
   {  734,   "Hakko Electronics Co., Ltd" }, \
   {  735,   "Reserved" }, \
   {  736,   "RFID, Inc." }, \
   {  737,   "Adwin Corporation" }, \
   {  738,   "Osaka Vacuum, Ltd." }, \
   {  739,   "A-Kyung Motion, Inc." }, \
   {  740,   "Camozzi S.P. A." }, \
   {  741,   "Crevis Co., LTD" }, \
   {  742,   "Rice Lake Weighing Systems" }, \
   {  743,   "Linux Network Services" }, \
   {  744,   "KEB Antriebstechnik GmbH" }, \
   {  745,   "Hagiwara Electric Co., Ltd." }, \
   {  746,   "Glass Inc. International" }, \
   {  747,   "Reserved" }, \
   {  748,   "DVT Corporation" }, \
   {  749,   "Woodward Governor" }, \
   {  750,   "Mosaic Systems, Inc." }, \
   {  751,   "Laserline GmbH" }, \
   {  752,   "COM-TEC, Inc." }, \
   {  753,   "Weed Instrument" }, \
   {  754,   "Prof-face European Technology Center" }, \
   {  755,   "Fuji Automation Co., Ltd." }, \
   {  756,   "Matsutame Co., Ltd." }, \
   {  757,   "Hitachi Via Mechanics, Ltd." }, \
   {  758,   "Dainippon Screen Mfg. Co. Ltd." }, \
   {  759,   "FLS Automation A/S" }, \
   {  760,   "ABB Stotz Kontakt GmbH" }, \
   {  761,   "Technical Marine Service" }, \
   {  762,   "Advanced Automation Associates, Inc." }, \
   {  763,   "Baumer Ident GmbH" }, \
   {  764,   "Tsubakimoto Chain Co." }, \
   {  765,   "Reserved" }, \
   {  766,   "Furukawa Co., Ltd." }, \
   {  767,   "Active Power" }, \
   {  768,   "CSIRO Mining Automation" }, \
   {  769,   "Matrix Integrated Systems" }, \
   {  770,   "Digitronic Automationsanlagen GmbH" }, \
   {  771,   "SICK STEGMANN Inc." }, \
   {  772,   "TAE-Antriebstechnik GmbH" }, \
   {  773,   "Electronic Solutions" }, \
   {  774,   "Rocon L.L.C." }, \
   {  775,   "Dijitized Communications Inc." }, \
   {  776,   "Asahi Organic Chemicals Industry Co., Ltd." }, \
   {  777,   "Hodensha" }, \
   {  778,   "Harting, Inc. NA" }, \
   {  779,   "Kubler GmbH" }, \
   {  780,   "Yamatake Corporation" }, \
   {  781,   "JEOL" }, \
   {  782,   "Yamatake Industrial Systems Co., Ltd." }, \
   {  783,   "HAEHNE Elektronische Messgerate GmbH" }, \
   {  784,   "Ci Technologies Pty Ltd (for Pelamos Industries)" }, \
   {  785,   "N. SCHLUMBERGER & CIE" }, \
   {  786,   "Teijin Seiki Co., Ltd." }, \
   {  787,   "DAIKIN Industries, Ltd" }, \
   {  788,   "RyuSyo Industrial Co., Ltd." }, \
   {  789,   "SAGINOMIYA SEISAKUSHO, INC." }, \
   {  790,   "Seishin Engineering Co., Ltd." }, \
   {  791,   "Japan Support System Ltd." }, \
   {  792,   "Decsys" }, \
   {  793,   "Metronix Messgerate u. Elektronik GmbH" }, \
   {  794,   "Reserved" }, \
   {  795,   "Vaccon Company, Inc." }, \
   {  796,   "Siemens Energy & Automation, Inc." }, \
   {  797,   "Ten X Technology, Inc." }, \
   {  798,   "Tyco Electronics" }, \
   {  799,   "Delta Power Electronics Center" }, \
   {  800,   "Denker" }, \
   {  801,   "Autonics Corporation" }, \
   {  802,   "JFE Electronic Engineering Pty. Ltd." }, \
   {  803,   "Reserved" }, \
   {  804,   "Electro-Sensors, Inc." }, \
   {  805,   "Digi International, Inc." }, \
   {  806,   "Texas Instruments" }, \
   {  807,   "ADTEC Plasma Technology Co., Ltd" }, \
   {  808,   "SICK AG" }, \
   {  809,   "Ethernet Peripherals, Inc." }, \
   {  810,   "Animatics Corporation" }, \
   {  811,   "Reserved" }, \
   {  812,   "Process Control Corporation" }, \
   {  813,   "SystemV. Inc." }, \
   {  814,   "Danaher Motion SRL" }, \
   {  815,   "SHINKAWA Sensor Technology, Inc." }, \
   {  816,   "Tesch GmbH & Co. KG" }, \
   {  817,   "Reserved" }, \
   {  818,   "Trend Controls Systems Ltd." }, \
   {  819,   "Guangzhou ZHIYUAN Electronic Co., Ltd." }, \
   {  820,   "Mykrolis Corporation" }, \
   {  821,   "Bethlehem Steel Corporation" }, \
   {  822,   "KK ICP" }, \
   {  823,   "Takemoto Denki Corporation" }, \
   {  824,   "The Montalvo Corporation" }, \
   {  825,   "Reserved" }, \
   {  826,   "LEONI Special Cables GmbH" }, \
   {  827,   "Reserved" }, \
   {  828,   "ONO SOKKI CO.,LTD." }, \
   {  829,   "Rockwell Samsung Automation" }, \
   {  830,   "SHINDENGEN ELECTRIC MFG. CO. LTD" }, \
   {  831,   "Origin Electric Co. Ltd." }, \
   {  832,   "Quest Technical Solutions, Inc." }, \
   {  833,   "LS Cable, Ltd." }, \
   {  834,   "Enercon-Nord Electronic GmbH" }, \
   {  835,   "Northwire Inc." }, \
   {  836,   "Engel Elektroantriebe GmbH" }, \
   {  837,   "The Stanley Works" }, \
   {  838,   "Celesco Transducer Products, Inc." }, \
   {  839,   "Chugoku Electric Wire and Cable Co." }, \
   {  840,   "Kongsberg Simrad AS" }, \
   {  841,   "Panduit Corporation" }, \
   {  842,   "Spellman High Voltage Electronics Corp." }, \
   {  843,   "Kokusai Electric Alpha Co., Ltd." }, \
   {  844,   "Brooks Automation, Inc." }, \
   {  845,   "ANYWIRE CORPORATION" }, \
   {  846,   "Honda Electronics Co. Ltd" }, \
   {  847,   "REO Elektronik AG" }, \
   {  848,   "Fusion UV Systems, Inc." }, \
   {  849,   "ASI Advanced Semiconductor Instruments GmbH" }, \
   {  850,   "Datalogic, Inc." }, \
   {  851,   "SoftPLC Corporation" }, \
   {  852,   "Dynisco Instruments LLC" }, \
   {  853,   "WEG Industrias SA" }, \
   {  854,   "Frontline Test Equipment, Inc." }, \
   {  855,   "Tamagawa Seiki Co., Ltd." }, \
   {  856,   "Multi Computing Co., Ltd." }, \
   {  857,   "RVSI" }, \
   {  858,   "Commercial Timesharing Inc." }, \
   {  859,   "Tennessee Rand Automation LLC" }, \
   {  860,   "Wacogiken Co., Ltd" }, \
   {  861,   "Reflex Integration Inc." }, \
   {  862,   "Siemens AG, A&D PI Flow Instruments" }, \
   {  863,   "G. Bachmann Electronic GmbH" }, \
   {  864,   "NT International" }, \
   {  865,   "Schweitzer Engineering Laboratories" }, \
   {  866,   "ATR Industrie-Elektronik GmbH Co." }, \
   {  867,   "PLASMATECH Co., Ltd" }, \
   {  868,   "Reserved" }, \
   {  869,   "GEMU GmbH & Co. KG" }, \
   {  870,   "Alcorn McBride Inc." }, \
   {  871,   "MORI SEIKI CO., LTD" }, \
   {  872,   "NodeTech Systems Ltd" }, \
   {  873,   "Emhart Teknologies" }, \
   {  874,   "Cervis, Inc." }, \
   {  875,   "FieldServer Technologies (Div Sierra Monitor Corp)" }, \
   {  876,   "NEDAP Power Supplies" }, \
   {  877,   "Nippon Sanso Corporation" }, \
   {  878,   "Mitomi Giken Co., Ltd." }, \
   {  879,   "PULS GmbH" }, \
   {  880,   "Reserved" }, \
   {  881,   "Japan Control Engineering Ltd" }, \
   {  882,   "Embedded Systems Korea (Former Zues Emtek Co Ltd.)" }, \
   {  883,   "Automa SRL" }, \
   {  884,   "Harms+Wende GmbH & Co KG" }, \
   {  885,   "SAE-STAHL GmbH" }, \
   {  886,   "Microwave Data Systems" }, \
   {  887,   "Bernecker + Rainer Industrie-Elektronik GmbH" }, \
   {  888,   "Hiprom Technologies" }, \
   {  889,   "Reserved" }, \
   {  890,   "Nitta Corporation" }, \
   {  891,   "Kontron Modular Computers GmbH" }, \
   {  892,   "Marlin Controls" }, \
   {  893,   "ELCIS s.r.l." }, \
   {  894,   "Acromag, Inc." }, \
   {  895,   "Avery Weigh-Tronix" }, \
   {  896,   "Reserved" }, \
   {  897,   "Reserved" }, \
   {  898,   "Reserved" }, \
   {  899,   "Practicon Ltd" }, \
   {  900,   "Schunk GmbH & Co. KG" }, \
   {  901,   "MYNAH Technologies" }, \
   {  902,   "Defontaine Groupe" }, \
   {  903,   "Emerson Process Management Power & Water Solutions" }, \
   {  904,   "F.A. Elec" }, \
   {  905,   "Hottinger Baldwin Messtechnik GmbH" }, \
   {  906,   "Coreco Imaging, Inc." }, \
   {  907,   "London Electronics Ltd." }, \
   {  908,   "HSD SpA" }, \
   {  909,   "Comtrol Corporation" }, \
   {  910,   "TEAM, S.A. (Tecnica Electronica de Automatismo Y Medida)" }, \
   {  911,   "MAN B&W Diesel Ltd. Regulateurs Europa" }, \
   {  912,   "Reserved" }, \
   {  913,   "Reserved" }, \
   {  914,   "Micro Motion, Inc." }, \
   {  915,   "Eckelmann AG" }, \
   {  916,   "Hanyoung Nux" }, \
   {  917,   "Ransburg Industrial Finishing KK" }, \
   {  918,   "Kun Hung Electric Co. Ltd." }, \
   {  919,   "Brimos wegbebakening b.v." }, \
   {  920,   "Nitto Seiki Co., Ltd" }, \
   {  921,   "PPT Vision, Inc." }, \
   {  922,   "Yamazaki Machinery Works" }, \
   {  923,   "SCHMIDT Technology GmbH" }, \
   {  924,   "Parker Hannifin SpA (SBC Division)" }, \
   {  925,   "HIMA Paul Hildebrandt GmbH" }, \
   {  926,   "RivaTek, Inc." }, \
   {  927,   "Misumi Corporation" }, \
   {  928,   "GE Multilin" }, \
   {  929,   "Measurement Computing Corporation" }, \
   {  930,   "Jetter AG" }, \
   {  931,   "Tokyo Electronics Systems Corporation" }, \
   {  932,   "Togami Electric Mfg. Co., Ltd." }, \
   {  933,   "HK Systems" }, \
   {  934,   "CDA Systems Ltd." }, \
   {  935,   "Aerotech Inc." }, \
   {  936,   "JVL Industrie Elektronik A/S" }, \
   {  937,   "NovaTech Process Solutions LLC" }, \
   {  938,   "Reserved" }, \
   {  939,   "Cisco Systems" }, \
   {  940,   "Grid Connect" }, \
   {  941,   "ITW Automotive Finishing" }, \
   {  942,   "HanYang System" }, \
   {  943,   "ABB K.K. Technical Center" }, \
   {  944,   "Taiyo Electric Wire & Cable Co., Ltd." }, \
   {  945,   "Reserved" }, \
   {  946,   "SEREN IPS INC" }, \
   {  947,   "Belden CDT Electronics Division" }, \
   {  948,   "ControlNet International" }, \
   {  949,   "Gefran S.P.A." }, \
   {  950,   "Jokab Safety AB" }, \
   {  951,   "SUMITA OPTICAL GLASS, INC." }, \
   {  952,   "Biffi Italia srl" }, \
   {  953,   "Beck IPC GmbH" }, \
   {  954,   "Copley Controls Corporation" }, \
   {  955,   "Fagor Automation S. Coop." }, \
   {  956,   "DARCOM" }, \
   {  957,   "Frick Controls (div. of York International)" }, \
   {  958,   "SymCom, Inc." }, \
   {  959,   "Infranor" }, \
   {  960,   "Kyosan Cable, Ltd." }, \
   {  961,   "Varian Vacuum Technologies" }, \
   {  962,   "Messung Systems" }, \
   {  963,   "Xantrex Technology, Inc." }, \
   {  964,   "StarThis Inc." }, \
   {  965,   "Chiyoda Co., Ltd." }, \
   {  966,   "Flowserve Corporation" }, \
   {  967,   "Spyder Controls Corp." }, \
   {  968,   "IBA AG" }, \
   {  969,   "SHIMOHIRA ELECTRIC MFG.CO.,LTD" }, \
   {  970,   "Reserved" }, \
   {  971,   "Siemens L&A" }, \
   {  972,   "Micro Innovations AG" }, \
   {  973,   "Switchgear & Instrumentation" }, \
   {  974,   "PRE-TECH CO., LTD." }, \
   {  975,   "National Semiconductor" }, \
   {  976,   "Invensys Process Systems" }, \
   {  977,   "Ametek HDR Power Systems" }, \
   {  978,   "Reserved" }, \
   {  979,   "TETRA-K Corporation" }, \
   {  980,   "C & M Corporation" }, \
   {  981,   "Siempelkamp Maschinen" }, \
   {  982,   "Reserved" }, \
   {  983,   "Daifuku America Corporation" }, \
   {  984,   "Electro-Matic Products Inc." }, \
   {  985,   "BUSSAN MICROELECTRONICS CORP." }, \
   {  986,   "ELAU AG" }, \
   {  987,   "Hetronic USA" }, \
   {  988,   "NIIGATA POWER SYSTEMS Co., Ltd." }, \
   {  989,   "Software Horizons Inc." }, \
   {  990,   "B3 Systems, Inc." }, \
   {  991,   "Moxa Networking Co., Ltd." }, \
   {  992,   "Reserved" }, \
   {  993,   "S4 Integration" }, \
   {  994,   "Elettro Stemi S.R.L." }, \
   {  995,   "AquaSensors" }, \
   {  996,   "Ifak System GmbH" }, \
   {  997,   "SANKEI MANUFACTURING Co.,LTD." }, \
   {  998,   "Emerson Network Power Co., Ltd." }, \
   {  999,   "Fairmount Automation, Inc." }, \
   { 1000,   "Bird Electronic Corporation" }, \
   { 1001,   "Nabtesco Corporation" }, \
   { 1002,   "AGM Electronics, Inc." }, \
   { 1003,   "ARCX Inc." }, \
   { 1004,   "DELTA I/O Co." }, \
   { 1005,   "Chun IL Electric Ind. Co." }, \
   { 1006,   "N-Tron" }, \
   { 1007,   "Nippon Pneumatics/Fludics System CO.,LTD." }, \
   { 1008,   "DDK Ltd." }, \
   { 1009,   "Seiko Epson Corporation" }, \
   { 1010,   "Halstrup-Walcher GmbH" }, \
   { 1011,   "ITT" }, \
   { 1012,   "Ground Fault Systems bv" }, \
   { 1013,   "Scolari Engineering S.p.A." }, \
   { 1014,   "Vialis Traffic bv" }, \
   { 1015,   "Weidmueller Interface GmbH & Co. KG" }, \
   { 1016,   "Shanghai Sibotech Automation Co. Ltd" }, \
   { 1017,   "AEG Power Supply Systems GmbH" }, \
   { 1018,   "Komatsu Electronics Inc." }, \
   { 1019,   "Souriau" }, \
   { 1020,   "Baumuller Chicago Corp." }, \
   { 1021,   "J. Schmalz GmbH" }, \
   { 1022,   "SEN Corporation" }, \
   { 1023,   "Korenix Technology Co. Ltd" }, \
   { 1024,   "Cooper Power Tools" }, \
   { 1025,   "INNOBIS" }, \
   { 1026,   "Shinho System" }, \
   { 1027,   "Xm Services Ltd." }, \
   { 1028,   "KVC Co., Ltd." }, \
   { 1029,   "Sanyu Seiki Co., Ltd." }, \
   { 1030,   "TuxPLC" }, \
   { 1031,   "Northern Network Solutions" }, \
   { 1032,   "Converteam GmbH" }, \
   { 1033,   "Symbol Technologies" }, \
   { 1034,   "S-TEAM Lab" }, \
   { 1035,   "Maguire Products, Inc." }, \
   { 1036,   "AC&T" }, \
   { 1037,   "MITSUBISHI HEAVY INDUSTRIES, LTD. KOBE SHIPYARD & MACHINERY WORKS" }, \
   { 1038,   "Hurletron Inc." }, \
   { 1039,   "Chunichi Denshi Co., Ltd" }, \
   { 1040,   "Cardinal Scale Mfg. Co." }, \
   { 1041,   "BTR NETCOM via RIA Connect, Inc." }, \
   { 1042,   "Base2" }, \
   { 1043,   "ASRC Aerospace" }, \
   { 1044,   "Beijing Stone Automation" }, \
   { 1045,   "Changshu Switchgear Manufacture Ltd." }, \
   { 1046,   "METRONIX Corp." }, \
   { 1047,   "WIT" }, \
   { 1048,   "ORMEC Systems Corp." }, \
   { 1049,   "ASATech (China) Inc." }, \
   { 1050,   "Controlled Systems Limited" }, \
   { 1051,   "Mitsubishi Heavy Ind. Digital System Co., Ltd. (M.H.I.)" }, \
   { 1052,   "Electrogrip" }, \
   { 1053,   "TDS Automation" }, \
   { 1054,   "T&C Power Conversion, Inc." }, \
   { 1055,   "Robostar Co., Ltd" }, \
   { 1056,   "Scancon A/S" }, \
   { 1057,   "Haas Automation, Inc." }, \
   { 1058,   "Eshed Technology" }, \
   { 1059,   "Delta Electronic Inc." }, \
   { 1060,   "Innovasic Semiconductor" }, \
   { 1061,   "SoftDEL Systems Limited" }, \
   { 1062,   "FiberFin, Inc." }, \
   { 1063,   "Nicollet Technologies Corp." }, \
   { 1064,   "B.F. Systems" }, \
   { 1065,   "Empire Wire and Supply LLC" }, \
   { 1066,   "Reserved" }, \
   { 1067,   "Elmo Motion Control LTD" }, \
   { 1068,   "Reserved" }, \
   { 1069,   "Asahi Keiki Co., Ltd." }, \
   { 1070,   "Joy Mining Machinery" }, \
   { 1071,   "MPM Engineering Ltd" }, \
   { 1072,   "Wolke Inks & Printers GmbH" }, \
   { 1073,   "Mitsubishi Electric Engineering Co., Ltd." }, \
   { 1074,   "COMET AG" }, \
   { 1075,   "Real Time Objects & Systems, LLC" }, \
   { 1076,   "MISCO Refractometer" }, \
   { 1077,   "JT Engineering Inc." }, \
   { 1078,   "Automated Packing Systems" }, \
   { 1079,   "Niobrara R&D Corp." }, \
   { 1080,   "Garmin Ltd." }, \
   { 1081,   "Japan Mobile Platform Co., Ltd" }, \
   { 1082,   "Advosol Inc." }, \
   { 1083,   "ABB Global Services Limited" }, \
   { 1084,   "Sciemetric Instruments Inc." }, \
   { 1085,   "Tata Elxsi Ltd." }, \
   { 1086,   "TPC Mechatronics, Co., Ltd." }, \
   { 1087,   "Cooper Bussmann" }, \
   { 1088,   "Trinite Automatisering B.V." }, \
   { 1089,   "Peek Traffic B.V." }, \
   { 1090,   "Acrison, Inc" }, \
   { 1091,   "Applied Robotics, Inc." }, \
   { 1092,   "FireBus Systems, Inc." }, \
   { 1093,   "Beijing Sevenstar Huachuang Electronics" }, \
   { 1094,   "Magnetek" }, \
   { 1095,   "Microscan" }, \
   { 1096,   "Air Water Inc." }, \
   { 1097,   "Sensopart Industriesensorik GmbH" }, \
   { 1098,   "Tiefenbach Control Systems GmbH" }, \
   { 1099,   "INOXPA S.A" }, \
   { 1100,   "Zurich University of Applied Sciences" }, \
   { 1101,   "Ethernet Direct" }, \
   { 1102,   "GSI-Micro-E Systems" }, \
   { 1103,   "S-Net Automation Co., Ltd." }, \
   { 1104,   "Power Electronics S.L." }, \
   { 1105,   "Renesas Technology Corp." }, \
   { 1106,   "NSWCCD-SSES" }, \
   { 1107,   "Porter Engineering Ltd." }, \
   { 1108,   "Meggitt Airdynamics, Inc." }, \
   { 1109,   "Inductive Automation" }, \
   { 1110,   "Neural ID" }, \
   { 1111,   "EEPod LLC" }, \
   { 1112,   "Hitachi Industrial Equipment Systems Co., Ltd." }, \
   { 1113,   "Salem Automation" }, \
   { 1114,   "port GmbH" }, \
   { 1115,   "B & PLUS" }, \
   { 1116,   "Graco Inc." }, \
   { 1117,   "Altera Corporation" }, \
   { 1118,   "Technology Brewing Corporation" },


/*
** Exported variables
*/

extern const value_string cip_devtype_vals[];
extern const value_string cip_vendor_vals[];
