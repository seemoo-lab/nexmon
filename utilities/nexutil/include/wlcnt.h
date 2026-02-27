/***************************************************************************
 *                                                                         *
 *          ###########   ###########   ##########    ##########           *
 *         ############  ############  ############  ############          *
 *         ##            ##            ##   ##   ##  ##        ##          *
 *         ##            ##            ##   ##   ##  ##        ##          *
 *         ###########   ####  ######  ##   ##   ##  ##    ######          *
 *          ###########  ####  #       ##   ##   ##  ##    #    #          *
 *                   ##  ##    ######  ##   ##   ##  ##    #    #          *
 *                   ##  ##    #       ##   ##   ##  ##    #    #          *
 *         ############  ##### ######  ##   ##   ##  ##### ######          *
 *         ###########    ###########  ##   ##   ##   ##########           *
 *                                                                         *
 *            S E C U R E   M O B I L E   N E T W O R K I N G              *
 *                                                                         *
 * This file is part of NexMon.                                            *
 *                                                                         *
 * Copyright (c) 2016 NexMon Team                                          *
 *                                                                         *
 * NexMon is free software: you can redistribute it and/or modify          *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation, either version 3 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * NexMon is distributed in the hope that it will be useful,               *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with NexMon. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 **************************************************************************/

typedef unsigned int uint32;
typedef unsigned short uint16;

// source of wl_cnt_t: http://svn.dd-wrt.com/browser/src/linux/universal/linux-3.10/brcm/arm/include/wlioctl.h?rev=23022

#define NFIFO           6   /* # tx/rx fifopairs */

#define WL_CNT_T_VERSION    9   /* current version of wl_cnt_t struct */

typedef struct {
    uint16  version;    /* see definition of WL_CNT_T_VERSION */
    uint16  length;     /* length of entire structure */

    /* transmit stat counters */
    uint32  txframe;    /* tx data frames */
    uint32  txbyte;     /* tx data bytes */
    uint32  txretrans;  /* tx mac retransmits */
    uint32  txerror;    /* tx data errors (derived: sum of others) */
    uint32  txctl;      /* tx management frames */
    uint32  txprshort;  /* tx short preamble frames */
    uint32  txserr;     /* tx status errors */
    uint32  txnobuf;    /* tx out of buffers errors */
    uint32  txnoassoc;  /* tx discard because we're not associated */
    uint32  txrunt;     /* tx runt frames */
    uint32  txchit;     /* tx header cache hit (fastpath) */
    uint32  txcmiss;    /* tx header cache miss (slowpath) */

    /* transmit chip error counters */
    uint32  txuflo;     /* tx fifo underflows */
    uint32  txphyerr;   /* tx phy errors (indicated in tx status) */
    uint32  txphycrs;

    /* receive stat counters */
    uint32  rxframe;    /* rx data frames */
    uint32  rxbyte;     /* rx data bytes */
    uint32  rxerror;    /* rx data errors (derived: sum of others) */
    uint32  rxctl;      /* rx management frames */
    uint32  rxnobuf;    /* rx out of buffers errors */
    uint32  rxnondata;  /* rx non data frames in the data channel errors */
    uint32  rxbadds;    /* rx bad DS errors */
    uint32  rxbadcm;    /* rx bad control or management frames */
    uint32  rxfragerr;  /* rx fragmentation errors */
    uint32  rxrunt;     /* rx runt frames */
    uint32  rxgiant;    /* rx giant frames */
    uint32  rxnoscb;    /* rx no scb error */
    uint32  rxbadproto; /* rx invalid frames */
    uint32  rxbadsrcmac;    /* rx frames with Invalid Src Mac */
    uint32  rxbadda;    /* rx frames tossed for invalid da */
    uint32  rxfilter;   /* rx frames filtered out */

    /* receive chip error counters */
    uint32  rxoflo;     /* rx fifo overflow errors */
    uint32  rxuflo[NFIFO];  /* rx dma descriptor underflow errors */

    uint32  d11cnt_txrts_off;   /* d11cnt txrts value when reset d11cnt */
    uint32  d11cnt_rxcrc_off;   /* d11cnt rxcrc value when reset d11cnt */
    uint32  d11cnt_txnocts_off; /* d11cnt txnocts value when reset d11cnt */

    /* misc counters */
    uint32  dmade;      /* tx/rx dma descriptor errors */
    uint32  dmada;      /* tx/rx dma data errors */
    uint32  dmape;      /* tx/rx dma descriptor protocol errors */
    uint32  reset;      /* reset count */
    uint32  tbtt;       /* cnts the TBTT int's */
    uint32  txdmawar;
    uint32  pkt_callback_reg_fail;  /* callbacks register failure */

    /* MAC counters: 32-bit version of d11.h's macstat_t */
    uint32  txallfrm;   /* total number of frames sent, incl. Data, ACK, RTS, CTS,
                 * Control Management (includes retransmissions)
                 */
    uint32  txrtsfrm;   /* number of RTS sent out by the MAC */
    uint32  txctsfrm;   /* number of CTS sent out by the MAC */
    uint32  txackfrm;   /* number of ACK frames sent out */
    uint32  txdnlfrm;   /* Not used */
    uint32  txbcnfrm;   /* beacons transmitted */
    uint32  txfunfl[6]; /* per-fifo tx underflows */
    uint32  rxtoolate;  /* receive too late */
    uint32  txfbw;      /* transmit at fallback bw (dynamic bw) */
    uint32  txtplunfl;  /* Template underflows (mac was too slow to transmit ACK/CTS
                 * or BCN)
                 */
    uint32  txphyerror; /* Transmit phy error, type of error is reported in tx-status for
                 * driver enqueued frames
                 */
    uint32  rxfrmtoolong;   /* Received frame longer than legal limit (2346 bytes) */
    uint32  rxfrmtooshrt;   /* Received frame did not contain enough bytes for its frame type */
    uint32  rxinvmachdr;    /* Either the protocol version != 0 or frame type not
                 * data/control/management
                 */
    uint32  rxbadfcs;   /* number of frames for which the CRC check failed in the MAC */
    uint32  rxbadplcp;  /* parity check of the PLCP header failed */
    uint32  rxcrsglitch;    /* PHY was able to correlate the preamble but not the header */
    uint32  rxstrt;     /* Number of received frames with a good PLCP
                 * (i.e. passing parity check)
                 */
    uint32  rxdfrmucastmbss; /* Number of received DATA frames with good FCS and matching RA */
    uint32  rxmfrmucastmbss; /* number of received mgmt frames with good FCS and matching RA */
    uint32  rxcfrmucast;    /* number of received CNTRL frames with good FCS and matching RA */
    uint32  rxrtsucast; /* number of unicast RTS addressed to the MAC (good FCS) */
    uint32  rxctsucast; /* number of unicast CTS addressed to the MAC (good FCS) */
    uint32  rxackucast; /* number of ucast ACKS received (good FCS) */
    uint32  rxdfrmocast;    /* number of received DATA frames (good FCS and not matching RA) */
    uint32  rxmfrmocast;    /* number of received MGMT frames (good FCS and not matching RA) */
    uint32  rxcfrmocast;    /* number of received CNTRL frame (good FCS and not matching RA) */
    uint32  rxrtsocast; /* number of received RTS not addressed to the MAC */
    uint32  rxctsocast; /* number of received CTS not addressed to the MAC */
    uint32  rxdfrmmcast;    /* number of RX Data multicast frames received by the MAC */
    uint32  rxmfrmmcast;    /* number of RX Management multicast frames received by the MAC */
    uint32  rxcfrmmcast;    /* number of RX Control multicast frames received by the MAC
                 * (unlikely to see these)
                 */
    uint32  rxbeaconmbss;   /* beacons received from member of BSS */
    uint32  rxdfrmucastobss; /* number of unicast frames addressed to the MAC from
                  * other BSS (WDS FRAME)
                  */
    uint32  rxbeaconobss;   /* beacons received from other BSS */
    uint32  rxrsptmout; /* Number of response timeouts for transmitted frames
                 * expecting a response
                 */
    uint32  bcntxcancl; /* transmit beacons canceled due to receipt of beacon (IBSS) */
    uint32  rxf0ovfl;   /* Number of receive fifo 0 overflows */
    uint32  rxf1ovfl;   /* Number of receive fifo 1 overflows (obsolete) */
    uint32  rxf2ovfl;   /* Number of receive fifo 2 overflows (obsolete) */
    uint32  txsfovfl;   /* Number of transmit status fifo overflows (obsolete) */
    uint32  pmqovfl;    /* Number of PMQ overflows */
    uint32  rxcgprqfrm; /* Number of received Probe requests that made it into
                 * the PRQ fifo
                 */
    uint32  rxcgprsqovfl;   /* Rx Probe Request Que overflow in the AP */
    uint32  txcgprsfail;    /* Tx Probe Response Fail. AP sent probe response but did
                 * not get ACK
                 */
    uint32  txcgprssuc; /* Tx Probe Response Success (ACK was received) */
    uint32  prs_timeout;    /* Number of probe requests that were dropped from the PRQ
                 * fifo because a probe response could not be sent out within
                 * the time limit defined in M_PRS_MAXTIME
                 */
    uint32  rxnack;     /* obsolete */
    uint32  frmscons;   /* obsolete */
    uint32  txnack;     /* obsolete */
    uint32  rxback;     /* blockack rxcnt */
    uint32  txback;     /* blockack txcnt */

    /* 802.11 MIB counters, pp. 614 of 802.11 reaff doc. */
    uint32  txfrag;     /* dot11TransmittedFragmentCount */
    uint32  txmulti;    /* dot11MulticastTransmittedFrameCount */
    uint32  txfail;     /* dot11FailedCount */
    uint32  txretry;    /* dot11RetryCount */
    uint32  txretrie;   /* dot11MultipleRetryCount */
    uint32  rxdup;      /* dot11FrameduplicateCount */
    uint32  txrts;      /* dot11RTSSuccessCount */
    uint32  txnocts;    /* dot11RTSFailureCount */
    uint32  txnoack;    /* dot11ACKFailureCount */
    uint32  rxfrag;     /* dot11ReceivedFragmentCount */
    uint32  rxmulti;    /* dot11MulticastReceivedFrameCount */
    uint32  rxcrc;      /* dot11FCSErrorCount */
    uint32  txfrmsnt;   /* dot11TransmittedFrameCount (bogus MIB?) */
    uint32  rxundec;    /* dot11WEPUndecryptableCount */

    /* WPA2 counters (see rxundec for DecryptFailureCount) */
    uint32  tkipmicfaill;   /* TKIPLocalMICFailures */
    uint32  tkipcntrmsr;    /* TKIPCounterMeasuresInvoked */
    uint32  tkipreplay; /* TKIPReplays */
    uint32  ccmpfmterr; /* CCMPFormatErrors */
    uint32  ccmpreplay; /* CCMPReplays */
    uint32  ccmpundec;  /* CCMPDecryptErrors */
    uint32  fourwayfail;    /* FourWayHandshakeFailures */
    uint32  wepundec;   /* dot11WEPUndecryptableCount */
    uint32  wepicverr;  /* dot11WEPICVErrorCount */
    uint32  decsuccess; /* DecryptSuccessCount */
    uint32  tkipicverr; /* TKIPICVErrorCount */
    uint32  wepexcluded;    /* dot11WEPExcludedCount */

    uint32  txchanrej;  /* Tx frames suppressed due to channel rejection */
    uint32  psmwds;     /* Count PSM watchdogs */
    uint32  phywatchdog;    /* Count Phy watchdogs (triggered by ucode) */

    /* MBSS counters, AP only */
    uint32  prq_entries_handled;    /* PRQ entries read in */
    uint32  prq_undirected_entries; /*    which were bcast bss & ssid */
    uint32  prq_bad_entries;    /*    which could not be translated to info */
    uint32  atim_suppress_count;    /* TX suppressions on ATIM fifo */
    uint32  bcn_template_not_ready; /* Template marked in use on send bcn ... */
    uint32  bcn_template_not_ready_done; /* ...but "DMA done" interrupt rcvd */
    uint32  late_tbtt_dpc;  /* TBTT DPC did not happen in time */

    /* per-rate receive stat counters */
    uint32  rx1mbps;    /* packets rx at 1Mbps */
    uint32  rx2mbps;    /* packets rx at 2Mbps */
    uint32  rx5mbps5;   /* packets rx at 5.5Mbps */
    uint32  rx6mbps;    /* packets rx at 6Mbps */
    uint32  rx9mbps;    /* packets rx at 9Mbps */
    uint32  rx11mbps;   /* packets rx at 11Mbps */
    uint32  rx12mbps;   /* packets rx at 12Mbps */
    uint32  rx18mbps;   /* packets rx at 18Mbps */
    uint32  rx24mbps;   /* packets rx at 24Mbps */
    uint32  rx36mbps;   /* packets rx at 36Mbps */
    uint32  rx48mbps;   /* packets rx at 48Mbps */
    uint32  rx54mbps;   /* packets rx at 54Mbps */
    uint32  rx108mbps;  /* packets rx at 108mbps */
    uint32  rx162mbps;  /* packets rx at 162mbps */
    uint32  rx216mbps;  /* packets rx at 216 mbps */
    uint32  rx270mbps;  /* packets rx at 270 mbps */
    uint32  rx324mbps;  /* packets rx at 324 mbps */
    uint32  rx378mbps;  /* packets rx at 378 mbps */
    uint32  rx432mbps;  /* packets rx at 432 mbps */
    uint32  rx486mbps;  /* packets rx at 486 mbps */
    uint32  rx540mbps;  /* packets rx at 540 mbps */

    /* pkteng rx frame stats */
    uint32  pktengrxducast; /* unicast frames rxed by the pkteng code */
    uint32  pktengrxdmcast; /* multicast frames rxed by the pkteng code */

    uint32  rfdisable;  /* count of radio disables */
    uint32  bphy_rxcrsglitch;   /* PHY count of bphy glitches */
    uint32  bphy_badplcp;

    uint32  txexptime;  /* Tx frames suppressed due to timer expiration */

    uint32  txmpdu_sgi; /* count for sgi transmit */
    uint32  rxmpdu_sgi; /* count for sgi received */
    uint32  txmpdu_stbc;    /* count for stbc transmit */
    uint32  rxmpdu_stbc;    /* count for stbc received */

    uint32  rxundec_mcst;   /* dot11WEPUndecryptableCount */

    /* WPA2 counters (see rxundec for DecryptFailureCount) */
    uint32  tkipmicfaill_mcst;  /* TKIPLocalMICFailures */
    uint32  tkipcntrmsr_mcst;   /* TKIPCounterMeasuresInvoked */
    uint32  tkipreplay_mcst;    /* TKIPReplays */
    uint32  ccmpfmterr_mcst;    /* CCMPFormatErrors */
    uint32  ccmpreplay_mcst;    /* CCMPReplays */
    uint32  ccmpundec_mcst; /* CCMPDecryptErrors */
    uint32  fourwayfail_mcst;   /* FourWayHandshakeFailures */
    uint32  wepundec_mcst;  /* dot11WEPUndecryptableCount */
    uint32  wepicverr_mcst; /* dot11WEPICVErrorCount */
    uint32  decsuccess_mcst;    /* DecryptSuccessCount */
    uint32  tkipicverr_mcst;    /* TKIPICVErrorCount */
    uint32  wepexcluded_mcst;   /* dot11WEPExcludedCount */

    uint32  dma_hang;   /* count for dma hang */
    uint32  reinit;     /* count for reinit */

    uint32  pstatxucast;    /* count of ucast frames xmitted on all psta assoc */
    uint32  pstatxnoassoc;  /* count of txnoassoc frames xmitted on all psta assoc */
    uint32  pstarxucast;    /* count of ucast frames received on all psta assoc */
    uint32  pstarxbcmc; /* count of bcmc frames received on all psta */
    uint32  pstatxbcmc; /* count of bcmc frames transmitted on all psta */

    uint32  cso_passthrough; /* hw cso required but passthrough */
    uint32  cso_normal; /* hw cso hdr for normal process */
    uint32  chained;    /* number of frames chained */
    uint32  chainedsz1; /* number of chain size 1 frames */
    uint32  unchained;  /* number of frames not chained */
    uint32  maxchainsz; /* max chain size so far */
    uint32  currchainsz;    /* current chain size */
    uint32  rxdrop20s;  /* drop secondary cnt */
    uint32  pciereset;  /* Secondary Bus Reset issued by driver */
    uint32  cfgrestore; /* configspace restore by driver */
} wl_cnt_t;

char *wl_cnt_varname[] = {
    "version_and_length",

    /* transmit stat counters */
    "txframe",    /* tx data frames */
    "txbyte",     /* tx data bytes */
    "txretrans",  /* tx mac retransmits */
    "txerror",    /* tx data errors (derived: sum of others) */
    "txctl",      /* tx management frames */
    "txprshort",  /* tx short preamble frames */
    "txserr",     /* tx status errors */
    "txnobuf",    /* tx out of buffers errors */
    "txnoassoc",  /* tx discard because we're not associated */
    "txrunt",     /* tx runt frames */
    "txchit",     /* tx header cache hit (fastpath) */
    "txcmiss",    /* tx header cache miss (slowpath) */

    /* transmit chip error counters */
    "txuflo",     /* tx fifo underflows */
    "txphyerr",   /* tx phy errors (indicated in tx status) */
    "txphycrs",

    /* receive stat counters */
    "rxframe",    /* rx data frames */
    "rxbyte",     /* rx data bytes */
    "rxerror",    /* rx data errors (derived: sum of others) */
    "rxctl",      /* rx management frames */
    "rxnobuf",    /* rx out of buffers errors */
    "rxnondata",  /* rx non data frames in the data channel errors */
    "rxbadds",    /* rx bad DS errors */
    "rxbadcm",    /* rx bad control or management frames */
    "rxfragerr",  /* rx fragmentation errors */
    "rxrunt",     /* rx runt frames */
    "rxgiant",    /* rx giant frames */
    "rxnoscb",    /* rx no scb error */
    "rxbadproto", /* rx invalid frames */
    "rxbadsrcmac",    /* rx frames with Invalid Src Mac */
    "rxbadda",    /* rx frames tossed for invalid da */
    "rxfilter",   /* rx frames filtered out */

    /* receive chip error counters */
    "rxoflo",     /* rx fifo overflow errors */
    "rxuflo[0]",  /* rx dma descriptor underflow errors */
    "rxuflo[1]",  /* rx dma descriptor underflow errors */
    "rxuflo[2]",  /* rx dma descriptor underflow errors */
    "rxuflo[3]",  /* rx dma descriptor underflow errors */
    "rxuflo[4]",  /* rx dma descriptor underflow errors */
    "rxuflo[5]",  /* rx dma descriptor underflow errors */

    "d11cnt_txrts_off",   /* d11cnt txrts value when reset d11cnt */
    "d11cnt_rxcrc_off",   /* d11cnt rxcrc value when reset d11cnt */
    "d11cnt_txnocts_off", /* d11cnt txnocts value when reset d11cnt */

    /* misc counters */
    "dmade",      /* tx/rx dma descriptor errors */
    "dmada",      /* tx/rx dma data errors */
    "dmape",      /* tx/rx dma descriptor protocol errors */
    "reset",      /* reset count */
    "tbtt",       /* cnts the TBTT int's */
    "txdmawar",
    "pkt_callback_reg_fail",  /* callbacks register failure */

    /* MAC counters: 32-bit version of d11.h's macstat_t */
    "txallfrm",   /* total number of frames sent, incl. Data, ACK, RTS, CTS,
                 * Control Management (includes retransmissions)
                 */
    "txrtsfrm",   /* number of RTS sent out by the MAC */
    "txctsfrm",   /* number of CTS sent out by the MAC */
    "txackfrm",   /* number of ACK frames sent out */
    "txdnlfrm",   /* Not used */
    "txbcnfrm",   /* beacons transmitted */
    "txfunfl[0]", /* per-fifo tx underflows */
    "txfunfl[1]", /* per-fifo tx underflows */
    "txfunfl[2]", /* per-fifo tx underflows */
    "txfunfl[3]", /* per-fifo tx underflows */
    "txfunfl[4]", /* per-fifo tx underflows */
    "txfunfl[5]", /* per-fifo tx underflows */
    "rxtoolate",  /* receive too late */
    "txfbw",      /* transmit at fallback bw (dynamic bw) */
    "txtplunfl",  /* Template underflows (mac was too slow to transmit ACK/CTS
                 * or BCN)
                 */
    "txphyerror", /* Transmit phy error, type of error is reported in tx-status for
                 * driver enqueued frames
                 */
    "rxfrmtoolong",   /* Received frame longer than legal limit (2346 bytes) */
    "rxfrmtooshrt",   /* Received frame did not contain enough bytes for its frame type */
    "rxinvmachdr",    /* Either the protocol version != 0 or frame type not
                 * data/control/management
                 */
    "rxbadfcs",   /* number of frames for which the CRC check failed in the MAC */
    "rxbadplcp",  /* parity check of the PLCP header failed */
    "rxcrsglitch",    /* PHY was able to correlate the preamble but not the header */
    "rxstrt",     /* Number of received frames with a good PLCP
                 * (i.e. passing parity check)
                 */
    "rxdfrmucastmbss", /* Number of received DATA frames with good FCS and matching RA */
    "rxmfrmucastmbss", /* number of received mgmt frames with good FCS and matching RA */
    "rxcfrmucast",    /* number of received CNTRL frames with good FCS and matching RA */
    "rxrtsucast", /* number of unicast RTS addressed to the MAC (good FCS) */
    "rxctsucast", /* number of unicast CTS addressed to the MAC (good FCS) */
    "rxackucast", /* number of ucast ACKS received (good FCS) */
    "rxdfrmocast",    /* number of received DATA frames (good FCS and not matching RA) */
    "rxmfrmocast",    /* number of received MGMT frames (good FCS and not matching RA) */
    "rxcfrmocast",    /* number of received CNTRL frame (good FCS and not matching RA) */
    "rxrtsocast", /* number of received RTS not addressed to the MAC */
    "rxctsocast", /* number of received CTS not addressed to the MAC */
    "rxdfrmmcast",    /* number of RX Data multicast frames received by the MAC */
    "rxmfrmmcast",    /* number of RX Management multicast frames received by the MAC */
    "rxcfrmmcast",    /* number of RX Control multicast frames received by the MAC
                 * (unlikely to see these)
                 */
    "rxbeaconmbss",   /* beacons received from member of BSS */
    "rxdfrmucastobss", /* number of unicast frames addressed to the MAC from
                  * other BSS (WDS FRAME)
                  */
    "rxbeaconobss",   /* beacons received from other BSS */
    "rxrsptmout", /* Number of response timeouts for transmitted frames
                 * expecting a response
                 */
    "bcntxcancl", /* transmit beacons canceled due to receipt of beacon (IBSS) */
    "rxf0ovfl",   /* Number of receive fifo 0 overflows */
    "rxf1ovfl",   /* Number of receive fifo 1 overflows (obsolete) */
    "rxf2ovfl",   /* Number of receive fifo 2 overflows (obsolete) */
    "txsfovfl",   /* Number of transmit status fifo overflows (obsolete) */
    "pmqovfl",    /* Number of PMQ overflows */
    "rxcgprqfrm", /* Number of received Probe requests that made it into
                 * the PRQ fifo
                 */
    "rxcgprsqovfl",   /* Rx Probe Request Que overflow in the AP */
    "txcgprsfail",    /* Tx Probe Response Fail. AP sent probe response but did
                 * not get ACK
                 */
    "txcgprssuc", /* Tx Probe Response Success (ACK was received) */
    "prs_timeout",    /* Number of probe requests that were dropped from the PRQ
                 * fifo because a probe response could not be sent out within
                 * the time limit defined in M_PRS_MAXTIME
                 */
    "rxnack",     /* obsolete */
    "frmscons",   /* obsolete */
    "txnack",     /* obsolete */
    "rxback",     /* blockack rxcnt */
    "txback",     /* blockack txcnt */

    /* 802.11 MIB counters, pp. 614 of 802.11 reaff doc. */
    "txfrag",     /* dot11TransmittedFragmentCount */
    "txmulti",    /* dot11MulticastTransmittedFrameCount */
    "txfail",     /* dot11FailedCount */
    "txretry",    /* dot11RetryCount */
    "txretrie",   /* dot11MultipleRetryCount */
    "rxdup",      /* dot11FrameduplicateCount */
    "txrts",      /* dot11RTSSuccessCount */
    "txnocts",    /* dot11RTSFailureCount */
    "txnoack",    /* dot11ACKFailureCount */
    "rxfrag",     /* dot11ReceivedFragmentCount */
    "rxmulti",    /* dot11MulticastReceivedFrameCount */
    "rxcrc",      /* dot11FCSErrorCount */
    "txfrmsnt",   /* dot11TransmittedFrameCount (bogus MIB?) */
    "rxundec",    /* dot11WEPUndecryptableCount */

    /* WPA2 counters (see rxundec for DecryptFailureCount) */
    "tkipmicfaill",   /* TKIPLocalMICFailures */
    "tkipcntrmsr",    /* TKIPCounterMeasuresInvoked */
    "tkipreplay", /* TKIPReplays */
    "ccmpfmterr", /* CCMPFormatErrors */
    "ccmpreplay", /* CCMPReplays */
    "ccmpundec",  /* CCMPDecryptErrors */
    "fourwayfail",    /* FourWayHandshakeFailures */
    "wepundec",   /* dot11WEPUndecryptableCount */
    "wepicverr",  /* dot11WEPICVErrorCount */
    "decsuccess", /* DecryptSuccessCount */
    "tkipicverr", /* TKIPICVErrorCount */
    "wepexcluded",    /* dot11WEPExcludedCount */

    "txchanrej",  /* Tx frames suppressed due to channel rejection */
    "psmwds",     /* Count PSM watchdogs */
    "phywatchdog",    /* Count Phy watchdogs (triggered by ucode) */

    /* MBSS counters, AP only */
    "prq_entries_handled",    /* PRQ entries read in */
    "prq_undirected_entries", /*    which were bcast bss & ssid */
    "prq_bad_entries",    /*    which could not be translated to info */
    "atim_suppress_count",    /* TX suppressions on ATIM fifo */
    "bcn_template_not_ready", /* Template marked in use on send bcn ... */
    "bcn_template_not_ready_done", /* ...but "DMA done" interrupt rcvd */
    "late_tbtt_dpc",  /* TBTT DPC did not happen in time */

    /* per-rate receive stat counters */
    "rx1mbps",    /* packets rx at 1Mbps */
    "rx2mbps",    /* packets rx at 2Mbps */
    "rx5mbps5",   /* packets rx at 5.5Mbps */
    "rx6mbps",    /* packets rx at 6Mbps */
    "rx9mbps",    /* packets rx at 9Mbps */
    "rx11mbps",   /* packets rx at 11Mbps */
    "rx12mbps",   /* packets rx at 12Mbps */
    "rx18mbps",   /* packets rx at 18Mbps */
    "rx24mbps",   /* packets rx at 24Mbps */
    "rx36mbps",   /* packets rx at 36Mbps */
    "rx48mbps",   /* packets rx at 48Mbps */
    "rx54mbps",   /* packets rx at 54Mbps */
    "rx108mbps",  /* packets rx at 108mbps */
    "rx162mbps",  /* packets rx at 162mbps */
    "rx216mbps",  /* packets rx at 216 mbps */
    "rx270mbps",  /* packets rx at 270 mbps */
    "rx324mbps",  /* packets rx at 324 mbps */
    "rx378mbps",  /* packets rx at 378 mbps */
    "rx432mbps",  /* packets rx at 432 mbps */
    "rx486mbps",  /* packets rx at 486 mbps */
    "rx540mbps",  /* packets rx at 540 mbps */

    /* pkteng rx frame stats */
    "pktengrxducast", /* unicast frames rxed by the pkteng code */
    "pktengrxdmcast", /* multicast frames rxed by the pkteng code */

    "rfdisable",  /* count of radio disables */
    "bphy_rxcrsglitch",   /* PHY count of bphy glitches */
    "bphy_badplcp",

    "txexptime",  /* Tx frames suppressed due to timer expiration */

    "txmpdu_sgi", /* count for sgi transmit */
    "rxmpdu_sgi", /* count for sgi received */
    "txmpdu_stbc",    /* count for stbc transmit */
    "rxmpdu_stbc",    /* count for stbc received */

    "rxundec_mcst",   /* dot11WEPUndecryptableCount */

    /* WPA2 counters (see rxundec for DecryptFailureCount) */
    "tkipmicfaill_mcst",  /* TKIPLocalMICFailures */
    "tkipcntrmsr_mcst",   /* TKIPCounterMeasuresInvoked */
    "tkipreplay_mcst",    /* TKIPReplays */
    "ccmpfmterr_mcst",    /* CCMPFormatErrors */
    "ccmpreplay_mcst",    /* CCMPReplays */
    "ccmpundec_mcst", /* CCMPDecryptErrors */
    "fourwayfail_mcst",   /* FourWayHandshakeFailures */
    "wepundec_mcst",  /* dot11WEPUndecryptableCount */
    "wepicverr_mcst", /* dot11WEPICVErrorCount */
    "decsuccess_mcst",    /* DecryptSuccessCount */
    "tkipicverr_mcst",    /* TKIPICVErrorCount */
    "wepexcluded_mcst",   /* dot11WEPExcludedCount */

    "dma_hang",   /* count for dma hang */
    "reinit",     /* count for reinit */

    "pstatxucast",    /* count of ucast frames xmitted on all psta assoc */
    "pstatxnoassoc",  /* count of txnoassoc frames xmitted on all psta assoc */
    "pstarxucast",    /* count of ucast frames received on all psta assoc */
    "pstarxbcmc", /* count of bcmc frames received on all psta */
    "pstatxbcmc", /* count of bcmc frames transmitted on all psta */

    "cso_passthrough", /* hw cso required but passthrough */
    "cso_normal", /* hw cso hdr for normal process */
    "chained",    /* number of frames chained */
    "chainedsz1", /* number of chain size 1 frames */
    "unchained",  /* number of frames not chained */
    "maxchainsz", /* max chain size so far */
    "currchainsz",    /* current chain size */
    "rxdrop20s",  /* drop secondary cnt */
    "pciereset",  /* Secondary Bus Reset issued by driver */
    "cfgrestore", /* configspace restore by driver */
};

char *wl_cnt_description[] = {
    "see definition of WL_CNT_T_VERSION and length of entire structure",

    /* transmit stat counters */
    "tx data frames",
    "tx data bytes",
    "tx mac retransmits",
    "tx data errors (derived: sum of others)",
    "tx management frames",
    "tx short preamble frames",
    "tx status errors",
    "tx out of buffers errors",
    "tx discard because we're not associated",
    "tx runt frames",
    "tx header cache hit (fastpath)",
    "tx header cache miss (slowpath)",

    /* transmit chip error counters */
    "tx fifo underflows",
    "tx phy errors (indicated in tx status)",
    "txphycrs",

    /* receive stat counters */
    "rx data frames",
    "rx data bytes",
    "rx data errors (derived: sum of others)",
    "rx management frames",
    "rx out of buffers errors",
    "rx non data frames in the data channel errors",
    "rx bad DS errors",
    "rx bad control or management frames",
    "rx fragmentation errors",
    "rx runt frames",
    "rx giant frames",
    "rx no scb error",
    "rx invalid frames",
    "rx frames with Invalid Src Mac",
    "rx frames tossed for invalid da",
    "rx frames filtered out",

    /* receive chip error counters */
    "rx fifo overflow errors",
    "rx dma descriptor underflow errors",
    "rx dma descriptor underflow errors",
    "rx dma descriptor underflow errors",
    "rx dma descriptor underflow errors",
    "rx dma descriptor underflow errors",
    "rx dma descriptor underflow errors",

    "d11cnt txrts value when reset d11cnt",
    "d11cnt rxcrc value when reset d11cnt",
    "d11cnt txnocts value when reset d11cnt",

    /* misc counters */
    "tx/rx dma descriptor errors",
    "tx/rx dma data errors",
    "tx/rx dma descriptor protocol errors",
    "reset count",
    "cnts the TBTT int's",
    "txdmawar",
    "callbacks register failure",

    /* MAC counters: 32-bit version of d11.h's macstat_t */
    "total number of frames sent, incl. Data, ACK, RTS, CTS, Control Management (includes retransmissions)",
    "number of RTS sent out by the MAC",
    "number of CTS sent out by the MAC",
    "number of ACK frames sent out",
    "Not used",
    "beacons transmitted",
    "per-fifo tx underflows",
    "per-fifo tx underflows",
    "per-fifo tx underflows",
    "per-fifo tx underflows",
    "per-fifo tx underflows",
    "per-fifo tx underflows",
    "receive too late",
    "transmit at fallback bw (dynamic bw)",
    "Template underflows (mac was too slow to transmit ACK/CTSor BCN)",
    "Transmit phy error, type of error is reported in tx-status for driver enqueued frames",
    "Received frame longer than legal limit (2346 bytes)",
    "Received frame did not contain enough bytes for its frame type",
    "Either the protocol version != 0 or frame type not data/control/management",
    "number of frames for which the CRC check failed in the MAC",
    "parity check of the PLCP header failed",
    "PHY was able to correlate the preamble but not the header",
    "Number of received frames with a good PLCP (i.e. passing parity check)",
    "Number of received DATA frames with good FCS and matching RA",
    "number of received mgmt frames with good FCS and matching RA",
    "number of received CNTRL frames with good FCS and matching RA",
    "number of unicast RTS addressed to the MAC (good FCS)",
    "number of unicast CTS addressed to the MAC (good FCS)",
    "number of ucast ACKS received (good FCS)",
    "number of received DATA frames (good FCS and not matching RA)",
    "number of received MGMT frames (good FCS and not matching RA)",
    "number of received CNTRL frame (good FCS and not matching RA)",
    "number of received RTS not addressed to the MAC",
    "number of received CTS not addressed to the MAC",
    "number of RX Data multicast frames received by the MAC",
    "number of RX Management multicast frames received by the MAC",
    "number of RX Control multicast frames received by the MAC(unlikely to see these)",
    "beacons received from member of BSS",
    "number of unicast frames addressed to the MAC from other BSS (WDS FRAME)",
    "beacons received from other BSS",
    "Number of response timeouts for transmitted frames expecting a response",
    "transmit beacons canceled due to receipt of beacon (IBSS)",
    "Number of receive fifo 0 overflows",
    "Number of receive fifo 1 overflows (obsolete)",
    "Number of receive fifo 2 overflows (obsolete)",
    "Number of transmit status fifo overflows (obsolete)",
    "Number of PMQ overflows",
    "Number of received Probe requests that made it into the PRQ fifo",
    "Rx Probe Request Que overflow in the AP",
    "Tx Probe Response Fail. AP sent probe response but did not get ACK",
    "Tx Probe Response Success (ACK was received)",
    "Number of probe requests that were dropped from the PRQ fifo because a probe response could not be sent out within the time limit defined in M_PRS_MAXTIME",
    "obsolete",
    "obsolete",
    "obsolete",
    "blockack rxcnt",
    "blockack txcnt",

    /* 802.11 MIB counters, pp. 614 of 802.11 reaff doc. */
    "dot11TransmittedFragmentCount",
    "dot11MulticastTransmittedFrameCount",
    "dot11FailedCount",
    "dot11RetryCount",
    "dot11MultipleRetryCount",
    "dot11FrameduplicateCount",
    "dot11RTSSuccessCount",
    "dot11RTSFailureCount",
    "dot11ACKFailureCount",
    "dot11ReceivedFragmentCount",
    "dot11MulticastReceivedFrameCount",
    "dot11FCSErrorCount",
    "dot11TransmittedFrameCount (bogus MIB?)",
    "dot11WEPUndecryptableCount",

    /* WPA2 counters (see rxundec for DecryptFailureCount) */
    "TKIPLocalMICFailures",
    "TKIPCounterMeasuresInvoked",
    "TKIPReplays",
    "CCMPFormatErrors",
    "CCMPReplays",
    "CCMPDecryptErrors",
    "FourWayHandshakeFailures",
    "dot11WEPUndecryptableCount",
    "dot11WEPICVErrorCount",
    "DecryptSuccessCount",
    "TKIPICVErrorCount",
    "dot11WEPExcludedCount",

    "Tx frames suppressed due to channel rejection",
    "Count PSM watchdogs",
    "Count Phy watchdogs (triggered by ucode)",

    /* MBSS counters, AP only */
    "PRQ entries read in",
    "   which were bcast bss & ssid",
    "   which could not be translated to info",
    "TX suppressions on ATIM fifo",
    "Template marked in use on send bcn ...",
    "...but \"DMA done\" interrupt rcvd",
    "TBTT DPC did not happen in time",

    /* per-rate receive stat counters */
    "packets rx at 1Mbps",
    "packets rx at 2Mbps",
    "packets rx at 5.5Mbps",
    "packets rx at 6Mbps",
    "packets rx at 9Mbps",
    "packets rx at 11Mbps",
    "packets rx at 12Mbps",
    "packets rx at 18Mbps",
    "packets rx at 24Mbps",
    "packets rx at 36Mbps",
    "packets rx at 48Mbps",
    "packets rx at 54Mbps",
    "packets rx at 108mbps",
    "packets rx at 162mbps",
    "packets rx at 216 mbps",
    "packets rx at 270 mbps",
    "packets rx at 324 mbps",
    "packets rx at 378 mbps",
    "packets rx at 432 mbps",
    "packets rx at 486 mbps",
    "packets rx at 540 mbps",

    /* pkteng rx frame stats */
    "unicast frames rxed by the pkteng code",
    "multicast frames rxed by the pkteng code",

    "count of radio disables",
    "PHY count of bphy glitches",
    "bphy_badplcp",

    "Tx frames suppressed due to timer expiration",

    "count for sgi transmit",
    "count for sgi received",
    "count for stbc transmit",
    "count for stbc received",

    "dot11WEPUndecryptableCount",

    /* WPA2 counters (see rxundec for DecryptFailureCount) */
    "TKIPLocalMICFailures",
    "TKIPCounterMeasuresInvoked",
    "TKIPReplays",
    "CCMPFormatErrors",
    "CCMPReplays",
    "CCMPDecryptErrors",
    "FourWayHandshakeFailures",
    "dot11WEPUndecryptableCount",
    "dot11WEPICVErrorCount",
    "DecryptSuccessCount",
    "TKIPICVErrorCount",
    "dot11WEPExcludedCount",

    "count for dma hang",
    "count for reinit",

    "count of ucast frames xmitted on all psta assoc",
    "count of txnoassoc frames xmitted on all psta assoc",
    "count of ucast frames received on all psta assoc",
    "count of bcmc frames received on all psta",
    "count of bcmc frames transmitted on all psta",

    "hw cso required but passthrough",
    "hw cso hdr for normal process",
    "number of frames chained",
    "number of chain size 1 frames",
    "number of frames not chained",
    "max chain size so far",
    "current chain size",
    "drop secondary cnt",
    "Secondary Bus Reset issued by driver",
    "configspace restore by driver",
};
