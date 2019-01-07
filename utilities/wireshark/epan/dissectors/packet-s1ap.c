/* Do not modify this file. Changes will be overwritten.                      */
/* Generated automatically by the ASN.1 to Wireshark dissector compiler       */
/* packet-s1ap.c                                                              */
/* asn2wrs.py -p s1ap -c ./s1ap.cnf -s ./packet-s1ap-template -D . -O ../.. S1AP-CommonDataTypes.asn S1AP-Constants.asn S1AP-Containers.asn S1AP-IEs.asn S1AP-PDU-Contents.asn S1AP-PDU-Descriptions.asn S1AP-SonTransfer-IEs.asn */

/* Input file: packet-s1ap-template.c */

#line 1 "./asn1/s1ap/packet-s1ap-template.c"
/* packet-s1ap.c
 * Routines for E-UTRAN S1 Application Protocol (S1AP) packet dissection
 * Copyright 2007-2010, Anders Broman <anders.broman@ericsson.com>
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
 * Based on the RANAP dissector
 *
 * References: 3GPP TS 36.413 V9.2.0 (2010-03)
 */

#include "config.h"

#include <epan/packet.h>

#include <epan/strutil.h>
#include <epan/asn1.h>
#include <epan/prefs.h>
#include <epan/sctpppids.h>

#include "packet-ber.h"
#include "packet-per.h"
#include "packet-e212.h"
#include "packet-sccp.h"
#include "packet-lte-rrc.h"
#include "packet-ranap.h"
#include "packet-bssgp.h"
#include "packet-s1ap.h"
#include "packet-a21.h"

#ifdef _MSC_VER
/* disable: "warning C4146: unary minus operator applied to unsigned type, result still unsigned" */
#pragma warning(disable:4146)
#endif

#define PNAME  "S1 Application Protocol"
#define PSNAME "S1AP"
#define PFNAME "s1ap"

/* Dissector will use SCTP PPID 18 or SCTP port. IANA assigned port = 36412 */
#define SCTP_PORT_S1AP	36412

void proto_register_s1ap(void);
void proto_reg_handoff_s1ap(void);

static dissector_handle_t nas_eps_handle;
static dissector_handle_t lppa_handle;
static dissector_handle_t bssgp_handle;


/*--- Included file: packet-s1ap-val.h ---*/
#line 1 "./asn1/s1ap/packet-s1ap-val.h"
#define maxPrivateIEs                  65535
#define maxProtocolExtensions          65535
#define maxProtocolIEs                 65535
#define maxnoofCSGs                    256
#define maxnoofE_RABs                  256
#define maxnoofTAIs                    256
#define maxnoofTACs                    256
#define maxnoofErrors                  256
#define maxnoofBPLMNs                  6
#define maxnoofPLMNsPerMME             32
#define maxnoofEPLMNs                  15
#define maxnoofEPLMNsPlusOne           16
#define maxnoofForbLACs                4096
#define maxnoofForbTACs                4096
#define maxnoofIndividualS1ConnectionsToReset 256
#define maxnoofCells                   16
#define maxnoofCellsineNB              256
#define maxnoofTAIforWarning           65535
#define maxnoofCellID                  65535
#define maxnoofEmergencyAreaID         65535
#define maxnoofCellinTAI               65535
#define maxnoofCellinEAI               65535
#define maxnoofeNBX2TLAs               2
#define maxnoofeNBX2ExtTLAs            16
#define maxnoofeNBX2GTPTLAs            16
#define maxnoofRATs                    8
#define maxnoofGroupIDs                65535
#define maxnoofMMECs                   256
#define maxnoofCellIDforMDT            32
#define maxnoofTAforMDT                8
#define maxnoofMDTPLMNs                16
#define maxnoofCellsforRestart         256
#define maxnoofRestartTAIs             2048
#define maxnoofRestartEmergencyAreaIDs 256
#define maxEARFCN                      262143
#define maxnoofMBSFNAreaMDT            8
#define maxnoofIRATReportingCells      128
#define maxnoofcandidateCells          16
#define maxnoofCellineNB               256

typedef enum _ProcedureCode_enum {
  id_HandoverPreparation =   0,
  id_HandoverResourceAllocation =   1,
  id_HandoverNotification =   2,
  id_PathSwitchRequest =   3,
  id_HandoverCancel =   4,
  id_E_RABSetup =   5,
  id_E_RABModify =   6,
  id_E_RABRelease =   7,
  id_E_RABReleaseIndication =   8,
  id_InitialContextSetup =   9,
  id_Paging    =  10,
  id_downlinkNASTransport =  11,
  id_initialUEMessage =  12,
  id_uplinkNASTransport =  13,
  id_Reset     =  14,
  id_ErrorIndication =  15,
  id_NASNonDeliveryIndication =  16,
  id_S1Setup   =  17,
  id_UEContextReleaseRequest =  18,
  id_DownlinkS1cdma2000tunnelling =  19,
  id_UplinkS1cdma2000tunnelling =  20,
  id_UEContextModification =  21,
  id_UECapabilityInfoIndication =  22,
  id_UEContextRelease =  23,
  id_eNBStatusTransfer =  24,
  id_MMEStatusTransfer =  25,
  id_DeactivateTrace =  26,
  id_TraceStart =  27,
  id_TraceFailureIndication =  28,
  id_ENBConfigurationUpdate =  29,
  id_MMEConfigurationUpdate =  30,
  id_LocationReportingControl =  31,
  id_LocationReportingFailureIndication =  32,
  id_LocationReport =  33,
  id_OverloadStart =  34,
  id_OverloadStop =  35,
  id_WriteReplaceWarning =  36,
  id_eNBDirectInformationTransfer =  37,
  id_MMEDirectInformationTransfer =  38,
  id_PrivateMessage =  39,
  id_eNBConfigurationTransfer =  40,
  id_MMEConfigurationTransfer =  41,
  id_CellTrafficTrace =  42,
  id_Kill      =  43,
  id_downlinkUEAssociatedLPPaTransport =  44,
  id_uplinkUEAssociatedLPPaTransport =  45,
  id_downlinkNonUEAssociatedLPPaTransport =  46,
  id_uplinkNonUEAssociatedLPPaTransport =  47,
  id_UERadioCapabilityMatch =  48,
  id_PWSRestartIndication =  49,
  id_E_RABModificationIndication =  50,
  id_PWSFailureIndication =  51,
  id_RerouteNASRequest =  52,
  id_UEContextModificationIndication =  53
} ProcedureCode_enum;

typedef enum _ProtocolIE_ID_enum {
  id_MME_UE_S1AP_ID =   0,
  id_HandoverType =   1,
  id_Cause     =   2,
  id_SourceID  =   3,
  id_TargetID  =   4,
  id_Unknown_5 =   5,
  id_Unknown_6 =   6,
  id_Unknown_7 =   7,
  id_eNB_UE_S1AP_ID =   8,
  id_Unknown_9 =   9,
  id_Unknown_10 =  10,
  id_Unknown_11 =  11,
  id_E_RABSubjecttoDataForwardingList =  12,
  id_E_RABtoReleaseListHOCmd =  13,
  id_E_RABDataForwardingItem =  14,
  id_E_RABReleaseItemBearerRelComp =  15,
  id_E_RABToBeSetupListBearerSUReq =  16,
  id_E_RABToBeSetupItemBearerSUReq =  17,
  id_E_RABAdmittedList =  18,
  id_E_RABFailedToSetupListHOReqAck =  19,
  id_E_RABAdmittedItem =  20,
  id_E_RABFailedtoSetupItemHOReqAck =  21,
  id_E_RABToBeSwitchedDLList =  22,
  id_E_RABToBeSwitchedDLItem =  23,
  id_E_RABToBeSetupListCtxtSUReq =  24,
  id_TraceActivation =  25,
  id_NAS_PDU   =  26,
  id_E_RABToBeSetupItemHOReq =  27,
  id_E_RABSetupListBearerSURes =  28,
  id_E_RABFailedToSetupListBearerSURes =  29,
  id_E_RABToBeModifiedListBearerModReq =  30,
  id_E_RABModifyListBearerModRes =  31,
  id_E_RABFailedToModifyList =  32,
  id_E_RABToBeReleasedList =  33,
  id_E_RABFailedToReleaseList =  34,
  id_E_RABItem =  35,
  id_E_RABToBeModifiedItemBearerModReq =  36,
  id_E_RABModifyItemBearerModRes =  37,
  id_E_RABReleaseItem =  38,
  id_E_RABSetupItemBearerSURes =  39,
  id_SecurityContext =  40,
  id_HandoverRestrictionList =  41,
  id_Unknown_42 =  42,
  id_UEPagingID =  43,
  id_pagingDRX =  44,
  id_Unknown_45 =  45,
  id_TAIList   =  46,
  id_TAIItem   =  47,
  id_E_RABFailedToSetupListCtxtSURes =  48,
  id_E_RABReleaseItemHOCmd =  49,
  id_E_RABSetupItemCtxtSURes =  50,
  id_E_RABSetupListCtxtSURes =  51,
  id_E_RABToBeSetupItemCtxtSUReq =  52,
  id_E_RABToBeSetupListHOReq =  53,
  id_Unknown_54 =  54,
  id_GERANtoLTEHOInformationRes =  55,
  id_Unknown_56 =  56,
  id_UTRANtoLTEHOInformationRes =  57,
  id_CriticalityDiagnostics =  58,
  id_Global_ENB_ID =  59,
  id_eNBname   =  60,
  id_MMEname   =  61,
  id_Unknown_62 =  62,
  id_ServedPLMNs =  63,
  id_SupportedTAs =  64,
  id_TimeToWait =  65,
  id_uEaggregateMaximumBitrate =  66,
  id_TAI       =  67,
  id_Unknown_68 =  68,
  id_E_RABReleaseListBearerRelComp =  69,
  id_cdma2000PDU =  70,
  id_cdma2000RATType =  71,
  id_cdma2000SectorID =  72,
  id_SecurityKey =  73,
  id_UERadioCapability =  74,
  id_GUMMEI_ID =  75,
  id_Unknown_76 =  76,
  id_E_RABInformationListItem =  78,
  id_Direct_Forwarding_Path_Availability =  79,
  id_UEIdentityIndexValue =  80,
  id_Unknown_81 =  81,
  id_Unknown_82 =  82,
  id_cdma2000HOStatus =  83,
  id_cdma2000HORequiredIndication =  84,
  id_E_UTRAN_Trace_ID =  86,
  id_RelativeMMECapacity =  87,
  id_SourceMME_UE_S1AP_ID =  88,
  id_Bearers_SubjectToStatusTransfer_Item =  89,
  id_eNB_StatusTransfer_TransparentContainer =  90,
  id_UE_associatedLogicalS1_ConnectionItem =  91,
  id_ResetType =  92,
  id_UE_associatedLogicalS1_ConnectionListResAck =  93,
  id_E_RABToBeSwitchedULItem =  94,
  id_E_RABToBeSwitchedULList =  95,
  id_S_TMSI    =  96,
  id_cdma2000OneXRAND =  97,
  id_RequestType =  98,
  id_UE_S1AP_IDs =  99,
  id_EUTRAN_CGI = 100,
  id_OverloadResponse = 101,
  id_cdma2000OneXSRVCCInfo = 102,
  id_E_RABFailedToBeReleasedList = 103,
  id_Source_ToTarget_TransparentContainer = 104,
  id_ServedGUMMEIs = 105,
  id_SubscriberProfileIDforRFP = 106,
  id_UESecurityCapabilities = 107,
  id_CSFallbackIndicator = 108,
  id_CNDomain  = 109,
  id_E_RABReleasedList = 110,
  id_MessageIdentifier = 111,
  id_SerialNumber = 112,
  id_WarningAreaList = 113,
  id_RepetitionPeriod = 114,
  id_NumberofBroadcastRequest = 115,
  id_WarningType = 116,
  id_WarningSecurityInfo = 117,
  id_DataCodingScheme = 118,
  id_WarningMessageContents = 119,
  id_BroadcastCompletedAreaList = 120,
  id_Inter_SystemInformationTransferTypeEDT = 121,
  id_Inter_SystemInformationTransferTypeMDT = 122,
  id_Target_ToSource_TransparentContainer = 123,
  id_SRVCCOperationPossible = 124,
  id_SRVCCHOIndication = 125,
  id_NAS_DownlinkCount = 126,
  id_CSG_Id    = 127,
  id_CSG_IdList = 128,
  id_SONConfigurationTransferECT = 129,
  id_SONConfigurationTransferMCT = 130,
  id_TraceCollectionEntityIPAddress = 131,
  id_MSClassmark2 = 132,
  id_MSClassmark3 = 133,
  id_RRC_Establishment_Cause = 134,
  id_NASSecurityParametersfromE_UTRAN = 135,
  id_NASSecurityParameterstoE_UTRAN = 136,
  id_DefaultPagingDRX = 137,
  id_Source_ToTarget_TransparentContainer_Secondary = 138,
  id_Target_ToSource_TransparentContainer_Secondary = 139,
  id_EUTRANRoundTripDelayEstimationInfo = 140,
  id_BroadcastCancelledAreaList = 141,
  id_ConcurrentWarningMessageIndicator = 142,
  id_Data_Forwarding_Not_Possible = 143,
  id_ExtendedRepetitionPeriod = 144,
  id_CellAccessMode = 145,
  id_CSGMembershipStatus = 146,
  id_LPPa_PDU  = 147,
  id_Routing_ID = 148,
  id_Time_Synchronisation_Info = 149,
  id_PS_ServiceNotAvailable = 150,
  id_PagingPriority = 151,
  id_x2TNLConfigurationInfo = 152,
  id_eNBX2ExtendedTransportLayerAddresses = 153,
  id_GUMMEIList = 154,
  id_GW_TransportLayerAddress = 155,
  id_Correlation_ID = 156,
  id_SourceMME_GUMMEI = 157,
  id_MME_UE_S1AP_ID_2 = 158,
  id_RegisteredLAI = 159,
  id_RelayNode_Indicator = 160,
  id_TrafficLoadReductionIndication = 161,
  id_MDTConfiguration = 162,
  id_MMERelaySupportIndicator = 163,
  id_GWContextReleaseIndication = 164,
  id_ManagementBasedMDTAllowed = 165,
  id_PrivacyIndicator = 166,
  id_Time_UE_StayedInCell_EnhancedGranularity = 167,
  id_HO_Cause  = 168,
  id_VoiceSupportMatchIndicator = 169,
  id_GUMMEIType = 170,
  id_M3Configuration = 171,
  id_M4Configuration = 172,
  id_M5Configuration = 173,
  id_MDT_Location_Info = 174,
  id_MobilityInformation = 175,
  id_Tunnel_Information_for_BBF = 176,
  id_ManagementBasedMDTPLMNList = 177,
  id_SignallingBasedMDTPLMNList = 178,
  id_ULCOUNTValueExtended = 179,
  id_DLCOUNTValueExtended = 180,
  id_ReceiveStatusOfULPDCPSDUsExtended = 181,
  id_ECGIListForRestart = 182,
  id_SIPTO_Correlation_ID = 183,
  id_SIPTO_L_GW_TransportLayerAddress = 184,
  id_TransportInformation = 185,
  id_LHN_ID    = 186,
  id_AdditionalCSFallbackIndicator = 187,
  id_TAIListForRestart = 188,
  id_UserLocationInformation = 189,
  id_EmergencyAreaIDListForRestart = 190,
  id_KillAllWarningMessages = 191,
  id_Masked_IMEISV = 192,
  id_eNBIndirectX2TransportLayerAddresses = 193,
  id_uE_HistoryInformationFromTheUE = 194,
  id_ProSeAuthorized = 195,
  id_ExpectedUEBehaviour = 196,
  id_LoggedMBSFNMDT = 197,
  id_UERadioCapabilityForPaging = 198,
  id_E_RABToBeModifiedListBearerModInd = 199,
  id_E_RABToBeModifiedItemBearerModInd = 200,
  id_E_RABNotToBeModifiedListBearerModInd = 201,
  id_E_RABNotToBeModifiedItemBearerModInd = 202,
  id_E_RABModifyListBearerModConf = 203,
  id_E_RABModifyItemBearerModConf = 204,
  id_E_RABFailedToModifyListBearerModConf = 205,
  id_SON_Information_Report = 206,
  id_Muting_Availability_Indication = 207,
  id_Muting_Pattern_Information = 208,
  id_Synchronisation_Information = 209,
  id_E_RABToBeReleasedListBearerModConf = 210,
  id_ProSeUEtoNetworkRelaying = 216,
  id_ULCOUNTValuePDCP_SNlength18 = 217,
  id_DLCOUNTValuePDCP_SNlength18 = 218,
  id_ReceiveStatusOfULPDCPSDUsPDCP_SNlength18 = 219,
  id_M6Configuration = 220,
  id_M7Configuration = 221,
  id_PWSfailedECGIList = 222,
  id_MME_Group_ID = 223,
  id_Additional_GUTI = 224,
  id_S1_Message = 225,
  id_CSGMembershipInfo = 226
} ProtocolIE_ID_enum;

/*--- End of included file: packet-s1ap-val.h ---*/
#line 67 "./asn1/s1ap/packet-s1ap-template.c"

/* Initialize the protocol and registered fields */
static int proto_s1ap = -1;

static int hf_s1ap_transportLayerAddressIPv4 = -1;
static int hf_s1ap_transportLayerAddressIPv6 = -1;

/*--- Included file: packet-s1ap-hf.c ---*/
#line 1 "./asn1/s1ap/packet-s1ap-hf.c"
static int hf_s1ap_Additional_GUTI_PDU = -1;      /* Additional_GUTI */
static int hf_s1ap_Bearers_SubjectToStatusTransfer_Item_PDU = -1;  /* Bearers_SubjectToStatusTransfer_Item */
static int hf_s1ap_BroadcastCancelledAreaList_PDU = -1;  /* BroadcastCancelledAreaList */
static int hf_s1ap_BroadcastCompletedAreaList_PDU = -1;  /* BroadcastCompletedAreaList */
static int hf_s1ap_Cause_PDU = -1;                /* Cause */
static int hf_s1ap_CellAccessMode_PDU = -1;       /* CellAccessMode */
static int hf_s1ap_Cdma2000PDU_PDU = -1;          /* Cdma2000PDU */
static int hf_s1ap_Cdma2000RATType_PDU = -1;      /* Cdma2000RATType */
static int hf_s1ap_Cdma2000SectorID_PDU = -1;     /* Cdma2000SectorID */
static int hf_s1ap_Cdma2000HOStatus_PDU = -1;     /* Cdma2000HOStatus */
static int hf_s1ap_Cdma2000HORequiredIndication_PDU = -1;  /* Cdma2000HORequiredIndication */
static int hf_s1ap_Cdma2000OneXSRVCCInfo_PDU = -1;  /* Cdma2000OneXSRVCCInfo */
static int hf_s1ap_Cdma2000OneXRAND_PDU = -1;     /* Cdma2000OneXRAND */
static int hf_s1ap_CNDomain_PDU = -1;             /* CNDomain */
static int hf_s1ap_ConcurrentWarningMessageIndicator_PDU = -1;  /* ConcurrentWarningMessageIndicator */
static int hf_s1ap_Correlation_ID_PDU = -1;       /* Correlation_ID */
static int hf_s1ap_CSFallbackIndicator_PDU = -1;  /* CSFallbackIndicator */
static int hf_s1ap_AdditionalCSFallbackIndicator_PDU = -1;  /* AdditionalCSFallbackIndicator */
static int hf_s1ap_CSG_Id_PDU = -1;               /* CSG_Id */
static int hf_s1ap_CSG_IdList_PDU = -1;           /* CSG_IdList */
static int hf_s1ap_CSGMembershipStatus_PDU = -1;  /* CSGMembershipStatus */
static int hf_s1ap_COUNTValueExtended_PDU = -1;   /* COUNTValueExtended */
static int hf_s1ap_COUNTvaluePDCP_SNlength18_PDU = -1;  /* COUNTvaluePDCP_SNlength18 */
static int hf_s1ap_CriticalityDiagnostics_PDU = -1;  /* CriticalityDiagnostics */
static int hf_s1ap_DataCodingScheme_PDU = -1;     /* DataCodingScheme */
static int hf_s1ap_Direct_Forwarding_Path_Availability_PDU = -1;  /* Direct_Forwarding_Path_Availability */
static int hf_s1ap_Data_Forwarding_Not_Possible_PDU = -1;  /* Data_Forwarding_Not_Possible */
static int hf_s1ap_PWSfailedECGIList_PDU = -1;    /* PWSfailedECGIList */
static int hf_s1ap_EmergencyAreaIDListForRestart_PDU = -1;  /* EmergencyAreaIDListForRestart */
static int hf_s1ap_s1ap_Global_ENB_ID_PDU = -1;   /* Global_ENB_ID */
static int hf_s1ap_GUMMEIList_PDU = -1;           /* GUMMEIList */
static int hf_s1ap_s1ap_ENB_StatusTransfer_TransparentContainer_PDU = -1;  /* ENB_StatusTransfer_TransparentContainer */
static int hf_s1ap_ENB_UE_S1AP_ID_PDU = -1;       /* ENB_UE_S1AP_ID */
static int hf_s1ap_ENBname_PDU = -1;              /* ENBname */
static int hf_s1ap_E_RABInformationListItem_PDU = -1;  /* E_RABInformationListItem */
static int hf_s1ap_E_RABList_PDU = -1;            /* E_RABList */
static int hf_s1ap_E_RABItem_PDU = -1;            /* E_RABItem */
static int hf_s1ap_EUTRAN_CGI_PDU = -1;           /* EUTRAN_CGI */
static int hf_s1ap_EUTRANRoundTripDelayEstimationInfo_PDU = -1;  /* EUTRANRoundTripDelayEstimationInfo */
static int hf_s1ap_ExpectedUEBehaviour_PDU = -1;  /* ExpectedUEBehaviour */
static int hf_s1ap_ExtendedRepetitionPeriod_PDU = -1;  /* ExtendedRepetitionPeriod */
static int hf_s1ap_GUMMEI_PDU = -1;               /* GUMMEI */
static int hf_s1ap_GUMMEIType_PDU = -1;           /* GUMMEIType */
static int hf_s1ap_GWContextReleaseIndication_PDU = -1;  /* GWContextReleaseIndication */
static int hf_s1ap_HandoverRestrictionList_PDU = -1;  /* HandoverRestrictionList */
static int hf_s1ap_HandoverType_PDU = -1;         /* HandoverType */
static int hf_s1ap_Masked_IMEISV_PDU = -1;        /* Masked_IMEISV */
static int hf_s1ap_KillAllWarningMessages_PDU = -1;  /* KillAllWarningMessages */
static int hf_s1ap_LAI_PDU = -1;                  /* LAI */
static int hf_s1ap_LPPa_PDU_PDU = -1;             /* LPPa_PDU */
static int hf_s1ap_LHN_ID_PDU = -1;               /* LHN_ID */
static int hf_s1ap_LoggedMBSFNMDT_PDU = -1;       /* LoggedMBSFNMDT */
static int hf_s1ap_M3Configuration_PDU = -1;      /* M3Configuration */
static int hf_s1ap_M4Configuration_PDU = -1;      /* M4Configuration */
static int hf_s1ap_M5Configuration_PDU = -1;      /* M5Configuration */
static int hf_s1ap_M6Configuration_PDU = -1;      /* M6Configuration */
static int hf_s1ap_M7Configuration_PDU = -1;      /* M7Configuration */
static int hf_s1ap_MDT_Location_Info_PDU = -1;    /* MDT_Location_Info */
static int hf_s1ap_MDT_Configuration_PDU = -1;    /* MDT_Configuration */
static int hf_s1ap_ManagementBasedMDTAllowed_PDU = -1;  /* ManagementBasedMDTAllowed */
static int hf_s1ap_MDTPLMNList_PDU = -1;          /* MDTPLMNList */
static int hf_s1ap_PrivacyIndicator_PDU = -1;     /* PrivacyIndicator */
static int hf_s1ap_MessageIdentifier_PDU = -1;    /* MessageIdentifier */
static int hf_s1ap_MobilityInformation_PDU = -1;  /* MobilityInformation */
static int hf_s1ap_MMEname_PDU = -1;              /* MMEname */
static int hf_s1ap_MMERelaySupportIndicator_PDU = -1;  /* MMERelaySupportIndicator */
static int hf_s1ap_MME_Group_ID_PDU = -1;         /* MME_Group_ID */
static int hf_s1ap_MME_UE_S1AP_ID_PDU = -1;       /* MME_UE_S1AP_ID */
static int hf_s1ap_MSClassmark2_PDU = -1;         /* MSClassmark2 */
static int hf_s1ap_MSClassmark3_PDU = -1;         /* MSClassmark3 */
static int hf_s1ap_MutingAvailabilityIndication_PDU = -1;  /* MutingAvailabilityIndication */
static int hf_s1ap_MutingPatternInformation_PDU = -1;  /* MutingPatternInformation */
static int hf_s1ap_NAS_PDU_PDU = -1;              /* NAS_PDU */
static int hf_s1ap_NASSecurityParametersfromE_UTRAN_PDU = -1;  /* NASSecurityParametersfromE_UTRAN */
static int hf_s1ap_NASSecurityParameterstoE_UTRAN_PDU = -1;  /* NASSecurityParameterstoE_UTRAN */
static int hf_s1ap_NumberofBroadcastRequest_PDU = -1;  /* NumberofBroadcastRequest */
static int hf_s1ap_OverloadResponse_PDU = -1;     /* OverloadResponse */
static int hf_s1ap_PagingDRX_PDU = -1;            /* PagingDRX */
static int hf_s1ap_PagingPriority_PDU = -1;       /* PagingPriority */
static int hf_s1ap_ProSeAuthorized_PDU = -1;      /* ProSeAuthorized */
static int hf_s1ap_ProSeUEtoNetworkRelaying_PDU = -1;  /* ProSeUEtoNetworkRelaying */
static int hf_s1ap_PS_ServiceNotAvailable_PDU = -1;  /* PS_ServiceNotAvailable */
static int hf_s1ap_ReceiveStatusOfULPDCPSDUsExtended_PDU = -1;  /* ReceiveStatusOfULPDCPSDUsExtended */
static int hf_s1ap_ReceiveStatusOfULPDCPSDUsPDCP_SNlength18_PDU = -1;  /* ReceiveStatusOfULPDCPSDUsPDCP_SNlength18 */
static int hf_s1ap_RelativeMMECapacity_PDU = -1;  /* RelativeMMECapacity */
static int hf_s1ap_RelayNode_Indicator_PDU = -1;  /* RelayNode_Indicator */
static int hf_s1ap_RequestType_PDU = -1;          /* RequestType */
static int hf_s1ap_RepetitionPeriod_PDU = -1;     /* RepetitionPeriod */
static int hf_s1ap_RRC_Establishment_Cause_PDU = -1;  /* RRC_Establishment_Cause */
static int hf_s1ap_ECGIListForRestart_PDU = -1;   /* ECGIListForRestart */
static int hf_s1ap_Routing_ID_PDU = -1;           /* Routing_ID */
static int hf_s1ap_SecurityKey_PDU = -1;          /* SecurityKey */
static int hf_s1ap_SecurityContext_PDU = -1;      /* SecurityContext */
static int hf_s1ap_SerialNumber_PDU = -1;         /* SerialNumber */
static int hf_s1ap_SONInformationReport_PDU = -1;  /* SONInformationReport */
static int hf_s1ap_SONConfigurationTransfer_PDU = -1;  /* SONConfigurationTransfer */
static int hf_s1ap_SynchronisationInformation_PDU = -1;  /* SynchronisationInformation */
static int hf_s1ap_Source_ToTarget_TransparentContainer_PDU = -1;  /* Source_ToTarget_TransparentContainer */
static int hf_s1ap_SRVCCOperationPossible_PDU = -1;  /* SRVCCOperationPossible */
static int hf_s1ap_SRVCCHOIndication_PDU = -1;    /* SRVCCHOIndication */
static int hf_s1ap_SourceeNB_ToTargeteNB_TransparentContainer_PDU = -1;  /* SourceeNB_ToTargeteNB_TransparentContainer */
static int hf_s1ap_ServedGUMMEIs_PDU = -1;        /* ServedGUMMEIs */
static int hf_s1ap_ServedPLMNs_PDU = -1;          /* ServedPLMNs */
static int hf_s1ap_SubscriberProfileIDforRFP_PDU = -1;  /* SubscriberProfileIDforRFP */
static int hf_s1ap_SupportedTAs_PDU = -1;         /* SupportedTAs */
static int hf_s1ap_TimeSynchronisationInfo_PDU = -1;  /* TimeSynchronisationInfo */
static int hf_s1ap_S_TMSI_PDU = -1;               /* S_TMSI */
static int hf_s1ap_TAI_PDU = -1;                  /* TAI */
static int hf_s1ap_TargetID_PDU = -1;             /* TargetID */
static int hf_s1ap_TargeteNB_ToSourceeNB_TransparentContainer_PDU = -1;  /* TargeteNB_ToSourceeNB_TransparentContainer */
static int hf_s1ap_Target_ToSource_TransparentContainer_PDU = -1;  /* Target_ToSource_TransparentContainer */
static int hf_s1ap_TimeToWait_PDU = -1;           /* TimeToWait */
static int hf_s1ap_Time_UE_StayedInCell_EnhancedGranularity_PDU = -1;  /* Time_UE_StayedInCell_EnhancedGranularity */
static int hf_s1ap_TransportInformation_PDU = -1;  /* TransportInformation */
static int hf_s1ap_TransportLayerAddress_PDU = -1;  /* TransportLayerAddress */
static int hf_s1ap_TraceActivation_PDU = -1;      /* TraceActivation */
static int hf_s1ap_E_UTRAN_Trace_ID_PDU = -1;     /* E_UTRAN_Trace_ID */
static int hf_s1ap_TrafficLoadReductionIndication_PDU = -1;  /* TrafficLoadReductionIndication */
static int hf_s1ap_TunnelInformation_PDU = -1;    /* TunnelInformation */
static int hf_s1ap_TAIListForRestart_PDU = -1;    /* TAIListForRestart */
static int hf_s1ap_UEAggregateMaximumBitrate_PDU = -1;  /* UEAggregateMaximumBitrate */
static int hf_s1ap_UE_S1AP_IDs_PDU = -1;          /* UE_S1AP_IDs */
static int hf_s1ap_UE_associatedLogicalS1_ConnectionItem_PDU = -1;  /* UE_associatedLogicalS1_ConnectionItem */
static int hf_s1ap_UEIdentityIndexValue_PDU = -1;  /* UEIdentityIndexValue */
static int hf_s1ap_s1ap_UE_HistoryInformation_PDU = -1;  /* UE_HistoryInformation */
static int hf_s1ap_UE_HistoryInformationFromTheUE_PDU = -1;  /* UE_HistoryInformationFromTheUE */
static int hf_s1ap_UEPagingID_PDU = -1;           /* UEPagingID */
static int hf_s1ap_UERadioCapability_PDU = -1;    /* UERadioCapability */
static int hf_s1ap_UERadioCapabilityForPaging_PDU = -1;  /* UERadioCapabilityForPaging */
static int hf_s1ap_UESecurityCapabilities_PDU = -1;  /* UESecurityCapabilities */
static int hf_s1ap_UserLocationInformation_PDU = -1;  /* UserLocationInformation */
static int hf_s1ap_VoiceSupportMatchIndicator_PDU = -1;  /* VoiceSupportMatchIndicator */
static int hf_s1ap_WarningAreaList_PDU = -1;      /* WarningAreaList */
static int hf_s1ap_WarningType_PDU = -1;          /* WarningType */
static int hf_s1ap_WarningSecurityInfo_PDU = -1;  /* WarningSecurityInfo */
static int hf_s1ap_WarningMessageContents_PDU = -1;  /* WarningMessageContents */
static int hf_s1ap_X2TNLConfigurationInfo_PDU = -1;  /* X2TNLConfigurationInfo */
static int hf_s1ap_ENBX2ExtTLAs_PDU = -1;         /* ENBX2ExtTLAs */
static int hf_s1ap_ENBIndirectX2TransportLayerAddresses_PDU = -1;  /* ENBIndirectX2TransportLayerAddresses */
static int hf_s1ap_HandoverRequired_PDU = -1;     /* HandoverRequired */
static int hf_s1ap_HandoverCommand_PDU = -1;      /* HandoverCommand */
static int hf_s1ap_E_RABSubjecttoDataForwardingList_PDU = -1;  /* E_RABSubjecttoDataForwardingList */
static int hf_s1ap_E_RABDataForwardingItem_PDU = -1;  /* E_RABDataForwardingItem */
static int hf_s1ap_HandoverPreparationFailure_PDU = -1;  /* HandoverPreparationFailure */
static int hf_s1ap_HandoverRequest_PDU = -1;      /* HandoverRequest */
static int hf_s1ap_E_RABToBeSetupListHOReq_PDU = -1;  /* E_RABToBeSetupListHOReq */
static int hf_s1ap_E_RABToBeSetupItemHOReq_PDU = -1;  /* E_RABToBeSetupItemHOReq */
static int hf_s1ap_HandoverRequestAcknowledge_PDU = -1;  /* HandoverRequestAcknowledge */
static int hf_s1ap_E_RABAdmittedList_PDU = -1;    /* E_RABAdmittedList */
static int hf_s1ap_E_RABAdmittedItem_PDU = -1;    /* E_RABAdmittedItem */
static int hf_s1ap_E_RABFailedtoSetupListHOReqAck_PDU = -1;  /* E_RABFailedtoSetupListHOReqAck */
static int hf_s1ap_E_RABFailedToSetupItemHOReqAck_PDU = -1;  /* E_RABFailedToSetupItemHOReqAck */
static int hf_s1ap_HandoverFailure_PDU = -1;      /* HandoverFailure */
static int hf_s1ap_HandoverNotify_PDU = -1;       /* HandoverNotify */
static int hf_s1ap_PathSwitchRequest_PDU = -1;    /* PathSwitchRequest */
static int hf_s1ap_E_RABToBeSwitchedDLList_PDU = -1;  /* E_RABToBeSwitchedDLList */
static int hf_s1ap_E_RABToBeSwitchedDLItem_PDU = -1;  /* E_RABToBeSwitchedDLItem */
static int hf_s1ap_PathSwitchRequestAcknowledge_PDU = -1;  /* PathSwitchRequestAcknowledge */
static int hf_s1ap_E_RABToBeSwitchedULList_PDU = -1;  /* E_RABToBeSwitchedULList */
static int hf_s1ap_E_RABToBeSwitchedULItem_PDU = -1;  /* E_RABToBeSwitchedULItem */
static int hf_s1ap_PathSwitchRequestFailure_PDU = -1;  /* PathSwitchRequestFailure */
static int hf_s1ap_HandoverCancel_PDU = -1;       /* HandoverCancel */
static int hf_s1ap_HandoverCancelAcknowledge_PDU = -1;  /* HandoverCancelAcknowledge */
static int hf_s1ap_E_RABSetupRequest_PDU = -1;    /* E_RABSetupRequest */
static int hf_s1ap_E_RABToBeSetupListBearerSUReq_PDU = -1;  /* E_RABToBeSetupListBearerSUReq */
static int hf_s1ap_E_RABToBeSetupItemBearerSUReq_PDU = -1;  /* E_RABToBeSetupItemBearerSUReq */
static int hf_s1ap_E_RABSetupResponse_PDU = -1;   /* E_RABSetupResponse */
static int hf_s1ap_E_RABSetupListBearerSURes_PDU = -1;  /* E_RABSetupListBearerSURes */
static int hf_s1ap_E_RABSetupItemBearerSURes_PDU = -1;  /* E_RABSetupItemBearerSURes */
static int hf_s1ap_E_RABModifyRequest_PDU = -1;   /* E_RABModifyRequest */
static int hf_s1ap_E_RABToBeModifiedListBearerModReq_PDU = -1;  /* E_RABToBeModifiedListBearerModReq */
static int hf_s1ap_E_RABToBeModifiedItemBearerModReq_PDU = -1;  /* E_RABToBeModifiedItemBearerModReq */
static int hf_s1ap_E_RABModifyResponse_PDU = -1;  /* E_RABModifyResponse */
static int hf_s1ap_E_RABModifyListBearerModRes_PDU = -1;  /* E_RABModifyListBearerModRes */
static int hf_s1ap_E_RABModifyItemBearerModRes_PDU = -1;  /* E_RABModifyItemBearerModRes */
static int hf_s1ap_E_RABReleaseCommand_PDU = -1;  /* E_RABReleaseCommand */
static int hf_s1ap_E_RABReleaseResponse_PDU = -1;  /* E_RABReleaseResponse */
static int hf_s1ap_E_RABReleaseListBearerRelComp_PDU = -1;  /* E_RABReleaseListBearerRelComp */
static int hf_s1ap_E_RABReleaseItemBearerRelComp_PDU = -1;  /* E_RABReleaseItemBearerRelComp */
static int hf_s1ap_E_RABReleaseIndication_PDU = -1;  /* E_RABReleaseIndication */
static int hf_s1ap_InitialContextSetupRequest_PDU = -1;  /* InitialContextSetupRequest */
static int hf_s1ap_E_RABToBeSetupListCtxtSUReq_PDU = -1;  /* E_RABToBeSetupListCtxtSUReq */
static int hf_s1ap_E_RABToBeSetupItemCtxtSUReq_PDU = -1;  /* E_RABToBeSetupItemCtxtSUReq */
static int hf_s1ap_InitialContextSetupResponse_PDU = -1;  /* InitialContextSetupResponse */
static int hf_s1ap_E_RABSetupListCtxtSURes_PDU = -1;  /* E_RABSetupListCtxtSURes */
static int hf_s1ap_E_RABSetupItemCtxtSURes_PDU = -1;  /* E_RABSetupItemCtxtSURes */
static int hf_s1ap_InitialContextSetupFailure_PDU = -1;  /* InitialContextSetupFailure */
static int hf_s1ap_Paging_PDU = -1;               /* Paging */
static int hf_s1ap_TAIList_PDU = -1;              /* TAIList */
static int hf_s1ap_TAIItem_PDU = -1;              /* TAIItem */
static int hf_s1ap_UEContextReleaseRequest_PDU = -1;  /* UEContextReleaseRequest */
static int hf_s1ap_UEContextReleaseCommand_PDU = -1;  /* UEContextReleaseCommand */
static int hf_s1ap_UEContextReleaseComplete_PDU = -1;  /* UEContextReleaseComplete */
static int hf_s1ap_UEContextModificationRequest_PDU = -1;  /* UEContextModificationRequest */
static int hf_s1ap_UEContextModificationResponse_PDU = -1;  /* UEContextModificationResponse */
static int hf_s1ap_UEContextModificationFailure_PDU = -1;  /* UEContextModificationFailure */
static int hf_s1ap_UERadioCapabilityMatchRequest_PDU = -1;  /* UERadioCapabilityMatchRequest */
static int hf_s1ap_UERadioCapabilityMatchResponse_PDU = -1;  /* UERadioCapabilityMatchResponse */
static int hf_s1ap_DownlinkNASTransport_PDU = -1;  /* DownlinkNASTransport */
static int hf_s1ap_InitialUEMessage_PDU = -1;     /* InitialUEMessage */
static int hf_s1ap_UplinkNASTransport_PDU = -1;   /* UplinkNASTransport */
static int hf_s1ap_NASNonDeliveryIndication_PDU = -1;  /* NASNonDeliveryIndication */
static int hf_s1ap_RerouteNASRequest_PDU = -1;    /* RerouteNASRequest */
static int hf_s1ap_Reset_PDU = -1;                /* Reset */
static int hf_s1ap_ResetType_PDU = -1;            /* ResetType */
static int hf_s1ap_ResetAcknowledge_PDU = -1;     /* ResetAcknowledge */
static int hf_s1ap_UE_associatedLogicalS1_ConnectionListResAck_PDU = -1;  /* UE_associatedLogicalS1_ConnectionListResAck */
static int hf_s1ap_ErrorIndication_PDU = -1;      /* ErrorIndication */
static int hf_s1ap_S1SetupRequest_PDU = -1;       /* S1SetupRequest */
static int hf_s1ap_S1SetupResponse_PDU = -1;      /* S1SetupResponse */
static int hf_s1ap_S1SetupFailure_PDU = -1;       /* S1SetupFailure */
static int hf_s1ap_ENBConfigurationUpdate_PDU = -1;  /* ENBConfigurationUpdate */
static int hf_s1ap_ENBConfigurationUpdateAcknowledge_PDU = -1;  /* ENBConfigurationUpdateAcknowledge */
static int hf_s1ap_ENBConfigurationUpdateFailure_PDU = -1;  /* ENBConfigurationUpdateFailure */
static int hf_s1ap_MMEConfigurationUpdate_PDU = -1;  /* MMEConfigurationUpdate */
static int hf_s1ap_MMEConfigurationUpdateAcknowledge_PDU = -1;  /* MMEConfigurationUpdateAcknowledge */
static int hf_s1ap_MMEConfigurationUpdateFailure_PDU = -1;  /* MMEConfigurationUpdateFailure */
static int hf_s1ap_DownlinkS1cdma2000tunnelling_PDU = -1;  /* DownlinkS1cdma2000tunnelling */
static int hf_s1ap_UplinkS1cdma2000tunnelling_PDU = -1;  /* UplinkS1cdma2000tunnelling */
static int hf_s1ap_UECapabilityInfoIndication_PDU = -1;  /* UECapabilityInfoIndication */
static int hf_s1ap_ENBStatusTransfer_PDU = -1;    /* ENBStatusTransfer */
static int hf_s1ap_MMEStatusTransfer_PDU = -1;    /* MMEStatusTransfer */
static int hf_s1ap_TraceStart_PDU = -1;           /* TraceStart */
static int hf_s1ap_TraceFailureIndication_PDU = -1;  /* TraceFailureIndication */
static int hf_s1ap_DeactivateTrace_PDU = -1;      /* DeactivateTrace */
static int hf_s1ap_CellTrafficTrace_PDU = -1;     /* CellTrafficTrace */
static int hf_s1ap_LocationReportingControl_PDU = -1;  /* LocationReportingControl */
static int hf_s1ap_LocationReportingFailureIndication_PDU = -1;  /* LocationReportingFailureIndication */
static int hf_s1ap_LocationReport_PDU = -1;       /* LocationReport */
static int hf_s1ap_OverloadStart_PDU = -1;        /* OverloadStart */
static int hf_s1ap_OverloadStop_PDU = -1;         /* OverloadStop */
static int hf_s1ap_WriteReplaceWarningRequest_PDU = -1;  /* WriteReplaceWarningRequest */
static int hf_s1ap_WriteReplaceWarningResponse_PDU = -1;  /* WriteReplaceWarningResponse */
static int hf_s1ap_ENBDirectInformationTransfer_PDU = -1;  /* ENBDirectInformationTransfer */
static int hf_s1ap_Inter_SystemInformationTransferType_PDU = -1;  /* Inter_SystemInformationTransferType */
static int hf_s1ap_MMEDirectInformationTransfer_PDU = -1;  /* MMEDirectInformationTransfer */
static int hf_s1ap_ENBConfigurationTransfer_PDU = -1;  /* ENBConfigurationTransfer */
static int hf_s1ap_MMEConfigurationTransfer_PDU = -1;  /* MMEConfigurationTransfer */
static int hf_s1ap_PrivateMessage_PDU = -1;       /* PrivateMessage */
static int hf_s1ap_KillRequest_PDU = -1;          /* KillRequest */
static int hf_s1ap_KillResponse_PDU = -1;         /* KillResponse */
static int hf_s1ap_PWSRestartIndication_PDU = -1;  /* PWSRestartIndication */
static int hf_s1ap_PWSFailureIndication_PDU = -1;  /* PWSFailureIndication */
static int hf_s1ap_DownlinkUEAssociatedLPPaTransport_PDU = -1;  /* DownlinkUEAssociatedLPPaTransport */
static int hf_s1ap_UplinkUEAssociatedLPPaTransport_PDU = -1;  /* UplinkUEAssociatedLPPaTransport */
static int hf_s1ap_DownlinkNonUEAssociatedLPPaTransport_PDU = -1;  /* DownlinkNonUEAssociatedLPPaTransport */
static int hf_s1ap_UplinkNonUEAssociatedLPPaTransport_PDU = -1;  /* UplinkNonUEAssociatedLPPaTransport */
static int hf_s1ap_E_RABModificationIndication_PDU = -1;  /* E_RABModificationIndication */
static int hf_s1ap_E_RABToBeModifiedListBearerModInd_PDU = -1;  /* E_RABToBeModifiedListBearerModInd */
static int hf_s1ap_E_RABToBeModifiedItemBearerModInd_PDU = -1;  /* E_RABToBeModifiedItemBearerModInd */
static int hf_s1ap_E_RABNotToBeModifiedListBearerModInd_PDU = -1;  /* E_RABNotToBeModifiedListBearerModInd */
static int hf_s1ap_E_RABNotToBeModifiedItemBearerModInd_PDU = -1;  /* E_RABNotToBeModifiedItemBearerModInd */
static int hf_s1ap_CSGMembershipInfo_PDU = -1;    /* CSGMembershipInfo */
static int hf_s1ap_E_RABModificationConfirm_PDU = -1;  /* E_RABModificationConfirm */
static int hf_s1ap_E_RABModifyListBearerModConf_PDU = -1;  /* E_RABModifyListBearerModConf */
static int hf_s1ap_E_RABModifyItemBearerModConf_PDU = -1;  /* E_RABModifyItemBearerModConf */
static int hf_s1ap_UEContextModificationIndication_PDU = -1;  /* UEContextModificationIndication */
static int hf_s1ap_UEContextModificationConfirm_PDU = -1;  /* UEContextModificationConfirm */
static int hf_s1ap_S1AP_PDU_PDU = -1;             /* S1AP_PDU */
static int hf_s1ap_s1ap_SONtransferApplicationIdentity_PDU = -1;  /* SONtransferApplicationIdentity */
static int hf_s1ap_s1ap_SONtransferRequestContainer_PDU = -1;  /* SONtransferRequestContainer */
static int hf_s1ap_s1ap_SONtransferResponseContainer_PDU = -1;  /* SONtransferResponseContainer */
static int hf_s1ap_s1ap_SONtransferCause_PDU = -1;  /* SONtransferCause */
static int hf_s1ap_local = -1;                    /* INTEGER_0_65535 */
static int hf_s1ap_global = -1;                   /* T_global */
static int hf_s1ap_ProtocolIE_Container_item = -1;  /* ProtocolIE_Field */
static int hf_s1ap_id = -1;                       /* ProtocolIE_ID */
static int hf_s1ap_criticality = -1;              /* Criticality */
static int hf_s1ap_ie_field_value = -1;           /* T_ie_field_value */
static int hf_s1ap_ProtocolIE_ContainerList_item = -1;  /* ProtocolIE_SingleContainer */
static int hf_s1ap_ProtocolExtensionContainer_item = -1;  /* ProtocolExtensionField */
static int hf_s1ap_ext_id = -1;                   /* ProtocolExtensionID */
static int hf_s1ap_extensionValue = -1;           /* T_extensionValue */
static int hf_s1ap_PrivateIE_Container_item = -1;  /* PrivateIE_Field */
static int hf_s1ap_private_id = -1;               /* PrivateIE_ID */
static int hf_s1ap_value = -1;                    /* T_value */
static int hf_s1ap_gUMMEI = -1;                   /* GUMMEI */
static int hf_s1ap_m_TMSI = -1;                   /* M_TMSI */
static int hf_s1ap_iE_Extensions = -1;            /* ProtocolExtensionContainer */
static int hf_s1ap_cellBased = -1;                /* CellBasedMDT */
static int hf_s1ap_tABased = -1;                  /* TABasedMDT */
static int hf_s1ap_pLMNWide = -1;                 /* NULL */
static int hf_s1ap_tAIBased = -1;                 /* TAIBasedMDT */
static int hf_s1ap_priorityLevel = -1;            /* PriorityLevel */
static int hf_s1ap_pre_emptionCapability = -1;    /* Pre_emptionCapability */
static int hf_s1ap_pre_emptionVulnerability = -1;  /* Pre_emptionVulnerability */
static int hf_s1ap_Bearers_SubjectToStatusTransferList_item = -1;  /* ProtocolIE_SingleContainer */
static int hf_s1ap_e_RAB_ID = -1;                 /* E_RAB_ID */
static int hf_s1ap_uL_COUNTvalue = -1;            /* COUNTvalue */
static int hf_s1ap_dL_COUNTvalue = -1;            /* COUNTvalue */
static int hf_s1ap_receiveStatusofULPDCPSDUs = -1;  /* ReceiveStatusofULPDCPSDUs */
static int hf_s1ap_BPLMNs_item = -1;              /* PLMNidentity */
static int hf_s1ap_cellID_Cancelled = -1;         /* CellID_Cancelled */
static int hf_s1ap_tAI_Cancelled = -1;            /* TAI_Cancelled */
static int hf_s1ap_emergencyAreaID_Cancelled = -1;  /* EmergencyAreaID_Cancelled */
static int hf_s1ap_cellID_Broadcast = -1;         /* CellID_Broadcast */
static int hf_s1ap_tAI_Broadcast = -1;            /* TAI_Broadcast */
static int hf_s1ap_emergencyAreaID_Broadcast = -1;  /* EmergencyAreaID_Broadcast */
static int hf_s1ap_CancelledCellinEAI_item = -1;  /* CancelledCellinEAI_Item */
static int hf_s1ap_eCGI = -1;                     /* EUTRAN_CGI */
static int hf_s1ap_numberOfBroadcasts = -1;       /* NumberOfBroadcasts */
static int hf_s1ap_CancelledCellinTAI_item = -1;  /* CancelledCellinTAI_Item */
static int hf_s1ap_radioNetwork = -1;             /* CauseRadioNetwork */
static int hf_s1ap_transport = -1;                /* CauseTransport */
static int hf_s1ap_nas = -1;                      /* CauseNas */
static int hf_s1ap_protocol = -1;                 /* CauseProtocol */
static int hf_s1ap_misc = -1;                     /* CauseMisc */
static int hf_s1ap_CellID_Broadcast_item = -1;    /* CellID_Broadcast_Item */
static int hf_s1ap_CellID_Cancelled_item = -1;    /* CellID_Cancelled_Item */
static int hf_s1ap_cellIdListforMDT = -1;         /* CellIdListforMDT */
static int hf_s1ap_CellIdListforMDT_item = -1;    /* EUTRAN_CGI */
static int hf_s1ap_cdma2000OneXMEID = -1;         /* Cdma2000OneXMEID */
static int hf_s1ap_cdma2000OneXMSI = -1;          /* Cdma2000OneXMSI */
static int hf_s1ap_cdma2000OneXPilot = -1;        /* Cdma2000OneXPilot */
static int hf_s1ap_cell_Size = -1;                /* Cell_Size */
static int hf_s1ap_pLMNidentity = -1;             /* PLMNidentity */
static int hf_s1ap_lAC = -1;                      /* LAC */
static int hf_s1ap_cI = -1;                       /* CI */
static int hf_s1ap_rAC = -1;                      /* RAC */
static int hf_s1ap_CSG_IdList_item = -1;          /* CSG_IdList_Item */
static int hf_s1ap_cSG_Id = -1;                   /* CSG_Id */
static int hf_s1ap_pDCP_SN = -1;                  /* PDCP_SN */
static int hf_s1ap_hFN = -1;                      /* HFN */
static int hf_s1ap_pDCP_SNExtended = -1;          /* PDCP_SNExtended */
static int hf_s1ap_hFNModified = -1;              /* HFNModified */
static int hf_s1ap_pDCP_SNlength18 = -1;          /* PDCP_SNlength18 */
static int hf_s1ap_hFNforPDCP_SNlength18 = -1;    /* HFNforPDCP_SNlength18 */
static int hf_s1ap_procedureCode = -1;            /* ProcedureCode */
static int hf_s1ap_triggeringMessage = -1;        /* TriggeringMessage */
static int hf_s1ap_procedureCriticality = -1;     /* Criticality */
static int hf_s1ap_iEsCriticalityDiagnostics = -1;  /* CriticalityDiagnostics_IE_List */
static int hf_s1ap_CriticalityDiagnostics_IE_List_item = -1;  /* CriticalityDiagnostics_IE_Item */
static int hf_s1ap_iECriticality = -1;            /* Criticality */
static int hf_s1ap_iE_ID = -1;                    /* ProtocolIE_ID */
static int hf_s1ap_typeOfError = -1;              /* TypeOfError */
static int hf_s1ap_ECGIList_item = -1;            /* EUTRAN_CGI */
static int hf_s1ap_PWSfailedECGIList_item = -1;   /* EUTRAN_CGI */
static int hf_s1ap_EmergencyAreaIDList_item = -1;  /* EmergencyAreaID */
static int hf_s1ap_EmergencyAreaID_Broadcast_item = -1;  /* EmergencyAreaID_Broadcast_Item */
static int hf_s1ap_emergencyAreaID = -1;          /* EmergencyAreaID */
static int hf_s1ap_completedCellinEAI = -1;       /* CompletedCellinEAI */
static int hf_s1ap_EmergencyAreaID_Cancelled_item = -1;  /* EmergencyAreaID_Cancelled_Item */
static int hf_s1ap_cancelledCellinEAI = -1;       /* CancelledCellinEAI */
static int hf_s1ap_CompletedCellinEAI_item = -1;  /* CompletedCellinEAI_Item */
static int hf_s1ap_ECGI_List_item = -1;           /* EUTRAN_CGI */
static int hf_s1ap_EmergencyAreaIDListForRestart_item = -1;  /* EmergencyAreaID */
static int hf_s1ap_macroENB_ID = -1;              /* BIT_STRING_SIZE_20 */
static int hf_s1ap_homeENB_ID = -1;               /* BIT_STRING_SIZE_28 */
static int hf_s1ap_lAI = -1;                      /* LAI */
static int hf_s1ap_eNB_ID = -1;                   /* ENB_ID */
static int hf_s1ap_GUMMEIList_item = -1;          /* GUMMEI */
static int hf_s1ap_bearers_SubjectToStatusTransferList = -1;  /* Bearers_SubjectToStatusTransferList */
static int hf_s1ap_ENBX2TLAs_item = -1;           /* TransportLayerAddress */
static int hf_s1ap_EPLMNs_item = -1;              /* PLMNidentity */
static int hf_s1ap_E_RABInformationList_item = -1;  /* ProtocolIE_SingleContainer */
static int hf_s1ap_dL_Forwarding = -1;            /* DL_Forwarding */
static int hf_s1ap_E_RABList_item = -1;           /* ProtocolIE_SingleContainer */
static int hf_s1ap_cause = -1;                    /* Cause */
static int hf_s1ap_qCI = -1;                      /* QCI */
static int hf_s1ap_allocationRetentionPriority = -1;  /* AllocationAndRetentionPriority */
static int hf_s1ap_gbrQosInformation = -1;        /* GBR_QosInformation */
static int hf_s1ap_cell_ID = -1;                  /* CellIdentity */
static int hf_s1ap_expectedActivity = -1;         /* ExpectedUEActivityBehaviour */
static int hf_s1ap_expectedHOInterval = -1;       /* ExpectedHOInterval */
static int hf_s1ap_expectedActivityPeriod = -1;   /* ExpectedActivityPeriod */
static int hf_s1ap_expectedIdlePeriod = -1;       /* ExpectedIdlePeriod */
static int hf_s1ap_sourceofUEActivityBehaviourInformation = -1;  /* SourceOfUEActivityBehaviourInformation */
static int hf_s1ap_ForbiddenTAs_item = -1;        /* ForbiddenTAs_Item */
static int hf_s1ap_pLMN_Identity = -1;            /* PLMNidentity */
static int hf_s1ap_forbiddenTACs = -1;            /* ForbiddenTACs */
static int hf_s1ap_ForbiddenTACs_item = -1;       /* TAC */
static int hf_s1ap_ForbiddenLAs_item = -1;        /* ForbiddenLAs_Item */
static int hf_s1ap_forbiddenLACs = -1;            /* ForbiddenLACs */
static int hf_s1ap_ForbiddenLACs_item = -1;       /* LAC */
static int hf_s1ap_e_RAB_MaximumBitrateDL = -1;   /* BitRate */
static int hf_s1ap_e_RAB_MaximumBitrateUL = -1;   /* BitRate */
static int hf_s1ap_e_RAB_GuaranteedBitrateDL = -1;  /* BitRate */
static int hf_s1ap_e_RAB_GuaranteedBitrateUL = -1;  /* BitRate */
static int hf_s1ap_mME_Group_ID = -1;             /* MME_Group_ID */
static int hf_s1ap_mME_Code = -1;                 /* MME_Code */
static int hf_s1ap_servingPLMN = -1;              /* PLMNidentity */
static int hf_s1ap_equivalentPLMNs = -1;          /* EPLMNs */
static int hf_s1ap_forbiddenTAs = -1;             /* ForbiddenTAs */
static int hf_s1ap_forbiddenLAs = -1;             /* ForbiddenLAs */
static int hf_s1ap_forbiddenInterRATs = -1;       /* ForbiddenInterRATs */
static int hf_s1ap_measurementsToActivate = -1;   /* MeasurementsToActivate */
static int hf_s1ap_m1reportingTrigger = -1;       /* M1ReportingTrigger */
static int hf_s1ap_m1thresholdeventA2 = -1;       /* M1ThresholdEventA2 */
static int hf_s1ap_m1periodicReporting = -1;      /* M1PeriodicReporting */
static int hf_s1ap_e_UTRAN_Cell = -1;             /* LastVisitedEUTRANCellInformation */
static int hf_s1ap_uTRAN_Cell = -1;               /* LastVisitedUTRANCellInformation */
static int hf_s1ap_gERAN_Cell = -1;               /* LastVisitedGERANCellInformation */
static int hf_s1ap_global_Cell_ID = -1;           /* EUTRAN_CGI */
static int hf_s1ap_cellType = -1;                 /* CellType */
static int hf_s1ap_time_UE_StayedInCell = -1;     /* Time_UE_StayedInCell */
static int hf_s1ap_undefined = -1;                /* NULL */
static int hf_s1ap_pattern_period = -1;           /* T_pattern_period */
static int hf_s1ap_pattern_offset = -1;           /* INTEGER_0_10239_ */
static int hf_s1ap_loggingInterval = -1;          /* LoggingInterval */
static int hf_s1ap_loggingDuration = -1;          /* LoggingDuration */
static int hf_s1ap_mBSFN_ResultToLog = -1;        /* MBSFN_ResultToLog */
static int hf_s1ap_m3period = -1;                 /* M3period */
static int hf_s1ap_m4period = -1;                 /* M4period */
static int hf_s1ap_m4_links_to_log = -1;          /* Links_to_log */
static int hf_s1ap_m5period = -1;                 /* M5period */
static int hf_s1ap_m5_links_to_log = -1;          /* Links_to_log */
static int hf_s1ap_m6report_Interval = -1;        /* M6report_Interval */
static int hf_s1ap_m6delay_threshold = -1;        /* M6delay_threshold */
static int hf_s1ap_m6_links_to_log = -1;          /* Links_to_log */
static int hf_s1ap_m7period = -1;                 /* M7period */
static int hf_s1ap_m7_links_to_log = -1;          /* Links_to_log */
static int hf_s1ap_mdt_Activation = -1;           /* MDT_Activation */
static int hf_s1ap_areaScopeOfMDT = -1;           /* AreaScopeOfMDT */
static int hf_s1ap_mDTMode = -1;                  /* MDTMode */
static int hf_s1ap_MBSFN_ResultToLog_item = -1;   /* MBSFN_ResultToLogInfo */
static int hf_s1ap_mBSFN_AreaId = -1;             /* INTEGER_0_255 */
static int hf_s1ap_carrierFreq = -1;              /* EARFCN */
static int hf_s1ap_MDTPLMNList_item = -1;         /* PLMNidentity */
static int hf_s1ap_immediateMDT = -1;             /* ImmediateMDT */
static int hf_s1ap_loggedMDT = -1;                /* LoggedMDT */
static int hf_s1ap_mDTMode_Extension = -1;        /* MDTMode_Extension */
static int hf_s1ap_threshold_RSRP = -1;           /* Threshold_RSRP */
static int hf_s1ap_threshold_RSRQ = -1;           /* Threshold_RSRQ */
static int hf_s1ap_muting_pattern_period = -1;    /* T_muting_pattern_period */
static int hf_s1ap_muting_pattern_offset = -1;    /* INTEGER_0_10239_ */
static int hf_s1ap_overloadAction = -1;           /* OverloadAction */
static int hf_s1ap_reportInterval = -1;           /* ReportIntervalMDT */
static int hf_s1ap_reportAmount = -1;             /* ReportAmountMDT */
static int hf_s1ap_proSeDirectDiscovery = -1;     /* ProSeDirectDiscovery */
static int hf_s1ap_proSeDirectCommunication = -1;  /* ProSeDirectCommunication */
static int hf_s1ap_eventType = -1;                /* EventType */
static int hf_s1ap_reportArea = -1;               /* ReportArea */
static int hf_s1ap_rIMInformation = -1;           /* RIMInformation */
static int hf_s1ap_rIMRoutingAddress = -1;        /* RIMRoutingAddress */
static int hf_s1ap_gERAN_Cell_ID = -1;            /* GERAN_Cell_ID */
static int hf_s1ap_targetRNC_ID = -1;             /* TargetRNC_ID */
static int hf_s1ap_eHRPD_Sector_ID = -1;          /* OCTET_STRING_SIZE_16 */
static int hf_s1ap_uE_RLF_Report_Container = -1;  /* UE_RLF_Report_Container */
static int hf_s1ap_uE_RLF_Report_Container_for_extended_bands = -1;  /* UE_RLF_Report_Container_for_extended_bands */
static int hf_s1ap_ECGIListForRestart_item = -1;  /* EUTRAN_CGI */
static int hf_s1ap_nextHopChainingCount = -1;     /* INTEGER_0_7 */
static int hf_s1ap_nextHopParameter = -1;         /* SecurityKey */
static int hf_s1ap_sONInformationRequest = -1;    /* SONInformationRequest */
static int hf_s1ap_sONInformationReply = -1;      /* SONInformationReply */
static int hf_s1ap_sONInformation_Extension = -1;  /* SONInformation_Extension */
static int hf_s1ap_x2TNLConfigurationInfo = -1;   /* X2TNLConfigurationInfo */
static int hf_s1ap_rLFReportInformation = -1;     /* RLFReportInformation */
static int hf_s1ap_targeteNB_ID = -1;             /* TargeteNB_ID */
static int hf_s1ap_sourceeNB_ID = -1;             /* SourceeNB_ID */
static int hf_s1ap_sONInformation = -1;           /* SONInformation */
static int hf_s1ap_sourceStratumLevel = -1;       /* StratumLevel */
static int hf_s1ap_listeningSubframePattern = -1;  /* ListeningSubframePattern */
static int hf_s1ap_aggressoreCGI_List = -1;       /* ECGI_List */
static int hf_s1ap_global_ENB_ID = -1;            /* Global_ENB_ID */
static int hf_s1ap_selected_TAI = -1;             /* TAI */
static int hf_s1ap_rRC_Container = -1;            /* RRC_Container */
static int hf_s1ap_e_RABInformationList = -1;     /* E_RABInformationList */
static int hf_s1ap_targetCell_ID = -1;            /* EUTRAN_CGI */
static int hf_s1ap_subscriberProfileIDforRFP = -1;  /* SubscriberProfileIDforRFP */
static int hf_s1ap_uE_HistoryInformation = -1;    /* UE_HistoryInformation */
static int hf_s1ap_ServedGUMMEIs_item = -1;       /* ServedGUMMEIsItem */
static int hf_s1ap_servedPLMNs = -1;              /* ServedPLMNs */
static int hf_s1ap_servedGroupIDs = -1;           /* ServedGroupIDs */
static int hf_s1ap_servedMMECs = -1;              /* ServedMMECs */
static int hf_s1ap_ServedGroupIDs_item = -1;      /* MME_Group_ID */
static int hf_s1ap_ServedMMECs_item = -1;         /* MME_Code */
static int hf_s1ap_ServedPLMNs_item = -1;         /* PLMNidentity */
static int hf_s1ap_SupportedTAs_item = -1;        /* SupportedTAs_Item */
static int hf_s1ap_tAC = -1;                      /* TAC */
static int hf_s1ap_broadcastPLMNs = -1;           /* BPLMNs */
static int hf_s1ap_stratumLevel = -1;             /* StratumLevel */
static int hf_s1ap_synchronisationStatus = -1;    /* SynchronisationStatus */
static int hf_s1ap_mMEC = -1;                     /* MME_Code */
static int hf_s1ap_tAIListforMDT = -1;            /* TAIListforMDT */
static int hf_s1ap_TAIListforMDT_item = -1;       /* TAI */
static int hf_s1ap_TAIListforWarning_item = -1;   /* TAI */
static int hf_s1ap_TAI_Broadcast_item = -1;       /* TAI_Broadcast_Item */
static int hf_s1ap_tAI = -1;                      /* TAI */
static int hf_s1ap_completedCellinTAI = -1;       /* CompletedCellinTAI */
static int hf_s1ap_TAI_Cancelled_item = -1;       /* TAI_Cancelled_Item */
static int hf_s1ap_cancelledCellinTAI = -1;       /* CancelledCellinTAI */
static int hf_s1ap_tAListforMDT = -1;             /* TAListforMDT */
static int hf_s1ap_TAListforMDT_item = -1;        /* TAC */
static int hf_s1ap_CompletedCellinTAI_item = -1;  /* CompletedCellinTAI_Item */
static int hf_s1ap_cGI = -1;                      /* CGI */
static int hf_s1ap_rNC_ID = -1;                   /* RNC_ID */
static int hf_s1ap_extendedRNC_ID = -1;           /* ExtendedRNC_ID */
static int hf_s1ap_measurementThreshold = -1;     /* MeasurementThresholdA2 */
static int hf_s1ap_transportLayerAddress = -1;    /* TransportLayerAddress */
static int hf_s1ap_uL_GTP_TEID = -1;              /* GTP_TEID */
static int hf_s1ap_e_UTRAN_Trace_ID = -1;         /* E_UTRAN_Trace_ID */
static int hf_s1ap_interfacesToTrace = -1;        /* InterfacesToTrace */
static int hf_s1ap_traceDepth = -1;               /* TraceDepth */
static int hf_s1ap_traceCollectionEntityIPAddress = -1;  /* TransportLayerAddress */
static int hf_s1ap_uDP_Port_Number = -1;          /* Port_Number */
static int hf_s1ap_TAIListForRestart_item = -1;   /* TAI */
static int hf_s1ap_uEaggregateMaximumBitRateDL = -1;  /* BitRate */
static int hf_s1ap_uEaggregateMaximumBitRateUL = -1;  /* BitRate */
static int hf_s1ap_uE_S1AP_ID_pair = -1;          /* UE_S1AP_ID_pair */
static int hf_s1ap_mME_UE_S1AP_ID = -1;           /* MME_UE_S1AP_ID */
static int hf_s1ap_eNB_UE_S1AP_ID = -1;           /* ENB_UE_S1AP_ID */
static int hf_s1ap_UE_HistoryInformation_item = -1;  /* LastVisitedCell_Item */
static int hf_s1ap_s_TMSI = -1;                   /* S_TMSI */
static int hf_s1ap_iMSI = -1;                     /* IMSI */
static int hf_s1ap_encryptionAlgorithms = -1;     /* EncryptionAlgorithms */
static int hf_s1ap_integrityProtectionAlgorithms = -1;  /* IntegrityProtectionAlgorithms */
static int hf_s1ap_eutran_cgi = -1;               /* EUTRAN_CGI */
static int hf_s1ap_tai = -1;                      /* TAI */
static int hf_s1ap_cellIDList = -1;               /* ECGIList */
static int hf_s1ap_trackingAreaListforWarning = -1;  /* TAIListforWarning */
static int hf_s1ap_emergencyAreaIDList = -1;      /* EmergencyAreaIDList */
static int hf_s1ap_eNBX2TransportLayerAddresses = -1;  /* ENBX2TLAs */
static int hf_s1ap_ENBX2ExtTLAs_item = -1;        /* ENBX2ExtTLA */
static int hf_s1ap_iPsecTLA = -1;                 /* TransportLayerAddress */
static int hf_s1ap_gTPTLAa = -1;                  /* ENBX2GTPTLAs */
static int hf_s1ap_ENBX2GTPTLAs_item = -1;        /* TransportLayerAddress */
static int hf_s1ap_ENBIndirectX2TransportLayerAddresses_item = -1;  /* TransportLayerAddress */
static int hf_s1ap_protocolIEs = -1;              /* ProtocolIE_Container */
static int hf_s1ap_dL_transportLayerAddress = -1;  /* TransportLayerAddress */
static int hf_s1ap_dL_gTP_TEID = -1;              /* GTP_TEID */
static int hf_s1ap_uL_TransportLayerAddress = -1;  /* TransportLayerAddress */
static int hf_s1ap_gTP_TEID = -1;                 /* GTP_TEID */
static int hf_s1ap_e_RABlevelQosParameters = -1;  /* E_RABLevelQoSParameters */
static int hf_s1ap_E_RABToBeSetupListBearerSUReq_item = -1;  /* ProtocolIE_SingleContainer */
static int hf_s1ap_e_RABlevelQoSParameters = -1;  /* E_RABLevelQoSParameters */
static int hf_s1ap_nAS_PDU = -1;                  /* NAS_PDU */
static int hf_s1ap_E_RABSetupListBearerSURes_item = -1;  /* ProtocolIE_SingleContainer */
static int hf_s1ap_E_RABToBeModifiedListBearerModReq_item = -1;  /* ProtocolIE_SingleContainer */
static int hf_s1ap_e_RABLevelQoSParameters = -1;  /* E_RABLevelQoSParameters */
static int hf_s1ap_E_RABModifyListBearerModRes_item = -1;  /* ProtocolIE_SingleContainer */
static int hf_s1ap_E_RABReleaseListBearerRelComp_item = -1;  /* ProtocolIE_SingleContainer */
static int hf_s1ap_E_RABToBeSetupListCtxtSUReq_item = -1;  /* ProtocolIE_SingleContainer */
static int hf_s1ap_E_RABSetupListCtxtSURes_item = -1;  /* ProtocolIE_SingleContainer */
static int hf_s1ap_TAIList_item = -1;             /* ProtocolIE_SingleContainer */
static int hf_s1ap_s1_Interface = -1;             /* ResetAll */
static int hf_s1ap_partOfS1_Interface = -1;       /* UE_associatedLogicalS1_ConnectionListRes */
static int hf_s1ap_UE_associatedLogicalS1_ConnectionListRes_item = -1;  /* ProtocolIE_SingleContainer */
static int hf_s1ap_UE_associatedLogicalS1_ConnectionListResAck_item = -1;  /* ProtocolIE_SingleContainer */
static int hf_s1ap_rIMTransfer = -1;              /* RIMTransfer */
static int hf_s1ap_privateIEs = -1;               /* PrivateIE_Container */
static int hf_s1ap_dL_GTP_TEID = -1;              /* GTP_TEID */
static int hf_s1ap_cSGMembershipStatus = -1;      /* CSGMembershipStatus */
static int hf_s1ap_cellAccessMode = -1;           /* CellAccessMode */
static int hf_s1ap_E_RABModifyListBearerModConf_item = -1;  /* ProtocolIE_SingleContainer */
static int hf_s1ap_initiatingMessage = -1;        /* InitiatingMessage */
static int hf_s1ap_successfulOutcome = -1;        /* SuccessfulOutcome */
static int hf_s1ap_unsuccessfulOutcome = -1;      /* UnsuccessfulOutcome */
static int hf_s1ap_initiatingMessagevalue = -1;   /* InitiatingMessage_value */
static int hf_s1ap_successfulOutcome_value = -1;  /* SuccessfulOutcome_value */
static int hf_s1ap_unsuccessfulOutcome_value = -1;  /* UnsuccessfulOutcome_value */
static int hf_s1ap_cellLoadReporting = -1;        /* NULL */
static int hf_s1ap_multiCellLoadReporting = -1;   /* MultiCellLoadReportingRequest */
static int hf_s1ap_eventTriggeredCellLoadReporting = -1;  /* EventTriggeredCellLoadReportingRequest */
static int hf_s1ap_hOReporting = -1;              /* HOReport */
static int hf_s1ap_eutranCellActivation = -1;     /* CellActivationRequest */
static int hf_s1ap_energySavingsIndication = -1;  /* CellStateIndication */
static int hf_s1ap_failureEventReporting = -1;    /* FailureEventReport */
static int hf_s1ap_cellLoadReporting_01 = -1;     /* CellLoadReportingResponse */
static int hf_s1ap_multiCellLoadReporting_01 = -1;  /* MultiCellLoadReportingResponse */
static int hf_s1ap_eventTriggeredCellLoadReporting_01 = -1;  /* EventTriggeredCellLoadReportingResponse */
static int hf_s1ap_hOReporting_01 = -1;           /* NULL */
static int hf_s1ap_eutranCellActivation_01 = -1;  /* CellActivationResponse */
static int hf_s1ap_energySavingsIndication_01 = -1;  /* NULL */
static int hf_s1ap_failureEventReporting_01 = -1;  /* NULL */
static int hf_s1ap_cellLoadReporting_02 = -1;     /* CellLoadReportingCause */
static int hf_s1ap_multiCellLoadReporting_02 = -1;  /* CellLoadReportingCause */
static int hf_s1ap_eventTriggeredCellLoadReporting_02 = -1;  /* CellLoadReportingCause */
static int hf_s1ap_hOReporting_02 = -1;           /* HOReportingCause */
static int hf_s1ap_eutranCellActivation_02 = -1;  /* CellActivationCause */
static int hf_s1ap_energySavingsIndication_02 = -1;  /* CellStateIndicationCause */
static int hf_s1ap_failureEventReporting_02 = -1;  /* FailureEventReportingCause */
static int hf_s1ap_eUTRAN = -1;                   /* EUTRANcellLoadReportingResponse */
static int hf_s1ap_uTRAN = -1;                    /* OCTET_STRING */
static int hf_s1ap_gERAN = -1;                    /* OCTET_STRING */
static int hf_s1ap_eHRPD = -1;                    /* EHRPDSectorLoadReportingResponse */
static int hf_s1ap_compositeAvailableCapacityGroup = -1;  /* CompositeAvailableCapacityGroup */
static int hf_s1ap_cell_ID_01 = -1;               /* OCTET_STRING */
static int hf_s1ap_eUTRANcellLoadReportingResponse = -1;  /* EUTRANcellLoadReportingResponse */
static int hf_s1ap_eUTRAN_01 = -1;                /* OCTET_STRING */
static int hf_s1ap_eHRPD_01 = -1;                 /* EHRPD_Sector_ID */
static int hf_s1ap_RequestedCellList_item = -1;   /* IRAT_Cell_ID */
static int hf_s1ap_requestedCellList = -1;        /* RequestedCellList */
static int hf_s1ap_cell_ID_02 = -1;               /* IRAT_Cell_ID */
static int hf_s1ap_ReportingCellList_item = -1;   /* ReportingCellList_Item */
static int hf_s1ap_MultiCellLoadReportingResponse_item = -1;  /* MultiCellLoadReportingResponse_Item */
static int hf_s1ap_eUTRANResponse = -1;           /* EUTRANResponse */
static int hf_s1ap_uTRANResponse = -1;            /* OCTET_STRING */
static int hf_s1ap_gERANResponse = -1;            /* OCTET_STRING */
static int hf_s1ap_eHRPD_02 = -1;                 /* EHRPDMultiSectorLoadReportingResponseItem */
static int hf_s1ap_numberOfMeasurementReportingLevels = -1;  /* NumberOfMeasurementReportingLevels */
static int hf_s1ap_cellLoadReportingResponse = -1;  /* CellLoadReportingResponse */
static int hf_s1ap_overloadFlag = -1;             /* OverloadFlag */
static int hf_s1ap_hoType = -1;                   /* HoType */
static int hf_s1ap_hoReportType = -1;             /* HoReportType */
static int hf_s1ap_hosourceID = -1;               /* IRAT_Cell_ID */
static int hf_s1ap_hoTargetID = -1;               /* IRAT_Cell_ID */
static int hf_s1ap_candidateCellList = -1;        /* CandidateCellList */
static int hf_s1ap_candidatePCIList = -1;         /* CandidatePCIList */
static int hf_s1ap_CandidateCellList_item = -1;   /* IRAT_Cell_ID */
static int hf_s1ap_CandidatePCIList_item = -1;    /* CandidatePCI */
static int hf_s1ap_pCI = -1;                      /* INTEGER_0_503 */
static int hf_s1ap_eARFCN = -1;                   /* OCTET_STRING */
static int hf_s1ap_cellsToActivateList = -1;      /* CellsToActivateList */
static int hf_s1ap_minimumActivationTime = -1;    /* INTEGER_1_60 */
static int hf_s1ap_CellsToActivateList_item = -1;  /* CellsToActivateList_Item */
static int hf_s1ap_activatedCellsList = -1;       /* ActivatedCellsList */
static int hf_s1ap_ActivatedCellsList_item = -1;  /* ActivatedCellsList_Item */
static int hf_s1ap_notificationCellList = -1;     /* NotificationCellList */
static int hf_s1ap_NotificationCellList_item = -1;  /* NotificationCellList_Item */
static int hf_s1ap_notifyFlag = -1;               /* NotifyFlag */
static int hf_s1ap_tooEarlyInterRATHOReportFromEUTRAN = -1;  /* TooEarlyInterRATHOReportReportFromEUTRAN */
static int hf_s1ap_uERLFReportContainer = -1;     /* OCTET_STRING */
static int hf_s1ap_mobilityInformation = -1;      /* MobilityInformation */
static int hf_s1ap_dL_EHRPD_CompositeAvailableCapacity = -1;  /* EHRPDCompositeAvailableCapacity */
static int hf_s1ap_uL_EHRPD_CompositeAvailableCapacity = -1;  /* EHRPDCompositeAvailableCapacity */
static int hf_s1ap_eHRPDSectorCapacityClassValue = -1;  /* EHRPDSectorCapacityClassValue */
static int hf_s1ap_eHRPDCapacityValue = -1;       /* EHRPDCapacityValue */
static int hf_s1ap_eHRPD_Sector_ID_01 = -1;       /* EHRPD_Sector_ID */
static int hf_s1ap_eHRPDSectorLoadReportingResponse = -1;  /* EHRPDSectorLoadReportingResponse */

/*--- End of included file: packet-s1ap-hf.c ---*/
#line 74 "./asn1/s1ap/packet-s1ap-template.c"

/* Initialize the subtree pointers */
static int ett_s1ap = -1;
static int ett_s1ap_TransportLayerAddress = -1;
static int ett_s1ap_ToTargetTransparentContainer = -1;
static int ett_s1ap_ToSourceTransparentContainer = -1;
static int ett_s1ap_RRCContainer = -1;
static int ett_s1ap_UERadioCapability = -1;
static int ett_s1ap_RIMInformation = -1;
static int ett_s1ap_Cdma2000PDU = -1;
static int ett_s1ap_Cdma2000SectorID = -1;


/*--- Included file: packet-s1ap-ett.c ---*/
#line 1 "./asn1/s1ap/packet-s1ap-ett.c"
static gint ett_s1ap_PrivateIE_ID = -1;
static gint ett_s1ap_ProtocolIE_Container = -1;
static gint ett_s1ap_ProtocolIE_Field = -1;
static gint ett_s1ap_ProtocolIE_ContainerList = -1;
static gint ett_s1ap_ProtocolExtensionContainer = -1;
static gint ett_s1ap_ProtocolExtensionField = -1;
static gint ett_s1ap_PrivateIE_Container = -1;
static gint ett_s1ap_PrivateIE_Field = -1;
static gint ett_s1ap_Additional_GUTI = -1;
static gint ett_s1ap_AreaScopeOfMDT = -1;
static gint ett_s1ap_AllocationAndRetentionPriority = -1;
static gint ett_s1ap_Bearers_SubjectToStatusTransferList = -1;
static gint ett_s1ap_Bearers_SubjectToStatusTransfer_Item = -1;
static gint ett_s1ap_BPLMNs = -1;
static gint ett_s1ap_BroadcastCancelledAreaList = -1;
static gint ett_s1ap_BroadcastCompletedAreaList = -1;
static gint ett_s1ap_CancelledCellinEAI = -1;
static gint ett_s1ap_CancelledCellinEAI_Item = -1;
static gint ett_s1ap_CancelledCellinTAI = -1;
static gint ett_s1ap_CancelledCellinTAI_Item = -1;
static gint ett_s1ap_Cause = -1;
static gint ett_s1ap_CellID_Broadcast = -1;
static gint ett_s1ap_CellID_Broadcast_Item = -1;
static gint ett_s1ap_CellID_Cancelled = -1;
static gint ett_s1ap_CellID_Cancelled_Item = -1;
static gint ett_s1ap_CellBasedMDT = -1;
static gint ett_s1ap_CellIdListforMDT = -1;
static gint ett_s1ap_Cdma2000OneXSRVCCInfo = -1;
static gint ett_s1ap_CellType = -1;
static gint ett_s1ap_CGI = -1;
static gint ett_s1ap_CSG_IdList = -1;
static gint ett_s1ap_CSG_IdList_Item = -1;
static gint ett_s1ap_COUNTvalue = -1;
static gint ett_s1ap_COUNTValueExtended = -1;
static gint ett_s1ap_COUNTvaluePDCP_SNlength18 = -1;
static gint ett_s1ap_CriticalityDiagnostics = -1;
static gint ett_s1ap_CriticalityDiagnostics_IE_List = -1;
static gint ett_s1ap_CriticalityDiagnostics_IE_Item = -1;
static gint ett_s1ap_ECGIList = -1;
static gint ett_s1ap_PWSfailedECGIList = -1;
static gint ett_s1ap_EmergencyAreaIDList = -1;
static gint ett_s1ap_EmergencyAreaID_Broadcast = -1;
static gint ett_s1ap_EmergencyAreaID_Broadcast_Item = -1;
static gint ett_s1ap_EmergencyAreaID_Cancelled = -1;
static gint ett_s1ap_EmergencyAreaID_Cancelled_Item = -1;
static gint ett_s1ap_CompletedCellinEAI = -1;
static gint ett_s1ap_CompletedCellinEAI_Item = -1;
static gint ett_s1ap_ECGI_List = -1;
static gint ett_s1ap_EmergencyAreaIDListForRestart = -1;
static gint ett_s1ap_ENB_ID = -1;
static gint ett_s1ap_GERAN_Cell_ID = -1;
static gint ett_s1ap_Global_ENB_ID = -1;
static gint ett_s1ap_GUMMEIList = -1;
static gint ett_s1ap_ENB_StatusTransfer_TransparentContainer = -1;
static gint ett_s1ap_ENBX2TLAs = -1;
static gint ett_s1ap_EPLMNs = -1;
static gint ett_s1ap_E_RABInformationList = -1;
static gint ett_s1ap_E_RABInformationListItem = -1;
static gint ett_s1ap_E_RABList = -1;
static gint ett_s1ap_E_RABItem = -1;
static gint ett_s1ap_E_RABLevelQoSParameters = -1;
static gint ett_s1ap_EUTRAN_CGI = -1;
static gint ett_s1ap_ExpectedUEBehaviour = -1;
static gint ett_s1ap_ExpectedUEActivityBehaviour = -1;
static gint ett_s1ap_ForbiddenTAs = -1;
static gint ett_s1ap_ForbiddenTAs_Item = -1;
static gint ett_s1ap_ForbiddenTACs = -1;
static gint ett_s1ap_ForbiddenLAs = -1;
static gint ett_s1ap_ForbiddenLAs_Item = -1;
static gint ett_s1ap_ForbiddenLACs = -1;
static gint ett_s1ap_GBR_QosInformation = -1;
static gint ett_s1ap_GUMMEI = -1;
static gint ett_s1ap_HandoverRestrictionList = -1;
static gint ett_s1ap_ImmediateMDT = -1;
static gint ett_s1ap_LAI = -1;
static gint ett_s1ap_LastVisitedCell_Item = -1;
static gint ett_s1ap_LastVisitedEUTRANCellInformation = -1;
static gint ett_s1ap_LastVisitedGERANCellInformation = -1;
static gint ett_s1ap_ListeningSubframePattern = -1;
static gint ett_s1ap_LoggedMDT = -1;
static gint ett_s1ap_LoggedMBSFNMDT = -1;
static gint ett_s1ap_M3Configuration = -1;
static gint ett_s1ap_M4Configuration = -1;
static gint ett_s1ap_M5Configuration = -1;
static gint ett_s1ap_M6Configuration = -1;
static gint ett_s1ap_M7Configuration = -1;
static gint ett_s1ap_MDT_Configuration = -1;
static gint ett_s1ap_MBSFN_ResultToLog = -1;
static gint ett_s1ap_MBSFN_ResultToLogInfo = -1;
static gint ett_s1ap_MDTPLMNList = -1;
static gint ett_s1ap_MDTMode = -1;
static gint ett_s1ap_MeasurementThresholdA2 = -1;
static gint ett_s1ap_MutingPatternInformation = -1;
static gint ett_s1ap_OverloadResponse = -1;
static gint ett_s1ap_M1PeriodicReporting = -1;
static gint ett_s1ap_ProSeAuthorized = -1;
static gint ett_s1ap_RequestType = -1;
static gint ett_s1ap_RIMTransfer = -1;
static gint ett_s1ap_RIMRoutingAddress = -1;
static gint ett_s1ap_RLFReportInformation = -1;
static gint ett_s1ap_ECGIListForRestart = -1;
static gint ett_s1ap_SecurityContext = -1;
static gint ett_s1ap_SONInformation = -1;
static gint ett_s1ap_SONInformationReply = -1;
static gint ett_s1ap_SONInformationReport = -1;
static gint ett_s1ap_SONConfigurationTransfer = -1;
static gint ett_s1ap_SynchronisationInformation = -1;
static gint ett_s1ap_SourceeNB_ID = -1;
static gint ett_s1ap_SourceeNB_ToTargeteNB_TransparentContainer = -1;
static gint ett_s1ap_ServedGUMMEIs = -1;
static gint ett_s1ap_ServedGUMMEIsItem = -1;
static gint ett_s1ap_ServedGroupIDs = -1;
static gint ett_s1ap_ServedMMECs = -1;
static gint ett_s1ap_ServedPLMNs = -1;
static gint ett_s1ap_SupportedTAs = -1;
static gint ett_s1ap_SupportedTAs_Item = -1;
static gint ett_s1ap_TimeSynchronisationInfo = -1;
static gint ett_s1ap_S_TMSI = -1;
static gint ett_s1ap_TAIBasedMDT = -1;
static gint ett_s1ap_TAIListforMDT = -1;
static gint ett_s1ap_TAIListforWarning = -1;
static gint ett_s1ap_TAI = -1;
static gint ett_s1ap_TAI_Broadcast = -1;
static gint ett_s1ap_TAI_Broadcast_Item = -1;
static gint ett_s1ap_TAI_Cancelled = -1;
static gint ett_s1ap_TAI_Cancelled_Item = -1;
static gint ett_s1ap_TABasedMDT = -1;
static gint ett_s1ap_TAListforMDT = -1;
static gint ett_s1ap_CompletedCellinTAI = -1;
static gint ett_s1ap_CompletedCellinTAI_Item = -1;
static gint ett_s1ap_TargetID = -1;
static gint ett_s1ap_TargeteNB_ID = -1;
static gint ett_s1ap_TargetRNC_ID = -1;
static gint ett_s1ap_TargeteNB_ToSourceeNB_TransparentContainer = -1;
static gint ett_s1ap_M1ThresholdEventA2 = -1;
static gint ett_s1ap_TransportInformation = -1;
static gint ett_s1ap_TraceActivation = -1;
static gint ett_s1ap_TunnelInformation = -1;
static gint ett_s1ap_TAIListForRestart = -1;
static gint ett_s1ap_UEAggregateMaximumBitrate = -1;
static gint ett_s1ap_UE_S1AP_IDs = -1;
static gint ett_s1ap_UE_S1AP_ID_pair = -1;
static gint ett_s1ap_UE_associatedLogicalS1_ConnectionItem = -1;
static gint ett_s1ap_UE_HistoryInformation = -1;
static gint ett_s1ap_UEPagingID = -1;
static gint ett_s1ap_UESecurityCapabilities = -1;
static gint ett_s1ap_UserLocationInformation = -1;
static gint ett_s1ap_WarningAreaList = -1;
static gint ett_s1ap_X2TNLConfigurationInfo = -1;
static gint ett_s1ap_ENBX2ExtTLAs = -1;
static gint ett_s1ap_ENBX2ExtTLA = -1;
static gint ett_s1ap_ENBX2GTPTLAs = -1;
static gint ett_s1ap_ENBIndirectX2TransportLayerAddresses = -1;
static gint ett_s1ap_HandoverRequired = -1;
static gint ett_s1ap_HandoverCommand = -1;
static gint ett_s1ap_E_RABDataForwardingItem = -1;
static gint ett_s1ap_HandoverPreparationFailure = -1;
static gint ett_s1ap_HandoverRequest = -1;
static gint ett_s1ap_E_RABToBeSetupItemHOReq = -1;
static gint ett_s1ap_HandoverRequestAcknowledge = -1;
static gint ett_s1ap_E_RABAdmittedItem = -1;
static gint ett_s1ap_E_RABFailedToSetupItemHOReqAck = -1;
static gint ett_s1ap_HandoverFailure = -1;
static gint ett_s1ap_HandoverNotify = -1;
static gint ett_s1ap_PathSwitchRequest = -1;
static gint ett_s1ap_E_RABToBeSwitchedDLItem = -1;
static gint ett_s1ap_PathSwitchRequestAcknowledge = -1;
static gint ett_s1ap_E_RABToBeSwitchedULItem = -1;
static gint ett_s1ap_PathSwitchRequestFailure = -1;
static gint ett_s1ap_HandoverCancel = -1;
static gint ett_s1ap_HandoverCancelAcknowledge = -1;
static gint ett_s1ap_E_RABSetupRequest = -1;
static gint ett_s1ap_E_RABToBeSetupListBearerSUReq = -1;
static gint ett_s1ap_E_RABToBeSetupItemBearerSUReq = -1;
static gint ett_s1ap_E_RABSetupResponse = -1;
static gint ett_s1ap_E_RABSetupListBearerSURes = -1;
static gint ett_s1ap_E_RABSetupItemBearerSURes = -1;
static gint ett_s1ap_E_RABModifyRequest = -1;
static gint ett_s1ap_E_RABToBeModifiedListBearerModReq = -1;
static gint ett_s1ap_E_RABToBeModifiedItemBearerModReq = -1;
static gint ett_s1ap_E_RABModifyResponse = -1;
static gint ett_s1ap_E_RABModifyListBearerModRes = -1;
static gint ett_s1ap_E_RABModifyItemBearerModRes = -1;
static gint ett_s1ap_E_RABReleaseCommand = -1;
static gint ett_s1ap_E_RABReleaseResponse = -1;
static gint ett_s1ap_E_RABReleaseListBearerRelComp = -1;
static gint ett_s1ap_E_RABReleaseItemBearerRelComp = -1;
static gint ett_s1ap_E_RABReleaseIndication = -1;
static gint ett_s1ap_InitialContextSetupRequest = -1;
static gint ett_s1ap_E_RABToBeSetupListCtxtSUReq = -1;
static gint ett_s1ap_E_RABToBeSetupItemCtxtSUReq = -1;
static gint ett_s1ap_InitialContextSetupResponse = -1;
static gint ett_s1ap_E_RABSetupListCtxtSURes = -1;
static gint ett_s1ap_E_RABSetupItemCtxtSURes = -1;
static gint ett_s1ap_InitialContextSetupFailure = -1;
static gint ett_s1ap_Paging = -1;
static gint ett_s1ap_TAIList = -1;
static gint ett_s1ap_TAIItem = -1;
static gint ett_s1ap_UEContextReleaseRequest = -1;
static gint ett_s1ap_UEContextReleaseCommand = -1;
static gint ett_s1ap_UEContextReleaseComplete = -1;
static gint ett_s1ap_UEContextModificationRequest = -1;
static gint ett_s1ap_UEContextModificationResponse = -1;
static gint ett_s1ap_UEContextModificationFailure = -1;
static gint ett_s1ap_UERadioCapabilityMatchRequest = -1;
static gint ett_s1ap_UERadioCapabilityMatchResponse = -1;
static gint ett_s1ap_DownlinkNASTransport = -1;
static gint ett_s1ap_InitialUEMessage = -1;
static gint ett_s1ap_UplinkNASTransport = -1;
static gint ett_s1ap_NASNonDeliveryIndication = -1;
static gint ett_s1ap_RerouteNASRequest = -1;
static gint ett_s1ap_Reset = -1;
static gint ett_s1ap_ResetType = -1;
static gint ett_s1ap_UE_associatedLogicalS1_ConnectionListRes = -1;
static gint ett_s1ap_ResetAcknowledge = -1;
static gint ett_s1ap_UE_associatedLogicalS1_ConnectionListResAck = -1;
static gint ett_s1ap_ErrorIndication = -1;
static gint ett_s1ap_S1SetupRequest = -1;
static gint ett_s1ap_S1SetupResponse = -1;
static gint ett_s1ap_S1SetupFailure = -1;
static gint ett_s1ap_ENBConfigurationUpdate = -1;
static gint ett_s1ap_ENBConfigurationUpdateAcknowledge = -1;
static gint ett_s1ap_ENBConfigurationUpdateFailure = -1;
static gint ett_s1ap_MMEConfigurationUpdate = -1;
static gint ett_s1ap_MMEConfigurationUpdateAcknowledge = -1;
static gint ett_s1ap_MMEConfigurationUpdateFailure = -1;
static gint ett_s1ap_DownlinkS1cdma2000tunnelling = -1;
static gint ett_s1ap_UplinkS1cdma2000tunnelling = -1;
static gint ett_s1ap_UECapabilityInfoIndication = -1;
static gint ett_s1ap_ENBStatusTransfer = -1;
static gint ett_s1ap_MMEStatusTransfer = -1;
static gint ett_s1ap_TraceStart = -1;
static gint ett_s1ap_TraceFailureIndication = -1;
static gint ett_s1ap_DeactivateTrace = -1;
static gint ett_s1ap_CellTrafficTrace = -1;
static gint ett_s1ap_LocationReportingControl = -1;
static gint ett_s1ap_LocationReportingFailureIndication = -1;
static gint ett_s1ap_LocationReport = -1;
static gint ett_s1ap_OverloadStart = -1;
static gint ett_s1ap_OverloadStop = -1;
static gint ett_s1ap_WriteReplaceWarningRequest = -1;
static gint ett_s1ap_WriteReplaceWarningResponse = -1;
static gint ett_s1ap_ENBDirectInformationTransfer = -1;
static gint ett_s1ap_Inter_SystemInformationTransferType = -1;
static gint ett_s1ap_MMEDirectInformationTransfer = -1;
static gint ett_s1ap_ENBConfigurationTransfer = -1;
static gint ett_s1ap_MMEConfigurationTransfer = -1;
static gint ett_s1ap_PrivateMessage = -1;
static gint ett_s1ap_KillRequest = -1;
static gint ett_s1ap_KillResponse = -1;
static gint ett_s1ap_PWSRestartIndication = -1;
static gint ett_s1ap_PWSFailureIndication = -1;
static gint ett_s1ap_DownlinkUEAssociatedLPPaTransport = -1;
static gint ett_s1ap_UplinkUEAssociatedLPPaTransport = -1;
static gint ett_s1ap_DownlinkNonUEAssociatedLPPaTransport = -1;
static gint ett_s1ap_UplinkNonUEAssociatedLPPaTransport = -1;
static gint ett_s1ap_E_RABModificationIndication = -1;
static gint ett_s1ap_E_RABToBeModifiedItemBearerModInd = -1;
static gint ett_s1ap_E_RABNotToBeModifiedItemBearerModInd = -1;
static gint ett_s1ap_CSGMembershipInfo = -1;
static gint ett_s1ap_E_RABModificationConfirm = -1;
static gint ett_s1ap_E_RABModifyListBearerModConf = -1;
static gint ett_s1ap_E_RABModifyItemBearerModConf = -1;
static gint ett_s1ap_UEContextModificationIndication = -1;
static gint ett_s1ap_UEContextModificationConfirm = -1;
static gint ett_s1ap_S1AP_PDU = -1;
static gint ett_s1ap_InitiatingMessage = -1;
static gint ett_s1ap_SuccessfulOutcome = -1;
static gint ett_s1ap_UnsuccessfulOutcome = -1;
static gint ett_s1ap_SONtransferRequestContainer = -1;
static gint ett_s1ap_SONtransferResponseContainer = -1;
static gint ett_s1ap_SONtransferCause = -1;
static gint ett_s1ap_CellLoadReportingResponse = -1;
static gint ett_s1ap_EUTRANcellLoadReportingResponse = -1;
static gint ett_s1ap_EUTRANResponse = -1;
static gint ett_s1ap_IRAT_Cell_ID = -1;
static gint ett_s1ap_RequestedCellList = -1;
static gint ett_s1ap_MultiCellLoadReportingRequest = -1;
static gint ett_s1ap_ReportingCellList_Item = -1;
static gint ett_s1ap_ReportingCellList = -1;
static gint ett_s1ap_MultiCellLoadReportingResponse = -1;
static gint ett_s1ap_MultiCellLoadReportingResponse_Item = -1;
static gint ett_s1ap_EventTriggeredCellLoadReportingRequest = -1;
static gint ett_s1ap_EventTriggeredCellLoadReportingResponse = -1;
static gint ett_s1ap_HOReport = -1;
static gint ett_s1ap_CandidateCellList = -1;
static gint ett_s1ap_CandidatePCIList = -1;
static gint ett_s1ap_CandidatePCI = -1;
static gint ett_s1ap_CellActivationRequest = -1;
static gint ett_s1ap_CellsToActivateList = -1;
static gint ett_s1ap_CellsToActivateList_Item = -1;
static gint ett_s1ap_CellActivationResponse = -1;
static gint ett_s1ap_ActivatedCellsList = -1;
static gint ett_s1ap_ActivatedCellsList_Item = -1;
static gint ett_s1ap_CellStateIndication = -1;
static gint ett_s1ap_NotificationCellList = -1;
static gint ett_s1ap_NotificationCellList_Item = -1;
static gint ett_s1ap_FailureEventReport = -1;
static gint ett_s1ap_TooEarlyInterRATHOReportReportFromEUTRAN = -1;
static gint ett_s1ap_EHRPDSectorLoadReportingResponse = -1;
static gint ett_s1ap_EHRPDCompositeAvailableCapacity = -1;
static gint ett_s1ap_EHRPDMultiSectorLoadReportingResponseItem = -1;

/*--- End of included file: packet-s1ap-ett.c ---*/
#line 87 "./asn1/s1ap/packet-s1ap-template.c"

enum{
	INITIATING_MESSAGE,
	SUCCESSFUL_OUTCOME,
	UNSUCCESSFUL_OUTCOME
};


/* Global variables */
static guint32 ProcedureCode;
static guint32 ProtocolIE_ID;
static guint32 ProtocolExtensionID;
static guint gbl_s1apSctpPort=SCTP_PORT_S1AP;
static guint32 handover_type_value;
static guint32 message_type;
static gboolean g_s1ap_dissect_container = TRUE;
static const char *obj_id = NULL;

static dissector_handle_t gcsna_handle = NULL;
static dissector_handle_t s1ap_handle;


/* Dissector tables */
static dissector_table_t s1ap_ies_dissector_table;
static dissector_table_t s1ap_ies_p1_dissector_table;
static dissector_table_t s1ap_ies_p2_dissector_table;
static dissector_table_t s1ap_extension_dissector_table;
static dissector_table_t s1ap_proc_imsg_dissector_table;
static dissector_table_t s1ap_proc_sout_dissector_table;
static dissector_table_t s1ap_proc_uout_dissector_table;

static int dissect_ProtocolIEFieldValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *);
/* Currently not used
static int dissect_ProtocolIEFieldPairFirstValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static int dissect_ProtocolIEFieldPairSecondValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
*/
static int dissect_ProtocolExtensionFieldExtensionValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *);
static int dissect_InitiatingMessageValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *);
static int dissect_SuccessfulOutcomeValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *);
static int dissect_UnsuccessfulOutcomeValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *);

static int dissect_SourceeNB_ToTargeteNB_TransparentContainer_PDU(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *);
static int dissect_TargeteNB_ToSourceeNB_TransparentContainer_PDU(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *);
#if 0
static int dissect_SourceRNC_ToTargetRNC_TransparentContainer_PDU(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static int dissect_TargetRNC_ToSourceRNC_TransparentContainer_PDU(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static int dissect_SourceBSS_ToTargetBSS_TransparentContainer_PDU(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
static int dissect_TargetBSS_ToSourceBSS_TransparentContainer_PDU(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
#endif


/*--- Included file: packet-s1ap-fn.c ---*/
#line 1 "./asn1/s1ap/packet-s1ap-fn.c"

static const value_string s1ap_Criticality_vals[] = {
  {   0, "reject" },
  {   1, "ignore" },
  {   2, "notify" },
  { 0, NULL }
};


static int
dissect_s1ap_Criticality(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     3, NULL, FALSE, 0, NULL);

  return offset;
}



static int
dissect_s1ap_INTEGER_0_65535(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 65535U, NULL, FALSE);

  return offset;
}



static int
dissect_s1ap_T_global(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_object_identifier_str(tvb, offset, actx, tree, hf_index, &obj_id);

  return offset;
}


static const value_string s1ap_PrivateIE_ID_vals[] = {
  {   0, "local" },
  {   1, "global" },
  { 0, NULL }
};

static const per_choice_t PrivateIE_ID_choice[] = {
  {   0, &hf_s1ap_local          , ASN1_NO_EXTENSIONS     , dissect_s1ap_INTEGER_0_65535 },
  {   1, &hf_s1ap_global         , ASN1_NO_EXTENSIONS     , dissect_s1ap_T_global },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_PrivateIE_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 189 "./asn1/s1ap/s1ap.cnf"
obj_id = NULL;


  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_PrivateIE_ID, PrivateIE_ID_choice,
                                 NULL);

  return offset;
}


static const value_string s1ap_ProcedureCode_vals[] = {
  { id_HandoverPreparation, "id-HandoverPreparation" },
  { id_HandoverResourceAllocation, "id-HandoverResourceAllocation" },
  { id_HandoverNotification, "id-HandoverNotification" },
  { id_PathSwitchRequest, "id-PathSwitchRequest" },
  { id_HandoverCancel, "id-HandoverCancel" },
  { id_E_RABSetup, "id-E-RABSetup" },
  { id_E_RABModify, "id-E-RABModify" },
  { id_E_RABRelease, "id-E-RABRelease" },
  { id_E_RABReleaseIndication, "id-E-RABReleaseIndication" },
  { id_InitialContextSetup, "id-InitialContextSetup" },
  { id_Paging, "id-Paging" },
  { id_downlinkNASTransport, "id-downlinkNASTransport" },
  { id_initialUEMessage, "id-initialUEMessage" },
  { id_uplinkNASTransport, "id-uplinkNASTransport" },
  { id_Reset, "id-Reset" },
  { id_ErrorIndication, "id-ErrorIndication" },
  { id_NASNonDeliveryIndication, "id-NASNonDeliveryIndication" },
  { id_S1Setup, "id-S1Setup" },
  { id_UEContextReleaseRequest, "id-UEContextReleaseRequest" },
  { id_DownlinkS1cdma2000tunnelling, "id-DownlinkS1cdma2000tunnelling" },
  { id_UplinkS1cdma2000tunnelling, "id-UplinkS1cdma2000tunnelling" },
  { id_UEContextModification, "id-UEContextModification" },
  { id_UECapabilityInfoIndication, "id-UECapabilityInfoIndication" },
  { id_UEContextRelease, "id-UEContextRelease" },
  { id_eNBStatusTransfer, "id-eNBStatusTransfer" },
  { id_MMEStatusTransfer, "id-MMEStatusTransfer" },
  { id_DeactivateTrace, "id-DeactivateTrace" },
  { id_TraceStart, "id-TraceStart" },
  { id_TraceFailureIndication, "id-TraceFailureIndication" },
  { id_ENBConfigurationUpdate, "id-ENBConfigurationUpdate" },
  { id_MMEConfigurationUpdate, "id-MMEConfigurationUpdate" },
  { id_LocationReportingControl, "id-LocationReportingControl" },
  { id_LocationReportingFailureIndication, "id-LocationReportingFailureIndication" },
  { id_LocationReport, "id-LocationReport" },
  { id_OverloadStart, "id-OverloadStart" },
  { id_OverloadStop, "id-OverloadStop" },
  { id_WriteReplaceWarning, "id-WriteReplaceWarning" },
  { id_eNBDirectInformationTransfer, "id-eNBDirectInformationTransfer" },
  { id_MMEDirectInformationTransfer, "id-MMEDirectInformationTransfer" },
  { id_PrivateMessage, "id-PrivateMessage" },
  { id_eNBConfigurationTransfer, "id-eNBConfigurationTransfer" },
  { id_MMEConfigurationTransfer, "id-MMEConfigurationTransfer" },
  { id_CellTrafficTrace, "id-CellTrafficTrace" },
  { id_Kill, "id-Kill" },
  { id_downlinkUEAssociatedLPPaTransport, "id-downlinkUEAssociatedLPPaTransport" },
  { id_uplinkUEAssociatedLPPaTransport, "id-uplinkUEAssociatedLPPaTransport" },
  { id_downlinkNonUEAssociatedLPPaTransport, "id-downlinkNonUEAssociatedLPPaTransport" },
  { id_uplinkNonUEAssociatedLPPaTransport, "id-uplinkNonUEAssociatedLPPaTransport" },
  { id_UERadioCapabilityMatch, "id-UERadioCapabilityMatch" },
  { id_PWSRestartIndication, "id-PWSRestartIndication" },
  { id_E_RABModificationIndication, "id-E-RABModificationIndication" },
  { id_PWSFailureIndication, "id-PWSFailureIndication" },
  { id_RerouteNASRequest, "id-RerouteNASRequest" },
  { id_UEContextModificationIndication, "id-UEContextModificationIndication" },
  { 0, NULL }
};

static value_string_ext s1ap_ProcedureCode_vals_ext = VALUE_STRING_EXT_INIT(s1ap_ProcedureCode_vals);


static int
dissect_s1ap_ProcedureCode(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, &ProcedureCode, FALSE);

#line 109 "./asn1/s1ap/s1ap.cnf"
     col_add_fstr(actx->pinfo->cinfo, COL_INFO, "%s",
                 val_to_str_ext(ProcedureCode, &s1ap_ProcedureCode_vals_ext,
                            "unknown message"));

  return offset;
}



static int
dissect_s1ap_ProtocolExtensionID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 65535U, &ProtocolExtensionID, FALSE);

  return offset;
}


static const value_string s1ap_ProtocolIE_ID_vals[] = {
  { id_MME_UE_S1AP_ID, "id-MME-UE-S1AP-ID" },
  { id_HandoverType, "id-HandoverType" },
  { id_Cause, "id-Cause" },
  { id_SourceID, "id-SourceID" },
  { id_TargetID, "id-TargetID" },
  { id_Unknown_5, "id-Unknown-5" },
  { id_Unknown_6, "id-Unknown-6" },
  { id_Unknown_7, "id-Unknown-7" },
  { id_eNB_UE_S1AP_ID, "id-eNB-UE-S1AP-ID" },
  { id_Unknown_9, "id-Unknown-9" },
  { id_Unknown_10, "id-Unknown-10" },
  { id_Unknown_11, "id-Unknown-11" },
  { id_E_RABSubjecttoDataForwardingList, "id-E-RABSubjecttoDataForwardingList" },
  { id_E_RABtoReleaseListHOCmd, "id-E-RABtoReleaseListHOCmd" },
  { id_E_RABDataForwardingItem, "id-E-RABDataForwardingItem" },
  { id_E_RABReleaseItemBearerRelComp, "id-E-RABReleaseItemBearerRelComp" },
  { id_E_RABToBeSetupListBearerSUReq, "id-E-RABToBeSetupListBearerSUReq" },
  { id_E_RABToBeSetupItemBearerSUReq, "id-E-RABToBeSetupItemBearerSUReq" },
  { id_E_RABAdmittedList, "id-E-RABAdmittedList" },
  { id_E_RABFailedToSetupListHOReqAck, "id-E-RABFailedToSetupListHOReqAck" },
  { id_E_RABAdmittedItem, "id-E-RABAdmittedItem" },
  { id_E_RABFailedtoSetupItemHOReqAck, "id-E-RABFailedtoSetupItemHOReqAck" },
  { id_E_RABToBeSwitchedDLList, "id-E-RABToBeSwitchedDLList" },
  { id_E_RABToBeSwitchedDLItem, "id-E-RABToBeSwitchedDLItem" },
  { id_E_RABToBeSetupListCtxtSUReq, "id-E-RABToBeSetupListCtxtSUReq" },
  { id_TraceActivation, "id-TraceActivation" },
  { id_NAS_PDU, "id-NAS-PDU" },
  { id_E_RABToBeSetupItemHOReq, "id-E-RABToBeSetupItemHOReq" },
  { id_E_RABSetupListBearerSURes, "id-E-RABSetupListBearerSURes" },
  { id_E_RABFailedToSetupListBearerSURes, "id-E-RABFailedToSetupListBearerSURes" },
  { id_E_RABToBeModifiedListBearerModReq, "id-E-RABToBeModifiedListBearerModReq" },
  { id_E_RABModifyListBearerModRes, "id-E-RABModifyListBearerModRes" },
  { id_E_RABFailedToModifyList, "id-E-RABFailedToModifyList" },
  { id_E_RABToBeReleasedList, "id-E-RABToBeReleasedList" },
  { id_E_RABFailedToReleaseList, "id-E-RABFailedToReleaseList" },
  { id_E_RABItem, "id-E-RABItem" },
  { id_E_RABToBeModifiedItemBearerModReq, "id-E-RABToBeModifiedItemBearerModReq" },
  { id_E_RABModifyItemBearerModRes, "id-E-RABModifyItemBearerModRes" },
  { id_E_RABReleaseItem, "id-E-RABReleaseItem" },
  { id_E_RABSetupItemBearerSURes, "id-E-RABSetupItemBearerSURes" },
  { id_SecurityContext, "id-SecurityContext" },
  { id_HandoverRestrictionList, "id-HandoverRestrictionList" },
  { id_Unknown_42, "id-Unknown-42" },
  { id_UEPagingID, "id-UEPagingID" },
  { id_pagingDRX, "id-pagingDRX" },
  { id_Unknown_45, "id-Unknown-45" },
  { id_TAIList, "id-TAIList" },
  { id_TAIItem, "id-TAIItem" },
  { id_E_RABFailedToSetupListCtxtSURes, "id-E-RABFailedToSetupListCtxtSURes" },
  { id_E_RABReleaseItemHOCmd, "id-E-RABReleaseItemHOCmd" },
  { id_E_RABSetupItemCtxtSURes, "id-E-RABSetupItemCtxtSURes" },
  { id_E_RABSetupListCtxtSURes, "id-E-RABSetupListCtxtSURes" },
  { id_E_RABToBeSetupItemCtxtSUReq, "id-E-RABToBeSetupItemCtxtSUReq" },
  { id_E_RABToBeSetupListHOReq, "id-E-RABToBeSetupListHOReq" },
  { id_Unknown_54, "id-Unknown-54" },
  { id_GERANtoLTEHOInformationRes, "id-GERANtoLTEHOInformationRes" },
  { id_Unknown_56, "id-Unknown-56" },
  { id_UTRANtoLTEHOInformationRes, "id-UTRANtoLTEHOInformationRes" },
  { id_CriticalityDiagnostics, "id-CriticalityDiagnostics" },
  { id_Global_ENB_ID, "id-Global-ENB-ID" },
  { id_eNBname, "id-eNBname" },
  { id_MMEname, "id-MMEname" },
  { id_Unknown_62, "id-Unknown-62" },
  { id_ServedPLMNs, "id-ServedPLMNs" },
  { id_SupportedTAs, "id-SupportedTAs" },
  { id_TimeToWait, "id-TimeToWait" },
  { id_uEaggregateMaximumBitrate, "id-uEaggregateMaximumBitrate" },
  { id_TAI, "id-TAI" },
  { id_Unknown_68, "id-Unknown-68" },
  { id_E_RABReleaseListBearerRelComp, "id-E-RABReleaseListBearerRelComp" },
  { id_cdma2000PDU, "id-cdma2000PDU" },
  { id_cdma2000RATType, "id-cdma2000RATType" },
  { id_cdma2000SectorID, "id-cdma2000SectorID" },
  { id_SecurityKey, "id-SecurityKey" },
  { id_UERadioCapability, "id-UERadioCapability" },
  { id_GUMMEI_ID, "id-GUMMEI-ID" },
  { id_Unknown_76, "id-Unknown-76" },
  { id_E_RABInformationListItem, "id-E-RABInformationListItem" },
  { id_Direct_Forwarding_Path_Availability, "id-Direct-Forwarding-Path-Availability" },
  { id_UEIdentityIndexValue, "id-UEIdentityIndexValue" },
  { id_Unknown_81, "id-Unknown-81" },
  { id_Unknown_82, "id-Unknown-82" },
  { id_cdma2000HOStatus, "id-cdma2000HOStatus" },
  { id_cdma2000HORequiredIndication, "id-cdma2000HORequiredIndication" },
  { id_E_UTRAN_Trace_ID, "id-E-UTRAN-Trace-ID" },
  { id_RelativeMMECapacity, "id-RelativeMMECapacity" },
  { id_SourceMME_UE_S1AP_ID, "id-SourceMME-UE-S1AP-ID" },
  { id_Bearers_SubjectToStatusTransfer_Item, "id-Bearers-SubjectToStatusTransfer-Item" },
  { id_eNB_StatusTransfer_TransparentContainer, "id-eNB-StatusTransfer-TransparentContainer" },
  { id_UE_associatedLogicalS1_ConnectionItem, "id-UE-associatedLogicalS1-ConnectionItem" },
  { id_ResetType, "id-ResetType" },
  { id_UE_associatedLogicalS1_ConnectionListResAck, "id-UE-associatedLogicalS1-ConnectionListResAck" },
  { id_E_RABToBeSwitchedULItem, "id-E-RABToBeSwitchedULItem" },
  { id_E_RABToBeSwitchedULList, "id-E-RABToBeSwitchedULList" },
  { id_S_TMSI, "id-S-TMSI" },
  { id_cdma2000OneXRAND, "id-cdma2000OneXRAND" },
  { id_RequestType, "id-RequestType" },
  { id_UE_S1AP_IDs, "id-UE-S1AP-IDs" },
  { id_EUTRAN_CGI, "id-EUTRAN-CGI" },
  { id_OverloadResponse, "id-OverloadResponse" },
  { id_cdma2000OneXSRVCCInfo, "id-cdma2000OneXSRVCCInfo" },
  { id_E_RABFailedToBeReleasedList, "id-E-RABFailedToBeReleasedList" },
  { id_Source_ToTarget_TransparentContainer, "id-Source-ToTarget-TransparentContainer" },
  { id_ServedGUMMEIs, "id-ServedGUMMEIs" },
  { id_SubscriberProfileIDforRFP, "id-SubscriberProfileIDforRFP" },
  { id_UESecurityCapabilities, "id-UESecurityCapabilities" },
  { id_CSFallbackIndicator, "id-CSFallbackIndicator" },
  { id_CNDomain, "id-CNDomain" },
  { id_E_RABReleasedList, "id-E-RABReleasedList" },
  { id_MessageIdentifier, "id-MessageIdentifier" },
  { id_SerialNumber, "id-SerialNumber" },
  { id_WarningAreaList, "id-WarningAreaList" },
  { id_RepetitionPeriod, "id-RepetitionPeriod" },
  { id_NumberofBroadcastRequest, "id-NumberofBroadcastRequest" },
  { id_WarningType, "id-WarningType" },
  { id_WarningSecurityInfo, "id-WarningSecurityInfo" },
  { id_DataCodingScheme, "id-DataCodingScheme" },
  { id_WarningMessageContents, "id-WarningMessageContents" },
  { id_BroadcastCompletedAreaList, "id-BroadcastCompletedAreaList" },
  { id_Inter_SystemInformationTransferTypeEDT, "id-Inter-SystemInformationTransferTypeEDT" },
  { id_Inter_SystemInformationTransferTypeMDT, "id-Inter-SystemInformationTransferTypeMDT" },
  { id_Target_ToSource_TransparentContainer, "id-Target-ToSource-TransparentContainer" },
  { id_SRVCCOperationPossible, "id-SRVCCOperationPossible" },
  { id_SRVCCHOIndication, "id-SRVCCHOIndication" },
  { id_NAS_DownlinkCount, "id-NAS-DownlinkCount" },
  { id_CSG_Id, "id-CSG-Id" },
  { id_CSG_IdList, "id-CSG-IdList" },
  { id_SONConfigurationTransferECT, "id-SONConfigurationTransferECT" },
  { id_SONConfigurationTransferMCT, "id-SONConfigurationTransferMCT" },
  { id_TraceCollectionEntityIPAddress, "id-TraceCollectionEntityIPAddress" },
  { id_MSClassmark2, "id-MSClassmark2" },
  { id_MSClassmark3, "id-MSClassmark3" },
  { id_RRC_Establishment_Cause, "id-RRC-Establishment-Cause" },
  { id_NASSecurityParametersfromE_UTRAN, "id-NASSecurityParametersfromE-UTRAN" },
  { id_NASSecurityParameterstoE_UTRAN, "id-NASSecurityParameterstoE-UTRAN" },
  { id_DefaultPagingDRX, "id-DefaultPagingDRX" },
  { id_Source_ToTarget_TransparentContainer_Secondary, "id-Source-ToTarget-TransparentContainer-Secondary" },
  { id_Target_ToSource_TransparentContainer_Secondary, "id-Target-ToSource-TransparentContainer-Secondary" },
  { id_EUTRANRoundTripDelayEstimationInfo, "id-EUTRANRoundTripDelayEstimationInfo" },
  { id_BroadcastCancelledAreaList, "id-BroadcastCancelledAreaList" },
  { id_ConcurrentWarningMessageIndicator, "id-ConcurrentWarningMessageIndicator" },
  { id_Data_Forwarding_Not_Possible, "id-Data-Forwarding-Not-Possible" },
  { id_ExtendedRepetitionPeriod, "id-ExtendedRepetitionPeriod" },
  { id_CellAccessMode, "id-CellAccessMode" },
  { id_CSGMembershipStatus, "id-CSGMembershipStatus" },
  { id_LPPa_PDU, "id-LPPa-PDU" },
  { id_Routing_ID, "id-Routing-ID" },
  { id_Time_Synchronisation_Info, "id-Time-Synchronisation-Info" },
  { id_PS_ServiceNotAvailable, "id-PS-ServiceNotAvailable" },
  { id_PagingPriority, "id-PagingPriority" },
  { id_x2TNLConfigurationInfo, "id-x2TNLConfigurationInfo" },
  { id_eNBX2ExtendedTransportLayerAddresses, "id-eNBX2ExtendedTransportLayerAddresses" },
  { id_GUMMEIList, "id-GUMMEIList" },
  { id_GW_TransportLayerAddress, "id-GW-TransportLayerAddress" },
  { id_Correlation_ID, "id-Correlation-ID" },
  { id_SourceMME_GUMMEI, "id-SourceMME-GUMMEI" },
  { id_MME_UE_S1AP_ID_2, "id-MME-UE-S1AP-ID-2" },
  { id_RegisteredLAI, "id-RegisteredLAI" },
  { id_RelayNode_Indicator, "id-RelayNode-Indicator" },
  { id_TrafficLoadReductionIndication, "id-TrafficLoadReductionIndication" },
  { id_MDTConfiguration, "id-MDTConfiguration" },
  { id_MMERelaySupportIndicator, "id-MMERelaySupportIndicator" },
  { id_GWContextReleaseIndication, "id-GWContextReleaseIndication" },
  { id_ManagementBasedMDTAllowed, "id-ManagementBasedMDTAllowed" },
  { id_PrivacyIndicator, "id-PrivacyIndicator" },
  { id_Time_UE_StayedInCell_EnhancedGranularity, "id-Time-UE-StayedInCell-EnhancedGranularity" },
  { id_HO_Cause, "id-HO-Cause" },
  { id_VoiceSupportMatchIndicator, "id-VoiceSupportMatchIndicator" },
  { id_GUMMEIType, "id-GUMMEIType" },
  { id_M3Configuration, "id-M3Configuration" },
  { id_M4Configuration, "id-M4Configuration" },
  { id_M5Configuration, "id-M5Configuration" },
  { id_MDT_Location_Info, "id-MDT-Location-Info" },
  { id_MobilityInformation, "id-MobilityInformation" },
  { id_Tunnel_Information_for_BBF, "id-Tunnel-Information-for-BBF" },
  { id_ManagementBasedMDTPLMNList, "id-ManagementBasedMDTPLMNList" },
  { id_SignallingBasedMDTPLMNList, "id-SignallingBasedMDTPLMNList" },
  { id_ULCOUNTValueExtended, "id-ULCOUNTValueExtended" },
  { id_DLCOUNTValueExtended, "id-DLCOUNTValueExtended" },
  { id_ReceiveStatusOfULPDCPSDUsExtended, "id-ReceiveStatusOfULPDCPSDUsExtended" },
  { id_ECGIListForRestart, "id-ECGIListForRestart" },
  { id_SIPTO_Correlation_ID, "id-SIPTO-Correlation-ID" },
  { id_SIPTO_L_GW_TransportLayerAddress, "id-SIPTO-L-GW-TransportLayerAddress" },
  { id_TransportInformation, "id-TransportInformation" },
  { id_LHN_ID, "id-LHN-ID" },
  { id_AdditionalCSFallbackIndicator, "id-AdditionalCSFallbackIndicator" },
  { id_TAIListForRestart, "id-TAIListForRestart" },
  { id_UserLocationInformation, "id-UserLocationInformation" },
  { id_EmergencyAreaIDListForRestart, "id-EmergencyAreaIDListForRestart" },
  { id_KillAllWarningMessages, "id-KillAllWarningMessages" },
  { id_Masked_IMEISV, "id-Masked-IMEISV" },
  { id_eNBIndirectX2TransportLayerAddresses, "id-eNBIndirectX2TransportLayerAddresses" },
  { id_uE_HistoryInformationFromTheUE, "id-uE-HistoryInformationFromTheUE" },
  { id_ProSeAuthorized, "id-ProSeAuthorized" },
  { id_ExpectedUEBehaviour, "id-ExpectedUEBehaviour" },
  { id_LoggedMBSFNMDT, "id-LoggedMBSFNMDT" },
  { id_UERadioCapabilityForPaging, "id-UERadioCapabilityForPaging" },
  { id_E_RABToBeModifiedListBearerModInd, "id-E-RABToBeModifiedListBearerModInd" },
  { id_E_RABToBeModifiedItemBearerModInd, "id-E-RABToBeModifiedItemBearerModInd" },
  { id_E_RABNotToBeModifiedListBearerModInd, "id-E-RABNotToBeModifiedListBearerModInd" },
  { id_E_RABNotToBeModifiedItemBearerModInd, "id-E-RABNotToBeModifiedItemBearerModInd" },
  { id_E_RABModifyListBearerModConf, "id-E-RABModifyListBearerModConf" },
  { id_E_RABModifyItemBearerModConf, "id-E-RABModifyItemBearerModConf" },
  { id_E_RABFailedToModifyListBearerModConf, "id-E-RABFailedToModifyListBearerModConf" },
  { id_SON_Information_Report, "id-SON-Information-Report" },
  { id_Muting_Availability_Indication, "id-Muting-Availability-Indication" },
  { id_Muting_Pattern_Information, "id-Muting-Pattern-Information" },
  { id_Synchronisation_Information, "id-Synchronisation-Information" },
  { id_E_RABToBeReleasedListBearerModConf, "id-E-RABToBeReleasedListBearerModConf" },
  { id_ProSeUEtoNetworkRelaying, "id-ProSeUEtoNetworkRelaying" },
  { id_ULCOUNTValuePDCP_SNlength18, "id-ULCOUNTValuePDCP-SNlength18" },
  { id_DLCOUNTValuePDCP_SNlength18, "id-DLCOUNTValuePDCP-SNlength18" },
  { id_ReceiveStatusOfULPDCPSDUsPDCP_SNlength18, "id-ReceiveStatusOfULPDCPSDUsPDCP-SNlength18" },
  { id_M6Configuration, "id-M6Configuration" },
  { id_M7Configuration, "id-M7Configuration" },
  { id_PWSfailedECGIList, "id-PWSfailedECGIList" },
  { id_MME_Group_ID, "id-MME-Group-ID" },
  { id_Additional_GUTI, "id-Additional-GUTI" },
  { id_S1_Message, "id-S1-Message" },
  { id_CSGMembershipInfo, "id-CSGMembershipInfo" },
  { 0, NULL }
};

static value_string_ext s1ap_ProtocolIE_ID_vals_ext = VALUE_STRING_EXT_INIT(s1ap_ProtocolIE_ID_vals);


static int
dissect_s1ap_ProtocolIE_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 65535U, &ProtocolIE_ID, FALSE);

#line 92 "./asn1/s1ap/s1ap.cnf"
  if (tree) {
    proto_item_append_text(proto_item_get_parent_nth(actx->created_item, 2), ": %s", val_to_str_ext(ProtocolIE_ID, &s1ap_ProtocolIE_ID_vals_ext, "unknown (%d)"));
  }

  return offset;
}


static const value_string s1ap_TriggeringMessage_vals[] = {
  {   0, "initiating-message" },
  {   1, "successful-outcome" },
  {   2, "unsuccessfull-outcome" },
  { 0, NULL }
};


static int
dissect_s1ap_TriggeringMessage(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     3, NULL, FALSE, 0, NULL);

  return offset;
}



static int
dissect_s1ap_T_ie_field_value(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_open_type_pdu_new(tvb, offset, actx, tree, hf_index, dissect_ProtocolIEFieldValue);

  return offset;
}


static const per_sequence_t ProtocolIE_Field_sequence[] = {
  { &hf_s1ap_id             , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_ID },
  { &hf_s1ap_criticality    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_Criticality },
  { &hf_s1ap_ie_field_value , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_T_ie_field_value },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_ProtocolIE_Field(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_ProtocolIE_Field, ProtocolIE_Field_sequence);

  return offset;
}


static const per_sequence_t ProtocolIE_Container_sequence_of[1] = {
  { &hf_s1ap_ProtocolIE_Container_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Field },
};

static int
dissect_s1ap_ProtocolIE_Container(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_ProtocolIE_Container, ProtocolIE_Container_sequence_of,
                                                  0, maxProtocolIEs, FALSE);

  return offset;
}



static int
dissect_s1ap_ProtocolIE_SingleContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_s1ap_ProtocolIE_Field(tvb, offset, actx, tree, hf_index);

  return offset;
}


static const per_sequence_t ProtocolIE_ContainerList_sequence_of[1] = {
  { &hf_s1ap_ProtocolIE_ContainerList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_SingleContainer },
};

static int
dissect_s1ap_ProtocolIE_ContainerList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 134 "./asn1/s1ap/s1ap.cnf"
  static const asn1_par_def_t ProtocolIE_ContainerList_pars[] = {
    { "lowerBound", ASN1_PAR_INTEGER },
    { "upperBound", ASN1_PAR_INTEGER },
    { NULL, (asn1_par_type)0 }
  };
  asn1_stack_frame_check(actx, "ProtocolIE-ContainerList", ProtocolIE_ContainerList_pars);

  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_ProtocolIE_ContainerList, ProtocolIE_ContainerList_sequence_of,
                                                  asn1_param_get_integer(actx,"lowerBound"), asn1_param_get_integer(actx,"upperBound"), FALSE);

  return offset;
}



static int
dissect_s1ap_T_extensionValue(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_open_type_pdu_new(tvb, offset, actx, tree, hf_index, dissect_ProtocolExtensionFieldExtensionValue);

  return offset;
}


static const per_sequence_t ProtocolExtensionField_sequence[] = {
  { &hf_s1ap_ext_id         , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolExtensionID },
  { &hf_s1ap_criticality    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_Criticality },
  { &hf_s1ap_extensionValue , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_T_extensionValue },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_ProtocolExtensionField(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_ProtocolExtensionField, ProtocolExtensionField_sequence);

  return offset;
}


static const per_sequence_t ProtocolExtensionContainer_sequence_of[1] = {
  { &hf_s1ap_ProtocolExtensionContainer_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolExtensionField },
};

static int
dissect_s1ap_ProtocolExtensionContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_ProtocolExtensionContainer, ProtocolExtensionContainer_sequence_of,
                                                  1, maxProtocolExtensions, FALSE);

  return offset;
}



static int
dissect_s1ap_T_value(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 194 "./asn1/s1ap/s1ap.cnf"

  if (obj_id){
	 offset=call_per_oid_callback(obj_id, tvb, actx->pinfo, tree, offset, actx, hf_index);
  }else{
  offset = dissect_per_open_type(tvb, offset, actx, tree, hf_index, NULL);

  }





  return offset;
}


static const per_sequence_t PrivateIE_Field_sequence[] = {
  { &hf_s1ap_private_id     , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_PrivateIE_ID },
  { &hf_s1ap_criticality    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_Criticality },
  { &hf_s1ap_value          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_T_value },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_PrivateIE_Field(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_PrivateIE_Field, PrivateIE_Field_sequence);

  return offset;
}


static const per_sequence_t PrivateIE_Container_sequence_of[1] = {
  { &hf_s1ap_PrivateIE_Container_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_PrivateIE_Field },
};

static int
dissect_s1ap_PrivateIE_Container(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_PrivateIE_Container, PrivateIE_Container_sequence_of,
                                                  1, maxPrivateIEs, FALSE);

  return offset;
}




static int
dissect_s1ap_PLMNidentity(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 207 "./asn1/s1ap/s1ap.cnf"
  tvbuff_t *parameter_tvb=NULL;

  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       3, 3, FALSE, &parameter_tvb);
	if(tvb_reported_length(tvb)==0)
		return offset;

	if (!parameter_tvb)
		return offset;
	dissect_e212_mcc_mnc(parameter_tvb, actx->pinfo, tree, 0, E212_NONE, FALSE);


  return offset;
}



static int
dissect_s1ap_MME_Group_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       2, 2, FALSE, NULL);

  return offset;
}



static int
dissect_s1ap_MME_Code(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       1, 1, FALSE, NULL);

  return offset;
}


static const per_sequence_t GUMMEI_sequence[] = {
  { &hf_s1ap_pLMN_Identity  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_PLMNidentity },
  { &hf_s1ap_mME_Group_ID   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_MME_Group_ID },
  { &hf_s1ap_mME_Code       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_MME_Code },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_GUMMEI(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_GUMMEI, GUMMEI_sequence);

  return offset;
}



static int
dissect_s1ap_M_TMSI(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       4, 4, FALSE, NULL);

  return offset;
}


static const per_sequence_t Additional_GUTI_sequence[] = {
  { &hf_s1ap_gUMMEI         , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_GUMMEI },
  { &hf_s1ap_m_TMSI         , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_M_TMSI },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_Additional_GUTI(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_Additional_GUTI, Additional_GUTI_sequence);

  return offset;
}



static int
dissect_s1ap_CellIdentity(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     28, 28, FALSE, NULL, NULL);

  return offset;
}


static const per_sequence_t EUTRAN_CGI_sequence[] = {
  { &hf_s1ap_pLMNidentity   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_PLMNidentity },
  { &hf_s1ap_cell_ID        , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_CellIdentity },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_EUTRAN_CGI(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_EUTRAN_CGI, EUTRAN_CGI_sequence);

  return offset;
}


static const per_sequence_t CellIdListforMDT_sequence_of[1] = {
  { &hf_s1ap_CellIdListforMDT_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_EUTRAN_CGI },
};

static int
dissect_s1ap_CellIdListforMDT(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_CellIdListforMDT, CellIdListforMDT_sequence_of,
                                                  1, maxnoofCellIDforMDT, FALSE);

  return offset;
}


static const per_sequence_t CellBasedMDT_sequence[] = {
  { &hf_s1ap_cellIdListforMDT, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_CellIdListforMDT },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_CellBasedMDT(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_CellBasedMDT, CellBasedMDT_sequence);

  return offset;
}



static int
dissect_s1ap_TAC(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 244 "./asn1/s1ap/s1ap.cnf"
tvbuff_t *parameter_tvb=NULL;

  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       2, 2, FALSE, &parameter_tvb);


  if (!parameter_tvb)
    return offset;

  proto_item_append_text(actx->created_item," (%u)",tvb_get_ntohs(parameter_tvb, 0));



  return offset;
}


static const per_sequence_t TAListforMDT_sequence_of[1] = {
  { &hf_s1ap_TAListforMDT_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_TAC },
};

static int
dissect_s1ap_TAListforMDT(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_TAListforMDT, TAListforMDT_sequence_of,
                                                  1, maxnoofTAforMDT, FALSE);

  return offset;
}


static const per_sequence_t TABasedMDT_sequence[] = {
  { &hf_s1ap_tAListforMDT   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TAListforMDT },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_TABasedMDT(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_TABasedMDT, TABasedMDT_sequence);

  return offset;
}



static int
dissect_s1ap_NULL(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_null(tvb, offset, actx, tree, hf_index);

  return offset;
}


static const per_sequence_t TAI_sequence[] = {
  { &hf_s1ap_pLMNidentity   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_PLMNidentity },
  { &hf_s1ap_tAC            , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TAC },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_TAI(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_TAI, TAI_sequence);

  return offset;
}


static const per_sequence_t TAIListforMDT_sequence_of[1] = {
  { &hf_s1ap_TAIListforMDT_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_TAI },
};

static int
dissect_s1ap_TAIListforMDT(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_TAIListforMDT, TAIListforMDT_sequence_of,
                                                  1, maxnoofTAforMDT, FALSE);

  return offset;
}


static const per_sequence_t TAIBasedMDT_sequence[] = {
  { &hf_s1ap_tAIListforMDT  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TAIListforMDT },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_TAIBasedMDT(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_TAIBasedMDT, TAIBasedMDT_sequence);

  return offset;
}


static const value_string s1ap_AreaScopeOfMDT_vals[] = {
  {   0, "cellBased" },
  {   1, "tABased" },
  {   2, "pLMNWide" },
  {   3, "tAIBased" },
  { 0, NULL }
};

static const per_choice_t AreaScopeOfMDT_choice[] = {
  {   0, &hf_s1ap_cellBased      , ASN1_EXTENSION_ROOT    , dissect_s1ap_CellBasedMDT },
  {   1, &hf_s1ap_tABased        , ASN1_EXTENSION_ROOT    , dissect_s1ap_TABasedMDT },
  {   2, &hf_s1ap_pLMNWide       , ASN1_EXTENSION_ROOT    , dissect_s1ap_NULL },
  {   3, &hf_s1ap_tAIBased       , ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_TAIBasedMDT },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_AreaScopeOfMDT(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_AreaScopeOfMDT, AreaScopeOfMDT_choice,
                                 NULL);

  return offset;
}


static const value_string s1ap_PriorityLevel_vals[] = {
  {   0, "spare" },
  {   1, "highest" },
  {  14, "lowest" },
  {  15, "no-priority" },
  { 0, NULL }
};


static int
dissect_s1ap_PriorityLevel(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 15U, NULL, FALSE);

  return offset;
}


static const value_string s1ap_Pre_emptionCapability_vals[] = {
  {   0, "shall-not-trigger-pre-emption" },
  {   1, "may-trigger-pre-emption" },
  { 0, NULL }
};


static int
dissect_s1ap_Pre_emptionCapability(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, FALSE, 0, NULL);

  return offset;
}


static const value_string s1ap_Pre_emptionVulnerability_vals[] = {
  {   0, "not-pre-emptable" },
  {   1, "pre-emptable" },
  { 0, NULL }
};


static int
dissect_s1ap_Pre_emptionVulnerability(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, FALSE, 0, NULL);

  return offset;
}


static const per_sequence_t AllocationAndRetentionPriority_sequence[] = {
  { &hf_s1ap_priorityLevel  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_PriorityLevel },
  { &hf_s1ap_pre_emptionCapability, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_Pre_emptionCapability },
  { &hf_s1ap_pre_emptionVulnerability, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_Pre_emptionVulnerability },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_AllocationAndRetentionPriority(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_AllocationAndRetentionPriority, AllocationAndRetentionPriority_sequence);

  return offset;
}


static const per_sequence_t Bearers_SubjectToStatusTransferList_sequence_of[1] = {
  { &hf_s1ap_Bearers_SubjectToStatusTransferList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_SingleContainer },
};

static int
dissect_s1ap_Bearers_SubjectToStatusTransferList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_Bearers_SubjectToStatusTransferList, Bearers_SubjectToStatusTransferList_sequence_of,
                                                  1, maxnoofE_RABs, FALSE);

  return offset;
}



static int
dissect_s1ap_E_RAB_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 15U, NULL, TRUE);

  return offset;
}



static int
dissect_s1ap_PDCP_SN(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 4095U, NULL, FALSE);

  return offset;
}



static int
dissect_s1ap_HFN(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 1048575U, NULL, FALSE);

  return offset;
}


static const per_sequence_t COUNTvalue_sequence[] = {
  { &hf_s1ap_pDCP_SN        , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_PDCP_SN },
  { &hf_s1ap_hFN            , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_HFN },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_COUNTvalue(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_COUNTvalue, COUNTvalue_sequence);

  return offset;
}



static int
dissect_s1ap_ReceiveStatusofULPDCPSDUs(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     4096, 4096, FALSE, NULL, NULL);

  return offset;
}


static const per_sequence_t Bearers_SubjectToStatusTransfer_Item_sequence[] = {
  { &hf_s1ap_e_RAB_ID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_E_RAB_ID },
  { &hf_s1ap_uL_COUNTvalue  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_COUNTvalue },
  { &hf_s1ap_dL_COUNTvalue  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_COUNTvalue },
  { &hf_s1ap_receiveStatusofULPDCPSDUs, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ReceiveStatusofULPDCPSDUs },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_Bearers_SubjectToStatusTransfer_Item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_Bearers_SubjectToStatusTransfer_Item, Bearers_SubjectToStatusTransfer_Item_sequence);

  return offset;
}



static int
dissect_s1ap_BitRate(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer_64b(tvb, offset, actx, tree, hf_index,
                                                            0U, G_GUINT64_CONSTANT(10000000000), NULL, FALSE);

  return offset;
}


static const per_sequence_t BPLMNs_sequence_of[1] = {
  { &hf_s1ap_BPLMNs_item    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_PLMNidentity },
};

static int
dissect_s1ap_BPLMNs(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_BPLMNs, BPLMNs_sequence_of,
                                                  1, maxnoofBPLMNs, FALSE);

  return offset;
}



static int
dissect_s1ap_NumberOfBroadcasts(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 65535U, NULL, FALSE);

  return offset;
}


static const per_sequence_t CellID_Cancelled_Item_sequence[] = {
  { &hf_s1ap_eCGI           , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_EUTRAN_CGI },
  { &hf_s1ap_numberOfBroadcasts, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_NumberOfBroadcasts },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_CellID_Cancelled_Item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_CellID_Cancelled_Item, CellID_Cancelled_Item_sequence);

  return offset;
}


static const per_sequence_t CellID_Cancelled_sequence_of[1] = {
  { &hf_s1ap_CellID_Cancelled_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_CellID_Cancelled_Item },
};

static int
dissect_s1ap_CellID_Cancelled(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_CellID_Cancelled, CellID_Cancelled_sequence_of,
                                                  1, maxnoofCellID, FALSE);

  return offset;
}


static const per_sequence_t CancelledCellinTAI_Item_sequence[] = {
  { &hf_s1ap_eCGI           , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_EUTRAN_CGI },
  { &hf_s1ap_numberOfBroadcasts, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_NumberOfBroadcasts },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_CancelledCellinTAI_Item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_CancelledCellinTAI_Item, CancelledCellinTAI_Item_sequence);

  return offset;
}


static const per_sequence_t CancelledCellinTAI_sequence_of[1] = {
  { &hf_s1ap_CancelledCellinTAI_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_CancelledCellinTAI_Item },
};

static int
dissect_s1ap_CancelledCellinTAI(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_CancelledCellinTAI, CancelledCellinTAI_sequence_of,
                                                  1, maxnoofCellinTAI, FALSE);

  return offset;
}


static const per_sequence_t TAI_Cancelled_Item_sequence[] = {
  { &hf_s1ap_tAI            , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TAI },
  { &hf_s1ap_cancelledCellinTAI, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_CancelledCellinTAI },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_TAI_Cancelled_Item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_TAI_Cancelled_Item, TAI_Cancelled_Item_sequence);

  return offset;
}


static const per_sequence_t TAI_Cancelled_sequence_of[1] = {
  { &hf_s1ap_TAI_Cancelled_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_TAI_Cancelled_Item },
};

static int
dissect_s1ap_TAI_Cancelled(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_TAI_Cancelled, TAI_Cancelled_sequence_of,
                                                  1, maxnoofTAIforWarning, FALSE);

  return offset;
}



static int
dissect_s1ap_EmergencyAreaID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       3, 3, FALSE, NULL);

  return offset;
}


static const per_sequence_t CancelledCellinEAI_Item_sequence[] = {
  { &hf_s1ap_eCGI           , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_EUTRAN_CGI },
  { &hf_s1ap_numberOfBroadcasts, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_NumberOfBroadcasts },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_CancelledCellinEAI_Item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_CancelledCellinEAI_Item, CancelledCellinEAI_Item_sequence);

  return offset;
}


static const per_sequence_t CancelledCellinEAI_sequence_of[1] = {
  { &hf_s1ap_CancelledCellinEAI_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_CancelledCellinEAI_Item },
};

static int
dissect_s1ap_CancelledCellinEAI(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_CancelledCellinEAI, CancelledCellinEAI_sequence_of,
                                                  1, maxnoofCellinEAI, FALSE);

  return offset;
}


static const per_sequence_t EmergencyAreaID_Cancelled_Item_sequence[] = {
  { &hf_s1ap_emergencyAreaID, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_EmergencyAreaID },
  { &hf_s1ap_cancelledCellinEAI, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_CancelledCellinEAI },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_EmergencyAreaID_Cancelled_Item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_EmergencyAreaID_Cancelled_Item, EmergencyAreaID_Cancelled_Item_sequence);

  return offset;
}


static const per_sequence_t EmergencyAreaID_Cancelled_sequence_of[1] = {
  { &hf_s1ap_EmergencyAreaID_Cancelled_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_EmergencyAreaID_Cancelled_Item },
};

static int
dissect_s1ap_EmergencyAreaID_Cancelled(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_EmergencyAreaID_Cancelled, EmergencyAreaID_Cancelled_sequence_of,
                                                  1, maxnoofEmergencyAreaID, FALSE);

  return offset;
}


static const value_string s1ap_BroadcastCancelledAreaList_vals[] = {
  {   0, "cellID-Cancelled" },
  {   1, "tAI-Cancelled" },
  {   2, "emergencyAreaID-Cancelled" },
  { 0, NULL }
};

static const per_choice_t BroadcastCancelledAreaList_choice[] = {
  {   0, &hf_s1ap_cellID_Cancelled, ASN1_EXTENSION_ROOT    , dissect_s1ap_CellID_Cancelled },
  {   1, &hf_s1ap_tAI_Cancelled  , ASN1_EXTENSION_ROOT    , dissect_s1ap_TAI_Cancelled },
  {   2, &hf_s1ap_emergencyAreaID_Cancelled, ASN1_EXTENSION_ROOT    , dissect_s1ap_EmergencyAreaID_Cancelled },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_BroadcastCancelledAreaList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_BroadcastCancelledAreaList, BroadcastCancelledAreaList_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t CellID_Broadcast_Item_sequence[] = {
  { &hf_s1ap_eCGI           , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_EUTRAN_CGI },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_CellID_Broadcast_Item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_CellID_Broadcast_Item, CellID_Broadcast_Item_sequence);

  return offset;
}


static const per_sequence_t CellID_Broadcast_sequence_of[1] = {
  { &hf_s1ap_CellID_Broadcast_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_CellID_Broadcast_Item },
};

static int
dissect_s1ap_CellID_Broadcast(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_CellID_Broadcast, CellID_Broadcast_sequence_of,
                                                  1, maxnoofCellID, FALSE);

  return offset;
}


static const per_sequence_t CompletedCellinTAI_Item_sequence[] = {
  { &hf_s1ap_eCGI           , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_EUTRAN_CGI },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_CompletedCellinTAI_Item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_CompletedCellinTAI_Item, CompletedCellinTAI_Item_sequence);

  return offset;
}


static const per_sequence_t CompletedCellinTAI_sequence_of[1] = {
  { &hf_s1ap_CompletedCellinTAI_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_CompletedCellinTAI_Item },
};

static int
dissect_s1ap_CompletedCellinTAI(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_CompletedCellinTAI, CompletedCellinTAI_sequence_of,
                                                  1, maxnoofCellinTAI, FALSE);

  return offset;
}


static const per_sequence_t TAI_Broadcast_Item_sequence[] = {
  { &hf_s1ap_tAI            , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TAI },
  { &hf_s1ap_completedCellinTAI, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_CompletedCellinTAI },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_TAI_Broadcast_Item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_TAI_Broadcast_Item, TAI_Broadcast_Item_sequence);

  return offset;
}


static const per_sequence_t TAI_Broadcast_sequence_of[1] = {
  { &hf_s1ap_TAI_Broadcast_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_TAI_Broadcast_Item },
};

static int
dissect_s1ap_TAI_Broadcast(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_TAI_Broadcast, TAI_Broadcast_sequence_of,
                                                  1, maxnoofTAIforWarning, FALSE);

  return offset;
}


static const per_sequence_t CompletedCellinEAI_Item_sequence[] = {
  { &hf_s1ap_eCGI           , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_EUTRAN_CGI },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_CompletedCellinEAI_Item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_CompletedCellinEAI_Item, CompletedCellinEAI_Item_sequence);

  return offset;
}


static const per_sequence_t CompletedCellinEAI_sequence_of[1] = {
  { &hf_s1ap_CompletedCellinEAI_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_CompletedCellinEAI_Item },
};

static int
dissect_s1ap_CompletedCellinEAI(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_CompletedCellinEAI, CompletedCellinEAI_sequence_of,
                                                  1, maxnoofCellinEAI, FALSE);

  return offset;
}


static const per_sequence_t EmergencyAreaID_Broadcast_Item_sequence[] = {
  { &hf_s1ap_emergencyAreaID, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_EmergencyAreaID },
  { &hf_s1ap_completedCellinEAI, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_CompletedCellinEAI },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_EmergencyAreaID_Broadcast_Item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_EmergencyAreaID_Broadcast_Item, EmergencyAreaID_Broadcast_Item_sequence);

  return offset;
}


static const per_sequence_t EmergencyAreaID_Broadcast_sequence_of[1] = {
  { &hf_s1ap_EmergencyAreaID_Broadcast_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_EmergencyAreaID_Broadcast_Item },
};

static int
dissect_s1ap_EmergencyAreaID_Broadcast(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_EmergencyAreaID_Broadcast, EmergencyAreaID_Broadcast_sequence_of,
                                                  1, maxnoofEmergencyAreaID, FALSE);

  return offset;
}


static const value_string s1ap_BroadcastCompletedAreaList_vals[] = {
  {   0, "cellID-Broadcast" },
  {   1, "tAI-Broadcast" },
  {   2, "emergencyAreaID-Broadcast" },
  { 0, NULL }
};

static const per_choice_t BroadcastCompletedAreaList_choice[] = {
  {   0, &hf_s1ap_cellID_Broadcast, ASN1_EXTENSION_ROOT    , dissect_s1ap_CellID_Broadcast },
  {   1, &hf_s1ap_tAI_Broadcast  , ASN1_EXTENSION_ROOT    , dissect_s1ap_TAI_Broadcast },
  {   2, &hf_s1ap_emergencyAreaID_Broadcast, ASN1_EXTENSION_ROOT    , dissect_s1ap_EmergencyAreaID_Broadcast },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_BroadcastCompletedAreaList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_BroadcastCompletedAreaList, BroadcastCompletedAreaList_choice,
                                 NULL);

  return offset;
}


const value_string s1ap_CauseRadioNetwork_vals[] = {
  {   0, "unspecified" },
  {   1, "tx2relocoverall-expiry" },
  {   2, "successful-handover" },
  {   3, "release-due-to-eutran-generated-reason" },
  {   4, "handover-cancelled" },
  {   5, "partial-handover" },
  {   6, "ho-failure-in-target-EPC-eNB-or-target-system" },
  {   7, "ho-target-not-allowed" },
  {   8, "tS1relocoverall-expiry" },
  {   9, "tS1relocprep-expiry" },
  {  10, "cell-not-available" },
  {  11, "unknown-targetID" },
  {  12, "no-radio-resources-available-in-target-cell" },
  {  13, "unknown-mme-ue-s1ap-id" },
  {  14, "unknown-enb-ue-s1ap-id" },
  {  15, "unknown-pair-ue-s1ap-id" },
  {  16, "handover-desirable-for-radio-reason" },
  {  17, "time-critical-handover" },
  {  18, "resource-optimisation-handover" },
  {  19, "reduce-load-in-serving-cell" },
  {  20, "user-inactivity" },
  {  21, "radio-connection-with-ue-lost" },
  {  22, "load-balancing-tau-required" },
  {  23, "cs-fallback-triggered" },
  {  24, "ue-not-available-for-ps-service" },
  {  25, "radio-resources-not-available" },
  {  26, "failure-in-radio-interface-procedure" },
  {  27, "invalid-qos-combination" },
  {  28, "interrat-redirection" },
  {  29, "interaction-with-other-procedure" },
  {  30, "unknown-E-RAB-ID" },
  {  31, "multiple-E-RAB-ID-instances" },
  {  32, "encryption-and-or-integrity-protection-algorithms-not-supported" },
  {  33, "s1-intra-system-handover-triggered" },
  {  34, "s1-inter-system-handover-triggered" },
  {  35, "x2-handover-triggered" },
  {  36, "redirection-towards-1xRTT" },
  {  37, "not-supported-QCI-value" },
  {  38, "invalid-CSG-Id" },
  { 0, NULL }
};

static value_string_ext s1ap_CauseRadioNetwork_vals_ext = VALUE_STRING_EXT_INIT(s1ap_CauseRadioNetwork_vals);


static int
dissect_s1ap_CauseRadioNetwork(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1327 "./asn1/s1ap/s1ap.cnf"
  guint32 value;
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     36, &value, TRUE, 3, NULL);

  col_append_fstr(actx->pinfo->cinfo, COL_INFO, " [RadioNetwork-cause=%s]", val_to_str_const(value, s1ap_CauseRadioNetwork_vals, "Unknown"));



  return offset;
}


const value_string s1ap_CauseTransport_vals[] = {
  {   0, "transport-resource-unavailable" },
  {   1, "unspecified" },
  { 0, NULL }
};


static int
dissect_s1ap_CauseTransport(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1332 "./asn1/s1ap/s1ap.cnf"
  guint32 value;
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, &value, TRUE, 0, NULL);

  col_append_fstr(actx->pinfo->cinfo, COL_INFO, " [Transport-cause=%s]", val_to_str_const(value, s1ap_CauseTransport_vals, "Unknown"));



  return offset;
}


const value_string s1ap_CauseNas_vals[] = {
  {   0, "normal-release" },
  {   1, "authentication-failure" },
  {   2, "detach" },
  {   3, "unspecified" },
  {   4, "csg-subscription-expiry" },
  { 0, NULL }
};


static int
dissect_s1ap_CauseNas(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1337 "./asn1/s1ap/s1ap.cnf"
  guint32 value;
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     4, &value, TRUE, 1, NULL);

  col_append_fstr(actx->pinfo->cinfo, COL_INFO, "  [NAS-cause=%s]", val_to_str_const(value, s1ap_CauseNas_vals, "Unknown"));



  return offset;
}


const value_string s1ap_CauseProtocol_vals[] = {
  {   0, "transfer-syntax-error" },
  {   1, "abstract-syntax-error-reject" },
  {   2, "abstract-syntax-error-ignore-and-notify" },
  {   3, "message-not-compatible-with-receiver-state" },
  {   4, "semantic-error" },
  {   5, "abstract-syntax-error-falsely-constructed-message" },
  {   6, "unspecified" },
  { 0, NULL }
};


static int
dissect_s1ap_CauseProtocol(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1342 "./asn1/s1ap/s1ap.cnf"
  guint32 value;
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     7, &value, TRUE, 0, NULL);

  col_append_fstr(actx->pinfo->cinfo, COL_INFO, " [Protocol-cause=%s]", val_to_str_const(value, s1ap_CauseProtocol_vals, "Unknown"));     



  return offset;
}


const value_string s1ap_CauseMisc_vals[] = {
  {   0, "control-processing-overload" },
  {   1, "not-enough-user-plane-processing-resources" },
  {   2, "hardware-failure" },
  {   3, "om-intervention" },
  {   4, "unspecified" },
  {   5, "unknown-PLMN" },
  { 0, NULL }
};


static int
dissect_s1ap_CauseMisc(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1347 "./asn1/s1ap/s1ap.cnf"
  guint32 value;
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     6, &value, TRUE, 0, NULL);

  col_append_fstr(actx->pinfo->cinfo, COL_INFO, " [Misc-cause=%s]", val_to_str_const(value, s1ap_CauseMisc_vals, "Unknown"));



  return offset;
}


static const value_string s1ap_Cause_vals[] = {
  {   0, "radioNetwork" },
  {   1, "transport" },
  {   2, "nas" },
  {   3, "protocol" },
  {   4, "misc" },
  { 0, NULL }
};

static const per_choice_t Cause_choice[] = {
  {   0, &hf_s1ap_radioNetwork   , ASN1_EXTENSION_ROOT    , dissect_s1ap_CauseRadioNetwork },
  {   1, &hf_s1ap_transport      , ASN1_EXTENSION_ROOT    , dissect_s1ap_CauseTransport },
  {   2, &hf_s1ap_nas            , ASN1_EXTENSION_ROOT    , dissect_s1ap_CauseNas },
  {   3, &hf_s1ap_protocol       , ASN1_EXTENSION_ROOT    , dissect_s1ap_CauseProtocol },
  {   4, &hf_s1ap_misc           , ASN1_EXTENSION_ROOT    , dissect_s1ap_CauseMisc },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_Cause(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_Cause, Cause_choice,
                                 NULL);

  return offset;
}


static const value_string s1ap_CellAccessMode_vals[] = {
  {   0, "hybrid" },
  { 0, NULL }
};


static int
dissect_s1ap_CellAccessMode(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     1, NULL, TRUE, 0, NULL);

  return offset;
}



static int
dissect_s1ap_Cdma2000PDU(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 541 "./asn1/s1ap/s1ap.cnf"

 tvbuff_t *parameter_tvb;
 proto_tree *subtree = NULL;

  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, &parameter_tvb);

  if (!parameter_tvb)
    return offset;

if (gcsna_handle) {
        subtree = proto_item_add_subtree(actx->created_item, ett_s1ap_Cdma2000PDU);
        call_dissector(gcsna_handle, parameter_tvb, actx->pinfo, subtree);
}




  return offset;
}


static const value_string s1ap_Cdma2000RATType_vals[] = {
  {   0, "hRPD" },
  {   1, "onexRTT" },
  { 0, NULL }
};


static int
dissect_s1ap_Cdma2000RATType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, TRUE, 0, NULL);

  return offset;
}



static int
dissect_s1ap_Cdma2000SectorID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 556 "./asn1/s1ap/s1ap.cnf"
/* 9.2.1.25
 * This IE is set to CDMA2000 Reference Cell ID
 * corresponding to the HRPD/1xRTT sector under
 * the HRPD AN/1xBS to which the eNB has initiated the UE
 * to handover to. The CDMA2000 Reference Cell
 * ID is statically configured in the eNB.
 */
 tvbuff_t *parameter_tvb;
 proto_tree *subtree = NULL;

  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, &parameter_tvb);

  if (!parameter_tvb)
    return offset;

  subtree = proto_item_add_subtree(actx->created_item, ett_s1ap_Cdma2000SectorID);
  dissect_a21_ie_common(parameter_tvb, actx->pinfo, NULL/* Top tree not needed */, subtree, 0,  0 /* message_type not needed */);




  return offset;
}


static const value_string s1ap_Cdma2000HOStatus_vals[] = {
  {   0, "hOSuccess" },
  {   1, "hOFailure" },
  { 0, NULL }
};


static int
dissect_s1ap_Cdma2000HOStatus(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string s1ap_Cdma2000HORequiredIndication_vals[] = {
  {   0, "true" },
  { 0, NULL }
};


static int
dissect_s1ap_Cdma2000HORequiredIndication(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     1, NULL, TRUE, 0, NULL);

  return offset;
}



static int
dissect_s1ap_Cdma2000OneXMEID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, NULL);

  return offset;
}



static int
dissect_s1ap_Cdma2000OneXMSI(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, NULL);

  return offset;
}



static int
dissect_s1ap_Cdma2000OneXPilot(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, NULL);

  return offset;
}


static const per_sequence_t Cdma2000OneXSRVCCInfo_sequence[] = {
  { &hf_s1ap_cdma2000OneXMEID, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_Cdma2000OneXMEID },
  { &hf_s1ap_cdma2000OneXMSI, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_Cdma2000OneXMSI },
  { &hf_s1ap_cdma2000OneXPilot, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_Cdma2000OneXPilot },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_Cdma2000OneXSRVCCInfo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_Cdma2000OneXSRVCCInfo, Cdma2000OneXSRVCCInfo_sequence);

  return offset;
}



static int
dissect_s1ap_Cdma2000OneXRAND(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, NULL);

  return offset;
}


static const value_string s1ap_Cell_Size_vals[] = {
  {   0, "verysmall" },
  {   1, "small" },
  {   2, "medium" },
  {   3, "large" },
  { 0, NULL }
};


static int
dissect_s1ap_Cell_Size(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     4, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t CellType_sequence[] = {
  { &hf_s1ap_cell_Size      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_Cell_Size },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_CellType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_CellType, CellType_sequence);

  return offset;
}



static int
dissect_s1ap_LAC(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       2, 2, FALSE, NULL);

  return offset;
}



static int
dissect_s1ap_CI(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       2, 2, FALSE, NULL);

  return offset;
}



static int
dissect_s1ap_RAC(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       1, 1, FALSE, NULL);

  return offset;
}


static const per_sequence_t CGI_sequence[] = {
  { &hf_s1ap_pLMNidentity   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_PLMNidentity },
  { &hf_s1ap_lAC            , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_LAC },
  { &hf_s1ap_cI             , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_CI },
  { &hf_s1ap_rAC            , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_RAC },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_CGI(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_CGI, CGI_sequence);

  return offset;
}


static const value_string s1ap_CNDomain_vals[] = {
  {   0, "ps" },
  {   1, "cs" },
  { 0, NULL }
};


static int
dissect_s1ap_CNDomain(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, FALSE, 0, NULL);

  return offset;
}


static const value_string s1ap_ConcurrentWarningMessageIndicator_vals[] = {
  {   0, "true" },
  { 0, NULL }
};


static int
dissect_s1ap_ConcurrentWarningMessageIndicator(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     1, NULL, FALSE, 0, NULL);

  return offset;
}



static int
dissect_s1ap_Correlation_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       4, 4, FALSE, NULL);

  return offset;
}


static const value_string s1ap_CSFallbackIndicator_vals[] = {
  {   0, "cs-fallback-required" },
  {   1, "cs-fallback-high-priority" },
  { 0, NULL }
};


static int
dissect_s1ap_CSFallbackIndicator(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     1, NULL, TRUE, 1, NULL);

  return offset;
}


static const value_string s1ap_AdditionalCSFallbackIndicator_vals[] = {
  {   0, "no-restriction" },
  {   1, "restriction" },
  { 0, NULL }
};


static int
dissect_s1ap_AdditionalCSFallbackIndicator(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, TRUE, 0, NULL);

  return offset;
}



static int
dissect_s1ap_CSG_Id(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     27, 27, FALSE, NULL, NULL);

  return offset;
}


static const per_sequence_t CSG_IdList_Item_sequence[] = {
  { &hf_s1ap_cSG_Id         , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_CSG_Id },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_CSG_IdList_Item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_CSG_IdList_Item, CSG_IdList_Item_sequence);

  return offset;
}


static const per_sequence_t CSG_IdList_sequence_of[1] = {
  { &hf_s1ap_CSG_IdList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_CSG_IdList_Item },
};

static int
dissect_s1ap_CSG_IdList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_CSG_IdList, CSG_IdList_sequence_of,
                                                  1, maxnoofCSGs, FALSE);

  return offset;
}


static const value_string s1ap_CSGMembershipStatus_vals[] = {
  {   0, "member" },
  {   1, "not-member" },
  { 0, NULL }
};


static int
dissect_s1ap_CSGMembershipStatus(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, FALSE, 0, NULL);

  return offset;
}



static int
dissect_s1ap_PDCP_SNExtended(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 32767U, NULL, FALSE);

  return offset;
}



static int
dissect_s1ap_HFNModified(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 131071U, NULL, FALSE);

  return offset;
}


static const per_sequence_t COUNTValueExtended_sequence[] = {
  { &hf_s1ap_pDCP_SNExtended, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_PDCP_SNExtended },
  { &hf_s1ap_hFNModified    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_HFNModified },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_COUNTValueExtended(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_COUNTValueExtended, COUNTValueExtended_sequence);

  return offset;
}



static int
dissect_s1ap_PDCP_SNlength18(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 262143U, NULL, FALSE);

  return offset;
}



static int
dissect_s1ap_HFNforPDCP_SNlength18(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 16383U, NULL, FALSE);

  return offset;
}


static const per_sequence_t COUNTvaluePDCP_SNlength18_sequence[] = {
  { &hf_s1ap_pDCP_SNlength18, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_PDCP_SNlength18 },
  { &hf_s1ap_hFNforPDCP_SNlength18, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_HFNforPDCP_SNlength18 },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_COUNTvaluePDCP_SNlength18(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_COUNTvaluePDCP_SNlength18, COUNTvaluePDCP_SNlength18_sequence);

  return offset;
}


static const value_string s1ap_TypeOfError_vals[] = {
  {   0, "not-understood" },
  {   1, "missing" },
  { 0, NULL }
};


static int
dissect_s1ap_TypeOfError(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t CriticalityDiagnostics_IE_Item_sequence[] = {
  { &hf_s1ap_iECriticality  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_Criticality },
  { &hf_s1ap_iE_ID          , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_ID },
  { &hf_s1ap_typeOfError    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TypeOfError },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_CriticalityDiagnostics_IE_Item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_CriticalityDiagnostics_IE_Item, CriticalityDiagnostics_IE_Item_sequence);

  return offset;
}


static const per_sequence_t CriticalityDiagnostics_IE_List_sequence_of[1] = {
  { &hf_s1ap_CriticalityDiagnostics_IE_List_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_CriticalityDiagnostics_IE_Item },
};

static int
dissect_s1ap_CriticalityDiagnostics_IE_List(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_CriticalityDiagnostics_IE_List, CriticalityDiagnostics_IE_List_sequence_of,
                                                  1, maxnoofErrors, FALSE);

  return offset;
}


static const per_sequence_t CriticalityDiagnostics_sequence[] = {
  { &hf_s1ap_procedureCode  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProcedureCode },
  { &hf_s1ap_triggeringMessage, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_TriggeringMessage },
  { &hf_s1ap_procedureCriticality, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_Criticality },
  { &hf_s1ap_iEsCriticalityDiagnostics, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_CriticalityDiagnostics_IE_List },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_CriticalityDiagnostics(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_CriticalityDiagnostics, CriticalityDiagnostics_sequence);

  return offset;
}



static int
dissect_s1ap_DataCodingScheme(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     8, 8, FALSE, NULL, NULL);

  return offset;
}


static const value_string s1ap_DL_Forwarding_vals[] = {
  {   0, "dL-Forwarding-proposed" },
  { 0, NULL }
};


static int
dissect_s1ap_DL_Forwarding(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     1, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string s1ap_Direct_Forwarding_Path_Availability_vals[] = {
  {   0, "directPathAvailable" },
  { 0, NULL }
};


static int
dissect_s1ap_Direct_Forwarding_Path_Availability(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     1, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string s1ap_Data_Forwarding_Not_Possible_vals[] = {
  {   0, "data-Forwarding-not-Possible" },
  { 0, NULL }
};


static int
dissect_s1ap_Data_Forwarding_Not_Possible(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     1, NULL, TRUE, 0, NULL);

  return offset;
}



static int
dissect_s1ap_EARFCN(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, maxEARFCN, NULL, TRUE);

  return offset;
}


static const per_sequence_t ECGIList_sequence_of[1] = {
  { &hf_s1ap_ECGIList_item  , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_EUTRAN_CGI },
};

static int
dissect_s1ap_ECGIList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_ECGIList, ECGIList_sequence_of,
                                                  1, maxnoofCellID, FALSE);

  return offset;
}


static const per_sequence_t PWSfailedECGIList_sequence_of[1] = {
  { &hf_s1ap_PWSfailedECGIList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_EUTRAN_CGI },
};

static int
dissect_s1ap_PWSfailedECGIList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_PWSfailedECGIList, PWSfailedECGIList_sequence_of,
                                                  1, maxnoofCellsineNB, FALSE);

  return offset;
}


static const per_sequence_t EmergencyAreaIDList_sequence_of[1] = {
  { &hf_s1ap_EmergencyAreaIDList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_EmergencyAreaID },
};

static int
dissect_s1ap_EmergencyAreaIDList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_EmergencyAreaIDList, EmergencyAreaIDList_sequence_of,
                                                  1, maxnoofEmergencyAreaID, FALSE);

  return offset;
}


static const per_sequence_t ECGI_List_sequence_of[1] = {
  { &hf_s1ap_ECGI_List_item , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_EUTRAN_CGI },
};

static int
dissect_s1ap_ECGI_List(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_ECGI_List, ECGI_List_sequence_of,
                                                  1, maxnoofCellsineNB, FALSE);

  return offset;
}


static const per_sequence_t EmergencyAreaIDListForRestart_sequence_of[1] = {
  { &hf_s1ap_EmergencyAreaIDListForRestart_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_EmergencyAreaID },
};

static int
dissect_s1ap_EmergencyAreaIDListForRestart(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_EmergencyAreaIDListForRestart, EmergencyAreaIDListForRestart_sequence_of,
                                                  1, maxnoofRestartEmergencyAreaIDs, FALSE);

  return offset;
}



static int
dissect_s1ap_BIT_STRING_SIZE_20(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     20, 20, FALSE, NULL, NULL);

  return offset;
}



static int
dissect_s1ap_BIT_STRING_SIZE_28(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     28, 28, FALSE, NULL, NULL);

  return offset;
}


static const value_string s1ap_ENB_ID_vals[] = {
  {   0, "macroENB-ID" },
  {   1, "homeENB-ID" },
  { 0, NULL }
};

static const per_choice_t ENB_ID_choice[] = {
  {   0, &hf_s1ap_macroENB_ID    , ASN1_EXTENSION_ROOT    , dissect_s1ap_BIT_STRING_SIZE_20 },
  {   1, &hf_s1ap_homeENB_ID     , ASN1_EXTENSION_ROOT    , dissect_s1ap_BIT_STRING_SIZE_28 },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_ENB_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_ENB_ID, ENB_ID_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t LAI_sequence[] = {
  { &hf_s1ap_pLMNidentity   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_PLMNidentity },
  { &hf_s1ap_lAC            , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_LAC },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_LAI(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_LAI, LAI_sequence);

  return offset;
}


static const per_sequence_t GERAN_Cell_ID_sequence[] = {
  { &hf_s1ap_lAI            , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_LAI },
  { &hf_s1ap_rAC            , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_RAC },
  { &hf_s1ap_cI             , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_CI },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_GERAN_Cell_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_GERAN_Cell_ID, GERAN_Cell_ID_sequence);

  return offset;
}


static const per_sequence_t Global_ENB_ID_sequence[] = {
  { &hf_s1ap_pLMNidentity   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_PLMNidentity },
  { &hf_s1ap_eNB_ID         , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ENB_ID },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

int
dissect_s1ap_Global_ENB_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_Global_ENB_ID, Global_ENB_ID_sequence);

  return offset;
}


static const per_sequence_t GUMMEIList_sequence_of[1] = {
  { &hf_s1ap_GUMMEIList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_GUMMEI },
};

static int
dissect_s1ap_GUMMEIList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_GUMMEIList, GUMMEIList_sequence_of,
                                                  1, maxnoofMMECs, FALSE);

  return offset;
}


static const per_sequence_t ENB_StatusTransfer_TransparentContainer_sequence[] = {
  { &hf_s1ap_bearers_SubjectToStatusTransferList, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_Bearers_SubjectToStatusTransferList },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_ENB_StatusTransfer_TransparentContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_ENB_StatusTransfer_TransparentContainer, ENB_StatusTransfer_TransparentContainer_sequence);

  return offset;
}



static int
dissect_s1ap_ENB_UE_S1AP_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 16777215U, NULL, FALSE);

  return offset;
}



static int
dissect_s1ap_ENBname(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 220 "./asn1/s1ap/s1ap.cnf"
  tvbuff_t *parameter_tvb=NULL;
  int length;
  int p_offset;
  gboolean is_ascii;

  offset = dissect_per_PrintableString(tvb, offset, actx, tree, hf_index,
                                          1, 150, TRUE);


  if (!parameter_tvb)
    return offset;

  length = tvb_reported_length(parameter_tvb);

  is_ascii = TRUE;
  for (p_offset=0; p_offset < length; p_offset++){
	 if(!g_ascii_isprint(tvb_get_guint8(parameter_tvb, p_offset ))){
		is_ascii = FALSE;
		break;
	 }
  }
  if (is_ascii)
  		proto_item_append_text(actx->created_item,"(%s)",tvb_format_text(parameter_tvb, 0, length));




  return offset;
}



static int
dissect_s1ap_TransportLayerAddress(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 275 "./asn1/s1ap/s1ap.cnf"
  tvbuff_t *parameter_tvb=NULL;
  proto_tree *subtree;
  gint tvb_len;

  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     1, 160, TRUE, &parameter_tvb, NULL);

  if (!parameter_tvb)
    return offset;

  /* Get the length */
  tvb_len = tvb_reported_length(parameter_tvb);
  subtree = proto_item_add_subtree(actx->created_item, ett_s1ap_TransportLayerAddress);
  if (tvb_len==4) {
    /* IPv4 */
     proto_tree_add_item(subtree, hf_s1ap_transportLayerAddressIPv4, parameter_tvb, 0, 4, ENC_BIG_ENDIAN);
  } else if (tvb_len==16) {
    /* IPv6 */
     proto_tree_add_item(subtree, hf_s1ap_transportLayerAddressIPv6, parameter_tvb, 0, 16, ENC_NA);
  } else if (tvb_len==20) {
    /* IPv4 */
     proto_tree_add_item(subtree, hf_s1ap_transportLayerAddressIPv4, parameter_tvb, 0, 4, ENC_BIG_ENDIAN);
    /* IPv6 */
     proto_tree_add_item(subtree, hf_s1ap_transportLayerAddressIPv6, parameter_tvb, 4, 16, ENC_NA);
  }



  return offset;
}


static const per_sequence_t ENBX2TLAs_sequence_of[1] = {
  { &hf_s1ap_ENBX2TLAs_item , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_TransportLayerAddress },
};

static int
dissect_s1ap_ENBX2TLAs(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_ENBX2TLAs, ENBX2TLAs_sequence_of,
                                                  1, maxnoofeNBX2TLAs, FALSE);

  return offset;
}



static int
dissect_s1ap_EncryptionAlgorithms(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     16, 16, TRUE, NULL, NULL);

  return offset;
}


static const per_sequence_t EPLMNs_sequence_of[1] = {
  { &hf_s1ap_EPLMNs_item    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_PLMNidentity },
};

static int
dissect_s1ap_EPLMNs(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_EPLMNs, EPLMNs_sequence_of,
                                                  1, maxnoofEPLMNs, FALSE);

  return offset;
}


static const value_string s1ap_EventType_vals[] = {
  {   0, "direct" },
  {   1, "change-of-serve-cell" },
  {   2, "stop-change-of-serve-cell" },
  { 0, NULL }
};


static int
dissect_s1ap_EventType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     3, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t E_RABInformationList_sequence_of[1] = {
  { &hf_s1ap_E_RABInformationList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_SingleContainer },
};

static int
dissect_s1ap_E_RABInformationList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_E_RABInformationList, E_RABInformationList_sequence_of,
                                                  1, maxnoofE_RABs, FALSE);

  return offset;
}


static const per_sequence_t E_RABInformationListItem_sequence[] = {
  { &hf_s1ap_e_RAB_ID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_E_RAB_ID },
  { &hf_s1ap_dL_Forwarding  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_DL_Forwarding },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABInformationListItem(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABInformationListItem, E_RABInformationListItem_sequence);

  return offset;
}


static const per_sequence_t E_RABList_sequence_of[1] = {
  { &hf_s1ap_E_RABList_item , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_SingleContainer },
};

static int
dissect_s1ap_E_RABList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_E_RABList, E_RABList_sequence_of,
                                                  1, maxnoofE_RABs, FALSE);

  return offset;
}


static const per_sequence_t E_RABItem_sequence[] = {
  { &hf_s1ap_e_RAB_ID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_E_RAB_ID },
  { &hf_s1ap_cause          , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_Cause },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABItem(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABItem, E_RABItem_sequence);

  return offset;
}



static int
dissect_s1ap_QCI(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, NULL, FALSE);

  return offset;
}


static const per_sequence_t GBR_QosInformation_sequence[] = {
  { &hf_s1ap_e_RAB_MaximumBitrateDL, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_BitRate },
  { &hf_s1ap_e_RAB_MaximumBitrateUL, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_BitRate },
  { &hf_s1ap_e_RAB_GuaranteedBitrateDL, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_BitRate },
  { &hf_s1ap_e_RAB_GuaranteedBitrateUL, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_BitRate },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_GBR_QosInformation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_GBR_QosInformation, GBR_QosInformation_sequence);

  return offset;
}


static const per_sequence_t E_RABLevelQoSParameters_sequence[] = {
  { &hf_s1ap_qCI            , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_QCI },
  { &hf_s1ap_allocationRetentionPriority, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_AllocationAndRetentionPriority },
  { &hf_s1ap_gbrQosInformation, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_GBR_QosInformation },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABLevelQoSParameters(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABLevelQoSParameters, E_RABLevelQoSParameters_sequence);

  return offset;
}



static int
dissect_s1ap_EUTRANRoundTripDelayEstimationInfo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 2047U, NULL, FALSE);

  return offset;
}



static int
dissect_s1ap_ExpectedActivityPeriod(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 181U, NULL, FALSE);

  return offset;
}



static int
dissect_s1ap_ExpectedIdlePeriod(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 181U, NULL, FALSE);

  return offset;
}


static const value_string s1ap_SourceOfUEActivityBehaviourInformation_vals[] = {
  {   0, "subscription-information" },
  {   1, "statistics" },
  { 0, NULL }
};


static int
dissect_s1ap_SourceOfUEActivityBehaviourInformation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t ExpectedUEActivityBehaviour_sequence[] = {
  { &hf_s1ap_expectedActivityPeriod, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ExpectedActivityPeriod },
  { &hf_s1ap_expectedIdlePeriod, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ExpectedIdlePeriod },
  { &hf_s1ap_sourceofUEActivityBehaviourInformation, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_SourceOfUEActivityBehaviourInformation },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_ExpectedUEActivityBehaviour(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_ExpectedUEActivityBehaviour, ExpectedUEActivityBehaviour_sequence);

  return offset;
}


static const value_string s1ap_ExpectedHOInterval_vals[] = {
  {   0, "sec15" },
  {   1, "sec30" },
  {   2, "sec60" },
  {   3, "sec90" },
  {   4, "sec120" },
  {   5, "sec180" },
  {   6, "long-time" },
  { 0, NULL }
};


static int
dissect_s1ap_ExpectedHOInterval(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     7, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t ExpectedUEBehaviour_sequence[] = {
  { &hf_s1ap_expectedActivity, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ExpectedUEActivityBehaviour },
  { &hf_s1ap_expectedHOInterval, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ExpectedHOInterval },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_ExpectedUEBehaviour(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_ExpectedUEBehaviour, ExpectedUEBehaviour_sequence);

  return offset;
}



static int
dissect_s1ap_ExtendedRNC_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            4096U, 65535U, NULL, FALSE);

  return offset;
}



static int
dissect_s1ap_ExtendedRepetitionPeriod(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            4096U, 131071U, NULL, FALSE);

  return offset;
}


static const value_string s1ap_ForbiddenInterRATs_vals[] = {
  {   0, "all" },
  {   1, "geran" },
  {   2, "utran" },
  {   3, "cdma2000" },
  {   4, "geranandutran" },
  {   5, "cdma2000andutran" },
  { 0, NULL }
};


static int
dissect_s1ap_ForbiddenInterRATs(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     4, NULL, TRUE, 2, NULL);

  return offset;
}


static const per_sequence_t ForbiddenTACs_sequence_of[1] = {
  { &hf_s1ap_ForbiddenTACs_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_TAC },
};

static int
dissect_s1ap_ForbiddenTACs(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_ForbiddenTACs, ForbiddenTACs_sequence_of,
                                                  1, maxnoofForbTACs, FALSE);

  return offset;
}


static const per_sequence_t ForbiddenTAs_Item_sequence[] = {
  { &hf_s1ap_pLMN_Identity  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_PLMNidentity },
  { &hf_s1ap_forbiddenTACs  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ForbiddenTACs },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_ForbiddenTAs_Item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_ForbiddenTAs_Item, ForbiddenTAs_Item_sequence);

  return offset;
}


static const per_sequence_t ForbiddenTAs_sequence_of[1] = {
  { &hf_s1ap_ForbiddenTAs_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ForbiddenTAs_Item },
};

static int
dissect_s1ap_ForbiddenTAs(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_ForbiddenTAs, ForbiddenTAs_sequence_of,
                                                  1, maxnoofEPLMNsPlusOne, FALSE);

  return offset;
}


static const per_sequence_t ForbiddenLACs_sequence_of[1] = {
  { &hf_s1ap_ForbiddenLACs_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_LAC },
};

static int
dissect_s1ap_ForbiddenLACs(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_ForbiddenLACs, ForbiddenLACs_sequence_of,
                                                  1, maxnoofForbLACs, FALSE);

  return offset;
}


static const per_sequence_t ForbiddenLAs_Item_sequence[] = {
  { &hf_s1ap_pLMN_Identity  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_PLMNidentity },
  { &hf_s1ap_forbiddenLACs  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ForbiddenLACs },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_ForbiddenLAs_Item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_ForbiddenLAs_Item, ForbiddenLAs_Item_sequence);

  return offset;
}


static const per_sequence_t ForbiddenLAs_sequence_of[1] = {
  { &hf_s1ap_ForbiddenLAs_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ForbiddenLAs_Item },
};

static int
dissect_s1ap_ForbiddenLAs(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_ForbiddenLAs, ForbiddenLAs_sequence_of,
                                                  1, maxnoofEPLMNsPlusOne, FALSE);

  return offset;
}



static int
dissect_s1ap_GTP_TEID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       4, 4, FALSE, NULL);

  return offset;
}


static const value_string s1ap_GUMMEIType_vals[] = {
  {   0, "native" },
  {   1, "mapped" },
  { 0, NULL }
};


static int
dissect_s1ap_GUMMEIType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string s1ap_GWContextReleaseIndication_vals[] = {
  {   0, "true" },
  { 0, NULL }
};


static int
dissect_s1ap_GWContextReleaseIndication(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     1, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t HandoverRestrictionList_sequence[] = {
  { &hf_s1ap_servingPLMN    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_PLMNidentity },
  { &hf_s1ap_equivalentPLMNs, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_EPLMNs },
  { &hf_s1ap_forbiddenTAs   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ForbiddenTAs },
  { &hf_s1ap_forbiddenLAs   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ForbiddenLAs },
  { &hf_s1ap_forbiddenInterRATs, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ForbiddenInterRATs },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_HandoverRestrictionList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_HandoverRestrictionList, HandoverRestrictionList_sequence);

  return offset;
}


static const value_string s1ap_HandoverType_vals[] = {
  {   0, "intralte" },
  {   1, "ltetoutran" },
  {   2, "ltetogeran" },
  {   3, "utrantolte" },
  {   4, "gerantolte" },
  { 0, NULL }
};


static int
dissect_s1ap_HandoverType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 316 "./asn1/s1ap/s1ap.cnf"

  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     5, &handover_type_value, TRUE, 0, NULL);




  return offset;
}



static int
dissect_s1ap_Masked_IMEISV(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     64, 64, FALSE, NULL, NULL);

  return offset;
}



static int
dissect_s1ap_MeasurementsToActivate(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     8, 8, FALSE, NULL, NULL);

  return offset;
}


static const value_string s1ap_M1ReportingTrigger_vals[] = {
  {   0, "periodic" },
  {   1, "a2eventtriggered" },
  {   2, "a2eventtriggered-periodic" },
  { 0, NULL }
};


static int
dissect_s1ap_M1ReportingTrigger(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, TRUE, 1, NULL);

  return offset;
}



static int
dissect_s1ap_Threshold_RSRP(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 97U, NULL, FALSE);

  return offset;
}



static int
dissect_s1ap_Threshold_RSRQ(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 34U, NULL, FALSE);

  return offset;
}


static const value_string s1ap_MeasurementThresholdA2_vals[] = {
  {   0, "threshold-RSRP" },
  {   1, "threshold-RSRQ" },
  { 0, NULL }
};

static const per_choice_t MeasurementThresholdA2_choice[] = {
  {   0, &hf_s1ap_threshold_RSRP , ASN1_EXTENSION_ROOT    , dissect_s1ap_Threshold_RSRP },
  {   1, &hf_s1ap_threshold_RSRQ , ASN1_EXTENSION_ROOT    , dissect_s1ap_Threshold_RSRQ },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_MeasurementThresholdA2(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_MeasurementThresholdA2, MeasurementThresholdA2_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t M1ThresholdEventA2_sequence[] = {
  { &hf_s1ap_measurementThreshold, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_MeasurementThresholdA2 },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_M1ThresholdEventA2(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_M1ThresholdEventA2, M1ThresholdEventA2_sequence);

  return offset;
}


static const value_string s1ap_ReportIntervalMDT_vals[] = {
  {   0, "ms120" },
  {   1, "ms240" },
  {   2, "ms480" },
  {   3, "ms640" },
  {   4, "ms1024" },
  {   5, "ms2048" },
  {   6, "ms5120" },
  {   7, "ms10240" },
  {   8, "min1" },
  {   9, "min6" },
  {  10, "min12" },
  {  11, "min30" },
  {  12, "min60" },
  { 0, NULL }
};


static int
dissect_s1ap_ReportIntervalMDT(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     13, NULL, FALSE, 0, NULL);

  return offset;
}


static const value_string s1ap_ReportAmountMDT_vals[] = {
  {   0, "r1" },
  {   1, "r2" },
  {   2, "r4" },
  {   3, "r8" },
  {   4, "r16" },
  {   5, "r32" },
  {   6, "r64" },
  {   7, "rinfinity" },
  { 0, NULL }
};


static int
dissect_s1ap_ReportAmountMDT(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     8, NULL, FALSE, 0, NULL);

  return offset;
}


static const per_sequence_t M1PeriodicReporting_sequence[] = {
  { &hf_s1ap_reportInterval , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ReportIntervalMDT },
  { &hf_s1ap_reportAmount   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ReportAmountMDT },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_M1PeriodicReporting(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_M1PeriodicReporting, M1PeriodicReporting_sequence);

  return offset;
}


static const per_sequence_t ImmediateMDT_sequence[] = {
  { &hf_s1ap_measurementsToActivate, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_MeasurementsToActivate },
  { &hf_s1ap_m1reportingTrigger, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_M1ReportingTrigger },
  { &hf_s1ap_m1thresholdeventA2, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_M1ThresholdEventA2 },
  { &hf_s1ap_m1periodicReporting, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_M1PeriodicReporting },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_ImmediateMDT(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_ImmediateMDT, ImmediateMDT_sequence);

  return offset;
}



static int
dissect_s1ap_IMSI(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       3, 8, FALSE, NULL);

  return offset;
}



static int
dissect_s1ap_IntegrityProtectionAlgorithms(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     16, 16, TRUE, NULL, NULL);

  return offset;
}



static int
dissect_s1ap_InterfacesToTrace(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     8, 8, FALSE, NULL, NULL);

  return offset;
}


static const value_string s1ap_KillAllWarningMessages_vals[] = {
  {   0, "true" },
  { 0, NULL }
};


static int
dissect_s1ap_KillAllWarningMessages(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     1, NULL, FALSE, 0, NULL);

  return offset;
}



static int
dissect_s1ap_Time_UE_StayedInCell(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 4095U, NULL, FALSE);

  return offset;
}


static const per_sequence_t LastVisitedEUTRANCellInformation_sequence[] = {
  { &hf_s1ap_global_Cell_ID , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_EUTRAN_CGI },
  { &hf_s1ap_cellType       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_CellType },
  { &hf_s1ap_time_UE_StayedInCell, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_Time_UE_StayedInCell },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_LastVisitedEUTRANCellInformation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_LastVisitedEUTRANCellInformation, LastVisitedEUTRANCellInformation_sequence);

  return offset;
}



static int
dissect_s1ap_LastVisitedUTRANCellInformation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, NULL);

  return offset;
}


static const value_string s1ap_LastVisitedGERANCellInformation_vals[] = {
  {   0, "undefined" },
  { 0, NULL }
};

static const per_choice_t LastVisitedGERANCellInformation_choice[] = {
  {   0, &hf_s1ap_undefined      , ASN1_EXTENSION_ROOT    , dissect_s1ap_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_LastVisitedGERANCellInformation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_LastVisitedGERANCellInformation, LastVisitedGERANCellInformation_choice,
                                 NULL);

  return offset;
}


static const value_string s1ap_LastVisitedCell_Item_vals[] = {
  {   0, "e-UTRAN-Cell" },
  {   1, "uTRAN-Cell" },
  {   2, "gERAN-Cell" },
  { 0, NULL }
};

static const per_choice_t LastVisitedCell_Item_choice[] = {
  {   0, &hf_s1ap_e_UTRAN_Cell   , ASN1_EXTENSION_ROOT    , dissect_s1ap_LastVisitedEUTRANCellInformation },
  {   1, &hf_s1ap_uTRAN_Cell     , ASN1_EXTENSION_ROOT    , dissect_s1ap_LastVisitedUTRANCellInformation },
  {   2, &hf_s1ap_gERAN_Cell     , ASN1_EXTENSION_ROOT    , dissect_s1ap_LastVisitedGERANCellInformation },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_LastVisitedCell_Item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_LastVisitedCell_Item, LastVisitedCell_Item_choice,
                                 NULL);

  return offset;
}



static int
dissect_s1ap_LPPa_PDU(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 266 "./asn1/s1ap/s1ap.cnf"

  tvbuff_t *parameter_tvb=NULL;

  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, &parameter_tvb);


  if ((tvb_reported_length(parameter_tvb)>0)&&(lppa_handle))
    call_dissector(lppa_handle, parameter_tvb, actx->pinfo, tree);



  return offset;
}



static int
dissect_s1ap_LHN_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       32, 256, FALSE, NULL);

  return offset;
}


static const value_string s1ap_Links_to_log_vals[] = {
  {   0, "uplink" },
  {   1, "downlink" },
  {   2, "both-uplink-and-downlink" },
  { 0, NULL }
};


static int
dissect_s1ap_Links_to_log(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     3, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string s1ap_T_pattern_period_vals[] = {
  {   0, "ms1280" },
  {   1, "ms2560" },
  {   2, "ms5120" },
  {   3, "ms10240" },
  { 0, NULL }
};


static int
dissect_s1ap_T_pattern_period(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     4, NULL, TRUE, 0, NULL);

  return offset;
}



static int
dissect_s1ap_INTEGER_0_10239_(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 10239U, NULL, TRUE);

  return offset;
}


static const per_sequence_t ListeningSubframePattern_sequence[] = {
  { &hf_s1ap_pattern_period , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_T_pattern_period },
  { &hf_s1ap_pattern_offset , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_INTEGER_0_10239_ },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_ListeningSubframePattern(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_ListeningSubframePattern, ListeningSubframePattern_sequence);

  return offset;
}


static const value_string s1ap_LoggingInterval_vals[] = {
  {   0, "ms128" },
  {   1, "ms256" },
  {   2, "ms512" },
  {   3, "ms1024" },
  {   4, "ms2048" },
  {   5, "ms3072" },
  {   6, "ms4096" },
  {   7, "ms6144" },
  { 0, NULL }
};


static int
dissect_s1ap_LoggingInterval(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     8, NULL, FALSE, 0, NULL);

  return offset;
}


static const value_string s1ap_LoggingDuration_vals[] = {
  {   0, "m10" },
  {   1, "m20" },
  {   2, "m40" },
  {   3, "m60" },
  {   4, "m90" },
  {   5, "m120" },
  { 0, NULL }
};


static int
dissect_s1ap_LoggingDuration(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     6, NULL, FALSE, 0, NULL);

  return offset;
}


static const per_sequence_t LoggedMDT_sequence[] = {
  { &hf_s1ap_loggingInterval, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_LoggingInterval },
  { &hf_s1ap_loggingDuration, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_LoggingDuration },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_LoggedMDT(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_LoggedMDT, LoggedMDT_sequence);

  return offset;
}



static int
dissect_s1ap_INTEGER_0_255(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, NULL, FALSE);

  return offset;
}


static const per_sequence_t MBSFN_ResultToLogInfo_sequence[] = {
  { &hf_s1ap_mBSFN_AreaId   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_INTEGER_0_255 },
  { &hf_s1ap_carrierFreq    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_EARFCN },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_MBSFN_ResultToLogInfo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_MBSFN_ResultToLogInfo, MBSFN_ResultToLogInfo_sequence);

  return offset;
}


static const per_sequence_t MBSFN_ResultToLog_sequence_of[1] = {
  { &hf_s1ap_MBSFN_ResultToLog_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_MBSFN_ResultToLogInfo },
};

static int
dissect_s1ap_MBSFN_ResultToLog(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_MBSFN_ResultToLog, MBSFN_ResultToLog_sequence_of,
                                                  1, maxnoofMBSFNAreaMDT, FALSE);

  return offset;
}


static const per_sequence_t LoggedMBSFNMDT_sequence[] = {
  { &hf_s1ap_loggingInterval, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_LoggingInterval },
  { &hf_s1ap_loggingDuration, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_LoggingDuration },
  { &hf_s1ap_mBSFN_ResultToLog, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_MBSFN_ResultToLog },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_LoggedMBSFNMDT(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_LoggedMBSFNMDT, LoggedMBSFNMDT_sequence);

  return offset;
}


static const value_string s1ap_M3period_vals[] = {
  {   0, "ms100" },
  {   1, "ms1000" },
  {   2, "ms10000" },
  { 0, NULL }
};


static int
dissect_s1ap_M3period(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     3, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t M3Configuration_sequence[] = {
  { &hf_s1ap_m3period       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_M3period },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_M3Configuration(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_M3Configuration, M3Configuration_sequence);

  return offset;
}


static const value_string s1ap_M4period_vals[] = {
  {   0, "ms1024" },
  {   1, "ms2048" },
  {   2, "ms5120" },
  {   3, "ms10240" },
  {   4, "min1" },
  { 0, NULL }
};


static int
dissect_s1ap_M4period(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     5, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t M4Configuration_sequence[] = {
  { &hf_s1ap_m4period       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_M4period },
  { &hf_s1ap_m4_links_to_log, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_Links_to_log },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_M4Configuration(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_M4Configuration, M4Configuration_sequence);

  return offset;
}


static const value_string s1ap_M5period_vals[] = {
  {   0, "ms1024" },
  {   1, "ms2048" },
  {   2, "ms5120" },
  {   3, "ms10240" },
  {   4, "min1" },
  { 0, NULL }
};


static int
dissect_s1ap_M5period(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     5, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t M5Configuration_sequence[] = {
  { &hf_s1ap_m5period       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_M5period },
  { &hf_s1ap_m5_links_to_log, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_Links_to_log },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_M5Configuration(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_M5Configuration, M5Configuration_sequence);

  return offset;
}


static const value_string s1ap_M6report_Interval_vals[] = {
  {   0, "ms1024" },
  {   1, "ms2048" },
  {   2, "ms5120" },
  {   3, "ms10240" },
  { 0, NULL }
};


static int
dissect_s1ap_M6report_Interval(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     4, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string s1ap_M6delay_threshold_vals[] = {
  {   0, "ms30" },
  {   1, "ms40" },
  {   2, "ms50" },
  {   3, "ms60" },
  {   4, "ms70" },
  {   5, "ms80" },
  {   6, "ms90" },
  {   7, "ms100" },
  {   8, "ms150" },
  {   9, "ms300" },
  {  10, "ms500" },
  {  11, "ms750" },
  { 0, NULL }
};


static int
dissect_s1ap_M6delay_threshold(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     12, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t M6Configuration_sequence[] = {
  { &hf_s1ap_m6report_Interval, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_M6report_Interval },
  { &hf_s1ap_m6delay_threshold, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_M6delay_threshold },
  { &hf_s1ap_m6_links_to_log, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_Links_to_log },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_M6Configuration(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_M6Configuration, M6Configuration_sequence);

  return offset;
}



static int
dissect_s1ap_M7period(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 60U, NULL, TRUE);

  return offset;
}


static const per_sequence_t M7Configuration_sequence[] = {
  { &hf_s1ap_m7period       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_M7period },
  { &hf_s1ap_m7_links_to_log, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_Links_to_log },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_M7Configuration(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_M7Configuration, M7Configuration_sequence);

  return offset;
}


static const value_string s1ap_MDT_Activation_vals[] = {
  {   0, "immediate-MDT-only" },
  {   1, "immediate-MDT-and-Trace" },
  {   2, "logged-MDT-only" },
  {   3, "logged-MBSFN-MDT" },
  { 0, NULL }
};


static int
dissect_s1ap_MDT_Activation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     3, NULL, TRUE, 1, NULL);

  return offset;
}



static int
dissect_s1ap_MDT_Location_Info(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     8, 8, FALSE, NULL, NULL);

  return offset;
}



static int
dissect_s1ap_MDTMode_Extension(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_s1ap_ProtocolIE_SingleContainer(tvb, offset, actx, tree, hf_index);

  return offset;
}


static const value_string s1ap_MDTMode_vals[] = {
  {   0, "immediateMDT" },
  {   1, "loggedMDT" },
  {   2, "mDTMode-Extension" },
  { 0, NULL }
};

static const per_choice_t MDTMode_choice[] = {
  {   0, &hf_s1ap_immediateMDT   , ASN1_EXTENSION_ROOT    , dissect_s1ap_ImmediateMDT },
  {   1, &hf_s1ap_loggedMDT      , ASN1_EXTENSION_ROOT    , dissect_s1ap_LoggedMDT },
  {   2, &hf_s1ap_mDTMode_Extension, ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_MDTMode_Extension },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_MDTMode(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_MDTMode, MDTMode_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t MDT_Configuration_sequence[] = {
  { &hf_s1ap_mdt_Activation , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_MDT_Activation },
  { &hf_s1ap_areaScopeOfMDT , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_AreaScopeOfMDT },
  { &hf_s1ap_mDTMode        , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_MDTMode },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_MDT_Configuration(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_MDT_Configuration, MDT_Configuration_sequence);

  return offset;
}


static const value_string s1ap_ManagementBasedMDTAllowed_vals[] = {
  {   0, "allowed" },
  { 0, NULL }
};


static int
dissect_s1ap_ManagementBasedMDTAllowed(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     1, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t MDTPLMNList_sequence_of[1] = {
  { &hf_s1ap_MDTPLMNList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_PLMNidentity },
};

static int
dissect_s1ap_MDTPLMNList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_MDTPLMNList, MDTPLMNList_sequence_of,
                                                  1, maxnoofMDTPLMNs, FALSE);

  return offset;
}


static const value_string s1ap_PrivacyIndicator_vals[] = {
  {   0, "immediate-MDT" },
  {   1, "logged-MDT" },
  { 0, NULL }
};


static int
dissect_s1ap_PrivacyIndicator(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, TRUE, 0, NULL);

  return offset;
}



static int
dissect_s1ap_MessageIdentifier(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     16, 16, FALSE, NULL, NULL);

  return offset;
}



static int
dissect_s1ap_MobilityInformation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     32, 32, FALSE, NULL, NULL);

  return offset;
}



static int
dissect_s1ap_MMEname(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_PrintableString(tvb, offset, actx, tree, hf_index,
                                          1, 150, TRUE);

  return offset;
}


static const value_string s1ap_MMERelaySupportIndicator_vals[] = {
  {   0, "true" },
  { 0, NULL }
};


static int
dissect_s1ap_MMERelaySupportIndicator(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     1, NULL, TRUE, 0, NULL);

  return offset;
}



static int
dissect_s1ap_MME_UE_S1AP_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 4294967295U, NULL, FALSE);

  return offset;
}



static int
dissect_s1ap_MSClassmark2(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, NULL);

  return offset;
}



static int
dissect_s1ap_MSClassmark3(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, NULL);

  return offset;
}


static const value_string s1ap_MutingAvailabilityIndication_vals[] = {
  {   0, "available" },
  {   1, "unavailable" },
  { 0, NULL }
};


static int
dissect_s1ap_MutingAvailabilityIndication(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string s1ap_T_muting_pattern_period_vals[] = {
  {   0, "ms0" },
  {   1, "ms1280" },
  {   2, "ms2560" },
  {   3, "ms5120" },
  {   4, "ms10240" },
  { 0, NULL }
};


static int
dissect_s1ap_T_muting_pattern_period(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     5, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t MutingPatternInformation_sequence[] = {
  { &hf_s1ap_muting_pattern_period, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_T_muting_pattern_period },
  { &hf_s1ap_muting_pattern_offset, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_INTEGER_0_10239_ },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_MutingPatternInformation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_MutingPatternInformation, MutingPatternInformation_sequence);

  return offset;
}



static int
dissect_s1ap_NAS_PDU(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 257 "./asn1/s1ap/s1ap.cnf"

tvbuff_t *parameter_tvb=NULL;

  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, &parameter_tvb);


  if ((tvb_reported_length(parameter_tvb)>0)&&(nas_eps_handle))
    call_dissector(nas_eps_handle,parameter_tvb,actx->pinfo, tree);



  return offset;
}



static int
dissect_s1ap_NASSecurityParametersfromE_UTRAN(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, NULL);

  return offset;
}



static int
dissect_s1ap_NASSecurityParameterstoE_UTRAN(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, NULL);

  return offset;
}



static int
dissect_s1ap_NumberofBroadcastRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 65535U, NULL, FALSE);

  return offset;
}


static const value_string s1ap_OverloadAction_vals[] = {
  {   0, "reject-non-emergency-mo-dt" },
  {   1, "reject-rrc-cr-signalling" },
  {   2, "permit-emergency-sessions-and-mobile-terminated-services-only" },
  {   3, "permit-high-priority-sessions-and-mobile-terminated-services-only" },
  {   4, "reject-delay-tolerant-access" },
  { 0, NULL }
};


static int
dissect_s1ap_OverloadAction(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     3, NULL, TRUE, 2, NULL);

  return offset;
}


static const value_string s1ap_OverloadResponse_vals[] = {
  {   0, "overloadAction" },
  { 0, NULL }
};

static const per_choice_t OverloadResponse_choice[] = {
  {   0, &hf_s1ap_overloadAction , ASN1_EXTENSION_ROOT    , dissect_s1ap_OverloadAction },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_OverloadResponse(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_OverloadResponse, OverloadResponse_choice,
                                 NULL);

  return offset;
}


static const value_string s1ap_PagingDRX_vals[] = {
  {   0, "v32" },
  {   1, "v64" },
  {   2, "v128" },
  {   3, "v256" },
  { 0, NULL }
};


static int
dissect_s1ap_PagingDRX(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     4, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string s1ap_PagingPriority_vals[] = {
  {   0, "priolevel1" },
  {   1, "priolevel2" },
  {   2, "priolevel3" },
  {   3, "priolevel4" },
  {   4, "priolevel5" },
  {   5, "priolevel6" },
  {   6, "priolevel7" },
  {   7, "priolevel8" },
  { 0, NULL }
};


static int
dissect_s1ap_PagingPriority(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     8, NULL, TRUE, 0, NULL);

  return offset;
}



static int
dissect_s1ap_Port_Number(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       2, 2, FALSE, NULL);

  return offset;
}


static const value_string s1ap_ProSeDirectDiscovery_vals[] = {
  {   0, "authorized" },
  {   1, "not-authorized" },
  { 0, NULL }
};


static int
dissect_s1ap_ProSeDirectDiscovery(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string s1ap_ProSeDirectCommunication_vals[] = {
  {   0, "authorized" },
  {   1, "not-authorized" },
  { 0, NULL }
};


static int
dissect_s1ap_ProSeDirectCommunication(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t ProSeAuthorized_sequence[] = {
  { &hf_s1ap_proSeDirectDiscovery, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProSeDirectDiscovery },
  { &hf_s1ap_proSeDirectCommunication, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProSeDirectCommunication },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_ProSeAuthorized(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_ProSeAuthorized, ProSeAuthorized_sequence);

  return offset;
}


static const value_string s1ap_ProSeUEtoNetworkRelaying_vals[] = {
  {   0, "authorized" },
  {   1, "not-authorized" },
  { 0, NULL }
};


static int
dissect_s1ap_ProSeUEtoNetworkRelaying(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string s1ap_PS_ServiceNotAvailable_vals[] = {
  {   0, "ps-service-not-available" },
  { 0, NULL }
};


static int
dissect_s1ap_PS_ServiceNotAvailable(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     1, NULL, TRUE, 0, NULL);

  return offset;
}



static int
dissect_s1ap_ReceiveStatusOfULPDCPSDUsExtended(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     1, 16384, FALSE, NULL, NULL);

  return offset;
}



static int
dissect_s1ap_ReceiveStatusOfULPDCPSDUsPDCP_SNlength18(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     1, 131072, FALSE, NULL, NULL);

  return offset;
}



static int
dissect_s1ap_RelativeMMECapacity(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, NULL, FALSE);

  return offset;
}


static const value_string s1ap_RelayNode_Indicator_vals[] = {
  {   0, "true" },
  { 0, NULL }
};


static int
dissect_s1ap_RelayNode_Indicator(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     1, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string s1ap_ReportArea_vals[] = {
  {   0, "ecgi" },
  { 0, NULL }
};


static int
dissect_s1ap_ReportArea(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     1, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t RequestType_sequence[] = {
  { &hf_s1ap_eventType      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_EventType },
  { &hf_s1ap_reportArea     , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ReportArea },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_RequestType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_RequestType, RequestType_sequence);

  return offset;
}



static int
dissect_s1ap_RIMInformation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 525 "./asn1/s1ap/s1ap.cnf"
 tvbuff_t *parameter_tvb;
 proto_tree *subtree;

  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, &parameter_tvb);


   if (!parameter_tvb)
    return offset;

   subtree = proto_item_add_subtree(actx->created_item, ett_s1ap_RIMInformation);
   if ((tvb_reported_length(parameter_tvb)>0)&&(bssgp_handle)){
    col_set_fence(actx->pinfo->cinfo, COL_INFO);
    call_dissector(bssgp_handle,parameter_tvb,actx->pinfo, subtree);
   }




  return offset;
}



static int
dissect_s1ap_RNC_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 4095U, NULL, FALSE);

  return offset;
}


static const per_sequence_t TargetRNC_ID_sequence[] = {
  { &hf_s1ap_lAI            , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_LAI },
  { &hf_s1ap_rAC            , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_RAC },
  { &hf_s1ap_rNC_ID         , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_RNC_ID },
  { &hf_s1ap_extendedRNC_ID , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ExtendedRNC_ID },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_TargetRNC_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_TargetRNC_ID, TargetRNC_ID_sequence);

  return offset;
}



static int
dissect_s1ap_OCTET_STRING_SIZE_16(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       16, 16, FALSE, NULL);

  return offset;
}


static const value_string s1ap_RIMRoutingAddress_vals[] = {
  {   0, "gERAN-Cell-ID" },
  {   1, "targetRNC-ID" },
  {   2, "eHRPD-Sector-ID" },
  { 0, NULL }
};

static const per_choice_t RIMRoutingAddress_choice[] = {
  {   0, &hf_s1ap_gERAN_Cell_ID  , ASN1_EXTENSION_ROOT    , dissect_s1ap_GERAN_Cell_ID },
  {   1, &hf_s1ap_targetRNC_ID   , ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_TargetRNC_ID },
  {   2, &hf_s1ap_eHRPD_Sector_ID, ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_OCTET_STRING_SIZE_16 },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_RIMRoutingAddress(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_RIMRoutingAddress, RIMRoutingAddress_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t RIMTransfer_sequence[] = {
  { &hf_s1ap_rIMInformation , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_RIMInformation },
  { &hf_s1ap_rIMRoutingAddress, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_RIMRoutingAddress },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_RIMTransfer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_RIMTransfer, RIMTransfer_sequence);

  return offset;
}



static int
dissect_s1ap_RepetitionPeriod(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 4095U, NULL, FALSE);

  return offset;
}



static int
dissect_s1ap_UE_RLF_Report_Container(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, NULL);

  return offset;
}



static int
dissect_s1ap_UE_RLF_Report_Container_for_extended_bands(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, NULL);

  return offset;
}


static const per_sequence_t RLFReportInformation_sequence[] = {
  { &hf_s1ap_uE_RLF_Report_Container, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_UE_RLF_Report_Container },
  { &hf_s1ap_uE_RLF_Report_Container_for_extended_bands, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_UE_RLF_Report_Container_for_extended_bands },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_RLFReportInformation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_RLFReportInformation, RLFReportInformation_sequence);

  return offset;
}



static int
dissect_s1ap_RRC_Container(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 450 "./asn1/s1ap/s1ap.cnf"


 tvbuff_t *parameter_tvb;
 proto_tree *subtree = NULL;

  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, &parameter_tvb);

  if (!parameter_tvb)
    return offset;

if (g_s1ap_dissect_container) {
	subtree = proto_item_add_subtree(actx->created_item, ett_s1ap_RRCContainer);

	switch(message_type){
		case INITIATING_MESSAGE:
		/* 9.2.1.7 Source eNB to Target eNB Transparent Container */
			dissect_lte_rrc_HandoverPreparationInformation_PDU(parameter_tvb, actx->pinfo, subtree, NULL);
			break;
		case SUCCESSFUL_OUTCOME:
		/* 9.2.1.7 Source eNB to Target eNB Transparent Container */
			dissect_lte_rrc_HandoverCommand_PDU(parameter_tvb, actx->pinfo, subtree, NULL);
			break;
		default:
			break;
	}
}






  return offset;
}


static const value_string s1ap_RRC_Establishment_Cause_vals[] = {
  {   0, "emergency" },
  {   1, "highPriorityAccess" },
  {   2, "mt-Access" },
  {   3, "mo-Signalling" },
  {   4, "mo-Data" },
  {   5, "delay-TolerantAccess" },
  { 0, NULL }
};


static int
dissect_s1ap_RRC_Establishment_Cause(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     5, NULL, TRUE, 1, NULL);

  return offset;
}


static const per_sequence_t ECGIListForRestart_sequence_of[1] = {
  { &hf_s1ap_ECGIListForRestart_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_EUTRAN_CGI },
};

static int
dissect_s1ap_ECGIListForRestart(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_ECGIListForRestart, ECGIListForRestart_sequence_of,
                                                  1, maxnoofCellsforRestart, FALSE);

  return offset;
}



static int
dissect_s1ap_Routing_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, NULL, FALSE);

  return offset;
}



static int
dissect_s1ap_SecurityKey(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     256, 256, FALSE, NULL, NULL);

  return offset;
}



static int
dissect_s1ap_INTEGER_0_7(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 7U, NULL, FALSE);

  return offset;
}


static const per_sequence_t SecurityContext_sequence[] = {
  { &hf_s1ap_nextHopChainingCount, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_INTEGER_0_7 },
  { &hf_s1ap_nextHopParameter, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_SecurityKey },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_SecurityContext(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_SecurityContext, SecurityContext_sequence);

  return offset;
}



static int
dissect_s1ap_SerialNumber(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     16, 16, FALSE, NULL, NULL);

  return offset;
}


static const value_string s1ap_SONInformationRequest_vals[] = {
  {   0, "x2TNL-Configuration-Info" },
  {   1, "time-Synchronisation-Info" },
  {   2, "activate-Muting" },
  {   3, "deactivate-Muting" },
  { 0, NULL }
};


static int
dissect_s1ap_SONInformationRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     1, NULL, TRUE, 3, NULL);

  return offset;
}


static const per_sequence_t X2TNLConfigurationInfo_sequence[] = {
  { &hf_s1ap_eNBX2TransportLayerAddresses, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ENBX2TLAs },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_X2TNLConfigurationInfo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_X2TNLConfigurationInfo, X2TNLConfigurationInfo_sequence);

  return offset;
}


static const per_sequence_t SONInformationReply_sequence[] = {
  { &hf_s1ap_x2TNLConfigurationInfo, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_X2TNLConfigurationInfo },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_SONInformationReply(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_SONInformationReply, SONInformationReply_sequence);

  return offset;
}



static int
dissect_s1ap_SONInformation_Extension(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_s1ap_ProtocolIE_SingleContainer(tvb, offset, actx, tree, hf_index);

  return offset;
}


static const value_string s1ap_SONInformation_vals[] = {
  {   0, "sONInformationRequest" },
  {   1, "sONInformationReply" },
  {   2, "sONInformation-Extension" },
  { 0, NULL }
};

static const per_choice_t SONInformation_choice[] = {
  {   0, &hf_s1ap_sONInformationRequest, ASN1_EXTENSION_ROOT    , dissect_s1ap_SONInformationRequest },
  {   1, &hf_s1ap_sONInformationReply, ASN1_EXTENSION_ROOT    , dissect_s1ap_SONInformationReply },
  {   2, &hf_s1ap_sONInformation_Extension, ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_SONInformation_Extension },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_SONInformation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_SONInformation, SONInformation_choice,
                                 NULL);

  return offset;
}


static const value_string s1ap_SONInformationReport_vals[] = {
  {   0, "rLFReportInformation" },
  { 0, NULL }
};

static const per_choice_t SONInformationReport_choice[] = {
  {   0, &hf_s1ap_rLFReportInformation, ASN1_EXTENSION_ROOT    , dissect_s1ap_RLFReportInformation },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_SONInformationReport(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_SONInformationReport, SONInformationReport_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t TargeteNB_ID_sequence[] = {
  { &hf_s1ap_global_ENB_ID  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_Global_ENB_ID },
  { &hf_s1ap_selected_TAI   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TAI },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_TargeteNB_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_TargeteNB_ID, TargeteNB_ID_sequence);

  return offset;
}


static const per_sequence_t SourceeNB_ID_sequence[] = {
  { &hf_s1ap_global_ENB_ID  , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_Global_ENB_ID },
  { &hf_s1ap_selected_TAI   , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_TAI },
  { &hf_s1ap_iE_Extensions  , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_SourceeNB_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_SourceeNB_ID, SourceeNB_ID_sequence);

  return offset;
}


static const per_sequence_t SONConfigurationTransfer_sequence[] = {
  { &hf_s1ap_targeteNB_ID   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TargeteNB_ID },
  { &hf_s1ap_sourceeNB_ID   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_SourceeNB_ID },
  { &hf_s1ap_sONInformation , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_SONInformation },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_SONConfigurationTransfer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_SONConfigurationTransfer, SONConfigurationTransfer_sequence);

  return offset;
}



static int
dissect_s1ap_StratumLevel(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 3U, NULL, TRUE);

  return offset;
}


static const per_sequence_t SynchronisationInformation_sequence[] = {
  { &hf_s1ap_sourceStratumLevel, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_StratumLevel },
  { &hf_s1ap_listeningSubframePattern, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ListeningSubframePattern },
  { &hf_s1ap_aggressoreCGI_List, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ECGI_List },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_SynchronisationInformation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_SynchronisationInformation, SynchronisationInformation_sequence);

  return offset;
}



static int
dissect_s1ap_Source_ToTarget_TransparentContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 335 "./asn1/s1ap/s1ap.cnf"
 tvbuff_t *parameter_tvb;
 proto_tree *subtree = NULL;

  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, &parameter_tvb);


	if (g_s1ap_dissect_container) {
		/* Don't want elements inside container to write to info column */
		col_set_writable(actx->pinfo->cinfo, COL_INFO, FALSE);
		subtree = proto_item_add_subtree(actx->created_item, ett_s1ap_ToTargetTransparentContainer);

    	switch(handover_type_value){
		/*
    	HandoverType ::= ENUMERATED {
    	intralte,
    	ltetoutran,
    	ltetogeran,
    	utrantolte,
    	gerantolte,
    	...
    	} */
			case 0:
			/* intralte
				Intra E-UTRAN handover Source eNB to Target eNB
				Transparent Container 36.413
		 	*/
			dissect_SourceeNB_ToTargeteNB_TransparentContainer_PDU(parameter_tvb, actx->pinfo, subtree, NULL);
			break;
			case 1:
			/* ltetoutran
				Source RNC to Target RNC
				Transparent Container 25.413
			*/
			dissect_ranap_SourceRNC_ToTargetRNC_TransparentContainer_PDU(parameter_tvb, actx->pinfo, subtree, NULL);
			break;
			case 2:
			/* ltetogeran
				Source BSS to Target BSS
				Transparent Container 48.018
			*/
			de_bssgp_source_BSS_to_target_BSS_transp_cont(parameter_tvb, subtree, actx->pinfo, 0, tvb_reported_length(parameter_tvb), NULL, 0);
			break;
			case 3:
			/* utrantolte */
			break;
			case 4:
			/* gerantolte */
			break;
			default:
			DISSECTOR_ASSERT_NOT_REACHED();
			break;
		}
		/* Enable writing of the column again */
		col_set_writable(actx->pinfo->cinfo, COL_INFO, TRUE);
	}


  return offset;
}



static const value_string s1ap_SRVCCOperationPossible_vals[] = {
  {   0, "possible" },
  { 0, NULL }
};


static int
dissect_s1ap_SRVCCOperationPossible(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     1, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string s1ap_SRVCCHOIndication_vals[] = {
  {   0, "pSandCS" },
  {   1, "cSonly" },
  { 0, NULL }
};


static int
dissect_s1ap_SRVCCHOIndication(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, TRUE, 0, NULL);

  return offset;
}



static int
dissect_s1ap_SubscriberProfileIDforRFP(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 256U, NULL, FALSE);

  return offset;
}


static const per_sequence_t UE_HistoryInformation_sequence_of[1] = {
  { &hf_s1ap_UE_HistoryInformation_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_LastVisitedCell_Item },
};

static int
dissect_s1ap_UE_HistoryInformation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_UE_HistoryInformation, UE_HistoryInformation_sequence_of,
                                                  1, maxnoofCells, FALSE);

  return offset;
}


static const per_sequence_t SourceeNB_ToTargeteNB_TransparentContainer_sequence[] = {
  { &hf_s1ap_rRC_Container  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_RRC_Container },
  { &hf_s1ap_e_RABInformationList, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_E_RABInformationList },
  { &hf_s1ap_targetCell_ID  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_EUTRAN_CGI },
  { &hf_s1ap_subscriberProfileIDforRFP, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_SubscriberProfileIDforRFP },
  { &hf_s1ap_uE_HistoryInformation, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_UE_HistoryInformation },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_SourceeNB_ToTargeteNB_TransparentContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_SourceeNB_ToTargeteNB_TransparentContainer, SourceeNB_ToTargeteNB_TransparentContainer_sequence);

  return offset;
}



static const per_sequence_t ServedPLMNs_sequence_of[1] = {
  { &hf_s1ap_ServedPLMNs_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_PLMNidentity },
};

static int
dissect_s1ap_ServedPLMNs(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_ServedPLMNs, ServedPLMNs_sequence_of,
                                                  1, maxnoofPLMNsPerMME, FALSE);

  return offset;
}


static const per_sequence_t ServedGroupIDs_sequence_of[1] = {
  { &hf_s1ap_ServedGroupIDs_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_MME_Group_ID },
};

static int
dissect_s1ap_ServedGroupIDs(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_ServedGroupIDs, ServedGroupIDs_sequence_of,
                                                  1, maxnoofGroupIDs, FALSE);

  return offset;
}


static const per_sequence_t ServedMMECs_sequence_of[1] = {
  { &hf_s1ap_ServedMMECs_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_MME_Code },
};

static int
dissect_s1ap_ServedMMECs(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_ServedMMECs, ServedMMECs_sequence_of,
                                                  1, maxnoofMMECs, FALSE);

  return offset;
}


static const per_sequence_t ServedGUMMEIsItem_sequence[] = {
  { &hf_s1ap_servedPLMNs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ServedPLMNs },
  { &hf_s1ap_servedGroupIDs , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ServedGroupIDs },
  { &hf_s1ap_servedMMECs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ServedMMECs },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_ServedGUMMEIsItem(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_ServedGUMMEIsItem, ServedGUMMEIsItem_sequence);

  return offset;
}


static const per_sequence_t ServedGUMMEIs_sequence_of[1] = {
  { &hf_s1ap_ServedGUMMEIs_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ServedGUMMEIsItem },
};

static int
dissect_s1ap_ServedGUMMEIs(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_ServedGUMMEIs, ServedGUMMEIs_sequence_of,
                                                  1, maxnoofRATs, FALSE);

  return offset;
}


static const per_sequence_t SupportedTAs_Item_sequence[] = {
  { &hf_s1ap_tAC            , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TAC },
  { &hf_s1ap_broadcastPLMNs , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_BPLMNs },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_SupportedTAs_Item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_SupportedTAs_Item, SupportedTAs_Item_sequence);

  return offset;
}


static const per_sequence_t SupportedTAs_sequence_of[1] = {
  { &hf_s1ap_SupportedTAs_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_SupportedTAs_Item },
};

static int
dissect_s1ap_SupportedTAs(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_SupportedTAs, SupportedTAs_sequence_of,
                                                  1, maxnoofTACs, FALSE);

  return offset;
}


static const value_string s1ap_SynchronisationStatus_vals[] = {
  {   0, "synchronous" },
  {   1, "asynchronous" },
  { 0, NULL }
};


static int
dissect_s1ap_SynchronisationStatus(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t TimeSynchronisationInfo_sequence[] = {
  { &hf_s1ap_stratumLevel   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_StratumLevel },
  { &hf_s1ap_synchronisationStatus, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_SynchronisationStatus },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_TimeSynchronisationInfo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_TimeSynchronisationInfo, TimeSynchronisationInfo_sequence);

  return offset;
}


static const per_sequence_t S_TMSI_sequence[] = {
  { &hf_s1ap_mMEC           , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_MME_Code },
  { &hf_s1ap_m_TMSI         , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_M_TMSI },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_S_TMSI(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_S_TMSI, S_TMSI_sequence);

  return offset;
}


static const per_sequence_t TAIListforWarning_sequence_of[1] = {
  { &hf_s1ap_TAIListforWarning_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_TAI },
};

static int
dissect_s1ap_TAIListforWarning(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_TAIListforWarning, TAIListforWarning_sequence_of,
                                                  1, maxnoofTAIforWarning, FALSE);

  return offset;
}


static const value_string s1ap_TargetID_vals[] = {
  {   0, "targeteNB-ID" },
  {   1, "targetRNC-ID" },
  {   2, "cGI" },
  { 0, NULL }
};

static const per_choice_t TargetID_choice[] = {
  {   0, &hf_s1ap_targeteNB_ID   , ASN1_EXTENSION_ROOT    , dissect_s1ap_TargeteNB_ID },
  {   1, &hf_s1ap_targetRNC_ID   , ASN1_EXTENSION_ROOT    , dissect_s1ap_TargetRNC_ID },
  {   2, &hf_s1ap_cGI            , ASN1_EXTENSION_ROOT    , dissect_s1ap_CGI },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_TargetID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_TargetID, TargetID_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t TargeteNB_ToSourceeNB_TransparentContainer_sequence[] = {
  { &hf_s1ap_rRC_Container  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_RRC_Container },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_TargeteNB_ToSourceeNB_TransparentContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_TargeteNB_ToSourceeNB_TransparentContainer, TargeteNB_ToSourceeNB_TransparentContainer_sequence);

  return offset;
}



static int
dissect_s1ap_Target_ToSource_TransparentContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 392 "./asn1/s1ap/s1ap.cnf"

 tvbuff_t *parameter_tvb;
 proto_tree *subtree = NULL;

  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, &parameter_tvb);


	if (g_s1ap_dissect_container) {
		subtree = proto_item_add_subtree(actx->created_item, ett_s1ap_ToSourceTransparentContainer);

    	switch(handover_type_value){
		/*
    	HandoverType ::= ENUMERATED {
    	intralte,
    	ltetoutran,
    	ltetogeran,
    	utrantolte,
    	gerantolte,
    	...
    	} */
			case 0:
			/* intralte
				Intra E-UTRAN handover Target eNB to Source eNB
				Transparent Container 36.413
		 	*/
			dissect_TargeteNB_ToSourceeNB_TransparentContainer_PDU(parameter_tvb, actx->pinfo, subtree, NULL);
			break;
			case 1:
			/* ltetoutran
				Target RNC to Source RNC
				Transparent Container 25.413
			*/
			dissect_ranap_TargetRNC_ToSourceRNC_TransparentContainer_PDU(parameter_tvb, actx->pinfo, subtree, NULL);
			break;
			case 2:
			/* ltetogeran
				Target BSS to Source BSS
				Transparent Container 48.018
			*/
			de_bssgp_target_BSS_to_source_BSS_transp_cont(parameter_tvb, subtree, actx->pinfo, 0, tvb_reported_length(parameter_tvb), NULL, 0);

			break;
			case 3:
			/* utrantolte */
			break;
			case 4:
			/* gerantolte */
			break;
			default:
			DISSECTOR_ASSERT_NOT_REACHED();
			break;
		}
	}



  return offset;
}




static const value_string s1ap_TimeToWait_vals[] = {
  {   0, "v1s" },
  {   1, "v2s" },
  {   2, "v5s" },
  {   3, "v10s" },
  {   4, "v20s" },
  {   5, "v60s" },
  { 0, NULL }
};


static int
dissect_s1ap_TimeToWait(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     6, NULL, TRUE, 0, NULL);

  return offset;
}



static int
dissect_s1ap_Time_UE_StayedInCell_EnhancedGranularity(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 40950U, NULL, FALSE);

  return offset;
}


static const per_sequence_t TransportInformation_sequence[] = {
  { &hf_s1ap_transportLayerAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TransportLayerAddress },
  { &hf_s1ap_uL_GTP_TEID    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_GTP_TEID },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_TransportInformation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_TransportInformation, TransportInformation_sequence);

  return offset;
}



static int
dissect_s1ap_E_UTRAN_Trace_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       8, 8, FALSE, NULL);

  return offset;
}


static const value_string s1ap_TraceDepth_vals[] = {
  {   0, "minimum" },
  {   1, "medium" },
  {   2, "maximum" },
  {   3, "minimumWithoutVendorSpecificExtension" },
  {   4, "mediumWithoutVendorSpecificExtension" },
  {   5, "maximumWithoutVendorSpecificExtension" },
  { 0, NULL }
};


static int
dissect_s1ap_TraceDepth(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     6, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t TraceActivation_sequence[] = {
  { &hf_s1ap_e_UTRAN_Trace_ID, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_E_UTRAN_Trace_ID },
  { &hf_s1ap_interfacesToTrace, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_InterfacesToTrace },
  { &hf_s1ap_traceDepth     , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TraceDepth },
  { &hf_s1ap_traceCollectionEntityIPAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TransportLayerAddress },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_TraceActivation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_TraceActivation, TraceActivation_sequence);

  return offset;
}



static int
dissect_s1ap_TrafficLoadReductionIndication(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 99U, NULL, FALSE);

  return offset;
}


static const per_sequence_t TunnelInformation_sequence[] = {
  { &hf_s1ap_transportLayerAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TransportLayerAddress },
  { &hf_s1ap_uDP_Port_Number, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_Port_Number },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_TunnelInformation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_TunnelInformation, TunnelInformation_sequence);

  return offset;
}


static const per_sequence_t TAIListForRestart_sequence_of[1] = {
  { &hf_s1ap_TAIListForRestart_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_TAI },
};

static int
dissect_s1ap_TAIListForRestart(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_TAIListForRestart, TAIListForRestart_sequence_of,
                                                  1, maxnoofRestartTAIs, FALSE);

  return offset;
}


static const per_sequence_t UEAggregateMaximumBitrate_sequence[] = {
  { &hf_s1ap_uEaggregateMaximumBitRateDL, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_BitRate },
  { &hf_s1ap_uEaggregateMaximumBitRateUL, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_BitRate },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_UEAggregateMaximumBitrate(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_UEAggregateMaximumBitrate, UEAggregateMaximumBitrate_sequence);

  return offset;
}


static const per_sequence_t UE_S1AP_ID_pair_sequence[] = {
  { &hf_s1ap_mME_UE_S1AP_ID , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_MME_UE_S1AP_ID },
  { &hf_s1ap_eNB_UE_S1AP_ID , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ENB_UE_S1AP_ID },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_UE_S1AP_ID_pair(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_UE_S1AP_ID_pair, UE_S1AP_ID_pair_sequence);

  return offset;
}


static const value_string s1ap_UE_S1AP_IDs_vals[] = {
  {   0, "uE-S1AP-ID-pair" },
  {   1, "mME-UE-S1AP-ID" },
  { 0, NULL }
};

static const per_choice_t UE_S1AP_IDs_choice[] = {
  {   0, &hf_s1ap_uE_S1AP_ID_pair, ASN1_EXTENSION_ROOT    , dissect_s1ap_UE_S1AP_ID_pair },
  {   1, &hf_s1ap_mME_UE_S1AP_ID , ASN1_EXTENSION_ROOT    , dissect_s1ap_MME_UE_S1AP_ID },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_UE_S1AP_IDs(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_UE_S1AP_IDs, UE_S1AP_IDs_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t UE_associatedLogicalS1_ConnectionItem_sequence[] = {
  { &hf_s1ap_mME_UE_S1AP_ID , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_MME_UE_S1AP_ID },
  { &hf_s1ap_eNB_UE_S1AP_ID , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ENB_UE_S1AP_ID },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_UE_associatedLogicalS1_ConnectionItem(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_UE_associatedLogicalS1_ConnectionItem, UE_associatedLogicalS1_ConnectionItem_sequence);

  return offset;
}



static int
dissect_s1ap_UEIdentityIndexValue(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     10, 10, FALSE, NULL, NULL);

  return offset;
}



static int
dissect_s1ap_UE_HistoryInformationFromTheUE(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, NULL);

  return offset;
}


static const value_string s1ap_UEPagingID_vals[] = {
  {   0, "s-TMSI" },
  {   1, "iMSI" },
  { 0, NULL }
};

static const per_choice_t UEPagingID_choice[] = {
  {   0, &hf_s1ap_s_TMSI         , ASN1_EXTENSION_ROOT    , dissect_s1ap_S_TMSI },
  {   1, &hf_s1ap_iMSI           , ASN1_EXTENSION_ROOT    , dissect_s1ap_IMSI },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_UEPagingID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_UEPagingID, UEPagingID_choice,
                                 NULL);

  return offset;
}



static int
dissect_s1ap_UERadioCapability(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 510 "./asn1/s1ap/s1ap.cnf"
 tvbuff_t *parameter_tvb;
 proto_tree *subtree = NULL;

  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, &parameter_tvb);

  if (!parameter_tvb)
    return offset;

if (g_s1ap_dissect_container) {
        subtree = proto_item_add_subtree(actx->created_item, ett_s1ap_UERadioCapability);
        dissect_lte_rrc_UERadioAccessCapabilityInformation_PDU(parameter_tvb, actx->pinfo, subtree, NULL);
}



  return offset;
}



static int
dissect_s1ap_UERadioCapabilityForPaging(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, NULL);

  return offset;
}


static const per_sequence_t UESecurityCapabilities_sequence[] = {
  { &hf_s1ap_encryptionAlgorithms, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_EncryptionAlgorithms },
  { &hf_s1ap_integrityProtectionAlgorithms, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_IntegrityProtectionAlgorithms },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_UESecurityCapabilities(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_UESecurityCapabilities, UESecurityCapabilities_sequence);

  return offset;
}


static const per_sequence_t UserLocationInformation_sequence[] = {
  { &hf_s1ap_eutran_cgi     , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_EUTRAN_CGI },
  { &hf_s1ap_tai            , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TAI },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_UserLocationInformation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_UserLocationInformation, UserLocationInformation_sequence);

  return offset;
}


static const value_string s1ap_VoiceSupportMatchIndicator_vals[] = {
  {   0, "supported" },
  {   1, "not-supported" },
  { 0, NULL }
};


static int
dissect_s1ap_VoiceSupportMatchIndicator(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string s1ap_WarningAreaList_vals[] = {
  {   0, "cellIDList" },
  {   1, "trackingAreaListforWarning" },
  {   2, "emergencyAreaIDList" },
  { 0, NULL }
};

static const per_choice_t WarningAreaList_choice[] = {
  {   0, &hf_s1ap_cellIDList     , ASN1_EXTENSION_ROOT    , dissect_s1ap_ECGIList },
  {   1, &hf_s1ap_trackingAreaListforWarning, ASN1_EXTENSION_ROOT    , dissect_s1ap_TAIListforWarning },
  {   2, &hf_s1ap_emergencyAreaIDList, ASN1_EXTENSION_ROOT    , dissect_s1ap_EmergencyAreaIDList },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_WarningAreaList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_WarningAreaList, WarningAreaList_choice,
                                 NULL);

  return offset;
}



static int
dissect_s1ap_WarningType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       2, 2, FALSE, NULL);

  return offset;
}



static int
dissect_s1ap_WarningSecurityInfo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       50, 50, FALSE, NULL);

  return offset;
}



static int
dissect_s1ap_WarningMessageContents(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       1, 9600, FALSE, NULL);

  return offset;
}


static const per_sequence_t ENBX2GTPTLAs_sequence_of[1] = {
  { &hf_s1ap_ENBX2GTPTLAs_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_TransportLayerAddress },
};

static int
dissect_s1ap_ENBX2GTPTLAs(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_ENBX2GTPTLAs, ENBX2GTPTLAs_sequence_of,
                                                  1, maxnoofeNBX2GTPTLAs, FALSE);

  return offset;
}


static const per_sequence_t ENBX2ExtTLA_sequence[] = {
  { &hf_s1ap_iPsecTLA       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_TransportLayerAddress },
  { &hf_s1ap_gTPTLAa        , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ENBX2GTPTLAs },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_ENBX2ExtTLA(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_ENBX2ExtTLA, ENBX2ExtTLA_sequence);

  return offset;
}


static const per_sequence_t ENBX2ExtTLAs_sequence_of[1] = {
  { &hf_s1ap_ENBX2ExtTLAs_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ENBX2ExtTLA },
};

static int
dissect_s1ap_ENBX2ExtTLAs(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_ENBX2ExtTLAs, ENBX2ExtTLAs_sequence_of,
                                                  1, maxnoofeNBX2ExtTLAs, FALSE);

  return offset;
}


static const per_sequence_t ENBIndirectX2TransportLayerAddresses_sequence_of[1] = {
  { &hf_s1ap_ENBIndirectX2TransportLayerAddresses_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_TransportLayerAddress },
};

static int
dissect_s1ap_ENBIndirectX2TransportLayerAddresses(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_ENBIndirectX2TransportLayerAddresses, ENBIndirectX2TransportLayerAddresses_sequence_of,
                                                  1, maxnoofeNBX2TLAs, FALSE);

  return offset;
}



static int
dissect_s1ap_E_RAB_IE_ContainerList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 161 "./asn1/s1ap/s1ap.cnf"
  asn1_stack_frame_push(actx, "ProtocolIE-ContainerList");
  asn1_param_push_integer(actx, 1);
  asn1_param_push_integer(actx, maxnoofE_RABs);
  offset = dissect_s1ap_ProtocolIE_ContainerList(tvb, offset, actx, tree, hf_index);

  asn1_stack_frame_pop(actx, "ProtocolIE-ContainerList");


  return offset;
}


static const per_sequence_t HandoverRequired_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_HandoverRequired(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 321 "./asn1/s1ap/s1ap.cnf"
	handover_type_value = 0;
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", HandoverRequired ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_HandoverRequired, HandoverRequired_sequence);

  return offset;
}


static const per_sequence_t HandoverCommand_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_HandoverCommand(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 323 "./asn1/s1ap/s1ap.cnf"
	handover_type_value = 0;
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", HandoverCommand ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_HandoverCommand, HandoverCommand_sequence);

  return offset;
}



static int
dissect_s1ap_E_RABSubjecttoDataForwardingList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_s1ap_E_RAB_IE_ContainerList(tvb, offset, actx, tree, hf_index);

  return offset;
}


static const per_sequence_t E_RABDataForwardingItem_sequence[] = {
  { &hf_s1ap_e_RAB_ID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_E_RAB_ID },
  { &hf_s1ap_dL_transportLayerAddress, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_TransportLayerAddress },
  { &hf_s1ap_dL_gTP_TEID    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_GTP_TEID },
  { &hf_s1ap_uL_TransportLayerAddress, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_TransportLayerAddress },
  { &hf_s1ap_uL_GTP_TEID    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_GTP_TEID },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABDataForwardingItem(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABDataForwardingItem, E_RABDataForwardingItem_sequence);

  return offset;
}


static const per_sequence_t HandoverPreparationFailure_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_HandoverPreparationFailure(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1232 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", HandoverPreparationFailure ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_HandoverPreparationFailure, HandoverPreparationFailure_sequence);

  return offset;
}


static const per_sequence_t HandoverRequest_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_HandoverRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 325 "./asn1/s1ap/s1ap.cnf"
	handover_type_value = 0;

     col_append_str(actx->pinfo->cinfo, COL_INFO, ", HandoverRequest ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_HandoverRequest, HandoverRequest_sequence);

  return offset;
}



static int
dissect_s1ap_E_RABToBeSetupListHOReq(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_s1ap_E_RAB_IE_ContainerList(tvb, offset, actx, tree, hf_index);

  return offset;
}


static const per_sequence_t E_RABToBeSetupItemHOReq_sequence[] = {
  { &hf_s1ap_e_RAB_ID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_E_RAB_ID },
  { &hf_s1ap_transportLayerAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TransportLayerAddress },
  { &hf_s1ap_gTP_TEID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_GTP_TEID },
  { &hf_s1ap_e_RABlevelQosParameters, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_E_RABLevelQoSParameters },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABToBeSetupItemHOReq(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABToBeSetupItemHOReq, E_RABToBeSetupItemHOReq_sequence);

  return offset;
}


static const per_sequence_t HandoverRequestAcknowledge_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_HandoverRequestAcknowledge(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1236 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", HandoverRequestAcknowledge ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_HandoverRequestAcknowledge, HandoverRequestAcknowledge_sequence);

  return offset;
}



static int
dissect_s1ap_E_RABAdmittedList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_s1ap_E_RAB_IE_ContainerList(tvb, offset, actx, tree, hf_index);

  return offset;
}


static const per_sequence_t E_RABAdmittedItem_sequence[] = {
  { &hf_s1ap_e_RAB_ID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_E_RAB_ID },
  { &hf_s1ap_transportLayerAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TransportLayerAddress },
  { &hf_s1ap_gTP_TEID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_GTP_TEID },
  { &hf_s1ap_dL_transportLayerAddress, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_TransportLayerAddress },
  { &hf_s1ap_dL_gTP_TEID    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_GTP_TEID },
  { &hf_s1ap_uL_TransportLayerAddress, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_TransportLayerAddress },
  { &hf_s1ap_uL_GTP_TEID    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_GTP_TEID },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABAdmittedItem(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABAdmittedItem, E_RABAdmittedItem_sequence);

  return offset;
}



static int
dissect_s1ap_E_RABFailedtoSetupListHOReqAck(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_s1ap_E_RAB_IE_ContainerList(tvb, offset, actx, tree, hf_index);

  return offset;
}


static const per_sequence_t E_RABFailedToSetupItemHOReqAck_sequence[] = {
  { &hf_s1ap_e_RAB_ID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_E_RAB_ID },
  { &hf_s1ap_cause          , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_Cause },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABFailedToSetupItemHOReqAck(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABFailedToSetupItemHOReqAck, E_RABFailedToSetupItemHOReqAck_sequence);

  return offset;
}


static const per_sequence_t HandoverFailure_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_HandoverFailure(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1238 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", HandoverFailure ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_HandoverFailure, HandoverFailure_sequence);

  return offset;
}


static const per_sequence_t HandoverNotify_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_HandoverNotify(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1240 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", HandoverNotify ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_HandoverNotify, HandoverNotify_sequence);

  return offset;
}


static const per_sequence_t PathSwitchRequest_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_PathSwitchRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1242 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", PathSwitchRequest ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_PathSwitchRequest, PathSwitchRequest_sequence);

  return offset;
}



static int
dissect_s1ap_E_RABToBeSwitchedDLList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_s1ap_E_RAB_IE_ContainerList(tvb, offset, actx, tree, hf_index);

  return offset;
}


static const per_sequence_t E_RABToBeSwitchedDLItem_sequence[] = {
  { &hf_s1ap_e_RAB_ID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_E_RAB_ID },
  { &hf_s1ap_transportLayerAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TransportLayerAddress },
  { &hf_s1ap_gTP_TEID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_GTP_TEID },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABToBeSwitchedDLItem(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABToBeSwitchedDLItem, E_RABToBeSwitchedDLItem_sequence);

  return offset;
}


static const per_sequence_t PathSwitchRequestAcknowledge_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_PathSwitchRequestAcknowledge(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1244 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", PathSwitchRequestAcknowledge ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_PathSwitchRequestAcknowledge, PathSwitchRequestAcknowledge_sequence);

  return offset;
}



static int
dissect_s1ap_E_RABToBeSwitchedULList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_s1ap_E_RAB_IE_ContainerList(tvb, offset, actx, tree, hf_index);

  return offset;
}


static const per_sequence_t E_RABToBeSwitchedULItem_sequence[] = {
  { &hf_s1ap_e_RAB_ID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_E_RAB_ID },
  { &hf_s1ap_transportLayerAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TransportLayerAddress },
  { &hf_s1ap_gTP_TEID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_GTP_TEID },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABToBeSwitchedULItem(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABToBeSwitchedULItem, E_RABToBeSwitchedULItem_sequence);

  return offset;
}


static const per_sequence_t PathSwitchRequestFailure_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_PathSwitchRequestFailure(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1246 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", PathSwitchRequestFailure ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_PathSwitchRequestFailure, PathSwitchRequestFailure_sequence);

  return offset;
}


static const per_sequence_t HandoverCancel_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_HandoverCancel(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1248 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", HandoverCancel ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_HandoverCancel, HandoverCancel_sequence);

  return offset;
}


static const per_sequence_t HandoverCancelAcknowledge_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_HandoverCancelAcknowledge(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1250 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", HandoverCancelAcknowledge ");



  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_HandoverCancelAcknowledge, HandoverCancelAcknowledge_sequence);

  return offset;
}


static const per_sequence_t E_RABSetupRequest_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABSetupRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1194 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", E_RABSetupRequest ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABSetupRequest, E_RABSetupRequest_sequence);

  return offset;
}


static const per_sequence_t E_RABToBeSetupListBearerSUReq_sequence_of[1] = {
  { &hf_s1ap_E_RABToBeSetupListBearerSUReq_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_SingleContainer },
};

static int
dissect_s1ap_E_RABToBeSetupListBearerSUReq(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_E_RABToBeSetupListBearerSUReq, E_RABToBeSetupListBearerSUReq_sequence_of,
                                                  1, maxnoofE_RABs, FALSE);

  return offset;
}


static const per_sequence_t E_RABToBeSetupItemBearerSUReq_sequence[] = {
  { &hf_s1ap_e_RAB_ID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_E_RAB_ID },
  { &hf_s1ap_e_RABlevelQoSParameters, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_E_RABLevelQoSParameters },
  { &hf_s1ap_transportLayerAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TransportLayerAddress },
  { &hf_s1ap_gTP_TEID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_GTP_TEID },
  { &hf_s1ap_nAS_PDU        , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_NAS_PDU },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABToBeSetupItemBearerSUReq(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABToBeSetupItemBearerSUReq, E_RABToBeSetupItemBearerSUReq_sequence);

  return offset;
}


static const per_sequence_t E_RABSetupResponse_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABSetupResponse(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1196 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", E_RABSetupResponse ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABSetupResponse, E_RABSetupResponse_sequence);

  return offset;
}


static const per_sequence_t E_RABSetupListBearerSURes_sequence_of[1] = {
  { &hf_s1ap_E_RABSetupListBearerSURes_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_SingleContainer },
};

static int
dissect_s1ap_E_RABSetupListBearerSURes(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_E_RABSetupListBearerSURes, E_RABSetupListBearerSURes_sequence_of,
                                                  1, maxnoofE_RABs, FALSE);

  return offset;
}


static const per_sequence_t E_RABSetupItemBearerSURes_sequence[] = {
  { &hf_s1ap_e_RAB_ID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_E_RAB_ID },
  { &hf_s1ap_transportLayerAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TransportLayerAddress },
  { &hf_s1ap_gTP_TEID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_GTP_TEID },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABSetupItemBearerSURes(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABSetupItemBearerSURes, E_RABSetupItemBearerSURes_sequence);

  return offset;
}


static const per_sequence_t E_RABModifyRequest_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABModifyRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1198 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", E_RABModifyRequest ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABModifyRequest, E_RABModifyRequest_sequence);

  return offset;
}


static const per_sequence_t E_RABToBeModifiedListBearerModReq_sequence_of[1] = {
  { &hf_s1ap_E_RABToBeModifiedListBearerModReq_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_SingleContainer },
};

static int
dissect_s1ap_E_RABToBeModifiedListBearerModReq(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_E_RABToBeModifiedListBearerModReq, E_RABToBeModifiedListBearerModReq_sequence_of,
                                                  1, maxnoofE_RABs, FALSE);

  return offset;
}


static const per_sequence_t E_RABToBeModifiedItemBearerModReq_sequence[] = {
  { &hf_s1ap_e_RAB_ID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_E_RAB_ID },
  { &hf_s1ap_e_RABLevelQoSParameters, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_E_RABLevelQoSParameters },
  { &hf_s1ap_nAS_PDU        , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_NAS_PDU },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABToBeModifiedItemBearerModReq(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABToBeModifiedItemBearerModReq, E_RABToBeModifiedItemBearerModReq_sequence);

  return offset;
}


static const per_sequence_t E_RABModifyResponse_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABModifyResponse(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1200 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", E_RABModifyResponse ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABModifyResponse, E_RABModifyResponse_sequence);

  return offset;
}


static const per_sequence_t E_RABModifyListBearerModRes_sequence_of[1] = {
  { &hf_s1ap_E_RABModifyListBearerModRes_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_SingleContainer },
};

static int
dissect_s1ap_E_RABModifyListBearerModRes(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_E_RABModifyListBearerModRes, E_RABModifyListBearerModRes_sequence_of,
                                                  1, maxnoofE_RABs, FALSE);

  return offset;
}


static const per_sequence_t E_RABModifyItemBearerModRes_sequence[] = {
  { &hf_s1ap_e_RAB_ID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_E_RAB_ID },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABModifyItemBearerModRes(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABModifyItemBearerModRes, E_RABModifyItemBearerModRes_sequence);

  return offset;
}


static const per_sequence_t E_RABReleaseCommand_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABReleaseCommand(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1202 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", E_RABReleaseCommand ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABReleaseCommand, E_RABReleaseCommand_sequence);

  return offset;
}


static const per_sequence_t E_RABReleaseResponse_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABReleaseResponse(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1204 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", E_RABReleaseResponse ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABReleaseResponse, E_RABReleaseResponse_sequence);

  return offset;
}


static const per_sequence_t E_RABReleaseListBearerRelComp_sequence_of[1] = {
  { &hf_s1ap_E_RABReleaseListBearerRelComp_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_SingleContainer },
};

static int
dissect_s1ap_E_RABReleaseListBearerRelComp(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_E_RABReleaseListBearerRelComp, E_RABReleaseListBearerRelComp_sequence_of,
                                                  1, maxnoofE_RABs, FALSE);

  return offset;
}


static const per_sequence_t E_RABReleaseItemBearerRelComp_sequence[] = {
  { &hf_s1ap_e_RAB_ID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_E_RAB_ID },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABReleaseItemBearerRelComp(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABReleaseItemBearerRelComp, E_RABReleaseItemBearerRelComp_sequence);

  return offset;
}


static const per_sequence_t E_RABReleaseIndication_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABReleaseIndication(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1206 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", E_RABReleaseIndication ");


  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABReleaseIndication, E_RABReleaseIndication_sequence);

  return offset;
}


static const per_sequence_t InitialContextSetupRequest_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_InitialContextSetupRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1209 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", InitialContextSetupRequest ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_InitialContextSetupRequest, InitialContextSetupRequest_sequence);

  return offset;
}


static const per_sequence_t E_RABToBeSetupListCtxtSUReq_sequence_of[1] = {
  { &hf_s1ap_E_RABToBeSetupListCtxtSUReq_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_SingleContainer },
};

static int
dissect_s1ap_E_RABToBeSetupListCtxtSUReq(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_E_RABToBeSetupListCtxtSUReq, E_RABToBeSetupListCtxtSUReq_sequence_of,
                                                  1, maxnoofE_RABs, FALSE);

  return offset;
}


static const per_sequence_t E_RABToBeSetupItemCtxtSUReq_sequence[] = {
  { &hf_s1ap_e_RAB_ID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_E_RAB_ID },
  { &hf_s1ap_e_RABlevelQoSParameters, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_E_RABLevelQoSParameters },
  { &hf_s1ap_transportLayerAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TransportLayerAddress },
  { &hf_s1ap_gTP_TEID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_GTP_TEID },
  { &hf_s1ap_nAS_PDU        , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_NAS_PDU },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABToBeSetupItemCtxtSUReq(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABToBeSetupItemCtxtSUReq, E_RABToBeSetupItemCtxtSUReq_sequence);

  return offset;
}


static const per_sequence_t InitialContextSetupResponse_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_InitialContextSetupResponse(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1211 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", InitialContextSetupResponse ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_InitialContextSetupResponse, InitialContextSetupResponse_sequence);

  return offset;
}


static const per_sequence_t E_RABSetupListCtxtSURes_sequence_of[1] = {
  { &hf_s1ap_E_RABSetupListCtxtSURes_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_SingleContainer },
};

static int
dissect_s1ap_E_RABSetupListCtxtSURes(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_E_RABSetupListCtxtSURes, E_RABSetupListCtxtSURes_sequence_of,
                                                  1, maxnoofE_RABs, FALSE);

  return offset;
}


static const per_sequence_t E_RABSetupItemCtxtSURes_sequence[] = {
  { &hf_s1ap_e_RAB_ID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_E_RAB_ID },
  { &hf_s1ap_transportLayerAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TransportLayerAddress },
  { &hf_s1ap_gTP_TEID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_GTP_TEID },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABSetupItemCtxtSURes(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABSetupItemCtxtSURes, E_RABSetupItemCtxtSURes_sequence);

  return offset;
}


static const per_sequence_t InitialContextSetupFailure_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_InitialContextSetupFailure(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1213 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", InitialContextSetupFailure ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_InitialContextSetupFailure, InitialContextSetupFailure_sequence);

  return offset;
}


static const per_sequence_t Paging_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_Paging(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_Paging, Paging_sequence);

  return offset;
}


static const per_sequence_t TAIList_sequence_of[1] = {
  { &hf_s1ap_TAIList_item   , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_SingleContainer },
};

static int
dissect_s1ap_TAIList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_TAIList, TAIList_sequence_of,
                                                  1, maxnoofTAIs, FALSE);

  return offset;
}


static const per_sequence_t TAIItem_sequence[] = {
  { &hf_s1ap_tAI            , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TAI },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_TAIItem(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_TAIItem, TAIItem_sequence);

  return offset;
}


static const per_sequence_t UEContextReleaseRequest_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_UEContextReleaseRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1215 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", UEContextReleaseRequest ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_UEContextReleaseRequest, UEContextReleaseRequest_sequence);

  return offset;
}


static const per_sequence_t UEContextReleaseCommand_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_UEContextReleaseCommand(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1217 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", UEContextReleaseCommand ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_UEContextReleaseCommand, UEContextReleaseCommand_sequence);

  return offset;
}


static const per_sequence_t UEContextReleaseComplete_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_UEContextReleaseComplete(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1219 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", UEContextReleaseComplete ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_UEContextReleaseComplete, UEContextReleaseComplete_sequence);

  return offset;
}


static const per_sequence_t UEContextModificationRequest_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_UEContextModificationRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1221 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", UEContextModificationRequest ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_UEContextModificationRequest, UEContextModificationRequest_sequence);

  return offset;
}


static const per_sequence_t UEContextModificationResponse_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_UEContextModificationResponse(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1223 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", UEContextModificationResponse ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_UEContextModificationResponse, UEContextModificationResponse_sequence);

  return offset;
}


static const per_sequence_t UEContextModificationFailure_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_UEContextModificationFailure(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1225 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", UEContextModificationFailure ");


  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_UEContextModificationFailure, UEContextModificationFailure_sequence);

  return offset;
}


static const per_sequence_t UERadioCapabilityMatchRequest_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_UERadioCapabilityMatchRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1300 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", UERadioCapabilityMatchRequest ");


  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_UERadioCapabilityMatchRequest, UERadioCapabilityMatchRequest_sequence);

  return offset;
}


static const per_sequence_t UERadioCapabilityMatchResponse_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_UERadioCapabilityMatchResponse(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1303 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", UERadioCapabilityMatchResponse ");


  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_UERadioCapabilityMatchResponse, UERadioCapabilityMatchResponse_sequence);

  return offset;
}


static const per_sequence_t DownlinkNASTransport_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_DownlinkNASTransport(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 308 "./asn1/s1ap/s1ap.cnf"
	/* Set the direction of the message */
	actx->pinfo->link_dir=P2P_DIR_DL;


  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_DownlinkNASTransport, DownlinkNASTransport_sequence);

  return offset;
}


static const per_sequence_t InitialUEMessage_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_InitialUEMessage(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 304 "./asn1/s1ap/s1ap.cnf"
	/* Set the direction of the message */
	actx->pinfo->link_dir=P2P_DIR_UL;


  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_InitialUEMessage, InitialUEMessage_sequence);

  return offset;
}


static const per_sequence_t UplinkNASTransport_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_UplinkNASTransport(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 312 "./asn1/s1ap/s1ap.cnf"
	/* Set the direction of the message */
	actx->pinfo->link_dir=P2P_DIR_UL;


  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_UplinkNASTransport, UplinkNASTransport_sequence);

  return offset;
}


static const per_sequence_t NASNonDeliveryIndication_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_NASNonDeliveryIndication(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_NASNonDeliveryIndication, NASNonDeliveryIndication_sequence);

  return offset;
}


static const per_sequence_t RerouteNASRequest_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_RerouteNASRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1318 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", RerouteNASRequest ");


  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_RerouteNASRequest, RerouteNASRequest_sequence);

  return offset;
}


static const per_sequence_t Reset_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_Reset(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1254 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", Reset ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_Reset, Reset_sequence);

  return offset;
}


static const value_string s1ap_ResetAll_vals[] = {
  {   0, "reset-all" },
  { 0, NULL }
};


static int
dissect_s1ap_ResetAll(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     1, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t UE_associatedLogicalS1_ConnectionListRes_sequence_of[1] = {
  { &hf_s1ap_UE_associatedLogicalS1_ConnectionListRes_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_SingleContainer },
};

static int
dissect_s1ap_UE_associatedLogicalS1_ConnectionListRes(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_UE_associatedLogicalS1_ConnectionListRes, UE_associatedLogicalS1_ConnectionListRes_sequence_of,
                                                  1, maxnoofIndividualS1ConnectionsToReset, FALSE);

  return offset;
}


static const value_string s1ap_ResetType_vals[] = {
  {   0, "s1-Interface" },
  {   1, "partOfS1-Interface" },
  { 0, NULL }
};

static const per_choice_t ResetType_choice[] = {
  {   0, &hf_s1ap_s1_Interface   , ASN1_EXTENSION_ROOT    , dissect_s1ap_ResetAll },
  {   1, &hf_s1ap_partOfS1_Interface, ASN1_EXTENSION_ROOT    , dissect_s1ap_UE_associatedLogicalS1_ConnectionListRes },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_ResetType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_ResetType, ResetType_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t ResetAcknowledge_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_ResetAcknowledge(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1256 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", ResetAcknowledge ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_ResetAcknowledge, ResetAcknowledge_sequence);

  return offset;
}


static const per_sequence_t UE_associatedLogicalS1_ConnectionListResAck_sequence_of[1] = {
  { &hf_s1ap_UE_associatedLogicalS1_ConnectionListResAck_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_SingleContainer },
};

static int
dissect_s1ap_UE_associatedLogicalS1_ConnectionListResAck(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_UE_associatedLogicalS1_ConnectionListResAck, UE_associatedLogicalS1_ConnectionListResAck_sequence_of,
                                                  1, maxnoofIndividualS1ConnectionsToReset, FALSE);

  return offset;
}


static const per_sequence_t ErrorIndication_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_ErrorIndication(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_ErrorIndication, ErrorIndication_sequence);

  return offset;
}


static const per_sequence_t S1SetupRequest_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_S1SetupRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1260 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", S1SetupRequest ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_S1SetupRequest, S1SetupRequest_sequence);

  return offset;
}


static const per_sequence_t S1SetupResponse_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_S1SetupResponse(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1262 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", S1SetupResponse ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_S1SetupResponse, S1SetupResponse_sequence);

  return offset;
}


static const per_sequence_t S1SetupFailure_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_S1SetupFailure(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1264 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", S1SetupFailure ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_S1SetupFailure, S1SetupFailure_sequence);

  return offset;
}


static const per_sequence_t ENBConfigurationUpdate_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_ENBConfigurationUpdate(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1266 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", ENBConfigurationUpdate ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_ENBConfigurationUpdate, ENBConfigurationUpdate_sequence);

  return offset;
}


static const per_sequence_t ENBConfigurationUpdateAcknowledge_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_ENBConfigurationUpdateAcknowledge(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1268 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", ENBConfigurationUpdateAcknowledge ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_ENBConfigurationUpdateAcknowledge, ENBConfigurationUpdateAcknowledge_sequence);

  return offset;
}


static const per_sequence_t ENBConfigurationUpdateFailure_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_ENBConfigurationUpdateFailure(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1270 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", ENBConfigurationUpdateFailure ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_ENBConfigurationUpdateFailure, ENBConfigurationUpdateFailure_sequence);

  return offset;
}


static const per_sequence_t MMEConfigurationUpdate_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_MMEConfigurationUpdate(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1272 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", MMEConfigurationUpdate ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_MMEConfigurationUpdate, MMEConfigurationUpdate_sequence);

  return offset;
}


static const per_sequence_t MMEConfigurationUpdateAcknowledge_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_MMEConfigurationUpdateAcknowledge(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1274 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", MMEConfigurationUpdateAcknowledge ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_MMEConfigurationUpdateAcknowledge, MMEConfigurationUpdateAcknowledge_sequence);

  return offset;
}


static const per_sequence_t MMEConfigurationUpdateFailure_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_MMEConfigurationUpdateFailure(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1276 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", MMEConfigurationUpdateFailure ");


  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_MMEConfigurationUpdateFailure, MMEConfigurationUpdateFailure_sequence);

  return offset;
}


static const per_sequence_t DownlinkS1cdma2000tunnelling_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_DownlinkS1cdma2000tunnelling(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_DownlinkS1cdma2000tunnelling, DownlinkS1cdma2000tunnelling_sequence);

  return offset;
}


static const per_sequence_t UplinkS1cdma2000tunnelling_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_UplinkS1cdma2000tunnelling(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_UplinkS1cdma2000tunnelling, UplinkS1cdma2000tunnelling_sequence);

  return offset;
}


static const per_sequence_t UECapabilityInfoIndication_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_UECapabilityInfoIndication(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_UECapabilityInfoIndication, UECapabilityInfoIndication_sequence);

  return offset;
}


static const per_sequence_t ENBStatusTransfer_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_ENBStatusTransfer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_ENBStatusTransfer, ENBStatusTransfer_sequence);

  return offset;
}


static const per_sequence_t MMEStatusTransfer_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_MMEStatusTransfer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_MMEStatusTransfer, MMEStatusTransfer_sequence);

  return offset;
}


static const per_sequence_t TraceStart_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_TraceStart(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_TraceStart, TraceStart_sequence);

  return offset;
}


static const per_sequence_t TraceFailureIndication_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_TraceFailureIndication(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_TraceFailureIndication, TraceFailureIndication_sequence);

  return offset;
}


static const per_sequence_t DeactivateTrace_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_DeactivateTrace(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_DeactivateTrace, DeactivateTrace_sequence);

  return offset;
}


static const per_sequence_t CellTrafficTrace_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_CellTrafficTrace(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_CellTrafficTrace, CellTrafficTrace_sequence);

  return offset;
}


static const per_sequence_t LocationReportingControl_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_LocationReportingControl(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_LocationReportingControl, LocationReportingControl_sequence);

  return offset;
}


static const per_sequence_t LocationReportingFailureIndication_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_LocationReportingFailureIndication(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_LocationReportingFailureIndication, LocationReportingFailureIndication_sequence);

  return offset;
}


static const per_sequence_t LocationReport_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_LocationReport(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_LocationReport, LocationReport_sequence);

  return offset;
}


static const per_sequence_t OverloadStart_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_OverloadStart(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_OverloadStart, OverloadStart_sequence);

  return offset;
}


static const per_sequence_t OverloadStop_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_OverloadStop(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_OverloadStop, OverloadStop_sequence);

  return offset;
}


static const per_sequence_t WriteReplaceWarningRequest_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_WriteReplaceWarningRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1279 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", WriteReplaceWarningRequest ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_WriteReplaceWarningRequest, WriteReplaceWarningRequest_sequence);

  return offset;
}


static const per_sequence_t WriteReplaceWarningResponse_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_WriteReplaceWarningResponse(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1281 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", WriteReplaceWarningResponse ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_WriteReplaceWarningResponse, WriteReplaceWarningResponse_sequence);

  return offset;
}


static const per_sequence_t ENBDirectInformationTransfer_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_ENBDirectInformationTransfer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_ENBDirectInformationTransfer, ENBDirectInformationTransfer_sequence);

  return offset;
}


static const value_string s1ap_Inter_SystemInformationTransferType_vals[] = {
  {   0, "rIMTransfer" },
  { 0, NULL }
};

static const per_choice_t Inter_SystemInformationTransferType_choice[] = {
  {   0, &hf_s1ap_rIMTransfer    , ASN1_EXTENSION_ROOT    , dissect_s1ap_RIMTransfer },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_Inter_SystemInformationTransferType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_Inter_SystemInformationTransferType, Inter_SystemInformationTransferType_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t MMEDirectInformationTransfer_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_MMEDirectInformationTransfer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_MMEDirectInformationTransfer, MMEDirectInformationTransfer_sequence);

  return offset;
}


static const per_sequence_t ENBConfigurationTransfer_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_ENBConfigurationTransfer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_ENBConfigurationTransfer, ENBConfigurationTransfer_sequence);

  return offset;
}


static const per_sequence_t MMEConfigurationTransfer_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_MMEConfigurationTransfer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_MMEConfigurationTransfer, MMEConfigurationTransfer_sequence);

  return offset;
}


static const per_sequence_t PrivateMessage_sequence[] = {
  { &hf_s1ap_privateIEs     , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_PrivateIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_PrivateMessage(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_PrivateMessage, PrivateMessage_sequence);

  return offset;
}


static const per_sequence_t KillRequest_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_KillRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1283 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", KillRequest ");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_KillRequest, KillRequest_sequence);

  return offset;
}


static const per_sequence_t KillResponse_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_KillResponse(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1285 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", KillResponse ");


  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_KillResponse, KillResponse_sequence);

  return offset;
}


static const per_sequence_t PWSRestartIndication_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_PWSRestartIndication(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_PWSRestartIndication, PWSRestartIndication_sequence);

  return offset;
}


static const per_sequence_t PWSFailureIndication_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_PWSFailureIndication(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1321 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", PWSFailureIndication ");


/* Whenever a Cause appears, look up the reason and show it in the Info column */
   

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_PWSFailureIndication, PWSFailureIndication_sequence);

  return offset;
}


static const per_sequence_t DownlinkUEAssociatedLPPaTransport_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_DownlinkUEAssociatedLPPaTransport(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1288 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", DownlinkUEAssociatedLPPaTransport ");


  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_DownlinkUEAssociatedLPPaTransport, DownlinkUEAssociatedLPPaTransport_sequence);

  return offset;
}


static const per_sequence_t UplinkUEAssociatedLPPaTransport_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_UplinkUEAssociatedLPPaTransport(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1291 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", UplinkUEAssociatedLPPaTransport ");


  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_UplinkUEAssociatedLPPaTransport, UplinkUEAssociatedLPPaTransport_sequence);

  return offset;
}


static const per_sequence_t DownlinkNonUEAssociatedLPPaTransport_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_DownlinkNonUEAssociatedLPPaTransport(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1294 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", DownlinkNonUEAssociatedLPPaTransport ");


  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_DownlinkNonUEAssociatedLPPaTransport, DownlinkNonUEAssociatedLPPaTransport_sequence);

  return offset;
}


static const per_sequence_t UplinkNonUEAssociatedLPPaTransport_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_UplinkNonUEAssociatedLPPaTransport(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1297 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", UplinkNonUEAssociatedLPPaTransport ");


  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_UplinkNonUEAssociatedLPPaTransport, UplinkNonUEAssociatedLPPaTransport_sequence);

  return offset;
}


static const per_sequence_t E_RABModificationIndication_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABModificationIndication(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1306 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", E-RABModificationIndication ");


  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABModificationIndication, E_RABModificationIndication_sequence);

  return offset;
}



static int
dissect_s1ap_E_RABToBeModifiedListBearerModInd(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_s1ap_E_RAB_IE_ContainerList(tvb, offset, actx, tree, hf_index);

  return offset;
}


static const per_sequence_t E_RABToBeModifiedItemBearerModInd_sequence[] = {
  { &hf_s1ap_e_RAB_ID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_E_RAB_ID },
  { &hf_s1ap_transportLayerAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TransportLayerAddress },
  { &hf_s1ap_dL_GTP_TEID    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_GTP_TEID },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABToBeModifiedItemBearerModInd(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABToBeModifiedItemBearerModInd, E_RABToBeModifiedItemBearerModInd_sequence);

  return offset;
}



static int
dissect_s1ap_E_RABNotToBeModifiedListBearerModInd(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_s1ap_E_RAB_IE_ContainerList(tvb, offset, actx, tree, hf_index);

  return offset;
}


static const per_sequence_t E_RABNotToBeModifiedItemBearerModInd_sequence[] = {
  { &hf_s1ap_e_RAB_ID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_E_RAB_ID },
  { &hf_s1ap_transportLayerAddress, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_TransportLayerAddress },
  { &hf_s1ap_dL_GTP_TEID    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_GTP_TEID },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABNotToBeModifiedItemBearerModInd(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABNotToBeModifiedItemBearerModInd, E_RABNotToBeModifiedItemBearerModInd_sequence);

  return offset;
}


static const per_sequence_t CSGMembershipInfo_sequence[] = {
  { &hf_s1ap_cSGMembershipStatus, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_CSGMembershipStatus },
  { &hf_s1ap_cSG_Id         , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_CSG_Id },
  { &hf_s1ap_cellAccessMode , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_CellAccessMode },
  { &hf_s1ap_pLMNidentity   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_PLMNidentity },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_CSGMembershipInfo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_CSGMembershipInfo, CSGMembershipInfo_sequence);

  return offset;
}


static const per_sequence_t E_RABModificationConfirm_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABModificationConfirm(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1309 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", E-RABModificationConfirm ");


  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABModificationConfirm, E_RABModificationConfirm_sequence);

  return offset;
}


static const per_sequence_t E_RABModifyListBearerModConf_sequence_of[1] = {
  { &hf_s1ap_E_RABModifyListBearerModConf_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_SingleContainer },
};

static int
dissect_s1ap_E_RABModifyListBearerModConf(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_E_RABModifyListBearerModConf, E_RABModifyListBearerModConf_sequence_of,
                                                  1, maxnoofE_RABs, FALSE);

  return offset;
}


static const per_sequence_t E_RABModifyItemBearerModConf_sequence[] = {
  { &hf_s1ap_e_RAB_ID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_E_RAB_ID },
  { &hf_s1ap_iE_Extensions  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_ProtocolExtensionContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_E_RABModifyItemBearerModConf(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_E_RABModifyItemBearerModConf, E_RABModifyItemBearerModConf_sequence);

  return offset;
}


static const per_sequence_t UEContextModificationIndication_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_UEContextModificationIndication(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1312 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", UEContextModificationIndication ");


  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_UEContextModificationIndication, UEContextModificationIndication_sequence);

  return offset;
}


static const per_sequence_t UEContextModificationConfirm_sequence[] = {
  { &hf_s1ap_protocolIEs    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ProtocolIE_Container },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_UEContextModificationConfirm(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1315 "./asn1/s1ap/s1ap.cnf"
     col_append_str(actx->pinfo->cinfo, COL_INFO, ", UEContextModificationConfirm ");


  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_UEContextModificationConfirm, UEContextModificationConfirm_sequence);

  return offset;
}



static int
dissect_s1ap_InitiatingMessage_value(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 116 "./asn1/s1ap/s1ap.cnf"
	message_type = INITIATING_MESSAGE;

  offset = dissect_per_open_type_pdu_new(tvb, offset, actx, tree, hf_index, dissect_InitiatingMessageValue);

  return offset;
}


static const per_sequence_t InitiatingMessage_sequence[] = {
  { &hf_s1ap_procedureCode  , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ProcedureCode },
  { &hf_s1ap_criticality    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_Criticality },
  { &hf_s1ap_initiatingMessagevalue, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_InitiatingMessage_value },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_InitiatingMessage(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_InitiatingMessage, InitiatingMessage_sequence);

  return offset;
}



static int
dissect_s1ap_SuccessfulOutcome_value(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 119 "./asn1/s1ap/s1ap.cnf"
	message_type = SUCCESSFUL_OUTCOME;

  offset = dissect_per_open_type_pdu_new(tvb, offset, actx, tree, hf_index, dissect_SuccessfulOutcomeValue);

  return offset;
}


static const per_sequence_t SuccessfulOutcome_sequence[] = {
  { &hf_s1ap_procedureCode  , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ProcedureCode },
  { &hf_s1ap_criticality    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_Criticality },
  { &hf_s1ap_successfulOutcome_value, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_SuccessfulOutcome_value },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_SuccessfulOutcome(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_SuccessfulOutcome, SuccessfulOutcome_sequence);

  return offset;
}



static int
dissect_s1ap_UnsuccessfulOutcome_value(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 122 "./asn1/s1ap/s1ap.cnf"
	message_type = UNSUCCESSFUL_OUTCOME;




  offset = dissect_per_open_type_pdu_new(tvb, offset, actx, tree, hf_index, dissect_UnsuccessfulOutcomeValue);

  return offset;
}


static const per_sequence_t UnsuccessfulOutcome_sequence[] = {
  { &hf_s1ap_procedureCode  , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ProcedureCode },
  { &hf_s1ap_criticality    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_Criticality },
  { &hf_s1ap_unsuccessfulOutcome_value, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_UnsuccessfulOutcome_value },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_UnsuccessfulOutcome(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_UnsuccessfulOutcome, UnsuccessfulOutcome_sequence);

  return offset;
}


static const value_string s1ap_S1AP_PDU_vals[] = {
  {   0, "initiatingMessage" },
  {   1, "successfulOutcome" },
  {   2, "unsuccessfulOutcome" },
  { 0, NULL }
};

static const per_choice_t S1AP_PDU_choice[] = {
  {   0, &hf_s1ap_initiatingMessage, ASN1_EXTENSION_ROOT    , dissect_s1ap_InitiatingMessage },
  {   1, &hf_s1ap_successfulOutcome, ASN1_EXTENSION_ROOT    , dissect_s1ap_SuccessfulOutcome },
  {   2, &hf_s1ap_unsuccessfulOutcome, ASN1_EXTENSION_ROOT    , dissect_s1ap_UnsuccessfulOutcome },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_S1AP_PDU(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_S1AP_PDU, S1AP_PDU_choice,
                                 NULL);

  return offset;
}


static const value_string s1ap_SONtransferApplicationIdentity_vals[] = {
  {   0, "cell-load-reporting" },
  {   1, "multi-cell-load-reporting" },
  {   2, "event-triggered-cell-load-reporting" },
  {   3, "ho-reporting" },
  {   4, "eutran-cell-activation" },
  {   5, "energy-savings-indication" },
  {   6, "failure-event-reporting" },
  { 0, NULL }
};


static int
dissect_s1ap_SONtransferApplicationIdentity(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     1, NULL, TRUE, 6, NULL);

  return offset;
}



static int
dissect_s1ap_OCTET_STRING(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, NULL);

  return offset;
}



static int
dissect_s1ap_EHRPD_Sector_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       16, 16, FALSE, NULL);

  return offset;
}


static const value_string s1ap_IRAT_Cell_ID_vals[] = {
  {   0, "eUTRAN" },
  {   1, "uTRAN" },
  {   2, "gERAN" },
  {   3, "eHRPD" },
  { 0, NULL }
};

static const per_choice_t IRAT_Cell_ID_choice[] = {
  {   0, &hf_s1ap_eUTRAN_01      , ASN1_EXTENSION_ROOT    , dissect_s1ap_OCTET_STRING },
  {   1, &hf_s1ap_uTRAN          , ASN1_EXTENSION_ROOT    , dissect_s1ap_OCTET_STRING },
  {   2, &hf_s1ap_gERAN          , ASN1_EXTENSION_ROOT    , dissect_s1ap_OCTET_STRING },
  {   3, &hf_s1ap_eHRPD_01       , ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_EHRPD_Sector_ID },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_IRAT_Cell_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_IRAT_Cell_ID, IRAT_Cell_ID_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t RequestedCellList_sequence_of[1] = {
  { &hf_s1ap_RequestedCellList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_IRAT_Cell_ID },
};

static int
dissect_s1ap_RequestedCellList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_RequestedCellList, RequestedCellList_sequence_of,
                                                  1, maxnoofIRATReportingCells, FALSE);

  return offset;
}


static const per_sequence_t MultiCellLoadReportingRequest_sequence[] = {
  { &hf_s1ap_requestedCellList, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_RequestedCellList },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_MultiCellLoadReportingRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_MultiCellLoadReportingRequest, MultiCellLoadReportingRequest_sequence);

  return offset;
}


static const value_string s1ap_NumberOfMeasurementReportingLevels_vals[] = {
  {   0, "rl2" },
  {   1, "rl3" },
  {   2, "rl4" },
  {   3, "rl5" },
  {   4, "rl10" },
  { 0, NULL }
};


static int
dissect_s1ap_NumberOfMeasurementReportingLevels(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     5, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t EventTriggeredCellLoadReportingRequest_sequence[] = {
  { &hf_s1ap_numberOfMeasurementReportingLevels, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_NumberOfMeasurementReportingLevels },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_EventTriggeredCellLoadReportingRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_EventTriggeredCellLoadReportingRequest, EventTriggeredCellLoadReportingRequest_sequence);

  return offset;
}


static const value_string s1ap_HoType_vals[] = {
  {   0, "ltetoutran" },
  {   1, "ltetogeran" },
  { 0, NULL }
};


static int
dissect_s1ap_HoType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string s1ap_HoReportType_vals[] = {
  {   0, "unnecessaryhotoanotherrat" },
  {   1, "earlyirathandover" },
  { 0, NULL }
};


static int
dissect_s1ap_HoReportType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     1, NULL, TRUE, 1, NULL);

  return offset;
}


static const per_sequence_t CandidateCellList_sequence_of[1] = {
  { &hf_s1ap_CandidateCellList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_IRAT_Cell_ID },
};

static int
dissect_s1ap_CandidateCellList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_CandidateCellList, CandidateCellList_sequence_of,
                                                  1, maxnoofcandidateCells, FALSE);

  return offset;
}



static int
dissect_s1ap_INTEGER_0_503(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 503U, NULL, FALSE);

  return offset;
}


static const per_sequence_t CandidatePCI_sequence[] = {
  { &hf_s1ap_pCI            , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_INTEGER_0_503 },
  { &hf_s1ap_eARFCN         , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_OCTET_STRING },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_CandidatePCI(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_CandidatePCI, CandidatePCI_sequence);

  return offset;
}


static const per_sequence_t CandidatePCIList_sequence_of[1] = {
  { &hf_s1ap_CandidatePCIList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_CandidatePCI },
};

static int
dissect_s1ap_CandidatePCIList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_CandidatePCIList, CandidatePCIList_sequence_of,
                                                  1, maxnoofcandidateCells, FALSE);

  return offset;
}


static const per_sequence_t HOReport_sequence[] = {
  { &hf_s1ap_hoType         , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_HoType },
  { &hf_s1ap_hoReportType   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_HoReportType },
  { &hf_s1ap_hosourceID     , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_IRAT_Cell_ID },
  { &hf_s1ap_hoTargetID     , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_IRAT_Cell_ID },
  { &hf_s1ap_candidateCellList, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_CandidateCellList },
  { &hf_s1ap_candidatePCIList, ASN1_NOT_EXTENSION_ROOT, ASN1_OPTIONAL    , dissect_s1ap_CandidatePCIList },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_HOReport(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_HOReport, HOReport_sequence);

  return offset;
}


static const per_sequence_t CellsToActivateList_Item_sequence[] = {
  { &hf_s1ap_cell_ID_01     , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_OCTET_STRING },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_CellsToActivateList_Item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_CellsToActivateList_Item, CellsToActivateList_Item_sequence);

  return offset;
}


static const per_sequence_t CellsToActivateList_sequence_of[1] = {
  { &hf_s1ap_CellsToActivateList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_CellsToActivateList_Item },
};

static int
dissect_s1ap_CellsToActivateList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_CellsToActivateList, CellsToActivateList_sequence_of,
                                                  1, maxnoofCellineNB, FALSE);

  return offset;
}



static int
dissect_s1ap_INTEGER_1_60(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 60U, NULL, FALSE);

  return offset;
}


static const per_sequence_t CellActivationRequest_sequence[] = {
  { &hf_s1ap_cellsToActivateList, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_CellsToActivateList },
  { &hf_s1ap_minimumActivationTime, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_INTEGER_1_60 },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_CellActivationRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_CellActivationRequest, CellActivationRequest_sequence);

  return offset;
}


static const value_string s1ap_NotifyFlag_vals[] = {
  {   0, "activated" },
  {   1, "deactivated" },
  { 0, NULL }
};


static int
dissect_s1ap_NotifyFlag(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t NotificationCellList_Item_sequence[] = {
  { &hf_s1ap_cell_ID_01     , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_OCTET_STRING },
  { &hf_s1ap_notifyFlag     , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_NotifyFlag },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_NotificationCellList_Item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_NotificationCellList_Item, NotificationCellList_Item_sequence);

  return offset;
}


static const per_sequence_t NotificationCellList_sequence_of[1] = {
  { &hf_s1ap_NotificationCellList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_NotificationCellList_Item },
};

static int
dissect_s1ap_NotificationCellList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_NotificationCellList, NotificationCellList_sequence_of,
                                                  1, maxnoofCellineNB, FALSE);

  return offset;
}


static const per_sequence_t CellStateIndication_sequence[] = {
  { &hf_s1ap_notificationCellList, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_NotificationCellList },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_CellStateIndication(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_CellStateIndication, CellStateIndication_sequence);

  return offset;
}


static const per_sequence_t TooEarlyInterRATHOReportReportFromEUTRAN_sequence[] = {
  { &hf_s1ap_uERLFReportContainer, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_OCTET_STRING },
  { &hf_s1ap_mobilityInformation, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_MobilityInformation },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_TooEarlyInterRATHOReportReportFromEUTRAN(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_TooEarlyInterRATHOReportReportFromEUTRAN, TooEarlyInterRATHOReportReportFromEUTRAN_sequence);

  return offset;
}


static const value_string s1ap_FailureEventReport_vals[] = {
  {   0, "tooEarlyInterRATHOReportFromEUTRAN" },
  { 0, NULL }
};

static const per_choice_t FailureEventReport_choice[] = {
  {   0, &hf_s1ap_tooEarlyInterRATHOReportFromEUTRAN, ASN1_EXTENSION_ROOT    , dissect_s1ap_TooEarlyInterRATHOReportReportFromEUTRAN },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_FailureEventReport(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_FailureEventReport, FailureEventReport_choice,
                                 NULL);

  return offset;
}


const value_string s1ap_SONtransferRequestContainer_vals[] = {
  {   0, "cellLoadReporting" },
  {   1, "multiCellLoadReporting" },
  {   2, "eventTriggeredCellLoadReporting" },
  {   3, "hOReporting" },
  {   4, "eutranCellActivation" },
  {   5, "energySavingsIndication" },
  {   6, "failureEventReporting" },
  { 0, NULL }
};

static const per_choice_t SONtransferRequestContainer_choice[] = {
  {   0, &hf_s1ap_cellLoadReporting, ASN1_EXTENSION_ROOT    , dissect_s1ap_NULL },
  {   1, &hf_s1ap_multiCellLoadReporting, ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_MultiCellLoadReportingRequest },
  {   2, &hf_s1ap_eventTriggeredCellLoadReporting, ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_EventTriggeredCellLoadReportingRequest },
  {   3, &hf_s1ap_hOReporting    , ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_HOReport },
  {   4, &hf_s1ap_eutranCellActivation, ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_CellActivationRequest },
  {   5, &hf_s1ap_energySavingsIndication, ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_CellStateIndication },
  {   6, &hf_s1ap_failureEventReporting, ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_FailureEventReport },
  { 0, NULL, 0, NULL }
};

int
dissect_s1ap_SONtransferRequestContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_SONtransferRequestContainer, SONtransferRequestContainer_choice,
                                 NULL);

  return offset;
}



static int
dissect_s1ap_CompositeAvailableCapacityGroup(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, NULL);

  return offset;
}


static const per_sequence_t EUTRANcellLoadReportingResponse_sequence[] = {
  { &hf_s1ap_compositeAvailableCapacityGroup, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_CompositeAvailableCapacityGroup },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_EUTRANcellLoadReportingResponse(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_EUTRANcellLoadReportingResponse, EUTRANcellLoadReportingResponse_sequence);

  return offset;
}



static int
dissect_s1ap_EHRPDSectorCapacityClassValue(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 100U, NULL, TRUE);

  return offset;
}



static int
dissect_s1ap_EHRPDCapacityValue(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 100U, NULL, FALSE);

  return offset;
}


static const per_sequence_t EHRPDCompositeAvailableCapacity_sequence[] = {
  { &hf_s1ap_eHRPDSectorCapacityClassValue, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_EHRPDSectorCapacityClassValue },
  { &hf_s1ap_eHRPDCapacityValue, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_EHRPDCapacityValue },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_EHRPDCompositeAvailableCapacity(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_EHRPDCompositeAvailableCapacity, EHRPDCompositeAvailableCapacity_sequence);

  return offset;
}


static const per_sequence_t EHRPDSectorLoadReportingResponse_sequence[] = {
  { &hf_s1ap_dL_EHRPD_CompositeAvailableCapacity, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_EHRPDCompositeAvailableCapacity },
  { &hf_s1ap_uL_EHRPD_CompositeAvailableCapacity, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_EHRPDCompositeAvailableCapacity },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_EHRPDSectorLoadReportingResponse(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_EHRPDSectorLoadReportingResponse, EHRPDSectorLoadReportingResponse_sequence);

  return offset;
}


static const value_string s1ap_CellLoadReportingResponse_vals[] = {
  {   0, "eUTRAN" },
  {   1, "uTRAN" },
  {   2, "gERAN" },
  {   3, "eHRPD" },
  { 0, NULL }
};

static const per_choice_t CellLoadReportingResponse_choice[] = {
  {   0, &hf_s1ap_eUTRAN         , ASN1_EXTENSION_ROOT    , dissect_s1ap_EUTRANcellLoadReportingResponse },
  {   1, &hf_s1ap_uTRAN          , ASN1_EXTENSION_ROOT    , dissect_s1ap_OCTET_STRING },
  {   2, &hf_s1ap_gERAN          , ASN1_EXTENSION_ROOT    , dissect_s1ap_OCTET_STRING },
  {   3, &hf_s1ap_eHRPD          , ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_EHRPDSectorLoadReportingResponse },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_CellLoadReportingResponse(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_CellLoadReportingResponse, CellLoadReportingResponse_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t EUTRANResponse_sequence[] = {
  { &hf_s1ap_cell_ID_01     , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_OCTET_STRING },
  { &hf_s1ap_eUTRANcellLoadReportingResponse, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_EUTRANcellLoadReportingResponse },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_EUTRANResponse(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_EUTRANResponse, EUTRANResponse_sequence);

  return offset;
}


static const per_sequence_t EHRPDMultiSectorLoadReportingResponseItem_sequence[] = {
  { &hf_s1ap_eHRPD_Sector_ID_01, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_EHRPD_Sector_ID },
  { &hf_s1ap_eHRPDSectorLoadReportingResponse, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_EHRPDSectorLoadReportingResponse },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_EHRPDMultiSectorLoadReportingResponseItem(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_EHRPDMultiSectorLoadReportingResponseItem, EHRPDMultiSectorLoadReportingResponseItem_sequence);

  return offset;
}


static const value_string s1ap_MultiCellLoadReportingResponse_Item_vals[] = {
  {   0, "eUTRANResponse" },
  {   1, "uTRANResponse" },
  {   2, "gERANResponse" },
  {   3, "eHRPD" },
  { 0, NULL }
};

static const per_choice_t MultiCellLoadReportingResponse_Item_choice[] = {
  {   0, &hf_s1ap_eUTRANResponse , ASN1_EXTENSION_ROOT    , dissect_s1ap_EUTRANResponse },
  {   1, &hf_s1ap_uTRANResponse  , ASN1_EXTENSION_ROOT    , dissect_s1ap_OCTET_STRING },
  {   2, &hf_s1ap_gERANResponse  , ASN1_EXTENSION_ROOT    , dissect_s1ap_OCTET_STRING },
  {   3, &hf_s1ap_eHRPD_02       , ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_EHRPDMultiSectorLoadReportingResponseItem },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_MultiCellLoadReportingResponse_Item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_MultiCellLoadReportingResponse_Item, MultiCellLoadReportingResponse_Item_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t MultiCellLoadReportingResponse_sequence_of[1] = {
  { &hf_s1ap_MultiCellLoadReportingResponse_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_MultiCellLoadReportingResponse_Item },
};

static int
dissect_s1ap_MultiCellLoadReportingResponse(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_MultiCellLoadReportingResponse, MultiCellLoadReportingResponse_sequence_of,
                                                  1, maxnoofIRATReportingCells, FALSE);

  return offset;
}


static const value_string s1ap_OverloadFlag_vals[] = {
  {   0, "overload" },
  { 0, NULL }
};


static int
dissect_s1ap_OverloadFlag(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     1, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t EventTriggeredCellLoadReportingResponse_sequence[] = {
  { &hf_s1ap_cellLoadReportingResponse, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_CellLoadReportingResponse },
  { &hf_s1ap_overloadFlag   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_s1ap_OverloadFlag },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_EventTriggeredCellLoadReportingResponse(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_EventTriggeredCellLoadReportingResponse, EventTriggeredCellLoadReportingResponse_sequence);

  return offset;
}


static const per_sequence_t ActivatedCellsList_Item_sequence[] = {
  { &hf_s1ap_cell_ID_01     , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_OCTET_STRING },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_ActivatedCellsList_Item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_ActivatedCellsList_Item, ActivatedCellsList_Item_sequence);

  return offset;
}


static const per_sequence_t ActivatedCellsList_sequence_of[1] = {
  { &hf_s1ap_ActivatedCellsList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_s1ap_ActivatedCellsList_Item },
};

static int
dissect_s1ap_ActivatedCellsList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_s1ap_ActivatedCellsList, ActivatedCellsList_sequence_of,
                                                  0, maxnoofCellineNB, FALSE);

  return offset;
}


static const per_sequence_t CellActivationResponse_sequence[] = {
  { &hf_s1ap_activatedCellsList, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_s1ap_ActivatedCellsList },
  { NULL, 0, 0, NULL }
};

static int
dissect_s1ap_CellActivationResponse(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_s1ap_CellActivationResponse, CellActivationResponse_sequence);

  return offset;
}


const value_string s1ap_SONtransferResponseContainer_vals[] = {
  {   0, "cellLoadReporting" },
  {   1, "multiCellLoadReporting" },
  {   2, "eventTriggeredCellLoadReporting" },
  {   3, "hOReporting" },
  {   4, "eutranCellActivation" },
  {   5, "energySavingsIndication" },
  {   6, "failureEventReporting" },
  { 0, NULL }
};

static const per_choice_t SONtransferResponseContainer_choice[] = {
  {   0, &hf_s1ap_cellLoadReporting_01, ASN1_EXTENSION_ROOT    , dissect_s1ap_CellLoadReportingResponse },
  {   1, &hf_s1ap_multiCellLoadReporting_01, ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_MultiCellLoadReportingResponse },
  {   2, &hf_s1ap_eventTriggeredCellLoadReporting_01, ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_EventTriggeredCellLoadReportingResponse },
  {   3, &hf_s1ap_hOReporting_01 , ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_NULL },
  {   4, &hf_s1ap_eutranCellActivation_01, ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_CellActivationResponse },
  {   5, &hf_s1ap_energySavingsIndication_01, ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_NULL },
  {   6, &hf_s1ap_failureEventReporting_01, ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_NULL },
  { 0, NULL, 0, NULL }
};

int
dissect_s1ap_SONtransferResponseContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_SONtransferResponseContainer, SONtransferResponseContainer_choice,
                                 NULL);

  return offset;
}


static const value_string s1ap_CellLoadReportingCause_vals[] = {
  {   0, "application-container-syntax-error" },
  {   1, "inconsistent-reporting-cell-identifier" },
  {   2, "unspecified" },
  { 0, NULL }
};


static int
dissect_s1ap_CellLoadReportingCause(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     3, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string s1ap_HOReportingCause_vals[] = {
  {   0, "application-container-syntax-error" },
  {   1, "inconsistent-reporting-cell-identifier" },
  {   2, "unspecified" },
  { 0, NULL }
};


static int
dissect_s1ap_HOReportingCause(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     3, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string s1ap_CellActivationCause_vals[] = {
  {   0, "application-container-syntax-error" },
  {   1, "inconsistent-reporting-cell-identifier" },
  {   2, "unspecified" },
  { 0, NULL }
};


static int
dissect_s1ap_CellActivationCause(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     3, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string s1ap_CellStateIndicationCause_vals[] = {
  {   0, "application-container-syntax-error" },
  {   1, "inconsistent-reporting-cell-identifier" },
  {   2, "unspecified" },
  { 0, NULL }
};


static int
dissect_s1ap_CellStateIndicationCause(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     3, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string s1ap_FailureEventReportingCause_vals[] = {
  {   0, "application-container-syntax-error" },
  {   1, "inconsistent-reporting-cell-identifier" },
  {   2, "unspecified" },
  { 0, NULL }
};


static int
dissect_s1ap_FailureEventReportingCause(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     3, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string s1ap_SONtransferCause_vals[] = {
  {   0, "cellLoadReporting" },
  {   1, "multiCellLoadReporting" },
  {   2, "eventTriggeredCellLoadReporting" },
  {   3, "hOReporting" },
  {   4, "eutranCellActivation" },
  {   5, "energySavingsIndication" },
  {   6, "failureEventReporting" },
  { 0, NULL }
};

static const per_choice_t SONtransferCause_choice[] = {
  {   0, &hf_s1ap_cellLoadReporting_02, ASN1_EXTENSION_ROOT    , dissect_s1ap_CellLoadReportingCause },
  {   1, &hf_s1ap_multiCellLoadReporting_02, ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_CellLoadReportingCause },
  {   2, &hf_s1ap_eventTriggeredCellLoadReporting_02, ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_CellLoadReportingCause },
  {   3, &hf_s1ap_hOReporting_02 , ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_HOReportingCause },
  {   4, &hf_s1ap_eutranCellActivation_02, ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_CellActivationCause },
  {   5, &hf_s1ap_energySavingsIndication_02, ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_CellStateIndicationCause },
  {   6, &hf_s1ap_failureEventReporting_02, ASN1_NOT_EXTENSION_ROOT, dissect_s1ap_FailureEventReportingCause },
  { 0, NULL, 0, NULL }
};

static int
dissect_s1ap_SONtransferCause(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_s1ap_SONtransferCause, SONtransferCause_choice,
                                 NULL);

  return offset;
}



/*--- PDUs ---*/

static int dissect_Additional_GUTI_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_Additional_GUTI(tvb, offset, &asn1_ctx, tree, hf_s1ap_Additional_GUTI_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_Bearers_SubjectToStatusTransfer_Item_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_Bearers_SubjectToStatusTransfer_Item(tvb, offset, &asn1_ctx, tree, hf_s1ap_Bearers_SubjectToStatusTransfer_Item_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_BroadcastCancelledAreaList_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_BroadcastCancelledAreaList(tvb, offset, &asn1_ctx, tree, hf_s1ap_BroadcastCancelledAreaList_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_BroadcastCompletedAreaList_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_BroadcastCompletedAreaList(tvb, offset, &asn1_ctx, tree, hf_s1ap_BroadcastCompletedAreaList_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_Cause_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_Cause(tvb, offset, &asn1_ctx, tree, hf_s1ap_Cause_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_CellAccessMode_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_CellAccessMode(tvb, offset, &asn1_ctx, tree, hf_s1ap_CellAccessMode_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_Cdma2000PDU_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_Cdma2000PDU(tvb, offset, &asn1_ctx, tree, hf_s1ap_Cdma2000PDU_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_Cdma2000RATType_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_Cdma2000RATType(tvb, offset, &asn1_ctx, tree, hf_s1ap_Cdma2000RATType_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_Cdma2000SectorID_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_Cdma2000SectorID(tvb, offset, &asn1_ctx, tree, hf_s1ap_Cdma2000SectorID_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_Cdma2000HOStatus_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_Cdma2000HOStatus(tvb, offset, &asn1_ctx, tree, hf_s1ap_Cdma2000HOStatus_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_Cdma2000HORequiredIndication_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_Cdma2000HORequiredIndication(tvb, offset, &asn1_ctx, tree, hf_s1ap_Cdma2000HORequiredIndication_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_Cdma2000OneXSRVCCInfo_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_Cdma2000OneXSRVCCInfo(tvb, offset, &asn1_ctx, tree, hf_s1ap_Cdma2000OneXSRVCCInfo_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_Cdma2000OneXRAND_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_Cdma2000OneXRAND(tvb, offset, &asn1_ctx, tree, hf_s1ap_Cdma2000OneXRAND_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_CNDomain_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_CNDomain(tvb, offset, &asn1_ctx, tree, hf_s1ap_CNDomain_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_ConcurrentWarningMessageIndicator_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ConcurrentWarningMessageIndicator(tvb, offset, &asn1_ctx, tree, hf_s1ap_ConcurrentWarningMessageIndicator_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_Correlation_ID_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_Correlation_ID(tvb, offset, &asn1_ctx, tree, hf_s1ap_Correlation_ID_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_CSFallbackIndicator_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_CSFallbackIndicator(tvb, offset, &asn1_ctx, tree, hf_s1ap_CSFallbackIndicator_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_AdditionalCSFallbackIndicator_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_AdditionalCSFallbackIndicator(tvb, offset, &asn1_ctx, tree, hf_s1ap_AdditionalCSFallbackIndicator_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_CSG_Id_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_CSG_Id(tvb, offset, &asn1_ctx, tree, hf_s1ap_CSG_Id_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_CSG_IdList_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_CSG_IdList(tvb, offset, &asn1_ctx, tree, hf_s1ap_CSG_IdList_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_CSGMembershipStatus_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_CSGMembershipStatus(tvb, offset, &asn1_ctx, tree, hf_s1ap_CSGMembershipStatus_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_COUNTValueExtended_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_COUNTValueExtended(tvb, offset, &asn1_ctx, tree, hf_s1ap_COUNTValueExtended_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_COUNTvaluePDCP_SNlength18_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_COUNTvaluePDCP_SNlength18(tvb, offset, &asn1_ctx, tree, hf_s1ap_COUNTvaluePDCP_SNlength18_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_CriticalityDiagnostics_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_CriticalityDiagnostics(tvb, offset, &asn1_ctx, tree, hf_s1ap_CriticalityDiagnostics_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_DataCodingScheme_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_DataCodingScheme(tvb, offset, &asn1_ctx, tree, hf_s1ap_DataCodingScheme_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_Direct_Forwarding_Path_Availability_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_Direct_Forwarding_Path_Availability(tvb, offset, &asn1_ctx, tree, hf_s1ap_Direct_Forwarding_Path_Availability_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_Data_Forwarding_Not_Possible_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_Data_Forwarding_Not_Possible(tvb, offset, &asn1_ctx, tree, hf_s1ap_Data_Forwarding_Not_Possible_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_PWSfailedECGIList_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_PWSfailedECGIList(tvb, offset, &asn1_ctx, tree, hf_s1ap_PWSfailedECGIList_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_EmergencyAreaIDListForRestart_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_EmergencyAreaIDListForRestart(tvb, offset, &asn1_ctx, tree, hf_s1ap_EmergencyAreaIDListForRestart_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
int dissect_s1ap_Global_ENB_ID_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_Global_ENB_ID(tvb, offset, &asn1_ctx, tree, hf_s1ap_s1ap_Global_ENB_ID_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_GUMMEIList_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_GUMMEIList(tvb, offset, &asn1_ctx, tree, hf_s1ap_GUMMEIList_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
int dissect_s1ap_ENB_StatusTransfer_TransparentContainer_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ENB_StatusTransfer_TransparentContainer(tvb, offset, &asn1_ctx, tree, hf_s1ap_s1ap_ENB_StatusTransfer_TransparentContainer_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_ENB_UE_S1AP_ID_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ENB_UE_S1AP_ID(tvb, offset, &asn1_ctx, tree, hf_s1ap_ENB_UE_S1AP_ID_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_ENBname_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ENBname(tvb, offset, &asn1_ctx, tree, hf_s1ap_ENBname_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABInformationListItem_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABInformationListItem(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABInformationListItem_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABList_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABList(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABList_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABItem_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABItem(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABItem_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_EUTRAN_CGI_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_EUTRAN_CGI(tvb, offset, &asn1_ctx, tree, hf_s1ap_EUTRAN_CGI_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_EUTRANRoundTripDelayEstimationInfo_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_EUTRANRoundTripDelayEstimationInfo(tvb, offset, &asn1_ctx, tree, hf_s1ap_EUTRANRoundTripDelayEstimationInfo_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_ExpectedUEBehaviour_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ExpectedUEBehaviour(tvb, offset, &asn1_ctx, tree, hf_s1ap_ExpectedUEBehaviour_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_ExtendedRepetitionPeriod_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ExtendedRepetitionPeriod(tvb, offset, &asn1_ctx, tree, hf_s1ap_ExtendedRepetitionPeriod_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_GUMMEI_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_GUMMEI(tvb, offset, &asn1_ctx, tree, hf_s1ap_GUMMEI_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_GUMMEIType_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_GUMMEIType(tvb, offset, &asn1_ctx, tree, hf_s1ap_GUMMEIType_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_GWContextReleaseIndication_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_GWContextReleaseIndication(tvb, offset, &asn1_ctx, tree, hf_s1ap_GWContextReleaseIndication_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_HandoverRestrictionList_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_HandoverRestrictionList(tvb, offset, &asn1_ctx, tree, hf_s1ap_HandoverRestrictionList_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_HandoverType_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_HandoverType(tvb, offset, &asn1_ctx, tree, hf_s1ap_HandoverType_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_Masked_IMEISV_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_Masked_IMEISV(tvb, offset, &asn1_ctx, tree, hf_s1ap_Masked_IMEISV_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_KillAllWarningMessages_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_KillAllWarningMessages(tvb, offset, &asn1_ctx, tree, hf_s1ap_KillAllWarningMessages_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_LAI_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_LAI(tvb, offset, &asn1_ctx, tree, hf_s1ap_LAI_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_LPPa_PDU_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_LPPa_PDU(tvb, offset, &asn1_ctx, tree, hf_s1ap_LPPa_PDU_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_LHN_ID_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_LHN_ID(tvb, offset, &asn1_ctx, tree, hf_s1ap_LHN_ID_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_LoggedMBSFNMDT_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_LoggedMBSFNMDT(tvb, offset, &asn1_ctx, tree, hf_s1ap_LoggedMBSFNMDT_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_M3Configuration_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_M3Configuration(tvb, offset, &asn1_ctx, tree, hf_s1ap_M3Configuration_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_M4Configuration_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_M4Configuration(tvb, offset, &asn1_ctx, tree, hf_s1ap_M4Configuration_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_M5Configuration_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_M5Configuration(tvb, offset, &asn1_ctx, tree, hf_s1ap_M5Configuration_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_M6Configuration_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_M6Configuration(tvb, offset, &asn1_ctx, tree, hf_s1ap_M6Configuration_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_M7Configuration_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_M7Configuration(tvb, offset, &asn1_ctx, tree, hf_s1ap_M7Configuration_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_MDT_Location_Info_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_MDT_Location_Info(tvb, offset, &asn1_ctx, tree, hf_s1ap_MDT_Location_Info_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_MDT_Configuration_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_MDT_Configuration(tvb, offset, &asn1_ctx, tree, hf_s1ap_MDT_Configuration_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_ManagementBasedMDTAllowed_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ManagementBasedMDTAllowed(tvb, offset, &asn1_ctx, tree, hf_s1ap_ManagementBasedMDTAllowed_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_MDTPLMNList_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_MDTPLMNList(tvb, offset, &asn1_ctx, tree, hf_s1ap_MDTPLMNList_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_PrivacyIndicator_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_PrivacyIndicator(tvb, offset, &asn1_ctx, tree, hf_s1ap_PrivacyIndicator_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_MessageIdentifier_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_MessageIdentifier(tvb, offset, &asn1_ctx, tree, hf_s1ap_MessageIdentifier_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_MobilityInformation_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_MobilityInformation(tvb, offset, &asn1_ctx, tree, hf_s1ap_MobilityInformation_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_MMEname_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_MMEname(tvb, offset, &asn1_ctx, tree, hf_s1ap_MMEname_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_MMERelaySupportIndicator_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_MMERelaySupportIndicator(tvb, offset, &asn1_ctx, tree, hf_s1ap_MMERelaySupportIndicator_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_MME_Group_ID_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_MME_Group_ID(tvb, offset, &asn1_ctx, tree, hf_s1ap_MME_Group_ID_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_MME_UE_S1AP_ID_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_MME_UE_S1AP_ID(tvb, offset, &asn1_ctx, tree, hf_s1ap_MME_UE_S1AP_ID_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_MSClassmark2_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_MSClassmark2(tvb, offset, &asn1_ctx, tree, hf_s1ap_MSClassmark2_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_MSClassmark3_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_MSClassmark3(tvb, offset, &asn1_ctx, tree, hf_s1ap_MSClassmark3_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_MutingAvailabilityIndication_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_MutingAvailabilityIndication(tvb, offset, &asn1_ctx, tree, hf_s1ap_MutingAvailabilityIndication_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_MutingPatternInformation_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_MutingPatternInformation(tvb, offset, &asn1_ctx, tree, hf_s1ap_MutingPatternInformation_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_NAS_PDU_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_NAS_PDU(tvb, offset, &asn1_ctx, tree, hf_s1ap_NAS_PDU_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_NASSecurityParametersfromE_UTRAN_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_NASSecurityParametersfromE_UTRAN(tvb, offset, &asn1_ctx, tree, hf_s1ap_NASSecurityParametersfromE_UTRAN_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_NASSecurityParameterstoE_UTRAN_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_NASSecurityParameterstoE_UTRAN(tvb, offset, &asn1_ctx, tree, hf_s1ap_NASSecurityParameterstoE_UTRAN_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_NumberofBroadcastRequest_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_NumberofBroadcastRequest(tvb, offset, &asn1_ctx, tree, hf_s1ap_NumberofBroadcastRequest_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_OverloadResponse_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_OverloadResponse(tvb, offset, &asn1_ctx, tree, hf_s1ap_OverloadResponse_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_PagingDRX_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_PagingDRX(tvb, offset, &asn1_ctx, tree, hf_s1ap_PagingDRX_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_PagingPriority_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_PagingPriority(tvb, offset, &asn1_ctx, tree, hf_s1ap_PagingPriority_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_ProSeAuthorized_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ProSeAuthorized(tvb, offset, &asn1_ctx, tree, hf_s1ap_ProSeAuthorized_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_ProSeUEtoNetworkRelaying_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ProSeUEtoNetworkRelaying(tvb, offset, &asn1_ctx, tree, hf_s1ap_ProSeUEtoNetworkRelaying_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_PS_ServiceNotAvailable_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_PS_ServiceNotAvailable(tvb, offset, &asn1_ctx, tree, hf_s1ap_PS_ServiceNotAvailable_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_ReceiveStatusOfULPDCPSDUsExtended_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ReceiveStatusOfULPDCPSDUsExtended(tvb, offset, &asn1_ctx, tree, hf_s1ap_ReceiveStatusOfULPDCPSDUsExtended_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_ReceiveStatusOfULPDCPSDUsPDCP_SNlength18_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ReceiveStatusOfULPDCPSDUsPDCP_SNlength18(tvb, offset, &asn1_ctx, tree, hf_s1ap_ReceiveStatusOfULPDCPSDUsPDCP_SNlength18_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_RelativeMMECapacity_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_RelativeMMECapacity(tvb, offset, &asn1_ctx, tree, hf_s1ap_RelativeMMECapacity_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_RelayNode_Indicator_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_RelayNode_Indicator(tvb, offset, &asn1_ctx, tree, hf_s1ap_RelayNode_Indicator_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_RequestType_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_RequestType(tvb, offset, &asn1_ctx, tree, hf_s1ap_RequestType_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_RepetitionPeriod_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_RepetitionPeriod(tvb, offset, &asn1_ctx, tree, hf_s1ap_RepetitionPeriod_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_RRC_Establishment_Cause_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_RRC_Establishment_Cause(tvb, offset, &asn1_ctx, tree, hf_s1ap_RRC_Establishment_Cause_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_ECGIListForRestart_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ECGIListForRestart(tvb, offset, &asn1_ctx, tree, hf_s1ap_ECGIListForRestart_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_Routing_ID_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_Routing_ID(tvb, offset, &asn1_ctx, tree, hf_s1ap_Routing_ID_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_SecurityKey_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_SecurityKey(tvb, offset, &asn1_ctx, tree, hf_s1ap_SecurityKey_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_SecurityContext_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_SecurityContext(tvb, offset, &asn1_ctx, tree, hf_s1ap_SecurityContext_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_SerialNumber_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_SerialNumber(tvb, offset, &asn1_ctx, tree, hf_s1ap_SerialNumber_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_SONInformationReport_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_SONInformationReport(tvb, offset, &asn1_ctx, tree, hf_s1ap_SONInformationReport_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_SONConfigurationTransfer_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_SONConfigurationTransfer(tvb, offset, &asn1_ctx, tree, hf_s1ap_SONConfigurationTransfer_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_SynchronisationInformation_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_SynchronisationInformation(tvb, offset, &asn1_ctx, tree, hf_s1ap_SynchronisationInformation_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_Source_ToTarget_TransparentContainer_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_Source_ToTarget_TransparentContainer(tvb, offset, &asn1_ctx, tree, hf_s1ap_Source_ToTarget_TransparentContainer_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_SRVCCOperationPossible_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_SRVCCOperationPossible(tvb, offset, &asn1_ctx, tree, hf_s1ap_SRVCCOperationPossible_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_SRVCCHOIndication_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_SRVCCHOIndication(tvb, offset, &asn1_ctx, tree, hf_s1ap_SRVCCHOIndication_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_SourceeNB_ToTargeteNB_TransparentContainer_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_SourceeNB_ToTargeteNB_TransparentContainer(tvb, offset, &asn1_ctx, tree, hf_s1ap_SourceeNB_ToTargeteNB_TransparentContainer_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_ServedGUMMEIs_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ServedGUMMEIs(tvb, offset, &asn1_ctx, tree, hf_s1ap_ServedGUMMEIs_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_ServedPLMNs_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ServedPLMNs(tvb, offset, &asn1_ctx, tree, hf_s1ap_ServedPLMNs_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_SubscriberProfileIDforRFP_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_SubscriberProfileIDforRFP(tvb, offset, &asn1_ctx, tree, hf_s1ap_SubscriberProfileIDforRFP_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_SupportedTAs_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_SupportedTAs(tvb, offset, &asn1_ctx, tree, hf_s1ap_SupportedTAs_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_TimeSynchronisationInfo_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_TimeSynchronisationInfo(tvb, offset, &asn1_ctx, tree, hf_s1ap_TimeSynchronisationInfo_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_S_TMSI_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_S_TMSI(tvb, offset, &asn1_ctx, tree, hf_s1ap_S_TMSI_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_TAI_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_TAI(tvb, offset, &asn1_ctx, tree, hf_s1ap_TAI_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_TargetID_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_TargetID(tvb, offset, &asn1_ctx, tree, hf_s1ap_TargetID_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_TargeteNB_ToSourceeNB_TransparentContainer_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_TargeteNB_ToSourceeNB_TransparentContainer(tvb, offset, &asn1_ctx, tree, hf_s1ap_TargeteNB_ToSourceeNB_TransparentContainer_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_Target_ToSource_TransparentContainer_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_Target_ToSource_TransparentContainer(tvb, offset, &asn1_ctx, tree, hf_s1ap_Target_ToSource_TransparentContainer_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_TimeToWait_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_TimeToWait(tvb, offset, &asn1_ctx, tree, hf_s1ap_TimeToWait_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_Time_UE_StayedInCell_EnhancedGranularity_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_Time_UE_StayedInCell_EnhancedGranularity(tvb, offset, &asn1_ctx, tree, hf_s1ap_Time_UE_StayedInCell_EnhancedGranularity_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_TransportInformation_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_TransportInformation(tvb, offset, &asn1_ctx, tree, hf_s1ap_TransportInformation_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_TransportLayerAddress_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_TransportLayerAddress(tvb, offset, &asn1_ctx, tree, hf_s1ap_TransportLayerAddress_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_TraceActivation_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_TraceActivation(tvb, offset, &asn1_ctx, tree, hf_s1ap_TraceActivation_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_UTRAN_Trace_ID_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_UTRAN_Trace_ID(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_UTRAN_Trace_ID_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_TrafficLoadReductionIndication_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_TrafficLoadReductionIndication(tvb, offset, &asn1_ctx, tree, hf_s1ap_TrafficLoadReductionIndication_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_TunnelInformation_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_TunnelInformation(tvb, offset, &asn1_ctx, tree, hf_s1ap_TunnelInformation_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_TAIListForRestart_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_TAIListForRestart(tvb, offset, &asn1_ctx, tree, hf_s1ap_TAIListForRestart_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UEAggregateMaximumBitrate_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UEAggregateMaximumBitrate(tvb, offset, &asn1_ctx, tree, hf_s1ap_UEAggregateMaximumBitrate_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UE_S1AP_IDs_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UE_S1AP_IDs(tvb, offset, &asn1_ctx, tree, hf_s1ap_UE_S1AP_IDs_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UE_associatedLogicalS1_ConnectionItem_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UE_associatedLogicalS1_ConnectionItem(tvb, offset, &asn1_ctx, tree, hf_s1ap_UE_associatedLogicalS1_ConnectionItem_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UEIdentityIndexValue_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UEIdentityIndexValue(tvb, offset, &asn1_ctx, tree, hf_s1ap_UEIdentityIndexValue_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
int dissect_s1ap_UE_HistoryInformation_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UE_HistoryInformation(tvb, offset, &asn1_ctx, tree, hf_s1ap_s1ap_UE_HistoryInformation_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UE_HistoryInformationFromTheUE_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UE_HistoryInformationFromTheUE(tvb, offset, &asn1_ctx, tree, hf_s1ap_UE_HistoryInformationFromTheUE_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UEPagingID_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UEPagingID(tvb, offset, &asn1_ctx, tree, hf_s1ap_UEPagingID_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UERadioCapability_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UERadioCapability(tvb, offset, &asn1_ctx, tree, hf_s1ap_UERadioCapability_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UERadioCapabilityForPaging_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UERadioCapabilityForPaging(tvb, offset, &asn1_ctx, tree, hf_s1ap_UERadioCapabilityForPaging_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UESecurityCapabilities_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UESecurityCapabilities(tvb, offset, &asn1_ctx, tree, hf_s1ap_UESecurityCapabilities_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UserLocationInformation_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UserLocationInformation(tvb, offset, &asn1_ctx, tree, hf_s1ap_UserLocationInformation_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_VoiceSupportMatchIndicator_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_VoiceSupportMatchIndicator(tvb, offset, &asn1_ctx, tree, hf_s1ap_VoiceSupportMatchIndicator_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_WarningAreaList_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_WarningAreaList(tvb, offset, &asn1_ctx, tree, hf_s1ap_WarningAreaList_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_WarningType_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_WarningType(tvb, offset, &asn1_ctx, tree, hf_s1ap_WarningType_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_WarningSecurityInfo_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_WarningSecurityInfo(tvb, offset, &asn1_ctx, tree, hf_s1ap_WarningSecurityInfo_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_WarningMessageContents_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_WarningMessageContents(tvb, offset, &asn1_ctx, tree, hf_s1ap_WarningMessageContents_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_X2TNLConfigurationInfo_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_X2TNLConfigurationInfo(tvb, offset, &asn1_ctx, tree, hf_s1ap_X2TNLConfigurationInfo_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_ENBX2ExtTLAs_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ENBX2ExtTLAs(tvb, offset, &asn1_ctx, tree, hf_s1ap_ENBX2ExtTLAs_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_ENBIndirectX2TransportLayerAddresses_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ENBIndirectX2TransportLayerAddresses(tvb, offset, &asn1_ctx, tree, hf_s1ap_ENBIndirectX2TransportLayerAddresses_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_HandoverRequired_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_HandoverRequired(tvb, offset, &asn1_ctx, tree, hf_s1ap_HandoverRequired_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_HandoverCommand_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_HandoverCommand(tvb, offset, &asn1_ctx, tree, hf_s1ap_HandoverCommand_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABSubjecttoDataForwardingList_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABSubjecttoDataForwardingList(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABSubjecttoDataForwardingList_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABDataForwardingItem_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABDataForwardingItem(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABDataForwardingItem_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_HandoverPreparationFailure_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_HandoverPreparationFailure(tvb, offset, &asn1_ctx, tree, hf_s1ap_HandoverPreparationFailure_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_HandoverRequest_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_HandoverRequest(tvb, offset, &asn1_ctx, tree, hf_s1ap_HandoverRequest_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABToBeSetupListHOReq_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABToBeSetupListHOReq(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABToBeSetupListHOReq_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABToBeSetupItemHOReq_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABToBeSetupItemHOReq(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABToBeSetupItemHOReq_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_HandoverRequestAcknowledge_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_HandoverRequestAcknowledge(tvb, offset, &asn1_ctx, tree, hf_s1ap_HandoverRequestAcknowledge_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABAdmittedList_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABAdmittedList(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABAdmittedList_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABAdmittedItem_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABAdmittedItem(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABAdmittedItem_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABFailedtoSetupListHOReqAck_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABFailedtoSetupListHOReqAck(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABFailedtoSetupListHOReqAck_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABFailedToSetupItemHOReqAck_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABFailedToSetupItemHOReqAck(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABFailedToSetupItemHOReqAck_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_HandoverFailure_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_HandoverFailure(tvb, offset, &asn1_ctx, tree, hf_s1ap_HandoverFailure_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_HandoverNotify_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_HandoverNotify(tvb, offset, &asn1_ctx, tree, hf_s1ap_HandoverNotify_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_PathSwitchRequest_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_PathSwitchRequest(tvb, offset, &asn1_ctx, tree, hf_s1ap_PathSwitchRequest_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABToBeSwitchedDLList_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABToBeSwitchedDLList(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABToBeSwitchedDLList_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABToBeSwitchedDLItem_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABToBeSwitchedDLItem(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABToBeSwitchedDLItem_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_PathSwitchRequestAcknowledge_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_PathSwitchRequestAcknowledge(tvb, offset, &asn1_ctx, tree, hf_s1ap_PathSwitchRequestAcknowledge_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABToBeSwitchedULList_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABToBeSwitchedULList(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABToBeSwitchedULList_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABToBeSwitchedULItem_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABToBeSwitchedULItem(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABToBeSwitchedULItem_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_PathSwitchRequestFailure_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_PathSwitchRequestFailure(tvb, offset, &asn1_ctx, tree, hf_s1ap_PathSwitchRequestFailure_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_HandoverCancel_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_HandoverCancel(tvb, offset, &asn1_ctx, tree, hf_s1ap_HandoverCancel_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_HandoverCancelAcknowledge_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_HandoverCancelAcknowledge(tvb, offset, &asn1_ctx, tree, hf_s1ap_HandoverCancelAcknowledge_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABSetupRequest_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABSetupRequest(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABSetupRequest_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABToBeSetupListBearerSUReq_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABToBeSetupListBearerSUReq(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABToBeSetupListBearerSUReq_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABToBeSetupItemBearerSUReq_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABToBeSetupItemBearerSUReq(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABToBeSetupItemBearerSUReq_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABSetupResponse_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABSetupResponse(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABSetupResponse_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABSetupListBearerSURes_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABSetupListBearerSURes(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABSetupListBearerSURes_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABSetupItemBearerSURes_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABSetupItemBearerSURes(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABSetupItemBearerSURes_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABModifyRequest_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABModifyRequest(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABModifyRequest_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABToBeModifiedListBearerModReq_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABToBeModifiedListBearerModReq(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABToBeModifiedListBearerModReq_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABToBeModifiedItemBearerModReq_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABToBeModifiedItemBearerModReq(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABToBeModifiedItemBearerModReq_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABModifyResponse_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABModifyResponse(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABModifyResponse_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABModifyListBearerModRes_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABModifyListBearerModRes(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABModifyListBearerModRes_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABModifyItemBearerModRes_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABModifyItemBearerModRes(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABModifyItemBearerModRes_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABReleaseCommand_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABReleaseCommand(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABReleaseCommand_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABReleaseResponse_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABReleaseResponse(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABReleaseResponse_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABReleaseListBearerRelComp_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABReleaseListBearerRelComp(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABReleaseListBearerRelComp_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABReleaseItemBearerRelComp_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABReleaseItemBearerRelComp(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABReleaseItemBearerRelComp_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABReleaseIndication_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABReleaseIndication(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABReleaseIndication_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_InitialContextSetupRequest_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_InitialContextSetupRequest(tvb, offset, &asn1_ctx, tree, hf_s1ap_InitialContextSetupRequest_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABToBeSetupListCtxtSUReq_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABToBeSetupListCtxtSUReq(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABToBeSetupListCtxtSUReq_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABToBeSetupItemCtxtSUReq_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABToBeSetupItemCtxtSUReq(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABToBeSetupItemCtxtSUReq_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_InitialContextSetupResponse_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_InitialContextSetupResponse(tvb, offset, &asn1_ctx, tree, hf_s1ap_InitialContextSetupResponse_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABSetupListCtxtSURes_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABSetupListCtxtSURes(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABSetupListCtxtSURes_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABSetupItemCtxtSURes_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABSetupItemCtxtSURes(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABSetupItemCtxtSURes_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_InitialContextSetupFailure_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_InitialContextSetupFailure(tvb, offset, &asn1_ctx, tree, hf_s1ap_InitialContextSetupFailure_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_Paging_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_Paging(tvb, offset, &asn1_ctx, tree, hf_s1ap_Paging_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_TAIList_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_TAIList(tvb, offset, &asn1_ctx, tree, hf_s1ap_TAIList_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_TAIItem_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_TAIItem(tvb, offset, &asn1_ctx, tree, hf_s1ap_TAIItem_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UEContextReleaseRequest_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UEContextReleaseRequest(tvb, offset, &asn1_ctx, tree, hf_s1ap_UEContextReleaseRequest_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UEContextReleaseCommand_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UEContextReleaseCommand(tvb, offset, &asn1_ctx, tree, hf_s1ap_UEContextReleaseCommand_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UEContextReleaseComplete_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UEContextReleaseComplete(tvb, offset, &asn1_ctx, tree, hf_s1ap_UEContextReleaseComplete_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UEContextModificationRequest_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UEContextModificationRequest(tvb, offset, &asn1_ctx, tree, hf_s1ap_UEContextModificationRequest_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UEContextModificationResponse_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UEContextModificationResponse(tvb, offset, &asn1_ctx, tree, hf_s1ap_UEContextModificationResponse_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UEContextModificationFailure_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UEContextModificationFailure(tvb, offset, &asn1_ctx, tree, hf_s1ap_UEContextModificationFailure_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UERadioCapabilityMatchRequest_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UERadioCapabilityMatchRequest(tvb, offset, &asn1_ctx, tree, hf_s1ap_UERadioCapabilityMatchRequest_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UERadioCapabilityMatchResponse_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UERadioCapabilityMatchResponse(tvb, offset, &asn1_ctx, tree, hf_s1ap_UERadioCapabilityMatchResponse_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_DownlinkNASTransport_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_DownlinkNASTransport(tvb, offset, &asn1_ctx, tree, hf_s1ap_DownlinkNASTransport_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_InitialUEMessage_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_InitialUEMessage(tvb, offset, &asn1_ctx, tree, hf_s1ap_InitialUEMessage_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UplinkNASTransport_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UplinkNASTransport(tvb, offset, &asn1_ctx, tree, hf_s1ap_UplinkNASTransport_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_NASNonDeliveryIndication_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_NASNonDeliveryIndication(tvb, offset, &asn1_ctx, tree, hf_s1ap_NASNonDeliveryIndication_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_RerouteNASRequest_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_RerouteNASRequest(tvb, offset, &asn1_ctx, tree, hf_s1ap_RerouteNASRequest_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_Reset_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_Reset(tvb, offset, &asn1_ctx, tree, hf_s1ap_Reset_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_ResetType_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ResetType(tvb, offset, &asn1_ctx, tree, hf_s1ap_ResetType_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_ResetAcknowledge_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ResetAcknowledge(tvb, offset, &asn1_ctx, tree, hf_s1ap_ResetAcknowledge_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UE_associatedLogicalS1_ConnectionListResAck_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UE_associatedLogicalS1_ConnectionListResAck(tvb, offset, &asn1_ctx, tree, hf_s1ap_UE_associatedLogicalS1_ConnectionListResAck_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_ErrorIndication_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ErrorIndication(tvb, offset, &asn1_ctx, tree, hf_s1ap_ErrorIndication_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_S1SetupRequest_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_S1SetupRequest(tvb, offset, &asn1_ctx, tree, hf_s1ap_S1SetupRequest_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_S1SetupResponse_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_S1SetupResponse(tvb, offset, &asn1_ctx, tree, hf_s1ap_S1SetupResponse_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_S1SetupFailure_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_S1SetupFailure(tvb, offset, &asn1_ctx, tree, hf_s1ap_S1SetupFailure_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_ENBConfigurationUpdate_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ENBConfigurationUpdate(tvb, offset, &asn1_ctx, tree, hf_s1ap_ENBConfigurationUpdate_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_ENBConfigurationUpdateAcknowledge_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ENBConfigurationUpdateAcknowledge(tvb, offset, &asn1_ctx, tree, hf_s1ap_ENBConfigurationUpdateAcknowledge_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_ENBConfigurationUpdateFailure_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ENBConfigurationUpdateFailure(tvb, offset, &asn1_ctx, tree, hf_s1ap_ENBConfigurationUpdateFailure_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_MMEConfigurationUpdate_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_MMEConfigurationUpdate(tvb, offset, &asn1_ctx, tree, hf_s1ap_MMEConfigurationUpdate_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_MMEConfigurationUpdateAcknowledge_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_MMEConfigurationUpdateAcknowledge(tvb, offset, &asn1_ctx, tree, hf_s1ap_MMEConfigurationUpdateAcknowledge_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_MMEConfigurationUpdateFailure_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_MMEConfigurationUpdateFailure(tvb, offset, &asn1_ctx, tree, hf_s1ap_MMEConfigurationUpdateFailure_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_DownlinkS1cdma2000tunnelling_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_DownlinkS1cdma2000tunnelling(tvb, offset, &asn1_ctx, tree, hf_s1ap_DownlinkS1cdma2000tunnelling_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UplinkS1cdma2000tunnelling_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UplinkS1cdma2000tunnelling(tvb, offset, &asn1_ctx, tree, hf_s1ap_UplinkS1cdma2000tunnelling_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UECapabilityInfoIndication_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UECapabilityInfoIndication(tvb, offset, &asn1_ctx, tree, hf_s1ap_UECapabilityInfoIndication_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_ENBStatusTransfer_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ENBStatusTransfer(tvb, offset, &asn1_ctx, tree, hf_s1ap_ENBStatusTransfer_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_MMEStatusTransfer_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_MMEStatusTransfer(tvb, offset, &asn1_ctx, tree, hf_s1ap_MMEStatusTransfer_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_TraceStart_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_TraceStart(tvb, offset, &asn1_ctx, tree, hf_s1ap_TraceStart_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_TraceFailureIndication_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_TraceFailureIndication(tvb, offset, &asn1_ctx, tree, hf_s1ap_TraceFailureIndication_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_DeactivateTrace_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_DeactivateTrace(tvb, offset, &asn1_ctx, tree, hf_s1ap_DeactivateTrace_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_CellTrafficTrace_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_CellTrafficTrace(tvb, offset, &asn1_ctx, tree, hf_s1ap_CellTrafficTrace_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_LocationReportingControl_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_LocationReportingControl(tvb, offset, &asn1_ctx, tree, hf_s1ap_LocationReportingControl_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_LocationReportingFailureIndication_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_LocationReportingFailureIndication(tvb, offset, &asn1_ctx, tree, hf_s1ap_LocationReportingFailureIndication_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_LocationReport_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_LocationReport(tvb, offset, &asn1_ctx, tree, hf_s1ap_LocationReport_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_OverloadStart_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_OverloadStart(tvb, offset, &asn1_ctx, tree, hf_s1ap_OverloadStart_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_OverloadStop_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_OverloadStop(tvb, offset, &asn1_ctx, tree, hf_s1ap_OverloadStop_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_WriteReplaceWarningRequest_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_WriteReplaceWarningRequest(tvb, offset, &asn1_ctx, tree, hf_s1ap_WriteReplaceWarningRequest_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_WriteReplaceWarningResponse_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_WriteReplaceWarningResponse(tvb, offset, &asn1_ctx, tree, hf_s1ap_WriteReplaceWarningResponse_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_ENBDirectInformationTransfer_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ENBDirectInformationTransfer(tvb, offset, &asn1_ctx, tree, hf_s1ap_ENBDirectInformationTransfer_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_Inter_SystemInformationTransferType_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_Inter_SystemInformationTransferType(tvb, offset, &asn1_ctx, tree, hf_s1ap_Inter_SystemInformationTransferType_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_MMEDirectInformationTransfer_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_MMEDirectInformationTransfer(tvb, offset, &asn1_ctx, tree, hf_s1ap_MMEDirectInformationTransfer_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_ENBConfigurationTransfer_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_ENBConfigurationTransfer(tvb, offset, &asn1_ctx, tree, hf_s1ap_ENBConfigurationTransfer_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_MMEConfigurationTransfer_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_MMEConfigurationTransfer(tvb, offset, &asn1_ctx, tree, hf_s1ap_MMEConfigurationTransfer_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_PrivateMessage_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_PrivateMessage(tvb, offset, &asn1_ctx, tree, hf_s1ap_PrivateMessage_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_KillRequest_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_KillRequest(tvb, offset, &asn1_ctx, tree, hf_s1ap_KillRequest_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_KillResponse_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_KillResponse(tvb, offset, &asn1_ctx, tree, hf_s1ap_KillResponse_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_PWSRestartIndication_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_PWSRestartIndication(tvb, offset, &asn1_ctx, tree, hf_s1ap_PWSRestartIndication_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_PWSFailureIndication_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_PWSFailureIndication(tvb, offset, &asn1_ctx, tree, hf_s1ap_PWSFailureIndication_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_DownlinkUEAssociatedLPPaTransport_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_DownlinkUEAssociatedLPPaTransport(tvb, offset, &asn1_ctx, tree, hf_s1ap_DownlinkUEAssociatedLPPaTransport_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UplinkUEAssociatedLPPaTransport_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UplinkUEAssociatedLPPaTransport(tvb, offset, &asn1_ctx, tree, hf_s1ap_UplinkUEAssociatedLPPaTransport_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_DownlinkNonUEAssociatedLPPaTransport_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_DownlinkNonUEAssociatedLPPaTransport(tvb, offset, &asn1_ctx, tree, hf_s1ap_DownlinkNonUEAssociatedLPPaTransport_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UplinkNonUEAssociatedLPPaTransport_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UplinkNonUEAssociatedLPPaTransport(tvb, offset, &asn1_ctx, tree, hf_s1ap_UplinkNonUEAssociatedLPPaTransport_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABModificationIndication_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABModificationIndication(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABModificationIndication_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABToBeModifiedListBearerModInd_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABToBeModifiedListBearerModInd(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABToBeModifiedListBearerModInd_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABToBeModifiedItemBearerModInd_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABToBeModifiedItemBearerModInd(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABToBeModifiedItemBearerModInd_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABNotToBeModifiedListBearerModInd_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABNotToBeModifiedListBearerModInd(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABNotToBeModifiedListBearerModInd_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABNotToBeModifiedItemBearerModInd_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABNotToBeModifiedItemBearerModInd(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABNotToBeModifiedItemBearerModInd_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_CSGMembershipInfo_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_CSGMembershipInfo(tvb, offset, &asn1_ctx, tree, hf_s1ap_CSGMembershipInfo_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABModificationConfirm_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABModificationConfirm(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABModificationConfirm_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABModifyListBearerModConf_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABModifyListBearerModConf(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABModifyListBearerModConf_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_E_RABModifyItemBearerModConf_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_E_RABModifyItemBearerModConf(tvb, offset, &asn1_ctx, tree, hf_s1ap_E_RABModifyItemBearerModConf_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UEContextModificationIndication_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UEContextModificationIndication(tvb, offset, &asn1_ctx, tree, hf_s1ap_UEContextModificationIndication_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_UEContextModificationConfirm_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_UEContextModificationConfirm(tvb, offset, &asn1_ctx, tree, hf_s1ap_UEContextModificationConfirm_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_S1AP_PDU_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_S1AP_PDU(tvb, offset, &asn1_ctx, tree, hf_s1ap_S1AP_PDU_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
int dissect_s1ap_SONtransferApplicationIdentity_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_SONtransferApplicationIdentity(tvb, offset, &asn1_ctx, tree, hf_s1ap_s1ap_SONtransferApplicationIdentity_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
int dissect_s1ap_SONtransferRequestContainer_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_SONtransferRequestContainer(tvb, offset, &asn1_ctx, tree, hf_s1ap_s1ap_SONtransferRequestContainer_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
int dissect_s1ap_SONtransferResponseContainer_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_SONtransferResponseContainer(tvb, offset, &asn1_ctx, tree, hf_s1ap_s1ap_SONtransferResponseContainer_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
int dissect_s1ap_SONtransferCause_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, TRUE, pinfo);
  offset = dissect_s1ap_SONtransferCause(tvb, offset, &asn1_ctx, tree, hf_s1ap_s1ap_SONtransferCause_PDU);
  offset += 7; offset >>= 3;
  return offset;
}


/*--- End of included file: packet-s1ap-fn.c ---*/
#line 138 "./asn1/s1ap/packet-s1ap-template.c"

static int dissect_ProtocolIEFieldValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
  s1ap_ctx_t s1ap_ctx;

  s1ap_ctx.message_type        = message_type;
  s1ap_ctx.ProcedureCode       = ProcedureCode;
  s1ap_ctx.ProtocolIE_ID       = ProtocolIE_ID;
  s1ap_ctx.ProtocolExtensionID = ProtocolExtensionID;

  return (dissector_try_uint_new(s1ap_ies_dissector_table, ProtocolIE_ID, tvb, pinfo, tree, TRUE, &s1ap_ctx)) ? tvb_captured_length(tvb) : 0;
}
/* Currently not used
static int dissect_ProtocolIEFieldPairFirstValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  return (dissector_try_uint(s1ap_ies_p1_dissector_table, ProtocolIE_ID, tvb, pinfo, tree)) ? tvb_captured_length(tvb) : 0;
}

static int dissect_ProtocolIEFieldPairSecondValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  return (dissector_try_uint(s1ap_ies_p2_dissector_table, ProtocolIE_ID, tvb, pinfo, tree)) ? tvb_captured_length(tvb) : 0;
}
*/

static int dissect_ProtocolExtensionFieldExtensionValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
  s1ap_ctx_t s1ap_ctx;

  s1ap_ctx.message_type        = message_type;
  s1ap_ctx.ProcedureCode       = ProcedureCode;
  s1ap_ctx.ProtocolIE_ID       = ProtocolIE_ID;
  s1ap_ctx.ProtocolExtensionID = ProtocolExtensionID;

  return (dissector_try_uint_new(s1ap_extension_dissector_table, ProtocolExtensionID, tvb, pinfo, tree, TRUE, &s1ap_ctx)) ? tvb_captured_length(tvb) : 0;
}

static int dissect_InitiatingMessageValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data)
{
  return (dissector_try_uint_new(s1ap_proc_imsg_dissector_table, ProcedureCode, tvb, pinfo, tree, TRUE, data)) ? tvb_captured_length(tvb) : 0;
}

static int dissect_SuccessfulOutcomeValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data)
{
  return (dissector_try_uint_new(s1ap_proc_sout_dissector_table, ProcedureCode, tvb, pinfo, tree, TRUE, data)) ? tvb_captured_length(tvb) : 0;
}

static int dissect_UnsuccessfulOutcomeValue(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data)
{
  return (dissector_try_uint_new(s1ap_proc_uout_dissector_table, ProcedureCode, tvb, pinfo, tree, TRUE, data)) ? tvb_captured_length(tvb) : 0;
}


static int
dissect_s1ap(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
	proto_item	*s1ap_item = NULL;
	proto_tree	*s1ap_tree = NULL;

	/* make entry in the Protocol column on summary display */
	col_set_str(pinfo->cinfo, COL_PROTOCOL, "S1AP");

	/* create the s1ap protocol tree */
	s1ap_item = proto_tree_add_item(tree, proto_s1ap, tvb, 0, -1, ENC_NA);
	s1ap_tree = proto_item_add_subtree(s1ap_item, ett_s1ap);

	dissect_S1AP_PDU_PDU(tvb, pinfo, s1ap_tree, NULL);
	return tvb_captured_length(tvb);
}

/*--- proto_reg_handoff_s1ap ---------------------------------------*/
void
proto_reg_handoff_s1ap(void)
{
	static gboolean Initialized=FALSE;
	static guint SctpPort;

    gcsna_handle = find_dissector_add_dependency("gcsna", proto_s1ap);

	if (!Initialized) {
		nas_eps_handle = find_dissector_add_dependency("nas-eps", proto_s1ap);
		lppa_handle = find_dissector_add_dependency("lppa", proto_s1ap);
		bssgp_handle = find_dissector_add_dependency("bssgp", proto_s1ap);
		dissector_add_for_decode_as("sctp.port", s1ap_handle);
		dissector_add_uint("sctp.ppi", S1AP_PAYLOAD_PROTOCOL_ID,   s1ap_handle);
		Initialized=TRUE;

/*--- Included file: packet-s1ap-dis-tab.c ---*/
#line 1 "./asn1/s1ap/packet-s1ap-dis-tab.c"
  dissector_add_uint("s1ap.ies", id_MME_UE_S1AP_ID, create_dissector_handle(dissect_MME_UE_S1AP_ID_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_HandoverType, create_dissector_handle(dissect_HandoverType_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_Cause, create_dissector_handle(dissect_Cause_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_TargetID, create_dissector_handle(dissect_TargetID_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_eNB_UE_S1AP_ID, create_dissector_handle(dissect_ENB_UE_S1AP_ID_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABSubjecttoDataForwardingList, create_dissector_handle(dissect_E_RABSubjecttoDataForwardingList_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABtoReleaseListHOCmd, create_dissector_handle(dissect_E_RABList_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABDataForwardingItem, create_dissector_handle(dissect_E_RABDataForwardingItem_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABReleaseItemBearerRelComp, create_dissector_handle(dissect_E_RABReleaseItemBearerRelComp_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABToBeSetupListBearerSUReq, create_dissector_handle(dissect_E_RABToBeSetupListBearerSUReq_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABToBeSetupItemBearerSUReq, create_dissector_handle(dissect_E_RABToBeSetupItemBearerSUReq_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABAdmittedList, create_dissector_handle(dissect_E_RABAdmittedList_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABFailedToSetupListHOReqAck, create_dissector_handle(dissect_E_RABFailedtoSetupListHOReqAck_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABAdmittedItem, create_dissector_handle(dissect_E_RABAdmittedItem_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABFailedtoSetupItemHOReqAck, create_dissector_handle(dissect_E_RABFailedToSetupItemHOReqAck_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABToBeSwitchedDLList, create_dissector_handle(dissect_E_RABToBeSwitchedDLList_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABToBeSwitchedDLItem, create_dissector_handle(dissect_E_RABToBeSwitchedDLItem_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABToBeSetupListCtxtSUReq, create_dissector_handle(dissect_E_RABToBeSetupListCtxtSUReq_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_TraceActivation, create_dissector_handle(dissect_TraceActivation_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_UTRAN_Trace_ID, create_dissector_handle(dissect_E_UTRAN_Trace_ID_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_NAS_PDU, create_dissector_handle(dissect_NAS_PDU_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABToBeSetupItemHOReq, create_dissector_handle(dissect_E_RABToBeSetupItemHOReq_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABSetupListBearerSURes, create_dissector_handle(dissect_E_RABSetupListBearerSURes_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABFailedToSetupListBearerSURes, create_dissector_handle(dissect_E_RABList_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABToBeModifiedListBearerModReq, create_dissector_handle(dissect_E_RABToBeModifiedListBearerModReq_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABModifyListBearerModRes, create_dissector_handle(dissect_E_RABModifyListBearerModRes_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABFailedToModifyList, create_dissector_handle(dissect_E_RABList_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABToBeReleasedList, create_dissector_handle(dissect_E_RABList_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABFailedToReleaseList, create_dissector_handle(dissect_E_RABList_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABItem, create_dissector_handle(dissect_E_RABItem_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABToBeModifiedItemBearerModReq, create_dissector_handle(dissect_E_RABToBeModifiedItemBearerModReq_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABModifyItemBearerModRes, create_dissector_handle(dissect_E_RABModifyItemBearerModRes_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABSetupItemBearerSURes, create_dissector_handle(dissect_E_RABSetupItemBearerSURes_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_SecurityContext, create_dissector_handle(dissect_SecurityContext_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_HandoverRestrictionList, create_dissector_handle(dissect_HandoverRestrictionList_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_UEPagingID, create_dissector_handle(dissect_UEPagingID_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_pagingDRX, create_dissector_handle(dissect_PagingDRX_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_TAIList, create_dissector_handle(dissect_TAIList_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_TAIItem, create_dissector_handle(dissect_TAIItem_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABFailedToSetupListCtxtSURes, create_dissector_handle(dissect_E_RABList_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABSetupItemCtxtSURes, create_dissector_handle(dissect_E_RABSetupItemCtxtSURes_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABSetupListCtxtSURes, create_dissector_handle(dissect_E_RABSetupListCtxtSURes_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABToBeSetupItemCtxtSUReq, create_dissector_handle(dissect_E_RABToBeSetupItemCtxtSUReq_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABToBeSetupListHOReq, create_dissector_handle(dissect_E_RABToBeSetupListHOReq_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_CriticalityDiagnostics, create_dissector_handle(dissect_CriticalityDiagnostics_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_Global_ENB_ID, create_dissector_handle(dissect_s1ap_Global_ENB_ID_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_eNBname, create_dissector_handle(dissect_ENBname_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_MMEname, create_dissector_handle(dissect_MMEname_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_ServedPLMNs, create_dissector_handle(dissect_ServedPLMNs_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_SupportedTAs, create_dissector_handle(dissect_SupportedTAs_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_TimeToWait, create_dissector_handle(dissect_TimeToWait_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_uEaggregateMaximumBitrate, create_dissector_handle(dissect_UEAggregateMaximumBitrate_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_TAI, create_dissector_handle(dissect_TAI_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABReleaseListBearerRelComp, create_dissector_handle(dissect_E_RABReleaseListBearerRelComp_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_cdma2000PDU, create_dissector_handle(dissect_Cdma2000PDU_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_cdma2000RATType, create_dissector_handle(dissect_Cdma2000RATType_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_cdma2000SectorID, create_dissector_handle(dissect_Cdma2000SectorID_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_SecurityKey, create_dissector_handle(dissect_SecurityKey_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_UERadioCapability, create_dissector_handle(dissect_UERadioCapability_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_GUMMEI_ID, create_dissector_handle(dissect_GUMMEI_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABInformationListItem, create_dissector_handle(dissect_E_RABInformationListItem_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_Direct_Forwarding_Path_Availability, create_dissector_handle(dissect_Direct_Forwarding_Path_Availability_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_UEIdentityIndexValue, create_dissector_handle(dissect_UEIdentityIndexValue_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_cdma2000HOStatus, create_dissector_handle(dissect_Cdma2000HOStatus_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_cdma2000HORequiredIndication, create_dissector_handle(dissect_Cdma2000HORequiredIndication_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_RelativeMMECapacity, create_dissector_handle(dissect_RelativeMMECapacity_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_SourceMME_UE_S1AP_ID, create_dissector_handle(dissect_MME_UE_S1AP_ID_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_Bearers_SubjectToStatusTransfer_Item, create_dissector_handle(dissect_Bearers_SubjectToStatusTransfer_Item_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_eNB_StatusTransfer_TransparentContainer, create_dissector_handle(dissect_s1ap_ENB_StatusTransfer_TransparentContainer_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_UE_associatedLogicalS1_ConnectionItem, create_dissector_handle(dissect_UE_associatedLogicalS1_ConnectionItem_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_ResetType, create_dissector_handle(dissect_ResetType_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_UE_associatedLogicalS1_ConnectionListResAck, create_dissector_handle(dissect_UE_associatedLogicalS1_ConnectionListResAck_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABToBeSwitchedULItem, create_dissector_handle(dissect_E_RABToBeSwitchedULItem_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABToBeSwitchedULList, create_dissector_handle(dissect_E_RABToBeSwitchedULList_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_S_TMSI, create_dissector_handle(dissect_S_TMSI_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_cdma2000OneXRAND, create_dissector_handle(dissect_Cdma2000OneXRAND_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_RequestType, create_dissector_handle(dissect_RequestType_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_UE_S1AP_IDs, create_dissector_handle(dissect_UE_S1AP_IDs_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_EUTRAN_CGI, create_dissector_handle(dissect_EUTRAN_CGI_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_OverloadResponse, create_dissector_handle(dissect_OverloadResponse_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_cdma2000OneXSRVCCInfo, create_dissector_handle(dissect_Cdma2000OneXSRVCCInfo_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_Source_ToTarget_TransparentContainer, create_dissector_handle(dissect_Source_ToTarget_TransparentContainer_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_ServedGUMMEIs, create_dissector_handle(dissect_ServedGUMMEIs_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_SubscriberProfileIDforRFP, create_dissector_handle(dissect_SubscriberProfileIDforRFP_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_UESecurityCapabilities, create_dissector_handle(dissect_UESecurityCapabilities_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_CSFallbackIndicator, create_dissector_handle(dissect_CSFallbackIndicator_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_CNDomain, create_dissector_handle(dissect_CNDomain_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABReleasedList, create_dissector_handle(dissect_E_RABList_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_MessageIdentifier, create_dissector_handle(dissect_MessageIdentifier_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_SerialNumber, create_dissector_handle(dissect_SerialNumber_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_WarningAreaList, create_dissector_handle(dissect_WarningAreaList_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_RepetitionPeriod, create_dissector_handle(dissect_RepetitionPeriod_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_NumberofBroadcastRequest, create_dissector_handle(dissect_NumberofBroadcastRequest_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_WarningType, create_dissector_handle(dissect_WarningType_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_WarningSecurityInfo, create_dissector_handle(dissect_WarningSecurityInfo_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_DataCodingScheme, create_dissector_handle(dissect_DataCodingScheme_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_WarningMessageContents, create_dissector_handle(dissect_WarningMessageContents_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_BroadcastCompletedAreaList, create_dissector_handle(dissect_BroadcastCompletedAreaList_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_Inter_SystemInformationTransferTypeEDT, create_dissector_handle(dissect_Inter_SystemInformationTransferType_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_Inter_SystemInformationTransferTypeMDT, create_dissector_handle(dissect_Inter_SystemInformationTransferType_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_Target_ToSource_TransparentContainer, create_dissector_handle(dissect_Target_ToSource_TransparentContainer_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_SRVCCOperationPossible, create_dissector_handle(dissect_SRVCCOperationPossible_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_SRVCCHOIndication, create_dissector_handle(dissect_SRVCCHOIndication_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_CSG_Id, create_dissector_handle(dissect_CSG_Id_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_CSG_IdList, create_dissector_handle(dissect_CSG_IdList_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_SONConfigurationTransferECT, create_dissector_handle(dissect_SONConfigurationTransfer_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_SONConfigurationTransferMCT, create_dissector_handle(dissect_SONConfigurationTransfer_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_TraceCollectionEntityIPAddress, create_dissector_handle(dissect_TransportLayerAddress_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_MSClassmark2, create_dissector_handle(dissect_MSClassmark2_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_MSClassmark3, create_dissector_handle(dissect_MSClassmark3_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_RRC_Establishment_Cause, create_dissector_handle(dissect_RRC_Establishment_Cause_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_NASSecurityParametersfromE_UTRAN, create_dissector_handle(dissect_NASSecurityParametersfromE_UTRAN_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_NASSecurityParameterstoE_UTRAN, create_dissector_handle(dissect_NASSecurityParameterstoE_UTRAN_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_DefaultPagingDRX, create_dissector_handle(dissect_PagingDRX_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_Source_ToTarget_TransparentContainer_Secondary, create_dissector_handle(dissect_Source_ToTarget_TransparentContainer_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_Target_ToSource_TransparentContainer_Secondary, create_dissector_handle(dissect_Target_ToSource_TransparentContainer_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_EUTRANRoundTripDelayEstimationInfo, create_dissector_handle(dissect_EUTRANRoundTripDelayEstimationInfo_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_BroadcastCancelledAreaList, create_dissector_handle(dissect_BroadcastCancelledAreaList_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_ConcurrentWarningMessageIndicator, create_dissector_handle(dissect_ConcurrentWarningMessageIndicator_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_ExtendedRepetitionPeriod, create_dissector_handle(dissect_ExtendedRepetitionPeriod_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_CellAccessMode, create_dissector_handle(dissect_CellAccessMode_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_CSGMembershipStatus, create_dissector_handle(dissect_CSGMembershipStatus_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_LPPa_PDU, create_dissector_handle(dissect_LPPa_PDU_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_Routing_ID, create_dissector_handle(dissect_Routing_ID_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_PS_ServiceNotAvailable, create_dissector_handle(dissect_PS_ServiceNotAvailable_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_PagingPriority, create_dissector_handle(dissect_PagingPriority_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_GUMMEIList, create_dissector_handle(dissect_GUMMEIList_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_GW_TransportLayerAddress, create_dissector_handle(dissect_TransportLayerAddress_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_SourceMME_GUMMEI, create_dissector_handle(dissect_GUMMEI_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_MME_UE_S1AP_ID_2, create_dissector_handle(dissect_MME_UE_S1AP_ID_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_RegisteredLAI, create_dissector_handle(dissect_LAI_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_RelayNode_Indicator, create_dissector_handle(dissect_RelayNode_Indicator_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_TrafficLoadReductionIndication, create_dissector_handle(dissect_TrafficLoadReductionIndication_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_MMERelaySupportIndicator, create_dissector_handle(dissect_MMERelaySupportIndicator_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_GWContextReleaseIndication, create_dissector_handle(dissect_GWContextReleaseIndication_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_ManagementBasedMDTAllowed, create_dissector_handle(dissect_ManagementBasedMDTAllowed_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_PrivacyIndicator, create_dissector_handle(dissect_PrivacyIndicator_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_VoiceSupportMatchIndicator, create_dissector_handle(dissect_VoiceSupportMatchIndicator_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_GUMMEIType, create_dissector_handle(dissect_GUMMEIType_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_Tunnel_Information_for_BBF, create_dissector_handle(dissect_TunnelInformation_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_ManagementBasedMDTPLMNList, create_dissector_handle(dissect_MDTPLMNList_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_ECGIListForRestart, create_dissector_handle(dissect_ECGIListForRestart_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_SIPTO_L_GW_TransportLayerAddress, create_dissector_handle(dissect_TransportLayerAddress_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_LHN_ID, create_dissector_handle(dissect_LHN_ID_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_AdditionalCSFallbackIndicator, create_dissector_handle(dissect_AdditionalCSFallbackIndicator_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_TAIListForRestart, create_dissector_handle(dissect_TAIListForRestart_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_UserLocationInformation, create_dissector_handle(dissect_UserLocationInformation_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_KillAllWarningMessages, create_dissector_handle(dissect_KillAllWarningMessages_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_Masked_IMEISV, create_dissector_handle(dissect_Masked_IMEISV_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_ProSeAuthorized, create_dissector_handle(dissect_ProSeAuthorized_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_ExpectedUEBehaviour, create_dissector_handle(dissect_ExpectedUEBehaviour_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_UERadioCapabilityForPaging, create_dissector_handle(dissect_UERadioCapabilityForPaging_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABToBeModifiedListBearerModInd, create_dissector_handle(dissect_E_RABToBeModifiedListBearerModInd_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABToBeModifiedItemBearerModInd, create_dissector_handle(dissect_E_RABToBeModifiedItemBearerModInd_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABNotToBeModifiedListBearerModInd, create_dissector_handle(dissect_E_RABNotToBeModifiedListBearerModInd_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABNotToBeModifiedItemBearerModInd, create_dissector_handle(dissect_E_RABNotToBeModifiedItemBearerModInd_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABModifyListBearerModConf, create_dissector_handle(dissect_E_RABModifyListBearerModConf_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABModifyItemBearerModConf, create_dissector_handle(dissect_E_RABModifyItemBearerModConf_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABFailedToModifyListBearerModConf, create_dissector_handle(dissect_E_RABList_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_SON_Information_Report, create_dissector_handle(dissect_SONInformationReport_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_E_RABToBeReleasedListBearerModConf, create_dissector_handle(dissect_E_RABList_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_PWSfailedECGIList, create_dissector_handle(dissect_PWSfailedECGIList_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_MME_Group_ID, create_dissector_handle(dissect_MME_Group_ID_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_Additional_GUTI, create_dissector_handle(dissect_Additional_GUTI_PDU, proto_s1ap));
  dissector_add_uint("s1ap.ies", id_CSGMembershipInfo, create_dissector_handle(dissect_CSGMembershipInfo_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_Data_Forwarding_Not_Possible, create_dissector_handle(dissect_Data_Forwarding_Not_Possible_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_Time_Synchronisation_Info, create_dissector_handle(dissect_TimeSynchronisationInfo_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_x2TNLConfigurationInfo, create_dissector_handle(dissect_X2TNLConfigurationInfo_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_eNBX2ExtendedTransportLayerAddresses, create_dissector_handle(dissect_ENBX2ExtTLAs_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_Correlation_ID, create_dissector_handle(dissect_Correlation_ID_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_MDTConfiguration, create_dissector_handle(dissect_MDT_Configuration_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_Time_UE_StayedInCell_EnhancedGranularity, create_dissector_handle(dissect_Time_UE_StayedInCell_EnhancedGranularity_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_HO_Cause, create_dissector_handle(dissect_Cause_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_M3Configuration, create_dissector_handle(dissect_M3Configuration_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_M4Configuration, create_dissector_handle(dissect_M4Configuration_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_M5Configuration, create_dissector_handle(dissect_M5Configuration_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_MDT_Location_Info, create_dissector_handle(dissect_MDT_Location_Info_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_MobilityInformation, create_dissector_handle(dissect_MobilityInformation_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_SignallingBasedMDTPLMNList, create_dissector_handle(dissect_MDTPLMNList_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_ULCOUNTValueExtended, create_dissector_handle(dissect_COUNTValueExtended_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_DLCOUNTValueExtended, create_dissector_handle(dissect_COUNTValueExtended_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_ReceiveStatusOfULPDCPSDUsExtended, create_dissector_handle(dissect_ReceiveStatusOfULPDCPSDUsExtended_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_SIPTO_Correlation_ID, create_dissector_handle(dissect_Correlation_ID_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_TransportInformation, create_dissector_handle(dissect_TransportInformation_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_EmergencyAreaIDListForRestart, create_dissector_handle(dissect_EmergencyAreaIDListForRestart_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_eNBIndirectX2TransportLayerAddresses, create_dissector_handle(dissect_ENBIndirectX2TransportLayerAddresses_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_LoggedMBSFNMDT, create_dissector_handle(dissect_LoggedMBSFNMDT_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_uE_HistoryInformationFromTheUE, create_dissector_handle(dissect_UE_HistoryInformationFromTheUE_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_Muting_Availability_Indication, create_dissector_handle(dissect_MutingAvailabilityIndication_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_Muting_Pattern_Information, create_dissector_handle(dissect_MutingPatternInformation_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_Synchronisation_Information, create_dissector_handle(dissect_SynchronisationInformation_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_ProSeUEtoNetworkRelaying, create_dissector_handle(dissect_ProSeUEtoNetworkRelaying_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_ULCOUNTValuePDCP_SNlength18, create_dissector_handle(dissect_COUNTvaluePDCP_SNlength18_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_DLCOUNTValuePDCP_SNlength18, create_dissector_handle(dissect_COUNTvaluePDCP_SNlength18_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_ReceiveStatusOfULPDCPSDUsPDCP_SNlength18, create_dissector_handle(dissect_ReceiveStatusOfULPDCPSDUsPDCP_SNlength18_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_M6Configuration, create_dissector_handle(dissect_M6Configuration_PDU, proto_s1ap));
  dissector_add_uint("s1ap.extension", id_M7Configuration, create_dissector_handle(dissect_M7Configuration_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_HandoverPreparation, create_dissector_handle(dissect_HandoverRequired_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.sout", id_HandoverPreparation, create_dissector_handle(dissect_HandoverCommand_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.uout", id_HandoverPreparation, create_dissector_handle(dissect_HandoverPreparationFailure_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_HandoverResourceAllocation, create_dissector_handle(dissect_HandoverRequest_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.sout", id_HandoverResourceAllocation, create_dissector_handle(dissect_HandoverRequestAcknowledge_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.uout", id_HandoverResourceAllocation, create_dissector_handle(dissect_HandoverFailure_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_HandoverNotification, create_dissector_handle(dissect_HandoverNotify_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_PathSwitchRequest, create_dissector_handle(dissect_PathSwitchRequest_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.sout", id_PathSwitchRequest, create_dissector_handle(dissect_PathSwitchRequestAcknowledge_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.uout", id_PathSwitchRequest, create_dissector_handle(dissect_PathSwitchRequestFailure_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_E_RABSetup, create_dissector_handle(dissect_E_RABSetupRequest_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.sout", id_E_RABSetup, create_dissector_handle(dissect_E_RABSetupResponse_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_E_RABModify, create_dissector_handle(dissect_E_RABModifyRequest_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.sout", id_E_RABModify, create_dissector_handle(dissect_E_RABModifyResponse_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_E_RABRelease, create_dissector_handle(dissect_E_RABReleaseCommand_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.sout", id_E_RABRelease, create_dissector_handle(dissect_E_RABReleaseResponse_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_E_RABReleaseIndication, create_dissector_handle(dissect_E_RABReleaseIndication_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_InitialContextSetup, create_dissector_handle(dissect_InitialContextSetupRequest_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.sout", id_InitialContextSetup, create_dissector_handle(dissect_InitialContextSetupResponse_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.uout", id_InitialContextSetup, create_dissector_handle(dissect_InitialContextSetupFailure_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_UEContextReleaseRequest, create_dissector_handle(dissect_UEContextReleaseRequest_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_Paging, create_dissector_handle(dissect_Paging_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_downlinkNASTransport, create_dissector_handle(dissect_DownlinkNASTransport_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_initialUEMessage, create_dissector_handle(dissect_InitialUEMessage_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_uplinkNASTransport, create_dissector_handle(dissect_UplinkNASTransport_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_NASNonDeliveryIndication, create_dissector_handle(dissect_NASNonDeliveryIndication_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_HandoverCancel, create_dissector_handle(dissect_HandoverCancel_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.sout", id_HandoverCancel, create_dissector_handle(dissect_HandoverCancelAcknowledge_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_Reset, create_dissector_handle(dissect_Reset_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.sout", id_Reset, create_dissector_handle(dissect_ResetAcknowledge_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_ErrorIndication, create_dissector_handle(dissect_ErrorIndication_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_S1Setup, create_dissector_handle(dissect_S1SetupRequest_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.sout", id_S1Setup, create_dissector_handle(dissect_S1SetupResponse_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.uout", id_S1Setup, create_dissector_handle(dissect_S1SetupFailure_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_DownlinkS1cdma2000tunnelling, create_dissector_handle(dissect_DownlinkS1cdma2000tunnelling_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_ENBConfigurationUpdate, create_dissector_handle(dissect_ENBConfigurationUpdate_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.sout", id_ENBConfigurationUpdate, create_dissector_handle(dissect_ENBConfigurationUpdateAcknowledge_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.uout", id_ENBConfigurationUpdate, create_dissector_handle(dissect_ENBConfigurationUpdateFailure_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_MMEConfigurationUpdate, create_dissector_handle(dissect_MMEConfigurationUpdate_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.sout", id_MMEConfigurationUpdate, create_dissector_handle(dissect_MMEConfigurationUpdateAcknowledge_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.uout", id_MMEConfigurationUpdate, create_dissector_handle(dissect_MMEConfigurationUpdateFailure_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_UplinkS1cdma2000tunnelling, create_dissector_handle(dissect_UplinkS1cdma2000tunnelling_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_UEContextModification, create_dissector_handle(dissect_UEContextModificationRequest_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.sout", id_UEContextModification, create_dissector_handle(dissect_UEContextModificationResponse_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.uout", id_UEContextModification, create_dissector_handle(dissect_UEContextModificationFailure_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_UECapabilityInfoIndication, create_dissector_handle(dissect_UECapabilityInfoIndication_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_UEContextRelease, create_dissector_handle(dissect_UEContextReleaseCommand_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.sout", id_UEContextRelease, create_dissector_handle(dissect_UEContextReleaseComplete_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_eNBStatusTransfer, create_dissector_handle(dissect_ENBStatusTransfer_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_MMEStatusTransfer, create_dissector_handle(dissect_MMEStatusTransfer_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_DeactivateTrace, create_dissector_handle(dissect_DeactivateTrace_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_TraceStart, create_dissector_handle(dissect_TraceStart_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_TraceFailureIndication, create_dissector_handle(dissect_TraceFailureIndication_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_LocationReportingControl, create_dissector_handle(dissect_LocationReportingControl_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_LocationReportingFailureIndication, create_dissector_handle(dissect_LocationReportingFailureIndication_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_LocationReport, create_dissector_handle(dissect_LocationReport_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_OverloadStart, create_dissector_handle(dissect_OverloadStart_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_OverloadStop, create_dissector_handle(dissect_OverloadStop_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_WriteReplaceWarning, create_dissector_handle(dissect_WriteReplaceWarningRequest_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.sout", id_WriteReplaceWarning, create_dissector_handle(dissect_WriteReplaceWarningResponse_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_eNBDirectInformationTransfer, create_dissector_handle(dissect_ENBDirectInformationTransfer_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_MMEDirectInformationTransfer, create_dissector_handle(dissect_MMEDirectInformationTransfer_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_eNBConfigurationTransfer, create_dissector_handle(dissect_ENBConfigurationTransfer_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_MMEConfigurationTransfer, create_dissector_handle(dissect_MMEConfigurationTransfer_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_CellTrafficTrace, create_dissector_handle(dissect_CellTrafficTrace_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_PrivateMessage, create_dissector_handle(dissect_PrivateMessage_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_PWSRestartIndication, create_dissector_handle(dissect_PWSRestartIndication_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_Kill, create_dissector_handle(dissect_KillRequest_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.sout", id_Kill, create_dissector_handle(dissect_KillResponse_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_downlinkUEAssociatedLPPaTransport, create_dissector_handle(dissect_DownlinkUEAssociatedLPPaTransport_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_uplinkUEAssociatedLPPaTransport, create_dissector_handle(dissect_UplinkUEAssociatedLPPaTransport_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_downlinkNonUEAssociatedLPPaTransport, create_dissector_handle(dissect_DownlinkNonUEAssociatedLPPaTransport_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_uplinkNonUEAssociatedLPPaTransport, create_dissector_handle(dissect_UplinkNonUEAssociatedLPPaTransport_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_UERadioCapabilityMatch, create_dissector_handle(dissect_UERadioCapabilityMatchRequest_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.sout", id_UERadioCapabilityMatch, create_dissector_handle(dissect_UERadioCapabilityMatchResponse_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_E_RABModificationIndication, create_dissector_handle(dissect_E_RABModificationIndication_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.sout", id_E_RABModificationIndication, create_dissector_handle(dissect_E_RABModificationConfirm_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_UEContextModificationIndication, create_dissector_handle(dissect_UEContextModificationIndication_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.sout", id_UEContextModificationIndication, create_dissector_handle(dissect_UEContextModificationConfirm_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_RerouteNASRequest, create_dissector_handle(dissect_RerouteNASRequest_PDU, proto_s1ap));
  dissector_add_uint("s1ap.proc.imsg", id_PWSFailureIndication, create_dissector_handle(dissect_PWSFailureIndication_PDU, proto_s1ap));


/*--- End of included file: packet-s1ap-dis-tab.c ---*/
#line 224 "./asn1/s1ap/packet-s1ap-template.c"
	} else {
		if (SctpPort != 0) {
			dissector_delete_uint("sctp.port", SctpPort, s1ap_handle);
		}
	}

	SctpPort=gbl_s1apSctpPort;
	if (SctpPort != 0) {
		dissector_add_uint("sctp.port", SctpPort, s1ap_handle);
	}
}

/*--- proto_register_s1ap -------------------------------------------*/
void proto_register_s1ap(void) {

  /* List of fields */

  static hf_register_info hf[] = {
    { &hf_s1ap_transportLayerAddressIPv4,
      { "transportLayerAddress(IPv4)", "s1ap.transportLayerAddressIPv4",
        FT_IPv4, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_transportLayerAddressIPv6,
      { "transportLayerAddress(IPv6)", "s1ap.transportLayerAddressIPv6",
        FT_IPv6, BASE_NONE, NULL, 0,
        NULL, HFILL }},


/*--- Included file: packet-s1ap-hfarr.c ---*/
#line 1 "./asn1/s1ap/packet-s1ap-hfarr.c"
    { &hf_s1ap_Additional_GUTI_PDU,
      { "Additional-GUTI", "s1ap.Additional_GUTI_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_Bearers_SubjectToStatusTransfer_Item_PDU,
      { "Bearers-SubjectToStatusTransfer-Item", "s1ap.Bearers_SubjectToStatusTransfer_Item_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_BroadcastCancelledAreaList_PDU,
      { "BroadcastCancelledAreaList", "s1ap.BroadcastCancelledAreaList",
        FT_UINT32, BASE_DEC, VALS(s1ap_BroadcastCancelledAreaList_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_BroadcastCompletedAreaList_PDU,
      { "BroadcastCompletedAreaList", "s1ap.BroadcastCompletedAreaList",
        FT_UINT32, BASE_DEC, VALS(s1ap_BroadcastCompletedAreaList_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_Cause_PDU,
      { "Cause", "s1ap.Cause",
        FT_UINT32, BASE_DEC, VALS(s1ap_Cause_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_CellAccessMode_PDU,
      { "CellAccessMode", "s1ap.CellAccessMode",
        FT_UINT32, BASE_DEC, VALS(s1ap_CellAccessMode_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_Cdma2000PDU_PDU,
      { "Cdma2000PDU", "s1ap.Cdma2000PDU",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_Cdma2000RATType_PDU,
      { "Cdma2000RATType", "s1ap.Cdma2000RATType",
        FT_UINT32, BASE_DEC, VALS(s1ap_Cdma2000RATType_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_Cdma2000SectorID_PDU,
      { "Cdma2000SectorID", "s1ap.Cdma2000SectorID",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_Cdma2000HOStatus_PDU,
      { "Cdma2000HOStatus", "s1ap.Cdma2000HOStatus",
        FT_UINT32, BASE_DEC, VALS(s1ap_Cdma2000HOStatus_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_Cdma2000HORequiredIndication_PDU,
      { "Cdma2000HORequiredIndication", "s1ap.Cdma2000HORequiredIndication",
        FT_UINT32, BASE_DEC, VALS(s1ap_Cdma2000HORequiredIndication_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_Cdma2000OneXSRVCCInfo_PDU,
      { "Cdma2000OneXSRVCCInfo", "s1ap.Cdma2000OneXSRVCCInfo_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_Cdma2000OneXRAND_PDU,
      { "Cdma2000OneXRAND", "s1ap.Cdma2000OneXRAND",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_CNDomain_PDU,
      { "CNDomain", "s1ap.CNDomain",
        FT_UINT32, BASE_DEC, VALS(s1ap_CNDomain_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_ConcurrentWarningMessageIndicator_PDU,
      { "ConcurrentWarningMessageIndicator", "s1ap.ConcurrentWarningMessageIndicator",
        FT_UINT32, BASE_DEC, VALS(s1ap_ConcurrentWarningMessageIndicator_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_Correlation_ID_PDU,
      { "Correlation-ID", "s1ap.Correlation_ID",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_CSFallbackIndicator_PDU,
      { "CSFallbackIndicator", "s1ap.CSFallbackIndicator",
        FT_UINT32, BASE_DEC, VALS(s1ap_CSFallbackIndicator_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_AdditionalCSFallbackIndicator_PDU,
      { "AdditionalCSFallbackIndicator", "s1ap.AdditionalCSFallbackIndicator",
        FT_UINT32, BASE_DEC, VALS(s1ap_AdditionalCSFallbackIndicator_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_CSG_Id_PDU,
      { "CSG-Id", "s1ap.CSG_Id",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_CSG_IdList_PDU,
      { "CSG-IdList", "s1ap.CSG_IdList",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_CSGMembershipStatus_PDU,
      { "CSGMembershipStatus", "s1ap.CSGMembershipStatus",
        FT_UINT32, BASE_DEC, VALS(s1ap_CSGMembershipStatus_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_COUNTValueExtended_PDU,
      { "COUNTValueExtended", "s1ap.COUNTValueExtended_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_COUNTvaluePDCP_SNlength18_PDU,
      { "COUNTvaluePDCP-SNlength18", "s1ap.COUNTvaluePDCP_SNlength18_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_CriticalityDiagnostics_PDU,
      { "CriticalityDiagnostics", "s1ap.CriticalityDiagnostics_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_DataCodingScheme_PDU,
      { "DataCodingScheme", "s1ap.DataCodingScheme",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_Direct_Forwarding_Path_Availability_PDU,
      { "Direct-Forwarding-Path-Availability", "s1ap.Direct_Forwarding_Path_Availability",
        FT_UINT32, BASE_DEC, VALS(s1ap_Direct_Forwarding_Path_Availability_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_Data_Forwarding_Not_Possible_PDU,
      { "Data-Forwarding-Not-Possible", "s1ap.Data_Forwarding_Not_Possible",
        FT_UINT32, BASE_DEC, VALS(s1ap_Data_Forwarding_Not_Possible_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_PWSfailedECGIList_PDU,
      { "PWSfailedECGIList", "s1ap.PWSfailedECGIList",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_EmergencyAreaIDListForRestart_PDU,
      { "EmergencyAreaIDListForRestart", "s1ap.EmergencyAreaIDListForRestart",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_s1ap_Global_ENB_ID_PDU,
      { "Global-ENB-ID", "s1ap.Global_ENB_ID_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_GUMMEIList_PDU,
      { "GUMMEIList", "s1ap.GUMMEIList",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_s1ap_ENB_StatusTransfer_TransparentContainer_PDU,
      { "ENB-StatusTransfer-TransparentContainer", "s1ap.ENB_StatusTransfer_TransparentContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ENB_UE_S1AP_ID_PDU,
      { "ENB-UE-S1AP-ID", "s1ap.ENB_UE_S1AP_ID",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ENBname_PDU,
      { "ENBname", "s1ap.ENBname",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABInformationListItem_PDU,
      { "E-RABInformationListItem", "s1ap.E_RABInformationListItem_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABList_PDU,
      { "E-RABList", "s1ap.E_RABList",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABItem_PDU,
      { "E-RABItem", "s1ap.E_RABItem_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_EUTRAN_CGI_PDU,
      { "EUTRAN-CGI", "s1ap.EUTRAN_CGI_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_EUTRANRoundTripDelayEstimationInfo_PDU,
      { "EUTRANRoundTripDelayEstimationInfo", "s1ap.EUTRANRoundTripDelayEstimationInfo",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ExpectedUEBehaviour_PDU,
      { "ExpectedUEBehaviour", "s1ap.ExpectedUEBehaviour_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ExtendedRepetitionPeriod_PDU,
      { "ExtendedRepetitionPeriod", "s1ap.ExtendedRepetitionPeriod",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_GUMMEI_PDU,
      { "GUMMEI", "s1ap.GUMMEI_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_GUMMEIType_PDU,
      { "GUMMEIType", "s1ap.GUMMEIType",
        FT_UINT32, BASE_DEC, VALS(s1ap_GUMMEIType_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_GWContextReleaseIndication_PDU,
      { "GWContextReleaseIndication", "s1ap.GWContextReleaseIndication",
        FT_UINT32, BASE_DEC, VALS(s1ap_GWContextReleaseIndication_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_HandoverRestrictionList_PDU,
      { "HandoverRestrictionList", "s1ap.HandoverRestrictionList_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_HandoverType_PDU,
      { "HandoverType", "s1ap.HandoverType",
        FT_UINT32, BASE_DEC, VALS(s1ap_HandoverType_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_Masked_IMEISV_PDU,
      { "Masked-IMEISV", "s1ap.Masked_IMEISV",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_KillAllWarningMessages_PDU,
      { "KillAllWarningMessages", "s1ap.KillAllWarningMessages",
        FT_UINT32, BASE_DEC, VALS(s1ap_KillAllWarningMessages_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_LAI_PDU,
      { "LAI", "s1ap.LAI_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_LPPa_PDU_PDU,
      { "LPPa-PDU", "s1ap.LPPa_PDU",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_LHN_ID_PDU,
      { "LHN-ID", "s1ap.LHN_ID",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_LoggedMBSFNMDT_PDU,
      { "LoggedMBSFNMDT", "s1ap.LoggedMBSFNMDT_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_M3Configuration_PDU,
      { "M3Configuration", "s1ap.M3Configuration_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_M4Configuration_PDU,
      { "M4Configuration", "s1ap.M4Configuration_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_M5Configuration_PDU,
      { "M5Configuration", "s1ap.M5Configuration_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_M6Configuration_PDU,
      { "M6Configuration", "s1ap.M6Configuration_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_M7Configuration_PDU,
      { "M7Configuration", "s1ap.M7Configuration_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_MDT_Location_Info_PDU,
      { "MDT-Location-Info", "s1ap.MDT_Location_Info",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_MDT_Configuration_PDU,
      { "MDT-Configuration", "s1ap.MDT_Configuration_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ManagementBasedMDTAllowed_PDU,
      { "ManagementBasedMDTAllowed", "s1ap.ManagementBasedMDTAllowed",
        FT_UINT32, BASE_DEC, VALS(s1ap_ManagementBasedMDTAllowed_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_MDTPLMNList_PDU,
      { "MDTPLMNList", "s1ap.MDTPLMNList",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_PrivacyIndicator_PDU,
      { "PrivacyIndicator", "s1ap.PrivacyIndicator",
        FT_UINT32, BASE_DEC, VALS(s1ap_PrivacyIndicator_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_MessageIdentifier_PDU,
      { "MessageIdentifier", "s1ap.MessageIdentifier",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_MobilityInformation_PDU,
      { "MobilityInformation", "s1ap.MobilityInformation",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_MMEname_PDU,
      { "MMEname", "s1ap.MMEname",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_MMERelaySupportIndicator_PDU,
      { "MMERelaySupportIndicator", "s1ap.MMERelaySupportIndicator",
        FT_UINT32, BASE_DEC, VALS(s1ap_MMERelaySupportIndicator_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_MME_Group_ID_PDU,
      { "MME-Group-ID", "s1ap.MME_Group_ID",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_MME_UE_S1AP_ID_PDU,
      { "MME-UE-S1AP-ID", "s1ap.MME_UE_S1AP_ID",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_MSClassmark2_PDU,
      { "MSClassmark2", "s1ap.MSClassmark2",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_MSClassmark3_PDU,
      { "MSClassmark3", "s1ap.MSClassmark3",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_MutingAvailabilityIndication_PDU,
      { "MutingAvailabilityIndication", "s1ap.MutingAvailabilityIndication",
        FT_UINT32, BASE_DEC, VALS(s1ap_MutingAvailabilityIndication_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_MutingPatternInformation_PDU,
      { "MutingPatternInformation", "s1ap.MutingPatternInformation_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_NAS_PDU_PDU,
      { "NAS-PDU", "s1ap.NAS_PDU",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_NASSecurityParametersfromE_UTRAN_PDU,
      { "NASSecurityParametersfromE-UTRAN", "s1ap.NASSecurityParametersfromE_UTRAN",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_NASSecurityParameterstoE_UTRAN_PDU,
      { "NASSecurityParameterstoE-UTRAN", "s1ap.NASSecurityParameterstoE_UTRAN",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_NumberofBroadcastRequest_PDU,
      { "NumberofBroadcastRequest", "s1ap.NumberofBroadcastRequest",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_OverloadResponse_PDU,
      { "OverloadResponse", "s1ap.OverloadResponse",
        FT_UINT32, BASE_DEC, VALS(s1ap_OverloadResponse_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_PagingDRX_PDU,
      { "PagingDRX", "s1ap.PagingDRX",
        FT_UINT32, BASE_DEC, VALS(s1ap_PagingDRX_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_PagingPriority_PDU,
      { "PagingPriority", "s1ap.PagingPriority",
        FT_UINT32, BASE_DEC, VALS(s1ap_PagingPriority_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_ProSeAuthorized_PDU,
      { "ProSeAuthorized", "s1ap.ProSeAuthorized_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ProSeUEtoNetworkRelaying_PDU,
      { "ProSeUEtoNetworkRelaying", "s1ap.ProSeUEtoNetworkRelaying",
        FT_UINT32, BASE_DEC, VALS(s1ap_ProSeUEtoNetworkRelaying_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_PS_ServiceNotAvailable_PDU,
      { "PS-ServiceNotAvailable", "s1ap.PS_ServiceNotAvailable",
        FT_UINT32, BASE_DEC, VALS(s1ap_PS_ServiceNotAvailable_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_ReceiveStatusOfULPDCPSDUsExtended_PDU,
      { "ReceiveStatusOfULPDCPSDUsExtended", "s1ap.ReceiveStatusOfULPDCPSDUsExtended",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ReceiveStatusOfULPDCPSDUsPDCP_SNlength18_PDU,
      { "ReceiveStatusOfULPDCPSDUsPDCP-SNlength18", "s1ap.ReceiveStatusOfULPDCPSDUsPDCP_SNlength18",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_RelativeMMECapacity_PDU,
      { "RelativeMMECapacity", "s1ap.RelativeMMECapacity",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_RelayNode_Indicator_PDU,
      { "RelayNode-Indicator", "s1ap.RelayNode_Indicator",
        FT_UINT32, BASE_DEC, VALS(s1ap_RelayNode_Indicator_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_RequestType_PDU,
      { "RequestType", "s1ap.RequestType_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_RepetitionPeriod_PDU,
      { "RepetitionPeriod", "s1ap.RepetitionPeriod",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_RRC_Establishment_Cause_PDU,
      { "RRC-Establishment-Cause", "s1ap.RRC_Establishment_Cause",
        FT_UINT32, BASE_DEC, VALS(s1ap_RRC_Establishment_Cause_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_ECGIListForRestart_PDU,
      { "ECGIListForRestart", "s1ap.ECGIListForRestart",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_Routing_ID_PDU,
      { "Routing-ID", "s1ap.Routing_ID",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_SecurityKey_PDU,
      { "SecurityKey", "s1ap.SecurityKey",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_SecurityContext_PDU,
      { "SecurityContext", "s1ap.SecurityContext_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_SerialNumber_PDU,
      { "SerialNumber", "s1ap.SerialNumber",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_SONInformationReport_PDU,
      { "SONInformationReport", "s1ap.SONInformationReport",
        FT_UINT32, BASE_DEC, VALS(s1ap_SONInformationReport_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_SONConfigurationTransfer_PDU,
      { "SONConfigurationTransfer", "s1ap.SONConfigurationTransfer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_SynchronisationInformation_PDU,
      { "SynchronisationInformation", "s1ap.SynchronisationInformation_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_Source_ToTarget_TransparentContainer_PDU,
      { "Source-ToTarget-TransparentContainer", "s1ap.Source_ToTarget_TransparentContainer",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_SRVCCOperationPossible_PDU,
      { "SRVCCOperationPossible", "s1ap.SRVCCOperationPossible",
        FT_UINT32, BASE_DEC, VALS(s1ap_SRVCCOperationPossible_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_SRVCCHOIndication_PDU,
      { "SRVCCHOIndication", "s1ap.SRVCCHOIndication",
        FT_UINT32, BASE_DEC, VALS(s1ap_SRVCCHOIndication_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_SourceeNB_ToTargeteNB_TransparentContainer_PDU,
      { "SourceeNB-ToTargeteNB-TransparentContainer", "s1ap.SourceeNB_ToTargeteNB_TransparentContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ServedGUMMEIs_PDU,
      { "ServedGUMMEIs", "s1ap.ServedGUMMEIs",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ServedPLMNs_PDU,
      { "ServedPLMNs", "s1ap.ServedPLMNs",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_SubscriberProfileIDforRFP_PDU,
      { "SubscriberProfileIDforRFP", "s1ap.SubscriberProfileIDforRFP",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_SupportedTAs_PDU,
      { "SupportedTAs", "s1ap.SupportedTAs",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_TimeSynchronisationInfo_PDU,
      { "TimeSynchronisationInfo", "s1ap.TimeSynchronisationInfo_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_S_TMSI_PDU,
      { "S-TMSI", "s1ap.S_TMSI_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_TAI_PDU,
      { "TAI", "s1ap.TAI_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_TargetID_PDU,
      { "TargetID", "s1ap.TargetID",
        FT_UINT32, BASE_DEC, VALS(s1ap_TargetID_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_TargeteNB_ToSourceeNB_TransparentContainer_PDU,
      { "TargeteNB-ToSourceeNB-TransparentContainer", "s1ap.TargeteNB_ToSourceeNB_TransparentContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_Target_ToSource_TransparentContainer_PDU,
      { "Target-ToSource-TransparentContainer", "s1ap.Target_ToSource_TransparentContainer",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_TimeToWait_PDU,
      { "TimeToWait", "s1ap.TimeToWait",
        FT_UINT32, BASE_DEC, VALS(s1ap_TimeToWait_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_Time_UE_StayedInCell_EnhancedGranularity_PDU,
      { "Time-UE-StayedInCell-EnhancedGranularity", "s1ap.Time_UE_StayedInCell_EnhancedGranularity",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_TransportInformation_PDU,
      { "TransportInformation", "s1ap.TransportInformation_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_TransportLayerAddress_PDU,
      { "TransportLayerAddress", "s1ap.TransportLayerAddress",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_TraceActivation_PDU,
      { "TraceActivation", "s1ap.TraceActivation_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_UTRAN_Trace_ID_PDU,
      { "E-UTRAN-Trace-ID", "s1ap.E_UTRAN_Trace_ID",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_TrafficLoadReductionIndication_PDU,
      { "TrafficLoadReductionIndication", "s1ap.TrafficLoadReductionIndication",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_TunnelInformation_PDU,
      { "TunnelInformation", "s1ap.TunnelInformation_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_TAIListForRestart_PDU,
      { "TAIListForRestart", "s1ap.TAIListForRestart",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UEAggregateMaximumBitrate_PDU,
      { "UEAggregateMaximumBitrate", "s1ap.UEAggregateMaximumBitrate_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UE_S1AP_IDs_PDU,
      { "UE-S1AP-IDs", "s1ap.UE_S1AP_IDs",
        FT_UINT32, BASE_DEC, VALS(s1ap_UE_S1AP_IDs_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_UE_associatedLogicalS1_ConnectionItem_PDU,
      { "UE-associatedLogicalS1-ConnectionItem", "s1ap.UE_associatedLogicalS1_ConnectionItem_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UEIdentityIndexValue_PDU,
      { "UEIdentityIndexValue", "s1ap.UEIdentityIndexValue",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_s1ap_UE_HistoryInformation_PDU,
      { "UE-HistoryInformation", "s1ap.UE_HistoryInformation",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UE_HistoryInformationFromTheUE_PDU,
      { "UE-HistoryInformationFromTheUE", "s1ap.UE_HistoryInformationFromTheUE",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UEPagingID_PDU,
      { "UEPagingID", "s1ap.UEPagingID",
        FT_UINT32, BASE_DEC, VALS(s1ap_UEPagingID_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_UERadioCapability_PDU,
      { "UERadioCapability", "s1ap.UERadioCapability",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UERadioCapabilityForPaging_PDU,
      { "UERadioCapabilityForPaging", "s1ap.UERadioCapabilityForPaging",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UESecurityCapabilities_PDU,
      { "UESecurityCapabilities", "s1ap.UESecurityCapabilities_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UserLocationInformation_PDU,
      { "UserLocationInformation", "s1ap.UserLocationInformation_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_VoiceSupportMatchIndicator_PDU,
      { "VoiceSupportMatchIndicator", "s1ap.VoiceSupportMatchIndicator",
        FT_UINT32, BASE_DEC, VALS(s1ap_VoiceSupportMatchIndicator_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_WarningAreaList_PDU,
      { "WarningAreaList", "s1ap.WarningAreaList",
        FT_UINT32, BASE_DEC, VALS(s1ap_WarningAreaList_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_WarningType_PDU,
      { "WarningType", "s1ap.WarningType",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_WarningSecurityInfo_PDU,
      { "WarningSecurityInfo", "s1ap.WarningSecurityInfo",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_WarningMessageContents_PDU,
      { "WarningMessageContents", "s1ap.WarningMessageContents",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_X2TNLConfigurationInfo_PDU,
      { "X2TNLConfigurationInfo", "s1ap.X2TNLConfigurationInfo_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ENBX2ExtTLAs_PDU,
      { "ENBX2ExtTLAs", "s1ap.ENBX2ExtTLAs",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ENBIndirectX2TransportLayerAddresses_PDU,
      { "ENBIndirectX2TransportLayerAddresses", "s1ap.ENBIndirectX2TransportLayerAddresses",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_HandoverRequired_PDU,
      { "HandoverRequired", "s1ap.HandoverRequired_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_HandoverCommand_PDU,
      { "HandoverCommand", "s1ap.HandoverCommand_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABSubjecttoDataForwardingList_PDU,
      { "E-RABSubjecttoDataForwardingList", "s1ap.E_RABSubjecttoDataForwardingList",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABDataForwardingItem_PDU,
      { "E-RABDataForwardingItem", "s1ap.E_RABDataForwardingItem_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_HandoverPreparationFailure_PDU,
      { "HandoverPreparationFailure", "s1ap.HandoverPreparationFailure_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_HandoverRequest_PDU,
      { "HandoverRequest", "s1ap.HandoverRequest_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABToBeSetupListHOReq_PDU,
      { "E-RABToBeSetupListHOReq", "s1ap.E_RABToBeSetupListHOReq",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABToBeSetupItemHOReq_PDU,
      { "E-RABToBeSetupItemHOReq", "s1ap.E_RABToBeSetupItemHOReq_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_HandoverRequestAcknowledge_PDU,
      { "HandoverRequestAcknowledge", "s1ap.HandoverRequestAcknowledge_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABAdmittedList_PDU,
      { "E-RABAdmittedList", "s1ap.E_RABAdmittedList",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABAdmittedItem_PDU,
      { "E-RABAdmittedItem", "s1ap.E_RABAdmittedItem_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABFailedtoSetupListHOReqAck_PDU,
      { "E-RABFailedtoSetupListHOReqAck", "s1ap.E_RABFailedtoSetupListHOReqAck",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABFailedToSetupItemHOReqAck_PDU,
      { "E-RABFailedToSetupItemHOReqAck", "s1ap.E_RABFailedToSetupItemHOReqAck_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_HandoverFailure_PDU,
      { "HandoverFailure", "s1ap.HandoverFailure_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_HandoverNotify_PDU,
      { "HandoverNotify", "s1ap.HandoverNotify_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_PathSwitchRequest_PDU,
      { "PathSwitchRequest", "s1ap.PathSwitchRequest_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABToBeSwitchedDLList_PDU,
      { "E-RABToBeSwitchedDLList", "s1ap.E_RABToBeSwitchedDLList",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABToBeSwitchedDLItem_PDU,
      { "E-RABToBeSwitchedDLItem", "s1ap.E_RABToBeSwitchedDLItem_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_PathSwitchRequestAcknowledge_PDU,
      { "PathSwitchRequestAcknowledge", "s1ap.PathSwitchRequestAcknowledge_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABToBeSwitchedULList_PDU,
      { "E-RABToBeSwitchedULList", "s1ap.E_RABToBeSwitchedULList",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABToBeSwitchedULItem_PDU,
      { "E-RABToBeSwitchedULItem", "s1ap.E_RABToBeSwitchedULItem_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_PathSwitchRequestFailure_PDU,
      { "PathSwitchRequestFailure", "s1ap.PathSwitchRequestFailure_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_HandoverCancel_PDU,
      { "HandoverCancel", "s1ap.HandoverCancel_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_HandoverCancelAcknowledge_PDU,
      { "HandoverCancelAcknowledge", "s1ap.HandoverCancelAcknowledge_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABSetupRequest_PDU,
      { "E-RABSetupRequest", "s1ap.E_RABSetupRequest_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABToBeSetupListBearerSUReq_PDU,
      { "E-RABToBeSetupListBearerSUReq", "s1ap.E_RABToBeSetupListBearerSUReq",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABToBeSetupItemBearerSUReq_PDU,
      { "E-RABToBeSetupItemBearerSUReq", "s1ap.E_RABToBeSetupItemBearerSUReq_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABSetupResponse_PDU,
      { "E-RABSetupResponse", "s1ap.E_RABSetupResponse_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABSetupListBearerSURes_PDU,
      { "E-RABSetupListBearerSURes", "s1ap.E_RABSetupListBearerSURes",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABSetupItemBearerSURes_PDU,
      { "E-RABSetupItemBearerSURes", "s1ap.E_RABSetupItemBearerSURes_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABModifyRequest_PDU,
      { "E-RABModifyRequest", "s1ap.E_RABModifyRequest_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABToBeModifiedListBearerModReq_PDU,
      { "E-RABToBeModifiedListBearerModReq", "s1ap.E_RABToBeModifiedListBearerModReq",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABToBeModifiedItemBearerModReq_PDU,
      { "E-RABToBeModifiedItemBearerModReq", "s1ap.E_RABToBeModifiedItemBearerModReq_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABModifyResponse_PDU,
      { "E-RABModifyResponse", "s1ap.E_RABModifyResponse_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABModifyListBearerModRes_PDU,
      { "E-RABModifyListBearerModRes", "s1ap.E_RABModifyListBearerModRes",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABModifyItemBearerModRes_PDU,
      { "E-RABModifyItemBearerModRes", "s1ap.E_RABModifyItemBearerModRes_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABReleaseCommand_PDU,
      { "E-RABReleaseCommand", "s1ap.E_RABReleaseCommand_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABReleaseResponse_PDU,
      { "E-RABReleaseResponse", "s1ap.E_RABReleaseResponse_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABReleaseListBearerRelComp_PDU,
      { "E-RABReleaseListBearerRelComp", "s1ap.E_RABReleaseListBearerRelComp",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABReleaseItemBearerRelComp_PDU,
      { "E-RABReleaseItemBearerRelComp", "s1ap.E_RABReleaseItemBearerRelComp_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABReleaseIndication_PDU,
      { "E-RABReleaseIndication", "s1ap.E_RABReleaseIndication_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_InitialContextSetupRequest_PDU,
      { "InitialContextSetupRequest", "s1ap.InitialContextSetupRequest_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABToBeSetupListCtxtSUReq_PDU,
      { "E-RABToBeSetupListCtxtSUReq", "s1ap.E_RABToBeSetupListCtxtSUReq",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABToBeSetupItemCtxtSUReq_PDU,
      { "E-RABToBeSetupItemCtxtSUReq", "s1ap.E_RABToBeSetupItemCtxtSUReq_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_InitialContextSetupResponse_PDU,
      { "InitialContextSetupResponse", "s1ap.InitialContextSetupResponse_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABSetupListCtxtSURes_PDU,
      { "E-RABSetupListCtxtSURes", "s1ap.E_RABSetupListCtxtSURes",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABSetupItemCtxtSURes_PDU,
      { "E-RABSetupItemCtxtSURes", "s1ap.E_RABSetupItemCtxtSURes_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_InitialContextSetupFailure_PDU,
      { "InitialContextSetupFailure", "s1ap.InitialContextSetupFailure_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_Paging_PDU,
      { "Paging", "s1ap.Paging_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_TAIList_PDU,
      { "TAIList", "s1ap.TAIList",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_TAIItem_PDU,
      { "TAIItem", "s1ap.TAIItem_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UEContextReleaseRequest_PDU,
      { "UEContextReleaseRequest", "s1ap.UEContextReleaseRequest_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UEContextReleaseCommand_PDU,
      { "UEContextReleaseCommand", "s1ap.UEContextReleaseCommand_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UEContextReleaseComplete_PDU,
      { "UEContextReleaseComplete", "s1ap.UEContextReleaseComplete_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UEContextModificationRequest_PDU,
      { "UEContextModificationRequest", "s1ap.UEContextModificationRequest_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UEContextModificationResponse_PDU,
      { "UEContextModificationResponse", "s1ap.UEContextModificationResponse_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UEContextModificationFailure_PDU,
      { "UEContextModificationFailure", "s1ap.UEContextModificationFailure_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UERadioCapabilityMatchRequest_PDU,
      { "UERadioCapabilityMatchRequest", "s1ap.UERadioCapabilityMatchRequest_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UERadioCapabilityMatchResponse_PDU,
      { "UERadioCapabilityMatchResponse", "s1ap.UERadioCapabilityMatchResponse_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_DownlinkNASTransport_PDU,
      { "DownlinkNASTransport", "s1ap.DownlinkNASTransport_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_InitialUEMessage_PDU,
      { "InitialUEMessage", "s1ap.InitialUEMessage_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UplinkNASTransport_PDU,
      { "UplinkNASTransport", "s1ap.UplinkNASTransport_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_NASNonDeliveryIndication_PDU,
      { "NASNonDeliveryIndication", "s1ap.NASNonDeliveryIndication_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_RerouteNASRequest_PDU,
      { "RerouteNASRequest", "s1ap.RerouteNASRequest_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_Reset_PDU,
      { "Reset", "s1ap.Reset_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ResetType_PDU,
      { "ResetType", "s1ap.ResetType",
        FT_UINT32, BASE_DEC, VALS(s1ap_ResetType_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_ResetAcknowledge_PDU,
      { "ResetAcknowledge", "s1ap.ResetAcknowledge_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UE_associatedLogicalS1_ConnectionListResAck_PDU,
      { "UE-associatedLogicalS1-ConnectionListResAck", "s1ap.UE_associatedLogicalS1_ConnectionListResAck",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ErrorIndication_PDU,
      { "ErrorIndication", "s1ap.ErrorIndication_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_S1SetupRequest_PDU,
      { "S1SetupRequest", "s1ap.S1SetupRequest_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_S1SetupResponse_PDU,
      { "S1SetupResponse", "s1ap.S1SetupResponse_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_S1SetupFailure_PDU,
      { "S1SetupFailure", "s1ap.S1SetupFailure_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ENBConfigurationUpdate_PDU,
      { "ENBConfigurationUpdate", "s1ap.ENBConfigurationUpdate_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ENBConfigurationUpdateAcknowledge_PDU,
      { "ENBConfigurationUpdateAcknowledge", "s1ap.ENBConfigurationUpdateAcknowledge_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ENBConfigurationUpdateFailure_PDU,
      { "ENBConfigurationUpdateFailure", "s1ap.ENBConfigurationUpdateFailure_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_MMEConfigurationUpdate_PDU,
      { "MMEConfigurationUpdate", "s1ap.MMEConfigurationUpdate_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_MMEConfigurationUpdateAcknowledge_PDU,
      { "MMEConfigurationUpdateAcknowledge", "s1ap.MMEConfigurationUpdateAcknowledge_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_MMEConfigurationUpdateFailure_PDU,
      { "MMEConfigurationUpdateFailure", "s1ap.MMEConfigurationUpdateFailure_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_DownlinkS1cdma2000tunnelling_PDU,
      { "DownlinkS1cdma2000tunnelling", "s1ap.DownlinkS1cdma2000tunnelling_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UplinkS1cdma2000tunnelling_PDU,
      { "UplinkS1cdma2000tunnelling", "s1ap.UplinkS1cdma2000tunnelling_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UECapabilityInfoIndication_PDU,
      { "UECapabilityInfoIndication", "s1ap.UECapabilityInfoIndication_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ENBStatusTransfer_PDU,
      { "ENBStatusTransfer", "s1ap.ENBStatusTransfer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_MMEStatusTransfer_PDU,
      { "MMEStatusTransfer", "s1ap.MMEStatusTransfer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_TraceStart_PDU,
      { "TraceStart", "s1ap.TraceStart_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_TraceFailureIndication_PDU,
      { "TraceFailureIndication", "s1ap.TraceFailureIndication_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_DeactivateTrace_PDU,
      { "DeactivateTrace", "s1ap.DeactivateTrace_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_CellTrafficTrace_PDU,
      { "CellTrafficTrace", "s1ap.CellTrafficTrace_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_LocationReportingControl_PDU,
      { "LocationReportingControl", "s1ap.LocationReportingControl_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_LocationReportingFailureIndication_PDU,
      { "LocationReportingFailureIndication", "s1ap.LocationReportingFailureIndication_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_LocationReport_PDU,
      { "LocationReport", "s1ap.LocationReport_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_OverloadStart_PDU,
      { "OverloadStart", "s1ap.OverloadStart_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_OverloadStop_PDU,
      { "OverloadStop", "s1ap.OverloadStop_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_WriteReplaceWarningRequest_PDU,
      { "WriteReplaceWarningRequest", "s1ap.WriteReplaceWarningRequest_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_WriteReplaceWarningResponse_PDU,
      { "WriteReplaceWarningResponse", "s1ap.WriteReplaceWarningResponse_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ENBDirectInformationTransfer_PDU,
      { "ENBDirectInformationTransfer", "s1ap.ENBDirectInformationTransfer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_Inter_SystemInformationTransferType_PDU,
      { "Inter-SystemInformationTransferType", "s1ap.Inter_SystemInformationTransferType",
        FT_UINT32, BASE_DEC, VALS(s1ap_Inter_SystemInformationTransferType_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_MMEDirectInformationTransfer_PDU,
      { "MMEDirectInformationTransfer", "s1ap.MMEDirectInformationTransfer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ENBConfigurationTransfer_PDU,
      { "ENBConfigurationTransfer", "s1ap.ENBConfigurationTransfer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_MMEConfigurationTransfer_PDU,
      { "MMEConfigurationTransfer", "s1ap.MMEConfigurationTransfer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_PrivateMessage_PDU,
      { "PrivateMessage", "s1ap.PrivateMessage_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_KillRequest_PDU,
      { "KillRequest", "s1ap.KillRequest_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_KillResponse_PDU,
      { "KillResponse", "s1ap.KillResponse_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_PWSRestartIndication_PDU,
      { "PWSRestartIndication", "s1ap.PWSRestartIndication_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_PWSFailureIndication_PDU,
      { "PWSFailureIndication", "s1ap.PWSFailureIndication_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_DownlinkUEAssociatedLPPaTransport_PDU,
      { "DownlinkUEAssociatedLPPaTransport", "s1ap.DownlinkUEAssociatedLPPaTransport_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UplinkUEAssociatedLPPaTransport_PDU,
      { "UplinkUEAssociatedLPPaTransport", "s1ap.UplinkUEAssociatedLPPaTransport_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_DownlinkNonUEAssociatedLPPaTransport_PDU,
      { "DownlinkNonUEAssociatedLPPaTransport", "s1ap.DownlinkNonUEAssociatedLPPaTransport_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UplinkNonUEAssociatedLPPaTransport_PDU,
      { "UplinkNonUEAssociatedLPPaTransport", "s1ap.UplinkNonUEAssociatedLPPaTransport_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABModificationIndication_PDU,
      { "E-RABModificationIndication", "s1ap.E_RABModificationIndication_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABToBeModifiedListBearerModInd_PDU,
      { "E-RABToBeModifiedListBearerModInd", "s1ap.E_RABToBeModifiedListBearerModInd",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABToBeModifiedItemBearerModInd_PDU,
      { "E-RABToBeModifiedItemBearerModInd", "s1ap.E_RABToBeModifiedItemBearerModInd_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABNotToBeModifiedListBearerModInd_PDU,
      { "E-RABNotToBeModifiedListBearerModInd", "s1ap.E_RABNotToBeModifiedListBearerModInd",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABNotToBeModifiedItemBearerModInd_PDU,
      { "E-RABNotToBeModifiedItemBearerModInd", "s1ap.E_RABNotToBeModifiedItemBearerModInd_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_CSGMembershipInfo_PDU,
      { "CSGMembershipInfo", "s1ap.CSGMembershipInfo_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABModificationConfirm_PDU,
      { "E-RABModificationConfirm", "s1ap.E_RABModificationConfirm_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABModifyListBearerModConf_PDU,
      { "E-RABModifyListBearerModConf", "s1ap.E_RABModifyListBearerModConf",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABModifyItemBearerModConf_PDU,
      { "E-RABModifyItemBearerModConf", "s1ap.E_RABModifyItemBearerModConf_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UEContextModificationIndication_PDU,
      { "UEContextModificationIndication", "s1ap.UEContextModificationIndication_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UEContextModificationConfirm_PDU,
      { "UEContextModificationConfirm", "s1ap.UEContextModificationConfirm_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_S1AP_PDU_PDU,
      { "S1AP-PDU", "s1ap.S1AP_PDU",
        FT_UINT32, BASE_DEC, VALS(s1ap_S1AP_PDU_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_s1ap_SONtransferApplicationIdentity_PDU,
      { "SONtransferApplicationIdentity", "s1ap.SONtransferApplicationIdentity",
        FT_UINT32, BASE_DEC, VALS(s1ap_SONtransferApplicationIdentity_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_s1ap_SONtransferRequestContainer_PDU,
      { "SONtransferRequestContainer", "s1ap.SONtransferRequestContainer",
        FT_UINT32, BASE_DEC, VALS(s1ap_SONtransferRequestContainer_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_s1ap_SONtransferResponseContainer_PDU,
      { "SONtransferResponseContainer", "s1ap.SONtransferResponseContainer",
        FT_UINT32, BASE_DEC, VALS(s1ap_SONtransferResponseContainer_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_s1ap_SONtransferCause_PDU,
      { "SONtransferCause", "s1ap.SONtransferCause",
        FT_UINT32, BASE_DEC, VALS(s1ap_SONtransferCause_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_local,
      { "local", "s1ap.local",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_65535", HFILL }},
    { &hf_s1ap_global,
      { "global", "s1ap.global",
        FT_OID, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ProtocolIE_Container_item,
      { "ProtocolIE-Field", "s1ap.ProtocolIE_Field_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_id,
      { "id", "s1ap.id",
        FT_UINT32, BASE_DEC|BASE_EXT_STRING, &s1ap_ProtocolIE_ID_vals_ext, 0,
        "ProtocolIE_ID", HFILL }},
    { &hf_s1ap_criticality,
      { "criticality", "s1ap.criticality",
        FT_UINT32, BASE_DEC, VALS(s1ap_Criticality_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_ie_field_value,
      { "value", "s1ap.value_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "T_ie_field_value", HFILL }},
    { &hf_s1ap_ProtocolIE_ContainerList_item,
      { "ProtocolIE-SingleContainer", "s1ap.ProtocolIE_SingleContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ProtocolExtensionContainer_item,
      { "ProtocolExtensionField", "s1ap.ProtocolExtensionField_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ext_id,
      { "id", "s1ap.id",
        FT_UINT8, BASE_DEC|BASE_EXT_STRING, &s1ap_ProtocolIE_ID_vals_ext, 0,
        "ProtocolExtensionID", HFILL }},
    { &hf_s1ap_extensionValue,
      { "extensionValue", "s1ap.extensionValue_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_PrivateIE_Container_item,
      { "PrivateIE-Field", "s1ap.PrivateIE_Field_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_private_id,
      { "id", "s1ap.id",
        FT_UINT32, BASE_DEC, VALS(s1ap_PrivateIE_ID_vals), 0,
        "PrivateIE_ID", HFILL }},
    { &hf_s1ap_value,
      { "value", "s1ap.value_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_gUMMEI,
      { "gUMMEI", "s1ap.gUMMEI_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_m_TMSI,
      { "m-TMSI", "s1ap.m_TMSI",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_iE_Extensions,
      { "iE-Extensions", "s1ap.iE_Extensions",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ProtocolExtensionContainer", HFILL }},
    { &hf_s1ap_cellBased,
      { "cellBased", "s1ap.cellBased_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "CellBasedMDT", HFILL }},
    { &hf_s1ap_tABased,
      { "tABased", "s1ap.tABased_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TABasedMDT", HFILL }},
    { &hf_s1ap_pLMNWide,
      { "pLMNWide", "s1ap.pLMNWide_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_tAIBased,
      { "tAIBased", "s1ap.tAIBased_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TAIBasedMDT", HFILL }},
    { &hf_s1ap_priorityLevel,
      { "priorityLevel", "s1ap.priorityLevel",
        FT_UINT32, BASE_DEC, VALS(s1ap_PriorityLevel_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_pre_emptionCapability,
      { "pre-emptionCapability", "s1ap.pre_emptionCapability",
        FT_UINT32, BASE_DEC, VALS(s1ap_Pre_emptionCapability_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_pre_emptionVulnerability,
      { "pre-emptionVulnerability", "s1ap.pre_emptionVulnerability",
        FT_UINT32, BASE_DEC, VALS(s1ap_Pre_emptionVulnerability_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_Bearers_SubjectToStatusTransferList_item,
      { "ProtocolIE-SingleContainer", "s1ap.ProtocolIE_SingleContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_e_RAB_ID,
      { "e-RAB-ID", "s1ap.e_RAB_ID",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_uL_COUNTvalue,
      { "uL-COUNTvalue", "s1ap.uL_COUNTvalue_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "COUNTvalue", HFILL }},
    { &hf_s1ap_dL_COUNTvalue,
      { "dL-COUNTvalue", "s1ap.dL_COUNTvalue_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "COUNTvalue", HFILL }},
    { &hf_s1ap_receiveStatusofULPDCPSDUs,
      { "receiveStatusofULPDCPSDUs", "s1ap.receiveStatusofULPDCPSDUs",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_BPLMNs_item,
      { "PLMNidentity", "s1ap.PLMNidentity",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_cellID_Cancelled,
      { "cellID-Cancelled", "s1ap.cellID_Cancelled",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_tAI_Cancelled,
      { "tAI-Cancelled", "s1ap.tAI_Cancelled",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_emergencyAreaID_Cancelled,
      { "emergencyAreaID-Cancelled", "s1ap.emergencyAreaID_Cancelled",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_cellID_Broadcast,
      { "cellID-Broadcast", "s1ap.cellID_Broadcast",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_tAI_Broadcast,
      { "tAI-Broadcast", "s1ap.tAI_Broadcast",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_emergencyAreaID_Broadcast,
      { "emergencyAreaID-Broadcast", "s1ap.emergencyAreaID_Broadcast",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_CancelledCellinEAI_item,
      { "CancelledCellinEAI-Item", "s1ap.CancelledCellinEAI_Item_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_eCGI,
      { "eCGI", "s1ap.eCGI_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "EUTRAN_CGI", HFILL }},
    { &hf_s1ap_numberOfBroadcasts,
      { "numberOfBroadcasts", "s1ap.numberOfBroadcasts",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_CancelledCellinTAI_item,
      { "CancelledCellinTAI-Item", "s1ap.CancelledCellinTAI_Item_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_radioNetwork,
      { "radioNetwork", "s1ap.radioNetwork",
        FT_UINT32, BASE_DEC|BASE_EXT_STRING, &s1ap_CauseRadioNetwork_vals_ext, 0,
        "CauseRadioNetwork", HFILL }},
    { &hf_s1ap_transport,
      { "transport", "s1ap.transport",
        FT_UINT32, BASE_DEC, VALS(s1ap_CauseTransport_vals), 0,
        "CauseTransport", HFILL }},
    { &hf_s1ap_nas,
      { "nas", "s1ap.nas",
        FT_UINT32, BASE_DEC, VALS(s1ap_CauseNas_vals), 0,
        "CauseNas", HFILL }},
    { &hf_s1ap_protocol,
      { "protocol", "s1ap.protocol",
        FT_UINT32, BASE_DEC, VALS(s1ap_CauseProtocol_vals), 0,
        "CauseProtocol", HFILL }},
    { &hf_s1ap_misc,
      { "misc", "s1ap.misc",
        FT_UINT32, BASE_DEC, VALS(s1ap_CauseMisc_vals), 0,
        "CauseMisc", HFILL }},
    { &hf_s1ap_CellID_Broadcast_item,
      { "CellID-Broadcast-Item", "s1ap.CellID_Broadcast_Item_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_CellID_Cancelled_item,
      { "CellID-Cancelled-Item", "s1ap.CellID_Cancelled_Item_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_cellIdListforMDT,
      { "cellIdListforMDT", "s1ap.cellIdListforMDT",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_CellIdListforMDT_item,
      { "EUTRAN-CGI", "s1ap.EUTRAN_CGI_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_cdma2000OneXMEID,
      { "cdma2000OneXMEID", "s1ap.cdma2000OneXMEID",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_cdma2000OneXMSI,
      { "cdma2000OneXMSI", "s1ap.cdma2000OneXMSI",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_cdma2000OneXPilot,
      { "cdma2000OneXPilot", "s1ap.cdma2000OneXPilot",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_cell_Size,
      { "cell-Size", "s1ap.cell_Size",
        FT_UINT32, BASE_DEC, VALS(s1ap_Cell_Size_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_pLMNidentity,
      { "pLMNidentity", "s1ap.pLMNidentity",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_lAC,
      { "lAC", "s1ap.lAC",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_cI,
      { "cI", "s1ap.cI",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_rAC,
      { "rAC", "s1ap.rAC",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_CSG_IdList_item,
      { "CSG-IdList-Item", "s1ap.CSG_IdList_Item_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_cSG_Id,
      { "cSG-Id", "s1ap.cSG_Id",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_pDCP_SN,
      { "pDCP-SN", "s1ap.pDCP_SN",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_hFN,
      { "hFN", "s1ap.hFN",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_pDCP_SNExtended,
      { "pDCP-SNExtended", "s1ap.pDCP_SNExtended",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_hFNModified,
      { "hFNModified", "s1ap.hFNModified",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_pDCP_SNlength18,
      { "pDCP-SNlength18", "s1ap.pDCP_SNlength18",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_hFNforPDCP_SNlength18,
      { "hFNforPDCP-SNlength18", "s1ap.hFNforPDCP_SNlength18",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_procedureCode,
      { "procedureCode", "s1ap.procedureCode",
        FT_UINT32, BASE_DEC|BASE_EXT_STRING, &s1ap_ProcedureCode_vals_ext, 0,
        NULL, HFILL }},
    { &hf_s1ap_triggeringMessage,
      { "triggeringMessage", "s1ap.triggeringMessage",
        FT_UINT32, BASE_DEC, VALS(s1ap_TriggeringMessage_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_procedureCriticality,
      { "procedureCriticality", "s1ap.procedureCriticality",
        FT_UINT32, BASE_DEC, VALS(s1ap_Criticality_vals), 0,
        "Criticality", HFILL }},
    { &hf_s1ap_iEsCriticalityDiagnostics,
      { "iEsCriticalityDiagnostics", "s1ap.iEsCriticalityDiagnostics",
        FT_UINT32, BASE_DEC, NULL, 0,
        "CriticalityDiagnostics_IE_List", HFILL }},
    { &hf_s1ap_CriticalityDiagnostics_IE_List_item,
      { "CriticalityDiagnostics-IE-Item", "s1ap.CriticalityDiagnostics_IE_Item_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_iECriticality,
      { "iECriticality", "s1ap.iECriticality",
        FT_UINT32, BASE_DEC, VALS(s1ap_Criticality_vals), 0,
        "Criticality", HFILL }},
    { &hf_s1ap_iE_ID,
      { "iE-ID", "s1ap.iE_ID",
        FT_UINT32, BASE_DEC|BASE_EXT_STRING, &s1ap_ProtocolIE_ID_vals_ext, 0,
        "ProtocolIE_ID", HFILL }},
    { &hf_s1ap_typeOfError,
      { "typeOfError", "s1ap.typeOfError",
        FT_UINT32, BASE_DEC, VALS(s1ap_TypeOfError_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_ECGIList_item,
      { "EUTRAN-CGI", "s1ap.EUTRAN_CGI_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_PWSfailedECGIList_item,
      { "EUTRAN-CGI", "s1ap.EUTRAN_CGI_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_EmergencyAreaIDList_item,
      { "EmergencyAreaID", "s1ap.EmergencyAreaID",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_EmergencyAreaID_Broadcast_item,
      { "EmergencyAreaID-Broadcast-Item", "s1ap.EmergencyAreaID_Broadcast_Item_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_emergencyAreaID,
      { "emergencyAreaID", "s1ap.emergencyAreaID",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_completedCellinEAI,
      { "completedCellinEAI", "s1ap.completedCellinEAI",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_EmergencyAreaID_Cancelled_item,
      { "EmergencyAreaID-Cancelled-Item", "s1ap.EmergencyAreaID_Cancelled_Item_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_cancelledCellinEAI,
      { "cancelledCellinEAI", "s1ap.cancelledCellinEAI",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_CompletedCellinEAI_item,
      { "CompletedCellinEAI-Item", "s1ap.CompletedCellinEAI_Item_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ECGI_List_item,
      { "EUTRAN-CGI", "s1ap.EUTRAN_CGI_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_EmergencyAreaIDListForRestart_item,
      { "EmergencyAreaID", "s1ap.EmergencyAreaID",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_macroENB_ID,
      { "macroENB-ID", "s1ap.macroENB_ID",
        FT_BYTES, BASE_NONE, NULL, 0,
        "BIT_STRING_SIZE_20", HFILL }},
    { &hf_s1ap_homeENB_ID,
      { "homeENB-ID", "s1ap.homeENB_ID",
        FT_BYTES, BASE_NONE, NULL, 0,
        "BIT_STRING_SIZE_28", HFILL }},
    { &hf_s1ap_lAI,
      { "lAI", "s1ap.lAI_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_eNB_ID,
      { "eNB-ID", "s1ap.eNB_ID",
        FT_UINT32, BASE_DEC, VALS(s1ap_ENB_ID_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_GUMMEIList_item,
      { "GUMMEI", "s1ap.GUMMEI_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_bearers_SubjectToStatusTransferList,
      { "bearers-SubjectToStatusTransferList", "s1ap.bearers_SubjectToStatusTransferList",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ENBX2TLAs_item,
      { "TransportLayerAddress", "s1ap.TransportLayerAddress",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_EPLMNs_item,
      { "PLMNidentity", "s1ap.PLMNidentity",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABInformationList_item,
      { "ProtocolIE-SingleContainer", "s1ap.ProtocolIE_SingleContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_dL_Forwarding,
      { "dL-Forwarding", "s1ap.dL_Forwarding",
        FT_UINT32, BASE_DEC, VALS(s1ap_DL_Forwarding_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABList_item,
      { "ProtocolIE-SingleContainer", "s1ap.ProtocolIE_SingleContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_cause,
      { "cause", "s1ap.cause",
        FT_UINT32, BASE_DEC, VALS(s1ap_Cause_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_qCI,
      { "qCI", "s1ap.qCI",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_allocationRetentionPriority,
      { "allocationRetentionPriority", "s1ap.allocationRetentionPriority_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "AllocationAndRetentionPriority", HFILL }},
    { &hf_s1ap_gbrQosInformation,
      { "gbrQosInformation", "s1ap.gbrQosInformation_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "GBR_QosInformation", HFILL }},
    { &hf_s1ap_cell_ID,
      { "cell-ID", "s1ap.cell_ID",
        FT_BYTES, BASE_NONE, NULL, 0,
        "CellIdentity", HFILL }},
    { &hf_s1ap_expectedActivity,
      { "expectedActivity", "s1ap.expectedActivity_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "ExpectedUEActivityBehaviour", HFILL }},
    { &hf_s1ap_expectedHOInterval,
      { "expectedHOInterval", "s1ap.expectedHOInterval",
        FT_UINT32, BASE_DEC, VALS(s1ap_ExpectedHOInterval_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_expectedActivityPeriod,
      { "expectedActivityPeriod", "s1ap.expectedActivityPeriod",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_expectedIdlePeriod,
      { "expectedIdlePeriod", "s1ap.expectedIdlePeriod",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_sourceofUEActivityBehaviourInformation,
      { "sourceofUEActivityBehaviourInformation", "s1ap.sourceofUEActivityBehaviourInformation",
        FT_UINT32, BASE_DEC, VALS(s1ap_SourceOfUEActivityBehaviourInformation_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_ForbiddenTAs_item,
      { "ForbiddenTAs-Item", "s1ap.ForbiddenTAs_Item_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_pLMN_Identity,
      { "pLMN-Identity", "s1ap.pLMN_Identity",
        FT_BYTES, BASE_NONE, NULL, 0,
        "PLMNidentity", HFILL }},
    { &hf_s1ap_forbiddenTACs,
      { "forbiddenTACs", "s1ap.forbiddenTACs",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ForbiddenTACs_item,
      { "TAC", "s1ap.TAC",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ForbiddenLAs_item,
      { "ForbiddenLAs-Item", "s1ap.ForbiddenLAs_Item_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_forbiddenLACs,
      { "forbiddenLACs", "s1ap.forbiddenLACs",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ForbiddenLACs_item,
      { "LAC", "s1ap.LAC",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_e_RAB_MaximumBitrateDL,
      { "e-RAB-MaximumBitrateDL", "s1ap.e_RAB_MaximumBitrateDL",
        FT_UINT64, BASE_DEC, NULL, 0,
        "BitRate", HFILL }},
    { &hf_s1ap_e_RAB_MaximumBitrateUL,
      { "e-RAB-MaximumBitrateUL", "s1ap.e_RAB_MaximumBitrateUL",
        FT_UINT64, BASE_DEC, NULL, 0,
        "BitRate", HFILL }},
    { &hf_s1ap_e_RAB_GuaranteedBitrateDL,
      { "e-RAB-GuaranteedBitrateDL", "s1ap.e_RAB_GuaranteedBitrateDL",
        FT_UINT64, BASE_DEC, NULL, 0,
        "BitRate", HFILL }},
    { &hf_s1ap_e_RAB_GuaranteedBitrateUL,
      { "e-RAB-GuaranteedBitrateUL", "s1ap.e_RAB_GuaranteedBitrateUL",
        FT_UINT64, BASE_DEC, NULL, 0,
        "BitRate", HFILL }},
    { &hf_s1ap_mME_Group_ID,
      { "mME-Group-ID", "s1ap.mME_Group_ID",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_mME_Code,
      { "mME-Code", "s1ap.mME_Code",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_servingPLMN,
      { "servingPLMN", "s1ap.servingPLMN",
        FT_BYTES, BASE_NONE, NULL, 0,
        "PLMNidentity", HFILL }},
    { &hf_s1ap_equivalentPLMNs,
      { "equivalentPLMNs", "s1ap.equivalentPLMNs",
        FT_UINT32, BASE_DEC, NULL, 0,
        "EPLMNs", HFILL }},
    { &hf_s1ap_forbiddenTAs,
      { "forbiddenTAs", "s1ap.forbiddenTAs",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_forbiddenLAs,
      { "forbiddenLAs", "s1ap.forbiddenLAs",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_forbiddenInterRATs,
      { "forbiddenInterRATs", "s1ap.forbiddenInterRATs",
        FT_UINT32, BASE_DEC, VALS(s1ap_ForbiddenInterRATs_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_measurementsToActivate,
      { "measurementsToActivate", "s1ap.measurementsToActivate",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_m1reportingTrigger,
      { "m1reportingTrigger", "s1ap.m1reportingTrigger",
        FT_UINT32, BASE_DEC, VALS(s1ap_M1ReportingTrigger_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_m1thresholdeventA2,
      { "m1thresholdeventA2", "s1ap.m1thresholdeventA2_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_m1periodicReporting,
      { "m1periodicReporting", "s1ap.m1periodicReporting_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_e_UTRAN_Cell,
      { "e-UTRAN-Cell", "s1ap.e_UTRAN_Cell_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "LastVisitedEUTRANCellInformation", HFILL }},
    { &hf_s1ap_uTRAN_Cell,
      { "uTRAN-Cell", "s1ap.uTRAN_Cell",
        FT_BYTES, BASE_NONE, NULL, 0,
        "LastVisitedUTRANCellInformation", HFILL }},
    { &hf_s1ap_gERAN_Cell,
      { "gERAN-Cell", "s1ap.gERAN_Cell",
        FT_UINT32, BASE_DEC, VALS(s1ap_LastVisitedGERANCellInformation_vals), 0,
        "LastVisitedGERANCellInformation", HFILL }},
    { &hf_s1ap_global_Cell_ID,
      { "global-Cell-ID", "s1ap.global_Cell_ID_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "EUTRAN_CGI", HFILL }},
    { &hf_s1ap_cellType,
      { "cellType", "s1ap.cellType_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_time_UE_StayedInCell,
      { "time-UE-StayedInCell", "s1ap.time_UE_StayedInCell",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_undefined,
      { "undefined", "s1ap.undefined_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_pattern_period,
      { "pattern-period", "s1ap.pattern_period",
        FT_UINT32, BASE_DEC, VALS(s1ap_T_pattern_period_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_pattern_offset,
      { "pattern-offset", "s1ap.pattern_offset",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_10239_", HFILL }},
    { &hf_s1ap_loggingInterval,
      { "loggingInterval", "s1ap.loggingInterval",
        FT_UINT32, BASE_DEC, VALS(s1ap_LoggingInterval_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_loggingDuration,
      { "loggingDuration", "s1ap.loggingDuration",
        FT_UINT32, BASE_DEC, VALS(s1ap_LoggingDuration_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_mBSFN_ResultToLog,
      { "mBSFN-ResultToLog", "s1ap.mBSFN_ResultToLog",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_m3period,
      { "m3period", "s1ap.m3period",
        FT_UINT32, BASE_DEC, VALS(s1ap_M3period_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_m4period,
      { "m4period", "s1ap.m4period",
        FT_UINT32, BASE_DEC, VALS(s1ap_M4period_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_m4_links_to_log,
      { "m4-links-to-log", "s1ap.m4_links_to_log",
        FT_UINT32, BASE_DEC, VALS(s1ap_Links_to_log_vals), 0,
        "Links_to_log", HFILL }},
    { &hf_s1ap_m5period,
      { "m5period", "s1ap.m5period",
        FT_UINT32, BASE_DEC, VALS(s1ap_M5period_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_m5_links_to_log,
      { "m5-links-to-log", "s1ap.m5_links_to_log",
        FT_UINT32, BASE_DEC, VALS(s1ap_Links_to_log_vals), 0,
        "Links_to_log", HFILL }},
    { &hf_s1ap_m6report_Interval,
      { "m6report-Interval", "s1ap.m6report_Interval",
        FT_UINT32, BASE_DEC, VALS(s1ap_M6report_Interval_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_m6delay_threshold,
      { "m6delay-threshold", "s1ap.m6delay_threshold",
        FT_UINT32, BASE_DEC, VALS(s1ap_M6delay_threshold_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_m6_links_to_log,
      { "m6-links-to-log", "s1ap.m6_links_to_log",
        FT_UINT32, BASE_DEC, VALS(s1ap_Links_to_log_vals), 0,
        "Links_to_log", HFILL }},
    { &hf_s1ap_m7period,
      { "m7period", "s1ap.m7period",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_m7_links_to_log,
      { "m7-links-to-log", "s1ap.m7_links_to_log",
        FT_UINT32, BASE_DEC, VALS(s1ap_Links_to_log_vals), 0,
        "Links_to_log", HFILL }},
    { &hf_s1ap_mdt_Activation,
      { "mdt-Activation", "s1ap.mdt_Activation",
        FT_UINT32, BASE_DEC, VALS(s1ap_MDT_Activation_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_areaScopeOfMDT,
      { "areaScopeOfMDT", "s1ap.areaScopeOfMDT",
        FT_UINT32, BASE_DEC, VALS(s1ap_AreaScopeOfMDT_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_mDTMode,
      { "mDTMode", "s1ap.mDTMode",
        FT_UINT32, BASE_DEC, VALS(s1ap_MDTMode_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_MBSFN_ResultToLog_item,
      { "MBSFN-ResultToLogInfo", "s1ap.MBSFN_ResultToLogInfo_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_mBSFN_AreaId,
      { "mBSFN-AreaId", "s1ap.mBSFN_AreaId",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_255", HFILL }},
    { &hf_s1ap_carrierFreq,
      { "carrierFreq", "s1ap.carrierFreq",
        FT_UINT32, BASE_DEC, NULL, 0,
        "EARFCN", HFILL }},
    { &hf_s1ap_MDTPLMNList_item,
      { "PLMNidentity", "s1ap.PLMNidentity",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_immediateMDT,
      { "immediateMDT", "s1ap.immediateMDT_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_loggedMDT,
      { "loggedMDT", "s1ap.loggedMDT_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_mDTMode_Extension,
      { "mDTMode-Extension", "s1ap.mDTMode_Extension_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_threshold_RSRP,
      { "threshold-RSRP", "s1ap.threshold_RSRP",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_threshold_RSRQ,
      { "threshold-RSRQ", "s1ap.threshold_RSRQ",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_muting_pattern_period,
      { "muting-pattern-period", "s1ap.muting_pattern_period",
        FT_UINT32, BASE_DEC, VALS(s1ap_T_muting_pattern_period_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_muting_pattern_offset,
      { "muting-pattern-offset", "s1ap.muting_pattern_offset",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_10239_", HFILL }},
    { &hf_s1ap_overloadAction,
      { "overloadAction", "s1ap.overloadAction",
        FT_UINT32, BASE_DEC, VALS(s1ap_OverloadAction_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_reportInterval,
      { "reportInterval", "s1ap.reportInterval",
        FT_UINT32, BASE_DEC, VALS(s1ap_ReportIntervalMDT_vals), 0,
        "ReportIntervalMDT", HFILL }},
    { &hf_s1ap_reportAmount,
      { "reportAmount", "s1ap.reportAmount",
        FT_UINT32, BASE_DEC, VALS(s1ap_ReportAmountMDT_vals), 0,
        "ReportAmountMDT", HFILL }},
    { &hf_s1ap_proSeDirectDiscovery,
      { "proSeDirectDiscovery", "s1ap.proSeDirectDiscovery",
        FT_UINT32, BASE_DEC, VALS(s1ap_ProSeDirectDiscovery_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_proSeDirectCommunication,
      { "proSeDirectCommunication", "s1ap.proSeDirectCommunication",
        FT_UINT32, BASE_DEC, VALS(s1ap_ProSeDirectCommunication_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_eventType,
      { "eventType", "s1ap.eventType",
        FT_UINT32, BASE_DEC, VALS(s1ap_EventType_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_reportArea,
      { "reportArea", "s1ap.reportArea",
        FT_UINT32, BASE_DEC, VALS(s1ap_ReportArea_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_rIMInformation,
      { "rIMInformation", "s1ap.rIMInformation",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_rIMRoutingAddress,
      { "rIMRoutingAddress", "s1ap.rIMRoutingAddress",
        FT_UINT32, BASE_DEC, VALS(s1ap_RIMRoutingAddress_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_gERAN_Cell_ID,
      { "gERAN-Cell-ID", "s1ap.gERAN_Cell_ID_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_targetRNC_ID,
      { "targetRNC-ID", "s1ap.targetRNC_ID_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_eHRPD_Sector_ID,
      { "eHRPD-Sector-ID", "s1ap.eHRPD_Sector_ID",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING_SIZE_16", HFILL }},
    { &hf_s1ap_uE_RLF_Report_Container,
      { "uE-RLF-Report-Container", "s1ap.uE_RLF_Report_Container",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_uE_RLF_Report_Container_for_extended_bands,
      { "uE-RLF-Report-Container-for-extended-bands", "s1ap.uE_RLF_Report_Container_for_extended_bands",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ECGIListForRestart_item,
      { "EUTRAN-CGI", "s1ap.EUTRAN_CGI_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_nextHopChainingCount,
      { "nextHopChainingCount", "s1ap.nextHopChainingCount",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_7", HFILL }},
    { &hf_s1ap_nextHopParameter,
      { "nextHopParameter", "s1ap.nextHopParameter",
        FT_BYTES, BASE_NONE, NULL, 0,
        "SecurityKey", HFILL }},
    { &hf_s1ap_sONInformationRequest,
      { "sONInformationRequest", "s1ap.sONInformationRequest",
        FT_UINT32, BASE_DEC, VALS(s1ap_SONInformationRequest_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_sONInformationReply,
      { "sONInformationReply", "s1ap.sONInformationReply_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_sONInformation_Extension,
      { "sONInformation-Extension", "s1ap.sONInformation_Extension_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_x2TNLConfigurationInfo,
      { "x2TNLConfigurationInfo", "s1ap.x2TNLConfigurationInfo_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_rLFReportInformation,
      { "rLFReportInformation", "s1ap.rLFReportInformation_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_targeteNB_ID,
      { "targeteNB-ID", "s1ap.targeteNB_ID_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_sourceeNB_ID,
      { "sourceeNB-ID", "s1ap.sourceeNB_ID_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_sONInformation,
      { "sONInformation", "s1ap.sONInformation",
        FT_UINT32, BASE_DEC, VALS(s1ap_SONInformation_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_sourceStratumLevel,
      { "sourceStratumLevel", "s1ap.sourceStratumLevel",
        FT_UINT32, BASE_DEC, NULL, 0,
        "StratumLevel", HFILL }},
    { &hf_s1ap_listeningSubframePattern,
      { "listeningSubframePattern", "s1ap.listeningSubframePattern_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_aggressoreCGI_List,
      { "aggressoreCGI-List", "s1ap.aggressoreCGI_List",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ECGI_List", HFILL }},
    { &hf_s1ap_global_ENB_ID,
      { "global-ENB-ID", "s1ap.global_ENB_ID_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_selected_TAI,
      { "selected-TAI", "s1ap.selected_TAI_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TAI", HFILL }},
    { &hf_s1ap_rRC_Container,
      { "rRC-Container", "s1ap.rRC_Container",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_e_RABInformationList,
      { "e-RABInformationList", "s1ap.e_RABInformationList",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_targetCell_ID,
      { "targetCell-ID", "s1ap.targetCell_ID_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "EUTRAN_CGI", HFILL }},
    { &hf_s1ap_subscriberProfileIDforRFP,
      { "subscriberProfileIDforRFP", "s1ap.subscriberProfileIDforRFP",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_uE_HistoryInformation,
      { "uE-HistoryInformation", "s1ap.uE_HistoryInformation",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ServedGUMMEIs_item,
      { "ServedGUMMEIsItem", "s1ap.ServedGUMMEIsItem_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_servedPLMNs,
      { "servedPLMNs", "s1ap.servedPLMNs",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_servedGroupIDs,
      { "servedGroupIDs", "s1ap.servedGroupIDs",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_servedMMECs,
      { "servedMMECs", "s1ap.servedMMECs",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ServedGroupIDs_item,
      { "MME-Group-ID", "s1ap.MME_Group_ID",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ServedMMECs_item,
      { "MME-Code", "s1ap.MME_Code",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ServedPLMNs_item,
      { "PLMNidentity", "s1ap.PLMNidentity",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_SupportedTAs_item,
      { "SupportedTAs-Item", "s1ap.SupportedTAs_Item_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_tAC,
      { "tAC", "s1ap.tAC",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_broadcastPLMNs,
      { "broadcastPLMNs", "s1ap.broadcastPLMNs",
        FT_UINT32, BASE_DEC, NULL, 0,
        "BPLMNs", HFILL }},
    { &hf_s1ap_stratumLevel,
      { "stratumLevel", "s1ap.stratumLevel",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_synchronisationStatus,
      { "synchronisationStatus", "s1ap.synchronisationStatus",
        FT_UINT32, BASE_DEC, VALS(s1ap_SynchronisationStatus_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_mMEC,
      { "mMEC", "s1ap.mMEC",
        FT_BYTES, BASE_NONE, NULL, 0,
        "MME_Code", HFILL }},
    { &hf_s1ap_tAIListforMDT,
      { "tAIListforMDT", "s1ap.tAIListforMDT",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_TAIListforMDT_item,
      { "TAI", "s1ap.TAI_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_TAIListforWarning_item,
      { "TAI", "s1ap.TAI_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_TAI_Broadcast_item,
      { "TAI-Broadcast-Item", "s1ap.TAI_Broadcast_Item_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_tAI,
      { "tAI", "s1ap.tAI_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_completedCellinTAI,
      { "completedCellinTAI", "s1ap.completedCellinTAI",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_TAI_Cancelled_item,
      { "TAI-Cancelled-Item", "s1ap.TAI_Cancelled_Item_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_cancelledCellinTAI,
      { "cancelledCellinTAI", "s1ap.cancelledCellinTAI",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_tAListforMDT,
      { "tAListforMDT", "s1ap.tAListforMDT",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_TAListforMDT_item,
      { "TAC", "s1ap.TAC",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_CompletedCellinTAI_item,
      { "CompletedCellinTAI-Item", "s1ap.CompletedCellinTAI_Item_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_cGI,
      { "cGI", "s1ap.cGI_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_rNC_ID,
      { "rNC-ID", "s1ap.rNC_ID",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_extendedRNC_ID,
      { "extendedRNC-ID", "s1ap.extendedRNC_ID",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_measurementThreshold,
      { "measurementThreshold", "s1ap.measurementThreshold",
        FT_UINT32, BASE_DEC, VALS(s1ap_MeasurementThresholdA2_vals), 0,
        "MeasurementThresholdA2", HFILL }},
    { &hf_s1ap_transportLayerAddress,
      { "transportLayerAddress", "s1ap.transportLayerAddress",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_uL_GTP_TEID,
      { "uL-GTP-TEID", "s1ap.uL_GTP_TEID",
        FT_BYTES, BASE_NONE, NULL, 0,
        "GTP_TEID", HFILL }},
    { &hf_s1ap_e_UTRAN_Trace_ID,
      { "e-UTRAN-Trace-ID", "s1ap.e_UTRAN_Trace_ID",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_interfacesToTrace,
      { "interfacesToTrace", "s1ap.interfacesToTrace",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_traceDepth,
      { "traceDepth", "s1ap.traceDepth",
        FT_UINT32, BASE_DEC, VALS(s1ap_TraceDepth_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_traceCollectionEntityIPAddress,
      { "traceCollectionEntityIPAddress", "s1ap.traceCollectionEntityIPAddress",
        FT_BYTES, BASE_NONE, NULL, 0,
        "TransportLayerAddress", HFILL }},
    { &hf_s1ap_uDP_Port_Number,
      { "uDP-Port-Number", "s1ap.uDP_Port_Number",
        FT_BYTES, BASE_NONE, NULL, 0,
        "Port_Number", HFILL }},
    { &hf_s1ap_TAIListForRestart_item,
      { "TAI", "s1ap.TAI_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_uEaggregateMaximumBitRateDL,
      { "uEaggregateMaximumBitRateDL", "s1ap.uEaggregateMaximumBitRateDL",
        FT_UINT64, BASE_DEC, NULL, 0,
        "BitRate", HFILL }},
    { &hf_s1ap_uEaggregateMaximumBitRateUL,
      { "uEaggregateMaximumBitRateUL", "s1ap.uEaggregateMaximumBitRateUL",
        FT_UINT64, BASE_DEC, NULL, 0,
        "BitRate", HFILL }},
    { &hf_s1ap_uE_S1AP_ID_pair,
      { "uE-S1AP-ID-pair", "s1ap.uE_S1AP_ID_pair_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_mME_UE_S1AP_ID,
      { "mME-UE-S1AP-ID", "s1ap.mME_UE_S1AP_ID",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_eNB_UE_S1AP_ID,
      { "eNB-UE-S1AP-ID", "s1ap.eNB_UE_S1AP_ID",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UE_HistoryInformation_item,
      { "LastVisitedCell-Item", "s1ap.LastVisitedCell_Item",
        FT_UINT32, BASE_DEC, VALS(s1ap_LastVisitedCell_Item_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_s_TMSI,
      { "s-TMSI", "s1ap.s_TMSI_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_iMSI,
      { "iMSI", "s1ap.iMSI",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_encryptionAlgorithms,
      { "encryptionAlgorithms", "s1ap.encryptionAlgorithms",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_integrityProtectionAlgorithms,
      { "integrityProtectionAlgorithms", "s1ap.integrityProtectionAlgorithms",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_eutran_cgi,
      { "eutran-cgi", "s1ap.eutran_cgi_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_tai,
      { "tai", "s1ap.tai_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_cellIDList,
      { "cellIDList", "s1ap.cellIDList",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ECGIList", HFILL }},
    { &hf_s1ap_trackingAreaListforWarning,
      { "trackingAreaListforWarning", "s1ap.trackingAreaListforWarning",
        FT_UINT32, BASE_DEC, NULL, 0,
        "TAIListforWarning", HFILL }},
    { &hf_s1ap_emergencyAreaIDList,
      { "emergencyAreaIDList", "s1ap.emergencyAreaIDList",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_eNBX2TransportLayerAddresses,
      { "eNBX2TransportLayerAddresses", "s1ap.eNBX2TransportLayerAddresses",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ENBX2TLAs", HFILL }},
    { &hf_s1ap_ENBX2ExtTLAs_item,
      { "ENBX2ExtTLA", "s1ap.ENBX2ExtTLA_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_iPsecTLA,
      { "iPsecTLA", "s1ap.iPsecTLA",
        FT_BYTES, BASE_NONE, NULL, 0,
        "TransportLayerAddress", HFILL }},
    { &hf_s1ap_gTPTLAa,
      { "gTPTLAa", "s1ap.gTPTLAa",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ENBX2GTPTLAs", HFILL }},
    { &hf_s1ap_ENBX2GTPTLAs_item,
      { "TransportLayerAddress", "s1ap.TransportLayerAddress",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ENBIndirectX2TransportLayerAddresses_item,
      { "TransportLayerAddress", "s1ap.TransportLayerAddress",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_protocolIEs,
      { "protocolIEs", "s1ap.protocolIEs",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ProtocolIE_Container", HFILL }},
    { &hf_s1ap_dL_transportLayerAddress,
      { "dL-transportLayerAddress", "s1ap.dL_transportLayerAddress",
        FT_BYTES, BASE_NONE, NULL, 0,
        "TransportLayerAddress", HFILL }},
    { &hf_s1ap_dL_gTP_TEID,
      { "dL-gTP-TEID", "s1ap.dL_gTP_TEID",
        FT_BYTES, BASE_NONE, NULL, 0,
        "GTP_TEID", HFILL }},
    { &hf_s1ap_uL_TransportLayerAddress,
      { "uL-TransportLayerAddress", "s1ap.uL_TransportLayerAddress",
        FT_BYTES, BASE_NONE, NULL, 0,
        "TransportLayerAddress", HFILL }},
    { &hf_s1ap_gTP_TEID,
      { "gTP-TEID", "s1ap.gTP_TEID",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_e_RABlevelQosParameters,
      { "e-RABlevelQosParameters", "s1ap.e_RABlevelQosParameters_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABToBeSetupListBearerSUReq_item,
      { "ProtocolIE-SingleContainer", "s1ap.ProtocolIE_SingleContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_e_RABlevelQoSParameters,
      { "e-RABlevelQoSParameters", "s1ap.e_RABlevelQoSParameters_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_nAS_PDU,
      { "nAS-PDU", "s1ap.nAS_PDU",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABSetupListBearerSURes_item,
      { "ProtocolIE-SingleContainer", "s1ap.ProtocolIE_SingleContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABToBeModifiedListBearerModReq_item,
      { "ProtocolIE-SingleContainer", "s1ap.ProtocolIE_SingleContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_e_RABLevelQoSParameters,
      { "e-RABLevelQoSParameters", "s1ap.e_RABLevelQoSParameters_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABModifyListBearerModRes_item,
      { "ProtocolIE-SingleContainer", "s1ap.ProtocolIE_SingleContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABReleaseListBearerRelComp_item,
      { "ProtocolIE-SingleContainer", "s1ap.ProtocolIE_SingleContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABToBeSetupListCtxtSUReq_item,
      { "ProtocolIE-SingleContainer", "s1ap.ProtocolIE_SingleContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABSetupListCtxtSURes_item,
      { "ProtocolIE-SingleContainer", "s1ap.ProtocolIE_SingleContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_TAIList_item,
      { "ProtocolIE-SingleContainer", "s1ap.ProtocolIE_SingleContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_s1_Interface,
      { "s1-Interface", "s1ap.s1_Interface",
        FT_UINT32, BASE_DEC, VALS(s1ap_ResetAll_vals), 0,
        "ResetAll", HFILL }},
    { &hf_s1ap_partOfS1_Interface,
      { "partOfS1-Interface", "s1ap.partOfS1_Interface",
        FT_UINT32, BASE_DEC, NULL, 0,
        "UE_associatedLogicalS1_ConnectionListRes", HFILL }},
    { &hf_s1ap_UE_associatedLogicalS1_ConnectionListRes_item,
      { "ProtocolIE-SingleContainer", "s1ap.ProtocolIE_SingleContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_UE_associatedLogicalS1_ConnectionListResAck_item,
      { "ProtocolIE-SingleContainer", "s1ap.ProtocolIE_SingleContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_rIMTransfer,
      { "rIMTransfer", "s1ap.rIMTransfer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_privateIEs,
      { "privateIEs", "s1ap.privateIEs",
        FT_UINT32, BASE_DEC, NULL, 0,
        "PrivateIE_Container", HFILL }},
    { &hf_s1ap_dL_GTP_TEID,
      { "dL-GTP-TEID", "s1ap.dL_GTP_TEID",
        FT_BYTES, BASE_NONE, NULL, 0,
        "GTP_TEID", HFILL }},
    { &hf_s1ap_cSGMembershipStatus,
      { "cSGMembershipStatus", "s1ap.cSGMembershipStatus",
        FT_UINT32, BASE_DEC, VALS(s1ap_CSGMembershipStatus_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_cellAccessMode,
      { "cellAccessMode", "s1ap.cellAccessMode",
        FT_UINT32, BASE_DEC, VALS(s1ap_CellAccessMode_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_E_RABModifyListBearerModConf_item,
      { "ProtocolIE-SingleContainer", "s1ap.ProtocolIE_SingleContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_initiatingMessage,
      { "initiatingMessage", "s1ap.initiatingMessage_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_successfulOutcome,
      { "successfulOutcome", "s1ap.successfulOutcome_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_unsuccessfulOutcome,
      { "unsuccessfulOutcome", "s1ap.unsuccessfulOutcome_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_initiatingMessagevalue,
      { "value", "s1ap.value_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "InitiatingMessage_value", HFILL }},
    { &hf_s1ap_successfulOutcome_value,
      { "value", "s1ap.value_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "SuccessfulOutcome_value", HFILL }},
    { &hf_s1ap_unsuccessfulOutcome_value,
      { "value", "s1ap.value_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "UnsuccessfulOutcome_value", HFILL }},
    { &hf_s1ap_cellLoadReporting,
      { "cellLoadReporting", "s1ap.cellLoadReporting_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_multiCellLoadReporting,
      { "multiCellLoadReporting", "s1ap.multiCellLoadReporting_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "MultiCellLoadReportingRequest", HFILL }},
    { &hf_s1ap_eventTriggeredCellLoadReporting,
      { "eventTriggeredCellLoadReporting", "s1ap.eventTriggeredCellLoadReporting_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "EventTriggeredCellLoadReportingRequest", HFILL }},
    { &hf_s1ap_hOReporting,
      { "hOReporting", "s1ap.hOReporting_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "HOReport", HFILL }},
    { &hf_s1ap_eutranCellActivation,
      { "eutranCellActivation", "s1ap.eutranCellActivation_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "CellActivationRequest", HFILL }},
    { &hf_s1ap_energySavingsIndication,
      { "energySavingsIndication", "s1ap.energySavingsIndication_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "CellStateIndication", HFILL }},
    { &hf_s1ap_failureEventReporting,
      { "failureEventReporting", "s1ap.failureEventReporting",
        FT_UINT32, BASE_DEC, VALS(s1ap_FailureEventReport_vals), 0,
        "FailureEventReport", HFILL }},
    { &hf_s1ap_cellLoadReporting_01,
      { "cellLoadReporting", "s1ap.cellLoadReporting",
        FT_UINT32, BASE_DEC, VALS(s1ap_CellLoadReportingResponse_vals), 0,
        "CellLoadReportingResponse", HFILL }},
    { &hf_s1ap_multiCellLoadReporting_01,
      { "multiCellLoadReporting", "s1ap.multiCellLoadReporting",
        FT_UINT32, BASE_DEC, NULL, 0,
        "MultiCellLoadReportingResponse", HFILL }},
    { &hf_s1ap_eventTriggeredCellLoadReporting_01,
      { "eventTriggeredCellLoadReporting", "s1ap.eventTriggeredCellLoadReporting_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "EventTriggeredCellLoadReportingResponse", HFILL }},
    { &hf_s1ap_hOReporting_01,
      { "hOReporting", "s1ap.hOReporting_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_eutranCellActivation_01,
      { "eutranCellActivation", "s1ap.eutranCellActivation_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "CellActivationResponse", HFILL }},
    { &hf_s1ap_energySavingsIndication_01,
      { "energySavingsIndication", "s1ap.energySavingsIndication_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_failureEventReporting_01,
      { "failureEventReporting", "s1ap.failureEventReporting_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_cellLoadReporting_02,
      { "cellLoadReporting", "s1ap.cellLoadReporting",
        FT_UINT32, BASE_DEC, VALS(s1ap_CellLoadReportingCause_vals), 0,
        "CellLoadReportingCause", HFILL }},
    { &hf_s1ap_multiCellLoadReporting_02,
      { "multiCellLoadReporting", "s1ap.multiCellLoadReporting",
        FT_UINT32, BASE_DEC, VALS(s1ap_CellLoadReportingCause_vals), 0,
        "CellLoadReportingCause", HFILL }},
    { &hf_s1ap_eventTriggeredCellLoadReporting_02,
      { "eventTriggeredCellLoadReporting", "s1ap.eventTriggeredCellLoadReporting",
        FT_UINT32, BASE_DEC, VALS(s1ap_CellLoadReportingCause_vals), 0,
        "CellLoadReportingCause", HFILL }},
    { &hf_s1ap_hOReporting_02,
      { "hOReporting", "s1ap.hOReporting",
        FT_UINT32, BASE_DEC, VALS(s1ap_HOReportingCause_vals), 0,
        "HOReportingCause", HFILL }},
    { &hf_s1ap_eutranCellActivation_02,
      { "eutranCellActivation", "s1ap.eutranCellActivation",
        FT_UINT32, BASE_DEC, VALS(s1ap_CellActivationCause_vals), 0,
        "CellActivationCause", HFILL }},
    { &hf_s1ap_energySavingsIndication_02,
      { "energySavingsIndication", "s1ap.energySavingsIndication",
        FT_UINT32, BASE_DEC, VALS(s1ap_CellStateIndicationCause_vals), 0,
        "CellStateIndicationCause", HFILL }},
    { &hf_s1ap_failureEventReporting_02,
      { "failureEventReporting", "s1ap.failureEventReporting",
        FT_UINT32, BASE_DEC, VALS(s1ap_FailureEventReportingCause_vals), 0,
        "FailureEventReportingCause", HFILL }},
    { &hf_s1ap_eUTRAN,
      { "eUTRAN", "s1ap.eUTRAN_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "EUTRANcellLoadReportingResponse", HFILL }},
    { &hf_s1ap_uTRAN,
      { "uTRAN", "s1ap.uTRAN",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING", HFILL }},
    { &hf_s1ap_gERAN,
      { "gERAN", "s1ap.gERAN",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING", HFILL }},
    { &hf_s1ap_eHRPD,
      { "eHRPD", "s1ap.eHRPD_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "EHRPDSectorLoadReportingResponse", HFILL }},
    { &hf_s1ap_compositeAvailableCapacityGroup,
      { "compositeAvailableCapacityGroup", "s1ap.compositeAvailableCapacityGroup",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_cell_ID_01,
      { "cell-ID", "s1ap.cell_ID",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING", HFILL }},
    { &hf_s1ap_eUTRANcellLoadReportingResponse,
      { "eUTRANcellLoadReportingResponse", "s1ap.eUTRANcellLoadReportingResponse_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_eUTRAN_01,
      { "eUTRAN", "s1ap.eUTRAN",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING", HFILL }},
    { &hf_s1ap_eHRPD_01,
      { "eHRPD", "s1ap.eHRPD",
        FT_BYTES, BASE_NONE, NULL, 0,
        "EHRPD_Sector_ID", HFILL }},
    { &hf_s1ap_RequestedCellList_item,
      { "IRAT-Cell-ID", "s1ap.IRAT_Cell_ID",
        FT_UINT32, BASE_DEC, VALS(s1ap_IRAT_Cell_ID_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_requestedCellList,
      { "requestedCellList", "s1ap.requestedCellList",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_cell_ID_02,
      { "cell-ID", "s1ap.cell_ID",
        FT_UINT32, BASE_DEC, VALS(s1ap_IRAT_Cell_ID_vals), 0,
        "IRAT_Cell_ID", HFILL }},
    { &hf_s1ap_ReportingCellList_item,
      { "ReportingCellList-Item", "s1ap.ReportingCellList_Item_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_MultiCellLoadReportingResponse_item,
      { "MultiCellLoadReportingResponse-Item", "s1ap.MultiCellLoadReportingResponse_Item",
        FT_UINT32, BASE_DEC, VALS(s1ap_MultiCellLoadReportingResponse_Item_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_eUTRANResponse,
      { "eUTRANResponse", "s1ap.eUTRANResponse_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_uTRANResponse,
      { "uTRANResponse", "s1ap.uTRANResponse",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING", HFILL }},
    { &hf_s1ap_gERANResponse,
      { "gERANResponse", "s1ap.gERANResponse",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING", HFILL }},
    { &hf_s1ap_eHRPD_02,
      { "eHRPD", "s1ap.eHRPD_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "EHRPDMultiSectorLoadReportingResponseItem", HFILL }},
    { &hf_s1ap_numberOfMeasurementReportingLevels,
      { "numberOfMeasurementReportingLevels", "s1ap.numberOfMeasurementReportingLevels",
        FT_UINT32, BASE_DEC, VALS(s1ap_NumberOfMeasurementReportingLevels_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_cellLoadReportingResponse,
      { "cellLoadReportingResponse", "s1ap.cellLoadReportingResponse",
        FT_UINT32, BASE_DEC, VALS(s1ap_CellLoadReportingResponse_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_overloadFlag,
      { "overloadFlag", "s1ap.overloadFlag",
        FT_UINT32, BASE_DEC, VALS(s1ap_OverloadFlag_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_hoType,
      { "hoType", "s1ap.hoType",
        FT_UINT32, BASE_DEC, VALS(s1ap_HoType_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_hoReportType,
      { "hoReportType", "s1ap.hoReportType",
        FT_UINT32, BASE_DEC, VALS(s1ap_HoReportType_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_hosourceID,
      { "hosourceID", "s1ap.hosourceID",
        FT_UINT32, BASE_DEC, VALS(s1ap_IRAT_Cell_ID_vals), 0,
        "IRAT_Cell_ID", HFILL }},
    { &hf_s1ap_hoTargetID,
      { "hoTargetID", "s1ap.hoTargetID",
        FT_UINT32, BASE_DEC, VALS(s1ap_IRAT_Cell_ID_vals), 0,
        "IRAT_Cell_ID", HFILL }},
    { &hf_s1ap_candidateCellList,
      { "candidateCellList", "s1ap.candidateCellList",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_candidatePCIList,
      { "candidatePCIList", "s1ap.candidatePCIList",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_CandidateCellList_item,
      { "IRAT-Cell-ID", "s1ap.IRAT_Cell_ID",
        FT_UINT32, BASE_DEC, VALS(s1ap_IRAT_Cell_ID_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_CandidatePCIList_item,
      { "CandidatePCI", "s1ap.CandidatePCI_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_pCI,
      { "pCI", "s1ap.pCI",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_503", HFILL }},
    { &hf_s1ap_eARFCN,
      { "eARFCN", "s1ap.eARFCN",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING", HFILL }},
    { &hf_s1ap_cellsToActivateList,
      { "cellsToActivateList", "s1ap.cellsToActivateList",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_minimumActivationTime,
      { "minimumActivationTime", "s1ap.minimumActivationTime",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_60", HFILL }},
    { &hf_s1ap_CellsToActivateList_item,
      { "CellsToActivateList-Item", "s1ap.CellsToActivateList_Item_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_activatedCellsList,
      { "activatedCellsList", "s1ap.activatedCellsList",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_ActivatedCellsList_item,
      { "ActivatedCellsList-Item", "s1ap.ActivatedCellsList_Item_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_notificationCellList,
      { "notificationCellList", "s1ap.notificationCellList",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_NotificationCellList_item,
      { "NotificationCellList-Item", "s1ap.NotificationCellList_Item_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_notifyFlag,
      { "notifyFlag", "s1ap.notifyFlag",
        FT_UINT32, BASE_DEC, VALS(s1ap_NotifyFlag_vals), 0,
        NULL, HFILL }},
    { &hf_s1ap_tooEarlyInterRATHOReportFromEUTRAN,
      { "tooEarlyInterRATHOReportFromEUTRAN", "s1ap.tooEarlyInterRATHOReportFromEUTRAN_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TooEarlyInterRATHOReportReportFromEUTRAN", HFILL }},
    { &hf_s1ap_uERLFReportContainer,
      { "uERLFReportContainer", "s1ap.uERLFReportContainer",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING", HFILL }},
    { &hf_s1ap_mobilityInformation,
      { "mobilityInformation", "s1ap.mobilityInformation",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_dL_EHRPD_CompositeAvailableCapacity,
      { "dL-EHRPD-CompositeAvailableCapacity", "s1ap.dL_EHRPD_CompositeAvailableCapacity_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "EHRPDCompositeAvailableCapacity", HFILL }},
    { &hf_s1ap_uL_EHRPD_CompositeAvailableCapacity,
      { "uL-EHRPD-CompositeAvailableCapacity", "s1ap.uL_EHRPD_CompositeAvailableCapacity_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "EHRPDCompositeAvailableCapacity", HFILL }},
    { &hf_s1ap_eHRPDSectorCapacityClassValue,
      { "eHRPDSectorCapacityClassValue", "s1ap.eHRPDSectorCapacityClassValue",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_eHRPDCapacityValue,
      { "eHRPDCapacityValue", "s1ap.eHRPDCapacityValue",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_eHRPD_Sector_ID_01,
      { "eHRPD-Sector-ID", "s1ap.eHRPD_Sector_ID",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_s1ap_eHRPDSectorLoadReportingResponse,
      { "eHRPDSectorLoadReportingResponse", "s1ap.eHRPDSectorLoadReportingResponse_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},

/*--- End of included file: packet-s1ap-hfarr.c ---*/
#line 252 "./asn1/s1ap/packet-s1ap-template.c"
  };

  /* List of subtrees */
  static gint *ett[] = {
		  &ett_s1ap,
		  &ett_s1ap_TransportLayerAddress,
		  &ett_s1ap_ToTargetTransparentContainer,
		  &ett_s1ap_ToSourceTransparentContainer,
		  &ett_s1ap_RRCContainer,
		  &ett_s1ap_UERadioCapability,
		  &ett_s1ap_RIMInformation,
          &ett_s1ap_Cdma2000PDU,
          &ett_s1ap_Cdma2000SectorID,

/*--- Included file: packet-s1ap-ettarr.c ---*/
#line 1 "./asn1/s1ap/packet-s1ap-ettarr.c"
    &ett_s1ap_PrivateIE_ID,
    &ett_s1ap_ProtocolIE_Container,
    &ett_s1ap_ProtocolIE_Field,
    &ett_s1ap_ProtocolIE_ContainerList,
    &ett_s1ap_ProtocolExtensionContainer,
    &ett_s1ap_ProtocolExtensionField,
    &ett_s1ap_PrivateIE_Container,
    &ett_s1ap_PrivateIE_Field,
    &ett_s1ap_Additional_GUTI,
    &ett_s1ap_AreaScopeOfMDT,
    &ett_s1ap_AllocationAndRetentionPriority,
    &ett_s1ap_Bearers_SubjectToStatusTransferList,
    &ett_s1ap_Bearers_SubjectToStatusTransfer_Item,
    &ett_s1ap_BPLMNs,
    &ett_s1ap_BroadcastCancelledAreaList,
    &ett_s1ap_BroadcastCompletedAreaList,
    &ett_s1ap_CancelledCellinEAI,
    &ett_s1ap_CancelledCellinEAI_Item,
    &ett_s1ap_CancelledCellinTAI,
    &ett_s1ap_CancelledCellinTAI_Item,
    &ett_s1ap_Cause,
    &ett_s1ap_CellID_Broadcast,
    &ett_s1ap_CellID_Broadcast_Item,
    &ett_s1ap_CellID_Cancelled,
    &ett_s1ap_CellID_Cancelled_Item,
    &ett_s1ap_CellBasedMDT,
    &ett_s1ap_CellIdListforMDT,
    &ett_s1ap_Cdma2000OneXSRVCCInfo,
    &ett_s1ap_CellType,
    &ett_s1ap_CGI,
    &ett_s1ap_CSG_IdList,
    &ett_s1ap_CSG_IdList_Item,
    &ett_s1ap_COUNTvalue,
    &ett_s1ap_COUNTValueExtended,
    &ett_s1ap_COUNTvaluePDCP_SNlength18,
    &ett_s1ap_CriticalityDiagnostics,
    &ett_s1ap_CriticalityDiagnostics_IE_List,
    &ett_s1ap_CriticalityDiagnostics_IE_Item,
    &ett_s1ap_ECGIList,
    &ett_s1ap_PWSfailedECGIList,
    &ett_s1ap_EmergencyAreaIDList,
    &ett_s1ap_EmergencyAreaID_Broadcast,
    &ett_s1ap_EmergencyAreaID_Broadcast_Item,
    &ett_s1ap_EmergencyAreaID_Cancelled,
    &ett_s1ap_EmergencyAreaID_Cancelled_Item,
    &ett_s1ap_CompletedCellinEAI,
    &ett_s1ap_CompletedCellinEAI_Item,
    &ett_s1ap_ECGI_List,
    &ett_s1ap_EmergencyAreaIDListForRestart,
    &ett_s1ap_ENB_ID,
    &ett_s1ap_GERAN_Cell_ID,
    &ett_s1ap_Global_ENB_ID,
    &ett_s1ap_GUMMEIList,
    &ett_s1ap_ENB_StatusTransfer_TransparentContainer,
    &ett_s1ap_ENBX2TLAs,
    &ett_s1ap_EPLMNs,
    &ett_s1ap_E_RABInformationList,
    &ett_s1ap_E_RABInformationListItem,
    &ett_s1ap_E_RABList,
    &ett_s1ap_E_RABItem,
    &ett_s1ap_E_RABLevelQoSParameters,
    &ett_s1ap_EUTRAN_CGI,
    &ett_s1ap_ExpectedUEBehaviour,
    &ett_s1ap_ExpectedUEActivityBehaviour,
    &ett_s1ap_ForbiddenTAs,
    &ett_s1ap_ForbiddenTAs_Item,
    &ett_s1ap_ForbiddenTACs,
    &ett_s1ap_ForbiddenLAs,
    &ett_s1ap_ForbiddenLAs_Item,
    &ett_s1ap_ForbiddenLACs,
    &ett_s1ap_GBR_QosInformation,
    &ett_s1ap_GUMMEI,
    &ett_s1ap_HandoverRestrictionList,
    &ett_s1ap_ImmediateMDT,
    &ett_s1ap_LAI,
    &ett_s1ap_LastVisitedCell_Item,
    &ett_s1ap_LastVisitedEUTRANCellInformation,
    &ett_s1ap_LastVisitedGERANCellInformation,
    &ett_s1ap_ListeningSubframePattern,
    &ett_s1ap_LoggedMDT,
    &ett_s1ap_LoggedMBSFNMDT,
    &ett_s1ap_M3Configuration,
    &ett_s1ap_M4Configuration,
    &ett_s1ap_M5Configuration,
    &ett_s1ap_M6Configuration,
    &ett_s1ap_M7Configuration,
    &ett_s1ap_MDT_Configuration,
    &ett_s1ap_MBSFN_ResultToLog,
    &ett_s1ap_MBSFN_ResultToLogInfo,
    &ett_s1ap_MDTPLMNList,
    &ett_s1ap_MDTMode,
    &ett_s1ap_MeasurementThresholdA2,
    &ett_s1ap_MutingPatternInformation,
    &ett_s1ap_OverloadResponse,
    &ett_s1ap_M1PeriodicReporting,
    &ett_s1ap_ProSeAuthorized,
    &ett_s1ap_RequestType,
    &ett_s1ap_RIMTransfer,
    &ett_s1ap_RIMRoutingAddress,
    &ett_s1ap_RLFReportInformation,
    &ett_s1ap_ECGIListForRestart,
    &ett_s1ap_SecurityContext,
    &ett_s1ap_SONInformation,
    &ett_s1ap_SONInformationReply,
    &ett_s1ap_SONInformationReport,
    &ett_s1ap_SONConfigurationTransfer,
    &ett_s1ap_SynchronisationInformation,
    &ett_s1ap_SourceeNB_ID,
    &ett_s1ap_SourceeNB_ToTargeteNB_TransparentContainer,
    &ett_s1ap_ServedGUMMEIs,
    &ett_s1ap_ServedGUMMEIsItem,
    &ett_s1ap_ServedGroupIDs,
    &ett_s1ap_ServedMMECs,
    &ett_s1ap_ServedPLMNs,
    &ett_s1ap_SupportedTAs,
    &ett_s1ap_SupportedTAs_Item,
    &ett_s1ap_TimeSynchronisationInfo,
    &ett_s1ap_S_TMSI,
    &ett_s1ap_TAIBasedMDT,
    &ett_s1ap_TAIListforMDT,
    &ett_s1ap_TAIListforWarning,
    &ett_s1ap_TAI,
    &ett_s1ap_TAI_Broadcast,
    &ett_s1ap_TAI_Broadcast_Item,
    &ett_s1ap_TAI_Cancelled,
    &ett_s1ap_TAI_Cancelled_Item,
    &ett_s1ap_TABasedMDT,
    &ett_s1ap_TAListforMDT,
    &ett_s1ap_CompletedCellinTAI,
    &ett_s1ap_CompletedCellinTAI_Item,
    &ett_s1ap_TargetID,
    &ett_s1ap_TargeteNB_ID,
    &ett_s1ap_TargetRNC_ID,
    &ett_s1ap_TargeteNB_ToSourceeNB_TransparentContainer,
    &ett_s1ap_M1ThresholdEventA2,
    &ett_s1ap_TransportInformation,
    &ett_s1ap_TraceActivation,
    &ett_s1ap_TunnelInformation,
    &ett_s1ap_TAIListForRestart,
    &ett_s1ap_UEAggregateMaximumBitrate,
    &ett_s1ap_UE_S1AP_IDs,
    &ett_s1ap_UE_S1AP_ID_pair,
    &ett_s1ap_UE_associatedLogicalS1_ConnectionItem,
    &ett_s1ap_UE_HistoryInformation,
    &ett_s1ap_UEPagingID,
    &ett_s1ap_UESecurityCapabilities,
    &ett_s1ap_UserLocationInformation,
    &ett_s1ap_WarningAreaList,
    &ett_s1ap_X2TNLConfigurationInfo,
    &ett_s1ap_ENBX2ExtTLAs,
    &ett_s1ap_ENBX2ExtTLA,
    &ett_s1ap_ENBX2GTPTLAs,
    &ett_s1ap_ENBIndirectX2TransportLayerAddresses,
    &ett_s1ap_HandoverRequired,
    &ett_s1ap_HandoverCommand,
    &ett_s1ap_E_RABDataForwardingItem,
    &ett_s1ap_HandoverPreparationFailure,
    &ett_s1ap_HandoverRequest,
    &ett_s1ap_E_RABToBeSetupItemHOReq,
    &ett_s1ap_HandoverRequestAcknowledge,
    &ett_s1ap_E_RABAdmittedItem,
    &ett_s1ap_E_RABFailedToSetupItemHOReqAck,
    &ett_s1ap_HandoverFailure,
    &ett_s1ap_HandoverNotify,
    &ett_s1ap_PathSwitchRequest,
    &ett_s1ap_E_RABToBeSwitchedDLItem,
    &ett_s1ap_PathSwitchRequestAcknowledge,
    &ett_s1ap_E_RABToBeSwitchedULItem,
    &ett_s1ap_PathSwitchRequestFailure,
    &ett_s1ap_HandoverCancel,
    &ett_s1ap_HandoverCancelAcknowledge,
    &ett_s1ap_E_RABSetupRequest,
    &ett_s1ap_E_RABToBeSetupListBearerSUReq,
    &ett_s1ap_E_RABToBeSetupItemBearerSUReq,
    &ett_s1ap_E_RABSetupResponse,
    &ett_s1ap_E_RABSetupListBearerSURes,
    &ett_s1ap_E_RABSetupItemBearerSURes,
    &ett_s1ap_E_RABModifyRequest,
    &ett_s1ap_E_RABToBeModifiedListBearerModReq,
    &ett_s1ap_E_RABToBeModifiedItemBearerModReq,
    &ett_s1ap_E_RABModifyResponse,
    &ett_s1ap_E_RABModifyListBearerModRes,
    &ett_s1ap_E_RABModifyItemBearerModRes,
    &ett_s1ap_E_RABReleaseCommand,
    &ett_s1ap_E_RABReleaseResponse,
    &ett_s1ap_E_RABReleaseListBearerRelComp,
    &ett_s1ap_E_RABReleaseItemBearerRelComp,
    &ett_s1ap_E_RABReleaseIndication,
    &ett_s1ap_InitialContextSetupRequest,
    &ett_s1ap_E_RABToBeSetupListCtxtSUReq,
    &ett_s1ap_E_RABToBeSetupItemCtxtSUReq,
    &ett_s1ap_InitialContextSetupResponse,
    &ett_s1ap_E_RABSetupListCtxtSURes,
    &ett_s1ap_E_RABSetupItemCtxtSURes,
    &ett_s1ap_InitialContextSetupFailure,
    &ett_s1ap_Paging,
    &ett_s1ap_TAIList,
    &ett_s1ap_TAIItem,
    &ett_s1ap_UEContextReleaseRequest,
    &ett_s1ap_UEContextReleaseCommand,
    &ett_s1ap_UEContextReleaseComplete,
    &ett_s1ap_UEContextModificationRequest,
    &ett_s1ap_UEContextModificationResponse,
    &ett_s1ap_UEContextModificationFailure,
    &ett_s1ap_UERadioCapabilityMatchRequest,
    &ett_s1ap_UERadioCapabilityMatchResponse,
    &ett_s1ap_DownlinkNASTransport,
    &ett_s1ap_InitialUEMessage,
    &ett_s1ap_UplinkNASTransport,
    &ett_s1ap_NASNonDeliveryIndication,
    &ett_s1ap_RerouteNASRequest,
    &ett_s1ap_Reset,
    &ett_s1ap_ResetType,
    &ett_s1ap_UE_associatedLogicalS1_ConnectionListRes,
    &ett_s1ap_ResetAcknowledge,
    &ett_s1ap_UE_associatedLogicalS1_ConnectionListResAck,
    &ett_s1ap_ErrorIndication,
    &ett_s1ap_S1SetupRequest,
    &ett_s1ap_S1SetupResponse,
    &ett_s1ap_S1SetupFailure,
    &ett_s1ap_ENBConfigurationUpdate,
    &ett_s1ap_ENBConfigurationUpdateAcknowledge,
    &ett_s1ap_ENBConfigurationUpdateFailure,
    &ett_s1ap_MMEConfigurationUpdate,
    &ett_s1ap_MMEConfigurationUpdateAcknowledge,
    &ett_s1ap_MMEConfigurationUpdateFailure,
    &ett_s1ap_DownlinkS1cdma2000tunnelling,
    &ett_s1ap_UplinkS1cdma2000tunnelling,
    &ett_s1ap_UECapabilityInfoIndication,
    &ett_s1ap_ENBStatusTransfer,
    &ett_s1ap_MMEStatusTransfer,
    &ett_s1ap_TraceStart,
    &ett_s1ap_TraceFailureIndication,
    &ett_s1ap_DeactivateTrace,
    &ett_s1ap_CellTrafficTrace,
    &ett_s1ap_LocationReportingControl,
    &ett_s1ap_LocationReportingFailureIndication,
    &ett_s1ap_LocationReport,
    &ett_s1ap_OverloadStart,
    &ett_s1ap_OverloadStop,
    &ett_s1ap_WriteReplaceWarningRequest,
    &ett_s1ap_WriteReplaceWarningResponse,
    &ett_s1ap_ENBDirectInformationTransfer,
    &ett_s1ap_Inter_SystemInformationTransferType,
    &ett_s1ap_MMEDirectInformationTransfer,
    &ett_s1ap_ENBConfigurationTransfer,
    &ett_s1ap_MMEConfigurationTransfer,
    &ett_s1ap_PrivateMessage,
    &ett_s1ap_KillRequest,
    &ett_s1ap_KillResponse,
    &ett_s1ap_PWSRestartIndication,
    &ett_s1ap_PWSFailureIndication,
    &ett_s1ap_DownlinkUEAssociatedLPPaTransport,
    &ett_s1ap_UplinkUEAssociatedLPPaTransport,
    &ett_s1ap_DownlinkNonUEAssociatedLPPaTransport,
    &ett_s1ap_UplinkNonUEAssociatedLPPaTransport,
    &ett_s1ap_E_RABModificationIndication,
    &ett_s1ap_E_RABToBeModifiedItemBearerModInd,
    &ett_s1ap_E_RABNotToBeModifiedItemBearerModInd,
    &ett_s1ap_CSGMembershipInfo,
    &ett_s1ap_E_RABModificationConfirm,
    &ett_s1ap_E_RABModifyListBearerModConf,
    &ett_s1ap_E_RABModifyItemBearerModConf,
    &ett_s1ap_UEContextModificationIndication,
    &ett_s1ap_UEContextModificationConfirm,
    &ett_s1ap_S1AP_PDU,
    &ett_s1ap_InitiatingMessage,
    &ett_s1ap_SuccessfulOutcome,
    &ett_s1ap_UnsuccessfulOutcome,
    &ett_s1ap_SONtransferRequestContainer,
    &ett_s1ap_SONtransferResponseContainer,
    &ett_s1ap_SONtransferCause,
    &ett_s1ap_CellLoadReportingResponse,
    &ett_s1ap_EUTRANcellLoadReportingResponse,
    &ett_s1ap_EUTRANResponse,
    &ett_s1ap_IRAT_Cell_ID,
    &ett_s1ap_RequestedCellList,
    &ett_s1ap_MultiCellLoadReportingRequest,
    &ett_s1ap_ReportingCellList_Item,
    &ett_s1ap_ReportingCellList,
    &ett_s1ap_MultiCellLoadReportingResponse,
    &ett_s1ap_MultiCellLoadReportingResponse_Item,
    &ett_s1ap_EventTriggeredCellLoadReportingRequest,
    &ett_s1ap_EventTriggeredCellLoadReportingResponse,
    &ett_s1ap_HOReport,
    &ett_s1ap_CandidateCellList,
    &ett_s1ap_CandidatePCIList,
    &ett_s1ap_CandidatePCI,
    &ett_s1ap_CellActivationRequest,
    &ett_s1ap_CellsToActivateList,
    &ett_s1ap_CellsToActivateList_Item,
    &ett_s1ap_CellActivationResponse,
    &ett_s1ap_ActivatedCellsList,
    &ett_s1ap_ActivatedCellsList_Item,
    &ett_s1ap_CellStateIndication,
    &ett_s1ap_NotificationCellList,
    &ett_s1ap_NotificationCellList_Item,
    &ett_s1ap_FailureEventReport,
    &ett_s1ap_TooEarlyInterRATHOReportReportFromEUTRAN,
    &ett_s1ap_EHRPDSectorLoadReportingResponse,
    &ett_s1ap_EHRPDCompositeAvailableCapacity,
    &ett_s1ap_EHRPDMultiSectorLoadReportingResponseItem,

/*--- End of included file: packet-s1ap-ettarr.c ---*/
#line 266 "./asn1/s1ap/packet-s1ap-template.c"
  };

  module_t *s1ap_module;

  /* Register protocol */
  proto_s1ap = proto_register_protocol(PNAME, PSNAME, PFNAME);
  /* Register fields and subtrees */
  proto_register_field_array(proto_s1ap, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

  /* Register dissector */
  s1ap_handle = register_dissector("s1ap", dissect_s1ap, proto_s1ap);

  /* Register dissector tables */
  s1ap_ies_dissector_table = register_dissector_table("s1ap.ies", "S1AP-PROTOCOL-IES", proto_s1ap, FT_UINT32, BASE_DEC);
  s1ap_ies_p1_dissector_table = register_dissector_table("s1ap.ies.pair.first", "S1AP-PROTOCOL-IES-PAIR FirstValue", proto_s1ap, FT_UINT32, BASE_DEC);
  s1ap_ies_p2_dissector_table = register_dissector_table("s1ap.ies.pair.second", "S1AP-PROTOCOL-IES-PAIR SecondValue", proto_s1ap, FT_UINT32, BASE_DEC);
  s1ap_extension_dissector_table = register_dissector_table("s1ap.extension", "S1AP-PROTOCOL-EXTENSION", proto_s1ap, FT_UINT32, BASE_DEC);
  s1ap_proc_imsg_dissector_table = register_dissector_table("s1ap.proc.imsg", "S1AP-ELEMENTARY-PROCEDURE InitiatingMessage", proto_s1ap, FT_UINT32, BASE_DEC);
  s1ap_proc_sout_dissector_table = register_dissector_table("s1ap.proc.sout", "S1AP-ELEMENTARY-PROCEDURE SuccessfulOutcome", proto_s1ap, FT_UINT32, BASE_DEC);
  s1ap_proc_uout_dissector_table = register_dissector_table("s1ap.proc.uout", "S1AP-ELEMENTARY-PROCEDURE UnsuccessfulOutcome", proto_s1ap, FT_UINT32, BASE_DEC);

  /* Register configuration options for ports */
  s1ap_module = prefs_register_protocol(proto_s1ap, proto_reg_handoff_s1ap);

  prefs_register_uint_preference(s1ap_module, "sctp.port",
                                 "S1AP SCTP Port",
                                 "Set the SCTP port for S1AP messages",
                                 10,
                                 &gbl_s1apSctpPort);
  prefs_register_bool_preference(s1ap_module, "dissect_container", "Dissect TransparentContainer", "Dissect TransparentContainers that are opaque to S1AP", &g_s1ap_dissect_container);

}





