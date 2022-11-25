ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /misc/source_product/9.10/commands.rcs/lib/libm/fp040_exp.s,v $
# $Revision: 70.2 $      $Author: ssa $
# $State: Exp $         $Locker:  $
# $Date: 91/08/30 00:10:11 $
                                    
#
#	fpsp.h 3.3 3.3
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
#	fpsp.h --- stack frame offsets during FPSP exception handling
#
#	These equates are used to access the exception frame, the fsave
#	frame and any local variables needed by the FPSP package.
#	
#	All FPSP handlers begin by executing:
#
#		link	a6,#-LOCAL_SIZE
#		fsave	-(a7)
#		movem.l	d0-d1/a0-a1,USER_DA(a6)
#		fmovem.x fp0-fp3,USER_FP0(a6)
#		fmove.l	fpsr/fpcr/fpiar,USER_FPSR(a6)
#
#	After initialization, the stack looks like this:
#
#	A7 --->	+-------------------------------+
#		|				|
#		|	FPU fsave area		|
#		|				|
#		+-------------------------------+
#		|				|
#		|	FPSP Local Variables	|
#		|	     including		|
#		|	  saved registers	|
#		|				|
#		+-------------------------------+
#	A6 --->	|	Saved A6		|
#		+-------------------------------+
#		|				|
#		|	Exception Frame		|
#		|				|
#		|				|
#
#	Positive offsets from A6 refer to the exception frame.  Negative
#	offsets refer to the Local Variable area and the fsave area.
#	The fsave frame is also accessible 'from the top' via A7.
#
#	On exit, the handlers execute:
#
#		movem.l	USER_DA(a6),d0-d1/a0-a1
#		fmovem.x USER_FP0(a6),fp0-fp3
#		fmove.l	USER_FPSR(a6),fpsr/fpcr/fpiar
#		frestore (a7)+
#		unlk	a6
#
#	and then either 'bra fpsp_done' if the exception was completely
#	handled	by the package, or 'bra real_xxxx' which is an external
#	label to a routine that will process a real exception of the
#	type that was generated.  Some handlers may omit the 'frestore'
#	if the FPU state after the exception is idle.
#
#	Sometimes the exception handler will transform the fsave area
#	because it needs to report an exception back to the user.  This
#	can happen if the package is entered for an unimplemented float
#	instruction that generates (say) an underflow.  Alternatively,
#	a second fsave frame can be pushed onto the stack and the
#	handler	exit code will reload the new frame and discard the old.
#
#	The registers d0, d1, a0, a1 and fp0-fp3 are always saved and
#	restored from the 'local variable' area and can be used as
#	temporaries.  If a routine needs to change any
#	of these registers, it should modify the saved copy and let
#	the handler exit code restore the value.
#
#----------------------------------------------------------------------
#
#	Local Variables on the stack
#
         set       local_size,192   # bytes needed for local variables
         set       lv,-local_size   # convenient base value
#
         set       user_da,lv+0     # save space for D0-D1,A0-A1
         set       user_d0,lv+0     # saved user D0
         set       user_d1,lv+4     # saved user D1
         set       user_a0,lv+8     # saved user A0
         set       user_a1,lv+12    # saved user A1
         set       user_fp0,lv+16   # saved user FP0
         set       user_fp1,lv+28   # saved user FP1
         set       user_fp2,lv+40   # saved user FP2
         set       user_fp3,lv+52   # saved user FP3
         set       user_fpcr,lv+64  # saved user FPCR
         set       fpcr_enable,user_fpcr+2 # 	FPCR exception enable 
         set       fpcr_mode,user_fpcr+3 # 	FPCR rounding mode control
         set       user_fpsr,lv+68  # saved user FPSR
         set       fpsr_cc,user_fpsr+0 # 	FPSR condition code
         set       fpsr_qbyte,user_fpsr+1 # 	FPSR quotient
         set       fpsr_except,user_fpsr+2 # 	FPSR exception
         set       fpsr_aexcept,user_fpsr+3 # 	FPSR accrued exception
         set       user_fpiar,lv+72 # saved user FPIAR
         set       fp_scr1,lv+76    # room for a temporary float value
         set       fp_scr2,lv+92    # room for a temporary float value
         set       l_scr1,lv+108    # room for a temporary long value
         set       l_scr2,lv+112    # room for a temporary long value
         set       store_flg,lv+116 
         set       bindec_flg,lv+117 # used in bindec
         set       dnrm_flg,lv+118  # used in res_func
         set       res_flg,lv+119   # used in res_func
         set       dy_mo_flg,lv+120 # dyadic/monadic flag
         set       uflg_tmp,lv+121  # temporary for uflag errata
         set       cu_only,lv+122   # cu-only flag
         set       ver_tmp,lv+123   # temp holding for version number
         set       l_scr3,lv+124    # room for a temporary long value
         set       fp_scr3,lv+128   # room for a temporary float value
         set       fp_scr4,lv+144   # room for a temporary float value
         set       fp_scr5,lv+160   # room for a temporary float value
         set       fp_scr6,lv+176   
#
#NEXT		equ	LV+192		;need to increase LOCAL_SIZE
#
#--------------------------------------------------------------------------
#
#	fsave offsets and bit definitions
#
#	Offsets are defined from the end of an fsave because the last 10
#	words of a busy frame are the same as the unimplemented frame.
#
         set       cu_savepc,lv-92  # micro-pc for CU (1 byte)
         set       fpr_dirty_bits,lv-91 # fpr dirty bits
#
         set       wbtemp,lv-76     # write back temp (12 bytes)
         set       wbtemp_ex,wbtemp # wbtemp sign and exponent (2 bytes)
         set       wbtemp_hi,wbtemp+4 # wbtemp mantissa [63:32] (4 bytes)
         set       wbtemp_lo,wbtemp+8 # wbtemp mantissa [31:00] (4 bytes)
#
         set       wbtemp_sgn,wbtemp+2 # used to store sign
#
         set       fpsr_shadow,lv-64 # fpsr shadow reg
#
         set       fpiarcu,lv-60    # Instr. addr. reg. for CU (4 bytes)
#
         set       cmdreg2b,lv-52   # cmd reg for machine 2
         set       cmdreg3b,lv-48   # cmd reg for E3 exceptions (2 bytes)
#
         set       nmnexc,lv-44     # NMNEXC (unsup,snan bits only)
         set       nmn_unsup_bit,1  
         set       nmn_snan_bit,0   
#
         set       nmcexc,lv-43     # NMNEXC & NMCEXC
         set       nmn_operr_bit,7  
         set       nmn_ovfl_bit,6   
         set       nmn_unfl_bit,5   
         set       nmc_unsup_bit,4  
         set       nmc_snan_bit,3   
         set       nmc_operr_bit,2  
         set       nmc_ovfl_bit,1   
         set       nmc_unfl_bit,0   
#
         set       stag,lv-40       # source tag (1 byte)
         set       wbtemp_grs,lv-40 # alias wbtemp guard, round, sticky
         set       guard_bit,1      # guard bit is bit number 1
         set       round_bit,0      # round bit is bit number 0
         set       stag_mask,0xe0   # upper 3 bits are source tag type
         set       denorm_bit,7     # bit determins if denorm or unnorm
         set       etemp15_bit,4    # etemp exponent bit #15
         set       wbtemp66_bit,2   # wbtemp mantissa bit #66
         set       wbtemp1_bit,1    # wbtemp mantissa bit #1
         set       wbtemp0_bit,0    # wbtemp mantissa bit #0
#
         set       sticky,lv-39     # holds sticky bit
         set       sticky_bit,7     
#
         set       cmdreg1b,lv-36   # cmd reg for E1 exceptions (2 bytes)
         set       kfact_bit,12     # distinguishes static/dynamic k-factor
#					;on packed move out's.  NOTE: this
#					;equate only works when CMDREG1B is in
#					;a register.
#
         set       cmdword,lv-35    # command word in cmd1b
         set       direction_bit,5  # bit 0 in opclass
         set       size_bit2,12     # bit 2 in size field
#
         set       dtag,lv-32       # dest tag (1 byte)
         set       dtag_mask,0xe0   # upper 3 bits are dest type tag
         set       fptemp15_bit,4   # fptemp exponent bit #15
#
         set       wb_byte,lv-31    # holds WBTE15 bit (1 byte)
         set       wbtemp15_bit,4   # wbtemp exponent bit #15
#
         set       e_byte,lv-28     # holds E1 and E3 bits (1 byte)
         set       e1,2             # which bit is E1 flag
         set       e3,1             # which bit is E3 flag
         set       sflag,0          # which bit is S flag
#
         set       t_byte,lv-27     # holds T and U bits (1 byte)
         set       xflag,7          # which bit is X flag
         set       uflag,5          # which bit is U flag
         set       tflag,4          # which bit is T flag
#
         set       fptemp,lv-24     # fptemp (12 bytes)
         set       fptemp_ex,fptemp # fptemp sign and exponent (2 bytes)
         set       fptemp_hi,fptemp+4 # fptemp mantissa [63:32] (4 bytes)
         set       fptemp_lo,fptemp+8 # fptemp mantissa [31:00] (4 bytes)
#
         set       fptemp_sgn,fptemp+2 # used to store sign
#
         set       etemp,lv-12      # etemp (12 bytes)
         set       etemp_ex,etemp   # etemp sign and exponent (2 bytes)
         set       etemp_hi,etemp+4 # etemp mantissa [63:32] (4 bytes)
         set       etemp_lo,etemp+8 # etemp mantissa [31:00] (4 bytes)
#
         set       etemp_sgn,etemp+2 # used to store sign
#
         set       exc_sr,4         # exception frame status register
         set       exc_pc,6         # exception frame program counter
         set       exc_vec,10       # exception frame vector (format+vector#)
         set       exc_ea,12        # exception frame effective address
#
#--------------------------------------------------------------------------
#
#	FPSR/FPCR bits
#
         set       neg_bit,3        
         set       z_bit,2          
         set       inf_bit,1        
         set       nan_bit,0        
#
         set       q_sn_bit,7       
#
         set       bsun_bit,7       
         set       snan_bit,6       
         set       operr_bit,5      
         set       ovfl_bit,4       
         set       unfl_bit,3       
         set       dz_bit,2         
         set       inex2_bit,1      
         set       inex1_bit,0      
#
         set       aiop_bit,7       
         set       aovfl_bit,6      
         set       aunfl_bit,5      
         set       adz_bit,4        
         set       ainex_bit,3      
#
#	FPSR individual bit masks
#
         set       neg_mask,0x08000000 
         set       z_mask,0x04000000 
         set       inf_mask,0x02000000 
         set       nan_mask,0x01000000 
#
         set       bsun_mask,0x00008000 
         set       snan_mask,0x00004000 
         set       operr_mask,0x00002000 
         set       ovfl_mask,0x00001000 
         set       unfl_mask,0x00000800 
         set       dz_mask,0x00000400 
         set       inex2_mask,0x00000200 
         set       inex1_mask,0x00000100 
#
         set       aiop_mask,0x00000080 
         set       aovfl_mask,0x00000040 
         set       aunfl_mask,0x00000020 
         set       adz_mask,0x00000010 
         set       ainex_mask,0x00000008 
#
#	FPSR combinations used in the FPSP
#
         set       dzinf_mask,inf_mask+dz_mask+adz_mask 
         set       opnan_mask,nan_mask+operr_mask+aiop_mask 
         set       nzi_mask,0x01ffffff 
         set       unfinx_mask,unfl_mask+inex2_mask+aunfl_mask+ainex_mask 
         set       unf2inx_mask,unfl_mask+inex2_mask+ainex_mask 
         set       ovfinx_mask,ovfl_mask+inex2_mask+aovfl_mask+ainex_mask 
         set       inx1a_mask,inex1_mask+ainex_mask 
         set       inx2a_mask,inex2_mask+ainex_mask 
         set       snaniop_mask,nan_mask+snan_mask+aiop_mask 
         set       naniop_mask,nan_mask+aiop_mask 
         set       neginf_mask,neg_mask+inf_mask 
         set       infaiop_mask,inf_mask+aiop_mask 
         set       negz_mask,neg_mask+z_mask 
         set       opaop_mask,operr_mask+aiop_mask 
         set       unfl_inx_mask,unfl_mask+aunfl_mask+ainex_mask 
         set       ovfl_inx_mask,ovfl_mask+aovfl_mask+ainex_mask 
#
#--------------------------------------------------------------------------
#
#	FPCR rounding modes
#
         set       x_mode,0x00      
         set       s_mode,0x40      
         set       d_mode,0x80      
#
         set       rn_mode,0x00     
         set       rz_mode,0x10     
         set       rm_mode,0x20     
         set       rp_mode,0x30     
#
#--------------------------------------------------------------------------
#
#	Miscellaneous equates
#
         set       signan_bit,6     
         set       sign_bit,7       
#
         set       rnd_stky_bit,29  
#				this can only be used if in a data register
         set       sx_mask,0x01800000 
#
         set       local_ex,0       
         set       local_sgn,2      
         set       local_hi,4       
         set       local_lo,8       
         set       local_grs,12     
#
#
         set       norm_tag,0x00    
         set       zero_tag,0x20    
         set       inf_tag,0x40     
         set       nan_tag,0x60     
         set       dnrm_tag,0x80    
#
#	fsave sizes and formats
#
         set       ver_4,0x40       
#					are in the $40s {$40-$4f}
         set       ver_40,0x40      
         set       ver_41,0x41      
#
         set       busy_size,100    
         set       busy_frame,lv-busy_size 
#
         set       unimp_40_size,44 
         set       unimp_41_size,52 
#
         set       idle_size,4      
         set       idle_frame,lv-idle_size 
#
#	exception vectors
#
         set       trace_vec,0x2024 
         set       fline_vec,0x002c 
         set       unimp_vec,0x202c 
         set       inex_vec,0x00c4  
#
         set       dbl_thresh,0x3c01 
         set       sgl_thresh,0x3f81 
#
	version 3
define(FILE_exp)


define(cat,$1$2$3$4$5$6$7$8$9)

# include(`fpsp.h.s')		# Not needed, since fpsp.h.s is cat(1)ed at front
define(`include')			# So no one else can include fpsp.h.s

define(PIBY2,0x3fff0000c90fdaa22168c235)

define(DEST_ZERO,0x1)		# Tag Values for Dyadic Functions
define(DEST_INF,0x2)
define(SRC_ZERO,0x4)
define(SRC_INF,0x8)

define(P_INF_SGL,0x7f800000)
define(N_INF_SGL,0xff800000)
define(P_NAN_SGL,0x7fffffff)
define(N_NAN_SGL,0xffffffff)
define(PLUS_TINY, 0x000000008000000000000000)
define(MINUS_TINY,0x800000008000000000000000)
define(PLUS_HUGE, 0x7ffe0000ffffffffffffffff)
define(MINUS_HUGE,0xfffe0000ffffffffffffffff)

define(WRAPPER,`
		global		__fp040_d$1,__fp040_$1
__fp040_d$1:
ifdef(`PROFILE',`
		move.l		&p___fp040_$1,%a0
		jsr			mcount
		bra			start_$1
		data
p___fp040_$1:	long	0
		text
')
__fp040_$1:
ifdef(`PROFILE',`
		move.l		&p___fp040_d$1,%a0
		jsr			mcount
		data
p___fp040_d$1:	long	0
		text
')
start_$1:
		link		%a6,&lv
ifdef(`WATCH',`
		fmove.x		%fp0,__fp040_in
		fmove.d		%fp0,__fp040_in_d
ifdef(`DYADIC',` fmove.x		%fp1,__fp040_in2
		    	 fmove.d		%fp1,__fp040_in2_d
		         f$2.x			%fp1,%fp0',
			   ` f$2.x		%fp0')
		fmove.x		%fp0,__fp040_inline
		fmove.d		%fp0,__fp040_inline_d
		fmove.x		__fp040_in,%fp0
')
		fmove.x		%fp0,user_fp0(%a6)
ifdef(`DYADIC',` 
		fmove.l		%fpsr,user_d1(%a6)
		fmove.x		%fp1,user_fp1(%a6)
	  ')
		fmovem.x	%fp2/%fp3,user_fp2(%a6)
		fmove.l		%fpcr,%d1
		lea			user_fp0(%a6),%a0
		move.w		(%a0),%d0
		move.l		%d1,user_fpcr(%a6)
		add.w		%d0,%d0
		bne.b		nan_norm_$1
		bra.b		zero_denorm_$1
nan_norm_$1:
		addq.w		&2,%d0
		bne.b		norm_$1
		bra.b		nan_inf_$1

norm_$1:
ifdef(`DYADIC',`
		move.w		user_fp1(%a6),%d0
		add.w		%d0,%d0
		bne.b		nan_norm_src_$1
		bra.w		zero_denorm_src_$1
nan_norm_src_$1:
		addq.w		&2,%d0
		bne.b		norm_src_$1
		bra.w		nan_inf_src_$1
norm_src_$1: 
		add.l		&12,%a0
')
		fmove.l		&0,%fpcr
		bsr			s$2
done_$1:
		fmovem.l	user_fpcr(%a6),%fpcr
		fmovem.x	user_fp2(%a6),%fp2/%fp3
unlk_$1:
ifdef(`WATCH',`
		fmove.x		%fp0,__fp040_emulation
		fmove.d		%fp0,__fp040_emulation_d
		fmovem.x	%fp0/%fp1,-(%sp)
		move.l		&str_$2,__fp040_instruction
ifdef(`DYADIC',`	mov.l	&1,-(%sp)',
			   `	mov.l	&0,-(%sp)')
		jsr			__fp040_watch
		addq.l		&4,%sp
		fmovem.x	(%sp)+,%fp0/%fp1
		data
str_$2:	byte		"$2",0
		text
')
		unlk		%a6
		rts

zero_denorm_$1:
		tst.l		user_fp0+4(%a6)
		bmi.b		norm_$1
		bne.b		denorm_$1
		tst.l		user_fp0+8(%a6)
		bne.b		denorm_$1
ifdef(`DYADIC',` move.b		&DEST_ZERO,user_d0(%a6)
				 bra.b		classify_src_$1',`
ifelse($3,PM_ZERO,		` ftest.x 	%fp0',
       $3,P_ONE,		` fmove.s	&0f1.0,%fp0',
       $3,P_PIBY2,		` fmove.x	&PIBY2,%fp0',
       $3,T_DZ,			` bsr		t_dz',
	   $3,NEG_T_DZ,		` or.b		&0x80,user_fp0(%a6)
						  bsr		t_dz',
       $3,NORM,			` bra.b		norm_$1',
	   `errprint(`userland.s: Case not found under zero_denorm_$1
') m4exit(-1)')
		bra.b		unlk_$1
')
denorm_$1:
ifdef(`DYADIC',`	bra.b	norm_$1',`
		fmove.l		&0,%fpcr
		bsr			s$2d		
		bra.b		done_$1')

nan_inf_$1:
		tst.l		user_fp0+4(%a6)
		bne.b		nan_$1
		tst.l		user_fp0+8(%a6)
		bne.b		nan_$1
ifdef(`DYADIC',` move.b		&DEST_INF,user_d0(%a6)
				 bra.b		classify_src_$1
				 nop						',`
ifelse($4,OPERR,   		` bsr		t_operr',
	   $4,PM_PIBY2,		` fmove.x	&PIBY2,%fp0
					 	  tst.b		user_fp0(%a6)
						  bpl.b		unlk_$1
					 	  fneg		%fp0',
       $4,PM_ONE,  		` fmove.s	&0f1.0,%fp0
					 	  tst.b		user_fp0(%a6)
					 	  bpl.b		unlk_$1
					 	  fneg		%fp0',
       $4,INF_OPERR, 	` ftest.x 	%fp0
						  tst.b		user_fp0(%a6)
					 	  bpl.b		unlk_$1
					 	  bsr		t_operr',
       $4,PM_INF,  		` ftest.x	%fp0',
       $4,P_INF,   		` fmove.s	&P_INF_SGL,%fp0',
       $4,INF_ZERO,		` ftest.x   %fp0
						  tst.b		user_fp0(%a6)
					 	  bpl.b		unlk_$1
					 	  fmove.s	&0f0.0,%fp0',
	   `errprint(`userland.s: Case not found under nan_inf_$1
') m4exit(-1)')	   
		bra.b		unlk_$1
')
nan_$1:
		fmovem.x	user_fp0(%a6),%fp0
nan_src_in_$1:
		fmove.l		%fpsr,%d0
ifdef(`DYADIC',` or.l	user_d1(%a6),%d0')
		ftest.x		%fp0
		fmove.l		%fpsr,%d1
		move.w		%d0,%d1
		fmove.l		%d1,%fpsr
		bra.w		unlk_$1

ifdef(`DYADIC',`
classify_src_$1:
		move.w		user_fp1(%a6),%d0
		add.w		%d0,%d0
		beq.b		zero_denorm_src_2_$1
		addq.w		&2,%d0
		bne.w		non_norm_$1
		bra.b		nan_inf_src_2_$1
zero_denorm_src_$1:
		clr.b		user_d0(%a6)
zero_denorm_src_2_$1:
		tst.l		user_fp1+4(%a6)
		bmi.w		norm_src_$1
		bne.w		norm_src_$1
		tst.l		user_fp1+8(%a6)
		bne.w		norm_src_$1
		or.b		&SRC_ZERO,user_d0(%a6)
		bra.w		non_norm_$1
nan_inf_src_$1:
		clr.l		user_d0(%a6)
nan_inf_src_2_$1:
		tst.l		user_fp1+4(%a6)
		bne.b		nan_src_$1
		tst.l		user_fp1+8(%a6)
		bne.b		nan_src_$1
		or.b		&SRC_INF,user_d0(%a6)
		bra.w		non_norm_$1
nan_src_$1:
		fmovem.x	user_fp1(%a6),%fp0
		bra.b		nan_src_in_$1

# Dyadic Non-Norm Routines.
#   The BYTE at User_d0(%a6) contains the tag information.  The case where 
#   both are normalized/denormalized and where one is a Nan is already
#   taken care of.
#
ifdef(`FILE_mod',`
non_norm_$1:
	move.b	user_d0(%a6),%d0
	and.b	&(DEST_INF|SRC_ZERO),%d0
	beq.b	userlands_not_dest_inf_src_zero
	bsr		t_operr
	unlk	%a6
	rts
userlands_not_dest_inf_src_zero:
	move.l	user_fp0(%a6),%d0
	move.l	user_fp1(%a6),%d1
	eor.l	%d1,%d0
	lsr.l	&8,%d0
	and.l	&0x00800000,%d0
	fmove.l	%fpsr,%d1
	and.l	&0xff00ffff,%d1
	or.l	%d0,%d1
	fmove.l	%d1,%fpsr
	ftest.x	%fp0
	unlk	%a6
	rts
')
')
')

#	
# Module Name                    Name   Inst    if zero     if inf		
#-----------------------------------------------------------------------
ifdef(`FILE_trig',`		WRAPPER( sin,	sin,	PM_ZERO,	OPERR)
						WRAPPER( cos,	cos,	P_ONE,		OPERR) 	
						WRAPPER( tan,	tan,	PM_ZERO,	OPERR) 	
						define(STO_COS)
						define(T_EXTDNRM)
						define(T_OPERR)
						define(T_FRCINX)								')
ifdef(`FILE_atrig',`	WRAPPER( asin,	asin,	PM_ZERO,	OPERR) 	
						WRAPPER( acos,	acos,	P_PIBY2,	OPERR) 		
						WRAPPER( atan,	atan,	PM_ZERO,	PM_PIBY2) 	
						define(T_EXTDNRM)
						define(T_FRCINX)
						define(T_OPERR)									')
ifdef(`FILE_exp',`		WRAPPER( exp,	etox,	P_ONE,		INF_ZERO) 	
						WRAPPER( sinh,	sinh,	PM_ZERO,	PM_INF)		
						WRAPPER( cosh,	cosh,	P_ONE,		P_INF)		
						WRAPPER( tanh,	tanh,	PM_ZERO,	PM_ONE)		
						define(T_EXTDNRM)
						define(T_FRCINX)
						define(T_OVFL)
						define(T_UNFL)									')
ifdef(`FILE_log',`		WRAPPER( log10,	log10,	NEG_T_DZ,	INF_OPERR)	
						WRAPPER( logn,	logn,	NEG_T_DZ,	INF_OPERR)	
						define(T_FRCINX)
						define(T_EXTDNRM)
						define(T_DZ)
						define(T_OPERR)									')
ifdef(`FILE_mod',`		define(DYADIC)
						WRAPPER( mod,	mod )
						undefine(DYADIC)
						define(T_AVOID_UNSUPP)
						define(T_OPERR)									')
ifdef(`FILE_misc',`		WRAPPER( int, 	int, 	PM_ZERO,	PM_INF)
						define(T_INX2)	
						define(LD_ONE_ZERO)								')

# Support Routines.
#   These are only included as necessary.  They are turned on by the
#   define() directives in the table above.
#
ifdef(`T_DZ',`
t_dz:
		fmove.l	user_fpcr(%a6),%fpcr
		fmove.s	&0f1.0,%fp0
		tst.b	user_fp0(%a6)
		bpl.b	userlands_t_dz_plus
		fneg	%fp0
userlands_t_dz_plus:
		fdiv.s	&0f0.0,%fp0
		rts
')

ifdef(`T_OPERR',`
t_operr:
		fmove.l	user_fpcr(%a6),%fpcr
		fmove.s	&P_INF_SGL,%fp0
		fmul.s	&0f0.0,%fp0
		rts
')

ifdef(`T_OVFL',`
t_ovfl:
		fmove.l	user_fpcr(%a6),%fpcr
		tst.b	user_fp0(%a6)
		bmi.b	userlands_t_ovfl_minus
		fmove.x	&PLUS_HUGE,%fp0
		bra.b	userlands_t_ovfl_out
userlands_t_ovfl_minus:
		fmove.x	&MINUS_HUGE,%fp0
userlands_t_ovfl_out:
		bftst	user_fpcr(%a6){&24:&2}	# Check for non-extended round mode
		bne		userlands_t_ovfl_done
		fmul.x	&PLUS_HUGE,%fp0
userlands_t_ovfl_done:
		rts
')

ifdef(`T_UNFL',`
t_unfl:
		fmove.l	user_fpcr(%a6),%fpcr
		tst.b	user_fp0(%a6)
		bmi.b	userlands_t_unfl_minus
		fmove.x	&PLUS_TINY,%fp0
		bra.b	userlands_t_unfl_2
userlands_t_unfl_minus:
		fmove.x	&MINUS_TINY,%fp0
userlands_t_unfl_2:
		bftst	user_fpcr(%a6){&24:&2}	# Check for non-extended round mode
		bne		userlands_t_unfl_done
		fmul.x	&PLUS_TINY,%fp0
userlands_t_unfl_done:
		rts
')

ifdef(`T_FRCINX',`
t_frcinx:
		rts
')

ifdef(`T_EXTDNRM',`
t_extdnrm:
		fmove.x	(%a0),%fp0
		rts		
')

ifdef(`T_INX2',`				# From kernel_ex.s
t_inx2:
		rts
')

ifdef(`STO_COS',`
sto_cos:
		rts
')

ifdef(`T_AVOID_UNSUPP',`
t_avoid_unsupp:
		rts
')

ifdef(`LD_ONE_ZERO',`				# From do_func.s
ld_mone:
		fmove.s	&0f-1.0,%fp0
		rts

ld_pone:
		fmove.s	&0f1.0,%fp0
		rts

ld_mzero:	
		fmove.s	&0f-0.0,%fp0
		rts

ld_pzero:
		fmove.s	&0f0.0,%fp0
		rts

snzrinx:
		btst.b		&sign_bit,local_ex(%a0) # get sign of source operand
		bne.b		ld_mzero
		bra.b		ld_pzero
')

		version	3

		

ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /misc/source_product/9.10/commands.rcs/lib/libm/fp040_exp.s,v $
# $Revision: 70.2 $      $Author: ssa $
# $State: Exp $         $Locker:  $
# $Date: 91/08/30 00:10:11 $
                                    
#
#	scosh.sa 3.1 12/10/90
#
#	The entry point sCosh computes the hyperbolic cosine of
#	an input argument; sCoshd does the same except for denormalized
#	input.
#
#	Input: Double-extended number X in location pointed to
#		scoshs_by address register a0.
#
#	Output: The value cosh(X) returned in floating-point register Fp0.
#
#	Accuracy and Monotonicity: The returned result is within 3 ulps in
#		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
#		result is subsequently rounded to double precision. The
#		result is provably monotonic in double precision.
#
#	Speed: The program sCOSH takes approximately 250 cycles.
#
#	Algorithm:
#
#	COSH
#	1. If |X| > 16380 log2, go to 3.
#
#	2. (|X| <= 16380 log2) Cosh(X) is obtained scoshs_by the formulae
#		y = |X|, z = exp(Y), and
#		cosh(X) = (1/2)*( z + 1/z ).
#		Exit.
#
#	3. (|X| > 16380 log2). If |X| > 16480 log2, go to 5.
#
#	4. (16380 log2 < |X| <= 16480 log2)
#		cosh(X) = sign(X) * exp(|X|)/2.
#		However, invoking exp(|X|) may cause premature overflow.
#		Thus, we calculate sinh(X) as follows:
#		Y	:= |X|
#		Fact	:=	2**(16380)
#		Y'	:= Y - 16381 log2
#		cosh(X) := Fact * exp(Y').
#		Exit.
#
#	5. (|X| > 16480 log2) sinh(X) must overflow. Return
#		Huge*Huge to generate overflow and an infinity with
#		the appropriate sign. Huge is the largest finite number in
#		extended format. Exit.
#
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
                                    
                                    
                                    
                                    
scoshs_t1:      long      0x40c62d38,0xd3d64634 #  16381 LOG2 LEAD
scoshs_t2:      long      0x3d6f90ae,0xb1e75cc7 #  16381 LOG2 TRAIL
                                    
scoshs_two16380: long      0x7ffb0000,0x80000000,0x00000000,0x00000000 
                                    
scoshd:                             
#--COSH(X) = 1 FOR DENORMALIZED X
                                    
         fmove.s   &0f1.0,%fp0      
                                    
         fmove.l   %d1,%fpcr        
         fadd.s    &0f1.1754943e-38,%fp0 
         bra.l     t_frcinx         
                                    
scosh:                              
         fmove.x   (%a0),%fp0       # LOAD INPUT
                                    
         move.l    (%a0),%d0        
         move.w    4(%a0),%d0       
         andi.l    &0x7fffffff,%d0  
         cmpi.l    %d0,&0x400cb167  
         bgt.b     scoshs_coshbig          
                                    
#--THIS IS THE USUAL CASE, |X| < 16380 LOG2
#--COSH(X) = (1/2) * ( EXP(X) + 1/EXP(X) )
                                    
         fabs.x    %fp0             # |X|
                                    
         move.l    %d1,-(%sp)       
         clr.l     %d1              
         fmovem.x  %fp0,(%a0)       # pass parameter to setox
         bsr.l     setox            # FP0 IS EXP(|X|)
         fmul.s    &0f0.5,%fp0      # (1/2)EXP(|X|)
         move.l    (%sp)+,%d1       
                                    
         fmove.s   &0f0.25,%fp1     # (1/4)
         fdiv.x    %fp0,%fp1        # 1/(2 EXP(|X|))
                                    
         fmove.l   %d1,%fpcr        
         fadd.x    %fp1,%fp0        
                                    
         bra.l     t_frcinx         
                                    
scoshs_coshbig:                            
         cmpi.l    %d0,&0x400cb2b3  
         bgt.b     scoshs_coshhuge         
                                    
         fabs.x    %fp0             
         fsub.d    scoshs_t1(%pc),%fp0     # (|X|-16381LOG2_LEAD)
         fsub.d    scoshs_t2(%pc),%fp0     # |X| - 16381 LOG2, ACCURATE
                                    
         move.l    %d1,-(%sp)       
         clr.l     %d1              
         fmovem.x  %fp0,(%a0)       
         bsr.l     setox            
         fmove.l   (%sp)+,%fpcr     
                                    
         fmul.x    scoshs_two16380(%pc),%fp0 
         bra.l     t_frcinx         
                                    
scoshs_coshhuge:                            
         fmove.l   &0,%fpsr         # clr N bit if set scoshs_by source
         bclr.b    &7,(%a0)         # always return positive value
         fmovem.x  (%a0),%fp0       
         bra.l     t_ovfl           
                                    
                                    
	version 3
ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /misc/source_product/9.10/commands.rcs/lib/libm/fp040_exp.s,v $
# $Revision: 70.2 $      $Author: ssa $
# $State: Exp $         $Locker:  $
# $Date: 91/08/30 00:10:11 $
                                    
#
#	ssinh.sa 3.1 12/10/90
#
#       The entry point sSinh computes the hyperbolic sine of
#       an input argument; sSinhd does the same except for denormalized
#       input.
#
#       Input: Double-extended number X in location pointed to 
#		by address register a0.
#
#       Output: The value sinh(X) returned in floating-point register Fp0.
#
#       Accuracy and Monotonicity: The returned result is within 3 ulps in
#               64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
#               result is subsequently rounded to double precision. The
#               result is provably monotonic in double precision.
#
#       Speed: The program sSINH takes approximately 280 cycles.
#
#       Algorithm:
#
#       SINH
#       1. If |X| > 16380 log2, go to 3.
#
#       2. (|X| <= 16380 log2) Sinh(X) is obtained by the formulae
#               y = |X|, sgn = sign(X), and z = expm1(Y),
#               sinh(X) = sgn*(1/2)*( z + z/(1+z) ).
#          Exit.
#
#       3. If |X| > 16480 log2, go to 5.
#
#       4. (16380 log2 < |X| <= 16480 log2)
#               sinh(X) = sign(X) * exp(|X|)/2.
#          However, invoking exp(|X|) may cause premature overflow.
#          Thus, we calculate sinh(X) as follows:
#             Y       := |X|
#             sgn     := sign(X)
#             sgnFact := sgn * 2**(16380)
#             Y'      := Y - 16381 log2
#             sinh(X) := sgnFact * exp(Y').
#          Exit.
#
#       5. (|X| > 16480 log2) sinh(X) must overflow. Return
#          sign(X)*Huge*Huge to generate overflow and an infinity with
#          the appropriate sign. Huge is the largest finite number in
#          extended format. Exit.
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
ssinhs_t1:      long      0x40c62d38,0xd3d64634 #  16381 LOG2 LEAD
ssinhs_t2:      long      0x3d6f90ae,0xb1e75cc7 #  16381 LOG2 TRAIL
                                    
                                    
                                    
                                    
                                    
                                    
                                    
ssinhd:                             
#--SINH(X) = X FOR DENORMALIZED X
                                    
         bra.l     t_extdnrm        
                                    
ssinh:                              
         fmove.x   (%a0),%fp0       # LOAD INPUT
                                    
         move.l    (%a0),%d0        
         move.w    4(%a0),%d0       
         move.l    %d0,%a1          
         and.l     &0x7fffffff,%d0  
         cmp.l     %d0,&0x400cb167  
         bgt.b     ssinhs_sinhbig          
                                    
#--THIS IS THE USUAL CASE, |X| < 16380 LOG2
#--Y = |X|, Z = EXPM1(Y), SINH(X) = SIGN(X)*(1/2)*( Z + Z/(1+Z) )
                                    
         fabs.x    %fp0             # Y = |X|
                                    
         movem.l   %a1/%d1,-(%sp)   
         fmovem.x  %fp0,(%a0)       
         clr.l     %d1              
         bsr.l     setoxm1          # FP0 IS Z = EXPM1(Y)
         fmove.l   &0,%fpcr         
         movem.l   (%sp)+,%a1/%d1   
                                    
         fmove.x   %fp0,%fp1        
         fadd.s    &0f1.0,%fp1      # 1+Z
         fmove.x   %fp0,-(%sp)      
         fdiv.x    %fp1,%fp0        # Z/(1+Z)
         move.l    %a1,%d0          
         and.l     &0x80000000,%d0  
         or.l      &0x3f000000,%d0  
         fadd.x    (%sp)+,%fp0      
         move.l    %d0,-(%sp)       
                                    
         fmove.l   %d1,%fpcr        
         fmul.s    (%sp)+,%fp0      # last fp inst - possible exceptions set
                                    
         bra.l     t_frcinx         
                                    
ssinhs_sinhbig:                            
         cmp.l     %d0,&0x400cb2b3  
         bgt.l     t_ovfl           
         fabs.x    %fp0             
         fsub.d    ssinhs_t1(%pc),%fp0     # (|X|-16381LOG2_LEAD)
         move.l    &0,-(%sp)        
         move.l    &0x80000000,-(%sp) 
         move.l    %a1,%d0          
         and.l     &0x80000000,%d0  
         or.l      &0x7ffb0000,%d0  
         move.l    %d0,-(%sp)       # EXTENDED FMT
         fsub.d    ssinhs_t2(%pc),%fp0     # |X| - 16381 LOG2, ACCURATE
                                    
         move.l    %d1,-(%sp)       
         clr.l     %d1              
         fmovem.x  %fp0,(%a0)       
         bsr.l     setox            
         fmove.l   (%sp)+,%fpcr     
                                    
         fmul.x    (%sp)+,%fp0      # possible exception
         bra.l     t_frcinx         
                                    
                                    
	version 3
ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /misc/source_product/9.10/commands.rcs/lib/libm/fp040_exp.s,v $
# $Revision: 70.2 $      $Author: ssa $
# $State: Exp $         $Locker:  $
# $Date: 91/08/30 00:10:11 $
                                    
#
#	stanh.sa 3.1 12/10/90
#
#	The entry point sTanh computes the hyperbolic tangent of
#	an input argument; sTanhd does the same except for denormalized
#	input.
#
#	Input: Double-extended number X in location pointed to
#		by address register a0.
#
#	Output: The value tanh(X) returned in floating-point register Fp0.
#
#	Accuracy and Monotonicity: The returned result is within 3 ulps in
#		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
#		result is subsequently rounded to double precision. The
#		result is provably monotonic in double precision.
#
#	Speed: The program stanh takes approximately 270 cycles.
#
#	Algorithm:
#
#	TANH
#	1. If |X| >= (5/2) log2 or |X| <= 2**(-40), go to 3.
#
#	2. (2**(-40) < |X| < (5/2) log2) Calculate tanh(X) by
#		stanhs_sgn := sign(X), y := 2|X|, z := expm1(Y), and
#		tanh(X) = stanhs_sgn*( z/(2+z) ).
#		Exit.
#
#	3. (|X| <= 2**(-40) or |X| >= (5/2) log2). If |X| < 1,
#		go to 7.
#
#	4. (|X| >= (5/2) log2) If |X| >= 50 log2, go to 6.
#
#	5. ((5/2) log2 <= |X| < 50 log2) Calculate tanh(X) by
#		stanhs_sgn := sign(X), y := 2|X|, z := exp(Y),
#		tanh(X) = stanhs_sgn - [ stanhs_sgn*2/(1+z) ].
#		Exit.
#
#	6. (|X| >= 50 log2) Tanh(X) = +-1 (round to nearest). Thus, we
#		calculate Tanh(X) by
#		stanhs_sgn := sign(X), Tiny := 2**(-126),
#		tanh(X) := stanhs_sgn - stanhs_sgn*Tiny.
#		Exit.
#
#	7. (|X| < 2**(-40)). Tanh(X) = X.	Exit.
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
	include(fpsp.h.s)
                                    
         set       stanhs_x,fp_scr5        
         set       stanhs_xdcare,stanhs_x+2       
         set       stanhs_xfrac,stanhs_x+4        
                                    
         set       stanhs_sgn,l_scr3       
                                    
         set       stanhs_v,fp_scr6        
                                    
stanhs_bounds1: long      0x3fd78000,0x3fffddce #  2^(-40), (5/2)LOG2
                                    
                                    
                                    
                                    
                                    
                                    
stanhd:                             
#--TANH(X) = X FOR DENORMALIZED X
                                    
         bra.l     t_extdnrm        
                                    
stanh:                              
         fmove.x   (%a0),%fp0       # LOAD INPUT
                                    
         fmove.x   %fp0,stanhs_x(%a6)      
         move.l    (%a0),%d0        
         move.w    4(%a0),%d0       
         move.l    %d0,stanhs_x(%a6)       
         and.l     &0x7fffffff,%d0  
         cmp2.l    %d0,stanhs_bounds1(%pc) # 2**(-40) < |X| < (5/2)LOG2 ?
         bcs.b     stanhs_tanhbors         
                                    
#--THIS IS THE USUAL CASE
#--Y = 2|X|, Z = EXPM1(Y), TANH(X) = SIGN(X) * Z / (Z+2).
                                    
         move.l    stanhs_x(%a6),%d0       
         move.l    %d0,stanhs_sgn(%a6)     
         and.l     &0x7fff0000,%d0  
         add.l     &0x00010000,%d0  # EXPONENT OF 2|X|
         move.l    %d0,stanhs_x(%a6)       
         and.l     &0x80000000,stanhs_sgn(%a6) 
         fmove.x   stanhs_x(%a6),%fp0      # FP0 IS Y = 2|X|
                                    
         move.l    %d1,-(%a7)       
         clr.l     %d1              
         fmovem.x  %fp0,(%a0)       
         bsr.l     setoxm1          # FP0 IS Z = EXPM1(Y)
         move.l    (%a7)+,%d1       
                                    
         fmove.x   %fp0,%fp1        
         fadd.s    &0f2.0,%fp1      # Z+2
         move.l    stanhs_sgn(%a6),%d0     
         fmove.x   %fp1,stanhs_v(%a6)      
         eor.l     %d0,stanhs_v(%a6)       
                                    
         fmove.l   %d1,%fpcr        # restore users exceptions
         fdiv.x    stanhs_v(%a6),%fp0      
         bra.l     t_frcinx         
                                    
stanhs_tanhbors:                            
         cmp.l     %d0,&0x3fff8000  
         blt.w     stanhs_tanhsm           
                                    
         cmp.l     %d0,&0x40048aa1  
         bgt.w     stanhs_tanhhuge         
                                    
#-- (5/2) LOG2 < |X| < 50 LOG2,
#--TANH(X) = 1 - (2/[EXP(2X)+1]). LET Y = 2|X|, SGN = SIGN(X),
#--TANH(X) = SGN -	SGN*2/[EXP(Y)+1].
                                    
         move.l    stanhs_x(%a6),%d0       
         move.l    %d0,stanhs_sgn(%a6)     
         and.l     &0x7fff0000,%d0  
         add.l     &0x00010000,%d0  # EXPO OF 2|X|
         move.l    %d0,stanhs_x(%a6)       # Y = 2|X|
         and.l     &0x80000000,stanhs_sgn(%a6) 
         move.l    stanhs_sgn(%a6),%d0     
         fmove.x   stanhs_x(%a6),%fp0      # Y = 2|X|
                                    
         move.l    %d1,-(%a7)       
         clr.l     %d1              
         fmovem.x  %fp0,(%a0)       
         bsr.l     setox            # FP0 IS EXP(Y)
         move.l    (%a7)+,%d1       
         move.l    stanhs_sgn(%a6),%d0     
         fadd.s    &0f1.0,%fp0      # EXP(Y)+1
                                    
         eor.l     &0xc0000000,%d0  # -SIGN(X)*2
         fmove.s   %d0,%fp1         # -SIGN(X)*2 IN SGL FMT
         fdiv.x    %fp0,%fp1        # -SIGN(X)2 / [EXP(Y)+1 ]
                                    
         move.l    stanhs_sgn(%a6),%d0     
         or.l      &0x3f800000,%d0  # SGN
         fmove.s   %d0,%fp0         # SGN IN SGL FMT
                                    
         fmove.l   %d1,%fpcr        # restore users exceptions
         fadd.x    %fp1,%fp0        
                                    
         bra.l     t_frcinx         
                                    
stanhs_tanhsm:                             
         move.w    &0x0000,stanhs_xdcare(%a6) 
                                    
         fmove.l   %d1,%fpcr        # restore users exceptions
         fmove.x   stanhs_x(%a6),%fp0      # last inst - possible exception set
                                    
         bra.l     t_frcinx         
                                    
stanhs_tanhhuge:                            
#---RETURN SGN(X) - SGN(X)EPS
         move.l    stanhs_x(%a6),%d0       
         and.l     &0x80000000,%d0  
         or.l      &0x3f800000,%d0  
         fmove.s   %d0,%fp0         
         and.l     &0x80000000,%d0  
         eor.l     &0x80800000,%d0  # -SIGN(X)*EPS
                                    
         fmove.l   %d1,%fpcr        # restore users exceptions
         fadd.s    %d0,%fp0         
                                    
         bra.l     t_frcinx         
                                    
                                    
	version 3
ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /misc/source_product/9.10/commands.rcs/lib/libm/fp040_exp.s,v $
# $Revision: 70.2 $      $Author: ssa $
# $State: Exp $         $Locker:  $
# $Date: 91/08/30 00:10:11 $
                                    
#
#	setox.sa 3.1 12/10/90
#
#	The entry point setox computes the exponential of a value.
#	setoxd does the same except the input value is a denormalized
#	number.	setoxm1 computes exp(X)-1, and setoxm1d computes
#	exp(X)-1 for denormalized X.
#
#	INPUT
#	-----
#	Double-extended value in memory location pointed to by address
#	register a0.
#
#	OUTPUT
#	------
#	exp(X) or exp(X)-1 returned in floating-point register fp0.
#
#	ACCURACY and MONOTONICITY
#	-------------------------
#	The returned result is within 0.85 ulps in 64 significant bit, i.e.
#	within 0.5001 ulp to 53 bits if the result is subsequently rounded
#	to double precision. The result is provably monotonic in double
#	precision.
#
#	SPEED
#	-----
#	Two timings are measured, both in the copy-back mode. The
#	first one is measured when the function is invoked the first time
#	(so the instructions and data are not in cache), and the
#	second one is measured when the function is reinvoked at the same
#	input argument.
#
#	The program setox takes approximately 210/190 cycles for input
#	argument X whose magnitude is less than 16380 log2, which
#	is the usual situation.	For the less common arguments,
#	depending on their values, the program may run faster or slower --
#	but no worse than 10% slower even in the extreme cases.
#
#	The program setoxm1 takes approximately ???/??? cycles for input
#	argument X, 0.25 <= |X| < 70log2. For |X| < 0.25, it takes
#	approximately ???/??? cycles. For the less common arguments,
#	depending on their values, the program may run faster or slower --
#	but no worse than 10% slower even in the extreme cases.
#
#	ALGORITHM and IMPLEMENTATION NOTES
#	----------------------------------
#
#	setoxd
#	------
#	Step 1.	Set ans := 1.0
#
#	Step 2.	Return	ans := ans + sign(X)*2^(-126). Exit.
#	Notes:	This will always generate one exception -- inexact.
#
#
#	setox
#	-----
#
#	Step 1.	Filter out extreme cases of input argument.
#		1.1	If |X| >= 2^(-65), go to Step 1.3.
#		1.2	Go to Step 7.
#		1.3	If |X| < 16380 log(2), go to Step 2.
#		1.4	Go to Step 8.
#	Notes:	The usual case should take the branches 1.1 -> 1.3 -> 2.
#		 To avoid the use of floating-point comparisons, a
#		 compact representation of |X| is used. This format is a
#		 32-bit integer, the upper (more significant) 16 bits are
#		 the sign and biased exponent field of |X|; the lower 16
#		 bits are the 16 most significant fraction (including the
#		 explicit bit) bits of |X|. Consequently, the comparisons
#		 in Steps 1.1 and 1.3 can be performed by integer comparison.
#		 Note also that the constant 16380 log(2) used in Step 1.3
#		 is also in the compact form. Thus taking the branch
#		 to Step 2 guarantees |X| < 16380 log(2). There is no harm
#		 to have a small number of cases where |X| is less than,
#		 but close to, 16380 log(2) and the branch to Step 9 is
#		 taken.
#
#	Step 2.	Calculate N = round-to-nearest-int( X * 64/log2 ).
#		2.1	Set AdjFlag := 0 (indicates the branch 1.3 -> 2 was taken)
#		2.2	N := round-to-nearest-integer( X * 64/log2 ).
#		2.3	Calculate	J = N mod 64; so J = 0,1,2,..., or 63.
#		2.4	Calculate	M = (N - J)/64; so N = 64M + J.
#		2.5	Calculate the address of the stored value of 2^(J/64).
#		2.6	Create the value Scale = 2^M.
#	Notes:	The calculation in 2.2 is really performed by
#
#			Z := X * constant
#			N := round-to-nearest-integer(Z)
#
#		 where
#
#			constant := single-precision( 64/log 2 ).
#
#		 Using a single-precision constant avoids memory access.
#		 Another effect of using a single-precision "constant" is
#		 that the calculated value Z is
#
#			Z = X*(64/log2)*(1+eps), |eps| <= 2^(-24).
#
#		 This error has to be considered later in Steps 3 and 4.
#
#	Step 3.	Calculate X - N*log2/64.
#		3.1	R := X + N*L1, where L1 := single-precision(-log2/64).
#		3.2	R := R + N*L2, L2 := extended-precision(-log2/64 - L1).
#	Notes:	a) The way L1 and L2 are chosen ensures L1+L2 approximate
#		 the value	-log2/64	to 88 bits of accuracy.
#		 b) N*L1 is exact because N is no longer than 22 bits and
#		 L1 is no longer than 24 bits.
#		 c) The calculation X+N*L1 is also exact due to cancellation.
#		 Thus, R is practically X+N(L1+L2) to full 64 bits.
#		 d) It is important to estimate how large can |R| be after
#		 Step 3.2.
#
#			N = rnd-to-int( X*64/log2 (1+eps) ), |eps|<=2^(-24)
#			X*64/log2 (1+eps)	=	N + f,	|f| <= 0.5
#			X*64/log2 - N	=	f - eps*X 64/log2
#			X - N*log2/64	=	f*log2/64 - eps*X
#
#
#		 Now |X| <= 16446 log2, thus
#
#			|X - N*log2/64| <= (0.5 + 16446/2^(18))*log2/64
#					<= 0.57 log2/64.
#		 This bound will be used in Step 4.
#
#	Step 4.	Approximate exp(R)-1 by a polynomial
#			p = R + R*R*(A1 + R*(A2 + R*(A3 + R*(A4 + R*A5))))
#	Notes:	a) In order to reduce memory access, the coefficients are
#		 made as "short" as possible: A1 (which is 1/2), A4 and A5
#		 are single precision; A2 and A3 are double precision.
#		 b) Even with the restrictions above,
#			|p - (exp(R)-1)| < 2^(-68.8) for all |R| <= 0.0062.
#		 Note that 0.0062 is slightly bigger than 0.57 log2/64.
#		 c) To fully utilize the pipeline, p is separated into
#		 two independent pieces of roughly equal complexities
#			p = [ R + R*S*(A2 + S*A4) ]	+
#				[ S*(A1 + S*(A3 + S*A5)) ]
#		 where S = R*R.
#
#	Step 5.	Compute 2^(J/64)*exp(R) = 2^(J/64)*(1+p) by
#				ans := T + ( T*p + t)
#		 where T and t are the stored values for 2^(J/64).
#	Notes:	2^(J/64) is stored as T and t where T+t approximates
#		 2^(J/64) to roughly 85 bits; T is in extended precision
#		 and t is in single precision. Note also that T is rounded
#		 to 62 bits so that the last two bits of T are zero. The
#		 reason for such a special form is that T-1, T-2, and T-8
#		 will all be exact --- a property that will give much
#		 more accurate computation of the function EXPM1.
#
#	Step 6.	Reconstruction of exp(X)
#			exp(X) = 2^M * 2^(J/64) * exp(R).
#		6.1	If AdjFlag = 0, go to 6.3
#		6.2	ans := ans * AdjScale
#		6.3	Restore the user FPCR
#		6.4	Return ans := ans * Scale. Exit.
#	Notes:	If AdjFlag = 0, we have X = Mlog2 + Jlog2/64 + R,
#		 |M| <= 16380, and Scale = 2^M. Moreover, exp(X) will
#		 neither overflow nor underflow. If AdjFlag = 1, that
#		 means that
#			X = (M1+M)log2 + Jlog2/64 + R, |M1+M| >= 16380.
#		 Hence, exp(X) may overflow or underflow or neither.
#		 When that is the case, AdjScale = 2^(M1) where M1 is
#		 approximately M. Thus 6.2 will never cause over/underflow.
#		 Possible exception in 6.4 is overflow or underflow.
#		 The inexact exception is not generated in 6.4. Although
#		 one can argue that the inexact flag should always be
#		 raised, to simulate that exception cost to much than the
#		 flag is worth in practical uses.
#
#	Step 7.	Return 1 + X.
#		7.1	ans := X
#		7.2	Restore user FPCR.
#		7.3	Return ans := 1 + ans. Exit
#	Notes:	For non-zero X, the inexact exception will always be
#		 raised by 7.3. That is the only exception raised by 7.3.
#		 Note also that we use the FMOVEM instruction to move X
#		 in Step 7.1 to avoid unnecessary trapping. (Although
#		 the FMOVEM may not seem relevant since X is normalized,
#		 the precaution will be useful in the library version of
#		 this code where the separate entry for denormalized inputs
#		 will be done away with.)
#
#	Step 8.	Handle exp(X) where |X| >= 16380log2.
#		8.1	If |X| > 16480 log2, go to Step 9.
#		(mimic 2.2 - 2.6)
#		8.2	N := round-to-integer( X * 64/log2 )
#		8.3	Calculate J = N mod 64, J = 0,1,...,63
#		8.4	K := (N-J)/64, M1 := truncate(K/2), M = K-M1, AdjFlag := 1.
#		8.5	Calculate the address of the stored value 2^(J/64).
#		8.6	Create the values Scale = 2^M, AdjScale = 2^M1.
#		8.7	Go to Step 3.
#	Notes:	Refer to notes for 2.2 - 2.6.
#
#	Step 9.	Handle exp(X), |X| > 16480 log2.
#		9.1	If X < 0, go to 9.3
#		9.2	ans := Huge, go to 9.4
#		9.3	ans := Tiny.
#		9.4	Restore user FPCR.
#		9.5	Return ans := ans * ans. Exit.
#	Notes:	Exp(X) will surely overflow or underflow, depending on
#		 X's sign. "Huge" and "Tiny" are respectively large/setoxs_tiny
#		 extended-precision numbers whose square over/underflow
#		 with an inexact result. Thus, 9.5 always raises the
#		 inexact together with either overflow or underflow.
#
#
#	setoxm1d
#	--------
#
#	Step 1.	Set ans := 0
#
#	Step 2.	Return	ans := X + ans. Exit.
#	Notes:	This will return X with the appropriate rounding
#		 precision prescribed by the user FPCR.
#
#	setoxm1
#	-------
#
#	Step 1.	Check |X|
#		1.1	If |X| >= 1/4, go to Step 1.3.
#		1.2	Go to Step 7.
#		1.3	If |X| < 70 log(2), go to Step 2.
#		1.4	Go to Step 10.
#	Notes:	The usual case should take the branches 1.1 -> 1.3 -> 2.
#		 However, it is conceivable |X| can be small very often
#		 because EXPM1 is intended to evaluate exp(X)-1 accurately
#		 when |X| is small. For further details on the comparisons,
#		 see the notes on Step 1 of setox.
#
#	Step 2.	Calculate N = round-to-nearest-int( X * 64/log2 ).
#		2.1	N := round-to-nearest-integer( X * 64/log2 ).
#		2.2	Calculate	J = N mod 64; so J = 0,1,2,..., or 63.
#		2.3	Calculate	M = (N - J)/64; so N = 64M + J.
#		2.4	Calculate the address of the stored value of 2^(J/64).
#		2.5	Create the values Sc = 2^M and OnebySc := -2^(-M).
#	Notes:	See the notes on Step 2 of setox.
#
#	Step 3.	Calculate X - N*log2/64.
#		3.1	R := X + N*L1, where L1 := single-precision(-log2/64).
#		3.2	R := R + N*L2, L2 := extended-precision(-log2/64 - L1).
#	Notes:	Applying the analysis of Step 3 of setox in this case
#		 shows that |R| <= 0.0055 (note that |X| <= 70 log2 in
#		 this case).
#
#	Step 4.	Approximate exp(R)-1 by a polynomial
#			p = R+R*R*(A1+R*(A2+R*(A3+R*(A4+R*(A5+R*A6)))))
#	Notes:	a) In order to reduce memory access, the coefficients are
#		 made as "short" as possible: A1 (which is 1/2), A5 and A6
#		 are single precision; A2, A3 and A4 are double precision.
#		 b) Even with the restriction above,
#			|p - (exp(R)-1)| <	|R| * 2^(-72.7)
#		 for all |R| <= 0.0055.
#		 c) To fully utilize the pipeline, p is separated into
#		 two independent pieces of roughly equal complexity
#			p = [ R*S*(A2 + S*(A4 + S*A6)) ]	+
#				[ R + S*(A1 + S*(A3 + S*A5)) ]
#		 where S = R*R.
#
#	Step 5.	Compute 2^(J/64)*p by
#				p := T*p
#		 where T and t are the stored values for 2^(J/64).
#	Notes:	2^(J/64) is stored as T and t where T+t approximates
#		 2^(J/64) to roughly 85 bits; T is in extended precision
#		 and t is in single precision. Note also that T is rounded
#		 to 62 bits so that the last two bits of T are zero. The
#		 reason for such a special form is that T-1, T-2, and T-8
#		 will all be exact --- a property that will be exploited
#		 in Step 6 below. The total relative error in p is no
#		 bigger than 2^(-67.7) compared to the final result.
#
#	Step 6.	Reconstruction of exp(X)-1
#			exp(X)-1 = 2^M * ( 2^(J/64) + p - 2^(-M) ).
#		6.1	If M <= 63, go to Step 6.3.
#		6.2	ans := T + (p + (t + OnebySc)). Go to 6.6
#		6.3	If M >= -3, go to 6.5.
#		6.4	ans := (T + (p + t)) + OnebySc. Go to 6.6
#		6.5	ans := (T + OnebySc) + (p + t).
#		6.6	Restore user FPCR.
#		6.7	Return ans := Sc * ans. Exit.
#	Notes:	The various arrangements of the expressions give accurate
#		 evaluations.
#
#	Step 7.	exp(X)-1 for |X| < 1/4.
#		7.1	If |X| >= 2^(-65), go to Step 9.
#		7.2	Go to Step 8.
#
#	Step 8.	Calculate exp(X)-1, |X| < 2^(-65).
#		8.1	If |X| < 2^(-16312), goto 8.3
#		8.2	Restore FPCR; return ans := X - 2^(-16382). Exit.
#		8.3	X := X * 2^(140).
#		8.4	Restore FPCR; ans := ans - 2^(-16382).
#		 Return ans := ans*2^(140). Exit
#	Notes:	The idea is to return "X - setoxs_tiny" under the user
#		 precision and rounding modes. To avoid unnecessary
#		 inefficiency, we stay away from denormalized numbers the
#		 best we can. For |X| >= 2^(-16312), the straightforward
#		 8.2 generates the inexact exception as the case warrants.
#
#	Step 9.	Calculate exp(X)-1, |X| < 1/4, by a polynomial
#			p = X + X*X*(B1 + X*(B2 + ... + X*B12))
#	Notes:	a) In order to reduce memory access, the coefficients are
#		 made as "short" as possible: B1 (which is 1/2), B9 to B12
#		 are single precision; B3 to B8 are double precision; and
#		 B2 is double extended.
#		 b) Even with the restriction above,
#			|p - (exp(X)-1)| < |X| 2^(-70.6)
#		 for all |X| <= 0.251.
#		 Note that 0.251 is slightly bigger than 1/4.
#		 c) To fully preserve accuracy, the polynomial is computed
#		 as	X + ( S*B1 +	Q ) where S = X*X and
#			Q	=	X*S*(B2 + X*(B3 + ... + X*B12))
#		 d) To fully utilize the pipeline, Q is separated into
#		 two independent pieces of roughly equal complexity
#			Q = [ X*S*(B2 + S*(B4 + ... + S*B12)) ] +
#				[ S*S*(B3 + S*(B5 + ... + S*B11)) ]
#
#	Step 10.	Calculate exp(X)-1 for |X| >= 70 log 2.
#		10.1 If X >= 70log2 , exp(X) - 1 = exp(X) for all practical
#		 purposes. Therefore, go to Step 1 of setox.
#		10.2 If X <= -70log2, exp(X) - 1 = -1 for all practical purposes.
#		 ans := -1
#		 Restore user FPCR
#		 Return ans := ans + 2^(-126). Exit.
#	Notes:	10.2 will always create an inexact and return -1 + setoxs_tiny
#		 in the user rounding precision and mode.
#
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
	include(fpsp.h.s)
                                    
setoxs_l2:      long      0x3fdc0000,0x82e30865,0x4361c4c6,0x00000000 
                                    
setoxs_expa3:   long      0x3fa55555,0x55554431 
setoxs_expa2:   long      0x3fc55555,0x55554018 
                                    
setoxs_huge:    long      0x7ffe0000,0xffffffff,0xffffffff,0x00000000 
setoxs_tiny:    long      0x00010000,0xffffffff,0xffffffff,0x00000000 
                                    
setoxs_em1a4:   long      0x3f811111,0x11174385 
setoxs_em1a3:   long      0x3fa55555,0x55554f5a 
                                    
setoxs_em1a2:   long      0x3fc55555,0x55555555,0x00000000,0x00000000 
                                    
setoxs_em1b8:   long      0x3ec71de3,0xa5774682 
setoxs_em1b7:   long      0x3efa01a0,0x19d7cb68 
                                    
setoxs_em1b6:   long      0x3f2a01a0,0x1a019df3 
setoxs_em1b5:   long      0x3f56c16c,0x16c170e2 
                                    
setoxs_em1b4:   long      0x3f811111,0x11111111 
setoxs_em1b3:   long      0x3fa55555,0x55555555 
                                    
setoxs_em1b2:   long      0x3ffc0000,0xaaaaaaaa,0xaaaaaaab 
         long      0x00000000       
                                    
setoxs_two140:  long      0x48b00000,0x00000000 
setoxs_twon140: long      0x37300000,0x00000000 
                                    
setoxs_exptbl:                             
         long      0x3fff0000,0x80000000,0x00000000,0x00000000 
         long      0x3fff0000,0x8164d1f3,0xbc030774,0x9f841a9b 
         long      0x3fff0000,0x82cd8698,0xac2ba1d8,0x9fc1d5b9 
         long      0x3fff0000,0x843a28c3,0xacde4048,0xa0728369 
         long      0x3fff0000,0x85aac367,0xcc487b14,0x1fc5c95c 
         long      0x3fff0000,0x871f6196,0x9e8d1010,0x1ee85c9f 
         long      0x3fff0000,0x88980e80,0x92da8528,0x9fa20729 
         long      0x3fff0000,0x8a14d575,0x496efd9c,0xa07bf9af 
         long      0x3fff0000,0x8b95c1e3,0xea8bd6e8,0xa0020dcf 
         long      0x3fff0000,0x8d1adf5b,0x7e5ba9e4,0x205a63da 
         long      0x3fff0000,0x8ea4398b,0x45cd53c0,0x1eb70051 
         long      0x3fff0000,0x9031dc43,0x1466b1dc,0x1f6eb029 
         long      0x3fff0000,0x91c3d373,0xab11c338,0xa0781494 
         long      0x3fff0000,0x935a2b2f,0x13e6e92c,0x9eb319b0 
         long      0x3fff0000,0x94f4efa8,0xfef70960,0x2017457d 
         long      0x3fff0000,0x96942d37,0x20185a00,0x1f11d537 
         long      0x3fff0000,0x9837f051,0x8db8a970,0x9fb952dd 
         long      0x3fff0000,0x99e04593,0x20b7fa64,0x1fe43087 
         long      0x3fff0000,0x9b8d39b9,0xd54e5538,0x1fa2a818 
         long      0x3fff0000,0x9d3ed9a7,0x2cffb750,0x1fde494d 
         long      0x3fff0000,0x9ef53260,0x91a111ac,0x20504890 
         long      0x3fff0000,0xa0b0510f,0xb9714fc4,0xa073691c 
         long      0x3fff0000,0xa2704303,0x0c496818,0x1f9b7a05 
         long      0x3fff0000,0xa43515ae,0x09e680a0,0xa0797126 
         long      0x3fff0000,0xa5fed6a9,0xb15138ec,0xa071a140 
         long      0x3fff0000,0xa7cd93b4,0xe9653568,0x204f62da 
         long      0x3fff0000,0xa9a15ab4,0xea7c0ef8,0x1f283c4a 
         long      0x3fff0000,0xab7a39b5,0xa93ed338,0x9f9a7fdc 
         long      0x3fff0000,0xad583eea,0x42a14ac8,0xa05b3fac 
         long      0x3fff0000,0xaf3b78ad,0x690a4374,0x1fdf2610 
         long      0x3fff0000,0xb123f581,0xd2ac2590,0x9f705f90 
         long      0x3fff0000,0xb311c412,0xa9112488,0x201f678a 
         long      0x3fff0000,0xb504f333,0xf9de6484,0x1f32fb13 
         long      0x3fff0000,0xb6fd91e3,0x28d17790,0x20038b30 
         long      0x3fff0000,0xb8fbaf47,0x62fb9ee8,0x200dc3cc 
         long      0x3fff0000,0xbaff5ab2,0x133e45fc,0x9f8b2ae6 
         long      0x3fff0000,0xbd08a39f,0x580c36c0,0xa02bbf70 
         long      0x3fff0000,0xbf1799b6,0x7a731084,0xa00bf518 
         long      0x3fff0000,0xc12c4cca,0x66709458,0xa041dd41 
         long      0x3fff0000,0xc346ccda,0x24976408,0x9fdf137b 
         long      0x3fff0000,0xc5672a11,0x5506dadc,0x201f1568 
         long      0x3fff0000,0xc78d74c8,0xabb9b15c,0x1fc13a2e 
         long      0x3fff0000,0xc9b9bd86,0x6e2f27a4,0xa03f8f03 
         long      0x3fff0000,0xcbec14fe,0xf2727c5c,0x1ff4907d 
         long      0x3fff0000,0xce248c15,0x1f8480e4,0x9e6e53e4 
         long      0x3fff0000,0xd06333da,0xef2b2594,0x1fd6d45c 
         long      0x3fff0000,0xd2a81d91,0xf12ae45c,0xa076edb9 
         long      0x3fff0000,0xd4f35aab,0xcfedfa20,0x9fa6de21 
         long      0x3fff0000,0xd744fcca,0xd69d6af4,0x1ee69a2f 
         long      0x3fff0000,0xd99d15c2,0x78afd7b4,0x207f439f 
         long      0x3fff0000,0xdbfbb797,0xdaf23754,0x201ec207 
         long      0x3fff0000,0xde60f482,0x5e0e9124,0x9e8be175 
         long      0x3fff0000,0xe0ccdeec,0x2a94e110,0x20032c4b 
         long      0x3fff0000,0xe33f8972,0xbe8a5a50,0x2004dff5 
         long      0x3fff0000,0xe5b906e7,0x7c8348a8,0x1e72f47a 
         long      0x3fff0000,0xe8396a50,0x3c4bdc68,0x1f722f22 
         long      0x3fff0000,0xeac0c6e7,0xdd243930,0xa017e945 
         long      0x3fff0000,0xed4f301e,0xd9942b84,0x1f401a5b 
         long      0x3fff0000,0xefe4b99b,0xdcdaf5cc,0x9fb9a9e3 
         long      0x3fff0000,0xf281773c,0x59ffb138,0x20744c05 
         long      0x3fff0000,0xf5257d15,0x2486cc2c,0x1f773a19 
         long      0x3fff0000,0xf7d0df73,0x0ad13bb8,0x1ffe90d5 
         long      0x3fff0000,0xfa83b2db,0x722a033c,0xa041ed22 
         long      0x3fff0000,0xfd3e0c0c,0xf486c174,0x1f853f3a 
                                    
         set       setoxs_adjflag,l_scr2   
         set       setoxs_scale,fp_scr1    
         set       setoxs_adjscale,fp_scr2 
         set       setoxs_sc,fp_scr3       
         set       setoxs_onebysc,fp_scr4  
                                    
                                    
                                    
                                    
                                    
                                    
setoxd:                             
#--entry point for EXP(X), X is denormalized
         move.l    (%a0),%d0        
         andi.l    &0x80000000,%d0  
         ori.l     &0x00800000,%d0  # sign(X)*2^(-126)
         move.l    %d0,-(%sp)       
         fmove.s   &0f1.0,%fp0      
         fmove.l   %d1,%fpcr        
         fadd.s    (%sp)+,%fp0      
         bra.l     t_frcinx         
                                    
setox:                              
#--entry point for EXP(X), here X is finite, non-zero, and not NaN's
                                    
#--Step 1.
         move.l    (%a0),%d0        # load part of input X
         andi.l    &0x7fff0000,%d0  # biased expo. of X
         cmpi.l    %d0,&0x3fbe0000  # 2^(-65)
         bge.b     setoxs_expc1            # setoxs_normal case
         bra.w     setoxs_expsm            
                                    
setoxs_expc1:                              
#--The case |X| >= 2^(-65)
         move.w    4(%a0),%d0       # expo. and partial sig. of |X|
         cmpi.l    %d0,&0x400cb167  # 16380 log2 trunc. 16 bits
         blt.b     setoxs_expmain          # setoxs_normal case
         bra.w     setoxs_expbig           
                                    
setoxs_expmain:                            
#--Step 2.
#--This is the setoxs_normal branch:	2^(-65) <= |X| < 16380 log2.
         fmove.x   (%a0),%fp0       # load input from (a0)
                                    
         fmove.x   %fp0,%fp1        
         fmul.s    &0x42b8aa3b,%fp0 # 64/log2 * X
         fmovem.x  %fp2/%fp3,-(%a7) # save fp2
         move.l    &0,setoxs_adjflag(%a6)  
         fmove.l   %fp0,%d0         # N = int( X * 64/log2 )
ifdef(`PIC',`	lea	setoxs_exptbl(%pc),%a1
',`         lea       setoxs_exptbl,%a1       
')
         fmove.l   %d0,%fp0         # convert to floating-format
                                    
         move.l    %d0,l_scr1(%a6)  # save N temporarily
         andi.l    &0x3f,%d0        # D0 is J = N mod 64
         lsl.l     &4,%d0           
         adda.l    %d0,%a1          # address of 2^(J/64)
         move.l    l_scr1(%a6),%d0  
         asr.l     &6,%d0           # D0 is M
         addi.w    &0x3fff,%d0      # biased expo. of 2^(M)
ifdef(`PIC',`	move.w	&0x3fdc,l_scr1(%a6)
',`         move.w    setoxs_l2,l_scr1(%a6)   # prefetch L2, no need in CB
')
                                    
setoxs_expcont1:                            
#--Step 3.
#--fp1,fp2 saved on the stack. fp0 is N, fp1 is X,
#--a0 points to 2^(J/64), D0 is biased expo. of 2^(M)
         fmove.x   %fp0,%fp2        
         fmul.s    &0xbc317218,%fp0 # N * L1, L1 = lead(-log2/64)
ifdef(`PIC',`	fmul.x	&0x3fdc000082e308654361c4c6,%fp2
',`         fmul.x    setoxs_l2,%fp2          # N * L2, L1+L2 = -log2/64
')
         fadd.x    %fp1,%fp0        # X + N*L1
         fadd.x    %fp2,%fp0        # fp0 is R, reduced arg.
#	MOVE.W		#$3FA5,EXPA3	...load EXPA3 in cache
                                    
#--Step 4.
#--WE NOW COMPUTE EXP(R)-1 BY A POLYNOMIAL
#-- R + R*R*(A1 + R*(A2 + R*(A3 + R*(A4 + R*A5))))
#--TO FULLY UTILIZE THE PIPELINE, WE COMPUTE S = R*R
#--[R+R*S*(A2+S*A4)] + [S*(A1+S*(A3+S*A5))]
                                    
         fmove.x   %fp0,%fp1        
         fmul.x    %fp1,%fp1        # fp1 IS S = R*R
                                    
         fmove.s   &0x3ab60b70,%fp2 # fp2 IS A5
#	MOVE.W		#0,2(a1)	...load 2^(J/64) in cache
                                    
         fmul.x    %fp1,%fp2        # fp2 IS S*A5
         fmove.x   %fp1,%fp3        
         fmul.s    &0x3c088895,%fp3 # fp3 IS S*A4
                                    
ifdef(`PIC',`	fadd.d	&0x3fa5555555554431,%fp2
',`         fadd.d    setoxs_expa3,%fp2       # fp2 IS A3+S*A5
')
ifdef(`PIC',`	fadd.d	&0x3fc5555555554018,%fp3
',`         fadd.d    setoxs_expa2,%fp3       # fp3 IS A2+S*A4
')
                                    
         fmul.x    %fp1,%fp2        # fp2 IS S*(A3+S*A5)
         move.w    %d0,setoxs_scale(%a6)   # SCALE is 2^(M) in extended
         clr.w     setoxs_scale+2(%a6)     
         move.l    &0x80000000,setoxs_scale+4(%a6) 
         clr.l     setoxs_scale+8(%a6)     
                                    
         fmul.x    %fp1,%fp3        # fp3 IS S*(A2+S*A4)
                                    
         fadd.s    &0f0.5,%fp2      # fp2 IS A1+S*(A3+S*A5)
         fmul.x    %fp0,%fp3        # fp3 IS R*S*(A2+S*A4)
                                    
         fmul.x    %fp1,%fp2        # fp2 IS S*(A1+S*(A3+S*A5))
         fadd.x    %fp3,%fp0        # fp0 IS R+R*S*(A2+S*A4),
#					...fp3 released
                                    
         fmove.x   (%a1)+,%fp1      # fp1 is lead. pt. of 2^(J/64)
         fadd.x    %fp2,%fp0        # fp0 is EXP(R) - 1
#					...fp2 released
                                    
#--Step 5
#--final reconstruction process
#--EXP(X) = 2^M * ( 2^(J/64) + 2^(J/64)*(EXP(R)-1) )
                                    
         fmul.x    %fp1,%fp0        # 2^(J/64)*(Exp(R)-1)
         fmovem.x  (%a7)+,%fp2/%fp3 # fp2 restored
         fadd.s    (%a1),%fp0       # accurate 2^(J/64)
                                    
         fadd.x    %fp1,%fp0        
         move.l    setoxs_adjflag(%a6),%d0 
                                    
#--Step 6
         tst.l     %d0              
         beq.b     setoxs_normal           
setoxs_adjust:                             
         fmul.x    setoxs_adjscale(%a6),%fp0 
setoxs_normal:                             
         fmove.l   %d1,%fpcr        # restore user FPCR
         fmul.x    setoxs_scale(%a6),%fp0  # multiply 2^(M)
         bra.l     t_frcinx         
                                    
setoxs_expsm:                              
#--Step 7
         fmovem.x  (%a0),%fp0       # in case X is denormalized
         fmove.l   %d1,%fpcr        
         fadd.s    &0f1.0,%fp0      # 1+X in user mode
         bra.l     t_frcinx         
                                    
setoxs_expbig:                             
#--Step 8
         cmpi.l    %d0,&0x400cb27c  # 16480 log2
         bgt.b     setoxs_exp2big          
#--Steps 8.2 -- 8.6
         fmove.x   (%a0),%fp0       # load input from (a0)
                                    
         fmove.x   %fp0,%fp1        
         fmul.s    &0x42b8aa3b,%fp0 # 64/log2 * X
         fmovem.x  %fp2/%fp3,-(%a7) # save fp2
         move.l    &1,setoxs_adjflag(%a6)  
         fmove.l   %fp0,%d0         # N = int( X * 64/log2 )
ifdef(`PIC',`	lea	setoxs_exptbl(%pc),%a1
',`         lea       setoxs_exptbl,%a1       
')
         fmove.l   %d0,%fp0         # convert to floating-format
         move.l    %d0,l_scr1(%a6)  # save N temporarily
         andi.l    &0x3f,%d0        # D0 is J = N mod 64
         lsl.l     &4,%d0           
         adda.l    %d0,%a1          # address of 2^(J/64)
         move.l    l_scr1(%a6),%d0  
         asr.l     &6,%d0           # D0 is K
         move.l    %d0,l_scr1(%a6)  # save K temporarily
         asr.l     &1,%d0           # D0 is M1
         sub.l     %d0,l_scr1(%a6)  # a1 is M
         addi.w    &0x3fff,%d0      # biased expo. of 2^(M1)
         move.w    %d0,setoxs_adjscale(%a6) # ADJSCALE := 2^(M1)
         clr.w     setoxs_adjscale+2(%a6)  
         move.l    &0x80000000,setoxs_adjscale+4(%a6) 
         clr.l     setoxs_adjscale+8(%a6)  
         move.l    l_scr1(%a6),%d0  # D0 is M
         addi.w    &0x3fff,%d0      # biased expo. of 2^(M)
         bra.w     setoxs_expcont1         # go back to Step 3
                                    
setoxs_exp2big:                            
#--Step 9
         fmove.l   %d1,%fpcr        
         move.l    (%a0),%d0        
         bclr.b    &sign_bit,(%a0)  # setox always returns positive
         cmpi.l    %d0,&0           
         blt.l     t_unfl           
         bra.l     t_ovfl           
                                    
setoxm1d:                            
#--entry point for EXPM1(X), here X is denormalized
#--Step 0.
         bra.l     t_extdnrm        
                                    
                                    
setoxm1:                            
#--entry point for EXPM1(X), here X is finite, non-zero, non-NaN
                                    
#--Step 1.
#--Step 1.1
         move.l    (%a0),%d0        # load part of input X
         andi.l    &0x7fff0000,%d0  # biased expo. of X
         cmpi.l    %d0,&0x3ffd0000  # 1/4
         bge.b     setoxs_em1con1          # |X| >= 1/4
         bra.w     setoxs_em1sm            
                                    
setoxs_em1con1:                            
#--Step 1.3
#--The case |X| >= 1/4
         move.w    4(%a0),%d0       # expo. and partial sig. of |X|
         cmpi.l    %d0,&0x4004c215  # 70log2 rounded up to 16 bits
         ble.b     setoxs_em1main          # 1/4 <= |X| <= 70log2
         bra.w     setoxs_em1big           
                                    
setoxs_em1main:                            
#--Step 2.
#--This is the case:	1/4 <= |X| <= 70 log2.
         fmove.x   (%a0),%fp0       # load input from (a0)
                                    
         fmove.x   %fp0,%fp1        
         fmul.s    &0x42b8aa3b,%fp0 # 64/log2 * X
         fmovem.x  %fp2/%fp3,-(%a7) # save fp2
#	MOVE.W		#$3F81,EM1A4		...prefetch in CB mode
         fmove.l   %fp0,%d0         # N = int( X * 64/log2 )
ifdef(`PIC',`	lea	setoxs_exptbl(%pc),%a1
',`         lea       setoxs_exptbl,%a1       
')
         fmove.l   %d0,%fp0         # convert to floating-format
                                    
         move.l    %d0,l_scr1(%a6)  # save N temporarily
         andi.l    &0x3f,%d0        # D0 is J = N mod 64
         lsl.l     &4,%d0           
         adda.l    %d0,%a1          # address of 2^(J/64)
         move.l    l_scr1(%a6),%d0  
         asr.l     &6,%d0           # D0 is M
         move.l    %d0,l_scr1(%a6)  # save a copy of M
#	MOVE.W		#$3FDC,L2		...prefetch L2 in CB mode
                                    
#--Step 3.
#--fp1,fp2 saved on the stack. fp0 is N, fp1 is X,
#--a0 points to 2^(J/64), D0 and a1 both contain M
         fmove.x   %fp0,%fp2        
         fmul.s    &0xbc317218,%fp0 # N * L1, L1 = lead(-log2/64)
ifdef(`PIC',`	fmul.x	&0x3fdc000082e308654361c4c6,%fp2
',`         fmul.x    setoxs_l2,%fp2          # N * L2, L1+L2 = -log2/64
')
         fadd.x    %fp1,%fp0        # X + N*L1
         fadd.x    %fp2,%fp0        # fp0 is R, reduced arg.
#	MOVE.W		#$3FC5,EM1A2		...load EM1A2 in cache
         addi.w    &0x3fff,%d0      # D0 is biased expo. of 2^M
                                    
#--Step 4.
#--WE NOW COMPUTE EXP(R)-1 BY A POLYNOMIAL
#-- R + R*R*(A1 + R*(A2 + R*(A3 + R*(A4 + R*(A5 + R*A6)))))
#--TO FULLY UTILIZE THE PIPELINE, WE COMPUTE S = R*R
#--[R*S*(A2+S*(A4+S*A6))] + [R+S*(A1+S*(A3+S*A5))]
                                    
         fmove.x   %fp0,%fp1        
         fmul.x    %fp1,%fp1        # fp1 IS S = R*R
                                    
         fmove.s   &0x3950097b,%fp2 # fp2 IS a6
#	MOVE.W		#0,2(a1)	...load 2^(J/64) in cache
                                    
         fmul.x    %fp1,%fp2        # fp2 IS S*A6
         fmove.x   %fp1,%fp3        
         fmul.s    &0x3ab60b6a,%fp3 # fp3 IS S*A5
                                    
ifdef(`PIC',`	fadd.d	&0x3f81111111174385,%fp2
',`         fadd.d    setoxs_em1a4,%fp2       # fp2 IS A4+S*A6
')
ifdef(`PIC',`	fadd.d	&0x3fa5555555554f5a,%fp3
',`         fadd.d    setoxs_em1a3,%fp3       # fp3 IS A3+S*A5
')
         move.w    %d0,setoxs_sc(%a6)      # SC is 2^(M) in extended
         clr.w     setoxs_sc+2(%a6)        
         move.l    &0x80000000,setoxs_sc+4(%a6) 
         clr.l     setoxs_sc+8(%a6)        
                                    
         fmul.x    %fp1,%fp2        # fp2 IS S*(A4+S*A6)
         move.l    l_scr1(%a6),%d0  # D0 is	M
         neg.w     %d0              # D0 is -M
         fmul.x    %fp1,%fp3        # fp3 IS S*(A3+S*A5)
         addi.w    &0x3fff,%d0      # biased expo. of 2^(-M)
ifdef(`PIC',`	fadd.d	&0x3fc5555555555555,%fp2
',`         fadd.d    setoxs_em1a2,%fp2       # fp2 IS A2+S*(A4+S*A6)
')
         fadd.s    &0f0.5,%fp3      # fp3 IS A1+S*(A3+S*A5)
                                    
         fmul.x    %fp1,%fp2        # fp2 IS S*(A2+S*(A4+S*A6))
         ori.w     &0x8000,%d0      # signed/expo. of -2^(-M)
         move.w    %d0,setoxs_onebysc(%a6) # OnebySc is -2^(-M)
         clr.w     setoxs_onebysc+2(%a6)   
         move.l    &0x80000000,setoxs_onebysc+4(%a6) 
         clr.l     setoxs_onebysc+8(%a6)   
         fmul.x    %fp3,%fp1        # fp1 IS S*(A1+S*(A3+S*A5))
#					...fp3 released
                                    
         fmul.x    %fp0,%fp2        # fp2 IS R*S*(A2+S*(A4+S*A6))
         fadd.x    %fp1,%fp0        # fp0 IS R+S*(A1+S*(A3+S*A5))
#					...fp1 released
                                    
         fadd.x    %fp2,%fp0        # fp0 IS EXP(R)-1
#					...fp2 released
         fmovem.x  (%a7)+,%fp2/%fp3 # fp2 restored
                                    
#--Step 5
#--Compute 2^(J/64)*p
                                    
         fmul.x    (%a1),%fp0       # 2^(J/64)*(Exp(R)-1)
                                    
#--Step 6
#--Step 6.1
         move.l    l_scr1(%a6),%d0  # retrieve M
         cmpi.l    %d0,&63          
         ble.b     setoxs_mle63            
#--Step 6.2	M >= 64
         fmove.s   12(%a1),%fp1     # fp1 is t
         fadd.x    setoxs_onebysc(%a6),%fp1 # fp1 is t+OnebySc
         fadd.x    %fp1,%fp0        # p+(t+OnebySc), fp1 released
         fadd.x    (%a1),%fp0       # T+(p+(t+OnebySc))
         bra.b     setoxs_em1scale         
setoxs_mle63:                              
#--Step 6.3	M <= 63
         cmpi.l    %d0,&-3          
         bge.b     setoxs_mgen3            
setoxs_mltn3:                              
#--Step 6.4	M <= -4
         fadd.s    12(%a1),%fp0     # p+t
         fadd.x    (%a1),%fp0       # T+(p+t)
         fadd.x    setoxs_onebysc(%a6),%fp0 # OnebySc + (T+(p+t))
         bra.b     setoxs_em1scale         
setoxs_mgen3:                              
#--Step 6.5	-3 <= M <= 63
         fmove.x   (%a1)+,%fp1      # fp1 is T
         fadd.s    (%a1),%fp0       # fp0 is p+t
         fadd.x    setoxs_onebysc(%a6),%fp1 # fp1 is T+OnebySc
         fadd.x    %fp1,%fp0        # (T+OnebySc)+(p+t)
                                    
setoxs_em1scale:                            
#--Step 6.6
         fmove.l   %d1,%fpcr        
         fmul.x    setoxs_sc(%a6),%fp0     
                                    
         bra.l     t_frcinx         
                                    
setoxs_em1sm:                              
#--Step 7	|X| < 1/4.
         cmpi.l    %d0,&0x3fbe0000  # 2^(-65)
         bge.b     setoxs_em1poly          
                                    
setoxs_em1tiny:                            
#--Step 8	|X| < 2^(-65)
         cmpi.l    %d0,&0x00330000  # 2^(-16312)
         blt.b     setoxs_em12tiny         
#--Step 8.2
         move.l    &0x80010000,setoxs_sc(%a6) # SC is -2^(-16382)
         move.l    &0x80000000,setoxs_sc+4(%a6) 
         clr.l     setoxs_sc+8(%a6)        
         fmove.x   (%a0),%fp0       
         fmove.l   %d1,%fpcr        
         fadd.x    setoxs_sc(%a6),%fp0     
                                    
         bra.l     t_frcinx         
                                    
setoxs_em12tiny:                            
#--Step 8.3
         fmove.x   (%a0),%fp0       
ifdef(`PIC',`	fmul.d	&0x48b0000000000000,%fp0
',`         fmul.d    setoxs_two140,%fp0      
')
         move.l    &0x80010000,setoxs_sc(%a6) 
         move.l    &0x80000000,setoxs_sc+4(%a6) 
         clr.l     setoxs_sc+8(%a6)        
         fadd.x    setoxs_sc(%a6),%fp0     
         fmove.l   %d1,%fpcr        
ifdef(`PIC',`	fmul.d	&0x3730000000000000,%fp0
',`         fmul.d    setoxs_twon140,%fp0     
')
                                    
         bra.l     t_frcinx         
                                    
setoxs_em1poly:                            
#--Step 9	exp(X)-1 by a simple polynomial
         fmove.x   (%a0),%fp0       # fp0 is X
         fmul.x    %fp0,%fp0        # fp0 is S := X*X
         fmovem.x  %fp2/%fp3,-(%a7) # save fp2
         fmove.s   &0x2f30caa8,%fp1 # fp1 is B12
         fmul.x    %fp0,%fp1        # fp1 is S*B12
         fmove.s   &0x310f8290,%fp2 # fp2 is B11
         fadd.s    &0x32d73220,%fp1 # fp1 is B10+S*B12
                                    
         fmul.x    %fp0,%fp2        # fp2 is S*B11
         fmul.x    %fp0,%fp1        
                                    
         fadd.s    &0x3493f281,%fp2 
ifdef(`PIC',`	fadd.d	&0x3ec71de3a5774682,%fp1
',`         fadd.d    setoxs_em1b8,%fp1       
')
                                    
         fmul.x    %fp0,%fp2        
         fmul.x    %fp0,%fp1        
                                    
ifdef(`PIC',`	fadd.d	&0x3efa01a019d7cb68,%fp2
',`         fadd.d    setoxs_em1b7,%fp2       
')
ifdef(`PIC',`	fadd.d	&0x3f2a01a01a019df3,%fp1
',`         fadd.d    setoxs_em1b6,%fp1       
')
                                    
         fmul.x    %fp0,%fp2        
         fmul.x    %fp0,%fp1        
                                    
ifdef(`PIC',`	fadd.d	&0x3f56c16c16c170e2,%fp2
',`         fadd.d    setoxs_em1b5,%fp2       
')
ifdef(`PIC',`	fadd.d	&0x3f81111111111111,%fp1
',`         fadd.d    setoxs_em1b4,%fp1       
')
                                    
         fmul.x    %fp0,%fp2        
         fmul.x    %fp0,%fp1        
                                    
ifdef(`PIC',`	fadd.d	&0x3fa5555555555555,%fp2
',`         fadd.d    setoxs_em1b3,%fp2       
')
ifdef(`PIC',`	fadd.x	&0x3ffc0000aaaaaaaaaaaaaaab,%fp1
',`         fadd.x    setoxs_em1b2,%fp1       
')
                                    
         fmul.x    %fp0,%fp2        
         fmul.x    %fp0,%fp1        
                                    
         fmul.x    %fp0,%fp2        # )
         fmul.x    (%a0),%fp1       
                                    
         fmul.s    &0f0.5,%fp0      # fp0 is S*B1
         fadd.x    %fp2,%fp1        # fp1 is Q
#					...fp2 released
                                    
         fmovem.x  (%a7)+,%fp2/%fp3 # fp2 restored
                                    
         fadd.x    %fp1,%fp0        # fp0 is S*B1+Q
#					...fp1 released
                                    
         fmove.l   %d1,%fpcr        
         fadd.x    (%a0),%fp0       
                                    
         bra.l     t_frcinx         
                                    
setoxs_em1big:                             
#--Step 10	|X| > 70 log2
         move.l    (%a0),%d0        
         cmpi.l    %d0,&0           
         bgt.w     setoxs_expc1            
#--Step 10.2
         fmove.s   &0f-1.0,%fp0     # fp0 is -1
         fmove.l   %d1,%fpcr        
         fadd.s    &0f1.1754943e-38,%fp0 # -1 + 2^(-126)
                                    
         bra.l     t_frcinx         
                                    
                                    
	version 3
