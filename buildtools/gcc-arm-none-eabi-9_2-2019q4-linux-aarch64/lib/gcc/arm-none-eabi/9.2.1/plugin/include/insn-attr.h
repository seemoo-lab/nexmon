/* Generated automatically by the program `genattr'
   from the machine description file `md'.  */

#ifndef GCC_INSN_ATTR_H
#define GCC_INSN_ATTR_H

#include "insn-attr-common.h"

#define HAVE_ATTR_nonce_enabled 1
extern enum attr_nonce_enabled get_attr_nonce_enabled (rtx_insn *);

#define HAVE_ATTR_ce_enabled 1
extern enum attr_ce_enabled get_attr_ce_enabled (rtx_insn *);

#define HAVE_ATTR_tune 1
extern enum attr_tune get_attr_tune (void);

#define HAVE_ATTR_type 1
extern enum attr_type get_attr_type (rtx_insn *);

#define HAVE_ATTR_mul32 1
extern enum attr_mul32 get_attr_mul32 (rtx_insn *);

#define HAVE_ATTR_widen_mul64 1
extern enum attr_widen_mul64 get_attr_widen_mul64 (rtx_insn *);

#define HAVE_ATTR_is_neon_type 1
extern enum attr_is_neon_type get_attr_is_neon_type (rtx_insn *);

#define HAVE_ATTR_is_thumb 1
extern enum attr_is_thumb get_attr_is_thumb (void);

#define HAVE_ATTR_is_arch6 1
extern enum attr_is_arch6 get_attr_is_arch6 (void);

#define HAVE_ATTR_is_thumb1 1
extern enum attr_is_thumb1 get_attr_is_thumb1 (void);

#define HAVE_ATTR_predicable_short_it 1
extern enum attr_predicable_short_it get_attr_predicable_short_it (rtx_insn *);

#define HAVE_ATTR_enabled_for_short_it 1
extern enum attr_enabled_for_short_it get_attr_enabled_for_short_it (rtx_insn *);

#define HAVE_ATTR_shift 1
extern int get_attr_shift (rtx_insn *);
#define HAVE_ATTR_fp 1
extern enum attr_fp get_attr_fp (rtx_insn *);

#define HAVE_ATTR_fpu 1
extern enum attr_fpu get_attr_fpu (void);

#define HAVE_ATTR_predicated 1
extern enum attr_predicated get_attr_predicated (rtx_insn *);

#define HAVE_ATTR_length 1
extern int get_attr_length (rtx_insn *);
extern void shorten_branches (rtx_insn *);
extern int insn_default_length (rtx_insn *);
extern int insn_min_length (rtx_insn *);
extern int insn_variable_length_p (rtx_insn *);
extern int insn_current_length (rtx_insn *);

#include "insn-addr.h"

#define HAVE_ATTR_arch 1
extern enum attr_arch get_attr_arch (rtx_insn *);

#define HAVE_ATTR_arch_enabled 1
extern enum attr_arch_enabled get_attr_arch_enabled (rtx_insn *);

#define HAVE_ATTR_opt 1
extern enum attr_opt get_attr_opt (rtx_insn *);

#define HAVE_ATTR_opt_enabled 1
extern enum attr_opt_enabled get_attr_opt_enabled (rtx_insn *);

#define HAVE_ATTR_use_literal_pool 1
extern enum attr_use_literal_pool get_attr_use_literal_pool (rtx_insn *);

#define HAVE_ATTR_enabled 1
extern enum attr_enabled get_attr_enabled (rtx_insn *);

#define HAVE_ATTR_arm_pool_range 1
extern int get_attr_arm_pool_range (rtx_insn *);
#define HAVE_ATTR_thumb2_pool_range 1
extern int get_attr_thumb2_pool_range (rtx_insn *);
#define HAVE_ATTR_arm_neg_pool_range 1
extern int get_attr_arm_neg_pool_range (rtx_insn *);
#define HAVE_ATTR_thumb2_neg_pool_range 1
extern int get_attr_thumb2_neg_pool_range (rtx_insn *);
#define HAVE_ATTR_pool_range 1
extern int get_attr_pool_range (rtx_insn *);
#define HAVE_ATTR_neg_pool_range 1
extern int get_attr_neg_pool_range (rtx_insn *);
#define HAVE_ATTR_ldsched 1
extern enum attr_ldsched get_attr_ldsched (void);

#define HAVE_ATTR_conds 1
extern enum attr_conds get_attr_conds (rtx_insn *);

#define HAVE_ATTR_predicable 1
extern enum attr_predicable get_attr_predicable (rtx_insn *);

#define HAVE_ATTR_model_wbuf 1
extern enum attr_model_wbuf get_attr_model_wbuf (void);

#define HAVE_ATTR_write_conflict 1
extern enum attr_write_conflict get_attr_write_conflict (rtx_insn *);

#define HAVE_ATTR_core_cycles 1
extern enum attr_core_cycles get_attr_core_cycles (rtx_insn *);

#define HAVE_ATTR_far_jump 1
extern enum attr_far_jump get_attr_far_jump (rtx_insn *);

#define HAVE_ATTR_ce_count 1
extern int get_attr_ce_count (rtx_insn *);
#define HAVE_ATTR_tune_cortexr4 1
extern enum attr_tune_cortexr4 get_attr_tune_cortexr4 (void);

#define HAVE_ATTR_generic_sched 1
extern enum attr_generic_sched get_attr_generic_sched (void);

#define HAVE_ATTR_generic_vfp 1
extern enum attr_generic_vfp get_attr_generic_vfp (void);

#define HAVE_ATTR_marvell_f_iwmmxt 1
extern enum attr_marvell_f_iwmmxt get_attr_marvell_f_iwmmxt (void);

#define HAVE_ATTR_wmmxt_shift 1
extern enum attr_wmmxt_shift get_attr_wmmxt_shift (rtx_insn *);

#define HAVE_ATTR_wmmxt_pack 1
extern enum attr_wmmxt_pack get_attr_wmmxt_pack (rtx_insn *);

#define HAVE_ATTR_wmmxt_mult_c1 1
extern enum attr_wmmxt_mult_c1 get_attr_wmmxt_mult_c1 (rtx_insn *);

#define HAVE_ATTR_wmmxt_mult_c2 1
extern enum attr_wmmxt_mult_c2 get_attr_wmmxt_mult_c2 (rtx_insn *);

#define HAVE_ATTR_wmmxt_alu_c1 1
extern enum attr_wmmxt_alu_c1 get_attr_wmmxt_alu_c1 (rtx_insn *);

#define HAVE_ATTR_wmmxt_alu_c2 1
extern enum attr_wmmxt_alu_c2 get_attr_wmmxt_alu_c2 (rtx_insn *);

#define HAVE_ATTR_wmmxt_alu_c3 1
extern enum attr_wmmxt_alu_c3 get_attr_wmmxt_alu_c3 (rtx_insn *);

#define HAVE_ATTR_wmmxt_transfer_c1 1
extern enum attr_wmmxt_transfer_c1 get_attr_wmmxt_transfer_c1 (rtx_insn *);

#define HAVE_ATTR_wmmxt_transfer_c2 1
extern enum attr_wmmxt_transfer_c2 get_attr_wmmxt_transfer_c2 (rtx_insn *);

#define HAVE_ATTR_wmmxt_transfer_c3 1
extern enum attr_wmmxt_transfer_c3 get_attr_wmmxt_transfer_c3 (rtx_insn *);

#define HAVE_ATTR_vfp10 1
extern enum attr_vfp10 get_attr_vfp10 (void);

#define HAVE_ATTR_cortex_a7_neon_type 1
extern enum attr_cortex_a7_neon_type get_attr_cortex_a7_neon_type (rtx_insn *);

#define HAVE_ATTR_cortex_a8_neon_type 1
extern enum attr_cortex_a8_neon_type get_attr_cortex_a8_neon_type (rtx_insn *);

#define HAVE_ATTR_cortex_a9_neon_type 1
extern enum attr_cortex_a9_neon_type get_attr_cortex_a9_neon_type (rtx_insn *);

#define HAVE_ATTR_cortex_a15_neon_type 1
extern enum attr_cortex_a15_neon_type get_attr_cortex_a15_neon_type (rtx_insn *);

#define HAVE_ATTR_cortex_a17_neon_type 1
extern enum attr_cortex_a17_neon_type get_attr_cortex_a17_neon_type (rtx_insn *);

#define HAVE_ATTR_cortex_a53_advsimd_type 1
extern enum attr_cortex_a53_advsimd_type get_attr_cortex_a53_advsimd_type (rtx_insn *);

#define HAVE_ATTR_cortex_a57_neon_type 1
extern enum attr_cortex_a57_neon_type get_attr_cortex_a57_neon_type (rtx_insn *);

#define HAVE_ATTR_exynos_m1_neon_type 1
extern enum attr_exynos_m1_neon_type get_attr_exynos_m1_neon_type (rtx_insn *);

#define HAVE_ATTR_vqh_mnem 1
extern enum attr_vqh_mnem get_attr_vqh_mnem (rtx_insn *);

extern int num_delay_slots (rtx_insn *);
extern int eligible_for_delay (rtx_insn *, int, rtx_insn *, int);

extern int const_num_delay_slots (rtx_insn *);

#define ANNUL_IFTRUE_SLOTS 0
extern int eligible_for_annul_true (rtx_insn *, int, rtx_insn *, int);
#define ANNUL_IFFALSE_SLOTS 0
extern int eligible_for_annul_false (rtx_insn *, int, rtx_insn *, int);

/* DFA based pipeline interface.  */
#ifndef AUTOMATON_ALTS
#define AUTOMATON_ALTS 0
#endif


#ifndef AUTOMATON_STATE_ALTS
#define AUTOMATON_STATE_ALTS 0
#endif

#ifndef CPU_UNITS_QUERY
#define CPU_UNITS_QUERY 0
#endif

#define init_sched_attrs() do { } while (0)

/* Internal insn code number used by automata.  */
extern int internal_dfa_insn_code (rtx_insn *);

/* Insn latency time defined in define_insn_reservation. */
extern int insn_default_latency (rtx_insn *);

/* Return nonzero if there is a bypass for given insn
   which is a data producer.  */
extern int bypass_p (rtx_insn *);

/* Insn latency time on data consumed by the 2nd insn.
   Use the function if bypass_p returns nonzero for
   the 1st insn. */
extern int insn_latency (rtx_insn *, rtx_insn *);

/* Maximal insn latency time possible of all bypasses for this insn.
   Use the function if bypass_p returns nonzero for
   the 1st insn. */
extern int maximal_insn_latency (rtx_insn *);


#if AUTOMATON_ALTS
/* The following function returns number of alternative
   reservations of given insn.  It may be used for better
   insns scheduling heuristics. */
extern int insn_alts (rtx);

#endif

/* Maximal possible number of insns waiting results being
   produced by insns whose execution is not finished. */
extern const int max_insn_queue_index;

/* Pointer to data describing current state of DFA.  */
typedef void *state_t;

/* Size of the data in bytes.  */
extern int state_size (void);

/* Initiate given DFA state, i.e. Set up the state
   as all functional units were not reserved.  */
extern void state_reset (state_t);
/* The following function returns negative value if given
   insn can be issued in processor state described by given
   DFA state.  In this case, the DFA state is changed to
   reflect the current and future reservations by given
   insn.  Otherwise the function returns minimal time
   delay to issue the insn.  This delay may be zero
   for superscalar or VLIW processors.  If the second
   parameter is NULL the function changes given DFA state
   as new processor cycle started.  */
extern int state_transition (state_t, rtx);

#if AUTOMATON_STATE_ALTS
/* The following function returns number of possible
   alternative reservations of given insn in given
   DFA state.  It may be used for better insns scheduling
   heuristics.  By default the function is defined if
   macro AUTOMATON_STATE_ALTS is defined because its
   implementation may require much memory.  */
extern int state_alts (state_t, rtx);
#endif

extern int min_issue_delay (state_t, rtx_insn *);
/* The following function returns nonzero if no one insn
   can be issued in current DFA state. */
extern int state_dead_lock_p (state_t);
/* The function returns minimal delay of issue of the 2nd
   insn after issuing the 1st insn in given DFA state.
   The 1st insn should be issued in given state (i.e.
    state_transition should return negative value for
    the insn and the state).  Data dependencies between
    the insns are ignored by the function.  */
extern int min_insn_conflict_delay (state_t, rtx_insn *, rtx_insn *);
/* The following function outputs reservations for given
   insn as they are described in the corresponding
   define_insn_reservation.  */
extern void print_reservation (FILE *, rtx_insn *);

#if CPU_UNITS_QUERY
/* The following function returns code of functional unit
   with given name (see define_cpu_unit). */
extern int get_cpu_unit_code (const char *);
/* The following function returns nonzero if functional
   unit with given code is currently reserved in given
   DFA state.  */
extern int cpu_unit_reservation_p (state_t, int);
#endif

/* The following function returns true if insn
   has a dfa reservation.  */
extern bool insn_has_dfa_reservation_p (rtx_insn *);

/* Clean insn code cache.  It should be called if there
   is a chance that condition value in a
   define_insn_reservation will be changed after
   last call of dfa_start.  */
extern void dfa_clean_insn_cache (void);

extern void dfa_clear_single_insn_cache (rtx_insn *);

/* Initiate and finish work with DFA.  They should be
   called as the first and the last interface
   functions.  */
extern void dfa_start (void);
extern void dfa_finish (void);
#ifndef HAVE_ATTR_length
#define HAVE_ATTR_length 0
#endif
#ifndef HAVE_ATTR_enabled
#define HAVE_ATTR_enabled 0
#endif
#ifndef HAVE_ATTR_preferred_for_size
#define HAVE_ATTR_preferred_for_size 0
#endif
#ifndef HAVE_ATTR_preferred_for_speed
#define HAVE_ATTR_preferred_for_speed 0
#endif
#if !HAVE_ATTR_length
extern int hook_int_rtx_insn_unreachable (rtx_insn *);
#define insn_default_length hook_int_rtx_insn_unreachable
#define insn_min_length hook_int_rtx_insn_unreachable
#define insn_variable_length_p hook_int_rtx_insn_unreachable
#define insn_current_length hook_int_rtx_insn_unreachable
#include "insn-addr.h"
#endif
extern int hook_int_rtx_1 (rtx);
#if !HAVE_ATTR_enabled
#define get_attr_enabled hook_int_rtx_1
#endif
#if !HAVE_ATTR_preferred_for_size
#define get_attr_preferred_for_size hook_int_rtx_1
#endif
#if !HAVE_ATTR_preferred_for_speed
#define get_attr_preferred_for_speed hook_int_rtx_1
#endif


#define ATTR_FLAG_forward	0x1
#define ATTR_FLAG_backward	0x2

#endif /* GCC_INSN_ATTR_H */
