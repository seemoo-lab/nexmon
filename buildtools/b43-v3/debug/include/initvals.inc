#ifndef INITVALUES_H_
#define INITVALUES_H_

/* MMIO registers for the kernel device driver.
 * This is for use by the initvals only. */

#define MMIO_DMA0_REASON		0x20
#define MMIO_DMA0_IRQ_MASK		0x24
#define MMIO_DMA1_REASON		0x28
#define MMIO_DMA1_IRQ_MASK		0x2C
#define MMIO_DMA2_REASON		0x30
#define MMIO_DMA2_IRQ_MASK		0x34
#define MMIO_DMA3_REASON		0x38
#define MMIO_DMA3_IRQ_MASK		0x3C
#define MMIO_DMA4_REASON		0x40
#define MMIO_DMA4_IRQ_MASK		0x44
#define MMIO_DMA5_REASON		0x48
#define MMIO_DMA5_IRQ_MASK		0x4C
#define MMIO_IPFT0			0x100	/* TX/RX interrupts per frame and timeout */
#define MMIO_IPFT1			0x104	/* TX/RX interrupts per frame and timeout */
#define MMIO_IPFT2			0x108	/* TX/RX interrupts per frame and timeout */
#define MMIO_IPFT3			0x10C	/* TX/RX interrupts per frame and timeout */
#define MMIO_MACCTL			0x120	/* MAC control */
#define MMIO_MACCMD			0x124	/* MAC command */
#define MMIO_GEN_IRQ_REASON		0x128
#define MMIO_GEN_IRQ_MASK		0x12C
#define MMIO_RAM_CONTROL		0x130
#define MMIO_RAM_DATA			0x134
#define MMIO_PS_STATUS			0x140
#define MMIO_RADIO_HWENABLED_HI		0x158
#define MMIO_SHM_CONTROL		0x160
#define MMIO_SHM_DATA			0x164
#define MMIO_SHM_DATA_UNALIGNED		0x166
#define MMIO_XMITSTAT_0			0x170
#define MMIO_XMITSTAT_1			0x174
#define MMIO_REV3PLUS_TSF_LOW		0x180	/* core rev >= 3 only */
#define MMIO_REV3PLUS_TSF_HIGH		0x184	/* core rev >= 3 only */
#define MMIO_TSF_CFP_REP		0x188
#define MMIO_TSF_CFP_START		0x18C
#define MMIO_TSF_CFP_MAXDUR		0x190

/* 32-bit DMA */
#define MMIO_DMA32_BASE0		0x200
#define MMIO_DMA32_BASE1		0x220
#define MMIO_DMA32_BASE2		0x240
#define MMIO_DMA32_BASE3		0x260
#define MMIO_DMA32_BASE4		0x280
#define MMIO_DMA32_BASE5		0x2A0
/* 64-bit DMA */
#define MMIO_DMA64_BASE0		0x200
#define MMIO_DMA64_BASE1		0x240
#define MMIO_DMA64_BASE2		0x280
#define MMIO_DMA64_BASE3		0x2C0
#define MMIO_DMA64_BASE4		0x300
#define MMIO_DMA64_BASE5		0x340

/* PIO on core rev < 11 */
#define MMIO_PIO_BASE0			0x300
#define MMIO_PIO_BASE1			0x310
#define MMIO_PIO_BASE2			0x320
#define MMIO_PIO_BASE3			0x330
#define MMIO_PIO_BASE4			0x340
#define MMIO_PIO_BASE5			0x350
#define MMIO_PIO_BASE6			0x360
#define MMIO_PIO_BASE7			0x370
/* PIO on core rev >= 11 */
#define MMIO_PIO11_BASE0		0x200
#define MMIO_PIO11_BASE1		0x240
#define MMIO_PIO11_BASE2		0x280
#define MMIO_PIO11_BASE3		0x2C0
#define MMIO_PIO11_BASE4		0x300
#define MMIO_PIO11_BASE5		0x340

#define MMIO_PHY_VER			0x3E0
#define MMIO_PHY_RADIO			0x3E2
#define MMIO_PHY0			0x3E6
#define MMIO_ANTENNA			0x3E8
#define MMIO_CHANNEL			0x3F0
#define MMIO_CHANNEL_EXT		0x3F4
#define MMIO_RADIO_CONTROL		0x3F6
#define MMIO_RADIO_DATA_HIGH		0x3F8
#define MMIO_RADIO_DATA_LOW		0x3FA
#define MMIO_PHY_CONTROL		0x3FC
#define MMIO_PHY_DATA			0x3FE

/* Receive engine */
#define MMIO_RXE_RXMEM			0x402
#define MMIO_RXE_RXCOPYLEN		0x404
#define MMIO_RXE_FIFOCTL		0x406
#define MMIO_RXE_RXHDR_OFFSET		0x40E /* in words */
#define MMIO_RXE_RXHDR_LEN		0x410 /* in words */

#define MMIO_MACFILTER_CONTROL		0x420
#define MMIO_MACFILTER_DATA		0x422
#define MMIO_RCMTA_COUNT		0x43C
#define MMIO_PSM_BRC			0x490
#define MMIO_RADIO_HWENABLED_LO		0x49A
#define MMIO_GPIO_CONTROL		0x49C
#define MMIO_GPIO_MASK			0x49E

#define MMIO_PSM_BRED0			0x4A0
#define MMIO_PSM_BRED1			0x4A2
#define MMIO_PSM_BRED2			0x4A4
#define MMIO_PSM_BRED3			0x4A6

#define MMIO_PSM_BRCL0			0x4A8
#define MMIO_PSM_BRCL1			0x4AA
#define MMIO_PSM_BRCL2			0x4AC
#define MMIO_PSM_BRCL3			0x4AE

#define MMIO_PSM_BRPO0			0x4B0
#define MMIO_PSM_BRPO1			0x4B2
#define MMIO_PSM_BRPO2			0x4B4
#define MMIO_PSM_BRPO3			0x4B6

#define MMIO_PSM_BRWK0			0x4B8
#define MMIO_PSM_BRWK1			0x4BA
#define MMIO_PSM_BRWK2			0x4BC
#define MMIO_PSM_BRWK3			0x4BE

/* Transmit control */
#define MMIO_TCTL_FIFODEF		0x520
#define MMIO_TCTL_FIFOCMD		0x540

/* TSF */
#define MMIO_TSF_CFP_START_LOW		0x604
#define MMIO_TSF_CFP_START_HIGH		0x606
#define MMIO_TSF_CFP_PRETBTT		0x612
#define MMIO_TSF_0			0x632	/* core rev < 3 only */
#define MMIO_TSF_1			0x634	/* core rev < 3 only */
#define MMIO_TSF_2			0x636	/* core rev < 3 only */
#define MMIO_TSF_3			0x638	/* core rev < 3 only */
#define MMIO_RNG			0x65A

/* IFS */
#define MMIO_IFS_SLOT			0x684
#define MMIO_IFSCTL			0x688 /* Interframe space control */
#define  MMIO_IFSCTL_USE_EDCF		0x0004
#define MMIO_POWERUP_DELAY		0x6A8

/* Network allocation vector */
#define MMIO_NAV_CTL			0x700


#endif /* INITVALUES_H_ */

// vim: syntax=b43 ts=8
