#define NULL ((void *)0)
#define FALSE 0
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define MAME82_KARMA_PROBE_RESP	(1 << 0)
#define MAME82_KARMA_ASSOC_RESP	(1 << 1)
#define MAME82_KARMA_DEBUG		(1 << 2)
#define MAME82_KARMA_BEACONING	(1 << 3)
#define MAME82_ENABLE_OPTION(var, opt) ({ uint32 _opt = (opt); (var) |= _opt; })
#define MAME82_DISABLE_OPTION(var, opt) ({ uint32 _opt = (opt); (var) &= ~_opt; })
#define MAME82_IS_ENABLED_OPTION(var, opt) ({ uint32 _opt = (opt); (var) & _opt; })

#define DOT11_MGMT_HDR_LEN	24              /* d11 management header length */
#define	D11_PHY_HDR_LEN		6
#define OSL_PKTTAG_SZ 32

#define PKTDATA(osh, skb)               (((struct sk_buff*)(skb))->data)

#define ETHER_ADDR_STR_LEN	18

/* 802.11 Frame Header Defs */

#define FC_TYPE_MASK	0xC
#define FC_SUBTYPE_MASK	0xF0
#define FC_TYPE_SHIFT	2
#define FC_SUBTYPE_SHIFT	4
#define FC_TYPE(fc)	(((fc) & FC_TYPE_MASK) >> FC_TYPE_SHIFT)
#define FC_KIND_MASK	(FC_TYPE_MASK | FC_SUBTYPE_MASK)


#define FC_TYPE_MNG             0
#define FC_TYPE_CTL             1
#define FC_TYPE_DATA            2

#define FC_SUBTYPE_ASSOC_RESP           1       /* assoc. response */
#define FC_SUBTYPE_REASSOC_REQ          2       /* reassoc. request */
#define FC_SUBTYPE_REASSOC_RESP         3       /* reassoc. response */
#define FC_SUBTYPE_PROBE_REQ            4       /* probe request */
#define FC_SUBTYPE_PROBE_RESP           5       /* probe response */
#define FC_SUBTYPE_BEACON               8       /* beacon */
#define FC_SUBTYPE_ATIM                 9       /* ATIM */
#define FC_SUBTYPE_DISASSOC             10      /* disassoc. */
#define FC_SUBTYPE_AUTH                 11      /* authentication */
#define FC_SUBTYPE_DEAUTH               12      /* de-authentication */
#define FC_SUBTYPE_ACTION               13      /* action */
#define FC_SUBTYPE_ACTION_NOACK         14      /* action no-ack */

#define FC_KIND(t, s)   (((t) << FC_TYPE_SHIFT) | ((s) << FC_SUBTYPE_SHIFT))
#define FC_ASSOC_RESP   FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_ASSOC_RESP)     /* assoc. response */
#define FC_REASSOC_REQ  FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_REASSOC_REQ)    /* reassoc. request */
#define FC_REASSOC_RESP FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_REASSOC_RESP)   /* reassoc. response */
#define FC_PROBE_REQ    FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_PROBE_REQ)      /* probe request */
#define FC_PROBE_RESP   FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_PROBE_RESP)     /* probe response */
#define FC_BEACON       FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_BEACON)         /* beacon */
#define FC_ATIM         FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_ATIM)           /* ATIM */
#define FC_DISASSOC     FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_DISASSOC)       /* disassoc */
#define FC_AUTH         FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_AUTH)           /* authentication */
#define FC_DEAUTH       FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_DEAUTH)         /* deauthentication */
#define FC_ACTION       FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_ACTION)         /* action */
#define FC_ACTION_NOACK FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_ACTION_NOACK)   /* action no-ack */

#define DOT11_MNG_SSID_ID	0


/*** Custom structs ***/
typedef struct ssid_list
{
	struct ssid_list *next;
	char ssid[32];
	uint8 len_ssid;
} ssid_list_t;


void push_ssid(ssid_list_t *head, char* ssid, uint8 ssid_len);
int validate_ssid(ssid_list_t *head, char* ssid, uint8 ssid_len);
void sscfg_iter_test(struct wlc_info *wlc);
/*** End custom structs ***/

typedef uint8 uchar;

/** Management frame header */
struct dot11_management_header {
        uint16                  fc;             /* frame control */
        uint16                  durid;          /* duration/ID */
        struct ether_addr       da;             /* receiver address */
        struct ether_addr       sa;             /* transmitter address */
        struct ether_addr       bssid;          /* BSS ID */
        uint16                  seq;            /* sequence control */
};


/** D11 structs/types **/
#define WL_RSSI_ANT_MAX	4 //Max possible RX antennas

typedef struct d11rxhdr {
        uint16 RxFrameSize;     /* Actual byte length of the frame data received */

        uint8 dma_flags;    /* bit 0 indicates short or long rx status. 1 == short. */
        uint8 fifo;         /* rx fifo number */

        uint16 PhyRxStatus_0;   /* PhyRxStatus 15:0 */
        uint16 PhyRxStatus_1;   /* PhyRxStatus 31:16 */
        uint16 PhyRxStatus_2;   /* PhyRxStatus 47:32 */
        uint16 PhyRxStatus_3;   /* PhyRxStatus 63:48 */
        uint16 PhyRxStatus_4;   /* PhyRxStatus 79:64 */
        uint16 PhyRxStatus_5;   /* PhyRxStatus 95:80 */
        uint16 RxStatus1;       /* MAC Rx Status */
        uint16 RxStatus2;       /* extended MAC Rx status */
        uint16 RxTSFTime;       /* RxTSFTime time of first MAC symbol + M_PHY_PLCPRX_DLY */
        uint16 RxChan;          /* Rx channel info or chanspec */
        uint16 RxFameSize_0;    /* size of rx-frame in fifo-0 in case frame is copied to fifo-1 */
        uint16 HdrConvSt;       /* hdr conversion status. Copy of ihr(RCV_HDR_CTLSTS). */
        uint16 AvbRxTimeL;      /* AVB RX timestamp low16 */
        uint16 AvbRxTimeH;      /* AVB RX timestamp high16 */
        uint16 MuRate;          /* MU rate info (bit3:0 MCS, bit6:4 NSTS) */
        uint16 Pad;             /* Reserved to make 4-bytes align. */
} d11rxhdr_t;

typedef struct wlc_d11rxhdr wlc_d11rxhdr_t;
struct wlc_d11rxhdr {
        /* Even though rxhdr can be in short or long format, always declare it here
         * to be in long format. So the offsets for the other fields are always the same.
         */
        d11rxhdr_t rxhdr;
        uint32  tsf_l;          /* TSF_L reading */
        int8    rssi;           /* computed instantaneous rssi in BMAC */
        int8    rssi_qdb;       /* qdB portion of the computed rssi */
        int8    do_rssi_ma;     /* do per-pkt sampling for per-antenna ma in HIGH */
        int8    rxpwr[WL_RSSI_ANT_MAX]; /* rssi for supported antennas */
};

/** Probe Response related structs/types **/
/*
#define WLC_PROBRESP_MAXFILTERS	3


typedef bool (*probreq_filter_fn_t)(void *handle, wlc_bsscfg_t *cfg, wlc_d11rxhdr_t *wrxh, uint8 *plcp, struct dot11_management_header *hdr, uint8 *body, int body_len, bool *psendProbeResp);

typedef struct probreqcb {
        void *hdl;
        probreq_filter_fn_t filter_fn;
} probreqcb_t;

typedef struct wlc_probresp_info {
        wlc_info_t *wlc;
        probreqcb_t probreq_filters[WLC_PROBRESP_MAXFILTERS];
        int p2p_index;
} wlc_probresp_info_t;
*/

typedef struct bcm_tlv {
	uint8 id;
	uint8 len;
	uint8 data[1];
} bcm_tlv_t;
typedef struct dot11_management_header dot11_management_header_t;

/* Endianess Conversion */
#define SWAP16(val) ((uint16)((((uint16)(val) & (uint16)0x00ffU) << 8) | (((uint16)(val) & (uint16)0xff00U) >> 8)))
#define SWAP32(val) ((uint32)((((uint32)(val) & (uint32)0x000000ffU) << 24) | (((uint32)(val) & (uint32)0x0000ff00U) <<  8) | (((uint32)(val) & (uint32)0x00ff0000U) >>  8) | (((uint32)(val) & (uint32)0xff000000U) >> 24)))
#define swap16(val) ({ uint16 _val = (val); SWAP16(_val); })
#define swap32(val) ({ uint32 _val = (val); SWAP32(_val); })

/*
#define LTOH16(i) swap16(i) 
#define ltoh16(i) swap16(i)
#define LTOH32(i) swap32(i)
#define ltoh32(i) swap32(i)
#define HTOL16(i) swap16(i)
#define htol16(i) swap16(i)
#define HTOL32(i) swap32(i)
#define htol32(i) swap32(i)
*/

#define LTOH16(i) (i) 
#define ltoh16(i) (i)
#define LTOH32(i) (i)
#define ltoh32(i) (i)
#define HTOL16(i) (i)
#define htol16(i) (i)
#define HTOL32(i) (i)
#define htol32(i) (i)

/** bsscfg related **/
#define WLC_MAXBSSCFG 0x20
//#define BSS_MATCH_WLC(_wlc, cfg) TRUE
#define BSS_MATCH_WLC(_wlc, cfg) ((_wlc) == ((cfg)->wlc))
#define FOREACH_BSS(wlc, idx, cfg) for (idx = 0; (int) idx < WLC_MAXBSSCFG; idx++) if (((cfg = (wlc)->bsscfg[idx]) != NULL) &&  BSS_MATCH_WLC((wlc), cfg))

//Note on BSSCFG_AP_ENABLED:	as the brcmfmac drive is patched to bring up the monitor interface as AP interface, we have two of them
//						in case the monitor interface is up, but only the real AP interface has set its bsscfg to enable
#define BSSCFG_AP_ENABLED(cfg)          (((cfg)->_ap) && ((cfg)->enable))

typedef struct beacon_fixed_params
{
	uint32 timestamp[2];
	uint16 beacon_interval;
	uint16 caps;
} beacon_fixed_params_t;
