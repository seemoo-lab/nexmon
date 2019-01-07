/* Packet-rdp.c
 * Routines for Remote Desktop Protocol (RDP) packet dissection
 * Copyright 2010, Graeme Lunt
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

/*
 * See: "[MS-RDPBCGR] Remote Desktop Protocol: Basic Connectivity and Graphics Remoting"
 */

#include "config.h"

#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/conversation.h>
#include <epan/asn1.h>
#include <epan/expert.h>
#include <epan/strutil.h>
#include "packet-ssl.h"
#include "packet-t124.h"

#define PNAME  "Remote Desktop Protocol"
#define PSNAME "RDP"
#define PFNAME "rdp"


static guint global_rdp_tcp_port = 3389;
static dissector_handle_t tpkt_handle;

void proto_register_rdp(void);
void proto_reg_handoff_rdp(void);
static void prefs_register_rdp(void);

static int proto_rdp = -1;

static int ett_rdp = -1;

static int ett_negReq_flags = -1;
static int ett_requestedProtocols = -1;

static int ett_negRsp_flags = -1;
static int ett_selectedProtocol = -1;

static int ett_rdp_SendData = -1;

static int ett_rdp_ClientData = -1;
static int ett_rdp_clientCoreData = -1;
static int ett_rdp_clientSecurityData = -1;
static int ett_rdp_clientNetworkData = -1;
static int ett_rdp_clientClusterData = -1;
static int ett_rdp_clientMonitorData = -1;
static int ett_rdp_clientMsgChannelData = -1;
static int ett_rdp_clientMonitorExData = -1;
static int ett_rdp_clientMultiTransportData = -1;
static int ett_rdp_clientUnknownData = -1;
static int ett_rdp_ServerData = -1;
static int ett_rdp_serverCoreData = -1;
static int ett_rdp_serverSecurityData = -1;
static int ett_rdp_serverNetworkData = -1;
static int ett_rdp_serverMsgChannelData = -1;
static int ett_rdp_serverMultiTransportData = -1;
static int ett_rdp_serverUnknownData = -1;
static int ett_rdp_channelIdArray = -1;
static int ett_rdp_securityExchangePDU = -1;
static int ett_rdp_clientInfoPDU = -1;
static int ett_rdp_validClientLicenseData = -1;
static int ett_rdp_shareControlHeader = -1;
static int ett_rdp_pduType = -1;
static int ett_rdp_flags = -1;
static int ett_rdp_compressedType = -1;
static int ett_rdp_mapFlags = -1;
static int ett_rdp_options = -1;
static int ett_rdp_channelDefArray = -1;
static int ett_rdp_channelDef = -1;
static int ett_rdp_channelPDUHeader = -1;
static int ett_rdp_channelFlags = -1;
static int ett_rdp_capabilitySet = -1;

static int ett_rdp_StandardDate = -1;
static int ett_rdp_DaylightDate = -1;
static int ett_rdp_clientTimeZone = -1;

static expert_field ei_rdp_neg_len_invalid = EI_INIT;
static expert_field ei_rdp_not_correlation_info = EI_INIT;

static int hf_rdp_rt_cookie = -1;
static int hf_rdp_neg_type = -1;
static int hf_rdp_negReq_flags = -1;
static int hf_rdp_negReq_flag_restricted_admin_mode_req = -1;
static int hf_rdp_negReq_flag_correlation_info_present = -1;
static int hf_rdp_neg_length = -1;
static int hf_rdp_requestedProtocols = -1;
static int hf_rdp_requestedProtocols_flag_ssl = -1;
static int hf_rdp_requestedProtocols_flag_hybrid = -1;
static int hf_rdp_requestedProtocols_flag_hybrid_ex = -1;
static int hf_rdp_correlationInfo_flags;
static int hf_rdp_correlationId = -1;
static int hf_rdp_correlationInfo_reserved = -1;
static int hf_rdp_negRsp_flags = -1;
static int hf_rdp_negRsp_flag_extended_client_data_supported = -1;
static int hf_rdp_negRsp_flag_dynvc_gfx_protocol_supported = -1;
static int hf_rdp_negRsp_flag_restricted_admin_mode_supported = -1;
static int hf_rdp_selectedProtocol = -1;
static int hf_rdp_selectedProtocol_flag_ssl = -1;
static int hf_rdp_selectedProtocol_flag_hybrid = -1;
static int hf_rdp_selectedProtocol_flag_hybrid_ex = -1;
static int hf_rdp_negFailure_failureCode = -1;

static int hf_rdp_ClientData = -1;
static int hf_rdp_SendData = -1;
static int hf_rdp_clientCoreData = -1;
static int hf_rdp_clientSecurityData = -1;
static int hf_rdp_clientNetworkData = -1;
static int hf_rdp_clientClusterData = -1;
static int hf_rdp_clientMonitorData = -1;
static int hf_rdp_clientMsgChannelData = -1;
static int hf_rdp_clientMonitorExData = -1;
static int hf_rdp_clientMultiTransportData = -1;
static int hf_rdp_clientUnknownData = -1;
static int hf_rdp_ServerData = -1;
static int hf_rdp_serverCoreData = -1;
static int hf_rdp_serverSecurityData = -1;
static int hf_rdp_serverNetworkData = -1;
static int hf_rdp_serverMsgChannelData = -1;
static int hf_rdp_serverMultiTransportData = -1;
static int hf_rdp_serverUnknownData = -1;

static int hf_rdp_securityExchangePDU = -1;
static int hf_rdp_clientInfoPDU = -1;
static int hf_rdp_validClientLicenseData = -1;

static int hf_rdp_headerType = -1;
static int hf_rdp_headerLength = -1;
static int hf_rdp_versionMajor = -1;
static int hf_rdp_versionMinor = -1;
static int hf_rdp_desktopWidth = -1;
static int hf_rdp_desktopHeight = -1;
static int hf_rdp_colorDepth = -1;
static int hf_rdp_SASSequence = -1;
static int hf_rdp_keyboardLayout = -1;
static int hf_rdp_clientBuild = -1;
static int hf_rdp_clientName = -1;
static int hf_rdp_keyboardType = -1;
static int hf_rdp_keyboardSubType = -1;
static int hf_rdp_keyboardFunctionKey = -1;
static int hf_rdp_imeFileName = -1;
static int hf_rdp_postBeta2ColorDepth = -1;
static int hf_rdp_clientProductId = -1;
static int hf_rdp_serialNumber = -1;
static int hf_rdp_highColorDepth = -1;
static int hf_rdp_supportedColorDepths = -1;
static int hf_rdp_earlyCapabilityFlags = -1;
static int hf_rdp_clientDigProductId = -1;
static int hf_rdp_connectionType = -1;
static int hf_rdp_pad1octet = -1;
static int hf_rdp_serverSelectedProtocol = -1;

static int hf_rdp_encryptionMethods = -1;
static int hf_rdp_extEncryptionMethods = -1;
static int hf_rdp_cluster_flags = -1;
static int hf_rdp_redirectedSessionId = -1;
static int hf_rdp_msgChannelFlags = -1;
static int hf_rdp_msgChannelId = -1;
static int hf_rdp_monitorExFlags = -1;
static int hf_rdp_monitorAttributeSize = -1;
static int hf_rdp_monitorCount = -1;
static int hf_rdp_multiTransportFlags = -1;


static int hf_rdp_encryptionMethod = -1;
static int hf_rdp_encryptionLevel  = -1;
static int hf_rdp_serverRandomLen  = -1;
static int hf_rdp_serverCertLen  = -1;
static int hf_rdp_serverRandom = -1;
static int hf_rdp_serverCertificate = -1;
static int hf_rdp_clientRequestedProtocols = -1;
static int hf_rdp_MCSChannelId = -1;
static int hf_rdp_channelCount = -1;
static int hf_rdp_channelIdArray = -1;
static int hf_rdp_Pad = -1;
static int hf_rdp_length = -1;
static int hf_rdp_encryptedClientRandom = -1;
static int hf_rdp_dataSignature = -1;
static int hf_rdp_fipsLength = -1;
static int hf_rdp_fipsVersion = -1;
static int hf_rdp_padlen = -1;
static int hf_rdp_flags = -1;
static int hf_rdp_flagsPkt = -1;
static int hf_rdp_flagsEncrypt = -1;
static int hf_rdp_flagsResetSeqno = -1;
static int hf_rdp_flagsIgnoreSeqno = -1;
static int hf_rdp_flagsLicenseEncrypt = -1;
static int hf_rdp_flagsSecureChecksum = -1;
static int hf_rdp_flagsFlagsHiValid = -1;
static int hf_rdp_flagsHi = -1;
static int hf_rdp_codePage = -1;
static int hf_rdp_optionFlags = -1;
static int hf_rdp_cbDomain = -1;
static int hf_rdp_cbUserName = -1;
static int hf_rdp_cbPassword = -1;
static int hf_rdp_cbAlternateShell = -1;
static int hf_rdp_cbWorkingDir = -1;
static int hf_rdp_cbClientAddress = -1;
static int hf_rdp_cbClientDir = -1;
static int hf_rdp_cbAutoReconnectLen = -1;
static int hf_rdp_domain = -1;
static int hf_rdp_userName = -1;
static int hf_rdp_password = -1;
static int hf_rdp_alternateShell = -1;
static int hf_rdp_workingDir = -1;
static int hf_rdp_clientAddressFamily = -1;
static int hf_rdp_clientAddress = -1;
static int hf_rdp_clientDir = -1;
static int hf_rdp_clientTimeZone = -1;
static int hf_rdp_clientSessionId = -1;
static int hf_rdp_performanceFlags = -1;
static int hf_rdp_autoReconnectCookie = -1;
static int hf_rdp_reserved1 = -1;
static int hf_rdp_reserved2 = -1;
static int hf_rdp_bMsgType = -1;
static int hf_rdp_bVersion = -1;
static int hf_rdp_wMsgSize = -1;
static int hf_rdp_wBlobType = -1;
static int hf_rdp_wBlobLen = -1;
static int hf_rdp_blobData = -1;
static int hf_rdp_shareControlHeader = -1;
static int hf_rdp_totalLength = -1;
static int hf_rdp_pduType = -1;
static int hf_rdp_pduTypeType = -1;
static int hf_rdp_pduTypeVersionLow = -1;
static int hf_rdp_pduTypeVersionHigh = -1;
static int hf_rdp_pduSource = -1;

static int hf_rdp_shareId = -1;
static int hf_rdp_pad1 = -1;
static int hf_rdp_streamId = -1;
static int hf_rdp_uncompressedLength = -1;
static int hf_rdp_pduType2 = -1;
static int hf_rdp_compressedType = -1;
static int hf_rdp_compressedTypeType = -1;
static int hf_rdp_compressedTypeCompressed = -1;
static int hf_rdp_compressedTypeAtFront = -1;
static int hf_rdp_compressedTypeFlushed = -1;
static int hf_rdp_compressedLength = -1;
static int hf_rdp_wErrorCode = -1;
static int hf_rdp_wStateTransition = -1;
static int hf_rdp_numberEntries = -1;
static int hf_rdp_totalNumberEntries = -1;
static int hf_rdp_mapFlags = -1;
static int hf_rdp_fontMapFirst = -1;
static int hf_rdp_fontMapLast = -1;

/* Control */
static int hf_rdp_action = -1;
static int hf_rdp_grantId = -1;
static int hf_rdp_controlId = -1;

/* Synchronize */
static int hf_rdp_messageType = -1;
static int hf_rdp_targetUser = -1;

/* BitmapCache Persistent List */
static int hf_rdp_numEntriesCache0 = -1;
static int hf_rdp_numEntriesCache1 = -1;
static int hf_rdp_numEntriesCache2 = -1;
static int hf_rdp_numEntriesCache3 = -1;
static int hf_rdp_numEntriesCache4 = -1;
static int hf_rdp_totalEntriesCache0 = -1;
static int hf_rdp_totalEntriesCache1 = -1;
static int hf_rdp_totalEntriesCache2 = -1;
static int hf_rdp_totalEntriesCache3 = -1;
static int hf_rdp_totalEntriesCache4 = -1;
static int hf_rdp_bBitMask = -1;
static int hf_rdp_Pad2 = -1;
static int hf_rdp_Pad3 = -1;

/* BitmapCache Persistent List Entry */
/* static int hf_rdp_Key1 = -1; */
/* static int hf_rdp_Key2 = -1; */

/* FontList */
#if 0
static int hf_rdp_numberFonts = -1;
static int hf_rdp_totalNumFonts = -1;
static int hf_rdp_listFlags = -1;
#endif
static int hf_rdp_entrySize = -1;

/* Confirm Active PDU */
static int hf_rdp_originatorId = -1;
static int hf_rdp_lengthSourceDescriptor = -1;
static int hf_rdp_lengthCombinedCapabilities = -1;
static int hf_rdp_sourceDescriptor = -1;
static int hf_rdp_numberCapabilities = -1;
static int hf_rdp_pad2Octets = -1;
static int hf_rdp_capabilitySet = -1;
static int hf_rdp_capabilitySetType = -1;
static int hf_rdp_lengthCapability = -1;
static int hf_rdp_capabilityData = -1;
static int hf_rdp_sessionId = -1;

/* static int hf_rdp_unknownData = -1; */
static int hf_rdp_notYetImplemented = -1;
static int hf_rdp_encrypted = -1;
/* static int hf_rdp_compressed = -1; */

static int hf_rdp_channelDefArray = -1;
static int hf_rdp_channelDef = -1;
static int hf_rdp_name = -1;
static int hf_rdp_options = -1;
static int hf_rdp_optionsInitialized = -1;
static int hf_rdp_optionsEncryptRDP = -1;
static int hf_rdp_optionsEncryptSC = -1;
static int hf_rdp_optionsEncryptCS = -1;
static int hf_rdp_optionsPriHigh = -1;
static int hf_rdp_optionsPriMed = -1;
static int hf_rdp_optionsPriLow = -1;
static int hf_rdp_optionsCompressRDP = -1;
static int hf_rdp_optionsCompress = -1;
static int hf_rdp_optionsShowProtocol= -1;
static int hf_rdp_optionsRemoteControlPersistent = -1;

static int hf_rdp_channelPDUHeader = -1;
static int hf_rdp_channelFlags = -1;
static int hf_rdp_channelFlagFirst = -1;
static int hf_rdp_channelFlagLast = -1;
static int hf_rdp_channelFlagShowProtocol = -1;
static int hf_rdp_channelFlagSuspend = -1;
static int hf_rdp_channelFlagResume = -1;
static int hf_rdp_channelPacketCompressed = -1;
static int hf_rdp_channelPacketAtFront = -1;
static int hf_rdp_channelPacketFlushed = -1;
static int hf_rdp_channelPacketCompressionType = -1;
static int hf_rdp_virtualChannelData = -1;

static int hf_rdp_wYear = -1;
static int hf_rdp_wMonth = -1;
static int hf_rdp_wDayOfWeek = -1;
static int hf_rdp_wDay = -1;
static int hf_rdp_wHour = -1;
static int hf_rdp_wMinute = -1;
static int hf_rdp_wSecond = -1;
static int hf_rdp_wMilliseconds = -1;

static int hf_rdp_Bias = -1;
static int hf_rdp_StandardName = -1;
static int hf_rdp_StandardDate = -1;
static int hf_rdp_StandardBias = -1;
static int hf_rdp_DaylightName = -1;
static int hf_rdp_DaylightDate = -1;
static int hf_rdp_DaylightBias = -1;

static int hf_rdp_unused = -1;

#define TYPE_RDP_NEG_REQ          0x01
#define TYPE_RDP_NEG_RSP          0x02
#define TYPE_RDP_NEG_FAILURE      0x03
#define TYPE_RDP_CORRELATION_INFO 0x06

static const value_string neg_type_vals[] = {
  { TYPE_RDP_NEG_REQ,          "RDP Negotiation Request" },
  { TYPE_RDP_NEG_RSP,          "RDP Negotiation Response" },
  { TYPE_RDP_NEG_FAILURE,      "RDP Negotiation Failure" },
  { TYPE_RDP_CORRELATION_INFO, "RDP Correlation Info" },
  { 0, NULL }
};

#define RESTRICTED_ADMIN_MODE_REQUIRED 0x01
#define CORRELATION_INFO_PRESENT       0x08

static const value_string failure_code_vals[] = {
  { 0x00000001, "TLS required by server" },
  { 0x00000002, "TLS not allowed by server" },
  { 0x00000003, "TLS certificate not on server" },
  { 0x00000004, "Inconsistent flags" },
  { 0x00000005, "Server requires Enhanced RDP Security with CredSSP" },
  { 0x00000006, "Server requires Enhanced RDP Security with TLS and certificate-based client authentication" },
  { 0, NULL }
};

#define CS_CORE                0xC001
#define CS_SECURITY            0xC002
#define CS_NET                 0xC003
#define CS_CLUSTER             0xC004
#define CS_MONITOR             0xC005
#define CS_MCS_MSGCHANNEL      0xC006
#define CS_MONITOR_EX          0xC008
#define CS_MULTITRANSPORT      0xC00A

#define SC_CORE                0x0C01
#define SC_SECURITY            0x0C02
#define SC_NET                 0x0C03
#define SC_MCS_MSGCHANNEL      0x0C04
#define SC_MULTITRANSPORT      0x0C08

#define SEC_EXCHANGE_PKT       0x0001
#define SEC_ENCRYPT            0x0008
#define SEC_RESET_SEQNO        0x0010
#define SEC_IGNORE_SEQNO       0x0020
#define SEC_INFO_PKT           0x0040
#define SEC_LICENSE_PKT        0x0080
#define SEC_LICENSE_ENCRYPT_CS 0x0200
#define SEC_LICENSE_ENCRYPT_SC 0x0200
#define SEC_REDIRECTION_PKT    0x0400
#define SEC_SECURE_CHECKSUM    0x0800
#define SEC_FLAGSHI_VALID      0x8000

#define SEC_PKT_MASK           0x04c1

#define ENCRYPTION_METHOD_NONE    0x00000000
#define ENCRYPTION_METHOD_40BIT   0x00000001
#define ENCRYPTION_METHOD_128BIT  0x00000002
#define ENCRYPTION_METHOD_56BIT   0x00000008
#define ENCRYPTION_METHOD_FIPS    0x00000010

#define ENCRYPTION_LEVEL_NONE               0x00000000
#define ENCRYPTION_LEVEL_LOW                0x00000001
#define ENCRYPTION_LEVEL_CLIENT_COMPATIBLE  0x00000002
#define ENCRYPTION_LEVEL_HIGH               0x00000003
#define ENCRYPTION_LEVEL_FIPS               0x00000004

/* sent by server */
#define LICENSE_REQUEST             0x01
#define PLATFORM_CHALLENGE          0x02
#define NEW_LICENSE                 0x03
#define UPGRADE_LICENSE             0x04
/* sent by client */
#define LICENSE_INFO                0x12
#define NEW_LICENSE_REQUEST         0x13
#define PLATFORM_CHALLENGE_RESPONSE 0x15
/* sent by either */
#define ERROR_ALERT                 0xff

#define ERR_INVALID_SERVER_CERTIFICIATE 0x00000001
#define ERR_NO_LICENSE                  0x00000002
#define ERR_INVALID_MAC                 0x00000003
#define ERR_INVALID_SCOPE               0x00000004
#define ERR_NO_LICENSE_SERVER           0x00000006
#define STATUS_VALID_CLIENT             0x00000007
#define ERR_INVALID_CLIENT              0x00000008
#define ERR_INVALID_PRODUCTID           0x0000000B
#define ERR_INVALID_MESSAGE_LEN         0x0000000C

#define ST_TOTAL_ABORT                  0x00000001
#define ST_NO_TRANSITION                0x00000002
#define ST_RESET_PHASE_TO_START         0x00000003
#define ST_RESEND_LAST_MESSAGE          0x00000004

#define BB_DATA_BLOB                0x0001
#define BB_RANDOM_BLOB              0x0002
#define BB_CERTIFICATE_BLOB         0x0003
#define BB_ERROR_BLOB               0x0004
#define BB_ENCRYPTED_DATA_BLOB      0x0009
#define BB_KEY_EXCHG_ALG_BLOB       0x000D
#define BB_SCOPE_BLOB               0x000E
#define BB_CLIENT_USER_NAME_BLOB    0x000F
#define BB_CLIENT_MACHINE_NAME_BLOB 0x0010

#define PDUTYPE_TYPE_MASK           0x000F
#define PDUTYPE_VERSIONLOW_MASK     0x00F0
#define PDUTYPE_VERSIONHIGH_MASK    0xFF00

#define PDUTYPE_DEMANDACTIVEPDU     0x1
#define PDUTYPE_CONFIRMACTIVEPDU    0x3
#define PDUTYPE_DEACTIVATEALLPDU    0x6
#define PDUTYPE_DATAPDU             0x7
#define PDUTYPE_SERVER_REDIR_PKT    0xA

#define TS_PROTOCOL_VERSION         0x1

#define PDUTYPE2_UPDATE                      0x02
#define PDUTYPE2_CONTROL                     0x14
#define PDUTYPE2_POINTER                     0x1B
#define PDUTYPE2_INPUT                       0x1C
#define PDUTYPE2_SYNCHRONIZE                 0x1F
#define PDUTYPE2_REFRESH_RECT                0x21
#define PDUTYPE2_PLAY_SOUND                  0x22
#define PDUTYPE2_SUPPRESS_OUTPUT             0x23
#define PDUTYPE2_SHUTDOWN_REQUEST            0x24
#define PDUTYPE2_SHUTDOWN_DENIED             0x25
#define PDUTYPE2_SAVE_SESSION_INFO           0x26
#define PDUTYPE2_FONTLIST                    0x27
#define PDUTYPE2_FONTMAP                     0x28
#define PDUTYPE2_SET_KEYBOARD_INDICATORS     0x29
#define PDUTYPE2_BITMAPCACHE_PERSISTENT_LIST 0x2B
#define PDUTYPE2_BITMAPCACHE_ERROR_PDU       0x2C
#define PDUTYPE2_SET_KEYBOARD_IME_STATUS     0x2D
#define PDUTYPE2_OFFSCRCACHE_ERROR_PDU       0x2E
#define PDUTYPE2_SET_ERROR_INFO_PDU          0x2F
#define PDUTYPE2_DRAWNINEGRID_ERROR_PDU      0x30
#define PDUTYPE2_DRAWGDIPLUS_ERROR_PDU       0x31
#define PDUTYPE2_ARC_STATUS_PDU              0x32
#define PDUTYPE2_STATUS_INFO_PDU             0x36
#define PDUTYPE2_MONITOR_LAYOUT_PDU          0x37

#define PACKET_COMPRESSED                    0x20
#define PACKET_AT_FRONT                      0x40
#define PACKET_FLUSHED                       0x80

#define PacketCompressionTypeMask            0x0f
#define PACKET_COMPR_TYPE_8K                 0x0
#define PACKET_COMPR_TYPE_64K                0x1
#define PACKET_COMPR_TYPE_RDP6               0x2
#define PACKET_COMPR_TYPE_RDP61              0x3


#define CHANNEL_FLAG_FIRST                   0x00000001
#define CHANNEL_FLAG_LAST                    0x00000002
#define CHANNEL_FLAG_SHOW_PROTOCOL           0x00000010
#define CHANNEL_FLAG_SUSPEND                 0x00000020
#define CHANNEL_FLAG_RESUME                  0x00000040
#define CHANNEL_PACKET_COMPRESSED            0x00200000
#define CHANNEL_PACKET_AT_FRONT              0x00400000
#define CHANNEL_PACKET_FLUSHED               0x00800000

#define ChannelCompressionTypeMask           0x000f0000
#define CHANNEL_COMPR_TYPE_8K                0x00000000
#define CHANNEL_COMPR_TYPE_64K               0x00010000
#define CHANNEL_COMPR_TYPE_RDP6              0x00020000
#define CHANNEL_COMPR_TYPE_RDP61             0x00030000

#define MapFlagsMask                         0xffff
#define FONTMAP_FIRST                        0x0001
#define FONTMAP_LAST                         0x0002
/* There may well be others */

#define CTRLACTION_REQUEST_CONTROL           0x0001
#define CTRLACTION_GRANTED_CONTROL           0x0002
#define CTRLACTION_DETACH                    0x0003
#define CTRLACTION_COOPERATE                 0x0004

#define CAPSTYPE_GENERAL                     0x0001
#define CAPSTYPE_BITMAP                      0x0002
#define CAPSTYPE_ORDER                       0x0003
#define CAPSTYPE_BITMAPCACHE                 0x0004
#define CAPSTYPE_CONTROL                     0x0005
#define CAPSTYPE_ACTIVATION                  0x0007
#define CAPSTYPE_POINTER                     0x0008
#define CAPSTYPE_SHARE                       0x0009
#define CAPSTYPE_COLORCACHE                  0x000A
#define CAPSTYPE_SOUND                       0x000C
#define CAPSTYPE_INPUT                       0x000D
#define CAPSTYPE_FONT                        0x000E
#define CAPSTYPE_BRUSH                       0x000F
#define CAPSTYPE_GLYPHCACHE                  0x0010
#define CAPSTYPE_OFFSCREENCACHE              0x0011
#define CAPSTYPE_BITMAPCACHE_HOSTSUPPORT     0x0012
#define CAPSTYPE_BITMAPCACHE_REV2            0x0013
#define CAPSTYPE_BITMAPCACHE_VIRTUALCHANNEL  0x0014
#define CAPSTYPE_DRAWNINEGRIDCACHE           0x0015
#define CAPSTYPE_DRAWGDIPLUS                 0x0016
#define CAPSTYPE_RAIL                        0x0017
#define CAPSTYPE_WINDOW                      0x0018
#define CAPSTYPE_COMPDESK                    0x0019
#define CAPSTYPE_MULTIFRAGMENTUPDATE         0x001A
#define CAPSTYPE_LARGE_POINTER               0x001B
#define CAPSTYPE_SURFACE_COMMANDS            0x001C
#define CAPSTYPE_BITMAP_CODECS               0x001D


#define CHANNEL_OPTION_INITIALIZED               0x80000000
#define CHANNEL_OPTION_ENCRYPT_RDP               0x40000000
#define CHANNEL_OPTION_ENCRYPT_SC                0x20000000
#define CHANNEL_OPTION_ENCRYPT_CS                0x10000000
#define CHANNEL_OPTION_PRI_HIGH                  0x08000000
#define CHANNEL_OPTION_PRI_MED                   0x04000000
#define CHANNEL_OPTION_PRI_LOW                   0x02000000
#define CHANNEL_OPTION_COMPRESS_RDP              0x00800000
#define CHANNEL_OPTION_COMPRESS                  0x00400000
#define CHANNEL_OPTION_SHOW_PROTOCOL             0x00200000
#define CHANNEL_OPTION_REMOTE_CONTROL_PERSISTENT 0x00100000

#define MAX_CHANNELS                         31

typedef struct rdp_conv_info_t {
  guint32 staticChannelId;
  guint32 encryptionMethod;
  guint32 encryptionLevel;
  guint32 licenseAgreed;
  guint8  maxChannels;
  value_string channels[MAX_CHANNELS+1]; /* we may need to hold more information later */
} rdp_conv_info_t;

#define RDP_FI_NONE          0x00
#define RDP_FI_OPTIONAL      0x01
#define RDP_FI_STRING        0x02
#define RDP_FI_UNICODE       0x04 /* field is always Unicode (UTF-16) */
#define RDP_FI_ANSI          0x08 /* field is always ANSI (code page) */
#define RDP_FI_NOINCOFFSET   0x10 /* do not increase the offset */
#define RDP_FI_SUBTREE       0x20
#define RDP_FI_INFO_FLAGS    0x40

typedef struct rdp_field_info_t {
  const int *pfield;
  gint32   fixedLength;
  guint32 *variableLength;
  int      offsetOrTree;
  guint32  flags;
  const struct rdp_field_info_t *subfields;
} rdp_field_info_t;

#define FI_FIXEDLEN(_hf_, _len_) { _hf_, _len_, NULL, 0, 0, NULL }
#define FI_FIXEDLEN_ANSI_STRING(_hf_, _len_) { _hf_, _len_, NULL, 0, RDP_FI_STRING|RDP_FI_ANSI, NULL }
#define FI_VALUE(_hf_, _len_, _value_) { _hf_, _len_, &_value_, 0, 0, NULL }
#define FI_VARLEN(_hf, _length_) { _hf_, 0, &_length_, 0, 0, NULL }
#define FI_SUBTREE(_hf_, _len_, _ett_, _sf_) { _hf_, _len_, NULL, _ett_, RDP_FI_SUBTREE, _sf_ }
#define FI_TERMINATOR {NULL, 0, NULL, 0, 0, NULL}

static const value_string rdp_headerType_vals[] = {
  { CS_CORE,           "clientCoreData" },
  { CS_SECURITY,       "clientSecurityData" },
  { CS_NET,            "clientNetworkData" },
  { CS_CLUSTER,        "clientClusterData" },
  { CS_MONITOR,        "clientMonitorData" },
  { CS_MCS_MSGCHANNEL, "clientMsgChannelData" },
  { CS_MONITOR_EX,     "clientMonitorExData" },
  { CS_MULTITRANSPORT, "clientMultiTransportData" },
  { SC_CORE,           "serverCoreData" },
  { SC_SECURITY,       "serverSecurityData" },
  { SC_NET,            "serverNetworkData" },
  { SC_MCS_MSGCHANNEL, "serverMsgChannelData" },
  { SC_MULTITRANSPORT, "serverMultiTransportData" },
  { 0, NULL }
};

static const value_string rdp_colorDepth_vals[] = {
  { 0xCA00, "4 bits-per-pixel (bpp)"},
  { 0xCA01, "8 bits-per-pixel (bpp)"},
  { 0xCA02, "15-bit 555 RGB mask"},
  { 0xCA03, "16-bit 565 RGB mask"},
  { 0xCA04, "24-bit RGB mask"},
  { 0, NULL }
};

static const value_string rdp_highColorDepth_vals[] = {
  { 0x0004, "4 bits-per-pixel (bpp)"},
  { 0x0008, "8 bits-per-pixel (bpp)"},
  { 0x000F, "15-bit 555 RGB mask"},
  { 0x0010, "16-bit 565 RGB mask"},
  { 0x0018, "24-bit RGB mask"},
  { 0, NULL }
};


static const value_string rdp_keyboardType_vals[] = {
  {   1, "IBM PC/XT or compatible (83-key) keyboard" },
  {   2, "Olivetti \"ICO\" (102-key) keyboard" },
  {   3, "IBM PC/AT (84-key) and similar keyboards" },
  {   4, "IBM enhanced (101-key or 102-key) keyboard" },
  {   5, "Noki 1050 and similar keyboards" },
  {   6, "Nokia 9140 and similar keyboards" },
  {   7, "Japanese keyboard" },
  {   0, NULL }
};

static const value_string rdp_connectionType_vals[] = {
  {   1, "Modem (56 Kbps)" },
  {   2, "Low-speed broadband (256 Kbps - 2Mbps)" },
  {   3, "Satellite (2 Mbps - 16Mbps with high latency)" },
  {   4, "High-speed broadband (2 Mbps - 10Mbps)" },
  {   5, "WAN (10 Mbps or higher with high latency)" },
  {   6, "LAN (10 Mbps or higher)" },
  {   7, "Auto Detect" },
  {   0, NULL},
};

static const value_string rdp_requestedProtocols_vals[] = {
  {   0, "Standard RDP Security" },
  {   1, "TLS 1.0" },
  {   2, "Credential Security Support Provider protocol (CredSSP)" },
  {   3, "Credential Security Support Provider protocol (CredSSP)" },
  {   0, NULL},
};

static const value_string rdp_flagsPkt_vals[] = {
  {0,                   "(None)" },
  {SEC_EXCHANGE_PKT,    "Security Exchange PDU" },
  {SEC_INFO_PKT,        "Client Info PDU" },
  {SEC_LICENSE_PKT,     "Licensing PDU" },
  {SEC_REDIRECTION_PKT, "Standard Security Server Redirection PDU"},
  {0, NULL},
};

static const value_string rdp_encryptionMethod_vals[] = {
  { ENCRYPTION_METHOD_NONE,   "None" },
  { ENCRYPTION_METHOD_40BIT,  "40-bit RC4" },
  { ENCRYPTION_METHOD_128BIT, "128-bit RC4" },
  { ENCRYPTION_METHOD_56BIT,  "56-bit RC4" },
  { ENCRYPTION_METHOD_FIPS,   "FIPS140-1 3DES" },
  { 0, NULL},
};

static const value_string rdp_encryptionLevel_vals[] = {
  { ENCRYPTION_LEVEL_NONE,              "None" },
  { ENCRYPTION_LEVEL_LOW,               "Low" },
  { ENCRYPTION_LEVEL_CLIENT_COMPATIBLE, "Client Compatible" },
  { ENCRYPTION_LEVEL_HIGH,              "High" },
  { ENCRYPTION_LEVEL_FIPS,              "FIPS140-1" },
  { 0, NULL},
};

static const value_string rdp_bMsgType_vals[] = {
  { LICENSE_REQUEST,             "License Request" },
  { PLATFORM_CHALLENGE,          "Platform Challenge" },
  { NEW_LICENSE,                 "New License" },
  { UPGRADE_LICENSE,             "Upgrade License" },
  { LICENSE_INFO,                "License Info" },
  { NEW_LICENSE_REQUEST,         "New License Request" },
  { PLATFORM_CHALLENGE_RESPONSE, "Platform Challenge Response" },
  { ERROR_ALERT,                 "Error Alert" },
  { 0, NULL},
};

static const value_string rdp_wErrorCode_vals[] = {
  { ERR_INVALID_SERVER_CERTIFICIATE, "Invalid Server Certificate" },
  { ERR_NO_LICENSE,                  "No License" },
  { ERR_INVALID_MAC,                 "Invalid MAC" },
  { ERR_INVALID_SCOPE,               "Invalid Scope" },
  { ERR_NO_LICENSE_SERVER,           "No License Server" },
  { STATUS_VALID_CLIENT,             "Valid Client" },
  { ERR_INVALID_CLIENT,              "Invalid Client" },
  { ERR_INVALID_PRODUCTID,           "Invalid Product Id" },
  { ERR_INVALID_MESSAGE_LEN,         "Invalid Message Length" },
  { 0, NULL},
};

static const value_string rdp_wStateTransition_vals[] = {
  { ST_TOTAL_ABORT,              "Total Abort" },
  { ST_NO_TRANSITION,            "No Transition" },
  { ST_RESET_PHASE_TO_START,     "Reset Phase to Start" },
  { ST_RESEND_LAST_MESSAGE,      "Resend Last Message" },
  { 0, NULL},
};

static const value_string rdp_wBlobType_vals[] = {
  { BB_DATA_BLOB,                "Data" },
  { BB_RANDOM_BLOB,              "Random" },
  { BB_CERTIFICATE_BLOB,         "Certificate" },
  { BB_ERROR_BLOB,               "Error" },
  { BB_ENCRYPTED_DATA_BLOB,      "Encrypted Data" },
  { BB_KEY_EXCHG_ALG_BLOB,       "Key Exchange Algorithm" },
  { BB_SCOPE_BLOB,               "Scope" },
  { BB_CLIENT_USER_NAME_BLOB,    "Client User Name" },
  { BB_CLIENT_MACHINE_NAME_BLOB, "Client Machine Name" },
  { 0, NULL},
};

static const value_string rdp_pduTypeType_vals[] = {
  { PDUTYPE_DEMANDACTIVEPDU,  "Demand Active PDU" },
  { PDUTYPE_CONFIRMACTIVEPDU, "Confirm Active PDU" },
  { PDUTYPE_DEACTIVATEALLPDU, "Deactivate All PDU" },
  { PDUTYPE_DATAPDU,          "Data PDU" },
  { PDUTYPE_SERVER_REDIR_PKT, "Server Redirection PDU" },
  { 0, NULL},
};

static const value_string rdp_pduType2_vals[] = {
  { PDUTYPE2_UPDATE,                      "Update"},
  { PDUTYPE2_CONTROL,                     "Control"},
  { PDUTYPE2_POINTER,                     "Pointer"},
  { PDUTYPE2_INPUT,                       "Input"},
  { PDUTYPE2_SYNCHRONIZE,                 "Synchronize"},
  { PDUTYPE2_REFRESH_RECT,                "Refresh Rect"},
  { PDUTYPE2_PLAY_SOUND,                  "Play Sound"},
  { PDUTYPE2_SUPPRESS_OUTPUT,             "Suppress Output"},
  { PDUTYPE2_SHUTDOWN_REQUEST,            "Shutdown Request" },
  { PDUTYPE2_SHUTDOWN_DENIED,             "Shutdown Denied" },
  { PDUTYPE2_SAVE_SESSION_INFO,           "Save Session Info" },
  { PDUTYPE2_FONTLIST,                    "FontList" },
  { PDUTYPE2_FONTMAP,                     "FontMap" },
  { PDUTYPE2_SET_KEYBOARD_INDICATORS,     "Set Keyboard Indicators" },
  { PDUTYPE2_BITMAPCACHE_PERSISTENT_LIST, "BitmapCache Persistent List" },
  { PDUTYPE2_BITMAPCACHE_ERROR_PDU,       "BitmapCache Error" },
  { PDUTYPE2_SET_KEYBOARD_IME_STATUS,     "Set Keyboard IME Status" },
  { PDUTYPE2_OFFSCRCACHE_ERROR_PDU,       "OffScrCache Error" },
  { PDUTYPE2_SET_ERROR_INFO_PDU,          "Set Error Info" },
  { PDUTYPE2_DRAWNINEGRID_ERROR_PDU,      "DrawNineGrid Error" },
  { PDUTYPE2_DRAWGDIPLUS_ERROR_PDU,       "DrawGDIPlus Error" },
  { PDUTYPE2_ARC_STATUS_PDU,              "Arc Status" },
  { PDUTYPE2_STATUS_INFO_PDU,             "Status Info" },
  { PDUTYPE2_MONITOR_LAYOUT_PDU,          "Monitor Layout" },
  { 0, NULL},
};

static const value_string rdp_compressionType_vals[] = {
  { PACKET_COMPR_TYPE_8K,     "RDP 4.0 bulk compression" },
  { PACKET_COMPR_TYPE_64K,    "RDP 5.0 bulk compression" },
  { PACKET_COMPR_TYPE_RDP6,   "RDP 6.0 bulk compression" },
  { PACKET_COMPR_TYPE_RDP61,  "RDP 6.1 bulk compression" },
  { 0, NULL},
};

static const value_string rdp_channelCompressionType_vals[] = {
  { CHANNEL_COMPR_TYPE_8K,     "RDP 4.0 bulk compression" },
  { CHANNEL_COMPR_TYPE_64K,    "RDP 5.0 bulk compression" },
  { CHANNEL_COMPR_TYPE_RDP6,   "RDP 6.0 bulk compression" },
  { CHANNEL_COMPR_TYPE_RDP61,  "RDP 6.1 bulk compression" },
  { 0, NULL},
};

static const value_string rdp_action_vals[] = {
  { CTRLACTION_REQUEST_CONTROL, "Request control" },
  { CTRLACTION_GRANTED_CONTROL, "Granted control" },
  { CTRLACTION_DETACH,          "Detach" },
  { CTRLACTION_COOPERATE,       "Cooperate" },
  {0, NULL },
};

static const value_string rdp_capabilityType_vals[] = {
  { CAPSTYPE_GENERAL,                    "General" },
  { CAPSTYPE_BITMAP,                     "Bitmap" },
  { CAPSTYPE_ORDER,                      "Order" },
  { CAPSTYPE_BITMAPCACHE,                "Bitmap Cache" },
  { CAPSTYPE_CONTROL,                    "Control" },
  { CAPSTYPE_ACTIVATION,                 "Activation" },
  { CAPSTYPE_POINTER,                    "Pointer" },
  { CAPSTYPE_SHARE,                      "Share" },
  { CAPSTYPE_COLORCACHE,                 "Color Cache" },
  { CAPSTYPE_SOUND,                      "Sound" },
  { CAPSTYPE_INPUT,                      "Input" },
  { CAPSTYPE_FONT,                       "Font" },
  { CAPSTYPE_BRUSH,                      "Brush" },
  { CAPSTYPE_GLYPHCACHE,                 "Glyph Cache" },
  { CAPSTYPE_OFFSCREENCACHE,             "Off-screen Cache" },
  { CAPSTYPE_BITMAPCACHE_HOSTSUPPORT,    "Bitmap Cache Host Support" },
  { CAPSTYPE_BITMAPCACHE_REV2,           "Bitmap Cache Rev 2" },
  { CAPSTYPE_BITMAPCACHE_VIRTUALCHANNEL, "Virtual Channel"},
  { CAPSTYPE_DRAWNINEGRIDCACHE,          "Draw Nine Grid Cache" },
  { CAPSTYPE_DRAWGDIPLUS,                "Draw GDI Plus" },
  { CAPSTYPE_RAIL,                       "Rail" },
  { CAPSTYPE_WINDOW,                     "Window" },
  { CAPSTYPE_COMPDESK,                   "Comp Desk" },
  { CAPSTYPE_MULTIFRAGMENTUPDATE,        "Multi-Fragment Update" },
  { CAPSTYPE_LARGE_POINTER,              "Large Pointer" },
  { CAPSTYPE_SURFACE_COMMANDS,           "Surface Commands" },
  { CAPSTYPE_BITMAP_CODECS,              "Bitmap Codecs" },
  {0, NULL },
};

static const value_string rdp_wDayOfWeek_vals[] = {
  { 0, "Sunday" },
  { 1, "Monday" },
  { 2, "Tuesday" },
  { 3, "Wednesday" },
  { 4, "Thursday" },
  { 5, "Friday" },
  { 6, "Saturday" },
  {0, NULL },
};

static const value_string rdp_wDay_vals[] = {
  { 1, "First occurrence" },
  { 2, "Second occurrence" },
  { 3, "Third occurrence" },
  { 4, "Fourth occurrence" },
  { 5, "Last occurrence" },
  {0, NULL },
};

static const value_string rdp_wMonth_vals[] = {
  {  1, "January" },
  {  2, "February" },
  {  3, "March" },
  {  4, "April" },
  {  5, "May" },
  {  6, "June" },
  {  7, "July" },
  {  8, "August" },
  {  9, "September" },
  { 10, "October" },
  { 11, "November" },
  { 12, "December" },
  {0, NULL },
};

/*
 * Flags in the flags field of a TS_INFO_PACKET.
 * XXX - define more, and show them underneath that field.
 */
#define INFO_UNICODE  0x00000010

static rdp_conv_info_t *
rdp_get_conversation_data(packet_info *pinfo)
{
  conversation_t  *conversation;
  rdp_conv_info_t *rdp_info;

  conversation = find_or_create_conversation(pinfo);

  rdp_info = (rdp_conv_info_t *)conversation_get_proto_data(conversation, proto_rdp);

  if (rdp_info == NULL) {
    rdp_info = wmem_new0(wmem_file_scope(), rdp_conv_info_t);
    rdp_info->staticChannelId  = -1;
    rdp_info->encryptionMethod = 0;
    rdp_info->encryptionLevel  = 0;
    rdp_info->licenseAgreed    = 0;
    rdp_info->maxChannels      = 0;

    conversation_add_proto_data(conversation, proto_rdp, rdp_info);
  }

  return rdp_info;
}


static int
dissect_rdp_fields(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, const rdp_field_info_t *fields, int totlen)
{
  const rdp_field_info_t *c;
  gint              len;
  int               base_offset = offset;
  guint32           info_flags = 0;
  guint             encoding;

  while (((c = fields++)->pfield) != NULL) {
    if ((c->fixedLength == 0) && (c->variableLength)) {
      len = *(c->variableLength);
    } else {
      len = c->fixedLength;

      if ((c->variableLength) && (c->fixedLength <= 4)) {
        switch (c->fixedLength) {
        case 1:
          *(c->variableLength) = tvb_get_guint8(tvb, offset);
          break;
        case 2:
          *(c->variableLength) = tvb_get_letohs(tvb, offset);
          break;
        case 4:
          *(c->variableLength) = tvb_get_letohl(tvb, offset);
          break;
        default:
          REPORT_DISSECTOR_BUG("Invalid length");
        }

        *(c->variableLength) += c->offsetOrTree; /* XXX: ??? */
      }
    }

    if (len) {
      proto_item *pi;
      if (c->flags & RDP_FI_STRING) {
        /* If this is always Unicode, or if the INFO_UNICODE flag is set,
           treat this as UTF-16; otherwise, treat it as "ANSI". */
        if (c->flags & RDP_FI_UNICODE)
          encoding = ENC_UTF_16|ENC_LITTLE_ENDIAN;
        else if (c->flags & RDP_FI_ANSI)
          encoding = ENC_ASCII|ENC_NA;  /* XXX - code page */
        else {
          /* Could be Unicode, could be ANSI, based on INFO_UNICODE flag */
          encoding = (info_flags & INFO_UNICODE) ? ENC_UTF_16|ENC_LITTLE_ENDIAN : ENC_ASCII|ENC_NA;  /* XXX - code page */
        }
      } else
        encoding = ENC_LITTLE_ENDIAN;

      pi = proto_tree_add_item(tree, *c->pfield, tvb, offset, len, encoding);

      if (c->flags & RDP_FI_INFO_FLAGS) {
        /* TS_INFO_PACKET flags field; save it for later use */
        DISSECTOR_ASSERT(len == 4);
        info_flags = tvb_get_letohl(tvb, offset);
      }

      if (c->flags & RDP_FI_SUBTREE) {
        proto_tree *next_tree;
        if (c->offsetOrTree != -1)
          next_tree = proto_item_add_subtree(pi, c->offsetOrTree);
        else
          REPORT_DISSECTOR_BUG("Tree Error!!");

        if (c->subfields)
          dissect_rdp_fields(tvb, offset, pinfo, next_tree, c->subfields, 0);
      }

      if (!(c->flags & RDP_FI_NOINCOFFSET))
        offset += len;
    }

    if ((totlen > 0) && ((offset-base_offset) >= totlen))
      break;  /* we're done: skip optional fields */
              /* XXX: err if > totlen ??          */
  }

  return offset;
}

static int
dissect_rdp_nyi(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, const char *info)
{
  rdp_field_info_t nyi_fields[] = {
    {&hf_rdp_notYetImplemented,      -1, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };

  offset = dissect_rdp_fields(tvb, offset, pinfo, tree, nyi_fields, 0);

  if ((tree != NULL) && (info != NULL))
    proto_item_append_text(tree->last_child, " (%s)", info);

  return offset;
}

static int
dissect_rdp_encrypted(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, const char *info)
{
  rdp_field_info_t enc_fields[] = {
    {&hf_rdp_encrypted,      -1, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };

  offset = dissect_rdp_fields(tvb, offset,pinfo, tree, enc_fields, 0);

  if ((tree != NULL) && (info != NULL))
    proto_item_append_text(tree->last_child, " (%s)", info);

  col_append_sep_str(pinfo->cinfo, COL_INFO, ", ", "[Encrypted]");

  return offset;
}


static int
dissect_rdp_clientNetworkData(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint length, rdp_conv_info_t *rdp_info)
{
  proto_tree *next_tree;
  proto_item *pi;
  guint32     channelCount = 0;

  rdp_field_info_t net_fields[] = {
    {&hf_rdp_headerType,             2, NULL, 0, 0, NULL },
    {&hf_rdp_headerLength,           2, NULL, 0, 0, NULL },
    FI_VALUE(&hf_rdp_channelCount, 4, channelCount),
    FI_TERMINATOR
  };
  rdp_field_info_t option_fields[] = {
    {&hf_rdp_optionsInitialized,  4, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_optionsEncryptRDP,   4, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_optionsEncryptSC,    4, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_optionsEncryptCS,    4, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_optionsPriHigh,      4, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_optionsPriMed,       4, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_optionsPriLow,       4, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_optionsCompressRDP,  4, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_optionsCompress,     4, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_optionsShowProtocol, 4, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_optionsRemoteControlPersistent, 4, NULL, 0, 0, NULL },
    FI_TERMINATOR,
  };
  rdp_field_info_t channel_fields[] = {
    FI_FIXEDLEN_ANSI_STRING(&hf_rdp_name, 8),
    FI_SUBTREE(&hf_rdp_options, 4, ett_rdp_options, option_fields),
    FI_TERMINATOR
  };
  rdp_field_info_t def_fields[] = {
    FI_SUBTREE(&hf_rdp_channelDef, 12, ett_rdp_channelDef, channel_fields),
    FI_TERMINATOR
  };

  pi        = proto_tree_add_item(tree, hf_rdp_clientNetworkData, tvb, offset, length, ENC_NA);
  next_tree = proto_item_add_subtree(pi, ett_rdp_clientNetworkData);

  offset = dissect_rdp_fields(tvb, offset, pinfo, next_tree, net_fields, 0);

  if (channelCount > 0) {
    guint i;
    pi        = proto_tree_add_item(next_tree, hf_rdp_channelDefArray, tvb, offset, channelCount * 12, ENC_NA);
    next_tree = proto_item_add_subtree(pi, ett_rdp_channelDefArray);

    if (rdp_info)
      rdp_info->maxChannels = MIN(channelCount, MAX_CHANNELS);

    for (i = 0; i < MIN(channelCount, MAX_CHANNELS); i++) {
      if (rdp_info) {
        rdp_info->channels[i].value = -1; /* unset */
        rdp_info->channels[i].strptr = tvb_get_string_enc(wmem_packet_scope(), tvb, offset, 8, ENC_ASCII);
      }
      offset = dissect_rdp_fields(tvb, offset, pinfo, next_tree, def_fields, 0);
    }

    if (rdp_info) {
      /* value_strings are normally terminated with a {0, NULL} entry */
      rdp_info->channels[i].value  = 0;
      rdp_info->channels[i].strptr = NULL;
    }
  }

  return offset;
}

static int
dissect_rdp_basicSecurityHeader(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint32 *flags_ptr) {

  guint32 flags = 0;

  rdp_field_info_t secFlags_fields[] = {
    {&hf_rdp_flagsPkt,           2, &flags, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_flagsEncrypt,       2, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_flagsResetSeqno,    2, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_flagsIgnoreSeqno,   2, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_flagsLicenseEncrypt,2, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_flagsSecureChecksum,2, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_flagsFlagsHiValid,  2, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    FI_TERMINATOR
  };

  rdp_field_info_t flags_fields[] = {
    FI_SUBTREE(&hf_rdp_flags, 2, ett_rdp_flags, secFlags_fields),
    FI_FIXEDLEN(&hf_rdp_flagsHi, 2),
    FI_TERMINATOR
  };

  offset = dissect_rdp_fields(tvb, offset, pinfo, tree, flags_fields, 0);

  if (flags_ptr)
    *flags_ptr = flags;

  return offset;
}


static int
dissect_rdp_securityHeader(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, rdp_conv_info_t *rdp_info, gboolean alwaysBasic, guint32 *flags_ptr) {

  rdp_field_info_t fips_fields[] = {
    {&hf_rdp_fipsLength,        2, NULL, 0, 0, NULL },
    {&hf_rdp_fipsVersion,       1, NULL, 0, 0, NULL },
    {&hf_rdp_padlen,            1, NULL, 0, 0, NULL },
    {&hf_rdp_dataSignature,     8, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t enc_fields[] = {
    {&hf_rdp_dataSignature,     8, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };
  const rdp_field_info_t *fields = NULL;

  if (rdp_info) {

    if (alwaysBasic || (rdp_info->encryptionLevel != ENCRYPTION_LEVEL_NONE))
      offset = dissect_rdp_basicSecurityHeader(tvb, offset, pinfo, tree, flags_ptr);

    if (rdp_info->encryptionMethod &
       (ENCRYPTION_METHOD_40BIT  |
        ENCRYPTION_METHOD_128BIT |
        ENCRYPTION_METHOD_56BIT)) {
      fields = enc_fields;
    } else if (rdp_info->encryptionMethod == ENCRYPTION_METHOD_FIPS) {
      fields = fips_fields;
    }

    if (fields)
      offset = dissect_rdp_fields(tvb, offset, pinfo, tree, fields, 0);
  }
  return offset;
}
static int
dissect_rdp_channelPDU(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {

  guint32 length = 0;

  rdp_field_info_t flag_fields[] = {
    {&hf_rdp_channelFlagFirst,        4, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_channelFlagLast,         4, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_channelFlagShowProtocol, 4, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_channelFlagSuspend,      4, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_channelFlagResume,       4, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_channelPacketCompressed, 4, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_channelPacketAtFront,    4, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_channelPacketFlushed,    4, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_channelPacketCompressionType,  4, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };

  rdp_field_info_t channel_fields[] =   {
    FI_VALUE(&hf_rdp_length, 4, length),
    FI_SUBTREE(&hf_rdp_channelFlags, 4, ett_rdp_channelFlags, flag_fields),
    FI_TERMINATOR
  };

  rdp_field_info_t channelPDU_fields[] =   {
    FI_SUBTREE(&hf_rdp_channelPDUHeader, 8, ett_rdp_channelPDUHeader, channel_fields),
    FI_FIXEDLEN(&hf_rdp_virtualChannelData, -1),
    FI_TERMINATOR
  };

  /* length is the uncompressed length, and the PDU may be compressed */
  offset = dissect_rdp_fields(tvb, offset, pinfo, tree, channelPDU_fields, 0);

  return offset;
}

static int
dissect_rdp_shareDataHeader(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  guint32 pduType2 = 0;
  guint32 compressedType;
  guint32 action = 0;

  rdp_field_info_t compressed_fields[] =   {
    {&hf_rdp_compressedTypeType, 1, &compressedType, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_compressedTypeCompressed, 1, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_compressedTypeAtFront,    1, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_compressedTypeFlushed,    1, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t share_fields[] =   {
    {&hf_rdp_shareId,            4, NULL, 0, 0, NULL },
    {&hf_rdp_pad1,               1, NULL, 0, 0, NULL },
    {&hf_rdp_streamId,           1, NULL, 0, 0, NULL },
    {&hf_rdp_uncompressedLength, 2, NULL, 0, 0, NULL },
    {&hf_rdp_pduType2,           1, &pduType2, 0, 0, NULL },
    FI_SUBTREE(&hf_rdp_compressedType, 1, ett_rdp_compressedType, compressed_fields),
    {&hf_rdp_compressedLength,   2, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t control_fields[] = {
    {&hf_rdp_action,             2, &action, 0, 0, NULL },
    {&hf_rdp_grantId,            2, NULL, 0, 0, NULL },
    {&hf_rdp_controlId,          4, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t sync_fields[] = {
    {&hf_rdp_messageType,        2, NULL, 0, 0, NULL },
    {&hf_rdp_targetUser,         2, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t mapflags_fields[] = {
    {&hf_rdp_fontMapFirst, 2, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_fontMapLast, 2, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t fontmap_fields[] = {
    {&hf_rdp_numberEntries,      2, NULL, 0, 0, NULL },
    {&hf_rdp_totalNumberEntries, 2, NULL, 0, 0, NULL },
    FI_SUBTREE(&hf_rdp_mapFlags, 2, ett_rdp_mapFlags, mapflags_fields),
    {&hf_rdp_entrySize,          2, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t persistent_fields[] = {
    {&hf_rdp_numEntriesCache0,   2, NULL, 0, 0, NULL },
    {&hf_rdp_numEntriesCache1,   2, NULL, 0, 0, NULL },
    {&hf_rdp_numEntriesCache2,   2, NULL, 0, 0, NULL },
    {&hf_rdp_numEntriesCache3,   2, NULL, 0, 0, NULL },
    {&hf_rdp_numEntriesCache4,   2, NULL, 0, 0, NULL },
    {&hf_rdp_totalEntriesCache0, 2, NULL, 0, 0, NULL },
    {&hf_rdp_totalEntriesCache1, 2, NULL, 0, 0, NULL },
    {&hf_rdp_totalEntriesCache2, 2, NULL, 0, 0, NULL },
    {&hf_rdp_totalEntriesCache3, 2, NULL, 0, 0, NULL },
    {&hf_rdp_totalEntriesCache4, 2, NULL, 0, 0, NULL },
    {&hf_rdp_bBitMask,           1, NULL, 0, 0, NULL },
    {&hf_rdp_Pad2,               1, NULL, 0, 0, NULL },
    {&hf_rdp_Pad3,               2, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };

  const rdp_field_info_t *fields;

  offset = dissect_rdp_fields(tvb, offset, pinfo, tree, share_fields, 0);

  if (pduType2 != PDUTYPE2_CONTROL)
    col_append_sep_str(pinfo->cinfo, COL_INFO, ", ", val_to_str_const(pduType2, rdp_pduType2_vals, "Unknown"));

  fields = NULL;
  switch(pduType2) {
  case PDUTYPE2_UPDATE:
    break;
  case PDUTYPE2_CONTROL:
    fields = control_fields;
    break;
  case PDUTYPE2_POINTER:
    break;
  case PDUTYPE2_INPUT:
    break;
  case PDUTYPE2_SYNCHRONIZE:
    fields = sync_fields;
    break;
  case PDUTYPE2_REFRESH_RECT:
    break;
  case PDUTYPE2_PLAY_SOUND:
    break;
  case PDUTYPE2_SUPPRESS_OUTPUT:
    break;
  case PDUTYPE2_SHUTDOWN_REQUEST:
    break;
  case PDUTYPE2_SHUTDOWN_DENIED:
    break;
  case PDUTYPE2_SAVE_SESSION_INFO:
    break;
  case PDUTYPE2_FONTLIST:
    break;
  case PDUTYPE2_FONTMAP:
    fields = fontmap_fields;
    break;
  case PDUTYPE2_SET_KEYBOARD_INDICATORS:
    break;
  case PDUTYPE2_BITMAPCACHE_PERSISTENT_LIST:
    fields = persistent_fields;
    break;
  case PDUTYPE2_BITMAPCACHE_ERROR_PDU:
    break;
  case PDUTYPE2_SET_KEYBOARD_IME_STATUS:
    break;
  case PDUTYPE2_OFFSCRCACHE_ERROR_PDU:
    break;
  case PDUTYPE2_SET_ERROR_INFO_PDU:
    break;
  case PDUTYPE2_DRAWNINEGRID_ERROR_PDU:
    break;
  case PDUTYPE2_DRAWGDIPLUS_ERROR_PDU:
    break;
  case PDUTYPE2_ARC_STATUS_PDU:
    break;
  case PDUTYPE2_STATUS_INFO_PDU:
    break;
  case PDUTYPE2_MONITOR_LAYOUT_PDU:
    break;
  default:
    break;
  }

  if (fields) {
    offset = dissect_rdp_fields(tvb, offset, pinfo, tree, fields, 0);
  }

  if (pduType2 == PDUTYPE2_CONTROL)
    col_append_sep_str(pinfo->cinfo, COL_INFO, ", ", val_to_str_const(action, rdp_action_vals, "Unknown"));

  return offset;
}

static int
dissect_rdp_capabilitySets(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint32 numberCapabilities) {
  guint   i;
  guint32 lengthCapability;

  rdp_field_info_t cs_fields[] = {
    {&hf_rdp_capabilitySetType, 2, NULL, 0, 0, NULL },
    {&hf_rdp_lengthCapability, 2, &lengthCapability, -4, 0, NULL },
    {&hf_rdp_capabilityData, 0, &lengthCapability, 0, 0, NULL },
    FI_TERMINATOR
  };

  rdp_field_info_t set_fields[] = {
    FI_SUBTREE(&hf_rdp_capabilitySet, 0, ett_rdp_capabilitySet, cs_fields),
    FI_TERMINATOR
  };

  for (i = 0; i < numberCapabilities; i++) {
    offset = dissect_rdp_fields(tvb, offset, pinfo, tree, set_fields, 0);
  }

  return offset;
}

static int
dissect_rdp_demandActivePDU(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {

  guint32 lengthSourceDescriptor;
  guint32 numberCapabilities = 0;

  rdp_field_info_t fields[] = {
    {&hf_rdp_shareId,                    4, NULL, 0, 0, NULL },
    {&hf_rdp_lengthSourceDescriptor,     2, &lengthSourceDescriptor, 0, 0, NULL },
    {&hf_rdp_lengthCombinedCapabilities, 2, NULL, 0, 0, NULL },
    {&hf_rdp_sourceDescriptor,           0, &lengthSourceDescriptor, 0, RDP_FI_STRING|RDP_FI_ANSI, NULL }, /* XXX - T.128 says this is T.50, which is ISO 646, which is only ASCII in its US form */
    {&hf_rdp_numberCapabilities,         2, &numberCapabilities, 0, 0, NULL },
    {&hf_rdp_pad2Octets,                 2, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t final_fields[] = {
    {&hf_rdp_sessionId,                    4, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };

  offset = dissect_rdp_fields(tvb, offset, pinfo, tree, fields, 0);

  offset = dissect_rdp_capabilitySets(tvb, offset, pinfo, tree, numberCapabilities);

  offset = dissect_rdp_fields(tvb, offset, pinfo, tree, final_fields, 0);

  return offset;
}

static int
dissect_rdp_confirmActivePDU(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {

  guint32 lengthSourceDescriptor;
  guint32 numberCapabilities = 0;

  rdp_field_info_t fields[] = {
    {&hf_rdp_shareId,                    4, NULL, 0, 0, NULL },
    {&hf_rdp_originatorId,               2, NULL, 0, 0, NULL },
    {&hf_rdp_lengthSourceDescriptor,     2, &lengthSourceDescriptor, 0, 0, NULL },
    {&hf_rdp_lengthCombinedCapabilities, 2, NULL, 0, 0, NULL },
    {&hf_rdp_sourceDescriptor,           0, &lengthSourceDescriptor, 0, 0, NULL },
    {&hf_rdp_numberCapabilities,         2, &numberCapabilities, 0, 0, NULL },
    {&hf_rdp_pad2Octets,                 2, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };

  offset = dissect_rdp_fields(tvb, offset, pinfo, tree, fields, 0);

  offset = dissect_rdp_capabilitySets(tvb, offset, pinfo, tree, numberCapabilities);

  return offset;
}


static proto_tree *
dissect_rdp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree)
{
  proto_item *item;
  proto_tree *tree;

  col_set_str(pinfo->cinfo, COL_PROTOCOL, "RDP");
  col_clear(pinfo->cinfo, COL_INFO);

  item = proto_tree_add_item(parent_tree, proto_rdp, tvb, 0, -1, ENC_NA);
  tree = proto_item_add_subtree(item, ett_rdp);

  return tree;
}

static int
dissect_rdp_SendData(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_) {
  proto_item      *pi;
  int              offset       = 0;
  guint32          flags        = 0;
  guint32          cbDomain, cbUserName, cbPassword, cbAlternateShell, cbWorkingDir,
                   cbClientAddress, cbClientDir, cbAutoReconnectLen, wBlobLen, pduType = 0;
  guint32          bMsgType = 0xffffffff;
  guint32          encryptedLen = 0;
  conversation_t  *conversation;
  rdp_conv_info_t *rdp_info;

  rdp_field_info_t secFlags_fields[] = {
    {&hf_rdp_flagsPkt,           2, &flags, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_flagsEncrypt,       2, NULL  , 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_flagsResetSeqno,    2, NULL  , 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_flagsIgnoreSeqno,   2, NULL  , 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_flagsLicenseEncrypt,2, NULL  , 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_flagsSecureChecksum,2, NULL  , 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_flagsFlagsHiValid,  2, NULL  , 0, RDP_FI_NOINCOFFSET, NULL },
    FI_TERMINATOR
  };

  rdp_field_info_t se_fields[] = {
    FI_SUBTREE(&hf_rdp_flags, 2, ett_rdp_flags, secFlags_fields),
    FI_FIXEDLEN(&hf_rdp_flagsHi, 2),
    {&hf_rdp_length,                4, &encryptedLen, 0, 0, NULL },
    {&hf_rdp_encryptedClientRandom, 0, &encryptedLen, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t systime_fields [] = {
    FI_FIXEDLEN(&hf_rdp_wYear        , 2),
    FI_FIXEDLEN(&hf_rdp_wMonth       , 2),
    FI_FIXEDLEN(&hf_rdp_wDayOfWeek   , 2),
    FI_FIXEDLEN(&hf_rdp_wDay         , 2),
    FI_FIXEDLEN(&hf_rdp_wHour        , 2),
    FI_FIXEDLEN(&hf_rdp_wMinute      , 2),
    FI_FIXEDLEN(&hf_rdp_wSecond      , 2),
    FI_FIXEDLEN(&hf_rdp_wMilliseconds, 2),
    FI_TERMINATOR,
  };
  rdp_field_info_t tz_info_fields [] = {
    FI_FIXEDLEN(&hf_rdp_Bias, 4),
    {&hf_rdp_StandardName,           64, NULL, 0, RDP_FI_STRING|RDP_FI_UNICODE, NULL },
    FI_SUBTREE(&hf_rdp_StandardDate, 16, ett_rdp_StandardDate, systime_fields),
    FI_FIXEDLEN(&hf_rdp_StandardBias, 4),
    {&hf_rdp_DaylightName,           64, NULL, 0, RDP_FI_STRING|RDP_FI_UNICODE, NULL },
    FI_SUBTREE(&hf_rdp_DaylightDate, 16, ett_rdp_DaylightDate, systime_fields),
    FI_FIXEDLEN(&hf_rdp_DaylightBias, 4),
    FI_TERMINATOR,
  };

  rdp_field_info_t ue_fields[] = {
    {&hf_rdp_codePage,           4, NULL, 0, 0, NULL },
    {&hf_rdp_optionFlags,        4, NULL, 0, RDP_FI_INFO_FLAGS, NULL },
    {&hf_rdp_cbDomain,           2, &cbDomain, 2, 0, NULL },
    {&hf_rdp_cbUserName,         2, &cbUserName, 2, 0, NULL },
    {&hf_rdp_cbPassword,         2, &cbPassword, 2, 0, NULL },
    {&hf_rdp_cbAlternateShell,   2, &cbAlternateShell, 2, 0, NULL },
    {&hf_rdp_cbWorkingDir,       2, &cbWorkingDir, 2, 0, NULL },
    {&hf_rdp_domain,             0, &cbDomain, 0, RDP_FI_STRING, NULL },
    {&hf_rdp_userName,           0, &cbUserName, 0, RDP_FI_STRING, NULL },
    {&hf_rdp_password,           0, &cbPassword, 0, RDP_FI_STRING, NULL },
    {&hf_rdp_alternateShell,     0, &cbAlternateShell, 0, RDP_FI_STRING, NULL },
    {&hf_rdp_workingDir,         0, &cbWorkingDir, 0, RDP_FI_STRING, NULL },
    {&hf_rdp_clientAddressFamily,2, NULL, 0, 0, NULL },
    {&hf_rdp_cbClientAddress,    2, &cbClientAddress, 0, 0, NULL },
    {&hf_rdp_clientAddress,      0, &cbClientAddress, 0, RDP_FI_STRING, NULL },
    {&hf_rdp_cbClientDir,        2, &cbClientDir, 0, 0, NULL },
    {&hf_rdp_clientDir,          0, &cbClientDir, 0, RDP_FI_STRING, NULL },
    FI_SUBTREE(&hf_rdp_clientTimeZone, 172, ett_rdp_clientTimeZone, tz_info_fields),
    {&hf_rdp_clientSessionId,    4, NULL, 0, 0, NULL },
    {&hf_rdp_performanceFlags,   4, NULL, 0, 0, NULL },
    {&hf_rdp_cbAutoReconnectLen, 2, &cbAutoReconnectLen, 0, 0, NULL },
    {&hf_rdp_autoReconnectCookie,0, &cbAutoReconnectLen, 0, 0, NULL },
    {&hf_rdp_reserved1,          2, NULL, 0, 0, NULL },
    {&hf_rdp_reserved2,          2, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t msg_fields[] = {
    {&hf_rdp_bMsgType,           1, &bMsgType, 0, 0, NULL },
    {&hf_rdp_bVersion,           1, NULL, 0, 0, NULL },
    {&hf_rdp_wMsgSize,           2, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t error_fields[] = {
    {&hf_rdp_wErrorCode,         4, NULL, 0, 0, NULL },
    {&hf_rdp_wStateTransition,   4, NULL, 0, 0, NULL },
    {&hf_rdp_wBlobType,          2, NULL, 0, 0, NULL },
    {&hf_rdp_wBlobLen,           2, &wBlobLen, 0, 0, NULL },
    {&hf_rdp_blobData,           0, &wBlobLen, 0, 0, NULL },
    FI_TERMINATOR
  };

  rdp_field_info_t pdu_fields[] = {
    {&hf_rdp_pduTypeType,        2, &pduType, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_pduTypeVersionLow,  2, NULL, 0, RDP_FI_NOINCOFFSET, NULL },
    {&hf_rdp_pduTypeVersionHigh, 2, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t ctrl_fields[] = {
    {&hf_rdp_totalLength,        2, NULL, 0, 0, NULL },
    {&hf_rdp_pduType,            2, NULL, ett_rdp_pduType, RDP_FI_SUBTREE,
     pdu_fields },
    {&hf_rdp_pduSource,          2, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };

  tree = dissect_rdp(tvb, pinfo, tree);

  pi   = proto_tree_add_item(tree, hf_rdp_SendData, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree(pi, ett_rdp_SendData);

  conversation = find_or_create_conversation(pinfo);
  rdp_info = (rdp_conv_info_t *)conversation_get_proto_data(conversation, proto_rdp);

  if (rdp_info &&
      ((rdp_info->licenseAgreed == 0) ||
       (pinfo->num <= rdp_info->licenseAgreed))) {
    /* licensing stage hasn't been completed */
    proto_tree *next_tree;

    flags = tvb_get_letohs(tvb, offset);

    switch(flags & SEC_PKT_MASK) {
    case SEC_EXCHANGE_PKT:
      pi        = proto_tree_add_item(tree, hf_rdp_securityExchangePDU, tvb, offset, -1, ENC_NA);
      next_tree = proto_item_add_subtree(pi, ett_rdp_securityExchangePDU);

      col_append_sep_str(pinfo->cinfo, COL_INFO, " ", "SecurityExchange");

      /*offset=*/ dissect_rdp_fields(tvb, offset, pinfo, next_tree, se_fields, 0);

      break;

    case SEC_INFO_PKT:
      pi        = proto_tree_add_item(tree, hf_rdp_clientInfoPDU, tvb, offset, -1, ENC_NA);
      next_tree = proto_item_add_subtree(pi, ett_rdp_clientInfoPDU);

      col_append_sep_str(pinfo->cinfo, COL_INFO, " ", "ClientInfo");

      offset = dissect_rdp_securityHeader(tvb, offset, pinfo, next_tree, rdp_info, TRUE, NULL);

      if (!(flags & SEC_ENCRYPT)) {

        /*offset =*/ dissect_rdp_fields(tvb, offset, pinfo, next_tree, ue_fields, 0);
      } else {

        /*offset =*/ dissect_rdp_encrypted(tvb, offset, pinfo, next_tree, NULL);
      }
      break;

    case SEC_LICENSE_PKT:
      pi        = proto_tree_add_item(tree, hf_rdp_validClientLicenseData, tvb, offset, -1, ENC_NA);
      next_tree = proto_item_add_subtree(pi, ett_rdp_validClientLicenseData);

      offset = dissect_rdp_securityHeader(tvb, offset, pinfo, next_tree, rdp_info, TRUE, NULL);
      if (!(flags & SEC_ENCRYPT)) {

        offset = dissect_rdp_fields(tvb, offset, pinfo, next_tree, msg_fields, 0);

        col_append_sep_str(pinfo->cinfo, COL_INFO, ", ", val_to_str_const(bMsgType, rdp_bMsgType_vals, "Unknown"));

        switch(bMsgType) {
        case LICENSE_REQUEST:
        case PLATFORM_CHALLENGE:
        case NEW_LICENSE:
        case UPGRADE_LICENSE:
        case LICENSE_INFO:
        case NEW_LICENSE_REQUEST:
        case PLATFORM_CHALLENGE_RESPONSE:
          /* RDPELE Not supported */
          /*offset =*/ dissect_rdp_nyi(tvb, offset, pinfo, next_tree, "RDPELE not implemented");
          break;
        case ERROR_ALERT:
          /*offset =*/ dissect_rdp_fields(tvb, offset, pinfo, next_tree, error_fields, 0);
          rdp_info->licenseAgreed = pinfo->num;
          break;
        default:
          /* Unknown msgType */
          break;
        }
      } else {
        /*offset =*/ dissect_rdp_encrypted(tvb, offset, pinfo, next_tree, NULL);

        /* XXX: we assume the license is agreed in this exchange */
        rdp_info->licenseAgreed = pinfo->num;
      }
      break;

    case SEC_REDIRECTION_PKT:
      /* NotYetImplemented */
      break;

    default:
      break;
    }

    return tvb_captured_length(tvb);
  } /* licensing stage */

  if (rdp_info && (t124_get_last_channelId() == rdp_info->staticChannelId)) {

    offset = dissect_rdp_securityHeader(tvb, offset, pinfo, tree, rdp_info, FALSE, &flags);

    if (!(flags & SEC_ENCRYPT)) {
      proto_tree *next_tree;
      pi        = proto_tree_add_item(tree, hf_rdp_shareControlHeader, tvb, offset, -1, ENC_NA);
      next_tree = proto_item_add_subtree(pi, ett_rdp_shareControlHeader);

      offset = dissect_rdp_fields(tvb, offset, pinfo, next_tree, ctrl_fields, 0);

      pduType &= PDUTYPE_TYPE_MASK; /* mask out just the type */

      if (pduType != PDUTYPE_DATAPDU)
        col_append_sep_str(pinfo->cinfo, COL_INFO, ", ", val_to_str_const(pduType, rdp_pduTypeType_vals, "Unknown"));

      switch(pduType) {
      case PDUTYPE_DEMANDACTIVEPDU:
        /*offset =*/ dissect_rdp_demandActivePDU(tvb, offset, pinfo, next_tree);
        break;
      case PDUTYPE_CONFIRMACTIVEPDU:
        /*offset =*/ dissect_rdp_confirmActivePDU(tvb, offset, pinfo, next_tree);
        break;
      case PDUTYPE_DEACTIVATEALLPDU:
        break;
      case PDUTYPE_DATAPDU:
        /*offset =*/ dissect_rdp_shareDataHeader(tvb, offset, pinfo, next_tree);
        break;
      case PDUTYPE_SERVER_REDIR_PKT:
        break;
      default:
        break;
      }
    } else {

      /*offset =*/ dissect_rdp_encrypted(tvb, offset, pinfo, tree, NULL);
    }

    /* we may get multiple control headers in a single frame */
    col_set_fence(pinfo->cinfo, COL_INFO);

    return tvb_captured_length(tvb);
  } /* (rdp_info && (t124_get_last_channelId() == rdp_info->staticChannelId)) */

  /* Virtual Channel */
  col_append_sep_str(pinfo->cinfo, COL_INFO, " ", "Virtual Channel PDU");

  offset = dissect_rdp_securityHeader(tvb, offset, pinfo, tree, rdp_info, FALSE, &flags);

  if (!(flags & SEC_ENCRYPT))
    /*offset =*/ dissect_rdp_channelPDU(tvb, offset, pinfo, tree);
  else
    /*offset =*/ dissect_rdp_encrypted(tvb, offset, pinfo, tree, "Channel PDU");

  return tvb_captured_length(tvb);
}

static int
dissect_rdp_ClientData(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_) {
  int              offset    = 0;
  proto_item      *pi;
  proto_tree      *next_tree;
  guint16          type;
  guint            length;
  rdp_conv_info_t *rdp_info;

  rdp_field_info_t header_fields[] = {
    {&hf_rdp_headerType,             2, NULL, 0, 0, NULL },
    {&hf_rdp_headerLength,           2, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t core_fields[] = {
    {&hf_rdp_headerType,             2, NULL, 0, 0, NULL },
    {&hf_rdp_headerLength,           2, NULL, 0, 0, NULL },
    {&hf_rdp_versionMajor,           2, NULL, 0, 0, NULL },
    {&hf_rdp_versionMinor,           2, NULL, 0, 0, NULL },
    {&hf_rdp_desktopWidth,           2, NULL, 0, 0, NULL },
    {&hf_rdp_desktopHeight,          2, NULL, 0, 0, NULL },
    {&hf_rdp_colorDepth,             2, NULL, 0, 0, NULL },
    {&hf_rdp_SASSequence,            2, NULL, 0, 0, NULL },
    {&hf_rdp_keyboardLayout,         4, NULL, 0, 0, NULL },
    {&hf_rdp_clientBuild,            4, NULL, 0, 0, NULL },
    {&hf_rdp_clientName,            32, NULL, 0, RDP_FI_STRING|RDP_FI_UNICODE, NULL },
    {&hf_rdp_keyboardType,           4, NULL, 0, 0, NULL },
    {&hf_rdp_keyboardSubType,        4, NULL, 0, 0, NULL },
    {&hf_rdp_keyboardFunctionKey,    4, NULL, 0, 0, NULL },
    {&hf_rdp_imeFileName,           64, NULL, 0, 0, NULL },
    /* The following fields are *optional*.                   */
    /*  I.E., a sequence of one or more of the trailing       */
    /*  fields at the end of the Data Block need not be       */
    /*  present. The length from the header field determines  */
    /*  the actual number of fields which are present.        */
    {&hf_rdp_postBeta2ColorDepth,    2, NULL, 0, 0, NULL },
    {&hf_rdp_clientProductId,        2, NULL, 0, 0, NULL },
    {&hf_rdp_serialNumber,           4, NULL, 0, 0, NULL },
    {&hf_rdp_highColorDepth,         2, NULL, 0, 0, NULL },
    {&hf_rdp_supportedColorDepths,   2, NULL, 0, 0, NULL },
    {&hf_rdp_earlyCapabilityFlags,   2, NULL, 0, 0, NULL },
    {&hf_rdp_clientDigProductId,    64, NULL, 0, RDP_FI_STRING|RDP_FI_UNICODE, NULL }, /* XXX - is this always a string?  MS-RDPBCGR doesn't say so */
    {&hf_rdp_connectionType,         1, NULL, 0, 0, NULL },
    {&hf_rdp_pad1octet,              1, NULL, 0, 0, NULL },
    {&hf_rdp_serverSelectedProtocol, 4, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t security_fields[] = {
    {&hf_rdp_headerType,             2, NULL, 0, 0, NULL },
    {&hf_rdp_headerLength,           2, NULL, 0, 0, NULL },
    {&hf_rdp_encryptionMethods,      4, NULL, 0, 0, NULL },
    {&hf_rdp_extEncryptionMethods,   4, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t cluster_fields[] = {
    {&hf_rdp_headerType,             2, NULL, 0, 0, NULL },
    {&hf_rdp_headerLength,           2, NULL, 0, 0, NULL },
    {&hf_rdp_cluster_flags,          4, NULL, 0, 0, NULL },
    {&hf_rdp_redirectedSessionId,    4, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t msgchannel_fields[] = {
    {&hf_rdp_headerType,             2, NULL, 0, 0, NULL },
    {&hf_rdp_headerLength,           2, NULL, 0, 0, NULL },
    {&hf_rdp_msgChannelFlags,        4, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t monitor_fields[] = {
    {&hf_rdp_headerType,             2, NULL, 0, 0, NULL },
    {&hf_rdp_headerLength,           2, NULL, 0, 0, NULL },
    {&hf_rdp_monitorCount,           4, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t monitorex_fields[] = {
    {&hf_rdp_headerType,             2, NULL, 0, 0, NULL },
    {&hf_rdp_headerLength,           2, NULL, 0, 0, NULL },
    {&hf_rdp_monitorExFlags,         4, NULL, 0, 0, NULL },
    {&hf_rdp_monitorAttributeSize,   4, NULL, 0, 0, NULL },
    {&hf_rdp_monitorCount,           4, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t multitransport_fields[] = {
    {&hf_rdp_headerType,             2, NULL, 0, 0, NULL },
    {&hf_rdp_headerLength,           2, NULL, 0, 0, NULL },
    {&hf_rdp_multiTransportFlags,    4, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };

  tree = dissect_rdp(tvb, pinfo, tree);

  rdp_info = rdp_get_conversation_data(pinfo);

  col_append_sep_str(pinfo->cinfo, COL_INFO, " ", "ClientData");

  pi   = proto_tree_add_item(tree, hf_rdp_ClientData, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree(pi, ett_rdp_ClientData);

  /* Advance through the data blocks using the length from the header for each block.
   *  ToDo: Expert if actual amount dissected (based upon field array) is not equal to length ??
   *  Note: If length is less than the header size (4 bytes) offset is advanced by 4 bytes
   *        to ensure that dissection eventually terminates.
   */

  while (tvb_reported_length_remaining(tvb, offset) > 0) {

    type   = tvb_get_letohs(tvb, offset);
    length = tvb_get_letohs(tvb, offset+2);

#if 0
    printf("offset=%d, type=%x, length=%d, remaining=%d\n",
           offset, type, length, tvb_captured_length_remaining(tvb, offset));
#endif

    switch(type) {
    case CS_CORE:
      pi        = proto_tree_add_item(tree, hf_rdp_clientCoreData, tvb, offset, length, ENC_NA);
      next_tree = proto_item_add_subtree(pi, ett_rdp_clientCoreData);
      /*offset    =*/ dissect_rdp_fields(tvb, offset, pinfo, next_tree, core_fields, length);
      break;

    case CS_SECURITY:
      pi        = proto_tree_add_item(tree, hf_rdp_clientSecurityData, tvb, offset, length, ENC_NA);
      next_tree = proto_item_add_subtree(pi, ett_rdp_clientSecurityData);
      /*offset    =*/ dissect_rdp_fields(tvb, offset, pinfo, next_tree, security_fields, 0);
      break;

    case CS_NET:
      /*offset    =*/ dissect_rdp_clientNetworkData(tvb, offset, pinfo, tree, length, rdp_info);
      break;

    case CS_CLUSTER:
      pi        = proto_tree_add_item(tree, hf_rdp_clientClusterData, tvb, offset, length, ENC_NA);
      next_tree = proto_item_add_subtree(pi, ett_rdp_clientClusterData);
      /*offset    =*/ dissect_rdp_fields(tvb, offset, pinfo, next_tree, cluster_fields, 0);

      break;

    case CS_MONITOR:
      pi        = proto_tree_add_item(tree, hf_rdp_clientMonitorData, tvb, offset, length, ENC_NA);
      next_tree = proto_item_add_subtree(pi, ett_rdp_clientMonitorData);
      /*offset    =*/ dissect_rdp_fields(tvb, offset, pinfo, next_tree, monitor_fields, 0);
      break;

    case CS_MONITOR_EX:
      pi        = proto_tree_add_item(tree, hf_rdp_clientMonitorExData, tvb, offset, length, ENC_NA);
      next_tree = proto_item_add_subtree(pi, ett_rdp_clientMonitorExData);
      /*offset    =*/ dissect_rdp_fields(tvb, offset, pinfo, next_tree, monitorex_fields, 0);
      break;

    case CS_MCS_MSGCHANNEL:
      pi        = proto_tree_add_item(tree, hf_rdp_clientMsgChannelData, tvb, offset, length, ENC_NA);
      next_tree = proto_item_add_subtree(pi, ett_rdp_clientMsgChannelData);
      /*offset    =*/ dissect_rdp_fields(tvb, offset, pinfo, next_tree, msgchannel_fields, 0);
      break;

    case CS_MULTITRANSPORT:
      pi        = proto_tree_add_item(tree, hf_rdp_clientMultiTransportData, tvb, offset, length, ENC_NA);
      next_tree = proto_item_add_subtree(pi, ett_rdp_clientMultiTransportData);
      /*offset    =*/ dissect_rdp_fields(tvb, offset, pinfo, next_tree, multitransport_fields, 0);
      break;

    default: /* unknown */
        pi        = proto_tree_add_item(tree, hf_rdp_clientUnknownData, tvb, offset, length, ENC_NA);
        next_tree = proto_item_add_subtree(pi, ett_rdp_clientUnknownData);
        /*offset    =*/ dissect_rdp_fields(tvb, offset, pinfo, next_tree, header_fields, 0);
        break;
    }
    offset += MAX(4, length);   /* Use length from header, but advance at least 4 bytes */
  }
  return tvb_captured_length(tvb);
}

static int
dissect_rdp_ServerData(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_) {
  int              offset           = 0;
  proto_item      *pi;
  proto_tree      *next_tree;
  guint16          type;
  guint            length;
  guint32          serverRandomLen  = 0;
  guint32          serverCertLen    = 0;
  guint32          encryptionMethod = 0;
  guint32          encryptionLevel  = 0;
  guint32          channelCount     = 0;
  guint32          channelId        = 0;
  guint            i;
  rdp_conv_info_t *rdp_info;

  rdp_field_info_t header_fields[] = {
    {&hf_rdp_headerType,               2, NULL, 0, 0, NULL },
    {&hf_rdp_headerLength,             2, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t sc_fields[] = {
    {&hf_rdp_headerType,               2, NULL, 0, 0, NULL },
    {&hf_rdp_headerLength,             2, NULL, 0, 0, NULL },
    {&hf_rdp_versionMajor,             2, NULL, 0, 0, NULL },
    {&hf_rdp_versionMinor,             2, NULL, 0, 0, NULL },
    /* The following fields are *optional*.                   */
    /*  I.E., a sequence of one or more of the trailing       */
    /*  fields at the end of the Data Block need not be       */
    /*  present. The length from the header field determines  */
    /*  the actual number of fields which are present.        */
    {&hf_rdp_clientRequestedProtocols, 4, NULL, 0, 0, NULL },
    {&hf_rdp_earlyCapabilityFlags,     2, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t ss_fields[] = {
    {&hf_rdp_headerType,               2, NULL, 0, 0, NULL },
    {&hf_rdp_headerLength,             2, NULL, 0, 0, NULL },
    {&hf_rdp_encryptionMethod,         4, &encryptionMethod, 0, 0, NULL },
    {&hf_rdp_encryptionLevel,          4, &encryptionLevel,  0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t encryption_fields[] = {
    {&hf_rdp_serverRandomLen,          4, &serverRandomLen,  0, 0, NULL },
    {&hf_rdp_serverCertLen,            4, &serverCertLen,    0, 0, NULL },
    {&hf_rdp_serverRandom,             0, &serverRandomLen,  0, 0, NULL },
    {&hf_rdp_serverCertificate,        0, &serverCertLen,    0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t sn_fields[] = {
    {&hf_rdp_headerType,               2, NULL, 0, 0, NULL },
    {&hf_rdp_headerLength,             2, NULL, 0, 0, NULL },
    {&hf_rdp_MCSChannelId,             2, &channelId, 0, 0, NULL },
    {&hf_rdp_channelCount,             2, &channelCount, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t array_fields[] = {
      {&hf_rdp_channelIdArray, 0 /*(channelCount * 2)*/, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t channel_fields[] = {
    {&hf_rdp_MCSChannelId, 2, &channelId, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t pad_fields[] = {
    {&hf_rdp_Pad, 2, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t msgchannel_fields[] = {
    {&hf_rdp_headerType,               2, NULL, 0, 0, NULL },
    {&hf_rdp_headerLength,             2, NULL, 0, 0, NULL },
    {&hf_rdp_msgChannelId,             2, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };
  rdp_field_info_t multitransport_fields[] = {
    {&hf_rdp_headerType,               2, NULL, 0, 0, NULL },
    {&hf_rdp_headerLength,             2, NULL, 0, 0, NULL },
    {&hf_rdp_multiTransportFlags,      4, NULL, 0, 0, NULL },
    FI_TERMINATOR
  };

  tree   = dissect_rdp(tvb, pinfo, tree);

  rdp_info = rdp_get_conversation_data(pinfo);

  col_append_sep_str(pinfo->cinfo, COL_INFO, " ", "ServerData");

  pi   = proto_tree_add_item(tree, hf_rdp_ServerData, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree(pi, ett_rdp_ServerData);

  /* Advance through the data blocks using the length from the header for each block.
   *  ToDo: Expert if actual amount dissected (based upon field array) is not equal to length ??
   *  Note: If length is less than the header size (4 bytes) offset is advanced by 4 bytes
   *        to ensure that dissection eventually terminates.
   */
  while (tvb_reported_length_remaining(tvb, offset) > 0) {

    type   = tvb_get_letohs(tvb, offset);
    length = tvb_get_letohs(tvb, offset+2);

    /*    printf("offset=%d, type=%x, length=%d, remaining=%d\n",
          offset, type, length, tvb_captured_length_remaining(tvb, offset)); */

    switch(type) {
    case SC_CORE:
      pi        = proto_tree_add_item(tree, hf_rdp_serverCoreData, tvb, offset, length, ENC_NA);
      next_tree = proto_item_add_subtree(pi, ett_rdp_serverCoreData);
      /*offset    =*/ dissect_rdp_fields(tvb, offset, pinfo, next_tree, sc_fields, length);
      break;

    case SC_SECURITY: {
      gint lcl_offset;
      pi         = proto_tree_add_item(tree, hf_rdp_serverSecurityData, tvb, offset, length, ENC_NA);
      next_tree  = proto_item_add_subtree(pi, ett_rdp_serverSecurityData);

      lcl_offset = dissect_rdp_fields(tvb, offset, pinfo, next_tree, ss_fields, 0);

      col_append_sep_fstr(pinfo->cinfo, COL_INFO, " ", "Encryption: %s (%s)",
                          val_to_str_const(encryptionMethod, rdp_encryptionMethod_vals, "Unknown"),
                          val_to_str_const(encryptionLevel, rdp_encryptionLevel_vals, "Unknown"));

      if ((encryptionLevel != 0) || (encryptionMethod != 0)) {
        /*lcl_offset =*/ dissect_rdp_fields(tvb, lcl_offset, pinfo, next_tree, encryption_fields, 0);
      }

      rdp_info->encryptionMethod = encryptionMethod;
      rdp_info->encryptionLevel  = encryptionLevel;
      break;
    }

    case SC_NET: {
      gint lcl_offset;
      pi        = proto_tree_add_item(tree, hf_rdp_serverNetworkData, tvb, offset, length, ENC_NA);
      next_tree = proto_item_add_subtree(pi, ett_rdp_serverNetworkData);

      lcl_offset = dissect_rdp_fields(tvb, offset, pinfo, next_tree, sn_fields, 0);

      rdp_info->staticChannelId = channelId;
      register_t124_sd_dissector(pinfo, channelId, dissect_rdp_SendData, proto_rdp);

      if (channelCount > 0) {
        array_fields[0].fixedLength = channelCount * 2;
        dissect_rdp_fields(tvb, lcl_offset, pinfo, next_tree, array_fields, 0);

        if (next_tree)
          next_tree = proto_item_add_subtree(next_tree->last_child, ett_rdp_channelIdArray);
        for (i = 0; i < channelCount; i++) {
          lcl_offset = dissect_rdp_fields(tvb, lcl_offset, pinfo, next_tree, channel_fields, 0);
          if (i < MAX_CHANNELS)
            rdp_info->channels[i].value = channelId;

          /* register SendData on this for now */
          register_t124_sd_dissector(pinfo, channelId, dissect_rdp_SendData, proto_rdp);
        }
        if (channelCount % 2)
          /*lcl_offset =*/ dissect_rdp_fields(tvb, lcl_offset, pinfo, next_tree, pad_fields, 0);
      }
      break;
    }

    case SC_MCS_MSGCHANNEL:
      pi        = proto_tree_add_item(tree, hf_rdp_serverMsgChannelData, tvb, offset, length, ENC_NA);
      next_tree = proto_item_add_subtree(pi, ett_rdp_serverMsgChannelData);
      /*offset    =*/ dissect_rdp_fields(tvb, offset, pinfo, next_tree, msgchannel_fields, length);
      break;

    case SC_MULTITRANSPORT:
      pi        = proto_tree_add_item(tree, hf_rdp_serverMultiTransportData, tvb, offset, length, ENC_NA);
      next_tree = proto_item_add_subtree(pi, ett_rdp_serverMultiTransportData);
      /*offset    =*/ dissect_rdp_fields(tvb, offset, pinfo, next_tree, multitransport_fields, length);
      break;

    default:  /* unknown */
      pi        = proto_tree_add_item(tree, hf_rdp_serverUnknownData, tvb, offset, length, ENC_NA);
      next_tree = proto_item_add_subtree(pi, ett_rdp_serverUnknownData);

      /*offset =*/ dissect_rdp_fields(tvb, offset, pinfo, next_tree, header_fields, 0);
      break;
    }
    offset += MAX(4, length);   /* Use length from header, but advance at least 4 bytes */
  }
  return tvb_captured_length(tvb);
}

/* Dissect extra data in a CR PDU */
static int
dissect_rdpCorrelationInfo(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  guint32 type;
  guint32 length;
  proto_item *type_item, *length_item;

  type_item = proto_tree_add_item_ret_uint(tree, hf_rdp_neg_type, tvb, offset, 1, ENC_NA, &type);
  offset += 1;
  if (type != TYPE_RDP_CORRELATION_INFO) {
    expert_add_info(pinfo, type_item, &ei_rdp_not_correlation_info);
    return offset;
  }
  proto_tree_add_item(tree, hf_rdp_correlationInfo_flags, tvb, offset, 1, ENC_NA);
  offset += 1;
  length_item = proto_tree_add_item_ret_uint(tree, hf_rdp_neg_length, tvb, offset, 1, ENC_LITTLE_ENDIAN, &length);
  offset += 2;
  if (length != 36) {
    expert_add_info_format(pinfo, length_item, &ei_rdp_neg_len_invalid, "RDP Correlation Info length is %u, not 36", length);
    return offset;
  }
  proto_tree_add_item(tree, hf_rdp_correlationId, tvb, offset, 16, ENC_NA);
  offset += 16;
  proto_tree_add_item(tree, hf_rdp_correlationInfo_reserved, tvb, offset, 16, ENC_NA);
  offset += 16;
  return offset;
}

static int
dissect_rdpNegReq(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  guint64 flags;
  guint32 length;
  proto_item *length_item;
  static const int *flag_bits[] = {
    &hf_rdp_negReq_flag_restricted_admin_mode_req,
    &hf_rdp_negReq_flag_correlation_info_present,
    NULL
  };
  static const int *requestedProtocols_bits[] = {
    &hf_rdp_requestedProtocols_flag_ssl,
    &hf_rdp_requestedProtocols_flag_hybrid,
    &hf_rdp_requestedProtocols_flag_hybrid_ex,
    NULL
  };

  col_append_str(pinfo->cinfo, COL_INFO, "Negotiate Request");

  proto_tree_add_item(tree, hf_rdp_neg_type, tvb, offset, 1, ENC_NA);
  offset += 1;
  proto_tree_add_bitmask_ret_uint64(tree, tvb, offset, hf_rdp_negReq_flags,
                                    ett_negReq_flags, flag_bits,
                                    ENC_LITTLE_ENDIAN, &flags);
  offset += 1;
  length_item = proto_tree_add_item_ret_uint(tree, hf_rdp_neg_length, tvb, offset, 1, ENC_LITTLE_ENDIAN, &length);
  offset += 2;
  if (length != 8) {
    expert_add_info_format(pinfo, length_item, &ei_rdp_neg_len_invalid, "RDP Negotiate Request length is %u, not 8", length);
    return offset;
  }
  proto_tree_add_bitmask(tree, tvb, offset, hf_rdp_requestedProtocols,
                         ett_requestedProtocols, requestedProtocols_bits,
                         ENC_LITTLE_ENDIAN);
  offset += 4;
  if (flags & CORRELATION_INFO_PRESENT)
    offset = dissect_rdpCorrelationInfo(tvb, offset, pinfo, tree);
  return offset;
}

static int
dissect_rdp_cr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, void* data _U_)
{
  int offset = 0;
  gboolean have_cookie = FALSE;
  gboolean have_rdpNegRequest = FALSE;
  proto_item *item;
  proto_tree *tree;
  gint linelen, next_offset;
  const guint8 *stringval;
  const char *sep = "";

  /*
   * routingToken or cookie?  Both begin with "Cookie: ".
   */
  if (tvb_memeql(tvb, offset, "Cookie: ", 8) == 0) {
    /* Looks like a routing token or cookie */
    have_cookie = TRUE;
  } else if (tvb_bytes_exist(tvb, offset, 4) &&
             tvb_get_guint8(tvb, offset) == TYPE_RDP_NEG_REQ &&
             tvb_get_letohs(tvb, offset + 2) == 8) {
    /* Looks like a Negotiate Request (TYPE_RDP_NEG_REQ, length 8) */
    have_rdpNegRequest = TRUE;
  }
  if (!have_cookie && !have_rdpNegRequest) {
    /* Doesn't look like our data */
    return 0;
  }

  col_set_str(pinfo->cinfo, COL_PROTOCOL, "RDP");
  col_clear(pinfo->cinfo, COL_INFO);

  item = proto_tree_add_item(parent_tree, proto_rdp, tvb, 0, -1, ENC_NA);
  tree = proto_item_add_subtree(item, ett_rdp);

  if (have_cookie) {
    /* XXX - distinguish between routing token and cookie? */
    linelen = tvb_find_line_end(tvb, offset, -1, &next_offset, TRUE);
    proto_tree_add_item_ret_string(tree, hf_rdp_rt_cookie, tvb, offset,
                                   linelen, ENC_ASCII|ENC_NA,
                                   wmem_packet_scope(), &stringval);
    offset = (linelen == -1) ? (gint)tvb_captured_length(tvb) : next_offset;
    col_append_str(pinfo->cinfo, COL_INFO, format_text(stringval, strlen(stringval)));
    sep = ", ";
  }
  /*
   * rdpNegRequest?
   */
  if (tvb_reported_length_remaining(tvb, offset) > 0) {
    col_append_str(pinfo->cinfo, COL_INFO, sep);
    offset = dissect_rdpNegReq(tvb, offset, pinfo, tree);
  }
  return offset; /* returns 0 if nothing was dissected, which is what we want */
}

/* Dissect extra data in a CC PDU */
static int
dissect_rdpNegRsp(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  guint32 length;
  proto_item *length_item;
  static const int *flag_bits[] = {
    &hf_rdp_negRsp_flag_extended_client_data_supported,
    &hf_rdp_negRsp_flag_dynvc_gfx_protocol_supported,
    &hf_rdp_negRsp_flag_restricted_admin_mode_supported,
    NULL
  };
  static const int *selectedProtocol_bits[] = {
    &hf_rdp_selectedProtocol_flag_ssl,
    &hf_rdp_selectedProtocol_flag_hybrid,
    &hf_rdp_selectedProtocol_flag_hybrid_ex,
    NULL
  };

  col_append_str(pinfo->cinfo, COL_INFO, "Negotiate Response");

  proto_tree_add_item(tree, hf_rdp_neg_type, tvb, offset, 1, ENC_NA);
  offset += 1;
  proto_tree_add_bitmask(tree, tvb, offset, hf_rdp_negRsp_flags,
                         ett_negRsp_flags, flag_bits,
                         ENC_LITTLE_ENDIAN);
  offset += 1;
  length_item = proto_tree_add_item_ret_uint(tree, hf_rdp_neg_length, tvb, offset, 1, ENC_LITTLE_ENDIAN, &length);
  offset += 2;
  if (length != 8) {
    expert_add_info_format(pinfo, length_item, &ei_rdp_neg_len_invalid, "RDP Negotiate Response length is %u, not 8", length);
    return offset;
  }
  proto_tree_add_bitmask(tree, tvb, offset, hf_rdp_selectedProtocol,
                         ett_selectedProtocol, selectedProtocol_bits,
                         ENC_LITTLE_ENDIAN);
  offset += 4;
  return offset;
}

static int
dissect_rdpNegFailure(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree) {
  guint32 length;
  proto_item *length_item;
  guint32 failureCode;

  col_append_str(pinfo->cinfo, COL_INFO, "Negotiate Failure");

  proto_tree_add_item(tree, hf_rdp_neg_type, tvb, offset, 1, ENC_NA);
  offset += 1;
  proto_tree_add_item(tree, hf_rdp_negReq_flags, tvb, offset, 1, ENC_LITTLE_ENDIAN);
  offset += 1;
  length_item = proto_tree_add_item_ret_uint(tree, hf_rdp_neg_length, tvb, offset, 1, ENC_LITTLE_ENDIAN, &length);
  offset += 2;
  if (length != 8) {
    expert_add_info_format(pinfo, length_item, &ei_rdp_neg_len_invalid, "RDP Negotiate Failure length is %u, not 8", length);
    return offset;
  }
  proto_tree_add_item_ret_uint(tree, hf_rdp_negFailure_failureCode, tvb, offset, 4, ENC_LITTLE_ENDIAN, &failureCode);
  offset += 4;
  col_append_fstr(pinfo->cinfo, COL_INFO, ", failureCode %s",
                  val_to_str(failureCode, failure_code_vals, "Unknown (0x%08x)"));
  return offset;
}

static int
dissect_rdp_cc(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, void* data _U_)
{
  int offset = 0;
  guint8 type;
  guint16 length;
  gboolean ours = FALSE;
  proto_item *item;
  proto_tree *tree;

  if (tvb_bytes_exist(tvb, offset, 4)) {
    type = tvb_get_guint8(tvb, offset);
    length = tvb_get_letohs(tvb, offset + 2);
    if ((type == TYPE_RDP_NEG_RSP || type == TYPE_RDP_NEG_FAILURE) &&
        length == 8) {
      /* Looks like a Negotiate Response (TYPE_RDP_NEG_RSP, length 8)
         or a Negotaiate Failure (TYPE_RDP_NEG_FAILURE, length 8) */
      ours = TRUE;
    }
  }
  if (!ours) {
    /* Doesn't look like our data */
    return 0;
  }

  col_set_str(pinfo->cinfo, COL_PROTOCOL, "RDP");
  col_clear(pinfo->cinfo, COL_INFO);

  item = proto_tree_add_item(parent_tree, proto_rdp, tvb, 0, -1, ENC_NA);
  tree = proto_item_add_subtree(item, ett_rdp);

  switch (type) {

  case TYPE_RDP_NEG_RSP:
    offset = dissect_rdpNegRsp(tvb, offset, pinfo, tree);
    break;

  case TYPE_RDP_NEG_FAILURE:
    offset = dissect_rdpNegFailure(tvb, offset, pinfo, tree);
    break;
  }
  return offset;
}

/*--- proto_register_rdp -------------------------------------------*/
void
proto_register_rdp(void) {

  /* List of fields */
  static hf_register_info hf[] = {
    { &hf_rdp_rt_cookie,
      { "Routing Token/Cookie", "rdp.rt_cookie",
        FT_STRING, BASE_NONE, NULL, 0,
	NULL, HFILL }},
    { &hf_rdp_neg_type,
      { "Type", "rdp.neg_type",
        FT_UINT8, BASE_HEX, VALS(neg_type_vals), 0,
	NULL, HFILL }},
    { &hf_rdp_negReq_flags,
      { "Flags", "rdp.negReq.flags",
        FT_UINT8, BASE_HEX, NULL, 0,
	NULL, HFILL }},
    { &hf_rdp_negReq_flag_restricted_admin_mode_req,
      { "Restricted admin mode required", "rdp.negReq.flags.restricted_admin_mode_req",
        FT_BOOLEAN, 8, NULL, RESTRICTED_ADMIN_MODE_REQUIRED,
	NULL, HFILL }},
    { &hf_rdp_negReq_flag_correlation_info_present,
      { "Correlation info present", "rdp.negReq.flags.correlation_info_present",
        FT_BOOLEAN, 8, NULL, CORRELATION_INFO_PRESENT,
	NULL, HFILL }},
    { &hf_rdp_neg_length,
      { "Length", "rdp.neg_length",
        FT_UINT16, BASE_DEC, NULL, 0,
	NULL, HFILL }},
    { &hf_rdp_requestedProtocols,
      { "requestedProtocols", "rdp.negReq.requestedProtocols",
        FT_UINT32, BASE_HEX, NULL, 0,
	NULL, HFILL }},
    { &hf_rdp_requestedProtocols_flag_ssl,
      { "TLS security supported", "rdp.negReq.requestedProtocols.ssl",
        FT_BOOLEAN, 32, NULL, 0x00000001,
	NULL, HFILL }},
    { &hf_rdp_requestedProtocols_flag_hybrid,
      { "CredSSP supported", "rdp.negReq.requestedProtocols.hybrid",
        FT_BOOLEAN, 32, NULL, 0x00000002,
	NULL, HFILL }},
    { &hf_rdp_requestedProtocols_flag_hybrid_ex,
      { "Early User Authorization Result PDU supported", "rdp.negReq.requestedProtocols.hybrid_ex",
        FT_BOOLEAN, 32, NULL, 0x00000008,
	NULL, HFILL }},
    { &hf_rdp_correlationInfo_flags,
      { "Flags", "rdp.correlationInfo.flags",
        FT_UINT8, BASE_HEX, NULL, 0,
	NULL, HFILL }},
    { &hf_rdp_correlationId,
      { "correlationId", "rdp.correlationInfo.correlationId",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_correlationInfo_reserved,
      { "Reserved", "rdp.correlationInfo.reserved",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_negRsp_flags,
      { "Flags", "rdp.negRsp.flags",
        FT_UINT8, BASE_HEX, NULL, 0,
	NULL, HFILL }},
    { &hf_rdp_negRsp_flag_extended_client_data_supported,
      { "Extended Client Data Blocks supported", "rdp.negRsp.flags.extended_client_data_supported",
        FT_BOOLEAN, 8, NULL, 0x01,
	NULL, HFILL }},
    { &hf_rdp_negRsp_flag_dynvc_gfx_protocol_supported,
      { "Graphics Pipeline Extension Protocol supported", "rdp.negRsp.flags.dynvc_gfx_protocol_supported",
        FT_BOOLEAN, 8, NULL, 0x02,
	NULL, HFILL }},
    { &hf_rdp_negRsp_flag_restricted_admin_mode_supported,
      { "Restricted admin mode supported", "rdp.negRsp.flags.restricted_admin_mode_supported",
        FT_BOOLEAN, 8, NULL, 0x08,
	NULL, HFILL }},
    { &hf_rdp_selectedProtocol,
      { "selectedProtocol", "rdp.negReq.selectedProtocol",
        FT_UINT32, BASE_HEX, NULL, 0,
	NULL, HFILL }},
    { &hf_rdp_selectedProtocol_flag_ssl,
      { "TLS security selected", "rdp.negReq.selectedProtocol.ssl",
        FT_BOOLEAN, 32, NULL, 0x00000001,
	NULL, HFILL }},
    { &hf_rdp_selectedProtocol_flag_hybrid,
      { "CredSSP selected", "rdp.negReq.selectedProtocol.hybrid",
        FT_BOOLEAN, 32, NULL, 0x00000002,
	NULL, HFILL }},
    { &hf_rdp_selectedProtocol_flag_hybrid_ex,
      { "Early User Authorization Result PDU selected", "rdp.negReq.selectedProtocol.hybrid_ex",
        FT_BOOLEAN, 32, NULL, 0x00000008,
	NULL, HFILL }},
    { &hf_rdp_negFailure_failureCode,
      { "failureCode", "rdp.negFailure.failureCode",
        FT_UINT32, BASE_HEX, VALS(failure_code_vals), 0,
	NULL, HFILL }},
    { &hf_rdp_ClientData,
      { "ClientData", "rdp.clientData",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_SendData,
      { "SendData", "rdp.sendData",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_clientCoreData,
      { "clientCoreData", "rdp.client.coreData",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_clientSecurityData,
      { "clientSecurityData", "rdp.client.securityData",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_clientNetworkData,
      { "clientNetworkData", "rdp.client.networkData",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_clientClusterData,
      { "clientClusterData", "rdp.client.clusterData",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_clientMonitorData,
      { "clientMonitorData", "rdp.client.monitorData",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_clientMsgChannelData,
      { "clientMsgChannelData", "rdp.client.msgChannelData",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_clientMonitorExData,
      { "clientMonitorExData", "rdp.client.monitorExData",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_clientMultiTransportData,
      { "clientMultiTransportData", "rdp.client.multiTransportData",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_clientUnknownData,
      { "clientUnknownData", "rdp.unknownData.client",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_ServerData,
      { "ServerData", "rdp.serverData",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_serverCoreData,
      { "serverCoreData", "rdp.server.coreData",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_serverSecurityData,
      { "serverSecurityData", "rdp.server.securityData",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_serverNetworkData,
      { "serverNetworkData", "rdp.server.networkData",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_serverMsgChannelData,
      { "serverMsgChannelData", "rdp.server.msgChannelData",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_serverMultiTransportData,
      { "serverMultiTransportData", "rdp.server.multiTransportData",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_serverUnknownData,
      { "serverUnknownData", "rdp.unknownData.server",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_securityExchangePDU,
      { "securityExchangePDU", "rdp.securityExchangePDU",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_clientInfoPDU,
      { "clientInfoPDU", "rdp.clientInfoPDU",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_validClientLicenseData,
      { "validClientLicenseData", "rdp.validClientLicenseData",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_headerType,
      { "headerType", "rdp.header.type",
        FT_UINT16, BASE_HEX, VALS(rdp_headerType_vals), 0,
        NULL, HFILL }},
    { &hf_rdp_headerLength,
      { "headerLength", "rdp.header.length",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_versionMajor,
      { "versionMajor", "rdp.version.major",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_versionMinor,
      { "versionMinor", "rdp.version.minor",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_desktopWidth,
      { "desktopWidth", "rdp.desktop.width",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_desktopHeight,
      { "desktopHeight", "rdp.desktop.height",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_colorDepth,
      { "colorDepth", "rdp.colorDepth",
        FT_UINT16, BASE_HEX, VALS(rdp_colorDepth_vals), 0,
        NULL, HFILL }},
    { &hf_rdp_SASSequence,
      { "SASSequence", "rdp.SASSequence",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_keyboardLayout,
      { "keyboardLayout", "rdp.keyboardLayout",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_clientBuild,
      { "clientBuild", "rdp.client.build",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_clientName,
      { "clientName", "rdp.client.name",
        FT_STRINGZ, BASE_NONE, NULL, 0, /* supposed to be null-terminated */
        NULL, HFILL }},
    { &hf_rdp_keyboardType,
      { "keyboardType", "rdp.keyboard.type",
        FT_UINT32, BASE_DEC, VALS(rdp_keyboardType_vals), 0,
        NULL, HFILL }},
    { &hf_rdp_keyboardSubType,
      { "keyboardSubType", "rdp.keyboard.subtype",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_keyboardFunctionKey,
      { "keyboardFunctionKey", "rdp.keyboard.functionkey",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_imeFileName,
      { "imeFileName", "rdp.imeFileName",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_postBeta2ColorDepth,
      { "postBeta2ColorDepth", "rdp.postBeta2ColorDepth",
        FT_UINT16, BASE_HEX, VALS(rdp_colorDepth_vals), 0,
        NULL, HFILL }},
    { &hf_rdp_clientProductId,
      { "clientProductId", "rdp.client.productId",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_serialNumber,
      { "serialNumber", "rdp.serialNumber",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_highColorDepth,
      { "highColorDepth", "rdp.highColorDepth",
        FT_UINT16, BASE_HEX, VALS(rdp_highColorDepth_vals), 0,
        NULL, HFILL }},
    { &hf_rdp_supportedColorDepths,
      { "supportedColorDepths", "rdp.supportedColorDepths",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_earlyCapabilityFlags,
      { "earlyCapabilityFlags", "rdp.earlyCapabilityFlags",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_clientDigProductId,
      { "clientDigProductId", "rdp.client.digProductId",
        FT_STRINGZ, BASE_NONE, NULL, 0, /* XXX - is this always a string?  MS-RDPBCGR doesn't say so */
        NULL, HFILL }},
    { &hf_rdp_connectionType,
      { "connectionType", "rdp.connectionType",
        FT_UINT8, BASE_DEC, VALS(rdp_connectionType_vals), 0,
        NULL, HFILL }},
    { &hf_rdp_pad1octet,
      { "pad1octet", "rdp.pad1octet",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_serverSelectedProtocol,
      { "serverSelectedProtocol", "rdp.serverSelectedProtocol",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_encryptionMethods,
      { "encryptionMethods", "rdp.encryptionMethods",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_extEncryptionMethods,
      { "extEncryptionMethods", "rdp.extEncryptionMethods",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_cluster_flags,    /* ToDo: Display flags in detail */
      { "clusterFlags", "rdp.clusterFlags",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_redirectedSessionId,
      { "redirectedSessionId", "rdp.redirectedSessionId",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_msgChannelFlags,
      { "msgChannelFlags", "rdp.msgChannelFlags",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_msgChannelId,
      { "msgChannelId", "rdp.msgChannelId",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_monitorExFlags,
      { "monitorExFlags", "rdp.monitorExFlags",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_monitorAttributeSize,
      { "monitorAttributeSize", "rdp.monitorAttributeSize",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_monitorCount,
      { "monitorCount", "rdp.monitorCount",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_multiTransportFlags,
      { "multiTransportFlags", "rdp.multiTransportFlags",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_encryptionMethod,
      { "encryptionMethod", "rdp.encryptionMethod",
        FT_UINT32, BASE_HEX, VALS(rdp_encryptionMethod_vals), 0,
        NULL, HFILL }},
    { &hf_rdp_encryptionLevel,
      { "encryptionLevel", "rdp.encryptionLevel",
        FT_UINT32, BASE_HEX, VALS(rdp_encryptionLevel_vals), 0,
        NULL, HFILL }},
    { &hf_rdp_serverRandomLen,
      { "serverRandomLen", "rdp.serverRandomLen",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_serverCertLen,
      { "serverCertLen", "rdp.serverCertLen",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_serverRandom,
      { "serverRandom", "rdp.serverRandom",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_serverCertificate,
      { "serverCertificate", "rdp.serverCertificate",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_clientRequestedProtocols,
      { "clientRequestedProtocols", "rdp.client.requestedProtocols",
        FT_UINT32, BASE_HEX, VALS(rdp_requestedProtocols_vals), 0,
        NULL, HFILL }},
    { &hf_rdp_MCSChannelId,
      { "MCSChannelId", "rdp.MCSChannelId",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_channelCount,
      { "channelCount", "rdp.channelCount",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_channelIdArray,
      { "channelIdArray", "rdp.channelIdArray",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_Pad,
      { "Pad", "rdp.Pad",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_flags,
      { "flags", "rdp.flags",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_channelFlags,
      { "channelFlags", "rdp.channelFlags",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_flagsPkt,
      { "flagsPkt", "rdp.flags.pkt",
        FT_UINT16, BASE_HEX, VALS(rdp_flagsPkt_vals), SEC_PKT_MASK,
        NULL, HFILL }},
    { &hf_rdp_flagsEncrypt,
      { "flagsEncrypt", "rdp.flags.encrypt",
        FT_UINT16, BASE_HEX, NULL, SEC_ENCRYPT,
        NULL, HFILL }},
    { &hf_rdp_flagsResetSeqno,
      { "flagsResetSeqno", "rdp.flags.resetseqno",
        FT_UINT16, BASE_HEX, NULL, SEC_RESET_SEQNO,
        NULL, HFILL }},
    { &hf_rdp_flagsIgnoreSeqno,
      { "flagsIgnoreSeqno", "rdp.flags.ignoreseqno",
        FT_UINT16, BASE_HEX, NULL, SEC_IGNORE_SEQNO,
        NULL, HFILL }},
    { &hf_rdp_flagsLicenseEncrypt,
      { "flagsLicenseEncrypt", "rdp.flags.licenseencrypt",
        FT_UINT16, BASE_HEX, NULL, SEC_LICENSE_ENCRYPT_CS,
        NULL, HFILL }},
    { &hf_rdp_flagsSecureChecksum,
      { "flagsSecureChecksum", "rdp.flags.securechecksum",
        FT_UINT16, BASE_HEX, NULL, SEC_SECURE_CHECKSUM,
        NULL, HFILL }},
    { &hf_rdp_flagsFlagsHiValid,
      { "flagsHiValid", "rdp.flags.flagshivalid",
        FT_UINT16, BASE_HEX, NULL, SEC_FLAGSHI_VALID,
        NULL, HFILL }},
    { &hf_rdp_flagsHi,
      { "flagsHi", "rdp.flagsHi",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_length,
      { "length", "rdp.length",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_encryptedClientRandom,
      { "encryptedClientRandom", "rdp.encryptedClientRandom",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_dataSignature,
      { "dataSignature", "rdp.dataSignature",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_fipsLength,
      { "fipsLength", "rdp.fipsLength",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_fipsVersion,
      { "fipsVersion", "rdp.fipsVersion",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_padlen,
      { "padlen", "rdp.padlen",
        FT_UINT8, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_codePage,
      { "codePage", "rdp.codePage",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_optionFlags,
      { "optionFlags", "rdp.optionFlags",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_cbDomain,
      { "cbDomain", "rdp.domain.length",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_cbUserName,
      { "cbUserName", "rdp.userName.length",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_cbPassword,
      { "cbPassword", "rdp.password.length",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_cbAlternateShell,
      { "cbAlternateShell", "rdp.alternateShell.length",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_cbWorkingDir,
      { "cbWorkingDir", "rdp.workingDir.length",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_cbClientAddress,
      { "cbClientAddress", "rdp.client.address.length",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_cbClientDir,
      { "cbClientDir", "rdp.client.dir.length",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_cbAutoReconnectLen,
      { "cbAutoReconnectLen", "rdp.autoReconnectCookie.length",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_domain,
      { "domain", "rdp.domain",
        FT_STRINGZ, BASE_NONE, NULL, 0,  /* null-terminated, count includes terminator */
        NULL, HFILL }},
    { &hf_rdp_userName,
      { "userName", "rdp.userName",
        FT_STRINGZ, BASE_NONE, NULL, 0,  /* null-terminated, count includes terminator */
        NULL, HFILL }},
    { &hf_rdp_password,
      { "password", "rdp.password",
        FT_STRINGZ, BASE_NONE, NULL, 0,  /* null-terminated, count includes terminator */
        NULL, HFILL }},
    { &hf_rdp_alternateShell,
      { "alternateShell", "rdp.alternateShell",
        FT_STRINGZ, BASE_NONE, NULL, 0,  /* null-terminated, count includes terminator */
        NULL, HFILL }},
    { &hf_rdp_workingDir,
      { "workingDir", "rdp.workingDir",
        FT_STRINGZ, BASE_NONE, NULL, 0,  /* null-terminated, count includes terminator */
        NULL, HFILL }},
    { &hf_rdp_clientAddressFamily,
      { "clientAddressFamily", "rdp.client.addressFamily",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_clientAddress,
      { "clientAddress", "rdp.client.address",
        FT_STRINGZ, BASE_NONE, NULL, 0,  /* null-terminated, count includes terminator */
        NULL, HFILL }},
    { &hf_rdp_clientDir,
      { "clientDir", "rdp.client.dir",
        FT_STRINGZ, BASE_NONE, NULL, 0,  /* null-terminated, count includes terminator */
        NULL, HFILL }},
    { &hf_rdp_clientTimeZone,
      { "clientTimeZone", "rdp.client.timeZone",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_clientSessionId,
      { "clientSessionId", "rdp.client.sessionId",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_performanceFlags,
      { "performanceFlags", "rdp.performanceFlags",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_autoReconnectCookie,
      { "autoReconnectCookie", "rdp.autoReconnectCookie",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_reserved1,
      { "reserved1", "rdp.reserved1",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_reserved2,
      { "reserved2", "rdp.reserved2",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_bMsgType,
      { "bMsgType", "rdp.bMsgType",
        FT_UINT8, BASE_HEX, VALS(rdp_bMsgType_vals), 0,
        NULL, HFILL }},
    { &hf_rdp_bVersion,
      { "bVersion", "rdp.bVersion",
        FT_UINT8, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_wMsgSize,
      { "wMsgSize", "rdp.wMsgSize",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_wBlobType,
      { "wBlobType", "rdp.wBlobType",
        FT_UINT16, BASE_DEC, VALS(rdp_wBlobType_vals), 0,
        NULL, HFILL }},
    { &hf_rdp_wBlobLen,
      { "wBlobLen", "rdp.wBlobLen",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_blobData,
      { "blobData", "rdp.blobData",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_shareControlHeader,
      { "shareControlHeader", "rdp.shareControlHeader",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_channelPDUHeader,
      { "channelPDUHeader", "rdp.channelPDUHeader",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_virtualChannelData,
      { "virtualChannelData", "rdp.virtualChannelData",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_totalLength,
      { "totalLength", "rdp.totalLength",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_pduType,
      { "pduType", "rdp.pduType",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_pduTypeType,
      { "pduTypeType", "rdp.pduType.type",
        FT_UINT16, BASE_HEX, VALS(rdp_pduTypeType_vals), PDUTYPE_TYPE_MASK,
        NULL, HFILL }},
    { &hf_rdp_pduTypeVersionLow,
      { "pduTypeVersionLow", "rdp.pduType.versionLow",
        FT_UINT16, BASE_DEC, NULL, PDUTYPE_VERSIONLOW_MASK,
        NULL, HFILL }},
    { &hf_rdp_pduTypeVersionHigh,
      { "pduTypeVersionHigh", "rdp.pduType.versionHigh",
        FT_UINT16, BASE_DEC, NULL, PDUTYPE_VERSIONHIGH_MASK,
        NULL, HFILL }},
    { &hf_rdp_pduSource,
      { "pduSource", "rdp.pduSource",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_shareId,
      { "shareId", "rdp.shareId",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_pad1,
      { "pad1", "rdp.pad1",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_streamId,
      { "streamId", "rdp.streamId",
        FT_UINT8, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_uncompressedLength,
      { "uncompressedLength", "rdp.uncompressedLength",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_pduType2,
      { "pduType2", "rdp.pduType2",
        FT_UINT8, BASE_DEC, VALS(rdp_pduType2_vals), 0,
        NULL, HFILL }},
    { &hf_rdp_compressedType,
      { "compressedType", "rdp.compressedType",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_compressedTypeType,
      { "compressedTypeType", "rdp.compressedType.type",
        FT_UINT8, BASE_HEX, VALS(rdp_compressionType_vals),
        PacketCompressionTypeMask,
        NULL, HFILL }},
    { &hf_rdp_compressedTypeCompressed,
      { "compressedTypeCompressed", "rdp.compressedType.compressed",
        FT_UINT8, BASE_HEX, NULL, PACKET_COMPRESSED,
        NULL, HFILL }},
    { &hf_rdp_compressedTypeAtFront,
      { "compressedTypeAtFront", "rdp.compressedType.atFront",
        FT_UINT8, BASE_HEX, NULL, PACKET_AT_FRONT,
        NULL, HFILL }},
    { &hf_rdp_compressedTypeFlushed,
      { "compressedTypeFlushed", "rdp.compressedType.flushed",
        FT_UINT8, BASE_HEX, NULL, PACKET_FLUSHED,
        NULL, HFILL }},
    { &hf_rdp_compressedLength,
      { "compressedLength", "rdp.compressedLength",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_wErrorCode,
      { "errorCode", "rdp.errorCode",
        FT_UINT32, BASE_DEC, VALS(rdp_wErrorCode_vals), 0,
        NULL, HFILL }},
    { &hf_rdp_wStateTransition,
      { "stateTransition", "rdp.stateTransition",
        FT_UINT32, BASE_DEC, VALS(rdp_wStateTransition_vals), 0,
        NULL, HFILL }},
    { &hf_rdp_numberEntries,
      { "numberEntries", "rdp.numberEntries",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_totalNumberEntries,
      { "totalNumberEntries", "rdp.totalNumberEntries",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_mapFlags,
      { "mapFlags", "rdp.mapFlags",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_fontMapFirst,
      { "fontMapFirst", "rdp.mapFlags.fontMapFirst",
        FT_UINT16, BASE_HEX, NULL, FONTMAP_FIRST,
        NULL, HFILL }},
    { &hf_rdp_fontMapLast,
      { "fontMapLast", "rdp.mapFlags.fontMapLast",
        FT_UINT16, BASE_HEX, NULL, FONTMAP_LAST,
        NULL, HFILL }},
    { &hf_rdp_entrySize,
      { "entrySize", "rdp.entrySize",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_action,
      { "action", "rdp.action",
        FT_UINT16, BASE_HEX, VALS(rdp_action_vals),
        0,
        NULL, HFILL }},
    { &hf_rdp_grantId,
      { "grantId", "rdp.grantId",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_controlId,
      { "controlId", "rdp.controlId",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_messageType,
      { "messageType", "rdp.messageType",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_targetUser,
      { "targetUser", "rdp.targetUser",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_numEntriesCache0,
      { "numEntriesCache0", "rdp.numEntriesCache0",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_numEntriesCache1,
      { "numEntriesCache1", "rdp.numEntriesCache1",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_numEntriesCache2,
      { "numEntriesCache2", "rdp.numEntriesCache2",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_numEntriesCache3,
      { "numEntriesCache3", "rdp.numEntriesCache3",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_numEntriesCache4,
      { "numEntriesCache4", "rdp.numEntriesCache4",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_totalEntriesCache0,
      { "totalEntriesCache0", "rdp.totalEntriesCache0",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_totalEntriesCache1,
      { "totalEntriesCache1", "rdp.totalEntriesCache1",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_totalEntriesCache2,
      { "totalEntriesCache2", "rdp.totalEntriesCache2",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_totalEntriesCache3,
      { "totalEntriesCache3", "rdp.totalEntriesCache3",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_totalEntriesCache4,
      { "totalEntriesCache4", "rdp.totalEntriesCache4",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_bBitMask,
      { "bBitMask", "rdp.bBitMask",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_Pad2,
      { "Pad2", "rdp.Pad2",
        FT_UINT8, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_Pad3,
      { "Pad3", "rdp.Pad3",
        FT_UINT16, BASE_HEX, NULL, 0,
        NULL, HFILL }},
#if 0
    { &hf_rdp_Key1,
      { "Key1", "rdp.Key1",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL }},
#endif
#if 0
    { &hf_rdp_Key2,
      { "Key2", "rdp.Key2",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL }},
#endif
    { &hf_rdp_originatorId,
      { "originatorId", "rdp.OriginatorId",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_lengthSourceDescriptor,
      { "lengthSourceDescriptor", "rdp.lengthSourceDescriptor",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_lengthCombinedCapabilities,
      { "lengthCombinedCapabilities", "rdp.lengthCombinedCapabilities",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_sourceDescriptor,
      { "sourceDescriptor", "rdp.sourceDescriptor",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_numberCapabilities,
      { "numberCapabilities", "rdp.numberCapabilities",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_pad2Octets,
      { "pad2Octets", "rdp.pad2Octets",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_capabilitySetType,
      { "capabilitySetType", "rdp.capabilitySetType",
        FT_UINT16, BASE_HEX, VALS(rdp_capabilityType_vals), 0,
        NULL, HFILL }},
    { &hf_rdp_capabilitySet,
      { "capabilitySet", "rdp.capabilitySet",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_lengthCapability,
      { "lengthCapability", "rdp.lengthCapability",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_capabilityData,
      { "capabilityData", "rdp.capabilityData",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
#if 0
    { &hf_rdp_unknownData,
      { "unknownData", "rdp.unknownData",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
#endif
    { &hf_rdp_notYetImplemented,
      { "notYetImplemented", "rdp.notYetImplemented",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_encrypted,
      { "encryptedData", "rdp.encryptedData",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
#if 0
    { &hf_rdp_compressed,
      { "compressedData", "rdp.compressedData",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
#endif
    { &hf_rdp_sessionId,
      { "sessionId", "rdp.sessionId",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_channelDefArray,
      { "channelDefArray", "rdp.channelDefArray",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_channelDef,
      { "channelDef", "rdp.channelDef",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_name,
      { "name", "rdp.name",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_options,
      { "options", "rdp.options",
        FT_UINT32, BASE_HEX, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_optionsInitialized,
      { "optionsInitialized", "rdp.options.initialized",
        FT_UINT32, BASE_HEX, NULL, CHANNEL_OPTION_INITIALIZED,
        NULL, HFILL }},
    { &hf_rdp_optionsEncryptRDP,
      { "encryptRDP", "rdp.options.encrypt.rdp",
        FT_UINT32, BASE_HEX, NULL, CHANNEL_OPTION_ENCRYPT_RDP,
        NULL, HFILL }},
    { &hf_rdp_optionsEncryptSC,
      { "encryptSC", "rdp.options.encrypt.sc",
        FT_UINT32, BASE_HEX, NULL, CHANNEL_OPTION_ENCRYPT_SC,
        NULL, HFILL }},
    { &hf_rdp_optionsEncryptCS,
      { "encryptCS", "rdp.options.encrypt.cs",
        FT_UINT32, BASE_HEX, NULL, CHANNEL_OPTION_ENCRYPT_CS,
        NULL, HFILL }},
    { &hf_rdp_optionsPriHigh,
      { "priorityHigh", "rdp.options.priority.high",
        FT_UINT32, BASE_HEX, NULL, CHANNEL_OPTION_PRI_HIGH,
        NULL, HFILL }},
    { &hf_rdp_optionsPriMed,
      { "priorityMed", "rdp.options.priority.med",
        FT_UINT32, BASE_HEX, NULL, CHANNEL_OPTION_PRI_MED,
        NULL, HFILL }},
    { &hf_rdp_optionsPriLow,
      { "priorityLow", "rdp.options.priority.low",
        FT_UINT32, BASE_HEX, NULL, CHANNEL_OPTION_PRI_LOW,
        NULL, HFILL }},
    { &hf_rdp_optionsCompressRDP,
      { "compressRDP", "rdp.options.compress.rdp",
        FT_UINT32, BASE_HEX, NULL, CHANNEL_OPTION_COMPRESS_RDP,
        NULL, HFILL }},
    { &hf_rdp_optionsCompress,
      { "compress", "rdp.options.compress",
        FT_UINT32, BASE_HEX, NULL, CHANNEL_OPTION_COMPRESS,
        NULL, HFILL }},
    { &hf_rdp_optionsShowProtocol,
      { "showProtocol", "rdp.options.showprotocol",
        FT_UINT32, BASE_HEX, NULL, CHANNEL_OPTION_SHOW_PROTOCOL,
        NULL, HFILL }},
    { &hf_rdp_optionsRemoteControlPersistent,
      { "remoteControlPersistent", "rdp.options.remotecontrolpersistent",
        FT_UINT32, BASE_HEX, NULL, CHANNEL_OPTION_REMOTE_CONTROL_PERSISTENT,
        NULL, HFILL }},
    { &hf_rdp_channelFlagFirst,
      { "channelFlagFirst", "rdp.channelFlag.first",
        FT_UINT32, BASE_HEX, NULL, CHANNEL_FLAG_FIRST,
        NULL, HFILL }},
    { &hf_rdp_channelFlagLast,
      { "channelFlagLast", "rdp.channelFlag.last",
        FT_UINT32, BASE_HEX, NULL, CHANNEL_FLAG_LAST,
        NULL, HFILL }},
    { &hf_rdp_channelFlagShowProtocol,
      { "channelFlagShowProtocol", "rdp.channelFlag.showProtocol",
        FT_UINT32, BASE_HEX, NULL, CHANNEL_FLAG_SHOW_PROTOCOL,
        NULL, HFILL }},
    { &hf_rdp_channelFlagSuspend,
      { "channelFlagSuspend", "rdp.channelFlag.suspend",
        FT_UINT32, BASE_HEX, NULL, CHANNEL_FLAG_SUSPEND,
        NULL, HFILL }},
    { &hf_rdp_channelFlagResume,
      { "channelFlagResume", "rdp.channelFlag.resume",
        FT_UINT32, BASE_HEX, NULL, CHANNEL_FLAG_RESUME,
        NULL, HFILL }},
    { &hf_rdp_channelPacketCompressed,
      { "channelPacketCompressed", "rdp.channelPacket.compressed",
        FT_UINT32, BASE_HEX, NULL, CHANNEL_PACKET_COMPRESSED,
        NULL, HFILL }},
    { &hf_rdp_channelPacketAtFront,
      { "channelPacketAtFront", "rdp.channelPacket.atFront",
        FT_UINT32, BASE_HEX, NULL, CHANNEL_PACKET_AT_FRONT,
        NULL, HFILL }},
    { &hf_rdp_channelPacketFlushed,
      { "channelPacketFlushed", "rdp.channelPacket.flushed",
        FT_UINT32, BASE_HEX, NULL, CHANNEL_PACKET_FLUSHED,
        NULL, HFILL }},
    { &hf_rdp_channelPacketCompressionType,
      { "channelPacketCompresssionType", "rdp.channelPacket.compressionType",
        FT_UINT32, BASE_HEX, VALS(rdp_channelCompressionType_vals), ChannelCompressionTypeMask,
        NULL, HFILL }},
    { &hf_rdp_wYear,
      { "wYear", "rdp.wYear",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_wMonth,
      { "wMonth", "rdp.wMonth",
        FT_UINT16, BASE_DEC, VALS(rdp_wMonth_vals), 0,
        NULL, HFILL }},
    { &hf_rdp_wDayOfWeek,
      { "wDayOfWeek", "rdp.wDayOfWeek",
        FT_UINT16, BASE_DEC, VALS(rdp_wDayOfWeek_vals), 0,
        NULL, HFILL }},
    { &hf_rdp_wDay,
      { "wDay", "rdp.wDay",
        FT_UINT16, BASE_DEC, VALS(rdp_wDay_vals), 0,
        NULL, HFILL }},
    { &hf_rdp_wHour,
      { "wHour", "rdp.wHour",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_wMinute,
      { "wMinute", "rdp.wMinute",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_wSecond,
      { "wSecond", "rdp.wSecond",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_wMilliseconds,
      { "wMilliseconds", "rdp.wMilliseconds",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_Bias,
      { "Bias", "rdp.Bias",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_StandardBias,
      { "StandardBias", "rdp.Bias.standard",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_DaylightBias,
      { "DaylightBias", "rdp.Bias.daylight",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_StandardName,
      { "StandardName", "rdp.Name.Standard",
        FT_STRINGZ, BASE_NONE, NULL, 0,      /* zero-padded, not null-terminated */
        NULL, HFILL }},
    { &hf_rdp_StandardDate,
      { "StandardDate", "rdp.Date.Standard",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_DaylightName,
      { "DaylightName", "rdp.Name.Daylight",
        FT_STRINGZ, BASE_NONE, NULL, 0,      /* zero-padded, not null-terminated */
        NULL, HFILL }},
    { &hf_rdp_DaylightDate,
      { "DaylightDate", "rdp.Date.Daylight",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_rdp_unused,
      { "Unused", "rdp.unused",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
  };

  /* List of subtrees */
  static gint *ett[] = {
    &ett_rdp,
    &ett_negReq_flags,
    &ett_requestedProtocols,
    &ett_negRsp_flags,
    &ett_selectedProtocol,
    &ett_rdp_ClientData,
    &ett_rdp_ServerData,
    &ett_rdp_SendData,
    &ett_rdp_capabilitySet,
    &ett_rdp_channelDef,
    &ett_rdp_channelDefArray,
    &ett_rdp_channelFlags,
    &ett_rdp_channelIdArray,
    &ett_rdp_channelPDUHeader,
    &ett_rdp_clientClusterData,
    &ett_rdp_clientCoreData,
    &ett_rdp_clientInfoPDU,
    &ett_rdp_clientMonitorData,
    &ett_rdp_clientMonitorExData,
    &ett_rdp_clientMsgChannelData,
    &ett_rdp_clientMultiTransportData,
    &ett_rdp_clientNetworkData,
    &ett_rdp_clientSecurityData,
    &ett_rdp_clientUnknownData,
    &ett_rdp_compressedType,
    &ett_rdp_flags,
    &ett_rdp_mapFlags,
    &ett_rdp_options,
    &ett_rdp_pduType,
    &ett_rdp_securityExchangePDU,
    &ett_rdp_serverCoreData,
    &ett_rdp_serverMsgChannelData,
    &ett_rdp_serverMultiTransportData,
    &ett_rdp_serverNetworkData,
    &ett_rdp_serverSecurityData,
    &ett_rdp_serverUnknownData,
    &ett_rdp_shareControlHeader,
    &ett_rdp_validClientLicenseData,
    &ett_rdp_StandardDate,
    &ett_rdp_DaylightDate,
    &ett_rdp_clientTimeZone,
  };
  static ei_register_info ei[] = {
     { &ei_rdp_neg_len_invalid, { "rdp.neg_len.invalid", PI_PROTOCOL, PI_ERROR, "Invalid length", EXPFILL }},
     { &ei_rdp_not_correlation_info, { "rdp.not_correlation_info", PI_PROTOCOL, PI_ERROR, "What follows RDP Negotiation Request is not an RDP Correlation Info", EXPFILL }},
  };
  module_t *rdp_module;
  expert_module_t* expert_rdp;

  /* Register protocol */
  proto_rdp = proto_register_protocol(PNAME, PSNAME, PFNAME);
  /* Register fields and subtrees */
  proto_register_field_array(proto_rdp, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
  expert_rdp = expert_register_protocol(proto_rdp);
  expert_register_field_array(expert_rdp, ei, array_length(ei));

  /*   register_dissector("rdp", dissect_rdp, proto_rdp); */

  /* Register our configuration options for RDP, particularly our port */

  rdp_module = prefs_register_protocol(proto_rdp, prefs_register_rdp);

  prefs_register_uint_preference(rdp_module, "tcp.port", "RDP TCP Port",
                                 "Set the port for RDP operations (if other"
                                 " than the default of 3389)",
                                 10, &global_rdp_tcp_port);

}

void
proto_reg_handoff_rdp(void)
{

  /* remember the tpkt handler for change in preferences */
  tpkt_handle = find_dissector("tpkt");

  heur_dissector_add("cotp_cr", dissect_rdp_cr, "RDP", "rdp_cr", proto_rdp, HEURISTIC_ENABLE);
  heur_dissector_add("cotp_cc", dissect_rdp_cc, "RDP", "rdp_cc", proto_rdp, HEURISTIC_ENABLE);

  prefs_register_rdp();

  register_t124_ns_dissector("Duca", dissect_rdp_ClientData, proto_rdp);
  register_t124_ns_dissector("McDn", dissect_rdp_ServerData, proto_rdp);
}

static void
prefs_register_rdp(void) {

  static guint tcp_port = 0;

  /* de-register the old port */
  /* port 102 is registered by TPKT - don't undo this! */
  if ((tcp_port > 0) && (tcp_port != 102) && tpkt_handle)
    dissector_delete_uint("tcp.port", tcp_port, tpkt_handle);

  /* Set our port number for future use */
  tcp_port = global_rdp_tcp_port;

  if ((tcp_port > 0) && (tcp_port != 102) && tpkt_handle)
    dissector_add_uint("tcp.port", tcp_port, tpkt_handle);
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local Variables:
 * c-basic-offset: 2
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=2 tabstop=8 expandtab:
 * :indentSize=2:tabSize=8:noTabs=true:
 */
