/* Generated automatically by the program `genflags'
   from the machine description file `md'.  */

#ifndef GCC_INSN_FLAGS_H
#define GCC_INSN_FLAGS_H

#define HAVE_tstsi_t (TARGET_SH1 \
   && (can_create_pseudo_p () || arith_reg_operand (operands[1], SImode) \
       || satisfies_constraint_K08 (operands[1])))
#define HAVE_cmpeqsi_t (TARGET_SH1)
#define HAVE_cmpgtsi_t (TARGET_SH1)
#define HAVE_cmpgesi_t (TARGET_SH1)
#define HAVE_cmp_div0s (TARGET_SH1)
#define HAVE_cmpgeusi_t (TARGET_SH1)
#define HAVE_cmpgtusi_t (TARGET_SH1)
#define HAVE_cmpeqdi_t (TARGET_SH1)
#define HAVE_cmpgtdi_t (TARGET_SH2)
#define HAVE_cmpgedi_t (TARGET_SH2)
#define HAVE_cmpgeudi_t (TARGET_SH2)
#define HAVE_cmpgtudi_t (TARGET_SH2)
#define HAVE_adddi3 (TARGET_SH1)
#define HAVE_addc (TARGET_SH1)
#define HAVE_addsi3_scr (TARGET_SH1)
#define HAVE_subdi3 (TARGET_SH1)
#define HAVE_subc (TARGET_SH1)
#define HAVE_use_sfunc_addr (TARGET_SH1 && check_use_sfunc_addr (insn, operands[0]))
#define HAVE_udivsi3_sh2a (TARGET_SH2A)
#define HAVE_udivsi3_i1 (TARGET_SH1 && TARGET_DIVIDE_CALL_DIV1)
#define HAVE_udivsi3_i4 (TARGET_FPU_DOUBLE && ! TARGET_FPU_SINGLE)
#define HAVE_udivsi3_i4_single (TARGET_FPU_ANY && TARGET_FPU_SINGLE)
#define HAVE_udivsi3_i4_int (TARGET_SH1)
#define HAVE_divsi3_sh2a (TARGET_SH2A)
#define HAVE_divsi3_i1 (TARGET_SH1 && TARGET_DIVIDE_CALL_DIV1)
#define HAVE_divsi3_i4 (TARGET_FPU_DOUBLE && ! TARGET_FPU_SINGLE)
#define HAVE_divsi3_i4_single (TARGET_FPU_ANY && TARGET_FPU_SINGLE)
#define HAVE_divsi3_i4_int (TARGET_SH1)
#define HAVE_mulhisi3 (TARGET_SH1 && can_create_pseudo_p ())
#define HAVE_umulhisi3 (TARGET_SH1 && can_create_pseudo_p ())
#define HAVE_umulhisi3_i (TARGET_SH1)
#define HAVE_mulhisi3_i (TARGET_SH1)
#define HAVE_mulsi3_call (TARGET_SH1)
#define HAVE_mul_r (TARGET_SH2A)
#define HAVE_mul_l (TARGET_SH2)
#define HAVE_mulsi3_i (TARGET_SH2 && can_create_pseudo_p ())
#define HAVE_mulsidi3_i (TARGET_SH2)
#define HAVE_mulsidi3_compact (TARGET_SH2 && can_create_pseudo_p ())
#define HAVE_umulsidi3_i (TARGET_SH2)
#define HAVE_umulsidi3_compact (TARGET_SH2 && can_create_pseudo_p ())
#define HAVE_smulsi3_highpart_i (TARGET_SH2)
#define HAVE_smulsi3_highpart (TARGET_SH2 && can_create_pseudo_p ())
#define HAVE_umulsi3_highpart_i (TARGET_SH2)
#define HAVE_umulsi3_highpart (TARGET_SH2 && can_create_pseudo_p ())
#define HAVE_xorsi3 (TARGET_SH1)
#define HAVE_rotrsi3_1 (TARGET_SH1)
#define HAVE_rotlsi3_1 (TARGET_SH1)
#define HAVE_rotlsi3_31 (TARGET_SH1)
#define HAVE_rotlsi3_16 (TARGET_SH1)
#define HAVE_rotlhi3_8 (TARGET_SH1)
#define HAVE_rotcr (TARGET_SH1)
#define HAVE_rotcl (TARGET_SH1)
#define HAVE_ashlsi3_k (TARGET_SH1)
#define HAVE_ashlsi3_d (TARGET_DYNSHIFT)
#define HAVE_ashlsi3_d_call (TARGET_SH1 && !TARGET_DYNSHIFT)
#define HAVE_ashlsi3_n (TARGET_SH1 && ! sh_ashlsi_clobbers_t_reg_p (operands[2]))
#define HAVE_ashlsi3_n_clobbers_t (TARGET_SH1 && sh_ashlsi_clobbers_t_reg_p (operands[2]))
#define HAVE_shll (TARGET_SH1)
#define HAVE_ashlhi3_k (TARGET_SH1 && satisfies_constraint_P27 (operands[2]))
#define HAVE_ashldi3_k (TARGET_SH1)
#define HAVE_shar (TARGET_SH1)
#define HAVE_ashrsi3_k (TARGET_SH1 && INTVAL (operands[2]) == 1)
#define HAVE_ashrsi2_16 (TARGET_SH1)
#define HAVE_ashrsi2_31 (TARGET_SH1)
#define HAVE_ashrsi3_d (TARGET_DYNSHIFT)
#define HAVE_ashrsi3_n (TARGET_SH1)
#define HAVE_ashrdi3_k (TARGET_SH1)
#define HAVE_lshrsi3_k (TARGET_SH1)
#define HAVE_lshrsi3_d (TARGET_DYNSHIFT)
#define HAVE_lshrsi3_d_call (TARGET_SH1 && !TARGET_DYNSHIFT)
#define HAVE_lshrsi3_n (TARGET_SH1 && ! sh_lshrsi_clobbers_t_reg_p (operands[2]))
#define HAVE_lshrsi3_n_clobbers_t (TARGET_SH1 && sh_lshrsi_clobbers_t_reg_p (operands[2]))
#define HAVE_shlr (TARGET_SH1)
#define HAVE_lshrdi3_k (TARGET_SH1)
#define HAVE_and_shl_scratch (TARGET_SH1)
#define HAVE_shl_sext_ext (TARGET_SH1 && (unsigned)shl_sext_kind (operands[2], operands[3], 0) - 1 < 5)
#define HAVE_shl_sext_sub (TARGET_SH1 && (shl_sext_kind (operands[2], operands[3], 0) & ~1) == 6)
#define HAVE_xtrct_left (TARGET_SH1)
#define HAVE_xtrct_right (TARGET_SH1)
#define HAVE_negc (TARGET_SH1)
#define HAVE_negdi2 (TARGET_SH1)
#define HAVE_negsi2 (TARGET_SH1)
#define HAVE_one_cmplsi2 (TARGET_SH1)
#define HAVE_abssi2 (TARGET_SH1)
#define HAVE_absdi2 (TARGET_SH1)
#define HAVE_negsi_cond (TARGET_SH1 && TARGET_ZDCBRANCH)
#define HAVE_negdi_cond (TARGET_SH1)
#define HAVE_swapbsi2 (TARGET_SH1)
#define HAVE_zero_extendqihi2 (TARGET_SH1)
#define HAVE_push_fpul (TARGET_SH2E)
#define HAVE_pop_fpul (TARGET_SH2E)
#define HAVE_clrt (TARGET_SH1)
#define HAVE_sett (TARGET_SH1)
#define HAVE_movsi_i (TARGET_SH1 && !TARGET_FPU_ANY \
   && (register_operand (operands[0], SImode) \
       || register_operand (operands[1], SImode)))
#define HAVE_movsi_ie (TARGET_SH1 && TARGET_FPU_ANY \
   && ((register_operand (operands[0], SImode) \
	&& !fpscr_operand (operands[0], SImode)) \
       || (register_operand (operands[1], SImode) \
	   && !fpscr_operand (operands[1], SImode))))
#define HAVE_movsi_i_lowpart (TARGET_SH1 \
   && (register_operand (operands[0], SImode) \
       || register_operand (operands[1], SImode)))
#define HAVE_load_ra (TARGET_SH1)
#define HAVE_ic_invalidate_line_i (TARGET_HARD_SH4)
#define HAVE_ic_invalidate_line_sh4a (TARGET_SH4A || TARGET_SH4_300)
#define HAVE_movdf_k (TARGET_SH1 \
   && (!TARGET_FPU_DOUBLE || reload_completed \
       /* ??? We provide some insn so that direct_{load,store}[DFmode] get set */ \
       || (REG_P (operands[0]) && REGNO (operands[0]) == 3) \
       || (REG_P (operands[1]) && REGNO (operands[1]) == 3)) \
   && (arith_reg_operand (operands[0], DFmode) \
       || arith_reg_operand (operands[1], DFmode)))
#define HAVE_movdf_i4 (TARGET_FPU_DOUBLE \
   && (arith_reg_operand (operands[0], DFmode) \
       || arith_reg_operand (operands[1], DFmode)))
#define HAVE_movsf_i (TARGET_SH1 \
   && (! TARGET_SH2E \
       /* ??? We provide some insn so that direct_{load,store}[SFmode] get set */ \
       || (REG_P (operands[0]) && REGNO (operands[0]) == 3) \
       || (REG_P (operands[1]) && REGNO (operands[1]) == 3)) \
   && (arith_reg_operand (operands[0], SFmode) \
       || arith_reg_operand (operands[1], SFmode)))
#define HAVE_movsf_ie (TARGET_SH2E \
   && (arith_reg_operand (operands[0], SFmode) \
       || fpul_operand (operands[0], SFmode) \
       || arith_reg_operand (operands[1], SFmode) \
       || fpul_operand (operands[1], SFmode) \
       || arith_reg_operand (operands[2], SImode)))
#define HAVE_movsf_ie_ra (TARGET_SH2E \
   && (arith_reg_operand (operands[0], SFmode) \
       || fpul_operand (operands[0], SFmode) \
       || arith_reg_operand (operands[1], SFmode) \
       || fpul_operand (operands[1], SFmode)))
#define HAVE_block_branch_redirect (TARGET_SH1)
#define HAVE_indirect_jump_scratch (TARGET_SH1)
#define HAVE_stuff_delay_slot (TARGET_SH1)
#define HAVE_doloop_end_split (TARGET_SH2)
#define HAVE_jump_compact (TARGET_SH1 && !CROSSING_JUMP_P (insn))
#define HAVE_calli (TARGET_SH1 && !TARGET_FDPIC)
#define HAVE_calli_fdpic (TARGET_FDPIC)
#define HAVE_calli_tbr_rel (TARGET_SH2A && sh2a_is_function_vector_call (operands[0]))
#define HAVE_calli_pcrel (TARGET_SH2)
#define HAVE_call_pcrel (TARGET_SH2)
#define HAVE_call_valuei (TARGET_SH1 && !TARGET_FDPIC)
#define HAVE_call_valuei_fdpic (TARGET_FDPIC)
#define HAVE_call_valuei_tbr_rel (TARGET_SH2A && sh2a_is_function_vector_call (operands[1]))
#define HAVE_call_valuei_pcrel (TARGET_SH2)
#define HAVE_call_value_pcrel (TARGET_SH2)
#define HAVE_sibcalli (TARGET_SH1 && !TARGET_FDPIC)
#define HAVE_sibcalli_fdpic (TARGET_FDPIC)
#define HAVE_sibcalli_pcrel (TARGET_SH2 && !TARGET_FDPIC)
#define HAVE_sibcalli_pcrel_fdpic (TARGET_SH2 && TARGET_FDPIC)
#define HAVE_sibcalli_thunk (TARGET_SH1)
#define HAVE_sibcall_pcrel (TARGET_SH2 && !TARGET_FDPIC)
#define HAVE_sibcall_pcrel_fdpic (TARGET_SH2 && TARGET_FDPIC)
#define HAVE_sibcall_valuei (TARGET_SH1 && !TARGET_FDPIC)
#define HAVE_sibcall_valuei_fdpic (TARGET_FDPIC)
#define HAVE_sibcall_valuei_pcrel (TARGET_SH2 && !TARGET_FDPIC)
#define HAVE_sibcall_valuei_pcrel_fdpic (TARGET_SH2 && TARGET_FDPIC)
#define HAVE_sibcall_value_pcrel (TARGET_SH2 && !TARGET_FDPIC)
#define HAVE_sibcall_value_pcrel_fdpic (TARGET_SH2 && TARGET_FDPIC)
#define HAVE_indirect_jump_compact (TARGET_SH1)
#define HAVE_casesi_jump_1 (TARGET_SH1)
#define HAVE_casesi_jump_2 (TARGET_SH2 \
   && (! INSN_UID (operands[1]) \
       || prev_real_insn (as_a<rtx_insn *> (operands[1])) == insn))
#define HAVE_dect (TARGET_SH2)
#define HAVE_nop 1
#define HAVE_mova (TARGET_SH1)
#define HAVE_mova_const (TARGET_SH1)
#define HAVE_chk_guard_add (TARGET_SH1)
#define HAVE_tls_global_dynamic (TARGET_SH1)
#define HAVE_tls_local_dynamic (TARGET_SH1)
#define HAVE_tls_initial_exec 1
#define HAVE_store_gbr 1
#define HAVE_load_gbr (TARGET_SH1)
#define HAVE_casesi_worker_0 (TARGET_SH1)
#define HAVE_casesi_worker_1 (TARGET_SH1)
#define HAVE_casesi_worker_2 (TARGET_SH2 && reload_completed && flag_pic)
#define HAVE_eh_set_ra_si 1
#define HAVE_blockage 1
#define HAVE_movml_push_banked (TARGET_SH2A && REGNO (operands[0]) == 15)
#define HAVE_movml_pop_banked (TARGET_SH2A && REGNO (operands[0]) == 15)
#define HAVE_movt (TARGET_SH1)
#define HAVE_movrt (TARGET_SH2A)
#define HAVE_movrt_negc (TARGET_SH1)
#define HAVE_movrt_xor (TARGET_SH1)
#define HAVE_mov_neg_si_t (TARGET_SH1)
#define HAVE_nott (TARGET_SH2A || (TARGET_SH1 && can_create_pseudo_p ()))
#define HAVE_any_treg_expr_to_reg (TARGET_SH1 && can_create_pseudo_p ())
#define HAVE_consttable_2 1
#define HAVE_consttable_4 1
#define HAVE_consttable_8 1
#define HAVE_consttable_sf 1
#define HAVE_consttable_df 1
#define HAVE_align_log 1
#define HAVE_consttable_end 1
#define HAVE_consttable_window_end 1
#define HAVE_clipu_one (TARGET_SH2A)
#define HAVE_block_move_real (TARGET_SH1 && ! TARGET_HARD_SH4)
#define HAVE_block_lump_real (TARGET_SH1 && ! TARGET_HARD_SH4)
#define HAVE_block_move_real_i4 (TARGET_HARD_SH4)
#define HAVE_block_lump_real_i4 (TARGET_HARD_SH4)
#define HAVE_cmpstr_t (TARGET_SH1)
#define HAVE_lds_fpscr (TARGET_FPU_ANY)
#define HAVE_sts_fpscr (TARGET_FPU_ANY)
#define HAVE_toggle_sz (TARGET_FPU_DOUBLE)
#define HAVE_toggle_pr (TARGET_SH4A_FP)
#define HAVE_addsf3_i (TARGET_SH2E)
#define HAVE_subsf3_i (TARGET_SH2E)
#define HAVE_mulsf3_i (TARGET_SH2E)
#define HAVE_fmasf4_i (TARGET_SH2E)
#define HAVE_divsf3_i (TARGET_SH2E)
#define HAVE_floatsisf2_i4 (TARGET_SH2E)
#define HAVE_fix_truncsfsi2_i4 (TARGET_SH2E)
#define HAVE_cmpgtsf_t (TARGET_SH2E || TARGET_SH4 || TARGET_SH2A_SINGLE)
#define HAVE_cmpeqsf_t (TARGET_SH2E || TARGET_SH4 || TARGET_SH2A_SINGLE)
#define HAVE_ieee_ccmpeqsf_t (TARGET_IEEE && TARGET_SH2E)
#define HAVE_sqrtsf2_i (TARGET_SH3E)
#define HAVE_rsqrtsf2 (TARGET_FPU_ANY && TARGET_FSRRA)
#define HAVE_fsca (TARGET_FPU_ANY && TARGET_FSCA)
#define HAVE_adddf3_i (TARGET_FPU_DOUBLE)
#define HAVE_subdf3_i (TARGET_FPU_DOUBLE)
#define HAVE_muldf3_i (TARGET_FPU_DOUBLE)
#define HAVE_divdf3_i (TARGET_FPU_DOUBLE)
#define HAVE_floatsidf2_i (TARGET_FPU_DOUBLE)
#define HAVE_fix_truncdfsi2_i (TARGET_FPU_DOUBLE)
#define HAVE_cmpgtdf_t (TARGET_FPU_DOUBLE)
#define HAVE_cmpeqdf_t (TARGET_FPU_DOUBLE)
#define HAVE_sqrtdf2_i (TARGET_FPU_DOUBLE)
#define HAVE_extendsfdf2_i4 (TARGET_FPU_DOUBLE)
#define HAVE_truncdfsf2_i4 (TARGET_FPU_DOUBLE)
#define HAVE_movua (TARGET_SH4A)
#define HAVE_bclr_m2a (TARGET_SH2A && TARGET_BITOPS && satisfies_constraint_K03 (operands[1]))
#define HAVE_bclrmem_m2a (TARGET_SH2A && satisfies_constraint_Psz (operands[1]) && TARGET_BITOPS)
#define HAVE_bset_m2a (TARGET_SH2A && TARGET_BITOPS && satisfies_constraint_K03 (operands[1]))
#define HAVE_bsetmem_m2a (TARGET_SH2A && satisfies_constraint_Pso (operands[1]) && TARGET_BITOPS)
#define HAVE_bst_m2a (TARGET_SH2A && TARGET_BITOPS && satisfies_constraint_K03 (operands[1]))
#define HAVE_bld_m2a (TARGET_SH2A && TARGET_BITOPS && satisfies_constraint_K03 (operands[1]))
#define HAVE_bldsign_m2a (TARGET_SH2A && TARGET_BITOPS && satisfies_constraint_K03 (operands[1]))
#define HAVE_bldqi_reg (TARGET_SH2A && satisfies_constraint_K03 (operands[1]))
#define HAVE_bldhi_reg (TARGET_SH2A && satisfies_constraint_K03 (operands[1]))
#define HAVE_bldsi_reg (TARGET_SH2A && satisfies_constraint_K03 (operands[1]))
#define HAVE_band_m2a (TARGET_SH2A && TARGET_BITOPS && satisfies_constraint_K03 (operands[1]))
#define HAVE_bandreg_m2a (TARGET_SH2A && TARGET_BITOPS && satisfies_constraint_K03 (operands[2]))
#define HAVE_bor_m2a (TARGET_SH2A && TARGET_BITOPS && satisfies_constraint_K03 (operands[1]))
#define HAVE_borreg_m2a (TARGET_SH2A && TARGET_BITOPS && satisfies_constraint_K03 (operands[2]))
#define HAVE_bxor_m2a (TARGET_SH2A && TARGET_BITOPS && satisfies_constraint_K03 (operands[1]))
#define HAVE_bxorreg_m2a (TARGET_SH2A && TARGET_BITOPS && satisfies_constraint_K03 (operands[2]))
#define HAVE_sp_switch_1 (TARGET_SH1)
#define HAVE_sp_switch_2 (TARGET_SH1)
#define HAVE_stack_protect_set_si 1
#define HAVE_stack_protect_test_si 1
#define HAVE_atomic_compare_and_swapsi_hard (TARGET_ATOMIC_HARD_LLCS \
   || (TARGET_SH4A && TARGET_ATOMIC_ANY && !TARGET_ATOMIC_STRICT))
#define HAVE_atomic_compare_and_swapqi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_compare_and_swaphi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_compare_and_swapqi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_compare_and_swaphi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_compare_and_swapqi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_compare_and_swaphi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_compare_and_swapsi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_compare_and_swapqi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_compare_and_swaphi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_compare_and_swapsi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_compare_and_swapqi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_compare_and_swaphi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_compare_and_swapsi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_exchangesi_hard (TARGET_ATOMIC_HARD_LLCS \
   || (TARGET_SH4A && TARGET_ATOMIC_ANY && !TARGET_ATOMIC_STRICT))
#define HAVE_atomic_exchangeqi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_exchangehi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_exchangeqi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_exchangehi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_exchangeqi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_exchangehi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_exchangesi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_exchangeqi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_exchangehi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_exchangesi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_exchangeqi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_exchangehi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_exchangesi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_fetch_addsi_hard (TARGET_ATOMIC_HARD_LLCS \
   || (TARGET_SH4A && TARGET_ATOMIC_ANY && !TARGET_ATOMIC_STRICT))
#define HAVE_atomic_fetch_subsi_hard (TARGET_ATOMIC_HARD_LLCS \
   || (TARGET_SH4A && TARGET_ATOMIC_ANY && !TARGET_ATOMIC_STRICT))
#define HAVE_atomic_fetch_orsi_hard (TARGET_ATOMIC_HARD_LLCS \
   || (TARGET_SH4A && TARGET_ATOMIC_ANY && !TARGET_ATOMIC_STRICT))
#define HAVE_atomic_fetch_xorsi_hard (TARGET_ATOMIC_HARD_LLCS \
   || (TARGET_SH4A && TARGET_ATOMIC_ANY && !TARGET_ATOMIC_STRICT))
#define HAVE_atomic_fetch_andsi_hard (TARGET_ATOMIC_HARD_LLCS \
   || (TARGET_SH4A && TARGET_ATOMIC_ANY && !TARGET_ATOMIC_STRICT))
#define HAVE_atomic_fetch_notsi_hard (TARGET_ATOMIC_HARD_LLCS \
   || (TARGET_SH4A && TARGET_ATOMIC_ANY && !TARGET_ATOMIC_STRICT))
#define HAVE_atomic_fetch_addqi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_fetch_subqi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_fetch_orqi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_fetch_xorqi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_fetch_andqi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_fetch_addhi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_fetch_subhi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_fetch_orhi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_fetch_xorhi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_fetch_andhi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_fetch_addqi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_fetch_subqi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_fetch_orqi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_fetch_xorqi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_fetch_andqi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_fetch_addhi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_fetch_subhi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_fetch_orhi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_fetch_xorhi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_fetch_andhi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_addqi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_subqi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_orqi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_xorqi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_andqi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_addhi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_subhi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_orhi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_xorhi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_andhi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_addqi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_subqi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_orqi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_xorqi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_andqi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_addhi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_subhi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_orhi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_xorhi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_andhi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_fetch_notqi_hard (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_fetch_nothi_hard (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_notqi_hard (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_nothi_hard (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_fetch_addqi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_fetch_subqi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_fetch_orqi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_fetch_xorqi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_fetch_andqi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_fetch_addhi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_fetch_subhi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_fetch_orhi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_fetch_xorhi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_fetch_andhi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_fetch_addsi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_fetch_subsi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_fetch_orsi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_fetch_xorsi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_fetch_andsi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_fetch_notqi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_fetch_nothi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_fetch_notsi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_fetch_addqi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_fetch_subqi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_fetch_orqi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_fetch_xorqi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_fetch_andqi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_fetch_addhi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_fetch_subhi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_fetch_orhi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_fetch_xorhi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_fetch_andhi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_fetch_addsi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_fetch_subsi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_fetch_orsi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_fetch_xorsi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_fetch_andsi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_addqi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_subqi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_orqi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_xorqi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_andqi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_addhi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_subhi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_orhi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_xorhi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_andhi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_addsi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_subsi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_orsi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_xorsi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_andsi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_fetch_notqi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_fetch_nothi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_fetch_notsi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_notqi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_nothi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_notsi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_fetch_addqi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_fetch_subqi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_fetch_orqi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_fetch_xorqi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_fetch_andqi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_fetch_addhi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_fetch_subhi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_fetch_orhi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_fetch_xorhi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_fetch_andhi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_fetch_addsi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_fetch_subsi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_fetch_orsi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_fetch_xorsi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_fetch_andsi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_fetch_notqi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_fetch_nothi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_fetch_notsi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_fetch_nandsi_hard (TARGET_ATOMIC_HARD_LLCS \
   || (TARGET_SH4A && TARGET_ATOMIC_ANY && !TARGET_ATOMIC_STRICT))
#define HAVE_atomic_fetch_nandqi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_fetch_nandhi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_fetch_nandqi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_fetch_nandhi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_nandqi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_nandhi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_nandqi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_nandhi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_fetch_nandqi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_fetch_nandhi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_fetch_nandsi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_fetch_nandqi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_fetch_nandhi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_fetch_nandsi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_nandqi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_nandhi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_nandsi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_fetch_nandqi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_fetch_nandhi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_fetch_nandsi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_add_fetchsi_hard (TARGET_ATOMIC_HARD_LLCS \
   || (TARGET_SH4A && TARGET_ATOMIC_ANY && !TARGET_ATOMIC_STRICT))
#define HAVE_atomic_sub_fetchsi_hard (TARGET_ATOMIC_HARD_LLCS \
   || (TARGET_SH4A && TARGET_ATOMIC_ANY && !TARGET_ATOMIC_STRICT))
#define HAVE_atomic_or_fetchsi_hard (TARGET_ATOMIC_HARD_LLCS \
   || (TARGET_SH4A && TARGET_ATOMIC_ANY && !TARGET_ATOMIC_STRICT))
#define HAVE_atomic_xor_fetchsi_hard (TARGET_ATOMIC_HARD_LLCS \
   || (TARGET_SH4A && TARGET_ATOMIC_ANY && !TARGET_ATOMIC_STRICT))
#define HAVE_atomic_and_fetchsi_hard (TARGET_ATOMIC_HARD_LLCS \
   || (TARGET_SH4A && TARGET_ATOMIC_ANY && !TARGET_ATOMIC_STRICT))
#define HAVE_atomic_not_fetchsi_hard (TARGET_ATOMIC_HARD_LLCS \
   || (TARGET_SH4A && TARGET_ATOMIC_ANY && !TARGET_ATOMIC_STRICT))
#define HAVE_atomic_add_fetchqi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_sub_fetchqi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_or_fetchqi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_xor_fetchqi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_and_fetchqi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_add_fetchhi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_sub_fetchhi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_or_fetchhi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_xor_fetchhi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_and_fetchhi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_add_fetchqi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_sub_fetchqi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_or_fetchqi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_xor_fetchqi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_and_fetchqi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_add_fetchhi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_sub_fetchhi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_or_fetchhi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_xor_fetchhi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_and_fetchhi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_not_fetchqi_hard (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_not_fetchhi_hard (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_add_fetchqi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_sub_fetchqi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_or_fetchqi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_xor_fetchqi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_and_fetchqi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_add_fetchhi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_sub_fetchhi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_or_fetchhi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_xor_fetchhi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_and_fetchhi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_add_fetchsi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_sub_fetchsi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_or_fetchsi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_xor_fetchsi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_and_fetchsi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_not_fetchqi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_not_fetchhi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_not_fetchsi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_add_fetchqi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_sub_fetchqi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_or_fetchqi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_xor_fetchqi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_and_fetchqi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_add_fetchhi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_sub_fetchhi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_or_fetchhi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_xor_fetchhi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_and_fetchhi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_add_fetchsi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_sub_fetchsi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_or_fetchsi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_xor_fetchsi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_and_fetchsi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_not_fetchqi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_not_fetchhi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_not_fetchsi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_add_fetchqi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_sub_fetchqi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_or_fetchqi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_xor_fetchqi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_and_fetchqi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_add_fetchhi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_sub_fetchhi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_or_fetchhi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_xor_fetchhi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_and_fetchhi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_add_fetchsi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_sub_fetchsi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_or_fetchsi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_xor_fetchsi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_and_fetchsi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_not_fetchqi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_not_fetchhi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_not_fetchsi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_nand_fetchsi_hard (TARGET_ATOMIC_HARD_LLCS \
   || (TARGET_SH4A && TARGET_ATOMIC_ANY && !TARGET_ATOMIC_STRICT))
#define HAVE_atomic_nand_fetchqi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_nand_fetchhi_hard (TARGET_ATOMIC_HARD_LLCS && can_create_pseudo_p ())
#define HAVE_atomic_nand_fetchqi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_nand_fetchhi_hard_1 (TARGET_ATOMIC_HARD_LLCS)
#define HAVE_atomic_nand_fetchqi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_nand_fetchhi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_nand_fetchsi_soft_gusa (TARGET_ATOMIC_SOFT_GUSA)
#define HAVE_atomic_nand_fetchqi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_nand_fetchhi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_nand_fetchsi_soft_tcb (TARGET_ATOMIC_SOFT_TCB)
#define HAVE_atomic_nand_fetchqi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_nand_fetchhi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_atomic_nand_fetchsi_soft_imask (TARGET_ATOMIC_SOFT_IMASK)
#define HAVE_tasb (TARGET_ENABLE_TAS)
#define HAVE_atomic_test_and_set_soft_gusa (TARGET_ATOMIC_SOFT_GUSA && !TARGET_ENABLE_TAS)
#define HAVE_atomic_test_and_set_soft_tcb (TARGET_ATOMIC_SOFT_TCB && !TARGET_ENABLE_TAS)
#define HAVE_atomic_test_and_set_soft_imask (TARGET_ATOMIC_SOFT_IMASK && !TARGET_ENABLE_TAS)
#define HAVE_atomic_test_and_set_hard (TARGET_ATOMIC_HARD_LLCS && !TARGET_ENABLE_TAS)
#define HAVE_cbranchsi4 (can_create_pseudo_p ())
#define HAVE_cbranchdi4 (TARGET_SH2 && can_create_pseudo_p ())
#define HAVE_movsicc (TARGET_PRETEND_CMOVE)
#define HAVE_addsi3 1
#define HAVE_subsi3 1
#define HAVE_udivsi3 1
#define HAVE_divsi3 1
#define HAVE_mulsi3 (TARGET_SH1)
#define HAVE_mulsidi3 (TARGET_SH2)
#define HAVE_umulsidi3 (TARGET_SH2)
#define HAVE_andsi3 1
#define HAVE_iorsi3 1
#define HAVE_rotrsi3 (TARGET_SH1)
#define HAVE_rotlsi3 (TARGET_SH1)
#define HAVE_rotlhi3 (TARGET_SH1)
#define HAVE_ashlsi3 1
#define HAVE_ashlhi3 (TARGET_SH1)
#define HAVE_ashldi3 1
#define HAVE_ashldi3_std (TARGET_SH1 && INTVAL (operands[2]) < 32)
#define HAVE_ashrsi3 1
#define HAVE_ashrdi3 1
#define HAVE_lshrsi3 1
#define HAVE_lshrdi3 1
#define HAVE_bswapsi2 (TARGET_SH1)
#define HAVE_zero_extendqisi2 1
#define HAVE_zero_extendhisi2 1
#define HAVE_extendqisi2 1
#define HAVE_extendhisi2 1
#define HAVE_extendqihi2 (TARGET_SH1)
#define HAVE_push 1
#define HAVE_pop 1
#define HAVE_push_e 1
#define HAVE_push_4 1
#define HAVE_pop_e 1
#define HAVE_pop_4 1
#define HAVE_push_fpscr (TARGET_SH2E)
#define HAVE_pop_fpscr (TARGET_SH2E)
#define HAVE_movsi 1
#define HAVE_ic_invalidate_line (TARGET_HARD_SH4)
#define HAVE_movqi 1
#define HAVE_movhi 1
#define HAVE_movdi 1
#define HAVE_reload_indf__frn (TARGET_SH1)
#define HAVE_reload_outdf__RnFRm (TARGET_SH1)
#define HAVE_movdf 1
#define HAVE_movsf 1
#define HAVE_reload_insf__frn (TARGET_SH1)
#define HAVE_reload_insi__i_fpul (TARGET_SH1)
#define HAVE_branch_true (TARGET_SH1)
#define HAVE_branch_false (TARGET_SH1)
#define HAVE_doloop_end (TARGET_SH2)
#define HAVE_jump 1
#define HAVE_call 1
#define HAVE_call_value 1
#define HAVE_sibcall 1
#define HAVE_sibcall_value 1
#define HAVE_sibcall_epilogue 1
#define HAVE_indirect_jump 1
#define HAVE_untyped_call (TARGET_SH2E || TARGET_SH2A)
#define HAVE_GOTaddr2picreg 1
#define HAVE_builtin_setjmp_receiver (flag_pic)
#define HAVE_call_site (TARGET_SH1)
#define HAVE_sym_label2reg (TARGET_SH1)
#define HAVE_symPCREL_label2reg (TARGET_SH1)
#define HAVE_symGOT_load 1
#define HAVE_sym2GOT 1
#define HAVE_symGOT2reg 1
#define HAVE_sym2GOTFUNCDESC (TARGET_FDPIC)
#define HAVE_symGOTFUNCDESC2reg (TARGET_FDPIC)
#define HAVE_symGOTPLT2reg 1
#define HAVE_sym2GOTOFF 1
#define HAVE_symGOTOFF2reg 1
#define HAVE_sym2GOTOFFFUNCDESC (TARGET_FDPIC)
#define HAVE_symGOTOFFFUNCDESC2reg (TARGET_FDPIC)
#define HAVE_symPLT_label2reg (TARGET_SH1)
#define HAVE_sym2PIC 1
#define HAVE_sym2DTPOFF 1
#define HAVE_symDTPOFF2reg 1
#define HAVE_sym2GOTTPOFF 1
#define HAVE_sym2TPOFF 1
#define HAVE_symTPOFF2reg 1
#define HAVE_get_thread_pointersi (TARGET_SH1)
#define HAVE_set_thread_pointersi (TARGET_SH1)
#define HAVE_casesi 1
#define HAVE_casesi_0 (TARGET_SH1)
#define HAVE_simple_return (sh_can_use_simple_return_p ())
#define HAVE_return (reload_completed && epilogue_completed)
#define HAVE_prologue 1
#define HAVE_epilogue 1
#define HAVE_eh_return 1
#define HAVE_cstoresi4 (TARGET_SH1)
#define HAVE_cstoredi4 (TARGET_SH2)
#define HAVE_movnegt (TARGET_SH1)
#define HAVE_cstoresf4 (TARGET_SH2E)
#define HAVE_cstoredf4 (TARGET_FPU_DOUBLE)
#define HAVE_align_2 1
#define HAVE_align_4 1
#define HAVE_sminsi3 (TARGET_SH2A)
#define HAVE_smaxsi3 (TARGET_SH2A)
#define HAVE_uminsi3 (TARGET_SH2A)
#define HAVE_movmemsi (TARGET_SH1)
#define HAVE_cmpstrsi (TARGET_SH1 && optimize)
#define HAVE_cmpstrnsi (TARGET_SH1 && optimize)
#define HAVE_strlensi (TARGET_SH1 && optimize)
#define HAVE_setmemqi (TARGET_SH1 && optimize)
#define HAVE_set_fpscr (TARGET_FPU_ANY)
#define HAVE_addsf3 (TARGET_SH2E)
#define HAVE_subsf3 (TARGET_SH2E)
#define HAVE_mulsf3 (TARGET_SH2E)
#define HAVE_fmasf4 (TARGET_SH2E)
#define HAVE_divsf3 (TARGET_SH2E)
#define HAVE_floatsisf2 (TARGET_SH2E)
#define HAVE_fix_truncsfsi2 (TARGET_SH2E)
#define HAVE_cbranchsf4 (TARGET_SH2E)
#define HAVE_negsf2 (TARGET_SH2E)
#define HAVE_sqrtsf2 (TARGET_SH3E)
#define HAVE_sincossf3 (TARGET_FPU_ANY && TARGET_FSCA)
#define HAVE_abssf2 (TARGET_SH2E)
#define HAVE_adddf3 (TARGET_FPU_DOUBLE)
#define HAVE_subdf3 (TARGET_FPU_DOUBLE)
#define HAVE_muldf3 (TARGET_FPU_DOUBLE)
#define HAVE_divdf3 (TARGET_FPU_DOUBLE)
#define HAVE_floatsidf2 (TARGET_FPU_DOUBLE)
#define HAVE_fix_truncdfsi2 (TARGET_FPU_DOUBLE)
#define HAVE_cbranchdf4 (TARGET_FPU_DOUBLE)
#define HAVE_negdf2 (TARGET_FPU_DOUBLE)
#define HAVE_sqrtdf2 (TARGET_FPU_DOUBLE)
#define HAVE_absdf2 (TARGET_FPU_DOUBLE)
#define HAVE_extendsfdf2 (TARGET_FPU_DOUBLE)
#define HAVE_truncdfsf2 (TARGET_FPU_DOUBLE)
#define HAVE_insv (TARGET_SH1 && TARGET_BIG_ENDIAN)
#define HAVE_extv (TARGET_SH4A || TARGET_SH2A)
#define HAVE_extzv (TARGET_SH4A || TARGET_SH2A)
#define HAVE_prefetch ((TARGET_SH2A || TARGET_SH3) && !TARGET_VXWORKS_RTP)
#define HAVE_stack_protect_set 1
#define HAVE_stack_protect_test 1
#define HAVE_atomic_compare_and_swapqi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_compare_and_swaphi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_compare_and_swapsi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_exchangeqi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_exchangehi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_exchangesi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_fetch_addqi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_fetch_subqi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_fetch_orqi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_fetch_xorqi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_fetch_andqi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_fetch_addhi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_fetch_subhi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_fetch_orhi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_fetch_xorhi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_fetch_andhi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_fetch_addsi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_fetch_subsi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_fetch_orsi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_fetch_xorsi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_fetch_andsi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_fetch_nandqi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_fetch_nandhi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_fetch_nandsi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_add_fetchqi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_sub_fetchqi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_or_fetchqi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_xor_fetchqi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_and_fetchqi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_add_fetchhi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_sub_fetchhi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_or_fetchhi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_xor_fetchhi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_and_fetchhi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_add_fetchsi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_sub_fetchsi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_or_fetchsi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_xor_fetchsi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_and_fetchsi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_nand_fetchqi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_nand_fetchhi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_nand_fetchsi (TARGET_ATOMIC_ANY)
#define HAVE_atomic_test_and_set (TARGET_ATOMIC_ANY || TARGET_ENABLE_TAS)
extern rtx        gen_tstsi_t                              (rtx, rtx);
extern rtx        gen_cmpeqsi_t                            (rtx, rtx);
extern rtx        gen_cmpgtsi_t                            (rtx, rtx);
extern rtx        gen_cmpgesi_t                            (rtx, rtx);
extern rtx        gen_cmp_div0s                            (rtx, rtx);
extern rtx        gen_cmpgeusi_t                           (rtx, rtx);
extern rtx        gen_cmpgtusi_t                           (rtx, rtx);
extern rtx        gen_cmpeqdi_t                            (rtx, rtx);
extern rtx        gen_cmpgtdi_t                            (rtx, rtx);
extern rtx        gen_cmpgedi_t                            (rtx, rtx);
extern rtx        gen_cmpgeudi_t                           (rtx, rtx);
extern rtx        gen_cmpgtudi_t                           (rtx, rtx);
extern rtx        gen_adddi3                               (rtx, rtx, rtx);
extern rtx        gen_addc                                 (rtx, rtx, rtx);
extern rtx        gen_addsi3_scr                           (rtx, rtx, rtx);
extern rtx        gen_subdi3                               (rtx, rtx, rtx);
extern rtx        gen_subc                                 (rtx, rtx, rtx);
extern rtx        gen_use_sfunc_addr                       (rtx);
extern rtx        gen_udivsi3_sh2a                         (rtx, rtx, rtx);
extern rtx        gen_udivsi3_i1                           (rtx, rtx, rtx);
extern rtx        gen_udivsi3_i4                           (rtx, rtx, rtx);
extern rtx        gen_udivsi3_i4_single                    (rtx, rtx, rtx);
extern rtx        gen_udivsi3_i4_int                       (rtx, rtx);
extern rtx        gen_divsi3_sh2a                          (rtx, rtx, rtx);
extern rtx        gen_divsi3_i1                            (rtx, rtx);
extern rtx        gen_divsi3_i4                            (rtx, rtx, rtx);
extern rtx        gen_divsi3_i4_single                     (rtx, rtx, rtx);
extern rtx        gen_divsi3_i4_int                        (rtx, rtx);
extern rtx        gen_mulhisi3                             (rtx, rtx, rtx);
extern rtx        gen_umulhisi3                            (rtx, rtx, rtx);
extern rtx        gen_umulhisi3_i                          (rtx, rtx);
extern rtx        gen_mulhisi3_i                           (rtx, rtx);
extern rtx        gen_mulsi3_call                          (rtx, rtx);
extern rtx        gen_mul_r                                (rtx, rtx, rtx);
extern rtx        gen_mul_l                                (rtx, rtx);
extern rtx        gen_mulsi3_i                             (rtx, rtx, rtx);
extern rtx        gen_mulsidi3_i                           (rtx, rtx);
extern rtx        gen_mulsidi3_compact                     (rtx, rtx, rtx);
extern rtx        gen_umulsidi3_i                          (rtx, rtx);
extern rtx        gen_umulsidi3_compact                    (rtx, rtx, rtx);
extern rtx        gen_smulsi3_highpart_i                   (rtx, rtx);
extern rtx        gen_smulsi3_highpart                     (rtx, rtx, rtx);
extern rtx        gen_umulsi3_highpart_i                   (rtx, rtx);
extern rtx        gen_umulsi3_highpart                     (rtx, rtx, rtx);
extern rtx        gen_xorsi3                               (rtx, rtx, rtx);
extern rtx        gen_rotrsi3_1                            (rtx, rtx);
extern rtx        gen_rotlsi3_1                            (rtx, rtx);
extern rtx        gen_rotlsi3_31                           (rtx, rtx);
extern rtx        gen_rotlsi3_16                           (rtx, rtx);
extern rtx        gen_rotlhi3_8                            (rtx, rtx);
extern rtx        gen_rotcr                                (rtx, rtx, rtx);
extern rtx        gen_rotcl                                (rtx, rtx, rtx);
extern rtx        gen_ashlsi3_k                            (rtx, rtx, rtx);
extern rtx        gen_ashlsi3_d                            (rtx, rtx, rtx);
extern rtx        gen_ashlsi3_d_call                       (rtx, rtx, rtx, rtx);
extern rtx        gen_ashlsi3_n                            (rtx, rtx, rtx);
extern rtx        gen_ashlsi3_n_clobbers_t                 (rtx, rtx, rtx);
extern rtx        gen_shll                                 (rtx, rtx);
extern rtx        gen_ashlhi3_k                            (rtx, rtx, rtx);
extern rtx        gen_ashldi3_k                            (rtx, rtx);
extern rtx        gen_shar                                 (rtx, rtx);
extern rtx        gen_ashrsi3_k                            (rtx, rtx, rtx);
extern rtx        gen_ashrsi2_16                           (rtx, rtx);
extern rtx        gen_ashrsi2_31                           (rtx, rtx);
extern rtx        gen_ashrsi3_d                            (rtx, rtx, rtx);
extern rtx        gen_ashrsi3_n                            (rtx, rtx, rtx);
extern rtx        gen_ashrdi3_k                            (rtx, rtx);
extern rtx        gen_lshrsi3_k                            (rtx, rtx, rtx);
extern rtx        gen_lshrsi3_d                            (rtx, rtx, rtx);
extern rtx        gen_lshrsi3_d_call                       (rtx, rtx, rtx, rtx);
extern rtx        gen_lshrsi3_n                            (rtx, rtx, rtx);
extern rtx        gen_lshrsi3_n_clobbers_t                 (rtx, rtx, rtx);
extern rtx        gen_shlr                                 (rtx, rtx);
extern rtx        gen_lshrdi3_k                            (rtx, rtx);
extern rtx        gen_and_shl_scratch                      (rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_shl_sext_ext                         (rtx, rtx, rtx, rtx);
extern rtx        gen_shl_sext_sub                         (rtx, rtx, rtx, rtx);
extern rtx        gen_xtrct_left                           (rtx, rtx, rtx);
extern rtx        gen_xtrct_right                          (rtx, rtx, rtx);
extern rtx        gen_negc                                 (rtx, rtx);
extern rtx        gen_negdi2                               (rtx, rtx);
extern rtx        gen_negsi2                               (rtx, rtx);
extern rtx        gen_one_cmplsi2                          (rtx, rtx);
extern rtx        gen_abssi2                               (rtx, rtx);
extern rtx        gen_absdi2                               (rtx, rtx);
extern rtx        gen_negsi_cond                           (rtx, rtx, rtx, rtx);
extern rtx        gen_negdi_cond                           (rtx, rtx, rtx, rtx);
extern rtx        gen_swapbsi2                             (rtx, rtx);
extern rtx        gen_zero_extendqihi2                     (rtx, rtx);
extern rtx        gen_push_fpul                            (void);
extern rtx        gen_pop_fpul                             (void);
extern rtx        gen_clrt                                 (void);
extern rtx        gen_sett                                 (void);
extern rtx        gen_movsi_i                              (rtx, rtx);
extern rtx        gen_movsi_ie                             (rtx, rtx);
extern rtx        gen_movsi_i_lowpart                      (rtx, rtx);
extern rtx        gen_load_ra                              (rtx, rtx);
extern rtx        gen_ic_invalidate_line_i                 (rtx, rtx);
extern rtx        gen_ic_invalidate_line_sh4a              (rtx);
extern rtx        gen_movdf_k                              (rtx, rtx);
extern rtx        gen_movdf_i4                             (rtx, rtx);
extern rtx        gen_movsf_i                              (rtx, rtx);
extern rtx        gen_movsf_ie                             (rtx, rtx);
extern rtx        gen_movsf_ie_ra                          (rtx, rtx);
extern rtx        gen_block_branch_redirect                (rtx);
extern rtx        gen_indirect_jump_scratch                (rtx, rtx);
extern rtx        gen_stuff_delay_slot                     (rtx, rtx);
extern rtx        gen_doloop_end_split                     (rtx, rtx, rtx);
extern rtx        gen_jump_compact                         (rtx);
extern rtx        gen_calli                                (rtx, rtx);
extern rtx        gen_calli_fdpic                          (rtx, rtx);
extern rtx        gen_calli_tbr_rel                        (rtx, rtx);
extern rtx        gen_calli_pcrel                          (rtx, rtx, rtx);
extern rtx        gen_call_pcrel                           (rtx, rtx);
extern rtx        gen_call_valuei                          (rtx, rtx, rtx);
extern rtx        gen_call_valuei_fdpic                    (rtx, rtx, rtx);
extern rtx        gen_call_valuei_tbr_rel                  (rtx, rtx, rtx);
extern rtx        gen_call_valuei_pcrel                    (rtx, rtx, rtx, rtx);
extern rtx        gen_call_value_pcrel                     (rtx, rtx, rtx);
extern rtx        gen_sibcalli                             (rtx, rtx);
extern rtx        gen_sibcalli_fdpic                       (rtx, rtx);
extern rtx        gen_sibcalli_pcrel                       (rtx, rtx, rtx);
extern rtx        gen_sibcalli_pcrel_fdpic                 (rtx, rtx, rtx);
extern rtx        gen_sibcalli_thunk                       (rtx, rtx);
extern rtx        gen_sibcall_pcrel                        (rtx, rtx);
extern rtx        gen_sibcall_pcrel_fdpic                  (rtx, rtx);
extern rtx        gen_sibcall_valuei                       (rtx, rtx, rtx);
extern rtx        gen_sibcall_valuei_fdpic                 (rtx, rtx, rtx);
extern rtx        gen_sibcall_valuei_pcrel                 (rtx, rtx, rtx, rtx);
extern rtx        gen_sibcall_valuei_pcrel_fdpic           (rtx, rtx, rtx, rtx);
extern rtx        gen_sibcall_value_pcrel                  (rtx, rtx, rtx);
extern rtx        gen_sibcall_value_pcrel_fdpic            (rtx, rtx, rtx);
extern rtx        gen_indirect_jump_compact                (rtx);
extern rtx        gen_casesi_jump_1                        (rtx, rtx);
extern rtx        gen_casesi_jump_2                        (rtx, rtx, rtx);
extern rtx        gen_dect                                 (rtx, rtx);
extern rtx        gen_nop                                  (void);
extern rtx        gen_mova                                 (rtx);
extern rtx        gen_mova_const                           (rtx);
extern rtx        gen_chk_guard_add                        (rtx, rtx);
extern rtx        gen_tls_global_dynamic                   (rtx, rtx);
extern rtx        gen_tls_local_dynamic                    (rtx, rtx);
extern rtx        gen_tls_initial_exec                     (rtx, rtx);
extern rtx        gen_store_gbr                            (rtx);
extern rtx        gen_load_gbr                             (rtx);
extern rtx        gen_casesi_worker_0                      (rtx, rtx, rtx);
extern rtx        gen_casesi_worker_1                      (rtx, rtx, rtx);
extern rtx        gen_casesi_worker_2                      (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_eh_set_ra_si                         (rtx);
extern rtx        gen_blockage                             (void);
extern rtx        gen_movml_push_banked                    (rtx);
extern rtx        gen_movml_pop_banked                     (rtx);
extern rtx        gen_movt                                 (rtx, rtx);
extern rtx        gen_movrt                                (rtx, rtx);
extern rtx        gen_movrt_negc                           (rtx, rtx, rtx);
extern rtx        gen_movrt_xor                            (rtx, rtx);
extern rtx        gen_mov_neg_si_t                         (rtx, rtx);
extern rtx        gen_nott                                 (rtx);
extern rtx        gen_any_treg_expr_to_reg                 (rtx, rtx);
extern rtx        gen_consttable_2                         (rtx, rtx);
extern rtx        gen_consttable_4                         (rtx, rtx);
extern rtx        gen_consttable_8                         (rtx, rtx);
extern rtx        gen_consttable_sf                        (rtx, rtx);
extern rtx        gen_consttable_df                        (rtx, rtx);
extern rtx        gen_align_log                            (rtx);
extern rtx        gen_consttable_end                       (void);
extern rtx        gen_consttable_window_end                (rtx);
extern rtx        gen_clipu_one                            (rtx, rtx);
extern rtx        gen_block_move_real                      (rtx, rtx);
extern rtx        gen_block_lump_real                      (rtx, rtx);
extern rtx        gen_block_move_real_i4                   (rtx, rtx);
extern rtx        gen_block_lump_real_i4                   (rtx, rtx);
extern rtx        gen_cmpstr_t                             (rtx, rtx);
extern rtx        gen_lds_fpscr                            (rtx);
extern rtx        gen_sts_fpscr                            (rtx);
extern rtx        gen_toggle_sz                            (void);
extern rtx        gen_toggle_pr                            (void);
extern rtx        gen_addsf3_i                             (rtx, rtx, rtx);
extern rtx        gen_subsf3_i                             (rtx, rtx, rtx);
extern rtx        gen_mulsf3_i                             (rtx, rtx, rtx);
extern rtx        gen_fmasf4_i                             (rtx, rtx, rtx, rtx);
extern rtx        gen_divsf3_i                             (rtx, rtx, rtx);
extern rtx        gen_floatsisf2_i4                        (rtx, rtx);
extern rtx        gen_fix_truncsfsi2_i4                    (rtx, rtx);
extern rtx        gen_cmpgtsf_t                            (rtx, rtx);
extern rtx        gen_cmpeqsf_t                            (rtx, rtx);
extern rtx        gen_ieee_ccmpeqsf_t                      (rtx, rtx);
extern rtx        gen_sqrtsf2_i                            (rtx, rtx);
extern rtx        gen_rsqrtsf2                             (rtx, rtx);
extern rtx        gen_fsca                                 (rtx, rtx, rtx);
extern rtx        gen_adddf3_i                             (rtx, rtx, rtx);
extern rtx        gen_subdf3_i                             (rtx, rtx, rtx);
extern rtx        gen_muldf3_i                             (rtx, rtx, rtx);
extern rtx        gen_divdf3_i                             (rtx, rtx, rtx);
extern rtx        gen_floatsidf2_i                         (rtx, rtx);
extern rtx        gen_fix_truncdfsi2_i                     (rtx, rtx);
extern rtx        gen_cmpgtdf_t                            (rtx, rtx);
extern rtx        gen_cmpeqdf_t                            (rtx, rtx);
extern rtx        gen_sqrtdf2_i                            (rtx, rtx);
extern rtx        gen_extendsfdf2_i4                       (rtx, rtx);
extern rtx        gen_truncdfsf2_i4                        (rtx, rtx);
extern rtx        gen_movua                                (rtx, rtx);
extern rtx        gen_bclr_m2a                             (rtx, rtx);
extern rtx        gen_bclrmem_m2a                          (rtx, rtx);
extern rtx        gen_bset_m2a                             (rtx, rtx);
extern rtx        gen_bsetmem_m2a                          (rtx, rtx);
extern rtx        gen_bst_m2a                              (rtx, rtx);
extern rtx        gen_bld_m2a                              (rtx, rtx);
extern rtx        gen_bldsign_m2a                          (rtx, rtx);
extern rtx        gen_bldqi_reg                            (rtx, rtx);
extern rtx        gen_bldhi_reg                            (rtx, rtx);
extern rtx        gen_bldsi_reg                            (rtx, rtx);
extern rtx        gen_band_m2a                             (rtx, rtx);
extern rtx        gen_bandreg_m2a                          (rtx, rtx, rtx, rtx);
extern rtx        gen_bor_m2a                              (rtx, rtx);
extern rtx        gen_borreg_m2a                           (rtx, rtx, rtx, rtx);
extern rtx        gen_bxor_m2a                             (rtx, rtx);
extern rtx        gen_bxorreg_m2a                          (rtx, rtx, rtx, rtx);
extern rtx        gen_sp_switch_1                          (rtx);
extern rtx        gen_sp_switch_2                          (void);
extern rtx        gen_stack_protect_set_si                 (rtx, rtx);
extern rtx        gen_stack_protect_test_si                (rtx, rtx);
extern rtx        gen_atomic_compare_and_swapsi_hard       (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swapqi_hard       (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swaphi_hard       (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swapqi_hard_1     (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swaphi_hard_1     (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swapqi_soft_gusa  (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swaphi_soft_gusa  (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swapsi_soft_gusa  (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swapqi_soft_tcb   (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swaphi_soft_tcb   (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swapsi_soft_tcb   (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swapqi_soft_imask (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swaphi_soft_imask (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swapsi_soft_imask (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_exchangesi_hard               (rtx, rtx, rtx);
extern rtx        gen_atomic_exchangeqi_hard               (rtx, rtx, rtx);
extern rtx        gen_atomic_exchangehi_hard               (rtx, rtx, rtx);
extern rtx        gen_atomic_exchangeqi_hard_1             (rtx, rtx, rtx);
extern rtx        gen_atomic_exchangehi_hard_1             (rtx, rtx, rtx);
extern rtx        gen_atomic_exchangeqi_soft_gusa          (rtx, rtx, rtx);
extern rtx        gen_atomic_exchangehi_soft_gusa          (rtx, rtx, rtx);
extern rtx        gen_atomic_exchangesi_soft_gusa          (rtx, rtx, rtx);
extern rtx        gen_atomic_exchangeqi_soft_tcb           (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_exchangehi_soft_tcb           (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_exchangesi_soft_tcb           (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_exchangeqi_soft_imask         (rtx, rtx, rtx);
extern rtx        gen_atomic_exchangehi_soft_imask         (rtx, rtx, rtx);
extern rtx        gen_atomic_exchangesi_soft_imask         (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_addsi_hard              (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_subsi_hard              (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_orsi_hard               (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_xorsi_hard              (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_andsi_hard              (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_notsi_hard              (rtx, rtx);
extern rtx        gen_atomic_fetch_addqi_hard              (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_subqi_hard              (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_orqi_hard               (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_xorqi_hard              (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_andqi_hard              (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_addhi_hard              (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_subhi_hard              (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_orhi_hard               (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_xorhi_hard              (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_andhi_hard              (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_addqi_hard_1            (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_subqi_hard_1            (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_orqi_hard_1             (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_xorqi_hard_1            (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_andqi_hard_1            (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_addhi_hard_1            (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_subhi_hard_1            (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_orhi_hard_1             (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_xorhi_hard_1            (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_andhi_hard_1            (rtx, rtx, rtx);
extern rtx        gen_atomic_addqi_hard                    (rtx, rtx);
extern rtx        gen_atomic_subqi_hard                    (rtx, rtx);
extern rtx        gen_atomic_orqi_hard                     (rtx, rtx);
extern rtx        gen_atomic_xorqi_hard                    (rtx, rtx);
extern rtx        gen_atomic_andqi_hard                    (rtx, rtx);
extern rtx        gen_atomic_addhi_hard                    (rtx, rtx);
extern rtx        gen_atomic_subhi_hard                    (rtx, rtx);
extern rtx        gen_atomic_orhi_hard                     (rtx, rtx);
extern rtx        gen_atomic_xorhi_hard                    (rtx, rtx);
extern rtx        gen_atomic_andhi_hard                    (rtx, rtx);
extern rtx        gen_atomic_addqi_hard_1                  (rtx, rtx);
extern rtx        gen_atomic_subqi_hard_1                  (rtx, rtx);
extern rtx        gen_atomic_orqi_hard_1                   (rtx, rtx);
extern rtx        gen_atomic_xorqi_hard_1                  (rtx, rtx);
extern rtx        gen_atomic_andqi_hard_1                  (rtx, rtx);
extern rtx        gen_atomic_addhi_hard_1                  (rtx, rtx);
extern rtx        gen_atomic_subhi_hard_1                  (rtx, rtx);
extern rtx        gen_atomic_orhi_hard_1                   (rtx, rtx);
extern rtx        gen_atomic_xorhi_hard_1                  (rtx, rtx);
extern rtx        gen_atomic_andhi_hard_1                  (rtx, rtx);
extern rtx        gen_atomic_fetch_notqi_hard              (rtx, rtx);
extern rtx        gen_atomic_fetch_nothi_hard              (rtx, rtx);
extern rtx        gen_atomic_notqi_hard                    (rtx);
extern rtx        gen_atomic_nothi_hard                    (rtx);
extern rtx        gen_atomic_fetch_addqi_soft_gusa         (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_subqi_soft_gusa         (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_orqi_soft_gusa          (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_xorqi_soft_gusa         (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_andqi_soft_gusa         (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_addhi_soft_gusa         (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_subhi_soft_gusa         (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_orhi_soft_gusa          (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_xorhi_soft_gusa         (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_andhi_soft_gusa         (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_addsi_soft_gusa         (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_subsi_soft_gusa         (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_orsi_soft_gusa          (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_xorsi_soft_gusa         (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_andsi_soft_gusa         (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_notqi_soft_gusa         (rtx, rtx);
extern rtx        gen_atomic_fetch_nothi_soft_gusa         (rtx, rtx);
extern rtx        gen_atomic_fetch_notsi_soft_gusa         (rtx, rtx);
extern rtx        gen_atomic_fetch_addqi_soft_tcb          (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_subqi_soft_tcb          (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_orqi_soft_tcb           (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_xorqi_soft_tcb          (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_andqi_soft_tcb          (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_addhi_soft_tcb          (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_subhi_soft_tcb          (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_orhi_soft_tcb           (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_xorhi_soft_tcb          (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_andhi_soft_tcb          (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_addsi_soft_tcb          (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_subsi_soft_tcb          (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_orsi_soft_tcb           (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_xorsi_soft_tcb          (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_andsi_soft_tcb          (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_addqi_soft_tcb                (rtx, rtx, rtx);
extern rtx        gen_atomic_subqi_soft_tcb                (rtx, rtx, rtx);
extern rtx        gen_atomic_orqi_soft_tcb                 (rtx, rtx, rtx);
extern rtx        gen_atomic_xorqi_soft_tcb                (rtx, rtx, rtx);
extern rtx        gen_atomic_andqi_soft_tcb                (rtx, rtx, rtx);
extern rtx        gen_atomic_addhi_soft_tcb                (rtx, rtx, rtx);
extern rtx        gen_atomic_subhi_soft_tcb                (rtx, rtx, rtx);
extern rtx        gen_atomic_orhi_soft_tcb                 (rtx, rtx, rtx);
extern rtx        gen_atomic_xorhi_soft_tcb                (rtx, rtx, rtx);
extern rtx        gen_atomic_andhi_soft_tcb                (rtx, rtx, rtx);
extern rtx        gen_atomic_addsi_soft_tcb                (rtx, rtx, rtx);
extern rtx        gen_atomic_subsi_soft_tcb                (rtx, rtx, rtx);
extern rtx        gen_atomic_orsi_soft_tcb                 (rtx, rtx, rtx);
extern rtx        gen_atomic_xorsi_soft_tcb                (rtx, rtx, rtx);
extern rtx        gen_atomic_andsi_soft_tcb                (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_notqi_soft_tcb          (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_nothi_soft_tcb          (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_notsi_soft_tcb          (rtx, rtx, rtx);
extern rtx        gen_atomic_notqi_soft_tcb                (rtx, rtx);
extern rtx        gen_atomic_nothi_soft_tcb                (rtx, rtx);
extern rtx        gen_atomic_notsi_soft_tcb                (rtx, rtx);
extern rtx        gen_atomic_fetch_addqi_soft_imask        (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_subqi_soft_imask        (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_orqi_soft_imask         (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_xorqi_soft_imask        (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_andqi_soft_imask        (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_addhi_soft_imask        (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_subhi_soft_imask        (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_orhi_soft_imask         (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_xorhi_soft_imask        (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_andhi_soft_imask        (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_addsi_soft_imask        (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_subsi_soft_imask        (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_orsi_soft_imask         (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_xorsi_soft_imask        (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_andsi_soft_imask        (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_notqi_soft_imask        (rtx, rtx);
extern rtx        gen_atomic_fetch_nothi_soft_imask        (rtx, rtx);
extern rtx        gen_atomic_fetch_notsi_soft_imask        (rtx, rtx);
extern rtx        gen_atomic_fetch_nandsi_hard             (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_nandqi_hard             (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_nandhi_hard             (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_nandqi_hard_1           (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_nandhi_hard_1           (rtx, rtx, rtx);
extern rtx        gen_atomic_nandqi_hard                   (rtx, rtx);
extern rtx        gen_atomic_nandhi_hard                   (rtx, rtx);
extern rtx        gen_atomic_nandqi_hard_1                 (rtx, rtx);
extern rtx        gen_atomic_nandhi_hard_1                 (rtx, rtx);
extern rtx        gen_atomic_fetch_nandqi_soft_gusa        (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_nandhi_soft_gusa        (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_nandsi_soft_gusa        (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_nandqi_soft_tcb         (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_nandhi_soft_tcb         (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_nandsi_soft_tcb         (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_nandqi_soft_tcb               (rtx, rtx, rtx);
extern rtx        gen_atomic_nandhi_soft_tcb               (rtx, rtx, rtx);
extern rtx        gen_atomic_nandsi_soft_tcb               (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_nandqi_soft_imask       (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_nandhi_soft_imask       (rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_nandsi_soft_imask       (rtx, rtx, rtx);
extern rtx        gen_atomic_add_fetchsi_hard              (rtx, rtx, rtx);
extern rtx        gen_atomic_sub_fetchsi_hard              (rtx, rtx, rtx);
extern rtx        gen_atomic_or_fetchsi_hard               (rtx, rtx, rtx);
extern rtx        gen_atomic_xor_fetchsi_hard              (rtx, rtx, rtx);
extern rtx        gen_atomic_and_fetchsi_hard              (rtx, rtx, rtx);
extern rtx        gen_atomic_not_fetchsi_hard              (rtx, rtx);
extern rtx        gen_atomic_add_fetchqi_hard              (rtx, rtx, rtx);
extern rtx        gen_atomic_sub_fetchqi_hard              (rtx, rtx, rtx);
extern rtx        gen_atomic_or_fetchqi_hard               (rtx, rtx, rtx);
extern rtx        gen_atomic_xor_fetchqi_hard              (rtx, rtx, rtx);
extern rtx        gen_atomic_and_fetchqi_hard              (rtx, rtx, rtx);
extern rtx        gen_atomic_add_fetchhi_hard              (rtx, rtx, rtx);
extern rtx        gen_atomic_sub_fetchhi_hard              (rtx, rtx, rtx);
extern rtx        gen_atomic_or_fetchhi_hard               (rtx, rtx, rtx);
extern rtx        gen_atomic_xor_fetchhi_hard              (rtx, rtx, rtx);
extern rtx        gen_atomic_and_fetchhi_hard              (rtx, rtx, rtx);
extern rtx        gen_atomic_add_fetchqi_hard_1            (rtx, rtx, rtx);
extern rtx        gen_atomic_sub_fetchqi_hard_1            (rtx, rtx, rtx);
extern rtx        gen_atomic_or_fetchqi_hard_1             (rtx, rtx, rtx);
extern rtx        gen_atomic_xor_fetchqi_hard_1            (rtx, rtx, rtx);
extern rtx        gen_atomic_and_fetchqi_hard_1            (rtx, rtx, rtx);
extern rtx        gen_atomic_add_fetchhi_hard_1            (rtx, rtx, rtx);
extern rtx        gen_atomic_sub_fetchhi_hard_1            (rtx, rtx, rtx);
extern rtx        gen_atomic_or_fetchhi_hard_1             (rtx, rtx, rtx);
extern rtx        gen_atomic_xor_fetchhi_hard_1            (rtx, rtx, rtx);
extern rtx        gen_atomic_and_fetchhi_hard_1            (rtx, rtx, rtx);
extern rtx        gen_atomic_not_fetchqi_hard              (rtx, rtx);
extern rtx        gen_atomic_not_fetchhi_hard              (rtx, rtx);
extern rtx        gen_atomic_add_fetchqi_soft_gusa         (rtx, rtx, rtx);
extern rtx        gen_atomic_sub_fetchqi_soft_gusa         (rtx, rtx, rtx);
extern rtx        gen_atomic_or_fetchqi_soft_gusa          (rtx, rtx, rtx);
extern rtx        gen_atomic_xor_fetchqi_soft_gusa         (rtx, rtx, rtx);
extern rtx        gen_atomic_and_fetchqi_soft_gusa         (rtx, rtx, rtx);
extern rtx        gen_atomic_add_fetchhi_soft_gusa         (rtx, rtx, rtx);
extern rtx        gen_atomic_sub_fetchhi_soft_gusa         (rtx, rtx, rtx);
extern rtx        gen_atomic_or_fetchhi_soft_gusa          (rtx, rtx, rtx);
extern rtx        gen_atomic_xor_fetchhi_soft_gusa         (rtx, rtx, rtx);
extern rtx        gen_atomic_and_fetchhi_soft_gusa         (rtx, rtx, rtx);
extern rtx        gen_atomic_add_fetchsi_soft_gusa         (rtx, rtx, rtx);
extern rtx        gen_atomic_sub_fetchsi_soft_gusa         (rtx, rtx, rtx);
extern rtx        gen_atomic_or_fetchsi_soft_gusa          (rtx, rtx, rtx);
extern rtx        gen_atomic_xor_fetchsi_soft_gusa         (rtx, rtx, rtx);
extern rtx        gen_atomic_and_fetchsi_soft_gusa         (rtx, rtx, rtx);
extern rtx        gen_atomic_not_fetchqi_soft_gusa         (rtx, rtx);
extern rtx        gen_atomic_not_fetchhi_soft_gusa         (rtx, rtx);
extern rtx        gen_atomic_not_fetchsi_soft_gusa         (rtx, rtx);
extern rtx        gen_atomic_add_fetchqi_soft_tcb          (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_sub_fetchqi_soft_tcb          (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_or_fetchqi_soft_tcb           (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_xor_fetchqi_soft_tcb          (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_and_fetchqi_soft_tcb          (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_add_fetchhi_soft_tcb          (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_sub_fetchhi_soft_tcb          (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_or_fetchhi_soft_tcb           (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_xor_fetchhi_soft_tcb          (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_and_fetchhi_soft_tcb          (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_add_fetchsi_soft_tcb          (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_sub_fetchsi_soft_tcb          (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_or_fetchsi_soft_tcb           (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_xor_fetchsi_soft_tcb          (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_and_fetchsi_soft_tcb          (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_not_fetchqi_soft_tcb          (rtx, rtx, rtx);
extern rtx        gen_atomic_not_fetchhi_soft_tcb          (rtx, rtx, rtx);
extern rtx        gen_atomic_not_fetchsi_soft_tcb          (rtx, rtx, rtx);
extern rtx        gen_atomic_add_fetchqi_soft_imask        (rtx, rtx, rtx);
extern rtx        gen_atomic_sub_fetchqi_soft_imask        (rtx, rtx, rtx);
extern rtx        gen_atomic_or_fetchqi_soft_imask         (rtx, rtx, rtx);
extern rtx        gen_atomic_xor_fetchqi_soft_imask        (rtx, rtx, rtx);
extern rtx        gen_atomic_and_fetchqi_soft_imask        (rtx, rtx, rtx);
extern rtx        gen_atomic_add_fetchhi_soft_imask        (rtx, rtx, rtx);
extern rtx        gen_atomic_sub_fetchhi_soft_imask        (rtx, rtx, rtx);
extern rtx        gen_atomic_or_fetchhi_soft_imask         (rtx, rtx, rtx);
extern rtx        gen_atomic_xor_fetchhi_soft_imask        (rtx, rtx, rtx);
extern rtx        gen_atomic_and_fetchhi_soft_imask        (rtx, rtx, rtx);
extern rtx        gen_atomic_add_fetchsi_soft_imask        (rtx, rtx, rtx);
extern rtx        gen_atomic_sub_fetchsi_soft_imask        (rtx, rtx, rtx);
extern rtx        gen_atomic_or_fetchsi_soft_imask         (rtx, rtx, rtx);
extern rtx        gen_atomic_xor_fetchsi_soft_imask        (rtx, rtx, rtx);
extern rtx        gen_atomic_and_fetchsi_soft_imask        (rtx, rtx, rtx);
extern rtx        gen_atomic_not_fetchqi_soft_imask        (rtx, rtx);
extern rtx        gen_atomic_not_fetchhi_soft_imask        (rtx, rtx);
extern rtx        gen_atomic_not_fetchsi_soft_imask        (rtx, rtx);
extern rtx        gen_atomic_nand_fetchsi_hard             (rtx, rtx, rtx);
extern rtx        gen_atomic_nand_fetchqi_hard             (rtx, rtx, rtx);
extern rtx        gen_atomic_nand_fetchhi_hard             (rtx, rtx, rtx);
extern rtx        gen_atomic_nand_fetchqi_hard_1           (rtx, rtx, rtx);
extern rtx        gen_atomic_nand_fetchhi_hard_1           (rtx, rtx, rtx);
extern rtx        gen_atomic_nand_fetchqi_soft_gusa        (rtx, rtx, rtx);
extern rtx        gen_atomic_nand_fetchhi_soft_gusa        (rtx, rtx, rtx);
extern rtx        gen_atomic_nand_fetchsi_soft_gusa        (rtx, rtx, rtx);
extern rtx        gen_atomic_nand_fetchqi_soft_tcb         (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_nand_fetchhi_soft_tcb         (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_nand_fetchsi_soft_tcb         (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_nand_fetchqi_soft_imask       (rtx, rtx, rtx);
extern rtx        gen_atomic_nand_fetchhi_soft_imask       (rtx, rtx, rtx);
extern rtx        gen_atomic_nand_fetchsi_soft_imask       (rtx, rtx, rtx);
extern rtx        gen_tasb                                 (rtx);
extern rtx        gen_atomic_test_and_set_soft_gusa        (rtx, rtx);
extern rtx        gen_atomic_test_and_set_soft_tcb         (rtx, rtx, rtx);
extern rtx        gen_atomic_test_and_set_soft_imask       (rtx, rtx);
extern rtx        gen_atomic_test_and_set_hard             (rtx, rtx);
extern rtx        gen_cbranchsi4                           (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchdi4                           (rtx, rtx, rtx, rtx);
extern rtx        gen_movsicc                              (rtx, rtx, rtx, rtx);
extern rtx        gen_addsi3                               (rtx, rtx, rtx);
extern rtx        gen_subsi3                               (rtx, rtx, rtx);
extern rtx        gen_udivsi3                              (rtx, rtx, rtx);
extern rtx        gen_divsi3                               (rtx, rtx, rtx);
extern rtx        gen_mulsi3                               (rtx, rtx, rtx);
extern rtx        gen_mulsidi3                             (rtx, rtx, rtx);
extern rtx        gen_umulsidi3                            (rtx, rtx, rtx);
extern rtx        gen_andsi3                               (rtx, rtx, rtx);
extern rtx        gen_iorsi3                               (rtx, rtx, rtx);
extern rtx        gen_rotrsi3                              (rtx, rtx, rtx);
extern rtx        gen_rotlsi3                              (rtx, rtx, rtx);
extern rtx        gen_rotlhi3                              (rtx, rtx, rtx);
extern rtx        gen_ashlsi3                              (rtx, rtx, rtx);
extern rtx        gen_ashlhi3                              (rtx, rtx, rtx);
extern rtx        gen_ashldi3                              (rtx, rtx, rtx);
extern rtx        gen_ashldi3_std                          (rtx, rtx, rtx);
extern rtx        gen_ashrsi3                              (rtx, rtx, rtx);
extern rtx        gen_ashrdi3                              (rtx, rtx, rtx);
extern rtx        gen_lshrsi3                              (rtx, rtx, rtx);
extern rtx        gen_lshrdi3                              (rtx, rtx, rtx);
extern rtx        gen_bswapsi2                             (rtx, rtx);
extern rtx        gen_zero_extendqisi2                     (rtx, rtx);
extern rtx        gen_zero_extendhisi2                     (rtx, rtx);
extern rtx        gen_extendqisi2                          (rtx, rtx);
extern rtx        gen_extendhisi2                          (rtx, rtx);
extern rtx        gen_extendqihi2                          (rtx, rtx);
extern rtx        gen_push                                 (rtx);
extern rtx        gen_pop                                  (rtx);
extern rtx        gen_push_e                               (rtx);
extern rtx        gen_push_4                               (rtx);
extern rtx        gen_pop_e                                (rtx);
extern rtx        gen_pop_4                                (rtx);
extern rtx        gen_push_fpscr                           (void);
extern rtx        gen_pop_fpscr                            (void);
extern rtx        gen_movsi                                (rtx, rtx);
extern rtx        gen_ic_invalidate_line                   (rtx);
extern rtx        gen_movqi                                (rtx, rtx);
extern rtx        gen_movhi                                (rtx, rtx);
extern rtx        gen_movdi                                (rtx, rtx);
extern rtx        gen_reload_indf__frn                     (rtx, rtx, rtx);
extern rtx        gen_reload_outdf__RnFRm                  (rtx, rtx, rtx);
extern rtx        gen_movdf                                (rtx, rtx);
extern rtx        gen_movsf                                (rtx, rtx);
extern rtx        gen_reload_insf__frn                     (rtx, rtx, rtx);
extern rtx        gen_reload_insi__i_fpul                  (rtx, rtx, rtx);
extern rtx        gen_branch_true                          (rtx);
extern rtx        gen_branch_false                         (rtx);
extern rtx        gen_doloop_end                           (rtx, rtx);
extern rtx        gen_jump                                 (rtx);
extern rtx        gen_call                                 (rtx, rtx, rtx);
extern rtx        gen_call_value                           (rtx, rtx, rtx, rtx);
extern rtx        gen_sibcall                              (rtx, rtx, rtx);
extern rtx        gen_sibcall_value                        (rtx, rtx, rtx, rtx);
extern rtx        gen_sibcall_epilogue                     (void);
extern rtx        gen_indirect_jump                        (rtx);
extern rtx        gen_untyped_call                         (rtx, rtx, rtx);
extern rtx        gen_GOTaddr2picreg                       (rtx);
static inline rtx gen_vxworks_picreg                       (rtx, rtx);
static inline rtx
gen_vxworks_picreg(rtx ARG_UNUSED (a), rtx ARG_UNUSED (b))
{
  return 0;
}
extern rtx        gen_builtin_setjmp_receiver              (rtx);
extern rtx        gen_call_site                            (void);
extern rtx        gen_sym_label2reg                        (rtx, rtx, rtx);
extern rtx        gen_symPCREL_label2reg                   (rtx, rtx, rtx);
extern rtx        gen_symGOT_load                          (rtx, rtx);
extern rtx        gen_sym2GOT                              (rtx);
extern rtx        gen_symGOT2reg                           (rtx, rtx);
extern rtx        gen_sym2GOTFUNCDESC                      (rtx);
extern rtx        gen_symGOTFUNCDESC2reg                   (rtx, rtx);
extern rtx        gen_symGOTPLT2reg                        (rtx, rtx);
extern rtx        gen_sym2GOTOFF                           (rtx);
extern rtx        gen_symGOTOFF2reg                        (rtx, rtx);
extern rtx        gen_sym2GOTOFFFUNCDESC                   (rtx);
extern rtx        gen_symGOTOFFFUNCDESC2reg                (rtx, rtx);
extern rtx        gen_symPLT_label2reg                     (rtx, rtx, rtx);
extern rtx        gen_sym2PIC                              (rtx);
extern rtx        gen_sym2DTPOFF                           (rtx);
extern rtx        gen_symDTPOFF2reg                        (rtx, rtx, rtx);
extern rtx        gen_sym2GOTTPOFF                         (rtx);
extern rtx        gen_sym2TPOFF                            (rtx);
extern rtx        gen_symTPOFF2reg                         (rtx, rtx);
extern rtx        gen_get_thread_pointersi                 (rtx);
extern rtx        gen_set_thread_pointersi                 (rtx);
extern rtx        gen_casesi                               (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_casesi_0                             (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_simple_return                        (void);
extern rtx        gen_return                               (void);
extern rtx        gen_prologue                             (void);
extern rtx        gen_epilogue                             (void);
extern rtx        gen_eh_return                            (rtx);
extern rtx        gen_cstoresi4                            (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoredi4                            (rtx, rtx, rtx, rtx);
extern rtx        gen_movnegt                              (rtx, rtx);
extern rtx        gen_cstoresf4                            (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoredf4                            (rtx, rtx, rtx, rtx);
extern rtx        gen_align_2                              (void);
extern rtx        gen_align_4                              (void);
extern rtx        gen_sminsi3                              (rtx, rtx, rtx);
extern rtx        gen_smaxsi3                              (rtx, rtx, rtx);
extern rtx        gen_uminsi3                              (rtx, rtx, rtx);
extern rtx        gen_movmemsi                             (rtx, rtx, rtx, rtx);
extern rtx        gen_cmpstrsi                             (rtx, rtx, rtx, rtx);
extern rtx        gen_cmpstrnsi                            (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_strlensi                             (rtx, rtx, rtx, rtx);
extern rtx        gen_setmemqi                             (rtx, rtx, rtx, rtx);
extern rtx        gen_set_fpscr                            (rtx);
extern rtx        gen_addsf3                               (rtx, rtx, rtx);
extern rtx        gen_subsf3                               (rtx, rtx, rtx);
extern rtx        gen_mulsf3                               (rtx, rtx, rtx);
extern rtx        gen_fmasf4                               (rtx, rtx, rtx, rtx);
extern rtx        gen_divsf3                               (rtx, rtx, rtx);
extern rtx        gen_floatsisf2                           (rtx, rtx);
extern rtx        gen_fix_truncsfsi2                       (rtx, rtx);
extern rtx        gen_cbranchsf4                           (rtx, rtx, rtx, rtx);
extern rtx        gen_negsf2                               (rtx, rtx);
extern rtx        gen_sqrtsf2                              (rtx, rtx);
extern rtx        gen_sincossf3                            (rtx, rtx, rtx);
extern rtx        gen_abssf2                               (rtx, rtx);
extern rtx        gen_adddf3                               (rtx, rtx, rtx);
extern rtx        gen_subdf3                               (rtx, rtx, rtx);
extern rtx        gen_muldf3                               (rtx, rtx, rtx);
extern rtx        gen_divdf3                               (rtx, rtx, rtx);
extern rtx        gen_floatsidf2                           (rtx, rtx);
extern rtx        gen_fix_truncdfsi2                       (rtx, rtx);
extern rtx        gen_cbranchdf4                           (rtx, rtx, rtx, rtx);
extern rtx        gen_negdf2                               (rtx, rtx);
extern rtx        gen_sqrtdf2                              (rtx, rtx);
extern rtx        gen_absdf2                               (rtx, rtx);
extern rtx        gen_extendsfdf2                          (rtx, rtx);
extern rtx        gen_truncdfsf2                           (rtx, rtx);
extern rtx        gen_insv                                 (rtx, rtx, rtx, rtx);
extern rtx        gen_extv                                 (rtx, rtx, rtx, rtx);
extern rtx        gen_extzv                                (rtx, rtx, rtx, rtx);
extern rtx        gen_prefetch                             (rtx, rtx, rtx);
extern rtx        gen_stack_protect_set                    (rtx, rtx);
extern rtx        gen_stack_protect_test                   (rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swapqi            (rtx, rtx, rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swaphi            (rtx, rtx, rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swapsi            (rtx, rtx, rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_exchangeqi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_exchangehi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_exchangesi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_addqi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_subqi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_orqi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_xorqi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_andqi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_addhi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_subhi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_orhi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_xorhi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_andhi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_addsi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_subsi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_orsi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_xorsi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_andsi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_nandqi                  (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_nandhi                  (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_fetch_nandsi                  (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_add_fetchqi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_sub_fetchqi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_or_fetchqi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_xor_fetchqi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_and_fetchqi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_add_fetchhi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_sub_fetchhi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_or_fetchhi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_xor_fetchhi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_and_fetchhi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_add_fetchsi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_sub_fetchsi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_or_fetchsi                    (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_xor_fetchsi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_and_fetchsi                   (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_nand_fetchqi                  (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_nand_fetchhi                  (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_nand_fetchsi                  (rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_test_and_set                  (rtx, rtx, rtx);

#endif /* GCC_INSN_FLAGS_H */
