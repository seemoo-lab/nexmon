/*
 * Misc system wide definitions
 *
 * Copyright (C) 2023, Broadcom. All Rights Reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifndef	_bcmdefs_h_
#define	_bcmdefs_h_

/*
 * One doesn't need to include this file explicitly, gets included automatically if
 * typedefs.h is included.
 */

/* Use BCM_REFERENCE to suppress warnings about intentionally-unused function
 * arguments or local variables.
 */
#define BCM_REFERENCE(data)	((void)(data))

/* Allow for suppressing unused variable warnings. */
#ifdef __GNUC__
#define UNUSED_VAR     __attribute__ ((unused))
#else
#define UNUSED_VAR
#endif

/* Disable -finstrument-function on specific functions */
#ifdef __GNUC__
#define NO_INSTR_FUNC  __attribute__ ((no_instrument_function))
#else
#define NO_INSTR_FUNC
#endif

/* Macros to allow Coverity modeling contructs in source code */
#if defined(__COVERITY__)

/* Coverity Doc:
 * Indicates to the TAINTED_SCALAR checker and the INTEGER_OVERFLOW checker
 * that a function taints its argument
 */
#define COV_TAINTED_DATA_ARG(arg)  __coverity_tainted_data_argument__(arg)

/* Coverity Doc:
 * Indicates to the TAINTED_SCALAR checker and the INTEGER_OVERFLOW checker
 * that a function is a tainted data sink for an argument.
 */
#define COV_TAINTED_DATA_SINK(arg) __coverity_tainted_data_sink__(arg)

/* Coverity Doc:
 * Models a function that cannot take a negative number as an argument. Used in
 * conjunction with other models to indicate that negative arguments are invalid.
 */
#define COV_NEG_SINK(arg)          __coverity_negative_sink__(arg)

#else

#define COV_TAINTED_DATA_ARG(arg)  do { } while (0)
#define COV_TAINTED_DATA_SINK(arg) do { } while (0)
#define COV_NEG_SINK(arg)          do { } while (0)

#endif /* __COVERITY__ */

/* GNU GCC 4.6+ supports selectively turning off a warning.
 * Define these diagnostic macros to help suppress cast-qual warning
 * until all the work can be done to fix the casting issues.
 */
#if defined(__GNUC__) && defined(STRICT_GCC_WARNINGS) && (__GNUC__ > 4 || (__GNUC__ == \
	4 && __GNUC_MINOR__ >= 6))
#define GCC_DIAGNOSTIC_PUSH_SUPPRESS_CAST()              \
	_Pragma("GCC diagnostic push")			 \
	_Pragma("GCC diagnostic ignored \"-Wcast-qual\"")
#define GCC_DIAGNOSTIC_POP()                             \
	_Pragma("GCC diagnostic pop")
#elif defined(_MSC_VER)
#define GCC_DIAGNOSTIC_PUSH_SUPPRESS_CAST()              \
	__pragma(warning(push))                          \
	__pragma(warning(disable:4090))
#define GCC_DIAGNOSTIC_POP()                             \
	__pragma(warning(pop))
#else
#define GCC_DIAGNOSTIC_PUSH_SUPPRESS_CAST()
#define GCC_DIAGNOSTIC_POP()
#endif   /* Diagnostic macros not defined */

/* Compile-time assert can be used in place of ASSERT if the expression evaluates
 * to a constant at compile time.
 * GCC 4.6 adds the C11 keyword _Static_assert which has the advantage of printing
 * an informative error message when the assertion fails.
 */
#ifdef __GNUC__
#if (__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || (__GNUC__ > 4)
#define __USE_C11_ASSERT
#endif /* GCC ver >= 4.6 */
#endif /* __GNUC__ */

#ifdef __USE_C11_ASSERT

#define STATIC_ASSERT(X) { _Static_assert((X), "Static assertion failure"); }

#ifdef WL_REG_SIZECHECK
#define SIZECHECK(LHS, RHS) { _Static_assert(sizeof((LHS)) == sizeof((RHS)), \
	"Register size mismatch: need a cast?"); }
#else
#define SIZECHECK(LHS, RHS)
#endif /* WL_REG_SIZECHECK */

#else /* ! __USE_C11_ASSERT */

#define STATIC_ASSERT(expr) { \
	/* Make sure the expression is constant. */ \
	typedef enum { _STATIC_ASSERT_NOT_CONSTANT = (expr) } _static_assert_e UNUSED_VAR; \
	/* Make sure the expression is true. */ \
	typedef char STATIC_ASSERT_FAIL[(expr) ? 1 : -1] UNUSED_VAR; \
}

#ifdef WL_REG_SIZECHECK
#define SIZECHECK(LHS, RHS) STATIC_ASSERT(sizeof((LHS)) == sizeof((RHS)))
#else
#define SIZECHECK(LHS, RHS)
#endif /* WL_REG_SIZECHECK */

#endif /* __USE_C11_ASSERT */

#ifdef BCM_SANITIZE
/*
 * There are a lot of time-sensitive tasks at attach time, so for the time being
 * we exclude attach functions from BCM_SANITIZE feature.
 */
#define ATTRIBUTE_NO_SANITIZE_ADDRESS __attribute__ ((no_sanitize_address))
#else
#define ATTRIBUTE_NO_SANITIZE_ADDRESS
#endif

/* Reclaiming text and data :
 * The following macros specify special linker sections that can be reclaimed
 * after a system is considered 'up'.
 * BCMATTACHFN is also used for detach functions (it's not worth having a BCMDETACHFN,
 * as in most cases, the attach function calls the detach function to clean up on error).
 */
#if defined(BCM_RECLAIM)

extern bool bcm_attach_part_reclaimed;
extern bool bcm_preattach_part_reclaimed;
extern bool bcm_postattach_part_reclaimed;

#define ATTACH_PART_RECLAIMED()		(bcm_attach_part_reclaimed)
#define PREATTACH_PART_RECLAIMED()	(bcm_preattach_part_reclaimed)
#define POSTATTACH_PART_RECLAIMED()	(bcm_postattach_part_reclaimed)

#if defined(BCM_RECLAIM_ATTACH_FN_DATA)
#define BCMATTACHDATA(_data)	__attribute__ ((__section__ (".dataini2." #_data))) _data
#define BCMATTACHFN(_fn)	ATTRIBUTE_NO_SANITIZE_ADDRESS \
	__attribute__ ((__section__ (".textini2." #_fn), noinline)) _fn

#ifndef PREATTACH_NORECLAIM
#define BCMPREATTACHDATA(_data)	__attribute__ ((__section__ (".dataini3." #_data))) _data
#define BCMPREATTACHFN(_fn)	ATTRIBUTE_NO_SANITIZE_ADDRESS \
	__attribute__ ((__section__ (".textini3." #_fn), noinline)) _fn
#else
#define BCMPREATTACHDATA(_data)	__attribute__ ((__section__ (".dataini2." #_data))) _data
#define BCMPREATTACHFN(_fn)	ATTRIBUTE_NO_SANITIZE_ADDRESS \
	__attribute__ ((__section__ (".textini2." #_fn), noinline)) _fn
#endif /* PREATTACH_NORECLAIM  */
#define BCMPOSTATTACHDATA(_data)	__attribute__ ((__section__ (".dataini5." #_data))) _data
#define BCMPOSTATTACHFN(_fn)	ATTRIBUTE_NO_SANITIZE_ADDRESS \
	__attribute__ ((__section__ (".textini5." #_fn), noinline)) _fn
#if defined(MLC_REBIND)
#define BCMATTACHMLODATA(_data)	_data
#define BCMATTACHMLOFN(_fn)	_fn
#else /* ! MLC_REBIND */
#define BCMATTACHMLODATA(_data)	__attribute__ ((__section__ (".dataini6." #_data))) _data
#define BCMATTACHMLOFN(_fn)	ATTRIBUTE_NO_SANITIZE_ADDRESS \
	__attribute__ ((__section__ (".textini6." #_fn), noinline)) _fn
#endif /* ! MLC_REBIND */

#else  /* BCM_RECLAIM_ATTACH_FN_DATA  */
#define BCMATTACHDATA(_data)	_data
#define BCMATTACHFN(_fn)	_fn
#define BCMPREATTACHDATA(_data)	_data
#define BCMPREATTACHFN(_fn)	_fn
#define BCMPOSTATTACHDATA(_data)	_data
#define BCMPOSTATTACHFN(_fn)	_fn
#define BCMATTACHMLODATA(_data)	_data
#define BCMATTACHMLOFN(_fn)	_fn
#endif /* BCM_RECLAIM_ATTACH_FN_DATA  */

#define BCMINITDATA(_data)	_data
#define BCMINITFN(_fn)		_fn
#ifndef CONST
#define CONST	const
#endif

/* Non-manufacture or internal attach function/dat */
#if !defined(WLTEST)
#define	BCMNMIATTACHFN(_fn)	BCMATTACHFN(_fn)
#define	BCMNMIATTACHDATA(_data)	BCMATTACHDATA(_data)
#else
#define	BCMNMIATTACHFN(_fn)	_fn
#define	BCMNMIATTACHDATA(_data)	_data
#endif

#if !defined(ATE_BUILD) && defined(BCM_CISDUMP_NO_RECLAIM)
#define	BCMCISDUMPATTACHFN(_fn)		_fn
#define	BCMCISDUMPATTACHDATA(_data)	_data
#else
#define	BCMCISDUMPATTACHFN(_fn)		BCMNMIATTACHFN(_fn)
#define	BCMCISDUMPATTACHDATA(_data)	BCMNMIATTACHDATA(_data)
#endif /* !ATE_BUILD && BCM_CISDUMP_NO_RECLAIM */

/* SROM with OTP support */
#if defined(BCMOTPSROM)
#define	BCMSROMATTACHFN(_fn)		_fn
#define	BCMSROMATTACHDATA(_data)	_data
#else
#define	BCMSROMATTACHFN(_fn)		BCMNMIATTACHFN(_fn)
#define	BCMSROMATTACHDATA(_data)	BCMNMIATTACHFN(_data)
#endif	/* BCMOTPSROM */

#if defined(BCM_CISDUMP_NO_RECLAIM)
#define	BCMSROMCISDUMPATTACHFN(_fn)	_fn
#define	BCMSROMCISDUMPATTACHDATA(_data)	_data
#else
#define	BCMSROMCISDUMPATTACHFN(_fn)	BCMSROMATTACHFN(_fn)
#define	BCMSROMCISDUMPATTACHDATA(_data)	BCMSROMATTACHDATA(_data)
#endif /* BCM_CISDUMP_NO_RECLAIM */

#define BCMUNINITFN(_fn)	_fn

#else /* BCM_RECLAIM */

#define bcm_attach_part_reclaimed	(1)
#define bcm_preattach_part_reclaimed	(1)
#define bcm_postattach_part_reclaimed	(1)
#define BCMATTACHDATA(_data)		_data
#define BCMATTACHFN(_fn)		_fn
#define BCMATTACHMLODATA(_data)		_data
#define BCMATTACHMLOFN(_fn)		_fn
#define BCM_SRM_ATTACH_DATA(_data)	_data
#define BCM_SRM_ATTACH_FN(_fn)		_fn
#define BCMPREATTACHDATA(_data)		_data
#define BCMPREATTACHFN(_fn)		_fn
#define BCMPOSTATTACHDATA(_data)	_data
#define BCMPOSTATTACHFN(_fn)		_fn
#define BCMINITDATA(_data)		_data
#define BCMINITFN(_fn)			_fn
#define BCMUNINITFN(_fn)		_fn
#define	BCMNMIATTACHFN(_fn)		_fn
#define	BCMNMIATTACHDATA(_data)		_data
#define	BCMSROMATTACHFN(_fn)		_fn
#define	BCMSROMATTACHDATA(_data)	_data
#define BCMPREATTACHFNSR(_fn)		_fn
#define BCMPREATTACHDATASR(_data)	_data
#define BCMATTACHFNSR(_fn)		_fn
#define BCMATTACHDATASR(_data)		_data
#define	BCMSROMATTACHFN(_fn)		_fn
#define	BCMSROMATTACHDATA(_data)	_data
#define	BCMCISDUMPATTACHFN(_fn)		_fn
#define	BCMCISDUMPATTACHDATA(_data)	_data
#define	BCMSROMCISDUMPATTACHFN(_fn)	_fn
#define	BCMSROMCISDUMPATTACHDATA(_data)	_data
#define CONST				const

#define ATTACH_PART_RECLAIMED()		(bcm_attach_part_reclaimed)
#define PREATTACH_PART_RECLAIMED()	(bcm_preattach_part_reclaimed)
#define POSTATTACH_PART_RECLAIMED()	(bcm_postattach_part_reclaimed)

#endif /* BCM_RECLAIM */

#define BCMUCODEDATA(_data)		BCMINITDATA(_data)

#if defined(BCM_DMA_CT) && !defined(BCM_DMA_CT_DISABLED)
#define BCMUCODEFN(_fn)			BCMINITFN(_fn)
#else
#define BCMUCODEFN(_fn)			BCMATTACHFN(_fn)
#endif /* BCM_DMA_CT */

/* BCMFASTPATH Related Macro defines
*/
#ifndef BCMFASTPATH
#if defined(DONGLEBUILD) && defined(SW_PAGING)
#define BCMFASTPATH		__attribute__ ((__section__ (".text_fastpath")))
#define BCMWATCHDOG		__attribute__ ((__section__ (".text_watchdog")))
#else /* DONGLEBUILD && SW_PAGING */
#if defined(mips) || defined(STB)
#define BCMFASTPATH		__attribute__ ((__section__ (".text.fastpath")))
#define BCMFASTPATH_HOST	__attribute__ ((__section__ (".text.fastpath_host")))
#else /* mips || STB */
#define BCMFASTPATH
#define BCMFASTPATH_HOST
#endif /* mips || STB */
#define BCMWATCHDOG
#endif /* DONGLEBUILD && SW_PAGING */
#endif /* BCMFASTPATH */

/* Use the BCMRAMFN() macro to tag functions in source that must be included in RAM (excluded from
 * ROM). This should eliminate the need to manually specify these functions in the ROM config file.
 * It should only be used in special cases where the function must be in RAM for *all* ROM-based
 * chips.
 */
#if defined(BCMROMBUILD)
#define BCMRAMFN(_fn)	__attribute__ ((__section__ (".text_ram." #_fn), noinline)) _fn
#define BCMPOSTTRAPFN(_fn)         _fn
#define BCMPOSTTRAPRAMFN(_fn)      __attribute__ ((__section__ (".text_ram." #_fn))) _fn
#define BCMPOSTTRAPFASTPATH(_fn)   __attribute__ ((__section__ (".text_ram." #_fn))) _fn
#else
#define BCMRAMFN(_fn)	_fn
#define BCMPOSTTRAPFN(_fn)         _fn
#define BCMPOSTTRAPFASTPATH(_fn)   _fn
#define BCMPOSTTRAPRAMFN(fn)       BCMPOSTTRAPFN(fn)
#endif

/* Use BCMSPECSYM() macro to tag symbols going to a special output section in the binary. */
#define BCMSPECSYM(_sym)        __attribute__ ((__section__ (".special." #_sym))) _sym

#define STATIC	static

/* Bus types */
enum bustype_e {
	SI_BUS = 0,	/* SOC Interconnect */
	PCI_BUS = 1,	/* PCI target */
	PCMCIA_BUS = 2,	/* PCMCIA target */
	SDIO_BUS = 3, 	/* SDIO target */
	JTAG_BUS = 4,	/* JTAG */
	USB_BUS = 5,	/* USB (does not support R/W REG) */
	SPI_BUS = 6,	/* gSPI target */
	RPC_BUS = 7,	/* RPC target */
	U_BUS = 8	/* UBUS target */
};

/* Allows size optimization for single-bus image */
#ifdef BCMBUSTYPE
#define BUSTYPE(bus)	(BCMBUSTYPE)
#else
#define BUSTYPE(bus)	(bus)
#endif

#ifdef BCMBUSCORETYPE
#define BUSCORETYPE(ct)		(BCMBUSCORETYPE)
#else
#define BUSCORETYPE(ct)		(ct)
#endif

/* Allows size optimization for single-backplane image */
#ifdef BCMCHIPTYPE
#define CHIPTYPE(bus)	(BCMCHIPTYPE)
#else
#define CHIPTYPE(bus)	(bus)
#endif

/* Allows size optimization for SPROM support */
#if defined(BCMSPROMBUS)
#define SPROMBUS	(BCMSPROMBUS)
#elif defined(SI_PCMCIA_SROM)
#define SPROMBUS	(PCMCIA_BUS)
#else
#define SPROMBUS	(PCI_BUS)
#endif

/* Allows size optimization for single-chip image */
#ifdef BCMCHIPID
#define CHIPID(chip)	(BCMCHIPID)
#else
#define CHIPID(chip)	(chip)
#endif

#ifdef BCMCHIPREV
#define CHIPREV(rev)	(BCMCHIPREV)
#else
#define CHIPREV(rev)	(rev)
#endif

#ifdef BCMPCIEREV
#define PCIECOREREV(rev)	(BCMPCIEREV)
#else
#define PCIECOREREV(rev)	(rev)
#endif

#ifdef BCMPMUREV
#define PMUREV(rev)	(BCMPMUREV)
#else
#define PMUREV(rev)	(rev)
#endif

#ifdef BCMCCREV
#define CCREV(rev)	(BCMCCREV)
#else
#define CCREV(rev)	(rev)
#endif

#ifdef BCMGCIREV
#define GCIREV(rev)	(BCMGCIREV)
#else
#define GCIREV(rev)	(rev)
#endif

#ifdef BCMCR4REV
#define CR4REV		(BCMCR4REV)
#endif

#ifdef BCMHWAREV
#define HWACOREREV(rev)	(BCMHWAREV)
#else
#define HWACOREREV(rev)	(rev)
#endif

/* Defines for DMA Address Width - Shared between OSL and HNDDMA */
#define DMADDR_MASK_32 0x0		/* Address mask for 32-bits */
#define DMADDR_MASK_30 0xc0000000	/* Address mask for 30-bits */
#define DMADDR_MASK_26 0xFC000000	/* Address maks for 26-bits */
#define DMADDR_MASK_0  0xffffffff	/* Address mask for 0-bits (hi-part) */

#define	DMADDRWIDTH_26  26 /* 26-bit addressing capability */
#define	DMADDRWIDTH_30  30 /* 30-bit addressing capability */
#define	DMADDRWIDTH_32  32 /* 32-bit addressing capability */
#define	DMADDRWIDTH_63  63 /* 64-bit addressing capability */
#define	DMADDRWIDTH_64  64 /* 64-bit addressing capability */

#define DMADDR_LOWADDR_MASK	0xffffffff		/* Address mask for low 4 bytes */

typedef union dma64addr {
	struct {
		uint32 lo;
		uint32 hi;
	};
	struct {
		uint32 low;
		uint32 high;
	};
	struct {
		uint32 loaddr;
		uint32 hiaddr;
	};
	struct {
		uint32 low_addr;
		uint32 high_addr;
	};
} dma64addr_t;

#define DADDR64_FMT             " daddr64 hi 0x%08x lo 0x%08x"
#define DADDR64_VAL(_d64_)      (_d64_).hi, (_d64_).lo

#define PHYSADDR64HI(_pa) ((_pa).hiaddr)
#define PHYSADDR64HISET(_pa, _val) \
	do { \
		(_pa).hiaddr = (_val);		\
	} while (0)
#define PHYSADDR64LO(_pa) ((_pa).loaddr)
#define PHYSADDR64LOSET(_pa, _val) \
	do { \
		(_pa).loaddr = (_val);		\
	} while (0)

#ifdef BCMDMA64OSL
typedef dma64addr_t dmaaddr_t;
#define PHYSADDRHI(_pa) PHYSADDR64HI(_pa)
#define PHYSADDRHISET(_pa, _val) PHYSADDR64HISET(_pa, _val)
#define PHYSADDRLO(_pa)  PHYSADDR64LO(_pa)
#define PHYSADDRLOSET(_pa, _val) PHYSADDR64LOSET(_pa, _val)
#define PHYSADDRTOULONG(_pa, _ulong) \
	do { \
		_ulong = ((unsigned long long)(_pa).hiaddr << 32) | ((_pa).loaddr); \
	} while (0)

#define ULONGTOPHYSADDR(_ulong, _pa) \
	do { \
		(_pa).loaddr = ((_ulong) &  DMADDR_LOWADDR_MASK);		\
		(_pa).hiaddr = (((_ulong) >> 32) & DMADDR_LOWADDR_MASK);	 \
	} while (0)
#else
typedef unsigned long dmaaddr_t;
#define PHYSADDRHI(_pa) (0)
#define PHYSADDRHISET(_pa, _val)
#define PHYSADDRLO(_pa) ((_pa))
#define PHYSADDRLOSET(_pa, _val) \
	do { \
		(_pa) = (_val);			\
	} while (0)
#define PHYSADDRTOULONG(_pa, _ulong) \
	do { \
		_ulong = (_pa); \
	} while (0)

#define ULONGTOPHYSADDR(_ulong, _pa) \
	do { \
		(_pa) = (_ulong);		\
	} while (0)
#endif /* BCMDMA64OSL */

#define PHYSADDR_ZERO(_pa)	\
({ \
	PHYSADDRLOSET((_pa), 0U); \
	PHYSADDRHISET((_pa), 0U); \
})

#define PHYSADDRISZERO(_pa) (PHYSADDRLO(_pa) == 0 && PHYSADDRHI(_pa) == 0)

/* One physical DMA segment */
typedef struct  {
	dmaaddr_t addr;
	uint32	  length;
} hnddma_seg_t;

#define LOHI32_TO_U64(_lo32_, _hi32_) \
	(uint64)(((uint64)((uintptr)(_hi32_)) << 32) | \
	         ((uint32)((uintptr)(_lo32_)) << 0))
#define U64_TO_LO32(_u64_)   (uint32)((_u64_) >>  0)
#define U64_TO_HI32(_u64_)   (uint32)((_u64_) >> 32)

#if defined(linux)
#define MAX_DMA_SEGS 8
#else
#define MAX_DMA_SEGS 4
#endif

typedef struct {
	void *oshdmah; /* Opaque handle for OSL to store its information */
	uint origsize; /* Size of the virtual packet */
	uint nsegs;
	hnddma_seg_t segs[MAX_DMA_SEGS];
} hnddma_seg_map_t;

/* packet headroom necessary to accommodate the largest header in the system, (i.e TXOFF).
 * By doing, we avoid the need  to allocate an extra buffer for the header when bridging to WL.
 * There is a compile time check in wlc.c which ensure that this value is at least as big
 * as TXOFF. This value is used in dma_rxfill (hnddma.c).
 */

#if defined(BCM_RPC_NOCOPY) || defined(BCM_RCP_TXNOCOPY)
/* add 40 bytes to allow for extra RPC header and info  */
#define BCMEXTRAHDROOM 260
#else /* BCM_RPC_NOCOPY || BCM_RPC_TXNOCOPY */
#if defined(linux) && defined(STB)
#if defined(BCMDONGLEHOST)
#define BCMEXTRAHDROOM 32 /* For FullDongle, no D11 headroom space required. */
#else
#ifdef WL_EAP_UCODE
#define BCMEXTRAHDROOM    272 /* EAP requires room for 48 more bytes in tx descriptor */
#else
#define BCMEXTRAHDROOM 224
#endif
#endif /* ! BCMDONGLEHOST */
#else
#ifdef WL_EAP_UCODE
#define BCMEXTRAHDROOM    252 /* EAP requires room for 48 more bytes in tx descriptor */
#else
#define BCMEXTRAHDROOM 204
#endif
#endif /* linux && STB */
#endif /* BCM_RPC_NOCOPY || BCM_RPC_TXNOCOPY */

/* Packet alignment for most efficient SDIO (can change based on platform) */
#ifndef SDALIGN
#define SDALIGN	32
#endif

/* Headroom required for dongle-to-host communication.  Packets allocated
 * locally in the dongle (e.g. for CDC ioctls or RNDIS messages) should
 * leave this much room in front for low-level message headers which may
 * be needed to get across the dongle bus to the host.  (These messages
 * don't go over the network, so room for the full WL header above would
 * be a waste.).
*/
#define BCMDONGLEHDRSZ 12
#define BCMDONGLEPADSZ 16

#define BCMDONGLEOVERHEAD	(BCMDONGLEHDRSZ + BCMDONGLEPADSZ)

#ifdef BCMDBG

#ifndef BCMDBG_ERR
#define BCMDBG_ERR
#endif /* BCMDBG_ERR */

#if defined(CMWIFI) && !defined(BCMDBG_ERR)
#define BCMDBG_ERR
#endif /* BCMDBG_ERR */

#ifndef BCMDBG_ASSERT
#define BCMDBG_ASSERT
#endif /* BCMDBG_ASSERT */

#endif /* BCMDBG */

#if defined(NO_BCMDBG_ASSERT)
#undef BCMDBG_ASSERT
#undef BCMASSERT_LOG
#endif

#if defined(BCMDBG_ASSERT)
#define BCMASSERT_SUPPORT
#endif

/* Macros for doing definition and get/set of bitfields
 * Usage example, e.g. a three-bit field (bits 4-6):
 *    #define <NAME>_M	BITFIELD_MASK(3)
 *    #define <NAME>_S	4
 * ...
 *    regval = R_REG(osh, &regs->regfoo);
 *    field = GFIELD(regval, <NAME>);
 *    regval = SFIELD(regval, <NAME>, 1);
 *    W_REG(osh, &regs->regfoo, regval);
 */
#define BITFIELD_MASK(width) \
		(((unsigned)1 << (width)) - 1)
#define GFIELD(val, field) \
		(((val) >> field ## _S) & field ## _M)
#define SFIELD(val, field, bits) \
		(((val) & (~(field ## _M << field ## _S))) | \
		 ((unsigned)(bits) << field ## _S))

/*
 * Bit manipulation macros for fields described as NAME_MASK and NAME_SHIFT
 *
 * E.g. register definitions of the form:
 *
 *      #define HWA_COMMON_HWA2HWCAP_SWPKT_ADDR32_CAP_SHIFT      2
 *      #define HWA_COMMON_HWA2HWCAP_SWPKT_ADDR32_CAP_MASK \
 *         (0x1 << HWA_COMMON_HWA2HWCAP_SWPKT_ADDR32_CAP_SHIFT)
 *
 * Note, the MASK will already have the bitmask shifted. This would allow for
 * shiftless operations (e.g clearing or simply testing for non 0)
 *
 * An example of a test a field for non-zero
 * BCM_TBF(val, NAME)               ((val) & NAME##_MASK)
 *
 * if (BCM_TBIT(my_u32, HWA_COMMON_HWA2HWCAP_SWPKT_ADDR32_CAP)) {
 *    printf("SWPKT_ADDR32 capability is set in my_u32\n");
 * }
 *
 */
#ifndef BCM_BIT_MANIP_MACROS
#define BCM_BIT_MANIP_MACROS
#define BCM_GBF(val, NAME)           (((val) & NAME##_MASK) >> NAME##_SHIFT)
#define BCM_CBF(val, NAME)           ((val) & ~NAME##_MASK)
#define BCM_SBF(val, NAME)           (((val) << NAME##_SHIFT) & NAME##_MASK)
#define BCM_GBIT(val, NAME)          (((val) & NAME##_MASK) >> NAME##_SHIFT)
#define BCM_CBIT(val, NAME)          ((val) & ~NAME##_MASK)
#define BCM_SBIT(NAME)               (1 << NAME##_SHIFT)
#define BCM_TBF(val, NAME)           ((val) & NAME##_MASK)
#define BCM_TBIT(val, NAME)          ((val) & NAME##_MASK)
#define BCM_MASK(NAME)               (((1 << NAME##_NBITS) - 1) << NAME##_SHIFT)

#define BCM_GBF_REV(val, NAME, rev)  (((val) & NAME##_MASK(rev)) >> NAME##_SHIFT(rev))
#define BCM_CBF_REV(val, NAME, rev)  ((val) & ~NAME##_MASK(rev))
#define BCM_SBF_REV(val, NAME, rev)  (((val) << NAME##_SHIFT(rev)) & NAME##_MASK(rev))
#define BCM_GBIT_REV(val, NAME, rev) (((val) & NAME##_MASK(rev)) >> NAME##_SHIFT(rev))
#define BCM_CBIT_REV(val, NAME, rev) ((val) & ~NAME##_MASK(rev))
#define BCM_SBIT_REV(NAME, rev)      (1 << NAME##_SHIFT(rev))
#define BCM_TBF_REV(val, NAME, rev)  ((val) & NAME##_MASK(rev))
#define BCM_TBIT_REV(val, NAME, rev) ((val) & NAME##_MASK(rev))
#define BCM_MASK_REV(NAME, rev)      (((1 << NAME##_NBITS(rev)) - 1) << NAME##_SHIFT(rev))

#endif /* BCM_BIT_MANIP_MACROS */

#ifndef NBU32
#define NBU32 (sizeof(uint32)) /* Number of Bytes in a uint32 */
#endif
/* define BCMSMALL to remove misc features for memory-constrained environments */
#ifdef BCMSMALL
#undef	BCMSPACE
#define bcmspace	FALSE	/* if (bcmspace) code is discarded */
#else
#define	BCMSPACE
#define bcmspace	TRUE	/* if (bcmspace) code is retained */
#endif

/* Max. nvram variable table size */
#ifndef MAXSZ_NVRAM_VARS
#ifdef LARGE_NVRAM_MAXSZ
#if defined(BCMROMBUILD) || defined(DONGLEBUILD)
#define MAXSZ_NVRAM_VARS	(LARGE_NVRAM_MAXSZ * 2)
#else
/* Double the space to store both OTP/SROM and NVRAM variables in NIC mode driver */
#define MAXSZ_NVRAM_VARS        (LARGE_NVRAM_MAXSZ * 4)
#endif /* BCMROMBUILD || DONGLEBUILD */
#else /* !LARGE_NVRAM_MAXSZ */
#if defined(BCMROMBUILD) || defined(DONGLEBUILD)
/* SROM12 changes */
#define	MAXSZ_NVRAM_VARS	6144
#else /* non-firmware builds (e.g. NIC builds) */
#define LARGE_NVRAM_MAXSZ	16384   /* for SROM19/.11be */
#define MAXSZ_NVRAM_VARS	(LARGE_NVRAM_MAXSZ * 4)
#endif /* BCMROMBUILD || DONGLEBUILD */
#endif /* LARGE_NVRAM_MAXSZ */
#endif /* !MAXSZ_NVRAM_VARS */

#ifdef ATE_BUILD
#ifndef ATE_NVRAM_MAXSIZE
#define ATE_NVRAM_MAXSIZE 64000
#endif /* ATE_NVRAM_MAXSIZE */
#endif /* ATE_BUILD */

/* ROM_ENAB_RUNTIME_CHECK may be set based upon the #define below (for ROM builds). It may also
 * be defined via makefiles (e.g. ROM auto abandon unoptimized compiles).
 */
#if defined(BCMROMBUILD)
	#ifndef ROM_ENAB_RUNTIME_CHECK
		#define ROM_ENAB_RUNTIME_CHECK
	#endif
#endif /* BCMROMBUILD */

#ifdef BCMLFRAG /* BCMLFRAG support enab macros  */
	extern bool _bcmlfrag;
	#if defined(ROM_ENAB_RUNTIME_CHECK) || !defined(DONGLEBUILD)
		#define BCMLFRAG_ENAB() (_bcmlfrag)
	#elif defined(BCMLFRAG_DISABLED)
		#define BCMLFRAG_ENAB()	(0)
	#else
		#define BCMLFRAG_ENAB()	(1)
	#endif
#else
	#define BCMLFRAG_ENAB()		(0)
#endif /* BCMLFRAG_ENAB */

#ifdef BCMPCIEDEV /* BCMPCIEDEV support enab macros */
extern bool _pciedevenab;
	#if defined(ROM_ENAB_RUNTIME_CHECK)
		#define BCMPCIEDEV_ENAB() (_pciedevenab)
	#elif defined(BCMPCIEDEV_ENABLED)
		#define BCMPCIEDEV_ENAB()	1
	#else
		#define BCMPCIEDEV_ENAB()	0
	#endif
#else
	#define BCMPCIEDEV_ENAB()	0
#endif /* BCMPCIEDEV */

#ifdef BCMRESVFRAGPOOL /* BCMRESVFRAGPOOL support enab macros */
extern bool _resvfragpool_enab;
	#if defined(ROM_ENAB_RUNTIME_CHECK) || !defined(DONGLEBUILD)
		#define  BCMRESVFRAGPOOL_ENAB() (_resvfragpool_enab)
	#elif defined(BCMRESVFRAGPOOL_ENABLED)
		#define BCMRESVFRAGPOOL_ENAB()	1
	#else
		#define BCMRESVFRAGPOOL_ENAB()	0
	#endif
#else
	#define BCMRESVFRAGPOOL_ENAB()	0
#endif /* BCMPCIEDEV */

	#define BCMSDIODEV_ENAB()	0

/* Max size for reclaimable NVRAM array */
#ifndef ATE_BUILD
#ifdef DL_NVRAM
#define NVRAM_ARRAY_MAXSIZE	DL_NVRAM
#else
#define NVRAM_ARRAY_MAXSIZE	MAXSZ_NVRAM_VARS
#endif /* DL_NVRAM */
#else
#define NVRAM_ARRAY_MAXSIZE	ATE_NVRAM_MAXSIZE
#endif /* ATE_BUILD */

extern uint32 gFWID;

/* Chip related low power flags (lpflags) */

#ifndef PAD
#define _PADLINE(line)  pad ## line
#define _XSTR(line)     _PADLINE(line)
#define PAD             _XSTR(__LINE__)
#endif

/* Reserved fields (or padding) in structures/unions */
#ifndef RSVD_FIELD
#define _RSVD_LINE(line)    rsvd ## line
#define _RSVD_STR(line)     _RSVD_LINE(line)
#define RSVD_FIELD          _RSVD_STR(__LINE__)
#endif /* RSVD_FIELD */

#ifndef EXPAND
/* macro EXPAND() can be used to print the value of #defines at preprocessor compile time
 * examples:
 * #pragma message "The value of BCMQT: " EXPAND(BCMQT)
 * #pragma message "The value of WL_PRINT(): " EXPAND(WL_PRINT(0))
 */
#define EXPANDX(x) #x
#define EXPAND(x) EXPANDX(x)
#endif /* EXPAND */

#if defined(DONGLEBUILD) && !defined(__COVERITY__)
#define MODULE_DETACH(var, detach_func)\
	do { \
		BCM_REFERENCE(detach_func); \
		OSL_SYS_HALT(); \
	} while (0);
#define MODULE_DETACH_2(var1, var2, detach_func) MODULE_DETACH(var1, detach_func)
#define MODULE_DETACH_TYPECASTED(var, detach_func)
#else
#define MODULE_DETACH(var, detach_func)\
	if (var) { \
		detach_func(var); \
		(var) = NULL; \
	}
#define MODULE_DETACH_2(var1, var2, detach_func) detach_func(var1, var2)
#define MODULE_DETACH_TYPECASTED(var, detach_func) detach_func(var)
#endif /* DONGLEBUILD */

/* When building ROML image use runtime conditional to cause the compiler
 * to compile everything but not to complain "defined but not used"
 * as #ifdef would cause at the callsites.
 * In the end functions called under if (0) {} will not be linked
 * into the final binary if they're not called from other places either.
 */
#if !defined(BCMROMBUILD) || defined(BCMROMSYMGEN_BUILD)
#define BCM_ATTACH_REF_DECL()
#define BCM_ATTACH_REF()	(1)
#else
#define BCM_ATTACH_REF_DECL()	static bool bcm_non_roml_build = 0;
#define BCM_ATTACH_REF()	(bcm_non_roml_build)
#endif

#define ROMCONST	CONST

/* Helper for named objects */
#ifdef BCM_OBJECT_ID
#define OBJECT_ID(obj)	(#obj)
#else
#define OBJECT_ID(obj)	NULL
#endif /* BCM_OBJECT_ID */

#if !defined(DONGLEBUILD) && !defined(BCMDONGLEHOST)
#if defined(CONFIG_BCM963178) || defined(CONFIG_BCM947622)
#define WLATF_BARE
#endif /* CONFIG_BCM963178 || CONFIG_BCM947622 */
#endif /* !DONGLEBUILD && !BCMDONGLEHOST */

/*
 * assure BCM_ROUTER is properly defined for user-space apps
 * when building for router. It aims to consolidate various
 * router definitions, but for now we need this construct as
 * we must built against various P4 branches.
 */
#if !defined(BCMDRIVER) && !defined(BCM_ROUTER)
#ifdef BCA_HNDROUTER
#define BCM_ROUTER
#endif
#endif /* !BCMDRIVER && !BCM_ROUTER */

/* Time */
typedef uint32 time_ms_t;
typedef uint32 time_us_t;
typedef uint64 time64_ms_t;
typedef uint64 time64_us_t;

#endif /* _bcmdefs_h_ */
