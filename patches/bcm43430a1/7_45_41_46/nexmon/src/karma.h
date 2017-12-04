
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
typedef struct dot11_management_header dot11_management_header_t;
