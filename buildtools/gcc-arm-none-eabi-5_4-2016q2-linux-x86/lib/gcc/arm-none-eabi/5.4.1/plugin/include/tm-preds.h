/* Generated automatically by the program 'build/genpreds'
   from the machine description file '/home/build/work/GCC-5-0-build/src/gcc/gcc/config/arm/arm.md'.  */

#ifndef GCC_TM_PREDS_H
#define GCC_TM_PREDS_H

#ifdef HAVE_MACHINE_MODES
extern int general_operand (rtx, machine_mode);
extern int address_operand (rtx, machine_mode);
extern int register_operand (rtx, machine_mode);
extern int pmode_register_operand (rtx, machine_mode);
extern int scratch_operand (rtx, machine_mode);
extern int immediate_operand (rtx, machine_mode);
extern int const_int_operand (rtx, machine_mode);
extern int const_scalar_int_operand (rtx, machine_mode);
extern int const_double_operand (rtx, machine_mode);
extern int nonimmediate_operand (rtx, machine_mode);
extern int nonmemory_operand (rtx, machine_mode);
extern int push_operand (rtx, machine_mode);
extern int pop_operand (rtx, machine_mode);
extern int memory_operand (rtx, machine_mode);
extern int indirect_operand (rtx, machine_mode);
extern int ordered_comparison_operator (rtx, machine_mode);
extern int comparison_operator (rtx, machine_mode);
extern int s_register_operand (rtx, machine_mode);
extern int imm_for_neon_inv_logic_operand (rtx, machine_mode);
extern int neon_inv_logic_op2 (rtx, machine_mode);
extern int imm_for_neon_logic_operand (rtx, machine_mode);
extern int neon_logic_op2 (rtx, machine_mode);
extern int arm_hard_general_register_operand (rtx, machine_mode);
extern int low_register_operand (rtx, machine_mode);
extern int low_reg_or_int_operand (rtx, machine_mode);
extern int arm_general_register_operand (rtx, machine_mode);
extern int vfp_register_operand (rtx, machine_mode);
extern int vfp_hard_register_operand (rtx, machine_mode);
extern int zero_operand (rtx, machine_mode);
extern int reg_or_zero_operand (rtx, machine_mode);
extern int subreg_lowpart_operator (rtx, machine_mode);
extern int reg_or_int_operand (rtx, machine_mode);
extern int arm_immediate_operand (rtx, machine_mode);
extern int arm_immediate_di_operand (rtx, machine_mode);
extern int arm_neg_immediate_operand (rtx, machine_mode);
extern int arm_not_immediate_operand (rtx, machine_mode);
extern int const0_operand (rtx, machine_mode);
extern int arm_rhs_operand (rtx, machine_mode);
extern int arm_rhsm_operand (rtx, machine_mode);
extern int const_int_I_operand (rtx, machine_mode);
extern int const_int_M_operand (rtx, machine_mode);
extern int shift_amount_operand (rtx, machine_mode);
extern int const_neon_scalar_shift_amount_operand (rtx, machine_mode);
extern int ldrd_strd_offset_operand (rtx, machine_mode);
extern int arm_add_operand (rtx, machine_mode);
extern int arm_anddi_operand_neon (rtx, machine_mode);
extern int arm_iordi_operand_neon (rtx, machine_mode);
extern int arm_xordi_operand (rtx, machine_mode);
extern int arm_adddi_operand (rtx, machine_mode);
extern int arm_addimm_operand (rtx, machine_mode);
extern int arm_not_operand (rtx, machine_mode);
extern int arm_di_operand (rtx, machine_mode);
extern int offsettable_memory_operand (rtx, machine_mode);
extern int call_memory_operand (rtx, machine_mode);
extern int arm_reload_memory_operand (rtx, machine_mode);
extern int vfp_compare_operand (rtx, machine_mode);
extern int arm_float_compare_operand (rtx, machine_mode);
extern int index_operand (rtx, machine_mode);
extern int shiftable_operator (rtx, machine_mode);
extern int shiftable_operator_strict_it (rtx, machine_mode);
extern int logical_binary_operator (rtx, machine_mode);
extern int commutative_binary_operator (rtx, machine_mode);
extern int shift_operator (rtx, machine_mode);
extern int shift_nomul_operator (rtx, machine_mode);
extern int sat_shift_operator (rtx, machine_mode);
extern int mult_operator (rtx, machine_mode);
extern int thumb_16bit_operator (rtx, machine_mode);
extern int equality_operator (rtx, machine_mode);
extern int expandable_comparison_operator (rtx, machine_mode);
extern int arm_comparison_operator (rtx, machine_mode);
extern int lt_ge_comparison_operator (rtx, machine_mode);
extern int arm_vsel_comparison_operator (rtx, machine_mode);
extern int arm_cond_move_operator (rtx, machine_mode);
extern int noov_comparison_operator (rtx, machine_mode);
extern int minmax_operator (rtx, machine_mode);
extern int cc_register (rtx, machine_mode);
extern int dominant_cc_register (rtx, machine_mode);
extern int arm_extendqisi_mem_op (rtx, machine_mode);
extern int arm_reg_or_extendqisi_mem_op (rtx, machine_mode);
extern int power_of_two_operand (rtx, machine_mode);
extern int nonimmediate_di_operand (rtx, machine_mode);
extern int di_operand (rtx, machine_mode);
extern int nonimmediate_soft_df_operand (rtx, machine_mode);
extern int soft_df_operand (rtx, machine_mode);
extern int load_multiple_operation (rtx, machine_mode);
extern int store_multiple_operation (rtx, machine_mode);
extern int pop_multiple_return (rtx, machine_mode);
extern int pop_multiple_fp (rtx, machine_mode);
extern int multi_register_push (rtx, machine_mode);
extern int push_mult_memory_operand (rtx, machine_mode);
extern int thumb1_cmp_operand (rtx, machine_mode);
extern int thumb1_cmpneg_operand (rtx, machine_mode);
extern int thumb_cbrch_target_operand (rtx, machine_mode);
extern int imm_or_reg_operand (rtx, machine_mode);
extern int const_multiple_of_8_operand (rtx, machine_mode);
extern int imm_for_neon_mov_operand (rtx, machine_mode);
extern int imm_for_neon_lshift_operand (rtx, machine_mode);
extern int imm_for_neon_rshift_operand (rtx, machine_mode);
extern int imm_lshift_or_reg_neon (rtx, machine_mode);
extern int imm_rshift_or_reg_neon (rtx, machine_mode);
extern int cmpdi_operand (rtx, machine_mode);
extern int arm_sync_memory_operand (rtx, machine_mode);
extern int vect_par_constant_high (rtx, machine_mode);
extern int vect_par_constant_low (rtx, machine_mode);
extern int const_double_vcvt_power_of_two_reciprocal (rtx, machine_mode);
extern int const_double_vcvt_power_of_two (rtx, machine_mode);
extern int neon_struct_operand (rtx, machine_mode);
extern int neon_permissive_struct_operand (rtx, machine_mode);
extern int neon_perm_struct_or_reg_operand (rtx, machine_mode);
extern int add_operator (rtx, machine_mode);
extern int mem_noofs_operand (rtx, machine_mode);
extern int call_insn_operand (rtx, machine_mode);
#endif /* HAVE_MACHINE_MODES */

#define CONSTRAINT_NUM_DEFINED_P 1
enum constraint_num
{
  CONSTRAINT__UNKNOWN = 0,
  CONSTRAINT_r,
  CONSTRAINT_t,
  CONSTRAINT_w,
  CONSTRAINT_x,
  CONSTRAINT_y,
  CONSTRAINT_z,
  CONSTRAINT_l,
  CONSTRAINT_h,
  CONSTRAINT_k,
  CONSTRAINT_q,
  CONSTRAINT_b,
  CONSTRAINT_c,
  CONSTRAINT_Cs,
  CONSTRAINT_Ts,
  CONSTRAINT_Pj,
  CONSTRAINT_PJ,
  CONSTRAINT_I,
  CONSTRAINT_J,
  CONSTRAINT_K,
  CONSTRAINT_L,
  CONSTRAINT_M,
  CONSTRAINT_N,
  CONSTRAINT_O,
  CONSTRAINT_Pa,
  CONSTRAINT_Pb,
  CONSTRAINT_Pc,
  CONSTRAINT_Pd,
  CONSTRAINT_Pe,
  CONSTRAINT_Ps,
  CONSTRAINT_Pt,
  CONSTRAINT_Pu,
  CONSTRAINT_Pv,
  CONSTRAINT_Pw,
  CONSTRAINT_Px,
  CONSTRAINT_Py,
  CONSTRAINT_Pz,
  CONSTRAINT_m,
  CONSTRAINT_o,
  CONSTRAINT_Ua,
  CONSTRAINT_Uh,
  CONSTRAINT_Ut,
  CONSTRAINT_Uv,
  CONSTRAINT_Uy,
  CONSTRAINT_Un,
  CONSTRAINT_Um,
  CONSTRAINT_Us,
  CONSTRAINT_Uq,
  CONSTRAINT_Q,
  CONSTRAINT_Uu,
  CONSTRAINT_Uw,
  CONSTRAINT_p,
  CONSTRAINT_j,
  CONSTRAINT_G,
  CONSTRAINT_Dz,
  CONSTRAINT_Da,
  CONSTRAINT_Db,
  CONSTRAINT_Dc,
  CONSTRAINT_Dd,
  CONSTRAINT_De,
  CONSTRAINT_Df,
  CONSTRAINT_Dg,
  CONSTRAINT_Di,
  CONSTRAINT_Dn,
  CONSTRAINT_Dl,
  CONSTRAINT_DL,
  CONSTRAINT_Do,
  CONSTRAINT_Dv,
  CONSTRAINT_Dy,
  CONSTRAINT_Dt,
  CONSTRAINT_Dp,
  CONSTRAINT_US,
  CONSTRAINT_V,
  CONSTRAINT__l,
  CONSTRAINT__g,
  CONSTRAINT_i,
  CONSTRAINT_s,
  CONSTRAINT_n,
  CONSTRAINT_E,
  CONSTRAINT_F,
  CONSTRAINT_X,
  CONSTRAINT__LIMIT
};

extern enum constraint_num lookup_constraint_1 (const char *);
extern const unsigned char lookup_constraint_array[];

/* Return the constraint at the beginning of P, or CONSTRAINT__UNKNOWN if it
   isn't recognized.  */

static inline enum constraint_num
lookup_constraint (const char *p)
{
  unsigned int index = lookup_constraint_array[(unsigned char) *p];
  return (index == UCHAR_MAX
          ? lookup_constraint_1 (p)
          : (enum constraint_num) index);
}

extern bool (*constraint_satisfied_p_array[]) (rtx);

/* Return true if X satisfies constraint C.  */

static inline bool
constraint_satisfied_p (rtx x, enum constraint_num c)
{
  int i = (int) c - (int) CONSTRAINT_Pj;
  return i >= 0 && constraint_satisfied_p_array[i] (x);
}

static inline bool
insn_extra_register_constraint (enum constraint_num c)
{
  return c >= CONSTRAINT_r && c <= CONSTRAINT_Ts;
}

static inline bool
insn_extra_memory_constraint (enum constraint_num c)
{
  return c >= CONSTRAINT_m && c <= CONSTRAINT_Uw;
}

static inline bool
insn_extra_address_constraint (enum constraint_num c)
{
  return c >= CONSTRAINT_p && c <= CONSTRAINT_p;
}

static inline void
insn_extra_constraint_allows_reg_mem (enum constraint_num c,
				      bool *allows_reg, bool *allows_mem)
{
  if (c >= CONSTRAINT_j && c <= CONSTRAINT_US)
    return;
  if (c >= CONSTRAINT_V && c <= CONSTRAINT__g)
    {
      *allows_mem = true;
      return;
    }
  (void) c;
  *allows_reg = true;
  *allows_mem = true;
}

static inline size_t
insn_constraint_len (char fc, const char *str ATTRIBUTE_UNUSED)
{
  switch (fc)
    {
    case 'C': return 2;
    case 'D': return 2;
    case 'P': return 2;
    case 'T': return 2;
    case 'U': return 2;
    default: break;
    }
  return 1;
}

#define CONSTRAINT_LEN(c_,s_) insn_constraint_len (c_,s_)

extern enum reg_class reg_class_for_constraint_1 (enum constraint_num);

static inline enum reg_class
reg_class_for_constraint (enum constraint_num c)
{
  if (insn_extra_register_constraint (c))
    return reg_class_for_constraint_1 (c);
  return NO_REGS;
}

extern bool insn_const_int_ok_for_constraint (HOST_WIDE_INT, enum constraint_num);
#define CONST_OK_FOR_CONSTRAINT_P(v_,c_,s_) \
    insn_const_int_ok_for_constraint (v_, lookup_constraint (s_))

enum constraint_type
{
  CT_REGISTER,
  CT_CONST_INT,
  CT_MEMORY,
  CT_ADDRESS,
  CT_FIXED_FORM
};

static inline enum constraint_type
get_constraint_type (enum constraint_num c)
{
  if (c >= CONSTRAINT_p)
    {
      if (c >= CONSTRAINT_j)
        return CT_FIXED_FORM;
      return CT_ADDRESS;
    }
  if (c >= CONSTRAINT_m)
    return CT_MEMORY;
  if (c >= CONSTRAINT_Pj)
    return CT_CONST_INT;
  return CT_REGISTER;
}
#endif /* tm-preds.h */
