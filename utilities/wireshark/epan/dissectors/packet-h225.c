/* Do not modify this file. Changes will be overwritten.                      */
/* Generated automatically by the ASN.1 to Wireshark dissector compiler       */
/* packet-h225.c                                                              */
/* asn2wrs.py -p h225 -c ./h225.cnf -s ./packet-h225-template -D . -O ../.. H323-MESSAGES.asn */

/* Input file: packet-h225-template.c */

#line 1 "./asn1/h225/packet-h225-template.c"
/* packet-h225.c
 * Routines for h225 packet dissection
 * Copyright 2005, Anders Broman <anders.broman@ericsson.com>
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
 *
 * To quote the author of the previous H323/H225/H245 dissector:
 *   "This is a complete replacement of the previous limitied dissector
 * that Ronnie was crazy enough to write by hand. It was a lot of time
 * to hack it by hand, but it is incomplete and buggy and it is good when
 * it will go away."
 * Ronnie did a great job and all the VoIP users had made good use of it!
 * Credit to Tomas Kukosa for developing the asn2wrs compiler.
 *
 */

#include "config.h"

#include <epan/packet.h>
#include <epan/conversation.h>
#include <epan/proto_data.h>

#include <epan/prefs.h>
#include <epan/oids.h>
#include <epan/next_tvb.h>
#include <epan/asn1.h>
#include <epan/t35.h>
#include <epan/tap.h>
#include <epan/stat_tap_ui.h>
#include <epan/rtd_table.h>
#include "packet-tpkt.h"
#include "packet-per.h"
#include "packet-h225.h"
#include "packet-h235.h"
#include "packet-h245.h"
#include "packet-h323.h"
#include "packet-q931.h"
#include "packet-ssl.h"

#define PNAME  "H323-MESSAGES"
#define PSNAME "H.225.0"
#define PFNAME "h225"

#define UDP_PORT_RAS1 1718
#define UDP_PORT_RAS2 1719
#define TCP_PORT_CS   1720
#define TLS_PORT_CS   1300

void proto_register_h225(void);
static h225_packet_info* create_h225_packet_info(packet_info *pinfo);
static void h225_init_routine(void);
static void ras_call_matching(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, h225_packet_info *pi);

/* Item of ras request list*/
typedef struct _h225ras_call_t {
  guint32 requestSeqNum;
  e_guid_t guid;
  guint32 req_num;  /* frame number request seen */
  guint32 rsp_num;  /* frame number response seen */
  nstime_t req_time;  /* arrival time of request */
  gboolean responded; /* true, if request has been responded */
  struct _h225ras_call_t *next_call; /* pointer to next ras request with same SequenceNumber and conversation handle */
} h225ras_call_t;


/* Item of ras-request key list*/
typedef struct _h225ras_call_info_key {
  guint reqSeqNum;
  conversation_t *conversation;
} h225ras_call_info_key;

/* Global Memory Chunks for lists and Global hash tables*/

static GHashTable *ras_calls[7] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};

/* functions, needed using ras-request and halfcall matching*/
static h225ras_call_t * find_h225ras_call(h225ras_call_info_key *h225ras_call_key ,int category);
static h225ras_call_t * new_h225ras_call(h225ras_call_info_key *h225ras_call_key, packet_info *pinfo, e_guid_t *guid, int category);
static h225ras_call_t * append_h225ras_call(h225ras_call_t *prev_call, packet_info *pinfo, e_guid_t *guid, int category);


static dissector_handle_t h225ras_handle;
static dissector_handle_t data_handle;
/* Subdissector tables */
static dissector_table_t nsp_object_dissector_table;
static dissector_table_t nsp_h221_dissector_table;
static dissector_table_t tp_dissector_table;
static dissector_table_t gef_name_dissector_table;
static dissector_table_t gef_content_dissector_table;


static dissector_handle_t h245_handle=NULL;
static dissector_handle_t h245dg_handle=NULL;
static dissector_handle_t h4501_handle=NULL;

static dissector_handle_t nsp_handle;
static dissector_handle_t tp_handle;

static next_tvb_list_t h245_list;
static next_tvb_list_t tp_list;

/* Initialize the protocol and registered fields */
static int h225_tap = -1;
static int proto_h225 = -1;

static int hf_h221Manufacturer = -1;
static int hf_h225_ras_req_frame = -1;
static int hf_h225_ras_rsp_frame = -1;
static int hf_h225_ras_dup = -1;
static int hf_h225_ras_deltatime = -1;
static int hf_h225_debug_dissector_try_string = -1;


/*--- Included file: packet-h225-hf.c ---*/
#line 1 "./asn1/h225/packet-h225-hf.c"
static int hf_h225_H323_UserInformation_PDU = -1;  /* H323_UserInformation */
static int hf_h225_h225_ExtendedAliasAddress_PDU = -1;  /* ExtendedAliasAddress */
static int hf_h225_RasMessage_PDU = -1;           /* RasMessage */
static int hf_h225_h323_uu_pdu = -1;              /* H323_UU_PDU */
static int hf_h225_user_data = -1;                /* T_user_data */
static int hf_h225_protocol_discriminator = -1;   /* INTEGER_0_255 */
static int hf_h225_user_information = -1;         /* OCTET_STRING_SIZE_1_131 */
static int hf_h225_h323_message_body = -1;        /* T_h323_message_body */
static int hf_h225_setup = -1;                    /* Setup_UUIE */
static int hf_h225_callProceeding = -1;           /* CallProceeding_UUIE */
static int hf_h225_connect = -1;                  /* Connect_UUIE */
static int hf_h225_alerting = -1;                 /* Alerting_UUIE */
static int hf_h225_information = -1;              /* Information_UUIE */
static int hf_h225_releaseComplete = -1;          /* ReleaseComplete_UUIE */
static int hf_h225_facility = -1;                 /* Facility_UUIE */
static int hf_h225_progress = -1;                 /* Progress_UUIE */
static int hf_h225_empty_flg = -1;                /* T_empty_flg */
static int hf_h225_status = -1;                   /* Status_UUIE */
static int hf_h225_statusInquiry = -1;            /* StatusInquiry_UUIE */
static int hf_h225_setupAcknowledge = -1;         /* SetupAcknowledge_UUIE */
static int hf_h225_notify = -1;                   /* Notify_UUIE */
static int hf_h225_nonStandardData = -1;          /* NonStandardParameter */
static int hf_h225_h4501SupplementaryService = -1;  /* T_h4501SupplementaryService */
static int hf_h225_h4501SupplementaryService_item = -1;  /* T_h4501SupplementaryService_item */
static int hf_h225_h245Tunnelling = -1;           /* T_h245Tunnelling */
static int hf_h225_H245Control_item = -1;         /* H245Control_item */
static int hf_h225_h245Control = -1;              /* H245Control */
static int hf_h225_nonStandardControl = -1;       /* SEQUENCE_OF_NonStandardParameter */
static int hf_h225_nonStandardControl_item = -1;  /* NonStandardParameter */
static int hf_h225_callLinkage = -1;              /* CallLinkage */
static int hf_h225_tunnelledSignallingMessage = -1;  /* T_tunnelledSignallingMessage */
static int hf_h225_tunnelledProtocolID = -1;      /* TunnelledProtocol */
static int hf_h225_messageContent = -1;           /* T_messageContent */
static int hf_h225_messageContent_item = -1;      /* T_messageContent_item */
static int hf_h225_tunnellingRequired = -1;       /* NULL */
static int hf_h225_provisionalRespToH245Tunnelling = -1;  /* NULL */
static int hf_h225_stimulusControl = -1;          /* StimulusControl */
static int hf_h225_genericData = -1;              /* SEQUENCE_OF_GenericData */
static int hf_h225_genericData_item = -1;         /* GenericData */
static int hf_h225_nonStandard = -1;              /* NonStandardParameter */
static int hf_h225_isText = -1;                   /* NULL */
static int hf_h225_h248Message = -1;              /* OCTET_STRING */
static int hf_h225_protocolIdentifier = -1;       /* ProtocolIdentifier */
static int hf_h225_uUIE_destinationInfo = -1;     /* EndpointType */
static int hf_h225_h245Address = -1;              /* H245TransportAddress */
static int hf_h225_callIdentifier = -1;           /* CallIdentifier */
static int hf_h225_h245SecurityMode = -1;         /* H245Security */
static int hf_h225_tokens = -1;                   /* SEQUENCE_OF_ClearToken */
static int hf_h225_tokens_item = -1;              /* ClearToken */
static int hf_h225_cryptoTokens = -1;             /* SEQUENCE_OF_CryptoH323Token */
static int hf_h225_cryptoTokens_item = -1;        /* CryptoH323Token */
static int hf_h225_fastStart = -1;                /* FastStart */
static int hf_h225_multipleCalls = -1;            /* BOOLEAN */
static int hf_h225_maintainConnection = -1;       /* BOOLEAN */
static int hf_h225_alertingAddress = -1;          /* SEQUENCE_OF_AliasAddress */
static int hf_h225_alertingAddress_item = -1;     /* AliasAddress */
static int hf_h225_presentationIndicator = -1;    /* PresentationIndicator */
static int hf_h225_screeningIndicator = -1;       /* ScreeningIndicator */
static int hf_h225_fastConnectRefused = -1;       /* NULL */
static int hf_h225_serviceControl = -1;           /* SEQUENCE_OF_ServiceControlSession */
static int hf_h225_serviceControl_item = -1;      /* ServiceControlSession */
static int hf_h225_capacity = -1;                 /* CallCapacity */
static int hf_h225_featureSet = -1;               /* FeatureSet */
static int hf_h225_displayName = -1;              /* SEQUENCE_OF_DisplayName */
static int hf_h225_displayName_item = -1;         /* DisplayName */
static int hf_h225_conferenceID = -1;             /* ConferenceIdentifier */
static int hf_h225_language = -1;                 /* Language */
static int hf_h225_connectedAddress = -1;         /* SEQUENCE_OF_AliasAddress */
static int hf_h225_connectedAddress_item = -1;    /* AliasAddress */
static int hf_h225_circuitInfo = -1;              /* CircuitInfo */
static int hf_h225_releaseCompleteReason = -1;    /* ReleaseCompleteReason */
static int hf_h225_busyAddress = -1;              /* SEQUENCE_OF_AliasAddress */
static int hf_h225_busyAddress_item = -1;         /* AliasAddress */
static int hf_h225_destinationInfo = -1;          /* EndpointType */
static int hf_h225_noBandwidth = -1;              /* NULL */
static int hf_h225_gatekeeperResources = -1;      /* NULL */
static int hf_h225_unreachableDestination = -1;   /* NULL */
static int hf_h225_destinationRejection = -1;     /* NULL */
static int hf_h225_invalidRevision = -1;          /* NULL */
static int hf_h225_noPermission = -1;             /* NULL */
static int hf_h225_unreachableGatekeeper = -1;    /* NULL */
static int hf_h225_gatewayResources = -1;         /* NULL */
static int hf_h225_badFormatAddress = -1;         /* NULL */
static int hf_h225_adaptiveBusy = -1;             /* NULL */
static int hf_h225_inConf = -1;                   /* NULL */
static int hf_h225_undefinedReason = -1;          /* NULL */
static int hf_h225_facilityCallDeflection = -1;   /* NULL */
static int hf_h225_securityDenied = -1;           /* NULL */
static int hf_h225_calledPartyNotRegistered = -1;  /* NULL */
static int hf_h225_callerNotRegistered = -1;      /* NULL */
static int hf_h225_newConnectionNeeded = -1;      /* NULL */
static int hf_h225_nonStandardReason = -1;        /* NonStandardParameter */
static int hf_h225_replaceWithConferenceInvite = -1;  /* ConferenceIdentifier */
static int hf_h225_genericDataReason = -1;        /* NULL */
static int hf_h225_neededFeatureNotSupported = -1;  /* NULL */
static int hf_h225_tunnelledSignallingRejected = -1;  /* NULL */
static int hf_h225_invalidCID = -1;               /* NULL */
static int hf_h225_rLC_securityError = -1;        /* SecurityErrors */
static int hf_h225_hopCountExceeded = -1;         /* NULL */
static int hf_h225_sourceAddress = -1;            /* SEQUENCE_OF_AliasAddress */
static int hf_h225_sourceAddress_item = -1;       /* AliasAddress */
static int hf_h225_setup_UUIE_sourceInfo = -1;    /* EndpointType */
static int hf_h225_destinationAddress = -1;       /* SEQUENCE_OF_AliasAddress */
static int hf_h225_destinationAddress_item = -1;  /* AliasAddress */
static int hf_h225_destCallSignalAddress = -1;    /* TransportAddress */
static int hf_h225_destExtraCallInfo = -1;        /* SEQUENCE_OF_AliasAddress */
static int hf_h225_destExtraCallInfo_item = -1;   /* AliasAddress */
static int hf_h225_destExtraCRV = -1;             /* SEQUENCE_OF_CallReferenceValue */
static int hf_h225_destExtraCRV_item = -1;        /* CallReferenceValue */
static int hf_h225_activeMC = -1;                 /* BOOLEAN */
static int hf_h225_conferenceGoal = -1;           /* T_conferenceGoal */
static int hf_h225_create = -1;                   /* NULL */
static int hf_h225_join = -1;                     /* NULL */
static int hf_h225_invite = -1;                   /* NULL */
static int hf_h225_capability_negotiation = -1;   /* NULL */
static int hf_h225_callIndependentSupplementaryService = -1;  /* NULL */
static int hf_h225_callServices = -1;             /* QseriesOptions */
static int hf_h225_callType = -1;                 /* CallType */
static int hf_h225_sourceCallSignalAddress = -1;  /* TransportAddress */
static int hf_h225_uUIE_remoteExtensionAddress = -1;  /* AliasAddress */
static int hf_h225_h245SecurityCapability = -1;   /* SEQUENCE_OF_H245Security */
static int hf_h225_h245SecurityCapability_item = -1;  /* H245Security */
static int hf_h225_FastStart_item = -1;           /* FastStart_item */
static int hf_h225_mediaWaitForConnect = -1;      /* BOOLEAN */
static int hf_h225_canOverlapSend = -1;           /* BOOLEAN */
static int hf_h225_endpointIdentifier = -1;       /* EndpointIdentifier */
static int hf_h225_connectionParameters = -1;     /* T_connectionParameters */
static int hf_h225_connectionType = -1;           /* ScnConnectionType */
static int hf_h225_numberOfScnConnections = -1;   /* INTEGER_0_65535 */
static int hf_h225_connectionAggregation = -1;    /* ScnConnectionAggregation */
static int hf_h225_Language_item = -1;            /* IA5String_SIZE_1_32 */
static int hf_h225_symmetricOperationRequired = -1;  /* NULL */
static int hf_h225_desiredProtocols = -1;         /* SEQUENCE_OF_SupportedProtocols */
static int hf_h225_desiredProtocols_item = -1;    /* SupportedProtocols */
static int hf_h225_neededFeatures = -1;           /* SEQUENCE_OF_FeatureDescriptor */
static int hf_h225_neededFeatures_item = -1;      /* FeatureDescriptor */
static int hf_h225_desiredFeatures = -1;          /* SEQUENCE_OF_FeatureDescriptor */
static int hf_h225_desiredFeatures_item = -1;     /* FeatureDescriptor */
static int hf_h225_supportedFeatures = -1;        /* SEQUENCE_OF_FeatureDescriptor */
static int hf_h225_supportedFeatures_item = -1;   /* FeatureDescriptor */
static int hf_h225_ParallelH245Control_item = -1;  /* ParallelH245Control_item */
static int hf_h225_parallelH245Control = -1;      /* ParallelH245Control */
static int hf_h225_additionalSourceAddresses = -1;  /* SEQUENCE_OF_ExtendedAliasAddress */
static int hf_h225_additionalSourceAddresses_item = -1;  /* ExtendedAliasAddress */
static int hf_h225_hopCount_1_31 = -1;            /* INTEGER_1_31 */
static int hf_h225_unknown = -1;                  /* NULL */
static int hf_h225_bChannel = -1;                 /* NULL */
static int hf_h225_hybrid2x64 = -1;               /* NULL */
static int hf_h225_hybrid384 = -1;                /* NULL */
static int hf_h225_hybrid1536 = -1;               /* NULL */
static int hf_h225_hybrid1920 = -1;               /* NULL */
static int hf_h225_multirate = -1;                /* NULL */
static int hf_h225_auto = -1;                     /* NULL */
static int hf_h225_none = -1;                     /* NULL */
static int hf_h225_h221 = -1;                     /* NULL */
static int hf_h225_bonded_mode1 = -1;             /* NULL */
static int hf_h225_bonded_mode2 = -1;             /* NULL */
static int hf_h225_bonded_mode3 = -1;             /* NULL */
static int hf_h225_presentationAllowed = -1;      /* NULL */
static int hf_h225_presentationRestricted = -1;   /* NULL */
static int hf_h225_addressNotAvailable = -1;      /* NULL */
static int hf_h225_alternativeAddress = -1;       /* TransportAddress */
static int hf_h225_alternativeAliasAddress = -1;  /* SEQUENCE_OF_AliasAddress */
static int hf_h225_alternativeAliasAddress_item = -1;  /* AliasAddress */
static int hf_h225_facilityReason = -1;           /* FacilityReason */
static int hf_h225_conferences = -1;              /* SEQUENCE_OF_ConferenceList */
static int hf_h225_conferences_item = -1;         /* ConferenceList */
static int hf_h225_conferenceAlias = -1;          /* AliasAddress */
static int hf_h225_routeCallToGatekeeper = -1;    /* NULL */
static int hf_h225_callForwarded = -1;            /* NULL */
static int hf_h225_routeCallToMC = -1;            /* NULL */
static int hf_h225_conferenceListChoice = -1;     /* NULL */
static int hf_h225_startH245 = -1;                /* NULL */
static int hf_h225_noH245 = -1;                   /* NULL */
static int hf_h225_newTokens = -1;                /* NULL */
static int hf_h225_featureSetUpdate = -1;         /* NULL */
static int hf_h225_forwardedElements = -1;        /* NULL */
static int hf_h225_transportedInformation = -1;   /* NULL */
static int hf_h225_h245IpAddress = -1;            /* T_h245IpAddress */
static int hf_h225_h245Ip = -1;                   /* T_h245Ip */
static int hf_h225_h245IpPort = -1;               /* T_h245IpPort */
static int hf_h225_h245IpSourceRoute = -1;        /* T_h245IpSourceRoute */
static int hf_h225_ip = -1;                       /* OCTET_STRING_SIZE_4 */
static int hf_h225_port = -1;                     /* INTEGER_0_65535 */
static int hf_h225_h245Route = -1;                /* T_h245Route */
static int hf_h225_h245Route_item = -1;           /* OCTET_STRING_SIZE_4 */
static int hf_h225_h245Routing = -1;              /* T_h245Routing */
static int hf_h225_strict = -1;                   /* NULL */
static int hf_h225_loose = -1;                    /* NULL */
static int hf_h225_h245IpxAddress = -1;           /* T_h245IpxAddress */
static int hf_h225_node = -1;                     /* OCTET_STRING_SIZE_6 */
static int hf_h225_netnum = -1;                   /* OCTET_STRING_SIZE_4 */
static int hf_h225_h245IpxPort = -1;              /* OCTET_STRING_SIZE_2 */
static int hf_h225_h245Ip6Address = -1;           /* T_h245Ip6Address */
static int hf_h225_h245Ip6 = -1;                  /* T_h245Ip6 */
static int hf_h225_port_01 = -1;                  /* T_port */
static int hf_h225_netBios = -1;                  /* OCTET_STRING_SIZE_16 */
static int hf_h225_nsap = -1;                     /* OCTET_STRING_SIZE_1_20 */
static int hf_h225_nonStandardAddress = -1;       /* NonStandardParameter */
static int hf_h225_ipAddress = -1;                /* T_ipAddress */
static int hf_h225_ipV4 = -1;                     /* IpV4 */
static int hf_h225_ipV4_port = -1;                /* INTEGER_0_65535 */
static int hf_h225_ipSourceRoute = -1;            /* T_ipSourceRoute */
static int hf_h225_src_route_ipV4 = -1;           /* OCTET_STRING_SIZE_4 */
static int hf_h225_ipV4_src_port = -1;            /* INTEGER_0_65535 */
static int hf_h225_route = -1;                    /* T_route */
static int hf_h225_route_item = -1;               /* OCTET_STRING_SIZE_4 */
static int hf_h225_routing = -1;                  /* T_routing */
static int hf_h225_ipxAddress = -1;               /* T_ipxAddress */
static int hf_h225_ipx_port = -1;                 /* OCTET_STRING_SIZE_2 */
static int hf_h225_ip6Address = -1;               /* T_ip6Address */
static int hf_h225_ipV6 = -1;                     /* OCTET_STRING_SIZE_16 */
static int hf_h225_ipV6_port = -1;                /* INTEGER_0_65535 */
static int hf_h225_vendor = -1;                   /* VendorIdentifier */
static int hf_h225_gatekeeper = -1;               /* GatekeeperInfo */
static int hf_h225_gateway = -1;                  /* GatewayInfo */
static int hf_h225_mcu = -1;                      /* McuInfo */
static int hf_h225_terminal = -1;                 /* TerminalInfo */
static int hf_h225_mc = -1;                       /* BOOLEAN */
static int hf_h225_undefinedNode = -1;            /* BOOLEAN */
static int hf_h225_set = -1;                      /* BIT_STRING_SIZE_32 */
static int hf_h225_supportedTunnelledProtocols = -1;  /* SEQUENCE_OF_TunnelledProtocol */
static int hf_h225_supportedTunnelledProtocols_item = -1;  /* TunnelledProtocol */
static int hf_h225_protocol = -1;                 /* SEQUENCE_OF_SupportedProtocols */
static int hf_h225_protocol_item = -1;            /* SupportedProtocols */
static int hf_h225_h310 = -1;                     /* H310Caps */
static int hf_h225_h320 = -1;                     /* H320Caps */
static int hf_h225_h321 = -1;                     /* H321Caps */
static int hf_h225_h322 = -1;                     /* H322Caps */
static int hf_h225_h323 = -1;                     /* H323Caps */
static int hf_h225_h324 = -1;                     /* H324Caps */
static int hf_h225_voice = -1;                    /* VoiceCaps */
static int hf_h225_t120_only = -1;                /* T120OnlyCaps */
static int hf_h225_nonStandardProtocol = -1;      /* NonStandardProtocol */
static int hf_h225_t38FaxAnnexbOnly = -1;         /* T38FaxAnnexbOnlyCaps */
static int hf_h225_sip = -1;                      /* SIPCaps */
static int hf_h225_dataRatesSupported = -1;       /* SEQUENCE_OF_DataRate */
static int hf_h225_dataRatesSupported_item = -1;  /* DataRate */
static int hf_h225_supportedPrefixes = -1;        /* SEQUENCE_OF_SupportedPrefix */
static int hf_h225_supportedPrefixes_item = -1;   /* SupportedPrefix */
static int hf_h225_t38FaxProtocol = -1;           /* DataProtocolCapability */
static int hf_h225_t38FaxProfile = -1;            /* T38FaxProfile */
static int hf_h225_vendorIdentifier_vendor = -1;  /* H221NonStandard */
static int hf_h225_productId = -1;                /* OCTET_STRING_SIZE_1_256 */
static int hf_h225_versionId = -1;                /* OCTET_STRING_SIZE_1_256 */
static int hf_h225_enterpriseNumber = -1;         /* OBJECT_IDENTIFIER */
static int hf_h225_t35CountryCode = -1;           /* T_t35CountryCode */
static int hf_h225_t35Extension = -1;             /* T_t35Extension */
static int hf_h225_manufacturerCode = -1;         /* T_manufacturerCode */
static int hf_h225_tunnelledProtocol_id = -1;     /* TunnelledProtocol_id */
static int hf_h225_tunnelledProtocolObjectID = -1;  /* T_tunnelledProtocolObjectID */
static int hf_h225_tunnelledProtocolAlternateID = -1;  /* TunnelledProtocolAlternateIdentifier */
static int hf_h225_subIdentifier = -1;            /* IA5String_SIZE_1_64 */
static int hf_h225_protocolType = -1;             /* IA5String_SIZE_1_64 */
static int hf_h225_protocolVariant = -1;          /* IA5String_SIZE_1_64 */
static int hf_h225_nonStandardIdentifier = -1;    /* NonStandardIdentifier */
static int hf_h225_nsp_data = -1;                 /* T_nsp_data */
static int hf_h225_nsiOID = -1;                   /* T_nsiOID */
static int hf_h225_h221NonStandard = -1;          /* H221NonStandard */
static int hf_h225_dialledDigits = -1;            /* DialedDigits */
static int hf_h225_h323_ID = -1;                  /* BMPString_SIZE_1_256 */
static int hf_h225_url_ID = -1;                   /* IA5String_SIZE_1_512 */
static int hf_h225_transportID = -1;              /* TransportAddress */
static int hf_h225_email_ID = -1;                 /* IA5String_SIZE_1_512 */
static int hf_h225_partyNumber = -1;              /* PartyNumber */
static int hf_h225_mobileUIM = -1;                /* MobileUIM */
static int hf_h225_isupNumber = -1;               /* IsupNumber */
static int hf_h225_wildcard = -1;                 /* AliasAddress */
static int hf_h225_range = -1;                    /* T_range */
static int hf_h225_startOfRange = -1;             /* PartyNumber */
static int hf_h225_endOfRange = -1;               /* PartyNumber */
static int hf_h225_e164Number = -1;               /* PublicPartyNumber */
static int hf_h225_dataPartyNumber = -1;          /* NumberDigits */
static int hf_h225_telexPartyNumber = -1;         /* NumberDigits */
static int hf_h225_privateNumber = -1;            /* PrivatePartyNumber */
static int hf_h225_nationalStandardPartyNumber = -1;  /* NumberDigits */
static int hf_h225_publicTypeOfNumber = -1;       /* PublicTypeOfNumber */
static int hf_h225_publicNumberDigits = -1;       /* NumberDigits */
static int hf_h225_privateTypeOfNumber = -1;      /* PrivateTypeOfNumber */
static int hf_h225_privateNumberDigits = -1;      /* NumberDigits */
static int hf_h225_language_01 = -1;              /* IA5String */
static int hf_h225_name = -1;                     /* BMPString_SIZE_1_80 */
static int hf_h225_internationalNumber = -1;      /* NULL */
static int hf_h225_nationalNumber = -1;           /* NULL */
static int hf_h225_networkSpecificNumber = -1;    /* NULL */
static int hf_h225_subscriberNumber = -1;         /* NULL */
static int hf_h225_abbreviatedNumber = -1;        /* NULL */
static int hf_h225_level2RegionalNumber = -1;     /* NULL */
static int hf_h225_level1RegionalNumber = -1;     /* NULL */
static int hf_h225_pISNSpecificNumber = -1;       /* NULL */
static int hf_h225_localNumber = -1;              /* NULL */
static int hf_h225_ansi_41_uim = -1;              /* ANSI_41_UIM */
static int hf_h225_gsm_uim = -1;                  /* GSM_UIM */
static int hf_h225_imsi = -1;                     /* TBCD_STRING_SIZE_3_16 */
static int hf_h225_min = -1;                      /* TBCD_STRING_SIZE_3_16 */
static int hf_h225_mdn = -1;                      /* TBCD_STRING_SIZE_3_16 */
static int hf_h225_msisdn = -1;                   /* TBCD_STRING_SIZE_3_16 */
static int hf_h225_esn = -1;                      /* TBCD_STRING_SIZE_16 */
static int hf_h225_mscid = -1;                    /* TBCD_STRING_SIZE_3_16 */
static int hf_h225_system_id = -1;                /* T_system_id */
static int hf_h225_sid = -1;                      /* TBCD_STRING_SIZE_1_4 */
static int hf_h225_mid = -1;                      /* TBCD_STRING_SIZE_1_4 */
static int hf_h225_systemMyTypeCode = -1;         /* OCTET_STRING_SIZE_1 */
static int hf_h225_systemAccessType = -1;         /* OCTET_STRING_SIZE_1 */
static int hf_h225_qualificationInformationCode = -1;  /* OCTET_STRING_SIZE_1 */
static int hf_h225_sesn = -1;                     /* TBCD_STRING_SIZE_16 */
static int hf_h225_soc = -1;                      /* TBCD_STRING_SIZE_3_16 */
static int hf_h225_tmsi = -1;                     /* OCTET_STRING_SIZE_1_4 */
static int hf_h225_imei = -1;                     /* TBCD_STRING_SIZE_15_16 */
static int hf_h225_hplmn = -1;                    /* TBCD_STRING_SIZE_1_4 */
static int hf_h225_vplmn = -1;                    /* TBCD_STRING_SIZE_1_4 */
static int hf_h225_isupE164Number = -1;           /* IsupPublicPartyNumber */
static int hf_h225_isupDataPartyNumber = -1;      /* IsupDigits */
static int hf_h225_isupTelexPartyNumber = -1;     /* IsupDigits */
static int hf_h225_isupPrivateNumber = -1;        /* IsupPrivatePartyNumber */
static int hf_h225_isupNationalStandardPartyNumber = -1;  /* IsupDigits */
static int hf_h225_natureOfAddress = -1;          /* NatureOfAddress */
static int hf_h225_address = -1;                  /* IsupDigits */
static int hf_h225_routingNumberNationalFormat = -1;  /* NULL */
static int hf_h225_routingNumberNetworkSpecificFormat = -1;  /* NULL */
static int hf_h225_routingNumberWithCalledDirectoryNumber = -1;  /* NULL */
static int hf_h225_extAliasAddress = -1;          /* AliasAddress */
static int hf_h225_aliasAddress = -1;             /* SEQUENCE_OF_AliasAddress */
static int hf_h225_aliasAddress_item = -1;        /* AliasAddress */
static int hf_h225_callSignalAddress = -1;        /* SEQUENCE_OF_TransportAddress */
static int hf_h225_callSignalAddress_item = -1;   /* TransportAddress */
static int hf_h225_rasAddress = -1;               /* SEQUENCE_OF_TransportAddress */
static int hf_h225_rasAddress_item = -1;          /* TransportAddress */
static int hf_h225_endpointType = -1;             /* EndpointType */
static int hf_h225_priority = -1;                 /* INTEGER_0_127 */
static int hf_h225_remoteExtensionAddress = -1;   /* SEQUENCE_OF_AliasAddress */
static int hf_h225_remoteExtensionAddress_item = -1;  /* AliasAddress */
static int hf_h225_alternateTransportAddresses = -1;  /* AlternateTransportAddresses */
static int hf_h225_annexE = -1;                   /* SEQUENCE_OF_TransportAddress */
static int hf_h225_annexE_item = -1;              /* TransportAddress */
static int hf_h225_sctp = -1;                     /* SEQUENCE_OF_TransportAddress */
static int hf_h225_sctp_item = -1;                /* TransportAddress */
static int hf_h225_tcp = -1;                      /* NULL */
static int hf_h225_annexE_flg = -1;               /* NULL */
static int hf_h225_sctp_flg = -1;                 /* NULL */
static int hf_h225_alternateGK_rasAddress = -1;   /* TransportAddress */
static int hf_h225_gatekeeperIdentifier = -1;     /* GatekeeperIdentifier */
static int hf_h225_needToRegister = -1;           /* BOOLEAN */
static int hf_h225_alternateGatekeeper = -1;      /* SEQUENCE_OF_AlternateGK */
static int hf_h225_alternateGatekeeper_item = -1;  /* AlternateGK */
static int hf_h225_altGKisPermanent = -1;         /* BOOLEAN */
static int hf_h225_default = -1;                  /* NULL */
static int hf_h225_encryption = -1;               /* SecurityServiceMode */
static int hf_h225_authenticaton = -1;            /* SecurityServiceMode */
static int hf_h225_securityCapabilities_integrity = -1;  /* SecurityServiceMode */
static int hf_h225_securityWrongSyncTime = -1;    /* NULL */
static int hf_h225_securityReplay = -1;           /* NULL */
static int hf_h225_securityWrongGeneralID = -1;   /* NULL */
static int hf_h225_securityWrongSendersID = -1;   /* NULL */
static int hf_h225_securityIntegrityFailed = -1;  /* NULL */
static int hf_h225_securityWrongOID = -1;         /* NULL */
static int hf_h225_securityDHmismatch = -1;       /* NULL */
static int hf_h225_securityCertificateExpired = -1;  /* NULL */
static int hf_h225_securityCertificateDateInvalid = -1;  /* NULL */
static int hf_h225_securityCertificateRevoked = -1;  /* NULL */
static int hf_h225_securityCertificateNotReadable = -1;  /* NULL */
static int hf_h225_securityCertificateSignatureInvalid = -1;  /* NULL */
static int hf_h225_securityCertificateMissing = -1;  /* NULL */
static int hf_h225_securityCertificateIncomplete = -1;  /* NULL */
static int hf_h225_securityUnsupportedCertificateAlgOID = -1;  /* NULL */
static int hf_h225_securityUnknownCA = -1;        /* NULL */
static int hf_h225_noSecurity = -1;               /* NULL */
static int hf_h225_tls = -1;                      /* SecurityCapabilities */
static int hf_h225_ipsec = -1;                    /* SecurityCapabilities */
static int hf_h225_q932Full = -1;                 /* BOOLEAN */
static int hf_h225_q951Full = -1;                 /* BOOLEAN */
static int hf_h225_q952Full = -1;                 /* BOOLEAN */
static int hf_h225_q953Full = -1;                 /* BOOLEAN */
static int hf_h225_q955Full = -1;                 /* BOOLEAN */
static int hf_h225_q956Full = -1;                 /* BOOLEAN */
static int hf_h225_q957Full = -1;                 /* BOOLEAN */
static int hf_h225_q954Info = -1;                 /* Q954Details */
static int hf_h225_conferenceCalling = -1;        /* BOOLEAN */
static int hf_h225_threePartyService = -1;        /* BOOLEAN */
static int hf_h225_guid = -1;                     /* T_guid */
static int hf_h225_isoAlgorithm = -1;             /* OBJECT_IDENTIFIER */
static int hf_h225_hMAC_MD5 = -1;                 /* NULL */
static int hf_h225_hMAC_iso10118_2_s = -1;        /* EncryptIntAlg */
static int hf_h225_hMAC_iso10118_2_l = -1;        /* EncryptIntAlg */
static int hf_h225_hMAC_iso10118_3 = -1;          /* OBJECT_IDENTIFIER */
static int hf_h225_digSig = -1;                   /* NULL */
static int hf_h225_iso9797 = -1;                  /* OBJECT_IDENTIFIER */
static int hf_h225_nonIsoIM = -1;                 /* NonIsoIntegrityMechanism */
static int hf_h225_algorithmOID = -1;             /* OBJECT_IDENTIFIER */
static int hf_h225_icv = -1;                      /* BIT_STRING */
static int hf_h225_cryptoEPPwdHash = -1;          /* T_cryptoEPPwdHash */
static int hf_h225_alias = -1;                    /* AliasAddress */
static int hf_h225_timeStamp = -1;                /* TimeStamp */
static int hf_h225_token = -1;                    /* HASHED */
static int hf_h225_cryptoGKPwdHash = -1;          /* T_cryptoGKPwdHash */
static int hf_h225_gatekeeperId = -1;             /* GatekeeperIdentifier */
static int hf_h225_cryptoEPPwdEncr = -1;          /* ENCRYPTED */
static int hf_h225_cryptoGKPwdEncr = -1;          /* ENCRYPTED */
static int hf_h225_cryptoEPCert = -1;             /* SIGNED */
static int hf_h225_cryptoGKCert = -1;             /* SIGNED */
static int hf_h225_cryptoFastStart = -1;          /* SIGNED */
static int hf_h225_nestedcryptoToken = -1;        /* CryptoToken */
static int hf_h225_channelRate = -1;              /* BandWidth */
static int hf_h225_channelMultiplier = -1;        /* INTEGER_1_256 */
static int hf_h225_globalCallId = -1;             /* GloballyUniqueID */
static int hf_h225_threadId = -1;                 /* GloballyUniqueID */
static int hf_h225_prefix = -1;                   /* AliasAddress */
static int hf_h225_canReportCallCapacity = -1;    /* BOOLEAN */
static int hf_h225_capacityReportingSpecification_when = -1;  /* CapacityReportingSpecification_when */
static int hf_h225_callStart = -1;                /* NULL */
static int hf_h225_callEnd = -1;                  /* NULL */
static int hf_h225_maximumCallCapacity = -1;      /* CallCapacityInfo */
static int hf_h225_currentCallCapacity = -1;      /* CallCapacityInfo */
static int hf_h225_voiceGwCallsAvailable = -1;    /* SEQUENCE_OF_CallsAvailable */
static int hf_h225_voiceGwCallsAvailable_item = -1;  /* CallsAvailable */
static int hf_h225_h310GwCallsAvailable = -1;     /* SEQUENCE_OF_CallsAvailable */
static int hf_h225_h310GwCallsAvailable_item = -1;  /* CallsAvailable */
static int hf_h225_h320GwCallsAvailable = -1;     /* SEQUENCE_OF_CallsAvailable */
static int hf_h225_h320GwCallsAvailable_item = -1;  /* CallsAvailable */
static int hf_h225_h321GwCallsAvailable = -1;     /* SEQUENCE_OF_CallsAvailable */
static int hf_h225_h321GwCallsAvailable_item = -1;  /* CallsAvailable */
static int hf_h225_h322GwCallsAvailable = -1;     /* SEQUENCE_OF_CallsAvailable */
static int hf_h225_h322GwCallsAvailable_item = -1;  /* CallsAvailable */
static int hf_h225_h323GwCallsAvailable = -1;     /* SEQUENCE_OF_CallsAvailable */
static int hf_h225_h323GwCallsAvailable_item = -1;  /* CallsAvailable */
static int hf_h225_h324GwCallsAvailable = -1;     /* SEQUENCE_OF_CallsAvailable */
static int hf_h225_h324GwCallsAvailable_item = -1;  /* CallsAvailable */
static int hf_h225_t120OnlyGwCallsAvailable = -1;  /* SEQUENCE_OF_CallsAvailable */
static int hf_h225_t120OnlyGwCallsAvailable_item = -1;  /* CallsAvailable */
static int hf_h225_t38FaxAnnexbOnlyGwCallsAvailable = -1;  /* SEQUENCE_OF_CallsAvailable */
static int hf_h225_t38FaxAnnexbOnlyGwCallsAvailable_item = -1;  /* CallsAvailable */
static int hf_h225_terminalCallsAvailable = -1;   /* SEQUENCE_OF_CallsAvailable */
static int hf_h225_terminalCallsAvailable_item = -1;  /* CallsAvailable */
static int hf_h225_mcuCallsAvailable = -1;        /* SEQUENCE_OF_CallsAvailable */
static int hf_h225_mcuCallsAvailable_item = -1;   /* CallsAvailable */
static int hf_h225_sipGwCallsAvailable = -1;      /* SEQUENCE_OF_CallsAvailable */
static int hf_h225_sipGwCallsAvailable_item = -1;  /* CallsAvailable */
static int hf_h225_calls = -1;                    /* INTEGER_0_4294967295 */
static int hf_h225_group_IA5String = -1;          /* IA5String_SIZE_1_128 */
static int hf_h225_carrier = -1;                  /* CarrierInfo */
static int hf_h225_sourceCircuitID = -1;          /* CircuitIdentifier */
static int hf_h225_destinationCircuitID = -1;     /* CircuitIdentifier */
static int hf_h225_cic = -1;                      /* CicInfo */
static int hf_h225_group = -1;                    /* GroupID */
static int hf_h225_cic_2_4 = -1;                  /* T_cic_2_4 */
static int hf_h225_cic_2_4_item = -1;             /* OCTET_STRING_SIZE_2_4 */
static int hf_h225_pointCode = -1;                /* OCTET_STRING_SIZE_2_5 */
static int hf_h225_member = -1;                   /* T_member */
static int hf_h225_member_item = -1;              /* INTEGER_0_65535 */
static int hf_h225_carrierIdentificationCode = -1;  /* OCTET_STRING_SIZE_3_4 */
static int hf_h225_carrierName = -1;              /* IA5String_SIZE_1_128 */
static int hf_h225_url = -1;                      /* IA5String_SIZE_0_512 */
static int hf_h225_signal = -1;                   /* H248SignalsDescriptor */
static int hf_h225_callCreditServiceControl = -1;  /* CallCreditServiceControl */
static int hf_h225_sessionId_0_255 = -1;          /* INTEGER_0_255 */
static int hf_h225_contents = -1;                 /* ServiceControlDescriptor */
static int hf_h225_reason = -1;                   /* ServiceControlSession_reason */
static int hf_h225_open = -1;                     /* NULL */
static int hf_h225_refresh = -1;                  /* NULL */
static int hf_h225_close = -1;                    /* NULL */
static int hf_h225_nonStandardUsageTypes = -1;    /* SEQUENCE_OF_NonStandardParameter */
static int hf_h225_nonStandardUsageTypes_item = -1;  /* NonStandardParameter */
static int hf_h225_startTime = -1;                /* NULL */
static int hf_h225_endTime_flg = -1;              /* NULL */
static int hf_h225_terminationCause_flg = -1;     /* NULL */
static int hf_h225_when = -1;                     /* RasUsageSpecification_when */
static int hf_h225_start = -1;                    /* NULL */
static int hf_h225_end = -1;                      /* NULL */
static int hf_h225_inIrr = -1;                    /* NULL */
static int hf_h225_ras_callStartingPoint = -1;    /* RasUsageSpecificationcallStartingPoint */
static int hf_h225_alerting_flg = -1;             /* NULL */
static int hf_h225_connect_flg = -1;              /* NULL */
static int hf_h225_required = -1;                 /* RasUsageInfoTypes */
static int hf_h225_nonStandardUsageFields = -1;   /* SEQUENCE_OF_NonStandardParameter */
static int hf_h225_nonStandardUsageFields_item = -1;  /* NonStandardParameter */
static int hf_h225_alertingTime = -1;             /* TimeStamp */
static int hf_h225_connectTime = -1;              /* TimeStamp */
static int hf_h225_endTime = -1;                  /* TimeStamp */
static int hf_h225_releaseCompleteCauseIE = -1;   /* OCTET_STRING_SIZE_2_32 */
static int hf_h225_sender = -1;                   /* BOOLEAN */
static int hf_h225_multicast = -1;                /* BOOLEAN */
static int hf_h225_bandwidth = -1;                /* BandWidth */
static int hf_h225_rtcpAddresses = -1;            /* TransportChannelInfo */
static int hf_h225_canDisplayAmountString = -1;   /* BOOLEAN */
static int hf_h225_canEnforceDurationLimit = -1;  /* BOOLEAN */
static int hf_h225_amountString = -1;             /* BMPString_SIZE_1_512 */
static int hf_h225_billingMode = -1;              /* T_billingMode */
static int hf_h225_credit = -1;                   /* NULL */
static int hf_h225_debit = -1;                    /* NULL */
static int hf_h225_callDurationLimit = -1;        /* INTEGER_1_4294967295 */
static int hf_h225_enforceCallDurationLimit = -1;  /* BOOLEAN */
static int hf_h225_callStartingPoint = -1;        /* CallCreditServiceControl_callStartingPoint */
static int hf_h225_id = -1;                       /* GenericIdentifier */
static int hf_h225_parameters = -1;               /* SEQUENCE_SIZE_1_512_OF_EnumeratedParameter */
static int hf_h225_parameters_item = -1;          /* EnumeratedParameter */
static int hf_h225_standard = -1;                 /* T_standard */
static int hf_h225_oid = -1;                      /* T_oid */
static int hf_h225_genericIdentifier_nonStandard = -1;  /* GloballyUniqueID */
static int hf_h225_content = -1;                  /* Content */
static int hf_h225_raw = -1;                      /* T_raw */
static int hf_h225_text = -1;                     /* IA5String */
static int hf_h225_unicode = -1;                  /* BMPString */
static int hf_h225_bool = -1;                     /* BOOLEAN */
static int hf_h225_number8 = -1;                  /* INTEGER_0_255 */
static int hf_h225_number16 = -1;                 /* INTEGER_0_65535 */
static int hf_h225_number32 = -1;                 /* INTEGER_0_4294967295 */
static int hf_h225_transport = -1;                /* TransportAddress */
static int hf_h225_compound = -1;                 /* SEQUENCE_SIZE_1_512_OF_EnumeratedParameter */
static int hf_h225_compound_item = -1;            /* EnumeratedParameter */
static int hf_h225_nested = -1;                   /* SEQUENCE_SIZE_1_16_OF_GenericData */
static int hf_h225_nested_item = -1;              /* GenericData */
static int hf_h225_replacementFeatureSet = -1;    /* BOOLEAN */
static int hf_h225_sendAddress = -1;              /* TransportAddress */
static int hf_h225_recvAddress = -1;              /* TransportAddress */
static int hf_h225_rtpAddress = -1;               /* TransportChannelInfo */
static int hf_h225_rtcpAddress = -1;              /* TransportChannelInfo */
static int hf_h225_cname = -1;                    /* PrintableString */
static int hf_h225_ssrc = -1;                     /* INTEGER_1_4294967295 */
static int hf_h225_sessionId = -1;                /* INTEGER_1_255 */
static int hf_h225_associatedSessionIds = -1;     /* T_associatedSessionIds */
static int hf_h225_associatedSessionIds_item = -1;  /* INTEGER_1_255 */
static int hf_h225_multicast_flg = -1;            /* NULL */
static int hf_h225_gatekeeperBased = -1;          /* NULL */
static int hf_h225_endpointBased = -1;            /* NULL */
static int hf_h225_gatekeeperRequest = -1;        /* GatekeeperRequest */
static int hf_h225_gatekeeperConfirm = -1;        /* GatekeeperConfirm */
static int hf_h225_gatekeeperReject = -1;         /* GatekeeperReject */
static int hf_h225_registrationRequest = -1;      /* RegistrationRequest */
static int hf_h225_registrationConfirm = -1;      /* RegistrationConfirm */
static int hf_h225_registrationReject = -1;       /* RegistrationReject */
static int hf_h225_unregistrationRequest = -1;    /* UnregistrationRequest */
static int hf_h225_unregistrationConfirm = -1;    /* UnregistrationConfirm */
static int hf_h225_unregistrationReject = -1;     /* UnregistrationReject */
static int hf_h225_admissionRequest = -1;         /* AdmissionRequest */
static int hf_h225_admissionConfirm = -1;         /* AdmissionConfirm */
static int hf_h225_admissionReject = -1;          /* AdmissionReject */
static int hf_h225_bandwidthRequest = -1;         /* BandwidthRequest */
static int hf_h225_bandwidthConfirm = -1;         /* BandwidthConfirm */
static int hf_h225_bandwidthReject = -1;          /* BandwidthReject */
static int hf_h225_disengageRequest = -1;         /* DisengageRequest */
static int hf_h225_disengageConfirm = -1;         /* DisengageConfirm */
static int hf_h225_disengageReject = -1;          /* DisengageReject */
static int hf_h225_locationRequest = -1;          /* LocationRequest */
static int hf_h225_locationConfirm = -1;          /* LocationConfirm */
static int hf_h225_locationReject = -1;           /* LocationReject */
static int hf_h225_infoRequest = -1;              /* InfoRequest */
static int hf_h225_infoRequestResponse = -1;      /* InfoRequestResponse */
static int hf_h225_nonStandardMessage = -1;       /* NonStandardMessage */
static int hf_h225_unknownMessageResponse = -1;   /* UnknownMessageResponse */
static int hf_h225_requestInProgress = -1;        /* RequestInProgress */
static int hf_h225_resourcesAvailableIndicate = -1;  /* ResourcesAvailableIndicate */
static int hf_h225_resourcesAvailableConfirm = -1;  /* ResourcesAvailableConfirm */
static int hf_h225_infoRequestAck = -1;           /* InfoRequestAck */
static int hf_h225_infoRequestNak = -1;           /* InfoRequestNak */
static int hf_h225_serviceControlIndication = -1;  /* ServiceControlIndication */
static int hf_h225_serviceControlResponse = -1;   /* ServiceControlResponse */
static int hf_h225_admissionConfirmSequence = -1;  /* SEQUENCE_OF_AdmissionConfirm */
static int hf_h225_admissionConfirmSequence_item = -1;  /* AdmissionConfirm */
static int hf_h225_requestSeqNum = -1;            /* RequestSeqNum */
static int hf_h225_gatekeeperRequest_rasAddress = -1;  /* TransportAddress */
static int hf_h225_endpointAlias = -1;            /* SEQUENCE_OF_AliasAddress */
static int hf_h225_endpointAlias_item = -1;       /* AliasAddress */
static int hf_h225_alternateEndpoints = -1;       /* SEQUENCE_OF_Endpoint */
static int hf_h225_alternateEndpoints_item = -1;  /* Endpoint */
static int hf_h225_authenticationCapability = -1;  /* SEQUENCE_OF_AuthenticationMechanism */
static int hf_h225_authenticationCapability_item = -1;  /* AuthenticationMechanism */
static int hf_h225_algorithmOIDs = -1;            /* T_algorithmOIDs */
static int hf_h225_algorithmOIDs_item = -1;       /* OBJECT_IDENTIFIER */
static int hf_h225_integrity = -1;                /* SEQUENCE_OF_IntegrityMechanism */
static int hf_h225_integrity_item = -1;           /* IntegrityMechanism */
static int hf_h225_integrityCheckValue = -1;      /* ICV */
static int hf_h225_supportsAltGK = -1;            /* NULL */
static int hf_h225_supportsAssignedGK = -1;       /* BOOLEAN */
static int hf_h225_assignedGatekeeper = -1;       /* AlternateGK */
static int hf_h225_gatekeeperConfirm_rasAddress = -1;  /* TransportAddress */
static int hf_h225_authenticationMode = -1;       /* AuthenticationMechanism */
static int hf_h225_rehomingModel = -1;            /* RehomingModel */
static int hf_h225_gatekeeperRejectReason = -1;   /* GatekeeperRejectReason */
static int hf_h225_altGKInfo = -1;                /* AltGKInfo */
static int hf_h225_resourceUnavailable = -1;      /* NULL */
static int hf_h225_terminalExcluded = -1;         /* NULL */
static int hf_h225_securityDenial = -1;           /* NULL */
static int hf_h225_gkRej_securityError = -1;      /* SecurityErrors */
static int hf_h225_discoveryComplete = -1;        /* BOOLEAN */
static int hf_h225_terminalType = -1;             /* EndpointType */
static int hf_h225_terminalAlias = -1;            /* SEQUENCE_OF_AliasAddress */
static int hf_h225_terminalAlias_item = -1;       /* AliasAddress */
static int hf_h225_endpointVendor = -1;           /* VendorIdentifier */
static int hf_h225_timeToLive = -1;               /* TimeToLive */
static int hf_h225_keepAlive = -1;                /* BOOLEAN */
static int hf_h225_willSupplyUUIEs = -1;          /* BOOLEAN */
static int hf_h225_additiveRegistration = -1;     /* NULL */
static int hf_h225_terminalAliasPattern = -1;     /* SEQUENCE_OF_AddressPattern */
static int hf_h225_terminalAliasPattern_item = -1;  /* AddressPattern */
static int hf_h225_usageReportingCapability = -1;  /* RasUsageInfoTypes */
static int hf_h225_supportedH248Packages = -1;    /* SEQUENCE_OF_H248PackagesDescriptor */
static int hf_h225_supportedH248Packages_item = -1;  /* H248PackagesDescriptor */
static int hf_h225_callCreditCapability = -1;     /* CallCreditCapability */
static int hf_h225_capacityReportingCapability = -1;  /* CapacityReportingCapability */
static int hf_h225_restart = -1;                  /* NULL */
static int hf_h225_supportsACFSequences = -1;     /* NULL */
static int hf_h225_transportQOS = -1;             /* TransportQOS */
static int hf_h225_willRespondToIRR = -1;         /* BOOLEAN */
static int hf_h225_preGrantedARQ = -1;            /* T_preGrantedARQ */
static int hf_h225_makeCall = -1;                 /* BOOLEAN */
static int hf_h225_useGKCallSignalAddressToMakeCall = -1;  /* BOOLEAN */
static int hf_h225_answerCall = -1;               /* BOOLEAN */
static int hf_h225_useGKCallSignalAddressToAnswer = -1;  /* BOOLEAN */
static int hf_h225_irrFrequencyInCall = -1;       /* INTEGER_1_65535 */
static int hf_h225_totalBandwidthRestriction = -1;  /* BandWidth */
static int hf_h225_useSpecifiedTransport = -1;    /* UseSpecifiedTransport */
static int hf_h225_supportsAdditiveRegistration = -1;  /* NULL */
static int hf_h225_usageSpec = -1;                /* SEQUENCE_OF_RasUsageSpecification */
static int hf_h225_usageSpec_item = -1;           /* RasUsageSpecification */
static int hf_h225_featureServerAlias = -1;       /* AliasAddress */
static int hf_h225_capacityReportingSpec = -1;    /* CapacityReportingSpecification */
static int hf_h225_registrationRejectReason = -1;  /* RegistrationRejectReason */
static int hf_h225_discoveryRequired = -1;        /* NULL */
static int hf_h225_invalidCallSignalAddress = -1;  /* NULL */
static int hf_h225_invalidRASAddress = -1;        /* NULL */
static int hf_h225_duplicateAlias = -1;           /* SEQUENCE_OF_AliasAddress */
static int hf_h225_duplicateAlias_item = -1;      /* AliasAddress */
static int hf_h225_invalidTerminalType = -1;      /* NULL */
static int hf_h225_transportNotSupported = -1;    /* NULL */
static int hf_h225_transportQOSNotSupported = -1;  /* NULL */
static int hf_h225_invalidAlias = -1;             /* NULL */
static int hf_h225_fullRegistrationRequired = -1;  /* NULL */
static int hf_h225_additiveRegistrationNotSupported = -1;  /* NULL */
static int hf_h225_invalidTerminalAliases = -1;   /* T_invalidTerminalAliases */
static int hf_h225_reg_securityError = -1;        /* SecurityErrors */
static int hf_h225_registerWithAssignedGK = -1;   /* NULL */
static int hf_h225_unregRequestReason = -1;       /* UnregRequestReason */
static int hf_h225_endpointAliasPattern = -1;     /* SEQUENCE_OF_AddressPattern */
static int hf_h225_endpointAliasPattern_item = -1;  /* AddressPattern */
static int hf_h225_reregistrationRequired = -1;   /* NULL */
static int hf_h225_ttlExpired = -1;               /* NULL */
static int hf_h225_maintenance = -1;              /* NULL */
static int hf_h225_securityError = -1;            /* SecurityErrors2 */
static int hf_h225_unregRejectReason = -1;        /* UnregRejectReason */
static int hf_h225_notCurrentlyRegistered = -1;   /* NULL */
static int hf_h225_callInProgress = -1;           /* NULL */
static int hf_h225_permissionDenied = -1;         /* NULL */
static int hf_h225_callModel = -1;                /* CallModel */
static int hf_h225_DestinationInfo_item = -1;     /* DestinationInfo_item */
static int hf_h225_destinationInfo_01 = -1;       /* DestinationInfo */
static int hf_h225_srcInfo = -1;                  /* SEQUENCE_OF_AliasAddress */
static int hf_h225_srcInfo_item = -1;             /* AliasAddress */
static int hf_h225_srcCallSignalAddress = -1;     /* TransportAddress */
static int hf_h225_bandWidth = -1;                /* BandWidth */
static int hf_h225_callReferenceValue = -1;       /* CallReferenceValue */
static int hf_h225_canMapAlias = -1;              /* BOOLEAN */
static int hf_h225_srcAlternatives = -1;          /* SEQUENCE_OF_Endpoint */
static int hf_h225_srcAlternatives_item = -1;     /* Endpoint */
static int hf_h225_destAlternatives = -1;         /* SEQUENCE_OF_Endpoint */
static int hf_h225_destAlternatives_item = -1;    /* Endpoint */
static int hf_h225_gatewayDataRate = -1;          /* DataRate */
static int hf_h225_desiredTunnelledProtocol = -1;  /* TunnelledProtocol */
static int hf_h225_canMapSrcAlias = -1;           /* BOOLEAN */
static int hf_h225_pointToPoint = -1;             /* NULL */
static int hf_h225_oneToN = -1;                   /* NULL */
static int hf_h225_nToOne = -1;                   /* NULL */
static int hf_h225_nToN = -1;                     /* NULL */
static int hf_h225_direct = -1;                   /* NULL */
static int hf_h225_gatekeeperRouted = -1;         /* NULL */
static int hf_h225_endpointControlled = -1;       /* NULL */
static int hf_h225_gatekeeperControlled = -1;     /* NULL */
static int hf_h225_noControl = -1;                /* NULL */
static int hf_h225_qOSCapabilities = -1;          /* SEQUENCE_SIZE_1_256_OF_QOSCapability */
static int hf_h225_qOSCapabilities_item = -1;     /* QOSCapability */
static int hf_h225_irrFrequency = -1;             /* INTEGER_1_65535 */
static int hf_h225_destinationType = -1;          /* EndpointType */
static int hf_h225_uuiesRequested = -1;           /* UUIEsRequested */
static int hf_h225_supportedProtocols = -1;       /* SEQUENCE_OF_SupportedProtocols */
static int hf_h225_supportedProtocols_item = -1;  /* SupportedProtocols */
static int hf_h225_modifiedSrcInfo = -1;          /* SEQUENCE_OF_AliasAddress */
static int hf_h225_modifiedSrcInfo_item = -1;     /* AliasAddress */
static int hf_h225_setup_bool = -1;               /* BOOLEAN */
static int hf_h225_callProceeding_flg = -1;       /* BOOLEAN */
static int hf_h225_connect_bool = -1;             /* BOOLEAN */
static int hf_h225_alerting_bool = -1;            /* BOOLEAN */
static int hf_h225_information_bool = -1;         /* BOOLEAN */
static int hf_h225_releaseComplete_bool = -1;     /* BOOLEAN */
static int hf_h225_facility_bool = -1;            /* BOOLEAN */
static int hf_h225_progress_bool = -1;            /* BOOLEAN */
static int hf_h225_empty = -1;                    /* BOOLEAN */
static int hf_h225_status_bool = -1;              /* BOOLEAN */
static int hf_h225_statusInquiry_bool = -1;       /* BOOLEAN */
static int hf_h225_setupAcknowledge_bool = -1;    /* BOOLEAN */
static int hf_h225_notify_bool = -1;              /* BOOLEAN */
static int hf_h225_rejectReason = -1;             /* AdmissionRejectReason */
static int hf_h225_invalidPermission = -1;        /* NULL */
static int hf_h225_requestDenied = -1;            /* NULL */
static int hf_h225_invalidEndpointIdentifier = -1;  /* NULL */
static int hf_h225_qosControlNotSupported = -1;   /* NULL */
static int hf_h225_incompleteAddress = -1;        /* NULL */
static int hf_h225_aliasesInconsistent = -1;      /* NULL */
static int hf_h225_routeCallToSCN = -1;           /* SEQUENCE_OF_PartyNumber */
static int hf_h225_routeCallToSCN_item = -1;      /* PartyNumber */
static int hf_h225_exceedsCallCapacity = -1;      /* NULL */
static int hf_h225_collectDestination = -1;       /* NULL */
static int hf_h225_collectPIN = -1;               /* NULL */
static int hf_h225_noRouteToDestination = -1;     /* NULL */
static int hf_h225_unallocatedNumber = -1;        /* NULL */
static int hf_h225_answeredCall = -1;             /* BOOLEAN */
static int hf_h225_usageInformation = -1;         /* RasUsageInformation */
static int hf_h225_bandwidthDetails = -1;         /* SEQUENCE_OF_BandwidthDetails */
static int hf_h225_bandwidthDetails_item = -1;    /* BandwidthDetails */
static int hf_h225_bandRejectReason = -1;         /* BandRejectReason */
static int hf_h225_allowedBandWidth = -1;         /* BandWidth */
static int hf_h225_notBound = -1;                 /* NULL */
static int hf_h225_invalidConferenceID = -1;      /* NULL */
static int hf_h225_insufficientResources = -1;    /* NULL */
static int hf_h225_replyAddress = -1;             /* TransportAddress */
static int hf_h225_sourceInfo = -1;               /* SEQUENCE_OF_AliasAddress */
static int hf_h225_sourceInfo_item = -1;          /* AliasAddress */
static int hf_h225_hopCount = -1;                 /* INTEGER_1_255 */
static int hf_h225_sourceEndpointInfo = -1;       /* SEQUENCE_OF_AliasAddress */
static int hf_h225_sourceEndpointInfo_item = -1;  /* AliasAddress */
static int hf_h225_locationConfirm_callSignalAddress = -1;  /* TransportAddress */
static int hf_h225_locationConfirm_rasAddress = -1;  /* TransportAddress */
static int hf_h225_locationRejectReason = -1;     /* LocationRejectReason */
static int hf_h225_notRegistered = -1;            /* NULL */
static int hf_h225_routeCalltoSCN = -1;           /* SEQUENCE_OF_PartyNumber */
static int hf_h225_routeCalltoSCN_item = -1;      /* PartyNumber */
static int hf_h225_disengageReason = -1;          /* DisengageReason */
static int hf_h225_terminationCause = -1;         /* CallTerminationCause */
static int hf_h225_forcedDrop = -1;               /* NULL */
static int hf_h225_normalDrop = -1;               /* NULL */
static int hf_h225_disengageRejectReason = -1;    /* DisengageRejectReason */
static int hf_h225_requestToDropOther = -1;       /* NULL */
static int hf_h225_usageInfoRequested = -1;       /* RasUsageInfoTypes */
static int hf_h225_segmentedResponseSupported = -1;  /* NULL */
static int hf_h225_nextSegmentRequested = -1;     /* INTEGER_0_65535 */
static int hf_h225_capacityInfoRequested = -1;    /* NULL */
static int hf_h225_infoRequestResponse_rasAddress = -1;  /* TransportAddress */
static int hf_h225_perCallInfo = -1;              /* T_perCallInfo */
static int hf_h225_perCallInfo_item = -1;         /* T_perCallInfo_item */
static int hf_h225_originator = -1;               /* BOOLEAN */
static int hf_h225_audio = -1;                    /* SEQUENCE_OF_RTPSession */
static int hf_h225_audio_item = -1;               /* RTPSession */
static int hf_h225_video = -1;                    /* SEQUENCE_OF_RTPSession */
static int hf_h225_video_item = -1;               /* RTPSession */
static int hf_h225_data = -1;                     /* SEQUENCE_OF_TransportChannelInfo */
static int hf_h225_data_item = -1;                /* TransportChannelInfo */
static int hf_h225_h245 = -1;                     /* TransportChannelInfo */
static int hf_h225_callSignalling = -1;           /* TransportChannelInfo */
static int hf_h225_substituteConfIDs = -1;        /* SEQUENCE_OF_ConferenceIdentifier */
static int hf_h225_substituteConfIDs_item = -1;   /* ConferenceIdentifier */
static int hf_h225_pdu = -1;                      /* T_pdu */
static int hf_h225_pdu_item = -1;                 /* T_pdu_item */
static int hf_h225_h323pdu = -1;                  /* H323_UU_PDU */
static int hf_h225_sent = -1;                     /* BOOLEAN */
static int hf_h225_needResponse = -1;             /* BOOLEAN */
static int hf_h225_irrStatus = -1;                /* InfoRequestResponseStatus */
static int hf_h225_unsolicited = -1;              /* BOOLEAN */
static int hf_h225_complete = -1;                 /* NULL */
static int hf_h225_incomplete = -1;               /* NULL */
static int hf_h225_segment = -1;                  /* INTEGER_0_65535 */
static int hf_h225_invalidCall = -1;              /* NULL */
static int hf_h225_nakReason = -1;                /* InfoRequestNakReason */
static int hf_h225_messageNotUnderstood = -1;     /* OCTET_STRING */
static int hf_h225_delay = -1;                    /* INTEGER_1_65535 */
static int hf_h225_protocols = -1;                /* SEQUENCE_OF_SupportedProtocols */
static int hf_h225_protocols_item = -1;           /* SupportedProtocols */
static int hf_h225_almostOutOfResources = -1;     /* BOOLEAN */
static int hf_h225_callSpecific = -1;             /* T_callSpecific */
static int hf_h225_result = -1;                   /* T_result */
static int hf_h225_started = -1;                  /* NULL */
static int hf_h225_failed = -1;                   /* NULL */
static int hf_h225_stopped = -1;                  /* NULL */
static int hf_h225_notAvailable = -1;             /* NULL */

/*--- End of included file: packet-h225-hf.c ---*/
#line 130 "./asn1/h225/packet-h225-template.c"

/* Initialize the subtree pointers */
static gint ett_h225 = -1;

/*--- Included file: packet-h225-ett.c ---*/
#line 1 "./asn1/h225/packet-h225-ett.c"
static gint ett_h225_H323_UserInformation = -1;
static gint ett_h225_T_user_data = -1;
static gint ett_h225_H323_UU_PDU = -1;
static gint ett_h225_T_h323_message_body = -1;
static gint ett_h225_T_h4501SupplementaryService = -1;
static gint ett_h225_H245Control = -1;
static gint ett_h225_SEQUENCE_OF_NonStandardParameter = -1;
static gint ett_h225_T_tunnelledSignallingMessage = -1;
static gint ett_h225_T_messageContent = -1;
static gint ett_h225_SEQUENCE_OF_GenericData = -1;
static gint ett_h225_StimulusControl = -1;
static gint ett_h225_Alerting_UUIE = -1;
static gint ett_h225_SEQUENCE_OF_ClearToken = -1;
static gint ett_h225_SEQUENCE_OF_CryptoH323Token = -1;
static gint ett_h225_SEQUENCE_OF_AliasAddress = -1;
static gint ett_h225_SEQUENCE_OF_ServiceControlSession = -1;
static gint ett_h225_SEQUENCE_OF_DisplayName = -1;
static gint ett_h225_CallProceeding_UUIE = -1;
static gint ett_h225_Connect_UUIE = -1;
static gint ett_h225_Information_UUIE = -1;
static gint ett_h225_ReleaseComplete_UUIE = -1;
static gint ett_h225_ReleaseCompleteReason = -1;
static gint ett_h225_Setup_UUIE = -1;
static gint ett_h225_SEQUENCE_OF_CallReferenceValue = -1;
static gint ett_h225_T_conferenceGoal = -1;
static gint ett_h225_SEQUENCE_OF_H245Security = -1;
static gint ett_h225_FastStart = -1;
static gint ett_h225_T_connectionParameters = -1;
static gint ett_h225_Language = -1;
static gint ett_h225_SEQUENCE_OF_SupportedProtocols = -1;
static gint ett_h225_SEQUENCE_OF_FeatureDescriptor = -1;
static gint ett_h225_ParallelH245Control = -1;
static gint ett_h225_SEQUENCE_OF_ExtendedAliasAddress = -1;
static gint ett_h225_ScnConnectionType = -1;
static gint ett_h225_ScnConnectionAggregation = -1;
static gint ett_h225_PresentationIndicator = -1;
static gint ett_h225_Facility_UUIE = -1;
static gint ett_h225_SEQUENCE_OF_ConferenceList = -1;
static gint ett_h225_ConferenceList = -1;
static gint ett_h225_FacilityReason = -1;
static gint ett_h225_Progress_UUIE = -1;
static gint ett_h225_TransportAddress = -1;
static gint ett_h225_H245TransportAddress = -1;
static gint ett_h225_T_h245IpAddress = -1;
static gint ett_h225_T_h245IpSourceRoute = -1;
static gint ett_h225_T_h245Route = -1;
static gint ett_h225_T_h245Routing = -1;
static gint ett_h225_T_h245IpxAddress = -1;
static gint ett_h225_T_h245Ip6Address = -1;
static gint ett_h225_T_ipAddress = -1;
static gint ett_h225_T_ipSourceRoute = -1;
static gint ett_h225_T_route = -1;
static gint ett_h225_T_routing = -1;
static gint ett_h225_T_ipxAddress = -1;
static gint ett_h225_T_ip6Address = -1;
static gint ett_h225_Status_UUIE = -1;
static gint ett_h225_StatusInquiry_UUIE = -1;
static gint ett_h225_SetupAcknowledge_UUIE = -1;
static gint ett_h225_Notify_UUIE = -1;
static gint ett_h225_EndpointType = -1;
static gint ett_h225_SEQUENCE_OF_TunnelledProtocol = -1;
static gint ett_h225_GatewayInfo = -1;
static gint ett_h225_SupportedProtocols = -1;
static gint ett_h225_H310Caps = -1;
static gint ett_h225_SEQUENCE_OF_DataRate = -1;
static gint ett_h225_SEQUENCE_OF_SupportedPrefix = -1;
static gint ett_h225_H320Caps = -1;
static gint ett_h225_H321Caps = -1;
static gint ett_h225_H322Caps = -1;
static gint ett_h225_H323Caps = -1;
static gint ett_h225_H324Caps = -1;
static gint ett_h225_VoiceCaps = -1;
static gint ett_h225_T120OnlyCaps = -1;
static gint ett_h225_NonStandardProtocol = -1;
static gint ett_h225_T38FaxAnnexbOnlyCaps = -1;
static gint ett_h225_SIPCaps = -1;
static gint ett_h225_McuInfo = -1;
static gint ett_h225_TerminalInfo = -1;
static gint ett_h225_GatekeeperInfo = -1;
static gint ett_h225_VendorIdentifier = -1;
static gint ett_h225_H221NonStandard = -1;
static gint ett_h225_TunnelledProtocol = -1;
static gint ett_h225_TunnelledProtocol_id = -1;
static gint ett_h225_TunnelledProtocolAlternateIdentifier = -1;
static gint ett_h225_NonStandardParameter = -1;
static gint ett_h225_NonStandardIdentifier = -1;
static gint ett_h225_AliasAddress = -1;
static gint ett_h225_AddressPattern = -1;
static gint ett_h225_T_range = -1;
static gint ett_h225_PartyNumber = -1;
static gint ett_h225_PublicPartyNumber = -1;
static gint ett_h225_PrivatePartyNumber = -1;
static gint ett_h225_DisplayName = -1;
static gint ett_h225_PublicTypeOfNumber = -1;
static gint ett_h225_PrivateTypeOfNumber = -1;
static gint ett_h225_MobileUIM = -1;
static gint ett_h225_ANSI_41_UIM = -1;
static gint ett_h225_T_system_id = -1;
static gint ett_h225_GSM_UIM = -1;
static gint ett_h225_IsupNumber = -1;
static gint ett_h225_IsupPublicPartyNumber = -1;
static gint ett_h225_IsupPrivatePartyNumber = -1;
static gint ett_h225_NatureOfAddress = -1;
static gint ett_h225_ExtendedAliasAddress = -1;
static gint ett_h225_Endpoint = -1;
static gint ett_h225_SEQUENCE_OF_TransportAddress = -1;
static gint ett_h225_AlternateTransportAddresses = -1;
static gint ett_h225_UseSpecifiedTransport = -1;
static gint ett_h225_AlternateGK = -1;
static gint ett_h225_AltGKInfo = -1;
static gint ett_h225_SEQUENCE_OF_AlternateGK = -1;
static gint ett_h225_SecurityServiceMode = -1;
static gint ett_h225_SecurityCapabilities = -1;
static gint ett_h225_SecurityErrors = -1;
static gint ett_h225_SecurityErrors2 = -1;
static gint ett_h225_H245Security = -1;
static gint ett_h225_QseriesOptions = -1;
static gint ett_h225_Q954Details = -1;
static gint ett_h225_CallIdentifier = -1;
static gint ett_h225_EncryptIntAlg = -1;
static gint ett_h225_NonIsoIntegrityMechanism = -1;
static gint ett_h225_IntegrityMechanism = -1;
static gint ett_h225_ICV = -1;
static gint ett_h225_CryptoH323Token = -1;
static gint ett_h225_T_cryptoEPPwdHash = -1;
static gint ett_h225_T_cryptoGKPwdHash = -1;
static gint ett_h225_DataRate = -1;
static gint ett_h225_CallLinkage = -1;
static gint ett_h225_SupportedPrefix = -1;
static gint ett_h225_CapacityReportingCapability = -1;
static gint ett_h225_CapacityReportingSpecification = -1;
static gint ett_h225_CapacityReportingSpecification_when = -1;
static gint ett_h225_CallCapacity = -1;
static gint ett_h225_CallCapacityInfo = -1;
static gint ett_h225_SEQUENCE_OF_CallsAvailable = -1;
static gint ett_h225_CallsAvailable = -1;
static gint ett_h225_CircuitInfo = -1;
static gint ett_h225_CircuitIdentifier = -1;
static gint ett_h225_CicInfo = -1;
static gint ett_h225_T_cic_2_4 = -1;
static gint ett_h225_GroupID = -1;
static gint ett_h225_T_member = -1;
static gint ett_h225_CarrierInfo = -1;
static gint ett_h225_ServiceControlDescriptor = -1;
static gint ett_h225_ServiceControlSession = -1;
static gint ett_h225_ServiceControlSession_reason = -1;
static gint ett_h225_RasUsageInfoTypes = -1;
static gint ett_h225_RasUsageSpecification = -1;
static gint ett_h225_RasUsageSpecification_when = -1;
static gint ett_h225_RasUsageSpecificationcallStartingPoint = -1;
static gint ett_h225_RasUsageInformation = -1;
static gint ett_h225_CallTerminationCause = -1;
static gint ett_h225_BandwidthDetails = -1;
static gint ett_h225_CallCreditCapability = -1;
static gint ett_h225_CallCreditServiceControl = -1;
static gint ett_h225_T_billingMode = -1;
static gint ett_h225_CallCreditServiceControl_callStartingPoint = -1;
static gint ett_h225_GenericData = -1;
static gint ett_h225_SEQUENCE_SIZE_1_512_OF_EnumeratedParameter = -1;
static gint ett_h225_GenericIdentifier = -1;
static gint ett_h225_EnumeratedParameter = -1;
static gint ett_h225_Content = -1;
static gint ett_h225_SEQUENCE_SIZE_1_16_OF_GenericData = -1;
static gint ett_h225_FeatureSet = -1;
static gint ett_h225_TransportChannelInfo = -1;
static gint ett_h225_RTPSession = -1;
static gint ett_h225_T_associatedSessionIds = -1;
static gint ett_h225_RehomingModel = -1;
static gint ett_h225_RasMessage = -1;
static gint ett_h225_SEQUENCE_OF_AdmissionConfirm = -1;
static gint ett_h225_GatekeeperRequest = -1;
static gint ett_h225_SEQUENCE_OF_Endpoint = -1;
static gint ett_h225_SEQUENCE_OF_AuthenticationMechanism = -1;
static gint ett_h225_T_algorithmOIDs = -1;
static gint ett_h225_SEQUENCE_OF_IntegrityMechanism = -1;
static gint ett_h225_GatekeeperConfirm = -1;
static gint ett_h225_GatekeeperReject = -1;
static gint ett_h225_GatekeeperRejectReason = -1;
static gint ett_h225_RegistrationRequest = -1;
static gint ett_h225_SEQUENCE_OF_AddressPattern = -1;
static gint ett_h225_SEQUENCE_OF_H248PackagesDescriptor = -1;
static gint ett_h225_RegistrationConfirm = -1;
static gint ett_h225_T_preGrantedARQ = -1;
static gint ett_h225_SEQUENCE_OF_RasUsageSpecification = -1;
static gint ett_h225_RegistrationReject = -1;
static gint ett_h225_RegistrationRejectReason = -1;
static gint ett_h225_T_invalidTerminalAliases = -1;
static gint ett_h225_UnregistrationRequest = -1;
static gint ett_h225_UnregRequestReason = -1;
static gint ett_h225_UnregistrationConfirm = -1;
static gint ett_h225_UnregistrationReject = -1;
static gint ett_h225_UnregRejectReason = -1;
static gint ett_h225_AdmissionRequest = -1;
static gint ett_h225_DestinationInfo = -1;
static gint ett_h225_CallType = -1;
static gint ett_h225_CallModel = -1;
static gint ett_h225_TransportQOS = -1;
static gint ett_h225_SEQUENCE_SIZE_1_256_OF_QOSCapability = -1;
static gint ett_h225_AdmissionConfirm = -1;
static gint ett_h225_UUIEsRequested = -1;
static gint ett_h225_AdmissionReject = -1;
static gint ett_h225_AdmissionRejectReason = -1;
static gint ett_h225_SEQUENCE_OF_PartyNumber = -1;
static gint ett_h225_BandwidthRequest = -1;
static gint ett_h225_SEQUENCE_OF_BandwidthDetails = -1;
static gint ett_h225_BandwidthConfirm = -1;
static gint ett_h225_BandwidthReject = -1;
static gint ett_h225_BandRejectReason = -1;
static gint ett_h225_LocationRequest = -1;
static gint ett_h225_LocationConfirm = -1;
static gint ett_h225_LocationReject = -1;
static gint ett_h225_LocationRejectReason = -1;
static gint ett_h225_DisengageRequest = -1;
static gint ett_h225_DisengageReason = -1;
static gint ett_h225_DisengageConfirm = -1;
static gint ett_h225_DisengageReject = -1;
static gint ett_h225_DisengageRejectReason = -1;
static gint ett_h225_InfoRequest = -1;
static gint ett_h225_InfoRequestResponse = -1;
static gint ett_h225_T_perCallInfo = -1;
static gint ett_h225_T_perCallInfo_item = -1;
static gint ett_h225_SEQUENCE_OF_RTPSession = -1;
static gint ett_h225_SEQUENCE_OF_TransportChannelInfo = -1;
static gint ett_h225_SEQUENCE_OF_ConferenceIdentifier = -1;
static gint ett_h225_T_pdu = -1;
static gint ett_h225_T_pdu_item = -1;
static gint ett_h225_InfoRequestResponseStatus = -1;
static gint ett_h225_InfoRequestAck = -1;
static gint ett_h225_InfoRequestNak = -1;
static gint ett_h225_InfoRequestNakReason = -1;
static gint ett_h225_NonStandardMessage = -1;
static gint ett_h225_UnknownMessageResponse = -1;
static gint ett_h225_RequestInProgress = -1;
static gint ett_h225_ResourcesAvailableIndicate = -1;
static gint ett_h225_ResourcesAvailableConfirm = -1;
static gint ett_h225_ServiceControlIndication = -1;
static gint ett_h225_T_callSpecific = -1;
static gint ett_h225_ServiceControlResponse = -1;
static gint ett_h225_T_result = -1;

/*--- End of included file: packet-h225-ett.c ---*/
#line 134 "./asn1/h225/packet-h225-template.c"

/* Preferences */
static guint h225_tls_port = TLS_PORT_CS;
static gboolean h225_reassembly = TRUE;
static gboolean h225_h245_in_tree = TRUE;
static gboolean h225_tp_in_tree = TRUE;

/* Global variables */
static guint32 ipv4_address;
static struct e_in6_addr ipv6_address;
static struct e_in6_addr ipv6_address_zeros = {{0}};
static guint32 ip_port;
static gboolean contains_faststart = FALSE;
static e_guid_t *call_id_guid;

/* NonStandardParameter */
static const char *nsiOID;
static guint32 h221NonStandard;
static guint32 t35CountryCode;
static guint32 t35Extension;
static guint32 manufacturerCode;

/* TunnelledProtocol */
static const char *tpOID;

static const value_string ras_message_category[] = {
  {  0, "Gatekeeper    "},
  {  1, "Registration  "},
  {  2, "UnRegistration"},
  {  3, "Admission     "},
  {  4, "Bandwidth     "},
  {  5, "Disengage     "},
  {  6, "Location      "},
  {  0, NULL }
};

typedef enum _ras_type {
  RAS_REQUEST,
  RAS_CONFIRM,
  RAS_REJECT,
  RAS_OTHER
}ras_type;

typedef enum _ras_category {
  RAS_GATEKEEPER,
  RAS_REGISTRATION,
  RAS_UNREGISTRATION,
  RAS_ADMISSION,
  RAS_BANDWIDTH,
  RAS_DISENGAGE,
  RAS_LOCATION,
  RAS_OTHERS
}ras_category;

#define NUM_RAS_STATS 7

static gboolean
h225rassrt_packet(void *phs, packet_info *pinfo _U_, epan_dissect_t *edt _U_, const void *phi)
{
  rtd_data_t* rtd_data = (rtd_data_t*)phs;
  rtd_stat_table* rs = &rtd_data->stat_table;
  const h225_packet_info *pi=(const h225_packet_info *)phi;

  ras_type rasmsg_type = RAS_OTHER;
  ras_category rascategory = RAS_OTHERS;

  if (pi->msg_type != H225_RAS || pi->msg_tag == -1) {
    /* No RAS Message or uninitialized msg_tag -> return */
    return FALSE;
  }

  if (pi->msg_tag < 21) {
    /* */
    rascategory = (ras_category)(pi->msg_tag / 3);
    rasmsg_type = (ras_type)(pi->msg_tag % 3);
  }
  else {
    /* No SRT yet (ToDo) */
    return FALSE;
  }

  switch(rasmsg_type) {

  case RAS_REQUEST:
    if(pi->is_duplicate){
      rs->time_stats[rascategory].req_dup_num++;
    }
    else {
      rs->time_stats[rascategory].open_req_num++;
    }
    break;

  case RAS_CONFIRM:
    /* no break - delay stats are identical for Confirm and Reject */
  case RAS_REJECT:
    if(pi->is_duplicate){
      /* Duplicate is ignored */
      rs->time_stats[rascategory].rsp_dup_num++;
    }
    else if (!pi->request_available) {
      /* no request was seen, ignore response */
      rs->time_stats[rascategory].disc_rsp_num++;
    }
    else {
      rs->time_stats[rascategory].open_req_num--;
      time_stat_update(&(rs->time_stats[rascategory].rtd[0]),&(pi->delta_time), pinfo);
    }
    break;

  default:
    return FALSE;
  }
  return TRUE;
}


/*--- Included file: packet-h225-fn.c ---*/
#line 1 "./asn1/h225/packet-h225-fn.c"
/*--- Cyclic dependencies ---*/

/* EnumeratedParameter -> Content -> Content/compound -> EnumeratedParameter */
static int dissect_h225_EnumeratedParameter(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);

/* GenericData -> GenericData/parameters -> EnumeratedParameter -> Content -> Content/nested -> GenericData */
/*int dissect_h225_GenericData(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);*/




static int
dissect_h225_ProtocolIdentifier(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_object_identifier(tvb, offset, actx, tree, hf_index, NULL);

  return offset;
}



static int
dissect_h225_T_h245Ip(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 341 "./asn1/h225/h225.cnf"
  tvbuff_t *value_tvb;

  ipv4_address = 0;
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       4, 4, FALSE, &value_tvb);

  if (value_tvb)
    ipv4_address = tvb_get_ipv4(value_tvb, 0);


  return offset;
}



static int
dissect_h225_T_h245IpPort(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 65535U, &ip_port, FALSE);

  return offset;
}


static const per_sequence_t T_h245IpAddress_sequence[] = {
  { &hf_h225_h245Ip         , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_T_h245Ip },
  { &hf_h225_h245IpPort     , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_T_h245IpPort },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_T_h245IpAddress(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_T_h245IpAddress, T_h245IpAddress_sequence);

  return offset;
}



static int
dissect_h225_OCTET_STRING_SIZE_4(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       4, 4, FALSE, NULL);

  return offset;
}



static int
dissect_h225_INTEGER_0_65535(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 65535U, NULL, FALSE);

  return offset;
}


static const per_sequence_t T_h245Route_sequence_of[1] = {
  { &hf_h225_h245Route_item , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_OCTET_STRING_SIZE_4 },
};

static int
dissect_h225_T_h245Route(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_T_h245Route, T_h245Route_sequence_of);

  return offset;
}



static int
dissect_h225_NULL(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_null(tvb, offset, actx, tree, hf_index);

  return offset;
}


static const value_string h225_T_h245Routing_vals[] = {
  {   0, "strict" },
  {   1, "loose" },
  { 0, NULL }
};

static const per_choice_t T_h245Routing_choice[] = {
  {   0, &hf_h225_strict         , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_loose          , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_T_h245Routing(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_T_h245Routing, T_h245Routing_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t T_h245IpSourceRoute_sequence[] = {
  { &hf_h225_ip             , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_OCTET_STRING_SIZE_4 },
  { &hf_h225_port           , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_INTEGER_0_65535 },
  { &hf_h225_h245Route      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_T_h245Route },
  { &hf_h225_h245Routing    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_T_h245Routing },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_T_h245IpSourceRoute(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_T_h245IpSourceRoute, T_h245IpSourceRoute_sequence);

  return offset;
}



static int
dissect_h225_OCTET_STRING_SIZE_6(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       6, 6, FALSE, NULL);

  return offset;
}



static int
dissect_h225_OCTET_STRING_SIZE_2(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       2, 2, FALSE, NULL);

  return offset;
}


static const per_sequence_t T_h245IpxAddress_sequence[] = {
  { &hf_h225_node           , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_OCTET_STRING_SIZE_6 },
  { &hf_h225_netnum         , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_OCTET_STRING_SIZE_4 },
  { &hf_h225_h245IpxPort    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_OCTET_STRING_SIZE_2 },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_T_h245IpxAddress(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_T_h245IpxAddress, T_h245IpxAddress_sequence);

  return offset;
}



static int
dissect_h225_T_h245Ip6(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 350 "./asn1/h225/h225.cnf"
  tvbuff_t *value_tvb;

  ipv6_address = ipv6_address_zeros;
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       16, 16, FALSE, &value_tvb);

  if (value_tvb)
    tvb_get_ipv6(value_tvb, 0, &ipv6_address);


  return offset;
}



static int
dissect_h225_T_port(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 65535U, &ip_port, FALSE);

  return offset;
}


static const per_sequence_t T_h245Ip6Address_sequence[] = {
  { &hf_h225_h245Ip6        , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_T_h245Ip6 },
  { &hf_h225_port_01        , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_T_port },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_T_h245Ip6Address(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_T_h245Ip6Address, T_h245Ip6Address_sequence);

  return offset;
}



static int
dissect_h225_OCTET_STRING_SIZE_16(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       16, 16, FALSE, NULL);

  return offset;
}



static int
dissect_h225_OCTET_STRING_SIZE_1_20(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       1, 20, FALSE, NULL);

  return offset;
}



static int
dissect_h225_T_nsiOID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_object_identifier_str(tvb, offset, actx, tree, hf_index, &nsiOID);

  return offset;
}



static int
dissect_h225_T_t35CountryCode(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, &t35CountryCode, FALSE);

  return offset;
}



static int
dissect_h225_T_t35Extension(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, &t35Extension, FALSE);

  return offset;
}



static int
dissect_h225_T_manufacturerCode(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 65535U, &manufacturerCode, FALSE);

  return offset;
}


static const per_sequence_t H221NonStandard_sequence[] = {
  { &hf_h225_t35CountryCode , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_T_t35CountryCode },
  { &hf_h225_t35Extension   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_T_t35Extension },
  { &hf_h225_manufacturerCode, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_T_manufacturerCode },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_H221NonStandard(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 802 "./asn1/h225/h225.cnf"
  t35CountryCode = 0;
  t35Extension = 0;
  manufacturerCode = 0;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_H221NonStandard, H221NonStandard_sequence);

#line 806 "./asn1/h225/h225.cnf"
  h221NonStandard = ((t35CountryCode * 256) + t35Extension) * 65536 + manufacturerCode;
  proto_tree_add_uint(tree, hf_h221Manufacturer, tvb, (offset>>3)-4, 4, h221NonStandard);

  return offset;
}


static const value_string h225_NonStandardIdentifier_vals[] = {
  {   0, "object" },
  {   1, "h221NonStandard" },
  { 0, NULL }
};

static const per_choice_t NonStandardIdentifier_choice[] = {
  {   0, &hf_h225_nsiOID         , ASN1_EXTENSION_ROOT    , dissect_h225_T_nsiOID },
  {   1, &hf_h225_h221NonStandard, ASN1_EXTENSION_ROOT    , dissect_h225_H221NonStandard },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_NonStandardIdentifier(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 783 "./asn1/h225/h225.cnf"
  gint32 value;

  nsiOID = "";
  h221NonStandard = 0;

  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_NonStandardIdentifier, NonStandardIdentifier_choice,
                                 &value);

  switch (value) {
    case 0 :  /* object */
      nsp_handle = dissector_get_string_handle(nsp_object_dissector_table, nsiOID);
      break;
    case 1 :  /* h221NonStandard */
      nsp_handle = dissector_get_uint_handle(nsp_h221_dissector_table, h221NonStandard);
      break;
    default :
      nsp_handle = NULL;
    }


  return offset;
}



static int
dissect_h225_T_nsp_data(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 817 "./asn1/h225/h225.cnf"
  tvbuff_t *next_tvb = NULL;

  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, &next_tvb);

  if (next_tvb && tvb_reported_length(next_tvb)) {
    call_dissector((nsp_handle)?nsp_handle:data_handle, next_tvb, actx->pinfo, tree);
  }


  return offset;
}


static const per_sequence_t NonStandardParameter_sequence[] = {
  { &hf_h225_nonStandardIdentifier, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_NonStandardIdentifier },
  { &hf_h225_nsp_data       , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_T_nsp_data },
  { NULL, 0, 0, NULL }
};

int
dissect_h225_NonStandardParameter(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 815 "./asn1/h225/h225.cnf"
  nsp_handle = NULL;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_NonStandardParameter, NonStandardParameter_sequence);

  return offset;
}


static const value_string h225_H245TransportAddress_vals[] = {
  {   0, "ipAddress" },
  {   1, "ipSourceRoute" },
  {   2, "ipxAddress" },
  {   3, "ip6Address" },
  {   4, "netBios" },
  {   5, "nsap" },
  {   6, "nonStandardAddress" },
  { 0, NULL }
};

static const per_choice_t H245TransportAddress_choice[] = {
  {   0, &hf_h225_h245IpAddress  , ASN1_EXTENSION_ROOT    , dissect_h225_T_h245IpAddress },
  {   1, &hf_h225_h245IpSourceRoute, ASN1_EXTENSION_ROOT    , dissect_h225_T_h245IpSourceRoute },
  {   2, &hf_h225_h245IpxAddress , ASN1_EXTENSION_ROOT    , dissect_h225_T_h245IpxAddress },
  {   3, &hf_h225_h245Ip6Address , ASN1_EXTENSION_ROOT    , dissect_h225_T_h245Ip6Address },
  {   4, &hf_h225_netBios        , ASN1_EXTENSION_ROOT    , dissect_h225_OCTET_STRING_SIZE_16 },
  {   5, &hf_h225_nsap           , ASN1_EXTENSION_ROOT    , dissect_h225_OCTET_STRING_SIZE_1_20 },
  {   6, &hf_h225_nonStandardAddress, ASN1_EXTENSION_ROOT    , dissect_h225_NonStandardParameter },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_H245TransportAddress(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 556 "./asn1/h225/h225.cnf"
  h225_packet_info* h225_pi;
  ipv4_address=0;
  ipv6_address = ipv6_address_zeros;
  ip_port=0;


  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_H245TransportAddress, H245TransportAddress_choice,
                                 NULL);

#line 564 "./asn1/h225/h225.cnf"
  /* we need this info for TAPing */
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);

  if (h225_pi) {
    h225_pi->is_h245 = TRUE;
    h225_pi->h245_address = ipv4_address;
    h225_pi->h245_port = ip_port;
  }
  if ( !actx->pinfo->fd->flags.visited && h245_handle && ip_port!=0 ) {
    address src_addr;
    conversation_t *conv=NULL;

    if (ipv4_address!=0) {
      set_address(&src_addr, AT_IPv4, 4, &ipv4_address);
    } else if (memcmp(ipv6_address.bytes, ipv6_address_zeros.bytes, sizeof(ipv6_address.bytes))!=0) {
      set_address(&src_addr, AT_IPv6, 16, ipv6_address.bytes);
    } else {
      return offset;
    }

    conv=find_conversation(actx->pinfo->num, &src_addr, &src_addr, PT_TCP, ip_port, ip_port, NO_ADDR_B|NO_PORT_B);
    if(!conv){
      conv=conversation_new(actx->pinfo->num, &src_addr, &src_addr, PT_TCP, ip_port, ip_port, NO_ADDR2|NO_PORT2);
      conversation_set_dissector(conv, h245_handle);
    }
  }

  return offset;
}



static int
dissect_h225_DialedDigits(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 321 "./asn1/h225/h225.cnf"
  tvbuff_t *value_tvb = NULL;
  guint len = 0;
  h225_packet_info* h225_pi;

  offset = dissect_per_restricted_character_string(tvb, offset, actx, tree, hf_index,
                                                      1, 128, FALSE, "0123456789#*,", 13,
                                                      &value_tvb);

  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);
  if (h225_pi && h225_pi->is_destinationInfo == TRUE) {
    if (value_tvb) {
      len = tvb_reported_length(value_tvb);
      /* XXX - should this be allocated as an ephemeral string? */
      if (len > sizeof h225_pi->dialedDigits - 1)
        len = sizeof h225_pi->dialedDigits - 1;
      tvb_memcpy(value_tvb, (guint8*)h225_pi->dialedDigits, 0, len);
    }
    h225_pi->dialedDigits[len] = '\0';
    h225_pi->is_destinationInfo = FALSE;
  }


  return offset;
}



static int
dissect_h225_BMPString_SIZE_1_256(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_BMPString(tvb, offset, actx, tree, hf_index,
                                          1, 256, FALSE);

  return offset;
}



static int
dissect_h225_IA5String_SIZE_1_512(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_IA5String(tvb, offset, actx, tree, hf_index,
                                          1, 512, FALSE);

  return offset;
}



static int
dissect_h225_IpV4(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       4, 4, FALSE, NULL);

  return offset;
}


static const per_sequence_t T_ipAddress_sequence[] = {
  { &hf_h225_ipV4           , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_IpV4 },
  { &hf_h225_ipV4_port      , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_INTEGER_0_65535 },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_T_ipAddress(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_T_ipAddress, T_ipAddress_sequence);

  return offset;
}


static const per_sequence_t T_route_sequence_of[1] = {
  { &hf_h225_route_item     , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_OCTET_STRING_SIZE_4 },
};

static int
dissect_h225_T_route(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_T_route, T_route_sequence_of);

  return offset;
}


static const value_string h225_T_routing_vals[] = {
  {   0, "strict" },
  {   1, "loose" },
  { 0, NULL }
};

static const per_choice_t T_routing_choice[] = {
  {   0, &hf_h225_strict         , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_loose          , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_T_routing(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_T_routing, T_routing_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t T_ipSourceRoute_sequence[] = {
  { &hf_h225_src_route_ipV4 , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_OCTET_STRING_SIZE_4 },
  { &hf_h225_ipV4_src_port  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_INTEGER_0_65535 },
  { &hf_h225_route          , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_T_route },
  { &hf_h225_routing        , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_T_routing },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_T_ipSourceRoute(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_T_ipSourceRoute, T_ipSourceRoute_sequence);

  return offset;
}


static const per_sequence_t T_ipxAddress_sequence[] = {
  { &hf_h225_node           , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_OCTET_STRING_SIZE_6 },
  { &hf_h225_netnum         , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_OCTET_STRING_SIZE_4 },
  { &hf_h225_ipx_port       , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_OCTET_STRING_SIZE_2 },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_T_ipxAddress(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_T_ipxAddress, T_ipxAddress_sequence);

  return offset;
}


static const per_sequence_t T_ip6Address_sequence[] = {
  { &hf_h225_ipV6           , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_OCTET_STRING_SIZE_16 },
  { &hf_h225_ipV6_port      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_INTEGER_0_65535 },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_T_ip6Address(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_T_ip6Address, T_ip6Address_sequence);

  return offset;
}


const value_string h225_TransportAddress_vals[] = {
  {   0, "ipAddress" },
  {   1, "ipSourceRoute" },
  {   2, "ipxAddress" },
  {   3, "ip6Address" },
  {   4, "netBios" },
  {   5, "nsap" },
  {   6, "nonStandardAddress" },
  { 0, NULL }
};

static const per_choice_t TransportAddress_choice[] = {
  {   0, &hf_h225_ipAddress      , ASN1_EXTENSION_ROOT    , dissect_h225_T_ipAddress },
  {   1, &hf_h225_ipSourceRoute  , ASN1_EXTENSION_ROOT    , dissect_h225_T_ipSourceRoute },
  {   2, &hf_h225_ipxAddress     , ASN1_EXTENSION_ROOT    , dissect_h225_T_ipxAddress },
  {   3, &hf_h225_ip6Address     , ASN1_EXTENSION_ROOT    , dissect_h225_T_ip6Address },
  {   4, &hf_h225_netBios        , ASN1_EXTENSION_ROOT    , dissect_h225_OCTET_STRING_SIZE_16 },
  {   5, &hf_h225_nsap           , ASN1_EXTENSION_ROOT    , dissect_h225_OCTET_STRING_SIZE_1_20 },
  {   6, &hf_h225_nonStandardAddress, ASN1_EXTENSION_ROOT    , dissect_h225_NonStandardParameter },
  { 0, NULL, 0, NULL }
};

int
dissect_h225_TransportAddress(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_TransportAddress, TransportAddress_choice,
                                 NULL);

  return offset;
}


const value_string h225_PublicTypeOfNumber_vals[] = {
  {   0, "unknown" },
  {   1, "internationalNumber" },
  {   2, "nationalNumber" },
  {   3, "networkSpecificNumber" },
  {   4, "subscriberNumber" },
  {   5, "abbreviatedNumber" },
  { 0, NULL }
};

static const per_choice_t PublicTypeOfNumber_choice[] = {
  {   0, &hf_h225_unknown        , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_internationalNumber, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_nationalNumber , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   3, &hf_h225_networkSpecificNumber, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   4, &hf_h225_subscriberNumber, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   5, &hf_h225_abbreviatedNumber, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

int
dissect_h225_PublicTypeOfNumber(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_PublicTypeOfNumber, PublicTypeOfNumber_choice,
                                 NULL);

  return offset;
}



static int
dissect_h225_NumberDigits(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_restricted_character_string(tvb, offset, actx, tree, hf_index,
                                                      1, 128, FALSE, "0123456789#*,", 13,
                                                      NULL);

  return offset;
}


static const per_sequence_t PublicPartyNumber_sequence[] = {
  { &hf_h225_publicTypeOfNumber, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_PublicTypeOfNumber },
  { &hf_h225_publicNumberDigits, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_NumberDigits },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_PublicPartyNumber(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_PublicPartyNumber, PublicPartyNumber_sequence);

  return offset;
}


const value_string h225_PrivateTypeOfNumber_vals[] = {
  {   0, "unknown" },
  {   1, "level2RegionalNumber" },
  {   2, "level1RegionalNumber" },
  {   3, "pISNSpecificNumber" },
  {   4, "localNumber" },
  {   5, "abbreviatedNumber" },
  { 0, NULL }
};

static const per_choice_t PrivateTypeOfNumber_choice[] = {
  {   0, &hf_h225_unknown        , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_level2RegionalNumber, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_level1RegionalNumber, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   3, &hf_h225_pISNSpecificNumber, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   4, &hf_h225_localNumber    , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   5, &hf_h225_abbreviatedNumber, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

int
dissect_h225_PrivateTypeOfNumber(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_PrivateTypeOfNumber, PrivateTypeOfNumber_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t PrivatePartyNumber_sequence[] = {
  { &hf_h225_privateTypeOfNumber, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_PrivateTypeOfNumber },
  { &hf_h225_privateNumberDigits, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_NumberDigits },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_PrivatePartyNumber(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_PrivatePartyNumber, PrivatePartyNumber_sequence);

  return offset;
}


const value_string h225_PartyNumber_vals[] = {
  {   0, "e164Number" },
  {   1, "dataPartyNumber" },
  {   2, "telexPartyNumber" },
  {   3, "privateNumber" },
  {   4, "nationalStandardPartyNumber" },
  { 0, NULL }
};

static const per_choice_t PartyNumber_choice[] = {
  {   0, &hf_h225_e164Number     , ASN1_EXTENSION_ROOT    , dissect_h225_PublicPartyNumber },
  {   1, &hf_h225_dataPartyNumber, ASN1_EXTENSION_ROOT    , dissect_h225_NumberDigits },
  {   2, &hf_h225_telexPartyNumber, ASN1_EXTENSION_ROOT    , dissect_h225_NumberDigits },
  {   3, &hf_h225_privateNumber  , ASN1_EXTENSION_ROOT    , dissect_h225_PrivatePartyNumber },
  {   4, &hf_h225_nationalStandardPartyNumber, ASN1_EXTENSION_ROOT    , dissect_h225_NumberDigits },
  { 0, NULL, 0, NULL }
};

int
dissect_h225_PartyNumber(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_PartyNumber, PartyNumber_choice,
                                 NULL);

  return offset;
}



static int
dissect_h225_TBCD_STRING(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 912 "./asn1/h225/h225.cnf"
  int min_len, max_len;
  gboolean has_extension;

  get_size_constraint_from_stack(actx, "TBCD_STRING", &min_len, &max_len, &has_extension);

  offset = dissect_per_restricted_character_string(tvb, offset, actx, tree, hf_index,
                                                      min_len, max_len, has_extension, "0123456789#*abc", 15,
                                                      NULL);

  return offset;
}



static int
dissect_h225_TBCD_STRING_SIZE_3_16(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_size_constrained_type(tvb, offset, actx, tree, hf_index, dissect_h225_TBCD_STRING,
                                                "TBCD_STRING", 3, 16, FALSE);

  return offset;
}



static int
dissect_h225_TBCD_STRING_SIZE_16(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_size_constrained_type(tvb, offset, actx, tree, hf_index, dissect_h225_TBCD_STRING,
                                                "TBCD_STRING", 16, 16, FALSE);

  return offset;
}



static int
dissect_h225_TBCD_STRING_SIZE_1_4(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_size_constrained_type(tvb, offset, actx, tree, hf_index, dissect_h225_TBCD_STRING,
                                                "TBCD_STRING", 1, 4, FALSE);

  return offset;
}


static const value_string h225_T_system_id_vals[] = {
  {   0, "sid" },
  {   1, "mid" },
  { 0, NULL }
};

static const per_choice_t T_system_id_choice[] = {
  {   0, &hf_h225_sid            , ASN1_EXTENSION_ROOT    , dissect_h225_TBCD_STRING_SIZE_1_4 },
  {   1, &hf_h225_mid            , ASN1_EXTENSION_ROOT    , dissect_h225_TBCD_STRING_SIZE_1_4 },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_T_system_id(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_T_system_id, T_system_id_choice,
                                 NULL);

  return offset;
}



static int
dissect_h225_OCTET_STRING_SIZE_1(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       1, 1, FALSE, NULL);

  return offset;
}


static const per_sequence_t ANSI_41_UIM_sequence[] = {
  { &hf_h225_imsi           , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_TBCD_STRING_SIZE_3_16 },
  { &hf_h225_min            , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_TBCD_STRING_SIZE_3_16 },
  { &hf_h225_mdn            , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_TBCD_STRING_SIZE_3_16 },
  { &hf_h225_msisdn         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_TBCD_STRING_SIZE_3_16 },
  { &hf_h225_esn            , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_TBCD_STRING_SIZE_16 },
  { &hf_h225_mscid          , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_TBCD_STRING_SIZE_3_16 },
  { &hf_h225_system_id      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_T_system_id },
  { &hf_h225_systemMyTypeCode, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_OCTET_STRING_SIZE_1 },
  { &hf_h225_systemAccessType, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_OCTET_STRING_SIZE_1 },
  { &hf_h225_qualificationInformationCode, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_OCTET_STRING_SIZE_1 },
  { &hf_h225_sesn           , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_TBCD_STRING_SIZE_16 },
  { &hf_h225_soc            , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_TBCD_STRING_SIZE_3_16 },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_ANSI_41_UIM(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_ANSI_41_UIM, ANSI_41_UIM_sequence);

  return offset;
}



static int
dissect_h225_OCTET_STRING_SIZE_1_4(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       1, 4, FALSE, NULL);

  return offset;
}



static int
dissect_h225_TBCD_STRING_SIZE_15_16(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_size_constrained_type(tvb, offset, actx, tree, hf_index, dissect_h225_TBCD_STRING,
                                                "TBCD_STRING", 15, 16, FALSE);

  return offset;
}


static const per_sequence_t GSM_UIM_sequence[] = {
  { &hf_h225_imsi           , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_TBCD_STRING_SIZE_3_16 },
  { &hf_h225_tmsi           , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_OCTET_STRING_SIZE_1_4 },
  { &hf_h225_msisdn         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_TBCD_STRING_SIZE_3_16 },
  { &hf_h225_imei           , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_TBCD_STRING_SIZE_15_16 },
  { &hf_h225_hplmn          , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_TBCD_STRING_SIZE_1_4 },
  { &hf_h225_vplmn          , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_TBCD_STRING_SIZE_1_4 },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_GSM_UIM(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_GSM_UIM, GSM_UIM_sequence);

  return offset;
}


static const value_string h225_MobileUIM_vals[] = {
  {   0, "ansi-41-uim" },
  {   1, "gsm-uim" },
  { 0, NULL }
};

static const per_choice_t MobileUIM_choice[] = {
  {   0, &hf_h225_ansi_41_uim    , ASN1_EXTENSION_ROOT    , dissect_h225_ANSI_41_UIM },
  {   1, &hf_h225_gsm_uim        , ASN1_EXTENSION_ROOT    , dissect_h225_GSM_UIM },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_MobileUIM(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_MobileUIM, MobileUIM_choice,
                                 NULL);

  return offset;
}


static const value_string h225_NatureOfAddress_vals[] = {
  {   0, "unknown" },
  {   1, "subscriberNumber" },
  {   2, "nationalNumber" },
  {   3, "internationalNumber" },
  {   4, "networkSpecificNumber" },
  {   5, "routingNumberNationalFormat" },
  {   6, "routingNumberNetworkSpecificFormat" },
  {   7, "routingNumberWithCalledDirectoryNumber" },
  { 0, NULL }
};

static const per_choice_t NatureOfAddress_choice[] = {
  {   0, &hf_h225_unknown        , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_subscriberNumber, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_nationalNumber , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   3, &hf_h225_internationalNumber, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   4, &hf_h225_networkSpecificNumber, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   5, &hf_h225_routingNumberNationalFormat, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   6, &hf_h225_routingNumberNetworkSpecificFormat, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   7, &hf_h225_routingNumberWithCalledDirectoryNumber, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_NatureOfAddress(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_NatureOfAddress, NatureOfAddress_choice,
                                 NULL);

  return offset;
}



static int
dissect_h225_IsupDigits(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_restricted_character_string(tvb, offset, actx, tree, hf_index,
                                                      1, 128, FALSE, "0123456789ABCDE", 15,
                                                      NULL);

  return offset;
}


static const per_sequence_t IsupPublicPartyNumber_sequence[] = {
  { &hf_h225_natureOfAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_NatureOfAddress },
  { &hf_h225_address        , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_IsupDigits },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_IsupPublicPartyNumber(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_IsupPublicPartyNumber, IsupPublicPartyNumber_sequence);

  return offset;
}


static const per_sequence_t IsupPrivatePartyNumber_sequence[] = {
  { &hf_h225_privateTypeOfNumber, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_PrivateTypeOfNumber },
  { &hf_h225_address        , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_IsupDigits },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_IsupPrivatePartyNumber(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_IsupPrivatePartyNumber, IsupPrivatePartyNumber_sequence);

  return offset;
}


static const value_string h225_IsupNumber_vals[] = {
  {   0, "e164Number" },
  {   1, "dataPartyNumber" },
  {   2, "telexPartyNumber" },
  {   3, "privateNumber" },
  {   4, "nationalStandardPartyNumber" },
  { 0, NULL }
};

static const per_choice_t IsupNumber_choice[] = {
  {   0, &hf_h225_isupE164Number , ASN1_EXTENSION_ROOT    , dissect_h225_IsupPublicPartyNumber },
  {   1, &hf_h225_isupDataPartyNumber, ASN1_EXTENSION_ROOT    , dissect_h225_IsupDigits },
  {   2, &hf_h225_isupTelexPartyNumber, ASN1_EXTENSION_ROOT    , dissect_h225_IsupDigits },
  {   3, &hf_h225_isupPrivateNumber, ASN1_EXTENSION_ROOT    , dissect_h225_IsupPrivatePartyNumber },
  {   4, &hf_h225_isupNationalStandardPartyNumber, ASN1_EXTENSION_ROOT    , dissect_h225_IsupDigits },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_IsupNumber(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_IsupNumber, IsupNumber_choice,
                                 NULL);

  return offset;
}


const value_string AliasAddress_vals[] = {
  {   0, "dialledDigits" },
  {   1, "h323-ID" },
  {   2, "url-ID" },
  {   3, "transportID" },
  {   4, "email-ID" },
  {   5, "partyNumber" },
  {   6, "mobileUIM" },
  {   7, "isupNumber" },
  { 0, NULL }
};

static const per_choice_t AliasAddress_choice[] = {
  {   0, &hf_h225_dialledDigits  , ASN1_EXTENSION_ROOT    , dissect_h225_DialedDigits },
  {   1, &hf_h225_h323_ID        , ASN1_EXTENSION_ROOT    , dissect_h225_BMPString_SIZE_1_256 },
  {   2, &hf_h225_url_ID         , ASN1_NOT_EXTENSION_ROOT, dissect_h225_IA5String_SIZE_1_512 },
  {   3, &hf_h225_transportID    , ASN1_NOT_EXTENSION_ROOT, dissect_h225_TransportAddress },
  {   4, &hf_h225_email_ID       , ASN1_NOT_EXTENSION_ROOT, dissect_h225_IA5String_SIZE_1_512 },
  {   5, &hf_h225_partyNumber    , ASN1_NOT_EXTENSION_ROOT, dissect_h225_PartyNumber },
  {   6, &hf_h225_mobileUIM      , ASN1_NOT_EXTENSION_ROOT, dissect_h225_MobileUIM },
  {   7, &hf_h225_isupNumber     , ASN1_NOT_EXTENSION_ROOT, dissect_h225_IsupNumber },
  { 0, NULL, 0, NULL }
};

int
dissect_h225_AliasAddress(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_AliasAddress, AliasAddress_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_AliasAddress_sequence_of[1] = {
  { &hf_h225_alertingAddress_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_AliasAddress },
};

static int
dissect_h225_SEQUENCE_OF_AliasAddress(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_AliasAddress, SEQUENCE_OF_AliasAddress_sequence_of);

  return offset;
}



static int
dissect_h225_OCTET_STRING_SIZE_1_256(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       1, 256, FALSE, NULL);

  return offset;
}



static int
dissect_h225_OBJECT_IDENTIFIER(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_object_identifier(tvb, offset, actx, tree, hf_index, NULL);

  return offset;
}


static const per_sequence_t VendorIdentifier_sequence[] = {
  { &hf_h225_vendorIdentifier_vendor, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_H221NonStandard },
  { &hf_h225_productId      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_OCTET_STRING_SIZE_1_256 },
  { &hf_h225_versionId      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_OCTET_STRING_SIZE_1_256 },
  { &hf_h225_enterpriseNumber, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_OBJECT_IDENTIFIER },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_VendorIdentifier(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_VendorIdentifier, VendorIdentifier_sequence);

  return offset;
}


static const per_sequence_t GatekeeperInfo_sequence[] = {
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_GatekeeperInfo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_GatekeeperInfo, GatekeeperInfo_sequence);

  return offset;
}



int
dissect_h225_BandWidth(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 4294967295U, NULL, FALSE);

  return offset;
}



static int
dissect_h225_INTEGER_1_256(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 256U, NULL, FALSE);

  return offset;
}


static const per_sequence_t DataRate_sequence[] = {
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_channelRate    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BandWidth },
  { &hf_h225_channelMultiplier, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_INTEGER_1_256 },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_DataRate(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_DataRate, DataRate_sequence);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_DataRate_sequence_of[1] = {
  { &hf_h225_dataRatesSupported_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_DataRate },
};

static int
dissect_h225_SEQUENCE_OF_DataRate(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_DataRate, SEQUENCE_OF_DataRate_sequence_of);

  return offset;
}


static const per_sequence_t SupportedPrefix_sequence[] = {
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_prefix         , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_AliasAddress },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_SupportedPrefix(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_SupportedPrefix, SupportedPrefix_sequence);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_SupportedPrefix_sequence_of[1] = {
  { &hf_h225_supportedPrefixes_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_SupportedPrefix },
};

static int
dissect_h225_SEQUENCE_OF_SupportedPrefix(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_SupportedPrefix, SEQUENCE_OF_SupportedPrefix_sequence_of);

  return offset;
}


static const per_sequence_t H310Caps_sequence[] = {
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_dataRatesSupported, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_DataRate },
  { &hf_h225_supportedPrefixes, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_SEQUENCE_OF_SupportedPrefix },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_H310Caps(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_H310Caps, H310Caps_sequence);

  return offset;
}


static const per_sequence_t H320Caps_sequence[] = {
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_dataRatesSupported, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_DataRate },
  { &hf_h225_supportedPrefixes, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_SEQUENCE_OF_SupportedPrefix },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_H320Caps(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_H320Caps, H320Caps_sequence);

  return offset;
}


static const per_sequence_t H321Caps_sequence[] = {
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_dataRatesSupported, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_DataRate },
  { &hf_h225_supportedPrefixes, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_SEQUENCE_OF_SupportedPrefix },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_H321Caps(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_H321Caps, H321Caps_sequence);

  return offset;
}


static const per_sequence_t H322Caps_sequence[] = {
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_dataRatesSupported, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_DataRate },
  { &hf_h225_supportedPrefixes, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_SEQUENCE_OF_SupportedPrefix },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_H322Caps(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_H322Caps, H322Caps_sequence);

  return offset;
}


static const per_sequence_t H323Caps_sequence[] = {
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_dataRatesSupported, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_DataRate },
  { &hf_h225_supportedPrefixes, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_SEQUENCE_OF_SupportedPrefix },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_H323Caps(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_H323Caps, H323Caps_sequence);

  return offset;
}


static const per_sequence_t H324Caps_sequence[] = {
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_dataRatesSupported, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_DataRate },
  { &hf_h225_supportedPrefixes, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_SEQUENCE_OF_SupportedPrefix },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_H324Caps(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_H324Caps, H324Caps_sequence);

  return offset;
}


static const per_sequence_t VoiceCaps_sequence[] = {
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_dataRatesSupported, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_DataRate },
  { &hf_h225_supportedPrefixes, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_SEQUENCE_OF_SupportedPrefix },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_VoiceCaps(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_VoiceCaps, VoiceCaps_sequence);

  return offset;
}


static const per_sequence_t T120OnlyCaps_sequence[] = {
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_dataRatesSupported, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_DataRate },
  { &hf_h225_supportedPrefixes, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_SEQUENCE_OF_SupportedPrefix },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_T120OnlyCaps(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_T120OnlyCaps, T120OnlyCaps_sequence);

  return offset;
}


static const per_sequence_t NonStandardProtocol_sequence[] = {
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_dataRatesSupported, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_DataRate },
  { &hf_h225_supportedPrefixes, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_SEQUENCE_OF_SupportedPrefix },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_NonStandardProtocol(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_NonStandardProtocol, NonStandardProtocol_sequence);

  return offset;
}


static const per_sequence_t T38FaxAnnexbOnlyCaps_sequence[] = {
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_dataRatesSupported, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_DataRate },
  { &hf_h225_supportedPrefixes, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_SEQUENCE_OF_SupportedPrefix },
  { &hf_h225_t38FaxProtocol , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h245_DataProtocolCapability },
  { &hf_h225_t38FaxProfile  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h245_T38FaxProfile },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_T38FaxAnnexbOnlyCaps(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_T38FaxAnnexbOnlyCaps, T38FaxAnnexbOnlyCaps_sequence);

  return offset;
}


static const per_sequence_t SIPCaps_sequence[] = {
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_dataRatesSupported, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_DataRate },
  { &hf_h225_supportedPrefixes, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_SupportedPrefix },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_SIPCaps(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_SIPCaps, SIPCaps_sequence);

  return offset;
}


const value_string h225_SupportedProtocols_vals[] = {
  {   0, "nonStandardData" },
  {   1, "h310" },
  {   2, "h320" },
  {   3, "h321" },
  {   4, "h322" },
  {   5, "h323" },
  {   6, "h324" },
  {   7, "voice" },
  {   8, "t120-only" },
  {   9, "nonStandardProtocol" },
  {  10, "t38FaxAnnexbOnly" },
  {  11, "sip" },
  { 0, NULL }
};

static const per_choice_t SupportedProtocols_choice[] = {
  {   0, &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , dissect_h225_NonStandardParameter },
  {   1, &hf_h225_h310           , ASN1_EXTENSION_ROOT    , dissect_h225_H310Caps },
  {   2, &hf_h225_h320           , ASN1_EXTENSION_ROOT    , dissect_h225_H320Caps },
  {   3, &hf_h225_h321           , ASN1_EXTENSION_ROOT    , dissect_h225_H321Caps },
  {   4, &hf_h225_h322           , ASN1_EXTENSION_ROOT    , dissect_h225_H322Caps },
  {   5, &hf_h225_h323           , ASN1_EXTENSION_ROOT    , dissect_h225_H323Caps },
  {   6, &hf_h225_h324           , ASN1_EXTENSION_ROOT    , dissect_h225_H324Caps },
  {   7, &hf_h225_voice          , ASN1_EXTENSION_ROOT    , dissect_h225_VoiceCaps },
  {   8, &hf_h225_t120_only      , ASN1_EXTENSION_ROOT    , dissect_h225_T120OnlyCaps },
  {   9, &hf_h225_nonStandardProtocol, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NonStandardProtocol },
  {  10, &hf_h225_t38FaxAnnexbOnly, ASN1_NOT_EXTENSION_ROOT, dissect_h225_T38FaxAnnexbOnlyCaps },
  {  11, &hf_h225_sip            , ASN1_NOT_EXTENSION_ROOT, dissect_h225_SIPCaps },
  { 0, NULL, 0, NULL }
};

int
dissect_h225_SupportedProtocols(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_SupportedProtocols, SupportedProtocols_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_SupportedProtocols_sequence_of[1] = {
  { &hf_h225_desiredProtocols_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_SupportedProtocols },
};

static int
dissect_h225_SEQUENCE_OF_SupportedProtocols(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_SupportedProtocols, SEQUENCE_OF_SupportedProtocols_sequence_of);

  return offset;
}


static const per_sequence_t GatewayInfo_sequence[] = {
  { &hf_h225_protocol       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_SupportedProtocols },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_GatewayInfo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_GatewayInfo, GatewayInfo_sequence);

  return offset;
}


static const per_sequence_t McuInfo_sequence[] = {
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_protocol       , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_SupportedProtocols },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_McuInfo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_McuInfo, McuInfo_sequence);

  return offset;
}


static const per_sequence_t TerminalInfo_sequence[] = {
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_TerminalInfo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_TerminalInfo, TerminalInfo_sequence);

  return offset;
}



static int
dissect_h225_BOOLEAN(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_boolean(tvb, offset, actx, tree, hf_index, NULL);

  return offset;
}



static int
dissect_h225_BIT_STRING_SIZE_32(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     32, 32, FALSE, NULL, NULL);

  return offset;
}



static int
dissect_h225_T_tunnelledProtocolObjectID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_object_identifier_str(tvb, offset, actx, tree, hf_index, &tpOID);

  return offset;
}



static int
dissect_h225_IA5String_SIZE_1_64(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_IA5String(tvb, offset, actx, tree, hf_index,
                                          1, 64, FALSE);

  return offset;
}


static const per_sequence_t TunnelledProtocolAlternateIdentifier_sequence[] = {
  { &hf_h225_protocolType   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_IA5String_SIZE_1_64 },
  { &hf_h225_protocolVariant, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_IA5String_SIZE_1_64 },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_TunnelledProtocolAlternateIdentifier(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_TunnelledProtocolAlternateIdentifier, TunnelledProtocolAlternateIdentifier_sequence);

  return offset;
}


static const value_string h225_TunnelledProtocol_id_vals[] = {
  {   0, "tunnelledProtocolObjectID" },
  {   1, "tunnelledProtocolAlternateID" },
  { 0, NULL }
};

static const per_choice_t TunnelledProtocol_id_choice[] = {
  {   0, &hf_h225_tunnelledProtocolObjectID, ASN1_EXTENSION_ROOT    , dissect_h225_T_tunnelledProtocolObjectID },
  {   1, &hf_h225_tunnelledProtocolAlternateID, ASN1_EXTENSION_ROOT    , dissect_h225_TunnelledProtocolAlternateIdentifier },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_TunnelledProtocol_id(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_TunnelledProtocol_id, TunnelledProtocol_id_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t TunnelledProtocol_sequence[] = {
  { &hf_h225_tunnelledProtocol_id, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_TunnelledProtocol_id },
  { &hf_h225_subIdentifier  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_IA5String_SIZE_1_64 },
  { NULL, 0, 0, NULL }
};

int
dissect_h225_TunnelledProtocol(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 760 "./asn1/h225/h225.cnf"
  tpOID = "";

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_TunnelledProtocol, TunnelledProtocol_sequence);

#line 762 "./asn1/h225/h225.cnf"
  tp_handle = dissector_get_string_handle(tp_dissector_table, tpOID);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_TunnelledProtocol_sequence_of[1] = {
  { &hf_h225_supportedTunnelledProtocols_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_TunnelledProtocol },
};

static int
dissect_h225_SEQUENCE_OF_TunnelledProtocol(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_TunnelledProtocol, SEQUENCE_OF_TunnelledProtocol_sequence_of);

  return offset;
}


static const per_sequence_t EndpointType_sequence[] = {
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_vendor         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_VendorIdentifier },
  { &hf_h225_gatekeeper     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_GatekeeperInfo },
  { &hf_h225_gateway        , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_GatewayInfo },
  { &hf_h225_mcu            , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_McuInfo },
  { &hf_h225_terminal       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_TerminalInfo },
  { &hf_h225_mc             , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_undefinedNode  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_set            , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_BIT_STRING_SIZE_32 },
  { &hf_h225_supportedTunnelledProtocols, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_TunnelledProtocol },
  { NULL, 0, 0, NULL }
};

int
dissect_h225_EndpointType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_EndpointType, EndpointType_sequence);

  return offset;
}



int
dissect_h225_CallReferenceValue(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 65535U, NULL, FALSE);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_CallReferenceValue_sequence_of[1] = {
  { &hf_h225_destExtraCRV_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_CallReferenceValue },
};

static int
dissect_h225_SEQUENCE_OF_CallReferenceValue(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_CallReferenceValue, SEQUENCE_OF_CallReferenceValue_sequence_of);

  return offset;
}



int
dissect_h225_GloballyUniqueID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       16, 16, FALSE, (tvbuff_t **)actx->value_ptr);

  return offset;
}



int
dissect_h225_ConferenceIdentifier(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_h225_GloballyUniqueID(tvb, offset, actx, tree, hf_index);

  return offset;
}


static const value_string h225_T_conferenceGoal_vals[] = {
  {   0, "create" },
  {   1, "join" },
  {   2, "invite" },
  {   3, "capability-negotiation" },
  {   4, "callIndependentSupplementaryService" },
  { 0, NULL }
};

static const per_choice_t T_conferenceGoal_choice[] = {
  {   0, &hf_h225_create         , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_join           , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_invite         , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   3, &hf_h225_capability_negotiation, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {   4, &hf_h225_callIndependentSupplementaryService, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_T_conferenceGoal(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_T_conferenceGoal, T_conferenceGoal_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t Q954Details_sequence[] = {
  { &hf_h225_conferenceCalling, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_threePartyService, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_Q954Details(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_Q954Details, Q954Details_sequence);

  return offset;
}


static const per_sequence_t QseriesOptions_sequence[] = {
  { &hf_h225_q932Full       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_q951Full       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_q952Full       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_q953Full       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_q955Full       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_q956Full       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_q957Full       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_q954Info       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_Q954Details },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_QseriesOptions(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_QseriesOptions, QseriesOptions_sequence);

  return offset;
}


static const value_string h225_CallType_vals[] = {
  {   0, "pointToPoint" },
  {   1, "oneToN" },
  {   2, "nToOne" },
  {   3, "nToN" },
  { 0, NULL }
};

static const per_choice_t CallType_choice[] = {
  {   0, &hf_h225_pointToPoint   , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_oneToN         , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_nToOne         , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   3, &hf_h225_nToN           , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_CallType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_CallType, CallType_choice,
                                 NULL);

  return offset;
}



static int
dissect_h225_T_guid(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 724 "./asn1/h225/h225.cnf"
  tvbuff_t *guid_tvb = NULL;

  actx->value_ptr = &guid_tvb;
  offset = dissect_h225_GloballyUniqueID(tvb, offset, actx, tree, hf_index);

  if (guid_tvb)
    tvb_get_ntohguid(guid_tvb, 0, call_id_guid = wmem_new(wmem_packet_scope(), e_guid_t));
  actx->value_ptr = NULL;


  return offset;
}


static const per_sequence_t CallIdentifier_sequence[] = {
  { &hf_h225_guid           , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_T_guid },
  { NULL, 0, 0, NULL }
};

int
dissect_h225_CallIdentifier(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_CallIdentifier, CallIdentifier_sequence);

  return offset;
}


static const value_string h225_SecurityServiceMode_vals[] = {
  {   0, "nonStandard" },
  {   1, "none" },
  {   2, "default" },
  { 0, NULL }
};

static const per_choice_t SecurityServiceMode_choice[] = {
  {   0, &hf_h225_nonStandard    , ASN1_EXTENSION_ROOT    , dissect_h225_NonStandardParameter },
  {   1, &hf_h225_none           , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_default        , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_SecurityServiceMode(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_SecurityServiceMode, SecurityServiceMode_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t SecurityCapabilities_sequence[] = {
  { &hf_h225_nonStandard    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_encryption     , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_SecurityServiceMode },
  { &hf_h225_authenticaton  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_SecurityServiceMode },
  { &hf_h225_securityCapabilities_integrity, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_SecurityServiceMode },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_SecurityCapabilities(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_SecurityCapabilities, SecurityCapabilities_sequence);

  return offset;
}


static const value_string h225_H245Security_vals[] = {
  {   0, "nonStandard" },
  {   1, "noSecurity" },
  {   2, "tls" },
  {   3, "ipsec" },
  { 0, NULL }
};

static const per_choice_t H245Security_choice[] = {
  {   0, &hf_h225_nonStandard    , ASN1_EXTENSION_ROOT    , dissect_h225_NonStandardParameter },
  {   1, &hf_h225_noSecurity     , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_tls            , ASN1_EXTENSION_ROOT    , dissect_h225_SecurityCapabilities },
  {   3, &hf_h225_ipsec          , ASN1_EXTENSION_ROOT    , dissect_h225_SecurityCapabilities },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_H245Security(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_H245Security, H245Security_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_H245Security_sequence_of[1] = {
  { &hf_h225_h245SecurityCapability_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_H245Security },
};

static int
dissect_h225_SEQUENCE_OF_H245Security(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_H245Security, SEQUENCE_OF_H245Security_sequence_of);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_ClearToken_sequence_of[1] = {
  { &hf_h225_tokens_item    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h235_ClearToken },
};

static int
dissect_h225_SEQUENCE_OF_ClearToken(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_ClearToken, SEQUENCE_OF_ClearToken_sequence_of);

  return offset;
}


static const per_sequence_t T_cryptoEPPwdHash_sequence[] = {
  { &hf_h225_alias          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_AliasAddress },
  { &hf_h225_timeStamp      , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h235_TimeStamp },
  { &hf_h225_token          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h235_HASHED },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_T_cryptoEPPwdHash(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_T_cryptoEPPwdHash, T_cryptoEPPwdHash_sequence);

  return offset;
}



int
dissect_h225_GatekeeperIdentifier(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_BMPString(tvb, offset, actx, tree, hf_index,
                                          1, 128, FALSE);

  return offset;
}


static const per_sequence_t T_cryptoGKPwdHash_sequence[] = {
  { &hf_h225_gatekeeperId   , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_GatekeeperIdentifier },
  { &hf_h225_timeStamp      , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h235_TimeStamp },
  { &hf_h225_token          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h235_HASHED },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_T_cryptoGKPwdHash(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_T_cryptoGKPwdHash, T_cryptoGKPwdHash_sequence);

  return offset;
}


const value_string h225_CryptoH323Token_vals[] = {
  {   0, "cryptoEPPwdHash" },
  {   1, "cryptoGKPwdHash" },
  {   2, "cryptoEPPwdEncr" },
  {   3, "cryptoGKPwdEncr" },
  {   4, "cryptoEPCert" },
  {   5, "cryptoGKCert" },
  {   6, "cryptoFastStart" },
  {   7, "nestedcryptoToken" },
  { 0, NULL }
};

static const per_choice_t CryptoH323Token_choice[] = {
  {   0, &hf_h225_cryptoEPPwdHash, ASN1_EXTENSION_ROOT    , dissect_h225_T_cryptoEPPwdHash },
  {   1, &hf_h225_cryptoGKPwdHash, ASN1_EXTENSION_ROOT    , dissect_h225_T_cryptoGKPwdHash },
  {   2, &hf_h225_cryptoEPPwdEncr, ASN1_EXTENSION_ROOT    , dissect_h235_ENCRYPTED },
  {   3, &hf_h225_cryptoGKPwdEncr, ASN1_EXTENSION_ROOT    , dissect_h235_ENCRYPTED },
  {   4, &hf_h225_cryptoEPCert   , ASN1_EXTENSION_ROOT    , dissect_h235_SIGNED },
  {   5, &hf_h225_cryptoGKCert   , ASN1_EXTENSION_ROOT    , dissect_h235_SIGNED },
  {   6, &hf_h225_cryptoFastStart, ASN1_EXTENSION_ROOT    , dissect_h235_SIGNED },
  {   7, &hf_h225_nestedcryptoToken, ASN1_EXTENSION_ROOT    , dissect_h235_CryptoToken },
  { 0, NULL, 0, NULL }
};

int
dissect_h225_CryptoH323Token(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_CryptoH323Token, CryptoH323Token_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_CryptoH323Token_sequence_of[1] = {
  { &hf_h225_cryptoTokens_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_CryptoH323Token },
};

static int
dissect_h225_SEQUENCE_OF_CryptoH323Token(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_CryptoH323Token, SEQUENCE_OF_CryptoH323Token_sequence_of);

  return offset;
}



static int
dissect_h225_FastStart_item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 272 "./asn1/h225/h225.cnf"
  tvbuff_t *value_tvb = NULL;
  char codec_str[50];
  h225_packet_info* h225_pi;
  codec_str[0] = '\0';

  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, &value_tvb);

  if (value_tvb && tvb_reported_length(value_tvb)) {
    dissect_h245_FastStart_OLC(value_tvb, actx->pinfo, tree, codec_str);
  }

  /* Add to packet info */
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);
  if (h225_pi != NULL) {
    char temp[50];
    g_snprintf(temp, 50, "%s %s", h225_pi->frame_label, codec_str);
    g_strlcpy(h225_pi->frame_label, temp, 50);
    h225_pi->is_faststart = TRUE;
  }
  contains_faststart = TRUE;


  return offset;
}


static const per_sequence_t FastStart_sequence_of[1] = {
  { &hf_h225_FastStart_item , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_FastStart_item },
};

static int
dissect_h225_FastStart(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_FastStart, FastStart_sequence_of);

  return offset;
}



static int
dissect_h225_EndpointIdentifier(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_BMPString(tvb, offset, actx, tree, hf_index,
                                          1, 128, FALSE);

  return offset;
}


static const value_string h225_ScnConnectionType_vals[] = {
  {   0, "unknown" },
  {   1, "bChannel" },
  {   2, "hybrid2x64" },
  {   3, "hybrid384" },
  {   4, "hybrid1536" },
  {   5, "hybrid1920" },
  {   6, "multirate" },
  { 0, NULL }
};

static const per_choice_t ScnConnectionType_choice[] = {
  {   0, &hf_h225_unknown        , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_bChannel       , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_hybrid2x64     , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   3, &hf_h225_hybrid384      , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   4, &hf_h225_hybrid1536     , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   5, &hf_h225_hybrid1920     , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   6, &hf_h225_multirate      , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_ScnConnectionType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_ScnConnectionType, ScnConnectionType_choice,
                                 NULL);

  return offset;
}


static const value_string h225_ScnConnectionAggregation_vals[] = {
  {   0, "auto" },
  {   1, "none" },
  {   2, "h221" },
  {   3, "bonded-mode1" },
  {   4, "bonded-mode2" },
  {   5, "bonded-mode3" },
  { 0, NULL }
};

static const per_choice_t ScnConnectionAggregation_choice[] = {
  {   0, &hf_h225_auto           , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_none           , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_h221           , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   3, &hf_h225_bonded_mode1   , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   4, &hf_h225_bonded_mode2   , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   5, &hf_h225_bonded_mode3   , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_ScnConnectionAggregation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_ScnConnectionAggregation, ScnConnectionAggregation_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t T_connectionParameters_sequence[] = {
  { &hf_h225_connectionType , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ScnConnectionType },
  { &hf_h225_numberOfScnConnections, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_INTEGER_0_65535 },
  { &hf_h225_connectionAggregation, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ScnConnectionAggregation },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_T_connectionParameters(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_T_connectionParameters, T_connectionParameters_sequence);

  return offset;
}



static int
dissect_h225_IA5String_SIZE_1_32(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_IA5String(tvb, offset, actx, tree, hf_index,
                                          1, 32, FALSE);

  return offset;
}


static const per_sequence_t Language_sequence_of[1] = {
  { &hf_h225_Language_item  , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_IA5String_SIZE_1_32 },
};

static int
dissect_h225_Language(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_Language, Language_sequence_of);

  return offset;
}


const value_string h225_PresentationIndicator_vals[] = {
  {   0, "presentationAllowed" },
  {   1, "presentationRestricted" },
  {   2, "addressNotAvailable" },
  { 0, NULL }
};

static const per_choice_t PresentationIndicator_choice[] = {
  {   0, &hf_h225_presentationAllowed, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_presentationRestricted, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_addressNotAvailable, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

int
dissect_h225_PresentationIndicator(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_PresentationIndicator, PresentationIndicator_choice,
                                 NULL);

  return offset;
}


const value_string h225_ScreeningIndicator_vals[] = {
  {   0, "userProvidedNotScreened" },
  {   1, "userProvidedVerifiedAndPassed" },
  {   2, "userProvidedVerifiedAndFailed" },
  {   3, "networkProvided" },
  { 0, NULL }
};


int
dissect_h225_ScreeningIndicator(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     4, NULL, TRUE, 0, NULL);

  return offset;
}



static int
dissect_h225_INTEGER_0_255(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, NULL, FALSE);

  return offset;
}



static int
dissect_h225_IA5String_SIZE_0_512(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_IA5String(tvb, offset, actx, tree, hf_index,
                                          0, 512, FALSE);

  return offset;
}



static int
dissect_h225_H248SignalsDescriptor(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, NULL);

  return offset;
}



static int
dissect_h225_BMPString_SIZE_1_512(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_BMPString(tvb, offset, actx, tree, hf_index,
                                          1, 512, FALSE);

  return offset;
}


static const value_string h225_T_billingMode_vals[] = {
  {   0, "credit" },
  {   1, "debit" },
  { 0, NULL }
};

static const per_choice_t T_billingMode_choice[] = {
  {   0, &hf_h225_credit         , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_debit          , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_T_billingMode(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_T_billingMode, T_billingMode_choice,
                                 NULL);

  return offset;
}



static int
dissect_h225_INTEGER_1_4294967295(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 4294967295U, NULL, FALSE);

  return offset;
}


static const value_string h225_CallCreditServiceControl_callStartingPoint_vals[] = {
  {   0, "alerting" },
  {   1, "connect" },
  { 0, NULL }
};

static const per_choice_t CallCreditServiceControl_callStartingPoint_choice[] = {
  {   0, &hf_h225_alerting_flg   , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_connect_flg    , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_CallCreditServiceControl_callStartingPoint(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_CallCreditServiceControl_callStartingPoint, CallCreditServiceControl_callStartingPoint_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t CallCreditServiceControl_sequence[] = {
  { &hf_h225_amountString   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_BMPString_SIZE_1_512 },
  { &hf_h225_billingMode    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_T_billingMode },
  { &hf_h225_callDurationLimit, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_INTEGER_1_4294967295 },
  { &hf_h225_enforceCallDurationLimit, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_BOOLEAN },
  { &hf_h225_callStartingPoint, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_CallCreditServiceControl_callStartingPoint },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_CallCreditServiceControl(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_CallCreditServiceControl, CallCreditServiceControl_sequence);

  return offset;
}


static const value_string h225_ServiceControlDescriptor_vals[] = {
  {   0, "url" },
  {   1, "signal" },
  {   2, "nonStandard" },
  {   3, "callCreditServiceControl" },
  { 0, NULL }
};

static const per_choice_t ServiceControlDescriptor_choice[] = {
  {   0, &hf_h225_url            , ASN1_EXTENSION_ROOT    , dissect_h225_IA5String_SIZE_0_512 },
  {   1, &hf_h225_signal         , ASN1_EXTENSION_ROOT    , dissect_h225_H248SignalsDescriptor },
  {   2, &hf_h225_nonStandard    , ASN1_EXTENSION_ROOT    , dissect_h225_NonStandardParameter },
  {   3, &hf_h225_callCreditServiceControl, ASN1_EXTENSION_ROOT    , dissect_h225_CallCreditServiceControl },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_ServiceControlDescriptor(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_ServiceControlDescriptor, ServiceControlDescriptor_choice,
                                 NULL);

  return offset;
}


static const value_string h225_ServiceControlSession_reason_vals[] = {
  {   0, "open" },
  {   1, "refresh" },
  {   2, "close" },
  { 0, NULL }
};

static const per_choice_t ServiceControlSession_reason_choice[] = {
  {   0, &hf_h225_open           , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_refresh        , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_close          , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_ServiceControlSession_reason(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_ServiceControlSession_reason, ServiceControlSession_reason_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t ServiceControlSession_sequence[] = {
  { &hf_h225_sessionId_0_255, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_INTEGER_0_255 },
  { &hf_h225_contents       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_ServiceControlDescriptor },
  { &hf_h225_reason         , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ServiceControlSession_reason },
  { NULL, 0, 0, NULL }
};

int
dissect_h225_ServiceControlSession(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_ServiceControlSession, ServiceControlSession_sequence);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_ServiceControlSession_sequence_of[1] = {
  { &hf_h225_serviceControl_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_ServiceControlSession },
};

static int
dissect_h225_SEQUENCE_OF_ServiceControlSession(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_ServiceControlSession, SEQUENCE_OF_ServiceControlSession_sequence_of);

  return offset;
}



static int
dissect_h225_INTEGER_0_4294967295(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 4294967295U, NULL, FALSE);

  return offset;
}



static int
dissect_h225_IA5String_SIZE_1_128(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_IA5String(tvb, offset, actx, tree, hf_index,
                                          1, 128, FALSE);

  return offset;
}



static int
dissect_h225_OCTET_STRING_SIZE_3_4(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       3, 4, FALSE, NULL);

  return offset;
}


static const per_sequence_t CarrierInfo_sequence[] = {
  { &hf_h225_carrierIdentificationCode, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_OCTET_STRING_SIZE_3_4 },
  { &hf_h225_carrierName    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_IA5String_SIZE_1_128 },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_CarrierInfo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_CarrierInfo, CarrierInfo_sequence);

  return offset;
}


static const per_sequence_t CallsAvailable_sequence[] = {
  { &hf_h225_calls          , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_INTEGER_0_4294967295 },
  { &hf_h225_group_IA5String, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_IA5String_SIZE_1_128 },
  { &hf_h225_carrier        , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CarrierInfo },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_CallsAvailable(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_CallsAvailable, CallsAvailable_sequence);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_CallsAvailable_sequence_of[1] = {
  { &hf_h225_voiceGwCallsAvailable_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_CallsAvailable },
};

static int
dissect_h225_SEQUENCE_OF_CallsAvailable(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_CallsAvailable, SEQUENCE_OF_CallsAvailable_sequence_of);

  return offset;
}


static const per_sequence_t CallCapacityInfo_sequence[] = {
  { &hf_h225_voiceGwCallsAvailable, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CallsAvailable },
  { &hf_h225_h310GwCallsAvailable, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CallsAvailable },
  { &hf_h225_h320GwCallsAvailable, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CallsAvailable },
  { &hf_h225_h321GwCallsAvailable, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CallsAvailable },
  { &hf_h225_h322GwCallsAvailable, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CallsAvailable },
  { &hf_h225_h323GwCallsAvailable, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CallsAvailable },
  { &hf_h225_h324GwCallsAvailable, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CallsAvailable },
  { &hf_h225_t120OnlyGwCallsAvailable, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CallsAvailable },
  { &hf_h225_t38FaxAnnexbOnlyGwCallsAvailable, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CallsAvailable },
  { &hf_h225_terminalCallsAvailable, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CallsAvailable },
  { &hf_h225_mcuCallsAvailable, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CallsAvailable },
  { &hf_h225_sipGwCallsAvailable, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CallsAvailable },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_CallCapacityInfo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_CallCapacityInfo, CallCapacityInfo_sequence);

  return offset;
}


static const per_sequence_t CallCapacity_sequence[] = {
  { &hf_h225_maximumCallCapacity, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_CallCapacityInfo },
  { &hf_h225_currentCallCapacity, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_CallCapacityInfo },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_CallCapacity(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_CallCapacity, CallCapacity_sequence);

  return offset;
}



static int
dissect_h225_OCTET_STRING_SIZE_2_4(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       2, 4, FALSE, NULL);

  return offset;
}


static const per_sequence_t T_cic_2_4_sequence_of[1] = {
  { &hf_h225_cic_2_4_item   , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_OCTET_STRING_SIZE_2_4 },
};

static int
dissect_h225_T_cic_2_4(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_T_cic_2_4, T_cic_2_4_sequence_of);

  return offset;
}



static int
dissect_h225_OCTET_STRING_SIZE_2_5(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       2, 5, FALSE, NULL);

  return offset;
}


static const per_sequence_t CicInfo_sequence[] = {
  { &hf_h225_cic_2_4        , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_T_cic_2_4 },
  { &hf_h225_pointCode      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_OCTET_STRING_SIZE_2_5 },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_CicInfo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_CicInfo, CicInfo_sequence);

  return offset;
}


static const per_sequence_t T_member_sequence_of[1] = {
  { &hf_h225_member_item    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_INTEGER_0_65535 },
};

static int
dissect_h225_T_member(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_T_member, T_member_sequence_of);

  return offset;
}


static const per_sequence_t GroupID_sequence[] = {
  { &hf_h225_member         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_T_member },
  { &hf_h225_group_IA5String, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_IA5String_SIZE_1_128 },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_GroupID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_GroupID, GroupID_sequence);

  return offset;
}


static const per_sequence_t CircuitIdentifier_sequence[] = {
  { &hf_h225_cic            , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_CicInfo },
  { &hf_h225_group          , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_GroupID },
  { &hf_h225_carrier        , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CarrierInfo },
  { NULL, 0, 0, NULL }
};

int
dissect_h225_CircuitIdentifier(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_CircuitIdentifier, CircuitIdentifier_sequence);

  return offset;
}



static int
dissect_h225_T_standard(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 874 "./asn1/h225/h225.cnf"
  guint32 value_int = (guint32)-1;
  gef_ctx_t *gefx;

  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 16383U, &value_int, TRUE);

  gefx = gef_ctx_get(actx->private_data);
  if (gefx) gefx->id = wmem_strdup_printf(wmem_packet_scope(), "%u", value_int);


  return offset;
}



static int
dissect_h225_T_oid(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 883 "./asn1/h225/h225.cnf"
  const gchar *oid_str = NULL;
  gef_ctx_t *gefx;

  offset = dissect_per_object_identifier_str(tvb, offset, actx, tree, hf_index, &oid_str);

  gefx = gef_ctx_get(actx->private_data);
  if (gefx) gefx->id = oid_str;


  return offset;
}


const value_string h225_GenericIdentifier_vals[] = {
  {   0, "standard" },
  {   1, "oid" },
  {   2, "nonStandard" },
  { 0, NULL }
};

static const per_choice_t GenericIdentifier_choice[] = {
  {   0, &hf_h225_standard       , ASN1_EXTENSION_ROOT    , dissect_h225_T_standard },
  {   1, &hf_h225_oid            , ASN1_EXTENSION_ROOT    , dissect_h225_T_oid },
  {   2, &hf_h225_genericIdentifier_nonStandard, ASN1_EXTENSION_ROOT    , dissect_h225_GloballyUniqueID },
  { 0, NULL, 0, NULL }
};

int
dissect_h225_GenericIdentifier(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 860 "./asn1/h225/h225.cnf"
  gef_ctx_t *gefx;
  proto_item* ti;

  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_GenericIdentifier, GenericIdentifier_choice,
                                 NULL);

#line 863 "./asn1/h225/h225.cnf"
  gef_ctx_update_key(gef_ctx_get(actx->private_data));
  gefx = gef_ctx_get(actx->private_data);
  if (gefx) {
    ti = proto_tree_add_string(tree, hf_h225_debug_dissector_try_string, tvb, offset>>3, 0, gefx->key);
  PROTO_ITEM_SET_HIDDEN(ti);
    dissector_try_string(gef_name_dissector_table, gefx->key, tvb_new_subset(tvb, offset>>3, 0, 0), actx->pinfo, tree, actx);
  }
  actx->private_data = gefx;  /* subdissector could overwrite it */

  return offset;
}



static int
dissect_h225_T_raw(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 893 "./asn1/h225/h225.cnf"
  tvbuff_t *value_tvb;
  gef_ctx_t *gefx;
  proto_item* ti;

  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, &value_tvb);

  gefx = gef_ctx_get(actx->private_data);
  if (gefx) {
    ti = proto_tree_add_string(tree, hf_h225_debug_dissector_try_string, tvb, offset>>3, 0, gefx->key);
  PROTO_ITEM_SET_HIDDEN(ti);
    dissector_try_string(gef_content_dissector_table, gefx->key, value_tvb, actx->pinfo, tree, actx);
  }


  return offset;
}



static int
dissect_h225_IA5String(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_IA5String(tvb, offset, actx, tree, hf_index,
                                          NO_BOUND, NO_BOUND, FALSE);

  return offset;
}



static int
dissect_h225_BMPString(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_BMPString(tvb, offset, actx, tree, hf_index,
                                          NO_BOUND, NO_BOUND, FALSE);

  return offset;
}


static const per_sequence_t SEQUENCE_SIZE_1_512_OF_EnumeratedParameter_sequence_of[1] = {
  { &hf_h225_parameters_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_EnumeratedParameter },
};

static int
dissect_h225_SEQUENCE_SIZE_1_512_OF_EnumeratedParameter(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_h225_SEQUENCE_SIZE_1_512_OF_EnumeratedParameter, SEQUENCE_SIZE_1_512_OF_EnumeratedParameter_sequence_of,
                                                  1, 512, FALSE);

  return offset;
}


static const per_sequence_t SEQUENCE_SIZE_1_16_OF_GenericData_sequence_of[1] = {
  { &hf_h225_nested_item    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_GenericData },
};

static int
dissect_h225_SEQUENCE_SIZE_1_16_OF_GenericData(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_h225_SEQUENCE_SIZE_1_16_OF_GenericData, SEQUENCE_SIZE_1_16_OF_GenericData_sequence_of,
                                                  1, 16, FALSE);

  return offset;
}


static const value_string h225_Content_vals[] = {
  {   0, "raw" },
  {   1, "text" },
  {   2, "unicode" },
  {   3, "bool" },
  {   4, "number8" },
  {   5, "number16" },
  {   6, "number32" },
  {   7, "id" },
  {   8, "alias" },
  {   9, "transport" },
  {  10, "compound" },
  {  11, "nested" },
  { 0, NULL }
};

static const per_choice_t Content_choice[] = {
  {   0, &hf_h225_raw            , ASN1_EXTENSION_ROOT    , dissect_h225_T_raw },
  {   1, &hf_h225_text           , ASN1_EXTENSION_ROOT    , dissect_h225_IA5String },
  {   2, &hf_h225_unicode        , ASN1_EXTENSION_ROOT    , dissect_h225_BMPString },
  {   3, &hf_h225_bool           , ASN1_EXTENSION_ROOT    , dissect_h225_BOOLEAN },
  {   4, &hf_h225_number8        , ASN1_EXTENSION_ROOT    , dissect_h225_INTEGER_0_255 },
  {   5, &hf_h225_number16       , ASN1_EXTENSION_ROOT    , dissect_h225_INTEGER_0_65535 },
  {   6, &hf_h225_number32       , ASN1_EXTENSION_ROOT    , dissect_h225_INTEGER_0_4294967295 },
  {   7, &hf_h225_id             , ASN1_EXTENSION_ROOT    , dissect_h225_GenericIdentifier },
  {   8, &hf_h225_alias          , ASN1_EXTENSION_ROOT    , dissect_h225_AliasAddress },
  {   9, &hf_h225_transport      , ASN1_EXTENSION_ROOT    , dissect_h225_TransportAddress },
  {  10, &hf_h225_compound       , ASN1_EXTENSION_ROOT    , dissect_h225_SEQUENCE_SIZE_1_512_OF_EnumeratedParameter },
  {  11, &hf_h225_nested         , ASN1_EXTENSION_ROOT    , dissect_h225_SEQUENCE_SIZE_1_16_OF_GenericData },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_Content(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_Content, Content_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t EnumeratedParameter_sequence[] = {
  { &hf_h225_id             , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_GenericIdentifier },
  { &hf_h225_content        , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_Content },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_EnumeratedParameter(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 842 "./asn1/h225/h225.cnf"
  gef_ctx_t *parent_gefx;

  parent_gefx = gef_ctx_get(actx->private_data);
  actx->private_data = gef_ctx_alloc(parent_gefx, NULL);

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_EnumeratedParameter, EnumeratedParameter_sequence);

#line 847 "./asn1/h225/h225.cnf"
  actx->private_data = parent_gefx;

  return offset;
}


static const per_sequence_t GenericData_sequence[] = {
  { &hf_h225_id             , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_GenericIdentifier },
  { &hf_h225_parameters     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_SIZE_1_512_OF_EnumeratedParameter },
  { NULL, 0, 0, NULL }
};

int
dissect_h225_GenericData(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 828 "./asn1/h225/h225.cnf"
  void *priv_data = actx->private_data;
  gef_ctx_t *gefx;

  /* check if not inherited from FeatureDescriptor */
  gefx = gef_ctx_get(actx->private_data);
  if (!gefx) {
    gefx = gef_ctx_alloc(NULL, "GenericData");
    actx->private_data = gefx;
  }

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_GenericData, GenericData_sequence);

#line 838 "./asn1/h225/h225.cnf"
  actx->private_data = priv_data;

  return offset;
}


static const per_sequence_t SEQUENCE_OF_GenericData_sequence_of[1] = {
  { &hf_h225_genericData_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_GenericData },
};

static int
dissect_h225_SEQUENCE_OF_GenericData(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_GenericData, SEQUENCE_OF_GenericData_sequence_of);

  return offset;
}


static const per_sequence_t CircuitInfo_sequence[] = {
  { &hf_h225_sourceCircuitID, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_CircuitIdentifier },
  { &hf_h225_destinationCircuitID, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_CircuitIdentifier },
  { &hf_h225_genericData    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { NULL, 0, 0, NULL }
};

int
dissect_h225_CircuitInfo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_CircuitInfo, CircuitInfo_sequence);

  return offset;
}



static int
dissect_h225_FeatureDescriptor(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 852 "./asn1/h225/h225.cnf"
  void *priv_data = actx->private_data;
  actx->private_data = gef_ctx_alloc(NULL, "FeatureDescriptor");

  offset = dissect_h225_GenericData(tvb, offset, actx, tree, hf_index);

#line 855 "./asn1/h225/h225.cnf"
  actx->private_data = priv_data;

  return offset;
}


static const per_sequence_t SEQUENCE_OF_FeatureDescriptor_sequence_of[1] = {
  { &hf_h225_neededFeatures_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_FeatureDescriptor },
};

static int
dissect_h225_SEQUENCE_OF_FeatureDescriptor(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_FeatureDescriptor, SEQUENCE_OF_FeatureDescriptor_sequence_of);

  return offset;
}



static int
dissect_h225_ParallelH245Control_item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 363 "./asn1/h225/h225.cnf"
  tvbuff_t *h245_tvb = NULL;

  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, &h245_tvb);

  next_tvb_add_handle(&h245_list, h245_tvb, (h225_h245_in_tree)?tree:NULL, h245dg_handle);


  return offset;
}


static const per_sequence_t ParallelH245Control_sequence_of[1] = {
  { &hf_h225_ParallelH245Control_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_ParallelH245Control_item },
};

static int
dissect_h225_ParallelH245Control(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_ParallelH245Control, ParallelH245Control_sequence_of);

  return offset;
}


static const per_sequence_t ExtendedAliasAddress_sequence[] = {
  { &hf_h225_extAliasAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_AliasAddress },
  { &hf_h225_presentationIndicator, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_PresentationIndicator },
  { &hf_h225_screeningIndicator, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_ScreeningIndicator },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_ExtendedAliasAddress(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_ExtendedAliasAddress, ExtendedAliasAddress_sequence);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_ExtendedAliasAddress_sequence_of[1] = {
  { &hf_h225_additionalSourceAddresses_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_ExtendedAliasAddress },
};

static int
dissect_h225_SEQUENCE_OF_ExtendedAliasAddress(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_ExtendedAliasAddress, SEQUENCE_OF_ExtendedAliasAddress_sequence_of);

  return offset;
}



static int
dissect_h225_INTEGER_1_31(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 31U, NULL, FALSE);

  return offset;
}



static int
dissect_h225_BMPString_SIZE_1_80(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_BMPString(tvb, offset, actx, tree, hf_index,
                                          1, 80, FALSE);

  return offset;
}


static const per_sequence_t DisplayName_sequence[] = {
  { &hf_h225_language_01    , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_h225_IA5String },
  { &hf_h225_name           , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_BMPString_SIZE_1_80 },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_DisplayName(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_DisplayName, DisplayName_sequence);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_DisplayName_sequence_of[1] = {
  { &hf_h225_displayName_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_DisplayName },
};

static int
dissect_h225_SEQUENCE_OF_DisplayName(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_DisplayName, SEQUENCE_OF_DisplayName_sequence_of);

  return offset;
}


static const per_sequence_t Setup_UUIE_sequence[] = {
  { &hf_h225_protocolIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ProtocolIdentifier },
  { &hf_h225_h245Address    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_H245TransportAddress },
  { &hf_h225_sourceAddress  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_setup_UUIE_sourceInfo, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_EndpointType },
  { &hf_h225_destinationAddress, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_destCallSignalAddress, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_TransportAddress },
  { &hf_h225_destExtraCallInfo, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_destExtraCRV   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CallReferenceValue },
  { &hf_h225_activeMC       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_conferenceID   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ConferenceIdentifier },
  { &hf_h225_conferenceGoal , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_T_conferenceGoal },
  { &hf_h225_callServices   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_QseriesOptions },
  { &hf_h225_callType       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_CallType },
  { &hf_h225_sourceCallSignalAddress, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_TransportAddress },
  { &hf_h225_uUIE_remoteExtensionAddress, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AliasAddress },
  { &hf_h225_callIdentifier , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_CallIdentifier },
  { &hf_h225_h245SecurityCapability, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_H245Security },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_fastStart      , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FastStart },
  { &hf_h225_mediaWaitForConnect, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_canOverlapSend , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_endpointIdentifier, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_EndpointIdentifier },
  { &hf_h225_multipleCalls  , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_maintainConnection, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_connectionParameters, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_T_connectionParameters },
  { &hf_h225_language       , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_Language },
  { &hf_h225_presentationIndicator, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_PresentationIndicator },
  { &hf_h225_screeningIndicator, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ScreeningIndicator },
  { &hf_h225_serviceControl , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ServiceControlSession },
  { &hf_h225_symmetricOperationRequired, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_NULL },
  { &hf_h225_capacity       , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CallCapacity },
  { &hf_h225_circuitInfo    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CircuitInfo },
  { &hf_h225_desiredProtocols, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_SupportedProtocols },
  { &hf_h225_neededFeatures , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_FeatureDescriptor },
  { &hf_h225_desiredFeatures, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_FeatureDescriptor },
  { &hf_h225_supportedFeatures, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_FeatureDescriptor },
  { &hf_h225_parallelH245Control, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ParallelH245Control },
  { &hf_h225_additionalSourceAddresses, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ExtendedAliasAddress },
  { &hf_h225_hopCount_1_31  , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_INTEGER_1_31 },
  { &hf_h225_displayName    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_DisplayName },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_Setup_UUIE(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 470 "./asn1/h225/h225.cnf"
  h225_packet_info* h225_pi;
  contains_faststart = FALSE;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_Setup_UUIE, Setup_UUIE_sequence);

#line 475 "./asn1/h225/h225.cnf"
  /* Add to packet info */
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);
  if (h225_pi != NULL) {
    h225_pi->cs_type = H225_SETUP;
    if (contains_faststart) {
      char temp[50];
      g_snprintf(temp, 50, "%s OLC (%s)", val_to_str(h225_pi->cs_type, T_h323_message_body_vals, "<unknown>"), h225_pi->frame_label);
      g_strlcpy(h225_pi->frame_label, temp, 50);
    } else
      g_snprintf(h225_pi->frame_label, 50, "%s", val_to_str(h225_pi->cs_type, T_h323_message_body_vals, "<unknown>"));
  }

  return offset;
}


static const per_sequence_t FeatureSet_sequence[] = {
  { &hf_h225_replacementFeatureSet, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_neededFeatures , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_FeatureDescriptor },
  { &hf_h225_desiredFeatures, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_FeatureDescriptor },
  { &hf_h225_supportedFeatures, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_FeatureDescriptor },
  { NULL, 0, 0, NULL }
};

int
dissect_h225_FeatureSet(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_FeatureSet, FeatureSet_sequence);

  return offset;
}


static const per_sequence_t CallProceeding_UUIE_sequence[] = {
  { &hf_h225_protocolIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ProtocolIdentifier },
  { &hf_h225_uUIE_destinationInfo, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_EndpointType },
  { &hf_h225_h245Address    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_H245TransportAddress },
  { &hf_h225_callIdentifier , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_CallIdentifier },
  { &hf_h225_h245SecurityMode, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_H245Security },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_fastStart      , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FastStart },
  { &hf_h225_multipleCalls  , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_maintainConnection, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_fastConnectRefused, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_NULL },
  { &hf_h225_featureSet     , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FeatureSet },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_CallProceeding_UUIE(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 489 "./asn1/h225/h225.cnf"
  h225_packet_info* h225_pi;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_CallProceeding_UUIE, CallProceeding_UUIE_sequence);

#line 493 "./asn1/h225/h225.cnf"
  /* Add to packet info */
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);
  if (h225_pi != NULL) {
    h225_pi->cs_type = H225_CALL_PROCEDING;
    if (contains_faststart) {
      char temp[50];
      g_snprintf(temp, 50, "%s OLC (%s)", val_to_str(h225_pi->cs_type, T_h323_message_body_vals, "<unknown>"), h225_pi->frame_label);
      g_strlcpy(h225_pi->frame_label, temp, 50);
    } else
      g_snprintf(h225_pi->frame_label, 50, "%s", val_to_str(h225_pi->cs_type, T_h323_message_body_vals, "<unknown>"));
  }

  return offset;
}


static const per_sequence_t Connect_UUIE_sequence[] = {
  { &hf_h225_protocolIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ProtocolIdentifier },
  { &hf_h225_h245Address    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_H245TransportAddress },
  { &hf_h225_uUIE_destinationInfo, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_EndpointType },
  { &hf_h225_conferenceID   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ConferenceIdentifier },
  { &hf_h225_callIdentifier , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_CallIdentifier },
  { &hf_h225_h245SecurityMode, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_H245Security },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_fastStart      , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FastStart },
  { &hf_h225_multipleCalls  , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_maintainConnection, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_language       , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_Language },
  { &hf_h225_connectedAddress, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_presentationIndicator, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_PresentationIndicator },
  { &hf_h225_screeningIndicator, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ScreeningIndicator },
  { &hf_h225_fastConnectRefused, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_NULL },
  { &hf_h225_serviceControl , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ServiceControlSession },
  { &hf_h225_capacity       , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CallCapacity },
  { &hf_h225_featureSet     , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FeatureSet },
  { &hf_h225_displayName    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_DisplayName },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_Connect_UUIE(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 538 "./asn1/h225/h225.cnf"
  h225_packet_info* h225_pi;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_Connect_UUIE, Connect_UUIE_sequence);

#line 542 "./asn1/h225/h225.cnf"
  /* Add to packet info */
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);
  if (h225_pi != NULL) {
    h225_pi->cs_type = H225_CONNECT;
    if (contains_faststart) {
      char temp[50];
      g_snprintf(temp, 50, "%s OLC (%s)", val_to_str(h225_pi->cs_type, T_h323_message_body_vals, "<unknown>"), h225_pi->frame_label);
      g_strlcpy(h225_pi->frame_label, temp, 50);
    } else
      g_snprintf(h225_pi->frame_label, 50, "%s", val_to_str(h225_pi->cs_type, T_h323_message_body_vals, "<unknown>"));
  }

  return offset;
}


static const per_sequence_t Alerting_UUIE_sequence[] = {
  { &hf_h225_protocolIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ProtocolIdentifier },
  { &hf_h225_uUIE_destinationInfo, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_EndpointType },
  { &hf_h225_h245Address    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_H245TransportAddress },
  { &hf_h225_callIdentifier , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_CallIdentifier },
  { &hf_h225_h245SecurityMode, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_H245Security },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_fastStart      , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FastStart },
  { &hf_h225_multipleCalls  , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_maintainConnection, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_alertingAddress, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_presentationIndicator, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_PresentationIndicator },
  { &hf_h225_screeningIndicator, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ScreeningIndicator },
  { &hf_h225_fastConnectRefused, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_NULL },
  { &hf_h225_serviceControl , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ServiceControlSession },
  { &hf_h225_capacity       , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CallCapacity },
  { &hf_h225_featureSet     , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FeatureSet },
  { &hf_h225_displayName    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_DisplayName },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_Alerting_UUIE(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 507 "./asn1/h225/h225.cnf"
  h225_packet_info* h225_pi;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_Alerting_UUIE, Alerting_UUIE_sequence);

#line 511 "./asn1/h225/h225.cnf"
  /* Add to packet info */
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);
  if (h225_pi != NULL) {
    h225_pi->cs_type = H225_ALERTING;
    if (contains_faststart) {
      char temp[50];
      g_snprintf(temp, 50, "%s OLC (%s)", val_to_str(h225_pi->cs_type, T_h323_message_body_vals, "<unknown>"), h225_pi->frame_label);
      g_strlcpy(h225_pi->frame_label, temp, 50);
    } else
      g_snprintf(h225_pi->frame_label, 50, "%s", val_to_str(h225_pi->cs_type, T_h323_message_body_vals, "<unknown>"));
  }

  return offset;
}


static const per_sequence_t Information_UUIE_sequence[] = {
  { &hf_h225_protocolIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ProtocolIdentifier },
  { &hf_h225_callIdentifier , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_CallIdentifier },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_fastStart      , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FastStart },
  { &hf_h225_fastConnectRefused, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_NULL },
  { &hf_h225_circuitInfo    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CircuitInfo },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_Information_UUIE(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 401 "./asn1/h225/h225.cnf"
  h225_packet_info* h225_pi;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_Information_UUIE, Information_UUIE_sequence);

#line 405 "./asn1/h225/h225.cnf"
  /* Add to packet info */
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);
  if (h225_pi != NULL) {
    h225_pi->cs_type = H225_INFORMATION;
    g_snprintf(h225_pi->frame_label, 50, "%s", val_to_str(h225_pi->cs_type, T_h323_message_body_vals, "<unknown>"));
  }

  return offset;
}


static const value_string h225_SecurityErrors_vals[] = {
  {   0, "securityWrongSyncTime" },
  {   1, "securityReplay" },
  {   2, "securityWrongGeneralID" },
  {   3, "securityWrongSendersID" },
  {   4, "securityIntegrityFailed" },
  {   5, "securityWrongOID" },
  {   6, "securityDHmismatch" },
  {   7, "securityCertificateExpired" },
  {   8, "securityCertificateDateInvalid" },
  {   9, "securityCertificateRevoked" },
  {  10, "securityCertificateNotReadable" },
  {  11, "securityCertificateSignatureInvalid" },
  {  12, "securityCertificateMissing" },
  {  13, "securityCertificateIncomplete" },
  {  14, "securityUnsupportedCertificateAlgOID" },
  {  15, "securityUnknownCA" },
  { 0, NULL }
};

static const per_choice_t SecurityErrors_choice[] = {
  {   0, &hf_h225_securityWrongSyncTime, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_securityReplay , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_securityWrongGeneralID, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   3, &hf_h225_securityWrongSendersID, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   4, &hf_h225_securityIntegrityFailed, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   5, &hf_h225_securityWrongOID, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   6, &hf_h225_securityDHmismatch, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   7, &hf_h225_securityCertificateExpired, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   8, &hf_h225_securityCertificateDateInvalid, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   9, &hf_h225_securityCertificateRevoked, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {  10, &hf_h225_securityCertificateNotReadable, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {  11, &hf_h225_securityCertificateSignatureInvalid, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {  12, &hf_h225_securityCertificateMissing, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {  13, &hf_h225_securityCertificateIncomplete, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {  14, &hf_h225_securityUnsupportedCertificateAlgOID, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {  15, &hf_h225_securityUnknownCA, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_SecurityErrors(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_SecurityErrors, SecurityErrors_choice,
                                 NULL);

  return offset;
}


const value_string h225_ReleaseCompleteReason_vals[] = {
  {   0, "noBandwidth" },
  {   1, "gatekeeperResources" },
  {   2, "unreachableDestination" },
  {   3, "destinationRejection" },
  {   4, "invalidRevision" },
  {   5, "noPermission" },
  {   6, "unreachableGatekeeper" },
  {   7, "gatewayResources" },
  {   8, "badFormatAddress" },
  {   9, "adaptiveBusy" },
  {  10, "inConf" },
  {  11, "undefinedReason" },
  {  12, "facilityCallDeflection" },
  {  13, "securityDenied" },
  {  14, "calledPartyNotRegistered" },
  {  15, "callerNotRegistered" },
  {  16, "newConnectionNeeded" },
  {  17, "nonStandardReason" },
  {  18, "replaceWithConferenceInvite" },
  {  19, "genericDataReason" },
  {  20, "neededFeatureNotSupported" },
  {  21, "tunnelledSignallingRejected" },
  {  22, "invalidCID" },
  {  23, "securityError" },
  {  24, "hopCountExceeded" },
  { 0, NULL }
};

static const per_choice_t ReleaseCompleteReason_choice[] = {
  {   0, &hf_h225_noBandwidth    , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_gatekeeperResources, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_unreachableDestination, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   3, &hf_h225_destinationRejection, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   4, &hf_h225_invalidRevision, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   5, &hf_h225_noPermission   , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   6, &hf_h225_unreachableGatekeeper, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   7, &hf_h225_gatewayResources, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   8, &hf_h225_badFormatAddress, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   9, &hf_h225_adaptiveBusy   , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {  10, &hf_h225_inConf         , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {  11, &hf_h225_undefinedReason, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {  12, &hf_h225_facilityCallDeflection, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  13, &hf_h225_securityDenied , ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  14, &hf_h225_calledPartyNotRegistered, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  15, &hf_h225_callerNotRegistered, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  16, &hf_h225_newConnectionNeeded, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  17, &hf_h225_nonStandardReason, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NonStandardParameter },
  {  18, &hf_h225_replaceWithConferenceInvite, ASN1_NOT_EXTENSION_ROOT, dissect_h225_ConferenceIdentifier },
  {  19, &hf_h225_genericDataReason, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  20, &hf_h225_neededFeatureNotSupported, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  21, &hf_h225_tunnelledSignallingRejected, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  22, &hf_h225_invalidCID     , ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  23, &hf_h225_rLC_securityError, ASN1_NOT_EXTENSION_ROOT, dissect_h225_SecurityErrors },
  {  24, &hf_h225_hopCountExceeded, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

int
dissect_h225_ReleaseCompleteReason(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 713 "./asn1/h225/h225.cnf"
  gint32 value;
  h225_packet_info* h225_pi;
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);

  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_ReleaseCompleteReason, ReleaseCompleteReason_choice,
                                 &value);

  if (h225_pi != NULL) {
    h225_pi->reason = value;
  }


  return offset;
}


static const per_sequence_t ReleaseComplete_UUIE_sequence[] = {
  { &hf_h225_protocolIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ProtocolIdentifier },
  { &hf_h225_releaseCompleteReason, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_ReleaseCompleteReason },
  { &hf_h225_callIdentifier , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_CallIdentifier },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_busyAddress    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_presentationIndicator, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_PresentationIndicator },
  { &hf_h225_screeningIndicator, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ScreeningIndicator },
  { &hf_h225_capacity       , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CallCapacity },
  { &hf_h225_serviceControl , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ServiceControlSession },
  { &hf_h225_featureSet     , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FeatureSet },
  { &hf_h225_destinationInfo, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_EndpointType },
  { &hf_h225_displayName    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_DisplayName },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_ReleaseComplete_UUIE(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 525 "./asn1/h225/h225.cnf"
  h225_packet_info* h225_pi;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_ReleaseComplete_UUIE, ReleaseComplete_UUIE_sequence);

#line 529 "./asn1/h225/h225.cnf"
  /* Add to packet info */
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);
  if (h225_pi != NULL) {
    h225_pi->cs_type = H225_RELEASE_COMPLET;
    g_snprintf(h225_pi->frame_label, 50, "%s", val_to_str(h225_pi->cs_type, T_h323_message_body_vals, "<unknown>"));
  }

  return offset;
}


const value_string FacilityReason_vals[] = {
  {   0, "routeCallToGatekeeper" },
  {   1, "callForwarded" },
  {   2, "routeCallToMC" },
  {   3, "undefinedReason" },
  {   4, "conferenceListChoice" },
  {   5, "startH245" },
  {   6, "noH245" },
  {   7, "newTokens" },
  {   8, "featureSetUpdate" },
  {   9, "forwardedElements" },
  {  10, "transportedInformation" },
  { 0, NULL }
};

static const per_choice_t FacilityReason_choice[] = {
  {   0, &hf_h225_routeCallToGatekeeper, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_callForwarded  , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_routeCallToMC  , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   3, &hf_h225_undefinedReason, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   4, &hf_h225_conferenceListChoice, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {   5, &hf_h225_startH245      , ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {   6, &hf_h225_noH245         , ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {   7, &hf_h225_newTokens      , ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {   8, &hf_h225_featureSetUpdate, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {   9, &hf_h225_forwardedElements, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  10, &hf_h225_transportedInformation, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_FacilityReason(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 593 "./asn1/h225/h225.cnf"
  gint32 value;
  h225_packet_info* h225_pi;
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);

  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_FacilityReason, FacilityReason_choice,
                                 &value);

  if (h225_pi != NULL) {
    h225_pi->reason = value;
  }


  return offset;
}


static const per_sequence_t ConferenceList_sequence[] = {
  { &hf_h225_conferenceID   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_ConferenceIdentifier },
  { &hf_h225_conferenceAlias, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_AliasAddress },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_ConferenceList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_ConferenceList, ConferenceList_sequence);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_ConferenceList_sequence_of[1] = {
  { &hf_h225_conferences_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_ConferenceList },
};

static int
dissect_h225_SEQUENCE_OF_ConferenceList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_ConferenceList, SEQUENCE_OF_ConferenceList_sequence_of);

  return offset;
}


static const per_sequence_t Facility_UUIE_sequence[] = {
  { &hf_h225_protocolIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ProtocolIdentifier },
  { &hf_h225_alternativeAddress, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_TransportAddress },
  { &hf_h225_alternativeAliasAddress, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_conferenceID   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_ConferenceIdentifier },
  { &hf_h225_facilityReason , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_FacilityReason },
  { &hf_h225_callIdentifier , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_CallIdentifier },
  { &hf_h225_destExtraCallInfo, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_uUIE_remoteExtensionAddress, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AliasAddress },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_conferences    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ConferenceList },
  { &hf_h225_h245Address    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_H245TransportAddress },
  { &hf_h225_fastStart      , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FastStart },
  { &hf_h225_multipleCalls  , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_maintainConnection, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_fastConnectRefused, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_NULL },
  { &hf_h225_serviceControl , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ServiceControlSession },
  { &hf_h225_circuitInfo    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CircuitInfo },
  { &hf_h225_featureSet     , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FeatureSet },
  { &hf_h225_uUIE_destinationInfo, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_EndpointType },
  { &hf_h225_h245SecurityMode, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_H245Security },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_Facility_UUIE(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 445 "./asn1/h225/h225.cnf"
  h225_packet_info* h225_pi;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_Facility_UUIE, Facility_UUIE_sequence);

#line 449 "./asn1/h225/h225.cnf"
  /* Add to packet info */
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);
  if (h225_pi != NULL) {
    h225_pi->cs_type = H225_FACILITY;
    g_snprintf(h225_pi->frame_label, 50, "%s", val_to_str(h225_pi->cs_type, T_h323_message_body_vals, "<unknown>"));
  }

  return offset;
}


static const per_sequence_t Progress_UUIE_sequence[] = {
  { &hf_h225_protocolIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ProtocolIdentifier },
  { &hf_h225_uUIE_destinationInfo, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_EndpointType },
  { &hf_h225_h245Address    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_H245TransportAddress },
  { &hf_h225_callIdentifier , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_CallIdentifier },
  { &hf_h225_h245SecurityMode, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_H245Security },
  { &hf_h225_tokens         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_fastStart      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_FastStart },
  { &hf_h225_multipleCalls  , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_maintainConnection, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_fastConnectRefused, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_NULL },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_Progress_UUIE(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 414 "./asn1/h225/h225.cnf"
  h225_packet_info* h225_pi;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_Progress_UUIE, Progress_UUIE_sequence);

#line 418 "./asn1/h225/h225.cnf"
  /* Add to packet info */
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);
  if (h225_pi != NULL) {
    h225_pi->cs_type = H225_PROGRESS;
    if (contains_faststart) {
      char temp[50];
      g_snprintf(temp, 50, "%s OLC (%s)", val_to_str(h225_pi->cs_type, T_h323_message_body_vals, "<unknown>"), h225_pi->frame_label);
      g_strlcpy(h225_pi->frame_label, temp, 50);
    } else
      g_snprintf(h225_pi->frame_label, 50, "%s", val_to_str(h225_pi->cs_type, T_h323_message_body_vals, "<unknown>"));
  }

  return offset;
}



static int
dissect_h225_T_empty_flg(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 377 "./asn1/h225/h225.cnf"
  h225_packet_info* h225_pi;

  offset = dissect_per_null(tvb, offset, actx, tree, hf_index);

#line 381 "./asn1/h225/h225.cnf"
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);
  if (h225_pi != NULL) {
    h225_pi->cs_type = H225_EMPTY;
  }

  return offset;
}


static const per_sequence_t Status_UUIE_sequence[] = {
  { &hf_h225_protocolIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ProtocolIdentifier },
  { &hf_h225_callIdentifier , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_CallIdentifier },
  { &hf_h225_tokens         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_Status_UUIE(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 388 "./asn1/h225/h225.cnf"
  h225_packet_info* h225_pi;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_Status_UUIE, Status_UUIE_sequence);

#line 392 "./asn1/h225/h225.cnf"
  /* Add to packet info */
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);
  if (h225_pi != NULL) {
    h225_pi->cs_type = H225_STATUS;
    g_snprintf(h225_pi->frame_label, 50, "%s", val_to_str(h225_pi->cs_type, T_h323_message_body_vals, "<unknown>"));
  }

  return offset;
}


static const per_sequence_t StatusInquiry_UUIE_sequence[] = {
  { &hf_h225_protocolIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ProtocolIdentifier },
  { &hf_h225_callIdentifier , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_CallIdentifier },
  { &hf_h225_tokens         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_StatusInquiry_UUIE(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_StatusInquiry_UUIE, StatusInquiry_UUIE_sequence);

  return offset;
}


static const per_sequence_t SetupAcknowledge_UUIE_sequence[] = {
  { &hf_h225_protocolIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ProtocolIdentifier },
  { &hf_h225_callIdentifier , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_CallIdentifier },
  { &hf_h225_tokens         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_SetupAcknowledge_UUIE(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 432 "./asn1/h225/h225.cnf"
  h225_packet_info* h225_pi;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_SetupAcknowledge_UUIE, SetupAcknowledge_UUIE_sequence);

#line 436 "./asn1/h225/h225.cnf"
  /* Add to packet info */
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);
  if (h225_pi != NULL) {
    h225_pi->cs_type = H225_SETUP_ACK;
    g_snprintf(h225_pi->frame_label, 50, "%s", val_to_str(h225_pi->cs_type, T_h323_message_body_vals, "<unknown>"));
  }

  return offset;
}


static const per_sequence_t Notify_UUIE_sequence[] = {
  { &hf_h225_protocolIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ProtocolIdentifier },
  { &hf_h225_callIdentifier , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_CallIdentifier },
  { &hf_h225_tokens         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_connectedAddress, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_presentationIndicator, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_PresentationIndicator },
  { &hf_h225_screeningIndicator, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ScreeningIndicator },
  { &hf_h225_destinationInfo, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_EndpointType },
  { &hf_h225_displayName    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_DisplayName },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_Notify_UUIE(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_Notify_UUIE, Notify_UUIE_sequence);

  return offset;
}


const value_string T_h323_message_body_vals[] = {
  {   0, "setup" },
  {   1, "callProceeding" },
  {   2, "connect" },
  {   3, "alerting" },
  {   4, "information" },
  {   5, "releaseComplete" },
  {   6, "facility" },
  {   7, "progress" },
  {   8, "empty" },
  {   9, "status" },
  {  10, "statusInquiry" },
  {  11, "setupAcknowledge" },
  {  12, "notify" },
  { 0, NULL }
};

static const per_choice_t T_h323_message_body_choice[] = {
  {   0, &hf_h225_setup          , ASN1_EXTENSION_ROOT    , dissect_h225_Setup_UUIE },
  {   1, &hf_h225_callProceeding , ASN1_EXTENSION_ROOT    , dissect_h225_CallProceeding_UUIE },
  {   2, &hf_h225_connect        , ASN1_EXTENSION_ROOT    , dissect_h225_Connect_UUIE },
  {   3, &hf_h225_alerting       , ASN1_EXTENSION_ROOT    , dissect_h225_Alerting_UUIE },
  {   4, &hf_h225_information    , ASN1_EXTENSION_ROOT    , dissect_h225_Information_UUIE },
  {   5, &hf_h225_releaseComplete, ASN1_EXTENSION_ROOT    , dissect_h225_ReleaseComplete_UUIE },
  {   6, &hf_h225_facility       , ASN1_EXTENSION_ROOT    , dissect_h225_Facility_UUIE },
  {   7, &hf_h225_progress       , ASN1_NOT_EXTENSION_ROOT, dissect_h225_Progress_UUIE },
  {   8, &hf_h225_empty_flg      , ASN1_NOT_EXTENSION_ROOT, dissect_h225_T_empty_flg },
  {   9, &hf_h225_status         , ASN1_NOT_EXTENSION_ROOT, dissect_h225_Status_UUIE },
  {  10, &hf_h225_statusInquiry  , ASN1_NOT_EXTENSION_ROOT, dissect_h225_StatusInquiry_UUIE },
  {  11, &hf_h225_setupAcknowledge, ASN1_NOT_EXTENSION_ROOT, dissect_h225_SetupAcknowledge_UUIE },
  {  12, &hf_h225_notify         , ASN1_NOT_EXTENSION_ROOT, dissect_h225_Notify_UUIE },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_T_h323_message_body(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 241 "./asn1/h225/h225.cnf"
  gint32 message_body_val;
  h225_packet_info* h225_pi;

  contains_faststart = FALSE;
  call_id_guid = NULL;
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_T_h323_message_body, T_h323_message_body_choice,
                                 &message_body_val);

  col_append_fstr(actx->pinfo->cinfo, COL_INFO, "CS: %s ",
    val_to_str(message_body_val, T_h323_message_body_vals, "<unknown>"));

  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);
  if (h225_pi != NULL) {
    if (h225_pi->msg_type == H225_CS) {
      /* Don't override msg_tag value from IRR */
      h225_pi->msg_tag = message_body_val;
    }

    if (call_id_guid) {
      h225_pi->guid = *call_id_guid;
    }
  }

  if (contains_faststart == TRUE )
  {
    col_append_str(actx->pinfo->cinfo, COL_INFO, "OpenLogicalChannel " );
  }

  col_set_fence(actx->pinfo->cinfo,COL_INFO);



  return offset;
}



static int
dissect_h225_T_h4501SupplementaryService_item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 748 "./asn1/h225/h225.cnf"
  tvbuff_t *h4501_tvb = NULL;

  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, &h4501_tvb);

  if (h4501_tvb && tvb_reported_length(h4501_tvb)) {
    call_dissector(h4501_handle, h4501_tvb, actx->pinfo, tree);
  }


  return offset;
}


static const per_sequence_t T_h4501SupplementaryService_sequence_of[1] = {
  { &hf_h225_h4501SupplementaryService_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_T_h4501SupplementaryService_item },
};

static int
dissect_h225_T_h4501SupplementaryService(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_T_h4501SupplementaryService, T_h4501SupplementaryService_sequence_of);

  return offset;
}



static int
dissect_h225_T_h245Tunnelling(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 458 "./asn1/h225/h225.cnf"
  h225_packet_info* h225_pi;
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);
  if (h225_pi != NULL) {

  offset = dissect_per_boolean(tvb, offset, actx, tree, hf_index, &(h225_pi->is_h245Tunneling));

#line 466 "./asn1/h225/h225.cnf"
  }

  return offset;
}



static int
dissect_h225_H245Control_item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 370 "./asn1/h225/h225.cnf"
  tvbuff_t *h245_tvb = NULL;

  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, &h245_tvb);

  next_tvb_add_handle(&h245_list, h245_tvb, (h225_h245_in_tree)?tree:NULL, h245dg_handle);


  return offset;
}


static const per_sequence_t H245Control_sequence_of[1] = {
  { &hf_h225_H245Control_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_H245Control_item },
};

static int
dissect_h225_H245Control(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_H245Control, H245Control_sequence_of);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_NonStandardParameter_sequence_of[1] = {
  { &hf_h225_nonStandardControl_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_NonStandardParameter },
};

static int
dissect_h225_SEQUENCE_OF_NonStandardParameter(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_NonStandardParameter, SEQUENCE_OF_NonStandardParameter_sequence_of);

  return offset;
}


static const per_sequence_t CallLinkage_sequence[] = {
  { &hf_h225_globalCallId   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_GloballyUniqueID },
  { &hf_h225_threadId       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_GloballyUniqueID },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_CallLinkage(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_CallLinkage, CallLinkage_sequence);

  return offset;
}



static int
dissect_h225_T_messageContent_item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 770 "./asn1/h225/h225.cnf"
  tvbuff_t *next_tvb = NULL;

  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, &next_tvb);

  next_tvb_add_handle(&tp_list, next_tvb, (h225_tp_in_tree)?tree:NULL, tp_handle);


  return offset;
}


static const per_sequence_t T_messageContent_sequence_of[1] = {
  { &hf_h225_messageContent_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_T_messageContent_item },
};

static int
dissect_h225_T_messageContent(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_T_messageContent, T_messageContent_sequence_of);

  return offset;
}


static const per_sequence_t T_tunnelledSignallingMessage_sequence[] = {
  { &hf_h225_tunnelledProtocolID, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_TunnelledProtocol },
  { &hf_h225_messageContent , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_T_messageContent },
  { &hf_h225_tunnellingRequired, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NULL },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_T_tunnelledSignallingMessage(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 768 "./asn1/h225/h225.cnf"
  tp_handle = NULL;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_T_tunnelledSignallingMessage, T_tunnelledSignallingMessage_sequence);

  return offset;
}



static int
dissect_h225_OCTET_STRING(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, NULL);

  return offset;
}


static const per_sequence_t StimulusControl_sequence[] = {
  { &hf_h225_nonStandard    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_isText         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NULL },
  { &hf_h225_h248Message    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_OCTET_STRING },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_StimulusControl(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_StimulusControl, StimulusControl_sequence);

  return offset;
}


static const per_sequence_t H323_UU_PDU_sequence[] = {
  { &hf_h225_h323_message_body, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_T_h323_message_body },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_h4501SupplementaryService, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_T_h4501SupplementaryService },
  { &hf_h225_h245Tunnelling , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_T_h245Tunnelling },
  { &hf_h225_h245Control    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_H245Control },
  { &hf_h225_nonStandardControl, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_NonStandardParameter },
  { &hf_h225_callLinkage    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CallLinkage },
  { &hf_h225_tunnelledSignallingMessage, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_T_tunnelledSignallingMessage },
  { &hf_h225_provisionalRespToH245Tunnelling, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_NULL },
  { &hf_h225_stimulusControl, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_StimulusControl },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_H323_UU_PDU(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_H323_UU_PDU, H323_UU_PDU_sequence);

  return offset;
}



static int
dissect_h225_OCTET_STRING_SIZE_1_131(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       1, 131, FALSE, NULL);

  return offset;
}


static const per_sequence_t T_user_data_sequence[] = {
  { &hf_h225_protocol_discriminator, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_INTEGER_0_255 },
  { &hf_h225_user_information, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_OCTET_STRING_SIZE_1_131 },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_T_user_data(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_T_user_data, T_user_data_sequence);

  return offset;
}


static const per_sequence_t H323_UserInformation_sequence[] = {
  { &hf_h225_h323_uu_pdu    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_H323_UU_PDU },
  { &hf_h225_user_data      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_T_user_data },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_H323_UserInformation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_H323_UserInformation, H323_UserInformation_sequence);

  return offset;
}


static const per_sequence_t T_range_sequence[] = {
  { &hf_h225_startOfRange   , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_PartyNumber },
  { &hf_h225_endOfRange     , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_PartyNumber },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_T_range(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_T_range, T_range_sequence);

  return offset;
}


static const value_string h225_AddressPattern_vals[] = {
  {   0, "wildcard" },
  {   1, "range" },
  { 0, NULL }
};

static const per_choice_t AddressPattern_choice[] = {
  {   0, &hf_h225_wildcard       , ASN1_EXTENSION_ROOT    , dissect_h225_AliasAddress },
  {   1, &hf_h225_range          , ASN1_EXTENSION_ROOT    , dissect_h225_T_range },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_AddressPattern(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_AddressPattern, AddressPattern_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_TransportAddress_sequence_of[1] = {
  { &hf_h225_callSignalAddress_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_TransportAddress },
};

static int
dissect_h225_SEQUENCE_OF_TransportAddress(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_TransportAddress, SEQUENCE_OF_TransportAddress_sequence_of);

  return offset;
}



static int
dissect_h225_INTEGER_0_127(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 127U, NULL, FALSE);

  return offset;
}


static const per_sequence_t AlternateTransportAddresses_sequence[] = {
  { &hf_h225_annexE         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_TransportAddress },
  { &hf_h225_sctp           , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_TransportAddress },
  { NULL, 0, 0, NULL }
};

int
dissect_h225_AlternateTransportAddresses(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_AlternateTransportAddresses, AlternateTransportAddresses_sequence);

  return offset;
}


static const per_sequence_t Endpoint_sequence[] = {
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_aliasAddress   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_callSignalAddress, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_TransportAddress },
  { &hf_h225_rasAddress     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_TransportAddress },
  { &hf_h225_endpointType   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_EndpointType },
  { &hf_h225_tokens         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_priority       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_INTEGER_0_127 },
  { &hf_h225_remoteExtensionAddress, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_destExtraCallInfo, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_alternateTransportAddresses, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AlternateTransportAddresses },
  { &hf_h225_circuitInfo    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CircuitInfo },
  { &hf_h225_featureSet     , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FeatureSet },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_Endpoint(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_Endpoint, Endpoint_sequence);

  return offset;
}


static const value_string h225_UseSpecifiedTransport_vals[] = {
  {   0, "tcp" },
  {   1, "annexE" },
  {   2, "sctp" },
  { 0, NULL }
};

static const per_choice_t UseSpecifiedTransport_choice[] = {
  {   0, &hf_h225_tcp            , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_annexE_flg     , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_sctp_flg       , ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_UseSpecifiedTransport(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_UseSpecifiedTransport, UseSpecifiedTransport_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t AlternateGK_sequence[] = {
  { &hf_h225_alternateGK_rasAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_TransportAddress },
  { &hf_h225_gatekeeperIdentifier, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_GatekeeperIdentifier },
  { &hf_h225_needToRegister , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_priority       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_INTEGER_0_127 },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_AlternateGK(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_AlternateGK, AlternateGK_sequence);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_AlternateGK_sequence_of[1] = {
  { &hf_h225_alternateGatekeeper_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_AlternateGK },
};

static int
dissect_h225_SEQUENCE_OF_AlternateGK(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_AlternateGK, SEQUENCE_OF_AlternateGK_sequence_of);

  return offset;
}


static const per_sequence_t AltGKInfo_sequence[] = {
  { &hf_h225_alternateGatekeeper, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_SEQUENCE_OF_AlternateGK },
  { &hf_h225_altGKisPermanent, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_AltGKInfo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_AltGKInfo, AltGKInfo_sequence);

  return offset;
}


static const value_string h225_SecurityErrors2_vals[] = {
  {   0, "securityWrongSyncTime" },
  {   1, "securityReplay" },
  {   2, "securityWrongGeneralID" },
  {   3, "securityWrongSendersID" },
  {   4, "securityIntegrityFailed" },
  {   5, "securityWrongOID" },
  { 0, NULL }
};

static const per_choice_t SecurityErrors2_choice[] = {
  {   0, &hf_h225_securityWrongSyncTime, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_securityReplay , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_securityWrongGeneralID, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   3, &hf_h225_securityWrongSendersID, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   4, &hf_h225_securityIntegrityFailed, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   5, &hf_h225_securityWrongOID, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_SecurityErrors2(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_SecurityErrors2, SecurityErrors2_choice,
                                 NULL);

  return offset;
}



static int
dissect_h225_RequestSeqNum(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 736 "./asn1/h225/h225.cnf"
  h225_packet_info* h225_pi;
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);
  if (h225_pi != NULL) {

  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 65535U, &(h225_pi->requestSeqNum), FALSE);

#line 744 "./asn1/h225/h225.cnf"
  }

  return offset;
}



int
dissect_h225_TimeToLive(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 4294967295U, NULL, FALSE);

  return offset;
}



static int
dissect_h225_H248PackagesDescriptor(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, NULL);

  return offset;
}


static const value_string h225_EncryptIntAlg_vals[] = {
  {   0, "nonStandard" },
  {   1, "isoAlgorithm" },
  { 0, NULL }
};

static const per_choice_t EncryptIntAlg_choice[] = {
  {   0, &hf_h225_nonStandard    , ASN1_EXTENSION_ROOT    , dissect_h225_NonStandardParameter },
  {   1, &hf_h225_isoAlgorithm   , ASN1_EXTENSION_ROOT    , dissect_h225_OBJECT_IDENTIFIER },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_EncryptIntAlg(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_EncryptIntAlg, EncryptIntAlg_choice,
                                 NULL);

  return offset;
}


static const value_string h225_NonIsoIntegrityMechanism_vals[] = {
  {   0, "hMAC-MD5" },
  {   1, "hMAC-iso10118-2-s" },
  {   2, "hMAC-iso10118-2-l" },
  {   3, "hMAC-iso10118-3" },
  { 0, NULL }
};

static const per_choice_t NonIsoIntegrityMechanism_choice[] = {
  {   0, &hf_h225_hMAC_MD5       , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_hMAC_iso10118_2_s, ASN1_EXTENSION_ROOT    , dissect_h225_EncryptIntAlg },
  {   2, &hf_h225_hMAC_iso10118_2_l, ASN1_EXTENSION_ROOT    , dissect_h225_EncryptIntAlg },
  {   3, &hf_h225_hMAC_iso10118_3, ASN1_EXTENSION_ROOT    , dissect_h225_OBJECT_IDENTIFIER },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_NonIsoIntegrityMechanism(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_NonIsoIntegrityMechanism, NonIsoIntegrityMechanism_choice,
                                 NULL);

  return offset;
}


const value_string h225_IntegrityMechanism_vals[] = {
  {   0, "nonStandard" },
  {   1, "digSig" },
  {   2, "iso9797" },
  {   3, "nonIsoIM" },
  { 0, NULL }
};

static const per_choice_t IntegrityMechanism_choice[] = {
  {   0, &hf_h225_nonStandard    , ASN1_EXTENSION_ROOT    , dissect_h225_NonStandardParameter },
  {   1, &hf_h225_digSig         , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_iso9797        , ASN1_EXTENSION_ROOT    , dissect_h225_OBJECT_IDENTIFIER },
  {   3, &hf_h225_nonIsoIM       , ASN1_EXTENSION_ROOT    , dissect_h225_NonIsoIntegrityMechanism },
  { 0, NULL, 0, NULL }
};

int
dissect_h225_IntegrityMechanism(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_IntegrityMechanism, IntegrityMechanism_choice,
                                 NULL);

  return offset;
}



static int
dissect_h225_BIT_STRING(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     NO_BOUND, NO_BOUND, FALSE, NULL, NULL);

  return offset;
}


static const per_sequence_t ICV_sequence[] = {
  { &hf_h225_algorithmOID   , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_OBJECT_IDENTIFIER },
  { &hf_h225_icv            , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_BIT_STRING },
  { NULL, 0, 0, NULL }
};

int
dissect_h225_ICV(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_ICV, ICV_sequence);

  return offset;
}


static const per_sequence_t CapacityReportingCapability_sequence[] = {
  { &hf_h225_canReportCallCapacity, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_CapacityReportingCapability(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_CapacityReportingCapability, CapacityReportingCapability_sequence);

  return offset;
}


static const per_sequence_t CapacityReportingSpecification_when_sequence[] = {
  { &hf_h225_callStart      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NULL },
  { &hf_h225_callEnd        , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NULL },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_CapacityReportingSpecification_when(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_CapacityReportingSpecification_when, CapacityReportingSpecification_when_sequence);

  return offset;
}


static const per_sequence_t CapacityReportingSpecification_sequence[] = {
  { &hf_h225_capacityReportingSpecification_when, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_CapacityReportingSpecification_when },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_CapacityReportingSpecification(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_CapacityReportingSpecification, CapacityReportingSpecification_sequence);

  return offset;
}


static const per_sequence_t RasUsageInfoTypes_sequence[] = {
  { &hf_h225_nonStandardUsageTypes, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_SEQUENCE_OF_NonStandardParameter },
  { &hf_h225_startTime      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NULL },
  { &hf_h225_endTime_flg    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NULL },
  { &hf_h225_terminationCause_flg, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NULL },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_RasUsageInfoTypes(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_RasUsageInfoTypes, RasUsageInfoTypes_sequence);

  return offset;
}


static const per_sequence_t RasUsageSpecification_when_sequence[] = {
  { &hf_h225_start          , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NULL },
  { &hf_h225_end            , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NULL },
  { &hf_h225_inIrr          , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NULL },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_RasUsageSpecification_when(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_RasUsageSpecification_when, RasUsageSpecification_when_sequence);

  return offset;
}


static const per_sequence_t RasUsageSpecificationcallStartingPoint_sequence[] = {
  { &hf_h225_alerting_flg   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NULL },
  { &hf_h225_connect_flg    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NULL },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_RasUsageSpecificationcallStartingPoint(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_RasUsageSpecificationcallStartingPoint, RasUsageSpecificationcallStartingPoint_sequence);

  return offset;
}


static const per_sequence_t RasUsageSpecification_sequence[] = {
  { &hf_h225_when           , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RasUsageSpecification_when },
  { &hf_h225_ras_callStartingPoint, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_RasUsageSpecificationcallStartingPoint },
  { &hf_h225_required       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RasUsageInfoTypes },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_RasUsageSpecification(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_RasUsageSpecification, RasUsageSpecification_sequence);

  return offset;
}


static const per_sequence_t RasUsageInformation_sequence[] = {
  { &hf_h225_nonStandardUsageFields, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_SEQUENCE_OF_NonStandardParameter },
  { &hf_h225_alertingTime   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h235_TimeStamp },
  { &hf_h225_connectTime    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h235_TimeStamp },
  { &hf_h225_endTime        , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h235_TimeStamp },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_RasUsageInformation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_RasUsageInformation, RasUsageInformation_sequence);

  return offset;
}



static int
dissect_h225_OCTET_STRING_SIZE_2_32(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       2, 32, FALSE, NULL);

  return offset;
}


static const value_string h225_CallTerminationCause_vals[] = {
  {   0, "releaseCompleteReason" },
  {   1, "releaseCompleteCauseIE" },
  { 0, NULL }
};

static const per_choice_t CallTerminationCause_choice[] = {
  {   0, &hf_h225_releaseCompleteReason, ASN1_EXTENSION_ROOT    , dissect_h225_ReleaseCompleteReason },
  {   1, &hf_h225_releaseCompleteCauseIE, ASN1_EXTENSION_ROOT    , dissect_h225_OCTET_STRING_SIZE_2_32 },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_CallTerminationCause(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_CallTerminationCause, CallTerminationCause_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t TransportChannelInfo_sequence[] = {
  { &hf_h225_sendAddress    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_TransportAddress },
  { &hf_h225_recvAddress    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_TransportAddress },
  { NULL, 0, 0, NULL }
};

int
dissect_h225_TransportChannelInfo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_TransportChannelInfo, TransportChannelInfo_sequence);

  return offset;
}


static const per_sequence_t BandwidthDetails_sequence[] = {
  { &hf_h225_sender         , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_multicast      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_bandwidth      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BandWidth },
  { &hf_h225_rtcpAddresses  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_TransportChannelInfo },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_BandwidthDetails(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_BandwidthDetails, BandwidthDetails_sequence);

  return offset;
}


static const per_sequence_t CallCreditCapability_sequence[] = {
  { &hf_h225_canDisplayAmountString, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_BOOLEAN },
  { &hf_h225_canEnforceDurationLimit, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_BOOLEAN },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_CallCreditCapability(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_CallCreditCapability, CallCreditCapability_sequence);

  return offset;
}



static int
dissect_h225_PrintableString(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_PrintableString(tvb, offset, actx, tree, hf_index,
                                          NO_BOUND, NO_BOUND, FALSE);

  return offset;
}



static int
dissect_h225_INTEGER_1_255(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 255U, NULL, FALSE);

  return offset;
}


static const per_sequence_t T_associatedSessionIds_sequence_of[1] = {
  { &hf_h225_associatedSessionIds_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_INTEGER_1_255 },
};

static int
dissect_h225_T_associatedSessionIds(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_T_associatedSessionIds, T_associatedSessionIds_sequence_of);

  return offset;
}


static const per_sequence_t RTPSession_sequence[] = {
  { &hf_h225_rtpAddress     , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_TransportChannelInfo },
  { &hf_h225_rtcpAddress    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_TransportChannelInfo },
  { &hf_h225_cname          , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_PrintableString },
  { &hf_h225_ssrc           , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_INTEGER_1_4294967295 },
  { &hf_h225_sessionId      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_INTEGER_1_255 },
  { &hf_h225_associatedSessionIds, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_T_associatedSessionIds },
  { &hf_h225_multicast_flg  , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_NULL },
  { &hf_h225_bandwidth      , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_BandWidth },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_RTPSession(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_RTPSession, RTPSession_sequence);

  return offset;
}


static const value_string h225_RehomingModel_vals[] = {
  {   0, "gatekeeperBased" },
  {   1, "endpointBased" },
  { 0, NULL }
};

static const per_choice_t RehomingModel_choice[] = {
  {   0, &hf_h225_gatekeeperBased, ASN1_NO_EXTENSIONS     , dissect_h225_NULL },
  {   1, &hf_h225_endpointBased  , ASN1_NO_EXTENSIONS     , dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_RehomingModel(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_RehomingModel, RehomingModel_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_Endpoint_sequence_of[1] = {
  { &hf_h225_alternateEndpoints_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_Endpoint },
};

static int
dissect_h225_SEQUENCE_OF_Endpoint(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_Endpoint, SEQUENCE_OF_Endpoint_sequence_of);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_AuthenticationMechanism_sequence_of[1] = {
  { &hf_h225_authenticationCapability_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h235_AuthenticationMechanism },
};

static int
dissect_h225_SEQUENCE_OF_AuthenticationMechanism(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_AuthenticationMechanism, SEQUENCE_OF_AuthenticationMechanism_sequence_of);

  return offset;
}


static const per_sequence_t T_algorithmOIDs_sequence_of[1] = {
  { &hf_h225_algorithmOIDs_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_OBJECT_IDENTIFIER },
};

static int
dissect_h225_T_algorithmOIDs(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_T_algorithmOIDs, T_algorithmOIDs_sequence_of);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_IntegrityMechanism_sequence_of[1] = {
  { &hf_h225_integrity_item , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_IntegrityMechanism },
};

static int
dissect_h225_SEQUENCE_OF_IntegrityMechanism(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_IntegrityMechanism, SEQUENCE_OF_IntegrityMechanism_sequence_of);

  return offset;
}


static const per_sequence_t GatekeeperRequest_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_protocolIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ProtocolIdentifier },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_gatekeeperRequest_rasAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_TransportAddress },
  { &hf_h225_endpointType   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_EndpointType },
  { &hf_h225_gatekeeperIdentifier, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_GatekeeperIdentifier },
  { &hf_h225_callServices   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_QseriesOptions },
  { &hf_h225_endpointAlias  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_alternateEndpoints, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_Endpoint },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_authenticationCapability, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AuthenticationMechanism },
  { &hf_h225_algorithmOIDs  , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_T_algorithmOIDs },
  { &hf_h225_integrity      , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_IntegrityMechanism },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_supportsAltGK  , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_NULL },
  { &hf_h225_featureSet     , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FeatureSet },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { &hf_h225_supportsAssignedGK, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_assignedGatekeeper, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AlternateGK },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_GatekeeperRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_GatekeeperRequest, GatekeeperRequest_sequence);

  return offset;
}


static const per_sequence_t GatekeeperConfirm_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_protocolIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ProtocolIdentifier },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_gatekeeperIdentifier, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_GatekeeperIdentifier },
  { &hf_h225_gatekeeperConfirm_rasAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_TransportAddress },
  { &hf_h225_alternateGatekeeper, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AlternateGK },
  { &hf_h225_authenticationMode, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h235_AuthenticationMechanism },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_algorithmOID   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_OBJECT_IDENTIFIER },
  { &hf_h225_integrity      , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_IntegrityMechanism },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_featureSet     , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FeatureSet },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { &hf_h225_assignedGatekeeper, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AlternateGK },
  { &hf_h225_rehomingModel  , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_RehomingModel },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_GatekeeperConfirm(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_GatekeeperConfirm, GatekeeperConfirm_sequence);

  return offset;
}


const value_string GatekeeperRejectReason_vals[] = {
  {   0, "resourceUnavailable" },
  {   1, "terminalExcluded" },
  {   2, "invalidRevision" },
  {   3, "undefinedReason" },
  {   4, "securityDenial" },
  {   5, "genericDataReason" },
  {   6, "neededFeatureNotSupported" },
  {   7, "securityError" },
  { 0, NULL }
};

static const per_choice_t GatekeeperRejectReason_choice[] = {
  {   0, &hf_h225_resourceUnavailable, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_terminalExcluded, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_invalidRevision, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   3, &hf_h225_undefinedReason, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   4, &hf_h225_securityDenial , ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {   5, &hf_h225_genericDataReason, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {   6, &hf_h225_neededFeatureNotSupported, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {   7, &hf_h225_gkRej_securityError, ASN1_NOT_EXTENSION_ROOT, dissect_h225_SecurityErrors },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_GatekeeperRejectReason(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 604 "./asn1/h225/h225.cnf"
  gint32 value;
  h225_packet_info* h225_pi;
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);

  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_GatekeeperRejectReason, GatekeeperRejectReason_choice,
                                 &value);

  if (h225_pi != NULL) {
    h225_pi->reason = value;
  }


  return offset;
}


static const per_sequence_t GatekeeperReject_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_protocolIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ProtocolIdentifier },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_gatekeeperIdentifier, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_GatekeeperIdentifier },
  { &hf_h225_gatekeeperRejectReason, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_GatekeeperRejectReason },
  { &hf_h225_altGKInfo      , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AltGKInfo },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_featureSet     , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FeatureSet },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_GatekeeperReject(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_GatekeeperReject, GatekeeperReject_sequence);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_AddressPattern_sequence_of[1] = {
  { &hf_h225_terminalAliasPattern_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_AddressPattern },
};

static int
dissect_h225_SEQUENCE_OF_AddressPattern(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_AddressPattern, SEQUENCE_OF_AddressPattern_sequence_of);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_H248PackagesDescriptor_sequence_of[1] = {
  { &hf_h225_supportedH248Packages_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_H248PackagesDescriptor },
};

static int
dissect_h225_SEQUENCE_OF_H248PackagesDescriptor(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_H248PackagesDescriptor, SEQUENCE_OF_H248PackagesDescriptor_sequence_of);

  return offset;
}


static const per_sequence_t SEQUENCE_SIZE_1_256_OF_QOSCapability_sequence_of[1] = {
  { &hf_h225_qOSCapabilities_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h245_QOSCapability },
};

static int
dissect_h225_SEQUENCE_SIZE_1_256_OF_QOSCapability(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_h225_SEQUENCE_SIZE_1_256_OF_QOSCapability, SEQUENCE_SIZE_1_256_OF_QOSCapability_sequence_of,
                                                  1, 256, FALSE);

  return offset;
}


const value_string h225_TransportQOS_vals[] = {
  {   0, "endpointControlled" },
  {   1, "gatekeeperControlled" },
  {   2, "noControl" },
  {   3, "qOSCapabilities" },
  { 0, NULL }
};

static const per_choice_t TransportQOS_choice[] = {
  {   0, &hf_h225_endpointControlled, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_gatekeeperControlled, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_noControl      , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   3, &hf_h225_qOSCapabilities, ASN1_NOT_EXTENSION_ROOT, dissect_h225_SEQUENCE_SIZE_1_256_OF_QOSCapability },
  { 0, NULL, 0, NULL }
};

int
dissect_h225_TransportQOS(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_TransportQOS, TransportQOS_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t RegistrationRequest_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_protocolIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ProtocolIdentifier },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_discoveryComplete, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_callSignalAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_SEQUENCE_OF_TransportAddress },
  { &hf_h225_rasAddress     , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_SEQUENCE_OF_TransportAddress },
  { &hf_h225_terminalType   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_EndpointType },
  { &hf_h225_terminalAlias  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_gatekeeperIdentifier, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_GatekeeperIdentifier },
  { &hf_h225_endpointVendor , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_VendorIdentifier },
  { &hf_h225_alternateEndpoints, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_Endpoint },
  { &hf_h225_timeToLive     , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_TimeToLive },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_keepAlive      , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_endpointIdentifier, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_EndpointIdentifier },
  { &hf_h225_willSupplyUUIEs, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_maintainConnection, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_alternateTransportAddresses, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AlternateTransportAddresses },
  { &hf_h225_additiveRegistration, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_NULL },
  { &hf_h225_terminalAliasPattern, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AddressPattern },
  { &hf_h225_supportsAltGK  , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_NULL },
  { &hf_h225_usageReportingCapability, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_RasUsageInfoTypes },
  { &hf_h225_multipleCalls  , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_BOOLEAN },
  { &hf_h225_supportedH248Packages, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_H248PackagesDescriptor },
  { &hf_h225_callCreditCapability, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CallCreditCapability },
  { &hf_h225_capacityReportingCapability, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CapacityReportingCapability },
  { &hf_h225_capacity       , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CallCapacity },
  { &hf_h225_featureSet     , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FeatureSet },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { &hf_h225_restart        , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_NULL },
  { &hf_h225_supportsACFSequences, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_NULL },
  { &hf_h225_supportsAssignedGK, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_assignedGatekeeper, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AlternateGK },
  { &hf_h225_transportQOS   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_TransportQOS },
  { &hf_h225_language       , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_Language },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_RegistrationRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_RegistrationRequest, RegistrationRequest_sequence);

  return offset;
}



static int
dissect_h225_INTEGER_1_65535(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 65535U, NULL, FALSE);

  return offset;
}


static const per_sequence_t T_preGrantedARQ_sequence[] = {
  { &hf_h225_makeCall       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_useGKCallSignalAddressToMakeCall, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_answerCall     , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_useGKCallSignalAddressToAnswer, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_irrFrequencyInCall, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_INTEGER_1_65535 },
  { &hf_h225_totalBandwidthRestriction, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_BandWidth },
  { &hf_h225_alternateTransportAddresses, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AlternateTransportAddresses },
  { &hf_h225_useSpecifiedTransport, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_UseSpecifiedTransport },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_T_preGrantedARQ(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_T_preGrantedARQ, T_preGrantedARQ_sequence);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_RasUsageSpecification_sequence_of[1] = {
  { &hf_h225_usageSpec_item , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_RasUsageSpecification },
};

static int
dissect_h225_SEQUENCE_OF_RasUsageSpecification(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_RasUsageSpecification, SEQUENCE_OF_RasUsageSpecification_sequence_of);

  return offset;
}


static const per_sequence_t RegistrationConfirm_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_protocolIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ProtocolIdentifier },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_callSignalAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_SEQUENCE_OF_TransportAddress },
  { &hf_h225_terminalAlias  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_gatekeeperIdentifier, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_GatekeeperIdentifier },
  { &hf_h225_endpointIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_EndpointIdentifier },
  { &hf_h225_alternateGatekeeper, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AlternateGK },
  { &hf_h225_timeToLive     , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_TimeToLive },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_willRespondToIRR, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_preGrantedARQ  , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_T_preGrantedARQ },
  { &hf_h225_maintainConnection, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_serviceControl , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ServiceControlSession },
  { &hf_h225_supportsAdditiveRegistration, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_NULL },
  { &hf_h225_terminalAliasPattern, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AddressPattern },
  { &hf_h225_supportedPrefixes, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_SupportedPrefix },
  { &hf_h225_usageSpec      , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_RasUsageSpecification },
  { &hf_h225_featureServerAlias, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AliasAddress },
  { &hf_h225_capacityReportingSpec, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CapacityReportingSpecification },
  { &hf_h225_featureSet     , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FeatureSet },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { &hf_h225_assignedGatekeeper, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AlternateGK },
  { &hf_h225_rehomingModel  , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_RehomingModel },
  { &hf_h225_transportQOS   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_TransportQOS },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_RegistrationConfirm(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_RegistrationConfirm, RegistrationConfirm_sequence);

  return offset;
}


static const per_sequence_t T_invalidTerminalAliases_sequence[] = {
  { &hf_h225_terminalAlias  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_terminalAliasPattern, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AddressPattern },
  { &hf_h225_supportedPrefixes, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_SupportedPrefix },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_T_invalidTerminalAliases(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_T_invalidTerminalAliases, T_invalidTerminalAliases_sequence);

  return offset;
}


const value_string RegistrationRejectReason_vals[] = {
  {   0, "discoveryRequired" },
  {   1, "invalidRevision" },
  {   2, "invalidCallSignalAddress" },
  {   3, "invalidRASAddress" },
  {   4, "duplicateAlias" },
  {   5, "invalidTerminalType" },
  {   6, "undefinedReason" },
  {   7, "transportNotSupported" },
  {   8, "transportQOSNotSupported" },
  {   9, "resourceUnavailable" },
  {  10, "invalidAlias" },
  {  11, "securityDenial" },
  {  12, "fullRegistrationRequired" },
  {  13, "additiveRegistrationNotSupported" },
  {  14, "invalidTerminalAliases" },
  {  15, "genericDataReason" },
  {  16, "neededFeatureNotSupported" },
  {  17, "securityError" },
  {  18, "registerWithAssignedGK" },
  { 0, NULL }
};

static const per_choice_t RegistrationRejectReason_choice[] = {
  {   0, &hf_h225_discoveryRequired, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_invalidRevision, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_invalidCallSignalAddress, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   3, &hf_h225_invalidRASAddress, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   4, &hf_h225_duplicateAlias , ASN1_EXTENSION_ROOT    , dissect_h225_SEQUENCE_OF_AliasAddress },
  {   5, &hf_h225_invalidTerminalType, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   6, &hf_h225_undefinedReason, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   7, &hf_h225_transportNotSupported, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   8, &hf_h225_transportQOSNotSupported, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {   9, &hf_h225_resourceUnavailable, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  10, &hf_h225_invalidAlias   , ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  11, &hf_h225_securityDenial , ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  12, &hf_h225_fullRegistrationRequired, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  13, &hf_h225_additiveRegistrationNotSupported, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  14, &hf_h225_invalidTerminalAliases, ASN1_NOT_EXTENSION_ROOT, dissect_h225_T_invalidTerminalAliases },
  {  15, &hf_h225_genericDataReason, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  16, &hf_h225_neededFeatureNotSupported, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  17, &hf_h225_reg_securityError, ASN1_NOT_EXTENSION_ROOT, dissect_h225_SecurityErrors },
  {  18, &hf_h225_registerWithAssignedGK, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_RegistrationRejectReason(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 691 "./asn1/h225/h225.cnf"
  gint32 value;
  h225_packet_info* h225_pi;
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);

  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_RegistrationRejectReason, RegistrationRejectReason_choice,
                                 &value);

  if (h225_pi != NULL) {
    h225_pi->reason = value;
  }


  return offset;
}


static const per_sequence_t RegistrationReject_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_protocolIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ProtocolIdentifier },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_registrationRejectReason, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RegistrationRejectReason },
  { &hf_h225_gatekeeperIdentifier, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_GatekeeperIdentifier },
  { &hf_h225_altGKInfo      , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AltGKInfo },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_featureSet     , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FeatureSet },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { &hf_h225_assignedGatekeeper, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AlternateGK },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_RegistrationReject(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_RegistrationReject, RegistrationReject_sequence);

  return offset;
}


const value_string UnregRequestReason_vals[] = {
  {   0, "reregistrationRequired" },
  {   1, "ttlExpired" },
  {   2, "securityDenial" },
  {   3, "undefinedReason" },
  {   4, "maintenance" },
  {   5, "securityError" },
  {   6, "registerWithAssignedGK" },
  { 0, NULL }
};

static const per_choice_t UnregRequestReason_choice[] = {
  {   0, &hf_h225_reregistrationRequired, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_ttlExpired     , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_securityDenial , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   3, &hf_h225_undefinedReason, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   4, &hf_h225_maintenance    , ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {   5, &hf_h225_securityError  , ASN1_NOT_EXTENSION_ROOT, dissect_h225_SecurityErrors2 },
  {   6, &hf_h225_registerWithAssignedGK, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_UnregRequestReason(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 615 "./asn1/h225/h225.cnf"
  gint32 value;
  h225_packet_info* h225_pi;
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);

  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_UnregRequestReason, UnregRequestReason_choice,
                                 &value);

  if (h225_pi != NULL) {
    h225_pi->reason = value;
  }


  return offset;
}


static const per_sequence_t UnregistrationRequest_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_callSignalAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_SEQUENCE_OF_TransportAddress },
  { &hf_h225_endpointAlias  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_endpointIdentifier, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_EndpointIdentifier },
  { &hf_h225_alternateEndpoints, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_Endpoint },
  { &hf_h225_gatekeeperIdentifier, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_GatekeeperIdentifier },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_unregRequestReason, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_UnregRequestReason },
  { &hf_h225_endpointAliasPattern, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AddressPattern },
  { &hf_h225_supportedPrefixes, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_SupportedPrefix },
  { &hf_h225_alternateGatekeeper, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AlternateGK },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { &hf_h225_assignedGatekeeper, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AlternateGK },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_UnregistrationRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_UnregistrationRequest, UnregistrationRequest_sequence);

  return offset;
}


static const per_sequence_t UnregistrationConfirm_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { &hf_h225_assignedGatekeeper, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AlternateGK },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_UnregistrationConfirm(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_UnregistrationConfirm, UnregistrationConfirm_sequence);

  return offset;
}


const value_string UnregRejectReason_vals[] = {
  {   0, "notCurrentlyRegistered" },
  {   1, "callInProgress" },
  {   2, "undefinedReason" },
  {   3, "permissionDenied" },
  {   4, "securityDenial" },
  {   5, "securityError" },
  { 0, NULL }
};

static const per_choice_t UnregRejectReason_choice[] = {
  {   0, &hf_h225_notCurrentlyRegistered, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_callInProgress , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_undefinedReason, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   3, &hf_h225_permissionDenied, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {   4, &hf_h225_securityDenial , ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {   5, &hf_h225_securityError  , ASN1_NOT_EXTENSION_ROOT, dissect_h225_SecurityErrors2 },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_UnregRejectReason(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 626 "./asn1/h225/h225.cnf"
  gint32 value;
  h225_packet_info* h225_pi;
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);

  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_UnregRejectReason, UnregRejectReason_choice,
                                 &value);

  if (h225_pi != NULL) {
    h225_pi->reason = value;
  }


  return offset;
}


static const per_sequence_t UnregistrationReject_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_unregRejectReason, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_UnregRejectReason },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_altGKInfo      , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AltGKInfo },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_UnregistrationReject(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_UnregistrationReject, UnregistrationReject_sequence);

  return offset;
}


static const value_string h225_CallModel_vals[] = {
  {   0, "direct" },
  {   1, "gatekeeperRouted" },
  { 0, NULL }
};

static const per_choice_t CallModel_choice[] = {
  {   0, &hf_h225_direct         , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_gatekeeperRouted, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_CallModel(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_CallModel, CallModel_choice,
                                 NULL);

  return offset;
}



static int
dissect_h225_DestinationInfo_item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 312 "./asn1/h225/h225.cnf"
  h225_packet_info* h225_pi;

  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);
  if (h225_pi != NULL) {
    h225_pi->is_destinationInfo = TRUE;
  }

  offset = dissect_h225_AliasAddress(tvb, offset, actx, tree, hf_index);

  return offset;
}


static const per_sequence_t DestinationInfo_sequence_of[1] = {
  { &hf_h225_DestinationInfo_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_DestinationInfo_item },
};

static int
dissect_h225_DestinationInfo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_DestinationInfo, DestinationInfo_sequence_of);

  return offset;
}


static const per_sequence_t AdmissionRequest_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_callType       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_CallType },
  { &hf_h225_callModel      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_CallModel },
  { &hf_h225_endpointIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_EndpointIdentifier },
  { &hf_h225_destinationInfo_01, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_DestinationInfo },
  { &hf_h225_destCallSignalAddress, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_TransportAddress },
  { &hf_h225_destExtraCallInfo, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_srcInfo        , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_srcCallSignalAddress, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_TransportAddress },
  { &hf_h225_bandWidth      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BandWidth },
  { &hf_h225_callReferenceValue, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_CallReferenceValue },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_callServices   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_QseriesOptions },
  { &hf_h225_conferenceID   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ConferenceIdentifier },
  { &hf_h225_activeMC       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_answerCall     , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_canMapAlias    , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_callIdentifier , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_CallIdentifier },
  { &hf_h225_srcAlternatives, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_Endpoint },
  { &hf_h225_destAlternatives, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_Endpoint },
  { &hf_h225_gatekeeperIdentifier, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_GatekeeperIdentifier },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_transportQOS   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_TransportQOS },
  { &hf_h225_willSupplyUUIEs, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_callLinkage    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CallLinkage },
  { &hf_h225_gatewayDataRate, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_DataRate },
  { &hf_h225_capacity       , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CallCapacity },
  { &hf_h225_circuitInfo    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CircuitInfo },
  { &hf_h225_desiredProtocols, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_SupportedProtocols },
  { &hf_h225_desiredTunnelledProtocol, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_TunnelledProtocol },
  { &hf_h225_featureSet     , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FeatureSet },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { &hf_h225_canMapSrcAlias , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_AdmissionRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_AdmissionRequest, AdmissionRequest_sequence);

  return offset;
}


static const per_sequence_t UUIEsRequested_sequence[] = {
  { &hf_h225_setup_bool     , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_callProceeding_flg, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_connect_bool   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_alerting_bool  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_information_bool, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_releaseComplete_bool, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_facility_bool  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_progress_bool  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_empty          , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_status_bool    , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_statusInquiry_bool, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_setupAcknowledge_bool, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_notify_bool    , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_UUIEsRequested(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_UUIEsRequested, UUIEsRequested_sequence);

  return offset;
}


static const per_sequence_t AdmissionConfirm_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_bandWidth      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BandWidth },
  { &hf_h225_callModel      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_CallModel },
  { &hf_h225_destCallSignalAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_TransportAddress },
  { &hf_h225_irrFrequency   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_INTEGER_1_65535 },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_destinationInfo_01, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_DestinationInfo },
  { &hf_h225_destExtraCallInfo, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_destinationType, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_EndpointType },
  { &hf_h225_remoteExtensionAddress, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_alternateEndpoints, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_Endpoint },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_transportQOS   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_TransportQOS },
  { &hf_h225_willRespondToIRR, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_uuiesRequested , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_UUIEsRequested },
  { &hf_h225_language       , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_Language },
  { &hf_h225_alternateTransportAddresses, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AlternateTransportAddresses },
  { &hf_h225_useSpecifiedTransport, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_UseSpecifiedTransport },
  { &hf_h225_circuitInfo    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CircuitInfo },
  { &hf_h225_usageSpec      , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_RasUsageSpecification },
  { &hf_h225_supportedProtocols, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_SupportedProtocols },
  { &hf_h225_serviceControl , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ServiceControlSession },
  { &hf_h225_multipleCalls  , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_BOOLEAN },
  { &hf_h225_featureSet     , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FeatureSet },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { &hf_h225_modifiedSrcInfo, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_assignedGatekeeper, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AlternateGK },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_AdmissionConfirm(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_AdmissionConfirm, AdmissionConfirm_sequence);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_PartyNumber_sequence_of[1] = {
  { &hf_h225_routeCallToSCN_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_PartyNumber },
};

static int
dissect_h225_SEQUENCE_OF_PartyNumber(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_PartyNumber, SEQUENCE_OF_PartyNumber_sequence_of);

  return offset;
}


const value_string AdmissionRejectReason_vals[] = {
  {   0, "calledPartyNotRegistered" },
  {   1, "invalidPermission" },
  {   2, "requestDenied" },
  {   3, "undefinedReason" },
  {   4, "callerNotRegistered" },
  {   5, "routeCallToGatekeeper" },
  {   6, "invalidEndpointIdentifier" },
  {   7, "resourceUnavailable" },
  {   8, "securityDenial" },
  {   9, "qosControlNotSupported" },
  {  10, "incompleteAddress" },
  {  11, "aliasesInconsistent" },
  {  12, "routeCallToSCN" },
  {  13, "exceedsCallCapacity" },
  {  14, "collectDestination" },
  {  15, "collectPIN" },
  {  16, "genericDataReason" },
  {  17, "neededFeatureNotSupported" },
  {  18, "securityError" },
  {  19, "securityDHmismatch" },
  {  20, "noRouteToDestination" },
  {  21, "unallocatedNumber" },
  {  22, "registerWithAssignedGK" },
  { 0, NULL }
};

static const per_choice_t AdmissionRejectReason_choice[] = {
  {   0, &hf_h225_calledPartyNotRegistered, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_invalidPermission, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_requestDenied  , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   3, &hf_h225_undefinedReason, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   4, &hf_h225_callerNotRegistered, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   5, &hf_h225_routeCallToGatekeeper, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   6, &hf_h225_invalidEndpointIdentifier, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   7, &hf_h225_resourceUnavailable, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   8, &hf_h225_securityDenial , ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {   9, &hf_h225_qosControlNotSupported, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  10, &hf_h225_incompleteAddress, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  11, &hf_h225_aliasesInconsistent, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  12, &hf_h225_routeCallToSCN , ASN1_NOT_EXTENSION_ROOT, dissect_h225_SEQUENCE_OF_PartyNumber },
  {  13, &hf_h225_exceedsCallCapacity, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  14, &hf_h225_collectDestination, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  15, &hf_h225_collectPIN     , ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  16, &hf_h225_genericDataReason, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  17, &hf_h225_neededFeatureNotSupported, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  18, &hf_h225_securityError  , ASN1_NOT_EXTENSION_ROOT, dissect_h225_SecurityErrors2 },
  {  19, &hf_h225_securityDHmismatch, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  20, &hf_h225_noRouteToDestination, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  21, &hf_h225_unallocatedNumber, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  22, &hf_h225_registerWithAssignedGK, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_AdmissionRejectReason(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 669 "./asn1/h225/h225.cnf"
  gint32 value;
  h225_packet_info* h225_pi;
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);

  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_AdmissionRejectReason, AdmissionRejectReason_choice,
                                 &value);

  if (h225_pi != NULL) {
    h225_pi->reason = value;
  }


  return offset;
}


static const per_sequence_t AdmissionReject_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_rejectReason   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_AdmissionRejectReason },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_altGKInfo      , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AltGKInfo },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_callSignalAddress, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_TransportAddress },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_serviceControl , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ServiceControlSession },
  { &hf_h225_featureSet     , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FeatureSet },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { &hf_h225_assignedGatekeeper, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AlternateGK },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_AdmissionReject(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_AdmissionReject, AdmissionReject_sequence);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_BandwidthDetails_sequence_of[1] = {
  { &hf_h225_bandwidthDetails_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_BandwidthDetails },
};

static int
dissect_h225_SEQUENCE_OF_BandwidthDetails(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_BandwidthDetails, SEQUENCE_OF_BandwidthDetails_sequence_of);

  return offset;
}


static const per_sequence_t BandwidthRequest_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_endpointIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_EndpointIdentifier },
  { &hf_h225_conferenceID   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ConferenceIdentifier },
  { &hf_h225_callReferenceValue, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_CallReferenceValue },
  { &hf_h225_callType       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_CallType },
  { &hf_h225_bandWidth      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BandWidth },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_callIdentifier , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_CallIdentifier },
  { &hf_h225_gatekeeperIdentifier, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_GatekeeperIdentifier },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_answeredCall   , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_callLinkage    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CallLinkage },
  { &hf_h225_capacity       , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CallCapacity },
  { &hf_h225_usageInformation, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_RasUsageInformation },
  { &hf_h225_bandwidthDetails, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_BandwidthDetails },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { &hf_h225_transportQOS   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_TransportQOS },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_BandwidthRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_BandwidthRequest, BandwidthRequest_sequence);

  return offset;
}


static const per_sequence_t BandwidthConfirm_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_bandWidth      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BandWidth },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_capacity       , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CallCapacity },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { &hf_h225_transportQOS   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_TransportQOS },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_BandwidthConfirm(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_BandwidthConfirm, BandwidthConfirm_sequence);

  return offset;
}


const value_string BandRejectReason_vals[] = {
  {   0, "notBound" },
  {   1, "invalidConferenceID" },
  {   2, "invalidPermission" },
  {   3, "insufficientResources" },
  {   4, "invalidRevision" },
  {   5, "undefinedReason" },
  {   6, "securityDenial" },
  {   7, "securityError" },
  { 0, NULL }
};

static const per_choice_t BandRejectReason_choice[] = {
  {   0, &hf_h225_notBound       , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_invalidConferenceID, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_invalidPermission, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   3, &hf_h225_insufficientResources, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   4, &hf_h225_invalidRevision, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   5, &hf_h225_undefinedReason, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   6, &hf_h225_securityDenial , ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {   7, &hf_h225_securityError  , ASN1_NOT_EXTENSION_ROOT, dissect_h225_SecurityErrors2 },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_BandRejectReason(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 637 "./asn1/h225/h225.cnf"
  gint32 value;
  h225_packet_info* h225_pi;
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);

  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_BandRejectReason, BandRejectReason_choice,
                                 &value);

  if (h225_pi != NULL) {
    h225_pi->reason = value;
  }


  return offset;
}


static const per_sequence_t BandwidthReject_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_bandRejectReason, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BandRejectReason },
  { &hf_h225_allowedBandWidth, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BandWidth },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_altGKInfo      , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AltGKInfo },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_BandwidthReject(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_BandwidthReject, BandwidthReject_sequence);

  return offset;
}


const value_string DisengageReason_vals[] = {
  {   0, "forcedDrop" },
  {   1, "normalDrop" },
  {   2, "undefinedReason" },
  { 0, NULL }
};

static const per_choice_t DisengageReason_choice[] = {
  {   0, &hf_h225_forcedDrop     , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_normalDrop     , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_undefinedReason, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_DisengageReason(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 648 "./asn1/h225/h225.cnf"
  gint32 value;
  h225_packet_info* h225_pi;
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);

  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_DisengageReason, DisengageReason_choice,
                                 &value);

  if (h225_pi != NULL) {
    h225_pi->reason = value;
  }


  return offset;
}


static const per_sequence_t DisengageRequest_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_endpointIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_EndpointIdentifier },
  { &hf_h225_conferenceID   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ConferenceIdentifier },
  { &hf_h225_callReferenceValue, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_CallReferenceValue },
  { &hf_h225_disengageReason, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_DisengageReason },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_callIdentifier , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_CallIdentifier },
  { &hf_h225_gatekeeperIdentifier, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_GatekeeperIdentifier },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_answeredCall   , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_callLinkage    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CallLinkage },
  { &hf_h225_capacity       , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CallCapacity },
  { &hf_h225_circuitInfo    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CircuitInfo },
  { &hf_h225_usageInformation, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_RasUsageInformation },
  { &hf_h225_terminationCause, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CallTerminationCause },
  { &hf_h225_serviceControl , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ServiceControlSession },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_DisengageRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_DisengageRequest, DisengageRequest_sequence);

  return offset;
}


static const per_sequence_t DisengageConfirm_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_capacity       , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CallCapacity },
  { &hf_h225_circuitInfo    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CircuitInfo },
  { &hf_h225_usageInformation, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_RasUsageInformation },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { &hf_h225_assignedGatekeeper, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AlternateGK },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_DisengageConfirm(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_DisengageConfirm, DisengageConfirm_sequence);

  return offset;
}


const value_string DisengageRejectReason_vals[] = {
  {   0, "notRegistered" },
  {   1, "requestToDropOther" },
  {   2, "securityDenial" },
  {   3, "securityError" },
  { 0, NULL }
};

static const per_choice_t DisengageRejectReason_choice[] = {
  {   0, &hf_h225_notRegistered  , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_requestToDropOther, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_securityDenial , ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {   3, &hf_h225_securityError  , ASN1_NOT_EXTENSION_ROOT, dissect_h225_SecurityErrors2 },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_DisengageRejectReason(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 658 "./asn1/h225/h225.cnf"
  gint32 value;
  h225_packet_info* h225_pi;
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);

  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_DisengageRejectReason, DisengageRejectReason_choice,
                                 &value);

  if (h225_pi != NULL) {
    h225_pi->reason = value;
  }


  return offset;
}


static const per_sequence_t DisengageReject_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_disengageRejectReason, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_DisengageRejectReason },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_altGKInfo      , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AltGKInfo },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_DisengageReject(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_DisengageReject, DisengageReject_sequence);

  return offset;
}


static const per_sequence_t LocationRequest_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_endpointIdentifier, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_EndpointIdentifier },
  { &hf_h225_destinationInfo_01, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_DestinationInfo },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_replyAddress   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_TransportAddress },
  { &hf_h225_sourceInfo     , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_canMapAlias    , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_gatekeeperIdentifier, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_GatekeeperIdentifier },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_desiredProtocols, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_SupportedProtocols },
  { &hf_h225_desiredTunnelledProtocol, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_TunnelledProtocol },
  { &hf_h225_featureSet     , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FeatureSet },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { &hf_h225_hopCount       , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_INTEGER_1_255 },
  { &hf_h225_circuitInfo    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CircuitInfo },
  { &hf_h225_callIdentifier , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CallIdentifier },
  { &hf_h225_bandWidth      , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_BandWidth },
  { &hf_h225_sourceEndpointInfo, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_canMapSrcAlias , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_language       , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_Language },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_LocationRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_LocationRequest, LocationRequest_sequence);

  return offset;
}


static const per_sequence_t LocationConfirm_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_locationConfirm_callSignalAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_TransportAddress },
  { &hf_h225_locationConfirm_rasAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_TransportAddress },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_destinationInfo_01, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_DestinationInfo },
  { &hf_h225_destExtraCallInfo, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_destinationType, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_EndpointType },
  { &hf_h225_remoteExtensionAddress, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_alternateEndpoints, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_Endpoint },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_alternateTransportAddresses, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AlternateTransportAddresses },
  { &hf_h225_supportedProtocols, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_SupportedProtocols },
  { &hf_h225_multipleCalls  , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_BOOLEAN },
  { &hf_h225_featureSet     , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FeatureSet },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { &hf_h225_circuitInfo    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CircuitInfo },
  { &hf_h225_serviceControl , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ServiceControlSession },
  { &hf_h225_modifiedSrcInfo, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_bandWidth      , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_BandWidth },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_LocationConfirm(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_LocationConfirm, LocationConfirm_sequence);

  return offset;
}


const value_string LocationRejectReason_vals[] = {
  {   0, "notRegistered" },
  {   1, "invalidPermission" },
  {   2, "requestDenied" },
  {   3, "undefinedReason" },
  {   4, "securityDenial" },
  {   5, "aliasesInconsistent" },
  {   6, "routeCalltoSCN" },
  {   7, "resourceUnavailable" },
  {   8, "genericDataReason" },
  {   9, "neededFeatureNotSupported" },
  {  10, "hopCountExceeded" },
  {  11, "incompleteAddress" },
  {  12, "securityError" },
  {  13, "securityDHmismatch" },
  {  14, "noRouteToDestination" },
  {  15, "unallocatedNumber" },
  { 0, NULL }
};

static const per_choice_t LocationRejectReason_choice[] = {
  {   0, &hf_h225_notRegistered  , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_invalidPermission, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_requestDenied  , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   3, &hf_h225_undefinedReason, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   4, &hf_h225_securityDenial , ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {   5, &hf_h225_aliasesInconsistent, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {   6, &hf_h225_routeCalltoSCN , ASN1_NOT_EXTENSION_ROOT, dissect_h225_SEQUENCE_OF_PartyNumber },
  {   7, &hf_h225_resourceUnavailable, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {   8, &hf_h225_genericDataReason, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {   9, &hf_h225_neededFeatureNotSupported, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  10, &hf_h225_hopCountExceeded, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  11, &hf_h225_incompleteAddress, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  12, &hf_h225_securityError  , ASN1_NOT_EXTENSION_ROOT, dissect_h225_SecurityErrors2 },
  {  13, &hf_h225_securityDHmismatch, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  14, &hf_h225_noRouteToDestination, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  {  15, &hf_h225_unallocatedNumber, ASN1_NOT_EXTENSION_ROOT, dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_LocationRejectReason(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 680 "./asn1/h225/h225.cnf"
  gint32 value;
  h225_packet_info* h225_pi;
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);

  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_LocationRejectReason, LocationRejectReason_choice,
                                 &value);

  if (h225_pi != NULL) {
    h225_pi->reason = value;
  }


  return offset;
}


static const per_sequence_t LocationReject_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_locationRejectReason, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_LocationRejectReason },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_altGKInfo      , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AltGKInfo },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_featureSet     , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FeatureSet },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { &hf_h225_serviceControl , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ServiceControlSession },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_LocationReject(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_LocationReject, LocationReject_sequence);

  return offset;
}


static const per_sequence_t InfoRequest_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_callReferenceValue, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_CallReferenceValue },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_replyAddress   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_TransportAddress },
  { &hf_h225_callIdentifier , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_CallIdentifier },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_uuiesRequested , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_UUIEsRequested },
  { &hf_h225_callLinkage    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CallLinkage },
  { &hf_h225_usageInfoRequested, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_RasUsageInfoTypes },
  { &hf_h225_segmentedResponseSupported, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_NULL },
  { &hf_h225_nextSegmentRequested, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_INTEGER_0_65535 },
  { &hf_h225_capacityInfoRequested, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_NULL },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { &hf_h225_assignedGatekeeper, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_AlternateGK },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_InfoRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_InfoRequest, InfoRequest_sequence);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_RTPSession_sequence_of[1] = {
  { &hf_h225_audio_item     , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_RTPSession },
};

static int
dissect_h225_SEQUENCE_OF_RTPSession(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_RTPSession, SEQUENCE_OF_RTPSession_sequence_of);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_TransportChannelInfo_sequence_of[1] = {
  { &hf_h225_data_item      , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_TransportChannelInfo },
};

static int
dissect_h225_SEQUENCE_OF_TransportChannelInfo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_TransportChannelInfo, SEQUENCE_OF_TransportChannelInfo_sequence_of);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_ConferenceIdentifier_sequence_of[1] = {
  { &hf_h225_substituteConfIDs_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_ConferenceIdentifier },
};

static int
dissect_h225_SEQUENCE_OF_ConferenceIdentifier(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_ConferenceIdentifier, SEQUENCE_OF_ConferenceIdentifier_sequence_of);

  return offset;
}


static const per_sequence_t T_pdu_item_sequence[] = {
  { &hf_h225_h323pdu        , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_H323_UU_PDU },
  { &hf_h225_sent           , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_T_pdu_item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_T_pdu_item, T_pdu_item_sequence);

  return offset;
}


static const per_sequence_t T_pdu_sequence_of[1] = {
  { &hf_h225_pdu_item       , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_T_pdu_item },
};

static int
dissect_h225_T_pdu(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_T_pdu, T_pdu_sequence_of);

  return offset;
}


static const per_sequence_t T_perCallInfo_item_sequence[] = {
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_callReferenceValue, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_CallReferenceValue },
  { &hf_h225_conferenceID   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ConferenceIdentifier },
  { &hf_h225_originator     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_BOOLEAN },
  { &hf_h225_audio          , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_RTPSession },
  { &hf_h225_video          , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_RTPSession },
  { &hf_h225_data           , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_TransportChannelInfo },
  { &hf_h225_h245           , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_TransportChannelInfo },
  { &hf_h225_callSignalling , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_TransportChannelInfo },
  { &hf_h225_callType       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_CallType },
  { &hf_h225_bandWidth      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BandWidth },
  { &hf_h225_callModel      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_CallModel },
  { &hf_h225_callIdentifier , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_CallIdentifier },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_substituteConfIDs, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_SEQUENCE_OF_ConferenceIdentifier },
  { &hf_h225_pdu            , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_T_pdu },
  { &hf_h225_callLinkage    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CallLinkage },
  { &hf_h225_usageInformation, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_RasUsageInformation },
  { &hf_h225_circuitInfo    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CircuitInfo },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_T_perCallInfo_item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_T_perCallInfo_item, T_perCallInfo_item_sequence);

  return offset;
}


static const per_sequence_t T_perCallInfo_sequence_of[1] = {
  { &hf_h225_perCallInfo_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_T_perCallInfo_item },
};

static int
dissect_h225_T_perCallInfo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_T_perCallInfo, T_perCallInfo_sequence_of);

  return offset;
}


static const value_string h225_InfoRequestResponseStatus_vals[] = {
  {   0, "complete" },
  {   1, "incomplete" },
  {   2, "segment" },
  {   3, "invalidCall" },
  { 0, NULL }
};

static const per_choice_t InfoRequestResponseStatus_choice[] = {
  {   0, &hf_h225_complete       , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_incomplete     , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_segment        , ASN1_EXTENSION_ROOT    , dissect_h225_INTEGER_0_65535 },
  {   3, &hf_h225_invalidCall    , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_InfoRequestResponseStatus(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_InfoRequestResponseStatus, InfoRequestResponseStatus_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t InfoRequestResponse_sequence[] = {
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_endpointType   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_EndpointType },
  { &hf_h225_endpointIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_EndpointIdentifier },
  { &hf_h225_infoRequestResponse_rasAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_TransportAddress },
  { &hf_h225_callSignalAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_SEQUENCE_OF_TransportAddress },
  { &hf_h225_endpointAlias  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_AliasAddress },
  { &hf_h225_perCallInfo    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_T_perCallInfo },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_needResponse   , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_capacity       , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CallCapacity },
  { &hf_h225_irrStatus      , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_InfoRequestResponseStatus },
  { &hf_h225_unsolicited    , ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_InfoRequestResponse(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_InfoRequestResponse, InfoRequestResponse_sequence);

  return offset;
}


static const per_sequence_t NonStandardMessage_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_NonStandardParameter },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_featureSet     , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_FeatureSet },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_NonStandardMessage(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_NonStandardMessage, NonStandardMessage_sequence);

  return offset;
}


static const per_sequence_t UnknownMessageResponse_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_tokens         , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_messageNotUnderstood, ASN1_NOT_EXTENSION_ROOT, ASN1_NOT_OPTIONAL, dissect_h225_OCTET_STRING },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_UnknownMessageResponse(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_UnknownMessageResponse, UnknownMessageResponse_sequence);

  return offset;
}


static const per_sequence_t RequestInProgress_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_tokens         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_delay          , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_INTEGER_1_65535 },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_RequestInProgress(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_RequestInProgress, RequestInProgress_sequence);

  return offset;
}


static const per_sequence_t ResourcesAvailableIndicate_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_protocolIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ProtocolIdentifier },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_endpointIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_EndpointIdentifier },
  { &hf_h225_protocols      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_SEQUENCE_OF_SupportedProtocols },
  { &hf_h225_almostOutOfResources, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { &hf_h225_tokens         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_capacity       , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_CallCapacity },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_ResourcesAvailableIndicate(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_ResourcesAvailableIndicate, ResourcesAvailableIndicate_sequence);

  return offset;
}


static const per_sequence_t ResourcesAvailableConfirm_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_protocolIdentifier, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ProtocolIdentifier },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_tokens         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_genericData    , ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_ResourcesAvailableConfirm(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_ResourcesAvailableConfirm, ResourcesAvailableConfirm_sequence);

  return offset;
}


static const per_sequence_t InfoRequestAck_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_tokens         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_ICV },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_InfoRequestAck(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_InfoRequestAck, InfoRequestAck_sequence);

  return offset;
}


const value_string InfoRequestNakReason_vals[] = {
  {   0, "notRegistered" },
  {   1, "securityDenial" },
  {   2, "undefinedReason" },
  {   3, "securityError" },
  { 0, NULL }
};

static const per_choice_t InfoRequestNakReason_choice[] = {
  {   0, &hf_h225_notRegistered  , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_securityDenial , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_undefinedReason, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   3, &hf_h225_securityError  , ASN1_NOT_EXTENSION_ROOT, dissect_h225_SecurityErrors2 },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_InfoRequestNakReason(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 702 "./asn1/h225/h225.cnf"
  gint32 value;
  h225_packet_info* h225_pi;
  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);

  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_InfoRequestNakReason, InfoRequestNakReason_choice,
                                 &value);

  if (h225_pi != NULL) {
    h225_pi->reason = value;
  }


  return offset;
}


static const per_sequence_t InfoRequestNak_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_nakReason      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_InfoRequestNakReason },
  { &hf_h225_altGKInfo      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_AltGKInfo },
  { &hf_h225_tokens         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_ICV },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_InfoRequestNak(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_InfoRequestNak, InfoRequestNak_sequence);

  return offset;
}


static const per_sequence_t T_callSpecific_sequence[] = {
  { &hf_h225_callIdentifier , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_CallIdentifier },
  { &hf_h225_conferenceID   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_ConferenceIdentifier },
  { &hf_h225_answeredCall   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_BOOLEAN },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_T_callSpecific(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_T_callSpecific, T_callSpecific_sequence);

  return offset;
}


static const per_sequence_t ServiceControlIndication_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_serviceControl , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_SEQUENCE_OF_ServiceControlSession },
  { &hf_h225_endpointIdentifier, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_EndpointIdentifier },
  { &hf_h225_callSpecific   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_T_callSpecific },
  { &hf_h225_tokens         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_featureSet     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_FeatureSet },
  { &hf_h225_genericData    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_ServiceControlIndication(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_ServiceControlIndication, ServiceControlIndication_sequence);

  return offset;
}


static const value_string h225_T_result_vals[] = {
  {   0, "started" },
  {   1, "failed" },
  {   2, "stopped" },
  {   3, "notAvailable" },
  {   4, "neededFeatureNotSupported" },
  { 0, NULL }
};

static const per_choice_t T_result_choice[] = {
  {   0, &hf_h225_started        , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   1, &hf_h225_failed         , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   2, &hf_h225_stopped        , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   3, &hf_h225_notAvailable   , ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  {   4, &hf_h225_neededFeatureNotSupported, ASN1_EXTENSION_ROOT    , dissect_h225_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_h225_T_result(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_T_result, T_result_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t ServiceControlResponse_sequence[] = {
  { &hf_h225_requestSeqNum  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_h225_RequestSeqNum },
  { &hf_h225_result         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_T_result },
  { &hf_h225_nonStandardData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_NonStandardParameter },
  { &hf_h225_tokens         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_ClearToken },
  { &hf_h225_cryptoTokens   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_CryptoH323Token },
  { &hf_h225_integrityCheckValue, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_ICV },
  { &hf_h225_featureSet     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_FeatureSet },
  { &hf_h225_genericData    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_h225_SEQUENCE_OF_GenericData },
  { NULL, 0, 0, NULL }
};

static int
dissect_h225_ServiceControlResponse(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_h225_ServiceControlResponse, ServiceControlResponse_sequence);

  return offset;
}


static const per_sequence_t SEQUENCE_OF_AdmissionConfirm_sequence_of[1] = {
  { &hf_h225_admissionConfirmSequence_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_h225_AdmissionConfirm },
};

static int
dissect_h225_SEQUENCE_OF_AdmissionConfirm(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence_of(tvb, offset, actx, tree, hf_index,
                                      ett_h225_SEQUENCE_OF_AdmissionConfirm, SEQUENCE_OF_AdmissionConfirm_sequence_of);

  return offset;
}


const value_string h225_RasMessage_vals[] = {
  {   0, "gatekeeperRequest" },
  {   1, "gatekeeperConfirm" },
  {   2, "gatekeeperReject" },
  {   3, "registrationRequest" },
  {   4, "registrationConfirm" },
  {   5, "registrationReject" },
  {   6, "unregistrationRequest" },
  {   7, "unregistrationConfirm" },
  {   8, "unregistrationReject" },
  {   9, "admissionRequest" },
  {  10, "admissionConfirm" },
  {  11, "admissionReject" },
  {  12, "bandwidthRequest" },
  {  13, "bandwidthConfirm" },
  {  14, "bandwidthReject" },
  {  15, "disengageRequest" },
  {  16, "disengageConfirm" },
  {  17, "disengageReject" },
  {  18, "locationRequest" },
  {  19, "locationConfirm" },
  {  20, "locationReject" },
  {  21, "infoRequest" },
  {  22, "infoRequestResponse" },
  {  23, "nonStandardMessage" },
  {  24, "unknownMessageResponse" },
  {  25, "requestInProgress" },
  {  26, "resourcesAvailableIndicate" },
  {  27, "resourcesAvailableConfirm" },
  {  28, "infoRequestAck" },
  {  29, "infoRequestNak" },
  {  30, "serviceControlIndication" },
  {  31, "serviceControlResponse" },
  {  32, "admissionConfirmSequence" },
  { 0, NULL }
};

static const per_choice_t RasMessage_choice[] = {
  {   0, &hf_h225_gatekeeperRequest, ASN1_EXTENSION_ROOT    , dissect_h225_GatekeeperRequest },
  {   1, &hf_h225_gatekeeperConfirm, ASN1_EXTENSION_ROOT    , dissect_h225_GatekeeperConfirm },
  {   2, &hf_h225_gatekeeperReject, ASN1_EXTENSION_ROOT    , dissect_h225_GatekeeperReject },
  {   3, &hf_h225_registrationRequest, ASN1_EXTENSION_ROOT    , dissect_h225_RegistrationRequest },
  {   4, &hf_h225_registrationConfirm, ASN1_EXTENSION_ROOT    , dissect_h225_RegistrationConfirm },
  {   5, &hf_h225_registrationReject, ASN1_EXTENSION_ROOT    , dissect_h225_RegistrationReject },
  {   6, &hf_h225_unregistrationRequest, ASN1_EXTENSION_ROOT    , dissect_h225_UnregistrationRequest },
  {   7, &hf_h225_unregistrationConfirm, ASN1_EXTENSION_ROOT    , dissect_h225_UnregistrationConfirm },
  {   8, &hf_h225_unregistrationReject, ASN1_EXTENSION_ROOT    , dissect_h225_UnregistrationReject },
  {   9, &hf_h225_admissionRequest, ASN1_EXTENSION_ROOT    , dissect_h225_AdmissionRequest },
  {  10, &hf_h225_admissionConfirm, ASN1_EXTENSION_ROOT    , dissect_h225_AdmissionConfirm },
  {  11, &hf_h225_admissionReject, ASN1_EXTENSION_ROOT    , dissect_h225_AdmissionReject },
  {  12, &hf_h225_bandwidthRequest, ASN1_EXTENSION_ROOT    , dissect_h225_BandwidthRequest },
  {  13, &hf_h225_bandwidthConfirm, ASN1_EXTENSION_ROOT    , dissect_h225_BandwidthConfirm },
  {  14, &hf_h225_bandwidthReject, ASN1_EXTENSION_ROOT    , dissect_h225_BandwidthReject },
  {  15, &hf_h225_disengageRequest, ASN1_EXTENSION_ROOT    , dissect_h225_DisengageRequest },
  {  16, &hf_h225_disengageConfirm, ASN1_EXTENSION_ROOT    , dissect_h225_DisengageConfirm },
  {  17, &hf_h225_disengageReject, ASN1_EXTENSION_ROOT    , dissect_h225_DisengageReject },
  {  18, &hf_h225_locationRequest, ASN1_EXTENSION_ROOT    , dissect_h225_LocationRequest },
  {  19, &hf_h225_locationConfirm, ASN1_EXTENSION_ROOT    , dissect_h225_LocationConfirm },
  {  20, &hf_h225_locationReject , ASN1_EXTENSION_ROOT    , dissect_h225_LocationReject },
  {  21, &hf_h225_infoRequest    , ASN1_EXTENSION_ROOT    , dissect_h225_InfoRequest },
  {  22, &hf_h225_infoRequestResponse, ASN1_EXTENSION_ROOT    , dissect_h225_InfoRequestResponse },
  {  23, &hf_h225_nonStandardMessage, ASN1_EXTENSION_ROOT    , dissect_h225_NonStandardMessage },
  {  24, &hf_h225_unknownMessageResponse, ASN1_EXTENSION_ROOT    , dissect_h225_UnknownMessageResponse },
  {  25, &hf_h225_requestInProgress, ASN1_NOT_EXTENSION_ROOT, dissect_h225_RequestInProgress },
  {  26, &hf_h225_resourcesAvailableIndicate, ASN1_NOT_EXTENSION_ROOT, dissect_h225_ResourcesAvailableIndicate },
  {  27, &hf_h225_resourcesAvailableConfirm, ASN1_NOT_EXTENSION_ROOT, dissect_h225_ResourcesAvailableConfirm },
  {  28, &hf_h225_infoRequestAck , ASN1_NOT_EXTENSION_ROOT, dissect_h225_InfoRequestAck },
  {  29, &hf_h225_infoRequestNak , ASN1_NOT_EXTENSION_ROOT, dissect_h225_InfoRequestNak },
  {  30, &hf_h225_serviceControlIndication, ASN1_NOT_EXTENSION_ROOT, dissect_h225_ServiceControlIndication },
  {  31, &hf_h225_serviceControlResponse, ASN1_NOT_EXTENSION_ROOT, dissect_h225_ServiceControlResponse },
  {  32, &hf_h225_admissionConfirmSequence, ASN1_NOT_EXTENSION_ROOT, dissect_h225_SEQUENCE_OF_AdmissionConfirm },
  { 0, NULL, 0, NULL }
};

int
dissect_h225_RasMessage(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 294 "./asn1/h225/h225.cnf"
    gint32 rasmessage_value;
    h225_packet_info* h225_pi;

  call_id_guid = NULL;
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_h225_RasMessage, RasMessage_choice,
                                 &rasmessage_value);

  col_add_fstr(actx->pinfo->cinfo, COL_INFO, "RAS: %s ",
    val_to_str(rasmessage_value, h225_RasMessage_vals, "<unknown>"));

  h225_pi = (h225_packet_info*)p_get_proto_data(wmem_packet_scope(), actx->pinfo, proto_h225, 0);
  if (h225_pi != NULL) {
    h225_pi->msg_tag = rasmessage_value;
    if (call_id_guid) {
      h225_pi->guid = *call_id_guid;
    }
  }


  return offset;
}

/*--- PDUs ---*/

static int dissect_H323_UserInformation_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_h225_H323_UserInformation(tvb, offset, &asn1_ctx, tree, hf_h225_H323_UserInformation_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
int dissect_h225_ExtendedAliasAddress_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_h225_ExtendedAliasAddress(tvb, offset, &asn1_ctx, tree, hf_h225_h225_ExtendedAliasAddress_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_RasMessage_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_h225_RasMessage(tvb, offset, &asn1_ctx, tree, hf_h225_RasMessage_PDU);
  offset += 7; offset >>= 3;
  return offset;
}


/*--- End of included file: packet-h225-fn.c ---*/
#line 250 "./asn1/h225/packet-h225-template.c"

/* Forward declaration we need below */
void proto_reg_handoff_h225(void);

/*
 * Functions needed for Ras-Hash-Table
 */

/* compare 2 keys */
static gint h225ras_call_equal(gconstpointer k1, gconstpointer k2)
{
  const h225ras_call_info_key* key1 = (const h225ras_call_info_key*) k1;
  const h225ras_call_info_key* key2 = (const h225ras_call_info_key*) k2;

  return (key1->reqSeqNum == key2->reqSeqNum &&
          key1->conversation == key2->conversation);
}

/* calculate a hash key */
static guint h225ras_call_hash(gconstpointer k)
{
  const h225ras_call_info_key* key = (const h225ras_call_info_key*) k;

  return key->reqSeqNum + GPOINTER_TO_UINT(key->conversation);
}


h225ras_call_t * find_h225ras_call(h225ras_call_info_key *h225ras_call_key ,int category)
{
  h225ras_call_t *h225ras_call = NULL;
  h225ras_call = (h225ras_call_t *)g_hash_table_lookup(ras_calls[category], h225ras_call_key);

  return h225ras_call;
}

h225ras_call_t * new_h225ras_call(h225ras_call_info_key *h225ras_call_key, packet_info *pinfo, e_guid_t *guid, int category)
{
  h225ras_call_info_key *new_h225ras_call_key;
  h225ras_call_t *h225ras_call = NULL;


  /* Prepare the value data.
     "req_num" and "rsp_num" are frame numbers;
     frame numbers are 1-origin, so we use 0
     to mean "we don't yet know in which frame
     the reply for this call appears". */
  new_h225ras_call_key = wmem_new(wmem_file_scope(), h225ras_call_info_key);
  new_h225ras_call_key->reqSeqNum = h225ras_call_key->reqSeqNum;
  new_h225ras_call_key->conversation = h225ras_call_key->conversation;
  h225ras_call = wmem_new(wmem_file_scope(), h225ras_call_t);
  h225ras_call->req_num = pinfo->num;
  h225ras_call->rsp_num = 0;
  h225ras_call->requestSeqNum = h225ras_call_key->reqSeqNum;
  h225ras_call->responded = FALSE;
  h225ras_call->next_call = NULL;
  h225ras_call->req_time=pinfo->abs_ts;
  h225ras_call->guid=*guid;
  /* store it */
  g_hash_table_insert(ras_calls[category], new_h225ras_call_key, h225ras_call);

  return h225ras_call;
}

h225ras_call_t * append_h225ras_call(h225ras_call_t *prev_call, packet_info *pinfo, e_guid_t *guid, int category _U_)
{
  h225ras_call_t *h225ras_call = NULL;

  /* Prepare the value data.
     "req_num" and "rsp_num" are frame numbers;
     frame numbers are 1-origin, so we use 0
     to mean "we don't yet know in which frame
     the reply for this call appears". */
  h225ras_call = wmem_new(wmem_file_scope(), h225ras_call_t);
  h225ras_call->req_num = pinfo->num;
  h225ras_call->rsp_num = 0;
  h225ras_call->requestSeqNum = prev_call->requestSeqNum;
  h225ras_call->responded = FALSE;
  h225ras_call->next_call = NULL;
  h225ras_call->req_time=pinfo->abs_ts;
  h225ras_call->guid=*guid;

  prev_call->next_call = h225ras_call;
  return h225ras_call;
}

/* Init routine for hash tables and delay calculation
   This routine will be called by Wireshark, before it
   is (re-)dissecting a trace file from beginning.
   We need to discard and init any state we've saved */

static void
h225_init_routine(void)
{
  int i;
  /* create new hash-tables for RAS SRT */

  for(i=0;i<7;i++) {
    ras_calls[i] = g_hash_table_new(h225ras_call_hash, h225ras_call_equal);
  }

}

static void
h225_cleanup_routine(void)
{
  int i;

  /* free hash-tables for RAS SRT */
  for(i=0;i<7;i++) {
    g_hash_table_destroy(ras_calls[i]);
  }
}

static int
dissect_h225_H323UserInformation(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
  proto_item *it;
  proto_tree *tr;
  int offset = 0;
  h225_packet_info* h225_pi;

  /* Init struct for collecting h225_packet_info */
  h225_pi = create_h225_packet_info(pinfo);
  h225_pi->msg_type = H225_CS;
  p_add_proto_data(wmem_packet_scope(), pinfo, proto_h225, 0, h225_pi);

  next_tvb_init(&h245_list);
  next_tvb_init(&tp_list);

  col_set_str(pinfo->cinfo, COL_PROTOCOL, PSNAME);
  col_clear(pinfo->cinfo, COL_INFO);

  it=proto_tree_add_protocol_format(tree, proto_h225, tvb, 0, -1, PSNAME" CS");
  tr=proto_item_add_subtree(it, ett_h225);

  offset = dissect_H323_UserInformation_PDU(tvb, pinfo, tr, NULL);

  if (h245_list.count){
    col_append_str(pinfo->cinfo, COL_PROTOCOL, "/");
    col_set_fence(pinfo->cinfo, COL_PROTOCOL);
  }

  next_tvb_call(&h245_list, pinfo, tree, h245dg_handle, data_handle);
  next_tvb_call(&tp_list, pinfo, tree, NULL, data_handle);

  tap_queue_packet(h225_tap, pinfo, h225_pi);

  return offset;
}
static int
dissect_h225_h225_RasMessage(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_){
  proto_item *it;
  proto_tree *tr;
  guint32 offset=0;
  h225_packet_info* h225_pi;

  /* Init struct for collecting h225_packet_info */
  h225_pi = create_h225_packet_info(pinfo);
  h225_pi->msg_type = H225_RAS;
  p_add_proto_data(wmem_packet_scope(), pinfo, proto_h225, 0, h225_pi);

  col_set_str(pinfo->cinfo, COL_PROTOCOL, PSNAME);

  it=proto_tree_add_protocol_format(tree, proto_h225, tvb, offset, -1, PSNAME" RAS");
  tr=proto_item_add_subtree(it, ett_h225);

  offset = dissect_RasMessage_PDU(tvb, pinfo, tr, NULL);

  ras_call_matching(tvb, pinfo, tr, h225_pi);

  tap_queue_packet(h225_tap, pinfo, h225_pi);

  return offset;
}


/* The following values represent the size of their valuestring arrays */

#define RAS_MSG_TYPES (sizeof(h225_RasMessage_vals) / sizeof(value_string))
#define CS_MSG_TYPES (sizeof(T_h323_message_body_vals) / sizeof(value_string))

#define GRJ_REASONS (sizeof(GatekeeperRejectReason_vals) / sizeof(value_string))
#define RRJ_REASONS (sizeof(RegistrationRejectReason_vals) / sizeof(value_string))
#define URQ_REASONS (sizeof(UnregRequestReason_vals) / sizeof(value_string))
#define URJ_REASONS (sizeof(UnregRejectReason_vals) / sizeof(value_string))
#define ARJ_REASONS (sizeof(AdmissionRejectReason_vals) / sizeof(value_string))
#define BRJ_REASONS (sizeof(BandRejectReason_vals) / sizeof(value_string))
#define DRQ_REASONS (sizeof(DisengageReason_vals) / sizeof(value_string))
#define DRJ_REASONS (sizeof(DisengageRejectReason_vals) / sizeof(value_string))
#define LRJ_REASONS (sizeof(LocationRejectReason_vals) / sizeof(value_string))
#define IRQNAK_REASONS (sizeof(InfoRequestNakReason_vals) / sizeof(value_string))
#define REL_CMP_REASONS (sizeof(h225_ReleaseCompleteReason_vals) / sizeof(value_string))
#define FACILITY_REASONS (sizeof(FacilityReason_vals) / sizeof(value_string))

/* TAP STAT INFO */
typedef enum
{
  MESSAGE_TYPE_COLUMN = 0,
  COUNT_COLUMN
} h225_stat_columns;

typedef struct _h225_table_item {
  guint count;     /* Message count */
  guint table_idx; /* stat_table index */
} h225_table_item_t;

static stat_tap_table_item h225_stat_fields[] = {{TABLE_ITEM_STRING, TAP_ALIGN_LEFT, "Message Type or Reason", "%-25s"}, {TABLE_ITEM_UINT, TAP_ALIGN_RIGHT, "Count", "%d"}};

static guint ras_msg_idx[RAS_MSG_TYPES];
static guint cs_msg_idx[CS_MSG_TYPES];

static guint grj_reason_idx[GRJ_REASONS];
static guint rrj_reason_idx[RRJ_REASONS];
static guint urq_reason_idx[URQ_REASONS];
static guint urj_reason_idx[URJ_REASONS];
static guint arj_reason_idx[ARJ_REASONS];
static guint brj_reason_idx[BRJ_REASONS];
static guint drq_reason_idx[DRQ_REASONS];
static guint drj_reason_idx[DRJ_REASONS];
static guint lrj_reason_idx[LRJ_REASONS];
static guint irqnak_reason_idx[IRQNAK_REASONS];
static guint rel_cmp_reason_idx[REL_CMP_REASONS];
static guint facility_reason_idx[FACILITY_REASONS];

static guint other_idx;

static void h225_stat_init(stat_tap_table_ui* new_stat, new_stat_tap_gui_init_cb gui_callback, void* gui_data)
{
  int num_fields = sizeof(h225_stat_fields)/sizeof(stat_tap_table_item);
  stat_tap_table* table = new_stat_tap_init_table("H.225 Messages and Message Reasons", num_fields, 0, NULL, gui_callback, gui_data);
  int row_idx = 0, msg_idx;
  stat_tap_table_item_type items[sizeof(h225_stat_fields)/sizeof(stat_tap_table_item)];

  new_stat_tap_add_table(new_stat, table);

  items[MESSAGE_TYPE_COLUMN].type = TABLE_ITEM_STRING;
  items[COUNT_COLUMN].type = TABLE_ITEM_UINT;
  items[COUNT_COLUMN].value.uint_value = 0;

  /* Add a row for each value type */

  msg_idx = 0;
  do
  {
    items[MESSAGE_TYPE_COLUMN].value.string_value =
      h225_RasMessage_vals[msg_idx].strptr
      ? h225_RasMessage_vals[msg_idx].strptr
      : "Unknown RAS message";
    ras_msg_idx[msg_idx] = row_idx;

    new_stat_tap_init_table_row(table, row_idx, num_fields, items);
    row_idx++;
    msg_idx++;
  } while (h225_RasMessage_vals[msg_idx].strptr);

  msg_idx = 0;
  do
  {
    items[MESSAGE_TYPE_COLUMN].value.string_value =
      T_h323_message_body_vals[msg_idx].strptr
      ? T_h323_message_body_vals[msg_idx].strptr
      : "Unknown CS message";
    cs_msg_idx[msg_idx] = row_idx;

    new_stat_tap_init_table_row(table, row_idx, num_fields, items);
    row_idx++;
    msg_idx++;
  } while (T_h323_message_body_vals[msg_idx].strptr);

  msg_idx = 0;
  do
  {
    items[MESSAGE_TYPE_COLUMN].value.string_value =
      GatekeeperRejectReason_vals[msg_idx].strptr
      ? GatekeeperRejectReason_vals[msg_idx].strptr
      : "Unknown gatekeeper reject reason";
    grj_reason_idx[msg_idx] = row_idx;

    new_stat_tap_init_table_row(table, row_idx, num_fields, items);
    row_idx++;
    msg_idx++;
  } while (GatekeeperRejectReason_vals[msg_idx].strptr);

  msg_idx = 0;
  do
  {
    items[MESSAGE_TYPE_COLUMN].value.string_value =
      RegistrationRejectReason_vals[msg_idx].strptr
      ? RegistrationRejectReason_vals[msg_idx].strptr
      : "Unknown registration reject reason";
    rrj_reason_idx[msg_idx] = row_idx;

    new_stat_tap_init_table_row(table, row_idx, num_fields, items);
    row_idx++;
    msg_idx++;
  } while (RegistrationRejectReason_vals[msg_idx].strptr);

  msg_idx = 0;
  do
  {
    items[MESSAGE_TYPE_COLUMN].value.string_value =
      UnregRequestReason_vals[msg_idx].strptr
      ? UnregRequestReason_vals[msg_idx].strptr
      : "Unknown unregistration request reason";
    urq_reason_idx[msg_idx] = row_idx;

    new_stat_tap_init_table_row(table, row_idx, num_fields, items);
    row_idx++;
    msg_idx++;
  } while (UnregRequestReason_vals[msg_idx].strptr);

  msg_idx = 0;
  do
  {
    items[MESSAGE_TYPE_COLUMN].value.string_value =
      UnregRejectReason_vals[msg_idx].strptr
      ? UnregRejectReason_vals[msg_idx].strptr
      : "Unknown unregistration reject reason";
    urj_reason_idx[msg_idx] = row_idx;

    new_stat_tap_init_table_row(table, row_idx, num_fields, items);
    row_idx++;
    msg_idx++;
  } while (UnregRejectReason_vals[msg_idx].strptr);

  msg_idx = 0;
  do
  {
    items[MESSAGE_TYPE_COLUMN].value.string_value =
      AdmissionRejectReason_vals[msg_idx].strptr
      ? AdmissionRejectReason_vals[msg_idx].strptr
      : "Unknown admission reject reason";
    arj_reason_idx[msg_idx] = row_idx;

    new_stat_tap_init_table_row(table, row_idx, num_fields, items);
    row_idx++;
    msg_idx++;
  } while (AdmissionRejectReason_vals[msg_idx].strptr);

  msg_idx = 0;
  do
  {
    items[MESSAGE_TYPE_COLUMN].value.string_value =
      BandRejectReason_vals[msg_idx].strptr
      ? BandRejectReason_vals[msg_idx].strptr
      : "Unknown band reject reason";
    brj_reason_idx[msg_idx] = row_idx;

    new_stat_tap_init_table_row(table, row_idx, num_fields, items);
    row_idx++;
    msg_idx++;
  } while (BandRejectReason_vals[msg_idx].strptr);

  msg_idx = 0;
  do
  {
    items[MESSAGE_TYPE_COLUMN].value.string_value =
      DisengageReason_vals[msg_idx].strptr
      ? DisengageReason_vals[msg_idx].strptr
      : "Unknown disengage reason";
    drq_reason_idx[msg_idx] = row_idx;

    new_stat_tap_init_table_row(table, row_idx, num_fields, items);
    row_idx++;
    msg_idx++;
  } while (DisengageReason_vals[msg_idx].strptr);

  msg_idx = 0;
  do
  {
    items[MESSAGE_TYPE_COLUMN].value.string_value =
      DisengageRejectReason_vals[msg_idx].strptr
      ? DisengageRejectReason_vals[msg_idx].strptr
      : "Unknown disengage reject reason";
    drj_reason_idx[msg_idx] = row_idx;

    new_stat_tap_init_table_row(table, row_idx, num_fields, items);
    row_idx++;
    msg_idx++;
  } while (DisengageRejectReason_vals[msg_idx].strptr);

  msg_idx = 0;
  do
  {
    items[MESSAGE_TYPE_COLUMN].value.string_value =
      LocationRejectReason_vals[msg_idx].strptr
      ? LocationRejectReason_vals[msg_idx].strptr
      : "Unknown location reject reason";
    lrj_reason_idx[msg_idx] = row_idx;

    new_stat_tap_init_table_row(table, row_idx, num_fields, items);
    row_idx++;
    msg_idx++;
  } while (LocationRejectReason_vals[msg_idx].strptr);

  msg_idx = 0;
  do
  {
    items[MESSAGE_TYPE_COLUMN].value.string_value =
      InfoRequestNakReason_vals[msg_idx].strptr
      ? InfoRequestNakReason_vals[msg_idx].strptr
      : "Unknown info request nak reason";
    irqnak_reason_idx[msg_idx] = row_idx;

    new_stat_tap_init_table_row(table, row_idx, num_fields, items);
    row_idx++;
    msg_idx++;
  } while (InfoRequestNakReason_vals[msg_idx].strptr);

  msg_idx = 0;
  do
  {
    items[MESSAGE_TYPE_COLUMN].value.string_value =
      h225_ReleaseCompleteReason_vals[msg_idx].strptr
      ? h225_ReleaseCompleteReason_vals[msg_idx].strptr
      : "Unknown release complete reason";
    rel_cmp_reason_idx[msg_idx] = row_idx;

    new_stat_tap_init_table_row(table, row_idx, num_fields, items);
    row_idx++;
    msg_idx++;
  } while (h225_ReleaseCompleteReason_vals[msg_idx].strptr);

  msg_idx = 0;
  do
  {
    items[MESSAGE_TYPE_COLUMN].value.string_value =
      FacilityReason_vals[msg_idx].strptr
      ? FacilityReason_vals[msg_idx].strptr
      : "Unknown facility reason";
    facility_reason_idx[msg_idx] = row_idx;

    new_stat_tap_init_table_row(table, row_idx, num_fields, items);
    row_idx++;
    msg_idx++;
  } while (FacilityReason_vals[msg_idx].strptr);


  items[MESSAGE_TYPE_COLUMN].value.string_value = "Unknown H.225 message";
  new_stat_tap_init_table_row(table, row_idx, num_fields, items);
  other_idx = row_idx;
}

static gboolean
h225_stat_packet(void *tapdata, packet_info *pinfo _U_, epan_dissect_t *edt _U_, const void *hpi_ptr)
{
  new_stat_data_t* stat_data = (new_stat_data_t*)tapdata;
  const h225_packet_info *hpi = (const h225_packet_info *)hpi_ptr;
  int tag_idx = -1;
  int reason_idx = -1;

  if(hpi->msg_tag < 0) { /* uninitialized */
    return FALSE;
  }

  switch (hpi->msg_type) {

  case H225_RAS:
    tag_idx = ras_msg_idx[MIN(hpi->msg_tag, (int)RAS_MSG_TYPES-1)];

    /* Look for reason tag */
    if(hpi->reason < 0) { /* uninitialized */
      break;
    }

    switch(hpi->msg_tag) {

    case 2: /* GRJ */
      reason_idx = grj_reason_idx[MIN(hpi->reason, (int)GRJ_REASONS-1)];
      break;
    case 5: /* RRJ */
      reason_idx = rrj_reason_idx[MIN(hpi->reason, (int)RRJ_REASONS-1)];
      break;
    case 6: /* URQ */
      reason_idx = urq_reason_idx[MIN(hpi->reason, (int)URQ_REASONS-1)];
      break;
    case 8: /* URJ */
      reason_idx = urj_reason_idx[MIN(hpi->reason, (int)URJ_REASONS-1)];
      break;
    case 11: /* ARJ */
      reason_idx = arj_reason_idx[MIN(hpi->reason, (int)ARJ_REASONS-1)];
      break;
    case 14: /* BRJ */
      reason_idx = brj_reason_idx[MIN(hpi->reason, (int)BRJ_REASONS-1)];
      break;
    case 15: /* DRQ */
      reason_idx = drq_reason_idx[MIN(hpi->reason, (int)DRQ_REASONS-1)];
      break;
    case 17: /* DRJ */
      reason_idx = drj_reason_idx[MIN(hpi->reason, (int)DRJ_REASONS-1)];
      break;
    case 20: /* LRJ */
      reason_idx = lrj_reason_idx[MIN(hpi->reason, (int)LRJ_REASONS-1)];
      break;
    case 29: /* IRQ Nak */
      reason_idx = irqnak_reason_idx[MIN(hpi->reason, (int)IRQNAK_REASONS-1)];
      break;
    default:
      /* do nothing */
      break;
    }

    break;

  case H225_CS:
    tag_idx = cs_msg_idx[MIN(hpi->msg_tag, (int)CS_MSG_TYPES-1)];

    /* Look for reason tag */
    if(hpi->reason < 0) { /* uninitialized */
      break;
    }

    switch(hpi->msg_tag) {

    case 5: /* ReleaseComplete */
      reason_idx = rel_cmp_reason_idx[MIN(hpi->reason, (int)REL_CMP_REASONS-1)];
      break;
    case 6: /* Facility */
      reason_idx = facility_reason_idx[MIN(hpi->reason, (int)FACILITY_REASONS-1)];
      break;
    default:
      /* do nothing */
      break;
    }

    break;

  case H225_OTHERS:
  default:
    tag_idx = other_idx;
  }

  if (tag_idx >= 0) {
    stat_tap_table*table = g_array_index(stat_data->stat_tap_data->tables, stat_tap_table*, 0);
    stat_tap_table_item_type* msg_data = new_stat_tap_get_field_data(table, tag_idx, COUNT_COLUMN);;
    msg_data->value.uint_value++;
    new_stat_tap_set_field_data(table, tag_idx, COUNT_COLUMN, msg_data);

    if (reason_idx >= 0) {
      msg_data = new_stat_tap_get_field_data(table, reason_idx, COUNT_COLUMN);;
      msg_data->value.uint_value++;
      new_stat_tap_set_field_data(table, reason_idx, COUNT_COLUMN, msg_data);
    }

    return TRUE;
  }
  return FALSE;
}

static void
h225_stat_reset(stat_tap_table* table)
{
  guint element;
  stat_tap_table_item_type* item_data;

  for (element = 0; element < table->num_elements; element++)
  {
    item_data = new_stat_tap_get_field_data(table, element, COUNT_COLUMN);
    item_data->value.uint_value = 0;
    new_stat_tap_set_field_data(table, element, COUNT_COLUMN, item_data);
  }
}

/*--- proto_register_h225 -------------------------------------------*/
void proto_register_h225(void) {

  /* List of fields */
  static hf_register_info hf[] = {
  { &hf_h221Manufacturer,
    { "H.225 Manufacturer", "h225.Manufacturer", FT_UINT32, BASE_HEX,
    VALS(H221ManufacturerCode_vals), 0, "h225.H.221 Manufacturer", HFILL }},

  { &hf_h225_ras_req_frame,
    { "RAS Request Frame", "h225.ras.reqframe", FT_FRAMENUM, BASE_NONE,
    NULL, 0, NULL, HFILL }},

  { &hf_h225_ras_rsp_frame,
    { "RAS Response Frame", "h225.ras.rspframe", FT_FRAMENUM, BASE_NONE,
    NULL, 0, NULL, HFILL }},

  { &hf_h225_ras_dup,
    { "Duplicate RAS Message", "h225.ras.dup", FT_UINT32, BASE_DEC,
    NULL, 0, NULL, HFILL }},

  { &hf_h225_ras_deltatime,
    { "RAS Service Response Time", "h225.ras.timedelta", FT_RELATIVE_TIME, BASE_NONE,
    NULL, 0, "Timedelta between RAS-Request and RAS-Response", HFILL }},

  { &hf_h225_debug_dissector_try_string,
    { "*** DEBUG dissector_try_string", "h225.debug.dissector_try_string", FT_STRING, BASE_NONE,
    NULL, 0, NULL, HFILL }},


/*--- Included file: packet-h225-hfarr.c ---*/
#line 1 "./asn1/h225/packet-h225-hfarr.c"
    { &hf_h225_H323_UserInformation_PDU,
      { "H323-UserInformation", "h225.H323_UserInformation_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_h225_ExtendedAliasAddress_PDU,
      { "ExtendedAliasAddress", "h225.ExtendedAliasAddress_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_RasMessage_PDU,
      { "RasMessage", "h225.RasMessage",
        FT_UINT32, BASE_DEC, VALS(h225_RasMessage_vals), 0,
        NULL, HFILL }},
    { &hf_h225_h323_uu_pdu,
      { "h323-uu-pdu", "h225.h323_uu_pdu_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_user_data,
      { "user-data", "h225.user_data_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_protocol_discriminator,
      { "protocol-discriminator", "h225.protocol_discriminator",
        FT_UINT32, BASE_DEC|BASE_EXT_STRING, &q931_protocol_discriminator_vals_ext, 0,
        "INTEGER_0_255", HFILL }},
    { &hf_h225_user_information,
      { "user-information", "h225.user_information",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING_SIZE_1_131", HFILL }},
    { &hf_h225_h323_message_body,
      { "h323-message-body", "h225.h323_message_body",
        FT_UINT32, BASE_DEC, VALS(T_h323_message_body_vals), 0,
        NULL, HFILL }},
    { &hf_h225_setup,
      { "setup", "h225.setup_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Setup_UUIE", HFILL }},
    { &hf_h225_callProceeding,
      { "callProceeding", "h225.callProceeding_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "CallProceeding_UUIE", HFILL }},
    { &hf_h225_connect,
      { "connect", "h225.connect_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Connect_UUIE", HFILL }},
    { &hf_h225_alerting,
      { "alerting", "h225.alerting_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Alerting_UUIE", HFILL }},
    { &hf_h225_information,
      { "information", "h225.information_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Information_UUIE", HFILL }},
    { &hf_h225_releaseComplete,
      { "releaseComplete", "h225.releaseComplete_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "ReleaseComplete_UUIE", HFILL }},
    { &hf_h225_facility,
      { "facility", "h225.facility_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Facility_UUIE", HFILL }},
    { &hf_h225_progress,
      { "progress", "h225.progress_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Progress_UUIE", HFILL }},
    { &hf_h225_empty_flg,
      { "empty", "h225.empty_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "T_empty_flg", HFILL }},
    { &hf_h225_status,
      { "status", "h225.status_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Status_UUIE", HFILL }},
    { &hf_h225_statusInquiry,
      { "statusInquiry", "h225.statusInquiry_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "StatusInquiry_UUIE", HFILL }},
    { &hf_h225_setupAcknowledge,
      { "setupAcknowledge", "h225.setupAcknowledge_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "SetupAcknowledge_UUIE", HFILL }},
    { &hf_h225_notify,
      { "notify", "h225.notify_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Notify_UUIE", HFILL }},
    { &hf_h225_nonStandardData,
      { "nonStandardData", "h225.nonStandardData_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "NonStandardParameter", HFILL }},
    { &hf_h225_h4501SupplementaryService,
      { "h4501SupplementaryService", "h225.h4501SupplementaryService",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_h4501SupplementaryService_item,
      { "h4501SupplementaryService item", "h225.h4501SupplementaryService_item",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_h245Tunnelling,
      { "h245Tunnelling", "h225.h245Tunnelling",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_H245Control_item,
      { "H245Control item", "h225.H245Control_item",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_h245Control,
      { "h245Control", "h225.h245Control",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_nonStandardControl,
      { "nonStandardControl", "h225.nonStandardControl",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_NonStandardParameter", HFILL }},
    { &hf_h225_nonStandardControl_item,
      { "NonStandardParameter", "h225.NonStandardParameter_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_callLinkage,
      { "callLinkage", "h225.callLinkage_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_tunnelledSignallingMessage,
      { "tunnelledSignallingMessage", "h225.tunnelledSignallingMessage_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_tunnelledProtocolID,
      { "tunnelledProtocolID", "h225.tunnelledProtocolID_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TunnelledProtocol", HFILL }},
    { &hf_h225_messageContent,
      { "messageContent", "h225.messageContent",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_messageContent_item,
      { "messageContent item", "h225.messageContent_item",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_messageContent_item", HFILL }},
    { &hf_h225_tunnellingRequired,
      { "tunnellingRequired", "h225.tunnellingRequired_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_provisionalRespToH245Tunnelling,
      { "provisionalRespToH245Tunnelling", "h225.provisionalRespToH245Tunnelling_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_stimulusControl,
      { "stimulusControl", "h225.stimulusControl_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_genericData,
      { "genericData", "h225.genericData",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_GenericData", HFILL }},
    { &hf_h225_genericData_item,
      { "GenericData", "h225.GenericData_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_nonStandard,
      { "nonStandard", "h225.nonStandard_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "NonStandardParameter", HFILL }},
    { &hf_h225_isText,
      { "isText", "h225.isText_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_h248Message,
      { "h248Message", "h225.h248Message",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING", HFILL }},
    { &hf_h225_protocolIdentifier,
      { "protocolIdentifier", "h225.protocolIdentifier",
        FT_OID, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_uUIE_destinationInfo,
      { "destinationInfo", "h225.destinationInfo_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "EndpointType", HFILL }},
    { &hf_h225_h245Address,
      { "h245Address", "h225.h245Address",
        FT_UINT32, BASE_DEC, VALS(h225_H245TransportAddress_vals), 0,
        "H245TransportAddress", HFILL }},
    { &hf_h225_callIdentifier,
      { "callIdentifier", "h225.callIdentifier_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_h245SecurityMode,
      { "h245SecurityMode", "h225.h245SecurityMode",
        FT_UINT32, BASE_DEC, VALS(h225_H245Security_vals), 0,
        "H245Security", HFILL }},
    { &hf_h225_tokens,
      { "tokens", "h225.tokens",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_ClearToken", HFILL }},
    { &hf_h225_tokens_item,
      { "ClearToken", "h225.ClearToken_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_cryptoTokens,
      { "cryptoTokens", "h225.cryptoTokens",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_CryptoH323Token", HFILL }},
    { &hf_h225_cryptoTokens_item,
      { "CryptoH323Token", "h225.CryptoH323Token",
        FT_UINT32, BASE_DEC, VALS(h225_CryptoH323Token_vals), 0,
        NULL, HFILL }},
    { &hf_h225_fastStart,
      { "fastStart", "h225.fastStart",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_multipleCalls,
      { "multipleCalls", "h225.multipleCalls",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_maintainConnection,
      { "maintainConnection", "h225.maintainConnection",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_alertingAddress,
      { "alertingAddress", "h225.alertingAddress",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_AliasAddress", HFILL }},
    { &hf_h225_alertingAddress_item,
      { "AliasAddress", "h225.AliasAddress",
        FT_UINT32, BASE_DEC, VALS(AliasAddress_vals), 0,
        NULL, HFILL }},
    { &hf_h225_presentationIndicator,
      { "presentationIndicator", "h225.presentationIndicator",
        FT_UINT32, BASE_DEC, VALS(h225_PresentationIndicator_vals), 0,
        NULL, HFILL }},
    { &hf_h225_screeningIndicator,
      { "screeningIndicator", "h225.screeningIndicator",
        FT_UINT32, BASE_DEC, VALS(h225_ScreeningIndicator_vals), 0,
        NULL, HFILL }},
    { &hf_h225_fastConnectRefused,
      { "fastConnectRefused", "h225.fastConnectRefused_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_serviceControl,
      { "serviceControl", "h225.serviceControl",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_ServiceControlSession", HFILL }},
    { &hf_h225_serviceControl_item,
      { "ServiceControlSession", "h225.ServiceControlSession_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_capacity,
      { "capacity", "h225.capacity_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "CallCapacity", HFILL }},
    { &hf_h225_featureSet,
      { "featureSet", "h225.featureSet_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_displayName,
      { "displayName", "h225.displayName",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_DisplayName", HFILL }},
    { &hf_h225_displayName_item,
      { "DisplayName", "h225.DisplayName_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_conferenceID,
      { "conferenceID", "h225.conferenceID",
        FT_GUID, BASE_NONE, NULL, 0,
        "ConferenceIdentifier", HFILL }},
    { &hf_h225_language,
      { "language", "h225.language",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_connectedAddress,
      { "connectedAddress", "h225.connectedAddress",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_AliasAddress", HFILL }},
    { &hf_h225_connectedAddress_item,
      { "AliasAddress", "h225.AliasAddress",
        FT_UINT32, BASE_DEC, VALS(AliasAddress_vals), 0,
        NULL, HFILL }},
    { &hf_h225_circuitInfo,
      { "circuitInfo", "h225.circuitInfo_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_releaseCompleteReason,
      { "reason", "h225.reason",
        FT_UINT32, BASE_DEC, VALS(h225_ReleaseCompleteReason_vals), 0,
        "ReleaseCompleteReason", HFILL }},
    { &hf_h225_busyAddress,
      { "busyAddress", "h225.busyAddress",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_AliasAddress", HFILL }},
    { &hf_h225_busyAddress_item,
      { "AliasAddress", "h225.AliasAddress",
        FT_UINT32, BASE_DEC, VALS(AliasAddress_vals), 0,
        NULL, HFILL }},
    { &hf_h225_destinationInfo,
      { "destinationInfo", "h225.destinationInfo_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "EndpointType", HFILL }},
    { &hf_h225_noBandwidth,
      { "noBandwidth", "h225.noBandwidth_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_gatekeeperResources,
      { "gatekeeperResources", "h225.gatekeeperResources_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_unreachableDestination,
      { "unreachableDestination", "h225.unreachableDestination_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_destinationRejection,
      { "destinationRejection", "h225.destinationRejection_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_invalidRevision,
      { "invalidRevision", "h225.invalidRevision_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_noPermission,
      { "noPermission", "h225.noPermission_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_unreachableGatekeeper,
      { "unreachableGatekeeper", "h225.unreachableGatekeeper_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_gatewayResources,
      { "gatewayResources", "h225.gatewayResources_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_badFormatAddress,
      { "badFormatAddress", "h225.badFormatAddress_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_adaptiveBusy,
      { "adaptiveBusy", "h225.adaptiveBusy_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_inConf,
      { "inConf", "h225.inConf_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_undefinedReason,
      { "undefinedReason", "h225.undefinedReason_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_facilityCallDeflection,
      { "facilityCallDeflection", "h225.facilityCallDeflection_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_securityDenied,
      { "securityDenied", "h225.securityDenied_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_calledPartyNotRegistered,
      { "calledPartyNotRegistered", "h225.calledPartyNotRegistered_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_callerNotRegistered,
      { "callerNotRegistered", "h225.callerNotRegistered_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_newConnectionNeeded,
      { "newConnectionNeeded", "h225.newConnectionNeeded_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_nonStandardReason,
      { "nonStandardReason", "h225.nonStandardReason_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "NonStandardParameter", HFILL }},
    { &hf_h225_replaceWithConferenceInvite,
      { "replaceWithConferenceInvite", "h225.replaceWithConferenceInvite",
        FT_GUID, BASE_NONE, NULL, 0,
        "ConferenceIdentifier", HFILL }},
    { &hf_h225_genericDataReason,
      { "genericDataReason", "h225.genericDataReason_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_neededFeatureNotSupported,
      { "neededFeatureNotSupported", "h225.neededFeatureNotSupported_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_tunnelledSignallingRejected,
      { "tunnelledSignallingRejected", "h225.tunnelledSignallingRejected_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_invalidCID,
      { "invalidCID", "h225.invalidCID_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_rLC_securityError,
      { "securityError", "h225.securityError",
        FT_UINT32, BASE_DEC, VALS(h225_SecurityErrors_vals), 0,
        "SecurityErrors", HFILL }},
    { &hf_h225_hopCountExceeded,
      { "hopCountExceeded", "h225.hopCountExceeded_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_sourceAddress,
      { "sourceAddress", "h225.sourceAddress",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_AliasAddress", HFILL }},
    { &hf_h225_sourceAddress_item,
      { "AliasAddress", "h225.AliasAddress",
        FT_UINT32, BASE_DEC, VALS(AliasAddress_vals), 0,
        NULL, HFILL }},
    { &hf_h225_setup_UUIE_sourceInfo,
      { "sourceInfo", "h225.sourceInfo_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "EndpointType", HFILL }},
    { &hf_h225_destinationAddress,
      { "destinationAddress", "h225.destinationAddress",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_AliasAddress", HFILL }},
    { &hf_h225_destinationAddress_item,
      { "AliasAddress", "h225.AliasAddress",
        FT_UINT32, BASE_DEC, VALS(AliasAddress_vals), 0,
        NULL, HFILL }},
    { &hf_h225_destCallSignalAddress,
      { "destCallSignalAddress", "h225.destCallSignalAddress",
        FT_UINT32, BASE_DEC, VALS(h225_TransportAddress_vals), 0,
        "TransportAddress", HFILL }},
    { &hf_h225_destExtraCallInfo,
      { "destExtraCallInfo", "h225.destExtraCallInfo",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_AliasAddress", HFILL }},
    { &hf_h225_destExtraCallInfo_item,
      { "AliasAddress", "h225.AliasAddress",
        FT_UINT32, BASE_DEC, VALS(AliasAddress_vals), 0,
        NULL, HFILL }},
    { &hf_h225_destExtraCRV,
      { "destExtraCRV", "h225.destExtraCRV",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_CallReferenceValue", HFILL }},
    { &hf_h225_destExtraCRV_item,
      { "CallReferenceValue", "h225.CallReferenceValue",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_activeMC,
      { "activeMC", "h225.activeMC",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_conferenceGoal,
      { "conferenceGoal", "h225.conferenceGoal",
        FT_UINT32, BASE_DEC, VALS(h225_T_conferenceGoal_vals), 0,
        NULL, HFILL }},
    { &hf_h225_create,
      { "create", "h225.create_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_join,
      { "join", "h225.join_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_invite,
      { "invite", "h225.invite_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_capability_negotiation,
      { "capability-negotiation", "h225.capability_negotiation_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_callIndependentSupplementaryService,
      { "callIndependentSupplementaryService", "h225.callIndependentSupplementaryService_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_callServices,
      { "callServices", "h225.callServices_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "QseriesOptions", HFILL }},
    { &hf_h225_callType,
      { "callType", "h225.callType",
        FT_UINT32, BASE_DEC, VALS(h225_CallType_vals), 0,
        NULL, HFILL }},
    { &hf_h225_sourceCallSignalAddress,
      { "sourceCallSignalAddress", "h225.sourceCallSignalAddress",
        FT_UINT32, BASE_DEC, VALS(h225_TransportAddress_vals), 0,
        "TransportAddress", HFILL }},
    { &hf_h225_uUIE_remoteExtensionAddress,
      { "remoteExtensionAddress", "h225.remoteExtensionAddress",
        FT_UINT32, BASE_DEC, VALS(AliasAddress_vals), 0,
        "AliasAddress", HFILL }},
    { &hf_h225_h245SecurityCapability,
      { "h245SecurityCapability", "h225.h245SecurityCapability",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_H245Security", HFILL }},
    { &hf_h225_h245SecurityCapability_item,
      { "H245Security", "h225.H245Security",
        FT_UINT32, BASE_DEC, VALS(h225_H245Security_vals), 0,
        NULL, HFILL }},
    { &hf_h225_FastStart_item,
      { "FastStart item", "h225.FastStart_item",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_mediaWaitForConnect,
      { "mediaWaitForConnect", "h225.mediaWaitForConnect",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_canOverlapSend,
      { "canOverlapSend", "h225.canOverlapSend",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_endpointIdentifier,
      { "endpointIdentifier", "h225.endpointIdentifier",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_connectionParameters,
      { "connectionParameters", "h225.connectionParameters_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_connectionType,
      { "connectionType", "h225.connectionType",
        FT_UINT32, BASE_DEC, VALS(h225_ScnConnectionType_vals), 0,
        "ScnConnectionType", HFILL }},
    { &hf_h225_numberOfScnConnections,
      { "numberOfScnConnections", "h225.numberOfScnConnections",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_65535", HFILL }},
    { &hf_h225_connectionAggregation,
      { "connectionAggregation", "h225.connectionAggregation",
        FT_UINT32, BASE_DEC, VALS(h225_ScnConnectionAggregation_vals), 0,
        "ScnConnectionAggregation", HFILL }},
    { &hf_h225_Language_item,
      { "Language item", "h225.Language_item",
        FT_STRING, BASE_NONE, NULL, 0,
        "IA5String_SIZE_1_32", HFILL }},
    { &hf_h225_symmetricOperationRequired,
      { "symmetricOperationRequired", "h225.symmetricOperationRequired_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_desiredProtocols,
      { "desiredProtocols", "h225.desiredProtocols",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_SupportedProtocols", HFILL }},
    { &hf_h225_desiredProtocols_item,
      { "SupportedProtocols", "h225.SupportedProtocols",
        FT_UINT32, BASE_DEC, VALS(h225_SupportedProtocols_vals), 0,
        NULL, HFILL }},
    { &hf_h225_neededFeatures,
      { "neededFeatures", "h225.neededFeatures",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_FeatureDescriptor", HFILL }},
    { &hf_h225_neededFeatures_item,
      { "FeatureDescriptor", "h225.FeatureDescriptor_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_desiredFeatures,
      { "desiredFeatures", "h225.desiredFeatures",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_FeatureDescriptor", HFILL }},
    { &hf_h225_desiredFeatures_item,
      { "FeatureDescriptor", "h225.FeatureDescriptor_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_supportedFeatures,
      { "supportedFeatures", "h225.supportedFeatures",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_FeatureDescriptor", HFILL }},
    { &hf_h225_supportedFeatures_item,
      { "FeatureDescriptor", "h225.FeatureDescriptor_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_ParallelH245Control_item,
      { "ParallelH245Control item", "h225.ParallelH245Control_item",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_parallelH245Control,
      { "parallelH245Control", "h225.parallelH245Control",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_additionalSourceAddresses,
      { "additionalSourceAddresses", "h225.additionalSourceAddresses",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_ExtendedAliasAddress", HFILL }},
    { &hf_h225_additionalSourceAddresses_item,
      { "ExtendedAliasAddress", "h225.ExtendedAliasAddress_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_hopCount_1_31,
      { "hopCount", "h225.hopCount",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_31", HFILL }},
    { &hf_h225_unknown,
      { "unknown", "h225.unknown_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_bChannel,
      { "bChannel", "h225.bChannel_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_hybrid2x64,
      { "hybrid2x64", "h225.hybrid2x64_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_hybrid384,
      { "hybrid384", "h225.hybrid384_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_hybrid1536,
      { "hybrid1536", "h225.hybrid1536_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_hybrid1920,
      { "hybrid1920", "h225.hybrid1920_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_multirate,
      { "multirate", "h225.multirate_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_auto,
      { "auto", "h225.auto_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_none,
      { "none", "h225.none_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_h221,
      { "h221", "h225.h221_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_bonded_mode1,
      { "bonded-mode1", "h225.bonded_mode1_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_bonded_mode2,
      { "bonded-mode2", "h225.bonded_mode2_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_bonded_mode3,
      { "bonded-mode3", "h225.bonded_mode3_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_presentationAllowed,
      { "presentationAllowed", "h225.presentationAllowed_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_presentationRestricted,
      { "presentationRestricted", "h225.presentationRestricted_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_addressNotAvailable,
      { "addressNotAvailable", "h225.addressNotAvailable_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_alternativeAddress,
      { "alternativeAddress", "h225.alternativeAddress",
        FT_UINT32, BASE_DEC, VALS(h225_TransportAddress_vals), 0,
        "TransportAddress", HFILL }},
    { &hf_h225_alternativeAliasAddress,
      { "alternativeAliasAddress", "h225.alternativeAliasAddress",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_AliasAddress", HFILL }},
    { &hf_h225_alternativeAliasAddress_item,
      { "AliasAddress", "h225.AliasAddress",
        FT_UINT32, BASE_DEC, VALS(AliasAddress_vals), 0,
        NULL, HFILL }},
    { &hf_h225_facilityReason,
      { "reason", "h225.reason",
        FT_UINT32, BASE_DEC, VALS(FacilityReason_vals), 0,
        "FacilityReason", HFILL }},
    { &hf_h225_conferences,
      { "conferences", "h225.conferences",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_ConferenceList", HFILL }},
    { &hf_h225_conferences_item,
      { "ConferenceList", "h225.ConferenceList_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_conferenceAlias,
      { "conferenceAlias", "h225.conferenceAlias",
        FT_UINT32, BASE_DEC, VALS(AliasAddress_vals), 0,
        "AliasAddress", HFILL }},
    { &hf_h225_routeCallToGatekeeper,
      { "routeCallToGatekeeper", "h225.routeCallToGatekeeper_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_callForwarded,
      { "callForwarded", "h225.callForwarded_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_routeCallToMC,
      { "routeCallToMC", "h225.routeCallToMC_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_conferenceListChoice,
      { "conferenceListChoice", "h225.conferenceListChoice_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_startH245,
      { "startH245", "h225.startH245_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_noH245,
      { "noH245", "h225.noH245_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_newTokens,
      { "newTokens", "h225.newTokens_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_featureSetUpdate,
      { "featureSetUpdate", "h225.featureSetUpdate_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_forwardedElements,
      { "forwardedElements", "h225.forwardedElements_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_transportedInformation,
      { "transportedInformation", "h225.transportedInformation_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_h245IpAddress,
      { "ipAddress", "h225.ipAddress_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "T_h245IpAddress", HFILL }},
    { &hf_h225_h245Ip,
      { "ip", "h225.ip",
        FT_IPv4, BASE_NONE, NULL, 0,
        "T_h245Ip", HFILL }},
    { &hf_h225_h245IpPort,
      { "port", "h225.port",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_h245IpPort", HFILL }},
    { &hf_h225_h245IpSourceRoute,
      { "ipSourceRoute", "h225.ipSourceRoute_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "T_h245IpSourceRoute", HFILL }},
    { &hf_h225_ip,
      { "ip", "h225.ip",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING_SIZE_4", HFILL }},
    { &hf_h225_port,
      { "port", "h225.port",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_65535", HFILL }},
    { &hf_h225_h245Route,
      { "route", "h225.route",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_h245Route", HFILL }},
    { &hf_h225_h245Route_item,
      { "route item", "h225.route_item",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING_SIZE_4", HFILL }},
    { &hf_h225_h245Routing,
      { "routing", "h225.routing",
        FT_UINT32, BASE_DEC, VALS(h225_T_h245Routing_vals), 0,
        "T_h245Routing", HFILL }},
    { &hf_h225_strict,
      { "strict", "h225.strict_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_loose,
      { "loose", "h225.loose_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_h245IpxAddress,
      { "ipxAddress", "h225.ipxAddress_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "T_h245IpxAddress", HFILL }},
    { &hf_h225_node,
      { "node", "h225.node",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING_SIZE_6", HFILL }},
    { &hf_h225_netnum,
      { "netnum", "h225.netnum",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING_SIZE_4", HFILL }},
    { &hf_h225_h245IpxPort,
      { "port", "h225.port",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING_SIZE_2", HFILL }},
    { &hf_h225_h245Ip6Address,
      { "ip6Address", "h225.ip6Address_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "T_h245Ip6Address", HFILL }},
    { &hf_h225_h245Ip6,
      { "ip", "h225.ip",
        FT_IPv6, BASE_NONE, NULL, 0,
        "T_h245Ip6", HFILL }},
    { &hf_h225_port_01,
      { "port", "h225.port",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_netBios,
      { "netBios", "h225.netBios",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING_SIZE_16", HFILL }},
    { &hf_h225_nsap,
      { "nsap", "h225.nsap",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING_SIZE_1_20", HFILL }},
    { &hf_h225_nonStandardAddress,
      { "nonStandardAddress", "h225.nonStandardAddress_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "NonStandardParameter", HFILL }},
    { &hf_h225_ipAddress,
      { "ipAddress", "h225.ipAddress_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_ipV4,
      { "ip", "h225.ip",
        FT_IPv4, BASE_NONE, NULL, 0,
        "IpV4", HFILL }},
    { &hf_h225_ipV4_port,
      { "port", "h225.port",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_65535", HFILL }},
    { &hf_h225_ipSourceRoute,
      { "ipSourceRoute", "h225.ipSourceRoute_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_src_route_ipV4,
      { "ip", "h225.ip",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING_SIZE_4", HFILL }},
    { &hf_h225_ipV4_src_port,
      { "port", "h225.port",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_65535", HFILL }},
    { &hf_h225_route,
      { "route", "h225.route",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_route_item,
      { "route item", "h225.route_item",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING_SIZE_4", HFILL }},
    { &hf_h225_routing,
      { "routing", "h225.routing",
        FT_UINT32, BASE_DEC, VALS(h225_T_routing_vals), 0,
        NULL, HFILL }},
    { &hf_h225_ipxAddress,
      { "ipxAddress", "h225.ipxAddress_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_ipx_port,
      { "port", "h225.port",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING_SIZE_2", HFILL }},
    { &hf_h225_ip6Address,
      { "ip6Address", "h225.ip6Address_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_ipV6,
      { "ip", "h225.ip",
        FT_IPv6, BASE_NONE, NULL, 0,
        "OCTET_STRING_SIZE_16", HFILL }},
    { &hf_h225_ipV6_port,
      { "port", "h225.port",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_65535", HFILL }},
    { &hf_h225_vendor,
      { "vendor", "h225.vendor_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "VendorIdentifier", HFILL }},
    { &hf_h225_gatekeeper,
      { "gatekeeper", "h225.gatekeeper_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "GatekeeperInfo", HFILL }},
    { &hf_h225_gateway,
      { "gateway", "h225.gateway_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "GatewayInfo", HFILL }},
    { &hf_h225_mcu,
      { "mcu", "h225.mcu_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "McuInfo", HFILL }},
    { &hf_h225_terminal,
      { "terminal", "h225.terminal_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TerminalInfo", HFILL }},
    { &hf_h225_mc,
      { "mc", "h225.mc",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_undefinedNode,
      { "undefinedNode", "h225.undefinedNode",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_set,
      { "set", "h225.set",
        FT_BYTES, BASE_NONE, NULL, 0,
        "BIT_STRING_SIZE_32", HFILL }},
    { &hf_h225_supportedTunnelledProtocols,
      { "supportedTunnelledProtocols", "h225.supportedTunnelledProtocols",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_TunnelledProtocol", HFILL }},
    { &hf_h225_supportedTunnelledProtocols_item,
      { "TunnelledProtocol", "h225.TunnelledProtocol_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_protocol,
      { "protocol", "h225.protocol",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_SupportedProtocols", HFILL }},
    { &hf_h225_protocol_item,
      { "SupportedProtocols", "h225.SupportedProtocols",
        FT_UINT32, BASE_DEC, VALS(h225_SupportedProtocols_vals), 0,
        NULL, HFILL }},
    { &hf_h225_h310,
      { "h310", "h225.h310_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "H310Caps", HFILL }},
    { &hf_h225_h320,
      { "h320", "h225.h320_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "H320Caps", HFILL }},
    { &hf_h225_h321,
      { "h321", "h225.h321_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "H321Caps", HFILL }},
    { &hf_h225_h322,
      { "h322", "h225.h322_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "H322Caps", HFILL }},
    { &hf_h225_h323,
      { "h323", "h225.h323_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "H323Caps", HFILL }},
    { &hf_h225_h324,
      { "h324", "h225.h324_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "H324Caps", HFILL }},
    { &hf_h225_voice,
      { "voice", "h225.voice_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "VoiceCaps", HFILL }},
    { &hf_h225_t120_only,
      { "t120-only", "h225.t120_only_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "T120OnlyCaps", HFILL }},
    { &hf_h225_nonStandardProtocol,
      { "nonStandardProtocol", "h225.nonStandardProtocol_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_t38FaxAnnexbOnly,
      { "t38FaxAnnexbOnly", "h225.t38FaxAnnexbOnly_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "T38FaxAnnexbOnlyCaps", HFILL }},
    { &hf_h225_sip,
      { "sip", "h225.sip_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "SIPCaps", HFILL }},
    { &hf_h225_dataRatesSupported,
      { "dataRatesSupported", "h225.dataRatesSupported",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_DataRate", HFILL }},
    { &hf_h225_dataRatesSupported_item,
      { "DataRate", "h225.DataRate_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_supportedPrefixes,
      { "supportedPrefixes", "h225.supportedPrefixes",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_SupportedPrefix", HFILL }},
    { &hf_h225_supportedPrefixes_item,
      { "SupportedPrefix", "h225.SupportedPrefix_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_t38FaxProtocol,
      { "t38FaxProtocol", "h225.t38FaxProtocol",
        FT_UINT32, BASE_DEC, VALS(DataProtocolCapability_vals), 0,
        "DataProtocolCapability", HFILL }},
    { &hf_h225_t38FaxProfile,
      { "t38FaxProfile", "h225.t38FaxProfile_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_vendorIdentifier_vendor,
      { "vendor", "h225.vendor_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "H221NonStandard", HFILL }},
    { &hf_h225_productId,
      { "productId", "h225.productId",
        FT_STRING, BASE_NONE, NULL, 0,
        "OCTET_STRING_SIZE_1_256", HFILL }},
    { &hf_h225_versionId,
      { "versionId", "h225.versionId",
        FT_STRING, BASE_NONE, NULL, 0,
        "OCTET_STRING_SIZE_1_256", HFILL }},
    { &hf_h225_enterpriseNumber,
      { "enterpriseNumber", "h225.enterpriseNumber",
        FT_OID, BASE_NONE, NULL, 0,
        "OBJECT_IDENTIFIER", HFILL }},
    { &hf_h225_t35CountryCode,
      { "t35CountryCode", "h225.t35CountryCode",
        FT_UINT32, BASE_DEC, VALS(T35CountryCode_vals), 0,
        NULL, HFILL }},
    { &hf_h225_t35Extension,
      { "t35Extension", "h225.t35Extension",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_manufacturerCode,
      { "manufacturerCode", "h225.manufacturerCode",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_tunnelledProtocol_id,
      { "id", "h225.id",
        FT_UINT32, BASE_DEC, VALS(h225_TunnelledProtocol_id_vals), 0,
        "TunnelledProtocol_id", HFILL }},
    { &hf_h225_tunnelledProtocolObjectID,
      { "tunnelledProtocolObjectID", "h225.tunnelledProtocolObjectID",
        FT_OID, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_tunnelledProtocolAlternateID,
      { "tunnelledProtocolAlternateID", "h225.tunnelledProtocolAlternateID_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TunnelledProtocolAlternateIdentifier", HFILL }},
    { &hf_h225_subIdentifier,
      { "subIdentifier", "h225.subIdentifier",
        FT_STRING, BASE_NONE, NULL, 0,
        "IA5String_SIZE_1_64", HFILL }},
    { &hf_h225_protocolType,
      { "protocolType", "h225.protocolType",
        FT_STRING, BASE_NONE, NULL, 0,
        "IA5String_SIZE_1_64", HFILL }},
    { &hf_h225_protocolVariant,
      { "protocolVariant", "h225.protocolVariant",
        FT_STRING, BASE_NONE, NULL, 0,
        "IA5String_SIZE_1_64", HFILL }},
    { &hf_h225_nonStandardIdentifier,
      { "nonStandardIdentifier", "h225.nonStandardIdentifier",
        FT_UINT32, BASE_DEC, VALS(h225_NonStandardIdentifier_vals), 0,
        NULL, HFILL }},
    { &hf_h225_nsp_data,
      { "data", "h225.data",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_nsp_data", HFILL }},
    { &hf_h225_nsiOID,
      { "object", "h225.object",
        FT_OID, BASE_NONE, NULL, 0,
        "T_nsiOID", HFILL }},
    { &hf_h225_h221NonStandard,
      { "h221NonStandard", "h225.h221NonStandard_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_dialledDigits,
      { "dialledDigits", "h225.dialledDigits",
        FT_STRING, BASE_NONE, NULL, 0,
        "DialedDigits", HFILL }},
    { &hf_h225_h323_ID,
      { "h323-ID", "h225.h323_ID",
        FT_STRING, BASE_NONE, NULL, 0,
        "BMPString_SIZE_1_256", HFILL }},
    { &hf_h225_url_ID,
      { "url-ID", "h225.url_ID",
        FT_STRING, BASE_NONE, NULL, 0,
        "IA5String_SIZE_1_512", HFILL }},
    { &hf_h225_transportID,
      { "transportID", "h225.transportID",
        FT_UINT32, BASE_DEC, VALS(h225_TransportAddress_vals), 0,
        "TransportAddress", HFILL }},
    { &hf_h225_email_ID,
      { "email-ID", "h225.email_ID",
        FT_STRING, BASE_NONE, NULL, 0,
        "IA5String_SIZE_1_512", HFILL }},
    { &hf_h225_partyNumber,
      { "partyNumber", "h225.partyNumber",
        FT_UINT32, BASE_DEC, VALS(h225_PartyNumber_vals), 0,
        NULL, HFILL }},
    { &hf_h225_mobileUIM,
      { "mobileUIM", "h225.mobileUIM",
        FT_UINT32, BASE_DEC, VALS(h225_MobileUIM_vals), 0,
        NULL, HFILL }},
    { &hf_h225_isupNumber,
      { "isupNumber", "h225.isupNumber",
        FT_UINT32, BASE_DEC, VALS(h225_IsupNumber_vals), 0,
        NULL, HFILL }},
    { &hf_h225_wildcard,
      { "wildcard", "h225.wildcard",
        FT_UINT32, BASE_DEC, VALS(AliasAddress_vals), 0,
        "AliasAddress", HFILL }},
    { &hf_h225_range,
      { "range", "h225.range_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_startOfRange,
      { "startOfRange", "h225.startOfRange",
        FT_UINT32, BASE_DEC, VALS(h225_PartyNumber_vals), 0,
        "PartyNumber", HFILL }},
    { &hf_h225_endOfRange,
      { "endOfRange", "h225.endOfRange",
        FT_UINT32, BASE_DEC, VALS(h225_PartyNumber_vals), 0,
        "PartyNumber", HFILL }},
    { &hf_h225_e164Number,
      { "e164Number", "h225.e164Number_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "PublicPartyNumber", HFILL }},
    { &hf_h225_dataPartyNumber,
      { "dataPartyNumber", "h225.dataPartyNumber",
        FT_STRING, BASE_NONE, NULL, 0,
        "NumberDigits", HFILL }},
    { &hf_h225_telexPartyNumber,
      { "telexPartyNumber", "h225.telexPartyNumber",
        FT_STRING, BASE_NONE, NULL, 0,
        "NumberDigits", HFILL }},
    { &hf_h225_privateNumber,
      { "privateNumber", "h225.privateNumber_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "PrivatePartyNumber", HFILL }},
    { &hf_h225_nationalStandardPartyNumber,
      { "nationalStandardPartyNumber", "h225.nationalStandardPartyNumber",
        FT_STRING, BASE_NONE, NULL, 0,
        "NumberDigits", HFILL }},
    { &hf_h225_publicTypeOfNumber,
      { "publicTypeOfNumber", "h225.publicTypeOfNumber",
        FT_UINT32, BASE_DEC, VALS(h225_PublicTypeOfNumber_vals), 0,
        NULL, HFILL }},
    { &hf_h225_publicNumberDigits,
      { "publicNumberDigits", "h225.publicNumberDigits",
        FT_STRING, BASE_NONE, NULL, 0,
        "NumberDigits", HFILL }},
    { &hf_h225_privateTypeOfNumber,
      { "privateTypeOfNumber", "h225.privateTypeOfNumber",
        FT_UINT32, BASE_DEC, VALS(h225_PrivateTypeOfNumber_vals), 0,
        NULL, HFILL }},
    { &hf_h225_privateNumberDigits,
      { "privateNumberDigits", "h225.privateNumberDigits",
        FT_STRING, BASE_NONE, NULL, 0,
        "NumberDigits", HFILL }},
    { &hf_h225_language_01,
      { "language", "h225.language",
        FT_STRING, BASE_NONE, NULL, 0,
        "IA5String", HFILL }},
    { &hf_h225_name,
      { "name", "h225.name",
        FT_STRING, BASE_NONE, NULL, 0,
        "BMPString_SIZE_1_80", HFILL }},
    { &hf_h225_internationalNumber,
      { "internationalNumber", "h225.internationalNumber_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_nationalNumber,
      { "nationalNumber", "h225.nationalNumber_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_networkSpecificNumber,
      { "networkSpecificNumber", "h225.networkSpecificNumber_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_subscriberNumber,
      { "subscriberNumber", "h225.subscriberNumber_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_abbreviatedNumber,
      { "abbreviatedNumber", "h225.abbreviatedNumber_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_level2RegionalNumber,
      { "level2RegionalNumber", "h225.level2RegionalNumber_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_level1RegionalNumber,
      { "level1RegionalNumber", "h225.level1RegionalNumber_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_pISNSpecificNumber,
      { "pISNSpecificNumber", "h225.pISNSpecificNumber_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_localNumber,
      { "localNumber", "h225.localNumber_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_ansi_41_uim,
      { "ansi-41-uim", "h225.ansi_41_uim_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_gsm_uim,
      { "gsm-uim", "h225.gsm_uim_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_imsi,
      { "imsi", "h225.imsi",
        FT_STRING, BASE_NONE, NULL, 0,
        "TBCD_STRING_SIZE_3_16", HFILL }},
    { &hf_h225_min,
      { "min", "h225.min",
        FT_STRING, BASE_NONE, NULL, 0,
        "TBCD_STRING_SIZE_3_16", HFILL }},
    { &hf_h225_mdn,
      { "mdn", "h225.mdn",
        FT_STRING, BASE_NONE, NULL, 0,
        "TBCD_STRING_SIZE_3_16", HFILL }},
    { &hf_h225_msisdn,
      { "msisdn", "h225.msisdn",
        FT_STRING, BASE_NONE, NULL, 0,
        "TBCD_STRING_SIZE_3_16", HFILL }},
    { &hf_h225_esn,
      { "esn", "h225.esn",
        FT_STRING, BASE_NONE, NULL, 0,
        "TBCD_STRING_SIZE_16", HFILL }},
    { &hf_h225_mscid,
      { "mscid", "h225.mscid",
        FT_STRING, BASE_NONE, NULL, 0,
        "TBCD_STRING_SIZE_3_16", HFILL }},
    { &hf_h225_system_id,
      { "system-id", "h225.system_id",
        FT_UINT32, BASE_DEC, VALS(h225_T_system_id_vals), 0,
        NULL, HFILL }},
    { &hf_h225_sid,
      { "sid", "h225.sid",
        FT_STRING, BASE_NONE, NULL, 0,
        "TBCD_STRING_SIZE_1_4", HFILL }},
    { &hf_h225_mid,
      { "mid", "h225.mid",
        FT_STRING, BASE_NONE, NULL, 0,
        "TBCD_STRING_SIZE_1_4", HFILL }},
    { &hf_h225_systemMyTypeCode,
      { "systemMyTypeCode", "h225.systemMyTypeCode",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING_SIZE_1", HFILL }},
    { &hf_h225_systemAccessType,
      { "systemAccessType", "h225.systemAccessType",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING_SIZE_1", HFILL }},
    { &hf_h225_qualificationInformationCode,
      { "qualificationInformationCode", "h225.qualificationInformationCode",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING_SIZE_1", HFILL }},
    { &hf_h225_sesn,
      { "sesn", "h225.sesn",
        FT_STRING, BASE_NONE, NULL, 0,
        "TBCD_STRING_SIZE_16", HFILL }},
    { &hf_h225_soc,
      { "soc", "h225.soc",
        FT_STRING, BASE_NONE, NULL, 0,
        "TBCD_STRING_SIZE_3_16", HFILL }},
    { &hf_h225_tmsi,
      { "tmsi", "h225.tmsi",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING_SIZE_1_4", HFILL }},
    { &hf_h225_imei,
      { "imei", "h225.imei",
        FT_STRING, BASE_NONE, NULL, 0,
        "TBCD_STRING_SIZE_15_16", HFILL }},
    { &hf_h225_hplmn,
      { "hplmn", "h225.hplmn",
        FT_STRING, BASE_NONE, NULL, 0,
        "TBCD_STRING_SIZE_1_4", HFILL }},
    { &hf_h225_vplmn,
      { "vplmn", "h225.vplmn",
        FT_STRING, BASE_NONE, NULL, 0,
        "TBCD_STRING_SIZE_1_4", HFILL }},
    { &hf_h225_isupE164Number,
      { "e164Number", "h225.e164Number_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "IsupPublicPartyNumber", HFILL }},
    { &hf_h225_isupDataPartyNumber,
      { "dataPartyNumber", "h225.dataPartyNumber",
        FT_STRING, BASE_NONE, NULL, 0,
        "IsupDigits", HFILL }},
    { &hf_h225_isupTelexPartyNumber,
      { "telexPartyNumber", "h225.telexPartyNumber",
        FT_STRING, BASE_NONE, NULL, 0,
        "IsupDigits", HFILL }},
    { &hf_h225_isupPrivateNumber,
      { "privateNumber", "h225.privateNumber_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "IsupPrivatePartyNumber", HFILL }},
    { &hf_h225_isupNationalStandardPartyNumber,
      { "nationalStandardPartyNumber", "h225.nationalStandardPartyNumber",
        FT_STRING, BASE_NONE, NULL, 0,
        "IsupDigits", HFILL }},
    { &hf_h225_natureOfAddress,
      { "natureOfAddress", "h225.natureOfAddress",
        FT_UINT32, BASE_DEC, VALS(h225_NatureOfAddress_vals), 0,
        NULL, HFILL }},
    { &hf_h225_address,
      { "address", "h225.address",
        FT_STRING, BASE_NONE, NULL, 0,
        "IsupDigits", HFILL }},
    { &hf_h225_routingNumberNationalFormat,
      { "routingNumberNationalFormat", "h225.routingNumberNationalFormat_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_routingNumberNetworkSpecificFormat,
      { "routingNumberNetworkSpecificFormat", "h225.routingNumberNetworkSpecificFormat_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_routingNumberWithCalledDirectoryNumber,
      { "routingNumberWithCalledDirectoryNumber", "h225.routingNumberWithCalledDirectoryNumber_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_extAliasAddress,
      { "address", "h225.address",
        FT_UINT32, BASE_DEC, VALS(AliasAddress_vals), 0,
        "AliasAddress", HFILL }},
    { &hf_h225_aliasAddress,
      { "aliasAddress", "h225.aliasAddress",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_AliasAddress", HFILL }},
    { &hf_h225_aliasAddress_item,
      { "AliasAddress", "h225.AliasAddress",
        FT_UINT32, BASE_DEC, VALS(AliasAddress_vals), 0,
        NULL, HFILL }},
    { &hf_h225_callSignalAddress,
      { "callSignalAddress", "h225.callSignalAddress",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_TransportAddress", HFILL }},
    { &hf_h225_callSignalAddress_item,
      { "TransportAddress", "h225.TransportAddress",
        FT_UINT32, BASE_DEC, VALS(h225_TransportAddress_vals), 0,
        NULL, HFILL }},
    { &hf_h225_rasAddress,
      { "rasAddress", "h225.rasAddress",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_TransportAddress", HFILL }},
    { &hf_h225_rasAddress_item,
      { "TransportAddress", "h225.TransportAddress",
        FT_UINT32, BASE_DEC, VALS(h225_TransportAddress_vals), 0,
        NULL, HFILL }},
    { &hf_h225_endpointType,
      { "endpointType", "h225.endpointType_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_priority,
      { "priority", "h225.priority",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_127", HFILL }},
    { &hf_h225_remoteExtensionAddress,
      { "remoteExtensionAddress", "h225.remoteExtensionAddress",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_AliasAddress", HFILL }},
    { &hf_h225_remoteExtensionAddress_item,
      { "AliasAddress", "h225.AliasAddress",
        FT_UINT32, BASE_DEC, VALS(AliasAddress_vals), 0,
        NULL, HFILL }},
    { &hf_h225_alternateTransportAddresses,
      { "alternateTransportAddresses", "h225.alternateTransportAddresses_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_annexE,
      { "annexE", "h225.annexE",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_TransportAddress", HFILL }},
    { &hf_h225_annexE_item,
      { "TransportAddress", "h225.TransportAddress",
        FT_UINT32, BASE_DEC, VALS(h225_TransportAddress_vals), 0,
        NULL, HFILL }},
    { &hf_h225_sctp,
      { "sctp", "h225.sctp",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_TransportAddress", HFILL }},
    { &hf_h225_sctp_item,
      { "TransportAddress", "h225.TransportAddress",
        FT_UINT32, BASE_DEC, VALS(h225_TransportAddress_vals), 0,
        NULL, HFILL }},
    { &hf_h225_tcp,
      { "tcp", "h225.tcp_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_annexE_flg,
      { "annexE", "h225.annexE_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_sctp_flg,
      { "sctp", "h225.sctp_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_alternateGK_rasAddress,
      { "rasAddress", "h225.rasAddress",
        FT_UINT32, BASE_DEC, VALS(h225_TransportAddress_vals), 0,
        "TransportAddress", HFILL }},
    { &hf_h225_gatekeeperIdentifier,
      { "gatekeeperIdentifier", "h225.gatekeeperIdentifier",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_needToRegister,
      { "needToRegister", "h225.needToRegister",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_alternateGatekeeper,
      { "alternateGatekeeper", "h225.alternateGatekeeper",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_AlternateGK", HFILL }},
    { &hf_h225_alternateGatekeeper_item,
      { "AlternateGK", "h225.AlternateGK_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_altGKisPermanent,
      { "altGKisPermanent", "h225.altGKisPermanent",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_default,
      { "default", "h225.default_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_encryption,
      { "encryption", "h225.encryption",
        FT_UINT32, BASE_DEC, VALS(h225_SecurityServiceMode_vals), 0,
        "SecurityServiceMode", HFILL }},
    { &hf_h225_authenticaton,
      { "authenticaton", "h225.authenticaton",
        FT_UINT32, BASE_DEC, VALS(h225_SecurityServiceMode_vals), 0,
        "SecurityServiceMode", HFILL }},
    { &hf_h225_securityCapabilities_integrity,
      { "integrity", "h225.integrity",
        FT_UINT32, BASE_DEC, VALS(h225_SecurityServiceMode_vals), 0,
        "SecurityServiceMode", HFILL }},
    { &hf_h225_securityWrongSyncTime,
      { "securityWrongSyncTime", "h225.securityWrongSyncTime_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_securityReplay,
      { "securityReplay", "h225.securityReplay_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_securityWrongGeneralID,
      { "securityWrongGeneralID", "h225.securityWrongGeneralID_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_securityWrongSendersID,
      { "securityWrongSendersID", "h225.securityWrongSendersID_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_securityIntegrityFailed,
      { "securityIntegrityFailed", "h225.securityIntegrityFailed_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_securityWrongOID,
      { "securityWrongOID", "h225.securityWrongOID_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_securityDHmismatch,
      { "securityDHmismatch", "h225.securityDHmismatch_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_securityCertificateExpired,
      { "securityCertificateExpired", "h225.securityCertificateExpired_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_securityCertificateDateInvalid,
      { "securityCertificateDateInvalid", "h225.securityCertificateDateInvalid_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_securityCertificateRevoked,
      { "securityCertificateRevoked", "h225.securityCertificateRevoked_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_securityCertificateNotReadable,
      { "securityCertificateNotReadable", "h225.securityCertificateNotReadable_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_securityCertificateSignatureInvalid,
      { "securityCertificateSignatureInvalid", "h225.securityCertificateSignatureInvalid_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_securityCertificateMissing,
      { "securityCertificateMissing", "h225.securityCertificateMissing_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_securityCertificateIncomplete,
      { "securityCertificateIncomplete", "h225.securityCertificateIncomplete_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_securityUnsupportedCertificateAlgOID,
      { "securityUnsupportedCertificateAlgOID", "h225.securityUnsupportedCertificateAlgOID_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_securityUnknownCA,
      { "securityUnknownCA", "h225.securityUnknownCA_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_noSecurity,
      { "noSecurity", "h225.noSecurity_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_tls,
      { "tls", "h225.tls_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "SecurityCapabilities", HFILL }},
    { &hf_h225_ipsec,
      { "ipsec", "h225.ipsec_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "SecurityCapabilities", HFILL }},
    { &hf_h225_q932Full,
      { "q932Full", "h225.q932Full",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_q951Full,
      { "q951Full", "h225.q951Full",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_q952Full,
      { "q952Full", "h225.q952Full",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_q953Full,
      { "q953Full", "h225.q953Full",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_q955Full,
      { "q955Full", "h225.q955Full",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_q956Full,
      { "q956Full", "h225.q956Full",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_q957Full,
      { "q957Full", "h225.q957Full",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_q954Info,
      { "q954Info", "h225.q954Info_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Q954Details", HFILL }},
    { &hf_h225_conferenceCalling,
      { "conferenceCalling", "h225.conferenceCalling",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_threePartyService,
      { "threePartyService", "h225.threePartyService",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_guid,
      { "guid", "h225.guid",
        FT_GUID, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_isoAlgorithm,
      { "isoAlgorithm", "h225.isoAlgorithm",
        FT_OID, BASE_NONE, NULL, 0,
        "OBJECT_IDENTIFIER", HFILL }},
    { &hf_h225_hMAC_MD5,
      { "hMAC-MD5", "h225.hMAC_MD5_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_hMAC_iso10118_2_s,
      { "hMAC-iso10118-2-s", "h225.hMAC_iso10118_2_s",
        FT_UINT32, BASE_DEC, VALS(h225_EncryptIntAlg_vals), 0,
        "EncryptIntAlg", HFILL }},
    { &hf_h225_hMAC_iso10118_2_l,
      { "hMAC-iso10118-2-l", "h225.hMAC_iso10118_2_l",
        FT_UINT32, BASE_DEC, VALS(h225_EncryptIntAlg_vals), 0,
        "EncryptIntAlg", HFILL }},
    { &hf_h225_hMAC_iso10118_3,
      { "hMAC-iso10118-3", "h225.hMAC_iso10118_3",
        FT_OID, BASE_NONE, NULL, 0,
        "OBJECT_IDENTIFIER", HFILL }},
    { &hf_h225_digSig,
      { "digSig", "h225.digSig_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_iso9797,
      { "iso9797", "h225.iso9797",
        FT_OID, BASE_NONE, NULL, 0,
        "OBJECT_IDENTIFIER", HFILL }},
    { &hf_h225_nonIsoIM,
      { "nonIsoIM", "h225.nonIsoIM",
        FT_UINT32, BASE_DEC, VALS(h225_NonIsoIntegrityMechanism_vals), 0,
        "NonIsoIntegrityMechanism", HFILL }},
    { &hf_h225_algorithmOID,
      { "algorithmOID", "h225.algorithmOID",
        FT_OID, BASE_NONE, NULL, 0,
        "OBJECT_IDENTIFIER", HFILL }},
    { &hf_h225_icv,
      { "icv", "h225.icv",
        FT_BYTES, BASE_NONE, NULL, 0,
        "BIT_STRING", HFILL }},
    { &hf_h225_cryptoEPPwdHash,
      { "cryptoEPPwdHash", "h225.cryptoEPPwdHash_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_alias,
      { "alias", "h225.alias",
        FT_UINT32, BASE_DEC, VALS(AliasAddress_vals), 0,
        "AliasAddress", HFILL }},
    { &hf_h225_timeStamp,
      { "timeStamp", "h225.timeStamp",
        FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_token,
      { "token", "h225.token_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "HASHED", HFILL }},
    { &hf_h225_cryptoGKPwdHash,
      { "cryptoGKPwdHash", "h225.cryptoGKPwdHash_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_gatekeeperId,
      { "gatekeeperId", "h225.gatekeeperId",
        FT_STRING, BASE_NONE, NULL, 0,
        "GatekeeperIdentifier", HFILL }},
    { &hf_h225_cryptoEPPwdEncr,
      { "cryptoEPPwdEncr", "h225.cryptoEPPwdEncr_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "ENCRYPTED", HFILL }},
    { &hf_h225_cryptoGKPwdEncr,
      { "cryptoGKPwdEncr", "h225.cryptoGKPwdEncr_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "ENCRYPTED", HFILL }},
    { &hf_h225_cryptoEPCert,
      { "cryptoEPCert", "h225.cryptoEPCert_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "SIGNED", HFILL }},
    { &hf_h225_cryptoGKCert,
      { "cryptoGKCert", "h225.cryptoGKCert_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "SIGNED", HFILL }},
    { &hf_h225_cryptoFastStart,
      { "cryptoFastStart", "h225.cryptoFastStart_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "SIGNED", HFILL }},
    { &hf_h225_nestedcryptoToken,
      { "nestedcryptoToken", "h225.nestedcryptoToken",
        FT_UINT32, BASE_DEC, VALS(h235_CryptoToken_vals), 0,
        "CryptoToken", HFILL }},
    { &hf_h225_channelRate,
      { "channelRate", "h225.channelRate",
        FT_UINT32, BASE_DEC, NULL, 0,
        "BandWidth", HFILL }},
    { &hf_h225_channelMultiplier,
      { "channelMultiplier", "h225.channelMultiplier",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_256", HFILL }},
    { &hf_h225_globalCallId,
      { "globalCallId", "h225.globalCallId",
        FT_GUID, BASE_NONE, NULL, 0,
        "GloballyUniqueID", HFILL }},
    { &hf_h225_threadId,
      { "threadId", "h225.threadId",
        FT_GUID, BASE_NONE, NULL, 0,
        "GloballyUniqueID", HFILL }},
    { &hf_h225_prefix,
      { "prefix", "h225.prefix",
        FT_UINT32, BASE_DEC, VALS(AliasAddress_vals), 0,
        "AliasAddress", HFILL }},
    { &hf_h225_canReportCallCapacity,
      { "canReportCallCapacity", "h225.canReportCallCapacity",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_capacityReportingSpecification_when,
      { "when", "h225.when_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "CapacityReportingSpecification_when", HFILL }},
    { &hf_h225_callStart,
      { "callStart", "h225.callStart_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_callEnd,
      { "callEnd", "h225.callEnd_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_maximumCallCapacity,
      { "maximumCallCapacity", "h225.maximumCallCapacity_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "CallCapacityInfo", HFILL }},
    { &hf_h225_currentCallCapacity,
      { "currentCallCapacity", "h225.currentCallCapacity_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "CallCapacityInfo", HFILL }},
    { &hf_h225_voiceGwCallsAvailable,
      { "voiceGwCallsAvailable", "h225.voiceGwCallsAvailable",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_CallsAvailable", HFILL }},
    { &hf_h225_voiceGwCallsAvailable_item,
      { "CallsAvailable", "h225.CallsAvailable_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_h310GwCallsAvailable,
      { "h310GwCallsAvailable", "h225.h310GwCallsAvailable",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_CallsAvailable", HFILL }},
    { &hf_h225_h310GwCallsAvailable_item,
      { "CallsAvailable", "h225.CallsAvailable_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_h320GwCallsAvailable,
      { "h320GwCallsAvailable", "h225.h320GwCallsAvailable",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_CallsAvailable", HFILL }},
    { &hf_h225_h320GwCallsAvailable_item,
      { "CallsAvailable", "h225.CallsAvailable_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_h321GwCallsAvailable,
      { "h321GwCallsAvailable", "h225.h321GwCallsAvailable",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_CallsAvailable", HFILL }},
    { &hf_h225_h321GwCallsAvailable_item,
      { "CallsAvailable", "h225.CallsAvailable_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_h322GwCallsAvailable,
      { "h322GwCallsAvailable", "h225.h322GwCallsAvailable",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_CallsAvailable", HFILL }},
    { &hf_h225_h322GwCallsAvailable_item,
      { "CallsAvailable", "h225.CallsAvailable_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_h323GwCallsAvailable,
      { "h323GwCallsAvailable", "h225.h323GwCallsAvailable",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_CallsAvailable", HFILL }},
    { &hf_h225_h323GwCallsAvailable_item,
      { "CallsAvailable", "h225.CallsAvailable_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_h324GwCallsAvailable,
      { "h324GwCallsAvailable", "h225.h324GwCallsAvailable",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_CallsAvailable", HFILL }},
    { &hf_h225_h324GwCallsAvailable_item,
      { "CallsAvailable", "h225.CallsAvailable_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_t120OnlyGwCallsAvailable,
      { "t120OnlyGwCallsAvailable", "h225.t120OnlyGwCallsAvailable",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_CallsAvailable", HFILL }},
    { &hf_h225_t120OnlyGwCallsAvailable_item,
      { "CallsAvailable", "h225.CallsAvailable_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_t38FaxAnnexbOnlyGwCallsAvailable,
      { "t38FaxAnnexbOnlyGwCallsAvailable", "h225.t38FaxAnnexbOnlyGwCallsAvailable",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_CallsAvailable", HFILL }},
    { &hf_h225_t38FaxAnnexbOnlyGwCallsAvailable_item,
      { "CallsAvailable", "h225.CallsAvailable_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_terminalCallsAvailable,
      { "terminalCallsAvailable", "h225.terminalCallsAvailable",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_CallsAvailable", HFILL }},
    { &hf_h225_terminalCallsAvailable_item,
      { "CallsAvailable", "h225.CallsAvailable_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_mcuCallsAvailable,
      { "mcuCallsAvailable", "h225.mcuCallsAvailable",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_CallsAvailable", HFILL }},
    { &hf_h225_mcuCallsAvailable_item,
      { "CallsAvailable", "h225.CallsAvailable_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_sipGwCallsAvailable,
      { "sipGwCallsAvailable", "h225.sipGwCallsAvailable",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_CallsAvailable", HFILL }},
    { &hf_h225_sipGwCallsAvailable_item,
      { "CallsAvailable", "h225.CallsAvailable_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_calls,
      { "calls", "h225.calls",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_4294967295", HFILL }},
    { &hf_h225_group_IA5String,
      { "group", "h225.group",
        FT_STRING, BASE_NONE, NULL, 0,
        "IA5String_SIZE_1_128", HFILL }},
    { &hf_h225_carrier,
      { "carrier", "h225.carrier_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "CarrierInfo", HFILL }},
    { &hf_h225_sourceCircuitID,
      { "sourceCircuitID", "h225.sourceCircuitID_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "CircuitIdentifier", HFILL }},
    { &hf_h225_destinationCircuitID,
      { "destinationCircuitID", "h225.destinationCircuitID_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "CircuitIdentifier", HFILL }},
    { &hf_h225_cic,
      { "cic", "h225.cic_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "CicInfo", HFILL }},
    { &hf_h225_group,
      { "group", "h225.group_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "GroupID", HFILL }},
    { &hf_h225_cic_2_4,
      { "cic", "h225.cic",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_cic_2_4", HFILL }},
    { &hf_h225_cic_2_4_item,
      { "cic item", "h225.cic_item",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING_SIZE_2_4", HFILL }},
    { &hf_h225_pointCode,
      { "pointCode", "h225.pointCode",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING_SIZE_2_5", HFILL }},
    { &hf_h225_member,
      { "member", "h225.member",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_member_item,
      { "member item", "h225.member_item",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_65535", HFILL }},
    { &hf_h225_carrierIdentificationCode,
      { "carrierIdentificationCode", "h225.carrierIdentificationCode",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING_SIZE_3_4", HFILL }},
    { &hf_h225_carrierName,
      { "carrierName", "h225.carrierName",
        FT_STRING, BASE_NONE, NULL, 0,
        "IA5String_SIZE_1_128", HFILL }},
    { &hf_h225_url,
      { "url", "h225.url",
        FT_STRING, BASE_NONE, NULL, 0,
        "IA5String_SIZE_0_512", HFILL }},
    { &hf_h225_signal,
      { "signal", "h225.signal",
        FT_BYTES, BASE_NONE, NULL, 0,
        "H248SignalsDescriptor", HFILL }},
    { &hf_h225_callCreditServiceControl,
      { "callCreditServiceControl", "h225.callCreditServiceControl_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_sessionId_0_255,
      { "sessionId", "h225.sessionId",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_255", HFILL }},
    { &hf_h225_contents,
      { "contents", "h225.contents",
        FT_UINT32, BASE_DEC, VALS(h225_ServiceControlDescriptor_vals), 0,
        "ServiceControlDescriptor", HFILL }},
    { &hf_h225_reason,
      { "reason", "h225.reason",
        FT_UINT32, BASE_DEC, VALS(h225_ServiceControlSession_reason_vals), 0,
        "ServiceControlSession_reason", HFILL }},
    { &hf_h225_open,
      { "open", "h225.open_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_refresh,
      { "refresh", "h225.refresh_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_close,
      { "close", "h225.close_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_nonStandardUsageTypes,
      { "nonStandardUsageTypes", "h225.nonStandardUsageTypes",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_NonStandardParameter", HFILL }},
    { &hf_h225_nonStandardUsageTypes_item,
      { "NonStandardParameter", "h225.NonStandardParameter_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_startTime,
      { "startTime", "h225.startTime_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_endTime_flg,
      { "endTime", "h225.endTime_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_terminationCause_flg,
      { "terminationCause", "h225.terminationCause_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_when,
      { "when", "h225.when_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "RasUsageSpecification_when", HFILL }},
    { &hf_h225_start,
      { "start", "h225.start_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_end,
      { "end", "h225.end_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_inIrr,
      { "inIrr", "h225.inIrr_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_ras_callStartingPoint,
      { "callStartingPoint", "h225.callStartingPoint_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "RasUsageSpecificationcallStartingPoint", HFILL }},
    { &hf_h225_alerting_flg,
      { "alerting", "h225.alerting_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_connect_flg,
      { "connect", "h225.connect_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_required,
      { "required", "h225.required_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "RasUsageInfoTypes", HFILL }},
    { &hf_h225_nonStandardUsageFields,
      { "nonStandardUsageFields", "h225.nonStandardUsageFields",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_NonStandardParameter", HFILL }},
    { &hf_h225_nonStandardUsageFields_item,
      { "NonStandardParameter", "h225.NonStandardParameter_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_alertingTime,
      { "alertingTime", "h225.alertingTime",
        FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL, NULL, 0,
        "TimeStamp", HFILL }},
    { &hf_h225_connectTime,
      { "connectTime", "h225.connectTime",
        FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL, NULL, 0,
        "TimeStamp", HFILL }},
    { &hf_h225_endTime,
      { "endTime", "h225.endTime",
        FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL, NULL, 0,
        "TimeStamp", HFILL }},
    { &hf_h225_releaseCompleteCauseIE,
      { "releaseCompleteCauseIE", "h225.releaseCompleteCauseIE",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING_SIZE_2_32", HFILL }},
    { &hf_h225_sender,
      { "sender", "h225.sender",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_multicast,
      { "multicast", "h225.multicast",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_bandwidth,
      { "bandwidth", "h225.bandwidth",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_rtcpAddresses,
      { "rtcpAddresses", "h225.rtcpAddresses_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TransportChannelInfo", HFILL }},
    { &hf_h225_canDisplayAmountString,
      { "canDisplayAmountString", "h225.canDisplayAmountString",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_canEnforceDurationLimit,
      { "canEnforceDurationLimit", "h225.canEnforceDurationLimit",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_amountString,
      { "amountString", "h225.amountString",
        FT_STRING, BASE_NONE, NULL, 0,
        "BMPString_SIZE_1_512", HFILL }},
    { &hf_h225_billingMode,
      { "billingMode", "h225.billingMode",
        FT_UINT32, BASE_DEC, VALS(h225_T_billingMode_vals), 0,
        NULL, HFILL }},
    { &hf_h225_credit,
      { "credit", "h225.credit_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_debit,
      { "debit", "h225.debit_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_callDurationLimit,
      { "callDurationLimit", "h225.callDurationLimit",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_4294967295", HFILL }},
    { &hf_h225_enforceCallDurationLimit,
      { "enforceCallDurationLimit", "h225.enforceCallDurationLimit",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_callStartingPoint,
      { "callStartingPoint", "h225.callStartingPoint",
        FT_UINT32, BASE_DEC, VALS(h225_CallCreditServiceControl_callStartingPoint_vals), 0,
        "CallCreditServiceControl_callStartingPoint", HFILL }},
    { &hf_h225_id,
      { "id", "h225.id",
        FT_UINT32, BASE_DEC, VALS(h225_GenericIdentifier_vals), 0,
        "GenericIdentifier", HFILL }},
    { &hf_h225_parameters,
      { "parameters", "h225.parameters",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_SIZE_1_512_OF_EnumeratedParameter", HFILL }},
    { &hf_h225_parameters_item,
      { "EnumeratedParameter", "h225.EnumeratedParameter_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_standard,
      { "standard", "h225.standard",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_oid,
      { "oid", "h225.oid",
        FT_OID, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_genericIdentifier_nonStandard,
      { "nonStandard", "h225.nonStandard",
        FT_GUID, BASE_NONE, NULL, 0,
        "GloballyUniqueID", HFILL }},
    { &hf_h225_content,
      { "content", "h225.content",
        FT_UINT32, BASE_DEC, VALS(h225_Content_vals), 0,
        NULL, HFILL }},
    { &hf_h225_raw,
      { "raw", "h225.raw",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_text,
      { "text", "h225.text",
        FT_STRING, BASE_NONE, NULL, 0,
        "IA5String", HFILL }},
    { &hf_h225_unicode,
      { "unicode", "h225.unicode",
        FT_STRING, BASE_NONE, NULL, 0,
        "BMPString", HFILL }},
    { &hf_h225_bool,
      { "bool", "h225.bool",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_number8,
      { "number8", "h225.number8",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_255", HFILL }},
    { &hf_h225_number16,
      { "number16", "h225.number16",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_65535", HFILL }},
    { &hf_h225_number32,
      { "number32", "h225.number32",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_4294967295", HFILL }},
    { &hf_h225_transport,
      { "transport", "h225.transport",
        FT_UINT32, BASE_DEC, VALS(h225_TransportAddress_vals), 0,
        "TransportAddress", HFILL }},
    { &hf_h225_compound,
      { "compound", "h225.compound",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_SIZE_1_512_OF_EnumeratedParameter", HFILL }},
    { &hf_h225_compound_item,
      { "EnumeratedParameter", "h225.EnumeratedParameter_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_nested,
      { "nested", "h225.nested",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_SIZE_1_16_OF_GenericData", HFILL }},
    { &hf_h225_nested_item,
      { "GenericData", "h225.GenericData_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_replacementFeatureSet,
      { "replacementFeatureSet", "h225.replacementFeatureSet",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_sendAddress,
      { "sendAddress", "h225.sendAddress",
        FT_UINT32, BASE_DEC, VALS(h225_TransportAddress_vals), 0,
        "TransportAddress", HFILL }},
    { &hf_h225_recvAddress,
      { "recvAddress", "h225.recvAddress",
        FT_UINT32, BASE_DEC, VALS(h225_TransportAddress_vals), 0,
        "TransportAddress", HFILL }},
    { &hf_h225_rtpAddress,
      { "rtpAddress", "h225.rtpAddress_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TransportChannelInfo", HFILL }},
    { &hf_h225_rtcpAddress,
      { "rtcpAddress", "h225.rtcpAddress_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TransportChannelInfo", HFILL }},
    { &hf_h225_cname,
      { "cname", "h225.cname",
        FT_STRING, BASE_NONE, NULL, 0,
        "PrintableString", HFILL }},
    { &hf_h225_ssrc,
      { "ssrc", "h225.ssrc",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_4294967295", HFILL }},
    { &hf_h225_sessionId,
      { "sessionId", "h225.sessionId",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_255", HFILL }},
    { &hf_h225_associatedSessionIds,
      { "associatedSessionIds", "h225.associatedSessionIds",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_associatedSessionIds_item,
      { "associatedSessionIds item", "h225.associatedSessionIds_item",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_255", HFILL }},
    { &hf_h225_multicast_flg,
      { "multicast", "h225.multicast_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_gatekeeperBased,
      { "gatekeeperBased", "h225.gatekeeperBased_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_endpointBased,
      { "endpointBased", "h225.endpointBased_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_gatekeeperRequest,
      { "gatekeeperRequest", "h225.gatekeeperRequest_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_gatekeeperConfirm,
      { "gatekeeperConfirm", "h225.gatekeeperConfirm_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_gatekeeperReject,
      { "gatekeeperReject", "h225.gatekeeperReject_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_registrationRequest,
      { "registrationRequest", "h225.registrationRequest_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_registrationConfirm,
      { "registrationConfirm", "h225.registrationConfirm_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_registrationReject,
      { "registrationReject", "h225.registrationReject_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_unregistrationRequest,
      { "unregistrationRequest", "h225.unregistrationRequest_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_unregistrationConfirm,
      { "unregistrationConfirm", "h225.unregistrationConfirm_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_unregistrationReject,
      { "unregistrationReject", "h225.unregistrationReject_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_admissionRequest,
      { "admissionRequest", "h225.admissionRequest_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_admissionConfirm,
      { "admissionConfirm", "h225.admissionConfirm_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_admissionReject,
      { "admissionReject", "h225.admissionReject_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_bandwidthRequest,
      { "bandwidthRequest", "h225.bandwidthRequest_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_bandwidthConfirm,
      { "bandwidthConfirm", "h225.bandwidthConfirm_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_bandwidthReject,
      { "bandwidthReject", "h225.bandwidthReject_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_disengageRequest,
      { "disengageRequest", "h225.disengageRequest_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_disengageConfirm,
      { "disengageConfirm", "h225.disengageConfirm_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_disengageReject,
      { "disengageReject", "h225.disengageReject_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_locationRequest,
      { "locationRequest", "h225.locationRequest_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_locationConfirm,
      { "locationConfirm", "h225.locationConfirm_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_locationReject,
      { "locationReject", "h225.locationReject_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_infoRequest,
      { "infoRequest", "h225.infoRequest_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_infoRequestResponse,
      { "infoRequestResponse", "h225.infoRequestResponse_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_nonStandardMessage,
      { "nonStandardMessage", "h225.nonStandardMessage_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_unknownMessageResponse,
      { "unknownMessageResponse", "h225.unknownMessageResponse_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_requestInProgress,
      { "requestInProgress", "h225.requestInProgress_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_resourcesAvailableIndicate,
      { "resourcesAvailableIndicate", "h225.resourcesAvailableIndicate_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_resourcesAvailableConfirm,
      { "resourcesAvailableConfirm", "h225.resourcesAvailableConfirm_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_infoRequestAck,
      { "infoRequestAck", "h225.infoRequestAck_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_infoRequestNak,
      { "infoRequestNak", "h225.infoRequestNak_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_serviceControlIndication,
      { "serviceControlIndication", "h225.serviceControlIndication_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_serviceControlResponse,
      { "serviceControlResponse", "h225.serviceControlResponse_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_admissionConfirmSequence,
      { "admissionConfirmSequence", "h225.admissionConfirmSequence",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_AdmissionConfirm", HFILL }},
    { &hf_h225_admissionConfirmSequence_item,
      { "AdmissionConfirm", "h225.AdmissionConfirm_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_requestSeqNum,
      { "requestSeqNum", "h225.requestSeqNum",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_gatekeeperRequest_rasAddress,
      { "rasAddress", "h225.rasAddress",
        FT_UINT32, BASE_DEC, VALS(h225_TransportAddress_vals), 0,
        "TransportAddress", HFILL }},
    { &hf_h225_endpointAlias,
      { "endpointAlias", "h225.endpointAlias",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_AliasAddress", HFILL }},
    { &hf_h225_endpointAlias_item,
      { "AliasAddress", "h225.AliasAddress",
        FT_UINT32, BASE_DEC, VALS(AliasAddress_vals), 0,
        NULL, HFILL }},
    { &hf_h225_alternateEndpoints,
      { "alternateEndpoints", "h225.alternateEndpoints",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_Endpoint", HFILL }},
    { &hf_h225_alternateEndpoints_item,
      { "Endpoint", "h225.Endpoint_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_authenticationCapability,
      { "authenticationCapability", "h225.authenticationCapability",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_AuthenticationMechanism", HFILL }},
    { &hf_h225_authenticationCapability_item,
      { "AuthenticationMechanism", "h225.AuthenticationMechanism",
        FT_UINT32, BASE_DEC, VALS(h235_AuthenticationMechanism_vals), 0,
        NULL, HFILL }},
    { &hf_h225_algorithmOIDs,
      { "algorithmOIDs", "h225.algorithmOIDs",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_algorithmOIDs_item,
      { "algorithmOIDs item", "h225.algorithmOIDs_item",
        FT_OID, BASE_NONE, NULL, 0,
        "OBJECT_IDENTIFIER", HFILL }},
    { &hf_h225_integrity,
      { "integrity", "h225.integrity",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_IntegrityMechanism", HFILL }},
    { &hf_h225_integrity_item,
      { "IntegrityMechanism", "h225.IntegrityMechanism",
        FT_UINT32, BASE_DEC, VALS(h225_IntegrityMechanism_vals), 0,
        NULL, HFILL }},
    { &hf_h225_integrityCheckValue,
      { "integrityCheckValue", "h225.integrityCheckValue_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "ICV", HFILL }},
    { &hf_h225_supportsAltGK,
      { "supportsAltGK", "h225.supportsAltGK_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_supportsAssignedGK,
      { "supportsAssignedGK", "h225.supportsAssignedGK",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_assignedGatekeeper,
      { "assignedGatekeeper", "h225.assignedGatekeeper_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "AlternateGK", HFILL }},
    { &hf_h225_gatekeeperConfirm_rasAddress,
      { "rasAddress", "h225.rasAddress",
        FT_UINT32, BASE_DEC, VALS(h225_TransportAddress_vals), 0,
        "TransportAddress", HFILL }},
    { &hf_h225_authenticationMode,
      { "authenticationMode", "h225.authenticationMode",
        FT_UINT32, BASE_DEC, VALS(h235_AuthenticationMechanism_vals), 0,
        "AuthenticationMechanism", HFILL }},
    { &hf_h225_rehomingModel,
      { "rehomingModel", "h225.rehomingModel",
        FT_UINT32, BASE_DEC, VALS(h225_RehomingModel_vals), 0,
        NULL, HFILL }},
    { &hf_h225_gatekeeperRejectReason,
      { "rejectReason", "h225.rejectReason",
        FT_UINT32, BASE_DEC, VALS(GatekeeperRejectReason_vals), 0,
        "GatekeeperRejectReason", HFILL }},
    { &hf_h225_altGKInfo,
      { "altGKInfo", "h225.altGKInfo_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_resourceUnavailable,
      { "resourceUnavailable", "h225.resourceUnavailable_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_terminalExcluded,
      { "terminalExcluded", "h225.terminalExcluded_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_securityDenial,
      { "securityDenial", "h225.securityDenial_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_gkRej_securityError,
      { "securityError", "h225.securityError",
        FT_UINT32, BASE_DEC, VALS(h225_SecurityErrors_vals), 0,
        "SecurityErrors", HFILL }},
    { &hf_h225_discoveryComplete,
      { "discoveryComplete", "h225.discoveryComplete",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_terminalType,
      { "terminalType", "h225.terminalType_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "EndpointType", HFILL }},
    { &hf_h225_terminalAlias,
      { "terminalAlias", "h225.terminalAlias",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_AliasAddress", HFILL }},
    { &hf_h225_terminalAlias_item,
      { "AliasAddress", "h225.AliasAddress",
        FT_UINT32, BASE_DEC, VALS(AliasAddress_vals), 0,
        NULL, HFILL }},
    { &hf_h225_endpointVendor,
      { "endpointVendor", "h225.endpointVendor_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "VendorIdentifier", HFILL }},
    { &hf_h225_timeToLive,
      { "timeToLive", "h225.timeToLive",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_keepAlive,
      { "keepAlive", "h225.keepAlive",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_willSupplyUUIEs,
      { "willSupplyUUIEs", "h225.willSupplyUUIEs",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_additiveRegistration,
      { "additiveRegistration", "h225.additiveRegistration_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_terminalAliasPattern,
      { "terminalAliasPattern", "h225.terminalAliasPattern",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_AddressPattern", HFILL }},
    { &hf_h225_terminalAliasPattern_item,
      { "AddressPattern", "h225.AddressPattern",
        FT_UINT32, BASE_DEC, VALS(h225_AddressPattern_vals), 0,
        NULL, HFILL }},
    { &hf_h225_usageReportingCapability,
      { "usageReportingCapability", "h225.usageReportingCapability_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "RasUsageInfoTypes", HFILL }},
    { &hf_h225_supportedH248Packages,
      { "supportedH248Packages", "h225.supportedH248Packages",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_H248PackagesDescriptor", HFILL }},
    { &hf_h225_supportedH248Packages_item,
      { "H248PackagesDescriptor", "h225.H248PackagesDescriptor",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_callCreditCapability,
      { "callCreditCapability", "h225.callCreditCapability_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_capacityReportingCapability,
      { "capacityReportingCapability", "h225.capacityReportingCapability_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_restart,
      { "restart", "h225.restart_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_supportsACFSequences,
      { "supportsACFSequences", "h225.supportsACFSequences_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_transportQOS,
      { "transportQOS", "h225.transportQOS",
        FT_UINT32, BASE_DEC, VALS(h225_TransportQOS_vals), 0,
        NULL, HFILL }},
    { &hf_h225_willRespondToIRR,
      { "willRespondToIRR", "h225.willRespondToIRR",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_preGrantedARQ,
      { "preGrantedARQ", "h225.preGrantedARQ_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_makeCall,
      { "makeCall", "h225.makeCall",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_useGKCallSignalAddressToMakeCall,
      { "useGKCallSignalAddressToMakeCall", "h225.useGKCallSignalAddressToMakeCall",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_answerCall,
      { "answerCall", "h225.answerCall",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_useGKCallSignalAddressToAnswer,
      { "useGKCallSignalAddressToAnswer", "h225.useGKCallSignalAddressToAnswer",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_irrFrequencyInCall,
      { "irrFrequencyInCall", "h225.irrFrequencyInCall",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_65535", HFILL }},
    { &hf_h225_totalBandwidthRestriction,
      { "totalBandwidthRestriction", "h225.totalBandwidthRestriction",
        FT_UINT32, BASE_DEC, NULL, 0,
        "BandWidth", HFILL }},
    { &hf_h225_useSpecifiedTransport,
      { "useSpecifiedTransport", "h225.useSpecifiedTransport",
        FT_UINT32, BASE_DEC, VALS(h225_UseSpecifiedTransport_vals), 0,
        NULL, HFILL }},
    { &hf_h225_supportsAdditiveRegistration,
      { "supportsAdditiveRegistration", "h225.supportsAdditiveRegistration_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_usageSpec,
      { "usageSpec", "h225.usageSpec",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_RasUsageSpecification", HFILL }},
    { &hf_h225_usageSpec_item,
      { "RasUsageSpecification", "h225.RasUsageSpecification_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_featureServerAlias,
      { "featureServerAlias", "h225.featureServerAlias",
        FT_UINT32, BASE_DEC, VALS(AliasAddress_vals), 0,
        "AliasAddress", HFILL }},
    { &hf_h225_capacityReportingSpec,
      { "capacityReportingSpec", "h225.capacityReportingSpec_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "CapacityReportingSpecification", HFILL }},
    { &hf_h225_registrationRejectReason,
      { "rejectReason", "h225.rejectReason",
        FT_UINT32, BASE_DEC, VALS(RegistrationRejectReason_vals), 0,
        "RegistrationRejectReason", HFILL }},
    { &hf_h225_discoveryRequired,
      { "discoveryRequired", "h225.discoveryRequired_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_invalidCallSignalAddress,
      { "invalidCallSignalAddress", "h225.invalidCallSignalAddress_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_invalidRASAddress,
      { "invalidRASAddress", "h225.invalidRASAddress_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_duplicateAlias,
      { "duplicateAlias", "h225.duplicateAlias",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_AliasAddress", HFILL }},
    { &hf_h225_duplicateAlias_item,
      { "AliasAddress", "h225.AliasAddress",
        FT_UINT32, BASE_DEC, VALS(AliasAddress_vals), 0,
        NULL, HFILL }},
    { &hf_h225_invalidTerminalType,
      { "invalidTerminalType", "h225.invalidTerminalType_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_transportNotSupported,
      { "transportNotSupported", "h225.transportNotSupported_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_transportQOSNotSupported,
      { "transportQOSNotSupported", "h225.transportQOSNotSupported_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_invalidAlias,
      { "invalidAlias", "h225.invalidAlias_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_fullRegistrationRequired,
      { "fullRegistrationRequired", "h225.fullRegistrationRequired_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_additiveRegistrationNotSupported,
      { "additiveRegistrationNotSupported", "h225.additiveRegistrationNotSupported_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_invalidTerminalAliases,
      { "invalidTerminalAliases", "h225.invalidTerminalAliases_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_reg_securityError,
      { "securityError", "h225.securityError",
        FT_UINT32, BASE_DEC, VALS(h225_SecurityErrors_vals), 0,
        "SecurityErrors", HFILL }},
    { &hf_h225_registerWithAssignedGK,
      { "registerWithAssignedGK", "h225.registerWithAssignedGK_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_unregRequestReason,
      { "reason", "h225.reason",
        FT_UINT32, BASE_DEC, VALS(UnregRequestReason_vals), 0,
        "UnregRequestReason", HFILL }},
    { &hf_h225_endpointAliasPattern,
      { "endpointAliasPattern", "h225.endpointAliasPattern",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_AddressPattern", HFILL }},
    { &hf_h225_endpointAliasPattern_item,
      { "AddressPattern", "h225.AddressPattern",
        FT_UINT32, BASE_DEC, VALS(h225_AddressPattern_vals), 0,
        NULL, HFILL }},
    { &hf_h225_reregistrationRequired,
      { "reregistrationRequired", "h225.reregistrationRequired_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_ttlExpired,
      { "ttlExpired", "h225.ttlExpired_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_maintenance,
      { "maintenance", "h225.maintenance_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_securityError,
      { "securityError", "h225.securityError",
        FT_UINT32, BASE_DEC, VALS(h225_SecurityErrors2_vals), 0,
        "SecurityErrors2", HFILL }},
    { &hf_h225_unregRejectReason,
      { "rejectReason", "h225.rejectReason",
        FT_UINT32, BASE_DEC, VALS(UnregRejectReason_vals), 0,
        "UnregRejectReason", HFILL }},
    { &hf_h225_notCurrentlyRegistered,
      { "notCurrentlyRegistered", "h225.notCurrentlyRegistered_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_callInProgress,
      { "callInProgress", "h225.callInProgress_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_permissionDenied,
      { "permissionDenied", "h225.permissionDenied_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_callModel,
      { "callModel", "h225.callModel",
        FT_UINT32, BASE_DEC, VALS(h225_CallModel_vals), 0,
        NULL, HFILL }},
    { &hf_h225_DestinationInfo_item,
      { "DestinationInfo item", "h225.DestinationInfo_item",
        FT_UINT32, BASE_DEC, VALS(AliasAddress_vals), 0,
        NULL, HFILL }},
    { &hf_h225_destinationInfo_01,
      { "destinationInfo", "h225.destinationInfo",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_srcInfo,
      { "srcInfo", "h225.srcInfo",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_AliasAddress", HFILL }},
    { &hf_h225_srcInfo_item,
      { "AliasAddress", "h225.AliasAddress",
        FT_UINT32, BASE_DEC, VALS(AliasAddress_vals), 0,
        NULL, HFILL }},
    { &hf_h225_srcCallSignalAddress,
      { "srcCallSignalAddress", "h225.srcCallSignalAddress",
        FT_UINT32, BASE_DEC, VALS(h225_TransportAddress_vals), 0,
        "TransportAddress", HFILL }},
    { &hf_h225_bandWidth,
      { "bandWidth", "h225.bandWidth",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_callReferenceValue,
      { "callReferenceValue", "h225.callReferenceValue",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_canMapAlias,
      { "canMapAlias", "h225.canMapAlias",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_srcAlternatives,
      { "srcAlternatives", "h225.srcAlternatives",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_Endpoint", HFILL }},
    { &hf_h225_srcAlternatives_item,
      { "Endpoint", "h225.Endpoint_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_destAlternatives,
      { "destAlternatives", "h225.destAlternatives",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_Endpoint", HFILL }},
    { &hf_h225_destAlternatives_item,
      { "Endpoint", "h225.Endpoint_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_gatewayDataRate,
      { "gatewayDataRate", "h225.gatewayDataRate_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "DataRate", HFILL }},
    { &hf_h225_desiredTunnelledProtocol,
      { "desiredTunnelledProtocol", "h225.desiredTunnelledProtocol_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TunnelledProtocol", HFILL }},
    { &hf_h225_canMapSrcAlias,
      { "canMapSrcAlias", "h225.canMapSrcAlias",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_pointToPoint,
      { "pointToPoint", "h225.pointToPoint_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_oneToN,
      { "oneToN", "h225.oneToN_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_nToOne,
      { "nToOne", "h225.nToOne_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_nToN,
      { "nToN", "h225.nToN_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_direct,
      { "direct", "h225.direct_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_gatekeeperRouted,
      { "gatekeeperRouted", "h225.gatekeeperRouted_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_endpointControlled,
      { "endpointControlled", "h225.endpointControlled_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_gatekeeperControlled,
      { "gatekeeperControlled", "h225.gatekeeperControlled_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_noControl,
      { "noControl", "h225.noControl_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_qOSCapabilities,
      { "qOSCapabilities", "h225.qOSCapabilities",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_SIZE_1_256_OF_QOSCapability", HFILL }},
    { &hf_h225_qOSCapabilities_item,
      { "QOSCapability", "h225.QOSCapability_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_irrFrequency,
      { "irrFrequency", "h225.irrFrequency",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_65535", HFILL }},
    { &hf_h225_destinationType,
      { "destinationType", "h225.destinationType_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "EndpointType", HFILL }},
    { &hf_h225_uuiesRequested,
      { "uuiesRequested", "h225.uuiesRequested_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_supportedProtocols,
      { "supportedProtocols", "h225.supportedProtocols",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_SupportedProtocols", HFILL }},
    { &hf_h225_supportedProtocols_item,
      { "SupportedProtocols", "h225.SupportedProtocols",
        FT_UINT32, BASE_DEC, VALS(h225_SupportedProtocols_vals), 0,
        NULL, HFILL }},
    { &hf_h225_modifiedSrcInfo,
      { "modifiedSrcInfo", "h225.modifiedSrcInfo",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_AliasAddress", HFILL }},
    { &hf_h225_modifiedSrcInfo_item,
      { "AliasAddress", "h225.AliasAddress",
        FT_UINT32, BASE_DEC, VALS(AliasAddress_vals), 0,
        NULL, HFILL }},
    { &hf_h225_setup_bool,
      { "setup", "h225.setup",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_callProceeding_flg,
      { "callProceeding", "h225.callProceeding",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_connect_bool,
      { "connect", "h225.connect",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_alerting_bool,
      { "alerting", "h225.alerting",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_information_bool,
      { "information", "h225.information",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_releaseComplete_bool,
      { "releaseComplete", "h225.releaseComplete",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_facility_bool,
      { "facility", "h225.facility",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_progress_bool,
      { "progress", "h225.progress",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_empty,
      { "empty", "h225.empty",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_status_bool,
      { "status", "h225.status",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_statusInquiry_bool,
      { "statusInquiry", "h225.statusInquiry",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_setupAcknowledge_bool,
      { "setupAcknowledge", "h225.setupAcknowledge",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_notify_bool,
      { "notify", "h225.notify",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_rejectReason,
      { "rejectReason", "h225.rejectReason",
        FT_UINT32, BASE_DEC, VALS(AdmissionRejectReason_vals), 0,
        "AdmissionRejectReason", HFILL }},
    { &hf_h225_invalidPermission,
      { "invalidPermission", "h225.invalidPermission_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_requestDenied,
      { "requestDenied", "h225.requestDenied_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_invalidEndpointIdentifier,
      { "invalidEndpointIdentifier", "h225.invalidEndpointIdentifier_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_qosControlNotSupported,
      { "qosControlNotSupported", "h225.qosControlNotSupported_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_incompleteAddress,
      { "incompleteAddress", "h225.incompleteAddress_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_aliasesInconsistent,
      { "aliasesInconsistent", "h225.aliasesInconsistent_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_routeCallToSCN,
      { "routeCallToSCN", "h225.routeCallToSCN",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_PartyNumber", HFILL }},
    { &hf_h225_routeCallToSCN_item,
      { "PartyNumber", "h225.PartyNumber",
        FT_UINT32, BASE_DEC, VALS(h225_PartyNumber_vals), 0,
        NULL, HFILL }},
    { &hf_h225_exceedsCallCapacity,
      { "exceedsCallCapacity", "h225.exceedsCallCapacity_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_collectDestination,
      { "collectDestination", "h225.collectDestination_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_collectPIN,
      { "collectPIN", "h225.collectPIN_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_noRouteToDestination,
      { "noRouteToDestination", "h225.noRouteToDestination_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_unallocatedNumber,
      { "unallocatedNumber", "h225.unallocatedNumber_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_answeredCall,
      { "answeredCall", "h225.answeredCall",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_usageInformation,
      { "usageInformation", "h225.usageInformation_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "RasUsageInformation", HFILL }},
    { &hf_h225_bandwidthDetails,
      { "bandwidthDetails", "h225.bandwidthDetails",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_BandwidthDetails", HFILL }},
    { &hf_h225_bandwidthDetails_item,
      { "BandwidthDetails", "h225.BandwidthDetails_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_bandRejectReason,
      { "rejectReason", "h225.rejectReason",
        FT_UINT32, BASE_DEC, VALS(BandRejectReason_vals), 0,
        "BandRejectReason", HFILL }},
    { &hf_h225_allowedBandWidth,
      { "allowedBandWidth", "h225.allowedBandWidth",
        FT_UINT32, BASE_DEC, NULL, 0,
        "BandWidth", HFILL }},
    { &hf_h225_notBound,
      { "notBound", "h225.notBound_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_invalidConferenceID,
      { "invalidConferenceID", "h225.invalidConferenceID_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_insufficientResources,
      { "insufficientResources", "h225.insufficientResources_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_replyAddress,
      { "replyAddress", "h225.replyAddress",
        FT_UINT32, BASE_DEC, VALS(h225_TransportAddress_vals), 0,
        "TransportAddress", HFILL }},
    { &hf_h225_sourceInfo,
      { "sourceInfo", "h225.sourceInfo",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_AliasAddress", HFILL }},
    { &hf_h225_sourceInfo_item,
      { "AliasAddress", "h225.AliasAddress",
        FT_UINT32, BASE_DEC, VALS(AliasAddress_vals), 0,
        NULL, HFILL }},
    { &hf_h225_hopCount,
      { "hopCount", "h225.hopCount",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_255", HFILL }},
    { &hf_h225_sourceEndpointInfo,
      { "sourceEndpointInfo", "h225.sourceEndpointInfo",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_AliasAddress", HFILL }},
    { &hf_h225_sourceEndpointInfo_item,
      { "AliasAddress", "h225.AliasAddress",
        FT_UINT32, BASE_DEC, VALS(AliasAddress_vals), 0,
        NULL, HFILL }},
    { &hf_h225_locationConfirm_callSignalAddress,
      { "callSignalAddress", "h225.callSignalAddress",
        FT_UINT32, BASE_DEC, VALS(h225_TransportAddress_vals), 0,
        "TransportAddress", HFILL }},
    { &hf_h225_locationConfirm_rasAddress,
      { "rasAddress", "h225.rasAddress",
        FT_UINT32, BASE_DEC, VALS(h225_TransportAddress_vals), 0,
        "TransportAddress", HFILL }},
    { &hf_h225_locationRejectReason,
      { "rejectReason", "h225.rejectReason",
        FT_UINT32, BASE_DEC, VALS(LocationRejectReason_vals), 0,
        "LocationRejectReason", HFILL }},
    { &hf_h225_notRegistered,
      { "notRegistered", "h225.notRegistered_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_routeCalltoSCN,
      { "routeCalltoSCN", "h225.routeCalltoSCN",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_PartyNumber", HFILL }},
    { &hf_h225_routeCalltoSCN_item,
      { "PartyNumber", "h225.PartyNumber",
        FT_UINT32, BASE_DEC, VALS(h225_PartyNumber_vals), 0,
        NULL, HFILL }},
    { &hf_h225_disengageReason,
      { "disengageReason", "h225.disengageReason",
        FT_UINT32, BASE_DEC, VALS(DisengageReason_vals), 0,
        NULL, HFILL }},
    { &hf_h225_terminationCause,
      { "terminationCause", "h225.terminationCause",
        FT_UINT32, BASE_DEC, VALS(h225_CallTerminationCause_vals), 0,
        "CallTerminationCause", HFILL }},
    { &hf_h225_forcedDrop,
      { "forcedDrop", "h225.forcedDrop_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_normalDrop,
      { "normalDrop", "h225.normalDrop_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_disengageRejectReason,
      { "rejectReason", "h225.rejectReason",
        FT_UINT32, BASE_DEC, VALS(DisengageRejectReason_vals), 0,
        "DisengageRejectReason", HFILL }},
    { &hf_h225_requestToDropOther,
      { "requestToDropOther", "h225.requestToDropOther_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_usageInfoRequested,
      { "usageInfoRequested", "h225.usageInfoRequested_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "RasUsageInfoTypes", HFILL }},
    { &hf_h225_segmentedResponseSupported,
      { "segmentedResponseSupported", "h225.segmentedResponseSupported_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_nextSegmentRequested,
      { "nextSegmentRequested", "h225.nextSegmentRequested",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_65535", HFILL }},
    { &hf_h225_capacityInfoRequested,
      { "capacityInfoRequested", "h225.capacityInfoRequested_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_infoRequestResponse_rasAddress,
      { "rasAddress", "h225.rasAddress",
        FT_UINT32, BASE_DEC, VALS(h225_TransportAddress_vals), 0,
        "TransportAddress", HFILL }},
    { &hf_h225_perCallInfo,
      { "perCallInfo", "h225.perCallInfo",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_perCallInfo_item,
      { "perCallInfo item", "h225.perCallInfo_item_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_originator,
      { "originator", "h225.originator",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_audio,
      { "audio", "h225.audio",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_RTPSession", HFILL }},
    { &hf_h225_audio_item,
      { "RTPSession", "h225.RTPSession_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_video,
      { "video", "h225.video",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_RTPSession", HFILL }},
    { &hf_h225_video_item,
      { "RTPSession", "h225.RTPSession_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_data,
      { "data", "h225.data",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_TransportChannelInfo", HFILL }},
    { &hf_h225_data_item,
      { "TransportChannelInfo", "h225.TransportChannelInfo_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_h245,
      { "h245", "h225.h245_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TransportChannelInfo", HFILL }},
    { &hf_h225_callSignalling,
      { "callSignalling", "h225.callSignalling_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TransportChannelInfo", HFILL }},
    { &hf_h225_substituteConfIDs,
      { "substituteConfIDs", "h225.substituteConfIDs",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_ConferenceIdentifier", HFILL }},
    { &hf_h225_substituteConfIDs_item,
      { "ConferenceIdentifier", "h225.ConferenceIdentifier",
        FT_GUID, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_pdu,
      { "pdu", "h225.pdu",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_pdu_item,
      { "pdu item", "h225.pdu_item_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_h323pdu,
      { "h323pdu", "h225.h323pdu_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "H323_UU_PDU", HFILL }},
    { &hf_h225_sent,
      { "sent", "h225.sent",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_needResponse,
      { "needResponse", "h225.needResponse",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_irrStatus,
      { "irrStatus", "h225.irrStatus",
        FT_UINT32, BASE_DEC, VALS(h225_InfoRequestResponseStatus_vals), 0,
        "InfoRequestResponseStatus", HFILL }},
    { &hf_h225_unsolicited,
      { "unsolicited", "h225.unsolicited",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_complete,
      { "complete", "h225.complete_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_incomplete,
      { "incomplete", "h225.incomplete_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_segment,
      { "segment", "h225.segment",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_65535", HFILL }},
    { &hf_h225_invalidCall,
      { "invalidCall", "h225.invalidCall_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_nakReason,
      { "nakReason", "h225.nakReason",
        FT_UINT32, BASE_DEC, VALS(InfoRequestNakReason_vals), 0,
        "InfoRequestNakReason", HFILL }},
    { &hf_h225_messageNotUnderstood,
      { "messageNotUnderstood", "h225.messageNotUnderstood",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING", HFILL }},
    { &hf_h225_delay,
      { "delay", "h225.delay",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_65535", HFILL }},
    { &hf_h225_protocols,
      { "protocols", "h225.protocols",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_OF_SupportedProtocols", HFILL }},
    { &hf_h225_protocols_item,
      { "SupportedProtocols", "h225.SupportedProtocols",
        FT_UINT32, BASE_DEC, VALS(h225_SupportedProtocols_vals), 0,
        NULL, HFILL }},
    { &hf_h225_almostOutOfResources,
      { "almostOutOfResources", "h225.almostOutOfResources",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_h225_callSpecific,
      { "callSpecific", "h225.callSpecific_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_result,
      { "result", "h225.result",
        FT_UINT32, BASE_DEC, VALS(h225_T_result_vals), 0,
        NULL, HFILL }},
    { &hf_h225_started,
      { "started", "h225.started_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_failed,
      { "failed", "h225.failed_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_stopped,
      { "stopped", "h225.stopped_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_h225_notAvailable,
      { "notAvailable", "h225.notAvailable_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},

/*--- End of included file: packet-h225-hfarr.c ---*/
#line 843 "./asn1/h225/packet-h225-template.c"
  };

  /* List of subtrees */
  static gint *ett[] = {
    &ett_h225,

/*--- Included file: packet-h225-ettarr.c ---*/
#line 1 "./asn1/h225/packet-h225-ettarr.c"
    &ett_h225_H323_UserInformation,
    &ett_h225_T_user_data,
    &ett_h225_H323_UU_PDU,
    &ett_h225_T_h323_message_body,
    &ett_h225_T_h4501SupplementaryService,
    &ett_h225_H245Control,
    &ett_h225_SEQUENCE_OF_NonStandardParameter,
    &ett_h225_T_tunnelledSignallingMessage,
    &ett_h225_T_messageContent,
    &ett_h225_SEQUENCE_OF_GenericData,
    &ett_h225_StimulusControl,
    &ett_h225_Alerting_UUIE,
    &ett_h225_SEQUENCE_OF_ClearToken,
    &ett_h225_SEQUENCE_OF_CryptoH323Token,
    &ett_h225_SEQUENCE_OF_AliasAddress,
    &ett_h225_SEQUENCE_OF_ServiceControlSession,
    &ett_h225_SEQUENCE_OF_DisplayName,
    &ett_h225_CallProceeding_UUIE,
    &ett_h225_Connect_UUIE,
    &ett_h225_Information_UUIE,
    &ett_h225_ReleaseComplete_UUIE,
    &ett_h225_ReleaseCompleteReason,
    &ett_h225_Setup_UUIE,
    &ett_h225_SEQUENCE_OF_CallReferenceValue,
    &ett_h225_T_conferenceGoal,
    &ett_h225_SEQUENCE_OF_H245Security,
    &ett_h225_FastStart,
    &ett_h225_T_connectionParameters,
    &ett_h225_Language,
    &ett_h225_SEQUENCE_OF_SupportedProtocols,
    &ett_h225_SEQUENCE_OF_FeatureDescriptor,
    &ett_h225_ParallelH245Control,
    &ett_h225_SEQUENCE_OF_ExtendedAliasAddress,
    &ett_h225_ScnConnectionType,
    &ett_h225_ScnConnectionAggregation,
    &ett_h225_PresentationIndicator,
    &ett_h225_Facility_UUIE,
    &ett_h225_SEQUENCE_OF_ConferenceList,
    &ett_h225_ConferenceList,
    &ett_h225_FacilityReason,
    &ett_h225_Progress_UUIE,
    &ett_h225_TransportAddress,
    &ett_h225_H245TransportAddress,
    &ett_h225_T_h245IpAddress,
    &ett_h225_T_h245IpSourceRoute,
    &ett_h225_T_h245Route,
    &ett_h225_T_h245Routing,
    &ett_h225_T_h245IpxAddress,
    &ett_h225_T_h245Ip6Address,
    &ett_h225_T_ipAddress,
    &ett_h225_T_ipSourceRoute,
    &ett_h225_T_route,
    &ett_h225_T_routing,
    &ett_h225_T_ipxAddress,
    &ett_h225_T_ip6Address,
    &ett_h225_Status_UUIE,
    &ett_h225_StatusInquiry_UUIE,
    &ett_h225_SetupAcknowledge_UUIE,
    &ett_h225_Notify_UUIE,
    &ett_h225_EndpointType,
    &ett_h225_SEQUENCE_OF_TunnelledProtocol,
    &ett_h225_GatewayInfo,
    &ett_h225_SupportedProtocols,
    &ett_h225_H310Caps,
    &ett_h225_SEQUENCE_OF_DataRate,
    &ett_h225_SEQUENCE_OF_SupportedPrefix,
    &ett_h225_H320Caps,
    &ett_h225_H321Caps,
    &ett_h225_H322Caps,
    &ett_h225_H323Caps,
    &ett_h225_H324Caps,
    &ett_h225_VoiceCaps,
    &ett_h225_T120OnlyCaps,
    &ett_h225_NonStandardProtocol,
    &ett_h225_T38FaxAnnexbOnlyCaps,
    &ett_h225_SIPCaps,
    &ett_h225_McuInfo,
    &ett_h225_TerminalInfo,
    &ett_h225_GatekeeperInfo,
    &ett_h225_VendorIdentifier,
    &ett_h225_H221NonStandard,
    &ett_h225_TunnelledProtocol,
    &ett_h225_TunnelledProtocol_id,
    &ett_h225_TunnelledProtocolAlternateIdentifier,
    &ett_h225_NonStandardParameter,
    &ett_h225_NonStandardIdentifier,
    &ett_h225_AliasAddress,
    &ett_h225_AddressPattern,
    &ett_h225_T_range,
    &ett_h225_PartyNumber,
    &ett_h225_PublicPartyNumber,
    &ett_h225_PrivatePartyNumber,
    &ett_h225_DisplayName,
    &ett_h225_PublicTypeOfNumber,
    &ett_h225_PrivateTypeOfNumber,
    &ett_h225_MobileUIM,
    &ett_h225_ANSI_41_UIM,
    &ett_h225_T_system_id,
    &ett_h225_GSM_UIM,
    &ett_h225_IsupNumber,
    &ett_h225_IsupPublicPartyNumber,
    &ett_h225_IsupPrivatePartyNumber,
    &ett_h225_NatureOfAddress,
    &ett_h225_ExtendedAliasAddress,
    &ett_h225_Endpoint,
    &ett_h225_SEQUENCE_OF_TransportAddress,
    &ett_h225_AlternateTransportAddresses,
    &ett_h225_UseSpecifiedTransport,
    &ett_h225_AlternateGK,
    &ett_h225_AltGKInfo,
    &ett_h225_SEQUENCE_OF_AlternateGK,
    &ett_h225_SecurityServiceMode,
    &ett_h225_SecurityCapabilities,
    &ett_h225_SecurityErrors,
    &ett_h225_SecurityErrors2,
    &ett_h225_H245Security,
    &ett_h225_QseriesOptions,
    &ett_h225_Q954Details,
    &ett_h225_CallIdentifier,
    &ett_h225_EncryptIntAlg,
    &ett_h225_NonIsoIntegrityMechanism,
    &ett_h225_IntegrityMechanism,
    &ett_h225_ICV,
    &ett_h225_CryptoH323Token,
    &ett_h225_T_cryptoEPPwdHash,
    &ett_h225_T_cryptoGKPwdHash,
    &ett_h225_DataRate,
    &ett_h225_CallLinkage,
    &ett_h225_SupportedPrefix,
    &ett_h225_CapacityReportingCapability,
    &ett_h225_CapacityReportingSpecification,
    &ett_h225_CapacityReportingSpecification_when,
    &ett_h225_CallCapacity,
    &ett_h225_CallCapacityInfo,
    &ett_h225_SEQUENCE_OF_CallsAvailable,
    &ett_h225_CallsAvailable,
    &ett_h225_CircuitInfo,
    &ett_h225_CircuitIdentifier,
    &ett_h225_CicInfo,
    &ett_h225_T_cic_2_4,
    &ett_h225_GroupID,
    &ett_h225_T_member,
    &ett_h225_CarrierInfo,
    &ett_h225_ServiceControlDescriptor,
    &ett_h225_ServiceControlSession,
    &ett_h225_ServiceControlSession_reason,
    &ett_h225_RasUsageInfoTypes,
    &ett_h225_RasUsageSpecification,
    &ett_h225_RasUsageSpecification_when,
    &ett_h225_RasUsageSpecificationcallStartingPoint,
    &ett_h225_RasUsageInformation,
    &ett_h225_CallTerminationCause,
    &ett_h225_BandwidthDetails,
    &ett_h225_CallCreditCapability,
    &ett_h225_CallCreditServiceControl,
    &ett_h225_T_billingMode,
    &ett_h225_CallCreditServiceControl_callStartingPoint,
    &ett_h225_GenericData,
    &ett_h225_SEQUENCE_SIZE_1_512_OF_EnumeratedParameter,
    &ett_h225_GenericIdentifier,
    &ett_h225_EnumeratedParameter,
    &ett_h225_Content,
    &ett_h225_SEQUENCE_SIZE_1_16_OF_GenericData,
    &ett_h225_FeatureSet,
    &ett_h225_TransportChannelInfo,
    &ett_h225_RTPSession,
    &ett_h225_T_associatedSessionIds,
    &ett_h225_RehomingModel,
    &ett_h225_RasMessage,
    &ett_h225_SEQUENCE_OF_AdmissionConfirm,
    &ett_h225_GatekeeperRequest,
    &ett_h225_SEQUENCE_OF_Endpoint,
    &ett_h225_SEQUENCE_OF_AuthenticationMechanism,
    &ett_h225_T_algorithmOIDs,
    &ett_h225_SEQUENCE_OF_IntegrityMechanism,
    &ett_h225_GatekeeperConfirm,
    &ett_h225_GatekeeperReject,
    &ett_h225_GatekeeperRejectReason,
    &ett_h225_RegistrationRequest,
    &ett_h225_SEQUENCE_OF_AddressPattern,
    &ett_h225_SEQUENCE_OF_H248PackagesDescriptor,
    &ett_h225_RegistrationConfirm,
    &ett_h225_T_preGrantedARQ,
    &ett_h225_SEQUENCE_OF_RasUsageSpecification,
    &ett_h225_RegistrationReject,
    &ett_h225_RegistrationRejectReason,
    &ett_h225_T_invalidTerminalAliases,
    &ett_h225_UnregistrationRequest,
    &ett_h225_UnregRequestReason,
    &ett_h225_UnregistrationConfirm,
    &ett_h225_UnregistrationReject,
    &ett_h225_UnregRejectReason,
    &ett_h225_AdmissionRequest,
    &ett_h225_DestinationInfo,
    &ett_h225_CallType,
    &ett_h225_CallModel,
    &ett_h225_TransportQOS,
    &ett_h225_SEQUENCE_SIZE_1_256_OF_QOSCapability,
    &ett_h225_AdmissionConfirm,
    &ett_h225_UUIEsRequested,
    &ett_h225_AdmissionReject,
    &ett_h225_AdmissionRejectReason,
    &ett_h225_SEQUENCE_OF_PartyNumber,
    &ett_h225_BandwidthRequest,
    &ett_h225_SEQUENCE_OF_BandwidthDetails,
    &ett_h225_BandwidthConfirm,
    &ett_h225_BandwidthReject,
    &ett_h225_BandRejectReason,
    &ett_h225_LocationRequest,
    &ett_h225_LocationConfirm,
    &ett_h225_LocationReject,
    &ett_h225_LocationRejectReason,
    &ett_h225_DisengageRequest,
    &ett_h225_DisengageReason,
    &ett_h225_DisengageConfirm,
    &ett_h225_DisengageReject,
    &ett_h225_DisengageRejectReason,
    &ett_h225_InfoRequest,
    &ett_h225_InfoRequestResponse,
    &ett_h225_T_perCallInfo,
    &ett_h225_T_perCallInfo_item,
    &ett_h225_SEQUENCE_OF_RTPSession,
    &ett_h225_SEQUENCE_OF_TransportChannelInfo,
    &ett_h225_SEQUENCE_OF_ConferenceIdentifier,
    &ett_h225_T_pdu,
    &ett_h225_T_pdu_item,
    &ett_h225_InfoRequestResponseStatus,
    &ett_h225_InfoRequestAck,
    &ett_h225_InfoRequestNak,
    &ett_h225_InfoRequestNakReason,
    &ett_h225_NonStandardMessage,
    &ett_h225_UnknownMessageResponse,
    &ett_h225_RequestInProgress,
    &ett_h225_ResourcesAvailableIndicate,
    &ett_h225_ResourcesAvailableConfirm,
    &ett_h225_ServiceControlIndication,
    &ett_h225_T_callSpecific,
    &ett_h225_ServiceControlResponse,
    &ett_h225_T_result,

/*--- End of included file: packet-h225-ettarr.c ---*/
#line 849 "./asn1/h225/packet-h225-template.c"
  };

  static tap_param h225_stat_params[] = {
    { PARAM_FILTER, "filter", "Filter", NULL, TRUE }
  };

  static stat_tap_table_ui h225_stat_table = {
    REGISTER_STAT_GROUP_TELEPHONY,
    "H.225",
    PFNAME,
    "h225,counter",
    h225_stat_init,
    h225_stat_packet,
    h225_stat_reset,
    NULL,
    NULL,
    sizeof(h225_stat_fields)/sizeof(stat_tap_table_item), h225_stat_fields,
    sizeof(h225_stat_params)/sizeof(tap_param), h225_stat_params,
    NULL,
    0
  };

  module_t *h225_module;
  int proto_h225_ras;

  /* Register protocol */
  proto_h225 = proto_register_protocol(PNAME, PSNAME, PFNAME);

  /* Create a "fake" protocol to get proper display strings for SRT dialogs */
  proto_h225_ras = proto_register_protocol("H.225 RAS", "H.225 RAS", "h225_ras");

  /* Register fields and subtrees */
  proto_register_field_array(proto_h225, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

  h225_module = prefs_register_protocol(proto_h225, proto_reg_handoff_h225);
  prefs_register_uint_preference(h225_module, "tls.port",
    "H.225 TLS Port",
    "H.225 Server TLS Port",
    10, &h225_tls_port);
  prefs_register_bool_preference(h225_module, "reassembly",
    "Reassemble H.225 messages spanning multiple TCP segments",
    "Whether the H.225 dissector should reassemble messages spanning multiple TCP segments."
    " To use this option, you must also enable \"Allow subdissectors to reassemble TCP streams\" in the TCP protocol settings.",
    &h225_reassembly);
  prefs_register_bool_preference(h225_module, "h245_in_tree",
    "Display tunnelled H.245 inside H.225.0 tree",
    "ON - display tunnelled H.245 inside H.225.0 tree, OFF - display tunnelled H.245 in root tree after H.225.0",
    &h225_h245_in_tree);
  prefs_register_bool_preference(h225_module, "tp_in_tree",
    "Display tunnelled protocols inside H.225.0 tree",
    "ON - display tunnelled protocols inside H.225.0 tree, OFF - display tunnelled protocols in root tree after H.225.0",
    &h225_tp_in_tree);

  register_dissector(PFNAME, dissect_h225_H323UserInformation, proto_h225);
  register_dissector("h323ui",dissect_h225_H323UserInformation, proto_h225);
  h225ras_handle = register_dissector("h225.ras", dissect_h225_h225_RasMessage, proto_h225);

  nsp_object_dissector_table = register_dissector_table("h225.nsp.object", "H.225 NonStandardParameter (object)", proto_h225, FT_STRING, BASE_NONE);
  nsp_h221_dissector_table = register_dissector_table("h225.nsp.h221", "H.225 NonStandardParameter (h221)", proto_h225, FT_UINT32, BASE_HEX);
  tp_dissector_table = register_dissector_table("h225.tp", "H.225 TunnelledProtocol", proto_h225, FT_STRING, BASE_NONE);
  gef_name_dissector_table = register_dissector_table("h225.gef.name", "H.225 Generic Extensible Framework (names)", proto_h225, FT_STRING, BASE_NONE);
  gef_content_dissector_table = register_dissector_table("h225.gef.content", "H.225 Generic Extensible Framework", proto_h225, FT_STRING, BASE_NONE);

  register_init_routine(&h225_init_routine);
  register_cleanup_routine(&h225_cleanup_routine);
  h225_tap = register_tap(PFNAME);

  register_rtd_table(proto_h225_ras, PFNAME, NUM_RAS_STATS, 1, ras_message_category, h225rassrt_packet, NULL);

  register_stat_tap_table_ui(&h225_stat_table);

  oid_add_from_string("Version 1","0.0.8.2250.0.1");
  oid_add_from_string("Version 2","0.0.8.2250.0.2");
  oid_add_from_string("Version 3","0.0.8.2250.0.3");
  oid_add_from_string("Version 4","0.0.8.2250.0.4");
  oid_add_from_string("Version 5","0.0.8.2250.0.5");
  oid_add_from_string("Version 6","0.0.8.2250.0.6");
}


/*--- proto_reg_handoff_h225 ---------------------------------------*/
void
proto_reg_handoff_h225(void)
{
  static gboolean h225_prefs_initialized = FALSE;
  static dissector_handle_t q931_tpkt_handle;
  static guint saved_h225_tls_port;

  if (!h225_prefs_initialized) {
    dissector_add_uint("udp.port", UDP_PORT_RAS1, h225ras_handle);
    dissector_add_uint("udp.port", UDP_PORT_RAS2, h225ras_handle);

    h245_handle = find_dissector("h245");
    h245dg_handle = find_dissector("h245dg");
    h4501_handle = find_dissector("h4501");
    data_handle = find_dissector("data");
    h225_prefs_initialized = TRUE;
    q931_tpkt_handle = find_dissector("q931.tpkt");
  } else {
    ssl_dissector_delete(saved_h225_tls_port, q931_tpkt_handle);
  }

  saved_h225_tls_port = h225_tls_port;
  ssl_dissector_add(saved_h225_tls_port, q931_tpkt_handle);
}

static h225_packet_info* create_h225_packet_info(packet_info *pinfo)
{
  h225_packet_info* pi = wmem_new0(pinfo->pool, h225_packet_info);

  pi->msg_type = H225_OTHERS;
  pi->cs_type = H225_OTHER;
  pi->msg_tag = -1;
  pi->reason = -1;

  return pi;
}

/*
  The following function contains the routines for RAS request/response matching.
  A RAS response matches with a request, if both messages have the same
  RequestSequenceNumber, belong to the same IP conversation and belong to the same
  RAS "category" (e.g. Admission, Registration).

  We use hashtables to access the lists of RAS calls (request/response pairs).
  We have one hashtable for each RAS category. The hashkeys consist of the
  non-unique 16-bit RequestSequenceNumber and values representing the conversation.

  In big capture files, we might get different requests with identical keys.
  These requests aren't necessarily duplicates. They might be valid new requests.
  At the moment we just use the timedelta between the last valid and the new request
  to decide if the new request is a duplicate or not. There might be better ways.
  Two thresholds are defined below.

  However the decision is made, another problem arises. We can't just add those
  requests to our hashtables. Instead we create lists of RAS calls with identical keys.
  The hashtables for RAS calls contain now pointers to the first RAS call in a list of
  RAS calls with identical keys.
  These lists aren't expected to contain more than 3 items and are usually single item
  lists. So we don't need an expensive but intelligent way to access these lists
  (e.g. hashtables). Just walk through such a list.
*/

#define THRESHOLD_REPEATED_RESPONDED_CALL 300
#define THRESHOLD_REPEATED_NOT_RESPONDED_CALL 1800

static void ras_call_matching(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, h225_packet_info *pi)
{
  proto_item *hidden_item;
  conversation_t* conversation = NULL;
  h225ras_call_info_key h225ras_call_key;
  h225ras_call_t *h225ras_call = NULL;
  nstime_t delta;
  guint msg_category;

  if(pi->msg_type == H225_RAS && pi->msg_tag < 21) {
    /* make RAS request/response matching only for tags from 0 to 20 for now */

    msg_category = pi->msg_tag / 3;
    if(pi->msg_tag % 3 == 0) {    /* Request Message */
      conversation = find_or_create_conversation(pinfo);

      /* prepare the key data */
      h225ras_call_key.reqSeqNum = pi->requestSeqNum;
      h225ras_call_key.conversation = conversation;

      /* look up the request */
      h225ras_call = find_h225ras_call(&h225ras_call_key ,msg_category);

      if (h225ras_call != NULL) {
        /* We've seen requests with this reqSeqNum, with the same
           source and destination, before - do we have
           *this* request already? */
        /* Walk through list of ras requests with identical keys */
        do {
          if (pinfo->num == h225ras_call->req_num) {
            /* We have seen this request before -> do nothing */
            break;
          }

          /* if end of list is reached, exit loop and decide if request is duplicate or not. */
          if (h225ras_call->next_call == NULL) {
            if ( (pinfo->num > h225ras_call->rsp_num && h225ras_call->rsp_num != 0
               && pinfo->abs_ts.secs > (h225ras_call->req_time.secs + THRESHOLD_REPEATED_RESPONDED_CALL) )
               ||(pinfo->num > h225ras_call->req_num && h225ras_call->rsp_num == 0
               && pinfo->abs_ts.secs > (h225ras_call->req_time.secs + THRESHOLD_REPEATED_NOT_RESPONDED_CALL) ) )
            {
              /* if last request has been responded
                 and this request appears after last response (has bigger frame number)
                 and last request occurred more than 300 seconds ago,
                 or if last request hasn't been responded
                 and this request appears after last request (has bigger frame number)
                 and last request occurred more than 1800 seconds ago,
                 we decide that we have a new request */
              /* Append new ras call to list */
              h225ras_call = append_h225ras_call(h225ras_call, pinfo, &pi->guid, msg_category);
            } else {
              /* No, so it's a duplicate request.
                 Mark it as such. */
              pi->is_duplicate = TRUE;
              hidden_item = proto_tree_add_uint(tree, hf_h225_ras_dup, tvb, 0,0, pi->requestSeqNum);
              PROTO_ITEM_SET_HIDDEN(hidden_item);
            }
            break;
          }
          h225ras_call = h225ras_call->next_call;
        } while (h225ras_call != NULL );
      }
      else {
        h225ras_call = new_h225ras_call(&h225ras_call_key, pinfo, &pi->guid, msg_category);
      }

      /* add link to response frame, if available */
      if(h225ras_call && h225ras_call->rsp_num != 0){
        proto_item *ti =
        proto_tree_add_uint_format(tree, hf_h225_ras_rsp_frame, tvb, 0, 0, h225ras_call->rsp_num,
                                     "The response to this request is in frame %u",
                                     h225ras_call->rsp_num);
        PROTO_ITEM_SET_GENERATED(ti);
      }

    /* end of request message handling*/
    }
    else {          /* Confirm or Reject Message */
      conversation = find_conversation(pinfo->num, &pinfo->src,
        &pinfo->dst, pinfo->ptype, pinfo->srcport,
        pinfo->destport, 0);
      if (conversation != NULL) {
        /* look only for matching request, if
           matching conversation is available. */
        h225ras_call_key.reqSeqNum = pi->requestSeqNum;
        h225ras_call_key.conversation = conversation;
        h225ras_call = find_h225ras_call(&h225ras_call_key ,msg_category);
        if(h225ras_call) {
          /* find matching ras_call in list of ras calls with identical keys */
          do {
            if (pinfo->num == h225ras_call->rsp_num) {
              /* We have seen this response before -> stop now with matching ras call */
              break;
            }

            /* Break when list end is reached */
            if(h225ras_call->next_call == NULL) {
              break;
            }
            h225ras_call = h225ras_call->next_call;
          } while (h225ras_call != NULL) ;

          if (!h225ras_call) {
            return;
          }

          /* if this is an ACF, ARJ or DCF, DRJ, give guid to tap and make it filterable */
          if (msg_category == 3 || msg_category == 5) {
            pi->guid = h225ras_call->guid;
            hidden_item = proto_tree_add_guid(tree, hf_h225_guid, tvb, 0, GUID_LEN, &pi->guid);
            PROTO_ITEM_SET_HIDDEN(hidden_item);
          }

          if (h225ras_call->rsp_num == 0) {
            /* We have not yet seen a response to that call, so
               this must be the first response; remember its
               frame number. */
            h225ras_call->rsp_num = pinfo->num;
          }
          else {
            /* We have seen a response to this call - but was it
               *this* response? */
            if (h225ras_call->rsp_num != pinfo->num) {
              /* No, so it's a duplicate response.
                 Mark it as such. */
              pi->is_duplicate = TRUE;
              hidden_item = proto_tree_add_uint(tree, hf_h225_ras_dup, tvb, 0,0, pi->requestSeqNum);
              PROTO_ITEM_SET_HIDDEN(hidden_item);
            }
          }

          if(h225ras_call->req_num != 0){
            proto_item *ti;
            h225ras_call->responded = TRUE;
            pi->request_available = TRUE;

            /* Indicate the frame to which this is a reply. */
            ti = proto_tree_add_uint_format(tree, hf_h225_ras_req_frame, tvb, 0, 0, h225ras_call->req_num,
              "This is a response to a request in frame %u", h225ras_call->req_num);
            PROTO_ITEM_SET_GENERATED(ti);

            /* Calculate RAS Service Response Time */
            nstime_delta(&delta, &pinfo->abs_ts, &h225ras_call->req_time);
            pi->delta_time = delta; /* give it to tap */

            /* display Ras Service Response Time and make it filterable */
            ti = proto_tree_add_time(tree, hf_h225_ras_deltatime, tvb, 0, 0, &(pi->delta_time));
            PROTO_ITEM_SET_GENERATED(ti);
          }
        }
      }
    }
  }
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
 * vi: set shiftwidth=2 tabstop=8 expandtab:
 * :indentSize=2:tabSize=8:noTabs=true:
 */
