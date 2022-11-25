ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /misc/source_product/9.10/commands.rcs/lib/libm/fp040_atrig.s,v $
# $Revision: 70.2 $      $Author: ssa $
# $State: Exp $         $Locker:  $
# $Date: 91/08/30 00:11:14 $
                                    
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
define(FILE_atrig)


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
# $Source: /misc/source_product/9.10/commands.rcs/lib/libm/fp040_atrig.s,v $
# $Revision: 70.2 $      $Author: ssa $
# $State: Exp $         $Locker:  $
# $Date: 91/08/30 00:11:14 $
                                    
#
#	sasin.sa 3.3 12/19/90
#
#	Description: The entry point sAsin computes the inverse sine of
#		an input argument; sAsind does the same except for denormalized
#		input.
#
#	Input: Double-extended number X in location pointed to
#		by address register a0.
#
#	Output: The value arcsin(X) returned in floating-point register Fp0.
#
#	Accuracy and Monotonicity: The returned result is within 3 ulps in
#		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
#		result is subsequently rounded to double precision. The 
#		result is provably monotonic in double precision.
#
#	Speed: The program sASIN takes approximately 310 cycles.
#
#	Algorithm:
#
#	ASIN
#	1. If |X| >= 1, go to 3.
#
#	2. (|X| < 1) Calculate asin(X) by
#		z := sqrt( [1-X][1+X] )
#		asin(X) = atan( x / z ).
#		Exit.
#
#	3. If |X| > 1, go to 5.
#
#	4. (|X| = 1) sgn := sign(X), return asin(X) := sgn * Pi/2. Exit.
#
#	5. (|X| > 1) Generate an invalid operation by 0 * infinity.
#		Exit.
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
sasins_piby2:   long      0x3fff0000,0xc90fdaa2,0x2168c235,0x00000000 
                                    
                                    
                                    
                                    
                                    
                                    
sasind:                             
#--ASIN(X) = X FOR DENORMALIZED X
                                    
         bra.l     t_extdnrm        
                                    
sasin:                              
         fmove.x   (%a0),%fp0       # LOAD INPUT
                                    
         move.l    (%a0),%d0        
         move.w    4(%a0),%d0       
         andi.l    &0x7fffffff,%d0  
         cmpi.l    %d0,&0x3fff8000  
         bge.b     sasins_asinbig          
                                    
#--THIS IS THE USUAL CASE, |X| < 1
#--ASIN(X) = ATAN( X / SQRT( (1-X)(1+X) ) )
                                    
         fmove.s   &0f1.0,%fp1      
         fsub.x    %fp0,%fp1        # 1-X
         fmovem.x  %fp2,-(%a7)      
         fmove.s   &0f1.0,%fp2      
         fadd.x    %fp0,%fp2        # 1+X
         fmul.x    %fp2,%fp1        # (1+X)(1-X)
         fmovem.x  (%a7)+,%fp2      
         fsqrt.x   %fp1             # SQRT([1-X][1+X])
         fdiv.x    %fp1,%fp0        # X/SQRT([1-X][1+X])
         fmovem.x  %fp0,(%a0)       
         bsr.l     satan            
         bra.l     t_frcinx         
                                    
sasins_asinbig:                            
         fabs.x    %fp0             # |X|
         fcmp.s    %fp0,&0f1.0      
         fbgt.l    t_operr          # cause an operr exception
                                    
#--|X| = 1, ASIN(X) = +- PI/2.
                                    
ifdef(`PIC',`	fmove.x	&0x3fff0000c90fdaa22168c235,%fp0
',`         fmove.x   sasins_piby2,%fp0       
')
         move.l    (%a0),%d0        
         andi.l    &0x80000000,%d0  # SIGN BIT OF X
         ori.l     &0x3f800000,%d0  # +-1 IN SGL FORMAT
         move.l    %d0,-(%sp)       # push SIGN(X) IN SGL-FMT
         fmove.l   %d1,%fpcr        
         fmul.s    (%sp)+,%fp0      
         bra.l     t_frcinx         
                                    
                                    
	version 3
ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /misc/source_product/9.10/commands.rcs/lib/libm/fp040_atrig.s,v $
# $Revision: 70.2 $      $Author: ssa $
# $State: Exp $         $Locker:  $
# $Date: 91/08/30 00:11:14 $
                                    
#
#	sacos.sa 3.3 12/19/90
#
#	Description: The entry point sAcos computes the inverse cosine of
#		an input argument; sAcosd does the same except for denormalized
#		input.
#
#	Input: Double-extended number X in location pointed to
#		by address register a0.
#
#	Output: The value arccos(X) returned in floating-point register Fp0.
#
#	Accuracy and Monotonicity: The returned result is within 3 ulps in
#		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
#		result is subsequently rounded to double precision. The 
#		result is provably monotonic in double precision.
#
#	Speed: The program sCOS takes approximately 310 cycles.
#
#	Algorithm:
#
#	ACOS
#	1. If |X| >= 1, go to 3.
#
#	2. (|X| < 1) Calculate acos(X) by
#		z := (1-X) / (1+X)
#		acos(X) = 2 * atan( sqrt(z) ).
#		Exit.
#
#	3. If |X| > 1, go to 5.
#
#	4. (|X| = 1) If X > 0, return 0. Otherwise, return Pi. Exit.
#
#	5. (|X| > 1) Generate an invalid operation by 0 * infinity.
#		Exit.
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
sacoss_pi:      long      0x40000000,0xc90fdaa2,0x2168c235,0x00000000 
sacoss_piby2:   long      0x3fff0000,0xc90fdaa2,0x2168c235,0x00000000 
                                    
                                    
                                    
                                    
                                    
sacosd:                             
#--ACOS(X) = PI/2 FOR DENORMALIZED X
         fmove.l   %d1,%fpcr        # load user's rounding mode/precision
ifdef(`PIC',`	fmove.x	&0x3fff0000c90fdaa22168c235,%fp0
',`         fmove.x   sacoss_piby2,%fp0       
')
         bra.l     t_frcinx         
                                    
sacos:                              
         fmove.x   (%a0),%fp0       # LOAD INPUT
                                    
         move.l    (%a0),%d0        # pack exponent with upper 16 fraction
         move.w    4(%a0),%d0       
         andi.l    &0x7fffffff,%d0  
         cmpi.l    %d0,&0x3fff8000  
         bge.b     sacoss_acosbig          
                                    
#--THIS IS THE USUAL CASE, |X| < 1
#--ACOS(X) = 2 * ATAN(	SQRT( (1-X)/(1+X) )	)
                                    
         fmove.s   &0f1.0,%fp1      
         fadd.x    %fp0,%fp1        # 1+X
         fneg.x    %fp0             #  -X
         fadd.s    &0f1.0,%fp0      # 1-X
         fdiv.x    %fp1,%fp0        # (1-X)/(1+X)
         fsqrt.x   %fp0             # SQRT((1-X)/(1+X))
         fmovem.x  %fp0,(%a0)       # overwrite input
         move.l    %d1,-(%sp)       # save original users fpcr
         clr.l     %d1              
         bsr.l     satan            # ATAN(SQRT([1-X]/[1+X]))
         fmove.l   (%sp)+,%fpcr     # restore users exceptions
         fadd.x    %fp0,%fp0        # 2 * ATAN( STUFF )
         bra.l     t_frcinx         
                                    
sacoss_acosbig:                            
         fabs.x    %fp0             
         fcmp.s    %fp0,&0f1.0      
         fbgt.l    t_operr          # cause an operr exception
                                    
#--|X| = 1, ACOS(X) = 0 OR PI
         move.l    (%a0),%d0        # pack exponent with upper 16 fraction
         move.w    4(%a0),%d0       
         cmp.l     %d0,&0           # D0 has original exponent+fraction
         bgt.b     sacoss_acosp1           
                                    
#--X = -1
#Returns PI and inexact exception
ifdef(`PIC',`	fmove.x	&0x40000000c90fdaa22168c235,%fp0
',`         fmove.x   sacoss_pi,%fp0          
')
         fmove.l   %d1,%fpcr        
         fadd.s    &0f1.1754943e-38,%fp0 # cause an inexact exception to be put
#					;into the 040 - will not trap until next
#					;fp inst.
         bra.l     t_frcinx         
                                    
sacoss_acosp1:                             
         fmove.l   %d1,%fpcr        
         fmove.s   &0f0.0,%fp0      
         rts                        # Facos of +1 is exact	
                                    
                                    
	version 3
ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /misc/source_product/9.10/commands.rcs/lib/libm/fp040_atrig.s,v $
# $Revision: 70.2 $      $Author: ssa $
# $State: Exp $         $Locker:  $
# $Date: 91/08/30 00:11:14 $
                                    
#
#	satan.sa 3.3 12/19/90
#
#	The entry point satan computes the arctagent of an
#	input value. satand does the same except the input value is a
#	denormalized number.
#
#	Input: Double-extended value in memory location pointed to by address
#		register a0.
#
#	Output:	Arctan(X) returned in floating-point register Fp0.
#
#	Accuracy and Monotonicity: The returned result is within 2 ulps in
#		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
#		result is subsequently rounded to double precision. The
#		result is provably monotonic in double precision.
#
#	Speed: The program satan takes approximately 160 cycles for input
#		argument X such that 1/16 < |X| < 16. For the other arguments,
#		the program will run no worse than 10% slower.
#
#	Algorithm:
#	Step 1. If |X| >= 16 or |X| < 1/16, go to Step 5.
#
#	Step 2. Let X = sgn * 2**k * 1.xxxxxxxx...x. Note that k = -4, -3,..., or 3.
#		Define F = sgn * 2**k * 1.xxxx1, i.e. the first 5 significant bits
#		of X with a bit-1 attached at the 6-th bit position. Define u
#		to be u = (X-F) / (1 + X*F).
#
#	Step 3. Approximate arctan(u) by a polynomial poly.
#
#	Step 4. Return arctan(F) + poly, arctan(F) is fetched from a table of values
#		calculated beforehand. Exit.
#
#	Step 5. If |X| >= 16, go to Step 7.
#
#	Step 6. Approximate arctan(X) by an odd polynomial in X. Exit.
#
#	Step 7. Define X' = -1/X. Approximate arctan(X') by an odd polynomial in X'.
#		Arctan(X) = sign(X)*Pi/2 + arctan(X'). Exit.
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
	include(fpsp.h.s)
                                    
satans_bounds1: long      0x3ffb8000,0x4002ffff 
                                    
satans_one:     long      0x3f800000       
                                    
         long      0x00000000       
                                    
satans_atana3:  long      0xbff6687e,0x314987d8 
satans_atana2:  long      0x4002ac69,0x34a26db3 
                                    
satans_atana1:  long      0xbfc2476f,0x4e1da28e 
satans_atanb6:  long      0x3fb34444,0x7f876989 
                                    
satans_atanb5:  long      0xbfb744ee,0x7faf45db 
satans_atanb4:  long      0x3fbc71c6,0x46940220 
                                    
satans_atanb3:  long      0xbfc24924,0x921872f9 
satans_atanb2:  long      0x3fc99999,0x99998fa9 
                                    
satans_atanb1:  long      0xbfd55555,0x55555555 
satans_atanc5:  long      0xbfb70bf3,0x98539e6a 
                                    
satans_atanc4:  long      0x3fbc7187,0x962d1d7d 
satans_atanc3:  long      0xbfc24924,0x827107b8 
                                    
satans_atanc2:  long      0x3fc99999,0x9996263e 
satans_atanc1:  long      0xbfd55555,0x55555536 
                                    
satans_ppiby2:  long      0x3fff0000,0xc90fdaa2,0x2168c235,0x00000000 
satans_npiby2:  long      0xbfff0000,0xc90fdaa2,0x2168c235,0x00000000 
satans_ptiny:   long      0x00010000,0x80000000,0x00000000,0x00000000 
satans_ntiny:   long      0x80010000,0x80000000,0x00000000,0x00000000 
                                    
satans_atantbl:                            
         long      0x3ffb0000,0x83d152c5,0x060b7a51,0x00000000 
         long      0x3ffb0000,0x8bc85445,0x65498b8b,0x00000000 
         long      0x3ffb0000,0x93be4060,0x17626b0d,0x00000000 
         long      0x3ffb0000,0x9bb3078d,0x35aec202,0x00000000 
         long      0x3ffb0000,0xa3a69a52,0x5ddce7de,0x00000000 
         long      0x3ffb0000,0xab98e943,0x62765619,0x00000000 
         long      0x3ffb0000,0xb389e502,0xf9c59862,0x00000000 
         long      0x3ffb0000,0xbb797e43,0x6b09e6fb,0x00000000 
         long      0x3ffb0000,0xc367a5c7,0x39e5f446,0x00000000 
         long      0x3ffb0000,0xcb544c61,0xcff7d5c6,0x00000000 
         long      0x3ffb0000,0xd33f62f8,0x2488533e,0x00000000 
         long      0x3ffb0000,0xdb28da81,0x62404c77,0x00000000 
         long      0x3ffb0000,0xe310a407,0x8ad34f18,0x00000000 
         long      0x3ffb0000,0xeaf6b0a8,0x188ee1eb,0x00000000 
         long      0x3ffb0000,0xf2daf194,0x9dbe79d5,0x00000000 
         long      0x3ffb0000,0xfabd5813,0x61d47e3e,0x00000000 
         long      0x3ffc0000,0x8346ac21,0x0959ecc4,0x00000000 
         long      0x3ffc0000,0x8b232a08,0x304282d8,0x00000000 
         long      0x3ffc0000,0x92fb70b8,0xd29ae2f9,0x00000000 
         long      0x3ffc0000,0x9acf476f,0x5ccd1cb4,0x00000000 
         long      0x3ffc0000,0xa29e7630,0x4954f23f,0x00000000 
         long      0x3ffc0000,0xaa68c5d0,0x8ab85230,0x00000000 
         long      0x3ffc0000,0xb22dfffd,0x9d539f83,0x00000000 
         long      0x3ffc0000,0xb9edef45,0x3e900ea5,0x00000000 
         long      0x3ffc0000,0xc1a85f1c,0xc75e3ea5,0x00000000 
         long      0x3ffc0000,0xc95d1be8,0x28138de6,0x00000000 
         long      0x3ffc0000,0xd10bf300,0x840d2de4,0x00000000 
         long      0x3ffc0000,0xd8b4b2ba,0x6bc05e7a,0x00000000 
         long      0x3ffc0000,0xe0572a6b,0xb42335f6,0x00000000 
         long      0x3ffc0000,0xe7f32a70,0xea9caa8f,0x00000000 
         long      0x3ffc0000,0xef888432,0x64ecefaa,0x00000000 
         long      0x3ffc0000,0xf7170a28,0xecc06666,0x00000000 
         long      0x3ffd0000,0x812fd288,0x332dad32,0x00000000 
         long      0x3ffd0000,0x88a8d1b1,0x218e4d64,0x00000000 
         long      0x3ffd0000,0x9012ab3f,0x23e4aee8,0x00000000 
         long      0x3ffd0000,0x976cc3d4,0x11e7f1b9,0x00000000 
         long      0x3ffd0000,0x9eb68949,0x3889a227,0x00000000 
         long      0x3ffd0000,0xa5ef72c3,0x4487361b,0x00000000 
         long      0x3ffd0000,0xad1700ba,0xf07a7227,0x00000000 
         long      0x3ffd0000,0xb42cbcfa,0xfd37efb7,0x00000000 
         long      0x3ffd0000,0xbb303a94,0x0ba80f89,0x00000000 
         long      0x3ffd0000,0xc22115c6,0xfcaebbaf,0x00000000 
         long      0x3ffd0000,0xc8fef3e6,0x86331221,0x00000000 
         long      0x3ffd0000,0xcfc98330,0xb4000c70,0x00000000 
         long      0x3ffd0000,0xd6807aa1,0x102c5bf9,0x00000000 
         long      0x3ffd0000,0xdd2399bc,0x31252aa3,0x00000000 
         long      0x3ffd0000,0xe3b2a855,0x6b8fc517,0x00000000 
         long      0x3ffd0000,0xea2d764f,0x64315989,0x00000000 
         long      0x3ffd0000,0xf3bf5bf8,0xbad1a21d,0x00000000 
         long      0x3ffe0000,0x801ce39e,0x0d205c9a,0x00000000 
         long      0x3ffe0000,0x8630a2da,0xda1ed066,0x00000000 
         long      0x3ffe0000,0x8c1ad445,0xf3e09b8c,0x00000000 
         long      0x3ffe0000,0x91db8f16,0x64f350e2,0x00000000 
         long      0x3ffe0000,0x97731420,0x365e538c,0x00000000 
         long      0x3ffe0000,0x9ce1c8e6,0xa0b8cdba,0x00000000 
         long      0x3ffe0000,0xa22832db,0xcadaae09,0x00000000 
         long      0x3ffe0000,0xa746f2dd,0xb7602294,0x00000000 
         long      0x3ffe0000,0xac3ec0fb,0x997dd6a2,0x00000000 
         long      0x3ffe0000,0xb110688a,0xebdc6f6a,0x00000000 
         long      0x3ffe0000,0xb5bcc490,0x59ecc4b0,0x00000000 
         long      0x3ffe0000,0xba44bc7d,0xd470782f,0x00000000 
         long      0x3ffe0000,0xbea94144,0xfd049aac,0x00000000 
         long      0x3ffe0000,0xc2eb4abb,0x661628b6,0x00000000 
         long      0x3ffe0000,0xc70bd54c,0xe602ee14,0x00000000 
         long      0x3ffe0000,0xcd000549,0xadec7159,0x00000000 
         long      0x3ffe0000,0xd48457d2,0xd8ea4ea3,0x00000000 
         long      0x3ffe0000,0xdb948da7,0x12dece3b,0x00000000 
         long      0x3ffe0000,0xe23855f9,0x69e8096a,0x00000000 
         long      0x3ffe0000,0xe8771129,0xc4353259,0x00000000 
         long      0x3ffe0000,0xee57c16e,0x0d379c0d,0x00000000 
         long      0x3ffe0000,0xf3e10211,0xa87c3779,0x00000000 
         long      0x3ffe0000,0xf919039d,0x758b8d41,0x00000000 
         long      0x3ffe0000,0xfe058b8f,0x64935fb3,0x00000000 
         long      0x3fff0000,0x8155fb49,0x7b685d04,0x00000000 
         long      0x3fff0000,0x83889e35,0x49d108e1,0x00000000 
         long      0x3fff0000,0x859cfa76,0x511d724b,0x00000000 
         long      0x3fff0000,0x87952ecf,0xff8131e7,0x00000000 
         long      0x3fff0000,0x89732fd1,0x9557641b,0x00000000 
         long      0x3fff0000,0x8b38cad1,0x01932a35,0x00000000 
         long      0x3fff0000,0x8ce7a8d8,0x301ee6b5,0x00000000 
         long      0x3fff0000,0x8f46a39e,0x2eae5281,0x00000000 
         long      0x3fff0000,0x922da7d7,0x91888487,0x00000000 
         long      0x3fff0000,0x94d19fcb,0xdedf5241,0x00000000 
         long      0x3fff0000,0x973ab944,0x19d2a08b,0x00000000 
         long      0x3fff0000,0x996ff00e,0x08e10b96,0x00000000 
         long      0x3fff0000,0x9b773f95,0x12321da7,0x00000000 
         long      0x3fff0000,0x9d55cc32,0x0f935624,0x00000000 
         long      0x3fff0000,0x9f100575,0x006cc571,0x00000000 
         long      0x3fff0000,0xa0a9c290,0xd97cc06c,0x00000000 
         long      0x3fff0000,0xa22659eb,0xebc0630a,0x00000000 
         long      0x3fff0000,0xa388b4af,0xf6ef0ec9,0x00000000 
         long      0x3fff0000,0xa4d35f10,0x61d292c4,0x00000000 
         long      0x3fff0000,0xa60895dc,0xfbe3187e,0x00000000 
         long      0x3fff0000,0xa72a51dc,0x7367beac,0x00000000 
         long      0x3fff0000,0xa83a5153,0x0956168f,0x00000000 
         long      0x3fff0000,0xa93a2007,0x7539546e,0x00000000 
         long      0x3fff0000,0xaa9e7245,0x023b2605,0x00000000 
         long      0x3fff0000,0xac4c84ba,0x6fe4d58f,0x00000000 
         long      0x3fff0000,0xadce4a4a,0x606b9712,0x00000000 
         long      0x3fff0000,0xaf2a2dcd,0x8d263c9c,0x00000000 
         long      0x3fff0000,0xb0656f81,0xf22265c7,0x00000000 
         long      0x3fff0000,0xb1846515,0x0f71496a,0x00000000 
         long      0x3fff0000,0xb28aaa15,0x6f9ada35,0x00000000 
         long      0x3fff0000,0xb37b44ff,0x3766b895,0x00000000 
         long      0x3fff0000,0xb458c3dc,0xe9630433,0x00000000 
         long      0x3fff0000,0xb525529d,0x562246bd,0x00000000 
         long      0x3fff0000,0xb5e2cca9,0x5f9d88cc,0x00000000 
         long      0x3fff0000,0xb692cada,0x7aca1ada,0x00000000 
         long      0x3fff0000,0xb736aea7,0xa6925838,0x00000000 
         long      0x3fff0000,0xb7cfab28,0x7e9f7b36,0x00000000 
         long      0x3fff0000,0xb85ecc66,0xcb219835,0x00000000 
         long      0x3fff0000,0xb8e4fd5a,0x20a593da,0x00000000 
         long      0x3fff0000,0xb99f41f6,0x4aff9bb5,0x00000000 
         long      0x3fff0000,0xba7f1e17,0x842bbe7b,0x00000000 
         long      0x3fff0000,0xbb471285,0x7637e17d,0x00000000 
         long      0x3fff0000,0xbbfabe8a,0x4788df6f,0x00000000 
         long      0x3fff0000,0xbc9d0fad,0x2b689d79,0x00000000 
         long      0x3fff0000,0xbd306a39,0x471ecd86,0x00000000 
         long      0x3fff0000,0xbdb6c731,0x856af18a,0x00000000 
         long      0x3fff0000,0xbe31cac5,0x02e80d70,0x00000000 
         long      0x3fff0000,0xbea2d55c,0xe33194e2,0x00000000 
         long      0x3fff0000,0xbf0b10b7,0xc03128f0,0x00000000 
         long      0x3fff0000,0xbf6b7a18,0xdacb778d,0x00000000 
         long      0x3fff0000,0xbfc4ea46,0x63fa18f6,0x00000000 
         long      0x3fff0000,0xc0181bde,0x8b89a454,0x00000000 
         long      0x3fff0000,0xc065b066,0xcfbf6439,0x00000000 
         long      0x3fff0000,0xc0ae345f,0x56340ae6,0x00000000 
         long      0x3fff0000,0xc0f22291,0x9cb9e6a7,0x00000000 
                                    
         set       satans_x,fp_scr1        
         set       satans_xdcare,satans_x+2       
         set       satans_xfrac,satans_x+4        
         set       satans_xfraclo,satans_x+8      
                                    
         set       satans_atanf,fp_scr2    
         set       satans_atanfhi,satans_atanf+4  
         set       satans_atanflo,satans_atanf+8  
                                    
                                    
                                    
                                    
                                    
satand:                             
#--ENTRY POINT FOR ATAN(X) FOR DENORMALIZED ARGUMENT
                                    
         bra.l     t_extdnrm        
                                    
satan:                              
#--ENTRY POINT FOR ATAN(X), HERE X IS FINITE, NON-ZERO, AND NOT NAN'S
                                    
         fmove.x   (%a0),%fp0       # LOAD INPUT
                                    
         move.l    (%a0),%d0        
         move.w    4(%a0),%d0       
         fmove.x   %fp0,satans_x(%a6)      
         andi.l    &0x7fffffff,%d0  
                                    
         cmpi.l    %d0,&0x3ffb8000  # |X| >= 1/16?
         bge.b     satans_atanok1          
         bra.w     satans_atansm           
                                    
satans_atanok1:                            
         cmpi.l    %d0,&0x4002ffff  # |X| < 16 ?
         ble.b     satans_atanmain         
         bra.w     satans_atanbig          
                                    
                                    
#--THE MOST LIKELY CASE, |X| IN [1/16, 16). WE USE TABLE TECHNIQUE
#--THE IDEA IS ATAN(X) = ATAN(F) + ATAN( [X-F] / [1+XF] ).
#--SO IF F IS CHOSEN TO BE CLOSE TO X AND ATAN(F) IS STORED IN
#--A TABLE, ALL WE NEED IS TO APPROXIMATE ATAN(U) WHERE
#--U = (X-F)/(1+XF) IS SMALL (REMEMBER F IS CLOSE TO X). IT IS
#--TRUE THAT A DIVIDE IS NOW NEEDED, BUT THE APPROXIMATION FOR
#--ATAN(U) IS A VERY SHORT POLYNOMIAL AND THE INDEXING TO
#--FETCH F AND SAVING OF REGISTERS CAN BE ALL HIDED UNDER THE
#--DIVIDE. IN THE END THIS METHOD IS MUCH FASTER THAN A TRADITIONAL
#--ONE. NOTE ALSO THAT THE TRADITIONAL SCHEME THAT APPROXIMATE
#--ATAN(X) DIRECTLY WILL NEED TO USE A RATIONAL APPROXIMATION
#--(DIVISION NEEDED) ANYWAY BECAUSE A POLYNOMIAL APPROXIMATION
#--WILL INVOLVE A VERY LONG POLYNOMIAL.
                                    
#--NOW WE SEE X AS +-2^K * 1.BBBBBBB....B <- 1. + 63 BITS
#--WE CHOSE F TO BE +-2^K * 1.BBBB1
#--THAT IS IT MATCHES THE EXPONENT AND FIRST 5 BITS OF X, THE
#--SIXTH BITS IS SET TO BE 1. SINCE K = -4, -3, ..., 3, THERE
#--ARE ONLY 8 TIMES 16 = 2^7 = 128 |F|'S. SINCE ATAN(-|F|) IS
#-- -ATAN(|F|), WE NEED TO STORE ONLY ATAN(|F|).
                                    
satans_atanmain:                            
                                    
         move.w    &0x0000,satans_xdcare(%a6) # CLEAN UP X JUST IN CASE
         andi.l    &0xf8000000,satans_xfrac(%a6) # FIRST 5 BITS
         ori.l     &0x04000000,satans_xfrac(%a6) # SET 6-TH BIT TO 1
         move.l    &0x00000000,satans_xfraclo(%a6) # LOCATION OF X IS NOW F
                                    
         fmove.x   %fp0,%fp1        # FP1 IS X
         fmul.x    satans_x(%a6),%fp1      # FP1 IS X*F, NOTE THAT X*F > 0
         fsub.x    satans_x(%a6),%fp0      # FP0 IS X-F
         fadd.s    &0f1.0,%fp1      # FP1 IS 1 + X*F
         fdiv.x    %fp1,%fp0        # FP0 IS U = (X-F)/(1+X*F)
                                    
#--WHILE THE DIVISION IS TAKING ITS TIME, WE FETCH ATAN(|F|)
#--CREATE ATAN(F) AND STORE IT IN ATANF, AND
#--SAVE REGISTERS FP2.
                                    
         move.l    %d2,-(%a7)       # SAVE d2 TEMPORARILY
         move.l    %d0,%d2          # THE EXPO AND 16 BITS OF X
         andi.l    &0x00007800,%d0  # 4 VARYING BITS OF F'S FRACTION
         andi.l    &0x7fff0000,%d2  # EXPONENT OF F
         subi.l    &0x3ffb0000,%d2  # K+4
         asr.l     &1,%d2           
         add.l     %d2,%d0          # THE 7 BITS IDENTIFYING F
         asr.l     &7,%d0           # INDEX INTO TBL OF ATAN(|F|)
ifdef(`PIC',`	lea	satans_atantbl(%pc),%a1
',`         lea       satans_atantbl,%a1      
')
         adda.l    %d0,%a1          # ADDRESS OF ATAN(|F|)
         move.l    (%a1)+,satans_atanf(%a6) 
         move.l    (%a1)+,satans_atanfhi(%a6) 
         move.l    (%a1)+,satans_atanflo(%a6) # ATANF IS NOW ATAN(|F|)
         move.l    satans_x(%a6),%d0       # LOAD SIGN AND EXPO. AGAIN
         andi.l    &0x80000000,%d0  # SIGN(F)
         or.l      %d0,satans_atanf(%a6)   # ATANF IS NOW SIGN(F)*ATAN(|F|)
         move.l    (%a7)+,%d2       # RESTORE d2
                                    
#--THAT'S ALL I HAVE TO DO FOR NOW,
#--BUT ALAS, THE DIVIDE IS STILL CRANKING!
                                    
#--U IN FP0, WE ARE NOW READY TO COMPUTE ATAN(U) AS
#--U + A1*U*V*(A2 + V*(A3 + V)), V = U*U
#--THE POLYNOMIAL MAY LOOK STRANGE, BUT IS NEVERTHELESS CORRECT.
#--THE NATURAL FORM IS U + U*V*(A1 + V*(A2 + V*A3))
#--WHAT WE HAVE HERE IS MERELY	A1 = A3, A2 = A1/A3, A3 = A2/A3.
#--THE REASON FOR THIS REARRANGEMENT IS TO MAKE THE INDEPENDENT
#--PARTS A1*U*V AND (A2 + ... STUFF) MORE LOAD-BALANCED
                                    
                                    
         fmove.x   %fp0,%fp1        
         fmul.x    %fp1,%fp1        
ifdef(`PIC',`	fmove.d	&0xbff6687e314987d8,%fp2
',`         fmove.d   satans_atana3,%fp2      
')
         fadd.x    %fp1,%fp2        # A3+V
         fmul.x    %fp1,%fp2        # V*(A3+V)
         fmul.x    %fp0,%fp1        # U*V
ifdef(`PIC',`	fadd.d	&0x4002ac6934a26db3,%fp2
',`         fadd.d    satans_atana2,%fp2      # A2+V*(A3+V)
')
ifdef(`PIC',`	fmul.d	&0xbfc2476f4e1da28e,%fp1
',`         fmul.d    satans_atana1,%fp1      # A1*U*V
')
         fmul.x    %fp2,%fp1        # A1*U*V*(A2+V*(A3+V))
                                    
         fadd.x    %fp1,%fp0        # ATAN(U), FP1 RELEASED
         fmove.l   %d1,%fpcr        # restore users exceptions
         fadd.x    satans_atanf(%a6),%fp0  # ATAN(X)
         bra.l     t_frcinx         
                                    
satans_atanbors:                            
#--|X| IS IN d0 IN COMPACT FORM. FP1, d0 SAVED.
#--FP0 IS X AND |X| <= 1/16 OR |X| >= 16.
         cmpi.l    %d0,&0x3fff8000  
         bgt.w     satans_atanbig          # I.E. |X| >= 16
                                    
satans_atansm:                             
#--|X| <= 1/16
#--IF |X| < 2^(-40), RETURN X AS ANSWER. OTHERWISE, APPROXIMATE
#--ATAN(X) BY X + X*Y*(B1+Y*(B2+Y*(B3+Y*(B4+Y*(B5+Y*B6)))))
#--WHICH IS X + X*Y*( [B1+Z*(B3+Z*B5)] + [Y*(B2+Z*(B4+Z*B6)] )
#--WHERE Y = X*X, AND Z = Y*Y.
                                    
         cmpi.l    %d0,&0x3fd78000  
         blt.w     satans_atantiny         
#--COMPUTE POLYNOMIAL
         fmul.x    %fp0,%fp0        # FP0 IS Y = X*X
                                    
                                    
         move.w    &0x0000,satans_xdcare(%a6) 
                                    
         fmove.x   %fp0,%fp1        
         fmul.x    %fp1,%fp1        # FP1 IS Z = Y*Y
                                    
ifdef(`PIC',`	fmove.d	&0x3fb344447f876989,%fp2
',`         fmove.d   satans_atanb6,%fp2      
')
ifdef(`PIC',`	fmove.d	&0xbfb744ee7faf45db,%fp3
',`         fmove.d   satans_atanb5,%fp3      
')
                                    
         fmul.x    %fp1,%fp2        # Z*B6
         fmul.x    %fp1,%fp3        # Z*B5
                                    
ifdef(`PIC',`	fadd.d	&0x3fbc71c646940220,%fp2
',`         fadd.d    satans_atanb4,%fp2      # B4+Z*B6
')
ifdef(`PIC',`	fadd.d	&0xbfc24924921872f9,%fp3
',`         fadd.d    satans_atanb3,%fp3      # B3+Z*B5
')
                                    
         fmul.x    %fp1,%fp2        # Z*(B4+Z*B6)
         fmul.x    %fp3,%fp1        # Z*(B3+Z*B5)
                                    
ifdef(`PIC',`	fadd.d	&0x3fc9999999998fa9,%fp2
',`         fadd.d    satans_atanb2,%fp2      # B2+Z*(B4+Z*B6)
')
ifdef(`PIC',`	fadd.d	&0xbfd5555555555555,%fp1
',`         fadd.d    satans_atanb1,%fp1      # B1+Z*(B3+Z*B5)
')
                                    
         fmul.x    %fp0,%fp2        # Y*(B2+Z*(B4+Z*B6))
         fmul.x    satans_x(%a6),%fp0      # X*Y
                                    
         fadd.x    %fp2,%fp1        # [B1+Z*(B3+Z*B5)]+[Y*(B2+Z*(B4+Z*B6))]
                                    
                                    
         fmul.x    %fp1,%fp0        # X*Y*([B1+Z*(B3+Z*B5)]+[Y*(B2+Z*(B4+Z*B6))])
                                    
         fmove.l   %d1,%fpcr        # restore users exceptions
         fadd.x    satans_x(%a6),%fp0      
                                    
         bra.l     t_frcinx         
                                    
satans_atantiny:                            
#--|X| < 2^(-40), ATAN(X) = X
         move.w    &0x0000,satans_xdcare(%a6) 
                                    
         fmove.l   %d1,%fpcr        # restore users exceptions
         fmove.x   satans_x(%a6),%fp0      # last inst - possible exception set
                                    
         bra.l     t_frcinx         
                                    
satans_atanbig:                            
#--IF |X| > 2^(100), RETURN	SIGN(X)*(PI/2 - TINY). OTHERWISE,
#--RETURN SIGN(X)*PI/2 + ATAN(-1/X).
         cmpi.l    %d0,&0x40638000  
         bgt.w     satans_atanhuge         
                                    
#--APPROXIMATE ATAN(-1/X) BY
#--X'+X'*Y*(C1+Y*(C2+Y*(C3+Y*(C4+Y*C5)))), X' = -1/X, Y = X'*X'
#--THIS CAN BE RE-WRITTEN AS
#--X'+X'*Y*( [C1+Z*(C3+Z*C5)] + [Y*(C2+Z*C4)] ), Z = Y*Y.
                                    
         fmove.s   &0f-1.0,%fp1     # LOAD -1
         fdiv.x    %fp0,%fp1        # FP1 IS -1/X
                                    
                                    
#--DIVIDE IS STILL CRANKING
                                    
         fmove.x   %fp1,%fp0        # FP0 IS X'
         fmul.x    %fp0,%fp0        # FP0 IS Y = X__hpasm_string1
         fmove.x   %fp1,satans_x(%a6)      # X IS REALLY X'
                                    
         fmove.x   %fp0,%fp1        
         fmul.x    %fp1,%fp1        # FP1 IS Z = Y*Y
                                    
ifdef(`PIC',`	fmove.d	&0xbfb70bf398539e6a,%fp3
',`         fmove.d   satans_atanc5,%fp3      
')
ifdef(`PIC',`	fmove.d	&0x3fbc7187962d1d7d,%fp2
',`         fmove.d   satans_atanc4,%fp2      
')
                                    
         fmul.x    %fp1,%fp3        # Z*C5
         fmul.x    %fp1,%fp2        # Z*B4
                                    
ifdef(`PIC',`	fadd.d	&0xbfc24924827107b8,%fp3
',`         fadd.d    satans_atanc3,%fp3      # C3+Z*C5
')
ifdef(`PIC',`	fadd.d	&0x3fc999999996263e,%fp2
',`         fadd.d    satans_atanc2,%fp2      # C2+Z*C4
')
                                    
         fmul.x    %fp3,%fp1        # Z*(C3+Z*C5), FP3 RELEASED
         fmul.x    %fp0,%fp2        # Y*(C2+Z*C4)
                                    
ifdef(`PIC',`	fadd.d	&0xbfd5555555555536,%fp1
',`         fadd.d    satans_atanc1,%fp1      # C1+Z*(C3+Z*C5)
')
         fmul.x    satans_x(%a6),%fp0      # X'*Y
                                    
         fadd.x    %fp2,%fp1        # [Y*(C2+Z*C4)]+[C1+Z*(C3+Z*C5)]
                                    
                                    
         fmul.x    %fp1,%fp0        # X'*Y*([B1+Z*(B3+Z*B5)]
#					...	+[Y*(B2+Z*(B4+Z*B6))])
         fadd.x    satans_x(%a6),%fp0      
                                    
         fmove.l   %d1,%fpcr        # restore users exceptions
                                    
         btst.b    &7,(%a0)         
         beq.b     satans_pos_big          
                                    
satans_neg_big:                            
ifdef(`PIC',`	fadd.x	&0xbfff0000c90fdaa22168c235,%fp0
',`         fadd.x    satans_npiby2,%fp0      
')
         bra.l     t_frcinx         
                                    
satans_pos_big:                            
ifdef(`PIC',`	fadd.x	&0x3fff0000c90fdaa22168c235,%fp0
',`         fadd.x    satans_ppiby2,%fp0      
')
         bra.l     t_frcinx         
                                    
satans_atanhuge:                            
#--RETURN SIGN(X)*(PIBY2 - TINY) = SIGN(X)*PIBY2 - SIGN(X)*TINY
         btst.b    &7,(%a0)         
         beq.b     satans_pos_huge         
                                    
satans_neg_huge:                            
ifdef(`PIC',`	fmove.x	&0xbfff0000c90fdaa22168c235,%fp0
',`         fmove.x   satans_npiby2,%fp0      
')
         fmove.l   %d1,%fpcr        
ifdef(`PIC',`	fsub.x	&0x800100008000000000000000,%fp0
',`         fsub.x    satans_ntiny,%fp0       
')
         bra.l     t_frcinx         
                                    
satans_pos_huge:                            
ifdef(`PIC',`	fmove.x	&0x3fff0000c90fdaa22168c235,%fp0
',`         fmove.x   satans_ppiby2,%fp0      
')
         fmove.l   %d1,%fpcr        
ifdef(`PIC',`	fsub.x	&0x000100008000000000000000,%fp0
',`         fsub.x    satans_ptiny,%fp0       
')
         bra.l     t_frcinx         
                                    
                                    
	version 3
