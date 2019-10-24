#ifndef EXTERNAL_CONDITIONS_H_
#define EXTERNAL_CONDITIONS_H_

/* Definitions for the External Conditions */

#define EXTCOND(reg, bit)	((bit) | ((reg) << 4))


/* Wrap a condition in the EOI() wrapper to EOI it.
 * Example:	jext EOI(COND_RADAR), jump_target
 */
#define EOI(ext_condition)	((ext_condition) | 0x80)
/* Use extcond_eoi_only() to EOI a condition without jumping.
 * Example:	extcond_eoi_only(COND_RADAR)
 */
#define extcond_eoi_only(ext_condition)		\
	jext EOI(ext_condition), __next_insn+;	\
	__next_insn:;



/* RX condition register */
#define CONDREG_RX		0
#define COND_RX_COMPLETE	EXTCOND(CONDREG_RX, 2)	// NEW
#define COND_RX_WME8		EXTCOND(CONDREG_RX, 3)	// NEW		/* FIXME: Maybe WME related */
#define COND_RX_CRYPTBUSY	EXTCOND(CONDREG_RX, 4)	// NEW
#define COND_RX_BADPLCP		EXTCOND(CONDREG_RX, 5)		/* Corrupt PLCP received */
#define COND_RX_FIFOFULL	EXTCOND(CONDREG_RX, 6)		/* RX complete */
#define	COND_RX_PLCP		EXTCOND(CONDREG_RX, 7)	// NEW
// OLD #define COND_RX_WME8		EXTCOND(CONDREG_RX, 8)		/* FIXME: Maybe WME related */
// OLD #define COND_RX_ATIMWINEND	EXTCOND(CONDREG_RX, 9)		/* FIXME: ATIM window end? */
// OLD #define COND_RX_CRYPTBUSY	EXTCOND(CONDREG_RX, 10)		/* RX crypto engine busy */
#define COND_RX_FIFOBUSY	EXTCOND(CONDREG_RX, 11)		/* RX FIFO operating */
#define COND_RX_MAH		EXTCOND(CONDREG_RX, 13)		/* RX FIFO full */
// OLD #define COND_RX_PLCP		EXTCOND(CONDREG_RX, 14)		/* We received a valid PLCP header */

/* OLD RXE/MAC match condition register (core rev < 5) */
/* NOW TX condition register */
#define CONDREG_RXE		1
#define	COND_TX_NOW		EXTCOND(CONDREG_RXE, 0)
#define	COND_TX_POWER		EXTCOND(CONDREG_RXE, 1)
#define	COND_TX_DONE		EXTCOND(CONDREG_RXE, 2)
#define	COND_TX_NAV		EXTCOND(CONDREG_RXE, 3)
#define	COND_TX_UNDERFLOW	EXTCOND(CONDREG_RXE, 6)
#define COND_TX_PHYERR		EXTCOND(CONDREG_RXE, 7)
#define	COND_TX_BUSY		EXTCOND(CONDREG_RXE, 8)


/* TX condition register */
#define CONDREG_TX		2
// OLD #define COND_TX_NOW		EXTCOND(CONDREG_TX, 0)		/* TX should be done now */
// OLD #define COND_TX_POWER		EXTCOND(CONDREG_TX, 1)		/* FIXME: TX power related */
// OLD #define COND_TX_DONE		EXTCOND(CONDREG_TX, 2)		/* The PHY has finished transmitting all bits */
// OLD #define COND_TX_PMQ		EXTCOND(CONDREG_TX, 3)		/* FIXME: PMQ related? */
//#define COND_MACEN		EXTCOND(CONDREG_TX, 4)		/* MAC enabled */
#define COND_BIH		EXTCOND(CONDREG_TX, 4)
// OLD #define COND_TX_NAV		EXTCOND(CONDREG_TX, 9)		/* FIXME: NAV/slot update required? */
#define COND_TX_PMQ		EXTCOND(CONDREG_TX, 10)	// NEW
// OLD #define COND_TX_UNDERFLOW	EXTCOND(CONDREG_TX, 11)		/* TX underflow */
// OLD #define COND_TX_TBTTEXPIRE	EXTCOND(CONDREG_TX, 12)		/* FIXME: TBTT timer expired? */
#define	COND_RX_ATIMWINEND		EXTCOND(CONDREG_TX, 12)	// NEW
// OLD #define COND_TX_PHYERR		EXTCOND(CONDREG_TX, 13)		/* Check for PHY TX errors needed */
// OLD #define COND_TX_FLUSH		EXTCOND(CONDREG_TX, 14)		/* TX flush requested */
// OLD #define COND_TX_BUSY		EXTCOND(CONDREG_TX, 15)		/* TX engine busy */

/* PHY condition register */
#define CONDREG_PHY		3
#define COND_PHY0		EXTCOND(CONDREG_PHY, 0)		/* FIXME: Unknown meaning */
//#define COND_PHY1		EXTCOND(CONDREG_PHY, 1)		/* FIXME: Unknown meaning */
#define	COND_RX_FCS_GOOD	EXTCOND(CONDREG_PHY, 1)	// NEW
#define COND_RADAR		EXTCOND(CONDREG_PHY, 2)		/* Radar detected */
#define	COND_STA_MODE		EXTCOND(CONDREG_PHY, 4)
#define	COND_AP_MODE		EXTCOND(CONDREG_PHY, 5)
#define COND_PHY6		EXTCOND(CONDREG_PHY, 6)		/* FIXME: Unknown meaning */
#define COND_MACEN		EXTCOND(CONDREG_PHY, 8)	// NEW

/* Unknown condition register */
#define CONDREG_4		4
#define COND_NEED_BEACON		EXTCOND(CONDREG_4, 0)		/* Current transmission should TX a beacon */
#define COND_NEED_RESPONSEFR		EXTCOND(CONDREG_4, 1)		/* Transmission of a response frame is needed (ACK, CTS). */
#define	COND_NEED_PROBE_RESP		EXTCOND(CONDREG_4, 2)           /* Current transmission should TX a probe response */
#define	COND_CONTENTION_PARAM_MODIFIED	EXTCOND(CONDREG_4, 3)
#define COND_MORE_FRAGMENT		EXTCOND(CONDREG_4, 4)		/* FIXME: Sometimes used to skip beacon related code */
#define	COND_FRAME_BURST		EXTCOND(CONDREG_4, 5)
#define	COND_REC_IN_PROGRESS		EXTCOND(CONDREG_4, 6)
#define COND_FRAME_NEED_ACK		EXTCOND(CONDREG_4, 7)		/* FIXME: Seems to be some error condition */
#define COND_TX_ERROR			EXTCOND(CONDREG_4, 8)		/* FIXME: Unknown meaning */
#define COND_RX_ERROR			EXTCOND(CONDREG_4, 9)		/* FIXME: Unknown meaning */
#define COND_TX_MULTICAST_FRAME		EXTCOND(CONDREG_4, 10)          /* Current transmission should TX a multicast frame */
#define COND_NEED_RTS			EXTCOND(CONDREG_4, 11)          /* Transmission of an RTS is needed */
#define COND_PROBE_RESP_LOADED		EXTCOND(CONDREG_4, 12)          /* Probe response was loaded into serializer */


/* PSM condition register */
#define CONDREG_PSM		5
#define COND_PSM(bit)		EXTCOND(CONDREG_PSM, bit)

/* RCM condition register */
#define CONDREG_RCM		6
#define COND_RX_RAMATCH		EXTCOND(CONDREG_RCM, 0)		/* RX RA match */
#define COND_RX_BSSMATCH	EXTCOND(CONDREG_RCM, 6)		/* RX BSS match */

/* Condition register 7 */
#define CONDREG_7		7
#define COND_TRUE		EXTCOND(CONDREG_7, 15)		/* Always true */

#endif /* EXTERNAL_CONDITIONS_H_ */

// vim: syntax=b43 ts=8
