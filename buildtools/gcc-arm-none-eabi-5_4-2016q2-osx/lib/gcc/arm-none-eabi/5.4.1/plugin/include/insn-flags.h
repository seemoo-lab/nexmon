/* Generated automatically by the program `genflags'
   from the machine description file `md'.  */

#ifndef GCC_INSN_FLAGS_H
#define GCC_INSN_FLAGS_H

#define HAVE_addsi3_compare0 (TARGET_ARM)
#define HAVE_cmpsi2_addneg (TARGET_32BIT && INTVAL (operands[2]) == -INTVAL (operands[3]))
#define HAVE_subsi3_compare (TARGET_32BIT)
#define HAVE_mulhisi3 (TARGET_DSP_MULTIPLY)
#define HAVE_maddhisi4 (TARGET_DSP_MULTIPLY)
#define HAVE_maddhidi4 (TARGET_DSP_MULTIPLY)
#define HAVE_insv_zero (arm_arch_thumb2)
#define HAVE_insv_t2 (arm_arch_thumb2)
#define HAVE_andsi_notsi_si (TARGET_32BIT)
#define HAVE_andsi_not_shiftsi_si (TARGET_ARM)
#define HAVE_arm_ashldi3_1bit (TARGET_32BIT)
#define HAVE_arm_ashrdi3_1bit (TARGET_32BIT)
#define HAVE_arm_lshrdi3_1bit (TARGET_32BIT)
#define HAVE_unaligned_loadsi (unaligned_access && TARGET_32BIT)
#define HAVE_unaligned_loadhis (unaligned_access && TARGET_32BIT)
#define HAVE_unaligned_loadhiu (unaligned_access && TARGET_32BIT)
#define HAVE_unaligned_storesi (unaligned_access && TARGET_32BIT)
#define HAVE_unaligned_storehi (unaligned_access && TARGET_32BIT)
#define HAVE_unaligned_loaddi (unaligned_access && TARGET_32BIT)
#define HAVE_unaligned_storedi (unaligned_access && TARGET_32BIT)
#define HAVE_extzv_t2 (arm_arch_thumb2)
#define HAVE_divsi3 (TARGET_IDIV)
#define HAVE_udivsi3 (TARGET_IDIV)
#define HAVE_one_cmpldi2 (TARGET_32BIT)
#define HAVE_zero_extendqidi2 (TARGET_32BIT )
#define HAVE_zero_extendhidi2 (TARGET_32BIT && arm_arch6)
#define HAVE_zero_extendsidi2 (TARGET_32BIT )
#define HAVE_extendqidi2 (TARGET_32BIT && arm_arch6)
#define HAVE_extendhidi2 (TARGET_32BIT && arm_arch6)
#define HAVE_extendsidi2 (TARGET_32BIT )
#define HAVE_pic_load_addr_unified (flag_pic)
#define HAVE_pic_load_addr_32bit (TARGET_32BIT && flag_pic)
#define HAVE_pic_load_addr_thumb1 (TARGET_THUMB1 && flag_pic)
#define HAVE_pic_add_dot_plus_four (TARGET_THUMB)
#define HAVE_pic_add_dot_plus_eight (TARGET_ARM)
#define HAVE_tls_load_dot_plus_eight (TARGET_ARM)
#define HAVE_arm_cond_branch (TARGET_32BIT)
#define HAVE_blockage 1
#define HAVE_arm_casesi_internal (TARGET_ARM)
#define HAVE_nop 1
#define HAVE_trap 1
#define HAVE_movcond_addsi (TARGET_32BIT)
#define HAVE_movcond (TARGET_ARM)
#define HAVE_stack_tie 1
#define HAVE_align_4 1
#define HAVE_align_8 1
#define HAVE_consttable_end 1
#define HAVE_consttable_1 1
#define HAVE_consttable_2 1
#define HAVE_consttable_4 1
#define HAVE_consttable_8 1
#define HAVE_consttable_16 1
#define HAVE_clzsi2 (TARGET_32BIT && arm_arch5)
#define HAVE_rbitsi2 (TARGET_32BIT && arm_arch_thumb2)
#define HAVE_prefetch (TARGET_32BIT && arm_arch5e)
#define HAVE_force_register_use 1
#define HAVE_arm_eh_return (TARGET_ARM)
#define HAVE_load_tp_hard (TARGET_HARD_TP)
#define HAVE_load_tp_soft (TARGET_SOFT_TP)
#define HAVE_tlscall (TARGET_GNU2_TLS)
#define HAVE_arm_rev16si2 (arm_arch6 \
   && aarch_rev16_shleft_mask_imm_p (operands[3], SImode) \
   && aarch_rev16_shright_mask_imm_p (operands[2], SImode))
#define HAVE_arm_rev16si2_alt (arm_arch6 \
   && aarch_rev16_shleft_mask_imm_p (operands[3], SImode) \
   && aarch_rev16_shright_mask_imm_p (operands[2], SImode))
#define HAVE_crc32b (TARGET_CRC32)
#define HAVE_crc32h (TARGET_CRC32)
#define HAVE_crc32w (TARGET_CRC32)
#define HAVE_crc32cb (TARGET_CRC32)
#define HAVE_crc32ch (TARGET_CRC32)
#define HAVE_crc32cw (TARGET_CRC32)
#define HAVE_tbcstv8qi (TARGET_REALLY_IWMMXT)
#define HAVE_tbcstv4hi (TARGET_REALLY_IWMMXT)
#define HAVE_tbcstv2si (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_iordi3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_xordi3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_anddi3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_nanddi3 (TARGET_REALLY_IWMMXT)
#define HAVE_movv2si_internal (TARGET_REALLY_IWMMXT)
#define HAVE_movv4hi_internal (TARGET_REALLY_IWMMXT)
#define HAVE_movv8qi_internal (TARGET_REALLY_IWMMXT)
#define HAVE_ssaddv8qi3 (TARGET_REALLY_IWMMXT)
#define HAVE_ssaddv4hi3 (TARGET_REALLY_IWMMXT)
#define HAVE_ssaddv2si3 (TARGET_REALLY_IWMMXT)
#define HAVE_usaddv8qi3 (TARGET_REALLY_IWMMXT)
#define HAVE_usaddv4hi3 (TARGET_REALLY_IWMMXT)
#define HAVE_usaddv2si3 (TARGET_REALLY_IWMMXT)
#define HAVE_sssubv8qi3 (TARGET_REALLY_IWMMXT)
#define HAVE_sssubv4hi3 (TARGET_REALLY_IWMMXT)
#define HAVE_sssubv2si3 (TARGET_REALLY_IWMMXT)
#define HAVE_ussubv8qi3 (TARGET_REALLY_IWMMXT)
#define HAVE_ussubv4hi3 (TARGET_REALLY_IWMMXT)
#define HAVE_ussubv2si3 (TARGET_REALLY_IWMMXT)
#define HAVE_smulv4hi3_highpart (TARGET_REALLY_IWMMXT)
#define HAVE_umulv4hi3_highpart (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmacs (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmacsz (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmacu (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmacuz (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_clrdi (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_clrv8qi (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_clrv4hi (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_clrv2si (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_uavgrndv8qi3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_uavgrndv4hi3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_uavgv8qi3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_uavgv4hi3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_tinsrb (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_tinsrh (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_tinsrw (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_textrmub (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_textrmsb (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_textrmuh (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_textrmsh (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_textrmw (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wshufh (TARGET_REALLY_IWMMXT)
#define HAVE_eqv8qi3 (TARGET_REALLY_IWMMXT)
#define HAVE_eqv4hi3 (TARGET_REALLY_IWMMXT)
#define HAVE_eqv2si3 (TARGET_REALLY_IWMMXT)
#define HAVE_gtuv8qi3 (TARGET_REALLY_IWMMXT)
#define HAVE_gtuv4hi3 (TARGET_REALLY_IWMMXT)
#define HAVE_gtuv2si3 (TARGET_REALLY_IWMMXT)
#define HAVE_gtv8qi3 (TARGET_REALLY_IWMMXT)
#define HAVE_gtv4hi3 (TARGET_REALLY_IWMMXT)
#define HAVE_gtv2si3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wpackhss (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wpackwss (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wpackdss (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wpackhus (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wpackwus (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wpackdus (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wunpckihb (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wunpckihh (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wunpckihw (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wunpckilb (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wunpckilh (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wunpckilw (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wunpckehub (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wunpckehuh (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wunpckehuw (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wunpckehsb (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wunpckehsh (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wunpckehsw (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wunpckelub (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wunpckeluh (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wunpckeluw (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wunpckelsb (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wunpckelsh (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wunpckelsw (TARGET_REALLY_IWMMXT)
#define HAVE_rorv4hi3 (TARGET_REALLY_IWMMXT)
#define HAVE_rorv2si3 (TARGET_REALLY_IWMMXT)
#define HAVE_rordi3 (TARGET_REALLY_IWMMXT)
#define HAVE_ashrv4hi3_iwmmxt (TARGET_REALLY_IWMMXT)
#define HAVE_ashrv2si3_iwmmxt (TARGET_REALLY_IWMMXT)
#define HAVE_ashrdi3_iwmmxt (TARGET_REALLY_IWMMXT)
#define HAVE_lshrv4hi3_iwmmxt (TARGET_REALLY_IWMMXT)
#define HAVE_lshrv2si3_iwmmxt (TARGET_REALLY_IWMMXT)
#define HAVE_lshrdi3_iwmmxt (TARGET_REALLY_IWMMXT)
#define HAVE_ashlv4hi3_iwmmxt (TARGET_REALLY_IWMMXT)
#define HAVE_ashlv2si3_iwmmxt (TARGET_REALLY_IWMMXT)
#define HAVE_ashldi3_iwmmxt (TARGET_REALLY_IWMMXT)
#define HAVE_rorv4hi3_di (TARGET_REALLY_IWMMXT)
#define HAVE_rorv2si3_di (TARGET_REALLY_IWMMXT)
#define HAVE_rordi3_di (TARGET_REALLY_IWMMXT)
#define HAVE_ashrv4hi3_di (TARGET_REALLY_IWMMXT)
#define HAVE_ashrv2si3_di (TARGET_REALLY_IWMMXT)
#define HAVE_ashrdi3_di (TARGET_REALLY_IWMMXT)
#define HAVE_lshrv4hi3_di (TARGET_REALLY_IWMMXT)
#define HAVE_lshrv2si3_di (TARGET_REALLY_IWMMXT)
#define HAVE_lshrdi3_di (TARGET_REALLY_IWMMXT)
#define HAVE_ashlv4hi3_di (TARGET_REALLY_IWMMXT)
#define HAVE_ashlv2si3_di (TARGET_REALLY_IWMMXT)
#define HAVE_ashldi3_di (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmadds (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmaddu (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_tmia (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_tmiaph (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_tmiabb (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_tmiatb (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_tmiabt (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_tmiatt (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_tmovmskb (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_tmovmskh (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_tmovmskw (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_waccb (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wacch (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_waccw (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_waligni (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_walignr (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_walignr0 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_walignr1 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_walignr2 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_walignr3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wsadb (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wsadh (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wsadbz (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wsadhz (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wabsv2si3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wabsv4hi3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wabsv8qi3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wabsdiffb (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wabsdiffh (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wabsdiffw (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_waddsubhx (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wsubaddhx (TARGET_REALLY_IWMMXT)
#define HAVE_addcv4hi3 (TARGET_REALLY_IWMMXT)
#define HAVE_addcv2si3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_avg4 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_avg4r (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmaddsx (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmaddux (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmaddsn (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmaddun (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmulwsm (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmulwum (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmulsmr (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmulumr (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmulwsmr (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmulwumr (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmulwl (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wqmulm (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wqmulwm (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wqmulmr (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wqmulwmr (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_waddbhusm (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_waddbhusl (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wqmiabb (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wqmiabt (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wqmiatb (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wqmiatt (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wqmiabbn (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wqmiabtn (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wqmiatbn (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wqmiattn (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmiabb (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmiabt (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmiatb (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmiatt (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmiabbn (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmiabtn (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmiatbn (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmiattn (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmiawbb (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmiawbt (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmiawtb (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmiawtt (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmiawbbn (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmiawbtn (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmiawtbn (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmiawttn (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_wmerge (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_tandcv2si3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_tandcv4hi3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_tandcv8qi3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_torcv2si3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_torcv4hi3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_torcv8qi3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_torvscv2si3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_torvscv4hi3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_torvscv8qi3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_textrcv2si3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_textrcv4hi3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_textrcv8qi3 (TARGET_REALLY_IWMMXT)
#define HAVE_fmasf4 ((TARGET_32BIT && TARGET_HARD_FLOAT && TARGET_FMA) && (TARGET_VFP))
#define HAVE_fmadf4 ((TARGET_32BIT && TARGET_HARD_FLOAT && TARGET_FMA) && (TARGET_VFP_DOUBLE))
#define HAVE_extendhfsf2 (TARGET_32BIT && TARGET_HARD_FLOAT && TARGET_FP16)
#define HAVE_truncsfhf2 (TARGET_32BIT && TARGET_HARD_FLOAT && TARGET_FP16)
#define HAVE_fixuns_truncsfsi2 (TARGET_32BIT && TARGET_HARD_FLOAT && TARGET_VFP)
#define HAVE_fixuns_truncdfsi2 (TARGET_32BIT && TARGET_HARD_FLOAT && TARGET_VFP_DOUBLE)
#define HAVE_floatunssisf2 (TARGET_32BIT && TARGET_HARD_FLOAT && TARGET_VFP)
#define HAVE_floatunssidf2 (TARGET_32BIT && TARGET_HARD_FLOAT && TARGET_VFP_DOUBLE)
#define HAVE_btruncsf2 ((TARGET_HARD_FLOAT && TARGET_VFP5 ) && (TARGET_VFP))
#define HAVE_ceilsf2 ((TARGET_HARD_FLOAT && TARGET_VFP5 ) && (TARGET_VFP))
#define HAVE_floorsf2 ((TARGET_HARD_FLOAT && TARGET_VFP5 ) && (TARGET_VFP))
#define HAVE_nearbyintsf2 ((TARGET_HARD_FLOAT && TARGET_VFP5 ) && (TARGET_VFP))
#define HAVE_rintsf2 ((TARGET_HARD_FLOAT && TARGET_VFP5 ) && (TARGET_VFP))
#define HAVE_roundsf2 ((TARGET_HARD_FLOAT && TARGET_VFP5 ) && (TARGET_VFP))
#define HAVE_btruncdf2 ((TARGET_HARD_FLOAT && TARGET_VFP5 && TARGET_VFP_DOUBLE) && (TARGET_VFP_DOUBLE))
#define HAVE_ceildf2 ((TARGET_HARD_FLOAT && TARGET_VFP5 && TARGET_VFP_DOUBLE) && (TARGET_VFP_DOUBLE))
#define HAVE_floordf2 ((TARGET_HARD_FLOAT && TARGET_VFP5 && TARGET_VFP_DOUBLE) && (TARGET_VFP_DOUBLE))
#define HAVE_nearbyintdf2 ((TARGET_HARD_FLOAT && TARGET_VFP5 && TARGET_VFP_DOUBLE) && (TARGET_VFP_DOUBLE))
#define HAVE_rintdf2 ((TARGET_HARD_FLOAT && TARGET_VFP5 && TARGET_VFP_DOUBLE) && (TARGET_VFP_DOUBLE))
#define HAVE_rounddf2 ((TARGET_HARD_FLOAT && TARGET_VFP5 && TARGET_VFP_DOUBLE) && (TARGET_VFP_DOUBLE))
#define HAVE_lceilsfsi2 ((TARGET_HARD_FLOAT && TARGET_FPU_ARMV8 ) && (TARGET_VFP))
#define HAVE_lfloorsfsi2 ((TARGET_HARD_FLOAT && TARGET_FPU_ARMV8 ) && (TARGET_VFP))
#define HAVE_lroundsfsi2 ((TARGET_HARD_FLOAT && TARGET_FPU_ARMV8 ) && (TARGET_VFP))
#define HAVE_lceilusfsi2 ((TARGET_HARD_FLOAT && TARGET_FPU_ARMV8 ) && (TARGET_VFP))
#define HAVE_lfloorusfsi2 ((TARGET_HARD_FLOAT && TARGET_FPU_ARMV8 ) && (TARGET_VFP))
#define HAVE_lroundusfsi2 ((TARGET_HARD_FLOAT && TARGET_FPU_ARMV8 ) && (TARGET_VFP))
#define HAVE_lceildfsi2 ((TARGET_HARD_FLOAT && TARGET_FPU_ARMV8 && TARGET_VFP_DOUBLE) && (TARGET_VFP_DOUBLE))
#define HAVE_lfloordfsi2 ((TARGET_HARD_FLOAT && TARGET_FPU_ARMV8 && TARGET_VFP_DOUBLE) && (TARGET_VFP_DOUBLE))
#define HAVE_lrounddfsi2 ((TARGET_HARD_FLOAT && TARGET_FPU_ARMV8 && TARGET_VFP_DOUBLE) && (TARGET_VFP_DOUBLE))
#define HAVE_lceiludfsi2 ((TARGET_HARD_FLOAT && TARGET_FPU_ARMV8 && TARGET_VFP_DOUBLE) && (TARGET_VFP_DOUBLE))
#define HAVE_lfloorudfsi2 ((TARGET_HARD_FLOAT && TARGET_FPU_ARMV8 && TARGET_VFP_DOUBLE) && (TARGET_VFP_DOUBLE))
#define HAVE_lroundudfsi2 ((TARGET_HARD_FLOAT && TARGET_FPU_ARMV8 && TARGET_VFP_DOUBLE) && (TARGET_VFP_DOUBLE))
#define HAVE_smaxsf3 ((TARGET_HARD_FLOAT && TARGET_VFP5 ) && (TARGET_VFP))
#define HAVE_smaxdf3 ((TARGET_HARD_FLOAT && TARGET_VFP5 && TARGET_VFP_DOUBLE) && (TARGET_VFP_DOUBLE))
#define HAVE_sminsf3 ((TARGET_HARD_FLOAT && TARGET_VFP5 ) && (TARGET_VFP))
#define HAVE_smindf3 ((TARGET_HARD_FLOAT && TARGET_VFP5 && TARGET_VFP_DOUBLE) && (TARGET_VFP_DOUBLE))
#define HAVE_set_fpscr (TARGET_VFP && TARGET_HARD_FLOAT)
#define HAVE_get_fpscr (TARGET_VFP && TARGET_HARD_FLOAT)
#define HAVE_thumb1_subsi3_insn (TARGET_THUMB1)
#define HAVE_thumb1_bicsi3 (TARGET_THUMB1)
#define HAVE_thumb1_extendhisi2 (TARGET_THUMB1)
#define HAVE_thumb1_extendqisi2 (TARGET_THUMB1)
#define HAVE_movmem12b (TARGET_THUMB1)
#define HAVE_movmem8b (TARGET_THUMB1)
#define HAVE_thumb1_cbz (TARGET_THUMB1 && TARGET_HAVE_MOVT)
#define HAVE_cbranchsi4_insn (TARGET_THUMB1)
#define HAVE_cbranchsi4_scratch (TARGET_THUMB1)
#define HAVE_cstoresi_nltu_thumb1 (TARGET_THUMB1)
#define HAVE_cstoresi_ltu_thumb1 (TARGET_THUMB1)
#define HAVE_thumb1_addsi3_addgeu (TARGET_THUMB1)
#define HAVE_thumb1_casesi_dispatch (TARGET_THUMB1)
#define HAVE_prologue_thumb1_interwork (TARGET_THUMB1)
#define HAVE_thumb_eh_return (TARGET_THUMB1)
#define HAVE_tls_load_dot_plus_four (TARGET_THUMB2)
#define HAVE_thumb2_zero_extendqisi2_v6 (TARGET_THUMB2 && arm_arch6)
#define HAVE_thumb2_casesi_internal (TARGET_THUMB2 && !flag_pic)
#define HAVE_thumb2_casesi_internal_pic (TARGET_THUMB2 && flag_pic)
#define HAVE_thumb2_eh_return (TARGET_THUMB2)
#define HAVE_thumb2_addsi3_compare0 (TARGET_THUMB2)
#define HAVE_vec_setv8qi_internal (TARGET_NEON)
#define HAVE_vec_setv4hi_internal (TARGET_NEON)
#define HAVE_vec_setv2si_internal (TARGET_NEON)
#define HAVE_vec_setv2sf_internal (TARGET_NEON)
#define HAVE_vec_setv16qi_internal (TARGET_NEON)
#define HAVE_vec_setv8hi_internal (TARGET_NEON)
#define HAVE_vec_setv4si_internal (TARGET_NEON)
#define HAVE_vec_setv4sf_internal (TARGET_NEON)
#define HAVE_vec_setv2di_internal (TARGET_NEON)
#define HAVE_vec_extractv8qi (TARGET_NEON)
#define HAVE_vec_extractv4hi (TARGET_NEON)
#define HAVE_vec_extractv2si (TARGET_NEON)
#define HAVE_vec_extractv2sf (TARGET_NEON)
#define HAVE_vec_extractv16qi (TARGET_NEON)
#define HAVE_vec_extractv8hi (TARGET_NEON)
#define HAVE_vec_extractv4si (TARGET_NEON)
#define HAVE_vec_extractv4sf (TARGET_NEON)
#define HAVE_vec_extractv2di (TARGET_NEON)
#define HAVE_adddi3_neon (TARGET_NEON)
#define HAVE_subdi3_neon (TARGET_NEON)
#define HAVE_mulv8qi3addv8qi_neon (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_mulv16qi3addv16qi_neon (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_mulv4hi3addv4hi_neon (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_mulv8hi3addv8hi_neon (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_mulv2si3addv2si_neon (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_mulv4si3addv4si_neon (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_mulv2sf3addv2sf_neon (TARGET_NEON && (!true || flag_unsafe_math_optimizations))
#define HAVE_mulv4sf3addv4sf_neon (TARGET_NEON && (!true || flag_unsafe_math_optimizations))
#define HAVE_mulv8qi3negv8qiaddv8qi_neon (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_mulv16qi3negv16qiaddv16qi_neon (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_mulv4hi3negv4hiaddv4hi_neon (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_mulv8hi3negv8hiaddv8hi_neon (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_mulv2si3negv2siaddv2si_neon (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_mulv4si3negv4siaddv4si_neon (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_mulv2sf3negv2sfaddv2sf_neon (TARGET_NEON && (!true || flag_unsafe_math_optimizations))
#define HAVE_mulv4sf3negv4sfaddv4sf_neon (TARGET_NEON && (!true || flag_unsafe_math_optimizations))
#define HAVE_fmav2sf4 (TARGET_NEON && TARGET_FMA && flag_unsafe_math_optimizations)
#define HAVE_fmav4sf4 (TARGET_NEON && TARGET_FMA && flag_unsafe_math_optimizations)
#define HAVE_fmav2sf4_intrinsic (TARGET_NEON && TARGET_FMA)
#define HAVE_fmav4sf4_intrinsic (TARGET_NEON && TARGET_FMA)
#define HAVE_fmsubv2sf4_intrinsic (TARGET_NEON && TARGET_FMA)
#define HAVE_fmsubv4sf4_intrinsic (TARGET_NEON && TARGET_FMA)
#define HAVE_neon_vrintpv2sf (TARGET_NEON && TARGET_FPU_ARMV8)
#define HAVE_neon_vrintzv2sf (TARGET_NEON && TARGET_FPU_ARMV8)
#define HAVE_neon_vrintmv2sf (TARGET_NEON && TARGET_FPU_ARMV8)
#define HAVE_neon_vrintxv2sf (TARGET_NEON && TARGET_FPU_ARMV8)
#define HAVE_neon_vrintav2sf (TARGET_NEON && TARGET_FPU_ARMV8)
#define HAVE_neon_vrintnv2sf (TARGET_NEON && TARGET_FPU_ARMV8)
#define HAVE_neon_vrintpv4sf (TARGET_NEON && TARGET_FPU_ARMV8)
#define HAVE_neon_vrintzv4sf (TARGET_NEON && TARGET_FPU_ARMV8)
#define HAVE_neon_vrintmv4sf (TARGET_NEON && TARGET_FPU_ARMV8)
#define HAVE_neon_vrintxv4sf (TARGET_NEON && TARGET_FPU_ARMV8)
#define HAVE_neon_vrintav4sf (TARGET_NEON && TARGET_FPU_ARMV8)
#define HAVE_neon_vrintnv4sf (TARGET_NEON && TARGET_FPU_ARMV8)
#define HAVE_neon_vcvtpv2sfv2si (TARGET_NEON && TARGET_FPU_ARMV8)
#define HAVE_neon_vcvtmv2sfv2si (TARGET_NEON && TARGET_FPU_ARMV8)
#define HAVE_neon_vcvtav2sfv2si (TARGET_NEON && TARGET_FPU_ARMV8)
#define HAVE_neon_vcvtpuv2sfv2si (TARGET_NEON && TARGET_FPU_ARMV8)
#define HAVE_neon_vcvtmuv2sfv2si (TARGET_NEON && TARGET_FPU_ARMV8)
#define HAVE_neon_vcvtauv2sfv2si (TARGET_NEON && TARGET_FPU_ARMV8)
#define HAVE_neon_vcvtpv4sfv4si (TARGET_NEON && TARGET_FPU_ARMV8)
#define HAVE_neon_vcvtmv4sfv4si (TARGET_NEON && TARGET_FPU_ARMV8)
#define HAVE_neon_vcvtav4sfv4si (TARGET_NEON && TARGET_FPU_ARMV8)
#define HAVE_neon_vcvtpuv4sfv4si (TARGET_NEON && TARGET_FPU_ARMV8)
#define HAVE_neon_vcvtmuv4sfv4si (TARGET_NEON && TARGET_FPU_ARMV8)
#define HAVE_neon_vcvtauv4sfv4si (TARGET_NEON && TARGET_FPU_ARMV8)
#define HAVE_iorv8qi3 (TARGET_NEON)
#define HAVE_iorv16qi3 (TARGET_NEON)
#define HAVE_iorv4hi3 (TARGET_NEON)
#define HAVE_iorv8hi3 (TARGET_NEON)
#define HAVE_iorv2si3 (TARGET_NEON)
#define HAVE_iorv4si3 (TARGET_NEON)
#define HAVE_iorv2sf3 (TARGET_NEON)
#define HAVE_iorv4sf3 (TARGET_NEON)
#define HAVE_iorv2di3 (TARGET_NEON)
#define HAVE_andv8qi3 (TARGET_NEON)
#define HAVE_andv16qi3 (TARGET_NEON)
#define HAVE_andv4hi3 (TARGET_NEON)
#define HAVE_andv8hi3 (TARGET_NEON)
#define HAVE_andv2si3 (TARGET_NEON)
#define HAVE_andv4si3 (TARGET_NEON)
#define HAVE_andv2sf3 (TARGET_NEON)
#define HAVE_andv4sf3 (TARGET_NEON)
#define HAVE_andv2di3 (TARGET_NEON)
#define HAVE_ornv8qi3_neon (TARGET_NEON)
#define HAVE_ornv16qi3_neon (TARGET_NEON)
#define HAVE_ornv4hi3_neon (TARGET_NEON)
#define HAVE_ornv8hi3_neon (TARGET_NEON)
#define HAVE_ornv2si3_neon (TARGET_NEON)
#define HAVE_ornv4si3_neon (TARGET_NEON)
#define HAVE_ornv2sf3_neon (TARGET_NEON)
#define HAVE_ornv4sf3_neon (TARGET_NEON)
#define HAVE_ornv2di3_neon (TARGET_NEON)
#define HAVE_orndi3_neon (TARGET_NEON)
#define HAVE_bicv8qi3_neon (TARGET_NEON)
#define HAVE_bicv16qi3_neon (TARGET_NEON)
#define HAVE_bicv4hi3_neon (TARGET_NEON)
#define HAVE_bicv8hi3_neon (TARGET_NEON)
#define HAVE_bicv2si3_neon (TARGET_NEON)
#define HAVE_bicv4si3_neon (TARGET_NEON)
#define HAVE_bicv2sf3_neon (TARGET_NEON)
#define HAVE_bicv4sf3_neon (TARGET_NEON)
#define HAVE_bicv2di3_neon (TARGET_NEON)
#define HAVE_bicdi3_neon (TARGET_NEON)
#define HAVE_xorv8qi3 (TARGET_NEON)
#define HAVE_xorv16qi3 (TARGET_NEON)
#define HAVE_xorv4hi3 (TARGET_NEON)
#define HAVE_xorv8hi3 (TARGET_NEON)
#define HAVE_xorv2si3 (TARGET_NEON)
#define HAVE_xorv4si3 (TARGET_NEON)
#define HAVE_xorv2sf3 (TARGET_NEON)
#define HAVE_xorv4sf3 (TARGET_NEON)
#define HAVE_xorv2di3 (TARGET_NEON)
#define HAVE_one_cmplv8qi2 (TARGET_NEON)
#define HAVE_one_cmplv16qi2 (TARGET_NEON)
#define HAVE_one_cmplv4hi2 (TARGET_NEON)
#define HAVE_one_cmplv8hi2 (TARGET_NEON)
#define HAVE_one_cmplv2si2 (TARGET_NEON)
#define HAVE_one_cmplv4si2 (TARGET_NEON)
#define HAVE_one_cmplv2sf2 (TARGET_NEON)
#define HAVE_one_cmplv4sf2 (TARGET_NEON)
#define HAVE_one_cmplv2di2 (TARGET_NEON)
#define HAVE_absv8qi2 (TARGET_NEON)
#define HAVE_absv16qi2 (TARGET_NEON)
#define HAVE_absv4hi2 (TARGET_NEON)
#define HAVE_absv8hi2 (TARGET_NEON)
#define HAVE_absv2si2 (TARGET_NEON)
#define HAVE_absv4si2 (TARGET_NEON)
#define HAVE_absv2sf2 (TARGET_NEON)
#define HAVE_absv4sf2 (TARGET_NEON)
#define HAVE_negv8qi2 (TARGET_NEON)
#define HAVE_negv16qi2 (TARGET_NEON)
#define HAVE_negv4hi2 (TARGET_NEON)
#define HAVE_negv8hi2 (TARGET_NEON)
#define HAVE_negv2si2 (TARGET_NEON)
#define HAVE_negv4si2 (TARGET_NEON)
#define HAVE_negv2sf2 (TARGET_NEON)
#define HAVE_negv4sf2 (TARGET_NEON)
#define HAVE_negdi2_neon (TARGET_NEON)
#define HAVE_vashlv8qi3 (TARGET_NEON)
#define HAVE_vashlv16qi3 (TARGET_NEON)
#define HAVE_vashlv4hi3 (TARGET_NEON)
#define HAVE_vashlv8hi3 (TARGET_NEON)
#define HAVE_vashlv2si3 (TARGET_NEON)
#define HAVE_vashlv4si3 (TARGET_NEON)
#define HAVE_vashrv8qi3_imm (TARGET_NEON)
#define HAVE_vashrv16qi3_imm (TARGET_NEON)
#define HAVE_vashrv4hi3_imm (TARGET_NEON)
#define HAVE_vashrv8hi3_imm (TARGET_NEON)
#define HAVE_vashrv2si3_imm (TARGET_NEON)
#define HAVE_vashrv4si3_imm (TARGET_NEON)
#define HAVE_vlshrv8qi3_imm (TARGET_NEON)
#define HAVE_vlshrv16qi3_imm (TARGET_NEON)
#define HAVE_vlshrv4hi3_imm (TARGET_NEON)
#define HAVE_vlshrv8hi3_imm (TARGET_NEON)
#define HAVE_vlshrv2si3_imm (TARGET_NEON)
#define HAVE_vlshrv4si3_imm (TARGET_NEON)
#define HAVE_ashlv8qi3_signed (TARGET_NEON)
#define HAVE_ashlv16qi3_signed (TARGET_NEON)
#define HAVE_ashlv4hi3_signed (TARGET_NEON)
#define HAVE_ashlv8hi3_signed (TARGET_NEON)
#define HAVE_ashlv2si3_signed (TARGET_NEON)
#define HAVE_ashlv4si3_signed (TARGET_NEON)
#define HAVE_ashlv2di3_signed (TARGET_NEON)
#define HAVE_ashlv8qi3_unsigned (TARGET_NEON)
#define HAVE_ashlv16qi3_unsigned (TARGET_NEON)
#define HAVE_ashlv4hi3_unsigned (TARGET_NEON)
#define HAVE_ashlv8hi3_unsigned (TARGET_NEON)
#define HAVE_ashlv2si3_unsigned (TARGET_NEON)
#define HAVE_ashlv4si3_unsigned (TARGET_NEON)
#define HAVE_ashlv2di3_unsigned (TARGET_NEON)
#define HAVE_neon_load_count (TARGET_NEON)
#define HAVE_ashldi3_neon_noclobber (TARGET_NEON && reload_completed \
   && (!CONST_INT_P (operands[2]) \
       || (INTVAL (operands[2]) >= 0 && INTVAL (operands[2]) < 64)))
#define HAVE_ashldi3_neon (TARGET_NEON)
#define HAVE_signed_shift_di3_neon (TARGET_NEON && reload_completed)
#define HAVE_unsigned_shift_di3_neon (TARGET_NEON && reload_completed)
#define HAVE_ashrdi3_neon_imm_noclobber (TARGET_NEON && reload_completed \
   && INTVAL (operands[2]) > 0 && INTVAL (operands[2]) <= 64)
#define HAVE_lshrdi3_neon_imm_noclobber (TARGET_NEON && reload_completed \
   && INTVAL (operands[2]) > 0 && INTVAL (operands[2]) <= 64)
#define HAVE_ashrdi3_neon (TARGET_NEON)
#define HAVE_lshrdi3_neon (TARGET_NEON)
#define HAVE_widen_ssumv8qi3 (TARGET_NEON)
#define HAVE_widen_ssumv4hi3 (TARGET_NEON)
#define HAVE_widen_ssumv2si3 (TARGET_NEON)
#define HAVE_widen_usumv8qi3 (TARGET_NEON)
#define HAVE_widen_usumv4hi3 (TARGET_NEON)
#define HAVE_widen_usumv2si3 (TARGET_NEON)
#define HAVE_quad_halves_plusv4si (TARGET_NEON)
#define HAVE_quad_halves_sminv4si (TARGET_NEON)
#define HAVE_quad_halves_smaxv4si (TARGET_NEON)
#define HAVE_quad_halves_uminv4si (TARGET_NEON)
#define HAVE_quad_halves_umaxv4si (TARGET_NEON)
#define HAVE_quad_halves_plusv4sf (TARGET_NEON && flag_unsafe_math_optimizations)
#define HAVE_quad_halves_sminv4sf (TARGET_NEON && flag_unsafe_math_optimizations)
#define HAVE_quad_halves_smaxv4sf (TARGET_NEON && flag_unsafe_math_optimizations)
#define HAVE_quad_halves_plusv8hi (TARGET_NEON)
#define HAVE_quad_halves_sminv8hi (TARGET_NEON)
#define HAVE_quad_halves_smaxv8hi (TARGET_NEON)
#define HAVE_quad_halves_uminv8hi (TARGET_NEON)
#define HAVE_quad_halves_umaxv8hi (TARGET_NEON)
#define HAVE_quad_halves_plusv16qi (TARGET_NEON)
#define HAVE_quad_halves_sminv16qi (TARGET_NEON)
#define HAVE_quad_halves_smaxv16qi (TARGET_NEON)
#define HAVE_quad_halves_uminv16qi (TARGET_NEON)
#define HAVE_quad_halves_umaxv16qi (TARGET_NEON)
#define HAVE_arm_reduc_plus_internal_v2di (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vpadd_internalv8qi (TARGET_NEON)
#define HAVE_neon_vpadd_internalv4hi (TARGET_NEON)
#define HAVE_neon_vpadd_internalv2si (TARGET_NEON)
#define HAVE_neon_vpadd_internalv2sf (TARGET_NEON)
#define HAVE_neon_vpsminv8qi (TARGET_NEON)
#define HAVE_neon_vpsminv4hi (TARGET_NEON)
#define HAVE_neon_vpsminv2si (TARGET_NEON)
#define HAVE_neon_vpsminv2sf (TARGET_NEON)
#define HAVE_neon_vpsmaxv8qi (TARGET_NEON)
#define HAVE_neon_vpsmaxv4hi (TARGET_NEON)
#define HAVE_neon_vpsmaxv2si (TARGET_NEON)
#define HAVE_neon_vpsmaxv2sf (TARGET_NEON)
#define HAVE_neon_vpuminv8qi (TARGET_NEON)
#define HAVE_neon_vpuminv4hi (TARGET_NEON)
#define HAVE_neon_vpuminv2si (TARGET_NEON)
#define HAVE_neon_vpumaxv8qi (TARGET_NEON)
#define HAVE_neon_vpumaxv4hi (TARGET_NEON)
#define HAVE_neon_vpumaxv2si (TARGET_NEON)
#define HAVE_neon_vaddv2sf_unspec (TARGET_NEON)
#define HAVE_neon_vaddv4sf_unspec (TARGET_NEON)
#define HAVE_neon_vaddlsv8qi (TARGET_NEON)
#define HAVE_neon_vaddluv8qi (TARGET_NEON)
#define HAVE_neon_vaddlsv4hi (TARGET_NEON)
#define HAVE_neon_vaddluv4hi (TARGET_NEON)
#define HAVE_neon_vaddlsv2si (TARGET_NEON)
#define HAVE_neon_vaddluv2si (TARGET_NEON)
#define HAVE_neon_vaddwsv8qi (TARGET_NEON)
#define HAVE_neon_vaddwuv8qi (TARGET_NEON)
#define HAVE_neon_vaddwsv4hi (TARGET_NEON)
#define HAVE_neon_vaddwuv4hi (TARGET_NEON)
#define HAVE_neon_vaddwsv2si (TARGET_NEON)
#define HAVE_neon_vaddwuv2si (TARGET_NEON)
#define HAVE_neon_vrhaddsv8qi (TARGET_NEON)
#define HAVE_neon_vrhadduv8qi (TARGET_NEON)
#define HAVE_neon_vhaddsv8qi (TARGET_NEON)
#define HAVE_neon_vhadduv8qi (TARGET_NEON)
#define HAVE_neon_vrhaddsv16qi (TARGET_NEON)
#define HAVE_neon_vrhadduv16qi (TARGET_NEON)
#define HAVE_neon_vhaddsv16qi (TARGET_NEON)
#define HAVE_neon_vhadduv16qi (TARGET_NEON)
#define HAVE_neon_vrhaddsv4hi (TARGET_NEON)
#define HAVE_neon_vrhadduv4hi (TARGET_NEON)
#define HAVE_neon_vhaddsv4hi (TARGET_NEON)
#define HAVE_neon_vhadduv4hi (TARGET_NEON)
#define HAVE_neon_vrhaddsv8hi (TARGET_NEON)
#define HAVE_neon_vrhadduv8hi (TARGET_NEON)
#define HAVE_neon_vhaddsv8hi (TARGET_NEON)
#define HAVE_neon_vhadduv8hi (TARGET_NEON)
#define HAVE_neon_vrhaddsv2si (TARGET_NEON)
#define HAVE_neon_vrhadduv2si (TARGET_NEON)
#define HAVE_neon_vhaddsv2si (TARGET_NEON)
#define HAVE_neon_vhadduv2si (TARGET_NEON)
#define HAVE_neon_vrhaddsv4si (TARGET_NEON)
#define HAVE_neon_vrhadduv4si (TARGET_NEON)
#define HAVE_neon_vhaddsv4si (TARGET_NEON)
#define HAVE_neon_vhadduv4si (TARGET_NEON)
#define HAVE_neon_vqaddsv8qi (TARGET_NEON)
#define HAVE_neon_vqadduv8qi (TARGET_NEON)
#define HAVE_neon_vqaddsv16qi (TARGET_NEON)
#define HAVE_neon_vqadduv16qi (TARGET_NEON)
#define HAVE_neon_vqaddsv4hi (TARGET_NEON)
#define HAVE_neon_vqadduv4hi (TARGET_NEON)
#define HAVE_neon_vqaddsv8hi (TARGET_NEON)
#define HAVE_neon_vqadduv8hi (TARGET_NEON)
#define HAVE_neon_vqaddsv2si (TARGET_NEON)
#define HAVE_neon_vqadduv2si (TARGET_NEON)
#define HAVE_neon_vqaddsv4si (TARGET_NEON)
#define HAVE_neon_vqadduv4si (TARGET_NEON)
#define HAVE_neon_vqaddsdi (TARGET_NEON)
#define HAVE_neon_vqaddudi (TARGET_NEON)
#define HAVE_neon_vqaddsv2di (TARGET_NEON)
#define HAVE_neon_vqadduv2di (TARGET_NEON)
#define HAVE_neon_vaddhnv8hi (TARGET_NEON)
#define HAVE_neon_vraddhnv8hi (TARGET_NEON)
#define HAVE_neon_vaddhnv4si (TARGET_NEON)
#define HAVE_neon_vraddhnv4si (TARGET_NEON)
#define HAVE_neon_vaddhnv2di (TARGET_NEON)
#define HAVE_neon_vraddhnv2di (TARGET_NEON)
#define HAVE_neon_vmulpv8qi (TARGET_NEON)
#define HAVE_neon_vmulpv16qi (TARGET_NEON)
#define HAVE_neon_vmulfv2sf (TARGET_NEON)
#define HAVE_neon_vmulfv4sf (TARGET_NEON)
#define HAVE_neon_vmlav8qi_unspec (TARGET_NEON)
#define HAVE_neon_vmlav16qi_unspec (TARGET_NEON)
#define HAVE_neon_vmlav4hi_unspec (TARGET_NEON)
#define HAVE_neon_vmlav8hi_unspec (TARGET_NEON)
#define HAVE_neon_vmlav2si_unspec (TARGET_NEON)
#define HAVE_neon_vmlav4si_unspec (TARGET_NEON)
#define HAVE_neon_vmlav2sf_unspec (TARGET_NEON)
#define HAVE_neon_vmlav4sf_unspec (TARGET_NEON)
#define HAVE_neon_vmlalsv8qi (TARGET_NEON)
#define HAVE_neon_vmlaluv8qi (TARGET_NEON)
#define HAVE_neon_vmlalsv4hi (TARGET_NEON)
#define HAVE_neon_vmlaluv4hi (TARGET_NEON)
#define HAVE_neon_vmlalsv2si (TARGET_NEON)
#define HAVE_neon_vmlaluv2si (TARGET_NEON)
#define HAVE_neon_vmlsv8qi_unspec (TARGET_NEON)
#define HAVE_neon_vmlsv16qi_unspec (TARGET_NEON)
#define HAVE_neon_vmlsv4hi_unspec (TARGET_NEON)
#define HAVE_neon_vmlsv8hi_unspec (TARGET_NEON)
#define HAVE_neon_vmlsv2si_unspec (TARGET_NEON)
#define HAVE_neon_vmlsv4si_unspec (TARGET_NEON)
#define HAVE_neon_vmlsv2sf_unspec (TARGET_NEON)
#define HAVE_neon_vmlsv4sf_unspec (TARGET_NEON)
#define HAVE_neon_vmlslsv8qi (TARGET_NEON)
#define HAVE_neon_vmlsluv8qi (TARGET_NEON)
#define HAVE_neon_vmlslsv4hi (TARGET_NEON)
#define HAVE_neon_vmlsluv4hi (TARGET_NEON)
#define HAVE_neon_vmlslsv2si (TARGET_NEON)
#define HAVE_neon_vmlsluv2si (TARGET_NEON)
#define HAVE_neon_vqdmulhv4hi (TARGET_NEON)
#define HAVE_neon_vqrdmulhv4hi (TARGET_NEON)
#define HAVE_neon_vqdmulhv2si (TARGET_NEON)
#define HAVE_neon_vqrdmulhv2si (TARGET_NEON)
#define HAVE_neon_vqdmulhv8hi (TARGET_NEON)
#define HAVE_neon_vqrdmulhv8hi (TARGET_NEON)
#define HAVE_neon_vqdmulhv4si (TARGET_NEON)
#define HAVE_neon_vqrdmulhv4si (TARGET_NEON)
#define HAVE_neon_vqdmlalv4hi (TARGET_NEON)
#define HAVE_neon_vqdmlalv2si (TARGET_NEON)
#define HAVE_neon_vqdmlslv4hi (TARGET_NEON)
#define HAVE_neon_vqdmlslv2si (TARGET_NEON)
#define HAVE_neon_vmullsv8qi (TARGET_NEON)
#define HAVE_neon_vmulluv8qi (TARGET_NEON)
#define HAVE_neon_vmullpv8qi (TARGET_NEON)
#define HAVE_neon_vmullsv4hi (TARGET_NEON)
#define HAVE_neon_vmulluv4hi (TARGET_NEON)
#define HAVE_neon_vmullpv4hi (TARGET_NEON)
#define HAVE_neon_vmullsv2si (TARGET_NEON)
#define HAVE_neon_vmulluv2si (TARGET_NEON)
#define HAVE_neon_vmullpv2si (TARGET_NEON)
#define HAVE_neon_vqdmullv4hi (TARGET_NEON)
#define HAVE_neon_vqdmullv2si (TARGET_NEON)
#define HAVE_neon_vsubv2sf_unspec (TARGET_NEON)
#define HAVE_neon_vsubv4sf_unspec (TARGET_NEON)
#define HAVE_neon_vsublsv8qi (TARGET_NEON)
#define HAVE_neon_vsubluv8qi (TARGET_NEON)
#define HAVE_neon_vsublsv4hi (TARGET_NEON)
#define HAVE_neon_vsubluv4hi (TARGET_NEON)
#define HAVE_neon_vsublsv2si (TARGET_NEON)
#define HAVE_neon_vsubluv2si (TARGET_NEON)
#define HAVE_neon_vsubwsv8qi (TARGET_NEON)
#define HAVE_neon_vsubwuv8qi (TARGET_NEON)
#define HAVE_neon_vsubwsv4hi (TARGET_NEON)
#define HAVE_neon_vsubwuv4hi (TARGET_NEON)
#define HAVE_neon_vsubwsv2si (TARGET_NEON)
#define HAVE_neon_vsubwuv2si (TARGET_NEON)
#define HAVE_neon_vqsubsv8qi (TARGET_NEON)
#define HAVE_neon_vqsubuv8qi (TARGET_NEON)
#define HAVE_neon_vqsubsv16qi (TARGET_NEON)
#define HAVE_neon_vqsubuv16qi (TARGET_NEON)
#define HAVE_neon_vqsubsv4hi (TARGET_NEON)
#define HAVE_neon_vqsubuv4hi (TARGET_NEON)
#define HAVE_neon_vqsubsv8hi (TARGET_NEON)
#define HAVE_neon_vqsubuv8hi (TARGET_NEON)
#define HAVE_neon_vqsubsv2si (TARGET_NEON)
#define HAVE_neon_vqsubuv2si (TARGET_NEON)
#define HAVE_neon_vqsubsv4si (TARGET_NEON)
#define HAVE_neon_vqsubuv4si (TARGET_NEON)
#define HAVE_neon_vqsubsdi (TARGET_NEON)
#define HAVE_neon_vqsubudi (TARGET_NEON)
#define HAVE_neon_vqsubsv2di (TARGET_NEON)
#define HAVE_neon_vqsubuv2di (TARGET_NEON)
#define HAVE_neon_vhsubsv8qi (TARGET_NEON)
#define HAVE_neon_vhsubuv8qi (TARGET_NEON)
#define HAVE_neon_vhsubsv16qi (TARGET_NEON)
#define HAVE_neon_vhsubuv16qi (TARGET_NEON)
#define HAVE_neon_vhsubsv4hi (TARGET_NEON)
#define HAVE_neon_vhsubuv4hi (TARGET_NEON)
#define HAVE_neon_vhsubsv8hi (TARGET_NEON)
#define HAVE_neon_vhsubuv8hi (TARGET_NEON)
#define HAVE_neon_vhsubsv2si (TARGET_NEON)
#define HAVE_neon_vhsubuv2si (TARGET_NEON)
#define HAVE_neon_vhsubsv4si (TARGET_NEON)
#define HAVE_neon_vhsubuv4si (TARGET_NEON)
#define HAVE_neon_vsubhnv8hi (TARGET_NEON)
#define HAVE_neon_vrsubhnv8hi (TARGET_NEON)
#define HAVE_neon_vsubhnv4si (TARGET_NEON)
#define HAVE_neon_vrsubhnv4si (TARGET_NEON)
#define HAVE_neon_vsubhnv2di (TARGET_NEON)
#define HAVE_neon_vrsubhnv2di (TARGET_NEON)
#define HAVE_neon_vceqv8qi (TARGET_NEON)
#define HAVE_neon_vceqv16qi (TARGET_NEON)
#define HAVE_neon_vceqv4hi (TARGET_NEON)
#define HAVE_neon_vceqv8hi (TARGET_NEON)
#define HAVE_neon_vceqv2si (TARGET_NEON)
#define HAVE_neon_vceqv4si (TARGET_NEON)
#define HAVE_neon_vceqv2sf (TARGET_NEON)
#define HAVE_neon_vceqv4sf (TARGET_NEON)
#define HAVE_neon_vcgev8qi (TARGET_NEON)
#define HAVE_neon_vcgev16qi (TARGET_NEON)
#define HAVE_neon_vcgev4hi (TARGET_NEON)
#define HAVE_neon_vcgev8hi (TARGET_NEON)
#define HAVE_neon_vcgev2si (TARGET_NEON)
#define HAVE_neon_vcgev4si (TARGET_NEON)
#define HAVE_neon_vcgev2sf (TARGET_NEON)
#define HAVE_neon_vcgev4sf (TARGET_NEON)
#define HAVE_neon_vcgeuv8qi (TARGET_NEON)
#define HAVE_neon_vcgeuv16qi (TARGET_NEON)
#define HAVE_neon_vcgeuv4hi (TARGET_NEON)
#define HAVE_neon_vcgeuv8hi (TARGET_NEON)
#define HAVE_neon_vcgeuv2si (TARGET_NEON)
#define HAVE_neon_vcgeuv4si (TARGET_NEON)
#define HAVE_neon_vcgtv8qi (TARGET_NEON)
#define HAVE_neon_vcgtv16qi (TARGET_NEON)
#define HAVE_neon_vcgtv4hi (TARGET_NEON)
#define HAVE_neon_vcgtv8hi (TARGET_NEON)
#define HAVE_neon_vcgtv2si (TARGET_NEON)
#define HAVE_neon_vcgtv4si (TARGET_NEON)
#define HAVE_neon_vcgtv2sf (TARGET_NEON)
#define HAVE_neon_vcgtv4sf (TARGET_NEON)
#define HAVE_neon_vcgtuv8qi (TARGET_NEON)
#define HAVE_neon_vcgtuv16qi (TARGET_NEON)
#define HAVE_neon_vcgtuv4hi (TARGET_NEON)
#define HAVE_neon_vcgtuv8hi (TARGET_NEON)
#define HAVE_neon_vcgtuv2si (TARGET_NEON)
#define HAVE_neon_vcgtuv4si (TARGET_NEON)
#define HAVE_neon_vclev8qi (TARGET_NEON)
#define HAVE_neon_vclev16qi (TARGET_NEON)
#define HAVE_neon_vclev4hi (TARGET_NEON)
#define HAVE_neon_vclev8hi (TARGET_NEON)
#define HAVE_neon_vclev2si (TARGET_NEON)
#define HAVE_neon_vclev4si (TARGET_NEON)
#define HAVE_neon_vclev2sf (TARGET_NEON)
#define HAVE_neon_vclev4sf (TARGET_NEON)
#define HAVE_neon_vcltv8qi (TARGET_NEON)
#define HAVE_neon_vcltv16qi (TARGET_NEON)
#define HAVE_neon_vcltv4hi (TARGET_NEON)
#define HAVE_neon_vcltv8hi (TARGET_NEON)
#define HAVE_neon_vcltv2si (TARGET_NEON)
#define HAVE_neon_vcltv4si (TARGET_NEON)
#define HAVE_neon_vcltv2sf (TARGET_NEON)
#define HAVE_neon_vcltv4sf (TARGET_NEON)
#define HAVE_neon_vcagev2sf (TARGET_NEON)
#define HAVE_neon_vcagev4sf (TARGET_NEON)
#define HAVE_neon_vcagtv2sf (TARGET_NEON)
#define HAVE_neon_vcagtv4sf (TARGET_NEON)
#define HAVE_neon_vtstv8qi (TARGET_NEON)
#define HAVE_neon_vtstv16qi (TARGET_NEON)
#define HAVE_neon_vtstv4hi (TARGET_NEON)
#define HAVE_neon_vtstv8hi (TARGET_NEON)
#define HAVE_neon_vtstv2si (TARGET_NEON)
#define HAVE_neon_vtstv4si (TARGET_NEON)
#define HAVE_neon_vabdsv8qi (TARGET_NEON)
#define HAVE_neon_vabduv8qi (TARGET_NEON)
#define HAVE_neon_vabdsv16qi (TARGET_NEON)
#define HAVE_neon_vabduv16qi (TARGET_NEON)
#define HAVE_neon_vabdsv4hi (TARGET_NEON)
#define HAVE_neon_vabduv4hi (TARGET_NEON)
#define HAVE_neon_vabdsv8hi (TARGET_NEON)
#define HAVE_neon_vabduv8hi (TARGET_NEON)
#define HAVE_neon_vabdsv2si (TARGET_NEON)
#define HAVE_neon_vabduv2si (TARGET_NEON)
#define HAVE_neon_vabdsv4si (TARGET_NEON)
#define HAVE_neon_vabduv4si (TARGET_NEON)
#define HAVE_neon_vabdfv2sf (TARGET_NEON)
#define HAVE_neon_vabdfv4sf (TARGET_NEON)
#define HAVE_neon_vabdlsv8qi (TARGET_NEON)
#define HAVE_neon_vabdluv8qi (TARGET_NEON)
#define HAVE_neon_vabdlsv4hi (TARGET_NEON)
#define HAVE_neon_vabdluv4hi (TARGET_NEON)
#define HAVE_neon_vabdlsv2si (TARGET_NEON)
#define HAVE_neon_vabdluv2si (TARGET_NEON)
#define HAVE_neon_vabasv8qi (TARGET_NEON)
#define HAVE_neon_vabauv8qi (TARGET_NEON)
#define HAVE_neon_vabasv16qi (TARGET_NEON)
#define HAVE_neon_vabauv16qi (TARGET_NEON)
#define HAVE_neon_vabasv4hi (TARGET_NEON)
#define HAVE_neon_vabauv4hi (TARGET_NEON)
#define HAVE_neon_vabasv8hi (TARGET_NEON)
#define HAVE_neon_vabauv8hi (TARGET_NEON)
#define HAVE_neon_vabasv2si (TARGET_NEON)
#define HAVE_neon_vabauv2si (TARGET_NEON)
#define HAVE_neon_vabasv4si (TARGET_NEON)
#define HAVE_neon_vabauv4si (TARGET_NEON)
#define HAVE_neon_vabalsv8qi (TARGET_NEON)
#define HAVE_neon_vabaluv8qi (TARGET_NEON)
#define HAVE_neon_vabalsv4hi (TARGET_NEON)
#define HAVE_neon_vabaluv4hi (TARGET_NEON)
#define HAVE_neon_vabalsv2si (TARGET_NEON)
#define HAVE_neon_vabaluv2si (TARGET_NEON)
#define HAVE_neon_vmaxsv8qi (TARGET_NEON)
#define HAVE_neon_vmaxuv8qi (TARGET_NEON)
#define HAVE_neon_vminsv8qi (TARGET_NEON)
#define HAVE_neon_vminuv8qi (TARGET_NEON)
#define HAVE_neon_vmaxsv16qi (TARGET_NEON)
#define HAVE_neon_vmaxuv16qi (TARGET_NEON)
#define HAVE_neon_vminsv16qi (TARGET_NEON)
#define HAVE_neon_vminuv16qi (TARGET_NEON)
#define HAVE_neon_vmaxsv4hi (TARGET_NEON)
#define HAVE_neon_vmaxuv4hi (TARGET_NEON)
#define HAVE_neon_vminsv4hi (TARGET_NEON)
#define HAVE_neon_vminuv4hi (TARGET_NEON)
#define HAVE_neon_vmaxsv8hi (TARGET_NEON)
#define HAVE_neon_vmaxuv8hi (TARGET_NEON)
#define HAVE_neon_vminsv8hi (TARGET_NEON)
#define HAVE_neon_vminuv8hi (TARGET_NEON)
#define HAVE_neon_vmaxsv2si (TARGET_NEON)
#define HAVE_neon_vmaxuv2si (TARGET_NEON)
#define HAVE_neon_vminsv2si (TARGET_NEON)
#define HAVE_neon_vminuv2si (TARGET_NEON)
#define HAVE_neon_vmaxsv4si (TARGET_NEON)
#define HAVE_neon_vmaxuv4si (TARGET_NEON)
#define HAVE_neon_vminsv4si (TARGET_NEON)
#define HAVE_neon_vminuv4si (TARGET_NEON)
#define HAVE_neon_vmaxfv2sf (TARGET_NEON)
#define HAVE_neon_vminfv2sf (TARGET_NEON)
#define HAVE_neon_vmaxfv4sf (TARGET_NEON)
#define HAVE_neon_vminfv4sf (TARGET_NEON)
#define HAVE_neon_vpaddlsv8qi (TARGET_NEON)
#define HAVE_neon_vpaddluv8qi (TARGET_NEON)
#define HAVE_neon_vpaddlsv16qi (TARGET_NEON)
#define HAVE_neon_vpaddluv16qi (TARGET_NEON)
#define HAVE_neon_vpaddlsv4hi (TARGET_NEON)
#define HAVE_neon_vpaddluv4hi (TARGET_NEON)
#define HAVE_neon_vpaddlsv8hi (TARGET_NEON)
#define HAVE_neon_vpaddluv8hi (TARGET_NEON)
#define HAVE_neon_vpaddlsv2si (TARGET_NEON)
#define HAVE_neon_vpaddluv2si (TARGET_NEON)
#define HAVE_neon_vpaddlsv4si (TARGET_NEON)
#define HAVE_neon_vpaddluv4si (TARGET_NEON)
#define HAVE_neon_vpadalsv8qi (TARGET_NEON)
#define HAVE_neon_vpadaluv8qi (TARGET_NEON)
#define HAVE_neon_vpadalsv16qi (TARGET_NEON)
#define HAVE_neon_vpadaluv16qi (TARGET_NEON)
#define HAVE_neon_vpadalsv4hi (TARGET_NEON)
#define HAVE_neon_vpadaluv4hi (TARGET_NEON)
#define HAVE_neon_vpadalsv8hi (TARGET_NEON)
#define HAVE_neon_vpadaluv8hi (TARGET_NEON)
#define HAVE_neon_vpadalsv2si (TARGET_NEON)
#define HAVE_neon_vpadaluv2si (TARGET_NEON)
#define HAVE_neon_vpadalsv4si (TARGET_NEON)
#define HAVE_neon_vpadaluv4si (TARGET_NEON)
#define HAVE_neon_vpmaxsv8qi (TARGET_NEON)
#define HAVE_neon_vpmaxuv8qi (TARGET_NEON)
#define HAVE_neon_vpminsv8qi (TARGET_NEON)
#define HAVE_neon_vpminuv8qi (TARGET_NEON)
#define HAVE_neon_vpmaxsv4hi (TARGET_NEON)
#define HAVE_neon_vpmaxuv4hi (TARGET_NEON)
#define HAVE_neon_vpminsv4hi (TARGET_NEON)
#define HAVE_neon_vpminuv4hi (TARGET_NEON)
#define HAVE_neon_vpmaxsv2si (TARGET_NEON)
#define HAVE_neon_vpmaxuv2si (TARGET_NEON)
#define HAVE_neon_vpminsv2si (TARGET_NEON)
#define HAVE_neon_vpminuv2si (TARGET_NEON)
#define HAVE_neon_vpmaxfv2sf (TARGET_NEON)
#define HAVE_neon_vpminfv2sf (TARGET_NEON)
#define HAVE_neon_vpmaxfv4sf (TARGET_NEON)
#define HAVE_neon_vpminfv4sf (TARGET_NEON)
#define HAVE_neon_vrecpsv2sf (TARGET_NEON)
#define HAVE_neon_vrecpsv4sf (TARGET_NEON)
#define HAVE_neon_vrsqrtsv2sf (TARGET_NEON)
#define HAVE_neon_vrsqrtsv4sf (TARGET_NEON)
#define HAVE_neon_vqabsv8qi (TARGET_NEON)
#define HAVE_neon_vqabsv16qi (TARGET_NEON)
#define HAVE_neon_vqabsv4hi (TARGET_NEON)
#define HAVE_neon_vqabsv8hi (TARGET_NEON)
#define HAVE_neon_vqabsv2si (TARGET_NEON)
#define HAVE_neon_vqabsv4si (TARGET_NEON)
#define HAVE_neon_bswapv4hi (TARGET_NEON)
#define HAVE_neon_bswapv8hi (TARGET_NEON)
#define HAVE_neon_bswapv2si (TARGET_NEON)
#define HAVE_neon_bswapv4si (TARGET_NEON)
#define HAVE_neon_bswapv2di (TARGET_NEON)
#define HAVE_neon_vqnegv8qi (TARGET_NEON)
#define HAVE_neon_vqnegv16qi (TARGET_NEON)
#define HAVE_neon_vqnegv4hi (TARGET_NEON)
#define HAVE_neon_vqnegv8hi (TARGET_NEON)
#define HAVE_neon_vqnegv2si (TARGET_NEON)
#define HAVE_neon_vqnegv4si (TARGET_NEON)
#define HAVE_neon_vclsv8qi (TARGET_NEON)
#define HAVE_neon_vclsv16qi (TARGET_NEON)
#define HAVE_neon_vclsv4hi (TARGET_NEON)
#define HAVE_neon_vclsv8hi (TARGET_NEON)
#define HAVE_neon_vclsv2si (TARGET_NEON)
#define HAVE_neon_vclsv4si (TARGET_NEON)
#define HAVE_clzv8qi2 (TARGET_NEON)
#define HAVE_clzv16qi2 (TARGET_NEON)
#define HAVE_clzv4hi2 (TARGET_NEON)
#define HAVE_clzv8hi2 (TARGET_NEON)
#define HAVE_clzv2si2 (TARGET_NEON)
#define HAVE_clzv4si2 (TARGET_NEON)
#define HAVE_popcountv8qi2 (TARGET_NEON)
#define HAVE_popcountv16qi2 (TARGET_NEON)
#define HAVE_neon_vrecpev2si (TARGET_NEON)
#define HAVE_neon_vrecpev2sf (TARGET_NEON)
#define HAVE_neon_vrecpev4si (TARGET_NEON)
#define HAVE_neon_vrecpev4sf (TARGET_NEON)
#define HAVE_neon_vrsqrtev2si (TARGET_NEON)
#define HAVE_neon_vrsqrtev2sf (TARGET_NEON)
#define HAVE_neon_vrsqrtev4si (TARGET_NEON)
#define HAVE_neon_vrsqrtev4sf (TARGET_NEON)
#define HAVE_neon_vget_lanev8qi_sext_internal (TARGET_NEON)
#define HAVE_neon_vget_lanev4hi_sext_internal (TARGET_NEON)
#define HAVE_neon_vget_lanev2si_sext_internal (TARGET_NEON)
#define HAVE_neon_vget_lanev2sf_sext_internal (TARGET_NEON)
#define HAVE_neon_vget_lanev8qi_zext_internal (TARGET_NEON)
#define HAVE_neon_vget_lanev4hi_zext_internal (TARGET_NEON)
#define HAVE_neon_vget_lanev2si_zext_internal (TARGET_NEON)
#define HAVE_neon_vget_lanev2sf_zext_internal (TARGET_NEON)
#define HAVE_neon_vget_lanev16qi_sext_internal (TARGET_NEON)
#define HAVE_neon_vget_lanev8hi_sext_internal (TARGET_NEON)
#define HAVE_neon_vget_lanev4si_sext_internal (TARGET_NEON)
#define HAVE_neon_vget_lanev4sf_sext_internal (TARGET_NEON)
#define HAVE_neon_vget_lanev16qi_zext_internal (TARGET_NEON)
#define HAVE_neon_vget_lanev8hi_zext_internal (TARGET_NEON)
#define HAVE_neon_vget_lanev4si_zext_internal (TARGET_NEON)
#define HAVE_neon_vget_lanev4sf_zext_internal (TARGET_NEON)
#define HAVE_neon_vdup_nv8qi (TARGET_NEON)
#define HAVE_neon_vdup_nv4hi (TARGET_NEON)
#define HAVE_neon_vdup_nv16qi (TARGET_NEON)
#define HAVE_neon_vdup_nv8hi (TARGET_NEON)
#define HAVE_neon_vdup_nv2si (TARGET_NEON)
#define HAVE_neon_vdup_nv2sf (TARGET_NEON)
#define HAVE_neon_vdup_nv4si (TARGET_NEON)
#define HAVE_neon_vdup_nv4sf (TARGET_NEON)
#define HAVE_neon_vdup_nv2di (TARGET_NEON)
#define HAVE_neon_vdup_lanev8qi_internal (TARGET_NEON)
#define HAVE_neon_vdup_lanev16qi_internal (TARGET_NEON)
#define HAVE_neon_vdup_lanev4hi_internal (TARGET_NEON)
#define HAVE_neon_vdup_lanev8hi_internal (TARGET_NEON)
#define HAVE_neon_vdup_lanev2si_internal (TARGET_NEON)
#define HAVE_neon_vdup_lanev4si_internal (TARGET_NEON)
#define HAVE_neon_vdup_lanev2sf_internal (TARGET_NEON)
#define HAVE_neon_vdup_lanev4sf_internal (TARGET_NEON)
#define HAVE_neon_vcombinev8qi (TARGET_NEON)
#define HAVE_neon_vcombinev4hi (TARGET_NEON)
#define HAVE_neon_vcombinev2si (TARGET_NEON)
#define HAVE_neon_vcombinev2sf (TARGET_NEON)
#define HAVE_neon_vcombinedi (TARGET_NEON)
#define HAVE_floatv2siv2sf2 (TARGET_NEON && !flag_rounding_math)
#define HAVE_floatv4siv4sf2 (TARGET_NEON && !flag_rounding_math)
#define HAVE_floatunsv2siv2sf2 (TARGET_NEON && !flag_rounding_math)
#define HAVE_floatunsv4siv4sf2 (TARGET_NEON && !flag_rounding_math)
#define HAVE_fix_truncv2sfv2si2 (TARGET_NEON)
#define HAVE_fix_truncv4sfv4si2 (TARGET_NEON)
#define HAVE_fixuns_truncv2sfv2si2 (TARGET_NEON)
#define HAVE_fixuns_truncv4sfv4si2 (TARGET_NEON)
#define HAVE_neon_vcvtsv2sf (TARGET_NEON)
#define HAVE_neon_vcvtuv2sf (TARGET_NEON)
#define HAVE_neon_vcvtsv4sf (TARGET_NEON)
#define HAVE_neon_vcvtuv4sf (TARGET_NEON)
#define HAVE_neon_vcvtsv2si (TARGET_NEON)
#define HAVE_neon_vcvtuv2si (TARGET_NEON)
#define HAVE_neon_vcvtsv4si (TARGET_NEON)
#define HAVE_neon_vcvtuv4si (TARGET_NEON)
#define HAVE_neon_vcvtv4sfv4hf (TARGET_NEON && TARGET_FP16)
#define HAVE_neon_vcvtv4hfv4sf (TARGET_NEON && TARGET_FP16)
#define HAVE_neon_vcvts_nv2sf (TARGET_NEON)
#define HAVE_neon_vcvtu_nv2sf (TARGET_NEON)
#define HAVE_neon_vcvts_nv4sf (TARGET_NEON)
#define HAVE_neon_vcvtu_nv4sf (TARGET_NEON)
#define HAVE_neon_vcvts_nv2si (TARGET_NEON)
#define HAVE_neon_vcvtu_nv2si (TARGET_NEON)
#define HAVE_neon_vcvts_nv4si (TARGET_NEON)
#define HAVE_neon_vcvtu_nv4si (TARGET_NEON)
#define HAVE_neon_vmovnv8hi (TARGET_NEON)
#define HAVE_neon_vmovnv4si (TARGET_NEON)
#define HAVE_neon_vmovnv2di (TARGET_NEON)
#define HAVE_neon_vqmovnsv8hi (TARGET_NEON)
#define HAVE_neon_vqmovnuv8hi (TARGET_NEON)
#define HAVE_neon_vqmovnsv4si (TARGET_NEON)
#define HAVE_neon_vqmovnuv4si (TARGET_NEON)
#define HAVE_neon_vqmovnsv2di (TARGET_NEON)
#define HAVE_neon_vqmovnuv2di (TARGET_NEON)
#define HAVE_neon_vqmovunv8hi (TARGET_NEON)
#define HAVE_neon_vqmovunv4si (TARGET_NEON)
#define HAVE_neon_vqmovunv2di (TARGET_NEON)
#define HAVE_neon_vmovlsv8qi (TARGET_NEON)
#define HAVE_neon_vmovluv8qi (TARGET_NEON)
#define HAVE_neon_vmovlsv4hi (TARGET_NEON)
#define HAVE_neon_vmovluv4hi (TARGET_NEON)
#define HAVE_neon_vmovlsv2si (TARGET_NEON)
#define HAVE_neon_vmovluv2si (TARGET_NEON)
#define HAVE_neon_vmul_lanev4hi (TARGET_NEON)
#define HAVE_neon_vmul_lanev2si (TARGET_NEON)
#define HAVE_neon_vmul_lanev2sf (TARGET_NEON)
#define HAVE_neon_vmul_lanev8hi (TARGET_NEON)
#define HAVE_neon_vmul_lanev4si (TARGET_NEON)
#define HAVE_neon_vmul_lanev4sf (TARGET_NEON)
#define HAVE_neon_vmulls_lanev4hi (TARGET_NEON)
#define HAVE_neon_vmullu_lanev4hi (TARGET_NEON)
#define HAVE_neon_vmulls_lanev2si (TARGET_NEON)
#define HAVE_neon_vmullu_lanev2si (TARGET_NEON)
#define HAVE_neon_vqdmull_lanev4hi (TARGET_NEON)
#define HAVE_neon_vqdmull_lanev2si (TARGET_NEON)
#define HAVE_neon_vqdmulh_lanev8hi (TARGET_NEON)
#define HAVE_neon_vqrdmulh_lanev8hi (TARGET_NEON)
#define HAVE_neon_vqdmulh_lanev4si (TARGET_NEON)
#define HAVE_neon_vqrdmulh_lanev4si (TARGET_NEON)
#define HAVE_neon_vqdmulh_lanev4hi (TARGET_NEON)
#define HAVE_neon_vqrdmulh_lanev4hi (TARGET_NEON)
#define HAVE_neon_vqdmulh_lanev2si (TARGET_NEON)
#define HAVE_neon_vqrdmulh_lanev2si (TARGET_NEON)
#define HAVE_neon_vmla_lanev4hi (TARGET_NEON)
#define HAVE_neon_vmla_lanev2si (TARGET_NEON)
#define HAVE_neon_vmla_lanev2sf (TARGET_NEON)
#define HAVE_neon_vmla_lanev8hi (TARGET_NEON)
#define HAVE_neon_vmla_lanev4si (TARGET_NEON)
#define HAVE_neon_vmla_lanev4sf (TARGET_NEON)
#define HAVE_neon_vmlals_lanev4hi (TARGET_NEON)
#define HAVE_neon_vmlalu_lanev4hi (TARGET_NEON)
#define HAVE_neon_vmlals_lanev2si (TARGET_NEON)
#define HAVE_neon_vmlalu_lanev2si (TARGET_NEON)
#define HAVE_neon_vqdmlal_lanev4hi (TARGET_NEON)
#define HAVE_neon_vqdmlal_lanev2si (TARGET_NEON)
#define HAVE_neon_vmls_lanev4hi (TARGET_NEON)
#define HAVE_neon_vmls_lanev2si (TARGET_NEON)
#define HAVE_neon_vmls_lanev2sf (TARGET_NEON)
#define HAVE_neon_vmls_lanev8hi (TARGET_NEON)
#define HAVE_neon_vmls_lanev4si (TARGET_NEON)
#define HAVE_neon_vmls_lanev4sf (TARGET_NEON)
#define HAVE_neon_vmlsls_lanev4hi (TARGET_NEON)
#define HAVE_neon_vmlslu_lanev4hi (TARGET_NEON)
#define HAVE_neon_vmlsls_lanev2si (TARGET_NEON)
#define HAVE_neon_vmlslu_lanev2si (TARGET_NEON)
#define HAVE_neon_vqdmlsl_lanev4hi (TARGET_NEON)
#define HAVE_neon_vqdmlsl_lanev2si (TARGET_NEON)
#define HAVE_neon_vextv8qi (TARGET_NEON)
#define HAVE_neon_vextv16qi (TARGET_NEON)
#define HAVE_neon_vextv4hi (TARGET_NEON)
#define HAVE_neon_vextv8hi (TARGET_NEON)
#define HAVE_neon_vextv2si (TARGET_NEON)
#define HAVE_neon_vextv4si (TARGET_NEON)
#define HAVE_neon_vextv2sf (TARGET_NEON)
#define HAVE_neon_vextv4sf (TARGET_NEON)
#define HAVE_neon_vextdi (TARGET_NEON)
#define HAVE_neon_vextv2di (TARGET_NEON)
#define HAVE_neon_vrev64v8qi (TARGET_NEON)
#define HAVE_neon_vrev64v16qi (TARGET_NEON)
#define HAVE_neon_vrev64v4hi (TARGET_NEON)
#define HAVE_neon_vrev64v8hi (TARGET_NEON)
#define HAVE_neon_vrev64v2si (TARGET_NEON)
#define HAVE_neon_vrev64v4si (TARGET_NEON)
#define HAVE_neon_vrev64v2sf (TARGET_NEON)
#define HAVE_neon_vrev64v4sf (TARGET_NEON)
#define HAVE_neon_vrev64v2di (TARGET_NEON)
#define HAVE_neon_vrev32v8qi (TARGET_NEON)
#define HAVE_neon_vrev32v4hi (TARGET_NEON)
#define HAVE_neon_vrev32v16qi (TARGET_NEON)
#define HAVE_neon_vrev32v8hi (TARGET_NEON)
#define HAVE_neon_vrev16v8qi (TARGET_NEON)
#define HAVE_neon_vrev16v16qi (TARGET_NEON)
#define HAVE_neon_vbslv8qi_internal (TARGET_NEON)
#define HAVE_neon_vbslv16qi_internal (TARGET_NEON)
#define HAVE_neon_vbslv4hi_internal (TARGET_NEON)
#define HAVE_neon_vbslv8hi_internal (TARGET_NEON)
#define HAVE_neon_vbslv2si_internal (TARGET_NEON)
#define HAVE_neon_vbslv4si_internal (TARGET_NEON)
#define HAVE_neon_vbslv2sf_internal (TARGET_NEON)
#define HAVE_neon_vbslv4sf_internal (TARGET_NEON)
#define HAVE_neon_vbsldi_internal (TARGET_NEON)
#define HAVE_neon_vbslv2di_internal (TARGET_NEON)
#define HAVE_neon_vshlsv8qi (TARGET_NEON)
#define HAVE_neon_vshluv8qi (TARGET_NEON)
#define HAVE_neon_vrshlsv8qi (TARGET_NEON)
#define HAVE_neon_vrshluv8qi (TARGET_NEON)
#define HAVE_neon_vshlsv16qi (TARGET_NEON)
#define HAVE_neon_vshluv16qi (TARGET_NEON)
#define HAVE_neon_vrshlsv16qi (TARGET_NEON)
#define HAVE_neon_vrshluv16qi (TARGET_NEON)
#define HAVE_neon_vshlsv4hi (TARGET_NEON)
#define HAVE_neon_vshluv4hi (TARGET_NEON)
#define HAVE_neon_vrshlsv4hi (TARGET_NEON)
#define HAVE_neon_vrshluv4hi (TARGET_NEON)
#define HAVE_neon_vshlsv8hi (TARGET_NEON)
#define HAVE_neon_vshluv8hi (TARGET_NEON)
#define HAVE_neon_vrshlsv8hi (TARGET_NEON)
#define HAVE_neon_vrshluv8hi (TARGET_NEON)
#define HAVE_neon_vshlsv2si (TARGET_NEON)
#define HAVE_neon_vshluv2si (TARGET_NEON)
#define HAVE_neon_vrshlsv2si (TARGET_NEON)
#define HAVE_neon_vrshluv2si (TARGET_NEON)
#define HAVE_neon_vshlsv4si (TARGET_NEON)
#define HAVE_neon_vshluv4si (TARGET_NEON)
#define HAVE_neon_vrshlsv4si (TARGET_NEON)
#define HAVE_neon_vrshluv4si (TARGET_NEON)
#define HAVE_neon_vshlsdi (TARGET_NEON)
#define HAVE_neon_vshludi (TARGET_NEON)
#define HAVE_neon_vrshlsdi (TARGET_NEON)
#define HAVE_neon_vrshludi (TARGET_NEON)
#define HAVE_neon_vshlsv2di (TARGET_NEON)
#define HAVE_neon_vshluv2di (TARGET_NEON)
#define HAVE_neon_vrshlsv2di (TARGET_NEON)
#define HAVE_neon_vrshluv2di (TARGET_NEON)
#define HAVE_neon_vqshlsv8qi (TARGET_NEON)
#define HAVE_neon_vqshluv8qi (TARGET_NEON)
#define HAVE_neon_vqrshlsv8qi (TARGET_NEON)
#define HAVE_neon_vqrshluv8qi (TARGET_NEON)
#define HAVE_neon_vqshlsv16qi (TARGET_NEON)
#define HAVE_neon_vqshluv16qi (TARGET_NEON)
#define HAVE_neon_vqrshlsv16qi (TARGET_NEON)
#define HAVE_neon_vqrshluv16qi (TARGET_NEON)
#define HAVE_neon_vqshlsv4hi (TARGET_NEON)
#define HAVE_neon_vqshluv4hi (TARGET_NEON)
#define HAVE_neon_vqrshlsv4hi (TARGET_NEON)
#define HAVE_neon_vqrshluv4hi (TARGET_NEON)
#define HAVE_neon_vqshlsv8hi (TARGET_NEON)
#define HAVE_neon_vqshluv8hi (TARGET_NEON)
#define HAVE_neon_vqrshlsv8hi (TARGET_NEON)
#define HAVE_neon_vqrshluv8hi (TARGET_NEON)
#define HAVE_neon_vqshlsv2si (TARGET_NEON)
#define HAVE_neon_vqshluv2si (TARGET_NEON)
#define HAVE_neon_vqrshlsv2si (TARGET_NEON)
#define HAVE_neon_vqrshluv2si (TARGET_NEON)
#define HAVE_neon_vqshlsv4si (TARGET_NEON)
#define HAVE_neon_vqshluv4si (TARGET_NEON)
#define HAVE_neon_vqrshlsv4si (TARGET_NEON)
#define HAVE_neon_vqrshluv4si (TARGET_NEON)
#define HAVE_neon_vqshlsdi (TARGET_NEON)
#define HAVE_neon_vqshludi (TARGET_NEON)
#define HAVE_neon_vqrshlsdi (TARGET_NEON)
#define HAVE_neon_vqrshludi (TARGET_NEON)
#define HAVE_neon_vqshlsv2di (TARGET_NEON)
#define HAVE_neon_vqshluv2di (TARGET_NEON)
#define HAVE_neon_vqrshlsv2di (TARGET_NEON)
#define HAVE_neon_vqrshluv2di (TARGET_NEON)
#define HAVE_neon_vshrs_nv8qi (TARGET_NEON)
#define HAVE_neon_vshru_nv8qi (TARGET_NEON)
#define HAVE_neon_vrshrs_nv8qi (TARGET_NEON)
#define HAVE_neon_vrshru_nv8qi (TARGET_NEON)
#define HAVE_neon_vshrs_nv16qi (TARGET_NEON)
#define HAVE_neon_vshru_nv16qi (TARGET_NEON)
#define HAVE_neon_vrshrs_nv16qi (TARGET_NEON)
#define HAVE_neon_vrshru_nv16qi (TARGET_NEON)
#define HAVE_neon_vshrs_nv4hi (TARGET_NEON)
#define HAVE_neon_vshru_nv4hi (TARGET_NEON)
#define HAVE_neon_vrshrs_nv4hi (TARGET_NEON)
#define HAVE_neon_vrshru_nv4hi (TARGET_NEON)
#define HAVE_neon_vshrs_nv8hi (TARGET_NEON)
#define HAVE_neon_vshru_nv8hi (TARGET_NEON)
#define HAVE_neon_vrshrs_nv8hi (TARGET_NEON)
#define HAVE_neon_vrshru_nv8hi (TARGET_NEON)
#define HAVE_neon_vshrs_nv2si (TARGET_NEON)
#define HAVE_neon_vshru_nv2si (TARGET_NEON)
#define HAVE_neon_vrshrs_nv2si (TARGET_NEON)
#define HAVE_neon_vrshru_nv2si (TARGET_NEON)
#define HAVE_neon_vshrs_nv4si (TARGET_NEON)
#define HAVE_neon_vshru_nv4si (TARGET_NEON)
#define HAVE_neon_vrshrs_nv4si (TARGET_NEON)
#define HAVE_neon_vrshru_nv4si (TARGET_NEON)
#define HAVE_neon_vshrs_ndi (TARGET_NEON)
#define HAVE_neon_vshru_ndi (TARGET_NEON)
#define HAVE_neon_vrshrs_ndi (TARGET_NEON)
#define HAVE_neon_vrshru_ndi (TARGET_NEON)
#define HAVE_neon_vshrs_nv2di (TARGET_NEON)
#define HAVE_neon_vshru_nv2di (TARGET_NEON)
#define HAVE_neon_vrshrs_nv2di (TARGET_NEON)
#define HAVE_neon_vrshru_nv2di (TARGET_NEON)
#define HAVE_neon_vshrn_nv8hi (TARGET_NEON)
#define HAVE_neon_vrshrn_nv8hi (TARGET_NEON)
#define HAVE_neon_vshrn_nv4si (TARGET_NEON)
#define HAVE_neon_vrshrn_nv4si (TARGET_NEON)
#define HAVE_neon_vshrn_nv2di (TARGET_NEON)
#define HAVE_neon_vrshrn_nv2di (TARGET_NEON)
#define HAVE_neon_vqshrns_nv8hi (TARGET_NEON)
#define HAVE_neon_vqshrnu_nv8hi (TARGET_NEON)
#define HAVE_neon_vqrshrns_nv8hi (TARGET_NEON)
#define HAVE_neon_vqrshrnu_nv8hi (TARGET_NEON)
#define HAVE_neon_vqshrns_nv4si (TARGET_NEON)
#define HAVE_neon_vqshrnu_nv4si (TARGET_NEON)
#define HAVE_neon_vqrshrns_nv4si (TARGET_NEON)
#define HAVE_neon_vqrshrnu_nv4si (TARGET_NEON)
#define HAVE_neon_vqshrns_nv2di (TARGET_NEON)
#define HAVE_neon_vqshrnu_nv2di (TARGET_NEON)
#define HAVE_neon_vqrshrns_nv2di (TARGET_NEON)
#define HAVE_neon_vqrshrnu_nv2di (TARGET_NEON)
#define HAVE_neon_vqshrun_nv8hi (TARGET_NEON)
#define HAVE_neon_vqrshrun_nv8hi (TARGET_NEON)
#define HAVE_neon_vqshrun_nv4si (TARGET_NEON)
#define HAVE_neon_vqrshrun_nv4si (TARGET_NEON)
#define HAVE_neon_vqshrun_nv2di (TARGET_NEON)
#define HAVE_neon_vqrshrun_nv2di (TARGET_NEON)
#define HAVE_neon_vshl_nv8qi (TARGET_NEON)
#define HAVE_neon_vshl_nv16qi (TARGET_NEON)
#define HAVE_neon_vshl_nv4hi (TARGET_NEON)
#define HAVE_neon_vshl_nv8hi (TARGET_NEON)
#define HAVE_neon_vshl_nv2si (TARGET_NEON)
#define HAVE_neon_vshl_nv4si (TARGET_NEON)
#define HAVE_neon_vshl_ndi (TARGET_NEON)
#define HAVE_neon_vshl_nv2di (TARGET_NEON)
#define HAVE_neon_vqshl_s_nv8qi (TARGET_NEON)
#define HAVE_neon_vqshl_u_nv8qi (TARGET_NEON)
#define HAVE_neon_vqshl_s_nv16qi (TARGET_NEON)
#define HAVE_neon_vqshl_u_nv16qi (TARGET_NEON)
#define HAVE_neon_vqshl_s_nv4hi (TARGET_NEON)
#define HAVE_neon_vqshl_u_nv4hi (TARGET_NEON)
#define HAVE_neon_vqshl_s_nv8hi (TARGET_NEON)
#define HAVE_neon_vqshl_u_nv8hi (TARGET_NEON)
#define HAVE_neon_vqshl_s_nv2si (TARGET_NEON)
#define HAVE_neon_vqshl_u_nv2si (TARGET_NEON)
#define HAVE_neon_vqshl_s_nv4si (TARGET_NEON)
#define HAVE_neon_vqshl_u_nv4si (TARGET_NEON)
#define HAVE_neon_vqshl_s_ndi (TARGET_NEON)
#define HAVE_neon_vqshl_u_ndi (TARGET_NEON)
#define HAVE_neon_vqshl_s_nv2di (TARGET_NEON)
#define HAVE_neon_vqshl_u_nv2di (TARGET_NEON)
#define HAVE_neon_vqshlu_nv8qi (TARGET_NEON)
#define HAVE_neon_vqshlu_nv16qi (TARGET_NEON)
#define HAVE_neon_vqshlu_nv4hi (TARGET_NEON)
#define HAVE_neon_vqshlu_nv8hi (TARGET_NEON)
#define HAVE_neon_vqshlu_nv2si (TARGET_NEON)
#define HAVE_neon_vqshlu_nv4si (TARGET_NEON)
#define HAVE_neon_vqshlu_ndi (TARGET_NEON)
#define HAVE_neon_vqshlu_nv2di (TARGET_NEON)
#define HAVE_neon_vshlls_nv8qi (TARGET_NEON)
#define HAVE_neon_vshllu_nv8qi (TARGET_NEON)
#define HAVE_neon_vshlls_nv4hi (TARGET_NEON)
#define HAVE_neon_vshllu_nv4hi (TARGET_NEON)
#define HAVE_neon_vshlls_nv2si (TARGET_NEON)
#define HAVE_neon_vshllu_nv2si (TARGET_NEON)
#define HAVE_neon_vsras_nv8qi (TARGET_NEON)
#define HAVE_neon_vsrau_nv8qi (TARGET_NEON)
#define HAVE_neon_vrsras_nv8qi (TARGET_NEON)
#define HAVE_neon_vrsrau_nv8qi (TARGET_NEON)
#define HAVE_neon_vsras_nv16qi (TARGET_NEON)
#define HAVE_neon_vsrau_nv16qi (TARGET_NEON)
#define HAVE_neon_vrsras_nv16qi (TARGET_NEON)
#define HAVE_neon_vrsrau_nv16qi (TARGET_NEON)
#define HAVE_neon_vsras_nv4hi (TARGET_NEON)
#define HAVE_neon_vsrau_nv4hi (TARGET_NEON)
#define HAVE_neon_vrsras_nv4hi (TARGET_NEON)
#define HAVE_neon_vrsrau_nv4hi (TARGET_NEON)
#define HAVE_neon_vsras_nv8hi (TARGET_NEON)
#define HAVE_neon_vsrau_nv8hi (TARGET_NEON)
#define HAVE_neon_vrsras_nv8hi (TARGET_NEON)
#define HAVE_neon_vrsrau_nv8hi (TARGET_NEON)
#define HAVE_neon_vsras_nv2si (TARGET_NEON)
#define HAVE_neon_vsrau_nv2si (TARGET_NEON)
#define HAVE_neon_vrsras_nv2si (TARGET_NEON)
#define HAVE_neon_vrsrau_nv2si (TARGET_NEON)
#define HAVE_neon_vsras_nv4si (TARGET_NEON)
#define HAVE_neon_vsrau_nv4si (TARGET_NEON)
#define HAVE_neon_vrsras_nv4si (TARGET_NEON)
#define HAVE_neon_vrsrau_nv4si (TARGET_NEON)
#define HAVE_neon_vsras_ndi (TARGET_NEON)
#define HAVE_neon_vsrau_ndi (TARGET_NEON)
#define HAVE_neon_vrsras_ndi (TARGET_NEON)
#define HAVE_neon_vrsrau_ndi (TARGET_NEON)
#define HAVE_neon_vsras_nv2di (TARGET_NEON)
#define HAVE_neon_vsrau_nv2di (TARGET_NEON)
#define HAVE_neon_vrsras_nv2di (TARGET_NEON)
#define HAVE_neon_vrsrau_nv2di (TARGET_NEON)
#define HAVE_neon_vsri_nv8qi (TARGET_NEON)
#define HAVE_neon_vsri_nv16qi (TARGET_NEON)
#define HAVE_neon_vsri_nv4hi (TARGET_NEON)
#define HAVE_neon_vsri_nv8hi (TARGET_NEON)
#define HAVE_neon_vsri_nv2si (TARGET_NEON)
#define HAVE_neon_vsri_nv4si (TARGET_NEON)
#define HAVE_neon_vsri_ndi (TARGET_NEON)
#define HAVE_neon_vsri_nv2di (TARGET_NEON)
#define HAVE_neon_vsli_nv8qi (TARGET_NEON)
#define HAVE_neon_vsli_nv16qi (TARGET_NEON)
#define HAVE_neon_vsli_nv4hi (TARGET_NEON)
#define HAVE_neon_vsli_nv8hi (TARGET_NEON)
#define HAVE_neon_vsli_nv2si (TARGET_NEON)
#define HAVE_neon_vsli_nv4si (TARGET_NEON)
#define HAVE_neon_vsli_ndi (TARGET_NEON)
#define HAVE_neon_vsli_nv2di (TARGET_NEON)
#define HAVE_neon_vtbl1v8qi (TARGET_NEON)
#define HAVE_neon_vtbl2v8qi (TARGET_NEON)
#define HAVE_neon_vtbl3v8qi (TARGET_NEON)
#define HAVE_neon_vtbl4v8qi (TARGET_NEON)
#define HAVE_neon_vtbl1v16qi (TARGET_NEON)
#define HAVE_neon_vtbl2v16qi (TARGET_NEON)
#define HAVE_neon_vcombinev16qi (TARGET_NEON)
#define HAVE_neon_vtbx1v8qi (TARGET_NEON)
#define HAVE_neon_vtbx2v8qi (TARGET_NEON)
#define HAVE_neon_vtbx3v8qi (TARGET_NEON)
#define HAVE_neon_vtbx4v8qi (TARGET_NEON)
#define HAVE_neon_vld1v8qi (TARGET_NEON)
#define HAVE_neon_vld1v16qi (TARGET_NEON)
#define HAVE_neon_vld1v4hi (TARGET_NEON)
#define HAVE_neon_vld1v8hi (TARGET_NEON)
#define HAVE_neon_vld1v2si (TARGET_NEON)
#define HAVE_neon_vld1v4si (TARGET_NEON)
#define HAVE_neon_vld1v2sf (TARGET_NEON)
#define HAVE_neon_vld1v4sf (TARGET_NEON)
#define HAVE_neon_vld1di (TARGET_NEON)
#define HAVE_neon_vld1v2di (TARGET_NEON)
#define HAVE_neon_vld1_lanev8qi (TARGET_NEON)
#define HAVE_neon_vld1_lanev4hi (TARGET_NEON)
#define HAVE_neon_vld1_lanev2si (TARGET_NEON)
#define HAVE_neon_vld1_lanev2sf (TARGET_NEON)
#define HAVE_neon_vld1_lanedi (TARGET_NEON)
#define HAVE_neon_vld1_lanev16qi (TARGET_NEON)
#define HAVE_neon_vld1_lanev8hi (TARGET_NEON)
#define HAVE_neon_vld1_lanev4si (TARGET_NEON)
#define HAVE_neon_vld1_lanev4sf (TARGET_NEON)
#define HAVE_neon_vld1_lanev2di (TARGET_NEON)
#define HAVE_neon_vld1_dupv8qi (TARGET_NEON)
#define HAVE_neon_vld1_dupv4hi (TARGET_NEON)
#define HAVE_neon_vld1_dupv2si (TARGET_NEON)
#define HAVE_neon_vld1_dupv2sf (TARGET_NEON)
#define HAVE_neon_vld1_dupv16qi (TARGET_NEON)
#define HAVE_neon_vld1_dupv8hi (TARGET_NEON)
#define HAVE_neon_vld1_dupv4si (TARGET_NEON)
#define HAVE_neon_vld1_dupv4sf (TARGET_NEON)
#define HAVE_neon_vld1_dupv2di (TARGET_NEON)
#define HAVE_neon_vst1v8qi (TARGET_NEON)
#define HAVE_neon_vst1v16qi (TARGET_NEON)
#define HAVE_neon_vst1v4hi (TARGET_NEON)
#define HAVE_neon_vst1v8hi (TARGET_NEON)
#define HAVE_neon_vst1v2si (TARGET_NEON)
#define HAVE_neon_vst1v4si (TARGET_NEON)
#define HAVE_neon_vst1v2sf (TARGET_NEON)
#define HAVE_neon_vst1v4sf (TARGET_NEON)
#define HAVE_neon_vst1di (TARGET_NEON)
#define HAVE_neon_vst1v2di (TARGET_NEON)
#define HAVE_neon_vst1_lanev8qi (TARGET_NEON)
#define HAVE_neon_vst1_lanev4hi (TARGET_NEON)
#define HAVE_neon_vst1_lanev2si (TARGET_NEON)
#define HAVE_neon_vst1_lanev2sf (TARGET_NEON)
#define HAVE_neon_vst1_lanedi (TARGET_NEON)
#define HAVE_neon_vst1_lanev16qi (TARGET_NEON)
#define HAVE_neon_vst1_lanev8hi (TARGET_NEON)
#define HAVE_neon_vst1_lanev4si (TARGET_NEON)
#define HAVE_neon_vst1_lanev4sf (TARGET_NEON)
#define HAVE_neon_vst1_lanev2di (TARGET_NEON)
#define HAVE_neon_vld2v8qi (TARGET_NEON)
#define HAVE_neon_vld2v4hi (TARGET_NEON)
#define HAVE_neon_vld2v2si (TARGET_NEON)
#define HAVE_neon_vld2v2sf (TARGET_NEON)
#define HAVE_neon_vld2di (TARGET_NEON)
#define HAVE_neon_vld2v16qi (TARGET_NEON)
#define HAVE_neon_vld2v8hi (TARGET_NEON)
#define HAVE_neon_vld2v4si (TARGET_NEON)
#define HAVE_neon_vld2v4sf (TARGET_NEON)
#define HAVE_neon_vld2_lanev8qi (TARGET_NEON)
#define HAVE_neon_vld2_lanev4hi (TARGET_NEON)
#define HAVE_neon_vld2_lanev2si (TARGET_NEON)
#define HAVE_neon_vld2_lanev2sf (TARGET_NEON)
#define HAVE_neon_vld2_lanev8hi (TARGET_NEON)
#define HAVE_neon_vld2_lanev4si (TARGET_NEON)
#define HAVE_neon_vld2_lanev4sf (TARGET_NEON)
#define HAVE_neon_vld2_dupv8qi (TARGET_NEON)
#define HAVE_neon_vld2_dupv4hi (TARGET_NEON)
#define HAVE_neon_vld2_dupv2si (TARGET_NEON)
#define HAVE_neon_vld2_dupv2sf (TARGET_NEON)
#define HAVE_neon_vld2_dupdi (TARGET_NEON)
#define HAVE_neon_vst2v8qi (TARGET_NEON)
#define HAVE_neon_vst2v4hi (TARGET_NEON)
#define HAVE_neon_vst2v2si (TARGET_NEON)
#define HAVE_neon_vst2v2sf (TARGET_NEON)
#define HAVE_neon_vst2di (TARGET_NEON)
#define HAVE_neon_vst2v16qi (TARGET_NEON)
#define HAVE_neon_vst2v8hi (TARGET_NEON)
#define HAVE_neon_vst2v4si (TARGET_NEON)
#define HAVE_neon_vst2v4sf (TARGET_NEON)
#define HAVE_neon_vst2_lanev8qi (TARGET_NEON)
#define HAVE_neon_vst2_lanev4hi (TARGET_NEON)
#define HAVE_neon_vst2_lanev2si (TARGET_NEON)
#define HAVE_neon_vst2_lanev2sf (TARGET_NEON)
#define HAVE_neon_vst2_lanev8hi (TARGET_NEON)
#define HAVE_neon_vst2_lanev4si (TARGET_NEON)
#define HAVE_neon_vst2_lanev4sf (TARGET_NEON)
#define HAVE_neon_vld3v8qi (TARGET_NEON)
#define HAVE_neon_vld3v4hi (TARGET_NEON)
#define HAVE_neon_vld3v2si (TARGET_NEON)
#define HAVE_neon_vld3v2sf (TARGET_NEON)
#define HAVE_neon_vld3di (TARGET_NEON)
#define HAVE_neon_vld3qav16qi (TARGET_NEON)
#define HAVE_neon_vld3qav8hi (TARGET_NEON)
#define HAVE_neon_vld3qav4si (TARGET_NEON)
#define HAVE_neon_vld3qav4sf (TARGET_NEON)
#define HAVE_neon_vld3qbv16qi (TARGET_NEON)
#define HAVE_neon_vld3qbv8hi (TARGET_NEON)
#define HAVE_neon_vld3qbv4si (TARGET_NEON)
#define HAVE_neon_vld3qbv4sf (TARGET_NEON)
#define HAVE_neon_vld3_lanev8qi (TARGET_NEON)
#define HAVE_neon_vld3_lanev4hi (TARGET_NEON)
#define HAVE_neon_vld3_lanev2si (TARGET_NEON)
#define HAVE_neon_vld3_lanev2sf (TARGET_NEON)
#define HAVE_neon_vld3_lanev8hi (TARGET_NEON)
#define HAVE_neon_vld3_lanev4si (TARGET_NEON)
#define HAVE_neon_vld3_lanev4sf (TARGET_NEON)
#define HAVE_neon_vld3_dupv8qi (TARGET_NEON)
#define HAVE_neon_vld3_dupv4hi (TARGET_NEON)
#define HAVE_neon_vld3_dupv2si (TARGET_NEON)
#define HAVE_neon_vld3_dupv2sf (TARGET_NEON)
#define HAVE_neon_vld3_dupdi (TARGET_NEON)
#define HAVE_neon_vst3v8qi (TARGET_NEON)
#define HAVE_neon_vst3v4hi (TARGET_NEON)
#define HAVE_neon_vst3v2si (TARGET_NEON)
#define HAVE_neon_vst3v2sf (TARGET_NEON)
#define HAVE_neon_vst3di (TARGET_NEON)
#define HAVE_neon_vst3qav16qi (TARGET_NEON)
#define HAVE_neon_vst3qav8hi (TARGET_NEON)
#define HAVE_neon_vst3qav4si (TARGET_NEON)
#define HAVE_neon_vst3qav4sf (TARGET_NEON)
#define HAVE_neon_vst3qbv16qi (TARGET_NEON)
#define HAVE_neon_vst3qbv8hi (TARGET_NEON)
#define HAVE_neon_vst3qbv4si (TARGET_NEON)
#define HAVE_neon_vst3qbv4sf (TARGET_NEON)
#define HAVE_neon_vst3_lanev8qi (TARGET_NEON)
#define HAVE_neon_vst3_lanev4hi (TARGET_NEON)
#define HAVE_neon_vst3_lanev2si (TARGET_NEON)
#define HAVE_neon_vst3_lanev2sf (TARGET_NEON)
#define HAVE_neon_vst3_lanev8hi (TARGET_NEON)
#define HAVE_neon_vst3_lanev4si (TARGET_NEON)
#define HAVE_neon_vst3_lanev4sf (TARGET_NEON)
#define HAVE_neon_vld4v8qi (TARGET_NEON)
#define HAVE_neon_vld4v4hi (TARGET_NEON)
#define HAVE_neon_vld4v2si (TARGET_NEON)
#define HAVE_neon_vld4v2sf (TARGET_NEON)
#define HAVE_neon_vld4di (TARGET_NEON)
#define HAVE_neon_vld4qav16qi (TARGET_NEON)
#define HAVE_neon_vld4qav8hi (TARGET_NEON)
#define HAVE_neon_vld4qav4si (TARGET_NEON)
#define HAVE_neon_vld4qav4sf (TARGET_NEON)
#define HAVE_neon_vld4qbv16qi (TARGET_NEON)
#define HAVE_neon_vld4qbv8hi (TARGET_NEON)
#define HAVE_neon_vld4qbv4si (TARGET_NEON)
#define HAVE_neon_vld4qbv4sf (TARGET_NEON)
#define HAVE_neon_vld4_lanev8qi (TARGET_NEON)
#define HAVE_neon_vld4_lanev4hi (TARGET_NEON)
#define HAVE_neon_vld4_lanev2si (TARGET_NEON)
#define HAVE_neon_vld4_lanev2sf (TARGET_NEON)
#define HAVE_neon_vld4_lanev8hi (TARGET_NEON)
#define HAVE_neon_vld4_lanev4si (TARGET_NEON)
#define HAVE_neon_vld4_lanev4sf (TARGET_NEON)
#define HAVE_neon_vld4_dupv8qi (TARGET_NEON)
#define HAVE_neon_vld4_dupv4hi (TARGET_NEON)
#define HAVE_neon_vld4_dupv2si (TARGET_NEON)
#define HAVE_neon_vld4_dupv2sf (TARGET_NEON)
#define HAVE_neon_vld4_dupdi (TARGET_NEON)
#define HAVE_neon_vst4v8qi (TARGET_NEON)
#define HAVE_neon_vst4v4hi (TARGET_NEON)
#define HAVE_neon_vst4v2si (TARGET_NEON)
#define HAVE_neon_vst4v2sf (TARGET_NEON)
#define HAVE_neon_vst4di (TARGET_NEON)
#define HAVE_neon_vst4qav16qi (TARGET_NEON)
#define HAVE_neon_vst4qav8hi (TARGET_NEON)
#define HAVE_neon_vst4qav4si (TARGET_NEON)
#define HAVE_neon_vst4qav4sf (TARGET_NEON)
#define HAVE_neon_vst4qbv16qi (TARGET_NEON)
#define HAVE_neon_vst4qbv8hi (TARGET_NEON)
#define HAVE_neon_vst4qbv4si (TARGET_NEON)
#define HAVE_neon_vst4qbv4sf (TARGET_NEON)
#define HAVE_neon_vst4_lanev8qi (TARGET_NEON)
#define HAVE_neon_vst4_lanev4hi (TARGET_NEON)
#define HAVE_neon_vst4_lanev2si (TARGET_NEON)
#define HAVE_neon_vst4_lanev2sf (TARGET_NEON)
#define HAVE_neon_vst4_lanev8hi (TARGET_NEON)
#define HAVE_neon_vst4_lanev4si (TARGET_NEON)
#define HAVE_neon_vst4_lanev4sf (TARGET_NEON)
#define HAVE_neon_vec_unpacks_lo_v16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_unpacku_lo_v16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_unpacks_lo_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_unpacku_lo_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_unpacks_lo_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_unpacku_lo_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_unpacks_hi_v16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_unpacku_hi_v16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_unpacks_hi_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_unpacku_hi_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_unpacks_hi_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_unpacku_hi_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_smult_lo_v16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_umult_lo_v16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_smult_lo_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_umult_lo_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_smult_lo_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_umult_lo_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_smult_hi_v16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_umult_hi_v16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_smult_hi_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_umult_hi_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_smult_hi_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_umult_hi_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_sshiftl_v8qi (TARGET_NEON)
#define HAVE_neon_vec_ushiftl_v8qi (TARGET_NEON)
#define HAVE_neon_vec_sshiftl_v4hi (TARGET_NEON)
#define HAVE_neon_vec_ushiftl_v4hi (TARGET_NEON)
#define HAVE_neon_vec_sshiftl_v2si (TARGET_NEON)
#define HAVE_neon_vec_ushiftl_v2si (TARGET_NEON)
#define HAVE_neon_unpacks_v8qi (TARGET_NEON)
#define HAVE_neon_unpacku_v8qi (TARGET_NEON)
#define HAVE_neon_unpacks_v4hi (TARGET_NEON)
#define HAVE_neon_unpacku_v4hi (TARGET_NEON)
#define HAVE_neon_unpacks_v2si (TARGET_NEON)
#define HAVE_neon_unpacku_v2si (TARGET_NEON)
#define HAVE_neon_vec_smult_v8qi (TARGET_NEON)
#define HAVE_neon_vec_umult_v8qi (TARGET_NEON)
#define HAVE_neon_vec_smult_v4hi (TARGET_NEON)
#define HAVE_neon_vec_umult_v4hi (TARGET_NEON)
#define HAVE_neon_vec_smult_v2si (TARGET_NEON)
#define HAVE_neon_vec_umult_v2si (TARGET_NEON)
#define HAVE_vec_pack_trunc_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_pack_trunc_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_pack_trunc_v2di (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_pack_trunc_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_pack_trunc_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vec_pack_trunc_v2di (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_neon_vabdv8qi_2 (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_neon_vabdv16qi_2 (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_neon_vabdv4hi_2 (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_neon_vabdv8hi_2 (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_neon_vabdv2si_2 (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_neon_vabdv4si_2 (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_neon_vabdv2sf_2 (TARGET_NEON && (!true || flag_unsafe_math_optimizations))
#define HAVE_neon_vabdv4sf_2 (TARGET_NEON && (!true || flag_unsafe_math_optimizations))
#define HAVE_neon_vabdv2di_2 (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_neon_vabdv8qi_3 (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_neon_vabdv16qi_3 (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_neon_vabdv4hi_3 (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_neon_vabdv8hi_3 (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_neon_vabdv2si_3 (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_neon_vabdv4si_3 (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_neon_vabdv2sf_3 (TARGET_NEON && (!true || flag_unsafe_math_optimizations))
#define HAVE_neon_vabdv4sf_3 (TARGET_NEON && (!true || flag_unsafe_math_optimizations))
#define HAVE_neon_vabdv2di_3 (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_crypto_aesmc (TARGET_CRYPTO)
#define HAVE_crypto_aesimc (TARGET_CRYPTO)
#define HAVE_crypto_aesd (TARGET_CRYPTO)
#define HAVE_crypto_aese (TARGET_CRYPTO)
#define HAVE_crypto_sha1su1 (TARGET_CRYPTO)
#define HAVE_crypto_sha256su0 (TARGET_CRYPTO)
#define HAVE_crypto_sha1su0 (TARGET_CRYPTO)
#define HAVE_crypto_sha256h (TARGET_CRYPTO)
#define HAVE_crypto_sha256h2 (TARGET_CRYPTO)
#define HAVE_crypto_sha256su1 (TARGET_CRYPTO)
#define HAVE_crypto_sha1h (TARGET_CRYPTO)
#define HAVE_crypto_vmullp64 (TARGET_CRYPTO)
#define HAVE_crypto_sha1c (TARGET_CRYPTO)
#define HAVE_crypto_sha1m (TARGET_CRYPTO)
#define HAVE_crypto_sha1p (TARGET_CRYPTO)
#define HAVE_atomic_loadqi (TARGET_HAVE_LDACQ)
#define HAVE_atomic_loadhi (TARGET_HAVE_LDACQ)
#define HAVE_atomic_loadsi (TARGET_HAVE_LDACQ)
#define HAVE_atomic_storeqi (TARGET_HAVE_LDACQ)
#define HAVE_atomic_storehi (TARGET_HAVE_LDACQ)
#define HAVE_atomic_storesi (TARGET_HAVE_LDACQ)
#define HAVE_arm_atomic_loaddi2_ldrd (ARM_DOUBLEWORD_ALIGN && TARGET_HAVE_LPAE)
#define HAVE_atomic_compare_and_swapqi_1 (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_compare_and_swaphi_1 (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_compare_and_swapsi_1 (TARGET_HAVE_LDREX && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_compare_and_swapdi_1 (TARGET_HAVE_LDREXD && ARM_DOUBLEWORD_ALIGN \
	&& TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_exchangeqi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_exchangehi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_exchangesi (TARGET_HAVE_LDREX && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_exchangedi (TARGET_HAVE_LDREXD && ARM_DOUBLEWORD_ALIGN \
	&& TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_addqi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_subqi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_orqi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_xorqi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_andqi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_addhi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_subhi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_orhi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_xorhi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_andhi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_addsi (TARGET_HAVE_LDREX && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_subsi (TARGET_HAVE_LDREX && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_orsi (TARGET_HAVE_LDREX && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_xorsi (TARGET_HAVE_LDREX && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_andsi (TARGET_HAVE_LDREX && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_adddi (TARGET_HAVE_LDREXD && ARM_DOUBLEWORD_ALIGN \
	&& TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_subdi (TARGET_HAVE_LDREXD && ARM_DOUBLEWORD_ALIGN \
	&& TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_ordi (TARGET_HAVE_LDREXD && ARM_DOUBLEWORD_ALIGN \
	&& TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_xordi (TARGET_HAVE_LDREXD && ARM_DOUBLEWORD_ALIGN \
	&& TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_anddi (TARGET_HAVE_LDREXD && ARM_DOUBLEWORD_ALIGN \
	&& TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_nandqi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_nandhi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_nandsi (TARGET_HAVE_LDREX && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_nanddi (TARGET_HAVE_LDREXD && ARM_DOUBLEWORD_ALIGN \
	&& TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_fetch_addqi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_fetch_subqi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_fetch_orqi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_fetch_xorqi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_fetch_andqi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_fetch_addhi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_fetch_subhi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_fetch_orhi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_fetch_xorhi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_fetch_andhi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_fetch_addsi (TARGET_HAVE_LDREX && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_fetch_subsi (TARGET_HAVE_LDREX && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_fetch_orsi (TARGET_HAVE_LDREX && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_fetch_xorsi (TARGET_HAVE_LDREX && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_fetch_andsi (TARGET_HAVE_LDREX && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_fetch_adddi (TARGET_HAVE_LDREXD && ARM_DOUBLEWORD_ALIGN \
	&& TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_fetch_subdi (TARGET_HAVE_LDREXD && ARM_DOUBLEWORD_ALIGN \
	&& TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_fetch_ordi (TARGET_HAVE_LDREXD && ARM_DOUBLEWORD_ALIGN \
	&& TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_fetch_xordi (TARGET_HAVE_LDREXD && ARM_DOUBLEWORD_ALIGN \
	&& TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_fetch_anddi (TARGET_HAVE_LDREXD && ARM_DOUBLEWORD_ALIGN \
	&& TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_fetch_nandqi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_fetch_nandhi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_fetch_nandsi (TARGET_HAVE_LDREX && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_fetch_nanddi (TARGET_HAVE_LDREXD && ARM_DOUBLEWORD_ALIGN \
	&& TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_add_fetchqi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_sub_fetchqi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_or_fetchqi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_xor_fetchqi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_and_fetchqi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_add_fetchhi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_sub_fetchhi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_or_fetchhi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_xor_fetchhi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_and_fetchhi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_add_fetchsi (TARGET_HAVE_LDREX && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_sub_fetchsi (TARGET_HAVE_LDREX && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_or_fetchsi (TARGET_HAVE_LDREX && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_xor_fetchsi (TARGET_HAVE_LDREX && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_and_fetchsi (TARGET_HAVE_LDREX && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_add_fetchdi (TARGET_HAVE_LDREXD && ARM_DOUBLEWORD_ALIGN \
	&& TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_sub_fetchdi (TARGET_HAVE_LDREXD && ARM_DOUBLEWORD_ALIGN \
	&& TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_or_fetchdi (TARGET_HAVE_LDREXD && ARM_DOUBLEWORD_ALIGN \
	&& TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_xor_fetchdi (TARGET_HAVE_LDREXD && ARM_DOUBLEWORD_ALIGN \
	&& TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_and_fetchdi (TARGET_HAVE_LDREXD && ARM_DOUBLEWORD_ALIGN \
	&& TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_nand_fetchqi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_nand_fetchhi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_nand_fetchsi (TARGET_HAVE_LDREX && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_nand_fetchdi (TARGET_HAVE_LDREXD && ARM_DOUBLEWORD_ALIGN \
	&& TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_arm_load_exclusiveqi (TARGET_HAVE_LDREXBH)
#define HAVE_arm_load_exclusivehi (TARGET_HAVE_LDREXBH)
#define HAVE_arm_load_acquire_exclusiveqi (TARGET_HAVE_LDACQ)
#define HAVE_arm_load_acquire_exclusivehi (TARGET_HAVE_LDACQ)
#define HAVE_arm_load_exclusivesi (TARGET_HAVE_LDREX)
#define HAVE_arm_load_acquire_exclusivesi (TARGET_HAVE_LDACQ)
#define HAVE_arm_load_exclusivedi (TARGET_HAVE_LDREXD)
#define HAVE_arm_load_acquire_exclusivedi (TARGET_HAVE_LDACQ && ARM_DOUBLEWORD_ALIGN)
#define HAVE_arm_store_exclusiveqi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_arm_store_exclusivehi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_arm_store_exclusivesi (TARGET_HAVE_LDREX && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_arm_store_exclusivedi (TARGET_HAVE_LDREXD && ARM_DOUBLEWORD_ALIGN \
	&& TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_arm_store_release_exclusivedi (TARGET_HAVE_LDACQ && ARM_DOUBLEWORD_ALIGN)
#define HAVE_arm_store_release_exclusiveqi (TARGET_HAVE_LDACQ)
#define HAVE_arm_store_release_exclusivehi (TARGET_HAVE_LDACQ)
#define HAVE_arm_store_release_exclusivesi (TARGET_HAVE_LDACQ)
#define HAVE_addqq3 (TARGET_32BIT)
#define HAVE_addhq3 (TARGET_32BIT)
#define HAVE_addsq3 (TARGET_32BIT)
#define HAVE_adduqq3 (TARGET_32BIT)
#define HAVE_adduhq3 (TARGET_32BIT)
#define HAVE_addusq3 (TARGET_32BIT)
#define HAVE_addha3 (TARGET_32BIT)
#define HAVE_addsa3 (TARGET_32BIT)
#define HAVE_adduha3 (TARGET_32BIT)
#define HAVE_addusa3 (TARGET_32BIT)
#define HAVE_addv4qq3 (TARGET_INT_SIMD)
#define HAVE_addv2hq3 (TARGET_INT_SIMD)
#define HAVE_addv2ha3 (TARGET_INT_SIMD)
#define HAVE_usaddv4uqq3 (TARGET_INT_SIMD)
#define HAVE_usaddv2uhq3 (TARGET_INT_SIMD)
#define HAVE_usadduqq3 (TARGET_INT_SIMD)
#define HAVE_usadduhq3 (TARGET_INT_SIMD)
#define HAVE_usaddv2uha3 (TARGET_INT_SIMD)
#define HAVE_usadduha3 (TARGET_INT_SIMD)
#define HAVE_ssaddv4qq3 (TARGET_INT_SIMD)
#define HAVE_ssaddv2hq3 (TARGET_INT_SIMD)
#define HAVE_ssaddqq3 (TARGET_INT_SIMD)
#define HAVE_ssaddhq3 (TARGET_INT_SIMD)
#define HAVE_ssaddv2ha3 (TARGET_INT_SIMD)
#define HAVE_ssaddha3 (TARGET_INT_SIMD)
#define HAVE_ssaddsq3 (TARGET_INT_SIMD)
#define HAVE_ssaddsa3 (TARGET_INT_SIMD)
#define HAVE_subqq3 (TARGET_32BIT)
#define HAVE_subhq3 (TARGET_32BIT)
#define HAVE_subsq3 (TARGET_32BIT)
#define HAVE_subuqq3 (TARGET_32BIT)
#define HAVE_subuhq3 (TARGET_32BIT)
#define HAVE_subusq3 (TARGET_32BIT)
#define HAVE_subha3 (TARGET_32BIT)
#define HAVE_subsa3 (TARGET_32BIT)
#define HAVE_subuha3 (TARGET_32BIT)
#define HAVE_subusa3 (TARGET_32BIT)
#define HAVE_subv4qq3 (TARGET_INT_SIMD)
#define HAVE_subv2hq3 (TARGET_INT_SIMD)
#define HAVE_subv2ha3 (TARGET_INT_SIMD)
#define HAVE_ussubv4uqq3 (TARGET_INT_SIMD)
#define HAVE_ussubv2uhq3 (TARGET_INT_SIMD)
#define HAVE_ussubuqq3 (TARGET_INT_SIMD)
#define HAVE_ussubuhq3 (TARGET_INT_SIMD)
#define HAVE_ussubv2uha3 (TARGET_INT_SIMD)
#define HAVE_ussubuha3 (TARGET_INT_SIMD)
#define HAVE_sssubv4qq3 (TARGET_INT_SIMD)
#define HAVE_sssubv2hq3 (TARGET_INT_SIMD)
#define HAVE_sssubqq3 (TARGET_INT_SIMD)
#define HAVE_sssubhq3 (TARGET_INT_SIMD)
#define HAVE_sssubv2ha3 (TARGET_INT_SIMD)
#define HAVE_sssubha3 (TARGET_INT_SIMD)
#define HAVE_sssubsq3 (TARGET_INT_SIMD)
#define HAVE_sssubsa3 (TARGET_INT_SIMD)
#define HAVE_ssmulsa3 (TARGET_32BIT && arm_arch6)
#define HAVE_usmulusa3 (TARGET_32BIT && arm_arch6)
#define HAVE_arm_ssatsihi_shift (TARGET_32BIT && arm_arch6)
#define HAVE_arm_usatsihi (TARGET_INT_SIMD)
#define HAVE_adddi3 1
#define HAVE_addsi3 1
#define HAVE_addsf3 (TARGET_32BIT && TARGET_HARD_FLOAT)
#define HAVE_adddf3 (TARGET_32BIT && TARGET_HARD_FLOAT && !TARGET_VFP_SINGLE)
#define HAVE_subdi3 1
#define HAVE_subsi3 1
#define HAVE_subsf3 (TARGET_32BIT && TARGET_HARD_FLOAT)
#define HAVE_subdf3 (TARGET_32BIT && TARGET_HARD_FLOAT && !TARGET_VFP_SINGLE)
#define HAVE_mulhi3 (TARGET_DSP_MULTIPLY)
#define HAVE_mulsi3 1
#define HAVE_maddsidi4 (TARGET_32BIT && arm_arch3m)
#define HAVE_mulsidi3 (TARGET_32BIT && arm_arch3m)
#define HAVE_umulsidi3 (TARGET_32BIT && arm_arch3m)
#define HAVE_umaddsidi4 (TARGET_32BIT && arm_arch3m)
#define HAVE_smulsi3_highpart (TARGET_32BIT && arm_arch3m)
#define HAVE_umulsi3_highpart (TARGET_32BIT && arm_arch3m)
#define HAVE_mulsf3 (TARGET_32BIT && TARGET_HARD_FLOAT)
#define HAVE_muldf3 (TARGET_32BIT && TARGET_HARD_FLOAT && !TARGET_VFP_SINGLE)
#define HAVE_divsf3 (TARGET_32BIT && TARGET_HARD_FLOAT && TARGET_VFP)
#define HAVE_divdf3 (TARGET_32BIT && TARGET_HARD_FLOAT && TARGET_VFP_DOUBLE)
#define HAVE_anddi3 (TARGET_32BIT)
#define HAVE_andsi3 1
#define HAVE_insv (TARGET_ARM || arm_arch_thumb2)
#define HAVE_iordi3 (TARGET_32BIT)
#define HAVE_iorsi3 1
#define HAVE_xordi3 (TARGET_32BIT)
#define HAVE_xorsi3 1
#define HAVE_smaxsi3 (TARGET_32BIT)
#define HAVE_sminsi3 (TARGET_32BIT)
#define HAVE_umaxsi3 (TARGET_32BIT)
#define HAVE_uminsi3 (TARGET_32BIT)
#define HAVE_ashldi3 (TARGET_32BIT)
#define HAVE_ashlsi3 1
#define HAVE_ashrdi3 (TARGET_32BIT)
#define HAVE_ashrsi3 1
#define HAVE_lshrdi3 (TARGET_32BIT)
#define HAVE_lshrsi3 1
#define HAVE_rotlsi3 (TARGET_32BIT)
#define HAVE_rotrsi3 1
#define HAVE_extzv (TARGET_THUMB1 || arm_arch_thumb2)
#define HAVE_extzv_t1 (TARGET_THUMB1)
#define HAVE_extv (arm_arch_thumb2)
#define HAVE_extv_regsi 1
#define HAVE_negdi2 1
#define HAVE_negsi2 1
#define HAVE_negsf2 (TARGET_32BIT && TARGET_HARD_FLOAT && TARGET_VFP)
#define HAVE_negdf2 (TARGET_32BIT && TARGET_HARD_FLOAT && TARGET_VFP_DOUBLE)
#define HAVE_abssi2 1
#define HAVE_abssf2 (TARGET_32BIT && TARGET_HARD_FLOAT)
#define HAVE_absdf2 (TARGET_32BIT && TARGET_HARD_FLOAT && !TARGET_VFP_SINGLE)
#define HAVE_sqrtsf2 (TARGET_32BIT && TARGET_HARD_FLOAT && TARGET_VFP)
#define HAVE_sqrtdf2 (TARGET_32BIT && TARGET_HARD_FLOAT && TARGET_VFP_DOUBLE)
#define HAVE_one_cmplsi2 1
#define HAVE_floatsihf2 1
#define HAVE_floatdihf2 1
#define HAVE_floatsisf2 (TARGET_32BIT && TARGET_HARD_FLOAT)
#define HAVE_floatsidf2 (TARGET_32BIT && TARGET_HARD_FLOAT && !TARGET_VFP_SINGLE)
#define HAVE_fix_trunchfsi2 1
#define HAVE_fix_trunchfdi2 1
#define HAVE_fix_truncsfsi2 (TARGET_32BIT && TARGET_HARD_FLOAT)
#define HAVE_fix_truncdfsi2 (TARGET_32BIT && TARGET_HARD_FLOAT && !TARGET_VFP_SINGLE)
#define HAVE_truncdfsf2 (TARGET_32BIT && TARGET_HARD_FLOAT && !TARGET_VFP_SINGLE)
#define HAVE_truncdfhf2 1
#define HAVE_zero_extendhisi2 1
#define HAVE_zero_extendqisi2 1
#define HAVE_extendhisi2 1
#define HAVE_extendhisi2_mem (TARGET_ARM)
#define HAVE_extendqihi2 (TARGET_ARM)
#define HAVE_extendqisi2 1
#define HAVE_extendsfdf2 (TARGET_32BIT && TARGET_HARD_FLOAT && !TARGET_VFP_SINGLE)
#define HAVE_extendhfdf2 1
#define HAVE_movdi 1
#define HAVE_movsi 1
#define HAVE_calculate_pic_address (flag_pic)
#define HAVE_builtin_setjmp_receiver (flag_pic)
#define HAVE_storehi (TARGET_ARM)
#define HAVE_storehi_bigend (TARGET_ARM)
#define HAVE_storeinthi (TARGET_ARM)
#define HAVE_storehi_single_op (TARGET_32BIT && arm_arch4)
#define HAVE_movhi 1
#define HAVE_movhi_bytes (TARGET_ARM)
#define HAVE_movhi_bigend (TARGET_ARM)
#define HAVE_reload_outhi 1
#define HAVE_reload_inhi 1
#define HAVE_movqi 1
#define HAVE_movhf 1
#define HAVE_movsf 1
#define HAVE_movdf 1
#define HAVE_reload_outdf (TARGET_THUMB2)
#define HAVE_load_multiple (TARGET_32BIT)
#define HAVE_store_multiple (TARGET_32BIT)
#define HAVE_setmemsi (TARGET_32BIT)
#define HAVE_movmemqi 1
#define HAVE_cbranchsi4 1
#define HAVE_cbranchsf4 (TARGET_32BIT && TARGET_HARD_FLOAT)
#define HAVE_cbranchdf4 (TARGET_32BIT && TARGET_HARD_FLOAT && !TARGET_VFP_SINGLE)
#define HAVE_cbranchdi4 (TARGET_32BIT)
#define HAVE_cbranch_cc (TARGET_32BIT)
#define HAVE_cstore_cc (TARGET_32BIT)
#define HAVE_cstoresi4 (TARGET_32BIT || TARGET_THUMB1)
#define HAVE_cstoresf4 (TARGET_32BIT && TARGET_HARD_FLOAT)
#define HAVE_cstoredf4 (TARGET_32BIT && TARGET_HARD_FLOAT && !TARGET_VFP_SINGLE)
#define HAVE_cstoredi4 (TARGET_32BIT)
#define HAVE_movsicc (TARGET_32BIT)
#define HAVE_movsfcc (TARGET_32BIT && TARGET_HARD_FLOAT)
#define HAVE_movdfcc (TARGET_32BIT && TARGET_HARD_FLOAT && TARGET_VFP_DOUBLE)
#define HAVE_jump 1
#define HAVE_call 1
#define HAVE_call_internal 1
#define HAVE_nonsecure_call_internal 1
#define HAVE_call_value 1
#define HAVE_call_value_internal 1
#define HAVE_nonsecure_call_value_internal 1
#define HAVE_sibcall_internal 1
#define HAVE_sibcall (TARGET_32BIT)
#define HAVE_sibcall_value_internal 1
#define HAVE_sibcall_value (TARGET_32BIT)
#define HAVE_return ((TARGET_ARM || (TARGET_THUMB2 \
                   && ARM_FUNC_TYPE (arm_current_func_type ()) == ARM_FT_NORMAL \
                   && !IS_STACKALIGN (arm_current_func_type ()))) \
     && USE_RETURN_INSN (FALSE))
#define HAVE_simple_return ((TARGET_ARM || (TARGET_THUMB2 \
                   && ARM_FUNC_TYPE (arm_current_func_type ()) == ARM_FT_NORMAL \
                   && !IS_STACKALIGN (arm_current_func_type ()))) \
     && use_simple_return_p ())
#define HAVE_return_addr_mask (TARGET_ARM)
#define HAVE_untyped_call 1
#define HAVE_untyped_return 1
#define HAVE_casesi (TARGET_32BIT || optimize_size || flag_pic)
#define HAVE_indirect_jump 1
#define HAVE_prologue 1
#define HAVE_epilogue 1
#define HAVE_sibcall_epilogue (TARGET_32BIT)
#define HAVE_eh_epilogue 1
#define HAVE_ctzsi2 (TARGET_32BIT && arm_arch_thumb2)
#define HAVE_eh_return 1
#define HAVE_get_thread_pointersi 1
#define HAVE_arm_legacy_rev (TARGET_32BIT)
#define HAVE_thumb_legacy_rev (TARGET_THUMB)
#define HAVE_bswapsi2 (TARGET_EITHER && (arm_arch6 || !optimize_size))
#define HAVE_bswaphi2 (arm_arch6)
#define HAVE_copysignsf3 (TARGET_SOFT_FLOAT && arm_arch_thumb2)
#define HAVE_copysigndf3 (TARGET_SOFT_FLOAT && arm_arch_thumb2)
#define HAVE_movv2di (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V2DImode)))
#define HAVE_movv2si (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V2SImode)))
#define HAVE_movv4hi (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V4HImode)))
#define HAVE_movv8qi (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V8QImode)))
#define HAVE_movv2sf (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V2SFmode)))
#define HAVE_movv4si (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V4SImode)))
#define HAVE_movv8hi (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V8HImode)))
#define HAVE_movv16qi (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V16QImode)))
#define HAVE_movv4sf (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V4SFmode)))
#define HAVE_addv2di3 ((TARGET_NEON && ((V2DImode != V2SFmode && V2DImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V2DImode)))
#define HAVE_addv2si3 ((TARGET_NEON && ((V2SImode != V2SFmode && V2SImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V2SImode)))
#define HAVE_addv4hi3 ((TARGET_NEON && ((V4HImode != V2SFmode && V4HImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V4HImode)))
#define HAVE_addv8qi3 ((TARGET_NEON && ((V8QImode != V2SFmode && V8QImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V8QImode)))
#define HAVE_addv2sf3 ((TARGET_NEON && ((V2SFmode != V2SFmode && V2SFmode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V2SFmode)))
#define HAVE_addv4si3 ((TARGET_NEON && ((V4SImode != V2SFmode && V4SImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V4SImode)))
#define HAVE_addv8hi3 ((TARGET_NEON && ((V8HImode != V2SFmode && V8HImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V8HImode)))
#define HAVE_addv16qi3 ((TARGET_NEON && ((V16QImode != V2SFmode && V16QImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V16QImode)))
#define HAVE_addv4sf3 ((TARGET_NEON && ((V4SFmode != V2SFmode && V4SFmode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V4SFmode)))
#define HAVE_subv2di3 ((TARGET_NEON && ((V2DImode != V2SFmode && V2DImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V2DImode)))
#define HAVE_subv2si3 ((TARGET_NEON && ((V2SImode != V2SFmode && V2SImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V2SImode)))
#define HAVE_subv4hi3 ((TARGET_NEON && ((V4HImode != V2SFmode && V4HImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V4HImode)))
#define HAVE_subv8qi3 ((TARGET_NEON && ((V8QImode != V2SFmode && V8QImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V8QImode)))
#define HAVE_subv2sf3 ((TARGET_NEON && ((V2SFmode != V2SFmode && V2SFmode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V2SFmode)))
#define HAVE_subv4si3 ((TARGET_NEON && ((V4SImode != V2SFmode && V4SImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V4SImode)))
#define HAVE_subv8hi3 ((TARGET_NEON && ((V8HImode != V2SFmode && V8HImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V8HImode)))
#define HAVE_subv16qi3 ((TARGET_NEON && ((V16QImode != V2SFmode && V16QImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V16QImode)))
#define HAVE_subv4sf3 ((TARGET_NEON && ((V4SFmode != V2SFmode && V4SFmode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V4SFmode)))
#define HAVE_mulv2si3 ((TARGET_NEON && ((V2SImode != V2SFmode && V2SImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (V2SImode == V4HImode && TARGET_REALLY_IWMMXT))
#define HAVE_mulv4hi3 ((TARGET_NEON && ((V4HImode != V2SFmode && V4HImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (V4HImode == V4HImode && TARGET_REALLY_IWMMXT))
#define HAVE_mulv8qi3 ((TARGET_NEON && ((V8QImode != V2SFmode && V8QImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (V8QImode == V4HImode && TARGET_REALLY_IWMMXT))
#define HAVE_mulv2sf3 ((TARGET_NEON && ((V2SFmode != V2SFmode && V2SFmode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (V2SFmode == V4HImode && TARGET_REALLY_IWMMXT))
#define HAVE_mulv4si3 ((TARGET_NEON && ((V4SImode != V2SFmode && V4SImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (V4SImode == V4HImode && TARGET_REALLY_IWMMXT))
#define HAVE_mulv8hi3 ((TARGET_NEON && ((V8HImode != V2SFmode && V8HImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (V8HImode == V4HImode && TARGET_REALLY_IWMMXT))
#define HAVE_mulv16qi3 ((TARGET_NEON && ((V16QImode != V2SFmode && V16QImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (V16QImode == V4HImode && TARGET_REALLY_IWMMXT))
#define HAVE_mulv4sf3 ((TARGET_NEON && ((V4SFmode != V2SFmode && V4SFmode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (V4SFmode == V4HImode && TARGET_REALLY_IWMMXT))
#define HAVE_sminv2si3 ((TARGET_NEON && ((V2SImode != V2SFmode && V2SImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V2SImode)))
#define HAVE_sminv4hi3 ((TARGET_NEON && ((V4HImode != V2SFmode && V4HImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V4HImode)))
#define HAVE_sminv8qi3 ((TARGET_NEON && ((V8QImode != V2SFmode && V8QImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V8QImode)))
#define HAVE_sminv2sf3 ((TARGET_NEON && ((V2SFmode != V2SFmode && V2SFmode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V2SFmode)))
#define HAVE_sminv4si3 ((TARGET_NEON && ((V4SImode != V2SFmode && V4SImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V4SImode)))
#define HAVE_sminv8hi3 ((TARGET_NEON && ((V8HImode != V2SFmode && V8HImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V8HImode)))
#define HAVE_sminv16qi3 ((TARGET_NEON && ((V16QImode != V2SFmode && V16QImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V16QImode)))
#define HAVE_sminv4sf3 ((TARGET_NEON && ((V4SFmode != V2SFmode && V4SFmode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V4SFmode)))
#define HAVE_uminv2si3 (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V2SImode)))
#define HAVE_uminv4hi3 (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V4HImode)))
#define HAVE_uminv8qi3 (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V8QImode)))
#define HAVE_uminv4si3 (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V4SImode)))
#define HAVE_uminv8hi3 (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V8HImode)))
#define HAVE_uminv16qi3 (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V16QImode)))
#define HAVE_smaxv2si3 ((TARGET_NEON && ((V2SImode != V2SFmode && V2SImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V2SImode)))
#define HAVE_smaxv4hi3 ((TARGET_NEON && ((V4HImode != V2SFmode && V4HImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V4HImode)))
#define HAVE_smaxv8qi3 ((TARGET_NEON && ((V8QImode != V2SFmode && V8QImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V8QImode)))
#define HAVE_smaxv2sf3 ((TARGET_NEON && ((V2SFmode != V2SFmode && V2SFmode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V2SFmode)))
#define HAVE_smaxv4si3 ((TARGET_NEON && ((V4SImode != V2SFmode && V4SImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V4SImode)))
#define HAVE_smaxv8hi3 ((TARGET_NEON && ((V8HImode != V2SFmode && V8HImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V8HImode)))
#define HAVE_smaxv16qi3 ((TARGET_NEON && ((V16QImode != V2SFmode && V16QImode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V16QImode)))
#define HAVE_smaxv4sf3 ((TARGET_NEON && ((V4SFmode != V2SFmode && V4SFmode != V4SFmode) \
		    || flag_unsafe_math_optimizations)) \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V4SFmode)))
#define HAVE_umaxv2si3 (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V2SImode)))
#define HAVE_umaxv4hi3 (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V4HImode)))
#define HAVE_umaxv8qi3 (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V8QImode)))
#define HAVE_umaxv4si3 (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V4SImode)))
#define HAVE_umaxv8hi3 (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V8HImode)))
#define HAVE_umaxv16qi3 (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V16QImode)))
#define HAVE_vec_perm_constv2di (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V2DImode)))
#define HAVE_vec_perm_constv2si (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V2SImode)))
#define HAVE_vec_perm_constv4hi (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V4HImode)))
#define HAVE_vec_perm_constv8qi (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V8QImode)))
#define HAVE_vec_perm_constv2sf (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V2SFmode)))
#define HAVE_vec_perm_constv4si (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V4SImode)))
#define HAVE_vec_perm_constv8hi (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V8HImode)))
#define HAVE_vec_perm_constv16qi (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V16QImode)))
#define HAVE_vec_perm_constv4sf (TARGET_NEON \
   || (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (V4SFmode)))
#define HAVE_vec_permv8qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_permv16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_iwmmxt_setwcgr0 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_setwcgr1 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_setwcgr2 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_setwcgr3 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_getwcgr0 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_getwcgr1 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_getwcgr2 (TARGET_REALLY_IWMMXT)
#define HAVE_iwmmxt_getwcgr3 (TARGET_REALLY_IWMMXT)
#define HAVE_thumb_movhi_clobber (TARGET_THUMB1)
#define HAVE_cbranchqi4 (TARGET_THUMB1)
#define HAVE_cstoresi_eq0_thumb1 (TARGET_THUMB1)
#define HAVE_cstoresi_ne0_thumb1 (TARGET_THUMB1)
#define HAVE_thumb1_casesi_internal_pic (TARGET_THUMB1)
#define HAVE_tablejump (TARGET_THUMB1)
#define HAVE_doloop_end (TARGET_32BIT)
#define HAVE_movti (TARGET_NEON)
#define HAVE_movei (TARGET_NEON)
#define HAVE_movoi (TARGET_NEON)
#define HAVE_movci (TARGET_NEON)
#define HAVE_movxi (TARGET_NEON)
#define HAVE_movmisalignv8qi (TARGET_NEON && !BYTES_BIG_ENDIAN && unaligned_access)
#define HAVE_movmisalignv16qi (TARGET_NEON && !BYTES_BIG_ENDIAN && unaligned_access)
#define HAVE_movmisalignv4hi (TARGET_NEON && !BYTES_BIG_ENDIAN && unaligned_access)
#define HAVE_movmisalignv8hi (TARGET_NEON && !BYTES_BIG_ENDIAN && unaligned_access)
#define HAVE_movmisalignv2si (TARGET_NEON && !BYTES_BIG_ENDIAN && unaligned_access)
#define HAVE_movmisalignv4si (TARGET_NEON && !BYTES_BIG_ENDIAN && unaligned_access)
#define HAVE_movmisalignv2sf (TARGET_NEON && !BYTES_BIG_ENDIAN && unaligned_access)
#define HAVE_movmisalignv4sf (TARGET_NEON && !BYTES_BIG_ENDIAN && unaligned_access)
#define HAVE_movmisaligndi (TARGET_NEON && !BYTES_BIG_ENDIAN && unaligned_access)
#define HAVE_movmisalignv2di (TARGET_NEON && !BYTES_BIG_ENDIAN && unaligned_access)
#define HAVE_vec_setv8qi (TARGET_NEON)
#define HAVE_vec_setv16qi (TARGET_NEON)
#define HAVE_vec_setv4hi (TARGET_NEON)
#define HAVE_vec_setv8hi (TARGET_NEON)
#define HAVE_vec_setv2si (TARGET_NEON)
#define HAVE_vec_setv4si (TARGET_NEON)
#define HAVE_vec_setv2sf (TARGET_NEON)
#define HAVE_vec_setv4sf (TARGET_NEON)
#define HAVE_vec_setv2di (TARGET_NEON)
#define HAVE_vec_initv8qi (TARGET_NEON)
#define HAVE_vec_initv16qi (TARGET_NEON)
#define HAVE_vec_initv4hi (TARGET_NEON)
#define HAVE_vec_initv8hi (TARGET_NEON)
#define HAVE_vec_initv2si (TARGET_NEON)
#define HAVE_vec_initv4si (TARGET_NEON)
#define HAVE_vec_initv2sf (TARGET_NEON)
#define HAVE_vec_initv4sf (TARGET_NEON)
#define HAVE_vec_initv2di (TARGET_NEON)
#define HAVE_vashrv8qi3 (TARGET_NEON)
#define HAVE_vashrv16qi3 (TARGET_NEON)
#define HAVE_vashrv4hi3 (TARGET_NEON)
#define HAVE_vashrv8hi3 (TARGET_NEON)
#define HAVE_vashrv2si3 (TARGET_NEON)
#define HAVE_vashrv4si3 (TARGET_NEON)
#define HAVE_vlshrv8qi3 (TARGET_NEON)
#define HAVE_vlshrv16qi3 (TARGET_NEON)
#define HAVE_vlshrv4hi3 (TARGET_NEON)
#define HAVE_vlshrv8hi3 (TARGET_NEON)
#define HAVE_vlshrv2si3 (TARGET_NEON)
#define HAVE_vlshrv4si3 (TARGET_NEON)
#define HAVE_vec_shr_v8qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_shr_v16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_shr_v4hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_shr_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_shr_v2si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_shr_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_shr_v2sf (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_shr_v4sf (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_shr_v2di (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_shl_v8qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_shl_v16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_shl_v4hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_shl_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_shl_v2si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_shl_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_shl_v2sf (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_shl_v4sf (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_shl_v2di (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_move_hi_quad_v2di (TARGET_NEON)
#define HAVE_move_hi_quad_v2df (TARGET_NEON)
#define HAVE_move_hi_quad_v16qi (TARGET_NEON)
#define HAVE_move_hi_quad_v8hi (TARGET_NEON)
#define HAVE_move_hi_quad_v4si (TARGET_NEON)
#define HAVE_move_hi_quad_v4sf (TARGET_NEON)
#define HAVE_move_lo_quad_v2di (TARGET_NEON)
#define HAVE_move_lo_quad_v2df (TARGET_NEON)
#define HAVE_move_lo_quad_v16qi (TARGET_NEON)
#define HAVE_move_lo_quad_v8hi (TARGET_NEON)
#define HAVE_move_lo_quad_v4si (TARGET_NEON)
#define HAVE_move_lo_quad_v4sf (TARGET_NEON)
#define HAVE_reduc_plus_scal_v8qi (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_reduc_plus_scal_v4hi (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_reduc_plus_scal_v2si (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_reduc_plus_scal_v2sf (TARGET_NEON && (!true || flag_unsafe_math_optimizations))
#define HAVE_reduc_plus_scal_v16qi (TARGET_NEON && (!false || flag_unsafe_math_optimizations) \
   && !BYTES_BIG_ENDIAN)
#define HAVE_reduc_plus_scal_v8hi (TARGET_NEON && (!false || flag_unsafe_math_optimizations) \
   && !BYTES_BIG_ENDIAN)
#define HAVE_reduc_plus_scal_v4si (TARGET_NEON && (!false || flag_unsafe_math_optimizations) \
   && !BYTES_BIG_ENDIAN)
#define HAVE_reduc_plus_scal_v4sf (TARGET_NEON && (!true || flag_unsafe_math_optimizations) \
   && !BYTES_BIG_ENDIAN)
#define HAVE_reduc_plus_scal_v2di (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_reduc_smin_scal_v8qi (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_reduc_smin_scal_v4hi (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_reduc_smin_scal_v2si (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_reduc_smin_scal_v2sf (TARGET_NEON && (!true || flag_unsafe_math_optimizations))
#define HAVE_reduc_smin_scal_v16qi (TARGET_NEON && (!false || flag_unsafe_math_optimizations) \
   && !BYTES_BIG_ENDIAN)
#define HAVE_reduc_smin_scal_v8hi (TARGET_NEON && (!false || flag_unsafe_math_optimizations) \
   && !BYTES_BIG_ENDIAN)
#define HAVE_reduc_smin_scal_v4si (TARGET_NEON && (!false || flag_unsafe_math_optimizations) \
   && !BYTES_BIG_ENDIAN)
#define HAVE_reduc_smin_scal_v4sf (TARGET_NEON && (!true || flag_unsafe_math_optimizations) \
   && !BYTES_BIG_ENDIAN)
#define HAVE_reduc_smax_scal_v8qi (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_reduc_smax_scal_v4hi (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_reduc_smax_scal_v2si (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_reduc_smax_scal_v2sf (TARGET_NEON && (!true || flag_unsafe_math_optimizations))
#define HAVE_reduc_smax_scal_v16qi (TARGET_NEON && (!false || flag_unsafe_math_optimizations) \
   && !BYTES_BIG_ENDIAN)
#define HAVE_reduc_smax_scal_v8hi (TARGET_NEON && (!false || flag_unsafe_math_optimizations) \
   && !BYTES_BIG_ENDIAN)
#define HAVE_reduc_smax_scal_v4si (TARGET_NEON && (!false || flag_unsafe_math_optimizations) \
   && !BYTES_BIG_ENDIAN)
#define HAVE_reduc_smax_scal_v4sf (TARGET_NEON && (!true || flag_unsafe_math_optimizations) \
   && !BYTES_BIG_ENDIAN)
#define HAVE_reduc_umin_scal_v8qi (TARGET_NEON)
#define HAVE_reduc_umin_scal_v4hi (TARGET_NEON)
#define HAVE_reduc_umin_scal_v2si (TARGET_NEON)
#define HAVE_reduc_umin_scal_v16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_reduc_umin_scal_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_reduc_umin_scal_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_reduc_umax_scal_v8qi (TARGET_NEON)
#define HAVE_reduc_umax_scal_v4hi (TARGET_NEON)
#define HAVE_reduc_umax_scal_v2si (TARGET_NEON)
#define HAVE_reduc_umax_scal_v16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_reduc_umax_scal_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_reduc_umax_scal_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vcondv8qiv8qi (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_vcondv16qiv16qi (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_vcondv4hiv4hi (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_vcondv8hiv8hi (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_vcondv2siv2si (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_vcondv4siv4si (TARGET_NEON && (!false || flag_unsafe_math_optimizations))
#define HAVE_vcondv2sfv2sf (TARGET_NEON && (!true || flag_unsafe_math_optimizations))
#define HAVE_vcondv4sfv4sf (TARGET_NEON && (!true || flag_unsafe_math_optimizations))
#define HAVE_vconduv8qiv8qi (TARGET_NEON)
#define HAVE_vconduv16qiv16qi (TARGET_NEON)
#define HAVE_vconduv4hiv4hi (TARGET_NEON)
#define HAVE_vconduv8hiv8hi (TARGET_NEON)
#define HAVE_vconduv2siv2si (TARGET_NEON)
#define HAVE_vconduv4siv4si (TARGET_NEON)
#define HAVE_neon_vaddv2sf (TARGET_NEON)
#define HAVE_neon_vaddv4sf (TARGET_NEON)
#define HAVE_neon_vmlav8qi (TARGET_NEON)
#define HAVE_neon_vmlav16qi (TARGET_NEON)
#define HAVE_neon_vmlav4hi (TARGET_NEON)
#define HAVE_neon_vmlav8hi (TARGET_NEON)
#define HAVE_neon_vmlav2si (TARGET_NEON)
#define HAVE_neon_vmlav4si (TARGET_NEON)
#define HAVE_neon_vmlav2sf (TARGET_NEON)
#define HAVE_neon_vmlav4sf (TARGET_NEON)
#define HAVE_neon_vfmav2sf (TARGET_NEON && TARGET_FMA)
#define HAVE_neon_vfmav4sf (TARGET_NEON && TARGET_FMA)
#define HAVE_neon_vfmsv2sf (TARGET_NEON && TARGET_FMA)
#define HAVE_neon_vfmsv4sf (TARGET_NEON && TARGET_FMA)
#define HAVE_neon_vmlsv8qi (TARGET_NEON)
#define HAVE_neon_vmlsv16qi (TARGET_NEON)
#define HAVE_neon_vmlsv4hi (TARGET_NEON)
#define HAVE_neon_vmlsv8hi (TARGET_NEON)
#define HAVE_neon_vmlsv2si (TARGET_NEON)
#define HAVE_neon_vmlsv4si (TARGET_NEON)
#define HAVE_neon_vmlsv2sf (TARGET_NEON)
#define HAVE_neon_vmlsv4sf (TARGET_NEON)
#define HAVE_neon_vsubv2sf (TARGET_NEON)
#define HAVE_neon_vsubv4sf (TARGET_NEON)
#define HAVE_neon_vpaddv8qi (TARGET_NEON)
#define HAVE_neon_vpaddv4hi (TARGET_NEON)
#define HAVE_neon_vpaddv2si (TARGET_NEON)
#define HAVE_neon_vpaddv2sf (TARGET_NEON)
#define HAVE_neon_vabsv8qi (TARGET_NEON)
#define HAVE_neon_vabsv16qi (TARGET_NEON)
#define HAVE_neon_vabsv4hi (TARGET_NEON)
#define HAVE_neon_vabsv8hi (TARGET_NEON)
#define HAVE_neon_vabsv2si (TARGET_NEON)
#define HAVE_neon_vabsv4si (TARGET_NEON)
#define HAVE_neon_vabsv2sf (TARGET_NEON)
#define HAVE_neon_vabsv4sf (TARGET_NEON)
#define HAVE_neon_vnegv8qi (TARGET_NEON)
#define HAVE_neon_vnegv16qi (TARGET_NEON)
#define HAVE_neon_vnegv4hi (TARGET_NEON)
#define HAVE_neon_vnegv8hi (TARGET_NEON)
#define HAVE_neon_vnegv2si (TARGET_NEON)
#define HAVE_neon_vnegv4si (TARGET_NEON)
#define HAVE_neon_vnegv2sf (TARGET_NEON)
#define HAVE_neon_vnegv4sf (TARGET_NEON)
#define HAVE_neon_copysignfv2sf (TARGET_NEON)
#define HAVE_neon_copysignfv4sf (TARGET_NEON)
#define HAVE_neon_vclzv8qi (TARGET_NEON)
#define HAVE_neon_vclzv16qi (TARGET_NEON)
#define HAVE_neon_vclzv4hi (TARGET_NEON)
#define HAVE_neon_vclzv8hi (TARGET_NEON)
#define HAVE_neon_vclzv2si (TARGET_NEON)
#define HAVE_neon_vclzv4si (TARGET_NEON)
#define HAVE_neon_vcntv8qi (TARGET_NEON)
#define HAVE_neon_vcntv16qi (TARGET_NEON)
#define HAVE_neon_vmvnv8qi (TARGET_NEON)
#define HAVE_neon_vmvnv16qi (TARGET_NEON)
#define HAVE_neon_vmvnv4hi (TARGET_NEON)
#define HAVE_neon_vmvnv8hi (TARGET_NEON)
#define HAVE_neon_vmvnv2si (TARGET_NEON)
#define HAVE_neon_vmvnv4si (TARGET_NEON)
#define HAVE_neon_vget_lanev8qi (TARGET_NEON)
#define HAVE_neon_vget_lanev16qi (TARGET_NEON)
#define HAVE_neon_vget_lanev4hi (TARGET_NEON)
#define HAVE_neon_vget_lanev8hi (TARGET_NEON)
#define HAVE_neon_vget_lanev2si (TARGET_NEON)
#define HAVE_neon_vget_lanev4si (TARGET_NEON)
#define HAVE_neon_vget_lanev2sf (TARGET_NEON)
#define HAVE_neon_vget_lanev4sf (TARGET_NEON)
#define HAVE_neon_vget_laneuv8qi (TARGET_NEON)
#define HAVE_neon_vget_laneuv16qi (TARGET_NEON)
#define HAVE_neon_vget_laneuv4hi (TARGET_NEON)
#define HAVE_neon_vget_laneuv8hi (TARGET_NEON)
#define HAVE_neon_vget_laneuv2si (TARGET_NEON)
#define HAVE_neon_vget_laneuv4si (TARGET_NEON)
#define HAVE_neon_vget_lanedi (TARGET_NEON)
#define HAVE_neon_vget_lanev2di (TARGET_NEON)
#define HAVE_neon_vset_lanev8qi (TARGET_NEON)
#define HAVE_neon_vset_lanev16qi (TARGET_NEON)
#define HAVE_neon_vset_lanev4hi (TARGET_NEON)
#define HAVE_neon_vset_lanev8hi (TARGET_NEON)
#define HAVE_neon_vset_lanev2si (TARGET_NEON)
#define HAVE_neon_vset_lanev4si (TARGET_NEON)
#define HAVE_neon_vset_lanev2sf (TARGET_NEON)
#define HAVE_neon_vset_lanev4sf (TARGET_NEON)
#define HAVE_neon_vset_lanev2di (TARGET_NEON)
#define HAVE_neon_vset_lanedi (TARGET_NEON)
#define HAVE_neon_vcreatev8qi (TARGET_NEON)
#define HAVE_neon_vcreatev4hi (TARGET_NEON)
#define HAVE_neon_vcreatev2si (TARGET_NEON)
#define HAVE_neon_vcreatev2sf (TARGET_NEON)
#define HAVE_neon_vcreatedi (TARGET_NEON)
#define HAVE_neon_vdup_ndi (TARGET_NEON)
#define HAVE_neon_vdup_lanev8qi (TARGET_NEON)
#define HAVE_neon_vdup_lanev16qi (TARGET_NEON)
#define HAVE_neon_vdup_lanev4hi (TARGET_NEON)
#define HAVE_neon_vdup_lanev8hi (TARGET_NEON)
#define HAVE_neon_vdup_lanev2si (TARGET_NEON)
#define HAVE_neon_vdup_lanev4si (TARGET_NEON)
#define HAVE_neon_vdup_lanev2sf (TARGET_NEON)
#define HAVE_neon_vdup_lanev4sf (TARGET_NEON)
#define HAVE_neon_vdup_lanedi (TARGET_NEON)
#define HAVE_neon_vdup_lanev2di (TARGET_NEON)
#define HAVE_neon_vget_highv16qi (TARGET_NEON)
#define HAVE_neon_vget_highv8hi (TARGET_NEON)
#define HAVE_neon_vget_highv4si (TARGET_NEON)
#define HAVE_neon_vget_highv4sf (TARGET_NEON)
#define HAVE_neon_vget_highv2di (TARGET_NEON)
#define HAVE_neon_vget_lowv16qi (TARGET_NEON)
#define HAVE_neon_vget_lowv8hi (TARGET_NEON)
#define HAVE_neon_vget_lowv4si (TARGET_NEON)
#define HAVE_neon_vget_lowv4sf (TARGET_NEON)
#define HAVE_neon_vget_lowv2di (TARGET_NEON)
#define HAVE_neon_vmul_nv4hi (TARGET_NEON)
#define HAVE_neon_vmul_nv2si (TARGET_NEON)
#define HAVE_neon_vmul_nv2sf (TARGET_NEON)
#define HAVE_neon_vmul_nv8hi (TARGET_NEON)
#define HAVE_neon_vmul_nv4si (TARGET_NEON)
#define HAVE_neon_vmul_nv4sf (TARGET_NEON)
#define HAVE_neon_vmulls_nv4hi (TARGET_NEON)
#define HAVE_neon_vmulls_nv2si (TARGET_NEON)
#define HAVE_neon_vmullu_nv4hi (TARGET_NEON)
#define HAVE_neon_vmullu_nv2si (TARGET_NEON)
#define HAVE_neon_vqdmull_nv4hi (TARGET_NEON)
#define HAVE_neon_vqdmull_nv2si (TARGET_NEON)
#define HAVE_neon_vqdmulh_nv4hi (TARGET_NEON)
#define HAVE_neon_vqdmulh_nv2si (TARGET_NEON)
#define HAVE_neon_vqrdmulh_nv4hi (TARGET_NEON)
#define HAVE_neon_vqrdmulh_nv2si (TARGET_NEON)
#define HAVE_neon_vqdmulh_nv8hi (TARGET_NEON)
#define HAVE_neon_vqdmulh_nv4si (TARGET_NEON)
#define HAVE_neon_vqrdmulh_nv8hi (TARGET_NEON)
#define HAVE_neon_vqrdmulh_nv4si (TARGET_NEON)
#define HAVE_neon_vmla_nv4hi (TARGET_NEON)
#define HAVE_neon_vmla_nv2si (TARGET_NEON)
#define HAVE_neon_vmla_nv2sf (TARGET_NEON)
#define HAVE_neon_vmla_nv8hi (TARGET_NEON)
#define HAVE_neon_vmla_nv4si (TARGET_NEON)
#define HAVE_neon_vmla_nv4sf (TARGET_NEON)
#define HAVE_neon_vmlals_nv4hi (TARGET_NEON)
#define HAVE_neon_vmlals_nv2si (TARGET_NEON)
#define HAVE_neon_vmlalu_nv4hi (TARGET_NEON)
#define HAVE_neon_vmlalu_nv2si (TARGET_NEON)
#define HAVE_neon_vqdmlal_nv4hi (TARGET_NEON)
#define HAVE_neon_vqdmlal_nv2si (TARGET_NEON)
#define HAVE_neon_vmls_nv4hi (TARGET_NEON)
#define HAVE_neon_vmls_nv2si (TARGET_NEON)
#define HAVE_neon_vmls_nv2sf (TARGET_NEON)
#define HAVE_neon_vmls_nv8hi (TARGET_NEON)
#define HAVE_neon_vmls_nv4si (TARGET_NEON)
#define HAVE_neon_vmls_nv4sf (TARGET_NEON)
#define HAVE_neon_vmlsls_nv4hi (TARGET_NEON)
#define HAVE_neon_vmlsls_nv2si (TARGET_NEON)
#define HAVE_neon_vmlslu_nv4hi (TARGET_NEON)
#define HAVE_neon_vmlslu_nv2si (TARGET_NEON)
#define HAVE_neon_vqdmlsl_nv4hi (TARGET_NEON)
#define HAVE_neon_vqdmlsl_nv2si (TARGET_NEON)
#define HAVE_neon_vbslv8qi (TARGET_NEON)
#define HAVE_neon_vbslv16qi (TARGET_NEON)
#define HAVE_neon_vbslv4hi (TARGET_NEON)
#define HAVE_neon_vbslv8hi (TARGET_NEON)
#define HAVE_neon_vbslv2si (TARGET_NEON)
#define HAVE_neon_vbslv4si (TARGET_NEON)
#define HAVE_neon_vbslv2sf (TARGET_NEON)
#define HAVE_neon_vbslv4sf (TARGET_NEON)
#define HAVE_neon_vbsldi (TARGET_NEON)
#define HAVE_neon_vbslv2di (TARGET_NEON)
#define HAVE_neon_vtrnv8qi_internal (TARGET_NEON)
#define HAVE_neon_vtrnv16qi_internal (TARGET_NEON)
#define HAVE_neon_vtrnv4hi_internal (TARGET_NEON)
#define HAVE_neon_vtrnv8hi_internal (TARGET_NEON)
#define HAVE_neon_vtrnv2si_internal (TARGET_NEON)
#define HAVE_neon_vtrnv4si_internal (TARGET_NEON)
#define HAVE_neon_vtrnv2sf_internal (TARGET_NEON)
#define HAVE_neon_vtrnv4sf_internal (TARGET_NEON)
#define HAVE_neon_vzipv8qi_internal (TARGET_NEON)
#define HAVE_neon_vzipv16qi_internal (TARGET_NEON)
#define HAVE_neon_vzipv4hi_internal (TARGET_NEON)
#define HAVE_neon_vzipv8hi_internal (TARGET_NEON)
#define HAVE_neon_vzipv2si_internal (TARGET_NEON)
#define HAVE_neon_vzipv4si_internal (TARGET_NEON)
#define HAVE_neon_vzipv2sf_internal (TARGET_NEON)
#define HAVE_neon_vzipv4sf_internal (TARGET_NEON)
#define HAVE_neon_vuzpv8qi_internal (TARGET_NEON)
#define HAVE_neon_vuzpv16qi_internal (TARGET_NEON)
#define HAVE_neon_vuzpv4hi_internal (TARGET_NEON)
#define HAVE_neon_vuzpv8hi_internal (TARGET_NEON)
#define HAVE_neon_vuzpv2si_internal (TARGET_NEON)
#define HAVE_neon_vuzpv4si_internal (TARGET_NEON)
#define HAVE_neon_vuzpv2sf_internal (TARGET_NEON)
#define HAVE_neon_vuzpv4sf_internal (TARGET_NEON)
#define HAVE_neon_vreinterpretv8qiv8qi (TARGET_NEON)
#define HAVE_neon_vreinterpretv8qiv4hi (TARGET_NEON)
#define HAVE_neon_vreinterpretv8qiv2si (TARGET_NEON)
#define HAVE_neon_vreinterpretv8qiv2sf (TARGET_NEON)
#define HAVE_neon_vreinterpretv8qidi (TARGET_NEON)
#define HAVE_neon_vreinterpretv4hiv8qi (TARGET_NEON)
#define HAVE_neon_vreinterpretv4hiv4hi (TARGET_NEON)
#define HAVE_neon_vreinterpretv4hiv2si (TARGET_NEON)
#define HAVE_neon_vreinterpretv4hiv2sf (TARGET_NEON)
#define HAVE_neon_vreinterpretv4hidi (TARGET_NEON)
#define HAVE_neon_vreinterpretv2siv8qi (TARGET_NEON)
#define HAVE_neon_vreinterpretv2siv4hi (TARGET_NEON)
#define HAVE_neon_vreinterpretv2siv2si (TARGET_NEON)
#define HAVE_neon_vreinterpretv2siv2sf (TARGET_NEON)
#define HAVE_neon_vreinterpretv2sidi (TARGET_NEON)
#define HAVE_neon_vreinterpretv2sfv8qi (TARGET_NEON)
#define HAVE_neon_vreinterpretv2sfv4hi (TARGET_NEON)
#define HAVE_neon_vreinterpretv2sfv2si (TARGET_NEON)
#define HAVE_neon_vreinterpretv2sfv2sf (TARGET_NEON)
#define HAVE_neon_vreinterpretv2sfdi (TARGET_NEON)
#define HAVE_neon_vreinterpretdiv8qi (TARGET_NEON)
#define HAVE_neon_vreinterpretdiv4hi (TARGET_NEON)
#define HAVE_neon_vreinterpretdiv2si (TARGET_NEON)
#define HAVE_neon_vreinterpretdiv2sf (TARGET_NEON)
#define HAVE_neon_vreinterpretdidi (TARGET_NEON)
#define HAVE_neon_vreinterprettiv16qi (TARGET_NEON)
#define HAVE_neon_vreinterprettiv8hi (TARGET_NEON)
#define HAVE_neon_vreinterprettiv4si (TARGET_NEON)
#define HAVE_neon_vreinterprettiv4sf (TARGET_NEON)
#define HAVE_neon_vreinterprettiv2di (TARGET_NEON)
#define HAVE_neon_vreinterprettiti (TARGET_NEON)
#define HAVE_neon_vreinterpretv16qiv16qi (TARGET_NEON)
#define HAVE_neon_vreinterpretv16qiv8hi (TARGET_NEON)
#define HAVE_neon_vreinterpretv16qiv4si (TARGET_NEON)
#define HAVE_neon_vreinterpretv16qiv4sf (TARGET_NEON)
#define HAVE_neon_vreinterpretv16qiv2di (TARGET_NEON)
#define HAVE_neon_vreinterpretv16qiti (TARGET_NEON)
#define HAVE_neon_vreinterpretv8hiv16qi (TARGET_NEON)
#define HAVE_neon_vreinterpretv8hiv8hi (TARGET_NEON)
#define HAVE_neon_vreinterpretv8hiv4si (TARGET_NEON)
#define HAVE_neon_vreinterpretv8hiv4sf (TARGET_NEON)
#define HAVE_neon_vreinterpretv8hiv2di (TARGET_NEON)
#define HAVE_neon_vreinterpretv8hiti (TARGET_NEON)
#define HAVE_neon_vreinterpretv4siv16qi (TARGET_NEON)
#define HAVE_neon_vreinterpretv4siv8hi (TARGET_NEON)
#define HAVE_neon_vreinterpretv4siv4si (TARGET_NEON)
#define HAVE_neon_vreinterpretv4siv4sf (TARGET_NEON)
#define HAVE_neon_vreinterpretv4siv2di (TARGET_NEON)
#define HAVE_neon_vreinterpretv4siti (TARGET_NEON)
#define HAVE_neon_vreinterpretv4sfv16qi (TARGET_NEON)
#define HAVE_neon_vreinterpretv4sfv8hi (TARGET_NEON)
#define HAVE_neon_vreinterpretv4sfv4si (TARGET_NEON)
#define HAVE_neon_vreinterpretv4sfv4sf (TARGET_NEON)
#define HAVE_neon_vreinterpretv4sfv2di (TARGET_NEON)
#define HAVE_neon_vreinterpretv4sfti (TARGET_NEON)
#define HAVE_neon_vreinterpretv2div16qi (TARGET_NEON)
#define HAVE_neon_vreinterpretv2div8hi (TARGET_NEON)
#define HAVE_neon_vreinterpretv2div4si (TARGET_NEON)
#define HAVE_neon_vreinterpretv2div4sf (TARGET_NEON)
#define HAVE_neon_vreinterpretv2div2di (TARGET_NEON)
#define HAVE_neon_vreinterpretv2diti (TARGET_NEON)
#define HAVE_vec_load_lanesv8qiv8qi (TARGET_NEON)
#define HAVE_vec_load_lanesv16qiv16qi (TARGET_NEON)
#define HAVE_vec_load_lanesv4hiv4hi (TARGET_NEON)
#define HAVE_vec_load_lanesv8hiv8hi (TARGET_NEON)
#define HAVE_vec_load_lanesv2siv2si (TARGET_NEON)
#define HAVE_vec_load_lanesv4siv4si (TARGET_NEON)
#define HAVE_vec_load_lanesv2sfv2sf (TARGET_NEON)
#define HAVE_vec_load_lanesv4sfv4sf (TARGET_NEON)
#define HAVE_vec_load_lanesdidi (TARGET_NEON)
#define HAVE_vec_load_lanesv2div2di (TARGET_NEON)
#define HAVE_neon_vld1_dupdi (TARGET_NEON)
#define HAVE_vec_store_lanesv8qiv8qi (TARGET_NEON)
#define HAVE_vec_store_lanesv16qiv16qi (TARGET_NEON)
#define HAVE_vec_store_lanesv4hiv4hi (TARGET_NEON)
#define HAVE_vec_store_lanesv8hiv8hi (TARGET_NEON)
#define HAVE_vec_store_lanesv2siv2si (TARGET_NEON)
#define HAVE_vec_store_lanesv4siv4si (TARGET_NEON)
#define HAVE_vec_store_lanesv2sfv2sf (TARGET_NEON)
#define HAVE_vec_store_lanesv4sfv4sf (TARGET_NEON)
#define HAVE_vec_store_lanesdidi (TARGET_NEON)
#define HAVE_vec_store_lanesv2div2di (TARGET_NEON)
#define HAVE_vec_load_lanestiv8qi (TARGET_NEON)
#define HAVE_vec_load_lanestiv4hi (TARGET_NEON)
#define HAVE_vec_load_lanestiv2si (TARGET_NEON)
#define HAVE_vec_load_lanestiv2sf (TARGET_NEON)
#define HAVE_vec_load_lanestidi (TARGET_NEON)
#define HAVE_vec_load_lanesoiv16qi (TARGET_NEON)
#define HAVE_vec_load_lanesoiv8hi (TARGET_NEON)
#define HAVE_vec_load_lanesoiv4si (TARGET_NEON)
#define HAVE_vec_load_lanesoiv4sf (TARGET_NEON)
#define HAVE_vec_store_lanestiv8qi (TARGET_NEON)
#define HAVE_vec_store_lanestiv4hi (TARGET_NEON)
#define HAVE_vec_store_lanestiv2si (TARGET_NEON)
#define HAVE_vec_store_lanestiv2sf (TARGET_NEON)
#define HAVE_vec_store_lanestidi (TARGET_NEON)
#define HAVE_vec_store_lanesoiv16qi (TARGET_NEON)
#define HAVE_vec_store_lanesoiv8hi (TARGET_NEON)
#define HAVE_vec_store_lanesoiv4si (TARGET_NEON)
#define HAVE_vec_store_lanesoiv4sf (TARGET_NEON)
#define HAVE_vec_load_laneseiv8qi (TARGET_NEON)
#define HAVE_vec_load_laneseiv4hi (TARGET_NEON)
#define HAVE_vec_load_laneseiv2si (TARGET_NEON)
#define HAVE_vec_load_laneseiv2sf (TARGET_NEON)
#define HAVE_vec_load_laneseidi (TARGET_NEON)
#define HAVE_vec_load_lanesciv16qi (TARGET_NEON)
#define HAVE_vec_load_lanesciv8hi (TARGET_NEON)
#define HAVE_vec_load_lanesciv4si (TARGET_NEON)
#define HAVE_vec_load_lanesciv4sf (TARGET_NEON)
#define HAVE_neon_vld3v16qi (TARGET_NEON)
#define HAVE_neon_vld3v8hi (TARGET_NEON)
#define HAVE_neon_vld3v4si (TARGET_NEON)
#define HAVE_neon_vld3v4sf (TARGET_NEON)
#define HAVE_vec_store_laneseiv8qi (TARGET_NEON)
#define HAVE_vec_store_laneseiv4hi (TARGET_NEON)
#define HAVE_vec_store_laneseiv2si (TARGET_NEON)
#define HAVE_vec_store_laneseiv2sf (TARGET_NEON)
#define HAVE_vec_store_laneseidi (TARGET_NEON)
#define HAVE_vec_store_lanesciv16qi (TARGET_NEON)
#define HAVE_vec_store_lanesciv8hi (TARGET_NEON)
#define HAVE_vec_store_lanesciv4si (TARGET_NEON)
#define HAVE_vec_store_lanesciv4sf (TARGET_NEON)
#define HAVE_neon_vst3v16qi (TARGET_NEON)
#define HAVE_neon_vst3v8hi (TARGET_NEON)
#define HAVE_neon_vst3v4si (TARGET_NEON)
#define HAVE_neon_vst3v4sf (TARGET_NEON)
#define HAVE_vec_load_lanesoiv8qi (TARGET_NEON)
#define HAVE_vec_load_lanesoiv4hi (TARGET_NEON)
#define HAVE_vec_load_lanesoiv2si (TARGET_NEON)
#define HAVE_vec_load_lanesoiv2sf (TARGET_NEON)
#define HAVE_vec_load_lanesoidi (TARGET_NEON)
#define HAVE_vec_load_lanesxiv16qi (TARGET_NEON)
#define HAVE_vec_load_lanesxiv8hi (TARGET_NEON)
#define HAVE_vec_load_lanesxiv4si (TARGET_NEON)
#define HAVE_vec_load_lanesxiv4sf (TARGET_NEON)
#define HAVE_neon_vld4v16qi (TARGET_NEON)
#define HAVE_neon_vld4v8hi (TARGET_NEON)
#define HAVE_neon_vld4v4si (TARGET_NEON)
#define HAVE_neon_vld4v4sf (TARGET_NEON)
#define HAVE_vec_store_lanesoiv8qi (TARGET_NEON)
#define HAVE_vec_store_lanesoiv4hi (TARGET_NEON)
#define HAVE_vec_store_lanesoiv2si (TARGET_NEON)
#define HAVE_vec_store_lanesoiv2sf (TARGET_NEON)
#define HAVE_vec_store_lanesoidi (TARGET_NEON)
#define HAVE_vec_store_lanesxiv16qi (TARGET_NEON)
#define HAVE_vec_store_lanesxiv8hi (TARGET_NEON)
#define HAVE_vec_store_lanesxiv4si (TARGET_NEON)
#define HAVE_vec_store_lanesxiv4sf (TARGET_NEON)
#define HAVE_neon_vst4v16qi (TARGET_NEON)
#define HAVE_neon_vst4v8hi (TARGET_NEON)
#define HAVE_neon_vst4v4si (TARGET_NEON)
#define HAVE_neon_vst4v4sf (TARGET_NEON)
#define HAVE_vec_unpacks_hi_v16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_unpacku_hi_v16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_unpacks_hi_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_unpacku_hi_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_unpacks_hi_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_unpacku_hi_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_unpacks_lo_v16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_unpacku_lo_v16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_unpacks_lo_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_unpacku_lo_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_unpacks_lo_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_unpacku_lo_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_widen_smult_lo_v16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_widen_umult_lo_v16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_widen_smult_lo_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_widen_umult_lo_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_widen_smult_lo_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_widen_umult_lo_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_widen_smult_hi_v16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_widen_umult_hi_v16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_widen_smult_hi_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_widen_umult_hi_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_widen_smult_hi_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_widen_umult_hi_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_widen_sshiftl_lo_v16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_widen_ushiftl_lo_v16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_widen_sshiftl_lo_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_widen_ushiftl_lo_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_widen_sshiftl_lo_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_widen_ushiftl_lo_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_widen_sshiftl_hi_v16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_widen_ushiftl_hi_v16qi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_widen_sshiftl_hi_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_widen_ushiftl_hi_v8hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_widen_sshiftl_hi_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_widen_ushiftl_hi_v4si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_unpacks_lo_v8qi (TARGET_NEON)
#define HAVE_vec_unpacku_lo_v8qi (TARGET_NEON)
#define HAVE_vec_unpacks_lo_v4hi (TARGET_NEON)
#define HAVE_vec_unpacku_lo_v4hi (TARGET_NEON)
#define HAVE_vec_unpacks_lo_v2si (TARGET_NEON)
#define HAVE_vec_unpacku_lo_v2si (TARGET_NEON)
#define HAVE_vec_unpacks_hi_v8qi (TARGET_NEON)
#define HAVE_vec_unpacku_hi_v8qi (TARGET_NEON)
#define HAVE_vec_unpacks_hi_v4hi (TARGET_NEON)
#define HAVE_vec_unpacku_hi_v4hi (TARGET_NEON)
#define HAVE_vec_unpacks_hi_v2si (TARGET_NEON)
#define HAVE_vec_unpacku_hi_v2si (TARGET_NEON)
#define HAVE_vec_widen_smult_hi_v8qi (TARGET_NEON)
#define HAVE_vec_widen_umult_hi_v8qi (TARGET_NEON)
#define HAVE_vec_widen_smult_hi_v4hi (TARGET_NEON)
#define HAVE_vec_widen_umult_hi_v4hi (TARGET_NEON)
#define HAVE_vec_widen_smult_hi_v2si (TARGET_NEON)
#define HAVE_vec_widen_umult_hi_v2si (TARGET_NEON)
#define HAVE_vec_widen_smult_lo_v8qi (TARGET_NEON)
#define HAVE_vec_widen_umult_lo_v8qi (TARGET_NEON)
#define HAVE_vec_widen_smult_lo_v4hi (TARGET_NEON)
#define HAVE_vec_widen_umult_lo_v4hi (TARGET_NEON)
#define HAVE_vec_widen_smult_lo_v2si (TARGET_NEON)
#define HAVE_vec_widen_umult_lo_v2si (TARGET_NEON)
#define HAVE_vec_widen_sshiftl_hi_v8qi (TARGET_NEON)
#define HAVE_vec_widen_ushiftl_hi_v8qi (TARGET_NEON)
#define HAVE_vec_widen_sshiftl_hi_v4hi (TARGET_NEON)
#define HAVE_vec_widen_ushiftl_hi_v4hi (TARGET_NEON)
#define HAVE_vec_widen_sshiftl_hi_v2si (TARGET_NEON)
#define HAVE_vec_widen_ushiftl_hi_v2si (TARGET_NEON)
#define HAVE_vec_widen_sshiftl_lo_v8qi (TARGET_NEON)
#define HAVE_vec_widen_ushiftl_lo_v8qi (TARGET_NEON)
#define HAVE_vec_widen_sshiftl_lo_v4hi (TARGET_NEON)
#define HAVE_vec_widen_ushiftl_lo_v4hi (TARGET_NEON)
#define HAVE_vec_widen_sshiftl_lo_v2si (TARGET_NEON)
#define HAVE_vec_widen_ushiftl_lo_v2si (TARGET_NEON)
#define HAVE_vec_pack_trunc_v4hi (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_pack_trunc_v2si (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_vec_pack_trunc_di (TARGET_NEON && !BYTES_BIG_ENDIAN)
#define HAVE_memory_barrier (TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_loaddi ((TARGET_HAVE_LDREXD || TARGET_HAVE_LPAE || TARGET_HAVE_LDACQ) \
   && ARM_DOUBLEWORD_ALIGN)
#define HAVE_atomic_compare_and_swapqi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_compare_and_swaphi (TARGET_HAVE_LDREXBH && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_compare_and_swapsi (TARGET_HAVE_LDREX && TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_atomic_compare_and_swapdi (TARGET_HAVE_LDREXD && ARM_DOUBLEWORD_ALIGN \
	&& TARGET_HAVE_MEMORY_BARRIER)
#define HAVE_mulqq3 (TARGET_DSP_MULTIPLY && arm_arch_thumb2)
#define HAVE_mulhq3 (TARGET_DSP_MULTIPLY && arm_arch_thumb2)
#define HAVE_mulsq3 (TARGET_32BIT && arm_arch3m)
#define HAVE_mulsa3 (TARGET_32BIT && arm_arch3m)
#define HAVE_mulusa3 (TARGET_32BIT && arm_arch3m)
#define HAVE_mulha3 (TARGET_DSP_MULTIPLY && arm_arch_thumb2)
#define HAVE_muluha3 (TARGET_DSP_MULTIPLY)
#define HAVE_ssmulha3 (TARGET_32BIT && TARGET_DSP_MULTIPLY && arm_arch6)
#define HAVE_usmuluha3 (TARGET_INT_SIMD)
extern rtx        gen_addsi3_compare0                   (rtx, rtx, rtx);
extern rtx        gen_cmpsi2_addneg                     (rtx, rtx, rtx, rtx);
extern rtx        gen_subsi3_compare                    (rtx, rtx, rtx);
extern rtx        gen_mulhisi3                          (rtx, rtx, rtx);
extern rtx        gen_maddhisi4                         (rtx, rtx, rtx, rtx);
extern rtx        gen_maddhidi4                         (rtx, rtx, rtx, rtx);
extern rtx        gen_insv_zero                         (rtx, rtx, rtx);
extern rtx        gen_insv_t2                           (rtx, rtx, rtx, rtx);
extern rtx        gen_andsi_notsi_si                    (rtx, rtx, rtx);
extern rtx        gen_andsi_not_shiftsi_si              (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_arm_ashldi3_1bit                  (rtx, rtx);
extern rtx        gen_arm_ashrdi3_1bit                  (rtx, rtx);
extern rtx        gen_arm_lshrdi3_1bit                  (rtx, rtx);
extern rtx        gen_unaligned_loadsi                  (rtx, rtx);
extern rtx        gen_unaligned_loadhis                 (rtx, rtx);
extern rtx        gen_unaligned_loadhiu                 (rtx, rtx);
extern rtx        gen_unaligned_storesi                 (rtx, rtx);
extern rtx        gen_unaligned_storehi                 (rtx, rtx);
extern rtx        gen_unaligned_loaddi                  (rtx, rtx);
extern rtx        gen_unaligned_storedi                 (rtx, rtx);
extern rtx        gen_extzv_t2                          (rtx, rtx, rtx, rtx);
extern rtx        gen_divsi3                            (rtx, rtx, rtx);
extern rtx        gen_udivsi3                           (rtx, rtx, rtx);
extern rtx        gen_one_cmpldi2                       (rtx, rtx);
extern rtx        gen_zero_extendqidi2                  (rtx, rtx);
extern rtx        gen_zero_extendhidi2                  (rtx, rtx);
extern rtx        gen_zero_extendsidi2                  (rtx, rtx);
extern rtx        gen_extendqidi2                       (rtx, rtx);
extern rtx        gen_extendhidi2                       (rtx, rtx);
extern rtx        gen_extendsidi2                       (rtx, rtx);
extern rtx        gen_pic_load_addr_unified             (rtx, rtx, rtx);
extern rtx        gen_pic_load_addr_32bit               (rtx, rtx);
extern rtx        gen_pic_load_addr_thumb1              (rtx, rtx);
extern rtx        gen_pic_add_dot_plus_four             (rtx, rtx, rtx);
extern rtx        gen_pic_add_dot_plus_eight            (rtx, rtx, rtx);
extern rtx        gen_tls_load_dot_plus_eight           (rtx, rtx, rtx);
static inline rtx gen_pic_offset_arm                    (rtx, rtx, rtx);
static inline rtx
gen_pic_offset_arm(rtx ARG_UNUSED (a), rtx ARG_UNUSED (b), rtx ARG_UNUSED (c))
{
  return 0;
}
extern rtx        gen_arm_cond_branch                   (rtx, rtx, rtx);
extern rtx        gen_blockage                          (void);
extern rtx        gen_arm_casesi_internal               (rtx, rtx, rtx, rtx);
extern rtx        gen_nop                               (void);
extern rtx        gen_trap                              (void);
extern rtx        gen_movcond_addsi                     (rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_movcond                           (rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_stack_tie                         (rtx, rtx);
extern rtx        gen_align_4                           (void);
extern rtx        gen_align_8                           (void);
extern rtx        gen_consttable_end                    (void);
extern rtx        gen_consttable_1                      (rtx);
extern rtx        gen_consttable_2                      (rtx);
extern rtx        gen_consttable_4                      (rtx);
extern rtx        gen_consttable_8                      (rtx);
extern rtx        gen_consttable_16                     (rtx);
extern rtx        gen_clzsi2                            (rtx, rtx);
extern rtx        gen_rbitsi2                           (rtx, rtx);
extern rtx        gen_prefetch                          (rtx, rtx, rtx);
extern rtx        gen_force_register_use                (rtx);
extern rtx        gen_arm_eh_return                     (rtx);
extern rtx        gen_load_tp_hard                      (rtx);
extern rtx        gen_load_tp_soft                      (void);
extern rtx        gen_tlscall                           (rtx, rtx);
extern rtx        gen_arm_rev16si2                      (rtx, rtx, rtx, rtx);
extern rtx        gen_arm_rev16si2_alt                  (rtx, rtx, rtx, rtx);
extern rtx        gen_crc32b                            (rtx, rtx, rtx);
extern rtx        gen_crc32h                            (rtx, rtx, rtx);
extern rtx        gen_crc32w                            (rtx, rtx, rtx);
extern rtx        gen_crc32cb                           (rtx, rtx, rtx);
extern rtx        gen_crc32ch                           (rtx, rtx, rtx);
extern rtx        gen_crc32cw                           (rtx, rtx, rtx);
extern rtx        gen_tbcstv8qi                         (rtx, rtx);
extern rtx        gen_tbcstv4hi                         (rtx, rtx);
extern rtx        gen_tbcstv2si                         (rtx, rtx);
extern rtx        gen_iwmmxt_iordi3                     (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_xordi3                     (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_anddi3                     (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_nanddi3                    (rtx, rtx, rtx);
extern rtx        gen_movv2si_internal                  (rtx, rtx);
extern rtx        gen_movv4hi_internal                  (rtx, rtx);
extern rtx        gen_movv8qi_internal                  (rtx, rtx);
extern rtx        gen_ssaddv8qi3                        (rtx, rtx, rtx);
extern rtx        gen_ssaddv4hi3                        (rtx, rtx, rtx);
extern rtx        gen_ssaddv2si3                        (rtx, rtx, rtx);
extern rtx        gen_usaddv8qi3                        (rtx, rtx, rtx);
extern rtx        gen_usaddv4hi3                        (rtx, rtx, rtx);
extern rtx        gen_usaddv2si3                        (rtx, rtx, rtx);
extern rtx        gen_sssubv8qi3                        (rtx, rtx, rtx);
extern rtx        gen_sssubv4hi3                        (rtx, rtx, rtx);
extern rtx        gen_sssubv2si3                        (rtx, rtx, rtx);
extern rtx        gen_ussubv8qi3                        (rtx, rtx, rtx);
extern rtx        gen_ussubv4hi3                        (rtx, rtx, rtx);
extern rtx        gen_ussubv2si3                        (rtx, rtx, rtx);
extern rtx        gen_smulv4hi3_highpart                (rtx, rtx, rtx);
extern rtx        gen_umulv4hi3_highpart                (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmacs                      (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmacsz                     (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmacu                      (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmacuz                     (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_clrdi                      (rtx);
extern rtx        gen_iwmmxt_clrv8qi                    (rtx);
extern rtx        gen_iwmmxt_clrv4hi                    (rtx);
extern rtx        gen_iwmmxt_clrv2si                    (rtx);
extern rtx        gen_iwmmxt_uavgrndv8qi3               (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_uavgrndv4hi3               (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_uavgv8qi3                  (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_uavgv4hi3                  (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_tinsrb                     (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_tinsrh                     (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_tinsrw                     (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_textrmub                   (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_textrmsb                   (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_textrmuh                   (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_textrmsh                   (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_textrmw                    (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wshufh                     (rtx, rtx, rtx);
extern rtx        gen_eqv8qi3                           (rtx, rtx, rtx);
extern rtx        gen_eqv4hi3                           (rtx, rtx, rtx);
extern rtx        gen_eqv2si3                           (rtx, rtx, rtx);
extern rtx        gen_gtuv8qi3                          (rtx, rtx, rtx);
extern rtx        gen_gtuv4hi3                          (rtx, rtx, rtx);
extern rtx        gen_gtuv2si3                          (rtx, rtx, rtx);
extern rtx        gen_gtv8qi3                           (rtx, rtx, rtx);
extern rtx        gen_gtv4hi3                           (rtx, rtx, rtx);
extern rtx        gen_gtv2si3                           (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wpackhss                   (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wpackwss                   (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wpackdss                   (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wpackhus                   (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wpackwus                   (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wpackdus                   (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wunpckihb                  (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wunpckihh                  (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wunpckihw                  (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wunpckilb                  (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wunpckilh                  (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wunpckilw                  (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wunpckehub                 (rtx, rtx);
extern rtx        gen_iwmmxt_wunpckehuh                 (rtx, rtx);
extern rtx        gen_iwmmxt_wunpckehuw                 (rtx, rtx);
extern rtx        gen_iwmmxt_wunpckehsb                 (rtx, rtx);
extern rtx        gen_iwmmxt_wunpckehsh                 (rtx, rtx);
extern rtx        gen_iwmmxt_wunpckehsw                 (rtx, rtx);
extern rtx        gen_iwmmxt_wunpckelub                 (rtx, rtx);
extern rtx        gen_iwmmxt_wunpckeluh                 (rtx, rtx);
extern rtx        gen_iwmmxt_wunpckeluw                 (rtx, rtx);
extern rtx        gen_iwmmxt_wunpckelsb                 (rtx, rtx);
extern rtx        gen_iwmmxt_wunpckelsh                 (rtx, rtx);
extern rtx        gen_iwmmxt_wunpckelsw                 (rtx, rtx);
extern rtx        gen_rorv4hi3                          (rtx, rtx, rtx);
extern rtx        gen_rorv2si3                          (rtx, rtx, rtx);
extern rtx        gen_rordi3                            (rtx, rtx, rtx);
extern rtx        gen_ashrv4hi3_iwmmxt                  (rtx, rtx, rtx);
extern rtx        gen_ashrv2si3_iwmmxt                  (rtx, rtx, rtx);
extern rtx        gen_ashrdi3_iwmmxt                    (rtx, rtx, rtx);
extern rtx        gen_lshrv4hi3_iwmmxt                  (rtx, rtx, rtx);
extern rtx        gen_lshrv2si3_iwmmxt                  (rtx, rtx, rtx);
extern rtx        gen_lshrdi3_iwmmxt                    (rtx, rtx, rtx);
extern rtx        gen_ashlv4hi3_iwmmxt                  (rtx, rtx, rtx);
extern rtx        gen_ashlv2si3_iwmmxt                  (rtx, rtx, rtx);
extern rtx        gen_ashldi3_iwmmxt                    (rtx, rtx, rtx);
extern rtx        gen_rorv4hi3_di                       (rtx, rtx, rtx);
extern rtx        gen_rorv2si3_di                       (rtx, rtx, rtx);
extern rtx        gen_rordi3_di                         (rtx, rtx, rtx);
extern rtx        gen_ashrv4hi3_di                      (rtx, rtx, rtx);
extern rtx        gen_ashrv2si3_di                      (rtx, rtx, rtx);
extern rtx        gen_ashrdi3_di                        (rtx, rtx, rtx);
extern rtx        gen_lshrv4hi3_di                      (rtx, rtx, rtx);
extern rtx        gen_lshrv2si3_di                      (rtx, rtx, rtx);
extern rtx        gen_lshrdi3_di                        (rtx, rtx, rtx);
extern rtx        gen_ashlv4hi3_di                      (rtx, rtx, rtx);
extern rtx        gen_ashlv2si3_di                      (rtx, rtx, rtx);
extern rtx        gen_ashldi3_di                        (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmadds                     (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmaddu                     (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_tmia                       (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_tmiaph                     (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_tmiabb                     (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_tmiatb                     (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_tmiabt                     (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_tmiatt                     (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_tmovmskb                   (rtx, rtx);
extern rtx        gen_iwmmxt_tmovmskh                   (rtx, rtx);
extern rtx        gen_iwmmxt_tmovmskw                   (rtx, rtx);
extern rtx        gen_iwmmxt_waccb                      (rtx, rtx);
extern rtx        gen_iwmmxt_wacch                      (rtx, rtx);
extern rtx        gen_iwmmxt_waccw                      (rtx, rtx);
extern rtx        gen_iwmmxt_waligni                    (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_walignr                    (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_walignr0                   (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_walignr1                   (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_walignr2                   (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_walignr3                   (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wsadb                      (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wsadh                      (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wsadbz                     (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wsadhz                     (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wabsv2si3                  (rtx, rtx);
extern rtx        gen_iwmmxt_wabsv4hi3                  (rtx, rtx);
extern rtx        gen_iwmmxt_wabsv8qi3                  (rtx, rtx);
extern rtx        gen_iwmmxt_wabsdiffb                  (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wabsdiffh                  (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wabsdiffw                  (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_waddsubhx                  (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wsubaddhx                  (rtx, rtx, rtx);
extern rtx        gen_addcv4hi3                         (rtx, rtx, rtx);
extern rtx        gen_addcv2si3                         (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_avg4                       (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_avg4r                      (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmaddsx                    (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmaddux                    (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmaddsn                    (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmaddun                    (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmulwsm                    (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmulwum                    (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmulsmr                    (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmulumr                    (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmulwsmr                   (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmulwumr                   (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmulwl                     (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wqmulm                     (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wqmulwm                    (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wqmulmr                    (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wqmulwmr                   (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_waddbhusm                  (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_waddbhusl                  (rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wqmiabb                    (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wqmiabt                    (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wqmiatb                    (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wqmiatt                    (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wqmiabbn                   (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wqmiabtn                   (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wqmiatbn                   (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wqmiattn                   (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmiabb                     (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmiabt                     (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmiatb                     (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmiatt                     (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmiabbn                    (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmiabtn                    (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmiatbn                    (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmiattn                    (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmiawbb                    (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmiawbt                    (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmiawtb                    (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmiawtt                    (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmiawbbn                   (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmiawbtn                   (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmiawtbn                   (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmiawttn                   (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_wmerge                     (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_tandcv2si3                 (void);
extern rtx        gen_iwmmxt_tandcv4hi3                 (void);
extern rtx        gen_iwmmxt_tandcv8qi3                 (void);
extern rtx        gen_iwmmxt_torcv2si3                  (void);
extern rtx        gen_iwmmxt_torcv4hi3                  (void);
extern rtx        gen_iwmmxt_torcv8qi3                  (void);
extern rtx        gen_iwmmxt_torvscv2si3                (void);
extern rtx        gen_iwmmxt_torvscv4hi3                (void);
extern rtx        gen_iwmmxt_torvscv8qi3                (void);
extern rtx        gen_iwmmxt_textrcv2si3                (rtx);
extern rtx        gen_iwmmxt_textrcv4hi3                (rtx);
extern rtx        gen_iwmmxt_textrcv8qi3                (rtx);
extern rtx        gen_fmasf4                            (rtx, rtx, rtx, rtx);
extern rtx        gen_fmadf4                            (rtx, rtx, rtx, rtx);
extern rtx        gen_extendhfsf2                       (rtx, rtx);
extern rtx        gen_truncsfhf2                        (rtx, rtx);
extern rtx        gen_fixuns_truncsfsi2                 (rtx, rtx);
extern rtx        gen_fixuns_truncdfsi2                 (rtx, rtx);
extern rtx        gen_floatunssisf2                     (rtx, rtx);
extern rtx        gen_floatunssidf2                     (rtx, rtx);
extern rtx        gen_btruncsf2                         (rtx, rtx);
extern rtx        gen_ceilsf2                           (rtx, rtx);
extern rtx        gen_floorsf2                          (rtx, rtx);
extern rtx        gen_nearbyintsf2                      (rtx, rtx);
extern rtx        gen_rintsf2                           (rtx, rtx);
extern rtx        gen_roundsf2                          (rtx, rtx);
extern rtx        gen_btruncdf2                         (rtx, rtx);
extern rtx        gen_ceildf2                           (rtx, rtx);
extern rtx        gen_floordf2                          (rtx, rtx);
extern rtx        gen_nearbyintdf2                      (rtx, rtx);
extern rtx        gen_rintdf2                           (rtx, rtx);
extern rtx        gen_rounddf2                          (rtx, rtx);
extern rtx        gen_lceilsfsi2                        (rtx, rtx);
extern rtx        gen_lfloorsfsi2                       (rtx, rtx);
extern rtx        gen_lroundsfsi2                       (rtx, rtx);
extern rtx        gen_lceilusfsi2                       (rtx, rtx);
extern rtx        gen_lfloorusfsi2                      (rtx, rtx);
extern rtx        gen_lroundusfsi2                      (rtx, rtx);
extern rtx        gen_lceildfsi2                        (rtx, rtx);
extern rtx        gen_lfloordfsi2                       (rtx, rtx);
extern rtx        gen_lrounddfsi2                       (rtx, rtx);
extern rtx        gen_lceiludfsi2                       (rtx, rtx);
extern rtx        gen_lfloorudfsi2                      (rtx, rtx);
extern rtx        gen_lroundudfsi2                      (rtx, rtx);
extern rtx        gen_smaxsf3                           (rtx, rtx, rtx);
extern rtx        gen_smaxdf3                           (rtx, rtx, rtx);
extern rtx        gen_sminsf3                           (rtx, rtx, rtx);
extern rtx        gen_smindf3                           (rtx, rtx, rtx);
extern rtx        gen_set_fpscr                         (rtx);
extern rtx        gen_get_fpscr                         (rtx);
extern rtx        gen_thumb1_subsi3_insn                (rtx, rtx, rtx);
extern rtx        gen_thumb1_bicsi3                     (rtx, rtx, rtx);
extern rtx        gen_thumb1_extendhisi2                (rtx, rtx);
extern rtx        gen_thumb1_extendqisi2                (rtx, rtx);
extern rtx        gen_movmem12b                         (rtx, rtx, rtx, rtx);
extern rtx        gen_movmem8b                          (rtx, rtx, rtx, rtx);
extern rtx        gen_thumb1_cbz                        (rtx, rtx, rtx);
extern rtx        gen_cbranchsi4_insn                   (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchsi4_scratch                (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_cstoresi_nltu_thumb1              (rtx, rtx, rtx);
extern rtx        gen_cstoresi_ltu_thumb1               (rtx, rtx, rtx);
extern rtx        gen_thumb1_addsi3_addgeu              (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_thumb1_casesi_dispatch            (rtx);
extern rtx        gen_prologue_thumb1_interwork         (void);
extern rtx        gen_thumb_eh_return                   (rtx);
extern rtx        gen_tls_load_dot_plus_four            (rtx, rtx, rtx, rtx);
extern rtx        gen_thumb2_zero_extendqisi2_v6        (rtx, rtx);
extern rtx        gen_thumb2_casesi_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_thumb2_casesi_internal_pic        (rtx, rtx, rtx, rtx);
extern rtx        gen_thumb2_eh_return                  (rtx);
extern rtx        gen_thumb2_addsi3_compare0            (rtx, rtx, rtx);
extern rtx        gen_vec_setv8qi_internal              (rtx, rtx, rtx, rtx);
extern rtx        gen_vec_setv4hi_internal              (rtx, rtx, rtx, rtx);
extern rtx        gen_vec_setv2si_internal              (rtx, rtx, rtx, rtx);
extern rtx        gen_vec_setv2sf_internal              (rtx, rtx, rtx, rtx);
extern rtx        gen_vec_setv16qi_internal             (rtx, rtx, rtx, rtx);
extern rtx        gen_vec_setv8hi_internal              (rtx, rtx, rtx, rtx);
extern rtx        gen_vec_setv4si_internal              (rtx, rtx, rtx, rtx);
extern rtx        gen_vec_setv4sf_internal              (rtx, rtx, rtx, rtx);
extern rtx        gen_vec_setv2di_internal              (rtx, rtx, rtx, rtx);
extern rtx        gen_vec_extractv8qi                   (rtx, rtx, rtx);
extern rtx        gen_vec_extractv4hi                   (rtx, rtx, rtx);
extern rtx        gen_vec_extractv2si                   (rtx, rtx, rtx);
extern rtx        gen_vec_extractv2sf                   (rtx, rtx, rtx);
extern rtx        gen_vec_extractv16qi                  (rtx, rtx, rtx);
extern rtx        gen_vec_extractv8hi                   (rtx, rtx, rtx);
extern rtx        gen_vec_extractv4si                   (rtx, rtx, rtx);
extern rtx        gen_vec_extractv4sf                   (rtx, rtx, rtx);
extern rtx        gen_vec_extractv2di                   (rtx, rtx, rtx);
extern rtx        gen_adddi3_neon                       (rtx, rtx, rtx);
extern rtx        gen_subdi3_neon                       (rtx, rtx, rtx);
extern rtx        gen_mulv8qi3addv8qi_neon              (rtx, rtx, rtx, rtx);
extern rtx        gen_mulv16qi3addv16qi_neon            (rtx, rtx, rtx, rtx);
extern rtx        gen_mulv4hi3addv4hi_neon              (rtx, rtx, rtx, rtx);
extern rtx        gen_mulv8hi3addv8hi_neon              (rtx, rtx, rtx, rtx);
extern rtx        gen_mulv2si3addv2si_neon              (rtx, rtx, rtx, rtx);
extern rtx        gen_mulv4si3addv4si_neon              (rtx, rtx, rtx, rtx);
extern rtx        gen_mulv2sf3addv2sf_neon              (rtx, rtx, rtx, rtx);
extern rtx        gen_mulv4sf3addv4sf_neon              (rtx, rtx, rtx, rtx);
extern rtx        gen_mulv8qi3negv8qiaddv8qi_neon       (rtx, rtx, rtx, rtx);
extern rtx        gen_mulv16qi3negv16qiaddv16qi_neon    (rtx, rtx, rtx, rtx);
extern rtx        gen_mulv4hi3negv4hiaddv4hi_neon       (rtx, rtx, rtx, rtx);
extern rtx        gen_mulv8hi3negv8hiaddv8hi_neon       (rtx, rtx, rtx, rtx);
extern rtx        gen_mulv2si3negv2siaddv2si_neon       (rtx, rtx, rtx, rtx);
extern rtx        gen_mulv4si3negv4siaddv4si_neon       (rtx, rtx, rtx, rtx);
extern rtx        gen_mulv2sf3negv2sfaddv2sf_neon       (rtx, rtx, rtx, rtx);
extern rtx        gen_mulv4sf3negv4sfaddv4sf_neon       (rtx, rtx, rtx, rtx);
extern rtx        gen_fmav2sf4                          (rtx, rtx, rtx, rtx);
extern rtx        gen_fmav4sf4                          (rtx, rtx, rtx, rtx);
extern rtx        gen_fmav2sf4_intrinsic                (rtx, rtx, rtx, rtx);
extern rtx        gen_fmav4sf4_intrinsic                (rtx, rtx, rtx, rtx);
extern rtx        gen_fmsubv2sf4_intrinsic              (rtx, rtx, rtx, rtx);
extern rtx        gen_fmsubv4sf4_intrinsic              (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vrintpv2sf                   (rtx, rtx);
extern rtx        gen_neon_vrintzv2sf                   (rtx, rtx);
extern rtx        gen_neon_vrintmv2sf                   (rtx, rtx);
extern rtx        gen_neon_vrintxv2sf                   (rtx, rtx);
extern rtx        gen_neon_vrintav2sf                   (rtx, rtx);
extern rtx        gen_neon_vrintnv2sf                   (rtx, rtx);
extern rtx        gen_neon_vrintpv4sf                   (rtx, rtx);
extern rtx        gen_neon_vrintzv4sf                   (rtx, rtx);
extern rtx        gen_neon_vrintmv4sf                   (rtx, rtx);
extern rtx        gen_neon_vrintxv4sf                   (rtx, rtx);
extern rtx        gen_neon_vrintav4sf                   (rtx, rtx);
extern rtx        gen_neon_vrintnv4sf                   (rtx, rtx);
extern rtx        gen_neon_vcvtpv2sfv2si                (rtx, rtx);
extern rtx        gen_neon_vcvtmv2sfv2si                (rtx, rtx);
extern rtx        gen_neon_vcvtav2sfv2si                (rtx, rtx);
extern rtx        gen_neon_vcvtpuv2sfv2si               (rtx, rtx);
extern rtx        gen_neon_vcvtmuv2sfv2si               (rtx, rtx);
extern rtx        gen_neon_vcvtauv2sfv2si               (rtx, rtx);
extern rtx        gen_neon_vcvtpv4sfv4si                (rtx, rtx);
extern rtx        gen_neon_vcvtmv4sfv4si                (rtx, rtx);
extern rtx        gen_neon_vcvtav4sfv4si                (rtx, rtx);
extern rtx        gen_neon_vcvtpuv4sfv4si               (rtx, rtx);
extern rtx        gen_neon_vcvtmuv4sfv4si               (rtx, rtx);
extern rtx        gen_neon_vcvtauv4sfv4si               (rtx, rtx);
extern rtx        gen_iorv8qi3                          (rtx, rtx, rtx);
extern rtx        gen_iorv16qi3                         (rtx, rtx, rtx);
extern rtx        gen_iorv4hi3                          (rtx, rtx, rtx);
extern rtx        gen_iorv8hi3                          (rtx, rtx, rtx);
extern rtx        gen_iorv2si3                          (rtx, rtx, rtx);
extern rtx        gen_iorv4si3                          (rtx, rtx, rtx);
extern rtx        gen_iorv2sf3                          (rtx, rtx, rtx);
extern rtx        gen_iorv4sf3                          (rtx, rtx, rtx);
extern rtx        gen_iorv2di3                          (rtx, rtx, rtx);
extern rtx        gen_andv8qi3                          (rtx, rtx, rtx);
extern rtx        gen_andv16qi3                         (rtx, rtx, rtx);
extern rtx        gen_andv4hi3                          (rtx, rtx, rtx);
extern rtx        gen_andv8hi3                          (rtx, rtx, rtx);
extern rtx        gen_andv2si3                          (rtx, rtx, rtx);
extern rtx        gen_andv4si3                          (rtx, rtx, rtx);
extern rtx        gen_andv2sf3                          (rtx, rtx, rtx);
extern rtx        gen_andv4sf3                          (rtx, rtx, rtx);
extern rtx        gen_andv2di3                          (rtx, rtx, rtx);
extern rtx        gen_ornv8qi3_neon                     (rtx, rtx, rtx);
extern rtx        gen_ornv16qi3_neon                    (rtx, rtx, rtx);
extern rtx        gen_ornv4hi3_neon                     (rtx, rtx, rtx);
extern rtx        gen_ornv8hi3_neon                     (rtx, rtx, rtx);
extern rtx        gen_ornv2si3_neon                     (rtx, rtx, rtx);
extern rtx        gen_ornv4si3_neon                     (rtx, rtx, rtx);
extern rtx        gen_ornv2sf3_neon                     (rtx, rtx, rtx);
extern rtx        gen_ornv4sf3_neon                     (rtx, rtx, rtx);
extern rtx        gen_ornv2di3_neon                     (rtx, rtx, rtx);
extern rtx        gen_orndi3_neon                       (rtx, rtx, rtx);
extern rtx        gen_bicv8qi3_neon                     (rtx, rtx, rtx);
extern rtx        gen_bicv16qi3_neon                    (rtx, rtx, rtx);
extern rtx        gen_bicv4hi3_neon                     (rtx, rtx, rtx);
extern rtx        gen_bicv8hi3_neon                     (rtx, rtx, rtx);
extern rtx        gen_bicv2si3_neon                     (rtx, rtx, rtx);
extern rtx        gen_bicv4si3_neon                     (rtx, rtx, rtx);
extern rtx        gen_bicv2sf3_neon                     (rtx, rtx, rtx);
extern rtx        gen_bicv4sf3_neon                     (rtx, rtx, rtx);
extern rtx        gen_bicv2di3_neon                     (rtx, rtx, rtx);
extern rtx        gen_bicdi3_neon                       (rtx, rtx, rtx);
extern rtx        gen_xorv8qi3                          (rtx, rtx, rtx);
extern rtx        gen_xorv16qi3                         (rtx, rtx, rtx);
extern rtx        gen_xorv4hi3                          (rtx, rtx, rtx);
extern rtx        gen_xorv8hi3                          (rtx, rtx, rtx);
extern rtx        gen_xorv2si3                          (rtx, rtx, rtx);
extern rtx        gen_xorv4si3                          (rtx, rtx, rtx);
extern rtx        gen_xorv2sf3                          (rtx, rtx, rtx);
extern rtx        gen_xorv4sf3                          (rtx, rtx, rtx);
extern rtx        gen_xorv2di3                          (rtx, rtx, rtx);
extern rtx        gen_one_cmplv8qi2                     (rtx, rtx);
extern rtx        gen_one_cmplv16qi2                    (rtx, rtx);
extern rtx        gen_one_cmplv4hi2                     (rtx, rtx);
extern rtx        gen_one_cmplv8hi2                     (rtx, rtx);
extern rtx        gen_one_cmplv2si2                     (rtx, rtx);
extern rtx        gen_one_cmplv4si2                     (rtx, rtx);
extern rtx        gen_one_cmplv2sf2                     (rtx, rtx);
extern rtx        gen_one_cmplv4sf2                     (rtx, rtx);
extern rtx        gen_one_cmplv2di2                     (rtx, rtx);
extern rtx        gen_absv8qi2                          (rtx, rtx);
extern rtx        gen_absv16qi2                         (rtx, rtx);
extern rtx        gen_absv4hi2                          (rtx, rtx);
extern rtx        gen_absv8hi2                          (rtx, rtx);
extern rtx        gen_absv2si2                          (rtx, rtx);
extern rtx        gen_absv4si2                          (rtx, rtx);
extern rtx        gen_absv2sf2                          (rtx, rtx);
extern rtx        gen_absv4sf2                          (rtx, rtx);
extern rtx        gen_negv8qi2                          (rtx, rtx);
extern rtx        gen_negv16qi2                         (rtx, rtx);
extern rtx        gen_negv4hi2                          (rtx, rtx);
extern rtx        gen_negv8hi2                          (rtx, rtx);
extern rtx        gen_negv2si2                          (rtx, rtx);
extern rtx        gen_negv4si2                          (rtx, rtx);
extern rtx        gen_negv2sf2                          (rtx, rtx);
extern rtx        gen_negv4sf2                          (rtx, rtx);
extern rtx        gen_negdi2_neon                       (rtx, rtx);
extern rtx        gen_vashlv8qi3                        (rtx, rtx, rtx);
extern rtx        gen_vashlv16qi3                       (rtx, rtx, rtx);
extern rtx        gen_vashlv4hi3                        (rtx, rtx, rtx);
extern rtx        gen_vashlv8hi3                        (rtx, rtx, rtx);
extern rtx        gen_vashlv2si3                        (rtx, rtx, rtx);
extern rtx        gen_vashlv4si3                        (rtx, rtx, rtx);
extern rtx        gen_vashrv8qi3_imm                    (rtx, rtx, rtx);
extern rtx        gen_vashrv16qi3_imm                   (rtx, rtx, rtx);
extern rtx        gen_vashrv4hi3_imm                    (rtx, rtx, rtx);
extern rtx        gen_vashrv8hi3_imm                    (rtx, rtx, rtx);
extern rtx        gen_vashrv2si3_imm                    (rtx, rtx, rtx);
extern rtx        gen_vashrv4si3_imm                    (rtx, rtx, rtx);
extern rtx        gen_vlshrv8qi3_imm                    (rtx, rtx, rtx);
extern rtx        gen_vlshrv16qi3_imm                   (rtx, rtx, rtx);
extern rtx        gen_vlshrv4hi3_imm                    (rtx, rtx, rtx);
extern rtx        gen_vlshrv8hi3_imm                    (rtx, rtx, rtx);
extern rtx        gen_vlshrv2si3_imm                    (rtx, rtx, rtx);
extern rtx        gen_vlshrv4si3_imm                    (rtx, rtx, rtx);
extern rtx        gen_ashlv8qi3_signed                  (rtx, rtx, rtx);
extern rtx        gen_ashlv16qi3_signed                 (rtx, rtx, rtx);
extern rtx        gen_ashlv4hi3_signed                  (rtx, rtx, rtx);
extern rtx        gen_ashlv8hi3_signed                  (rtx, rtx, rtx);
extern rtx        gen_ashlv2si3_signed                  (rtx, rtx, rtx);
extern rtx        gen_ashlv4si3_signed                  (rtx, rtx, rtx);
extern rtx        gen_ashlv2di3_signed                  (rtx, rtx, rtx);
extern rtx        gen_ashlv8qi3_unsigned                (rtx, rtx, rtx);
extern rtx        gen_ashlv16qi3_unsigned               (rtx, rtx, rtx);
extern rtx        gen_ashlv4hi3_unsigned                (rtx, rtx, rtx);
extern rtx        gen_ashlv8hi3_unsigned                (rtx, rtx, rtx);
extern rtx        gen_ashlv2si3_unsigned                (rtx, rtx, rtx);
extern rtx        gen_ashlv4si3_unsigned                (rtx, rtx, rtx);
extern rtx        gen_ashlv2di3_unsigned                (rtx, rtx, rtx);
extern rtx        gen_neon_load_count                   (rtx, rtx);
extern rtx        gen_ashldi3_neon_noclobber            (rtx, rtx, rtx);
extern rtx        gen_ashldi3_neon                      (rtx, rtx, rtx);
extern rtx        gen_signed_shift_di3_neon             (rtx, rtx, rtx);
extern rtx        gen_unsigned_shift_di3_neon           (rtx, rtx, rtx);
extern rtx        gen_ashrdi3_neon_imm_noclobber        (rtx, rtx, rtx);
extern rtx        gen_lshrdi3_neon_imm_noclobber        (rtx, rtx, rtx);
extern rtx        gen_ashrdi3_neon                      (rtx, rtx, rtx);
extern rtx        gen_lshrdi3_neon                      (rtx, rtx, rtx);
extern rtx        gen_widen_ssumv8qi3                   (rtx, rtx, rtx);
extern rtx        gen_widen_ssumv4hi3                   (rtx, rtx, rtx);
extern rtx        gen_widen_ssumv2si3                   (rtx, rtx, rtx);
extern rtx        gen_widen_usumv8qi3                   (rtx, rtx, rtx);
extern rtx        gen_widen_usumv4hi3                   (rtx, rtx, rtx);
extern rtx        gen_widen_usumv2si3                   (rtx, rtx, rtx);
extern rtx        gen_quad_halves_plusv4si              (rtx, rtx);
extern rtx        gen_quad_halves_sminv4si              (rtx, rtx);
extern rtx        gen_quad_halves_smaxv4si              (rtx, rtx);
extern rtx        gen_quad_halves_uminv4si              (rtx, rtx);
extern rtx        gen_quad_halves_umaxv4si              (rtx, rtx);
extern rtx        gen_quad_halves_plusv4sf              (rtx, rtx);
extern rtx        gen_quad_halves_sminv4sf              (rtx, rtx);
extern rtx        gen_quad_halves_smaxv4sf              (rtx, rtx);
extern rtx        gen_quad_halves_plusv8hi              (rtx, rtx);
extern rtx        gen_quad_halves_sminv8hi              (rtx, rtx);
extern rtx        gen_quad_halves_smaxv8hi              (rtx, rtx);
extern rtx        gen_quad_halves_uminv8hi              (rtx, rtx);
extern rtx        gen_quad_halves_umaxv8hi              (rtx, rtx);
extern rtx        gen_quad_halves_plusv16qi             (rtx, rtx);
extern rtx        gen_quad_halves_sminv16qi             (rtx, rtx);
extern rtx        gen_quad_halves_smaxv16qi             (rtx, rtx);
extern rtx        gen_quad_halves_uminv16qi             (rtx, rtx);
extern rtx        gen_quad_halves_umaxv16qi             (rtx, rtx);
extern rtx        gen_arm_reduc_plus_internal_v2di      (rtx, rtx);
extern rtx        gen_neon_vpadd_internalv8qi           (rtx, rtx, rtx);
extern rtx        gen_neon_vpadd_internalv4hi           (rtx, rtx, rtx);
extern rtx        gen_neon_vpadd_internalv2si           (rtx, rtx, rtx);
extern rtx        gen_neon_vpadd_internalv2sf           (rtx, rtx, rtx);
extern rtx        gen_neon_vpsminv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpsminv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpsminv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpsminv2sf                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpsmaxv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpsmaxv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpsmaxv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpsmaxv2sf                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpuminv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpuminv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpuminv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpumaxv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpumaxv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpumaxv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vaddv2sf_unspec              (rtx, rtx, rtx);
extern rtx        gen_neon_vaddv4sf_unspec              (rtx, rtx, rtx);
extern rtx        gen_neon_vaddlsv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vaddluv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vaddlsv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vaddluv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vaddlsv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vaddluv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vaddwsv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vaddwuv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vaddwsv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vaddwuv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vaddwsv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vaddwuv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vrhaddsv8qi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vrhadduv8qi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vhaddsv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vhadduv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vrhaddsv16qi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vrhadduv16qi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vhaddsv16qi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vhadduv16qi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vrhaddsv4hi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vrhadduv4hi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vhaddsv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vhadduv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vrhaddsv8hi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vrhadduv8hi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vhaddsv8hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vhadduv8hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vrhaddsv2si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vrhadduv2si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vhaddsv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vhadduv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vrhaddsv4si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vrhadduv4si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vhaddsv4si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vhadduv4si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqaddsv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqadduv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqaddsv16qi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqadduv16qi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqaddsv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqadduv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqaddsv8hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqadduv8hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqaddsv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqadduv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqaddsv4si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqadduv4si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqaddsdi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vqaddudi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vqaddsv2di                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqadduv2di                   (rtx, rtx, rtx);
extern rtx        gen_neon_vaddhnv8hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vraddhnv8hi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vaddhnv4si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vraddhnv4si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vaddhnv2di                   (rtx, rtx, rtx);
extern rtx        gen_neon_vraddhnv2di                  (rtx, rtx, rtx);
extern rtx        gen_neon_vmulpv8qi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vmulpv16qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vmulfv2sf                    (rtx, rtx, rtx);
extern rtx        gen_neon_vmulfv4sf                    (rtx, rtx, rtx);
extern rtx        gen_neon_vmlav8qi_unspec              (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlav16qi_unspec             (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlav4hi_unspec              (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlav8hi_unspec              (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlav2si_unspec              (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlav4si_unspec              (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlav2sf_unspec              (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlav4sf_unspec              (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlalsv8qi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlaluv8qi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlalsv4hi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlaluv4hi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlalsv2si                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlaluv2si                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlsv8qi_unspec              (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlsv16qi_unspec             (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlsv4hi_unspec              (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlsv8hi_unspec              (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlsv2si_unspec              (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlsv4si_unspec              (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlsv2sf_unspec              (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlsv4sf_unspec              (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlslsv8qi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlsluv8qi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlslsv4hi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlsluv4hi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlslsv2si                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlsluv2si                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vqdmulhv4hi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqrdmulhv4hi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vqdmulhv2si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqrdmulhv2si                 (rtx, rtx, rtx);
extern rtx        gen_neon_vqdmulhv8hi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqrdmulhv8hi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vqdmulhv4si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqrdmulhv4si                 (rtx, rtx, rtx);
extern rtx        gen_neon_vqdmlalv4hi                  (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vqdmlalv2si                  (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vqdmlslv4hi                  (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vqdmlslv2si                  (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmullsv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vmulluv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vmullpv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vmullsv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vmulluv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vmullpv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vmullsv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vmulluv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vmullpv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqdmullv4hi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqdmullv2si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vsubv2sf_unspec              (rtx, rtx, rtx);
extern rtx        gen_neon_vsubv4sf_unspec              (rtx, rtx, rtx);
extern rtx        gen_neon_vsublsv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vsubluv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vsublsv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vsubluv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vsublsv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vsubluv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vsubwsv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vsubwuv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vsubwsv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vsubwuv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vsubwsv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vsubwuv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqsubsv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqsubuv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqsubsv16qi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqsubuv16qi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqsubsv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqsubuv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqsubsv8hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqsubuv8hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqsubsv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqsubuv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqsubsv4si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqsubuv4si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqsubsdi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vqsubudi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vqsubsv2di                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqsubuv2di                   (rtx, rtx, rtx);
extern rtx        gen_neon_vhsubsv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vhsubuv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vhsubsv16qi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vhsubuv16qi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vhsubsv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vhsubuv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vhsubsv8hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vhsubuv8hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vhsubsv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vhsubuv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vhsubsv4si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vhsubuv4si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vsubhnv8hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vrsubhnv8hi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vsubhnv4si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vrsubhnv4si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vsubhnv2di                   (rtx, rtx, rtx);
extern rtx        gen_neon_vrsubhnv2di                  (rtx, rtx, rtx);
extern rtx        gen_neon_vceqv8qi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vceqv16qi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vceqv4hi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vceqv8hi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vceqv2si                     (rtx, rtx, rtx);
extern rtx        gen_neon_vceqv4si                     (rtx, rtx, rtx);
extern rtx        gen_neon_vceqv2sf                     (rtx, rtx, rtx);
extern rtx        gen_neon_vceqv4sf                     (rtx, rtx, rtx);
extern rtx        gen_neon_vcgev8qi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vcgev16qi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vcgev4hi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vcgev8hi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vcgev2si                     (rtx, rtx, rtx);
extern rtx        gen_neon_vcgev4si                     (rtx, rtx, rtx);
extern rtx        gen_neon_vcgev2sf                     (rtx, rtx, rtx);
extern rtx        gen_neon_vcgev4sf                     (rtx, rtx, rtx);
extern rtx        gen_neon_vcgeuv8qi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vcgeuv16qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vcgeuv4hi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vcgeuv8hi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vcgeuv2si                    (rtx, rtx, rtx);
extern rtx        gen_neon_vcgeuv4si                    (rtx, rtx, rtx);
extern rtx        gen_neon_vcgtv8qi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vcgtv16qi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vcgtv4hi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vcgtv8hi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vcgtv2si                     (rtx, rtx, rtx);
extern rtx        gen_neon_vcgtv4si                     (rtx, rtx, rtx);
extern rtx        gen_neon_vcgtv2sf                     (rtx, rtx, rtx);
extern rtx        gen_neon_vcgtv4sf                     (rtx, rtx, rtx);
extern rtx        gen_neon_vcgtuv8qi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vcgtuv16qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vcgtuv4hi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vcgtuv8hi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vcgtuv2si                    (rtx, rtx, rtx);
extern rtx        gen_neon_vcgtuv4si                    (rtx, rtx, rtx);
extern rtx        gen_neon_vclev8qi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vclev16qi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vclev4hi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vclev8hi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vclev2si                     (rtx, rtx, rtx);
extern rtx        gen_neon_vclev4si                     (rtx, rtx, rtx);
extern rtx        gen_neon_vclev2sf                     (rtx, rtx, rtx);
extern rtx        gen_neon_vclev4sf                     (rtx, rtx, rtx);
extern rtx        gen_neon_vcltv8qi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vcltv16qi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vcltv4hi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vcltv8hi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vcltv2si                     (rtx, rtx, rtx);
extern rtx        gen_neon_vcltv4si                     (rtx, rtx, rtx);
extern rtx        gen_neon_vcltv2sf                     (rtx, rtx, rtx);
extern rtx        gen_neon_vcltv4sf                     (rtx, rtx, rtx);
extern rtx        gen_neon_vcagev2sf                    (rtx, rtx, rtx);
extern rtx        gen_neon_vcagev4sf                    (rtx, rtx, rtx);
extern rtx        gen_neon_vcagtv2sf                    (rtx, rtx, rtx);
extern rtx        gen_neon_vcagtv4sf                    (rtx, rtx, rtx);
extern rtx        gen_neon_vtstv8qi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vtstv16qi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vtstv4hi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vtstv8hi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vtstv2si                     (rtx, rtx, rtx);
extern rtx        gen_neon_vtstv4si                     (rtx, rtx, rtx);
extern rtx        gen_neon_vabdsv8qi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vabduv8qi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vabdsv16qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vabduv16qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vabdsv4hi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vabduv4hi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vabdsv8hi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vabduv8hi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vabdsv2si                    (rtx, rtx, rtx);
extern rtx        gen_neon_vabduv2si                    (rtx, rtx, rtx);
extern rtx        gen_neon_vabdsv4si                    (rtx, rtx, rtx);
extern rtx        gen_neon_vabduv4si                    (rtx, rtx, rtx);
extern rtx        gen_neon_vabdfv2sf                    (rtx, rtx, rtx);
extern rtx        gen_neon_vabdfv4sf                    (rtx, rtx, rtx);
extern rtx        gen_neon_vabdlsv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vabdluv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vabdlsv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vabdluv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vabdlsv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vabdluv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vabasv8qi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vabauv8qi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vabasv16qi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vabauv16qi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vabasv4hi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vabauv4hi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vabasv8hi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vabauv8hi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vabasv2si                    (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vabauv2si                    (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vabasv4si                    (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vabauv4si                    (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vabalsv8qi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vabaluv8qi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vabalsv4hi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vabaluv4hi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vabalsv2si                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vabaluv2si                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmaxsv8qi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vmaxuv8qi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vminsv8qi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vminuv8qi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vmaxsv16qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vmaxuv16qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vminsv16qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vminuv16qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vmaxsv4hi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vmaxuv4hi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vminsv4hi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vminuv4hi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vmaxsv8hi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vmaxuv8hi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vminsv8hi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vminuv8hi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vmaxsv2si                    (rtx, rtx, rtx);
extern rtx        gen_neon_vmaxuv2si                    (rtx, rtx, rtx);
extern rtx        gen_neon_vminsv2si                    (rtx, rtx, rtx);
extern rtx        gen_neon_vminuv2si                    (rtx, rtx, rtx);
extern rtx        gen_neon_vmaxsv4si                    (rtx, rtx, rtx);
extern rtx        gen_neon_vmaxuv4si                    (rtx, rtx, rtx);
extern rtx        gen_neon_vminsv4si                    (rtx, rtx, rtx);
extern rtx        gen_neon_vminuv4si                    (rtx, rtx, rtx);
extern rtx        gen_neon_vmaxfv2sf                    (rtx, rtx, rtx);
extern rtx        gen_neon_vminfv2sf                    (rtx, rtx, rtx);
extern rtx        gen_neon_vmaxfv4sf                    (rtx, rtx, rtx);
extern rtx        gen_neon_vminfv4sf                    (rtx, rtx, rtx);
extern rtx        gen_neon_vpaddlsv8qi                  (rtx, rtx);
extern rtx        gen_neon_vpaddluv8qi                  (rtx, rtx);
extern rtx        gen_neon_vpaddlsv16qi                 (rtx, rtx);
extern rtx        gen_neon_vpaddluv16qi                 (rtx, rtx);
extern rtx        gen_neon_vpaddlsv4hi                  (rtx, rtx);
extern rtx        gen_neon_vpaddluv4hi                  (rtx, rtx);
extern rtx        gen_neon_vpaddlsv8hi                  (rtx, rtx);
extern rtx        gen_neon_vpaddluv8hi                  (rtx, rtx);
extern rtx        gen_neon_vpaddlsv2si                  (rtx, rtx);
extern rtx        gen_neon_vpaddluv2si                  (rtx, rtx);
extern rtx        gen_neon_vpaddlsv4si                  (rtx, rtx);
extern rtx        gen_neon_vpaddluv4si                  (rtx, rtx);
extern rtx        gen_neon_vpadalsv8qi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vpadaluv8qi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vpadalsv16qi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vpadaluv16qi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vpadalsv4hi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vpadaluv4hi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vpadalsv8hi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vpadaluv8hi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vpadalsv2si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vpadaluv2si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vpadalsv4si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vpadaluv4si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vpmaxsv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpmaxuv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpminsv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpminuv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpmaxsv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpmaxuv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpminsv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpminuv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpmaxsv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpmaxuv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpminsv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpminuv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpmaxfv2sf                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpminfv2sf                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpmaxfv4sf                   (rtx, rtx, rtx);
extern rtx        gen_neon_vpminfv4sf                   (rtx, rtx, rtx);
extern rtx        gen_neon_vrecpsv2sf                   (rtx, rtx, rtx);
extern rtx        gen_neon_vrecpsv4sf                   (rtx, rtx, rtx);
extern rtx        gen_neon_vrsqrtsv2sf                  (rtx, rtx, rtx);
extern rtx        gen_neon_vrsqrtsv4sf                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqabsv8qi                    (rtx, rtx);
extern rtx        gen_neon_vqabsv16qi                   (rtx, rtx);
extern rtx        gen_neon_vqabsv4hi                    (rtx, rtx);
extern rtx        gen_neon_vqabsv8hi                    (rtx, rtx);
extern rtx        gen_neon_vqabsv2si                    (rtx, rtx);
extern rtx        gen_neon_vqabsv4si                    (rtx, rtx);
extern rtx        gen_neon_bswapv4hi                    (rtx, rtx);
extern rtx        gen_neon_bswapv8hi                    (rtx, rtx);
extern rtx        gen_neon_bswapv2si                    (rtx, rtx);
extern rtx        gen_neon_bswapv4si                    (rtx, rtx);
extern rtx        gen_neon_bswapv2di                    (rtx, rtx);
extern rtx        gen_neon_vqnegv8qi                    (rtx, rtx);
extern rtx        gen_neon_vqnegv16qi                   (rtx, rtx);
extern rtx        gen_neon_vqnegv4hi                    (rtx, rtx);
extern rtx        gen_neon_vqnegv8hi                    (rtx, rtx);
extern rtx        gen_neon_vqnegv2si                    (rtx, rtx);
extern rtx        gen_neon_vqnegv4si                    (rtx, rtx);
extern rtx        gen_neon_vclsv8qi                     (rtx, rtx);
extern rtx        gen_neon_vclsv16qi                    (rtx, rtx);
extern rtx        gen_neon_vclsv4hi                     (rtx, rtx);
extern rtx        gen_neon_vclsv8hi                     (rtx, rtx);
extern rtx        gen_neon_vclsv2si                     (rtx, rtx);
extern rtx        gen_neon_vclsv4si                     (rtx, rtx);
extern rtx        gen_clzv8qi2                          (rtx, rtx);
extern rtx        gen_clzv16qi2                         (rtx, rtx);
extern rtx        gen_clzv4hi2                          (rtx, rtx);
extern rtx        gen_clzv8hi2                          (rtx, rtx);
extern rtx        gen_clzv2si2                          (rtx, rtx);
extern rtx        gen_clzv4si2                          (rtx, rtx);
extern rtx        gen_popcountv8qi2                     (rtx, rtx);
extern rtx        gen_popcountv16qi2                    (rtx, rtx);
extern rtx        gen_neon_vrecpev2si                   (rtx, rtx);
extern rtx        gen_neon_vrecpev2sf                   (rtx, rtx);
extern rtx        gen_neon_vrecpev4si                   (rtx, rtx);
extern rtx        gen_neon_vrecpev4sf                   (rtx, rtx);
extern rtx        gen_neon_vrsqrtev2si                  (rtx, rtx);
extern rtx        gen_neon_vrsqrtev2sf                  (rtx, rtx);
extern rtx        gen_neon_vrsqrtev4si                  (rtx, rtx);
extern rtx        gen_neon_vrsqrtev4sf                  (rtx, rtx);
extern rtx        gen_neon_vget_lanev8qi_sext_internal  (rtx, rtx, rtx);
extern rtx        gen_neon_vget_lanev4hi_sext_internal  (rtx, rtx, rtx);
extern rtx        gen_neon_vget_lanev2si_sext_internal  (rtx, rtx, rtx);
extern rtx        gen_neon_vget_lanev2sf_sext_internal  (rtx, rtx, rtx);
extern rtx        gen_neon_vget_lanev8qi_zext_internal  (rtx, rtx, rtx);
extern rtx        gen_neon_vget_lanev4hi_zext_internal  (rtx, rtx, rtx);
extern rtx        gen_neon_vget_lanev2si_zext_internal  (rtx, rtx, rtx);
extern rtx        gen_neon_vget_lanev2sf_zext_internal  (rtx, rtx, rtx);
extern rtx        gen_neon_vget_lanev16qi_sext_internal (rtx, rtx, rtx);
extern rtx        gen_neon_vget_lanev8hi_sext_internal  (rtx, rtx, rtx);
extern rtx        gen_neon_vget_lanev4si_sext_internal  (rtx, rtx, rtx);
extern rtx        gen_neon_vget_lanev4sf_sext_internal  (rtx, rtx, rtx);
extern rtx        gen_neon_vget_lanev16qi_zext_internal (rtx, rtx, rtx);
extern rtx        gen_neon_vget_lanev8hi_zext_internal  (rtx, rtx, rtx);
extern rtx        gen_neon_vget_lanev4si_zext_internal  (rtx, rtx, rtx);
extern rtx        gen_neon_vget_lanev4sf_zext_internal  (rtx, rtx, rtx);
extern rtx        gen_neon_vdup_nv8qi                   (rtx, rtx);
extern rtx        gen_neon_vdup_nv4hi                   (rtx, rtx);
extern rtx        gen_neon_vdup_nv16qi                  (rtx, rtx);
extern rtx        gen_neon_vdup_nv8hi                   (rtx, rtx);
extern rtx        gen_neon_vdup_nv2si                   (rtx, rtx);
extern rtx        gen_neon_vdup_nv2sf                   (rtx, rtx);
extern rtx        gen_neon_vdup_nv4si                   (rtx, rtx);
extern rtx        gen_neon_vdup_nv4sf                   (rtx, rtx);
extern rtx        gen_neon_vdup_nv2di                   (rtx, rtx);
extern rtx        gen_neon_vdup_lanev8qi_internal       (rtx, rtx, rtx);
extern rtx        gen_neon_vdup_lanev16qi_internal      (rtx, rtx, rtx);
extern rtx        gen_neon_vdup_lanev4hi_internal       (rtx, rtx, rtx);
extern rtx        gen_neon_vdup_lanev8hi_internal       (rtx, rtx, rtx);
extern rtx        gen_neon_vdup_lanev2si_internal       (rtx, rtx, rtx);
extern rtx        gen_neon_vdup_lanev4si_internal       (rtx, rtx, rtx);
extern rtx        gen_neon_vdup_lanev2sf_internal       (rtx, rtx, rtx);
extern rtx        gen_neon_vdup_lanev4sf_internal       (rtx, rtx, rtx);
extern rtx        gen_neon_vcombinev8qi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vcombinev4hi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vcombinev2si                 (rtx, rtx, rtx);
extern rtx        gen_neon_vcombinev2sf                 (rtx, rtx, rtx);
extern rtx        gen_neon_vcombinedi                   (rtx, rtx, rtx);
extern rtx        gen_floatv2siv2sf2                    (rtx, rtx);
extern rtx        gen_floatv4siv4sf2                    (rtx, rtx);
extern rtx        gen_floatunsv2siv2sf2                 (rtx, rtx);
extern rtx        gen_floatunsv4siv4sf2                 (rtx, rtx);
extern rtx        gen_fix_truncv2sfv2si2                (rtx, rtx);
extern rtx        gen_fix_truncv4sfv4si2                (rtx, rtx);
extern rtx        gen_fixuns_truncv2sfv2si2             (rtx, rtx);
extern rtx        gen_fixuns_truncv4sfv4si2             (rtx, rtx);
extern rtx        gen_neon_vcvtsv2sf                    (rtx, rtx);
extern rtx        gen_neon_vcvtuv2sf                    (rtx, rtx);
extern rtx        gen_neon_vcvtsv4sf                    (rtx, rtx);
extern rtx        gen_neon_vcvtuv4sf                    (rtx, rtx);
extern rtx        gen_neon_vcvtsv2si                    (rtx, rtx);
extern rtx        gen_neon_vcvtuv2si                    (rtx, rtx);
extern rtx        gen_neon_vcvtsv4si                    (rtx, rtx);
extern rtx        gen_neon_vcvtuv4si                    (rtx, rtx);
extern rtx        gen_neon_vcvtv4sfv4hf                 (rtx, rtx);
extern rtx        gen_neon_vcvtv4hfv4sf                 (rtx, rtx);
extern rtx        gen_neon_vcvts_nv2sf                  (rtx, rtx, rtx);
extern rtx        gen_neon_vcvtu_nv2sf                  (rtx, rtx, rtx);
extern rtx        gen_neon_vcvts_nv4sf                  (rtx, rtx, rtx);
extern rtx        gen_neon_vcvtu_nv4sf                  (rtx, rtx, rtx);
extern rtx        gen_neon_vcvts_nv2si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vcvtu_nv2si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vcvts_nv4si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vcvtu_nv4si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vmovnv8hi                    (rtx, rtx);
extern rtx        gen_neon_vmovnv4si                    (rtx, rtx);
extern rtx        gen_neon_vmovnv2di                    (rtx, rtx);
extern rtx        gen_neon_vqmovnsv8hi                  (rtx, rtx);
extern rtx        gen_neon_vqmovnuv8hi                  (rtx, rtx);
extern rtx        gen_neon_vqmovnsv4si                  (rtx, rtx);
extern rtx        gen_neon_vqmovnuv4si                  (rtx, rtx);
extern rtx        gen_neon_vqmovnsv2di                  (rtx, rtx);
extern rtx        gen_neon_vqmovnuv2di                  (rtx, rtx);
extern rtx        gen_neon_vqmovunv8hi                  (rtx, rtx);
extern rtx        gen_neon_vqmovunv4si                  (rtx, rtx);
extern rtx        gen_neon_vqmovunv2di                  (rtx, rtx);
extern rtx        gen_neon_vmovlsv8qi                   (rtx, rtx);
extern rtx        gen_neon_vmovluv8qi                   (rtx, rtx);
extern rtx        gen_neon_vmovlsv4hi                   (rtx, rtx);
extern rtx        gen_neon_vmovluv4hi                   (rtx, rtx);
extern rtx        gen_neon_vmovlsv2si                   (rtx, rtx);
extern rtx        gen_neon_vmovluv2si                   (rtx, rtx);
extern rtx        gen_neon_vmul_lanev4hi                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmul_lanev2si                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmul_lanev2sf                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmul_lanev8hi                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmul_lanev4si                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmul_lanev4sf                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmulls_lanev4hi              (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmullu_lanev4hi              (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmulls_lanev2si              (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmullu_lanev2si              (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vqdmull_lanev4hi             (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vqdmull_lanev2si             (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vqdmulh_lanev8hi             (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vqrdmulh_lanev8hi            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vqdmulh_lanev4si             (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vqrdmulh_lanev4si            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vqdmulh_lanev4hi             (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vqrdmulh_lanev4hi            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vqdmulh_lanev2si             (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vqrdmulh_lanev2si            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmla_lanev4hi                (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmla_lanev2si                (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmla_lanev2sf                (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmla_lanev8hi                (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmla_lanev4si                (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmla_lanev4sf                (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlals_lanev4hi              (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlalu_lanev4hi              (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlals_lanev2si              (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlalu_lanev2si              (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vqdmlal_lanev4hi             (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vqdmlal_lanev2si             (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmls_lanev4hi                (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmls_lanev2si                (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmls_lanev2sf                (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmls_lanev8hi                (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmls_lanev4si                (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmls_lanev4sf                (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlsls_lanev4hi              (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlslu_lanev4hi              (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlsls_lanev2si              (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlslu_lanev2si              (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vqdmlsl_lanev4hi             (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vqdmlsl_lanev2si             (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vextv8qi                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vextv16qi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vextv4hi                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vextv8hi                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vextv2si                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vextv4si                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vextv2sf                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vextv4sf                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vextdi                       (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vextv2di                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vrev64v8qi                   (rtx, rtx);
extern rtx        gen_neon_vrev64v16qi                  (rtx, rtx);
extern rtx        gen_neon_vrev64v4hi                   (rtx, rtx);
extern rtx        gen_neon_vrev64v8hi                   (rtx, rtx);
extern rtx        gen_neon_vrev64v2si                   (rtx, rtx);
extern rtx        gen_neon_vrev64v4si                   (rtx, rtx);
extern rtx        gen_neon_vrev64v2sf                   (rtx, rtx);
extern rtx        gen_neon_vrev64v4sf                   (rtx, rtx);
extern rtx        gen_neon_vrev64v2di                   (rtx, rtx);
extern rtx        gen_neon_vrev32v8qi                   (rtx, rtx);
extern rtx        gen_neon_vrev32v4hi                   (rtx, rtx);
extern rtx        gen_neon_vrev32v16qi                  (rtx, rtx);
extern rtx        gen_neon_vrev32v8hi                   (rtx, rtx);
extern rtx        gen_neon_vrev16v8qi                   (rtx, rtx);
extern rtx        gen_neon_vrev16v16qi                  (rtx, rtx);
extern rtx        gen_neon_vbslv8qi_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vbslv16qi_internal           (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vbslv4hi_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vbslv8hi_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vbslv2si_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vbslv4si_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vbslv2sf_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vbslv4sf_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vbsldi_internal              (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vbslv2di_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vshlsv8qi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vshluv8qi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vrshlsv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vrshluv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vshlsv16qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vshluv16qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vrshlsv16qi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vrshluv16qi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vshlsv4hi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vshluv4hi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vrshlsv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vrshluv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vshlsv8hi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vshluv8hi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vrshlsv8hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vrshluv8hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vshlsv2si                    (rtx, rtx, rtx);
extern rtx        gen_neon_vshluv2si                    (rtx, rtx, rtx);
extern rtx        gen_neon_vrshlsv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vrshluv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vshlsv4si                    (rtx, rtx, rtx);
extern rtx        gen_neon_vshluv4si                    (rtx, rtx, rtx);
extern rtx        gen_neon_vrshlsv4si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vrshluv4si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vshlsdi                      (rtx, rtx, rtx);
extern rtx        gen_neon_vshludi                      (rtx, rtx, rtx);
extern rtx        gen_neon_vrshlsdi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vrshludi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vshlsv2di                    (rtx, rtx, rtx);
extern rtx        gen_neon_vshluv2di                    (rtx, rtx, rtx);
extern rtx        gen_neon_vrshlsv2di                   (rtx, rtx, rtx);
extern rtx        gen_neon_vrshluv2di                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqshlsv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqshluv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshlsv8qi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshluv8qi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqshlsv16qi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqshluv16qi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshlsv16qi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshluv16qi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vqshlsv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqshluv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshlsv4hi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshluv4hi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqshlsv8hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqshluv8hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshlsv8hi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshluv8hi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqshlsv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqshluv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshlsv2si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshluv2si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqshlsv4si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqshluv4si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshlsv4si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshluv4si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqshlsdi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vqshludi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshlsdi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshludi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vqshlsv2di                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqshluv2di                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshlsv2di                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshluv2di                  (rtx, rtx, rtx);
extern rtx        gen_neon_vshrs_nv8qi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vshru_nv8qi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vrshrs_nv8qi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vrshru_nv8qi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vshrs_nv16qi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vshru_nv16qi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vrshrs_nv16qi                (rtx, rtx, rtx);
extern rtx        gen_neon_vrshru_nv16qi                (rtx, rtx, rtx);
extern rtx        gen_neon_vshrs_nv4hi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vshru_nv4hi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vrshrs_nv4hi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vrshru_nv4hi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vshrs_nv8hi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vshru_nv8hi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vrshrs_nv8hi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vrshru_nv8hi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vshrs_nv2si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vshru_nv2si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vrshrs_nv2si                 (rtx, rtx, rtx);
extern rtx        gen_neon_vrshru_nv2si                 (rtx, rtx, rtx);
extern rtx        gen_neon_vshrs_nv4si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vshru_nv4si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vrshrs_nv4si                 (rtx, rtx, rtx);
extern rtx        gen_neon_vrshru_nv4si                 (rtx, rtx, rtx);
extern rtx        gen_neon_vshrs_ndi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vshru_ndi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vrshrs_ndi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vrshru_ndi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vshrs_nv2di                  (rtx, rtx, rtx);
extern rtx        gen_neon_vshru_nv2di                  (rtx, rtx, rtx);
extern rtx        gen_neon_vrshrs_nv2di                 (rtx, rtx, rtx);
extern rtx        gen_neon_vrshru_nv2di                 (rtx, rtx, rtx);
extern rtx        gen_neon_vshrn_nv8hi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vrshrn_nv8hi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vshrn_nv4si                  (rtx, rtx, rtx);
extern rtx        gen_neon_vrshrn_nv4si                 (rtx, rtx, rtx);
extern rtx        gen_neon_vshrn_nv2di                  (rtx, rtx, rtx);
extern rtx        gen_neon_vrshrn_nv2di                 (rtx, rtx, rtx);
extern rtx        gen_neon_vqshrns_nv8hi                (rtx, rtx, rtx);
extern rtx        gen_neon_vqshrnu_nv8hi                (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshrns_nv8hi               (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshrnu_nv8hi               (rtx, rtx, rtx);
extern rtx        gen_neon_vqshrns_nv4si                (rtx, rtx, rtx);
extern rtx        gen_neon_vqshrnu_nv4si                (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshrns_nv4si               (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshrnu_nv4si               (rtx, rtx, rtx);
extern rtx        gen_neon_vqshrns_nv2di                (rtx, rtx, rtx);
extern rtx        gen_neon_vqshrnu_nv2di                (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshrns_nv2di               (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshrnu_nv2di               (rtx, rtx, rtx);
extern rtx        gen_neon_vqshrun_nv8hi                (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshrun_nv8hi               (rtx, rtx, rtx);
extern rtx        gen_neon_vqshrun_nv4si                (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshrun_nv4si               (rtx, rtx, rtx);
extern rtx        gen_neon_vqshrun_nv2di                (rtx, rtx, rtx);
extern rtx        gen_neon_vqrshrun_nv2di               (rtx, rtx, rtx);
extern rtx        gen_neon_vshl_nv8qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vshl_nv16qi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vshl_nv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vshl_nv8hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vshl_nv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vshl_nv4si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vshl_ndi                     (rtx, rtx, rtx);
extern rtx        gen_neon_vshl_nv2di                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqshl_s_nv8qi                (rtx, rtx, rtx);
extern rtx        gen_neon_vqshl_u_nv8qi                (rtx, rtx, rtx);
extern rtx        gen_neon_vqshl_s_nv16qi               (rtx, rtx, rtx);
extern rtx        gen_neon_vqshl_u_nv16qi               (rtx, rtx, rtx);
extern rtx        gen_neon_vqshl_s_nv4hi                (rtx, rtx, rtx);
extern rtx        gen_neon_vqshl_u_nv4hi                (rtx, rtx, rtx);
extern rtx        gen_neon_vqshl_s_nv8hi                (rtx, rtx, rtx);
extern rtx        gen_neon_vqshl_u_nv8hi                (rtx, rtx, rtx);
extern rtx        gen_neon_vqshl_s_nv2si                (rtx, rtx, rtx);
extern rtx        gen_neon_vqshl_u_nv2si                (rtx, rtx, rtx);
extern rtx        gen_neon_vqshl_s_nv4si                (rtx, rtx, rtx);
extern rtx        gen_neon_vqshl_u_nv4si                (rtx, rtx, rtx);
extern rtx        gen_neon_vqshl_s_ndi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqshl_u_ndi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vqshl_s_nv2di                (rtx, rtx, rtx);
extern rtx        gen_neon_vqshl_u_nv2di                (rtx, rtx, rtx);
extern rtx        gen_neon_vqshlu_nv8qi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vqshlu_nv16qi                (rtx, rtx, rtx);
extern rtx        gen_neon_vqshlu_nv4hi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vqshlu_nv8hi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vqshlu_nv2si                 (rtx, rtx, rtx);
extern rtx        gen_neon_vqshlu_nv4si                 (rtx, rtx, rtx);
extern rtx        gen_neon_vqshlu_ndi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vqshlu_nv2di                 (rtx, rtx, rtx);
extern rtx        gen_neon_vshlls_nv8qi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vshllu_nv8qi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vshlls_nv4hi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vshllu_nv4hi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vshlls_nv2si                 (rtx, rtx, rtx);
extern rtx        gen_neon_vshllu_nv2si                 (rtx, rtx, rtx);
extern rtx        gen_neon_vsras_nv8qi                  (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsrau_nv8qi                  (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vrsras_nv8qi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vrsrau_nv8qi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsras_nv16qi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsrau_nv16qi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vrsras_nv16qi                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vrsrau_nv16qi                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsras_nv4hi                  (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsrau_nv4hi                  (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vrsras_nv4hi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vrsrau_nv4hi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsras_nv8hi                  (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsrau_nv8hi                  (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vrsras_nv8hi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vrsrau_nv8hi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsras_nv2si                  (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsrau_nv2si                  (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vrsras_nv2si                 (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vrsrau_nv2si                 (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsras_nv4si                  (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsrau_nv4si                  (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vrsras_nv4si                 (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vrsrau_nv4si                 (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsras_ndi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsrau_ndi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vrsras_ndi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vrsrau_ndi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsras_nv2di                  (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsrau_nv2di                  (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vrsras_nv2di                 (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vrsrau_nv2di                 (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsri_nv8qi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsri_nv16qi                  (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsri_nv4hi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsri_nv8hi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsri_nv2si                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsri_nv4si                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsri_ndi                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsri_nv2di                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsli_nv8qi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsli_nv16qi                  (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsli_nv4hi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsli_nv8hi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsli_nv2si                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsli_nv4si                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsli_ndi                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsli_nv2di                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vtbl1v8qi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vtbl2v8qi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vtbl3v8qi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vtbl4v8qi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vtbl1v16qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vtbl2v16qi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vcombinev16qi                (rtx, rtx, rtx);
extern rtx        gen_neon_vtbx1v8qi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vtbx2v8qi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vtbx3v8qi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vtbx4v8qi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld1v8qi                     (rtx, rtx);
extern rtx        gen_neon_vld1v16qi                    (rtx, rtx);
extern rtx        gen_neon_vld1v4hi                     (rtx, rtx);
extern rtx        gen_neon_vld1v8hi                     (rtx, rtx);
extern rtx        gen_neon_vld1v2si                     (rtx, rtx);
extern rtx        gen_neon_vld1v4si                     (rtx, rtx);
extern rtx        gen_neon_vld1v2sf                     (rtx, rtx);
extern rtx        gen_neon_vld1v4sf                     (rtx, rtx);
extern rtx        gen_neon_vld1di                       (rtx, rtx);
extern rtx        gen_neon_vld1v2di                     (rtx, rtx);
extern rtx        gen_neon_vld1_lanev8qi                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld1_lanev4hi                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld1_lanev2si                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld1_lanev2sf                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld1_lanedi                  (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld1_lanev16qi               (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld1_lanev8hi                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld1_lanev4si                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld1_lanev4sf                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld1_lanev2di                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld1_dupv8qi                 (rtx, rtx);
extern rtx        gen_neon_vld1_dupv4hi                 (rtx, rtx);
extern rtx        gen_neon_vld1_dupv2si                 (rtx, rtx);
extern rtx        gen_neon_vld1_dupv2sf                 (rtx, rtx);
extern rtx        gen_neon_vld1_dupv16qi                (rtx, rtx);
extern rtx        gen_neon_vld1_dupv8hi                 (rtx, rtx);
extern rtx        gen_neon_vld1_dupv4si                 (rtx, rtx);
extern rtx        gen_neon_vld1_dupv4sf                 (rtx, rtx);
extern rtx        gen_neon_vld1_dupv2di                 (rtx, rtx);
extern rtx        gen_neon_vst1v8qi                     (rtx, rtx);
extern rtx        gen_neon_vst1v16qi                    (rtx, rtx);
extern rtx        gen_neon_vst1v4hi                     (rtx, rtx);
extern rtx        gen_neon_vst1v8hi                     (rtx, rtx);
extern rtx        gen_neon_vst1v2si                     (rtx, rtx);
extern rtx        gen_neon_vst1v4si                     (rtx, rtx);
extern rtx        gen_neon_vst1v2sf                     (rtx, rtx);
extern rtx        gen_neon_vst1v4sf                     (rtx, rtx);
extern rtx        gen_neon_vst1di                       (rtx, rtx);
extern rtx        gen_neon_vst1v2di                     (rtx, rtx);
extern rtx        gen_neon_vst1_lanev8qi                (rtx, rtx, rtx);
extern rtx        gen_neon_vst1_lanev4hi                (rtx, rtx, rtx);
extern rtx        gen_neon_vst1_lanev2si                (rtx, rtx, rtx);
extern rtx        gen_neon_vst1_lanev2sf                (rtx, rtx, rtx);
extern rtx        gen_neon_vst1_lanedi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vst1_lanev16qi               (rtx, rtx, rtx);
extern rtx        gen_neon_vst1_lanev8hi                (rtx, rtx, rtx);
extern rtx        gen_neon_vst1_lanev4si                (rtx, rtx, rtx);
extern rtx        gen_neon_vst1_lanev4sf                (rtx, rtx, rtx);
extern rtx        gen_neon_vst1_lanev2di                (rtx, rtx, rtx);
extern rtx        gen_neon_vld2v8qi                     (rtx, rtx);
extern rtx        gen_neon_vld2v4hi                     (rtx, rtx);
extern rtx        gen_neon_vld2v2si                     (rtx, rtx);
extern rtx        gen_neon_vld2v2sf                     (rtx, rtx);
extern rtx        gen_neon_vld2di                       (rtx, rtx);
extern rtx        gen_neon_vld2v16qi                    (rtx, rtx);
extern rtx        gen_neon_vld2v8hi                     (rtx, rtx);
extern rtx        gen_neon_vld2v4si                     (rtx, rtx);
extern rtx        gen_neon_vld2v4sf                     (rtx, rtx);
extern rtx        gen_neon_vld2_lanev8qi                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld2_lanev4hi                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld2_lanev2si                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld2_lanev2sf                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld2_lanev8hi                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld2_lanev4si                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld2_lanev4sf                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld2_dupv8qi                 (rtx, rtx);
extern rtx        gen_neon_vld2_dupv4hi                 (rtx, rtx);
extern rtx        gen_neon_vld2_dupv2si                 (rtx, rtx);
extern rtx        gen_neon_vld2_dupv2sf                 (rtx, rtx);
extern rtx        gen_neon_vld2_dupdi                   (rtx, rtx);
extern rtx        gen_neon_vst2v8qi                     (rtx, rtx);
extern rtx        gen_neon_vst2v4hi                     (rtx, rtx);
extern rtx        gen_neon_vst2v2si                     (rtx, rtx);
extern rtx        gen_neon_vst2v2sf                     (rtx, rtx);
extern rtx        gen_neon_vst2di                       (rtx, rtx);
extern rtx        gen_neon_vst2v16qi                    (rtx, rtx);
extern rtx        gen_neon_vst2v8hi                     (rtx, rtx);
extern rtx        gen_neon_vst2v4si                     (rtx, rtx);
extern rtx        gen_neon_vst2v4sf                     (rtx, rtx);
extern rtx        gen_neon_vst2_lanev8qi                (rtx, rtx, rtx);
extern rtx        gen_neon_vst2_lanev4hi                (rtx, rtx, rtx);
extern rtx        gen_neon_vst2_lanev2si                (rtx, rtx, rtx);
extern rtx        gen_neon_vst2_lanev2sf                (rtx, rtx, rtx);
extern rtx        gen_neon_vst2_lanev8hi                (rtx, rtx, rtx);
extern rtx        gen_neon_vst2_lanev4si                (rtx, rtx, rtx);
extern rtx        gen_neon_vst2_lanev4sf                (rtx, rtx, rtx);
extern rtx        gen_neon_vld3v8qi                     (rtx, rtx);
extern rtx        gen_neon_vld3v4hi                     (rtx, rtx);
extern rtx        gen_neon_vld3v2si                     (rtx, rtx);
extern rtx        gen_neon_vld3v2sf                     (rtx, rtx);
extern rtx        gen_neon_vld3di                       (rtx, rtx);
extern rtx        gen_neon_vld3qav16qi                  (rtx, rtx);
extern rtx        gen_neon_vld3qav8hi                   (rtx, rtx);
extern rtx        gen_neon_vld3qav4si                   (rtx, rtx);
extern rtx        gen_neon_vld3qav4sf                   (rtx, rtx);
extern rtx        gen_neon_vld3qbv16qi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vld3qbv8hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vld3qbv4si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vld3qbv4sf                   (rtx, rtx, rtx);
extern rtx        gen_neon_vld3_lanev8qi                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld3_lanev4hi                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld3_lanev2si                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld3_lanev2sf                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld3_lanev8hi                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld3_lanev4si                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld3_lanev4sf                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld3_dupv8qi                 (rtx, rtx);
extern rtx        gen_neon_vld3_dupv4hi                 (rtx, rtx);
extern rtx        gen_neon_vld3_dupv2si                 (rtx, rtx);
extern rtx        gen_neon_vld3_dupv2sf                 (rtx, rtx);
extern rtx        gen_neon_vld3_dupdi                   (rtx, rtx);
extern rtx        gen_neon_vst3v8qi                     (rtx, rtx);
extern rtx        gen_neon_vst3v4hi                     (rtx, rtx);
extern rtx        gen_neon_vst3v2si                     (rtx, rtx);
extern rtx        gen_neon_vst3v2sf                     (rtx, rtx);
extern rtx        gen_neon_vst3di                       (rtx, rtx);
extern rtx        gen_neon_vst3qav16qi                  (rtx, rtx);
extern rtx        gen_neon_vst3qav8hi                   (rtx, rtx);
extern rtx        gen_neon_vst3qav4si                   (rtx, rtx);
extern rtx        gen_neon_vst3qav4sf                   (rtx, rtx);
extern rtx        gen_neon_vst3qbv16qi                  (rtx, rtx);
extern rtx        gen_neon_vst3qbv8hi                   (rtx, rtx);
extern rtx        gen_neon_vst3qbv4si                   (rtx, rtx);
extern rtx        gen_neon_vst3qbv4sf                   (rtx, rtx);
extern rtx        gen_neon_vst3_lanev8qi                (rtx, rtx, rtx);
extern rtx        gen_neon_vst3_lanev4hi                (rtx, rtx, rtx);
extern rtx        gen_neon_vst3_lanev2si                (rtx, rtx, rtx);
extern rtx        gen_neon_vst3_lanev2sf                (rtx, rtx, rtx);
extern rtx        gen_neon_vst3_lanev8hi                (rtx, rtx, rtx);
extern rtx        gen_neon_vst3_lanev4si                (rtx, rtx, rtx);
extern rtx        gen_neon_vst3_lanev4sf                (rtx, rtx, rtx);
extern rtx        gen_neon_vld4v8qi                     (rtx, rtx);
extern rtx        gen_neon_vld4v4hi                     (rtx, rtx);
extern rtx        gen_neon_vld4v2si                     (rtx, rtx);
extern rtx        gen_neon_vld4v2sf                     (rtx, rtx);
extern rtx        gen_neon_vld4di                       (rtx, rtx);
extern rtx        gen_neon_vld4qav16qi                  (rtx, rtx);
extern rtx        gen_neon_vld4qav8hi                   (rtx, rtx);
extern rtx        gen_neon_vld4qav4si                   (rtx, rtx);
extern rtx        gen_neon_vld4qav4sf                   (rtx, rtx);
extern rtx        gen_neon_vld4qbv16qi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vld4qbv8hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vld4qbv4si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vld4qbv4sf                   (rtx, rtx, rtx);
extern rtx        gen_neon_vld4_lanev8qi                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld4_lanev4hi                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld4_lanev2si                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld4_lanev2sf                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld4_lanev8hi                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld4_lanev4si                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld4_lanev4sf                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vld4_dupv8qi                 (rtx, rtx);
extern rtx        gen_neon_vld4_dupv4hi                 (rtx, rtx);
extern rtx        gen_neon_vld4_dupv2si                 (rtx, rtx);
extern rtx        gen_neon_vld4_dupv2sf                 (rtx, rtx);
extern rtx        gen_neon_vld4_dupdi                   (rtx, rtx);
extern rtx        gen_neon_vst4v8qi                     (rtx, rtx);
extern rtx        gen_neon_vst4v4hi                     (rtx, rtx);
extern rtx        gen_neon_vst4v2si                     (rtx, rtx);
extern rtx        gen_neon_vst4v2sf                     (rtx, rtx);
extern rtx        gen_neon_vst4di                       (rtx, rtx);
extern rtx        gen_neon_vst4qav16qi                  (rtx, rtx);
extern rtx        gen_neon_vst4qav8hi                   (rtx, rtx);
extern rtx        gen_neon_vst4qav4si                   (rtx, rtx);
extern rtx        gen_neon_vst4qav4sf                   (rtx, rtx);
extern rtx        gen_neon_vst4qbv16qi                  (rtx, rtx);
extern rtx        gen_neon_vst4qbv8hi                   (rtx, rtx);
extern rtx        gen_neon_vst4qbv4si                   (rtx, rtx);
extern rtx        gen_neon_vst4qbv4sf                   (rtx, rtx);
extern rtx        gen_neon_vst4_lanev8qi                (rtx, rtx, rtx);
extern rtx        gen_neon_vst4_lanev4hi                (rtx, rtx, rtx);
extern rtx        gen_neon_vst4_lanev2si                (rtx, rtx, rtx);
extern rtx        gen_neon_vst4_lanev2sf                (rtx, rtx, rtx);
extern rtx        gen_neon_vst4_lanev8hi                (rtx, rtx, rtx);
extern rtx        gen_neon_vst4_lanev4si                (rtx, rtx, rtx);
extern rtx        gen_neon_vst4_lanev4sf                (rtx, rtx, rtx);
extern rtx        gen_neon_vec_unpacks_lo_v16qi         (rtx, rtx, rtx);
extern rtx        gen_neon_vec_unpacku_lo_v16qi         (rtx, rtx, rtx);
extern rtx        gen_neon_vec_unpacks_lo_v8hi          (rtx, rtx, rtx);
extern rtx        gen_neon_vec_unpacku_lo_v8hi          (rtx, rtx, rtx);
extern rtx        gen_neon_vec_unpacks_lo_v4si          (rtx, rtx, rtx);
extern rtx        gen_neon_vec_unpacku_lo_v4si          (rtx, rtx, rtx);
extern rtx        gen_neon_vec_unpacks_hi_v16qi         (rtx, rtx, rtx);
extern rtx        gen_neon_vec_unpacku_hi_v16qi         (rtx, rtx, rtx);
extern rtx        gen_neon_vec_unpacks_hi_v8hi          (rtx, rtx, rtx);
extern rtx        gen_neon_vec_unpacku_hi_v8hi          (rtx, rtx, rtx);
extern rtx        gen_neon_vec_unpacks_hi_v4si          (rtx, rtx, rtx);
extern rtx        gen_neon_vec_unpacku_hi_v4si          (rtx, rtx, rtx);
extern rtx        gen_neon_vec_smult_lo_v16qi           (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vec_umult_lo_v16qi           (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vec_smult_lo_v8hi            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vec_umult_lo_v8hi            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vec_smult_lo_v4si            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vec_umult_lo_v4si            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vec_smult_hi_v16qi           (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vec_umult_hi_v16qi           (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vec_smult_hi_v8hi            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vec_umult_hi_v8hi            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vec_smult_hi_v4si            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vec_umult_hi_v4si            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vec_sshiftl_v8qi             (rtx, rtx, rtx);
extern rtx        gen_neon_vec_ushiftl_v8qi             (rtx, rtx, rtx);
extern rtx        gen_neon_vec_sshiftl_v4hi             (rtx, rtx, rtx);
extern rtx        gen_neon_vec_ushiftl_v4hi             (rtx, rtx, rtx);
extern rtx        gen_neon_vec_sshiftl_v2si             (rtx, rtx, rtx);
extern rtx        gen_neon_vec_ushiftl_v2si             (rtx, rtx, rtx);
extern rtx        gen_neon_unpacks_v8qi                 (rtx, rtx);
extern rtx        gen_neon_unpacku_v8qi                 (rtx, rtx);
extern rtx        gen_neon_unpacks_v4hi                 (rtx, rtx);
extern rtx        gen_neon_unpacku_v4hi                 (rtx, rtx);
extern rtx        gen_neon_unpacks_v2si                 (rtx, rtx);
extern rtx        gen_neon_unpacku_v2si                 (rtx, rtx);
extern rtx        gen_neon_vec_smult_v8qi               (rtx, rtx, rtx);
extern rtx        gen_neon_vec_umult_v8qi               (rtx, rtx, rtx);
extern rtx        gen_neon_vec_smult_v4hi               (rtx, rtx, rtx);
extern rtx        gen_neon_vec_umult_v4hi               (rtx, rtx, rtx);
extern rtx        gen_neon_vec_smult_v2si               (rtx, rtx, rtx);
extern rtx        gen_neon_vec_umult_v2si               (rtx, rtx, rtx);
extern rtx        gen_vec_pack_trunc_v8hi               (rtx, rtx, rtx);
extern rtx        gen_vec_pack_trunc_v4si               (rtx, rtx, rtx);
extern rtx        gen_vec_pack_trunc_v2di               (rtx, rtx, rtx);
extern rtx        gen_neon_vec_pack_trunc_v8hi          (rtx, rtx);
extern rtx        gen_neon_vec_pack_trunc_v4si          (rtx, rtx);
extern rtx        gen_neon_vec_pack_trunc_v2di          (rtx, rtx);
extern rtx        gen_neon_vabdv8qi_2                   (rtx, rtx, rtx);
extern rtx        gen_neon_vabdv16qi_2                  (rtx, rtx, rtx);
extern rtx        gen_neon_vabdv4hi_2                   (rtx, rtx, rtx);
extern rtx        gen_neon_vabdv8hi_2                   (rtx, rtx, rtx);
extern rtx        gen_neon_vabdv2si_2                   (rtx, rtx, rtx);
extern rtx        gen_neon_vabdv4si_2                   (rtx, rtx, rtx);
extern rtx        gen_neon_vabdv2sf_2                   (rtx, rtx, rtx);
extern rtx        gen_neon_vabdv4sf_2                   (rtx, rtx, rtx);
extern rtx        gen_neon_vabdv2di_2                   (rtx, rtx, rtx);
extern rtx        gen_neon_vabdv8qi_3                   (rtx, rtx, rtx);
extern rtx        gen_neon_vabdv16qi_3                  (rtx, rtx, rtx);
extern rtx        gen_neon_vabdv4hi_3                   (rtx, rtx, rtx);
extern rtx        gen_neon_vabdv8hi_3                   (rtx, rtx, rtx);
extern rtx        gen_neon_vabdv2si_3                   (rtx, rtx, rtx);
extern rtx        gen_neon_vabdv4si_3                   (rtx, rtx, rtx);
extern rtx        gen_neon_vabdv2sf_3                   (rtx, rtx, rtx);
extern rtx        gen_neon_vabdv4sf_3                   (rtx, rtx, rtx);
extern rtx        gen_neon_vabdv2di_3                   (rtx, rtx, rtx);
extern rtx        gen_crypto_aesmc                      (rtx, rtx);
extern rtx        gen_crypto_aesimc                     (rtx, rtx);
extern rtx        gen_crypto_aesd                       (rtx, rtx, rtx);
extern rtx        gen_crypto_aese                       (rtx, rtx, rtx);
extern rtx        gen_crypto_sha1su1                    (rtx, rtx, rtx);
extern rtx        gen_crypto_sha256su0                  (rtx, rtx, rtx);
extern rtx        gen_crypto_sha1su0                    (rtx, rtx, rtx, rtx);
extern rtx        gen_crypto_sha256h                    (rtx, rtx, rtx, rtx);
extern rtx        gen_crypto_sha256h2                   (rtx, rtx, rtx, rtx);
extern rtx        gen_crypto_sha256su1                  (rtx, rtx, rtx, rtx);
extern rtx        gen_crypto_sha1h                      (rtx, rtx, rtx);
extern rtx        gen_crypto_vmullp64                   (rtx, rtx, rtx);
extern rtx        gen_crypto_sha1c                      (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_crypto_sha1m                      (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_crypto_sha1p                      (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_loadqi                     (rtx, rtx, rtx);
extern rtx        gen_atomic_loadhi                     (rtx, rtx, rtx);
extern rtx        gen_atomic_loadsi                     (rtx, rtx, rtx);
extern rtx        gen_atomic_storeqi                    (rtx, rtx, rtx);
extern rtx        gen_atomic_storehi                    (rtx, rtx, rtx);
extern rtx        gen_atomic_storesi                    (rtx, rtx, rtx);
extern rtx        gen_arm_atomic_loaddi2_ldrd           (rtx, rtx);
extern rtx        gen_atomic_compare_and_swapqi_1       (rtx, rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swaphi_1       (rtx, rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swapsi_1       (rtx, rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swapdi_1       (rtx, rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_exchangeqi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_exchangehi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_exchangesi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_exchangedi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_addqi                      (rtx, rtx, rtx);
extern rtx        gen_atomic_subqi                      (rtx, rtx, rtx);
extern rtx        gen_atomic_orqi                       (rtx, rtx, rtx);
extern rtx        gen_atomic_xorqi                      (rtx, rtx, rtx);
extern rtx        gen_atomic_andqi                      (rtx, rtx, rtx);
extern rtx        gen_atomic_addhi                      (rtx, rtx, rtx);
extern rtx        gen_atomic_subhi                      (rtx, rtx, rtx);
extern rtx        gen_atomic_orhi                       (rtx, rtx, rtx);
extern rtx        gen_atomic_xorhi                      (rtx, rtx, rtx);
extern rtx        gen_atomic_andhi                      (rtx, rtx, rtx);
extern rtx        gen_atomic_addsi                      (rtx, rtx, rtx);
extern rtx        gen_atomic_subsi                      (rtx, rtx, rtx);
extern rtx        gen_atomic_orsi                       (rtx, rtx, rtx);
extern rtx        gen_atomic_xorsi                      (rtx, rtx, rtx);
extern rtx        gen_atomic_andsi                      (rtx, rtx, rtx);
extern rtx        gen_atomic_adddi                      (rtx, rtx, rtx);
extern rtx        gen_atomic_subdi                      (rtx, rtx, rtx);
extern rtx        gen_atomic_ordi                       (rtx, rtx, rtx);
extern rtx        gen_atomic_xordi                      (rtx, rtx, rtx);
extern rtx        gen_atomic_anddi                      (rtx, rtx, rtx);
extern rtx        gen_atomic_nandqi                     (rtx, rtx, rtx);
extern rtx        gen_atomic_nandhi                     (rtx, rtx, rtx);
extern rtx        gen_atomic_nandsi                     (rtx, rtx, rtx);
extern rtx        gen_atomic_nanddi                     (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_addqi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_subqi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_orqi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_xorqi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_andqi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_addhi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_subhi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_orhi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_xorhi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_andhi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_addsi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_subsi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_orsi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_xorsi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_andsi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_adddi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_subdi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_ordi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_xordi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_anddi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_nandqi               (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_nandhi               (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_nandsi               (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_nanddi               (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_add_fetchqi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_sub_fetchqi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_or_fetchqi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_xor_fetchqi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_and_fetchqi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_add_fetchhi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_sub_fetchhi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_or_fetchhi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_xor_fetchhi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_and_fetchhi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_add_fetchsi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_sub_fetchsi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_or_fetchsi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_xor_fetchsi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_and_fetchsi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_add_fetchdi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_sub_fetchdi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_or_fetchdi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_xor_fetchdi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_and_fetchdi                (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_nand_fetchqi               (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_nand_fetchhi               (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_nand_fetchsi               (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_nand_fetchdi               (rtx, rtx, rtx, rtx);
extern rtx        gen_arm_load_exclusiveqi              (rtx, rtx);
extern rtx        gen_arm_load_exclusivehi              (rtx, rtx);
extern rtx        gen_arm_load_acquire_exclusiveqi      (rtx, rtx);
extern rtx        gen_arm_load_acquire_exclusivehi      (rtx, rtx);
extern rtx        gen_arm_load_exclusivesi              (rtx, rtx);
extern rtx        gen_arm_load_acquire_exclusivesi      (rtx, rtx);
extern rtx        gen_arm_load_exclusivedi              (rtx, rtx);
extern rtx        gen_arm_load_acquire_exclusivedi      (rtx, rtx);
extern rtx        gen_arm_store_exclusiveqi             (rtx, rtx, rtx);
extern rtx        gen_arm_store_exclusivehi             (rtx, rtx, rtx);
extern rtx        gen_arm_store_exclusivesi             (rtx, rtx, rtx);
extern rtx        gen_arm_store_exclusivedi             (rtx, rtx, rtx);
extern rtx        gen_arm_store_release_exclusivedi     (rtx, rtx, rtx);
extern rtx        gen_arm_store_release_exclusiveqi     (rtx, rtx, rtx);
extern rtx        gen_arm_store_release_exclusivehi     (rtx, rtx, rtx);
extern rtx        gen_arm_store_release_exclusivesi     (rtx, rtx, rtx);
extern rtx        gen_addqq3                            (rtx, rtx, rtx);
extern rtx        gen_addhq3                            (rtx, rtx, rtx);
extern rtx        gen_addsq3                            (rtx, rtx, rtx);
extern rtx        gen_adduqq3                           (rtx, rtx, rtx);
extern rtx        gen_adduhq3                           (rtx, rtx, rtx);
extern rtx        gen_addusq3                           (rtx, rtx, rtx);
extern rtx        gen_addha3                            (rtx, rtx, rtx);
extern rtx        gen_addsa3                            (rtx, rtx, rtx);
extern rtx        gen_adduha3                           (rtx, rtx, rtx);
extern rtx        gen_addusa3                           (rtx, rtx, rtx);
extern rtx        gen_addv4qq3                          (rtx, rtx, rtx);
extern rtx        gen_addv2hq3                          (rtx, rtx, rtx);
extern rtx        gen_addv2ha3                          (rtx, rtx, rtx);
extern rtx        gen_usaddv4uqq3                       (rtx, rtx, rtx);
extern rtx        gen_usaddv2uhq3                       (rtx, rtx, rtx);
extern rtx        gen_usadduqq3                         (rtx, rtx, rtx);
extern rtx        gen_usadduhq3                         (rtx, rtx, rtx);
extern rtx        gen_usaddv2uha3                       (rtx, rtx, rtx);
extern rtx        gen_usadduha3                         (rtx, rtx, rtx);
extern rtx        gen_ssaddv4qq3                        (rtx, rtx, rtx);
extern rtx        gen_ssaddv2hq3                        (rtx, rtx, rtx);
extern rtx        gen_ssaddqq3                          (rtx, rtx, rtx);
extern rtx        gen_ssaddhq3                          (rtx, rtx, rtx);
extern rtx        gen_ssaddv2ha3                        (rtx, rtx, rtx);
extern rtx        gen_ssaddha3                          (rtx, rtx, rtx);
extern rtx        gen_ssaddsq3                          (rtx, rtx, rtx);
extern rtx        gen_ssaddsa3                          (rtx, rtx, rtx);
extern rtx        gen_subqq3                            (rtx, rtx, rtx);
extern rtx        gen_subhq3                            (rtx, rtx, rtx);
extern rtx        gen_subsq3                            (rtx, rtx, rtx);
extern rtx        gen_subuqq3                           (rtx, rtx, rtx);
extern rtx        gen_subuhq3                           (rtx, rtx, rtx);
extern rtx        gen_subusq3                           (rtx, rtx, rtx);
extern rtx        gen_subha3                            (rtx, rtx, rtx);
extern rtx        gen_subsa3                            (rtx, rtx, rtx);
extern rtx        gen_subuha3                           (rtx, rtx, rtx);
extern rtx        gen_subusa3                           (rtx, rtx, rtx);
extern rtx        gen_subv4qq3                          (rtx, rtx, rtx);
extern rtx        gen_subv2hq3                          (rtx, rtx, rtx);
extern rtx        gen_subv2ha3                          (rtx, rtx, rtx);
extern rtx        gen_ussubv4uqq3                       (rtx, rtx, rtx);
extern rtx        gen_ussubv2uhq3                       (rtx, rtx, rtx);
extern rtx        gen_ussubuqq3                         (rtx, rtx, rtx);
extern rtx        gen_ussubuhq3                         (rtx, rtx, rtx);
extern rtx        gen_ussubv2uha3                       (rtx, rtx, rtx);
extern rtx        gen_ussubuha3                         (rtx, rtx, rtx);
extern rtx        gen_sssubv4qq3                        (rtx, rtx, rtx);
extern rtx        gen_sssubv2hq3                        (rtx, rtx, rtx);
extern rtx        gen_sssubqq3                          (rtx, rtx, rtx);
extern rtx        gen_sssubhq3                          (rtx, rtx, rtx);
extern rtx        gen_sssubv2ha3                        (rtx, rtx, rtx);
extern rtx        gen_sssubha3                          (rtx, rtx, rtx);
extern rtx        gen_sssubsq3                          (rtx, rtx, rtx);
extern rtx        gen_sssubsa3                          (rtx, rtx, rtx);
extern rtx        gen_ssmulsa3                          (rtx, rtx, rtx);
extern rtx        gen_usmulusa3                         (rtx, rtx, rtx);
extern rtx        gen_arm_ssatsihi_shift                (rtx, rtx, rtx, rtx);
extern rtx        gen_arm_usatsihi                      (rtx, rtx);
extern rtx        gen_adddi3                            (rtx, rtx, rtx);
extern rtx        gen_addsi3                            (rtx, rtx, rtx);
extern rtx        gen_addsf3                            (rtx, rtx, rtx);
extern rtx        gen_adddf3                            (rtx, rtx, rtx);
extern rtx        gen_subdi3                            (rtx, rtx, rtx);
extern rtx        gen_subsi3                            (rtx, rtx, rtx);
extern rtx        gen_subsf3                            (rtx, rtx, rtx);
extern rtx        gen_subdf3                            (rtx, rtx, rtx);
extern rtx        gen_mulhi3                            (rtx, rtx, rtx);
extern rtx        gen_mulsi3                            (rtx, rtx, rtx);
extern rtx        gen_maddsidi4                         (rtx, rtx, rtx, rtx);
extern rtx        gen_mulsidi3                          (rtx, rtx, rtx);
extern rtx        gen_umulsidi3                         (rtx, rtx, rtx);
extern rtx        gen_umaddsidi4                        (rtx, rtx, rtx, rtx);
extern rtx        gen_smulsi3_highpart                  (rtx, rtx, rtx);
extern rtx        gen_umulsi3_highpart                  (rtx, rtx, rtx);
extern rtx        gen_mulsf3                            (rtx, rtx, rtx);
extern rtx        gen_muldf3                            (rtx, rtx, rtx);
extern rtx        gen_divsf3                            (rtx, rtx, rtx);
extern rtx        gen_divdf3                            (rtx, rtx, rtx);
extern rtx        gen_anddi3                            (rtx, rtx, rtx);
extern rtx        gen_andsi3                            (rtx, rtx, rtx);
extern rtx        gen_insv                              (rtx, rtx, rtx, rtx);
extern rtx        gen_iordi3                            (rtx, rtx, rtx);
extern rtx        gen_iorsi3                            (rtx, rtx, rtx);
extern rtx        gen_xordi3                            (rtx, rtx, rtx);
extern rtx        gen_xorsi3                            (rtx, rtx, rtx);
extern rtx        gen_smaxsi3                           (rtx, rtx, rtx);
extern rtx        gen_sminsi3                           (rtx, rtx, rtx);
extern rtx        gen_umaxsi3                           (rtx, rtx, rtx);
extern rtx        gen_uminsi3                           (rtx, rtx, rtx);
extern rtx        gen_ashldi3                           (rtx, rtx, rtx);
extern rtx        gen_ashlsi3                           (rtx, rtx, rtx);
extern rtx        gen_ashrdi3                           (rtx, rtx, rtx);
extern rtx        gen_ashrsi3                           (rtx, rtx, rtx);
extern rtx        gen_lshrdi3                           (rtx, rtx, rtx);
extern rtx        gen_lshrsi3                           (rtx, rtx, rtx);
extern rtx        gen_rotlsi3                           (rtx, rtx, rtx);
extern rtx        gen_rotrsi3                           (rtx, rtx, rtx);
extern rtx        gen_extzv                             (rtx, rtx, rtx, rtx);
extern rtx        gen_extzv_t1                          (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_extv                              (rtx, rtx, rtx, rtx);
extern rtx        gen_extv_regsi                        (rtx, rtx, rtx, rtx);
extern rtx        gen_negdi2                            (rtx, rtx);
extern rtx        gen_negsi2                            (rtx, rtx);
extern rtx        gen_negsf2                            (rtx, rtx);
extern rtx        gen_negdf2                            (rtx, rtx);
extern rtx        gen_abssi2                            (rtx, rtx);
extern rtx        gen_abssf2                            (rtx, rtx);
extern rtx        gen_absdf2                            (rtx, rtx);
extern rtx        gen_sqrtsf2                           (rtx, rtx);
extern rtx        gen_sqrtdf2                           (rtx, rtx);
extern rtx        gen_one_cmplsi2                       (rtx, rtx);
extern rtx        gen_floatsihf2                        (rtx, rtx);
extern rtx        gen_floatdihf2                        (rtx, rtx);
extern rtx        gen_floatsisf2                        (rtx, rtx);
extern rtx        gen_floatsidf2                        (rtx, rtx);
extern rtx        gen_fix_trunchfsi2                    (rtx, rtx);
extern rtx        gen_fix_trunchfdi2                    (rtx, rtx);
extern rtx        gen_fix_truncsfsi2                    (rtx, rtx);
extern rtx        gen_fix_truncdfsi2                    (rtx, rtx);
extern rtx        gen_truncdfsf2                        (rtx, rtx);
extern rtx        gen_truncdfhf2                        (rtx, rtx);
extern rtx        gen_zero_extendhisi2                  (rtx, rtx);
extern rtx        gen_zero_extendqisi2                  (rtx, rtx);
extern rtx        gen_extendhisi2                       (rtx, rtx);
extern rtx        gen_extendhisi2_mem                   (rtx, rtx);
extern rtx        gen_extendqihi2                       (rtx, rtx);
extern rtx        gen_extendqisi2                       (rtx, rtx);
extern rtx        gen_extendsfdf2                       (rtx, rtx);
extern rtx        gen_extendhfdf2                       (rtx, rtx);
extern rtx        gen_movdi                             (rtx, rtx);
extern rtx        gen_movsi                             (rtx, rtx);
extern rtx        gen_calculate_pic_address             (rtx, rtx, rtx);
extern rtx        gen_builtin_setjmp_receiver           (rtx);
extern rtx        gen_storehi                           (rtx, rtx);
extern rtx        gen_storehi_bigend                    (rtx, rtx);
extern rtx        gen_storeinthi                        (rtx, rtx);
extern rtx        gen_storehi_single_op                 (rtx, rtx);
extern rtx        gen_movhi                             (rtx, rtx);
extern rtx        gen_movhi_bytes                       (rtx, rtx);
extern rtx        gen_movhi_bigend                      (rtx, rtx);
extern rtx        gen_reload_outhi                      (rtx, rtx, rtx);
extern rtx        gen_reload_inhi                       (rtx, rtx, rtx);
extern rtx        gen_movqi                             (rtx, rtx);
extern rtx        gen_movhf                             (rtx, rtx);
extern rtx        gen_movsf                             (rtx, rtx);
extern rtx        gen_movdf                             (rtx, rtx);
extern rtx        gen_reload_outdf                      (rtx, rtx, rtx);
extern rtx        gen_load_multiple                     (rtx, rtx, rtx);
extern rtx        gen_store_multiple                    (rtx, rtx, rtx);
extern rtx        gen_setmemsi                          (rtx, rtx, rtx, rtx);
extern rtx        gen_movmemqi                          (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchsi4                        (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchsf4                        (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchdf4                        (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchdi4                        (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranch_cc                        (rtx, rtx, rtx, rtx);
extern rtx        gen_cstore_cc                         (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoresi4                         (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoresf4                         (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoredf4                         (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoredi4                         (rtx, rtx, rtx, rtx);
extern rtx        gen_movsicc                           (rtx, rtx, rtx, rtx);
extern rtx        gen_movsfcc                           (rtx, rtx, rtx, rtx);
extern rtx        gen_movdfcc                           (rtx, rtx, rtx, rtx);
extern rtx        gen_jump                              (rtx);
#define GEN_CALL(A, B, C, D) gen_call ((A), (B), (C))
extern rtx        gen_call                              (rtx, rtx, rtx);
extern rtx        gen_call_internal                     (rtx, rtx, rtx);
extern rtx        gen_nonsecure_call_internal           (rtx, rtx, rtx);
#define GEN_CALL_VALUE(A, B, C, D, E) gen_call_value ((A), (B), (C), (D))
extern rtx        gen_call_value                        (rtx, rtx, rtx, rtx);
extern rtx        gen_call_value_internal               (rtx, rtx, rtx, rtx);
extern rtx        gen_nonsecure_call_value_internal     (rtx, rtx, rtx, rtx);
extern rtx        gen_sibcall_internal                  (rtx, rtx, rtx);
#define GEN_SIBCALL(A, B, C, D) gen_sibcall ((A), (B), (C))
extern rtx        gen_sibcall                           (rtx, rtx, rtx);
extern rtx        gen_sibcall_value_internal            (rtx, rtx, rtx, rtx);
#define GEN_SIBCALL_VALUE(A, B, C, D, E) gen_sibcall_value ((A), (B), (C), (D))
extern rtx        gen_sibcall_value                     (rtx, rtx, rtx, rtx);
extern rtx        gen_return                            (void);
extern rtx        gen_simple_return                     (void);
extern rtx        gen_return_addr_mask                  (rtx);
extern rtx        gen_untyped_call                      (rtx, rtx, rtx);
extern rtx        gen_untyped_return                    (rtx, rtx);
extern rtx        gen_casesi                            (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_indirect_jump                     (rtx);
extern rtx        gen_prologue                          (void);
extern rtx        gen_epilogue                          (void);
extern rtx        gen_sibcall_epilogue                  (void);
extern rtx        gen_eh_epilogue                       (rtx, rtx, rtx);
extern rtx        gen_ctzsi2                            (rtx, rtx);
extern rtx        gen_eh_return                         (rtx);
extern rtx        gen_get_thread_pointersi              (rtx);
extern rtx        gen_arm_legacy_rev                    (rtx, rtx, rtx, rtx);
extern rtx        gen_thumb_legacy_rev                  (rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_bswapsi2                          (rtx, rtx);
extern rtx        gen_bswaphi2                          (rtx, rtx);
extern rtx        gen_copysignsf3                       (rtx, rtx, rtx);
extern rtx        gen_copysigndf3                       (rtx, rtx, rtx);
extern rtx        gen_movv2di                           (rtx, rtx);
extern rtx        gen_movv2si                           (rtx, rtx);
extern rtx        gen_movv4hi                           (rtx, rtx);
extern rtx        gen_movv8qi                           (rtx, rtx);
extern rtx        gen_movv2sf                           (rtx, rtx);
extern rtx        gen_movv4si                           (rtx, rtx);
extern rtx        gen_movv8hi                           (rtx, rtx);
extern rtx        gen_movv16qi                          (rtx, rtx);
extern rtx        gen_movv4sf                           (rtx, rtx);
extern rtx        gen_addv2di3                          (rtx, rtx, rtx);
extern rtx        gen_addv2si3                          (rtx, rtx, rtx);
extern rtx        gen_addv4hi3                          (rtx, rtx, rtx);
extern rtx        gen_addv8qi3                          (rtx, rtx, rtx);
extern rtx        gen_addv2sf3                          (rtx, rtx, rtx);
extern rtx        gen_addv4si3                          (rtx, rtx, rtx);
extern rtx        gen_addv8hi3                          (rtx, rtx, rtx);
extern rtx        gen_addv16qi3                         (rtx, rtx, rtx);
extern rtx        gen_addv4sf3                          (rtx, rtx, rtx);
extern rtx        gen_subv2di3                          (rtx, rtx, rtx);
extern rtx        gen_subv2si3                          (rtx, rtx, rtx);
extern rtx        gen_subv4hi3                          (rtx, rtx, rtx);
extern rtx        gen_subv8qi3                          (rtx, rtx, rtx);
extern rtx        gen_subv2sf3                          (rtx, rtx, rtx);
extern rtx        gen_subv4si3                          (rtx, rtx, rtx);
extern rtx        gen_subv8hi3                          (rtx, rtx, rtx);
extern rtx        gen_subv16qi3                         (rtx, rtx, rtx);
extern rtx        gen_subv4sf3                          (rtx, rtx, rtx);
extern rtx        gen_mulv2si3                          (rtx, rtx, rtx);
extern rtx        gen_mulv4hi3                          (rtx, rtx, rtx);
extern rtx        gen_mulv8qi3                          (rtx, rtx, rtx);
extern rtx        gen_mulv2sf3                          (rtx, rtx, rtx);
extern rtx        gen_mulv4si3                          (rtx, rtx, rtx);
extern rtx        gen_mulv8hi3                          (rtx, rtx, rtx);
extern rtx        gen_mulv16qi3                         (rtx, rtx, rtx);
extern rtx        gen_mulv4sf3                          (rtx, rtx, rtx);
extern rtx        gen_sminv2si3                         (rtx, rtx, rtx);
extern rtx        gen_sminv4hi3                         (rtx, rtx, rtx);
extern rtx        gen_sminv8qi3                         (rtx, rtx, rtx);
extern rtx        gen_sminv2sf3                         (rtx, rtx, rtx);
extern rtx        gen_sminv4si3                         (rtx, rtx, rtx);
extern rtx        gen_sminv8hi3                         (rtx, rtx, rtx);
extern rtx        gen_sminv16qi3                        (rtx, rtx, rtx);
extern rtx        gen_sminv4sf3                         (rtx, rtx, rtx);
extern rtx        gen_uminv2si3                         (rtx, rtx, rtx);
extern rtx        gen_uminv4hi3                         (rtx, rtx, rtx);
extern rtx        gen_uminv8qi3                         (rtx, rtx, rtx);
extern rtx        gen_uminv4si3                         (rtx, rtx, rtx);
extern rtx        gen_uminv8hi3                         (rtx, rtx, rtx);
extern rtx        gen_uminv16qi3                        (rtx, rtx, rtx);
extern rtx        gen_smaxv2si3                         (rtx, rtx, rtx);
extern rtx        gen_smaxv4hi3                         (rtx, rtx, rtx);
extern rtx        gen_smaxv8qi3                         (rtx, rtx, rtx);
extern rtx        gen_smaxv2sf3                         (rtx, rtx, rtx);
extern rtx        gen_smaxv4si3                         (rtx, rtx, rtx);
extern rtx        gen_smaxv8hi3                         (rtx, rtx, rtx);
extern rtx        gen_smaxv16qi3                        (rtx, rtx, rtx);
extern rtx        gen_smaxv4sf3                         (rtx, rtx, rtx);
extern rtx        gen_umaxv2si3                         (rtx, rtx, rtx);
extern rtx        gen_umaxv4hi3                         (rtx, rtx, rtx);
extern rtx        gen_umaxv8qi3                         (rtx, rtx, rtx);
extern rtx        gen_umaxv4si3                         (rtx, rtx, rtx);
extern rtx        gen_umaxv8hi3                         (rtx, rtx, rtx);
extern rtx        gen_umaxv16qi3                        (rtx, rtx, rtx);
extern rtx        gen_vec_perm_constv2di                (rtx, rtx, rtx, rtx);
extern rtx        gen_vec_perm_constv2si                (rtx, rtx, rtx, rtx);
extern rtx        gen_vec_perm_constv4hi                (rtx, rtx, rtx, rtx);
extern rtx        gen_vec_perm_constv8qi                (rtx, rtx, rtx, rtx);
extern rtx        gen_vec_perm_constv2sf                (rtx, rtx, rtx, rtx);
extern rtx        gen_vec_perm_constv4si                (rtx, rtx, rtx, rtx);
extern rtx        gen_vec_perm_constv8hi                (rtx, rtx, rtx, rtx);
extern rtx        gen_vec_perm_constv16qi               (rtx, rtx, rtx, rtx);
extern rtx        gen_vec_perm_constv4sf                (rtx, rtx, rtx, rtx);
extern rtx        gen_vec_permv8qi                      (rtx, rtx, rtx, rtx);
extern rtx        gen_vec_permv16qi                     (rtx, rtx, rtx, rtx);
extern rtx        gen_iwmmxt_setwcgr0                   (rtx);
extern rtx        gen_iwmmxt_setwcgr1                   (rtx);
extern rtx        gen_iwmmxt_setwcgr2                   (rtx);
extern rtx        gen_iwmmxt_setwcgr3                   (rtx);
extern rtx        gen_iwmmxt_getwcgr0                   (rtx);
extern rtx        gen_iwmmxt_getwcgr1                   (rtx);
extern rtx        gen_iwmmxt_getwcgr2                   (rtx);
extern rtx        gen_iwmmxt_getwcgr3                   (rtx);
extern rtx        gen_thumb_movhi_clobber               (rtx, rtx, rtx);
extern rtx        gen_cbranchqi4                        (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoresi_eq0_thumb1               (rtx, rtx);
extern rtx        gen_cstoresi_ne0_thumb1               (rtx, rtx);
extern rtx        gen_thumb1_casesi_internal_pic        (rtx, rtx, rtx, rtx);
extern rtx        gen_tablejump                         (rtx, rtx);
extern rtx        gen_doloop_end                        (rtx, rtx);
extern rtx        gen_movti                             (rtx, rtx);
extern rtx        gen_movei                             (rtx, rtx);
extern rtx        gen_movoi                             (rtx, rtx);
extern rtx        gen_movci                             (rtx, rtx);
extern rtx        gen_movxi                             (rtx, rtx);
extern rtx        gen_movmisalignv8qi                   (rtx, rtx);
extern rtx        gen_movmisalignv16qi                  (rtx, rtx);
extern rtx        gen_movmisalignv4hi                   (rtx, rtx);
extern rtx        gen_movmisalignv8hi                   (rtx, rtx);
extern rtx        gen_movmisalignv2si                   (rtx, rtx);
extern rtx        gen_movmisalignv4si                   (rtx, rtx);
extern rtx        gen_movmisalignv2sf                   (rtx, rtx);
extern rtx        gen_movmisalignv4sf                   (rtx, rtx);
extern rtx        gen_movmisaligndi                     (rtx, rtx);
extern rtx        gen_movmisalignv2di                   (rtx, rtx);
extern rtx        gen_vec_setv8qi                       (rtx, rtx, rtx);
extern rtx        gen_vec_setv16qi                      (rtx, rtx, rtx);
extern rtx        gen_vec_setv4hi                       (rtx, rtx, rtx);
extern rtx        gen_vec_setv8hi                       (rtx, rtx, rtx);
extern rtx        gen_vec_setv2si                       (rtx, rtx, rtx);
extern rtx        gen_vec_setv4si                       (rtx, rtx, rtx);
extern rtx        gen_vec_setv2sf                       (rtx, rtx, rtx);
extern rtx        gen_vec_setv4sf                       (rtx, rtx, rtx);
extern rtx        gen_vec_setv2di                       (rtx, rtx, rtx);
extern rtx        gen_vec_initv8qi                      (rtx, rtx);
extern rtx        gen_vec_initv16qi                     (rtx, rtx);
extern rtx        gen_vec_initv4hi                      (rtx, rtx);
extern rtx        gen_vec_initv8hi                      (rtx, rtx);
extern rtx        gen_vec_initv2si                      (rtx, rtx);
extern rtx        gen_vec_initv4si                      (rtx, rtx);
extern rtx        gen_vec_initv2sf                      (rtx, rtx);
extern rtx        gen_vec_initv4sf                      (rtx, rtx);
extern rtx        gen_vec_initv2di                      (rtx, rtx);
extern rtx        gen_vashrv8qi3                        (rtx, rtx, rtx);
extern rtx        gen_vashrv16qi3                       (rtx, rtx, rtx);
extern rtx        gen_vashrv4hi3                        (rtx, rtx, rtx);
extern rtx        gen_vashrv8hi3                        (rtx, rtx, rtx);
extern rtx        gen_vashrv2si3                        (rtx, rtx, rtx);
extern rtx        gen_vashrv4si3                        (rtx, rtx, rtx);
extern rtx        gen_vlshrv8qi3                        (rtx, rtx, rtx);
extern rtx        gen_vlshrv16qi3                       (rtx, rtx, rtx);
extern rtx        gen_vlshrv4hi3                        (rtx, rtx, rtx);
extern rtx        gen_vlshrv8hi3                        (rtx, rtx, rtx);
extern rtx        gen_vlshrv2si3                        (rtx, rtx, rtx);
extern rtx        gen_vlshrv4si3                        (rtx, rtx, rtx);
extern rtx        gen_vec_shr_v8qi                      (rtx, rtx, rtx);
extern rtx        gen_vec_shr_v16qi                     (rtx, rtx, rtx);
extern rtx        gen_vec_shr_v4hi                      (rtx, rtx, rtx);
extern rtx        gen_vec_shr_v8hi                      (rtx, rtx, rtx);
extern rtx        gen_vec_shr_v2si                      (rtx, rtx, rtx);
extern rtx        gen_vec_shr_v4si                      (rtx, rtx, rtx);
extern rtx        gen_vec_shr_v2sf                      (rtx, rtx, rtx);
extern rtx        gen_vec_shr_v4sf                      (rtx, rtx, rtx);
extern rtx        gen_vec_shr_v2di                      (rtx, rtx, rtx);
extern rtx        gen_vec_shl_v8qi                      (rtx, rtx, rtx);
extern rtx        gen_vec_shl_v16qi                     (rtx, rtx, rtx);
extern rtx        gen_vec_shl_v4hi                      (rtx, rtx, rtx);
extern rtx        gen_vec_shl_v8hi                      (rtx, rtx, rtx);
extern rtx        gen_vec_shl_v2si                      (rtx, rtx, rtx);
extern rtx        gen_vec_shl_v4si                      (rtx, rtx, rtx);
extern rtx        gen_vec_shl_v2sf                      (rtx, rtx, rtx);
extern rtx        gen_vec_shl_v4sf                      (rtx, rtx, rtx);
extern rtx        gen_vec_shl_v2di                      (rtx, rtx, rtx);
extern rtx        gen_move_hi_quad_v2di                 (rtx, rtx);
extern rtx        gen_move_hi_quad_v2df                 (rtx, rtx);
extern rtx        gen_move_hi_quad_v16qi                (rtx, rtx);
extern rtx        gen_move_hi_quad_v8hi                 (rtx, rtx);
extern rtx        gen_move_hi_quad_v4si                 (rtx, rtx);
extern rtx        gen_move_hi_quad_v4sf                 (rtx, rtx);
extern rtx        gen_move_lo_quad_v2di                 (rtx, rtx);
extern rtx        gen_move_lo_quad_v2df                 (rtx, rtx);
extern rtx        gen_move_lo_quad_v16qi                (rtx, rtx);
extern rtx        gen_move_lo_quad_v8hi                 (rtx, rtx);
extern rtx        gen_move_lo_quad_v4si                 (rtx, rtx);
extern rtx        gen_move_lo_quad_v4sf                 (rtx, rtx);
extern rtx        gen_reduc_plus_scal_v8qi              (rtx, rtx);
extern rtx        gen_reduc_plus_scal_v4hi              (rtx, rtx);
extern rtx        gen_reduc_plus_scal_v2si              (rtx, rtx);
extern rtx        gen_reduc_plus_scal_v2sf              (rtx, rtx);
extern rtx        gen_reduc_plus_scal_v16qi             (rtx, rtx);
extern rtx        gen_reduc_plus_scal_v8hi              (rtx, rtx);
extern rtx        gen_reduc_plus_scal_v4si              (rtx, rtx);
extern rtx        gen_reduc_plus_scal_v4sf              (rtx, rtx);
extern rtx        gen_reduc_plus_scal_v2di              (rtx, rtx);
extern rtx        gen_reduc_smin_scal_v8qi              (rtx, rtx);
extern rtx        gen_reduc_smin_scal_v4hi              (rtx, rtx);
extern rtx        gen_reduc_smin_scal_v2si              (rtx, rtx);
extern rtx        gen_reduc_smin_scal_v2sf              (rtx, rtx);
extern rtx        gen_reduc_smin_scal_v16qi             (rtx, rtx);
extern rtx        gen_reduc_smin_scal_v8hi              (rtx, rtx);
extern rtx        gen_reduc_smin_scal_v4si              (rtx, rtx);
extern rtx        gen_reduc_smin_scal_v4sf              (rtx, rtx);
extern rtx        gen_reduc_smax_scal_v8qi              (rtx, rtx);
extern rtx        gen_reduc_smax_scal_v4hi              (rtx, rtx);
extern rtx        gen_reduc_smax_scal_v2si              (rtx, rtx);
extern rtx        gen_reduc_smax_scal_v2sf              (rtx, rtx);
extern rtx        gen_reduc_smax_scal_v16qi             (rtx, rtx);
extern rtx        gen_reduc_smax_scal_v8hi              (rtx, rtx);
extern rtx        gen_reduc_smax_scal_v4si              (rtx, rtx);
extern rtx        gen_reduc_smax_scal_v4sf              (rtx, rtx);
extern rtx        gen_reduc_umin_scal_v8qi              (rtx, rtx);
extern rtx        gen_reduc_umin_scal_v4hi              (rtx, rtx);
extern rtx        gen_reduc_umin_scal_v2si              (rtx, rtx);
extern rtx        gen_reduc_umin_scal_v16qi             (rtx, rtx);
extern rtx        gen_reduc_umin_scal_v8hi              (rtx, rtx);
extern rtx        gen_reduc_umin_scal_v4si              (rtx, rtx);
extern rtx        gen_reduc_umax_scal_v8qi              (rtx, rtx);
extern rtx        gen_reduc_umax_scal_v4hi              (rtx, rtx);
extern rtx        gen_reduc_umax_scal_v2si              (rtx, rtx);
extern rtx        gen_reduc_umax_scal_v16qi             (rtx, rtx);
extern rtx        gen_reduc_umax_scal_v8hi              (rtx, rtx);
extern rtx        gen_reduc_umax_scal_v4si              (rtx, rtx);
extern rtx        gen_vcondv8qiv8qi                     (rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_vcondv16qiv16qi                   (rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_vcondv4hiv4hi                     (rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_vcondv8hiv8hi                     (rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_vcondv2siv2si                     (rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_vcondv4siv4si                     (rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_vcondv2sfv2sf                     (rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_vcondv4sfv4sf                     (rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_vconduv8qiv8qi                    (rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_vconduv16qiv16qi                  (rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_vconduv4hiv4hi                    (rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_vconduv8hiv8hi                    (rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_vconduv2siv2si                    (rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_vconduv4siv4si                    (rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vaddv2sf                     (rtx, rtx, rtx);
extern rtx        gen_neon_vaddv4sf                     (rtx, rtx, rtx);
extern rtx        gen_neon_vmlav8qi                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlav16qi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlav4hi                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlav8hi                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlav2si                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlav4si                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlav2sf                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlav4sf                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vfmav2sf                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vfmav4sf                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vfmsv2sf                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vfmsv4sf                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlsv8qi                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlsv16qi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlsv4hi                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlsv8hi                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlsv2si                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlsv4si                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlsv2sf                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlsv4sf                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vsubv2sf                     (rtx, rtx, rtx);
extern rtx        gen_neon_vsubv4sf                     (rtx, rtx, rtx);
extern rtx        gen_neon_vpaddv8qi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vpaddv4hi                    (rtx, rtx, rtx);
extern rtx        gen_neon_vpaddv2si                    (rtx, rtx, rtx);
extern rtx        gen_neon_vpaddv2sf                    (rtx, rtx, rtx);
extern rtx        gen_neon_vabsv8qi                     (rtx, rtx);
extern rtx        gen_neon_vabsv16qi                    (rtx, rtx);
extern rtx        gen_neon_vabsv4hi                     (rtx, rtx);
extern rtx        gen_neon_vabsv8hi                     (rtx, rtx);
extern rtx        gen_neon_vabsv2si                     (rtx, rtx);
extern rtx        gen_neon_vabsv4si                     (rtx, rtx);
extern rtx        gen_neon_vabsv2sf                     (rtx, rtx);
extern rtx        gen_neon_vabsv4sf                     (rtx, rtx);
extern rtx        gen_neon_vnegv8qi                     (rtx, rtx);
extern rtx        gen_neon_vnegv16qi                    (rtx, rtx);
extern rtx        gen_neon_vnegv4hi                     (rtx, rtx);
extern rtx        gen_neon_vnegv8hi                     (rtx, rtx);
extern rtx        gen_neon_vnegv2si                     (rtx, rtx);
extern rtx        gen_neon_vnegv4si                     (rtx, rtx);
extern rtx        gen_neon_vnegv2sf                     (rtx, rtx);
extern rtx        gen_neon_vnegv4sf                     (rtx, rtx);
extern rtx        gen_neon_copysignfv2sf                (rtx, rtx, rtx);
extern rtx        gen_neon_copysignfv4sf                (rtx, rtx, rtx);
extern rtx        gen_neon_vclzv8qi                     (rtx, rtx);
extern rtx        gen_neon_vclzv16qi                    (rtx, rtx);
extern rtx        gen_neon_vclzv4hi                     (rtx, rtx);
extern rtx        gen_neon_vclzv8hi                     (rtx, rtx);
extern rtx        gen_neon_vclzv2si                     (rtx, rtx);
extern rtx        gen_neon_vclzv4si                     (rtx, rtx);
extern rtx        gen_neon_vcntv8qi                     (rtx, rtx);
extern rtx        gen_neon_vcntv16qi                    (rtx, rtx);
extern rtx        gen_neon_vmvnv8qi                     (rtx, rtx);
extern rtx        gen_neon_vmvnv16qi                    (rtx, rtx);
extern rtx        gen_neon_vmvnv4hi                     (rtx, rtx);
extern rtx        gen_neon_vmvnv8hi                     (rtx, rtx);
extern rtx        gen_neon_vmvnv2si                     (rtx, rtx);
extern rtx        gen_neon_vmvnv4si                     (rtx, rtx);
extern rtx        gen_neon_vget_lanev8qi                (rtx, rtx, rtx);
extern rtx        gen_neon_vget_lanev16qi               (rtx, rtx, rtx);
extern rtx        gen_neon_vget_lanev4hi                (rtx, rtx, rtx);
extern rtx        gen_neon_vget_lanev8hi                (rtx, rtx, rtx);
extern rtx        gen_neon_vget_lanev2si                (rtx, rtx, rtx);
extern rtx        gen_neon_vget_lanev4si                (rtx, rtx, rtx);
extern rtx        gen_neon_vget_lanev2sf                (rtx, rtx, rtx);
extern rtx        gen_neon_vget_lanev4sf                (rtx, rtx, rtx);
extern rtx        gen_neon_vget_laneuv8qi               (rtx, rtx, rtx);
extern rtx        gen_neon_vget_laneuv16qi              (rtx, rtx, rtx);
extern rtx        gen_neon_vget_laneuv4hi               (rtx, rtx, rtx);
extern rtx        gen_neon_vget_laneuv8hi               (rtx, rtx, rtx);
extern rtx        gen_neon_vget_laneuv2si               (rtx, rtx, rtx);
extern rtx        gen_neon_vget_laneuv4si               (rtx, rtx, rtx);
extern rtx        gen_neon_vget_lanedi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vget_lanev2di                (rtx, rtx, rtx);
extern rtx        gen_neon_vset_lanev8qi                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vset_lanev16qi               (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vset_lanev4hi                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vset_lanev8hi                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vset_lanev2si                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vset_lanev4si                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vset_lanev2sf                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vset_lanev4sf                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vset_lanev2di                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vset_lanedi                  (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vcreatev8qi                  (rtx, rtx);
extern rtx        gen_neon_vcreatev4hi                  (rtx, rtx);
extern rtx        gen_neon_vcreatev2si                  (rtx, rtx);
extern rtx        gen_neon_vcreatev2sf                  (rtx, rtx);
extern rtx        gen_neon_vcreatedi                    (rtx, rtx);
extern rtx        gen_neon_vdup_ndi                     (rtx, rtx);
extern rtx        gen_neon_vdup_lanev8qi                (rtx, rtx, rtx);
extern rtx        gen_neon_vdup_lanev16qi               (rtx, rtx, rtx);
extern rtx        gen_neon_vdup_lanev4hi                (rtx, rtx, rtx);
extern rtx        gen_neon_vdup_lanev8hi                (rtx, rtx, rtx);
extern rtx        gen_neon_vdup_lanev2si                (rtx, rtx, rtx);
extern rtx        gen_neon_vdup_lanev4si                (rtx, rtx, rtx);
extern rtx        gen_neon_vdup_lanev2sf                (rtx, rtx, rtx);
extern rtx        gen_neon_vdup_lanev4sf                (rtx, rtx, rtx);
extern rtx        gen_neon_vdup_lanedi                  (rtx, rtx, rtx);
extern rtx        gen_neon_vdup_lanev2di                (rtx, rtx, rtx);
extern rtx        gen_neon_vget_highv16qi               (rtx, rtx);
extern rtx        gen_neon_vget_highv8hi                (rtx, rtx);
extern rtx        gen_neon_vget_highv4si                (rtx, rtx);
extern rtx        gen_neon_vget_highv4sf                (rtx, rtx);
extern rtx        gen_neon_vget_highv2di                (rtx, rtx);
extern rtx        gen_neon_vget_lowv16qi                (rtx, rtx);
extern rtx        gen_neon_vget_lowv8hi                 (rtx, rtx);
extern rtx        gen_neon_vget_lowv4si                 (rtx, rtx);
extern rtx        gen_neon_vget_lowv4sf                 (rtx, rtx);
extern rtx        gen_neon_vget_lowv2di                 (rtx, rtx);
extern rtx        gen_neon_vmul_nv4hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vmul_nv2si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vmul_nv2sf                   (rtx, rtx, rtx);
extern rtx        gen_neon_vmul_nv8hi                   (rtx, rtx, rtx);
extern rtx        gen_neon_vmul_nv4si                   (rtx, rtx, rtx);
extern rtx        gen_neon_vmul_nv4sf                   (rtx, rtx, rtx);
extern rtx        gen_neon_vmulls_nv4hi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vmulls_nv2si                 (rtx, rtx, rtx);
extern rtx        gen_neon_vmullu_nv4hi                 (rtx, rtx, rtx);
extern rtx        gen_neon_vmullu_nv2si                 (rtx, rtx, rtx);
extern rtx        gen_neon_vqdmull_nv4hi                (rtx, rtx, rtx);
extern rtx        gen_neon_vqdmull_nv2si                (rtx, rtx, rtx);
extern rtx        gen_neon_vqdmulh_nv4hi                (rtx, rtx, rtx);
extern rtx        gen_neon_vqdmulh_nv2si                (rtx, rtx, rtx);
extern rtx        gen_neon_vqrdmulh_nv4hi               (rtx, rtx, rtx);
extern rtx        gen_neon_vqrdmulh_nv2si               (rtx, rtx, rtx);
extern rtx        gen_neon_vqdmulh_nv8hi                (rtx, rtx, rtx);
extern rtx        gen_neon_vqdmulh_nv4si                (rtx, rtx, rtx);
extern rtx        gen_neon_vqrdmulh_nv8hi               (rtx, rtx, rtx);
extern rtx        gen_neon_vqrdmulh_nv4si               (rtx, rtx, rtx);
extern rtx        gen_neon_vmla_nv4hi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmla_nv2si                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmla_nv2sf                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmla_nv8hi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmla_nv4si                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmla_nv4sf                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlals_nv4hi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlals_nv2si                 (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlalu_nv4hi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlalu_nv2si                 (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vqdmlal_nv4hi                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vqdmlal_nv2si                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmls_nv4hi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmls_nv2si                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmls_nv2sf                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmls_nv8hi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmls_nv4si                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmls_nv4sf                   (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlsls_nv4hi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlsls_nv2si                 (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlslu_nv4hi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vmlslu_nv2si                 (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vqdmlsl_nv4hi                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vqdmlsl_nv2si                (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vbslv8qi                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vbslv16qi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vbslv4hi                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vbslv8hi                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vbslv2si                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vbslv4si                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vbslv2sf                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vbslv4sf                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vbsldi                       (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vbslv2di                     (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vtrnv8qi_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vtrnv16qi_internal           (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vtrnv4hi_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vtrnv8hi_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vtrnv2si_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vtrnv4si_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vtrnv2sf_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vtrnv4sf_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vzipv8qi_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vzipv16qi_internal           (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vzipv4hi_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vzipv8hi_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vzipv2si_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vzipv4si_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vzipv2sf_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vzipv4sf_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vuzpv8qi_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vuzpv16qi_internal           (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vuzpv4hi_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vuzpv8hi_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vuzpv2si_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vuzpv4si_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vuzpv2sf_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vuzpv4sf_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_neon_vreinterpretv8qiv8qi         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv8qiv4hi         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv8qiv2si         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv8qiv2sf         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv8qidi           (rtx, rtx);
extern rtx        gen_neon_vreinterpretv4hiv8qi         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv4hiv4hi         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv4hiv2si         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv4hiv2sf         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv4hidi           (rtx, rtx);
extern rtx        gen_neon_vreinterpretv2siv8qi         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv2siv4hi         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv2siv2si         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv2siv2sf         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv2sidi           (rtx, rtx);
extern rtx        gen_neon_vreinterpretv2sfv8qi         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv2sfv4hi         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv2sfv2si         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv2sfv2sf         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv2sfdi           (rtx, rtx);
extern rtx        gen_neon_vreinterpretdiv8qi           (rtx, rtx);
extern rtx        gen_neon_vreinterpretdiv4hi           (rtx, rtx);
extern rtx        gen_neon_vreinterpretdiv2si           (rtx, rtx);
extern rtx        gen_neon_vreinterpretdiv2sf           (rtx, rtx);
extern rtx        gen_neon_vreinterpretdidi             (rtx, rtx);
extern rtx        gen_neon_vreinterprettiv16qi          (rtx, rtx);
extern rtx        gen_neon_vreinterprettiv8hi           (rtx, rtx);
extern rtx        gen_neon_vreinterprettiv4si           (rtx, rtx);
extern rtx        gen_neon_vreinterprettiv4sf           (rtx, rtx);
extern rtx        gen_neon_vreinterprettiv2di           (rtx, rtx);
extern rtx        gen_neon_vreinterprettiti             (rtx, rtx);
extern rtx        gen_neon_vreinterpretv16qiv16qi       (rtx, rtx);
extern rtx        gen_neon_vreinterpretv16qiv8hi        (rtx, rtx);
extern rtx        gen_neon_vreinterpretv16qiv4si        (rtx, rtx);
extern rtx        gen_neon_vreinterpretv16qiv4sf        (rtx, rtx);
extern rtx        gen_neon_vreinterpretv16qiv2di        (rtx, rtx);
extern rtx        gen_neon_vreinterpretv16qiti          (rtx, rtx);
extern rtx        gen_neon_vreinterpretv8hiv16qi        (rtx, rtx);
extern rtx        gen_neon_vreinterpretv8hiv8hi         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv8hiv4si         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv8hiv4sf         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv8hiv2di         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv8hiti           (rtx, rtx);
extern rtx        gen_neon_vreinterpretv4siv16qi        (rtx, rtx);
extern rtx        gen_neon_vreinterpretv4siv8hi         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv4siv4si         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv4siv4sf         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv4siv2di         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv4siti           (rtx, rtx);
extern rtx        gen_neon_vreinterpretv4sfv16qi        (rtx, rtx);
extern rtx        gen_neon_vreinterpretv4sfv8hi         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv4sfv4si         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv4sfv4sf         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv4sfv2di         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv4sfti           (rtx, rtx);
extern rtx        gen_neon_vreinterpretv2div16qi        (rtx, rtx);
extern rtx        gen_neon_vreinterpretv2div8hi         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv2div4si         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv2div4sf         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv2div2di         (rtx, rtx);
extern rtx        gen_neon_vreinterpretv2diti           (rtx, rtx);
extern rtx        gen_vec_load_lanesv8qiv8qi            (rtx, rtx);
extern rtx        gen_vec_load_lanesv16qiv16qi          (rtx, rtx);
extern rtx        gen_vec_load_lanesv4hiv4hi            (rtx, rtx);
extern rtx        gen_vec_load_lanesv8hiv8hi            (rtx, rtx);
extern rtx        gen_vec_load_lanesv2siv2si            (rtx, rtx);
extern rtx        gen_vec_load_lanesv4siv4si            (rtx, rtx);
extern rtx        gen_vec_load_lanesv2sfv2sf            (rtx, rtx);
extern rtx        gen_vec_load_lanesv4sfv4sf            (rtx, rtx);
extern rtx        gen_vec_load_lanesdidi                (rtx, rtx);
extern rtx        gen_vec_load_lanesv2div2di            (rtx, rtx);
extern rtx        gen_neon_vld1_dupdi                   (rtx, rtx);
extern rtx        gen_vec_store_lanesv8qiv8qi           (rtx, rtx);
extern rtx        gen_vec_store_lanesv16qiv16qi         (rtx, rtx);
extern rtx        gen_vec_store_lanesv4hiv4hi           (rtx, rtx);
extern rtx        gen_vec_store_lanesv8hiv8hi           (rtx, rtx);
extern rtx        gen_vec_store_lanesv2siv2si           (rtx, rtx);
extern rtx        gen_vec_store_lanesv4siv4si           (rtx, rtx);
extern rtx        gen_vec_store_lanesv2sfv2sf           (rtx, rtx);
extern rtx        gen_vec_store_lanesv4sfv4sf           (rtx, rtx);
extern rtx        gen_vec_store_lanesdidi               (rtx, rtx);
extern rtx        gen_vec_store_lanesv2div2di           (rtx, rtx);
extern rtx        gen_vec_load_lanestiv8qi              (rtx, rtx);
extern rtx        gen_vec_load_lanestiv4hi              (rtx, rtx);
extern rtx        gen_vec_load_lanestiv2si              (rtx, rtx);
extern rtx        gen_vec_load_lanestiv2sf              (rtx, rtx);
extern rtx        gen_vec_load_lanestidi                (rtx, rtx);
extern rtx        gen_vec_load_lanesoiv16qi             (rtx, rtx);
extern rtx        gen_vec_load_lanesoiv8hi              (rtx, rtx);
extern rtx        gen_vec_load_lanesoiv4si              (rtx, rtx);
extern rtx        gen_vec_load_lanesoiv4sf              (rtx, rtx);
extern rtx        gen_vec_store_lanestiv8qi             (rtx, rtx);
extern rtx        gen_vec_store_lanestiv4hi             (rtx, rtx);
extern rtx        gen_vec_store_lanestiv2si             (rtx, rtx);
extern rtx        gen_vec_store_lanestiv2sf             (rtx, rtx);
extern rtx        gen_vec_store_lanestidi               (rtx, rtx);
extern rtx        gen_vec_store_lanesoiv16qi            (rtx, rtx);
extern rtx        gen_vec_store_lanesoiv8hi             (rtx, rtx);
extern rtx        gen_vec_store_lanesoiv4si             (rtx, rtx);
extern rtx        gen_vec_store_lanesoiv4sf             (rtx, rtx);
extern rtx        gen_vec_load_laneseiv8qi              (rtx, rtx);
extern rtx        gen_vec_load_laneseiv4hi              (rtx, rtx);
extern rtx        gen_vec_load_laneseiv2si              (rtx, rtx);
extern rtx        gen_vec_load_laneseiv2sf              (rtx, rtx);
extern rtx        gen_vec_load_laneseidi                (rtx, rtx);
extern rtx        gen_vec_load_lanesciv16qi             (rtx, rtx);
extern rtx        gen_vec_load_lanesciv8hi              (rtx, rtx);
extern rtx        gen_vec_load_lanesciv4si              (rtx, rtx);
extern rtx        gen_vec_load_lanesciv4sf              (rtx, rtx);
extern rtx        gen_neon_vld3v16qi                    (rtx, rtx);
extern rtx        gen_neon_vld3v8hi                     (rtx, rtx);
extern rtx        gen_neon_vld3v4si                     (rtx, rtx);
extern rtx        gen_neon_vld3v4sf                     (rtx, rtx);
extern rtx        gen_vec_store_laneseiv8qi             (rtx, rtx);
extern rtx        gen_vec_store_laneseiv4hi             (rtx, rtx);
extern rtx        gen_vec_store_laneseiv2si             (rtx, rtx);
extern rtx        gen_vec_store_laneseiv2sf             (rtx, rtx);
extern rtx        gen_vec_store_laneseidi               (rtx, rtx);
extern rtx        gen_vec_store_lanesciv16qi            (rtx, rtx);
extern rtx        gen_vec_store_lanesciv8hi             (rtx, rtx);
extern rtx        gen_vec_store_lanesciv4si             (rtx, rtx);
extern rtx        gen_vec_store_lanesciv4sf             (rtx, rtx);
extern rtx        gen_neon_vst3v16qi                    (rtx, rtx);
extern rtx        gen_neon_vst3v8hi                     (rtx, rtx);
extern rtx        gen_neon_vst3v4si                     (rtx, rtx);
extern rtx        gen_neon_vst3v4sf                     (rtx, rtx);
extern rtx        gen_vec_load_lanesoiv8qi              (rtx, rtx);
extern rtx        gen_vec_load_lanesoiv4hi              (rtx, rtx);
extern rtx        gen_vec_load_lanesoiv2si              (rtx, rtx);
extern rtx        gen_vec_load_lanesoiv2sf              (rtx, rtx);
extern rtx        gen_vec_load_lanesoidi                (rtx, rtx);
extern rtx        gen_vec_load_lanesxiv16qi             (rtx, rtx);
extern rtx        gen_vec_load_lanesxiv8hi              (rtx, rtx);
extern rtx        gen_vec_load_lanesxiv4si              (rtx, rtx);
extern rtx        gen_vec_load_lanesxiv4sf              (rtx, rtx);
extern rtx        gen_neon_vld4v16qi                    (rtx, rtx);
extern rtx        gen_neon_vld4v8hi                     (rtx, rtx);
extern rtx        gen_neon_vld4v4si                     (rtx, rtx);
extern rtx        gen_neon_vld4v4sf                     (rtx, rtx);
extern rtx        gen_vec_store_lanesoiv8qi             (rtx, rtx);
extern rtx        gen_vec_store_lanesoiv4hi             (rtx, rtx);
extern rtx        gen_vec_store_lanesoiv2si             (rtx, rtx);
extern rtx        gen_vec_store_lanesoiv2sf             (rtx, rtx);
extern rtx        gen_vec_store_lanesoidi               (rtx, rtx);
extern rtx        gen_vec_store_lanesxiv16qi            (rtx, rtx);
extern rtx        gen_vec_store_lanesxiv8hi             (rtx, rtx);
extern rtx        gen_vec_store_lanesxiv4si             (rtx, rtx);
extern rtx        gen_vec_store_lanesxiv4sf             (rtx, rtx);
extern rtx        gen_neon_vst4v16qi                    (rtx, rtx);
extern rtx        gen_neon_vst4v8hi                     (rtx, rtx);
extern rtx        gen_neon_vst4v4si                     (rtx, rtx);
extern rtx        gen_neon_vst4v4sf                     (rtx, rtx);
extern rtx        gen_vec_unpacks_hi_v16qi              (rtx, rtx);
extern rtx        gen_vec_unpacku_hi_v16qi              (rtx, rtx);
extern rtx        gen_vec_unpacks_hi_v8hi               (rtx, rtx);
extern rtx        gen_vec_unpacku_hi_v8hi               (rtx, rtx);
extern rtx        gen_vec_unpacks_hi_v4si               (rtx, rtx);
extern rtx        gen_vec_unpacku_hi_v4si               (rtx, rtx);
extern rtx        gen_vec_unpacks_lo_v16qi              (rtx, rtx);
extern rtx        gen_vec_unpacku_lo_v16qi              (rtx, rtx);
extern rtx        gen_vec_unpacks_lo_v8hi               (rtx, rtx);
extern rtx        gen_vec_unpacku_lo_v8hi               (rtx, rtx);
extern rtx        gen_vec_unpacks_lo_v4si               (rtx, rtx);
extern rtx        gen_vec_unpacku_lo_v4si               (rtx, rtx);
extern rtx        gen_vec_widen_smult_lo_v16qi          (rtx, rtx, rtx);
extern rtx        gen_vec_widen_umult_lo_v16qi          (rtx, rtx, rtx);
extern rtx        gen_vec_widen_smult_lo_v8hi           (rtx, rtx, rtx);
extern rtx        gen_vec_widen_umult_lo_v8hi           (rtx, rtx, rtx);
extern rtx        gen_vec_widen_smult_lo_v4si           (rtx, rtx, rtx);
extern rtx        gen_vec_widen_umult_lo_v4si           (rtx, rtx, rtx);
extern rtx        gen_vec_widen_smult_hi_v16qi          (rtx, rtx, rtx);
extern rtx        gen_vec_widen_umult_hi_v16qi          (rtx, rtx, rtx);
extern rtx        gen_vec_widen_smult_hi_v8hi           (rtx, rtx, rtx);
extern rtx        gen_vec_widen_umult_hi_v8hi           (rtx, rtx, rtx);
extern rtx        gen_vec_widen_smult_hi_v4si           (rtx, rtx, rtx);
extern rtx        gen_vec_widen_umult_hi_v4si           (rtx, rtx, rtx);
extern rtx        gen_vec_widen_sshiftl_lo_v16qi        (rtx, rtx, rtx);
extern rtx        gen_vec_widen_ushiftl_lo_v16qi        (rtx, rtx, rtx);
extern rtx        gen_vec_widen_sshiftl_lo_v8hi         (rtx, rtx, rtx);
extern rtx        gen_vec_widen_ushiftl_lo_v8hi         (rtx, rtx, rtx);
extern rtx        gen_vec_widen_sshiftl_lo_v4si         (rtx, rtx, rtx);
extern rtx        gen_vec_widen_ushiftl_lo_v4si         (rtx, rtx, rtx);
extern rtx        gen_vec_widen_sshiftl_hi_v16qi        (rtx, rtx, rtx);
extern rtx        gen_vec_widen_ushiftl_hi_v16qi        (rtx, rtx, rtx);
extern rtx        gen_vec_widen_sshiftl_hi_v8hi         (rtx, rtx, rtx);
extern rtx        gen_vec_widen_ushiftl_hi_v8hi         (rtx, rtx, rtx);
extern rtx        gen_vec_widen_sshiftl_hi_v4si         (rtx, rtx, rtx);
extern rtx        gen_vec_widen_ushiftl_hi_v4si         (rtx, rtx, rtx);
extern rtx        gen_vec_unpacks_lo_v8qi               (rtx, rtx);
extern rtx        gen_vec_unpacku_lo_v8qi               (rtx, rtx);
extern rtx        gen_vec_unpacks_lo_v4hi               (rtx, rtx);
extern rtx        gen_vec_unpacku_lo_v4hi               (rtx, rtx);
extern rtx        gen_vec_unpacks_lo_v2si               (rtx, rtx);
extern rtx        gen_vec_unpacku_lo_v2si               (rtx, rtx);
extern rtx        gen_vec_unpacks_hi_v8qi               (rtx, rtx);
extern rtx        gen_vec_unpacku_hi_v8qi               (rtx, rtx);
extern rtx        gen_vec_unpacks_hi_v4hi               (rtx, rtx);
extern rtx        gen_vec_unpacku_hi_v4hi               (rtx, rtx);
extern rtx        gen_vec_unpacks_hi_v2si               (rtx, rtx);
extern rtx        gen_vec_unpacku_hi_v2si               (rtx, rtx);
extern rtx        gen_vec_widen_smult_hi_v8qi           (rtx, rtx, rtx);
extern rtx        gen_vec_widen_umult_hi_v8qi           (rtx, rtx, rtx);
extern rtx        gen_vec_widen_smult_hi_v4hi           (rtx, rtx, rtx);
extern rtx        gen_vec_widen_umult_hi_v4hi           (rtx, rtx, rtx);
extern rtx        gen_vec_widen_smult_hi_v2si           (rtx, rtx, rtx);
extern rtx        gen_vec_widen_umult_hi_v2si           (rtx, rtx, rtx);
extern rtx        gen_vec_widen_smult_lo_v8qi           (rtx, rtx, rtx);
extern rtx        gen_vec_widen_umult_lo_v8qi           (rtx, rtx, rtx);
extern rtx        gen_vec_widen_smult_lo_v4hi           (rtx, rtx, rtx);
extern rtx        gen_vec_widen_umult_lo_v4hi           (rtx, rtx, rtx);
extern rtx        gen_vec_widen_smult_lo_v2si           (rtx, rtx, rtx);
extern rtx        gen_vec_widen_umult_lo_v2si           (rtx, rtx, rtx);
extern rtx        gen_vec_widen_sshiftl_hi_v8qi         (rtx, rtx, rtx);
extern rtx        gen_vec_widen_ushiftl_hi_v8qi         (rtx, rtx, rtx);
extern rtx        gen_vec_widen_sshiftl_hi_v4hi         (rtx, rtx, rtx);
extern rtx        gen_vec_widen_ushiftl_hi_v4hi         (rtx, rtx, rtx);
extern rtx        gen_vec_widen_sshiftl_hi_v2si         (rtx, rtx, rtx);
extern rtx        gen_vec_widen_ushiftl_hi_v2si         (rtx, rtx, rtx);
extern rtx        gen_vec_widen_sshiftl_lo_v8qi         (rtx, rtx, rtx);
extern rtx        gen_vec_widen_ushiftl_lo_v8qi         (rtx, rtx, rtx);
extern rtx        gen_vec_widen_sshiftl_lo_v4hi         (rtx, rtx, rtx);
extern rtx        gen_vec_widen_ushiftl_lo_v4hi         (rtx, rtx, rtx);
extern rtx        gen_vec_widen_sshiftl_lo_v2si         (rtx, rtx, rtx);
extern rtx        gen_vec_widen_ushiftl_lo_v2si         (rtx, rtx, rtx);
extern rtx        gen_vec_pack_trunc_v4hi               (rtx, rtx, rtx);
extern rtx        gen_vec_pack_trunc_v2si               (rtx, rtx, rtx);
extern rtx        gen_vec_pack_trunc_di                 (rtx, rtx, rtx);
extern rtx        gen_memory_barrier                    (void);
extern rtx        gen_atomic_loaddi                     (rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swapqi         (rtx, rtx, rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swaphi         (rtx, rtx, rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swapsi         (rtx, rtx, rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swapdi         (rtx, rtx, rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_mulqq3                            (rtx, rtx, rtx);
extern rtx        gen_mulhq3                            (rtx, rtx, rtx);
extern rtx        gen_mulsq3                            (rtx, rtx, rtx);
extern rtx        gen_mulsa3                            (rtx, rtx, rtx);
extern rtx        gen_mulusa3                           (rtx, rtx, rtx);
extern rtx        gen_mulha3                            (rtx, rtx, rtx);
extern rtx        gen_muluha3                           (rtx, rtx, rtx);
extern rtx        gen_ssmulha3                          (rtx, rtx, rtx);
extern rtx        gen_usmuluha3                         (rtx, rtx, rtx);

#endif /* GCC_INSN_FLAGS_H */
