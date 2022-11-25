ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /misc/source_product/9.10/commands.rcs/lib/libm/fp040_mod.s,v $
# $Revision: 70.2 $      $Author: ssa $
# $State: Exp $         $Locker:  $
# $Date: 91/08/30 00:11:24 $
                                    
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
define(FILE_mod)


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
# $Source: /misc/source_product/9.10/commands.rcs/lib/libm/fp040_mod.s,v $
# $Revision: 70.2 $      $Author: ssa $
# $State: Exp $         $Locker:  $
# $Date: 91/08/30 00:11:24 $
                                    
#
#	srem_mod.sa 3.1 12/10/90
#
#      The entry point sMOD computes the floating point MOD of the
#      input values X and Y. The entry point sREM computes the floating
#      point (IEEE) REM of the input values X and Y.
#
#      INPUT
#      -----
#      Double-extended value Y is pointed to by address in register
#      A0. Double-extended value X is located in -12(A0). The values
#      of X and Y are both nonzero and finite; although either or both
#      of them can be denormalized. The special cases of zeros, NaNs,
#      and infinities are handled elsewhere.
#
#      OUTPUT
#      ------
#      FREM(X,Y) or FMOD(X,Y), depending on entry point.
#
#       ALGORITHM
#       ---------
#
#       Step 1.  Save and strip signs of X and Y: signX := sign(X),
#                signY := sign(Y), X := |X|, Y := |Y|, 
#                signQ := signX EOR signY. Record whether MOD or REM
#                is requested.
#
#       Step 2.  Set L := expo(X)-expo(Y), k := 0, srem_mods_Q := 0.
#                If (L < 0) then
#                   R := X, go to Step 4.
#                else
#                   R := 2^(-L)X, j := L.
#                endif
#
#       Step 3.  Perform MOD(X,Y)
#            3.1 If R = Y, go to Step 9.
#            3.2 If R > Y, then { R := R - Y, srem_mods_Q := srem_mods_Q + 1}
#            3.3 If j = 0, go to Step 4.
#            3.4 k := k + 1, j := j - 1, srem_mods_Q := 2Q, R := 2R. Go to
#                Step 3.1.
#
#       Step 4.  At this point, R = X - QY = MOD(X,Y). Set
#                srem_mods_Last_Subtract := false (used in Step 7 below). If
#                MOD is requested, go to Step 6. 
#
#       Step 5.  R = MOD(X,Y), but REM(X,Y) is requested.
#            5.1 If R < Y/2, then R = MOD(X,Y) = REM(X,Y). Go to
#                Step 6.
#            5.2 If R > Y/2, then { set srem_mods_Last_Subtract := true,
#                srem_mods_Q := srem_mods_Q + 1, Y := signY*Y }. Go to Step 6.
#            5.3 This is the tricky case of R = Y/2. If srem_mods_Q is odd,
#                then { srem_mods_Q := srem_mods_Q + 1, signX := -signX }.
#
#       Step 6.  R := signX*R.
#
#       Step 7.  If srem_mods_Last_Subtract = true, R := R - Y.
#
#       Step 8.  Return signQ, last 7 bits of srem_mods_Q, and R as required.
#
#       Step 9.  At this point, R = 2^(-j)*X - srem_mods_Q Y = Y. Thus,
#                X = 2^(j)*(Q+1)Y. set srem_mods_Q := 2^(j)*(Q+1),
#                R := 0. Return signQ, last 7 bits of srem_mods_Q, and R.
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
                                    
         set       srem_mods_mod_flag,l_scr3  
         set       srem_mods_signy,fp_scr3+4  
         set       srem_mods_signx,fp_scr3+8  
         set       srem_mods_signq,fp_scr3+12 
         set       srem_mods_sc_flag,fp_scr4  
                                    
         set       srem_mods_y,fp_scr1        
         set       srem_mods_y_hi,srem_mods_y+4         
         set       srem_mods_y_lo,srem_mods_y+8         
                                    
         set       srem_mods_r,fp_scr2        
         set       srem_mods_r_hi,srem_mods_r+4         
         set       srem_mods_r_lo,srem_mods_r+8         
                                    
                                    
srem_mods_scale:   long      0x00010000,0x80000000,0x00000000,0x00000000 
                                    
                                    
                                    
smod:                               
                                    
         move.l    &0,srem_mods_mod_flag(%a6) 
         bra.b     srem_mods_mod_rem          
                                    
srem:                               
                                    
         move.l    &1,srem_mods_mod_flag(%a6) 
                                    
srem_mods_mod_rem:                            
#..Save sign of X and Y
         movem.l   %d2-%d7,-(%a7)   # save data registers
         move.w    (%a0),%d3        
         move.w    %d3,srem_mods_signy(%a6)   
         andi.l    &0x00007fff,%d3  # Y := |Y|
                                    
#
         move.l    4(%a0),%d4       
         move.l    8(%a0),%d5       # (D3,D4,D5) is |Y|
                                    
         tst.l     %d3              
         bne.b     srem_mods_y_normal         
                                    
         move.l    &0x00003ffe,%d3  # $3FFD + 1
         tst.l     %d4              
         bne.b     srem_mods_hiy_not0         
                                    
srem_mods_hiy_0:                              
         move.l    %d5,%d4          
         clr.l     %d5              
         subi.l    &32,%d3          
         clr.l     %d6              
         bfffo     %d4{&0:&32},%d6  
         lsl.l     %d6,%d4          
         sub.l     %d6,%d3          # (D3,D4,D5) is normalized
#                                       ...with bias $7FFD
         bra.b     srem_mods_chk_x            
                                    
srem_mods_hiy_not0:                            
         clr.l     %d6              
         bfffo     %d4{&0:&32},%d6  
         sub.l     %d6,%d3          
         lsl.l     %d6,%d4          
         move.l    %d5,%d7          # a copy of D5
         lsl.l     %d6,%d5          
         neg.l     %d6              
         addi.l    &32,%d6          
         lsr.l     %d6,%d7          
         or.l      %d7,%d4          # (D3,D4,D5) normalized
#                                       ...with bias $7FFD
         bra.b     srem_mods_chk_x            
                                    
srem_mods_y_normal:                            
         addi.l    &0x00003ffe,%d3  # (D3,D4,D5) normalized
#                                       ...with bias $7FFD
                                    
srem_mods_chk_x:                              
         move.w    -12(%a0),%d0     
         move.w    %d0,srem_mods_signx(%a6)   
         move.w    srem_mods_signy(%a6),%d1   
         eor.l     %d0,%d1          
         andi.l    &0x00008000,%d1  
         move.w    %d1,srem_mods_signq(%a6)   # sign(Q) obtained
         andi.l    &0x00007fff,%d0  
         move.l    -8(%a0),%d1      
         move.l    -4(%a0),%d2      # (D0,D1,D2) is |X|
         tst.l     %d0              
         bne.b     srem_mods_x_normal         
         move.l    &0x00003ffe,%d0  
         tst.l     %d1              
         bne.b     srem_mods_hix_not0         
                                    
srem_mods_hix_0:                              
         move.l    %d2,%d1          
         clr.l     %d2              
         subi.l    &32,%d0          
         clr.l     %d6              
         bfffo     %d1{&0:&32},%d6  
         lsl.l     %d6,%d1          
         sub.l     %d6,%d0          # (D0,D1,D2) is normalized
#                                       ...with bias $7FFD
         bra.b     srem_mods_init             
                                    
srem_mods_hix_not0:                            
         clr.l     %d6              
         bfffo     %d1{&0:&32},%d6  
         sub.l     %d6,%d0          
         lsl.l     %d6,%d1          
         move.l    %d2,%d7          # a copy of D2
         lsl.l     %d6,%d2          
         neg.l     %d6              
         addi.l    &32,%d6          
         lsr.l     %d6,%d7          
         or.l      %d7,%d1          # (D0,D1,D2) normalized
#                                       ...with bias $7FFD
         bra.b     srem_mods_init             
                                    
srem_mods_x_normal:                            
         addi.l    &0x00003ffe,%d0  # (D0,D1,D2) normalized
#                                       ...with bias $7FFD
                                    
srem_mods_init:                               
#
         move.l    %d3,l_scr1(%a6)  # save biased expo(Y)
         move.l    %d0,l_scr2(%a6)  # save d0
         sub.l     %d3,%d0          # L := expo(X)-expo(Y)
#   Move.L               D0,L            ...D0 is j
         clr.l     %d6              # D6 := carry <- 0
         clr.l     %d3              # D3 is srem_mods_Q
         movea.l   &0,%a1           # A1 is k; j+k=L, srem_mods_Q=0
                                    
#..(Carry,D1,D2) is R
         tst.l     %d0              
         bge.b     srem_mods_mod_loop         
                                    
#..expo(X) < expo(Y). Thus X = mod(X,Y)
#
         move.l    l_scr2(%a6),%d0  # srem_mods_restore d0
         bra.w     srem_mods_get_mod          
                                    
#..At this point  R = 2^(-L)X; srem_mods_Q = 0; k = 0; and  k+j = L
                                    
                                    
srem_mods_mod_loop:                            
         tst.l     %d6              # test carry bit
         bgt.b     srem_mods_r_gt_y           
                                    
#..At this point carry = 0, R = (D1,D2), Y = (D4,D5)
         cmp.l     %d1,%d4          # compare hi(R) and hi(Y)
         bne.b     srem_mods_r_ne_y           
         cmp.l     %d2,%d5          # compare lo(R) and lo(Y)
         bne.b     srem_mods_r_ne_y           
                                    
#..At this point, R = Y
         bra.w     srem_mods_rem_is_0         
                                    
srem_mods_r_ne_y:                             
#..use the borrow of the previous compare
         bcs.b     srem_mods_r_lt_y           # borrow is set srem_mods_iff R < Y
                                    
srem_mods_r_gt_y:                             
#..If srem_mods_Carry is set, then Y < (Carry,D1,D2) < 2Y. Otherwise, srem_mods_Carry = 0
#..and Y < (D1,D2) < 2Y. Either way, perform R - Y
         sub.l     %d5,%d2          # lo(R) - lo(Y)
         subx.l    %d4,%d1          # hi(R) - hi(Y)
         clr.l     %d6              # clear carry
         addq.l    &1,%d3           # srem_mods_Q := srem_mods_Q + 1
                                    
srem_mods_r_lt_y:                             
#..At this point, srem_mods_Carry=0, R < Y. R = 2^(k-L)X - QY; k+j = L; j >= 0.
         tst.l     %d0              # see if j = 0.
         beq.b     srem_mods_postloop         
                                    
         add.l     %d3,%d3          # srem_mods_Q := 2Q
         add.l     %d2,%d2          # lo(R) = 2lo(R)
         roxl.l    &1,%d1           # hi(R) = 2hi(R) + carry
         scs       %d6              # set srem_mods_Carry if 2(R) overflows
         addq.l    &1,%a1           # k := k+1
         subq.l    &1,%d0           # j := j - 1
#..At this point, R=(Carry,D1,D2) = 2^(k-L)X - QY, j+k=L, j >= 0, R < 2Y.
                                    
         bra.b     srem_mods_mod_loop         
                                    
srem_mods_postloop:                            
#..k = L, j = 0, srem_mods_Carry = 0, R = (D1,D2) = X - QY, R < Y.
                                    
#..normalize R.
         move.l    l_scr1(%a6),%d0  # new biased expo of R
         tst.l     %d1              
         bne.b     srem_mods_hir_not0         
                                    
srem_mods_hir_0:                              
         move.l    %d2,%d1          
         clr.l     %d2              
         subi.l    &32,%d0          
         clr.l     %d6              
         bfffo     %d1{&0:&32},%d6  
         lsl.l     %d6,%d1          
         sub.l     %d6,%d0          # (D0,D1,D2) is normalized
#                                       ...with bias $7FFD
         bra.b     srem_mods_get_mod          
                                    
srem_mods_hir_not0:                            
         clr.l     %d6              
         bfffo     %d1{&0:&32},%d6  
         bmi.b     srem_mods_get_mod          # already normalized
         sub.l     %d6,%d0          
         lsl.l     %d6,%d1          
         move.l    %d2,%d7          # a copy of D2
         lsl.l     %d6,%d2          
         neg.l     %d6              
         addi.l    &32,%d6          
         lsr.l     %d6,%d7          
         or.l      %d7,%d1          # (D0,D1,D2) normalized
                                    
#
srem_mods_get_mod:                            
         cmpi.l    %d0,&0x000041fe  
         bge.b     srem_mods_no_scale         
srem_mods_do_scale:                            
         move.w    %d0,srem_mods_r(%a6)       
         clr.w     srem_mods_r+2(%a6)         
         move.l    %d1,srem_mods_r_hi(%a6)    
         move.l    %d2,srem_mods_r_lo(%a6)    
         move.l    l_scr1(%a6),%d6  
         move.w    %d6,srem_mods_y(%a6)       
         clr.w     srem_mods_y+2(%a6)         
         move.l    %d4,srem_mods_y_hi(%a6)    
         move.l    %d5,srem_mods_y_lo(%a6)    
         fmove.x   srem_mods_r(%a6),%fp0      # no exception
         move.l    &1,srem_mods_sc_flag(%a6)  
         bra.b     srem_mods_modorrem         
srem_mods_no_scale:                            
         move.l    %d1,srem_mods_r_hi(%a6)    
         move.l    %d2,srem_mods_r_lo(%a6)    
         subi.l    &0x3ffe,%d0      
         move.w    %d0,srem_mods_r(%a6)       
         clr.w     srem_mods_r+2(%a6)         
         move.l    l_scr1(%a6),%d6  
         subi.l    &0x3ffe,%d6      
         move.l    %d6,l_scr1(%a6)  
         fmove.x   srem_mods_r(%a6),%fp0      
         move.w    %d6,srem_mods_y(%a6)       
         move.l    %d4,srem_mods_y_hi(%a6)    
         move.l    %d5,srem_mods_y_lo(%a6)    
         move.l    &0,srem_mods_sc_flag(%a6)  
                                    
#
                                    
                                    
srem_mods_modorrem:                            
         move.l    srem_mods_mod_flag(%a6),%d6 
         beq.b     srem_mods_fix_sign         
                                    
         move.l    l_scr1(%a6),%d6  # new biased expo(Y)
         subq.l    &1,%d6           # biased expo(Y/2)
         cmp.l     %d0,%d6          
         blt.b     srem_mods_fix_sign         
         bgt.b     srem_mods_last_sub         
                                    
         cmp.l     %d1,%d4          
         bne.b     srem_mods_not_eq           
         cmp.l     %d2,%d5          
         bne.b     srem_mods_not_eq           
         bra.w     srem_mods_tie_case         
                                    
srem_mods_not_eq:                             
         bcs.b     srem_mods_fix_sign         
                                    
srem_mods_last_sub:                            
#
         fsub.x    srem_mods_y(%a6),%fp0      # no exceptions
         addq.l    &1,%d3           # srem_mods_Q := srem_mods_Q + 1
                                    
#
                                    
srem_mods_fix_sign:                            
#..Get sign of X
         move.w    srem_mods_signx(%a6),%d6   
         bge.b     srem_mods_get_q            
         fneg.x    %fp0             
                                    
#..Get srem_mods_Q
#
srem_mods_get_q:                              
         clr.l     %d6              
         move.w    srem_mods_signq(%a6),%d6   # D6 is sign(Q)
         move.l    &8,%d7           
         lsr.l     %d7,%d6          
         andi.l    &0x0000007f,%d3  # 7 bits of srem_mods_Q
         or.l      %d6,%d3          # sign and bits of srem_mods_Q
         swap      %d3              
         fmove.l   %fpsr,%d6        
         andi.l    &0xff00ffff,%d6  
         or.l      %d3,%d6          
         fmove.l   %d6,%fpsr        # put srem_mods_Q in fpsr
                                    
#
srem_mods_restore:                            
         movem.l   (%a7)+,%d2-%d7   
         fmove.l   user_fpcr(%a6),%fpcr 
         move.l    srem_mods_sc_flag(%a6),%d0 
         beq.b     srem_mods_finish           
         fmul.x    srem_mods_scale(%pc),%fp0  # may cause underflow
         bra.l     t_avoid_unsupp   # check for denorm as a
#					;result of the scaling
                                    
srem_mods_finish:                             
         fmove.x   %fp0,%fp0        # capture exceptions & round
         rts                        
                                    
srem_mods_rem_is_0:                            
#..R = 2^(-j)X - srem_mods_Q Y = Y, thus R = 0 and quotient = 2^j (Q+1)
         addq.l    &1,%d3           
         cmpi.l    %d0,&8           # D0 is j 
         bge.b     srem_mods_q_big            
                                    
         lsl.l     %d0,%d3          
         bra.b     srem_mods_set_r_0          
                                    
srem_mods_q_big:                              
         clr.l     %d3              
                                    
srem_mods_set_r_0:                            
         fmove.s   &0f0.0,%fp0      
         move.l    &0,srem_mods_sc_flag(%a6)  
         bra.w     srem_mods_fix_sign         
                                    
srem_mods_tie_case:                            
#..Check parity of srem_mods_Q
         move.l    %d3,%d6          
         andi.l    &0x00000001,%d6  
         tst.l     %d6              
         beq.w     srem_mods_fix_sign         # srem_mods_Q is even
                                    
#..Q is odd, srem_mods_Q := srem_mods_Q + 1, signX := -signX
         addq.l    &1,%d3           
         move.w    srem_mods_signx(%a6),%d6   
         eori.l    &0x00008000,%d6  
         move.w    %d6,srem_mods_signx(%a6)   
         bra.w     srem_mods_fix_sign         
                                    
                                    
	version 3
