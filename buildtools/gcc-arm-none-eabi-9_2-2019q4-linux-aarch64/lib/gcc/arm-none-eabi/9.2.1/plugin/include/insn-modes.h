/* Generated automatically from machmode.def and config/arm/arm-modes.def
   by genmodes.  */

#ifndef GCC_INSN_MODES_H
#define GCC_INSN_MODES_H

enum machine_mode
{
  E_VOIDmode,              /* machmode.def:189 */
#define HAVE_VOIDmode
#ifdef USE_ENUM_MODES
#define VOIDmode E_VOIDmode
#else
#define VOIDmode ((void) 0, E_VOIDmode)
#endif
  E_BLKmode,               /* machmode.def:193 */
#define HAVE_BLKmode
#ifdef USE_ENUM_MODES
#define BLKmode E_BLKmode
#else
#define BLKmode ((void) 0, E_BLKmode)
#endif
  E_CCmode,                /* machmode.def:231 */
#define HAVE_CCmode
#ifdef USE_ENUM_MODES
#define CCmode E_CCmode
#else
#define CCmode ((void) 0, E_CCmode)
#endif
  E_CC_NOOVmode,           /* config/arm/arm-modes.def:43 */
#define HAVE_CC_NOOVmode
#ifdef USE_ENUM_MODES
#define CC_NOOVmode E_CC_NOOVmode
#else
#define CC_NOOVmode ((void) 0, E_CC_NOOVmode)
#endif
  E_CC_Zmode,              /* config/arm/arm-modes.def:44 */
#define HAVE_CC_Zmode
#ifdef USE_ENUM_MODES
#define CC_Zmode E_CC_Zmode
#else
#define CC_Zmode ((void) 0, E_CC_Zmode)
#endif
  E_CC_CZmode,             /* config/arm/arm-modes.def:45 */
#define HAVE_CC_CZmode
#ifdef USE_ENUM_MODES
#define CC_CZmode E_CC_CZmode
#else
#define CC_CZmode ((void) 0, E_CC_CZmode)
#endif
  E_CC_NCVmode,            /* config/arm/arm-modes.def:46 */
#define HAVE_CC_NCVmode
#ifdef USE_ENUM_MODES
#define CC_NCVmode E_CC_NCVmode
#else
#define CC_NCVmode ((void) 0, E_CC_NCVmode)
#endif
  E_CC_SWPmode,            /* config/arm/arm-modes.def:47 */
#define HAVE_CC_SWPmode
#ifdef USE_ENUM_MODES
#define CC_SWPmode E_CC_SWPmode
#else
#define CC_SWPmode ((void) 0, E_CC_SWPmode)
#endif
  E_CCFPmode,              /* config/arm/arm-modes.def:48 */
#define HAVE_CCFPmode
#ifdef USE_ENUM_MODES
#define CCFPmode E_CCFPmode
#else
#define CCFPmode ((void) 0, E_CCFPmode)
#endif
  E_CCFPEmode,             /* config/arm/arm-modes.def:49 */
#define HAVE_CCFPEmode
#ifdef USE_ENUM_MODES
#define CCFPEmode E_CCFPEmode
#else
#define CCFPEmode ((void) 0, E_CCFPEmode)
#endif
  E_CC_DNEmode,            /* config/arm/arm-modes.def:50 */
#define HAVE_CC_DNEmode
#ifdef USE_ENUM_MODES
#define CC_DNEmode E_CC_DNEmode
#else
#define CC_DNEmode ((void) 0, E_CC_DNEmode)
#endif
  E_CC_DEQmode,            /* config/arm/arm-modes.def:51 */
#define HAVE_CC_DEQmode
#ifdef USE_ENUM_MODES
#define CC_DEQmode E_CC_DEQmode
#else
#define CC_DEQmode ((void) 0, E_CC_DEQmode)
#endif
  E_CC_DLEmode,            /* config/arm/arm-modes.def:52 */
#define HAVE_CC_DLEmode
#ifdef USE_ENUM_MODES
#define CC_DLEmode E_CC_DLEmode
#else
#define CC_DLEmode ((void) 0, E_CC_DLEmode)
#endif
  E_CC_DLTmode,            /* config/arm/arm-modes.def:53 */
#define HAVE_CC_DLTmode
#ifdef USE_ENUM_MODES
#define CC_DLTmode E_CC_DLTmode
#else
#define CC_DLTmode ((void) 0, E_CC_DLTmode)
#endif
  E_CC_DGEmode,            /* config/arm/arm-modes.def:54 */
#define HAVE_CC_DGEmode
#ifdef USE_ENUM_MODES
#define CC_DGEmode E_CC_DGEmode
#else
#define CC_DGEmode ((void) 0, E_CC_DGEmode)
#endif
  E_CC_DGTmode,            /* config/arm/arm-modes.def:55 */
#define HAVE_CC_DGTmode
#ifdef USE_ENUM_MODES
#define CC_DGTmode E_CC_DGTmode
#else
#define CC_DGTmode ((void) 0, E_CC_DGTmode)
#endif
  E_CC_DLEUmode,           /* config/arm/arm-modes.def:56 */
#define HAVE_CC_DLEUmode
#ifdef USE_ENUM_MODES
#define CC_DLEUmode E_CC_DLEUmode
#else
#define CC_DLEUmode ((void) 0, E_CC_DLEUmode)
#endif
  E_CC_DLTUmode,           /* config/arm/arm-modes.def:57 */
#define HAVE_CC_DLTUmode
#ifdef USE_ENUM_MODES
#define CC_DLTUmode E_CC_DLTUmode
#else
#define CC_DLTUmode ((void) 0, E_CC_DLTUmode)
#endif
  E_CC_DGEUmode,           /* config/arm/arm-modes.def:58 */
#define HAVE_CC_DGEUmode
#ifdef USE_ENUM_MODES
#define CC_DGEUmode E_CC_DGEUmode
#else
#define CC_DGEUmode ((void) 0, E_CC_DGEUmode)
#endif
  E_CC_DGTUmode,           /* config/arm/arm-modes.def:59 */
#define HAVE_CC_DGTUmode
#ifdef USE_ENUM_MODES
#define CC_DGTUmode E_CC_DGTUmode
#else
#define CC_DGTUmode ((void) 0, E_CC_DGTUmode)
#endif
  E_CC_Cmode,              /* config/arm/arm-modes.def:60 */
#define HAVE_CC_Cmode
#ifdef USE_ENUM_MODES
#define CC_Cmode E_CC_Cmode
#else
#define CC_Cmode ((void) 0, E_CC_Cmode)
#endif
  E_CC_Nmode,              /* config/arm/arm-modes.def:61 */
#define HAVE_CC_Nmode
#ifdef USE_ENUM_MODES
#define CC_Nmode E_CC_Nmode
#else
#define CC_Nmode ((void) 0, E_CC_Nmode)
#endif
  E_CC_Vmode,              /* config/arm/arm-modes.def:62 */
#define HAVE_CC_Vmode
#ifdef USE_ENUM_MODES
#define CC_Vmode E_CC_Vmode
#else
#define CC_Vmode ((void) 0, E_CC_Vmode)
#endif
  E_BImode,                /* machmode.def:196 */
#define HAVE_BImode
#ifdef USE_ENUM_MODES
#define BImode E_BImode
#else
#define BImode (scalar_int_mode ((scalar_int_mode::from_int) E_BImode))
#endif
  E_QImode,                /* machmode.def:204 */
#define HAVE_QImode
#ifdef USE_ENUM_MODES
#define QImode E_QImode
#else
#define QImode (scalar_int_mode ((scalar_int_mode::from_int) E_QImode))
#endif
  E_HImode,                /* machmode.def:205 */
#define HAVE_HImode
#ifdef USE_ENUM_MODES
#define HImode E_HImode
#else
#define HImode (scalar_int_mode ((scalar_int_mode::from_int) E_HImode))
#endif
  E_SImode,                /* machmode.def:206 */
#define HAVE_SImode
#ifdef USE_ENUM_MODES
#define SImode E_SImode
#else
#define SImode (scalar_int_mode ((scalar_int_mode::from_int) E_SImode))
#endif
  E_DImode,                /* machmode.def:207 */
#define HAVE_DImode
#ifdef USE_ENUM_MODES
#define DImode E_DImode
#else
#define DImode (scalar_int_mode ((scalar_int_mode::from_int) E_DImode))
#endif
  E_TImode,                /* machmode.def:208 */
#define HAVE_TImode
#ifdef USE_ENUM_MODES
#define TImode E_TImode
#else
#define TImode (scalar_int_mode ((scalar_int_mode::from_int) E_TImode))
#endif
  E_EImode,                /* config/arm/arm-modes.def:80 */
#define HAVE_EImode
#ifdef USE_ENUM_MODES
#define EImode E_EImode
#else
#define EImode (scalar_int_mode ((scalar_int_mode::from_int) E_EImode))
#endif
  E_OImode,                /* config/arm/arm-modes.def:81 */
#define HAVE_OImode
#ifdef USE_ENUM_MODES
#define OImode E_OImode
#else
#define OImode (scalar_int_mode ((scalar_int_mode::from_int) E_OImode))
#endif
  E_CImode,                /* config/arm/arm-modes.def:82 */
#define HAVE_CImode
#ifdef USE_ENUM_MODES
#define CImode E_CImode
#else
#define CImode (scalar_int_mode ((scalar_int_mode::from_int) E_CImode))
#endif
  E_XImode,                /* config/arm/arm-modes.def:83 */
#define HAVE_XImode
#ifdef USE_ENUM_MODES
#define XImode E_XImode
#else
#define XImode (scalar_int_mode ((scalar_int_mode::from_int) E_XImode))
#endif
  E_QQmode,                /* machmode.def:234 */
#define HAVE_QQmode
#ifdef USE_ENUM_MODES
#define QQmode E_QQmode
#else
#define QQmode (scalar_mode ((scalar_mode::from_int) E_QQmode))
#endif
  E_HQmode,                /* machmode.def:235 */
#define HAVE_HQmode
#ifdef USE_ENUM_MODES
#define HQmode E_HQmode
#else
#define HQmode (scalar_mode ((scalar_mode::from_int) E_HQmode))
#endif
  E_SQmode,                /* machmode.def:236 */
#define HAVE_SQmode
#ifdef USE_ENUM_MODES
#define SQmode E_SQmode
#else
#define SQmode (scalar_mode ((scalar_mode::from_int) E_SQmode))
#endif
  E_DQmode,                /* machmode.def:237 */
#define HAVE_DQmode
#ifdef USE_ENUM_MODES
#define DQmode E_DQmode
#else
#define DQmode (scalar_mode ((scalar_mode::from_int) E_DQmode))
#endif
  E_TQmode,                /* machmode.def:238 */
#define HAVE_TQmode
#ifdef USE_ENUM_MODES
#define TQmode E_TQmode
#else
#define TQmode (scalar_mode ((scalar_mode::from_int) E_TQmode))
#endif
  E_UQQmode,               /* machmode.def:240 */
#define HAVE_UQQmode
#ifdef USE_ENUM_MODES
#define UQQmode E_UQQmode
#else
#define UQQmode (scalar_mode ((scalar_mode::from_int) E_UQQmode))
#endif
  E_UHQmode,               /* machmode.def:241 */
#define HAVE_UHQmode
#ifdef USE_ENUM_MODES
#define UHQmode E_UHQmode
#else
#define UHQmode (scalar_mode ((scalar_mode::from_int) E_UHQmode))
#endif
  E_USQmode,               /* machmode.def:242 */
#define HAVE_USQmode
#ifdef USE_ENUM_MODES
#define USQmode E_USQmode
#else
#define USQmode (scalar_mode ((scalar_mode::from_int) E_USQmode))
#endif
  E_UDQmode,               /* machmode.def:243 */
#define HAVE_UDQmode
#ifdef USE_ENUM_MODES
#define UDQmode E_UDQmode
#else
#define UDQmode (scalar_mode ((scalar_mode::from_int) E_UDQmode))
#endif
  E_UTQmode,               /* machmode.def:244 */
#define HAVE_UTQmode
#ifdef USE_ENUM_MODES
#define UTQmode E_UTQmode
#else
#define UTQmode (scalar_mode ((scalar_mode::from_int) E_UTQmode))
#endif
  E_HAmode,                /* machmode.def:246 */
#define HAVE_HAmode
#ifdef USE_ENUM_MODES
#define HAmode E_HAmode
#else
#define HAmode (scalar_mode ((scalar_mode::from_int) E_HAmode))
#endif
  E_SAmode,                /* machmode.def:247 */
#define HAVE_SAmode
#ifdef USE_ENUM_MODES
#define SAmode E_SAmode
#else
#define SAmode (scalar_mode ((scalar_mode::from_int) E_SAmode))
#endif
  E_DAmode,                /* machmode.def:248 */
#define HAVE_DAmode
#ifdef USE_ENUM_MODES
#define DAmode E_DAmode
#else
#define DAmode (scalar_mode ((scalar_mode::from_int) E_DAmode))
#endif
  E_TAmode,                /* machmode.def:249 */
#define HAVE_TAmode
#ifdef USE_ENUM_MODES
#define TAmode E_TAmode
#else
#define TAmode (scalar_mode ((scalar_mode::from_int) E_TAmode))
#endif
  E_UHAmode,               /* machmode.def:251 */
#define HAVE_UHAmode
#ifdef USE_ENUM_MODES
#define UHAmode E_UHAmode
#else
#define UHAmode (scalar_mode ((scalar_mode::from_int) E_UHAmode))
#endif
  E_USAmode,               /* machmode.def:252 */
#define HAVE_USAmode
#ifdef USE_ENUM_MODES
#define USAmode E_USAmode
#else
#define USAmode (scalar_mode ((scalar_mode::from_int) E_USAmode))
#endif
  E_UDAmode,               /* machmode.def:253 */
#define HAVE_UDAmode
#ifdef USE_ENUM_MODES
#define UDAmode E_UDAmode
#else
#define UDAmode (scalar_mode ((scalar_mode::from_int) E_UDAmode))
#endif
  E_UTAmode,               /* machmode.def:254 */
#define HAVE_UTAmode
#ifdef USE_ENUM_MODES
#define UTAmode E_UTAmode
#else
#define UTAmode (scalar_mode ((scalar_mode::from_int) E_UTAmode))
#endif
  E_HFmode,                /* config/arm/arm-modes.def:26 */
#define HAVE_HFmode
#ifdef USE_ENUM_MODES
#define HFmode E_HFmode
#else
#define HFmode (scalar_float_mode ((scalar_float_mode::from_int) E_HFmode))
#endif
  E_SFmode,                /* machmode.def:226 */
#define HAVE_SFmode
#ifdef USE_ENUM_MODES
#define SFmode E_SFmode
#else
#define SFmode (scalar_float_mode ((scalar_float_mode::from_int) E_SFmode))
#endif
  E_DFmode,                /* machmode.def:227 */
#define HAVE_DFmode
#ifdef USE_ENUM_MODES
#define DFmode E_DFmode
#else
#define DFmode (scalar_float_mode ((scalar_float_mode::from_int) E_DFmode))
#endif
  E_SDmode,                /* machmode.def:267 */
#define HAVE_SDmode
#ifdef USE_ENUM_MODES
#define SDmode E_SDmode
#else
#define SDmode (scalar_float_mode ((scalar_float_mode::from_int) E_SDmode))
#endif
  E_DDmode,                /* machmode.def:268 */
#define HAVE_DDmode
#ifdef USE_ENUM_MODES
#define DDmode E_DDmode
#else
#define DDmode (scalar_float_mode ((scalar_float_mode::from_int) E_DDmode))
#endif
  E_TDmode,                /* machmode.def:269 */
#define HAVE_TDmode
#ifdef USE_ENUM_MODES
#define TDmode E_TDmode
#else
#define TDmode (scalar_float_mode ((scalar_float_mode::from_int) E_TDmode))
#endif
  E_CQImode,               /* machmode.def:262 */
#define HAVE_CQImode
#ifdef USE_ENUM_MODES
#define CQImode E_CQImode
#else
#define CQImode (complex_mode ((complex_mode::from_int) E_CQImode))
#endif
  E_CHImode,               /* machmode.def:262 */
#define HAVE_CHImode
#ifdef USE_ENUM_MODES
#define CHImode E_CHImode
#else
#define CHImode (complex_mode ((complex_mode::from_int) E_CHImode))
#endif
  E_CSImode,               /* machmode.def:262 */
#define HAVE_CSImode
#ifdef USE_ENUM_MODES
#define CSImode E_CSImode
#else
#define CSImode (complex_mode ((complex_mode::from_int) E_CSImode))
#endif
  E_CDImode,               /* machmode.def:262 */
#define HAVE_CDImode
#ifdef USE_ENUM_MODES
#define CDImode E_CDImode
#else
#define CDImode (complex_mode ((complex_mode::from_int) E_CDImode))
#endif
  E_CTImode,               /* machmode.def:262 */
#define HAVE_CTImode
#ifdef USE_ENUM_MODES
#define CTImode E_CTImode
#else
#define CTImode (complex_mode ((complex_mode::from_int) E_CTImode))
#endif
  E_CEImode,               /* machmode.def:262 */
#define HAVE_CEImode
#ifdef USE_ENUM_MODES
#define CEImode E_CEImode
#else
#define CEImode (complex_mode ((complex_mode::from_int) E_CEImode))
#endif
  E_COImode,               /* machmode.def:262 */
#define HAVE_COImode
#ifdef USE_ENUM_MODES
#define COImode E_COImode
#else
#define COImode (complex_mode ((complex_mode::from_int) E_COImode))
#endif
  E_CCImode,               /* machmode.def:262 */
#define HAVE_CCImode
#ifdef USE_ENUM_MODES
#define CCImode E_CCImode
#else
#define CCImode (complex_mode ((complex_mode::from_int) E_CCImode))
#endif
  E_CXImode,               /* machmode.def:262 */
#define HAVE_CXImode
#ifdef USE_ENUM_MODES
#define CXImode E_CXImode
#else
#define CXImode (complex_mode ((complex_mode::from_int) E_CXImode))
#endif
  E_HCmode,                /* machmode.def:264 */
#define HAVE_HCmode
#ifdef USE_ENUM_MODES
#define HCmode E_HCmode
#else
#define HCmode (complex_mode ((complex_mode::from_int) E_HCmode))
#endif
  E_SCmode,                /* machmode.def:264 */
#define HAVE_SCmode
#ifdef USE_ENUM_MODES
#define SCmode E_SCmode
#else
#define SCmode (complex_mode ((complex_mode::from_int) E_SCmode))
#endif
  E_DCmode,                /* machmode.def:264 */
#define HAVE_DCmode
#ifdef USE_ENUM_MODES
#define DCmode E_DCmode
#else
#define DCmode (complex_mode ((complex_mode::from_int) E_DCmode))
#endif
  E_V4QImode,              /* config/arm/arm-modes.def:65 */
#define HAVE_V4QImode
#ifdef USE_ENUM_MODES
#define V4QImode E_V4QImode
#else
#define V4QImode ((void) 0, E_V4QImode)
#endif
  E_V2HImode,              /* config/arm/arm-modes.def:65 */
#define HAVE_V2HImode
#ifdef USE_ENUM_MODES
#define V2HImode E_V2HImode
#else
#define V2HImode ((void) 0, E_V2HImode)
#endif
  E_V8QImode,              /* config/arm/arm-modes.def:66 */
#define HAVE_V8QImode
#ifdef USE_ENUM_MODES
#define V8QImode E_V8QImode
#else
#define V8QImode ((void) 0, E_V8QImode)
#endif
  E_V4HImode,              /* config/arm/arm-modes.def:66 */
#define HAVE_V4HImode
#ifdef USE_ENUM_MODES
#define V4HImode E_V4HImode
#else
#define V4HImode ((void) 0, E_V4HImode)
#endif
  E_V2SImode,              /* config/arm/arm-modes.def:66 */
#define HAVE_V2SImode
#ifdef USE_ENUM_MODES
#define V2SImode E_V2SImode
#else
#define V2SImode ((void) 0, E_V2SImode)
#endif
  E_V16QImode,             /* config/arm/arm-modes.def:67 */
#define HAVE_V16QImode
#ifdef USE_ENUM_MODES
#define V16QImode E_V16QImode
#else
#define V16QImode ((void) 0, E_V16QImode)
#endif
  E_V8HImode,              /* config/arm/arm-modes.def:67 */
#define HAVE_V8HImode
#ifdef USE_ENUM_MODES
#define V8HImode E_V8HImode
#else
#define V8HImode ((void) 0, E_V8HImode)
#endif
  E_V4SImode,              /* config/arm/arm-modes.def:67 */
#define HAVE_V4SImode
#ifdef USE_ENUM_MODES
#define V4SImode E_V4SImode
#else
#define V4SImode ((void) 0, E_V4SImode)
#endif
  E_V2DImode,              /* config/arm/arm-modes.def:67 */
#define HAVE_V2DImode
#ifdef USE_ENUM_MODES
#define V2DImode E_V2DImode
#else
#define V2DImode ((void) 0, E_V2DImode)
#endif
  E_V4QQmode,              /* config/arm/arm-modes.def:73 */
#define HAVE_V4QQmode
#ifdef USE_ENUM_MODES
#define V4QQmode E_V4QQmode
#else
#define V4QQmode ((void) 0, E_V4QQmode)
#endif
  E_V2HQmode,              /* config/arm/arm-modes.def:73 */
#define HAVE_V2HQmode
#ifdef USE_ENUM_MODES
#define V2HQmode E_V2HQmode
#else
#define V2HQmode ((void) 0, E_V2HQmode)
#endif
  E_V4UQQmode,             /* config/arm/arm-modes.def:74 */
#define HAVE_V4UQQmode
#ifdef USE_ENUM_MODES
#define V4UQQmode E_V4UQQmode
#else
#define V4UQQmode ((void) 0, E_V4UQQmode)
#endif
  E_V2UHQmode,             /* config/arm/arm-modes.def:74 */
#define HAVE_V2UHQmode
#ifdef USE_ENUM_MODES
#define V2UHQmode E_V2UHQmode
#else
#define V2UHQmode ((void) 0, E_V2UHQmode)
#endif
  E_V2HAmode,              /* config/arm/arm-modes.def:75 */
#define HAVE_V2HAmode
#ifdef USE_ENUM_MODES
#define V2HAmode E_V2HAmode
#else
#define V2HAmode ((void) 0, E_V2HAmode)
#endif
  E_V2UHAmode,             /* config/arm/arm-modes.def:76 */
#define HAVE_V2UHAmode
#ifdef USE_ENUM_MODES
#define V2UHAmode E_V2UHAmode
#else
#define V2UHAmode ((void) 0, E_V2UHAmode)
#endif
  E_V2HFmode,              /* config/arm/arm-modes.def:70 */
#define HAVE_V2HFmode
#ifdef USE_ENUM_MODES
#define V2HFmode E_V2HFmode
#else
#define V2HFmode ((void) 0, E_V2HFmode)
#endif
  E_V4HFmode,              /* config/arm/arm-modes.def:68 */
#define HAVE_V4HFmode
#ifdef USE_ENUM_MODES
#define V4HFmode E_V4HFmode
#else
#define V4HFmode ((void) 0, E_V4HFmode)
#endif
  E_V2SFmode,              /* config/arm/arm-modes.def:68 */
#define HAVE_V2SFmode
#ifdef USE_ENUM_MODES
#define V2SFmode E_V2SFmode
#else
#define V2SFmode ((void) 0, E_V2SFmode)
#endif
  E_V8HFmode,              /* config/arm/arm-modes.def:69 */
#define HAVE_V8HFmode
#ifdef USE_ENUM_MODES
#define V8HFmode E_V8HFmode
#else
#define V8HFmode ((void) 0, E_V8HFmode)
#endif
  E_V4SFmode,              /* config/arm/arm-modes.def:69 */
#define HAVE_V4SFmode
#ifdef USE_ENUM_MODES
#define V4SFmode E_V4SFmode
#else
#define V4SFmode ((void) 0, E_V4SFmode)
#endif
  E_V2DFmode,              /* config/arm/arm-modes.def:69 */
#define HAVE_V2DFmode
#ifdef USE_ENUM_MODES
#define V2DFmode E_V2DFmode
#else
#define V2DFmode ((void) 0, E_V2DFmode)
#endif
  MAX_MACHINE_MODE,

  MIN_MODE_RANDOM = E_VOIDmode,
  MAX_MODE_RANDOM = E_BLKmode,

  MIN_MODE_CC = E_CCmode,
  MAX_MODE_CC = E_CC_Vmode,

  MIN_MODE_INT = E_QImode,
  MAX_MODE_INT = E_XImode,

  MIN_MODE_PARTIAL_INT = E_VOIDmode,
  MAX_MODE_PARTIAL_INT = E_VOIDmode,

  MIN_MODE_FRACT = E_QQmode,
  MAX_MODE_FRACT = E_TQmode,

  MIN_MODE_UFRACT = E_UQQmode,
  MAX_MODE_UFRACT = E_UTQmode,

  MIN_MODE_ACCUM = E_HAmode,
  MAX_MODE_ACCUM = E_TAmode,

  MIN_MODE_UACCUM = E_UHAmode,
  MAX_MODE_UACCUM = E_UTAmode,

  MIN_MODE_FLOAT = E_HFmode,
  MAX_MODE_FLOAT = E_DFmode,

  MIN_MODE_DECIMAL_FLOAT = E_SDmode,
  MAX_MODE_DECIMAL_FLOAT = E_TDmode,

  MIN_MODE_COMPLEX_INT = E_CQImode,
  MAX_MODE_COMPLEX_INT = E_CXImode,

  MIN_MODE_COMPLEX_FLOAT = E_HCmode,
  MAX_MODE_COMPLEX_FLOAT = E_DCmode,

  MIN_MODE_VECTOR_BOOL = E_VOIDmode,
  MAX_MODE_VECTOR_BOOL = E_VOIDmode,

  MIN_MODE_VECTOR_INT = E_V4QImode,
  MAX_MODE_VECTOR_INT = E_V2DImode,

  MIN_MODE_VECTOR_FRACT = E_V4QQmode,
  MAX_MODE_VECTOR_FRACT = E_V2HQmode,

  MIN_MODE_VECTOR_UFRACT = E_V4UQQmode,
  MAX_MODE_VECTOR_UFRACT = E_V2UHQmode,

  MIN_MODE_VECTOR_ACCUM = E_V2HAmode,
  MAX_MODE_VECTOR_ACCUM = E_V2HAmode,

  MIN_MODE_VECTOR_UACCUM = E_V2UHAmode,
  MAX_MODE_VECTOR_UACCUM = E_V2UHAmode,

  MIN_MODE_VECTOR_FLOAT = E_V2HFmode,
  MAX_MODE_VECTOR_FLOAT = E_V2DFmode,

  NUM_MACHINE_MODES = MAX_MACHINE_MODE
};

#define CONST_MODE_NUNITS const
#define CONST_MODE_PRECISION const
#define CONST_MODE_SIZE const
#define CONST_MODE_UNIT_SIZE const
#define CONST_MODE_BASE_ALIGN const
#define CONST_MODE_IBIT const
#define CONST_MODE_FBIT const

#define BITS_PER_UNIT (8)
#define MAX_BITSIZE_MODE_ANY_INT (64*BITS_PER_UNIT)
#define MAX_BITSIZE_MODE_ANY_MODE (128*BITS_PER_UNIT)
#define NUM_INT_N_ENTS 1
#define NUM_POLY_INT_COEFFS 1

#endif /* insn-modes.h */
