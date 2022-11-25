ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /misc/source_product/9.10/commands.rcs/lib/libm/fp040_log.s,v $
# $Revision: 70.3 $      $Author: ssa $
# $State: Exp $         $Locker:  $
# $Date: 91/08/30 00:13:44 $
                                    
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
define(FILE_log)


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
# $Source: /misc/source_product/9.10/commands.rcs/lib/libm/fp040_log.s,v $
# $Revision: 70.3 $      $Author: ssa $
# $State: Exp $         $Locker:  $
# $Date: 91/08/30 00:13:44 $
                                    
#
#	slogn.sa 3.1 12/10/90
#
#	slogn computes the natural logarithm of an
#	input value. slognd does the same except the input value is a
#	denormalized number. slognp1 computes log(1+X), and slognp1d
#	computes log(1+X) for denormalized X.
#
#	Input: Double-extended value in memory location pointed to by address
#		register a0.
#
#	Output:	log(X) or log(1+X) returned in floating-point register Fp0.
#
#	Accuracy and Monotonicity: The returned result is within 2 ulps in
#		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
#		result is subsequently rounded to double precision. The 
#		result is provably monotonic in double precision.
#
#	Speed: The program slogn takes approximately 190 cycles for input 
#		argument X such that |X-1| >= 1/16, which is the the usual 
#		situation. For those arguments, slognp1 takes approximately
#		 210 cycles. For the less common arguments, the program will
#		 run no worse than 10% slower.
#
#	Algorithm:
#	LOGN:
#	Step 1. If |X-1| < 1/16, approximate log(X) by an odd polynomial in
#		u, where u = 2(X-1)/(X+1). Otherwise, move on to Step 2.
#
#	Step 2. X = 2**k * Y where 1 <= Y < 2. Define F to be the first seven
#		significant bits of Y plus 2**(-7), i.e. F = 1.xxxxxx1 in base
#		2 where the six "x" match those of Y. Note that |Y-F| <= 2**(-7).
#
#	Step 3. Define u = (Y-F)/F. Approximate log(1+u) by a polynomial in u,
#		log(1+u) = poly.
#
#	Step 4. Reconstruct log(X) = log( 2**k * Y ) = k*log(2) + log(F) + log(1+u)
#		by k*log(2) + (log(F) + poly). The values of log(F) are calculated
#		beforehand and stored in the program.
#
#	lognp1:
#	Step 1: If |X| < 1/16, approximate log(1+X) by an odd polynomial in
#		u where u = 2X/(2+X). Otherwise, move on to Step 2.
#
#	Step 2: Let 1+X = 2**k * Y, where 1 <= Y < 2. Define F as done in Step 2
#		of the algorithm for LOGN and compute log(1+X) as
#		k*log(2) + log(F) + poly where poly approximates log(1+u),
#		u = (Y-F)/F. 
#
#	Implementation Notes:
#	Note 1. There are 64 different possible values for F, thus 64 log(F)'s
#		need to be tabulated. Moreover, the values of 1/F are also 
#		tabulated so that the division in (Y-F)/F can be performed by a
#		multiplication.
#
#	Note 2. In Step 2 of lognp1, in order to preserved accuracy, the value
#		Y-F has to be calculated carefully when 1/2 <= X < 3/2. 
#
#	Note 3. To fully exploit the pipeline, polynomials are usually separated
#		into slogns_two parts evaluated independently before being added up.
#	
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
	include(fpsp.h.s)
                                    
slogns_bounds1: long      0x3ffef07d,0x3fff8841 
slogns_bounds2: long      0x3ffe8000,0x3fffc000 
                                    
slogns_logof2:  long      0x3ffe0000,0xb17217f7,0xd1cf79ac,0x00000000 
                                    
slogns_one:     long      0x3f800000       
slogns_zero:    long      0x00000000       
slogns_infty:   long      0x7f800000       
slogns_negone:  long      0xbf800000       
                                    
slogns_loga6:   long      0x3fc2499a,0xb5e4040b 
slogns_loga5:   long      0xbfc555b5,0x848cb7db 
                                    
slogns_loga4:   long      0x3fc99999,0x987d8730 
slogns_loga3:   long      0xbfcfffff,0xff6f7e97 
                                    
slogns_loga2:   long      0x3fd55555,0x555555a4 
slogns_loga1:   long      0xbfe00000,0x00000008 
                                    
slogns_logb5:   long      0x3f175496,0xadd7dad6 
slogns_logb4:   long      0x3f3c71c2,0xfe80c7e0 
                                    
slogns_logb3:   long      0x3f624924,0x928bccff 
slogns_logb2:   long      0x3f899999,0x999995ec 
                                    
slogns_logb1:   long      0x3fb55555,0x55555555 
slogns_two:     long      0x40000000,0x00000000 
                                    
slogns_lthold:  long      0x3f990000,0x80000000,0x00000000,0x00000000 
                                    
slogns_logtbl:                             
         long      0x3ffe0000,0xfe03f80f,0xe03f80fe,0x00000000 
         long      0x3ff70000,0xff015358,0x833c47e2,0x00000000 
         long      0x3ffe0000,0xfa232cf2,0x52138ac0,0x00000000 
         long      0x3ff90000,0xbdc8d83e,0xad88d549,0x00000000 
         long      0x3ffe0000,0xf6603d98,0x0f6603da,0x00000000 
         long      0x3ffa0000,0x9cf43dcf,0xf5eafd48,0x00000000 
         long      0x3ffe0000,0xf2b9d648,0x0f2b9d65,0x00000000 
         long      0x3ffa0000,0xda16eb88,0xcb8df614,0x00000000 
         long      0x3ffe0000,0xef2eb71f,0xc4345238,0x00000000 
         long      0x3ffb0000,0x8b29b775,0x1bd70743,0x00000000 
         long      0x3ffe0000,0xebbdb2a5,0xc1619c8c,0x00000000 
         long      0x3ffb0000,0xa8d839f8,0x30c1fb49,0x00000000 
         long      0x3ffe0000,0xe865ac7b,0x7603a197,0x00000000 
         long      0x3ffb0000,0xc61a2eb1,0x8cd907ad,0x00000000 
         long      0x3ffe0000,0xe525982a,0xf70c880e,0x00000000 
         long      0x3ffb0000,0xe2f2a47a,0xde3a18af,0x00000000 
         long      0x3ffe0000,0xe1fc780e,0x1fc780e2,0x00000000 
         long      0x3ffb0000,0xff64898e,0xdf55d551,0x00000000 
         long      0x3ffe0000,0xdee95c4c,0xa037ba57,0x00000000 
         long      0x3ffc0000,0x8db956a9,0x7b3d0148,0x00000000 
         long      0x3ffe0000,0xdbeb61ee,0xd19c5958,0x00000000 
         long      0x3ffc0000,0x9b8fe100,0xf47ba1de,0x00000000 
         long      0x3ffe0000,0xd901b203,0x6406c80e,0x00000000 
         long      0x3ffc0000,0xa9372f1d,0x0da1bd17,0x00000000 
         long      0x3ffe0000,0xd62b80d6,0x2b80d62c,0x00000000 
         long      0x3ffc0000,0xb6b07f38,0xce90e46b,0x00000000 
         long      0x3ffe0000,0xd3680d36,0x80d3680d,0x00000000 
         long      0x3ffc0000,0xc3fd0329,0x06488481,0x00000000 
         long      0x3ffe0000,0xd0b69fcb,0xd2580d0b,0x00000000 
         long      0x3ffc0000,0xd11de0ff,0x15ab18ca,0x00000000 
         long      0x3ffe0000,0xce168a77,0x25080ce1,0x00000000 
         long      0x3ffc0000,0xde1433a1,0x6c66b150,0x00000000 
         long      0x3ffe0000,0xcb8727c0,0x65c393e0,0x00000000 
         long      0x3ffc0000,0xeae10b5a,0x7ddc8add,0x00000000 
         long      0x3ffe0000,0xc907da4e,0x871146ad,0x00000000 
         long      0x3ffc0000,0xf7856e5e,0xe2c9b291,0x00000000 
         long      0x3ffe0000,0xc6980c69,0x80c6980c,0x00000000 
         long      0x3ffd0000,0x82012ca5,0xa68206d7,0x00000000 
         long      0x3ffe0000,0xc4372f85,0x5d824ca6,0x00000000 
         long      0x3ffd0000,0x882c5fcd,0x7256a8c5,0x00000000 
         long      0x3ffe0000,0xc1e4bbd5,0x95f6e947,0x00000000 
         long      0x3ffd0000,0x8e44c60b,0x4ccfd7de,0x00000000 
         long      0x3ffe0000,0xbfa02fe8,0x0bfa02ff,0x00000000 
         long      0x3ffd0000,0x944ad09e,0xf4351af6,0x00000000 
         long      0x3ffe0000,0xbd691047,0x07661aa3,0x00000000 
         long      0x3ffd0000,0x9a3eecd4,0xc3eaa6b2,0x00000000 
         long      0x3ffe0000,0xbb3ee721,0xa54d880c,0x00000000 
         long      0x3ffd0000,0xa0218434,0x353f1de8,0x00000000 
         long      0x3ffe0000,0xb92143fa,0x36f5e02e,0x00000000 
         long      0x3ffd0000,0xa5f2fcab,0xbbc506da,0x00000000 
         long      0x3ffe0000,0xb70fbb5a,0x19be3659,0x00000000 
         long      0x3ffd0000,0xabb3b8ba,0x2ad362a5,0x00000000 
         long      0x3ffe0000,0xb509e68a,0x9b94821f,0x00000000 
         long      0x3ffd0000,0xb1641795,0xce3ca97b,0x00000000 
         long      0x3ffe0000,0xb30f6352,0x8917c80b,0x00000000 
         long      0x3ffd0000,0xb7047551,0x5d0f1c61,0x00000000 
         long      0x3ffe0000,0xb11fd3b8,0x0b11fd3c,0x00000000 
         long      0x3ffd0000,0xbc952afe,0xea3d13e1,0x00000000 
         long      0x3ffe0000,0xaf3addc6,0x80af3ade,0x00000000 
         long      0x3ffd0000,0xc2168ed0,0xf458ba4a,0x00000000 
         long      0x3ffe0000,0xad602b58,0x0ad602b6,0x00000000 
         long      0x3ffd0000,0xc788f439,0xb3163bf1,0x00000000 
         long      0x3ffe0000,0xab8f69e2,0x8359cd11,0x00000000 
         long      0x3ffd0000,0xccecac08,0xbf04565d,0x00000000 
         long      0x3ffe0000,0xa9c84a47,0xa07f5638,0x00000000 
         long      0x3ffd0000,0xd2420487,0x2dd85160,0x00000000 
         long      0x3ffe0000,0xa80a80a8,0x0a80a80b,0x00000000 
         long      0x3ffd0000,0xd7894992,0x3bc3588a,0x00000000 
         long      0x3ffe0000,0xa655c439,0x2d7b73a8,0x00000000 
         long      0x3ffd0000,0xdcc2c4b4,0x9887dacc,0x00000000 
         long      0x3ffe0000,0xa4a9cf1d,0x96833751,0x00000000 
         long      0x3ffd0000,0xe1eebd3e,0x6d6a6b9e,0x00000000 
         long      0x3ffe0000,0xa3065e3f,0xae7cd0e0,0x00000000 
         long      0x3ffd0000,0xe70d785c,0x2f9f5bdc,0x00000000 
         long      0x3ffe0000,0xa16b312e,0xa8fc377d,0x00000000 
         long      0x3ffd0000,0xec1f392c,0x5179f283,0x00000000 
         long      0x3ffe0000,0x9fd809fd,0x809fd80a,0x00000000 
         long      0x3ffd0000,0xf12440d3,0xe36130e6,0x00000000 
         long      0x3ffe0000,0x9e4cad23,0xdd5f3a20,0x00000000 
         long      0x3ffd0000,0xf61cce92,0x346600bb,0x00000000 
         long      0x3ffe0000,0x9cc8e160,0xc3fb19b9,0x00000000 
         long      0x3ffd0000,0xfb091fd3,0x8145630a,0x00000000 
         long      0x3ffe0000,0x9b4c6f9e,0xf03a3caa,0x00000000 
         long      0x3ffd0000,0xffe97042,0xbfa4c2ad,0x00000000 
         long      0x3ffe0000,0x99d722da,0xbde58f06,0x00000000 
         long      0x3ffe0000,0x825efced,0x49369330,0x00000000 
         long      0x3ffe0000,0x9868c809,0x868c8098,0x00000000 
         long      0x3ffe0000,0x84c37a7a,0xb9a905c9,0x00000000 
         long      0x3ffe0000,0x97012e02,0x5c04b809,0x00000000 
         long      0x3ffe0000,0x87224c2e,0x8e645fb7,0x00000000 
         long      0x3ffe0000,0x95a02568,0x095a0257,0x00000000 
         long      0x3ffe0000,0x897b8cac,0x9f7de298,0x00000000 
         long      0x3ffe0000,0x94458094,0x45809446,0x00000000 
         long      0x3ffe0000,0x8bcf55de,0xc4cd05fe,0x00000000 
         long      0x3ffe0000,0x92f11384,0x0497889c,0x00000000 
         long      0x3ffe0000,0x8e1dc0fb,0x89e125e5,0x00000000 
         long      0x3ffe0000,0x91a2b3c4,0xd5e6f809,0x00000000 
         long      0x3ffe0000,0x9066e68c,0x955b6c9b,0x00000000 
         long      0x3ffe0000,0x905a3863,0x3e06c43b,0x00000000 
         long      0x3ffe0000,0x92aade74,0xc7be59e0,0x00000000 
         long      0x3ffe0000,0x8f1779d9,0xfdc3a219,0x00000000 
         long      0x3ffe0000,0x94e9bff6,0x15845643,0x00000000 
         long      0x3ffe0000,0x8dda5202,0x37694809,0x00000000 
         long      0x3ffe0000,0x9723a1b7,0x20134203,0x00000000 
         long      0x3ffe0000,0x8ca29c04,0x6514e023,0x00000000 
         long      0x3ffe0000,0x995899c8,0x90eb8990,0x00000000 
         long      0x3ffe0000,0x8b70344a,0x139bc75a,0x00000000 
         long      0x3ffe0000,0x9b88bdaa,0x3a3dae2f,0x00000000 
         long      0x3ffe0000,0x8a42f870,0x5669db46,0x00000000 
         long      0x3ffe0000,0x9db4224f,0xffe1157c,0x00000000 
         long      0x3ffe0000,0x891ac73a,0xe9819b50,0x00000000 
         long      0x3ffe0000,0x9fdadc26,0x8b7a12da,0x00000000 
         long      0x3ffe0000,0x87f78087,0xf78087f8,0x00000000 
         long      0x3ffe0000,0xa1fcff17,0xce733bd4,0x00000000 
         long      0x3ffe0000,0x86d90544,0x7a34acc6,0x00000000 
         long      0x3ffe0000,0xa41a9e8f,0x5446fb9f,0x00000000 
         long      0x3ffe0000,0x85bf3761,0x2cee3c9b,0x00000000 
         long      0x3ffe0000,0xa633cd7e,0x6771cd8b,0x00000000 
         long      0x3ffe0000,0x84a9f9c8,0x084a9f9d,0x00000000 
         long      0x3ffe0000,0xa8489e60,0x0b435a5e,0x00000000 
         long      0x3ffe0000,0x83993052,0x3fbe3368,0x00000000 
         long      0x3ffe0000,0xaa59233c,0xcca4bd49,0x00000000 
         long      0x3ffe0000,0x828cbfbe,0xb9a020a3,0x00000000 
         long      0x3ffe0000,0xac656dae,0x6bcc4985,0x00000000 
         long      0x3ffe0000,0x81848da8,0xfaf0d277,0x00000000 
         long      0x3ffe0000,0xae6d8ee3,0x60bb2468,0x00000000 
         long      0x3ffe0000,0x80808080,0x80808081,0x00000000 
         long      0x3ffe0000,0xb07197a2,0x3c46c654,0x00000000 
                                    
         set       slogns_adjk,l_scr1      
                                    
         set       slogns_x,fp_scr1        
         set       slogns_xdcare,slogns_x+2       
         set       slogns_xfrac,slogns_x+4        
                                    
         set       slogns_f,fp_scr2        
         set       slogns_ffrac,slogns_f+4        
                                    
         set       slogns_klog2,fp_scr3    
                                    
         set       slogns_saveu,fp_scr4    
                                    
                                    
                                    
                                    
                                    
                                    
slognd:                             
#--ENTRY POINT FOR LOG(X) FOR DENORMALIZED INPUT
                                    
         move.l    &-100,slogns_adjk(%a6)  # INPUT = 2^(ADJK) * FP0
                                    
#----normalize the input value by left shifting k bits (k to be determined
#----below), adjusting exponent and storing -k to  ADJK
#----the value TWOTO100 is no longer needed.
#----Note that this code assumes the denormalized input is NON-ZERO.
                                    
         movem.l   %d2-%d7,-(%a7)   # save some registers 
         move.l    &0x00000000,%d3  # D3 is exponent of smallest norm. #
         move.l    4(%a0),%d4       
         move.l    8(%a0),%d5       # (D4,D5) is (Hi_X,Lo_X)
         clr.l     %d2              # D2 used for holding K
                                    
         tst.l     %d4              
         bne.b     slogns_hix_not0         
                                    
slogns_hix_0:                              
         move.l    %d5,%d4          
         clr.l     %d5              
         move.l    &32,%d2          
         clr.l     %d6              
         bfffo     %d4{&0:&32},%d6  
         lsl.l     %d6,%d4          
         add.l     %d6,%d2          # (D3,D4,D5) is normalized
                                    
         move.l    %d3,slogns_x(%a6)       
         move.l    %d4,slogns_xfrac(%a6)   
         move.l    %d5,slogns_xfrac+4(%a6) 
         neg.l     %d2              
         move.l    %d2,slogns_adjk(%a6)    
         fmove.x   slogns_x(%a6),%fp0      
         movem.l   (%a7)+,%d2-%d7   # restore registers
         lea       slogns_x(%a6),%a0       
         bra.b     slogns_logbgn           # begin regular log(X)
                                    
                                    
slogns_hix_not0:                            
         clr.l     %d6              
         bfffo     %d4{&0:&32},%d6  # find first 1
         move.l    %d6,%d2          # get k
         lsl.l     %d6,%d4          
         move.l    %d5,%d7          # a copy of D5
         lsl.l     %d6,%d5          
         neg.l     %d6              
         addi.l    &32,%d6          
         lsr.l     %d6,%d7          
         or.l      %d7,%d4          # (D3,D4,D5) normalized
                                    
         move.l    %d3,slogns_x(%a6)       
         move.l    %d4,slogns_xfrac(%a6)   
         move.l    %d5,slogns_xfrac+4(%a6) 
         neg.l     %d2              
         move.l    %d2,slogns_adjk(%a6)    
         fmove.x   slogns_x(%a6),%fp0      
         movem.l   (%a7)+,%d2-%d7   # restore registers
         lea       slogns_x(%a6),%a0       
         bra.b     slogns_logbgn           # begin regular log(X)
                                    
                                    
slogn:                              
#--ENTRY POINT FOR LOG(X) FOR X FINITE, NON-ZERO, NOT NAN'S
                                    
         fmove.x   (%a0),%fp0       # LOAD INPUT
         move.l    &0x00000000,slogns_adjk(%a6) 
                                    
slogns_logbgn:                             
#--FPCR SAVED AND CLEARED, INPUT IS 2^(ADJK)*FP0, FP0 CONTAINS
#--A FINITE, NON-ZERO, NORMALIZED NUMBER.
                                    
         move.l    (%a0),%d0        
         move.w    4(%a0),%d0       
                                    
         move.l    (%a0),slogns_x(%a6)     
         move.l    4(%a0),slogns_x+4(%a6)  
         move.l    8(%a0),slogns_x+8(%a6)  
                                    
         cmpi.l    %d0,&0           # CHECK IF X IS NEGATIVE
         blt.w     slogns_logneg           # LOG OF NEGATIVE ARGUMENT IS INVALID
ifdef(`PIC',`	cmp2.l	%d0,slogns_bounds1(%pc)
',`         cmp2.l    %d0,slogns_bounds1      # X IS POSITIVE, CHECK IF X IS NEAR 1
')
         bcc.w     slogns_lognear1         # BOUNDS IS ROUGHLY [15/16, 17/16]
                                    
slogns_logmain:                            
#--THIS SHOULD BE THE USUAL CASE, X NOT VERY CLOSE TO 1
                                    
#--X = 2^(K) * Y, 1 <= Y < 2. THUS, Y = 1.XXXXXXXX....XX IN BINARY.
#--WE DEFINE F = 1.XXXXXX1, I.E. FIRST 7 BITS OF Y AND ATTACH A 1.
#--THE IDEA IS THAT LOG(X) = K*LOG2 + LOG(Y)
#--			 = K*LOG2 + LOG(F) + LOG(1 + (Y-F)/F).
#--NOTE THAT U = (Y-F)/F IS VERY SMALL AND THUS APPROXIMATING
#--LOG(1+U) CAN BE VERY EFFICIENT.
#--ALSO NOTE THAT THE VALUE 1/F IS STORED IN A TABLE SO THAT NO
#--DIVISION IS NEEDED TO CALCULATE (Y-F)/F. 
                                    
#--GET K, Y, F, AND ADDRESS OF 1/F.
         asr.l     &8,%d0           
         asr.l     &8,%d0           # SHIFTED 16 BITS, BIASED EXPO. OF X
         subi.l    &0x3fff,%d0      # THIS IS K
         add.l     slogns_adjk(%a6),%d0    # ADJUST K, ORIGINAL INPUT MAY BE  DENORM.
ifdef(`PIC',`	lea	slogns_logtbl(%pc),%a0
',`         lea       slogns_logtbl,%a0       # BASE ADDRESS OF 1/F AND LOG(F)
')
         fmove.l   %d0,%fp1         # CONVERT K TO FLOATING-POINT FORMAT
                                    
#--WHILE THE CONVERSION IS GOING ON, WE GET F AND ADDRESS OF 1/F
         move.l    &0x3fff0000,slogns_x(%a6) # X IS NOW Y, I.E. 2^(-K)*X
         move.l    slogns_xfrac(%a6),slogns_ffrac(%a6) 
         andi.l    &0xfe000000,slogns_ffrac(%a6) # FIRST 7 BITS OF Y
         ori.l     &0x01000000,slogns_ffrac(%a6) # GET F: ATTACH A 1 AT THE EIGHTH BIT
         move.l    slogns_ffrac(%a6),%d0   # READY TO GET ADDRESS OF 1/F
         andi.l    &0x7e000000,%d0  
         asr.l     &8,%d0           
         asr.l     &8,%d0           
         asr.l     &4,%d0           # SHIFTED 20, D0 IS THE DISPLACEMENT
         adda.l    %d0,%a0          # A0 IS THE ADDRESS FOR 1/F
                                    
         fmove.x   slogns_x(%a6),%fp0      
         move.l    &0x3fff0000,slogns_f(%a6) 
         clr.l     slogns_f+8(%a6)         
         fsub.x    slogns_f(%a6),%fp0      # Y-F
         fmovem.x  %fp2/%fp3,-(%sp) # SAVE FP2 WHILE FP0 IS NOT READY
#--SUMMARY: FP0 IS Y-F, A0 IS ADDRESS OF 1/F, FP1 IS K
#--REGISTERS SAVED: FPCR, FP1, FP2
                                    
slogns_lp1cont1:                            
#--AN RE-ENTRY POINT FOR LOGNP1
         fmul.x    (%a0),%fp0       # FP0 IS U = (Y-F)/F
ifdef(`PIC',`	fmul.x	&0x3ffe0000b17217f7d1cf79ac,%fp1
',`         fmul.x    slogns_logof2,%fp1      # GET K*LOG2 WHILE FP0 IS NOT READY
')
         fmove.x   %fp0,%fp2        
         fmul.x    %fp2,%fp2        # FP2 IS V=U*U
         fmove.x   %fp1,slogns_klog2(%a6)  # PUT K*LOG2 IN MEMEORY, FREE FP1
                                    
#--LOG(1+U) IS APPROXIMATED BY
#--U + V*(A1+U*(A2+U*(A3+U*(A4+U*(A5+U*A6))))) WHICH IS
#--[U + V*(A1+V*(A3+V*A5))]  +  [U*V*(A2+V*(A4+V*A6))]
                                    
         fmove.x   %fp2,%fp3        
         fmove.x   %fp2,%fp1        
                                    
ifdef(`PIC',`	fmul.d	&0x3fc2499ab5e4040b,%fp1
',`         fmul.d    slogns_loga6,%fp1       # V*A6
')
ifdef(`PIC',`	fmul.d	&0xbfc555b5848cb7db,%fp2
',`         fmul.d    slogns_loga5,%fp2       # V*A5
')
                                    
ifdef(`PIC',`	fadd.d	&0x3fc99999987d8730,%fp1
',`         fadd.d    slogns_loga4,%fp1       # A4+V*A6
')
ifdef(`PIC',`	fadd.d	&0xbfcfffffff6f7e97,%fp2
',`         fadd.d    slogns_loga3,%fp2       # A3+V*A5
')
                                    
         fmul.x    %fp3,%fp1        # V*(A4+V*A6)
         fmul.x    %fp3,%fp2        # V*(A3+V*A5)
                                    
ifdef(`PIC',`	fadd.d	&0x3fd55555555555a4,%fp1
',`         fadd.d    slogns_loga2,%fp1       # A2+V*(A4+V*A6)
')
ifdef(`PIC',`	fadd.d	&0xbfe0000000000008,%fp2
',`         fadd.d    slogns_loga1,%fp2       # A1+V*(A3+V*A5)
')
                                    
         fmul.x    %fp3,%fp1        # V*(A2+V*(A4+V*A6))
         adda.l    &16,%a0          # ADDRESS OF LOG(F)
         fmul.x    %fp3,%fp2        # V*(A1+V*(A3+V*A5)), FP3 RELEASED
                                    
         fmul.x    %fp0,%fp1        # U*V*(A2+V*(A4+V*A6))
         fadd.x    %fp2,%fp0        # U+V*(A1+V*(A3+V*A5)), FP2 RELEASED
                                    
         fadd.x    (%a0),%fp1       # LOG(F)+U*V*(A2+V*(A4+V*A6))
         fmovem.x  (%sp)+,%fp2/%fp3 # RESTORE FP2
         fadd.x    %fp1,%fp0        # FP0 IS LOG(F) + LOG(1+U)
                                    
         fmove.l   %d1,%fpcr        
         fadd.x    slogns_klog2(%a6),%fp0  # FINAL ADD
         bra.l     t_frcinx         
                                    
                                    
slogns_lognear1:                            
#--REGISTERS SAVED: FPCR, FP1. FP0 CONTAINS THE INPUT.
         fmove.x   %fp0,%fp1        
ifdef(`PIC',`	fsub.s	&0x3f800000,%fp1
',`         fsub.s    slogns_one,%fp1         # FP1 IS X-1
')
ifdef(`PIC',`	fadd.s	&0x3f800000,%fp0
',`         fadd.s    slogns_one,%fp0         # FP0 IS X+1
')
         fadd.x    %fp1,%fp1        # FP1 IS 2(X-1)
#--LOG(X) = LOG(1+U/2)-LOG(1-U/2) WHICH IS AN ODD POLYNOMIAL
#--IN U, U = 2(X-1)/(X+1) = FP1/FP0
                                    
slogns_lp1cont2:                            
#--THIS IS AN RE-ENTRY POINT FOR LOGNP1
         fdiv.x    %fp0,%fp1        # FP1 IS U
         fmovem.x  %fp2/%fp3,-(%sp) # SAVE FP2
#--REGISTERS SAVED ARE NOW FPCR,FP1,FP2,FP3
#--LET V=U*U, W=V*V, CALCULATE
#--U + U*V*(B1 + V*(B2 + V*(B3 + V*(B4 + V*B5)))) BY
#--U + U*V*(  [B1 + W*(B3 + W*B5)]  +  [V*(B2 + W*B4)]  )
         fmove.x   %fp1,%fp0        
         fmul.x    %fp0,%fp0        # FP0 IS V
         fmove.x   %fp1,slogns_saveu(%a6)  # STORE U IN MEMORY, FREE FP1
         fmove.x   %fp0,%fp1        
         fmul.x    %fp1,%fp1        # FP1 IS W
                                    
ifdef(`PIC',`	fmove.d	&0x3f175496add7dad6,%fp3
',`         fmove.d   slogns_logb5,%fp3       
')
ifdef(`PIC',`	fmove.d	&0x3f3c71c2fe80c7e0,%fp2
',`         fmove.d   slogns_logb4,%fp2       
')
                                    
         fmul.x    %fp1,%fp3        # W*B5
         fmul.x    %fp1,%fp2        # W*B4
                                    
ifdef(`PIC',`	fadd.d	&0x3f624924928bccff,%fp3
',`         fadd.d    slogns_logb3,%fp3       # B3+W*B5
')
ifdef(`PIC',`	fadd.d	&0x3f899999999995ec,%fp2
',`         fadd.d    slogns_logb2,%fp2       # B2+W*B4
')
                                    
         fmul.x    %fp3,%fp1        # W*(B3+W*B5), FP3 RELEASED
                                    
         fmul.x    %fp0,%fp2        # V*(B2+W*B4)
                                    
ifdef(`PIC',`	fadd.d	&0x3fb5555555555555,%fp1
',`         fadd.d    slogns_logb1,%fp1       # B1+W*(B3+W*B5)
')
         fmul.x    slogns_saveu(%a6),%fp0  # FP0 IS U*V
                                    
         fadd.x    %fp2,%fp1        # B1+W*(B3+W*B5) + V*(B2+W*B4), FP2 RELEASED
         fmovem.x  (%sp)+,%fp2/%fp3 # FP2 RESTORED
                                    
         fmul.x    %fp1,%fp0        # U*V*( [B1+W*(B3+W*B5)] + [V*(B2+W*B4)] )
                                    
         fmove.l   %d1,%fpcr        
         fadd.x    slogns_saveu(%a6),%fp0  
         bra.l     t_frcinx         
         rts                        
                                    
slogns_logneg:                             
#--REGISTERS SAVED FPCR. LOG(-VE) IS INVALID
         bra.l     t_operr          
                                    
slognp1d:                            
#--ENTRY POINT FOR LOG(1+Z) FOR DENORMALIZED INPUT
# Simply return the denorm
                                    
         bra.l     t_extdnrm        
                                    
slognp1:                            
#--ENTRY POINT FOR LOG(1+X) FOR X FINITE, NON-ZERO, NOT NAN'S
                                    
         fmove.x   (%a0),%fp0       # LOAD INPUT
         fabs.x    %fp0             # test magnitude
ifdef(`PIC',`	fcmp.x	%fp0,&0x3f9900008000000000000000
',`         fcmp.x    %fp0,slogns_lthold      # compare with min threshold
')
         fbgt.w    slogns_lp1real          # if greater, continue
         fmove.l   &0,%fpsr         # clr N flag from compare
         fmove.l   %d1,%fpcr        
         fmove.x   (%a0),%fp0       # return signed argument
         bra.l     t_frcinx         
                                    
slogns_lp1real:                            
         fmove.x   (%a0),%fp0       # LOAD INPUT
         move.l    &0x00000000,slogns_adjk(%a6) 
         fmove.x   %fp0,%fp1        # FP1 IS INPUT Z
ifdef(`PIC',`	fadd.s	&0x3f800000,%fp0
',`         fadd.s    slogns_one,%fp0         # X := ROUND(1+Z)
')
         fmove.x   %fp0,slogns_x(%a6)      
         move.w    slogns_xfrac(%a6),slogns_xdcare(%a6) 
         move.l    slogns_x(%a6),%d0       
         cmpi.l    %d0,&0           
         ble.w     slogns_lp1neg0          # LOG OF ZERO OR -VE
ifdef(`PIC',`	cmp2.l	%d0,slogns_bounds2(%pc)
',`         cmp2.l    %d0,slogns_bounds2      
')
         bcs.w     slogns_logmain          # BOUNDS2 IS [1/2,3/2]
#--IF 1+Z > 3/2 OR 1+Z < 1/2, THEN X, WHICH IS ROUNDING 1+Z,
#--CONTAINS AT LEAST 63 BITS OF INFORMATION OF Z. IN THAT CASE,
#--SIMPLY INVOKE LOG(X) FOR LOG(1+Z).
                                    
slogns_lp1near1:                            
#--NEXT SEE IF EXP(-1/16) < X < EXP(1/16)
ifdef(`PIC',`	cmp2.l	%d0,slogns_bounds1(%pc)
',`         cmp2.l    %d0,slogns_bounds1      
')
         bcs.b     slogns_lp1care          
                                    
slogns_lp1one16:                            
#--EXP(-1/16) < X < EXP(1/16). LOG(1+Z) = LOG(1+U/2) - LOG(1-U/2)
#--WHERE U = 2Z/(2+Z) = 2Z/(1+X).
         fadd.x    %fp1,%fp1        # FP1 IS 2Z
ifdef(`PIC',`	fadd.s	&0x3f800000,%fp0
',`         fadd.s    slogns_one,%fp0         # FP0 IS 1+X
')
#--U = FP1/FP0
         bra.w     slogns_lp1cont2         
                                    
slogns_lp1care:                            
#--HERE WE USE THE USUAL TABLE DRIVEN APPROACH. CARE HAS TO BE
#--TAKEN BECAUSE 1+Z CAN HAVE 67 BITS OF INFORMATION AND WE MUST
#--PRESERVE ALL THE INFORMATION. BECAUSE 1+Z IS IN [1/2,3/2],
#--THERE ARE ONLY TWO CASES.
#--CASE 1: 1+Z < 1, THEN K = -1 AND Y-F = (2-F) + 2Z
#--CASE 2: 1+Z > 1, THEN K = 0  AND Y-F = (1-F) + Z
#--ON RETURNING TO LP1CONT1, WE MUST HAVE K IN FP1, ADDRESS OF
#--(1/F) IN A0, Y-F IN FP0, AND FP2 SAVED.
                                    
         move.l    slogns_xfrac(%a6),slogns_ffrac(%a6) 
         andi.l    &0xfe000000,slogns_ffrac(%a6) 
         ori.l     &0x01000000,slogns_ffrac(%a6) # F OBTAINED
         cmpi.l    %d0,&0x3fff8000  # SEE IF 1+Z > 1
         bge.b     slogns_kiszero          
                                    
slogns_kisneg1:                            
ifdef(`PIC',`	fmove.s	&0x40000000,%fp0
',`         fmove.s   slogns_two,%fp0         
')
         move.l    &0x3fff0000,slogns_f(%a6) 
         clr.l     slogns_f+8(%a6)         
         fsub.x    slogns_f(%a6),%fp0      # 2-F
         move.l    slogns_ffrac(%a6),%d0   
         andi.l    &0x7e000000,%d0  
         asr.l     &8,%d0           
         asr.l     &8,%d0           
         asr.l     &4,%d0           # D0 CONTAINS DISPLACEMENT FOR 1/F
         fadd.x    %fp1,%fp1        # GET 2Z
         fmovem.x  %fp2/%fp3,-(%sp) # SAVE FP2 
         fadd.x    %fp1,%fp0        # FP0 IS Y-F = (2-F)+2Z
ifdef(`PIC',`	lea	slogns_logtbl(%pc),%a0
',`         lea       slogns_logtbl,%a0       # A0 IS ADDRESS OF 1/F
')
         adda.l    %d0,%a0          
ifdef(`PIC',`	fmove.s	&0xbf800000,%fp1
',`         fmove.s   slogns_negone,%fp1      # FP1 IS K = -1
')
         bra.w     slogns_lp1cont1         
                                    
slogns_kiszero:                            
ifdef(`PIC',`	fmove.s	&0x3f800000,%fp0
',`         fmove.s   slogns_one,%fp0         
')
         move.l    &0x3fff0000,slogns_f(%a6) 
         clr.l     slogns_f+8(%a6)         
         fsub.x    slogns_f(%a6),%fp0      # 1-F
         move.l    slogns_ffrac(%a6),%d0   
         andi.l    &0x7e000000,%d0  
         asr.l     &8,%d0           
         asr.l     &8,%d0           
         asr.l     &4,%d0           
         fadd.x    %fp1,%fp0        # FP0 IS Y-F
         fmovem.x  %fp2/%fp3,-(%sp) # FP2 SAVED
ifdef(`PIC',`	lea	slogns_logtbl(%pc),%a0
',`         lea       slogns_logtbl,%a0       
')
         adda.l    %d0,%a0          # A0 IS ADDRESS OF 1/F
ifdef(`PIC',`	fmove.s	&0x00000000,%fp1
',`         fmove.s   slogns_zero,%fp1        # FP1 IS K = 0
')
         bra.w     slogns_lp1cont1         
                                    
slogns_lp1neg0:                            
#--FPCR SAVED. D0 IS X IN COMPACT FORM.
         cmpi.l    %d0,&0           
         blt.b     slogns_lp1neg           
slogns_lp1zero:                            
ifdef(`PIC',`	fmove.s	&0xbf800000,%fp0
',`         fmove.s   slogns_negone,%fp0      
')
                                    
         fmove.l   %d1,%fpcr        
         bra.l     t_dz             
                                    
slogns_lp1neg:                             
ifdef(`PIC',`	fmove.s	&0x00000000,%fp0
',`         fmove.s   slogns_zero,%fp0        
')
                                    
         fmove.l   %d1,%fpcr        
         bra.l     t_operr          
                                    
                                    
	version 3
ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /misc/source_product/9.10/commands.rcs/lib/libm/fp040_log.s,v $
# $Revision: 70.3 $      $Author: ssa $
# $State: Exp $         $Locker:  $
# $Date: 91/08/30 00:13:44 $
                                    
#
#	slog2.sa 3.1 12/10/90
#
#       The entry point slog10 computes the base-10 
#	logarithm of an input argument X.
#	slog10d does the same except the input value is a 
#	denormalized number.  
#	sLog2 and sLog2d are the base-2 analogues.
#
#       INPUT:	Double-extended value in memory location pointed to 
#		by address register a0.
#
#       OUTPUT: log_10(X) or log_2(X) returned in floating-point 
#		register fp0.
#
#       ACCURACY and MONOTONICITY: The returned result is within 1.7 
#		ulps in 64 significant bit, i.e. within 0.5003 ulp 
#		to 53 bits if the result is subsequently rounded 
#		to double precision. The result is provably monotonic 
#		in double precision.
#
#       SPEED:	Two timings are measured, both in the copy-back mode. 
#		The first one is measured when the function is invoked 
#		the first time (so the instructions and data are not 
#		in cache), and the second one is measured when the 
#		function is reinvoked at the same input argument.
#
#       ALGORITHM and IMPLEMENTATION NOTES:
#
#       slog10d:
#
#       Step 0.   If X < 0, create a NaN and raise the slog2s_invalid operation
#                 flag. Otherwise, save FPCR in D1; set slog2s_FpCR to default.
#       Notes:    Default means round-to-nearest mode, no floating-point
#                 traps, and precision control = double extended.
#
#       Step 1.   Call slognd to obtain Y = log(X), the natural log of X.
#       Notes:    Even if X is denormalized, log(X) is always normalized.
#
#       Step 2.   Compute log_10(X) = log(X) * (1/log(10)).
#            2.1  Restore the user FPCR
#            2.2  Return ans := Y * INV_L10.
#
#
#       slog10: 
#
#       Step 0.   If X < 0, create a NaN and raise the slog2s_invalid operation
#                 flag. Otherwise, save FPCR in D1; set slog2s_FpCR to default.
#       Notes:    Default means round-to-nearest mode, no floating-point
#                 traps, and precision control = double extended.
#
#       Step 1.   Call sLogN to obtain Y = log(X), the natural log of X.
#
#       Step 2.   Compute log_10(X) = log(X) * (1/log(10)).
#            2.1  Restore the user FPCR
#            2.2  Return ans := Y * INV_L10.
#
#
#       sLog2d:
#
#       Step 0.   If X < 0, create a NaN and raise the slog2s_invalid operation
#                 flag. Otherwise, save FPCR in D1; set slog2s_FpCR to default.
#       Notes:    Default means round-to-nearest mode, no floating-point
#                 traps, and precision control = double extended.
#
#       Step 1.   Call slognd to obtain Y = log(X), the natural log of X.
#       Notes:    Even if X is denormalized, log(X) is always normalized.
#
#       Step 2.   Compute log_10(X) = log(X) * (1/log(2)).
#            2.1  Restore the user FPCR
#            2.2  Return ans := Y * INV_L2.
#
#
#       sLog2:
#
#       Step 0.   If X < 0, create a NaN and raise the slog2s_invalid operation
#                 flag. Otherwise, save FPCR in D1; set slog2s_FpCR to default.
#       Notes:    Default means round-to-nearest mode, no floating-point
#                 traps, and precision control = double extended.
#
#       Step 1.   If X is not an integer power of two, i.e., X != 2^k,
#                 go to Step 3.
#
#       Step 2.   Return k.
#            2.1  Get integer k, X = 2^k.
#            2.2  Restore the user FPCR.
#            2.3  Return ans := convert-to-double-extended(k).
#
#       Step 3.   Call sLogN to obtain Y = log(X), the natural log of X.
#
#       Step 4.   Compute log_2(X) = log(X) * (1/log(2)).
#            4.1  Restore the user FPCR
#            4.2  Return ans := Y * INV_L2.
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
                                    
                                    
                                    
                                    
                                    
slog2s_inv_l10: long      0x3ffd0000,0xde5bd8a9,0x37287195,0x00000000 
                                    
slog2s_inv_l2:  long      0x3fff0000,0xb8aa3b29,0x5c17f0bc,0x00000000 
                                    
slog10d:                            
#--entry point for Log10(X), X is denormalized
         move.l    (%a0),%d0        
         blt.w     slog2s_invalid          
         move.l    %d1,-(%sp)       
         clr.l     %d1              
         bsr.l     slognd           # log(X), X denorm.
         fmove.l   (%sp)+,%fpcr     
ifdef(`PIC',`	fmul.x	&0x3ffd0000de5bd8a937287195,%fp0
',`         fmul.x    slog2s_inv_l10,%fp0     
')
         bra.l     t_frcinx         
                                    
slog10:                             
#--entry point for Log10(X), X is normalized
                                    
         move.l    (%a0),%d0        
         blt.w     slog2s_invalid          
         move.l    %d1,-(%sp)       
         clr.l     %d1              
         bsr.l     slogn            # log(X), X normal.
         fmove.l   (%sp)+,%fpcr     
ifdef(`PIC',`	fmul.x	&0x3ffd0000de5bd8a937287195,%fp0
',`         fmul.x    slog2s_inv_l10,%fp0     
')
         bra.l     t_frcinx         
                                    
                                    
slog2d:                             
#--entry point for Log2(X), X is denormalized
                                    
         move.l    (%a0),%d0        
         blt.w     slog2s_invalid          
         move.l    %d1,-(%sp)       
         clr.l     %d1              
         bsr.l     slognd           # log(X), X denorm.
         fmove.l   (%sp)+,%fpcr     
ifdef(`PIC',`	fmul.x	&0x3fff0000b8aa3b295c17f0bc,%fp0
',`         fmul.x    slog2s_inv_l2,%fp0      
')
         bra.l     t_frcinx         
                                    
slog2:                              
#--entry point for Log2(X), X is normalized
         move.l    (%a0),%d0        
         blt.w     slog2s_invalid          
                                    
         move.l    8(%a0),%d0       
         bne.b     slog2s_continue         # X is not 2^k
                                    
         move.l    4(%a0),%d0       
         and.l     &0x7fffffff,%d0  
         tst.l     %d0              
         bne.b     slog2s_continue         
                                    
#--X = 2^k.
         move.w    (%a0),%d0        
         and.l     &0x00007fff,%d0  
         sub.l     &0x3fff,%d0      
         fmove.l   %d1,%fpcr        
         fmove.l   %d0,%fp0         
         bra.l     t_frcinx         
                                    
slog2s_continue:                            
         move.l    %d1,-(%sp)       
         clr.l     %d1              
         bsr.l     slogn            # log(X), X normal.
         fmove.l   (%sp)+,%fpcr     
ifdef(`PIC',`	fmul.x	&0x3fff0000b8aa3b295c17f0bc,%fp0
',`         fmul.x    slog2s_inv_l2,%fp0      
')
         bra.l     t_frcinx         
                                    
slog2s_invalid:                            
         bra.l     t_operr          
                                    
                                    
	version 3
