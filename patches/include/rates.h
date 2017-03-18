#ifndef RATES_H
#define RATES_H

/* Rate Defines */

/* Valid rates for the Supported Rates and Extended Supported Rates IEs.
 * Encoding is the rate in 500kbps units, rouding up for fractional values.
 * 802.11-2012, section 6.5.5.2, DATA_RATE parameter enumerates all the values.
 * The rate values cover DSSS, HR/DSSS, ERP, and OFDM phy rates.
 * The defines below do not cover the rates specific to 10MHz, {3, 4.5, 27},
 * and 5MHz, {1.5, 2.25, 3, 4.5, 13.5}, which are not supported by Broadcom devices.
 */

#define RATES_RATE_1M   2       /* 1  Mbps in 500kbps units DSSS */
#define RATES_RATE_2M   4       /* 2  Mbps in 500kbps units DSSS */
#define RATES_RATE_5M5  11      /* 5.5 Mbps in 500kbps units DSSS */
#define RATES_RATE_11M  22      /* 11 Mbps in 500kbps units DSSS */
#define RATES_RATE_6M   12      /* 6  Mbps in 500kbps units OFDM */
#define RATES_RATE_9M   18      /* 9  Mbps in 500kbps units OFDN */
#define RATES_RATE_12M  24      /* 12 Mbps in 500kbps units OFDM */
#define RATES_RATE_18M  36      /* 18 Mbps in 500kbps units OFDM */
#define RATES_RATE_24M  48      /* 24 Mbps in 500kbps units OFDM */
#define RATES_RATE_36M  72      /* 36 Mbps in 500kbps units OFDM */
#define RATES_RATE_48M  96      /* 48 Mbps in 500kbps units OFDM */
#define RATES_RATE_54M  108     /* 54 Mbps in 500kbps units OFDM */
#define RATES_RATE_MAX  108     /* highest rate (54 Mbps) in 500kbps units */

#define RATES_RATE_MASK         0x000000FF

#define RATES_VHT_MCS_MASK      0x0000000F
#define RATES_VHT_MCS_SHIFT     0
#define RATES_VHT_MCS(n)        ((n << RATES_VHT_MCS_SHIFT) & RATES_VHT_MCS_MASK)

#define RATES_VHT_NSS_MASK      0x000000F0
#define RATES_VHT_NSS_SHIFT     4
#define RATES_VHT_NSS(n)        ((n << RATES_VHT_NSS_SHIFT) & RATES_VHT_NSS_MASK)

#define RATES_HT_MCS_MASK       0x00000007
#define RATES_HT_MCS_SHIFT      0
#define RATES_HT_MCS(n)         ((n << RATES_HT_MCS_SHIFT) & RATES_HT_MCS_MASK)

#define RATES_HT_NSS_MASK       0x00000078
#define RATES_HT_NSS_SHIFT      3
#define RATES_HT_NSS(n)         ((n << RATES_HT_NSS_SHIFT) & RATES_HT_NSS_MASK)

#define RATES_TXEXP_MASK        0x00000300
#define RATES_TXEXP_SHIFT       8

#define RATES_BW_MASK           0x00070000
#define RATES_BW_SHIFT          16

#define RATES_STBC              0x00100000
#define RATES_TXBF              0x00200000
#define RATES_LDPC_CODING       0x00400000
#define RATES_SHORT_GI          0x00800000
#define RATES_SHORT_PREAMBLE    0x00800000
#define RATES_ENCODING_MASK     0x03000000
#define RATES_OVERRIDE_RATE     0x40000000
#define RATES_OVERRIDE_MODE     0x80000000

#define RATES_ENCODE_RATE       0x00000000
#define RATES_ENCODE_HT         0x01000000
#define RATES_ENCODE_VHT        0x02000000

#define BW_20MHZ                1
#define BW_40MHZ                2
#define BW_80MHZ                3
#define BW_160MHZ               4

#define RATES_BW_UNSPECIFIED    0x00000000
#define RATES_BW_20MHZ          (BW_20MHZ << RATES_BW_SHIFT)
#define RATES_BW_40MHZ          (BW_40MHZ << RATES_BW_SHIFT)
#define RATES_BW_80MHZ          (BW_80MHZ << RATES_BW_SHIFT)
#define RATES_BW_160MHZ         (BW_160MHZ << RATES_BW_SHIFT)

#define RATES_SET_HT(mcs)       (RATES_OVERRIDE_RATE | RATES_OVERRIDE_MODE | RATES_ENCODE_HT | (RATES_HT_MCS_MASK & mcs))
#define RATES_SET_VHT(mcs)      (RATES_OVERRIDE_RATE | RATES_OVERRIDE_MODE | RATES_ENCODE_VHT | RATES_VHT_NSS(1) | (RATES_VHT_MCS_MASK & mcs))

#endif /* RATES_H */
