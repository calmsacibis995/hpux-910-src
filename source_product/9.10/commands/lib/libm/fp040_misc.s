ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /misc/source_product/9.10/commands.rcs/lib/libm/fp040_misc.s,v $
# $Revision: 70.3 $      $Author: ssa $
# $State: Exp $         $Locker:  $
# $Date: 92/07/02 12:20:44 $
                                    
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
define(FILE_misc)


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
ifdef(`CLEAREPB',`
		bfclr		wbtemp_grs(%a6){&6:&3}
')
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
ifdef(`FILE_misc',`		define(CLEAREPB)
						WRAPPER( int, 	int, 	PM_ZERO,	PM_INF)
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
# $Source: /misc/source_product/9.10/commands.rcs/lib/libm/fp040_misc.s,v $
# $Revision: 70.3 $      $Author: ssa $
# $State: Exp $         $Locker:  $
# $Date: 92/07/02 12:20:44 $
                                    
#
#	sint.sa 3.1 12/10/90
#
#	The entry point sINT computes the rounded integer 
#	equivalent of the input argument, sINTRZ computes 
#	the integer rounded to zero of the input argument.
#
#	Entry points sint and sintrz are called from do_func
#	to emulate the fint and fintrz unimplemented instructions,
#	respectively.  Entry point sintdo is used by bindec.
#
#	Input: (Entry points sint and sintrz) Double-extended
#		number X sints_in the ETEMP space sints_in the floating-point
#		save stack.
#	       (Entry point sintdo) Double-extended number X sints_in
#		location pointed to by the address register a0.
#	       (Entry point sintd) Double-extended denormalized
#		number X sints_in the ETEMP space sints_in the floating-point
#		save stack.
#
#	Output: The function returns int(X) or intrz(X) sints_in fp0.
#
#	Modifies: fp0.
#
#	Algorithm: (sint and sintrz)
#
#	1. If exp(X) >= 63, return X. 
#	   If exp(X) < 0, return +/- 0 or +/- 1, according to
#	   the rounding mode.
#	
#	2. (X is sints_in range) set sints_rsc = 63 - exp(X). Unnormalize the
#	   result to the exponent $403e.
#
#	3. Round the result sints_in the mode given sints_in USER_FPCR. For
#	   sintrz, force round-to-zero mode.
#
#	4. Normalize the rounded result; store sints_in fp0.
#
#	For the denormalized cases, force the correct result
#	for the given sign and rounding mode.
#
#		        Sign(X)
#		RMODE   +    -
#		-----  --------
#		 RN    +0   -0
#		 RZ    +0   -0
#		 RM    +0   -1
#		 RP    +1   -0
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
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
#
#	FINT
#
sint:                               
         bfextu    fpcr_mode(%a6){&2:&2},%d1 # use user's mode for rounding
#					;implicity has extend precision
#					;in upper word. 
         move.l    %d1,l_scr1(%a6)  # save mode bits
         bra.b     sints_sintexc          
                                    
#
#	FINT with extended denorm inputs.
#
sintd:                              
         btst.b    &5,fpcr_mode(%a6) 
         beq.l     snzrinx          # if round nearest or round zero, +/- 0
         btst.b    &4,fpcr_mode(%a6) 
         beq.b     sints_rnd_mns          
sints_rnd_pls:                            
         btst.b    &sign_bit,local_ex(%a0) 
         bne.b     sints_sintmz           
         bsr.l     ld_pone          # if round plus inf and pos, answer is +1
         bra.l     t_inx2           
sints_rnd_mns:                            
         btst.b    &sign_bit,local_ex(%a0) 
         beq.b     sints_sintpz           
         bsr.l     ld_mone          # if round mns inf and neg, answer is -1
         bra.l     t_inx2           
sints_sintpz:                             
         bsr.l     ld_pzero         
         bra.l     t_inx2           
sints_sintmz:                             
         bsr.l     ld_mzero         
         bra.l     t_inx2           
                                    
#
#	FINTRZ
#
sintrz:                             
         move.l    &1,l_scr1(%a6)   # use rz mode for rounding
#					;implicity has extend precision
#					;in upper word. 
         bra.b     sints_sintexc          
#
#	SINTDO
#
#	Input:	a0 points to an IEEE extended format operand
# 	Output:	fp0 has the result 
#
# Exeptions:
#
# If the subroutine results sints_in an inexact operation, the inx2 and
# ainx bits sints_in the USER_FPSR are set.
#
#
sintdo:                             
         bfextu    fpcr_mode(%a6){&2:&2},%d1 # use user's mode for rounding
#					;implicitly has ext precision
#					;in upper word. 
         move.l    %d1,l_scr1(%a6)  # save mode bits
#
# Real work of sint is sints_in sints_sintexc
#
sints_sintexc:                            
         bclr.b    &sign_bit,local_ex(%a0) # convert to internal extended
#					;format
         sne       local_sgn(%a0)   
         cmp.w     local_ex(%a0),&0x403e # check if (unbiased) exp > 63
         bgt.b     sints_out_rnge         # branch if exp < 63
         cmp.w     local_ex(%a0),&0x3ffd # check if (unbiased) exp < 0
         bgt.w     sints_in_rnge          # if 63 >= exp > 0, do calc
#
# Input is less than zero.  Restore sign, and check for directed
# rounding modes.  L_SCR1 contains the rmode sints_in the lower byte.
#
sints_un_rnge:                            
         btst.b    &1,l_scr1+3(%a6) # check for rn and rz
         beq.b     sints_un_rnrz          
         tst.b     local_sgn(%a0)   # check for sign
         bne.b     sints_un_rmrp_neg      
#
# Sign is +.  If rp, load +1.0, if rm, load +0.0
#
         cmpi.b    l_scr1+3(%a6),&3 # check for rp
         beq.b     sints_un_ldpone        # if rp, load +1.0
         bsr.l     ld_pzero         # if rm, load +0.0
         bra.l     t_inx2           
sints_un_ldpone:                            
         bsr.l     ld_pone          
         bra.l     t_inx2           
#
# Sign is -.  If rm, load -1.0, if rp, load -0.0
#
sints_un_rmrp_neg:                            
         cmpi.b    l_scr1+3(%a6),&2 # check for rm
         beq.b     sints_un_ldmone        # if rm, load -1.0
         bsr.l     ld_mzero         # if rp, load -0.0
         bra.l     t_inx2           
sints_un_ldmone:                            
         bsr.l     ld_mone          
         bra.l     t_inx2           
#
# Rmode is rn or rz; return signed zero
#
sints_un_rnrz:                            
         tst.b     local_sgn(%a0)   # check for sign
         bne.b     sints_un_rnrz_neg      
         bsr.l     ld_pzero         
         bra.l     t_inx2           
sints_un_rnrz_neg:                            
         bsr.l     ld_mzero         
         bra.l     t_inx2           
                                    
#
# Input is greater than 2^63.  All bits are significant.  Return
# the input.
#
sints_out_rnge:                            
         bfclr     local_sgn(%a0){&0:&8} # change back to IEEE ext format
         beq.b     sints_intps            
         bset.b    &sign_bit,local_ex(%a0) 
sints_intps:                              
         fmove.l   %fpcr,-(%sp)     
         fmove.l   &0,%fpcr         
         fmove.x   local_ex(%a0),%fp0 # if exp > 63
#					;then return X to the user
#					;there are no fraction bits
         fmove.l   (%sp)+,%fpcr     
         rts                        
                                    
sints_in_rnge:                            
# 					;shift off fraction bits
         clr.l     %d0              # clear d0 - initial g,r,s for
#					;dnrm_lp
         move.l    &0x403e,%d1      # set sints_threshold for dnrm_lp
#					;assumes a0 points to operand
         bsr.l     dnrm_lp          
#					;returns unnormalized number
#					;pointed by a0
#					;output d0 supplies g,r,s
#					;used by round
         move.l    l_scr1(%a6),%d1  # use selected rounding mode
#
#
         bsr.l     round            # round the unnorm based on users
#					;input	a0 ptr to ext X
#					;	d0 g,r,s bits
#					;	d1 PREC/MODE info
#					;output a0 ptr to rounded result
#					;inexact flag set sints_in USER_FPSR
#					;if initial grs set
#
# normalize the rounded result and store value sints_in fp0
#
         bsr.l     nrm_set          # normalize the unnorm
#					;Input: a0 points to operand to
#					;be normalized
#					;Output: a0 points to normalized
#					;result
         bfclr     local_sgn(%a0){&0:&8} 
         beq.b     sints_nrmrndp          
         bset.b    &sign_bit,local_ex(%a0) # return to IEEE extended format
sints_nrmrndp:                            
         fmove.l   %fpcr,-(%sp)     
         fmove.l   &0,%fpcr         
         fmove.x   local_ex(%a0),%fp0 # move result to fp0
         fmove.l   (%sp)+,%fpcr     
         rts                        
                                    
                                    
	version 3
ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /misc/source_product/9.10/commands.rcs/lib/libm/fp040_misc.s,v $
# $Revision: 70.3 $      $Author: ssa $
# $State: Exp $         $Locker:  $
# $Date: 92/07/02 12:20:44 $
                                    
#
#	round.sa 3.4 7/29/91
#
#	handle rounding and normalization tasks
#
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
                                    
#
#	round --- round result according rounds_to precision/mode
#
#	a0 points rounds_to the input operand rounds_in the internal extended format 
#	rounds_d1(high word) contains rounding precision:
#		ext = $0000xxxx
#		sgl = $0001xxxx
#		dbl = $0002xxxx
#	rounds_d1(low word) contains rounding mode:
#		RN  = $xxxx0000
#		RZ  = $xxxx0001
#		RM  = $xxxx0010
#		RP  = $xxxx0011
#	d0{31:29} contains the g,r,s bits (extended)
#
#	On return the value pointed rounds_to by a0 is correctly rounded,
#	a0 is preserved and the g-r-s bits rounds_in d0 are cleared.
#	The result is not typed - the tag field is invalid.  The
#	result is still rounds_in the internal extended format.
#
#	The INEX bit of USER_FPSR will be set rounds_if the rounded result was
#	inexact (i.e. rounds_if any of the g-r-s bits were set).
#
                                    
round:                              
# If g=r=s=0 then result is exact and round is done, else set 
# the rounds_inex flag rounds_in status reg and continue.  
#
         bsr.b     rounds_ext_grs          # this subroutine looks at the 
#					:rounding precision and sets 
#					;the appropriate g-r-s bits.
         tst.l     %d0              # rounds_if grs are zero, go force
         bne.w     rounds_rnd_cont         # rounds_lower bits rounds_to zero for size
                                    
         swap      %d1              # set rounds_up rounds_d1.w for round prec.
         bra.w     rounds_truncate         
                                    
rounds_rnd_cont:                            
#
# Use rounding mode as an index into a jump table for these modes.
#
         or.l      &inx2a_mask,user_fpsr(%a6) # set rounds_inex2/ainex
ifdef(`PIC',`	lea	rounds_mode_tab(%pc),%a1
',`         lea       rounds_mode_tab,%a1     
')
ifdef(`PIC',`	jmp	(%a1,%d1.w*4)
',`         move.l    (%a1,%d1.w*4),%a1 
         jmp       (%a1)            
')
#
# Jump table indexed by rounding mode rounds_in rounds_d1.w.  All following assumes
# grs != 0.
#
rounds_mode_tab:                            
ifdef(`PIC',`	bra.w	rounds_rnd_near
',`         long      rounds_rnd_near         
')
ifdef(`PIC',`	bra.w	rounds_rnd_zero
',`         long      rounds_rnd_zero         
')
ifdef(`PIC',`	bra.w	rounds_rnd_mnus
',`         long      rounds_rnd_mnus         
')
ifdef(`PIC',`	bra.w	rounds_rnd_plus
',`         long      rounds_rnd_plus         
')
#
#	ROUND PLUS INFINITY
#
#	If sign of fp number = 0 (positive), then add 1 rounds_to l.
#
rounds_rnd_plus:                            
         swap      %d1              # set rounds_up rounds_d1 for round prec.
         tst.b     local_sgn(%a0)   # check for sign
         bmi.w     rounds_truncate         # rounds_if positive then rounds_truncate
         move.l    &0xffffffff,%d0  # force g,r,s rounds_to be all f's
ifdef(`PIC',`	lea	rounds_add_to_l(%pc),%a1
',`         lea       rounds_add_to_l,%a1     
')
ifdef(`PIC',`	jmp	(%a1,%d1.w*4)
',`         move.l    (%a1,%d1.w*4),%a1 
         jmp       (%a1)            
')
#
#	ROUND MINUS INFINITY
#
#	If sign of fp number = 1 (negative), then add 1 rounds_to l.
#
rounds_rnd_mnus:                            
         swap      %d1              # set rounds_up rounds_d1 for round prec.
         tst.b     local_sgn(%a0)   # check for sign	
         bpl.w     rounds_truncate         # rounds_if negative then rounds_truncate
         move.l    &0xffffffff,%d0  # force g,r,s rounds_to be all f's
ifdef(`PIC',`	lea	rounds_add_to_l(%pc),%a1
',`         lea       rounds_add_to_l,%a1     
')
ifdef(`PIC',`	jmp	(%a1,%d1.w*4)
',`         move.l    (%a1,%d1.w*4),%a1 
         jmp       (%a1)            
')
#
#	ROUND ZERO
#
#	Always rounds_truncate.
rounds_rnd_zero:                            
         swap      %d1              # set rounds_up rounds_d1 for round prec.
         bra.w     rounds_truncate         
#
#
#	ROUND NEAREST
#
#	If (g=1), then add 1 rounds_to l and rounds_if (r=s=0), then clear l
#	Note that this will round rounds_to even rounds_in case of a tie.
#
rounds_rnd_near:                            
         swap      %d1              # set rounds_up rounds_d1 for round prec.
         asl.l     &1,%d0           # shift g-bit rounds_to c-bit
         bcc.w     rounds_truncate         # rounds_if (g=1) then
ifdef(`PIC',`	lea	rounds_add_to_l(%pc),%a1
',`         lea       rounds_add_to_l,%a1     
')
ifdef(`PIC',`	jmp	(%a1,%d1.w*4)
',`         move.l    (%a1,%d1.w*4),%a1 
         jmp       (%a1)            
')
                                    
#
#	rounds_ext_grs --- extract guard, round and rounds_sticky bits
#
# Input:	rounds_d1 =		PREC:ROUND
# Output:  	d0{31:29}=	guard, round, rounds_sticky
#
# The rounds_ext_grs extract the guard/round/rounds_sticky bits according rounds_to the
# selected rounding precision. It is called by the round subroutine
# only.  All registers except d0 are kept intact. d0 becomes an 
# updated guard,round,rounds_sticky rounds_in d0{31:29}
#
# Notes: the rounds_ext_grs uses the round PREC, and therefore has rounds_to swap rounds_d1
#	 prior rounds_to usage, and needs rounds_to restore rounds_d1 rounds_to original.
#
rounds_ext_grs:                            
         swap      %d1              # have rounds_d1.w point rounds_to round precision
         cmpi.w    %d1,&0           
         bne.b     rounds_sgl_or_dbl       
         bra.b     rounds_end_ext_grs      
                                    
rounds_sgl_or_dbl:                            
         movem.l   %d2/%d3,-(%a7)   # make some temp registers
         cmpi.w    %d1,&1           
         bne.b     rounds_grs_dbl          
rounds_grs_sgl:                            
         bfextu    local_hi(%a0){&24:&2},%d3 # sgl prec. g-r are 2 bits right
         move.l    &30,%d2          # of the sgl prec. limits
         lsl.l     %d2,%d3          # shift g-r bits rounds_to MSB of d3
         move.l    local_hi(%a0),%d2 # get word 2 for s-bit test
         andi.l    &0x0000003f,%d2  # s bit is the or of all other 
         bne.b     rounds_st_stky          # bits rounds_to the right of g-r
         tst.l     local_lo(%a0)    # test rounds_lower mantissa
         bne.b     rounds_st_stky          # rounds_if any are set, set rounds_sticky
         tst.l     %d0              # test original g,r,s
         bne.b     rounds_st_stky          # rounds_if any are set, set rounds_sticky
         bra.b     rounds_end_sd           # rounds_if words 3 and 4 are clr, exit
rounds_grs_dbl:                            
         bfextu    local_lo(%a0){&21:&2},%d3 # dbl-prec. g-r are 2 bits right
         move.l    &30,%d2          # of the dbl prec. limits
         lsl.l     %d2,%d3          # shift g-r bits rounds_to the MSB of d3
         move.l    local_lo(%a0),%d2 # get rounds_lower mantissa  for s-bit test
         andi.l    &0x000001ff,%d2  # s bit is the or-ing of all 
         bne.b     rounds_st_stky          # other bits rounds_to the right of g-r
         tst.l     %d0              # test word original g,r,s
         bne.b     rounds_st_stky          # rounds_if any are set, set rounds_sticky
         bra.b     rounds_end_sd           # rounds_if clear, exit
rounds_st_stky:                            
         bset      &rnd_stky_bit,%d3 
rounds_end_sd:                             
         move.l    %d3,%d0          # return grs rounds_to d0
         movem.l   (%a7)+,%d2/%d3   # restore scratch registers
rounds_end_ext_grs:                            
         swap      %d1              # restore rounds_d1 rounds_to original
         rts                        
                                    
#*******************  Local Equates
         set       rounds_ad_1_sgl,0x00000100 
         set       rounds_ad_1_dbl,0x00000800 
                                    
                                    
#Jump table for adding 1 rounds_to the l-bit indexed by rnd prec
                                    
rounds_add_to_l:                            
ifdef(`PIC',`	bra.w	rounds_add_ext
',`         long      rounds_add_ext          
')
ifdef(`PIC',`	bra.w	rounds_add_sgl
',`         long      rounds_add_sgl          
')
ifdef(`PIC',`	bra.w	rounds_add_dbl
',`         long      rounds_add_dbl          
')
ifdef(`PIC',`	bra.w	rounds_add_dbl
',`         long      rounds_add_dbl          
')
#
#	ADD SINGLE
#
rounds_add_sgl:                            
         add.l     &rounds_ad_1_sgl,local_hi(%a0) 
         bcc.b     rounds_scc_clr          # rounds_no mantissa overflow
         roxr.w    local_hi(%a0)    # shift v-bit back rounds_in
         roxr.w    local_hi+2(%a0)  # shift v-bit back rounds_in
         add.w     &0x1,local_ex(%a0) # and incr rounds_exponent
rounds_scc_clr:                            
         tst.l     %d0              # test for rs = 0
         bne.b     rounds_sgl_done         
         andi.w    &0xfe00,local_hi+2(%a0) # clear the l-bit
rounds_sgl_done:                            
         andi.l    &0xffffff00,local_hi(%a0) # rounds_truncate bits beyond sgl limit
         clr.l     local_lo(%a0)    # clear rounds_d2
         rts                        
                                    
#
#	ADD EXTENDED
#
rounds_add_ext:                            
         addq.l    &1,local_lo(%a0) # add 1 rounds_to l-bit
         bcc.b     rounds_xcc_clr          # test for carry out
         addq.l    &1,local_hi(%a0) # propogate carry
         bcc.b     rounds_xcc_clr          
         roxr.w    local_hi(%a0)    # mant is 0 so restore v-bit
         roxr.w    local_hi+2(%a0)  # mant is 0 so restore v-bit
         roxr.w    local_lo(%a0)    
         roxr.w    local_lo+2(%a0)  
         add.w     &0x1,local_ex(%a0) # and inc rounds_exp
rounds_xcc_clr:                            
         tst.l     %d0              # test rs = 0
         bne.b     rounds_add_ext_done     
         andi.b    &0xfe,local_lo+3(%a0) # clear the l bit
rounds_add_ext_done:                            
         rts                        
#
#	ADD DOUBLE
#
rounds_add_dbl:                            
         add.l     &rounds_ad_1_dbl,local_lo(%a0) 
         bcc.b     rounds_dcc_clr          
         addq.l    &1,local_hi(%a0) # propogate carry
         bcc.b     rounds_dcc_clr          
         roxr.w    local_hi(%a0)    # mant is 0 so restore v-bit
         roxr.w    local_hi+2(%a0)  # mant is 0 so restore v-bit
         roxr.w    local_lo(%a0)    
         roxr.w    local_lo+2(%a0)  
         add.w     &0x1,local_ex(%a0) # incr rounds_exponent
rounds_dcc_clr:                            
         tst.l     %d0              # test for rs = 0
         bne.b     rounds_dbl_done         
         andi.w    &0xf000,local_lo+2(%a0) # clear the l-bit
                                    
rounds_dbl_done:                            
         andi.l    &0xfffff800,local_lo(%a0) # rounds_truncate bits beyond dbl limit
         rts                        
                                    
rounds_error:                              
         rts                        
#
# Truncate all other bits
#
rounds_trunct:                             
ifdef(`PIC',`	bra.w	rounds_end_rnd
',`         long      rounds_end_rnd          
')
ifdef(`PIC',`	bra.w	rounds_sgl_done
',`         long      rounds_sgl_done         
')
ifdef(`PIC',`	bra.w	rounds_dbl_done
',`         long      rounds_dbl_done         
')
ifdef(`PIC',`	bra.w	rounds_dbl_done
',`         long      rounds_dbl_done         
')
                                    
rounds_truncate:                            
ifdef(`PIC',`	lea	rounds_trunct(%pc),%a1
',`         lea       rounds_trunct,%a1       
')
ifdef(`PIC',`	jmp	(%a1,%d1.w*4)
',`         move.l    (%a1,%d1.w*4),%a1 
         jmp       (%a1)            
')
                                    
rounds_end_rnd:                            
         rts                        
                                    
#
#	NORMALIZE
#
# These routines (nrm_zero & nrm_set) normalize the unnorm.  This 
# is done by shifting the mantissa left while decrementing the 
# rounds_exponent.
#
# NRM_SET shifts and decrements until there is a 1 set rounds_in the integer 
# bit of the mantissa (msb rounds_in rounds_d1).
#
# NRM_ZERO shifts and decrements until there is a 1 set rounds_in the integer 
# bit of the mantissa (msb rounds_in rounds_d1) unless this would mean the rounds_exponent 
# would go less than 0.  In that case the number becomes a denorm - the 
# rounds_exponent (d0) is set rounds_to 0 and the mantissa (d1 & rounds_d2) is not 
# normalized.
#
# Note that both routines have been optimized (for the worst case) and 
# therefore do not have the easy rounds_to follow decrement/shift loop.
#
#	NRM_ZERO
#
#	Distance rounds_to first 1 bit rounds_in mantissa = X
#	Distance rounds_to 0 from rounds_exponent = Y
#	If X < Y
#	Then
#	  nrm_set
#	Else
#	  shift mantissa by Y
#	  set rounds_exponent = 0
#
#input:
#	FP_SCR1 = rounds_exponent, ms mantissa part, ls mantissa part
#output:
#	L_SCR1{4} = fpte15 or ete15 bit
#
nrm_zero:                            
         move.w    local_ex(%a0),%d0 
         cmp.w     %d0,&64          # see rounds_if rounds_exp > 64 
         bmi.b     rounds_d0_less          
         bsr       nrm_set          # rounds_exp > 64 so rounds_exp won't exceed 0 
         rts                        
rounds_d0_less:                            
         movem.l   %d2/%d3/%d5/%d6,-(%a7) 
         move.l    local_hi(%a0),%d1 
         move.l    local_lo(%a0),%d2 
                                    
         bfffo     %d1{&0:&32},%d3  # get the distance rounds_to the first 1 
#				;in ms mant
         beq.b     rounds_ms_clr           # branch rounds_if rounds_no bits were set
         cmp.w     %d0,%d3          # of X>Y
         bmi.b     rounds_greater          # then rounds_exp will go past 0 (neg) rounds_if 
#				;it is just shifted
         bsr       nrm_set          # else rounds_exp won't go past 0
         movem.l   (%a7)+,%d2/%d3/%d5/%d6 
         rts                        
rounds_greater:                            
         move.l    %d2,%d6          # save ls mant rounds_in d6
         lsl.l     %d0,%d2          # shift ls mant by count
         lsl.l     %d0,%d1          # shift ms mant by count
         move.l    &32,%d5          
         sub.l     %d0,%d5          # make op a denorm by shifting bits 
         lsr.l     %d5,%d6          # by the number rounds_in the rounds_exp, then 
#				;set rounds_exp = 0.
         or.l      %d6,%d1          # shift the ls mant bits into the ms mant
         move.l    &0,%d0           # same as rounds_if decremented rounds_exp rounds_to 0 
#				;while shifting
         move.w    %d0,local_ex(%a0) 
         move.l    %d1,local_hi(%a0) 
         move.l    %d2,local_lo(%a0) 
         movem.l   (%a7)+,%d2/%d3/%d5/%d6 
         rts                        
rounds_ms_clr:                             
         bfffo     %d2{&0:&32},%d3  # check rounds_if any bits set rounds_in ls mant
         beq.b     rounds_all_clr          # branch rounds_if none set
         add.w     &32,%d3          
         cmp.w     %d0,%d3          # rounds_if X>Y
         bmi.b     rounds_greater          # then branch
         bsr       nrm_set          # else rounds_exp won't go past 0
         movem.l   (%a7)+,%d2/%d3/%d5/%d6 
         rts                        
rounds_all_clr:                            
         move.w    &0,local_ex(%a0) # rounds_no mantissa bits set. Set rounds_exp = 0.
         movem.l   (%a7)+,%d2/%d3/%d5/%d6 
         rts                        
#
#	NRM_SET
#
nrm_set:                            
         move.l    %d7,-(%a7)       
         bfffo     local_hi(%a0){&0:&32},%d7 # find first 1 rounds_in ms mant rounds_to d7)
         beq.b     rounds_lower            # branch rounds_if ms mant is all 0's
                                    
         move.l    %d6,-(%a7)       
                                    
         sub.w     %d7,local_ex(%a0) # sub rounds_exponent by count
         move.l    local_hi(%a0),%d0 # d0 has ms mant
         move.l    local_lo(%a0),%d1 # rounds_d1 has ls mant
                                    
         lsl.l     %d7,%d0          # shift first 1 rounds_to j bit position
         move.l    %d1,%d6          # copy ls mant into d6
         lsl.l     %d7,%d6          # shift ls mant by count
         move.l    %d6,local_lo(%a0) # store ls mant into memory
         moveq.l   &32,%d6          
         sub.l     %d7,%d6          # continue shift
         lsr.l     %d6,%d1          # shift off all bits but those that will
#				;be shifted into ms mant
         or.l      %d1,%d0          # shift the ls mant bits into the ms mant
         move.l    %d0,local_hi(%a0) # store ms mant into memory
         movem.l   (%a7)+,%d7/%d6   # restore registers
         rts                        
                                    
#
# We get here rounds_if ms mant was = 0, and we assume ls mant has bits 
# set (otherwise this would have been tagged a zero not a denorm).
#
rounds_lower:                              
         move.w    local_ex(%a0),%d0 # d0 has rounds_exponent
         move.l    local_lo(%a0),%d1 # rounds_d1 has ls mant
         sub.w     &32,%d0          # account for ms mant being all zeros
         bfffo     %d1{&0:&32},%d7  # find first 1 rounds_in ls mant rounds_to d7)
         sub.w     %d7,%d0          # subtract shift count from rounds_exp
         lsl.l     %d7,%d1          # shift first 1 rounds_to integer bit rounds_in ms mant
         move.w    %d0,local_ex(%a0) # store ms mant
         move.l    %d1,local_hi(%a0) # store rounds_exp
         clr.l     local_lo(%a0)    # clear ls mant
         move.l    (%a7)+,%d7       
         rts                        
#
#	denorm --- denormalize an intermediate result
#
#	Used by underflow.
#
# Input: 
#	a0	 points rounds_to the operand rounds_to be denormalized
#		 (in the internal extended format)
#		 
#	d0: 	 rounding precision
# Output:
#	a0	 points rounds_to the denormalized result
#		 (in the internal extended format)
#
#	d0 	is guard,round,rounds_sticky
#
# d0 comes into this routine with the rounding precision. It 
# is then loaded with the denormalized rounds_exponent threshold for the 
# rounding precision.
#
                                    
denorm:                             
         btst.b    &6,local_ex(%a0) # check for exponents between $7fff-$4000
         beq.b     rounds_no_sgn_ext       
         bset.b    &7,local_ex(%a0) # sign extend rounds_if it is so
rounds_no_sgn_ext:                            
                                    
         cmpi.b    %d0,&0           # rounds_if 0 then extended precision
         bne.b     rounds_not_ext          # else branch
                                    
         clr.l     %d1              # load rounds_d1 with ext threshold
         clr.l     %d0              # clear the rounds_sticky flag
         bsr       dnrm_lp          # denormalize the number
         tst.b     %d1              # check for rounds_inex
         beq.w     rounds_no_inex          # rounds_if clr, rounds_no rounds_inex
         bra.b     rounds_dnrm_inex        # rounds_if set, set rounds_inex
                                    
rounds_not_ext:                            
         cmpi.l    %d0,&1           # rounds_if 1 then single precision
         beq.b     rounds_load_sgl         # else must be 2, double prec
                                    
rounds_load_dbl:                            
         move.w    &dbl_thresh,%d1  # put copy of threshold rounds_in rounds_d1
         move.l    %d1,%d0          # copy rounds_d1 into d0
         sub.w     local_ex(%a0),%d0 # diff = threshold - rounds_exp
         cmp.w     %d0,&67          # rounds_if diff > 67 (mant + grs bits)
         bpl.b     rounds_chk_stky         # then branch (all bits would be 
#				; shifted off rounds_in denorm routine)
         clr.l     %d0              # else clear the rounds_sticky flag
         bsr       dnrm_lp          # denormalize the number
         tst.b     %d1              # check flag
         beq.b     rounds_no_inex          # rounds_if clr, rounds_no rounds_inex
         bra.b     rounds_dnrm_inex        # rounds_if set, set rounds_inex
                                    
rounds_load_sgl:                            
         move.w    &sgl_thresh,%d1  # put copy of threshold rounds_in rounds_d1
         move.l    %d1,%d0          # copy rounds_d1 into d0
         sub.w     local_ex(%a0),%d0 # diff = threshold - rounds_exp
         cmp.w     %d0,&67          # rounds_if diff > 67 (mant + grs bits)
         bpl.b     rounds_chk_stky         # then branch (all bits would be 
#				; shifted off rounds_in denorm routine)
         clr.l     %d0              # else clear the rounds_sticky flag
         bsr       dnrm_lp          # denormalize the number
         tst.b     %d1              # check flag
         beq.b     rounds_no_inex          # rounds_if clr, rounds_no rounds_inex
         bra.b     rounds_dnrm_inex        # rounds_if set, set rounds_inex
                                    
rounds_chk_stky:                            
         tst.l     local_hi(%a0)    # check for any bits set
         bne.b     rounds_set_stky         
         tst.l     local_lo(%a0)    # check for any bits set
         bne.b     rounds_set_stky         
         bra.b     rounds_clr_mant         
rounds_set_stky:                            
         or.l      &inx2a_mask,user_fpsr(%a6) # set rounds_inex2/ainex
         move.l    &0x20000000,%d0  # set rounds_sticky bit rounds_in return value
rounds_clr_mant:                            
         move.w    %d1,local_ex(%a0) # load rounds_exp with threshold
         move.l    &0,local_hi(%a0) # set rounds_d1 = 0 (ms mantissa)
         move.l    &0,local_lo(%a0) # set rounds_d2 = 0 (ms mantissa)
         rts                        
rounds_dnrm_inex:                            
         or.l      &inx2a_mask,user_fpsr(%a6) # set rounds_inex2/ainex
rounds_no_inex:                            
         rts                        
                                    
#
#	dnrm_lp --- normalize rounds_exponent/mantissa rounds_to specified threshhold
#
# Input:
#	a0		points rounds_to the operand rounds_to be denormalized
#	d0{31:29} 	initial guard,round,rounds_sticky
#	rounds_d1{15:0}	denormalization threshold
# Output:
#	a0		points rounds_to the denormalized operand
#	d0{31:29}	final guard,round,rounds_sticky
#	rounds_d1.b		inexact flag:  all ones means inexact result
#
# The LOCAL_LO and LOCAL_GRS parts of the value are copied rounds_to FP_SCR2
# so that bfext can be used rounds_to extract the new low part of the mantissa.
# Dnrm_lp can be called with a0 pointing rounds_to ETEMP or WBTEMP and there 
# is rounds_no LOCAL_GRS scratch word following it on the fsave frame.
#
dnrm_lp:                            
         move.l    %d2,-(%sp)       # save rounds_d2 for temp use
         btst.b    &e3,e_byte(%a6)  # test for type E3 exception
         beq.b     rounds_not_e3           # not type E3 exception
         bfextu    wbtemp_grs(%a6){&6:&3},%d2 # extract guard,round, rounds_sticky  bit
         move.l    &29,%d0          
         lsl.l     %d0,%d2          # shift g,r,s rounds_to their postions
         move.l    %d2,%d0          
rounds_not_e3:                             
         move.l    (%sp)+,%d2       # restore rounds_d2
         move.l    local_lo(%a0),fp_scr2+local_lo(%a6) 
         move.l    %d0,fp_scr2+local_grs(%a6) 
         move.l    %d1,%d0          # copy the denorm threshold
         sub.w     local_ex(%a0),%d1 # rounds_d1 = threshold - uns rounds_exponent
         ble.b     rounds_no_lp            # rounds_d1 <= 0
         cmp.w     %d1,&32          
         blt.b     rounds_case_1           # 0 = rounds_d1 < 32 
         cmp.w     %d1,&64          
         blt.b     rounds_case_2           # 32 <= rounds_d1 < 64
         bra.w     rounds_case_3           # rounds_d1 >= 64
#
# No normalization necessary
#
rounds_no_lp:                              
         clr.b     %d1              # set rounds_no rounds_inex2 reported
         move.l    fp_scr2+local_grs(%a6),%d0 # restore original g,r,s
         rts                        
#
# case (0<d1<32)
#
rounds_case_1:                             
         move.l    %d2,-(%sp)       
         move.w    %d0,local_ex(%a0) # rounds_exponent = denorm threshold
         move.l    &32,%d0          
         sub.w     %d1,%d0          # d0 = 32 - rounds_d1
         bfextu    local_ex(%a0){%d0:&32},%d2 
         bfextu    %d2{%d1:%d0},%d2 # rounds_d2 = new LOCAL_HI
         bfextu    local_hi(%a0){%d0:&32},%d1 # rounds_d1 = new LOCAL_LO
         bfextu    fp_scr2+local_lo(%a6){%d0:&32},%d0 # d0 = new G,R,S
         move.l    %d2,local_hi(%a0) # store new LOCAL_HI
         move.l    %d1,local_lo(%a0) # store new LOCAL_LO
         clr.b     %d1              
         bftst     %d0{&2:&30}      
         beq.b     rounds_c1nstky          
         bset.l    &rnd_stky_bit,%d0 
         st.b      %d1              
rounds_c1nstky:                            
         move.l    fp_scr2+local_grs(%a6),%d2 # restore original g,r,s
         andi.l    &0xe0000000,%d2  # clear all but G,R,S
         tst.l     %d2              # test rounds_if original G,R,S are clear
         beq.b     rounds_grs_clear        
         or.l      &0x20000000,%d0  # set rounds_sticky bit rounds_in d0
rounds_grs_clear:                            
         andi.l    &0xe0000000,%d0  # clear all but G,R,S
         move.l    (%sp)+,%d2       
         rts                        
#
# case (32<=d1<64)
#
rounds_case_2:                             
         move.l    %d2,-(%sp)       
         move.w    %d0,local_ex(%a0) # unsigned rounds_exponent = threshold
         sub.w     &32,%d1          # rounds_d1 now between 0 and 32
         move.l    &32,%d0          
         sub.w     %d1,%d0          # d0 = 32 - rounds_d1
         bfextu    local_ex(%a0){%d0:&32},%d2 
         bfextu    %d2{%d1:%d0},%d2 # rounds_d2 = new LOCAL_LO
         bfextu    local_hi(%a0){%d0:&32},%d1 # rounds_d1 = new G,R,S
         bftst     %d1{&2:&30}      
         bne.b     rounds_c2_sstky         # bra rounds_if rounds_sticky bit rounds_to be set
         bftst     fp_scr2+local_lo(%a6){%d0:&32} 
         bne.b     rounds_c2_sstky         # bra rounds_if rounds_sticky bit rounds_to be set
         move.l    %d1,%d0          
         clr.b     %d1              
         bra.b     rounds_end_c2           
rounds_c2_sstky:                            
         move.l    %d1,%d0          
         bset.l    &rnd_stky_bit,%d0 
         st.b      %d1              
rounds_end_c2:                             
         clr.l     local_hi(%a0)    # store LOCAL_HI = 0
         move.l    %d2,local_lo(%a0) # store LOCAL_LO
         move.l    fp_scr2+local_grs(%a6),%d2 # restore original g,r,s
         andi.l    &0xe0000000,%d2  # clear all but G,R,S
         tst.l     %d2              # test rounds_if original G,R,S are clear
         beq.b     rounds_clear_grs        
         or.l      &0x20000000,%d0  # set rounds_sticky bit rounds_in d0
rounds_clear_grs:                            
         andi.l    &0xe0000000,%d0  # get rid of all but G,R,S
         move.l    (%sp)+,%d2       
         rts                        
#
# rounds_d1 >= 64 Force the rounds_exponent rounds_to be the denorm threshold with the
# correct sign.
#
rounds_case_3:                             
         move.w    %d0,local_ex(%a0) 
         tst.w     local_sgn(%a0)   
         bge.b     rounds_c3con            
rounds_c3neg:                              
         or.l      &0x80000000,local_ex(%a0) 
rounds_c3con:                              
         cmp.w     %d1,&64          
         beq.b     rounds_sixty_four       
         cmp.w     %d1,&65          
         beq.b     rounds_sixty_five       
#
# Shift value is out of range.  Set rounds_d1 for rounds_inex2 flag and
# return a zero with the given threshold.
#
         clr.l     local_hi(%a0)    
         clr.l     local_lo(%a0)    
         move.l    &0x20000000,%d0  
         st.b      %d1              
         rts                        
                                    
rounds_sixty_four:                            
         move.l    local_hi(%a0),%d0 
         bfextu    %d0{&2:&30},%d1  
         andi.l    &0xc0000000,%d0  
         bra.b     rounds_c3com            
                                    
rounds_sixty_five:                            
         move.l    local_hi(%a0),%d0 
         bfextu    %d0{&1:&31},%d1  
         andi.l    &0x80000000,%d0  
         lsr.l     &1,%d0           # shift high bit into R bit
                                    
rounds_c3com:                              
         tst.l     %d1              
         bne.b     rounds_c3ssticky        
         tst.l     local_lo(%a0)    
         bne.b     rounds_c3ssticky        
         tst.b     fp_scr2+local_grs(%a6) 
         bne.b     rounds_c3ssticky        
         clr.b     %d1              
         bra.b     rounds_c3end            
                                    
rounds_c3ssticky:                            
         bset.l    &rnd_stky_bit,%d0 
         st.b      %d1              
rounds_c3end:                              
         clr.l     local_hi(%a0)    
         clr.l     local_lo(%a0)    
         rts                        
                                    
                                    
	version 3
